
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
 * Author:
 *   Keith Whitwell <keithw@valinux.com>
 */


/* Template for building functions to plug into the driver interface
 * of t_vb_render.c:
 *     ctx->Driver.QuadFunc  
 *     ctx->Driver.TriangleFunc  
 *     ctx->Driver.LineFunc  
 *     ctx->Driver.PointsFunc  
 *
 * DO_TWOSIDE:   Plug back-color values from the VB into backfacing triangles, 
 *               and restore vertices afterwards.
 * DO_OFFSET:    Calculate offset for triangles and adjust vertices.  Restore
 *               vertices after rendering.
 * DO_FLAT:      For hardware without native flatshading, copy provoking colors
 *               into the other vertices.  Restore after rendering.
 * DO_UNFILLED:  Decompose triangles to lines and points where appropriate.
 *
 * HAVE_RGBA: Vertices have rgba values (otherwise index values).
 */



static void TAG(triangle)( GLcontext *ctx, GLuint e0, GLuint e1, GLuint e2 )
{
   struct vertex_buffer *VB = &TNL_CONTEXT( ctx )->vb;
   VERTEX *v[3];
   GLfloat offset;
   GLfloat z[3];
   GLenum mode = GL_FILL;
   GLuint facing;
   LOCAL_VARS(3);

   v[0] = GET_VERTEX(e0);
   v[1] = GET_VERTEX(e1);
   v[2] = GET_VERTEX(e2);

   if (DO_TWOSIDE || DO_OFFSET || DO_UNFILLED)
   {
      GLfloat ex = VERT_X(v[0]) - VERT_X(v[2]);
      GLfloat ey = VERT_Y(v[0]) - VERT_Y(v[2]);
      GLfloat fx = VERT_X(v[1]) - VERT_X(v[2]);
      GLfloat fy = VERT_Y(v[1]) - VERT_Y(v[2]);
      GLfloat cc = ex*fy - ey*fx;

      if (DO_TWOSIDE || DO_UNFILLED)
      {
	 facing = AREA_IS_CCW( cc ) ^ ctx->Polygon._FrontBit;

	 if (DO_UNFILLED) {
	    if (facing) {
	       mode = ctx->Polygon.BackMode;
	       if (ctx->Polygon.CullFlag && 
		   ctx->Polygon.CullFaceMode != GL_FRONT) {
		  return;
	       }
	    } else {
	       mode = ctx->Polygon.FrontMode;
	       if (ctx->Polygon.CullFlag &&
		   ctx->Polygon.CullFaceMode != GL_BACK) {
		  return;
	       }
	    }
	 }

	 if (DO_TWOSIDE && facing == 1)
	 {
	    if (HAVE_RGBA) {
	       GLubyte (*vbcolor)[4] = VB->ColorPtr[1]->data;
	       ASSERT(VB->ColorPtr[1]->stride == 4*sizeof(GLubyte));

	       if (!DO_FLAT) {
		  VERT_SET_COLOR( v[0], vbcolor[e0] );
		  VERT_SET_COLOR( v[1], vbcolor[e1] );
	       }
	       VERT_SET_COLOR( v[2], vbcolor[e2] );

	       if (VB->SecondaryColorPtr[1]) {
		  GLubyte (*vbspec)[4] = VB->SecondaryColorPtr[1]->data;
		  ASSERT(VB->SecondaryColorPtr[1]->stride == 4*sizeof(GLubyte));
		
		  if (!DO_FLAT) {
		     VERT_SET_SPEC( v[0], vbspec[e0] );
		     VERT_SET_SPEC( v[1], vbspec[e1] );
		  }
		  VERT_SET_SPEC( v[2], vbspec[e2] );
	       }
	    } 
	    else {
	       GLuint *vbindex = VB->IndexPtr[1]->data;
	       if (!DO_FLAT) {
		  VERT_SET_IND( v[0], vbindex[e0] );
		  VERT_SET_IND( v[1], vbindex[e1] );
	       }
	       VERT_SET_IND( v[2], vbindex[e2] );
	    }
	 }
      }


      if (DO_OFFSET)
      {
	 offset = ctx->Polygon.OffsetUnits * DEPTH_SCALE;
	 z[0] = VERT_Z(v[0]);
	 z[1] = VERT_Z(v[1]);
	 z[2] = VERT_Z(v[2]);
	 if (cc * cc > 1e-16) {
	    GLfloat ic	= 1.0 / cc;
	    GLfloat ez	= z[0] - z[2];
	    GLfloat fz	= z[1] - z[2];
	    GLfloat a	= ey*fz - ez*fy;
	    GLfloat b	= ez*fx - ex*fz;
	    GLfloat ac	= a * ic;
	    GLfloat bc	= b * ic;
	    if ( ac < 0.0f ) ac = -ac;
	    if ( bc < 0.0f ) bc = -bc;
	    offset += MAX2( ac, bc ) * ctx->Polygon.OffsetFactor;
	 }
	 offset *= ctx->MRD;
      }
   }

   if (DO_FLAT) {
      if (HAVE_RGBA) {
	 VERT_SAVE_VERT_RGBA( 0 );
	 VERT_SAVE_VERT_RGBA( 1 );
	 VERT_COPY_VERT_RGBA( v[0], v[2] );
	 VERT_COPY_VERT_RGBA( v[1], v[2] );
	 if (VB->SecondaryColorPtr[0]) {
	    VERT_SAVE_VERT_SPEC( 0 );
	    VERT_SAVE_VERT_SPEC( 1 );
	    VERT_COPY_VERT_SPEC( v[0], v[2] );
	    VERT_COPY_VERT_SPEC( v[1], v[2] );
	 }
      } 
      else {
	 VERT_SAVE_VERT_IND( 0 );
	 VERT_SAVE_VERT_IND( 1 );
	 VERT_COPY_VERT_IND( v[0], v[2] );
	 VERT_COPY_VERT_IND( v[1], v[2] );
      }
   }

   if (mode == GL_POINT) {
      if (DO_OFFSET && ctx->Polygon.OffsetPoint) {
	 VERT_Z(v[0]) += offset;
	 VERT_Z(v[1]) += offset;
	 VERT_Z(v[2]) += offset;
      }
      UNFILLED_POINT_TRI( ctx, e0, e1, e2 );
   } else if (mode == GL_LINE) {
      if (DO_OFFSET && ctx->Polygon.OffsetLine) {
	 VERT_Z(v[0]) += offset;
	 VERT_Z(v[1]) += offset;
	 VERT_Z(v[2]) += offset;
      }
      UNFILLED_LINE_TRI( ctx, e0, e1, e2 );
   } else {
      if (DO_OFFSET && ctx->Polygon.OffsetFill) {
	 VERT_Z(v[0]) += offset;
	 VERT_Z(v[1]) += offset;
	 VERT_Z(v[2]) += offset;
      }
      if (DO_UNFILLED)
	 SET_REDUCED_PRIM( GL_TRIANGLES, GL_TRIANGLES );
      TRI( ctx, v[0], v[1], v[2] ); 
   }

   if (DO_OFFSET)
   {
      VERT_Z(v[0]) = z[0];
      VERT_Z(v[1]) = z[1];
      VERT_Z(v[2]) = z[2];
   }

   /* ==> Need to import Color, SecondaryColor, Index to meet assertions
    *     in DO_FLAT case.
    *
    * ==> Copy/Restore vertex data instead?
    */
   if (DO_TWOSIDE && facing == 1)
   {
      if (HAVE_RGBA) {
	 GLubyte (*vbcolor)[4] = VB->ColorPtr[0]->data;
	 ASSERT(VB->ColorPtr[0]->stride == 4*sizeof(GLubyte));
	 
	 if (!DO_FLAT) {
	    VERT_SET_COLOR( v[0], vbcolor[e0] );
	    VERT_SET_COLOR( v[1], vbcolor[e1] );
	 }
	 VERT_SET_COLOR( v[2], vbcolor[e2] );
	 
	 if (VB->SecondaryColorPtr[0]) {
	    GLubyte (*vbspec)[4] = VB->SecondaryColorPtr[0]->data;
	    ASSERT(VB->SecondaryColorPtr[0]->stride == 4*sizeof(GLubyte));
	  
	    if (!DO_FLAT) {
	       VERT_SET_SPEC( v[0], vbspec[e0] );
	       VERT_SET_SPEC( v[1], vbspec[e1] );
	    }
	    VERT_SET_SPEC( v[2], vbspec[e2] );
	 }
      } 
      else {
	 GLuint *vbindex = VB->IndexPtr[0]->data;
	 if (!DO_FLAT) {
	    VERT_SET_IND( v[0], vbindex[e0] );
	    VERT_SET_IND( v[1], vbindex[e1] );
	 }
	 VERT_SET_IND( v[2], vbindex[e2] );
      }
   }


   if (DO_FLAT) {
      if (HAVE_RGBA) {
	 VERT_RESTORE_VERT_RGBA( 0 );
	 VERT_RESTORE_VERT_RGBA( 1 );
	 if (VB->SecondaryColorPtr[0]) {
	    VERT_RESTORE_VERT_SPEC( 0 );
	    VERT_RESTORE_VERT_SPEC( 1 );
	 }
      } 
      else {
	 VERT_RESTORE_VERT_IND( 0 );
	 VERT_RESTORE_VERT_IND( 1 );
      }
   }


}

#if (DO_FULL_QUAD)
static void TAG(quad)( GLcontext *ctx,
		       GLuint e0, GLuint e1, GLuint e2, GLuint e3 )
{
   struct vertex_buffer *VB = &TNL_CONTEXT( ctx )->vb;
   VERTEX *v[4];
   GLfloat offset;
   GLfloat z[4];
   GLenum mode = GL_FILL;
   GLuint facing;
   LOCAL_VARS(4);

   v[0] = GET_VERTEX(e0);
   v[1] = GET_VERTEX(e1);
   v[2] = GET_VERTEX(e2);
   v[3] = GET_VERTEX(e3);

   if (DO_TWOSIDE || DO_OFFSET || DO_UNFILLED)
   {
      GLfloat ex = VERT_X(v[2]) - VERT_X(v[0]);
      GLfloat ey = VERT_Y(v[2]) - VERT_Y(v[0]);
      GLfloat fx = VERT_X(v[3]) - VERT_X(v[1]);
      GLfloat fy = VERT_Y(v[3]) - VERT_Y(v[1]);
      GLfloat cc = ex*fy - ey*fx;

      if (DO_TWOSIDE || DO_UNFILLED)
      {
	 facing = AREA_IS_CCW( cc ) ^ ctx->Polygon._FrontBit;

	 if (DO_UNFILLED) {
	    if (facing) {
	       mode = ctx->Polygon.BackMode;
	       if (ctx->Polygon.CullFlag && 
		   ctx->Polygon.CullFaceMode != GL_FRONT) {
		  return;
	       }
	    } else {
	       mode = ctx->Polygon.FrontMode;
	       if (ctx->Polygon.CullFlag &&
		   ctx->Polygon.CullFaceMode != GL_BACK) {
		  return;
	       }
	    }
	 }

	 if (DO_TWOSIDE && facing == 1)
	 {
	    if (HAVE_RGBA) {
	       GLubyte (*vbcolor)[4] = VB->ColorPtr[1]->data;
	       
	       if (!DO_FLAT) {
		  VERT_SET_COLOR( v[0], vbcolor[e0] );
		  VERT_SET_COLOR( v[1], vbcolor[e1] );
		  VERT_SET_COLOR( v[2], vbcolor[e2] );
	       }
	       VERT_SET_COLOR( v[3], vbcolor[e3] );

	       if (VB->SecondaryColorPtr[facing]) {
		  GLubyte (*vbspec)[4] = VB->SecondaryColorPtr[1]->data;
		  ASSERT(VB->SecondaryColorPtr[1]->stride == 4*sizeof(GLubyte));

		  if (!DO_FLAT) {
		     VERT_SET_SPEC( v[0], vbspec[e0] );
		     VERT_SET_SPEC( v[1], vbspec[e1] );
		     VERT_SET_SPEC( v[2], vbspec[e2] );
		  }
		  VERT_SET_SPEC( v[3], vbspec[e3] );
	       }
	    }
	    else {
	       GLuint *vbindex = VB->IndexPtr[1]->data;
	       if (!DO_FLAT) {
		  VERT_SET_IND( v[0], vbindex[e0] );
		  VERT_SET_IND( v[1], vbindex[e1] );
		  VERT_SET_IND( v[2], vbindex[e2] );
	       }
	       VERT_SET_IND( v[3], vbindex[e3] );
	    }
	 }
      }


      if (DO_OFFSET)
      {
	 offset = ctx->Polygon.OffsetUnits * DEPTH_SCALE;
	 z[0] = VERT_Z(v[0]);
	 z[1] = VERT_Z(v[1]);
	 z[2] = VERT_Z(v[2]);
	 z[3] = VERT_Z(v[3]);
	 if (cc * cc > 1e-16) {
	    GLfloat ez = z[2] - z[0];
	    GLfloat fz = z[3] - z[1];
	    GLfloat a	= ey*fz - ez*fy;
	    GLfloat b	= ez*fx - ex*fz;
	    GLfloat ic	= 1.0 / cc;
	    GLfloat ac	= a * ic;
	    GLfloat bc	= b * ic;
	    if ( ac < 0.0f ) ac = -ac;
	    if ( bc < 0.0f ) bc = -bc;
	    offset += MAX2( ac, bc ) * ctx->Polygon.OffsetFactor;
	 }
	 offset *= ctx->MRD;
      }
   }

   if (DO_FLAT) {
      if (HAVE_RGBA) {
	 VERT_SAVE_VERT_RGBA( 0 );
	 VERT_SAVE_VERT_RGBA( 1 );
	 VERT_SAVE_VERT_RGBA( 2 );
	 VERT_COPY_VERT_RGBA( v[0], v[3] );
	 VERT_COPY_VERT_RGBA( v[1], v[3] );
	 VERT_COPY_VERT_RGBA( v[2], v[3] );
	 if (VB->SecondaryColorPtr[0]) {
	    VERT_SAVE_VERT_SPEC( 0 );
	    VERT_SAVE_VERT_SPEC( 1 );
	    VERT_SAVE_VERT_SPEC( 2 );
	    VERT_COPY_VERT_SPEC( v[0], v[3] );
	    VERT_COPY_VERT_SPEC( v[1], v[3] );
	    VERT_COPY_VERT_SPEC( v[2], v[3] );
	 }
      } 
      else {
	 VERT_SAVE_VERT_IND( 0 );
	 VERT_SAVE_VERT_IND( 1 );
	 VERT_SAVE_VERT_IND( 2 );
	 VERT_COPY_VERT_IND( v[0], v[3] );
	 VERT_COPY_VERT_IND( v[1], v[3] );
	 VERT_COPY_VERT_IND( v[2], v[3] );
      }
   }

   if (mode == GL_POINT) {
      if (( DO_OFFSET) && ctx->Polygon.OffsetPoint) {
	 VERT_Z(v[0]) += offset;
	 VERT_Z(v[1]) += offset;
	 VERT_Z(v[2]) += offset;
	 VERT_Z(v[3]) += offset;
      }
      UNFILLED_POINT_QUAD( ctx, e0, e1, e2, e3 );
   } else if (mode == GL_LINE) {
      if (DO_OFFSET && ctx->Polygon.OffsetLine) {
	 VERT_Z(v[0]) += offset;
	 VERT_Z(v[1]) += offset;
	 VERT_Z(v[2]) += offset;
	 VERT_Z(v[3]) += offset;
      }
      UNFILLED_LINE_QUAD( ctx, e0, e1, e2, e3 );
   } else {
      if (DO_OFFSET && ctx->Polygon.OffsetFill) {
	 VERT_Z(v[0]) += offset;
	 VERT_Z(v[1]) += offset;
	 VERT_Z(v[2]) += offset;
	 VERT_Z(v[3]) += offset;
      }
      if (DO_UNFILLED)
	 SET_REDUCED_PRIM( GL_QUADS, GL_TRIANGLES );
      QUAD( (v[0]), (v[1]), (v[2]), (v[3]) ); 
   }

   if (DO_OFFSET)
   {
      VERT_Z(v[0]) = z[0];
      VERT_Z(v[1]) = z[1];
      VERT_Z(v[2]) = z[2];
      VERT_Z(v[3]) = z[3];
   }   

   if (DO_TWOSIDE && facing == 1)
   {
      if (HAVE_RGBA) {
	 GLubyte (*vbcolor)[4] = VB->ColorPtr[0]->data;
	 ASSERT(VB->ColorPtr[0]->stride == 4*sizeof(GLubyte));
	 
	 if (!DO_FLAT) {
	    VERT_SET_COLOR( v[0], vbcolor[e0] );
	    VERT_SET_COLOR( v[1], vbcolor[e1] );
	    VERT_SET_COLOR( v[2], vbcolor[e2] );
	 }
	 VERT_SET_COLOR( v[3], vbcolor[e3] );
	 
	 if (VB->SecondaryColorPtr[0]) {
	    GLubyte (*vbspec)[4] = VB->SecondaryColorPtr[0]->data;
	    ASSERT(VB->SecondaryColorPtr[0]->stride == 4*sizeof(GLubyte));
	  
	    if (!DO_FLAT) {
	       VERT_SET_SPEC( v[0], vbspec[e0] );
	       VERT_SET_SPEC( v[1], vbspec[e1] );
	       VERT_SET_SPEC( v[2], vbspec[e2] );
	    }
	    VERT_SET_SPEC( v[3], vbspec[e3] );
	 }
      } 
      else {
	 GLuint *vbindex = VB->IndexPtr[0]->data;
	 if (!DO_FLAT) {
	    VERT_SET_IND( v[0], vbindex[e0] );
	    VERT_SET_IND( v[1], vbindex[e1] );
	    VERT_SET_IND( v[2], vbindex[e2] );
	 }
	 VERT_SET_IND( v[3], vbindex[e3] );
      }
   }


   if (DO_FLAT) {
      if (HAVE_RGBA) {
	 VERT_RESTORE_VERT_RGBA( 0 );
	 VERT_RESTORE_VERT_RGBA( 1 );
	 VERT_RESTORE_VERT_RGBA( 2 );
	 if (VB->SecondaryColorPtr[0]) {
	    VERT_RESTORE_VERT_SPEC( 0 );
	    VERT_RESTORE_VERT_SPEC( 1 );
	    VERT_RESTORE_VERT_SPEC( 2 );
	 }
      } 
      else {
	 VERT_RESTORE_VERT_IND( 0 );
	 VERT_RESTORE_VERT_IND( 1 );
	 VERT_RESTORE_VERT_IND( 2 );
      }
   }
}
#else 
static void TAG(quad)( GLcontext *ctx, GLuint e0,
		       GLuint e1, GLuint e2, GLuint e3 )
{
   if (IND & SS_UNFILLED_BIT) {
      struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
      GLubyte ef1 = VB->EdgeFlag[e1];
      GLubyte ef3 = VB->EdgeFlag[e3];
      VB->EdgeFlag[e1] = 0;      
      TAG(triangle)( ctx, e0, e1, e3 );
      VB->EdgeFlag[e1] = ef1;
      VB->EdgeFlag[e3] = 0;      
      TAG(triangle)( ctx, e1, e2, e3 );      
      VB->EdgeFlag[e3] = ef3;      
   } else {
      TAG(triangle)( ctx, e0, e1, e3 );
      TAG(triangle)( ctx, e1, e2, e3 );
   }
}
#endif


static void TAG(line)( GLcontext *ctx, GLuint e0, GLuint e1 )
{
   VERTEX *v[2];
   LOCAL_VARS(2);

   v[0] = GET_VERTEX(e0);
   v[1] = GET_VERTEX(e1);

   if (DO_FLAT) {
      if (HAVE_RGBA) {
	 VERT_SAVE_VERT_RGBA( 0 );
	 VERT_COPY_VERT_RGBA( v[0], v[1] );
	 if (VB->SecondaryColorPtr[0]) {
	    VERT_SAVE_VERT_SPEC( 0 );
	    VERT_COPY_VERT_SPEC( v[0], v[1] );
	 }
      } 
      else {
	 VERT_SAVE_VERT_IND( 0 );
	 VERT_COPY_VERT_IND( v[0], v[1] );
      }
   }

   LINE( v[0], v[1] );

   if (DO_FLAT) {
      if (HAVE_RGBA) {
	 VERT_RESTORE_VERT_RGBA( 0 );

	 if (VB->SecondaryColorPtr[0]) {
	    VERT_RESTORE_VERT_SPEC( 0 );
	 }
      } 
      else {
	 VERT_RESTORE_VERT_IND( 0 );
      }
   }
}


static void TAG(points)( GLcontext *ctx, GLuint first, GLuint last )
{
   struct vertex_buffer *VB = &TNL_CONTEXT( ctx )->vb;
   int i;
   LOCAL_VARS(1);

   if (VB->Elts == 0) {
      for ( i = first ; i < last ; i++ ) {
	 if ( VB->ClipMask[i] == 0 ) {
	    VERTEX *v = GET_VERTEX(i);
	    POINT( v );
	 }
      }
   } else {
      for ( i = first ; i < last ; i++ ) {
	 GLuint e = VB->Elts[i];
	 if ( VB->ClipMask[e] == 0 ) {
	    VERTEX *v = GET_VERTEX(e);
	    POINT( v );
	 }
      }
   }
}

#undef IND
#undef TAG
