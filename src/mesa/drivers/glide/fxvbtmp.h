/* $Id: fxvbtmp.h,v 1.10 2001/09/23 16:50:01 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  4.0
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
 */

/* Authors:
 *    Keith Whitwell <keithw@valinux.com>
 */


static void TAG(emit)( GLcontext *ctx,
		       GLuint start, GLuint end,
		       void *dest )
{
   fxMesaContext fxMesa = FX_CONTEXT(ctx);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLuint tmu0_source = fxMesa->tmu_source[0];
   GLuint tmu1_source = fxMesa->tmu_source[1];
   GLfloat (*tc0)[4], (*tc1)[4];
   GLubyte (*col)[4];
   GLuint tc0_stride, tc1_stride, col_stride;
   GLuint tc0_size, tc1_size;
   GLfloat (*proj)[4] = VB->ProjectedClipPtr->data; 
   GLuint proj_stride = VB->ProjectedClipPtr->stride;
   GrVertex *v = (GrVertex *)dest;
   GLfloat u0scale,v0scale,u1scale,v1scale;
   const GLfloat *const s = ctx->Viewport._WindowMap.m;
   int i;


   if (IND & SETUP_TMU0) {
      tc0 = VB->TexCoordPtr[tmu0_source]->data;
      tc0_stride = VB->TexCoordPtr[tmu0_source]->stride;
      u0scale = fxMesa->s0scale;
      v0scale = fxMesa->t0scale;
      if (IND & SETUP_PTEX)
	 tc0_size = VB->TexCoordPtr[tmu0_source]->size;
   }

   if (IND & SETUP_TMU1) {
      tc1 = VB->TexCoordPtr[tmu1_source]->data;
      tc1_stride = VB->TexCoordPtr[tmu1_source]->stride;
      u1scale = fxMesa->s1scale; /* wrong if tmu1_source == 0, possible? */
      v1scale = fxMesa->t1scale;
      if (IND & SETUP_PTEX)
	 tc1_size = VB->TexCoordPtr[tmu1_source]->size;
   }
   
   if (IND & SETUP_RGBA) {
      if (VB->ColorPtr[0]->Type != GL_UNSIGNED_BYTE)
	 import_float_colors( ctx );
      col = VB->ColorPtr[0]->Ptr;
      col_stride = VB->ColorPtr[0]->StrideB;
   }

   if (start) {
      proj =  (GLfloat (*)[4])((GLubyte *)proj + start * proj_stride);
      if (IND & SETUP_TMU0)
	 tc0 =  (GLfloat (*)[4])((GLubyte *)tc0 + start * tc0_stride);
      if (IND & SETUP_TMU1) 
	 tc1 =  (GLfloat (*)[4])((GLubyte *)tc1 + start * tc1_stride);
      if (IND & SETUP_RGBA) 
	 STRIDE_4UB(col, start * col_stride);
   }

   for (i=start; i < end; i++, v++) {
      if (IND & SETUP_XYZW) {
	 /* unclipped */
	 v->x   = s[0]  * proj[0][0] + s[12];	
	 v->y   = s[5]  * proj[0][1] + s[13];	
	 v->ooz = s[10] * proj[0][2] + s[14];	
	 v->oow = proj[0][3];	

	 if (IND & SETUP_SNAP) {
#if defined(USE_IEEE)
	    const float snapper = (3L << 18);
	    v->x += snapper;
	    v->x -= snapper;
	    v->y += snapper;
	    v->y -= snapper;
#else
	    v->x = ((int) (v->x * 16.0f)) * (1.0f / 16.0f);
	    v->y = ((int) (v->y * 16.0f)) * (1.0f / 16.0f);
#endif
	 }

	 proj =  (GLfloat (*)[4])((GLubyte *)proj +  proj_stride);
      }
      if (IND & SETUP_RGBA) {
	 v->r = (GLfloat) col[0][0];
	 v->g = (GLfloat) col[0][1];
	 v->b = (GLfloat) col[0][2];
	 v->a = (GLfloat) col[0][3];
	 STRIDE_4UB(col, col_stride);
      }
      if (IND & SETUP_TMU0) {
	 GLfloat w = v->oow;
	 if (IND & SETUP_PTEX) {
	    v->tmuvtx[0].sow = tc0[0][0] * u0scale * w;
	    v->tmuvtx[0].tow = tc0[0][1] * v0scale * w;
	    v->tmuvtx[0].oow = w;
	    if (tc0_size == 4) 
	       v->tmuvtx[0].oow = tc0[0][3] * w;
	 } 
	 else {
	    v->tmuvtx[0].sow = tc0[0][0] * u0scale * w;
	    v->tmuvtx[0].tow = tc0[0][1] * v0scale * w;
	 } 
	 tc0 =  (GLfloat (*)[4])((GLubyte *)tc0 +  tc0_stride);
      }
      if (IND & SETUP_TMU1) {
	 GLfloat w = v->oow;
	 if (IND & SETUP_PTEX) {
	    v->tmuvtx[1].sow = tc1[0][0] * u1scale * w;
	    v->tmuvtx[1].tow = tc1[0][1] * v1scale * w;
	    v->tmuvtx[1].oow = w;
	    if (tc1_size == 4) 
	       v->tmuvtx[1].oow = tc1[0][3] * w;
	 } 
	 else {
	    v->tmuvtx[1].sow = tc1[0][0] * u1scale * w;
	    v->tmuvtx[1].tow = tc1[0][1] * v1scale * w;
	 }
	 tc1 =  (GLfloat (*)[4])((GLubyte *)tc1 +  tc1_stride);
      } 
   }
}

#if (IND & SETUP_XYZW) && (IND & SETUP_RGBA)

static GLboolean TAG(check_tex_sizes)( GLcontext *ctx )
{
/*     fprintf(stderr, "%s\n", __FUNCTION__); */

   if (IND & SETUP_PTEX)
      return GL_TRUE;
   
   if (IND & SETUP_TMU0) {
      struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

      if (IND & SETUP_TMU1) {
	 if (VB->TexCoordPtr[0] == 0)
	    VB->TexCoordPtr[0] = VB->TexCoordPtr[1];
	 
	 if (VB->TexCoordPtr[1]->size == 4)
	    return GL_FALSE;
      }

      if (VB->TexCoordPtr[0] && VB->TexCoordPtr[0]->size == 4)
	 return GL_FALSE;
   }

   return GL_TRUE;
}

static void TAG(interp)( GLcontext *ctx,
			 GLfloat t, 
			 GLuint edst, GLuint eout, GLuint ein,
			 GLboolean force_boundary )
{
   fxMesaContext fxMesa = FX_CONTEXT( ctx );
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   const GLfloat *dstclip = VB->ClipPtr->data[edst];
   const GLfloat oow = (dstclip[3] == 0.0F) ? 1.0F : (1.0F / dstclip[3]);
   const GLfloat *const s = ctx->Viewport._WindowMap.m;
   GrVertex *fxverts = fxMesa->verts;
   GrVertex *dst = (GrVertex *) (fxverts + edst);
   const GrVertex *out = (const GrVertex *) (fxverts + eout);
   const GrVertex *in = (const GrVertex *) (fxverts + ein);
   const GLfloat wout = 1.0F / out->oow;
   const GLfloat win = 1.0F / in->oow;

   dst->x   = s[0]  * dstclip[0] * oow + s[12];	
   dst->y   = s[5]  * dstclip[1] * oow + s[13];	
   dst->ooz = s[10] * dstclip[2] * oow + s[14];	
   dst->oow = oow;	
   
   if (IND & SETUP_SNAP) {
#if defined(USE_IEEE)
      const float snapper = (3L << 18);
      dst->x += snapper;
      dst->x -= snapper;
      dst->y += snapper;
      dst->y -= snapper;
#else
      dst->x = ((int) (dst->x * 16.0f)) * (1.0f / 16.0f);
      dst->y = ((int) (dst->y * 16.0f)) * (1.0f / 16.0f);
#endif
   }

   
   INTERP_F( t, dst->r, out->r, in->r );
   INTERP_F( t, dst->g, out->g, in->g );
   INTERP_F( t, dst->b, out->b, in->b );
   INTERP_F( t, dst->a, out->a, in->a );

   if (IND & SETUP_TMU0) {
      if (IND & SETUP_PTEX) {
	 INTERP_F( t, 
		   dst->tmuvtx[0].sow, 
		   out->tmuvtx[0].sow * wout, 
		   in->tmuvtx[0].sow * win );
	 INTERP_F( t,
		   dst->tmuvtx[0].tow,
		   out->tmuvtx[0].tow * wout, 
		   in->tmuvtx[0].tow * win );
	 INTERP_F( t, 
		   dst->tmuvtx[0].oow, 
		   out->tmuvtx[0].oow * wout, 
		   in->tmuvtx[0].oow * win );

	 dst->tmuvtx[0].sow *= oow;
	 dst->tmuvtx[0].tow *= oow;
	 dst->tmuvtx[0].oow *= oow;
      } else {
	 INTERP_F( t, 
		   dst->tmuvtx[0].sow, 
		   out->tmuvtx[0].sow * wout, 
		   in->tmuvtx[0].sow * win );
	 INTERP_F( t,
		   dst->tmuvtx[0].tow,
		   out->tmuvtx[0].tow * wout, 
		   in->tmuvtx[0].tow * win );

	 dst->tmuvtx[0].sow *= oow;
	 dst->tmuvtx[0].tow *= oow;
      }
   }

   if (IND & SETUP_TMU1) {
      if (IND & SETUP_PTEX) {
	 INTERP_F( t, 
		   dst->tmuvtx[1].sow, 
		   out->tmuvtx[1].sow * wout, 
		   in->tmuvtx[1].sow * win );
	 INTERP_F( t,
		   dst->tmuvtx[1].tow,
		   out->tmuvtx[1].tow * wout, 
		   in->tmuvtx[1].tow * win );
	 INTERP_F( t, 
		   dst->tmuvtx[1].oow, 
		   out->tmuvtx[1].oow * wout, 
		   in->tmuvtx[1].oow * win );

	 dst->tmuvtx[1].sow *= oow;
	 dst->tmuvtx[1].tow *= oow;
	 dst->tmuvtx[1].oow *= oow;
      } else {
	 INTERP_F( t, 
		   dst->tmuvtx[1].sow, 
		   out->tmuvtx[1].sow * wout, 
		   in->tmuvtx[1].sow * win );
	 INTERP_F( t,
		   dst->tmuvtx[1].tow,
		   out->tmuvtx[1].tow * wout, 
		   in->tmuvtx[1].tow * win );

	 dst->tmuvtx[1].sow *= oow;
	 dst->tmuvtx[1].tow *= oow;
      }
   }
}
#endif


static void TAG(init)( void )
{
   setup_tab[IND].emit = TAG(emit);
   
#if ((IND & SETUP_XYZW) && (IND & SETUP_RGBA))
   setup_tab[IND].check_tex_sizes = TAG(check_tex_sizes);
   setup_tab[IND].interp = TAG(interp);

   if (IND & SETUP_PTEX) {
      setup_tab[IND].vertex_format = (GR_STWHINT_W_DIFF_TMU0 |
				      GR_STWHINT_W_DIFF_TMU1);
   }
   else {
      setup_tab[IND].vertex_format = 0;
   }

#if (IND & SETUP_TMU1)
     setup_tab[IND].vertex_format |= GR_STWHINT_ST_DIFF_TMU1;
#endif

#endif
}


#undef IND
#undef TAG
