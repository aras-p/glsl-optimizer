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
 * Fallback (non-llvm) path for triangle setup.  Will remove once llvm
 * is up and running.
 *
 * TODO: line/point setup.
 */

#include "util/u_math.h"
#include "util/u_memory.h"
#include "lp_state_setup.h"



#if defined(PIPE_ARCH_SSE)
#include <emmintrin.h>

struct setup_args {
   float (*a0)[4];		/* aligned */
   float (*dadx)[4];		/* aligned */
   float (*dady)[4];		/* aligned */

   float x0_center;
   float y0_center;

   /* turn these into an aligned float[4] */
   float dy01_ooa;
   float dy20_ooa;
   float dx01_ooa;
   float dx20_ooa;

   const float (*v0)[4];	/* aligned */
   const float (*v1)[4];	/* aligned */
   const float (*v2)[4];	/* aligned */

   boolean frontfacing;		/* remove eventually */
};


static void constant_coef4( struct setup_args *args,
			    unsigned slot,
			    const float *attr)
{
   *(__m128 *)args->a0[slot]   = *(__m128 *)attr;
   *(__m128 *)args->dadx[slot] = _mm_set1_ps(0.0);
   *(__m128 *)args->dady[slot] = _mm_set1_ps(0.0);
}



/**
 * Setup the fragment input attribute with the front-facing value.
 * \param frontface  is the triangle front facing?
 */
static void setup_facing_coef( struct setup_args *args,
			       unsigned slot )
{
   /* XXX: just pass frontface directly to the shader, don't bother
    * treating it as an input.
    */
   __m128 a0 = _mm_setr_ps(args->frontfacing ? 1.0 : -1.0,
			   0, 0, 0);

   *(__m128 *)args->a0[slot]   = a0;
   *(__m128 *)args->dadx[slot] = _mm_set1_ps(0.0);
   *(__m128 *)args->dady[slot] = _mm_set1_ps(0.0);
}



static void calc_coef4( struct setup_args *args,
			unsigned slot,
			__m128 a0,
			__m128 a1,
			__m128 a2)
{
   __m128 da01          = _mm_sub_ps(a0, a1);
   __m128 da20          = _mm_sub_ps(a2, a0);

   __m128 da01_dy20_ooa = _mm_mul_ps(da01, _mm_set1_ps(args->dy20_ooa));
   __m128 da20_dy01_ooa = _mm_mul_ps(da20, _mm_set1_ps(args->dy01_ooa));   
   __m128 dadx          = _mm_sub_ps(da01_dy20_ooa, da20_dy01_ooa);

   __m128 da01_dx20_ooa = _mm_mul_ps(da01, _mm_set1_ps(args->dx20_ooa));
   __m128 da20_dx01_ooa = _mm_mul_ps(da20, _mm_set1_ps(args->dx01_ooa));
   __m128 dady          = _mm_sub_ps(da20_dx01_ooa, da01_dx20_ooa);

   __m128 dadx_x0       = _mm_mul_ps(dadx, _mm_set1_ps(args->x0_center));
   __m128 dady_y0       = _mm_mul_ps(dady, _mm_set1_ps(args->y0_center));
   __m128 attr_v0       = _mm_add_ps(dadx_x0, dady_y0);
   __m128 attr_0        = _mm_sub_ps(a0, attr_v0);

   *(__m128 *)args->a0[slot]   = attr_0;
   *(__m128 *)args->dadx[slot] = dadx;
   *(__m128 *)args->dady[slot] = dady;
}


static void linear_coef( struct setup_args *args,
                         unsigned slot,
                         unsigned vert_attr)
{
   __m128 a0 = *(const __m128 *)args->v0[vert_attr];
   __m128 a1 = *(const __m128 *)args->v1[vert_attr];
   __m128 a2 = *(const __m128 *)args->v2[vert_attr];

   calc_coef4(args, slot, a0, a1, a2);
}



/**
 * Compute a0, dadx and dady for a perspective-corrected interpolant,
 * for a triangle.
 * We basically multiply the vertex value by 1/w before computing
 * the plane coefficients (a0, dadx, dady).
 * Later, when we compute the value at a particular fragment position we'll
 * divide the interpolated value by the interpolated W at that fragment.
 */
static void perspective_coef( struct setup_args *args,
                              unsigned slot,
			      unsigned vert_attr)
{
   /* premultiply by 1/w  (v[0][3] is always 1/w):
    */
   __m128 a0 = *(const __m128 *)args->v0[vert_attr];
   __m128 a1 = *(const __m128 *)args->v1[vert_attr];
   __m128 a2 = *(const __m128 *)args->v2[vert_attr];

   __m128 a0_oow = _mm_mul_ps(a0, _mm_set1_ps(args->v0[0][3]));
   __m128 a1_oow = _mm_mul_ps(a1, _mm_set1_ps(args->v1[0][3]));
   __m128 a2_oow = _mm_mul_ps(a2, _mm_set1_ps(args->v2[0][3]));

   calc_coef4(args, slot, a0_oow, a1_oow, a2_oow);
}





/**
 * Compute the args-> dadx, dady, a0 values.
 *
 * Note that this was effectively a little interpreted program, where
 * the opcodes were LP_INTERP_*.  This is the program which is now
 * being code-generated in lp_state_setup.c.
 */
void lp_setup_tri_fallback( const float (*v0)[4],
			    const float (*v1)[4],
			    const float (*v2)[4],
			    boolean front_facing,
			    float (*a0)[4],
			    float (*dadx)[4],
			    float (*dady)[4],
			    const struct lp_setup_variant_key *key )
{
   struct setup_args args;
   float pixel_offset = key->pixel_center_half ? 0.5 : 0.0;
   float dx01 = v0[0][0] - v1[0][0];
   float dy01 = v0[0][1] - v1[0][1];
   float dx20 = v2[0][0] - v0[0][0];
   float dy20 = v2[0][1] - v0[0][1];
   float oneoverarea = 1.0f / (dx01 * dy20 - dx20 * dy01);
   unsigned slot;

   args.v0 = v0;
   args.v1 = v1;
   args.v2 = v2;
   args.frontfacing = front_facing;
   args.a0 = a0;
   args.dadx = dadx;
   args.dady = dady;

   args.x0_center = v0[0][0] - pixel_offset;
   args.y0_center = v0[0][1] - pixel_offset;
   args.dx01_ooa  = dx01 * oneoverarea;
   args.dx20_ooa  = dx20 * oneoverarea;
   args.dy01_ooa  = dy01 * oneoverarea;
   args.dy20_ooa  = dy20 * oneoverarea;

   /* The internal position input is in slot zero:
    */
   linear_coef(&args, 0, 0);

   /* setup interpolation for all the remaining attributes:
    */
   for (slot = 0; slot < key->num_inputs; slot++) {
      unsigned vert_attr = key->inputs[slot].src_index;

      switch (key->inputs[slot].interp) {
      case LP_INTERP_CONSTANT:
         if (key->flatshade_first) {
	    constant_coef4(&args, slot+1, args.v0[vert_attr]);
         }
         else {
	    constant_coef4(&args, slot+1, args.v2[vert_attr]);
         }
         break;

      case LP_INTERP_LINEAR:
	 linear_coef(&args, slot+1, vert_attr);
         break;

      case LP_INTERP_PERSPECTIVE:
	 perspective_coef(&args, slot+1, vert_attr);
         break;

      case LP_INTERP_POSITION:
         /*
          * The generated pixel interpolators will pick up the coeffs from
          * slot 0.
          */
         break;

      case LP_INTERP_FACING:
         setup_facing_coef(&args, slot+1);
         break;

      default:
         assert(0);
      }
   }
}

#else

void lp_setup_tri_fallback( const float (*v0)[4],
			    const float (*v1)[4],
			    const float (*v2)[4],
			    boolean front_facing,
			    float (*a0)[4],
			    float (*dadx)[4],
			    float (*dady)[4],
			    const struct lp_setup_variant_key *key )
{
   /* this path for debugging only, don't need a non-sse version. */
}

#endif
