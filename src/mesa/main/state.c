/* $Id: state.c,v 1.68 2001/06/18 17:26:08 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
 */


/*
 * This file manages recalculation of derived values in the
 * __GLcontext.
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include "glheader.h"
#include "accum.h"
#include "api_loopback.h"
#include "attrib.h"
#include "blend.h"
#include "buffers.h"
#include "clip.h"
#include "colortab.h"
#include "context.h"
#include "convolve.h"
#include "depth.h"
#include "dlist.h"
#include "drawpix.h"
#include "enable.h"
#include "eval.h"
#include "get.h"
#include "feedback.h"
#include "fog.h"
#include "hint.h"
#include "histogram.h"
#include "light.h"
#include "lines.h"
#include "matrix.h"
#include "mmath.h"
#include "pixel.h"
#include "points.h"
#include "polygon.h"
#include "rastpos.h"
#include "state.h"
#include "stencil.h"
#include "teximage.h"
#include "texobj.h"
#include "texstate.h"
#include "mtypes.h"
#include "varray.h"

#include "math/m_matrix.h"
#include "math/m_xform.h"
#endif


static int
generic_noop(void)
{
#ifdef DEBUG
   _mesa_problem(NULL, "undefined function dispatch");
#endif
   return 0;
}


/*
 * Set all pointers in the given dispatch table to point to a
 * generic no-op function.
 */
void
_mesa_init_no_op_table(struct _glapi_table *table, GLuint tableSize)
{
   GLuint i;
   void **dispatch = (void **) table;
   for (i = 0; i < tableSize; i++) {
      dispatch[i] = (void *) generic_noop;
   }
}



/*
 * Initialize the given dispatch table with pointers to Mesa's
 * immediate-mode commands.
 *
 * Pointers to begin/end object commands and a few others
 * are provided via the vtxfmt interface elsewhere.
 */
void
_mesa_init_exec_table(struct _glapi_table *exec, GLuint tableSize)
{
   /* first initialize all dispatch slots to no-op */
   _mesa_init_no_op_table(exec, tableSize);

   _mesa_loopback_init_api_table( exec, GL_TRUE );

   /* load the dispatch slots we understand */
   exec->Accum = _mesa_Accum;
   exec->AlphaFunc = _mesa_AlphaFunc;
   exec->Bitmap = _mesa_Bitmap;
   exec->BlendFunc = _mesa_BlendFunc;
   exec->CallList = _mesa_CallList;
   exec->CallLists = _mesa_CallLists;
   exec->Clear = _mesa_Clear;
   exec->ClearAccum = _mesa_ClearAccum;
   exec->ClearColor = _mesa_ClearColor;
   exec->ClearDepth = _mesa_ClearDepth;
   exec->ClearIndex = _mesa_ClearIndex;
   exec->ClearStencil = _mesa_ClearStencil;
   exec->ClipPlane = _mesa_ClipPlane;
   exec->ColorMask = _mesa_ColorMask;
   exec->ColorMaterial = _mesa_ColorMaterial;
   exec->CopyPixels = _mesa_CopyPixels;
   exec->CullFace = _mesa_CullFace;
   exec->DeleteLists = _mesa_DeleteLists;
   exec->DepthFunc = _mesa_DepthFunc;
   exec->DepthMask = _mesa_DepthMask;
   exec->DepthRange = _mesa_DepthRange;
   exec->Disable = _mesa_Disable;
   exec->DrawBuffer = _mesa_DrawBuffer;
   exec->DrawPixels = _mesa_DrawPixels;
   exec->Enable = _mesa_Enable;
   exec->EndList = _mesa_EndList;
   exec->FeedbackBuffer = _mesa_FeedbackBuffer;
   exec->Finish = _mesa_Finish;
   exec->Flush = _mesa_Flush;
   exec->FogCoordPointerEXT = _mesa_FogCoordPointerEXT;
   exec->Fogf = _mesa_Fogf;
   exec->Fogfv = _mesa_Fogfv;
   exec->Fogi = _mesa_Fogi;
   exec->Fogiv = _mesa_Fogiv;
   exec->FrontFace = _mesa_FrontFace;
   exec->Frustum = _mesa_Frustum;
   exec->GenLists = _mesa_GenLists;
   exec->GetBooleanv = _mesa_GetBooleanv;
   exec->GetClipPlane = _mesa_GetClipPlane;
   exec->GetDoublev = _mesa_GetDoublev;
   exec->GetError = _mesa_GetError;
   exec->GetFloatv = _mesa_GetFloatv;
   exec->GetIntegerv = _mesa_GetIntegerv;
   exec->GetLightfv = _mesa_GetLightfv;
   exec->GetLightiv = _mesa_GetLightiv;
   exec->GetMapdv = _mesa_GetMapdv;
   exec->GetMapfv = _mesa_GetMapfv;
   exec->GetMapiv = _mesa_GetMapiv;
   exec->GetMaterialfv = _mesa_GetMaterialfv;
   exec->GetMaterialiv = _mesa_GetMaterialiv;
   exec->GetPixelMapfv = _mesa_GetPixelMapfv;
   exec->GetPixelMapuiv = _mesa_GetPixelMapuiv;
   exec->GetPixelMapusv = _mesa_GetPixelMapusv;
   exec->GetPolygonStipple = _mesa_GetPolygonStipple;
   exec->GetString = _mesa_GetString;
   exec->GetTexEnvfv = _mesa_GetTexEnvfv;
   exec->GetTexEnviv = _mesa_GetTexEnviv;
   exec->GetTexGendv = _mesa_GetTexGendv;
   exec->GetTexGenfv = _mesa_GetTexGenfv;
   exec->GetTexGeniv = _mesa_GetTexGeniv;
   exec->GetTexImage = _mesa_GetTexImage;
   exec->GetTexLevelParameterfv = _mesa_GetTexLevelParameterfv;
   exec->GetTexLevelParameteriv = _mesa_GetTexLevelParameteriv;
   exec->GetTexParameterfv = _mesa_GetTexParameterfv;
   exec->GetTexParameteriv = _mesa_GetTexParameteriv;
   exec->Hint = _mesa_Hint;
   exec->IndexMask = _mesa_IndexMask;
   exec->InitNames = _mesa_InitNames;
   exec->IsEnabled = _mesa_IsEnabled;
   exec->IsList = _mesa_IsList;
   exec->LightModelf = _mesa_LightModelf;
   exec->LightModelfv = _mesa_LightModelfv;
   exec->LightModeli = _mesa_LightModeli;
   exec->LightModeliv = _mesa_LightModeliv;
   exec->Lightf = _mesa_Lightf;
   exec->Lightfv = _mesa_Lightfv;
   exec->Lighti = _mesa_Lighti;
   exec->Lightiv = _mesa_Lightiv;
   exec->LineStipple = _mesa_LineStipple;
   exec->LineWidth = _mesa_LineWidth;
   exec->ListBase = _mesa_ListBase;
   exec->LoadIdentity = _mesa_LoadIdentity;
   exec->LoadMatrixd = _mesa_LoadMatrixd;
   exec->LoadMatrixf = _mesa_LoadMatrixf;
   exec->LoadName = _mesa_LoadName;
   exec->LogicOp = _mesa_LogicOp;
   exec->Map1d = _mesa_Map1d;
   exec->Map1f = _mesa_Map1f;
   exec->Map2d = _mesa_Map2d;
   exec->Map2f = _mesa_Map2f;
   exec->MapGrid1d = _mesa_MapGrid1d;
   exec->MapGrid1f = _mesa_MapGrid1f;
   exec->MapGrid2d = _mesa_MapGrid2d;
   exec->MapGrid2f = _mesa_MapGrid2f;
   exec->MatrixMode = _mesa_MatrixMode;
   exec->MultMatrixd = _mesa_MultMatrixd;
   exec->MultMatrixf = _mesa_MultMatrixf;
   exec->NewList = _mesa_NewList;
   exec->Ortho = _mesa_Ortho;
   exec->PassThrough = _mesa_PassThrough;
   exec->PixelMapfv = _mesa_PixelMapfv;
   exec->PixelMapuiv = _mesa_PixelMapuiv;
   exec->PixelMapusv = _mesa_PixelMapusv;
   exec->PixelStoref = _mesa_PixelStoref;
   exec->PixelStorei = _mesa_PixelStorei;
   exec->PixelTransferf = _mesa_PixelTransferf;
   exec->PixelTransferi = _mesa_PixelTransferi;
   exec->PixelZoom = _mesa_PixelZoom;
   exec->PointSize = _mesa_PointSize;
   exec->PolygonMode = _mesa_PolygonMode;
   exec->PolygonOffset = _mesa_PolygonOffset;
   exec->PolygonStipple = _mesa_PolygonStipple;
   exec->PopAttrib = _mesa_PopAttrib;
   exec->PopMatrix = _mesa_PopMatrix;
   exec->PopName = _mesa_PopName;
   exec->PushAttrib = _mesa_PushAttrib;
   exec->PushMatrix = _mesa_PushMatrix;
   exec->PushName = _mesa_PushName;
   exec->RasterPos2d = _mesa_RasterPos2d;
   exec->RasterPos2dv = _mesa_RasterPos2dv;
   exec->RasterPos2f = _mesa_RasterPos2f;
   exec->RasterPos2fv = _mesa_RasterPos2fv;
   exec->RasterPos2i = _mesa_RasterPos2i;
   exec->RasterPos2iv = _mesa_RasterPos2iv;
   exec->RasterPos2s = _mesa_RasterPos2s;
   exec->RasterPos2sv = _mesa_RasterPos2sv;
   exec->RasterPos3d = _mesa_RasterPos3d;
   exec->RasterPos3dv = _mesa_RasterPos3dv;
   exec->RasterPos3f = _mesa_RasterPos3f;
   exec->RasterPos3fv = _mesa_RasterPos3fv;
   exec->RasterPos3i = _mesa_RasterPos3i;
   exec->RasterPos3iv = _mesa_RasterPos3iv;
   exec->RasterPos3s = _mesa_RasterPos3s;
   exec->RasterPos3sv = _mesa_RasterPos3sv;
   exec->RasterPos4d = _mesa_RasterPos4d;
   exec->RasterPos4dv = _mesa_RasterPos4dv;
   exec->RasterPos4f = _mesa_RasterPos4f;
   exec->RasterPos4fv = _mesa_RasterPos4fv;
   exec->RasterPos4i = _mesa_RasterPos4i;
   exec->RasterPos4iv = _mesa_RasterPos4iv;
   exec->RasterPos4s = _mesa_RasterPos4s;
   exec->RasterPos4sv = _mesa_RasterPos4sv;
   exec->ReadBuffer = _mesa_ReadBuffer;
   exec->ReadPixels = _mesa_ReadPixels;
   exec->RenderMode = _mesa_RenderMode;
   exec->Rotated = _mesa_Rotated;
   exec->Rotatef = _mesa_Rotatef;
   exec->Scaled = _mesa_Scaled;
   exec->Scalef = _mesa_Scalef;
   exec->Scissor = _mesa_Scissor;
   exec->SecondaryColorPointerEXT = _mesa_SecondaryColorPointerEXT;
   exec->SelectBuffer = _mesa_SelectBuffer;
   exec->ShadeModel = _mesa_ShadeModel;
   exec->StencilFunc = _mesa_StencilFunc;
   exec->StencilMask = _mesa_StencilMask;
   exec->StencilOp = _mesa_StencilOp;
   exec->TexEnvf = _mesa_TexEnvf;
   exec->TexEnvfv = _mesa_TexEnvfv;
   exec->TexEnvi = _mesa_TexEnvi;
   exec->TexEnviv = _mesa_TexEnviv;
   exec->TexGend = _mesa_TexGend;
   exec->TexGendv = _mesa_TexGendv;
   exec->TexGenf = _mesa_TexGenf;
   exec->TexGenfv = _mesa_TexGenfv;
   exec->TexGeni = _mesa_TexGeni;
   exec->TexGeniv = _mesa_TexGeniv;
   exec->TexImage1D = _mesa_TexImage1D;
   exec->TexImage2D = _mesa_TexImage2D;
   exec->TexParameterf = _mesa_TexParameterf;
   exec->TexParameterfv = _mesa_TexParameterfv;
   exec->TexParameteri = _mesa_TexParameteri;
   exec->TexParameteriv = _mesa_TexParameteriv;
   exec->Translated = _mesa_Translated;
   exec->Translatef = _mesa_Translatef;
   exec->Viewport = _mesa_Viewport;

   /* 1.1 */
   exec->AreTexturesResident = _mesa_AreTexturesResident;
   exec->BindTexture = _mesa_BindTexture;
   exec->ColorPointer = _mesa_ColorPointer;
   exec->CopyTexImage1D = _mesa_CopyTexImage1D;
   exec->CopyTexImage2D = _mesa_CopyTexImage2D;
   exec->CopyTexSubImage1D = _mesa_CopyTexSubImage1D;
   exec->CopyTexSubImage2D = _mesa_CopyTexSubImage2D;
   exec->DeleteTextures = _mesa_DeleteTextures;
   exec->DisableClientState = _mesa_DisableClientState;
   exec->EdgeFlagPointer = _mesa_EdgeFlagPointer;
   exec->EnableClientState = _mesa_EnableClientState;
   exec->GenTextures = _mesa_GenTextures;
   exec->GetPointerv = _mesa_GetPointerv;
   exec->IndexPointer = _mesa_IndexPointer;
   exec->InterleavedArrays = _mesa_InterleavedArrays;
   exec->IsTexture = _mesa_IsTexture;
   exec->NormalPointer = _mesa_NormalPointer;
   exec->PopClientAttrib = _mesa_PopClientAttrib;
   exec->PrioritizeTextures = _mesa_PrioritizeTextures;
   exec->PushClientAttrib = _mesa_PushClientAttrib;
   exec->TexCoordPointer = _mesa_TexCoordPointer;
   exec->TexSubImage1D = _mesa_TexSubImage1D;
   exec->TexSubImage2D = _mesa_TexSubImage2D;
   exec->VertexPointer = _mesa_VertexPointer;

   /* 1.2 */
   exec->CopyTexSubImage3D = _mesa_CopyTexSubImage3D;
   exec->TexImage3D = _mesa_TexImage3D;
   exec->TexSubImage3D = _mesa_TexSubImage3D;

   /* OpenGL 1.2  GL_ARB_imaging */
   exec->BlendColor = _mesa_BlendColor;
   exec->BlendEquation = _mesa_BlendEquation;
   exec->ColorSubTable = _mesa_ColorSubTable;
   exec->ColorTable = _mesa_ColorTable;
   exec->ColorTableParameterfv = _mesa_ColorTableParameterfv;
   exec->ColorTableParameteriv = _mesa_ColorTableParameteriv;
   exec->ConvolutionFilter1D = _mesa_ConvolutionFilter1D;
   exec->ConvolutionFilter2D = _mesa_ConvolutionFilter2D;
   exec->ConvolutionParameterf = _mesa_ConvolutionParameterf;
   exec->ConvolutionParameterfv = _mesa_ConvolutionParameterfv;
   exec->ConvolutionParameteri = _mesa_ConvolutionParameteri;
   exec->ConvolutionParameteriv = _mesa_ConvolutionParameteriv;
   exec->CopyColorSubTable = _mesa_CopyColorSubTable;
   exec->CopyColorTable = _mesa_CopyColorTable;
   exec->CopyConvolutionFilter1D = _mesa_CopyConvolutionFilter1D;
   exec->CopyConvolutionFilter2D = _mesa_CopyConvolutionFilter2D;
   exec->GetColorTable = _mesa_GetColorTable;
   exec->GetColorTableParameterfv = _mesa_GetColorTableParameterfv;
   exec->GetColorTableParameteriv = _mesa_GetColorTableParameteriv;
   exec->GetConvolutionFilter = _mesa_GetConvolutionFilter;
   exec->GetConvolutionParameterfv = _mesa_GetConvolutionParameterfv;
   exec->GetConvolutionParameteriv = _mesa_GetConvolutionParameteriv;
   exec->GetHistogram = _mesa_GetHistogram;
   exec->GetHistogramParameterfv = _mesa_GetHistogramParameterfv;
   exec->GetHistogramParameteriv = _mesa_GetHistogramParameteriv;
   exec->GetMinmax = _mesa_GetMinmax;
   exec->GetMinmaxParameterfv = _mesa_GetMinmaxParameterfv;
   exec->GetMinmaxParameteriv = _mesa_GetMinmaxParameteriv;
   exec->GetSeparableFilter = _mesa_GetSeparableFilter;
   exec->Histogram = _mesa_Histogram;
   exec->Minmax = _mesa_Minmax;
   exec->ResetHistogram = _mesa_ResetHistogram;
   exec->ResetMinmax = _mesa_ResetMinmax;
   exec->SeparableFilter2D = _mesa_SeparableFilter2D;

   /* 2. GL_EXT_blend_color */
#if 0
   exec->BlendColorEXT = _mesa_BlendColorEXT;
#endif

   /* 3. GL_EXT_polygon_offset */
   exec->PolygonOffsetEXT = _mesa_PolygonOffsetEXT;

   /* 6. GL_EXT_texture3d */
#if 0
   exec->CopyTexSubImage3DEXT = _mesa_CopyTexSubImage3D;
   exec->TexImage3DEXT = _mesa_TexImage3DEXT;
   exec->TexSubImage3DEXT = _mesa_TexSubImage3D;
#endif

   /* 11. GL_EXT_histogram */
   exec->GetHistogramEXT = _mesa_GetHistogram;
   exec->GetHistogramParameterfvEXT = _mesa_GetHistogramParameterfv;
   exec->GetHistogramParameterivEXT = _mesa_GetHistogramParameteriv;
   exec->GetMinmaxEXT = _mesa_GetMinmax;
   exec->GetMinmaxParameterfvEXT = _mesa_GetMinmaxParameterfv;
   exec->GetMinmaxParameterivEXT = _mesa_GetMinmaxParameteriv;

   /* ?. GL_SGIX_pixel_texture */
   exec->PixelTexGenSGIX = _mesa_PixelTexGenSGIX;

   /* 15. GL_SGIS_pixel_texture */
   exec->PixelTexGenParameteriSGIS = _mesa_PixelTexGenParameteriSGIS;
   exec->PixelTexGenParameterivSGIS = _mesa_PixelTexGenParameterivSGIS;
   exec->PixelTexGenParameterfSGIS = _mesa_PixelTexGenParameterfSGIS;
   exec->PixelTexGenParameterfvSGIS = _mesa_PixelTexGenParameterfvSGIS;
   exec->GetPixelTexGenParameterivSGIS = _mesa_GetPixelTexGenParameterivSGIS;
   exec->GetPixelTexGenParameterfvSGIS = _mesa_GetPixelTexGenParameterfvSGIS;

   /* 30. GL_EXT_vertex_array */
   exec->ColorPointerEXT = _mesa_ColorPointerEXT;
   exec->EdgeFlagPointerEXT = _mesa_EdgeFlagPointerEXT;
   exec->IndexPointerEXT = _mesa_IndexPointerEXT;
   exec->NormalPointerEXT = _mesa_NormalPointerEXT;
   exec->TexCoordPointerEXT = _mesa_TexCoordPointerEXT;
   exec->VertexPointerEXT = _mesa_VertexPointerEXT;

   /* 37. GL_EXT_blend_minmax */
#if 0
   exec->BlendEquationEXT = _mesa_BlendEquationEXT;
#endif

   /* 54. GL_EXT_point_parameters */
   exec->PointParameterfEXT = _mesa_PointParameterfEXT;
   exec->PointParameterfvEXT = _mesa_PointParameterfvEXT;

   /* 78. GL_EXT_paletted_texture */
#if 0
   exec->ColorTableEXT = _mesa_ColorTableEXT;
   exec->ColorSubTableEXT = _mesa_ColorSubTableEXT;
#endif
   exec->GetColorTableEXT = _mesa_GetColorTable;
   exec->GetColorTableParameterfvEXT = _mesa_GetColorTableParameterfv;
   exec->GetColorTableParameterivEXT = _mesa_GetColorTableParameteriv;

   /* 97. GL_EXT_compiled_vertex_array */
   exec->LockArraysEXT = _mesa_LockArraysEXT;
   exec->UnlockArraysEXT = _mesa_UnlockArraysEXT;

   /* 173. GL_INGR_blend_func_separate */
   exec->BlendFuncSeparateEXT = _mesa_BlendFuncSeparateEXT;

   /* 196. GL_MESA_resize_buffers */
   exec->ResizeBuffersMESA = _mesa_ResizeBuffersMESA;

   /* 197. GL_MESA_window_pos */
   exec->WindowPos2dMESA = _mesa_WindowPos2dMESA;
   exec->WindowPos2dvMESA = _mesa_WindowPos2dvMESA;
   exec->WindowPos2fMESA = _mesa_WindowPos2fMESA;
   exec->WindowPos2fvMESA = _mesa_WindowPos2fvMESA;
   exec->WindowPos2iMESA = _mesa_WindowPos2iMESA;
   exec->WindowPos2ivMESA = _mesa_WindowPos2ivMESA;
   exec->WindowPos2sMESA = _mesa_WindowPos2sMESA;
   exec->WindowPos2svMESA = _mesa_WindowPos2svMESA;
   exec->WindowPos3dMESA = _mesa_WindowPos3dMESA;
   exec->WindowPos3dvMESA = _mesa_WindowPos3dvMESA;
   exec->WindowPos3fMESA = _mesa_WindowPos3fMESA;
   exec->WindowPos3fvMESA = _mesa_WindowPos3fvMESA;
   exec->WindowPos3iMESA = _mesa_WindowPos3iMESA;
   exec->WindowPos3ivMESA = _mesa_WindowPos3ivMESA;
   exec->WindowPos3sMESA = _mesa_WindowPos3sMESA;
   exec->WindowPos3svMESA = _mesa_WindowPos3svMESA;
   exec->WindowPos4dMESA = _mesa_WindowPos4dMESA;
   exec->WindowPos4dvMESA = _mesa_WindowPos4dvMESA;
   exec->WindowPos4fMESA = _mesa_WindowPos4fMESA;
   exec->WindowPos4fvMESA = _mesa_WindowPos4fvMESA;
   exec->WindowPos4iMESA = _mesa_WindowPos4iMESA;
   exec->WindowPos4ivMESA = _mesa_WindowPos4ivMESA;
   exec->WindowPos4sMESA = _mesa_WindowPos4sMESA;
   exec->WindowPos4svMESA = _mesa_WindowPos4svMESA;

   /* ARB 1. GL_ARB_multitexture */
   exec->ActiveTextureARB = _mesa_ActiveTextureARB;
   exec->ClientActiveTextureARB = _mesa_ClientActiveTextureARB;

   /* ARB 3. GL_ARB_transpose_matrix */
   exec->LoadTransposeMatrixdARB = _mesa_LoadTransposeMatrixdARB;
   exec->LoadTransposeMatrixfARB = _mesa_LoadTransposeMatrixfARB;
   exec->MultTransposeMatrixdARB = _mesa_MultTransposeMatrixdARB;
   exec->MultTransposeMatrixfARB = _mesa_MultTransposeMatrixfARB;

   /* ARB 5. GL_ARB_multisample */
   exec->SampleCoverageARB = _mesa_SampleCoverageARB;

   /* ARB 12. GL_ARB_texture_compression */
   exec->CompressedTexImage3DARB = _mesa_CompressedTexImage3DARB;
   exec->CompressedTexImage2DARB = _mesa_CompressedTexImage2DARB;
   exec->CompressedTexImage1DARB = _mesa_CompressedTexImage1DARB;
   exec->CompressedTexSubImage3DARB = _mesa_CompressedTexSubImage3DARB;
   exec->CompressedTexSubImage2DARB = _mesa_CompressedTexSubImage2DARB;
   exec->CompressedTexSubImage1DARB = _mesa_CompressedTexSubImage1DARB;
   exec->GetCompressedTexImageARB = _mesa_GetCompressedTexImageARB;

}



/**********************************************************************/
/*****                   State update logic                       *****/
/**********************************************************************/


static void
update_polygon( GLcontext *ctx )
{
   ctx->_TriangleCaps &= ~(DD_TRI_CULL_FRONT_BACK | DD_TRI_OFFSET);

   if (ctx->Polygon.CullFlag && ctx->Polygon.CullFaceMode == GL_FRONT_AND_BACK)
      ctx->_TriangleCaps |= DD_TRI_CULL_FRONT_BACK;

   /* Any Polygon offsets enabled? */
   ctx->Polygon._OffsetAny = GL_FALSE;
   ctx->_TriangleCaps &= ~DD_TRI_OFFSET;

   if (ctx->Polygon.OffsetPoint ||
       ctx->Polygon.OffsetLine ||
       ctx->Polygon.OffsetFill) {
      ctx->_TriangleCaps |= DD_TRI_OFFSET;
      ctx->Polygon._OffsetAny = GL_TRUE;
   }
}

static void
calculate_model_project_matrix( GLcontext *ctx )
{
   if (!ctx->_NeedEyeCoords) {
      _math_matrix_mul_matrix( &ctx->_ModelProjectMatrix,
			       &ctx->ProjectionMatrix,
			       &ctx->ModelView );

      _math_matrix_analyse( &ctx->_ModelProjectMatrix );
   }
}

static void
update_modelview_scale( GLcontext *ctx )
{
   ctx->_ModelViewInvScale = 1.0F;
   if (ctx->ModelView.flags & (MAT_FLAG_UNIFORM_SCALE |
			       MAT_FLAG_GENERAL_SCALE |
			       MAT_FLAG_GENERAL_3D |
			       MAT_FLAG_GENERAL) ) {
      const GLfloat *m = ctx->ModelView.inv;
      GLfloat f = m[2] * m[2] + m[6] * m[6] + m[10] * m[10];
      if (f < 1e-12) f = 1.0;
      if (ctx->_NeedEyeCoords)
	 ctx->_ModelViewInvScale = 1.0/GL_SQRT(f);
      else
	 ctx->_ModelViewInvScale = GL_SQRT(f);
   }
}


/* Bring uptodate any state that relies on _NeedEyeCoords.
 */
static void
update_tnl_spaces( GLcontext *ctx, GLuint oldneedeyecoords )
{
   /* Check if the truth-value interpretations of the bitfields have
    * changed:
    */
   if ((oldneedeyecoords == 0) != (ctx->_NeedEyeCoords == 0)) {
      /* Recalculate all state that depends on _NeedEyeCoords.
       */
      update_modelview_scale(ctx);
      calculate_model_project_matrix(ctx);
      _mesa_compute_light_positions( ctx );

      if (ctx->Driver.LightingSpaceChange)
	 ctx->Driver.LightingSpaceChange( ctx );
   }
   else {
      GLuint new_state = ctx->NewState;

      /* Recalculate that same state only if it has been invalidated
       * by other statechanges.
       */
      if (new_state & _NEW_MODELVIEW)
	 update_modelview_scale(ctx);

      if (new_state & (_NEW_MODELVIEW|_NEW_PROJECTION))
	 calculate_model_project_matrix(ctx);

      if (new_state & (_NEW_LIGHT|_NEW_MODELVIEW))
	 _mesa_compute_light_positions( ctx );
   }
}


static void
update_drawbuffer( GLcontext *ctx )
{
   ctx->DrawBuffer->_Xmin = 0;
   ctx->DrawBuffer->_Ymin = 0;
   ctx->DrawBuffer->_Xmax = ctx->DrawBuffer->Width;
   ctx->DrawBuffer->_Ymax = ctx->DrawBuffer->Height;
   if (ctx->Scissor.Enabled) {
      if (ctx->Scissor.X > ctx->DrawBuffer->_Xmin) {
	 ctx->DrawBuffer->_Xmin = ctx->Scissor.X;
      }
      if (ctx->Scissor.Y > ctx->DrawBuffer->_Ymin) {
	 ctx->DrawBuffer->_Ymin = ctx->Scissor.Y;
      }
      if (ctx->Scissor.X + ctx->Scissor.Width < ctx->DrawBuffer->_Xmax) {
	 ctx->DrawBuffer->_Xmax = ctx->Scissor.X + ctx->Scissor.Width;
      }
      if (ctx->Scissor.Y + ctx->Scissor.Height < ctx->DrawBuffer->_Ymax) {
	 ctx->DrawBuffer->_Ymax = ctx->Scissor.Y + ctx->Scissor.Height;
      }
   }
}


/* NOTE: This routine references Tranform attribute values to compute
 * userclip positions in clip space, but is only called on
 * _NEW_PROJECTION.  The _mesa_ClipPlane() function keeps these values
 * uptodate across changes to the Transform attributes.
 */
static void
update_projection( GLcontext *ctx )
{
   _math_matrix_analyse( &ctx->ProjectionMatrix );

   /* Recompute clip plane positions in clipspace.  This is also done
    * in _mesa_ClipPlane().
    */
   if (ctx->Transform._AnyClip) {
      GLuint p;
      for (p = 0; p < ctx->Const.MaxClipPlanes; p++) {
	 if (ctx->Transform.ClipEnabled[p]) {
	    _mesa_transform_vector( ctx->Transform._ClipUserPlane[p],
				 ctx->Transform.EyeUserPlane[p],
				 ctx->ProjectionMatrix.inv );
	 }
      }
   }
}


/*
 * Return a bitmask of IMAGE_*_BIT flags which to indicate which
 * pixel transfer operations are enabled.
 */
static void
update_image_transfer_state(GLcontext *ctx)
{
   GLuint mask = 0;

   if (ctx->Pixel.RedScale   != 1.0F || ctx->Pixel.RedBias   != 0.0F ||
       ctx->Pixel.GreenScale != 1.0F || ctx->Pixel.GreenBias != 0.0F ||
       ctx->Pixel.BlueScale  != 1.0F || ctx->Pixel.BlueBias  != 0.0F ||
       ctx->Pixel.AlphaScale != 1.0F || ctx->Pixel.AlphaBias != 0.0F)
      mask |= IMAGE_SCALE_BIAS_BIT;

   if (ctx->Pixel.IndexShift || ctx->Pixel.IndexOffset)
      mask |= IMAGE_SHIFT_OFFSET_BIT;

   if (ctx->Pixel.MapColorFlag)
      mask |= IMAGE_MAP_COLOR_BIT;

   if (ctx->Pixel.ColorTableEnabled)
      mask |= IMAGE_COLOR_TABLE_BIT;

   if (ctx->Pixel.Convolution1DEnabled ||
       ctx->Pixel.Convolution2DEnabled ||
       ctx->Pixel.Separable2DEnabled) {
      mask |= IMAGE_CONVOLUTION_BIT;
      if (ctx->Pixel.PostConvolutionScale[0] != 1.0F ||
          ctx->Pixel.PostConvolutionScale[1] != 1.0F ||
          ctx->Pixel.PostConvolutionScale[2] != 1.0F ||
          ctx->Pixel.PostConvolutionScale[3] != 1.0F ||
          ctx->Pixel.PostConvolutionBias[0] != 0.0F ||
          ctx->Pixel.PostConvolutionBias[1] != 0.0F ||
          ctx->Pixel.PostConvolutionBias[2] != 0.0F ||
          ctx->Pixel.PostConvolutionBias[3] != 0.0F) {
         mask |= IMAGE_POST_CONVOLUTION_SCALE_BIAS;
      }
   }

   if (ctx->Pixel.PostConvolutionColorTableEnabled)
      mask |= IMAGE_POST_CONVOLUTION_COLOR_TABLE_BIT;

   if (ctx->ColorMatrix.type != MATRIX_IDENTITY ||
       ctx->Pixel.PostColorMatrixScale[0] != 1.0F ||
       ctx->Pixel.PostColorMatrixBias[0]  != 0.0F ||
       ctx->Pixel.PostColorMatrixScale[1] != 1.0F ||
       ctx->Pixel.PostColorMatrixBias[1]  != 0.0F ||
       ctx->Pixel.PostColorMatrixScale[2] != 1.0F ||
       ctx->Pixel.PostColorMatrixBias[2]  != 0.0F ||
       ctx->Pixel.PostColorMatrixScale[3] != 1.0F ||
       ctx->Pixel.PostColorMatrixBias[3]  != 0.0F)
      mask |= IMAGE_COLOR_MATRIX_BIT;

   if (ctx->Pixel.PostColorMatrixColorTableEnabled)
      mask |= IMAGE_POST_COLOR_MATRIX_COLOR_TABLE_BIT;

   if (ctx->Pixel.HistogramEnabled)
      mask |= IMAGE_HISTOGRAM_BIT;

   if (ctx->Pixel.MinMaxEnabled)
      mask |= IMAGE_MIN_MAX_BIT;

   ctx->_ImageTransferState = mask;
}




/* Note: This routine refers to derived texture attribute values to
 * compute the ENABLE_TEXMAT flags, but is only called on
 * _NEW_TEXTURE_MATRIX.  On changes to _NEW_TEXTURE, the ENABLE_TEXMAT
 * flags are updated by _mesa_update_textures(), below.
 *
 * If both TEXTURE and TEXTURE_MATRIX change at once, these values
 * will be computed twice.
 */
static void
update_texture_matrices( GLcontext *ctx )
{
   GLuint i;

   ctx->Texture._TexMatEnabled = 0;

   for (i=0; i < ctx->Const.MaxTextureUnits; i++) {
      if (ctx->TextureMatrix[i].flags & MAT_DIRTY) {
	 _math_matrix_analyse( &ctx->TextureMatrix[i] );

	 if (ctx->Driver.TextureMatrix)
	    ctx->Driver.TextureMatrix( ctx, i, &ctx->TextureMatrix[i] );

	 if (ctx->Texture.Unit[i]._ReallyEnabled &&
	     ctx->TextureMatrix[i].type != MATRIX_IDENTITY)
	    ctx->Texture._TexMatEnabled |= ENABLE_TEXMAT(i);
      }
   }
}


/* Note: This routine refers to derived texture matrix values to
 * compute the ENABLE_TEXMAT flags, but is only called on
 * _NEW_TEXTURE.  On changes to _NEW_TEXTURE_MATRIX, the ENABLE_TEXMAT
 * flags are updated by _mesa_update_texture_matrices, above.
 *
 * If both TEXTURE and TEXTURE_MATRIX change at once, these values
 * will be computed twice.
 */
static void
update_texture_state( GLcontext *ctx )
{
   GLuint i;

   ctx->Texture._ReallyEnabled = 0;
   ctx->Texture._GenFlags = 0;
   ctx->_NeedNormals &= ~NEED_NORMALS_TEXGEN;
   ctx->_NeedEyeCoords &= ~NEED_EYE_TEXGEN;
   ctx->Texture._TexMatEnabled = 0;
   ctx->Texture._TexGenEnabled = 0;

   /* Update texture unit state.
    */
   for (i=0; i < ctx->Const.MaxTextureUnits; i++) {
      struct gl_texture_unit *texUnit = &ctx->Texture.Unit[i];

      texUnit->_ReallyEnabled = 0;
      texUnit->_GenFlags = 0;

      if (!texUnit->Enabled)
	 continue;

      /* Find the texture of highest dimensionality that is enabled
       * and complete.  We'll use it for texturing.
       */
      if (texUnit->Enabled & TEXTURE0_CUBE) {
         struct gl_texture_object *texObj = texUnit->CurrentCubeMap;
         if (!texObj->Complete) {
            _mesa_test_texobj_completeness(ctx, texObj);
         }
         if (texObj->Complete) {
            texUnit->_ReallyEnabled = TEXTURE0_CUBE;
            texUnit->_Current = texObj;
         }
      }

      if (!texUnit->_ReallyEnabled && (texUnit->Enabled & TEXTURE0_3D)) {
         struct gl_texture_object *texObj = texUnit->Current3D;
         if (!texObj->Complete) {
            _mesa_test_texobj_completeness(ctx, texObj);
         }
         if (texObj->Complete) {
            texUnit->_ReallyEnabled = TEXTURE0_3D;
            texUnit->_Current = texObj;
         }
      }

      if (!texUnit->_ReallyEnabled && (texUnit->Enabled & TEXTURE0_2D)) {
         struct gl_texture_object *texObj = texUnit->Current2D;
         if (!texObj->Complete) {
            _mesa_test_texobj_completeness(ctx, texObj);
         }
         if (texObj->Complete) {
            texUnit->_ReallyEnabled = TEXTURE0_2D;
            texUnit->_Current = texObj;
         }
      }

      if (!texUnit->_ReallyEnabled && (texUnit->Enabled & TEXTURE0_1D)) {
         struct gl_texture_object *texObj = texUnit->Current1D;
         if (!texObj->Complete) {
            _mesa_test_texobj_completeness(ctx, texObj);
         }
         if (texObj->Complete) {
            texUnit->_ReallyEnabled = TEXTURE0_1D;
            texUnit->_Current = texObj;
         }
      }

      if (!texUnit->_ReallyEnabled) {
	 texUnit->_Current = NULL;
	 continue;
      }

      {
	 GLuint flag = texUnit->_ReallyEnabled << (i * 4);
	 ctx->Texture._ReallyEnabled |= flag;
      }

      if (texUnit->TexGenEnabled) {
	 if (texUnit->TexGenEnabled & S_BIT) {
	    texUnit->_GenFlags |= texUnit->_GenBitS;
	 }
	 if (texUnit->TexGenEnabled & T_BIT) {
	    texUnit->_GenFlags |= texUnit->_GenBitT;
	 }
	 if (texUnit->TexGenEnabled & Q_BIT) {
	    texUnit->_GenFlags |= texUnit->_GenBitQ;
	 }
	 if (texUnit->TexGenEnabled & R_BIT) {
	    texUnit->_GenFlags |= texUnit->_GenBitR;
	 }

	 ctx->Texture._TexGenEnabled |= ENABLE_TEXGEN(i);
	 ctx->Texture._GenFlags |= texUnit->_GenFlags;
      }

      if (ctx->TextureMatrix[i].type != MATRIX_IDENTITY)
	 ctx->Texture._TexMatEnabled |= ENABLE_TEXMAT(i);
   }

   if (ctx->Texture._GenFlags & TEXGEN_NEED_NORMALS) {
      ctx->_NeedNormals |= NEED_NORMALS_TEXGEN;
      ctx->_NeedEyeCoords |= NEED_EYE_TEXGEN;
   }

   if (ctx->Texture._GenFlags & TEXGEN_NEED_EYE_COORD) {
      ctx->_NeedEyeCoords |= NEED_EYE_TEXGEN;
   }
}


/*
 * If ctx->NewState is non-zero then this function MUST be called before
 * rendering any primitive.  Basically, function pointers and miscellaneous
 * flags are updated to reflect the current state of the state machine.
 *
 * The above constraint is now maintained largely by the two Exec
 * dispatch tables, which trigger the appropriate flush on transition
 * between State and Geometry modes.
 *
 * Special care is taken with the derived value _NeedEyeCoords.  This
 * is a bitflag which is updated with information from a number of
 * attribute groups (MODELVIEW, LIGHT, TEXTURE).  A lot of derived
 * state references this value, and must be treated with care to
 * ensure that updates are done correctly.  All state dependent on
 * _NeedEyeCoords is calculated from within _mesa_update_tnl_spaces(),
 * and from nowhere else.
 */
void _mesa_update_state( GLcontext *ctx )
{
   const GLuint new_state = ctx->NewState;
   const GLuint oldneedeyecoords = ctx->_NeedEyeCoords;

   if (MESA_VERBOSE & VERBOSE_STATE)
      _mesa_print_state("", new_state);

   if (new_state & _NEW_MODELVIEW)
      _math_matrix_analyse( &ctx->ModelView );

   if (new_state & _NEW_PROJECTION)
      update_projection( ctx );

   if (new_state & _NEW_TEXTURE_MATRIX)
      update_texture_matrices( ctx );

   if (new_state & _NEW_COLOR_MATRIX)
      _math_matrix_analyse( &ctx->ColorMatrix );

   /* References ColorMatrix.type (derived above).
    */
   if (new_state & _IMAGE_NEW_TRANSFER_STATE)
      update_image_transfer_state(ctx);

   /* Contributes to NeedEyeCoords, NeedNormals.
    */
   if (new_state & _NEW_TEXTURE)
      update_texture_state( ctx );

   if (new_state & (_NEW_BUFFERS|_NEW_SCISSOR))
      update_drawbuffer( ctx );

   if (new_state & _NEW_POLYGON)
      update_polygon( ctx );

   /* Contributes to NeedEyeCoords, NeedNormals.
    */
   if (new_state & _NEW_LIGHT)
      _mesa_update_lighting( ctx );

   /* We can light in object space if the modelview matrix preserves
    * lengths and relative angles.
    */
   if (new_state & (_NEW_MODELVIEW|_NEW_LIGHT)) {
      ctx->_NeedEyeCoords &= ~NEED_EYE_LIGHT_MODELVIEW;
      if (ctx->Light.Enabled &&
	  !TEST_MAT_FLAGS( &ctx->ModelView, MAT_FLAGS_LENGTH_PRESERVING))
	    ctx->_NeedEyeCoords |= NEED_EYE_LIGHT_MODELVIEW;
   }


   /* ctx->_NeedEyeCoords is now uptodate.
    *
    * If the truth value of this variable has changed, update for the
    * new lighting space and recompute the positions of lights and the
    * normal transform.
    *
    * If the lighting space hasn't changed, may still need to recompute
    * light positions & normal transforms for other reasons.
    */
   if (new_state & (_NEW_MODELVIEW |
		    _NEW_PROJECTION |
		    _NEW_LIGHT |
		    _MESA_NEW_NEED_EYE_COORDS))
      update_tnl_spaces( ctx, oldneedeyecoords );

   /*
    * Here the driver sets up all the ctx->Driver function pointers
    * to it's specific, private functions, and performs any
    * internal state management necessary, including invalidating
    * state of active modules.
    *
    * Set ctx->NewState to zero to avoid recursion if
    * Driver.UpdateState() has to call FLUSH_VERTICES().  (fixed?)
    */
   ctx->NewState = 0;
   ctx->Driver.UpdateState(ctx, new_state);
   ctx->Array.NewState = 0;

   /* At this point we can do some assertions to be sure the required
    * device driver function pointers are all initialized.
    */
   ASSERT(ctx->Driver.GetString);
   ASSERT(ctx->Driver.UpdateState);
   ASSERT(ctx->Driver.Clear);
   ASSERT(ctx->Driver.SetDrawBuffer);
   ASSERT(ctx->Driver.GetBufferSize);
   if (ctx->Visual.accumRedBits > 0) {
      ASSERT(ctx->Driver.Accum);
   }
   ASSERT(ctx->Driver.DrawPixels);
   ASSERT(ctx->Driver.ReadPixels);
   ASSERT(ctx->Driver.CopyPixels);
   ASSERT(ctx->Driver.Bitmap);
   ASSERT(ctx->Driver.ResizeBuffersMESA);
   ASSERT(ctx->Driver.TexImage1D);
   ASSERT(ctx->Driver.TexImage2D);
   ASSERT(ctx->Driver.TexImage3D);
   ASSERT(ctx->Driver.TexSubImage1D);
   ASSERT(ctx->Driver.TexSubImage2D);
   ASSERT(ctx->Driver.TexSubImage3D);
   ASSERT(ctx->Driver.CopyTexImage1D);
   ASSERT(ctx->Driver.CopyTexImage2D);
   ASSERT(ctx->Driver.CopyTexSubImage1D);
   ASSERT(ctx->Driver.CopyTexSubImage2D);
   ASSERT(ctx->Driver.CopyTexSubImage3D);
   if (ctx->Extensions.ARB_texture_compression) {
      ASSERT(ctx->Driver.BaseCompressedTexFormat);
      ASSERT(ctx->Driver.CompressedTextureSize);
      ASSERT(ctx->Driver.GetCompressedTexImage);
#if 0  /* HW drivers need these, but not SW rasterizers */
      ASSERT(ctx->Driver.CompressedTexImage1D);
      ASSERT(ctx->Driver.CompressedTexImage2D);
      ASSERT(ctx->Driver.CompressedTexImage3D);
      ASSERT(ctx->Driver.CompressedTexSubImage1D);
      ASSERT(ctx->Driver.CompressedTexSubImage2D);
      ASSERT(ctx->Driver.CompressedTexSubImage3D);
#endif
   }
}
