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


#include "sp_context.h"
#include "sp_headers.h"
#include "sp_quad.h"
#include "sp_prim_setup.h"
#include "pipe/draw/draw_private.h"
#include "pipe/draw/draw_vertex.h"
#include "pipe/p_util.h"

#define DEBUG_VERTS 0

/**
 * Triangle edge info
 */
struct edge {
   float dx;			/**< X(v1) - X(v0), used only during setup */
   float dy;			/**< Y(v1) - Y(v0), used only during setup */
   float dxdy;		/**< dx/dy */
   float sx, sy;		/**< first sample point coord */
   int lines;			/**< number of lines on this edge */
};


/**
 * Triangle setup info (derived from draw_stage).
 * Also used for line drawing (taking some liberties).
 */
struct setup_stage {
   struct draw_stage stage; /**< This must be first (base class) */

   struct softpipe_context *softpipe;

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

   struct tgsi_interp_coef coef[PIPE_MAX_SHADER_INPUTS];
   struct quad_header quad; 

   struct {
      int left[2];   /**< [0] = row0, [1] = row1 */
      int right[2];
      int y;
      unsigned y_flags;
      unsigned mask;     /**< mask of MASK_BOTTOM/TOP_LEFT/RIGHT bits */
   } span;
};



/**
 * Basically a cast wrapper.
 */
static INLINE struct setup_stage *setup_stage( struct draw_stage *stage )
{
   return (struct setup_stage *)stage;
}


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


/**
 * Emit a quad (pass to next stage).  No clipping is done.
 */
static INLINE void
emit_quad( struct setup_stage *setup, int x, int y, unsigned mask )
{
   struct softpipe_context *sp = setup->softpipe;
   setup->quad.x0 = x;
   setup->quad.y0 = y;
   setup->quad.mask = mask;
   sp->quad.first->run(sp->quad.first, &setup->quad);
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
static unsigned calculate_mask( struct setup_stage *setup,
			    int x )
{
   unsigned mask = 0;

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
   case 3:
      minleft = MIN2(setup->span.left[0], setup->span.left[1]);
      maxright = MAX2(setup->span.right[0], setup->span.right[1]);
      break;

   case 1:
      minleft = setup->span.left[0];
      maxright = setup->span.right[0];
      break;

   case 2:
      minleft = setup->span.left[1];
      maxright = setup->span.right[1];
      break;

   default:
      return;
   }


   for (x = block(minleft); x <= block(maxright); )
   {
      emit_quad( setup, x, setup->span.y, 
                 calculate_mask( setup, x ) );
      x += 2;
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

   /* We need to know if this is a front or back-facing triangle for:
    *  - the GLSL gl_FrontFacing fragment attribute (bool)
    *  - two-sided stencil test
    */
   setup->quad.facing = (prim->det > 0.0) ^ (setup->softpipe->rasterizer->front_winding == PIPE_WINDING_CW);

   return TRUE;
}


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


/**
 * Compute a0, dadx and dady for a linearly interpolated coefficient,
 * for a triangle.
 */
static void tri_linear_coeff( struct setup_stage *setup,
                              unsigned slot,
                              unsigned i)
{
   float botda = setup->vmid->data[slot][i] - setup->vmin->data[slot][i];
   float majda = setup->vmax->data[slot][i] - setup->vmin->data[slot][i];
   float a = setup->ebot.dy * majda - botda * setup->emaj.dy;
   float b = setup->emaj.dx * botda - majda * setup->ebot.dx;
   
   assert(slot < PIPE_MAX_SHADER_INPUTS);
   assert(i <= 3);

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

   /*
   _mesa_printf("attr[%d].%c: %f dx:%f dy:%f\n",
		slot, "xyzw"[i], 
		setup->coef[slot].a0[i],
		setup->coef[slot].dadx[i],
		setup->coef[slot].dady[i]);
   */
}


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


/**
 * Compute the setup->coef[] array dadx, dady, a0 values.
 * Must be called after setup->vmin,vmid,vmax,vprovoke are initialized.
 */
static void setup_tri_coefficients( struct setup_stage *setup )
{
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
   const struct pipe_scissor_state *cliprect = &setup->softpipe->cliprect;
   const int minx = (int) cliprect->minx;
   const int maxx = (int) cliprect->maxx;
   const int miny = (int) cliprect->miny;
   const int maxy = (int) cliprect->maxy;
   int y, start_y, finish_y;
   int sy = (int)eleft->sy;

   assert((int)eleft->sy == (int) eright->sy);

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

         setup->span.left[_y&1] = left;setup->span.right[_y&1] = right;
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
static void setup_tri( struct draw_stage *stage,
		       struct prim_header *prim )
{
   struct setup_stage *setup = setup_stage( stage );

   /*
   _mesa_printf("%s\n", __FUNCTION__ );
   */

   setup_sort_vertices( setup, prim );
   setup_tri_coefficients( setup );
   setup_tri_edges( setup );

   setup->quad.prim = PRIM_TRI;

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
 * Compute a0, dadx and dady for a linearly interpolated coefficient,
 * for a line.
 */
static void
line_linear_coeff(struct setup_stage *setup, unsigned slot, unsigned i)
{
   const float da = setup->vmax->data[slot][i] - setup->vmin->data[slot][i];
   const float dadx = da * setup->emaj.dx * setup->oneoverarea;
   const float dady = da * setup->emaj.dy * setup->oneoverarea;
   setup->coef[slot].dadx[i] = dadx;
   setup->coef[slot].dady[i] = dady;
   setup->coef[slot].a0[i]
      = (setup->vmin->data[slot][i] - 
         (dadx * (setup->vmin->data[0][0] - 0.5f) + 
          dady * (setup->vmin->data[0][1] - 0.5f)));
}


/**
 * Compute a0, dadx and dady for a perspective-corrected interpolant,
 * for a line.
 */
static void
line_persp_coeff(struct setup_stage *setup, unsigned slot, unsigned i)
{
   /* XXX double-check/verify this arithmetic */
   const float a0 = setup->vmin->data[slot][i] * setup->vmin->data[0][3];
   const float a1 = setup->vmax->data[slot][i] * setup->vmin->data[0][3];
   const float da = a1 - a0;
   const float dadx = da * setup->emaj.dx * setup->oneoverarea;
   const float dady = da * setup->emaj.dy * setup->oneoverarea;
   setup->coef[slot].dadx[i] = dadx;
   setup->coef[slot].dady[i] = dady;
   setup->coef[slot].a0[i]
      = (setup->vmin->data[slot][i] - 
         (dadx * (setup->vmin->data[0][0] - 0.5f) + 
          dady * (setup->vmin->data[0][1] - 0.5f)));

}


/**
 * Compute the setup->coef[] array dadx, dady, a0 values.
 * Must be called after setup->vmin,vmax are initialized.
 */
static INLINE void
setup_line_coefficients(struct setup_stage *setup, struct prim_header *prim)
{
   const enum interp_mode *interp = setup->softpipe->vertex_info.interp_mode;
   unsigned slot, j;

   /* use setup->vmin, vmax to point to vertices */
   setup->vprovoke = prim->v[1];
   setup->vmin = prim->v[0];
   setup->vmax = prim->v[1];

   setup->emaj.dx = setup->vmax->data[0][0] - setup->vmin->data[0][0];
   setup->emaj.dy = setup->vmax->data[0][1] - setup->vmin->data[0][1];
   /* NOTE: this is not really 1/area */
   setup->oneoverarea = 1.0f / (setup->emaj.dx * setup->emaj.dx +
                                setup->emaj.dy * setup->emaj.dy);

   /* z and w are done by linear interpolation:
    */
   line_linear_coeff(setup, 0, 2);
   line_linear_coeff(setup, 0, 3);

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
	    line_linear_coeff(setup, slot, j);
	 break;

      case INTERP_PERSPECTIVE:
	 for (j = 0; j < NUM_CHANNELS; j++)
	    line_persp_coeff(setup, slot, j);
	 break;

      default:
         /* invalid interp mode */
         assert(0);
      }
   }
}


/**
 * Plot a pixel in a line segment.
 */
static INLINE void
plot(struct setup_stage *setup, int x, int y)
{
   const int iy = y & 1;
   const int ix = x & 1;
   const int quadX = x - ix;
   const int quadY = y - iy;
   const int mask = (1 << ix) << (2 * iy);

   if (quadX != setup->quad.x0 || 
       quadY != setup->quad.y0) 
   {
      /* flush prev quad, start new quad */

      if (setup->quad.x0 != -1)
         clip_emit_quad(setup);

      setup->quad.x0 = quadX;
      setup->quad.y0 = quadY;
      setup->quad.mask = 0x0;
   }

   setup->quad.mask |= mask;
}


/**
 * Determine whether or not to emit a line fragment by checking
 * line stipple pattern.
 */
static INLINE unsigned
stipple_test(int counter, ushort pattern, int factor)
{
   int b = (counter / factor) & 0xf;
   return (1 << b) & pattern;
}


/**
 * Do setup for line rasterization, then render the line.
 * XXX single-pixel width, no stipple, etc
 */
static void
setup_line(struct draw_stage *stage, struct prim_header *prim)
{
   const struct vertex_header *v0 = prim->v[0];
   const struct vertex_header *v1 = prim->v[1];
   struct setup_stage *setup = setup_stage( stage );
   struct softpipe_context *sp = setup->softpipe;

   int x0 = (int) v0->data[0][0];
   int x1 = (int) v1->data[0][0];
   int y0 = (int) v0->data[0][1];
   int y1 = (int) v1->data[0][1];
   int dx = x1 - x0;
   int dy = y1 - y0;
   int xstep, ystep;

   if (dx == 0 && dy == 0)
      return;

   setup_line_coefficients(setup, prim);

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

   setup->quad.x0 = setup->quad.y0 = -1;
   setup->quad.mask = 0x0;
   setup->quad.prim = PRIM_LINE;
   /* XXX temporary: set coverage to 1.0 so the line appears
    * if AA mode happens to be enabled.
    */
   setup->quad.coverage[0] =
   setup->quad.coverage[1] =
   setup->quad.coverage[2] =
   setup->quad.coverage[3] = 1.0;

   if (dx > dy) {
      /*** X-major line ***/
      int i;
      const int errorInc = dy + dy;
      int error = errorInc - dx;
      const int errorDec = error - dx;

      for (i = 0; i < dx; i++) {
         if (!sp->rasterizer->line_stipple_enable ||
             stipple_test(sp->line_stipple_counter,
                          (ushort) sp->rasterizer->line_stipple_pattern,
                          sp->rasterizer->line_stipple_factor + 1)) {
             plot(setup, x0, y0);
         }

         x0 += xstep;
         if (error < 0) {
            error += errorInc;
         }
         else {
            error += errorDec;
            y0 += ystep;
         }

         sp->line_stipple_counter++;
      }
   }
   else {
      /*** Y-major line ***/
      int i;
      const int errorInc = dx + dx;
      int error = errorInc - dy;
      const int errorDec = error - dy;

      for (i = 0; i < dy; i++) {
         if (!sp->rasterizer->line_stipple_enable ||
             stipple_test(sp->line_stipple_counter,
                          (ushort) sp->rasterizer->line_stipple_pattern,
                          sp->rasterizer->line_stipple_factor + 1)) {
            plot(setup, x0, y0);
         }

         y0 += ystep;

         if (error < 0) {
            error += errorInc;
         }
         else {
            error += errorDec;
            x0 += xstep;
         }

         sp->line_stipple_counter++;
      }
   }

   /* draw final quad */
   if (setup->quad.mask) {
      clip_emit_quad(setup);
   }
}


static void
point_persp_coeff(struct setup_stage *setup, const struct vertex_header *vert,
                  uint slot, uint i)
{
   assert(slot < PIPE_MAX_SHADER_INPUTS);
   assert(i <= 3);
   setup->coef[slot].dadx[i] = 0.0F;
   setup->coef[slot].dady[i] = 0.0F;
   setup->coef[slot].a0[i] = vert->data[slot][i] * vert->data[0][3];
}


/**
 * Do setup for point rasterization, then render the point.
 * Round or square points...
 * XXX could optimize a lot for 1-pixel points.
 */
static void
setup_point(struct draw_stage *stage, struct prim_header *prim)
{
   struct setup_stage *setup = setup_stage( stage );
   const enum interp_mode *interp = setup->softpipe->vertex_info.interp_mode;
   const struct vertex_header *v0 = prim->v[0];
   const int sizeAttr = setup->softpipe->psize_slot;
   const float size
      = sizeAttr > 0 ? v0->data[sizeAttr][0]
      : setup->softpipe->rasterizer->point_size;
   const float halfSize = 0.5F * size;
   const boolean round = (boolean) setup->softpipe->rasterizer->point_smooth;
   const float x = v0->data[0][0];  /* Note: data[0] is always position */
   const float y = v0->data[0][1];
   unsigned slot, j;

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
   setup->vprovoke = prim->v[0];
   const_coeff(setup, 0, 2);
   const_coeff(setup, 0, 3);
   for (slot = 1; slot < setup->quad.nr_attrs; slot++) {
      switch (interp[slot]) {
      case INTERP_CONSTANT:
         /* fall-through */
      case INTERP_LINEAR:
         for (j = 0; j < NUM_CHANNELS; j++)
            const_coeff(setup, slot, j);
         break;
      case INTERP_PERSPECTIVE:
         for (j = 0; j < NUM_CHANNELS; j++)
            point_persp_coeff(setup, v0, slot, j);
         break;
      default:
         assert(0);
      }
   }

   setup->quad.prim = PRIM_POINT;

   if (halfSize <= 0.5 && !round) {
      /* special case for 1-pixel points */
      const int ix = ((int) x) & 1;
      const int iy = ((int) y) & 1;
      setup->quad.x0 = (int) x - ix;
      setup->quad.y0 = (int) y - iy;
      setup->quad.mask = (1 << ix) << (2 * iy);
      clip_emit_quad(setup);
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

               setup->quad.mask = 0x0;

               dx = (ix + 0.5f) - x;
               dy = (iy + 0.5f) - y;
               dist2 = dx * dx + dy * dy;
               if (dist2 <= rmax2) {
                  cover = 1.0F - (dist2 - rmin2) * cscale;
                  setup->quad.coverage[QUAD_TOP_LEFT] = MIN2(cover, 1.0f);
                  setup->quad.mask |= MASK_TOP_LEFT;
               }

               dx = (ix + 1.5f) - x;
               dy = (iy + 0.5f) - y;
               dist2 = dx * dx + dy * dy;
               if (dist2 <= rmax2) {
                  cover = 1.0F - (dist2 - rmin2) * cscale;
                  setup->quad.coverage[QUAD_TOP_RIGHT] = MIN2(cover, 1.0f);
                  setup->quad.mask |= MASK_TOP_RIGHT;
               }

               dx = (ix + 0.5f) - x;
               dy = (iy + 1.5f) - y;
               dist2 = dx * dx + dy * dy;
               if (dist2 <= rmax2) {
                  cover = 1.0F - (dist2 - rmin2) * cscale;
                  setup->quad.coverage[QUAD_BOTTOM_LEFT] = MIN2(cover, 1.0f);
                  setup->quad.mask |= MASK_BOTTOM_LEFT;
               }

               dx = (ix + 1.5f) - x;
               dy = (iy + 1.5f) - y;
               dist2 = dx * dx + dy * dy;
               if (dist2 <= rmax2) {
                  cover = 1.0F - (dist2 - rmin2) * cscale;
                  setup->quad.coverage[QUAD_BOTTOM_RIGHT] = MIN2(cover, 1.0f);
                  setup->quad.mask |= MASK_BOTTOM_RIGHT;
               }

               if (setup->quad.mask) {
                  setup->quad.x0 = ix;
                  setup->quad.y0 = iy;
                  clip_emit_quad(setup);
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
         printf("(%f, %f) -> X:%d..%d Y:%d..%d\n", x, y, xmin, xmax,ymin,ymax);
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
                  
               setup->quad.mask = mask;
               setup->quad.x0 = ix;
               setup->quad.y0 = iy;
               clip_emit_quad(setup);
            }
         }
      }
   }
}



static void setup_begin( struct draw_stage *stage )
{
   struct setup_stage *setup = setup_stage(stage);
   struct softpipe_context *sp = setup->softpipe;

   setup->quad.nr_attrs = setup->softpipe->nr_frag_attrs;

   sp->quad.first->begin(sp->quad.first);
}


static void setup_end( struct draw_stage *stage )
{
}


static void reset_stipple_counter( struct draw_stage *stage )
{
   struct setup_stage *setup = setup_stage(stage);
   setup->softpipe->line_stipple_counter = 0;
}


/**
 * Create a new primitive setup/render stage.
 */
struct draw_stage *sp_draw_render_stage( struct softpipe_context *softpipe )
{
   struct setup_stage *setup = CALLOC_STRUCT(setup_stage);

   setup->softpipe = softpipe;
   setup->stage.draw = softpipe->draw;
   setup->stage.begin = setup_begin;
   setup->stage.point = setup_point;
   setup->stage.line = setup_line;
   setup->stage.tri = setup_tri;
   setup->stage.end = setup_end;
   setup->stage.reset_stipple_counter = reset_stipple_counter;

   setup->quad.coef = setup->coef;

   return &setup->stage;
}


/* Recalculate det.  This is only used in the test harness below:
 */
static void calc_det( struct prim_header *header )
{
   /* Window coords: */
   const float *v0 = header->v[0]->data[0];
   const float *v1 = header->v[1]->data[0];
   const float *v2 = header->v[2]->data[0];

   /* edge vectors e = v0 - v2, f = v1 - v2 */
   const float ex = v0[0] - v2[0];
   const float ey = v0[1] - v2[1];
   const float fx = v1[0] - v2[0];
   const float fy = v1[1] - v2[1];
   
   /* det = cross(e,f).z */
   header->det = ex * fy - ey * fx;
}



/* Test harness - feed vertex buffer back into prim pipeline.
 *
 * The big issue at this point is that reset_stipple doesn't make it
 * through the interface.  Probably need to split primitives at reset
 * stipple, perhaps using the ~0 index marker.
 */
void sp_vbuf_setup_draw( struct pipe_context *pipe,
                         unsigned primitive,
                         const ushort *elements,
                         unsigned nr_elements,
                         const void *vertex_buffer,
                         unsigned nr_vertices )
{
   struct softpipe_context *softpipe = softpipe_context( pipe );
   struct setup_stage *setup = setup_stage( softpipe->setup );
   struct prim_header prim;
   unsigned vertex_size = setup->stage.draw->vertex_info.size * sizeof(float);
   unsigned i, j;

   prim.det = 0;
   prim.reset_line_stipple = 0;
   prim.edgeflags = 0;
   prim.pad = 0;

   setup->stage.begin( &setup->stage );

   switch (primitive) {
   case PIPE_PRIM_TRIANGLES:
      for (i = 0; i < nr_elements; i += 3) {
         for (j = 0; j < 3; j++) 
            prim.v[j] = (struct vertex_header *)((char *)vertex_buffer + 
                                                 elements[i+j] * vertex_size);
         
         calc_det(&prim);
         setup->stage.tri( &setup->stage, &prim );
      }
      break;

   case PIPE_PRIM_LINES:
      for (i = 0; i < nr_elements; i += 2) {
         for (j = 0; j < 2; j++) 
            prim.v[j] = (struct vertex_header *)((char *)vertex_buffer + 
                                                 elements[i+j] * vertex_size);
         
         setup->stage.line( &setup->stage, &prim );
      }
      break;

   case PIPE_PRIM_POINTS:
      for (i = 0; i < nr_elements; i += 2) {
         prim.v[i] = (struct vertex_header *)((char *)vertex_buffer + 
                                              elements[i] * vertex_size);         
         setup->stage.point( &setup->stage, &prim );
      }
      break;
   }

   setup->stage.end( &setup->stage );
}
