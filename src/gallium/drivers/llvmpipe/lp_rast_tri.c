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


static INLINE unsigned
build_mask(int c, int dcdx, int dcdy)
{
   int mask = 0;

   int c0 = c;
   int c1 = c0 + dcdx;
   int c2 = c1 + dcdx;
   int c3 = c2 + dcdx;

   mask |= ((c0 + 0 * dcdy) >> 31) & (1 << 0);
   mask |= ((c0 + 1 * dcdy) >> 31) & (1 << 2);
   mask |= ((c0 + 2 * dcdy) >> 31) & (1 << 8);
   mask |= ((c0 + 3 * dcdy) >> 31) & (1 << 10);
   mask |= ((c1 + 0 * dcdy) >> 31) & (1 << 1);
   mask |= ((c1 + 1 * dcdy) >> 31) & (1 << 3);
   mask |= ((c1 + 2 * dcdy) >> 31) & (1 << 9);
   mask |= ((c1 + 3 * dcdy) >> 31) & (1 << 11); 
   mask |= ((c2 + 0 * dcdy) >> 31) & (1 << 4);
   mask |= ((c2 + 1 * dcdy) >> 31) & (1 << 6);
   mask |= ((c2 + 2 * dcdy) >> 31) & (1 << 12);
   mask |= ((c2 + 3 * dcdy) >> 31) & (1 << 14);
   mask |= ((c3 + 0 * dcdy) >> 31) & (1 << 5);
   mask |= ((c3 + 1 * dcdy) >> 31) & (1 << 7);
   mask |= ((c3 + 2 * dcdy) >> 31) & (1 << 13);
   mask |= ((c3 + 3 * dcdy) >> 31) & (1 << 15);
  
   return mask;
}

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
   const int x = task->x + (mask & 0xf) * 16;
   const int y = task->y + (mask >> 4) * 16;
   unsigned outmask, inmask, partmask, partial_mask;
   unsigned j;
   int c[3];

   outmask = 0;                 /* outside one or more trivial reject planes */
   partmask = 0;                /* outside one or more trivial accept planes */

   for (j = 0; j < 3; j++) {
      c[j] = plane[j].c + plane[j].dcdy * y - plane[j].dcdx * x;

      {
	 const int dcdx = -plane[j].dcdx * 4;
	 const int dcdy = plane[j].dcdy * 4;
	 const int cox = c[j] + plane[j].eo * 4;
	 const int cio = c[j] + plane[j].ei * 4 - 1;

	 outmask |= build_mask_linear(cox, dcdx, dcdy);
	 partmask |= build_mask_linear(cio, dcdx, dcdy);
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
      int cx[3];

      partial_mask &= ~(1 << i);

      for (j = 0; j < 3; j++)
         cx[j] = (c[j] 
		  - plane[j].dcdx * ix
		  + plane[j].dcdy * iy);

      do_block_4_3(task, tri, plane, px, py, cx);
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
