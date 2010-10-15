/**************************************************************************
 *
 * Copyright 2010, VMware.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/*
 * Binning code for triangles
 */

#include "util/u_math.h"
#include "util/u_memory.h"
#include "lp_perf.h"
#include "lp_setup_context.h"
#include "lp_setup_coef.h"
#include "lp_rast.h"
#include "lp_state_fs.h"

#if !defined(PIPE_ARCH_SSE)

/**
 * Compute a0 for a constant-valued coefficient (GL_FLAT shading).
 */
static void constant_coef( struct lp_tri_info *info,
                           unsigned slot,
			   const float value,
                           unsigned i )
{
   info->a0[slot][i] = value;
   info->dadx[slot][i] = 0.0f;
   info->dady[slot][i] = 0.0f;
}



static void linear_coef( struct lp_tri_info *info,
                         unsigned slot,
                         unsigned vert_attr,
                         unsigned i)
{
   float a0 = info->v0[vert_attr][i];
   float a1 = info->v1[vert_attr][i];
   float a2 = info->v2[vert_attr][i];

   float da01 = a0 - a1;
   float da20 = a2 - a0;
   float dadx = (da01 * info->dy20_ooa - info->dy01_ooa * da20);
   float dady = (da20 * info->dx01_ooa - info->dx20_ooa * da01);

   info->dadx[slot][i] = dadx;
   info->dady[slot][i] = dady;

   /* calculate a0 as the value which would be sampled for the
    * fragment at (0,0), taking into account that we want to sample at
    * pixel centers, in other words (0.5, 0.5).
    *
    * this is neat but unfortunately not a good way to do things for
    * triangles with very large values of dadx or dady as it will
    * result in the subtraction and re-addition from a0 of a very
    * large number, which means we'll end up loosing a lot of the
    * fractional bits and precision from a0.  the way to fix this is
    * to define a0 as the sample at a pixel center somewhere near vmin
    * instead - i'll switch to this later.
    */
   info->a0[slot][i] = a0 - (dadx * info->x0_center +
				   dady * info->y0_center);
}


/**
 * Compute a0, dadx and dady for a perspective-corrected interpolant,
 * for a triangle.
 * We basically multiply the vertex value by 1/w before computing
 * the plane coefficients (a0, dadx, dady).
 * Later, when we compute the value at a particular fragment position we'll
 * divide the interpolated value by the interpolated W at that fragment.
 */
static void perspective_coef( struct lp_tri_info *info,
                              unsigned slot,
			      unsigned vert_attr,
                              unsigned i)
{
   /* premultiply by 1/w  (v[0][3] is always 1/w):
    */
   float a0 = info->v0[vert_attr][i] * info->v0[0][3];
   float a1 = info->v1[vert_attr][i] * info->v1[0][3];
   float a2 = info->v2[vert_attr][i] * info->v2[0][3];
   float da01 = a0 - a1;
   float da20 = a2 - a0;
   float dadx = da01 * info->dy20_ooa - info->dy01_ooa * da20;
   float dady = da20 * info->dx01_ooa - info->dx20_ooa * da01;

   info->dadx[slot][i] = dadx;
   info->dady[slot][i] = dady;
   info->a0[slot][i] = a0 - (dadx * info->x0_center +
				   dady * info->y0_center);
}


/**
 * Special coefficient setup for gl_FragCoord.
 * X and Y are trivial
 * Z and W are copied from position_coef which should have already been computed.
 * We could do a bit less work if we'd examine gl_FragCoord's swizzle mask.
 */
static void
setup_fragcoord_coef(struct lp_tri_info *info,
                     unsigned slot,
                     unsigned usage_mask)
{
   /*X*/
   if (usage_mask & TGSI_WRITEMASK_X) {
      info->a0[slot][0] = 0.0;
      info->dadx[slot][0] = 1.0;
      info->dady[slot][0] = 0.0;
   }

   /*Y*/
   if (usage_mask & TGSI_WRITEMASK_Y) {
      info->a0[slot][1] = 0.0;
      info->dadx[slot][1] = 0.0;
      info->dady[slot][1] = 1.0;
   }

   /*Z*/
   if (usage_mask & TGSI_WRITEMASK_Z) {
      linear_coef(inputs, info, slot, 0, 2);
   }

   /*W*/
   if (usage_mask & TGSI_WRITEMASK_W) {
      linear_coef(inputs, info, slot, 0, 3);
   }
}


/**
 * Setup the fragment input attribute with the front-facing value.
 * \param frontface  is the triangle front facing?
 */
static void setup_facing_coef( struct lp_tri_info *info,
                               unsigned slot,
                               boolean frontface,
                               unsigned usage_mask)
{
   /* convert TRUE to 1.0 and FALSE to -1.0 */
   if (usage_mask & TGSI_WRITEMASK_X)
      constant_coef( info, slot, 2.0f * frontface - 1.0f, 0 );

   if (usage_mask & TGSI_WRITEMASK_Y)
      constant_coef( info, slot, 0.0f, 1 ); /* wasted */

   if (usage_mask & TGSI_WRITEMASK_Z)
      constant_coef( info, slot, 0.0f, 2 ); /* wasted */

   if (usage_mask & TGSI_WRITEMASK_W)
      constant_coef( info, slot, 0.0f, 3 ); /* wasted */
}


/**
 * Compute the tri->coef[] array dadx, dady, a0 values.
 */
void lp_setup_tri_coef( struct lp_setup_context *setup,
			struct lp_rast_shader_inputs *inputs,
                        const float (*v0)[4],
                        const float (*v1)[4],
                        const float (*v2)[4],
                        boolean frontfacing)
{
   unsigned fragcoord_usage_mask = TGSI_WRITEMASK_XYZ;
   unsigned slot;
   unsigned i;
   struct lp_tri_info info;
   float dx01 = v0[0][0] - v1[0][0];
   float dy01 = v0[0][1] - v1[0][1];
   float dx20 = v2[0][0] - v0[0][0];
   float dy20 = v2[0][1] - v0[0][1];
   float oneoverarea = 1.0f / (dx01 * dy20 - dx20 * dy01);

   info.v0 = v0;
   info.v1 = v1;
   info.v2 = v2;
   info.frontfacing = frontfacing;
   info.x0_center = v0[0][0] - setup->pixel_offset;
   info.y0_center = v0[0][1] - setup->pixel_offset;
   info.dx01_ooa  = dx01 * oneoverarea;
   info.dx20_ooa  = dx20 * oneoverarea;
   info.dy01_ooa  = dy01 * oneoverarea;
   info.dy20_ooa  = dy20 * oneoverarea;
   info.a0 = GET_A0(inputs);
   info.dadx = GET_DADX(inputs);
   info.dady = GET_DADY(inputs);
      


   /* setup interpolation for all the remaining attributes:
    */
   for (slot = 0; slot < setup->fs.nr_inputs; slot++) {
      unsigned vert_attr = setup->fs.input[slot].src_index;
      unsigned usage_mask = setup->fs.input[slot].usage_mask;

      switch (setup->fs.input[slot].interp) {
      case LP_INTERP_CONSTANT:
         if (setup->flatshade_first) {
            for (i = 0; i < NUM_CHANNELS; i++)
               if (usage_mask & (1 << i))
                  constant_coef(&info, slot+1, info.v0[vert_attr][i], i);
         }
         else {
            for (i = 0; i < NUM_CHANNELS; i++)
               if (usage_mask & (1 << i))
                  constant_coef(&info, slot+1, info.v2[vert_attr][i], i);
         }
         break;

      case LP_INTERP_LINEAR:
         for (i = 0; i < NUM_CHANNELS; i++)
            if (usage_mask & (1 << i))
               linear_coef(&info, slot+1, vert_attr, i);
         break;

      case LP_INTERP_PERSPECTIVE:
         for (i = 0; i < NUM_CHANNELS; i++)
            if (usage_mask & (1 << i))
               perspective_coef(&info, slot+1, vert_attr, i);
         fragcoord_usage_mask |= TGSI_WRITEMASK_W;
         break;

      case LP_INTERP_POSITION:
         /*
          * The generated pixel interpolators will pick up the coeffs from
          * slot 0, so all need to ensure that the usage mask is covers all
          * usages.
          */
         fragcoord_usage_mask |= usage_mask;
         break;

      case LP_INTERP_FACING:
         setup_facing_coef(&info, slot+1, info.frontfacing, usage_mask);
         break;

      default:
         assert(0);
      }
   }

   /* The internal position input is in slot zero:
    */
   setup_fragcoord_coef(&info, 0, fragcoord_usage_mask);
}

#else
extern void lp_setup_coef_dummy(void);
void lp_setup_coef_dummy(void)
{
}

#endif
