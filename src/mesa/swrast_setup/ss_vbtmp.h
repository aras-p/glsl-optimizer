/* $Id: ss_vbtmp.h,v 1.15 2001/04/30 09:04:00 keithw Exp $ */

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


static void TAG(rs)(GLcontext *ctx, GLuint start, GLuint end, GLuint newinputs )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   SWvertex *v;
   GLfloat (*proj)[4];		/* projected clip coordinates */
   GLfloat (*tc[MAX_TEXTURE_UNITS])[4];
   GLfloat (*color)[4];
   GLfloat (*spec)[4];
   GLuint *index;
   GLfloat *fog;
   GLfloat *pointSize;
   GLuint tsz[MAX_TEXTURE_UNITS];
   GLuint i;
   GLfloat *m = ctx->Viewport._WindowMap.m;
   const GLfloat sx = m[0];
   const GLfloat sy = m[5];
   const GLfloat sz = m[10];
   const GLfloat tx = m[12];
   const GLfloat ty = m[13];
   const GLfloat tz = m[14];
   GLuint maxtex = 0;

   /* Only the most basic optimization for cva:
    */
   if (!newinputs) 
      return;

   /* TODO:  Get import_client_data to pad vectors out to 4 cleanly.
    *
    * NOTE: This has the effect of converting any remaining ubyte
    *       colors to floats...  As they're already there 90% of the
    *       time, this isn't a bad thing.  
    */
   if (VB->importable_data)
      VB->import_data( ctx, VB->importable_data & newinputs,
		       (VB->ClipOrMask
			? VEC_NOT_WRITEABLE|VEC_BAD_STRIDE
			: VEC_BAD_STRIDE));

   if (IND & TEX0) {
      tc[0] = VB->TexCoordPtr[0]->data;
      tsz[0] = VB->TexCoordPtr[0]->size;
   }

   if (IND & MULTITEX) {
      for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++) {
	 if (VB->TexCoordPtr[i]) {
	    maxtex = i+1;
	    tc[i] = VB->TexCoordPtr[i]->data;
	    tsz[i] = VB->TexCoordPtr[i]->size;
	 }
	 else tc[i] = 0;
      }
   }

   /* Tie up some dangling pointers for flat/twoside code in ss_tritmp.h
    */
   if ((ctx->_TriangleCaps & DD_SEPARATE_SPECULAR) == 0) {
      VB->SecondaryColorPtr[0] = VB->ColorPtr[0];
      VB->SecondaryColorPtr[1] = VB->ColorPtr[1];
   }


   proj = VB->ProjectedClipPtr->data;
   if (IND & FOG)
      fog = VB->FogCoordPtr->data;
   if (IND & COLOR)
      color = (GLfloat (*)[4])VB->ColorPtr[0]->Ptr;
   if (IND & SPEC)
      spec = (GLfloat (*)[4])VB->SecondaryColorPtr[0]->Ptr;
   if (IND & INDEX)
      index = VB->IndexPtr[0]->data;
   if (IND & POINT)
      pointSize = VB->PointSizePtr->data;

   v = &(SWSETUP_CONTEXT(ctx)->verts[start]);

   for (i=start; i < end; i++, v++) {
      if (VB->ClipMask[i] == 0) {
	 v->win[0] = sx * proj[i][0] + tx;
	 v->win[1] = sy * proj[i][1] + ty;
	 v->win[2] = sz * proj[i][2] + tz;
	 v->win[3] =      proj[i][3];

	 if (IND & TEX0)
	    COPY_CLEAN_4V( v->texcoord[0], tsz[0], tc[0][i] );

	 if (IND & MULTITEX) {
	    GLuint u;
	    for (u = 0 ; u < maxtex ; u++)
	       if (tc[u])
		  COPY_CLEAN_4V( v->texcoord[u], tsz[u], tc[u][i] );
	 }

	 if (IND & COLOR)
	    UNCLAMPED_FLOAT_TO_RGBA_CHAN(v->color, color[i]);

	 if (IND & SPEC)
	    UNCLAMPED_FLOAT_TO_RGBA_CHAN(v->specular, spec[i]);

	 if (IND & FOG)
	    v->fog = fog[i];

	 if (IND & INDEX)
	    v->index = index[i];

         if (IND & POINT)
            v->pointSize = pointSize[i];
      }
   }
}

#undef TAG
#undef IND
#undef SETUP_FLAGS
