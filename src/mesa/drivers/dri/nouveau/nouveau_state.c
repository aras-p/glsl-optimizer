/**************************************************************************

Copyright 2006 Jeremy Kolb
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

#include "nouveau_context.h"
#include "nouveau_state.h"
#include "nouveau_ioctl.h"
#include "nouveau_tris.h"

#include "swrast/swrast.h"
#include "array_cache/acache.h"
#include "tnl/tnl.h"
#include "swrast_setup/swrast_setup.h"

#include "tnl/t_pipeline.h"

static void nouveauCalcViewport(GLcontext *ctx)
{
    /* Calculate the Viewport Matrix */
    
    nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
    const GLfloat *v = ctx->Viewport._WindowMap.m;
    GLfloat *m = nmesa->viewport.m;
    GLint h = 0;
    
    if (nmesa->driDrawable) 
       h = nmesa->driDrawable->h + SUBPIXEL_Y;
    
    m[MAT_SX] =   v[MAT_SX];
    m[MAT_TX] =   v[MAT_TX] + SUBPIXEL_X;
    m[MAT_SY] = - v[MAT_SY];
    m[MAT_TY] = - v[MAT_TY] + h;
    m[MAT_SZ] =   v[MAT_SZ] * nmesa->depth_scale;
    m[MAT_TZ] =   v[MAT_TZ] * nmesa->depth_scale;

}

static nouveauViewport(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
    /* 
     * Need to send (at least on an nv35 the following:
     * cons = 4 (this may be bytes per pixel)
     *
     * The viewport:
     * 445   0x0000bee0   {size: 0x0   channel: 0x1   cmd: 0x00009ee0} <-- VIEWPORT_SETUP/HEADER ?
     * 446   0x00000000   {size: 0x0   channel: 0x0   cmd: 0x00000000} <-- x * cons
     * 447   0x00000c80   {size: 0x0   channel: 0x0   cmd: 0x00000c80} <-- (height + x) * cons
     * 448   0x00000000   {size: 0x0   channel: 0x0   cmd: 0x00000000} <-- y * cons
     * 449   0x00000960   {size: 0x0   channel: 0x0   cmd: 0x00000960} <-- (width + y) * cons
     * 44a   0x00082a00   {size: 0x2   channel: 0x1   cmd: 0x00000a00} <-- VIEWPORT_DIMS
     * 44b   0x04000000  <-- (Width_from_glViewport << 16) | x
     * 44c   0x03000000  <-- (Height_from_glViewport << 16) | (win_height - height - y)
     *
     */
    
    nouveauCalcViewport(ctx);
}

void nouveauDepthRange(GLcontext *ctx)
{
    nouveauCalcViewport(ctx);
}

/* Initialize the context's hardware state. */
void nouveauDDInitState(nouveauContextPtr nmesa)
{

}

/* Initialize the driver's state functions */
void nouveauDDInitStateFuncs(GLcontext *ctx)
{
   ctx->Driver.UpdateState		= NULL; //nouveauDDInvalidateState;

   ctx->Driver.ClearIndex		= NULL;
   ctx->Driver.ClearColor		= NULL; //nouveauDDClearColor;
   ctx->Driver.ClearStencil		= NULL; //nouveauDDClearStencil;
   ctx->Driver.DrawBuffer		= NULL; //nouveauDDDrawBuffer;
   ctx->Driver.ReadBuffer		= NULL; //nouveauDDReadBuffer;

   ctx->Driver.IndexMask		= NULL;
   ctx->Driver.ColorMask		= NULL; //nouveauDDColorMask;
   ctx->Driver.AlphaFunc		= NULL; //nouveauDDAlphaFunc;
   ctx->Driver.BlendEquationSeparate	= NULL; //nouveauDDBlendEquationSeparate;
   ctx->Driver.BlendFuncSeparate	= NULL; //nouveauDDBlendFuncSeparate;
   ctx->Driver.ClearDepth		= NULL; //nouveauDDClearDepth;
   ctx->Driver.CullFace			= NULL; //nouveauDDCullFace;
   ctx->Driver.FrontFace		= NULL; //nouveauDDFrontFace;
   ctx->Driver.DepthFunc		= NULL; //nouveauDDDepthFunc;
   ctx->Driver.DepthMask		= NULL; //nouveauDDDepthMask;
   ctx->Driver.Enable			= NULL; //nouveauDDEnable;
   ctx->Driver.Fogfv			= NULL; //nouveauDDFogfv;
   ctx->Driver.Hint			= NULL;
   ctx->Driver.Lightfv			= NULL;
   ctx->Driver.LightModelfv		= NULL; //nouveauDDLightModelfv;
   ctx->Driver.LogicOpcode		= NULL; //nouveauDDLogicOpCode;
   ctx->Driver.PolygonMode		= NULL;
   ctx->Driver.PolygonStipple		= NULL; //nouveauDDPolygonStipple;
   ctx->Driver.RenderMode		= NULL; //nouveauDDRenderMode;
   ctx->Driver.Scissor			= NULL; //nouveauDDScissor;
   ctx->Driver.ShadeModel		= NULL; //nouveauDDShadeModel;
   ctx->Driver.StencilFuncSeparate	= NULL; //nouveauDDStencilFuncSeparate;
   ctx->Driver.StencilMaskSeparate	= NULL; //nouveauDDStencilMaskSeparate;
   ctx->Driver.StencilOpSeparate	= NULL; //nouveauDDStencilOpSeparate;

   ctx->Driver.DepthRange               = nouveauDepthRange;
   ctx->Driver.Viewport                 = nouveauViewport;

   /* Pixel path fallbacks.
    */
   ctx->Driver.Accum = _swrast_Accum;
   ctx->Driver.Bitmap = _swrast_Bitmap;
   ctx->Driver.CopyPixels = _swrast_CopyPixels;
   ctx->Driver.DrawPixels = _swrast_DrawPixels;
   ctx->Driver.ReadPixels = _swrast_ReadPixels;

   /* Swrast hooks for imaging extensions:
    */
   ctx->Driver.CopyColorTable = _swrast_CopyColorTable;
   ctx->Driver.CopyColorSubTable = _swrast_CopyColorSubTable;
   ctx->Driver.CopyConvolutionFilter1D = _swrast_CopyConvolutionFilter1D;
   ctx->Driver.CopyConvolutionFilter2D = _swrast_CopyConvolutionFilter2D;
}
