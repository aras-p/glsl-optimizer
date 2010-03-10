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

#include "util/u_math.h"
#include "util/u_memory.h"
#include "lp_perf.h"
#include "lp_setup_context.h"
#include "lp_rast.h"

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
   tri->inputs.dadx[slot][i] = 0.0f;
   tri->inputs.dady[slot][i] = 0.0f;
}


/**
 * Compute a0, dadx and dady for a linearly interpolated coefficient,
 * for a triangle.
 */
static void linear_coef( struct lp_rast_triangle *tri,
                         float oneoverarea,
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
   float dadx = (da12 * tri->dy31 - tri->dy12 * da31) * oneoverarea;
   float dady = (da31 * tri->dx12 - tri->dx31 * da12) * oneoverarea;

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
   tri->inputs.a0[slot][i] = (a1 -
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
                              float oneoverarea,
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
   float dadx = (da12 * tri->dy31 - tri->dy12 * da31) * oneoverarea;
   float dady = (da31 * tri->dx12 - tri->dx31 * da12) * oneoverarea;

   tri->inputs.dadx[slot][i] = dadx;
   tri->inputs.dady[slot][i] = dady;
   tri->inputs.a0[slot][i] = (a1 -
                              (dadx * (v1[0][0] - 0.5f) +
                               dady * (v1[0][1] - 0.5f)));
}


/**
 * Special coefficient setup for gl_FragCoord.
 * X and Y are trivial
 * Z and W are copied from position_coef which should have already been computed.
 * We could do a bit less work if we'd examine gl_FragCoord's swizzle mask.
 */
static void
setup_fragcoord_coef(struct lp_rast_triangle *tri,
                     float oneoverarea,
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
   linear_coef(tri, oneoverarea, slot, v1, v2, v3, 0, 2);
   /*W*/
   linear_coef(tri, oneoverarea, slot, v1, v2, v3, 0, 3);
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
                                    float oneoverarea,
				    const float (*v1)[4],
				    const float (*v2)[4],
				    const float (*v3)[4],
				    boolean frontface)
{
   unsigned slot;

   /* The internal position input is in slot zero:
    */
   setup_fragcoord_coef(tri, oneoverarea, 0, v1, v2, v3);

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
            linear_coef(tri, oneoverarea, slot+1, v1, v2, v3, vert_attr, i);
         break;

      case LP_INTERP_PERSPECTIVE:
         for (i = 0; i < NUM_CHANNELS; i++)
            perspective_coef(tri, oneoverarea, slot+1, v1, v2, v3, vert_attr, i);
         break;

      case LP_INTERP_POSITION:
         /* XXX: fix me - duplicates the values in slot zero.
          */
         setup_fragcoord_coef(tri, oneoverarea, slot+1, v1, v2, v3);
         break;

      case LP_INTERP_FACING:
         setup_facing_coef(tri, slot+1, frontface);
         break;

      default:
         assert(0);
      }
   }
}



static INLINE int subpixel_snap( float a )
{
   return util_iround(FIXED_ONE * a - (FIXED_ONE / 2));
}



/**
 * Alloc space for a new triangle plus the input.a0/dadx/dady arrays
 * immediately after it.
 * The memory is allocated from the per-scene pool, not per-tile.
 * \param tri_size  returns number of bytes allocated
 * \param nr_inputs  number of fragment shader inputs
 * \return pointer to triangle space
 */
static INLINE struct lp_rast_triangle *
alloc_triangle(struct lp_scene *scene, unsigned nr_inputs, unsigned *tri_size)
{
   unsigned input_array_sz = NUM_CHANNELS * (nr_inputs + 1) * sizeof(float);
   struct lp_rast_triangle *tri;
   unsigned bytes;
   char *inputs;

   assert(sizeof(*tri) % 16 == 0);

   bytes = sizeof(*tri) + (3 * input_array_sz);

   tri = lp_scene_alloc_aligned( scene, bytes, 16 );

   inputs = (char *) (tri + 1);
   tri->inputs.a0   = (float (*)[4]) inputs;
   tri->inputs.dadx = (float (*)[4]) (inputs + input_array_sz);
   tri->inputs.dady = (float (*)[4]) (inputs + 2 * input_array_sz);

   *tri_size = bytes;

   return tri;
}



/**
 * Do basic setup for triangle rasterization and determine which
 * framebuffer tiles are touched.  Put the triangle in the scene's
 * bins for the tiles which we overlap.
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

   struct lp_scene *scene = lp_setup_get_current_scene(setup);
   struct lp_rast_triangle *tri;
   int area;
   float oneoverarea;
   int minx, maxx, miny, maxy;
   unsigned tri_bytes;

   tri = alloc_triangle(scene, setup->fs.nr_inputs, &tri_bytes);

#ifdef DEBUG
   tri->v[0][0] = v1[0][0];
   tri->v[1][0] = v2[0][0];
   tri->v[2][0] = v3[0][0];
   tri->v[0][1] = v1[0][1];
   tri->v[1][1] = v2[0][1];
   tri->v[2][1] = v3[0][1];
#endif

   tri->dx12 = x1 - x2;
   tri->dx23 = x2 - x3;
   tri->dx31 = x3 - x1;

   tri->dy12 = y1 - y2;
   tri->dy23 = y2 - y3;
   tri->dy31 = y3 - y1;

   area = (tri->dx12 * tri->dy31 - tri->dx31 * tri->dy12);

   LP_COUNT(nr_tris);

   /* Cull non-ccw and zero-sized triangles. 
    *
    * XXX: subject to overflow??
    */
   if (area <= 0) {
      lp_scene_putback_data( scene, tri_bytes );
      LP_COUNT(nr_culled_tris);
      return;
   }

   /* Bounding rectangle (in pixels) */
   minx = (MIN3(x1, x2, x3) + (FIXED_ONE-1)) >> FIXED_ORDER;
   maxx = (MAX3(x1, x2, x3) + (FIXED_ONE-1)) >> FIXED_ORDER;
   miny = (MIN3(y1, y2, y3) + (FIXED_ONE-1)) >> FIXED_ORDER;
   maxy = (MAX3(y1, y2, y3) + (FIXED_ONE-1)) >> FIXED_ORDER;
   
   if (setup->scissor_test) {
      minx = MAX2(minx, setup->scissor.current.minx);
      maxx = MIN2(maxx, setup->scissor.current.maxx);
      miny = MAX2(miny, setup->scissor.current.miny);
      maxy = MIN2(maxy, setup->scissor.current.maxy);
   }

   if (miny == maxy || 
       minx == maxx) {
      lp_scene_putback_data( scene, tri_bytes );
      LP_COUNT(nr_culled_tris);
      return;
   }

   /* 
    */
   oneoverarea = ((float)FIXED_ONE) / (float)area;

   /* Setup parameter interpolants:
    */
   setup_tri_coefficients( setup, tri, oneoverarea, v1, v2, v3, frontfacing );

   /* half-edge constants, will be interated over the whole render target.
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

   /* Fill in the inputs.step[][] arrays.
    * We've manually unrolled some loops here.
    */
   {
      const int xstep1 = -tri->dy12;
      const int xstep2 = -tri->dy23;
      const int xstep3 = -tri->dy31;
      const int ystep1 = tri->dx12;
      const int ystep2 = tri->dx23;
      const int ystep3 = tri->dx31;

#define SETUP_STEP(i, x, y)                                \
      do {                                                 \
         tri->inputs.step[0][i] = x * xstep1 + y * ystep1; \
         tri->inputs.step[1][i] = x * xstep2 + y * ystep2; \
         tri->inputs.step[2][i] = x * xstep3 + y * ystep3; \
      } while (0)

      SETUP_STEP(0, 0, 0);
      SETUP_STEP(1, 1, 0);
      SETUP_STEP(2, 0, 1);
      SETUP_STEP(3, 1, 1);

      SETUP_STEP(4, 2, 0);
      SETUP_STEP(5, 3, 0);
      SETUP_STEP(6, 2, 1);
      SETUP_STEP(7, 3, 1);

      SETUP_STEP(8, 0, 2);
      SETUP_STEP(9, 1, 2);
      SETUP_STEP(10, 0, 3);
      SETUP_STEP(11, 1, 3);

      SETUP_STEP(12, 2, 2);
      SETUP_STEP(13, 3, 2);
      SETUP_STEP(14, 2, 3);
      SETUP_STEP(15, 3, 3);
#undef STEP
   }

   /*
    * All fields of 'tri' are now set.  The remaining code here is
    * concerned with binning.
    */

   /* Convert to tile coordinates:
    */
   minx = minx / TILE_SIZE;
   miny = miny / TILE_SIZE;
   maxx = maxx / TILE_SIZE;
   maxy = maxy / TILE_SIZE;

   /*
    * Clamp to framebuffer size
    */
   minx = MAX2(minx, 0);
   miny = MAX2(miny, 0);
   maxx = MIN2(maxx, scene->tiles_x - 1);
   maxy = MIN2(maxy, scene->tiles_y - 1);

   /* Determine which tile(s) intersect the triangle's bounding box
    */
   if (miny == maxy && minx == maxx)
   {
      /* Triangle is contained in a single tile:
       */
      lp_scene_bin_command( scene, minx, miny, lp_rast_triangle, 
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


      /* Test tile-sized blocks against the triangle.
       * Discard blocks fully outside the tri.  If the block is fully
       * contained inside the tri, bin an lp_rast_shade_tile command.
       * Else, bin a lp_rast_triangle command.
       */
      for (y = miny; y <= maxy; y++)
      {
	 int cx1 = c1;
	 int cx2 = c2;
	 int cx3 = c3;
	 boolean in = FALSE;  /* are we inside the triangle? */

	 for (x = minx; x <= maxx; x++)
	 {
	    if (cx1 + eo1 < 0 || 
		cx2 + eo2 < 0 ||
		cx3 + eo3 < 0) 
	    {
	       /* do nothing */
               LP_COUNT(nr_empty_64);
	       if (in)
		  break;  /* exiting triangle, all done with this row */
	    }
	    else if (cx1 + ei1 > 0 &&
		     cx2 + ei2 > 0 &&
		     cx3 + ei3 > 0) 
	    {
               /* triangle covers the whole tile- shade whole tile */
               LP_COUNT(nr_fully_covered_64);
	       in = TRUE;
	       if(setup->fs.current.opaque) {
	          lp_scene_bin_reset( scene, x, y );
	          lp_scene_bin_command( scene, x, y,
	                                lp_rast_set_state,
	                                lp_rast_arg_state(setup->fs.stored) );
	       }
               lp_scene_bin_command( scene, x, y,
				     lp_rast_shade_tile,
				     lp_rast_arg_inputs(&tri->inputs) );
	    }
	    else 
	    { 
               /* rasterizer/shade partial tile */
               LP_COUNT(nr_partially_covered_64);
	       in = TRUE;
               lp_scene_bin_command( scene, x, y,
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
   if (ex * fy - ey * fx < 0.0f) 
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
