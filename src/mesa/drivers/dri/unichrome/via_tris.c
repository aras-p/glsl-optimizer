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
#include "enums.h"

#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#include "via_context.h"
#include "via_tris.h"
#include "via_state.h"
#include "via_vb.h"
#include "via_ioctl.h"

/***********************************************************************
 *                    Emit primitives as inline vertices               *
 ***********************************************************************/

#if 0
#define COPY_DWORDS(vb, vertsize, v)					\
    do {								\
        int j;								\
        int __tmp;							\
        __asm__ __volatile__("rep ; movsl"				\
                              : "=%c" (j), "=D" (vb), "=S" (__tmp)	\
                              : "0" (vertsize),				\
                                "D" ((long)vb),				\
                                "S" ((long)v));				\
    } while (0)
#else
#define COPY_DWORDS(vb, vertsize, v)		\
    do {					\
        int j;					\
        for (j = 0; j < vertsize; j++)		\
            vb[j] = ((GLuint *)v)[j];		\
        vb += vertsize;				\
    } while (0)
#endif

static void __inline__ via_draw_triangle(viaContextPtr vmesa,
                                         viaVertexPtr v0,
                                         viaVertexPtr v1,
                                         viaVertexPtr v2)
{
    GLuint vertsize = vmesa->vertexSize;
    GLuint *vb = viaAllocDma(vmesa, 3 * 4 * vertsize);
/*     fprintf(stderr, "%s: %p %p %p\n", __FUNCTION__, v0, v1, v2); */
    COPY_DWORDS(vb, vertsize, v0);
    COPY_DWORDS(vb, vertsize, v1);
    COPY_DWORDS(vb, vertsize, v2);
}


static void __inline__ via_draw_quad(viaContextPtr vmesa,
                                     viaVertexPtr v0,
                                     viaVertexPtr v1,
                                     viaVertexPtr v2,
                                     viaVertexPtr v3)
{
    GLuint vertsize = vmesa->vertexSize;
    GLuint *vb = viaAllocDma(vmesa, 6 * 4 * vertsize);

/*     fprintf(stderr, "%s: %p %p %p %p\n", __FUNCTION__, v0, v1, v2, v3); */
    COPY_DWORDS(vb, vertsize, v0);
    COPY_DWORDS(vb, vertsize, v1);
    COPY_DWORDS(vb, vertsize, v3);
    COPY_DWORDS(vb, vertsize, v1);
    COPY_DWORDS(vb, vertsize, v2);
    COPY_DWORDS(vb, vertsize, v3);
}

static __inline__ void via_draw_line(viaContextPtr vmesa,
                                     viaVertexPtr v0,
                                     viaVertexPtr v1)
{
    GLuint vertsize = vmesa->vertexSize;
    GLuint *vb = viaAllocDma(vmesa, 2 * 4 * vertsize);
    COPY_DWORDS(vb, vertsize, v0);
    COPY_DWORDS(vb, vertsize, v1);
}


static __inline__ void via_draw_point(viaContextPtr vmesa,
                                      viaVertexPtr v0)
{
    GLuint vertsize = vmesa->vertexSize;
    GLuint *vb = viaAllocDma(vmesa, 4 * vertsize);
    COPY_DWORDS(vb, vertsize, v0);
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
#define GET_VERTEX(e) (vmesa->verts + (e * vmesa->vertexSize * sizeof(int)))

#define VERT_SET_RGBA( v, c )  					\
do {								\
   via_color_t *color = (via_color_t *)&((v)->ui[coloroffset]);	\
   UNCLAMPED_FLOAT_TO_UBYTE(color->red, (c)[0]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->green, (c)[1]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->blue, (c)[2]);		\
   UNCLAMPED_FLOAT_TO_UBYTE(color->alpha, (c)[3]);		\
} while (0)

#define VERT_COPY_RGBA( v0, v1 ) v0->ui[coloroffset] = v1->ui[coloroffset]

#define VERT_SET_SPEC( v0, c )					\
do {								\
   if (havespec) {						\
      UNCLAMPED_FLOAT_TO_UBYTE(v0->v.specular.red, (c)[0]);	\
      UNCLAMPED_FLOAT_TO_UBYTE(v0->v.specular.green, (c)[1]);	\
      UNCLAMPED_FLOAT_TO_UBYTE(v0->v.specular.blue, (c)[2]);	\
   }								\
} while (0)
#define VERT_COPY_SPEC( v0, v1 )			\
do {							\
   if (havespec) {					\
      v0->v.specular.red   = v1->v.specular.red;	\
      v0->v.specular.green = v1->v.specular.green;	\
      v0->v.specular.blue  = v1->v.specular.blue; 	\
   }							\
} while (0)


#define VERT_SAVE_RGBA( idx )    color[idx] = v[idx]->ui[coloroffset]
#define VERT_RESTORE_RGBA( idx ) v[idx]->ui[coloroffset] = color[idx]
#define VERT_SAVE_SPEC( idx )    if (havespec) spec[idx] = v[idx]->ui[5]
#define VERT_RESTORE_SPEC( idx ) if (havespec) v[idx]->ui[5] = spec[idx]


#define LOCAL_VARS(n)                                                   \
    viaContextPtr vmesa = VIA_CONTEXT(ctx);                             \
    GLuint color[n], spec[n];                                           \
    GLuint coloroffset = (vmesa->vertexSize == 4 ? 3 : 4);              \
    GLboolean havespec = (vmesa->vertexSize > 4);                       \
    (void)color; (void)spec; (void)coloroffset; (void)havespec;


/***********************************************************************
 *                Helpers for rendering unfilled primitives            *
 ***********************************************************************/

static const GLenum hwPrim[GL_POLYGON + 1] = {
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


#define RASTERIZE(x) viaRasterPrimitive( ctx, x, hwPrim[x] )
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
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_OFFSET_BIT)
#define TAG(x) x##_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_TWOSIDE_BIT)
#define TAG(x) x##_twoside
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_TWOSIDE_BIT|VIA_OFFSET_BIT)
#define TAG(x) x##_twoside_offset
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_UNFILLED_BIT)
#define TAG(x) x##_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_OFFSET_BIT|VIA_UNFILLED_BIT)
#define TAG(x) x##_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_TWOSIDE_BIT|VIA_UNFILLED_BIT)
#define TAG(x) x##_twoside_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_TWOSIDE_BIT|VIA_OFFSET_BIT|VIA_UNFILLED_BIT)
#define TAG(x) x##_twoside_offset_unfilled
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_FALLBACK_BIT)
#define TAG(x) x##_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_OFFSET_BIT|VIA_FALLBACK_BIT)
#define TAG(x) x##_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_TWOSIDE_BIT|VIA_FALLBACK_BIT)
#define TAG(x) x##_twoside_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_TWOSIDE_BIT|VIA_OFFSET_BIT|VIA_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_UNFILLED_BIT|VIA_FALLBACK_BIT)
#define TAG(x) x##_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_OFFSET_BIT|VIA_UNFILLED_BIT|VIA_FALLBACK_BIT)
#define TAG(x) x##_offset_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_TWOSIDE_BIT|VIA_UNFILLED_BIT|VIA_FALLBACK_BIT)
#define TAG(x) x##_twoside_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"

#define IND (VIA_TWOSIDE_BIT|VIA_OFFSET_BIT|VIA_UNFILLED_BIT| \
             VIA_FALLBACK_BIT)
#define TAG(x) x##_twoside_offset_unfilled_fallback
#include "tnl_dd/t_dd_tritmp.h"


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
    via_translate_vertex(ctx, v0, &v[0]);
    via_translate_vertex(ctx, v1, &v[1]);
    via_translate_vertex(ctx, v2, &v[2]);
    viaSpanRenderStart( ctx );
    _swrast_Triangle(ctx, &v[0], &v[1], &v[2]);
    viaSpanRenderFinish( ctx );
}


static void
via_fallback_line(viaContextPtr vmesa,
                  viaVertex *v0,
                  viaVertex *v1)
{
    GLcontext *ctx = vmesa->glCtx;
    SWvertex v[2];
    via_translate_vertex(ctx, v0, &v[0]);
    via_translate_vertex(ctx, v1, &v[1]);
    viaSpanRenderStart( ctx );
    _swrast_Line(ctx, &v[0], &v[1]);
    viaSpanRenderFinish( ctx );
}


static void
via_fallback_point(viaContextPtr vmesa,
                   viaVertex *v0)
{
    GLcontext *ctx = vmesa->glCtx;
    SWvertex v[1];
    via_translate_vertex(ctx, v0, &v[0]);
    viaSpanRenderStart( ctx );
    _swrast_Point(ctx, &v[0]);
    viaSpanRenderFinish( ctx );
}

/**********************************************************************/
/*               Render unclipped begin/end objects                   */
/**********************************************************************/
#define IND 0
#define V(x) (viaVertex *)(vertptr + ((x) * vertsize * sizeof(int)))
#define RENDER_POINTS(start, count)   \
    for (; start < count; start++) POINT(V(ELT(start)));
#define RENDER_LINE(v0, v1)         LINE(V(v0), V(v1))
#define RENDER_TRI( v0, v1, v2)     TRI( V(v0), V(v1), V(v2))
#define RENDER_QUAD(v0, v1, v2, v3) QUAD(V(v0), V(v1), V(v2), V(v3))
#define INIT(x) viaRasterPrimitive(ctx, x, hwPrim[x])
#undef LOCAL_VARS
#define LOCAL_VARS                                              \
    viaContextPtr vmesa = VIA_CONTEXT(ctx);                     \
    GLubyte *vertptr = (GLubyte *)vmesa->verts;                 \
    const GLuint vertsize = vmesa->vertexSize;          \
    const GLuint * const elt = TNL_CONTEXT(ctx)->vb.Elts;       \
    (void)elt;
#define RESET_STIPPLE
#define RESET_OCCLUSION
#define PRESERVE_VB_DEFS
#define ELT(x) x
#define TAG(x) via_fast##x##_verts
#include "tnl/t_vb_rendertmp.h"
#undef ELT
#undef TAG
#define TAG(x) via_fast##x##_elts
#define ELT(x) elt[x]
#include "tnl/t_vb_rendertmp.h"
#undef ELT
#undef TAG
#undef NEED_EDGEFLAG_SETUP
#undef EDGEFLAG_GET
#undef EDGEFLAG_SET
#undef RESET_OCCLUSION


/**********************************************************************/
/*                   Render clipped primitives                        */
/**********************************************************************/



static void viaRenderClippedPoly(GLcontext *ctx, const GLuint *elts,
                                 GLuint n)
{
    TNLcontext *tnl = TNL_CONTEXT(ctx);
    struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
    GLuint prim = VIA_CONTEXT(ctx)->renderPrimitive;

    /* Render the new vertices as an unclipped polygon.
     */
    {
        GLuint *tmp = VB->Elts;
        VB->Elts = (GLuint *)elts;
        tnl->Driver.Render.PrimTabElts[GL_POLYGON](ctx, 0, n,
                                                   PRIM_BEGIN|PRIM_END);
        VB->Elts = tmp;
    }

    /* Restore the render primitive
     */
    if (prim != GL_POLYGON)
       tnl->Driver.Render.PrimitiveNotify( ctx, prim );
}

static void viaRenderClippedLine(GLcontext *ctx, GLuint ii, GLuint jj)
{
    TNLcontext *tnl = TNL_CONTEXT(ctx);
    tnl->Driver.Render.Line(ctx, ii, jj);
}

static void viaFastRenderClippedPoly(GLcontext *ctx, const GLuint *elts,
                                     GLuint n)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    GLuint vertsize = vmesa->vertexSize;
    GLuint *vb = viaAllocDma(vmesa, (n - 2) * 3 * 4 * vertsize);
    GLubyte *vertptr = (GLubyte *)vmesa->verts;
    const GLuint *start = (const GLuint *)V(elts[0]);
    int i;

    for (i = 2; i < n; i++) {
	COPY_DWORDS(vb, vertsize, V(elts[i - 1]));
        COPY_DWORDS(vb, vertsize, V(elts[i]));
	COPY_DWORDS(vb, vertsize, start);	
    }
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

/* Via does support line stipple in hardware, and it is partially
 * working in the older versions of this driver:
 */
#define LINE_FALLBACK (DD_LINE_STIPPLE)
#define POINT_FALLBACK (0)
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

        tnl->Driver.Render.Quad = rast_tab[index].quad;

        if (index == 0) {
            tnl->Driver.Render.PrimTabVerts = via_fastrender_tab_verts;
            tnl->Driver.Render.PrimTabElts = via_fastrender_tab_elts;
            tnl->Driver.Render.ClippedLine = line; /* from tritmp.h */
            tnl->Driver.Render.ClippedPolygon = viaFastRenderClippedPoly;
        }
        else {
            tnl->Driver.Render.PrimTabVerts = _tnl_render_tab_verts;
            tnl->Driver.Render.PrimTabElts = _tnl_render_tab_elts;
            tnl->Driver.Render.ClippedLine = viaRenderClippedLine;
            tnl->Driver.Render.ClippedPolygon = viaRenderClippedPoly;
        }
    }
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}





/**********************************************************************/
/*                 High level hooks for t_vb_render.c                 */
/**********************************************************************/

static void viaRunPipeline(GLcontext *ctx)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    
    if (vmesa->newState) {
       viaValidateState( ctx );

       if (!vmesa->Fallback) {
	  viaChooseVertexState(ctx);
	  viaChooseRenderState(ctx);
       }
    }

    _tnl_run_pipeline(ctx);
}

static void viaRenderStart(GLcontext *ctx)
{
    /* Check for projective texturing.  Make sure all texcoord
     * pointers point to something.  (fix in mesa?)
     */
    viaCheckTexSizes(ctx);
}

static void viaRenderFinish(GLcontext *ctx)
{
    if (VIA_CONTEXT(ctx)->renderIndex & VIA_FALLBACK_BIT)
        _swrast_flush(ctx);
    else
        VIA_FINISH_PRIM(VIA_CONTEXT(ctx));
}



/* System to flush dma and emit state changes based on the rasterized
 * primitive.
 */
void viaRasterPrimitive(GLcontext *ctx,
			GLenum glprim,
			GLenum hwprim)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    GLuint regCmdB;
    RING_VARS;

    if (VIA_DEBUG) 
       fprintf(stderr, "%s: %s/%s\n", __FUNCTION__, _mesa_lookup_enum_by_nr(glprim),
	       _mesa_lookup_enum_by_nr(hwprim));

    VIA_FINISH_PRIM(vmesa);

    vmesa->renderPrimitive = glprim;
    
    regCmdB = vmesa->regCmdB;

    switch (hwprim) {
    case GL_POINTS:
        vmesa->regCmdA_End = vmesa->regCmdA | HC_HPMType_Point | HC_HVCycle_Full;
        if (ctx->Light.ShadeModel == GL_FLAT)
            vmesa->regCmdA_End |= HC_HShading_FlatA;
        break;
    case GL_LINES:
        vmesa->regCmdA_End = vmesa->regCmdA | HC_HPMType_Line | HC_HVCycle_Full;
        if (ctx->Light.ShadeModel == GL_FLAT)
            vmesa->regCmdA_End |= HC_HShading_FlatB; 
        break;
    case GL_LINE_LOOP:
    case GL_LINE_STRIP:
        vmesa->regCmdA_End = vmesa->regCmdA | HC_HPMType_Line | HC_HVCycle_AFP |
                             HC_HVCycle_AB | HC_HVCycle_NewB;
        regCmdB |= HC_HVCycle_AB | HC_HVCycle_NewB | HC_HLPrst_MASK;
        if (ctx->Light.ShadeModel == GL_FLAT)
            vmesa->regCmdA_End |= HC_HShading_FlatB; 
        break;
    case GL_TRIANGLES:
        vmesa->regCmdA_End = vmesa->regCmdA | HC_HPMType_Tri | HC_HVCycle_Full;
        if (ctx->Light.ShadeModel == GL_FLAT)
            vmesa->regCmdA_End |= HC_HShading_FlatC; 
        break;
    case GL_TRIANGLE_STRIP:
        vmesa->regCmdA_End = vmesa->regCmdA | HC_HPMType_Tri | HC_HVCycle_AFP |
                             HC_HVCycle_AC | HC_HVCycle_BB | HC_HVCycle_NewC;
        regCmdB |= HC_HVCycle_AA | HC_HVCycle_BC | HC_HVCycle_NewC;
        if (ctx->Light.ShadeModel == GL_FLAT)
            vmesa->regCmdA_End |= HC_HShading_FlatB; 
        break;
    case GL_TRIANGLE_FAN:
        vmesa->regCmdA_End = vmesa->regCmdA | HC_HPMType_Tri | HC_HVCycle_AFP |
                             HC_HVCycle_AA | HC_HVCycle_BC | HC_HVCycle_NewC;
        regCmdB |= HC_HVCycle_AA | HC_HVCycle_BC | HC_HVCycle_NewC;
        if (ctx->Light.ShadeModel == GL_FLAT)
            vmesa->regCmdA_End |= HC_HShading_FlatC; 
        break;
    case GL_QUADS:
	abort();
        return;
    case GL_QUAD_STRIP:
	abort();
        return;
    case GL_POLYGON:
        vmesa->regCmdA_End = vmesa->regCmdA | HC_HPMType_Tri | HC_HVCycle_AFP |
                             HC_HVCycle_AA | HC_HVCycle_BC | HC_HVCycle_NewC;
        regCmdB |= HC_HVCycle_AA | HC_HVCycle_BC | HC_HVCycle_NewC;
        if (ctx->Light.ShadeModel == GL_FLAT)
            vmesa->regCmdA_End |= HC_HShading_FlatC; 
        break;                          
    default:
	abort();
        return;
    }
    
    assert((vmesa->dmaLow & 0x4) == 0);

    BEGIN_RING(8);
    OUT_RING( HC_HEADER2 );    
    OUT_RING( (HC_ParaType_NotTex << 16) );
    OUT_RING( 0xCCCCCCCC );
    OUT_RING( 0xDDDDDDDD );

    OUT_RING( HC_HEADER2 );    
    OUT_RING( (HC_ParaType_CmdVdata << 16) );
    OUT_RING( regCmdB );
    OUT_RING( vmesa->regCmdA_End );
    ADVANCE_RING();

    vmesa->dmaLastPrim = vmesa->dmaLow;
    vmesa->hwPrimitive = hwprim;    
}

/* Callback for mesa:
 */
static void viaRenderPrimitive( GLcontext *ctx, GLuint prim )
{
   viaRasterPrimitive( ctx, prim, hwPrim[prim] );
}


void viaFinishPrimitive(viaContextPtr vmesa)
{
    if (!vmesa->dmaLastPrim) {
       return;
    }
    else if (vmesa->dmaLow != vmesa->dmaLastPrim) {
	GLuint cmdA = vmesa->regCmdA_End | HC_HPLEND_MASK | HC_HPMValidN_MASK | HC_HE3Fire_MASK;    
	RING_VARS;

	/* KW: modified 0x1 to 0x4 below:
	 */
	if ((vmesa->dmaLow & 0x1) || !vmesa->useAgp) {
	   BEGIN_RING_NOCHECK( 1 );
	   OUT_RING( cmdA );
	   ADVANCE_RING();
	}   
	else {      
	   BEGIN_RING_NOCHECK( 2 );
	   OUT_RING( cmdA );
	   OUT_RING( cmdA );
	   ADVANCE_RING();
        }   
	vmesa->dmaLastPrim = 0;

	if (1 || vmesa->dmaLow > VIA_DMA_HIGHWATER)
	   viaFlushPrims( vmesa );
    }
    else {
       assert(vmesa->dmaLow >= (32 + DMA_OFFSET));
       vmesa->dmaLow -= 32;
       vmesa->dmaLastPrim = 0;
    }
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
	    VIA_FLUSH_DMA(vmesa);
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
