/* DO NOT EDIT - This file generated automatically by remap_helper.py (from Mesa) script */

/*
 * Copyright (C) 2009 Chia-I Wu <olv@0xlab.org>
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
 * Chia-I Wu,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "main/dispatch.h"
#include "main/remap.h"

/* this is internal to remap.c */
#ifdef need_MESA_remap_table

static const char _mesa_function_pool[] =
   /* _mesa_function_pool[0]: MapGrid1d (offset 224) */
   "idd\0"
   "glMapGrid1d\0"
   "\0"
   /* _mesa_function_pool[17]: UniformMatrix3fvARB (will be remapped) */
   "iiip\0"
   "glUniformMatrix3fv\0"
   "glUniformMatrix3fvARB\0"
   "\0"
   /* _mesa_function_pool[64]: MapGrid1f (offset 225) */
   "iff\0"
   "glMapGrid1f\0"
   "\0"
   /* _mesa_function_pool[81]: VertexAttribI2iEXT (will be remapped) */
   "iii\0"
   "glVertexAttribI2iEXT\0"
   "glVertexAttribI2i\0"
   "\0"
   /* _mesa_function_pool[125]: RasterPos4i (offset 82) */
   "iiii\0"
   "glRasterPos4i\0"
   "\0"
   /* _mesa_function_pool[145]: RasterPos4d (offset 78) */
   "dddd\0"
   "glRasterPos4d\0"
   "\0"
   /* _mesa_function_pool[165]: NewList (dynamic) */
   "ii\0"
   "glNewList\0"
   "\0"
   /* _mesa_function_pool[179]: RasterPos4f (offset 80) */
   "ffff\0"
   "glRasterPos4f\0"
   "\0"
   /* _mesa_function_pool[199]: LoadIdentity (offset 290) */
   "\0"
   "glLoadIdentity\0"
   "\0"
   /* _mesa_function_pool[216]: SampleCoverageARB (will be remapped) */
   "fi\0"
   "glSampleCoverage\0"
   "glSampleCoverageARB\0"
   "\0"
   /* _mesa_function_pool[257]: ConvolutionFilter1D (offset 348) */
   "iiiiip\0"
   "glConvolutionFilter1D\0"
   "glConvolutionFilter1DEXT\0"
   "\0"
   /* _mesa_function_pool[312]: BeginQueryARB (will be remapped) */
   "ii\0"
   "glBeginQuery\0"
   "glBeginQueryARB\0"
   "\0"
   /* _mesa_function_pool[345]: RasterPos3dv (offset 71) */
   "p\0"
   "glRasterPos3dv\0"
   "\0"
   /* _mesa_function_pool[363]: PointParameteriNV (will be remapped) */
   "ii\0"
   "glPointParameteri\0"
   "glPointParameteriNV\0"
   "\0"
   /* _mesa_function_pool[405]: GetProgramiv (will be remapped) */
   "iip\0"
   "glGetProgramiv\0"
   "\0"
   /* _mesa_function_pool[425]: MultiTexCoord3sARB (offset 398) */
   "iiii\0"
   "glMultiTexCoord3s\0"
   "glMultiTexCoord3sARB\0"
   "\0"
   /* _mesa_function_pool[470]: SecondaryColor3iEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3i\0"
   "glSecondaryColor3iEXT\0"
   "\0"
   /* _mesa_function_pool[516]: WindowPos3fMESA (will be remapped) */
   "fff\0"
   "glWindowPos3f\0"
   "glWindowPos3fARB\0"
   "glWindowPos3fMESA\0"
   "\0"
   /* _mesa_function_pool[570]: TexCoord1iv (offset 99) */
   "p\0"
   "glTexCoord1iv\0"
   "\0"
   /* _mesa_function_pool[587]: TexCoord4sv (offset 125) */
   "p\0"
   "glTexCoord4sv\0"
   "\0"
   /* _mesa_function_pool[604]: RasterPos4s (offset 84) */
   "iiii\0"
   "glRasterPos4s\0"
   "\0"
   /* _mesa_function_pool[624]: PixelTexGenParameterfvSGIS (will be remapped) */
   "ip\0"
   "glPixelTexGenParameterfvSGIS\0"
   "\0"
   /* _mesa_function_pool[657]: ActiveTextureARB (offset 374) */
   "i\0"
   "glActiveTexture\0"
   "glActiveTextureARB\0"
   "\0"
   /* _mesa_function_pool[695]: BlitFramebufferEXT (will be remapped) */
   "iiiiiiiiii\0"
   "glBlitFramebuffer\0"
   "glBlitFramebufferEXT\0"
   "\0"
   /* _mesa_function_pool[746]: TexCoord1f (offset 96) */
   "f\0"
   "glTexCoord1f\0"
   "\0"
   /* _mesa_function_pool[762]: TexCoord1d (offset 94) */
   "d\0"
   "glTexCoord1d\0"
   "\0"
   /* _mesa_function_pool[778]: VertexAttrib4ubvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib4ubvNV\0"
   "\0"
   /* _mesa_function_pool[803]: TexCoord1i (offset 98) */
   "i\0"
   "glTexCoord1i\0"
   "\0"
   /* _mesa_function_pool[819]: GetProgramNamedParameterdvNV (will be remapped) */
   "iipp\0"
   "glGetProgramNamedParameterdvNV\0"
   "\0"
   /* _mesa_function_pool[856]: Histogram (offset 367) */
   "iiii\0"
   "glHistogram\0"
   "glHistogramEXT\0"
   "\0"
   /* _mesa_function_pool[889]: TexCoord1s (offset 100) */
   "i\0"
   "glTexCoord1s\0"
   "\0"
   /* _mesa_function_pool[905]: GetMapfv (offset 267) */
   "iip\0"
   "glGetMapfv\0"
   "\0"
   /* _mesa_function_pool[921]: EvalCoord1f (offset 230) */
   "f\0"
   "glEvalCoord1f\0"
   "\0"
   /* _mesa_function_pool[938]: FramebufferTexture (will be remapped) */
   "iiii\0"
   "glFramebufferTexture\0"
   "\0"
   /* _mesa_function_pool[965]: VertexAttribI1ivEXT (will be remapped) */
   "ip\0"
   "glVertexAttribI1ivEXT\0"
   "glVertexAttribI1iv\0"
   "\0"
   /* _mesa_function_pool[1010]: TexImage4DSGIS (dynamic) */
   "iiiiiiiiiip\0"
   "glTexImage4DSGIS\0"
   "\0"
   /* _mesa_function_pool[1040]: PolygonStipple (offset 175) */
   "p\0"
   "glPolygonStipple\0"
   "\0"
   /* _mesa_function_pool[1060]: WindowPos2dvMESA (will be remapped) */
   "p\0"
   "glWindowPos2dv\0"
   "glWindowPos2dvARB\0"
   "glWindowPos2dvMESA\0"
   "\0"
   /* _mesa_function_pool[1115]: ReplacementCodeuiColor3fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glReplacementCodeuiColor3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[1159]: BlendEquationSeparateEXT (will be remapped) */
   "ii\0"
   "glBlendEquationSeparate\0"
   "glBlendEquationSeparateEXT\0"
   "glBlendEquationSeparateATI\0"
   "\0"
   /* _mesa_function_pool[1241]: ListParameterfSGIX (dynamic) */
   "iif\0"
   "glListParameterfSGIX\0"
   "\0"
   /* _mesa_function_pool[1267]: SecondaryColor3bEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3b\0"
   "glSecondaryColor3bEXT\0"
   "\0"
   /* _mesa_function_pool[1313]: TexCoord4fColor4fNormal3fVertex4fvSUN (dynamic) */
   "pppp\0"
   "glTexCoord4fColor4fNormal3fVertex4fvSUN\0"
   "\0"
   /* _mesa_function_pool[1359]: GetPixelMapfv (offset 271) */
   "ip\0"
   "glGetPixelMapfv\0"
   "\0"
   /* _mesa_function_pool[1379]: Color3uiv (offset 22) */
   "p\0"
   "glColor3uiv\0"
   "\0"
   /* _mesa_function_pool[1394]: IsEnabled (offset 286) */
   "i\0"
   "glIsEnabled\0"
   "\0"
   /* _mesa_function_pool[1409]: VertexAttrib4svNV (will be remapped) */
   "ip\0"
   "glVertexAttrib4svNV\0"
   "\0"
   /* _mesa_function_pool[1433]: EvalCoord2fv (offset 235) */
   "p\0"
   "glEvalCoord2fv\0"
   "\0"
   /* _mesa_function_pool[1451]: GetBufferSubDataARB (will be remapped) */
   "iiip\0"
   "glGetBufferSubData\0"
   "glGetBufferSubDataARB\0"
   "\0"
   /* _mesa_function_pool[1498]: BufferSubDataARB (will be remapped) */
   "iiip\0"
   "glBufferSubData\0"
   "glBufferSubDataARB\0"
   "\0"
   /* _mesa_function_pool[1539]: TexCoord2fColor4ubVertex3fvSUN (dynamic) */
   "ppp\0"
   "glTexCoord2fColor4ubVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[1577]: AttachShader (will be remapped) */
   "ii\0"
   "glAttachShader\0"
   "\0"
   /* _mesa_function_pool[1596]: VertexAttrib2fARB (will be remapped) */
   "iff\0"
   "glVertexAttrib2f\0"
   "glVertexAttrib2fARB\0"
   "\0"
   /* _mesa_function_pool[1638]: GetDebugLogLengthMESA (dynamic) */
   "iii\0"
   "glGetDebugLogLengthMESA\0"
   "\0"
   /* _mesa_function_pool[1667]: GetMapiv (offset 268) */
   "iip\0"
   "glGetMapiv\0"
   "\0"
   /* _mesa_function_pool[1683]: VertexAttrib3fARB (will be remapped) */
   "ifff\0"
   "glVertexAttrib3f\0"
   "glVertexAttrib3fARB\0"
   "\0"
   /* _mesa_function_pool[1726]: Indexubv (offset 316) */
   "p\0"
   "glIndexubv\0"
   "\0"
   /* _mesa_function_pool[1740]: GetQueryivARB (will be remapped) */
   "iip\0"
   "glGetQueryiv\0"
   "glGetQueryivARB\0"
   "\0"
   /* _mesa_function_pool[1774]: TexImage3D (offset 371) */
   "iiiiiiiiip\0"
   "glTexImage3D\0"
   "glTexImage3DEXT\0"
   "\0"
   /* _mesa_function_pool[1815]: BindFragDataLocationEXT (will be remapped) */
   "iip\0"
   "glBindFragDataLocationEXT\0"
   "glBindFragDataLocation\0"
   "\0"
   /* _mesa_function_pool[1869]: ReplacementCodeuiVertex3fvSUN (dynamic) */
   "pp\0"
   "glReplacementCodeuiVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[1905]: EdgeFlagPointer (offset 312) */
   "ip\0"
   "glEdgeFlagPointer\0"
   "\0"
   /* _mesa_function_pool[1927]: Color3ubv (offset 20) */
   "p\0"
   "glColor3ubv\0"
   "\0"
   /* _mesa_function_pool[1942]: GetQueryObjectivARB (will be remapped) */
   "iip\0"
   "glGetQueryObjectiv\0"
   "glGetQueryObjectivARB\0"
   "\0"
   /* _mesa_function_pool[1988]: Vertex3dv (offset 135) */
   "p\0"
   "glVertex3dv\0"
   "\0"
   /* _mesa_function_pool[2003]: ReplacementCodeuiTexCoord2fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glReplacementCodeuiTexCoord2fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[2050]: CompressedTexSubImage2DARB (will be remapped) */
   "iiiiiiiip\0"
   "glCompressedTexSubImage2D\0"
   "glCompressedTexSubImage2DARB\0"
   "\0"
   /* _mesa_function_pool[2116]: CombinerOutputNV (will be remapped) */
   "iiiiiiiiii\0"
   "glCombinerOutputNV\0"
   "\0"
   /* _mesa_function_pool[2147]: VertexAttribs3fvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs3fvNV\0"
   "\0"
   /* _mesa_function_pool[2173]: Uniform2fARB (will be remapped) */
   "iff\0"
   "glUniform2f\0"
   "glUniform2fARB\0"
   "\0"
   /* _mesa_function_pool[2205]: LightModeliv (offset 166) */
   "ip\0"
   "glLightModeliv\0"
   "\0"
   /* _mesa_function_pool[2224]: VertexAttrib1svARB (will be remapped) */
   "ip\0"
   "glVertexAttrib1sv\0"
   "glVertexAttrib1svARB\0"
   "\0"
   /* _mesa_function_pool[2267]: VertexAttribs1dvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs1dvNV\0"
   "\0"
   /* _mesa_function_pool[2293]: Uniform2ivARB (will be remapped) */
   "iip\0"
   "glUniform2iv\0"
   "glUniform2ivARB\0"
   "\0"
   /* _mesa_function_pool[2327]: GetImageTransformParameterfvHP (dynamic) */
   "iip\0"
   "glGetImageTransformParameterfvHP\0"
   "\0"
   /* _mesa_function_pool[2365]: Normal3bv (offset 53) */
   "p\0"
   "glNormal3bv\0"
   "\0"
   /* _mesa_function_pool[2380]: TexGeniv (offset 193) */
   "iip\0"
   "glTexGeniv\0"
   "\0"
   /* _mesa_function_pool[2396]: WeightubvARB (dynamic) */
   "ip\0"
   "glWeightubvARB\0"
   "\0"
   /* _mesa_function_pool[2415]: VertexAttrib1fvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib1fvNV\0"
   "\0"
   /* _mesa_function_pool[2439]: Vertex3iv (offset 139) */
   "p\0"
   "glVertex3iv\0"
   "\0"
   /* _mesa_function_pool[2454]: CopyConvolutionFilter1D (offset 354) */
   "iiiii\0"
   "glCopyConvolutionFilter1D\0"
   "glCopyConvolutionFilter1DEXT\0"
   "\0"
   /* _mesa_function_pool[2516]: VertexAttribI1uiEXT (will be remapped) */
   "ii\0"
   "glVertexAttribI1uiEXT\0"
   "glVertexAttribI1ui\0"
   "\0"
   /* _mesa_function_pool[2561]: ReplacementCodeuiNormal3fVertex3fSUN (dynamic) */
   "iffffff\0"
   "glReplacementCodeuiNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[2609]: DeleteSync (will be remapped) */
   "i\0"
   "glDeleteSync\0"
   "\0"
   /* _mesa_function_pool[2625]: FragmentMaterialfvSGIX (dynamic) */
   "iip\0"
   "glFragmentMaterialfvSGIX\0"
   "\0"
   /* _mesa_function_pool[2655]: BlendColor (offset 336) */
   "ffff\0"
   "glBlendColor\0"
   "glBlendColorEXT\0"
   "\0"
   /* _mesa_function_pool[2690]: UniformMatrix4fvARB (will be remapped) */
   "iiip\0"
   "glUniformMatrix4fv\0"
   "glUniformMatrix4fvARB\0"
   "\0"
   /* _mesa_function_pool[2737]: DeleteVertexArraysAPPLE (will be remapped) */
   "ip\0"
   "glDeleteVertexArrays\0"
   "glDeleteVertexArraysAPPLE\0"
   "\0"
   /* _mesa_function_pool[2788]: TexBuffer (will be remapped) */
   "iii\0"
   "glTexBuffer\0"
   "\0"
   /* _mesa_function_pool[2805]: ReadInstrumentsSGIX (dynamic) */
   "i\0"
   "glReadInstrumentsSGIX\0"
   "\0"
   /* _mesa_function_pool[2830]: CallLists (offset 3) */
   "iip\0"
   "glCallLists\0"
   "\0"
   /* _mesa_function_pool[2847]: UniformMatrix2x4fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix2x4fv\0"
   "\0"
   /* _mesa_function_pool[2874]: Color4ubVertex3fvSUN (dynamic) */
   "pp\0"
   "glColor4ubVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[2901]: Normal3iv (offset 59) */
   "p\0"
   "glNormal3iv\0"
   "\0"
   /* _mesa_function_pool[2916]: PassThrough (offset 199) */
   "f\0"
   "glPassThrough\0"
   "\0"
   /* _mesa_function_pool[2933]: GetVertexAttribIivEXT (will be remapped) */
   "iip\0"
   "glGetVertexAttribIivEXT\0"
   "glGetVertexAttribIiv\0"
   "\0"
   /* _mesa_function_pool[2983]: TexParameterIivEXT (will be remapped) */
   "iip\0"
   "glTexParameterIivEXT\0"
   "glTexParameterIiv\0"
   "\0"
   /* _mesa_function_pool[3027]: FramebufferTextureLayerEXT (will be remapped) */
   "iiiii\0"
   "glFramebufferTextureLayer\0"
   "glFramebufferTextureLayerEXT\0"
   "\0"
   /* _mesa_function_pool[3089]: GetListParameterfvSGIX (dynamic) */
   "iip\0"
   "glGetListParameterfvSGIX\0"
   "\0"
   /* _mesa_function_pool[3119]: Viewport (offset 305) */
   "iiii\0"
   "glViewport\0"
   "\0"
   /* _mesa_function_pool[3136]: VertexAttrib4NusvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Nusv\0"
   "glVertexAttrib4NusvARB\0"
   "\0"
   /* _mesa_function_pool[3183]: WindowPos4svMESA (will be remapped) */
   "p\0"
   "glWindowPos4svMESA\0"
   "\0"
   /* _mesa_function_pool[3205]: CreateProgramObjectARB (will be remapped) */
   "\0"
   "glCreateProgramObjectARB\0"
   "\0"
   /* _mesa_function_pool[3232]: DeleteTransformFeedbacks (will be remapped) */
   "ip\0"
   "glDeleteTransformFeedbacks\0"
   "\0"
   /* _mesa_function_pool[3263]: UniformMatrix4x3fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix4x3fv\0"
   "\0"
   /* _mesa_function_pool[3290]: PrioritizeTextures (offset 331) */
   "ipp\0"
   "glPrioritizeTextures\0"
   "glPrioritizeTexturesEXT\0"
   "\0"
   /* _mesa_function_pool[3340]: VertexAttribI3uiEXT (will be remapped) */
   "iiii\0"
   "glVertexAttribI3uiEXT\0"
   "glVertexAttribI3ui\0"
   "\0"
   /* _mesa_function_pool[3387]: AsyncMarkerSGIX (dynamic) */
   "i\0"
   "glAsyncMarkerSGIX\0"
   "\0"
   /* _mesa_function_pool[3408]: GlobalAlphaFactorubSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactorubSUN\0"
   "\0"
   /* _mesa_function_pool[3436]: ClearColorIuiEXT (will be remapped) */
   "iiii\0"
   "glClearColorIuiEXT\0"
   "\0"
   /* _mesa_function_pool[3461]: ClearDebugLogMESA (dynamic) */
   "iii\0"
   "glClearDebugLogMESA\0"
   "\0"
   /* _mesa_function_pool[3486]: Uniform4uiEXT (will be remapped) */
   "iiiii\0"
   "glUniform4uiEXT\0"
   "glUniform4ui\0"
   "\0"
   /* _mesa_function_pool[3522]: ResetHistogram (offset 369) */
   "i\0"
   "glResetHistogram\0"
   "glResetHistogramEXT\0"
   "\0"
   /* _mesa_function_pool[3562]: GetProgramNamedParameterfvNV (will be remapped) */
   "iipp\0"
   "glGetProgramNamedParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[3599]: PointParameterfEXT (will be remapped) */
   "if\0"
   "glPointParameterf\0"
   "glPointParameterfARB\0"
   "glPointParameterfEXT\0"
   "glPointParameterfSGIS\0"
   "\0"
   /* _mesa_function_pool[3685]: LoadIdentityDeformationMapSGIX (dynamic) */
   "i\0"
   "glLoadIdentityDeformationMapSGIX\0"
   "\0"
   /* _mesa_function_pool[3721]: GenFencesNV (will be remapped) */
   "ip\0"
   "glGenFencesNV\0"
   "\0"
   /* _mesa_function_pool[3739]: ImageTransformParameterfHP (dynamic) */
   "iif\0"
   "glImageTransformParameterfHP\0"
   "\0"
   /* _mesa_function_pool[3773]: MatrixIndexusvARB (dynamic) */
   "ip\0"
   "glMatrixIndexusvARB\0"
   "\0"
   /* _mesa_function_pool[3797]: DrawElementsBaseVertex (will be remapped) */
   "iiipi\0"
   "glDrawElementsBaseVertex\0"
   "\0"
   /* _mesa_function_pool[3829]: DisableVertexAttribArrayARB (will be remapped) */
   "i\0"
   "glDisableVertexAttribArray\0"
   "glDisableVertexAttribArrayARB\0"
   "\0"
   /* _mesa_function_pool[3889]: TexCoord2sv (offset 109) */
   "p\0"
   "glTexCoord2sv\0"
   "\0"
   /* _mesa_function_pool[3906]: Vertex4dv (offset 143) */
   "p\0"
   "glVertex4dv\0"
   "\0"
   /* _mesa_function_pool[3921]: StencilMaskSeparate (will be remapped) */
   "ii\0"
   "glStencilMaskSeparate\0"
   "\0"
   /* _mesa_function_pool[3947]: ProgramLocalParameter4dARB (will be remapped) */
   "iidddd\0"
   "glProgramLocalParameter4dARB\0"
   "\0"
   /* _mesa_function_pool[3984]: CompressedTexImage3DARB (will be remapped) */
   "iiiiiiiip\0"
   "glCompressedTexImage3D\0"
   "glCompressedTexImage3DARB\0"
   "\0"
   /* _mesa_function_pool[4044]: Color3sv (offset 18) */
   "p\0"
   "glColor3sv\0"
   "\0"
   /* _mesa_function_pool[4058]: GetConvolutionParameteriv (offset 358) */
   "iip\0"
   "glGetConvolutionParameteriv\0"
   "glGetConvolutionParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[4122]: VertexAttrib1fARB (will be remapped) */
   "if\0"
   "glVertexAttrib1f\0"
   "glVertexAttrib1fARB\0"
   "\0"
   /* _mesa_function_pool[4163]: Vertex2dv (offset 127) */
   "p\0"
   "glVertex2dv\0"
   "\0"
   /* _mesa_function_pool[4178]: TestFenceNV (will be remapped) */
   "i\0"
   "glTestFenceNV\0"
   "\0"
   /* _mesa_function_pool[4195]: GetVertexAttribIuivEXT (will be remapped) */
   "iip\0"
   "glGetVertexAttribIuivEXT\0"
   "glGetVertexAttribIuiv\0"
   "\0"
   /* _mesa_function_pool[4247]: MultiTexCoord1fvARB (offset 379) */
   "ip\0"
   "glMultiTexCoord1fv\0"
   "glMultiTexCoord1fvARB\0"
   "\0"
   /* _mesa_function_pool[4292]: TexCoord3iv (offset 115) */
   "p\0"
   "glTexCoord3iv\0"
   "\0"
   /* _mesa_function_pool[4309]: Uniform2uivEXT (will be remapped) */
   "iip\0"
   "glUniform2uivEXT\0"
   "glUniform2uiv\0"
   "\0"
   /* _mesa_function_pool[4345]: ColorFragmentOp2ATI (will be remapped) */
   "iiiiiiiiii\0"
   "glColorFragmentOp2ATI\0"
   "\0"
   /* _mesa_function_pool[4379]: SecondaryColorPointerListIBM (dynamic) */
   "iiipi\0"
   "glSecondaryColorPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[4417]: GetPixelTexGenParameterivSGIS (will be remapped) */
   "ip\0"
   "glGetPixelTexGenParameterivSGIS\0"
   "\0"
   /* _mesa_function_pool[4453]: Color3fv (offset 14) */
   "p\0"
   "glColor3fv\0"
   "\0"
   /* _mesa_function_pool[4467]: VertexAttrib4fNV (will be remapped) */
   "iffff\0"
   "glVertexAttrib4fNV\0"
   "\0"
   /* _mesa_function_pool[4493]: ReplacementCodeubSUN (dynamic) */
   "i\0"
   "glReplacementCodeubSUN\0"
   "\0"
   /* _mesa_function_pool[4519]: FinishAsyncSGIX (dynamic) */
   "p\0"
   "glFinishAsyncSGIX\0"
   "\0"
   /* _mesa_function_pool[4540]: GetDebugLogMESA (dynamic) */
   "iiiipp\0"
   "glGetDebugLogMESA\0"
   "\0"
   /* _mesa_function_pool[4566]: FogCoorddEXT (will be remapped) */
   "d\0"
   "glFogCoordd\0"
   "glFogCoorddEXT\0"
   "\0"
   /* _mesa_function_pool[4596]: BeginConditionalRenderNV (will be remapped) */
   "ii\0"
   "glBeginConditionalRenderNV\0"
   "glBeginConditionalRender\0"
   "\0"
   /* _mesa_function_pool[4652]: Color4ubVertex3fSUN (dynamic) */
   "iiiifff\0"
   "glColor4ubVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[4683]: FogCoordfEXT (will be remapped) */
   "f\0"
   "glFogCoordf\0"
   "glFogCoordfEXT\0"
   "\0"
   /* _mesa_function_pool[4713]: PointSize (offset 173) */
   "f\0"
   "glPointSize\0"
   "\0"
   /* _mesa_function_pool[4728]: VertexAttribI2uivEXT (will be remapped) */
   "ip\0"
   "glVertexAttribI2uivEXT\0"
   "glVertexAttribI2uiv\0"
   "\0"
   /* _mesa_function_pool[4775]: TexCoord2fVertex3fSUN (dynamic) */
   "fffff\0"
   "glTexCoord2fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[4806]: PopName (offset 200) */
   "\0"
   "glPopName\0"
   "\0"
   /* _mesa_function_pool[4818]: GlobalAlphaFactoriSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactoriSUN\0"
   "\0"
   /* _mesa_function_pool[4845]: VertexAttrib2dNV (will be remapped) */
   "idd\0"
   "glVertexAttrib2dNV\0"
   "\0"
   /* _mesa_function_pool[4869]: GetProgramInfoLog (will be remapped) */
   "iipp\0"
   "glGetProgramInfoLog\0"
   "\0"
   /* _mesa_function_pool[4895]: VertexAttrib4NbvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Nbv\0"
   "glVertexAttrib4NbvARB\0"
   "\0"
   /* _mesa_function_pool[4940]: GetActiveAttribARB (will be remapped) */
   "iiipppp\0"
   "glGetActiveAttrib\0"
   "glGetActiveAttribARB\0"
   "\0"
   /* _mesa_function_pool[4988]: Vertex4sv (offset 149) */
   "p\0"
   "glVertex4sv\0"
   "\0"
   /* _mesa_function_pool[5003]: VertexAttrib4ubNV (will be remapped) */
   "iiiii\0"
   "glVertexAttrib4ubNV\0"
   "\0"
   /* _mesa_function_pool[5030]: ClampColor (will be remapped) */
   "ii\0"
   "glClampColor\0"
   "\0"
   /* _mesa_function_pool[5047]: TextureRangeAPPLE (will be remapped) */
   "iip\0"
   "glTextureRangeAPPLE\0"
   "\0"
   /* _mesa_function_pool[5072]: GetTexEnvfv (offset 276) */
   "iip\0"
   "glGetTexEnvfv\0"
   "\0"
   /* _mesa_function_pool[5091]: BindTransformFeedback (will be remapped) */
   "ii\0"
   "glBindTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[5119]: TexCoord2fColor4fNormal3fVertex3fSUN (dynamic) */
   "ffffffffffff\0"
   "glTexCoord2fColor4fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[5172]: Indexub (offset 315) */
   "i\0"
   "glIndexub\0"
   "\0"
   /* _mesa_function_pool[5185]: TexEnvi (offset 186) */
   "iii\0"
   "glTexEnvi\0"
   "\0"
   /* _mesa_function_pool[5200]: GetClipPlane (offset 259) */
   "ip\0"
   "glGetClipPlane\0"
   "\0"
   /* _mesa_function_pool[5219]: CombinerParameterfvNV (will be remapped) */
   "ip\0"
   "glCombinerParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[5247]: VertexAttribs3dvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs3dvNV\0"
   "\0"
   /* _mesa_function_pool[5273]: VertexAttribI2uiEXT (will be remapped) */
   "iii\0"
   "glVertexAttribI2uiEXT\0"
   "glVertexAttribI2ui\0"
   "\0"
   /* _mesa_function_pool[5319]: VertexAttribs4fvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs4fvNV\0"
   "\0"
   /* _mesa_function_pool[5345]: VertexArrayRangeNV (will be remapped) */
   "ip\0"
   "glVertexArrayRangeNV\0"
   "\0"
   /* _mesa_function_pool[5370]: FragmentLightiSGIX (dynamic) */
   "iii\0"
   "glFragmentLightiSGIX\0"
   "\0"
   /* _mesa_function_pool[5396]: PolygonOffsetEXT (will be remapped) */
   "ff\0"
   "glPolygonOffsetEXT\0"
   "\0"
   /* _mesa_function_pool[5419]: VertexAttribI4uivEXT (will be remapped) */
   "ip\0"
   "glVertexAttribI4uivEXT\0"
   "glVertexAttribI4uiv\0"
   "\0"
   /* _mesa_function_pool[5466]: PollAsyncSGIX (dynamic) */
   "p\0"
   "glPollAsyncSGIX\0"
   "\0"
   /* _mesa_function_pool[5485]: DeleteFragmentShaderATI (will be remapped) */
   "i\0"
   "glDeleteFragmentShaderATI\0"
   "\0"
   /* _mesa_function_pool[5514]: Scaled (offset 301) */
   "ddd\0"
   "glScaled\0"
   "\0"
   /* _mesa_function_pool[5528]: ResumeTransformFeedback (will be remapped) */
   "\0"
   "glResumeTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[5556]: Scalef (offset 302) */
   "fff\0"
   "glScalef\0"
   "\0"
   /* _mesa_function_pool[5570]: TexCoord2fNormal3fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glTexCoord2fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[5608]: MultTransposeMatrixdARB (will be remapped) */
   "p\0"
   "glMultTransposeMatrixd\0"
   "glMultTransposeMatrixdARB\0"
   "\0"
   /* _mesa_function_pool[5660]: ColorMaskIndexedEXT (will be remapped) */
   "iiiii\0"
   "glColorMaskIndexedEXT\0"
   "glColorMaski\0"
   "\0"
   /* _mesa_function_pool[5702]: ObjectUnpurgeableAPPLE (will be remapped) */
   "iii\0"
   "glObjectUnpurgeableAPPLE\0"
   "\0"
   /* _mesa_function_pool[5732]: AlphaFunc (offset 240) */
   "if\0"
   "glAlphaFunc\0"
   "\0"
   /* _mesa_function_pool[5748]: WindowPos2svMESA (will be remapped) */
   "p\0"
   "glWindowPos2sv\0"
   "glWindowPos2svARB\0"
   "glWindowPos2svMESA\0"
   "\0"
   /* _mesa_function_pool[5803]: EdgeFlag (offset 41) */
   "i\0"
   "glEdgeFlag\0"
   "\0"
   /* _mesa_function_pool[5817]: TexCoord2iv (offset 107) */
   "p\0"
   "glTexCoord2iv\0"
   "\0"
   /* _mesa_function_pool[5834]: CompressedTexImage1DARB (will be remapped) */
   "iiiiiip\0"
   "glCompressedTexImage1D\0"
   "glCompressedTexImage1DARB\0"
   "\0"
   /* _mesa_function_pool[5892]: Rotated (offset 299) */
   "dddd\0"
   "glRotated\0"
   "\0"
   /* _mesa_function_pool[5908]: GetTexParameterIuivEXT (will be remapped) */
   "iip\0"
   "glGetTexParameterIuivEXT\0"
   "glGetTexParameterIuiv\0"
   "\0"
   /* _mesa_function_pool[5960]: VertexAttrib2sNV (will be remapped) */
   "iii\0"
   "glVertexAttrib2sNV\0"
   "\0"
   /* _mesa_function_pool[5984]: ReadPixels (offset 256) */
   "iiiiiip\0"
   "glReadPixels\0"
   "\0"
   /* _mesa_function_pool[6006]: EdgeFlagv (offset 42) */
   "p\0"
   "glEdgeFlagv\0"
   "\0"
   /* _mesa_function_pool[6021]: NormalPointerListIBM (dynamic) */
   "iipi\0"
   "glNormalPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[6050]: IndexPointerEXT (will be remapped) */
   "iiip\0"
   "glIndexPointerEXT\0"
   "\0"
   /* _mesa_function_pool[6074]: Color4iv (offset 32) */
   "p\0"
   "glColor4iv\0"
   "\0"
   /* _mesa_function_pool[6088]: TexParameterf (offset 178) */
   "iif\0"
   "glTexParameterf\0"
   "\0"
   /* _mesa_function_pool[6109]: TexParameteri (offset 180) */
   "iii\0"
   "glTexParameteri\0"
   "\0"
   /* _mesa_function_pool[6130]: NormalPointerEXT (will be remapped) */
   "iiip\0"
   "glNormalPointerEXT\0"
   "\0"
   /* _mesa_function_pool[6155]: MultiTexCoord3dARB (offset 392) */
   "iddd\0"
   "glMultiTexCoord3d\0"
   "glMultiTexCoord3dARB\0"
   "\0"
   /* _mesa_function_pool[6200]: MultiTexCoord2iARB (offset 388) */
   "iii\0"
   "glMultiTexCoord2i\0"
   "glMultiTexCoord2iARB\0"
   "\0"
   /* _mesa_function_pool[6244]: DrawPixels (offset 257) */
   "iiiip\0"
   "glDrawPixels\0"
   "\0"
   /* _mesa_function_pool[6264]: ReplacementCodeuiTexCoord2fNormal3fVertex3fSUN (dynamic) */
   "iffffffff\0"
   "glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[6324]: MultiTexCoord2svARB (offset 391) */
   "ip\0"
   "glMultiTexCoord2sv\0"
   "glMultiTexCoord2svARB\0"
   "\0"
   /* _mesa_function_pool[6369]: ReplacementCodeubvSUN (dynamic) */
   "p\0"
   "glReplacementCodeubvSUN\0"
   "\0"
   /* _mesa_function_pool[6396]: Uniform3iARB (will be remapped) */
   "iiii\0"
   "glUniform3i\0"
   "glUniform3iARB\0"
   "\0"
   /* _mesa_function_pool[6429]: DrawTransformFeedback (will be remapped) */
   "ii\0"
   "glDrawTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[6457]: DrawElementsInstancedARB (will be remapped) */
   "iiipi\0"
   "glDrawElementsInstancedARB\0"
   "glDrawElementsInstancedEXT\0"
   "glDrawElementsInstanced\0"
   "\0"
   /* _mesa_function_pool[6542]: GetShaderInfoLog (will be remapped) */
   "iipp\0"
   "glGetShaderInfoLog\0"
   "\0"
   /* _mesa_function_pool[6567]: WeightivARB (dynamic) */
   "ip\0"
   "glWeightivARB\0"
   "\0"
   /* _mesa_function_pool[6585]: PollInstrumentsSGIX (dynamic) */
   "p\0"
   "glPollInstrumentsSGIX\0"
   "\0"
   /* _mesa_function_pool[6610]: GlobalAlphaFactordSUN (dynamic) */
   "d\0"
   "glGlobalAlphaFactordSUN\0"
   "\0"
   /* _mesa_function_pool[6637]: GetFinalCombinerInputParameterfvNV (will be remapped) */
   "iip\0"
   "glGetFinalCombinerInputParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[6679]: GenerateMipmapEXT (will be remapped) */
   "i\0"
   "glGenerateMipmap\0"
   "glGenerateMipmapEXT\0"
   "\0"
   /* _mesa_function_pool[6719]: GenLists (offset 5) */
   "i\0"
   "glGenLists\0"
   "\0"
   /* _mesa_function_pool[6733]: DepthRangef (will be remapped) */
   "ff\0"
   "glDepthRangef\0"
   "\0"
   /* _mesa_function_pool[6751]: GetMapAttribParameterivNV (dynamic) */
   "iiip\0"
   "glGetMapAttribParameterivNV\0"
   "\0"
   /* _mesa_function_pool[6785]: CreateShaderObjectARB (will be remapped) */
   "i\0"
   "glCreateShaderObjectARB\0"
   "\0"
   /* _mesa_function_pool[6812]: GetSharpenTexFuncSGIS (dynamic) */
   "ip\0"
   "glGetSharpenTexFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[6840]: BufferDataARB (will be remapped) */
   "iipi\0"
   "glBufferData\0"
   "glBufferDataARB\0"
   "\0"
   /* _mesa_function_pool[6875]: FlushVertexArrayRangeNV (will be remapped) */
   "\0"
   "glFlushVertexArrayRangeNV\0"
   "\0"
   /* _mesa_function_pool[6903]: MapGrid2d (offset 226) */
   "iddidd\0"
   "glMapGrid2d\0"
   "\0"
   /* _mesa_function_pool[6923]: MapGrid2f (offset 227) */
   "iffiff\0"
   "glMapGrid2f\0"
   "\0"
   /* _mesa_function_pool[6943]: SampleMapATI (will be remapped) */
   "iii\0"
   "glSampleMapATI\0"
   "\0"
   /* _mesa_function_pool[6963]: VertexPointerEXT (will be remapped) */
   "iiiip\0"
   "glVertexPointerEXT\0"
   "\0"
   /* _mesa_function_pool[6989]: GetTexFilterFuncSGIS (dynamic) */
   "iip\0"
   "glGetTexFilterFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[7017]: Scissor (offset 176) */
   "iiii\0"
   "glScissor\0"
   "\0"
   /* _mesa_function_pool[7033]: Fogf (offset 153) */
   "if\0"
   "glFogf\0"
   "\0"
   /* _mesa_function_pool[7044]: ReplacementCodeuiColor4ubVertex3fvSUN (dynamic) */
   "ppp\0"
   "glReplacementCodeuiColor4ubVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[7089]: TexSubImage1D (offset 332) */
   "iiiiiip\0"
   "glTexSubImage1D\0"
   "glTexSubImage1DEXT\0"
   "\0"
   /* _mesa_function_pool[7133]: VertexAttrib1sARB (will be remapped) */
   "ii\0"
   "glVertexAttrib1s\0"
   "glVertexAttrib1sARB\0"
   "\0"
   /* _mesa_function_pool[7174]: FenceSync (will be remapped) */
   "ii\0"
   "glFenceSync\0"
   "\0"
   /* _mesa_function_pool[7190]: Color4usv (offset 40) */
   "p\0"
   "glColor4usv\0"
   "\0"
   /* _mesa_function_pool[7205]: Fogi (offset 155) */
   "ii\0"
   "glFogi\0"
   "\0"
   /* _mesa_function_pool[7216]: DepthRange (offset 288) */
   "dd\0"
   "glDepthRange\0"
   "\0"
   /* _mesa_function_pool[7233]: RasterPos3iv (offset 75) */
   "p\0"
   "glRasterPos3iv\0"
   "\0"
   /* _mesa_function_pool[7251]: FinalCombinerInputNV (will be remapped) */
   "iiii\0"
   "glFinalCombinerInputNV\0"
   "\0"
   /* _mesa_function_pool[7280]: TexCoord2i (offset 106) */
   "ii\0"
   "glTexCoord2i\0"
   "\0"
   /* _mesa_function_pool[7297]: PixelMapfv (offset 251) */
   "iip\0"
   "glPixelMapfv\0"
   "\0"
   /* _mesa_function_pool[7315]: Color4ui (offset 37) */
   "iiii\0"
   "glColor4ui\0"
   "\0"
   /* _mesa_function_pool[7332]: RasterPos3s (offset 76) */
   "iii\0"
   "glRasterPos3s\0"
   "\0"
   /* _mesa_function_pool[7351]: Color3usv (offset 24) */
   "p\0"
   "glColor3usv\0"
   "\0"
   /* _mesa_function_pool[7366]: FlushRasterSGIX (dynamic) */
   "\0"
   "glFlushRasterSGIX\0"
   "\0"
   /* _mesa_function_pool[7386]: TexCoord2f (offset 104) */
   "ff\0"
   "glTexCoord2f\0"
   "\0"
   /* _mesa_function_pool[7403]: ReplacementCodeuiTexCoord2fVertex3fSUN (dynamic) */
   "ifffff\0"
   "glReplacementCodeuiTexCoord2fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[7452]: TexCoord2d (offset 102) */
   "dd\0"
   "glTexCoord2d\0"
   "\0"
   /* _mesa_function_pool[7469]: RasterPos3d (offset 70) */
   "ddd\0"
   "glRasterPos3d\0"
   "\0"
   /* _mesa_function_pool[7488]: RasterPos3f (offset 72) */
   "fff\0"
   "glRasterPos3f\0"
   "\0"
   /* _mesa_function_pool[7507]: Uniform1fARB (will be remapped) */
   "if\0"
   "glUniform1f\0"
   "glUniform1fARB\0"
   "\0"
   /* _mesa_function_pool[7538]: AreTexturesResident (offset 322) */
   "ipp\0"
   "glAreTexturesResident\0"
   "glAreTexturesResidentEXT\0"
   "\0"
   /* _mesa_function_pool[7590]: TexCoord2s (offset 108) */
   "ii\0"
   "glTexCoord2s\0"
   "\0"
   /* _mesa_function_pool[7607]: StencilOpSeparate (will be remapped) */
   "iiii\0"
   "glStencilOpSeparate\0"
   "glStencilOpSeparateATI\0"
   "\0"
   /* _mesa_function_pool[7656]: ColorTableParameteriv (offset 341) */
   "iip\0"
   "glColorTableParameteriv\0"
   "glColorTableParameterivSGI\0"
   "\0"
   /* _mesa_function_pool[7712]: FogCoordPointerListIBM (dynamic) */
   "iipi\0"
   "glFogCoordPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[7743]: WindowPos3dMESA (will be remapped) */
   "ddd\0"
   "glWindowPos3d\0"
   "glWindowPos3dARB\0"
   "glWindowPos3dMESA\0"
   "\0"
   /* _mesa_function_pool[7797]: Color4us (offset 39) */
   "iiii\0"
   "glColor4us\0"
   "\0"
   /* _mesa_function_pool[7814]: PointParameterfvEXT (will be remapped) */
   "ip\0"
   "glPointParameterfv\0"
   "glPointParameterfvARB\0"
   "glPointParameterfvEXT\0"
   "glPointParameterfvSGIS\0"
   "\0"
   /* _mesa_function_pool[7904]: Color3bv (offset 10) */
   "p\0"
   "glColor3bv\0"
   "\0"
   /* _mesa_function_pool[7918]: WindowPos2fvMESA (will be remapped) */
   "p\0"
   "glWindowPos2fv\0"
   "glWindowPos2fvARB\0"
   "glWindowPos2fvMESA\0"
   "\0"
   /* _mesa_function_pool[7973]: SecondaryColor3bvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3bv\0"
   "glSecondaryColor3bvEXT\0"
   "\0"
   /* _mesa_function_pool[8019]: VertexPointerListIBM (dynamic) */
   "iiipi\0"
   "glVertexPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[8049]: GetProgramLocalParameterfvARB (will be remapped) */
   "iip\0"
   "glGetProgramLocalParameterfvARB\0"
   "\0"
   /* _mesa_function_pool[8086]: FragmentMaterialfSGIX (dynamic) */
   "iif\0"
   "glFragmentMaterialfSGIX\0"
   "\0"
   /* _mesa_function_pool[8115]: TexCoord2fNormal3fVertex3fSUN (dynamic) */
   "ffffffff\0"
   "glTexCoord2fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[8157]: RenderbufferStorageEXT (will be remapped) */
   "iiii\0"
   "glRenderbufferStorage\0"
   "glRenderbufferStorageEXT\0"
   "\0"
   /* _mesa_function_pool[8210]: IsFenceNV (will be remapped) */
   "i\0"
   "glIsFenceNV\0"
   "\0"
   /* _mesa_function_pool[8225]: AttachObjectARB (will be remapped) */
   "ii\0"
   "glAttachObjectARB\0"
   "\0"
   /* _mesa_function_pool[8247]: GetFragmentLightivSGIX (dynamic) */
   "iip\0"
   "glGetFragmentLightivSGIX\0"
   "\0"
   /* _mesa_function_pool[8277]: UniformMatrix2fvARB (will be remapped) */
   "iiip\0"
   "glUniformMatrix2fv\0"
   "glUniformMatrix2fvARB\0"
   "\0"
   /* _mesa_function_pool[8324]: MultiTexCoord2fARB (offset 386) */
   "iff\0"
   "glMultiTexCoord2f\0"
   "glMultiTexCoord2fARB\0"
   "\0"
   /* _mesa_function_pool[8368]: ColorTable (offset 339) */
   "iiiiip\0"
   "glColorTable\0"
   "glColorTableSGI\0"
   "glColorTableEXT\0"
   "\0"
   /* _mesa_function_pool[8421]: IndexPointer (offset 314) */
   "iip\0"
   "glIndexPointer\0"
   "\0"
   /* _mesa_function_pool[8441]: Accum (offset 213) */
   "if\0"
   "glAccum\0"
   "\0"
   /* _mesa_function_pool[8453]: GetTexImage (offset 281) */
   "iiiip\0"
   "glGetTexImage\0"
   "\0"
   /* _mesa_function_pool[8474]: MapControlPointsNV (dynamic) */
   "iiiiiiiip\0"
   "glMapControlPointsNV\0"
   "\0"
   /* _mesa_function_pool[8506]: ConvolutionFilter2D (offset 349) */
   "iiiiiip\0"
   "glConvolutionFilter2D\0"
   "glConvolutionFilter2DEXT\0"
   "\0"
   /* _mesa_function_pool[8562]: Finish (offset 216) */
   "\0"
   "glFinish\0"
   "\0"
   /* _mesa_function_pool[8573]: MapParameterfvNV (dynamic) */
   "iip\0"
   "glMapParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[8597]: ClearStencil (offset 207) */
   "i\0"
   "glClearStencil\0"
   "\0"
   /* _mesa_function_pool[8615]: VertexAttrib3dvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib3dv\0"
   "glVertexAttrib3dvARB\0"
   "\0"
   /* _mesa_function_pool[8658]: Uniform4uivEXT (will be remapped) */
   "iip\0"
   "glUniform4uivEXT\0"
   "glUniform4uiv\0"
   "\0"
   /* _mesa_function_pool[8694]: HintPGI (dynamic) */
   "ii\0"
   "glHintPGI\0"
   "\0"
   /* _mesa_function_pool[8708]: ConvolutionParameteriv (offset 353) */
   "iip\0"
   "glConvolutionParameteriv\0"
   "glConvolutionParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[8766]: Color4s (offset 33) */
   "iiii\0"
   "glColor4s\0"
   "\0"
   /* _mesa_function_pool[8782]: InterleavedArrays (offset 317) */
   "iip\0"
   "glInterleavedArrays\0"
   "\0"
   /* _mesa_function_pool[8807]: RasterPos2fv (offset 65) */
   "p\0"
   "glRasterPos2fv\0"
   "\0"
   /* _mesa_function_pool[8825]: TexCoord1fv (offset 97) */
   "p\0"
   "glTexCoord1fv\0"
   "\0"
   /* _mesa_function_pool[8842]: Vertex2d (offset 126) */
   "dd\0"
   "glVertex2d\0"
   "\0"
   /* _mesa_function_pool[8857]: CullParameterdvEXT (dynamic) */
   "ip\0"
   "glCullParameterdvEXT\0"
   "\0"
   /* _mesa_function_pool[8882]: ProgramNamedParameter4fNV (will be remapped) */
   "iipffff\0"
   "glProgramNamedParameter4fNV\0"
   "\0"
   /* _mesa_function_pool[8919]: Color3fVertex3fSUN (dynamic) */
   "ffffff\0"
   "glColor3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[8948]: ProgramEnvParameter4fvARB (will be remapped) */
   "iip\0"
   "glProgramEnvParameter4fvARB\0"
   "glProgramParameter4fvNV\0"
   "\0"
   /* _mesa_function_pool[9005]: Color4i (offset 31) */
   "iiii\0"
   "glColor4i\0"
   "\0"
   /* _mesa_function_pool[9021]: Color4f (offset 29) */
   "ffff\0"
   "glColor4f\0"
   "\0"
   /* _mesa_function_pool[9037]: RasterPos4fv (offset 81) */
   "p\0"
   "glRasterPos4fv\0"
   "\0"
   /* _mesa_function_pool[9055]: Color4d (offset 27) */
   "dddd\0"
   "glColor4d\0"
   "\0"
   /* _mesa_function_pool[9071]: ClearIndex (offset 205) */
   "f\0"
   "glClearIndex\0"
   "\0"
   /* _mesa_function_pool[9087]: Color4b (offset 25) */
   "iiii\0"
   "glColor4b\0"
   "\0"
   /* _mesa_function_pool[9103]: LoadMatrixd (offset 292) */
   "p\0"
   "glLoadMatrixd\0"
   "\0"
   /* _mesa_function_pool[9120]: FragmentLightModeliSGIX (dynamic) */
   "ii\0"
   "glFragmentLightModeliSGIX\0"
   "\0"
   /* _mesa_function_pool[9150]: RasterPos2dv (offset 63) */
   "p\0"
   "glRasterPos2dv\0"
   "\0"
   /* _mesa_function_pool[9168]: ConvolutionParameterfv (offset 351) */
   "iip\0"
   "glConvolutionParameterfv\0"
   "glConvolutionParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[9226]: TbufferMask3DFX (dynamic) */
   "i\0"
   "glTbufferMask3DFX\0"
   "\0"
   /* _mesa_function_pool[9247]: GetTexGendv (offset 278) */
   "iip\0"
   "glGetTexGendv\0"
   "\0"
   /* _mesa_function_pool[9266]: GetVertexAttribfvNV (will be remapped) */
   "iip\0"
   "glGetVertexAttribfvNV\0"
   "\0"
   /* _mesa_function_pool[9293]: BeginTransformFeedbackEXT (will be remapped) */
   "i\0"
   "glBeginTransformFeedbackEXT\0"
   "glBeginTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[9349]: LoadProgramNV (will be remapped) */
   "iiip\0"
   "glLoadProgramNV\0"
   "\0"
   /* _mesa_function_pool[9371]: WaitSync (will be remapped) */
   "iii\0"
   "glWaitSync\0"
   "\0"
   /* _mesa_function_pool[9387]: EndList (offset 1) */
   "\0"
   "glEndList\0"
   "\0"
   /* _mesa_function_pool[9399]: VertexAttrib4fvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib4fvNV\0"
   "\0"
   /* _mesa_function_pool[9423]: GetAttachedObjectsARB (will be remapped) */
   "iipp\0"
   "glGetAttachedObjectsARB\0"
   "\0"
   /* _mesa_function_pool[9453]: Uniform3fvARB (will be remapped) */
   "iip\0"
   "glUniform3fv\0"
   "glUniform3fvARB\0"
   "\0"
   /* _mesa_function_pool[9487]: EvalCoord1fv (offset 231) */
   "p\0"
   "glEvalCoord1fv\0"
   "\0"
   /* _mesa_function_pool[9505]: DrawRangeElements (offset 338) */
   "iiiiip\0"
   "glDrawRangeElements\0"
   "glDrawRangeElementsEXT\0"
   "\0"
   /* _mesa_function_pool[9556]: EvalMesh2 (offset 238) */
   "iiiii\0"
   "glEvalMesh2\0"
   "\0"
   /* _mesa_function_pool[9575]: Vertex4fv (offset 145) */
   "p\0"
   "glVertex4fv\0"
   "\0"
   /* _mesa_function_pool[9590]: GenTransformFeedbacks (will be remapped) */
   "ip\0"
   "glGenTransformFeedbacks\0"
   "\0"
   /* _mesa_function_pool[9618]: SpriteParameterfvSGIX (dynamic) */
   "ip\0"
   "glSpriteParameterfvSGIX\0"
   "\0"
   /* _mesa_function_pool[9646]: CheckFramebufferStatusEXT (will be remapped) */
   "i\0"
   "glCheckFramebufferStatus\0"
   "glCheckFramebufferStatusEXT\0"
   "\0"
   /* _mesa_function_pool[9702]: GlobalAlphaFactoruiSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactoruiSUN\0"
   "\0"
   /* _mesa_function_pool[9730]: GetHandleARB (will be remapped) */
   "i\0"
   "glGetHandleARB\0"
   "\0"
   /* _mesa_function_pool[9748]: GetVertexAttribivARB (will be remapped) */
   "iip\0"
   "glGetVertexAttribiv\0"
   "glGetVertexAttribivARB\0"
   "\0"
   /* _mesa_function_pool[9796]: BlendFunciARB (will be remapped) */
   "iii\0"
   "glBlendFunciARB\0"
   "\0"
   /* _mesa_function_pool[9817]: GetCombinerInputParameterfvNV (will be remapped) */
   "iiiip\0"
   "glGetCombinerInputParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[9856]: GetTexParameterIivEXT (will be remapped) */
   "iip\0"
   "glGetTexParameterIivEXT\0"
   "glGetTexParameterIiv\0"
   "\0"
   /* _mesa_function_pool[9906]: CreateProgram (will be remapped) */
   "\0"
   "glCreateProgram\0"
   "\0"
   /* _mesa_function_pool[9924]: LoadTransposeMatrixdARB (will be remapped) */
   "p\0"
   "glLoadTransposeMatrixd\0"
   "glLoadTransposeMatrixdARB\0"
   "\0"
   /* _mesa_function_pool[9976]: ReleaseShaderCompiler (will be remapped) */
   "\0"
   "glReleaseShaderCompiler\0"
   "\0"
   /* _mesa_function_pool[10002]: GetMinmax (offset 364) */
   "iiiip\0"
   "glGetMinmax\0"
   "glGetMinmaxEXT\0"
   "\0"
   /* _mesa_function_pool[10036]: StencilFuncSeparate (will be remapped) */
   "iiii\0"
   "glStencilFuncSeparate\0"
   "\0"
   /* _mesa_function_pool[10064]: SecondaryColor3sEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3s\0"
   "glSecondaryColor3sEXT\0"
   "\0"
   /* _mesa_function_pool[10110]: Color3fVertex3fvSUN (dynamic) */
   "pp\0"
   "glColor3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[10136]: GetInteger64i_v (will be remapped) */
   "iip\0"
   "glGetInteger64i_v\0"
   "\0"
   /* _mesa_function_pool[10159]: Normal3fv (offset 57) */
   "p\0"
   "glNormal3fv\0"
   "\0"
   /* _mesa_function_pool[10174]: GlobalAlphaFactorbSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactorbSUN\0"
   "\0"
   /* _mesa_function_pool[10201]: Color3us (offset 23) */
   "iii\0"
   "glColor3us\0"
   "\0"
   /* _mesa_function_pool[10217]: ImageTransformParameterfvHP (dynamic) */
   "iip\0"
   "glImageTransformParameterfvHP\0"
   "\0"
   /* _mesa_function_pool[10252]: VertexAttrib4ivARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4iv\0"
   "glVertexAttrib4ivARB\0"
   "\0"
   /* _mesa_function_pool[10295]: End (offset 43) */
   "\0"
   "glEnd\0"
   "\0"
   /* _mesa_function_pool[10303]: VertexAttrib3fNV (will be remapped) */
   "ifff\0"
   "glVertexAttrib3fNV\0"
   "\0"
   /* _mesa_function_pool[10328]: VertexAttribs2dvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs2dvNV\0"
   "\0"
   /* _mesa_function_pool[10354]: GetQueryObjectui64vEXT (will be remapped) */
   "iip\0"
   "glGetQueryObjectui64vEXT\0"
   "\0"
   /* _mesa_function_pool[10384]: MultiTexCoord3fvARB (offset 395) */
   "ip\0"
   "glMultiTexCoord3fv\0"
   "glMultiTexCoord3fvARB\0"
   "\0"
   /* _mesa_function_pool[10429]: SecondaryColor3dEXT (will be remapped) */
   "ddd\0"
   "glSecondaryColor3d\0"
   "glSecondaryColor3dEXT\0"
   "\0"
   /* _mesa_function_pool[10475]: Color3ub (offset 19) */
   "iii\0"
   "glColor3ub\0"
   "\0"
   /* _mesa_function_pool[10491]: GetProgramParameterfvNV (will be remapped) */
   "iiip\0"
   "glGetProgramParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[10523]: TangentPointerEXT (dynamic) */
   "iip\0"
   "glTangentPointerEXT\0"
   "\0"
   /* _mesa_function_pool[10548]: Color4fNormal3fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glColor4fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[10583]: GetInstrumentsSGIX (dynamic) */
   "\0"
   "glGetInstrumentsSGIX\0"
   "\0"
   /* _mesa_function_pool[10606]: GetUniformuivEXT (will be remapped) */
   "iip\0"
   "glGetUniformuivEXT\0"
   "glGetUniformuiv\0"
   "\0"
   /* _mesa_function_pool[10646]: Color3ui (offset 21) */
   "iii\0"
   "glColor3ui\0"
   "\0"
   /* _mesa_function_pool[10662]: EvalMapsNV (dynamic) */
   "ii\0"
   "glEvalMapsNV\0"
   "\0"
   /* _mesa_function_pool[10679]: TexSubImage2D (offset 333) */
   "iiiiiiiip\0"
   "glTexSubImage2D\0"
   "glTexSubImage2DEXT\0"
   "\0"
   /* _mesa_function_pool[10725]: FragmentLightivSGIX (dynamic) */
   "iip\0"
   "glFragmentLightivSGIX\0"
   "\0"
   /* _mesa_function_pool[10752]: GetTexParameterPointervAPPLE (will be remapped) */
   "iip\0"
   "glGetTexParameterPointervAPPLE\0"
   "\0"
   /* _mesa_function_pool[10788]: TexGenfv (offset 191) */
   "iip\0"
   "glTexGenfv\0"
   "\0"
   /* _mesa_function_pool[10804]: GetTransformFeedbackVaryingEXT (will be remapped) */
   "iiipppp\0"
   "glGetTransformFeedbackVaryingEXT\0"
   "glGetTransformFeedbackVarying\0"
   "\0"
   /* _mesa_function_pool[10876]: VertexAttrib4bvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4bv\0"
   "glVertexAttrib4bvARB\0"
   "\0"
   /* _mesa_function_pool[10919]: ShaderBinary (will be remapped) */
   "ipipi\0"
   "glShaderBinary\0"
   "\0"
   /* _mesa_function_pool[10941]: GetIntegerIndexedvEXT (will be remapped) */
   "iip\0"
   "glGetIntegerIndexedvEXT\0"
   "glGetIntegeri_v\0"
   "\0"
   /* _mesa_function_pool[10986]: MultiTexCoord4sARB (offset 406) */
   "iiiii\0"
   "glMultiTexCoord4s\0"
   "glMultiTexCoord4sARB\0"
   "\0"
   /* _mesa_function_pool[11032]: GetFragmentMaterialivSGIX (dynamic) */
   "iip\0"
   "glGetFragmentMaterialivSGIX\0"
   "\0"
   /* _mesa_function_pool[11065]: WindowPos4dMESA (will be remapped) */
   "dddd\0"
   "glWindowPos4dMESA\0"
   "\0"
   /* _mesa_function_pool[11089]: WeightPointerARB (dynamic) */
   "iiip\0"
   "glWeightPointerARB\0"
   "\0"
   /* _mesa_function_pool[11114]: WindowPos2dMESA (will be remapped) */
   "dd\0"
   "glWindowPos2d\0"
   "glWindowPos2dARB\0"
   "glWindowPos2dMESA\0"
   "\0"
   /* _mesa_function_pool[11167]: FramebufferTexture3DEXT (will be remapped) */
   "iiiiii\0"
   "glFramebufferTexture3D\0"
   "glFramebufferTexture3DEXT\0"
   "\0"
   /* _mesa_function_pool[11224]: BlendEquation (offset 337) */
   "i\0"
   "glBlendEquation\0"
   "glBlendEquationEXT\0"
   "\0"
   /* _mesa_function_pool[11262]: VertexAttrib3dNV (will be remapped) */
   "iddd\0"
   "glVertexAttrib3dNV\0"
   "\0"
   /* _mesa_function_pool[11287]: VertexAttrib3dARB (will be remapped) */
   "iddd\0"
   "glVertexAttrib3d\0"
   "glVertexAttrib3dARB\0"
   "\0"
   /* _mesa_function_pool[11330]: VertexAttribI4usvEXT (will be remapped) */
   "ip\0"
   "glVertexAttribI4usvEXT\0"
   "glVertexAttribI4usv\0"
   "\0"
   /* _mesa_function_pool[11377]: ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN (dynamic) */
   "ppppp\0"
   "glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[11441]: VertexAttrib4fARB (will be remapped) */
   "iffff\0"
   "glVertexAttrib4f\0"
   "glVertexAttrib4fARB\0"
   "\0"
   /* _mesa_function_pool[11485]: GetError (offset 261) */
   "\0"
   "glGetError\0"
   "\0"
   /* _mesa_function_pool[11498]: IndexFuncEXT (dynamic) */
   "if\0"
   "glIndexFuncEXT\0"
   "\0"
   /* _mesa_function_pool[11517]: TexCoord3dv (offset 111) */
   "p\0"
   "glTexCoord3dv\0"
   "\0"
   /* _mesa_function_pool[11534]: Indexdv (offset 45) */
   "p\0"
   "glIndexdv\0"
   "\0"
   /* _mesa_function_pool[11547]: FramebufferTexture2DEXT (will be remapped) */
   "iiiii\0"
   "glFramebufferTexture2D\0"
   "glFramebufferTexture2DEXT\0"
   "\0"
   /* _mesa_function_pool[11603]: Normal3s (offset 60) */
   "iii\0"
   "glNormal3s\0"
   "\0"
   /* _mesa_function_pool[11619]: GetObjectParameterivAPPLE (will be remapped) */
   "iiip\0"
   "glGetObjectParameterivAPPLE\0"
   "\0"
   /* _mesa_function_pool[11653]: PushName (offset 201) */
   "i\0"
   "glPushName\0"
   "\0"
   /* _mesa_function_pool[11667]: MultiTexCoord2dvARB (offset 385) */
   "ip\0"
   "glMultiTexCoord2dv\0"
   "glMultiTexCoord2dvARB\0"
   "\0"
   /* _mesa_function_pool[11712]: CullParameterfvEXT (dynamic) */
   "ip\0"
   "glCullParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[11737]: Normal3i (offset 58) */
   "iii\0"
   "glNormal3i\0"
   "\0"
   /* _mesa_function_pool[11753]: ProgramNamedParameter4fvNV (will be remapped) */
   "iipp\0"
   "glProgramNamedParameter4fvNV\0"
   "\0"
   /* _mesa_function_pool[11788]: SecondaryColorPointerEXT (will be remapped) */
   "iiip\0"
   "glSecondaryColorPointer\0"
   "glSecondaryColorPointerEXT\0"
   "\0"
   /* _mesa_function_pool[11845]: VertexAttrib4fvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4fv\0"
   "glVertexAttrib4fvARB\0"
   "\0"
   /* _mesa_function_pool[11888]: ColorPointerListIBM (dynamic) */
   "iiipi\0"
   "glColorPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[11917]: GetActiveUniformARB (will be remapped) */
   "iiipppp\0"
   "glGetActiveUniform\0"
   "glGetActiveUniformARB\0"
   "\0"
   /* _mesa_function_pool[11967]: ImageTransformParameteriHP (dynamic) */
   "iii\0"
   "glImageTransformParameteriHP\0"
   "\0"
   /* _mesa_function_pool[12001]: Normal3b (offset 52) */
   "iii\0"
   "glNormal3b\0"
   "\0"
   /* _mesa_function_pool[12017]: Normal3d (offset 54) */
   "ddd\0"
   "glNormal3d\0"
   "\0"
   /* _mesa_function_pool[12033]: Uniform1uiEXT (will be remapped) */
   "ii\0"
   "glUniform1uiEXT\0"
   "glUniform1ui\0"
   "\0"
   /* _mesa_function_pool[12066]: Normal3f (offset 56) */
   "fff\0"
   "glNormal3f\0"
   "\0"
   /* _mesa_function_pool[12082]: MultiTexCoord1svARB (offset 383) */
   "ip\0"
   "glMultiTexCoord1sv\0"
   "glMultiTexCoord1svARB\0"
   "\0"
   /* _mesa_function_pool[12127]: Indexi (offset 48) */
   "i\0"
   "glIndexi\0"
   "\0"
   /* _mesa_function_pool[12139]: EGLImageTargetTexture2DOES (will be remapped) */
   "ip\0"
   "glEGLImageTargetTexture2DOES\0"
   "\0"
   /* _mesa_function_pool[12172]: EndQueryARB (will be remapped) */
   "i\0"
   "glEndQuery\0"
   "glEndQueryARB\0"
   "\0"
   /* _mesa_function_pool[12200]: DeleteFencesNV (will be remapped) */
   "ip\0"
   "glDeleteFencesNV\0"
   "\0"
   /* _mesa_function_pool[12221]: BindBufferRangeEXT (will be remapped) */
   "iiiii\0"
   "glBindBufferRangeEXT\0"
   "glBindBufferRange\0"
   "\0"
   /* _mesa_function_pool[12267]: DepthMask (offset 211) */
   "i\0"
   "glDepthMask\0"
   "\0"
   /* _mesa_function_pool[12282]: IsShader (will be remapped) */
   "i\0"
   "glIsShader\0"
   "\0"
   /* _mesa_function_pool[12296]: Indexf (offset 46) */
   "f\0"
   "glIndexf\0"
   "\0"
   /* _mesa_function_pool[12308]: GetImageTransformParameterivHP (dynamic) */
   "iip\0"
   "glGetImageTransformParameterivHP\0"
   "\0"
   /* _mesa_function_pool[12346]: Indexd (offset 44) */
   "d\0"
   "glIndexd\0"
   "\0"
   /* _mesa_function_pool[12358]: GetMaterialiv (offset 270) */
   "iip\0"
   "glGetMaterialiv\0"
   "\0"
   /* _mesa_function_pool[12379]: StencilOp (offset 244) */
   "iii\0"
   "glStencilOp\0"
   "\0"
   /* _mesa_function_pool[12396]: WindowPos4ivMESA (will be remapped) */
   "p\0"
   "glWindowPos4ivMESA\0"
   "\0"
   /* _mesa_function_pool[12418]: FramebufferTextureLayer (dynamic) */
   "iiiii\0"
   "glFramebufferTextureLayerARB\0"
   "\0"
   /* _mesa_function_pool[12454]: MultiTexCoord3svARB (offset 399) */
   "ip\0"
   "glMultiTexCoord3sv\0"
   "glMultiTexCoord3svARB\0"
   "\0"
   /* _mesa_function_pool[12499]: TexEnvfv (offset 185) */
   "iip\0"
   "glTexEnvfv\0"
   "\0"
   /* _mesa_function_pool[12515]: MultiTexCoord4iARB (offset 404) */
   "iiiii\0"
   "glMultiTexCoord4i\0"
   "glMultiTexCoord4iARB\0"
   "\0"
   /* _mesa_function_pool[12561]: Indexs (offset 50) */
   "i\0"
   "glIndexs\0"
   "\0"
   /* _mesa_function_pool[12573]: Binormal3ivEXT (dynamic) */
   "p\0"
   "glBinormal3ivEXT\0"
   "\0"
   /* _mesa_function_pool[12593]: ResizeBuffersMESA (will be remapped) */
   "\0"
   "glResizeBuffersMESA\0"
   "\0"
   /* _mesa_function_pool[12615]: BlendFuncSeparateiARB (will be remapped) */
   "iiiii\0"
   "glBlendFuncSeparateiARB\0"
   "\0"
   /* _mesa_function_pool[12646]: GetUniformivARB (will be remapped) */
   "iip\0"
   "glGetUniformiv\0"
   "glGetUniformivARB\0"
   "\0"
   /* _mesa_function_pool[12684]: PixelTexGenParameteriSGIS (will be remapped) */
   "ii\0"
   "glPixelTexGenParameteriSGIS\0"
   "\0"
   /* _mesa_function_pool[12716]: VertexPointervINTEL (dynamic) */
   "iip\0"
   "glVertexPointervINTEL\0"
   "\0"
   /* _mesa_function_pool[12743]: Vertex2i (offset 130) */
   "ii\0"
   "glVertex2i\0"
   "\0"
   /* _mesa_function_pool[12758]: LoadMatrixf (offset 291) */
   "p\0"
   "glLoadMatrixf\0"
   "\0"
   /* _mesa_function_pool[12775]: VertexAttribI1uivEXT (will be remapped) */
   "ip\0"
   "glVertexAttribI1uivEXT\0"
   "glVertexAttribI1uiv\0"
   "\0"
   /* _mesa_function_pool[12822]: Vertex2f (offset 128) */
   "ff\0"
   "glVertex2f\0"
   "\0"
   /* _mesa_function_pool[12837]: ReplacementCodeuiColor4fNormal3fVertex3fvSUN (dynamic) */
   "pppp\0"
   "glReplacementCodeuiColor4fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[12890]: Color4bv (offset 26) */
   "p\0"
   "glColor4bv\0"
   "\0"
   /* _mesa_function_pool[12904]: VertexPointer (offset 321) */
   "iiip\0"
   "glVertexPointer\0"
   "\0"
   /* _mesa_function_pool[12926]: SecondaryColor3uiEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3ui\0"
   "glSecondaryColor3uiEXT\0"
   "\0"
   /* _mesa_function_pool[12974]: StartInstrumentsSGIX (dynamic) */
   "\0"
   "glStartInstrumentsSGIX\0"
   "\0"
   /* _mesa_function_pool[12999]: SecondaryColor3usvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3usv\0"
   "glSecondaryColor3usvEXT\0"
   "\0"
   /* _mesa_function_pool[13047]: VertexAttrib2fvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib2fvNV\0"
   "\0"
   /* _mesa_function_pool[13071]: ProgramLocalParameter4dvARB (will be remapped) */
   "iip\0"
   "glProgramLocalParameter4dvARB\0"
   "\0"
   /* _mesa_function_pool[13106]: DeleteLists (offset 4) */
   "ii\0"
   "glDeleteLists\0"
   "\0"
   /* _mesa_function_pool[13124]: LogicOp (offset 242) */
   "i\0"
   "glLogicOp\0"
   "\0"
   /* _mesa_function_pool[13137]: MatrixIndexuivARB (dynamic) */
   "ip\0"
   "glMatrixIndexuivARB\0"
   "\0"
   /* _mesa_function_pool[13161]: Vertex2s (offset 132) */
   "ii\0"
   "glVertex2s\0"
   "\0"
   /* _mesa_function_pool[13176]: RenderbufferStorageMultisample (will be remapped) */
   "iiiii\0"
   "glRenderbufferStorageMultisample\0"
   "glRenderbufferStorageMultisampleEXT\0"
   "\0"
   /* _mesa_function_pool[13252]: TexCoord4fv (offset 121) */
   "p\0"
   "glTexCoord4fv\0"
   "\0"
   /* _mesa_function_pool[13269]: Tangent3sEXT (dynamic) */
   "iii\0"
   "glTangent3sEXT\0"
   "\0"
   /* _mesa_function_pool[13289]: GlobalAlphaFactorfSUN (dynamic) */
   "f\0"
   "glGlobalAlphaFactorfSUN\0"
   "\0"
   /* _mesa_function_pool[13316]: MultiTexCoord3iARB (offset 396) */
   "iiii\0"
   "glMultiTexCoord3i\0"
   "glMultiTexCoord3iARB\0"
   "\0"
   /* _mesa_function_pool[13361]: IsProgram (will be remapped) */
   "i\0"
   "glIsProgram\0"
   "\0"
   /* _mesa_function_pool[13376]: TexCoordPointerListIBM (dynamic) */
   "iiipi\0"
   "glTexCoordPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[13408]: VertexAttribI4svEXT (will be remapped) */
   "ip\0"
   "glVertexAttribI4svEXT\0"
   "glVertexAttribI4sv\0"
   "\0"
   /* _mesa_function_pool[13453]: GlobalAlphaFactorusSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactorusSUN\0"
   "\0"
   /* _mesa_function_pool[13481]: VertexAttrib2dvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib2dvNV\0"
   "\0"
   /* _mesa_function_pool[13505]: FramebufferRenderbufferEXT (will be remapped) */
   "iiii\0"
   "glFramebufferRenderbuffer\0"
   "glFramebufferRenderbufferEXT\0"
   "\0"
   /* _mesa_function_pool[13566]: ClearBufferuiv (will be remapped) */
   "iip\0"
   "glClearBufferuiv\0"
   "\0"
   /* _mesa_function_pool[13588]: VertexAttrib1dvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib1dvNV\0"
   "\0"
   /* _mesa_function_pool[13612]: GenTextures (offset 328) */
   "ip\0"
   "glGenTextures\0"
   "glGenTexturesEXT\0"
   "\0"
   /* _mesa_function_pool[13647]: FramebufferTextureARB (will be remapped) */
   "iiii\0"
   "glFramebufferTextureARB\0"
   "\0"
   /* _mesa_function_pool[13677]: SetFenceNV (will be remapped) */
   "ii\0"
   "glSetFenceNV\0"
   "\0"
   /* _mesa_function_pool[13694]: FramebufferTexture1DEXT (will be remapped) */
   "iiiii\0"
   "glFramebufferTexture1D\0"
   "glFramebufferTexture1DEXT\0"
   "\0"
   /* _mesa_function_pool[13750]: GetCombinerOutputParameterivNV (will be remapped) */
   "iiip\0"
   "glGetCombinerOutputParameterivNV\0"
   "\0"
   /* _mesa_function_pool[13789]: MultiModeDrawArraysIBM (will be remapped) */
   "pppii\0"
   "glMultiModeDrawArraysIBM\0"
   "\0"
   /* _mesa_function_pool[13821]: PixelTexGenParameterivSGIS (will be remapped) */
   "ip\0"
   "glPixelTexGenParameterivSGIS\0"
   "\0"
   /* _mesa_function_pool[13854]: TextureNormalEXT (dynamic) */
   "i\0"
   "glTextureNormalEXT\0"
   "\0"
   /* _mesa_function_pool[13876]: IndexPointerListIBM (dynamic) */
   "iipi\0"
   "glIndexPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[13904]: WeightfvARB (dynamic) */
   "ip\0"
   "glWeightfvARB\0"
   "\0"
   /* _mesa_function_pool[13922]: GetCombinerOutputParameterfvNV (will be remapped) */
   "iiip\0"
   "glGetCombinerOutputParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[13961]: RasterPos2sv (offset 69) */
   "p\0"
   "glRasterPos2sv\0"
   "\0"
   /* _mesa_function_pool[13979]: Color4ubv (offset 36) */
   "p\0"
   "glColor4ubv\0"
   "\0"
   /* _mesa_function_pool[13994]: DrawBuffer (offset 202) */
   "i\0"
   "glDrawBuffer\0"
   "\0"
   /* _mesa_function_pool[14010]: TexCoord2fv (offset 105) */
   "p\0"
   "glTexCoord2fv\0"
   "\0"
   /* _mesa_function_pool[14027]: WindowPos4fMESA (will be remapped) */
   "ffff\0"
   "glWindowPos4fMESA\0"
   "\0"
   /* _mesa_function_pool[14051]: TexCoord1sv (offset 101) */
   "p\0"
   "glTexCoord1sv\0"
   "\0"
   /* _mesa_function_pool[14068]: WindowPos3dvMESA (will be remapped) */
   "p\0"
   "glWindowPos3dv\0"
   "glWindowPos3dvARB\0"
   "glWindowPos3dvMESA\0"
   "\0"
   /* _mesa_function_pool[14123]: DepthFunc (offset 245) */
   "i\0"
   "glDepthFunc\0"
   "\0"
   /* _mesa_function_pool[14138]: PixelMapusv (offset 253) */
   "iip\0"
   "glPixelMapusv\0"
   "\0"
   /* _mesa_function_pool[14157]: GetQueryObjecti64vEXT (will be remapped) */
   "iip\0"
   "glGetQueryObjecti64vEXT\0"
   "\0"
   /* _mesa_function_pool[14186]: MultiTexCoord1dARB (offset 376) */
   "id\0"
   "glMultiTexCoord1d\0"
   "glMultiTexCoord1dARB\0"
   "\0"
   /* _mesa_function_pool[14229]: PointParameterivNV (will be remapped) */
   "ip\0"
   "glPointParameteriv\0"
   "glPointParameterivNV\0"
   "\0"
   /* _mesa_function_pool[14273]: BlendFunc (offset 241) */
   "ii\0"
   "glBlendFunc\0"
   "\0"
   /* _mesa_function_pool[14289]: EndTransformFeedbackEXT (will be remapped) */
   "\0"
   "glEndTransformFeedbackEXT\0"
   "glEndTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[14340]: Uniform2fvARB (will be remapped) */
   "iip\0"
   "glUniform2fv\0"
   "glUniform2fvARB\0"
   "\0"
   /* _mesa_function_pool[14374]: BufferParameteriAPPLE (will be remapped) */
   "iii\0"
   "glBufferParameteriAPPLE\0"
   "\0"
   /* _mesa_function_pool[14403]: MultiTexCoord3dvARB (offset 393) */
   "ip\0"
   "glMultiTexCoord3dv\0"
   "glMultiTexCoord3dvARB\0"
   "\0"
   /* _mesa_function_pool[14448]: ReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN (dynamic) */
   "pppp\0"
   "glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[14504]: DeleteObjectARB (will be remapped) */
   "i\0"
   "glDeleteObjectARB\0"
   "\0"
   /* _mesa_function_pool[14525]: GetShaderPrecisionFormat (will be remapped) */
   "iipp\0"
   "glGetShaderPrecisionFormat\0"
   "\0"
   /* _mesa_function_pool[14558]: MatrixIndexPointerARB (dynamic) */
   "iiip\0"
   "glMatrixIndexPointerARB\0"
   "\0"
   /* _mesa_function_pool[14588]: ProgramNamedParameter4dvNV (will be remapped) */
   "iipp\0"
   "glProgramNamedParameter4dvNV\0"
   "\0"
   /* _mesa_function_pool[14623]: Tangent3fvEXT (dynamic) */
   "p\0"
   "glTangent3fvEXT\0"
   "\0"
   /* _mesa_function_pool[14642]: Flush (offset 217) */
   "\0"
   "glFlush\0"
   "\0"
   /* _mesa_function_pool[14652]: Color4uiv (offset 38) */
   "p\0"
   "glColor4uiv\0"
   "\0"
   /* _mesa_function_pool[14667]: VertexAttribI4iEXT (will be remapped) */
   "iiiii\0"
   "glVertexAttribI4iEXT\0"
   "glVertexAttribI4i\0"
   "\0"
   /* _mesa_function_pool[14713]: GenVertexArrays (will be remapped) */
   "ip\0"
   "glGenVertexArrays\0"
   "\0"
   /* _mesa_function_pool[14735]: Uniform3uivEXT (will be remapped) */
   "iip\0"
   "glUniform3uivEXT\0"
   "glUniform3uiv\0"
   "\0"
   /* _mesa_function_pool[14771]: RasterPos3sv (offset 77) */
   "p\0"
   "glRasterPos3sv\0"
   "\0"
   /* _mesa_function_pool[14789]: BindFramebufferEXT (will be remapped) */
   "ii\0"
   "glBindFramebuffer\0"
   "glBindFramebufferEXT\0"
   "\0"
   /* _mesa_function_pool[14832]: ReferencePlaneSGIX (dynamic) */
   "p\0"
   "glReferencePlaneSGIX\0"
   "\0"
   /* _mesa_function_pool[14856]: PushAttrib (offset 219) */
   "i\0"
   "glPushAttrib\0"
   "\0"
   /* _mesa_function_pool[14872]: RasterPos2i (offset 66) */
   "ii\0"
   "glRasterPos2i\0"
   "\0"
   /* _mesa_function_pool[14890]: ValidateProgramARB (will be remapped) */
   "i\0"
   "glValidateProgram\0"
   "glValidateProgramARB\0"
   "\0"
   /* _mesa_function_pool[14932]: TexParameteriv (offset 181) */
   "iip\0"
   "glTexParameteriv\0"
   "\0"
   /* _mesa_function_pool[14954]: UnlockArraysEXT (will be remapped) */
   "\0"
   "glUnlockArraysEXT\0"
   "\0"
   /* _mesa_function_pool[14974]: TexCoord2fColor3fVertex3fSUN (dynamic) */
   "ffffffff\0"
   "glTexCoord2fColor3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[15015]: WindowPos3fvMESA (will be remapped) */
   "p\0"
   "glWindowPos3fv\0"
   "glWindowPos3fvARB\0"
   "glWindowPos3fvMESA\0"
   "\0"
   /* _mesa_function_pool[15070]: RasterPos2f (offset 64) */
   "ff\0"
   "glRasterPos2f\0"
   "\0"
   /* _mesa_function_pool[15088]: VertexAttrib1svNV (will be remapped) */
   "ip\0"
   "glVertexAttrib1svNV\0"
   "\0"
   /* _mesa_function_pool[15112]: RasterPos2d (offset 62) */
   "dd\0"
   "glRasterPos2d\0"
   "\0"
   /* _mesa_function_pool[15130]: RasterPos3fv (offset 73) */
   "p\0"
   "glRasterPos3fv\0"
   "\0"
   /* _mesa_function_pool[15148]: CopyTexSubImage3D (offset 373) */
   "iiiiiiiii\0"
   "glCopyTexSubImage3D\0"
   "glCopyTexSubImage3DEXT\0"
   "\0"
   /* _mesa_function_pool[15202]: VertexAttrib2dARB (will be remapped) */
   "idd\0"
   "glVertexAttrib2d\0"
   "glVertexAttrib2dARB\0"
   "\0"
   /* _mesa_function_pool[15244]: Color4ub (offset 35) */
   "iiii\0"
   "glColor4ub\0"
   "\0"
   /* _mesa_function_pool[15261]: GetInteger64v (will be remapped) */
   "ip\0"
   "glGetInteger64v\0"
   "\0"
   /* _mesa_function_pool[15281]: TextureColorMaskSGIS (dynamic) */
   "iiii\0"
   "glTextureColorMaskSGIS\0"
   "\0"
   /* _mesa_function_pool[15310]: RasterPos2s (offset 68) */
   "ii\0"
   "glRasterPos2s\0"
   "\0"
   /* _mesa_function_pool[15328]: GetColorTable (offset 343) */
   "iiip\0"
   "glGetColorTable\0"
   "glGetColorTableSGI\0"
   "glGetColorTableEXT\0"
   "\0"
   /* _mesa_function_pool[15388]: SelectBuffer (offset 195) */
   "ip\0"
   "glSelectBuffer\0"
   "\0"
   /* _mesa_function_pool[15407]: Indexiv (offset 49) */
   "p\0"
   "glIndexiv\0"
   "\0"
   /* _mesa_function_pool[15420]: TexCoord3i (offset 114) */
   "iii\0"
   "glTexCoord3i\0"
   "\0"
   /* _mesa_function_pool[15438]: CopyColorTable (offset 342) */
   "iiiii\0"
   "glCopyColorTable\0"
   "glCopyColorTableSGI\0"
   "\0"
   /* _mesa_function_pool[15482]: GetHistogramParameterfv (offset 362) */
   "iip\0"
   "glGetHistogramParameterfv\0"
   "glGetHistogramParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[15542]: Frustum (offset 289) */
   "dddddd\0"
   "glFrustum\0"
   "\0"
   /* _mesa_function_pool[15560]: GetString (offset 275) */
   "i\0"
   "glGetString\0"
   "\0"
   /* _mesa_function_pool[15575]: ColorPointervINTEL (dynamic) */
   "iip\0"
   "glColorPointervINTEL\0"
   "\0"
   /* _mesa_function_pool[15601]: TexEnvf (offset 184) */
   "iif\0"
   "glTexEnvf\0"
   "\0"
   /* _mesa_function_pool[15616]: TexCoord3d (offset 110) */
   "ddd\0"
   "glTexCoord3d\0"
   "\0"
   /* _mesa_function_pool[15634]: AlphaFragmentOp1ATI (will be remapped) */
   "iiiiii\0"
   "glAlphaFragmentOp1ATI\0"
   "\0"
   /* _mesa_function_pool[15664]: TexCoord3f (offset 112) */
   "fff\0"
   "glTexCoord3f\0"
   "\0"
   /* _mesa_function_pool[15682]: MultiTexCoord3ivARB (offset 397) */
   "ip\0"
   "glMultiTexCoord3iv\0"
   "glMultiTexCoord3ivARB\0"
   "\0"
   /* _mesa_function_pool[15727]: MultiTexCoord2sARB (offset 390) */
   "iii\0"
   "glMultiTexCoord2s\0"
   "glMultiTexCoord2sARB\0"
   "\0"
   /* _mesa_function_pool[15771]: VertexAttrib1dvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib1dv\0"
   "glVertexAttrib1dvARB\0"
   "\0"
   /* _mesa_function_pool[15814]: DeleteTextures (offset 327) */
   "ip\0"
   "glDeleteTextures\0"
   "glDeleteTexturesEXT\0"
   "\0"
   /* _mesa_function_pool[15855]: TexCoordPointerEXT (will be remapped) */
   "iiiip\0"
   "glTexCoordPointerEXT\0"
   "\0"
   /* _mesa_function_pool[15883]: TexSubImage4DSGIS (dynamic) */
   "iiiiiiiiiiiip\0"
   "glTexSubImage4DSGIS\0"
   "\0"
   /* _mesa_function_pool[15918]: TexCoord3s (offset 116) */
   "iii\0"
   "glTexCoord3s\0"
   "\0"
   /* _mesa_function_pool[15936]: GetTexLevelParameteriv (offset 285) */
   "iiip\0"
   "glGetTexLevelParameteriv\0"
   "\0"
   /* _mesa_function_pool[15967]: CombinerStageParameterfvNV (dynamic) */
   "iip\0"
   "glCombinerStageParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[16001]: StopInstrumentsSGIX (dynamic) */
   "i\0"
   "glStopInstrumentsSGIX\0"
   "\0"
   /* _mesa_function_pool[16026]: TexCoord4fColor4fNormal3fVertex4fSUN (dynamic) */
   "fffffffffffffff\0"
   "glTexCoord4fColor4fNormal3fVertex4fSUN\0"
   "\0"
   /* _mesa_function_pool[16082]: ClearAccum (offset 204) */
   "ffff\0"
   "glClearAccum\0"
   "\0"
   /* _mesa_function_pool[16101]: DeformSGIX (dynamic) */
   "i\0"
   "glDeformSGIX\0"
   "\0"
   /* _mesa_function_pool[16117]: GetVertexAttribfvARB (will be remapped) */
   "iip\0"
   "glGetVertexAttribfv\0"
   "glGetVertexAttribfvARB\0"
   "\0"
   /* _mesa_function_pool[16165]: SecondaryColor3ivEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3iv\0"
   "glSecondaryColor3ivEXT\0"
   "\0"
   /* _mesa_function_pool[16211]: TexCoord4iv (offset 123) */
   "p\0"
   "glTexCoord4iv\0"
   "\0"
   /* _mesa_function_pool[16228]: VertexAttribI4uiEXT (will be remapped) */
   "iiiii\0"
   "glVertexAttribI4uiEXT\0"
   "glVertexAttribI4ui\0"
   "\0"
   /* _mesa_function_pool[16276]: GetFragmentMaterialfvSGIX (dynamic) */
   "iip\0"
   "glGetFragmentMaterialfvSGIX\0"
   "\0"
   /* _mesa_function_pool[16309]: UniformMatrix4x2fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix4x2fv\0"
   "\0"
   /* _mesa_function_pool[16336]: GetDetailTexFuncSGIS (dynamic) */
   "ip\0"
   "glGetDetailTexFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[16363]: GetCombinerStageParameterfvNV (dynamic) */
   "iip\0"
   "glGetCombinerStageParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[16400]: PolygonOffset (offset 319) */
   "ff\0"
   "glPolygonOffset\0"
   "\0"
   /* _mesa_function_pool[16420]: BindVertexArray (will be remapped) */
   "i\0"
   "glBindVertexArray\0"
   "\0"
   /* _mesa_function_pool[16441]: Color4ubVertex2fvSUN (dynamic) */
   "pp\0"
   "glColor4ubVertex2fvSUN\0"
   "\0"
   /* _mesa_function_pool[16468]: Rectd (offset 86) */
   "dddd\0"
   "glRectd\0"
   "\0"
   /* _mesa_function_pool[16482]: TexFilterFuncSGIS (dynamic) */
   "iiip\0"
   "glTexFilterFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[16508]: TextureBarrierNV (will be remapped) */
   "\0"
   "glTextureBarrierNV\0"
   "\0"
   /* _mesa_function_pool[16529]: VertexAttribI4ubvEXT (will be remapped) */
   "ip\0"
   "glVertexAttribI4ubvEXT\0"
   "glVertexAttribI4ubv\0"
   "\0"
   /* _mesa_function_pool[16576]: GetAttribLocationARB (will be remapped) */
   "ip\0"
   "glGetAttribLocation\0"
   "glGetAttribLocationARB\0"
   "\0"
   /* _mesa_function_pool[16623]: RasterPos3i (offset 74) */
   "iii\0"
   "glRasterPos3i\0"
   "\0"
   /* _mesa_function_pool[16642]: BlendEquationSeparateiARB (will be remapped) */
   "iii\0"
   "glBlendEquationSeparateiARB\0"
   "\0"
   /* _mesa_function_pool[16675]: VertexAttrib4ubvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4ubv\0"
   "glVertexAttrib4ubvARB\0"
   "\0"
   /* _mesa_function_pool[16720]: DetailTexFuncSGIS (dynamic) */
   "iip\0"
   "glDetailTexFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[16745]: Normal3fVertex3fSUN (dynamic) */
   "ffffff\0"
   "glNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[16775]: CopyTexImage2D (offset 324) */
   "iiiiiiii\0"
   "glCopyTexImage2D\0"
   "glCopyTexImage2DEXT\0"
   "\0"
   /* _mesa_function_pool[16822]: GetBufferPointervARB (will be remapped) */
   "iip\0"
   "glGetBufferPointerv\0"
   "glGetBufferPointervARB\0"
   "\0"
   /* _mesa_function_pool[16870]: ProgramEnvParameter4fARB (will be remapped) */
   "iiffff\0"
   "glProgramEnvParameter4fARB\0"
   "glProgramParameter4fNV\0"
   "\0"
   /* _mesa_function_pool[16928]: Uniform3ivARB (will be remapped) */
   "iip\0"
   "glUniform3iv\0"
   "glUniform3ivARB\0"
   "\0"
   /* _mesa_function_pool[16962]: Lightfv (offset 160) */
   "iip\0"
   "glLightfv\0"
   "\0"
   /* _mesa_function_pool[16977]: PrimitiveRestartIndexNV (will be remapped) */
   "i\0"
   "glPrimitiveRestartIndexNV\0"
   "glPrimitiveRestartIndex\0"
   "\0"
   /* _mesa_function_pool[17030]: ClearDepth (offset 208) */
   "d\0"
   "glClearDepth\0"
   "\0"
   /* _mesa_function_pool[17046]: GetFenceivNV (will be remapped) */
   "iip\0"
   "glGetFenceivNV\0"
   "\0"
   /* _mesa_function_pool[17066]: WindowPos4dvMESA (will be remapped) */
   "p\0"
   "glWindowPos4dvMESA\0"
   "\0"
   /* _mesa_function_pool[17088]: ColorSubTable (offset 346) */
   "iiiiip\0"
   "glColorSubTable\0"
   "glColorSubTableEXT\0"
   "\0"
   /* _mesa_function_pool[17131]: Color4fv (offset 30) */
   "p\0"
   "glColor4fv\0"
   "\0"
   /* _mesa_function_pool[17145]: MultiTexCoord4ivARB (offset 405) */
   "ip\0"
   "glMultiTexCoord4iv\0"
   "glMultiTexCoord4ivARB\0"
   "\0"
   /* _mesa_function_pool[17190]: ProgramLocalParameters4fvEXT (will be remapped) */
   "iiip\0"
   "glProgramLocalParameters4fvEXT\0"
   "\0"
   /* _mesa_function_pool[17227]: ColorPointer (offset 308) */
   "iiip\0"
   "glColorPointer\0"
   "\0"
   /* _mesa_function_pool[17248]: Rects (offset 92) */
   "iiii\0"
   "glRects\0"
   "\0"
   /* _mesa_function_pool[17262]: GetMapAttribParameterfvNV (dynamic) */
   "iiip\0"
   "glGetMapAttribParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[17296]: CreateShaderProgramEXT (will be remapped) */
   "ip\0"
   "glCreateShaderProgramEXT\0"
   "\0"
   /* _mesa_function_pool[17325]: ActiveProgramEXT (will be remapped) */
   "i\0"
   "glActiveProgramEXT\0"
   "\0"
   /* _mesa_function_pool[17347]: Lightiv (offset 162) */
   "iip\0"
   "glLightiv\0"
   "\0"
   /* _mesa_function_pool[17362]: VertexAttrib4sARB (will be remapped) */
   "iiiii\0"
   "glVertexAttrib4s\0"
   "glVertexAttrib4sARB\0"
   "\0"
   /* _mesa_function_pool[17406]: GetQueryObjectuivARB (will be remapped) */
   "iip\0"
   "glGetQueryObjectuiv\0"
   "glGetQueryObjectuivARB\0"
   "\0"
   /* _mesa_function_pool[17454]: GetTexParameteriv (offset 283) */
   "iip\0"
   "glGetTexParameteriv\0"
   "\0"
   /* _mesa_function_pool[17479]: MapParameterivNV (dynamic) */
   "iip\0"
   "glMapParameterivNV\0"
   "\0"
   /* _mesa_function_pool[17503]: GenRenderbuffersEXT (will be remapped) */
   "ip\0"
   "glGenRenderbuffers\0"
   "glGenRenderbuffersEXT\0"
   "\0"
   /* _mesa_function_pool[17548]: ClearBufferfv (will be remapped) */
   "iip\0"
   "glClearBufferfv\0"
   "\0"
   /* _mesa_function_pool[17569]: VertexAttrib2dvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib2dv\0"
   "glVertexAttrib2dvARB\0"
   "\0"
   /* _mesa_function_pool[17612]: EdgeFlagPointerEXT (will be remapped) */
   "iip\0"
   "glEdgeFlagPointerEXT\0"
   "\0"
   /* _mesa_function_pool[17638]: VertexAttribs2svNV (will be remapped) */
   "iip\0"
   "glVertexAttribs2svNV\0"
   "\0"
   /* _mesa_function_pool[17664]: WeightbvARB (dynamic) */
   "ip\0"
   "glWeightbvARB\0"
   "\0"
   /* _mesa_function_pool[17682]: VertexAttrib2fvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib2fv\0"
   "glVertexAttrib2fvARB\0"
   "\0"
   /* _mesa_function_pool[17725]: GetBufferParameterivARB (will be remapped) */
   "iip\0"
   "glGetBufferParameteriv\0"
   "glGetBufferParameterivARB\0"
   "\0"
   /* _mesa_function_pool[17779]: Rectdv (offset 87) */
   "pp\0"
   "glRectdv\0"
   "\0"
   /* _mesa_function_pool[17792]: ListParameteriSGIX (dynamic) */
   "iii\0"
   "glListParameteriSGIX\0"
   "\0"
   /* _mesa_function_pool[17818]: BlendEquationiARB (will be remapped) */
   "ii\0"
   "glBlendEquationiARB\0"
   "\0"
   /* _mesa_function_pool[17842]: ReplacementCodeuiColor4fNormal3fVertex3fSUN (dynamic) */
   "iffffffffff\0"
   "glReplacementCodeuiColor4fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[17901]: InstrumentsBufferSGIX (dynamic) */
   "ip\0"
   "glInstrumentsBufferSGIX\0"
   "\0"
   /* _mesa_function_pool[17929]: VertexAttrib4NivARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Niv\0"
   "glVertexAttrib4NivARB\0"
   "\0"
   /* _mesa_function_pool[17974]: DrawArraysInstancedARB (will be remapped) */
   "iiii\0"
   "glDrawArraysInstancedARB\0"
   "glDrawArraysInstancedEXT\0"
   "glDrawArraysInstanced\0"
   "\0"
   /* _mesa_function_pool[18052]: GetAttachedShaders (will be remapped) */
   "iipp\0"
   "glGetAttachedShaders\0"
   "\0"
   /* _mesa_function_pool[18079]: GenVertexArraysAPPLE (will be remapped) */
   "ip\0"
   "glGenVertexArraysAPPLE\0"
   "\0"
   /* _mesa_function_pool[18106]: ClearBufferfi (will be remapped) */
   "iifi\0"
   "glClearBufferfi\0"
   "\0"
   /* _mesa_function_pool[18128]: Materialiv (offset 172) */
   "iip\0"
   "glMaterialiv\0"
   "\0"
   /* _mesa_function_pool[18146]: PushClientAttrib (offset 335) */
   "i\0"
   "glPushClientAttrib\0"
   "\0"
   /* _mesa_function_pool[18168]: ProgramEnvParameters4fvEXT (will be remapped) */
   "iiip\0"
   "glProgramEnvParameters4fvEXT\0"
   "\0"
   /* _mesa_function_pool[18203]: TexCoord2fColor4fNormal3fVertex3fvSUN (dynamic) */
   "pppp\0"
   "glTexCoord2fColor4fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[18249]: WindowPos2iMESA (will be remapped) */
   "ii\0"
   "glWindowPos2i\0"
   "glWindowPos2iARB\0"
   "glWindowPos2iMESA\0"
   "\0"
   /* _mesa_function_pool[18302]: SampleMaskSGIS (will be remapped) */
   "fi\0"
   "glSampleMaskSGIS\0"
   "glSampleMaskEXT\0"
   "\0"
   /* _mesa_function_pool[18339]: SecondaryColor3fvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3fv\0"
   "glSecondaryColor3fvEXT\0"
   "\0"
   /* _mesa_function_pool[18385]: PolygonMode (offset 174) */
   "ii\0"
   "glPolygonMode\0"
   "\0"
   /* _mesa_function_pool[18403]: CompressedTexSubImage1DARB (will be remapped) */
   "iiiiiip\0"
   "glCompressedTexSubImage1D\0"
   "glCompressedTexSubImage1DARB\0"
   "\0"
   /* _mesa_function_pool[18467]: VertexAttribI1iEXT (will be remapped) */
   "ii\0"
   "glVertexAttribI1iEXT\0"
   "glVertexAttribI1i\0"
   "\0"
   /* _mesa_function_pool[18510]: GetVertexAttribivNV (will be remapped) */
   "iip\0"
   "glGetVertexAttribivNV\0"
   "\0"
   /* _mesa_function_pool[18537]: GetProgramStringARB (will be remapped) */
   "iip\0"
   "glGetProgramStringARB\0"
   "\0"
   /* _mesa_function_pool[18564]: VertexAttribIPointerEXT (will be remapped) */
   "iiiip\0"
   "glVertexAttribIPointerEXT\0"
   "glVertexAttribIPointer\0"
   "\0"
   /* _mesa_function_pool[18620]: TexBumpParameterfvATI (will be remapped) */
   "ip\0"
   "glTexBumpParameterfvATI\0"
   "\0"
   /* _mesa_function_pool[18648]: CompileShaderARB (will be remapped) */
   "i\0"
   "glCompileShader\0"
   "glCompileShaderARB\0"
   "\0"
   /* _mesa_function_pool[18686]: DeleteShader (will be remapped) */
   "i\0"
   "glDeleteShader\0"
   "\0"
   /* _mesa_function_pool[18704]: DisableClientState (offset 309) */
   "i\0"
   "glDisableClientState\0"
   "\0"
   /* _mesa_function_pool[18728]: TexGeni (offset 192) */
   "iii\0"
   "glTexGeni\0"
   "\0"
   /* _mesa_function_pool[18743]: TexGenf (offset 190) */
   "iif\0"
   "glTexGenf\0"
   "\0"
   /* _mesa_function_pool[18758]: Uniform3fARB (will be remapped) */
   "ifff\0"
   "glUniform3f\0"
   "glUniform3fARB\0"
   "\0"
   /* _mesa_function_pool[18791]: TexGend (offset 188) */
   "iid\0"
   "glTexGend\0"
   "\0"
   /* _mesa_function_pool[18806]: ListParameterfvSGIX (dynamic) */
   "iip\0"
   "glListParameterfvSGIX\0"
   "\0"
   /* _mesa_function_pool[18833]: GetPolygonStipple (offset 274) */
   "p\0"
   "glGetPolygonStipple\0"
   "\0"
   /* _mesa_function_pool[18856]: Tangent3dvEXT (dynamic) */
   "p\0"
   "glTangent3dvEXT\0"
   "\0"
   /* _mesa_function_pool[18875]: BindBufferOffsetEXT (will be remapped) */
   "iiii\0"
   "glBindBufferOffsetEXT\0"
   "\0"
   /* _mesa_function_pool[18903]: WindowPos3sMESA (will be remapped) */
   "iii\0"
   "glWindowPos3s\0"
   "glWindowPos3sARB\0"
   "glWindowPos3sMESA\0"
   "\0"
   /* _mesa_function_pool[18957]: VertexAttrib2svNV (will be remapped) */
   "ip\0"
   "glVertexAttrib2svNV\0"
   "\0"
   /* _mesa_function_pool[18981]: DisableIndexedEXT (will be remapped) */
   "ii\0"
   "glDisableIndexedEXT\0"
   "glDisablei\0"
   "\0"
   /* _mesa_function_pool[19016]: BindBufferBaseEXT (will be remapped) */
   "iii\0"
   "glBindBufferBaseEXT\0"
   "glBindBufferBase\0"
   "\0"
   /* _mesa_function_pool[19058]: TexCoord2fVertex3fvSUN (dynamic) */
   "pp\0"
   "glTexCoord2fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[19087]: WindowPos4sMESA (will be remapped) */
   "iiii\0"
   "glWindowPos4sMESA\0"
   "\0"
   /* _mesa_function_pool[19111]: VertexAttrib4NuivARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Nuiv\0"
   "glVertexAttrib4NuivARB\0"
   "\0"
   /* _mesa_function_pool[19158]: ClientActiveTextureARB (offset 375) */
   "i\0"
   "glClientActiveTexture\0"
   "glClientActiveTextureARB\0"
   "\0"
   /* _mesa_function_pool[19208]: PixelTexGenSGIX (will be remapped) */
   "i\0"
   "glPixelTexGenSGIX\0"
   "\0"
   /* _mesa_function_pool[19229]: ReplacementCodeusvSUN (dynamic) */
   "p\0"
   "glReplacementCodeusvSUN\0"
   "\0"
   /* _mesa_function_pool[19256]: Uniform4fARB (will be remapped) */
   "iffff\0"
   "glUniform4f\0"
   "glUniform4fARB\0"
   "\0"
   /* _mesa_function_pool[19290]: Color4sv (offset 34) */
   "p\0"
   "glColor4sv\0"
   "\0"
   /* _mesa_function_pool[19304]: FlushMappedBufferRange (will be remapped) */
   "iii\0"
   "glFlushMappedBufferRange\0"
   "\0"
   /* _mesa_function_pool[19334]: IsProgramNV (will be remapped) */
   "i\0"
   "glIsProgramARB\0"
   "glIsProgramNV\0"
   "\0"
   /* _mesa_function_pool[19366]: FlushMappedBufferRangeAPPLE (will be remapped) */
   "iii\0"
   "glFlushMappedBufferRangeAPPLE\0"
   "\0"
   /* _mesa_function_pool[19401]: PixelZoom (offset 246) */
   "ff\0"
   "glPixelZoom\0"
   "\0"
   /* _mesa_function_pool[19417]: ReplacementCodePointerSUN (dynamic) */
   "iip\0"
   "glReplacementCodePointerSUN\0"
   "\0"
   /* _mesa_function_pool[19450]: ProgramEnvParameter4dARB (will be remapped) */
   "iidddd\0"
   "glProgramEnvParameter4dARB\0"
   "glProgramParameter4dNV\0"
   "\0"
   /* _mesa_function_pool[19508]: ColorTableParameterfv (offset 340) */
   "iip\0"
   "glColorTableParameterfv\0"
   "glColorTableParameterfvSGI\0"
   "\0"
   /* _mesa_function_pool[19564]: FragmentLightModelfSGIX (dynamic) */
   "if\0"
   "glFragmentLightModelfSGIX\0"
   "\0"
   /* _mesa_function_pool[19594]: Binormal3bvEXT (dynamic) */
   "p\0"
   "glBinormal3bvEXT\0"
   "\0"
   /* _mesa_function_pool[19614]: PixelMapuiv (offset 252) */
   "iip\0"
   "glPixelMapuiv\0"
   "\0"
   /* _mesa_function_pool[19633]: Color3dv (offset 12) */
   "p\0"
   "glColor3dv\0"
   "\0"
   /* _mesa_function_pool[19647]: IsTexture (offset 330) */
   "i\0"
   "glIsTexture\0"
   "glIsTextureEXT\0"
   "\0"
   /* _mesa_function_pool[19677]: VertexWeightfvEXT (dynamic) */
   "p\0"
   "glVertexWeightfvEXT\0"
   "\0"
   /* _mesa_function_pool[19700]: VertexAttrib1dARB (will be remapped) */
   "id\0"
   "glVertexAttrib1d\0"
   "glVertexAttrib1dARB\0"
   "\0"
   /* _mesa_function_pool[19741]: ImageTransformParameterivHP (dynamic) */
   "iip\0"
   "glImageTransformParameterivHP\0"
   "\0"
   /* _mesa_function_pool[19776]: TexCoord4i (offset 122) */
   "iiii\0"
   "glTexCoord4i\0"
   "\0"
   /* _mesa_function_pool[19795]: DeleteQueriesARB (will be remapped) */
   "ip\0"
   "glDeleteQueries\0"
   "glDeleteQueriesARB\0"
   "\0"
   /* _mesa_function_pool[19834]: Color4ubVertex2fSUN (dynamic) */
   "iiiiff\0"
   "glColor4ubVertex2fSUN\0"
   "\0"
   /* _mesa_function_pool[19864]: FragmentColorMaterialSGIX (dynamic) */
   "ii\0"
   "glFragmentColorMaterialSGIX\0"
   "\0"
   /* _mesa_function_pool[19896]: CurrentPaletteMatrixARB (dynamic) */
   "i\0"
   "glCurrentPaletteMatrixARB\0"
   "\0"
   /* _mesa_function_pool[19925]: GetMapdv (offset 266) */
   "iip\0"
   "glGetMapdv\0"
   "\0"
   /* _mesa_function_pool[19941]: ObjectPurgeableAPPLE (will be remapped) */
   "iii\0"
   "glObjectPurgeableAPPLE\0"
   "\0"
   /* _mesa_function_pool[19969]: GetStringi (will be remapped) */
   "ii\0"
   "glGetStringi\0"
   "\0"
   /* _mesa_function_pool[19986]: SamplePatternSGIS (will be remapped) */
   "i\0"
   "glSamplePatternSGIS\0"
   "glSamplePatternEXT\0"
   "\0"
   /* _mesa_function_pool[20028]: PixelStoref (offset 249) */
   "if\0"
   "glPixelStoref\0"
   "\0"
   /* _mesa_function_pool[20046]: IsQueryARB (will be remapped) */
   "i\0"
   "glIsQuery\0"
   "glIsQueryARB\0"
   "\0"
   /* _mesa_function_pool[20072]: ReplacementCodeuiColor4ubVertex3fSUN (dynamic) */
   "iiiiifff\0"
   "glReplacementCodeuiColor4ubVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[20121]: PixelStorei (offset 250) */
   "ii\0"
   "glPixelStorei\0"
   "\0"
   /* _mesa_function_pool[20139]: VertexAttrib4usvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4usv\0"
   "glVertexAttrib4usvARB\0"
   "\0"
   /* _mesa_function_pool[20184]: LinkProgramARB (will be remapped) */
   "i\0"
   "glLinkProgram\0"
   "glLinkProgramARB\0"
   "\0"
   /* _mesa_function_pool[20218]: VertexAttrib2fNV (will be remapped) */
   "iff\0"
   "glVertexAttrib2fNV\0"
   "\0"
   /* _mesa_function_pool[20242]: ShaderSourceARB (will be remapped) */
   "iipp\0"
   "glShaderSource\0"
   "glShaderSourceARB\0"
   "\0"
   /* _mesa_function_pool[20281]: FragmentMaterialiSGIX (dynamic) */
   "iii\0"
   "glFragmentMaterialiSGIX\0"
   "\0"
   /* _mesa_function_pool[20310]: EvalCoord2dv (offset 233) */
   "p\0"
   "glEvalCoord2dv\0"
   "\0"
   /* _mesa_function_pool[20328]: VertexAttrib3svARB (will be remapped) */
   "ip\0"
   "glVertexAttrib3sv\0"
   "glVertexAttrib3svARB\0"
   "\0"
   /* _mesa_function_pool[20371]: ColorMaterial (offset 151) */
   "ii\0"
   "glColorMaterial\0"
   "\0"
   /* _mesa_function_pool[20391]: CompressedTexSubImage3DARB (will be remapped) */
   "iiiiiiiiiip\0"
   "glCompressedTexSubImage3D\0"
   "glCompressedTexSubImage3DARB\0"
   "\0"
   /* _mesa_function_pool[20459]: WindowPos2ivMESA (will be remapped) */
   "p\0"
   "glWindowPos2iv\0"
   "glWindowPos2ivARB\0"
   "glWindowPos2ivMESA\0"
   "\0"
   /* _mesa_function_pool[20514]: IsFramebufferEXT (will be remapped) */
   "i\0"
   "glIsFramebuffer\0"
   "glIsFramebufferEXT\0"
   "\0"
   /* _mesa_function_pool[20552]: Uniform4ivARB (will be remapped) */
   "iip\0"
   "glUniform4iv\0"
   "glUniform4ivARB\0"
   "\0"
   /* _mesa_function_pool[20586]: GetVertexAttribdvARB (will be remapped) */
   "iip\0"
   "glGetVertexAttribdv\0"
   "glGetVertexAttribdvARB\0"
   "\0"
   /* _mesa_function_pool[20634]: TexBumpParameterivATI (will be remapped) */
   "ip\0"
   "glTexBumpParameterivATI\0"
   "\0"
   /* _mesa_function_pool[20662]: GetSeparableFilter (offset 359) */
   "iiippp\0"
   "glGetSeparableFilter\0"
   "glGetSeparableFilterEXT\0"
   "\0"
   /* _mesa_function_pool[20715]: Binormal3dEXT (dynamic) */
   "ddd\0"
   "glBinormal3dEXT\0"
   "\0"
   /* _mesa_function_pool[20736]: SpriteParameteriSGIX (dynamic) */
   "ii\0"
   "glSpriteParameteriSGIX\0"
   "\0"
   /* _mesa_function_pool[20763]: RequestResidentProgramsNV (will be remapped) */
   "ip\0"
   "glRequestResidentProgramsNV\0"
   "\0"
   /* _mesa_function_pool[20795]: TagSampleBufferSGIX (dynamic) */
   "\0"
   "glTagSampleBufferSGIX\0"
   "\0"
   /* _mesa_function_pool[20819]: TransformFeedbackVaryingsEXT (will be remapped) */
   "iipi\0"
   "glTransformFeedbackVaryingsEXT\0"
   "glTransformFeedbackVaryings\0"
   "\0"
   /* _mesa_function_pool[20884]: FeedbackBuffer (offset 194) */
   "iip\0"
   "glFeedbackBuffer\0"
   "\0"
   /* _mesa_function_pool[20906]: RasterPos2iv (offset 67) */
   "p\0"
   "glRasterPos2iv\0"
   "\0"
   /* _mesa_function_pool[20924]: TexImage1D (offset 182) */
   "iiiiiiip\0"
   "glTexImage1D\0"
   "\0"
   /* _mesa_function_pool[20947]: ListParameterivSGIX (dynamic) */
   "iip\0"
   "glListParameterivSGIX\0"
   "\0"
   /* _mesa_function_pool[20974]: MultiDrawElementsEXT (will be remapped) */
   "ipipi\0"
   "glMultiDrawElements\0"
   "glMultiDrawElementsEXT\0"
   "\0"
   /* _mesa_function_pool[21024]: Color3s (offset 17) */
   "iii\0"
   "glColor3s\0"
   "\0"
   /* _mesa_function_pool[21039]: Uniform1ivARB (will be remapped) */
   "iip\0"
   "glUniform1iv\0"
   "glUniform1ivARB\0"
   "\0"
   /* _mesa_function_pool[21073]: WindowPos2sMESA (will be remapped) */
   "ii\0"
   "glWindowPos2s\0"
   "glWindowPos2sARB\0"
   "glWindowPos2sMESA\0"
   "\0"
   /* _mesa_function_pool[21126]: WeightusvARB (dynamic) */
   "ip\0"
   "glWeightusvARB\0"
   "\0"
   /* _mesa_function_pool[21145]: TexCoordPointer (offset 320) */
   "iiip\0"
   "glTexCoordPointer\0"
   "\0"
   /* _mesa_function_pool[21169]: FogCoordPointerEXT (will be remapped) */
   "iip\0"
   "glFogCoordPointer\0"
   "glFogCoordPointerEXT\0"
   "\0"
   /* _mesa_function_pool[21213]: IndexMaterialEXT (dynamic) */
   "ii\0"
   "glIndexMaterialEXT\0"
   "\0"
   /* _mesa_function_pool[21236]: Color3i (offset 15) */
   "iii\0"
   "glColor3i\0"
   "\0"
   /* _mesa_function_pool[21251]: FrontFace (offset 157) */
   "i\0"
   "glFrontFace\0"
   "\0"
   /* _mesa_function_pool[21266]: EvalCoord2d (offset 232) */
   "dd\0"
   "glEvalCoord2d\0"
   "\0"
   /* _mesa_function_pool[21284]: SecondaryColor3ubvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3ubv\0"
   "glSecondaryColor3ubvEXT\0"
   "\0"
   /* _mesa_function_pool[21332]: EvalCoord2f (offset 234) */
   "ff\0"
   "glEvalCoord2f\0"
   "\0"
   /* _mesa_function_pool[21350]: VertexAttrib4dvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4dv\0"
   "glVertexAttrib4dvARB\0"
   "\0"
   /* _mesa_function_pool[21393]: BindAttribLocationARB (will be remapped) */
   "iip\0"
   "glBindAttribLocation\0"
   "glBindAttribLocationARB\0"
   "\0"
   /* _mesa_function_pool[21443]: Color3b (offset 9) */
   "iii\0"
   "glColor3b\0"
   "\0"
   /* _mesa_function_pool[21458]: MultiTexCoord2dARB (offset 384) */
   "idd\0"
   "glMultiTexCoord2d\0"
   "glMultiTexCoord2dARB\0"
   "\0"
   /* _mesa_function_pool[21502]: ExecuteProgramNV (will be remapped) */
   "iip\0"
   "glExecuteProgramNV\0"
   "\0"
   /* _mesa_function_pool[21526]: Color3f (offset 13) */
   "fff\0"
   "glColor3f\0"
   "\0"
   /* _mesa_function_pool[21541]: LightEnviSGIX (dynamic) */
   "ii\0"
   "glLightEnviSGIX\0"
   "\0"
   /* _mesa_function_pool[21561]: Color3d (offset 11) */
   "ddd\0"
   "glColor3d\0"
   "\0"
   /* _mesa_function_pool[21576]: Normal3dv (offset 55) */
   "p\0"
   "glNormal3dv\0"
   "\0"
   /* _mesa_function_pool[21591]: Lightf (offset 159) */
   "iif\0"
   "glLightf\0"
   "\0"
   /* _mesa_function_pool[21605]: ReplacementCodeuiSUN (dynamic) */
   "i\0"
   "glReplacementCodeuiSUN\0"
   "\0"
   /* _mesa_function_pool[21631]: MatrixMode (offset 293) */
   "i\0"
   "glMatrixMode\0"
   "\0"
   /* _mesa_function_pool[21647]: GetPixelMapusv (offset 273) */
   "ip\0"
   "glGetPixelMapusv\0"
   "\0"
   /* _mesa_function_pool[21668]: Lighti (offset 161) */
   "iii\0"
   "glLighti\0"
   "\0"
   /* _mesa_function_pool[21682]: VertexAttribPointerNV (will be remapped) */
   "iiiip\0"
   "glVertexAttribPointerNV\0"
   "\0"
   /* _mesa_function_pool[21713]: ClearDepthf (will be remapped) */
   "f\0"
   "glClearDepthf\0"
   "\0"
   /* _mesa_function_pool[21730]: GetBooleanIndexedvEXT (will be remapped) */
   "iip\0"
   "glGetBooleanIndexedvEXT\0"
   "glGetBooleani_v\0"
   "\0"
   /* _mesa_function_pool[21775]: GetFramebufferAttachmentParameterivEXT (will be remapped) */
   "iiip\0"
   "glGetFramebufferAttachmentParameteriv\0"
   "glGetFramebufferAttachmentParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[21860]: PixelTransformParameterfEXT (dynamic) */
   "iif\0"
   "glPixelTransformParameterfEXT\0"
   "\0"
   /* _mesa_function_pool[21895]: MultiTexCoord4dvARB (offset 401) */
   "ip\0"
   "glMultiTexCoord4dv\0"
   "glMultiTexCoord4dvARB\0"
   "\0"
   /* _mesa_function_pool[21940]: PixelTransformParameteriEXT (dynamic) */
   "iii\0"
   "glPixelTransformParameteriEXT\0"
   "\0"
   /* _mesa_function_pool[21975]: GetDoublev (offset 260) */
   "ip\0"
   "glGetDoublev\0"
   "\0"
   /* _mesa_function_pool[21992]: MultMatrixd (offset 295) */
   "p\0"
   "glMultMatrixd\0"
   "\0"
   /* _mesa_function_pool[22009]: MultMatrixf (offset 294) */
   "p\0"
   "glMultMatrixf\0"
   "\0"
   /* _mesa_function_pool[22026]: VertexAttribI4bvEXT (will be remapped) */
   "ip\0"
   "glVertexAttribI4bvEXT\0"
   "glVertexAttribI4bv\0"
   "\0"
   /* _mesa_function_pool[22071]: TexCoord2fColor4ubVertex3fSUN (dynamic) */
   "ffiiiifff\0"
   "glTexCoord2fColor4ubVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[22114]: Uniform1iARB (will be remapped) */
   "ii\0"
   "glUniform1i\0"
   "glUniform1iARB\0"
   "\0"
   /* _mesa_function_pool[22145]: VertexAttribPointerARB (will be remapped) */
   "iiiiip\0"
   "glVertexAttribPointer\0"
   "glVertexAttribPointerARB\0"
   "\0"
   /* _mesa_function_pool[22200]: VertexAttrib3sNV (will be remapped) */
   "iiii\0"
   "glVertexAttrib3sNV\0"
   "\0"
   /* _mesa_function_pool[22225]: SharpenTexFuncSGIS (dynamic) */
   "iip\0"
   "glSharpenTexFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[22251]: MultiTexCoord4fvARB (offset 403) */
   "ip\0"
   "glMultiTexCoord4fv\0"
   "glMultiTexCoord4fvARB\0"
   "\0"
   /* _mesa_function_pool[22296]: Uniform2uiEXT (will be remapped) */
   "iii\0"
   "glUniform2uiEXT\0"
   "glUniform2ui\0"
   "\0"
   /* _mesa_function_pool[22330]: UniformMatrix2x3fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix2x3fv\0"
   "\0"
   /* _mesa_function_pool[22357]: TrackMatrixNV (will be remapped) */
   "iiii\0"
   "glTrackMatrixNV\0"
   "\0"
   /* _mesa_function_pool[22379]: CombinerParameteriNV (will be remapped) */
   "ii\0"
   "glCombinerParameteriNV\0"
   "\0"
   /* _mesa_function_pool[22406]: DeleteAsyncMarkersSGIX (dynamic) */
   "ii\0"
   "glDeleteAsyncMarkersSGIX\0"
   "\0"
   /* _mesa_function_pool[22435]: ReplacementCodeusSUN (dynamic) */
   "i\0"
   "glReplacementCodeusSUN\0"
   "\0"
   /* _mesa_function_pool[22461]: IsAsyncMarkerSGIX (dynamic) */
   "i\0"
   "glIsAsyncMarkerSGIX\0"
   "\0"
   /* _mesa_function_pool[22484]: FrameZoomSGIX (dynamic) */
   "i\0"
   "glFrameZoomSGIX\0"
   "\0"
   /* _mesa_function_pool[22503]: Normal3fVertex3fvSUN (dynamic) */
   "pp\0"
   "glNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[22530]: RasterPos4sv (offset 85) */
   "p\0"
   "glRasterPos4sv\0"
   "\0"
   /* _mesa_function_pool[22548]: VertexAttrib4NsvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Nsv\0"
   "glVertexAttrib4NsvARB\0"
   "\0"
   /* _mesa_function_pool[22593]: VertexAttrib3fvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib3fv\0"
   "glVertexAttrib3fvARB\0"
   "\0"
   /* _mesa_function_pool[22636]: ClearColor (offset 206) */
   "ffff\0"
   "glClearColor\0"
   "\0"
   /* _mesa_function_pool[22655]: GetSynciv (will be remapped) */
   "iiipp\0"
   "glGetSynciv\0"
   "\0"
   /* _mesa_function_pool[22674]: ClearColorIiEXT (will be remapped) */
   "iiii\0"
   "glClearColorIiEXT\0"
   "\0"
   /* _mesa_function_pool[22698]: DeleteFramebuffersEXT (will be remapped) */
   "ip\0"
   "glDeleteFramebuffers\0"
   "glDeleteFramebuffersEXT\0"
   "\0"
   /* _mesa_function_pool[22747]: GlobalAlphaFactorsSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactorsSUN\0"
   "\0"
   /* _mesa_function_pool[22774]: IsEnabledIndexedEXT (will be remapped) */
   "ii\0"
   "glIsEnabledIndexedEXT\0"
   "glIsEnabledi\0"
   "\0"
   /* _mesa_function_pool[22813]: TexEnviv (offset 187) */
   "iip\0"
   "glTexEnviv\0"
   "\0"
   /* _mesa_function_pool[22829]: TexSubImage3D (offset 372) */
   "iiiiiiiiiip\0"
   "glTexSubImage3D\0"
   "glTexSubImage3DEXT\0"
   "\0"
   /* _mesa_function_pool[22877]: Tangent3fEXT (dynamic) */
   "fff\0"
   "glTangent3fEXT\0"
   "\0"
   /* _mesa_function_pool[22897]: SecondaryColor3uivEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3uiv\0"
   "glSecondaryColor3uivEXT\0"
   "\0"
   /* _mesa_function_pool[22945]: MatrixIndexubvARB (dynamic) */
   "ip\0"
   "glMatrixIndexubvARB\0"
   "\0"
   /* _mesa_function_pool[22969]: Color4fNormal3fVertex3fSUN (dynamic) */
   "ffffffffff\0"
   "glColor4fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[23010]: PixelTexGenParameterfSGIS (will be remapped) */
   "if\0"
   "glPixelTexGenParameterfSGIS\0"
   "\0"
   /* _mesa_function_pool[23042]: CreateShader (will be remapped) */
   "i\0"
   "glCreateShader\0"
   "\0"
   /* _mesa_function_pool[23060]: GetColorTableParameterfv (offset 344) */
   "iip\0"
   "glGetColorTableParameterfv\0"
   "glGetColorTableParameterfvSGI\0"
   "glGetColorTableParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[23152]: FragmentLightModelfvSGIX (dynamic) */
   "ip\0"
   "glFragmentLightModelfvSGIX\0"
   "\0"
   /* _mesa_function_pool[23183]: Bitmap (offset 8) */
   "iiffffp\0"
   "glBitmap\0"
   "\0"
   /* _mesa_function_pool[23201]: MultiTexCoord3fARB (offset 394) */
   "ifff\0"
   "glMultiTexCoord3f\0"
   "glMultiTexCoord3fARB\0"
   "\0"
   /* _mesa_function_pool[23246]: GetTexLevelParameterfv (offset 284) */
   "iiip\0"
   "glGetTexLevelParameterfv\0"
   "\0"
   /* _mesa_function_pool[23277]: GetPixelTexGenParameterfvSGIS (will be remapped) */
   "ip\0"
   "glGetPixelTexGenParameterfvSGIS\0"
   "\0"
   /* _mesa_function_pool[23313]: GenFramebuffersEXT (will be remapped) */
   "ip\0"
   "glGenFramebuffers\0"
   "glGenFramebuffersEXT\0"
   "\0"
   /* _mesa_function_pool[23356]: VertexAttribDivisor (will be remapped) */
   "ii\0"
   "glVertexAttribDivisor\0"
   "\0"
   /* _mesa_function_pool[23382]: GetProgramParameterdvNV (will be remapped) */
   "iiip\0"
   "glGetProgramParameterdvNV\0"
   "\0"
   /* _mesa_function_pool[23414]: Vertex2sv (offset 133) */
   "p\0"
   "glVertex2sv\0"
   "\0"
   /* _mesa_function_pool[23429]: GetIntegerv (offset 263) */
   "ip\0"
   "glGetIntegerv\0"
   "\0"
   /* _mesa_function_pool[23447]: IsVertexArrayAPPLE (will be remapped) */
   "i\0"
   "glIsVertexArray\0"
   "glIsVertexArrayAPPLE\0"
   "\0"
   /* _mesa_function_pool[23487]: FragmentLightfvSGIX (dynamic) */
   "iip\0"
   "glFragmentLightfvSGIX\0"
   "\0"
   /* _mesa_function_pool[23514]: VertexAttribDivisorARB (will be remapped) */
   "ii\0"
   "glVertexAttribDivisorARB\0"
   "\0"
   /* _mesa_function_pool[23543]: DetachShader (will be remapped) */
   "ii\0"
   "glDetachShader\0"
   "\0"
   /* _mesa_function_pool[23562]: VertexAttrib4NubARB (will be remapped) */
   "iiiii\0"
   "glVertexAttrib4Nub\0"
   "glVertexAttrib4NubARB\0"
   "\0"
   /* _mesa_function_pool[23610]: GetProgramEnvParameterfvARB (will be remapped) */
   "iip\0"
   "glGetProgramEnvParameterfvARB\0"
   "\0"
   /* _mesa_function_pool[23645]: GetTrackMatrixivNV (will be remapped) */
   "iiip\0"
   "glGetTrackMatrixivNV\0"
   "\0"
   /* _mesa_function_pool[23672]: VertexAttrib3svNV (will be remapped) */
   "ip\0"
   "glVertexAttrib3svNV\0"
   "\0"
   /* _mesa_function_pool[23696]: Uniform4fvARB (will be remapped) */
   "iip\0"
   "glUniform4fv\0"
   "glUniform4fvARB\0"
   "\0"
   /* _mesa_function_pool[23730]: MultTransposeMatrixfARB (will be remapped) */
   "p\0"
   "glMultTransposeMatrixf\0"
   "glMultTransposeMatrixfARB\0"
   "\0"
   /* _mesa_function_pool[23782]: GetTexEnviv (offset 277) */
   "iip\0"
   "glGetTexEnviv\0"
   "\0"
   /* _mesa_function_pool[23801]: ColorFragmentOp1ATI (will be remapped) */
   "iiiiiii\0"
   "glColorFragmentOp1ATI\0"
   "\0"
   /* _mesa_function_pool[23832]: GetUniformfvARB (will be remapped) */
   "iip\0"
   "glGetUniformfv\0"
   "glGetUniformfvARB\0"
   "\0"
   /* _mesa_function_pool[23870]: EGLImageTargetRenderbufferStorageOES (will be remapped) */
   "ip\0"
   "glEGLImageTargetRenderbufferStorageOES\0"
   "\0"
   /* _mesa_function_pool[23913]: VertexAttribI2ivEXT (will be remapped) */
   "ip\0"
   "glVertexAttribI2ivEXT\0"
   "glVertexAttribI2iv\0"
   "\0"
   /* _mesa_function_pool[23958]: PopClientAttrib (offset 334) */
   "\0"
   "glPopClientAttrib\0"
   "\0"
   /* _mesa_function_pool[23978]: ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN (dynamic) */
   "iffffffffffff\0"
   "glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[24049]: DetachObjectARB (will be remapped) */
   "ii\0"
   "glDetachObjectARB\0"
   "\0"
   /* _mesa_function_pool[24071]: VertexBlendARB (dynamic) */
   "i\0"
   "glVertexBlendARB\0"
   "\0"
   /* _mesa_function_pool[24091]: WindowPos3iMESA (will be remapped) */
   "iii\0"
   "glWindowPos3i\0"
   "glWindowPos3iARB\0"
   "glWindowPos3iMESA\0"
   "\0"
   /* _mesa_function_pool[24145]: SeparableFilter2D (offset 360) */
   "iiiiiipp\0"
   "glSeparableFilter2D\0"
   "glSeparableFilter2DEXT\0"
   "\0"
   /* _mesa_function_pool[24198]: ProgramParameteriARB (will be remapped) */
   "iii\0"
   "glProgramParameteriARB\0"
   "\0"
   /* _mesa_function_pool[24226]: Map1d (offset 220) */
   "iddiip\0"
   "glMap1d\0"
   "\0"
   /* _mesa_function_pool[24242]: Map1f (offset 221) */
   "iffiip\0"
   "glMap1f\0"
   "\0"
   /* _mesa_function_pool[24258]: CompressedTexImage2DARB (will be remapped) */
   "iiiiiiip\0"
   "glCompressedTexImage2D\0"
   "glCompressedTexImage2DARB\0"
   "\0"
   /* _mesa_function_pool[24317]: ArrayElement (offset 306) */
   "i\0"
   "glArrayElement\0"
   "glArrayElementEXT\0"
   "\0"
   /* _mesa_function_pool[24353]: TexImage2D (offset 183) */
   "iiiiiiiip\0"
   "glTexImage2D\0"
   "\0"
   /* _mesa_function_pool[24377]: DepthBoundsEXT (will be remapped) */
   "dd\0"
   "glDepthBoundsEXT\0"
   "\0"
   /* _mesa_function_pool[24398]: ProgramParameters4fvNV (will be remapped) */
   "iiip\0"
   "glProgramParameters4fvNV\0"
   "\0"
   /* _mesa_function_pool[24429]: DeformationMap3fSGIX (dynamic) */
   "iffiiffiiffiip\0"
   "glDeformationMap3fSGIX\0"
   "\0"
   /* _mesa_function_pool[24468]: GetProgramivNV (will be remapped) */
   "iip\0"
   "glGetProgramivNV\0"
   "\0"
   /* _mesa_function_pool[24490]: GetFragDataLocationEXT (will be remapped) */
   "ip\0"
   "glGetFragDataLocationEXT\0"
   "glGetFragDataLocation\0"
   "\0"
   /* _mesa_function_pool[24541]: GetMinmaxParameteriv (offset 366) */
   "iip\0"
   "glGetMinmaxParameteriv\0"
   "glGetMinmaxParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[24595]: PixelTransferf (offset 247) */
   "if\0"
   "glPixelTransferf\0"
   "\0"
   /* _mesa_function_pool[24616]: CopyTexImage1D (offset 323) */
   "iiiiiii\0"
   "glCopyTexImage1D\0"
   "glCopyTexImage1DEXT\0"
   "\0"
   /* _mesa_function_pool[24662]: PushMatrix (offset 298) */
   "\0"
   "glPushMatrix\0"
   "\0"
   /* _mesa_function_pool[24677]: Fogiv (offset 156) */
   "ip\0"
   "glFogiv\0"
   "\0"
   /* _mesa_function_pool[24689]: TexCoord1dv (offset 95) */
   "p\0"
   "glTexCoord1dv\0"
   "\0"
   /* _mesa_function_pool[24706]: AlphaFragmentOp3ATI (will be remapped) */
   "iiiiiiiiiiii\0"
   "glAlphaFragmentOp3ATI\0"
   "\0"
   /* _mesa_function_pool[24742]: PixelTransferi (offset 248) */
   "ii\0"
   "glPixelTransferi\0"
   "\0"
   /* _mesa_function_pool[24763]: GetVertexAttribdvNV (will be remapped) */
   "iip\0"
   "glGetVertexAttribdvNV\0"
   "\0"
   /* _mesa_function_pool[24790]: VertexAttrib3fvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib3fvNV\0"
   "\0"
   /* _mesa_function_pool[24814]: Rotatef (offset 300) */
   "ffff\0"
   "glRotatef\0"
   "\0"
   /* _mesa_function_pool[24830]: GetFinalCombinerInputParameterivNV (will be remapped) */
   "iip\0"
   "glGetFinalCombinerInputParameterivNV\0"
   "\0"
   /* _mesa_function_pool[24872]: Vertex3i (offset 138) */
   "iii\0"
   "glVertex3i\0"
   "\0"
   /* _mesa_function_pool[24888]: Vertex3f (offset 136) */
   "fff\0"
   "glVertex3f\0"
   "\0"
   /* _mesa_function_pool[24904]: Clear (offset 203) */
   "i\0"
   "glClear\0"
   "\0"
   /* _mesa_function_pool[24915]: Vertex3d (offset 134) */
   "ddd\0"
   "glVertex3d\0"
   "\0"
   /* _mesa_function_pool[24931]: GetMapParameterivNV (dynamic) */
   "iip\0"
   "glGetMapParameterivNV\0"
   "\0"
   /* _mesa_function_pool[24958]: Uniform4iARB (will be remapped) */
   "iiiii\0"
   "glUniform4i\0"
   "glUniform4iARB\0"
   "\0"
   /* _mesa_function_pool[24992]: ReadBuffer (offset 254) */
   "i\0"
   "glReadBuffer\0"
   "\0"
   /* _mesa_function_pool[25008]: ConvolutionParameteri (offset 352) */
   "iii\0"
   "glConvolutionParameteri\0"
   "glConvolutionParameteriEXT\0"
   "\0"
   /* _mesa_function_pool[25064]: Ortho (offset 296) */
   "dddddd\0"
   "glOrtho\0"
   "\0"
   /* _mesa_function_pool[25080]: Binormal3sEXT (dynamic) */
   "iii\0"
   "glBinormal3sEXT\0"
   "\0"
   /* _mesa_function_pool[25101]: ListBase (offset 6) */
   "i\0"
   "glListBase\0"
   "\0"
   /* _mesa_function_pool[25115]: Vertex3s (offset 140) */
   "iii\0"
   "glVertex3s\0"
   "\0"
   /* _mesa_function_pool[25131]: ConvolutionParameterf (offset 350) */
   "iif\0"
   "glConvolutionParameterf\0"
   "glConvolutionParameterfEXT\0"
   "\0"
   /* _mesa_function_pool[25187]: GetColorTableParameteriv (offset 345) */
   "iip\0"
   "glGetColorTableParameteriv\0"
   "glGetColorTableParameterivSGI\0"
   "glGetColorTableParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[25279]: ProgramEnvParameter4dvARB (will be remapped) */
   "iip\0"
   "glProgramEnvParameter4dvARB\0"
   "glProgramParameter4dvNV\0"
   "\0"
   /* _mesa_function_pool[25336]: ShadeModel (offset 177) */
   "i\0"
   "glShadeModel\0"
   "\0"
   /* _mesa_function_pool[25352]: VertexAttribs2fvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs2fvNV\0"
   "\0"
   /* _mesa_function_pool[25378]: Rectiv (offset 91) */
   "pp\0"
   "glRectiv\0"
   "\0"
   /* _mesa_function_pool[25391]: UseProgramObjectARB (will be remapped) */
   "i\0"
   "glUseProgram\0"
   "glUseProgramObjectARB\0"
   "\0"
   /* _mesa_function_pool[25429]: GetMapParameterfvNV (dynamic) */
   "iip\0"
   "glGetMapParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[25456]: EndConditionalRenderNV (will be remapped) */
   "\0"
   "glEndConditionalRenderNV\0"
   "glEndConditionalRender\0"
   "\0"
   /* _mesa_function_pool[25506]: PassTexCoordATI (will be remapped) */
   "iii\0"
   "glPassTexCoordATI\0"
   "\0"
   /* _mesa_function_pool[25529]: DeleteProgram (will be remapped) */
   "i\0"
   "glDeleteProgram\0"
   "\0"
   /* _mesa_function_pool[25548]: Tangent3ivEXT (dynamic) */
   "p\0"
   "glTangent3ivEXT\0"
   "\0"
   /* _mesa_function_pool[25567]: Tangent3dEXT (dynamic) */
   "ddd\0"
   "glTangent3dEXT\0"
   "\0"
   /* _mesa_function_pool[25587]: SecondaryColor3dvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3dv\0"
   "glSecondaryColor3dvEXT\0"
   "\0"
   /* _mesa_function_pool[25633]: AlphaFragmentOp2ATI (will be remapped) */
   "iiiiiiiii\0"
   "glAlphaFragmentOp2ATI\0"
   "\0"
   /* _mesa_function_pool[25666]: Vertex2fv (offset 129) */
   "p\0"
   "glVertex2fv\0"
   "\0"
   /* _mesa_function_pool[25681]: MultiDrawArraysEXT (will be remapped) */
   "ippi\0"
   "glMultiDrawArrays\0"
   "glMultiDrawArraysEXT\0"
   "\0"
   /* _mesa_function_pool[25726]: BindRenderbufferEXT (will be remapped) */
   "ii\0"
   "glBindRenderbuffer\0"
   "glBindRenderbufferEXT\0"
   "\0"
   /* _mesa_function_pool[25771]: MultiTexCoord4dARB (offset 400) */
   "idddd\0"
   "glMultiTexCoord4d\0"
   "glMultiTexCoord4dARB\0"
   "\0"
   /* _mesa_function_pool[25817]: FramebufferTextureFaceARB (will be remapped) */
   "iiiii\0"
   "glFramebufferTextureFaceARB\0"
   "\0"
   /* _mesa_function_pool[25852]: Vertex3sv (offset 141) */
   "p\0"
   "glVertex3sv\0"
   "\0"
   /* _mesa_function_pool[25867]: SecondaryColor3usEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3us\0"
   "glSecondaryColor3usEXT\0"
   "\0"
   /* _mesa_function_pool[25915]: ProgramLocalParameter4fvARB (will be remapped) */
   "iip\0"
   "glProgramLocalParameter4fvARB\0"
   "\0"
   /* _mesa_function_pool[25950]: DeleteProgramsNV (will be remapped) */
   "ip\0"
   "glDeleteProgramsARB\0"
   "glDeleteProgramsNV\0"
   "\0"
   /* _mesa_function_pool[25993]: EvalMesh1 (offset 236) */
   "iii\0"
   "glEvalMesh1\0"
   "\0"
   /* _mesa_function_pool[26010]: PauseTransformFeedback (will be remapped) */
   "\0"
   "glPauseTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[26037]: MultiTexCoord1sARB (offset 382) */
   "ii\0"
   "glMultiTexCoord1s\0"
   "glMultiTexCoord1sARB\0"
   "\0"
   /* _mesa_function_pool[26080]: ReplacementCodeuiColor3fVertex3fSUN (dynamic) */
   "iffffff\0"
   "glReplacementCodeuiColor3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[26127]: GetVertexAttribPointervNV (will be remapped) */
   "iip\0"
   "glGetVertexAttribPointerv\0"
   "glGetVertexAttribPointervARB\0"
   "glGetVertexAttribPointervNV\0"
   "\0"
   /* _mesa_function_pool[26215]: VertexAttribs1fvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs1fvNV\0"
   "\0"
   /* _mesa_function_pool[26241]: MultiTexCoord1dvARB (offset 377) */
   "ip\0"
   "glMultiTexCoord1dv\0"
   "glMultiTexCoord1dvARB\0"
   "\0"
   /* _mesa_function_pool[26286]: Uniform2iARB (will be remapped) */
   "iii\0"
   "glUniform2i\0"
   "glUniform2iARB\0"
   "\0"
   /* _mesa_function_pool[26318]: Vertex2iv (offset 131) */
   "p\0"
   "glVertex2iv\0"
   "\0"
   /* _mesa_function_pool[26333]: GetProgramStringNV (will be remapped) */
   "iip\0"
   "glGetProgramStringNV\0"
   "\0"
   /* _mesa_function_pool[26359]: ColorPointerEXT (will be remapped) */
   "iiiip\0"
   "glColorPointerEXT\0"
   "\0"
   /* _mesa_function_pool[26384]: LineWidth (offset 168) */
   "f\0"
   "glLineWidth\0"
   "\0"
   /* _mesa_function_pool[26399]: MapBufferARB (will be remapped) */
   "ii\0"
   "glMapBuffer\0"
   "glMapBufferARB\0"
   "\0"
   /* _mesa_function_pool[26430]: MultiDrawElementsBaseVertex (will be remapped) */
   "ipipip\0"
   "glMultiDrawElementsBaseVertex\0"
   "\0"
   /* _mesa_function_pool[26468]: TexParameterIuivEXT (will be remapped) */
   "iip\0"
   "glTexParameterIuivEXT\0"
   "glTexParameterIuiv\0"
   "\0"
   /* _mesa_function_pool[26514]: Binormal3svEXT (dynamic) */
   "p\0"
   "glBinormal3svEXT\0"
   "\0"
   /* _mesa_function_pool[26534]: ApplyTextureEXT (dynamic) */
   "i\0"
   "glApplyTextureEXT\0"
   "\0"
   /* _mesa_function_pool[26555]: GetBufferParameteri64v (will be remapped) */
   "iip\0"
   "glGetBufferParameteri64v\0"
   "\0"
   /* _mesa_function_pool[26585]: TexGendv (offset 189) */
   "iip\0"
   "glTexGendv\0"
   "\0"
   /* _mesa_function_pool[26601]: VertexAttribI3iEXT (will be remapped) */
   "iiii\0"
   "glVertexAttribI3iEXT\0"
   "glVertexAttribI3i\0"
   "\0"
   /* _mesa_function_pool[26646]: EnableIndexedEXT (will be remapped) */
   "ii\0"
   "glEnableIndexedEXT\0"
   "glEnablei\0"
   "\0"
   /* _mesa_function_pool[26679]: TextureMaterialEXT (dynamic) */
   "ii\0"
   "glTextureMaterialEXT\0"
   "\0"
   /* _mesa_function_pool[26704]: TextureLightEXT (dynamic) */
   "i\0"
   "glTextureLightEXT\0"
   "\0"
   /* _mesa_function_pool[26725]: ResetMinmax (offset 370) */
   "i\0"
   "glResetMinmax\0"
   "glResetMinmaxEXT\0"
   "\0"
   /* _mesa_function_pool[26759]: SpriteParameterfSGIX (dynamic) */
   "if\0"
   "glSpriteParameterfSGIX\0"
   "\0"
   /* _mesa_function_pool[26786]: EnableClientState (offset 313) */
   "i\0"
   "glEnableClientState\0"
   "\0"
   /* _mesa_function_pool[26809]: VertexAttrib4sNV (will be remapped) */
   "iiiii\0"
   "glVertexAttrib4sNV\0"
   "\0"
   /* _mesa_function_pool[26835]: GetConvolutionParameterfv (offset 357) */
   "iip\0"
   "glGetConvolutionParameterfv\0"
   "glGetConvolutionParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[26899]: VertexAttribs4dvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs4dvNV\0"
   "\0"
   /* _mesa_function_pool[26925]: VertexAttrib4dARB (will be remapped) */
   "idddd\0"
   "glVertexAttrib4d\0"
   "glVertexAttrib4dARB\0"
   "\0"
   /* _mesa_function_pool[26969]: GetTexBumpParameterfvATI (will be remapped) */
   "ip\0"
   "glGetTexBumpParameterfvATI\0"
   "\0"
   /* _mesa_function_pool[27000]: ProgramNamedParameter4dNV (will be remapped) */
   "iipdddd\0"
   "glProgramNamedParameter4dNV\0"
   "\0"
   /* _mesa_function_pool[27037]: GetMaterialfv (offset 269) */
   "iip\0"
   "glGetMaterialfv\0"
   "\0"
   /* _mesa_function_pool[27058]: VertexWeightfEXT (dynamic) */
   "f\0"
   "glVertexWeightfEXT\0"
   "\0"
   /* _mesa_function_pool[27080]: SetFragmentShaderConstantATI (will be remapped) */
   "ip\0"
   "glSetFragmentShaderConstantATI\0"
   "\0"
   /* _mesa_function_pool[27115]: Binormal3fEXT (dynamic) */
   "fff\0"
   "glBinormal3fEXT\0"
   "\0"
   /* _mesa_function_pool[27136]: CallList (offset 2) */
   "i\0"
   "glCallList\0"
   "\0"
   /* _mesa_function_pool[27150]: Materialfv (offset 170) */
   "iip\0"
   "glMaterialfv\0"
   "\0"
   /* _mesa_function_pool[27168]: TexCoord3fv (offset 113) */
   "p\0"
   "glTexCoord3fv\0"
   "\0"
   /* _mesa_function_pool[27185]: FogCoordfvEXT (will be remapped) */
   "p\0"
   "glFogCoordfv\0"
   "glFogCoordfvEXT\0"
   "\0"
   /* _mesa_function_pool[27217]: MultiTexCoord1ivARB (offset 381) */
   "ip\0"
   "glMultiTexCoord1iv\0"
   "glMultiTexCoord1ivARB\0"
   "\0"
   /* _mesa_function_pool[27262]: SecondaryColor3ubEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3ub\0"
   "glSecondaryColor3ubEXT\0"
   "\0"
   /* _mesa_function_pool[27310]: MultiTexCoord2ivARB (offset 389) */
   "ip\0"
   "glMultiTexCoord2iv\0"
   "glMultiTexCoord2ivARB\0"
   "\0"
   /* _mesa_function_pool[27355]: FogFuncSGIS (dynamic) */
   "ip\0"
   "glFogFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[27373]: CopyTexSubImage2D (offset 326) */
   "iiiiiiii\0"
   "glCopyTexSubImage2D\0"
   "glCopyTexSubImage2DEXT\0"
   "\0"
   /* _mesa_function_pool[27426]: GetObjectParameterivARB (will be remapped) */
   "iip\0"
   "glGetObjectParameterivARB\0"
   "\0"
   /* _mesa_function_pool[27457]: Color3iv (offset 16) */
   "p\0"
   "glColor3iv\0"
   "\0"
   /* _mesa_function_pool[27471]: TexCoord4fVertex4fSUN (dynamic) */
   "ffffffff\0"
   "glTexCoord4fVertex4fSUN\0"
   "\0"
   /* _mesa_function_pool[27505]: DrawElements (offset 311) */
   "iiip\0"
   "glDrawElements\0"
   "\0"
   /* _mesa_function_pool[27526]: BindVertexArrayAPPLE (will be remapped) */
   "i\0"
   "glBindVertexArrayAPPLE\0"
   "\0"
   /* _mesa_function_pool[27552]: GetProgramLocalParameterdvARB (will be remapped) */
   "iip\0"
   "glGetProgramLocalParameterdvARB\0"
   "\0"
   /* _mesa_function_pool[27589]: GetHistogramParameteriv (offset 363) */
   "iip\0"
   "glGetHistogramParameteriv\0"
   "glGetHistogramParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[27649]: MultiTexCoord1iARB (offset 380) */
   "ii\0"
   "glMultiTexCoord1i\0"
   "glMultiTexCoord1iARB\0"
   "\0"
   /* _mesa_function_pool[27692]: GetConvolutionFilter (offset 356) */
   "iiip\0"
   "glGetConvolutionFilter\0"
   "glGetConvolutionFilterEXT\0"
   "\0"
   /* _mesa_function_pool[27747]: GetProgramivARB (will be remapped) */
   "iip\0"
   "glGetProgramivARB\0"
   "\0"
   /* _mesa_function_pool[27770]: TexBufferARB (will be remapped) */
   "iii\0"
   "glTexBufferARB\0"
   "\0"
   /* _mesa_function_pool[27790]: BlendFuncSeparateEXT (will be remapped) */
   "iiii\0"
   "glBlendFuncSeparate\0"
   "glBlendFuncSeparateEXT\0"
   "glBlendFuncSeparateINGR\0"
   "\0"
   /* _mesa_function_pool[27863]: MapBufferRange (will be remapped) */
   "iiii\0"
   "glMapBufferRange\0"
   "\0"
   /* _mesa_function_pool[27886]: ProgramParameters4dvNV (will be remapped) */
   "iiip\0"
   "glProgramParameters4dvNV\0"
   "\0"
   /* _mesa_function_pool[27917]: TexCoord2fColor3fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glTexCoord2fColor3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[27954]: EvalPoint2 (offset 239) */
   "ii\0"
   "glEvalPoint2\0"
   "\0"
   /* _mesa_function_pool[27971]: Uniform1uivEXT (will be remapped) */
   "iip\0"
   "glUniform1uivEXT\0"
   "glUniform1uiv\0"
   "\0"
   /* _mesa_function_pool[28007]: EvalPoint1 (offset 237) */
   "i\0"
   "glEvalPoint1\0"
   "\0"
   /* _mesa_function_pool[28023]: Binormal3dvEXT (dynamic) */
   "p\0"
   "glBinormal3dvEXT\0"
   "\0"
   /* _mesa_function_pool[28043]: PopMatrix (offset 297) */
   "\0"
   "glPopMatrix\0"
   "\0"
   /* _mesa_function_pool[28057]: FinishFenceNV (will be remapped) */
   "i\0"
   "glFinishFenceNV\0"
   "\0"
   /* _mesa_function_pool[28076]: GetFogFuncSGIS (dynamic) */
   "p\0"
   "glGetFogFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[28096]: GetUniformLocationARB (will be remapped) */
   "ip\0"
   "glGetUniformLocation\0"
   "glGetUniformLocationARB\0"
   "\0"
   /* _mesa_function_pool[28145]: SecondaryColor3fEXT (will be remapped) */
   "fff\0"
   "glSecondaryColor3f\0"
   "glSecondaryColor3fEXT\0"
   "\0"
   /* _mesa_function_pool[28191]: GetTexGeniv (offset 280) */
   "iip\0"
   "glGetTexGeniv\0"
   "\0"
   /* _mesa_function_pool[28210]: CombinerInputNV (will be remapped) */
   "iiiiii\0"
   "glCombinerInputNV\0"
   "\0"
   /* _mesa_function_pool[28236]: VertexAttrib3sARB (will be remapped) */
   "iiii\0"
   "glVertexAttrib3s\0"
   "glVertexAttrib3sARB\0"
   "\0"
   /* _mesa_function_pool[28279]: IsTransformFeedback (will be remapped) */
   "i\0"
   "glIsTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[28304]: ReplacementCodeuiNormal3fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glReplacementCodeuiNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[28349]: Map2d (offset 222) */
   "iddiiddiip\0"
   "glMap2d\0"
   "\0"
   /* _mesa_function_pool[28369]: Map2f (offset 223) */
   "iffiiffiip\0"
   "glMap2f\0"
   "\0"
   /* _mesa_function_pool[28389]: ProgramStringARB (will be remapped) */
   "iiip\0"
   "glProgramStringARB\0"
   "\0"
   /* _mesa_function_pool[28414]: Vertex4s (offset 148) */
   "iiii\0"
   "glVertex4s\0"
   "\0"
   /* _mesa_function_pool[28431]: TexCoord4fVertex4fvSUN (dynamic) */
   "pp\0"
   "glTexCoord4fVertex4fvSUN\0"
   "\0"
   /* _mesa_function_pool[28460]: FragmentLightModelivSGIX (dynamic) */
   "ip\0"
   "glFragmentLightModelivSGIX\0"
   "\0"
   /* _mesa_function_pool[28491]: VertexAttrib1fNV (will be remapped) */
   "if\0"
   "glVertexAttrib1fNV\0"
   "\0"
   /* _mesa_function_pool[28514]: Vertex4f (offset 144) */
   "ffff\0"
   "glVertex4f\0"
   "\0"
   /* _mesa_function_pool[28531]: EvalCoord1d (offset 228) */
   "d\0"
   "glEvalCoord1d\0"
   "\0"
   /* _mesa_function_pool[28548]: Vertex4d (offset 142) */
   "dddd\0"
   "glVertex4d\0"
   "\0"
   /* _mesa_function_pool[28565]: RasterPos4dv (offset 79) */
   "p\0"
   "glRasterPos4dv\0"
   "\0"
   /* _mesa_function_pool[28583]: UseShaderProgramEXT (will be remapped) */
   "ii\0"
   "glUseShaderProgramEXT\0"
   "\0"
   /* _mesa_function_pool[28609]: FragmentLightfSGIX (dynamic) */
   "iif\0"
   "glFragmentLightfSGIX\0"
   "\0"
   /* _mesa_function_pool[28635]: GetCompressedTexImageARB (will be remapped) */
   "iip\0"
   "glGetCompressedTexImage\0"
   "glGetCompressedTexImageARB\0"
   "\0"
   /* _mesa_function_pool[28691]: GetTexGenfv (offset 279) */
   "iip\0"
   "glGetTexGenfv\0"
   "\0"
   /* _mesa_function_pool[28710]: Vertex4i (offset 146) */
   "iiii\0"
   "glVertex4i\0"
   "\0"
   /* _mesa_function_pool[28727]: VertexWeightPointerEXT (dynamic) */
   "iiip\0"
   "glVertexWeightPointerEXT\0"
   "\0"
   /* _mesa_function_pool[28758]: GetHistogram (offset 361) */
   "iiiip\0"
   "glGetHistogram\0"
   "glGetHistogramEXT\0"
   "\0"
   /* _mesa_function_pool[28798]: ActiveStencilFaceEXT (will be remapped) */
   "i\0"
   "glActiveStencilFaceEXT\0"
   "\0"
   /* _mesa_function_pool[28824]: StencilFuncSeparateATI (will be remapped) */
   "iiii\0"
   "glStencilFuncSeparateATI\0"
   "\0"
   /* _mesa_function_pool[28855]: Materialf (offset 169) */
   "iif\0"
   "glMaterialf\0"
   "\0"
   /* _mesa_function_pool[28872]: GetShaderSourceARB (will be remapped) */
   "iipp\0"
   "glGetShaderSource\0"
   "glGetShaderSourceARB\0"
   "\0"
   /* _mesa_function_pool[28917]: IglooInterfaceSGIX (dynamic) */
   "ip\0"
   "glIglooInterfaceSGIX\0"
   "\0"
   /* _mesa_function_pool[28942]: Materiali (offset 171) */
   "iii\0"
   "glMateriali\0"
   "\0"
   /* _mesa_function_pool[28959]: VertexAttrib4dNV (will be remapped) */
   "idddd\0"
   "glVertexAttrib4dNV\0"
   "\0"
   /* _mesa_function_pool[28985]: MultiModeDrawElementsIBM (will be remapped) */
   "ppipii\0"
   "glMultiModeDrawElementsIBM\0"
   "\0"
   /* _mesa_function_pool[29020]: Indexsv (offset 51) */
   "p\0"
   "glIndexsv\0"
   "\0"
   /* _mesa_function_pool[29033]: MultiTexCoord4svARB (offset 407) */
   "ip\0"
   "glMultiTexCoord4sv\0"
   "glMultiTexCoord4svARB\0"
   "\0"
   /* _mesa_function_pool[29078]: LightModelfv (offset 164) */
   "ip\0"
   "glLightModelfv\0"
   "\0"
   /* _mesa_function_pool[29097]: TexCoord2dv (offset 103) */
   "p\0"
   "glTexCoord2dv\0"
   "\0"
   /* _mesa_function_pool[29114]: GenQueriesARB (will be remapped) */
   "ip\0"
   "glGenQueries\0"
   "glGenQueriesARB\0"
   "\0"
   /* _mesa_function_pool[29147]: EvalCoord1dv (offset 229) */
   "p\0"
   "glEvalCoord1dv\0"
   "\0"
   /* _mesa_function_pool[29165]: ReplacementCodeuiVertex3fSUN (dynamic) */
   "ifff\0"
   "glReplacementCodeuiVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[29202]: Translated (offset 303) */
   "ddd\0"
   "glTranslated\0"
   "\0"
   /* _mesa_function_pool[29220]: Translatef (offset 304) */
   "fff\0"
   "glTranslatef\0"
   "\0"
   /* _mesa_function_pool[29238]: Uniform3uiEXT (will be remapped) */
   "iiii\0"
   "glUniform3uiEXT\0"
   "glUniform3ui\0"
   "\0"
   /* _mesa_function_pool[29273]: StencilMask (offset 209) */
   "i\0"
   "glStencilMask\0"
   "\0"
   /* _mesa_function_pool[29290]: Tangent3iEXT (dynamic) */
   "iii\0"
   "glTangent3iEXT\0"
   "\0"
   /* _mesa_function_pool[29310]: ClampColorARB (will be remapped) */
   "ii\0"
   "glClampColorARB\0"
   "\0"
   /* _mesa_function_pool[29330]: GetLightiv (offset 265) */
   "iip\0"
   "glGetLightiv\0"
   "\0"
   /* _mesa_function_pool[29348]: DrawMeshArraysSUN (dynamic) */
   "iiii\0"
   "glDrawMeshArraysSUN\0"
   "\0"
   /* _mesa_function_pool[29374]: IsList (offset 287) */
   "i\0"
   "glIsList\0"
   "\0"
   /* _mesa_function_pool[29386]: IsSync (will be remapped) */
   "i\0"
   "glIsSync\0"
   "\0"
   /* _mesa_function_pool[29398]: RenderMode (offset 196) */
   "i\0"
   "glRenderMode\0"
   "\0"
   /* _mesa_function_pool[29414]: GetMapControlPointsNV (dynamic) */
   "iiiiiip\0"
   "glGetMapControlPointsNV\0"
   "\0"
   /* _mesa_function_pool[29447]: DrawBuffersARB (will be remapped) */
   "ip\0"
   "glDrawBuffers\0"
   "glDrawBuffersARB\0"
   "glDrawBuffersATI\0"
   "\0"
   /* _mesa_function_pool[29499]: ClearBufferiv (will be remapped) */
   "iip\0"
   "glClearBufferiv\0"
   "\0"
   /* _mesa_function_pool[29520]: ProgramLocalParameter4fARB (will be remapped) */
   "iiffff\0"
   "glProgramLocalParameter4fARB\0"
   "\0"
   /* _mesa_function_pool[29557]: SpriteParameterivSGIX (dynamic) */
   "ip\0"
   "glSpriteParameterivSGIX\0"
   "\0"
   /* _mesa_function_pool[29585]: ProvokingVertexEXT (will be remapped) */
   "i\0"
   "glProvokingVertexEXT\0"
   "glProvokingVertex\0"
   "\0"
   /* _mesa_function_pool[29627]: MultiTexCoord1fARB (offset 378) */
   "if\0"
   "glMultiTexCoord1f\0"
   "glMultiTexCoord1fARB\0"
   "\0"
   /* _mesa_function_pool[29670]: LoadName (offset 198) */
   "i\0"
   "glLoadName\0"
   "\0"
   /* _mesa_function_pool[29684]: VertexAttribs4ubvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs4ubvNV\0"
   "\0"
   /* _mesa_function_pool[29711]: WeightsvARB (dynamic) */
   "ip\0"
   "glWeightsvARB\0"
   "\0"
   /* _mesa_function_pool[29729]: Uniform1fvARB (will be remapped) */
   "iip\0"
   "glUniform1fv\0"
   "glUniform1fvARB\0"
   "\0"
   /* _mesa_function_pool[29763]: CopyTexSubImage1D (offset 325) */
   "iiiiii\0"
   "glCopyTexSubImage1D\0"
   "glCopyTexSubImage1DEXT\0"
   "\0"
   /* _mesa_function_pool[29814]: CullFace (offset 152) */
   "i\0"
   "glCullFace\0"
   "\0"
   /* _mesa_function_pool[29828]: BindTexture (offset 307) */
   "ii\0"
   "glBindTexture\0"
   "glBindTextureEXT\0"
   "\0"
   /* _mesa_function_pool[29863]: BeginFragmentShaderATI (will be remapped) */
   "\0"
   "glBeginFragmentShaderATI\0"
   "\0"
   /* _mesa_function_pool[29890]: MultiTexCoord4fARB (offset 402) */
   "iffff\0"
   "glMultiTexCoord4f\0"
   "glMultiTexCoord4fARB\0"
   "\0"
   /* _mesa_function_pool[29936]: VertexAttribs3svNV (will be remapped) */
   "iip\0"
   "glVertexAttribs3svNV\0"
   "\0"
   /* _mesa_function_pool[29962]: StencilFunc (offset 243) */
   "iii\0"
   "glStencilFunc\0"
   "\0"
   /* _mesa_function_pool[29981]: CopyPixels (offset 255) */
   "iiiii\0"
   "glCopyPixels\0"
   "\0"
   /* _mesa_function_pool[30001]: Rectsv (offset 93) */
   "pp\0"
   "glRectsv\0"
   "\0"
   /* _mesa_function_pool[30014]: ReplacementCodeuivSUN (dynamic) */
   "p\0"
   "glReplacementCodeuivSUN\0"
   "\0"
   /* _mesa_function_pool[30041]: EnableVertexAttribArrayARB (will be remapped) */
   "i\0"
   "glEnableVertexAttribArray\0"
   "glEnableVertexAttribArrayARB\0"
   "\0"
   /* _mesa_function_pool[30099]: NormalPointervINTEL (dynamic) */
   "ip\0"
   "glNormalPointervINTEL\0"
   "\0"
   /* _mesa_function_pool[30125]: CopyConvolutionFilter2D (offset 355) */
   "iiiiii\0"
   "glCopyConvolutionFilter2D\0"
   "glCopyConvolutionFilter2DEXT\0"
   "\0"
   /* _mesa_function_pool[30188]: WindowPos3ivMESA (will be remapped) */
   "p\0"
   "glWindowPos3iv\0"
   "glWindowPos3ivARB\0"
   "glWindowPos3ivMESA\0"
   "\0"
   /* _mesa_function_pool[30243]: CopyBufferSubData (will be remapped) */
   "iiiii\0"
   "glCopyBufferSubData\0"
   "\0"
   /* _mesa_function_pool[30270]: NormalPointer (offset 318) */
   "iip\0"
   "glNormalPointer\0"
   "\0"
   /* _mesa_function_pool[30291]: TexParameterfv (offset 179) */
   "iip\0"
   "glTexParameterfv\0"
   "\0"
   /* _mesa_function_pool[30313]: IsBufferARB (will be remapped) */
   "i\0"
   "glIsBuffer\0"
   "glIsBufferARB\0"
   "\0"
   /* _mesa_function_pool[30341]: WindowPos4iMESA (will be remapped) */
   "iiii\0"
   "glWindowPos4iMESA\0"
   "\0"
   /* _mesa_function_pool[30365]: VertexAttrib4uivARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4uiv\0"
   "glVertexAttrib4uivARB\0"
   "\0"
   /* _mesa_function_pool[30410]: Tangent3bvEXT (dynamic) */
   "p\0"
   "glTangent3bvEXT\0"
   "\0"
   /* _mesa_function_pool[30429]: VertexAttribI3uivEXT (will be remapped) */
   "ip\0"
   "glVertexAttribI3uivEXT\0"
   "glVertexAttribI3uiv\0"
   "\0"
   /* _mesa_function_pool[30476]: UniformMatrix3x4fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix3x4fv\0"
   "\0"
   /* _mesa_function_pool[30503]: ClipPlane (offset 150) */
   "ip\0"
   "glClipPlane\0"
   "\0"
   /* _mesa_function_pool[30519]: Recti (offset 90) */
   "iiii\0"
   "glRecti\0"
   "\0"
   /* _mesa_function_pool[30533]: VertexAttribI3ivEXT (will be remapped) */
   "ip\0"
   "glVertexAttribI3ivEXT\0"
   "glVertexAttribI3iv\0"
   "\0"
   /* _mesa_function_pool[30578]: DrawRangeElementsBaseVertex (will be remapped) */
   "iiiiipi\0"
   "glDrawRangeElementsBaseVertex\0"
   "\0"
   /* _mesa_function_pool[30617]: TexCoordPointervINTEL (dynamic) */
   "iip\0"
   "glTexCoordPointervINTEL\0"
   "\0"
   /* _mesa_function_pool[30646]: DeleteBuffersARB (will be remapped) */
   "ip\0"
   "glDeleteBuffers\0"
   "glDeleteBuffersARB\0"
   "\0"
   /* _mesa_function_pool[30685]: PixelTransformParameterfvEXT (dynamic) */
   "iip\0"
   "glPixelTransformParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[30721]: PrimitiveRestartNV (will be remapped) */
   "\0"
   "glPrimitiveRestartNV\0"
   "\0"
   /* _mesa_function_pool[30744]: WindowPos4fvMESA (will be remapped) */
   "p\0"
   "glWindowPos4fvMESA\0"
   "\0"
   /* _mesa_function_pool[30766]: GetPixelMapuiv (offset 272) */
   "ip\0"
   "glGetPixelMapuiv\0"
   "\0"
   /* _mesa_function_pool[30787]: Rectf (offset 88) */
   "ffff\0"
   "glRectf\0"
   "\0"
   /* _mesa_function_pool[30801]: VertexAttrib1sNV (will be remapped) */
   "ii\0"
   "glVertexAttrib1sNV\0"
   "\0"
   /* _mesa_function_pool[30824]: Indexfv (offset 47) */
   "p\0"
   "glIndexfv\0"
   "\0"
   /* _mesa_function_pool[30837]: SecondaryColor3svEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3sv\0"
   "glSecondaryColor3svEXT\0"
   "\0"
   /* _mesa_function_pool[30883]: LoadTransposeMatrixfARB (will be remapped) */
   "p\0"
   "glLoadTransposeMatrixf\0"
   "glLoadTransposeMatrixfARB\0"
   "\0"
   /* _mesa_function_pool[30935]: GetPointerv (offset 329) */
   "ip\0"
   "glGetPointerv\0"
   "glGetPointervEXT\0"
   "\0"
   /* _mesa_function_pool[30970]: Tangent3bEXT (dynamic) */
   "iii\0"
   "glTangent3bEXT\0"
   "\0"
   /* _mesa_function_pool[30990]: CombinerParameterfNV (will be remapped) */
   "if\0"
   "glCombinerParameterfNV\0"
   "\0"
   /* _mesa_function_pool[31017]: IndexMask (offset 212) */
   "i\0"
   "glIndexMask\0"
   "\0"
   /* _mesa_function_pool[31032]: BindProgramNV (will be remapped) */
   "ii\0"
   "glBindProgramARB\0"
   "glBindProgramNV\0"
   "\0"
   /* _mesa_function_pool[31069]: VertexAttrib4svARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4sv\0"
   "glVertexAttrib4svARB\0"
   "\0"
   /* _mesa_function_pool[31112]: GetFloatv (offset 262) */
   "ip\0"
   "glGetFloatv\0"
   "\0"
   /* _mesa_function_pool[31128]: CreateDebugObjectMESA (dynamic) */
   "\0"
   "glCreateDebugObjectMESA\0"
   "\0"
   /* _mesa_function_pool[31154]: GetShaderiv (will be remapped) */
   "iip\0"
   "glGetShaderiv\0"
   "\0"
   /* _mesa_function_pool[31173]: ClientWaitSync (will be remapped) */
   "iii\0"
   "glClientWaitSync\0"
   "\0"
   /* _mesa_function_pool[31195]: TexCoord4s (offset 124) */
   "iiii\0"
   "glTexCoord4s\0"
   "\0"
   /* _mesa_function_pool[31214]: TexCoord3sv (offset 117) */
   "p\0"
   "glTexCoord3sv\0"
   "\0"
   /* _mesa_function_pool[31231]: BindFragmentShaderATI (will be remapped) */
   "i\0"
   "glBindFragmentShaderATI\0"
   "\0"
   /* _mesa_function_pool[31258]: PopAttrib (offset 218) */
   "\0"
   "glPopAttrib\0"
   "\0"
   /* _mesa_function_pool[31272]: Fogfv (offset 154) */
   "ip\0"
   "glFogfv\0"
   "\0"
   /* _mesa_function_pool[31284]: UnmapBufferARB (will be remapped) */
   "i\0"
   "glUnmapBuffer\0"
   "glUnmapBufferARB\0"
   "\0"
   /* _mesa_function_pool[31318]: InitNames (offset 197) */
   "\0"
   "glInitNames\0"
   "\0"
   /* _mesa_function_pool[31332]: Normal3sv (offset 61) */
   "p\0"
   "glNormal3sv\0"
   "\0"
   /* _mesa_function_pool[31347]: Minmax (offset 368) */
   "iii\0"
   "glMinmax\0"
   "glMinmaxEXT\0"
   "\0"
   /* _mesa_function_pool[31373]: TexCoord4d (offset 118) */
   "dddd\0"
   "glTexCoord4d\0"
   "\0"
   /* _mesa_function_pool[31392]: DeformationMap3dSGIX (dynamic) */
   "iddiiddiiddiip\0"
   "glDeformationMap3dSGIX\0"
   "\0"
   /* _mesa_function_pool[31431]: TexCoord4f (offset 120) */
   "ffff\0"
   "glTexCoord4f\0"
   "\0"
   /* _mesa_function_pool[31450]: FogCoorddvEXT (will be remapped) */
   "p\0"
   "glFogCoorddv\0"
   "glFogCoorddvEXT\0"
   "\0"
   /* _mesa_function_pool[31482]: FinishTextureSUNX (dynamic) */
   "\0"
   "glFinishTextureSUNX\0"
   "\0"
   /* _mesa_function_pool[31504]: GetFragmentLightfvSGIX (dynamic) */
   "iip\0"
   "glGetFragmentLightfvSGIX\0"
   "\0"
   /* _mesa_function_pool[31534]: Binormal3fvEXT (dynamic) */
   "p\0"
   "glBinormal3fvEXT\0"
   "\0"
   /* _mesa_function_pool[31554]: GetBooleanv (offset 258) */
   "ip\0"
   "glGetBooleanv\0"
   "\0"
   /* _mesa_function_pool[31572]: ColorFragmentOp3ATI (will be remapped) */
   "iiiiiiiiiiiii\0"
   "glColorFragmentOp3ATI\0"
   "\0"
   /* _mesa_function_pool[31609]: Hint (offset 158) */
   "ii\0"
   "glHint\0"
   "\0"
   /* _mesa_function_pool[31620]: Color4dv (offset 28) */
   "p\0"
   "glColor4dv\0"
   "\0"
   /* _mesa_function_pool[31634]: VertexAttrib2svARB (will be remapped) */
   "ip\0"
   "glVertexAttrib2sv\0"
   "glVertexAttrib2svARB\0"
   "\0"
   /* _mesa_function_pool[31677]: AreProgramsResidentNV (will be remapped) */
   "ipp\0"
   "glAreProgramsResidentNV\0"
   "\0"
   /* _mesa_function_pool[31706]: WindowPos3svMESA (will be remapped) */
   "p\0"
   "glWindowPos3sv\0"
   "glWindowPos3svARB\0"
   "glWindowPos3svMESA\0"
   "\0"
   /* _mesa_function_pool[31761]: CopyColorSubTable (offset 347) */
   "iiiii\0"
   "glCopyColorSubTable\0"
   "glCopyColorSubTableEXT\0"
   "\0"
   /* _mesa_function_pool[31811]: WeightdvARB (dynamic) */
   "ip\0"
   "glWeightdvARB\0"
   "\0"
   /* _mesa_function_pool[31829]: DeleteRenderbuffersEXT (will be remapped) */
   "ip\0"
   "glDeleteRenderbuffers\0"
   "glDeleteRenderbuffersEXT\0"
   "\0"
   /* _mesa_function_pool[31880]: VertexAttrib4NubvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Nubv\0"
   "glVertexAttrib4NubvARB\0"
   "\0"
   /* _mesa_function_pool[31927]: VertexAttrib3dvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib3dvNV\0"
   "\0"
   /* _mesa_function_pool[31951]: GetObjectParameterfvARB (will be remapped) */
   "iip\0"
   "glGetObjectParameterfvARB\0"
   "\0"
   /* _mesa_function_pool[31982]: Vertex4iv (offset 147) */
   "p\0"
   "glVertex4iv\0"
   "\0"
   /* _mesa_function_pool[31997]: GetProgramEnvParameterdvARB (will be remapped) */
   "iip\0"
   "glGetProgramEnvParameterdvARB\0"
   "\0"
   /* _mesa_function_pool[32032]: TexCoord4dv (offset 119) */
   "p\0"
   "glTexCoord4dv\0"
   "\0"
   /* _mesa_function_pool[32049]: LockArraysEXT (will be remapped) */
   "ii\0"
   "glLockArraysEXT\0"
   "\0"
   /* _mesa_function_pool[32069]: Begin (offset 7) */
   "i\0"
   "glBegin\0"
   "\0"
   /* _mesa_function_pool[32080]: LightModeli (offset 165) */
   "ii\0"
   "glLightModeli\0"
   "\0"
   /* _mesa_function_pool[32098]: VertexAttribI4ivEXT (will be remapped) */
   "ip\0"
   "glVertexAttribI4ivEXT\0"
   "glVertexAttribI4iv\0"
   "\0"
   /* _mesa_function_pool[32143]: Rectfv (offset 89) */
   "pp\0"
   "glRectfv\0"
   "\0"
   /* _mesa_function_pool[32156]: LightModelf (offset 163) */
   "if\0"
   "glLightModelf\0"
   "\0"
   /* _mesa_function_pool[32174]: GetTexParameterfv (offset 282) */
   "iip\0"
   "glGetTexParameterfv\0"
   "\0"
   /* _mesa_function_pool[32199]: GetLightfv (offset 264) */
   "iip\0"
   "glGetLightfv\0"
   "\0"
   /* _mesa_function_pool[32217]: PixelTransformParameterivEXT (dynamic) */
   "iip\0"
   "glPixelTransformParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[32253]: BinormalPointerEXT (dynamic) */
   "iip\0"
   "glBinormalPointerEXT\0"
   "\0"
   /* _mesa_function_pool[32279]: VertexAttrib1dNV (will be remapped) */
   "id\0"
   "glVertexAttrib1dNV\0"
   "\0"
   /* _mesa_function_pool[32302]: GetCombinerInputParameterivNV (will be remapped) */
   "iiiip\0"
   "glGetCombinerInputParameterivNV\0"
   "\0"
   /* _mesa_function_pool[32341]: Disable (offset 214) */
   "i\0"
   "glDisable\0"
   "\0"
   /* _mesa_function_pool[32354]: MultiTexCoord2fvARB (offset 387) */
   "ip\0"
   "glMultiTexCoord2fv\0"
   "glMultiTexCoord2fvARB\0"
   "\0"
   /* _mesa_function_pool[32399]: GetRenderbufferParameterivEXT (will be remapped) */
   "iip\0"
   "glGetRenderbufferParameteriv\0"
   "glGetRenderbufferParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[32465]: CombinerParameterivNV (will be remapped) */
   "ip\0"
   "glCombinerParameterivNV\0"
   "\0"
   /* _mesa_function_pool[32493]: GenFragmentShadersATI (will be remapped) */
   "i\0"
   "glGenFragmentShadersATI\0"
   "\0"
   /* _mesa_function_pool[32520]: DrawArrays (offset 310) */
   "iii\0"
   "glDrawArrays\0"
   "glDrawArraysEXT\0"
   "\0"
   /* _mesa_function_pool[32554]: WeightuivARB (dynamic) */
   "ip\0"
   "glWeightuivARB\0"
   "\0"
   /* _mesa_function_pool[32573]: VertexAttrib2sARB (will be remapped) */
   "iii\0"
   "glVertexAttrib2s\0"
   "glVertexAttrib2sARB\0"
   "\0"
   /* _mesa_function_pool[32615]: ColorMask (offset 210) */
   "iiii\0"
   "glColorMask\0"
   "\0"
   /* _mesa_function_pool[32633]: GenAsyncMarkersSGIX (dynamic) */
   "i\0"
   "glGenAsyncMarkersSGIX\0"
   "\0"
   /* _mesa_function_pool[32658]: Tangent3svEXT (dynamic) */
   "p\0"
   "glTangent3svEXT\0"
   "\0"
   /* _mesa_function_pool[32677]: GetListParameterivSGIX (dynamic) */
   "iip\0"
   "glGetListParameterivSGIX\0"
   "\0"
   /* _mesa_function_pool[32707]: BindBufferARB (will be remapped) */
   "ii\0"
   "glBindBuffer\0"
   "glBindBufferARB\0"
   "\0"
   /* _mesa_function_pool[32740]: GetInfoLogARB (will be remapped) */
   "iipp\0"
   "glGetInfoLogARB\0"
   "\0"
   /* _mesa_function_pool[32762]: RasterPos4iv (offset 83) */
   "p\0"
   "glRasterPos4iv\0"
   "\0"
   /* _mesa_function_pool[32780]: Enable (offset 215) */
   "i\0"
   "glEnable\0"
   "\0"
   /* _mesa_function_pool[32792]: LineStipple (offset 167) */
   "ii\0"
   "glLineStipple\0"
   "\0"
   /* _mesa_function_pool[32810]: VertexAttribs4svNV (will be remapped) */
   "iip\0"
   "glVertexAttribs4svNV\0"
   "\0"
   /* _mesa_function_pool[32836]: EdgeFlagPointerListIBM (dynamic) */
   "ipi\0"
   "glEdgeFlagPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[32866]: UniformMatrix3x2fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix3x2fv\0"
   "\0"
   /* _mesa_function_pool[32893]: GetMinmaxParameterfv (offset 365) */
   "iip\0"
   "glGetMinmaxParameterfv\0"
   "glGetMinmaxParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[32947]: VertexAttrib1fvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib1fv\0"
   "glVertexAttrib1fvARB\0"
   "\0"
   /* _mesa_function_pool[32990]: GenBuffersARB (will be remapped) */
   "ip\0"
   "glGenBuffers\0"
   "glGenBuffersARB\0"
   "\0"
   /* _mesa_function_pool[33023]: VertexAttribs1svNV (will be remapped) */
   "iip\0"
   "glVertexAttribs1svNV\0"
   "\0"
   /* _mesa_function_pool[33049]: Vertex3fv (offset 137) */
   "p\0"
   "glVertex3fv\0"
   "\0"
   /* _mesa_function_pool[33064]: GetTexBumpParameterivATI (will be remapped) */
   "ip\0"
   "glGetTexBumpParameterivATI\0"
   "\0"
   /* _mesa_function_pool[33095]: Binormal3bEXT (dynamic) */
   "iii\0"
   "glBinormal3bEXT\0"
   "\0"
   /* _mesa_function_pool[33116]: FragmentMaterialivSGIX (dynamic) */
   "iip\0"
   "glFragmentMaterialivSGIX\0"
   "\0"
   /* _mesa_function_pool[33146]: IsRenderbufferEXT (will be remapped) */
   "i\0"
   "glIsRenderbuffer\0"
   "glIsRenderbufferEXT\0"
   "\0"
   /* _mesa_function_pool[33186]: GenProgramsNV (will be remapped) */
   "ip\0"
   "glGenProgramsARB\0"
   "glGenProgramsNV\0"
   "\0"
   /* _mesa_function_pool[33223]: VertexAttrib4dvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib4dvNV\0"
   "\0"
   /* _mesa_function_pool[33247]: EndFragmentShaderATI (will be remapped) */
   "\0"
   "glEndFragmentShaderATI\0"
   "\0"
   /* _mesa_function_pool[33272]: Binormal3iEXT (dynamic) */
   "iii\0"
   "glBinormal3iEXT\0"
   "\0"
   /* _mesa_function_pool[33293]: WindowPos2fMESA (will be remapped) */
   "ff\0"
   "glWindowPos2f\0"
   "glWindowPos2fARB\0"
   "glWindowPos2fMESA\0"
   "\0"
   ;

/* these functions need to be remapped */
static const struct gl_function_pool_remap MESA_remap_table_functions[] = {
   {  1577, AttachShader_remap_index },
   {  9906, CreateProgram_remap_index },
   { 23042, CreateShader_remap_index },
   { 25529, DeleteProgram_remap_index },
   { 18686, DeleteShader_remap_index },
   { 23543, DetachShader_remap_index },
   { 18052, GetAttachedShaders_remap_index },
   {  4869, GetProgramInfoLog_remap_index },
   {   405, GetProgramiv_remap_index },
   {  6542, GetShaderInfoLog_remap_index },
   { 31154, GetShaderiv_remap_index },
   { 13361, IsProgram_remap_index },
   { 12282, IsShader_remap_index },
   { 10036, StencilFuncSeparate_remap_index },
   {  3921, StencilMaskSeparate_remap_index },
   {  7607, StencilOpSeparate_remap_index },
   { 22330, UniformMatrix2x3fv_remap_index },
   {  2847, UniformMatrix2x4fv_remap_index },
   { 32866, UniformMatrix3x2fv_remap_index },
   { 30476, UniformMatrix3x4fv_remap_index },
   { 16309, UniformMatrix4x2fv_remap_index },
   {  3263, UniformMatrix4x3fv_remap_index },
   {  5030, ClampColor_remap_index },
   { 18106, ClearBufferfi_remap_index },
   { 17548, ClearBufferfv_remap_index },
   { 29499, ClearBufferiv_remap_index },
   { 13566, ClearBufferuiv_remap_index },
   { 19969, GetStringi_remap_index },
   {  2788, TexBuffer_remap_index },
   {   938, FramebufferTexture_remap_index },
   { 26555, GetBufferParameteri64v_remap_index },
   { 10136, GetInteger64i_v_remap_index },
   { 23356, VertexAttribDivisor_remap_index },
   {  9924, LoadTransposeMatrixdARB_remap_index },
   { 30883, LoadTransposeMatrixfARB_remap_index },
   {  5608, MultTransposeMatrixdARB_remap_index },
   { 23730, MultTransposeMatrixfARB_remap_index },
   {   216, SampleCoverageARB_remap_index },
   {  5834, CompressedTexImage1DARB_remap_index },
   { 24258, CompressedTexImage2DARB_remap_index },
   {  3984, CompressedTexImage3DARB_remap_index },
   { 18403, CompressedTexSubImage1DARB_remap_index },
   {  2050, CompressedTexSubImage2DARB_remap_index },
   { 20391, CompressedTexSubImage3DARB_remap_index },
   { 28635, GetCompressedTexImageARB_remap_index },
   {  3829, DisableVertexAttribArrayARB_remap_index },
   { 30041, EnableVertexAttribArrayARB_remap_index },
   { 31997, GetProgramEnvParameterdvARB_remap_index },
   { 23610, GetProgramEnvParameterfvARB_remap_index },
   { 27552, GetProgramLocalParameterdvARB_remap_index },
   {  8049, GetProgramLocalParameterfvARB_remap_index },
   { 18537, GetProgramStringARB_remap_index },
   { 27747, GetProgramivARB_remap_index },
   { 20586, GetVertexAttribdvARB_remap_index },
   { 16117, GetVertexAttribfvARB_remap_index },
   {  9748, GetVertexAttribivARB_remap_index },
   { 19450, ProgramEnvParameter4dARB_remap_index },
   { 25279, ProgramEnvParameter4dvARB_remap_index },
   { 16870, ProgramEnvParameter4fARB_remap_index },
   {  8948, ProgramEnvParameter4fvARB_remap_index },
   {  3947, ProgramLocalParameter4dARB_remap_index },
   { 13071, ProgramLocalParameter4dvARB_remap_index },
   { 29520, ProgramLocalParameter4fARB_remap_index },
   { 25915, ProgramLocalParameter4fvARB_remap_index },
   { 28389, ProgramStringARB_remap_index },
   { 19700, VertexAttrib1dARB_remap_index },
   { 15771, VertexAttrib1dvARB_remap_index },
   {  4122, VertexAttrib1fARB_remap_index },
   { 32947, VertexAttrib1fvARB_remap_index },
   {  7133, VertexAttrib1sARB_remap_index },
   {  2224, VertexAttrib1svARB_remap_index },
   { 15202, VertexAttrib2dARB_remap_index },
   { 17569, VertexAttrib2dvARB_remap_index },
   {  1596, VertexAttrib2fARB_remap_index },
   { 17682, VertexAttrib2fvARB_remap_index },
   { 32573, VertexAttrib2sARB_remap_index },
   { 31634, VertexAttrib2svARB_remap_index },
   { 11287, VertexAttrib3dARB_remap_index },
   {  8615, VertexAttrib3dvARB_remap_index },
   {  1683, VertexAttrib3fARB_remap_index },
   { 22593, VertexAttrib3fvARB_remap_index },
   { 28236, VertexAttrib3sARB_remap_index },
   { 20328, VertexAttrib3svARB_remap_index },
   {  4895, VertexAttrib4NbvARB_remap_index },
   { 17929, VertexAttrib4NivARB_remap_index },
   { 22548, VertexAttrib4NsvARB_remap_index },
   { 23562, VertexAttrib4NubARB_remap_index },
   { 31880, VertexAttrib4NubvARB_remap_index },
   { 19111, VertexAttrib4NuivARB_remap_index },
   {  3136, VertexAttrib4NusvARB_remap_index },
   { 10876, VertexAttrib4bvARB_remap_index },
   { 26925, VertexAttrib4dARB_remap_index },
   { 21350, VertexAttrib4dvARB_remap_index },
   { 11441, VertexAttrib4fARB_remap_index },
   { 11845, VertexAttrib4fvARB_remap_index },
   { 10252, VertexAttrib4ivARB_remap_index },
   { 17362, VertexAttrib4sARB_remap_index },
   { 31069, VertexAttrib4svARB_remap_index },
   { 16675, VertexAttrib4ubvARB_remap_index },
   { 30365, VertexAttrib4uivARB_remap_index },
   { 20139, VertexAttrib4usvARB_remap_index },
   { 22145, VertexAttribPointerARB_remap_index },
   { 32707, BindBufferARB_remap_index },
   {  6840, BufferDataARB_remap_index },
   {  1498, BufferSubDataARB_remap_index },
   { 30646, DeleteBuffersARB_remap_index },
   { 32990, GenBuffersARB_remap_index },
   { 17725, GetBufferParameterivARB_remap_index },
   { 16822, GetBufferPointervARB_remap_index },
   {  1451, GetBufferSubDataARB_remap_index },
   { 30313, IsBufferARB_remap_index },
   { 26399, MapBufferARB_remap_index },
   { 31284, UnmapBufferARB_remap_index },
   {   312, BeginQueryARB_remap_index },
   { 19795, DeleteQueriesARB_remap_index },
   { 12172, EndQueryARB_remap_index },
   { 29114, GenQueriesARB_remap_index },
   {  1942, GetQueryObjectivARB_remap_index },
   { 17406, GetQueryObjectuivARB_remap_index },
   {  1740, GetQueryivARB_remap_index },
   { 20046, IsQueryARB_remap_index },
   {  8225, AttachObjectARB_remap_index },
   { 18648, CompileShaderARB_remap_index },
   {  3205, CreateProgramObjectARB_remap_index },
   {  6785, CreateShaderObjectARB_remap_index },
   { 14504, DeleteObjectARB_remap_index },
   { 24049, DetachObjectARB_remap_index },
   { 11917, GetActiveUniformARB_remap_index },
   {  9423, GetAttachedObjectsARB_remap_index },
   {  9730, GetHandleARB_remap_index },
   { 32740, GetInfoLogARB_remap_index },
   { 31951, GetObjectParameterfvARB_remap_index },
   { 27426, GetObjectParameterivARB_remap_index },
   { 28872, GetShaderSourceARB_remap_index },
   { 28096, GetUniformLocationARB_remap_index },
   { 23832, GetUniformfvARB_remap_index },
   { 12646, GetUniformivARB_remap_index },
   { 20184, LinkProgramARB_remap_index },
   { 20242, ShaderSourceARB_remap_index },
   {  7507, Uniform1fARB_remap_index },
   { 29729, Uniform1fvARB_remap_index },
   { 22114, Uniform1iARB_remap_index },
   { 21039, Uniform1ivARB_remap_index },
   {  2173, Uniform2fARB_remap_index },
   { 14340, Uniform2fvARB_remap_index },
   { 26286, Uniform2iARB_remap_index },
   {  2293, Uniform2ivARB_remap_index },
   { 18758, Uniform3fARB_remap_index },
   {  9453, Uniform3fvARB_remap_index },
   {  6396, Uniform3iARB_remap_index },
   { 16928, Uniform3ivARB_remap_index },
   { 19256, Uniform4fARB_remap_index },
   { 23696, Uniform4fvARB_remap_index },
   { 24958, Uniform4iARB_remap_index },
   { 20552, Uniform4ivARB_remap_index },
   {  8277, UniformMatrix2fvARB_remap_index },
   {    17, UniformMatrix3fvARB_remap_index },
   {  2690, UniformMatrix4fvARB_remap_index },
   { 25391, UseProgramObjectARB_remap_index },
   { 14890, ValidateProgramARB_remap_index },
   { 21393, BindAttribLocationARB_remap_index },
   {  4940, GetActiveAttribARB_remap_index },
   { 16576, GetAttribLocationARB_remap_index },
   { 29447, DrawBuffersARB_remap_index },
   { 29310, ClampColorARB_remap_index },
   { 17974, DrawArraysInstancedARB_remap_index },
   {  6457, DrawElementsInstancedARB_remap_index },
   { 13176, RenderbufferStorageMultisample_remap_index },
   { 13647, FramebufferTextureARB_remap_index },
   { 25817, FramebufferTextureFaceARB_remap_index },
   { 24198, ProgramParameteriARB_remap_index },
   { 23514, VertexAttribDivisorARB_remap_index },
   { 19304, FlushMappedBufferRange_remap_index },
   { 27863, MapBufferRange_remap_index },
   { 27770, TexBufferARB_remap_index },
   { 16420, BindVertexArray_remap_index },
   { 14713, GenVertexArrays_remap_index },
   { 30243, CopyBufferSubData_remap_index },
   { 31173, ClientWaitSync_remap_index },
   {  2609, DeleteSync_remap_index },
   {  7174, FenceSync_remap_index },
   { 15261, GetInteger64v_remap_index },
   { 22655, GetSynciv_remap_index },
   { 29386, IsSync_remap_index },
   {  9371, WaitSync_remap_index },
   {  3797, DrawElementsBaseVertex_remap_index },
   { 30578, DrawRangeElementsBaseVertex_remap_index },
   { 26430, MultiDrawElementsBaseVertex_remap_index },
   { 16642, BlendEquationSeparateiARB_remap_index },
   { 17818, BlendEquationiARB_remap_index },
   { 12615, BlendFuncSeparateiARB_remap_index },
   {  9796, BlendFunciARB_remap_index },
   {  5091, BindTransformFeedback_remap_index },
   {  3232, DeleteTransformFeedbacks_remap_index },
   {  6429, DrawTransformFeedback_remap_index },
   {  9590, GenTransformFeedbacks_remap_index },
   { 28279, IsTransformFeedback_remap_index },
   { 26010, PauseTransformFeedback_remap_index },
   {  5528, ResumeTransformFeedback_remap_index },
   { 21713, ClearDepthf_remap_index },
   {  6733, DepthRangef_remap_index },
   { 14525, GetShaderPrecisionFormat_remap_index },
   {  9976, ReleaseShaderCompiler_remap_index },
   { 10919, ShaderBinary_remap_index },
   {  5396, PolygonOffsetEXT_remap_index },
   { 23277, GetPixelTexGenParameterfvSGIS_remap_index },
   {  4417, GetPixelTexGenParameterivSGIS_remap_index },
   { 23010, PixelTexGenParameterfSGIS_remap_index },
   {   624, PixelTexGenParameterfvSGIS_remap_index },
   { 12684, PixelTexGenParameteriSGIS_remap_index },
   { 13821, PixelTexGenParameterivSGIS_remap_index },
   { 18302, SampleMaskSGIS_remap_index },
   { 19986, SamplePatternSGIS_remap_index },
   { 26359, ColorPointerEXT_remap_index },
   { 17612, EdgeFlagPointerEXT_remap_index },
   {  6050, IndexPointerEXT_remap_index },
   {  6130, NormalPointerEXT_remap_index },
   { 15855, TexCoordPointerEXT_remap_index },
   {  6963, VertexPointerEXT_remap_index },
   {  3599, PointParameterfEXT_remap_index },
   {  7814, PointParameterfvEXT_remap_index },
   { 32049, LockArraysEXT_remap_index },
   { 14954, UnlockArraysEXT_remap_index },
   {  1267, SecondaryColor3bEXT_remap_index },
   {  7973, SecondaryColor3bvEXT_remap_index },
   { 10429, SecondaryColor3dEXT_remap_index },
   { 25587, SecondaryColor3dvEXT_remap_index },
   { 28145, SecondaryColor3fEXT_remap_index },
   { 18339, SecondaryColor3fvEXT_remap_index },
   {   470, SecondaryColor3iEXT_remap_index },
   { 16165, SecondaryColor3ivEXT_remap_index },
   { 10064, SecondaryColor3sEXT_remap_index },
   { 30837, SecondaryColor3svEXT_remap_index },
   { 27262, SecondaryColor3ubEXT_remap_index },
   { 21284, SecondaryColor3ubvEXT_remap_index },
   { 12926, SecondaryColor3uiEXT_remap_index },
   { 22897, SecondaryColor3uivEXT_remap_index },
   { 25867, SecondaryColor3usEXT_remap_index },
   { 12999, SecondaryColor3usvEXT_remap_index },
   { 11788, SecondaryColorPointerEXT_remap_index },
   { 25681, MultiDrawArraysEXT_remap_index },
   { 20974, MultiDrawElementsEXT_remap_index },
   { 21169, FogCoordPointerEXT_remap_index },
   {  4566, FogCoorddEXT_remap_index },
   { 31450, FogCoorddvEXT_remap_index },
   {  4683, FogCoordfEXT_remap_index },
   { 27185, FogCoordfvEXT_remap_index },
   { 19208, PixelTexGenSGIX_remap_index },
   { 27790, BlendFuncSeparateEXT_remap_index },
   {  6875, FlushVertexArrayRangeNV_remap_index },
   {  5345, VertexArrayRangeNV_remap_index },
   { 28210, CombinerInputNV_remap_index },
   {  2116, CombinerOutputNV_remap_index },
   { 30990, CombinerParameterfNV_remap_index },
   {  5219, CombinerParameterfvNV_remap_index },
   { 22379, CombinerParameteriNV_remap_index },
   { 32465, CombinerParameterivNV_remap_index },
   {  7251, FinalCombinerInputNV_remap_index },
   {  9817, GetCombinerInputParameterfvNV_remap_index },
   { 32302, GetCombinerInputParameterivNV_remap_index },
   { 13922, GetCombinerOutputParameterfvNV_remap_index },
   { 13750, GetCombinerOutputParameterivNV_remap_index },
   {  6637, GetFinalCombinerInputParameterfvNV_remap_index },
   { 24830, GetFinalCombinerInputParameterivNV_remap_index },
   { 12593, ResizeBuffersMESA_remap_index },
   { 11114, WindowPos2dMESA_remap_index },
   {  1060, WindowPos2dvMESA_remap_index },
   { 33293, WindowPos2fMESA_remap_index },
   {  7918, WindowPos2fvMESA_remap_index },
   { 18249, WindowPos2iMESA_remap_index },
   { 20459, WindowPos2ivMESA_remap_index },
   { 21073, WindowPos2sMESA_remap_index },
   {  5748, WindowPos2svMESA_remap_index },
   {  7743, WindowPos3dMESA_remap_index },
   { 14068, WindowPos3dvMESA_remap_index },
   {   516, WindowPos3fMESA_remap_index },
   { 15015, WindowPos3fvMESA_remap_index },
   { 24091, WindowPos3iMESA_remap_index },
   { 30188, WindowPos3ivMESA_remap_index },
   { 18903, WindowPos3sMESA_remap_index },
   { 31706, WindowPos3svMESA_remap_index },
   { 11065, WindowPos4dMESA_remap_index },
   { 17066, WindowPos4dvMESA_remap_index },
   { 14027, WindowPos4fMESA_remap_index },
   { 30744, WindowPos4fvMESA_remap_index },
   { 30341, WindowPos4iMESA_remap_index },
   { 12396, WindowPos4ivMESA_remap_index },
   { 19087, WindowPos4sMESA_remap_index },
   {  3183, WindowPos4svMESA_remap_index },
   { 13789, MultiModeDrawArraysIBM_remap_index },
   { 28985, MultiModeDrawElementsIBM_remap_index },
   { 12200, DeleteFencesNV_remap_index },
   { 28057, FinishFenceNV_remap_index },
   {  3721, GenFencesNV_remap_index },
   { 17046, GetFenceivNV_remap_index },
   {  8210, IsFenceNV_remap_index },
   { 13677, SetFenceNV_remap_index },
   {  4178, TestFenceNV_remap_index },
   { 31677, AreProgramsResidentNV_remap_index },
   { 31032, BindProgramNV_remap_index },
   { 25950, DeleteProgramsNV_remap_index },
   { 21502, ExecuteProgramNV_remap_index },
   { 33186, GenProgramsNV_remap_index },
   { 23382, GetProgramParameterdvNV_remap_index },
   { 10491, GetProgramParameterfvNV_remap_index },
   { 26333, GetProgramStringNV_remap_index },
   { 24468, GetProgramivNV_remap_index },
   { 23645, GetTrackMatrixivNV_remap_index },
   { 26127, GetVertexAttribPointervNV_remap_index },
   { 24763, GetVertexAttribdvNV_remap_index },
   {  9266, GetVertexAttribfvNV_remap_index },
   { 18510, GetVertexAttribivNV_remap_index },
   { 19334, IsProgramNV_remap_index },
   {  9349, LoadProgramNV_remap_index },
   { 27886, ProgramParameters4dvNV_remap_index },
   { 24398, ProgramParameters4fvNV_remap_index },
   { 20763, RequestResidentProgramsNV_remap_index },
   { 22357, TrackMatrixNV_remap_index },
   { 32279, VertexAttrib1dNV_remap_index },
   { 13588, VertexAttrib1dvNV_remap_index },
   { 28491, VertexAttrib1fNV_remap_index },
   {  2415, VertexAttrib1fvNV_remap_index },
   { 30801, VertexAttrib1sNV_remap_index },
   { 15088, VertexAttrib1svNV_remap_index },
   {  4845, VertexAttrib2dNV_remap_index },
   { 13481, VertexAttrib2dvNV_remap_index },
   { 20218, VertexAttrib2fNV_remap_index },
   { 13047, VertexAttrib2fvNV_remap_index },
   {  5960, VertexAttrib2sNV_remap_index },
   { 18957, VertexAttrib2svNV_remap_index },
   { 11262, VertexAttrib3dNV_remap_index },
   { 31927, VertexAttrib3dvNV_remap_index },
   { 10303, VertexAttrib3fNV_remap_index },
   { 24790, VertexAttrib3fvNV_remap_index },
   { 22200, VertexAttrib3sNV_remap_index },
   { 23672, VertexAttrib3svNV_remap_index },
   { 28959, VertexAttrib4dNV_remap_index },
   { 33223, VertexAttrib4dvNV_remap_index },
   {  4467, VertexAttrib4fNV_remap_index },
   {  9399, VertexAttrib4fvNV_remap_index },
   { 26809, VertexAttrib4sNV_remap_index },
   {  1409, VertexAttrib4svNV_remap_index },
   {  5003, VertexAttrib4ubNV_remap_index },
   {   778, VertexAttrib4ubvNV_remap_index },
   { 21682, VertexAttribPointerNV_remap_index },
   {  2267, VertexAttribs1dvNV_remap_index },
   { 26215, VertexAttribs1fvNV_remap_index },
   { 33023, VertexAttribs1svNV_remap_index },
   { 10328, VertexAttribs2dvNV_remap_index },
   { 25352, VertexAttribs2fvNV_remap_index },
   { 17638, VertexAttribs2svNV_remap_index },
   {  5247, VertexAttribs3dvNV_remap_index },
   {  2147, VertexAttribs3fvNV_remap_index },
   { 29936, VertexAttribs3svNV_remap_index },
   { 26899, VertexAttribs4dvNV_remap_index },
   {  5319, VertexAttribs4fvNV_remap_index },
   { 32810, VertexAttribs4svNV_remap_index },
   { 29684, VertexAttribs4ubvNV_remap_index },
   { 26969, GetTexBumpParameterfvATI_remap_index },
   { 33064, GetTexBumpParameterivATI_remap_index },
   { 18620, TexBumpParameterfvATI_remap_index },
   { 20634, TexBumpParameterivATI_remap_index },
   { 15634, AlphaFragmentOp1ATI_remap_index },
   { 25633, AlphaFragmentOp2ATI_remap_index },
   { 24706, AlphaFragmentOp3ATI_remap_index },
   { 29863, BeginFragmentShaderATI_remap_index },
   { 31231, BindFragmentShaderATI_remap_index },
   { 23801, ColorFragmentOp1ATI_remap_index },
   {  4345, ColorFragmentOp2ATI_remap_index },
   { 31572, ColorFragmentOp3ATI_remap_index },
   {  5485, DeleteFragmentShaderATI_remap_index },
   { 33247, EndFragmentShaderATI_remap_index },
   { 32493, GenFragmentShadersATI_remap_index },
   { 25506, PassTexCoordATI_remap_index },
   {  6943, SampleMapATI_remap_index },
   { 27080, SetFragmentShaderConstantATI_remap_index },
   {   363, PointParameteriNV_remap_index },
   { 14229, PointParameterivNV_remap_index },
   { 28798, ActiveStencilFaceEXT_remap_index },
   { 27526, BindVertexArrayAPPLE_remap_index },
   {  2737, DeleteVertexArraysAPPLE_remap_index },
   { 18079, GenVertexArraysAPPLE_remap_index },
   { 23447, IsVertexArrayAPPLE_remap_index },
   {   819, GetProgramNamedParameterdvNV_remap_index },
   {  3562, GetProgramNamedParameterfvNV_remap_index },
   { 27000, ProgramNamedParameter4dNV_remap_index },
   { 14588, ProgramNamedParameter4dvNV_remap_index },
   {  8882, ProgramNamedParameter4fNV_remap_index },
   { 11753, ProgramNamedParameter4fvNV_remap_index },
   { 16977, PrimitiveRestartIndexNV_remap_index },
   { 30721, PrimitiveRestartNV_remap_index },
   { 24377, DepthBoundsEXT_remap_index },
   {  1159, BlendEquationSeparateEXT_remap_index },
   { 14789, BindFramebufferEXT_remap_index },
   { 25726, BindRenderbufferEXT_remap_index },
   {  9646, CheckFramebufferStatusEXT_remap_index },
   { 22698, DeleteFramebuffersEXT_remap_index },
   { 31829, DeleteRenderbuffersEXT_remap_index },
   { 13505, FramebufferRenderbufferEXT_remap_index },
   { 13694, FramebufferTexture1DEXT_remap_index },
   { 11547, FramebufferTexture2DEXT_remap_index },
   { 11167, FramebufferTexture3DEXT_remap_index },
   { 23313, GenFramebuffersEXT_remap_index },
   { 17503, GenRenderbuffersEXT_remap_index },
   {  6679, GenerateMipmapEXT_remap_index },
   { 21775, GetFramebufferAttachmentParameterivEXT_remap_index },
   { 32399, GetRenderbufferParameterivEXT_remap_index },
   { 20514, IsFramebufferEXT_remap_index },
   { 33146, IsRenderbufferEXT_remap_index },
   {  8157, RenderbufferStorageEXT_remap_index },
   {   695, BlitFramebufferEXT_remap_index },
   { 14374, BufferParameteriAPPLE_remap_index },
   { 19366, FlushMappedBufferRangeAPPLE_remap_index },
   {  1815, BindFragDataLocationEXT_remap_index },
   { 24490, GetFragDataLocationEXT_remap_index },
   { 10606, GetUniformuivEXT_remap_index },
   {  2933, GetVertexAttribIivEXT_remap_index },
   {  4195, GetVertexAttribIuivEXT_remap_index },
   { 12033, Uniform1uiEXT_remap_index },
   { 27971, Uniform1uivEXT_remap_index },
   { 22296, Uniform2uiEXT_remap_index },
   {  4309, Uniform2uivEXT_remap_index },
   { 29238, Uniform3uiEXT_remap_index },
   { 14735, Uniform3uivEXT_remap_index },
   {  3486, Uniform4uiEXT_remap_index },
   {  8658, Uniform4uivEXT_remap_index },
   { 18467, VertexAttribI1iEXT_remap_index },
   {   965, VertexAttribI1ivEXT_remap_index },
   {  2516, VertexAttribI1uiEXT_remap_index },
   { 12775, VertexAttribI1uivEXT_remap_index },
   {    81, VertexAttribI2iEXT_remap_index },
   { 23913, VertexAttribI2ivEXT_remap_index },
   {  5273, VertexAttribI2uiEXT_remap_index },
   {  4728, VertexAttribI2uivEXT_remap_index },
   { 26601, VertexAttribI3iEXT_remap_index },
   { 30533, VertexAttribI3ivEXT_remap_index },
   {  3340, VertexAttribI3uiEXT_remap_index },
   { 30429, VertexAttribI3uivEXT_remap_index },
   { 22026, VertexAttribI4bvEXT_remap_index },
   { 14667, VertexAttribI4iEXT_remap_index },
   { 32098, VertexAttribI4ivEXT_remap_index },
   { 13408, VertexAttribI4svEXT_remap_index },
   { 16529, VertexAttribI4ubvEXT_remap_index },
   { 16228, VertexAttribI4uiEXT_remap_index },
   {  5419, VertexAttribI4uivEXT_remap_index },
   { 11330, VertexAttribI4usvEXT_remap_index },
   { 18564, VertexAttribIPointerEXT_remap_index },
   {  3027, FramebufferTextureLayerEXT_remap_index },
   {  5660, ColorMaskIndexedEXT_remap_index },
   { 18981, DisableIndexedEXT_remap_index },
   { 26646, EnableIndexedEXT_remap_index },
   { 21730, GetBooleanIndexedvEXT_remap_index },
   { 10941, GetIntegerIndexedvEXT_remap_index },
   { 22774, IsEnabledIndexedEXT_remap_index },
   { 22674, ClearColorIiEXT_remap_index },
   {  3436, ClearColorIuiEXT_remap_index },
   {  9856, GetTexParameterIivEXT_remap_index },
   {  5908, GetTexParameterIuivEXT_remap_index },
   {  2983, TexParameterIivEXT_remap_index },
   { 26468, TexParameterIuivEXT_remap_index },
   {  4596, BeginConditionalRenderNV_remap_index },
   { 25456, EndConditionalRenderNV_remap_index },
   {  9293, BeginTransformFeedbackEXT_remap_index },
   { 19016, BindBufferBaseEXT_remap_index },
   { 18875, BindBufferOffsetEXT_remap_index },
   { 12221, BindBufferRangeEXT_remap_index },
   { 14289, EndTransformFeedbackEXT_remap_index },
   { 10804, GetTransformFeedbackVaryingEXT_remap_index },
   { 20819, TransformFeedbackVaryingsEXT_remap_index },
   { 29585, ProvokingVertexEXT_remap_index },
   { 10752, GetTexParameterPointervAPPLE_remap_index },
   {  5047, TextureRangeAPPLE_remap_index },
   { 11619, GetObjectParameterivAPPLE_remap_index },
   { 19941, ObjectPurgeableAPPLE_remap_index },
   {  5702, ObjectUnpurgeableAPPLE_remap_index },
   { 17325, ActiveProgramEXT_remap_index },
   { 17296, CreateShaderProgramEXT_remap_index },
   { 28583, UseShaderProgramEXT_remap_index },
   { 16508, TextureBarrierNV_remap_index },
   { 28824, StencilFuncSeparateATI_remap_index },
   { 18168, ProgramEnvParameters4fvEXT_remap_index },
   { 17190, ProgramLocalParameters4fvEXT_remap_index },
   { 14157, GetQueryObjecti64vEXT_remap_index },
   { 10354, GetQueryObjectui64vEXT_remap_index },
   { 23870, EGLImageTargetRenderbufferStorageOES_remap_index },
   { 12139, EGLImageTargetTexture2DOES_remap_index },
   {    -1, -1 }
};

/* these functions are in the ABI, but have alternative names */
static const struct gl_function_remap MESA_alt_functions[] = {
   /* from GL_EXT_blend_color */
   {  2655, _gloffset_BlendColor },
   /* from GL_EXT_blend_minmax */
   { 11224, _gloffset_BlendEquation },
   /* from GL_EXT_color_subtable */
   { 17088, _gloffset_ColorSubTable },
   { 31761, _gloffset_CopyColorSubTable },
   /* from GL_EXT_convolution */
   {   257, _gloffset_ConvolutionFilter1D },
   {  2454, _gloffset_CopyConvolutionFilter1D },
   {  4058, _gloffset_GetConvolutionParameteriv },
   {  8506, _gloffset_ConvolutionFilter2D },
   {  8708, _gloffset_ConvolutionParameteriv },
   {  9168, _gloffset_ConvolutionParameterfv },
   { 20662, _gloffset_GetSeparableFilter },
   { 24145, _gloffset_SeparableFilter2D },
   { 25008, _gloffset_ConvolutionParameteri },
   { 25131, _gloffset_ConvolutionParameterf },
   { 26835, _gloffset_GetConvolutionParameterfv },
   { 27692, _gloffset_GetConvolutionFilter },
   { 30125, _gloffset_CopyConvolutionFilter2D },
   /* from GL_EXT_copy_texture */
   { 15148, _gloffset_CopyTexSubImage3D },
   { 16775, _gloffset_CopyTexImage2D },
   { 24616, _gloffset_CopyTexImage1D },
   { 27373, _gloffset_CopyTexSubImage2D },
   { 29763, _gloffset_CopyTexSubImage1D },
   /* from GL_EXT_draw_range_elements */
   {  9505, _gloffset_DrawRangeElements },
   /* from GL_EXT_histogram */
   {   856, _gloffset_Histogram },
   {  3522, _gloffset_ResetHistogram },
   { 10002, _gloffset_GetMinmax },
   { 15482, _gloffset_GetHistogramParameterfv },
   { 24541, _gloffset_GetMinmaxParameteriv },
   { 26725, _gloffset_ResetMinmax },
   { 27589, _gloffset_GetHistogramParameteriv },
   { 28758, _gloffset_GetHistogram },
   { 31347, _gloffset_Minmax },
   { 32893, _gloffset_GetMinmaxParameterfv },
   /* from GL_EXT_paletted_texture */
   {  8368, _gloffset_ColorTable },
   { 15328, _gloffset_GetColorTable },
   { 23060, _gloffset_GetColorTableParameterfv },
   { 25187, _gloffset_GetColorTableParameteriv },
   /* from GL_EXT_subtexture */
   {  7089, _gloffset_TexSubImage1D },
   { 10679, _gloffset_TexSubImage2D },
   /* from GL_EXT_texture3D */
   {  1774, _gloffset_TexImage3D },
   { 22829, _gloffset_TexSubImage3D },
   /* from GL_EXT_texture_object */
   {  3290, _gloffset_PrioritizeTextures },
   {  7538, _gloffset_AreTexturesResident },
   { 13612, _gloffset_GenTextures },
   { 15814, _gloffset_DeleteTextures },
   { 19647, _gloffset_IsTexture },
   { 29828, _gloffset_BindTexture },
   /* from GL_EXT_vertex_array */
   { 24317, _gloffset_ArrayElement },
   { 30935, _gloffset_GetPointerv },
   { 32520, _gloffset_DrawArrays },
   /* from GL_SGI_color_table */
   {  7656, _gloffset_ColorTableParameteriv },
   {  8368, _gloffset_ColorTable },
   { 15328, _gloffset_GetColorTable },
   { 15438, _gloffset_CopyColorTable },
   { 19508, _gloffset_ColorTableParameterfv },
   { 23060, _gloffset_GetColorTableParameterfv },
   { 25187, _gloffset_GetColorTableParameteriv },
   /* from GL_VERSION_1_3 */
   {   425, _gloffset_MultiTexCoord3sARB },
   {   657, _gloffset_ActiveTextureARB },
   {  4247, _gloffset_MultiTexCoord1fvARB },
   {  6155, _gloffset_MultiTexCoord3dARB },
   {  6200, _gloffset_MultiTexCoord2iARB },
   {  6324, _gloffset_MultiTexCoord2svARB },
   {  8324, _gloffset_MultiTexCoord2fARB },
   { 10384, _gloffset_MultiTexCoord3fvARB },
   { 10986, _gloffset_MultiTexCoord4sARB },
   { 11667, _gloffset_MultiTexCoord2dvARB },
   { 12082, _gloffset_MultiTexCoord1svARB },
   { 12454, _gloffset_MultiTexCoord3svARB },
   { 12515, _gloffset_MultiTexCoord4iARB },
   { 13316, _gloffset_MultiTexCoord3iARB },
   { 14186, _gloffset_MultiTexCoord1dARB },
   { 14403, _gloffset_MultiTexCoord3dvARB },
   { 15682, _gloffset_MultiTexCoord3ivARB },
   { 15727, _gloffset_MultiTexCoord2sARB },
   { 17145, _gloffset_MultiTexCoord4ivARB },
   { 19158, _gloffset_ClientActiveTextureARB },
   { 21458, _gloffset_MultiTexCoord2dARB },
   { 21895, _gloffset_MultiTexCoord4dvARB },
   { 22251, _gloffset_MultiTexCoord4fvARB },
   { 23201, _gloffset_MultiTexCoord3fARB },
   { 25771, _gloffset_MultiTexCoord4dARB },
   { 26037, _gloffset_MultiTexCoord1sARB },
   { 26241, _gloffset_MultiTexCoord1dvARB },
   { 27217, _gloffset_MultiTexCoord1ivARB },
   { 27310, _gloffset_MultiTexCoord2ivARB },
   { 27649, _gloffset_MultiTexCoord1iARB },
   { 29033, _gloffset_MultiTexCoord4svARB },
   { 29627, _gloffset_MultiTexCoord1fARB },
   { 29890, _gloffset_MultiTexCoord4fARB },
   { 32354, _gloffset_MultiTexCoord2fvARB },
   {    -1, -1 }
};

#endif /* need_MESA_remap_table */

#if defined(need_GL_3DFX_tbuffer)
static const struct gl_function_remap GL_3DFX_tbuffer_functions[] = {
   {  9226, -1 }, /* TbufferMask3DFX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_APPLE_flush_buffer_range)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_APPLE_flush_buffer_range_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_APPLE_object_purgeable)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_APPLE_object_purgeable_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_APPLE_texture_range)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_APPLE_texture_range_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_APPLE_vertex_array_object)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_APPLE_vertex_array_object_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_ES2_compatibility)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_ES2_compatibility_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_color_buffer_float)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_color_buffer_float_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_copy_buffer)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_copy_buffer_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_draw_buffers)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_draw_buffers_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_draw_buffers_blend)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_draw_buffers_blend_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_draw_elements_base_vertex)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_draw_elements_base_vertex_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_draw_instanced)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_draw_instanced_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_framebuffer_object)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_framebuffer_object_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_geometry_shader4)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_geometry_shader4_functions[] = {
   { 12418, -1 }, /* FramebufferTextureLayer */
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_instanced_arrays)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_instanced_arrays_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_map_buffer_range)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_map_buffer_range_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_matrix_palette)
static const struct gl_function_remap GL_ARB_matrix_palette_functions[] = {
   {  3773, -1 }, /* MatrixIndexusvARB */
   { 13137, -1 }, /* MatrixIndexuivARB */
   { 14558, -1 }, /* MatrixIndexPointerARB */
   { 19896, -1 }, /* CurrentPaletteMatrixARB */
   { 22945, -1 }, /* MatrixIndexubvARB */
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_multisample)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_multisample_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_occlusion_query)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_occlusion_query_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_point_parameters)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_point_parameters_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_provoking_vertex)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_provoking_vertex_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_shader_objects)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_shader_objects_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_sync)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_sync_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_texture_buffer_object)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_texture_buffer_object_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_texture_compression)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_texture_compression_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_transform_feedback2)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_transform_feedback2_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_transpose_matrix)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_transpose_matrix_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_vertex_array_object)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_vertex_array_object_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_vertex_blend)
static const struct gl_function_remap GL_ARB_vertex_blend_functions[] = {
   {  2396, -1 }, /* WeightubvARB */
   {  6567, -1 }, /* WeightivARB */
   { 11089, -1 }, /* WeightPointerARB */
   { 13904, -1 }, /* WeightfvARB */
   { 17664, -1 }, /* WeightbvARB */
   { 21126, -1 }, /* WeightusvARB */
   { 24071, -1 }, /* VertexBlendARB */
   { 29711, -1 }, /* WeightsvARB */
   { 31811, -1 }, /* WeightdvARB */
   { 32554, -1 }, /* WeightuivARB */
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_vertex_buffer_object)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_vertex_buffer_object_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_vertex_program)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_vertex_program_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_vertex_shader)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_vertex_shader_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_window_pos)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_window_pos_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ATI_blend_equation_separate)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ATI_blend_equation_separate_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ATI_draw_buffers)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ATI_draw_buffers_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ATI_envmap_bumpmap)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ATI_envmap_bumpmap_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ATI_fragment_shader)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ATI_fragment_shader_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ATI_separate_stencil)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ATI_separate_stencil_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_blend_color)
static const struct gl_function_remap GL_EXT_blend_color_functions[] = {
   {  2655, _gloffset_BlendColor },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_blend_equation_separate)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_blend_equation_separate_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_blend_func_separate)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_blend_func_separate_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_blend_minmax)
static const struct gl_function_remap GL_EXT_blend_minmax_functions[] = {
   { 11224, _gloffset_BlendEquation },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_color_subtable)
static const struct gl_function_remap GL_EXT_color_subtable_functions[] = {
   { 17088, _gloffset_ColorSubTable },
   { 31761, _gloffset_CopyColorSubTable },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_compiled_vertex_array)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_compiled_vertex_array_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_convolution)
static const struct gl_function_remap GL_EXT_convolution_functions[] = {
   {   257, _gloffset_ConvolutionFilter1D },
   {  2454, _gloffset_CopyConvolutionFilter1D },
   {  4058, _gloffset_GetConvolutionParameteriv },
   {  8506, _gloffset_ConvolutionFilter2D },
   {  8708, _gloffset_ConvolutionParameteriv },
   {  9168, _gloffset_ConvolutionParameterfv },
   { 20662, _gloffset_GetSeparableFilter },
   { 24145, _gloffset_SeparableFilter2D },
   { 25008, _gloffset_ConvolutionParameteri },
   { 25131, _gloffset_ConvolutionParameterf },
   { 26835, _gloffset_GetConvolutionParameterfv },
   { 27692, _gloffset_GetConvolutionFilter },
   { 30125, _gloffset_CopyConvolutionFilter2D },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const struct gl_function_remap GL_EXT_coordinate_frame_functions[] = {
   { 10523, -1 }, /* TangentPointerEXT */
   { 12573, -1 }, /* Binormal3ivEXT */
   { 13269, -1 }, /* Tangent3sEXT */
   { 14623, -1 }, /* Tangent3fvEXT */
   { 18856, -1 }, /* Tangent3dvEXT */
   { 19594, -1 }, /* Binormal3bvEXT */
   { 20715, -1 }, /* Binormal3dEXT */
   { 22877, -1 }, /* Tangent3fEXT */
   { 25080, -1 }, /* Binormal3sEXT */
   { 25548, -1 }, /* Tangent3ivEXT */
   { 25567, -1 }, /* Tangent3dEXT */
   { 26514, -1 }, /* Binormal3svEXT */
   { 27115, -1 }, /* Binormal3fEXT */
   { 28023, -1 }, /* Binormal3dvEXT */
   { 29290, -1 }, /* Tangent3iEXT */
   { 30410, -1 }, /* Tangent3bvEXT */
   { 30970, -1 }, /* Tangent3bEXT */
   { 31534, -1 }, /* Binormal3fvEXT */
   { 32253, -1 }, /* BinormalPointerEXT */
   { 32658, -1 }, /* Tangent3svEXT */
   { 33095, -1 }, /* Binormal3bEXT */
   { 33272, -1 }, /* Binormal3iEXT */
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_copy_texture)
static const struct gl_function_remap GL_EXT_copy_texture_functions[] = {
   { 15148, _gloffset_CopyTexSubImage3D },
   { 16775, _gloffset_CopyTexImage2D },
   { 24616, _gloffset_CopyTexImage1D },
   { 27373, _gloffset_CopyTexSubImage2D },
   { 29763, _gloffset_CopyTexSubImage1D },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_cull_vertex)
static const struct gl_function_remap GL_EXT_cull_vertex_functions[] = {
   {  8857, -1 }, /* CullParameterdvEXT */
   { 11712, -1 }, /* CullParameterfvEXT */
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_depth_bounds_test)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_depth_bounds_test_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_draw_buffers2)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_draw_buffers2_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_draw_instanced)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_draw_instanced_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_draw_range_elements)
static const struct gl_function_remap GL_EXT_draw_range_elements_functions[] = {
   {  9505, _gloffset_DrawRangeElements },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_fog_coord)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_fog_coord_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_framebuffer_blit)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_framebuffer_blit_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_framebuffer_multisample)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_framebuffer_multisample_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_framebuffer_object)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_framebuffer_object_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_gpu_program_parameters)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_gpu_program_parameters_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_gpu_shader4)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_gpu_shader4_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_histogram)
static const struct gl_function_remap GL_EXT_histogram_functions[] = {
   {   856, _gloffset_Histogram },
   {  3522, _gloffset_ResetHistogram },
   { 10002, _gloffset_GetMinmax },
   { 15482, _gloffset_GetHistogramParameterfv },
   { 24541, _gloffset_GetMinmaxParameteriv },
   { 26725, _gloffset_ResetMinmax },
   { 27589, _gloffset_GetHistogramParameteriv },
   { 28758, _gloffset_GetHistogram },
   { 31347, _gloffset_Minmax },
   { 32893, _gloffset_GetMinmaxParameterfv },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_index_func)
static const struct gl_function_remap GL_EXT_index_func_functions[] = {
   { 11498, -1 }, /* IndexFuncEXT */
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_index_material)
static const struct gl_function_remap GL_EXT_index_material_functions[] = {
   { 21213, -1 }, /* IndexMaterialEXT */
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_light_texture)
static const struct gl_function_remap GL_EXT_light_texture_functions[] = {
   { 26534, -1 }, /* ApplyTextureEXT */
   { 26679, -1 }, /* TextureMaterialEXT */
   { 26704, -1 }, /* TextureLightEXT */
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_multi_draw_arrays)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_multi_draw_arrays_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_multisample)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_multisample_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_paletted_texture)
static const struct gl_function_remap GL_EXT_paletted_texture_functions[] = {
   {  8368, _gloffset_ColorTable },
   { 15328, _gloffset_GetColorTable },
   { 23060, _gloffset_GetColorTableParameterfv },
   { 25187, _gloffset_GetColorTableParameteriv },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_pixel_transform)
static const struct gl_function_remap GL_EXT_pixel_transform_functions[] = {
   { 21860, -1 }, /* PixelTransformParameterfEXT */
   { 21940, -1 }, /* PixelTransformParameteriEXT */
   { 30685, -1 }, /* PixelTransformParameterfvEXT */
   { 32217, -1 }, /* PixelTransformParameterivEXT */
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_point_parameters)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_point_parameters_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_polygon_offset)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_polygon_offset_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_provoking_vertex)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_provoking_vertex_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_secondary_color)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_secondary_color_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_separate_shader_objects)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_separate_shader_objects_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_stencil_two_side)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_stencil_two_side_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_subtexture)
static const struct gl_function_remap GL_EXT_subtexture_functions[] = {
   {  7089, _gloffset_TexSubImage1D },
   { 10679, _gloffset_TexSubImage2D },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_texture3D)
static const struct gl_function_remap GL_EXT_texture3D_functions[] = {
   {  1774, _gloffset_TexImage3D },
   { 22829, _gloffset_TexSubImage3D },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_texture_array)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_texture_array_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_texture_integer)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_texture_integer_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_texture_object)
static const struct gl_function_remap GL_EXT_texture_object_functions[] = {
   {  3290, _gloffset_PrioritizeTextures },
   {  7538, _gloffset_AreTexturesResident },
   { 13612, _gloffset_GenTextures },
   { 15814, _gloffset_DeleteTextures },
   { 19647, _gloffset_IsTexture },
   { 29828, _gloffset_BindTexture },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_texture_perturb_normal)
static const struct gl_function_remap GL_EXT_texture_perturb_normal_functions[] = {
   { 13854, -1 }, /* TextureNormalEXT */
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_timer_query)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_timer_query_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_transform_feedback)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_transform_feedback_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_vertex_array)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_vertex_array_functions[] = {
   { 24317, _gloffset_ArrayElement },
   { 30935, _gloffset_GetPointerv },
   { 32520, _gloffset_DrawArrays },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_vertex_weighting)
static const struct gl_function_remap GL_EXT_vertex_weighting_functions[] = {
   { 19677, -1 }, /* VertexWeightfvEXT */
   { 27058, -1 }, /* VertexWeightfEXT */
   { 28727, -1 }, /* VertexWeightPointerEXT */
   {    -1, -1 }
};
#endif

#if defined(need_GL_HP_image_transform)
static const struct gl_function_remap GL_HP_image_transform_functions[] = {
   {  2327, -1 }, /* GetImageTransformParameterfvHP */
   {  3739, -1 }, /* ImageTransformParameterfHP */
   { 10217, -1 }, /* ImageTransformParameterfvHP */
   { 11967, -1 }, /* ImageTransformParameteriHP */
   { 12308, -1 }, /* GetImageTransformParameterivHP */
   { 19741, -1 }, /* ImageTransformParameterivHP */
   {    -1, -1 }
};
#endif

#if defined(need_GL_IBM_multimode_draw_arrays)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_IBM_multimode_draw_arrays_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_IBM_vertex_array_lists)
static const struct gl_function_remap GL_IBM_vertex_array_lists_functions[] = {
   {  4379, -1 }, /* SecondaryColorPointerListIBM */
   {  6021, -1 }, /* NormalPointerListIBM */
   {  7712, -1 }, /* FogCoordPointerListIBM */
   {  8019, -1 }, /* VertexPointerListIBM */
   { 11888, -1 }, /* ColorPointerListIBM */
   { 13376, -1 }, /* TexCoordPointerListIBM */
   { 13876, -1 }, /* IndexPointerListIBM */
   { 32836, -1 }, /* EdgeFlagPointerListIBM */
   {    -1, -1 }
};
#endif

#if defined(need_GL_INGR_blend_func_separate)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_INGR_blend_func_separate_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_INTEL_parallel_arrays)
static const struct gl_function_remap GL_INTEL_parallel_arrays_functions[] = {
   { 12716, -1 }, /* VertexPointervINTEL */
   { 15575, -1 }, /* ColorPointervINTEL */
   { 30099, -1 }, /* NormalPointervINTEL */
   { 30617, -1 }, /* TexCoordPointervINTEL */
   {    -1, -1 }
};
#endif

#if defined(need_GL_MESA_resize_buffers)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_MESA_resize_buffers_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_MESA_shader_debug)
static const struct gl_function_remap GL_MESA_shader_debug_functions[] = {
   {  1638, -1 }, /* GetDebugLogLengthMESA */
   {  3461, -1 }, /* ClearDebugLogMESA */
   {  4540, -1 }, /* GetDebugLogMESA */
   { 31128, -1 }, /* CreateDebugObjectMESA */
   {    -1, -1 }
};
#endif

#if defined(need_GL_MESA_window_pos)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_MESA_window_pos_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_NV_condtitional_render)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_NV_condtitional_render_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_NV_evaluators)
static const struct gl_function_remap GL_NV_evaluators_functions[] = {
   {  6751, -1 }, /* GetMapAttribParameterivNV */
   {  8474, -1 }, /* MapControlPointsNV */
   {  8573, -1 }, /* MapParameterfvNV */
   { 10662, -1 }, /* EvalMapsNV */
   { 17262, -1 }, /* GetMapAttribParameterfvNV */
   { 17479, -1 }, /* MapParameterivNV */
   { 24931, -1 }, /* GetMapParameterivNV */
   { 25429, -1 }, /* GetMapParameterfvNV */
   { 29414, -1 }, /* GetMapControlPointsNV */
   {    -1, -1 }
};
#endif

#if defined(need_GL_NV_fence)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_NV_fence_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_NV_fragment_program)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_NV_fragment_program_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_NV_point_sprite)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_NV_point_sprite_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_NV_primitive_restart)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_NV_primitive_restart_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_NV_register_combiners)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_NV_register_combiners_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_NV_register_combiners2)
static const struct gl_function_remap GL_NV_register_combiners2_functions[] = {
   { 15967, -1 }, /* CombinerStageParameterfvNV */
   { 16363, -1 }, /* GetCombinerStageParameterfvNV */
   {    -1, -1 }
};
#endif

#if defined(need_GL_NV_texture_barrier)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_NV_texture_barrier_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_NV_vertex_array_range)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_NV_vertex_array_range_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_NV_vertex_program)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_NV_vertex_program_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_OES_EGL_image)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_OES_EGL_image_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_PGI_misc_hints)
static const struct gl_function_remap GL_PGI_misc_hints_functions[] = {
   {  8694, -1 }, /* HintPGI */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_detail_texture)
static const struct gl_function_remap GL_SGIS_detail_texture_functions[] = {
   { 16336, -1 }, /* GetDetailTexFuncSGIS */
   { 16720, -1 }, /* DetailTexFuncSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_fog_function)
static const struct gl_function_remap GL_SGIS_fog_function_functions[] = {
   { 27355, -1 }, /* FogFuncSGIS */
   { 28076, -1 }, /* GetFogFuncSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_multisample)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_SGIS_multisample_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_pixel_texture)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_SGIS_pixel_texture_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_point_parameters)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_SGIS_point_parameters_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_sharpen_texture)
static const struct gl_function_remap GL_SGIS_sharpen_texture_functions[] = {
   {  6812, -1 }, /* GetSharpenTexFuncSGIS */
   { 22225, -1 }, /* SharpenTexFuncSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_texture4D)
static const struct gl_function_remap GL_SGIS_texture4D_functions[] = {
   {  1010, -1 }, /* TexImage4DSGIS */
   { 15883, -1 }, /* TexSubImage4DSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_texture_color_mask)
static const struct gl_function_remap GL_SGIS_texture_color_mask_functions[] = {
   { 15281, -1 }, /* TextureColorMaskSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_texture_filter4)
static const struct gl_function_remap GL_SGIS_texture_filter4_functions[] = {
   {  6989, -1 }, /* GetTexFilterFuncSGIS */
   { 16482, -1 }, /* TexFilterFuncSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_async)
static const struct gl_function_remap GL_SGIX_async_functions[] = {
   {  3387, -1 }, /* AsyncMarkerSGIX */
   {  4519, -1 }, /* FinishAsyncSGIX */
   {  5466, -1 }, /* PollAsyncSGIX */
   { 22406, -1 }, /* DeleteAsyncMarkersSGIX */
   { 22461, -1 }, /* IsAsyncMarkerSGIX */
   { 32633, -1 }, /* GenAsyncMarkersSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_flush_raster)
static const struct gl_function_remap GL_SGIX_flush_raster_functions[] = {
   {  7366, -1 }, /* FlushRasterSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_fragment_lighting)
static const struct gl_function_remap GL_SGIX_fragment_lighting_functions[] = {
   {  2625, -1 }, /* FragmentMaterialfvSGIX */
   {  5370, -1 }, /* FragmentLightiSGIX */
   {  8086, -1 }, /* FragmentMaterialfSGIX */
   {  8247, -1 }, /* GetFragmentLightivSGIX */
   {  9120, -1 }, /* FragmentLightModeliSGIX */
   { 10725, -1 }, /* FragmentLightivSGIX */
   { 11032, -1 }, /* GetFragmentMaterialivSGIX */
   { 16276, -1 }, /* GetFragmentMaterialfvSGIX */
   { 19564, -1 }, /* FragmentLightModelfSGIX */
   { 19864, -1 }, /* FragmentColorMaterialSGIX */
   { 20281, -1 }, /* FragmentMaterialiSGIX */
   { 21541, -1 }, /* LightEnviSGIX */
   { 23152, -1 }, /* FragmentLightModelfvSGIX */
   { 23487, -1 }, /* FragmentLightfvSGIX */
   { 28460, -1 }, /* FragmentLightModelivSGIX */
   { 28609, -1 }, /* FragmentLightfSGIX */
   { 31504, -1 }, /* GetFragmentLightfvSGIX */
   { 33116, -1 }, /* FragmentMaterialivSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_framezoom)
static const struct gl_function_remap GL_SGIX_framezoom_functions[] = {
   { 22484, -1 }, /* FrameZoomSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_igloo_interface)
static const struct gl_function_remap GL_SGIX_igloo_interface_functions[] = {
   { 28917, -1 }, /* IglooInterfaceSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_instruments)
static const struct gl_function_remap GL_SGIX_instruments_functions[] = {
   {  2805, -1 }, /* ReadInstrumentsSGIX */
   {  6585, -1 }, /* PollInstrumentsSGIX */
   { 10583, -1 }, /* GetInstrumentsSGIX */
   { 12974, -1 }, /* StartInstrumentsSGIX */
   { 16001, -1 }, /* StopInstrumentsSGIX */
   { 17901, -1 }, /* InstrumentsBufferSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_list_priority)
static const struct gl_function_remap GL_SGIX_list_priority_functions[] = {
   {  1241, -1 }, /* ListParameterfSGIX */
   {  3089, -1 }, /* GetListParameterfvSGIX */
   { 17792, -1 }, /* ListParameteriSGIX */
   { 18806, -1 }, /* ListParameterfvSGIX */
   { 20947, -1 }, /* ListParameterivSGIX */
   { 32677, -1 }, /* GetListParameterivSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_pixel_texture)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_SGIX_pixel_texture_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_polynomial_ffd)
static const struct gl_function_remap GL_SGIX_polynomial_ffd_functions[] = {
   {  3685, -1 }, /* LoadIdentityDeformationMapSGIX */
   { 16101, -1 }, /* DeformSGIX */
   { 24429, -1 }, /* DeformationMap3fSGIX */
   { 31392, -1 }, /* DeformationMap3dSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_reference_plane)
static const struct gl_function_remap GL_SGIX_reference_plane_functions[] = {
   { 14832, -1 }, /* ReferencePlaneSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_sprite)
static const struct gl_function_remap GL_SGIX_sprite_functions[] = {
   {  9618, -1 }, /* SpriteParameterfvSGIX */
   { 20736, -1 }, /* SpriteParameteriSGIX */
   { 26759, -1 }, /* SpriteParameterfSGIX */
   { 29557, -1 }, /* SpriteParameterivSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_tag_sample_buffer)
static const struct gl_function_remap GL_SGIX_tag_sample_buffer_functions[] = {
   { 20795, -1 }, /* TagSampleBufferSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGI_color_table)
static const struct gl_function_remap GL_SGI_color_table_functions[] = {
   {  7656, _gloffset_ColorTableParameteriv },
   {  8368, _gloffset_ColorTable },
   { 15328, _gloffset_GetColorTable },
   { 15438, _gloffset_CopyColorTable },
   { 19508, _gloffset_ColorTableParameterfv },
   { 23060, _gloffset_GetColorTableParameterfv },
   { 25187, _gloffset_GetColorTableParameteriv },
   {    -1, -1 }
};
#endif

#if defined(need_GL_SUNX_constant_data)
static const struct gl_function_remap GL_SUNX_constant_data_functions[] = {
   { 31482, -1 }, /* FinishTextureSUNX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SUN_global_alpha)
static const struct gl_function_remap GL_SUN_global_alpha_functions[] = {
   {  3408, -1 }, /* GlobalAlphaFactorubSUN */
   {  4818, -1 }, /* GlobalAlphaFactoriSUN */
   {  6610, -1 }, /* GlobalAlphaFactordSUN */
   {  9702, -1 }, /* GlobalAlphaFactoruiSUN */
   { 10174, -1 }, /* GlobalAlphaFactorbSUN */
   { 13289, -1 }, /* GlobalAlphaFactorfSUN */
   { 13453, -1 }, /* GlobalAlphaFactorusSUN */
   { 22747, -1 }, /* GlobalAlphaFactorsSUN */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SUN_mesh_array)
static const struct gl_function_remap GL_SUN_mesh_array_functions[] = {
   { 29348, -1 }, /* DrawMeshArraysSUN */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SUN_triangle_list)
static const struct gl_function_remap GL_SUN_triangle_list_functions[] = {
   {  4493, -1 }, /* ReplacementCodeubSUN */
   {  6369, -1 }, /* ReplacementCodeubvSUN */
   { 19229, -1 }, /* ReplacementCodeusvSUN */
   { 19417, -1 }, /* ReplacementCodePointerSUN */
   { 21605, -1 }, /* ReplacementCodeuiSUN */
   { 22435, -1 }, /* ReplacementCodeusSUN */
   { 30014, -1 }, /* ReplacementCodeuivSUN */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SUN_vertex)
static const struct gl_function_remap GL_SUN_vertex_functions[] = {
   {  1115, -1 }, /* ReplacementCodeuiColor3fVertex3fvSUN */
   {  1313, -1 }, /* TexCoord4fColor4fNormal3fVertex4fvSUN */
   {  1539, -1 }, /* TexCoord2fColor4ubVertex3fvSUN */
   {  1869, -1 }, /* ReplacementCodeuiVertex3fvSUN */
   {  2003, -1 }, /* ReplacementCodeuiTexCoord2fVertex3fvSUN */
   {  2561, -1 }, /* ReplacementCodeuiNormal3fVertex3fSUN */
   {  2874, -1 }, /* Color4ubVertex3fvSUN */
   {  4652, -1 }, /* Color4ubVertex3fSUN */
   {  4775, -1 }, /* TexCoord2fVertex3fSUN */
   {  5119, -1 }, /* TexCoord2fColor4fNormal3fVertex3fSUN */
   {  5570, -1 }, /* TexCoord2fNormal3fVertex3fvSUN */
   {  6264, -1 }, /* ReplacementCodeuiTexCoord2fNormal3fVertex3fSUN */
   {  7044, -1 }, /* ReplacementCodeuiColor4ubVertex3fvSUN */
   {  7403, -1 }, /* ReplacementCodeuiTexCoord2fVertex3fSUN */
   {  8115, -1 }, /* TexCoord2fNormal3fVertex3fSUN */
   {  8919, -1 }, /* Color3fVertex3fSUN */
   { 10110, -1 }, /* Color3fVertex3fvSUN */
   { 10548, -1 }, /* Color4fNormal3fVertex3fvSUN */
   { 11377, -1 }, /* ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN */
   { 12837, -1 }, /* ReplacementCodeuiColor4fNormal3fVertex3fvSUN */
   { 14448, -1 }, /* ReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN */
   { 14974, -1 }, /* TexCoord2fColor3fVertex3fSUN */
   { 16026, -1 }, /* TexCoord4fColor4fNormal3fVertex4fSUN */
   { 16441, -1 }, /* Color4ubVertex2fvSUN */
   { 16745, -1 }, /* Normal3fVertex3fSUN */
   { 17842, -1 }, /* ReplacementCodeuiColor4fNormal3fVertex3fSUN */
   { 18203, -1 }, /* TexCoord2fColor4fNormal3fVertex3fvSUN */
   { 19058, -1 }, /* TexCoord2fVertex3fvSUN */
   { 19834, -1 }, /* Color4ubVertex2fSUN */
   { 20072, -1 }, /* ReplacementCodeuiColor4ubVertex3fSUN */
   { 22071, -1 }, /* TexCoord2fColor4ubVertex3fSUN */
   { 22503, -1 }, /* Normal3fVertex3fvSUN */
   { 22969, -1 }, /* Color4fNormal3fVertex3fSUN */
   { 23978, -1 }, /* ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN */
   { 26080, -1 }, /* ReplacementCodeuiColor3fVertex3fSUN */
   { 27471, -1 }, /* TexCoord4fVertex4fSUN */
   { 27917, -1 }, /* TexCoord2fColor3fVertex3fvSUN */
   { 28304, -1 }, /* ReplacementCodeuiNormal3fVertex3fvSUN */
   { 28431, -1 }, /* TexCoord4fVertex4fvSUN */
   { 29165, -1 }, /* ReplacementCodeuiVertex3fSUN */
   {    -1, -1 }
};
#endif

#if defined(need_GL_VERSION_1_3)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_VERSION_1_3_functions[] = {
   {   425, _gloffset_MultiTexCoord3sARB },
   {   657, _gloffset_ActiveTextureARB },
   {  4247, _gloffset_MultiTexCoord1fvARB },
   {  6155, _gloffset_MultiTexCoord3dARB },
   {  6200, _gloffset_MultiTexCoord2iARB },
   {  6324, _gloffset_MultiTexCoord2svARB },
   {  8324, _gloffset_MultiTexCoord2fARB },
   { 10384, _gloffset_MultiTexCoord3fvARB },
   { 10986, _gloffset_MultiTexCoord4sARB },
   { 11667, _gloffset_MultiTexCoord2dvARB },
   { 12082, _gloffset_MultiTexCoord1svARB },
   { 12454, _gloffset_MultiTexCoord3svARB },
   { 12515, _gloffset_MultiTexCoord4iARB },
   { 13316, _gloffset_MultiTexCoord3iARB },
   { 14186, _gloffset_MultiTexCoord1dARB },
   { 14403, _gloffset_MultiTexCoord3dvARB },
   { 15682, _gloffset_MultiTexCoord3ivARB },
   { 15727, _gloffset_MultiTexCoord2sARB },
   { 17145, _gloffset_MultiTexCoord4ivARB },
   { 19158, _gloffset_ClientActiveTextureARB },
   { 21458, _gloffset_MultiTexCoord2dARB },
   { 21895, _gloffset_MultiTexCoord4dvARB },
   { 22251, _gloffset_MultiTexCoord4fvARB },
   { 23201, _gloffset_MultiTexCoord3fARB },
   { 25771, _gloffset_MultiTexCoord4dARB },
   { 26037, _gloffset_MultiTexCoord1sARB },
   { 26241, _gloffset_MultiTexCoord1dvARB },
   { 27217, _gloffset_MultiTexCoord1ivARB },
   { 27310, _gloffset_MultiTexCoord2ivARB },
   { 27649, _gloffset_MultiTexCoord1iARB },
   { 29033, _gloffset_MultiTexCoord4svARB },
   { 29627, _gloffset_MultiTexCoord1fARB },
   { 29890, _gloffset_MultiTexCoord4fARB },
   { 32354, _gloffset_MultiTexCoord2fvARB },
   {    -1, -1 }
};
#endif

#if defined(need_GL_VERSION_1_4)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_VERSION_1_4_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_VERSION_1_5)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_VERSION_1_5_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_VERSION_2_0)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_VERSION_2_0_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_VERSION_2_1)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_VERSION_2_1_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_VERSION_3_0)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_VERSION_3_0_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_VERSION_3_1)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_VERSION_3_1_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_VERSION_3_2)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_VERSION_3_2_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_VERSION_3_3)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_VERSION_3_3_functions[] = {
   {    -1, -1 }
};
#endif

