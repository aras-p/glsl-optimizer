/* $XFree86: xc/lib/GL/glx/indirect.h,v 1.5 2003/09/28 20:15:03 alanh Exp $ */
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
 *
 */

#if !defined( _INDIRECT_H_ ) || defined( GENERATE_GLX_PROTOCOL_FUNCTIONS )

# if !defined( _INDIRECT_H_ ) 
#  if defined( GENERATE_GLX_PROTOCOL_FUNCTIONS )
#   error "indirect.h must be included once without GENERATE_GLX_PROTOCOL_FUNCTIONS defined!"
#  endif

#  define _INDIRECT_H_
#  include "indirect_wrap.h"

#  define glxproto_void(name, rop) \
   extern void __indirect_gl ## name ( void );
#  define glxproto_Cv(name, rop, type, count) \
   extern void __indirect_gl ## name (const type * v);
#  define glxproto_Cv_transpose(name, rop, type, w) \
   extern void __indirect_gl ## name (const type * v);
#  define glxproto_1s(name, rop, type) \
   extern void __indirect_gl ## name (type v1);
#  define glxproto_2s(name, rop, type) \
   extern void __indirect_gl ## name (type v1, type v2);
#  define glxproto_3s(name, rop, type) \
   extern void __indirect_gl ## name (type v1, type v2, type v3);
#  define glxproto_4s(name, rop, type) \
   extern void __indirect_gl ## name (type v1, type v2, type v3, type v4);
#  define glxproto_6s(name, rop, type) \
   void __indirect_gl ## name (type v1, type v2, type v3, type v4, type v5, type v6);
#  define glxproto_enum1_1s(name, rop, type) \
   void __indirect_gl ## name (GLenum e, type v1);
#  define glxproto_enum1_1v(name, rop, type) \
   void __indirect_gl ## name (GLenum e, const type * v);
#  define glxproto_enum1_2s(name, rop, type) \
   void __indirect_gl ## name (GLenum e, type v1, type v2);
#  define glxproto_enum1_2v(name, rop, type) \
   void __indirect_gl ## name (GLenum e, const type * v);
#  define glxproto_enum1_3s(name, rop, type) \
   void __indirect_gl ## name (GLenum e, type v1, type v2, type v3);
#  define glxproto_enum1_3v(name, rop, type) \
   void __indirect_gl ## name (GLenum e, const type * v);
#  define glxproto_enum1_4s(name, rop, type) \
   void __indirect_gl ## name (GLenum e, type v1, type v2, type v3, type v4);
#  define glxproto_enum1_4v(name, rop, type) \
   void __indirect_gl ## name (GLenum e, const type * v);
#  define glxproto_enum1_Vv(name, rop, type) \
   void __indirect_gl ## name (GLenum pname, const type * v);
#  define glxproto_enum2_1s(name, rop, type) \
   void __indirect_gl ## name (GLenum target, GLenum pname, type v1);
#define glxproto_enum2_Vv(name, rop, type) \
   void __indirect_gl ## name (GLenum target, GLenum pname, const type * v);

# endif /* !defined( _INDIRECT_H_ ) */

#define glxproto_1(name, rop, type) \
   glxproto_1s(name,      rop, type) \
   glxproto_Cv(name ## v, rop, type, 1)

#define glxvendr_1(name, rop, type, VEN) \
   glxproto_1s(name ## VEN,      rop, type) \
   glxproto_Cv(name ## v ## VEN, rop, type, 1)

#define glxproto_2(name, rop, type) \
   glxproto_2s(name,      rop, type) \
   glxproto_Cv(name ## v, rop, type, 2)

#define glxvendr_2(name, rop, type, VEN) \
   glxproto_2s(name ## VEN,      rop, type) \
   glxproto_Cv(name ## v ## VEN, rop, type, 2)

#define glxproto_3(name, rop, type) \
   glxproto_3s(name,      rop, type) \
   glxproto_Cv(name ## v, rop, type, 3)

#define glxvendr_3(name, rop, type, VEN) \
   glxproto_3s(name ## VEN,      rop, type) \
   glxproto_Cv(name ## v ## VEN, rop, type, 3)

#define glxproto_4(name, rop, type) \
   glxproto_4s(name,      rop, type) \
   glxproto_Cv(name ## v, rop, type, 4)

#define glxproto_enum1_1(name, rop, type) \
   glxproto_enum1_1s(name,      rop, type) \
   glxproto_enum1_1v(name ## v, rop, type)

#define glxvendr_enum1_1(name, rop, type, VEN) \
   glxproto_enum1_1s(name ## VEN,      rop, type) \
   glxproto_enum1_1v(name ## v ## VEN, rop, type)

#define glxproto_enum1_2(name, rop, type) \
   glxproto_enum1_2s(name,      rop, type) \
   glxproto_enum1_2v(name ## v, rop, type)

#define glxvendr_enum1_2(name, rop, type, VEN) \
   glxproto_enum1_2s(name ## VEN,      rop, type) \
   glxproto_enum1_2v(name ## v ## VEN, rop, type)

#define glxproto_enum1_3(name, rop, type) \
   glxproto_enum1_3s(name,      rop, type) \
   glxproto_enum1_3v(name ## v, rop, type)

#define glxvendr_enum1_3(name, rop, type, VEN) \
   glxproto_enum1_3s(name ## VEN,      rop, type) \
   glxproto_enum1_3v(name ## v ## VEN, rop, type)

#define glxproto_enum1_4(name, rop, type) \
   glxproto_enum1_4s(name,      rop, type) \
   glxproto_enum1_4v(name ## v, rop, type)

#define glxvendr_enum1_4(name, rop, type, VEN) \
   glxproto_enum1_4s(name ## VEN,      rop, type) \
   glxproto_enum1_4v(name ## v ## VEN, rop, type)

#define glxproto_enum1_V(name, rop, type) \
   glxproto_enum1_1s(name,      rop,      type) \
   glxproto_enum1_Vv(name ## v, rop ## v, type)

#define glxvendr_enum1_V(name, rop, type, VEN) \
   glxproto_enum1_1s(name ## VEN,      rop ## VEN,      type) \
   glxproto_enum1_Vv(name ## v ## VEN, rop ## v ## VEN, type)

#define glxproto_enum2_V(name, rop, type) \
   glxproto_enum2_1s(name,      rop,      type) \
   glxproto_enum2_Vv(name ## v, rop ## v, type)

#define glxvendr_enum2_V(name, rop, type, VEN) \
   glxproto_enum2_1s(name ## VEN,      rop ## VEN,      type) \
   glxproto_enum2_Vv(name ## v ## VEN, rop ## v ## VEN, type)

glxproto_1s(CallList, X_GLrop_CallList, GLuint)
glxproto_1s(ListBase, X_GLrop_ListBase, GLuint)
glxproto_1s(Begin,    X_GLrop_Begin,    GLenum)

glxproto_3(Color3b,  X_GLrop_Color3bv,  GLbyte)
glxproto_3(Color3s,  X_GLrop_Color3sv,  GLshort)
glxproto_3(Color3i,  X_GLrop_Color3iv,  GLint)
glxproto_3(Color3ub, X_GLrop_Color3ubv, GLubyte)
glxproto_3(Color3us, X_GLrop_Color3usv, GLushort)
glxproto_3(Color3ui, X_GLrop_Color3uiv, GLuint)
glxproto_3(Color3f,  X_GLrop_Color3fv,  GLfloat)
glxproto_3(Color3d,  X_GLrop_Color3dv,  GLdouble)

glxproto_4(Color4b,  X_GLrop_Color4bv,  GLbyte)
glxproto_4(Color4s,  X_GLrop_Color4sv,  GLshort)
glxproto_4(Color4i,  X_GLrop_Color4iv,  GLint)
glxproto_4(Color4ub, X_GLrop_Color4ubv, GLubyte)
glxproto_4(Color4us, X_GLrop_Color4usv, GLushort)
glxproto_4(Color4ui, X_GLrop_Color4uiv, GLuint)
glxproto_4(Color4f,  X_GLrop_Color4fv,  GLfloat)
glxproto_4(Color4d,  X_GLrop_Color4dv,  GLdouble)

glxproto_1(FogCoordf, X_GLrop_FogCoordfv, GLfloat)
glxproto_1(FogCoordd, X_GLrop_FogCoorddv, GLdouble)

glxproto_3(SecondaryColor3b,  X_GLrop_SecondaryColor3bv,  GLbyte)
glxproto_3(SecondaryColor3s,  X_GLrop_SecondaryColor3sv,  GLshort)
glxproto_3(SecondaryColor3i,  X_GLrop_SecondaryColor3iv,  GLint)
glxproto_3(SecondaryColor3ub, X_GLrop_SecondaryColor3ubv, GLubyte)
glxproto_3(SecondaryColor3us, X_GLrop_SecondaryColor3usv, GLushort)
glxproto_3(SecondaryColor3ui, X_GLrop_SecondaryColor3uiv, GLuint)
glxproto_3(SecondaryColor3f,  X_GLrop_SecondaryColor3fv,  GLfloat)
glxproto_3(SecondaryColor3d,  X_GLrop_SecondaryColor3dv,  GLdouble)

glxproto_1(EdgeFlag, X_GLrop_EdgeFlagv, GLboolean)

glxproto_1(Indexd,  X_GLrop_Indexdv,  GLdouble)
glxproto_1(Indexf,  X_GLrop_Indexfv,  GLfloat)
glxproto_1(Indexi,  X_GLrop_Indexiv,  GLint)
glxproto_1(Indexs,  X_GLrop_Indexsv,  GLshort)
glxproto_1(Indexub, X_GLrop_Indexubv, GLubyte)

glxproto_void(End, X_GLrop_End)

glxproto_3(Normal3b, X_GLrop_Normal3bv, GLbyte)
glxproto_3(Normal3s, X_GLrop_Normal3sv, GLshort)
glxproto_3(Normal3i, X_GLrop_Normal3iv, GLint)
glxproto_3(Normal3f, X_GLrop_Normal3fv, GLfloat)
glxproto_3(Normal3d, X_GLrop_Normal3dv, GLdouble)

glxproto_2(RasterPos2s, X_GLrop_RasterPos2sv, GLshort)
glxproto_2(RasterPos2i, X_GLrop_RasterPos2iv, GLint)
glxproto_2(RasterPos2f, X_GLrop_RasterPos2fv, GLfloat)
glxproto_2(RasterPos2d, X_GLrop_RasterPos2dv, GLdouble)
glxproto_3(RasterPos3s, X_GLrop_RasterPos3sv, GLshort)
glxproto_3(RasterPos3i, X_GLrop_RasterPos3iv, GLint)
glxproto_3(RasterPos3f, X_GLrop_RasterPos3fv, GLfloat)
glxproto_3(RasterPos3d, X_GLrop_RasterPos3dv, GLdouble)
glxproto_4(RasterPos4s, X_GLrop_RasterPos4sv, GLshort)
glxproto_4(RasterPos4i, X_GLrop_RasterPos4iv, GLint)
glxproto_4(RasterPos4f, X_GLrop_RasterPos4fv, GLfloat)
glxproto_4(RasterPos4d, X_GLrop_RasterPos4dv, GLdouble)

glxproto_1(TexCoord1s, X_GLrop_TexCoord1sv, GLshort)
glxproto_1(TexCoord1i, X_GLrop_TexCoord1iv, GLint)
glxproto_1(TexCoord1f, X_GLrop_TexCoord1fv, GLfloat)
glxproto_1(TexCoord1d, X_GLrop_TexCoord1dv, GLdouble)
glxproto_2(TexCoord2s, X_GLrop_TexCoord2sv, GLshort)
glxproto_2(TexCoord2i, X_GLrop_TexCoord2iv, GLint)
glxproto_2(TexCoord2f, X_GLrop_TexCoord2fv, GLfloat)
glxproto_2(TexCoord2d, X_GLrop_TexCoord2dv, GLdouble)
glxproto_3(TexCoord3s, X_GLrop_TexCoord3sv, GLshort)
glxproto_3(TexCoord3i, X_GLrop_TexCoord3iv, GLint)
glxproto_3(TexCoord3f, X_GLrop_TexCoord3fv, GLfloat)
glxproto_3(TexCoord3d, X_GLrop_TexCoord3dv, GLdouble)
glxproto_4(TexCoord4s, X_GLrop_TexCoord4sv, GLshort)
glxproto_4(TexCoord4i, X_GLrop_TexCoord4iv, GLint)
glxproto_4(TexCoord4f, X_GLrop_TexCoord4fv, GLfloat)
glxproto_4(TexCoord4d, X_GLrop_TexCoord4dv, GLdouble)

glxproto_2(Vertex2s, X_GLrop_Vertex2sv, GLshort)
glxproto_2(Vertex2i, X_GLrop_Vertex2iv, GLint)
glxproto_2(Vertex2f, X_GLrop_Vertex2fv, GLfloat)
glxproto_2(Vertex2d, X_GLrop_Vertex2dv, GLdouble)
glxproto_3(Vertex3s, X_GLrop_Vertex3sv, GLshort)
glxproto_3(Vertex3i, X_GLrop_Vertex3iv, GLint)
glxproto_3(Vertex3f, X_GLrop_Vertex3fv, GLfloat)
glxproto_3(Vertex3d, X_GLrop_Vertex3dv, GLdouble)
glxproto_4(Vertex4s, X_GLrop_Vertex4sv, GLshort)
glxproto_4(Vertex4i, X_GLrop_Vertex4iv, GLint)
glxproto_4(Vertex4f, X_GLrop_Vertex4fv, GLfloat)
glxproto_4(Vertex4d, X_GLrop_Vertex4dv, GLdouble)

glxproto_enum1_4v(ClipPlane, X_GLrop_ClipPlane, GLdouble)

glxproto_2s(ColorMaterial, X_GLrop_ColorMaterial, GLenum)

glxproto_1s(CullFace, X_GLrop_CullFace, GLenum)

glxproto_enum1_V(Fogi, X_GLrop_Fogi, GLint)
glxproto_enum1_V(Fogf, X_GLrop_Fogf, GLfloat)

glxproto_1s(FrontFace, X_GLrop_FrontFace, GLenum)
glxproto_2s(Hint,      X_GLrop_Hint,      GLenum)

glxproto_enum2_V(Lighti,      X_GLrop_Lighti,      GLint)
glxproto_enum2_V(Lightf,      X_GLrop_Lightf,      GLfloat)

glxproto_enum1_V(LightModeli, X_GLrop_LightModeli, GLint)
glxproto_enum1_V(LightModelf, X_GLrop_LightModelf, GLfloat)

glxproto_1s(LineWidth, X_GLrop_LineWidth, GLfloat)

glxproto_enum2_V(Materiali, X_GLrop_Materiali, GLint)
glxproto_enum2_V(Materialf, X_GLrop_Materialf, GLfloat)

glxproto_1s(PointSize, X_GLrop_PointSize, GLfloat)

glxproto_2s(PolygonMode, X_GLrop_PolygonMode, GLenum)

glxproto_1s(ShadeModel, X_GLrop_ShadeModel, GLenum)

glxproto_enum2_V(TexParameteri, X_GLrop_TexParameteri, GLint)
glxproto_enum2_V(TexParameterf, X_GLrop_TexParameterf, GLfloat)

glxproto_enum2_V(TexEnvi, X_GLrop_TexEnvi, GLint)
glxproto_enum2_V(TexEnvf, X_GLrop_TexEnvf, GLfloat)
glxproto_enum2_V(TexGeni, X_GLrop_TexGeni, GLint)
glxproto_enum2_V(TexGenf, X_GLrop_TexGenf, GLfloat)
glxproto_enum2_V(TexGend, X_GLrop_TexGend, GLdouble)

glxproto_void(InitNames, X_GLrop_InitNames)
glxproto_1s(LoadName, X_GLrop_LoadName, GLuint)
glxproto_1s(PassThrough, X_GLrop_PassThrough, GLfloat)
glxproto_void(PopName, X_GLrop_PopName)
glxproto_1s(PushName, X_GLrop_PushName, GLuint)

glxproto_1s(DrawBuffer, X_GLrop_DrawBuffer, GLenum)
glxproto_1s(Clear, X_GLrop_Clear, GLbitfield)

glxproto_4s(ClearAccum,   X_GLrop_ClearAccum,   GLfloat)
glxproto_1s(ClearIndex,   X_GLrop_ClearIndex,   GLfloat)
glxproto_4s(ClearColor,   X_GLrop_ClearColor,   GLclampf)
glxproto_1s(ClearStencil, X_GLrop_ClearStencil, GLint)
glxproto_1s(ClearDepth,   X_GLrop_ClearDepth,   GLclampd)

glxproto_1s(StencilMask,  X_GLrop_StencilMask,  GLuint)
glxproto_4s(ColorMask,    X_GLrop_ColorMask,    GLboolean)
glxproto_1s(DepthMask,    X_GLrop_DepthMask,    GLboolean)
glxproto_1s(IndexMask,    X_GLrop_IndexMask,    GLuint)

glxproto_enum1_1s(Accum, X_GLrop_Accum, GLfloat)

glxproto_void(PopAttrib, X_GLrop_PopAttrib)
glxproto_1s(PushAttrib,  X_GLrop_PushAttrib, GLbitfield)

glxproto_1(EvalCoord1f, X_GLrop_EvalCoord1fv, GLfloat)
glxproto_1(EvalCoord1d, X_GLrop_EvalCoord1dv, GLdouble)
glxproto_2(EvalCoord2f, X_GLrop_EvalCoord2fv, GLfloat)
glxproto_2(EvalCoord2d, X_GLrop_EvalCoord2dv, GLdouble)
glxproto_enum1_2s(EvalMesh1, X_GLrop_EvalMesh1, GLint)
glxproto_enum1_4s(EvalMesh2, X_GLrop_EvalMesh2, GLint)
glxproto_1s(EvalPoint1, X_GLrop_EvalPoint1, GLint)
glxproto_2s(EvalPoint2, X_GLrop_EvalPoint2, GLint)

glxproto_enum1_1s(AlphaFunc, X_GLrop_AlphaFunc, GLclampf)

glxproto_2s(BlendFunc,         X_GLrop_BlendFunc,         GLenum)
glxproto_4s(BlendFuncSeparate, X_GLrop_BlendFuncSeparate, GLenum)

glxproto_1s(LogicOp, X_GLrop_LogicOp, GLenum)

glxproto_3s(StencilOp, X_GLrop_StencilOp, GLenum)
glxproto_1s(DepthFunc, X_GLrop_DepthFunc, GLenum)

glxproto_2s(PixelZoom, X_GLrop_PixelZoom, GLfloat)

glxproto_enum1_1s(PixelTransferf, X_GLrop_PixelTransferf, GLfloat)
glxproto_enum1_1s(PixelTransferi, X_GLrop_PixelTransferi, GLint)

glxproto_1s(ReadBuffer, X_GLrop_ReadBuffer, GLenum)

glxproto_2s(DepthRange, X_GLrop_DepthRange, GLclampd)

glxproto_6s(Frustum, X_GLrop_Frustum, GLdouble)

glxproto_void(LoadIdentity, X_GLrop_LoadIdentity)
glxproto_1s(MatrixMode, X_GLrop_MatrixMode, GLenum)
glxproto_Cv(LoadMatrixf, X_GLrop_LoadMatrixf, GLfloat, 16)
glxproto_Cv(MultMatrixf, X_GLrop_MultMatrixf, GLfloat, 16)
glxproto_Cv(LoadMatrixd, X_GLrop_LoadMatrixd, GLdouble, 16)
glxproto_Cv(MultMatrixd, X_GLrop_MultMatrixd, GLdouble, 16)
glxproto_Cv_transpose(LoadTransposeMatrixfARB, X_GLrop_LoadMatrixf, GLfloat, 4)
glxproto_Cv_transpose(MultTransposeMatrixfARB, X_GLrop_MultMatrixf, GLfloat, 4)
glxproto_Cv_transpose(LoadTransposeMatrixdARB, X_GLrop_LoadMatrixd, GLdouble, 4)
glxproto_Cv_transpose(MultTransposeMatrixdARB, X_GLrop_MultMatrixd, GLdouble, 4)

glxproto_6s(Ortho, X_GLrop_Ortho, GLdouble)

glxproto_void(PushMatrix, X_GLrop_PushMatrix)
glxproto_void(PopMatrix,  X_GLrop_PopMatrix)

glxproto_4s(Rotatef,    X_GLrop_Rotatef,    GLfloat)
glxproto_3s(Scalef,     X_GLrop_Scalef,     GLfloat)
glxproto_3s(Translatef, X_GLrop_Translatef, GLfloat)
glxproto_4s(Rotated,    X_GLrop_Rotated,    GLdouble)
glxproto_3s(Scaled,     X_GLrop_Scaled,     GLdouble)
glxproto_3s(Translated, X_GLrop_Translated, GLdouble)

glxproto_2s(PolygonOffset, X_GLrop_PolygonOffset, GLfloat)

glxproto_enum1_1s(BindTexture, X_GLrop_BindTexture, GLuint)

glxproto_4s(BlendColor,    X_GLrop_BlendColor,    GLclampf)
glxproto_1s(BlendEquation, X_GLrop_BlendEquation, GLenum)

glxproto_enum2_Vv(ColorTableParameteriv, X_GLrop_ColorTableParameteriv, GLint)
glxproto_enum2_Vv(ColorTableParameterfv, X_GLrop_ColorTableParameterfv, GLfloat)

glxproto_enum2_V(ConvolutionParameteri, X_GLrop_ConvolutionParameteri, GLint)
glxproto_enum2_V(ConvolutionParameterf, X_GLrop_ConvolutionParameterf, GLfloat)

glxproto_enum2_1s(Minmax, X_GLrop_Minmax, GLboolean)

glxproto_1s(ResetHistogram, X_GLrop_ResetHistogram, GLenum)
glxproto_1s(ResetMinmax,    X_GLrop_ResetMinmax,    GLenum)

glxproto_1s(     ActiveTextureARB, X_GLrop_ActiveTextureARB,    GLenum)
glxvendr_enum1_1(MultiTexCoord1s,  X_GLrop_MultiTexCoord1svARB, GLshort,  ARB)
glxvendr_enum1_1(MultiTexCoord1i,  X_GLrop_MultiTexCoord1ivARB, GLint,    ARB)
glxvendr_enum1_1(MultiTexCoord1f,  X_GLrop_MultiTexCoord1fvARB, GLfloat,  ARB)
glxvendr_enum1_1(MultiTexCoord1d,  X_GLrop_MultiTexCoord1dvARB, GLdouble, ARB)
glxvendr_enum1_2(MultiTexCoord2s,  X_GLrop_MultiTexCoord2svARB, GLshort,  ARB)
glxvendr_enum1_2(MultiTexCoord2i,  X_GLrop_MultiTexCoord2ivARB, GLint,    ARB)
glxvendr_enum1_2(MultiTexCoord2f,  X_GLrop_MultiTexCoord2fvARB, GLfloat,  ARB)
glxvendr_enum1_2(MultiTexCoord2d,  X_GLrop_MultiTexCoord2dvARB, GLdouble, ARB)
glxvendr_enum1_3(MultiTexCoord3s,  X_GLrop_MultiTexCoord3svARB, GLshort,  ARB)
glxvendr_enum1_3(MultiTexCoord3i,  X_GLrop_MultiTexCoord3ivARB, GLint,    ARB)
glxvendr_enum1_3(MultiTexCoord3f,  X_GLrop_MultiTexCoord3fvARB, GLfloat,  ARB)
glxvendr_enum1_3(MultiTexCoord3d,  X_GLrop_MultiTexCoord3dvARB, GLdouble, ARB)
glxvendr_enum1_4(MultiTexCoord4s,  X_GLrop_MultiTexCoord4svARB, GLshort,  ARB)
glxvendr_enum1_4(MultiTexCoord4i,  X_GLrop_MultiTexCoord4ivARB, GLint,    ARB)
glxvendr_enum1_4(MultiTexCoord4f,  X_GLrop_MultiTexCoord4fvARB, GLfloat,  ARB)
glxvendr_enum1_4(MultiTexCoord4d,  X_GLrop_MultiTexCoord4dvARB, GLdouble, ARB)

glxvendr_enum1_V(PointParameterf, X_GLrop_PointParameterf, GLfloat, ARB)
glxproto_enum1_V(PointParameteri, X_GLrop_PointParameteri, GLint)

glxvendr_3(WindowPos3f, X_GLrop_WindowPos3fARB, GLfloat, ARB)

glxproto_1s(ActiveStencilFaceEXT, X_GLrop_ActiveStencilFaceEXT, GLenum)

glxproto_4s(Rects, X_GLrop_Rectsv, GLshort)
glxproto_4s(Recti, X_GLrop_Rectiv, GLint)
glxproto_4s(Rectf, X_GLrop_Rectfv, GLfloat)
glxproto_4s(Rectd, X_GLrop_Rectdv, GLdouble)

#if !defined( GENERATE_GLX_PROTOCOL_FUNCTIONS )
GLboolean __indirect_glAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences);
GLboolean __indirect_glAreTexturesResidentEXT(GLsizei n, const GLuint *textures, GLboolean *residences);
void __indirect_glArrayElement(GLint i);
void __indirect_glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
void __indirect_glCallLists(GLsizei n, GLenum type, const GLvoid *lists);
void __indirect_glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void __indirect_glColorSubTable(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *table);
void __indirect_glColorTable(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table);
void __indirect_glConvolutionFilter1D(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image);
void __indirect_glConvolutionFilter2D(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image);
void __indirect_glCopyConvolutionFilter1D(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
void __indirect_glCopyConvolutionFilter2D(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height);
void __indirect_glCopyColorTable(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
void __indirect_glCopyColorSubTable(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width);
void __indirect_glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
void __indirect_glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
void __indirect_glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void __indirect_glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
void __indirect_glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
void __indirect_glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
void __indirect_glDeleteLists(GLuint list, GLsizei range);
void __indirect_glDeleteTextures(GLsizei n, const GLuint *textures);
void __indirect_glDeleteTexturesEXT(GLsizei n, const GLuint *textures);
void __indirect_glDisable(GLenum cap);
void __indirect_glDisableClientState(GLenum array);
void __indirect_glDrawArrays(GLenum mode, GLint first, GLsizei count);
void __indirect_glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void __indirect_glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image);
void __indirect_glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);
void __indirect_glEdgeFlagPointer(GLsizei stride, const GLvoid *pointer);
void __indirect_glEnable(GLenum cap);
void __indirect_glEnableClientState(GLenum array);
void __indirect_glEndList(void);
void __indirect_glFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer);
void __indirect_glFinish(void);
void __indirect_glFlush(void);
GLuint __indirect_glGenLists(GLsizei range);
void __indirect_glGenTextures(GLsizei n, GLuint *textures);
void __indirect_glGenTexturesEXT(GLsizei n, GLuint *textures);
void __indirect_glGetBooleanv(GLenum val, GLboolean *b);
void __indirect_glGetClipPlane(GLenum plane, GLdouble *equation);
void __indirect_glGetColorTable(GLenum target, GLenum format, GLenum type, GLvoid *table);
void __indirect_glGetColorTableParameterfv(GLenum target, GLenum pname, GLfloat *params);
void __indirect_glGetColorTableParameteriv(GLenum target, GLenum pname, GLint *params);
void __indirect_glGetConvolutionFilter(GLenum target, GLenum format, GLenum type, GLvoid *image);
void __indirect_glGetConvolutionParameterfv(GLenum target, GLenum pname, GLfloat *params);
void __indirect_glGetConvolutionParameteriv(GLenum target, GLenum pname, GLint *params);
void __indirect_glGetDoublev(GLenum val, GLdouble *d);
GLenum __indirect_glGetError(void);
void __indirect_glGetFloatv(GLenum val, GLfloat *f);
void __indirect_glGetHistogram(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values);
void __indirect_glGetHistogramParameterfv(GLenum target, GLenum pname, GLfloat *params);
void __indirect_glGetHistogramParameteriv(GLenum target, GLenum pname, GLint *params);
void __indirect_glGetIntegerv(GLenum val, GLint *i);
void __indirect_glGetLightfv(GLenum light, GLenum pname, GLfloat *params);
void __indirect_glGetLightiv(GLenum light, GLenum pname, GLint *params);
void __indirect_glGetMapdv(GLenum target, GLenum query, GLdouble *v);
void __indirect_glGetMapfv(GLenum target, GLenum query, GLfloat *v);
void __indirect_glGetMapiv(GLenum target, GLenum query, GLint *v);
void __indirect_glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params);
void __indirect_glGetMaterialiv(GLenum face, GLenum pname, GLint *params);
void __indirect_glGetMinmax(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values);
void __indirect_glGetMinmaxParameterfv(GLenum target, GLenum pname, GLfloat *params);
void __indirect_glGetMinmaxParameteriv(GLenum target, GLenum pname, GLint *params);
void __indirect_glGetPixelMapfv(GLenum map, GLfloat *values);
void __indirect_glGetPixelMapuiv(GLenum map, GLuint *values);
void __indirect_glGetPixelMapusv(GLenum map, GLushort *values);
void __indirect_glGetPointerv(GLenum pname, void **params);
void __indirect_glGetPolygonStipple(GLubyte *mask);
const GLubyte *__indirect_glGetString(GLenum name);
void __indirect_glGetSeparableFilter(GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span);
void __indirect_glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params);
void __indirect_glGetTexEnviv(GLenum target, GLenum pname, GLint *params);
void __indirect_glGetTexGendv(GLenum coord, GLenum pname, GLdouble *params);
void __indirect_glGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params);
void __indirect_glGetTexGeniv(GLenum coord, GLenum pname, GLint *params);
void __indirect_glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *texels);
void __indirect_glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params);
void __indirect_glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params);
void __indirect_glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params);
void __indirect_glGetTexParameteriv(GLenum target, GLenum pname, GLint *params);
void __indirect_glHistogram(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink);
void __indirect_glIndexPointer(GLenum type, GLsizei stride, const GLvoid *pointer);
void __indirect_glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer);
GLboolean __indirect_glIsEnabled(GLenum cap);
GLboolean __indirect_glIsList(GLuint list);
GLboolean __indirect_glIsTexture(GLuint texture);
GLboolean __indirect_glIsTextureEXT(GLuint texture);
void __indirect_glLineStipple(GLint factor, GLushort pattern);
void __indirect_glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *pnts);
void __indirect_glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *pnts);
void __indirect_glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustr, GLint uord, GLdouble v1, GLdouble v2, GLint vstr, GLint vord, const GLdouble *pnts);
void __indirect_glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustr, GLint uord, GLfloat v1, GLfloat v2, GLint vstr, GLint vord, const GLfloat *pnts);
void __indirect_glMapGrid1d(GLint un, GLdouble u1, GLdouble u2);
void __indirect_glMapGrid1f(GLint un, GLfloat u1, GLfloat u2);
void __indirect_glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
void __indirect_glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
void __indirect_glNewList(GLuint list, GLenum mode);
void __indirect_glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer);
void __indirect_glPixelMapfv(GLenum map, GLint mapsize, const GLfloat *values);
void __indirect_glPixelMapuiv(GLenum map, GLint mapsize, const GLuint *values);
void __indirect_glPixelMapusv(GLenum map, GLint mapsize, const GLushort *values);
void __indirect_glPixelStoref(GLenum pname, GLfloat param);
void __indirect_glPixelStorei(GLenum pname, GLint param);
void __indirect_glPolygonStipple(const GLubyte *mask);
void __indirect_glPopClientAttrib(void);
void __indirect_glPrioritizeTextures(GLsizei n, const GLuint *textures, const GLclampf *priorities);
void __indirect_glPushClientAttrib(GLuint mask);
void __indirect_glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
void __indirect_glRectdv(const GLdouble *v1, const GLdouble *v2);
void __indirect_glRectfv(const GLfloat *v1, const GLfloat *v2);
void __indirect_glRectiv(const GLint *v1, const GLint *v2);
void __indirect_glRectsv(const GLshort *v1, const GLshort *v2);
GLint __indirect_glRenderMode(GLenum mode);
void __indirect_glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
void __indirect_glSelectBuffer(GLsizei numnames, GLuint *buffer);
void __indirect_glSeparableFilter2D(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column);
void __indirect_glStencilFunc(GLenum func, GLint ref, GLuint mask);
void __indirect_glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void __indirect_glTexImage1D(GLenum target, GLint level, GLint components, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *image);
void __indirect_glTexImage2D(GLenum target, GLint level, GLint components, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *image);
void __indirect_glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *image);
void __indirect_glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *image);
void __indirect_glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image);
void __indirect_glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *image);
void __indirect_glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void __indirect_glViewport(GLint x, GLint y, GLsizei width, GLsizei height);

void __indirect_glClientActiveTextureARB(GLenum texture);

void __indirect_glSampleCoverageARB( GLfloat value, GLboolean invert );

void __indirect_glWindowPos2dARB(GLdouble x, GLdouble y);
void __indirect_glWindowPos2iARB(GLint x, GLint y);
void __indirect_glWindowPos2fARB(GLfloat x, GLfloat y);
void __indirect_glWindowPos2sARB(GLshort x, GLshort y);
void __indirect_glWindowPos2dvARB(const GLdouble * p);
void __indirect_glWindowPos2fvARB(const GLfloat * p);
void __indirect_glWindowPos2ivARB(const GLint * p);
void __indirect_glWindowPos2svARB(const GLshort * p);
void __indirect_glWindowPos3dARB(GLdouble x, GLdouble y, GLdouble z);
void __indirect_glWindowPos3iARB(GLint x, GLint y, GLint z);
void __indirect_glWindowPos3sARB(GLshort x, GLshort y, GLshort z);
void __indirect_glWindowPos3dvARB(const GLdouble * p);
void __indirect_glWindowPos3ivARB(const GLint * p);
void __indirect_glWindowPos3svARB(const GLshort * p);

void __indirect_glMultiDrawArrays(GLenum mode, GLint *first, GLsizei *count, GLsizei primcount);
void __indirect_glMultiDrawElements(GLenum mode, const GLsizei *count, GLenum type, const GLvoid ** indices, GLsizei primcount);

void __indirect_glSampleMaskSGIS( GLfloat value, GLboolean invert );
void __indirect_glSamplePatternSGIS( GLenum pass );

/* ARB 12. GL_ARB_texture_compression / GL 1.3 */

void __indirect_glGetCompressedTexImage( GLenum target, GLint level,
    GLvoid * img );
void __indirect_glCompressedTexImage1D( GLenum target, GLint level,
    GLenum internalformat, GLsizei width,
    GLint border, GLsizei image_size, const GLvoid *data );
void __indirect_glCompressedTexImage2D( GLenum target, GLint level,
    GLenum internalformat, GLsizei width, GLsizei height,
    GLint border, GLsizei image_size, const GLvoid *data );
void __indirect_glCompressedTexImage3D( GLenum target, GLint level,
    GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth,
    GLint border, GLsizei image_size, const GLvoid *data );
void __indirect_glCompressedTexSubImage1D( GLenum target, GLint level,
    GLint xoffset,
    GLsizei width,
    GLenum format, GLsizei image_size, const GLvoid *data );
void __indirect_glCompressedTexSubImage2D( GLenum target, GLint level,
    GLint xoffset, GLint yoffset,
    GLsizei width, GLsizei height,
    GLenum format, GLsizei image_size, const GLvoid *data );
void __indirect_glCompressedTexSubImage3D( GLenum target, GLint level,
    GLint xoffset, GLint yoffset, GLint zoffset,
    GLsizei width, GLsizei height, GLsizei depth,
    GLenum format, GLsizei image_size, const GLvoid *data );

/* 145. GL_EXT_secondary_color / GL 1.4 */

void __indirect_glSecondaryColorPointer (GLint, GLenum, GLsizei, const GLvoid *);

/* 149. GL_EXT_fog_coord / GL 1.4 */

void __indirect_glFogCoordPointer (GLenum, GLsizei, const GLvoid *);

# undef glxproto_void
# undef glxproto_Cv
# undef glxproto_Cv_transpose
# undef glxproto_1s
# undef glxproto_2s
# undef glxproto_3s
# undef glxproto_4s
# undef glxproto_6s
# undef glxproto_enum1_1s
# undef glxproto_enum1_1v
# undef glxproto_enum1_2s
# undef glxproto_enum1_2v
# undef glxproto_enum1_3s
# undef glxproto_enum1_3v
# undef glxproto_enum1_4s
# undef glxproto_enum1_4v
# undef glxproto_enum1_Vv
# undef glxproto_enum2_1s
# undef glxproto_enum2_Vv
# undef glxproto_1
# undef glxvendr_1
# undef glxproto_2
# undef glxvendr_2
# undef glxproto_3
# undef glxvendr_3
# undef glxproto_4
# undef glxproto_enum1_1
# undef glxvendr_enum1_1
# undef glxproto_enum1_2
# undef glxvendr_enum1_2
# undef glxproto_enum1_3
# undef glxvendr_enum1_3
# undef glxproto_enum1_4
# undef glxvendr_enum1_4
# undef glxproto_enum1_V
# undef glxvendr_enum1_V
# undef glxproto_enum2_V
# undef glxvendr_enum2_V
#endif /* !defined( GENERATE_GLX_PROTOCOL_FUNCTIONS ) */

#endif /* _INDIRECT_H_ */
