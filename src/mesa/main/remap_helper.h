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
   /* _mesa_function_pool[81]: RasterPos4i (offset 82) */
   "iiii\0"
   "glRasterPos4i\0"
   "\0"
   /* _mesa_function_pool[101]: RasterPos4d (offset 78) */
   "dddd\0"
   "glRasterPos4d\0"
   "\0"
   /* _mesa_function_pool[121]: NewList (dynamic) */
   "ii\0"
   "glNewList\0"
   "\0"
   /* _mesa_function_pool[135]: RasterPos4f (offset 80) */
   "ffff\0"
   "glRasterPos4f\0"
   "\0"
   /* _mesa_function_pool[155]: LoadIdentity (offset 290) */
   "\0"
   "glLoadIdentity\0"
   "\0"
   /* _mesa_function_pool[172]: SampleCoverageARB (will be remapped) */
   "fi\0"
   "glSampleCoverage\0"
   "glSampleCoverageARB\0"
   "\0"
   /* _mesa_function_pool[213]: ConvolutionFilter1D (offset 348) */
   "iiiiip\0"
   "glConvolutionFilter1D\0"
   "glConvolutionFilter1DEXT\0"
   "\0"
   /* _mesa_function_pool[268]: BeginQueryARB (will be remapped) */
   "ii\0"
   "glBeginQuery\0"
   "glBeginQueryARB\0"
   "\0"
   /* _mesa_function_pool[301]: RasterPos3dv (offset 71) */
   "p\0"
   "glRasterPos3dv\0"
   "\0"
   /* _mesa_function_pool[319]: PointParameteriNV (will be remapped) */
   "ii\0"
   "glPointParameteri\0"
   "glPointParameteriNV\0"
   "\0"
   /* _mesa_function_pool[361]: GetProgramiv (will be remapped) */
   "iip\0"
   "glGetProgramiv\0"
   "\0"
   /* _mesa_function_pool[381]: MultiTexCoord3sARB (offset 398) */
   "iiii\0"
   "glMultiTexCoord3s\0"
   "glMultiTexCoord3sARB\0"
   "\0"
   /* _mesa_function_pool[426]: SecondaryColor3iEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3i\0"
   "glSecondaryColor3iEXT\0"
   "\0"
   /* _mesa_function_pool[472]: WindowPos3fMESA (will be remapped) */
   "fff\0"
   "glWindowPos3f\0"
   "glWindowPos3fARB\0"
   "glWindowPos3fMESA\0"
   "\0"
   /* _mesa_function_pool[526]: TexCoord1iv (offset 99) */
   "p\0"
   "glTexCoord1iv\0"
   "\0"
   /* _mesa_function_pool[543]: TexCoord4sv (offset 125) */
   "p\0"
   "glTexCoord4sv\0"
   "\0"
   /* _mesa_function_pool[560]: RasterPos4s (offset 84) */
   "iiii\0"
   "glRasterPos4s\0"
   "\0"
   /* _mesa_function_pool[580]: PixelTexGenParameterfvSGIS (will be remapped) */
   "ip\0"
   "glPixelTexGenParameterfvSGIS\0"
   "\0"
   /* _mesa_function_pool[613]: ActiveTextureARB (offset 374) */
   "i\0"
   "glActiveTexture\0"
   "glActiveTextureARB\0"
   "\0"
   /* _mesa_function_pool[651]: BlitFramebufferEXT (will be remapped) */
   "iiiiiiiiii\0"
   "glBlitFramebuffer\0"
   "glBlitFramebufferEXT\0"
   "\0"
   /* _mesa_function_pool[702]: TexCoord1f (offset 96) */
   "f\0"
   "glTexCoord1f\0"
   "\0"
   /* _mesa_function_pool[718]: TexCoord1d (offset 94) */
   "d\0"
   "glTexCoord1d\0"
   "\0"
   /* _mesa_function_pool[734]: VertexAttrib4ubvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib4ubvNV\0"
   "\0"
   /* _mesa_function_pool[759]: TexCoord1i (offset 98) */
   "i\0"
   "glTexCoord1i\0"
   "\0"
   /* _mesa_function_pool[775]: GetProgramNamedParameterdvNV (will be remapped) */
   "iipp\0"
   "glGetProgramNamedParameterdvNV\0"
   "\0"
   /* _mesa_function_pool[812]: Histogram (offset 367) */
   "iiii\0"
   "glHistogram\0"
   "glHistogramEXT\0"
   "\0"
   /* _mesa_function_pool[845]: TexCoord1s (offset 100) */
   "i\0"
   "glTexCoord1s\0"
   "\0"
   /* _mesa_function_pool[861]: GetMapfv (offset 267) */
   "iip\0"
   "glGetMapfv\0"
   "\0"
   /* _mesa_function_pool[877]: EvalCoord1f (offset 230) */
   "f\0"
   "glEvalCoord1f\0"
   "\0"
   /* _mesa_function_pool[894]: TexImage4DSGIS (dynamic) */
   "iiiiiiiiiip\0"
   "glTexImage4DSGIS\0"
   "\0"
   /* _mesa_function_pool[924]: PolygonStipple (offset 175) */
   "p\0"
   "glPolygonStipple\0"
   "\0"
   /* _mesa_function_pool[944]: WindowPos2dvMESA (will be remapped) */
   "p\0"
   "glWindowPos2dv\0"
   "glWindowPos2dvARB\0"
   "glWindowPos2dvMESA\0"
   "\0"
   /* _mesa_function_pool[999]: ReplacementCodeuiColor3fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glReplacementCodeuiColor3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[1043]: BlendEquationSeparateEXT (will be remapped) */
   "ii\0"
   "glBlendEquationSeparate\0"
   "glBlendEquationSeparateEXT\0"
   "glBlendEquationSeparateATI\0"
   "\0"
   /* _mesa_function_pool[1125]: ListParameterfSGIX (dynamic) */
   "iif\0"
   "glListParameterfSGIX\0"
   "\0"
   /* _mesa_function_pool[1151]: SecondaryColor3bEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3b\0"
   "glSecondaryColor3bEXT\0"
   "\0"
   /* _mesa_function_pool[1197]: TexCoord4fColor4fNormal3fVertex4fvSUN (dynamic) */
   "pppp\0"
   "glTexCoord4fColor4fNormal3fVertex4fvSUN\0"
   "\0"
   /* _mesa_function_pool[1243]: GetPixelMapfv (offset 271) */
   "ip\0"
   "glGetPixelMapfv\0"
   "\0"
   /* _mesa_function_pool[1263]: Color3uiv (offset 22) */
   "p\0"
   "glColor3uiv\0"
   "\0"
   /* _mesa_function_pool[1278]: IsEnabled (offset 286) */
   "i\0"
   "glIsEnabled\0"
   "\0"
   /* _mesa_function_pool[1293]: VertexAttrib4svNV (will be remapped) */
   "ip\0"
   "glVertexAttrib4svNV\0"
   "\0"
   /* _mesa_function_pool[1317]: EvalCoord2fv (offset 235) */
   "p\0"
   "glEvalCoord2fv\0"
   "\0"
   /* _mesa_function_pool[1335]: GetBufferSubDataARB (will be remapped) */
   "iiip\0"
   "glGetBufferSubData\0"
   "glGetBufferSubDataARB\0"
   "\0"
   /* _mesa_function_pool[1382]: BufferSubDataARB (will be remapped) */
   "iiip\0"
   "glBufferSubData\0"
   "glBufferSubDataARB\0"
   "\0"
   /* _mesa_function_pool[1423]: TexCoord2fColor4ubVertex3fvSUN (dynamic) */
   "ppp\0"
   "glTexCoord2fColor4ubVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[1461]: AttachShader (will be remapped) */
   "ii\0"
   "glAttachShader\0"
   "\0"
   /* _mesa_function_pool[1480]: VertexAttrib2fARB (will be remapped) */
   "iff\0"
   "glVertexAttrib2f\0"
   "glVertexAttrib2fARB\0"
   "\0"
   /* _mesa_function_pool[1522]: GetDebugLogLengthMESA (dynamic) */
   "iii\0"
   "glGetDebugLogLengthMESA\0"
   "\0"
   /* _mesa_function_pool[1551]: GetMapiv (offset 268) */
   "iip\0"
   "glGetMapiv\0"
   "\0"
   /* _mesa_function_pool[1567]: VertexAttrib3fARB (will be remapped) */
   "ifff\0"
   "glVertexAttrib3f\0"
   "glVertexAttrib3fARB\0"
   "\0"
   /* _mesa_function_pool[1610]: Indexubv (offset 316) */
   "p\0"
   "glIndexubv\0"
   "\0"
   /* _mesa_function_pool[1624]: GetQueryivARB (will be remapped) */
   "iip\0"
   "glGetQueryiv\0"
   "glGetQueryivARB\0"
   "\0"
   /* _mesa_function_pool[1658]: TexImage3D (offset 371) */
   "iiiiiiiiip\0"
   "glTexImage3D\0"
   "glTexImage3DEXT\0"
   "\0"
   /* _mesa_function_pool[1699]: ReplacementCodeuiVertex3fvSUN (dynamic) */
   "pp\0"
   "glReplacementCodeuiVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[1735]: EdgeFlagPointer (offset 312) */
   "ip\0"
   "glEdgeFlagPointer\0"
   "\0"
   /* _mesa_function_pool[1757]: Color3ubv (offset 20) */
   "p\0"
   "glColor3ubv\0"
   "\0"
   /* _mesa_function_pool[1772]: GetQueryObjectivARB (will be remapped) */
   "iip\0"
   "glGetQueryObjectiv\0"
   "glGetQueryObjectivARB\0"
   "\0"
   /* _mesa_function_pool[1818]: Vertex3dv (offset 135) */
   "p\0"
   "glVertex3dv\0"
   "\0"
   /* _mesa_function_pool[1833]: ReplacementCodeuiTexCoord2fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glReplacementCodeuiTexCoord2fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[1880]: CompressedTexSubImage2DARB (will be remapped) */
   "iiiiiiiip\0"
   "glCompressedTexSubImage2D\0"
   "glCompressedTexSubImage2DARB\0"
   "\0"
   /* _mesa_function_pool[1946]: CombinerOutputNV (will be remapped) */
   "iiiiiiiiii\0"
   "glCombinerOutputNV\0"
   "\0"
   /* _mesa_function_pool[1977]: VertexAttribs3fvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs3fvNV\0"
   "\0"
   /* _mesa_function_pool[2003]: Uniform2fARB (will be remapped) */
   "iff\0"
   "glUniform2f\0"
   "glUniform2fARB\0"
   "\0"
   /* _mesa_function_pool[2035]: LightModeliv (offset 166) */
   "ip\0"
   "glLightModeliv\0"
   "\0"
   /* _mesa_function_pool[2054]: VertexAttrib1svARB (will be remapped) */
   "ip\0"
   "glVertexAttrib1sv\0"
   "glVertexAttrib1svARB\0"
   "\0"
   /* _mesa_function_pool[2097]: VertexAttribs1dvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs1dvNV\0"
   "\0"
   /* _mesa_function_pool[2123]: Uniform2ivARB (will be remapped) */
   "iip\0"
   "glUniform2iv\0"
   "glUniform2ivARB\0"
   "\0"
   /* _mesa_function_pool[2157]: GetImageTransformParameterfvHP (dynamic) */
   "iip\0"
   "glGetImageTransformParameterfvHP\0"
   "\0"
   /* _mesa_function_pool[2195]: Normal3bv (offset 53) */
   "p\0"
   "glNormal3bv\0"
   "\0"
   /* _mesa_function_pool[2210]: TexGeniv (offset 193) */
   "iip\0"
   "glTexGeniv\0"
   "\0"
   /* _mesa_function_pool[2226]: WeightubvARB (dynamic) */
   "ip\0"
   "glWeightubvARB\0"
   "\0"
   /* _mesa_function_pool[2245]: VertexAttrib1fvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib1fvNV\0"
   "\0"
   /* _mesa_function_pool[2269]: Vertex3iv (offset 139) */
   "p\0"
   "glVertex3iv\0"
   "\0"
   /* _mesa_function_pool[2284]: CopyConvolutionFilter1D (offset 354) */
   "iiiii\0"
   "glCopyConvolutionFilter1D\0"
   "glCopyConvolutionFilter1DEXT\0"
   "\0"
   /* _mesa_function_pool[2346]: ReplacementCodeuiNormal3fVertex3fSUN (dynamic) */
   "iffffff\0"
   "glReplacementCodeuiNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[2394]: DeleteSync (will be remapped) */
   "i\0"
   "glDeleteSync\0"
   "\0"
   /* _mesa_function_pool[2410]: FragmentMaterialfvSGIX (dynamic) */
   "iip\0"
   "glFragmentMaterialfvSGIX\0"
   "\0"
   /* _mesa_function_pool[2440]: BlendColor (offset 336) */
   "ffff\0"
   "glBlendColor\0"
   "glBlendColorEXT\0"
   "\0"
   /* _mesa_function_pool[2475]: UniformMatrix4fvARB (will be remapped) */
   "iiip\0"
   "glUniformMatrix4fv\0"
   "glUniformMatrix4fvARB\0"
   "\0"
   /* _mesa_function_pool[2522]: DeleteVertexArraysAPPLE (will be remapped) */
   "ip\0"
   "glDeleteVertexArrays\0"
   "glDeleteVertexArraysAPPLE\0"
   "\0"
   /* _mesa_function_pool[2573]: ReadInstrumentsSGIX (dynamic) */
   "i\0"
   "glReadInstrumentsSGIX\0"
   "\0"
   /* _mesa_function_pool[2598]: CallLists (offset 3) */
   "iip\0"
   "glCallLists\0"
   "\0"
   /* _mesa_function_pool[2615]: UniformMatrix2x4fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix2x4fv\0"
   "\0"
   /* _mesa_function_pool[2642]: Color4ubVertex3fvSUN (dynamic) */
   "pp\0"
   "glColor4ubVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[2669]: Normal3iv (offset 59) */
   "p\0"
   "glNormal3iv\0"
   "\0"
   /* _mesa_function_pool[2684]: PassThrough (offset 199) */
   "f\0"
   "glPassThrough\0"
   "\0"
   /* _mesa_function_pool[2701]: TexParameterIivEXT (will be remapped) */
   "iip\0"
   "glTexParameterIivEXT\0"
   "\0"
   /* _mesa_function_pool[2727]: FramebufferTextureLayerEXT (will be remapped) */
   "iiiii\0"
   "glFramebufferTextureLayer\0"
   "glFramebufferTextureLayerEXT\0"
   "\0"
   /* _mesa_function_pool[2789]: GetListParameterfvSGIX (dynamic) */
   "iip\0"
   "glGetListParameterfvSGIX\0"
   "\0"
   /* _mesa_function_pool[2819]: Viewport (offset 305) */
   "iiii\0"
   "glViewport\0"
   "\0"
   /* _mesa_function_pool[2836]: VertexAttrib4NusvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Nusv\0"
   "glVertexAttrib4NusvARB\0"
   "\0"
   /* _mesa_function_pool[2883]: WindowPos4svMESA (will be remapped) */
   "p\0"
   "glWindowPos4svMESA\0"
   "\0"
   /* _mesa_function_pool[2905]: CreateProgramObjectARB (will be remapped) */
   "\0"
   "glCreateProgramObjectARB\0"
   "\0"
   /* _mesa_function_pool[2932]: DeleteTransformFeedbacks (will be remapped) */
   "ip\0"
   "glDeleteTransformFeedbacks\0"
   "\0"
   /* _mesa_function_pool[2963]: UniformMatrix4x3fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix4x3fv\0"
   "\0"
   /* _mesa_function_pool[2990]: PrioritizeTextures (offset 331) */
   "ipp\0"
   "glPrioritizeTextures\0"
   "glPrioritizeTexturesEXT\0"
   "\0"
   /* _mesa_function_pool[3040]: AsyncMarkerSGIX (dynamic) */
   "i\0"
   "glAsyncMarkerSGIX\0"
   "\0"
   /* _mesa_function_pool[3061]: GlobalAlphaFactorubSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactorubSUN\0"
   "\0"
   /* _mesa_function_pool[3089]: ClearColorIuiEXT (will be remapped) */
   "iiii\0"
   "glClearColorIuiEXT\0"
   "\0"
   /* _mesa_function_pool[3114]: ClearDebugLogMESA (dynamic) */
   "iii\0"
   "glClearDebugLogMESA\0"
   "\0"
   /* _mesa_function_pool[3139]: ResetHistogram (offset 369) */
   "i\0"
   "glResetHistogram\0"
   "glResetHistogramEXT\0"
   "\0"
   /* _mesa_function_pool[3179]: GetProgramNamedParameterfvNV (will be remapped) */
   "iipp\0"
   "glGetProgramNamedParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[3216]: PointParameterfEXT (will be remapped) */
   "if\0"
   "glPointParameterf\0"
   "glPointParameterfARB\0"
   "glPointParameterfEXT\0"
   "glPointParameterfSGIS\0"
   "\0"
   /* _mesa_function_pool[3302]: LoadIdentityDeformationMapSGIX (dynamic) */
   "i\0"
   "glLoadIdentityDeformationMapSGIX\0"
   "\0"
   /* _mesa_function_pool[3338]: GenFencesNV (will be remapped) */
   "ip\0"
   "glGenFencesNV\0"
   "\0"
   /* _mesa_function_pool[3356]: ImageTransformParameterfHP (dynamic) */
   "iif\0"
   "glImageTransformParameterfHP\0"
   "\0"
   /* _mesa_function_pool[3390]: MatrixIndexusvARB (dynamic) */
   "ip\0"
   "glMatrixIndexusvARB\0"
   "\0"
   /* _mesa_function_pool[3414]: DrawElementsBaseVertex (will be remapped) */
   "iiipi\0"
   "glDrawElementsBaseVertex\0"
   "\0"
   /* _mesa_function_pool[3446]: DisableVertexAttribArrayARB (will be remapped) */
   "i\0"
   "glDisableVertexAttribArray\0"
   "glDisableVertexAttribArrayARB\0"
   "\0"
   /* _mesa_function_pool[3506]: TexCoord2sv (offset 109) */
   "p\0"
   "glTexCoord2sv\0"
   "\0"
   /* _mesa_function_pool[3523]: Vertex4dv (offset 143) */
   "p\0"
   "glVertex4dv\0"
   "\0"
   /* _mesa_function_pool[3538]: StencilMaskSeparate (will be remapped) */
   "ii\0"
   "glStencilMaskSeparate\0"
   "\0"
   /* _mesa_function_pool[3564]: ProgramLocalParameter4dARB (will be remapped) */
   "iidddd\0"
   "glProgramLocalParameter4dARB\0"
   "\0"
   /* _mesa_function_pool[3601]: CompressedTexImage3DARB (will be remapped) */
   "iiiiiiiip\0"
   "glCompressedTexImage3D\0"
   "glCompressedTexImage3DARB\0"
   "\0"
   /* _mesa_function_pool[3661]: Color3sv (offset 18) */
   "p\0"
   "glColor3sv\0"
   "\0"
   /* _mesa_function_pool[3675]: GetConvolutionParameteriv (offset 358) */
   "iip\0"
   "glGetConvolutionParameteriv\0"
   "glGetConvolutionParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[3739]: VertexAttrib1fARB (will be remapped) */
   "if\0"
   "glVertexAttrib1f\0"
   "glVertexAttrib1fARB\0"
   "\0"
   /* _mesa_function_pool[3780]: Vertex2dv (offset 127) */
   "p\0"
   "glVertex2dv\0"
   "\0"
   /* _mesa_function_pool[3795]: TestFenceNV (will be remapped) */
   "i\0"
   "glTestFenceNV\0"
   "\0"
   /* _mesa_function_pool[3812]: MultiTexCoord1fvARB (offset 379) */
   "ip\0"
   "glMultiTexCoord1fv\0"
   "glMultiTexCoord1fvARB\0"
   "\0"
   /* _mesa_function_pool[3857]: TexCoord3iv (offset 115) */
   "p\0"
   "glTexCoord3iv\0"
   "\0"
   /* _mesa_function_pool[3874]: ColorFragmentOp2ATI (will be remapped) */
   "iiiiiiiiii\0"
   "glColorFragmentOp2ATI\0"
   "\0"
   /* _mesa_function_pool[3908]: SecondaryColorPointerListIBM (dynamic) */
   "iiipi\0"
   "glSecondaryColorPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[3946]: GetPixelTexGenParameterivSGIS (will be remapped) */
   "ip\0"
   "glGetPixelTexGenParameterivSGIS\0"
   "\0"
   /* _mesa_function_pool[3982]: Color3fv (offset 14) */
   "p\0"
   "glColor3fv\0"
   "\0"
   /* _mesa_function_pool[3996]: VertexAttrib4fNV (will be remapped) */
   "iffff\0"
   "glVertexAttrib4fNV\0"
   "\0"
   /* _mesa_function_pool[4022]: ReplacementCodeubSUN (dynamic) */
   "i\0"
   "glReplacementCodeubSUN\0"
   "\0"
   /* _mesa_function_pool[4048]: FinishAsyncSGIX (dynamic) */
   "p\0"
   "glFinishAsyncSGIX\0"
   "\0"
   /* _mesa_function_pool[4069]: GetDebugLogMESA (dynamic) */
   "iiiipp\0"
   "glGetDebugLogMESA\0"
   "\0"
   /* _mesa_function_pool[4095]: FogCoorddEXT (will be remapped) */
   "d\0"
   "glFogCoordd\0"
   "glFogCoorddEXT\0"
   "\0"
   /* _mesa_function_pool[4125]: BeginConditionalRenderNV (will be remapped) */
   "ii\0"
   "glBeginConditionalRenderNV\0"
   "\0"
   /* _mesa_function_pool[4156]: Color4ubVertex3fSUN (dynamic) */
   "iiiifff\0"
   "glColor4ubVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[4187]: FogCoordfEXT (will be remapped) */
   "f\0"
   "glFogCoordf\0"
   "glFogCoordfEXT\0"
   "\0"
   /* _mesa_function_pool[4217]: PointSize (offset 173) */
   "f\0"
   "glPointSize\0"
   "\0"
   /* _mesa_function_pool[4232]: TexCoord2fVertex3fSUN (dynamic) */
   "fffff\0"
   "glTexCoord2fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[4263]: PopName (offset 200) */
   "\0"
   "glPopName\0"
   "\0"
   /* _mesa_function_pool[4275]: GlobalAlphaFactoriSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactoriSUN\0"
   "\0"
   /* _mesa_function_pool[4302]: VertexAttrib2dNV (will be remapped) */
   "idd\0"
   "glVertexAttrib2dNV\0"
   "\0"
   /* _mesa_function_pool[4326]: GetProgramInfoLog (will be remapped) */
   "iipp\0"
   "glGetProgramInfoLog\0"
   "\0"
   /* _mesa_function_pool[4352]: VertexAttrib4NbvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Nbv\0"
   "glVertexAttrib4NbvARB\0"
   "\0"
   /* _mesa_function_pool[4397]: GetActiveAttribARB (will be remapped) */
   "iiipppp\0"
   "glGetActiveAttrib\0"
   "glGetActiveAttribARB\0"
   "\0"
   /* _mesa_function_pool[4445]: Vertex4sv (offset 149) */
   "p\0"
   "glVertex4sv\0"
   "\0"
   /* _mesa_function_pool[4460]: VertexAttrib4ubNV (will be remapped) */
   "iiiii\0"
   "glVertexAttrib4ubNV\0"
   "\0"
   /* _mesa_function_pool[4487]: TextureRangeAPPLE (will be remapped) */
   "iip\0"
   "glTextureRangeAPPLE\0"
   "\0"
   /* _mesa_function_pool[4512]: GetTexEnvfv (offset 276) */
   "iip\0"
   "glGetTexEnvfv\0"
   "\0"
   /* _mesa_function_pool[4531]: BindTransformFeedback (will be remapped) */
   "ii\0"
   "glBindTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[4559]: TexCoord2fColor4fNormal3fVertex3fSUN (dynamic) */
   "ffffffffffff\0"
   "glTexCoord2fColor4fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[4612]: Indexub (offset 315) */
   "i\0"
   "glIndexub\0"
   "\0"
   /* _mesa_function_pool[4625]: TexEnvi (offset 186) */
   "iii\0"
   "glTexEnvi\0"
   "\0"
   /* _mesa_function_pool[4640]: GetClipPlane (offset 259) */
   "ip\0"
   "glGetClipPlane\0"
   "\0"
   /* _mesa_function_pool[4659]: CombinerParameterfvNV (will be remapped) */
   "ip\0"
   "glCombinerParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[4687]: VertexAttribs3dvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs3dvNV\0"
   "\0"
   /* _mesa_function_pool[4713]: VertexAttribs4fvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs4fvNV\0"
   "\0"
   /* _mesa_function_pool[4739]: VertexArrayRangeNV (will be remapped) */
   "ip\0"
   "glVertexArrayRangeNV\0"
   "\0"
   /* _mesa_function_pool[4764]: FragmentLightiSGIX (dynamic) */
   "iii\0"
   "glFragmentLightiSGIX\0"
   "\0"
   /* _mesa_function_pool[4790]: PolygonOffsetEXT (will be remapped) */
   "ff\0"
   "glPolygonOffsetEXT\0"
   "\0"
   /* _mesa_function_pool[4813]: PollAsyncSGIX (dynamic) */
   "p\0"
   "glPollAsyncSGIX\0"
   "\0"
   /* _mesa_function_pool[4832]: DeleteFragmentShaderATI (will be remapped) */
   "i\0"
   "glDeleteFragmentShaderATI\0"
   "\0"
   /* _mesa_function_pool[4861]: Scaled (offset 301) */
   "ddd\0"
   "glScaled\0"
   "\0"
   /* _mesa_function_pool[4875]: ResumeTransformFeedback (will be remapped) */
   "\0"
   "glResumeTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[4903]: Scalef (offset 302) */
   "fff\0"
   "glScalef\0"
   "\0"
   /* _mesa_function_pool[4917]: TexCoord2fNormal3fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glTexCoord2fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[4955]: MultTransposeMatrixdARB (will be remapped) */
   "p\0"
   "glMultTransposeMatrixd\0"
   "glMultTransposeMatrixdARB\0"
   "\0"
   /* _mesa_function_pool[5007]: ColorMaskIndexedEXT (will be remapped) */
   "iiiii\0"
   "glColorMaskIndexedEXT\0"
   "\0"
   /* _mesa_function_pool[5036]: ObjectUnpurgeableAPPLE (will be remapped) */
   "iii\0"
   "glObjectUnpurgeableAPPLE\0"
   "\0"
   /* _mesa_function_pool[5066]: AlphaFunc (offset 240) */
   "if\0"
   "glAlphaFunc\0"
   "\0"
   /* _mesa_function_pool[5082]: WindowPos2svMESA (will be remapped) */
   "p\0"
   "glWindowPos2sv\0"
   "glWindowPos2svARB\0"
   "glWindowPos2svMESA\0"
   "\0"
   /* _mesa_function_pool[5137]: EdgeFlag (offset 41) */
   "i\0"
   "glEdgeFlag\0"
   "\0"
   /* _mesa_function_pool[5151]: TexCoord2iv (offset 107) */
   "p\0"
   "glTexCoord2iv\0"
   "\0"
   /* _mesa_function_pool[5168]: CompressedTexImage1DARB (will be remapped) */
   "iiiiiip\0"
   "glCompressedTexImage1D\0"
   "glCompressedTexImage1DARB\0"
   "\0"
   /* _mesa_function_pool[5226]: Rotated (offset 299) */
   "dddd\0"
   "glRotated\0"
   "\0"
   /* _mesa_function_pool[5242]: GetTexParameterIuivEXT (will be remapped) */
   "iip\0"
   "glGetTexParameterIuivEXT\0"
   "\0"
   /* _mesa_function_pool[5272]: VertexAttrib2sNV (will be remapped) */
   "iii\0"
   "glVertexAttrib2sNV\0"
   "\0"
   /* _mesa_function_pool[5296]: ReadPixels (offset 256) */
   "iiiiiip\0"
   "glReadPixels\0"
   "\0"
   /* _mesa_function_pool[5318]: EdgeFlagv (offset 42) */
   "p\0"
   "glEdgeFlagv\0"
   "\0"
   /* _mesa_function_pool[5333]: NormalPointerListIBM (dynamic) */
   "iipi\0"
   "glNormalPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[5362]: IndexPointerEXT (will be remapped) */
   "iiip\0"
   "glIndexPointerEXT\0"
   "\0"
   /* _mesa_function_pool[5386]: Color4iv (offset 32) */
   "p\0"
   "glColor4iv\0"
   "\0"
   /* _mesa_function_pool[5400]: TexParameterf (offset 178) */
   "iif\0"
   "glTexParameterf\0"
   "\0"
   /* _mesa_function_pool[5421]: TexParameteri (offset 180) */
   "iii\0"
   "glTexParameteri\0"
   "\0"
   /* _mesa_function_pool[5442]: NormalPointerEXT (will be remapped) */
   "iiip\0"
   "glNormalPointerEXT\0"
   "\0"
   /* _mesa_function_pool[5467]: MultiTexCoord3dARB (offset 392) */
   "iddd\0"
   "glMultiTexCoord3d\0"
   "glMultiTexCoord3dARB\0"
   "\0"
   /* _mesa_function_pool[5512]: MultiTexCoord2iARB (offset 388) */
   "iii\0"
   "glMultiTexCoord2i\0"
   "glMultiTexCoord2iARB\0"
   "\0"
   /* _mesa_function_pool[5556]: DrawPixels (offset 257) */
   "iiiip\0"
   "glDrawPixels\0"
   "\0"
   /* _mesa_function_pool[5576]: ReplacementCodeuiTexCoord2fNormal3fVertex3fSUN (dynamic) */
   "iffffffff\0"
   "glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[5636]: MultiTexCoord2svARB (offset 391) */
   "ip\0"
   "glMultiTexCoord2sv\0"
   "glMultiTexCoord2svARB\0"
   "\0"
   /* _mesa_function_pool[5681]: ReplacementCodeubvSUN (dynamic) */
   "p\0"
   "glReplacementCodeubvSUN\0"
   "\0"
   /* _mesa_function_pool[5708]: Uniform3iARB (will be remapped) */
   "iiii\0"
   "glUniform3i\0"
   "glUniform3iARB\0"
   "\0"
   /* _mesa_function_pool[5741]: DrawTransformFeedback (will be remapped) */
   "ii\0"
   "glDrawTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[5769]: GetFragmentMaterialfvSGIX (dynamic) */
   "iip\0"
   "glGetFragmentMaterialfvSGIX\0"
   "\0"
   /* _mesa_function_pool[5802]: GetShaderInfoLog (will be remapped) */
   "iipp\0"
   "glGetShaderInfoLog\0"
   "\0"
   /* _mesa_function_pool[5827]: WeightivARB (dynamic) */
   "ip\0"
   "glWeightivARB\0"
   "\0"
   /* _mesa_function_pool[5845]: PollInstrumentsSGIX (dynamic) */
   "p\0"
   "glPollInstrumentsSGIX\0"
   "\0"
   /* _mesa_function_pool[5870]: GlobalAlphaFactordSUN (dynamic) */
   "d\0"
   "glGlobalAlphaFactordSUN\0"
   "\0"
   /* _mesa_function_pool[5897]: GetFinalCombinerInputParameterfvNV (will be remapped) */
   "iip\0"
   "glGetFinalCombinerInputParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[5939]: GenerateMipmapEXT (will be remapped) */
   "i\0"
   "glGenerateMipmap\0"
   "glGenerateMipmapEXT\0"
   "\0"
   /* _mesa_function_pool[5979]: GenLists (offset 5) */
   "i\0"
   "glGenLists\0"
   "\0"
   /* _mesa_function_pool[5993]: SetFragmentShaderConstantATI (will be remapped) */
   "ip\0"
   "glSetFragmentShaderConstantATI\0"
   "\0"
   /* _mesa_function_pool[6028]: GetMapAttribParameterivNV (dynamic) */
   "iiip\0"
   "glGetMapAttribParameterivNV\0"
   "\0"
   /* _mesa_function_pool[6062]: CreateShaderObjectARB (will be remapped) */
   "i\0"
   "glCreateShaderObjectARB\0"
   "\0"
   /* _mesa_function_pool[6089]: GetSharpenTexFuncSGIS (dynamic) */
   "ip\0"
   "glGetSharpenTexFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[6117]: BufferDataARB (will be remapped) */
   "iipi\0"
   "glBufferData\0"
   "glBufferDataARB\0"
   "\0"
   /* _mesa_function_pool[6152]: FlushVertexArrayRangeNV (will be remapped) */
   "\0"
   "glFlushVertexArrayRangeNV\0"
   "\0"
   /* _mesa_function_pool[6180]: MapGrid2d (offset 226) */
   "iddidd\0"
   "glMapGrid2d\0"
   "\0"
   /* _mesa_function_pool[6200]: MapGrid2f (offset 227) */
   "iffiff\0"
   "glMapGrid2f\0"
   "\0"
   /* _mesa_function_pool[6220]: SampleMapATI (will be remapped) */
   "iii\0"
   "glSampleMapATI\0"
   "\0"
   /* _mesa_function_pool[6240]: VertexPointerEXT (will be remapped) */
   "iiiip\0"
   "glVertexPointerEXT\0"
   "\0"
   /* _mesa_function_pool[6266]: GetTexFilterFuncSGIS (dynamic) */
   "iip\0"
   "glGetTexFilterFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[6294]: Scissor (offset 176) */
   "iiii\0"
   "glScissor\0"
   "\0"
   /* _mesa_function_pool[6310]: Fogf (offset 153) */
   "if\0"
   "glFogf\0"
   "\0"
   /* _mesa_function_pool[6321]: ReplacementCodeuiColor4ubVertex3fvSUN (dynamic) */
   "ppp\0"
   "glReplacementCodeuiColor4ubVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[6366]: TexSubImage1D (offset 332) */
   "iiiiiip\0"
   "glTexSubImage1D\0"
   "glTexSubImage1DEXT\0"
   "\0"
   /* _mesa_function_pool[6410]: VertexAttrib1sARB (will be remapped) */
   "ii\0"
   "glVertexAttrib1s\0"
   "glVertexAttrib1sARB\0"
   "\0"
   /* _mesa_function_pool[6451]: FenceSync (will be remapped) */
   "ii\0"
   "glFenceSync\0"
   "\0"
   /* _mesa_function_pool[6467]: Color4usv (offset 40) */
   "p\0"
   "glColor4usv\0"
   "\0"
   /* _mesa_function_pool[6482]: Fogi (offset 155) */
   "ii\0"
   "glFogi\0"
   "\0"
   /* _mesa_function_pool[6493]: DepthRange (offset 288) */
   "dd\0"
   "glDepthRange\0"
   "\0"
   /* _mesa_function_pool[6510]: RasterPos3iv (offset 75) */
   "p\0"
   "glRasterPos3iv\0"
   "\0"
   /* _mesa_function_pool[6528]: FinalCombinerInputNV (will be remapped) */
   "iiii\0"
   "glFinalCombinerInputNV\0"
   "\0"
   /* _mesa_function_pool[6557]: TexCoord2i (offset 106) */
   "ii\0"
   "glTexCoord2i\0"
   "\0"
   /* _mesa_function_pool[6574]: PixelMapfv (offset 251) */
   "iip\0"
   "glPixelMapfv\0"
   "\0"
   /* _mesa_function_pool[6592]: Color4ui (offset 37) */
   "iiii\0"
   "glColor4ui\0"
   "\0"
   /* _mesa_function_pool[6609]: RasterPos3s (offset 76) */
   "iii\0"
   "glRasterPos3s\0"
   "\0"
   /* _mesa_function_pool[6628]: Color3usv (offset 24) */
   "p\0"
   "glColor3usv\0"
   "\0"
   /* _mesa_function_pool[6643]: FlushRasterSGIX (dynamic) */
   "\0"
   "glFlushRasterSGIX\0"
   "\0"
   /* _mesa_function_pool[6663]: TexCoord2f (offset 104) */
   "ff\0"
   "glTexCoord2f\0"
   "\0"
   /* _mesa_function_pool[6680]: ReplacementCodeuiTexCoord2fVertex3fSUN (dynamic) */
   "ifffff\0"
   "glReplacementCodeuiTexCoord2fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[6729]: TexCoord2d (offset 102) */
   "dd\0"
   "glTexCoord2d\0"
   "\0"
   /* _mesa_function_pool[6746]: RasterPos3d (offset 70) */
   "ddd\0"
   "glRasterPos3d\0"
   "\0"
   /* _mesa_function_pool[6765]: RasterPos3f (offset 72) */
   "fff\0"
   "glRasterPos3f\0"
   "\0"
   /* _mesa_function_pool[6784]: Uniform1fARB (will be remapped) */
   "if\0"
   "glUniform1f\0"
   "glUniform1fARB\0"
   "\0"
   /* _mesa_function_pool[6815]: AreTexturesResident (offset 322) */
   "ipp\0"
   "glAreTexturesResident\0"
   "glAreTexturesResidentEXT\0"
   "\0"
   /* _mesa_function_pool[6867]: TexCoord2s (offset 108) */
   "ii\0"
   "glTexCoord2s\0"
   "\0"
   /* _mesa_function_pool[6884]: StencilOpSeparate (will be remapped) */
   "iiii\0"
   "glStencilOpSeparate\0"
   "glStencilOpSeparateATI\0"
   "\0"
   /* _mesa_function_pool[6933]: ColorTableParameteriv (offset 341) */
   "iip\0"
   "glColorTableParameteriv\0"
   "glColorTableParameterivSGI\0"
   "\0"
   /* _mesa_function_pool[6989]: FogCoordPointerListIBM (dynamic) */
   "iipi\0"
   "glFogCoordPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[7020]: WindowPos3dMESA (will be remapped) */
   "ddd\0"
   "glWindowPos3d\0"
   "glWindowPos3dARB\0"
   "glWindowPos3dMESA\0"
   "\0"
   /* _mesa_function_pool[7074]: Color4us (offset 39) */
   "iiii\0"
   "glColor4us\0"
   "\0"
   /* _mesa_function_pool[7091]: PointParameterfvEXT (will be remapped) */
   "ip\0"
   "glPointParameterfv\0"
   "glPointParameterfvARB\0"
   "glPointParameterfvEXT\0"
   "glPointParameterfvSGIS\0"
   "\0"
   /* _mesa_function_pool[7181]: Color3bv (offset 10) */
   "p\0"
   "glColor3bv\0"
   "\0"
   /* _mesa_function_pool[7195]: WindowPos2fvMESA (will be remapped) */
   "p\0"
   "glWindowPos2fv\0"
   "glWindowPos2fvARB\0"
   "glWindowPos2fvMESA\0"
   "\0"
   /* _mesa_function_pool[7250]: SecondaryColor3bvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3bv\0"
   "glSecondaryColor3bvEXT\0"
   "\0"
   /* _mesa_function_pool[7296]: VertexPointerListIBM (dynamic) */
   "iiipi\0"
   "glVertexPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[7326]: GetProgramLocalParameterfvARB (will be remapped) */
   "iip\0"
   "glGetProgramLocalParameterfvARB\0"
   "\0"
   /* _mesa_function_pool[7363]: FragmentMaterialfSGIX (dynamic) */
   "iif\0"
   "glFragmentMaterialfSGIX\0"
   "\0"
   /* _mesa_function_pool[7392]: TexCoord2fNormal3fVertex3fSUN (dynamic) */
   "ffffffff\0"
   "glTexCoord2fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[7434]: RenderbufferStorageEXT (will be remapped) */
   "iiii\0"
   "glRenderbufferStorage\0"
   "glRenderbufferStorageEXT\0"
   "\0"
   /* _mesa_function_pool[7487]: IsFenceNV (will be remapped) */
   "i\0"
   "glIsFenceNV\0"
   "\0"
   /* _mesa_function_pool[7502]: AttachObjectARB (will be remapped) */
   "ii\0"
   "glAttachObjectARB\0"
   "\0"
   /* _mesa_function_pool[7524]: GetFragmentLightivSGIX (dynamic) */
   "iip\0"
   "glGetFragmentLightivSGIX\0"
   "\0"
   /* _mesa_function_pool[7554]: UniformMatrix2fvARB (will be remapped) */
   "iiip\0"
   "glUniformMatrix2fv\0"
   "glUniformMatrix2fvARB\0"
   "\0"
   /* _mesa_function_pool[7601]: MultiTexCoord2fARB (offset 386) */
   "iff\0"
   "glMultiTexCoord2f\0"
   "glMultiTexCoord2fARB\0"
   "\0"
   /* _mesa_function_pool[7645]: ColorTable (offset 339) */
   "iiiiip\0"
   "glColorTable\0"
   "glColorTableSGI\0"
   "glColorTableEXT\0"
   "\0"
   /* _mesa_function_pool[7698]: IndexPointer (offset 314) */
   "iip\0"
   "glIndexPointer\0"
   "\0"
   /* _mesa_function_pool[7718]: Accum (offset 213) */
   "if\0"
   "glAccum\0"
   "\0"
   /* _mesa_function_pool[7730]: GetTexImage (offset 281) */
   "iiiip\0"
   "glGetTexImage\0"
   "\0"
   /* _mesa_function_pool[7751]: MapControlPointsNV (dynamic) */
   "iiiiiiiip\0"
   "glMapControlPointsNV\0"
   "\0"
   /* _mesa_function_pool[7783]: ConvolutionFilter2D (offset 349) */
   "iiiiiip\0"
   "glConvolutionFilter2D\0"
   "glConvolutionFilter2DEXT\0"
   "\0"
   /* _mesa_function_pool[7839]: Finish (offset 216) */
   "\0"
   "glFinish\0"
   "\0"
   /* _mesa_function_pool[7850]: MapParameterfvNV (dynamic) */
   "iip\0"
   "glMapParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[7874]: ClearStencil (offset 207) */
   "i\0"
   "glClearStencil\0"
   "\0"
   /* _mesa_function_pool[7892]: VertexAttrib3dvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib3dv\0"
   "glVertexAttrib3dvARB\0"
   "\0"
   /* _mesa_function_pool[7935]: HintPGI (dynamic) */
   "ii\0"
   "glHintPGI\0"
   "\0"
   /* _mesa_function_pool[7949]: ConvolutionParameteriv (offset 353) */
   "iip\0"
   "glConvolutionParameteriv\0"
   "glConvolutionParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[8007]: Color4s (offset 33) */
   "iiii\0"
   "glColor4s\0"
   "\0"
   /* _mesa_function_pool[8023]: InterleavedArrays (offset 317) */
   "iip\0"
   "glInterleavedArrays\0"
   "\0"
   /* _mesa_function_pool[8048]: RasterPos2fv (offset 65) */
   "p\0"
   "glRasterPos2fv\0"
   "\0"
   /* _mesa_function_pool[8066]: TexCoord1fv (offset 97) */
   "p\0"
   "glTexCoord1fv\0"
   "\0"
   /* _mesa_function_pool[8083]: Vertex2d (offset 126) */
   "dd\0"
   "glVertex2d\0"
   "\0"
   /* _mesa_function_pool[8098]: CullParameterdvEXT (dynamic) */
   "ip\0"
   "glCullParameterdvEXT\0"
   "\0"
   /* _mesa_function_pool[8123]: ProgramNamedParameter4fNV (will be remapped) */
   "iipffff\0"
   "glProgramNamedParameter4fNV\0"
   "\0"
   /* _mesa_function_pool[8160]: Color3fVertex3fSUN (dynamic) */
   "ffffff\0"
   "glColor3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[8189]: ProgramEnvParameter4fvARB (will be remapped) */
   "iip\0"
   "glProgramEnvParameter4fvARB\0"
   "glProgramParameter4fvNV\0"
   "\0"
   /* _mesa_function_pool[8246]: Color4i (offset 31) */
   "iiii\0"
   "glColor4i\0"
   "\0"
   /* _mesa_function_pool[8262]: Color4f (offset 29) */
   "ffff\0"
   "glColor4f\0"
   "\0"
   /* _mesa_function_pool[8278]: RasterPos4fv (offset 81) */
   "p\0"
   "glRasterPos4fv\0"
   "\0"
   /* _mesa_function_pool[8296]: Color4d (offset 27) */
   "dddd\0"
   "glColor4d\0"
   "\0"
   /* _mesa_function_pool[8312]: ClearIndex (offset 205) */
   "f\0"
   "glClearIndex\0"
   "\0"
   /* _mesa_function_pool[8328]: Color4b (offset 25) */
   "iiii\0"
   "glColor4b\0"
   "\0"
   /* _mesa_function_pool[8344]: LoadMatrixd (offset 292) */
   "p\0"
   "glLoadMatrixd\0"
   "\0"
   /* _mesa_function_pool[8361]: FragmentLightModeliSGIX (dynamic) */
   "ii\0"
   "glFragmentLightModeliSGIX\0"
   "\0"
   /* _mesa_function_pool[8391]: RasterPos2dv (offset 63) */
   "p\0"
   "glRasterPos2dv\0"
   "\0"
   /* _mesa_function_pool[8409]: ConvolutionParameterfv (offset 351) */
   "iip\0"
   "glConvolutionParameterfv\0"
   "glConvolutionParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[8467]: TbufferMask3DFX (dynamic) */
   "i\0"
   "glTbufferMask3DFX\0"
   "\0"
   /* _mesa_function_pool[8488]: GetTexGendv (offset 278) */
   "iip\0"
   "glGetTexGendv\0"
   "\0"
   /* _mesa_function_pool[8507]: GetVertexAttribfvNV (will be remapped) */
   "iip\0"
   "glGetVertexAttribfvNV\0"
   "\0"
   /* _mesa_function_pool[8534]: BeginTransformFeedbackEXT (will be remapped) */
   "i\0"
   "glBeginTransformFeedbackEXT\0"
   "glBeginTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[8590]: LoadProgramNV (will be remapped) */
   "iiip\0"
   "glLoadProgramNV\0"
   "\0"
   /* _mesa_function_pool[8612]: WaitSync (will be remapped) */
   "iii\0"
   "glWaitSync\0"
   "\0"
   /* _mesa_function_pool[8628]: EndList (offset 1) */
   "\0"
   "glEndList\0"
   "\0"
   /* _mesa_function_pool[8640]: VertexAttrib4fvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib4fvNV\0"
   "\0"
   /* _mesa_function_pool[8664]: GetAttachedObjectsARB (will be remapped) */
   "iipp\0"
   "glGetAttachedObjectsARB\0"
   "\0"
   /* _mesa_function_pool[8694]: Uniform3fvARB (will be remapped) */
   "iip\0"
   "glUniform3fv\0"
   "glUniform3fvARB\0"
   "\0"
   /* _mesa_function_pool[8728]: EvalCoord1fv (offset 231) */
   "p\0"
   "glEvalCoord1fv\0"
   "\0"
   /* _mesa_function_pool[8746]: DrawRangeElements (offset 338) */
   "iiiiip\0"
   "glDrawRangeElements\0"
   "glDrawRangeElementsEXT\0"
   "\0"
   /* _mesa_function_pool[8797]: EvalMesh2 (offset 238) */
   "iiiii\0"
   "glEvalMesh2\0"
   "\0"
   /* _mesa_function_pool[8816]: Vertex4fv (offset 145) */
   "p\0"
   "glVertex4fv\0"
   "\0"
   /* _mesa_function_pool[8831]: GenTransformFeedbacks (will be remapped) */
   "ip\0"
   "glGenTransformFeedbacks\0"
   "\0"
   /* _mesa_function_pool[8859]: SpriteParameterfvSGIX (dynamic) */
   "ip\0"
   "glSpriteParameterfvSGIX\0"
   "\0"
   /* _mesa_function_pool[8887]: CheckFramebufferStatusEXT (will be remapped) */
   "i\0"
   "glCheckFramebufferStatus\0"
   "glCheckFramebufferStatusEXT\0"
   "\0"
   /* _mesa_function_pool[8943]: GlobalAlphaFactoruiSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactoruiSUN\0"
   "\0"
   /* _mesa_function_pool[8971]: GetHandleARB (will be remapped) */
   "i\0"
   "glGetHandleARB\0"
   "\0"
   /* _mesa_function_pool[8989]: GetVertexAttribivARB (will be remapped) */
   "iip\0"
   "glGetVertexAttribiv\0"
   "glGetVertexAttribivARB\0"
   "\0"
   /* _mesa_function_pool[9037]: GetCombinerInputParameterfvNV (will be remapped) */
   "iiiip\0"
   "glGetCombinerInputParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[9076]: GetTexParameterIivEXT (will be remapped) */
   "iip\0"
   "glGetTexParameterIivEXT\0"
   "\0"
   /* _mesa_function_pool[9105]: CreateProgram (will be remapped) */
   "\0"
   "glCreateProgram\0"
   "\0"
   /* _mesa_function_pool[9123]: LoadTransposeMatrixdARB (will be remapped) */
   "p\0"
   "glLoadTransposeMatrixd\0"
   "glLoadTransposeMatrixdARB\0"
   "\0"
   /* _mesa_function_pool[9175]: GetMinmax (offset 364) */
   "iiiip\0"
   "glGetMinmax\0"
   "glGetMinmaxEXT\0"
   "\0"
   /* _mesa_function_pool[9209]: StencilFuncSeparate (will be remapped) */
   "iiii\0"
   "glStencilFuncSeparate\0"
   "\0"
   /* _mesa_function_pool[9237]: SecondaryColor3sEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3s\0"
   "glSecondaryColor3sEXT\0"
   "\0"
   /* _mesa_function_pool[9283]: Color3fVertex3fvSUN (dynamic) */
   "pp\0"
   "glColor3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[9309]: Normal3fv (offset 57) */
   "p\0"
   "glNormal3fv\0"
   "\0"
   /* _mesa_function_pool[9324]: GlobalAlphaFactorbSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactorbSUN\0"
   "\0"
   /* _mesa_function_pool[9351]: Color3us (offset 23) */
   "iii\0"
   "glColor3us\0"
   "\0"
   /* _mesa_function_pool[9367]: ImageTransformParameterfvHP (dynamic) */
   "iip\0"
   "glImageTransformParameterfvHP\0"
   "\0"
   /* _mesa_function_pool[9402]: VertexAttrib4ivARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4iv\0"
   "glVertexAttrib4ivARB\0"
   "\0"
   /* _mesa_function_pool[9445]: End (offset 43) */
   "\0"
   "glEnd\0"
   "\0"
   /* _mesa_function_pool[9453]: VertexAttrib3fNV (will be remapped) */
   "ifff\0"
   "glVertexAttrib3fNV\0"
   "\0"
   /* _mesa_function_pool[9478]: VertexAttribs2dvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs2dvNV\0"
   "\0"
   /* _mesa_function_pool[9504]: GetQueryObjectui64vEXT (will be remapped) */
   "iip\0"
   "glGetQueryObjectui64vEXT\0"
   "\0"
   /* _mesa_function_pool[9534]: MultiTexCoord3fvARB (offset 395) */
   "ip\0"
   "glMultiTexCoord3fv\0"
   "glMultiTexCoord3fvARB\0"
   "\0"
   /* _mesa_function_pool[9579]: SecondaryColor3dEXT (will be remapped) */
   "ddd\0"
   "glSecondaryColor3d\0"
   "glSecondaryColor3dEXT\0"
   "\0"
   /* _mesa_function_pool[9625]: Color3ub (offset 19) */
   "iii\0"
   "glColor3ub\0"
   "\0"
   /* _mesa_function_pool[9641]: GetProgramParameterfvNV (will be remapped) */
   "iiip\0"
   "glGetProgramParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[9673]: TangentPointerEXT (dynamic) */
   "iip\0"
   "glTangentPointerEXT\0"
   "\0"
   /* _mesa_function_pool[9698]: Color4fNormal3fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glColor4fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[9733]: GetInstrumentsSGIX (dynamic) */
   "\0"
   "glGetInstrumentsSGIX\0"
   "\0"
   /* _mesa_function_pool[9756]: Color3ui (offset 21) */
   "iii\0"
   "glColor3ui\0"
   "\0"
   /* _mesa_function_pool[9772]: EvalMapsNV (dynamic) */
   "ii\0"
   "glEvalMapsNV\0"
   "\0"
   /* _mesa_function_pool[9789]: TexSubImage2D (offset 333) */
   "iiiiiiiip\0"
   "glTexSubImage2D\0"
   "glTexSubImage2DEXT\0"
   "\0"
   /* _mesa_function_pool[9835]: FragmentLightivSGIX (dynamic) */
   "iip\0"
   "glFragmentLightivSGIX\0"
   "\0"
   /* _mesa_function_pool[9862]: GetTexParameterPointervAPPLE (will be remapped) */
   "iip\0"
   "glGetTexParameterPointervAPPLE\0"
   "\0"
   /* _mesa_function_pool[9898]: TexGenfv (offset 191) */
   "iip\0"
   "glTexGenfv\0"
   "\0"
   /* _mesa_function_pool[9914]: GetTransformFeedbackVaryingEXT (will be remapped) */
   "iiipppp\0"
   "glGetTransformFeedbackVaryingEXT\0"
   "glGetTransformFeedbackVarying\0"
   "\0"
   /* _mesa_function_pool[9986]: VertexAttrib4bvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4bv\0"
   "glVertexAttrib4bvARB\0"
   "\0"
   /* _mesa_function_pool[10029]: AlphaFragmentOp2ATI (will be remapped) */
   "iiiiiiiii\0"
   "glAlphaFragmentOp2ATI\0"
   "\0"
   /* _mesa_function_pool[10062]: GetIntegerIndexedvEXT (will be remapped) */
   "iip\0"
   "glGetIntegerIndexedvEXT\0"
   "\0"
   /* _mesa_function_pool[10091]: MultiTexCoord4sARB (offset 406) */
   "iiiii\0"
   "glMultiTexCoord4s\0"
   "glMultiTexCoord4sARB\0"
   "\0"
   /* _mesa_function_pool[10137]: GetFragmentMaterialivSGIX (dynamic) */
   "iip\0"
   "glGetFragmentMaterialivSGIX\0"
   "\0"
   /* _mesa_function_pool[10170]: WindowPos4dMESA (will be remapped) */
   "dddd\0"
   "glWindowPos4dMESA\0"
   "\0"
   /* _mesa_function_pool[10194]: WeightPointerARB (dynamic) */
   "iiip\0"
   "glWeightPointerARB\0"
   "\0"
   /* _mesa_function_pool[10219]: WindowPos2dMESA (will be remapped) */
   "dd\0"
   "glWindowPos2d\0"
   "glWindowPos2dARB\0"
   "glWindowPos2dMESA\0"
   "\0"
   /* _mesa_function_pool[10272]: FramebufferTexture3DEXT (will be remapped) */
   "iiiiii\0"
   "glFramebufferTexture3D\0"
   "glFramebufferTexture3DEXT\0"
   "\0"
   /* _mesa_function_pool[10329]: BlendEquation (offset 337) */
   "i\0"
   "glBlendEquation\0"
   "glBlendEquationEXT\0"
   "\0"
   /* _mesa_function_pool[10367]: VertexAttrib3dNV (will be remapped) */
   "iddd\0"
   "glVertexAttrib3dNV\0"
   "\0"
   /* _mesa_function_pool[10392]: VertexAttrib3dARB (will be remapped) */
   "iddd\0"
   "glVertexAttrib3d\0"
   "glVertexAttrib3dARB\0"
   "\0"
   /* _mesa_function_pool[10435]: ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN (dynamic) */
   "ppppp\0"
   "glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[10499]: VertexAttrib4fARB (will be remapped) */
   "iffff\0"
   "glVertexAttrib4f\0"
   "glVertexAttrib4fARB\0"
   "\0"
   /* _mesa_function_pool[10543]: GetError (offset 261) */
   "\0"
   "glGetError\0"
   "\0"
   /* _mesa_function_pool[10556]: IndexFuncEXT (dynamic) */
   "if\0"
   "glIndexFuncEXT\0"
   "\0"
   /* _mesa_function_pool[10575]: TexCoord3dv (offset 111) */
   "p\0"
   "glTexCoord3dv\0"
   "\0"
   /* _mesa_function_pool[10592]: Indexdv (offset 45) */
   "p\0"
   "glIndexdv\0"
   "\0"
   /* _mesa_function_pool[10605]: FramebufferTexture2DEXT (will be remapped) */
   "iiiii\0"
   "glFramebufferTexture2D\0"
   "glFramebufferTexture2DEXT\0"
   "\0"
   /* _mesa_function_pool[10661]: Normal3s (offset 60) */
   "iii\0"
   "glNormal3s\0"
   "\0"
   /* _mesa_function_pool[10677]: GetObjectParameterivAPPLE (will be remapped) */
   "iiip\0"
   "glGetObjectParameterivAPPLE\0"
   "\0"
   /* _mesa_function_pool[10711]: PushName (offset 201) */
   "i\0"
   "glPushName\0"
   "\0"
   /* _mesa_function_pool[10725]: MultiTexCoord2dvARB (offset 385) */
   "ip\0"
   "glMultiTexCoord2dv\0"
   "glMultiTexCoord2dvARB\0"
   "\0"
   /* _mesa_function_pool[10770]: CullParameterfvEXT (dynamic) */
   "ip\0"
   "glCullParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[10795]: Normal3i (offset 58) */
   "iii\0"
   "glNormal3i\0"
   "\0"
   /* _mesa_function_pool[10811]: ProgramNamedParameter4fvNV (will be remapped) */
   "iipp\0"
   "glProgramNamedParameter4fvNV\0"
   "\0"
   /* _mesa_function_pool[10846]: SecondaryColorPointerEXT (will be remapped) */
   "iiip\0"
   "glSecondaryColorPointer\0"
   "glSecondaryColorPointerEXT\0"
   "\0"
   /* _mesa_function_pool[10903]: VertexAttrib4fvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4fv\0"
   "glVertexAttrib4fvARB\0"
   "\0"
   /* _mesa_function_pool[10946]: ColorPointerListIBM (dynamic) */
   "iiipi\0"
   "glColorPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[10975]: GetActiveUniformARB (will be remapped) */
   "iiipppp\0"
   "glGetActiveUniform\0"
   "glGetActiveUniformARB\0"
   "\0"
   /* _mesa_function_pool[11025]: ImageTransformParameteriHP (dynamic) */
   "iii\0"
   "glImageTransformParameteriHP\0"
   "\0"
   /* _mesa_function_pool[11059]: Normal3b (offset 52) */
   "iii\0"
   "glNormal3b\0"
   "\0"
   /* _mesa_function_pool[11075]: Normal3d (offset 54) */
   "ddd\0"
   "glNormal3d\0"
   "\0"
   /* _mesa_function_pool[11091]: Normal3f (offset 56) */
   "fff\0"
   "glNormal3f\0"
   "\0"
   /* _mesa_function_pool[11107]: MultiTexCoord1svARB (offset 383) */
   "ip\0"
   "glMultiTexCoord1sv\0"
   "glMultiTexCoord1svARB\0"
   "\0"
   /* _mesa_function_pool[11152]: Indexi (offset 48) */
   "i\0"
   "glIndexi\0"
   "\0"
   /* _mesa_function_pool[11164]: EGLImageTargetTexture2DOES (will be remapped) */
   "ip\0"
   "glEGLImageTargetTexture2DOES\0"
   "\0"
   /* _mesa_function_pool[11197]: EndQueryARB (will be remapped) */
   "i\0"
   "glEndQuery\0"
   "glEndQueryARB\0"
   "\0"
   /* _mesa_function_pool[11225]: DeleteFencesNV (will be remapped) */
   "ip\0"
   "glDeleteFencesNV\0"
   "\0"
   /* _mesa_function_pool[11246]: BindBufferRangeEXT (will be remapped) */
   "iiiii\0"
   "glBindBufferRangeEXT\0"
   "glBindBufferRange\0"
   "\0"
   /* _mesa_function_pool[11292]: DepthMask (offset 211) */
   "i\0"
   "glDepthMask\0"
   "\0"
   /* _mesa_function_pool[11307]: IsShader (will be remapped) */
   "i\0"
   "glIsShader\0"
   "\0"
   /* _mesa_function_pool[11321]: Indexf (offset 46) */
   "f\0"
   "glIndexf\0"
   "\0"
   /* _mesa_function_pool[11333]: GetImageTransformParameterivHP (dynamic) */
   "iip\0"
   "glGetImageTransformParameterivHP\0"
   "\0"
   /* _mesa_function_pool[11371]: Indexd (offset 44) */
   "d\0"
   "glIndexd\0"
   "\0"
   /* _mesa_function_pool[11383]: GetMaterialiv (offset 270) */
   "iip\0"
   "glGetMaterialiv\0"
   "\0"
   /* _mesa_function_pool[11404]: StencilOp (offset 244) */
   "iii\0"
   "glStencilOp\0"
   "\0"
   /* _mesa_function_pool[11421]: WindowPos4ivMESA (will be remapped) */
   "p\0"
   "glWindowPos4ivMESA\0"
   "\0"
   /* _mesa_function_pool[11443]: FramebufferTextureLayer (dynamic) */
   "iiiii\0"
   "glFramebufferTextureLayerARB\0"
   "\0"
   /* _mesa_function_pool[11479]: MultiTexCoord3svARB (offset 399) */
   "ip\0"
   "glMultiTexCoord3sv\0"
   "glMultiTexCoord3svARB\0"
   "\0"
   /* _mesa_function_pool[11524]: TexEnvfv (offset 185) */
   "iip\0"
   "glTexEnvfv\0"
   "\0"
   /* _mesa_function_pool[11540]: MultiTexCoord4iARB (offset 404) */
   "iiiii\0"
   "glMultiTexCoord4i\0"
   "glMultiTexCoord4iARB\0"
   "\0"
   /* _mesa_function_pool[11586]: Indexs (offset 50) */
   "i\0"
   "glIndexs\0"
   "\0"
   /* _mesa_function_pool[11598]: Binormal3ivEXT (dynamic) */
   "p\0"
   "glBinormal3ivEXT\0"
   "\0"
   /* _mesa_function_pool[11618]: ResizeBuffersMESA (will be remapped) */
   "\0"
   "glResizeBuffersMESA\0"
   "\0"
   /* _mesa_function_pool[11640]: GetUniformivARB (will be remapped) */
   "iip\0"
   "glGetUniformiv\0"
   "glGetUniformivARB\0"
   "\0"
   /* _mesa_function_pool[11678]: PixelTexGenParameteriSGIS (will be remapped) */
   "ii\0"
   "glPixelTexGenParameteriSGIS\0"
   "\0"
   /* _mesa_function_pool[11710]: VertexPointervINTEL (dynamic) */
   "iip\0"
   "glVertexPointervINTEL\0"
   "\0"
   /* _mesa_function_pool[11737]: Vertex2i (offset 130) */
   "ii\0"
   "glVertex2i\0"
   "\0"
   /* _mesa_function_pool[11752]: LoadMatrixf (offset 291) */
   "p\0"
   "glLoadMatrixf\0"
   "\0"
   /* _mesa_function_pool[11769]: Vertex2f (offset 128) */
   "ff\0"
   "glVertex2f\0"
   "\0"
   /* _mesa_function_pool[11784]: ReplacementCodeuiColor4fNormal3fVertex3fvSUN (dynamic) */
   "pppp\0"
   "glReplacementCodeuiColor4fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[11837]: Color4bv (offset 26) */
   "p\0"
   "glColor4bv\0"
   "\0"
   /* _mesa_function_pool[11851]: VertexPointer (offset 321) */
   "iiip\0"
   "glVertexPointer\0"
   "\0"
   /* _mesa_function_pool[11873]: SecondaryColor3uiEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3ui\0"
   "glSecondaryColor3uiEXT\0"
   "\0"
   /* _mesa_function_pool[11921]: StartInstrumentsSGIX (dynamic) */
   "\0"
   "glStartInstrumentsSGIX\0"
   "\0"
   /* _mesa_function_pool[11946]: SecondaryColor3usvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3usv\0"
   "glSecondaryColor3usvEXT\0"
   "\0"
   /* _mesa_function_pool[11994]: VertexAttrib2fvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib2fvNV\0"
   "\0"
   /* _mesa_function_pool[12018]: ProgramLocalParameter4dvARB (will be remapped) */
   "iip\0"
   "glProgramLocalParameter4dvARB\0"
   "\0"
   /* _mesa_function_pool[12053]: DeleteLists (offset 4) */
   "ii\0"
   "glDeleteLists\0"
   "\0"
   /* _mesa_function_pool[12071]: LogicOp (offset 242) */
   "i\0"
   "glLogicOp\0"
   "\0"
   /* _mesa_function_pool[12084]: MatrixIndexuivARB (dynamic) */
   "ip\0"
   "glMatrixIndexuivARB\0"
   "\0"
   /* _mesa_function_pool[12108]: Vertex2s (offset 132) */
   "ii\0"
   "glVertex2s\0"
   "\0"
   /* _mesa_function_pool[12123]: RenderbufferStorageMultisample (will be remapped) */
   "iiiii\0"
   "glRenderbufferStorageMultisample\0"
   "glRenderbufferStorageMultisampleEXT\0"
   "\0"
   /* _mesa_function_pool[12199]: TexCoord4fv (offset 121) */
   "p\0"
   "glTexCoord4fv\0"
   "\0"
   /* _mesa_function_pool[12216]: Tangent3sEXT (dynamic) */
   "iii\0"
   "glTangent3sEXT\0"
   "\0"
   /* _mesa_function_pool[12236]: GlobalAlphaFactorfSUN (dynamic) */
   "f\0"
   "glGlobalAlphaFactorfSUN\0"
   "\0"
   /* _mesa_function_pool[12263]: MultiTexCoord3iARB (offset 396) */
   "iiii\0"
   "glMultiTexCoord3i\0"
   "glMultiTexCoord3iARB\0"
   "\0"
   /* _mesa_function_pool[12308]: IsProgram (will be remapped) */
   "i\0"
   "glIsProgram\0"
   "\0"
   /* _mesa_function_pool[12323]: TexCoordPointerListIBM (dynamic) */
   "iiipi\0"
   "glTexCoordPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[12355]: GlobalAlphaFactorusSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactorusSUN\0"
   "\0"
   /* _mesa_function_pool[12383]: VertexAttrib2dvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib2dvNV\0"
   "\0"
   /* _mesa_function_pool[12407]: FramebufferRenderbufferEXT (will be remapped) */
   "iiii\0"
   "glFramebufferRenderbuffer\0"
   "glFramebufferRenderbufferEXT\0"
   "\0"
   /* _mesa_function_pool[12468]: VertexAttrib1dvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib1dvNV\0"
   "\0"
   /* _mesa_function_pool[12492]: GenTextures (offset 328) */
   "ip\0"
   "glGenTextures\0"
   "glGenTexturesEXT\0"
   "\0"
   /* _mesa_function_pool[12527]: FramebufferTextureARB (will be remapped) */
   "iiii\0"
   "glFramebufferTextureARB\0"
   "\0"
   /* _mesa_function_pool[12557]: SetFenceNV (will be remapped) */
   "ii\0"
   "glSetFenceNV\0"
   "\0"
   /* _mesa_function_pool[12574]: FramebufferTexture1DEXT (will be remapped) */
   "iiiii\0"
   "glFramebufferTexture1D\0"
   "glFramebufferTexture1DEXT\0"
   "\0"
   /* _mesa_function_pool[12630]: GetCombinerOutputParameterivNV (will be remapped) */
   "iiip\0"
   "glGetCombinerOutputParameterivNV\0"
   "\0"
   /* _mesa_function_pool[12669]: MultiModeDrawArraysIBM (will be remapped) */
   "pppii\0"
   "glMultiModeDrawArraysIBM\0"
   "\0"
   /* _mesa_function_pool[12701]: PixelTexGenParameterivSGIS (will be remapped) */
   "ip\0"
   "glPixelTexGenParameterivSGIS\0"
   "\0"
   /* _mesa_function_pool[12734]: TextureNormalEXT (dynamic) */
   "i\0"
   "glTextureNormalEXT\0"
   "\0"
   /* _mesa_function_pool[12756]: IndexPointerListIBM (dynamic) */
   "iipi\0"
   "glIndexPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[12784]: WeightfvARB (dynamic) */
   "ip\0"
   "glWeightfvARB\0"
   "\0"
   /* _mesa_function_pool[12802]: GetCombinerOutputParameterfvNV (will be remapped) */
   "iiip\0"
   "glGetCombinerOutputParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[12841]: RasterPos2sv (offset 69) */
   "p\0"
   "glRasterPos2sv\0"
   "\0"
   /* _mesa_function_pool[12859]: Color4ubv (offset 36) */
   "p\0"
   "glColor4ubv\0"
   "\0"
   /* _mesa_function_pool[12874]: DrawBuffer (offset 202) */
   "i\0"
   "glDrawBuffer\0"
   "\0"
   /* _mesa_function_pool[12890]: TexCoord2fv (offset 105) */
   "p\0"
   "glTexCoord2fv\0"
   "\0"
   /* _mesa_function_pool[12907]: WindowPos4fMESA (will be remapped) */
   "ffff\0"
   "glWindowPos4fMESA\0"
   "\0"
   /* _mesa_function_pool[12931]: TexCoord1sv (offset 101) */
   "p\0"
   "glTexCoord1sv\0"
   "\0"
   /* _mesa_function_pool[12948]: WindowPos3dvMESA (will be remapped) */
   "p\0"
   "glWindowPos3dv\0"
   "glWindowPos3dvARB\0"
   "glWindowPos3dvMESA\0"
   "\0"
   /* _mesa_function_pool[13003]: DepthFunc (offset 245) */
   "i\0"
   "glDepthFunc\0"
   "\0"
   /* _mesa_function_pool[13018]: PixelMapusv (offset 253) */
   "iip\0"
   "glPixelMapusv\0"
   "\0"
   /* _mesa_function_pool[13037]: GetQueryObjecti64vEXT (will be remapped) */
   "iip\0"
   "glGetQueryObjecti64vEXT\0"
   "\0"
   /* _mesa_function_pool[13066]: MultiTexCoord1dARB (offset 376) */
   "id\0"
   "glMultiTexCoord1d\0"
   "glMultiTexCoord1dARB\0"
   "\0"
   /* _mesa_function_pool[13109]: PointParameterivNV (will be remapped) */
   "ip\0"
   "glPointParameteriv\0"
   "glPointParameterivNV\0"
   "\0"
   /* _mesa_function_pool[13153]: BlendFunc (offset 241) */
   "ii\0"
   "glBlendFunc\0"
   "\0"
   /* _mesa_function_pool[13169]: EndTransformFeedbackEXT (will be remapped) */
   "\0"
   "glEndTransformFeedbackEXT\0"
   "glEndTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[13220]: Uniform2fvARB (will be remapped) */
   "iip\0"
   "glUniform2fv\0"
   "glUniform2fvARB\0"
   "\0"
   /* _mesa_function_pool[13254]: BufferParameteriAPPLE (will be remapped) */
   "iii\0"
   "glBufferParameteriAPPLE\0"
   "\0"
   /* _mesa_function_pool[13283]: MultiTexCoord3dvARB (offset 393) */
   "ip\0"
   "glMultiTexCoord3dv\0"
   "glMultiTexCoord3dvARB\0"
   "\0"
   /* _mesa_function_pool[13328]: ReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN (dynamic) */
   "pppp\0"
   "glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[13384]: DeleteObjectARB (will be remapped) */
   "i\0"
   "glDeleteObjectARB\0"
   "\0"
   /* _mesa_function_pool[13405]: MatrixIndexPointerARB (dynamic) */
   "iiip\0"
   "glMatrixIndexPointerARB\0"
   "\0"
   /* _mesa_function_pool[13435]: ProgramNamedParameter4dvNV (will be remapped) */
   "iipp\0"
   "glProgramNamedParameter4dvNV\0"
   "\0"
   /* _mesa_function_pool[13470]: Tangent3fvEXT (dynamic) */
   "p\0"
   "glTangent3fvEXT\0"
   "\0"
   /* _mesa_function_pool[13489]: Flush (offset 217) */
   "\0"
   "glFlush\0"
   "\0"
   /* _mesa_function_pool[13499]: Color4uiv (offset 38) */
   "p\0"
   "glColor4uiv\0"
   "\0"
   /* _mesa_function_pool[13514]: GenVertexArrays (will be remapped) */
   "ip\0"
   "glGenVertexArrays\0"
   "\0"
   /* _mesa_function_pool[13536]: RasterPos3sv (offset 77) */
   "p\0"
   "glRasterPos3sv\0"
   "\0"
   /* _mesa_function_pool[13554]: BindFramebufferEXT (will be remapped) */
   "ii\0"
   "glBindFramebuffer\0"
   "glBindFramebufferEXT\0"
   "\0"
   /* _mesa_function_pool[13597]: ReferencePlaneSGIX (dynamic) */
   "p\0"
   "glReferencePlaneSGIX\0"
   "\0"
   /* _mesa_function_pool[13621]: PushAttrib (offset 219) */
   "i\0"
   "glPushAttrib\0"
   "\0"
   /* _mesa_function_pool[13637]: RasterPos2i (offset 66) */
   "ii\0"
   "glRasterPos2i\0"
   "\0"
   /* _mesa_function_pool[13655]: ValidateProgramARB (will be remapped) */
   "i\0"
   "glValidateProgram\0"
   "glValidateProgramARB\0"
   "\0"
   /* _mesa_function_pool[13697]: TexParameteriv (offset 181) */
   "iip\0"
   "glTexParameteriv\0"
   "\0"
   /* _mesa_function_pool[13719]: UnlockArraysEXT (will be remapped) */
   "\0"
   "glUnlockArraysEXT\0"
   "\0"
   /* _mesa_function_pool[13739]: TexCoord2fColor3fVertex3fSUN (dynamic) */
   "ffffffff\0"
   "glTexCoord2fColor3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[13780]: WindowPos3fvMESA (will be remapped) */
   "p\0"
   "glWindowPos3fv\0"
   "glWindowPos3fvARB\0"
   "glWindowPos3fvMESA\0"
   "\0"
   /* _mesa_function_pool[13835]: RasterPos2f (offset 64) */
   "ff\0"
   "glRasterPos2f\0"
   "\0"
   /* _mesa_function_pool[13853]: VertexAttrib1svNV (will be remapped) */
   "ip\0"
   "glVertexAttrib1svNV\0"
   "\0"
   /* _mesa_function_pool[13877]: RasterPos2d (offset 62) */
   "dd\0"
   "glRasterPos2d\0"
   "\0"
   /* _mesa_function_pool[13895]: RasterPos3fv (offset 73) */
   "p\0"
   "glRasterPos3fv\0"
   "\0"
   /* _mesa_function_pool[13913]: CopyTexSubImage3D (offset 373) */
   "iiiiiiiii\0"
   "glCopyTexSubImage3D\0"
   "glCopyTexSubImage3DEXT\0"
   "\0"
   /* _mesa_function_pool[13967]: VertexAttrib2dARB (will be remapped) */
   "idd\0"
   "glVertexAttrib2d\0"
   "glVertexAttrib2dARB\0"
   "\0"
   /* _mesa_function_pool[14009]: Color4ub (offset 35) */
   "iiii\0"
   "glColor4ub\0"
   "\0"
   /* _mesa_function_pool[14026]: GetInteger64v (will be remapped) */
   "ip\0"
   "glGetInteger64v\0"
   "\0"
   /* _mesa_function_pool[14046]: TextureColorMaskSGIS (dynamic) */
   "iiii\0"
   "glTextureColorMaskSGIS\0"
   "\0"
   /* _mesa_function_pool[14075]: RasterPos2s (offset 68) */
   "ii\0"
   "glRasterPos2s\0"
   "\0"
   /* _mesa_function_pool[14093]: GetColorTable (offset 343) */
   "iiip\0"
   "glGetColorTable\0"
   "glGetColorTableSGI\0"
   "glGetColorTableEXT\0"
   "\0"
   /* _mesa_function_pool[14153]: SelectBuffer (offset 195) */
   "ip\0"
   "glSelectBuffer\0"
   "\0"
   /* _mesa_function_pool[14172]: Indexiv (offset 49) */
   "p\0"
   "glIndexiv\0"
   "\0"
   /* _mesa_function_pool[14185]: TexCoord3i (offset 114) */
   "iii\0"
   "glTexCoord3i\0"
   "\0"
   /* _mesa_function_pool[14203]: CopyColorTable (offset 342) */
   "iiiii\0"
   "glCopyColorTable\0"
   "glCopyColorTableSGI\0"
   "\0"
   /* _mesa_function_pool[14247]: GetHistogramParameterfv (offset 362) */
   "iip\0"
   "glGetHistogramParameterfv\0"
   "glGetHistogramParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[14307]: Frustum (offset 289) */
   "dddddd\0"
   "glFrustum\0"
   "\0"
   /* _mesa_function_pool[14325]: GetString (offset 275) */
   "i\0"
   "glGetString\0"
   "\0"
   /* _mesa_function_pool[14340]: ColorPointervINTEL (dynamic) */
   "iip\0"
   "glColorPointervINTEL\0"
   "\0"
   /* _mesa_function_pool[14366]: TexEnvf (offset 184) */
   "iif\0"
   "glTexEnvf\0"
   "\0"
   /* _mesa_function_pool[14381]: TexCoord3d (offset 110) */
   "ddd\0"
   "glTexCoord3d\0"
   "\0"
   /* _mesa_function_pool[14399]: AlphaFragmentOp1ATI (will be remapped) */
   "iiiiii\0"
   "glAlphaFragmentOp1ATI\0"
   "\0"
   /* _mesa_function_pool[14429]: TexCoord3f (offset 112) */
   "fff\0"
   "glTexCoord3f\0"
   "\0"
   /* _mesa_function_pool[14447]: MultiTexCoord3ivARB (offset 397) */
   "ip\0"
   "glMultiTexCoord3iv\0"
   "glMultiTexCoord3ivARB\0"
   "\0"
   /* _mesa_function_pool[14492]: MultiTexCoord2sARB (offset 390) */
   "iii\0"
   "glMultiTexCoord2s\0"
   "glMultiTexCoord2sARB\0"
   "\0"
   /* _mesa_function_pool[14536]: VertexAttrib1dvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib1dv\0"
   "glVertexAttrib1dvARB\0"
   "\0"
   /* _mesa_function_pool[14579]: DeleteTextures (offset 327) */
   "ip\0"
   "glDeleteTextures\0"
   "glDeleteTexturesEXT\0"
   "\0"
   /* _mesa_function_pool[14620]: TexCoordPointerEXT (will be remapped) */
   "iiiip\0"
   "glTexCoordPointerEXT\0"
   "\0"
   /* _mesa_function_pool[14648]: TexSubImage4DSGIS (dynamic) */
   "iiiiiiiiiiiip\0"
   "glTexSubImage4DSGIS\0"
   "\0"
   /* _mesa_function_pool[14683]: TexCoord3s (offset 116) */
   "iii\0"
   "glTexCoord3s\0"
   "\0"
   /* _mesa_function_pool[14701]: GetTexLevelParameteriv (offset 285) */
   "iiip\0"
   "glGetTexLevelParameteriv\0"
   "\0"
   /* _mesa_function_pool[14732]: DrawArraysInstanced (will be remapped) */
   "iiii\0"
   "glDrawArraysInstanced\0"
   "glDrawArraysInstancedARB\0"
   "glDrawArraysInstancedEXT\0"
   "\0"
   /* _mesa_function_pool[14810]: CombinerStageParameterfvNV (dynamic) */
   "iip\0"
   "glCombinerStageParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[14844]: StopInstrumentsSGIX (dynamic) */
   "i\0"
   "glStopInstrumentsSGIX\0"
   "\0"
   /* _mesa_function_pool[14869]: TexCoord4fColor4fNormal3fVertex4fSUN (dynamic) */
   "fffffffffffffff\0"
   "glTexCoord4fColor4fNormal3fVertex4fSUN\0"
   "\0"
   /* _mesa_function_pool[14925]: ClearAccum (offset 204) */
   "ffff\0"
   "glClearAccum\0"
   "\0"
   /* _mesa_function_pool[14944]: DeformSGIX (dynamic) */
   "i\0"
   "glDeformSGIX\0"
   "\0"
   /* _mesa_function_pool[14960]: GetVertexAttribfvARB (will be remapped) */
   "iip\0"
   "glGetVertexAttribfv\0"
   "glGetVertexAttribfvARB\0"
   "\0"
   /* _mesa_function_pool[15008]: SecondaryColor3ivEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3iv\0"
   "glSecondaryColor3ivEXT\0"
   "\0"
   /* _mesa_function_pool[15054]: TexCoord4iv (offset 123) */
   "p\0"
   "glTexCoord4iv\0"
   "\0"
   /* _mesa_function_pool[15071]: UniformMatrix4x2fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix4x2fv\0"
   "\0"
   /* _mesa_function_pool[15098]: GetDetailTexFuncSGIS (dynamic) */
   "ip\0"
   "glGetDetailTexFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[15125]: GetCombinerStageParameterfvNV (dynamic) */
   "iip\0"
   "glGetCombinerStageParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[15162]: PolygonOffset (offset 319) */
   "ff\0"
   "glPolygonOffset\0"
   "\0"
   /* _mesa_function_pool[15182]: BindVertexArray (will be remapped) */
   "i\0"
   "glBindVertexArray\0"
   "\0"
   /* _mesa_function_pool[15203]: Color4ubVertex2fvSUN (dynamic) */
   "pp\0"
   "glColor4ubVertex2fvSUN\0"
   "\0"
   /* _mesa_function_pool[15230]: Rectd (offset 86) */
   "dddd\0"
   "glRectd\0"
   "\0"
   /* _mesa_function_pool[15244]: TexFilterFuncSGIS (dynamic) */
   "iiip\0"
   "glTexFilterFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[15270]: SampleMaskSGIS (will be remapped) */
   "fi\0"
   "glSampleMaskSGIS\0"
   "glSampleMaskEXT\0"
   "\0"
   /* _mesa_function_pool[15307]: GetAttribLocationARB (will be remapped) */
   "ip\0"
   "glGetAttribLocation\0"
   "glGetAttribLocationARB\0"
   "\0"
   /* _mesa_function_pool[15354]: RasterPos3i (offset 74) */
   "iii\0"
   "glRasterPos3i\0"
   "\0"
   /* _mesa_function_pool[15373]: VertexAttrib4ubvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4ubv\0"
   "glVertexAttrib4ubvARB\0"
   "\0"
   /* _mesa_function_pool[15418]: DetailTexFuncSGIS (dynamic) */
   "iip\0"
   "glDetailTexFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[15443]: Normal3fVertex3fSUN (dynamic) */
   "ffffff\0"
   "glNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[15473]: CopyTexImage2D (offset 324) */
   "iiiiiiii\0"
   "glCopyTexImage2D\0"
   "glCopyTexImage2DEXT\0"
   "\0"
   /* _mesa_function_pool[15520]: GetBufferPointervARB (will be remapped) */
   "iip\0"
   "glGetBufferPointerv\0"
   "glGetBufferPointervARB\0"
   "\0"
   /* _mesa_function_pool[15568]: ProgramEnvParameter4fARB (will be remapped) */
   "iiffff\0"
   "glProgramEnvParameter4fARB\0"
   "glProgramParameter4fNV\0"
   "\0"
   /* _mesa_function_pool[15626]: Uniform3ivARB (will be remapped) */
   "iip\0"
   "glUniform3iv\0"
   "glUniform3ivARB\0"
   "\0"
   /* _mesa_function_pool[15660]: Lightfv (offset 160) */
   "iip\0"
   "glLightfv\0"
   "\0"
   /* _mesa_function_pool[15675]: PrimitiveRestartIndexNV (will be remapped) */
   "i\0"
   "glPrimitiveRestartIndexNV\0"
   "\0"
   /* _mesa_function_pool[15704]: ClearDepth (offset 208) */
   "d\0"
   "glClearDepth\0"
   "\0"
   /* _mesa_function_pool[15720]: GetFenceivNV (will be remapped) */
   "iip\0"
   "glGetFenceivNV\0"
   "\0"
   /* _mesa_function_pool[15740]: WindowPos4dvMESA (will be remapped) */
   "p\0"
   "glWindowPos4dvMESA\0"
   "\0"
   /* _mesa_function_pool[15762]: ColorSubTable (offset 346) */
   "iiiiip\0"
   "glColorSubTable\0"
   "glColorSubTableEXT\0"
   "\0"
   /* _mesa_function_pool[15805]: Color4fv (offset 30) */
   "p\0"
   "glColor4fv\0"
   "\0"
   /* _mesa_function_pool[15819]: MultiTexCoord4ivARB (offset 405) */
   "ip\0"
   "glMultiTexCoord4iv\0"
   "glMultiTexCoord4ivARB\0"
   "\0"
   /* _mesa_function_pool[15864]: DrawElementsInstanced (will be remapped) */
   "iiipi\0"
   "glDrawElementsInstanced\0"
   "glDrawElementsInstancedARB\0"
   "glDrawElementsInstancedEXT\0"
   "\0"
   /* _mesa_function_pool[15949]: ColorPointer (offset 308) */
   "iiip\0"
   "glColorPointer\0"
   "\0"
   /* _mesa_function_pool[15970]: Rects (offset 92) */
   "iiii\0"
   "glRects\0"
   "\0"
   /* _mesa_function_pool[15984]: GetMapAttribParameterfvNV (dynamic) */
   "iiip\0"
   "glGetMapAttribParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[16018]: Lightiv (offset 162) */
   "iip\0"
   "glLightiv\0"
   "\0"
   /* _mesa_function_pool[16033]: VertexAttrib4sARB (will be remapped) */
   "iiiii\0"
   "glVertexAttrib4s\0"
   "glVertexAttrib4sARB\0"
   "\0"
   /* _mesa_function_pool[16077]: GetQueryObjectuivARB (will be remapped) */
   "iip\0"
   "glGetQueryObjectuiv\0"
   "glGetQueryObjectuivARB\0"
   "\0"
   /* _mesa_function_pool[16125]: GetTexParameteriv (offset 283) */
   "iip\0"
   "glGetTexParameteriv\0"
   "\0"
   /* _mesa_function_pool[16150]: MapParameterivNV (dynamic) */
   "iip\0"
   "glMapParameterivNV\0"
   "\0"
   /* _mesa_function_pool[16174]: GenRenderbuffersEXT (will be remapped) */
   "ip\0"
   "glGenRenderbuffers\0"
   "glGenRenderbuffersEXT\0"
   "\0"
   /* _mesa_function_pool[16219]: VertexAttrib2dvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib2dv\0"
   "glVertexAttrib2dvARB\0"
   "\0"
   /* _mesa_function_pool[16262]: EdgeFlagPointerEXT (will be remapped) */
   "iip\0"
   "glEdgeFlagPointerEXT\0"
   "\0"
   /* _mesa_function_pool[16288]: VertexAttribs2svNV (will be remapped) */
   "iip\0"
   "glVertexAttribs2svNV\0"
   "\0"
   /* _mesa_function_pool[16314]: WeightbvARB (dynamic) */
   "ip\0"
   "glWeightbvARB\0"
   "\0"
   /* _mesa_function_pool[16332]: VertexAttrib2fvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib2fv\0"
   "glVertexAttrib2fvARB\0"
   "\0"
   /* _mesa_function_pool[16375]: GetBufferParameterivARB (will be remapped) */
   "iip\0"
   "glGetBufferParameteriv\0"
   "glGetBufferParameterivARB\0"
   "\0"
   /* _mesa_function_pool[16429]: Rectdv (offset 87) */
   "pp\0"
   "glRectdv\0"
   "\0"
   /* _mesa_function_pool[16442]: ListParameteriSGIX (dynamic) */
   "iii\0"
   "glListParameteriSGIX\0"
   "\0"
   /* _mesa_function_pool[16468]: ReplacementCodeuiColor4fNormal3fVertex3fSUN (dynamic) */
   "iffffffffff\0"
   "glReplacementCodeuiColor4fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[16527]: InstrumentsBufferSGIX (dynamic) */
   "ip\0"
   "glInstrumentsBufferSGIX\0"
   "\0"
   /* _mesa_function_pool[16555]: VertexAttrib4NivARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Niv\0"
   "glVertexAttrib4NivARB\0"
   "\0"
   /* _mesa_function_pool[16600]: GetAttachedShaders (will be remapped) */
   "iipp\0"
   "glGetAttachedShaders\0"
   "\0"
   /* _mesa_function_pool[16627]: GenVertexArraysAPPLE (will be remapped) */
   "ip\0"
   "glGenVertexArraysAPPLE\0"
   "\0"
   /* _mesa_function_pool[16654]: Materialiv (offset 172) */
   "iip\0"
   "glMaterialiv\0"
   "\0"
   /* _mesa_function_pool[16672]: PushClientAttrib (offset 335) */
   "i\0"
   "glPushClientAttrib\0"
   "\0"
   /* _mesa_function_pool[16694]: ProgramEnvParameters4fvEXT (will be remapped) */
   "iiip\0"
   "glProgramEnvParameters4fvEXT\0"
   "\0"
   /* _mesa_function_pool[16729]: TexCoord2fColor4fNormal3fVertex3fvSUN (dynamic) */
   "pppp\0"
   "glTexCoord2fColor4fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[16775]: WindowPos2iMESA (will be remapped) */
   "ii\0"
   "glWindowPos2i\0"
   "glWindowPos2iARB\0"
   "glWindowPos2iMESA\0"
   "\0"
   /* _mesa_function_pool[16828]: SecondaryColor3fvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3fv\0"
   "glSecondaryColor3fvEXT\0"
   "\0"
   /* _mesa_function_pool[16874]: PolygonMode (offset 174) */
   "ii\0"
   "glPolygonMode\0"
   "\0"
   /* _mesa_function_pool[16892]: CompressedTexSubImage1DARB (will be remapped) */
   "iiiiiip\0"
   "glCompressedTexSubImage1D\0"
   "glCompressedTexSubImage1DARB\0"
   "\0"
   /* _mesa_function_pool[16956]: GetVertexAttribivNV (will be remapped) */
   "iip\0"
   "glGetVertexAttribivNV\0"
   "\0"
   /* _mesa_function_pool[16983]: GetProgramStringARB (will be remapped) */
   "iip\0"
   "glGetProgramStringARB\0"
   "\0"
   /* _mesa_function_pool[17010]: TexBumpParameterfvATI (will be remapped) */
   "ip\0"
   "glTexBumpParameterfvATI\0"
   "\0"
   /* _mesa_function_pool[17038]: CompileShaderARB (will be remapped) */
   "i\0"
   "glCompileShader\0"
   "glCompileShaderARB\0"
   "\0"
   /* _mesa_function_pool[17076]: DeleteShader (will be remapped) */
   "i\0"
   "glDeleteShader\0"
   "\0"
   /* _mesa_function_pool[17094]: DisableClientState (offset 309) */
   "i\0"
   "glDisableClientState\0"
   "\0"
   /* _mesa_function_pool[17118]: TexGeni (offset 192) */
   "iii\0"
   "glTexGeni\0"
   "\0"
   /* _mesa_function_pool[17133]: TexGenf (offset 190) */
   "iif\0"
   "glTexGenf\0"
   "\0"
   /* _mesa_function_pool[17148]: Uniform3fARB (will be remapped) */
   "ifff\0"
   "glUniform3f\0"
   "glUniform3fARB\0"
   "\0"
   /* _mesa_function_pool[17181]: TexGend (offset 188) */
   "iid\0"
   "glTexGend\0"
   "\0"
   /* _mesa_function_pool[17196]: ListParameterfvSGIX (dynamic) */
   "iip\0"
   "glListParameterfvSGIX\0"
   "\0"
   /* _mesa_function_pool[17223]: GetPolygonStipple (offset 274) */
   "p\0"
   "glGetPolygonStipple\0"
   "\0"
   /* _mesa_function_pool[17246]: Tangent3dvEXT (dynamic) */
   "p\0"
   "glTangent3dvEXT\0"
   "\0"
   /* _mesa_function_pool[17265]: BindBufferOffsetEXT (will be remapped) */
   "iiii\0"
   "glBindBufferOffsetEXT\0"
   "\0"
   /* _mesa_function_pool[17293]: WindowPos3sMESA (will be remapped) */
   "iii\0"
   "glWindowPos3s\0"
   "glWindowPos3sARB\0"
   "glWindowPos3sMESA\0"
   "\0"
   /* _mesa_function_pool[17347]: VertexAttrib2svNV (will be remapped) */
   "ip\0"
   "glVertexAttrib2svNV\0"
   "\0"
   /* _mesa_function_pool[17371]: DisableIndexedEXT (will be remapped) */
   "ii\0"
   "glDisableIndexedEXT\0"
   "\0"
   /* _mesa_function_pool[17395]: BindBufferBaseEXT (will be remapped) */
   "iii\0"
   "glBindBufferBaseEXT\0"
   "glBindBufferBase\0"
   "\0"
   /* _mesa_function_pool[17437]: TexCoord2fVertex3fvSUN (dynamic) */
   "pp\0"
   "glTexCoord2fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[17466]: WindowPos4sMESA (will be remapped) */
   "iiii\0"
   "glWindowPos4sMESA\0"
   "\0"
   /* _mesa_function_pool[17490]: VertexAttrib4NuivARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Nuiv\0"
   "glVertexAttrib4NuivARB\0"
   "\0"
   /* _mesa_function_pool[17537]: ClientActiveTextureARB (offset 375) */
   "i\0"
   "glClientActiveTexture\0"
   "glClientActiveTextureARB\0"
   "\0"
   /* _mesa_function_pool[17587]: PixelTexGenSGIX (will be remapped) */
   "i\0"
   "glPixelTexGenSGIX\0"
   "\0"
   /* _mesa_function_pool[17608]: ReplacementCodeusvSUN (dynamic) */
   "p\0"
   "glReplacementCodeusvSUN\0"
   "\0"
   /* _mesa_function_pool[17635]: Uniform4fARB (will be remapped) */
   "iffff\0"
   "glUniform4f\0"
   "glUniform4fARB\0"
   "\0"
   /* _mesa_function_pool[17669]: Color4sv (offset 34) */
   "p\0"
   "glColor4sv\0"
   "\0"
   /* _mesa_function_pool[17683]: FlushMappedBufferRange (will be remapped) */
   "iii\0"
   "glFlushMappedBufferRange\0"
   "\0"
   /* _mesa_function_pool[17713]: IsProgramNV (will be remapped) */
   "i\0"
   "glIsProgramARB\0"
   "glIsProgramNV\0"
   "\0"
   /* _mesa_function_pool[17745]: FlushMappedBufferRangeAPPLE (will be remapped) */
   "iii\0"
   "glFlushMappedBufferRangeAPPLE\0"
   "\0"
   /* _mesa_function_pool[17780]: PixelZoom (offset 246) */
   "ff\0"
   "glPixelZoom\0"
   "\0"
   /* _mesa_function_pool[17796]: ReplacementCodePointerSUN (dynamic) */
   "iip\0"
   "glReplacementCodePointerSUN\0"
   "\0"
   /* _mesa_function_pool[17829]: ProgramEnvParameter4dARB (will be remapped) */
   "iidddd\0"
   "glProgramEnvParameter4dARB\0"
   "glProgramParameter4dNV\0"
   "\0"
   /* _mesa_function_pool[17887]: ColorTableParameterfv (offset 340) */
   "iip\0"
   "glColorTableParameterfv\0"
   "glColorTableParameterfvSGI\0"
   "\0"
   /* _mesa_function_pool[17943]: FragmentLightModelfSGIX (dynamic) */
   "if\0"
   "glFragmentLightModelfSGIX\0"
   "\0"
   /* _mesa_function_pool[17973]: Binormal3bvEXT (dynamic) */
   "p\0"
   "glBinormal3bvEXT\0"
   "\0"
   /* _mesa_function_pool[17993]: PixelMapuiv (offset 252) */
   "iip\0"
   "glPixelMapuiv\0"
   "\0"
   /* _mesa_function_pool[18012]: Color3dv (offset 12) */
   "p\0"
   "glColor3dv\0"
   "\0"
   /* _mesa_function_pool[18026]: IsTexture (offset 330) */
   "i\0"
   "glIsTexture\0"
   "glIsTextureEXT\0"
   "\0"
   /* _mesa_function_pool[18056]: VertexWeightfvEXT (dynamic) */
   "p\0"
   "glVertexWeightfvEXT\0"
   "\0"
   /* _mesa_function_pool[18079]: VertexAttrib1dARB (will be remapped) */
   "id\0"
   "glVertexAttrib1d\0"
   "glVertexAttrib1dARB\0"
   "\0"
   /* _mesa_function_pool[18120]: ImageTransformParameterivHP (dynamic) */
   "iip\0"
   "glImageTransformParameterivHP\0"
   "\0"
   /* _mesa_function_pool[18155]: TexCoord4i (offset 122) */
   "iiii\0"
   "glTexCoord4i\0"
   "\0"
   /* _mesa_function_pool[18174]: DeleteQueriesARB (will be remapped) */
   "ip\0"
   "glDeleteQueries\0"
   "glDeleteQueriesARB\0"
   "\0"
   /* _mesa_function_pool[18213]: Color4ubVertex2fSUN (dynamic) */
   "iiiiff\0"
   "glColor4ubVertex2fSUN\0"
   "\0"
   /* _mesa_function_pool[18243]: FragmentColorMaterialSGIX (dynamic) */
   "ii\0"
   "glFragmentColorMaterialSGIX\0"
   "\0"
   /* _mesa_function_pool[18275]: CurrentPaletteMatrixARB (dynamic) */
   "i\0"
   "glCurrentPaletteMatrixARB\0"
   "\0"
   /* _mesa_function_pool[18304]: GetMapdv (offset 266) */
   "iip\0"
   "glGetMapdv\0"
   "\0"
   /* _mesa_function_pool[18320]: ObjectPurgeableAPPLE (will be remapped) */
   "iii\0"
   "glObjectPurgeableAPPLE\0"
   "\0"
   /* _mesa_function_pool[18348]: SamplePatternSGIS (will be remapped) */
   "i\0"
   "glSamplePatternSGIS\0"
   "glSamplePatternEXT\0"
   "\0"
   /* _mesa_function_pool[18390]: PixelStoref (offset 249) */
   "if\0"
   "glPixelStoref\0"
   "\0"
   /* _mesa_function_pool[18408]: IsQueryARB (will be remapped) */
   "i\0"
   "glIsQuery\0"
   "glIsQueryARB\0"
   "\0"
   /* _mesa_function_pool[18434]: ReplacementCodeuiColor4ubVertex3fSUN (dynamic) */
   "iiiiifff\0"
   "glReplacementCodeuiColor4ubVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[18483]: PixelStorei (offset 250) */
   "ii\0"
   "glPixelStorei\0"
   "\0"
   /* _mesa_function_pool[18501]: VertexAttrib4usvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4usv\0"
   "glVertexAttrib4usvARB\0"
   "\0"
   /* _mesa_function_pool[18546]: LinkProgramARB (will be remapped) */
   "i\0"
   "glLinkProgram\0"
   "glLinkProgramARB\0"
   "\0"
   /* _mesa_function_pool[18580]: VertexAttrib2fNV (will be remapped) */
   "iff\0"
   "glVertexAttrib2fNV\0"
   "\0"
   /* _mesa_function_pool[18604]: ShaderSourceARB (will be remapped) */
   "iipp\0"
   "glShaderSource\0"
   "glShaderSourceARB\0"
   "\0"
   /* _mesa_function_pool[18643]: FragmentMaterialiSGIX (dynamic) */
   "iii\0"
   "glFragmentMaterialiSGIX\0"
   "\0"
   /* _mesa_function_pool[18672]: EvalCoord2dv (offset 233) */
   "p\0"
   "glEvalCoord2dv\0"
   "\0"
   /* _mesa_function_pool[18690]: VertexAttrib3svARB (will be remapped) */
   "ip\0"
   "glVertexAttrib3sv\0"
   "glVertexAttrib3svARB\0"
   "\0"
   /* _mesa_function_pool[18733]: ColorMaterial (offset 151) */
   "ii\0"
   "glColorMaterial\0"
   "\0"
   /* _mesa_function_pool[18753]: CompressedTexSubImage3DARB (will be remapped) */
   "iiiiiiiiiip\0"
   "glCompressedTexSubImage3D\0"
   "glCompressedTexSubImage3DARB\0"
   "\0"
   /* _mesa_function_pool[18821]: WindowPos2ivMESA (will be remapped) */
   "p\0"
   "glWindowPos2iv\0"
   "glWindowPos2ivARB\0"
   "glWindowPos2ivMESA\0"
   "\0"
   /* _mesa_function_pool[18876]: IsFramebufferEXT (will be remapped) */
   "i\0"
   "glIsFramebuffer\0"
   "glIsFramebufferEXT\0"
   "\0"
   /* _mesa_function_pool[18914]: Uniform4ivARB (will be remapped) */
   "iip\0"
   "glUniform4iv\0"
   "glUniform4ivARB\0"
   "\0"
   /* _mesa_function_pool[18948]: GetVertexAttribdvARB (will be remapped) */
   "iip\0"
   "glGetVertexAttribdv\0"
   "glGetVertexAttribdvARB\0"
   "\0"
   /* _mesa_function_pool[18996]: TexBumpParameterivATI (will be remapped) */
   "ip\0"
   "glTexBumpParameterivATI\0"
   "\0"
   /* _mesa_function_pool[19024]: GetSeparableFilter (offset 359) */
   "iiippp\0"
   "glGetSeparableFilter\0"
   "glGetSeparableFilterEXT\0"
   "\0"
   /* _mesa_function_pool[19077]: Binormal3dEXT (dynamic) */
   "ddd\0"
   "glBinormal3dEXT\0"
   "\0"
   /* _mesa_function_pool[19098]: SpriteParameteriSGIX (dynamic) */
   "ii\0"
   "glSpriteParameteriSGIX\0"
   "\0"
   /* _mesa_function_pool[19125]: RequestResidentProgramsNV (will be remapped) */
   "ip\0"
   "glRequestResidentProgramsNV\0"
   "\0"
   /* _mesa_function_pool[19157]: TagSampleBufferSGIX (dynamic) */
   "\0"
   "glTagSampleBufferSGIX\0"
   "\0"
   /* _mesa_function_pool[19181]: TransformFeedbackVaryingsEXT (will be remapped) */
   "iipi\0"
   "glTransformFeedbackVaryingsEXT\0"
   "glTransformFeedbackVaryings\0"
   "\0"
   /* _mesa_function_pool[19246]: FeedbackBuffer (offset 194) */
   "iip\0"
   "glFeedbackBuffer\0"
   "\0"
   /* _mesa_function_pool[19268]: RasterPos2iv (offset 67) */
   "p\0"
   "glRasterPos2iv\0"
   "\0"
   /* _mesa_function_pool[19286]: TexImage1D (offset 182) */
   "iiiiiiip\0"
   "glTexImage1D\0"
   "\0"
   /* _mesa_function_pool[19309]: ListParameterivSGIX (dynamic) */
   "iip\0"
   "glListParameterivSGIX\0"
   "\0"
   /* _mesa_function_pool[19336]: MultiDrawElementsEXT (will be remapped) */
   "ipipi\0"
   "glMultiDrawElements\0"
   "glMultiDrawElementsEXT\0"
   "\0"
   /* _mesa_function_pool[19386]: Color3s (offset 17) */
   "iii\0"
   "glColor3s\0"
   "\0"
   /* _mesa_function_pool[19401]: Uniform1ivARB (will be remapped) */
   "iip\0"
   "glUniform1iv\0"
   "glUniform1ivARB\0"
   "\0"
   /* _mesa_function_pool[19435]: WindowPos2sMESA (will be remapped) */
   "ii\0"
   "glWindowPos2s\0"
   "glWindowPos2sARB\0"
   "glWindowPos2sMESA\0"
   "\0"
   /* _mesa_function_pool[19488]: WeightusvARB (dynamic) */
   "ip\0"
   "glWeightusvARB\0"
   "\0"
   /* _mesa_function_pool[19507]: TexCoordPointer (offset 320) */
   "iiip\0"
   "glTexCoordPointer\0"
   "\0"
   /* _mesa_function_pool[19531]: FogCoordPointerEXT (will be remapped) */
   "iip\0"
   "glFogCoordPointer\0"
   "glFogCoordPointerEXT\0"
   "\0"
   /* _mesa_function_pool[19575]: IndexMaterialEXT (dynamic) */
   "ii\0"
   "glIndexMaterialEXT\0"
   "\0"
   /* _mesa_function_pool[19598]: Color3i (offset 15) */
   "iii\0"
   "glColor3i\0"
   "\0"
   /* _mesa_function_pool[19613]: FrontFace (offset 157) */
   "i\0"
   "glFrontFace\0"
   "\0"
   /* _mesa_function_pool[19628]: EvalCoord2d (offset 232) */
   "dd\0"
   "glEvalCoord2d\0"
   "\0"
   /* _mesa_function_pool[19646]: SecondaryColor3ubvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3ubv\0"
   "glSecondaryColor3ubvEXT\0"
   "\0"
   /* _mesa_function_pool[19694]: EvalCoord2f (offset 234) */
   "ff\0"
   "glEvalCoord2f\0"
   "\0"
   /* _mesa_function_pool[19712]: VertexAttrib4dvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4dv\0"
   "glVertexAttrib4dvARB\0"
   "\0"
   /* _mesa_function_pool[19755]: BindAttribLocationARB (will be remapped) */
   "iip\0"
   "glBindAttribLocation\0"
   "glBindAttribLocationARB\0"
   "\0"
   /* _mesa_function_pool[19805]: Color3b (offset 9) */
   "iii\0"
   "glColor3b\0"
   "\0"
   /* _mesa_function_pool[19820]: MultiTexCoord2dARB (offset 384) */
   "idd\0"
   "glMultiTexCoord2d\0"
   "glMultiTexCoord2dARB\0"
   "\0"
   /* _mesa_function_pool[19864]: ExecuteProgramNV (will be remapped) */
   "iip\0"
   "glExecuteProgramNV\0"
   "\0"
   /* _mesa_function_pool[19888]: Color3f (offset 13) */
   "fff\0"
   "glColor3f\0"
   "\0"
   /* _mesa_function_pool[19903]: LightEnviSGIX (dynamic) */
   "ii\0"
   "glLightEnviSGIX\0"
   "\0"
   /* _mesa_function_pool[19923]: Color3d (offset 11) */
   "ddd\0"
   "glColor3d\0"
   "\0"
   /* _mesa_function_pool[19938]: Normal3dv (offset 55) */
   "p\0"
   "glNormal3dv\0"
   "\0"
   /* _mesa_function_pool[19953]: Lightf (offset 159) */
   "iif\0"
   "glLightf\0"
   "\0"
   /* _mesa_function_pool[19967]: ReplacementCodeuiSUN (dynamic) */
   "i\0"
   "glReplacementCodeuiSUN\0"
   "\0"
   /* _mesa_function_pool[19993]: MatrixMode (offset 293) */
   "i\0"
   "glMatrixMode\0"
   "\0"
   /* _mesa_function_pool[20009]: GetPixelMapusv (offset 273) */
   "ip\0"
   "glGetPixelMapusv\0"
   "\0"
   /* _mesa_function_pool[20030]: Lighti (offset 161) */
   "iii\0"
   "glLighti\0"
   "\0"
   /* _mesa_function_pool[20044]: VertexAttribPointerNV (will be remapped) */
   "iiiip\0"
   "glVertexAttribPointerNV\0"
   "\0"
   /* _mesa_function_pool[20075]: ProgramLocalParameters4fvEXT (will be remapped) */
   "iiip\0"
   "glProgramLocalParameters4fvEXT\0"
   "\0"
   /* _mesa_function_pool[20112]: GetBooleanIndexedvEXT (will be remapped) */
   "iip\0"
   "glGetBooleanIndexedvEXT\0"
   "\0"
   /* _mesa_function_pool[20141]: GetFramebufferAttachmentParameterivEXT (will be remapped) */
   "iiip\0"
   "glGetFramebufferAttachmentParameteriv\0"
   "glGetFramebufferAttachmentParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[20226]: PixelTransformParameterfEXT (dynamic) */
   "iif\0"
   "glPixelTransformParameterfEXT\0"
   "\0"
   /* _mesa_function_pool[20261]: MultiTexCoord4dvARB (offset 401) */
   "ip\0"
   "glMultiTexCoord4dv\0"
   "glMultiTexCoord4dvARB\0"
   "\0"
   /* _mesa_function_pool[20306]: PixelTransformParameteriEXT (dynamic) */
   "iii\0"
   "glPixelTransformParameteriEXT\0"
   "\0"
   /* _mesa_function_pool[20341]: GetDoublev (offset 260) */
   "ip\0"
   "glGetDoublev\0"
   "\0"
   /* _mesa_function_pool[20358]: MultMatrixd (offset 295) */
   "p\0"
   "glMultMatrixd\0"
   "\0"
   /* _mesa_function_pool[20375]: MultMatrixf (offset 294) */
   "p\0"
   "glMultMatrixf\0"
   "\0"
   /* _mesa_function_pool[20392]: TexCoord2fColor4ubVertex3fSUN (dynamic) */
   "ffiiiifff\0"
   "glTexCoord2fColor4ubVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[20435]: Uniform1iARB (will be remapped) */
   "ii\0"
   "glUniform1i\0"
   "glUniform1iARB\0"
   "\0"
   /* _mesa_function_pool[20466]: VertexAttribPointerARB (will be remapped) */
   "iiiiip\0"
   "glVertexAttribPointer\0"
   "glVertexAttribPointerARB\0"
   "\0"
   /* _mesa_function_pool[20521]: VertexAttrib3sNV (will be remapped) */
   "iiii\0"
   "glVertexAttrib3sNV\0"
   "\0"
   /* _mesa_function_pool[20546]: SharpenTexFuncSGIS (dynamic) */
   "iip\0"
   "glSharpenTexFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[20572]: MultiTexCoord4fvARB (offset 403) */
   "ip\0"
   "glMultiTexCoord4fv\0"
   "glMultiTexCoord4fvARB\0"
   "\0"
   /* _mesa_function_pool[20617]: UniformMatrix2x3fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix2x3fv\0"
   "\0"
   /* _mesa_function_pool[20644]: TrackMatrixNV (will be remapped) */
   "iiii\0"
   "glTrackMatrixNV\0"
   "\0"
   /* _mesa_function_pool[20666]: CombinerParameteriNV (will be remapped) */
   "ii\0"
   "glCombinerParameteriNV\0"
   "\0"
   /* _mesa_function_pool[20693]: DeleteAsyncMarkersSGIX (dynamic) */
   "ii\0"
   "glDeleteAsyncMarkersSGIX\0"
   "\0"
   /* _mesa_function_pool[20722]: ReplacementCodeusSUN (dynamic) */
   "i\0"
   "glReplacementCodeusSUN\0"
   "\0"
   /* _mesa_function_pool[20748]: IsAsyncMarkerSGIX (dynamic) */
   "i\0"
   "glIsAsyncMarkerSGIX\0"
   "\0"
   /* _mesa_function_pool[20771]: FrameZoomSGIX (dynamic) */
   "i\0"
   "glFrameZoomSGIX\0"
   "\0"
   /* _mesa_function_pool[20790]: Normal3fVertex3fvSUN (dynamic) */
   "pp\0"
   "glNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[20817]: RasterPos4sv (offset 85) */
   "p\0"
   "glRasterPos4sv\0"
   "\0"
   /* _mesa_function_pool[20835]: VertexAttrib4NsvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Nsv\0"
   "glVertexAttrib4NsvARB\0"
   "\0"
   /* _mesa_function_pool[20880]: VertexAttrib3fvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib3fv\0"
   "glVertexAttrib3fvARB\0"
   "\0"
   /* _mesa_function_pool[20923]: ClearColor (offset 206) */
   "ffff\0"
   "glClearColor\0"
   "\0"
   /* _mesa_function_pool[20942]: GetSynciv (will be remapped) */
   "iiipp\0"
   "glGetSynciv\0"
   "\0"
   /* _mesa_function_pool[20961]: ClearColorIiEXT (will be remapped) */
   "iiii\0"
   "glClearColorIiEXT\0"
   "\0"
   /* _mesa_function_pool[20985]: DeleteFramebuffersEXT (will be remapped) */
   "ip\0"
   "glDeleteFramebuffers\0"
   "glDeleteFramebuffersEXT\0"
   "\0"
   /* _mesa_function_pool[21034]: GlobalAlphaFactorsSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactorsSUN\0"
   "\0"
   /* _mesa_function_pool[21061]: IsEnabledIndexedEXT (will be remapped) */
   "ii\0"
   "glIsEnabledIndexedEXT\0"
   "\0"
   /* _mesa_function_pool[21087]: TexEnviv (offset 187) */
   "iip\0"
   "glTexEnviv\0"
   "\0"
   /* _mesa_function_pool[21103]: TexSubImage3D (offset 372) */
   "iiiiiiiiiip\0"
   "glTexSubImage3D\0"
   "glTexSubImage3DEXT\0"
   "\0"
   /* _mesa_function_pool[21151]: Tangent3fEXT (dynamic) */
   "fff\0"
   "glTangent3fEXT\0"
   "\0"
   /* _mesa_function_pool[21171]: SecondaryColor3uivEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3uiv\0"
   "glSecondaryColor3uivEXT\0"
   "\0"
   /* _mesa_function_pool[21219]: MatrixIndexubvARB (dynamic) */
   "ip\0"
   "glMatrixIndexubvARB\0"
   "\0"
   /* _mesa_function_pool[21243]: Color4fNormal3fVertex3fSUN (dynamic) */
   "ffffffffff\0"
   "glColor4fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[21284]: PixelTexGenParameterfSGIS (will be remapped) */
   "if\0"
   "glPixelTexGenParameterfSGIS\0"
   "\0"
   /* _mesa_function_pool[21316]: CreateShader (will be remapped) */
   "i\0"
   "glCreateShader\0"
   "\0"
   /* _mesa_function_pool[21334]: GetColorTableParameterfv (offset 344) */
   "iip\0"
   "glGetColorTableParameterfv\0"
   "glGetColorTableParameterfvSGI\0"
   "glGetColorTableParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[21426]: FragmentLightModelfvSGIX (dynamic) */
   "ip\0"
   "glFragmentLightModelfvSGIX\0"
   "\0"
   /* _mesa_function_pool[21457]: Bitmap (offset 8) */
   "iiffffp\0"
   "glBitmap\0"
   "\0"
   /* _mesa_function_pool[21475]: MultiTexCoord3fARB (offset 394) */
   "ifff\0"
   "glMultiTexCoord3f\0"
   "glMultiTexCoord3fARB\0"
   "\0"
   /* _mesa_function_pool[21520]: GetTexLevelParameterfv (offset 284) */
   "iiip\0"
   "glGetTexLevelParameterfv\0"
   "\0"
   /* _mesa_function_pool[21551]: GetPixelTexGenParameterfvSGIS (will be remapped) */
   "ip\0"
   "glGetPixelTexGenParameterfvSGIS\0"
   "\0"
   /* _mesa_function_pool[21587]: GenFramebuffersEXT (will be remapped) */
   "ip\0"
   "glGenFramebuffers\0"
   "glGenFramebuffersEXT\0"
   "\0"
   /* _mesa_function_pool[21630]: GetProgramParameterdvNV (will be remapped) */
   "iiip\0"
   "glGetProgramParameterdvNV\0"
   "\0"
   /* _mesa_function_pool[21662]: Vertex2sv (offset 133) */
   "p\0"
   "glVertex2sv\0"
   "\0"
   /* _mesa_function_pool[21677]: GetIntegerv (offset 263) */
   "ip\0"
   "glGetIntegerv\0"
   "\0"
   /* _mesa_function_pool[21695]: IsVertexArrayAPPLE (will be remapped) */
   "i\0"
   "glIsVertexArray\0"
   "glIsVertexArrayAPPLE\0"
   "\0"
   /* _mesa_function_pool[21735]: FragmentLightfvSGIX (dynamic) */
   "iip\0"
   "glFragmentLightfvSGIX\0"
   "\0"
   /* _mesa_function_pool[21762]: DetachShader (will be remapped) */
   "ii\0"
   "glDetachShader\0"
   "\0"
   /* _mesa_function_pool[21781]: VertexAttrib4NubARB (will be remapped) */
   "iiiii\0"
   "glVertexAttrib4Nub\0"
   "glVertexAttrib4NubARB\0"
   "\0"
   /* _mesa_function_pool[21829]: GetProgramEnvParameterfvARB (will be remapped) */
   "iip\0"
   "glGetProgramEnvParameterfvARB\0"
   "\0"
   /* _mesa_function_pool[21864]: GetTrackMatrixivNV (will be remapped) */
   "iiip\0"
   "glGetTrackMatrixivNV\0"
   "\0"
   /* _mesa_function_pool[21891]: VertexAttrib3svNV (will be remapped) */
   "ip\0"
   "glVertexAttrib3svNV\0"
   "\0"
   /* _mesa_function_pool[21915]: Uniform4fvARB (will be remapped) */
   "iip\0"
   "glUniform4fv\0"
   "glUniform4fvARB\0"
   "\0"
   /* _mesa_function_pool[21949]: MultTransposeMatrixfARB (will be remapped) */
   "p\0"
   "glMultTransposeMatrixf\0"
   "glMultTransposeMatrixfARB\0"
   "\0"
   /* _mesa_function_pool[22001]: GetTexEnviv (offset 277) */
   "iip\0"
   "glGetTexEnviv\0"
   "\0"
   /* _mesa_function_pool[22020]: ColorFragmentOp1ATI (will be remapped) */
   "iiiiiii\0"
   "glColorFragmentOp1ATI\0"
   "\0"
   /* _mesa_function_pool[22051]: GetUniformfvARB (will be remapped) */
   "iip\0"
   "glGetUniformfv\0"
   "glGetUniformfvARB\0"
   "\0"
   /* _mesa_function_pool[22089]: EGLImageTargetRenderbufferStorageOES (will be remapped) */
   "ip\0"
   "glEGLImageTargetRenderbufferStorageOES\0"
   "\0"
   /* _mesa_function_pool[22132]: PopClientAttrib (offset 334) */
   "\0"
   "glPopClientAttrib\0"
   "\0"
   /* _mesa_function_pool[22152]: ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN (dynamic) */
   "iffffffffffff\0"
   "glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[22223]: DetachObjectARB (will be remapped) */
   "ii\0"
   "glDetachObjectARB\0"
   "\0"
   /* _mesa_function_pool[22245]: VertexBlendARB (dynamic) */
   "i\0"
   "glVertexBlendARB\0"
   "\0"
   /* _mesa_function_pool[22265]: WindowPos3iMESA (will be remapped) */
   "iii\0"
   "glWindowPos3i\0"
   "glWindowPos3iARB\0"
   "glWindowPos3iMESA\0"
   "\0"
   /* _mesa_function_pool[22319]: SeparableFilter2D (offset 360) */
   "iiiiiipp\0"
   "glSeparableFilter2D\0"
   "glSeparableFilter2DEXT\0"
   "\0"
   /* _mesa_function_pool[22372]: ProgramParameteriARB (will be remapped) */
   "iii\0"
   "glProgramParameteriARB\0"
   "\0"
   /* _mesa_function_pool[22400]: Map1d (offset 220) */
   "iddiip\0"
   "glMap1d\0"
   "\0"
   /* _mesa_function_pool[22416]: Map1f (offset 221) */
   "iffiip\0"
   "glMap1f\0"
   "\0"
   /* _mesa_function_pool[22432]: CompressedTexImage2DARB (will be remapped) */
   "iiiiiiip\0"
   "glCompressedTexImage2D\0"
   "glCompressedTexImage2DARB\0"
   "\0"
   /* _mesa_function_pool[22491]: ArrayElement (offset 306) */
   "i\0"
   "glArrayElement\0"
   "glArrayElementEXT\0"
   "\0"
   /* _mesa_function_pool[22527]: TexImage2D (offset 183) */
   "iiiiiiiip\0"
   "glTexImage2D\0"
   "\0"
   /* _mesa_function_pool[22551]: DepthBoundsEXT (will be remapped) */
   "dd\0"
   "glDepthBoundsEXT\0"
   "\0"
   /* _mesa_function_pool[22572]: ProgramParameters4fvNV (will be remapped) */
   "iiip\0"
   "glProgramParameters4fvNV\0"
   "\0"
   /* _mesa_function_pool[22603]: DeformationMap3fSGIX (dynamic) */
   "iffiiffiiffiip\0"
   "glDeformationMap3fSGIX\0"
   "\0"
   /* _mesa_function_pool[22642]: GetProgramivNV (will be remapped) */
   "iip\0"
   "glGetProgramivNV\0"
   "\0"
   /* _mesa_function_pool[22664]: GetMinmaxParameteriv (offset 366) */
   "iip\0"
   "glGetMinmaxParameteriv\0"
   "glGetMinmaxParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[22718]: PixelTransferf (offset 247) */
   "if\0"
   "glPixelTransferf\0"
   "\0"
   /* _mesa_function_pool[22739]: CopyTexImage1D (offset 323) */
   "iiiiiii\0"
   "glCopyTexImage1D\0"
   "glCopyTexImage1DEXT\0"
   "\0"
   /* _mesa_function_pool[22785]: PushMatrix (offset 298) */
   "\0"
   "glPushMatrix\0"
   "\0"
   /* _mesa_function_pool[22800]: Fogiv (offset 156) */
   "ip\0"
   "glFogiv\0"
   "\0"
   /* _mesa_function_pool[22812]: TexCoord1dv (offset 95) */
   "p\0"
   "glTexCoord1dv\0"
   "\0"
   /* _mesa_function_pool[22829]: AlphaFragmentOp3ATI (will be remapped) */
   "iiiiiiiiiiii\0"
   "glAlphaFragmentOp3ATI\0"
   "\0"
   /* _mesa_function_pool[22865]: PixelTransferi (offset 248) */
   "ii\0"
   "glPixelTransferi\0"
   "\0"
   /* _mesa_function_pool[22886]: GetVertexAttribdvNV (will be remapped) */
   "iip\0"
   "glGetVertexAttribdvNV\0"
   "\0"
   /* _mesa_function_pool[22913]: VertexAttrib3fvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib3fvNV\0"
   "\0"
   /* _mesa_function_pool[22937]: Rotatef (offset 300) */
   "ffff\0"
   "glRotatef\0"
   "\0"
   /* _mesa_function_pool[22953]: GetFinalCombinerInputParameterivNV (will be remapped) */
   "iip\0"
   "glGetFinalCombinerInputParameterivNV\0"
   "\0"
   /* _mesa_function_pool[22995]: Vertex3i (offset 138) */
   "iii\0"
   "glVertex3i\0"
   "\0"
   /* _mesa_function_pool[23011]: Vertex3f (offset 136) */
   "fff\0"
   "glVertex3f\0"
   "\0"
   /* _mesa_function_pool[23027]: Clear (offset 203) */
   "i\0"
   "glClear\0"
   "\0"
   /* _mesa_function_pool[23038]: Vertex3d (offset 134) */
   "ddd\0"
   "glVertex3d\0"
   "\0"
   /* _mesa_function_pool[23054]: GetMapParameterivNV (dynamic) */
   "iip\0"
   "glGetMapParameterivNV\0"
   "\0"
   /* _mesa_function_pool[23081]: Uniform4iARB (will be remapped) */
   "iiiii\0"
   "glUniform4i\0"
   "glUniform4iARB\0"
   "\0"
   /* _mesa_function_pool[23115]: ReadBuffer (offset 254) */
   "i\0"
   "glReadBuffer\0"
   "\0"
   /* _mesa_function_pool[23131]: ConvolutionParameteri (offset 352) */
   "iii\0"
   "glConvolutionParameteri\0"
   "glConvolutionParameteriEXT\0"
   "\0"
   /* _mesa_function_pool[23187]: Ortho (offset 296) */
   "dddddd\0"
   "glOrtho\0"
   "\0"
   /* _mesa_function_pool[23203]: Binormal3sEXT (dynamic) */
   "iii\0"
   "glBinormal3sEXT\0"
   "\0"
   /* _mesa_function_pool[23224]: ListBase (offset 6) */
   "i\0"
   "glListBase\0"
   "\0"
   /* _mesa_function_pool[23238]: Vertex3s (offset 140) */
   "iii\0"
   "glVertex3s\0"
   "\0"
   /* _mesa_function_pool[23254]: ConvolutionParameterf (offset 350) */
   "iif\0"
   "glConvolutionParameterf\0"
   "glConvolutionParameterfEXT\0"
   "\0"
   /* _mesa_function_pool[23310]: GetColorTableParameteriv (offset 345) */
   "iip\0"
   "glGetColorTableParameteriv\0"
   "glGetColorTableParameterivSGI\0"
   "glGetColorTableParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[23402]: ProgramEnvParameter4dvARB (will be remapped) */
   "iip\0"
   "glProgramEnvParameter4dvARB\0"
   "glProgramParameter4dvNV\0"
   "\0"
   /* _mesa_function_pool[23459]: ShadeModel (offset 177) */
   "i\0"
   "glShadeModel\0"
   "\0"
   /* _mesa_function_pool[23475]: VertexAttribs2fvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs2fvNV\0"
   "\0"
   /* _mesa_function_pool[23501]: Rectiv (offset 91) */
   "pp\0"
   "glRectiv\0"
   "\0"
   /* _mesa_function_pool[23514]: UseProgramObjectARB (will be remapped) */
   "i\0"
   "glUseProgram\0"
   "glUseProgramObjectARB\0"
   "\0"
   /* _mesa_function_pool[23552]: GetMapParameterfvNV (dynamic) */
   "iip\0"
   "glGetMapParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[23579]: EndConditionalRenderNV (will be remapped) */
   "\0"
   "glEndConditionalRenderNV\0"
   "\0"
   /* _mesa_function_pool[23606]: PassTexCoordATI (will be remapped) */
   "iii\0"
   "glPassTexCoordATI\0"
   "\0"
   /* _mesa_function_pool[23629]: DeleteProgram (will be remapped) */
   "i\0"
   "glDeleteProgram\0"
   "\0"
   /* _mesa_function_pool[23648]: Tangent3ivEXT (dynamic) */
   "p\0"
   "glTangent3ivEXT\0"
   "\0"
   /* _mesa_function_pool[23667]: Tangent3dEXT (dynamic) */
   "ddd\0"
   "glTangent3dEXT\0"
   "\0"
   /* _mesa_function_pool[23687]: SecondaryColor3dvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3dv\0"
   "glSecondaryColor3dvEXT\0"
   "\0"
   /* _mesa_function_pool[23733]: Vertex2fv (offset 129) */
   "p\0"
   "glVertex2fv\0"
   "\0"
   /* _mesa_function_pool[23748]: MultiDrawArraysEXT (will be remapped) */
   "ippi\0"
   "glMultiDrawArrays\0"
   "glMultiDrawArraysEXT\0"
   "\0"
   /* _mesa_function_pool[23793]: BindRenderbufferEXT (will be remapped) */
   "ii\0"
   "glBindRenderbuffer\0"
   "glBindRenderbufferEXT\0"
   "\0"
   /* _mesa_function_pool[23838]: MultiTexCoord4dARB (offset 400) */
   "idddd\0"
   "glMultiTexCoord4d\0"
   "glMultiTexCoord4dARB\0"
   "\0"
   /* _mesa_function_pool[23884]: FramebufferTextureFaceARB (will be remapped) */
   "iiiii\0"
   "glFramebufferTextureFaceARB\0"
   "\0"
   /* _mesa_function_pool[23919]: Vertex3sv (offset 141) */
   "p\0"
   "glVertex3sv\0"
   "\0"
   /* _mesa_function_pool[23934]: SecondaryColor3usEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3us\0"
   "glSecondaryColor3usEXT\0"
   "\0"
   /* _mesa_function_pool[23982]: ProgramLocalParameter4fvARB (will be remapped) */
   "iip\0"
   "glProgramLocalParameter4fvARB\0"
   "\0"
   /* _mesa_function_pool[24017]: DeleteProgramsNV (will be remapped) */
   "ip\0"
   "glDeleteProgramsARB\0"
   "glDeleteProgramsNV\0"
   "\0"
   /* _mesa_function_pool[24060]: EvalMesh1 (offset 236) */
   "iii\0"
   "glEvalMesh1\0"
   "\0"
   /* _mesa_function_pool[24077]: PauseTransformFeedback (will be remapped) */
   "\0"
   "glPauseTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[24104]: MultiTexCoord1sARB (offset 382) */
   "ii\0"
   "glMultiTexCoord1s\0"
   "glMultiTexCoord1sARB\0"
   "\0"
   /* _mesa_function_pool[24147]: ReplacementCodeuiColor3fVertex3fSUN (dynamic) */
   "iffffff\0"
   "glReplacementCodeuiColor3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[24194]: GetVertexAttribPointervNV (will be remapped) */
   "iip\0"
   "glGetVertexAttribPointerv\0"
   "glGetVertexAttribPointervARB\0"
   "glGetVertexAttribPointervNV\0"
   "\0"
   /* _mesa_function_pool[24282]: VertexAttribs1fvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs1fvNV\0"
   "\0"
   /* _mesa_function_pool[24308]: MultiTexCoord1dvARB (offset 377) */
   "ip\0"
   "glMultiTexCoord1dv\0"
   "glMultiTexCoord1dvARB\0"
   "\0"
   /* _mesa_function_pool[24353]: Uniform2iARB (will be remapped) */
   "iii\0"
   "glUniform2i\0"
   "glUniform2iARB\0"
   "\0"
   /* _mesa_function_pool[24385]: Vertex2iv (offset 131) */
   "p\0"
   "glVertex2iv\0"
   "\0"
   /* _mesa_function_pool[24400]: GetProgramStringNV (will be remapped) */
   "iip\0"
   "glGetProgramStringNV\0"
   "\0"
   /* _mesa_function_pool[24426]: ColorPointerEXT (will be remapped) */
   "iiiip\0"
   "glColorPointerEXT\0"
   "\0"
   /* _mesa_function_pool[24451]: LineWidth (offset 168) */
   "f\0"
   "glLineWidth\0"
   "\0"
   /* _mesa_function_pool[24466]: MapBufferARB (will be remapped) */
   "ii\0"
   "glMapBuffer\0"
   "glMapBufferARB\0"
   "\0"
   /* _mesa_function_pool[24497]: MultiDrawElementsBaseVertex (will be remapped) */
   "ipipip\0"
   "glMultiDrawElementsBaseVertex\0"
   "\0"
   /* _mesa_function_pool[24535]: TexParameterIuivEXT (will be remapped) */
   "iip\0"
   "glTexParameterIuivEXT\0"
   "\0"
   /* _mesa_function_pool[24562]: Binormal3svEXT (dynamic) */
   "p\0"
   "glBinormal3svEXT\0"
   "\0"
   /* _mesa_function_pool[24582]: ApplyTextureEXT (dynamic) */
   "i\0"
   "glApplyTextureEXT\0"
   "\0"
   /* _mesa_function_pool[24603]: TexGendv (offset 189) */
   "iip\0"
   "glTexGendv\0"
   "\0"
   /* _mesa_function_pool[24619]: EnableIndexedEXT (will be remapped) */
   "ii\0"
   "glEnableIndexedEXT\0"
   "\0"
   /* _mesa_function_pool[24642]: TextureMaterialEXT (dynamic) */
   "ii\0"
   "glTextureMaterialEXT\0"
   "\0"
   /* _mesa_function_pool[24667]: TextureLightEXT (dynamic) */
   "i\0"
   "glTextureLightEXT\0"
   "\0"
   /* _mesa_function_pool[24688]: ResetMinmax (offset 370) */
   "i\0"
   "glResetMinmax\0"
   "glResetMinmaxEXT\0"
   "\0"
   /* _mesa_function_pool[24722]: SpriteParameterfSGIX (dynamic) */
   "if\0"
   "glSpriteParameterfSGIX\0"
   "\0"
   /* _mesa_function_pool[24749]: EnableClientState (offset 313) */
   "i\0"
   "glEnableClientState\0"
   "\0"
   /* _mesa_function_pool[24772]: VertexAttrib4sNV (will be remapped) */
   "iiiii\0"
   "glVertexAttrib4sNV\0"
   "\0"
   /* _mesa_function_pool[24798]: GetConvolutionParameterfv (offset 357) */
   "iip\0"
   "glGetConvolutionParameterfv\0"
   "glGetConvolutionParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[24862]: VertexAttribs4dvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs4dvNV\0"
   "\0"
   /* _mesa_function_pool[24888]: VertexAttrib4dARB (will be remapped) */
   "idddd\0"
   "glVertexAttrib4d\0"
   "glVertexAttrib4dARB\0"
   "\0"
   /* _mesa_function_pool[24932]: GetTexBumpParameterfvATI (will be remapped) */
   "ip\0"
   "glGetTexBumpParameterfvATI\0"
   "\0"
   /* _mesa_function_pool[24963]: ProgramNamedParameter4dNV (will be remapped) */
   "iipdddd\0"
   "glProgramNamedParameter4dNV\0"
   "\0"
   /* _mesa_function_pool[25000]: GetMaterialfv (offset 269) */
   "iip\0"
   "glGetMaterialfv\0"
   "\0"
   /* _mesa_function_pool[25021]: VertexWeightfEXT (dynamic) */
   "f\0"
   "glVertexWeightfEXT\0"
   "\0"
   /* _mesa_function_pool[25043]: Binormal3fEXT (dynamic) */
   "fff\0"
   "glBinormal3fEXT\0"
   "\0"
   /* _mesa_function_pool[25064]: CallList (offset 2) */
   "i\0"
   "glCallList\0"
   "\0"
   /* _mesa_function_pool[25078]: Materialfv (offset 170) */
   "iip\0"
   "glMaterialfv\0"
   "\0"
   /* _mesa_function_pool[25096]: TexCoord3fv (offset 113) */
   "p\0"
   "glTexCoord3fv\0"
   "\0"
   /* _mesa_function_pool[25113]: FogCoordfvEXT (will be remapped) */
   "p\0"
   "glFogCoordfv\0"
   "glFogCoordfvEXT\0"
   "\0"
   /* _mesa_function_pool[25145]: MultiTexCoord1ivARB (offset 381) */
   "ip\0"
   "glMultiTexCoord1iv\0"
   "glMultiTexCoord1ivARB\0"
   "\0"
   /* _mesa_function_pool[25190]: SecondaryColor3ubEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3ub\0"
   "glSecondaryColor3ubEXT\0"
   "\0"
   /* _mesa_function_pool[25238]: MultiTexCoord2ivARB (offset 389) */
   "ip\0"
   "glMultiTexCoord2iv\0"
   "glMultiTexCoord2ivARB\0"
   "\0"
   /* _mesa_function_pool[25283]: FogFuncSGIS (dynamic) */
   "ip\0"
   "glFogFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[25301]: CopyTexSubImage2D (offset 326) */
   "iiiiiiii\0"
   "glCopyTexSubImage2D\0"
   "glCopyTexSubImage2DEXT\0"
   "\0"
   /* _mesa_function_pool[25354]: GetObjectParameterivARB (will be remapped) */
   "iip\0"
   "glGetObjectParameterivARB\0"
   "\0"
   /* _mesa_function_pool[25385]: Color3iv (offset 16) */
   "p\0"
   "glColor3iv\0"
   "\0"
   /* _mesa_function_pool[25399]: TexCoord4fVertex4fSUN (dynamic) */
   "ffffffff\0"
   "glTexCoord4fVertex4fSUN\0"
   "\0"
   /* _mesa_function_pool[25433]: DrawElements (offset 311) */
   "iiip\0"
   "glDrawElements\0"
   "\0"
   /* _mesa_function_pool[25454]: BindVertexArrayAPPLE (will be remapped) */
   "i\0"
   "glBindVertexArrayAPPLE\0"
   "\0"
   /* _mesa_function_pool[25480]: GetProgramLocalParameterdvARB (will be remapped) */
   "iip\0"
   "glGetProgramLocalParameterdvARB\0"
   "\0"
   /* _mesa_function_pool[25517]: GetHistogramParameteriv (offset 363) */
   "iip\0"
   "glGetHistogramParameteriv\0"
   "glGetHistogramParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[25577]: MultiTexCoord1iARB (offset 380) */
   "ii\0"
   "glMultiTexCoord1i\0"
   "glMultiTexCoord1iARB\0"
   "\0"
   /* _mesa_function_pool[25620]: GetConvolutionFilter (offset 356) */
   "iiip\0"
   "glGetConvolutionFilter\0"
   "glGetConvolutionFilterEXT\0"
   "\0"
   /* _mesa_function_pool[25675]: GetProgramivARB (will be remapped) */
   "iip\0"
   "glGetProgramivARB\0"
   "\0"
   /* _mesa_function_pool[25698]: BlendFuncSeparateEXT (will be remapped) */
   "iiii\0"
   "glBlendFuncSeparate\0"
   "glBlendFuncSeparateEXT\0"
   "glBlendFuncSeparateINGR\0"
   "\0"
   /* _mesa_function_pool[25771]: MapBufferRange (will be remapped) */
   "iiii\0"
   "glMapBufferRange\0"
   "\0"
   /* _mesa_function_pool[25794]: ProgramParameters4dvNV (will be remapped) */
   "iiip\0"
   "glProgramParameters4dvNV\0"
   "\0"
   /* _mesa_function_pool[25825]: TexCoord2fColor3fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glTexCoord2fColor3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[25862]: EvalPoint2 (offset 239) */
   "ii\0"
   "glEvalPoint2\0"
   "\0"
   /* _mesa_function_pool[25879]: EvalPoint1 (offset 237) */
   "i\0"
   "glEvalPoint1\0"
   "\0"
   /* _mesa_function_pool[25895]: Binormal3dvEXT (dynamic) */
   "p\0"
   "glBinormal3dvEXT\0"
   "\0"
   /* _mesa_function_pool[25915]: PopMatrix (offset 297) */
   "\0"
   "glPopMatrix\0"
   "\0"
   /* _mesa_function_pool[25929]: FinishFenceNV (will be remapped) */
   "i\0"
   "glFinishFenceNV\0"
   "\0"
   /* _mesa_function_pool[25948]: GetFogFuncSGIS (dynamic) */
   "p\0"
   "glGetFogFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[25968]: GetUniformLocationARB (will be remapped) */
   "ip\0"
   "glGetUniformLocation\0"
   "glGetUniformLocationARB\0"
   "\0"
   /* _mesa_function_pool[26017]: SecondaryColor3fEXT (will be remapped) */
   "fff\0"
   "glSecondaryColor3f\0"
   "glSecondaryColor3fEXT\0"
   "\0"
   /* _mesa_function_pool[26063]: GetTexGeniv (offset 280) */
   "iip\0"
   "glGetTexGeniv\0"
   "\0"
   /* _mesa_function_pool[26082]: CombinerInputNV (will be remapped) */
   "iiiiii\0"
   "glCombinerInputNV\0"
   "\0"
   /* _mesa_function_pool[26108]: VertexAttrib3sARB (will be remapped) */
   "iiii\0"
   "glVertexAttrib3s\0"
   "glVertexAttrib3sARB\0"
   "\0"
   /* _mesa_function_pool[26151]: IsTransformFeedback (will be remapped) */
   "i\0"
   "glIsTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[26176]: ReplacementCodeuiNormal3fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glReplacementCodeuiNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[26221]: Map2d (offset 222) */
   "iddiiddiip\0"
   "glMap2d\0"
   "\0"
   /* _mesa_function_pool[26241]: Map2f (offset 223) */
   "iffiiffiip\0"
   "glMap2f\0"
   "\0"
   /* _mesa_function_pool[26261]: ProgramStringARB (will be remapped) */
   "iiip\0"
   "glProgramStringARB\0"
   "\0"
   /* _mesa_function_pool[26286]: Vertex4s (offset 148) */
   "iiii\0"
   "glVertex4s\0"
   "\0"
   /* _mesa_function_pool[26303]: TexCoord4fVertex4fvSUN (dynamic) */
   "pp\0"
   "glTexCoord4fVertex4fvSUN\0"
   "\0"
   /* _mesa_function_pool[26332]: FragmentLightModelivSGIX (dynamic) */
   "ip\0"
   "glFragmentLightModelivSGIX\0"
   "\0"
   /* _mesa_function_pool[26363]: VertexAttrib1fNV (will be remapped) */
   "if\0"
   "glVertexAttrib1fNV\0"
   "\0"
   /* _mesa_function_pool[26386]: Vertex4f (offset 144) */
   "ffff\0"
   "glVertex4f\0"
   "\0"
   /* _mesa_function_pool[26403]: EvalCoord1d (offset 228) */
   "d\0"
   "glEvalCoord1d\0"
   "\0"
   /* _mesa_function_pool[26420]: Vertex4d (offset 142) */
   "dddd\0"
   "glVertex4d\0"
   "\0"
   /* _mesa_function_pool[26437]: RasterPos4dv (offset 79) */
   "p\0"
   "glRasterPos4dv\0"
   "\0"
   /* _mesa_function_pool[26455]: FragmentLightfSGIX (dynamic) */
   "iif\0"
   "glFragmentLightfSGIX\0"
   "\0"
   /* _mesa_function_pool[26481]: GetCompressedTexImageARB (will be remapped) */
   "iip\0"
   "glGetCompressedTexImage\0"
   "glGetCompressedTexImageARB\0"
   "\0"
   /* _mesa_function_pool[26537]: GetTexGenfv (offset 279) */
   "iip\0"
   "glGetTexGenfv\0"
   "\0"
   /* _mesa_function_pool[26556]: Vertex4i (offset 146) */
   "iiii\0"
   "glVertex4i\0"
   "\0"
   /* _mesa_function_pool[26573]: VertexWeightPointerEXT (dynamic) */
   "iiip\0"
   "glVertexWeightPointerEXT\0"
   "\0"
   /* _mesa_function_pool[26604]: GetHistogram (offset 361) */
   "iiiip\0"
   "glGetHistogram\0"
   "glGetHistogramEXT\0"
   "\0"
   /* _mesa_function_pool[26644]: ActiveStencilFaceEXT (will be remapped) */
   "i\0"
   "glActiveStencilFaceEXT\0"
   "\0"
   /* _mesa_function_pool[26670]: StencilFuncSeparateATI (will be remapped) */
   "iiii\0"
   "glStencilFuncSeparateATI\0"
   "\0"
   /* _mesa_function_pool[26701]: Materialf (offset 169) */
   "iif\0"
   "glMaterialf\0"
   "\0"
   /* _mesa_function_pool[26718]: GetShaderSourceARB (will be remapped) */
   "iipp\0"
   "glGetShaderSource\0"
   "glGetShaderSourceARB\0"
   "\0"
   /* _mesa_function_pool[26763]: IglooInterfaceSGIX (dynamic) */
   "ip\0"
   "glIglooInterfaceSGIX\0"
   "\0"
   /* _mesa_function_pool[26788]: Materiali (offset 171) */
   "iii\0"
   "glMateriali\0"
   "\0"
   /* _mesa_function_pool[26805]: VertexAttrib4dNV (will be remapped) */
   "idddd\0"
   "glVertexAttrib4dNV\0"
   "\0"
   /* _mesa_function_pool[26831]: MultiModeDrawElementsIBM (will be remapped) */
   "ppipii\0"
   "glMultiModeDrawElementsIBM\0"
   "\0"
   /* _mesa_function_pool[26866]: Indexsv (offset 51) */
   "p\0"
   "glIndexsv\0"
   "\0"
   /* _mesa_function_pool[26879]: MultiTexCoord4svARB (offset 407) */
   "ip\0"
   "glMultiTexCoord4sv\0"
   "glMultiTexCoord4svARB\0"
   "\0"
   /* _mesa_function_pool[26924]: LightModelfv (offset 164) */
   "ip\0"
   "glLightModelfv\0"
   "\0"
   /* _mesa_function_pool[26943]: TexCoord2dv (offset 103) */
   "p\0"
   "glTexCoord2dv\0"
   "\0"
   /* _mesa_function_pool[26960]: GenQueriesARB (will be remapped) */
   "ip\0"
   "glGenQueries\0"
   "glGenQueriesARB\0"
   "\0"
   /* _mesa_function_pool[26993]: EvalCoord1dv (offset 229) */
   "p\0"
   "glEvalCoord1dv\0"
   "\0"
   /* _mesa_function_pool[27011]: ReplacementCodeuiVertex3fSUN (dynamic) */
   "ifff\0"
   "glReplacementCodeuiVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[27048]: Translated (offset 303) */
   "ddd\0"
   "glTranslated\0"
   "\0"
   /* _mesa_function_pool[27066]: Translatef (offset 304) */
   "fff\0"
   "glTranslatef\0"
   "\0"
   /* _mesa_function_pool[27084]: StencilMask (offset 209) */
   "i\0"
   "glStencilMask\0"
   "\0"
   /* _mesa_function_pool[27101]: Tangent3iEXT (dynamic) */
   "iii\0"
   "glTangent3iEXT\0"
   "\0"
   /* _mesa_function_pool[27121]: GetLightiv (offset 265) */
   "iip\0"
   "glGetLightiv\0"
   "\0"
   /* _mesa_function_pool[27139]: DrawMeshArraysSUN (dynamic) */
   "iiii\0"
   "glDrawMeshArraysSUN\0"
   "\0"
   /* _mesa_function_pool[27165]: IsList (offset 287) */
   "i\0"
   "glIsList\0"
   "\0"
   /* _mesa_function_pool[27177]: IsSync (will be remapped) */
   "i\0"
   "glIsSync\0"
   "\0"
   /* _mesa_function_pool[27189]: RenderMode (offset 196) */
   "i\0"
   "glRenderMode\0"
   "\0"
   /* _mesa_function_pool[27205]: GetMapControlPointsNV (dynamic) */
   "iiiiiip\0"
   "glGetMapControlPointsNV\0"
   "\0"
   /* _mesa_function_pool[27238]: DrawBuffersARB (will be remapped) */
   "ip\0"
   "glDrawBuffers\0"
   "glDrawBuffersARB\0"
   "glDrawBuffersATI\0"
   "\0"
   /* _mesa_function_pool[27290]: ProgramLocalParameter4fARB (will be remapped) */
   "iiffff\0"
   "glProgramLocalParameter4fARB\0"
   "\0"
   /* _mesa_function_pool[27327]: SpriteParameterivSGIX (dynamic) */
   "ip\0"
   "glSpriteParameterivSGIX\0"
   "\0"
   /* _mesa_function_pool[27355]: ProvokingVertexEXT (will be remapped) */
   "i\0"
   "glProvokingVertexEXT\0"
   "glProvokingVertex\0"
   "\0"
   /* _mesa_function_pool[27397]: MultiTexCoord1fARB (offset 378) */
   "if\0"
   "glMultiTexCoord1f\0"
   "glMultiTexCoord1fARB\0"
   "\0"
   /* _mesa_function_pool[27440]: LoadName (offset 198) */
   "i\0"
   "glLoadName\0"
   "\0"
   /* _mesa_function_pool[27454]: VertexAttribs4ubvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs4ubvNV\0"
   "\0"
   /* _mesa_function_pool[27481]: WeightsvARB (dynamic) */
   "ip\0"
   "glWeightsvARB\0"
   "\0"
   /* _mesa_function_pool[27499]: Uniform1fvARB (will be remapped) */
   "iip\0"
   "glUniform1fv\0"
   "glUniform1fvARB\0"
   "\0"
   /* _mesa_function_pool[27533]: CopyTexSubImage1D (offset 325) */
   "iiiiii\0"
   "glCopyTexSubImage1D\0"
   "glCopyTexSubImage1DEXT\0"
   "\0"
   /* _mesa_function_pool[27584]: CullFace (offset 152) */
   "i\0"
   "glCullFace\0"
   "\0"
   /* _mesa_function_pool[27598]: BindTexture (offset 307) */
   "ii\0"
   "glBindTexture\0"
   "glBindTextureEXT\0"
   "\0"
   /* _mesa_function_pool[27633]: BeginFragmentShaderATI (will be remapped) */
   "\0"
   "glBeginFragmentShaderATI\0"
   "\0"
   /* _mesa_function_pool[27660]: MultiTexCoord4fARB (offset 402) */
   "iffff\0"
   "glMultiTexCoord4f\0"
   "glMultiTexCoord4fARB\0"
   "\0"
   /* _mesa_function_pool[27706]: VertexAttribs3svNV (will be remapped) */
   "iip\0"
   "glVertexAttribs3svNV\0"
   "\0"
   /* _mesa_function_pool[27732]: StencilFunc (offset 243) */
   "iii\0"
   "glStencilFunc\0"
   "\0"
   /* _mesa_function_pool[27751]: CopyPixels (offset 255) */
   "iiiii\0"
   "glCopyPixels\0"
   "\0"
   /* _mesa_function_pool[27771]: Rectsv (offset 93) */
   "pp\0"
   "glRectsv\0"
   "\0"
   /* _mesa_function_pool[27784]: ReplacementCodeuivSUN (dynamic) */
   "p\0"
   "glReplacementCodeuivSUN\0"
   "\0"
   /* _mesa_function_pool[27811]: EnableVertexAttribArrayARB (will be remapped) */
   "i\0"
   "glEnableVertexAttribArray\0"
   "glEnableVertexAttribArrayARB\0"
   "\0"
   /* _mesa_function_pool[27869]: NormalPointervINTEL (dynamic) */
   "ip\0"
   "glNormalPointervINTEL\0"
   "\0"
   /* _mesa_function_pool[27895]: CopyConvolutionFilter2D (offset 355) */
   "iiiiii\0"
   "glCopyConvolutionFilter2D\0"
   "glCopyConvolutionFilter2DEXT\0"
   "\0"
   /* _mesa_function_pool[27958]: WindowPos3ivMESA (will be remapped) */
   "p\0"
   "glWindowPos3iv\0"
   "glWindowPos3ivARB\0"
   "glWindowPos3ivMESA\0"
   "\0"
   /* _mesa_function_pool[28013]: CopyBufferSubData (will be remapped) */
   "iiiii\0"
   "glCopyBufferSubData\0"
   "\0"
   /* _mesa_function_pool[28040]: NormalPointer (offset 318) */
   "iip\0"
   "glNormalPointer\0"
   "\0"
   /* _mesa_function_pool[28061]: TexParameterfv (offset 179) */
   "iip\0"
   "glTexParameterfv\0"
   "\0"
   /* _mesa_function_pool[28083]: IsBufferARB (will be remapped) */
   "i\0"
   "glIsBuffer\0"
   "glIsBufferARB\0"
   "\0"
   /* _mesa_function_pool[28111]: WindowPos4iMESA (will be remapped) */
   "iiii\0"
   "glWindowPos4iMESA\0"
   "\0"
   /* _mesa_function_pool[28135]: VertexAttrib4uivARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4uiv\0"
   "glVertexAttrib4uivARB\0"
   "\0"
   /* _mesa_function_pool[28180]: Tangent3bvEXT (dynamic) */
   "p\0"
   "glTangent3bvEXT\0"
   "\0"
   /* _mesa_function_pool[28199]: UniformMatrix3x4fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix3x4fv\0"
   "\0"
   /* _mesa_function_pool[28226]: ClipPlane (offset 150) */
   "ip\0"
   "glClipPlane\0"
   "\0"
   /* _mesa_function_pool[28242]: Recti (offset 90) */
   "iiii\0"
   "glRecti\0"
   "\0"
   /* _mesa_function_pool[28256]: DrawRangeElementsBaseVertex (will be remapped) */
   "iiiiipi\0"
   "glDrawRangeElementsBaseVertex\0"
   "\0"
   /* _mesa_function_pool[28295]: TexCoordPointervINTEL (dynamic) */
   "iip\0"
   "glTexCoordPointervINTEL\0"
   "\0"
   /* _mesa_function_pool[28324]: DeleteBuffersARB (will be remapped) */
   "ip\0"
   "glDeleteBuffers\0"
   "glDeleteBuffersARB\0"
   "\0"
   /* _mesa_function_pool[28363]: PixelTransformParameterfvEXT (dynamic) */
   "iip\0"
   "glPixelTransformParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[28399]: PrimitiveRestartNV (will be remapped) */
   "\0"
   "glPrimitiveRestartNV\0"
   "\0"
   /* _mesa_function_pool[28422]: WindowPos4fvMESA (will be remapped) */
   "p\0"
   "glWindowPos4fvMESA\0"
   "\0"
   /* _mesa_function_pool[28444]: GetPixelMapuiv (offset 272) */
   "ip\0"
   "glGetPixelMapuiv\0"
   "\0"
   /* _mesa_function_pool[28465]: Rectf (offset 88) */
   "ffff\0"
   "glRectf\0"
   "\0"
   /* _mesa_function_pool[28479]: VertexAttrib1sNV (will be remapped) */
   "ii\0"
   "glVertexAttrib1sNV\0"
   "\0"
   /* _mesa_function_pool[28502]: Indexfv (offset 47) */
   "p\0"
   "glIndexfv\0"
   "\0"
   /* _mesa_function_pool[28515]: SecondaryColor3svEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3sv\0"
   "glSecondaryColor3svEXT\0"
   "\0"
   /* _mesa_function_pool[28561]: LoadTransposeMatrixfARB (will be remapped) */
   "p\0"
   "glLoadTransposeMatrixf\0"
   "glLoadTransposeMatrixfARB\0"
   "\0"
   /* _mesa_function_pool[28613]: GetPointerv (offset 329) */
   "ip\0"
   "glGetPointerv\0"
   "glGetPointervEXT\0"
   "\0"
   /* _mesa_function_pool[28648]: Tangent3bEXT (dynamic) */
   "iii\0"
   "glTangent3bEXT\0"
   "\0"
   /* _mesa_function_pool[28668]: CombinerParameterfNV (will be remapped) */
   "if\0"
   "glCombinerParameterfNV\0"
   "\0"
   /* _mesa_function_pool[28695]: IndexMask (offset 212) */
   "i\0"
   "glIndexMask\0"
   "\0"
   /* _mesa_function_pool[28710]: BindProgramNV (will be remapped) */
   "ii\0"
   "glBindProgramARB\0"
   "glBindProgramNV\0"
   "\0"
   /* _mesa_function_pool[28747]: VertexAttrib4svARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4sv\0"
   "glVertexAttrib4svARB\0"
   "\0"
   /* _mesa_function_pool[28790]: GetFloatv (offset 262) */
   "ip\0"
   "glGetFloatv\0"
   "\0"
   /* _mesa_function_pool[28806]: CreateDebugObjectMESA (dynamic) */
   "\0"
   "glCreateDebugObjectMESA\0"
   "\0"
   /* _mesa_function_pool[28832]: GetShaderiv (will be remapped) */
   "iip\0"
   "glGetShaderiv\0"
   "\0"
   /* _mesa_function_pool[28851]: ClientWaitSync (will be remapped) */
   "iii\0"
   "glClientWaitSync\0"
   "\0"
   /* _mesa_function_pool[28873]: TexCoord4s (offset 124) */
   "iiii\0"
   "glTexCoord4s\0"
   "\0"
   /* _mesa_function_pool[28892]: TexCoord3sv (offset 117) */
   "p\0"
   "glTexCoord3sv\0"
   "\0"
   /* _mesa_function_pool[28909]: BindFragmentShaderATI (will be remapped) */
   "i\0"
   "glBindFragmentShaderATI\0"
   "\0"
   /* _mesa_function_pool[28936]: PopAttrib (offset 218) */
   "\0"
   "glPopAttrib\0"
   "\0"
   /* _mesa_function_pool[28950]: Fogfv (offset 154) */
   "ip\0"
   "glFogfv\0"
   "\0"
   /* _mesa_function_pool[28962]: UnmapBufferARB (will be remapped) */
   "i\0"
   "glUnmapBuffer\0"
   "glUnmapBufferARB\0"
   "\0"
   /* _mesa_function_pool[28996]: InitNames (offset 197) */
   "\0"
   "glInitNames\0"
   "\0"
   /* _mesa_function_pool[29010]: Normal3sv (offset 61) */
   "p\0"
   "glNormal3sv\0"
   "\0"
   /* _mesa_function_pool[29025]: Minmax (offset 368) */
   "iii\0"
   "glMinmax\0"
   "glMinmaxEXT\0"
   "\0"
   /* _mesa_function_pool[29051]: TexCoord4d (offset 118) */
   "dddd\0"
   "glTexCoord4d\0"
   "\0"
   /* _mesa_function_pool[29070]: DeformationMap3dSGIX (dynamic) */
   "iddiiddiiddiip\0"
   "glDeformationMap3dSGIX\0"
   "\0"
   /* _mesa_function_pool[29109]: TexCoord4f (offset 120) */
   "ffff\0"
   "glTexCoord4f\0"
   "\0"
   /* _mesa_function_pool[29128]: FogCoorddvEXT (will be remapped) */
   "p\0"
   "glFogCoorddv\0"
   "glFogCoorddvEXT\0"
   "\0"
   /* _mesa_function_pool[29160]: FinishTextureSUNX (dynamic) */
   "\0"
   "glFinishTextureSUNX\0"
   "\0"
   /* _mesa_function_pool[29182]: GetFragmentLightfvSGIX (dynamic) */
   "iip\0"
   "glGetFragmentLightfvSGIX\0"
   "\0"
   /* _mesa_function_pool[29212]: Binormal3fvEXT (dynamic) */
   "p\0"
   "glBinormal3fvEXT\0"
   "\0"
   /* _mesa_function_pool[29232]: GetBooleanv (offset 258) */
   "ip\0"
   "glGetBooleanv\0"
   "\0"
   /* _mesa_function_pool[29250]: ColorFragmentOp3ATI (will be remapped) */
   "iiiiiiiiiiiii\0"
   "glColorFragmentOp3ATI\0"
   "\0"
   /* _mesa_function_pool[29287]: Hint (offset 158) */
   "ii\0"
   "glHint\0"
   "\0"
   /* _mesa_function_pool[29298]: Color4dv (offset 28) */
   "p\0"
   "glColor4dv\0"
   "\0"
   /* _mesa_function_pool[29312]: VertexAttrib2svARB (will be remapped) */
   "ip\0"
   "glVertexAttrib2sv\0"
   "glVertexAttrib2svARB\0"
   "\0"
   /* _mesa_function_pool[29355]: AreProgramsResidentNV (will be remapped) */
   "ipp\0"
   "glAreProgramsResidentNV\0"
   "\0"
   /* _mesa_function_pool[29384]: WindowPos3svMESA (will be remapped) */
   "p\0"
   "glWindowPos3sv\0"
   "glWindowPos3svARB\0"
   "glWindowPos3svMESA\0"
   "\0"
   /* _mesa_function_pool[29439]: CopyColorSubTable (offset 347) */
   "iiiii\0"
   "glCopyColorSubTable\0"
   "glCopyColorSubTableEXT\0"
   "\0"
   /* _mesa_function_pool[29489]: WeightdvARB (dynamic) */
   "ip\0"
   "glWeightdvARB\0"
   "\0"
   /* _mesa_function_pool[29507]: DeleteRenderbuffersEXT (will be remapped) */
   "ip\0"
   "glDeleteRenderbuffers\0"
   "glDeleteRenderbuffersEXT\0"
   "\0"
   /* _mesa_function_pool[29558]: VertexAttrib4NubvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Nubv\0"
   "glVertexAttrib4NubvARB\0"
   "\0"
   /* _mesa_function_pool[29605]: VertexAttrib3dvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib3dvNV\0"
   "\0"
   /* _mesa_function_pool[29629]: GetObjectParameterfvARB (will be remapped) */
   "iip\0"
   "glGetObjectParameterfvARB\0"
   "\0"
   /* _mesa_function_pool[29660]: Vertex4iv (offset 147) */
   "p\0"
   "glVertex4iv\0"
   "\0"
   /* _mesa_function_pool[29675]: GetProgramEnvParameterdvARB (will be remapped) */
   "iip\0"
   "glGetProgramEnvParameterdvARB\0"
   "\0"
   /* _mesa_function_pool[29710]: TexCoord4dv (offset 119) */
   "p\0"
   "glTexCoord4dv\0"
   "\0"
   /* _mesa_function_pool[29727]: LockArraysEXT (will be remapped) */
   "ii\0"
   "glLockArraysEXT\0"
   "\0"
   /* _mesa_function_pool[29747]: Begin (offset 7) */
   "i\0"
   "glBegin\0"
   "\0"
   /* _mesa_function_pool[29758]: LightModeli (offset 165) */
   "ii\0"
   "glLightModeli\0"
   "\0"
   /* _mesa_function_pool[29776]: Rectfv (offset 89) */
   "pp\0"
   "glRectfv\0"
   "\0"
   /* _mesa_function_pool[29789]: LightModelf (offset 163) */
   "if\0"
   "glLightModelf\0"
   "\0"
   /* _mesa_function_pool[29807]: GetTexParameterfv (offset 282) */
   "iip\0"
   "glGetTexParameterfv\0"
   "\0"
   /* _mesa_function_pool[29832]: GetLightfv (offset 264) */
   "iip\0"
   "glGetLightfv\0"
   "\0"
   /* _mesa_function_pool[29850]: PixelTransformParameterivEXT (dynamic) */
   "iip\0"
   "glPixelTransformParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[29886]: BinormalPointerEXT (dynamic) */
   "iip\0"
   "glBinormalPointerEXT\0"
   "\0"
   /* _mesa_function_pool[29912]: VertexAttrib1dNV (will be remapped) */
   "id\0"
   "glVertexAttrib1dNV\0"
   "\0"
   /* _mesa_function_pool[29935]: GetCombinerInputParameterivNV (will be remapped) */
   "iiiip\0"
   "glGetCombinerInputParameterivNV\0"
   "\0"
   /* _mesa_function_pool[29974]: Disable (offset 214) */
   "i\0"
   "glDisable\0"
   "\0"
   /* _mesa_function_pool[29987]: MultiTexCoord2fvARB (offset 387) */
   "ip\0"
   "glMultiTexCoord2fv\0"
   "glMultiTexCoord2fvARB\0"
   "\0"
   /* _mesa_function_pool[30032]: GetRenderbufferParameterivEXT (will be remapped) */
   "iip\0"
   "glGetRenderbufferParameteriv\0"
   "glGetRenderbufferParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[30098]: CombinerParameterivNV (will be remapped) */
   "ip\0"
   "glCombinerParameterivNV\0"
   "\0"
   /* _mesa_function_pool[30126]: GenFragmentShadersATI (will be remapped) */
   "i\0"
   "glGenFragmentShadersATI\0"
   "\0"
   /* _mesa_function_pool[30153]: DrawArrays (offset 310) */
   "iii\0"
   "glDrawArrays\0"
   "glDrawArraysEXT\0"
   "\0"
   /* _mesa_function_pool[30187]: WeightuivARB (dynamic) */
   "ip\0"
   "glWeightuivARB\0"
   "\0"
   /* _mesa_function_pool[30206]: VertexAttrib2sARB (will be remapped) */
   "iii\0"
   "glVertexAttrib2s\0"
   "glVertexAttrib2sARB\0"
   "\0"
   /* _mesa_function_pool[30248]: ColorMask (offset 210) */
   "iiii\0"
   "glColorMask\0"
   "\0"
   /* _mesa_function_pool[30266]: GenAsyncMarkersSGIX (dynamic) */
   "i\0"
   "glGenAsyncMarkersSGIX\0"
   "\0"
   /* _mesa_function_pool[30291]: Tangent3svEXT (dynamic) */
   "p\0"
   "glTangent3svEXT\0"
   "\0"
   /* _mesa_function_pool[30310]: GetListParameterivSGIX (dynamic) */
   "iip\0"
   "glGetListParameterivSGIX\0"
   "\0"
   /* _mesa_function_pool[30340]: BindBufferARB (will be remapped) */
   "ii\0"
   "glBindBuffer\0"
   "glBindBufferARB\0"
   "\0"
   /* _mesa_function_pool[30373]: GetInfoLogARB (will be remapped) */
   "iipp\0"
   "glGetInfoLogARB\0"
   "\0"
   /* _mesa_function_pool[30395]: RasterPos4iv (offset 83) */
   "p\0"
   "glRasterPos4iv\0"
   "\0"
   /* _mesa_function_pool[30413]: Enable (offset 215) */
   "i\0"
   "glEnable\0"
   "\0"
   /* _mesa_function_pool[30425]: LineStipple (offset 167) */
   "ii\0"
   "glLineStipple\0"
   "\0"
   /* _mesa_function_pool[30443]: VertexAttribs4svNV (will be remapped) */
   "iip\0"
   "glVertexAttribs4svNV\0"
   "\0"
   /* _mesa_function_pool[30469]: EdgeFlagPointerListIBM (dynamic) */
   "ipi\0"
   "glEdgeFlagPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[30499]: UniformMatrix3x2fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix3x2fv\0"
   "\0"
   /* _mesa_function_pool[30526]: GetMinmaxParameterfv (offset 365) */
   "iip\0"
   "glGetMinmaxParameterfv\0"
   "glGetMinmaxParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[30580]: VertexAttrib1fvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib1fv\0"
   "glVertexAttrib1fvARB\0"
   "\0"
   /* _mesa_function_pool[30623]: GenBuffersARB (will be remapped) */
   "ip\0"
   "glGenBuffers\0"
   "glGenBuffersARB\0"
   "\0"
   /* _mesa_function_pool[30656]: VertexAttribs1svNV (will be remapped) */
   "iip\0"
   "glVertexAttribs1svNV\0"
   "\0"
   /* _mesa_function_pool[30682]: Vertex3fv (offset 137) */
   "p\0"
   "glVertex3fv\0"
   "\0"
   /* _mesa_function_pool[30697]: GetTexBumpParameterivATI (will be remapped) */
   "ip\0"
   "glGetTexBumpParameterivATI\0"
   "\0"
   /* _mesa_function_pool[30728]: Binormal3bEXT (dynamic) */
   "iii\0"
   "glBinormal3bEXT\0"
   "\0"
   /* _mesa_function_pool[30749]: FragmentMaterialivSGIX (dynamic) */
   "iip\0"
   "glFragmentMaterialivSGIX\0"
   "\0"
   /* _mesa_function_pool[30779]: IsRenderbufferEXT (will be remapped) */
   "i\0"
   "glIsRenderbuffer\0"
   "glIsRenderbufferEXT\0"
   "\0"
   /* _mesa_function_pool[30819]: GenProgramsNV (will be remapped) */
   "ip\0"
   "glGenProgramsARB\0"
   "glGenProgramsNV\0"
   "\0"
   /* _mesa_function_pool[30856]: VertexAttrib4dvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib4dvNV\0"
   "\0"
   /* _mesa_function_pool[30880]: EndFragmentShaderATI (will be remapped) */
   "\0"
   "glEndFragmentShaderATI\0"
   "\0"
   /* _mesa_function_pool[30905]: Binormal3iEXT (dynamic) */
   "iii\0"
   "glBinormal3iEXT\0"
   "\0"
   /* _mesa_function_pool[30926]: WindowPos2fMESA (will be remapped) */
   "ff\0"
   "glWindowPos2f\0"
   "glWindowPos2fARB\0"
   "glWindowPos2fMESA\0"
   "\0"
   ;

/* these functions need to be remapped */
static const struct gl_function_pool_remap MESA_remap_table_functions[] = {
   {  1461, AttachShader_remap_index },
   {  9105, CreateProgram_remap_index },
   { 21316, CreateShader_remap_index },
   { 23629, DeleteProgram_remap_index },
   { 17076, DeleteShader_remap_index },
   { 21762, DetachShader_remap_index },
   { 16600, GetAttachedShaders_remap_index },
   {  4326, GetProgramInfoLog_remap_index },
   {   361, GetProgramiv_remap_index },
   {  5802, GetShaderInfoLog_remap_index },
   { 28832, GetShaderiv_remap_index },
   { 12308, IsProgram_remap_index },
   { 11307, IsShader_remap_index },
   {  9209, StencilFuncSeparate_remap_index },
   {  3538, StencilMaskSeparate_remap_index },
   {  6884, StencilOpSeparate_remap_index },
   { 20617, UniformMatrix2x3fv_remap_index },
   {  2615, UniformMatrix2x4fv_remap_index },
   { 30499, UniformMatrix3x2fv_remap_index },
   { 28199, UniformMatrix3x4fv_remap_index },
   { 15071, UniformMatrix4x2fv_remap_index },
   {  2963, UniformMatrix4x3fv_remap_index },
   { 14732, DrawArraysInstanced_remap_index },
   { 15864, DrawElementsInstanced_remap_index },
   {  9123, LoadTransposeMatrixdARB_remap_index },
   { 28561, LoadTransposeMatrixfARB_remap_index },
   {  4955, MultTransposeMatrixdARB_remap_index },
   { 21949, MultTransposeMatrixfARB_remap_index },
   {   172, SampleCoverageARB_remap_index },
   {  5168, CompressedTexImage1DARB_remap_index },
   { 22432, CompressedTexImage2DARB_remap_index },
   {  3601, CompressedTexImage3DARB_remap_index },
   { 16892, CompressedTexSubImage1DARB_remap_index },
   {  1880, CompressedTexSubImage2DARB_remap_index },
   { 18753, CompressedTexSubImage3DARB_remap_index },
   { 26481, GetCompressedTexImageARB_remap_index },
   {  3446, DisableVertexAttribArrayARB_remap_index },
   { 27811, EnableVertexAttribArrayARB_remap_index },
   { 29675, GetProgramEnvParameterdvARB_remap_index },
   { 21829, GetProgramEnvParameterfvARB_remap_index },
   { 25480, GetProgramLocalParameterdvARB_remap_index },
   {  7326, GetProgramLocalParameterfvARB_remap_index },
   { 16983, GetProgramStringARB_remap_index },
   { 25675, GetProgramivARB_remap_index },
   { 18948, GetVertexAttribdvARB_remap_index },
   { 14960, GetVertexAttribfvARB_remap_index },
   {  8989, GetVertexAttribivARB_remap_index },
   { 17829, ProgramEnvParameter4dARB_remap_index },
   { 23402, ProgramEnvParameter4dvARB_remap_index },
   { 15568, ProgramEnvParameter4fARB_remap_index },
   {  8189, ProgramEnvParameter4fvARB_remap_index },
   {  3564, ProgramLocalParameter4dARB_remap_index },
   { 12018, ProgramLocalParameter4dvARB_remap_index },
   { 27290, ProgramLocalParameter4fARB_remap_index },
   { 23982, ProgramLocalParameter4fvARB_remap_index },
   { 26261, ProgramStringARB_remap_index },
   { 18079, VertexAttrib1dARB_remap_index },
   { 14536, VertexAttrib1dvARB_remap_index },
   {  3739, VertexAttrib1fARB_remap_index },
   { 30580, VertexAttrib1fvARB_remap_index },
   {  6410, VertexAttrib1sARB_remap_index },
   {  2054, VertexAttrib1svARB_remap_index },
   { 13967, VertexAttrib2dARB_remap_index },
   { 16219, VertexAttrib2dvARB_remap_index },
   {  1480, VertexAttrib2fARB_remap_index },
   { 16332, VertexAttrib2fvARB_remap_index },
   { 30206, VertexAttrib2sARB_remap_index },
   { 29312, VertexAttrib2svARB_remap_index },
   { 10392, VertexAttrib3dARB_remap_index },
   {  7892, VertexAttrib3dvARB_remap_index },
   {  1567, VertexAttrib3fARB_remap_index },
   { 20880, VertexAttrib3fvARB_remap_index },
   { 26108, VertexAttrib3sARB_remap_index },
   { 18690, VertexAttrib3svARB_remap_index },
   {  4352, VertexAttrib4NbvARB_remap_index },
   { 16555, VertexAttrib4NivARB_remap_index },
   { 20835, VertexAttrib4NsvARB_remap_index },
   { 21781, VertexAttrib4NubARB_remap_index },
   { 29558, VertexAttrib4NubvARB_remap_index },
   { 17490, VertexAttrib4NuivARB_remap_index },
   {  2836, VertexAttrib4NusvARB_remap_index },
   {  9986, VertexAttrib4bvARB_remap_index },
   { 24888, VertexAttrib4dARB_remap_index },
   { 19712, VertexAttrib4dvARB_remap_index },
   { 10499, VertexAttrib4fARB_remap_index },
   { 10903, VertexAttrib4fvARB_remap_index },
   {  9402, VertexAttrib4ivARB_remap_index },
   { 16033, VertexAttrib4sARB_remap_index },
   { 28747, VertexAttrib4svARB_remap_index },
   { 15373, VertexAttrib4ubvARB_remap_index },
   { 28135, VertexAttrib4uivARB_remap_index },
   { 18501, VertexAttrib4usvARB_remap_index },
   { 20466, VertexAttribPointerARB_remap_index },
   { 30340, BindBufferARB_remap_index },
   {  6117, BufferDataARB_remap_index },
   {  1382, BufferSubDataARB_remap_index },
   { 28324, DeleteBuffersARB_remap_index },
   { 30623, GenBuffersARB_remap_index },
   { 16375, GetBufferParameterivARB_remap_index },
   { 15520, GetBufferPointervARB_remap_index },
   {  1335, GetBufferSubDataARB_remap_index },
   { 28083, IsBufferARB_remap_index },
   { 24466, MapBufferARB_remap_index },
   { 28962, UnmapBufferARB_remap_index },
   {   268, BeginQueryARB_remap_index },
   { 18174, DeleteQueriesARB_remap_index },
   { 11197, EndQueryARB_remap_index },
   { 26960, GenQueriesARB_remap_index },
   {  1772, GetQueryObjectivARB_remap_index },
   { 16077, GetQueryObjectuivARB_remap_index },
   {  1624, GetQueryivARB_remap_index },
   { 18408, IsQueryARB_remap_index },
   {  7502, AttachObjectARB_remap_index },
   { 17038, CompileShaderARB_remap_index },
   {  2905, CreateProgramObjectARB_remap_index },
   {  6062, CreateShaderObjectARB_remap_index },
   { 13384, DeleteObjectARB_remap_index },
   { 22223, DetachObjectARB_remap_index },
   { 10975, GetActiveUniformARB_remap_index },
   {  8664, GetAttachedObjectsARB_remap_index },
   {  8971, GetHandleARB_remap_index },
   { 30373, GetInfoLogARB_remap_index },
   { 29629, GetObjectParameterfvARB_remap_index },
   { 25354, GetObjectParameterivARB_remap_index },
   { 26718, GetShaderSourceARB_remap_index },
   { 25968, GetUniformLocationARB_remap_index },
   { 22051, GetUniformfvARB_remap_index },
   { 11640, GetUniformivARB_remap_index },
   { 18546, LinkProgramARB_remap_index },
   { 18604, ShaderSourceARB_remap_index },
   {  6784, Uniform1fARB_remap_index },
   { 27499, Uniform1fvARB_remap_index },
   { 20435, Uniform1iARB_remap_index },
   { 19401, Uniform1ivARB_remap_index },
   {  2003, Uniform2fARB_remap_index },
   { 13220, Uniform2fvARB_remap_index },
   { 24353, Uniform2iARB_remap_index },
   {  2123, Uniform2ivARB_remap_index },
   { 17148, Uniform3fARB_remap_index },
   {  8694, Uniform3fvARB_remap_index },
   {  5708, Uniform3iARB_remap_index },
   { 15626, Uniform3ivARB_remap_index },
   { 17635, Uniform4fARB_remap_index },
   { 21915, Uniform4fvARB_remap_index },
   { 23081, Uniform4iARB_remap_index },
   { 18914, Uniform4ivARB_remap_index },
   {  7554, UniformMatrix2fvARB_remap_index },
   {    17, UniformMatrix3fvARB_remap_index },
   {  2475, UniformMatrix4fvARB_remap_index },
   { 23514, UseProgramObjectARB_remap_index },
   { 13655, ValidateProgramARB_remap_index },
   { 19755, BindAttribLocationARB_remap_index },
   {  4397, GetActiveAttribARB_remap_index },
   { 15307, GetAttribLocationARB_remap_index },
   { 27238, DrawBuffersARB_remap_index },
   { 12123, RenderbufferStorageMultisample_remap_index },
   { 12527, FramebufferTextureARB_remap_index },
   { 23884, FramebufferTextureFaceARB_remap_index },
   { 22372, ProgramParameteriARB_remap_index },
   { 17683, FlushMappedBufferRange_remap_index },
   { 25771, MapBufferRange_remap_index },
   { 15182, BindVertexArray_remap_index },
   { 13514, GenVertexArrays_remap_index },
   { 28013, CopyBufferSubData_remap_index },
   { 28851, ClientWaitSync_remap_index },
   {  2394, DeleteSync_remap_index },
   {  6451, FenceSync_remap_index },
   { 14026, GetInteger64v_remap_index },
   { 20942, GetSynciv_remap_index },
   { 27177, IsSync_remap_index },
   {  8612, WaitSync_remap_index },
   {  3414, DrawElementsBaseVertex_remap_index },
   { 28256, DrawRangeElementsBaseVertex_remap_index },
   { 24497, MultiDrawElementsBaseVertex_remap_index },
   {  4531, BindTransformFeedback_remap_index },
   {  2932, DeleteTransformFeedbacks_remap_index },
   {  5741, DrawTransformFeedback_remap_index },
   {  8831, GenTransformFeedbacks_remap_index },
   { 26151, IsTransformFeedback_remap_index },
   { 24077, PauseTransformFeedback_remap_index },
   {  4875, ResumeTransformFeedback_remap_index },
   {  4790, PolygonOffsetEXT_remap_index },
   { 21551, GetPixelTexGenParameterfvSGIS_remap_index },
   {  3946, GetPixelTexGenParameterivSGIS_remap_index },
   { 21284, PixelTexGenParameterfSGIS_remap_index },
   {   580, PixelTexGenParameterfvSGIS_remap_index },
   { 11678, PixelTexGenParameteriSGIS_remap_index },
   { 12701, PixelTexGenParameterivSGIS_remap_index },
   { 15270, SampleMaskSGIS_remap_index },
   { 18348, SamplePatternSGIS_remap_index },
   { 24426, ColorPointerEXT_remap_index },
   { 16262, EdgeFlagPointerEXT_remap_index },
   {  5362, IndexPointerEXT_remap_index },
   {  5442, NormalPointerEXT_remap_index },
   { 14620, TexCoordPointerEXT_remap_index },
   {  6240, VertexPointerEXT_remap_index },
   {  3216, PointParameterfEXT_remap_index },
   {  7091, PointParameterfvEXT_remap_index },
   { 29727, LockArraysEXT_remap_index },
   { 13719, UnlockArraysEXT_remap_index },
   {  1151, SecondaryColor3bEXT_remap_index },
   {  7250, SecondaryColor3bvEXT_remap_index },
   {  9579, SecondaryColor3dEXT_remap_index },
   { 23687, SecondaryColor3dvEXT_remap_index },
   { 26017, SecondaryColor3fEXT_remap_index },
   { 16828, SecondaryColor3fvEXT_remap_index },
   {   426, SecondaryColor3iEXT_remap_index },
   { 15008, SecondaryColor3ivEXT_remap_index },
   {  9237, SecondaryColor3sEXT_remap_index },
   { 28515, SecondaryColor3svEXT_remap_index },
   { 25190, SecondaryColor3ubEXT_remap_index },
   { 19646, SecondaryColor3ubvEXT_remap_index },
   { 11873, SecondaryColor3uiEXT_remap_index },
   { 21171, SecondaryColor3uivEXT_remap_index },
   { 23934, SecondaryColor3usEXT_remap_index },
   { 11946, SecondaryColor3usvEXT_remap_index },
   { 10846, SecondaryColorPointerEXT_remap_index },
   { 23748, MultiDrawArraysEXT_remap_index },
   { 19336, MultiDrawElementsEXT_remap_index },
   { 19531, FogCoordPointerEXT_remap_index },
   {  4095, FogCoorddEXT_remap_index },
   { 29128, FogCoorddvEXT_remap_index },
   {  4187, FogCoordfEXT_remap_index },
   { 25113, FogCoordfvEXT_remap_index },
   { 17587, PixelTexGenSGIX_remap_index },
   { 25698, BlendFuncSeparateEXT_remap_index },
   {  6152, FlushVertexArrayRangeNV_remap_index },
   {  4739, VertexArrayRangeNV_remap_index },
   { 26082, CombinerInputNV_remap_index },
   {  1946, CombinerOutputNV_remap_index },
   { 28668, CombinerParameterfNV_remap_index },
   {  4659, CombinerParameterfvNV_remap_index },
   { 20666, CombinerParameteriNV_remap_index },
   { 30098, CombinerParameterivNV_remap_index },
   {  6528, FinalCombinerInputNV_remap_index },
   {  9037, GetCombinerInputParameterfvNV_remap_index },
   { 29935, GetCombinerInputParameterivNV_remap_index },
   { 12802, GetCombinerOutputParameterfvNV_remap_index },
   { 12630, GetCombinerOutputParameterivNV_remap_index },
   {  5897, GetFinalCombinerInputParameterfvNV_remap_index },
   { 22953, GetFinalCombinerInputParameterivNV_remap_index },
   { 11618, ResizeBuffersMESA_remap_index },
   { 10219, WindowPos2dMESA_remap_index },
   {   944, WindowPos2dvMESA_remap_index },
   { 30926, WindowPos2fMESA_remap_index },
   {  7195, WindowPos2fvMESA_remap_index },
   { 16775, WindowPos2iMESA_remap_index },
   { 18821, WindowPos2ivMESA_remap_index },
   { 19435, WindowPos2sMESA_remap_index },
   {  5082, WindowPos2svMESA_remap_index },
   {  7020, WindowPos3dMESA_remap_index },
   { 12948, WindowPos3dvMESA_remap_index },
   {   472, WindowPos3fMESA_remap_index },
   { 13780, WindowPos3fvMESA_remap_index },
   { 22265, WindowPos3iMESA_remap_index },
   { 27958, WindowPos3ivMESA_remap_index },
   { 17293, WindowPos3sMESA_remap_index },
   { 29384, WindowPos3svMESA_remap_index },
   { 10170, WindowPos4dMESA_remap_index },
   { 15740, WindowPos4dvMESA_remap_index },
   { 12907, WindowPos4fMESA_remap_index },
   { 28422, WindowPos4fvMESA_remap_index },
   { 28111, WindowPos4iMESA_remap_index },
   { 11421, WindowPos4ivMESA_remap_index },
   { 17466, WindowPos4sMESA_remap_index },
   {  2883, WindowPos4svMESA_remap_index },
   { 12669, MultiModeDrawArraysIBM_remap_index },
   { 26831, MultiModeDrawElementsIBM_remap_index },
   { 11225, DeleteFencesNV_remap_index },
   { 25929, FinishFenceNV_remap_index },
   {  3338, GenFencesNV_remap_index },
   { 15720, GetFenceivNV_remap_index },
   {  7487, IsFenceNV_remap_index },
   { 12557, SetFenceNV_remap_index },
   {  3795, TestFenceNV_remap_index },
   { 29355, AreProgramsResidentNV_remap_index },
   { 28710, BindProgramNV_remap_index },
   { 24017, DeleteProgramsNV_remap_index },
   { 19864, ExecuteProgramNV_remap_index },
   { 30819, GenProgramsNV_remap_index },
   { 21630, GetProgramParameterdvNV_remap_index },
   {  9641, GetProgramParameterfvNV_remap_index },
   { 24400, GetProgramStringNV_remap_index },
   { 22642, GetProgramivNV_remap_index },
   { 21864, GetTrackMatrixivNV_remap_index },
   { 24194, GetVertexAttribPointervNV_remap_index },
   { 22886, GetVertexAttribdvNV_remap_index },
   {  8507, GetVertexAttribfvNV_remap_index },
   { 16956, GetVertexAttribivNV_remap_index },
   { 17713, IsProgramNV_remap_index },
   {  8590, LoadProgramNV_remap_index },
   { 25794, ProgramParameters4dvNV_remap_index },
   { 22572, ProgramParameters4fvNV_remap_index },
   { 19125, RequestResidentProgramsNV_remap_index },
   { 20644, TrackMatrixNV_remap_index },
   { 29912, VertexAttrib1dNV_remap_index },
   { 12468, VertexAttrib1dvNV_remap_index },
   { 26363, VertexAttrib1fNV_remap_index },
   {  2245, VertexAttrib1fvNV_remap_index },
   { 28479, VertexAttrib1sNV_remap_index },
   { 13853, VertexAttrib1svNV_remap_index },
   {  4302, VertexAttrib2dNV_remap_index },
   { 12383, VertexAttrib2dvNV_remap_index },
   { 18580, VertexAttrib2fNV_remap_index },
   { 11994, VertexAttrib2fvNV_remap_index },
   {  5272, VertexAttrib2sNV_remap_index },
   { 17347, VertexAttrib2svNV_remap_index },
   { 10367, VertexAttrib3dNV_remap_index },
   { 29605, VertexAttrib3dvNV_remap_index },
   {  9453, VertexAttrib3fNV_remap_index },
   { 22913, VertexAttrib3fvNV_remap_index },
   { 20521, VertexAttrib3sNV_remap_index },
   { 21891, VertexAttrib3svNV_remap_index },
   { 26805, VertexAttrib4dNV_remap_index },
   { 30856, VertexAttrib4dvNV_remap_index },
   {  3996, VertexAttrib4fNV_remap_index },
   {  8640, VertexAttrib4fvNV_remap_index },
   { 24772, VertexAttrib4sNV_remap_index },
   {  1293, VertexAttrib4svNV_remap_index },
   {  4460, VertexAttrib4ubNV_remap_index },
   {   734, VertexAttrib4ubvNV_remap_index },
   { 20044, VertexAttribPointerNV_remap_index },
   {  2097, VertexAttribs1dvNV_remap_index },
   { 24282, VertexAttribs1fvNV_remap_index },
   { 30656, VertexAttribs1svNV_remap_index },
   {  9478, VertexAttribs2dvNV_remap_index },
   { 23475, VertexAttribs2fvNV_remap_index },
   { 16288, VertexAttribs2svNV_remap_index },
   {  4687, VertexAttribs3dvNV_remap_index },
   {  1977, VertexAttribs3fvNV_remap_index },
   { 27706, VertexAttribs3svNV_remap_index },
   { 24862, VertexAttribs4dvNV_remap_index },
   {  4713, VertexAttribs4fvNV_remap_index },
   { 30443, VertexAttribs4svNV_remap_index },
   { 27454, VertexAttribs4ubvNV_remap_index },
   { 24932, GetTexBumpParameterfvATI_remap_index },
   { 30697, GetTexBumpParameterivATI_remap_index },
   { 17010, TexBumpParameterfvATI_remap_index },
   { 18996, TexBumpParameterivATI_remap_index },
   { 14399, AlphaFragmentOp1ATI_remap_index },
   { 10029, AlphaFragmentOp2ATI_remap_index },
   { 22829, AlphaFragmentOp3ATI_remap_index },
   { 27633, BeginFragmentShaderATI_remap_index },
   { 28909, BindFragmentShaderATI_remap_index },
   { 22020, ColorFragmentOp1ATI_remap_index },
   {  3874, ColorFragmentOp2ATI_remap_index },
   { 29250, ColorFragmentOp3ATI_remap_index },
   {  4832, DeleteFragmentShaderATI_remap_index },
   { 30880, EndFragmentShaderATI_remap_index },
   { 30126, GenFragmentShadersATI_remap_index },
   { 23606, PassTexCoordATI_remap_index },
   {  6220, SampleMapATI_remap_index },
   {  5993, SetFragmentShaderConstantATI_remap_index },
   {   319, PointParameteriNV_remap_index },
   { 13109, PointParameterivNV_remap_index },
   { 26644, ActiveStencilFaceEXT_remap_index },
   { 25454, BindVertexArrayAPPLE_remap_index },
   {  2522, DeleteVertexArraysAPPLE_remap_index },
   { 16627, GenVertexArraysAPPLE_remap_index },
   { 21695, IsVertexArrayAPPLE_remap_index },
   {   775, GetProgramNamedParameterdvNV_remap_index },
   {  3179, GetProgramNamedParameterfvNV_remap_index },
   { 24963, ProgramNamedParameter4dNV_remap_index },
   { 13435, ProgramNamedParameter4dvNV_remap_index },
   {  8123, ProgramNamedParameter4fNV_remap_index },
   { 10811, ProgramNamedParameter4fvNV_remap_index },
   { 15675, PrimitiveRestartIndexNV_remap_index },
   { 28399, PrimitiveRestartNV_remap_index },
   { 22551, DepthBoundsEXT_remap_index },
   {  1043, BlendEquationSeparateEXT_remap_index },
   { 13554, BindFramebufferEXT_remap_index },
   { 23793, BindRenderbufferEXT_remap_index },
   {  8887, CheckFramebufferStatusEXT_remap_index },
   { 20985, DeleteFramebuffersEXT_remap_index },
   { 29507, DeleteRenderbuffersEXT_remap_index },
   { 12407, FramebufferRenderbufferEXT_remap_index },
   { 12574, FramebufferTexture1DEXT_remap_index },
   { 10605, FramebufferTexture2DEXT_remap_index },
   { 10272, FramebufferTexture3DEXT_remap_index },
   { 21587, GenFramebuffersEXT_remap_index },
   { 16174, GenRenderbuffersEXT_remap_index },
   {  5939, GenerateMipmapEXT_remap_index },
   { 20141, GetFramebufferAttachmentParameterivEXT_remap_index },
   { 30032, GetRenderbufferParameterivEXT_remap_index },
   { 18876, IsFramebufferEXT_remap_index },
   { 30779, IsRenderbufferEXT_remap_index },
   {  7434, RenderbufferStorageEXT_remap_index },
   {   651, BlitFramebufferEXT_remap_index },
   { 13254, BufferParameteriAPPLE_remap_index },
   { 17745, FlushMappedBufferRangeAPPLE_remap_index },
   {  2727, FramebufferTextureLayerEXT_remap_index },
   {  5007, ColorMaskIndexedEXT_remap_index },
   { 17371, DisableIndexedEXT_remap_index },
   { 24619, EnableIndexedEXT_remap_index },
   { 20112, GetBooleanIndexedvEXT_remap_index },
   { 10062, GetIntegerIndexedvEXT_remap_index },
   { 21061, IsEnabledIndexedEXT_remap_index },
   { 20961, ClearColorIiEXT_remap_index },
   {  3089, ClearColorIuiEXT_remap_index },
   {  9076, GetTexParameterIivEXT_remap_index },
   {  5242, GetTexParameterIuivEXT_remap_index },
   {  2701, TexParameterIivEXT_remap_index },
   { 24535, TexParameterIuivEXT_remap_index },
   {  4125, BeginConditionalRenderNV_remap_index },
   { 23579, EndConditionalRenderNV_remap_index },
   {  8534, BeginTransformFeedbackEXT_remap_index },
   { 17395, BindBufferBaseEXT_remap_index },
   { 17265, BindBufferOffsetEXT_remap_index },
   { 11246, BindBufferRangeEXT_remap_index },
   { 13169, EndTransformFeedbackEXT_remap_index },
   {  9914, GetTransformFeedbackVaryingEXT_remap_index },
   { 19181, TransformFeedbackVaryingsEXT_remap_index },
   { 27355, ProvokingVertexEXT_remap_index },
   {  9862, GetTexParameterPointervAPPLE_remap_index },
   {  4487, TextureRangeAPPLE_remap_index },
   { 10677, GetObjectParameterivAPPLE_remap_index },
   { 18320, ObjectPurgeableAPPLE_remap_index },
   {  5036, ObjectUnpurgeableAPPLE_remap_index },
   { 26670, StencilFuncSeparateATI_remap_index },
   { 16694, ProgramEnvParameters4fvEXT_remap_index },
   { 20075, ProgramLocalParameters4fvEXT_remap_index },
   { 13037, GetQueryObjecti64vEXT_remap_index },
   {  9504, GetQueryObjectui64vEXT_remap_index },
   { 22089, EGLImageTargetRenderbufferStorageOES_remap_index },
   { 11164, EGLImageTargetTexture2DOES_remap_index },
   {    -1, -1 }
};

/* these functions are in the ABI, but have alternative names */
static const struct gl_function_remap MESA_alt_functions[] = {
   /* from GL_EXT_blend_color */
   {  2440, _gloffset_BlendColor },
   /* from GL_EXT_blend_minmax */
   { 10329, _gloffset_BlendEquation },
   /* from GL_EXT_color_subtable */
   { 15762, _gloffset_ColorSubTable },
   { 29439, _gloffset_CopyColorSubTable },
   /* from GL_EXT_convolution */
   {   213, _gloffset_ConvolutionFilter1D },
   {  2284, _gloffset_CopyConvolutionFilter1D },
   {  3675, _gloffset_GetConvolutionParameteriv },
   {  7783, _gloffset_ConvolutionFilter2D },
   {  7949, _gloffset_ConvolutionParameteriv },
   {  8409, _gloffset_ConvolutionParameterfv },
   { 19024, _gloffset_GetSeparableFilter },
   { 22319, _gloffset_SeparableFilter2D },
   { 23131, _gloffset_ConvolutionParameteri },
   { 23254, _gloffset_ConvolutionParameterf },
   { 24798, _gloffset_GetConvolutionParameterfv },
   { 25620, _gloffset_GetConvolutionFilter },
   { 27895, _gloffset_CopyConvolutionFilter2D },
   /* from GL_EXT_copy_texture */
   { 13913, _gloffset_CopyTexSubImage3D },
   { 15473, _gloffset_CopyTexImage2D },
   { 22739, _gloffset_CopyTexImage1D },
   { 25301, _gloffset_CopyTexSubImage2D },
   { 27533, _gloffset_CopyTexSubImage1D },
   /* from GL_EXT_draw_range_elements */
   {  8746, _gloffset_DrawRangeElements },
   /* from GL_EXT_histogram */
   {   812, _gloffset_Histogram },
   {  3139, _gloffset_ResetHistogram },
   {  9175, _gloffset_GetMinmax },
   { 14247, _gloffset_GetHistogramParameterfv },
   { 22664, _gloffset_GetMinmaxParameteriv },
   { 24688, _gloffset_ResetMinmax },
   { 25517, _gloffset_GetHistogramParameteriv },
   { 26604, _gloffset_GetHistogram },
   { 29025, _gloffset_Minmax },
   { 30526, _gloffset_GetMinmaxParameterfv },
   /* from GL_EXT_paletted_texture */
   {  7645, _gloffset_ColorTable },
   { 14093, _gloffset_GetColorTable },
   { 21334, _gloffset_GetColorTableParameterfv },
   { 23310, _gloffset_GetColorTableParameteriv },
   /* from GL_EXT_subtexture */
   {  6366, _gloffset_TexSubImage1D },
   {  9789, _gloffset_TexSubImage2D },
   /* from GL_EXT_texture3D */
   {  1658, _gloffset_TexImage3D },
   { 21103, _gloffset_TexSubImage3D },
   /* from GL_EXT_texture_object */
   {  2990, _gloffset_PrioritizeTextures },
   {  6815, _gloffset_AreTexturesResident },
   { 12492, _gloffset_GenTextures },
   { 14579, _gloffset_DeleteTextures },
   { 18026, _gloffset_IsTexture },
   { 27598, _gloffset_BindTexture },
   /* from GL_EXT_vertex_array */
   { 22491, _gloffset_ArrayElement },
   { 28613, _gloffset_GetPointerv },
   { 30153, _gloffset_DrawArrays },
   /* from GL_SGI_color_table */
   {  6933, _gloffset_ColorTableParameteriv },
   {  7645, _gloffset_ColorTable },
   { 14093, _gloffset_GetColorTable },
   { 14203, _gloffset_CopyColorTable },
   { 17887, _gloffset_ColorTableParameterfv },
   { 21334, _gloffset_GetColorTableParameterfv },
   { 23310, _gloffset_GetColorTableParameteriv },
   /* from GL_VERSION_1_3 */
   {   381, _gloffset_MultiTexCoord3sARB },
   {   613, _gloffset_ActiveTextureARB },
   {  3812, _gloffset_MultiTexCoord1fvARB },
   {  5467, _gloffset_MultiTexCoord3dARB },
   {  5512, _gloffset_MultiTexCoord2iARB },
   {  5636, _gloffset_MultiTexCoord2svARB },
   {  7601, _gloffset_MultiTexCoord2fARB },
   {  9534, _gloffset_MultiTexCoord3fvARB },
   { 10091, _gloffset_MultiTexCoord4sARB },
   { 10725, _gloffset_MultiTexCoord2dvARB },
   { 11107, _gloffset_MultiTexCoord1svARB },
   { 11479, _gloffset_MultiTexCoord3svARB },
   { 11540, _gloffset_MultiTexCoord4iARB },
   { 12263, _gloffset_MultiTexCoord3iARB },
   { 13066, _gloffset_MultiTexCoord1dARB },
   { 13283, _gloffset_MultiTexCoord3dvARB },
   { 14447, _gloffset_MultiTexCoord3ivARB },
   { 14492, _gloffset_MultiTexCoord2sARB },
   { 15819, _gloffset_MultiTexCoord4ivARB },
   { 17537, _gloffset_ClientActiveTextureARB },
   { 19820, _gloffset_MultiTexCoord2dARB },
   { 20261, _gloffset_MultiTexCoord4dvARB },
   { 20572, _gloffset_MultiTexCoord4fvARB },
   { 21475, _gloffset_MultiTexCoord3fARB },
   { 23838, _gloffset_MultiTexCoord4dARB },
   { 24104, _gloffset_MultiTexCoord1sARB },
   { 24308, _gloffset_MultiTexCoord1dvARB },
   { 25145, _gloffset_MultiTexCoord1ivARB },
   { 25238, _gloffset_MultiTexCoord2ivARB },
   { 25577, _gloffset_MultiTexCoord1iARB },
   { 26879, _gloffset_MultiTexCoord4svARB },
   { 27397, _gloffset_MultiTexCoord1fARB },
   { 27660, _gloffset_MultiTexCoord4fARB },
   { 29987, _gloffset_MultiTexCoord2fvARB },
   {    -1, -1 }
};

#endif /* need_MESA_remap_table */

#if defined(need_GL_3DFX_tbuffer)
static const struct gl_function_remap GL_3DFX_tbuffer_functions[] = {
   {  8467, -1 }, /* TbufferMask3DFX */
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
   { 11443, -1 }, /* FramebufferTextureLayer */
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
   {  3390, -1 }, /* MatrixIndexusvARB */
   { 12084, -1 }, /* MatrixIndexuivARB */
   { 13405, -1 }, /* MatrixIndexPointerARB */
   { 18275, -1 }, /* CurrentPaletteMatrixARB */
   { 21219, -1 }, /* MatrixIndexubvARB */
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
   {  2226, -1 }, /* WeightubvARB */
   {  5827, -1 }, /* WeightivARB */
   { 10194, -1 }, /* WeightPointerARB */
   { 12784, -1 }, /* WeightfvARB */
   { 16314, -1 }, /* WeightbvARB */
   { 19488, -1 }, /* WeightusvARB */
   { 22245, -1 }, /* VertexBlendARB */
   { 27481, -1 }, /* WeightsvARB */
   { 29489, -1 }, /* WeightdvARB */
   { 30187, -1 }, /* WeightuivARB */
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
   {  2440, _gloffset_BlendColor },
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
   { 10329, _gloffset_BlendEquation },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_color_subtable)
static const struct gl_function_remap GL_EXT_color_subtable_functions[] = {
   { 15762, _gloffset_ColorSubTable },
   { 29439, _gloffset_CopyColorSubTable },
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
   {   213, _gloffset_ConvolutionFilter1D },
   {  2284, _gloffset_CopyConvolutionFilter1D },
   {  3675, _gloffset_GetConvolutionParameteriv },
   {  7783, _gloffset_ConvolutionFilter2D },
   {  7949, _gloffset_ConvolutionParameteriv },
   {  8409, _gloffset_ConvolutionParameterfv },
   { 19024, _gloffset_GetSeparableFilter },
   { 22319, _gloffset_SeparableFilter2D },
   { 23131, _gloffset_ConvolutionParameteri },
   { 23254, _gloffset_ConvolutionParameterf },
   { 24798, _gloffset_GetConvolutionParameterfv },
   { 25620, _gloffset_GetConvolutionFilter },
   { 27895, _gloffset_CopyConvolutionFilter2D },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const struct gl_function_remap GL_EXT_coordinate_frame_functions[] = {
   {  9673, -1 }, /* TangentPointerEXT */
   { 11598, -1 }, /* Binormal3ivEXT */
   { 12216, -1 }, /* Tangent3sEXT */
   { 13470, -1 }, /* Tangent3fvEXT */
   { 17246, -1 }, /* Tangent3dvEXT */
   { 17973, -1 }, /* Binormal3bvEXT */
   { 19077, -1 }, /* Binormal3dEXT */
   { 21151, -1 }, /* Tangent3fEXT */
   { 23203, -1 }, /* Binormal3sEXT */
   { 23648, -1 }, /* Tangent3ivEXT */
   { 23667, -1 }, /* Tangent3dEXT */
   { 24562, -1 }, /* Binormal3svEXT */
   { 25043, -1 }, /* Binormal3fEXT */
   { 25895, -1 }, /* Binormal3dvEXT */
   { 27101, -1 }, /* Tangent3iEXT */
   { 28180, -1 }, /* Tangent3bvEXT */
   { 28648, -1 }, /* Tangent3bEXT */
   { 29212, -1 }, /* Binormal3fvEXT */
   { 29886, -1 }, /* BinormalPointerEXT */
   { 30291, -1 }, /* Tangent3svEXT */
   { 30728, -1 }, /* Binormal3bEXT */
   { 30905, -1 }, /* Binormal3iEXT */
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_copy_texture)
static const struct gl_function_remap GL_EXT_copy_texture_functions[] = {
   { 13913, _gloffset_CopyTexSubImage3D },
   { 15473, _gloffset_CopyTexImage2D },
   { 22739, _gloffset_CopyTexImage1D },
   { 25301, _gloffset_CopyTexSubImage2D },
   { 27533, _gloffset_CopyTexSubImage1D },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_cull_vertex)
static const struct gl_function_remap GL_EXT_cull_vertex_functions[] = {
   {  8098, -1 }, /* CullParameterdvEXT */
   { 10770, -1 }, /* CullParameterfvEXT */
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
   {  8746, _gloffset_DrawRangeElements },
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

#if defined(need_GL_EXT_histogram)
static const struct gl_function_remap GL_EXT_histogram_functions[] = {
   {   812, _gloffset_Histogram },
   {  3139, _gloffset_ResetHistogram },
   {  9175, _gloffset_GetMinmax },
   { 14247, _gloffset_GetHistogramParameterfv },
   { 22664, _gloffset_GetMinmaxParameteriv },
   { 24688, _gloffset_ResetMinmax },
   { 25517, _gloffset_GetHistogramParameteriv },
   { 26604, _gloffset_GetHistogram },
   { 29025, _gloffset_Minmax },
   { 30526, _gloffset_GetMinmaxParameterfv },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_index_func)
static const struct gl_function_remap GL_EXT_index_func_functions[] = {
   { 10556, -1 }, /* IndexFuncEXT */
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_index_material)
static const struct gl_function_remap GL_EXT_index_material_functions[] = {
   { 19575, -1 }, /* IndexMaterialEXT */
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_light_texture)
static const struct gl_function_remap GL_EXT_light_texture_functions[] = {
   { 24582, -1 }, /* ApplyTextureEXT */
   { 24642, -1 }, /* TextureMaterialEXT */
   { 24667, -1 }, /* TextureLightEXT */
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
   {  7645, _gloffset_ColorTable },
   { 14093, _gloffset_GetColorTable },
   { 21334, _gloffset_GetColorTableParameterfv },
   { 23310, _gloffset_GetColorTableParameteriv },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_pixel_transform)
static const struct gl_function_remap GL_EXT_pixel_transform_functions[] = {
   { 20226, -1 }, /* PixelTransformParameterfEXT */
   { 20306, -1 }, /* PixelTransformParameteriEXT */
   { 28363, -1 }, /* PixelTransformParameterfvEXT */
   { 29850, -1 }, /* PixelTransformParameterivEXT */
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

#if defined(need_GL_EXT_stencil_two_side)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_stencil_two_side_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_subtexture)
static const struct gl_function_remap GL_EXT_subtexture_functions[] = {
   {  6366, _gloffset_TexSubImage1D },
   {  9789, _gloffset_TexSubImage2D },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_texture3D)
static const struct gl_function_remap GL_EXT_texture3D_functions[] = {
   {  1658, _gloffset_TexImage3D },
   { 21103, _gloffset_TexSubImage3D },
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
   {  2990, _gloffset_PrioritizeTextures },
   {  6815, _gloffset_AreTexturesResident },
   { 12492, _gloffset_GenTextures },
   { 14579, _gloffset_DeleteTextures },
   { 18026, _gloffset_IsTexture },
   { 27598, _gloffset_BindTexture },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_texture_perturb_normal)
static const struct gl_function_remap GL_EXT_texture_perturb_normal_functions[] = {
   { 12734, -1 }, /* TextureNormalEXT */
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
   { 22491, _gloffset_ArrayElement },
   { 28613, _gloffset_GetPointerv },
   { 30153, _gloffset_DrawArrays },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_vertex_weighting)
static const struct gl_function_remap GL_EXT_vertex_weighting_functions[] = {
   { 18056, -1 }, /* VertexWeightfvEXT */
   { 25021, -1 }, /* VertexWeightfEXT */
   { 26573, -1 }, /* VertexWeightPointerEXT */
   {    -1, -1 }
};
#endif

#if defined(need_GL_HP_image_transform)
static const struct gl_function_remap GL_HP_image_transform_functions[] = {
   {  2157, -1 }, /* GetImageTransformParameterfvHP */
   {  3356, -1 }, /* ImageTransformParameterfHP */
   {  9367, -1 }, /* ImageTransformParameterfvHP */
   { 11025, -1 }, /* ImageTransformParameteriHP */
   { 11333, -1 }, /* GetImageTransformParameterivHP */
   { 18120, -1 }, /* ImageTransformParameterivHP */
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
   {  3908, -1 }, /* SecondaryColorPointerListIBM */
   {  5333, -1 }, /* NormalPointerListIBM */
   {  6989, -1 }, /* FogCoordPointerListIBM */
   {  7296, -1 }, /* VertexPointerListIBM */
   { 10946, -1 }, /* ColorPointerListIBM */
   { 12323, -1 }, /* TexCoordPointerListIBM */
   { 12756, -1 }, /* IndexPointerListIBM */
   { 30469, -1 }, /* EdgeFlagPointerListIBM */
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
   { 11710, -1 }, /* VertexPointervINTEL */
   { 14340, -1 }, /* ColorPointervINTEL */
   { 27869, -1 }, /* NormalPointervINTEL */
   { 28295, -1 }, /* TexCoordPointervINTEL */
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
   {  1522, -1 }, /* GetDebugLogLengthMESA */
   {  3114, -1 }, /* ClearDebugLogMESA */
   {  4069, -1 }, /* GetDebugLogMESA */
   { 28806, -1 }, /* CreateDebugObjectMESA */
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
   {  6028, -1 }, /* GetMapAttribParameterivNV */
   {  7751, -1 }, /* MapControlPointsNV */
   {  7850, -1 }, /* MapParameterfvNV */
   {  9772, -1 }, /* EvalMapsNV */
   { 15984, -1 }, /* GetMapAttribParameterfvNV */
   { 16150, -1 }, /* MapParameterivNV */
   { 23054, -1 }, /* GetMapParameterivNV */
   { 23552, -1 }, /* GetMapParameterfvNV */
   { 27205, -1 }, /* GetMapControlPointsNV */
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
   { 14810, -1 }, /* CombinerStageParameterfvNV */
   { 15125, -1 }, /* GetCombinerStageParameterfvNV */
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
   {  7935, -1 }, /* HintPGI */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_detail_texture)
static const struct gl_function_remap GL_SGIS_detail_texture_functions[] = {
   { 15098, -1 }, /* GetDetailTexFuncSGIS */
   { 15418, -1 }, /* DetailTexFuncSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_fog_function)
static const struct gl_function_remap GL_SGIS_fog_function_functions[] = {
   { 25283, -1 }, /* FogFuncSGIS */
   { 25948, -1 }, /* GetFogFuncSGIS */
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
   {  6089, -1 }, /* GetSharpenTexFuncSGIS */
   { 20546, -1 }, /* SharpenTexFuncSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_texture4D)
static const struct gl_function_remap GL_SGIS_texture4D_functions[] = {
   {   894, -1 }, /* TexImage4DSGIS */
   { 14648, -1 }, /* TexSubImage4DSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_texture_color_mask)
static const struct gl_function_remap GL_SGIS_texture_color_mask_functions[] = {
   { 14046, -1 }, /* TextureColorMaskSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_texture_filter4)
static const struct gl_function_remap GL_SGIS_texture_filter4_functions[] = {
   {  6266, -1 }, /* GetTexFilterFuncSGIS */
   { 15244, -1 }, /* TexFilterFuncSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_async)
static const struct gl_function_remap GL_SGIX_async_functions[] = {
   {  3040, -1 }, /* AsyncMarkerSGIX */
   {  4048, -1 }, /* FinishAsyncSGIX */
   {  4813, -1 }, /* PollAsyncSGIX */
   { 20693, -1 }, /* DeleteAsyncMarkersSGIX */
   { 20748, -1 }, /* IsAsyncMarkerSGIX */
   { 30266, -1 }, /* GenAsyncMarkersSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_flush_raster)
static const struct gl_function_remap GL_SGIX_flush_raster_functions[] = {
   {  6643, -1 }, /* FlushRasterSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_fragment_lighting)
static const struct gl_function_remap GL_SGIX_fragment_lighting_functions[] = {
   {  2410, -1 }, /* FragmentMaterialfvSGIX */
   {  4764, -1 }, /* FragmentLightiSGIX */
   {  5769, -1 }, /* GetFragmentMaterialfvSGIX */
   {  7363, -1 }, /* FragmentMaterialfSGIX */
   {  7524, -1 }, /* GetFragmentLightivSGIX */
   {  8361, -1 }, /* FragmentLightModeliSGIX */
   {  9835, -1 }, /* FragmentLightivSGIX */
   { 10137, -1 }, /* GetFragmentMaterialivSGIX */
   { 17943, -1 }, /* FragmentLightModelfSGIX */
   { 18243, -1 }, /* FragmentColorMaterialSGIX */
   { 18643, -1 }, /* FragmentMaterialiSGIX */
   { 19903, -1 }, /* LightEnviSGIX */
   { 21426, -1 }, /* FragmentLightModelfvSGIX */
   { 21735, -1 }, /* FragmentLightfvSGIX */
   { 26332, -1 }, /* FragmentLightModelivSGIX */
   { 26455, -1 }, /* FragmentLightfSGIX */
   { 29182, -1 }, /* GetFragmentLightfvSGIX */
   { 30749, -1 }, /* FragmentMaterialivSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_framezoom)
static const struct gl_function_remap GL_SGIX_framezoom_functions[] = {
   { 20771, -1 }, /* FrameZoomSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_igloo_interface)
static const struct gl_function_remap GL_SGIX_igloo_interface_functions[] = {
   { 26763, -1 }, /* IglooInterfaceSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_instruments)
static const struct gl_function_remap GL_SGIX_instruments_functions[] = {
   {  2573, -1 }, /* ReadInstrumentsSGIX */
   {  5845, -1 }, /* PollInstrumentsSGIX */
   {  9733, -1 }, /* GetInstrumentsSGIX */
   { 11921, -1 }, /* StartInstrumentsSGIX */
   { 14844, -1 }, /* StopInstrumentsSGIX */
   { 16527, -1 }, /* InstrumentsBufferSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_list_priority)
static const struct gl_function_remap GL_SGIX_list_priority_functions[] = {
   {  1125, -1 }, /* ListParameterfSGIX */
   {  2789, -1 }, /* GetListParameterfvSGIX */
   { 16442, -1 }, /* ListParameteriSGIX */
   { 17196, -1 }, /* ListParameterfvSGIX */
   { 19309, -1 }, /* ListParameterivSGIX */
   { 30310, -1 }, /* GetListParameterivSGIX */
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
   {  3302, -1 }, /* LoadIdentityDeformationMapSGIX */
   { 14944, -1 }, /* DeformSGIX */
   { 22603, -1 }, /* DeformationMap3fSGIX */
   { 29070, -1 }, /* DeformationMap3dSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_reference_plane)
static const struct gl_function_remap GL_SGIX_reference_plane_functions[] = {
   { 13597, -1 }, /* ReferencePlaneSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_sprite)
static const struct gl_function_remap GL_SGIX_sprite_functions[] = {
   {  8859, -1 }, /* SpriteParameterfvSGIX */
   { 19098, -1 }, /* SpriteParameteriSGIX */
   { 24722, -1 }, /* SpriteParameterfSGIX */
   { 27327, -1 }, /* SpriteParameterivSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_tag_sample_buffer)
static const struct gl_function_remap GL_SGIX_tag_sample_buffer_functions[] = {
   { 19157, -1 }, /* TagSampleBufferSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGI_color_table)
static const struct gl_function_remap GL_SGI_color_table_functions[] = {
   {  6933, _gloffset_ColorTableParameteriv },
   {  7645, _gloffset_ColorTable },
   { 14093, _gloffset_GetColorTable },
   { 14203, _gloffset_CopyColorTable },
   { 17887, _gloffset_ColorTableParameterfv },
   { 21334, _gloffset_GetColorTableParameterfv },
   { 23310, _gloffset_GetColorTableParameteriv },
   {    -1, -1 }
};
#endif

#if defined(need_GL_SUNX_constant_data)
static const struct gl_function_remap GL_SUNX_constant_data_functions[] = {
   { 29160, -1 }, /* FinishTextureSUNX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SUN_global_alpha)
static const struct gl_function_remap GL_SUN_global_alpha_functions[] = {
   {  3061, -1 }, /* GlobalAlphaFactorubSUN */
   {  4275, -1 }, /* GlobalAlphaFactoriSUN */
   {  5870, -1 }, /* GlobalAlphaFactordSUN */
   {  8943, -1 }, /* GlobalAlphaFactoruiSUN */
   {  9324, -1 }, /* GlobalAlphaFactorbSUN */
   { 12236, -1 }, /* GlobalAlphaFactorfSUN */
   { 12355, -1 }, /* GlobalAlphaFactorusSUN */
   { 21034, -1 }, /* GlobalAlphaFactorsSUN */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SUN_mesh_array)
static const struct gl_function_remap GL_SUN_mesh_array_functions[] = {
   { 27139, -1 }, /* DrawMeshArraysSUN */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SUN_triangle_list)
static const struct gl_function_remap GL_SUN_triangle_list_functions[] = {
   {  4022, -1 }, /* ReplacementCodeubSUN */
   {  5681, -1 }, /* ReplacementCodeubvSUN */
   { 17608, -1 }, /* ReplacementCodeusvSUN */
   { 17796, -1 }, /* ReplacementCodePointerSUN */
   { 19967, -1 }, /* ReplacementCodeuiSUN */
   { 20722, -1 }, /* ReplacementCodeusSUN */
   { 27784, -1 }, /* ReplacementCodeuivSUN */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SUN_vertex)
static const struct gl_function_remap GL_SUN_vertex_functions[] = {
   {   999, -1 }, /* ReplacementCodeuiColor3fVertex3fvSUN */
   {  1197, -1 }, /* TexCoord4fColor4fNormal3fVertex4fvSUN */
   {  1423, -1 }, /* TexCoord2fColor4ubVertex3fvSUN */
   {  1699, -1 }, /* ReplacementCodeuiVertex3fvSUN */
   {  1833, -1 }, /* ReplacementCodeuiTexCoord2fVertex3fvSUN */
   {  2346, -1 }, /* ReplacementCodeuiNormal3fVertex3fSUN */
   {  2642, -1 }, /* Color4ubVertex3fvSUN */
   {  4156, -1 }, /* Color4ubVertex3fSUN */
   {  4232, -1 }, /* TexCoord2fVertex3fSUN */
   {  4559, -1 }, /* TexCoord2fColor4fNormal3fVertex3fSUN */
   {  4917, -1 }, /* TexCoord2fNormal3fVertex3fvSUN */
   {  5576, -1 }, /* ReplacementCodeuiTexCoord2fNormal3fVertex3fSUN */
   {  6321, -1 }, /* ReplacementCodeuiColor4ubVertex3fvSUN */
   {  6680, -1 }, /* ReplacementCodeuiTexCoord2fVertex3fSUN */
   {  7392, -1 }, /* TexCoord2fNormal3fVertex3fSUN */
   {  8160, -1 }, /* Color3fVertex3fSUN */
   {  9283, -1 }, /* Color3fVertex3fvSUN */
   {  9698, -1 }, /* Color4fNormal3fVertex3fvSUN */
   { 10435, -1 }, /* ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN */
   { 11784, -1 }, /* ReplacementCodeuiColor4fNormal3fVertex3fvSUN */
   { 13328, -1 }, /* ReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN */
   { 13739, -1 }, /* TexCoord2fColor3fVertex3fSUN */
   { 14869, -1 }, /* TexCoord4fColor4fNormal3fVertex4fSUN */
   { 15203, -1 }, /* Color4ubVertex2fvSUN */
   { 15443, -1 }, /* Normal3fVertex3fSUN */
   { 16468, -1 }, /* ReplacementCodeuiColor4fNormal3fVertex3fSUN */
   { 16729, -1 }, /* TexCoord2fColor4fNormal3fVertex3fvSUN */
   { 17437, -1 }, /* TexCoord2fVertex3fvSUN */
   { 18213, -1 }, /* Color4ubVertex2fSUN */
   { 18434, -1 }, /* ReplacementCodeuiColor4ubVertex3fSUN */
   { 20392, -1 }, /* TexCoord2fColor4ubVertex3fSUN */
   { 20790, -1 }, /* Normal3fVertex3fvSUN */
   { 21243, -1 }, /* Color4fNormal3fVertex3fSUN */
   { 22152, -1 }, /* ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN */
   { 24147, -1 }, /* ReplacementCodeuiColor3fVertex3fSUN */
   { 25399, -1 }, /* TexCoord4fVertex4fSUN */
   { 25825, -1 }, /* TexCoord2fColor3fVertex3fvSUN */
   { 26176, -1 }, /* ReplacementCodeuiNormal3fVertex3fvSUN */
   { 26303, -1 }, /* TexCoord4fVertex4fvSUN */
   { 27011, -1 }, /* ReplacementCodeuiVertex3fSUN */
   {    -1, -1 }
};
#endif

#if defined(need_GL_VERSION_1_3)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_VERSION_1_3_functions[] = {
   {   381, _gloffset_MultiTexCoord3sARB },
   {   613, _gloffset_ActiveTextureARB },
   {  3812, _gloffset_MultiTexCoord1fvARB },
   {  5467, _gloffset_MultiTexCoord3dARB },
   {  5512, _gloffset_MultiTexCoord2iARB },
   {  5636, _gloffset_MultiTexCoord2svARB },
   {  7601, _gloffset_MultiTexCoord2fARB },
   {  9534, _gloffset_MultiTexCoord3fvARB },
   { 10091, _gloffset_MultiTexCoord4sARB },
   { 10725, _gloffset_MultiTexCoord2dvARB },
   { 11107, _gloffset_MultiTexCoord1svARB },
   { 11479, _gloffset_MultiTexCoord3svARB },
   { 11540, _gloffset_MultiTexCoord4iARB },
   { 12263, _gloffset_MultiTexCoord3iARB },
   { 13066, _gloffset_MultiTexCoord1dARB },
   { 13283, _gloffset_MultiTexCoord3dvARB },
   { 14447, _gloffset_MultiTexCoord3ivARB },
   { 14492, _gloffset_MultiTexCoord2sARB },
   { 15819, _gloffset_MultiTexCoord4ivARB },
   { 17537, _gloffset_ClientActiveTextureARB },
   { 19820, _gloffset_MultiTexCoord2dARB },
   { 20261, _gloffset_MultiTexCoord4dvARB },
   { 20572, _gloffset_MultiTexCoord4fvARB },
   { 21475, _gloffset_MultiTexCoord3fARB },
   { 23838, _gloffset_MultiTexCoord4dARB },
   { 24104, _gloffset_MultiTexCoord1sARB },
   { 24308, _gloffset_MultiTexCoord1dvARB },
   { 25145, _gloffset_MultiTexCoord1ivARB },
   { 25238, _gloffset_MultiTexCoord2ivARB },
   { 25577, _gloffset_MultiTexCoord1iARB },
   { 26879, _gloffset_MultiTexCoord4svARB },
   { 27397, _gloffset_MultiTexCoord1fARB },
   { 27660, _gloffset_MultiTexCoord4fARB },
   { 29987, _gloffset_MultiTexCoord2fvARB },
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

