/* $Id: dispatch.c,v 1.13 2000/02/01 23:57:03 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 *
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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


#ifdef PC_HEADER
#include "all.h"
#else
#include "glheader.h"
#include "accum.h"
#include "alpha.h"
#include "attrib.h"
#include "bitmap.h"
#include "blend.h"
#include "clip.h"
#include "context.h"
#include "colortab.h"
#include "copypix.h"
#include "cva.h"
#include "depth.h"
#include "dispatch.h"
#include "dlist.h"
#include "drawpix.h"
#include "enable.h"
#include "eval.h"
#include "feedback.h"
#include "fog.h"
#include "get.h"
#include "glapi.h"
#include "glmisc.h"
#include "imaging.h"
#include "light.h"
#include "lines.h"
#include "logic.h"
#include "masking.h"
#include "matrix.h"
#include "pixel.h"
#include "points.h"
#include "polygon.h"
#include "rastpos.h"
#include "readpix.h"
#include "rect.h"
#include "scissor.h"
#include "stencil.h"
#include "teximage.h"
#include "texobj.h"
#include "texstate.h"
#include "types.h"
#include "varray.h"
#include "vbfill.h"
#include "winpos.h"
#endif




/**********************************************************************
 * Generate the GL entrypoint functions here.
 */

#if defined(GLX_DIRECT_RENDERING) && !defined(XFree86Server)

/* We're building a DRI driver.
 * GL entrypoints will be in libGL.so, not in this rendering core.
 */

#else

#define KEYWORD1
#define KEYWORD2 GLAPIENTRY
#if defined(USE_X86_ASM) && !defined(__WIN32__) && !defined(XF86DRI)
#define NAME(func) _glapi_fallback_##func
#elif defined(USE_MGL_NAMESPACE)
#define NAME(func)  mgl##func
#else
#define NAME(func)  gl##func
#endif

#ifdef DEBUG

static int
trace(void)
{
   static int trace = -1;
   if (trace < 0)
      trace = getenv("MESA_TRACE") ? 1 : 0;
   return trace > 0;
}

#define F stderr

#define DISPATCH(FUNC, ARGS, MESSAGE)					\
   const struct _glapi_table *dispatch;					\
   dispatch = _glapi_Dispatch ? _glapi_Dispatch : _glapi_get_dispatch();\
   if (trace()) {							\
      fprintf MESSAGE;							\
      fprintf(F, "\n");							\
   }									\
   (dispatch->FUNC) ARGS

#define RETURN_DISPATCH(FUNC, ARGS, MESSAGE) 				\
   const struct _glapi_table *dispatch;					\
   dispatch = _glapi_Dispatch ? _glapi_Dispatch : _glapi_get_dispatch();\
   if (trace()) {							\
      fprintf MESSAGE;							\
      fprintf(F, "\n");							\
   }									\
   return (dispatch->FUNC) ARGS

#else

#define DISPATCH(FUNC, ARGS, MESSAGE)					\
   const struct _glapi_table *dispatch;					\
   dispatch = _glapi_Dispatch ? _glapi_Dispatch : _glapi_get_dispatch();\
   (dispatch->FUNC) ARGS

#define RETURN_DISPATCH(FUNC, ARGS, MESSAGE)				\
   const struct _glapi_table *dispatch;					\
   dispatch = _glapi_Dispatch ? _glapi_Dispatch : _glapi_get_dispatch();\
   return (dispatch->FUNC) ARGS

#endif


#ifndef GLAPIENTRY
#define GLAPIENTRY
#endif

#include "glapitemp.h"


#endif /*GLX_DIRECT_RENDERING*/


/**********************************************************************/


static int
generic_noop(void)
{
#ifdef DEBUG
   gl_problem(NULL, "undefined function dispatch");
#endif
   return 0;
}


void
_mesa_init_no_op_table(struct _glapi_table *table)
{
   /* Check to be sure the dispatcher's table is at least as big as Mesa's. */
   const GLuint size = sizeof(struct _glapi_table) / sizeof(void *);
   assert(_glapi_get_dispatch_table_size() >= size);

   {
      const GLuint n = _glapi_get_dispatch_table_size();
      GLuint i;
      void **dispatch = (void **) table;
      for (i = 0; i < n; i++) {
         dispatch[i] = (void *) generic_noop;
      }
   }
}


/*
 * Initialize the given dispatch table with pointers to Mesa's
 * immediate-mode commands.
 */
void
_mesa_init_exec_table(struct _glapi_table *exec)
{
   /* first initialize all dispatch slots to no-op */
   _mesa_init_no_op_table(exec);

   /* load the dispatch slots we understand */
   exec->Accum = _mesa_Accum;
   exec->AlphaFunc = _mesa_AlphaFunc;
   exec->Begin = _mesa_Begin;
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
   exec->Color3b = _mesa_Color3b;
   exec->Color3bv = _mesa_Color3bv;
   exec->Color3d = _mesa_Color3d;
   exec->Color3dv = _mesa_Color3dv;
   exec->Color3f = _mesa_Color3f;
   exec->Color3fv = _mesa_Color3fv;
   exec->Color3i = _mesa_Color3i;
   exec->Color3iv = _mesa_Color3iv;
   exec->Color3s = _mesa_Color3s;
   exec->Color3sv = _mesa_Color3sv;
   exec->Color3ub = _mesa_Color3ub;
   exec->Color3ubv = _mesa_Color3ubv;
   exec->Color3ui = _mesa_Color3ui;
   exec->Color3uiv = _mesa_Color3uiv;
   exec->Color3us = _mesa_Color3us;
   exec->Color3usv = _mesa_Color3usv;
   exec->Color4b = _mesa_Color4b;
   exec->Color4bv = _mesa_Color4bv;
   exec->Color4d = _mesa_Color4d;
   exec->Color4dv = _mesa_Color4dv;
   exec->Color4f = _mesa_Color4f;
   exec->Color4fv = _mesa_Color4fv;
   exec->Color4i = _mesa_Color4i;
   exec->Color4iv = _mesa_Color4iv;
   exec->Color4s = _mesa_Color4s;
   exec->Color4sv = _mesa_Color4sv;
   exec->Color4ub = _mesa_Color4ub;
   exec->Color4ubv = _mesa_Color4ubv;
   exec->Color4ui = _mesa_Color4ui;
   exec->Color4uiv = _mesa_Color4uiv;
   exec->Color4us = _mesa_Color4us;
   exec->Color4usv = _mesa_Color4usv;
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
   exec->EdgeFlag = _mesa_EdgeFlag;
   exec->EdgeFlagv = _mesa_EdgeFlagv;
   exec->Enable = _mesa_Enable;
   exec->End = _mesa_End;
   exec->EndList = _mesa_EndList;
   exec->EvalCoord1d = _mesa_EvalCoord1d;
   exec->EvalCoord1dv = _mesa_EvalCoord1dv;
   exec->EvalCoord1f = _mesa_EvalCoord1f;
   exec->EvalCoord1fv = _mesa_EvalCoord1fv;
   exec->EvalCoord2d = _mesa_EvalCoord2d;
   exec->EvalCoord2dv = _mesa_EvalCoord2dv;
   exec->EvalCoord2f = _mesa_EvalCoord2f;
   exec->EvalCoord2fv = _mesa_EvalCoord2fv;
   exec->EvalMesh1 = _mesa_EvalMesh1;
   exec->EvalMesh2 = _mesa_EvalMesh2;
   exec->EvalPoint1 = _mesa_EvalPoint1;
   exec->EvalPoint2 = _mesa_EvalPoint2;
   exec->FeedbackBuffer = _mesa_FeedbackBuffer;
   exec->Finish = _mesa_Finish;
   exec->Flush = _mesa_Flush;
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
   exec->Indexd = _mesa_Indexd;
   exec->Indexdv = _mesa_Indexdv;
   exec->Indexf = _mesa_Indexf;
   exec->Indexfv = _mesa_Indexfv;
   exec->Indexi = _mesa_Indexi;
   exec->Indexiv = _mesa_Indexiv;
   exec->Indexs = _mesa_Indexs;
   exec->Indexsv = _mesa_Indexsv;
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
   exec->Materialf = _mesa_Materialf;
   exec->Materialfv = _mesa_Materialfv;
   exec->Materiali = _mesa_Materiali;
   exec->Materialiv = _mesa_Materialiv;
   exec->MatrixMode = _mesa_MatrixMode;
   exec->MultMatrixd = _mesa_MultMatrixd;
   exec->MultMatrixf = _mesa_MultMatrixf;
   exec->NewList = _mesa_NewList;
   exec->Normal3b = _mesa_Normal3b;
   exec->Normal3bv = _mesa_Normal3bv;
   exec->Normal3d = _mesa_Normal3d;
   exec->Normal3dv = _mesa_Normal3dv;
   exec->Normal3f = _mesa_Normal3f;
   exec->Normal3fv = _mesa_Normal3fv;
   exec->Normal3i = _mesa_Normal3i;
   exec->Normal3iv = _mesa_Normal3iv;
   exec->Normal3s = _mesa_Normal3s;
   exec->Normal3sv = _mesa_Normal3sv;
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
   exec->Rectd = _mesa_Rectd;
   exec->Rectdv = _mesa_Rectdv;
   exec->Rectf = _mesa_Rectf;
   exec->Rectfv = _mesa_Rectfv;
   exec->Recti = _mesa_Recti;
   exec->Rectiv = _mesa_Rectiv;
   exec->Rects = _mesa_Rects;
   exec->Rectsv = _mesa_Rectsv;
   exec->RenderMode = _mesa_RenderMode;
   exec->Rotated = _mesa_Rotated;
   exec->Rotatef = _mesa_Rotatef;
   exec->Scaled = _mesa_Scaled;
   exec->Scalef = _mesa_Scalef;
   exec->Scissor = _mesa_Scissor;
   exec->SelectBuffer = _mesa_SelectBuffer;
   exec->ShadeModel = _mesa_ShadeModel;
   exec->StencilFunc = _mesa_StencilFunc;
   exec->StencilMask = _mesa_StencilMask;
   exec->StencilOp = _mesa_StencilOp;
   exec->TexCoord1d = _mesa_TexCoord1d;
   exec->TexCoord1dv = _mesa_TexCoord1dv;
   exec->TexCoord1f = _mesa_TexCoord1f;
   exec->TexCoord1fv = _mesa_TexCoord1fv;
   exec->TexCoord1i = _mesa_TexCoord1i;
   exec->TexCoord1iv = _mesa_TexCoord1iv;
   exec->TexCoord1s = _mesa_TexCoord1s;
   exec->TexCoord1sv = _mesa_TexCoord1sv;
   exec->TexCoord2d = _mesa_TexCoord2d;
   exec->TexCoord2dv = _mesa_TexCoord2dv;
   exec->TexCoord2f = _mesa_TexCoord2f;
   exec->TexCoord2fv = _mesa_TexCoord2fv;
   exec->TexCoord2i = _mesa_TexCoord2i;
   exec->TexCoord2iv = _mesa_TexCoord2iv;
   exec->TexCoord2s = _mesa_TexCoord2s;
   exec->TexCoord2sv = _mesa_TexCoord2sv;
   exec->TexCoord3d = _mesa_TexCoord3d;
   exec->TexCoord3dv = _mesa_TexCoord3dv;
   exec->TexCoord3f = _mesa_TexCoord3f;
   exec->TexCoord3fv = _mesa_TexCoord3fv;
   exec->TexCoord3i = _mesa_TexCoord3i;
   exec->TexCoord3iv = _mesa_TexCoord3iv;
   exec->TexCoord3s = _mesa_TexCoord3s;
   exec->TexCoord3sv = _mesa_TexCoord3sv;
   exec->TexCoord4d = _mesa_TexCoord4d;
   exec->TexCoord4dv = _mesa_TexCoord4dv;
   exec->TexCoord4f = _mesa_TexCoord4f;
   exec->TexCoord4fv = _mesa_TexCoord4fv;
   exec->TexCoord4i = _mesa_TexCoord4i;
   exec->TexCoord4iv = _mesa_TexCoord4iv;
   exec->TexCoord4s = _mesa_TexCoord4s;
   exec->TexCoord4sv = _mesa_TexCoord4sv;
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
   exec->Vertex2d = _mesa_Vertex2d;
   exec->Vertex2dv = _mesa_Vertex2dv;
   exec->Vertex2f = _mesa_Vertex2f;
   exec->Vertex2fv = _mesa_Vertex2fv;
   exec->Vertex2i = _mesa_Vertex2i;
   exec->Vertex2iv = _mesa_Vertex2iv;
   exec->Vertex2s = _mesa_Vertex2s;
   exec->Vertex2sv = _mesa_Vertex2sv;
   exec->Vertex3d = _mesa_Vertex3d;
   exec->Vertex3dv = _mesa_Vertex3dv;
   exec->Vertex3f = _mesa_Vertex3f;
   exec->Vertex3fv = _mesa_Vertex3fv;
   exec->Vertex3i = _mesa_Vertex3i;
   exec->Vertex3iv = _mesa_Vertex3iv;
   exec->Vertex3s = _mesa_Vertex3s;
   exec->Vertex3sv = _mesa_Vertex3sv;
   exec->Vertex4d = _mesa_Vertex4d;
   exec->Vertex4dv = _mesa_Vertex4dv;
   exec->Vertex4f = _mesa_Vertex4f;
   exec->Vertex4fv = _mesa_Vertex4fv;
   exec->Vertex4i = _mesa_Vertex4i;
   exec->Vertex4iv = _mesa_Vertex4iv;
   exec->Vertex4s = _mesa_Vertex4s;
   exec->Vertex4sv = _mesa_Vertex4sv;
   exec->Viewport = _mesa_Viewport;

   /* 1.1 */
   exec->AreTexturesResident = _mesa_AreTexturesResident;
   exec->ArrayElement = _mesa_ArrayElement;
   exec->BindTexture = _mesa_BindTexture;
   exec->ColorPointer = _mesa_ColorPointer;
   exec->CopyTexImage1D = _mesa_CopyTexImage1D;
   exec->CopyTexImage2D = _mesa_CopyTexImage2D;
   exec->CopyTexSubImage1D = _mesa_CopyTexSubImage1D;
   exec->CopyTexSubImage2D = _mesa_CopyTexSubImage2D;
   exec->DeleteTextures = _mesa_DeleteTextures;
   exec->DisableClientState = _mesa_DisableClientState;
   exec->DrawArrays = _mesa_DrawArrays;
   exec->DrawElements = _mesa_DrawElements;
   exec->EdgeFlagPointer = _mesa_EdgeFlagPointer;
   exec->EnableClientState = _mesa_EnableClientState;
   exec->GenTextures = _mesa_GenTextures;
   exec->GetPointerv = _mesa_GetPointerv;
   exec->IndexPointer = _mesa_IndexPointer;
   exec->Indexub = _mesa_Indexub;
   exec->Indexubv = _mesa_Indexubv;
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
   exec->DrawRangeElements = _mesa_DrawRangeElements;
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

   /* 6. GL_EXT_texture3d */
   exec->CopyTexSubImage3DEXT = _mesa_CopyTexSubImage3D;
   exec->TexImage3DEXT = _mesa_TexImage3DEXT;
   exec->TexSubImage3DEXT = _mesa_TexSubImage3D;

   /* GL_EXT_paletted_texture */
   exec->ColorTableEXT = _mesa_ColorTableEXT;
   exec->ColorSubTableEXT = _mesa_ColorSubTableEXT;
   exec->GetColorTableEXT = _mesa_GetColorTableEXT;
   exec->GetColorTableParameterfvEXT = _mesa_GetColorTableParameterfvEXT;
   exec->GetColorTableParameterivEXT = _mesa_GetColorTableParameterivEXT;

   /* GL_EXT_compiled_vertex_array */
   exec->LockArraysEXT = _mesa_LockArraysEXT;
   exec->UnlockArraysEXT = _mesa_UnlockArraysEXT;

   /* GL_EXT_point_parameters */
   exec->PointParameterfEXT = _mesa_PointParameterfEXT;
   exec->PointParameterfvEXT = _mesa_PointParameterfvEXT;

   /* 77. GL_PGI_misc_hints */
   exec->HintPGI = _mesa_HintPGI;

   /* GL_EXT_polygon_offset */
   exec->PolygonOffsetEXT = _mesa_PolygonOffsetEXT;

   /* GL_EXT_blend_minmax */
   exec->BlendEquationEXT = _mesa_BlendEquationEXT;

   /* GL_EXT_blend_color */
   exec->BlendColorEXT = _mesa_BlendColorEXT;

   /* GL_ARB_multitexture */
   exec->ActiveTextureARB = _mesa_ActiveTextureARB;
   exec->ClientActiveTextureARB = _mesa_ClientActiveTextureARB;
   exec->MultiTexCoord1dARB = _mesa_MultiTexCoord1dARB;
   exec->MultiTexCoord1dvARB = _mesa_MultiTexCoord1dvARB;
   exec->MultiTexCoord1fARB = _mesa_MultiTexCoord1fARB;
   exec->MultiTexCoord1fvARB = _mesa_MultiTexCoord1fvARB;
   exec->MultiTexCoord1iARB = _mesa_MultiTexCoord1iARB;
   exec->MultiTexCoord1ivARB = _mesa_MultiTexCoord1ivARB;
   exec->MultiTexCoord1sARB = _mesa_MultiTexCoord1sARB;
   exec->MultiTexCoord1svARB = _mesa_MultiTexCoord1svARB;
   exec->MultiTexCoord2dARB = _mesa_MultiTexCoord2dARB;
   exec->MultiTexCoord2dvARB = _mesa_MultiTexCoord2dvARB;
   exec->MultiTexCoord2fARB = _mesa_MultiTexCoord2fARB;
   exec->MultiTexCoord2fvARB = _mesa_MultiTexCoord2fvARB;
   exec->MultiTexCoord2iARB = _mesa_MultiTexCoord2iARB;
   exec->MultiTexCoord2ivARB = _mesa_MultiTexCoord2ivARB;
   exec->MultiTexCoord2sARB = _mesa_MultiTexCoord2sARB;
   exec->MultiTexCoord2svARB = _mesa_MultiTexCoord2svARB;
   exec->MultiTexCoord3dARB = _mesa_MultiTexCoord3dARB;
   exec->MultiTexCoord3dvARB = _mesa_MultiTexCoord3dvARB;
   exec->MultiTexCoord3fARB = _mesa_MultiTexCoord3fARB;
   exec->MultiTexCoord3fvARB = _mesa_MultiTexCoord3fvARB;
   exec->MultiTexCoord3iARB = _mesa_MultiTexCoord3iARB;
   exec->MultiTexCoord3ivARB = _mesa_MultiTexCoord3ivARB;
   exec->MultiTexCoord3sARB = _mesa_MultiTexCoord3sARB;
   exec->MultiTexCoord3svARB = _mesa_MultiTexCoord3svARB;
   exec->MultiTexCoord4dARB = _mesa_MultiTexCoord4dARB;
   exec->MultiTexCoord4dvARB = _mesa_MultiTexCoord4dvARB;
   exec->MultiTexCoord4fARB = _mesa_MultiTexCoord4fARB;
   exec->MultiTexCoord4fvARB = _mesa_MultiTexCoord4fvARB;
   exec->MultiTexCoord4iARB = _mesa_MultiTexCoord4iARB;
   exec->MultiTexCoord4ivARB = _mesa_MultiTexCoord4ivARB;
   exec->MultiTexCoord4sARB = _mesa_MultiTexCoord4sARB;
   exec->MultiTexCoord4svARB = _mesa_MultiTexCoord4svARB;

   /* GL_INGR_blend_func_separate */
   exec->BlendFuncSeparateINGR = _mesa_BlendFuncSeparateINGR;

   /* GL_MESA_window_pos */
   exec->WindowPos4fMESA = _mesa_WindowPos4fMESA;

   /* GL_MESA_resize_buffers */
   exec->ResizeBuffersMESA = _mesa_ResizeBuffersMESA;

   /* GL_ARB_transpose_matrix */
   exec->LoadTransposeMatrixdARB = _mesa_LoadTransposeMatrixdARB;
   exec->LoadTransposeMatrixfARB = _mesa_LoadTransposeMatrixfARB;
   exec->MultTransposeMatrixdARB = _mesa_MultTransposeMatrixdARB;
   exec->MultTransposeMatrixfARB = _mesa_MultTransposeMatrixfARB;
}

