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


static void TAG(rs)(struct vertex_buffer *VB, GLuint start, GLuint end)
{
   GLcontext *ctx = VB->ctx;
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   SWvertex *v;
   GLfloat (*eye)[4];
   GLfloat (*win)[4];
   GLfloat (*tc[MAX_TEXTURE_UNITS])[4];
   GLubyte (*color)[4];
   GLubyte (*spec)[4];
   GLuint *index;
   GLfloat *fog;
   GLuint sz[MAX_TEXTURE_UNITS];
   GLuint szeye;
   int i;

   /* TODO: Do window map here.
    */
/*     GLfloat *m = VB->ctx->Viewport.WindowMap.m; */
/*     const GLfloat sx = m[0]; */
/*     const GLfloat sy = m[5]; */
/*     const GLfloat sz = m[10] * ctx->Visual->DepthMaxF; */
/*     const GLfloat tx = m[12]; */
/*     const GLfloat ty = m[13]; */
/*     const GLfloat tz = m[14] * ctx->Visual->DepthMaxF; */


   /* TODO:  Get import_client_data to pad vectors out to 4 cleanly.
    */
   gl_import_client_data( VB, tnl->_RenderFlags,
			  (VB->ClipOrMask
			   ? /*  VEC_CLEAN| */VEC_WRITABLE|VEC_GOOD_STRIDE
			   : /*  VEC_CLEAN| */VEC_GOOD_STRIDE));

   if (IND & TEX0) {
      tc[0] = VB->TexCoordPtr[0]->data;
      sz[0] = VB->TexCoordPtr[0]->size;
   }
   
   if (IND & MULTITEX) {
      for (i = 0 ; i < MAX_TEXTURE_UNITS ; i++) {
	 tc[i] = VB->TexCoordPtr[i]->data;
	 sz[i] = VB->TexCoordPtr[i]->size;
      }
   }

   fog = VB->FogCoordPtr->data;
   eye = VB->EyePtr->data;
   szeye = VB->EyePtr->size;
   win = VB->Win.data;
   color = VB->Color[0]->data;
   spec = VB->SecondaryColor[0]->data;
   index = VB->Index[0]->data;

   v = &(SWSETUP_VB(VB)->verts[start]);

   for (i=start; i < end; i++, v++) {
      if (VB->ClipMask[i] == 0) {
	 COPY_4FV( v->win, win[i] );
	 
	 if (IND & EYE) 
	    COPY_4FV( v->eye, eye[i] );

	 if (IND & TEX0) 
	    COPY_CLEAN_4V( v->texcoord[0], sz[0], tc[0][i] );

	 if (IND & MULTITEX) {
	    GLuint u;
	    for (u = 0 ; u < MAX_TEXTURE_UNITS ; u++)
	       if (ctx->Texture.Unit[u]._ReallyEnabled)
		  COPY_CLEAN_4V( v->texcoord[u], sz[u], tc[u][i] );
	 }

	 if (IND & COLOR)
	    COPY_4UBV(v->color, color[i]);
	 
	 if (IND & SPEC) 
	    COPY_4UBV(v->specular, spec[i]);

	 if (IND & FOG)
	    v->fog = fog[i]; 

	 if (IND & INDEX)
	    v->index = index[i]; 
      }
   }
}

#undef TAG
#undef IND
