/*
 * Intel i810 DRI driver for Mesa 3.5
 *
 * Copyright (C) 1999-2000  Keith Whitwell   All Rights Reserved.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL KEITH WHITWELL BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Author:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */


/*
 * Render unclipped vertex buffers by emitting vertices directly to
 * dma buffers.  Use strip/fan hardware acceleration where possible.
 *
 */
#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "imports.h"
#include "mtypes.h"

#include "tnl/t_context.h"

#include "i810screen.h"
#include "i810_dri.h"

#include "i810context.h"
#include "i810tris.h"
#include "i810state.h"
#include "i810vb.h"
#include "i810ioctl.h"

/*
 * Render unclipped vertex buffers by emitting vertices directly to
 * dma buffers.  Use strip/fan hardware primitives where possible.
 * Try to simulate missing primitives with indexed vertices.
 */
#define HAVE_POINTS      0
#define HAVE_LINES       1
#define HAVE_LINE_STRIPS 1
#define HAVE_TRIANGLES   1
#define HAVE_TRI_STRIPS  1
#define HAVE_TRI_STRIP_1 0	/* has it, template can't use it yet */
#define HAVE_TRI_FANS    1
#define HAVE_POLYGONS    1
#define HAVE_QUADS       0
#define HAVE_QUAD_STRIPS 0

#define HAVE_ELTS        0


static GLuint hw_prim[GL_POLYGON+1] = {
   0,
   PR_LINES,
   0,
   PR_LINESTRIP,
   PR_TRIANGLES,
   PR_TRISTRIP_0,
   PR_TRIFAN,
   0,
   0,
   PR_POLYGON
};

static const GLenum reduced_prim[GL_POLYGON+1] = {
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
static void VERT_FALLBACK( GLcontext *ctx,
			   GLuint start,
			   GLuint count,
			   GLuint flags )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   tnl->Driver.Render.PrimitiveNotify( ctx, flags & PRIM_MODE_MASK );
   tnl->Driver.Render.BuildVertices( ctx, start, count, ~0 );
   tnl->Driver.Render.PrimTabVerts[flags&PRIM_MODE_MASK]( ctx, start, 
							  count, flags );
   I810_CONTEXT(ctx)->SetupNewInputs = VERT_BIT_CLIP;
}



#define LOCAL_VARS i810ContextPtr imesa = I810_CONTEXT(ctx)
#define INIT( prim ) do {						\
   I810_STATECHANGE(imesa, 0);						\
   i810RasterPrimitive( ctx, reduced_prim[prim], hw_prim[prim] );	\
} while (0)
#define NEW_PRIMITIVE()  I810_STATECHANGE( imesa, 0 )
#define NEW_BUFFER()  I810_FIREVERTICES( imesa )
#define GET_CURRENT_VB_MAX_VERTS() \
  (((int)imesa->vertex_high - (int)imesa->vertex_low) / (imesa->vertex_size*4))
#define GET_SUBSEQUENT_VB_MAX_VERTS() \
  (I810_DMA_BUF_SZ-4) / (imesa->vertex_size * 4)


#define EMIT_VERTS( ctx, j, nr ) \
  i810_emit_contiguous_verts(ctx, j, (j)+(nr))


#define TAG(x) i810_##x
#include "tnl_dd/t_dd_dmatmp.h"


/**********************************************************************/
/*                          Render pipeline stage                     */
/**********************************************************************/


static GLboolean i810_run_render( GLcontext *ctx,
				  struct gl_pipeline_stage *stage )
{
   i810ContextPtr imesa = I810_CONTEXT(ctx);
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i, length, flags = 0;

   /* Don't handle clipping or indexed vertices.
    */
   if (VB->ClipOrMask || imesa->RenderIndex != 0 || VB->Elts) {
      return GL_TRUE;
   }

   imesa->SetupNewInputs = VERT_BIT_CLIP;

   tnl->Driver.Render.Start( ctx );

   for (i = VB->FirstPrimitive ; !(flags & PRIM_LAST) ; i += length)
   {
      flags = VB->Primitive[i];
      length= VB->PrimitiveLength[i];
      if (length)
	 i810_render_tab_verts[flags & PRIM_MODE_MASK]( ctx, i, i + length,
							flags );
   }

   tnl->Driver.Render.Finish( ctx );

   return GL_FALSE;		/* finished the pipe */
}


static void i810_check_render( GLcontext *ctx, struct gl_pipeline_stage *stage )
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


static void dtr( struct gl_pipeline_stage *stage )
{
   (void)stage;
}


const struct gl_pipeline_stage _i810_render_stage =
{
   "i810 render",
   (_DD_NEW_SEPARATE_SPECULAR |
    _NEW_TEXTURE|
    _NEW_FOG|
    _NEW_RENDERMODE),		/* re-check (new inputs) */
   0,				/* re-run (always runs) */
   GL_TRUE,			/* active */
   0, 0,			/* inputs (set in check_render), outputs */
   0, 0,			/* changed_inputs, private */
   dtr,				/* destructor */
   i810_check_render,		/* check - initially set to alloc data */
   i810_run_render		/* run */
};
