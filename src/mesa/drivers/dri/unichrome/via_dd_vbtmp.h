/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
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
 *        float u2, v2;
 *        float u3, v3;
 *    } v;
 *    struct {
 *        float x, y, z, w;
 *        struct { char r, g, b, a; } color;
 *        struct { char r, g, b, fog; } spec;
 *        float u0, v0, q0;
 *        float u1, v1, q1;
 *        float u2, v2, q2;
 *        float u3, v3, q3;
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

 * DO_XYZW:  Emit xyz and maybe w coordinates.
 * DO_RGBA:  Emit color.
 * DO_SPEC:  Emit specular color.
 * DO_FOG:   Emit fog coordinate in specular alpha.
 * DO_TEX0:  Emit tex0 u,v coordinates.
 * DO_TEX1:  Emit tex1 u,v coordinates.
 * DO_TEX2:  Emit tex2 u,v coordinates.
 * DO_TEX3:  Emit tex3 u,v coordinates.
 * DO_PTEX:  Emit tex0,1,2,3 q coordinates where possible.
 *
 * HAVE_RGBA_COLOR: Hardware takes color in rgba order (else bgra).
 *
 * HAVE_HW_VIEWPORT:  Hardware performs viewport transform.
 * HAVE_HW_DIVIDE:  Hardware performs perspective divide.
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

#if (HAVE_HW_VIEWPORT)
#define VIEWPORT_X(dst, x) dst = x
#define VIEWPORT_Y(dst, y) dst = y
#define VIEWPORT_Z(dst, z) dst = z
#else
#define VIEWPORT_X(dst, x) dst = s[0] * x + s[12]
#define VIEWPORT_Y(dst, y) dst = s[5] * y + s[13]
#define VIEWPORT_Z(dst, z) dst = s[10] * z + s[14]
#endif

#if (HAVE_HW_DIVIDE && !HAVE_PTEX_VERTICES)
#error "can't cope with this combination" 
#endif 

#ifndef LOCALVARS
#define LOCALVARS
#endif

#ifndef CHECK_HW_DIVIDE
#define CHECK_HW_DIVIDE 1
#endif

#if (HAVE_HW_DIVIDE || DO_SPEC || DO_TEX0 || DO_FOG || !HAVE_TINY_VERTICES)

static void TAG(emit)(GLcontext *ctx,
		      GLuint start, GLuint end,
		      void *dest,
		      GLuint stride)
{
    LOCALVARS
    struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
    GLfloat (*tc0)[4], (*tc1)[4], (*fog)[4];
    GLfloat (*tc2)[4], (*tc3)[4];
    GLfloat (*col)[4], (*spec)[4];
    GLuint tc0_stride, tc1_stride, col_stride, spec_stride, fog_stride;
    GLuint tc2_stride, tc3_stride;
    GLuint tc0_size, tc1_size;
    GLuint tc2_size, tc3_size;
    GLfloat (*coord)[4];
    GLuint coord_stride;
    VERTEX *v = (VERTEX *)dest;
    const GLfloat *s = GET_VIEWPORT_MAT();
    const GLubyte *mask = VB->ClipMask;
    int i;

    if (VIA_DEBUG) fprintf(stderr, "TAG-emit for HAVE_HW_DIVIDE || DO_SPEC || DO_TEX0 || DO_FOG || !HAVE_TINY_VERTICE\n");
    
    if (HAVE_HW_VIEWPORT && HAVE_HW_DIVIDE && CHECK_HW_DIVIDE) {
        (void) s;
        coord = VB->ClipPtr->data;
        coord_stride = VB->ClipPtr->stride;
    }
    else {
        coord = VB->NdcPtr->data;
        coord_stride = VB->NdcPtr->stride;
    }

    if (DO_TEX3) {
        const GLuint t3 = GET_TEXSOURCE(3);
        tc3 = VB->TexCoordPtr[t3]->data;
        tc3_stride = VB->TexCoordPtr[t3]->stride;
        if (DO_PTEX)
	    tc3_size = VB->TexCoordPtr[t3]->size;
    }

    if (DO_TEX2) {
        const GLuint t2 = GET_TEXSOURCE(2);
        tc2 = VB->TexCoordPtr[t2]->data;
        tc2_stride = VB->TexCoordPtr[t2]->stride;
        if (DO_PTEX)
	    tc2_size = VB->TexCoordPtr[t2]->size;
    }

    if (DO_TEX1) {
        const GLuint t1 = GET_TEXSOURCE(1);
        tc1 = VB->TexCoordPtr[t1]->data;
        tc1_stride = VB->TexCoordPtr[t1]->stride;
        if (DO_PTEX)
	    tc1_size = VB->TexCoordPtr[t1]->size;
    }

    if (DO_TEX0) {
        const GLuint t0 = GET_TEXSOURCE(0);
        /* test */
	tc0_stride = VB->TexCoordPtr[t0]->stride;
    	tc0 = VB->TexCoordPtr[t0]->data;
    	if (DO_PTEX) 
	    tc0_size = VB->TexCoordPtr[t0]->size;
    }

    if (DO_RGBA) {
        col = VB->ColorPtr[0]->data;
        col_stride = VB->ColorPtr[0]->stride;
    }

    if (DO_SPEC) {
        spec = VB->SecondaryColorPtr[0]->data;
        spec_stride = VB->SecondaryColorPtr[0]->stride;
    }

    if (DO_FOG) {
      if (VB->FogCoordPtr) {
	 fog = VB->FogCoordPtr->data;
	 fog_stride = VB->FogCoordPtr->stride;
      }
      else {
	 static GLfloat tmp[4] = { 0, 0, 0, 0 };
	 fog = &tmp;
	 fog_stride = 0;
      }
   }

    /* May have nonstandard strides:
     */
    if (start) {
        STRIDE_4F(coord, start * coord_stride);
        if (DO_TEX0)
            STRIDE_4F(tc0, start * tc0_stride);
        if (DO_TEX1) 
            STRIDE_4F(tc1, start * tc1_stride);
        if (DO_TEX2) 
            STRIDE_4F(tc2, start * tc2_stride);
        if (DO_TEX3) 
            STRIDE_4F(tc3, start * tc3_stride);
        if (DO_RGBA) 
            STRIDE_4F(col, start * col_stride);
        if (DO_SPEC)
            STRIDE_4F(spec, start * spec_stride);
        if (DO_FOG)
            STRIDE_4F(fog, start * fog_stride);
    }

    for (i = start; i < end; i++, v = (VERTEX *)((GLubyte *)v + stride)) {
        if (DO_XYZW) {
            if (HAVE_HW_VIEWPORT || mask[i] == 0) {
                VIEWPORT_X(v->v.x, coord[0][0]);
                VIEWPORT_Y(v->v.y, coord[0][1]);
                VIEWPORT_Z(v->v.z, coord[0][2]);
            }
            v->v.w = coord[0][3];		
            STRIDE_4F(coord, coord_stride);
        }
        if (DO_RGBA) {
            UNCLAMPED_FLOAT_TO_UBYTE(v->v.color.red, col[0][0]);
            UNCLAMPED_FLOAT_TO_UBYTE(v->v.color.green, col[0][1]);
            UNCLAMPED_FLOAT_TO_UBYTE(v->v.color.blue, col[0][2]);
            UNCLAMPED_FLOAT_TO_UBYTE(v->v.color.alpha, col[0][3]);
            STRIDE_4F(col, col_stride);
        }

        if (DO_SPEC) {
            UNCLAMPED_FLOAT_TO_UBYTE(v->v.specular.red, spec[0][0]);
            UNCLAMPED_FLOAT_TO_UBYTE(v->v.specular.green, spec[0][1]);
            UNCLAMPED_FLOAT_TO_UBYTE(v->v.specular.blue, spec[0][2]);
            STRIDE_4F(spec, spec_stride);
        }
        else { 
            v->v.specular.red = 0;
            v->v.specular.green = 0;
            v->v.specular.blue = 0;
        }

        if (DO_FOG) {
            UNCLAMPED_FLOAT_TO_UBYTE(v->v.specular.alpha, fog[0][0]);
            /*=* [DBG]  exy : fix lighting on + fog off error *=*/
            STRIDE_4F(fog, fog_stride);
        }
        else {
            v->v.specular.alpha = 0;
        }

        if (DO_TEX0) {
            v->v.u0 = tc0[0][0];
            v->v.v0 = tc0[0][1];
            if (DO_PTEX) {
                if (HAVE_PTEX_VERTICES) {
                    if (tc0_size == 4) 
                        v->pv.q0 = tc0[0][3];
                    else
                        v->pv.q0 = 1.0;
                } 
                else if (tc0_size == 4) {
                    float rhw = 1.0 / tc0[0][3];
                    v->v.w *= tc0[0][3];
                    v->v.u0 *= rhw;
                    v->v.v0 *= rhw;
                }
            }
            STRIDE_4F(tc0, tc0_stride);
        }
        if (DO_TEX1) {
            if (DO_PTEX && HAVE_PTEX_VERTICES) {		    
                v->pv.u1 = tc1[0][0];
                v->pv.v1 = tc1[0][1];		    
                if (tc1_size == 4) 
                    v->pv.q1 = tc1[0][3];
                else
                    v->pv.q1 = 1.0;
            } 
            else {
                v->v.u1 = tc1[0][0];
                v->v.v1 = tc1[0][1];
            }
            STRIDE_4F(tc1, tc1_stride);
        } 
        else if (DO_PTEX) {
            *(GLuint *)&v->pv.q1 = 0;
        }
        if (DO_TEX2) {
            if (DO_PTEX) {
                v->pv.u2 = tc2[0][0];
                v->pv.v2 = tc2[0][1];
                if (tc2_size == 4) 
                    v->pv.q2 = tc2[0][3];
                else
                    v->pv.q2 = 1.0;
            } 
            else {
                v->v.u2 = tc2[0][0];
                v->v.v2 = tc2[0][1];
            }
            STRIDE_4F(tc2, tc2_stride);
        } 
        if (DO_TEX3) {
            if (DO_PTEX) {
                v->pv.u3 = tc3[0][0];
                v->pv.v3 = tc3[0][1];
                if (tc3_size == 4) 
                    v->pv.q3 = tc3[0][3];
                else
                    v->pv.q3 = 1.0;
            } 
            else {
                v->v.u3 = tc3[0][0];
                v->v.v3 = tc3[0][1];
            }
            STRIDE_4F(tc3, tc3_stride);
        } 
    }
}
#else
#if DO_XYZW

#if HAVE_HW_DIVIDE
#error "cannot use tiny vertices with hw perspective divide"
#endif

static void TAG(emit)(GLcontext *ctx, GLuint start, GLuint end,
		      void *dest, GLuint stride)
{
    LOCALVARS
    struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
    GLfloat (*col)[4];
    GLuint col_stride;
    GLfloat (*coord)[4] = VB->NdcPtr->data;
    GLuint coord_stride = VB->NdcPtr->stride;
    GLfloat *v = (GLfloat *)dest;
    const GLubyte *mask = VB->ClipMask;
    const GLfloat *s = GET_VIEWPORT_MAT();
    int i;

    (void) s;

    /*ASSERT(stride == 4);*/
    if (VIA_DEBUG) {
	fprintf(stderr, "TAG-emit for DO_XYZW\n");
	fprintf(stderr, "%s\n", __FUNCTION__);	
    }	

    col = VB->ColorPtr[0]->data;
    col_stride = VB->ColorPtr[0]->stride;

    if (start) {
        STRIDE_4F(coord, start * coord_stride);
        STRIDE_4F(col, start * col_stride);
    }

    for (i = start; i < end; i++, v += 4) {
        if (DO_XYZW) {
            if (HAVE_HW_VIEWPORT || mask[i] == 0) {
                VIEWPORT_X(v[0], coord[0][0]);
                VIEWPORT_Y(v[1], coord[0][1]);
                VIEWPORT_Z(v[2], coord[0][2]);
            }
            STRIDE_4F(coord, coord_stride);
        }
        if (DO_RGBA) {
            VERTEX_COLOR *c = (VERTEX_COLOR *)&v[3];
            UNCLAMPED_FLOAT_TO_UBYTE(c->red, col[0][0]);
            UNCLAMPED_FLOAT_TO_UBYTE(c->green, col[0][1]);
            UNCLAMPED_FLOAT_TO_UBYTE(c->blue, col[0][2]);
            UNCLAMPED_FLOAT_TO_UBYTE(c->alpha, col[0][3]);
            STRIDE_4F( col, col_stride );
        }
    }
}
#else
static void TAG(emit)(GLcontext *ctx, GLuint start, GLuint end,
		      void *dest, GLuint stride)
{
    LOCALVARS
    struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
    GLubyte (*col)[4];
    GLuint col_stride;
    GLfloat *v = (GLfloat *)dest;
    int i;
    if (VIA_DEBUG) {
	fprintf(stderr, "TAG-emit for No DO_XYZW\n");
	fprintf(stderr, "%s\n", __FUNCTION__);
    }

    if (VB->ColorPtr[0]->Type != GL_UNSIGNED_BYTE)
        IMPORT_FLOAT_COLORS( ctx );

    col = VB->ColorPtr[0]->Ptr;
    col_stride = VB->ColorPtr[0]->StrideB;

    if (start)
        STRIDE_4UB(col, col_stride * start);

    /* Need to figure out where color is:
     */
    if (GET_VERTEX_FORMAT() == TINY_VERTEX_FORMAT)
        v += 3;
    else
        v += 4;

    for (i = start; i < end; i++, STRIDE_F(v, stride)) {
        if (HAVE_RGBA_COLOR) {
	    *(GLuint *)v = *(GLuint *)col[0];
        }
        else {
	    GLubyte *b = (GLubyte *)v;
	    b[0] = col[0][2];
	    b[1] = col[0][1];
	    b[2] = col[0][0];
	    b[3] = col[0][3];
        }
        STRIDE_4UB(col, col_stride);
    }
}
#endif /* emit */
#endif /* emit */

#if (DO_XYZW) && (DO_RGBA)

#if (HAVE_PTEX_VERTICES)
static GLboolean TAG(check_tex_sizes)(GLcontext *ctx)
{
    LOCALVARS
    struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

    /* Force 'missing' texcoords to something valid.
     */
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
    if (DO_TEX3 && VB->TexCoordPtr[2] == 0)
        VB->TexCoordPtr[2] = VB->TexCoordPtr[3];

    if (DO_TEX2 && VB->TexCoordPtr[1] == 0)
        VB->TexCoordPtr[1] = VB->TexCoordPtr[2];

    if (DO_TEX1 && VB->TexCoordPtr[0] == 0)
        VB->TexCoordPtr[0] = VB->TexCoordPtr[1];

    if (DO_PTEX)
        return GL_TRUE;
   
    if ((DO_TEX3 && VB->TexCoordPtr[GET_TEXSOURCE(3)]->size == 4) ||
        (DO_TEX2 && VB->TexCoordPtr[GET_TEXSOURCE(2)]->size == 4) ||
        (DO_TEX1 && VB->TexCoordPtr[GET_TEXSOURCE(1)]->size == 4) ||
        (DO_TEX0 && VB->TexCoordPtr[GET_TEXSOURCE(0)]->size == 4))
        return GL_FALSE;
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);
    return GL_TRUE;
}
#else
static GLboolean TAG(check_tex_sizes)(GLcontext *ctx)
{
    LOCALVARS
    struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

    /* Force 'missing' texcoords to something valid.
     */
    if (DO_TEX3 && VB->TexCoordPtr[2] == 0)
        VB->TexCoordPtr[2] = VB->TexCoordPtr[3];

    if (DO_TEX2 && VB->TexCoordPtr[1] == 0)
        VB->TexCoordPtr[1] = VB->TexCoordPtr[2];

    if (DO_TEX1 && VB->TexCoordPtr[0] == 0) {
        VB->TexCoordPtr[0] = VB->TexCoordPtr[1];
    }

    if (DO_PTEX)
        return GL_TRUE;

    if ((DO_TEX3 && VB->TexCoordPtr[GET_TEXSOURCE(3)]->size == 4) ||
        (DO_TEX2 && VB->TexCoordPtr[GET_TEXSOURCE(2)]->size == 4) ||
        (DO_TEX1 && VB->TexCoordPtr[GET_TEXSOURCE(1)]->size == 4)) {
        /*PTEX_FALLBACK();*/
        return GL_FALSE;
    }
    
    if (DO_TEX0 && VB->TexCoordPtr[GET_TEXSOURCE(0)]->size == 4) {
        if (DO_TEX1 || DO_TEX2 || DO_TEX3) {
	    /*PTEX_FALLBACK();*/
        }
        return GL_FALSE;
    }
    return GL_TRUE;
}
#endif /* ptex */


static void TAG(interp)(GLcontext *ctx,
			GLfloat t,
			GLuint edst, GLuint eout, GLuint ein,
			GLboolean force_boundary)
{
    LOCALVARS
    struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
    GLubyte *ddverts = GET_VERTEX_STORE();
    GLuint shift = GET_VERTEX_STRIDE_SHIFT();
    const GLfloat *dstclip = VB->ClipPtr->data[edst];
    GLfloat w;
    const GLfloat *s = GET_VIEWPORT_MAT();
    
    VERTEX *dst = (VERTEX *)(ddverts + (edst << shift));
    VERTEX *in = (VERTEX *)(ddverts + (ein << shift));
    VERTEX *out = (VERTEX *)(ddverts + (eout << shift));

    (void)s;
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
    if (HAVE_HW_DIVIDE && CHECK_HW_DIVIDE) {
        VIEWPORT_X(dst->v.x, dstclip[0]);
        VIEWPORT_Y(dst->v.y, dstclip[1]);
        VIEWPORT_Z(dst->v.z, dstclip[2]);
        w = dstclip[3];
    }
    else {
        w = 1.0 / dstclip[3];
        VIEWPORT_X(dst->v.x, dstclip[0] * w);
        VIEWPORT_Y(dst->v.y, dstclip[1] * w);
        VIEWPORT_Z(dst->v.z, dstclip[2] * w);
    }

    if ((HAVE_HW_DIVIDE && CHECK_HW_DIVIDE) || 
        DO_FOG || DO_SPEC || DO_TEX0 || DO_TEX1 ||
        DO_TEX2 || DO_TEX3 || !HAVE_TINY_VERTICES) {

        dst->v.w = w;
	
        INTERP_UB(t, dst->ub4[4][0], out->ub4[4][0], in->ub4[4][0]);
        INTERP_UB(t, dst->ub4[4][1], out->ub4[4][1], in->ub4[4][1]);
        INTERP_UB(t, dst->ub4[4][2], out->ub4[4][2], in->ub4[4][2]);
        INTERP_UB(t, dst->ub4[4][3], out->ub4[4][3], in->ub4[4][3]);
	
        if (DO_SPEC) {
	    INTERP_UB(t, dst->ub4[5][0], out->ub4[5][0], in->ub4[5][0]);
	    INTERP_UB(t, dst->ub4[5][1], out->ub4[5][1], in->ub4[5][1]);
	    INTERP_UB(t, dst->ub4[5][2], out->ub4[5][2], in->ub4[5][2]);
        }
        if (DO_FOG) {
	    INTERP_UB(t, dst->ub4[5][3], out->ub4[5][3], in->ub4[5][3]);
        }
        if (DO_TEX0) {
	    if (DO_PTEX) {
		if (HAVE_PTEX_VERTICES) {
	    	    INTERP_F(t, dst->pv.u0, out->pv.u0, in->pv.u0);
	    	    INTERP_F(t, dst->pv.v0, out->pv.v0, in->pv.v0);
	    	    INTERP_F(t, dst->pv.q0, out->pv.q0, in->pv.q0);
		} 
		else {
	    	    INTERP_F(t, dst->v.u0, out->v.u0, in->v.u0);
	    	    INTERP_F(t, dst->v.v0, out->v.v0, in->v.v0);
		}
	    }
	    else {
		INTERP_F(t, dst->v.u0, out->v.u0, in->v.u0);
		INTERP_F(t, dst->v.v0, out->v.v0, in->v.v0);
	    }
        }
        if (DO_TEX1) {
	    if (DO_PTEX) {
		if (HAVE_PTEX_VERTICES) {	    
		    INTERP_F(t, dst->pv.u1, out->pv.u1, in->pv.u1);
		    INTERP_F(t, dst->pv.v1, out->pv.v1, in->pv.v1);
		    INTERP_F(t, dst->pv.q1, out->pv.q1, in->pv.q1);
		}
		else {
		    INTERP_F(t, dst->v.u1, out->v.u1, in->v.u1);
		    INTERP_F(t, dst->v.v1, out->v.v1, in->v.v1);
		}		    
	    } 
	    else {
		INTERP_F(t, dst->v.u1, out->v.u1, in->v.u1);
		INTERP_F(t, dst->v.v1, out->v.v1, in->v.v1);
	    }
        }
        else if (DO_PTEX) {
	    dst->pv.q0 = 0.0;	/* must be a valid float on radeon */
        }
        if (DO_TEX2) {
	    if (DO_PTEX) {
		INTERP_F(t, dst->pv.u2, out->pv.u2, in->pv.u2);
		INTERP_F(t, dst->pv.v2, out->pv.v2, in->pv.v2);
		INTERP_F(t, dst->pv.q2, out->pv.q2, in->pv.q2);
	    } 
	    else {
		INTERP_F(t, dst->v.u2, out->v.u2, in->v.u2);
		INTERP_F(t, dst->v.v2, out->v.v2, in->v.v2);
	    }
        }
        if (DO_TEX3) {
	    if (DO_PTEX) {
		INTERP_F(t, dst->pv.u3, out->pv.u3, in->pv.u3);
		INTERP_F(t, dst->pv.v3, out->pv.v3, in->pv.v3);
		INTERP_F(t, dst->pv.q3, out->pv.q3, in->pv.q3);
	    } 
	    else {
		INTERP_F(t, dst->v.u3, out->v.u3, in->v.u3);
		INTERP_F(t, dst->v.v3, out->v.v3, in->v.v3);
	    }
        }
    } 
    else {
        /* 4-dword vertex.  Color is in v[3] and there is no oow coordinate.
         */
        INTERP_UB(t, dst->ub4[3][0], out->ub4[3][0], in->ub4[3][0]);
        INTERP_UB(t, dst->ub4[3][1], out->ub4[3][1], in->ub4[3][1]);
        INTERP_UB(t, dst->ub4[3][2], out->ub4[3][2], in->ub4[3][2]);
        INTERP_UB(t, dst->ub4[3][3], out->ub4[3][3], in->ub4[3][3]);
    }
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}

#endif /* rgba && xyzw */

static void TAG(init)(void)
{
    setup_tab[IND].emit = TAG(emit);

#if (DO_XYZW && DO_RGBA)
    setup_tab[IND].check_tex_sizes = TAG(check_tex_sizes);
    setup_tab[IND].interp = TAG(interp);
#endif

    if (DO_SPEC)
        setup_tab[IND].copyPv = copy_pv_rgba4_spec5;
     else if (HAVE_HW_DIVIDE || DO_SPEC || DO_FOG || DO_TEX0 || DO_TEX1 ||
	      DO_TEX2 || DO_TEX3 || !HAVE_TINY_VERTICES)
        setup_tab[IND].copyPv = copy_pv_rgba4;
    else
        setup_tab[IND].copyPv = copy_pv_rgba3;

    if (DO_TEX3) {
        if (DO_PTEX && HAVE_PTEX_VERTICES) {
	    ASSERT(HAVE_PTEX_VERTICES);
	    setup_tab[IND].vertexFormat = PROJ_TEX3_VERTEX_FORMAT;
	    setup_tab[IND].vertexSize = 18;
	    setup_tab[IND].vertexStrideShift = 7;
        }
        else {
	    setup_tab[IND].vertexFormat = TEX3_VERTEX_FORMAT;
	    setup_tab[IND].vertexSize = 14;
	    setup_tab[IND].vertexStrideShift = 6;
        }
    }
    else if (DO_TEX2) {
        if (DO_PTEX && HAVE_PTEX_VERTICES) {
	    ASSERT(HAVE_PTEX_VERTICES);
	    setup_tab[IND].vertexFormat = PROJ_TEX3_VERTEX_FORMAT;
	    setup_tab[IND].vertexSize = 18;
	    setup_tab[IND].vertexStrideShift = 7;
        }
        else {
	    setup_tab[IND].vertexFormat = TEX2_VERTEX_FORMAT;
	    setup_tab[IND].vertexSize = 12;
	    setup_tab[IND].vertexStrideShift = 6;
        }
    }
    else if (DO_TEX1) {
        if (DO_PTEX && HAVE_PTEX_VERTICES) {
	    ASSERT(HAVE_PTEX_VERTICES);
	    setup_tab[IND].vertexFormat = PROJ_TEX1_VERTEX_FORMAT;
	    setup_tab[IND].vertexSize = 12;
	    setup_tab[IND].vertexStrideShift = 6;
        }
        else {
	    setup_tab[IND].vertexFormat = TEX1_VERTEX_FORMAT;
	    setup_tab[IND].vertexSize = 10;
	    setup_tab[IND].vertexStrideShift = 6;
        }
    }
    else if (DO_TEX0) {
        if (DO_PTEX && HAVE_PTEX_VERTICES) {
	    setup_tab[IND].vertexFormat = PROJ_TEX1_VERTEX_FORMAT;
	    setup_tab[IND].vertexSize = 12;
	    setup_tab[IND].vertexStrideShift = 6;
        } 
	else {
	    setup_tab[IND].vertexFormat = TEX0_VERTEX_FORMAT;
	    setup_tab[IND].vertexSize = 8;
	    setup_tab[IND].vertexStrideShift = 5;
        }
    }
    else if (!HAVE_HW_DIVIDE && !DO_SPEC && !DO_FOG && HAVE_TINY_VERTICES) {
        setup_tab[IND].vertexFormat = TINY_VERTEX_FORMAT;
        setup_tab[IND].vertexSize = 4;
        setup_tab[IND].vertexStrideShift = 4;
    } 
    else if (HAVE_NOTEX_VERTICES) {
        setup_tab[IND].vertexFormat = NOTEX_VERTEX_FORMAT;
        setup_tab[IND].vertexSize = 6;
        setup_tab[IND].vertexStrideShift = 5;
    } 
    else {
        setup_tab[IND].vertexFormat = TEX0_VERTEX_FORMAT;
        setup_tab[IND].vertexSize = 8;
        setup_tab[IND].vertexStrideShift = 5;
    }

    assert(setup_tab[IND].vertexSize * 4 <=
           1 << setup_tab[IND].vertexStrideShift);
}

#undef IND
#undef TAG
