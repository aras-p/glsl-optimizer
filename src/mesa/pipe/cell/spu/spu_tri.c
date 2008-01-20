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
#include "pipe/p_util.h"
#include "spu_main.h"
#include "spu_tile.h"
#include "spu_tri.h"



/**
 * Simplified types taken from other parts of Gallium
 */
struct vertex_header {
   float data[2][4];  /* pos and color */
};

struct prim_header {
   struct vertex_header *v[3];
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
   float a0[4];
   float dadx[4];
   float dady[4];
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
 * Clip setup->quad against the scissor/surface bounds.
 */
static INLINE void
quad_clip(struct setup_stage *setup)
{
   const struct pipe_scissor_state *cliprect = &setup->softpipe->cliprect;
   const int minx = (int) cliprect->minx;
   const int maxx = (int) cliprect->maxx;
   const int miny = (int) cliprect->miny;
   const int maxy = (int) cliprect->maxy;

   if (setup->quad.x0 >= maxx ||
       setup->quad.y0 >= maxy ||
       setup->quad.x0 + 1 < minx ||
       setup->quad.y0 + 1 < miny) {
      /* totally clipped */
      setup->quad.mask = 0x0;
      return;
   }
   if (setup->quad.x0 < minx)
      setup->quad.mask &= (MASK_BOTTOM_RIGHT | MASK_TOP_RIGHT);
   if (setup->quad.y0 < miny)
      setup->quad.mask &= (MASK_BOTTOM_LEFT | MASK_BOTTOM_RIGHT);
   if (setup->quad.x0 == maxx - 1)
      setup->quad.mask &= (MASK_BOTTOM_LEFT | MASK_TOP_LEFT);
   if (setup->quad.y0 == maxy - 1)
      setup->quad.mask &= (MASK_TOP_LEFT | MASK_TOP_RIGHT);
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
   if (setup->quad.mask) {
      struct softpipe_context *sp = setup->softpipe;
      sp->quad.first->run(sp->quad.first, &setup->quad);
   }
}
#endif

/**
 * Evaluate attribute coefficients (plane equations) to compute
 * attribute values for the four fragments in a quad.
 * Eg: four colors will be compute.
 */
static INLINE void
eval_coeff( struct setup_stage *setup, uint slot,
            float x, float y, float result[4][4])
{
   uint i;
   const float *dadx = setup->coef[slot].dadx;
   const float *dady = setup->coef[slot].dady;

   /* loop over XYZW comps */
   for (i = 0; i < 4; i++) {
      result[QUAD_TOP_LEFT][i] = setup->coef[slot].a0[i] + x * dadx[i] + y * dady[i];
      result[QUAD_TOP_RIGHT][i] = result[0][i] + dadx[i];
      result[QUAD_BOTTOM_LEFT][i] = result[0][i] + dady[i];
      result[QUAD_BOTTOM_RIGHT][i] = result[0][i] + dadx[i] + dady[i];
   }
}


static INLINE void
eval_z( struct setup_stage *setup,
        float x, float y, float result[4])
{
   const uint slot = 0;
   const uint i = 2;
   const float *dadx = setup->coef[slot].dadx;
   const float *dady = setup->coef[slot].dady;

   result[QUAD_TOP_LEFT] = setup->coef[slot].a0[i] + x * dadx[i] + y * dady[i];
   result[QUAD_TOP_RIGHT] = result[0] + dadx[i];
   result[QUAD_BOTTOM_LEFT] = result[0] + dady[i];
   result[QUAD_BOTTOM_RIGHT] = result[0] + dadx[i] + dady[i];
}


static INLINE uint
pack_color(const float color[4])
{
   uint r = (uint) (color[0] * 255.0);
   uint g = (uint) (color[1] * 255.0);
   uint b = (uint) (color[2] * 255.0);
   uint a = (uint) (color[3] * 255.0);
   r = MIN2(r, 255);
   g = MIN2(g, 255);
   b = MIN2(b, 255);
   a = MIN2(a, 255);
   switch (spu.fb.color_format) {
   case PIPE_FORMAT_A8R8G8B8_UNORM:
      return (a << 24) | (r << 16) | (g << 8) | b;
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      return (b << 24) | (g << 16) | (r << 8) | a;
   default:
      ASSERT(0);
      return 0;
   }
}


static uint
do_depth_test(struct setup_stage *setup, int x, int y, unsigned mask)
{
   int ix = x - setup->cliprect_minx;
   int iy = y - setup->cliprect_miny;
   float zvals[4];
   float zscale = 65535.0;

   if (ZSIZE == 2) {
      ASSERT(spu.fb.depth_format == PIPE_FORMAT_Z16_UNORM);
   }
   else {
      ASSERT(spu.fb.depth_format == PIPE_FORMAT_Z32_UNORM);
   }
   ASSERT(sizeof(ztile[0][0]) == ZSIZE);


   eval_z(setup, (float) x, (float) y, zvals);

   if (tile_status_z[setup->ty][setup->tx] == TILE_STATUS_CLEAR) {
      /* now, _really_ clear the tile */
      clear_tile_z(ztile, spu.fb.depth_clear_value);
   }
   else {
      /* make sure we've got the tile from main mem */
      wait_on_mask(1 << TAG_READ_TILE_Z);
   }
   tile_status_z[setup->ty][setup->tx] = TILE_STATUS_DIRTY;


   if (mask & MASK_TOP_LEFT) {
      uint z = (uint) (zvals[0] * zscale);
      if (z < ztile[iy][ix])
         ztile[iy][ix] = z;
      else
         mask &= ~MASK_TOP_LEFT;
   }

   if (mask & MASK_TOP_RIGHT) {
      uint z = (uint) (zvals[1] * zscale);
      if (z < ztile[iy][ix+1])
         ztile[iy][ix+1] = z;
      else
         mask &= ~MASK_TOP_RIGHT;
   }

   if (mask & MASK_BOTTOM_LEFT) {
      uint z = (uint) (zvals[2] * zscale);
      if (z < ztile[iy+1][ix])
         ztile[iy+1][ix] = z;
      else
         mask &= ~MASK_BOTTOM_LEFT;
   }

   if (mask & MASK_BOTTOM_RIGHT) {
      uint z = (uint) (zvals[3] * zscale);
      if (z < ztile[iy+1][ix+1])
         ztile[iy+1][ix+1] = z;
      else
         mask &= ~MASK_BOTTOM_RIGHT;
   }

   return mask;
}


/**
 * Emit a quad (pass to next stage).  No clipping is done.
 */
static INLINE void
emit_quad( struct setup_stage *setup, int x, int y, unsigned mask )
{
#if 0
   struct softpipe_context *sp = setup->softpipe;
   setup->quad.x0 = x;
   setup->quad.y0 = y;
   setup->quad.mask = mask;
   sp->quad.first->run(sp->quad.first, &setup->quad);
#else
   /* Cell: "write" quad fragments to the tile by setting prim color */
   const int ix = x - setup->cliprect_minx;
   const int iy = y - setup->cliprect_miny;
   float colors[4][4];

   eval_coeff(setup, 1, (float) x, (float) y, colors);

   if (spu.depth_stencil.depth.enabled) {
      mask &= do_depth_test(setup, x, y, mask);
   }

   if (mask) {
      if (tile_status[setup->ty][setup->tx] == TILE_STATUS_CLEAR) {
         /* now, _really_ clear the tile */
         clear_tile(ctile, spu.fb.color_clear_value);
      }
      else {
         /* make sure we've got the tile from main mem */
         wait_on_mask(1 << TAG_READ_TILE_COLOR);
      }
      tile_status[setup->ty][setup->tx] = TILE_STATUS_DIRTY;

      if (mask & MASK_TOP_LEFT)
         ctile[iy][ix] = pack_color(colors[QUAD_TOP_LEFT]);
      if (mask & MASK_TOP_RIGHT)
         ctile[iy][ix+1] = pack_color(colors[QUAD_TOP_RIGHT]);
      if (mask & MASK_BOTTOM_LEFT)
         ctile[iy+1][ix] = pack_color(colors[QUAD_BOTTOM_LEFT]);
      if (mask & MASK_BOTTOM_RIGHT)
         ctile[iy+1][ix+1] = pack_color(colors[QUAD_BOTTOM_RIGHT]);
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
 *
 * this is pretty nasty...  may need to rework flush_spans again to
 * fix it, if possible.
 */
static unsigned calculate_mask( struct setup_stage *setup, int x )
{
   unsigned mask = 0x0;

   if (x >= setup->span.left[0] && x < setup->span.right[0]) 
      mask |= MASK_TOP_LEFT;

   if (x >= setup->span.left[1] && x < setup->span.right[1]) 
      mask |= MASK_BOTTOM_LEFT;
      
   if (x+1 >= setup->span.left[0] && x+1 < setup->span.right[0]) 
      mask |= MASK_TOP_RIGHT;

   if (x+1 >= setup->span.left[1] && x+1 < setup->span.right[1]) 
      mask |= MASK_BOTTOM_RIGHT;

   return mask;
}


/**
 * Render a horizontal span of quads
 */
static void flush_spans( struct setup_stage *setup )
{
   int minleft, maxright;
   int x;

   switch (setup->span.y_flags) {
   case 0x3:
      /* both odd and even lines written (both quad rows) */
      minleft = MIN2(setup->span.left[0], setup->span.left[1]);
      maxright = MAX2(setup->span.right[0], setup->span.right[1]);
      break;

   case 0x1:
      /* only even line written (quad top row) */
      minleft = setup->span.left[0];
      maxright = setup->span.right[0];
      break;

   case 0x2:
      /* only odd line written (quad bottom row) */
      minleft = setup->span.left[1];
      maxright = setup->span.right[1];
      break;

   default:
      return;
   }

   /* XXX this loop could be moved into the above switch cases and
    * calculate_mask() could be simplified a bit...
    */
   for (x = block(minleft); x <= block(maxright); x += 2) {
      emit_quad( setup, x, setup->span.y, 
                 calculate_mask( setup, x ) );
   }

   setup->span.y = 0;
   setup->span.y_flags = 0;
   setup->span.right[0] = 0;
   setup->span.right[1] = 0;
}

#if DEBUG_VERTS
static void print_vertex(const struct setup_stage *setup,
                         const struct vertex_header *v)
{
   int i;
   fprintf(stderr, "Vertex: (%p)\n", v);
   for (i = 0; i < setup->quad.nr_attrs; i++) {
      fprintf(stderr, "  %d: %f %f %f %f\n",  i, 
              v->data[i][0], v->data[i][1], v->data[i][2], v->data[i][3]);
   }
}
#endif

static boolean setup_sort_vertices( struct setup_stage *setup,
				      const struct prim_header *prim )
{
   const struct vertex_header *v0 = prim->v[0];
   const struct vertex_header *v1 = prim->v[1];
   const struct vertex_header *v2 = prim->v[2];

#if DEBUG_VERTS
   fprintf(stderr, "Triangle:\n");
   print_vertex(setup, v0);
   print_vertex(setup, v1);
   print_vertex(setup, v2);
#endif

   setup->vprovoke = v2;

   /* determine bottom to top order of vertices */
   {
      float y0 = v0->data[0][1];
      float y1 = v1->data[0][1];
      float y2 = v2->data[0][1];
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

   /* Check if triangle is completely outside the tile bounds */
   if (setup->vmin->data[0][1] > setup->cliprect_maxy)
      return FALSE;
   if (setup->vmax->data[0][1] < setup->cliprect_miny)
      return FALSE;
   if (setup->vmin->data[0][0] < setup->cliprect_minx &&
       setup->vmid->data[0][0] < setup->cliprect_minx &&
       setup->vmax->data[0][0] < setup->cliprect_minx)
      return FALSE;
   if (setup->vmin->data[0][0] > setup->cliprect_maxx &&
       setup->vmid->data[0][0] > setup->cliprect_maxx &&
       setup->vmax->data[0][0] > setup->cliprect_maxx)
      return FALSE;

   setup->ebot.dx = setup->vmid->data[0][0] - setup->vmin->data[0][0];
   setup->ebot.dy = setup->vmid->data[0][1] - setup->vmin->data[0][1];
   setup->emaj.dx = setup->vmax->data[0][0] - setup->vmin->data[0][0];
   setup->emaj.dy = setup->vmax->data[0][1] - setup->vmin->data[0][1];
   setup->etop.dx = setup->vmax->data[0][0] - setup->vmid->data[0][0];
   setup->etop.dy = setup->vmax->data[0][1] - setup->vmid->data[0][1];

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
      _mesa_printf("%s one-over-area %f  area %f  det %f\n",
                   __FUNCTION__, setup->oneoverarea, area, prim->det );
      */
   }

#if 0
   /* We need to know if this is a front or back-facing triangle for:
    *  - the GLSL gl_FrontFacing fragment attribute (bool)
    *  - two-sided stencil test
    */
   setup->quad.facing = (prim->det > 0.0) ^ (setup->softpipe->rasterizer->front_winding == PIPE_WINDING_CW);
#endif

   return TRUE;
}


#if 0
/**
 * Compute a0 for a constant-valued coefficient (GL_FLAT shading).
 * The value value comes from vertex->data[slot][i].
 * The result will be put into setup->coef[slot].a0[i].
 * \param slot  which attribute slot 
 * \param i  which component of the slot (0..3)
 */
static void const_coeff( struct setup_stage *setup,
			 unsigned slot,
			 unsigned i )
{
   assert(slot < PIPE_MAX_SHADER_INPUTS);
   assert(i <= 3);

   setup->coef[slot].dadx[i] = 0;
   setup->coef[slot].dady[i] = 0;

   /* need provoking vertex info!
    */
   setup->coef[slot].a0[i] = setup->vprovoke->data[slot][i];
}
#endif


/**
 * Compute a0, dadx and dady for a linearly interpolated coefficient,
 * for a triangle.
 */
static void tri_linear_coeff( struct setup_stage *setup,
                              uint slot, uint firstComp, uint lastComp )
{
   uint i;
   for (i = firstComp; i < lastComp; i++) {
      float botda = setup->vmid->data[slot][i] - setup->vmin->data[slot][i];
      float majda = setup->vmax->data[slot][i] - setup->vmin->data[slot][i];
      float a = setup->ebot.dy * majda - botda * setup->emaj.dy;
      float b = setup->emaj.dx * botda - majda * setup->ebot.dx;
   
      ASSERT(slot < PIPE_MAX_SHADER_INPUTS);

      setup->coef[slot].dadx[i] = a * setup->oneoverarea;
      setup->coef[slot].dady[i] = b * setup->oneoverarea;

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
      setup->coef[slot].a0[i] = (setup->vmin->data[slot][i] - 
                                 (setup->coef[slot].dadx[i] * (setup->vmin->data[0][0] - 0.5f) + 
                                  setup->coef[slot].dady[i] * (setup->vmin->data[0][1] - 0.5f)));
   }

   /*
   _mesa_printf("attr[%d].%c: %f dx:%f dy:%f\n",
		slot, "xyzw"[i], 
		setup->coef[slot].a0[i],
		setup->coef[slot].dadx[i],
		setup->coef[slot].dady[i]);
   */
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
static void tri_persp_coeff( struct setup_stage *setup,
                             unsigned slot,
                             unsigned i )
{
   /* premultiply by 1/w:
    */
   float mina = setup->vmin->data[slot][i] * setup->vmin->data[0][3];
   float mida = setup->vmid->data[slot][i] * setup->vmid->data[0][3];
   float maxa = setup->vmax->data[slot][i] * setup->vmax->data[0][3];

   float botda = mida - mina;
   float majda = maxa - mina;
   float a = setup->ebot.dy * majda - botda * setup->emaj.dy;
   float b = setup->emaj.dx * botda - majda * setup->ebot.dx;
      
   /*
   printf("tri persp %d,%d: %f %f %f\n", slot, i,
          setup->vmin->data[slot][i],
          setup->vmid->data[slot][i],
          setup->vmax->data[slot][i]
          );
   */

   assert(slot < PIPE_MAX_SHADER_INPUTS);
   assert(i <= 3);

   setup->coef[slot].dadx[i] = a * setup->oneoverarea;
   setup->coef[slot].dady[i] = b * setup->oneoverarea;
   setup->coef[slot].a0[i] = (mina - 
			    (setup->coef[slot].dadx[i] * (setup->vmin->data[0][0] - 0.5f) + 
			     setup->coef[slot].dady[i] * (setup->vmin->data[0][1] - 0.5f)));
}
#endif


/**
 * Compute the setup->coef[] array dadx, dady, a0 values.
 * Must be called after setup->vmin,vmid,vmax,vprovoke are initialized.
 */
static void setup_tri_coefficients( struct setup_stage *setup )
{
#if 0
   const enum interp_mode *interp = setup->softpipe->vertex_info.interp_mode;
   unsigned slot, j;

   /* z and w are done by linear interpolation:
    */
   tri_linear_coeff(setup, 0, 2);
   tri_linear_coeff(setup, 0, 3);

   /* setup interpolation for all the remaining attributes:
    */
   for (slot = 1; slot < setup->quad.nr_attrs; slot++) {
      switch (interp[slot]) {
      case INTERP_CONSTANT:
	 for (j = 0; j < NUM_CHANNELS; j++)
	    const_coeff(setup, slot, j);
	 break;
      
      case INTERP_LINEAR:
	 for (j = 0; j < NUM_CHANNELS; j++)
	    tri_linear_coeff(setup, slot, j);
	 break;

      case INTERP_PERSPECTIVE:
	 for (j = 0; j < NUM_CHANNELS; j++)
	    tri_persp_coeff(setup, slot, j);
	 break;

      default:
         /* invalid interp mode */
         assert(0);
      }
   }
#else
   tri_linear_coeff(setup, 0, 2, 3);  /* slot 0, z */
   tri_linear_coeff(setup, 1, 0, 4);  /* slot 1, color */
#endif
}


static void setup_tri_edges( struct setup_stage *setup )
{
   float vmin_x = setup->vmin->data[0][0] + 0.5f;
   float vmid_x = setup->vmid->data[0][0] + 0.5f;

   float vmin_y = setup->vmin->data[0][1] - 0.5f;
   float vmid_y = setup->vmid->data[0][1] - 0.5f;
   float vmax_y = setup->vmax->data[0][1] - 0.5f;

   setup->emaj.sy = CEILF(vmin_y);
   setup->emaj.lines = (int) CEILF(vmax_y - setup->emaj.sy);
   setup->emaj.dxdy = setup->emaj.dx / setup->emaj.dy;
   setup->emaj.sx = vmin_x + (setup->emaj.sy - vmin_y) * setup->emaj.dxdy;

   setup->etop.sy = CEILF(vmid_y);
   setup->etop.lines = (int) CEILF(vmax_y - setup->etop.sy);
   setup->etop.dxdy = setup->etop.dx / setup->etop.dy;
   setup->etop.sx = vmid_x + (setup->etop.sy - vmid_y) * setup->etop.dxdy;

   setup->ebot.sy = CEILF(vmin_y);
   setup->ebot.lines = (int) CEILF(vmid_y - setup->ebot.sy);
   setup->ebot.dxdy = setup->ebot.dx / setup->ebot.dy;
   setup->ebot.sx = vmin_x + (setup->ebot.sy - vmin_y) * setup->ebot.dxdy;
}


/**
 * Render the upper or lower half of a triangle.
 * Scissoring/cliprect is applied here too.
 */
static void subtriangle( struct setup_stage *setup,
			 struct edge *eleft,
			 struct edge *eright,
			 unsigned lines )
{
   const int minx = setup->cliprect_minx;
   const int maxx = setup->cliprect_maxx;
   const int miny = setup->cliprect_miny;
   const int maxy = setup->cliprect_maxy;
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
         if (block(_y) != setup->span.y) {
            flush_spans(setup);
            setup->span.y = block(_y);
         }

         setup->span.left[_y&1] = left;
         setup->span.right[_y&1] = right;
         setup->span.y_flags |= 1<<(_y&1);
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
 * Do setup for triangle rasterization, then render the triangle.
 */
static void
setup_tri(struct setup_stage *setup, struct prim_header *prim)
{
   if (!setup_sort_vertices( setup, prim )) {
      return; /* totally clipped */
   }

   setup_tri_coefficients( setup );
   setup_tri_edges( setup );

#if 0
   setup->quad.prim = PRIM_TRI;
#endif

   setup->span.y = 0;
   setup->span.y_flags = 0;
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
}



/**
 * Draw triangle into tile at (tx, ty) (tile coords)
 * The tile data should have already been fetched.
 */
void
tri_draw(const float *v0, const float *v1, const float *v2, uint tx, uint ty)
{
   struct prim_header tri;
   struct setup_stage setup;

   tri.v[0] = (struct vertex_header *) v0;
   tri.v[1] = (struct vertex_header *) v1;
   tri.v[2] = (struct vertex_header *) v2;

   setup.tx = tx;
   setup.ty = ty;

   /* set clipping bounds to tile bounds */
   setup.cliprect_minx = tx * TILE_SIZE;
   setup.cliprect_miny = ty * TILE_SIZE;
   setup.cliprect_maxx = (tx + 1) * TILE_SIZE;
   setup.cliprect_maxy = (ty + 1) * TILE_SIZE;

   setup_tri(&setup, &tri);
}
