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


/* Unlike the other templates here, this assumes quite a bit about the
 * underlying hardware.  Specifically it assumes a d3d-like vertex
 * format, with a layout more or less constrained to look like the
 * following:
 *
 * union {
 *    struct {
 *        float x, y, z, w;
 *        struct { char r, g, b, a; } color;
 *        struct { char r, g, b, fog; } spec;
 *        float u0, v0;
 *        float u1, v1;
 *    } v;
 *    struct {
 *        float x, y, z, w;
 *        struct { char r, g, b, a; } color;
 *        struct { char r, g, b, fog; } spec;
 *        float u0, v0, q0;
 *        float u1, v1, q1;
 *    } pv;
 *    struct {
 *        float x, y, z;
 *        struct { char r, g, b, a; } color;
 *    } tv;
 *    float f[16];
 *    unsigned int ui[16];
 *    unsigned char ub4[4][16];
 * }
 * 
 * HW_VIEWPORT:  Hardware performs viewport transform.
 * HW_DIVIDE:  Hardware performs perspective divide.
 *
 * DO_XYZW:  Emit xyz and maybe w coordinates.
 * DO_RGBA:  Emit color, v.color is in RGBA order. 
 * DO_BGRA:  Emit color, v.color is in BGRA order. 
 * DO_SPEC:  Emit specular color.
 * DO_TEX0:  Emit tex0 u,v coordinates.
 * DO_TEX1:  Emit tex1 u,v coordinates.
 * DO_PTEX:  Emit tex0, tex1 q coordinates where possible.
 * 
 * HAVE_TINY_VERTICES:  Hardware understands v.tv format.
 * HAVE_PTEX_VERTICES:  Hardware understands v.pv format.
 * HAVE_NOTEX_VERTICES:  Hardware understands v.v format with texcount 0.
 *
 * Additionally, this template assumes it is emitting *transformed*
 * vertices; the modifications to emit untransformed vertices (ie. to
 * t&l hardware) are probably too great to cooexist with the code
 * already in this file.
 *
 * NOTE: The PTEX vertex format always includes TEX0 and TEX1, even if
 * only TEX0 is enabled, in order to maintain a vertex size which is
 * an exact number of quadwords.
 */

#if (HW_VIEWPORT)
#define VIEWPORT_X(x) x
#define VIEWPORT_Y(x) x
#define VIEWPORT_Z(x) x
#else
#define VIEWPORT_X(x) (s[0]  * x + s[12])
#define VIEWPORT_Y(y) (s[5]  * y + s[13])
#define VIEWPORT_Z(z) (s[10] * z + s[14])
#endif

#if (HW_DIVIDE || DO_RGBA || DO_XYZW || !HAVE_TINY_VERTICES)

static void TAG(emit)( GLcontext *ctx,
		       GLuint start, GLuint end,
		       void *dest,
		       GLuint stride )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLfloat (*tc0)[4], (*tc1)[4], *fog;
   GLubyte (*col)[4], (*spec)[4];
   GLuint tc0_stride, tc1_stride, col_stride, spec_stride, fog_stride;
   GLuint tc0_size, tc1_size;
   GLfloat (*coord)[4];
   GLuint coord_stride;
   VERTEX *v = (VERTEX *)dest;
   int i;

   if (HW_VIEWPORT && HW_DIVIDE) {
      coord = VB->ClipPtr->data;
      coord_stride = VB->ClipPtr->stride;
   }
   else {
      coord = VB->ProjectedClipPtr->data;
      coord_stride = VB->ProjectedClipPtr->stride;
   }

   if (DO_TEX0) {
      tc0_stride = VB->TexCoordPtr[0]->stride;
      tc0 = VB->TexCoordPtr[0]->data;
      if (DO_PTEX) 
	 tc0_size = VB->TexCoordPtr[0]->size;
   }

   if (DO_TEX1) {
      tc1 = VB->TexCoordPtr[1]->data;
      tc1_stride = VB->TexCoordPtr[1]->stride;
      if (DO_PTEX)
	 tc1_size = VB->TexCoordPtr[1]->size;
   }
   
   if (DO_RGBA || DO_BGRA) {
      col = VB->ColorPtr[0]->data;
      col_stride = VB->ColorPtr[0]->stride;
   }

   if (DO_SPEC) {
      spec = VB->SecondaryColorPtr[0]->data;
      spec_stride = VB->SecondaryColorPtr[0]->stride;
   }

   if (DO_FOG) {
      fog = VB->FogCoordPtr->data;
      fog_stride = VB->FogCoordPtr->stride;
   }

   if (VB->importable_data) {
      /* May have nonstandard strides:
       */
      if (start) {
	 coord =  (GLfloat (*)[4])((GLubyte *)coord + start * coord_stride);
	 if (DO_TEX0)
	    tc0 =  (GLfloat (*)[4])((GLubyte *)tc0 + start * tc0_stride);
	 if (DO_TEX1) 
	    tc0 =  (GLfloat (*)[4])((GLubyte *)tc1 + start * tc1_stride);
	 if (DO_RGBA || DO_BGRA) 
	    STRIDE_4UB(col, start * col_stride);
	 if (DO_SPEC) 
	    STRIDE_4UB(spec, start * spec_stride);
	 if (DO_FOG) 
	    STRIDE_F(fog, start * fog_stride);
      }

      for (i=start; i < end; i++, v = (ddVertex *)((GLubyte *)v + stride)) {
	 if (DO_XYZW) {
	    if (HW_VIEWPORT || mask[i] == 0) {
	       VIEWPORT_X(v->v.x, coord[0][0]);	
	       VIEWPORT_Y(v->v.y, coord[0][1]);	
	       VIEWPORT_Z(v->v.z, coord[0][2]);	
	       VIEWPORT_W(v->v.w, coord[0][3]);	
	    }
	    coord =  (GLfloat (*)[4])((GLubyte *)coord +  coord_stride);
	 }
	 if (DO_RGBA) {
	    *(GLuint *)&v->v.color = *(GLuint *)&col[0];
	    STRIDE_4UB(col, col_stride);
	 }
	 if (DO_BGRA) {
	    v->v.color.blue  = col[0][2];
	    v->v.color.green = col[0][1];
	    v->v.color.red   = col[0][0];
	    v->v.color.alpha = col[0][3];
	    STRIDE_4UB(col, col_stride);
	 }
	 if (DO_SPEC) {
	    v->v.specular.red = spec[0][0];
	    v->v.specular.green = spec[0][1];
	    v->v.specular.blue = spec[0][2];
	    STRIDE_4UB(spec, spec_stride);
	 }
	 if (DO_FOG) {
	    v->v.specular.alpha = fog[0] * 255.0;
	    STRIDE_F(fog, fog_stride);
	 }
	 if (DO_TEX0) {
	    *(GLuint *)&v->v.tu0 = *(GLuint *)&tc0[0][0];
	    *(GLuint *)&v->v.tv0 = *(GLuint *)&tc0[0][1];
	    if (DO_PTEX) {
	       if (HAVE_PTEX_VERTICES) {
		  if (tc0_size == 4) 
		     *(GLuint *)&v->pv.tq0 = *(GLuint *)&tc0[0][3];
		  else
		     *(GLuint *)&v->pv.tq0 = IEEE_ONE;
	       } 
	       else if (tc0_size == 4) {
		  float rhw = 1.0 / tc0[0][3];
		  v->v.w *= tc0[0][3];
		  v->v.u0 *= w;
		  v->v.v0 *= w;
	       } 
	    } 
	    tc0 =  (GLfloat (*)[4])((GLubyte *)tc0 +  tc0_stride);
	 }
	 if (DO_TEX1) {
	    if (DO_PTEX) {
	       *(GLuint *)&v->pv.u1 = *(GLuint *)&tc1[0][0];
	       *(GLuint *)&v->pv.v1 = *(GLuint *)&tc1[0][1];
	       *(GLuint *)&v->pv.q1 = IEEE_ONE;
	       if (tc1_size == 4) 
		  *(GLuint *)&v->pv.q1 = *(GLuint *)&tc1[0][3];
	    } 
	    else {
	       *(GLuint *)&v->v.u1 = *(GLuint *)&tc1[0][0];
	       *(GLuint *)&v->v.v1 = *(GLuint *)&tc1[0][1];
	    }
	    tc1 =  (GLfloat (*)[4])((GLubyte *)tc1 +  tc1_stride);
	 } 
	 else if (DO_PTEX) {
	    *(GLuint *)&v->pv.q1 = 0;	/* avoid culling on radeon */
	 }
      }
   }
   else {
      for (i=start; i < end; i++, v = (ddVertex *)((GLubyte *)v + stride)) {
	 if (DO_XYZW) {
	    if (HW_VIEWPORT || mask[i] == 0) {
	       VIEWPORT_X(v->v.x, coord[i][0]);	
	       VIEWPORT_Y(v->v.y, coord[i][1]);	
	       VIEWPORT_Z(v->v.z, coord[i][2]);	
	       VIEWPORT_W(v->v.w, coord[i][3]);	
	    }
	 }
	 if (DO_RGBA) {
	    *(GLuint *)&v->v.color = *(GLuint *)&col[i];
	 }
	 if (DO_BGRA) {
	    v->v.color.blue  = col[i][2];
	    v->v.color.green = col[i][1];
	    v->v.color.red   = col[i][0];
	    v->v.color.alpha = col[i][3];
	 }
	 if (DO_SPEC) {
	    v->v.specular.red   = spec[i][0];
	    v->v.specular.green = spec[i][1];
	    v->v.specular.blue  = spec[i][2];
	 }
	 if (DO_FOG) {
	    v->v.specular.alpha = fog[i] * 255.0;
	 }
	 if (DO_TEX0) {
	    if (DO_PTEX) {
	       *(GLuint *)&v->pv.u0 = *(GLuint *)&tc0[i][0];
	       *(GLuint *)&v->pv.v0 = *(GLuint *)&tc0[i][1];
	       *(GLuint *)&v->pv.q0 = IEEE_ONE;
	       if (tc0_size == 4) 
		  *(GLuint *)&v->pv.q0 = *(GLuint *)&tc0[i][3];
	    } 
	    else {
	       *(GLuint *)&v->v.u0 = *(GLuint *)&tc0[i][0];
	       *(GLuint *)&v->v.v0 = *(GLuint *)&tc0[i][1];
	    }
	 }
	 if (DO_TEX1) {
	    if (DO_PTEX) {
	       *(GLuint *)&v->pv.u1 = *(GLuint *)&tc1[i][0];
	       *(GLuint *)&v->pv.v1 = *(GLuint *)&tc1[i][1];
	       *(GLuint *)&v->pv.q1 = IEEE_ONE;
	       if (tc1_size == 4) 
		  *(GLuint *)&v->pv.q1 = *(GLuint *)&tc1[i][3];
	    } 
	    else {
	       *(GLuint *)&v->v.u1 = *(GLuint *)&tc1[i][0];
	       *(GLuint *)&v->v.v1 = *(GLuint *)&tc1[i][1];
	    }
	 }
	 else if (DO_PTEX) {
	    *(GLuint *)&v->pv.q1 = 0;	/* must be valid float to avoid culling? */
	 }
      }
   }

   if (DO_PTEX && !HAVE_PTEX_VERTICES) {
      INVALIDATE_STORED_VERTICES();
   }
}
#else 
static void TAG(emit)( GLcontext *ctx, GLuint start, GLuint end,
		       void *dest, GLuint stride )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLubyte (*col)[4] = VB->ColorPtr[0]->data;
   GLuint col_stride = VB->ColorPtr[0]->stride;
   GLfloat (*coord)[4] = VB->ProjectedClipPtr->data; 
   GLuint coord_stride = VB->ProjectedClipPtr->stride;
   GLfloat *v = (GLfloat *)dest;
   int i;

   ASSERT(stride == 4);

   /* Pack what's left into a 4-dword vertex.  Color is in a different
    * place, and there is no 'w' coordinate.  
    */
   if (VB->importable_data) {
      if (start) {
	 coord =  (GLfloat (*)[4])((GLubyte *)coord + start * coord_stride);
	 STRIDE_4UB(col, start * col_stride);
      }

      for (i=start; i < end; i++, v+=4) {
	 if (HW_VIEWPORT || mask[i] == 0) {
	    v[0] = VIEWPORT_X(coord[0][0]);	
	    v[1] = VIEWPORT_Y(coord[0][1]);	
	    v[2] = VIEWPORT_Z(coord[0][2]);	
	 }
	 coord =  (GLfloat (*)[4])((GLubyte *)coord +  coord_stride);
	 if (DO_RGBA) {
	    *(GLuint *)&v[3] = *(GLuint *)col;
	 } 
	 else if (DO_BGRA) {
	    GLubyte *b = (GLubyte *)&v[3];
	    b[0] = col[0][2];
	    b[1] = col[0][1];
	    b[2] = col[0][0];
	    b[3] = col[0][3];
	 }
	 STRIDE_4UB( col, col_stride );
      }
   }
   else {
      for (i=start; i < end; i++, v+=4) {
	 if (HW_VIEWPORT || mask[i] == 0) {
	    v[0] = VIEWPORT_X(coord[i][0]);	
	    v[1] = VIEWPORT_Y(coord[i][1]);	
	    v[2] = VIEWPORT_Z(coord[i][2]);	
	 }
	 if (DO_RGBA) {
	    *(GLuint *)&v[3] = *(GLuint *)&col[i];
	 }
	 else if (DO_BGRA) {
	    GLubyte *b = (GLubyte *)&v[3];
	    b[0] = col[i][2];
	    b[1] = col[i][1];
	    b[2] = col[i][0];
	    b[3] = col[i][3];
	 }
      }
   }
}
#endif

#if (DO_XYZW) && (DO_RGBA)

static GLboolean TAG(check_tex_sizes)( GLcontext *ctx )
{
   if (DO_PTEX)
      return GL_TRUE;
   
   if (DO_TEX0) {
      struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

      if (DO_TEX1) {
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
#if (!DO_PTEX || HAVE_PTEX_VERTICES)
static void TAG(interp)( GLcontext *ctx,
			 GLfloat t, 
			 GLuint edst, GLuint eout, GLuint ein,
			 GLboolean force_boundary )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLubyte *ddverts = GET_VERTEX_STORE();
   GLuint shift = GET_VERTEX_STRIDE_SHIFT();
   const GLfloat *dstclip = VB->ClipPtr->data[edst];
   GLfloat w;

   VERTEX *dst = (VERTEX *)(ddverts + (edst << shift));
   VERTEX *in  = (VERTEX *)(ddverts + (eout << shift));
   VERTEX *out = (VERTEX *)(ddverts + (ein << shift));

/*     fprintf(stderr, "%s\n", __FUNCTION__); */
   
   if (!HW_DIVIDE) {
      w = 1.0 / dstclip[3];
      VIEWPORT_X( dst->v.x, dstclip[0] * w );	
      VIEWPORT_Y( dst->v.y, dstclip[1] * w );	
      VIEWPORT_Z( dst->v.z, dstclip[2] * w );	
   } 
   else {
      VIEWPORT_X( dst->v.x, dstclip[0] );	
      VIEWPORT_Y( dst->v.y, dstclip[1] );	
      VIEWPORT_Z( dst->v.z, dstclip[2] );	
      w = dstclip[3];	
   }

   if (HW_DIVIDE || DO_FOG || DO_SPEC || DO_TEX0 || DO_TEX1) {
      
      if (!HW_VIEWPORT || !HW_DIVIDE)
	 dst->v.w = w;	
   
      INTERP_UB( t, dst->ub4[4][0], out->ub4[4][0], in->ub4[4][0] );
      INTERP_UB( t, dst->ub4[4][1], out->ub4[4][1], in->ub4[4][1] );
      INTERP_UB( t, dst->ub4[4][2], out->ub4[4][2], in->ub4[4][2] );
      INTERP_UB( t, dst->ub4[4][3], out->ub4[4][3], in->ub4[4][3] );

      if (DO_SPEC) {
	 INTERP_UB( t, dst->ub4[5][0], out->ub4[5][0], in->ub4[5][0] );
	 INTERP_UB( t, dst->ub4[5][1], out->ub4[5][1], in->ub4[5][1] );
	 INTERP_UB( t, dst->ub4[5][2], out->ub4[5][2], in->ub4[5][2] );
      }
      if (DO_FOG) {
	 INTERP_UB( t, dst->ub4[5][3], out->ub4[5][3], in->ub4[5][3] );
      }
      if (DO_TEX0) {
	 if (DO_PTEX && HAVE_PTEX_VERTICES) {
	    INTERP_F( t, dst->pv.u0, out->pv.u0, in->pv.u0 );
	    INTERP_F( t, dst->pv.v0, out->pv.v0, in->pv.v0 );
	    INTERP_F( t, dst->pv.q0, out->pv.q0, in->pv.q0 );
	 } 
	 else {
	    INTERP_F( t, dst->v.u0, out->v.u0, in->v.u0 );
	    INTERP_F( t, dst->v.v0, out->v.v0, in->v.v0 );
	 }
      }
      if (DO_TEX1) {
	 if (DO_PTEX) {
	    INTERP_F( t, dst->pv.u1, out->pv.u1, in->pv.u1 );
	    INTERP_F( t, dst->pv.v1, out->pv.v1, in->pv.v1 );
	    INTERP_F( t, dst->pv.q1, out->pv.q1, in->pv.q1 );
	 } else {
	    INTERP_F( t, dst->v.u1, out->v.u1, in->v.u1 );
	    INTERP_F( t, dst->v.v1, out->v.v1, in->v.v1 );
	 }
      }
      else if (DO_PTEX) {
	 dst->pv.q0 = 0.0;	/* must be a valid float on radeon */
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
#endif


static void TAG(init)( void )
{
   setup_tab[IND].emit = TAG(emit);
   
#if (DO_XYZW && DO_RGBA)
   setup_tab[IND].check_tex_sizes = TAG(check_tex_sizes);
   setup_tab[IND].interp = TAG(interp);
#endif
   
   if (DO_SPEC)
      setup_tab[IND].copy_pv = _tnl_dd_copy_pv_rgba4_spec5;
   else if (HW_DIVIDE || DO_SPEC || DO_FOG || DO_TEX0 || DO_TEX1)
      setup_tab[IND].copy_pv = _tnl_dd_copy_pv_rgba4;
   else
      setup_tab[IND].copy_pv = _tnl_dd_copy_pv_rgba3;

   if (DO_TEX1) {
      if (DO_PTEX) {
	 ASSERT(HAVE_PTEX_VERTICES);
	 setup_tab[IND].vc_format = PROJ_TEX_VERTEX_FORMAT;
	 setup_tab[IND].vertex_size = 12;
	 setup_tab[IND].vertex_stride_shift = 6; 
      }
      else {
	 setup_tab[IND].vc_format = TEX1_VERTEX_FORMAT;
	 setup_tab[IND].vertex_size = 10;
	 setup_tab[IND].vertex_stride_shift = 6; 
      }
   } 
   else if (DO_TEX0) {
      if (DO_PTEX && HAVE_PTEX_VERTICES) {
	 setup_tab[IND].vc_format = PROJ_TEX_VERTEX_FORMAT;
	 setup_tab[IND].vertex_size = 12;
	 setup_tab[IND].vertex_stride_shift = 6; 
      } else {
	 setup_tab[IND].vc_format = TEX0_VERTEX_FORMAT;
	 setup_tab[IND].vertex_size = 8;
	 setup_tab[IND].vertex_stride_shift = 5; 
      }
   }
   else if (!HW_DIVIDE && !DO_SPEC && !DO_FOG && HAVE_TINY_VERTICES) {
      setup_tab[IND].vertex_format = TINY_VERTEX_FORMAT;
      setup_tab[IND].vertex_size = 4;
      setup_tab[IND].vertex_stride_shift = 4; 
   } else if (HAVE_NOTEX_VERTICES) {
      setup_tab[IND].vertex_format = NOTEX_VERTEX_FORMAT;
      setup_tab[IND].vertex_size = 6;
      setup_tab[IND].vertex_stride_shift = 5;
   } else {
      setup_tab[IND].vc_format = TEX0_VERTEX_FORMAT;
      setup_tab[IND].vertex_size = 8;
      setup_tab[IND].vertex_stride_shift = 5; 
   }
}


#undef IND
#undef TAG
