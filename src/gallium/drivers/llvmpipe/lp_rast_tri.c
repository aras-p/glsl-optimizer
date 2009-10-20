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

static struct {
   int x;
   int y;
   unsigned mask;
} blocks[256];
static int nr_blocks;

/* Render a 4x4 unmasked block:
 */
static void block_full_4( struct lp_rasterizer *rast,
                        int x, int y )
{
   blocks[nr_blocks].x = x;
   blocks[nr_blocks].y = y;
   blocks[nr_blocks].mask = ~0;
   nr_blocks++;
}


static void block_full_16( struct lp_rasterizer *rast,
                        int x, int y )
{
   unsigned ix, iy;

   for (iy = 0; iy < 16; iy+=4) 
      for (ix = 0; ix < 16; ix+=4) 
	 block_full_4(rast, x + ix, y + iy);
}



/* Evaluate each pixel in a block, generate a mask and possibly render
 * the quad:
 */
static void
do_block_4( struct lp_rasterizer *rast,
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
   if (mask) {
      blocks[nr_blocks].x = x;
      blocks[nr_blocks].y = y;
      blocks[nr_blocks].mask = mask;
      nr_blocks++;
   }
}

static void
do_block_16( struct lp_rasterizer *rast,
	    const struct lp_rast_triangle *tri,
	    int x, int y,
	    int c1,
	    int c2,
	    int c3 )
{
   int ix,iy,i = 0;

   int ei1 = tri->ei1 << 2;
   int ei2 = tri->ei2 << 2;
   int ei3 = tri->ei3 << 2;

   int eo1 = tri->eo1 << 2;
   int eo2 = tri->eo2 << 2;
   int eo3 = tri->eo3 << 2;

   for (iy = 0; iy < 16; iy+=4) 
   {
      for (ix = 0; ix < 16; ix+=4, i++) 
      {
	 int cx1 = c1 + (tri->step[0][i] << 2);
	 int cx2 = c2 + (tri->step[1][i] << 2);
	 int cx3 = c3 + (tri->step[2][i] << 2);
	 
	 if (cx1 + eo1 < 0 ||
	     cx2 + eo2 < 0 ||
	     cx3 + eo3 < 0)
	 {
	 }
	 else if (cx1 + ei1 > 0 &&
		  cx2 + ei2 > 0 &&
		  cx3 + ei3 > 0)
	 {
	    block_full_4(rast, x+ix, y+iy); /* trivial accept */
	 }
	 else
	 {
	    do_block_4(rast, tri, x+ix, y+iy, cx1, cx2, cx3);
	 }
      }
   }
}

/* Scan the tile in chunks and figure out which pixels to rasterize
 * for this triangle:
 */
void lp_rast_triangle( struct lp_rasterizer *rast,
                       const union lp_rast_cmd_arg arg )
{
   const struct lp_rast_triangle *tri = arg.triangle;

   int x = rast->x;
   int y = rast->y;
   int ix,iy,i = 0;

   int c1 = tri->c1 + tri->dx12 * y - tri->dy12 * x;
   int c2 = tri->c2 + tri->dx23 * y - tri->dy23 * x;
   int c3 = tri->c3 + tri->dx31 * y - tri->dy31 * x;

   int ei1 = tri->ei1 << 4;
   int ei2 = tri->ei2 << 4;
   int ei3 = tri->ei3 << 4;

   int eo1 = tri->eo1 << 4;
   int eo2 = tri->eo2 << 4;
   int eo3 = tri->eo3 << 4;

   debug_printf("%s\n", __FUNCTION__);

   nr_blocks = 0;

   for (iy = 0; iy < 64; iy+=16) 
   {
      for (ix = 0; ix < 64; ix+=16, i++) 
      {
	 int cx1 = c1 + (tri->step[0][i] << 4);
	 int cx2 = c2 + (tri->step[1][i] << 4);
	 int cx3 = c3 + (tri->step[2][i] << 4);
	 
	 if (cx1 + eo1 < 0 ||
	     cx2 + eo2 < 0 ||
	     cx3 + eo3 < 0)
	 {
	 }
	 else if (cx1 + ei1 > 0 &&
		  cx2 + ei2 > 0 &&
		  cx3 + ei3 > 0)
	 {
	    block_full_16(rast, x+ix, y+iy); /* trivial accept */
	 }
	 else
	 {
	    do_block_16(rast, tri, x+ix, y+iy, cx1, cx2, cx3);
	 }
      }
   }

   for (i = 0; i < nr_blocks; i++) 
      lp_rast_shade_quads(rast, &tri->inputs, 
			  blocks[i].x,
			  blocks[i].y,
			  blocks[i].mask);
}

