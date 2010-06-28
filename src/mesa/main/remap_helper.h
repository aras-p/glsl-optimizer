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
   /* _mesa_function_pool[2701]: FramebufferTextureLayerEXT (will be remapped) */
   "iiiii\0"
   "glFramebufferTextureLayer\0"
   "glFramebufferTextureLayerEXT\0"
   "\0"
   /* _mesa_function_pool[2763]: GetListParameterfvSGIX (dynamic) */
   "iip\0"
   "glGetListParameterfvSGIX\0"
   "\0"
   /* _mesa_function_pool[2793]: Viewport (offset 305) */
   "iiii\0"
   "glViewport\0"
   "\0"
   /* _mesa_function_pool[2810]: VertexAttrib4NusvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Nusv\0"
   "glVertexAttrib4NusvARB\0"
   "\0"
   /* _mesa_function_pool[2857]: WindowPos4svMESA (will be remapped) */
   "p\0"
   "glWindowPos4svMESA\0"
   "\0"
   /* _mesa_function_pool[2879]: CreateProgramObjectARB (will be remapped) */
   "\0"
   "glCreateProgramObjectARB\0"
   "\0"
   /* _mesa_function_pool[2906]: DeleteTransformFeedbacks (will be remapped) */
   "ip\0"
   "glDeleteTransformFeedbacks\0"
   "\0"
   /* _mesa_function_pool[2937]: UniformMatrix4x3fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix4x3fv\0"
   "\0"
   /* _mesa_function_pool[2964]: PrioritizeTextures (offset 331) */
   "ipp\0"
   "glPrioritizeTextures\0"
   "glPrioritizeTexturesEXT\0"
   "\0"
   /* _mesa_function_pool[3014]: AsyncMarkerSGIX (dynamic) */
   "i\0"
   "glAsyncMarkerSGIX\0"
   "\0"
   /* _mesa_function_pool[3035]: GlobalAlphaFactorubSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactorubSUN\0"
   "\0"
   /* _mesa_function_pool[3063]: ClearDebugLogMESA (dynamic) */
   "iii\0"
   "glClearDebugLogMESA\0"
   "\0"
   /* _mesa_function_pool[3088]: ResetHistogram (offset 369) */
   "i\0"
   "glResetHistogram\0"
   "glResetHistogramEXT\0"
   "\0"
   /* _mesa_function_pool[3128]: GetProgramNamedParameterfvNV (will be remapped) */
   "iipp\0"
   "glGetProgramNamedParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[3165]: PointParameterfEXT (will be remapped) */
   "if\0"
   "glPointParameterf\0"
   "glPointParameterfARB\0"
   "glPointParameterfEXT\0"
   "glPointParameterfSGIS\0"
   "\0"
   /* _mesa_function_pool[3251]: LoadIdentityDeformationMapSGIX (dynamic) */
   "i\0"
   "glLoadIdentityDeformationMapSGIX\0"
   "\0"
   /* _mesa_function_pool[3287]: GenFencesNV (will be remapped) */
   "ip\0"
   "glGenFencesNV\0"
   "\0"
   /* _mesa_function_pool[3305]: ImageTransformParameterfHP (dynamic) */
   "iif\0"
   "glImageTransformParameterfHP\0"
   "\0"
   /* _mesa_function_pool[3339]: MatrixIndexusvARB (dynamic) */
   "ip\0"
   "glMatrixIndexusvARB\0"
   "\0"
   /* _mesa_function_pool[3363]: DrawElementsBaseVertex (will be remapped) */
   "iiipi\0"
   "glDrawElementsBaseVertex\0"
   "\0"
   /* _mesa_function_pool[3395]: DisableVertexAttribArrayARB (will be remapped) */
   "i\0"
   "glDisableVertexAttribArray\0"
   "glDisableVertexAttribArrayARB\0"
   "\0"
   /* _mesa_function_pool[3455]: TexCoord2sv (offset 109) */
   "p\0"
   "glTexCoord2sv\0"
   "\0"
   /* _mesa_function_pool[3472]: Vertex4dv (offset 143) */
   "p\0"
   "glVertex4dv\0"
   "\0"
   /* _mesa_function_pool[3487]: StencilMaskSeparate (will be remapped) */
   "ii\0"
   "glStencilMaskSeparate\0"
   "\0"
   /* _mesa_function_pool[3513]: ProgramLocalParameter4dARB (will be remapped) */
   "iidddd\0"
   "glProgramLocalParameter4dARB\0"
   "\0"
   /* _mesa_function_pool[3550]: CompressedTexImage3DARB (will be remapped) */
   "iiiiiiiip\0"
   "glCompressedTexImage3D\0"
   "glCompressedTexImage3DARB\0"
   "\0"
   /* _mesa_function_pool[3610]: Color3sv (offset 18) */
   "p\0"
   "glColor3sv\0"
   "\0"
   /* _mesa_function_pool[3624]: GetConvolutionParameteriv (offset 358) */
   "iip\0"
   "glGetConvolutionParameteriv\0"
   "glGetConvolutionParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[3688]: VertexAttrib1fARB (will be remapped) */
   "if\0"
   "glVertexAttrib1f\0"
   "glVertexAttrib1fARB\0"
   "\0"
   /* _mesa_function_pool[3729]: Vertex2dv (offset 127) */
   "p\0"
   "glVertex2dv\0"
   "\0"
   /* _mesa_function_pool[3744]: TestFenceNV (will be remapped) */
   "i\0"
   "glTestFenceNV\0"
   "\0"
   /* _mesa_function_pool[3761]: MultiTexCoord1fvARB (offset 379) */
   "ip\0"
   "glMultiTexCoord1fv\0"
   "glMultiTexCoord1fvARB\0"
   "\0"
   /* _mesa_function_pool[3806]: TexCoord3iv (offset 115) */
   "p\0"
   "glTexCoord3iv\0"
   "\0"
   /* _mesa_function_pool[3823]: ColorFragmentOp2ATI (will be remapped) */
   "iiiiiiiiii\0"
   "glColorFragmentOp2ATI\0"
   "\0"
   /* _mesa_function_pool[3857]: SecondaryColorPointerListIBM (dynamic) */
   "iiipi\0"
   "glSecondaryColorPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[3895]: GetPixelTexGenParameterivSGIS (will be remapped) */
   "ip\0"
   "glGetPixelTexGenParameterivSGIS\0"
   "\0"
   /* _mesa_function_pool[3931]: Color3fv (offset 14) */
   "p\0"
   "glColor3fv\0"
   "\0"
   /* _mesa_function_pool[3945]: VertexAttrib4fNV (will be remapped) */
   "iffff\0"
   "glVertexAttrib4fNV\0"
   "\0"
   /* _mesa_function_pool[3971]: ReplacementCodeubSUN (dynamic) */
   "i\0"
   "glReplacementCodeubSUN\0"
   "\0"
   /* _mesa_function_pool[3997]: FinishAsyncSGIX (dynamic) */
   "p\0"
   "glFinishAsyncSGIX\0"
   "\0"
   /* _mesa_function_pool[4018]: GetDebugLogMESA (dynamic) */
   "iiiipp\0"
   "glGetDebugLogMESA\0"
   "\0"
   /* _mesa_function_pool[4044]: FogCoorddEXT (will be remapped) */
   "d\0"
   "glFogCoordd\0"
   "glFogCoorddEXT\0"
   "\0"
   /* _mesa_function_pool[4074]: BeginConditionalRenderNV (will be remapped) */
   "ii\0"
   "glBeginConditionalRenderNV\0"
   "\0"
   /* _mesa_function_pool[4105]: Color4ubVertex3fSUN (dynamic) */
   "iiiifff\0"
   "glColor4ubVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[4136]: FogCoordfEXT (will be remapped) */
   "f\0"
   "glFogCoordf\0"
   "glFogCoordfEXT\0"
   "\0"
   /* _mesa_function_pool[4166]: PointSize (offset 173) */
   "f\0"
   "glPointSize\0"
   "\0"
   /* _mesa_function_pool[4181]: TexCoord2fVertex3fSUN (dynamic) */
   "fffff\0"
   "glTexCoord2fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[4212]: PopName (offset 200) */
   "\0"
   "glPopName\0"
   "\0"
   /* _mesa_function_pool[4224]: GlobalAlphaFactoriSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactoriSUN\0"
   "\0"
   /* _mesa_function_pool[4251]: VertexAttrib2dNV (will be remapped) */
   "idd\0"
   "glVertexAttrib2dNV\0"
   "\0"
   /* _mesa_function_pool[4275]: GetProgramInfoLog (will be remapped) */
   "iipp\0"
   "glGetProgramInfoLog\0"
   "\0"
   /* _mesa_function_pool[4301]: VertexAttrib4NbvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Nbv\0"
   "glVertexAttrib4NbvARB\0"
   "\0"
   /* _mesa_function_pool[4346]: GetActiveAttribARB (will be remapped) */
   "iiipppp\0"
   "glGetActiveAttrib\0"
   "glGetActiveAttribARB\0"
   "\0"
   /* _mesa_function_pool[4394]: Vertex4sv (offset 149) */
   "p\0"
   "glVertex4sv\0"
   "\0"
   /* _mesa_function_pool[4409]: VertexAttrib4ubNV (will be remapped) */
   "iiiii\0"
   "glVertexAttrib4ubNV\0"
   "\0"
   /* _mesa_function_pool[4436]: TextureRangeAPPLE (will be remapped) */
   "iip\0"
   "glTextureRangeAPPLE\0"
   "\0"
   /* _mesa_function_pool[4461]: GetTexEnvfv (offset 276) */
   "iip\0"
   "glGetTexEnvfv\0"
   "\0"
   /* _mesa_function_pool[4480]: BindTransformFeedback (will be remapped) */
   "ii\0"
   "glBindTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[4508]: TexCoord2fColor4fNormal3fVertex3fSUN (dynamic) */
   "ffffffffffff\0"
   "glTexCoord2fColor4fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[4561]: Indexub (offset 315) */
   "i\0"
   "glIndexub\0"
   "\0"
   /* _mesa_function_pool[4574]: TexEnvi (offset 186) */
   "iii\0"
   "glTexEnvi\0"
   "\0"
   /* _mesa_function_pool[4589]: GetClipPlane (offset 259) */
   "ip\0"
   "glGetClipPlane\0"
   "\0"
   /* _mesa_function_pool[4608]: CombinerParameterfvNV (will be remapped) */
   "ip\0"
   "glCombinerParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[4636]: VertexAttribs3dvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs3dvNV\0"
   "\0"
   /* _mesa_function_pool[4662]: VertexAttribs4fvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs4fvNV\0"
   "\0"
   /* _mesa_function_pool[4688]: VertexArrayRangeNV (will be remapped) */
   "ip\0"
   "glVertexArrayRangeNV\0"
   "\0"
   /* _mesa_function_pool[4713]: FragmentLightiSGIX (dynamic) */
   "iii\0"
   "glFragmentLightiSGIX\0"
   "\0"
   /* _mesa_function_pool[4739]: PolygonOffsetEXT (will be remapped) */
   "ff\0"
   "glPolygonOffsetEXT\0"
   "\0"
   /* _mesa_function_pool[4762]: PollAsyncSGIX (dynamic) */
   "p\0"
   "glPollAsyncSGIX\0"
   "\0"
   /* _mesa_function_pool[4781]: DeleteFragmentShaderATI (will be remapped) */
   "i\0"
   "glDeleteFragmentShaderATI\0"
   "\0"
   /* _mesa_function_pool[4810]: Scaled (offset 301) */
   "ddd\0"
   "glScaled\0"
   "\0"
   /* _mesa_function_pool[4824]: ResumeTransformFeedback (will be remapped) */
   "\0"
   "glResumeTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[4852]: Scalef (offset 302) */
   "fff\0"
   "glScalef\0"
   "\0"
   /* _mesa_function_pool[4866]: TexCoord2fNormal3fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glTexCoord2fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[4904]: MultTransposeMatrixdARB (will be remapped) */
   "p\0"
   "glMultTransposeMatrixd\0"
   "glMultTransposeMatrixdARB\0"
   "\0"
   /* _mesa_function_pool[4956]: ColorMaskIndexedEXT (will be remapped) */
   "iiiii\0"
   "glColorMaskIndexedEXT\0"
   "\0"
   /* _mesa_function_pool[4985]: ObjectUnpurgeableAPPLE (will be remapped) */
   "iii\0"
   "glObjectUnpurgeableAPPLE\0"
   "\0"
   /* _mesa_function_pool[5015]: AlphaFunc (offset 240) */
   "if\0"
   "glAlphaFunc\0"
   "\0"
   /* _mesa_function_pool[5031]: WindowPos2svMESA (will be remapped) */
   "p\0"
   "glWindowPos2sv\0"
   "glWindowPos2svARB\0"
   "glWindowPos2svMESA\0"
   "\0"
   /* _mesa_function_pool[5086]: EdgeFlag (offset 41) */
   "i\0"
   "glEdgeFlag\0"
   "\0"
   /* _mesa_function_pool[5100]: TexCoord2iv (offset 107) */
   "p\0"
   "glTexCoord2iv\0"
   "\0"
   /* _mesa_function_pool[5117]: CompressedTexImage1DARB (will be remapped) */
   "iiiiiip\0"
   "glCompressedTexImage1D\0"
   "glCompressedTexImage1DARB\0"
   "\0"
   /* _mesa_function_pool[5175]: Rotated (offset 299) */
   "dddd\0"
   "glRotated\0"
   "\0"
   /* _mesa_function_pool[5191]: VertexAttrib2sNV (will be remapped) */
   "iii\0"
   "glVertexAttrib2sNV\0"
   "\0"
   /* _mesa_function_pool[5215]: ReadPixels (offset 256) */
   "iiiiiip\0"
   "glReadPixels\0"
   "\0"
   /* _mesa_function_pool[5237]: EdgeFlagv (offset 42) */
   "p\0"
   "glEdgeFlagv\0"
   "\0"
   /* _mesa_function_pool[5252]: NormalPointerListIBM (dynamic) */
   "iipi\0"
   "glNormalPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[5281]: IndexPointerEXT (will be remapped) */
   "iiip\0"
   "glIndexPointerEXT\0"
   "\0"
   /* _mesa_function_pool[5305]: Color4iv (offset 32) */
   "p\0"
   "glColor4iv\0"
   "\0"
   /* _mesa_function_pool[5319]: TexParameterf (offset 178) */
   "iif\0"
   "glTexParameterf\0"
   "\0"
   /* _mesa_function_pool[5340]: TexParameteri (offset 180) */
   "iii\0"
   "glTexParameteri\0"
   "\0"
   /* _mesa_function_pool[5361]: NormalPointerEXT (will be remapped) */
   "iiip\0"
   "glNormalPointerEXT\0"
   "\0"
   /* _mesa_function_pool[5386]: MultiTexCoord3dARB (offset 392) */
   "iddd\0"
   "glMultiTexCoord3d\0"
   "glMultiTexCoord3dARB\0"
   "\0"
   /* _mesa_function_pool[5431]: MultiTexCoord2iARB (offset 388) */
   "iii\0"
   "glMultiTexCoord2i\0"
   "glMultiTexCoord2iARB\0"
   "\0"
   /* _mesa_function_pool[5475]: DrawPixels (offset 257) */
   "iiiip\0"
   "glDrawPixels\0"
   "\0"
   /* _mesa_function_pool[5495]: ReplacementCodeuiTexCoord2fNormal3fVertex3fSUN (dynamic) */
   "iffffffff\0"
   "glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[5555]: MultiTexCoord2svARB (offset 391) */
   "ip\0"
   "glMultiTexCoord2sv\0"
   "glMultiTexCoord2svARB\0"
   "\0"
   /* _mesa_function_pool[5600]: ReplacementCodeubvSUN (dynamic) */
   "p\0"
   "glReplacementCodeubvSUN\0"
   "\0"
   /* _mesa_function_pool[5627]: Uniform3iARB (will be remapped) */
   "iiii\0"
   "glUniform3i\0"
   "glUniform3iARB\0"
   "\0"
   /* _mesa_function_pool[5660]: DrawTransformFeedback (will be remapped) */
   "ii\0"
   "glDrawTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[5688]: GetFragmentMaterialfvSGIX (dynamic) */
   "iip\0"
   "glGetFragmentMaterialfvSGIX\0"
   "\0"
   /* _mesa_function_pool[5721]: GetShaderInfoLog (will be remapped) */
   "iipp\0"
   "glGetShaderInfoLog\0"
   "\0"
   /* _mesa_function_pool[5746]: WeightivARB (dynamic) */
   "ip\0"
   "glWeightivARB\0"
   "\0"
   /* _mesa_function_pool[5764]: PollInstrumentsSGIX (dynamic) */
   "p\0"
   "glPollInstrumentsSGIX\0"
   "\0"
   /* _mesa_function_pool[5789]: GlobalAlphaFactordSUN (dynamic) */
   "d\0"
   "glGlobalAlphaFactordSUN\0"
   "\0"
   /* _mesa_function_pool[5816]: GetFinalCombinerInputParameterfvNV (will be remapped) */
   "iip\0"
   "glGetFinalCombinerInputParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[5858]: GenerateMipmapEXT (will be remapped) */
   "i\0"
   "glGenerateMipmap\0"
   "glGenerateMipmapEXT\0"
   "\0"
   /* _mesa_function_pool[5898]: GenLists (offset 5) */
   "i\0"
   "glGenLists\0"
   "\0"
   /* _mesa_function_pool[5912]: SetFragmentShaderConstantATI (will be remapped) */
   "ip\0"
   "glSetFragmentShaderConstantATI\0"
   "\0"
   /* _mesa_function_pool[5947]: GetMapAttribParameterivNV (dynamic) */
   "iiip\0"
   "glGetMapAttribParameterivNV\0"
   "\0"
   /* _mesa_function_pool[5981]: CreateShaderObjectARB (will be remapped) */
   "i\0"
   "glCreateShaderObjectARB\0"
   "\0"
   /* _mesa_function_pool[6008]: GetSharpenTexFuncSGIS (dynamic) */
   "ip\0"
   "glGetSharpenTexFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[6036]: BufferDataARB (will be remapped) */
   "iipi\0"
   "glBufferData\0"
   "glBufferDataARB\0"
   "\0"
   /* _mesa_function_pool[6071]: FlushVertexArrayRangeNV (will be remapped) */
   "\0"
   "glFlushVertexArrayRangeNV\0"
   "\0"
   /* _mesa_function_pool[6099]: MapGrid2d (offset 226) */
   "iddidd\0"
   "glMapGrid2d\0"
   "\0"
   /* _mesa_function_pool[6119]: MapGrid2f (offset 227) */
   "iffiff\0"
   "glMapGrid2f\0"
   "\0"
   /* _mesa_function_pool[6139]: SampleMapATI (will be remapped) */
   "iii\0"
   "glSampleMapATI\0"
   "\0"
   /* _mesa_function_pool[6159]: VertexPointerEXT (will be remapped) */
   "iiiip\0"
   "glVertexPointerEXT\0"
   "\0"
   /* _mesa_function_pool[6185]: GetTexFilterFuncSGIS (dynamic) */
   "iip\0"
   "glGetTexFilterFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[6213]: Scissor (offset 176) */
   "iiii\0"
   "glScissor\0"
   "\0"
   /* _mesa_function_pool[6229]: Fogf (offset 153) */
   "if\0"
   "glFogf\0"
   "\0"
   /* _mesa_function_pool[6240]: ReplacementCodeuiColor4ubVertex3fvSUN (dynamic) */
   "ppp\0"
   "glReplacementCodeuiColor4ubVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[6285]: TexSubImage1D (offset 332) */
   "iiiiiip\0"
   "glTexSubImage1D\0"
   "glTexSubImage1DEXT\0"
   "\0"
   /* _mesa_function_pool[6329]: VertexAttrib1sARB (will be remapped) */
   "ii\0"
   "glVertexAttrib1s\0"
   "glVertexAttrib1sARB\0"
   "\0"
   /* _mesa_function_pool[6370]: FenceSync (will be remapped) */
   "ii\0"
   "glFenceSync\0"
   "\0"
   /* _mesa_function_pool[6386]: Color4usv (offset 40) */
   "p\0"
   "glColor4usv\0"
   "\0"
   /* _mesa_function_pool[6401]: Fogi (offset 155) */
   "ii\0"
   "glFogi\0"
   "\0"
   /* _mesa_function_pool[6412]: DepthRange (offset 288) */
   "dd\0"
   "glDepthRange\0"
   "\0"
   /* _mesa_function_pool[6429]: RasterPos3iv (offset 75) */
   "p\0"
   "glRasterPos3iv\0"
   "\0"
   /* _mesa_function_pool[6447]: FinalCombinerInputNV (will be remapped) */
   "iiii\0"
   "glFinalCombinerInputNV\0"
   "\0"
   /* _mesa_function_pool[6476]: TexCoord2i (offset 106) */
   "ii\0"
   "glTexCoord2i\0"
   "\0"
   /* _mesa_function_pool[6493]: PixelMapfv (offset 251) */
   "iip\0"
   "glPixelMapfv\0"
   "\0"
   /* _mesa_function_pool[6511]: Color4ui (offset 37) */
   "iiii\0"
   "glColor4ui\0"
   "\0"
   /* _mesa_function_pool[6528]: RasterPos3s (offset 76) */
   "iii\0"
   "glRasterPos3s\0"
   "\0"
   /* _mesa_function_pool[6547]: Color3usv (offset 24) */
   "p\0"
   "glColor3usv\0"
   "\0"
   /* _mesa_function_pool[6562]: FlushRasterSGIX (dynamic) */
   "\0"
   "glFlushRasterSGIX\0"
   "\0"
   /* _mesa_function_pool[6582]: TexCoord2f (offset 104) */
   "ff\0"
   "glTexCoord2f\0"
   "\0"
   /* _mesa_function_pool[6599]: ReplacementCodeuiTexCoord2fVertex3fSUN (dynamic) */
   "ifffff\0"
   "glReplacementCodeuiTexCoord2fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[6648]: TexCoord2d (offset 102) */
   "dd\0"
   "glTexCoord2d\0"
   "\0"
   /* _mesa_function_pool[6665]: RasterPos3d (offset 70) */
   "ddd\0"
   "glRasterPos3d\0"
   "\0"
   /* _mesa_function_pool[6684]: RasterPos3f (offset 72) */
   "fff\0"
   "glRasterPos3f\0"
   "\0"
   /* _mesa_function_pool[6703]: Uniform1fARB (will be remapped) */
   "if\0"
   "glUniform1f\0"
   "glUniform1fARB\0"
   "\0"
   /* _mesa_function_pool[6734]: AreTexturesResident (offset 322) */
   "ipp\0"
   "glAreTexturesResident\0"
   "glAreTexturesResidentEXT\0"
   "\0"
   /* _mesa_function_pool[6786]: TexCoord2s (offset 108) */
   "ii\0"
   "glTexCoord2s\0"
   "\0"
   /* _mesa_function_pool[6803]: StencilOpSeparate (will be remapped) */
   "iiii\0"
   "glStencilOpSeparate\0"
   "glStencilOpSeparateATI\0"
   "\0"
   /* _mesa_function_pool[6852]: ColorTableParameteriv (offset 341) */
   "iip\0"
   "glColorTableParameteriv\0"
   "glColorTableParameterivSGI\0"
   "\0"
   /* _mesa_function_pool[6908]: FogCoordPointerListIBM (dynamic) */
   "iipi\0"
   "glFogCoordPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[6939]: WindowPos3dMESA (will be remapped) */
   "ddd\0"
   "glWindowPos3d\0"
   "glWindowPos3dARB\0"
   "glWindowPos3dMESA\0"
   "\0"
   /* _mesa_function_pool[6993]: Color4us (offset 39) */
   "iiii\0"
   "glColor4us\0"
   "\0"
   /* _mesa_function_pool[7010]: PointParameterfvEXT (will be remapped) */
   "ip\0"
   "glPointParameterfv\0"
   "glPointParameterfvARB\0"
   "glPointParameterfvEXT\0"
   "glPointParameterfvSGIS\0"
   "\0"
   /* _mesa_function_pool[7100]: Color3bv (offset 10) */
   "p\0"
   "glColor3bv\0"
   "\0"
   /* _mesa_function_pool[7114]: WindowPos2fvMESA (will be remapped) */
   "p\0"
   "glWindowPos2fv\0"
   "glWindowPos2fvARB\0"
   "glWindowPos2fvMESA\0"
   "\0"
   /* _mesa_function_pool[7169]: SecondaryColor3bvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3bv\0"
   "glSecondaryColor3bvEXT\0"
   "\0"
   /* _mesa_function_pool[7215]: VertexPointerListIBM (dynamic) */
   "iiipi\0"
   "glVertexPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[7245]: GetProgramLocalParameterfvARB (will be remapped) */
   "iip\0"
   "glGetProgramLocalParameterfvARB\0"
   "\0"
   /* _mesa_function_pool[7282]: FragmentMaterialfSGIX (dynamic) */
   "iif\0"
   "glFragmentMaterialfSGIX\0"
   "\0"
   /* _mesa_function_pool[7311]: TexCoord2fNormal3fVertex3fSUN (dynamic) */
   "ffffffff\0"
   "glTexCoord2fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[7353]: RenderbufferStorageEXT (will be remapped) */
   "iiii\0"
   "glRenderbufferStorage\0"
   "glRenderbufferStorageEXT\0"
   "\0"
   /* _mesa_function_pool[7406]: IsFenceNV (will be remapped) */
   "i\0"
   "glIsFenceNV\0"
   "\0"
   /* _mesa_function_pool[7421]: AttachObjectARB (will be remapped) */
   "ii\0"
   "glAttachObjectARB\0"
   "\0"
   /* _mesa_function_pool[7443]: GetFragmentLightivSGIX (dynamic) */
   "iip\0"
   "glGetFragmentLightivSGIX\0"
   "\0"
   /* _mesa_function_pool[7473]: UniformMatrix2fvARB (will be remapped) */
   "iiip\0"
   "glUniformMatrix2fv\0"
   "glUniformMatrix2fvARB\0"
   "\0"
   /* _mesa_function_pool[7520]: MultiTexCoord2fARB (offset 386) */
   "iff\0"
   "glMultiTexCoord2f\0"
   "glMultiTexCoord2fARB\0"
   "\0"
   /* _mesa_function_pool[7564]: ColorTable (offset 339) */
   "iiiiip\0"
   "glColorTable\0"
   "glColorTableSGI\0"
   "glColorTableEXT\0"
   "\0"
   /* _mesa_function_pool[7617]: IndexPointer (offset 314) */
   "iip\0"
   "glIndexPointer\0"
   "\0"
   /* _mesa_function_pool[7637]: Accum (offset 213) */
   "if\0"
   "glAccum\0"
   "\0"
   /* _mesa_function_pool[7649]: GetTexImage (offset 281) */
   "iiiip\0"
   "glGetTexImage\0"
   "\0"
   /* _mesa_function_pool[7670]: MapControlPointsNV (dynamic) */
   "iiiiiiiip\0"
   "glMapControlPointsNV\0"
   "\0"
   /* _mesa_function_pool[7702]: ConvolutionFilter2D (offset 349) */
   "iiiiiip\0"
   "glConvolutionFilter2D\0"
   "glConvolutionFilter2DEXT\0"
   "\0"
   /* _mesa_function_pool[7758]: Finish (offset 216) */
   "\0"
   "glFinish\0"
   "\0"
   /* _mesa_function_pool[7769]: MapParameterfvNV (dynamic) */
   "iip\0"
   "glMapParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[7793]: ClearStencil (offset 207) */
   "i\0"
   "glClearStencil\0"
   "\0"
   /* _mesa_function_pool[7811]: VertexAttrib3dvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib3dv\0"
   "glVertexAttrib3dvARB\0"
   "\0"
   /* _mesa_function_pool[7854]: HintPGI (dynamic) */
   "ii\0"
   "glHintPGI\0"
   "\0"
   /* _mesa_function_pool[7868]: ConvolutionParameteriv (offset 353) */
   "iip\0"
   "glConvolutionParameteriv\0"
   "glConvolutionParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[7926]: Color4s (offset 33) */
   "iiii\0"
   "glColor4s\0"
   "\0"
   /* _mesa_function_pool[7942]: InterleavedArrays (offset 317) */
   "iip\0"
   "glInterleavedArrays\0"
   "\0"
   /* _mesa_function_pool[7967]: RasterPos2fv (offset 65) */
   "p\0"
   "glRasterPos2fv\0"
   "\0"
   /* _mesa_function_pool[7985]: TexCoord1fv (offset 97) */
   "p\0"
   "glTexCoord1fv\0"
   "\0"
   /* _mesa_function_pool[8002]: Vertex2d (offset 126) */
   "dd\0"
   "glVertex2d\0"
   "\0"
   /* _mesa_function_pool[8017]: CullParameterdvEXT (will be remapped) */
   "ip\0"
   "glCullParameterdvEXT\0"
   "\0"
   /* _mesa_function_pool[8042]: ProgramNamedParameter4fNV (will be remapped) */
   "iipffff\0"
   "glProgramNamedParameter4fNV\0"
   "\0"
   /* _mesa_function_pool[8079]: Color3fVertex3fSUN (dynamic) */
   "ffffff\0"
   "glColor3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[8108]: ProgramEnvParameter4fvARB (will be remapped) */
   "iip\0"
   "glProgramEnvParameter4fvARB\0"
   "glProgramParameter4fvNV\0"
   "\0"
   /* _mesa_function_pool[8165]: Color4i (offset 31) */
   "iiii\0"
   "glColor4i\0"
   "\0"
   /* _mesa_function_pool[8181]: Color4f (offset 29) */
   "ffff\0"
   "glColor4f\0"
   "\0"
   /* _mesa_function_pool[8197]: RasterPos4fv (offset 81) */
   "p\0"
   "glRasterPos4fv\0"
   "\0"
   /* _mesa_function_pool[8215]: Color4d (offset 27) */
   "dddd\0"
   "glColor4d\0"
   "\0"
   /* _mesa_function_pool[8231]: ClearIndex (offset 205) */
   "f\0"
   "glClearIndex\0"
   "\0"
   /* _mesa_function_pool[8247]: Color4b (offset 25) */
   "iiii\0"
   "glColor4b\0"
   "\0"
   /* _mesa_function_pool[8263]: LoadMatrixd (offset 292) */
   "p\0"
   "glLoadMatrixd\0"
   "\0"
   /* _mesa_function_pool[8280]: FragmentLightModeliSGIX (dynamic) */
   "ii\0"
   "glFragmentLightModeliSGIX\0"
   "\0"
   /* _mesa_function_pool[8310]: RasterPos2dv (offset 63) */
   "p\0"
   "glRasterPos2dv\0"
   "\0"
   /* _mesa_function_pool[8328]: ConvolutionParameterfv (offset 351) */
   "iip\0"
   "glConvolutionParameterfv\0"
   "glConvolutionParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[8386]: TbufferMask3DFX (dynamic) */
   "i\0"
   "glTbufferMask3DFX\0"
   "\0"
   /* _mesa_function_pool[8407]: GetTexGendv (offset 278) */
   "iip\0"
   "glGetTexGendv\0"
   "\0"
   /* _mesa_function_pool[8426]: GetVertexAttribfvNV (will be remapped) */
   "iip\0"
   "glGetVertexAttribfvNV\0"
   "\0"
   /* _mesa_function_pool[8453]: BeginTransformFeedbackEXT (will be remapped) */
   "i\0"
   "glBeginTransformFeedbackEXT\0"
   "glBeginTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[8509]: LoadProgramNV (will be remapped) */
   "iiip\0"
   "glLoadProgramNV\0"
   "\0"
   /* _mesa_function_pool[8531]: WaitSync (will be remapped) */
   "iii\0"
   "glWaitSync\0"
   "\0"
   /* _mesa_function_pool[8547]: EndList (offset 1) */
   "\0"
   "glEndList\0"
   "\0"
   /* _mesa_function_pool[8559]: VertexAttrib4fvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib4fvNV\0"
   "\0"
   /* _mesa_function_pool[8583]: GetAttachedObjectsARB (will be remapped) */
   "iipp\0"
   "glGetAttachedObjectsARB\0"
   "\0"
   /* _mesa_function_pool[8613]: Uniform3fvARB (will be remapped) */
   "iip\0"
   "glUniform3fv\0"
   "glUniform3fvARB\0"
   "\0"
   /* _mesa_function_pool[8647]: EvalCoord1fv (offset 231) */
   "p\0"
   "glEvalCoord1fv\0"
   "\0"
   /* _mesa_function_pool[8665]: DrawRangeElements (offset 338) */
   "iiiiip\0"
   "glDrawRangeElements\0"
   "glDrawRangeElementsEXT\0"
   "\0"
   /* _mesa_function_pool[8716]: EvalMesh2 (offset 238) */
   "iiiii\0"
   "glEvalMesh2\0"
   "\0"
   /* _mesa_function_pool[8735]: Vertex4fv (offset 145) */
   "p\0"
   "glVertex4fv\0"
   "\0"
   /* _mesa_function_pool[8750]: GenTransformFeedbacks (will be remapped) */
   "ip\0"
   "glGenTransformFeedbacks\0"
   "\0"
   /* _mesa_function_pool[8778]: SpriteParameterfvSGIX (dynamic) */
   "ip\0"
   "glSpriteParameterfvSGIX\0"
   "\0"
   /* _mesa_function_pool[8806]: CheckFramebufferStatusEXT (will be remapped) */
   "i\0"
   "glCheckFramebufferStatus\0"
   "glCheckFramebufferStatusEXT\0"
   "\0"
   /* _mesa_function_pool[8862]: GlobalAlphaFactoruiSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactoruiSUN\0"
   "\0"
   /* _mesa_function_pool[8890]: GetHandleARB (will be remapped) */
   "i\0"
   "glGetHandleARB\0"
   "\0"
   /* _mesa_function_pool[8908]: GetVertexAttribivARB (will be remapped) */
   "iip\0"
   "glGetVertexAttribiv\0"
   "glGetVertexAttribivARB\0"
   "\0"
   /* _mesa_function_pool[8956]: GetCombinerInputParameterfvNV (will be remapped) */
   "iiiip\0"
   "glGetCombinerInputParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[8995]: CreateProgram (will be remapped) */
   "\0"
   "glCreateProgram\0"
   "\0"
   /* _mesa_function_pool[9013]: LoadTransposeMatrixdARB (will be remapped) */
   "p\0"
   "glLoadTransposeMatrixd\0"
   "glLoadTransposeMatrixdARB\0"
   "\0"
   /* _mesa_function_pool[9065]: GetMinmax (offset 364) */
   "iiiip\0"
   "glGetMinmax\0"
   "glGetMinmaxEXT\0"
   "\0"
   /* _mesa_function_pool[9099]: StencilFuncSeparate (will be remapped) */
   "iiii\0"
   "glStencilFuncSeparate\0"
   "\0"
   /* _mesa_function_pool[9127]: SecondaryColor3sEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3s\0"
   "glSecondaryColor3sEXT\0"
   "\0"
   /* _mesa_function_pool[9173]: Color3fVertex3fvSUN (dynamic) */
   "pp\0"
   "glColor3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[9199]: Normal3fv (offset 57) */
   "p\0"
   "glNormal3fv\0"
   "\0"
   /* _mesa_function_pool[9214]: GlobalAlphaFactorbSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactorbSUN\0"
   "\0"
   /* _mesa_function_pool[9241]: Color3us (offset 23) */
   "iii\0"
   "glColor3us\0"
   "\0"
   /* _mesa_function_pool[9257]: ImageTransformParameterfvHP (dynamic) */
   "iip\0"
   "glImageTransformParameterfvHP\0"
   "\0"
   /* _mesa_function_pool[9292]: VertexAttrib4ivARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4iv\0"
   "glVertexAttrib4ivARB\0"
   "\0"
   /* _mesa_function_pool[9335]: End (offset 43) */
   "\0"
   "glEnd\0"
   "\0"
   /* _mesa_function_pool[9343]: VertexAttrib3fNV (will be remapped) */
   "ifff\0"
   "glVertexAttrib3fNV\0"
   "\0"
   /* _mesa_function_pool[9368]: VertexAttribs2dvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs2dvNV\0"
   "\0"
   /* _mesa_function_pool[9394]: GetQueryObjectui64vEXT (will be remapped) */
   "iip\0"
   "glGetQueryObjectui64vEXT\0"
   "\0"
   /* _mesa_function_pool[9424]: MultiTexCoord3fvARB (offset 395) */
   "ip\0"
   "glMultiTexCoord3fv\0"
   "glMultiTexCoord3fvARB\0"
   "\0"
   /* _mesa_function_pool[9469]: SecondaryColor3dEXT (will be remapped) */
   "ddd\0"
   "glSecondaryColor3d\0"
   "glSecondaryColor3dEXT\0"
   "\0"
   /* _mesa_function_pool[9515]: Color3ub (offset 19) */
   "iii\0"
   "glColor3ub\0"
   "\0"
   /* _mesa_function_pool[9531]: GetProgramParameterfvNV (will be remapped) */
   "iiip\0"
   "glGetProgramParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[9563]: TangentPointerEXT (dynamic) */
   "iip\0"
   "glTangentPointerEXT\0"
   "\0"
   /* _mesa_function_pool[9588]: Color4fNormal3fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glColor4fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[9623]: GetInstrumentsSGIX (dynamic) */
   "\0"
   "glGetInstrumentsSGIX\0"
   "\0"
   /* _mesa_function_pool[9646]: Color3ui (offset 21) */
   "iii\0"
   "glColor3ui\0"
   "\0"
   /* _mesa_function_pool[9662]: EvalMapsNV (dynamic) */
   "ii\0"
   "glEvalMapsNV\0"
   "\0"
   /* _mesa_function_pool[9679]: TexSubImage2D (offset 333) */
   "iiiiiiiip\0"
   "glTexSubImage2D\0"
   "glTexSubImage2DEXT\0"
   "\0"
   /* _mesa_function_pool[9725]: FragmentLightivSGIX (dynamic) */
   "iip\0"
   "glFragmentLightivSGIX\0"
   "\0"
   /* _mesa_function_pool[9752]: GetTexParameterPointervAPPLE (will be remapped) */
   "iip\0"
   "glGetTexParameterPointervAPPLE\0"
   "\0"
   /* _mesa_function_pool[9788]: TexGenfv (offset 191) */
   "iip\0"
   "glTexGenfv\0"
   "\0"
   /* _mesa_function_pool[9804]: GetTransformFeedbackVaryingEXT (will be remapped) */
   "iiipppp\0"
   "glGetTransformFeedbackVaryingEXT\0"
   "glGetTransformFeedbackVarying\0"
   "\0"
   /* _mesa_function_pool[9876]: VertexAttrib4bvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4bv\0"
   "glVertexAttrib4bvARB\0"
   "\0"
   /* _mesa_function_pool[9919]: AlphaFragmentOp2ATI (will be remapped) */
   "iiiiiiiii\0"
   "glAlphaFragmentOp2ATI\0"
   "\0"
   /* _mesa_function_pool[9952]: GetIntegerIndexedvEXT (will be remapped) */
   "iip\0"
   "glGetIntegerIndexedvEXT\0"
   "\0"
   /* _mesa_function_pool[9981]: MultiTexCoord4sARB (offset 406) */
   "iiiii\0"
   "glMultiTexCoord4s\0"
   "glMultiTexCoord4sARB\0"
   "\0"
   /* _mesa_function_pool[10027]: GetFragmentMaterialivSGIX (dynamic) */
   "iip\0"
   "glGetFragmentMaterialivSGIX\0"
   "\0"
   /* _mesa_function_pool[10060]: WindowPos4dMESA (will be remapped) */
   "dddd\0"
   "glWindowPos4dMESA\0"
   "\0"
   /* _mesa_function_pool[10084]: WeightPointerARB (dynamic) */
   "iiip\0"
   "glWeightPointerARB\0"
   "\0"
   /* _mesa_function_pool[10109]: WindowPos2dMESA (will be remapped) */
   "dd\0"
   "glWindowPos2d\0"
   "glWindowPos2dARB\0"
   "glWindowPos2dMESA\0"
   "\0"
   /* _mesa_function_pool[10162]: FramebufferTexture3DEXT (will be remapped) */
   "iiiiii\0"
   "glFramebufferTexture3D\0"
   "glFramebufferTexture3DEXT\0"
   "\0"
   /* _mesa_function_pool[10219]: BlendEquation (offset 337) */
   "i\0"
   "glBlendEquation\0"
   "glBlendEquationEXT\0"
   "\0"
   /* _mesa_function_pool[10257]: VertexAttrib3dNV (will be remapped) */
   "iddd\0"
   "glVertexAttrib3dNV\0"
   "\0"
   /* _mesa_function_pool[10282]: VertexAttrib3dARB (will be remapped) */
   "iddd\0"
   "glVertexAttrib3d\0"
   "glVertexAttrib3dARB\0"
   "\0"
   /* _mesa_function_pool[10325]: ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN (dynamic) */
   "ppppp\0"
   "glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[10389]: VertexAttrib4fARB (will be remapped) */
   "iffff\0"
   "glVertexAttrib4f\0"
   "glVertexAttrib4fARB\0"
   "\0"
   /* _mesa_function_pool[10433]: GetError (offset 261) */
   "\0"
   "glGetError\0"
   "\0"
   /* _mesa_function_pool[10446]: IndexFuncEXT (dynamic) */
   "if\0"
   "glIndexFuncEXT\0"
   "\0"
   /* _mesa_function_pool[10465]: TexCoord3dv (offset 111) */
   "p\0"
   "glTexCoord3dv\0"
   "\0"
   /* _mesa_function_pool[10482]: Indexdv (offset 45) */
   "p\0"
   "glIndexdv\0"
   "\0"
   /* _mesa_function_pool[10495]: FramebufferTexture2DEXT (will be remapped) */
   "iiiii\0"
   "glFramebufferTexture2D\0"
   "glFramebufferTexture2DEXT\0"
   "\0"
   /* _mesa_function_pool[10551]: Normal3s (offset 60) */
   "iii\0"
   "glNormal3s\0"
   "\0"
   /* _mesa_function_pool[10567]: GetObjectParameterivAPPLE (will be remapped) */
   "iiip\0"
   "glGetObjectParameterivAPPLE\0"
   "\0"
   /* _mesa_function_pool[10601]: PushName (offset 201) */
   "i\0"
   "glPushName\0"
   "\0"
   /* _mesa_function_pool[10615]: MultiTexCoord2dvARB (offset 385) */
   "ip\0"
   "glMultiTexCoord2dv\0"
   "glMultiTexCoord2dvARB\0"
   "\0"
   /* _mesa_function_pool[10660]: CullParameterfvEXT (will be remapped) */
   "ip\0"
   "glCullParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[10685]: Normal3i (offset 58) */
   "iii\0"
   "glNormal3i\0"
   "\0"
   /* _mesa_function_pool[10701]: ProgramNamedParameter4fvNV (will be remapped) */
   "iipp\0"
   "glProgramNamedParameter4fvNV\0"
   "\0"
   /* _mesa_function_pool[10736]: SecondaryColorPointerEXT (will be remapped) */
   "iiip\0"
   "glSecondaryColorPointer\0"
   "glSecondaryColorPointerEXT\0"
   "\0"
   /* _mesa_function_pool[10793]: VertexAttrib4fvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4fv\0"
   "glVertexAttrib4fvARB\0"
   "\0"
   /* _mesa_function_pool[10836]: ColorPointerListIBM (dynamic) */
   "iiipi\0"
   "glColorPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[10865]: GetActiveUniformARB (will be remapped) */
   "iiipppp\0"
   "glGetActiveUniform\0"
   "glGetActiveUniformARB\0"
   "\0"
   /* _mesa_function_pool[10915]: ImageTransformParameteriHP (dynamic) */
   "iii\0"
   "glImageTransformParameteriHP\0"
   "\0"
   /* _mesa_function_pool[10949]: Normal3b (offset 52) */
   "iii\0"
   "glNormal3b\0"
   "\0"
   /* _mesa_function_pool[10965]: Normal3d (offset 54) */
   "ddd\0"
   "glNormal3d\0"
   "\0"
   /* _mesa_function_pool[10981]: Normal3f (offset 56) */
   "fff\0"
   "glNormal3f\0"
   "\0"
   /* _mesa_function_pool[10997]: MultiTexCoord1svARB (offset 383) */
   "ip\0"
   "glMultiTexCoord1sv\0"
   "glMultiTexCoord1svARB\0"
   "\0"
   /* _mesa_function_pool[11042]: Indexi (offset 48) */
   "i\0"
   "glIndexi\0"
   "\0"
   /* _mesa_function_pool[11054]: EGLImageTargetTexture2DOES (will be remapped) */
   "ip\0"
   "glEGLImageTargetTexture2DOES\0"
   "\0"
   /* _mesa_function_pool[11087]: EndQueryARB (will be remapped) */
   "i\0"
   "glEndQuery\0"
   "glEndQueryARB\0"
   "\0"
   /* _mesa_function_pool[11115]: DeleteFencesNV (will be remapped) */
   "ip\0"
   "glDeleteFencesNV\0"
   "\0"
   /* _mesa_function_pool[11136]: BindBufferRangeEXT (will be remapped) */
   "iiiii\0"
   "glBindBufferRangeEXT\0"
   "glBindBufferRange\0"
   "\0"
   /* _mesa_function_pool[11182]: DepthMask (offset 211) */
   "i\0"
   "glDepthMask\0"
   "\0"
   /* _mesa_function_pool[11197]: IsShader (will be remapped) */
   "i\0"
   "glIsShader\0"
   "\0"
   /* _mesa_function_pool[11211]: Indexf (offset 46) */
   "f\0"
   "glIndexf\0"
   "\0"
   /* _mesa_function_pool[11223]: GetImageTransformParameterivHP (dynamic) */
   "iip\0"
   "glGetImageTransformParameterivHP\0"
   "\0"
   /* _mesa_function_pool[11261]: Indexd (offset 44) */
   "d\0"
   "glIndexd\0"
   "\0"
   /* _mesa_function_pool[11273]: GetMaterialiv (offset 270) */
   "iip\0"
   "glGetMaterialiv\0"
   "\0"
   /* _mesa_function_pool[11294]: StencilOp (offset 244) */
   "iii\0"
   "glStencilOp\0"
   "\0"
   /* _mesa_function_pool[11311]: WindowPos4ivMESA (will be remapped) */
   "p\0"
   "glWindowPos4ivMESA\0"
   "\0"
   /* _mesa_function_pool[11333]: FramebufferTextureLayer (dynamic) */
   "iiiii\0"
   "glFramebufferTextureLayerARB\0"
   "\0"
   /* _mesa_function_pool[11369]: MultiTexCoord3svARB (offset 399) */
   "ip\0"
   "glMultiTexCoord3sv\0"
   "glMultiTexCoord3svARB\0"
   "\0"
   /* _mesa_function_pool[11414]: TexEnvfv (offset 185) */
   "iip\0"
   "glTexEnvfv\0"
   "\0"
   /* _mesa_function_pool[11430]: MultiTexCoord4iARB (offset 404) */
   "iiiii\0"
   "glMultiTexCoord4i\0"
   "glMultiTexCoord4iARB\0"
   "\0"
   /* _mesa_function_pool[11476]: Indexs (offset 50) */
   "i\0"
   "glIndexs\0"
   "\0"
   /* _mesa_function_pool[11488]: Binormal3ivEXT (dynamic) */
   "p\0"
   "glBinormal3ivEXT\0"
   "\0"
   /* _mesa_function_pool[11508]: ResizeBuffersMESA (will be remapped) */
   "\0"
   "glResizeBuffersMESA\0"
   "\0"
   /* _mesa_function_pool[11530]: GetUniformivARB (will be remapped) */
   "iip\0"
   "glGetUniformiv\0"
   "glGetUniformivARB\0"
   "\0"
   /* _mesa_function_pool[11568]: PixelTexGenParameteriSGIS (will be remapped) */
   "ii\0"
   "glPixelTexGenParameteriSGIS\0"
   "\0"
   /* _mesa_function_pool[11600]: VertexPointervINTEL (dynamic) */
   "iip\0"
   "glVertexPointervINTEL\0"
   "\0"
   /* _mesa_function_pool[11627]: Vertex2i (offset 130) */
   "ii\0"
   "glVertex2i\0"
   "\0"
   /* _mesa_function_pool[11642]: LoadMatrixf (offset 291) */
   "p\0"
   "glLoadMatrixf\0"
   "\0"
   /* _mesa_function_pool[11659]: Vertex2f (offset 128) */
   "ff\0"
   "glVertex2f\0"
   "\0"
   /* _mesa_function_pool[11674]: ReplacementCodeuiColor4fNormal3fVertex3fvSUN (dynamic) */
   "pppp\0"
   "glReplacementCodeuiColor4fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[11727]: Color4bv (offset 26) */
   "p\0"
   "glColor4bv\0"
   "\0"
   /* _mesa_function_pool[11741]: VertexPointer (offset 321) */
   "iiip\0"
   "glVertexPointer\0"
   "\0"
   /* _mesa_function_pool[11763]: SecondaryColor3uiEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3ui\0"
   "glSecondaryColor3uiEXT\0"
   "\0"
   /* _mesa_function_pool[11811]: StartInstrumentsSGIX (dynamic) */
   "\0"
   "glStartInstrumentsSGIX\0"
   "\0"
   /* _mesa_function_pool[11836]: SecondaryColor3usvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3usv\0"
   "glSecondaryColor3usvEXT\0"
   "\0"
   /* _mesa_function_pool[11884]: VertexAttrib2fvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib2fvNV\0"
   "\0"
   /* _mesa_function_pool[11908]: ProgramLocalParameter4dvARB (will be remapped) */
   "iip\0"
   "glProgramLocalParameter4dvARB\0"
   "\0"
   /* _mesa_function_pool[11943]: DeleteLists (offset 4) */
   "ii\0"
   "glDeleteLists\0"
   "\0"
   /* _mesa_function_pool[11961]: LogicOp (offset 242) */
   "i\0"
   "glLogicOp\0"
   "\0"
   /* _mesa_function_pool[11974]: MatrixIndexuivARB (dynamic) */
   "ip\0"
   "glMatrixIndexuivARB\0"
   "\0"
   /* _mesa_function_pool[11998]: Vertex2s (offset 132) */
   "ii\0"
   "glVertex2s\0"
   "\0"
   /* _mesa_function_pool[12013]: RenderbufferStorageMultisample (will be remapped) */
   "iiiii\0"
   "glRenderbufferStorageMultisample\0"
   "glRenderbufferStorageMultisampleEXT\0"
   "\0"
   /* _mesa_function_pool[12089]: TexCoord4fv (offset 121) */
   "p\0"
   "glTexCoord4fv\0"
   "\0"
   /* _mesa_function_pool[12106]: Tangent3sEXT (dynamic) */
   "iii\0"
   "glTangent3sEXT\0"
   "\0"
   /* _mesa_function_pool[12126]: GlobalAlphaFactorfSUN (dynamic) */
   "f\0"
   "glGlobalAlphaFactorfSUN\0"
   "\0"
   /* _mesa_function_pool[12153]: MultiTexCoord3iARB (offset 396) */
   "iiii\0"
   "glMultiTexCoord3i\0"
   "glMultiTexCoord3iARB\0"
   "\0"
   /* _mesa_function_pool[12198]: IsProgram (will be remapped) */
   "i\0"
   "glIsProgram\0"
   "\0"
   /* _mesa_function_pool[12213]: TexCoordPointerListIBM (dynamic) */
   "iiipi\0"
   "glTexCoordPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[12245]: GlobalAlphaFactorusSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactorusSUN\0"
   "\0"
   /* _mesa_function_pool[12273]: VertexAttrib2dvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib2dvNV\0"
   "\0"
   /* _mesa_function_pool[12297]: FramebufferRenderbufferEXT (will be remapped) */
   "iiii\0"
   "glFramebufferRenderbuffer\0"
   "glFramebufferRenderbufferEXT\0"
   "\0"
   /* _mesa_function_pool[12358]: VertexAttrib1dvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib1dvNV\0"
   "\0"
   /* _mesa_function_pool[12382]: GenTextures (offset 328) */
   "ip\0"
   "glGenTextures\0"
   "glGenTexturesEXT\0"
   "\0"
   /* _mesa_function_pool[12417]: FramebufferTextureARB (will be remapped) */
   "iiii\0"
   "glFramebufferTextureARB\0"
   "\0"
   /* _mesa_function_pool[12447]: SetFenceNV (will be remapped) */
   "ii\0"
   "glSetFenceNV\0"
   "\0"
   /* _mesa_function_pool[12464]: FramebufferTexture1DEXT (will be remapped) */
   "iiiii\0"
   "glFramebufferTexture1D\0"
   "glFramebufferTexture1DEXT\0"
   "\0"
   /* _mesa_function_pool[12520]: GetCombinerOutputParameterivNV (will be remapped) */
   "iiip\0"
   "glGetCombinerOutputParameterivNV\0"
   "\0"
   /* _mesa_function_pool[12559]: MultiModeDrawArraysIBM (will be remapped) */
   "pppii\0"
   "glMultiModeDrawArraysIBM\0"
   "\0"
   /* _mesa_function_pool[12591]: PixelTexGenParameterivSGIS (will be remapped) */
   "ip\0"
   "glPixelTexGenParameterivSGIS\0"
   "\0"
   /* _mesa_function_pool[12624]: TextureNormalEXT (dynamic) */
   "i\0"
   "glTextureNormalEXT\0"
   "\0"
   /* _mesa_function_pool[12646]: IndexPointerListIBM (dynamic) */
   "iipi\0"
   "glIndexPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[12674]: WeightfvARB (dynamic) */
   "ip\0"
   "glWeightfvARB\0"
   "\0"
   /* _mesa_function_pool[12692]: GetCombinerOutputParameterfvNV (will be remapped) */
   "iiip\0"
   "glGetCombinerOutputParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[12731]: RasterPos2sv (offset 69) */
   "p\0"
   "glRasterPos2sv\0"
   "\0"
   /* _mesa_function_pool[12749]: Color4ubv (offset 36) */
   "p\0"
   "glColor4ubv\0"
   "\0"
   /* _mesa_function_pool[12764]: DrawBuffer (offset 202) */
   "i\0"
   "glDrawBuffer\0"
   "\0"
   /* _mesa_function_pool[12780]: TexCoord2fv (offset 105) */
   "p\0"
   "glTexCoord2fv\0"
   "\0"
   /* _mesa_function_pool[12797]: WindowPos4fMESA (will be remapped) */
   "ffff\0"
   "glWindowPos4fMESA\0"
   "\0"
   /* _mesa_function_pool[12821]: TexCoord1sv (offset 101) */
   "p\0"
   "glTexCoord1sv\0"
   "\0"
   /* _mesa_function_pool[12838]: WindowPos3dvMESA (will be remapped) */
   "p\0"
   "glWindowPos3dv\0"
   "glWindowPos3dvARB\0"
   "glWindowPos3dvMESA\0"
   "\0"
   /* _mesa_function_pool[12893]: DepthFunc (offset 245) */
   "i\0"
   "glDepthFunc\0"
   "\0"
   /* _mesa_function_pool[12908]: PixelMapusv (offset 253) */
   "iip\0"
   "glPixelMapusv\0"
   "\0"
   /* _mesa_function_pool[12927]: GetQueryObjecti64vEXT (will be remapped) */
   "iip\0"
   "glGetQueryObjecti64vEXT\0"
   "\0"
   /* _mesa_function_pool[12956]: MultiTexCoord1dARB (offset 376) */
   "id\0"
   "glMultiTexCoord1d\0"
   "glMultiTexCoord1dARB\0"
   "\0"
   /* _mesa_function_pool[12999]: PointParameterivNV (will be remapped) */
   "ip\0"
   "glPointParameteriv\0"
   "glPointParameterivNV\0"
   "\0"
   /* _mesa_function_pool[13043]: BlendFunc (offset 241) */
   "ii\0"
   "glBlendFunc\0"
   "\0"
   /* _mesa_function_pool[13059]: EndTransformFeedbackEXT (will be remapped) */
   "\0"
   "glEndTransformFeedbackEXT\0"
   "glEndTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[13110]: Uniform2fvARB (will be remapped) */
   "iip\0"
   "glUniform2fv\0"
   "glUniform2fvARB\0"
   "\0"
   /* _mesa_function_pool[13144]: BufferParameteriAPPLE (will be remapped) */
   "iii\0"
   "glBufferParameteriAPPLE\0"
   "\0"
   /* _mesa_function_pool[13173]: MultiTexCoord3dvARB (offset 393) */
   "ip\0"
   "glMultiTexCoord3dv\0"
   "glMultiTexCoord3dvARB\0"
   "\0"
   /* _mesa_function_pool[13218]: ReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN (dynamic) */
   "pppp\0"
   "glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[13274]: DeleteObjectARB (will be remapped) */
   "i\0"
   "glDeleteObjectARB\0"
   "\0"
   /* _mesa_function_pool[13295]: MatrixIndexPointerARB (dynamic) */
   "iiip\0"
   "glMatrixIndexPointerARB\0"
   "\0"
   /* _mesa_function_pool[13325]: ProgramNamedParameter4dvNV (will be remapped) */
   "iipp\0"
   "glProgramNamedParameter4dvNV\0"
   "\0"
   /* _mesa_function_pool[13360]: Tangent3fvEXT (dynamic) */
   "p\0"
   "glTangent3fvEXT\0"
   "\0"
   /* _mesa_function_pool[13379]: Flush (offset 217) */
   "\0"
   "glFlush\0"
   "\0"
   /* _mesa_function_pool[13389]: Color4uiv (offset 38) */
   "p\0"
   "glColor4uiv\0"
   "\0"
   /* _mesa_function_pool[13404]: GenVertexArrays (will be remapped) */
   "ip\0"
   "glGenVertexArrays\0"
   "\0"
   /* _mesa_function_pool[13426]: RasterPos3sv (offset 77) */
   "p\0"
   "glRasterPos3sv\0"
   "\0"
   /* _mesa_function_pool[13444]: BindFramebufferEXT (will be remapped) */
   "ii\0"
   "glBindFramebuffer\0"
   "glBindFramebufferEXT\0"
   "\0"
   /* _mesa_function_pool[13487]: ReferencePlaneSGIX (dynamic) */
   "p\0"
   "glReferencePlaneSGIX\0"
   "\0"
   /* _mesa_function_pool[13511]: PushAttrib (offset 219) */
   "i\0"
   "glPushAttrib\0"
   "\0"
   /* _mesa_function_pool[13527]: RasterPos2i (offset 66) */
   "ii\0"
   "glRasterPos2i\0"
   "\0"
   /* _mesa_function_pool[13545]: ValidateProgramARB (will be remapped) */
   "i\0"
   "glValidateProgram\0"
   "glValidateProgramARB\0"
   "\0"
   /* _mesa_function_pool[13587]: TexParameteriv (offset 181) */
   "iip\0"
   "glTexParameteriv\0"
   "\0"
   /* _mesa_function_pool[13609]: UnlockArraysEXT (will be remapped) */
   "\0"
   "glUnlockArraysEXT\0"
   "\0"
   /* _mesa_function_pool[13629]: TexCoord2fColor3fVertex3fSUN (dynamic) */
   "ffffffff\0"
   "glTexCoord2fColor3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[13670]: WindowPos3fvMESA (will be remapped) */
   "p\0"
   "glWindowPos3fv\0"
   "glWindowPos3fvARB\0"
   "glWindowPos3fvMESA\0"
   "\0"
   /* _mesa_function_pool[13725]: RasterPos2f (offset 64) */
   "ff\0"
   "glRasterPos2f\0"
   "\0"
   /* _mesa_function_pool[13743]: VertexAttrib1svNV (will be remapped) */
   "ip\0"
   "glVertexAttrib1svNV\0"
   "\0"
   /* _mesa_function_pool[13767]: RasterPos2d (offset 62) */
   "dd\0"
   "glRasterPos2d\0"
   "\0"
   /* _mesa_function_pool[13785]: RasterPos3fv (offset 73) */
   "p\0"
   "glRasterPos3fv\0"
   "\0"
   /* _mesa_function_pool[13803]: CopyTexSubImage3D (offset 373) */
   "iiiiiiiii\0"
   "glCopyTexSubImage3D\0"
   "glCopyTexSubImage3DEXT\0"
   "\0"
   /* _mesa_function_pool[13857]: VertexAttrib2dARB (will be remapped) */
   "idd\0"
   "glVertexAttrib2d\0"
   "glVertexAttrib2dARB\0"
   "\0"
   /* _mesa_function_pool[13899]: Color4ub (offset 35) */
   "iiii\0"
   "glColor4ub\0"
   "\0"
   /* _mesa_function_pool[13916]: GetInteger64v (will be remapped) */
   "ip\0"
   "glGetInteger64v\0"
   "\0"
   /* _mesa_function_pool[13936]: TextureColorMaskSGIS (dynamic) */
   "iiii\0"
   "glTextureColorMaskSGIS\0"
   "\0"
   /* _mesa_function_pool[13965]: RasterPos2s (offset 68) */
   "ii\0"
   "glRasterPos2s\0"
   "\0"
   /* _mesa_function_pool[13983]: GetColorTable (offset 343) */
   "iiip\0"
   "glGetColorTable\0"
   "glGetColorTableSGI\0"
   "glGetColorTableEXT\0"
   "\0"
   /* _mesa_function_pool[14043]: SelectBuffer (offset 195) */
   "ip\0"
   "glSelectBuffer\0"
   "\0"
   /* _mesa_function_pool[14062]: Indexiv (offset 49) */
   "p\0"
   "glIndexiv\0"
   "\0"
   /* _mesa_function_pool[14075]: TexCoord3i (offset 114) */
   "iii\0"
   "glTexCoord3i\0"
   "\0"
   /* _mesa_function_pool[14093]: CopyColorTable (offset 342) */
   "iiiii\0"
   "glCopyColorTable\0"
   "glCopyColorTableSGI\0"
   "\0"
   /* _mesa_function_pool[14137]: GetHistogramParameterfv (offset 362) */
   "iip\0"
   "glGetHistogramParameterfv\0"
   "glGetHistogramParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[14197]: Frustum (offset 289) */
   "dddddd\0"
   "glFrustum\0"
   "\0"
   /* _mesa_function_pool[14215]: GetString (offset 275) */
   "i\0"
   "glGetString\0"
   "\0"
   /* _mesa_function_pool[14230]: ColorPointervINTEL (dynamic) */
   "iip\0"
   "glColorPointervINTEL\0"
   "\0"
   /* _mesa_function_pool[14256]: TexEnvf (offset 184) */
   "iif\0"
   "glTexEnvf\0"
   "\0"
   /* _mesa_function_pool[14271]: TexCoord3d (offset 110) */
   "ddd\0"
   "glTexCoord3d\0"
   "\0"
   /* _mesa_function_pool[14289]: AlphaFragmentOp1ATI (will be remapped) */
   "iiiiii\0"
   "glAlphaFragmentOp1ATI\0"
   "\0"
   /* _mesa_function_pool[14319]: TexCoord3f (offset 112) */
   "fff\0"
   "glTexCoord3f\0"
   "\0"
   /* _mesa_function_pool[14337]: MultiTexCoord3ivARB (offset 397) */
   "ip\0"
   "glMultiTexCoord3iv\0"
   "glMultiTexCoord3ivARB\0"
   "\0"
   /* _mesa_function_pool[14382]: MultiTexCoord2sARB (offset 390) */
   "iii\0"
   "glMultiTexCoord2s\0"
   "glMultiTexCoord2sARB\0"
   "\0"
   /* _mesa_function_pool[14426]: VertexAttrib1dvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib1dv\0"
   "glVertexAttrib1dvARB\0"
   "\0"
   /* _mesa_function_pool[14469]: DeleteTextures (offset 327) */
   "ip\0"
   "glDeleteTextures\0"
   "glDeleteTexturesEXT\0"
   "\0"
   /* _mesa_function_pool[14510]: TexCoordPointerEXT (will be remapped) */
   "iiiip\0"
   "glTexCoordPointerEXT\0"
   "\0"
   /* _mesa_function_pool[14538]: TexSubImage4DSGIS (dynamic) */
   "iiiiiiiiiiiip\0"
   "glTexSubImage4DSGIS\0"
   "\0"
   /* _mesa_function_pool[14573]: TexCoord3s (offset 116) */
   "iii\0"
   "glTexCoord3s\0"
   "\0"
   /* _mesa_function_pool[14591]: GetTexLevelParameteriv (offset 285) */
   "iiip\0"
   "glGetTexLevelParameteriv\0"
   "\0"
   /* _mesa_function_pool[14622]: DrawArraysInstanced (will be remapped) */
   "iiii\0"
   "glDrawArraysInstanced\0"
   "glDrawArraysInstancedARB\0"
   "glDrawArraysInstancedEXT\0"
   "\0"
   /* _mesa_function_pool[14700]: CombinerStageParameterfvNV (dynamic) */
   "iip\0"
   "glCombinerStageParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[14734]: StopInstrumentsSGIX (dynamic) */
   "i\0"
   "glStopInstrumentsSGIX\0"
   "\0"
   /* _mesa_function_pool[14759]: TexCoord4fColor4fNormal3fVertex4fSUN (dynamic) */
   "fffffffffffffff\0"
   "glTexCoord4fColor4fNormal3fVertex4fSUN\0"
   "\0"
   /* _mesa_function_pool[14815]: ClearAccum (offset 204) */
   "ffff\0"
   "glClearAccum\0"
   "\0"
   /* _mesa_function_pool[14834]: DeformSGIX (dynamic) */
   "i\0"
   "glDeformSGIX\0"
   "\0"
   /* _mesa_function_pool[14850]: GetVertexAttribfvARB (will be remapped) */
   "iip\0"
   "glGetVertexAttribfv\0"
   "glGetVertexAttribfvARB\0"
   "\0"
   /* _mesa_function_pool[14898]: SecondaryColor3ivEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3iv\0"
   "glSecondaryColor3ivEXT\0"
   "\0"
   /* _mesa_function_pool[14944]: TexCoord4iv (offset 123) */
   "p\0"
   "glTexCoord4iv\0"
   "\0"
   /* _mesa_function_pool[14961]: UniformMatrix4x2fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix4x2fv\0"
   "\0"
   /* _mesa_function_pool[14988]: GetDetailTexFuncSGIS (dynamic) */
   "ip\0"
   "glGetDetailTexFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[15015]: GetCombinerStageParameterfvNV (dynamic) */
   "iip\0"
   "glGetCombinerStageParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[15052]: PolygonOffset (offset 319) */
   "ff\0"
   "glPolygonOffset\0"
   "\0"
   /* _mesa_function_pool[15072]: BindVertexArray (will be remapped) */
   "i\0"
   "glBindVertexArray\0"
   "\0"
   /* _mesa_function_pool[15093]: Color4ubVertex2fvSUN (dynamic) */
   "pp\0"
   "glColor4ubVertex2fvSUN\0"
   "\0"
   /* _mesa_function_pool[15120]: Rectd (offset 86) */
   "dddd\0"
   "glRectd\0"
   "\0"
   /* _mesa_function_pool[15134]: TexFilterFuncSGIS (dynamic) */
   "iiip\0"
   "glTexFilterFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[15160]: SampleMaskSGIS (will be remapped) */
   "fi\0"
   "glSampleMaskSGIS\0"
   "glSampleMaskEXT\0"
   "\0"
   /* _mesa_function_pool[15197]: GetAttribLocationARB (will be remapped) */
   "ip\0"
   "glGetAttribLocation\0"
   "glGetAttribLocationARB\0"
   "\0"
   /* _mesa_function_pool[15244]: RasterPos3i (offset 74) */
   "iii\0"
   "glRasterPos3i\0"
   "\0"
   /* _mesa_function_pool[15263]: VertexAttrib4ubvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4ubv\0"
   "glVertexAttrib4ubvARB\0"
   "\0"
   /* _mesa_function_pool[15308]: DetailTexFuncSGIS (dynamic) */
   "iip\0"
   "glDetailTexFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[15333]: Normal3fVertex3fSUN (dynamic) */
   "ffffff\0"
   "glNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[15363]: CopyTexImage2D (offset 324) */
   "iiiiiiii\0"
   "glCopyTexImage2D\0"
   "glCopyTexImage2DEXT\0"
   "\0"
   /* _mesa_function_pool[15410]: GetBufferPointervARB (will be remapped) */
   "iip\0"
   "glGetBufferPointerv\0"
   "glGetBufferPointervARB\0"
   "\0"
   /* _mesa_function_pool[15458]: ProgramEnvParameter4fARB (will be remapped) */
   "iiffff\0"
   "glProgramEnvParameter4fARB\0"
   "glProgramParameter4fNV\0"
   "\0"
   /* _mesa_function_pool[15516]: Uniform3ivARB (will be remapped) */
   "iip\0"
   "glUniform3iv\0"
   "glUniform3ivARB\0"
   "\0"
   /* _mesa_function_pool[15550]: Lightfv (offset 160) */
   "iip\0"
   "glLightfv\0"
   "\0"
   /* _mesa_function_pool[15565]: ClearDepth (offset 208) */
   "d\0"
   "glClearDepth\0"
   "\0"
   /* _mesa_function_pool[15581]: GetFenceivNV (will be remapped) */
   "iip\0"
   "glGetFenceivNV\0"
   "\0"
   /* _mesa_function_pool[15601]: WindowPos4dvMESA (will be remapped) */
   "p\0"
   "glWindowPos4dvMESA\0"
   "\0"
   /* _mesa_function_pool[15623]: ColorSubTable (offset 346) */
   "iiiiip\0"
   "glColorSubTable\0"
   "glColorSubTableEXT\0"
   "\0"
   /* _mesa_function_pool[15666]: Color4fv (offset 30) */
   "p\0"
   "glColor4fv\0"
   "\0"
   /* _mesa_function_pool[15680]: MultiTexCoord4ivARB (offset 405) */
   "ip\0"
   "glMultiTexCoord4iv\0"
   "glMultiTexCoord4ivARB\0"
   "\0"
   /* _mesa_function_pool[15725]: DrawElementsInstanced (will be remapped) */
   "iiipi\0"
   "glDrawElementsInstanced\0"
   "glDrawElementsInstancedARB\0"
   "glDrawElementsInstancedEXT\0"
   "\0"
   /* _mesa_function_pool[15810]: ColorPointer (offset 308) */
   "iiip\0"
   "glColorPointer\0"
   "\0"
   /* _mesa_function_pool[15831]: Rects (offset 92) */
   "iiii\0"
   "glRects\0"
   "\0"
   /* _mesa_function_pool[15845]: GetMapAttribParameterfvNV (dynamic) */
   "iiip\0"
   "glGetMapAttribParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[15879]: Lightiv (offset 162) */
   "iip\0"
   "glLightiv\0"
   "\0"
   /* _mesa_function_pool[15894]: VertexAttrib4sARB (will be remapped) */
   "iiiii\0"
   "glVertexAttrib4s\0"
   "glVertexAttrib4sARB\0"
   "\0"
   /* _mesa_function_pool[15938]: GetQueryObjectuivARB (will be remapped) */
   "iip\0"
   "glGetQueryObjectuiv\0"
   "glGetQueryObjectuivARB\0"
   "\0"
   /* _mesa_function_pool[15986]: GetTexParameteriv (offset 283) */
   "iip\0"
   "glGetTexParameteriv\0"
   "\0"
   /* _mesa_function_pool[16011]: MapParameterivNV (dynamic) */
   "iip\0"
   "glMapParameterivNV\0"
   "\0"
   /* _mesa_function_pool[16035]: GenRenderbuffersEXT (will be remapped) */
   "ip\0"
   "glGenRenderbuffers\0"
   "glGenRenderbuffersEXT\0"
   "\0"
   /* _mesa_function_pool[16080]: VertexAttrib2dvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib2dv\0"
   "glVertexAttrib2dvARB\0"
   "\0"
   /* _mesa_function_pool[16123]: EdgeFlagPointerEXT (will be remapped) */
   "iip\0"
   "glEdgeFlagPointerEXT\0"
   "\0"
   /* _mesa_function_pool[16149]: VertexAttribs2svNV (will be remapped) */
   "iip\0"
   "glVertexAttribs2svNV\0"
   "\0"
   /* _mesa_function_pool[16175]: WeightbvARB (dynamic) */
   "ip\0"
   "glWeightbvARB\0"
   "\0"
   /* _mesa_function_pool[16193]: VertexAttrib2fvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib2fv\0"
   "glVertexAttrib2fvARB\0"
   "\0"
   /* _mesa_function_pool[16236]: GetBufferParameterivARB (will be remapped) */
   "iip\0"
   "glGetBufferParameteriv\0"
   "glGetBufferParameterivARB\0"
   "\0"
   /* _mesa_function_pool[16290]: Rectdv (offset 87) */
   "pp\0"
   "glRectdv\0"
   "\0"
   /* _mesa_function_pool[16303]: ListParameteriSGIX (dynamic) */
   "iii\0"
   "glListParameteriSGIX\0"
   "\0"
   /* _mesa_function_pool[16329]: ReplacementCodeuiColor4fNormal3fVertex3fSUN (dynamic) */
   "iffffffffff\0"
   "glReplacementCodeuiColor4fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[16388]: InstrumentsBufferSGIX (dynamic) */
   "ip\0"
   "glInstrumentsBufferSGIX\0"
   "\0"
   /* _mesa_function_pool[16416]: VertexAttrib4NivARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Niv\0"
   "glVertexAttrib4NivARB\0"
   "\0"
   /* _mesa_function_pool[16461]: GetAttachedShaders (will be remapped) */
   "iipp\0"
   "glGetAttachedShaders\0"
   "\0"
   /* _mesa_function_pool[16488]: GenVertexArraysAPPLE (will be remapped) */
   "ip\0"
   "glGenVertexArraysAPPLE\0"
   "\0"
   /* _mesa_function_pool[16515]: Materialiv (offset 172) */
   "iip\0"
   "glMaterialiv\0"
   "\0"
   /* _mesa_function_pool[16533]: PushClientAttrib (offset 335) */
   "i\0"
   "glPushClientAttrib\0"
   "\0"
   /* _mesa_function_pool[16555]: ProgramEnvParameters4fvEXT (will be remapped) */
   "iiip\0"
   "glProgramEnvParameters4fvEXT\0"
   "\0"
   /* _mesa_function_pool[16590]: TexCoord2fColor4fNormal3fVertex3fvSUN (dynamic) */
   "pppp\0"
   "glTexCoord2fColor4fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[16636]: WindowPos2iMESA (will be remapped) */
   "ii\0"
   "glWindowPos2i\0"
   "glWindowPos2iARB\0"
   "glWindowPos2iMESA\0"
   "\0"
   /* _mesa_function_pool[16689]: SecondaryColor3fvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3fv\0"
   "glSecondaryColor3fvEXT\0"
   "\0"
   /* _mesa_function_pool[16735]: PolygonMode (offset 174) */
   "ii\0"
   "glPolygonMode\0"
   "\0"
   /* _mesa_function_pool[16753]: CompressedTexSubImage1DARB (will be remapped) */
   "iiiiiip\0"
   "glCompressedTexSubImage1D\0"
   "glCompressedTexSubImage1DARB\0"
   "\0"
   /* _mesa_function_pool[16817]: GetVertexAttribivNV (will be remapped) */
   "iip\0"
   "glGetVertexAttribivNV\0"
   "\0"
   /* _mesa_function_pool[16844]: GetProgramStringARB (will be remapped) */
   "iip\0"
   "glGetProgramStringARB\0"
   "\0"
   /* _mesa_function_pool[16871]: TexBumpParameterfvATI (will be remapped) */
   "ip\0"
   "glTexBumpParameterfvATI\0"
   "\0"
   /* _mesa_function_pool[16899]: CompileShaderARB (will be remapped) */
   "i\0"
   "glCompileShader\0"
   "glCompileShaderARB\0"
   "\0"
   /* _mesa_function_pool[16937]: DeleteShader (will be remapped) */
   "i\0"
   "glDeleteShader\0"
   "\0"
   /* _mesa_function_pool[16955]: DisableClientState (offset 309) */
   "i\0"
   "glDisableClientState\0"
   "\0"
   /* _mesa_function_pool[16979]: TexGeni (offset 192) */
   "iii\0"
   "glTexGeni\0"
   "\0"
   /* _mesa_function_pool[16994]: TexGenf (offset 190) */
   "iif\0"
   "glTexGenf\0"
   "\0"
   /* _mesa_function_pool[17009]: Uniform3fARB (will be remapped) */
   "ifff\0"
   "glUniform3f\0"
   "glUniform3fARB\0"
   "\0"
   /* _mesa_function_pool[17042]: TexGend (offset 188) */
   "iid\0"
   "glTexGend\0"
   "\0"
   /* _mesa_function_pool[17057]: ListParameterfvSGIX (dynamic) */
   "iip\0"
   "glListParameterfvSGIX\0"
   "\0"
   /* _mesa_function_pool[17084]: GetPolygonStipple (offset 274) */
   "p\0"
   "glGetPolygonStipple\0"
   "\0"
   /* _mesa_function_pool[17107]: Tangent3dvEXT (dynamic) */
   "p\0"
   "glTangent3dvEXT\0"
   "\0"
   /* _mesa_function_pool[17126]: BindBufferOffsetEXT (will be remapped) */
   "iiii\0"
   "glBindBufferOffsetEXT\0"
   "\0"
   /* _mesa_function_pool[17154]: WindowPos3sMESA (will be remapped) */
   "iii\0"
   "glWindowPos3s\0"
   "glWindowPos3sARB\0"
   "glWindowPos3sMESA\0"
   "\0"
   /* _mesa_function_pool[17208]: VertexAttrib2svNV (will be remapped) */
   "ip\0"
   "glVertexAttrib2svNV\0"
   "\0"
   /* _mesa_function_pool[17232]: DisableIndexedEXT (will be remapped) */
   "ii\0"
   "glDisableIndexedEXT\0"
   "\0"
   /* _mesa_function_pool[17256]: BindBufferBaseEXT (will be remapped) */
   "iii\0"
   "glBindBufferBaseEXT\0"
   "glBindBufferBase\0"
   "\0"
   /* _mesa_function_pool[17298]: TexCoord2fVertex3fvSUN (dynamic) */
   "pp\0"
   "glTexCoord2fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[17327]: WindowPos4sMESA (will be remapped) */
   "iiii\0"
   "glWindowPos4sMESA\0"
   "\0"
   /* _mesa_function_pool[17351]: VertexAttrib4NuivARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Nuiv\0"
   "glVertexAttrib4NuivARB\0"
   "\0"
   /* _mesa_function_pool[17398]: ClientActiveTextureARB (offset 375) */
   "i\0"
   "glClientActiveTexture\0"
   "glClientActiveTextureARB\0"
   "\0"
   /* _mesa_function_pool[17448]: PixelTexGenSGIX (will be remapped) */
   "i\0"
   "glPixelTexGenSGIX\0"
   "\0"
   /* _mesa_function_pool[17469]: ReplacementCodeusvSUN (dynamic) */
   "p\0"
   "glReplacementCodeusvSUN\0"
   "\0"
   /* _mesa_function_pool[17496]: Uniform4fARB (will be remapped) */
   "iffff\0"
   "glUniform4f\0"
   "glUniform4fARB\0"
   "\0"
   /* _mesa_function_pool[17530]: Color4sv (offset 34) */
   "p\0"
   "glColor4sv\0"
   "\0"
   /* _mesa_function_pool[17544]: FlushMappedBufferRange (will be remapped) */
   "iii\0"
   "glFlushMappedBufferRange\0"
   "\0"
   /* _mesa_function_pool[17574]: IsProgramNV (will be remapped) */
   "i\0"
   "glIsProgramARB\0"
   "glIsProgramNV\0"
   "\0"
   /* _mesa_function_pool[17606]: FlushMappedBufferRangeAPPLE (will be remapped) */
   "iii\0"
   "glFlushMappedBufferRangeAPPLE\0"
   "\0"
   /* _mesa_function_pool[17641]: PixelZoom (offset 246) */
   "ff\0"
   "glPixelZoom\0"
   "\0"
   /* _mesa_function_pool[17657]: ReplacementCodePointerSUN (dynamic) */
   "iip\0"
   "glReplacementCodePointerSUN\0"
   "\0"
   /* _mesa_function_pool[17690]: ProgramEnvParameter4dARB (will be remapped) */
   "iidddd\0"
   "glProgramEnvParameter4dARB\0"
   "glProgramParameter4dNV\0"
   "\0"
   /* _mesa_function_pool[17748]: ColorTableParameterfv (offset 340) */
   "iip\0"
   "glColorTableParameterfv\0"
   "glColorTableParameterfvSGI\0"
   "\0"
   /* _mesa_function_pool[17804]: FragmentLightModelfSGIX (dynamic) */
   "if\0"
   "glFragmentLightModelfSGIX\0"
   "\0"
   /* _mesa_function_pool[17834]: Binormal3bvEXT (dynamic) */
   "p\0"
   "glBinormal3bvEXT\0"
   "\0"
   /* _mesa_function_pool[17854]: PixelMapuiv (offset 252) */
   "iip\0"
   "glPixelMapuiv\0"
   "\0"
   /* _mesa_function_pool[17873]: Color3dv (offset 12) */
   "p\0"
   "glColor3dv\0"
   "\0"
   /* _mesa_function_pool[17887]: IsTexture (offset 330) */
   "i\0"
   "glIsTexture\0"
   "glIsTextureEXT\0"
   "\0"
   /* _mesa_function_pool[17917]: VertexWeightfvEXT (dynamic) */
   "p\0"
   "glVertexWeightfvEXT\0"
   "\0"
   /* _mesa_function_pool[17940]: VertexAttrib1dARB (will be remapped) */
   "id\0"
   "glVertexAttrib1d\0"
   "glVertexAttrib1dARB\0"
   "\0"
   /* _mesa_function_pool[17981]: ImageTransformParameterivHP (dynamic) */
   "iip\0"
   "glImageTransformParameterivHP\0"
   "\0"
   /* _mesa_function_pool[18016]: TexCoord4i (offset 122) */
   "iiii\0"
   "glTexCoord4i\0"
   "\0"
   /* _mesa_function_pool[18035]: DeleteQueriesARB (will be remapped) */
   "ip\0"
   "glDeleteQueries\0"
   "glDeleteQueriesARB\0"
   "\0"
   /* _mesa_function_pool[18074]: Color4ubVertex2fSUN (dynamic) */
   "iiiiff\0"
   "glColor4ubVertex2fSUN\0"
   "\0"
   /* _mesa_function_pool[18104]: FragmentColorMaterialSGIX (dynamic) */
   "ii\0"
   "glFragmentColorMaterialSGIX\0"
   "\0"
   /* _mesa_function_pool[18136]: CurrentPaletteMatrixARB (dynamic) */
   "i\0"
   "glCurrentPaletteMatrixARB\0"
   "\0"
   /* _mesa_function_pool[18165]: GetMapdv (offset 266) */
   "iip\0"
   "glGetMapdv\0"
   "\0"
   /* _mesa_function_pool[18181]: ObjectPurgeableAPPLE (will be remapped) */
   "iii\0"
   "glObjectPurgeableAPPLE\0"
   "\0"
   /* _mesa_function_pool[18209]: SamplePatternSGIS (will be remapped) */
   "i\0"
   "glSamplePatternSGIS\0"
   "glSamplePatternEXT\0"
   "\0"
   /* _mesa_function_pool[18251]: PixelStoref (offset 249) */
   "if\0"
   "glPixelStoref\0"
   "\0"
   /* _mesa_function_pool[18269]: IsQueryARB (will be remapped) */
   "i\0"
   "glIsQuery\0"
   "glIsQueryARB\0"
   "\0"
   /* _mesa_function_pool[18295]: ReplacementCodeuiColor4ubVertex3fSUN (dynamic) */
   "iiiiifff\0"
   "glReplacementCodeuiColor4ubVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[18344]: PixelStorei (offset 250) */
   "ii\0"
   "glPixelStorei\0"
   "\0"
   /* _mesa_function_pool[18362]: VertexAttrib4usvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4usv\0"
   "glVertexAttrib4usvARB\0"
   "\0"
   /* _mesa_function_pool[18407]: LinkProgramARB (will be remapped) */
   "i\0"
   "glLinkProgram\0"
   "glLinkProgramARB\0"
   "\0"
   /* _mesa_function_pool[18441]: VertexAttrib2fNV (will be remapped) */
   "iff\0"
   "glVertexAttrib2fNV\0"
   "\0"
   /* _mesa_function_pool[18465]: ShaderSourceARB (will be remapped) */
   "iipp\0"
   "glShaderSource\0"
   "glShaderSourceARB\0"
   "\0"
   /* _mesa_function_pool[18504]: FragmentMaterialiSGIX (dynamic) */
   "iii\0"
   "glFragmentMaterialiSGIX\0"
   "\0"
   /* _mesa_function_pool[18533]: EvalCoord2dv (offset 233) */
   "p\0"
   "glEvalCoord2dv\0"
   "\0"
   /* _mesa_function_pool[18551]: VertexAttrib3svARB (will be remapped) */
   "ip\0"
   "glVertexAttrib3sv\0"
   "glVertexAttrib3svARB\0"
   "\0"
   /* _mesa_function_pool[18594]: ColorMaterial (offset 151) */
   "ii\0"
   "glColorMaterial\0"
   "\0"
   /* _mesa_function_pool[18614]: CompressedTexSubImage3DARB (will be remapped) */
   "iiiiiiiiiip\0"
   "glCompressedTexSubImage3D\0"
   "glCompressedTexSubImage3DARB\0"
   "\0"
   /* _mesa_function_pool[18682]: WindowPos2ivMESA (will be remapped) */
   "p\0"
   "glWindowPos2iv\0"
   "glWindowPos2ivARB\0"
   "glWindowPos2ivMESA\0"
   "\0"
   /* _mesa_function_pool[18737]: IsFramebufferEXT (will be remapped) */
   "i\0"
   "glIsFramebuffer\0"
   "glIsFramebufferEXT\0"
   "\0"
   /* _mesa_function_pool[18775]: Uniform4ivARB (will be remapped) */
   "iip\0"
   "glUniform4iv\0"
   "glUniform4ivARB\0"
   "\0"
   /* _mesa_function_pool[18809]: GetVertexAttribdvARB (will be remapped) */
   "iip\0"
   "glGetVertexAttribdv\0"
   "glGetVertexAttribdvARB\0"
   "\0"
   /* _mesa_function_pool[18857]: TexBumpParameterivATI (will be remapped) */
   "ip\0"
   "glTexBumpParameterivATI\0"
   "\0"
   /* _mesa_function_pool[18885]: GetSeparableFilter (offset 359) */
   "iiippp\0"
   "glGetSeparableFilter\0"
   "glGetSeparableFilterEXT\0"
   "\0"
   /* _mesa_function_pool[18938]: Binormal3dEXT (dynamic) */
   "ddd\0"
   "glBinormal3dEXT\0"
   "\0"
   /* _mesa_function_pool[18959]: SpriteParameteriSGIX (dynamic) */
   "ii\0"
   "glSpriteParameteriSGIX\0"
   "\0"
   /* _mesa_function_pool[18986]: RequestResidentProgramsNV (will be remapped) */
   "ip\0"
   "glRequestResidentProgramsNV\0"
   "\0"
   /* _mesa_function_pool[19018]: TagSampleBufferSGIX (dynamic) */
   "\0"
   "glTagSampleBufferSGIX\0"
   "\0"
   /* _mesa_function_pool[19042]: TransformFeedbackVaryingsEXT (will be remapped) */
   "iipi\0"
   "glTransformFeedbackVaryingsEXT\0"
   "glTransformFeedbackVaryings\0"
   "\0"
   /* _mesa_function_pool[19107]: FeedbackBuffer (offset 194) */
   "iip\0"
   "glFeedbackBuffer\0"
   "\0"
   /* _mesa_function_pool[19129]: RasterPos2iv (offset 67) */
   "p\0"
   "glRasterPos2iv\0"
   "\0"
   /* _mesa_function_pool[19147]: TexImage1D (offset 182) */
   "iiiiiiip\0"
   "glTexImage1D\0"
   "\0"
   /* _mesa_function_pool[19170]: ListParameterivSGIX (dynamic) */
   "iip\0"
   "glListParameterivSGIX\0"
   "\0"
   /* _mesa_function_pool[19197]: MultiDrawElementsEXT (will be remapped) */
   "ipipi\0"
   "glMultiDrawElements\0"
   "glMultiDrawElementsEXT\0"
   "\0"
   /* _mesa_function_pool[19247]: Color3s (offset 17) */
   "iii\0"
   "glColor3s\0"
   "\0"
   /* _mesa_function_pool[19262]: Uniform1ivARB (will be remapped) */
   "iip\0"
   "glUniform1iv\0"
   "glUniform1ivARB\0"
   "\0"
   /* _mesa_function_pool[19296]: WindowPos2sMESA (will be remapped) */
   "ii\0"
   "glWindowPos2s\0"
   "glWindowPos2sARB\0"
   "glWindowPos2sMESA\0"
   "\0"
   /* _mesa_function_pool[19349]: WeightusvARB (dynamic) */
   "ip\0"
   "glWeightusvARB\0"
   "\0"
   /* _mesa_function_pool[19368]: TexCoordPointer (offset 320) */
   "iiip\0"
   "glTexCoordPointer\0"
   "\0"
   /* _mesa_function_pool[19392]: FogCoordPointerEXT (will be remapped) */
   "iip\0"
   "glFogCoordPointer\0"
   "glFogCoordPointerEXT\0"
   "\0"
   /* _mesa_function_pool[19436]: IndexMaterialEXT (dynamic) */
   "ii\0"
   "glIndexMaterialEXT\0"
   "\0"
   /* _mesa_function_pool[19459]: Color3i (offset 15) */
   "iii\0"
   "glColor3i\0"
   "\0"
   /* _mesa_function_pool[19474]: FrontFace (offset 157) */
   "i\0"
   "glFrontFace\0"
   "\0"
   /* _mesa_function_pool[19489]: EvalCoord2d (offset 232) */
   "dd\0"
   "glEvalCoord2d\0"
   "\0"
   /* _mesa_function_pool[19507]: SecondaryColor3ubvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3ubv\0"
   "glSecondaryColor3ubvEXT\0"
   "\0"
   /* _mesa_function_pool[19555]: EvalCoord2f (offset 234) */
   "ff\0"
   "glEvalCoord2f\0"
   "\0"
   /* _mesa_function_pool[19573]: VertexAttrib4dvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4dv\0"
   "glVertexAttrib4dvARB\0"
   "\0"
   /* _mesa_function_pool[19616]: BindAttribLocationARB (will be remapped) */
   "iip\0"
   "glBindAttribLocation\0"
   "glBindAttribLocationARB\0"
   "\0"
   /* _mesa_function_pool[19666]: Color3b (offset 9) */
   "iii\0"
   "glColor3b\0"
   "\0"
   /* _mesa_function_pool[19681]: MultiTexCoord2dARB (offset 384) */
   "idd\0"
   "glMultiTexCoord2d\0"
   "glMultiTexCoord2dARB\0"
   "\0"
   /* _mesa_function_pool[19725]: ExecuteProgramNV (will be remapped) */
   "iip\0"
   "glExecuteProgramNV\0"
   "\0"
   /* _mesa_function_pool[19749]: Color3f (offset 13) */
   "fff\0"
   "glColor3f\0"
   "\0"
   /* _mesa_function_pool[19764]: LightEnviSGIX (dynamic) */
   "ii\0"
   "glLightEnviSGIX\0"
   "\0"
   /* _mesa_function_pool[19784]: Color3d (offset 11) */
   "ddd\0"
   "glColor3d\0"
   "\0"
   /* _mesa_function_pool[19799]: Normal3dv (offset 55) */
   "p\0"
   "glNormal3dv\0"
   "\0"
   /* _mesa_function_pool[19814]: Lightf (offset 159) */
   "iif\0"
   "glLightf\0"
   "\0"
   /* _mesa_function_pool[19828]: ReplacementCodeuiSUN (dynamic) */
   "i\0"
   "glReplacementCodeuiSUN\0"
   "\0"
   /* _mesa_function_pool[19854]: MatrixMode (offset 293) */
   "i\0"
   "glMatrixMode\0"
   "\0"
   /* _mesa_function_pool[19870]: GetPixelMapusv (offset 273) */
   "ip\0"
   "glGetPixelMapusv\0"
   "\0"
   /* _mesa_function_pool[19891]: Lighti (offset 161) */
   "iii\0"
   "glLighti\0"
   "\0"
   /* _mesa_function_pool[19905]: VertexAttribPointerNV (will be remapped) */
   "iiiip\0"
   "glVertexAttribPointerNV\0"
   "\0"
   /* _mesa_function_pool[19936]: ProgramLocalParameters4fvEXT (will be remapped) */
   "iiip\0"
   "glProgramLocalParameters4fvEXT\0"
   "\0"
   /* _mesa_function_pool[19973]: GetBooleanIndexedvEXT (will be remapped) */
   "iip\0"
   "glGetBooleanIndexedvEXT\0"
   "\0"
   /* _mesa_function_pool[20002]: GetFramebufferAttachmentParameterivEXT (will be remapped) */
   "iiip\0"
   "glGetFramebufferAttachmentParameteriv\0"
   "glGetFramebufferAttachmentParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[20087]: PixelTransformParameterfEXT (dynamic) */
   "iif\0"
   "glPixelTransformParameterfEXT\0"
   "\0"
   /* _mesa_function_pool[20122]: MultiTexCoord4dvARB (offset 401) */
   "ip\0"
   "glMultiTexCoord4dv\0"
   "glMultiTexCoord4dvARB\0"
   "\0"
   /* _mesa_function_pool[20167]: PixelTransformParameteriEXT (dynamic) */
   "iii\0"
   "glPixelTransformParameteriEXT\0"
   "\0"
   /* _mesa_function_pool[20202]: GetDoublev (offset 260) */
   "ip\0"
   "glGetDoublev\0"
   "\0"
   /* _mesa_function_pool[20219]: MultMatrixd (offset 295) */
   "p\0"
   "glMultMatrixd\0"
   "\0"
   /* _mesa_function_pool[20236]: MultMatrixf (offset 294) */
   "p\0"
   "glMultMatrixf\0"
   "\0"
   /* _mesa_function_pool[20253]: TexCoord2fColor4ubVertex3fSUN (dynamic) */
   "ffiiiifff\0"
   "glTexCoord2fColor4ubVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[20296]: Uniform1iARB (will be remapped) */
   "ii\0"
   "glUniform1i\0"
   "glUniform1iARB\0"
   "\0"
   /* _mesa_function_pool[20327]: VertexAttribPointerARB (will be remapped) */
   "iiiiip\0"
   "glVertexAttribPointer\0"
   "glVertexAttribPointerARB\0"
   "\0"
   /* _mesa_function_pool[20382]: VertexAttrib3sNV (will be remapped) */
   "iiii\0"
   "glVertexAttrib3sNV\0"
   "\0"
   /* _mesa_function_pool[20407]: SharpenTexFuncSGIS (dynamic) */
   "iip\0"
   "glSharpenTexFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[20433]: MultiTexCoord4fvARB (offset 403) */
   "ip\0"
   "glMultiTexCoord4fv\0"
   "glMultiTexCoord4fvARB\0"
   "\0"
   /* _mesa_function_pool[20478]: UniformMatrix2x3fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix2x3fv\0"
   "\0"
   /* _mesa_function_pool[20505]: TrackMatrixNV (will be remapped) */
   "iiii\0"
   "glTrackMatrixNV\0"
   "\0"
   /* _mesa_function_pool[20527]: CombinerParameteriNV (will be remapped) */
   "ii\0"
   "glCombinerParameteriNV\0"
   "\0"
   /* _mesa_function_pool[20554]: DeleteAsyncMarkersSGIX (dynamic) */
   "ii\0"
   "glDeleteAsyncMarkersSGIX\0"
   "\0"
   /* _mesa_function_pool[20583]: ReplacementCodeusSUN (dynamic) */
   "i\0"
   "glReplacementCodeusSUN\0"
   "\0"
   /* _mesa_function_pool[20609]: IsAsyncMarkerSGIX (dynamic) */
   "i\0"
   "glIsAsyncMarkerSGIX\0"
   "\0"
   /* _mesa_function_pool[20632]: FrameZoomSGIX (dynamic) */
   "i\0"
   "glFrameZoomSGIX\0"
   "\0"
   /* _mesa_function_pool[20651]: Normal3fVertex3fvSUN (dynamic) */
   "pp\0"
   "glNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[20678]: RasterPos4sv (offset 85) */
   "p\0"
   "glRasterPos4sv\0"
   "\0"
   /* _mesa_function_pool[20696]: VertexAttrib4NsvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Nsv\0"
   "glVertexAttrib4NsvARB\0"
   "\0"
   /* _mesa_function_pool[20741]: VertexAttrib3fvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib3fv\0"
   "glVertexAttrib3fvARB\0"
   "\0"
   /* _mesa_function_pool[20784]: ClearColor (offset 206) */
   "ffff\0"
   "glClearColor\0"
   "\0"
   /* _mesa_function_pool[20803]: GetSynciv (will be remapped) */
   "iiipp\0"
   "glGetSynciv\0"
   "\0"
   /* _mesa_function_pool[20822]: DeleteFramebuffersEXT (will be remapped) */
   "ip\0"
   "glDeleteFramebuffers\0"
   "glDeleteFramebuffersEXT\0"
   "\0"
   /* _mesa_function_pool[20871]: GlobalAlphaFactorsSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactorsSUN\0"
   "\0"
   /* _mesa_function_pool[20898]: IsEnabledIndexedEXT (will be remapped) */
   "ii\0"
   "glIsEnabledIndexedEXT\0"
   "\0"
   /* _mesa_function_pool[20924]: TexEnviv (offset 187) */
   "iip\0"
   "glTexEnviv\0"
   "\0"
   /* _mesa_function_pool[20940]: TexSubImage3D (offset 372) */
   "iiiiiiiiiip\0"
   "glTexSubImage3D\0"
   "glTexSubImage3DEXT\0"
   "\0"
   /* _mesa_function_pool[20988]: Tangent3fEXT (dynamic) */
   "fff\0"
   "glTangent3fEXT\0"
   "\0"
   /* _mesa_function_pool[21008]: SecondaryColor3uivEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3uiv\0"
   "glSecondaryColor3uivEXT\0"
   "\0"
   /* _mesa_function_pool[21056]: MatrixIndexubvARB (dynamic) */
   "ip\0"
   "glMatrixIndexubvARB\0"
   "\0"
   /* _mesa_function_pool[21080]: Color4fNormal3fVertex3fSUN (dynamic) */
   "ffffffffff\0"
   "glColor4fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[21121]: PixelTexGenParameterfSGIS (will be remapped) */
   "if\0"
   "glPixelTexGenParameterfSGIS\0"
   "\0"
   /* _mesa_function_pool[21153]: CreateShader (will be remapped) */
   "i\0"
   "glCreateShader\0"
   "\0"
   /* _mesa_function_pool[21171]: GetColorTableParameterfv (offset 344) */
   "iip\0"
   "glGetColorTableParameterfv\0"
   "glGetColorTableParameterfvSGI\0"
   "glGetColorTableParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[21263]: FragmentLightModelfvSGIX (dynamic) */
   "ip\0"
   "glFragmentLightModelfvSGIX\0"
   "\0"
   /* _mesa_function_pool[21294]: Bitmap (offset 8) */
   "iiffffp\0"
   "glBitmap\0"
   "\0"
   /* _mesa_function_pool[21312]: MultiTexCoord3fARB (offset 394) */
   "ifff\0"
   "glMultiTexCoord3f\0"
   "glMultiTexCoord3fARB\0"
   "\0"
   /* _mesa_function_pool[21357]: GetTexLevelParameterfv (offset 284) */
   "iiip\0"
   "glGetTexLevelParameterfv\0"
   "\0"
   /* _mesa_function_pool[21388]: GetPixelTexGenParameterfvSGIS (will be remapped) */
   "ip\0"
   "glGetPixelTexGenParameterfvSGIS\0"
   "\0"
   /* _mesa_function_pool[21424]: GenFramebuffersEXT (will be remapped) */
   "ip\0"
   "glGenFramebuffers\0"
   "glGenFramebuffersEXT\0"
   "\0"
   /* _mesa_function_pool[21467]: GetProgramParameterdvNV (will be remapped) */
   "iiip\0"
   "glGetProgramParameterdvNV\0"
   "\0"
   /* _mesa_function_pool[21499]: Vertex2sv (offset 133) */
   "p\0"
   "glVertex2sv\0"
   "\0"
   /* _mesa_function_pool[21514]: GetIntegerv (offset 263) */
   "ip\0"
   "glGetIntegerv\0"
   "\0"
   /* _mesa_function_pool[21532]: IsVertexArrayAPPLE (will be remapped) */
   "i\0"
   "glIsVertexArray\0"
   "glIsVertexArrayAPPLE\0"
   "\0"
   /* _mesa_function_pool[21572]: FragmentLightfvSGIX (dynamic) */
   "iip\0"
   "glFragmentLightfvSGIX\0"
   "\0"
   /* _mesa_function_pool[21599]: DetachShader (will be remapped) */
   "ii\0"
   "glDetachShader\0"
   "\0"
   /* _mesa_function_pool[21618]: VertexAttrib4NubARB (will be remapped) */
   "iiiii\0"
   "glVertexAttrib4Nub\0"
   "glVertexAttrib4NubARB\0"
   "\0"
   /* _mesa_function_pool[21666]: GetProgramEnvParameterfvARB (will be remapped) */
   "iip\0"
   "glGetProgramEnvParameterfvARB\0"
   "\0"
   /* _mesa_function_pool[21701]: GetTrackMatrixivNV (will be remapped) */
   "iiip\0"
   "glGetTrackMatrixivNV\0"
   "\0"
   /* _mesa_function_pool[21728]: VertexAttrib3svNV (will be remapped) */
   "ip\0"
   "glVertexAttrib3svNV\0"
   "\0"
   /* _mesa_function_pool[21752]: Uniform4fvARB (will be remapped) */
   "iip\0"
   "glUniform4fv\0"
   "glUniform4fvARB\0"
   "\0"
   /* _mesa_function_pool[21786]: MultTransposeMatrixfARB (will be remapped) */
   "p\0"
   "glMultTransposeMatrixf\0"
   "glMultTransposeMatrixfARB\0"
   "\0"
   /* _mesa_function_pool[21838]: GetTexEnviv (offset 277) */
   "iip\0"
   "glGetTexEnviv\0"
   "\0"
   /* _mesa_function_pool[21857]: ColorFragmentOp1ATI (will be remapped) */
   "iiiiiii\0"
   "glColorFragmentOp1ATI\0"
   "\0"
   /* _mesa_function_pool[21888]: GetUniformfvARB (will be remapped) */
   "iip\0"
   "glGetUniformfv\0"
   "glGetUniformfvARB\0"
   "\0"
   /* _mesa_function_pool[21926]: EGLImageTargetRenderbufferStorageOES (will be remapped) */
   "ip\0"
   "glEGLImageTargetRenderbufferStorageOES\0"
   "\0"
   /* _mesa_function_pool[21969]: PopClientAttrib (offset 334) */
   "\0"
   "glPopClientAttrib\0"
   "\0"
   /* _mesa_function_pool[21989]: ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN (dynamic) */
   "iffffffffffff\0"
   "glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[22060]: DetachObjectARB (will be remapped) */
   "ii\0"
   "glDetachObjectARB\0"
   "\0"
   /* _mesa_function_pool[22082]: VertexBlendARB (dynamic) */
   "i\0"
   "glVertexBlendARB\0"
   "\0"
   /* _mesa_function_pool[22102]: WindowPos3iMESA (will be remapped) */
   "iii\0"
   "glWindowPos3i\0"
   "glWindowPos3iARB\0"
   "glWindowPos3iMESA\0"
   "\0"
   /* _mesa_function_pool[22156]: SeparableFilter2D (offset 360) */
   "iiiiiipp\0"
   "glSeparableFilter2D\0"
   "glSeparableFilter2DEXT\0"
   "\0"
   /* _mesa_function_pool[22209]: ProgramParameteriARB (will be remapped) */
   "iii\0"
   "glProgramParameteriARB\0"
   "\0"
   /* _mesa_function_pool[22237]: Map1d (offset 220) */
   "iddiip\0"
   "glMap1d\0"
   "\0"
   /* _mesa_function_pool[22253]: Map1f (offset 221) */
   "iffiip\0"
   "glMap1f\0"
   "\0"
   /* _mesa_function_pool[22269]: CompressedTexImage2DARB (will be remapped) */
   "iiiiiiip\0"
   "glCompressedTexImage2D\0"
   "glCompressedTexImage2DARB\0"
   "\0"
   /* _mesa_function_pool[22328]: ArrayElement (offset 306) */
   "i\0"
   "glArrayElement\0"
   "glArrayElementEXT\0"
   "\0"
   /* _mesa_function_pool[22364]: TexImage2D (offset 183) */
   "iiiiiiiip\0"
   "glTexImage2D\0"
   "\0"
   /* _mesa_function_pool[22388]: DepthBoundsEXT (will be remapped) */
   "dd\0"
   "glDepthBoundsEXT\0"
   "\0"
   /* _mesa_function_pool[22409]: ProgramParameters4fvNV (will be remapped) */
   "iiip\0"
   "glProgramParameters4fvNV\0"
   "\0"
   /* _mesa_function_pool[22440]: DeformationMap3fSGIX (dynamic) */
   "iffiiffiiffiip\0"
   "glDeformationMap3fSGIX\0"
   "\0"
   /* _mesa_function_pool[22479]: GetProgramivNV (will be remapped) */
   "iip\0"
   "glGetProgramivNV\0"
   "\0"
   /* _mesa_function_pool[22501]: GetMinmaxParameteriv (offset 366) */
   "iip\0"
   "glGetMinmaxParameteriv\0"
   "glGetMinmaxParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[22555]: PixelTransferf (offset 247) */
   "if\0"
   "glPixelTransferf\0"
   "\0"
   /* _mesa_function_pool[22576]: CopyTexImage1D (offset 323) */
   "iiiiiii\0"
   "glCopyTexImage1D\0"
   "glCopyTexImage1DEXT\0"
   "\0"
   /* _mesa_function_pool[22622]: PushMatrix (offset 298) */
   "\0"
   "glPushMatrix\0"
   "\0"
   /* _mesa_function_pool[22637]: Fogiv (offset 156) */
   "ip\0"
   "glFogiv\0"
   "\0"
   /* _mesa_function_pool[22649]: TexCoord1dv (offset 95) */
   "p\0"
   "glTexCoord1dv\0"
   "\0"
   /* _mesa_function_pool[22666]: AlphaFragmentOp3ATI (will be remapped) */
   "iiiiiiiiiiii\0"
   "glAlphaFragmentOp3ATI\0"
   "\0"
   /* _mesa_function_pool[22702]: PixelTransferi (offset 248) */
   "ii\0"
   "glPixelTransferi\0"
   "\0"
   /* _mesa_function_pool[22723]: GetVertexAttribdvNV (will be remapped) */
   "iip\0"
   "glGetVertexAttribdvNV\0"
   "\0"
   /* _mesa_function_pool[22750]: VertexAttrib3fvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib3fvNV\0"
   "\0"
   /* _mesa_function_pool[22774]: Rotatef (offset 300) */
   "ffff\0"
   "glRotatef\0"
   "\0"
   /* _mesa_function_pool[22790]: GetFinalCombinerInputParameterivNV (will be remapped) */
   "iip\0"
   "glGetFinalCombinerInputParameterivNV\0"
   "\0"
   /* _mesa_function_pool[22832]: Vertex3i (offset 138) */
   "iii\0"
   "glVertex3i\0"
   "\0"
   /* _mesa_function_pool[22848]: Vertex3f (offset 136) */
   "fff\0"
   "glVertex3f\0"
   "\0"
   /* _mesa_function_pool[22864]: Clear (offset 203) */
   "i\0"
   "glClear\0"
   "\0"
   /* _mesa_function_pool[22875]: Vertex3d (offset 134) */
   "ddd\0"
   "glVertex3d\0"
   "\0"
   /* _mesa_function_pool[22891]: GetMapParameterivNV (dynamic) */
   "iip\0"
   "glGetMapParameterivNV\0"
   "\0"
   /* _mesa_function_pool[22918]: Uniform4iARB (will be remapped) */
   "iiiii\0"
   "glUniform4i\0"
   "glUniform4iARB\0"
   "\0"
   /* _mesa_function_pool[22952]: ReadBuffer (offset 254) */
   "i\0"
   "glReadBuffer\0"
   "\0"
   /* _mesa_function_pool[22968]: ConvolutionParameteri (offset 352) */
   "iii\0"
   "glConvolutionParameteri\0"
   "glConvolutionParameteriEXT\0"
   "\0"
   /* _mesa_function_pool[23024]: Ortho (offset 296) */
   "dddddd\0"
   "glOrtho\0"
   "\0"
   /* _mesa_function_pool[23040]: Binormal3sEXT (dynamic) */
   "iii\0"
   "glBinormal3sEXT\0"
   "\0"
   /* _mesa_function_pool[23061]: ListBase (offset 6) */
   "i\0"
   "glListBase\0"
   "\0"
   /* _mesa_function_pool[23075]: Vertex3s (offset 140) */
   "iii\0"
   "glVertex3s\0"
   "\0"
   /* _mesa_function_pool[23091]: ConvolutionParameterf (offset 350) */
   "iif\0"
   "glConvolutionParameterf\0"
   "glConvolutionParameterfEXT\0"
   "\0"
   /* _mesa_function_pool[23147]: GetColorTableParameteriv (offset 345) */
   "iip\0"
   "glGetColorTableParameteriv\0"
   "glGetColorTableParameterivSGI\0"
   "glGetColorTableParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[23239]: ProgramEnvParameter4dvARB (will be remapped) */
   "iip\0"
   "glProgramEnvParameter4dvARB\0"
   "glProgramParameter4dvNV\0"
   "\0"
   /* _mesa_function_pool[23296]: ShadeModel (offset 177) */
   "i\0"
   "glShadeModel\0"
   "\0"
   /* _mesa_function_pool[23312]: VertexAttribs2fvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs2fvNV\0"
   "\0"
   /* _mesa_function_pool[23338]: Rectiv (offset 91) */
   "pp\0"
   "glRectiv\0"
   "\0"
   /* _mesa_function_pool[23351]: UseProgramObjectARB (will be remapped) */
   "i\0"
   "glUseProgram\0"
   "glUseProgramObjectARB\0"
   "\0"
   /* _mesa_function_pool[23389]: GetMapParameterfvNV (dynamic) */
   "iip\0"
   "glGetMapParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[23416]: EndConditionalRenderNV (will be remapped) */
   "\0"
   "glEndConditionalRenderNV\0"
   "\0"
   /* _mesa_function_pool[23443]: PassTexCoordATI (will be remapped) */
   "iii\0"
   "glPassTexCoordATI\0"
   "\0"
   /* _mesa_function_pool[23466]: DeleteProgram (will be remapped) */
   "i\0"
   "glDeleteProgram\0"
   "\0"
   /* _mesa_function_pool[23485]: Tangent3ivEXT (dynamic) */
   "p\0"
   "glTangent3ivEXT\0"
   "\0"
   /* _mesa_function_pool[23504]: Tangent3dEXT (dynamic) */
   "ddd\0"
   "glTangent3dEXT\0"
   "\0"
   /* _mesa_function_pool[23524]: SecondaryColor3dvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3dv\0"
   "glSecondaryColor3dvEXT\0"
   "\0"
   /* _mesa_function_pool[23570]: Vertex2fv (offset 129) */
   "p\0"
   "glVertex2fv\0"
   "\0"
   /* _mesa_function_pool[23585]: MultiDrawArraysEXT (will be remapped) */
   "ippi\0"
   "glMultiDrawArrays\0"
   "glMultiDrawArraysEXT\0"
   "\0"
   /* _mesa_function_pool[23630]: BindRenderbufferEXT (will be remapped) */
   "ii\0"
   "glBindRenderbuffer\0"
   "glBindRenderbufferEXT\0"
   "\0"
   /* _mesa_function_pool[23675]: MultiTexCoord4dARB (offset 400) */
   "idddd\0"
   "glMultiTexCoord4d\0"
   "glMultiTexCoord4dARB\0"
   "\0"
   /* _mesa_function_pool[23721]: FramebufferTextureFaceARB (will be remapped) */
   "iiiii\0"
   "glFramebufferTextureFaceARB\0"
   "\0"
   /* _mesa_function_pool[23756]: Vertex3sv (offset 141) */
   "p\0"
   "glVertex3sv\0"
   "\0"
   /* _mesa_function_pool[23771]: SecondaryColor3usEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3us\0"
   "glSecondaryColor3usEXT\0"
   "\0"
   /* _mesa_function_pool[23819]: ProgramLocalParameter4fvARB (will be remapped) */
   "iip\0"
   "glProgramLocalParameter4fvARB\0"
   "\0"
   /* _mesa_function_pool[23854]: DeleteProgramsNV (will be remapped) */
   "ip\0"
   "glDeleteProgramsARB\0"
   "glDeleteProgramsNV\0"
   "\0"
   /* _mesa_function_pool[23897]: EvalMesh1 (offset 236) */
   "iii\0"
   "glEvalMesh1\0"
   "\0"
   /* _mesa_function_pool[23914]: PauseTransformFeedback (will be remapped) */
   "\0"
   "glPauseTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[23941]: MultiTexCoord1sARB (offset 382) */
   "ii\0"
   "glMultiTexCoord1s\0"
   "glMultiTexCoord1sARB\0"
   "\0"
   /* _mesa_function_pool[23984]: ReplacementCodeuiColor3fVertex3fSUN (dynamic) */
   "iffffff\0"
   "glReplacementCodeuiColor3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[24031]: GetVertexAttribPointervNV (will be remapped) */
   "iip\0"
   "glGetVertexAttribPointerv\0"
   "glGetVertexAttribPointervARB\0"
   "glGetVertexAttribPointervNV\0"
   "\0"
   /* _mesa_function_pool[24119]: VertexAttribs1fvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs1fvNV\0"
   "\0"
   /* _mesa_function_pool[24145]: MultiTexCoord1dvARB (offset 377) */
   "ip\0"
   "glMultiTexCoord1dv\0"
   "glMultiTexCoord1dvARB\0"
   "\0"
   /* _mesa_function_pool[24190]: Uniform2iARB (will be remapped) */
   "iii\0"
   "glUniform2i\0"
   "glUniform2iARB\0"
   "\0"
   /* _mesa_function_pool[24222]: Vertex2iv (offset 131) */
   "p\0"
   "glVertex2iv\0"
   "\0"
   /* _mesa_function_pool[24237]: GetProgramStringNV (will be remapped) */
   "iip\0"
   "glGetProgramStringNV\0"
   "\0"
   /* _mesa_function_pool[24263]: ColorPointerEXT (will be remapped) */
   "iiiip\0"
   "glColorPointerEXT\0"
   "\0"
   /* _mesa_function_pool[24288]: LineWidth (offset 168) */
   "f\0"
   "glLineWidth\0"
   "\0"
   /* _mesa_function_pool[24303]: MapBufferARB (will be remapped) */
   "ii\0"
   "glMapBuffer\0"
   "glMapBufferARB\0"
   "\0"
   /* _mesa_function_pool[24334]: MultiDrawElementsBaseVertex (will be remapped) */
   "ipipip\0"
   "glMultiDrawElementsBaseVertex\0"
   "\0"
   /* _mesa_function_pool[24372]: Binormal3svEXT (dynamic) */
   "p\0"
   "glBinormal3svEXT\0"
   "\0"
   /* _mesa_function_pool[24392]: ApplyTextureEXT (dynamic) */
   "i\0"
   "glApplyTextureEXT\0"
   "\0"
   /* _mesa_function_pool[24413]: TexGendv (offset 189) */
   "iip\0"
   "glTexGendv\0"
   "\0"
   /* _mesa_function_pool[24429]: EnableIndexedEXT (will be remapped) */
   "ii\0"
   "glEnableIndexedEXT\0"
   "\0"
   /* _mesa_function_pool[24452]: TextureMaterialEXT (dynamic) */
   "ii\0"
   "glTextureMaterialEXT\0"
   "\0"
   /* _mesa_function_pool[24477]: TextureLightEXT (dynamic) */
   "i\0"
   "glTextureLightEXT\0"
   "\0"
   /* _mesa_function_pool[24498]: ResetMinmax (offset 370) */
   "i\0"
   "glResetMinmax\0"
   "glResetMinmaxEXT\0"
   "\0"
   /* _mesa_function_pool[24532]: SpriteParameterfSGIX (dynamic) */
   "if\0"
   "glSpriteParameterfSGIX\0"
   "\0"
   /* _mesa_function_pool[24559]: EnableClientState (offset 313) */
   "i\0"
   "glEnableClientState\0"
   "\0"
   /* _mesa_function_pool[24582]: VertexAttrib4sNV (will be remapped) */
   "iiiii\0"
   "glVertexAttrib4sNV\0"
   "\0"
   /* _mesa_function_pool[24608]: GetConvolutionParameterfv (offset 357) */
   "iip\0"
   "glGetConvolutionParameterfv\0"
   "glGetConvolutionParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[24672]: VertexAttribs4dvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs4dvNV\0"
   "\0"
   /* _mesa_function_pool[24698]: VertexAttrib4dARB (will be remapped) */
   "idddd\0"
   "glVertexAttrib4d\0"
   "glVertexAttrib4dARB\0"
   "\0"
   /* _mesa_function_pool[24742]: GetTexBumpParameterfvATI (will be remapped) */
   "ip\0"
   "glGetTexBumpParameterfvATI\0"
   "\0"
   /* _mesa_function_pool[24773]: ProgramNamedParameter4dNV (will be remapped) */
   "iipdddd\0"
   "glProgramNamedParameter4dNV\0"
   "\0"
   /* _mesa_function_pool[24810]: GetMaterialfv (offset 269) */
   "iip\0"
   "glGetMaterialfv\0"
   "\0"
   /* _mesa_function_pool[24831]: VertexWeightfEXT (dynamic) */
   "f\0"
   "glVertexWeightfEXT\0"
   "\0"
   /* _mesa_function_pool[24853]: Binormal3fEXT (dynamic) */
   "fff\0"
   "glBinormal3fEXT\0"
   "\0"
   /* _mesa_function_pool[24874]: CallList (offset 2) */
   "i\0"
   "glCallList\0"
   "\0"
   /* _mesa_function_pool[24888]: Materialfv (offset 170) */
   "iip\0"
   "glMaterialfv\0"
   "\0"
   /* _mesa_function_pool[24906]: TexCoord3fv (offset 113) */
   "p\0"
   "glTexCoord3fv\0"
   "\0"
   /* _mesa_function_pool[24923]: FogCoordfvEXT (will be remapped) */
   "p\0"
   "glFogCoordfv\0"
   "glFogCoordfvEXT\0"
   "\0"
   /* _mesa_function_pool[24955]: MultiTexCoord1ivARB (offset 381) */
   "ip\0"
   "glMultiTexCoord1iv\0"
   "glMultiTexCoord1ivARB\0"
   "\0"
   /* _mesa_function_pool[25000]: SecondaryColor3ubEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3ub\0"
   "glSecondaryColor3ubEXT\0"
   "\0"
   /* _mesa_function_pool[25048]: MultiTexCoord2ivARB (offset 389) */
   "ip\0"
   "glMultiTexCoord2iv\0"
   "glMultiTexCoord2ivARB\0"
   "\0"
   /* _mesa_function_pool[25093]: FogFuncSGIS (dynamic) */
   "ip\0"
   "glFogFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[25111]: CopyTexSubImage2D (offset 326) */
   "iiiiiiii\0"
   "glCopyTexSubImage2D\0"
   "glCopyTexSubImage2DEXT\0"
   "\0"
   /* _mesa_function_pool[25164]: GetObjectParameterivARB (will be remapped) */
   "iip\0"
   "glGetObjectParameterivARB\0"
   "\0"
   /* _mesa_function_pool[25195]: Color3iv (offset 16) */
   "p\0"
   "glColor3iv\0"
   "\0"
   /* _mesa_function_pool[25209]: TexCoord4fVertex4fSUN (dynamic) */
   "ffffffff\0"
   "glTexCoord4fVertex4fSUN\0"
   "\0"
   /* _mesa_function_pool[25243]: DrawElements (offset 311) */
   "iiip\0"
   "glDrawElements\0"
   "\0"
   /* _mesa_function_pool[25264]: BindVertexArrayAPPLE (will be remapped) */
   "i\0"
   "glBindVertexArrayAPPLE\0"
   "\0"
   /* _mesa_function_pool[25290]: GetProgramLocalParameterdvARB (will be remapped) */
   "iip\0"
   "glGetProgramLocalParameterdvARB\0"
   "\0"
   /* _mesa_function_pool[25327]: GetHistogramParameteriv (offset 363) */
   "iip\0"
   "glGetHistogramParameteriv\0"
   "glGetHistogramParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[25387]: MultiTexCoord1iARB (offset 380) */
   "ii\0"
   "glMultiTexCoord1i\0"
   "glMultiTexCoord1iARB\0"
   "\0"
   /* _mesa_function_pool[25430]: GetConvolutionFilter (offset 356) */
   "iiip\0"
   "glGetConvolutionFilter\0"
   "glGetConvolutionFilterEXT\0"
   "\0"
   /* _mesa_function_pool[25485]: GetProgramivARB (will be remapped) */
   "iip\0"
   "glGetProgramivARB\0"
   "\0"
   /* _mesa_function_pool[25508]: BlendFuncSeparateEXT (will be remapped) */
   "iiii\0"
   "glBlendFuncSeparate\0"
   "glBlendFuncSeparateEXT\0"
   "glBlendFuncSeparateINGR\0"
   "\0"
   /* _mesa_function_pool[25581]: MapBufferRange (will be remapped) */
   "iiii\0"
   "glMapBufferRange\0"
   "\0"
   /* _mesa_function_pool[25604]: ProgramParameters4dvNV (will be remapped) */
   "iiip\0"
   "glProgramParameters4dvNV\0"
   "\0"
   /* _mesa_function_pool[25635]: TexCoord2fColor3fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glTexCoord2fColor3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[25672]: EvalPoint2 (offset 239) */
   "ii\0"
   "glEvalPoint2\0"
   "\0"
   /* _mesa_function_pool[25689]: EvalPoint1 (offset 237) */
   "i\0"
   "glEvalPoint1\0"
   "\0"
   /* _mesa_function_pool[25705]: Binormal3dvEXT (dynamic) */
   "p\0"
   "glBinormal3dvEXT\0"
   "\0"
   /* _mesa_function_pool[25725]: PopMatrix (offset 297) */
   "\0"
   "glPopMatrix\0"
   "\0"
   /* _mesa_function_pool[25739]: FinishFenceNV (will be remapped) */
   "i\0"
   "glFinishFenceNV\0"
   "\0"
   /* _mesa_function_pool[25758]: GetFogFuncSGIS (dynamic) */
   "p\0"
   "glGetFogFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[25778]: GetUniformLocationARB (will be remapped) */
   "ip\0"
   "glGetUniformLocation\0"
   "glGetUniformLocationARB\0"
   "\0"
   /* _mesa_function_pool[25827]: SecondaryColor3fEXT (will be remapped) */
   "fff\0"
   "glSecondaryColor3f\0"
   "glSecondaryColor3fEXT\0"
   "\0"
   /* _mesa_function_pool[25873]: GetTexGeniv (offset 280) */
   "iip\0"
   "glGetTexGeniv\0"
   "\0"
   /* _mesa_function_pool[25892]: CombinerInputNV (will be remapped) */
   "iiiiii\0"
   "glCombinerInputNV\0"
   "\0"
   /* _mesa_function_pool[25918]: VertexAttrib3sARB (will be remapped) */
   "iiii\0"
   "glVertexAttrib3s\0"
   "glVertexAttrib3sARB\0"
   "\0"
   /* _mesa_function_pool[25961]: IsTransformFeedback (will be remapped) */
   "i\0"
   "glIsTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[25986]: ReplacementCodeuiNormal3fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glReplacementCodeuiNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[26031]: Map2d (offset 222) */
   "iddiiddiip\0"
   "glMap2d\0"
   "\0"
   /* _mesa_function_pool[26051]: Map2f (offset 223) */
   "iffiiffiip\0"
   "glMap2f\0"
   "\0"
   /* _mesa_function_pool[26071]: ProgramStringARB (will be remapped) */
   "iiip\0"
   "glProgramStringARB\0"
   "\0"
   /* _mesa_function_pool[26096]: Vertex4s (offset 148) */
   "iiii\0"
   "glVertex4s\0"
   "\0"
   /* _mesa_function_pool[26113]: TexCoord4fVertex4fvSUN (dynamic) */
   "pp\0"
   "glTexCoord4fVertex4fvSUN\0"
   "\0"
   /* _mesa_function_pool[26142]: FragmentLightModelivSGIX (dynamic) */
   "ip\0"
   "glFragmentLightModelivSGIX\0"
   "\0"
   /* _mesa_function_pool[26173]: VertexAttrib1fNV (will be remapped) */
   "if\0"
   "glVertexAttrib1fNV\0"
   "\0"
   /* _mesa_function_pool[26196]: Vertex4f (offset 144) */
   "ffff\0"
   "glVertex4f\0"
   "\0"
   /* _mesa_function_pool[26213]: EvalCoord1d (offset 228) */
   "d\0"
   "glEvalCoord1d\0"
   "\0"
   /* _mesa_function_pool[26230]: Vertex4d (offset 142) */
   "dddd\0"
   "glVertex4d\0"
   "\0"
   /* _mesa_function_pool[26247]: RasterPos4dv (offset 79) */
   "p\0"
   "glRasterPos4dv\0"
   "\0"
   /* _mesa_function_pool[26265]: FragmentLightfSGIX (dynamic) */
   "iif\0"
   "glFragmentLightfSGIX\0"
   "\0"
   /* _mesa_function_pool[26291]: GetCompressedTexImageARB (will be remapped) */
   "iip\0"
   "glGetCompressedTexImage\0"
   "glGetCompressedTexImageARB\0"
   "\0"
   /* _mesa_function_pool[26347]: GetTexGenfv (offset 279) */
   "iip\0"
   "glGetTexGenfv\0"
   "\0"
   /* _mesa_function_pool[26366]: Vertex4i (offset 146) */
   "iiii\0"
   "glVertex4i\0"
   "\0"
   /* _mesa_function_pool[26383]: VertexWeightPointerEXT (dynamic) */
   "iiip\0"
   "glVertexWeightPointerEXT\0"
   "\0"
   /* _mesa_function_pool[26414]: GetHistogram (offset 361) */
   "iiiip\0"
   "glGetHistogram\0"
   "glGetHistogramEXT\0"
   "\0"
   /* _mesa_function_pool[26454]: ActiveStencilFaceEXT (will be remapped) */
   "i\0"
   "glActiveStencilFaceEXT\0"
   "\0"
   /* _mesa_function_pool[26480]: StencilFuncSeparateATI (will be remapped) */
   "iiii\0"
   "glStencilFuncSeparateATI\0"
   "\0"
   /* _mesa_function_pool[26511]: Materialf (offset 169) */
   "iif\0"
   "glMaterialf\0"
   "\0"
   /* _mesa_function_pool[26528]: GetShaderSourceARB (will be remapped) */
   "iipp\0"
   "glGetShaderSource\0"
   "glGetShaderSourceARB\0"
   "\0"
   /* _mesa_function_pool[26573]: IglooInterfaceSGIX (dynamic) */
   "ip\0"
   "glIglooInterfaceSGIX\0"
   "\0"
   /* _mesa_function_pool[26598]: Materiali (offset 171) */
   "iii\0"
   "glMateriali\0"
   "\0"
   /* _mesa_function_pool[26615]: VertexAttrib4dNV (will be remapped) */
   "idddd\0"
   "glVertexAttrib4dNV\0"
   "\0"
   /* _mesa_function_pool[26641]: MultiModeDrawElementsIBM (will be remapped) */
   "ppipii\0"
   "glMultiModeDrawElementsIBM\0"
   "\0"
   /* _mesa_function_pool[26676]: Indexsv (offset 51) */
   "p\0"
   "glIndexsv\0"
   "\0"
   /* _mesa_function_pool[26689]: MultiTexCoord4svARB (offset 407) */
   "ip\0"
   "glMultiTexCoord4sv\0"
   "glMultiTexCoord4svARB\0"
   "\0"
   /* _mesa_function_pool[26734]: LightModelfv (offset 164) */
   "ip\0"
   "glLightModelfv\0"
   "\0"
   /* _mesa_function_pool[26753]: TexCoord2dv (offset 103) */
   "p\0"
   "glTexCoord2dv\0"
   "\0"
   /* _mesa_function_pool[26770]: GenQueriesARB (will be remapped) */
   "ip\0"
   "glGenQueries\0"
   "glGenQueriesARB\0"
   "\0"
   /* _mesa_function_pool[26803]: EvalCoord1dv (offset 229) */
   "p\0"
   "glEvalCoord1dv\0"
   "\0"
   /* _mesa_function_pool[26821]: ReplacementCodeuiVertex3fSUN (dynamic) */
   "ifff\0"
   "glReplacementCodeuiVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[26858]: Translated (offset 303) */
   "ddd\0"
   "glTranslated\0"
   "\0"
   /* _mesa_function_pool[26876]: Translatef (offset 304) */
   "fff\0"
   "glTranslatef\0"
   "\0"
   /* _mesa_function_pool[26894]: StencilMask (offset 209) */
   "i\0"
   "glStencilMask\0"
   "\0"
   /* _mesa_function_pool[26911]: Tangent3iEXT (dynamic) */
   "iii\0"
   "glTangent3iEXT\0"
   "\0"
   /* _mesa_function_pool[26931]: GetLightiv (offset 265) */
   "iip\0"
   "glGetLightiv\0"
   "\0"
   /* _mesa_function_pool[26949]: DrawMeshArraysSUN (dynamic) */
   "iiii\0"
   "glDrawMeshArraysSUN\0"
   "\0"
   /* _mesa_function_pool[26975]: IsList (offset 287) */
   "i\0"
   "glIsList\0"
   "\0"
   /* _mesa_function_pool[26987]: IsSync (will be remapped) */
   "i\0"
   "glIsSync\0"
   "\0"
   /* _mesa_function_pool[26999]: RenderMode (offset 196) */
   "i\0"
   "glRenderMode\0"
   "\0"
   /* _mesa_function_pool[27015]: GetMapControlPointsNV (dynamic) */
   "iiiiiip\0"
   "glGetMapControlPointsNV\0"
   "\0"
   /* _mesa_function_pool[27048]: DrawBuffersARB (will be remapped) */
   "ip\0"
   "glDrawBuffers\0"
   "glDrawBuffersARB\0"
   "glDrawBuffersATI\0"
   "\0"
   /* _mesa_function_pool[27100]: ProgramLocalParameter4fARB (will be remapped) */
   "iiffff\0"
   "glProgramLocalParameter4fARB\0"
   "\0"
   /* _mesa_function_pool[27137]: SpriteParameterivSGIX (dynamic) */
   "ip\0"
   "glSpriteParameterivSGIX\0"
   "\0"
   /* _mesa_function_pool[27165]: ProvokingVertexEXT (will be remapped) */
   "i\0"
   "glProvokingVertexEXT\0"
   "glProvokingVertex\0"
   "\0"
   /* _mesa_function_pool[27207]: MultiTexCoord1fARB (offset 378) */
   "if\0"
   "glMultiTexCoord1f\0"
   "glMultiTexCoord1fARB\0"
   "\0"
   /* _mesa_function_pool[27250]: LoadName (offset 198) */
   "i\0"
   "glLoadName\0"
   "\0"
   /* _mesa_function_pool[27264]: VertexAttribs4ubvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs4ubvNV\0"
   "\0"
   /* _mesa_function_pool[27291]: WeightsvARB (dynamic) */
   "ip\0"
   "glWeightsvARB\0"
   "\0"
   /* _mesa_function_pool[27309]: Uniform1fvARB (will be remapped) */
   "iip\0"
   "glUniform1fv\0"
   "glUniform1fvARB\0"
   "\0"
   /* _mesa_function_pool[27343]: CopyTexSubImage1D (offset 325) */
   "iiiiii\0"
   "glCopyTexSubImage1D\0"
   "glCopyTexSubImage1DEXT\0"
   "\0"
   /* _mesa_function_pool[27394]: CullFace (offset 152) */
   "i\0"
   "glCullFace\0"
   "\0"
   /* _mesa_function_pool[27408]: BindTexture (offset 307) */
   "ii\0"
   "glBindTexture\0"
   "glBindTextureEXT\0"
   "\0"
   /* _mesa_function_pool[27443]: BeginFragmentShaderATI (will be remapped) */
   "\0"
   "glBeginFragmentShaderATI\0"
   "\0"
   /* _mesa_function_pool[27470]: MultiTexCoord4fARB (offset 402) */
   "iffff\0"
   "glMultiTexCoord4f\0"
   "glMultiTexCoord4fARB\0"
   "\0"
   /* _mesa_function_pool[27516]: VertexAttribs3svNV (will be remapped) */
   "iip\0"
   "glVertexAttribs3svNV\0"
   "\0"
   /* _mesa_function_pool[27542]: StencilFunc (offset 243) */
   "iii\0"
   "glStencilFunc\0"
   "\0"
   /* _mesa_function_pool[27561]: CopyPixels (offset 255) */
   "iiiii\0"
   "glCopyPixels\0"
   "\0"
   /* _mesa_function_pool[27581]: Rectsv (offset 93) */
   "pp\0"
   "glRectsv\0"
   "\0"
   /* _mesa_function_pool[27594]: ReplacementCodeuivSUN (dynamic) */
   "p\0"
   "glReplacementCodeuivSUN\0"
   "\0"
   /* _mesa_function_pool[27621]: EnableVertexAttribArrayARB (will be remapped) */
   "i\0"
   "glEnableVertexAttribArray\0"
   "glEnableVertexAttribArrayARB\0"
   "\0"
   /* _mesa_function_pool[27679]: NormalPointervINTEL (dynamic) */
   "ip\0"
   "glNormalPointervINTEL\0"
   "\0"
   /* _mesa_function_pool[27705]: CopyConvolutionFilter2D (offset 355) */
   "iiiiii\0"
   "glCopyConvolutionFilter2D\0"
   "glCopyConvolutionFilter2DEXT\0"
   "\0"
   /* _mesa_function_pool[27768]: WindowPos3ivMESA (will be remapped) */
   "p\0"
   "glWindowPos3iv\0"
   "glWindowPos3ivARB\0"
   "glWindowPos3ivMESA\0"
   "\0"
   /* _mesa_function_pool[27823]: CopyBufferSubData (will be remapped) */
   "iiiii\0"
   "glCopyBufferSubData\0"
   "\0"
   /* _mesa_function_pool[27850]: NormalPointer (offset 318) */
   "iip\0"
   "glNormalPointer\0"
   "\0"
   /* _mesa_function_pool[27871]: TexParameterfv (offset 179) */
   "iip\0"
   "glTexParameterfv\0"
   "\0"
   /* _mesa_function_pool[27893]: IsBufferARB (will be remapped) */
   "i\0"
   "glIsBuffer\0"
   "glIsBufferARB\0"
   "\0"
   /* _mesa_function_pool[27921]: WindowPos4iMESA (will be remapped) */
   "iiii\0"
   "glWindowPos4iMESA\0"
   "\0"
   /* _mesa_function_pool[27945]: VertexAttrib4uivARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4uiv\0"
   "glVertexAttrib4uivARB\0"
   "\0"
   /* _mesa_function_pool[27990]: Tangent3bvEXT (dynamic) */
   "p\0"
   "glTangent3bvEXT\0"
   "\0"
   /* _mesa_function_pool[28009]: UniformMatrix3x4fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix3x4fv\0"
   "\0"
   /* _mesa_function_pool[28036]: ClipPlane (offset 150) */
   "ip\0"
   "glClipPlane\0"
   "\0"
   /* _mesa_function_pool[28052]: Recti (offset 90) */
   "iiii\0"
   "glRecti\0"
   "\0"
   /* _mesa_function_pool[28066]: DrawRangeElementsBaseVertex (will be remapped) */
   "iiiiipi\0"
   "glDrawRangeElementsBaseVertex\0"
   "\0"
   /* _mesa_function_pool[28105]: TexCoordPointervINTEL (dynamic) */
   "iip\0"
   "glTexCoordPointervINTEL\0"
   "\0"
   /* _mesa_function_pool[28134]: DeleteBuffersARB (will be remapped) */
   "ip\0"
   "glDeleteBuffers\0"
   "glDeleteBuffersARB\0"
   "\0"
   /* _mesa_function_pool[28173]: PixelTransformParameterfvEXT (dynamic) */
   "iip\0"
   "glPixelTransformParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[28209]: WindowPos4fvMESA (will be remapped) */
   "p\0"
   "glWindowPos4fvMESA\0"
   "\0"
   /* _mesa_function_pool[28231]: GetPixelMapuiv (offset 272) */
   "ip\0"
   "glGetPixelMapuiv\0"
   "\0"
   /* _mesa_function_pool[28252]: Rectf (offset 88) */
   "ffff\0"
   "glRectf\0"
   "\0"
   /* _mesa_function_pool[28266]: VertexAttrib1sNV (will be remapped) */
   "ii\0"
   "glVertexAttrib1sNV\0"
   "\0"
   /* _mesa_function_pool[28289]: Indexfv (offset 47) */
   "p\0"
   "glIndexfv\0"
   "\0"
   /* _mesa_function_pool[28302]: SecondaryColor3svEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3sv\0"
   "glSecondaryColor3svEXT\0"
   "\0"
   /* _mesa_function_pool[28348]: LoadTransposeMatrixfARB (will be remapped) */
   "p\0"
   "glLoadTransposeMatrixf\0"
   "glLoadTransposeMatrixfARB\0"
   "\0"
   /* _mesa_function_pool[28400]: GetPointerv (offset 329) */
   "ip\0"
   "glGetPointerv\0"
   "glGetPointervEXT\0"
   "\0"
   /* _mesa_function_pool[28435]: Tangent3bEXT (dynamic) */
   "iii\0"
   "glTangent3bEXT\0"
   "\0"
   /* _mesa_function_pool[28455]: CombinerParameterfNV (will be remapped) */
   "if\0"
   "glCombinerParameterfNV\0"
   "\0"
   /* _mesa_function_pool[28482]: IndexMask (offset 212) */
   "i\0"
   "glIndexMask\0"
   "\0"
   /* _mesa_function_pool[28497]: BindProgramNV (will be remapped) */
   "ii\0"
   "glBindProgramARB\0"
   "glBindProgramNV\0"
   "\0"
   /* _mesa_function_pool[28534]: VertexAttrib4svARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4sv\0"
   "glVertexAttrib4svARB\0"
   "\0"
   /* _mesa_function_pool[28577]: GetFloatv (offset 262) */
   "ip\0"
   "glGetFloatv\0"
   "\0"
   /* _mesa_function_pool[28593]: CreateDebugObjectMESA (dynamic) */
   "\0"
   "glCreateDebugObjectMESA\0"
   "\0"
   /* _mesa_function_pool[28619]: GetShaderiv (will be remapped) */
   "iip\0"
   "glGetShaderiv\0"
   "\0"
   /* _mesa_function_pool[28638]: ClientWaitSync (will be remapped) */
   "iii\0"
   "glClientWaitSync\0"
   "\0"
   /* _mesa_function_pool[28660]: TexCoord4s (offset 124) */
   "iiii\0"
   "glTexCoord4s\0"
   "\0"
   /* _mesa_function_pool[28679]: TexCoord3sv (offset 117) */
   "p\0"
   "glTexCoord3sv\0"
   "\0"
   /* _mesa_function_pool[28696]: BindFragmentShaderATI (will be remapped) */
   "i\0"
   "glBindFragmentShaderATI\0"
   "\0"
   /* _mesa_function_pool[28723]: PopAttrib (offset 218) */
   "\0"
   "glPopAttrib\0"
   "\0"
   /* _mesa_function_pool[28737]: Fogfv (offset 154) */
   "ip\0"
   "glFogfv\0"
   "\0"
   /* _mesa_function_pool[28749]: UnmapBufferARB (will be remapped) */
   "i\0"
   "glUnmapBuffer\0"
   "glUnmapBufferARB\0"
   "\0"
   /* _mesa_function_pool[28783]: InitNames (offset 197) */
   "\0"
   "glInitNames\0"
   "\0"
   /* _mesa_function_pool[28797]: Normal3sv (offset 61) */
   "p\0"
   "glNormal3sv\0"
   "\0"
   /* _mesa_function_pool[28812]: Minmax (offset 368) */
   "iii\0"
   "glMinmax\0"
   "glMinmaxEXT\0"
   "\0"
   /* _mesa_function_pool[28838]: TexCoord4d (offset 118) */
   "dddd\0"
   "glTexCoord4d\0"
   "\0"
   /* _mesa_function_pool[28857]: DeformationMap3dSGIX (dynamic) */
   "iddiiddiiddiip\0"
   "glDeformationMap3dSGIX\0"
   "\0"
   /* _mesa_function_pool[28896]: TexCoord4f (offset 120) */
   "ffff\0"
   "glTexCoord4f\0"
   "\0"
   /* _mesa_function_pool[28915]: FogCoorddvEXT (will be remapped) */
   "p\0"
   "glFogCoorddv\0"
   "glFogCoorddvEXT\0"
   "\0"
   /* _mesa_function_pool[28947]: FinishTextureSUNX (dynamic) */
   "\0"
   "glFinishTextureSUNX\0"
   "\0"
   /* _mesa_function_pool[28969]: GetFragmentLightfvSGIX (dynamic) */
   "iip\0"
   "glGetFragmentLightfvSGIX\0"
   "\0"
   /* _mesa_function_pool[28999]: Binormal3fvEXT (dynamic) */
   "p\0"
   "glBinormal3fvEXT\0"
   "\0"
   /* _mesa_function_pool[29019]: GetBooleanv (offset 258) */
   "ip\0"
   "glGetBooleanv\0"
   "\0"
   /* _mesa_function_pool[29037]: ColorFragmentOp3ATI (will be remapped) */
   "iiiiiiiiiiiii\0"
   "glColorFragmentOp3ATI\0"
   "\0"
   /* _mesa_function_pool[29074]: Hint (offset 158) */
   "ii\0"
   "glHint\0"
   "\0"
   /* _mesa_function_pool[29085]: Color4dv (offset 28) */
   "p\0"
   "glColor4dv\0"
   "\0"
   /* _mesa_function_pool[29099]: VertexAttrib2svARB (will be remapped) */
   "ip\0"
   "glVertexAttrib2sv\0"
   "glVertexAttrib2svARB\0"
   "\0"
   /* _mesa_function_pool[29142]: AreProgramsResidentNV (will be remapped) */
   "ipp\0"
   "glAreProgramsResidentNV\0"
   "\0"
   /* _mesa_function_pool[29171]: WindowPos3svMESA (will be remapped) */
   "p\0"
   "glWindowPos3sv\0"
   "glWindowPos3svARB\0"
   "glWindowPos3svMESA\0"
   "\0"
   /* _mesa_function_pool[29226]: CopyColorSubTable (offset 347) */
   "iiiii\0"
   "glCopyColorSubTable\0"
   "glCopyColorSubTableEXT\0"
   "\0"
   /* _mesa_function_pool[29276]: WeightdvARB (dynamic) */
   "ip\0"
   "glWeightdvARB\0"
   "\0"
   /* _mesa_function_pool[29294]: DeleteRenderbuffersEXT (will be remapped) */
   "ip\0"
   "glDeleteRenderbuffers\0"
   "glDeleteRenderbuffersEXT\0"
   "\0"
   /* _mesa_function_pool[29345]: VertexAttrib4NubvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Nubv\0"
   "glVertexAttrib4NubvARB\0"
   "\0"
   /* _mesa_function_pool[29392]: VertexAttrib3dvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib3dvNV\0"
   "\0"
   /* _mesa_function_pool[29416]: GetObjectParameterfvARB (will be remapped) */
   "iip\0"
   "glGetObjectParameterfvARB\0"
   "\0"
   /* _mesa_function_pool[29447]: Vertex4iv (offset 147) */
   "p\0"
   "glVertex4iv\0"
   "\0"
   /* _mesa_function_pool[29462]: GetProgramEnvParameterdvARB (will be remapped) */
   "iip\0"
   "glGetProgramEnvParameterdvARB\0"
   "\0"
   /* _mesa_function_pool[29497]: TexCoord4dv (offset 119) */
   "p\0"
   "glTexCoord4dv\0"
   "\0"
   /* _mesa_function_pool[29514]: LockArraysEXT (will be remapped) */
   "ii\0"
   "glLockArraysEXT\0"
   "\0"
   /* _mesa_function_pool[29534]: Begin (offset 7) */
   "i\0"
   "glBegin\0"
   "\0"
   /* _mesa_function_pool[29545]: LightModeli (offset 165) */
   "ii\0"
   "glLightModeli\0"
   "\0"
   /* _mesa_function_pool[29563]: Rectfv (offset 89) */
   "pp\0"
   "glRectfv\0"
   "\0"
   /* _mesa_function_pool[29576]: LightModelf (offset 163) */
   "if\0"
   "glLightModelf\0"
   "\0"
   /* _mesa_function_pool[29594]: GetTexParameterfv (offset 282) */
   "iip\0"
   "glGetTexParameterfv\0"
   "\0"
   /* _mesa_function_pool[29619]: GetLightfv (offset 264) */
   "iip\0"
   "glGetLightfv\0"
   "\0"
   /* _mesa_function_pool[29637]: PixelTransformParameterivEXT (dynamic) */
   "iip\0"
   "glPixelTransformParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[29673]: BinormalPointerEXT (dynamic) */
   "iip\0"
   "glBinormalPointerEXT\0"
   "\0"
   /* _mesa_function_pool[29699]: VertexAttrib1dNV (will be remapped) */
   "id\0"
   "glVertexAttrib1dNV\0"
   "\0"
   /* _mesa_function_pool[29722]: GetCombinerInputParameterivNV (will be remapped) */
   "iiiip\0"
   "glGetCombinerInputParameterivNV\0"
   "\0"
   /* _mesa_function_pool[29761]: Disable (offset 214) */
   "i\0"
   "glDisable\0"
   "\0"
   /* _mesa_function_pool[29774]: MultiTexCoord2fvARB (offset 387) */
   "ip\0"
   "glMultiTexCoord2fv\0"
   "glMultiTexCoord2fvARB\0"
   "\0"
   /* _mesa_function_pool[29819]: GetRenderbufferParameterivEXT (will be remapped) */
   "iip\0"
   "glGetRenderbufferParameteriv\0"
   "glGetRenderbufferParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[29885]: CombinerParameterivNV (will be remapped) */
   "ip\0"
   "glCombinerParameterivNV\0"
   "\0"
   /* _mesa_function_pool[29913]: GenFragmentShadersATI (will be remapped) */
   "i\0"
   "glGenFragmentShadersATI\0"
   "\0"
   /* _mesa_function_pool[29940]: DrawArrays (offset 310) */
   "iii\0"
   "glDrawArrays\0"
   "glDrawArraysEXT\0"
   "\0"
   /* _mesa_function_pool[29974]: WeightuivARB (dynamic) */
   "ip\0"
   "glWeightuivARB\0"
   "\0"
   /* _mesa_function_pool[29993]: VertexAttrib2sARB (will be remapped) */
   "iii\0"
   "glVertexAttrib2s\0"
   "glVertexAttrib2sARB\0"
   "\0"
   /* _mesa_function_pool[30035]: ColorMask (offset 210) */
   "iiii\0"
   "glColorMask\0"
   "\0"
   /* _mesa_function_pool[30053]: GenAsyncMarkersSGIX (dynamic) */
   "i\0"
   "glGenAsyncMarkersSGIX\0"
   "\0"
   /* _mesa_function_pool[30078]: Tangent3svEXT (dynamic) */
   "p\0"
   "glTangent3svEXT\0"
   "\0"
   /* _mesa_function_pool[30097]: GetListParameterivSGIX (dynamic) */
   "iip\0"
   "glGetListParameterivSGIX\0"
   "\0"
   /* _mesa_function_pool[30127]: BindBufferARB (will be remapped) */
   "ii\0"
   "glBindBuffer\0"
   "glBindBufferARB\0"
   "\0"
   /* _mesa_function_pool[30160]: GetInfoLogARB (will be remapped) */
   "iipp\0"
   "glGetInfoLogARB\0"
   "\0"
   /* _mesa_function_pool[30182]: RasterPos4iv (offset 83) */
   "p\0"
   "glRasterPos4iv\0"
   "\0"
   /* _mesa_function_pool[30200]: Enable (offset 215) */
   "i\0"
   "glEnable\0"
   "\0"
   /* _mesa_function_pool[30212]: LineStipple (offset 167) */
   "ii\0"
   "glLineStipple\0"
   "\0"
   /* _mesa_function_pool[30230]: VertexAttribs4svNV (will be remapped) */
   "iip\0"
   "glVertexAttribs4svNV\0"
   "\0"
   /* _mesa_function_pool[30256]: EdgeFlagPointerListIBM (dynamic) */
   "ipi\0"
   "glEdgeFlagPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[30286]: UniformMatrix3x2fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix3x2fv\0"
   "\0"
   /* _mesa_function_pool[30313]: GetMinmaxParameterfv (offset 365) */
   "iip\0"
   "glGetMinmaxParameterfv\0"
   "glGetMinmaxParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[30367]: VertexAttrib1fvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib1fv\0"
   "glVertexAttrib1fvARB\0"
   "\0"
   /* _mesa_function_pool[30410]: GenBuffersARB (will be remapped) */
   "ip\0"
   "glGenBuffers\0"
   "glGenBuffersARB\0"
   "\0"
   /* _mesa_function_pool[30443]: VertexAttribs1svNV (will be remapped) */
   "iip\0"
   "glVertexAttribs1svNV\0"
   "\0"
   /* _mesa_function_pool[30469]: Vertex3fv (offset 137) */
   "p\0"
   "glVertex3fv\0"
   "\0"
   /* _mesa_function_pool[30484]: GetTexBumpParameterivATI (will be remapped) */
   "ip\0"
   "glGetTexBumpParameterivATI\0"
   "\0"
   /* _mesa_function_pool[30515]: Binormal3bEXT (dynamic) */
   "iii\0"
   "glBinormal3bEXT\0"
   "\0"
   /* _mesa_function_pool[30536]: FragmentMaterialivSGIX (dynamic) */
   "iip\0"
   "glFragmentMaterialivSGIX\0"
   "\0"
   /* _mesa_function_pool[30566]: IsRenderbufferEXT (will be remapped) */
   "i\0"
   "glIsRenderbuffer\0"
   "glIsRenderbufferEXT\0"
   "\0"
   /* _mesa_function_pool[30606]: GenProgramsNV (will be remapped) */
   "ip\0"
   "glGenProgramsARB\0"
   "glGenProgramsNV\0"
   "\0"
   /* _mesa_function_pool[30643]: VertexAttrib4dvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib4dvNV\0"
   "\0"
   /* _mesa_function_pool[30667]: EndFragmentShaderATI (will be remapped) */
   "\0"
   "glEndFragmentShaderATI\0"
   "\0"
   /* _mesa_function_pool[30692]: Binormal3iEXT (dynamic) */
   "iii\0"
   "glBinormal3iEXT\0"
   "\0"
   /* _mesa_function_pool[30713]: WindowPos2fMESA (will be remapped) */
   "ff\0"
   "glWindowPos2f\0"
   "glWindowPos2fARB\0"
   "glWindowPos2fMESA\0"
   "\0"
   ;

/* these functions need to be remapped */
static const struct gl_function_pool_remap MESA_remap_table_functions[] = {
   {  1461, AttachShader_remap_index },
   {  8995, CreateProgram_remap_index },
   { 21153, CreateShader_remap_index },
   { 23466, DeleteProgram_remap_index },
   { 16937, DeleteShader_remap_index },
   { 21599, DetachShader_remap_index },
   { 16461, GetAttachedShaders_remap_index },
   {  4275, GetProgramInfoLog_remap_index },
   {   361, GetProgramiv_remap_index },
   {  5721, GetShaderInfoLog_remap_index },
   { 28619, GetShaderiv_remap_index },
   { 12198, IsProgram_remap_index },
   { 11197, IsShader_remap_index },
   {  9099, StencilFuncSeparate_remap_index },
   {  3487, StencilMaskSeparate_remap_index },
   {  6803, StencilOpSeparate_remap_index },
   { 20478, UniformMatrix2x3fv_remap_index },
   {  2615, UniformMatrix2x4fv_remap_index },
   { 30286, UniformMatrix3x2fv_remap_index },
   { 28009, UniformMatrix3x4fv_remap_index },
   { 14961, UniformMatrix4x2fv_remap_index },
   {  2937, UniformMatrix4x3fv_remap_index },
   { 14622, DrawArraysInstanced_remap_index },
   { 15725, DrawElementsInstanced_remap_index },
   {  9013, LoadTransposeMatrixdARB_remap_index },
   { 28348, LoadTransposeMatrixfARB_remap_index },
   {  4904, MultTransposeMatrixdARB_remap_index },
   { 21786, MultTransposeMatrixfARB_remap_index },
   {   172, SampleCoverageARB_remap_index },
   {  5117, CompressedTexImage1DARB_remap_index },
   { 22269, CompressedTexImage2DARB_remap_index },
   {  3550, CompressedTexImage3DARB_remap_index },
   { 16753, CompressedTexSubImage1DARB_remap_index },
   {  1880, CompressedTexSubImage2DARB_remap_index },
   { 18614, CompressedTexSubImage3DARB_remap_index },
   { 26291, GetCompressedTexImageARB_remap_index },
   {  3395, DisableVertexAttribArrayARB_remap_index },
   { 27621, EnableVertexAttribArrayARB_remap_index },
   { 29462, GetProgramEnvParameterdvARB_remap_index },
   { 21666, GetProgramEnvParameterfvARB_remap_index },
   { 25290, GetProgramLocalParameterdvARB_remap_index },
   {  7245, GetProgramLocalParameterfvARB_remap_index },
   { 16844, GetProgramStringARB_remap_index },
   { 25485, GetProgramivARB_remap_index },
   { 18809, GetVertexAttribdvARB_remap_index },
   { 14850, GetVertexAttribfvARB_remap_index },
   {  8908, GetVertexAttribivARB_remap_index },
   { 17690, ProgramEnvParameter4dARB_remap_index },
   { 23239, ProgramEnvParameter4dvARB_remap_index },
   { 15458, ProgramEnvParameter4fARB_remap_index },
   {  8108, ProgramEnvParameter4fvARB_remap_index },
   {  3513, ProgramLocalParameter4dARB_remap_index },
   { 11908, ProgramLocalParameter4dvARB_remap_index },
   { 27100, ProgramLocalParameter4fARB_remap_index },
   { 23819, ProgramLocalParameter4fvARB_remap_index },
   { 26071, ProgramStringARB_remap_index },
   { 17940, VertexAttrib1dARB_remap_index },
   { 14426, VertexAttrib1dvARB_remap_index },
   {  3688, VertexAttrib1fARB_remap_index },
   { 30367, VertexAttrib1fvARB_remap_index },
   {  6329, VertexAttrib1sARB_remap_index },
   {  2054, VertexAttrib1svARB_remap_index },
   { 13857, VertexAttrib2dARB_remap_index },
   { 16080, VertexAttrib2dvARB_remap_index },
   {  1480, VertexAttrib2fARB_remap_index },
   { 16193, VertexAttrib2fvARB_remap_index },
   { 29993, VertexAttrib2sARB_remap_index },
   { 29099, VertexAttrib2svARB_remap_index },
   { 10282, VertexAttrib3dARB_remap_index },
   {  7811, VertexAttrib3dvARB_remap_index },
   {  1567, VertexAttrib3fARB_remap_index },
   { 20741, VertexAttrib3fvARB_remap_index },
   { 25918, VertexAttrib3sARB_remap_index },
   { 18551, VertexAttrib3svARB_remap_index },
   {  4301, VertexAttrib4NbvARB_remap_index },
   { 16416, VertexAttrib4NivARB_remap_index },
   { 20696, VertexAttrib4NsvARB_remap_index },
   { 21618, VertexAttrib4NubARB_remap_index },
   { 29345, VertexAttrib4NubvARB_remap_index },
   { 17351, VertexAttrib4NuivARB_remap_index },
   {  2810, VertexAttrib4NusvARB_remap_index },
   {  9876, VertexAttrib4bvARB_remap_index },
   { 24698, VertexAttrib4dARB_remap_index },
   { 19573, VertexAttrib4dvARB_remap_index },
   { 10389, VertexAttrib4fARB_remap_index },
   { 10793, VertexAttrib4fvARB_remap_index },
   {  9292, VertexAttrib4ivARB_remap_index },
   { 15894, VertexAttrib4sARB_remap_index },
   { 28534, VertexAttrib4svARB_remap_index },
   { 15263, VertexAttrib4ubvARB_remap_index },
   { 27945, VertexAttrib4uivARB_remap_index },
   { 18362, VertexAttrib4usvARB_remap_index },
   { 20327, VertexAttribPointerARB_remap_index },
   { 30127, BindBufferARB_remap_index },
   {  6036, BufferDataARB_remap_index },
   {  1382, BufferSubDataARB_remap_index },
   { 28134, DeleteBuffersARB_remap_index },
   { 30410, GenBuffersARB_remap_index },
   { 16236, GetBufferParameterivARB_remap_index },
   { 15410, GetBufferPointervARB_remap_index },
   {  1335, GetBufferSubDataARB_remap_index },
   { 27893, IsBufferARB_remap_index },
   { 24303, MapBufferARB_remap_index },
   { 28749, UnmapBufferARB_remap_index },
   {   268, BeginQueryARB_remap_index },
   { 18035, DeleteQueriesARB_remap_index },
   { 11087, EndQueryARB_remap_index },
   { 26770, GenQueriesARB_remap_index },
   {  1772, GetQueryObjectivARB_remap_index },
   { 15938, GetQueryObjectuivARB_remap_index },
   {  1624, GetQueryivARB_remap_index },
   { 18269, IsQueryARB_remap_index },
   {  7421, AttachObjectARB_remap_index },
   { 16899, CompileShaderARB_remap_index },
   {  2879, CreateProgramObjectARB_remap_index },
   {  5981, CreateShaderObjectARB_remap_index },
   { 13274, DeleteObjectARB_remap_index },
   { 22060, DetachObjectARB_remap_index },
   { 10865, GetActiveUniformARB_remap_index },
   {  8583, GetAttachedObjectsARB_remap_index },
   {  8890, GetHandleARB_remap_index },
   { 30160, GetInfoLogARB_remap_index },
   { 29416, GetObjectParameterfvARB_remap_index },
   { 25164, GetObjectParameterivARB_remap_index },
   { 26528, GetShaderSourceARB_remap_index },
   { 25778, GetUniformLocationARB_remap_index },
   { 21888, GetUniformfvARB_remap_index },
   { 11530, GetUniformivARB_remap_index },
   { 18407, LinkProgramARB_remap_index },
   { 18465, ShaderSourceARB_remap_index },
   {  6703, Uniform1fARB_remap_index },
   { 27309, Uniform1fvARB_remap_index },
   { 20296, Uniform1iARB_remap_index },
   { 19262, Uniform1ivARB_remap_index },
   {  2003, Uniform2fARB_remap_index },
   { 13110, Uniform2fvARB_remap_index },
   { 24190, Uniform2iARB_remap_index },
   {  2123, Uniform2ivARB_remap_index },
   { 17009, Uniform3fARB_remap_index },
   {  8613, Uniform3fvARB_remap_index },
   {  5627, Uniform3iARB_remap_index },
   { 15516, Uniform3ivARB_remap_index },
   { 17496, Uniform4fARB_remap_index },
   { 21752, Uniform4fvARB_remap_index },
   { 22918, Uniform4iARB_remap_index },
   { 18775, Uniform4ivARB_remap_index },
   {  7473, UniformMatrix2fvARB_remap_index },
   {    17, UniformMatrix3fvARB_remap_index },
   {  2475, UniformMatrix4fvARB_remap_index },
   { 23351, UseProgramObjectARB_remap_index },
   { 13545, ValidateProgramARB_remap_index },
   { 19616, BindAttribLocationARB_remap_index },
   {  4346, GetActiveAttribARB_remap_index },
   { 15197, GetAttribLocationARB_remap_index },
   { 27048, DrawBuffersARB_remap_index },
   { 12013, RenderbufferStorageMultisample_remap_index },
   { 12417, FramebufferTextureARB_remap_index },
   { 23721, FramebufferTextureFaceARB_remap_index },
   { 22209, ProgramParameteriARB_remap_index },
   { 17544, FlushMappedBufferRange_remap_index },
   { 25581, MapBufferRange_remap_index },
   { 15072, BindVertexArray_remap_index },
   { 13404, GenVertexArrays_remap_index },
   { 27823, CopyBufferSubData_remap_index },
   { 28638, ClientWaitSync_remap_index },
   {  2394, DeleteSync_remap_index },
   {  6370, FenceSync_remap_index },
   { 13916, GetInteger64v_remap_index },
   { 20803, GetSynciv_remap_index },
   { 26987, IsSync_remap_index },
   {  8531, WaitSync_remap_index },
   {  3363, DrawElementsBaseVertex_remap_index },
   { 28066, DrawRangeElementsBaseVertex_remap_index },
   { 24334, MultiDrawElementsBaseVertex_remap_index },
   {  4480, BindTransformFeedback_remap_index },
   {  2906, DeleteTransformFeedbacks_remap_index },
   {  5660, DrawTransformFeedback_remap_index },
   {  8750, GenTransformFeedbacks_remap_index },
   { 25961, IsTransformFeedback_remap_index },
   { 23914, PauseTransformFeedback_remap_index },
   {  4824, ResumeTransformFeedback_remap_index },
   {  4739, PolygonOffsetEXT_remap_index },
   { 21388, GetPixelTexGenParameterfvSGIS_remap_index },
   {  3895, GetPixelTexGenParameterivSGIS_remap_index },
   { 21121, PixelTexGenParameterfSGIS_remap_index },
   {   580, PixelTexGenParameterfvSGIS_remap_index },
   { 11568, PixelTexGenParameteriSGIS_remap_index },
   { 12591, PixelTexGenParameterivSGIS_remap_index },
   { 15160, SampleMaskSGIS_remap_index },
   { 18209, SamplePatternSGIS_remap_index },
   { 24263, ColorPointerEXT_remap_index },
   { 16123, EdgeFlagPointerEXT_remap_index },
   {  5281, IndexPointerEXT_remap_index },
   {  5361, NormalPointerEXT_remap_index },
   { 14510, TexCoordPointerEXT_remap_index },
   {  6159, VertexPointerEXT_remap_index },
   {  3165, PointParameterfEXT_remap_index },
   {  7010, PointParameterfvEXT_remap_index },
   { 29514, LockArraysEXT_remap_index },
   { 13609, UnlockArraysEXT_remap_index },
   {  8017, CullParameterdvEXT_remap_index },
   { 10660, CullParameterfvEXT_remap_index },
   {  1151, SecondaryColor3bEXT_remap_index },
   {  7169, SecondaryColor3bvEXT_remap_index },
   {  9469, SecondaryColor3dEXT_remap_index },
   { 23524, SecondaryColor3dvEXT_remap_index },
   { 25827, SecondaryColor3fEXT_remap_index },
   { 16689, SecondaryColor3fvEXT_remap_index },
   {   426, SecondaryColor3iEXT_remap_index },
   { 14898, SecondaryColor3ivEXT_remap_index },
   {  9127, SecondaryColor3sEXT_remap_index },
   { 28302, SecondaryColor3svEXT_remap_index },
   { 25000, SecondaryColor3ubEXT_remap_index },
   { 19507, SecondaryColor3ubvEXT_remap_index },
   { 11763, SecondaryColor3uiEXT_remap_index },
   { 21008, SecondaryColor3uivEXT_remap_index },
   { 23771, SecondaryColor3usEXT_remap_index },
   { 11836, SecondaryColor3usvEXT_remap_index },
   { 10736, SecondaryColorPointerEXT_remap_index },
   { 23585, MultiDrawArraysEXT_remap_index },
   { 19197, MultiDrawElementsEXT_remap_index },
   { 19392, FogCoordPointerEXT_remap_index },
   {  4044, FogCoorddEXT_remap_index },
   { 28915, FogCoorddvEXT_remap_index },
   {  4136, FogCoordfEXT_remap_index },
   { 24923, FogCoordfvEXT_remap_index },
   { 17448, PixelTexGenSGIX_remap_index },
   { 25508, BlendFuncSeparateEXT_remap_index },
   {  6071, FlushVertexArrayRangeNV_remap_index },
   {  4688, VertexArrayRangeNV_remap_index },
   { 25892, CombinerInputNV_remap_index },
   {  1946, CombinerOutputNV_remap_index },
   { 28455, CombinerParameterfNV_remap_index },
   {  4608, CombinerParameterfvNV_remap_index },
   { 20527, CombinerParameteriNV_remap_index },
   { 29885, CombinerParameterivNV_remap_index },
   {  6447, FinalCombinerInputNV_remap_index },
   {  8956, GetCombinerInputParameterfvNV_remap_index },
   { 29722, GetCombinerInputParameterivNV_remap_index },
   { 12692, GetCombinerOutputParameterfvNV_remap_index },
   { 12520, GetCombinerOutputParameterivNV_remap_index },
   {  5816, GetFinalCombinerInputParameterfvNV_remap_index },
   { 22790, GetFinalCombinerInputParameterivNV_remap_index },
   { 11508, ResizeBuffersMESA_remap_index },
   { 10109, WindowPos2dMESA_remap_index },
   {   944, WindowPos2dvMESA_remap_index },
   { 30713, WindowPos2fMESA_remap_index },
   {  7114, WindowPos2fvMESA_remap_index },
   { 16636, WindowPos2iMESA_remap_index },
   { 18682, WindowPos2ivMESA_remap_index },
   { 19296, WindowPos2sMESA_remap_index },
   {  5031, WindowPos2svMESA_remap_index },
   {  6939, WindowPos3dMESA_remap_index },
   { 12838, WindowPos3dvMESA_remap_index },
   {   472, WindowPos3fMESA_remap_index },
   { 13670, WindowPos3fvMESA_remap_index },
   { 22102, WindowPos3iMESA_remap_index },
   { 27768, WindowPos3ivMESA_remap_index },
   { 17154, WindowPos3sMESA_remap_index },
   { 29171, WindowPos3svMESA_remap_index },
   { 10060, WindowPos4dMESA_remap_index },
   { 15601, WindowPos4dvMESA_remap_index },
   { 12797, WindowPos4fMESA_remap_index },
   { 28209, WindowPos4fvMESA_remap_index },
   { 27921, WindowPos4iMESA_remap_index },
   { 11311, WindowPos4ivMESA_remap_index },
   { 17327, WindowPos4sMESA_remap_index },
   {  2857, WindowPos4svMESA_remap_index },
   { 12559, MultiModeDrawArraysIBM_remap_index },
   { 26641, MultiModeDrawElementsIBM_remap_index },
   { 11115, DeleteFencesNV_remap_index },
   { 25739, FinishFenceNV_remap_index },
   {  3287, GenFencesNV_remap_index },
   { 15581, GetFenceivNV_remap_index },
   {  7406, IsFenceNV_remap_index },
   { 12447, SetFenceNV_remap_index },
   {  3744, TestFenceNV_remap_index },
   { 29142, AreProgramsResidentNV_remap_index },
   { 28497, BindProgramNV_remap_index },
   { 23854, DeleteProgramsNV_remap_index },
   { 19725, ExecuteProgramNV_remap_index },
   { 30606, GenProgramsNV_remap_index },
   { 21467, GetProgramParameterdvNV_remap_index },
   {  9531, GetProgramParameterfvNV_remap_index },
   { 24237, GetProgramStringNV_remap_index },
   { 22479, GetProgramivNV_remap_index },
   { 21701, GetTrackMatrixivNV_remap_index },
   { 24031, GetVertexAttribPointervNV_remap_index },
   { 22723, GetVertexAttribdvNV_remap_index },
   {  8426, GetVertexAttribfvNV_remap_index },
   { 16817, GetVertexAttribivNV_remap_index },
   { 17574, IsProgramNV_remap_index },
   {  8509, LoadProgramNV_remap_index },
   { 25604, ProgramParameters4dvNV_remap_index },
   { 22409, ProgramParameters4fvNV_remap_index },
   { 18986, RequestResidentProgramsNV_remap_index },
   { 20505, TrackMatrixNV_remap_index },
   { 29699, VertexAttrib1dNV_remap_index },
   { 12358, VertexAttrib1dvNV_remap_index },
   { 26173, VertexAttrib1fNV_remap_index },
   {  2245, VertexAttrib1fvNV_remap_index },
   { 28266, VertexAttrib1sNV_remap_index },
   { 13743, VertexAttrib1svNV_remap_index },
   {  4251, VertexAttrib2dNV_remap_index },
   { 12273, VertexAttrib2dvNV_remap_index },
   { 18441, VertexAttrib2fNV_remap_index },
   { 11884, VertexAttrib2fvNV_remap_index },
   {  5191, VertexAttrib2sNV_remap_index },
   { 17208, VertexAttrib2svNV_remap_index },
   { 10257, VertexAttrib3dNV_remap_index },
   { 29392, VertexAttrib3dvNV_remap_index },
   {  9343, VertexAttrib3fNV_remap_index },
   { 22750, VertexAttrib3fvNV_remap_index },
   { 20382, VertexAttrib3sNV_remap_index },
   { 21728, VertexAttrib3svNV_remap_index },
   { 26615, VertexAttrib4dNV_remap_index },
   { 30643, VertexAttrib4dvNV_remap_index },
   {  3945, VertexAttrib4fNV_remap_index },
   {  8559, VertexAttrib4fvNV_remap_index },
   { 24582, VertexAttrib4sNV_remap_index },
   {  1293, VertexAttrib4svNV_remap_index },
   {  4409, VertexAttrib4ubNV_remap_index },
   {   734, VertexAttrib4ubvNV_remap_index },
   { 19905, VertexAttribPointerNV_remap_index },
   {  2097, VertexAttribs1dvNV_remap_index },
   { 24119, VertexAttribs1fvNV_remap_index },
   { 30443, VertexAttribs1svNV_remap_index },
   {  9368, VertexAttribs2dvNV_remap_index },
   { 23312, VertexAttribs2fvNV_remap_index },
   { 16149, VertexAttribs2svNV_remap_index },
   {  4636, VertexAttribs3dvNV_remap_index },
   {  1977, VertexAttribs3fvNV_remap_index },
   { 27516, VertexAttribs3svNV_remap_index },
   { 24672, VertexAttribs4dvNV_remap_index },
   {  4662, VertexAttribs4fvNV_remap_index },
   { 30230, VertexAttribs4svNV_remap_index },
   { 27264, VertexAttribs4ubvNV_remap_index },
   { 24742, GetTexBumpParameterfvATI_remap_index },
   { 30484, GetTexBumpParameterivATI_remap_index },
   { 16871, TexBumpParameterfvATI_remap_index },
   { 18857, TexBumpParameterivATI_remap_index },
   { 14289, AlphaFragmentOp1ATI_remap_index },
   {  9919, AlphaFragmentOp2ATI_remap_index },
   { 22666, AlphaFragmentOp3ATI_remap_index },
   { 27443, BeginFragmentShaderATI_remap_index },
   { 28696, BindFragmentShaderATI_remap_index },
   { 21857, ColorFragmentOp1ATI_remap_index },
   {  3823, ColorFragmentOp2ATI_remap_index },
   { 29037, ColorFragmentOp3ATI_remap_index },
   {  4781, DeleteFragmentShaderATI_remap_index },
   { 30667, EndFragmentShaderATI_remap_index },
   { 29913, GenFragmentShadersATI_remap_index },
   { 23443, PassTexCoordATI_remap_index },
   {  6139, SampleMapATI_remap_index },
   {  5912, SetFragmentShaderConstantATI_remap_index },
   {   319, PointParameteriNV_remap_index },
   { 12999, PointParameterivNV_remap_index },
   { 26454, ActiveStencilFaceEXT_remap_index },
   { 25264, BindVertexArrayAPPLE_remap_index },
   {  2522, DeleteVertexArraysAPPLE_remap_index },
   { 16488, GenVertexArraysAPPLE_remap_index },
   { 21532, IsVertexArrayAPPLE_remap_index },
   {   775, GetProgramNamedParameterdvNV_remap_index },
   {  3128, GetProgramNamedParameterfvNV_remap_index },
   { 24773, ProgramNamedParameter4dNV_remap_index },
   { 13325, ProgramNamedParameter4dvNV_remap_index },
   {  8042, ProgramNamedParameter4fNV_remap_index },
   { 10701, ProgramNamedParameter4fvNV_remap_index },
   { 22388, DepthBoundsEXT_remap_index },
   {  1043, BlendEquationSeparateEXT_remap_index },
   { 13444, BindFramebufferEXT_remap_index },
   { 23630, BindRenderbufferEXT_remap_index },
   {  8806, CheckFramebufferStatusEXT_remap_index },
   { 20822, DeleteFramebuffersEXT_remap_index },
   { 29294, DeleteRenderbuffersEXT_remap_index },
   { 12297, FramebufferRenderbufferEXT_remap_index },
   { 12464, FramebufferTexture1DEXT_remap_index },
   { 10495, FramebufferTexture2DEXT_remap_index },
   { 10162, FramebufferTexture3DEXT_remap_index },
   { 21424, GenFramebuffersEXT_remap_index },
   { 16035, GenRenderbuffersEXT_remap_index },
   {  5858, GenerateMipmapEXT_remap_index },
   { 20002, GetFramebufferAttachmentParameterivEXT_remap_index },
   { 29819, GetRenderbufferParameterivEXT_remap_index },
   { 18737, IsFramebufferEXT_remap_index },
   { 30566, IsRenderbufferEXT_remap_index },
   {  7353, RenderbufferStorageEXT_remap_index },
   {   651, BlitFramebufferEXT_remap_index },
   { 13144, BufferParameteriAPPLE_remap_index },
   { 17606, FlushMappedBufferRangeAPPLE_remap_index },
   {  2701, FramebufferTextureLayerEXT_remap_index },
   {  4956, ColorMaskIndexedEXT_remap_index },
   { 17232, DisableIndexedEXT_remap_index },
   { 24429, EnableIndexedEXT_remap_index },
   { 19973, GetBooleanIndexedvEXT_remap_index },
   {  9952, GetIntegerIndexedvEXT_remap_index },
   { 20898, IsEnabledIndexedEXT_remap_index },
   {  4074, BeginConditionalRenderNV_remap_index },
   { 23416, EndConditionalRenderNV_remap_index },
   {  8453, BeginTransformFeedbackEXT_remap_index },
   { 17256, BindBufferBaseEXT_remap_index },
   { 17126, BindBufferOffsetEXT_remap_index },
   { 11136, BindBufferRangeEXT_remap_index },
   { 13059, EndTransformFeedbackEXT_remap_index },
   {  9804, GetTransformFeedbackVaryingEXT_remap_index },
   { 19042, TransformFeedbackVaryingsEXT_remap_index },
   { 27165, ProvokingVertexEXT_remap_index },
   {  9752, GetTexParameterPointervAPPLE_remap_index },
   {  4436, TextureRangeAPPLE_remap_index },
   { 10567, GetObjectParameterivAPPLE_remap_index },
   { 18181, ObjectPurgeableAPPLE_remap_index },
   {  4985, ObjectUnpurgeableAPPLE_remap_index },
   { 26480, StencilFuncSeparateATI_remap_index },
   { 16555, ProgramEnvParameters4fvEXT_remap_index },
   { 19936, ProgramLocalParameters4fvEXT_remap_index },
   { 12927, GetQueryObjecti64vEXT_remap_index },
   {  9394, GetQueryObjectui64vEXT_remap_index },
   { 21926, EGLImageTargetRenderbufferStorageOES_remap_index },
   { 11054, EGLImageTargetTexture2DOES_remap_index },
   {    -1, -1 }
};

/* these functions are in the ABI, but have alternative names */
static const struct gl_function_remap MESA_alt_functions[] = {
   /* from GL_EXT_blend_color */
   {  2440, _gloffset_BlendColor },
   /* from GL_EXT_blend_minmax */
   { 10219, _gloffset_BlendEquation },
   /* from GL_EXT_color_subtable */
   { 15623, _gloffset_ColorSubTable },
   { 29226, _gloffset_CopyColorSubTable },
   /* from GL_EXT_convolution */
   {   213, _gloffset_ConvolutionFilter1D },
   {  2284, _gloffset_CopyConvolutionFilter1D },
   {  3624, _gloffset_GetConvolutionParameteriv },
   {  7702, _gloffset_ConvolutionFilter2D },
   {  7868, _gloffset_ConvolutionParameteriv },
   {  8328, _gloffset_ConvolutionParameterfv },
   { 18885, _gloffset_GetSeparableFilter },
   { 22156, _gloffset_SeparableFilter2D },
   { 22968, _gloffset_ConvolutionParameteri },
   { 23091, _gloffset_ConvolutionParameterf },
   { 24608, _gloffset_GetConvolutionParameterfv },
   { 25430, _gloffset_GetConvolutionFilter },
   { 27705, _gloffset_CopyConvolutionFilter2D },
   /* from GL_EXT_copy_texture */
   { 13803, _gloffset_CopyTexSubImage3D },
   { 15363, _gloffset_CopyTexImage2D },
   { 22576, _gloffset_CopyTexImage1D },
   { 25111, _gloffset_CopyTexSubImage2D },
   { 27343, _gloffset_CopyTexSubImage1D },
   /* from GL_EXT_draw_range_elements */
   {  8665, _gloffset_DrawRangeElements },
   /* from GL_EXT_histogram */
   {   812, _gloffset_Histogram },
   {  3088, _gloffset_ResetHistogram },
   {  9065, _gloffset_GetMinmax },
   { 14137, _gloffset_GetHistogramParameterfv },
   { 22501, _gloffset_GetMinmaxParameteriv },
   { 24498, _gloffset_ResetMinmax },
   { 25327, _gloffset_GetHistogramParameteriv },
   { 26414, _gloffset_GetHistogram },
   { 28812, _gloffset_Minmax },
   { 30313, _gloffset_GetMinmaxParameterfv },
   /* from GL_EXT_paletted_texture */
   {  7564, _gloffset_ColorTable },
   { 13983, _gloffset_GetColorTable },
   { 21171, _gloffset_GetColorTableParameterfv },
   { 23147, _gloffset_GetColorTableParameteriv },
   /* from GL_EXT_subtexture */
   {  6285, _gloffset_TexSubImage1D },
   {  9679, _gloffset_TexSubImage2D },
   /* from GL_EXT_texture3D */
   {  1658, _gloffset_TexImage3D },
   { 20940, _gloffset_TexSubImage3D },
   /* from GL_EXT_texture_object */
   {  2964, _gloffset_PrioritizeTextures },
   {  6734, _gloffset_AreTexturesResident },
   { 12382, _gloffset_GenTextures },
   { 14469, _gloffset_DeleteTextures },
   { 17887, _gloffset_IsTexture },
   { 27408, _gloffset_BindTexture },
   /* from GL_EXT_vertex_array */
   { 22328, _gloffset_ArrayElement },
   { 28400, _gloffset_GetPointerv },
   { 29940, _gloffset_DrawArrays },
   /* from GL_SGI_color_table */
   {  6852, _gloffset_ColorTableParameteriv },
   {  7564, _gloffset_ColorTable },
   { 13983, _gloffset_GetColorTable },
   { 14093, _gloffset_CopyColorTable },
   { 17748, _gloffset_ColorTableParameterfv },
   { 21171, _gloffset_GetColorTableParameterfv },
   { 23147, _gloffset_GetColorTableParameteriv },
   /* from GL_VERSION_1_3 */
   {   381, _gloffset_MultiTexCoord3sARB },
   {   613, _gloffset_ActiveTextureARB },
   {  3761, _gloffset_MultiTexCoord1fvARB },
   {  5386, _gloffset_MultiTexCoord3dARB },
   {  5431, _gloffset_MultiTexCoord2iARB },
   {  5555, _gloffset_MultiTexCoord2svARB },
   {  7520, _gloffset_MultiTexCoord2fARB },
   {  9424, _gloffset_MultiTexCoord3fvARB },
   {  9981, _gloffset_MultiTexCoord4sARB },
   { 10615, _gloffset_MultiTexCoord2dvARB },
   { 10997, _gloffset_MultiTexCoord1svARB },
   { 11369, _gloffset_MultiTexCoord3svARB },
   { 11430, _gloffset_MultiTexCoord4iARB },
   { 12153, _gloffset_MultiTexCoord3iARB },
   { 12956, _gloffset_MultiTexCoord1dARB },
   { 13173, _gloffset_MultiTexCoord3dvARB },
   { 14337, _gloffset_MultiTexCoord3ivARB },
   { 14382, _gloffset_MultiTexCoord2sARB },
   { 15680, _gloffset_MultiTexCoord4ivARB },
   { 17398, _gloffset_ClientActiveTextureARB },
   { 19681, _gloffset_MultiTexCoord2dARB },
   { 20122, _gloffset_MultiTexCoord4dvARB },
   { 20433, _gloffset_MultiTexCoord4fvARB },
   { 21312, _gloffset_MultiTexCoord3fARB },
   { 23675, _gloffset_MultiTexCoord4dARB },
   { 23941, _gloffset_MultiTexCoord1sARB },
   { 24145, _gloffset_MultiTexCoord1dvARB },
   { 24955, _gloffset_MultiTexCoord1ivARB },
   { 25048, _gloffset_MultiTexCoord2ivARB },
   { 25387, _gloffset_MultiTexCoord1iARB },
   { 26689, _gloffset_MultiTexCoord4svARB },
   { 27207, _gloffset_MultiTexCoord1fARB },
   { 27470, _gloffset_MultiTexCoord4fARB },
   { 29774, _gloffset_MultiTexCoord2fvARB },
   {    -1, -1 }
};

#endif /* need_MESA_remap_table */

#if defined(need_GL_3DFX_tbuffer)
static const struct gl_function_remap GL_3DFX_tbuffer_functions[] = {
   {  8386, -1 }, /* TbufferMask3DFX */
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
   { 11333, -1 }, /* FramebufferTextureLayer */
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
   {  3339, -1 }, /* MatrixIndexusvARB */
   { 11974, -1 }, /* MatrixIndexuivARB */
   { 13295, -1 }, /* MatrixIndexPointerARB */
   { 18136, -1 }, /* CurrentPaletteMatrixARB */
   { 21056, -1 }, /* MatrixIndexubvARB */
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
   {  5746, -1 }, /* WeightivARB */
   { 10084, -1 }, /* WeightPointerARB */
   { 12674, -1 }, /* WeightfvARB */
   { 16175, -1 }, /* WeightbvARB */
   { 19349, -1 }, /* WeightusvARB */
   { 22082, -1 }, /* VertexBlendARB */
   { 27291, -1 }, /* WeightsvARB */
   { 29276, -1 }, /* WeightdvARB */
   { 29974, -1 }, /* WeightuivARB */
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
   { 10219, _gloffset_BlendEquation },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_color_subtable)
static const struct gl_function_remap GL_EXT_color_subtable_functions[] = {
   { 15623, _gloffset_ColorSubTable },
   { 29226, _gloffset_CopyColorSubTable },
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
   {  3624, _gloffset_GetConvolutionParameteriv },
   {  7702, _gloffset_ConvolutionFilter2D },
   {  7868, _gloffset_ConvolutionParameteriv },
   {  8328, _gloffset_ConvolutionParameterfv },
   { 18885, _gloffset_GetSeparableFilter },
   { 22156, _gloffset_SeparableFilter2D },
   { 22968, _gloffset_ConvolutionParameteri },
   { 23091, _gloffset_ConvolutionParameterf },
   { 24608, _gloffset_GetConvolutionParameterfv },
   { 25430, _gloffset_GetConvolutionFilter },
   { 27705, _gloffset_CopyConvolutionFilter2D },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const struct gl_function_remap GL_EXT_coordinate_frame_functions[] = {
   {  9563, -1 }, /* TangentPointerEXT */
   { 11488, -1 }, /* Binormal3ivEXT */
   { 12106, -1 }, /* Tangent3sEXT */
   { 13360, -1 }, /* Tangent3fvEXT */
   { 17107, -1 }, /* Tangent3dvEXT */
   { 17834, -1 }, /* Binormal3bvEXT */
   { 18938, -1 }, /* Binormal3dEXT */
   { 20988, -1 }, /* Tangent3fEXT */
   { 23040, -1 }, /* Binormal3sEXT */
   { 23485, -1 }, /* Tangent3ivEXT */
   { 23504, -1 }, /* Tangent3dEXT */
   { 24372, -1 }, /* Binormal3svEXT */
   { 24853, -1 }, /* Binormal3fEXT */
   { 25705, -1 }, /* Binormal3dvEXT */
   { 26911, -1 }, /* Tangent3iEXT */
   { 27990, -1 }, /* Tangent3bvEXT */
   { 28435, -1 }, /* Tangent3bEXT */
   { 28999, -1 }, /* Binormal3fvEXT */
   { 29673, -1 }, /* BinormalPointerEXT */
   { 30078, -1 }, /* Tangent3svEXT */
   { 30515, -1 }, /* Binormal3bEXT */
   { 30692, -1 }, /* Binormal3iEXT */
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_copy_texture)
static const struct gl_function_remap GL_EXT_copy_texture_functions[] = {
   { 13803, _gloffset_CopyTexSubImage3D },
   { 15363, _gloffset_CopyTexImage2D },
   { 22576, _gloffset_CopyTexImage1D },
   { 25111, _gloffset_CopyTexSubImage2D },
   { 27343, _gloffset_CopyTexSubImage1D },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_cull_vertex)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_cull_vertex_functions[] = {
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
   {  8665, _gloffset_DrawRangeElements },
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
   {  3088, _gloffset_ResetHistogram },
   {  9065, _gloffset_GetMinmax },
   { 14137, _gloffset_GetHistogramParameterfv },
   { 22501, _gloffset_GetMinmaxParameteriv },
   { 24498, _gloffset_ResetMinmax },
   { 25327, _gloffset_GetHistogramParameteriv },
   { 26414, _gloffset_GetHistogram },
   { 28812, _gloffset_Minmax },
   { 30313, _gloffset_GetMinmaxParameterfv },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_index_func)
static const struct gl_function_remap GL_EXT_index_func_functions[] = {
   { 10446, -1 }, /* IndexFuncEXT */
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_index_material)
static const struct gl_function_remap GL_EXT_index_material_functions[] = {
   { 19436, -1 }, /* IndexMaterialEXT */
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_light_texture)
static const struct gl_function_remap GL_EXT_light_texture_functions[] = {
   { 24392, -1 }, /* ApplyTextureEXT */
   { 24452, -1 }, /* TextureMaterialEXT */
   { 24477, -1 }, /* TextureLightEXT */
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
   {  7564, _gloffset_ColorTable },
   { 13983, _gloffset_GetColorTable },
   { 21171, _gloffset_GetColorTableParameterfv },
   { 23147, _gloffset_GetColorTableParameteriv },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_pixel_transform)
static const struct gl_function_remap GL_EXT_pixel_transform_functions[] = {
   { 20087, -1 }, /* PixelTransformParameterfEXT */
   { 20167, -1 }, /* PixelTransformParameteriEXT */
   { 28173, -1 }, /* PixelTransformParameterfvEXT */
   { 29637, -1 }, /* PixelTransformParameterivEXT */
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
   {  6285, _gloffset_TexSubImage1D },
   {  9679, _gloffset_TexSubImage2D },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_texture3D)
static const struct gl_function_remap GL_EXT_texture3D_functions[] = {
   {  1658, _gloffset_TexImage3D },
   { 20940, _gloffset_TexSubImage3D },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_texture_array)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_EXT_texture_array_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_texture_object)
static const struct gl_function_remap GL_EXT_texture_object_functions[] = {
   {  2964, _gloffset_PrioritizeTextures },
   {  6734, _gloffset_AreTexturesResident },
   { 12382, _gloffset_GenTextures },
   { 14469, _gloffset_DeleteTextures },
   { 17887, _gloffset_IsTexture },
   { 27408, _gloffset_BindTexture },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_texture_perturb_normal)
static const struct gl_function_remap GL_EXT_texture_perturb_normal_functions[] = {
   { 12624, -1 }, /* TextureNormalEXT */
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
   { 22328, _gloffset_ArrayElement },
   { 28400, _gloffset_GetPointerv },
   { 29940, _gloffset_DrawArrays },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_vertex_weighting)
static const struct gl_function_remap GL_EXT_vertex_weighting_functions[] = {
   { 17917, -1 }, /* VertexWeightfvEXT */
   { 24831, -1 }, /* VertexWeightfEXT */
   { 26383, -1 }, /* VertexWeightPointerEXT */
   {    -1, -1 }
};
#endif

#if defined(need_GL_HP_image_transform)
static const struct gl_function_remap GL_HP_image_transform_functions[] = {
   {  2157, -1 }, /* GetImageTransformParameterfvHP */
   {  3305, -1 }, /* ImageTransformParameterfHP */
   {  9257, -1 }, /* ImageTransformParameterfvHP */
   { 10915, -1 }, /* ImageTransformParameteriHP */
   { 11223, -1 }, /* GetImageTransformParameterivHP */
   { 17981, -1 }, /* ImageTransformParameterivHP */
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
   {  3857, -1 }, /* SecondaryColorPointerListIBM */
   {  5252, -1 }, /* NormalPointerListIBM */
   {  6908, -1 }, /* FogCoordPointerListIBM */
   {  7215, -1 }, /* VertexPointerListIBM */
   { 10836, -1 }, /* ColorPointerListIBM */
   { 12213, -1 }, /* TexCoordPointerListIBM */
   { 12646, -1 }, /* IndexPointerListIBM */
   { 30256, -1 }, /* EdgeFlagPointerListIBM */
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
   { 11600, -1 }, /* VertexPointervINTEL */
   { 14230, -1 }, /* ColorPointervINTEL */
   { 27679, -1 }, /* NormalPointervINTEL */
   { 28105, -1 }, /* TexCoordPointervINTEL */
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
   {  3063, -1 }, /* ClearDebugLogMESA */
   {  4018, -1 }, /* GetDebugLogMESA */
   { 28593, -1 }, /* CreateDebugObjectMESA */
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
   {  5947, -1 }, /* GetMapAttribParameterivNV */
   {  7670, -1 }, /* MapControlPointsNV */
   {  7769, -1 }, /* MapParameterfvNV */
   {  9662, -1 }, /* EvalMapsNV */
   { 15845, -1 }, /* GetMapAttribParameterfvNV */
   { 16011, -1 }, /* MapParameterivNV */
   { 22891, -1 }, /* GetMapParameterivNV */
   { 23389, -1 }, /* GetMapParameterfvNV */
   { 27015, -1 }, /* GetMapControlPointsNV */
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

#if defined(need_GL_NV_register_combiners)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_NV_register_combiners_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_NV_register_combiners2)
static const struct gl_function_remap GL_NV_register_combiners2_functions[] = {
   { 14700, -1 }, /* CombinerStageParameterfvNV */
   { 15015, -1 }, /* GetCombinerStageParameterfvNV */
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
   {  7854, -1 }, /* HintPGI */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_detail_texture)
static const struct gl_function_remap GL_SGIS_detail_texture_functions[] = {
   { 14988, -1 }, /* GetDetailTexFuncSGIS */
   { 15308, -1 }, /* DetailTexFuncSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_fog_function)
static const struct gl_function_remap GL_SGIS_fog_function_functions[] = {
   { 25093, -1 }, /* FogFuncSGIS */
   { 25758, -1 }, /* GetFogFuncSGIS */
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
   {  6008, -1 }, /* GetSharpenTexFuncSGIS */
   { 20407, -1 }, /* SharpenTexFuncSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_texture4D)
static const struct gl_function_remap GL_SGIS_texture4D_functions[] = {
   {   894, -1 }, /* TexImage4DSGIS */
   { 14538, -1 }, /* TexSubImage4DSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_texture_color_mask)
static const struct gl_function_remap GL_SGIS_texture_color_mask_functions[] = {
   { 13936, -1 }, /* TextureColorMaskSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_texture_filter4)
static const struct gl_function_remap GL_SGIS_texture_filter4_functions[] = {
   {  6185, -1 }, /* GetTexFilterFuncSGIS */
   { 15134, -1 }, /* TexFilterFuncSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_async)
static const struct gl_function_remap GL_SGIX_async_functions[] = {
   {  3014, -1 }, /* AsyncMarkerSGIX */
   {  3997, -1 }, /* FinishAsyncSGIX */
   {  4762, -1 }, /* PollAsyncSGIX */
   { 20554, -1 }, /* DeleteAsyncMarkersSGIX */
   { 20609, -1 }, /* IsAsyncMarkerSGIX */
   { 30053, -1 }, /* GenAsyncMarkersSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_flush_raster)
static const struct gl_function_remap GL_SGIX_flush_raster_functions[] = {
   {  6562, -1 }, /* FlushRasterSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_fragment_lighting)
static const struct gl_function_remap GL_SGIX_fragment_lighting_functions[] = {
   {  2410, -1 }, /* FragmentMaterialfvSGIX */
   {  4713, -1 }, /* FragmentLightiSGIX */
   {  5688, -1 }, /* GetFragmentMaterialfvSGIX */
   {  7282, -1 }, /* FragmentMaterialfSGIX */
   {  7443, -1 }, /* GetFragmentLightivSGIX */
   {  8280, -1 }, /* FragmentLightModeliSGIX */
   {  9725, -1 }, /* FragmentLightivSGIX */
   { 10027, -1 }, /* GetFragmentMaterialivSGIX */
   { 17804, -1 }, /* FragmentLightModelfSGIX */
   { 18104, -1 }, /* FragmentColorMaterialSGIX */
   { 18504, -1 }, /* FragmentMaterialiSGIX */
   { 19764, -1 }, /* LightEnviSGIX */
   { 21263, -1 }, /* FragmentLightModelfvSGIX */
   { 21572, -1 }, /* FragmentLightfvSGIX */
   { 26142, -1 }, /* FragmentLightModelivSGIX */
   { 26265, -1 }, /* FragmentLightfSGIX */
   { 28969, -1 }, /* GetFragmentLightfvSGIX */
   { 30536, -1 }, /* FragmentMaterialivSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_framezoom)
static const struct gl_function_remap GL_SGIX_framezoom_functions[] = {
   { 20632, -1 }, /* FrameZoomSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_igloo_interface)
static const struct gl_function_remap GL_SGIX_igloo_interface_functions[] = {
   { 26573, -1 }, /* IglooInterfaceSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_instruments)
static const struct gl_function_remap GL_SGIX_instruments_functions[] = {
   {  2573, -1 }, /* ReadInstrumentsSGIX */
   {  5764, -1 }, /* PollInstrumentsSGIX */
   {  9623, -1 }, /* GetInstrumentsSGIX */
   { 11811, -1 }, /* StartInstrumentsSGIX */
   { 14734, -1 }, /* StopInstrumentsSGIX */
   { 16388, -1 }, /* InstrumentsBufferSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_list_priority)
static const struct gl_function_remap GL_SGIX_list_priority_functions[] = {
   {  1125, -1 }, /* ListParameterfSGIX */
   {  2763, -1 }, /* GetListParameterfvSGIX */
   { 16303, -1 }, /* ListParameteriSGIX */
   { 17057, -1 }, /* ListParameterfvSGIX */
   { 19170, -1 }, /* ListParameterivSGIX */
   { 30097, -1 }, /* GetListParameterivSGIX */
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
   {  3251, -1 }, /* LoadIdentityDeformationMapSGIX */
   { 14834, -1 }, /* DeformSGIX */
   { 22440, -1 }, /* DeformationMap3fSGIX */
   { 28857, -1 }, /* DeformationMap3dSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_reference_plane)
static const struct gl_function_remap GL_SGIX_reference_plane_functions[] = {
   { 13487, -1 }, /* ReferencePlaneSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_sprite)
static const struct gl_function_remap GL_SGIX_sprite_functions[] = {
   {  8778, -1 }, /* SpriteParameterfvSGIX */
   { 18959, -1 }, /* SpriteParameteriSGIX */
   { 24532, -1 }, /* SpriteParameterfSGIX */
   { 27137, -1 }, /* SpriteParameterivSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_tag_sample_buffer)
static const struct gl_function_remap GL_SGIX_tag_sample_buffer_functions[] = {
   { 19018, -1 }, /* TagSampleBufferSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGI_color_table)
static const struct gl_function_remap GL_SGI_color_table_functions[] = {
   {  6852, _gloffset_ColorTableParameteriv },
   {  7564, _gloffset_ColorTable },
   { 13983, _gloffset_GetColorTable },
   { 14093, _gloffset_CopyColorTable },
   { 17748, _gloffset_ColorTableParameterfv },
   { 21171, _gloffset_GetColorTableParameterfv },
   { 23147, _gloffset_GetColorTableParameteriv },
   {    -1, -1 }
};
#endif

#if defined(need_GL_SUNX_constant_data)
static const struct gl_function_remap GL_SUNX_constant_data_functions[] = {
   { 28947, -1 }, /* FinishTextureSUNX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SUN_global_alpha)
static const struct gl_function_remap GL_SUN_global_alpha_functions[] = {
   {  3035, -1 }, /* GlobalAlphaFactorubSUN */
   {  4224, -1 }, /* GlobalAlphaFactoriSUN */
   {  5789, -1 }, /* GlobalAlphaFactordSUN */
   {  8862, -1 }, /* GlobalAlphaFactoruiSUN */
   {  9214, -1 }, /* GlobalAlphaFactorbSUN */
   { 12126, -1 }, /* GlobalAlphaFactorfSUN */
   { 12245, -1 }, /* GlobalAlphaFactorusSUN */
   { 20871, -1 }, /* GlobalAlphaFactorsSUN */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SUN_mesh_array)
static const struct gl_function_remap GL_SUN_mesh_array_functions[] = {
   { 26949, -1 }, /* DrawMeshArraysSUN */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SUN_triangle_list)
static const struct gl_function_remap GL_SUN_triangle_list_functions[] = {
   {  3971, -1 }, /* ReplacementCodeubSUN */
   {  5600, -1 }, /* ReplacementCodeubvSUN */
   { 17469, -1 }, /* ReplacementCodeusvSUN */
   { 17657, -1 }, /* ReplacementCodePointerSUN */
   { 19828, -1 }, /* ReplacementCodeuiSUN */
   { 20583, -1 }, /* ReplacementCodeusSUN */
   { 27594, -1 }, /* ReplacementCodeuivSUN */
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
   {  4105, -1 }, /* Color4ubVertex3fSUN */
   {  4181, -1 }, /* TexCoord2fVertex3fSUN */
   {  4508, -1 }, /* TexCoord2fColor4fNormal3fVertex3fSUN */
   {  4866, -1 }, /* TexCoord2fNormal3fVertex3fvSUN */
   {  5495, -1 }, /* ReplacementCodeuiTexCoord2fNormal3fVertex3fSUN */
   {  6240, -1 }, /* ReplacementCodeuiColor4ubVertex3fvSUN */
   {  6599, -1 }, /* ReplacementCodeuiTexCoord2fVertex3fSUN */
   {  7311, -1 }, /* TexCoord2fNormal3fVertex3fSUN */
   {  8079, -1 }, /* Color3fVertex3fSUN */
   {  9173, -1 }, /* Color3fVertex3fvSUN */
   {  9588, -1 }, /* Color4fNormal3fVertex3fvSUN */
   { 10325, -1 }, /* ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN */
   { 11674, -1 }, /* ReplacementCodeuiColor4fNormal3fVertex3fvSUN */
   { 13218, -1 }, /* ReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN */
   { 13629, -1 }, /* TexCoord2fColor3fVertex3fSUN */
   { 14759, -1 }, /* TexCoord4fColor4fNormal3fVertex4fSUN */
   { 15093, -1 }, /* Color4ubVertex2fvSUN */
   { 15333, -1 }, /* Normal3fVertex3fSUN */
   { 16329, -1 }, /* ReplacementCodeuiColor4fNormal3fVertex3fSUN */
   { 16590, -1 }, /* TexCoord2fColor4fNormal3fVertex3fvSUN */
   { 17298, -1 }, /* TexCoord2fVertex3fvSUN */
   { 18074, -1 }, /* Color4ubVertex2fSUN */
   { 18295, -1 }, /* ReplacementCodeuiColor4ubVertex3fSUN */
   { 20253, -1 }, /* TexCoord2fColor4ubVertex3fSUN */
   { 20651, -1 }, /* Normal3fVertex3fvSUN */
   { 21080, -1 }, /* Color4fNormal3fVertex3fSUN */
   { 21989, -1 }, /* ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN */
   { 23984, -1 }, /* ReplacementCodeuiColor3fVertex3fSUN */
   { 25209, -1 }, /* TexCoord4fVertex4fSUN */
   { 25635, -1 }, /* TexCoord2fColor3fVertex3fvSUN */
   { 25986, -1 }, /* ReplacementCodeuiNormal3fVertex3fvSUN */
   { 26113, -1 }, /* TexCoord4fVertex4fvSUN */
   { 26821, -1 }, /* ReplacementCodeuiVertex3fSUN */
   {    -1, -1 }
};
#endif

#if defined(need_GL_VERSION_1_3)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_VERSION_1_3_functions[] = {
   {   381, _gloffset_MultiTexCoord3sARB },
   {   613, _gloffset_ActiveTextureARB },
   {  3761, _gloffset_MultiTexCoord1fvARB },
   {  5386, _gloffset_MultiTexCoord3dARB },
   {  5431, _gloffset_MultiTexCoord2iARB },
   {  5555, _gloffset_MultiTexCoord2svARB },
   {  7520, _gloffset_MultiTexCoord2fARB },
   {  9424, _gloffset_MultiTexCoord3fvARB },
   {  9981, _gloffset_MultiTexCoord4sARB },
   { 10615, _gloffset_MultiTexCoord2dvARB },
   { 10997, _gloffset_MultiTexCoord1svARB },
   { 11369, _gloffset_MultiTexCoord3svARB },
   { 11430, _gloffset_MultiTexCoord4iARB },
   { 12153, _gloffset_MultiTexCoord3iARB },
   { 12956, _gloffset_MultiTexCoord1dARB },
   { 13173, _gloffset_MultiTexCoord3dvARB },
   { 14337, _gloffset_MultiTexCoord3ivARB },
   { 14382, _gloffset_MultiTexCoord2sARB },
   { 15680, _gloffset_MultiTexCoord4ivARB },
   { 17398, _gloffset_ClientActiveTextureARB },
   { 19681, _gloffset_MultiTexCoord2dARB },
   { 20122, _gloffset_MultiTexCoord4dvARB },
   { 20433, _gloffset_MultiTexCoord4fvARB },
   { 21312, _gloffset_MultiTexCoord3fARB },
   { 23675, _gloffset_MultiTexCoord4dARB },
   { 23941, _gloffset_MultiTexCoord1sARB },
   { 24145, _gloffset_MultiTexCoord1dvARB },
   { 24955, _gloffset_MultiTexCoord1ivARB },
   { 25048, _gloffset_MultiTexCoord2ivARB },
   { 25387, _gloffset_MultiTexCoord1iARB },
   { 26689, _gloffset_MultiTexCoord4svARB },
   { 27207, _gloffset_MultiTexCoord1fARB },
   { 27470, _gloffset_MultiTexCoord4fARB },
   { 29774, _gloffset_MultiTexCoord2fvARB },
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

