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
 * Map an index in [0,15] to an x,y position, multiplied by 4.
 * This is used to get the position of each subtile in a 4x4
 * grid of edge step values.
 * Note: we can use some bit twiddling to compute these values instead
 * of using a look-up table, but there's no measurable performance
 * difference.
 */
static const int pos_table4[16][2] = {
   { 0, 0 },
   { 4, 0 },
   { 0, 4 },
   { 4, 4 },
   { 8, 0 },
   { 12, 0 },
   { 8, 4 },
   { 12, 4 },
   { 0, 8 },
   { 4, 8 },
   { 0, 12 },
   { 4, 12 },
   { 8, 8 },
   { 12, 8 },
   { 8, 12 },
   { 12, 12 }
};


static const int pos_table16[16][2] = {
   { 0, 0 },
   { 16, 0 },
   { 0, 16 },
   { 16, 16 },
   { 32, 0 },
   { 48, 0 },
   { 32, 16 },
   { 48, 16 },
   { 0, 32 },
   { 16, 32 },
   { 0, 48 },
   { 16, 48 },
   { 32, 32 },
   { 48, 32 },
   { 32, 48 },
   { 48, 48 }
};


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


/**
 * Pass the 4x4 pixel block to the shader function.
 * Determination of which of the 16 pixels lies inside the triangle
 * will be done as part of the fragment shader.
 */
static void
do_block_4(struct lp_rasterizer_task *task,
           const struct lp_rast_triangle *tri,
           int x, int y,
           int c1, int c2, int c3)
{
   assert(x >= 0);
   assert(y >= 0);

   lp_rast_shade_quads(task, &tri->inputs, x, y, -c1, -c2, -c3);
}


/**
 * Evaluate a 16x16 block of pixels to determine which 4x4 subblocks are in/out
 * of the triangle's bounds.
 */
static void
do_block_16(struct lp_rasterizer_task *task,
            const struct lp_rast_triangle *tri,
            int x, int y,
            int c0, int c1, int c2)
{
   unsigned mask = 0;
   int eo[3];
   int c[3];
   int i, j;

   assert(x >= 0);
   assert(y >= 0);
   assert(x % 16 == 0);
   assert(y % 16 == 0);

   eo[0] = tri->eo1 * 4;
   eo[1] = tri->eo2 * 4;
   eo[2] = tri->eo3 * 4;

   c[0] = c0;
   c[1] = c1;
   c[2] = c2;

   for (j = 0; j < 3; j++) {
      const int *step = tri->inputs.step[j];
      const int cx = c[j] + eo[j];

      /* Mask has bits set whenever we are outside any of the edges.
       */
      for (i = 0; i < 16; i++) {
         int out = cx + step[i] * 4;
         mask |= (out >> 31) & (1 << i);
      }
   }

   mask = ~mask & 0xffff;
   while (mask) {
      int i = ffs(mask) - 1;
      int px = x + pos_table4[i][0];
      int py = y + pos_table4[i][1];
      int cx1 = c0 + tri->inputs.step[0][i] * 4;
      int cx2 = c1 + tri->inputs.step[1][i] * 4;
      int cx3 = c2 + tri->inputs.step[2][i] * 4;

      mask &= ~(1 << i);

      /* Don't bother testing if the 4x4 block is entirely in/out of
       * the triangle.  It's a little faster to do it in the jit code.
       */
      LP_COUNT(nr_non_empty_4);
      do_block_4(task, tri, px, py, cx1, cx2, cx3);
   }
}


/**
 * Scan the tile in chunks and figure out which pixels to rasterize
 * for this triangle.
 */
void
lp_rast_triangle(struct lp_rasterizer_task *task,
                 const union lp_rast_cmd_arg arg)
{
   const struct lp_rast_triangle *tri = arg.triangle;
   const int x = task->x, y = task->y;
   int ei[3], eo[3], c[3];
   unsigned outmask, inmask, partial_mask;
   unsigned i, j;

   c[0] = tri->c1 + tri->dx12 * y - tri->dy12 * x;
   c[1] = tri->c2 + tri->dx23 * y - tri->dy23 * x;
   c[2] = tri->c3 + tri->dx31 * y - tri->dy31 * x;

   eo[0] = tri->eo1 * 16;
   eo[1] = tri->eo2 * 16;
   eo[2] = tri->eo3 * 16;

   ei[0] = tri->ei1 * 16;
   ei[1] = tri->ei2 * 16;
   ei[2] = tri->ei3 * 16;

   outmask = 0;
   inmask = 0xffff;

   for (j = 0; j < 3; j++) {
      const int *step = tri->inputs.step[j];
      const int cox = c[j] + eo[j];
      const int cio = ei[j]- eo[j];

      /* Outmask has bits set whenever we are outside any of the
       * edges.
       */
      /* Inmask has bits set whenever we are inside all of the edges.
       */
      for (i = 0; i < 16; i++) {
         int out = cox + step[i] * 16;
         int in = out + cio;
         outmask |= (out >> 31) & (1 << i);
         inmask &= ~((in >> 31) & (1 << i));
      }
   }

   assert((outmask & inmask) == 0);

   if (outmask == 0xffff)
      return;

   /* Invert mask, so that bits are set whenever we are at least
    * partially inside all of the edges:
    */
   partial_mask = ~inmask & ~outmask & 0xffff;

   /* Iterate over partials:
    */
   while (partial_mask) {
      int i = ffs(partial_mask) - 1;
      int px = x + pos_table16[i][0];
      int py = y + pos_table16[i][1];
      int cx1 = c[0] + tri->inputs.step[0][i] * 16;
      int cx2 = c[1] + tri->inputs.step[1][i] * 16;
      int cx3 = c[2] + tri->inputs.step[2][i] * 16;

      partial_mask &= ~(1 << i);

      LP_COUNT(nr_partially_covered_16);
      do_block_16(task, tri, px, py, cx1, cx2, cx3);
   }

   /* Iterate over fulls: 
    */
   while (inmask) {
      int i = ffs(inmask) - 1;
      int px = x + pos_table16[i][0];
      int py = y + pos_table16[i][1];

      inmask &= ~(1 << i);

      LP_COUNT(nr_fully_covered_16);
      block_full_16(task, tri, px, py);
   }
}
