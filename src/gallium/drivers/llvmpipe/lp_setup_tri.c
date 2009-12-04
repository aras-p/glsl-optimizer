/**************************************************************************
 *
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/*
 * Binning code for triangles
 */

#include "lp_setup_context.h"
#include "lp_rast.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#define NUM_CHANNELS 4

/**
 * Compute a0 for a constant-valued coefficient (GL_FLAT shading).
 */
static void constant_coef( struct lp_rast_triangle *tri,
                           unsigned slot,
			   const float value,
                           unsigned i )
{
   tri->inputs.a0[slot][i] = value;
   tri->inputs.dadx[slot][i] = 0;
   tri->inputs.dady[slot][i] = 0;
}

/**
 * Compute a0, dadx and dady for a linearly interpolated coefficient,
 * for a triangle.
 */
static void linear_coef( struct lp_rast_triangle *tri,
                         unsigned slot,
                         const float (*v1)[4],
                         const float (*v2)[4],
                         const float (*v3)[4],
                         unsigned vert_attr,
                         unsigned i)
{
   float a1 = v1[vert_attr][i];
   float a2 = v2[vert_attr][i];
   float a3 = v3[vert_attr][i];

   float da12 = a1 - a2;
   float da31 = a3 - a1;
   float dadx = (da12 * tri->dy31 - tri->dy12 * da31) * tri->oneoverarea;
   float dady = (da31 * tri->dx12 - tri->dx31 * da12) * tri->oneoverarea;

   tri->inputs.dadx[slot][i] = dadx;
   tri->inputs.dady[slot][i] = dady;

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
   tri->inputs.a0[slot][i] = (v1[vert_attr][i] -
                              (dadx * (v1[0][0] - 0.5f) +
                               dady * (v1[0][1] - 0.5f)));
}


/**
 * Compute a0, dadx and dady for a perspective-corrected interpolant,
 * for a triangle.
 * We basically multiply the vertex value by 1/w before computing
 * the plane coefficients (a0, dadx, dady).
 * Later, when we compute the value at a particular fragment position we'll
 * divide the interpolated value by the interpolated W at that fragment.
 */
static void perspective_coef( struct lp_rast_triangle *tri,
                              unsigned slot,
			      const float (*v1)[4],
			      const float (*v2)[4],
			      const float (*v3)[4],
			      unsigned vert_attr,
                              unsigned i)
{
   /* premultiply by 1/w  (v[0][3] is always 1/w):
    */
   float a1 = v1[vert_attr][i] * v1[0][3];
   float a2 = v2[vert_attr][i] * v2[0][3];
   float a3 = v3[vert_attr][i] * v3[0][3];
   float da12 = a1 - a2;
   float da31 = a3 - a1;
   float dadx = (da12 * tri->dy31 - tri->dy12 * da31) * tri->oneoverarea;
   float dady = (da31 * tri->dx12 - tri->dx31 * da12) * tri->oneoverarea;

   tri->inputs.dadx[slot][i] = dadx;
   tri->inputs.dady[slot][i] = dady;
   tri->inputs.a0[slot][i] = (a1 -
                              (dadx * (v1[0][0] - 0.5f) +
                               dady * (v1[0][1] - 0.5f)));
}


/**
 * Special coefficient setup for gl_FragCoord.
 * X and Y are trivial, though Y has to be inverted for OpenGL.
 * Z and W are copied from position_coef which should have already been computed.
 * We could do a bit less work if we'd examine gl_FragCoord's swizzle mask.
 */
static void
setup_fragcoord_coef(struct lp_rast_triangle *tri,
                     unsigned slot,
                     const float (*v1)[4],
                     const float (*v2)[4],
                     const float (*v3)[4])
{
   /*X*/
   tri->inputs.a0[slot][0] = 0.0;
   tri->inputs.dadx[slot][0] = 1.0;
   tri->inputs.dady[slot][0] = 0.0;
   /*Y*/
   tri->inputs.a0[slot][1] = 0.0;
   tri->inputs.dadx[slot][1] = 0.0;
   tri->inputs.dady[slot][1] = 1.0;
   /*Z*/
   linear_coef(tri, slot, v1, v2, v3, 0, 2);
   /*W*/
   linear_coef(tri, slot, v1, v2, v3, 0, 3);
}


static void setup_facing_coef( struct lp_rast_triangle *tri,
                               unsigned slot,
                               boolean frontface )
{
   constant_coef( tri, slot, 1.0f - frontface, 0 );
   constant_coef( tri, slot, 0.0f, 1 ); /* wasted */
   constant_coef( tri, slot, 0.0f, 2 ); /* wasted */
   constant_coef( tri, slot, 0.0f, 3 ); /* wasted */
}


/**
 * Compute the tri->coef[] array dadx, dady, a0 values.
 */
static void setup_tri_coefficients( struct setup_context *setup,
				    struct lp_rast_triangle *tri,
				    const float (*v1)[4],
				    const float (*v2)[4],
				    const float (*v3)[4],
				    boolean frontface )
{
   unsigned slot;

   /* Allocate space for the a0, dadx and dady arrays
    */
   {
      unsigned bytes;
      bytes = (setup->fs.nr_inputs + 1) * 4 * sizeof(float);
      tri->inputs.a0   = get_data_aligned( &setup->data, bytes, 16 );
      tri->inputs.dadx = get_data_aligned( &setup->data, bytes, 16 );
      tri->inputs.dady = get_data_aligned( &setup->data, bytes, 16 );
   }

   /* The internal position input is in slot zero:
    */
   setup_fragcoord_coef(tri, 0, v1, v2, v3);

   /* setup interpolation for all the remaining attributes:
    */
   for (slot = 0; slot < setup->fs.nr_inputs; slot++) {
      unsigned vert_attr = setup->fs.input[slot].src_index;
      unsigned i;

      switch (setup->fs.input[slot].interp) {
      case LP_INTERP_CONSTANT:
         for (i = 0; i < NUM_CHANNELS; i++)
            constant_coef(tri, slot+1, v3[vert_attr][i], i);
         break;

      case LP_INTERP_LINEAR:
         for (i = 0; i < NUM_CHANNELS; i++)
            linear_coef(tri, slot+1, v1, v2, v3, vert_attr, i);
         break;

      case LP_INTERP_PERSPECTIVE:
         for (i = 0; i < NUM_CHANNELS; i++)
            perspective_coef(tri, slot+1, v1, v2, v3, vert_attr, i);
         break;

      case LP_INTERP_POSITION:
         /* XXX: fix me - duplicates the values in slot zero.
          */
         setup_fragcoord_coef(tri, slot+1, v1, v2, v3);
         break;

      case LP_INTERP_FACING:
         setup_facing_coef(tri, slot+1, frontface);
         break;

      default:
         assert(0);
      }
   }
}



static inline int subpixel_snap( float a )
{
   return util_iround(FIXED_ONE * a);
}


#define MIN3(a,b,c) MIN2(MIN2(a,b),c)
#define MAX3(a,b,c) MAX2(MAX2(a,b),c)

/**
 * Do basic setup for triangle rasterization and determine which
 * framebuffer tiles are touched.  Put the triangle in the bins for the
 * tiles which we overlap.
 */
static void 
do_triangle_ccw(struct setup_context *setup,
		const float (*v1)[4],
		const float (*v2)[4],
		const float (*v3)[4],
		boolean frontfacing )
{
   /* x/y positions in fixed point */
   const int x1 = subpixel_snap(v1[0][0]);
   const int x2 = subpixel_snap(v2[0][0]);
   const int x3 = subpixel_snap(v3[0][0]);
   const int y1 = subpixel_snap(v1[0][1]);
   const int y2 = subpixel_snap(v2[0][1]);
   const int y3 = subpixel_snap(v3[0][1]);

   struct lp_rast_triangle *tri = get_data( &setup->data, sizeof *tri );
   float area;
   int minx, maxx, miny, maxy;

   tri->dx12 = x1 - x2;
   tri->dx23 = x2 - x3;
   tri->dx31 = x3 - x1;

   tri->dy12 = y1 - y2;
   tri->dy23 = y2 - y3;
   tri->dy31 = y3 - y1;

   area = (tri->dx12 * tri->dy31 - 
	   tri->dx31 * tri->dy12);

   /* Cull non-ccw and zero-sized triangles. 
    *
    * XXX: subject to overflow??
    */
   if (area <= 0) {
      putback_data( &setup->data, sizeof *tri );
      return;
   }

   /* Bounding rectangle (in pixels) */
   tri->minx = (MIN3(x1, x2, x3) + 0xf) >> FIXED_ORDER;
   tri->maxx = (MAX3(x1, x2, x3) + 0xf) >> FIXED_ORDER;
   tri->miny = (MIN3(y1, y2, y3) + 0xf) >> FIXED_ORDER;
   tri->maxy = (MAX3(y1, y2, y3) + 0xf) >> FIXED_ORDER;
   
   if (tri->miny == tri->maxy || 
       tri->minx == tri->maxx) {
      putback_data( &setup->data, sizeof *tri );
      return;
   }

   tri->inputs.state = setup->fs.stored;

   /* 
    */
   tri->oneoverarea = ((float)FIXED_ONE) / (float)area;

   /* Setup parameter interpolants:
    */
   setup_tri_coefficients( setup, tri, v1, v2, v3, frontfacing );

   /* half-edge constants, will be interated over the whole
    * rendertarget.
    */
   tri->c1 = tri->dy12 * x1 - tri->dx12 * y1;
   tri->c2 = tri->dy23 * x2 - tri->dx23 * y2;
   tri->c3 = tri->dy31 * x3 - tri->dx31 * y3;

   /* correct for top-left fill convention:
    */
   if (tri->dy12 < 0 || (tri->dy12 == 0 && tri->dx12 > 0)) tri->c1++;
   if (tri->dy23 < 0 || (tri->dy23 == 0 && tri->dx23 > 0)) tri->c2++;
   if (tri->dy31 < 0 || (tri->dy31 == 0 && tri->dx31 > 0)) tri->c3++;

   tri->dy12 *= FIXED_ONE;
   tri->dy23 *= FIXED_ONE;
   tri->dy31 *= FIXED_ONE;

   tri->dx12 *= FIXED_ONE;
   tri->dx23 *= FIXED_ONE;
   tri->dx31 *= FIXED_ONE;

   /* find trivial reject offsets for each edge for a single-pixel
    * sized block.  These will be scaled up at each recursive level to
    * match the active blocksize.  Scaling in this way works best if
    * the blocks are square.
    */
   tri->eo1 = 0;
   if (tri->dy12 < 0) tri->eo1 -= tri->dy12;
   if (tri->dx12 > 0) tri->eo1 += tri->dx12;

   tri->eo2 = 0;
   if (tri->dy23 < 0) tri->eo2 -= tri->dy23;
   if (tri->dx23 > 0) tri->eo2 += tri->dx23;

   tri->eo3 = 0;
   if (tri->dy31 < 0) tri->eo3 -= tri->dy31;
   if (tri->dx31 > 0) tri->eo3 += tri->dx31;

   /* Calculate trivial accept offsets from the above.
    */
   tri->ei1 = tri->dx12 - tri->dy12 - tri->eo1;
   tri->ei2 = tri->dx23 - tri->dy23 - tri->eo2;
   tri->ei3 = tri->dx31 - tri->dy31 - tri->eo3;

   {
      int xstep1 = -tri->dy12;
      int xstep2 = -tri->dy23;
      int xstep3 = -tri->dy31;

      int ystep1 = tri->dx12;
      int ystep2 = tri->dx23;
      int ystep3 = tri->dx31;
      
      int ix, iy;
      int i = 0;

      int c1 = 0;
      int c2 = 0;
      int c3 = 0;
      
      for (iy = 0; iy < 4; iy++) {
	 int cx1 = c1;
	 int cx2 = c2;
	 int cx3 = c3;

	 for (ix = 0; ix < 4; ix++, i++) {
	    tri->step[0][i] = cx1;
	    tri->step[1][i] = cx2;
	    tri->step[2][i] = cx3;
	    cx1 += xstep1;
	    cx2 += xstep2;
	    cx3 += xstep3;
	 }

	 c1 += ystep1;
	 c2 += ystep2;
	 c3 += ystep3;
      }
   }

   /*
    * All fields of 'tri' are now set.  The remaining code here is
    * concerned with binning.
    */

   /* Convert to tile coordinates:
    */
   minx = tri->minx / TILE_SIZE;
   miny = tri->miny / TILE_SIZE;
   maxx = tri->maxx / TILE_SIZE;
   maxy = tri->maxy / TILE_SIZE;

   /* Determine which tile(s) intersect the triangle's bounding box
    */
   if (miny == maxy && minx == maxx)
   {
      /* Triangle is contained in a single tile:
       */
      bin_command( &setup->tile[minx][miny], lp_rast_triangle, 
                   lp_rast_arg_triangle(tri) );
   }
   else 
   {
      int c1 = (tri->c1 + 
                tri->dx12 * miny * TILE_SIZE - 
                tri->dy12 * minx * TILE_SIZE);
      int c2 = (tri->c2 + 
                tri->dx23 * miny * TILE_SIZE -
                tri->dy23 * minx * TILE_SIZE);
      int c3 = (tri->c3 +
                tri->dx31 * miny * TILE_SIZE -
                tri->dy31 * minx * TILE_SIZE);

      int ei1 = tri->ei1 << TILE_ORDER;
      int ei2 = tri->ei2 << TILE_ORDER;
      int ei3 = tri->ei3 << TILE_ORDER;

      int eo1 = tri->eo1 << TILE_ORDER;
      int eo2 = tri->eo2 << TILE_ORDER;
      int eo3 = tri->eo3 << TILE_ORDER;

      int xstep1 = -(tri->dy12 << TILE_ORDER);
      int xstep2 = -(tri->dy23 << TILE_ORDER);
      int xstep3 = -(tri->dy31 << TILE_ORDER);

      int ystep1 = tri->dx12 << TILE_ORDER;
      int ystep2 = tri->dx23 << TILE_ORDER;
      int ystep3 = tri->dx31 << TILE_ORDER;
      int x, y;


      /* Trivially accept or reject blocks, else jump to per-pixel
       * examination above.
       */
      for (y = miny; y <= maxy; y++)
      {
	 int cx1 = c1;
	 int cx2 = c2;
	 int cx3 = c3;
	 int in = 0;

	 for (x = minx; x <= maxx; x++)
	 {
	    if (cx1 + eo1 < 0 || 
		cx2 + eo2 < 0 ||
		cx3 + eo3 < 0) 
	    {
	       /* do nothing */
	       if (in)
		  break;
	    }
	    else if (cx1 + ei1 > 0 &&
		     cx2 + ei2 > 0 &&
		     cx3 + ei3 > 0) 
	    {
	       in = 1;
               /* triangle covers the whole tile- shade whole tile */
               bin_command( &setup->tile[x][y],
                            lp_rast_shade_tile,
                            lp_rast_arg_inputs(&tri->inputs) );
	    }
	    else 
	    { 
	       in = 1;
               /* shade partial tile */
               bin_command( &setup->tile[x][y], 
                            lp_rast_triangle, 
                            lp_rast_arg_triangle(tri) );
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

static void triangle_cw( struct setup_context *setup,
			 const float (*v0)[4],
			 const float (*v1)[4],
			 const float (*v2)[4] )
{
   do_triangle_ccw( setup, v1, v0, v2, !setup->ccw_is_frontface );
}

static void triangle_ccw( struct setup_context *setup,
			 const float (*v0)[4],
			 const float (*v1)[4],
			 const float (*v2)[4] )
{
   do_triangle_ccw( setup, v0, v1, v2, setup->ccw_is_frontface );
}

static void triangle_both( struct setup_context *setup,
			   const float (*v0)[4],
			   const float (*v1)[4],
			   const float (*v2)[4] )
{
   /* edge vectors e = v0 - v2, f = v1 - v2 */
   const float ex = v0[0][0] - v2[0][0];
   const float ey = v0[0][1] - v2[0][1];
   const float fx = v1[0][0] - v2[0][0];
   const float fy = v1[0][1] - v2[0][1];

   /* det = cross(e,f).z */
   if (ex * fy - ey * fx < 0) 
      triangle_ccw( setup, v0, v1, v2 );
   else
      triangle_cw( setup, v0, v1, v2 );
}

static void triangle_nop( struct setup_context *setup,
			  const float (*v0)[4],
			  const float (*v1)[4],
			  const float (*v2)[4] )
{
}


void 
lp_setup_choose_triangle( struct setup_context *setup )
{
   switch (setup->cullmode) {
   case PIPE_WINDING_NONE:
      setup->triangle = triangle_both;
      break;
   case PIPE_WINDING_CCW:
      setup->triangle = triangle_cw;
      break;
   case PIPE_WINDING_CW:
      setup->triangle = triangle_ccw;
      break;
   default:
      setup->triangle = triangle_nop;
      break;
   }
}


