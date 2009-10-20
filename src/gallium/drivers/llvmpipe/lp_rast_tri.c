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

#include "util/u_math.h"
#include "lp_rast_priv.h"
#include "lp_tile_soa.h"


#define BLOCKSIZE 4



/* Render a 4x4 unmasked block:
 */
static void block_full( struct lp_rasterizer *rast,
                        const struct lp_rast_triangle *tri,
                        int x, int y )
{
   unsigned mask = ~0;

   lp_rast_shade_quads(rast, &tri->inputs, x, y, mask);
}



/* Evaluate each pixel in a block, generate a mask and possibly render
 * the quad:
 */
static void
do_block( struct lp_rasterizer *rast,
	  const struct lp_rast_triangle *tri,
          int x, int y,
          int c1,
          int c2,
          int c3 )
{
   int i;
   unsigned mask = 0;

   for (i = 0; i < 16; i++)
      mask |= (~(((c1 + tri->step[0][i]) | 
		  (c2 + tri->step[1][i]) | 
		  (c3 + tri->step[2][i])) >> 31)) & (1 << i);
   

   /* As we do trivial reject already, masks should rarely be all
    * zero:
    */
   lp_rast_shade_quads(rast, &tri->inputs, x, y, mask );
}



/* Scan the tile in chunks and figure out which pixels to rasterize
 * for this triangle:
 */
void lp_rast_triangle( struct lp_rasterizer *rast,
                       const union lp_rast_cmd_arg arg )
{
   const struct lp_rast_triangle *tri = arg.triangle;

   const int step = BLOCKSIZE;

   int ei1 = tri->ei1 * step;
   int ei2 = tri->ei2 * step;
   int ei3 = tri->ei3 * step;

   int eo1 = tri->eo1 * step;
   int eo2 = tri->eo2 * step;
   int eo3 = tri->eo3 * step;

   int xstep1 = -step * tri->dy12;
   int xstep2 = -step * tri->dy23;
   int xstep3 = -step * tri->dy31;

   int ystep1 = step * tri->dx12;
   int ystep2 = step * tri->dx23;
   int ystep3 = step * tri->dx31;

   /* Clamp to tile dimensions:
    */
   int minx = MAX2(tri->minx, rast->x);
   int miny = MAX2(tri->miny, rast->y);
   int maxx = MIN2(tri->maxx, rast->x + TILE_SIZE);
   int maxy = MIN2(tri->maxy, rast->y + TILE_SIZE);

   int x, y;
   int c1, c2, c3;

   debug_printf("%s\n", __FUNCTION__);

   if (miny == maxy || minx == maxx) {
      debug_printf("%s: non-intersecting triangle in bin\n", __FUNCTION__);
      return;
   }

   minx &= ~(BLOCKSIZE-1);
   miny &= ~(BLOCKSIZE-1);

   c1 = tri->c1 + tri->dx12 * miny - tri->dy12 * minx;
   c2 = tri->c2 + tri->dx23 * miny - tri->dy23 * minx;
   c3 = tri->c3 + tri->dx31 * miny - tri->dy31 * minx;

   for (y = miny; y < maxy; y += BLOCKSIZE)
   {
      int cx1 = c1;
      int cx2 = c2;
      int cx3 = c3;

      for (x = minx; x < maxx; x += BLOCKSIZE)
      {
         if (cx1 + eo1 < 0 ||
             cx2 + eo2 < 0 ||
             cx3 + eo3 < 0)
         {
         }
         else if (cx1 + ei1 > 0 &&
                  cx2 + ei2 > 0 &&
                  cx3 + ei3 > 0)
         {
            block_full(rast, tri, x, y); /* trivial accept */
         }
         else
         {
            do_block(rast, tri, x, y, cx1, cx2, cx3);
         }

         /* Iterate cx values across the region:
          */
         cx1 += xstep1;
         cx2 += xstep2;
         cx3 += xstep3;
      }

      /* Iterate c values down the region:
       */
      c1 += ystep1;
      c2 += ystep2;
      c3 += ystep3;
   }
}

