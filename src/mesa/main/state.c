/* $Id: state.c,v 1.39 2000/10/31 18:09:45 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
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


/*
 * This file initializes the immediate-mode dispatch table (which may
 * be state-dependant) and manages internal Mesa state update.
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
#include "buffers.h"
#include "clip.h"
#include "colortab.h"
#include "context.h"
#include "convolve.h"
#include "copypix.h"
#include "cva.h"
#include "depth.h"
#include "dlist.h"
#include "drawpix.h"
#include "enable.h"
#include "eval.h"
#include "get.h"
#include "feedback.h"
#include "fog.h"
#include "hint.h"
#include "imaging.h"
#include "light.h"
#include "lines.h"
#include "logic.h"
#include "masking.h"
#include "matrix.h"
#include "mmath.h"
#include "pipeline.h"
#include "pixel.h"
#include "pixeltex.h"
#include "points.h"
#include "polygon.h"
#include "rastpos.h"
#include "readpix.h"
#include "rect.h"
#include "scissor.h"
#include "shade.h"
#include "state.h"
#include "stencil.h"
#include "teximage.h"
#include "texobj.h"
#include "texstate.h"
#include "texture.h"
#include "types.h"
#include "varray.h"
#include "vbfill.h"
#include "vbrender.h"
#include "winpos.h"
#include "swrast/swrast.h"
#endif



static int
generic_noop(void)
{
#ifdef DEBUG
   gl_problem(NULL, "undefined function dispatch");
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
 */
void
_mesa_init_exec_table(struct _glapi_table *exec, GLuint tableSize)
{
   /* first initialize all dispatch slots to no-op */
   _mesa_init_no_op_table(exec, tableSize);

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

   exec->FogCoordfEXT = _mesa_FogCoordfEXT;
   exec->FogCoordfvEXT = _mesa_FogCoordfvEXT;
   exec->FogCoorddEXT = _mesa_FogCoorddEXT;
   exec->FogCoorddvEXT = _mesa_FogCoorddvEXT;
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
   exec->SecondaryColor3bEXT = _mesa_SecondaryColor3bEXT;
   exec->SecondaryColor3bvEXT = _mesa_SecondaryColor3bvEXT;
   exec->SecondaryColor3sEXT = _mesa_SecondaryColor3sEXT;
   exec->SecondaryColor3svEXT = _mesa_SecondaryColor3svEXT;
   exec->SecondaryColor3iEXT = _mesa_SecondaryColor3iEXT;
   exec->SecondaryColor3ivEXT = _mesa_SecondaryColor3ivEXT;
   exec->SecondaryColor3fEXT = _mesa_SecondaryColor3fEXT;
   exec->SecondaryColor3fvEXT = _mesa_SecondaryColor3fvEXT;
   exec->SecondaryColor3dEXT = _mesa_SecondaryColor3dEXT;
   exec->SecondaryColor3dvEXT = _mesa_SecondaryColor3dvEXT;
   exec->SecondaryColor3ubEXT = _mesa_SecondaryColor3ubEXT;
   exec->SecondaryColor3ubvEXT = _mesa_SecondaryColor3ubvEXT;
   exec->SecondaryColor3usEXT = _mesa_SecondaryColor3usEXT;
   exec->SecondaryColor3usvEXT = _mesa_SecondaryColor3usvEXT;
   exec->SecondaryColor3uiEXT = _mesa_SecondaryColor3uiEXT;
   exec->SecondaryColor3uivEXT = _mesa_SecondaryColor3uivEXT;
   exec->SecondaryColorPointerEXT = _mesa_SecondaryColorPointerEXT;
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

   /* 77. GL_PGI_misc_hints */
   exec->HintPGI = _mesa_HintPGI;

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

   /* ARB 3. GL_ARB_transpose_matrix */
   exec->LoadTransposeMatrixdARB = _mesa_LoadTransposeMatrixdARB;
   exec->LoadTransposeMatrixfARB = _mesa_LoadTransposeMatrixfARB;
   exec->MultTransposeMatrixdARB = _mesa_MultTransposeMatrixdARB;
   exec->MultTransposeMatrixfARB = _mesa_MultTransposeMatrixfARB;

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



/*
 * Recompute the value of ctx->RasterMask, etc. according to
 * the current context.
 */
static void update_rasterflags( GLcontext *ctx )
{
   ctx->RasterMask = 0;

   if (ctx->Color.AlphaEnabled)           ctx->RasterMask |= ALPHATEST_BIT;
   if (ctx->Color.BlendEnabled)           ctx->RasterMask |= BLEND_BIT;
   if (ctx->Depth.Test)                   ctx->RasterMask |= DEPTH_BIT;
   if (ctx->Fog.Enabled)                  ctx->RasterMask |= FOG_BIT;
   if (ctx->Scissor.Enabled)              ctx->RasterMask |= SCISSOR_BIT;
   if (ctx->Stencil.Enabled)              ctx->RasterMask |= STENCIL_BIT;
   if (ctx->Visual.RGBAflag) {
      const GLuint colorMask = *((GLuint *) &ctx->Color.ColorMask);
      if (colorMask != 0xffffffff)        ctx->RasterMask |= MASKING_BIT;
      if (ctx->Color.ColorLogicOpEnabled) ctx->RasterMask |= LOGIC_OP_BIT;
      if (ctx->Texture.ReallyEnabled)     ctx->RasterMask |= TEXTURE_BIT;
   }
   else {
      if (ctx->Color.IndexMask != 0xffffffff) ctx->RasterMask |= MASKING_BIT;
      if (ctx->Color.IndexLogicOpEnabled)     ctx->RasterMask |= LOGIC_OP_BIT;
   }

   if (ctx->DrawBuffer->UseSoftwareAlphaBuffers
       && ctx->Color.ColorMask[ACOMP]
       && ctx->Color.DrawBuffer != GL_NONE)
      ctx->RasterMask |= ALPHABUF_BIT;

   if (   ctx->Viewport.X < 0
       || ctx->Viewport.X + ctx->Viewport.Width > ctx->DrawBuffer->Width
       || ctx->Viewport.Y < 0
       || ctx->Viewport.Y + ctx->Viewport.Height > ctx->DrawBuffer->Height) {
      ctx->RasterMask |= WINCLIP_BIT;
   }

   if (ctx->Depth.OcclusionTest)
      ctx->RasterMask |= OCCLUSION_BIT;


   /* If we're not drawing to exactly one color buffer set the
    * MULTI_DRAW_BIT flag.  Also set it if we're drawing to no
    * buffers or the RGBA or CI mask disables all writes.
    */
   ctx->TriangleCaps &= ~DD_MULTIDRAW;

   if (ctx->Color.MultiDrawBuffer) {
      ctx->RasterMask |= MULTI_DRAW_BIT;
      ctx->TriangleCaps |= DD_MULTIDRAW;
   }
   else if (ctx->Color.DrawBuffer==GL_NONE) {
      ctx->RasterMask |= MULTI_DRAW_BIT;
      ctx->TriangleCaps |= DD_MULTIDRAW;
   }
   else if (ctx->Visual.RGBAflag && *((GLuint *) ctx->Color.ColorMask) == 0) {
      /* all RGBA channels disabled */
      ctx->RasterMask |= MULTI_DRAW_BIT;
      ctx->TriangleCaps |= DD_MULTIDRAW;
   }
   else if (!ctx->Visual.RGBAflag && ctx->Color.IndexMask==0) {
      /* all color index bits disabled */
      ctx->RasterMask |= MULTI_DRAW_BIT;
      ctx->TriangleCaps |= DD_MULTIDRAW;
   }
}


void gl_print_state( const char *msg, GLuint state )
{
   fprintf(stderr,
	   "%s: (0x%x) %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
	   msg,
	   state,
	   (state & _NEW_MODELVIEW)       ? "ctx->ModelView, " : "",
	   (state & _NEW_PROJECTION)      ? "ctx->Projection, " : "",
	   (state & _NEW_TEXTURE_MATRIX)  ? "ctx->TextureMatrix, " : "",
	   (state & _NEW_COLOR_MATRIX)    ? "ctx->ColorMatrix, " : "",
	   (state & _NEW_ACCUM)           ? "ctx->Accum, " : "",
	   (state & _NEW_COLOR)           ? "ctx->Color, " : "",
	   (state & _NEW_DEPTH)           ? "ctx->Depth, " : "",
	   (state & _NEW_EVAL)            ? "ctx->Eval/EvalMap, " : "",
	   (state & _NEW_FOG)             ? "ctx->Fog, " : "",
	   (state & _NEW_HINT)            ? "ctx->Hint, " : "",
	   (state & _NEW_IMAGING)         ? "ctx->Histogram/MinMax/Convolve/Seperable, ": "",
	   (state & _NEW_LIGHT)           ? "ctx->Light, " : "",
	   (state & _NEW_LINE)            ? "ctx->Line, " : "",
	   (state & _NEW_FEEDBACK_SELECT) ? "ctx->Feedback/Select, " : "",
	   (state & _NEW_PIXEL)           ? "ctx->Pixel, " : "",
	   (state & _NEW_POINT)           ? "ctx->Point, " : "",
	   (state & _NEW_POLYGON)         ? "ctx->Polygon, " : "",
	   (state & _NEW_POLYGONSTIPPLE)  ? "ctx->PolygonStipple, " : "",
	   (state & _NEW_SCISSOR)         ? "ctx->Scissor, " : "",
	   (state & _NEW_TEXTURE)         ? "ctx->Texture, " : "",
	   (state & _NEW_TRANSFORM)       ? "ctx->Transform, " : "",
	   (state & _NEW_VIEWPORT)        ? "ctx->Viewport, " : "",
	   (state & _NEW_PACKUNPACK)      ? "ctx->Pack/Unpack, " : "",
	   (state & _NEW_ARRAY)           ? "ctx->Array, " : "",
	   (state & _NEW_COLORTABLE)      ? "ctx->{*}ColorTable, " : "",
	   (state & _NEW_RENDERMODE)      ? "ctx->RenderMode, " : "",
	   (state & _NEW_BUFFERS)         ? "ctx->Visual, ctx->DrawBuffer,, " : "");
}


void gl_print_enable_flags( const char *msg, GLuint flags )
{
   fprintf(stderr,
	   "%s: (0x%x) %s%s%s%s%s%s%s%s%s%s%s\n",
	   msg,
	   flags,
	   (flags & ENABLE_TEX0)       ? "tex-0, " : "",
	   (flags & ENABLE_TEX1)       ? "tex-1, " : "",
	   (flags & ENABLE_LIGHT)      ? "light, " : "",
	   (flags & ENABLE_FOG)        ? "fog, " : "",
	   (flags & ENABLE_USERCLIP)   ? "userclip, " : "",
	   (flags & ENABLE_TEXGEN0)    ? "tex-gen-0, " : "",
	   (flags & ENABLE_TEXGEN1)    ? "tex-gen-1, " : "",
	   (flags & ENABLE_TEXMAT0)    ? "tex-mat-0, " : "",
	   (flags & ENABLE_TEXMAT1)    ? "tex-mat-1, " : "",
	   (flags & ENABLE_NORMALIZE)  ? "normalize, " : "",
	   (flags & ENABLE_RESCALE)    ? "rescale, " : "");
}


/*
 * If ctx->NewState is non-zero then this function MUST be called before
 * rendering any primitive.  Basically, function pointers and miscellaneous
 * flags are updated to reflect the current state of the state machine.
 */
void gl_update_state( GLcontext *ctx )
{
   GLuint i;

   if (MESA_VERBOSE & VERBOSE_STATE)
      gl_print_state("", ctx->NewState);

   if (ctx->NewState & _NEW_PIXEL)
      _mesa_update_image_transfer_state(ctx);

   if (ctx->NewState & _NEW_ARRAY)
      gl_update_client_state( ctx );

   if (ctx->NewState & _NEW_TEXTURE_MATRIX) {
      ctx->Enabled &= ~(ENABLE_TEXMAT0 | ENABLE_TEXMAT1 | ENABLE_TEXMAT2);

      for (i=0; i < ctx->Const.MaxTextureUnits; i++) {
	 if (ctx->TextureMatrix[i].flags & MAT_DIRTY_ALL_OVER) {
	    gl_matrix_analyze( &ctx->TextureMatrix[i] );
	    ctx->TextureMatrix[i].flags &= ~MAT_DIRTY_DEPENDENTS;

	    if (ctx->Texture.Unit[i].Enabled &&
		ctx->TextureMatrix[i].type != MATRIX_IDENTITY)
	       ctx->Enabled |= ENABLE_TEXMAT0 << i;
	 }
      }
   }

   if (ctx->NewState & _NEW_TEXTURE) {
      ctx->Texture.MultiTextureEnabled = GL_FALSE;
      ctx->Texture.NeedNormals = GL_FALSE;
      gl_update_dirty_texobjs(ctx);
      ctx->Enabled &= ~(ENABLE_TEXGEN0 | ENABLE_TEXGEN1 | ENABLE_TEXGEN2);
      ctx->Texture.ReallyEnabled = 0;

      for (i=0; i < ctx->Const.MaxTextureUnits; i++) {
	 if (ctx->Texture.Unit[i].Enabled) {
	    gl_update_texture_unit( ctx, &ctx->Texture.Unit[i] );

	    ctx->Texture.ReallyEnabled |=
	       ctx->Texture.Unit[i].ReallyEnabled << (i * 4);

	    if (ctx->Texture.Unit[i].GenFlags != 0) {
	       ctx->Enabled |= ENABLE_TEXGEN0 << i;

	       if (ctx->Texture.Unit[i].GenFlags & TEXGEN_NEED_NORMALS) {
		  ctx->Texture.NeedNormals = GL_TRUE;
		  ctx->Texture.NeedEyeCoords = GL_TRUE;
	       }

	       if (ctx->Texture.Unit[i].GenFlags & TEXGEN_NEED_EYE_COORD) {
		  ctx->Texture.NeedEyeCoords = GL_TRUE;
	       }
	    }

            if (i > 0 && ctx->Texture.Unit[i].ReallyEnabled) {
               ctx->Texture.MultiTextureEnabled = GL_TRUE;
            }
	 }
         else {
            ctx->Texture.Unit[i].ReallyEnabled = 0;
         }
      }
      ctx->Enabled = (ctx->Enabled & ~ENABLE_TEX_ANY) | ctx->Texture.ReallyEnabled;
      ctx->NeedNormals = (ctx->Light.Enabled || ctx->Texture.NeedNormals);
   }


   if (ctx->NewState & _SWRAST_NEW_RASTERMASK) 
      update_rasterflags(ctx);
   
   if (ctx->NewState & (_NEW_BUFFERS|_NEW_SCISSOR)) {
      /* update scissor region */
      ctx->DrawBuffer->Xmin = 0;
      ctx->DrawBuffer->Ymin = 0;
      ctx->DrawBuffer->Xmax = ctx->DrawBuffer->Width;
      ctx->DrawBuffer->Ymax = ctx->DrawBuffer->Height;
      if (ctx->Scissor.Enabled) {
         if (ctx->Scissor.X > ctx->DrawBuffer->Xmin) {
            ctx->DrawBuffer->Xmin = ctx->Scissor.X;
         }
         if (ctx->Scissor.Y > ctx->DrawBuffer->Ymin) {
            ctx->DrawBuffer->Ymin = ctx->Scissor.Y;
         }
         if (ctx->Scissor.X + ctx->Scissor.Width < ctx->DrawBuffer->Xmax) {
            ctx->DrawBuffer->Xmax = ctx->Scissor.X + ctx->Scissor.Width;
         }
         if (ctx->Scissor.Y + ctx->Scissor.Height < ctx->DrawBuffer->Ymax) {
            ctx->DrawBuffer->Ymax = ctx->Scissor.Y + ctx->Scissor.Height;
         }
      }
   }

   if (ctx->NewState & _NEW_LIGHT) {
      ctx->TriangleCaps &= ~(DD_TRI_LIGHT_TWOSIDE|DD_LIGHTING_CULL);
      if (ctx->Light.Enabled) {
         if (ctx->Light.Model.TwoSide)
            ctx->TriangleCaps |= (DD_TRI_LIGHT_TWOSIDE|DD_LIGHTING_CULL);
         gl_update_lighting(ctx);
      }
   }

   if (ctx->NewState & (_NEW_POLYGON | _NEW_LIGHT)) {

      ctx->TriangleCaps &= ~DD_TRI_CULL_FRONT_BACK;

      if (ctx->NewState & _NEW_POLYGON) {
	 /* Setup CullBits bitmask */
	 if (ctx->Polygon.CullFlag) {
	    ctx->backface_sign = 1;
	    switch(ctx->Polygon.CullFaceMode) {
	    case GL_BACK:
	       if(ctx->Polygon.FrontFace==GL_CCW)
		  ctx->backface_sign = -1;
	       ctx->Polygon.CullBits = 1;
	       break;
	    case GL_FRONT:
	       if(ctx->Polygon.FrontFace!=GL_CCW)
		  ctx->backface_sign = -1;
	       ctx->Polygon.CullBits = 2;
	       break;
	    default:
	    case GL_FRONT_AND_BACK:
	       ctx->backface_sign = 0;
	       ctx->Polygon.CullBits = 0;
	       ctx->TriangleCaps |= DD_TRI_CULL_FRONT_BACK;
	       break;
	    }
	 }
	 else {
	    ctx->Polygon.CullBits = 3;
	    ctx->backface_sign = 0;
	 }

	 /* Any Polygon offsets enabled? */
	 ctx->TriangleCaps &= ~DD_TRI_OFFSET;

	 if (ctx->Polygon.OffsetPoint ||
	     ctx->Polygon.OffsetLine ||
	     ctx->Polygon.OffsetFill)
	    ctx->TriangleCaps |= DD_TRI_OFFSET;
      }
   }

   if (ctx->NewState & (_NEW_LIGHT|
			_NEW_TEXTURE|
			_NEW_FOG|
			_NEW_POLYGON))
      gl_update_clipmask(ctx);

   if (ctx->NewState & ctx->Driver.UpdateStateNotify)
   {
      ctx->IndirectTriangles = ctx->TriangleCaps & ~ctx->Driver.TriangleCaps;
      ctx->IndirectTriangles |= DD_SW_RASTERIZE;

      if (MESA_VERBOSE&VERBOSE_CULL)
	 gl_print_tri_caps("initial indirect tris", ctx->IndirectTriangles);

      ctx->Driver.PointsFunc = NULL;
      ctx->Driver.LineFunc = NULL;
      ctx->Driver.TriangleFunc = NULL;
      ctx->Driver.QuadFunc = NULL;
      ctx->Driver.RectFunc = NULL;
      ctx->Driver.RenderVBClippedTab = NULL;
      ctx->Driver.RenderVBCulledTab = NULL;
      ctx->Driver.RenderVBRawTab = NULL;

      /*
       * Here the driver sets up all the ctx->Driver function pointers to
       * it's specific, private functions.
       */
      ctx->Driver.UpdateState(ctx);

      if (MESA_VERBOSE&VERBOSE_CULL)
	 gl_print_tri_caps("indirect tris", ctx->IndirectTriangles);

      /*
       * In case the driver didn't hook in an optimized point, line or
       * triangle function we'll now select "core/fallback" point, line
       * and triangle functions.
       */
      if (ctx->IndirectTriangles & DD_SW_RASTERIZE) {
	 _swrast_set_point_function(ctx);
	 _swrast_set_line_function(ctx);
	 _swrast_set_triangle_function(ctx);
	 _swrast_set_quad_function(ctx);

	 if ((ctx->IndirectTriangles & 
	      (DD_TRI_SW_RASTERIZE|DD_QUAD_SW_RASTERIZE|DD_TRI_CULL)) ==
	     (DD_TRI_SW_RASTERIZE|DD_QUAD_SW_RASTERIZE|DD_TRI_CULL)) 
	    ctx->IndirectTriangles &= ~DD_TRI_CULL;
      }

      if (MESA_VERBOSE&VERBOSE_CULL)
	 gl_print_tri_caps("indirect tris 2", ctx->IndirectTriangles);

      gl_set_render_vb_function(ctx);
   }

   /* Should only be calc'd when !need_eye_coords and not culling.
    */
   if (ctx->NewState & (_NEW_MODELVIEW|_NEW_PROJECTION)) {
      if (ctx->NewState & _NEW_MODELVIEW) {
	 gl_matrix_analyze( &ctx->ModelView );
	 ctx->ProjectionMatrix.flags &= ~MAT_DIRTY_DEPENDENTS;
      }

      if (ctx->NewState & _NEW_PROJECTION) {
	 gl_matrix_analyze( &ctx->ProjectionMatrix );
	 ctx->ProjectionMatrix.flags &= ~MAT_DIRTY_DEPENDENTS;

	 if (ctx->Transform.AnyClip) {
	    gl_update_userclip( ctx );
	 }
      }

      gl_calculate_model_project_matrix( ctx );
   }

   if (ctx->NewState & _NEW_COLOR_MATRIX) {
      gl_matrix_analyze( &ctx->ColorMatrix );
   }

   /* Figure out whether we can light in object space or not.  If we
    * can, find the current positions of the lights in object space
    */
   if ((ctx->Enabled & (ENABLE_POINT_ATTEN | ENABLE_LIGHT | ENABLE_FOG |
			ENABLE_TEXGEN0 | ENABLE_TEXGEN1 | ENABLE_TEXGEN2)) &&
       (ctx->NewState & (_NEW_LIGHT | 
			 _NEW_TEXTURE |
                         _NEW_FOG |
			 _NEW_TRANSFORM | 
			 _NEW_MODELVIEW | 
			 _NEW_PROJECTION |
			 _NEW_POINT |
			 _NEW_RENDERMODE |
			 _NEW_TRANSFORM)))
   {
      GLboolean oldcoord, oldnorm;

      oldcoord = ctx->NeedEyeCoords;
      oldnorm = ctx->NeedEyeNormals;

      ctx->NeedNormals = (ctx->Light.Enabled || ctx->Texture.NeedNormals);
      ctx->NeedEyeCoords = (ctx->Fog.Enabled || ctx->Point.Attenuated);
      ctx->NeedEyeNormals = GL_FALSE;

      if (ctx->Light.Enabled) {
	 if ((ctx->Light.Flags & LIGHT_POSITIONAL) ||
             ctx->Light.NeedVertices ||
             !TEST_MAT_FLAGS( &ctx->ModelView, MAT_FLAGS_LENGTH_PRESERVING)) {
            /* Need length for attenuation or need angle for spotlights
             * or non-uniform scale matrix
             */
            ctx->NeedEyeCoords = GL_TRUE;
	 }
	 ctx->NeedEyeNormals = ctx->NeedEyeCoords;
      }
      if (ctx->Texture.ReallyEnabled || ctx->RenderMode==GL_FEEDBACK) {
	 if (ctx->Texture.NeedEyeCoords) ctx->NeedEyeCoords = GL_TRUE;
	 if (ctx->Texture.NeedNormals)
	    ctx->NeedNormals = ctx->NeedEyeNormals = GL_TRUE;
      }

      ctx->vb_proj_matrix = &ctx->ModelProjectMatrix;

      if (ctx->NeedEyeCoords)
	 ctx->vb_proj_matrix = &ctx->ProjectionMatrix;

      if (ctx->Light.Enabled) {
	 gl_update_lighting_function(ctx);

	 if ( (ctx->NewState & _NEW_LIGHT) ||
	      ((ctx->NewState & (_NEW_MODELVIEW|_NEW_PROJECTION)) &&
	       !ctx->NeedEyeCoords) ||
	      oldcoord != ctx->NeedEyeCoords ||
	      oldnorm != ctx->NeedEyeNormals) {
	    gl_compute_light_positions(ctx);
	 }

	 ctx->rescale_factor = 1.0F;
	 if (ctx->ModelView.flags & (MAT_FLAG_UNIFORM_SCALE |
				     MAT_FLAG_GENERAL_SCALE |
				     MAT_FLAG_GENERAL_3D |
				     MAT_FLAG_GENERAL) ) {
	    const GLfloat *m = ctx->ModelView.inv;
	    const GLfloat f = m[2] * m[2] + m[6] * m[6] + m[10] * m[10];
	    if (f > 1e-12 && (f - 1.0) * (f - 1.0) > 1e-12)
	       ctx->rescale_factor = 1.0 / GL_SQRT(f);
	 }
      }

      gl_update_normal_transform( ctx );
   }

   gl_update_pipelines(ctx);
   ctx->NewState = 0;
}




/*
 * Return a bitmask of IMAGE_*_BIT flags which to indicate which
 * pixel transfer operations are enabled.
 */
void
_mesa_update_image_transfer_state(GLcontext *ctx)
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
       ctx->Pixel.Separable2DEnabled)
      mask |= IMAGE_CONVOLUTION_BIT;

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

   ctx->ImageTransferState = mask;
}
