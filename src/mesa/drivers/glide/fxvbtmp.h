
/*
 * Mesa 3-D graphics library
 * Version:  3.3
 *
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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
 *   Keith Whitwell <keith@precisioninsight.com>
 */


static void NAME(GLcontext *ctx, GLuint start, GLuint end, GLuint newinputs)
{
   fxMesaContext fxMesa = (fxMesaContext)ctx->DriverCtx;
   fxVertex *verts = fxMesa->verts;
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLuint tmu0_source = fxMesa->tmu_source[0];
   GLuint tmu1_source = fxMesa->tmu_source[1];
   GLfloat (*tmu0_data)[4];
   GLfloat (*tmu1_data)[4];
   GLubyte (*color)[4];
   GLfloat (*proj)[4] = VB->ProjectedClipPtr->data; 
   fxVertex *v = &verts[start];
   GLfloat sscale0 = fxMesa->s0scale;
   GLfloat tscale0 = fxMesa->t0scale;
   GLfloat sscale1 = fxMesa->s1scale;
   GLfloat tscale1 = fxMesa->t1scale;
   GLubyte *clipmask = VB->ClipMask;
   GLuint i;
   const GLfloat * const s = ctx->Viewport._WindowMap.m;

   if (IND & SETUP_TMU0)
      tmu0_data = VB->TexCoordPtr[tmu0_source]->data;

   if (IND & SETUP_TMU1)
      tmu1_data = VB->TexCoordPtr[tmu1_source]->data;
      
   if (IND & SETUP_RGBA)
      color = VB->ColorPtr[0]->data;

   for (i = start ; i < end ; i++, v++) {
      if (!clipmask[i]) {
	 if (IND & SETUP_XYZW) {
	    v->v.x   = s[0]  * proj[i][0] + s[12];	
	    v->v.y   = s[5]  * proj[i][1] + s[13];	
	    v->v.ooz = s[10] * proj[i][2] + s[14];	
	    v->v.oow = proj[i][3];	
		
	    if (IND & SETUP_SNAP) {
#if defined(USE_IEEE)
	       const float snapper = (3L<<18);
	       v->v.x   += snapper;
	       v->v.x   -= snapper;
	       v->v.y   += snapper;
	       v->v.y   -= snapper;
#else
	       v->v.x = ((int)(v->v.x*16.0f)) * (1.0f/16.0f);
	       v->v.y = ((int)(v->v.y*16.0f)) * (1.0f/16.0f);
#endif
	    }
	 }
	 if (IND & SETUP_RGBA) {
	    UBYTE_COLOR_TO_FLOAT_255_COLOR2(v->v.r, color[i][0]);
	    UBYTE_COLOR_TO_FLOAT_255_COLOR2(v->v.g, color[i][1]);
	    UBYTE_COLOR_TO_FLOAT_255_COLOR2(v->v.b, color[i][2]);
	    UBYTE_COLOR_TO_FLOAT_255_COLOR2(v->v.a, color[i][3]);
	 }
	 if (IND & SETUP_TMU0) {
	    v->v.tmuvtx[0].sow = sscale0*tmu0_data[i][0]*v->v.oow; 
	    v->v.tmuvtx[0].tow = tscale0*tmu0_data[i][1]*v->v.oow; 
	 }
	 if (IND & SETUP_TMU1) {
	    v->v.tmuvtx[1].sow = sscale1*tmu1_data[i][0]*v->v.oow; 
	    v->v.tmuvtx[1].tow = tscale1*tmu1_data[i][1]*v->v.oow; 
	 }
      }
   }
      
   if ((IND & SETUP_XYZW) &&
       ctx->ProjectionMatrix.m[15] != 0.0F && 
       ctx->Fog.Enabled) 
   {
      fx_fake_fog_w( ctx, v, VB, start, end );
   }

   /* Check for and enable projective texturing in each texture unit.
    */
   if (IND & (SETUP_TMU0|SETUP_TMU1)) {
      GLuint tmu0_sz = 2;
      GLuint tmu1_sz = 2;
      GLuint hs = fxMesa->stw_hint_state & ~(GR_STWHINT_W_DIFF_TMU0 |	
					     GR_STWHINT_W_DIFF_TMU1);

      if (VB->TexCoordPtr[tmu0_source])
	 tmu0_sz = VB->TexCoordPtr[tmu0_source]->size;

      if (VB->TexCoordPtr[tmu1_source])
	 tmu1_sz = VB->TexCoordPtr[tmu1_source]->size;

      if (tmu0_sz == 4) {
	 project_texcoords( v, VB, 0, tmu0_source, start, end );
	 if (tmu1_sz == 4)
	    project_texcoords( v, VB, 1, tmu1_source, start, end );
	 else 
	    copy_w( v, VB, 1, start, end );
	 hs |= (GR_STWHINT_W_DIFF_TMU0|GR_STWHINT_W_DIFF_TMU1);
      } 
      else if (tmu1_sz == 4) {
	 project_texcoords( v, VB, 1, tmu1_source, start, end );
	 hs |= GR_STWHINT_W_DIFF_TMU1;
      }

      if (hs != fxMesa->stw_hint_state) {
	 fxMesa->stw_hint_state = hs;
	 FX_grHints(GR_HINT_STWHINT, hs);
      }
   }
}




#undef IND
#undef NAME
#undef INPUTS
