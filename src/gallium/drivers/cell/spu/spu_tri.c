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
#include "util/u_math.h"
#include "spu_colorpack.h"
#include "spu_main.h"
#include "spu_texture.h"
#include "spu_tile.h"
#include "spu_tri.h"


/** Masks are uint[4] vectors with each element being 0 or 0xffffffff */
typedef vector unsigned int mask_t;



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
   vector float a0;
   vector float dadx;
   vector float dady;
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

   float oneOverArea;  /* XXX maybe make into vector? */

   uint facing;

   uint tx, ty;  /**< position of current tile (x, y) */

   int cliprect_minx, cliprect_maxx, cliprect_miny, cliprect_maxy;

   struct interp_coef coef[PIPE_MAX_SHADER_INPUTS];

   struct {
      int left[2];   /**< [0] = row0, [1] = row1 */
      int right[2];
      int y;
      unsigned y_flags;
      unsigned mask;     /**< mask of MASK_BOTTOM/TOP_LEFT/RIGHT bits */
   } span;
};


static struct setup_stage setup;


/**
 * Evaluate attribute coefficients (plane equations) to compute
 * attribute values for the four fragments in a quad.
 * Eg: four colors will be computed (in AoS format).
 */
static INLINE void
eval_coeff(uint slot, float x, float y, vector float w, vector float result[4])
{
   switch (spu.vertex_info.attrib[slot].interp_mode) {
   case INTERP_CONSTANT:
      result[QUAD_TOP_LEFT] =
      result[QUAD_TOP_RIGHT] =
      result[QUAD_BOTTOM_LEFT] =
      result[QUAD_BOTTOM_RIGHT] = setup.coef[slot].a0;
      break;
   case INTERP_LINEAR:
      {
         vector float dadx = setup.coef[slot].dadx;
         vector float dady = setup.coef[slot].dady;
         vector float topLeft =
            spu_add(setup.coef[slot].a0,
                    spu_add(spu_mul(spu_splats(x), dadx),
                            spu_mul(spu_splats(y), dady)));

         result[QUAD_TOP_LEFT] = topLeft;
         result[QUAD_TOP_RIGHT] = spu_add(topLeft, dadx);
         result[QUAD_BOTTOM_LEFT] = spu_add(topLeft, dady);
         result[QUAD_BOTTOM_RIGHT] = spu_add(spu_add(topLeft, dadx), dady);
      }
      break;
   case INTERP_PERSPECTIVE:
      {
         vector float dadx = setup.coef[slot].dadx;
         vector float dady = setup.coef[slot].dady;
         vector float topLeft =
            spu_add(setup.coef[slot].a0,
                    spu_add(spu_mul(spu_splats(x), dadx),
                            spu_mul(spu_splats(y), dady)));

         vector float wInv = spu_re(w);  /* 1.0 / w */

         result[QUAD_TOP_LEFT] = spu_mul(topLeft, wInv);
         result[QUAD_TOP_RIGHT] = spu_mul(spu_add(topLeft, dadx), wInv);
         result[QUAD_BOTTOM_LEFT] = spu_mul(spu_add(topLeft, dady), wInv);
         result[QUAD_BOTTOM_RIGHT] = spu_mul(spu_add(spu_add(topLeft, dadx), dady), wInv);
      }
      break;
   case INTERP_POS:
   case INTERP_NONE:
      break;
   default:
      ASSERT(0);
   }
}


/**
 * As above, but return 4 vectors in SOA format.
 * XXX this will all be re-written someday.
 */
static INLINE void
eval_coeff_soa(uint slot, float x, float y, vector float w, vector float result[4])
{
   eval_coeff(slot, x, y, w, result);
   _transpose_matrix4x4(result, result);
}


/** Evalute coefficients to get Z for four pixels in a quad */
static INLINE vector float
eval_z(float x, float y)
{
   const uint slot = 0;
   const float dzdx = spu_extract(setup.coef[slot].dadx, 2);
   const float dzdy = spu_extract(setup.coef[slot].dady, 2);
   const float topLeft = spu_extract(setup.coef[slot].a0, 2) + x * dzdx + y * dzdy;
   const vector float topLeftv = spu_splats(topLeft);
   const vector float derivs = (vector float) { 0.0, dzdx, dzdy, dzdx + dzdy };
   return spu_add(topLeftv, derivs);
}


/** Evalute coefficients to get W for four pixels in a quad */
static INLINE vector float
eval_w(float x, float y)
{
   const uint slot = 0;
   const float dwdx = spu_extract(setup.coef[slot].dadx, 3);
   const float dwdy = spu_extract(setup.coef[slot].dady, 3);
   const float topLeft = spu_extract(setup.coef[slot].a0, 3) + x * dwdx + y * dwdy;
   const vector float topLeftv = spu_splats(topLeft);
   const vector float derivs = (vector float) { 0.0, dwdx, dwdy, dwdx + dwdy };
   return spu_add(topLeftv, derivs);
}


/**
 * Emit a quad (pass to next stage).  No clipping is done.
 * Note: about 1/5 to 1/7 of the time, mask is zero and this function
 * should be skipped.  But adding the test for that slows things down
 * overall.
 */
static INLINE void
emit_quad( int x, int y, mask_t mask)
{
   /* If any bits in mask are set... */
   if (spu_extract(spu_orx(mask), 0)) {
      const int ix = x - setup.cliprect_minx;
      const int iy = y - setup.cliprect_miny;

      spu.cur_ctile_status = TILE_STATUS_DIRTY;
      spu.cur_ztile_status = TILE_STATUS_DIRTY;

      {
         /*
          * Run fragment shader, execute per-fragment ops, update fb/tile.
          */
         vector float inputs[4*4], outputs[2*4];
         vector float fragZ = eval_z((float) x, (float) y);
         vector float fragW = eval_w((float) x, (float) y);
         vector unsigned int kill_mask;

         /* setup inputs */
#if 0
         eval_coeff_soa(1, (float) x, (float) y, fragW, inputs);
#else
         uint i;
         for (i = 0; i < spu.vertex_info.num_attribs; i++) {
            eval_coeff_soa(i+1, (float) x, (float) y, fragW, inputs + i * 4);
         }
#endif
         ASSERT(spu.fragment_program);
         ASSERT(spu.fragment_ops);

         /* Execute the current fragment program */
         kill_mask = spu.fragment_program(inputs, outputs, spu.constants);

         mask = spu_andc(mask, kill_mask);

         /* Execute per-fragment/quad operations, including:
          * alpha test, z test, stencil test, blend and framebuffer writing.
          */
         spu.fragment_ops(ix, iy, &spu.ctile, &spu.ztile,
                          fragZ,
                          outputs[0*4+0],
                          outputs[0*4+1],
                          outputs[0*4+2],
                          outputs[0*4+3],
                          mask,
                          setup.facing);
      }
   }
}


/**
 * Given an X or Y coordinate, return the block/quad coordinate that it
 * belongs to.
 */
static INLINE int
block(int x)
{
   return x & ~1;
}


/**
 * Compute mask which indicates which pixels in the 2x2 quad are actually inside
 * the triangle's bounds.
 * The mask is a uint4 vector and each element will be 0 or 0xffffffff.
 */
static INLINE mask_t
calculate_mask(int x)
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
static void
flush_spans(void)
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
      emit_quad( x, setup.span.y, calculate_mask( x ));
   }

   setup.span.y = 0;
   setup.span.y_flags = 0;
   setup.span.right[0] = 0;
   setup.span.right[1] = 0;
}


#if DEBUG_VERTS
static void
print_vertex(const struct vertex_header *v)
{
   uint i;
   fprintf(stderr, "  Vertex: (%p)\n", v);
   for (i = 0; i < spu.vertex_info.num_attribs; i++) {
      fprintf(stderr, "    %d: %f %f %f %f\n",  i, 
              spu_extract(v->data[i], 0),
              spu_extract(v->data[i], 1),
              spu_extract(v->data[i], 2),
              spu_extract(v->data[i], 3));
   }
}
#endif


/**
 * Sort vertices from top to bottom.
 * Compute area and determine front vs. back facing.
 * Do coarse clip test against tile bounds
 * \return  FALSE if tri is totally outside tile, TRUE otherwise
 */
static boolean
setup_sort_vertices(const struct vertex_header *v0,
                    const struct vertex_header *v1,
                    const struct vertex_header *v2)
{
   float area, sign;

#if DEBUG_VERTS
   if (spu.init.id==0) {
      fprintf(stderr, "SPU %u: Triangle:\n", spu.init.id);
      print_vertex(v0);
      print_vertex(v1);
      print_vertex(v2);
   }
#endif

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
            sign = -1.0f;
	 }
	 else if (y2 <= y0) {
	    /* y2<=y0<=y1 */
	    setup.vmin = v2;   
	    setup.vmid = v0;   
	    setup.vmax = v1;   
            sign = -1.0f;
	 }
	 else {
	    /* y0<=y2<=y1 */
	    setup.vmin = v0;   
	    setup.vmid = v2;   
	    setup.vmax = v1;  
            sign = 1.0f;
	 }
      }
      else {
	 if (y0 <= y2) {
	    /* y1<=y0<=y2 */
	    setup.vmin = v1;   
	    setup.vmid = v0;   
	    setup.vmax = v2;  
            sign = 1.0f;
	 }
	 else if (y2 <= y1) {
	    /* y2<=y1<=y0 */
	    setup.vmin = v2;   
	    setup.vmid = v1;   
	    setup.vmax = v0;  
            sign = 1.0f;
	 }
	 else {
	    /* y1<=y2<=y0 */
	    setup.vmin = v1;   
	    setup.vmid = v2;   
	    setup.vmax = v0;
            sign = -1.0f;
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
    */
   area = setup.emaj.dx * setup.ebot.dy - setup.ebot.dx * setup.emaj.dy;

   setup.oneOverArea = 1.0f / area;

   /* The product of area * sign indicates front/back orientation (0/1) */
   setup.facing = (area * sign > 0.0f)
      ^ (spu.rasterizer.front_winding == PIPE_WINDING_CW);

   setup.vprovoke = v2;

   return TRUE;
}


/**
 * Compute a0 for a constant-valued coefficient (GL_FLAT shading).
 * The value value comes from vertex->data[slot].
 * The result will be put into setup.coef[slot].a0.
 * \param slot  which attribute slot 
 */
static INLINE void
const_coeff4(uint slot)
{
   setup.coef[slot].dadx = (vector float) {0.0, 0.0, 0.0, 0.0};
   setup.coef[slot].dady = (vector float) {0.0, 0.0, 0.0, 0.0};
   setup.coef[slot].a0 = setup.vprovoke->data[slot];
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

   setup.coef[slot].dadx = spu_mul(a, spu_splats(setup.oneOverArea));
   setup.coef[slot].dady = spu_mul(b, spu_splats(setup.oneOverArea));

   vector float tempx = spu_mul(setup.coef[slot].dadx, xxxx);
   vector float tempy = spu_mul(setup.coef[slot].dady, yyyy);
                         
   setup.coef[slot].a0 = spu_sub(vmin_d, spu_add(tempx, tempy));
}


/**
 * Compute a0, dadx and dady for a perspective-corrected interpolant,
 * for a triangle.
 * We basically multiply the vertex value by 1/w before computing
 * the plane coefficients (a0, dadx, dady).
 * Later, when we compute the value at a particular fragment position we'll
 * divide the interpolated value by the interpolated W at that fragment.
 */
static void
tri_persp_coeff4(uint slot)
{
   const vector float xxxx = spu_splats(spu_extract(setup.vmin->data[0], 0) - 0.5f);
   const vector float yyyy = spu_splats(spu_extract(setup.vmin->data[0], 1) - 0.5f);

   const vector float vmin_w = spu_splats(spu_extract(setup.vmin->data[0], 3));
   const vector float vmid_w = spu_splats(spu_extract(setup.vmid->data[0], 3));
   const vector float vmax_w = spu_splats(spu_extract(setup.vmax->data[0], 3));

   vector float vmin_d = setup.vmin->data[slot];
   vector float vmid_d = setup.vmid->data[slot];
   vector float vmax_d = setup.vmax->data[slot];

   vmin_d = spu_mul(vmin_d, vmin_w);
   vmid_d = spu_mul(vmid_d, vmid_w);
   vmax_d = spu_mul(vmax_d, vmax_w);

   vector float botda = vmid_d - vmin_d;
   vector float majda = vmax_d - vmin_d;

   vector float a = spu_sub(spu_mul(spu_splats(setup.ebot.dy), majda),
                            spu_mul(botda, spu_splats(setup.emaj.dy)));
   vector float b = spu_sub(spu_mul(spu_splats(setup.emaj.dx), botda),
                            spu_mul(majda, spu_splats(setup.ebot.dx)));

   setup.coef[slot].dadx = spu_mul(a, spu_splats(setup.oneOverArea));
   setup.coef[slot].dady = spu_mul(b, spu_splats(setup.oneOverArea));

   vector float tempx = spu_mul(setup.coef[slot].dadx, xxxx);
   vector float tempy = spu_mul(setup.coef[slot].dady, yyyy);
                         
   setup.coef[slot].a0 = spu_sub(vmin_d, spu_add(tempx, tempy));
}



/**
 * Compute the setup.coef[] array dadx, dady, a0 values.
 * Must be called after setup.vmin,vmid,vmax,vprovoke are initialized.
 */
static void
setup_tri_coefficients(void)
{
   uint i;

   for (i = 0; i < spu.vertex_info.num_attribs; i++) {
      switch (spu.vertex_info.attrib[i].interp_mode) {
      case INTERP_NONE:
         break;
      case INTERP_CONSTANT:
         const_coeff4(i);
         break;
      case INTERP_POS:
         /* fall-through */
      case INTERP_LINEAR:
         tri_linear_coeff4(i);
         break;
      case INTERP_PERSPECTIVE:
         tri_persp_coeff4(i);
         break;
      default:
         ASSERT(0);
      }
   }
}


static void
setup_tri_edges(void)
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
static void
subtriangle(struct edge *eleft, struct edge *eright, unsigned lines)
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
tri_draw(const float *v0, const float *v1, const float *v2,
         uint tx, uint ty)
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

   if (setup.oneOverArea < 0.0) {
      /* emaj on left */
      subtriangle( &setup.emaj, &setup.ebot, setup.ebot.lines );
      subtriangle( &setup.emaj, &setup.etop, setup.etop.lines );
   }
   else {
      /* emaj on right */
      subtriangle( &setup.ebot, &setup.emaj, setup.ebot.lines );
      subtriangle( &setup.etop, &setup.emaj, setup.etop.lines );
   }

   flush_spans();

   return TRUE;
}
