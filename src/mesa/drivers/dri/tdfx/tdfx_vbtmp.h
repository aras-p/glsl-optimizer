
#if (IND & (TDFX_W_BIT|TDFX_TEX0_BIT|TDFX_TEX1_BIT))

static void TAG(emit)( GLcontext *ctx,
		       GLuint start, GLuint end,
		       void *dest,
		       GLuint stride )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLfloat (*tc0)[4], (*tc1)[4];
   GLubyte (*col)[4];
   GLuint tc0_stride, tc1_stride, col_stride;
   GLuint tc0_size, tc1_size;
   GLfloat (*proj)[4] = VB->NdcPtr->data; 
   GLuint proj_stride = VB->NdcPtr->stride;
   tdfxVertex *v = (tdfxVertex *)dest;
   GLfloat u0scale,v0scale,u1scale,v1scale;
   const GLubyte *mask = VB->ClipMask;
   const GLfloat *s = fxMesa->hw_viewport;
   int i;

/*     fprintf(stderr, "%s\n", __FUNCTION__); */
   ASSERT(stride > 16);


   if (IND & TDFX_TEX0_BIT) {
      tc0_stride = VB->TexCoordPtr[0]->stride;
      tc0 = VB->TexCoordPtr[0]->data;
      u0scale = fxMesa->sScale0;
      v0scale = fxMesa->tScale0;
      if (IND & TDFX_PTEX_BIT)
	 tc0_size = VB->TexCoordPtr[0]->size;
   }

   if (IND & TDFX_TEX1_BIT) {
      tc1 = VB->TexCoordPtr[1]->data;
      tc1_stride = VB->TexCoordPtr[1]->stride;
      u1scale = fxMesa->sScale1;
      v1scale = fxMesa->tScale1;
      if (IND & TDFX_PTEX_BIT)
	 tc1_size = VB->TexCoordPtr[1]->size;
   }
   
   if (IND & TDFX_RGBA_BIT) {
      if (VB->ColorPtr[0]->Type != GL_UNSIGNED_BYTE)
	 import_float_colors( ctx );
      col = VB->ColorPtr[0]->Ptr;
      col_stride = VB->ColorPtr[0]->StrideB;
   }

   if (VB->importable_data) {
      /* May have nonstandard strides:
       */
      if (start) {
	 proj =  (GLfloat (*)[4])((GLubyte *)proj + start * proj_stride);
	 if (IND & TDFX_TEX0_BIT)
	    tc0 =  (GLfloat (*)[4])((GLubyte *)tc0 + start * tc0_stride);
	 if (IND & TDFX_TEX1_BIT) 
	    tc1 =  (GLfloat (*)[4])((GLubyte *)tc1 + start * tc1_stride);
	 if (IND & TDFX_RGBA_BIT) 
	    STRIDE_4UB(col, start * col_stride);
      }

      for (i=start; i < end; i++, v = (tdfxVertex *)((GLubyte *)v + stride)) {
	 if (IND & TDFX_XYZ_BIT) {
	    if (mask[i] == 0) {
               /* unclipped */
	       v->v.x   = s[0]  * proj[0][0] + s[12];	
	       v->v.y   = s[5]  * proj[0][1] + s[13];	
	       v->v.z   = s[10] * proj[0][2] + s[14];	
	       v->v.rhw = proj[0][3];	
	    } else {
               /* clipped */
               v->v.rhw = 1.0;
	    }
	    proj =  (GLfloat (*)[4])((GLubyte *)proj +  proj_stride);
	 }
	 if (IND & TDFX_RGBA_BIT) {
#if 0
	    *(GLuint *)&v->v.color = *(GLuint *)col;
#else
	    GLubyte *b = (GLubyte *) &v->v.color;
	    b[0] = col[0][2];
	    b[1] = col[0][1];
	    b[2] = col[0][0];
	    b[3] = col[0][3];

#endif
	    STRIDE_4UB(col, col_stride);
	 }
	 if (IND & TDFX_TEX0_BIT) {
	    GLfloat w = v->v.rhw;
	    if (IND & TDFX_PTEX_BIT) {
	       v->pv.tu0 = tc0[0][0] * u0scale * w;
	       v->pv.tv0 = tc0[0][1] * v0scale * w;
	       v->pv.tq0 = w;
	       if (tc0_size == 4) 
		  v->pv.tq0 = tc0[0][3] * w;
	    } 
	    else {
	       v->v.tu0 = tc0[0][0] * u0scale * w;
	       v->v.tv0 = tc0[0][1] * v0scale * w;
	    } 
	    tc0 =  (GLfloat (*)[4])((GLubyte *)tc0 +  tc0_stride);
	 }
	 if (IND & TDFX_TEX1_BIT) {
	    GLfloat w = v->v.rhw;
	    if (IND & TDFX_PTEX_BIT) {
	       v->pv.tu1 = tc1[0][0] * u1scale * w;
	       v->pv.tv1 = tc1[0][1] * v1scale * w;
	       v->pv.tq1 = w;
	       if (tc1_size == 4) 
		  v->pv.tq1 = tc1[0][3] * w;
	    } 
	    else {
	       v->v.tu1 = tc1[0][0] * u1scale * w;
	       v->v.tv1 = tc1[0][1] * v1scale * w;
	    }
	    tc1 =  (GLfloat (*)[4])((GLubyte *)tc1 +  tc1_stride);
	 } 
      }
   }
   else {
      for (i=start; i < end; i++, v = (tdfxVertex *)((GLubyte *)v + stride)) {
	 if (IND & TDFX_XYZ_BIT) {
	    if (mask[i] == 0) {
	       v->v.x   = s[0]  * proj[i][0] + s[12];	
	       v->v.y   = s[5]  * proj[i][1] + s[13];	
	       v->v.z   = s[10] * proj[i][2] + s[14];	
	       v->v.rhw = proj[i][3];	
	    } else {
	       v->v.rhw = 1.0;
	    }
	 }
	 if (IND & TDFX_RGBA_BIT) {
#if 0
	    *(GLuint *)&v->v.color = *(GLuint *)&col[i];
#else
	    GLubyte *b = (GLubyte *) &v->v.color;
	    b[0] = col[i][2];
	    b[1] = col[i][1];
	    b[2] = col[i][0];
	    b[3] = col[i][3];

#endif
	 }
	 if (IND & TDFX_TEX0_BIT) {
	    GLfloat w = v->v.rhw;
	    if (IND & TDFX_PTEX_BIT) {
	       v->pv.tu0 = tc0[i][0] * u0scale * w;
	       v->pv.tv0 = tc0[i][1] * v0scale * w;
	       v->pv.tq0 = w;
	       if (tc0_size == 4) 
		  v->pv.tq0 = tc0[i][3] * w;
	    } 
	    else {
	       v->v.tu0 = tc0[i][0] * u0scale * w;
               v->v.tv0 = tc0[i][1] * v0scale * w;
	    }
	 }
	 if (IND & TDFX_TEX1_BIT) {
	    GLfloat w = v->v.rhw;
	    if (IND & TDFX_PTEX_BIT) {
	       v->pv.tu1 = tc1[i][0] * u1scale * w;
	       v->pv.tv1 = tc1[i][1] * v1scale * w;
	       v->pv.tq1 = w;
	       if (tc1_size == 4) 
		  v->pv.tq1 = tc1[i][3] * w;
	    } 
	    else {
	       v->v.tu1 = tc1[i][0] * u1scale * w;
	       v->v.tv1 = tc1[i][1] * v1scale * w;
	    }
	 }
      }
   }
}
#else 
#if (IND & TDFX_XYZ_BIT)
static void TAG(emit)( GLcontext *ctx, GLuint start, GLuint end,
		       void *dest, GLuint stride )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLubyte (*col)[4];
   GLuint col_stride;
   GLfloat (*proj)[4] = VB->NdcPtr->data; 
   GLuint proj_stride = VB->NdcPtr->stride;
   GLfloat *v = (GLfloat *)dest;
   const GLubyte *mask = VB->ClipMask;
   const GLfloat *s = fxMesa->hw_viewport;
   int i;

/*       fprintf(stderr, "%s %d..%d dest %p stride %d\n", __FUNCTION__,  */
/*    	   start, end, dest, stride);  */

   ASSERT(fxMesa->SetupIndex == (TDFX_XYZ_BIT|TDFX_RGBA_BIT));
   ASSERT(stride == 16);

   if (VB->ColorPtr[0]->Type != GL_UNSIGNED_BYTE)
      import_float_colors( ctx );

   col = (GLubyte (*)[4])VB->ColorPtr[0]->Ptr;
   col_stride = VB->ColorPtr[0]->StrideB;
   ASSERT(VB->ColorPtr[0]->Type == GL_UNSIGNED_BYTE);

   /* Pack what's left into a 4-dword vertex.  Color is in a different
    * place, and there is no 'w' coordinate.  
    */
   if (VB->importable_data) {
      if (start) {
	 proj =  (GLfloat (*)[4])((GLubyte *)proj + start * proj_stride);
	 STRIDE_4UB(col, start * col_stride);
      }

      for (i=start; i < end; i++, v+=4) {
	 if (mask[i] == 0) {
	    v[0]   = s[0]  * proj[0][0] + s[12];	
	    v[1]   = s[5]  * proj[0][1] + s[13];	
	    v[2]   = s[10] * proj[0][2] + s[14];	
	 }
	 proj =  (GLfloat (*)[4])((GLubyte *)proj +  proj_stride);
	 {
	    GLubyte *b = (GLubyte *)&v[3];
	    b[0] = col[0][2];
	    b[1] = col[0][1];
	    b[2] = col[0][0];
	    b[3] = col[0][3];
	    STRIDE_4UB(col, col_stride);
	 }
      }
   }
   else {
      for (i=start; i < end; i++, v+=4) {
	 if (mask[i] == 0) {
	    v[0]   = s[0]  * proj[i][0] + s[12];	
	    v[1]   = s[5]  * proj[i][1] + s[13];	
	    v[2]   = s[10] * proj[i][2] + s[14];	
	 }
	 {
	    GLubyte *b = (GLubyte *)&v[3];
	    b[0] = col[i][2];
	    b[1] = col[i][1];
	    b[2] = col[i][0];
	    b[3] = col[i][3];
	 }
      }
   }
}
#else
static void TAG(emit)( GLcontext *ctx, GLuint start, GLuint end,
		       void *dest, GLuint stride )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT(ctx);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLubyte (*col)[4];
   GLuint col_stride;
   GLfloat *v = (GLfloat *)dest;
   int i;

   if (VB->ColorPtr[0]->Type != GL_UNSIGNED_BYTE)
      import_float_colors( ctx );

   col = VB->ColorPtr[0]->Ptr;
   col_stride = VB->ColorPtr[0]->StrideB;

   if (start)
      STRIDE_4UB(col, col_stride * start);

   /* Need to figure out where color is:
    */
   if (fxMesa->SetupIndex & TDFX_W_BIT )
      v += 4;
   else
      v += 3;

   for (i=start; i < end; i++, STRIDE_F(v, stride)) {
      GLubyte *b = (GLubyte *)v;
      b[0] = col[0][2];
      b[1] = col[0][1];
      b[2] = col[0][0];
      b[3] = col[0][3];
      STRIDE_4UB( col, col_stride );
   }
}
#endif
#endif

#if (IND & TDFX_XYZ_BIT) && (IND & TDFX_RGBA_BIT)

static GLboolean TAG(check_tex_sizes)( GLcontext *ctx )
{
/*     fprintf(stderr, "%s\n", __FUNCTION__); */

   if (IND & TDFX_PTEX_BIT)
      return GL_TRUE;
   
   if (IND & TDFX_TEX0_BIT) {
      struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

      if (IND & TDFX_TEX1_BIT) {
	 if (VB->TexCoordPtr[0] == 0)
	    VB->TexCoordPtr[0] = VB->TexCoordPtr[1];
	 
	 if (VB->TexCoordPtr[1]->size == 4)
	    return GL_FALSE;
      }

      if (VB->TexCoordPtr[0]->size == 4)
	 return GL_FALSE;
   }

   return GL_TRUE;
}

static void TAG(interp)( GLcontext *ctx,
			 GLfloat t, 
			 GLuint edst, GLuint eout, GLuint ein,
			 GLboolean force_boundary )
{
   tdfxContextPtr fxMesa = TDFX_CONTEXT( ctx );
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   const GLuint shift = fxMesa->vertex_stride_shift;
   const GLfloat *dstclip = VB->ClipPtr->data[edst];
   const GLfloat oow = (dstclip[3] == 0.0F) ? 1.0F : (1.0F / dstclip[3]);
   const GLfloat *s = fxMesa->hw_viewport;
   GLubyte *tdfxverts = (GLubyte *)fxMesa->verts;
   tdfxVertex *dst = (tdfxVertex *) (tdfxverts + (edst << shift));
   const tdfxVertex *out = (const tdfxVertex *) (tdfxverts + (eout << shift));
   const tdfxVertex *in = (const tdfxVertex *) (tdfxverts + (ein << shift));
   const GLfloat wout = 1.0F / out->v.rhw;
   const GLfloat win = 1.0F / in->v.rhw;

   dst->v.x   = s[0]  * dstclip[0] * oow + s[12];	
   dst->v.y   = s[5]  * dstclip[1] * oow + s[13];	
   dst->v.z   = s[10] * dstclip[2] * oow + s[14];	

   if (IND & (TDFX_W_BIT|TDFX_TEX0_BIT|TDFX_TEX1_BIT)) {
      dst->v.rhw = oow;	
   
      INTERP_UB( t, dst->ub4[4][0], out->ub4[4][0], in->ub4[4][0] );
      INTERP_UB( t, dst->ub4[4][1], out->ub4[4][1], in->ub4[4][1] );
      INTERP_UB( t, dst->ub4[4][2], out->ub4[4][2], in->ub4[4][2] );
      INTERP_UB( t, dst->ub4[4][3], out->ub4[4][3], in->ub4[4][3] );

      if (IND & TDFX_TEX0_BIT) {
	 if (IND & TDFX_PTEX_BIT) {
	    INTERP_F( t, dst->pv.tu0, out->pv.tu0 * wout, in->pv.tu0 * win );
	    INTERP_F( t, dst->pv.tv0, out->pv.tv0 * wout, in->pv.tv0 * win );
	    INTERP_F( t, dst->pv.tq0, out->pv.tq0 * wout, in->pv.tq0 * win );
	    dst->pv.tu0 *= oow;
	    dst->pv.tv0 *= oow;
	    dst->pv.tq0 *= oow;
	 } else {
	    INTERP_F( t, dst->v.tu0, out->v.tu0 * wout, in->v.tu0 * win );
	    INTERP_F( t, dst->v.tv0, out->v.tv0 * wout, in->v.tv0 * win );
	    dst->v.tu0 *= oow;
	    dst->v.tv0 *= oow;
	 }
      }
      if (IND & TDFX_TEX1_BIT) {
	 if (IND & TDFX_PTEX_BIT) {
	    INTERP_F( t, dst->pv.tu1, out->pv.tu1 * wout, in->pv.tu1 * win );
	    INTERP_F( t, dst->pv.tv1, out->pv.tv1 * wout, in->pv.tv1 * win );
	    INTERP_F( t, dst->pv.tq1, out->pv.tq1 * wout, in->pv.tq1 * win );
	    dst->pv.tu1 *= oow;
	    dst->pv.tv1 *= oow;
	    dst->pv.tq1 *= oow;
	 } else {
	    INTERP_F( t, dst->v.tu1, out->v.tu1 * wout, in->v.tu1 * win );
	    INTERP_F( t, dst->v.tv1, out->v.tv1 * wout, in->v.tv1 * win );
	    dst->v.tu1 *= oow;
	    dst->v.tv1 *= oow;
	 }
      }
   } else {
      /* 4-dword vertex.  Color is in v[3] and there is no oow coordinate.
       */
      INTERP_UB( t, dst->ub4[3][0], out->ub4[3][0], in->ub4[3][0] );
      INTERP_UB( t, dst->ub4[3][1], out->ub4[3][1], in->ub4[3][1] );
      INTERP_UB( t, dst->ub4[3][2], out->ub4[3][2], in->ub4[3][2] );
      INTERP_UB( t, dst->ub4[3][3], out->ub4[3][3], in->ub4[3][3] );
   }
}
#endif


static void TAG(init)( void )
{
/*     fprintf(stderr, "%s\n", __FUNCTION__); */

   setup_tab[IND].emit = TAG(emit);
   
#if ((IND & TDFX_XYZ_BIT) && (IND & TDFX_RGBA_BIT))
   setup_tab[IND].check_tex_sizes = TAG(check_tex_sizes);
   setup_tab[IND].interp = TAG(interp);
   
   if (IND & (TDFX_W_BIT|TDFX_TEX0_BIT|TDFX_TEX1_BIT))
      setup_tab[IND].copy_pv = copy_pv_rgba4;
   else
      setup_tab[IND].copy_pv = copy_pv_rgba3;


   if (IND & TDFX_TEX1_BIT) {
      if (IND & TDFX_PTEX_BIT) {
	 setup_tab[IND].vertex_format = TDFX_LAYOUT_PROJECT;
	 setup_tab[IND].vertex_size = 12;
	 setup_tab[IND].vertex_stride_shift = 6; 
      }
      else {
	 setup_tab[IND].vertex_format = TDFX_LAYOUT_MULTI;
	 setup_tab[IND].vertex_size = 10;
	 setup_tab[IND].vertex_stride_shift = 6; 
      }
   } 
   else if (IND & TDFX_TEX0_BIT) {
      if (IND & TDFX_PTEX_BIT) {
	 setup_tab[IND].vertex_format = TDFX_LAYOUT_PROJECT;
	 setup_tab[IND].vertex_size = 12;
	 setup_tab[IND].vertex_stride_shift = 6; 
      } else {
	 setup_tab[IND].vertex_format = TDFX_LAYOUT_SINGLE;
	 setup_tab[IND].vertex_size = 8;
	 setup_tab[IND].vertex_stride_shift = 5; 
      }
   }
   else if (IND & TDFX_W_BIT) {
      setup_tab[IND].vertex_format = TDFX_LAYOUT_NOTEX;
      setup_tab[IND].vertex_size = 6;
      setup_tab[IND].vertex_stride_shift = 5; 
   } else {
      setup_tab[IND].vertex_format = TDFX_LAYOUT_TINY;
      setup_tab[IND].vertex_size = 4;
      setup_tab[IND].vertex_stride_shift = 4; 
   }
#endif
}


#undef IND
#undef TAG
