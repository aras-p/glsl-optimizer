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

   /* The internal position input is in slot zero:
    */
   setup_fragcoord_coef(tri, 0, v1, v2, v3);

   /* setup interpolation for all the remaining attrbutes:
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



/* XXX: do this by add/subtracting a large floating point number:
 */
static inline float subpixel_snap( float a )
{
   int i = a * 16;
   return (float)i * (1.0/16);
}


static INLINE void bin_triangle( struct cmd_block_list *list,
                                 const struct lp_rast_triangle arg )
{
}


/* to avoid having to allocate power-of-four, square render targets,
 * end up having a specialized version of the above that runs only at
 * the topmost level.
 *
 * at the topmost level there may be an arbitary number of steps on
 * either dimension, so this loop needs to be either separately
 * code-generated and unrolled for each render target size, or kept as
 * generic looping code:
 */

#define MIN3(a,b,c) MIN2(MIN2(a,b),c)
#define MAX3(a,b,c) MAX2(MAX2(a,b),c)

static void 
do_triangle_ccw(struct setup_context *setup,
		const float (*v1)[4],
		const float (*v2)[4],
		const float (*v3)[4],
		boolean frontfacing )
{
   const int rt_width = setup->fb.width;
   const int rt_height = setup->fb.height;

   const float y1 = subpixel_snap(v1[0][1]);
   const float y2 = subpixel_snap(v2[0][1]);
   const float y3 = subpixel_snap(v3[0][1]);

   const float x1 = subpixel_snap(v1[0][0]);
   const float x2 = subpixel_snap(v2[0][0]);
   const float x3 = subpixel_snap(v3[0][0]);
   
   struct lp_rast_triangle *tri = get_data( &setup->data, sizeof *tri );
   float area;
   int minx, maxx, miny, maxy;
   float c1, c2, c3;

   tri->inputs.state = setup->fs.stored;

   tri->dx12 = x1 - x2;
   tri->dx23 = x2 - x3;
   tri->dx31 = x3 - x1;

   tri->dy12 = y1 - y2;
   tri->dy23 = y2 - y3;
   tri->dy31 = y3 - y1;

   area = (tri->dx12 * tri->dy31 - 
	   tri->dx31 * tri->dy12);

   /* Cull non-ccw and zero-sized triangles.
    */
   if (area <= 0 || util_is_inf_or_nan(area))
      return;

   // Bounding rectangle
   minx = util_iround(MIN3(x1, x2, x3) - .5);
   maxx = util_iround(MAX3(x1, x2, x3) + .5);
   miny = util_iround(MIN3(y1, y2, y3) - .5);
   maxy = util_iround(MAX3(y1, y2, y3) + .5);
   
   /* Clamp to framebuffer (or tile) dimensions:
    */
   miny = MAX2(0, miny);
   minx = MAX2(0, minx);
   maxy = MIN2(rt_height, maxy);
   maxx = MIN2(rt_width, maxx);

   if (miny == maxy || minx == maxx)
      return;

   tri->miny = miny;
   tri->minx = minx;
   tri->maxy = maxy;
   tri->maxx = maxx;

   /* The only divide in this code.  Is it really needed?
    */
   tri->oneoverarea = 1.0f / area;

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

   minx &= ~(TILESIZE-1);		/* aligned blocks */
   miny &= ~(TILESIZE-1);		/* aligned blocks */

   c1 = tri->c1 + tri->dx12 * miny - tri->dy12 * minx;
   c2 = tri->c2 + tri->dx23 * miny - tri->dy23 * minx;
   c3 = tri->c3 + tri->dx31 * miny - tri->dy31 * minx;

   minx /= TILESIZE;
   miny /= TILESIZE;
   maxx /= TILESIZE;
   maxy /= TILESIZE;

   /* Convert to tile coordinates:
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
      const int step = TILESIZE;

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


      /* Subdivide space into NxM blocks, where each block is square and
       * power-of-four in dimension.
       *
       * Trivially accept or reject blocks, else jump to per-pixel
       * examination above.
       */
      for (y = miny; y <= maxy; y++)
      {
	 float cx1 = c1;
	 float cx2 = c2;
	 float cx3 = c3;

	 for (x = minx; x <= maxx; x++)
	 {
	    if (cx1 + eo1 < 0 || 
		cx2 + eo2 < 0 ||
		cx3 + eo3 < 0) 
	    {
	       /* do nothing */
	    }
	    else if (cx1 + ei1 > 0 &&
		     cx2 + ei2 > 0 &&
		     cx3 + ei3 > 0) 
	    {
               /* shade whole tile */
               bin_command( &setup->tile[x][y], lp_rast_shade_tile,
                            lp_rast_arg_inputs(&tri->inputs) );
	    }
	    else 
	    {
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


