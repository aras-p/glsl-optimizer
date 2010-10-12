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
#include "util/u_rect.h"
#include "lp_perf.h"
#include "lp_setup_context.h"
#include "lp_setup_coef.h"
#include "lp_rast.h"
#include "lp_state_fs.h"

#define NUM_CHANNELS 4


   
static INLINE int
subpixel_snap(float a)
{
   return util_iround(FIXED_ONE * a);
}

static INLINE float
fixed_to_float(int a)
{
   return a * (1.0 / FIXED_ONE);
}







/**
 * Alloc space for a new triangle plus the input.a0/dadx/dady arrays
 * immediately after it.
 * The memory is allocated from the per-scene pool, not per-tile.
 * \param tri_size  returns number of bytes allocated
 * \param nr_inputs  number of fragment shader inputs
 * \return pointer to triangle space
 */
struct lp_rast_triangle *
lp_setup_alloc_triangle(struct lp_scene *scene,
                        unsigned nr_inputs,
                        unsigned nr_planes,
                        unsigned *tri_size)
{
   unsigned input_array_sz = NUM_CHANNELS * (nr_inputs + 1) * sizeof(float);
   struct lp_rast_triangle *tri;
   unsigned tri_bytes, bytes;
   char *inputs;

   tri_bytes = align(Offset(struct lp_rast_triangle, plane[nr_planes]), 16);
   bytes = tri_bytes + (3 * input_array_sz);

   tri = lp_scene_alloc_aligned( scene, bytes, 16 );

   if (tri) {
      inputs = ((char *)tri) + tri_bytes;
      tri->inputs.a0   = (float (*)[4]) inputs;
      tri->inputs.dadx = (float (*)[4]) (inputs + input_array_sz);
      tri->inputs.dady = (float (*)[4]) (inputs + 2 * input_array_sz);

      *tri_size = bytes;
   }

   return tri;
}

void
lp_setup_print_vertex(struct lp_setup_context *setup,
                      const char *name,
                      const float (*v)[4])
{
   int i, j;

   debug_printf("   wpos (%s[0]) xyzw %f %f %f %f\n",
                name,
                v[0][0], v[0][1], v[0][2], v[0][3]);

   for (i = 0; i < setup->fs.nr_inputs; i++) {
      const float *in = v[setup->fs.input[i].src_index];

      debug_printf("  in[%d] (%s[%d]) %s%s%s%s ",
                   i, 
                   name, setup->fs.input[i].src_index,
                   (setup->fs.input[i].usage_mask & 0x1) ? "x" : " ",
                   (setup->fs.input[i].usage_mask & 0x2) ? "y" : " ",
                   (setup->fs.input[i].usage_mask & 0x4) ? "z" : " ",
                   (setup->fs.input[i].usage_mask & 0x8) ? "w" : " ");

      for (j = 0; j < 4; j++)
         if (setup->fs.input[i].usage_mask & (1<<j))
            debug_printf("%.5f ", in[j]);

      debug_printf("\n");
   }
}


/**
 * Print triangle vertex attribs (for debug).
 */
void
lp_setup_print_triangle(struct lp_setup_context *setup,
                        const float (*v0)[4],
                        const float (*v1)[4],
                        const float (*v2)[4])
{
   debug_printf("triangle\n");

   {
      const float ex = v0[0][0] - v2[0][0];
      const float ey = v0[0][1] - v2[0][1];
      const float fx = v1[0][0] - v2[0][0];
      const float fy = v1[0][1] - v2[0][1];

      /* det = cross(e,f).z */
      const float det = ex * fy - ey * fx;
      if (det < 0.0f) 
         debug_printf("   - ccw\n");
      else if (det > 0.0f)
         debug_printf("   - cw\n");
      else
         debug_printf("   - zero area\n");
   }

   lp_setup_print_vertex(setup, "v0", v0);
   lp_setup_print_vertex(setup, "v1", v1);
   lp_setup_print_vertex(setup, "v2", v2);
}


#define MAX_PLANES 8
static unsigned
lp_rast_tri_tab[MAX_PLANES+1] = {
   0,               /* should be impossible */
   LP_RAST_OP_TRIANGLE_1,
   LP_RAST_OP_TRIANGLE_2,
   LP_RAST_OP_TRIANGLE_3,
   LP_RAST_OP_TRIANGLE_4,
   LP_RAST_OP_TRIANGLE_5,
   LP_RAST_OP_TRIANGLE_6,
   LP_RAST_OP_TRIANGLE_7,
   LP_RAST_OP_TRIANGLE_8
};



/**
 * The primitive covers the whole tile- shade whole tile.
 *
 * \param tx, ty  the tile position in tiles, not pixels
 */
static boolean
lp_setup_whole_tile(struct lp_setup_context *setup,
                    const struct lp_rast_shader_inputs *inputs,
                    int tx, int ty)
{
   struct lp_scene *scene = setup->scene;

   LP_COUNT(nr_fully_covered_64);

   /* if variant is opaque and scissor doesn't effect the tile */
   if (inputs->opaque) {
      if (!scene->fb.zsbuf) {
         /*
          * All previous rendering will be overwritten so reset the bin.
          */
         lp_scene_bin_reset( scene, tx, ty );
      }

      LP_COUNT(nr_shade_opaque_64);
      return lp_scene_bin_command( scene, tx, ty,
                                   LP_RAST_OP_SHADE_TILE_OPAQUE,
                                   lp_rast_arg_inputs(inputs) );
   } else {
      LP_COUNT(nr_shade_64);
      return lp_scene_bin_command( scene, tx, ty,
                                   LP_RAST_OP_SHADE_TILE,
                                   lp_rast_arg_inputs(inputs) );
   }
}


/**
 * Do basic setup for triangle rasterization and determine which
 * framebuffer tiles are touched.  Put the triangle in the scene's
 * bins for the tiles which we overlap.
 */
static boolean
do_triangle_ccw(struct lp_setup_context *setup,
		const float (*v0)[4],
		const float (*v1)[4],
		const float (*v2)[4],
		boolean frontfacing )
{
   struct lp_scene *scene = setup->scene;
   struct lp_rast_triangle *tri;
   int x[3];
   int y[3];
   struct u_rect bbox;
   unsigned tri_bytes;
   int i;
   int nr_planes = 3;

   if (0)
      lp_setup_print_triangle(setup, v0, v1, v2);

   if (setup->scissor_test) {
      nr_planes = 7;
   }
   else {
      nr_planes = 3;
   }

   /* x/y positions in fixed point */
   x[0] = subpixel_snap(v0[0][0] - setup->pixel_offset);
   x[1] = subpixel_snap(v1[0][0] - setup->pixel_offset);
   x[2] = subpixel_snap(v2[0][0] - setup->pixel_offset);
   y[0] = subpixel_snap(v0[0][1] - setup->pixel_offset);
   y[1] = subpixel_snap(v1[0][1] - setup->pixel_offset);
   y[2] = subpixel_snap(v2[0][1] - setup->pixel_offset);


   /* Bounding rectangle (in pixels) */
   {
      /* Yes this is necessary to accurately calculate bounding boxes
       * with the two fill-conventions we support.  GL (normally) ends
       * up needing a bottom-left fill convention, which requires
       * slightly different rounding.
       */
      int adj = (setup->pixel_offset != 0) ? 1 : 0;

      bbox.x0 = (MIN3(x[0], x[1], x[2]) + (FIXED_ONE-1)) >> FIXED_ORDER;
      bbox.x1 = (MAX3(x[0], x[1], x[2]) + (FIXED_ONE-1)) >> FIXED_ORDER;
      bbox.y0 = (MIN3(y[0], y[1], y[2]) + (FIXED_ONE-1) + adj) >> FIXED_ORDER;
      bbox.y1 = (MAX3(y[0], y[1], y[2]) + (FIXED_ONE-1) + adj) >> FIXED_ORDER;

      /* Inclusive coordinates:
       */
      bbox.x1--;
      bbox.y1--;
   }

   if (bbox.x1 < bbox.x0 ||
       bbox.y1 < bbox.y0) {
      if (0) debug_printf("empty bounding box\n");
      LP_COUNT(nr_culled_tris);
      return TRUE;
   }

   if (!u_rect_test_intersection(&setup->draw_region, &bbox)) {
      if (0) debug_printf("offscreen\n");
      LP_COUNT(nr_culled_tris);
      return TRUE;
   }

   u_rect_find_intersection(&setup->draw_region, &bbox);

   tri = lp_setup_alloc_triangle(scene,
                                 setup->fs.nr_inputs,
                                 nr_planes,
                                 &tri_bytes);
   if (!tri)
      return FALSE;

#ifdef DEBUG
   tri->v[0][0] = v0[0][0];
   tri->v[1][0] = v1[0][0];
   tri->v[2][0] = v2[0][0];
   tri->v[0][1] = v0[0][1];
   tri->v[1][1] = v1[0][1];
   tri->v[2][1] = v2[0][1];
#endif

   tri->plane[0].dcdy = x[0] - x[1];
   tri->plane[1].dcdy = x[1] - x[2];
   tri->plane[2].dcdy = x[2] - x[0];

   tri->plane[0].dcdx = y[0] - y[1];
   tri->plane[1].dcdx = y[1] - y[2];
   tri->plane[2].dcdx = y[2] - y[0];

   LP_COUNT(nr_tris);

   /* Setup parameter interpolants:
    */
   lp_setup_tri_coef( setup, &tri->inputs, v0, v1, v2, frontfacing );

   tri->inputs.facing = frontfacing ? 1.0F : -1.0F;
   tri->inputs.disable = FALSE;
   tri->inputs.opaque = setup->fs.current.variant->opaque;
   tri->inputs.state = setup->fs.stored;

  
   for (i = 0; i < 3; i++) {
      struct lp_rast_plane *plane = &tri->plane[i];

      /* half-edge constants, will be interated over the whole render
       * target.
       */
      plane->c = plane->dcdx * x[i] - plane->dcdy * y[i];

      /* correct for top-left vs. bottom-left fill convention.  
       *
       * note that we're overloading gl_rasterization_rules to mean
       * both (0.5,0.5) pixel centers *and* bottom-left filling
       * convention.
       *
       * GL actually has a top-left filling convention, but GL's
       * notion of "top" differs from gallium's...
       *
       * Also, sometimes (in FBO cases) GL will render upside down
       * to its usual method, in which case it will probably want
       * to use the opposite, top-left convention.
       *
       * XXX: Chances are this will get stripped away.  In fact this
       * is only meaningful if:
       *
       *          (plane->c & (FIXED_ONE-1)) == 0
       *
       */         
      if ((plane->c & (FIXED_ONE-1)) == 0) {
         if (plane->dcdx < 0) {
            /* both fill conventions want this - adjust for left edges */
            plane->c++;            
         }
         else if (plane->dcdx == 0) {
            if (setup->pixel_offset == 0) {
               /* correct for top-left fill convention:
                */
               if (plane->dcdy > 0) plane->c++;
            }
            else {
               /* correct for bottom-left fill convention:
                */
               if (plane->dcdy < 0) plane->c++;
            }
         }
      }

      plane->c = (plane->c + (FIXED_ONE-1)) / FIXED_ONE;


      /* find trivial reject offsets for each edge for a single-pixel
       * sized block.  These will be scaled up at each recursive level to
       * match the active blocksize.  Scaling in this way works best if
       * the blocks are square.
       */
      plane->eo = 0;
      if (plane->dcdx < 0) plane->eo -= plane->dcdx;
      if (plane->dcdy > 0) plane->eo += plane->dcdy;

      /* Calculate trivial accept offsets from the above.
       */
      plane->ei = plane->dcdy - plane->dcdx - plane->eo;
   }


   /* 
    * When rasterizing scissored tris, use the intersection of the
    * triangle bounding box and the scissor rect to generate the
    * scissor planes.
    *
    * This permits us to cut off the triangle "tails" that are present
    * in the intermediate recursive levels caused when two of the
    * triangles edges don't diverge quickly enough to trivially reject
    * exterior blocks from the triangle.
    *
    * It's not really clear if it's worth worrying about these tails,
    * but since we generate the planes for each scissored tri, it's
    * free to trim them in this case.
    * 
    * Note that otherwise, the scissor planes only vary in 'C' value,
    * and even then only on state-changes.  Could alternatively store
    * these planes elsewhere.
    */
   if (nr_planes == 7) {
      tri->plane[3].dcdx = -1;
      tri->plane[3].dcdy = 0;
      tri->plane[3].c = 1-bbox.x0;
      tri->plane[3].ei = 0;
      tri->plane[3].eo = 1;

      tri->plane[4].dcdx = 1;
      tri->plane[4].dcdy = 0;
      tri->plane[4].c = bbox.x1+1;
      tri->plane[4].ei = -1;
      tri->plane[4].eo = 0;

      tri->plane[5].dcdx = 0;
      tri->plane[5].dcdy = 1;
      tri->plane[5].c = 1-bbox.y0;
      tri->plane[5].ei = 0;
      tri->plane[5].eo = 1;

      tri->plane[6].dcdx = 0;
      tri->plane[6].dcdy = -1;
      tri->plane[6].c = bbox.y1+1;
      tri->plane[6].ei = -1;
      tri->plane[6].eo = 0;
   }

   return lp_setup_bin_triangle( setup, tri, &bbox, nr_planes );
}

/*
 * Round to nearest less or equal power of two of the input.
 *
 * Undefined if no bit set exists, so code should check against 0 first.
 */
static INLINE uint32_t 
floor_pot(uint32_t n)
{
#if defined(PIPE_CC_GCC) && defined(PIPE_ARCH_X86)
   if (n == 0)
      return 0;

   __asm__("bsr %1,%0"
          : "=r" (n)
          : "rm" (n));
   return 1 << n;
#else
   n |= (n >>  1);
   n |= (n >>  2);
   n |= (n >>  4);
   n |= (n >>  8);
   n |= (n >> 16);
   return n - (n >> 1);
#endif
}


boolean
lp_setup_bin_triangle( struct lp_setup_context *setup,
                       struct lp_rast_triangle *tri,
                       const struct u_rect *bbox,
                       int nr_planes )
{
   struct lp_scene *scene = setup->scene;
   int i;

   /* What is the largest power-of-two boundary this triangle crosses:
    */
   int dx = floor_pot((bbox->x0 ^ bbox->x1) |
		      (bbox->y0 ^ bbox->y1));

   /* The largest dimension of the rasterized area of the triangle
    * (aligned to a 4x4 grid), rounded down to the nearest power of two:
    */
   int sz = floor_pot((bbox->x1 - (bbox->x0 & ~3)) |
		      (bbox->y1 - (bbox->y0 & ~3)));

   /* Determine which tile(s) intersect the triangle's bounding box
    */
   if (dx < TILE_SIZE)
   {
      int ix0 = bbox->x0 / TILE_SIZE;
      int iy0 = bbox->y0 / TILE_SIZE;
      int px = bbox->x0 & 63 & ~3;
      int py = bbox->y0 & 63 & ~3;
      int mask = px | (py << 8);

      assert(iy0 == bbox->y1 / TILE_SIZE &&
	     ix0 == bbox->x1 / TILE_SIZE);

      if (nr_planes == 3) {
         if (sz < 4)
         {
            /* Triangle is contained in a single 4x4 stamp:
             */

            return lp_scene_bin_command( scene, ix0, iy0,
                                         LP_RAST_OP_TRIANGLE_3_4,
                                         lp_rast_arg_triangle(tri, mask) );
         }

         if (sz < 16)
         {
            /* Triangle is contained in a single 16x16 block:
             */
            return lp_scene_bin_command( scene, ix0, iy0,
                                         LP_RAST_OP_TRIANGLE_3_16,
                                         lp_rast_arg_triangle(tri, mask) );
         }
      }
      else if (nr_planes == 4 && sz < 16) 
      {
         return lp_scene_bin_command( scene, ix0, iy0,
                                      LP_RAST_OP_TRIANGLE_4_16,
                                      lp_rast_arg_triangle(tri, mask) );
      }


      /* Triangle is contained in a single tile:
       */
      return lp_scene_bin_command( scene, ix0, iy0,
                                   lp_rast_tri_tab[nr_planes], 
                                   lp_rast_arg_triangle(tri, (1<<nr_planes)-1) );
   }
   else
   {
      int c[MAX_PLANES];
      int ei[MAX_PLANES];
      int eo[MAX_PLANES];
      int xstep[MAX_PLANES];
      int ystep[MAX_PLANES];
      int x, y;

      int ix0 = bbox->x0 / TILE_SIZE;
      int iy0 = bbox->y0 / TILE_SIZE;
      int ix1 = bbox->x1 / TILE_SIZE;
      int iy1 = bbox->y1 / TILE_SIZE;
      
      for (i = 0; i < nr_planes; i++) {
         c[i] = (tri->plane[i].c + 
                 tri->plane[i].dcdy * iy0 * TILE_SIZE - 
                 tri->plane[i].dcdx * ix0 * TILE_SIZE);

         ei[i] = tri->plane[i].ei << TILE_ORDER;
         eo[i] = tri->plane[i].eo << TILE_ORDER;
         xstep[i] = -(tri->plane[i].dcdx << TILE_ORDER);
         ystep[i] = tri->plane[i].dcdy << TILE_ORDER;
      }



      /* Test tile-sized blocks against the triangle.
       * Discard blocks fully outside the tri.  If the block is fully
       * contained inside the tri, bin an lp_rast_shade_tile command.
       * Else, bin a lp_rast_triangle command.
       */
      for (y = iy0; y <= iy1; y++)
      {
	 boolean in = FALSE;  /* are we inside the triangle? */
	 int cx[MAX_PLANES];

         for (i = 0; i < nr_planes; i++)
            cx[i] = c[i];

	 for (x = ix0; x <= ix1; x++)
	 {
            int out = 0;
            int partial = 0;

            for (i = 0; i < nr_planes; i++) {
               int planeout = cx[i] + eo[i];
               int planepartial = cx[i] + ei[i] - 1;
               out |= (planeout >> 31);
               partial |= (planepartial >> 31) & (1<<i);
            }

            if (out) {
               /* do nothing */
               if (in)
                  break;  /* exiting triangle, all done with this row */
               LP_COUNT(nr_empty_64);
            }
            else if (partial) {
               /* Not trivially accepted by at least one plane - 
                * rasterize/shade partial tile
                */
               int count = util_bitcount(partial);
               in = TRUE;
               if (!lp_scene_bin_command( scene, x, y,
                                          lp_rast_tri_tab[count], 
                                          lp_rast_arg_triangle(tri, partial) ))
                  goto fail;

               LP_COUNT(nr_partially_covered_64);
            }
            else {
               /* triangle covers the whole tile- shade whole tile */
               LP_COUNT(nr_fully_covered_64);
               in = TRUE;
               if (!lp_setup_whole_tile(setup, &tri->inputs, x, y))
                  goto fail;
            }

	    /* Iterate cx values across the region:
	     */
            for (i = 0; i < nr_planes; i++)
               cx[i] += xstep[i];
	 }
      
	 /* Iterate c values down the region:
	  */
         for (i = 0; i < nr_planes; i++)
            c[i] += ystep[i];
      }
   }

   return TRUE;

fail:
   /* Need to disable any partially binned triangle.  This is easier
    * than trying to locate all the triangle, shade-tile, etc,
    * commands which may have been binned.
    */
   tri->inputs.disable = TRUE;
   return FALSE;
}


/**
 * Try to draw the triangle, restart the scene on failure.
 */
static void retry_triangle_ccw( struct lp_setup_context *setup,
                                const float (*v0)[4],
                                const float (*v1)[4],
                                const float (*v2)[4],
                                boolean front)
{
   if (!do_triangle_ccw( setup, v0, v1, v2, front ))
   {
      if (!lp_setup_flush_and_restart(setup))
         return;

      if (!do_triangle_ccw( setup, v0, v1, v2, front ))
         return;
   }
}

static INLINE float
calc_area(const float (*v0)[4],
          const float (*v1)[4],
          const float (*v2)[4])
{
   float dx01 = v0[0][0] - v1[0][0];
   float dy01 = v0[0][1] - v1[0][1];
   float dx20 = v2[0][0] - v0[0][0];
   float dy20 = v2[0][1] - v0[0][1];
   return dx01 * dy20 - dx20 * dy01;
}


/**
 * Draw triangle if it's CW, cull otherwise.
 */
static void triangle_cw( struct lp_setup_context *setup,
			 const float (*v0)[4],
			 const float (*v1)[4],
			 const float (*v2)[4] )
{
   float area = calc_area(v0, v1, v2);

   if (area < 0.0f) 
      retry_triangle_ccw(setup, v0, v2, v1, !setup->ccw_is_frontface);
}


static void triangle_ccw( struct lp_setup_context *setup,
                          const float (*v0)[4],
                          const float (*v1)[4],
                          const float (*v2)[4])
{
   float area = calc_area(v0, v1, v2);

   if (area > 0.0f) 
      retry_triangle_ccw(setup, v0, v1, v2, setup->ccw_is_frontface);
}

/**
 * Draw triangle whether it's CW or CCW.
 */
static void triangle_both( struct lp_setup_context *setup,
			   const float (*v0)[4],
			   const float (*v1)[4],
			   const float (*v2)[4] )
{
   float area = calc_area(v0, v1, v2);

   if (area > 0.0f) 
      retry_triangle_ccw( setup, v0, v1, v2, setup->ccw_is_frontface );
   else if (area < 0.0f)
      retry_triangle_ccw( setup, v0, v2, v1, !setup->ccw_is_frontface );
}


static void triangle_nop( struct lp_setup_context *setup,
			  const float (*v0)[4],
			  const float (*v1)[4],
			  const float (*v2)[4] )
{
}


void 
lp_setup_choose_triangle( struct lp_setup_context *setup )
{
   switch (setup->cullmode) {
   case PIPE_FACE_NONE:
      setup->triangle = triangle_both;
      break;
   case PIPE_FACE_BACK:
      setup->triangle = setup->ccw_is_frontface ? triangle_ccw : triangle_cw;
      break;
   case PIPE_FACE_FRONT:
      setup->triangle = setup->ccw_is_frontface ? triangle_cw : triangle_ccw;
      break;
   default:
      setup->triangle = triangle_nop;
      break;
   }
}
