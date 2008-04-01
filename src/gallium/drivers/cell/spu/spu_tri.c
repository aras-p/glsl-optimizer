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
 * Triangle rendering within a tile.
 */

#include <transpose_matrix4x4.h>
#include "pipe/p_compiler.h"
#include "pipe/p_format.h"
#include "pipe/p_util.h"
#include "spu_colorpack.h"
#include "spu_main.h"
#include "spu_texture.h"
#include "spu_tile.h"
#include "spu_tri.h"
#include "spu_per_fragment_op.h"


/** Masks are uint[4] vectors with each element being 0 or 0xffffffff */
typedef vector unsigned int mask_t;

typedef union
{
   vector float v;
   float f[4];
} float4;


/**
 * Simplified types taken from other parts of Gallium
 */
struct vertex_header {
   vector float data[1];
};



/* XXX fix this */
#undef CEILF
#define CEILF(X) ((float) (int) ((X) + 0.99999))


#define QUAD_TOP_LEFT     0
#define QUAD_TOP_RIGHT    1
#define QUAD_BOTTOM_LEFT  2
#define QUAD_BOTTOM_RIGHT 3
#define MASK_TOP_LEFT     (1 << QUAD_TOP_LEFT)
#define MASK_TOP_RIGHT    (1 << QUAD_TOP_RIGHT)
#define MASK_BOTTOM_LEFT  (1 << QUAD_BOTTOM_LEFT)
#define MASK_BOTTOM_RIGHT (1 << QUAD_BOTTOM_RIGHT)
#define MASK_ALL          0xf


#define DEBUG_VERTS 0

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


struct interp_coef
{
   float4 a0;
   float4 dadx;
   float4 dady;
};


/**
 * Triangle setup info (derived from draw_stage).
 * Also used for line drawing (taking some liberties).
 */
struct setup_stage {

   /* Vertices are just an array of floats making up each attribute in
    * turn.  Currently fixed at 4 floats, but should change in time.
    * Codegen will help cope with this.
    */
   const struct vertex_header *vmax;
   const struct vertex_header *vmid;
   const struct vertex_header *vmin;
   const struct vertex_header *vprovoke;

   struct edge ebot;
   struct edge etop;
   struct edge emaj;

   float oneoverarea;

   uint tx, ty;

   int cliprect_minx, cliprect_maxx, cliprect_miny, cliprect_maxy;

#if 0
   struct tgsi_interp_coef coef[PIPE_MAX_SHADER_INPUTS];
#else
   struct interp_coef coef[PIPE_MAX_SHADER_INPUTS];
#endif

#if 0
   struct quad_header quad; 
#endif

   struct {
      int left[2];   /**< [0] = row0, [1] = row1 */
      int right[2];
      int y;
      unsigned y_flags;
      unsigned mask;     /**< mask of MASK_BOTTOM/TOP_LEFT/RIGHT bits */
   } span;
};



static struct setup_stage setup;




#if 0
/**
 * Basically a cast wrapper.
 */
static INLINE struct setup_stage *setup_stage( struct draw_stage *stage )
{
   return (struct setup_stage *)stage;
}
#endif

#if 0
/**
 * Clip setup.quad against the scissor/surface bounds.
 */
static INLINE void
quad_clip(struct setup_stage *setup)
{
   const struct pipe_scissor_state *cliprect = &setup.softpipe->cliprect;
   const int minx = (int) cliprect->minx;
   const int maxx = (int) cliprect->maxx;
   const int miny = (int) cliprect->miny;
   const int maxy = (int) cliprect->maxy;

   if (setup.quad.x0 >= maxx ||
       setup.quad.y0 >= maxy ||
       setup.quad.x0 + 1 < minx ||
       setup.quad.y0 + 1 < miny) {
      /* totally clipped */
      setup.quad.mask = 0x0;
      return;
   }
   if (setup.quad.x0 < minx)
      setup.quad.mask &= (MASK_BOTTOM_RIGHT | MASK_TOP_RIGHT);
   if (setup.quad.y0 < miny)
      setup.quad.mask &= (MASK_BOTTOM_LEFT | MASK_BOTTOM_RIGHT);
   if (setup.quad.x0 == maxx - 1)
      setup.quad.mask &= (MASK_BOTTOM_LEFT | MASK_TOP_LEFT);
   if (setup.quad.y0 == maxy - 1)
      setup.quad.mask &= (MASK_TOP_LEFT | MASK_TOP_RIGHT);
}
#endif

#if 0
/**
 * Emit a quad (pass to next stage) with clipping.
 */
static INLINE void
clip_emit_quad(struct setup_stage *setup)
{
   quad_clip(setup);
   if (setup.quad.mask) {
      struct softpipe_context *sp = setup.softpipe;
      sp->quad.first->run(sp->quad.first, &setup.quad);
   }
}
#endif

/**
 * Evaluate attribute coefficients (plane equations) to compute
 * attribute values for the four fragments in a quad.
 * Eg: four colors will be compute.
 */
static INLINE void
eval_coeff(uint slot, float x, float y, vector float result[4])
{
   switch (spu.vertex_info.interp_mode[slot]) {
   case INTERP_CONSTANT:
      result[QUAD_TOP_LEFT] =
      result[QUAD_TOP_RIGHT] =
      result[QUAD_BOTTOM_LEFT] =
      result[QUAD_BOTTOM_RIGHT] = setup.coef[slot].a0.v;
      break;

   case INTERP_LINEAR:
      /* fall-through, for now */
   default:
      {
         register vector float dadx = setup.coef[slot].dadx.v;
         register vector float dady = setup.coef[slot].dady.v;
         register vector float topLeft
            = spu_add(setup.coef[slot].a0.v,
                      spu_add(spu_mul(spu_splats(x), dadx),
                              spu_mul(spu_splats(y), dady)));

         result[QUAD_TOP_LEFT] = topLeft;
         result[QUAD_TOP_RIGHT] = spu_add(topLeft, dadx);
         result[QUAD_BOTTOM_LEFT] = spu_add(topLeft, dady);
         result[QUAD_BOTTOM_RIGHT] = spu_add(spu_add(topLeft, dadx), dady);
      }
   }
}


static INLINE vector float
eval_z(float x, float y)
{
   const uint slot = 0;
   const float dzdx = setup.coef[slot].dadx.f[2];
   const float dzdy = setup.coef[slot].dady.f[2];
   const float topLeft = setup.coef[slot].a0.f[2] + x * dzdx + y * dzdy;
   const vector float topLeftv = spu_splats(topLeft);
   const vector float derivs = (vector float) { 0.0, dzdx, dzdy, dzdx + dzdy };
   return spu_add(topLeftv, derivs);
}


static INLINE mask_t
do_depth_test(int x, int y, mask_t quadmask)
{
   float4 zvals;
   mask_t mask;

   if (spu.fb.depth_format == PIPE_FORMAT_NONE)
      return quadmask;

   zvals.v = eval_z((float) x, (float) y);

   mask = (mask_t) spu_do_depth_stencil(x - setup.cliprect_minx,
					y - setup.cliprect_miny,
					(qword) quadmask, 
					(qword) zvals.v,
					(qword) spu_splats((unsigned char) 0x0ffu),
					(qword) spu_splats((unsigned int) 0x01u));

   if (spu_extract(spu_orx(mask), 0))
      spu.cur_ztile_status = TILE_STATUS_DIRTY;

   return mask;
}


/**
 * Emit a quad (pass to next stage).  No clipping is done.
 * Note: about 1/5 to 1/7 of the time, mask is zero and this function
 * should be skipped.  But adding the test for that slows things down
 * overall.
 */
static INLINE void
emit_quad( int x, int y, mask_t mask )
{
#if 0
   struct softpipe_context *sp = setup.softpipe;
   setup.quad.x0 = x;
   setup.quad.y0 = y;
   setup.quad.mask = mask;
   sp->quad.first->run(sp->quad.first, &setup.quad);
#else

   if (spu.read_depth) {
      mask = do_depth_test(x, y, mask);
   }

   /* If any bits in mask are set... */
   if (spu_extract(spu_orx(mask), 0)) {
      const int ix = x - setup.cliprect_minx;
      const int iy = y - setup.cliprect_miny;
      vector float colors[4];

      spu.cur_ctile_status = TILE_STATUS_DIRTY;

      if (spu.texture[0].start) {
         /* texture mapping */
         const uint unit = 0;
         vector float texcoords[4];
         eval_coeff(2, (float) x, (float) y, texcoords);

         if (spu_extract(mask, 0))
            colors[0] = spu.sample_texture(unit, texcoords[0]);
         if (spu_extract(mask, 1))
            colors[1] = spu.sample_texture(unit, texcoords[1]);
         if (spu_extract(mask, 2))
            colors[2] = spu.sample_texture(unit, texcoords[2]);
         if (spu_extract(mask, 3))
            colors[3] = spu.sample_texture(unit, texcoords[3]);


         if (spu.texture[1].start) {
            /* multi-texture mapping */
            const uint unit = 1;
            vector float colors1[4];

            eval_coeff(3, (float) x, (float) y, texcoords);

            if (spu_extract(mask, 0))
               colors1[0] = spu.sample_texture(unit, texcoords[0]);
            if (spu_extract(mask, 1))
               colors1[1] = spu.sample_texture(unit, texcoords[1]);
            if (spu_extract(mask, 2))
               colors1[2] = spu.sample_texture(unit, texcoords[2]);
            if (spu_extract(mask, 3))
               colors1[3] = spu.sample_texture(unit, texcoords[3]);

            /* hack: modulate first texture by second */
            colors[0] = spu_mul(colors[0], colors1[0]);
            colors[1] = spu_mul(colors[1], colors1[1]);
            colors[2] = spu_mul(colors[2], colors1[2]);
            colors[3] = spu_mul(colors[3], colors1[3]);
         }

      }
      else {
         /* simple shading */
         eval_coeff(1, (float) x, (float) y, colors);
      }


      /* Convert fragment data from AoS to SoA format.
       */
      qword soa_frag[4];
      _transpose_matrix4x4((vec_float4 *) soa_frag, colors);

      /* Read the current framebuffer values.
       */
      const qword pix[4] = {
         (qword) spu_splats(spu.ctile.ui[iy+0][ix+0]),
         (qword) spu_splats(spu.ctile.ui[iy+0][ix+1]),
         (qword) spu_splats(spu.ctile.ui[iy+1][ix+0]),
         (qword) spu_splats(spu.ctile.ui[iy+1][ix+1]),
      };

      qword soa_pix[4];

      if (spu.read_fb) {
         /* Convert pixel data from AoS to SoA format.
          */
         vec_float4 aos_pix[4] = {
            spu_unpack_A8R8G8B8(spu.ctile.ui[iy+0][ix+0]),
            spu_unpack_A8R8G8B8(spu.ctile.ui[iy+0][ix+1]),
            spu_unpack_A8R8G8B8(spu.ctile.ui[iy+1][ix+0]),
            spu_unpack_A8R8G8B8(spu.ctile.ui[iy+1][ix+1]),
         };

         _transpose_matrix4x4((vec_float4 *) soa_pix, aos_pix);
      }


      struct spu_blend_results result =
          (*spu.blend)(soa_frag[0], soa_frag[1], soa_frag[2], soa_frag[3],
                       soa_pix[0], soa_pix[1], soa_pix[2], soa_pix[3],
                       spu.const_blend_color[0], spu.const_blend_color[1],
                       spu.const_blend_color[2], spu.const_blend_color[3]);


      /* Convert final pixel data from SoA to AoS format.
       */
      result = (*spu.logicop)(pix[0], pix[1], pix[2], pix[3],
                              result.r, result.g, result.b, result.a,
                              (qword) mask);

      spu.ctile.ui[iy+0][ix+0] = spu_extract((vec_uint4) result.r, 0);
      spu.ctile.ui[iy+0][ix+1] = spu_extract((vec_uint4) result.g, 0);
      spu.ctile.ui[iy+1][ix+0] = spu_extract((vec_uint4) result.b, 0);
      spu.ctile.ui[iy+1][ix+1] = spu_extract((vec_uint4) result.a, 0);
   }
#endif
}


/**
 * Given an X or Y coordinate, return the block/quad coordinate that it
 * belongs to.
 */
static INLINE int block( int x )
{
   return x & ~1;
}


/**
 * Compute mask which indicates which pixels in the 2x2 quad are actually inside
 * the triangle's bounds.
 * The mask is a uint4 vector and each element will be 0 or 0xffffffff.
 */
static INLINE mask_t calculate_mask( int x )
{
   /* This is a little tricky.
    * Use & instead of && to avoid branches.
    * Use negation to convert true/false to ~0/0 values.
    */
   mask_t mask;
   mask = spu_insert(-((x   >= setup.span.left[0]) & (x   < setup.span.right[0])), mask, 0);
   mask = spu_insert(-((x+1 >= setup.span.left[0]) & (x+1 < setup.span.right[0])), mask, 1);
   mask = spu_insert(-((x   >= setup.span.left[1]) & (x   < setup.span.right[1])), mask, 2);
   mask = spu_insert(-((x+1 >= setup.span.left[1]) & (x+1 < setup.span.right[1])), mask, 3);
   return mask;
}


/**
 * Render a horizontal span of quads
 */
static void flush_spans( void )
{
   int minleft, maxright;
   int x;

   switch (setup.span.y_flags) {
   case 0x3:
      /* both odd and even lines written (both quad rows) */
      minleft = MIN2(setup.span.left[0], setup.span.left[1]);
      maxright = MAX2(setup.span.right[0], setup.span.right[1]);
      break;

   case 0x1:
      /* only even line written (quad top row) */
      minleft = setup.span.left[0];
      maxright = setup.span.right[0];
      break;

   case 0x2:
      /* only odd line written (quad bottom row) */
      minleft = setup.span.left[1];
      maxright = setup.span.right[1];
      break;

   default:
      return;
   }


   /* OK, we're very likely to need the tile data now.
    * clear or finish waiting if needed.
    */
   if (spu.cur_ctile_status == TILE_STATUS_GETTING) {
      /* wait for mfc_get() to complete */
      //printf("SPU: %u: waiting for ctile\n", spu.init.id);
      wait_on_mask(1 << TAG_READ_TILE_COLOR);
      spu.cur_ctile_status = TILE_STATUS_CLEAN;
   }
   else if (spu.cur_ctile_status == TILE_STATUS_CLEAR) {
      //printf("SPU %u: clearing C tile %u, %u\n", spu.init.id, setup.tx, setup.ty);
      clear_c_tile(&spu.ctile);
      spu.cur_ctile_status = TILE_STATUS_DIRTY;
   }
   ASSERT(spu.cur_ctile_status != TILE_STATUS_DEFINED);

   if (spu.read_depth) {
      if (spu.cur_ztile_status == TILE_STATUS_GETTING) {
         /* wait for mfc_get() to complete */
         //printf("SPU: %u: waiting for ztile\n", spu.init.id);
         wait_on_mask(1 << TAG_READ_TILE_Z);
         spu.cur_ztile_status = TILE_STATUS_CLEAN;
      }
      else if (spu.cur_ztile_status == TILE_STATUS_CLEAR) {
         //printf("SPU %u: clearing Z tile %u, %u\n", spu.init.id, setup.tx, setup.ty);
         clear_z_tile(&spu.ztile);
         spu.cur_ztile_status = TILE_STATUS_DIRTY;
      }
      ASSERT(spu.cur_ztile_status != TILE_STATUS_DEFINED);
   }

   /* XXX this loop could be moved into the above switch cases and
    * calculate_mask() could be simplified a bit...
    */
   for (x = block(minleft); x <= block(maxright); x += 2) {
#if 1
      emit_quad( x, setup.span.y, calculate_mask( x ) );
#endif
   }

   setup.span.y = 0;
   setup.span.y_flags = 0;
   setup.span.right[0] = 0;
   setup.span.right[1] = 0;
}

#if DEBUG_VERTS
static void print_vertex(const struct vertex_header *v)
{
   int i;
   fprintf(stderr, "Vertex: (%p)\n", v);
   for (i = 0; i < setup.quad.nr_attrs; i++) {
      fprintf(stderr, "  %d: %f %f %f %f\n",  i, 
              v->data[i][0], v->data[i][1], v->data[i][2], v->data[i][3]);
   }
}
#endif


static boolean setup_sort_vertices(const struct vertex_header *v0,
                                   const struct vertex_header *v1,
                                   const struct vertex_header *v2)
{

#if DEBUG_VERTS
   fprintf(stderr, "Triangle:\n");
   print_vertex(v0);
   print_vertex(v1);
   print_vertex(v2);
#endif

   setup.vprovoke = v2;

   /* determine bottom to top order of vertices */
   {
      float y0 = spu_extract(v0->data[0], 1);
      float y1 = spu_extract(v1->data[0], 1);
      float y2 = spu_extract(v2->data[0], 1);
      if (y0 <= y1) {
	 if (y1 <= y2) {
	    /* y0<=y1<=y2 */
	    setup.vmin = v0;   
	    setup.vmid = v1;   
	    setup.vmax = v2;
	 }
	 else if (y2 <= y0) {
	    /* y2<=y0<=y1 */
	    setup.vmin = v2;   
	    setup.vmid = v0;   
	    setup.vmax = v1;   
	 }
	 else {
	    /* y0<=y2<=y1 */
	    setup.vmin = v0;   
	    setup.vmid = v2;   
	    setup.vmax = v1;  
	 }
      }
      else {
	 if (y0 <= y2) {
	    /* y1<=y0<=y2 */
	    setup.vmin = v1;   
	    setup.vmid = v0;   
	    setup.vmax = v2;  
	 }
	 else if (y2 <= y1) {
	    /* y2<=y1<=y0 */
	    setup.vmin = v2;   
	    setup.vmid = v1;   
	    setup.vmax = v0;  
	 }
	 else {
	    /* y1<=y2<=y0 */
	    setup.vmin = v1;   
	    setup.vmid = v2;   
	    setup.vmax = v0;
	 }
      }
   }

   /* Check if triangle is completely outside the tile bounds */
   if (spu_extract(setup.vmin->data[0], 1) > setup.cliprect_maxy)
      return FALSE;
   if (spu_extract(setup.vmax->data[0], 1) < setup.cliprect_miny)
      return FALSE;
   if (spu_extract(setup.vmin->data[0], 0) < setup.cliprect_minx &&
       spu_extract(setup.vmid->data[0], 0) < setup.cliprect_minx &&
       spu_extract(setup.vmax->data[0], 0) < setup.cliprect_minx)
      return FALSE;
   if (spu_extract(setup.vmin->data[0], 0) > setup.cliprect_maxx &&
       spu_extract(setup.vmid->data[0], 0) > setup.cliprect_maxx &&
       spu_extract(setup.vmax->data[0], 0) > setup.cliprect_maxx)
      return FALSE;

   setup.ebot.dx = spu_extract(setup.vmid->data[0], 0) - spu_extract(setup.vmin->data[0], 0);
   setup.ebot.dy = spu_extract(setup.vmid->data[0], 1) - spu_extract(setup.vmin->data[0], 1);
   setup.emaj.dx = spu_extract(setup.vmax->data[0], 0) - spu_extract(setup.vmin->data[0], 0);
   setup.emaj.dy = spu_extract(setup.vmax->data[0], 1) - spu_extract(setup.vmin->data[0], 1);
   setup.etop.dx = spu_extract(setup.vmax->data[0], 0) - spu_extract(setup.vmid->data[0], 0);
   setup.etop.dy = spu_extract(setup.vmax->data[0], 1) - spu_extract(setup.vmid->data[0], 1);

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
      const float area = (setup.emaj.dx * setup.ebot.dy - 
			    setup.ebot.dx * setup.emaj.dy);

      setup.oneoverarea = 1.0f / area;
      /*
      _mesa_printf("%s one-over-area %f  area %f  det %f\n",
                   __FUNCTION__, setup.oneoverarea, area, prim->det );
      */
   }

#if 0
   /* We need to know if this is a front or back-facing triangle for:
    *  - the GLSL gl_FrontFacing fragment attribute (bool)
    *  - two-sided stencil test
    */
   setup.quad.facing = (prim->det > 0.0) ^ (setup.softpipe->rasterizer->front_winding == PIPE_WINDING_CW);
#endif

   return TRUE;
}


/**
 * Compute a0 for a constant-valued coefficient (GL_FLAT shading).
 * The value value comes from vertex->data[slot].
 * The result will be put into setup.coef[slot].a0.
 * \param slot  which attribute slot 
 */
static INLINE void
const_coeff(uint slot)
{
   setup.coef[slot].dadx.v = (vector float) {0.0, 0.0, 0.0, 0.0};
   setup.coef[slot].dady.v = (vector float) {0.0, 0.0, 0.0, 0.0};
   setup.coef[slot].a0.v = setup.vprovoke->data[slot];
}


/**
 * Compute a0, dadx and dady for a linearly interpolated coefficient,
 * for a triangle.
 */
static INLINE void
tri_linear_coeff(uint slot, uint firstComp, uint lastComp)
{
   uint i;
   const float *vmin_d = (float *) &setup.vmin->data[slot];
   const float *vmid_d = (float *) &setup.vmid->data[slot];
   const float *vmax_d = (float *) &setup.vmax->data[slot];
   const float x = spu_extract(setup.vmin->data[0], 0) - 0.5f;
   const float y = spu_extract(setup.vmin->data[0], 1) - 0.5f;

   for (i = firstComp; i < lastComp; i++) {
      float botda = vmid_d[i] - vmin_d[i];
      float majda = vmax_d[i] - vmin_d[i];
      float a = setup.ebot.dy * majda - botda * setup.emaj.dy;
      float b = setup.emaj.dx * botda - majda * setup.ebot.dx;
   
      ASSERT(slot < PIPE_MAX_SHADER_INPUTS);

      setup.coef[slot].dadx.f[i] = a * setup.oneoverarea;
      setup.coef[slot].dady.f[i] = b * setup.oneoverarea;

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
      setup.coef[slot].a0.f[i] = (vmin_d[i] - 
                                 (setup.coef[slot].dadx.f[i] * x + 
                                  setup.coef[slot].dady.f[i] * y));
   }

   /*
   _mesa_printf("attr[%d].%c: %f dx:%f dy:%f\n",
		slot, "xyzw"[i], 
		setup.coef[slot].a0[i],
		setup.coef[slot].dadx.f[i],
		setup.coef[slot].dady.f[i]);
   */
}


/**
 * As above, but interp setup all four vector components.
 */
static INLINE void
tri_linear_coeff4(uint slot)
{
   const vector float vmin_d = setup.vmin->data[slot];
   const vector float vmid_d = setup.vmid->data[slot];
   const vector float vmax_d = setup.vmax->data[slot];
   const vector float xxxx = spu_splats(spu_extract(setup.vmin->data[0], 0) - 0.5f);
   const vector float yyyy = spu_splats(spu_extract(setup.vmin->data[0], 1) - 0.5f);

   vector float botda = vmid_d - vmin_d;
   vector float majda = vmax_d - vmin_d;

   vector float a = spu_sub(spu_mul(spu_splats(setup.ebot.dy), majda),
                            spu_mul(botda, spu_splats(setup.emaj.dy)));
   vector float b = spu_sub(spu_mul(spu_splats(setup.emaj.dx), botda),
                            spu_mul(majda, spu_splats(setup.ebot.dx)));

   setup.coef[slot].dadx.v = spu_mul(a, spu_splats(setup.oneoverarea));
   setup.coef[slot].dady.v = spu_mul(b, spu_splats(setup.oneoverarea));

   vector float tempx = spu_mul(setup.coef[slot].dadx.v, xxxx);
   vector float tempy = spu_mul(setup.coef[slot].dady.v, yyyy);
                         
   setup.coef[slot].a0.v = spu_sub(vmin_d, spu_add(tempx, tempy));
}



#if 0
/**
 * Compute a0, dadx and dady for a perspective-corrected interpolant,
 * for a triangle.
 * We basically multiply the vertex value by 1/w before computing
 * the plane coefficients (a0, dadx, dady).
 * Later, when we compute the value at a particular fragment position we'll
 * divide the interpolated value by the interpolated W at that fragment.
 */
static void tri_persp_coeff( unsigned slot,
                             unsigned i )
{
   /* premultiply by 1/w:
    */
   float mina = setup.vmin->data[slot][i] * setup.vmin->data[0][3];
   float mida = setup.vmid->data[slot][i] * setup.vmid->data[0][3];
   float maxa = setup.vmax->data[slot][i] * setup.vmax->data[0][3];

   float botda = mida - mina;
   float majda = maxa - mina;
   float a = setup.ebot.dy * majda - botda * setup.emaj.dy;
   float b = setup.emaj.dx * botda - majda * setup.ebot.dx;
      
   /*
   printf("tri persp %d,%d: %f %f %f\n", slot, i,
          setup.vmin->data[slot][i],
          setup.vmid->data[slot][i],
          setup.vmax->data[slot][i]
          );
   */

   assert(slot < PIPE_MAX_SHADER_INPUTS);
   assert(i <= 3);

   setup.coef[slot].dadx.f[i] = a * setup.oneoverarea;
   setup.coef[slot].dady.f[i] = b * setup.oneoverarea;
   setup.coef[slot].a0.f[i] = (mina - 
			    (setup.coef[slot].dadx.f[i] * (setup.vmin->data[0][0] - 0.5f) + 
			     setup.coef[slot].dady.f[i] * (setup.vmin->data[0][1] - 0.5f)));
}
#endif


/**
 * Compute the setup.coef[] array dadx, dady, a0 values.
 * Must be called after setup.vmin,vmid,vmax,vprovoke are initialized.
 */
static void setup_tri_coefficients(void)
{
#if 1
   uint i;

   for (i = 0; i < spu.vertex_info.num_attribs; i++) {
      switch (spu.vertex_info.interp_mode[i]) {
      case INTERP_NONE:
         break;
      case INTERP_POS:
         /*tri_linear_coeff(i, 2, 3);*/
         /* XXX interp W if PERSPECTIVE... */
         tri_linear_coeff4(i);
         break;
      case INTERP_CONSTANT:
         const_coeff(i);
         break;
      case INTERP_LINEAR:
         tri_linear_coeff4(i);
         break;
      case INTERP_PERSPECTIVE:
         tri_linear_coeff4(i);  /* temporary */
         break;
      default:
         ASSERT(0);
      }
   }
#else
   ASSERT(spu.vertex_info.interp_mode[0] == INTERP_POS);
   ASSERT(spu.vertex_info.interp_mode[1] == INTERP_LINEAR ||
          spu.vertex_info.interp_mode[1] == INTERP_CONSTANT);
   tri_linear_coeff(0, 2, 3);  /* slot 0, z */
   tri_linear_coeff(1, 0, 4);  /* slot 1, color */
#endif
}


static void setup_tri_edges(void)
{
   float vmin_x = spu_extract(setup.vmin->data[0], 0) + 0.5f;
   float vmid_x = spu_extract(setup.vmid->data[0], 0) + 0.5f;

   float vmin_y = spu_extract(setup.vmin->data[0], 1) - 0.5f;
   float vmid_y = spu_extract(setup.vmid->data[0], 1) - 0.5f;
   float vmax_y = spu_extract(setup.vmax->data[0], 1) - 0.5f;

   setup.emaj.sy = CEILF(vmin_y);
   setup.emaj.lines = (int) CEILF(vmax_y - setup.emaj.sy);
   setup.emaj.dxdy = setup.emaj.dx / setup.emaj.dy;
   setup.emaj.sx = vmin_x + (setup.emaj.sy - vmin_y) * setup.emaj.dxdy;

   setup.etop.sy = CEILF(vmid_y);
   setup.etop.lines = (int) CEILF(vmax_y - setup.etop.sy);
   setup.etop.dxdy = setup.etop.dx / setup.etop.dy;
   setup.etop.sx = vmid_x + (setup.etop.sy - vmid_y) * setup.etop.dxdy;

   setup.ebot.sy = CEILF(vmin_y);
   setup.ebot.lines = (int) CEILF(vmid_y - setup.ebot.sy);
   setup.ebot.dxdy = setup.ebot.dx / setup.ebot.dy;
   setup.ebot.sx = vmin_x + (setup.ebot.sy - vmin_y) * setup.ebot.dxdy;
}


/**
 * Render the upper or lower half of a triangle.
 * Scissoring/cliprect is applied here too.
 */
static void subtriangle( struct edge *eleft,
			 struct edge *eright,
			 unsigned lines )
{
   const int minx = setup.cliprect_minx;
   const int maxx = setup.cliprect_maxx;
   const int miny = setup.cliprect_miny;
   const int maxy = setup.cliprect_maxy;
   int y, start_y, finish_y;
   int sy = (int)eleft->sy;

   ASSERT((int)eleft->sy == (int) eright->sy);

   /* clip top/bottom */
   start_y = sy;
   finish_y = sy + lines;

   if (start_y < miny)
      start_y = miny;

   if (finish_y > maxy)
      finish_y = maxy;

   start_y -= sy;
   finish_y -= sy;

   /*
   _mesa_printf("%s %d %d\n", __FUNCTION__, start_y, finish_y);  
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
         if (block(_y) != setup.span.y) {
            flush_spans();
            setup.span.y = block(_y);
         }

         setup.span.left[_y&1] = left;
         setup.span.right[_y&1] = right;
         setup.span.y_flags |= 1<<(_y&1);
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
 * Draw triangle into tile at (tx, ty) (tile coords)
 * The tile data should have already been fetched.
 */
boolean
tri_draw(const float *v0, const float *v1, const float *v2, uint tx, uint ty)
{
   setup.tx = tx;
   setup.ty = ty;

   /* set clipping bounds to tile bounds */
   setup.cliprect_minx = tx * TILE_SIZE;
   setup.cliprect_miny = ty * TILE_SIZE;
   setup.cliprect_maxx = (tx + 1) * TILE_SIZE;
   setup.cliprect_maxy = (ty + 1) * TILE_SIZE;

   if (!setup_sort_vertices((struct vertex_header *) v0,
                            (struct vertex_header *) v1,
                            (struct vertex_header *) v2)) {
      return FALSE; /* totally clipped */
   }

   setup_tri_coefficients();
   setup_tri_edges();

   setup.span.y = 0;
   setup.span.y_flags = 0;
   setup.span.right[0] = 0;
   setup.span.right[1] = 0;
   /*   setup.span.z_mode = tri_z_mode( setup.ctx ); */

   /*   init_constant_attribs( setup ); */
      
   if (setup.oneoverarea < 0.0) {
      /* emaj on left:
       */
      subtriangle( &setup.emaj, &setup.ebot, setup.ebot.lines );
      subtriangle( &setup.emaj, &setup.etop, setup.etop.lines );
   }
   else {
      /* emaj on right:
       */
      subtriangle( &setup.ebot, &setup.emaj, setup.ebot.lines );
      subtriangle( &setup.etop, &setup.emaj, setup.etop.lines );
   }

   flush_spans();

   return TRUE;
}
