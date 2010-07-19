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
 * Binning code for lines
 */

#include "util/u_math.h"
#include "util/u_memory.h"
#include "lp_perf.h"
#include "lp_setup_context.h"
#include "lp_rast.h"
#include "lp_state_fs.h"

#define NUM_CHANNELS 4


static const int step_scissor_minx[16] = {
   0, 1, 0, 1,
   2, 3, 2, 3,
   0, 1, 0, 1,
   2, 3, 2, 3
};

static const int step_scissor_maxx[16] = {
    0, -1,  0, -1,
   -2, -3, -2, -3,
    0, -1,  0, -1,
   -2, -3, -2, -3
};

static const int step_scissor_miny[16] = {
   0, 0, 1, 1,
   0, 0, 1, 1,
   2, 2, 3, 3,
   2, 2, 3, 3
};

static const int step_scissor_maxy[16] = {
    0,  0, -1, -1,
    0,  0, -1, -1,
   -2, -2, -3, -3,
   -2, -2, -3, -3
};



/**
 * Compute a0 for a constant-valued coefficient (GL_FLAT shading).
 */
static void constant_coef( struct lp_setup_context *setup,
                           struct lp_rast_triangle *tri,
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
static void linear_coef( struct lp_setup_context *setup,
                         struct lp_rast_triangle *tri,
                         float oneoverarea,
                         unsigned slot,
                         const float (*v1)[4],
                         const float (*v2)[4],
                         unsigned vert_attr,
                         unsigned i)
{
   float a1 = v1[vert_attr][i]; 
   float a2 = v2[vert_attr][i];
      
   float da21 = a1 - a2;   
   float dadx = da21 * tri->dx * oneoverarea;
   float dady = da21 * tri->dy * oneoverarea;

   tri->inputs.dadx[slot][i] = dadx;
   tri->inputs.dady[slot][i] = dady;  
   
   tri->inputs.a0[slot][i] = (a1 -
                              (dadx * (v1[0][0] - setup->pixel_offset) +
                               dady * (v1[0][1] - setup->pixel_offset)));
}


/**
 * Compute a0, dadx and dady for a perspective-corrected interpolant,
 * for a triangle.
 * We basically multiply the vertex value by 1/w before computing
 * the plane coefficients (a0, dadx, dady).
 * Later, when we compute the value at a particular fragment position we'll
 * divide the interpolated value by the interpolated W at that fragment.
 */
static void perspective_coef( struct lp_setup_context *setup,
                              struct lp_rast_triangle *tri,
                              float oneoverarea,
                              unsigned slot,
                              const float (*v1)[4],
                              const float (*v2)[4],
                              unsigned vert_attr,
                              unsigned i)
{
   /* premultiply by 1/w  (v[0][3] is always 1/w):
    */
   float a1 = v1[vert_attr][i] * v1[0][3];
   float a2 = v2[vert_attr][i] * v2[0][3];

   float da21 = a1 - a2;   
   float dadx = da21 * tri->dx * oneoverarea;
   float dady = da21 * tri->dy * oneoverarea;

   tri->inputs.dadx[slot][i] = dadx;
   tri->inputs.dady[slot][i] = dady;
   
   tri->inputs.a0[slot][i] = (a1 -
                              (dadx * (v1[0][0] - setup->pixel_offset) +
                               dady * (v1[0][1] - setup->pixel_offset)));
}

/**
 * Compute the tri->coef[] array dadx, dady, a0 values.
 */
static void setup_line_coefficients( struct lp_setup_context *setup,
                                     struct lp_rast_triangle *tri,
                                     float oneoverarea,
                                     const float (*v1)[4],
                                     const float (*v2)[4])
{
   unsigned fragcoord_usage_mask = TGSI_WRITEMASK_XYZ;
   unsigned slot;

   /* setup interpolation for all the remaining attributes:
    */
   for (slot = 0; slot < setup->fs.nr_inputs; slot++) {
      unsigned vert_attr = setup->fs.input[slot].src_index;
      unsigned usage_mask = setup->fs.input[slot].usage_mask;
      unsigned i;
           
      switch (setup->fs.input[slot].interp) {
      case LP_INTERP_CONSTANT:
         if (setup->flatshade_first) {
            for (i = 0; i < NUM_CHANNELS; i++)
               if (usage_mask & (1 << i))
                  constant_coef(setup, tri, slot+1, v1[vert_attr][i], i);
         }
         else {
            for (i = 0; i < NUM_CHANNELS; i++)
               if (usage_mask & (1 << i))
                  constant_coef(setup, tri, slot+1, v2[vert_attr][i], i);
         }
         break;

      case LP_INTERP_LINEAR:
         for (i = 0; i < NUM_CHANNELS; i++)
            if (usage_mask & (1 << i))
               linear_coef(setup, tri, oneoverarea, slot+1, v1, v2, vert_attr, i);
         break;

      case LP_INTERP_PERSPECTIVE:
         for (i = 0; i < NUM_CHANNELS; i++)
            if (usage_mask & (1 << i))
               perspective_coef(setup, tri, oneoverarea, slot+1, v1, v2, vert_attr, i);
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

      default:
         assert(0);
      }
   }

   /* The internal position input is in slot zero:
    */
   lp_setup_fragcoord_coef(setup, tri, oneoverarea, 0, v1, v2, v2,
                            fragcoord_usage_mask);
}



static INLINE int subpixel_snap( float a )
{
   return util_iround(FIXED_ONE * a);
}


/**
 * Print line vertex attribs (for debug).
 */
static void
print_line(struct lp_setup_context *setup,
           const float (*v1)[4],
           const float (*v2)[4])
{
   uint i;

   debug_printf("llvmpipe line\n");
   for (i = 0; i < 1 + setup->fs.nr_inputs; i++) {
      debug_printf("  v1[%d]:  %f %f %f %f\n", i,
                   v1[i][0], v1[i][1], v1[i][2], v1[i][3]);
   }
   for (i = 0; i < 1 + setup->fs.nr_inputs; i++) {
      debug_printf("  v2[%d]:  %f %f %f %f\n", i,
                   v2[i][0], v2[i][1], v2[i][2], v2[i][3]);
   }
}


static void
lp_setup_line( struct lp_setup_context *setup,
               const float (*v1)[4],
               const float (*v2)[4])
{
   struct lp_scene *scene = lp_setup_get_current_scene(setup);
   struct lp_rast_triangle *line;
   float oneoverarea;
   float half_width = setup->line_width / 2;
   int minx, maxx, miny, maxy;
   int ix0, ix1, iy0, iy1;
   unsigned tri_bytes;
   int x[4]; 
   int y[4];
   int i;
   int nr_planes = 4;
   boolean opaque;
         
   if (0)
      print_line(setup, v1, v2);

   if (setup->scissor_test) {
      nr_planes = 8;
   }
   else {
      nr_planes = 4;
   }

   line = lp_setup_alloc_triangle(scene,
                                  setup->fs.nr_inputs,
                                  nr_planes,
                                  &tri_bytes);
   if (!line)
      return;

#ifndef DEBUG
   line->v[0][0] = v1[0][0];
   line->v[1][0] = v2[0][0];   
   line->v[0][1] = v1[0][1];
   line->v[1][1] = v2[0][1];
#endif

   /* pre-calculation(based on given vertices) to determine if line is
    * more horizontal or more vertical
    */
   line->dx = v1[0][0] - v2[0][0];
   line->dy = v1[0][1] - v2[0][1];
   
   /* x-major line */
   if (fabsf(line->dx) >= fabsf(line->dy)) {
      if (line->dx < 0) {
         /* if v2 is to the right of v1, swap pointers */
         const float (*temp)[4] = v1;
         v1 = v2;
         v2 = temp;
         line->dx = -line->dx;
         line->dy = -line->dy;
      }
      
      /* x/y positions in fixed point */
      x[0] = subpixel_snap(v1[0][0] - setup->pixel_offset);
      x[1] = subpixel_snap(v2[0][0] - setup->pixel_offset);
      x[2] = subpixel_snap(v2[0][0] - setup->pixel_offset);
      x[3] = subpixel_snap(v1[0][0] - setup->pixel_offset);
      
      y[0] = subpixel_snap(v1[0][1] - half_width - setup->pixel_offset);
      y[1] = subpixel_snap(v2[0][1] - half_width - setup->pixel_offset);
      y[2] = subpixel_snap(v2[0][1] + half_width - setup->pixel_offset);
      y[3] = subpixel_snap(v1[0][1] + half_width - setup->pixel_offset);
   }
   else{
      /* y-major line */
      if (line->dy > 0) {
         /* if v2 is on top of v1, swap pointers */
         const float (*temp)[4] = v1;
         v1 = v2;
         v2 = temp; 
         line->dx = -line->dx;
         line->dy = -line->dy;
      }
 
      x[0] = subpixel_snap(v1[0][0] - half_width - setup->pixel_offset);
      x[1] = subpixel_snap(v2[0][0] - half_width - setup->pixel_offset);
      x[2] = subpixel_snap(v2[0][0] + half_width - setup->pixel_offset);
      x[3] = subpixel_snap(v1[0][0] + half_width - setup->pixel_offset);
     
      y[0] = subpixel_snap(v1[0][1] - setup->pixel_offset);
      y[1] = subpixel_snap(v2[0][1] - setup->pixel_offset);
      y[2] = subpixel_snap(v2[0][1] - setup->pixel_offset);
      y[3] = subpixel_snap(v1[0][1] - setup->pixel_offset);
   }

   /* calculate the deltas */
   line->plane[0].dcdy = x[0] - x[1];
   line->plane[1].dcdy = x[1] - x[2];
   line->plane[2].dcdy = x[2] - x[3];
   line->plane[3].dcdy = x[3] - x[0];

   line->plane[0].dcdx = y[0] - y[1];
   line->plane[1].dcdx = y[1] - y[2];
   line->plane[2].dcdx = y[2] - y[3];
   line->plane[3].dcdx = y[3] - y[0];


   LP_COUNT(nr_tris);

 
   /* Bounding rectangle (in pixels) */
   {
      /* Yes this is necessary to accurately calculate bounding boxes
       * with the two fill-conventions we support.  GL (normally) ends
       * up needing a bottom-left fill convention, which requires
       * slightly different rounding.
       */
      int adj = (setup->pixel_offset != 0) ? 1 : 0;

      minx = (MIN4(x[0], x[1], x[2], x[3]) + (FIXED_ONE-1)) >> FIXED_ORDER;
      maxx = (MAX4(x[0], x[1], x[2], x[3]) + (FIXED_ONE-1)) >> FIXED_ORDER;
      miny = (MIN4(y[0], y[1], y[3], y[3]) + (FIXED_ONE-1) + adj) >> FIXED_ORDER;
      maxy = (MAX4(y[0], y[1], y[3], y[3]) + (FIXED_ONE-1) + adj) >> FIXED_ORDER;
   }

   if (setup->scissor_test) {
      minx = MAX2(minx, setup->scissor.current.minx);
      maxx = MIN2(maxx, setup->scissor.current.maxx);
      miny = MAX2(miny, setup->scissor.current.miny);
      maxy = MIN2(maxy, setup->scissor.current.maxy);
   }
   else {
      minx = MAX2(minx, 0);
      miny = MAX2(miny, 0);
      maxx = MIN2(maxx, scene->fb.width);
      maxy = MIN2(maxy, scene->fb.height);
   }


   if (miny >= maxy || minx >= maxx) {
      lp_scene_putback_data( scene, tri_bytes );
      return;
   }

   oneoverarea = 1.0f / (line->dx * line->dx  + line->dy * line->dy);    

   /* Setup parameter interpolants:
    */
   setup_line_coefficients( setup, line, oneoverarea, v1, v2); 

   for (i = 0; i < 4; i++) {
      struct lp_rast_plane *plane = &line->plane[i];

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

      plane->step = line->step[i];

      /* Fill in the inputs.step[][] arrays.
       * We've manually unrolled some loops here.
       */
#define SETUP_STEP(j, x, y) \
      line->step[i][j] = y * plane->dcdy - x * plane->dcdx                                     
      
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
   if (nr_planes == 8) {
      line->plane[4].step = step_scissor_maxx;
      line->plane[4].dcdx = 1;
      line->plane[4].dcdy = 0;
      line->plane[4].c = maxx;
      line->plane[4].ei = -1;
      line->plane[4].eo = 0;

      line->plane[5].step = step_scissor_miny;
      line->plane[5].dcdx = 0;
      line->plane[5].dcdy = 1;
      line->plane[5].c = 1-miny;
      line->plane[5].ei = 0;
      line->plane[5].eo = 1;

      line->plane[6].step = step_scissor_maxy;
      line->plane[6].dcdx = 0;
      line->plane[6].dcdy = -1;
      line->plane[6].c = maxy;
      line->plane[6].ei = -1;
      line->plane[6].eo = 0;

      line->plane[7].step = step_scissor_minx;
      line->plane[7].dcdx = -1;
      line->plane[7].dcdy = 0;
      line->plane[7].c = 1-minx;
      line->plane[7].ei = 0;
      line->plane[7].eo = 1;
   }


   /*
    * All fields of 'tri' are now set.  The remaining code here is
    * concerned with binning.
    */

   /* Convert to tile coordinates, and inclusive ranges:
    */
   ix0 = minx / TILE_SIZE;
   iy0 = miny / TILE_SIZE;
   ix1 = (maxx-1) / TILE_SIZE;
   iy1 = (maxy-1) / TILE_SIZE;

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
                            lp_rast_arg_triangle(line, (1<<nr_planes)-1) );
   }
   else
   {
      int c[8];
      int ei[8];
      int eo[8];
      int xstep[8];
      int ystep[8];
      int x, y;
      int is_blit = -1; /* undetermined */
      
      for (i = 0; i < nr_planes; i++) {
         c[i] = (line->plane[i].c + 
                 line->plane[i].dcdy * iy0 * TILE_SIZE - 
                 line->plane[i].dcdx * ix0 * TILE_SIZE);

         ei[i] = line->plane[i].ei << TILE_ORDER;
         eo[i] = line->plane[i].eo << TILE_ORDER;
         xstep[i] = -(line->plane[i].dcdx << TILE_ORDER);
         ystep[i] = line->plane[i].dcdy << TILE_ORDER;
      }



      /* Test tile-sized blocks against the triangle.
       * Discard blocks fully outside the tri.  If the block is fully
       * contained inside the tri, bin an lp_rast_shade_tile command.
       * Else, bin a lp_rast_triangle command.
       */
      for (y = iy0; y <= iy1; y++)
      {
         boolean in = FALSE;  /* are we inside the triangle? */
         int cx[8];

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
                                     lp_rast_arg_triangle(line, partial) );

               LP_COUNT(nr_partially_covered_64);
            }
            else {
               /* triangle covers the whole tile- shade whole tile */
               LP_COUNT(nr_fully_covered_64);
               in = TRUE;
               /* leverages on existing code in lp_setup_tri.c */ 
               do_triangle_ccw_whole_tile(setup, scene, line, x, y,
                                          opaque, &is_blit);
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


void lp_setup_choose_line( struct lp_setup_context *setup ) 
{ 
   setup->line = lp_setup_line;
}


