/* DO NOT EDIT - This file generated automatically by glX_proto_send.py (from Mesa) script */

/*
 * Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
 * (C) Copyright IBM Corporation 2004
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT, IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/**
 * \file indirect_init.c
 * Initialize indirect rendering dispatch table.
 *
 * \author Kevin E. Martin <kevin@precisioninsight.com>
 * \author Brian Paul <brian@precisioninsight.com>
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include "indirect_init.h"
#include "indirect.h"
#include "glapi.h"


/**
 * No-op function used to initialize functions that have no GLX protocol
 * support.
 */
static int NoOp(void)
{
    return 0;
}

/**
 * Create and initialize a new GL dispatch table.  The table is initialized
 * with GLX indirect rendering protocol functions.
 */
__GLapi * __glXNewIndirectAPI( void )
{
    __GLapi *glAPI;
    GLuint entries;

    entries = _glapi_get_dispatch_table_size();
    glAPI = (__GLapi *) Xmalloc(entries * sizeof(void *));

    /* first, set all entries to point to no-op functions */
    {
       int i;
       void **dispatch = (void **) glAPI;
       for (i = 0; i < entries; i++) {
          dispatch[i] = (void *) NoOp;
       }
    }

    /* now, initialize the entries we understand */

    /* 1.0 */

    glAPI->Accum = __indirect_glAccum;
    glAPI->AlphaFunc = __indirect_glAlphaFunc;
    glAPI->Begin = __indirect_glBegin;
    glAPI->Bitmap = __indirect_glBitmap;
    glAPI->BlendFunc = __indirect_glBlendFunc;
    glAPI->CallList = __indirect_glCallList;
    glAPI->CallLists = __indirect_glCallLists;
    glAPI->Clear = __indirect_glClear;
    glAPI->ClearAccum = __indirect_glClearAccum;
    glAPI->ClearColor = __indirect_glClearColor;
    glAPI->ClearDepth = __indirect_glClearDepth;
    glAPI->ClearIndex = __indirect_glClearIndex;
    glAPI->ClearStencil = __indirect_glClearStencil;
    glAPI->ClipPlane = __indirect_glClipPlane;
    glAPI->Color3b = __indirect_glColor3b;
    glAPI->Color3bv = __indirect_glColor3bv;
    glAPI->Color3d = __indirect_glColor3d;
    glAPI->Color3dv = __indirect_glColor3dv;
    glAPI->Color3f = __indirect_glColor3f;
    glAPI->Color3fv = __indirect_glColor3fv;
    glAPI->Color3i = __indirect_glColor3i;
    glAPI->Color3iv = __indirect_glColor3iv;
    glAPI->Color3s = __indirect_glColor3s;
    glAPI->Color3sv = __indirect_glColor3sv;
    glAPI->Color3ub = __indirect_glColor3ub;
    glAPI->Color3ubv = __indirect_glColor3ubv;
    glAPI->Color3ui = __indirect_glColor3ui;
    glAPI->Color3uiv = __indirect_glColor3uiv;
    glAPI->Color3us = __indirect_glColor3us;
    glAPI->Color3usv = __indirect_glColor3usv;
    glAPI->Color4b = __indirect_glColor4b;
    glAPI->Color4bv = __indirect_glColor4bv;
    glAPI->Color4d = __indirect_glColor4d;
    glAPI->Color4dv = __indirect_glColor4dv;
    glAPI->Color4f = __indirect_glColor4f;
    glAPI->Color4fv = __indirect_glColor4fv;
    glAPI->Color4i = __indirect_glColor4i;
    glAPI->Color4iv = __indirect_glColor4iv;
    glAPI->Color4s = __indirect_glColor4s;
    glAPI->Color4sv = __indirect_glColor4sv;
    glAPI->Color4ub = __indirect_glColor4ub;
    glAPI->Color4ubv = __indirect_glColor4ubv;
    glAPI->Color4ui = __indirect_glColor4ui;
    glAPI->Color4uiv = __indirect_glColor4uiv;
    glAPI->Color4us = __indirect_glColor4us;
    glAPI->Color4usv = __indirect_glColor4usv;
    glAPI->ColorMask = __indirect_glColorMask;
    glAPI->ColorMaterial = __indirect_glColorMaterial;
    glAPI->CopyPixels = __indirect_glCopyPixels;
    glAPI->CullFace = __indirect_glCullFace;
    glAPI->DeleteLists = __indirect_glDeleteLists;
    glAPI->DepthFunc = __indirect_glDepthFunc;
    glAPI->DepthMask = __indirect_glDepthMask;
    glAPI->DepthRange = __indirect_glDepthRange;
    glAPI->Disable = __indirect_glDisable;
    glAPI->DrawBuffer = __indirect_glDrawBuffer;
    glAPI->DrawPixels = __indirect_glDrawPixels;
    glAPI->EdgeFlag = __indirect_glEdgeFlag;
    glAPI->EdgeFlagv = __indirect_glEdgeFlagv;
    glAPI->Enable = __indirect_glEnable;
    glAPI->End = __indirect_glEnd;
    glAPI->EndList = __indirect_glEndList;
    glAPI->EvalCoord1d = __indirect_glEvalCoord1d;
    glAPI->EvalCoord1dv = __indirect_glEvalCoord1dv;
    glAPI->EvalCoord1f = __indirect_glEvalCoord1f;
    glAPI->EvalCoord1fv = __indirect_glEvalCoord1fv;
    glAPI->EvalCoord2d = __indirect_glEvalCoord2d;
    glAPI->EvalCoord2dv = __indirect_glEvalCoord2dv;
    glAPI->EvalCoord2f = __indirect_glEvalCoord2f;
    glAPI->EvalCoord2fv = __indirect_glEvalCoord2fv;
    glAPI->EvalMesh1 = __indirect_glEvalMesh1;
    glAPI->EvalMesh2 = __indirect_glEvalMesh2;
    glAPI->EvalPoint1 = __indirect_glEvalPoint1;
    glAPI->EvalPoint2 = __indirect_glEvalPoint2;
    glAPI->FeedbackBuffer = __indirect_glFeedbackBuffer;
    glAPI->Finish = __indirect_glFinish;
    glAPI->Flush = __indirect_glFlush;
    glAPI->Fogf = __indirect_glFogf;
    glAPI->Fogfv = __indirect_glFogfv;
    glAPI->Fogi = __indirect_glFogi;
    glAPI->Fogiv = __indirect_glFogiv;
    glAPI->FrontFace = __indirect_glFrontFace;
    glAPI->Frustum = __indirect_glFrustum;
    glAPI->GenLists = __indirect_glGenLists;
    glAPI->GetBooleanv = __indirect_glGetBooleanv;
    glAPI->GetClipPlane = __indirect_glGetClipPlane;
    glAPI->GetDoublev = __indirect_glGetDoublev;
    glAPI->GetError = __indirect_glGetError;
    glAPI->GetFloatv = __indirect_glGetFloatv;
    glAPI->GetIntegerv = __indirect_glGetIntegerv;
    glAPI->GetLightfv = __indirect_glGetLightfv;
    glAPI->GetLightiv = __indirect_glGetLightiv;
    glAPI->GetMapdv = __indirect_glGetMapdv;
    glAPI->GetMapfv = __indirect_glGetMapfv;
    glAPI->GetMapiv = __indirect_glGetMapiv;
    glAPI->GetMaterialfv = __indirect_glGetMaterialfv;
    glAPI->GetMaterialiv = __indirect_glGetMaterialiv;
    glAPI->GetPixelMapfv = __indirect_glGetPixelMapfv;
    glAPI->GetPixelMapuiv = __indirect_glGetPixelMapuiv;
    glAPI->GetPixelMapusv = __indirect_glGetPixelMapusv;
    glAPI->GetPolygonStipple = __indirect_glGetPolygonStipple;
    glAPI->GetString = __indirect_glGetString;
    glAPI->GetTexEnvfv = __indirect_glGetTexEnvfv;
    glAPI->GetTexEnviv = __indirect_glGetTexEnviv;
    glAPI->GetTexGendv = __indirect_glGetTexGendv;
    glAPI->GetTexGenfv = __indirect_glGetTexGenfv;
    glAPI->GetTexGeniv = __indirect_glGetTexGeniv;
    glAPI->GetTexImage = __indirect_glGetTexImage;
    glAPI->GetTexLevelParameterfv = __indirect_glGetTexLevelParameterfv;
    glAPI->GetTexLevelParameteriv = __indirect_glGetTexLevelParameteriv;
    glAPI->GetTexParameterfv = __indirect_glGetTexParameterfv;
    glAPI->GetTexParameteriv = __indirect_glGetTexParameteriv;
    glAPI->Hint = __indirect_glHint;
    glAPI->IndexMask = __indirect_glIndexMask;
    glAPI->Indexd = __indirect_glIndexd;
    glAPI->Indexdv = __indirect_glIndexdv;
    glAPI->Indexf = __indirect_glIndexf;
    glAPI->Indexfv = __indirect_glIndexfv;
    glAPI->Indexi = __indirect_glIndexi;
    glAPI->Indexiv = __indirect_glIndexiv;
    glAPI->Indexs = __indirect_glIndexs;
    glAPI->Indexsv = __indirect_glIndexsv;
    glAPI->InitNames = __indirect_glInitNames;
    glAPI->IsEnabled = __indirect_glIsEnabled;
    glAPI->IsList = __indirect_glIsList;
    glAPI->LightModelf = __indirect_glLightModelf;
    glAPI->LightModelfv = __indirect_glLightModelfv;
    glAPI->LightModeli = __indirect_glLightModeli;
    glAPI->LightModeliv = __indirect_glLightModeliv;
    glAPI->Lightf = __indirect_glLightf;
    glAPI->Lightfv = __indirect_glLightfv;
    glAPI->Lighti = __indirect_glLighti;
    glAPI->Lightiv = __indirect_glLightiv;
    glAPI->LineStipple = __indirect_glLineStipple;
    glAPI->LineWidth = __indirect_glLineWidth;
    glAPI->ListBase = __indirect_glListBase;
    glAPI->LoadIdentity = __indirect_glLoadIdentity;
    glAPI->LoadMatrixd = __indirect_glLoadMatrixd;
    glAPI->LoadMatrixf = __indirect_glLoadMatrixf;
    glAPI->LoadName = __indirect_glLoadName;
    glAPI->LogicOp = __indirect_glLogicOp;
    glAPI->Map1d = __indirect_glMap1d;
    glAPI->Map1f = __indirect_glMap1f;
    glAPI->Map2d = __indirect_glMap2d;
    glAPI->Map2f = __indirect_glMap2f;
    glAPI->MapGrid1d = __indirect_glMapGrid1d;
    glAPI->MapGrid1f = __indirect_glMapGrid1f;
    glAPI->MapGrid2d = __indirect_glMapGrid2d;
    glAPI->MapGrid2f = __indirect_glMapGrid2f;
    glAPI->Materialf = __indirect_glMaterialf;
    glAPI->Materialfv = __indirect_glMaterialfv;
    glAPI->Materiali = __indirect_glMateriali;
    glAPI->Materialiv = __indirect_glMaterialiv;
    glAPI->MatrixMode = __indirect_glMatrixMode;
    glAPI->MultMatrixd = __indirect_glMultMatrixd;
    glAPI->MultMatrixf = __indirect_glMultMatrixf;
    glAPI->NewList = __indirect_glNewList;
    glAPI->Normal3b = __indirect_glNormal3b;
    glAPI->Normal3bv = __indirect_glNormal3bv;
    glAPI->Normal3d = __indirect_glNormal3d;
    glAPI->Normal3dv = __indirect_glNormal3dv;
    glAPI->Normal3f = __indirect_glNormal3f;
    glAPI->Normal3fv = __indirect_glNormal3fv;
    glAPI->Normal3i = __indirect_glNormal3i;
    glAPI->Normal3iv = __indirect_glNormal3iv;
    glAPI->Normal3s = __indirect_glNormal3s;
    glAPI->Normal3sv = __indirect_glNormal3sv;
    glAPI->Ortho = __indirect_glOrtho;
    glAPI->PassThrough = __indirect_glPassThrough;
    glAPI->PixelMapfv = __indirect_glPixelMapfv;
    glAPI->PixelMapuiv = __indirect_glPixelMapuiv;
    glAPI->PixelMapusv = __indirect_glPixelMapusv;
    glAPI->PixelStoref = __indirect_glPixelStoref;
    glAPI->PixelStorei = __indirect_glPixelStorei;
    glAPI->PixelTransferf = __indirect_glPixelTransferf;
    glAPI->PixelTransferi = __indirect_glPixelTransferi;
    glAPI->PixelZoom = __indirect_glPixelZoom;
    glAPI->PointSize = __indirect_glPointSize;
    glAPI->PolygonMode = __indirect_glPolygonMode;
    glAPI->PolygonStipple = __indirect_glPolygonStipple;
    glAPI->PopAttrib = __indirect_glPopAttrib;
    glAPI->PopMatrix = __indirect_glPopMatrix;
    glAPI->PopName = __indirect_glPopName;
    glAPI->PushAttrib = __indirect_glPushAttrib;
    glAPI->PushMatrix = __indirect_glPushMatrix;
    glAPI->PushName = __indirect_glPushName;
    glAPI->RasterPos2d = __indirect_glRasterPos2d;
    glAPI->RasterPos2dv = __indirect_glRasterPos2dv;
    glAPI->RasterPos2f = __indirect_glRasterPos2f;
    glAPI->RasterPos2fv = __indirect_glRasterPos2fv;
    glAPI->RasterPos2i = __indirect_glRasterPos2i;
    glAPI->RasterPos2iv = __indirect_glRasterPos2iv;
    glAPI->RasterPos2s = __indirect_glRasterPos2s;
    glAPI->RasterPos2sv = __indirect_glRasterPos2sv;
    glAPI->RasterPos3d = __indirect_glRasterPos3d;
    glAPI->RasterPos3dv = __indirect_glRasterPos3dv;
    glAPI->RasterPos3f = __indirect_glRasterPos3f;
    glAPI->RasterPos3fv = __indirect_glRasterPos3fv;
    glAPI->RasterPos3i = __indirect_glRasterPos3i;
    glAPI->RasterPos3iv = __indirect_glRasterPos3iv;
    glAPI->RasterPos3s = __indirect_glRasterPos3s;
    glAPI->RasterPos3sv = __indirect_glRasterPos3sv;
    glAPI->RasterPos4d = __indirect_glRasterPos4d;
    glAPI->RasterPos4dv = __indirect_glRasterPos4dv;
    glAPI->RasterPos4f = __indirect_glRasterPos4f;
    glAPI->RasterPos4fv = __indirect_glRasterPos4fv;
    glAPI->RasterPos4i = __indirect_glRasterPos4i;
    glAPI->RasterPos4iv = __indirect_glRasterPos4iv;
    glAPI->RasterPos4s = __indirect_glRasterPos4s;
    glAPI->RasterPos4sv = __indirect_glRasterPos4sv;
    glAPI->ReadBuffer = __indirect_glReadBuffer;
    glAPI->ReadPixels = __indirect_glReadPixels;
    glAPI->Rectd = __indirect_glRectd;
    glAPI->Rectdv = __indirect_glRectdv;
    glAPI->Rectf = __indirect_glRectf;
    glAPI->Rectfv = __indirect_glRectfv;
    glAPI->Recti = __indirect_glRecti;
    glAPI->Rectiv = __indirect_glRectiv;
    glAPI->Rects = __indirect_glRects;
    glAPI->Rectsv = __indirect_glRectsv;
    glAPI->RenderMode = __indirect_glRenderMode;
    glAPI->Rotated = __indirect_glRotated;
    glAPI->Rotatef = __indirect_glRotatef;
    glAPI->Scaled = __indirect_glScaled;
    glAPI->Scalef = __indirect_glScalef;
    glAPI->Scissor = __indirect_glScissor;
    glAPI->SelectBuffer = __indirect_glSelectBuffer;
    glAPI->ShadeModel = __indirect_glShadeModel;
    glAPI->StencilFunc = __indirect_glStencilFunc;
    glAPI->StencilMask = __indirect_glStencilMask;
    glAPI->StencilOp = __indirect_glStencilOp;
    glAPI->TexCoord1d = __indirect_glTexCoord1d;
    glAPI->TexCoord1dv = __indirect_glTexCoord1dv;
    glAPI->TexCoord1f = __indirect_glTexCoord1f;
    glAPI->TexCoord1fv = __indirect_glTexCoord1fv;
    glAPI->TexCoord1i = __indirect_glTexCoord1i;
    glAPI->TexCoord1iv = __indirect_glTexCoord1iv;
    glAPI->TexCoord1s = __indirect_glTexCoord1s;
    glAPI->TexCoord1sv = __indirect_glTexCoord1sv;
    glAPI->TexCoord2d = __indirect_glTexCoord2d;
    glAPI->TexCoord2dv = __indirect_glTexCoord2dv;
    glAPI->TexCoord2f = __indirect_glTexCoord2f;
    glAPI->TexCoord2fv = __indirect_glTexCoord2fv;
    glAPI->TexCoord2i = __indirect_glTexCoord2i;
    glAPI->TexCoord2iv = __indirect_glTexCoord2iv;
    glAPI->TexCoord2s = __indirect_glTexCoord2s;
    glAPI->TexCoord2sv = __indirect_glTexCoord2sv;
    glAPI->TexCoord3d = __indirect_glTexCoord3d;
    glAPI->TexCoord3dv = __indirect_glTexCoord3dv;
    glAPI->TexCoord3f = __indirect_glTexCoord3f;
    glAPI->TexCoord3fv = __indirect_glTexCoord3fv;
    glAPI->TexCoord3i = __indirect_glTexCoord3i;
    glAPI->TexCoord3iv = __indirect_glTexCoord3iv;
    glAPI->TexCoord3s = __indirect_glTexCoord3s;
    glAPI->TexCoord3sv = __indirect_glTexCoord3sv;
    glAPI->TexCoord4d = __indirect_glTexCoord4d;
    glAPI->TexCoord4dv = __indirect_glTexCoord4dv;
    glAPI->TexCoord4f = __indirect_glTexCoord4f;
    glAPI->TexCoord4fv = __indirect_glTexCoord4fv;
    glAPI->TexCoord4i = __indirect_glTexCoord4i;
    glAPI->TexCoord4iv = __indirect_glTexCoord4iv;
    glAPI->TexCoord4s = __indirect_glTexCoord4s;
    glAPI->TexCoord4sv = __indirect_glTexCoord4sv;
    glAPI->TexEnvf = __indirect_glTexEnvf;
    glAPI->TexEnvfv = __indirect_glTexEnvfv;
    glAPI->TexEnvi = __indirect_glTexEnvi;
    glAPI->TexEnviv = __indirect_glTexEnviv;
    glAPI->TexGend = __indirect_glTexGend;
    glAPI->TexGendv = __indirect_glTexGendv;
    glAPI->TexGenf = __indirect_glTexGenf;
    glAPI->TexGenfv = __indirect_glTexGenfv;
    glAPI->TexGeni = __indirect_glTexGeni;
    glAPI->TexGeniv = __indirect_glTexGeniv;
    glAPI->TexImage1D = __indirect_glTexImage1D;
    glAPI->TexImage2D = __indirect_glTexImage2D;
    glAPI->TexParameterf = __indirect_glTexParameterf;
    glAPI->TexParameterfv = __indirect_glTexParameterfv;
    glAPI->TexParameteri = __indirect_glTexParameteri;
    glAPI->TexParameteriv = __indirect_glTexParameteriv;
    glAPI->Translated = __indirect_glTranslated;
    glAPI->Translatef = __indirect_glTranslatef;
    glAPI->Vertex2d = __indirect_glVertex2d;
    glAPI->Vertex2dv = __indirect_glVertex2dv;
    glAPI->Vertex2f = __indirect_glVertex2f;
    glAPI->Vertex2fv = __indirect_glVertex2fv;
    glAPI->Vertex2i = __indirect_glVertex2i;
    glAPI->Vertex2iv = __indirect_glVertex2iv;
    glAPI->Vertex2s = __indirect_glVertex2s;
    glAPI->Vertex2sv = __indirect_glVertex2sv;
    glAPI->Vertex3d = __indirect_glVertex3d;
    glAPI->Vertex3dv = __indirect_glVertex3dv;
    glAPI->Vertex3f = __indirect_glVertex3f;
    glAPI->Vertex3fv = __indirect_glVertex3fv;
    glAPI->Vertex3i = __indirect_glVertex3i;
    glAPI->Vertex3iv = __indirect_glVertex3iv;
    glAPI->Vertex3s = __indirect_glVertex3s;
    glAPI->Vertex3sv = __indirect_glVertex3sv;
    glAPI->Vertex4d = __indirect_glVertex4d;
    glAPI->Vertex4dv = __indirect_glVertex4dv;
    glAPI->Vertex4f = __indirect_glVertex4f;
    glAPI->Vertex4fv = __indirect_glVertex4fv;
    glAPI->Vertex4i = __indirect_glVertex4i;
    glAPI->Vertex4iv = __indirect_glVertex4iv;
    glAPI->Vertex4s = __indirect_glVertex4s;
    glAPI->Vertex4sv = __indirect_glVertex4sv;
    glAPI->Viewport = __indirect_glViewport;

    /* 1.1 */

    glAPI->AreTexturesResident = __indirect_glAreTexturesResident;
    glAPI->ArrayElement = __indirect_glArrayElement;
    glAPI->BindTexture = __indirect_glBindTexture;
    glAPI->ColorPointer = __indirect_glColorPointer;
    glAPI->CopyTexImage1D = __indirect_glCopyTexImage1D;
    glAPI->CopyTexImage2D = __indirect_glCopyTexImage2D;
    glAPI->CopyTexSubImage1D = __indirect_glCopyTexSubImage1D;
    glAPI->CopyTexSubImage2D = __indirect_glCopyTexSubImage2D;
    glAPI->DeleteTextures = __indirect_glDeleteTextures;
    glAPI->DisableClientState = __indirect_glDisableClientState;
    glAPI->DrawArrays = __indirect_glDrawArrays;
    glAPI->DrawElements = __indirect_glDrawElements;
    glAPI->EdgeFlagPointer = __indirect_glEdgeFlagPointer;
    glAPI->EnableClientState = __indirect_glEnableClientState;
    glAPI->GenTextures = __indirect_glGenTextures;
    glAPI->GetPointerv = __indirect_glGetPointerv;
    glAPI->IndexPointer = __indirect_glIndexPointer;
    glAPI->Indexub = __indirect_glIndexub;
    glAPI->Indexubv = __indirect_glIndexubv;
    glAPI->InterleavedArrays = __indirect_glInterleavedArrays;
    glAPI->IsTexture = __indirect_glIsTexture;
    glAPI->NormalPointer = __indirect_glNormalPointer;
    glAPI->PolygonOffset = __indirect_glPolygonOffset;
    glAPI->PopClientAttrib = __indirect_glPopClientAttrib;
    glAPI->PrioritizeTextures = __indirect_glPrioritizeTextures;
    glAPI->PushClientAttrib = __indirect_glPushClientAttrib;
    glAPI->TexCoordPointer = __indirect_glTexCoordPointer;
    glAPI->TexSubImage1D = __indirect_glTexSubImage1D;
    glAPI->TexSubImage2D = __indirect_glTexSubImage2D;
    glAPI->VertexPointer = __indirect_glVertexPointer;

    /* 1.2 */

    glAPI->BlendColor = __indirect_glBlendColor;
    glAPI->BlendEquation = __indirect_glBlendEquation;
    glAPI->ColorSubTable = __indirect_glColorSubTable;
    glAPI->ColorTable = __indirect_glColorTable;
    glAPI->ColorTableParameterfv = __indirect_glColorTableParameterfv;
    glAPI->ColorTableParameteriv = __indirect_glColorTableParameteriv;
    glAPI->ConvolutionFilter1D = __indirect_glConvolutionFilter1D;
    glAPI->ConvolutionFilter2D = __indirect_glConvolutionFilter2D;
    glAPI->ConvolutionParameterf = __indirect_glConvolutionParameterf;
    glAPI->ConvolutionParameterfv = __indirect_glConvolutionParameterfv;
    glAPI->ConvolutionParameteri = __indirect_glConvolutionParameteri;
    glAPI->ConvolutionParameteriv = __indirect_glConvolutionParameteriv;
    glAPI->CopyColorSubTable = __indirect_glCopyColorSubTable;
    glAPI->CopyColorTable = __indirect_glCopyColorTable;
    glAPI->CopyConvolutionFilter1D = __indirect_glCopyConvolutionFilter1D;
    glAPI->CopyConvolutionFilter2D = __indirect_glCopyConvolutionFilter2D;
    glAPI->CopyTexSubImage3D = __indirect_glCopyTexSubImage3D;
    glAPI->DrawRangeElements = __indirect_glDrawRangeElements;
    glAPI->GetColorTable = __indirect_glGetColorTable;
    glAPI->GetColorTableParameterfv = __indirect_glGetColorTableParameterfv;
    glAPI->GetColorTableParameteriv = __indirect_glGetColorTableParameteriv;
    glAPI->GetConvolutionFilter = __indirect_glGetConvolutionFilter;
    glAPI->GetConvolutionParameterfv = __indirect_glGetConvolutionParameterfv;
    glAPI->GetConvolutionParameteriv = __indirect_glGetConvolutionParameteriv;
    glAPI->GetHistogram = __indirect_glGetHistogram;
    glAPI->GetHistogramParameterfv = __indirect_glGetHistogramParameterfv;
    glAPI->GetHistogramParameteriv = __indirect_glGetHistogramParameteriv;
    glAPI->GetMinmax = __indirect_glGetMinmax;
    glAPI->GetMinmaxParameterfv = __indirect_glGetMinmaxParameterfv;
    glAPI->GetMinmaxParameteriv = __indirect_glGetMinmaxParameteriv;
    glAPI->GetSeparableFilter = __indirect_glGetSeparableFilter;
    glAPI->Histogram = __indirect_glHistogram;
    glAPI->Minmax = __indirect_glMinmax;
    glAPI->ResetHistogram = __indirect_glResetHistogram;
    glAPI->ResetMinmax = __indirect_glResetMinmax;
    glAPI->SeparableFilter2D = __indirect_glSeparableFilter2D;
    glAPI->TexImage3D = __indirect_glTexImage3D;
    glAPI->TexSubImage3D = __indirect_glTexSubImage3D;

    /*   1. GL_ARB_multitexture */

    glAPI->ActiveTextureARB = __indirect_glActiveTextureARB;
    glAPI->ClientActiveTextureARB = __indirect_glClientActiveTextureARB;
    glAPI->MultiTexCoord1dARB = __indirect_glMultiTexCoord1dARB;
    glAPI->MultiTexCoord1dvARB = __indirect_glMultiTexCoord1dvARB;
    glAPI->MultiTexCoord1fARB = __indirect_glMultiTexCoord1fARB;
    glAPI->MultiTexCoord1fvARB = __indirect_glMultiTexCoord1fvARB;
    glAPI->MultiTexCoord1iARB = __indirect_glMultiTexCoord1iARB;
    glAPI->MultiTexCoord1ivARB = __indirect_glMultiTexCoord1ivARB;
    glAPI->MultiTexCoord1sARB = __indirect_glMultiTexCoord1sARB;
    glAPI->MultiTexCoord1svARB = __indirect_glMultiTexCoord1svARB;
    glAPI->MultiTexCoord2dARB = __indirect_glMultiTexCoord2dARB;
    glAPI->MultiTexCoord2dvARB = __indirect_glMultiTexCoord2dvARB;
    glAPI->MultiTexCoord2fARB = __indirect_glMultiTexCoord2fARB;
    glAPI->MultiTexCoord2fvARB = __indirect_glMultiTexCoord2fvARB;
    glAPI->MultiTexCoord2iARB = __indirect_glMultiTexCoord2iARB;
    glAPI->MultiTexCoord2ivARB = __indirect_glMultiTexCoord2ivARB;
    glAPI->MultiTexCoord2sARB = __indirect_glMultiTexCoord2sARB;
    glAPI->MultiTexCoord2svARB = __indirect_glMultiTexCoord2svARB;
    glAPI->MultiTexCoord3dARB = __indirect_glMultiTexCoord3dARB;
    glAPI->MultiTexCoord3dvARB = __indirect_glMultiTexCoord3dvARB;
    glAPI->MultiTexCoord3fARB = __indirect_glMultiTexCoord3fARB;
    glAPI->MultiTexCoord3fvARB = __indirect_glMultiTexCoord3fvARB;
    glAPI->MultiTexCoord3iARB = __indirect_glMultiTexCoord3iARB;
    glAPI->MultiTexCoord3ivARB = __indirect_glMultiTexCoord3ivARB;
    glAPI->MultiTexCoord3sARB = __indirect_glMultiTexCoord3sARB;
    glAPI->MultiTexCoord3svARB = __indirect_glMultiTexCoord3svARB;
    glAPI->MultiTexCoord4dARB = __indirect_glMultiTexCoord4dARB;
    glAPI->MultiTexCoord4dvARB = __indirect_glMultiTexCoord4dvARB;
    glAPI->MultiTexCoord4fARB = __indirect_glMultiTexCoord4fARB;
    glAPI->MultiTexCoord4fvARB = __indirect_glMultiTexCoord4fvARB;
    glAPI->MultiTexCoord4iARB = __indirect_glMultiTexCoord4iARB;
    glAPI->MultiTexCoord4ivARB = __indirect_glMultiTexCoord4ivARB;
    glAPI->MultiTexCoord4sARB = __indirect_glMultiTexCoord4sARB;
    glAPI->MultiTexCoord4svARB = __indirect_glMultiTexCoord4svARB;

    /*   3. GL_ARB_transpose_matrix */

    glAPI->LoadTransposeMatrixdARB = __indirect_glLoadTransposeMatrixdARB;
    glAPI->LoadTransposeMatrixfARB = __indirect_glLoadTransposeMatrixfARB;
    glAPI->MultTransposeMatrixdARB = __indirect_glMultTransposeMatrixdARB;
    glAPI->MultTransposeMatrixfARB = __indirect_glMultTransposeMatrixfARB;

    /*   5. GL_ARB_multisample */

    glAPI->SampleCoverageARB = __indirect_glSampleCoverageARB;

    /*  12. GL_ARB_texture_compression */

    glAPI->CompressedTexImage1DARB = __indirect_glCompressedTexImage1DARB;
    glAPI->CompressedTexImage2DARB = __indirect_glCompressedTexImage2DARB;
    glAPI->CompressedTexImage3DARB = __indirect_glCompressedTexImage3DARB;
    glAPI->CompressedTexSubImage1DARB = __indirect_glCompressedTexSubImage1DARB;
    glAPI->CompressedTexSubImage2DARB = __indirect_glCompressedTexSubImage2DARB;
    glAPI->CompressedTexSubImage3DARB = __indirect_glCompressedTexSubImage3DARB;
    glAPI->GetCompressedTexImageARB = __indirect_glGetCompressedTexImageARB;

    /*  26. GL_ARB_vertex_program */

    glAPI->DisableVertexAttribArrayARB = __indirect_glDisableVertexAttribArrayARB;
    glAPI->EnableVertexAttribArrayARB = __indirect_glEnableVertexAttribArrayARB;
    glAPI->GetProgramEnvParameterdvARB = __indirect_glGetProgramEnvParameterdvARB;
    glAPI->GetProgramEnvParameterfvARB = __indirect_glGetProgramEnvParameterfvARB;
    glAPI->GetProgramLocalParameterdvARB = __indirect_glGetProgramLocalParameterdvARB;
    glAPI->GetProgramLocalParameterfvARB = __indirect_glGetProgramLocalParameterfvARB;
    glAPI->GetProgramStringARB = __indirect_glGetProgramStringARB;
    glAPI->GetProgramivARB = __indirect_glGetProgramivARB;
    glAPI->GetVertexAttribdvARB = __indirect_glGetVertexAttribdvARB;
    glAPI->GetVertexAttribfvARB = __indirect_glGetVertexAttribfvARB;
    glAPI->GetVertexAttribivARB = __indirect_glGetVertexAttribivARB;
    glAPI->ProgramEnvParameter4dARB = __indirect_glProgramEnvParameter4dARB;
    glAPI->ProgramEnvParameter4dvARB = __indirect_glProgramEnvParameter4dvARB;
    glAPI->ProgramEnvParameter4fARB = __indirect_glProgramEnvParameter4fARB;
    glAPI->ProgramEnvParameter4fvARB = __indirect_glProgramEnvParameter4fvARB;
    glAPI->ProgramLocalParameter4dARB = __indirect_glProgramLocalParameter4dARB;
    glAPI->ProgramLocalParameter4dvARB = __indirect_glProgramLocalParameter4dvARB;
    glAPI->ProgramLocalParameter4fARB = __indirect_glProgramLocalParameter4fARB;
    glAPI->ProgramLocalParameter4fvARB = __indirect_glProgramLocalParameter4fvARB;
    glAPI->ProgramStringARB = __indirect_glProgramStringARB;
    glAPI->VertexAttrib1dARB = __indirect_glVertexAttrib1dARB;
    glAPI->VertexAttrib1dvARB = __indirect_glVertexAttrib1dvARB;
    glAPI->VertexAttrib1fARB = __indirect_glVertexAttrib1fARB;
    glAPI->VertexAttrib1fvARB = __indirect_glVertexAttrib1fvARB;
    glAPI->VertexAttrib1sARB = __indirect_glVertexAttrib1sARB;
    glAPI->VertexAttrib1svARB = __indirect_glVertexAttrib1svARB;
    glAPI->VertexAttrib2dARB = __indirect_glVertexAttrib2dARB;
    glAPI->VertexAttrib2dvARB = __indirect_glVertexAttrib2dvARB;
    glAPI->VertexAttrib2fARB = __indirect_glVertexAttrib2fARB;
    glAPI->VertexAttrib2fvARB = __indirect_glVertexAttrib2fvARB;
    glAPI->VertexAttrib2sARB = __indirect_glVertexAttrib2sARB;
    glAPI->VertexAttrib2svARB = __indirect_glVertexAttrib2svARB;
    glAPI->VertexAttrib3dARB = __indirect_glVertexAttrib3dARB;
    glAPI->VertexAttrib3dvARB = __indirect_glVertexAttrib3dvARB;
    glAPI->VertexAttrib3fARB = __indirect_glVertexAttrib3fARB;
    glAPI->VertexAttrib3fvARB = __indirect_glVertexAttrib3fvARB;
    glAPI->VertexAttrib3sARB = __indirect_glVertexAttrib3sARB;
    glAPI->VertexAttrib3svARB = __indirect_glVertexAttrib3svARB;
    glAPI->VertexAttrib4NbvARB = __indirect_glVertexAttrib4NbvARB;
    glAPI->VertexAttrib4NivARB = __indirect_glVertexAttrib4NivARB;
    glAPI->VertexAttrib4NsvARB = __indirect_glVertexAttrib4NsvARB;
    glAPI->VertexAttrib4NubARB = __indirect_glVertexAttrib4NubARB;
    glAPI->VertexAttrib4NubvARB = __indirect_glVertexAttrib4NubvARB;
    glAPI->VertexAttrib4NuivARB = __indirect_glVertexAttrib4NuivARB;
    glAPI->VertexAttrib4NusvARB = __indirect_glVertexAttrib4NusvARB;
    glAPI->VertexAttrib4bvARB = __indirect_glVertexAttrib4bvARB;
    glAPI->VertexAttrib4dARB = __indirect_glVertexAttrib4dARB;
    glAPI->VertexAttrib4dvARB = __indirect_glVertexAttrib4dvARB;
    glAPI->VertexAttrib4fARB = __indirect_glVertexAttrib4fARB;
    glAPI->VertexAttrib4fvARB = __indirect_glVertexAttrib4fvARB;
    glAPI->VertexAttrib4ivARB = __indirect_glVertexAttrib4ivARB;
    glAPI->VertexAttrib4sARB = __indirect_glVertexAttrib4sARB;
    glAPI->VertexAttrib4svARB = __indirect_glVertexAttrib4svARB;
    glAPI->VertexAttrib4ubvARB = __indirect_glVertexAttrib4ubvARB;
    glAPI->VertexAttrib4uivARB = __indirect_glVertexAttrib4uivARB;
    glAPI->VertexAttrib4usvARB = __indirect_glVertexAttrib4usvARB;
    glAPI->VertexAttribPointerARB = __indirect_glVertexAttribPointerARB;

    /*  29. GL_ARB_occlusion_query */

    glAPI->BeginQueryARB = __indirect_glBeginQueryARB;
    glAPI->DeleteQueriesARB = __indirect_glDeleteQueriesARB;
    glAPI->EndQueryARB = __indirect_glEndQueryARB;
    glAPI->GenQueriesARB = __indirect_glGenQueriesARB;
    glAPI->GetQueryObjectivARB = __indirect_glGetQueryObjectivARB;
    glAPI->GetQueryObjectuivARB = __indirect_glGetQueryObjectuivARB;
    glAPI->GetQueryivARB = __indirect_glGetQueryivARB;
    glAPI->IsQueryARB = __indirect_glIsQueryARB;

    /*  37. GL_ARB_draw_buffers */

    glAPI->DrawBuffersARB = __indirect_glDrawBuffersARB;

    /*  25. GL_SGIS_multisample */

    glAPI->SampleMaskSGIS = __indirect_glSampleMaskSGIS;
    glAPI->SamplePatternSGIS = __indirect_glSamplePatternSGIS;

    /*  30. GL_EXT_vertex_array */

    glAPI->ColorPointerEXT = __indirect_glColorPointerEXT;
    glAPI->EdgeFlagPointerEXT = __indirect_glEdgeFlagPointerEXT;
    glAPI->IndexPointerEXT = __indirect_glIndexPointerEXT;
    glAPI->NormalPointerEXT = __indirect_glNormalPointerEXT;
    glAPI->TexCoordPointerEXT = __indirect_glTexCoordPointerEXT;
    glAPI->VertexPointerEXT = __indirect_glVertexPointerEXT;

    /*  54. GL_EXT_point_parameters */

    glAPI->PointParameterfEXT = __indirect_glPointParameterfEXT;
    glAPI->PointParameterfvEXT = __indirect_glPointParameterfvEXT;

    /* 145. GL_EXT_secondary_color */

    glAPI->SecondaryColor3bEXT = __indirect_glSecondaryColor3bEXT;
    glAPI->SecondaryColor3bvEXT = __indirect_glSecondaryColor3bvEXT;
    glAPI->SecondaryColor3dEXT = __indirect_glSecondaryColor3dEXT;
    glAPI->SecondaryColor3dvEXT = __indirect_glSecondaryColor3dvEXT;
    glAPI->SecondaryColor3fEXT = __indirect_glSecondaryColor3fEXT;
    glAPI->SecondaryColor3fvEXT = __indirect_glSecondaryColor3fvEXT;
    glAPI->SecondaryColor3iEXT = __indirect_glSecondaryColor3iEXT;
    glAPI->SecondaryColor3ivEXT = __indirect_glSecondaryColor3ivEXT;
    glAPI->SecondaryColor3sEXT = __indirect_glSecondaryColor3sEXT;
    glAPI->SecondaryColor3svEXT = __indirect_glSecondaryColor3svEXT;
    glAPI->SecondaryColor3ubEXT = __indirect_glSecondaryColor3ubEXT;
    glAPI->SecondaryColor3ubvEXT = __indirect_glSecondaryColor3ubvEXT;
    glAPI->SecondaryColor3uiEXT = __indirect_glSecondaryColor3uiEXT;
    glAPI->SecondaryColor3uivEXT = __indirect_glSecondaryColor3uivEXT;
    glAPI->SecondaryColor3usEXT = __indirect_glSecondaryColor3usEXT;
    glAPI->SecondaryColor3usvEXT = __indirect_glSecondaryColor3usvEXT;
    glAPI->SecondaryColorPointerEXT = __indirect_glSecondaryColorPointerEXT;

    /* 148. GL_EXT_multi_draw_arrays */

    glAPI->MultiDrawArraysEXT = __indirect_glMultiDrawArraysEXT;
    glAPI->MultiDrawElementsEXT = __indirect_glMultiDrawElementsEXT;

    /* 149. GL_EXT_fog_coord */

    glAPI->FogCoordPointerEXT = __indirect_glFogCoordPointerEXT;
    glAPI->FogCoorddEXT = __indirect_glFogCoorddEXT;
    glAPI->FogCoorddvEXT = __indirect_glFogCoorddvEXT;
    glAPI->FogCoordfEXT = __indirect_glFogCoordfEXT;
    glAPI->FogCoordfvEXT = __indirect_glFogCoordfvEXT;

    /* 173. GL_EXT_blend_func_separate */

    glAPI->BlendFuncSeparateEXT = __indirect_glBlendFuncSeparateEXT;

    /* 197. GL_MESA_window_pos */

    glAPI->WindowPos2dMESA = __indirect_glWindowPos2dMESA;
    glAPI->WindowPos2dvMESA = __indirect_glWindowPos2dvMESA;
    glAPI->WindowPos2fMESA = __indirect_glWindowPos2fMESA;
    glAPI->WindowPos2fvMESA = __indirect_glWindowPos2fvMESA;
    glAPI->WindowPos2iMESA = __indirect_glWindowPos2iMESA;
    glAPI->WindowPos2ivMESA = __indirect_glWindowPos2ivMESA;
    glAPI->WindowPos2sMESA = __indirect_glWindowPos2sMESA;
    glAPI->WindowPos2svMESA = __indirect_glWindowPos2svMESA;
    glAPI->WindowPos3dMESA = __indirect_glWindowPos3dMESA;
    glAPI->WindowPos3dvMESA = __indirect_glWindowPos3dvMESA;
    glAPI->WindowPos3fMESA = __indirect_glWindowPos3fMESA;
    glAPI->WindowPos3fvMESA = __indirect_glWindowPos3fvMESA;
    glAPI->WindowPos3iMESA = __indirect_glWindowPos3iMESA;
    glAPI->WindowPos3ivMESA = __indirect_glWindowPos3ivMESA;
    glAPI->WindowPos3sMESA = __indirect_glWindowPos3sMESA;
    glAPI->WindowPos3svMESA = __indirect_glWindowPos3svMESA;

    /* 233. GL_NV_vertex_program */

    glAPI->AreProgramsResidentNV = __indirect_glAreProgramsResidentNV;
    glAPI->BindProgramNV = __indirect_glBindProgramNV;
    glAPI->DeleteProgramsNV = __indirect_glDeleteProgramsNV;
    glAPI->ExecuteProgramNV = __indirect_glExecuteProgramNV;
    glAPI->GenProgramsNV = __indirect_glGenProgramsNV;
    glAPI->GetProgramParameterdvNV = __indirect_glGetProgramParameterdvNV;
    glAPI->GetProgramParameterfvNV = __indirect_glGetProgramParameterfvNV;
    glAPI->GetProgramStringNV = __indirect_glGetProgramStringNV;
    glAPI->GetProgramivNV = __indirect_glGetProgramivNV;
    glAPI->GetTrackMatrixivNV = __indirect_glGetTrackMatrixivNV;
    glAPI->GetVertexAttribPointervNV = __indirect_glGetVertexAttribPointervNV;
    glAPI->GetVertexAttribdvNV = __indirect_glGetVertexAttribdvNV;
    glAPI->GetVertexAttribfvNV = __indirect_glGetVertexAttribfvNV;
    glAPI->GetVertexAttribivNV = __indirect_glGetVertexAttribivNV;
    glAPI->IsProgramNV = __indirect_glIsProgramNV;
    glAPI->LoadProgramNV = __indirect_glLoadProgramNV;
    glAPI->ProgramParameters4dvNV = __indirect_glProgramParameters4dvNV;
    glAPI->ProgramParameters4fvNV = __indirect_glProgramParameters4fvNV;
    glAPI->RequestResidentProgramsNV = __indirect_glRequestResidentProgramsNV;
    glAPI->TrackMatrixNV = __indirect_glTrackMatrixNV;
    glAPI->VertexAttrib1dNV = __indirect_glVertexAttrib1dNV;
    glAPI->VertexAttrib1dvNV = __indirect_glVertexAttrib1dvNV;
    glAPI->VertexAttrib1fNV = __indirect_glVertexAttrib1fNV;
    glAPI->VertexAttrib1fvNV = __indirect_glVertexAttrib1fvNV;
    glAPI->VertexAttrib1sNV = __indirect_glVertexAttrib1sNV;
    glAPI->VertexAttrib1svNV = __indirect_glVertexAttrib1svNV;
    glAPI->VertexAttrib2dNV = __indirect_glVertexAttrib2dNV;
    glAPI->VertexAttrib2dvNV = __indirect_glVertexAttrib2dvNV;
    glAPI->VertexAttrib2fNV = __indirect_glVertexAttrib2fNV;
    glAPI->VertexAttrib2fvNV = __indirect_glVertexAttrib2fvNV;
    glAPI->VertexAttrib2sNV = __indirect_glVertexAttrib2sNV;
    glAPI->VertexAttrib2svNV = __indirect_glVertexAttrib2svNV;
    glAPI->VertexAttrib3dNV = __indirect_glVertexAttrib3dNV;
    glAPI->VertexAttrib3dvNV = __indirect_glVertexAttrib3dvNV;
    glAPI->VertexAttrib3fNV = __indirect_glVertexAttrib3fNV;
    glAPI->VertexAttrib3fvNV = __indirect_glVertexAttrib3fvNV;
    glAPI->VertexAttrib3sNV = __indirect_glVertexAttrib3sNV;
    glAPI->VertexAttrib3svNV = __indirect_glVertexAttrib3svNV;
    glAPI->VertexAttrib4dNV = __indirect_glVertexAttrib4dNV;
    glAPI->VertexAttrib4dvNV = __indirect_glVertexAttrib4dvNV;
    glAPI->VertexAttrib4fNV = __indirect_glVertexAttrib4fNV;
    glAPI->VertexAttrib4fvNV = __indirect_glVertexAttrib4fvNV;
    glAPI->VertexAttrib4sNV = __indirect_glVertexAttrib4sNV;
    glAPI->VertexAttrib4svNV = __indirect_glVertexAttrib4svNV;
    glAPI->VertexAttrib4ubNV = __indirect_glVertexAttrib4ubNV;
    glAPI->VertexAttrib4ubvNV = __indirect_glVertexAttrib4ubvNV;
    glAPI->VertexAttribPointerNV = __indirect_glVertexAttribPointerNV;
    glAPI->VertexAttribs1dvNV = __indirect_glVertexAttribs1dvNV;
    glAPI->VertexAttribs1fvNV = __indirect_glVertexAttribs1fvNV;
    glAPI->VertexAttribs1svNV = __indirect_glVertexAttribs1svNV;
    glAPI->VertexAttribs2dvNV = __indirect_glVertexAttribs2dvNV;
    glAPI->VertexAttribs2fvNV = __indirect_glVertexAttribs2fvNV;
    glAPI->VertexAttribs2svNV = __indirect_glVertexAttribs2svNV;
    glAPI->VertexAttribs3dvNV = __indirect_glVertexAttribs3dvNV;
    glAPI->VertexAttribs3fvNV = __indirect_glVertexAttribs3fvNV;
    glAPI->VertexAttribs3svNV = __indirect_glVertexAttribs3svNV;
    glAPI->VertexAttribs4dvNV = __indirect_glVertexAttribs4dvNV;
    glAPI->VertexAttribs4fvNV = __indirect_glVertexAttribs4fvNV;
    glAPI->VertexAttribs4svNV = __indirect_glVertexAttribs4svNV;
    glAPI->VertexAttribs4ubvNV = __indirect_glVertexAttribs4ubvNV;

    /* 262. GL_NV_point_sprite */

    glAPI->PointParameteriNV = __indirect_glPointParameteriNV;
    glAPI->PointParameterivNV = __indirect_glPointParameterivNV;

    /* 268. GL_EXT_stencil_two_side */

    glAPI->ActiveStencilFaceEXT = __indirect_glActiveStencilFaceEXT;

    /* 282. GL_NV_fragment_program */

    glAPI->GetProgramNamedParameterdvNV = __indirect_glGetProgramNamedParameterdvNV;
    glAPI->GetProgramNamedParameterfvNV = __indirect_glGetProgramNamedParameterfvNV;
    glAPI->ProgramNamedParameter4dNV = __indirect_glProgramNamedParameter4dNV;
    glAPI->ProgramNamedParameter4dvNV = __indirect_glProgramNamedParameter4dvNV;
    glAPI->ProgramNamedParameter4fNV = __indirect_glProgramNamedParameter4fNV;
    glAPI->ProgramNamedParameter4fvNV = __indirect_glProgramNamedParameter4fvNV;

    /* 299. GL_EXT_blend_equation_separate */

    glAPI->BlendEquationSeparateEXT = __indirect_glBlendEquationSeparateEXT;

    /* 310. GL_EXT_framebuffer_object */

    glAPI->BindFramebufferEXT = __indirect_glBindFramebufferEXT;
    glAPI->BindRenderbufferEXT = __indirect_glBindRenderbufferEXT;
    glAPI->CheckFramebufferStatusEXT = __indirect_glCheckFramebufferStatusEXT;
    glAPI->DeleteFramebuffersEXT = __indirect_glDeleteFramebuffersEXT;
    glAPI->DeleteRenderbuffersEXT = __indirect_glDeleteRenderbuffersEXT;
    glAPI->FramebufferRenderbufferEXT = __indirect_glFramebufferRenderbufferEXT;
    glAPI->FramebufferTexture1DEXT = __indirect_glFramebufferTexture1DEXT;
    glAPI->FramebufferTexture2DEXT = __indirect_glFramebufferTexture2DEXT;
    glAPI->FramebufferTexture3DEXT = __indirect_glFramebufferTexture3DEXT;
    glAPI->GenFramebuffersEXT = __indirect_glGenFramebuffersEXT;
    glAPI->GenRenderbuffersEXT = __indirect_glGenRenderbuffersEXT;
    glAPI->GenerateMipmapEXT = __indirect_glGenerateMipmapEXT;
    glAPI->GetFramebufferAttachmentParameterivEXT = __indirect_glGetFramebufferAttachmentParameterivEXT;
    glAPI->GetRenderbufferParameterivEXT = __indirect_glGetRenderbufferParameterivEXT;
    glAPI->IsFramebufferEXT = __indirect_glIsFramebufferEXT;
    glAPI->IsRenderbufferEXT = __indirect_glIsRenderbufferEXT;
    glAPI->RenderbufferStorageEXT = __indirect_glRenderbufferStorageEXT;

    return glAPI;
}

