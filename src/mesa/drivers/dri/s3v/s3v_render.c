/*
 * Author: Max Lingua <sunmax@libero.it>
 */

#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "mtypes.h"

#include "tnl/t_context.h"

#include "s3v_context.h"
#include "s3v_tris.h"
#include "s3v_vb.h"


#define HAVE_POINTS      0
#define HAVE_LINES       0
#define HAVE_LINE_STRIPS 0
#define HAVE_TRIANGLES   0
#define HAVE_TRI_STRIPS  0
#define HAVE_TRI_STRIP_1 0
#define HAVE_TRI_FANS    0
#define HAVE_QUADS       0
#define HAVE_QUAD_STRIPS 0
#define HAVE_POLYGONS    0

#define HAVE_ELTS        0

#if 0
static void VERT_FALLBACK( GLcontext *ctx,
			   GLuint start,
			   GLuint count,
			   GLuint flags )
{
	TNLcontext *tnl = TNL_CONTEXT(ctx);
/*	s3vContextPtr vmesa = S3V_CONTEXT(ctx); */
	int _flags;
   
	DEBUG(("VERT_FALLBACK: flags & PRIM_MODE_MASK = %i\n",
		flags & PRIM_MODE_MASK));
	DEBUG(("VERT_FALLBACK: flags=%i PRIM_MODE_MASK=%i\n",
		flags, PRIM_MODE_MASK));
#if 0
	tnl->Driver.Render.PrimitiveNotify( ctx, flags & PRIM_MODE_MASK );
#endif
	tnl->Driver.Render.BuildVertices( ctx, start, count, ~0 );

	_flags = flags & PRIM_MODE_MASK;

	tnl->Driver.Render.PrimTabVerts[_flags]( ctx, start, count, flags );
	S3V_CONTEXT(ctx)->SetupNewInputs = VERT_BIT_POS;
}
#endif

static const GLuint hw_prim[GL_POLYGON+1] = {
	PrimType_Points,
	PrimType_Lines,
	PrimType_LineLoop,
	PrimType_LineStrip,
	PrimType_Triangles,
	PrimType_TriangleStrip,
	PrimType_TriangleFan,
	PrimType_Quads,
	PrimType_QuadStrip,
	PrimType_Polygon
};

static __inline void s3vStartPrimitive( s3vContextPtr vmesa, GLenum prim )
{
	__DRIdrawablePrivate *dPriv = vmesa->driDrawable;

	int _hw_prim = hw_prim[prim];

	DEBUG(("s3vStartPrimitive (new #%i) ", prim));

	if (_hw_prim != vmesa->restore_primitive) {

		if (prim == 4) { /* TRI */
			DEBUG(("switching to tri\n"));
			vmesa->prim_cmd = vmesa->_tri[vmesa->_3d_mode];
			vmesa->alpha_cmd = vmesa->_alpha[vmesa->_3d_mode];
			DMAOUT_CHECK(3DTRI_Z_BASE, 12);
		} else if (prim == 1) { /* LINE */
			DEBUG(("switching to line\n"));
			vmesa->prim_cmd = DO_3D_LINE;
			vmesa->alpha_cmd = vmesa->_alpha[0];
			DMAOUT_CHECK(3DLINE_Z_BASE, 12);
		} else {
			DEBUG(("Never mind the bollocks!\n"));
		}

        	DMAOUT(vmesa->s3vScreen->depthOffset & 0x003FFFF8);
	        DMAOUT(vmesa->DestBase);
		/* DMAOUT(vmesa->ScissorLR); */
		/* DMAOUT(vmesa->ScissorTB); */
		DMAOUT( (0 << 16) | (dPriv->w-1) );
		DMAOUT( (0 << 16) | (dPriv->h-1) );
		DMAOUT( (vmesa->SrcStride << 16) | vmesa->TexStride );
		DMAOUT(vmesa->SrcStride);
		DMAOUT(vmesa->TexOffset);
		DMAOUT(vmesa->TextureBorderColor);
		DMAOUT(0); /* FOG */
		DMAOUT(0);
		DMAOUT(0);
	        DMAOUT(vmesa->CMD | vmesa->prim_cmd | vmesa->alpha_cmd);
		DMAFINISH();
   	}

	vmesa->restore_primitive = _hw_prim;
}

static __inline void s3vEndPrimitive( s3vContextPtr vmesa )
{
/*	GLcontext *ctx = vmesa->glCtx; */
	DEBUG(("s3vEndPrimitive\n"));
}

#define LOCAL_VARS s3vContextPtr vmesa = S3V_CONTEXT(ctx)
#define INIT( prim ) s3vStartPrimitive( vmesa, prim )
#define FINISH s3vEndPrimitive( vmesa )
#define NEW_PRIMITIVE() (void) vmesa
#define NEW_BUFFER() (void) vmesa
#define FIRE_VERTICES() (void) vmesa
#define GET_CURRENT_VB_MAX_VERTS() \
	(vmesa->bufSize - vmesa->bufCount) / 2
#define GET_SUBSEQUENT_VB_MAX_VERTS() \
	S3V_DMA_BUF_SZ / 2
/* XXX */
#define ALLOC_VERTS(nr) NULL
#define EMIT_VERTS(ctx, start, count, buf) NULL
#define FLUSH() s3vEndPrimitive( vmesa )

#define TAG(x) s3v_##x

#include "tnl_dd/t_dd_dmatmp.h"

/**********************************************************************/
/*                          Render pipeline stage                     */
/**********************************************************************/


static GLboolean s3v_run_render( GLcontext *ctx,
				  struct tnl_pipeline_stage *stage )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);
	TNLcontext *tnl = TNL_CONTEXT(ctx);
	struct vertex_buffer *VB = &tnl->vb;
	GLuint i;
	tnl_render_func *tab;

	DEBUG(("s3v_run_render\n"));
	
	/* FIXME: hw clip */
	if (VB->ClipOrMask || vmesa->RenderIndex != 0) {
		DEBUG(("*** CLIPPED in render ***\n"));
#if 1
		return GL_TRUE;	/* don't handle clipping here */
#endif
	}


	/* We don't do elts */
	if (VB->Elts)
		return GL_TRUE;

	tab = TAG(render_tab_verts);

	tnl->Driver.Render.Start( ctx );

	for (i = 0 ; i < VB->PrimitiveCount ; i++ )
	{
                GLuint prim = VB->Primitive[i].mode;
		GLuint start = VB->Primitive[i].start;
		GLuint length = VB->Primitive[i].count;

		DEBUG(("s3v_run_render (loop=%i) (lenght=%i)\n", i, length));

		if (length) {
			tnl->Driver.Render.BuildVertices( ctx, start,
                                start+length, ~0 /*stage->inputs*/); /* XXX */
			tnl->Driver.Render.PrimTabVerts[prim & PRIM_MODE_MASK]
				( ctx, start, start + length, prim );
			vmesa->SetupNewInputs = VERT_BIT_POS;
		}
	}
	
	tnl->Driver.Render.Finish( ctx );

	return GL_FALSE; /* finished the pipe */
}


static void s3v_check_render( GLcontext *ctx,
				 struct tnl_pipeline_stage *stage )
{
	s3vContextPtr vmesa = S3V_CONTEXT(ctx);
	GLuint inputs = VERT_BIT_POS | VERT_BIT_COLOR0;

	DEBUG(("s3v_check_render\n"));

	if (ctx->RenderMode == GL_RENDER) {
	
		if (ctx->_TriangleCaps & DD_SEPARATE_SPECULAR) {
			DEBUG(("DD_SEPARATE_SPECULAR\n"));
			inputs |= VERT_BIT_COLOR1;
		}
		
		if (ctx->Texture.Unit[0]._ReallyEnabled) {
			DEBUG(("ctx->Texture.Unit[0]._ReallyEnabled\n"));
			inputs |= VERT_BIT_TEX(0);
		}

		if (ctx->Texture.Unit[1]._ReallyEnabled) {
			DEBUG(("ctx->Texture.Unit[1]._ReallyEnabled\n"));
			inputs |= VERT_BIT_TEX(1);
		}

		if (ctx->Fog.Enabled) {
			DEBUG(("ctx->Fog.Enabled\n"));
			inputs |= VERT_BIT_FOG;
		}
	}

	stage->inputs = inputs;
	vmesa->SetupNewInputs = inputs;
}


static void dtr( struct tnl_pipeline_stage *stage )
{
	(void)stage;
        /* hack to silence a compiler warning */
        (void) &s3v_validate_render;
}


const struct tnl_pipeline_stage _s3v_render_stage =
{
	"s3v render",
	(_DD_NEW_SEPARATE_SPECULAR |
	 _NEW_TEXTURE|
	 _NEW_FOG|
	 _NEW_RENDERMODE),	/* re-check (new inputs) */
	 0,			/* re-run (always runs) */
	 GL_TRUE,		/* active */
	 0, 0,			/* inputs (set in check_render), outputs */
	 0, 0,			/* changed_inputs, private */
	 dtr,			/* destructor */
	 s3v_check_render,	/* check - initially set to alloc data */
	 s3v_run_render		/* run */
};
