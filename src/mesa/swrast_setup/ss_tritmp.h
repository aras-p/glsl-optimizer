/* $Id: ss_tritmp.h,v 1.12 2001/04/28 08:39:18 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keithw@valinux.com>
 */


static void TAG(triangle)(GLcontext *ctx, GLuint e0, GLuint e1, GLuint e2 )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   SWvertex *verts = SWSETUP_CONTEXT(ctx)->verts;
   SWvertex *v[3];
   GLfloat z[3];
   GLfloat offset;
   GLenum mode = GL_FILL;
   GLuint facing;

   v[0] = &verts[e0];
   v[1] = &verts[e1];
   v[2] = &verts[e2];


   if (IND & (SS_TWOSIDE_BIT | SS_OFFSET_BIT | SS_UNFILLED_BIT))
   {
      GLfloat ex = v[0]->win[0] - v[2]->win[0];
      GLfloat ey = v[0]->win[1] - v[2]->win[1];
      GLfloat fx = v[1]->win[0] - v[2]->win[0];
      GLfloat fy = v[1]->win[1] - v[2]->win[1];
      GLfloat cc  = ex*fy - ey*fx;

      if (IND & (SS_TWOSIDE_BIT | SS_UNFILLED_BIT))
      {
	 facing = (cc < 0.0) ^ ctx->Polygon._FrontBit;

	 if (IND & SS_UNFILLED_BIT)
	    mode = facing ? ctx->Polygon.BackMode : ctx->Polygon.FrontMode;

	 if (facing == 1) {
	    if (IND & SS_TWOSIDE_BIT) {
	       if (IND & SS_RGBA_BIT) {
		  GLfloat (*vbcolor)[4] = (GLfloat (*)[4])VB->ColorPtr[1]->Ptr;
		  GLfloat (*vbspec)[4] = (GLfloat (*)[4])VB->SecondaryColorPtr[1]->Ptr;
		  SS_COLOR(v[0]->color, vbcolor[e0]);
		  SS_COLOR(v[1]->color, vbcolor[e1]);
		  SS_COLOR(v[2]->color, vbcolor[e2]);
		  SS_SPEC(v[0]->specular, vbspec[e0]);
		  SS_SPEC(v[1]->specular, vbspec[e1]);
		  SS_SPEC(v[2]->specular, vbspec[e2]);
	       } else {
		  GLuint *vbindex = VB->IndexPtr[1]->data;
		  SS_IND(v[0]->index, vbindex[e0]);
		  SS_IND(v[1]->index, vbindex[e1]);
		  SS_IND(v[2]->index, vbindex[e2]);
	       }
	    }
	 }
      }

      if (IND & SS_OFFSET_BIT)
      {
	 offset = ctx->Polygon.OffsetUnits;
	 z[0] = v[0]->win[2];
	 z[1] = v[1]->win[2];
	 z[2] = v[2]->win[2];
	 if (cc * cc > 1e-16) {
	    GLfloat ez = z[0] - z[2];
	    GLfloat fz = z[1] - z[2];
	    GLfloat a = ey*fz - ez*fy;
	    GLfloat b = ez*fx - ex*fz;
	    GLfloat ic = 1.0 / cc;
	    GLfloat ac = a * ic;
	    GLfloat bc = b * ic;
	    if (ac < 0.0f) ac = -ac;
	    if (bc < 0.0f) bc = -bc;
	    offset += MAX2(ac, bc) * ctx->Polygon.OffsetFactor;
	 }
         offset *= ctx->MRD;
         /*printf("offset %g\n", offset);*/
      }
   }

   if (mode == GL_POINT) {
      if ((IND & SS_OFFSET_BIT) && ctx->Polygon.OffsetPoint) {
	 v[0]->win[2] += offset;
	 v[1]->win[2] += offset;
	 v[2]->win[2] += offset;
      }
      _swsetup_render_point_tri( ctx, e0, e1, e2 );
   } else if (mode == GL_LINE) {
      if ((IND & SS_OFFSET_BIT) && ctx->Polygon.OffsetLine) {
	 v[0]->win[2] += offset;
	 v[1]->win[2] += offset;
	 v[2]->win[2] += offset;
      }
      _swsetup_render_line_tri( ctx, e0, e1, e2 );
   } else {
      if ((IND & SS_OFFSET_BIT) && ctx->Polygon.OffsetFill) {
	 v[0]->win[2] += offset;
	 v[1]->win[2] += offset;
	 v[2]->win[2] += offset;
      }
      _swrast_Triangle( ctx, v[0], v[1], v[2] );
   }

   if (IND & SS_OFFSET_BIT) {
      v[0]->win[2] = z[0];
      v[1]->win[2] = z[1];
      v[2]->win[2] = z[2];
   }

   if (IND & SS_TWOSIDE_BIT) {
      if (facing == 1) {
	 if (IND & SS_RGBA_BIT) {
	    GLfloat (*vbcolor)[4] = (GLfloat (*)[4])VB->ColorPtr[0]->Ptr;
	    GLfloat (*vbspec)[4] = (GLfloat (*)[4])VB->SecondaryColorPtr[0]->Ptr;
	    SS_COLOR(v[0]->color, vbcolor[e0]);
	    SS_COLOR(v[1]->color, vbcolor[e1]);
	    SS_COLOR(v[2]->color, vbcolor[e2]);
	    SS_SPEC(v[0]->specular, vbspec[e0]);
	    SS_SPEC(v[1]->specular, vbspec[e1]);
	    SS_SPEC(v[2]->specular, vbspec[e2]);
	 } else {
	    GLuint *vbindex = VB->IndexPtr[0]->data;
	    SS_IND(v[0]->index, vbindex[e0]);
	    SS_IND(v[1]->index, vbindex[e1]);
	    SS_IND(v[2]->index, vbindex[e2]);
	 }
      }
   }
}



/* Need to fixup edgeflags when decomposing to triangles:
 */
static void TAG(quadfunc)( GLcontext *ctx, GLuint v0,
		       GLuint v1, GLuint v2, GLuint v3 )
{
   if (IND & SS_UNFILLED_BIT) {
      struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
      GLubyte ef1 = VB->EdgeFlag[v1];
      GLubyte ef3 = VB->EdgeFlag[v3];
      VB->EdgeFlag[v1] = 0;
      TAG(triangle)( ctx, v0, v1, v3 );
      VB->EdgeFlag[v1] = ef1;
      VB->EdgeFlag[v3] = 0;
      TAG(triangle)( ctx, v1, v2, v3 );
      VB->EdgeFlag[v3] = ef3;
   } else {
      TAG(triangle)( ctx, v0, v1, v3 );
      TAG(triangle)( ctx, v1, v2, v3 );
   }
}




static void TAG(init)( void )
{
   tri_tab[IND] = TAG(triangle);
   quad_tab[IND] = TAG(quadfunc);
}


#undef IND
#undef TAG
