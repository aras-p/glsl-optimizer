/* $XFree86: xc/lib/GL/mesa/src/drv/r200/r200_maos_vbtmp.h,v 1.2 2002/12/16 16:18:54 dawes Exp $ */
/*
Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.

The Weather Channel (TM) funded Tungsten Graphics to develop the
initial release of the Radeon 8500 driver under the XFree86 license.
This notice must be preserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef LOCALVARS
#define LOCALVARS
#endif

#undef TCL_DEBUG
#ifndef TCL_DEBUG
#define TCL_DEBUG 0
#endif

static void TAG(emit)( GLcontext *ctx,
		       GLuint start, GLuint end,
		       void *dest )
{
   LOCALVARS
      struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLuint (*tc0)[4], (*tc1)[4];
   GLfloat *fog;
   GLuint (*tc2)[4], (*norm)[3];
   GLubyte (*col)[4], (*spec)[4];
   GLuint tc0_stride, tc1_stride, col_stride, spec_stride, fog_stride;
   GLuint tc2_stride, norm_stride;
   GLuint (*coord)[4];
   GLuint coord_stride;
   GLubyte dummy[4];
   int i;

   union emit_union *v = (union emit_union *)dest;


   if (R200_DEBUG & DEBUG_VERTS)
      fprintf(stderr, "%s\n", __FUNCTION__); 

   /* The vertex code expects Obj to be clean to element 3.  To fix
    * this, add more vertex code (for obj-2, obj-3) or preferably move
    * to maos.  
    */
   if (VB->ObjPtr->size < 3) {
      if (VB->ObjPtr->flags & VEC_NOT_WRITEABLE) {
	 VB->import_data( ctx, VERT_OBJ, VEC_NOT_WRITEABLE );
      }
      _mesa_vector4f_clean_elem( VB->ObjPtr, VB->Count, 2 );
   }

   if (DO_W && VB->ObjPtr->size < 4) {
      if (VB->ObjPtr->flags & VEC_NOT_WRITEABLE) {
	 VB->import_data( ctx, VERT_OBJ, VEC_NOT_WRITEABLE );
      }
      _mesa_vector4f_clean_elem( VB->ObjPtr, VB->Count, 3 );
   }

   coord = (GLuint (*)[4])VB->ObjPtr->data;
   coord_stride = VB->ObjPtr->stride;

   if (DO_TEX2) {
      const GLuint t2 = GET_TEXSOURCE(2);
      tc2 = (GLuint (*)[4])VB->TexCoordPtr[t2]->data;
      tc2_stride = VB->TexCoordPtr[t2]->stride;
      if (DO_PTEX && VB->TexCoordPtr[t2]->size < 4) {
	 if (VB->TexCoordPtr[t2]->flags & VEC_NOT_WRITEABLE) {
	    VB->import_data( ctx, VERT_TEX2, VEC_NOT_WRITEABLE );
	 }
	 _mesa_vector4f_clean_elem( VB->TexCoordPtr[t2], VB->Count, 3 );
      }
   }

   if (DO_TEX1) {
      if (VB->TexCoordPtr[1]) {
	 const GLuint t1 = GET_TEXSOURCE(1);
	 tc1 = (GLuint (*)[4])VB->TexCoordPtr[t1]->data;
	 tc1_stride = VB->TexCoordPtr[t1]->stride;
	 if (DO_PTEX && VB->TexCoordPtr[t1]->size < 4) {
	    if (VB->TexCoordPtr[t1]->flags & VEC_NOT_WRITEABLE) {
	       VB->import_data( ctx, VERT_TEX1, VEC_NOT_WRITEABLE );
	    }
	    _mesa_vector4f_clean_elem( VB->TexCoordPtr[t1], VB->Count, 3 );
	 }
      } else {
	 tc1 = (GLuint (*)[4])&ctx->Current.Texcoord[1]; /* could be anything, really */
	 tc1_stride = 0;
      }
   }

   if (DO_TEX0) {
      if (VB->TexCoordPtr[0]) {
	 const GLuint t0 = GET_TEXSOURCE(0);
	 tc0_stride = VB->TexCoordPtr[t0]->stride;
	 tc0 = (GLuint (*)[4])VB->TexCoordPtr[t0]->data;
	 if (DO_PTEX && VB->TexCoordPtr[t0]->size < 4) {
	    if (VB->TexCoordPtr[t0]->flags & VEC_NOT_WRITEABLE) {
	       VB->import_data( ctx, VERT_TEX0, VEC_NOT_WRITEABLE );
	    }
	    _mesa_vector4f_clean_elem( VB->TexCoordPtr[t0], VB->Count, 3 );
	 }
      } else {
	 tc0 = (GLuint (*)[4])&ctx->Current.Texcoord[0]; /* could be anything, really */
	 tc0_stride = 0;
      }
	 
   }

   if (DO_NORM) {
      if (VB->NormalPtr) {
	 norm_stride = VB->NormalPtr->stride;
	 norm = (GLuint (*)[3])VB->NormalPtr->data;
      } else {
	 norm_stride = 0;
	 norm = (GLuint (*)[3])&ctx->Current.Normal;
      }
   }

   if (DO_RGBA) {
      if (VB->ColorPtr[0]) {
	 /* This is incorrect when colormaterial is enabled:
	  */
	 if (VB->ColorPtr[0]->Type != GL_UNSIGNED_BYTE) {
	    if (0) fprintf(stderr, "IMPORTING FLOAT COLORS\n");
	    IMPORT_FLOAT_COLORS( ctx );
	 }
	 col = (GLubyte (*)[4])VB->ColorPtr[0]->Ptr;
	 col_stride = VB->ColorPtr[0]->StrideB;
      } else {
	 col = &dummy; /* any old memory is fine */
	 col_stride = 0;
      }
      
   }

   if (DO_SPEC) {
      if (VB->SecondaryColorPtr[0]) {
	 if (VB->SecondaryColorPtr[0]->Type != GL_UNSIGNED_BYTE)
	    IMPORT_FLOAT_SPEC_COLORS( ctx );
	 spec = (GLubyte (*)[4])VB->SecondaryColorPtr[0]->Ptr;
	 spec_stride = VB->SecondaryColorPtr[0]->StrideB;
      } else {
	 spec = &dummy;
	 spec_stride = 0;
      }
	 
   }

   if (DO_FOG) {
      if (VB->FogCoordPtr) {
	 fog = VB->FogCoordPtr->data;
	 fog_stride = VB->FogCoordPtr->stride;
      } else {
	 fog = (GLfloat *)&dummy; *fog = 0;
	 fog_stride = 0;
      }
	      
   }
   
   
   if (VB->importable_data) {
      if (start) {
	 coord =  (GLuint (*)[4])((GLubyte *)coord + start * coord_stride);
	 if (DO_TEX0)
	    tc0 =  (GLuint (*)[4])((GLubyte *)tc0 + start * tc0_stride);
	 if (DO_TEX1) 
	    tc1 =  (GLuint (*)[4])((GLubyte *)tc1 + start * tc1_stride);
	 if (DO_TEX2) 
	    tc2 =  (GLuint (*)[4])((GLubyte *)tc2 + start * tc2_stride);
	 if (DO_NORM) 
	    norm =  (GLuint (*)[3])((GLubyte *)norm + start * norm_stride);
	 if (DO_RGBA) 
	    STRIDE_4UB(col, start * col_stride);
	 if (DO_SPEC)
	    STRIDE_4UB(spec, start * spec_stride);
	 if (DO_FOG)
	    STRIDE_F(fog, start * fog_stride);
      }

      for (i=start; i < end; i++) {
	 v[0].ui = coord[0][0];
	 v[1].ui = coord[0][1];
	 v[2].ui = coord[0][2];
	 if (TCL_DEBUG) fprintf(stderr, "%d: %.2f %.2f %.2f ", i, v[0].f, v[1].f, v[2].f);
	 if (DO_W) {
	    v[3].ui = coord[0][3];
	    if (TCL_DEBUG) fprintf(stderr, "%.2f ", v[3].f);
	    v += 4;
	 } 
	 else
	    v += 3;
	 coord =  (GLuint (*)[4])((GLubyte *)coord +  coord_stride);

	 if (DO_NORM) {
	    v[0].ui = norm[0][0];
	    v[1].ui = norm[0][1];
	    v[2].ui = norm[0][2];
	    if (TCL_DEBUG) fprintf(stderr, "norm: %.2f %.2f %.2f ", v[0].f, v[1].f, v[2].f);
	    v += 3;
	    norm =  (GLuint (*)[3])((GLubyte *)norm +  norm_stride);
	 }
	 if (DO_RGBA) {
	    v[0].ui = LE32_TO_CPU(*(GLuint *)&col[0]);
	    STRIDE_4UB(col, col_stride);
	    if (TCL_DEBUG) fprintf(stderr, "%x ", v[0].ui);
	    v++;
	 }
	 if (DO_SPEC || DO_FOG) {
	    if (DO_SPEC) {
	       v[0].ub[0] = spec[0][0];
	       v[0].ub[1] = spec[0][1];
	       v[0].ub[2] = spec[0][2];
	       STRIDE_4UB(spec, spec_stride);
	    }
	    if (DO_FOG) {
	       v[0].ub[3] = fog[0] * 255.0;
	       STRIDE_F(fog, fog_stride);
	    }
	    if (TCL_DEBUG) fprintf(stderr, "%x ", v[0].ui);
	    v++;
	 }
	 if (DO_TEX0) {
	    v[0].ui = tc0[0][0];
	    v[1].ui = tc0[0][1];
	    if (TCL_DEBUG) fprintf(stderr, "t0: %.2f %.2f ", v[0].f, v[1].f);
	    if (DO_PTEX) {
	       v[2].ui = tc0[0][3];
	       if (TCL_DEBUG) fprintf(stderr, "%.2f ", v[2].f);
	       v += 3;
	    } 
	    else
	       v += 2;
	    tc0 =  (GLuint (*)[4])((GLubyte *)tc0 +  tc0_stride);
	 }
	 if (DO_TEX1) {
	    v[0].ui = tc1[0][0];
	    v[1].ui = tc1[0][1];
	    if (TCL_DEBUG) fprintf(stderr, "t1: %.2f %.2f ", v[0].f, v[1].f);
	    if (DO_PTEX) {
	       v[2].ui = tc1[0][3];
	       if (TCL_DEBUG) fprintf(stderr, "%.2f ", v[2].f);
	       v += 3;
	    } 
	    else
	       v += 2;
	    tc1 =  (GLuint (*)[4])((GLubyte *)tc1 +  tc1_stride);
	 } 
	 if (DO_TEX2) {
	    v[0].ui = tc2[0][0];
	    v[1].ui = tc2[0][1];
	    if (DO_PTEX) {
	       v[2].ui = tc2[0][3];
	       v += 3;
	    } 
	    else
	       v += 2;
	    tc2 =  (GLuint (*)[4])((GLubyte *)tc2 +  tc2_stride);
	 } 
	 if (TCL_DEBUG) fprintf(stderr, "\n");
      }
   } else {
      for (i=start; i < end; i++) {
	 v[0].ui = coord[i][0];
	 v[1].ui = coord[i][1];
	 v[2].ui = coord[i][2];
	 if (DO_W) {
	    v[3].ui = coord[i][3];
	    v += 4;
	 } 
	 else
	    v += 3;

	 if (DO_NORM) {
	    v[0].ui = norm[i][0];
	    v[1].ui = norm[i][1];
	    v[2].ui = norm[i][2];
	    v += 3;
	 }
	 if (DO_RGBA) {
	    v[0].ui = LE32_TO_CPU(*(GLuint *)&col[i]);
	    v++;
	 }
	 if (DO_SPEC || DO_FOG) {
	    if (DO_SPEC) {
	       v[0].ub[0] = spec[i][0];
	       v[0].ub[1] = spec[i][1];
	       v[0].ub[2] = spec[i][2];
	    }
	    if (DO_FOG) {
	       v[0].ub[3] = fog[i] * 255.0;
	    }
	    v++;
	 }
	 if (DO_TEX0) {
	    v[0].ui = tc0[i][0];
	    v[1].ui = tc0[i][1];
	    if (DO_PTEX) {
	       v[2].ui = tc0[i][3];
	       v += 3;
	    } 
	    else
	       v += 2;
	 }
	 if (DO_TEX1) {
	    v[0].ui = tc1[i][0];
	    v[1].ui = tc1[i][1];
	    if (DO_PTEX) {
	       v[2].ui = tc1[i][3];
	       v += 3;
	    } 
	    else
	       v += 2;
	 } 
	 if (DO_TEX2) {
	    v[0].ui = tc2[i][0];
	    v[1].ui = tc2[i][1];
	    if (DO_PTEX) {
	       v[2].ui = tc2[i][3];
	       v += 3;
	    } 
	    else
	       v += 2;
	 } 
      }
   }
}



static void TAG(init)( void )
{
   int sz = 3;
   if (DO_W) sz++;
   if (DO_NORM) sz += 3;
   if (DO_RGBA) sz++;
   if (DO_SPEC || DO_FOG) sz++;
   if (DO_TEX0) sz += 2;
   if (DO_TEX0 && DO_PTEX) sz++;
   if (DO_TEX1) sz += 2;
   if (DO_TEX1 && DO_PTEX) sz++;
   if (DO_TEX2) sz += 2;
   if (DO_TEX2 && DO_PTEX) sz++;

   setup_tab[IDX].emit = TAG(emit);
   setup_tab[IDX].vertex_format = IND;
   setup_tab[IDX].vertex_size = sz;
}


#undef IND
#undef TAG
#undef IDX
