/**************************************************************************
 *
 * Copyright 2010 VMware.
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

#if defined(PIPE_ARCH_SSE)
#include <emmintrin.h>


static void constant_coef4( struct lp_rast_shader_inputs *inputs,
			    const struct lp_tri_info *info,
			    unsigned slot,
			    const float *attr)
{
   *(__m128 *)inputs->a0[slot]   = *(__m128 *)attr;
   *(__m128 *)inputs->dadx[slot] = _mm_set1_ps(0.0);
   *(__m128 *)inputs->dady[slot] = _mm_set1_ps(0.0);
}



/**
 * Setup the fragment input attribute with the front-facing value.
 * \param frontface  is the triangle front facing?
 */
static void setup_facing_coef( struct lp_rast_shader_inputs *inputs,
			       const struct lp_tri_info *info,
			       unsigned slot )
{
   /* XXX: just pass frontface directly to the shader, don't bother
    * treating it as an input.
    */
   __m128 a0 = _mm_setr_ps(info->frontfacing ? 1.0 : -1.0,
			   0, 0, 0);

   *(__m128 *)inputs->a0[slot]   = a0;
   *(__m128 *)inputs->dadx[slot] = _mm_set1_ps(0.0);
   *(__m128 *)inputs->dady[slot] = _mm_set1_ps(0.0);
}



static void calc_coef4( struct lp_rast_shader_inputs *inputs,
			const struct lp_tri_info *info,
			unsigned slot,
			__m128 a0,
			__m128 a1,
			__m128 a2)
{
   __m128 da01          = _mm_sub_ps(a0, a1);
   __m128 da20          = _mm_sub_ps(a2, a0);

   __m128 da01_dy20_ooa = _mm_mul_ps(da01, _mm_set1_ps(info->dy20_ooa));
   __m128 da20_dy01_ooa = _mm_mul_ps(da20, _mm_set1_ps(info->dy01_ooa));   
   __m128 dadx          = _mm_sub_ps(da01_dy20_ooa, da20_dy01_ooa);

   __m128 da01_dx20_ooa = _mm_mul_ps(da01, _mm_set1_ps(info->dx20_ooa));
   __m128 da20_dx01_ooa = _mm_mul_ps(da20, _mm_set1_ps(info->dx01_ooa));
   __m128 dady          = _mm_sub_ps(da20_dx01_ooa, da01_dx20_ooa);

   __m128 dadx_x0       = _mm_mul_ps(dadx, _mm_set1_ps(info->x0_center));
   __m128 dady_y0       = _mm_mul_ps(dady, _mm_set1_ps(info->y0_center));
   __m128 attr_v0       = _mm_add_ps(dadx_x0, dady_y0);
   __m128 attr_0        = _mm_sub_ps(a0, attr_v0);

   *(__m128 *)inputs->a0[slot]   = attr_0;
   *(__m128 *)inputs->dadx[slot] = dadx;
   *(__m128 *)inputs->dady[slot] = dady;
}


static void linear_coef( struct lp_rast_shader_inputs *inputs,
                         const struct lp_tri_info *info,
                         unsigned slot,
                         unsigned vert_attr)
{
   __m128 a0 = *(const __m128 *)info->v0[vert_attr];
   __m128 a1 = *(const __m128 *)info->v1[vert_attr];
   __m128 a2 = *(const __m128 *)info->v2[vert_attr];

   calc_coef4(inputs, info, slot, a0, a1, a2);
}



/**
 * Compute a0, dadx and dady for a perspective-corrected interpolant,
 * for a triangle.
 * We basically multiply the vertex value by 1/w before computing
 * the plane coefficients (a0, dadx, dady).
 * Later, when we compute the value at a particular fragment position we'll
 * divide the interpolated value by the interpolated W at that fragment.
 */
static void perspective_coef( struct lp_rast_shader_inputs *inputs,
                              const struct lp_tri_info *info,
                              unsigned slot,
			      unsigned vert_attr)
{
   /* premultiply by 1/w  (v[0][3] is always 1/w):
    */
   __m128 a0 = *(const __m128 *)info->v0[vert_attr];
   __m128 a1 = *(const __m128 *)info->v1[vert_attr];
   __m128 a2 = *(const __m128 *)info->v2[vert_attr];

   __m128 a0_oow = _mm_mul_ps(a0, _mm_set1_ps(info->v0[0][3]));
   __m128 a1_oow = _mm_mul_ps(a1, _mm_set1_ps(info->v1[0][3]));
   __m128 a2_oow = _mm_mul_ps(a2, _mm_set1_ps(info->v2[0][3]));

   calc_coef4(inputs, info, slot, a0_oow, a1_oow, a2_oow);
}





/**
 * Compute the inputs-> dadx, dady, a0 values.
 */
void lp_setup_tri_coef( struct lp_setup_context *setup,
			struct lp_rast_shader_inputs *inputs,
                        const float (*v0)[4],
                        const float (*v1)[4],
                        const float (*v2)[4],
                        boolean frontfacing)
{
   unsigned slot;
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


   /* The internal position input is in slot zero:
    */
   linear_coef(inputs, &info, 0, 0);

   /* setup interpolation for all the remaining attributes:
    */
   for (slot = 0; slot < setup->fs.nr_inputs; slot++) {
      unsigned vert_attr = setup->fs.input[slot].src_index;

      switch (setup->fs.input[slot].interp) {
      case LP_INTERP_CONSTANT:
         if (setup->flatshade_first) {
	    constant_coef4(inputs, &info, slot+1, info.v0[vert_attr]);
         }
         else {
	    constant_coef4(inputs, &info, slot+1, info.v2[vert_attr]);
         }
         break;

      case LP_INTERP_LINEAR:
	 linear_coef(inputs, &info, slot+1, vert_attr);
         break;

      case LP_INTERP_PERSPECTIVE:
	 perspective_coef(inputs, &info, slot+1, vert_attr);
         break;

      case LP_INTERP_POSITION:
         /*
          * The generated pixel interpolators will pick up the coeffs from
          * slot 0.
          */
         break;

      case LP_INTERP_FACING:
         setup_facing_coef(inputs, &info, slot+1);
         break;

      default:
         assert(0);
      }
   }
}

#else
extern void lp_setup_coef_dummy(void);
void lp_setup_coef_dummy(void)
{
}
#endif
