/*
 * Mesa 3-D graphics library
 * Version:  3.5
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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


static void TAG(triangle)(GLcontext *ctx,
			  GLuint e0, GLuint e1, GLuint e2,
			  GLuint pv)
{
   struct vertex_buffer *VB = TNL_VB(ctx);
   SWvertex *verts = SWSETUP_VB(VB)->verts;
   SWvertex *v[3];
   GLfloat offset;
   GLfloat z[3];
   GLubyte c[3][4], s[3][4];
   GLuint i[3];
   GLenum mode = GL_FILL;

   v[0] = &verts[e0];
   v[1] = &verts[e1];
   v[2] = &verts[e2];

   if (IND & (SS_TWOSIDE_BIT | SS_FLAT_BIT)) {
      SS_COLOR(c[0], v[0]->color);
      
      if (IND & SS_TWOSIDE_BIT) {
	 SS_COLOR(c[1], v[1]->color);
	 SS_COLOR(c[2], v[2]->color);
      }
      
      if (IND & SS_COPY_EXTRAS) {
	 SS_SPEC(s[0], v[0]->specular);
	 SS_IND(i[0], v[0]->index);

	 if (IND & SS_TWOSIDE_BIT) {
	    SS_SPEC(s[1], v[1]->specular);
	    SS_IND(i[1], v[1]->index);

	    SS_SPEC(s[2], v[2]->specular);
	    SS_IND(i[2], v[2]->index);
	 }
      }
   }

   if (IND & (SS_TWOSIDE_BIT | SS_OFFSET_BIT | SS_UNFILLED_BIT))
   {
      GLfloat ex = v[0]->win[0] - v[2]->win[0];
      GLfloat ey = v[0]->win[1] - v[2]->win[1];
      GLfloat fx = v[1]->win[0] - v[2]->win[0];
      GLfloat fy = v[1]->win[1] - v[2]->win[1];
      GLfloat cc  = ex*fy - ey*fx;

      if (IND & (SS_TWOSIDE_BIT | SS_UNFILLED_BIT))
      {
	 GLuint  facing = (cc < 0.0) ^ ctx->Polygon.FrontBit;
	
	 if (IND & SS_UNFILLED_BIT)
	    mode = facing ? ctx->Polygon.BackMode : ctx->Polygon.FrontMode;
 
	 if (IND & SS_TWOSIDE_BIT) {
	    GLubyte (*vbcolor)[4] = VB->Color[facing]->data;
	    GLubyte (*vbspec)[4] = VB->SecondaryColor[facing]->data;
	    GLuint *vbindex = VB->Index[facing]->data;

	    if (IND & SS_FLAT_BIT) {
	       SS_COLOR(v[0]->color, vbcolor[pv]);
	       SS_COLOR(v[1]->color, vbcolor[pv]);
	       SS_COLOR(v[2]->color, vbcolor[pv]);

	       if (IND & SS_COPY_EXTRAS) {
		  SS_SPEC(v[0]->specular, vbspec[pv]);
		  SS_SPEC(v[1]->specular, vbspec[pv]);
		  SS_SPEC(v[2]->specular, vbspec[pv]);
		  
		  SS_IND(v[0]->index, vbindex[pv]);
		  SS_IND(v[1]->index, vbindex[pv]);
		  SS_IND(v[2]->index, vbindex[pv]);
	       }
	    } else {
	       SS_COLOR(v[0]->color, vbcolor[e0]);
	       SS_COLOR(v[1]->color, vbcolor[e1]);
	       SS_COLOR(v[2]->color, vbcolor[e2]);

	       if (IND & SS_COPY_EXTRAS) {
		  SS_SPEC(v[0]->specular, vbspec[e0]);
		  SS_SPEC(v[1]->specular, vbspec[e1]);
		  SS_SPEC(v[2]->specular, vbspec[e2]);

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
      }
   }
   else if (IND & SS_FLAT_BIT)
   {
      GLubyte *color = VB->Color[0]->data[pv];
      GLubyte *spec = VB->SecondaryColor[0]->data[pv];
      GLuint index = VB->Index[0]->data[pv];

      SS_COLOR(v[0]->color, color);
      
      if (IND & SS_COPY_EXTRAS) {
	 SS_SPEC(v[0]->specular, spec);
	 SS_IND(v[0]->index, index);
      }
   }

   if (mode == GL_POINT) {
      GLubyte *ef = VB->EdgeFlagPtr->data;
      if ((IND & SS_OFFSET_BIT) && ctx->Polygon.OffsetPoint) {
	 v[0]->win[2] += offset;
	 v[1]->win[2] += offset;
	 v[2]->win[2] += offset;
      }
      if (ef[e0]&0x1) { ef[e0] &= ~0x1; _swrast_Point( ctx, v[0] ); }
      if (ef[e1]&0x1) { ef[e1] &= ~0x1; _swrast_Point( ctx, v[1] ); }
      if (ef[e2]&0x2) { ef[e2] &= ~0x2; _swrast_Point( ctx, v[2] ); }
   } else if (mode == GL_LINE) {
      GLubyte *ef = VB->EdgeFlagPtr->data;
      if ((IND & SS_OFFSET_BIT) && ctx->Polygon.OffsetLine) {
	 v[0]->win[2] += offset;
	 v[1]->win[2] += offset;
	 v[2]->win[2] += offset;
      }
      if (ef[e0]&0x1) { ef[e0] &= ~0x1; _swrast_Line( ctx, v[0], v[1] ); }
      if (ef[e1]&0x1) { ef[e1] &= ~0x1; _swrast_Line( ctx, v[1], v[2] ); }
      if (ef[e2]&0x2) { ef[e2] &= ~0x2; _swrast_Line( ctx, v[2], v[0] ); }
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

   if (IND & (SS_FLAT_BIT | SS_TWOSIDE_BIT)) {
      SS_COLOR(v[0]->color, c[0]);

      if (IND & SS_TWOSIDE_BIT) {
	 SS_COLOR(v[1]->color, c[1]);
	 SS_COLOR(v[2]->color, c[2]);
      }

      if (IND & SS_COPY_EXTRAS) {
	 SS_SPEC(v[0]->specular, s[0]);
	 SS_IND(v[0]->index, i[0]);

	 if (IND & SS_TWOSIDE_BIT) {
	    SS_SPEC(v[1]->specular, s[1]);
	    SS_IND(v[1]->index, i[1]);

	    SS_SPEC(v[2]->specular, s[2]);
	    SS_IND(v[2]->index, i[2]);
	 }
      }
   } 
}



/* Need to do something with edgeflags:
 */
static void TAG(quad)( GLcontext *ctx, GLuint v0,
		       GLuint v1, GLuint v2, GLuint v3, 
		       GLuint pv )
{
   TAG(triangle)( ctx, v0, v1, v3, pv );
   TAG(triangle)( ctx, v1, v2, v3, pv );
}


static void TAG(line)( GLcontext *ctx, GLuint v0, GLuint v1, GLuint pv )
{
   struct vertex_buffer *VB = TNL_VB(ctx);
   SWvertex *verts = SWSETUP_VB(VB)->verts;
   GLubyte c[2][4], s[2][4];
   GLuint i[2];
   SWvertex *vert0 = &verts[v0];
   SWvertex *vert1 = &verts[v1];


   if (IND & SS_FLAT_BIT) {
      GLubyte *color = VB->Color[0]->data[pv];
      GLubyte *spec = VB->SecondaryColor[0]->data[pv];
      GLuint index = VB->Index[0]->data[pv];

      SS_COLOR(c[0], vert0->color);
      SS_COLOR(vert0->color, color);

      if (IND & SS_COPY_EXTRAS) {
	 SS_SPEC(s[0], vert0->specular);
	 SS_SPEC(vert0->specular, spec);
	 
	 SS_IND(i[0], vert0->index);
	 SS_IND(vert0->index, index);
      }
   }

   _swrast_Line( ctx, vert0, vert1 );

   if (IND & SS_FLAT_BIT) {
      SS_COLOR(vert0->color, c[0]);
      
      if (IND & SS_COPY_EXTRAS) {
	 SS_SPEC(vert0->specular, s[0]);
	 SS_IND(vert0->index, i[0]);
      }
   }
}


static void TAG(points)( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = TNL_VB(ctx);
   SWvertex *verts = SWSETUP_VB(VB)->verts;
   int i;
   
   for(i=first;i<=last;i++) 
      if(VB->ClipMask[i]==0)
	 _swrast_Point( ctx, &verts[i] );
}




static void TAG(init)( void )
{
   tri_tab[IND] = TAG(triangle);
   quad_tab[IND] = TAG(quad);
   line_tab[IND] = TAG(line);
   points_tab[IND] = TAG(points);
}


#undef IND
#undef TAG
			 


