/* $Id: glapitable.h,v 1.8 2000/02/03 09:35:17 joukj Exp $ */

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


/*
 * This file is not specific to Mesa.  It defines a dispatch table
 * which could be used by any number of OpenGL implementations.
 * It's designed to be gl.h-independent as well.  That is, it does
 * not depend on any particular extensions being defined in the gl.h
 * header.  We #define extension symbols (like _GLAPI_EXT_blend_color)
 * to determine which entry points to compile.  Clients of this dispatcher
 * (like Mesa) can #ifdef test these symbols to know how to fill in the
 * table.
 */


#ifndef _GLAPI_TABLE_H
#define _GLAPI_TABLE_H


#include "GL/gl.h"


#define _GLAPI_EXTRA_SLOTS 1000


/*
 * This struct contains pointers for all the GL API entrypoints
 * plus some reserved slots for dynamic extensions.
 *
 * Strictly speaking, this struct isn't needed if we have assembly
 * language entrypoint functions since no knowledge of function
 * arguments is needed.
 *
 * This struct may be replaced by an automatically-generated struct
 * using the spec files in the OpenGL SI.
 */
struct _glapi_table
{
   void (*Dummy)(void);

   /*
    * OpenGL 1.0
    */
   void (*Accum)(GLenum, GLfloat);
   void (*AlphaFunc)(GLenum, GLclampf);
   void (*Begin)(GLenum);
   void (*Bitmap)(GLsizei, GLsizei, GLfloat, GLfloat, GLfloat, GLfloat, const GLubyte *);
   void (*BlendFunc)(GLenum, GLenum);
   void (*CallList)(GLuint list);
   void (*CallLists)(GLsizei, GLenum, const GLvoid *);
   void (*Clear)(GLbitfield);
   void (*ClearAccum)(GLfloat, GLfloat, GLfloat, GLfloat);
   void (*ClearColor)(GLclampf, GLclampf, GLclampf, GLclampf);
   void (*ClearDepth)(GLclampd);
   void (*ClearIndex)(GLfloat);
   void (*ClearStencil)(GLint);
   void (*ClipPlane)(GLenum, const GLdouble *);
   void (*Color3b)(GLbyte, GLbyte, GLbyte);
   void (*Color3bv)(const GLbyte *);
   void (*Color3d)(GLdouble, GLdouble, GLdouble);
   void (*Color3dv)(const GLdouble *);
   void (*Color3f)(GLfloat, GLfloat, GLfloat);
   void (*Color3fv)(const GLfloat *);
   void (*Color3i)(GLint, GLint, GLint);
   void (*Color3iv)(const GLint *);
   void (*Color3s)(GLshort, GLshort, GLshort);
   void (*Color3sv)(const GLshort *);
   void (*Color3ub)(GLubyte, GLubyte, GLubyte);
   void (*Color3ubv)(const GLubyte *);
   void (*Color3ui)(GLuint, GLuint, GLuint);
   void (*Color3uiv)(const GLuint *);
   void (*Color3us)(GLushort, GLushort, GLushort);
   void (*Color3usv)(const GLushort *);
   void (*Color4b)(GLbyte, GLbyte, GLbyte, GLbyte);
   void (*Color4bv)(const GLbyte *);
   void (*Color4d)(GLdouble, GLdouble, GLdouble, GLdouble);
   void (*Color4dv)(const GLdouble *);
   void (*Color4f)(GLfloat, GLfloat, GLfloat, GLfloat);
   void (*Color4fv)(const GLfloat *);
   void (*Color4i)(GLint, GLint, GLint, GLint);
   void (*Color4iv)(const GLint *);
   void (*Color4s)(GLshort, GLshort, GLshort, GLshort);
   void (*Color4sv)(const GLshort *);
   void (*Color4ub)(GLubyte, GLubyte, GLubyte, GLubyte);
   void (*Color4ubv)(const GLubyte *);
   void (*Color4ui)(GLuint, GLuint, GLuint, GLuint);
   void (*Color4uiv)(const GLuint *);
   void (*Color4us)(GLushort, GLushort, GLushort, GLushort);
   void (*Color4usv)(const GLushort *);
   void (*ColorMask)(GLboolean, GLboolean, GLboolean, GLboolean);
   void (*ColorMaterial)(GLenum, GLenum);
   void (*CopyPixels)(GLint, GLint, GLsizei, GLsizei, GLenum);
   void (*CullFace)(GLenum);
   void (*DeleteLists)(GLuint, GLsizei);
   void (*DepthFunc)(GLenum);
   void (*DepthMask)(GLboolean);
   void (*DepthRange)(GLclampd, GLclampd);
   void (*Disable)(GLenum);
   void (*DrawBuffer)(GLenum);
   void (*DrawPixels)(GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
   void (*EdgeFlag)(GLboolean);
   void (*EdgeFlagv)(const GLboolean *);
   void (*Enable)(GLenum);
   void (*End)(void);
   void (*EndList)(void);
   void (*EvalCoord1d)(GLdouble);
   void (*EvalCoord1dv)(const GLdouble *);
   void (*EvalCoord1f)(GLfloat);
   void (*EvalCoord1fv)(const GLfloat *);
   void (*EvalCoord2d)(GLdouble u, GLdouble);
   void (*EvalCoord2dv)(const GLdouble *);
   void (*EvalCoord2f)(GLfloat u, GLfloat);
   void (*EvalCoord2fv)(const GLfloat *);
   void (*EvalMesh1)(GLenum, GLint, GLint);
   void (*EvalMesh2)(GLenum, GLint, GLint, GLint, GLint);
   void (*EvalPoint1)(GLint);
   void (*EvalPoint2)(GLint, GLint);
   void (*FeedbackBuffer)(GLsizei, GLenum, GLfloat *);
   void (*Finish)(void);
   void (*Flush)(void);
   void (*Fogf)(GLenum, GLfloat);
   void (*Fogfv)(GLenum, const GLfloat *);
   void (*Fogi)(GLenum, GLint);
   void (*Fogiv)(GLenum, const GLint *);
   void (*FrontFace)(GLenum);
   void (*Frustum)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
   GLuint (*GenLists)(GLsizei);
   void (*GetBooleanv)(GLenum, GLboolean *);
   void (*GetClipPlane)(GLenum, GLdouble *);
   void (*GetDoublev)(GLenum, GLdouble *);
   GLenum (*GetError)(void);
   void (*GetFloatv)(GLenum, GLfloat *);
   void (*GetIntegerv)(GLenum, GLint *);
   void (*GetLightfv)(GLenum light, GLenum, GLfloat *);
   void (*GetLightiv)(GLenum light, GLenum, GLint *);
   void (*GetMapdv)(GLenum, GLenum, GLdouble *);
   void (*GetMapfv)(GLenum, GLenum, GLfloat *);
   void (*GetMapiv)(GLenum, GLenum, GLint *);
   void (*GetMaterialfv)(GLenum, GLenum, GLfloat *);
   void (*GetMaterialiv)(GLenum, GLenum, GLint *);
   void (*GetPixelMapfv)(GLenum, GLfloat *);
   void (*GetPixelMapuiv)(GLenum, GLuint *);
   void (*GetPixelMapusv)(GLenum, GLushort *);
   void (*GetPolygonStipple)(GLubyte *);
   const GLubyte* (*GetString)(GLenum name);
   void (*GetTexEnvfv)(GLenum, GLenum, GLfloat *);
   void (*GetTexEnviv)(GLenum, GLenum, GLint *);
   void (*GetTexGendv)(GLenum coord, GLenum, GLdouble *);
   void (*GetTexGenfv)(GLenum coord, GLenum, GLfloat *);
   void (*GetTexGeniv)(GLenum coord, GLenum, GLint *);
   void (*GetTexImage)(GLenum, GLint level, GLenum, GLenum, GLvoid *);
   void (*GetTexLevelParameterfv)(GLenum, GLint, GLenum, GLfloat *);
   void (*GetTexLevelParameteriv)(GLenum, GLint, GLenum, GLint *);
   void (*GetTexParameterfv)(GLenum, GLenum, GLfloat *);
   void (*GetTexParameteriv)(GLenum, GLenum, GLint *);
   void (*Hint)(GLenum, GLenum);
   void (*IndexMask)(GLuint);
   void (*Indexd)(GLdouble);
   void (*Indexdv)(const GLdouble *);
   void (*Indexf)(GLfloat);
   void (*Indexfv)(const GLfloat *);
   void (*Indexi)(GLint);
   void (*Indexiv)(const GLint *);
   void (*Indexs)(GLshort);
   void (*Indexsv)(const GLshort *);
   void (*InitNames)(void);
   GLboolean (*IsEnabled)(GLenum);
   GLboolean (*IsList)(GLuint);
   void (*LightModelf)(GLenum, GLfloat);
   void (*LightModelfv)(GLenum, const GLfloat *);
   void (*LightModeli)(GLenum, GLint);
   void (*LightModeliv)(GLenum, const GLint *);
   void (*Lightf)(GLenum light, GLenum, GLfloat);
   void (*Lightfv)(GLenum light, GLenum, const GLfloat *);
   void (*Lighti)(GLenum light, GLenum, GLint);
   void (*Lightiv)(GLenum light, GLenum, const GLint *);
   void (*LineStipple)(GLint factor, GLushort);
   void (*LineWidth)(GLfloat);
   void (*ListBase)(GLuint);
   void (*LoadIdentity)(void);
   void (*LoadMatrixd)(const GLdouble *);
   void (*LoadMatrixf)(const GLfloat *);
   void (*LoadName)(GLuint);
   void (*LogicOp)(GLenum);
   void (*Map1d)(GLenum, GLdouble, GLdouble, GLint, GLint, const GLdouble *);
   void (*Map1f)(GLenum, GLfloat, GLfloat, GLint, GLint, const GLfloat *);
   void (*Map2d)(GLenum, GLdouble, GLdouble, GLint, GLint, GLdouble, GLdouble, GLint, GLint, const GLdouble *);
   void (*Map2f)(GLenum, GLfloat, GLfloat, GLint, GLint, GLfloat, GLfloat, GLint, GLint, const GLfloat *);
   void (*MapGrid1d)(GLint, GLdouble, GLdouble);
   void (*MapGrid1f)(GLint, GLfloat, GLfloat);
   void (*MapGrid2d)(GLint, GLdouble, GLdouble, GLint, GLdouble, GLdouble);
   void (*MapGrid2f)(GLint, GLfloat, GLfloat, GLint, GLfloat, GLfloat);
   void (*Materialf)(GLenum, GLenum, GLfloat);
   void (*Materialfv)(GLenum, GLenum, const GLfloat *);
   void (*Materiali)(GLenum, GLenum, GLint);
   void (*Materialiv)(GLenum, GLenum, const GLint *);
   void (*MatrixMode)(GLenum);
   void (*MultMatrixd)(const GLdouble *);
   void (*MultMatrixf)(const GLfloat *);
   void (*NewList)(GLuint list, GLenum);
   void (*Normal3b)(GLbyte, GLbyte, GLbyte);
   void (*Normal3bv)(const GLbyte *);
   void (*Normal3d)(GLdouble, GLdouble, GLdouble);
   void (*Normal3dv)(const GLdouble *);
   void (*Normal3f)(GLfloat, GLfloat, GLfloat);
   void (*Normal3fv)(const GLfloat *);
   void (*Normal3i)(GLint, GLint, GLint);
   void (*Normal3iv)(const GLint *);
   void (*Normal3s)(GLshort, GLshort, GLshort);
   void (*Normal3sv)(const GLshort *);
   void (*Ortho)(GLdouble, GLdouble, GLdouble, GLdouble, GLdouble, GLdouble);
   void (*PassThrough)(GLfloat);
   void (*PixelMapfv)(GLenum, GLint, const GLfloat *);
   void (*PixelMapuiv)(GLenum, GLint, const GLuint *);
   void (*PixelMapusv)(GLenum, GLint, const GLushort *);
   void (*PixelStoref)(GLenum, GLfloat);
   void (*PixelStorei)(GLenum, GLint);
   void (*PixelTransferf)(GLenum, GLfloat);
   void (*PixelTransferi)(GLenum, GLint);
   void (*PixelZoom)(GLfloat, GLfloat);
   void (*PointSize)(GLfloat);
   void (*PolygonMode)(GLenum, GLenum);
   void (*PolygonOffset)(GLfloat, GLfloat);
   void (*PolygonStipple)(const GLubyte *);
   void (*PopAttrib)(void);
   void (*PopMatrix)(void);
   void (*PopName)(void);
   void (*PushAttrib)(GLbitfield);
   void (*PushMatrix)(void);
   void (*PushName)(GLuint);
   void (*RasterPos2d)(GLdouble, GLdouble);
   void (*RasterPos2dv)(const GLdouble *);
   void (*RasterPos2f)(GLfloat, GLfloat);
   void (*RasterPos2fv)(const GLfloat *);
   void (*RasterPos2i)(GLint, GLint);
   void (*RasterPos2iv)(const GLint *);
   void (*RasterPos2s)(GLshort, GLshort);
   void (*RasterPos2sv)(const GLshort *);
   void (*RasterPos3d)(GLdouble, GLdouble, GLdouble);
   void (*RasterPos3dv)(const GLdouble *);
   void (*RasterPos3f)(GLfloat, GLfloat, GLfloat);
   void (*RasterPos3fv)(const GLfloat *);
   void (*RasterPos3i)(GLint, GLint, GLint);
   void (*RasterPos3iv)(const GLint *);
   void (*RasterPos3s)(GLshort, GLshort, GLshort);
   void (*RasterPos3sv)(const GLshort *);
   void (*RasterPos4d)(GLdouble, GLdouble, GLdouble, GLdouble);
   void (*RasterPos4dv)(const GLdouble *);
   void (*RasterPos4f)(GLfloat, GLfloat, GLfloat, GLfloat);
   void (*RasterPos4fv)(const GLfloat *);
   void (*RasterPos4i)(GLint, GLint, GLint, GLint);
   void (*RasterPos4iv)(const GLint *);
   void (*RasterPos4s)(GLshort, GLshort, GLshort, GLshort);
   void (*RasterPos4sv)(const GLshort *);
   void (*ReadBuffer)(GLenum);
   void (*ReadPixels)(GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, GLvoid *);
   void (*Rectd)(GLdouble, GLdouble, GLdouble, GLdouble);
   void (*Rectdv)(const GLdouble *, const GLdouble *);
   void (*Rectf)(GLfloat, GLfloat, GLfloat, GLfloat);
   void (*Rectfv)(const GLfloat *, const GLfloat *);
   void (*Recti)(GLint, GLint, GLint, GLint);
   void (*Rectiv)(const GLint *, const GLint *);
   void (*Rects)(GLshort, GLshort, GLshort, GLshort);
   void (*Rectsv)(const GLshort *, const GLshort *);
   GLint (*RenderMode)(GLenum);
   void (*Rotated)(GLdouble, GLdouble, GLdouble, GLdouble);
   void (*Rotatef)(GLfloat, GLfloat, GLfloat, GLfloat);
   void (*Scaled)(GLdouble, GLdouble, GLdouble);
   void (*Scalef)(GLfloat, GLfloat, GLfloat);
   void (*Scissor)(GLint, GLint, GLsizei, GLsizei);
   void (*SelectBuffer)(GLsizei, GLuint *);
   void (*ShadeModel)(GLenum);
   void (*StencilFunc)(GLenum, GLint, GLuint);
   void (*StencilMask)(GLuint);
   void (*StencilOp)(GLenum, GLenum, GLenum);
   void (*TexCoord1d)(GLdouble);
   void (*TexCoord1dv)(const GLdouble *);
   void (*TexCoord1f)(GLfloat);
   void (*TexCoord1fv)(const GLfloat *);
   void (*TexCoord1i)(GLint);
   void (*TexCoord1iv)(const GLint *);
   void (*TexCoord1s)(GLshort);
   void (*TexCoord1sv)(const GLshort *);
   void (*TexCoord2d)(GLdouble, GLdouble);
   void (*TexCoord2dv)(const GLdouble *);
   void (*TexCoord2f)(GLfloat, GLfloat);
   void (*TexCoord2fv)(const GLfloat *);
   void (*TexCoord2i)(GLint, GLint);
   void (*TexCoord2iv)(const GLint *);
   void (*TexCoord2s)(GLshort, GLshort);
   void (*TexCoord2sv)(const GLshort *);
   void (*TexCoord3d)(GLdouble, GLdouble, GLdouble);
   void (*TexCoord3dv)(const GLdouble *);
   void (*TexCoord3f)(GLfloat, GLfloat, GLfloat);
   void (*TexCoord3fv)(const GLfloat *);
   void (*TexCoord3i)(GLint, GLint, GLint);
   void (*TexCoord3iv)(const GLint *);
   void (*TexCoord3s)(GLshort, GLshort, GLshort);
   void (*TexCoord3sv)(const GLshort *);
   void (*TexCoord4d)(GLdouble, GLdouble, GLdouble, GLdouble);
   void (*TexCoord4dv)(const GLdouble *);
   void (*TexCoord4f)(GLfloat, GLfloat, GLfloat, GLfloat);
   void (*TexCoord4fv)(const GLfloat *);
   void (*TexCoord4i)(GLint, GLint, GLint, GLint);
   void (*TexCoord4iv)(const GLint *);
   void (*TexCoord4s)(GLshort, GLshort, GLshort, GLshort);
   void (*TexCoord4sv)(const GLshort *);
   void (*TexEnvf)(GLenum, GLenum, GLfloat);
   void (*TexEnvfv)(GLenum, GLenum, const GLfloat *);
   void (*TexEnvi)(GLenum, GLenum, GLint);
   void (*TexEnviv)(GLenum, GLenum, const GLint *);
   void (*TexGend)(GLenum, GLenum, GLdouble);
   void (*TexGendv)(GLenum, GLenum, const GLdouble *);
   void (*TexGenf)(GLenum, GLenum, GLfloat);
   void (*TexGenfv)(GLenum, GLenum, const GLfloat *);
   void (*TexGeni)(GLenum, GLenum, GLint);
   void (*TexGeniv)(GLenum, GLenum, const GLint *);
   void (*TexImage1D)(GLenum, GLint, GLint, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
   void (*TexImage2D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
   void (*TexParameterf)(GLenum, GLenum, GLfloat);
   void (*TexParameterfv)(GLenum, GLenum, const GLfloat *);
   void (*TexParameteri)(GLenum, GLenum, GLint);
   void (*TexParameteriv)(GLenum, GLenum, const GLint *);
   void (*Translated)(GLdouble, GLdouble, GLdouble);
   void (*Translatef)(GLfloat, GLfloat, GLfloat);
   void (*Vertex2d)(GLdouble, GLdouble);
   void (*Vertex2dv)(const GLdouble *);
   void (*Vertex2f)(GLfloat, GLfloat);
   void (*Vertex2fv)(const GLfloat *);
   void (*Vertex2i)(GLint, GLint);
   void (*Vertex2iv)(const GLint *);
   void (*Vertex2s)(GLshort, GLshort);
   void (*Vertex2sv)(const GLshort *);
   void (*Vertex3d)(GLdouble, GLdouble, GLdouble);
   void (*Vertex3dv)(const GLdouble *);
   void (*Vertex3f)(GLfloat, GLfloat, GLfloat);
   void (*Vertex3fv)(const GLfloat *);
   void (*Vertex3i)(GLint, GLint, GLint);
   void (*Vertex3iv)(const GLint *);
   void (*Vertex3s)(GLshort, GLshort, GLshort);
   void (*Vertex3sv)(const GLshort *);
   void (*Vertex4d)(GLdouble, GLdouble, GLdouble, GLdouble);
   void (*Vertex4dv)(const GLdouble *);
   void (*Vertex4f)(GLfloat, GLfloat, GLfloat, GLfloat);
   void (*Vertex4fv)(const GLfloat *);
   void (*Vertex4i)(GLint, GLint, GLint, GLint);
   void (*Vertex4iv)(const GLint *);
   void (*Vertex4s)(GLshort, GLshort, GLshort, GLshort);
   void (*Vertex4sv)(const GLshort *);
   void (*Viewport)(GLint, GLint, GLsizei, GLsizei);

   /*
    * OpenGL 1.1
    */
   GLboolean (*AreTexturesResident)(GLsizei, const GLuint *, GLboolean *);
   void (*ArrayElement)(GLint);
   void (*BindTexture)(GLenum, GLuint);
   void (*ColorPointer)(GLint, GLenum, GLsizei, const GLvoid *);
   void (*CopyTexImage1D)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint);
   void (*CopyTexImage2D)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint);
   void (*CopyTexSubImage1D)(GLenum, GLint, GLint, GLint, GLint, GLsizei);
   void (*CopyTexSubImage2D)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
   void (*DeleteTextures)(GLsizei, const GLuint *);
   void (*DisableClientState)(GLenum);
   void (*DrawArrays)(GLenum, GLint, GLsizei);
   void (*DrawElements)(GLenum, GLsizei, GLenum, const GLvoid *);
   void (*EdgeFlagPointer)(GLsizei, const GLvoid *);
   void (*EnableClientState)(GLenum);
   void (*GenTextures)(GLsizei, GLuint *);
   void (*GetPointerv)(GLenum, GLvoid **);
   void (*IndexPointer)(GLenum, GLsizei, const GLvoid *);
   void (*Indexub)(GLubyte);
   void (*Indexubv)(const GLubyte *);
   void (*InterleavedArrays)(GLenum, GLsizei, const GLvoid *);
   GLboolean (*IsTexture)(GLuint);
   void (*NormalPointer)(GLenum, GLsizei, const GLvoid *);
   void (*PopClientAttrib)(void);
   void (*PrioritizeTextures)(GLsizei, const GLuint *, const GLclampf *);
   void (*PushClientAttrib)(GLbitfield);
   void (*TexCoordPointer)(GLint, GLenum, GLsizei, const GLvoid *);
   void (*TexSubImage1D)(GLenum, GLint, GLint, GLsizei, GLenum, GLenum, const GLvoid *);
   void (*TexSubImage2D)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
   void (*VertexPointer)(GLint, GLenum, GLsizei, const GLvoid *);


   /*
    * OpenGL 1.2
    */
   void (*CopyTexSubImage3D)(GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
   void (*DrawRangeElements)(GLenum, GLuint, GLuint, GLsizei, GLenum, const GLvoid *);
   void (*TexImage3D)(GLenum, GLint, GLint, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
   void (*TexSubImage3D)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);


   /*
    * GL_ARB_imaging
    */
   void (*BlendColor)(GLclampf, GLclampf, GLclampf, GLclampf);
   void (*BlendEquation)(GLenum);
   void (*ColorSubTable)(GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
   void (*ColorTable)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *);
   void (*ColorTableParameterfv)(GLenum, GLenum, const GLfloat *);
   void (*ColorTableParameteriv)(GLenum, GLenum, const GLint *);
   void (*ConvolutionFilter1D)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *);
   void (*ConvolutionFilter2D)(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
   void (*ConvolutionParameterf)(GLenum, GLenum, GLfloat);
   void (*ConvolutionParameterfv)(GLenum, GLenum, const GLfloat *);
   void (*ConvolutionParameteri)(GLenum, GLenum, GLint);
   void (*ConvolutionParameteriv)(GLenum, GLenum, const GLint *);
   void (*CopyColorSubTable)(GLenum, GLsizei, GLint, GLint, GLsizei);
   void (*CopyColorTable)(GLenum, GLenum, GLint, GLint, GLsizei);
   void (*CopyConvolutionFilter1D)(GLenum, GLenum, GLint x, GLint y, GLsizei);
   void (*CopyConvolutionFilter2D)(GLenum, GLenum, GLint x, GLint y, GLsizei, GLsizei);
   void (*GetColorTable)(GLenum, GLenum, GLenum, GLvoid *);
   void (*GetColorTableParameterfv)(GLenum, GLenum, GLfloat *);
   void (*GetColorTableParameteriv)(GLenum, GLenum, GLint *);
   void (*GetConvolutionFilter)(GLenum, GLenum, GLenum, GLvoid *);
   void (*GetConvolutionParameterfv)(GLenum, GLenum, GLfloat *);
   void (*GetConvolutionParameteriv)(GLenum, GLenum, GLint *);
   void (*GetHistogram)(GLenum, GLboolean, GLenum, GLenum, GLvoid *);
   void (*GetHistogramParameterfv)(GLenum, GLenum, GLfloat *);
   void (*GetHistogramParameteriv)(GLenum, GLenum, GLint *);
   void (*GetMinmax)(GLenum, GLboolean, GLenum, GLenum, GLvoid *);
   void (*GetMinmaxParameterfv)(GLenum, GLenum, GLfloat *);
   void (*GetMinmaxParameteriv)(GLenum, GLenum, GLint *);
   void (*GetSeparableFilter)(GLenum, GLenum, GLenum, GLvoid *, GLvoid *, GLvoid *);
   void (*Histogram)(GLenum, GLsizei, GLenum, GLboolean);
   void (*Minmax)(GLenum, GLenum, GLboolean);
   void (*ResetHistogram)(GLenum);
   void (*ResetMinmax)(GLenum);
   void (*SeparableFilter2D)(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *, const GLvoid *);

   /*
    * GL_ARB_multitexture
    */
   void (*ActiveTextureARB)(GLenum);
   void (*ClientActiveTextureARB)(GLenum);
   void (*MultiTexCoord1dARB)(GLenum, GLdouble);
   void (*MultiTexCoord1dvARB)(GLenum, const GLdouble *);
   void (*MultiTexCoord1fARB)(GLenum, GLfloat);
   void (*MultiTexCoord1fvARB)(GLenum, const GLfloat *);
   void (*MultiTexCoord1iARB)(GLenum, GLint);
   void (*MultiTexCoord1ivARB)(GLenum, const GLint *);
   void (*MultiTexCoord1sARB)(GLenum, GLshort);
   void (*MultiTexCoord1svARB)(GLenum, const GLshort *);
   void (*MultiTexCoord2dARB)(GLenum, GLdouble, GLdouble);
   void (*MultiTexCoord2dvARB)(GLenum, const GLdouble *);
   void (*MultiTexCoord2fARB)(GLenum, GLfloat, GLfloat);
   void (*MultiTexCoord2fvARB)(GLenum, const GLfloat *);
   void (*MultiTexCoord2iARB)(GLenum, GLint, GLint);
   void (*MultiTexCoord2ivARB)(GLenum, const GLint *);
   void (*MultiTexCoord2sARB)(GLenum, GLshort, GLshort);
   void (*MultiTexCoord2svARB)(GLenum, const GLshort *);
   void (*MultiTexCoord3dARB)(GLenum, GLdouble, GLdouble, GLdouble);
   void (*MultiTexCoord3dvARB)(GLenum, const GLdouble *);
   void (*MultiTexCoord3fARB)(GLenum, GLfloat, GLfloat, GLfloat);
   void (*MultiTexCoord3fvARB)(GLenum, const GLfloat *);
   void (*MultiTexCoord3iARB)(GLenum, GLint, GLint, GLint);
   void (*MultiTexCoord3ivARB)(GLenum, const GLint *);
   void (*MultiTexCoord3sARB)(GLenum, GLshort, GLshort, GLshort);
   void (*MultiTexCoord3svARB)(GLenum, const GLshort *);
   void (*MultiTexCoord4dARB)(GLenum, GLdouble, GLdouble, GLdouble, GLdouble);
   void (*MultiTexCoord4dvARB)(GLenum, const GLdouble *);
   void (*MultiTexCoord4fARB)(GLenum, GLfloat, GLfloat, GLfloat, GLfloat);
   void (*MultiTexCoord4fvARB)(GLenum, const GLfloat *);
   void (*MultiTexCoord4iARB)(GLenum, GLint, GLint, GLint, GLint);
   void (*MultiTexCoord4ivARB)(GLenum, const GLint *);
   void (*MultiTexCoord4sARB)(GLenum, GLshort, GLshort, GLshort, GLshort);
   void (*MultiTexCoord4svARB)(GLenum, const GLshort *);


   /*
    * Extensions
    */

   /* 1. GL_EXT_abgr - no functions */

   /* 2. GL_EXT_blend_color */
   void (*BlendColorEXT)(GLclampf, GLclampf, GLclampf, GLclampf);

   /* 3. GL_EXT_polygon_offset */
   void (*PolygonOffsetEXT)(GLfloat, GLfloat);

   /* 4. GL_EXT_texture - no functions */

   /* 5. ??? */

   /* 6. GL_EXT_texture3D */
   void (*CopyTexSubImage3DEXT)(GLenum, GLint, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);
   void (*TexImage3DEXT)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const GLvoid *);
   void (*TexSubImage3DEXT)(GLenum, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);

   /* 7. GL_SGI_texture_filter4 */
   void (*GetTexFilterFuncSGIS)(GLenum, GLenum, GLsizei, const GLfloat *);
   void (*TexFilterFuncSGIS)(GLenum, GLenum, GLfloat *);

   /* 8. ??? */

   /* 9. GL_EXT_subtexture */
   void (*TexSubImage1DEXT)(GLenum, GLint, GLint, GLsizei, GLenum, GLenum, const GLvoid *);
   void (*TexSubImage2DEXT)(GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);

   /* 10. GL_EXT_copy_texture */
   void (*CopyTexImage1DEXT)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLint);
   void (*CopyTexImage2DEXT)(GLenum, GLint, GLenum, GLint, GLint, GLsizei, GLsizei, GLint);
   void (*CopyTexSubImage1DEXT)(GLenum, GLint, GLint, GLint, GLint, GLsizei);
   void (*CopyTexSubImage2DEXT)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei);

   /* 11. GL_EXT_histogram */
   void (*GetHistogramEXT)(GLenum, GLboolean, GLenum, GLenum, GLvoid *);
   void (*GetHistogramParameterfvEXT)(GLenum, GLenum, GLfloat *);
   void (*GetHistogramParameterivEXT)(GLenum, GLenum, GLint *);
   void (*GetMinmaxEXT)(GLenum, GLboolean, GLenum, GLenum, GLvoid *);
   void (*GetMinmaxParameterfvEXT)(GLenum, GLenum, GLfloat *);
   void (*GetMinmaxParameterivEXT)(GLenum, GLenum, GLint *);
   void (*HistogramEXT)(GLenum, GLsizei, GLenum, GLboolean);
   void (*MinmaxEXT)(GLenum, GLenum, GLboolean);
   void (*ResetHistogramEXT)(GLenum);
   void (*ResetMinmaxEXT)(GLenum);

   /* 12. GL_EXT_convolution */
   void (*ConvolutionFilter1DEXT)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *);
   void (*ConvolutionFilter2DEXT)(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *);
   void (*ConvolutionParameterfEXT)(GLenum, GLenum, GLfloat);
   void (*ConvolutionParameterfvEXT)(GLenum, GLenum, const GLfloat *);
   void (*ConvolutionParameteriEXT)(GLenum, GLenum, GLint);
   void (*ConvolutionParameterivEXT)(GLenum, GLenum, const GLint *);
   void (*CopyConvolutionFilter1DEXT)(GLenum, GLenum, GLint x, GLint y, GLsizei);
   void (*CopyConvolutionFilter2DEXT)(GLenum, GLenum, GLint x, GLint y, GLsizei, GLsizei);
   void (*GetConvolutionFilterEXT)(GLenum, GLenum, GLenum, GLvoid *);
   void (*GetConvolutionParameterfvEXT)(GLenum, GLenum, GLfloat *);
   void (*GetConvolutionParameterivEXT)(GLenum, GLenum, GLint *);
   void (*GetSeparableFilterEXT)(GLenum, GLenum, GLenum, GLvoid *, GLvoid *, GLvoid *);
   void (*SeparableFilter2DEXT)(GLenum, GLenum, GLsizei, GLsizei, GLenum, GLenum, const GLvoid *, const GLvoid *);

   /* 13. GL_SGI_color_matrix - no functions */

   /* 14. GL_SGI_color_table */
   void (*ColorTableSGI)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *);
   void (*ColorTableParameterfvSGI)(GLenum, GLenum, const GLfloat *); 
   void (*ColorTableParameterivSGI)(GLenum, GLenum, const GLint *);
   void (*CopyColorTableSGI)(GLenum, GLenum, GLint, GLint, GLsizei);
   void (*GetColorTableSGI)(GLenum, GLenum, GLenum, GLvoid *);
   void (*GetColorTableParameterfvSGI)(GLenum, GLenum, GLfloat *);
   void (*GetColorTableParameterivSGI)(GLenum, GLenum, GLint *);

   /* 15. GL_SGIS_pixel_texture */
   void (*PixelTexGenParameterfSGIS)(GLenum, GLfloat);
   void (*PixelTexGenParameteriSGIS)(GLenum, GLint);
   void (*GetPixelTexGenParameterfvSGIS)(GLenum, GLfloat *);
   void (*GetPixelTexGenParameterivSGIS)(GLenum, GLint *);

   /* 16. GL_SGIS_texture4D */
   void (*TexImage4DSGIS)(GLenum, GLint, GLenum, GLsizei, GLsizei, GLsizei, GLsizei, GLint, GLenum, GLenum, const void *);
   void (*TexSubImage4DSGIS)(GLenum, GLint, GLint, GLint, GLint, GLint, GLsizei, GLsizei, GLsizei, GLsizei, GLenum, GLenum, const void *);

   /* 17. GL_SGI_texture_color_table - no functions */

   /* 18. GL_EXT_cmyka - no functions */

   /* 19. ??? */

   /* 20. GL_EXT_texture_object */
   GLboolean (*AreTexturesResidentEXT)(GLsizei, const GLuint *, GLboolean *);
   void (*BindTextureEXT)(GLenum, GLuint);
   void (*DeleteTexturesEXT)(GLsizei, const GLuint *);
   void (*GenTexturesEXT)(GLsizei, GLuint *);
   GLboolean (*IsTextureEXT)(GLuint);
   void (*PrioritizeTexturesEXT)(GLsizei, const GLuint *, const GLclampf *);

   /* 21. GL_SGIS_detail_texture */
   void (*DetailTexFuncSGIS)(GLenum, GLsizei, const GLfloat *);
   void (*GetDetailTexFuncSGIS)(GLenum, GLfloat *);

   /* 22. GL_SGIS_sharpen_texture */
   void (*GetSharpenTexFuncSGIS)(GLenum, GLfloat *);
   void (*SharpenTexFuncSGIS)(GLenum, GLsizei, const GLfloat *);

   /* 23. GL_EXT_packed_pixels - no functions */

   /* 24. GL_SGIS_texture_lod - no functions */

   /* 25. GL_SGIS_multisample */
   void (*SampleMaskSGIS)(GLclampf, GLboolean);
   void (*SamplePatternSGIS)(GLenum);

   /* 26. ??? */

   /* 27. GL_EXT_rescale_normal - no functions */
   
   /* 28. GLX_EXT_visual_info - no functions */

   /* 29. ??? */

   /* 30. GL_EXT_vertex_array */
   void (*ArrayElementEXT)(GLint);
   void (*ColorPointerEXT)(GLint, GLenum, GLsizei, GLsizei, const void *);
   void (*DrawArraysEXT)(GLenum, GLint, GLsizei);
   void (*EdgeFlagPointerEXT)(GLsizei, GLsizei, const GLboolean *);
   void (*GetPointervEXT)(GLenum, void **);
   void (*IndexPointerEXT)(GLenum, GLsizei, GLsizei, const void *);
   void (*NormalPointerEXT)(GLenum, GLsizei, GLsizei, const void *);
   void (*TexCoordPointerEXT)(GLint, GLenum, GLsizei, GLsizei, const void *);
   void (*VertexPointerEXT)(GLint, GLenum, GLsizei, GLsizei, const void *);

   /* 31. GL_EXT_misc_attribute - no functions */

   /* 32. GL_SGIS_generate_mipmap - no functions */

   /* 33. GL_SGIX_clipmap - no functions */

   /* 34. GL_SGIX_shadow - no functions */

   /* 35. GL_SGIS_texture_edge_clamp - no functions */

   /* 36. GL_SGIS_texture_border_clamp - no functions */

   /* 37. GL_EXT_blend_minmax */
   void (*BlendEquationEXT)(GLenum);

   /* 38. GL_EXT_blend_subtract - no functions */

   /* 39. GL_EXT_blend_logic_op - no functions */

   /* 40. GLX_SGI_swap_control - GLX functions */

   /* 41. GLX_SGI_video_sync - GLX functions */

   /* 42. GLX_SGI_make_current_read - GLX functions */

   /* 43. GLX_SGIX_video_source - GLX functions */

   /* 44. GLX_EXT_visual_rating - no functions */

   /* 45. GL_SGIX_interlace - no functions */

   /* 46. ??? */

   /* 47. GLX_EXT_import_context - GLX functions */

   /* 48. ??? */

   /* 49. GLX_SGIX_fbconfig - GLX functions */

   /* 50. GLX_SGIX_pbuffer - GLX functions */

   /* 51. GL_SGIS_texture_select - no functions */

   /* 52. GL_SGIX_sprite */
   void (*SpriteParameterfSGIX)(GLenum, GLfloat);
   void (*SpriteParameterfvSGIX)(GLenum, const GLfloat *);
   void (*SpriteParameteriSGIX)(GLenum, GLint);
   void (*SpriteParameterivSGIX)(GLenum, const GLint *);

   /* 53. ??? */

   /* 54. GL_EXT_point_parameters */
   void (*PointParameterfEXT)(GLenum, GLfloat);
   void (*PointParameterfvEXT)(GLenum, const GLfloat *);

   /* 55. GL_SGIX_instruments */
   GLint (*GetInstrumentsSGIX)(void);
   void (*InstrumentsBufferSGIX)(GLsizei, GLint *);
   GLint (*PollInstrumentsSGIX)(GLint *);
   void (*ReadInstrumentsSGIX)(GLint);
   void (*StartInstrumentsSGIX)(void);
   void (*StopInstrumentsSGIX)(GLint);

   /* 56. GL_SGIX_texture_scale_bias - no functions */

   /* 57. GL_SGIX_framezoom */
   void (*FrameZoomSGIX)(GLint);

   /* 58. GL_SGIX_tag_sample_buffer - no functions */

   /* 59. ??? */

   /* 60. GL_SGIX_reference_plane */
   void (*ReferencePlaneSGIX)(const GLdouble *);

   /* 61. GL_SGIX_flush_raster */
   void (*FlushRasterSGIX)(void);

   /* 62. GLX_SGI_cushion - GLX functions */

   /* 63. GL_SGIX_depth_texture - no functions */

   /* 64. ??? */

   /* 65. GL_SGIX_fog_offset - no functions */

   /* 66. GL_HP_image_transform */
#ifdef VMS
#define glGetImageTransformParameterfvHP glGetImageTransformParameterfvH
#define glGetImageTransformParameterivHP glGetImageTransformParameterivH
#endif
   void (*GetImageTransformParameterfvHP)(GLenum, GLenum, GLfloat *);
   void (*GetImageTransformParameterivHP)(GLenum, GLenum, GLint *);
   void (*ImageTransformParameterfHP)(GLenum, GLenum, const GLfloat);
   void (*ImageTransformParameterfvHP)(GLenum, GLenum, const GLfloat *);
   void (*ImageTransformParameteriHP)(GLenum, GLenum, const GLint);
   void (*ImageTransformParameterivHP)(GLenum, GLenum, const GLint *);

   /* 67. GL_HP_convolution_border_modes - no functions */

   /* 68. ??? */

   /* 69. GL_SGIX_texture_add_env - no functions */

   /* 70. ??? */

   /* 71. ??? */

   /* 72. ??? */

   /* 73. ??? */

   /* 74. GL_EXT_color_subtable */
   void (*ColorSubTableEXT)(GLenum, GLsizei, GLsizei, GLenum, GLenum, const void *);
   void (*CopyColorSubTableEXT)(GLenum, GLsizei, GLint, GLint, GLsizei);

   /* 75. GLU_EXT_object_space_tess - GLU functions */

   /* 76. GL_PGI_vertex_hints - no functions */

   /* 77. GL_PGI_misc_hints */
   void (*HintPGI)(GLenum, GLint);

   /* 78. GL_EXT_paletted_texture */
   /* ColorSubTableEXT already defined */
   void (*ColorTableEXT)(GLenum, GLenum, GLsizei, GLenum, GLenum, const GLvoid *);
   void (*GetColorTableEXT)(GLenum, GLenum, GLenum, GLvoid *);
   void (*GetColorTableParameterfvEXT)(GLenum, GLenum, GLfloat *);
   void (*GetColorTableParameterivEXT)(GLenum, GLenum, GLint *);

   /* 79. GL_EXT_clip_volume_hint - no functions */

   /* 80. GL_SGIX_list_priority */
   void (*GetListParameterfvSGIX)(GLuint, GLenum, GLfloat *);
   void (*GetListParameterivSGIX)(GLuint, GLenum, GLint *);
   void (*ListParameterfSGIX)(GLuint, GLenum, GLfloat);
   void (*ListParameterfvSGIX)(GLuint, GLenum, const GLfloat *);
   void (*ListParameteriSGIX)(GLuint, GLenum, GLint);
   void (*ListParameterivSGIX)(GLuint, GLenum, const GLint *);

   /* 81. GL_SGIX_ir_instrument1 - no functions */

   /* 82. ??? */

   /* 83. GLX_SGIX_video_resize - GLX functions */

   /* 84. GL_SGIX_texture_lod_bias - no functions */

   /* 85. GLU_SGI_filter4_parameters - GLU functions */

   /* 86. GLX_SGIX_dm_buffer - GLX functions */

   /* 87. ??? */

   /* 88. ??? */

   /* 89. ??? */

   /* 90. ??? */

   /* 91. GLX_SGIX_swap_group - GLX functions */

   /* 92. GLX_SGIX_swap_barrier - GLX functions */

   /* 93. GL_EXT_index_texture - no functions */

   /* 94. GL_EXT_index_material */
   void (*IndexMaterialEXT)(GLenum, GLenum);

   /* 95. GL_EXT_index_func */
   void (*IndexFuncEXT)(GLenum, GLfloat);

   /* 96. GL_EXT_index_array_formats - no functions */

   /* 97. GL_EXT_compiled_vertex_array */
   void (*LockArraysEXT)(GLint, GLsizei);
   void (*UnlockArraysEXT)(void);

   /* 98. GL_EXT_cull_vertex */
   void (*CullParameterfvEXT)(GLenum, const GLfloat *);
   void (*CullParameterdvEXT)(GLenum, const GLdouble *);

   /* 99. ??? */

   /* 100. GLU_EXT_nurbs_tessellator - GLU functions */

   /* 173. GL_EXT/INGR_blend_func_separate */
   void (*BlendFuncSeparateINGR)(GLenum, GLenum, GLenum, GLenum);

   /* GL_MESA_window_pos */
   void (*WindowPos4fMESA)(GLfloat x, GLfloat y, GLfloat z, GLfloat w);

   /* GL_MESA_resize_buffers */
   void (*ResizeBuffersMESA)(void);

   /* GL_ARB_transpose_matrix */
   void (*LoadTransposeMatrixdARB)(const GLdouble m[16]);
   void (*LoadTransposeMatrixfARB)(const GLfloat m[16]);
   void (*MultTransposeMatrixdARB)(const GLdouble m[16]);
   void (*MultTransposeMatrixfARB)(const GLfloat m[16]);

#if 0
   void *ExtensionFuncs[_GLAPI_EXTRA_SLOTS];
#endif
};


#endif
