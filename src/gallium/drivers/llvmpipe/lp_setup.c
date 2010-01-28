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

/**
 * \brief  Primitive rasterization/rendering (points, lines, triangles)
 *
 * \author  Keith Whitwell <keith@tungstengraphics.com>
 * \author  Brian Paul
 */

#include "lp_context.h"
#include "lp_quad.h"
#include "lp_setup.h"
#include "lp_state.h"
#include "draw/draw_context.h"
#include "draw/draw_vertex.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "lp_bld_debug.h"
#include "lp_tile_cache.h"
#include "lp_tile_soa.h"


#define DEBUG_VERTS 0
#define DEBUG_FRAGS 0

/**
 * Triangle edge info
 */
struct edge {
   float dx;		/**< X(v1) - X(v0), used only during setup */
   float dy;		/**< Y(v1) - Y(v0), used only during setup */
   float dxdy;		/**< dx/dy */
   float sx, sy;	/**< first sample point coord */
   int lines;		/**< number of lines on this edge */
};


#define MAX_QUADS 16


/**
 * Triangle setup info (derived from draw_stage).
 * Also used for line drawing (taking some liberties).
 */
struct setup_context {
   struct llvmpipe_context *llvmpipe;

   /* Vertices are just an array of floats making up each attribute in
    * turn.  Currently fixed at 4 floats, but should change in time.
    * Codegen will help cope with this.
    */
   const float (*vmax)[4];
   const float (*vmid)[4];
   const float (*vmin)[4];
   const float (*vprovoke)[4];

   struct edge ebot;
   struct edge etop;
   struct edge emaj;

   float oneoverarea;
   int facing;

   float pixel_offset;

   struct quad_header quad[MAX_QUADS];
   struct quad_header *quad_ptrs[MAX_QUADS];
   unsigned count;

   struct quad_interp_coef coef;

   struct {
      int left[2];   /**< [0] = row0, [1] = row1 */
      int right[2];
      int y;
   } span;

#if DEBUG_FRAGS
   uint numFragsEmitted;  /**< per primitive */
   uint numFragsWritten;  /**< per primitive */
#endif

   unsigned winding;		/* which winding to cull */
};



/**
 * Execute fragment shader for the four fragments in the quad.
 */
PIPE_ALIGN_STACK
static void
shade_quads(struct llvmpipe_context *llvmpipe,
            struct quad_header *quads[],
            unsigned nr)
{
   struct lp_fragment_shader *fs = llvmpipe->fs;
   struct quad_header *quad = quads[0];
   const unsigned x = quad->input.x0;
   const unsigned y = quad->input.y0;
   uint8_t *tile;
   uint8_t *color;
   void *depth;
   PIPE_ALIGN_VAR(16) uint32_t mask[4][NUM_CHANNELS];
   unsigned chan_index;
   unsigned q;

   assert(fs->current);
   if(!fs->current)
      return;

   /* Sanity checks */
   assert(nr * QUAD_SIZE == TILE_VECTOR_HEIGHT * TILE_VECTOR_WIDTH);
   assert(x % TILE_VECTOR_WIDTH == 0);
   assert(y % TILE_VECTOR_HEIGHT == 0);
   for (q = 0; q < nr; ++q) {
      assert(quads[q]->input.x0 == x + q*2);
      assert(quads[q]->input.y0 == y);
   }

   /* mask */
   for (q = 0; q < 4; ++q)
      for (chan_index = 0; chan_index < NUM_CHANNELS; ++chan_index)
         mask[q][chan_index] = quads[q]->inout.mask & (1 << chan_index) ? ~0 : 0;

   /* color buffer */
   if(llvmpipe->framebuffer.nr_cbufs >= 1 &&
      llvmpipe->framebuffer.cbufs[0]) {
      tile = lp_get_cached_tile(llvmpipe->cbuf_cache[0], x, y);
      color = &TILE_PIXEL(tile, x & (TILE_SIZE-1), y & (TILE_SIZE-1), 0);
   }
   else
      color = NULL;

   /* depth buffer */
   if(llvmpipe->zsbuf_map) {
      assert((x % 2) == 0);
      assert((y % 2) == 0);
      depth = llvmpipe->zsbuf_map +
              y*llvmpipe->zsbuf_transfer->stride +
              2*x*util_format_get_blocksize(llvmpipe->zsbuf_transfer->texture->format);
   }
   else
      depth = NULL;

   /* XXX: This will most likely fail on 32bit x86 without -mstackrealign */
   assert(lp_check_alignment(mask, 16));

   assert(lp_check_alignment(depth, 16));
   assert(lp_check_alignment(color, 16));
   assert(lp_check_alignment(llvmpipe->jit_context.blend_color, 16));

   /* run shader */
   fs->current->jit_function( &llvmpipe->jit_context,
                              x, y,
                              quad->coef->a0,
                              quad->coef->dadx,
                              quad->coef->dady,
                              &mask[0][0],
                              color,
                              depth);
}




/**
 * Do triangle cull test using tri determinant (sign indicates orientation)
 * \return true if triangle is to be culled.
 */
static INLINE boolean
cull_tri(const struct setup_context *setup, float det)
{
   if (det != 0) {   
      /* if (det < 0 then Z points toward camera and triangle is 
       * counter-clockwise winding.
       */
      unsigned winding = (det < 0) ? PIPE_WINDING_CCW : PIPE_WINDING_CW;

      if ((winding & setup->winding) == 0)
	 return FALSE;
   }

   /* Culled:
    */
   return TRUE;
}



/**
 * Clip setup->quad against the scissor/surface bounds.
 */
static INLINE void
quad_clip( struct setup_context *setup, struct quad_header *quad )
{
   const struct pipe_scissor_state *cliprect = &setup->llvmpipe->cliprect;
   const int minx = (int) cliprect->minx;
   const int maxx = (int) cliprect->maxx;
   const int miny = (int) cliprect->miny;
   const int maxy = (int) cliprect->maxy;

   if (quad->input.x0 >= maxx ||
       quad->input.y0 >= maxy ||
       quad->input.x0 + 1 < minx ||
       quad->input.y0 + 1 < miny) {
      /* totally clipped */
      quad->inout.mask = 0x0;
      return;
   }
   if (quad->input.x0 < minx)
      quad->inout.mask &= (MASK_BOTTOM_RIGHT | MASK_TOP_RIGHT);
   if (quad->input.y0 < miny)
      quad->inout.mask &= (MASK_BOTTOM_LEFT | MASK_BOTTOM_RIGHT);
   if (quad->input.x0 == maxx - 1)
      quad->inout.mask &= (MASK_BOTTOM_LEFT | MASK_TOP_LEFT);
   if (quad->input.y0 == maxy - 1)
      quad->inout.mask &= (MASK_TOP_LEFT | MASK_TOP_RIGHT);
}



/**
 * Given an X or Y coordinate, return the block/quad coordinate that it
 * belongs to.
 */
static INLINE int block( int x )
{
   return x & ~(2-1);
}

static INLINE int block_x( int x )
{
   return x & ~(TILE_VECTOR_WIDTH - 1);
}


/**
 * Emit a quad (pass to next stage) with clipping.
 */
static INLINE void
clip_emit_quad( struct setup_context *setup, struct quad_header *quad )
{
   quad_clip( setup, quad );

   if (quad->inout.mask) {
      struct llvmpipe_context *lp = setup->llvmpipe;

#if 1
      /* XXX: The blender expects 4 quads. This is far from efficient, but
       * until we codegenerate single-quad variants of the fragment pipeline
       * we need this hack. */
      const unsigned nr_quads = TILE_VECTOR_HEIGHT*TILE_VECTOR_WIDTH/QUAD_SIZE;
      struct quad_header quads[4];
      struct quad_header *quad_ptrs[4];
      int x0 = block_x(quad->input.x0);
      unsigned i;

      assert(nr_quads == 4);

      for(i = 0; i < nr_quads; ++i) {
         int x = x0 + 2*i;
         if(x == quad->input.x0)
            memcpy(&quads[i], quad, sizeof quads[i]);
         else {
            memset(&quads[i], 0, sizeof quads[i]);
            quads[i].input.x0 = x;
            quads[i].input.y0 = quad->input.y0;
            quads[i].coef = quad->coef;
         }
         quad_ptrs[i] = &quads[i];
      }

      shade_quads( lp, quad_ptrs, nr_quads );
#else
      shade_quads( lp, &quad, 1 );
#endif
   }
}


/**
 * Render a horizontal span of quads
 */
static void flush_spans( struct setup_context *setup )
{
   const int step = TILE_VECTOR_WIDTH;
   const int xleft0 = setup->span.left[0];
   const int xleft1 = setup->span.left[1];
   const int xright0 = setup->span.right[0];
   const int xright1 = setup->span.right[1];


   int minleft = block_x(MIN2(xleft0, xleft1));
   int maxright = MAX2(xright0, xright1);
   int x;

   for (x = minleft; x < maxright; x += step) {
      unsigned skip_left0 = CLAMP(xleft0 - x, 0, step);
      unsigned skip_left1 = CLAMP(xleft1 - x, 0, step);
      unsigned skip_right0 = CLAMP(x + step - xright0, 0, step);
      unsigned skip_right1 = CLAMP(x + step - xright1, 0, step);
      unsigned lx = x;
      const unsigned nr_quads = TILE_VECTOR_HEIGHT*TILE_VECTOR_WIDTH/QUAD_SIZE;
      unsigned q = 0;

      unsigned skipmask_left0 = (1U << skip_left0) - 1U;
      unsigned skipmask_left1 = (1U << skip_left1) - 1U;

      /* These calculations fail when step == 32 and skip_right == 0.
       */
      unsigned skipmask_right0 = ~0U << (unsigned)(step - skip_right0);
      unsigned skipmask_right1 = ~0U << (unsigned)(step - skip_right1);

      unsigned mask0 = ~skipmask_left0 & ~skipmask_right0;
      unsigned mask1 = ~skipmask_left1 & ~skipmask_right1;

      if (mask0 | mask1) {
         for(q = 0; q < nr_quads; ++q) {
            unsigned quadmask = (mask0 & 3) | ((mask1 & 3) << 2);
            setup->quad[q].input.x0 = lx;
            setup->quad[q].input.y0 = setup->span.y;
            setup->quad[q].inout.mask = quadmask;
            setup->quad_ptrs[q] = &setup->quad[q];
            mask0 >>= 2;
            mask1 >>= 2;
            lx += 2;
         }
         assert(!(mask0 | mask1));

         shade_quads(setup->llvmpipe, setup->quad_ptrs, nr_quads );
      }
   }


   setup->span.y = 0;
   setup->span.right[0] = 0;
   setup->span.right[1] = 0;
   setup->span.left[0] = 1000000;     /* greater than right[0] */
   setup->span.left[1] = 1000000;     /* greater than right[1] */
}


#if DEBUG_VERTS
static void print_vertex(const struct setup_context *setup,
                         const float (*v)[4])
{
   int i;
   debug_printf("   Vertex: (%p)\n", v);
   for (i = 0; i < setup->quad[0].nr_attrs; i++) {
      debug_printf("     %d: %f %f %f %f\n",  i,
              v[i][0], v[i][1], v[i][2], v[i][3]);
      if (util_is_inf_or_nan(v[i][0])) {
         debug_printf("   NaN!\n");
      }
   }
}
#endif

/**
 * Sort the vertices from top to bottom order, setting up the triangle
 * edge fields (ebot, emaj, etop).
 * \return FALSE if coords are inf/nan (cull the tri), TRUE otherwise
 */
static boolean setup_sort_vertices( struct setup_context *setup,
                                    float det,
                                    const float (*v0)[4],
                                    const float (*v1)[4],
                                    const float (*v2)[4] )
{
   setup->vprovoke = v2;

   /* determine bottom to top order of vertices */
   {
      float y0 = v0[0][1];
      float y1 = v1[0][1];
      float y2 = v2[0][1];
      if (y0 <= y1) {
	 if (y1 <= y2) {
	    /* y0<=y1<=y2 */
	    setup->vmin = v0;
	    setup->vmid = v1;
	    setup->vmax = v2;
	 }
	 else if (y2 <= y0) {
	    /* y2<=y0<=y1 */
	    setup->vmin = v2;
	    setup->vmid = v0;
	    setup->vmax = v1;
	 }
	 else {
	    /* y0<=y2<=y1 */
	    setup->vmin = v0;
	    setup->vmid = v2;
	    setup->vmax = v1;
	 }
      }
      else {
	 if (y0 <= y2) {
	    /* y1<=y0<=y2 */
	    setup->vmin = v1;
	    setup->vmid = v0;
	    setup->vmax = v2;
	 }
	 else if (y2 <= y1) {
	    /* y2<=y1<=y0 */
	    setup->vmin = v2;
	    setup->vmid = v1;
	    setup->vmax = v0;
	 }
	 else {
	    /* y1<=y2<=y0 */
	    setup->vmin = v1;
	    setup->vmid = v2;
	    setup->vmax = v0;
	 }
      }
   }

   setup->ebot.dx = setup->vmid[0][0] - setup->vmin[0][0];
   setup->ebot.dy = setup->vmid[0][1] - setup->vmin[0][1];
   setup->emaj.dx = setup->vmax[0][0] - setup->vmin[0][0];
   setup->emaj.dy = setup->vmax[0][1] - setup->vmin[0][1];
   setup->etop.dx = setup->vmax[0][0] - setup->vmid[0][0];
   setup->etop.dy = setup->vmax[0][1] - setup->vmid[0][1];

   /*
    * Compute triangle's area.  Use 1/area to compute partial
    * derivatives of attributes later.
    *
    * The area will be the same as prim->det, but the sign may be
    * different depending on how the vertices get sorted above.
    *
    * To determine whether the primitive is front or back facing we
    * use the prim->det value because its sign is correct.
    */
   {
      const float area = (setup->emaj.dx * setup->ebot.dy -
			    setup->ebot.dx * setup->emaj.dy);

      setup->oneoverarea = 1.0f / area;

      /*
      debug_printf("%s one-over-area %f  area %f  det %f\n",
                   __FUNCTION__, setup->oneoverarea, area, det );
      */
      if (util_is_inf_or_nan(setup->oneoverarea))
         return FALSE;
   }

   /* We need to know if this is a front or back-facing triangle for:
    *  - the GLSL gl_FrontFacing fragment attribute (bool)
    *  - two-sided stencil test
    */
   setup->facing = 
      ((det > 0.0) ^ 
       (setup->llvmpipe->rasterizer->front_winding == PIPE_WINDING_CW));

   /* Prepare pixel offset for rasterisation:
    *  - pixel center (0.5, 0.5) for GL, or
    *  - assume (0.0, 0.0) for other APIs.
    */
   if (setup->llvmpipe->rasterizer->gl_rasterization_rules) {
      setup->pixel_offset = 0.5f;
   } else {
      setup->pixel_offset = 0.0f;
   }

   return TRUE;
}


/**
 * Compute a0, dadx and dady for a linearly interpolated coefficient,
 * for a triangle.
 */
static void tri_pos_coeff( struct setup_context *setup,
                           uint vertSlot, unsigned i)
{
   float botda = setup->vmid[vertSlot][i] - setup->vmin[vertSlot][i];
   float majda = setup->vmax[vertSlot][i] - setup->vmin[vertSlot][i];
   float a = setup->ebot.dy * majda - botda * setup->emaj.dy;
   float b = setup->emaj.dx * botda - majda * setup->ebot.dx;
   float dadx = a * setup->oneoverarea;
   float dady = b * setup->oneoverarea;

   assert(i <= 3);

   setup->coef.dadx[0][i] = dadx;
   setup->coef.dady[0][i] = dady;

   /* calculate a0 as the value which would be sampled for the
    * fragment at (0,0), taking into account that we want to sample at
    * pixel centers, in other words (pixel_offset, pixel_offset).
    *
    * this is neat but unfortunately not a good way to do things for
    * triangles with very large values of dadx or dady as it will
    * result in the subtraction and re-addition from a0 of a very
    * large number, which means we'll end up loosing a lot of the
    * fractional bits and precision from a0.  the way to fix this is
    * to define a0 as the sample at a pixel center somewhere near vmin
    * instead - i'll switch to this later.
    */
   setup->coef.a0[0][i] = (setup->vmin[vertSlot][i] -
                           (dadx * (setup->vmin[0][0] - setup->pixel_offset) +
                            dady * (setup->vmin[0][1] - setup->pixel_offset)));

   /*
   debug_printf("attr[%d].%c: %f dx:%f dy:%f\n",
                slot, "xyzw"[i],
                setup->coef[slot].a0[i],
                setup->coef[slot].dadx[i],
                setup->coef[slot].dady[i]);
   */
}


/**
 * Compute a0 for a constant-valued coefficient (GL_FLAT shading).
 * The value value comes from vertex[slot][i].
 * The result will be put into setup->coef[slot].a0[i].
 * \param slot  which attribute slot
 * \param i  which component of the slot (0..3)
 */
static void const_pos_coeff( struct setup_context *setup,
                             uint vertSlot, unsigned i)
{
   setup->coef.dadx[0][i] = 0;
   setup->coef.dady[0][i] = 0;

   /* need provoking vertex info!
    */
   setup->coef.a0[0][i] = setup->vprovoke[vertSlot][i];
}


/**
 * Compute a0 for a constant-valued coefficient (GL_FLAT shading).
 * The value value comes from vertex[slot][i].
 * The result will be put into setup->coef[slot].a0[i].
 * \param slot  which attribute slot
 * \param i  which component of the slot (0..3)
 */
static void const_coeff( struct setup_context *setup,
                         unsigned attrib,
                         uint vertSlot)
{
   unsigned i;
   for (i = 0; i < NUM_CHANNELS; ++i) {
      setup->coef.dadx[1 + attrib][i] = 0;
      setup->coef.dady[1 + attrib][i] = 0;

      /* need provoking vertex info!
       */
      setup->coef.a0[1 + attrib][i] = setup->vprovoke[vertSlot][i];
   }
}


/**
 * Compute a0, dadx and dady for a linearly interpolated coefficient,
 * for a triangle.
 */
static void tri_linear_coeff( struct setup_context *setup,
                              unsigned attrib,
                              uint vertSlot)
{
   unsigned i;
   for (i = 0; i < NUM_CHANNELS; ++i) {
      float botda = setup->vmid[vertSlot][i] - setup->vmin[vertSlot][i];
      float majda = setup->vmax[vertSlot][i] - setup->vmin[vertSlot][i];
      float a = setup->ebot.dy * majda - botda * setup->emaj.dy;
      float b = setup->emaj.dx * botda - majda * setup->ebot.dx;
      float dadx = a * setup->oneoverarea;
      float dady = b * setup->oneoverarea;

      assert(i <= 3);

      setup->coef.dadx[1 + attrib][i] = dadx;
      setup->coef.dady[1 + attrib][i] = dady;

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
      setup->coef.a0[1 + attrib][i] = (setup->vmin[vertSlot][i] -
                     (dadx * (setup->vmin[0][0] - setup->pixel_offset) +
                      dady * (setup->vmin[0][1] - setup->pixel_offset)));

      /*
      debug_printf("attr[%d].%c: %f dx:%f dy:%f\n",
                   slot, "xyzw"[i],
                   setup->coef[slot].a0[i],
                   setup->coef[slot].dadx[i],
                   setup->coef[slot].dady[i]);
      */
   }
}


/**
 * Compute a0, dadx and dady for a perspective-corrected interpolant,
 * for a triangle.
 * We basically multiply the vertex value by 1/w before computing
 * the plane coefficients (a0, dadx, dady).
 * Later, when we compute the value at a particular fragment position we'll
 * divide the interpolated value by the interpolated W at that fragment.
 */
static void tri_persp_coeff( struct setup_context *setup,
                             unsigned attrib,
                             uint vertSlot)
{
   unsigned i;
   for (i = 0; i < NUM_CHANNELS; ++i) {
      /* premultiply by 1/w  (v[0][3] is always W):
       */
      float mina = setup->vmin[vertSlot][i] * setup->vmin[0][3];
      float mida = setup->vmid[vertSlot][i] * setup->vmid[0][3];
      float maxa = setup->vmax[vertSlot][i] * setup->vmax[0][3];
      float botda = mida - mina;
      float majda = maxa - mina;
      float a = setup->ebot.dy * majda - botda * setup->emaj.dy;
      float b = setup->emaj.dx * botda - majda * setup->ebot.dx;
      float dadx = a * setup->oneoverarea;
      float dady = b * setup->oneoverarea;

      /*
      debug_printf("tri persp %d,%d: %f %f %f\n", vertSlot, i,
                   setup->vmin[vertSlot][i],
                   setup->vmid[vertSlot][i],
                   setup->vmax[vertSlot][i]
             );
      */
      assert(i <= 3);

      setup->coef.dadx[1 + attrib][i] = dadx;
      setup->coef.dady[1 + attrib][i] = dady;
      setup->coef.a0[1 + attrib][i] = (mina -
                     (dadx * (setup->vmin[0][0] - setup->pixel_offset) +
                      dady * (setup->vmin[0][1] - setup->pixel_offset)));
   }
}


/**
 * Special coefficient setup for gl_FragCoord.
 * X and Y are trivial, though Y has to be inverted for OpenGL.
 * Z and W are copied from posCoef which should have already been computed.
 * We could do a bit less work if we'd examine gl_FragCoord's swizzle mask.
 */
static void
setup_fragcoord_coeff(struct setup_context *setup, uint slot)
{
   /*X*/
   setup->coef.a0[1 + slot][0] = 0;
   setup->coef.dadx[1 + slot][0] = 1.0;
   setup->coef.dady[1 + slot][0] = 0.0;
   /*Y*/
   setup->coef.a0[1 + slot][1] = 0.0;
   setup->coef.dadx[1 + slot][1] = 0.0;
   setup->coef.dady[1 + slot][1] = 1.0;
   /*Z*/
   setup->coef.a0[1 + slot][2] = setup->coef.a0[0][2];
   setup->coef.dadx[1 + slot][2] = setup->coef.dadx[0][2];
   setup->coef.dady[1 + slot][2] = setup->coef.dady[0][2];
   /*W*/
   setup->coef.a0[1 + slot][3] = setup->coef.a0[0][3];
   setup->coef.dadx[1 + slot][3] = setup->coef.dadx[0][3];
   setup->coef.dady[1 + slot][3] = setup->coef.dady[0][3];
}



/**
 * Compute the setup->coef[] array dadx, dady, a0 values.
 * Must be called after setup->vmin,vmid,vmax,vprovoke are initialized.
 */
static void setup_tri_coefficients( struct setup_context *setup )
{
   struct llvmpipe_context *llvmpipe = setup->llvmpipe;
   const struct lp_fragment_shader *lpfs = llvmpipe->fs;
   const struct vertex_info *vinfo = llvmpipe_get_vertex_info(llvmpipe);
   uint fragSlot;

   /* z and w are done by linear interpolation:
    */
   tri_pos_coeff(setup, 0, 2);
   tri_pos_coeff(setup, 0, 3);

   /* setup interpolation for all the remaining attributes:
    */
   for (fragSlot = 0; fragSlot < lpfs->info.num_inputs; fragSlot++) {
      const uint vertSlot = vinfo->attrib[fragSlot].src_index;

      switch (vinfo->attrib[fragSlot].interp_mode) {
      case INTERP_CONSTANT:
         const_coeff(setup, fragSlot, vertSlot);
         break;
      case INTERP_LINEAR:
         tri_linear_coeff(setup, fragSlot, vertSlot);
         break;
      case INTERP_PERSPECTIVE:
         tri_persp_coeff(setup, fragSlot, vertSlot);
         break;
      case INTERP_POS:
         setup_fragcoord_coeff(setup, fragSlot);
         break;
      default:
         assert(0);
      }

      if (lpfs->info.input_semantic_name[fragSlot] == TGSI_SEMANTIC_FACE) {
         setup->coef.a0[1 + fragSlot][0] = 1.0f - setup->facing;
         setup->coef.dadx[1 + fragSlot][0] = 0.0;
         setup->coef.dady[1 + fragSlot][0] = 0.0;
      }
   }
}



static void setup_tri_edges( struct setup_context *setup )
{
   float vmin_x = setup->vmin[0][0] + setup->pixel_offset;
   float vmid_x = setup->vmid[0][0] + setup->pixel_offset;

   float vmin_y = setup->vmin[0][1] - setup->pixel_offset;
   float vmid_y = setup->vmid[0][1] - setup->pixel_offset;
   float vmax_y = setup->vmax[0][1] - setup->pixel_offset;

   setup->emaj.sy = ceilf(vmin_y);
   setup->emaj.lines = (int) ceilf(vmax_y - setup->emaj.sy);
   setup->emaj.dxdy = setup->emaj.dx / setup->emaj.dy;
   setup->emaj.sx = vmin_x + (setup->emaj.sy - vmin_y) * setup->emaj.dxdy;

   setup->etop.sy = ceilf(vmid_y);
   setup->etop.lines = (int) ceilf(vmax_y - setup->etop.sy);
   setup->etop.dxdy = setup->etop.dx / setup->etop.dy;
   setup->etop.sx = vmid_x + (setup->etop.sy - vmid_y) * setup->etop.dxdy;

   setup->ebot.sy = ceilf(vmin_y);
   setup->ebot.lines = (int) ceilf(vmid_y - setup->ebot.sy);
   setup->ebot.dxdy = setup->ebot.dx / setup->ebot.dy;
   setup->ebot.sx = vmin_x + (setup->ebot.sy - vmin_y) * setup->ebot.dxdy;
}


/**
 * Render the upper or lower half of a triangle.
 * Scissoring/cliprect is applied here too.
 */
static void subtriangle( struct setup_context *setup,
			 struct edge *eleft,
			 struct edge *eright,
			 unsigned lines )
{
   const struct pipe_scissor_state *cliprect = &setup->llvmpipe->cliprect;
   const int minx = (int) cliprect->minx;
   const int maxx = (int) cliprect->maxx;
   const int miny = (int) cliprect->miny;
   const int maxy = (int) cliprect->maxy;
   int y, start_y, finish_y;
   int sy = (int)eleft->sy;

   assert((int)eleft->sy == (int) eright->sy);

   /* clip top/bottom */
   start_y = sy;
   if (start_y < miny)
      start_y = miny;

   finish_y = sy + lines;
   if (finish_y > maxy)
      finish_y = maxy;

   start_y -= sy;
   finish_y -= sy;

   /*
   debug_printf("%s %d %d\n", __FUNCTION__, start_y, finish_y);
   */

   for (y = start_y; y < finish_y; y++) {

      /* avoid accumulating adds as floats don't have the precision to
       * accurately iterate large triangle edges that way.  luckily we
       * can just multiply these days.
       *
       * this is all drowned out by the attribute interpolation anyway.
       */
      int left = (int)(eleft->sx + y * eleft->dxdy);
      int right = (int)(eright->sx + y * eright->dxdy);

      /* clip left/right */
      if (left < minx)
         left = minx;
      if (right > maxx)
         right = maxx;

      if (left < right) {
         int _y = sy + y;
         if (block(_y) != setup->span.y) {
            flush_spans(setup);
            setup->span.y = block(_y);
         }

         setup->span.left[_y&1] = left;
         setup->span.right[_y&1] = right;
      }
   }


   /* save the values so that emaj can be restarted:
    */
   eleft->sx += lines * eleft->dxdy;
   eright->sx += lines * eright->dxdy;
   eleft->sy += lines;
   eright->sy += lines;
}


/**
 * Recalculate prim's determinant.  This is needed as we don't have
 * get this information through the vbuf_render interface & we must
 * calculate it here.
 */
static float
calc_det( const float (*v0)[4],
          const float (*v1)[4],
          const float (*v2)[4] )
{
   /* edge vectors e = v0 - v2, f = v1 - v2 */
   const float ex = v0[0][0] - v2[0][0];
   const float ey = v0[0][1] - v2[0][1];
   const float fx = v1[0][0] - v2[0][0];
   const float fy = v1[0][1] - v2[0][1];

   /* det = cross(e,f).z */
   return ex * fy - ey * fx;
}


/**
 * Do setup for triangle rasterization, then render the triangle.
 */
void llvmpipe_setup_tri( struct setup_context *setup,
                const float (*v0)[4],
                const float (*v1)[4],
                const float (*v2)[4] )
{
   float det;

#if DEBUG_VERTS
   debug_printf("Setup triangle:\n");
   print_vertex(setup, v0);
   print_vertex(setup, v1);
   print_vertex(setup, v2);
#endif

   if (setup->llvmpipe->no_rast)
      return;
   
   det = calc_det(v0, v1, v2);
   /*
   debug_printf("%s\n", __FUNCTION__ );
   */

#if DEBUG_FRAGS
   setup->numFragsEmitted = 0;
   setup->numFragsWritten = 0;
#endif

   if (cull_tri( setup, det ))
      return;

   if (!setup_sort_vertices( setup, det, v0, v1, v2 ))
      return;
   setup_tri_coefficients( setup );
   setup_tri_edges( setup );

   assert(setup->llvmpipe->reduced_prim == PIPE_PRIM_TRIANGLES);

   setup->span.y = 0;
   setup->span.right[0] = 0;
   setup->span.right[1] = 0;
   /*   setup->span.z_mode = tri_z_mode( setup->ctx ); */

   /*   init_constant_attribs( setup ); */

   if (setup->oneoverarea < 0.0) {
      /* emaj on left:
       */
      subtriangle( setup, &setup->emaj, &setup->ebot, setup->ebot.lines );
      subtriangle( setup, &setup->emaj, &setup->etop, setup->etop.lines );
   }
   else {
      /* emaj on right:
       */
      subtriangle( setup, &setup->ebot, &setup->emaj, setup->ebot.lines );
      subtriangle( setup, &setup->etop, &setup->emaj, setup->etop.lines );
   }

   flush_spans( setup );

#if DEBUG_FRAGS
   printf("Tri: %u frags emitted, %u written\n",
          setup->numFragsEmitted,
          setup->numFragsWritten);
#endif
}



/**
 * Compute a0, dadx and dady for a linearly interpolated coefficient,
 * for a line.
 */
static void
linear_pos_coeff(struct setup_context *setup,
                 uint vertSlot, uint i)
{
   const float da = setup->vmax[vertSlot][i] - setup->vmin[vertSlot][i];
   const float dadx = da * setup->emaj.dx * setup->oneoverarea;
   const float dady = da * setup->emaj.dy * setup->oneoverarea;
   setup->coef.dadx[0][i] = dadx;
   setup->coef.dady[0][i] = dady;
   setup->coef.a0[0][i] = (setup->vmin[vertSlot][i] -
                           (dadx * (setup->vmin[0][0] - setup->pixel_offset) +
                            dady * (setup->vmin[0][1] - setup->pixel_offset)));
}


/**
 * Compute a0, dadx and dady for a linearly interpolated coefficient,
 * for a line.
 */
static void
line_linear_coeff(struct setup_context *setup,
                  unsigned attrib,
                  uint vertSlot)
{
   unsigned i;
   for (i = 0; i < NUM_CHANNELS; ++i) {
      const float da = setup->vmax[vertSlot][i] - setup->vmin[vertSlot][i];
      const float dadx = da * setup->emaj.dx * setup->oneoverarea;
      const float dady = da * setup->emaj.dy * setup->oneoverarea;
      setup->coef.dadx[1 + attrib][i] = dadx;
      setup->coef.dady[1 + attrib][i] = dady;
      setup->coef.a0[1 + attrib][i] = (setup->vmin[vertSlot][i] -
                     (dadx * (setup->vmin[0][0] - setup->pixel_offset) +
                      dady * (setup->vmin[0][1] - setup->pixel_offset)));
   }
}


/**
 * Compute a0, dadx and dady for a perspective-corrected interpolant,
 * for a line.
 */
static void
line_persp_coeff(struct setup_context *setup,
                 unsigned attrib,
                 uint vertSlot)
{
   unsigned i;
   for (i = 0; i < NUM_CHANNELS; ++i) {
      /* XXX double-check/verify this arithmetic */
      const float a0 = setup->vmin[vertSlot][i] * setup->vmin[0][3];
      const float a1 = setup->vmax[vertSlot][i] * setup->vmax[0][3];
      const float da = a1 - a0;
      const float dadx = da * setup->emaj.dx * setup->oneoverarea;
      const float dady = da * setup->emaj.dy * setup->oneoverarea;
      setup->coef.dadx[1 + attrib][i] = dadx;
      setup->coef.dady[1 + attrib][i] = dady;
      setup->coef.a0[1 + attrib][i] = (setup->vmin[vertSlot][i] -
                     (dadx * (setup->vmin[0][0] - setup->pixel_offset) +
                      dady * (setup->vmin[0][1] - setup->pixel_offset)));
   }
}


/**
 * Compute the setup->coef[] array dadx, dady, a0 values.
 * Must be called after setup->vmin,vmax are initialized.
 */
static INLINE boolean
setup_line_coefficients(struct setup_context *setup,
                        const float (*v0)[4],
                        const float (*v1)[4])
{
   struct llvmpipe_context *llvmpipe = setup->llvmpipe;
   const struct lp_fragment_shader *lpfs = llvmpipe->fs;
   const struct vertex_info *vinfo = llvmpipe_get_vertex_info(llvmpipe);
   uint fragSlot;
   float area;

   /* use setup->vmin, vmax to point to vertices */
   if (llvmpipe->rasterizer->flatshade_first)
      setup->vprovoke = v0;
   else
      setup->vprovoke = v1;
   setup->vmin = v0;
   setup->vmax = v1;

   setup->emaj.dx = setup->vmax[0][0] - setup->vmin[0][0];
   setup->emaj.dy = setup->vmax[0][1] - setup->vmin[0][1];

   /* NOTE: this is not really area but something proportional to it */
   area = setup->emaj.dx * setup->emaj.dx + setup->emaj.dy * setup->emaj.dy;
   if (area == 0.0f || util_is_inf_or_nan(area))
      return FALSE;
   setup->oneoverarea = 1.0f / area;

   /* z and w are done by linear interpolation:
    */
   linear_pos_coeff(setup, 0, 2);
   linear_pos_coeff(setup, 0, 3);

   /* setup interpolation for all the remaining attributes:
    */
   for (fragSlot = 0; fragSlot < lpfs->info.num_inputs; fragSlot++) {
      const uint vertSlot = vinfo->attrib[fragSlot].src_index;

      switch (vinfo->attrib[fragSlot].interp_mode) {
      case INTERP_CONSTANT:
         const_coeff(setup, fragSlot, vertSlot);
         break;
      case INTERP_LINEAR:
         line_linear_coeff(setup, fragSlot, vertSlot);
         break;
      case INTERP_PERSPECTIVE:
         line_persp_coeff(setup, fragSlot, vertSlot);
         break;
      case INTERP_POS:
         setup_fragcoord_coeff(setup, fragSlot);
         break;
      default:
         assert(0);
      }

      if (lpfs->info.input_semantic_name[fragSlot] == TGSI_SEMANTIC_FACE) {
         setup->coef.a0[1 + fragSlot][0] = 1.0f - setup->facing;
         setup->coef.dadx[1 + fragSlot][0] = 0.0;
         setup->coef.dady[1 + fragSlot][0] = 0.0;
      }
   }
   return TRUE;
}


/**
 * Plot a pixel in a line segment.
 */
static INLINE void
plot(struct setup_context *setup, int x, int y)
{
   const int iy = y & 1;
   const int ix = x & 1;
   const int quadX = x - ix;
   const int quadY = y - iy;
   const int mask = (1 << ix) << (2 * iy);

   if (quadX != setup->quad[0].input.x0 ||
       quadY != setup->quad[0].input.y0)
   {
      /* flush prev quad, start new quad */

      if (setup->quad[0].input.x0 != -1)
         clip_emit_quad( setup, &setup->quad[0] );

      setup->quad[0].input.x0 = quadX;
      setup->quad[0].input.y0 = quadY;
      setup->quad[0].inout.mask = 0x0;
   }

   setup->quad[0].inout.mask |= mask;
}


/**
 * Do setup for line rasterization, then render the line.
 * Single-pixel width, no stipple, etc.  We rely on the 'draw' module
 * to handle stippling and wide lines.
 */
void
llvmpipe_setup_line(struct setup_context *setup,
           const float (*v0)[4],
           const float (*v1)[4])
{
   int x0 = (int) v0[0][0];
   int x1 = (int) v1[0][0];
   int y0 = (int) v0[0][1];
   int y1 = (int) v1[0][1];
   int dx = x1 - x0;
   int dy = y1 - y0;
   int xstep, ystep;

#if DEBUG_VERTS
   debug_printf("Setup line:\n");
   print_vertex(setup, v0);
   print_vertex(setup, v1);
#endif

   if (setup->llvmpipe->no_rast)
      return;

   if (dx == 0 && dy == 0)
      return;

   if (!setup_line_coefficients(setup, v0, v1))
      return;

   assert(v0[0][0] < 1.0e9);
   assert(v0[0][1] < 1.0e9);
   assert(v1[0][0] < 1.0e9);
   assert(v1[0][1] < 1.0e9);

   if (dx < 0) {
      dx = -dx;   /* make positive */
      xstep = -1;
   }
   else {
      xstep = 1;
   }

   if (dy < 0) {
      dy = -dy;   /* make positive */
      ystep = -1;
   }
   else {
      ystep = 1;
   }

   assert(dx >= 0);
   assert(dy >= 0);
   assert(setup->llvmpipe->reduced_prim == PIPE_PRIM_LINES);

   setup->quad[0].input.x0 = setup->quad[0].input.y0 = -1;
   setup->quad[0].inout.mask = 0x0;

   /* XXX temporary: set coverage to 1.0 so the line appears
    * if AA mode happens to be enabled.
    */
   setup->quad[0].input.coverage[0] =
   setup->quad[0].input.coverage[1] =
   setup->quad[0].input.coverage[2] =
   setup->quad[0].input.coverage[3] = 1.0;

   if (dx > dy) {
      /*** X-major line ***/
      int i;
      const int errorInc = dy + dy;
      int error = errorInc - dx;
      const int errorDec = error - dx;

      for (i = 0; i < dx; i++) {
         plot(setup, x0, y0);

         x0 += xstep;
         if (error < 0) {
            error += errorInc;
         }
         else {
            error += errorDec;
            y0 += ystep;
         }
      }
   }
   else {
      /*** Y-major line ***/
      int i;
      const int errorInc = dx + dx;
      int error = errorInc - dy;
      const int errorDec = error - dy;

      for (i = 0; i < dy; i++) {
         plot(setup, x0, y0);

         y0 += ystep;
         if (error < 0) {
            error += errorInc;
         }
         else {
            error += errorDec;
            x0 += xstep;
         }
      }
   }

   /* draw final quad */
   if (setup->quad[0].inout.mask) {
      clip_emit_quad( setup, &setup->quad[0] );
   }
}


static void
point_persp_coeff(struct setup_context *setup,
                  const float (*vert)[4],
                  unsigned attrib,
                  uint vertSlot)
{
   unsigned i;
   for(i = 0; i < NUM_CHANNELS; ++i) {
      setup->coef.dadx[1 + attrib][i] = 0.0F;
      setup->coef.dady[1 + attrib][i] = 0.0F;
      setup->coef.a0[1 + attrib][i] = vert[vertSlot][i] * vert[0][3];
   }
}


/**
 * Do setup for point rasterization, then render the point.
 * Round or square points...
 * XXX could optimize a lot for 1-pixel points.
 */
void
llvmpipe_setup_point( struct setup_context *setup,
             const float (*v0)[4] )
{
   struct llvmpipe_context *llvmpipe = setup->llvmpipe;
   const struct lp_fragment_shader *lpfs = llvmpipe->fs;
   const int sizeAttr = setup->llvmpipe->psize_slot;
   const float size
      = sizeAttr > 0 ? v0[sizeAttr][0]
      : setup->llvmpipe->rasterizer->point_size;
   const float halfSize = 0.5F * size;
   const boolean round = (boolean) setup->llvmpipe->rasterizer->point_smooth;
   const float x = v0[0][0];  /* Note: data[0] is always position */
   const float y = v0[0][1];
   const struct vertex_info *vinfo = llvmpipe_get_vertex_info(llvmpipe);
   uint fragSlot;

#if DEBUG_VERTS
   debug_printf("Setup point:\n");
   print_vertex(setup, v0);
#endif

   if (llvmpipe->no_rast)
      return;

   assert(setup->llvmpipe->reduced_prim == PIPE_PRIM_POINTS);

   /* For points, all interpolants are constant-valued.
    * However, for point sprites, we'll need to setup texcoords appropriately.
    * XXX: which coefficients are the texcoords???
    * We may do point sprites as textured quads...
    *
    * KW: We don't know which coefficients are texcoords - ultimately
    * the choice of what interpolation mode to use for each attribute
    * should be determined by the fragment program, using
    * per-attribute declaration statements that include interpolation
    * mode as a parameter.  So either the fragment program will have
    * to be adjusted for pointsprite vs normal point behaviour, or
    * otherwise a special interpolation mode will have to be defined
    * which matches the required behaviour for point sprites.  But -
    * the latter is not a feature of normal hardware, and as such
    * probably should be ruled out on that basis.
    */
   setup->vprovoke = v0;

   /* setup Z, W */
   const_pos_coeff(setup, 0, 2);
   const_pos_coeff(setup, 0, 3);

   for (fragSlot = 0; fragSlot < lpfs->info.num_inputs; fragSlot++) {
      const uint vertSlot = vinfo->attrib[fragSlot].src_index;

      switch (vinfo->attrib[fragSlot].interp_mode) {
      case INTERP_CONSTANT:
         /* fall-through */
      case INTERP_LINEAR:
         const_coeff(setup, fragSlot, vertSlot);
         break;
      case INTERP_PERSPECTIVE:
         point_persp_coeff(setup, setup->vprovoke, fragSlot, vertSlot);
         break;
      case INTERP_POS:
         setup_fragcoord_coeff(setup, fragSlot);
         break;
      default:
         assert(0);
      }

      if (lpfs->info.input_semantic_name[fragSlot] == TGSI_SEMANTIC_FACE) {
         setup->coef.a0[1 + fragSlot][0] = 1.0f - setup->facing;
         setup->coef.dadx[1 + fragSlot][0] = 0.0;
         setup->coef.dady[1 + fragSlot][0] = 0.0;
      }
   }


   if (halfSize <= 0.5 && !round) {
      /* special case for 1-pixel points */
      const int ix = ((int) x) & 1;
      const int iy = ((int) y) & 1;
      setup->quad[0].input.x0 = (int) x - ix;
      setup->quad[0].input.y0 = (int) y - iy;
      setup->quad[0].inout.mask = (1 << ix) << (2 * iy);
      clip_emit_quad( setup, &setup->quad[0] );
   }
   else {
      if (round) {
         /* rounded points */
         const int ixmin = block((int) (x - halfSize));
         const int ixmax = block((int) (x + halfSize));
         const int iymin = block((int) (y - halfSize));
         const int iymax = block((int) (y + halfSize));
         const float rmin = halfSize - 0.7071F;  /* 0.7071 = sqrt(2)/2 */
         const float rmax = halfSize + 0.7071F;
         const float rmin2 = MAX2(0.0F, rmin * rmin);
         const float rmax2 = rmax * rmax;
         const float cscale = 1.0F / (rmax2 - rmin2);
         int ix, iy;

         for (iy = iymin; iy <= iymax; iy += 2) {
            for (ix = ixmin; ix <= ixmax; ix += 2) {
               float dx, dy, dist2, cover;

               setup->quad[0].inout.mask = 0x0;

               dx = (ix + 0.5f) - x;
               dy = (iy + 0.5f) - y;
               dist2 = dx * dx + dy * dy;
               if (dist2 <= rmax2) {
                  cover = 1.0F - (dist2 - rmin2) * cscale;
                  setup->quad[0].input.coverage[QUAD_TOP_LEFT] = MIN2(cover, 1.0f);
                  setup->quad[0].inout.mask |= MASK_TOP_LEFT;
               }

               dx = (ix + 1.5f) - x;
               dy = (iy + 0.5f) - y;
               dist2 = dx * dx + dy * dy;
               if (dist2 <= rmax2) {
                  cover = 1.0F - (dist2 - rmin2) * cscale;
                  setup->quad[0].input.coverage[QUAD_TOP_RIGHT] = MIN2(cover, 1.0f);
                  setup->quad[0].inout.mask |= MASK_TOP_RIGHT;
               }

               dx = (ix + 0.5f) - x;
               dy = (iy + 1.5f) - y;
               dist2 = dx * dx + dy * dy;
               if (dist2 <= rmax2) {
                  cover = 1.0F - (dist2 - rmin2) * cscale;
                  setup->quad[0].input.coverage[QUAD_BOTTOM_LEFT] = MIN2(cover, 1.0f);
                  setup->quad[0].inout.mask |= MASK_BOTTOM_LEFT;
               }

               dx = (ix + 1.5f) - x;
               dy = (iy + 1.5f) - y;
               dist2 = dx * dx + dy * dy;
               if (dist2 <= rmax2) {
                  cover = 1.0F - (dist2 - rmin2) * cscale;
                  setup->quad[0].input.coverage[QUAD_BOTTOM_RIGHT] = MIN2(cover, 1.0f);
                  setup->quad[0].inout.mask |= MASK_BOTTOM_RIGHT;
               }

               if (setup->quad[0].inout.mask) {
                  setup->quad[0].input.x0 = ix;
                  setup->quad[0].input.y0 = iy;
                  clip_emit_quad( setup, &setup->quad[0] );
               }
            }
         }
      }
      else {
         /* square points */
         const int xmin = (int) (x + 0.75 - halfSize);
         const int ymin = (int) (y + 0.25 - halfSize);
         const int xmax = xmin + (int) size;
         const int ymax = ymin + (int) size;
         /* XXX could apply scissor to xmin,ymin,xmax,ymax now */
         const int ixmin = block(xmin);
         const int ixmax = block(xmax - 1);
         const int iymin = block(ymin);
         const int iymax = block(ymax - 1);
         int ix, iy;

         /*
         debug_printf("(%f, %f) -> X:%d..%d Y:%d..%d\n", x, y, xmin, xmax,ymin,ymax);
         */
         for (iy = iymin; iy <= iymax; iy += 2) {
            uint rowMask = 0xf;
            if (iy < ymin) {
               /* above the top edge */
               rowMask &= (MASK_BOTTOM_LEFT | MASK_BOTTOM_RIGHT);
            }
            if (iy + 1 >= ymax) {
               /* below the bottom edge */
               rowMask &= (MASK_TOP_LEFT | MASK_TOP_RIGHT);
            }

            for (ix = ixmin; ix <= ixmax; ix += 2) {
               uint mask = rowMask;

               if (ix < xmin) {
                  /* fragment is past left edge of point, turn off left bits */
                  mask &= (MASK_BOTTOM_RIGHT | MASK_TOP_RIGHT);
               }
               if (ix + 1 >= xmax) {
                  /* past the right edge */
                  mask &= (MASK_BOTTOM_LEFT | MASK_TOP_LEFT);
               }

               setup->quad[0].inout.mask = mask;
               setup->quad[0].input.x0 = ix;
               setup->quad[0].input.y0 = iy;
               clip_emit_quad( setup, &setup->quad[0] );
            }
         }
      }
   }
}

void llvmpipe_setup_prepare( struct setup_context *setup )
{
   struct llvmpipe_context *lp = setup->llvmpipe;

   if (lp->dirty) {
      llvmpipe_update_derived(lp);
   }

   if (lp->reduced_api_prim == PIPE_PRIM_TRIANGLES &&
       lp->rasterizer->fill_cw == PIPE_POLYGON_MODE_FILL &&
       lp->rasterizer->fill_ccw == PIPE_POLYGON_MODE_FILL) {
      /* we'll do culling */
      setup->winding = lp->rasterizer->cull_mode;
   }
   else {
      /* 'draw' will do culling */
      setup->winding = PIPE_WINDING_NONE;
   }
}



void llvmpipe_setup_destroy_context( struct setup_context *setup )
{
   align_free( setup );
}


/**
 * Create a new primitive setup/render stage.
 */
struct setup_context *llvmpipe_setup_create_context( struct llvmpipe_context *llvmpipe )
{
   struct setup_context *setup;
   unsigned i;

   setup = align_malloc(sizeof(struct setup_context), 16);
   if (!setup)
      return NULL;

   memset(setup, 0, sizeof *setup);
   setup->llvmpipe = llvmpipe;

   for (i = 0; i < MAX_QUADS; i++) {
      setup->quad[i].coef = &setup->coef;
   }

   setup->span.left[0] = 1000000;     /* greater than right[0] */
   setup->span.left[1] = 1000000;     /* greater than right[1] */

   return setup;
}

