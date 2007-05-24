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

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */

#include "imports.h"
#include "macros.h"

#include "g_context.h"
#include "g_prim.h"
#include "g_tile.h"


struct edge {
   GLfloat dx;			/* X(v1) - X(v0), used only during setup */
   GLfloat dy;			/* Y(v1) - Y(v0), used only during setup */
   GLfloat dxdy;		/* dx/dy */
   GLfloat sx;			/* first sample point x coord */
   GLfloat sy;
   GLint lines;			/* number of lines  on this edge */
};


struct setup_stage {
   struct prim_stage stage;
   
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
      GLint left[2];
      GLint right[2];
      GLint y;
      GLuint y_flags;
      GLuint mask;
   } span;

};




static inline struct setup_stage *setup_stage( struct prim_stage *stage )
{
   return (struct setup_stage *)stage;
}



static inline GLint _min(GLint x, GLint y) 
{
   return x < y ? x : y;
}

static inline GLint _max(GLint x, GLint y) 
{
   return x > y ? x : y;
}

static inline GLint block( GLint x )
{
   return x & ~1;
}



static void setup_begin( struct prim_stage *stage )
{
   setup_stage(stage)->quad.nr_attrs = stage->generic->nr_frag_attrs;
}



static void run_shader_block( struct setup_stage *setup, 
			      GLint x, GLint y, GLuint mask )
{
   setup->quad.x0 = x;
   setup->quad.y0 = y;
   setup->quad.mask = mask;

   quad_shade( setup->stage.generic, &setup->quad );
}


/* this is pretty nasty...  may need to rework flush_spans again to
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


static void flush_spans( struct setup_stage *setup )
{
   GLint minleft, maxright;
   GLint x;

   switch (setup->span.y_flags) {      
   case 3:
      minleft = _min(setup->span.left[0],
		    setup->span.left[1]);
      maxright = _max(setup->span.right[0],
		     setup->span.right[1]);
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


     
static void
setup_point( struct prim_stage *stage, 
	     struct prim_header *header )
{
}


static void
setup_line( struct prim_stage *stage,
	    struct prim_header *header )
{
}






static GLboolean setup_sort_vertices( struct setup_stage *setup,
				      const struct prim_header *prim )
{
   
   const struct vertex_header *v0 = prim->v[0];
   const struct vertex_header *v1 = prim->v[1];
   const struct vertex_header *v2 = prim->v[2];

   setup->vprovoke = v2;
//   setup->oneoverarea = -1.0 / prim->det;

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
//	    setup->oneoverarea = -setup->oneoverarea;
	 }
      }
      else {
	 if (y0 <= y2) {
	    /* y1<=y0<=y2 */
	    setup->vmin = v1;   
	    setup->vmid = v0;   
	    setup->vmax = v2;  
//	    setup->oneoverarea = -setup->oneoverarea;
	 }
	 else if (y2 <= y1) {
	    /* y2<=y1<=y0 */
	    setup->vmin = v2;   
	    setup->vmid = v1;   
	    setup->vmax = v0;  
//	    setup->oneoverarea = -setup->oneoverarea;
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

   /* xxx: may need to adjust this sign according to the if-tree
    * above:
    *
    * XXX: this is like 'det', but calculated from screen coords??
    */
   {
      const GLfloat area = (setup->emaj.dx * setup->ebot.dy - 
			    setup->ebot.dx * setup->emaj.dy);

      setup->oneoverarea = 1.0 / area;
   }


   _mesa_printf("%s one-over-area %f\n", __FUNCTION__, setup->oneoverarea );


   return GL_TRUE;
}


static void const_coeff( struct setup_stage *setup,
			 GLuint slot,
			 GLuint i )
{
   setup->coef[slot].dadx[i] = 0;
   setup->coef[slot].dady[i] = 0;

   /* need provoking vertex info!
    */
   setup->coef[slot].a0[i] = setup->vprovoke->data[slot][i];
}


static void linear_coeff( struct setup_stage *setup,
			  GLuint slot,
			  GLuint i)
{
   GLfloat botda = setup->vmid->data[slot][i] - setup->vmin->data[slot][i];
   GLfloat majda = setup->vmax->data[slot][i] - setup->vmin->data[slot][i];
   GLfloat a = setup->ebot.dy * majda - botda * setup->emaj.dy;
   GLfloat b = setup->emaj.dx * botda - majda * setup->ebot.dx;
   
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

   _mesa_printf("attr[%d].%c: %f dx:%f dy:%f\n",
		slot, "xyzw"[i], 
		setup->coef[slot].a0[i],
		setup->coef[slot].dadx[i],
		setup->coef[slot].dady[i]);
}


static void persp_coeff( struct setup_stage *setup,
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
      
   setup->coef[slot].dadx[i] = a * setup->oneoverarea;
   setup->coef[slot].dady[i] = b * setup->oneoverarea;
   setup->coef[slot].a0[i] = (mina - 
			    (setup->coef[slot].dadx[i] * (setup->vmin->data[0][0] - 0.5) + 
			     setup->coef[slot].dady[i] * (setup->vmin->data[0][1] - 0.5)));
}




static void setup_coefficients( struct setup_stage *setup )
{
   const enum interp_mode *interp = setup->stage.generic->interp;
   GLuint slot, j;

   /* z and w are done by linear interpolation:
    */
   linear_coeff(setup, 0, 2);
   linear_coeff(setup, 0, 3);

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
	    linear_coeff(setup, slot, j);
	 break;

      case INTERP_PERSPECTIVE:
	 for (j = 0; j < NUM_CHANNELS; j++)
	    persp_coeff(setup, slot, j);
	 break;
      }
   }
}



static void setup_edges( struct setup_stage *setup )
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
   if (setup->stage.generic->setup.scissor) {
      start_y = sy;
      finish_y = start_y + lines;

      if (start_y < setup->stage.generic->scissor.miny) 
	 start_y = setup->stage.generic->scissor.miny;

      if (finish_y > setup->stage.generic->scissor.maxy) 
	 finish_y = setup->stage.generic->scissor.maxy;

      start_y -= sy;
      finish_y -= sy;
   }
   else {
      start_y = 0;
      finish_y = lines;
   }

   _mesa_printf("%s %d %d\n", __FUNCTION__, start_y, finish_y);  

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
      if (setup->stage.generic->setup.scissor) {
	 if (left  < setup->stage.generic->scissor.minx) 
	    left  = setup->stage.generic->scissor.minx;

	 if (right > setup->stage.generic->scissor.maxx) 
	    right = setup->stage.generic->scissor.maxx;
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


static void setup_tri( struct prim_stage *stage,
		       struct prim_header *prim )
{
   struct setup_stage *setup = setup_stage( stage );

   _mesa_printf("%s\n", __FUNCTION__ );

   setup_sort_vertices( setup, prim );
   setup_coefficients( setup );
   setup_edges( setup );

   setup->span.y = 0;
   setup->span.y_flags = 0;
   setup->span.right[0] = 0;
   setup->span.right[1] = 0;
//   setup->span.z_mode = tri_z_mode( setup->ctx );

//   init_constant_attribs( setup );
      
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

static void setup_end( struct prim_stage *stage )
{
}


struct prim_stage *prim_setup( struct generic_context *generic )
{
   struct setup_stage *setup = CALLOC_STRUCT(setup_stage);

   setup->stage.generic = generic;
   setup->stage.begin = setup_begin;
   setup->stage.point = setup_point;
   setup->stage.line = setup_line;
   setup->stage.tri = setup_tri;
   setup->stage.end = setup_end;

   setup->quad.coef = setup->coef;

   return &setup->stage;
}
