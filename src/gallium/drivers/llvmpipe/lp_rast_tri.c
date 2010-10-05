/**************************************************************************
 *
 * Copyright 2007-2009 VMware, Inc.
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
 * Rasterization for binned triangles within a tile
 */

#include <limits.h>
#include "util/u_math.h"
#include "lp_debug.h"
#include "lp_perf.h"
#include "lp_rast_priv.h"
#include "lp_tile_soa.h"




/**
 * Shade all pixels in a 4x4 block.
 */
static void
block_full_4(struct lp_rasterizer_task *task,
             const struct lp_rast_triangle *tri,
             int x, int y)
{
   lp_rast_shade_quads_all(task, &tri->inputs, x, y);
}


/**
 * Shade all pixels in a 16x16 block.
 */
static void
block_full_16(struct lp_rasterizer_task *task,
              const struct lp_rast_triangle *tri,
              int x, int y)
{
   unsigned ix, iy;
   assert(x % 16 == 0);
   assert(y % 16 == 0);
   for (iy = 0; iy < 16; iy += 4)
      for (ix = 0; ix < 16; ix += 4)
	 block_full_4(task, tri, x + ix, y + iy);
}

#if !defined(PIPE_ARCH_SSE)

static INLINE unsigned
build_mask_linear(int c, int dcdx, int dcdy)
{
   int mask = 0;

   int c0 = c;
   int c1 = c0 + dcdy;
   int c2 = c1 + dcdy;
   int c3 = c2 + dcdy;

   mask |= ((c0 + 0 * dcdx) >> 31) & (1 << 0);
   mask |= ((c0 + 1 * dcdx) >> 31) & (1 << 1);
   mask |= ((c0 + 2 * dcdx) >> 31) & (1 << 2);
   mask |= ((c0 + 3 * dcdx) >> 31) & (1 << 3);
   mask |= ((c1 + 0 * dcdx) >> 31) & (1 << 4);
   mask |= ((c1 + 1 * dcdx) >> 31) & (1 << 5);
   mask |= ((c1 + 2 * dcdx) >> 31) & (1 << 6);
   mask |= ((c1 + 3 * dcdx) >> 31) & (1 << 7); 
   mask |= ((c2 + 0 * dcdx) >> 31) & (1 << 8);
   mask |= ((c2 + 1 * dcdx) >> 31) & (1 << 9);
   mask |= ((c2 + 2 * dcdx) >> 31) & (1 << 10);
   mask |= ((c2 + 3 * dcdx) >> 31) & (1 << 11);
   mask |= ((c3 + 0 * dcdx) >> 31) & (1 << 12);
   mask |= ((c3 + 1 * dcdx) >> 31) & (1 << 13);
   mask |= ((c3 + 2 * dcdx) >> 31) & (1 << 14);
   mask |= ((c3 + 3 * dcdx) >> 31) & (1 << 15);
  
   return mask;
}


static INLINE void
build_masks(int c, 
	    int cdiff,
	    int dcdx,
	    int dcdy,
	    unsigned *outmask,
	    unsigned *partmask)
{
   *outmask |= build_mask_linear(c, dcdx, dcdy);
   *partmask |= build_mask_linear(c + cdiff, dcdx, dcdy);
}

void
lp_rast_triangle_3_16(struct lp_rasterizer_task *task,
                      const union lp_rast_cmd_arg arg)
{
   union lp_rast_cmd_arg arg2;
   arg2.triangle.tri = arg.triangle.tri;
   arg2.triangle.plane_mask = (1<<3)-1;
   lp_rast_triangle_3(task, arg2);
}

void
lp_rast_triangle_3_4(struct lp_rasterizer_task *task,
                      const union lp_rast_cmd_arg arg)
{
   lp_rast_triangle_3_16(task, arg);
}

#else
#include <emmintrin.h>
#include "util/u_sse.h"


static INLINE void
build_masks(int c, 
	    int cdiff,
	    int dcdx,
	    int dcdy,
	    unsigned *outmask,
	    unsigned *partmask)
{
   __m128i cstep0 = _mm_setr_epi32(c, c+dcdx, c+dcdx*2, c+dcdx*3);
   __m128i xdcdy = _mm_set1_epi32(dcdy);

   /* Get values across the quad
    */
   __m128i cstep1 = _mm_add_epi32(cstep0, xdcdy);
   __m128i cstep2 = _mm_add_epi32(cstep1, xdcdy);
   __m128i cstep3 = _mm_add_epi32(cstep2, xdcdy);

   {
      __m128i cstep01, cstep23, result;

      cstep01 = _mm_packs_epi32(cstep0, cstep1);
      cstep23 = _mm_packs_epi32(cstep2, cstep3);
      result = _mm_packs_epi16(cstep01, cstep23);

      *outmask |= _mm_movemask_epi8(result);
   }


   {
      __m128i cio4 = _mm_set1_epi32(cdiff);
      __m128i cstep01, cstep23, result;

      cstep0 = _mm_add_epi32(cstep0, cio4);
      cstep1 = _mm_add_epi32(cstep1, cio4);
      cstep2 = _mm_add_epi32(cstep2, cio4);
      cstep3 = _mm_add_epi32(cstep3, cio4);

      cstep01 = _mm_packs_epi32(cstep0, cstep1);
      cstep23 = _mm_packs_epi32(cstep2, cstep3);
      result = _mm_packs_epi16(cstep01, cstep23);

      *partmask |= _mm_movemask_epi8(result);
   }
}


static INLINE unsigned
build_mask_linear(int c, int dcdx, int dcdy)
{
   __m128i cstep0 = _mm_setr_epi32(c, c+dcdx, c+dcdx*2, c+dcdx*3);
   __m128i xdcdy = _mm_set1_epi32(dcdy);

   /* Get values across the quad
    */
   __m128i cstep1 = _mm_add_epi32(cstep0, xdcdy);
   __m128i cstep2 = _mm_add_epi32(cstep1, xdcdy);
   __m128i cstep3 = _mm_add_epi32(cstep2, xdcdy);

   /* pack pairs of results into epi16
    */
   __m128i cstep01 = _mm_packs_epi32(cstep0, cstep1);
   __m128i cstep23 = _mm_packs_epi32(cstep2, cstep3);

   /* pack into epi8, preserving sign bits
    */
   __m128i result = _mm_packs_epi16(cstep01, cstep23);

   /* extract sign bits to create mask
    */
   return _mm_movemask_epi8(result);
}

static INLINE unsigned
sign_bits4(const __m128i *cstep, int cdiff)
{

   /* Adjust the step values
    */
   __m128i cio4 = _mm_set1_epi32(cdiff);
   __m128i cstep0 = _mm_add_epi32(cstep[0], cio4);
   __m128i cstep1 = _mm_add_epi32(cstep[1], cio4);
   __m128i cstep2 = _mm_add_epi32(cstep[2], cio4);
   __m128i cstep3 = _mm_add_epi32(cstep[3], cio4);

   /* Pack down to epi8
    */
   __m128i cstep01 = _mm_packs_epi32(cstep0, cstep1);
   __m128i cstep23 = _mm_packs_epi32(cstep2, cstep3);
   __m128i result = _mm_packs_epi16(cstep01, cstep23);

   /* Extract the sign bits
    */
   return _mm_movemask_epi8(result);
}


/* Special case for 3 plane triangle which is contained entirely
 * within a 16x16 block.
 */
void
lp_rast_triangle_3_16(struct lp_rasterizer_task *task,
                      const union lp_rast_cmd_arg arg)
{
   const struct lp_rast_triangle *tri = arg.triangle.tri;
   const struct lp_rast_plane *plane = tri->plane;
   unsigned mask = arg.triangle.plane_mask;
   const int x = task->x + (mask & 0xff);
   const int y = task->y + (mask >> 8);
   unsigned outmask, inmask, partmask, partial_mask;
   unsigned j;
   __m128i cstep4[3][4];

   outmask = 0;                 /* outside one or more trivial reject planes */
   partmask = 0;                /* outside one or more trivial accept planes */

   for (j = 0; j < 3; j++) {
      const int dcdx = -plane[j].dcdx * 4;
      const int dcdy = plane[j].dcdy * 4;
      __m128i xdcdy = _mm_set1_epi32(dcdy);

      cstep4[j][0] = _mm_setr_epi32(0, dcdx, dcdx*2, dcdx*3);
      cstep4[j][1] = _mm_add_epi32(cstep4[j][0], xdcdy);
      cstep4[j][2] = _mm_add_epi32(cstep4[j][1], xdcdy);
      cstep4[j][3] = _mm_add_epi32(cstep4[j][2], xdcdy);

      {
	 const int c = plane[j].c + plane[j].dcdy * y - plane[j].dcdx * x;
	 const int cox = plane[j].eo * 4;
	 const int cio = plane[j].ei * 4 - 1;

	 outmask |= sign_bits4(cstep4[j], c + cox);
	 partmask |= sign_bits4(cstep4[j], c + cio);
      }
   }

   if (outmask == 0xffff)
      return;

   /* Mask of sub-blocks which are inside all trivial accept planes:
    */
   inmask = ~partmask & 0xffff;

   /* Mask of sub-blocks which are inside all trivial reject planes,
    * but outside at least one trivial accept plane:
    */
   partial_mask = partmask & ~outmask;

   assert((partial_mask & inmask) == 0);

   /* Iterate over partials:
    */
   while (partial_mask) {
      int i = ffs(partial_mask) - 1;
      int ix = (i & 3) * 4;
      int iy = (i >> 2) * 4;
      int px = x + ix;
      int py = y + iy; 
      unsigned mask = 0xffff;

      partial_mask &= ~(1 << i);

      for (j = 0; j < 3; j++) {
         const int cx = (plane[j].c 
			 - plane[j].dcdx * px
			 + plane[j].dcdy * py) * 4;

	 mask &= ~sign_bits4(cstep4[j], cx);
      }

      if (mask)
	 lp_rast_shade_quads_mask(task, &tri->inputs, px, py, mask);
   }

   /* Iterate over fulls: 
    */
   while (inmask) {
      int i = ffs(inmask) - 1;
      int ix = (i & 3) * 4;
      int iy = (i >> 2) * 4;
      int px = x + ix;
      int py = y + iy; 

      inmask &= ~(1 << i);

      block_full_4(task, tri, px, py);
   }
}


void
lp_rast_triangle_3_4(struct lp_rasterizer_task *task,
		     const union lp_rast_cmd_arg arg)
{
   const struct lp_rast_triangle *tri = arg.triangle.tri;
   const struct lp_rast_plane *plane = tri->plane;
   unsigned mask = arg.triangle.plane_mask;
   const int x = task->x + (mask & 0xff);
   const int y = task->y + (mask >> 8);
   unsigned j;

   /* Iterate over partials:
    */
   {
      unsigned mask = 0xffff;

      for (j = 0; j < 3; j++) {
	 const int cx = (plane[j].c 
			 - plane[j].dcdx * x
			 + plane[j].dcdy * y);

	 const int dcdx = -plane[j].dcdx;
	 const int dcdy = plane[j].dcdy;
	 __m128i xdcdy = _mm_set1_epi32(dcdy);

	 __m128i cstep0 = _mm_setr_epi32(cx, cx + dcdx, cx + dcdx*2, cx + dcdx*3);
	 __m128i cstep1 = _mm_add_epi32(cstep0, xdcdy);
	 __m128i cstep2 = _mm_add_epi32(cstep1, xdcdy);
	 __m128i cstep3 = _mm_add_epi32(cstep2, xdcdy);

	 __m128i cstep01 = _mm_packs_epi32(cstep0, cstep1);
	 __m128i cstep23 = _mm_packs_epi32(cstep2, cstep3);
	 __m128i result = _mm_packs_epi16(cstep01, cstep23);

	 /* Extract the sign bits
	  */
	 mask &= ~_mm_movemask_epi8(result);
      }

      if (mask)
	 lp_rast_shade_quads_mask(task, &tri->inputs, x, y, mask);
   }
}


#endif




#define TAG(x) x##_1
#define NR_PLANES 1
#include "lp_rast_tri_tmp.h"

#define TAG(x) x##_2
#define NR_PLANES 2
#include "lp_rast_tri_tmp.h"

#define TAG(x) x##_3
#define NR_PLANES 3
#include "lp_rast_tri_tmp.h"

#define TAG(x) x##_4
#define NR_PLANES 4
#include "lp_rast_tri_tmp.h"

#define TAG(x) x##_5
#define NR_PLANES 5
#include "lp_rast_tri_tmp.h"

#define TAG(x) x##_6
#define NR_PLANES 6
#include "lp_rast_tri_tmp.h"

#define TAG(x) x##_7
#define NR_PLANES 7
#include "lp_rast_tri_tmp.h"

#define TAG(x) x##_8
#define NR_PLANES 8
#include "lp_rast_tri_tmp.h"

