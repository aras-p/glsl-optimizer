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


/*
 * Render unclipped vertex buffers by emitting vertices directly to
 * dma buffers.  Use strip/fan hardware acceleration where possible.
 *
 */
#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "mtypes.h"

#include "tnl/t_context.h"

#include "via_context.h"
#include "via_tris.h"
#include "via_state.h"
#include "via_vb.h"
#include "via_ioctl.h"

/*
 * Render unclipped vertex buffers by emitting vertices directly to
 * dma buffers.  Use strip/fan hardware primitives where possible.
 * Try to simulate missing primitives with indexed vertices.
 */
#define HAVE_POINTS      1
#define HAVE_LINES       1
#define HAVE_LINE_STRIPS 1
#define HAVE_LINE_LOOP   1
#define HAVE_TRIANGLES   1
#define HAVE_TRI_STRIPS  1
#define HAVE_TRI_STRIP_1 0      /* has it, template can't use it yet */
#define HAVE_TRI_FANS    1
#define HAVE_POLYGONS    1
#define HAVE_QUADS       0
#define HAVE_QUAD_STRIPS 0

#define HAVE_ELTS        0

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

/* Fallback to normal rendering.
 */
static void VERT_FALLBACK(GLcontext *ctx,
                          GLuint start,
                          GLuint count,
                          GLuint flags)
{
    TNLcontext *tnl = TNL_CONTEXT(ctx);
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
#endif
    fprintf(stderr, "VERT_FALLBACK\n");
    tnl->Driver.Render.PrimitiveNotify(ctx, flags & PRIM_MODE_MASK);
    tnl->Driver.Render.BuildVertices(ctx, start, count, ~0);
    tnl->Driver.Render.PrimTabVerts[flags & PRIM_MODE_MASK](ctx, start,
                                                            count, flags);
    VIA_CONTEXT(ctx)->setupNewInputs = VERT_BIT_CLIP;
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);
#endif
}


#define LOCAL_VARS viaContextPtr vmesa = VIA_CONTEXT(ctx)
/*
#define INIT(prim)                                                          \
    do {                                                                    \
        VIA_STATECHANGE(vmesa, 0);                                          \
        viaRasterPrimitive(ctx, reducedPrim[prim], prim);                   \
    } while (0)
*/
#define INIT(prim)                                                          \
    do {                                                                    \
        viaRasterPrimitive(ctx, reducedPrim[prim], prim);                   \
    } while (0)
#define NEW_PRIMITIVE()  VIA_STATECHANGE(vmesa, 0)
#define NEW_BUFFER()  VIA_FIREVERTICES(vmesa)
#define GET_CURRENT_VB_MAX_VERTS() \
    (((int)vmesa->dmaHigh - (int)vmesa->dmaLow) / (vmesa->vertexSize * 4))
#define GET_SUBSEQUENT_VB_MAX_VERTS() \
    (VIA_DMA_BUF_SZ - 4) / (vmesa->vertexSize * 4)


#define EMIT_VERTS(ctx, j, nr) \
    via_emit_contiguous_verts(ctx, j, (j) + (nr))
    
#define FINISH                                                          \
    do {                                                                \
        vmesa->primitiveRendered = GL_TRUE;                             \
        viaRasterPrimitiveFinish(ctx);                                  \
    } while (0)                                                       


#define TAG(x) via_fast##x
#include "via_dmatmp.h"
#undef TAG
#undef LOCAL_VARS
#undef INIT

/**********************************************************************/
/*                          Fast Render pipeline stage                */
/**********************************************************************/
static GLboolean via_run_fastrender(GLcontext *ctx,
                                    struct tnl_pipeline_stage *stage)
{
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    TNLcontext *tnl = TNL_CONTEXT(ctx);
    struct vertex_buffer *VB = &tnl->vb;
    GLuint i, length, flags = 0;
    
    /* Don't handle clipping or indexed vertices.
     */
#ifdef PERFORMANCE_MEASURE
    if (VIA_PERFORMANCE) P_M;
#endif
    
    if (VB->ClipOrMask || vmesa->renderIndex != 0 || VB->Elts) {
#ifdef DEBUG
	if (VIA_DEBUG) { 
	    fprintf(stderr, "slow path\n");    
	    fprintf(stderr, "ClipOrMask = %08x\n", VB->ClipOrMask);
	    fprintf(stderr, "renderIndex = %08x\n", vmesa->renderIndex);	
	    fprintf(stderr, "Elts = %08x\n", (GLuint)VB->Elts);	
	}	    
#endif
        return GL_TRUE;
    }
#ifdef DEBUG    
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
#endif    
    vmesa->setupNewInputs = VERT_BIT_CLIP;
    vmesa->primitiveRendered = GL_TRUE;

    tnl->Driver.Render.Start(ctx);
    
    for (i = 0; i < VB->PrimitiveCount; ++i) {
        GLuint mode = VB->Primitive[i].mode;
        GLuint start = VB->Primitive[i].start;
        GLuint length = VB->Primitive[i].count;
        if (length)
            via_fastrender_tab_verts[mode & PRIM_MODE_MASK](ctx, start, start+length, mode);
    }

    tnl->Driver.Render.Finish(ctx);
    
    /*=* DBG - viewperf7.0 : fix command buffer overflow *=*/
    if (vmesa->dmaLow > (VIA_DMA_BUFSIZ / 2))
	viaFlushPrims(vmesa);
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
#endif
    return GL_FALSE;            /* finished the pipe */
}


static void via_check_fastrender(GLcontext *ctx, struct tnl_pipeline_stage *stage)
{
    GLuint inputs = VERT_BIT_CLIP | VERT_BIT_COLOR0;
    
    if (ctx->RenderMode == GL_RENDER) {
        if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR)
            inputs |= VERT_BIT_COLOR1;

        if (ctx->Texture.Unit[0]._ReallyEnabled)
            inputs |= VERT_BIT_TEX0;

        if (ctx->Texture.Unit[1]._ReallyEnabled)
            inputs |= VERT_BIT_TEX1;

        if (ctx->Fog.Enabled)
            inputs |= VERT_BIT_FOG;
    }

    stage->inputs = inputs;
}


static void fastdtr(struct tnl_pipeline_stage *stage)
{
    (void)stage;
}


const struct tnl_pipeline_stage _via_fastrender_stage =
{
    "via fast render",
    (_DD_NEW_SEPARATE_SPECULAR |
     _NEW_TEXTURE|
     _NEW_FOG|
     _NEW_RENDERMODE),           /* re-check (new inputs) */
    0,                           /* re-run (always runs) */
    GL_TRUE,                     /* active */
    0, 0,                        /* inputs (set in check_render), outputs */
    0, 0,                        /* changed_inputs, private */
    fastdtr,                     /* destructor */
    via_check_fastrender,        /* check - initially set to alloc data */
    via_run_fastrender           /* run */
};


/*
 * Render whole vertex buffers, including projection of vertices from
 * clip space and clipping of primitives.
 *
 * This file makes calls to project vertices and to the point, line
 * and triangle rasterizers via the function pointers:
 *
 *    context->Driver.Render.*
 *
 */


/**********************************************************************/
/*                        Clip single primitives                      */
/**********************************************************************/
#undef DIFFERENT_SIGNS
#if defined(USE_IEEE)
#define NEGATIVE(x) (GET_FLOAT_BITS(x) & (1 << 31))
#define DIFFERENT_SIGNS(x, y) ((GET_FLOAT_BITS(x) ^ GET_FLOAT_BITS(y)) & (1 << 31))
#else
#define NEGATIVE(x) (x < 0)
#define DIFFERENT_SIGNS(x,y) (x * y <= 0 && x - y != 0)
/* Could just use (x*y<0) except for the flatshading requirements.
 * Maybe there's a better way?
 */
#endif

#define W(i) coord[i][3]
#define Z(i) coord[i][2]
#define Y(i) coord[i][1]
#define X(i) coord[i][0]
#define SIZE 4
#define TAG(x) x##_4
#include "via_vb_cliptmp.h"


/**********************************************************************/
/*              Clip and render whole begin/end objects               */
/**********************************************************************/
#define NEED_EDGEFLAG_SETUP (ctx->_TriangleCaps & DD_TRI_UNFILLED)
#define EDGEFLAG_GET(idx) VB->EdgeFlag[idx]
#define EDGEFLAG_SET(idx, val) VB->EdgeFlag[idx] = val


/* Vertices, with the possibility of clipping.
 */
#define RENDER_POINTS(start, count)  		\
   tnl->Driver.Render.Points(ctx, start, count)

#define RENDER_LINE(v1, v2)	 			\
    do {						\
	GLubyte c1 = mask[v1], c2 = mask[v2];		\
	GLubyte ormask = c1 | c2;			\
	if (!ormask)					\
    	    LineFunc(ctx, v1, v2);			\
	else if (!(c1 & c2 & 0x3f))			\
    	    clip_line_4(ctx, v1, v2, ormask);		\
    } while (0)

#define RENDER_TRI(v1, v2, v3)					\
    if (VIA_DEBUG) fprintf(stderr, "RENDER_TRI - clip\n");      \
    do {							\
	GLubyte c1 = mask[v1], c2 = mask[v2], c3 = mask[v3];	\
	GLubyte ormask = c1 | c2 | c3;				\
	if (!ormask)						\
    	    TriangleFunc(ctx, v1, v2, v3);			\
	else if (!(c1 & c2 & c3 & 0x3f)) 			\
    	    clip_tri_4(ctx, v1, v2, v3, ormask);    		\
    } while (0)

#define RENDER_QUAD(v1, v2, v3, v4)				\
    do {							\
	GLubyte c1 = mask[v1], c2 = mask[v2];			\
	GLubyte c3 = mask[v3], c4 = mask[v4];			\
	GLubyte ormask = c1 | c2 | c3 | c4;			\
	if (!ormask)						\
    	    QuadFunc(ctx, v1, v2, v3, v4);			\
	else if (!(c1 & c2 & c3 & c4 & 0x3f)) 			\
    	    clip_quad_4(ctx, v1, v2, v3, v4, ormask);		\
    } while (0)


#define LOCAL_VARS							\
    TNLcontext *tnl = TNL_CONTEXT(ctx);					\
    struct vertex_buffer *VB = &tnl->vb;				\
    const GLuint * const elt = VB->Elts;				\
    const GLubyte *mask = VB->ClipMask;					\
    const GLuint sz = VB->ClipPtr->size;				\
    const tnl_line_func LineFunc = tnl->Driver.Render.Line;			\
    const tnl_triangle_func TriangleFunc = tnl->Driver.Render.Triangle;	\
    const tnl_quad_func QuadFunc = tnl->Driver.Render.Quad;			\
    const GLboolean stipple = ctx->Line.StippleFlag;			\
    (void) (LineFunc && TriangleFunc && QuadFunc);			\
    (void) elt; (void) mask; (void) sz; (void) stipple;
    
#define POSTFIX                                                         \
    viaRasterPrimitiveFinish(ctx)	                    

#define TAG(x) clip_##x##_verts
#define INIT(x) tnl->Driver.Render.PrimitiveNotify(ctx, x)
#define RESET_STIPPLE if (stipple) tnl->Driver.Render.ResetLineStipple(ctx)
#define RESET_OCCLUSION ctx->OcclusionResult = GL_TRUE
#define PRESERVE_VB_DEFS
#include "via_vb_rendertmp.h"


/* Elts, with the possibility of clipping.
 */
#undef ELT
#undef TAG
#define ELT(x) elt[x]
#define TAG(x) clip_##x##_elts
#include "via_vb_rendertmp.h"

/* TODO: do this for all primitives, verts and elts:
 */
static void clip_elt_triangles(GLcontext *ctx,
			       GLuint start,
			       GLuint count,
			       GLuint flags)
{
    TNLcontext *tnl = TNL_CONTEXT(ctx);
    tnl_render_func render_tris = tnl->Driver.Render.PrimTabElts[GL_TRIANGLES];
    struct vertex_buffer *VB = &tnl->vb;
    const GLuint * const elt = VB->Elts;
    GLubyte *mask = VB->ClipMask;
    GLuint last = count-2;
    GLuint j;
    (void)flags;
#ifdef PERFORMANCE_MEASURE
    if (VIA_PERFORMANCE) P_M;
#endif    
    tnl->Driver.Render.PrimitiveNotify(ctx, GL_TRIANGLES);

    for (j = start; j < last; j += 3) {
        GLubyte c1 = mask[elt[j]];
        GLubyte c2 = mask[elt[j + 1]];
        GLubyte c3 = mask[elt[j + 2]];
        GLubyte ormask = c1 | c2 | c3;
        if (ormask) {
	    if (start < j)
		render_tris(ctx, start, j, 0);
	    if (!(c1 & c2 & c3 & 0x3f))
		clip_tri_4(ctx, elt[j], elt[j + 1], elt[j + 2], ormask);
	    start = j+3;
        }
    }	

    if (start < j)
       render_tris(ctx, start, j, 0);
    
    viaRasterPrimitiveFinish(ctx);
}


/**********************************************************************/
/*              Helper functions for drivers                  	      */
/**********************************************************************/
/*
void _tnl_RenderClippedPolygon(GLcontext *ctx, const GLuint *elts, GLuint n)
{
    TNLcontext *tnl = TNL_CONTEXT(ctx);
    struct vertex_buffer *VB = &tnl->vb;
    GLuint *tmp = VB->Elts;

    VB->Elts = (GLuint *)elts;
    tnl->Driver.Render.PrimTabElts[GL_POLYGON](ctx, 0, n, PRIM_BEGIN|PRIM_END);
    VB->Elts = tmp;
}

void _tnl_RenderClippedLine(GLcontext *ctx, GLuint ii, GLuint jj)
{
    TNLcontext *tnl = TNL_CONTEXT(ctx);
    tnl->Driver.Render.Line(ctx, ii, jj);
}
*/

/**********************************************************************/
/*                          Render pipeline stage                     */
/**********************************************************************/
static GLboolean via_run_render(GLcontext *ctx,
                                struct tnl_pipeline_stage *stage)
{
    TNLcontext *tnl = TNL_CONTEXT(ctx);
    viaContextPtr vmesa = VIA_CONTEXT(ctx);
    struct vertex_buffer *VB = &tnl->vb;
    /* DBG */
    GLuint newInputs = stage->changed_inputs;
    /*GLuint newInputs = stage->inputs;*/
    
    tnl_render_func *tab;
    GLuint pass = 0;
    
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);
#endif
#ifdef PERFORMANCE_MEASURE
    if (VIA_PERFORMANCE) P_M;
#endif
    tnl->Driver.Render.Start(ctx);
    tnl->Driver.Render.BuildVertices(ctx, 0, VB->Count, newInputs);
    if (VB->ClipOrMask) {
	tab = VB->Elts ? clip_render_tab_elts : clip_render_tab_verts;
	clip_render_tab_elts[GL_TRIANGLES] = clip_elt_triangles;
    }
    else {
	tab = VB->Elts ? tnl->Driver.Render.PrimTabElts : tnl->Driver.Render.PrimTabVerts;
    }
    	
    do {
	GLuint i;
	
        for (i = 0; i < VB->PrimitiveCount; i++) {
            GLuint flags = VB->Primitive[i].mode;
            GLuint start = VB->Primitive[i].start;
	    GLuint length= VB->Primitive[i].count;
	    ASSERT(length || (flags & PRIM_END));
	    ASSERT((flags & PRIM_MODE_MASK) <= GL_POLYGON + 1);
    	    if (length)
        	tab[flags & PRIM_MODE_MASK](ctx, start, start + length,flags);
	}		
    }
    while (tnl->Driver.Render.Multipass && tnl->Driver.Render.Multipass(ctx, ++pass));
    tnl->Driver.Render.Finish(ctx);
    
    /*=* DBG - flush : if hw idel *=*/
    /*{
        GLuint volatile *pnEnginStatus = vmesa->regEngineStatus;
	GLuint nStatus;
        nStatus = *pnEnginStatus;
	if ((nStatus & 0xFFFEFFFF) == 0x00020000)
	    viaFlushPrims(vmesa);
    }*/
    
    /*=* DBG viewperf7.0 : fix command buffer overflow *=*/
    if (vmesa->dmaLow > (VIA_DMA_BUFSIZ / 2))
	viaFlushPrims(vmesa);
#ifdef DEBUG
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
#endif
    return GL_FALSE;            /* finished the pipe */
}

/* Quite a bit of work involved in finding out the inputs for the
 * render stage.
 */

static void via_check_render(GLcontext *ctx, struct tnl_pipeline_stage *stage)
{
    GLuint inputs = VERT_BIT_CLIP;
    
    if (ctx->Visual.rgbMode) {
	inputs |= VERT_BIT_COLOR0;
	
        if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR)
 	    inputs |= VERT_BIT_COLOR1;
	    
        if (ctx->Texture.Unit[0]._ReallyEnabled) {
	 	    inputs |= VERT_BIT_TEX0;
	}
	
	if (ctx->Texture.Unit[1]._ReallyEnabled) {
	 	    inputs |= VERT_BIT_TEX1;
	}
    }
    else {
        /*inputs |= VERT_BIT_INDEX;*/
    }	
	    
    /*if (ctx->Point._Attenuated)
        inputs |= VERT_POINT_SIZE;*/

    if (ctx->Fog.Enabled)
        inputs |= VERT_BIT_FOG;

    /*if (ctx->_TriangleCaps & DD_TRI_UNFILLED)
        inputs |= VERT_EDGE;

    if (ctx->RenderMode == GL_FEEDBACK)
        inputs |= VERT_TEX_ANY;*/

    stage->inputs = inputs;
}


static void dtr(struct tnl_pipeline_stage *stage)
{
    (void)stage;
}


const struct tnl_pipeline_stage _via_render_stage =
{
    "via render",
    (_NEW_BUFFERS |
     _DD_NEW_SEPARATE_SPECULAR |
     _DD_NEW_FLATSHADE |
     _NEW_TEXTURE |
     _NEW_LIGHT |     
     _NEW_POINT |
     _NEW_FOG |
     _DD_NEW_TRI_UNFILLED |
     _NEW_RENDERMODE),           /* re-check (new inputs) */
     0,                          /* re-run (always runs) */
     GL_TRUE,                    /* active */
     0, 0,                       /* inputs (set in check_render), outputs */
     0, 0,                       /* changed_inputs, private */
     dtr,                        /* destructor */
     via_check_render,           /* check - initially set to alloc data */
     via_run_render              /* run */
};
