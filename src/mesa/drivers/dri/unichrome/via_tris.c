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

#include <stdio.h>
#include <math.h>

#include "glheader.h"
#include "context.h"
#include "mtypes.h"
#include "macros.h"
#include "colormac.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#include "via_context.h"
#include "via_tris.h"
#include "via_state.h"
#include "via_vb.h"
#include "via_ioctl.h"

static void viaRenderPrimitive(GLcontext *ctx, GLenum prim);
GLuint RasterCounter = 0;
extern GLuint idle;
extern GLuint busy;
/***********************************************************************
 *                    Emit primitives as inline vertices               *
 ***********************************************************************/

#if defined(USE_X86_ASM)
#define COPY_DWORDS(j, vb, vertsize, v)                                 \
    do {                                                                \
        int __tmp;                                                      \
        __asm__ __volatile__("rep ; movsl"                              \
                              : "=%c" (j), "=D" (vb), "=S" (__tmp)      \
                              : "0" (vertsize),                         \
                                "D" ((long)vb),                         \
                                "S" ((long)v));                         \
    } while (0)
#else
#define COPY_DWORDS(j, vb, vertsize, v)                                 \
    do {                                                                \
        for (j = 0; j < vertsize; j++)                                  \
            vb[j] = ((GLuint *)v)[j];                                   \
        vb += vertsize;                                                 \
    } while (0)
#endif

static void __inline__ via_draw_triangle(viaContextPtr vmesa,
                                         viaVertexPtr v0,
                                         viaVertexPtr v1,
                                         viaVertexPtr v2)
{
    GLuint vertsize = vmesa->vertexSize;
    GLuint *vb = viaCheckDma(vmesa, 3 * 4 * vertsize);
    int j;
    
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
#ifdef PERFORMANCE_MEASURE
    if (VIA_PERFORMANCE) P_M;
#endif
    COPY_DWORDS(j, vb, vertsize, v0);
    COPY_DWORDS(j, vb, vertsize, v1);
    COPY_DWORDS(j, vb, vertsize, v2);
    vmesa->dmaLow += 3 * 4 * vertsize;
    vmesa->primitiveRendered = GL_TRUE;
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}


static void __inline__ via_draw_quad(viaContextPtr vmesa,
                                     viaVertexPtr v0,
                                     viaVertexPtr v1,
                                     viaVertexPtr v2,
                                     viaVertexPtr v3)
{
    GLuint vertsize = vmesa->vertexSize;
    GLuint *vb = viaCheckDma(vmesa, 6 * 4 * vertsize);
    int j;
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
#ifdef PERFORMANCE_MEASURE
    if (VIA_PERFORMANCE) P_M;
#endif    
    COPY_DWORDS(j, vb, vertsize, v0);
    COPY_DWORDS(j, vb, vertsize, v1);
    COPY_DWORDS(j, vb, vertsize, v3);
    COPY_DWORDS(j, vb, vertsize, v1);
    COPY_DWORDS(j, vb, vertsize, v2);
    COPY_DWORDS(j, vb, vertsize, v3);
    vmesa->dmaLow += 6 * 4 * vertsize;
    vmesa->primitiveRendered = GL_TRUE;    
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}


static __inline__ void via_draw_point(viaContextPtr vmesa,
                                      viaVertexPtr v0)
{
    /*GLfloat sz = vmesa->glCtx->Point._Size * .5;*/
    int vertsize = vmesa->vertexSize;
    /*GLuint *vb = viaCheckDma(vmesa, 2 * 4 * vertsize);*/
    GLuint *vb = viaCheckDma(vmesa, 4 * vertsize);
    int j;
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
#ifdef PERFORMANCE_MEASURE
    if (VIA_PERFORMANCE) P_M;
#endif
    COPY_DWORDS(j, vb, vertsize, v0);
    vmesa->dmaLow += 4 * vertsize;
    vmesa->primitiveRendered = GL_TRUE;
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}


/*
 *	Draw line in hardware.
 *	Checked out - AC.
 */
static __inline__ void via_draw_line(viaContextPtr vmesa,
                                     viaVertexPtr v0,
                                     viaVertexPtr v1)
{
    GLuint vertsize = vmesa->vertexSize;
    GLuint *vb = viaCheckDma(vmesa, 2 * 4 * vertsize);
    int j;
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
#ifdef PERFORMANCE_MEASURE
    if (VIA_PERFORMANCE) P_M;
#endif    
    COPY_DWORDS(j, vb, vertsize, v0);
    COPY_DWORDS(j, vb, vertsize, v1);
    vmesa->dmaLow += 2 * 4 * vertsize;
    vmesa->primitiveRendered = GL_TRUE;    
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);
}


/***********************************************************************
 *          Macros for via_dd_tritmp.h to draw basic primitives        *
 ***********************************************************************/

#define TRI(a, b, c)                                \
    do {                                            \
        if (VIA_DEBUG) fprintf(stderr, "hw TRI\n"); \
        if (DO_FALLBACK)                            \
            vmesa->drawTri(vmesa, a, b, c);         \
        else                                        \
            via_draw_triangle(vmesa, a, b, c);      \
    } while (0)

#define QUAD(a, b, c, d)                            \
    do {                                            \
        if (VIA_DEBUG) fprintf(stderr, "hw QUAD\n");\
        if (DO_FALLBACK) {                          \
            vmesa->drawTri(vmesa, a, b, d);         \
            vmesa->drawTri(vmesa, b, c, d);         \
        }                                           \
        else                                        \
            via_draw_quad(vmesa, a, b, c, d);       \
    } while (0)

#define LINE(v0, v1)                                \
    do {                                            \
        if(VIA_DEBUG) fprintf(stderr, "hw LINE\n");\
        if (DO_FALLBACK)                            \
            vmesa->drawLine(vmesa, v0, v1);         \
        else                                        \
            via_draw_line(vmesa, v0, v1);           \
    } while (0)

#define POINT(v0)                                    \
    do {                                             \
        if (VIA_DEBUG) fprintf(stderr, "hw POINT\n");\
        if (DO_FALLBACK)                             \
            vmesa->drawPoint(vmesa, v0);             \
        else                                         \
            via_draw_point(vmesa, v0);               \
    } while (0)


/***********************************************************************
 *              Build render functions from dd templates               *
 ***********************************************************************/

#define VIA_OFFSET_BIT         0x01
#define VIA_TWOSIDE_BIT        0x02
#define VIA_UNFILLED_BIT       0x04
#define VIA_FALLBACK_BIT       0x08
#define VIA_MAX_TRIFUNC        0x10


static struct {
    tnl_points_func          points;
    tnl_line_func            line;
    tnl_triangle_func        triangle;
    tnl_quad_func            quad;
} rast_tab[VIA_MAX_TRIFUNC];


#define DO_FALLBACK (IND & VIA_FALLBACK_BIT)
#define DO_OFFSET   (IND & VIA_OFFSET_BIT)
#define DO_UNFILLED (IND & VIA_UNFILLED_BIT)
#define DO_TWOSIDE  (IND & VIA_TWOSIDE_BIT)
#define DO_FLAT      0
#define DO_TRI       1
#define DO_QUAD      1
#define DO_LINE      1
#define DO_POINTS    1
#define DO_FULL_QUAD 1

#define HAVE_RGBA         1
#define HAVE_SPEC         1
#define HAVE_BACK_COLORS  0
#define HAVE_HW_FLATSHADE 1
#define VERTEX            viaVertex
#define TAB               rast_tab

/* Only used to pull back colors into vertices (ie, we know color is
 * floating point).
 */
#define VIA_COLOR(dst, src)                     \
    do {                                        \
        dst[0] = src[2];                        \
        dst[1] = src[1];                        \
        dst[2] = src[0];                        \
        dst[3] = src[3];                        \
    } while (0)

#define VIA_SPEC(dst, src)                      \
    do {                                        \
        dst[0] = src[2];                        \
        dst[1] = src[1];                        \
        dst[2] = src[0];                        \
    } while (0)


#define DEPTH_SCALE (1.0 / 0xffff)
#define UNFILLED_TRI unfilled_tri
#define UNFILLED_QUAD unfilled_quad
#define VERT_X(_v) _v->v.x
#define VERT_Y(_v) _v->v.y
#define VERT_Z(_v) _v->v.z
#define AREA_IS_CCW(a) (a > 0)
#define GET_VERTEX(e) (vmesa->verts + (e<<vmesa->vertexStrideShift))

#define VERT_SET_RGBA(v, c)    VIA_COLOR(v->ub4[coloroffset], c)
#define VERT_COPY_RGBA(v0, v1) v0->ui[coloroffset] = v1->ui[coloroffset]
#define VERT_SAVE_RGBA(idx)    color[idx] = v[idx]->ui[coloroffset]
#define VERT_RESTORE_RGBA(idx) v[idx]->ui[coloroffset] = color[idx]
#define VERT_SET_SPEC(v, c)    if (havespec) VIA_SPEC(v->ub4[5], c)
#define VERT_COPY_SPEC(v0, v1) if (havespec) COPY_3V(v0->ub4[5], v1->ub4[5])
#define VERT_SAVE_SPEC(idx)    if (havespec) spec[idx] = v[idx]->ui[5]
#define VERT_RESTORE_SPEC(idx) if (havespec) v[idx]->ui[5] = spec[idx]

#define SET_PRIMITIVE_RENDERED vmesa->primitiveRendered = GL_TRUE;

#define LOCAL_VARS(n)                                                   \
    viaContextPtr vmesa = VIA_CONTEXT(ctx);                             \
    GLuint color[n], spec[n];                                           \
    GLuint coloroffset = (vmesa->vertexSize == 4 ? 3 : 4);              \
    GLboolean havespec = (vmesa->vertexSize > 4);                       \
    (void)color; (void)spec; (void)coloroffset; (void)havespec;


/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/
/*
static const GLuint hwPrim[GL_POLYGON + 1] = {
    PR_LINES,
    PR_LINES,
    PR_LINES,
    PR_LINES,
    PR_TRIANGLES,
    PR_TRIANGLES,
    PR_TRIANGLES,
    PR_TRIANGLES,
    PR_TRIANGLES,
    PR_TRIANGLES
};
*/

#define RASTERIZE(x)           	                \
    if (vmesa->hwPrimitive != x) {		\
        viaRasterPrimitiveFinish(ctx);		\
        viaRasterPrimitive(ctx, x, x);		\
    }          	
	
#define RENDER_PRIMITIVE vmesa->renderPrimitive
#define TAG(x) x
#define IND VIA_FALLBACK_BIT
#include "tnl_dd/t_dd_unfilled.h"
#undef IND
#undef RASTERIZE

/***********************************************************************
 *                      Generate GL render functions                   *
 ***********************************************************************/
#define RASTERIZE(x)

#define IND (0)
#define TAG(x) x
#include "via_dd_tritmp.h"

#define IND (VIA_OFFSET_BIT)
#define TAG(x) x##_offset
#include "via_dd_tritmp.h"

#define IND (VIA_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "via_dd_tritmp.h"

#define IND (VIA_TWOSIDE_BIT|VIA_OFFSET_BIT)
#define TAG(x) x##_twoside_offset
#include "via_dd_tritmp.h"

#define IND (VIA_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "via_dd_tritmp.h"

#define IND (VIA_OFFSET_BIT|VIA_UNFILLED_BIT)
#define TAG(x) x##_offset_unfilled
#include "via_dd_tritmp.h"

#define IND (VIA_TWOSIDE_BIT|VIA_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled
#include "via_dd_tritmp.h"

#define IND (VIA_TWOSIDE_BIT|VIA_OFFSET_BIT|VIA_UNFILLED_BIT)
#define TAG(x) x##_twoside_offset_unfilled
#include "via_dd_tritmp.h"

#define IND (VIA_FALLBACK_BIT)
#define TAG(x) x##_fallback
#include "via_dd_tritmp.h"

#define IND (VIA_OFFSET_BIT|VIA_FALLBACK_BIT)
#define TAG(x) x##_offset_fallback
#include "via_dd_tritmp.h"

#define IND (VIA_TWOSIDE_BIT|VIA_FALLBACK_BIT)
#define TAG(x) x##_twoside_fallback
#include "via_dd_tritmp.h"

#define IND (VIA_TWOSIDE_BIT|VIA_OFFSET_BIT|VIA_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_fallback
#include "via_dd_tritmp.h"

#define IND (VIA_UNFILLED_BIT|VIA_FALLBACK_BIT)
#define TAG(x) x##_unfilled_fallback
#include "via_dd_tritmp.h"

#define IND (VIA_OFFSET_BIT|VIA_UNFILLED_BIT|VIA_FALLBACK_BIT)
#define TAG(x) x##_offset_unfilled_fallback
#include "via_dd_tritmp.h"

#define IND (VIA_TWOSIDE_BIT|VIA_UNFILLED_BIT|VIA_FALLBACK_BIT)
#define TAG(x) x##_twoside_unfilled_fallback
#include "via_dd_tritmp.h"

#define IND (VIA_TWOSIDE_BIT|VIA_OFFSET_BIT|VIA_UNFILLED_BIT| \
             VIA_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_unfilled_fallback
#include "via_dd_tritmp.h"


static void init_rast_tab(void)
{
    init();
    init_offset();
    init_twoside();
    init_twoside_offset();
    init_unfilled();
    init_offset_unfilled();
    init_twoside_unfilled();
    init_twoside_offset_unfilled();
    init_fallback();
    init_offset_fallback();
    init_twoside_fallback();
    init_twoside_offset_fallback();
    init_unfilled_fallback();
    init_offset_unfilled_fallback();
    init_twoside_unfilled_fallback();
    init_twoside_offset_unfilled_fallback();
}


/***********************************************************************
 *                    Rasterization fallback helpers                   *
 ***********************************************************************/


/* This code is hit only when a mix of accelerated and unaccelerated
 * primitives are being drawn, and only for the unaccelerated
 * primitives.
 */
static void
via_fallback_tri(viaContextPtr vmesa,
                 viaVertex *v0,
                 viaVertex *v1,
                 viaVertex *v2)
{    
    GLcontext *ctx = vmesa->glCtx;
    SWvertex v[3];
#ifdef PERFORMANCE_MEASURE
    if (VIA_PERFORMANCE) P_M;
#endif
    via_translate_vertex(ctx, v0, &v[0]);
    via_translate_vertex(ctx, v1, &v[1]);
    via_translate_vertex(ctx, v2, &v[2]);
    _swrast_Triangle(ctx, &v[0], &v[1], &v[2]);
}


static void
via_fallback_line(viaContextPtr vmesa,
                  viaVertex *v0,
                  viaVertex *v1)
{
    GLcontext *ctx = vmesa->glCtx;
    SWvertex v[2];
#ifdef PERFORMANCE_MEASURE
    if (VIA_PERFORMANCE) P_M;
#endif
    via_translate_vertex(ctx, v0, &v[0]);
    via_translate_vertex(ctx, v1, &v[1]);
    _swrast_Line(ctx, &v[0], &v[1]);
}


static void
via_fallback_point(viaContextPtr vmesa,
                   viaVertex *v0)
{
    GLcontext *ctx = vmesa->glCtx;
    SWvertex v[1];
#ifdef PERFORMANCE_MEASURE
    if (VIA_PERFORMANCE) P_M;
#endif
    via_translate_vertex(ctx, v0, &v[0]);
    _swrast_Point(ctx, &v[0]);
}

/**********************************************************************/
/*               Render unclipped begin/end objects                   */
/*		 (No Twoside / Offset / Unfilled)		      */
/**********************************************************************/
#define IND 0
#define V(x) (viaVertex *)(vertptr + ((x) << vertshift))
#define RENDER_POINTS(start, count)   \
    for (; start < count; start++) POINT(V(ELT(start)));
#define RENDER_LINE(v0, v1)         LINE(V(v0), V(v1))

#define RENDER_TRI( v0, v1, v2)                            	\
    if (VIA_DEBUG) fprintf(stderr, "RENDER_TRI - simple\n");	\
    TRI( V(v0), V(v1), V(v2))
    
#define RENDER_QUAD(v0, v1, v2, v3) QUAD(V(v0), V(v1), V(v2), V(v3))

#define INIT(x) viaRasterPrimitive(ctx, x, x)

#undef LOCAL_VARS
#define LOCAL_VARS                                              \
    viaContextPtr vmesa = VIA_CONTEXT(ctx);                     \
    GLubyte *vertptr = (GLubyte *)vmesa->verts;                 \
    const GLuint vertshift = vmesa->vertexStrideShift;          \
    const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;       \
    (void)elt;
#define POSTFIX							\
    viaRasterPrimitiveFinish(ctx)    
#define RESET_STIPPLE
#define RESET_OCCLUSION
#define PRESERVE_VB_DEFS
#define ELT(x) x
#define TAG(x) via_fast##x##_verts
#include "via_vb_rendertmp.h"
#undef ELT
#undef TAG
#define TAG(x) via_fast##x##_elts
#define ELT(x) elt[x]
#include "via_vb_rendertmp.h"
#undef ELT
#undef TAG
#undef NEED_EDGEFLAG_SETUP
#undef EDGEFLAG_GET
#undef EDGEFLAG_SET
#undef RESET_OCCLUSION

/**********************************************************************/
/*               Render unclipped begin/end objects                   */
/*		 (Can handle Twoside / Offset / Unfilled	      */
/**********************************************************************/
#define NEED_EDGEFLAG_SETUP (ctx->_TriangleCaps & DD_TRI_UNFILLED)
#define EDGEFLAG_GET(idx) VB->EdgeFlag[idx]
#define EDGEFLAG_SET(idx, val) VB->EdgeFlag[idx] = val

#define RENDER_POINTS(start, count)  		\
   tnl->Driver.Render.Points(ctx, start, count)

#define RENDER_LINE(v1, v2)	 		\
    LineFunc(ctx, v1, v2)				

#define RENDER_TRI(v1, v2, v3)			            				\
    if (VIA_DEBUG) fprintf(stderr, "RENDER_TRI - complex\n");				\
    if (VIA_DEBUG) fprintf(stderr, "TriangleFunc = %x\n", (unsigned int)TriangleFunc); 	\
    TriangleFunc(ctx, v1, v2, v3)

#define RENDER_QUAD(v1, v2, v3, v4)		\
    QuadFunc(ctx, v1, v2, v3, v4)		

#define LOCAL_VARS							\
    TNLcontext *tnl = TNL_CONTEXT(ctx);					\
    struct vertex_buffer *VB = &tnl->vb;				\
    const GLuint * const elt = VB->Elts;				\
    const tnl_line_func LineFunc = tnl->Driver.Render.Line;			\
    const tnl_triangle_func TriangleFunc = tnl->Driver.Render.Triangle;	\
    const tnl_quad_func QuadFunc = tnl->Driver.Render.Quad;			\
    const GLboolean stipple = ctx->Line.StippleFlag;			\
    (void) (LineFunc && TriangleFunc && QuadFunc);			\
    (void) elt; (void) stipple;

#define POSTFIX                                                         \
    viaRasterPrimitiveFinish(ctx)	                    
#define ELT(x) x
#define TAG(x) via_##x##_verts
/*#define INIT(x) tnl->Driver.Render.PrimitiveNotify(ctx, x)*/
#define INIT(x) viaRasterPrimitive(ctx, x, x)
#define RESET_STIPPLE if (stipple) tnl->Driver.Render.ResetLineStipple(ctx)
#define RESET_OCCLUSION ctx->OcclusionResult = GL_TRUE
#define PRESERVE_VB_DEFS
#include "via_vb_rendertmp.h"
#undef ELT
#undef TAG
#define ELT(x) elt[x]
#define TAG(x) via_##x##_elts
#include "via_vb_rendertmp.h"

/**********************************************************************/
/*                   Render clipped primitives                        */
/**********************************************************************/



static void viaRenderClippedPoly(GLcontext *ctx, const GLuint *elts,
                                 GLuint n)
{
    TNLcontext *tnl = TNL_CONTEXT(ctx);
    struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
#ifdef PERFORMANCE_MEASURE
    if (VIA_PERFORMANCE) P_M;
#endif
    /* Render the new vertices as an unclipped polygon.
     */
    {
        GLuint *tmp = VB->Elts;
        VB->Elts = (GLuint *)elts;
        tnl->Driver.Render.PrimTabElts[GL_POLYGON](ctx, 0, n,
                                                   PRIM_BEGIN|PRIM_END);
        VB->Elts = tmp;
    }
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);	
}

static void viaRenderClippedLine(GLcontext *ctx, GLuint ii, GLuint jj)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    TNLcontext *tnl = TNL_CONTEXT(ctx);
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
#ifdef PERFORMANCE_MEASURE
    if (VIA_PERFORMANCE) P_M;
#endif    
    vmesa->primitiveRendered = GL_TRUE;
    
    tnl->Driver.Render.Line(ctx, ii, jj);
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}

static void viaFastRenderClippedPoly(GLcontext *ctx, const GLuint *elts,
                                     GLuint n)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    GLuint vertsize = vmesa->vertexSize;
    GLuint *vb = viaCheckDma(vmesa, (n - 2) * 3 * 4 * vertsize);
    GLubyte *vertptr = (GLubyte *)vmesa->verts;
    const GLuint vertshift = vmesa->vertexStrideShift;
    const GLuint *start = (const GLuint *)V(elts[0]);
    GLuint *temp1;
    GLuint *temp2;
    int i,j;
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
#ifdef PERFORMANCE_MEASURE
    if (VIA_PERFORMANCE) P_M;
#endif
    vmesa->primitiveRendered = GL_TRUE;

    for (i = 2; i < n; i++) {
	/*=* [DBG] exy : fix flat-shading + clipping error *=*/
        /*COPY_DWORDS(j, vb, vertsize, start);
        COPY_DWORDS(j, vb, vertsize, V(elts[i - 1]));
	temp1 = (GLuint *)V(elts[i - 1]);	
        COPY_DWORDS(j, vb, vertsize, V(elts[i]));
	temp2 = (GLuint *)V(elts[i]);*/
	COPY_DWORDS(j, vb, vertsize, V(elts[i - 1]));
        COPY_DWORDS(j, vb, vertsize, V(elts[i]));
	temp1 = (GLuint *)V(elts[i - 1]);
	COPY_DWORDS(j, vb, vertsize, start);	
	temp2 = (GLuint *)V(elts[i]);
	if (VIA_DEBUG) fprintf(stderr, "start = %d -  x = %f, y = %f, z = %f, w = %f, u = %f, v = %f\n", elts[0], *(GLfloat *)&start[0], *(GLfloat *)&start[1], *(GLfloat *)&start[2], *(GLfloat *)&start[3], *(GLfloat *)&start[6], *(GLfloat *)&start[7]);
	if (VIA_DEBUG) fprintf(stderr, "%d -  x = %f, y = %f, z = %f, w = %f, u = %f, v = %f\n", elts[i - 1], *(GLfloat *)&temp1[0], *(GLfloat *)&temp1[1], *(GLfloat *)&temp1[2], *(GLfloat *)&temp1[3], *(GLfloat *)&temp1[6], *(GLfloat *)&temp1[7]);
	if (VIA_DEBUG) fprintf(stderr, "%d -  x = %f, y = %f, z = %f, w = %f, u = %f, v = %f\n", elts[i], *(GLfloat *)&temp2[0], *(GLfloat *)&temp2[1], *(GLfloat *)&temp2[2], *(GLfloat *)&temp2[3], *(GLfloat *)&temp2[6], *(GLfloat *)&temp2[7]);	
    }
    vmesa->dmaLow += (n - 2) * 3 * 4 * vertsize;
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}

/**********************************************************************/
/*                    Choose render functions                         */
/**********************************************************************/



#define _VIA_NEW_RENDERSTATE (_DD_NEW_LINE_STIPPLE |            \
                              _DD_NEW_TRI_UNFILLED |            \
                              _DD_NEW_TRI_LIGHT_TWOSIDE |       \
                              _DD_NEW_TRI_OFFSET |              \
                              _DD_NEW_TRI_STIPPLE |             \
                              _NEW_POLYGONSTIPPLE)

#define POINT_FALLBACK (0)
/*#define LINE_FALLBACK (DD_LINE_STIPPLE)
*/
#define LINE_FALLBACK (0)
#define TRI_FALLBACK (0)
#define ANY_FALLBACK_FLAGS (POINT_FALLBACK|LINE_FALLBACK|TRI_FALLBACK)
#define ANY_RASTER_FLAGS (DD_TRI_LIGHT_TWOSIDE|DD_TRI_OFFSET|DD_TRI_UNFILLED)

static void viaChooseRenderState(GLcontext *ctx)
{
    TNLcontext *tnl = TNL_CONTEXT(ctx);
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    GLuint flags = ctx->_TriangleCaps;
    GLuint index = 0;
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
    
    if (VIA_DEBUG) fprintf(stderr, "_TriangleCaps = %x\n", flags);    
    if (flags & (ANY_FALLBACK_FLAGS|ANY_RASTER_FLAGS)) {
        if (flags & ANY_RASTER_FLAGS) {
            if (flags & DD_TRI_LIGHT_TWOSIDE)    index |= VIA_TWOSIDE_BIT;
            if (flags & DD_TRI_OFFSET)           index |= VIA_OFFSET_BIT;
            if (flags & DD_TRI_UNFILLED)         index |= VIA_UNFILLED_BIT;
        }

        vmesa->drawPoint = via_draw_point;
        vmesa->drawLine = via_draw_line;
        vmesa->drawTri = via_draw_triangle;

        /* Hook in fallbacks for specific primitives.
         */
        if (flags & ANY_FALLBACK_FLAGS) {
            if (flags & POINT_FALLBACK)
                vmesa->drawPoint = via_fallback_point;

            if (flags & LINE_FALLBACK)
                vmesa->drawLine = via_fallback_line;

            if (flags & TRI_FALLBACK)
                vmesa->drawTri = via_fallback_tri;

            index |= VIA_FALLBACK_BIT;
        }
    }
    if (VIA_DEBUG) {
	fprintf(stderr, "index = %x\n", index);    
	fprintf(stderr, "renderIndex = %x\n", vmesa->renderIndex);
    }	
    if (vmesa->renderIndex != index) {
        vmesa->renderIndex = index;

        tnl->Driver.Render.Points = rast_tab[index].points;
        tnl->Driver.Render.Line = rast_tab[index].line;
        tnl->Driver.Render.Triangle = rast_tab[index].triangle;
	if (VIA_DEBUG) fprintf(stderr, "tnl->Driver.Render.xxx = rast_tab[index].xxx = %x\n", (unsigned int)tnl->Driver.Render.Triangle);
        tnl->Driver.Render.Quad = rast_tab[index].quad;

        if (index == 0) {
            tnl->Driver.Render.PrimTabVerts = via_fastrender_tab_verts;
            tnl->Driver.Render.PrimTabElts = via_fastrender_tab_elts;
            tnl->Driver.Render.ClippedLine = line; /* from tritmp.h */
            tnl->Driver.Render.ClippedPolygon = viaFastRenderClippedPoly;
        }
        else {
            tnl->Driver.Render.PrimTabVerts = via_render_tab_verts;
            tnl->Driver.Render.PrimTabElts = via_render_tab_elts;
            tnl->Driver.Render.ClippedLine = viaRenderClippedLine;
            tnl->Driver.Render.ClippedPolygon = viaRenderClippedPoly;
        }
    }
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}

static const GLenum reducedPrim[GL_POLYGON + 1] = {
    GL_POINTS,
    GL_LINES,
    GL_LINES,
    GL_LINES,
    GL_TRIANGLES,
    GL_TRIANGLES,
    GL_TRIANGLES,
    GL_TRIANGLES,
    GL_TRIANGLES,
    GL_TRIANGLES
};


static void emit_all_state(viaContextPtr vmesa)
{
    GLcontext *ctx = vmesa->glCtx;
    GLuint *vb = viaCheckDma(vmesa, 0x110);
    GLuint i = 0;
    GLuint j = 0;
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
#ifdef PERFORMANCE_MEASURE
    if (VIA_PERFORMANCE) P_M;
#endif    
    
    *vb++ = HC_HEADER2;
    *vb++ = (HC_ParaType_NotTex << 16);
    *vb++ = ((HC_SubA_HEnable << 24) | vmesa->regEnable);
    *vb++ = ((HC_SubA_HFBBMSKL << 24) | vmesa->regHFBBMSKL);    
    *vb++ = ((HC_SubA_HROP << 24) | vmesa->regHROP);        
    i += 5;
    
    if (vmesa->have_hw_stencil) {
	GLuint pitch, format, offset;
	
	format = HC_HZWBFM_24;	    	
	offset = vmesa->depth.offset;
	pitch = vmesa->depth.pitch;
	
        *vb++ = ((HC_SubA_HZWBBasL << 24) | (offset & 0xFFFFFF));
        *vb++ = ((HC_SubA_HZWBBasH << 24) | ((offset & 0xFF000000) >> 24));	
        *vb++ = ((HC_SubA_HZWBType << 24) | HC_HDBLoc_Local | HC_HZONEasFF_MASK | 
	         format | pitch);            
        *vb++ = ((HC_SubA_HZWTMD << 24) | vmesa->regHZWTMD);
	/* set stencil */
	*vb++ = ((HC_SubA_HSTREF << 24) | vmesa->regHSTREF);
	*vb++ = ((HC_SubA_HSTMD << 24) | vmesa->regHSTMD);
	
	i += 6;	
    }
    else if (vmesa->hasDepth) {
	GLuint pitch, format, offset;
	
	if (vmesa->depthBits == 16) {
	    /* We haven't support 16bit depth yet */
	    format = HC_HZWBFM_16;
	    /*format = HC_HZWBFM_32;*/
	    if (VIA_DEBUG) fprintf(stderr, "z format = 16\n");
	}	    
	else {
	    format = HC_HZWBFM_32;
	    if (VIA_DEBUG) fprintf(stderr, "z format = 32\n");
	}
	    
	    
	offset = vmesa->depth.offset;
	pitch = vmesa->depth.pitch;
	
        *vb++ = ((HC_SubA_HZWBBasL << 24) | (offset & 0xFFFFFF));
        *vb++ = ((HC_SubA_HZWBBasH << 24) | ((offset & 0xFF000000) >> 24));	
        *vb++ = ((HC_SubA_HZWBType << 24) | HC_HDBLoc_Local | HC_HZONEasFF_MASK | 
	         format | pitch);            
        *vb++ = ((HC_SubA_HZWTMD << 24) | vmesa->regHZWTMD);
	i += 4;	
    }
    
    if (ctx->Color.AlphaEnabled) {
        *vb++ = ((HC_SubA_HATMD << 24) | vmesa->regHATMD);
        i++;
    }   

    if (ctx->Color.BlendEnabled) {
        *vb++ = ((HC_SubA_HABLCsat << 24) | vmesa->regHABLCsat);
        *vb++ = ((HC_SubA_HABLCop  << 24) | vmesa->regHABLCop); 
        *vb++ = ((HC_SubA_HABLAsat << 24) | vmesa->regHABLAsat);        
        *vb++ = ((HC_SubA_HABLAop  << 24) | vmesa->regHABLAop); 
        *vb++ = ((HC_SubA_HABLRCa  << 24) | vmesa->regHABLRCa); 
        *vb++ = ((HC_SubA_HABLRFCa << 24) | vmesa->regHABLRFCa);        
        *vb++ = ((HC_SubA_HABLRCbias << 24) | vmesa->regHABLRCbias);    
        *vb++ = ((HC_SubA_HABLRCb  << 24) | vmesa->regHABLRCb); 
        *vb++ = ((HC_SubA_HABLRFCb << 24) | vmesa->regHABLRFCb);        
        *vb++ = ((HC_SubA_HABLRAa  << 24) | vmesa->regHABLRAa); 
        *vb++ = ((HC_SubA_HABLRAb  << 24) | vmesa->regHABLRAb); 
        i += 11;
    }
    
    if (ctx->Fog.Enabled) {
        *vb++ = ((HC_SubA_HFogLF << 24) | vmesa->regHFogLF);        
        *vb++ = ((HC_SubA_HFogCL << 24) | vmesa->regHFogCL);            
        *vb++ = ((HC_SubA_HFogCH << 24) | vmesa->regHFogCH);            
        i += 3;
    }
    
    if (ctx->Line.StippleFlag) {
        *vb++ = ((HC_SubA_HLP << 24) | ctx->Line.StipplePattern);           
        *vb++ = ((HC_SubA_HLPRF << 24) | ctx->Line.StippleFactor);                  
    }
    else {
        *vb++ = ((HC_SubA_HLP << 24) | 0xFFFF);         
        *vb++ = ((HC_SubA_HLPRF << 24) | 0x1);              
    }

    i += 2;
    
    *vb++ = ((HC_SubA_HPixGC << 24) | 0x0);             
    i++;
    
    if (i & 0x1) {
        *vb++ = HC_DUMMY;
        i++;    
    }    
    
    if (ctx->Texture._EnabledUnits) {
    
	struct gl_texture_unit *texUnit0 = &ctx->Texture.Unit[0];
	struct gl_texture_unit *texUnit1 = &ctx->Texture.Unit[1];

        {
	    viaTextureObjectPtr t = (viaTextureObjectPtr)texUnit0->_Current->DriverData;
	    GLuint nDummyValue = 0;
            
	    *vb++ = HC_HEADER2;
            *vb++ = (HC_ParaType_Tex << 16) | (HC_SubType_TexGeneral << 24);

	    if (ctx->Texture._EnabledUnits > 1) {
		if (VIA_DEBUG) fprintf(stderr, "multi texture\n");
		nDummyValue = (HC_SubA_HTXSMD << 24) | (1 << 3);
                
		if (t && t->needClearCache) {
                    *vb++ = nDummyValue | HC_HTXCHCLR_MASK;
                    *vb++ = nDummyValue;
                }
                else {
                    *vb++ = nDummyValue;
                    *vb++ = nDummyValue;
                }
            }
            else {
		if (VIA_DEBUG) fprintf(stderr, "single texture\n");
		nDummyValue = (HC_SubA_HTXSMD << 24) | 0;
                
		if (t && t->needClearCache) {
                    *vb++ = nDummyValue | HC_HTXCHCLR_MASK;
                    *vb++ = nDummyValue;
                }
                else {
                    *vb++ = nDummyValue;
                    *vb++ = nDummyValue;
                }
            }
            *vb++ = HC_HEADER2;
            *vb++ = HC_ParaType_NotTex << 16;
            *vb++ = (HC_SubA_HEnable << 24) | vmesa->regEnable;
            *vb++ = (HC_SubA_HEnable << 24) | vmesa->regEnable;
            i += 8;
        }

        if (texUnit0->Enabled) {
	    struct gl_texture_object *texObj = texUnit0->_Current;
	    viaTextureObjectPtr t = (viaTextureObjectPtr)texObj->DriverData;
	    GLuint numLevels = t->lastLevel - t->firstLevel + 1;
	    GLuint nDummyValue = 0;
	    if (VIA_DEBUG) {
		fprintf(stderr, "texture0 enabled\n");
		fprintf(stderr, "texture level %d\n", t->actualLevel);
	    }		
            if (numLevels == 8) {
                nDummyValue = t->regTexFM;
                *vb++ = HC_HEADER2;
                *vb++ = (HC_ParaType_Tex << 16) |  (0 << 24);
                *vb++ = t->regTexFM;
                *vb++ = (HC_SubA_HTXnL0OS << 24) |
		    ((t->lastLevel) << HC_HTXnLVmax_SHIFT) | t->firstLevel;
                *vb++ = t->regTexWidthLog2[0];
                *vb++ = t->regTexWidthLog2[1];
                *vb++ = t->regTexHeightLog2[0];
                *vb++ = t->regTexHeightLog2[1];
                *vb++ = t->regTexBaseH[0];
                *vb++ = t->regTexBaseH[1];
                *vb++ = t->regTexBaseH[2];

                *vb++ = t->regTexBaseAndPitch[0].baseL;
                *vb++ = t->regTexBaseAndPitch[0].pitchLog2;
                *vb++ = t->regTexBaseAndPitch[1].baseL;
                *vb++ = t->regTexBaseAndPitch[1].pitchLog2;
                *vb++ = t->regTexBaseAndPitch[2].baseL;
                *vb++ = t->regTexBaseAndPitch[2].pitchLog2;
                *vb++ = t->regTexBaseAndPitch[3].baseL;
                *vb++ = t->regTexBaseAndPitch[3].pitchLog2;
                *vb++ = t->regTexBaseAndPitch[4].baseL;
                *vb++ = t->regTexBaseAndPitch[4].pitchLog2;
                *vb++ = t->regTexBaseAndPitch[5].baseL;
                *vb++ = t->regTexBaseAndPitch[5].pitchLog2;
                *vb++ = t->regTexBaseAndPitch[6].baseL;
                *vb++ = t->regTexBaseAndPitch[6].pitchLog2;
                *vb++ = t->regTexBaseAndPitch[7].baseL;
                *vb++ = t->regTexBaseAndPitch[7].pitchLog2;
                i += 27;
            }
            else if (numLevels > 1) {
                nDummyValue = t->regTexFM;
                *vb++ = HC_HEADER2;
                *vb++ = (HC_ParaType_Tex << 16) |  (0 << 24);
                *vb++ = t->regTexFM;
                *vb++ = (HC_SubA_HTXnL0OS << 24) |
		    ((t->lastLevel) << HC_HTXnLVmax_SHIFT) | t->firstLevel;
		*vb++ = t->regTexWidthLog2[0];
		*vb++ = t->regTexHeightLog2[0];
		
		if (numLevels > 6) {
		    *vb++ = t->regTexWidthLog2[1];
		    *vb++ = t->regTexHeightLog2[1];
		    i += 2;
		}
                
		*vb++ = t->regTexBaseH[0];
		
                if (numLevels > 3) { 
		    *vb++ = t->regTexBaseH[1];
		    i++;
		}
		if (numLevels > 6) {
		    *vb++ = t->regTexBaseH[2];
		    i++;
		}
		if (numLevels > 9)  {
		    *vb++ = t->regTexBaseH[3];
		    i++;
		}
		
		i += 7;

                for (j = 0; j < numLevels; j++) {
                    *vb++ = t->regTexBaseAndPitch[j].baseL;
                    *vb++ = t->regTexBaseAndPitch[j].pitchLog2;
		    i += 2;
                }
            }
            else {
                nDummyValue = t->regTexFM;
                *vb++ = HC_HEADER2;
                *vb++ = (HC_ParaType_Tex << 16) |  (0 << 24);
                *vb++ = t->regTexFM;
                *vb++ = (HC_SubA_HTXnL0OS << 24) |
		    ((t->lastLevel) << HC_HTXnLVmax_SHIFT) | t->firstLevel;
                *vb++ = t->regTexWidthLog2[0];
                *vb++ = t->regTexHeightLog2[0];
                *vb++ = t->regTexBaseH[0];
                *vb++ = t->regTexBaseAndPitch[0].baseL;
                *vb++ = t->regTexBaseAndPitch[0].pitchLog2;
                i += 9;
            }

            *vb++ = (HC_SubA_HTXnTB << 24) | vmesa->regHTXnTB_0;
            *vb++ = (HC_SubA_HTXnMPMD << 24) | vmesa->regHTXnMPMD_0;
            *vb++ = (HC_SubA_HTXnTBLCsat << 24) | vmesa->regHTXnTBLCsat_0;
            *vb++ = (HC_SubA_HTXnTBLCop << 24) | vmesa->regHTXnTBLCop_0;
            *vb++ = (HC_SubA_HTXnTBLMPfog << 24) | vmesa->regHTXnTBLMPfog_0;
            *vb++ = (HC_SubA_HTXnTBLAsat << 24) | vmesa->regHTXnTBLAsat_0;
            *vb++ = (HC_SubA_HTXnTBLRCb << 24) | vmesa->regHTXnTBLRCb_0;
            *vb++ = (HC_SubA_HTXnTBLRAa << 24) | vmesa->regHTXnTBLRAa_0;
            *vb++ = (HC_SubA_HTXnTBLRFog << 24) | vmesa->regHTXnTBLRFog_0;
            i += 9;
	    /*=* John Sheng [2003.7.18] texture combine */
	    *vb++ = (HC_SubA_HTXnTBLRCa << 24) | vmesa->regHTXnTBLRCa_0;
            *vb++ = (HC_SubA_HTXnTBLRCc << 24) | vmesa->regHTXnTBLRCc_0;
            *vb++ = (HC_SubA_HTXnTBLRCbias << 24) | vmesa->regHTXnTBLRCbias_0;
	    i += 3;

            if (t->regTexFM == HC_HTXnFM_Index8) {
                struct gl_color_table *table = &texObj->Palette;
		GLfloat *tableF = (GLfloat *)table->Table;

                *vb++ = HC_HEADER2;
                *vb++ = (HC_ParaType_Palette << 16) | (0 << 24);
                i += 2;
                for (j = 0; j < table->Size; j++) {
                    *vb++ = tableF[j];
		    i++;
                }
            }
            if (i & 0x1) {
    		*vb++ = HC_DUMMY;
    		i++;    
	    }
        }
	
	if (texUnit1->Enabled) {
	    struct gl_texture_object *texObj = texUnit1->_Current;
	    viaTextureObjectPtr t = (viaTextureObjectPtr)texObj->DriverData;
	    GLuint numLevels = t->lastLevel - t->firstLevel + 1;
	    GLuint nDummyValue = 0;
	    if (VIA_DEBUG) {
		fprintf(stderr, "texture1 enabled\n");
		fprintf(stderr, "texture level %d\n", t->actualLevel);
	    }		
            if (numLevels == 8) {
                nDummyValue = t->regTexFM;
                *vb++ = HC_HEADER2;
                *vb++ = (HC_ParaType_Tex << 16) |  (1 << 24);
                *vb++ = t->regTexFM;
                *vb++ = (HC_SubA_HTXnL0OS << 24) |
		    ((t->lastLevel) << HC_HTXnLVmax_SHIFT) | t->firstLevel;
                *vb++ = t->regTexWidthLog2[0];
                *vb++ = t->regTexWidthLog2[1];
                *vb++ = t->regTexHeightLog2[0];
                *vb++ = t->regTexHeightLog2[1];
                *vb++ = t->regTexBaseH[0];
                *vb++ = t->regTexBaseH[1];
                *vb++ = t->regTexBaseH[2];

                *vb++ = t->regTexBaseAndPitch[0].baseL;
                *vb++ = t->regTexBaseAndPitch[0].pitchLog2;
                *vb++ = t->regTexBaseAndPitch[1].baseL;
                *vb++ = t->regTexBaseAndPitch[1].pitchLog2;
                *vb++ = t->regTexBaseAndPitch[2].baseL;
                *vb++ = t->regTexBaseAndPitch[2].pitchLog2;
                *vb++ = t->regTexBaseAndPitch[3].baseL;
                *vb++ = t->regTexBaseAndPitch[3].pitchLog2;
                *vb++ = t->regTexBaseAndPitch[4].baseL;
                *vb++ = t->regTexBaseAndPitch[4].pitchLog2;
                *vb++ = t->regTexBaseAndPitch[5].baseL;
                *vb++ = t->regTexBaseAndPitch[5].pitchLog2;
                *vb++ = t->regTexBaseAndPitch[6].baseL;
                *vb++ = t->regTexBaseAndPitch[6].pitchLog2;
                *vb++ = t->regTexBaseAndPitch[7].baseL;
                *vb++ = t->regTexBaseAndPitch[7].pitchLog2;
                i += 27;
            }
            else if (numLevels > 1) {
                nDummyValue = t->regTexFM;
                *vb++ = HC_HEADER2;
                *vb++ = (HC_ParaType_Tex << 16) |  (1 << 24);
                *vb++ = t->regTexFM;
                *vb++ = (HC_SubA_HTXnL0OS << 24) |
		    ((t->lastLevel) << HC_HTXnLVmax_SHIFT) | t->firstLevel;
		*vb++ = t->regTexWidthLog2[0];
		*vb++ = t->regTexHeightLog2[0];
		
		if (numLevels > 6) {
		    *vb++ = t->regTexWidthLog2[1];
		    *vb++ = t->regTexHeightLog2[1];
		    i += 2;
		}
                
		*vb++ = t->regTexBaseH[0];
		
                if (numLevels > 3) { 
		    *vb++ = t->regTexBaseH[1];
		    i++;
		}
		if (numLevels > 6) {
		    *vb++ = t->regTexBaseH[2];
		    i++;
		}
		if (numLevels > 9)  {
		    *vb++ = t->regTexBaseH[3];
		    i++;
		}
		
		i += 7;

                for (j = 0; j < numLevels; j++) {
                    *vb++ = t->regTexBaseAndPitch[j].baseL;
                    *vb++ = t->regTexBaseAndPitch[j].pitchLog2;
		    i += 2;
                }
            }
            else {
                nDummyValue = t->regTexFM;
                *vb++ = HC_HEADER2;
                *vb++ = (HC_ParaType_Tex << 16) |  (1 << 24);
                *vb++ = t->regTexFM;
                *vb++ = (HC_SubA_HTXnL0OS << 24) |
		    ((t->lastLevel) << HC_HTXnLVmax_SHIFT) | t->firstLevel;
                *vb++ = t->regTexWidthLog2[0];
                *vb++ = t->regTexHeightLog2[0];
                *vb++ = t->regTexBaseH[0];
                *vb++ = t->regTexBaseAndPitch[0].baseL;
                *vb++ = t->regTexBaseAndPitch[0].pitchLog2;
                i += 9;
            }

            *vb++ = (HC_SubA_HTXnTB << 24) | vmesa->regHTXnTB_1;
            *vb++ = (HC_SubA_HTXnMPMD << 24) | vmesa->regHTXnMPMD_1;
            *vb++ = (HC_SubA_HTXnTBLCsat << 24) | vmesa->regHTXnTBLCsat_1;
            *vb++ = (HC_SubA_HTXnTBLCop << 24) | vmesa->regHTXnTBLCop_1;
            *vb++ = (HC_SubA_HTXnTBLMPfog << 24) | vmesa->regHTXnTBLMPfog_1;
            *vb++ = (HC_SubA_HTXnTBLAsat << 24) | vmesa->regHTXnTBLAsat_1;
            *vb++ = (HC_SubA_HTXnTBLRCb << 24) | vmesa->regHTXnTBLRCb_1;
            *vb++ = (HC_SubA_HTXnTBLRAa << 24) | vmesa->regHTXnTBLRAa_1;
            *vb++ = (HC_SubA_HTXnTBLRFog << 24) | vmesa->regHTXnTBLRFog_1;
            i += 9;

            if (t->regTexFM == HC_HTXnFM_Index8) {
                struct gl_color_table *table = &texObj->Palette;
		GLfloat *tableF = (GLfloat *)table->Table;

                *vb++ = HC_HEADER2;
                *vb++ = (HC_ParaType_Palette << 16) | (1 << 24);
                i += 2;
                for (j = 0; j < table->Size; j++) {
                    *vb++ = tableF[j];
		    i++;
                }
            }
            if (i & 0x1) {
    		*vb++ = HC_DUMMY;
    		i++;    
	    }
        }
    }
    
    
    if (ctx->Polygon.StippleFlag) {
        GLuint *stipple = &ctx->PolygonStipple[0];
        
        *vb++ = HC_HEADER2;             
        *vb++ = ((HC_ParaType_Palette << 16) | (HC_SubType_Stipple << 24));

        *vb++ = stipple[31];            
        *vb++ = stipple[30];            
        *vb++ = stipple[29];            
        *vb++ = stipple[28];            
        *vb++ = stipple[27];            
        *vb++ = stipple[26];            
        *vb++ = stipple[25];            
        *vb++ = stipple[24];            
        *vb++ = stipple[23];            
        *vb++ = stipple[22];            
        *vb++ = stipple[21];            
        *vb++ = stipple[20];            
        *vb++ = stipple[19];            
        *vb++ = stipple[18];            
        *vb++ = stipple[17];            
        *vb++ = stipple[16];            
        *vb++ = stipple[15];            
        *vb++ = stipple[14];            
        *vb++ = stipple[13];            
        *vb++ = stipple[12];            
        *vb++ = stipple[11];            
        *vb++ = stipple[10];            
        *vb++ = stipple[9];             
        *vb++ = stipple[8];             
        *vb++ = stipple[7];             
        *vb++ = stipple[6];             
        *vb++ = stipple[5];             
        *vb++ = stipple[4];             
        *vb++ = stipple[3];             
        *vb++ = stipple[2];             
        *vb++ = stipple[1];             
        *vb++ = stipple[0];             
        
        *vb++ = HC_HEADER2;                     
        *vb++ = (HC_ParaType_NotTex << 16);
        *vb++ = ((HC_SubA_HSPXYOS << 24) | (0x20 - (vmesa->driDrawable->h & 0x1F)));
        *vb++ = ((HC_SubA_HSPXYOS << 24) | (0x20 - (vmesa->driDrawable->h & 0x1F)));
        i += 38;
    }
    
    vmesa->dmaLow += (i << 2);

    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);
}



/**********************************************************************/
/*                 High level hooks for t_vb_render.c                 */
/**********************************************************************/

/* Determine the rasterized primitive when not drawing unfilled
 * polygons.
 *
 * Used only for the default render stage which always decomposes
 * primitives to trianges/lines/points.  For the accelerated stage,
 * which renders strips as strips, the equivalent calculations are
 * performed in via_render.c.
 */
static void viaRenderPrimitive(GLcontext *ctx, GLenum prim)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    GLuint rprim = reducedPrim[prim];
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
    vmesa->renderPrimitive = prim;
    viaRasterPrimitive(ctx, rprim, rprim);
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}

static void viaRunPipeline(GLcontext *ctx)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
    
    if (VIA_DEBUG) fprintf(stderr, "newState = %x\n", vmesa->newState);        
    
    if (vmesa->newState) {
        if (vmesa->newState & _NEW_TEXTURE)
            viaUpdateTextureState(ctx); /* may modify vmesa->newState */

	viaChooseVertexState(ctx);
	viaChooseRenderState(ctx);
        vmesa->newState = 0;
    }

    emit_all_state(vmesa);

    _tnl_run_pipeline(ctx);
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);        
}

static void viaRenderStart(GLcontext *ctx)
{
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
    
    /* Check for projective texturing.  Make sure all texcoord
     * pointers point to something.  (fix in mesa?)
     */
    viaCheckTexSizes(ctx);
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);
}

static void viaRenderFinish(GLcontext *ctx)
{
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
    if (VIA_CONTEXT(ctx)->renderIndex & VIA_FALLBACK_BIT)
        _swrast_flush(ctx);
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);	
}


/* System to flush dma and emit state changes based on the rasterized
 * primitive.
 */
void viaRasterPrimitive(GLcontext *ctx,
                        GLenum rprim,
                        GLuint hwprim)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    GLuint *vb = viaCheckDma(vmesa, 32);
    GLuint regCmdB;
    if (VIA_DEBUG) {
	fprintf(stderr, "%s - in\n", __FUNCTION__);
	fprintf(stderr, "hwprim = %x\n", hwprim);
    }
    /*=* [DBG] exy : fix wireframe + clipping error *=*/
    if (((rprim == GL_TRIANGLES && (ctx->_TriangleCaps & DD_TRI_UNFILLED)))) {
	hwprim = GL_LINES;
    }
    
    if (RasterCounter > 0) {
    
	if (VIA_DEBUG) fprintf(stderr, "enter twice:%d\n",RasterCounter);
	RasterCounter++;
	return;
    }
    RasterCounter++;
    
    vmesa->primitiveRendered = GL_FALSE;
    regCmdB = vmesa->regCmdB;

    switch (hwprim) {
    case GL_POINTS:
        if (VIA_DEBUG) fprintf(stderr, "Points\n");
        vmesa->regCmdA_End = vmesa->regCmdA | HC_HPMType_Point | HC_HVCycle_Full;
        if (ctx->Light.ShadeModel == GL_FLAT)
            vmesa->regCmdA_End |= HC_HShading_FlatA;
        break;
    case GL_LINES:
        if (VIA_DEBUG) fprintf(stderr, "Lines\n");    
        vmesa->regCmdA_End = vmesa->regCmdA | HC_HPMType_Line | HC_HVCycle_Full;
        if (ctx->Light.ShadeModel == GL_FLAT)
            vmesa->regCmdA_End |= HC_HShading_FlatB; 
        break;
    case GL_LINE_LOOP:
    case GL_LINE_STRIP:
        if (VIA_DEBUG) fprintf(stderr, "Line Loop / Line Strip\n");
        vmesa->regCmdA_End = vmesa->regCmdA | HC_HPMType_Line | HC_HVCycle_AFP |
                             HC_HVCycle_AB | HC_HVCycle_NewB;
        regCmdB |= HC_HVCycle_AB | HC_HVCycle_NewB | HC_HLPrst_MASK;
        if (ctx->Light.ShadeModel == GL_FLAT)
            vmesa->regCmdA_End |= HC_HShading_FlatB; 
        break;
    case GL_TRIANGLES:
        if (VIA_DEBUG) fprintf(stderr, "Triangles\n");        
        vmesa->regCmdA_End = vmesa->regCmdA | HC_HPMType_Tri | HC_HVCycle_Full;
        if (ctx->Light.ShadeModel == GL_FLAT)
            vmesa->regCmdA_End |= HC_HShading_FlatC; 
        break;
    case GL_TRIANGLE_STRIP:
        if (VIA_DEBUG) fprintf(stderr, "Triangle Strip\n");
        vmesa->regCmdA_End = vmesa->regCmdA | HC_HPMType_Tri | HC_HVCycle_AFP |
                             HC_HVCycle_AC | HC_HVCycle_BB | HC_HVCycle_NewC;
        regCmdB |= HC_HVCycle_AA | HC_HVCycle_BC | HC_HVCycle_NewC;
        if (ctx->Light.ShadeModel == GL_FLAT)
            vmesa->regCmdA_End |= HC_HShading_FlatB; 
        break;
    case GL_TRIANGLE_FAN:
        if (VIA_DEBUG) fprintf(stderr, "Triangle Fan\n");
        vmesa->regCmdA_End = vmesa->regCmdA | HC_HPMType_Tri | HC_HVCycle_AFP |
                             HC_HVCycle_AA | HC_HVCycle_BC | HC_HVCycle_NewC;
        regCmdB |= HC_HVCycle_AA | HC_HVCycle_BC | HC_HVCycle_NewC;
        if (ctx->Light.ShadeModel == GL_FLAT)
            vmesa->regCmdA_End |= HC_HShading_FlatC; 
        break;
    case GL_QUADS:
        if (VIA_DEBUG) fprintf(stderr, "No HW Quads\n");
        return;
    case GL_QUAD_STRIP:
        if (VIA_DEBUG) fprintf(stderr, "No HW Quad Strip\n");
        return;
    case GL_POLYGON:
        if (VIA_DEBUG) fprintf(stderr, "Polygon\n");        
        vmesa->regCmdA_End = vmesa->regCmdA | HC_HPMType_Tri | HC_HVCycle_AFP |
                             HC_HVCycle_AA | HC_HVCycle_BC | HC_HVCycle_NewC;
        regCmdB |= HC_HVCycle_AA | HC_HVCycle_BC | HC_HVCycle_NewC;
        if (ctx->Light.ShadeModel == GL_FLAT)
            vmesa->regCmdA_End |= HC_HShading_FlatC; 
        break;                          
    default:
        if (VIA_DEBUG) fprintf(stderr, "Unknow\n");        
        return;
    }
    
    *vb++ = HC_HEADER2;    
    *vb++ = (HC_ParaType_NotTex << 16);
    *vb++ = 0xCCCCCCCC;
    *vb++ = 0xDDDDDDDD;

    *vb++ = HC_HEADER2;    
    *vb++ = (HC_ParaType_CmdVdata << 16);
    *vb++ = regCmdB;
    *vb++ = vmesa->regCmdA_End;
    vmesa->dmaLow += 32;
    
    vmesa->reducedPrimitive = rprim;
    vmesa->hwPrimitive = rprim;    
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}

void viaRasterPrimitiveFinish(GLcontext *ctx)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
    
    if (VIA_DEBUG) fprintf(stderr, "primitiveRendered = %x\n", vmesa->primitiveRendered);    
    if (RasterCounter > 1) {
	RasterCounter--;
	if (VIA_DEBUG) fprintf(stderr, "finish enter twice: %d\n",RasterCounter);
	return;
    }
    RasterCounter = 0;

    
    if (vmesa->primitiveRendered) {
        GLuint *vb = viaCheckDma(vmesa, 0);
	GLuint cmdA = vmesa->regCmdA_End | HC_HPLEND_MASK | HC_HPMValidN_MASK | HC_HE3Fire_MASK;    
	
	/*=* John Sheng [2003.6.20] fix pci *=*/
        /*if (vmesa->dmaLow & 0x1) {*/
	if (vmesa->dmaLow & 0x1 || !vmesa->useAgp) {
            *vb++ = cmdA ;
            vmesa->dmaLow += 4;
	}   
	else {      
    	    *vb++ = cmdA;
            *vb++ = cmdA;
	    vmesa->dmaLow += 8;
        }   
    }
    else {
	if (vmesa->dmaLow >=  (32 + DMA_OFFSET))	
	    vmesa->dmaLow -= 32;
    }
    
    if (0) viaFlushPrimsLocked(vmesa);
    if (0) {	
	volatile GLuint *pnMMIOBase = vmesa->regMMIOBase;
        volatile GLuint *pnEngBase = (volatile GLuint *)((GLuint)pnMMIOBase + 0x400);
        int nStatus = *pnEngBase;
	if (((nStatus & 0xFFFEFFFF) == 0x00020000)) {
#ifdef PERFORMANCE_MEASURE    
	    idle++;
#endif
	    viaFlushPrims(vmesa);        
	}
#ifdef PERFORMANCE_MEASURE    
	else {
	    busy++;
	}
#endif
    }	
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}


/**********************************************************************/
/*           Transition to/from hardware rasterization.               */
/**********************************************************************/


void viaFallback(viaContextPtr vmesa, GLuint bit, GLboolean mode)
{
    GLcontext *ctx = vmesa->glCtx;
    TNLcontext *tnl = TNL_CONTEXT(ctx);
    GLuint oldfallback = vmesa->Fallback;
    if (VIA_DEBUG) fprintf(stderr, "%s old %x bit %x mode %d\n", __FUNCTION__,
                   vmesa->Fallback, bit, mode);
    
    if (mode) {
        vmesa->Fallback |= bit;
        if (oldfallback == 0) {
            if (VIA_DEBUG) fprintf(stderr, "ENTER FALLBACK\n");
	    VIA_FIREVERTICES(vmesa);
            _swsetup_Wakeup(ctx);
            vmesa->renderIndex = ~0;
        }
    }
    else {
        vmesa->Fallback &= ~bit;
        if (oldfallback == bit) {
            if (VIA_DEBUG) fprintf(stderr, "LEAVE FALLBACK\n");
	    tnl->Driver.Render.Start = viaRenderStart;
            tnl->Driver.Render.PrimitiveNotify = viaRenderPrimitive;
            tnl->Driver.Render.Finish = viaRenderFinish;
            tnl->Driver.Render.BuildVertices = viaBuildVertices;
            vmesa->newState |= (_VIA_NEW_RENDERSTATE|_VIA_NEW_VERTEX);
        }
    }
    
}


/**********************************************************************/
/*                            Initialization.                         */
/**********************************************************************/


void viaInitTriFuncs(GLcontext *ctx)
{
    TNLcontext *tnl = TNL_CONTEXT(ctx);
    static int firsttime = 1;

    if (firsttime) {
        init_rast_tab();
        firsttime = 0;
    }

    tnl->Driver.RunPipeline = viaRunPipeline;
    tnl->Driver.Render.Start = viaRenderStart;
    tnl->Driver.Render.Finish = viaRenderFinish;
    tnl->Driver.Render.PrimitiveNotify = viaRenderPrimitive;
    tnl->Driver.Render.ResetLineStipple = _swrast_ResetLineStipple;
    tnl->Driver.Render.BuildVertices = viaBuildVertices;
}
