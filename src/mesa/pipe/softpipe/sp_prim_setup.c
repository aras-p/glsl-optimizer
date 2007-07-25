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


#include "imports.h"
#include "macros.h"

#include "sp_context.h"
#include "sp_headers.h"
#include "pipe/draw/draw_private.h"
#include "sp_quad.h"
#include "sp_prim_setup.h"



/**
 * Emit/render a quad.
 * This passes the quad to the first stage of per-fragment operations.
 */
static INLINE void
quad_emit(struct softpipe_context *sp, struct quad_header *quad)
{
   sp->quad.first->run(sp->quad.first, quad);
}


/**
 * Triangle edge info
 */
struct edge {
   GLfloat dx;			/* X(v1) - X(v0), used only during setup */
   GLfloat dy;			/* Y(v1) - Y(v0), used only during setup */
   GLfloat dxdy;		/* dx/dy */
   GLfloat sx;			/* first sample point x coord */
   GLfloat sy;
   GLint lines;			/* number of lines  on this edge */
};


/**
 * Triangle setup info (derived from draw_stage).
 * Also used for line drawing (taking some liberties).
 */
struct setup_stage {
   struct draw_stage stage; /**< This must be first (base class) */

   /*XXX NEW */
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

   GLfloat oneoverarea;

   struct setup_coefficient coef[FRAG_ATTRIB_MAX];
   struct quad_header quad; 

   struct {
      GLint left[2];   /**< [0] = row0, [1] = row1 */
      GLint right[2];
      GLint y;
      GLuint y_flags;
      GLuint mask;     /**< mask of MASK_BOTTOM/TOP_LEFT/RIGHT bits */
   } span;
};



/**
 * Basically a cast wrapper.
 */
static inline struct setup_stage *setup_stage( struct draw_stage *stage )
{
   return (struct setup_stage *)stage;
}


/**
 * Given an X or Y coordinate, return the block/quad coordinate that it
 * belongs to.
 */
static inline GLint block( GLint x )
{
   return x & ~1;
}



/**
 * Run shader on a quad/block.
 */
static void run_shader_block( struct setup_stage *setup, 
			      GLint x, GLint y, GLuint mask )
{
   setup->quad.x0 = x;
   setup->quad.y0 = y;
   setup->quad.mask = mask;

   quad_emit(setup->softpipe, &setup->quad);
}


/**
 * Compute mask which indicates which pixels in the 2x2 quad are actually inside
 * the triangle's bounds.
 *
 * this is pretty nasty...  may need to rework flush_spans again to
 * fix it, if possible.
 */
static GLuint calculate_mask( struct setup_stage *setup,
			    GLint x )
{
   GLuint mask = 0;

   if (x >= setup->span.left[0] && x < setup->span.right[0]) 
      mask |= MASK_BOTTOM_LEFT;

   if (x >= setup->span.left[1] && x < setup->span.right[1]) 
      mask |= MASK_TOP_LEFT;
      
   if (x+1 >= setup->span.left[0] && x+1 < setup->span.right[0]) 
      mask |= MASK_BOTTOM_RIGHT;

   if (x+1 >= setup->span.left[1] && x+1 < setup->span.right[1]) 
      mask |= MASK_TOP_RIGHT;

   return mask;
}


/**
 * Render a horizontal span of quads
 */
static void flush_spans( struct setup_stage *setup )
{
   GLint minleft, maxright;
   GLint x;

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
      run_shader_block( setup, x,
			setup->span.y, 
			calculate_mask( setup, x ) );
      x += 2;
   }

   setup->span.y = 0;
   setup->span.y_flags = 0;
   setup->span.right[0] = 0;
   setup->span.right[1] = 0;
}


static GLboolean setup_sort_vertices( struct setup_stage *setup,
				      const struct prim_header *prim )
{
   const struct vertex_header *v0 = prim->v[0];
   const struct vertex_header *v1 = prim->v[1];
   const struct vertex_header *v2 = prim->v[2];

   setup->vprovoke = v2;

   /* determine bottom to top order of vertices */
   {
      GLfloat y0 = v0->data[0][1];
      GLfloat y1 = v1->data[0][1];
      GLfloat y2 = v2->data[0][1];
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
      const GLfloat area = (setup->emaj.dx * setup->ebot.dy - 
			    setup->ebot.dx * setup->emaj.dy);

      setup->oneoverarea = 1.0 / area;
      /*
      _mesa_printf("%s one-over-area %f  area %f  det %f\n",
                   __FUNCTION__, setup->oneoverarea, area, prim->det );
      */
   }

   /* We need to know if this is a front or back-facing triangle for:
    *  - the GLSL gl_FrontFacing fragment attribute (bool)
    *  - two-sided stencil test
    */
   setup->quad.facing = (prim->det > 0.0) ^ (setup->softpipe->setup.front_winding == PIPE_WINDING_CW);

   return GL_TRUE;
}


/**
 * Compute a0 for a constant-valued coefficient (GL_FLAT shading).
 * The value value comes from vertex->data[slot][i].
 * The result will be put into setup->coef[slot].a0[i].
 * \param slot  which attribute slot 
 * \param i  which component of the slot (0..3)
 */
static void const_coeff( struct setup_stage *setup,
			 GLuint slot,
			 GLuint i )
{
   assert(slot < FRAG_ATTRIB_MAX);
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
                              GLuint slot,
                              GLuint i)
{
   GLfloat botda = setup->vmid->data[slot][i] - setup->vmin->data[slot][i];
   GLfloat majda = setup->vmax->data[slot][i] - setup->vmin->data[slot][i];
   GLfloat a = setup->ebot.dy * majda - botda * setup->emaj.dy;
   GLfloat b = setup->emaj.dx * botda - majda * setup->ebot.dx;
   
   assert(slot < FRAG_ATTRIB_MAX);
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
			    (setup->coef[slot].dadx[i] * (setup->vmin->data[0][0] - 0.5) + 
			     setup->coef[slot].dady[i] * (setup->vmin->data[0][1] - 0.5)));

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
 */
static void tri_persp_coeff( struct setup_stage *setup,
                             GLuint slot,
                             GLuint i )
{
   /* premultiply by 1/w:
    */
   GLfloat mina = setup->vmin->data[slot][i] * setup->vmin->data[0][3];
   GLfloat mida = setup->vmid->data[slot][i] * setup->vmid->data[0][3];
   GLfloat maxa = setup->vmax->data[slot][i] * setup->vmax->data[0][3];

   GLfloat botda = mida - mina;
   GLfloat majda = maxa - mina;
   GLfloat a = setup->ebot.dy * majda - botda * setup->emaj.dy;
   GLfloat b = setup->emaj.dx * botda - majda * setup->ebot.dx;
      
   assert(slot < FRAG_ATTRIB_MAX);
   assert(i <= 3);

   setup->coef[slot].dadx[i] = a * setup->oneoverarea;
   setup->coef[slot].dady[i] = b * setup->oneoverarea;
   setup->coef[slot].a0[i] = (mina - 
			    (setup->coef[slot].dadx[i] * (setup->vmin->data[0][0] - 0.5) + 
			     setup->coef[slot].dady[i] * (setup->vmin->data[0][1] - 0.5)));
}



/**
 * Compute the setup->coef[] array dadx, dady, a0 values.
 * Must be called after setup->vmin,vmid,vmax,vprovoke are initialized.
 */
static void setup_tri_coefficients( struct setup_stage *setup )
{
   const enum interp_mode *interp = setup->softpipe->interp;
   GLuint slot, j;

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
      }
   }
}



static void setup_tri_edges( struct setup_stage *setup )
{
   GLfloat vmin_x = setup->vmin->data[0][0] + 0.5;
   GLfloat vmid_x = setup->vmid->data[0][0] + 0.5;

   GLfloat vmin_y = setup->vmin->data[0][1] - 0.5;
   GLfloat vmid_y = setup->vmid->data[0][1] - 0.5;
   GLfloat vmax_y = setup->vmax->data[0][1] - 0.5;

   setup->emaj.sy = ceilf(vmin_y);
   setup->emaj.lines = (GLint) ceilf(vmax_y - setup->emaj.sy);
   setup->emaj.dxdy = setup->emaj.dx / setup->emaj.dy;
   setup->emaj.sx = vmin_x + (setup->emaj.sy - vmin_y) * setup->emaj.dxdy;

   setup->etop.sy = ceilf(vmid_y);
   setup->etop.lines = (GLint) ceilf(vmax_y - setup->etop.sy);
   setup->etop.dxdy = setup->etop.dx / setup->etop.dy;
   setup->etop.sx = vmid_x + (setup->etop.sy - vmid_y) * setup->etop.dxdy;

   setup->ebot.sy = ceilf(vmin_y);
   setup->ebot.lines = (GLint) ceilf(vmid_y - setup->ebot.sy);
   setup->ebot.dxdy = setup->ebot.dx / setup->ebot.dy;
   setup->ebot.sx = vmin_x + (setup->ebot.sy - vmin_y) * setup->ebot.dxdy;
}


/**
 * Render the upper or lower half of a triangle.
 * Scissoring is applied here too.
 */
static void subtriangle( struct setup_stage *setup,
			 struct edge *eleft,
			 struct edge *eright,
			 GLuint lines )
{
   GLint y, start_y, finish_y;
   GLint sy = (GLint)eleft->sy;

   assert((GLint)eleft->sy == (GLint) eright->sy);
   assert((GLint)eleft->sy >= 0);	/* catch bug in x64? */

   /* scissor y:
    */
   if (setup->softpipe->setup.scissor) {
      start_y = sy;
      finish_y = start_y + lines;

      if (start_y < setup->softpipe->scissor.miny) 
	 start_y = setup->softpipe->scissor.miny;

      if (finish_y > setup->softpipe->scissor.maxy) 
	 finish_y = setup->softpipe->scissor.maxy;

      start_y -= sy;
      finish_y -= sy;
   }
   else {
      start_y = 0;
      finish_y = lines;
   }

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
      GLint left = (GLint)(eleft->sx + y * eleft->dxdy);
      GLint right = (GLint)(eright->sx + y * eright->dxdy);

      /* scissor x: 
       */
      if (setup->softpipe->setup.scissor) {
	 if (left  < setup->softpipe->scissor.minx) 
	    left  = setup->softpipe->scissor.minx;

	 if (right > setup->softpipe->scissor.maxx) 
	    right = setup->softpipe->scissor.maxx;
      }

      if (left < right) {
	 GLint _y = sy+y;
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
line_linear_coeff(struct setup_stage *setup, GLuint slot, GLuint i)
{
   const GLfloat dz = setup->vmax->data[slot][i] - setup->vmin->data[slot][i];
   const GLfloat dadx = dz * setup->emaj.dx * setup->oneoverarea;
   const GLfloat dady = dz * setup->emaj.dy * setup->oneoverarea;
   setup->coef[slot].dadx[i] = dadx;
   setup->coef[slot].dady[i] = dady;
   setup->coef[slot].a0[i]
      = (setup->vmin->data[slot][i] - 
         (dadx * (setup->vmin->data[0][0] - 0.5) + 
          dady * (setup->vmin->data[0][1] - 0.5)));
}


/**
 * Compute a0, dadx and dady for a perspective-corrected interpolant,
 * for a line.
 */
static void
line_persp_coeff(struct setup_stage *setup, GLuint slot, GLuint i)
{
   /* XXX to do */
   line_linear_coeff(setup, slot, i); /* XXX temporary */
}


/**
 * Compute the setup->coef[] array dadx, dady, a0 values.
 * Must be called after setup->vmin,vmax are initialized.
 */
static INLINE void
setup_line_coefficients(struct setup_stage *setup, struct prim_header *prim)
{
   const enum interp_mode *interp = setup->softpipe->interp;
   GLuint slot, j;

   /* use setup->vmin, vmax to point to vertices */
   setup->vprovoke = prim->v[1];
   setup->vmin = prim->v[0];
   setup->vmax = prim->v[1];

   setup->emaj.dx = setup->vmax->data[0][0] - setup->vmin->data[0][0];
   setup->emaj.dy = setup->vmax->data[0][1] - setup->vmin->data[0][1];
   /* NOTE: this is not really 1/area */
   setup->oneoverarea = 1.0 / (setup->emaj.dx * setup->emaj.dx +
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
      }
   }
}


/**
 * Plot a pixel in a line segment.
 */
static INLINE void
plot(struct setup_stage *setup, GLint x, GLint y)
{
   const GLint iy = y & 1;
   const GLint ix = x & 1;
   const GLint quadX = x - ix;
   const GLint quadY = y - iy;
   const GLint mask = (1 << ix) << (2 * iy);

   if (quadX != setup->quad.x0 || 
       quadY != setup->quad.y0) 
   {
      /* flush prev quad, start new quad */

      if (setup->quad.x0 != -1) 
	 quad_emit(setup->softpipe, &setup->quad);

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
static INLINE GLuint
stipple_test(GLint counter, GLushort pattern, GLint factor)
{
   GLint b = (counter / factor) & 0xf;
   return (1 << b) & pattern;
}


/**
 * Do setup for line rasterization, then render the line.
 * XXX single-pixel width, no stipple, etc
 * XXX no scissoring yet.
 */
static void
setup_line(struct draw_stage *stage, struct prim_header *prim)
{
   const struct vertex_header *v0 = prim->v[0];
   const struct vertex_header *v1 = prim->v[1];
   struct setup_stage *setup = setup_stage( stage );
   struct softpipe_context *sp = setup->softpipe;

   GLint x0 = (GLint) v0->data[0][0];
   GLint x1 = (GLint) v1->data[0][0];
   GLint y0 = (GLint) v0->data[0][1];
   GLint y1 = (GLint) v1->data[0][1];
   GLint dx = x1 - x0;
   GLint dy = y1 - y0;
   GLint xstep, ystep;

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

   if (dx > dy) {
      /*** X-major line ***/
      GLint i;
      const GLint errorInc = dy + dy;
      GLint error = errorInc - dx;
      const GLint errorDec = error - dx;

      for (i = 0; i < dx; i++) {
         if (!sp->setup.line_stipple_enable ||
             stipple_test(sp->line_stipple_counter,
                          sp->setup.line_stipple_pattern,
                          sp->setup.line_stipple_factor + 1)) {
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
      GLint i;
      const GLint errorInc = dx + dx;
      GLint error = errorInc - dy;
      const GLint errorDec = error - dy;

      for (i = 0; i < dy; i++) {
         if (!sp->setup.line_stipple_enable ||
             stipple_test(sp->line_stipple_counter,
                          sp->setup.line_stipple_pattern,
                          sp->setup.line_stipple_factor + 1)) {
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
      quad_emit(setup->softpipe, &setup->quad);
   }
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
   /*XXX this should be a vertex attrib! */
   GLfloat halfSize = 0.5 * setup->softpipe->setup.point_size;
   GLboolean round = setup->softpipe->setup.point_smooth;
   const struct vertex_header *v0 = prim->v[0];
   const GLfloat x = v0->data[FRAG_ATTRIB_WPOS][0];
   const GLfloat y = v0->data[FRAG_ATTRIB_WPOS][1];
   GLuint slot, j;

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
      for (j = 0; j < NUM_CHANNELS; j++)
         const_coeff(setup, slot, j);
   }

   setup->quad.prim = PRIM_POINT;

   /* XXX need to clip against scissor bounds too */

   if (halfSize <= 0.5 && !round) {
      /* special case for 1-pixel points */
      const GLint ix = ((GLint) x) & 1;
      const GLint iy = ((GLint) y) & 1;
      setup->quad.x0 = x - ix;
      setup->quad.y0 = y - iy;
      setup->quad.mask = (1 << ix) << (2 * iy);
      quad_emit(setup->softpipe, &setup->quad);
   }
   else {
      const GLint ixmin = block((GLint) (x - halfSize));
      const GLint ixmax = block((GLint) (x + halfSize));
      const GLint iymin = block((GLint) (y - halfSize));
      const GLint iymax = block((GLint) (y + halfSize));
      GLint ix, iy;

      if (round) {
         /* rounded points */
         const GLfloat rmin = halfSize - 0.7071F;  /* 0.7071 = sqrt(2)/2 */
         const GLfloat rmax = halfSize + 0.7071F;
         const GLfloat rmin2 = MAX2(0.0F, rmin * rmin);
         const GLfloat rmax2 = rmax * rmax;
         const GLfloat cscale = 1.0F / (rmax2 - rmin2);

         for (iy = iymin; iy <= iymax; iy += 2) {
            for (ix = ixmin; ix <= ixmax; ix += 2) {
               GLfloat dx, dy, dist2, cover;

               setup->quad.mask = 0x0;

               dx = (ix + 0.5) - x;
               dy = (iy + 0.5) - y;
               dist2 = dx * dx + dy * dy;
               if (dist2 <= rmax2) {
                  cover = 1.0F - (dist2 - rmin2) * cscale;
                  setup->quad.coverage[QUAD_BOTTOM_LEFT] = MIN2(cover, 1.0);
                  setup->quad.mask |= MASK_BOTTOM_LEFT;
               }

               dx = (ix + 1.5) - x;
               dy = (iy + 0.5) - y;
               dist2 = dx * dx + dy * dy;
               if (dist2 <= rmax2) {
                  cover = 1.0F - (dist2 - rmin2) * cscale;
                  setup->quad.coverage[QUAD_BOTTOM_RIGHT] = MIN2(cover, 1.0);
                  setup->quad.mask |= MASK_BOTTOM_RIGHT;
               }

               dx = (ix + 0.5) - x;
               dy = (iy + 1.5) - y;
               dist2 = dx * dx + dy * dy;
               if (dist2 <= rmax2) {
                  cover = 1.0F - (dist2 - rmin2) * cscale;
                  setup->quad.coverage[QUAD_TOP_LEFT] = MIN2(cover, 1.0);
                  setup->quad.mask |= MASK_TOP_LEFT;
               }

               dx = (ix + 1.5) - x;
               dy = (iy + 1.5) - y;
               dist2 = dx * dx + dy * dy;
               if (dist2 <= rmax2) {
                  cover = 1.0F - (dist2 - rmin2) * cscale;
                  setup->quad.coverage[QUAD_TOP_RIGHT] = MIN2(cover, 1.0);
                  setup->quad.mask |= MASK_TOP_RIGHT;
               }

               if (setup->quad.mask) {
                  setup->quad.x0 = ix;
                  setup->quad.y0 = iy;
                  quad_emit( setup->softpipe, &setup->quad );
               }
            }
         }
      }
      else {
         /* square points */
         for (iy = iymin; iy <= iymax; iy += 2) {
            for (ix = ixmin; ix <= ixmax; ix += 2) {
               setup->quad.mask = 0xf;

               if (ix + 0.5 < x - halfSize) {
                  /* fragment is past left edge of point, turn off left bits */
                  setup->quad.mask &= ~(MASK_BOTTOM_LEFT | MASK_TOP_LEFT);
               }

               if (ix + 1.5 > x + halfSize) {
                  /* past the right edge */
                  setup->quad.mask &= ~(MASK_BOTTOM_RIGHT | MASK_TOP_RIGHT);
               }

               if (iy + 0.5 < y - halfSize) {
                  /* below the bottom edge */
                  setup->quad.mask &= ~(MASK_BOTTOM_LEFT | MASK_BOTTOM_RIGHT);
               }

               if (iy + 1.5 > y + halfSize) {
                  /* above the top edge */
                  setup->quad.mask &= ~(MASK_TOP_LEFT | MASK_TOP_RIGHT);
               }

               if (setup->quad.mask) {
                  setup->quad.x0 = ix;
                  setup->quad.y0 = iy;
                  quad_emit( setup->softpipe, &setup->quad );
               }
            }
         }
      }
   }
}



static void setup_begin( struct draw_stage *stage )
{
   struct setup_stage *setup = setup_stage(stage);

   setup->quad.nr_attrs = setup->softpipe->nr_frag_attrs;

   /*
    * XXX this is where we might map() the renderbuffers to begin
    * s/w rendering.
    */
}


static void setup_end( struct draw_stage *stage )
{
   /*
    * XXX this is where we might unmap() the renderbuffers after
    * s/w rendering.
    */
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
