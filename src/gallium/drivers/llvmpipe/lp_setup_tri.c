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
#include "lp_rast.h"
#include "lp_state_fs.h"

#define NUM_CHANNELS 4

struct tri_info {

   float pixel_offset;

   /* fixed point vertex coordinates */
   int x[3];
   int y[3];

   /* float x,y deltas - all from the original coordinates
    */
   float dy01, dy20;
   float dx01, dx20;
   float oneoverarea;

   const float (*v0)[4];
   const float (*v1)[4];
   const float (*v2)[4];

   boolean frontfacing;
};



   
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



static void linear_coef( struct lp_rast_triangle *tri,
                         const struct tri_info *info,
                         unsigned slot,
                         unsigned vert_attr,
                         unsigned i)
{
   float a0 = info->v0[vert_attr][i];
   float a1 = info->v1[vert_attr][i];
   float a2 = info->v2[vert_attr][i];

   float da01 = a0 - a1;
   float da20 = a2 - a0;
   float dadx = (da01 * info->dy20 - info->dy01 * da20) * info->oneoverarea;
   float dady = (da20 * info->dx01 - info->dx20 * da01) * info->oneoverarea;

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
   tri->inputs.a0[slot][i] = (a0 -
                              (dadx * (info->v0[0][0] - info->pixel_offset) +
                               dady * (info->v0[0][1] - info->pixel_offset)));
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
                              const struct tri_info *info,
                              unsigned slot,
			      unsigned vert_attr,
                              unsigned i)
{
   /* premultiply by 1/w  (v[0][3] is always 1/w):
    */
   float a0 = info->v0[vert_attr][i] * info->v0[0][3];
   float a1 = info->v1[vert_attr][i] * info->v1[0][3];
   float a2 = info->v2[vert_attr][i] * info->v2[0][3];
   float da01 = a0 - a1;
   float da20 = a2 - a0;
   float dadx = (da01 * info->dy20 - info->dy01 * da20) * info->oneoverarea;
   float dady = (da20 * info->dx01 - info->dx20 * da01) * info->oneoverarea;

   tri->inputs.dadx[slot][i] = dadx;
   tri->inputs.dady[slot][i] = dady;
   tri->inputs.a0[slot][i] = (a0 -
                              (dadx * (info->v0[0][0] - info->pixel_offset) +
                               dady * (info->v0[0][1] - info->pixel_offset)));
}


/**
 * Special coefficient setup for gl_FragCoord.
 * X and Y are trivial
 * Z and W are copied from position_coef which should have already been computed.
 * We could do a bit less work if we'd examine gl_FragCoord's swizzle mask.
 */
static void
setup_fragcoord_coef(struct lp_rast_triangle *tri,
                     const struct tri_info *info,
                     unsigned slot,
                     unsigned usage_mask)
{
   /*X*/
   if (usage_mask & TGSI_WRITEMASK_X) {
      tri->inputs.a0[slot][0] = 0.0;
      tri->inputs.dadx[slot][0] = 1.0;
      tri->inputs.dady[slot][0] = 0.0;
   }

   /*Y*/
   if (usage_mask & TGSI_WRITEMASK_Y) {
      tri->inputs.a0[slot][1] = 0.0;
      tri->inputs.dadx[slot][1] = 0.0;
      tri->inputs.dady[slot][1] = 1.0;
   }

   /*Z*/
   if (usage_mask & TGSI_WRITEMASK_Z) {
      linear_coef(tri, info, slot, 0, 2);
   }

   /*W*/
   if (usage_mask & TGSI_WRITEMASK_W) {
      linear_coef(tri, info, slot, 0, 3);
   }
}


/**
 * Setup the fragment input attribute with the front-facing value.
 * \param frontface  is the triangle front facing?
 */
static void setup_facing_coef( struct lp_rast_triangle *tri,
                               unsigned slot,
                               boolean frontface,
                               unsigned usage_mask)
{
   /* convert TRUE to 1.0 and FALSE to -1.0 */
   if (usage_mask & TGSI_WRITEMASK_X)
      constant_coef( tri, slot, 2.0f * frontface - 1.0f, 0 );

   if (usage_mask & TGSI_WRITEMASK_Y)
      constant_coef( tri, slot, 0.0f, 1 ); /* wasted */

   if (usage_mask & TGSI_WRITEMASK_Z)
      constant_coef( tri, slot, 0.0f, 2 ); /* wasted */

   if (usage_mask & TGSI_WRITEMASK_W)
      constant_coef( tri, slot, 0.0f, 3 ); /* wasted */
}


/**
 * Compute the tri->coef[] array dadx, dady, a0 values.
 */
static void setup_tri_coefficients( struct lp_setup_context *setup,
				    struct lp_rast_triangle *tri,
                                    const struct tri_info *info)
{
   unsigned fragcoord_usage_mask = TGSI_WRITEMASK_XYZ;
   unsigned slot;
   unsigned i;

   /* setup interpolation for all the remaining attributes:
    */
   for (slot = 0; slot < setup->fs.nr_inputs; slot++) {
      unsigned vert_attr = setup->fs.input[slot].src_index;
      unsigned usage_mask = setup->fs.input[slot].usage_mask;

      switch (setup->fs.input[slot].interp) {
      case LP_INTERP_CONSTANT:
         if (setup->flatshade_first) {
            for (i = 0; i < NUM_CHANNELS; i++)
               if (usage_mask & (1 << i))
                  constant_coef(tri, slot+1, info->v0[vert_attr][i], i);
         }
         else {
            for (i = 0; i < NUM_CHANNELS; i++)
               if (usage_mask & (1 << i))
                  constant_coef(tri, slot+1, info->v2[vert_attr][i], i);
         }
         break;

      case LP_INTERP_LINEAR:
         for (i = 0; i < NUM_CHANNELS; i++)
            if (usage_mask & (1 << i))
               linear_coef(tri, info, slot+1, vert_attr, i);
         break;

      case LP_INTERP_PERSPECTIVE:
         for (i = 0; i < NUM_CHANNELS; i++)
            if (usage_mask & (1 << i))
               perspective_coef(tri, info, slot+1, vert_attr, i);
         fragcoord_usage_mask |= TGSI_WRITEMASK_W;
         break;

      case LP_INTERP_POSITION:
         /*
          * The generated pixel interpolators will pick up the coeffs from
          * slot 0, so all need to ensure that the usage mask is covers all
          * usages.
          */
         fragcoord_usage_mask |= usage_mask;
         break;

      case LP_INTERP_FACING:
         setup_facing_coef(tri, slot+1, info->frontfacing, usage_mask);
         break;

      default:
         assert(0);
      }
   }

   /* The internal position input is in slot zero:
    */
   setup_fragcoord_coef(tri, info, 0, fragcoord_usage_mask);

   if (0) {
      for (i = 0; i < NUM_CHANNELS; i++) {
         float a0   = tri->inputs.a0  [0][i];
         float dadx = tri->inputs.dadx[0][i];
         float dady = tri->inputs.dady[0][i];

         debug_printf("POS.%c: a0 = %f, dadx = %f, dady = %f\n",
                      "xyzw"[i],
                      a0, dadx, dady);
      }

      for (slot = 0; slot < setup->fs.nr_inputs; slot++) {
         unsigned usage_mask = setup->fs.input[slot].usage_mask;
         for (i = 0; i < NUM_CHANNELS; i++) {
            if (usage_mask & (1 << i)) {
               float a0   = tri->inputs.a0  [1 + slot][i];
               float dadx = tri->inputs.dadx[1 + slot][i];
               float dady = tri->inputs.dady[1 + slot][i];

               debug_printf("IN[%u].%c: a0 = %f, dadx = %f, dady = %f\n",
                            slot,
                            "xyzw"[i],
                            a0, dadx, dady);
            }
         }
      }
   }
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
alloc_triangle(struct lp_scene *scene,
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


lp_rast_cmd lp_rast_tri_tab[8] = {
   NULL,               /* should be impossible */
   lp_rast_triangle_1,
   lp_rast_triangle_2,
   lp_rast_triangle_3,
   lp_rast_triangle_4,
   lp_rast_triangle_5,
   lp_rast_triangle_6,
   lp_rast_triangle_7
};

/**
 * Do basic setup for triangle rasterization and determine which
 * framebuffer tiles are touched.  Put the triangle in the scene's
 * bins for the tiles which we overlap.
 */
static void
do_triangle_ccw(struct lp_setup_context *setup,
		const float (*v1)[4],
		const float (*v2)[4],
		const float (*v3)[4],
		boolean frontfacing )
{

   struct lp_scene *scene = lp_setup_get_current_scene(setup);
   struct lp_fragment_shader_variant *variant = setup->fs.current.variant;
   struct lp_rast_triangle *tri;
   struct tri_info info;
   int area;
   struct u_rect bbox;
   int ix0, ix1, iy0, iy1;
   unsigned tri_bytes;
   int i;
   int nr_planes = 3;
      
   if (0)
      lp_setup_print_triangle(setup, v1, v2, v3);

   if (setup->scissor_test) {
      nr_planes = 7;
   }
   else {
      nr_planes = 3;
   }

   /* x/y positions in fixed point */
   info.x[0] = subpixel_snap(v1[0][0] - setup->pixel_offset);
   info.x[1] = subpixel_snap(v2[0][0] - setup->pixel_offset);
   info.x[2] = subpixel_snap(v3[0][0] - setup->pixel_offset);
   info.y[0] = subpixel_snap(v1[0][1] - setup->pixel_offset);
   info.y[1] = subpixel_snap(v2[0][1] - setup->pixel_offset);
   info.y[2] = subpixel_snap(v3[0][1] - setup->pixel_offset);



   /* Bounding rectangle (in pixels) */
   {
      /* Yes this is necessary to accurately calculate bounding boxes
       * with the two fill-conventions we support.  GL (normally) ends
       * up needing a bottom-left fill convention, which requires
       * slightly different rounding.
       */
      int adj = (setup->pixel_offset != 0) ? 1 : 0;

      bbox.x0 = (MIN3(info.x[0], info.x[1], info.x[2]) + (FIXED_ONE-1)) >> FIXED_ORDER;
      bbox.x1 = (MAX3(info.x[0], info.x[1], info.x[2]) + (FIXED_ONE-1)) >> FIXED_ORDER;
      bbox.y0 = (MIN3(info.y[0], info.y[1], info.y[2]) + (FIXED_ONE-1) + adj) >> FIXED_ORDER;
      bbox.y1 = (MAX3(info.y[0], info.y[1], info.y[2]) + (FIXED_ONE-1) + adj) >> FIXED_ORDER;

      /* Inclusive coordinates:
       */
      bbox.x1--;
      bbox.y1--;
   }

   if (bbox.x1 < bbox.x0 ||
       bbox.y1 < bbox.y0) {
      if (0) debug_printf("empty bounding box\n");
      LP_COUNT(nr_culled_tris);
      return;
   }

   if (!u_rect_test_intersection(&setup->draw_region, &bbox)) {
      if (0) debug_printf("offscreen\n");
      LP_COUNT(nr_culled_tris);
      return;
   }

   u_rect_find_intersection(&setup->draw_region, &bbox);

   tri = alloc_triangle(scene,
                        setup->fs.nr_inputs,
                        nr_planes,
                        &tri_bytes);
   if (!tri)
      return;

#ifdef DEBUG
   tri->v[0][0] = v1[0][0];
   tri->v[1][0] = v2[0][0];
   tri->v[2][0] = v3[0][0];
   tri->v[0][1] = v1[0][1];
   tri->v[1][1] = v2[0][1];
   tri->v[2][1] = v3[0][1];
#endif

   tri->plane[0].dcdy = info.x[0] - info.x[1];
   tri->plane[1].dcdy = info.x[1] - info.x[2];
   tri->plane[2].dcdy = info.x[2] - info.x[0];

   tri->plane[0].dcdx = info.y[0] - info.y[1];
   tri->plane[1].dcdx = info.y[1] - info.y[2];
   tri->plane[2].dcdx = info.y[2] - info.y[0];

   area = (tri->plane[0].dcdy * tri->plane[2].dcdx -
           tri->plane[2].dcdy * tri->plane[0].dcdx);

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


   /* 
    */
   info.pixel_offset = setup->pixel_offset;
   info.v0 = v1;
   info.v1 = v2;
   info.v2 = v3;
   info.dx01 = info.v0[0][0] - info.v1[0][0];
   info.dx20 = info.v2[0][0] - info.v0[0][0];
   info.dy01 = info.v0[0][1] - info.v1[0][1];
   info.dy20 = info.v2[0][1] - info.v0[0][1];
   info.oneoverarea = 1.0f / (info.dx01 * info.dy20 - info.dx20 * info.dy01);
   info.frontfacing = frontfacing;

   /* Setup parameter interpolants:
    */
   setup_tri_coefficients( setup, tri, &info );

   tri->inputs.facing = frontfacing ? 1.0F : -1.0F;
   tri->inputs.state = setup->fs.stored;


  
   for (i = 0; i < 3; i++) {
      struct lp_rast_plane *plane = &tri->plane[i];

      /* half-edge constants, will be interated over the whole render
       * target.
       */
      plane->c = plane->dcdx * info.x[i] - plane->dcdy * info.y[i];

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
       */         
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

      plane->dcdx *= FIXED_ONE;
      plane->dcdy *= FIXED_ONE;

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


   /*
    * All fields of 'tri' are now set.  The remaining code here is
    * concerned with binning.
    */

   /* Convert to tile coordinates, and inclusive ranges:
    */
   if (nr_planes == 3) {
      int ix0 = bbox.x0 / 16;
      int iy0 = bbox.y0 / 16;
      int ix1 = bbox.x1 / 16;
      int iy1 = bbox.y1 / 16;
      
      if (iy0 == iy1 && ix0 == ix1)
      {

	 /* Triangle is contained in a single 16x16 block:
	  */
	 int mask = (ix0 & 3) | ((iy0 & 3) << 4);

	 lp_scene_bin_command( scene, ix0/4, iy0/4,
			       lp_rast_triangle_3_16,
			       lp_rast_arg_triangle(tri, mask) );
	 return;
      }
   }

   ix0 = bbox.x0 / TILE_SIZE;
   iy0 = bbox.y0 / TILE_SIZE;
   ix1 = bbox.x1 / TILE_SIZE;
   iy1 = bbox.y1 / TILE_SIZE;

   /*
    * Clamp to framebuffer size
    */
   assert(ix0 == MAX2(ix0, 0));
   assert(iy0 == MAX2(iy0, 0));
   assert(ix1 == MIN2(ix1, scene->tiles_x - 1));
   assert(iy1 == MIN2(iy1, scene->tiles_y - 1));

   /* Determine which tile(s) intersect the triangle's bounding box
    */
   if (iy0 == iy1 && ix0 == ix1)
   {
      /* Triangle is contained in a single tile:
       */
      lp_scene_bin_command( scene, ix0, iy0,
                            lp_rast_tri_tab[nr_planes], 
			    lp_rast_arg_triangle(tri, (1<<nr_planes)-1) );
   }
   else
   {
      int c[7];
      int ei[7];
      int eo[7];
      int xstep[7];
      int ystep[7];
      int x, y;
      
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
	 int cx[7];

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
               lp_scene_bin_command( scene, x, y,
                                     lp_rast_tri_tab[count], 
                                     lp_rast_arg_triangle(tri, partial) );

               LP_COUNT(nr_partially_covered_64);
            }
            else {
               /* triangle covers the whole tile- shade whole tile */
               LP_COUNT(nr_fully_covered_64);
               in = TRUE;
	       if (variant->opaque &&
	           !setup->fb.zsbuf) {
	          lp_scene_bin_reset( scene, x, y );
	       }
               lp_scene_bin_command( scene, x, y,
				     lp_rast_shade_tile,
				     lp_rast_arg_inputs(&tri->inputs) );
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
}


/**
 * Draw triangle if it's CW, cull otherwise.
 */
static void triangle_cw( struct lp_setup_context *setup,
			 const float (*v0)[4],
			 const float (*v1)[4],
			 const float (*v2)[4] )
{
   do_triangle_ccw( setup, v1, v0, v2, !setup->ccw_is_frontface );
}


/**
 * Draw triangle if it's CCW, cull otherwise.
 */
static void triangle_ccw( struct lp_setup_context *setup,
			 const float (*v0)[4],
			 const float (*v1)[4],
			 const float (*v2)[4] )
{
   do_triangle_ccw( setup, v0, v1, v2, setup->ccw_is_frontface );
}



/**
 * Draw triangle whether it's CW or CCW.
 */
static void triangle_both( struct lp_setup_context *setup,
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
   const float det = ex * fy - ey * fx;
   if (det < 0.0f) 
      triangle_ccw( setup, v0, v1, v2 );
   else if (det > 0.0f)
      triangle_cw( setup, v0, v1, v2 );
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
