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


/* Convert 8x8 block into four runs of quads and render each in turn.
 */
#if (BLOCKSIZE == 8)
static void block_full( struct lp_rasterizer *rast,
                        const struct lp_rast_triangle *tri,
                        int x, int y )
{
   const unsigned masks[4] = {~0, ~0, ~0, ~0};
   int iy;

   for (iy = 0; iy < 8; iy += 2)
      lp_rast_shade_quads(rast, &tri->inputs, x, y + iy, masks);
}
#else
static void block_full( struct lp_rasterizer *rast,
                        const struct lp_rast_triangle *tri,
                        int x, int y )
{
   const unsigned masks[4] = {~0, ~0, ~0, ~0};

   lp_rast_shade_quads(rast, &tri->inputs, x, y, masks);
}
#endif

static INLINE unsigned
do_quad( const struct lp_rast_triangle *tri,
	 int x, int y,
	 int c1, int c2, int c3 )
{
   const int xstep1 = -tri->dy12 * FIXED_ONE;
   const int xstep2 = -tri->dy23 * FIXED_ONE;
   const int xstep3 = -tri->dy31 * FIXED_ONE;

   const int ystep1 = tri->dx12 * FIXED_ONE;
   const int ystep2 = tri->dx23 * FIXED_ONE;
   const int ystep3 = tri->dx31 * FIXED_ONE;

   unsigned mask = 0;

   if (c1 > 0 &&
       c2 > 0 &&
       c3 > 0)
      mask |= 1;
	 
   if (c1 + xstep1 > 0 && 
       c2 + xstep2 > 0 && 
       c3 + xstep3 > 0)
      mask |= 2;

   if (c1 + ystep1 > 0 && 
       c2 + ystep2 > 0 && 
       c3 + ystep3 > 0)
      mask |= 4;

   if (c1 + ystep1 + xstep1 > 0 && 
       c2 + ystep2 + xstep2 > 0 && 
       c3 + ystep3 + xstep3 > 0)
      mask |= 8;

   return mask;
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
   const int step = 2 * FIXED_ONE;

   const int xstep1 = -step * tri->dy12;
   const int xstep2 = -step * tri->dy23;
   const int xstep3 = -step * tri->dy31;

   const int ystep1 = step * tri->dx12;
   const int ystep2 = step * tri->dx23;
   const int ystep3 = step * tri->dx31;

   int ix, iy;

   unsigned masks[2][2] = {{0, 0}, {0, 0}};

   for (iy = 0; iy < BLOCKSIZE; iy += 2) {
      int cx1 = c1;
      int cx2 = c2;
      int cx3 = c3;

      for (ix = 0; ix < BLOCKSIZE; ix += 2) {

	 masks[iy >> 1][ix >> 1] = do_quad(tri, x + ix, y + iy, cx1, cx2, cx3);

	 cx1 += xstep1;
	 cx2 += xstep2;
	 cx3 += xstep3;
      }

      c1 += ystep1;
      c2 += ystep2;
      c3 += ystep3;
   }

   if(masks[0][0] || masks[0][1] || masks[1][0] || masks[1][1])
      lp_rast_shade_quads(rast, &tri->inputs, x, y, &masks[0][0]);
}



/* Scan the tile in chunks and figure out which pixels to rasterize
 * for this triangle:
 */
void lp_rast_triangle( struct lp_rasterizer *rast,
                       const union lp_rast_cmd_arg arg )
{
   const struct lp_rast_triangle *tri = arg.triangle;

   const int step = BLOCKSIZE * FIXED_ONE;

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
   int x0, y0;
   int c1, c2, c3;

   debug_printf("%s\n", __FUNCTION__);

   if (miny == maxy || minx == maxx) {
      debug_printf("%s: non-intersecting triangle in bin\n", __FUNCTION__);
      return;
   }

   minx &= ~(BLOCKSIZE-1);
   miny &= ~(BLOCKSIZE-1);

   x0 = minx << FIXED_ORDER;
   y0 = miny << FIXED_ORDER;

   c1 = tri->c1 + tri->dx12 * y0 - tri->dy12 * x0;
   c2 = tri->c2 + tri->dx23 * y0 - tri->dy23 * x0;
   c3 = tri->c3 + tri->dx31 * y0 - tri->dy31 * x0;

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

