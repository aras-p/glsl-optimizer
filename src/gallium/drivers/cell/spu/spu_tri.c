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

#include "pipe/p_compiler.h"
#include "pipe/p_format.h"
#include "util/u_math.h"
#include "spu_colorpack.h"
#include "spu_main.h"
#include "spu_shuffle.h"
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
#define CEILF(X) ((float) (int) ((X) + 0.99999f))


#define QUAD_TOP_LEFT     0
#define QUAD_TOP_RIGHT    1
#define QUAD_BOTTOM_LEFT  2
#define QUAD_BOTTOM_RIGHT 3
#define MASK_TOP_LEFT     (1 << QUAD_TOP_LEFT)
#define MASK_TOP_RIGHT    (1 << QUAD_TOP_RIGHT)
#define MASK_BOTTOM_LEFT  (1 << QUAD_BOTTOM_LEFT)
#define MASK_BOTTOM_RIGHT (1 << QUAD_BOTTOM_RIGHT)
#define MASK_ALL          0xf


#define CHAN0 0
#define CHAN1 1
#define CHAN2 2
#define CHAN3 3


#define DEBUG_VERTS 0

/**
 * Triangle edge info
 */
struct edge {
   union {
      struct {
         float dx;	/**< X(v1) - X(v0), used only during setup */
         float dy;	/**< Y(v1) - Y(v0), used only during setup */
      };
      vec_float4 ds;    /**< vector accessor for dx and dy */
   };
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
   union {
      struct {
         const struct vertex_header *vmin;
         const struct vertex_header *vmid;
         const struct vertex_header *vmax;
         const struct vertex_header *vprovoke;
      };
      qword vertex_headers;
   };

   struct edge ebot;
   struct edge etop;
   struct edge emaj;

   float oneOverArea;  /* XXX maybe make into vector? */

   uint facing;

   uint tx, ty;  /**< position of current tile (x, y) */

   union {
      struct {
         int cliprect_minx;
         int cliprect_miny;
         int cliprect_maxx;
         int cliprect_maxy;
      };
      qword cliprect;
   };

   struct interp_coef coef[PIPE_MAX_SHADER_INPUTS];

   struct {
      vec_int4 quad; /**< [0] = row0, [1] = row1; {left[0],left[1],right[0],right[1]} */
      int y;
      unsigned y_flags;
      unsigned mask;     /**< mask of MASK_BOTTOM/TOP_LEFT/RIGHT bits */
   } span;
};


static struct setup_stage setup;


static INLINE vector float
splatx(vector float v)
{
   return spu_splats(spu_extract(v, CHAN0));
}

static INLINE vector float
splaty(vector float v)
{
   return spu_splats(spu_extract(v, CHAN1));
}

static INLINE vector float
splatz(vector float v)
{
   return spu_splats(spu_extract(v, CHAN2));
}

static INLINE vector float
splatw(vector float v)
{
   return spu_splats(spu_extract(v, CHAN3));
}


/**
 * Setup fragment shader inputs by evaluating triangle's vertex
 * attribute coefficient info.
 * \param x  quad x pos
 * \param y  quad y pos
 * \param fragZ  returns quad Z values
 * \param fragInputs  returns fragment program inputs
 * Note: this code could be incorporated into the fragment program
 * itself to avoid the loop and switch.
 */
static void
eval_inputs(float x, float y, vector float *fragZ, vector float fragInputs[])
{
   static const vector float deltaX = (const vector float) {0, 1, 0, 1};
   static const vector float deltaY = (const vector float) {0, 0, 1, 1};

   const uint posSlot = 0;
   const vector float pos = setup.coef[posSlot].a0;
   const vector float dposdx = setup.coef[posSlot].dadx;
   const vector float dposdy = setup.coef[posSlot].dady;
   const vector float fragX = spu_splats(x) + deltaX;
   const vector float fragY = spu_splats(y) + deltaY;
   vector float fragW, wInv;
   uint i;

   *fragZ = splatz(pos) + fragX * splatz(dposdx) + fragY * splatz(dposdy);
   fragW =  splatw(pos) + fragX * splatw(dposdx) + fragY * splatw(dposdy);
   wInv = spu_re(fragW);  /* 1 / w */

   /* loop over fragment program inputs */
   for (i = 0; i < spu.vertex_info.num_attribs; i++) {
      uint attr = i + 1;
      enum interp_mode interp = spu.vertex_info.attrib[attr].interp_mode;

      /* constant term */
      vector float a0 = setup.coef[attr].a0;
      vector float r0 = splatx(a0);
      vector float r1 = splaty(a0);
      vector float r2 = splatz(a0);
      vector float r3 = splatw(a0);

      if (interp == INTERP_LINEAR || interp == INTERP_PERSPECTIVE) {
         /* linear term */
         vector float dadx = setup.coef[attr].dadx;
         vector float dady = setup.coef[attr].dady;
         /* Use SPU intrinsics here to get slightly better code.
          * originally: r0 += fragX * splatx(dadx) + fragY * splatx(dady);
          */
         r0 = spu_madd(fragX, splatx(dadx), spu_madd(fragY, splatx(dady), r0));
         r1 = spu_madd(fragX, splaty(dadx), spu_madd(fragY, splaty(dady), r1));
         r2 = spu_madd(fragX, splatz(dadx), spu_madd(fragY, splatz(dady), r2));
         r3 = spu_madd(fragX, splatw(dadx), spu_madd(fragY, splatw(dady), r3));
         if (interp == INTERP_PERSPECTIVE) {
            /* perspective term */
            r0 *= wInv;
            r1 *= wInv;
            r2 *= wInv;
            r3 *= wInv;
         }
      }
      fragInputs[CHAN0] = r0;
      fragInputs[CHAN1] = r1;
      fragInputs[CHAN2] = r2;
      fragInputs[CHAN3] = r3;
      fragInputs += 4;
   }
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
         vector unsigned int kill_mask;
         vector float fragZ;

         eval_inputs((float) x, (float) y, &fragZ, inputs);

         ASSERT(spu.fragment_program);
         ASSERT(spu.fragment_ops);

         /* Execute the current fragment program */
         kill_mask = spu.fragment_program(inputs, outputs, spu.constants);

         mask = spu_andc(mask, kill_mask);

         /* Execute per-fragment/quad operations, including:
          * alpha test, z test, stencil test, blend and framebuffer writing.
          * Note that there are two different fragment operations functions
          * that can be called, one for front-facing fragments, and one
          * for back-facing fragments.  (Often the two are the same;
          * but in some cases, like two-sided stenciling, they can be
          * very different.)  So choose the correct function depending
          * on the calculated facing.
          */
         spu.fragment_ops[setup.facing](ix, iy, &spu.ctile, &spu.ztile,
                          fragZ,
                          outputs[0*4+0],
                          outputs[0*4+1],
                          outputs[0*4+2],
                          outputs[0*4+3],
                          mask);
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
 * Render a horizontal span of quads
 */
static void
flush_spans(void)
{
   int minleft, maxright;

   const int l0 = spu_extract(setup.span.quad, 0);
   const int l1 = spu_extract(setup.span.quad, 1);
   const int r0 = spu_extract(setup.span.quad, 2);
   const int r1 = spu_extract(setup.span.quad, 3);

   switch (setup.span.y_flags) {
   case 0x3:
      /* both odd and even lines written (both quad rows) */
      minleft = MIN2(l0, l1);
      maxright = MAX2(r0, r1);
      break;

   case 0x1:
      /* only even line written (quad top row) */
      minleft = l0;
      maxright = r0;
      break;

   case 0x2:
      /* only odd line written (quad bottom row) */
      minleft = l1;
      maxright = r1;
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

   if (spu.read_depth_stencil) {
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

   /* XXX this loop could be moved into the above switch cases... */
   
   /* Setup for mask calculation */
   const vec_int4 quad_LlRr = setup.span.quad;
   const vec_int4 quad_RrLl = spu_rlqwbyte(quad_LlRr, 8);
   const vec_int4 quad_LLll = spu_shuffle(quad_LlRr, quad_LlRr, SHUFFLE4(A,A,B,B));
   const vec_int4 quad_RRrr = spu_shuffle(quad_RrLl, quad_RrLl, SHUFFLE4(A,A,B,B));

   const vec_int4 twos = spu_splats(2);

   const int x = block(minleft);
   vec_int4 xs = {x, x+1, x, x+1};

   for (; spu_extract(xs, 0) <= block(maxright); xs += twos) {
      /**
       * Computes mask to indicate which pixels in the 2x2 quad are actually
       * inside the triangle's bounds.
       */
      
      /* Calculate ({x,x+1,x,x+1} >= {l[0],l[0],l[1],l[1]}) */
      const mask_t gt_LLll_xs = spu_cmpgt(quad_LLll, xs);
      const mask_t gte_xs_LLll = spu_nand(gt_LLll_xs, gt_LLll_xs); 
      
      /* Calculate ({r[0],r[0],r[1],r[1]} > {x,x+1,x,x+1}) */
      const mask_t gt_RRrr_xs = spu_cmpgt(quad_RRrr, xs);

      /* Combine results to create mask */
      const mask_t mask = spu_and(gte_xs_LLll, gt_RRrr_xs);

      emit_quad(spu_extract(xs, 0), setup.span.y, mask);
   }

   setup.span.y = 0;
   setup.span.y_flags = 0;
   /* Zero right elements */
   setup.span.quad = spu_shuffle(setup.span.quad, setup.span.quad, SHUFFLE4(A,B,0,0));
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

/* Returns the minimum of each slot of two vec_float4s as qwords.
 * i.e. return[n] = min(q0[n],q1[n]);
 */
static qword
minfq(qword q0, qword q1)
{
   const qword q0q1m = si_fcgt(q0, q1);
   return si_selb(q0, q1, q0q1m);
}

/* Returns the minimum of each slot of three vec_float4s as qwords.
 * i.e. return[n] = min(q0[n],q1[n],q2[n]);
 */
static qword
min3fq(qword q0, qword q1, qword q2)
{
   return minfq(minfq(q0, q1), q2);
}

/* Returns the maximum of each slot of two vec_float4s as qwords.
 * i.e. return[n] = min(q0[n],q1[n],q2[n]);
 */
static qword
maxfq(qword q0, qword q1) {
   const qword q0q1m = si_fcgt(q0, q1);
   return si_selb(q1, q0, q0q1m);
}

/* Returns the maximum of each slot of three vec_float4s as qwords.
 * i.e. return[n] = min(q0[n],q1[n],q2[n]);
 */
static qword
max3fq(qword q0, qword q1, qword q2) {
   return maxfq(maxfq(q0, q1), q2);
}

/**
 * Sort vertices from top to bottom.
 * Compute area and determine front vs. back facing.
 * Do coarse clip test against tile bounds
 * \return  FALSE if tri is totally outside tile, TRUE otherwise
 */
static boolean
setup_sort_vertices(const qword vs)
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

   {
      /* Load the float values for various processing... */
      const qword f0 = (qword)(((const struct vertex_header*)si_to_ptr(vs))->data[0]);
      const qword f1 = (qword)(((const struct vertex_header*)si_to_ptr(si_rotqbyi(vs, 4)))->data[0]);
      const qword f2 = (qword)(((const struct vertex_header*)si_to_ptr(si_rotqbyi(vs, 8)))->data[0]);

      /* Check if triangle is completely outside the tile bounds
       * Find the min and max x and y positions of the three poits */
      const qword minf = min3fq(f0, f1, f2);
      const qword maxf = max3fq(f0, f1, f2);

      /* Compare min and max against cliprect vals */
      const qword maxsmins = si_shufb(maxf, minf, SHUFB4(A,B,a,b));
      const qword outside = si_fcgt(maxsmins, si_csflt(setup.cliprect, 0));

      /* Use a little magic to work out of the tri is visible or not */
      if(si_to_uint(si_xori(si_gb(outside), 0xc))) return FALSE;

      /* determine bottom to top order of vertices */
      /* A table of shuffle patterns for putting vertex_header pointers into
         correct order.  Quite magical. */
      const qword sort_order_patterns[] = {
         SHUFB4(A,B,C,C),
         SHUFB4(C,A,B,C),
         SHUFB4(A,C,B,C),
         SHUFB4(B,C,A,C),
         SHUFB4(B,A,C,C),
         SHUFB4(C,B,A,C) };

      /* Collate y values into two vectors for comparison.
         Using only one shuffle constant! ;) */
      const qword y_02_ = si_shufb(f0, f2, SHUFB4(0,B,b,C));
      const qword y_10_ = si_shufb(f1, f0, SHUFB4(0,B,b,C));
      const qword y_012 = si_shufb(y_02_, f1, SHUFB4(0,B,b,C));
      const qword y_120 = si_shufb(y_10_, f2, SHUFB4(0,B,b,C));

      /* Perform comparison: {y0,y1,y2} > {y1,y2,y0} */
      const qword compare = si_fcgt(y_012, y_120);
      /* Compress the result of the comparison into 4 bits */
      const qword gather = si_gb(compare);
      /* Subtract one to attain the index into the LUT.  Magical. */
      const unsigned int index = si_to_uint(gather) - 1;

      /* Load the appropriate pattern and construct the desired vector. */
      setup.vertex_headers = si_shufb(vs, vs, sort_order_patterns[index]);

      /* Using the result of the comparison, set sign.
         Very magical. */
      sign = ((si_to_uint(si_cntb(gather)) == 2) ? 1.0f : -1.0f);
   }

   setup.ebot.ds = spu_sub(setup.vmid->data[0], setup.vmin->data[0]);
   setup.emaj.ds = spu_sub(setup.vmax->data[0], setup.vmin->data[0]);
   setup.etop.ds = spu_sub(setup.vmax->data[0], setup.vmid->data[0]);

   /*
    * Compute triangle's area.  Use 1/area to compute partial
    * derivatives of attributes later.
    */
   area = setup.emaj.dx * setup.ebot.dy - setup.ebot.dx * setup.emaj.dy;

   setup.oneOverArea = 1.0f / area;

   /* The product of area * sign indicates front/back orientation (0/1).
    * Just in case someone gets the bright idea of switching the front
    * and back constants without noticing that we're assuming their
    * values in this operation, also assert that the values are
    * what we think they are.
    */
   ASSERT(CELL_FACING_FRONT == 0);
   ASSERT(CELL_FACING_BACK == 1);
   setup.facing = (area * sign > 0.0f)
      ^ (spu.rasterizer.front_winding == PIPE_WINDING_CW);

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
   printf("%s %d %d\n", __FUNCTION__, start_y, finish_y);  
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

         int offset = _y&1;
         vec_int4 quad_LlRr = {left, left, right, right};
         /* Store left and right in 0 or 1 row of quad based on offset */
         setup.span.quad = spu_sel(quad_LlRr, setup.span.quad, spu_maskw(5<<offset));
         setup.span.y_flags |= 1<<offset;
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
tri_draw(const qword vs,
         uint tx, uint ty)
{
   setup.tx = tx;
   setup.ty = ty;

   /* set clipping bounds to tile bounds */
   const qword clipbase = (qword)((vec_uint4){tx, ty});
   const qword clipmin = si_mpyui(clipbase, TILE_SIZE);
   const qword clipmax = si_ai(clipmin, TILE_SIZE);
   setup.cliprect = si_shufb(clipmin, clipmax, SHUFB4(A,B,a,b));

   if(!setup_sort_vertices(vs)) {
      return FALSE; /* totally clipped */
   }

   setup_tri_coefficients();
   setup_tri_edges();

   setup.span.y = 0;
   setup.span.y_flags = 0;
   /* Zero right elements */
   setup.span.quad = spu_shuffle(setup.span.quad, setup.span.quad, SHUFFLE4(A,B,0,0));

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
