/* $XFree86: xc/lib/GL/glx/indirect_init.c,v 1.9 2004/01/28 18:11:41 alanh Exp $ */
/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <kevin@precisioninsight.com>
 *   Brian Paul <brian@precisioninsight.com>
 */

#include "indirect_init.h"
#include "indirect.h"
#include "glapi.h"


/*
** No-op function
*/
static int NoOp(void)
{
    return 0;
}

/**
 * \name Vertex array pointer bridge functions
 *
 * When EXT_vertex_array was moved into the core GL spec, the \c count
 * parameter was lost.  This libGL really only wants to implement the GL 1.1
 * version, but we need to support applications that were written to the old
 * interface.  These bridge functions are part of the glue that makes this
 * happen.
 */
/*@{*/
static void ColorPointerEXT(GLint size, GLenum type, GLsizei stride,
			    GLsizei count, const GLvoid * pointer )
{
    (void) count; __indirect_glColorPointer( size, type, stride, pointer );
}

static void EdgeFlagPointerEXT(GLsizei stride,
			       GLsizei count, const GLboolean * pointer )
{
    (void) count; __indirect_glEdgeFlagPointer( stride, pointer );
}

static void IndexPointerEXT(GLenum type, GLsizei stride,
			    GLsizei count, const GLvoid * pointer )
{
    (void) count; __indirect_glIndexPointer( type, stride, pointer );
}

static void NormalPointerEXT(GLenum type, GLsizei stride, GLsizei count,
			     const GLvoid * pointer )
{
    (void) count; __indirect_glNormalPointer( type, stride, pointer );
}

static void TexCoordPointerEXT(GLint size, GLenum type, GLsizei stride,
			       GLsizei count, const GLvoid * pointer )
{
    (void) count; __indirect_glTexCoordPointer( size, type, stride, pointer );
}

static void VertexPointerEXT(GLint size, GLenum type, GLsizei stride,
			    GLsizei count, const GLvoid * pointer )
{
    (void) count; __indirect_glVertexPointer( size, type, stride, pointer );
}
/*@}*/


__GLapi *__glXNewIndirectAPI(void)
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
    glAPI->Accum = __indirect_glAccum;
    glAPI->AlphaFunc = __indirect_glAlphaFunc;
    glAPI->AreTexturesResident = __indirect_glAreTexturesResident;
    glAPI->ArrayElement = __indirect_glArrayElement;
    glAPI->Begin = __indirect_glBegin;
    glAPI->BindTexture = __indirect_glBindTexture;
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
    glAPI->ColorPointer = __indirect_glColorPointer;
    glAPI->CopyPixels = __indirect_glCopyPixels;
    glAPI->CopyTexImage1D = __indirect_glCopyTexImage1D;
    glAPI->CopyTexImage2D = __indirect_glCopyTexImage2D;
    glAPI->CopyTexSubImage1D = __indirect_glCopyTexSubImage1D;
    glAPI->CopyTexSubImage2D = __indirect_glCopyTexSubImage2D;
    glAPI->CullFace = __indirect_glCullFace;
    glAPI->DeleteLists = __indirect_glDeleteLists;
    glAPI->DeleteTextures = __indirect_glDeleteTextures;
    glAPI->DepthFunc = __indirect_glDepthFunc;
    glAPI->DepthMask = __indirect_glDepthMask;
    glAPI->DepthRange = __indirect_glDepthRange;
    glAPI->Disable = __indirect_glDisable;
    glAPI->DisableClientState = __indirect_glDisableClientState;
    glAPI->DrawArrays = __indirect_glDrawArrays;
    glAPI->DrawBuffer = __indirect_glDrawBuffer;
    glAPI->DrawElements = __indirect_glDrawElements;
    glAPI->DrawPixels = __indirect_glDrawPixels;
    glAPI->DrawRangeElements = __indirect_glDrawRangeElements;
    glAPI->EdgeFlag = __indirect_glEdgeFlag;
    glAPI->EdgeFlagPointer = __indirect_glEdgeFlagPointer;
    glAPI->EdgeFlagv = __indirect_glEdgeFlagv;
    glAPI->Enable = __indirect_glEnable;
    glAPI->EnableClientState = __indirect_glEnableClientState;
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
    glAPI->GenTextures = __indirect_glGenTextures;
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
    glAPI->GetPointerv = __indirect_glGetPointerv;
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
    glAPI->IndexPointer = __indirect_glIndexPointer;
    glAPI->Indexd = __indirect_glIndexd;
    glAPI->Indexdv = __indirect_glIndexdv;
    glAPI->Indexf = __indirect_glIndexf;
    glAPI->Indexfv = __indirect_glIndexfv;
    glAPI->Indexi = __indirect_glIndexi;
    glAPI->Indexiv = __indirect_glIndexiv;
    glAPI->Indexs = __indirect_glIndexs;
    glAPI->Indexsv = __indirect_glIndexsv;
    glAPI->Indexub = __indirect_glIndexub;
    glAPI->Indexubv = __indirect_glIndexubv;
    glAPI->InitNames = __indirect_glInitNames;
    glAPI->InterleavedArrays = __indirect_glInterleavedArrays;
    glAPI->IsEnabled = __indirect_glIsEnabled;
    glAPI->IsList = __indirect_glIsList;
    glAPI->IsTexture = __indirect_glIsTexture;
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
    glAPI->NormalPointer = __indirect_glNormalPointer;
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
    glAPI->PolygonOffset = __indirect_glPolygonOffset;
    glAPI->PolygonStipple = __indirect_glPolygonStipple;
    glAPI->PopAttrib = __indirect_glPopAttrib;
    glAPI->PopClientAttrib = __indirect_glPopClientAttrib;
    glAPI->PopMatrix = __indirect_glPopMatrix;
    glAPI->PopName = __indirect_glPopName;
    glAPI->PrioritizeTextures = __indirect_glPrioritizeTextures;
    glAPI->PushAttrib = __indirect_glPushAttrib;
    glAPI->PushClientAttrib = __indirect_glPushClientAttrib;
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
    glAPI->TexCoordPointer = __indirect_glTexCoordPointer;
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
    glAPI->TexSubImage1D = __indirect_glTexSubImage1D;
    glAPI->TexSubImage2D = __indirect_glTexSubImage2D;
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
    glAPI->VertexPointer = __indirect_glVertexPointer;
    glAPI->Viewport = __indirect_glViewport;

    /* 1.2 */
    glAPI->CopyTexSubImage3D = __indirect_glCopyTexSubImage3D;
    glAPI->DrawRangeElements = __indirect_glDrawRangeElements;
    glAPI->TexImage3D = __indirect_glTexImage3D;
    glAPI->TexSubImage3D = __indirect_glTexSubImage3D;

    /* OpenGL 1.2  GL_ARB_imaging */
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

    /* 1.4 */
    glAPI->MultiDrawArraysEXT = __indirect_glMultiDrawArrays;
    glAPI->MultiDrawElementsEXT = __indirect_glMultiDrawElements;

    /* ARB 1. GL_ARB_multitexture */
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

    /* ARB 3. GL_ARB_transpose_matrix */
    glAPI->LoadTransposeMatrixdARB = __indirect_glLoadTransposeMatrixdARB;
    glAPI->LoadTransposeMatrixfARB = __indirect_glLoadTransposeMatrixfARB;
    glAPI->MultTransposeMatrixdARB = __indirect_glMultTransposeMatrixdARB;
    glAPI->MultTransposeMatrixfARB = __indirect_glMultTransposeMatrixfARB;

    /* ARB 5. GL_ARB_multisample */
    glAPI->SampleCoverageARB = __indirect_glSampleCoverageARB;

    /* ARB 12. GL_ARB_texture_compression / 1.3 */
    glAPI->GetCompressedTexImageARB = __indirect_glGetCompressedTexImage;
    glAPI->CompressedTexImage1DARB = __indirect_glCompressedTexImage1D;
    glAPI->CompressedTexImage2DARB = __indirect_glCompressedTexImage2D;
    glAPI->CompressedTexImage3DARB = __indirect_glCompressedTexImage3D;
    glAPI->CompressedTexSubImage1DARB = __indirect_glCompressedTexSubImage1D;
    glAPI->CompressedTexSubImage2DARB = __indirect_glCompressedTexSubImage2D;
    glAPI->CompressedTexSubImage3DARB = __indirect_glCompressedTexSubImage3D;

    /* ARB 14. GL_ARB_point_parameters */
    glAPI->PointParameterfEXT = __indirect_glPointParameterfARB;
    glAPI->PointParameterfvEXT = __indirect_glPointParameterfvARB;

    /* ARB 15. GL_ARB_window_pos */
    glAPI->WindowPos2dMESA = __indirect_glWindowPos2dARB;
    glAPI->WindowPos2iMESA = __indirect_glWindowPos2iARB;
    glAPI->WindowPos2fMESA = __indirect_glWindowPos2fARB;
    glAPI->WindowPos2iMESA = __indirect_glWindowPos2iARB;
    glAPI->WindowPos2sMESA = __indirect_glWindowPos2sARB;
    glAPI->WindowPos2dvMESA = __indirect_glWindowPos2dvARB;
    glAPI->WindowPos2fvMESA = __indirect_glWindowPos2fvARB;
    glAPI->WindowPos2ivMESA = __indirect_glWindowPos2ivARB;
    glAPI->WindowPos2svMESA = __indirect_glWindowPos2svARB;
    glAPI->WindowPos3dMESA = __indirect_glWindowPos3dARB;
    glAPI->WindowPos3fMESA = __indirect_glWindowPos3fARB;
    glAPI->WindowPos3iMESA = __indirect_glWindowPos3iARB;
    glAPI->WindowPos3sMESA = __indirect_glWindowPos3sARB;
    glAPI->WindowPos3dvMESA = __indirect_glWindowPos3dvARB;
    glAPI->WindowPos3fvMESA = __indirect_glWindowPos3fvARB;
    glAPI->WindowPos3ivMESA = __indirect_glWindowPos3ivARB;
    glAPI->WindowPos3svMESA = __indirect_glWindowPos3svARB;

    /* 25. GL_SGIS_multisample */
    glAPI->SampleMaskSGIS = __indirect_glSampleMaskSGIS;
    glAPI->SamplePatternSGIS = __indirect_glSamplePatternSGIS;

    /* 30. GL_EXT_vertex_array */
    glAPI->ColorPointerEXT    = ColorPointerEXT;
    glAPI->EdgeFlagPointerEXT = EdgeFlagPointerEXT;
    glAPI->IndexPointerEXT    = IndexPointerEXT;
    glAPI->NormalPointerEXT   = NormalPointerEXT;
    glAPI->TexCoordPointerEXT = TexCoordPointerEXT;
    glAPI->VertexPointerEXT   = VertexPointerEXT;

    /* 145. GL_EXT_secondary_color / GL 1.4 */
    glAPI->SecondaryColor3bEXT       = __indirect_glSecondaryColor3b;
    glAPI->SecondaryColor3bvEXT      = __indirect_glSecondaryColor3bv;
    glAPI->SecondaryColor3sEXT       = __indirect_glSecondaryColor3s;
    glAPI->SecondaryColor3svEXT      = __indirect_glSecondaryColor3sv;
    glAPI->SecondaryColor3iEXT       = __indirect_glSecondaryColor3i;
    glAPI->SecondaryColor3ivEXT      = __indirect_glSecondaryColor3iv;
    glAPI->SecondaryColor3ubEXT      = __indirect_glSecondaryColor3ub;
    glAPI->SecondaryColor3ubvEXT     = __indirect_glSecondaryColor3ubv;
    glAPI->SecondaryColor3usEXT      = __indirect_glSecondaryColor3us;
    glAPI->SecondaryColor3usvEXT     = __indirect_glSecondaryColor3usv;
    glAPI->SecondaryColor3uiEXT      = __indirect_glSecondaryColor3ui;
    glAPI->SecondaryColor3uivEXT     = __indirect_glSecondaryColor3uiv;
    glAPI->SecondaryColor3fEXT       = __indirect_glSecondaryColor3f;
    glAPI->SecondaryColor3fvEXT      = __indirect_glSecondaryColor3fv;
    glAPI->SecondaryColor3dEXT       = __indirect_glSecondaryColor3d;
    glAPI->SecondaryColor3dvEXT      = __indirect_glSecondaryColor3dv;
    glAPI->SecondaryColorPointerEXT  = __indirect_glSecondaryColorPointer;

    /* 149. GL_EXT_fog_coord / GL 1.4 */
    glAPI->FogCoordfEXT       = __indirect_glFogCoordf;
    glAPI->FogCoordfvEXT      = __indirect_glFogCoordfv;
    glAPI->FogCoorddEXT       = __indirect_glFogCoordd;
    glAPI->FogCoorddvEXT      = __indirect_glFogCoorddv;
    glAPI->FogCoordPointerEXT = __indirect_glFogCoordPointer;

    /* 173. GL_EXT_blend_func_separate / GL 1.4 */
    glAPI->BlendFuncSeparateEXT = __indirect_glBlendFuncSeparate;

    /* 262. GL_NV_point_sprite / GL 1.4 */
    glAPI->PointParameteriNV = __indirect_glPointParameteri;
    glAPI->PointParameterivNV = __indirect_glPointParameteriv;

    /* 268. GL_EXT_stencil_two_side */
    glAPI->ActiveStencilFaceEXT = __indirect_glActiveStencilFaceEXT;

    return glAPI;
}
