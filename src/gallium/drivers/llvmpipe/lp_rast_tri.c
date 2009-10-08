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

#include "lp_context.h"
#include "lp_quad.h"
#include "lp_quad_pipe.h"
#include "lp_setup.h"
#include "lp_state.h"
#include "draw/draw_context.h"
#include "draw/draw_private.h"
#include "draw/draw_vertex.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/p_thread.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#define BLOCKSIZE 4


/* Convert 8x8 block into four runs of quads and render each in turn.
 */
#if (BLOCKSIZE == 8)
static void block_full( struct triangle *tri, int x, int y )
{
   struct quad_header *ptrs[4];
   int i;

   tri->quad[0].input.x0 = x + 0;
   tri->quad[1].input.x0 = x + 2;
   tri->quad[2].input.x0 = x + 4;
   tri->quad[3].input.x0 = x + 6;

   for (i = 0; i < 4; i++, y += 2) {
      tri->quad[0].inout.mask = 0xf;
      tri->quad[1].inout.mask = 0xf;
      tri->quad[2].inout.mask = 0xf;
      tri->quad[3].inout.mask = 0xf;

      tri->quad[0].input.y0 = y;
      tri->quad[1].input.y0 = y;
      tri->quad[2].input.y0 = y;
      tri->quad[3].input.y0 = y;

      /* XXX: don't bother with this ptrs business */
      ptrs[0] = &tri->quad[0];
      ptrs[1] = &tri->quad[1];
      ptrs[2] = &tri->quad[2];
      ptrs[3] = &tri->quad[3];

      tri->llvmpipe->quad.first->run( tri->llvmpipe->quad.first, ptrs, 4 );
   }
}
#else
static void block_full( struct triangle *tri, int x, int y )
{
   struct quad_header *ptrs[4];
   int iy;

   tri->quad[0].input.x0 = x + 0;
   tri->quad[1].input.x0 = x + 2;

   for (iy = 0; iy < 4; iy += 2) {
      tri->quad[0].inout.mask = 0xf;
      tri->quad[1].inout.mask = 0xf;

      tri->quad[0].input.y0 = y + iy;
      tri->quad[1].input.y0 = y + iy;

      /* XXX: don't bother with this ptrs business */
      ptrs[0] = &tri->quad[0];
      ptrs[1] = &tri->quad[1];

      tri->llvmpipe->quad.first->run( tri->llvmpipe->quad.first, ptrs, 2 );
   }
}
#endif

static void
do_quad( struct lp_rasterizer *rast,
	 int x, int y,
	 float c1, float c2, float c3 )
{
   struct triangle *tri = rast->tri;
   struct quad_header *quad = &rast->quad[0];

   float xstep1 = -tri->dy12;
   float xstep2 = -tri->dy23;
   float xstep3 = -tri->dy31;

   float ystep1 = tri->dx12;
   float ystep2 = tri->dx23;
   float ystep3 = tri->dx31;

   quad->input.x0 = x;
   quad->input.y0 = y;
   quad->inout.mask = 0;

   if (c1 > 0 &&
       c2 > 0 &&
       c3 > 0)
      quad->inout.mask |= 1;
	 
   if (c1 + xstep1 > 0 && 
       c2 + xstep2 > 0 && 
       c3 + xstep3 > 0)
      quad->inout.mask |= 2;

   if (c1 + ystep1 > 0 && 
       c2 + ystep2 > 0 && 
       c3 + ystep3 > 0)
      quad->inout.mask |= 4;

   if (c1 + ystep1 + xstep1 > 0 && 
       c2 + ystep2 + xstep2 > 0 && 
       c3 + ystep3 + xstep3 > 0)
      quad->inout.mask |= 8;

   if (quad->inout.mask)
      rast->state->run( rast->state->state, &quad, 1 );
}

/* Evaluate each pixel in a block, generate a mask and possibly render
 * the quad:
 */
static void
do_block( struct triangle *tri,
	 int x, int y,
	 float c1,
	 float c2,
	 float c3 )
{
   const int step = 2;

   float xstep1 = -step * tri->dy12;
   float xstep2 = -step * tri->dy23;
   float xstep3 = -step * tri->dy31;

   float ystep1 = step * tri->dx12;
   float ystep2 = step * tri->dx23;
   float ystep3 = step * tri->dx31;

   int ix, iy;

   for (iy = 0; iy < BLOCKSIZE; iy += 2) {
      float cx1 = c1;
      float cx2 = c2;
      float cx3 = c3;

      for (ix = 0; ix < BLOCKSIZE; ix += 2) {

	 do_quad(tri, x+ix, y+iy, cx1, cx2, cx3);

	 cx1 += xstep1;
	 cx2 += xstep2;
	 cx3 += xstep3;
      }

      c1 += ystep1;
      c2 += ystep2;
      c3 += ystep3;
   }
}



/* Scan the tile in chunks and figure out which pixels to rasterize
 * for this triangle:
 */
void lp_rast_triangle( struct lp_rasterizer *rast,
		       const struct lp_rast_triangle *tri )
{
   int minx, maxx, miny, maxy;

   /* Clamp to tile dimensions:
    */
   minx = MAX2(tri->maxx, rast->x);
   miny = MAX2(tri->miny, rast->y);
   maxx = MIN2(tri->maxx, rast->x + TILE_SIZE);
   maxy = MIN2(tri->maxy, rast->y + TILE_SIZE);

   if (miny == maxy ||
       minx == maxx) {
      debug_printf("%s: non-intersecting triangle in bin\n", __FUNCTION__);
      //assert(0);
      return;
   }

   /* Bind parameter interpolants:
    */
   for (i = 0; i < Elements(rast->quad); i++) {
      rast->quad[i].coef = tri->coef;
      rast->quad[i].posCoef = &tri->position_coef;
   }

   /* Small area?
    */
   if (miny + 16 > maxy &&
       minx + 16 > maxx)
   {
      const int step = 2;

      float xstep1 = -step * tri->dy12;
      float xstep2 = -step * tri->dy23;
      float xstep3 = -step * tri->dy31;

      float ystep1 = step * tri->dx12;
      float ystep2 = step * tri->dx23;
      float ystep3 = step * tri->dx31;

      float eo1 = tri->eo1 * step;
      float eo2 = tri->eo2 * step;
      float eo3 = tri->eo3 * step;

      int x, y;

      minx &= ~(step-1);
      maxx &= ~(step-1);

      /* Subdivide space into NxM blocks, where each block is square and
       * power-of-four in dimension.
       *
       * Trivially accept or reject blocks, else jump to per-pixel
       * examination above.
       */
      for (y = miny; y < maxy; y += step)
      {
	 float cx1 = c1;
	 float cx2 = c2;
	 float cx3 = c3;

	 for (x = minx; x < maxx; x += step)
	 {
	    if (cx1 + eo1 < 0 || 
		cx2 + eo2 < 0 ||
		cx3 + eo3 < 0) 
	    {
	    }
	    else 
	    {
	       do_quad(&tri, x, y, cx1, cx2, cx3);
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
   else 
   {
      const int step = BLOCKSIZE;

      float ei1 = tri->ei1 * step;
      float ei2 = tri->ei2 * step;
      float ei3 = tri->ei3 * step;

      float eo1 = tri->eo1 * step;
      float eo2 = tri->eo2 * step;
      float eo3 = tri->eo3 * step;

      float xstep1 = -step * tri->dy12;
      float xstep2 = -step * tri->dy23;
      float xstep3 = -step * tri->dy31;

      float ystep1 = step * tri->dx12;
      float ystep2 = step * tri->dx23;
      float ystep3 = step * tri->dx31;
      int x, y;

      minx &= ~(step-1);
      miny &= ~(step-1);

      for (y = miny; y < maxy; y += step)
      {
	 float cx1 = c1;
	 float cx2 = c2;
	 float cx3 = c3;

	 for (x = minx; x < maxx; x += step)
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
	       block_full(&tri, x, y); /* trivial accept */
	    }
	    else 
	    {
	       do_block(&tri, x, y, cx1, cx2, cx3);
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
}

