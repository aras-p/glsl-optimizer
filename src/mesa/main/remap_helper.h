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

struct gl_function_remap {
   GLint func_index;
   GLint dispatch_offset; /* for sanity check */
};

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
   /* _mesa_function_pool[2906]: FragmentLightModelivSGIX (dynamic) */
   "ip\0"
   "glFragmentLightModelivSGIX\0"
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
   /* _mesa_function_pool[4480]: TexCoord2fColor4fNormal3fVertex3fSUN (dynamic) */
   "ffffffffffff\0"
   "glTexCoord2fColor4fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[4533]: Indexub (offset 315) */
   "i\0"
   "glIndexub\0"
   "\0"
   /* _mesa_function_pool[4546]: TexEnvi (offset 186) */
   "iii\0"
   "glTexEnvi\0"
   "\0"
   /* _mesa_function_pool[4561]: GetClipPlane (offset 259) */
   "ip\0"
   "glGetClipPlane\0"
   "\0"
   /* _mesa_function_pool[4580]: CombinerParameterfvNV (will be remapped) */
   "ip\0"
   "glCombinerParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[4608]: VertexAttribs3dvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs3dvNV\0"
   "\0"
   /* _mesa_function_pool[4634]: VertexAttribs4fvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs4fvNV\0"
   "\0"
   /* _mesa_function_pool[4660]: VertexArrayRangeNV (will be remapped) */
   "ip\0"
   "glVertexArrayRangeNV\0"
   "\0"
   /* _mesa_function_pool[4685]: FragmentLightiSGIX (dynamic) */
   "iii\0"
   "glFragmentLightiSGIX\0"
   "\0"
   /* _mesa_function_pool[4711]: PolygonOffsetEXT (will be remapped) */
   "ff\0"
   "glPolygonOffsetEXT\0"
   "\0"
   /* _mesa_function_pool[4734]: PollAsyncSGIX (dynamic) */
   "p\0"
   "glPollAsyncSGIX\0"
   "\0"
   /* _mesa_function_pool[4753]: DeleteFragmentShaderATI (will be remapped) */
   "i\0"
   "glDeleteFragmentShaderATI\0"
   "\0"
   /* _mesa_function_pool[4782]: Scaled (offset 301) */
   "ddd\0"
   "glScaled\0"
   "\0"
   /* _mesa_function_pool[4796]: Scalef (offset 302) */
   "fff\0"
   "glScalef\0"
   "\0"
   /* _mesa_function_pool[4810]: TexCoord2fNormal3fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glTexCoord2fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[4848]: MultTransposeMatrixdARB (will be remapped) */
   "p\0"
   "glMultTransposeMatrixd\0"
   "glMultTransposeMatrixdARB\0"
   "\0"
   /* _mesa_function_pool[4900]: ObjectUnpurgeableAPPLE (will be remapped) */
   "iii\0"
   "glObjectUnpurgeableAPPLE\0"
   "\0"
   /* _mesa_function_pool[4930]: AlphaFunc (offset 240) */
   "if\0"
   "glAlphaFunc\0"
   "\0"
   /* _mesa_function_pool[4946]: WindowPos2svMESA (will be remapped) */
   "p\0"
   "glWindowPos2sv\0"
   "glWindowPos2svARB\0"
   "glWindowPos2svMESA\0"
   "\0"
   /* _mesa_function_pool[5001]: EdgeFlag (offset 41) */
   "i\0"
   "glEdgeFlag\0"
   "\0"
   /* _mesa_function_pool[5015]: TexCoord2iv (offset 107) */
   "p\0"
   "glTexCoord2iv\0"
   "\0"
   /* _mesa_function_pool[5032]: CompressedTexImage1DARB (will be remapped) */
   "iiiiiip\0"
   "glCompressedTexImage1D\0"
   "glCompressedTexImage1DARB\0"
   "\0"
   /* _mesa_function_pool[5090]: Rotated (offset 299) */
   "dddd\0"
   "glRotated\0"
   "\0"
   /* _mesa_function_pool[5106]: VertexAttrib2sNV (will be remapped) */
   "iii\0"
   "glVertexAttrib2sNV\0"
   "\0"
   /* _mesa_function_pool[5130]: ReadPixels (offset 256) */
   "iiiiiip\0"
   "glReadPixels\0"
   "\0"
   /* _mesa_function_pool[5152]: EdgeFlagv (offset 42) */
   "p\0"
   "glEdgeFlagv\0"
   "\0"
   /* _mesa_function_pool[5167]: NormalPointerListIBM (dynamic) */
   "iipi\0"
   "glNormalPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[5196]: IndexPointerEXT (will be remapped) */
   "iiip\0"
   "glIndexPointerEXT\0"
   "\0"
   /* _mesa_function_pool[5220]: Color4iv (offset 32) */
   "p\0"
   "glColor4iv\0"
   "\0"
   /* _mesa_function_pool[5234]: TexParameterf (offset 178) */
   "iif\0"
   "glTexParameterf\0"
   "\0"
   /* _mesa_function_pool[5255]: TexParameteri (offset 180) */
   "iii\0"
   "glTexParameteri\0"
   "\0"
   /* _mesa_function_pool[5276]: NormalPointerEXT (will be remapped) */
   "iiip\0"
   "glNormalPointerEXT\0"
   "\0"
   /* _mesa_function_pool[5301]: MultiTexCoord3dARB (offset 392) */
   "iddd\0"
   "glMultiTexCoord3d\0"
   "glMultiTexCoord3dARB\0"
   "\0"
   /* _mesa_function_pool[5346]: MultiTexCoord2iARB (offset 388) */
   "iii\0"
   "glMultiTexCoord2i\0"
   "glMultiTexCoord2iARB\0"
   "\0"
   /* _mesa_function_pool[5390]: DrawPixels (offset 257) */
   "iiiip\0"
   "glDrawPixels\0"
   "\0"
   /* _mesa_function_pool[5410]: ReplacementCodeuiTexCoord2fNormal3fVertex3fSUN (dynamic) */
   "iffffffff\0"
   "glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[5470]: MultiTexCoord2svARB (offset 391) */
   "ip\0"
   "glMultiTexCoord2sv\0"
   "glMultiTexCoord2svARB\0"
   "\0"
   /* _mesa_function_pool[5515]: ReplacementCodeubvSUN (dynamic) */
   "p\0"
   "glReplacementCodeubvSUN\0"
   "\0"
   /* _mesa_function_pool[5542]: Uniform3iARB (will be remapped) */
   "iiii\0"
   "glUniform3i\0"
   "glUniform3iARB\0"
   "\0"
   /* _mesa_function_pool[5575]: GetFragmentMaterialfvSGIX (dynamic) */
   "iip\0"
   "glGetFragmentMaterialfvSGIX\0"
   "\0"
   /* _mesa_function_pool[5608]: GetShaderInfoLog (will be remapped) */
   "iipp\0"
   "glGetShaderInfoLog\0"
   "\0"
   /* _mesa_function_pool[5633]: WeightivARB (dynamic) */
   "ip\0"
   "glWeightivARB\0"
   "\0"
   /* _mesa_function_pool[5651]: PollInstrumentsSGIX (dynamic) */
   "p\0"
   "glPollInstrumentsSGIX\0"
   "\0"
   /* _mesa_function_pool[5676]: GlobalAlphaFactordSUN (dynamic) */
   "d\0"
   "glGlobalAlphaFactordSUN\0"
   "\0"
   /* _mesa_function_pool[5703]: GetFinalCombinerInputParameterfvNV (will be remapped) */
   "iip\0"
   "glGetFinalCombinerInputParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[5745]: GenerateMipmapEXT (will be remapped) */
   "i\0"
   "glGenerateMipmap\0"
   "glGenerateMipmapEXT\0"
   "\0"
   /* _mesa_function_pool[5785]: GenLists (offset 5) */
   "i\0"
   "glGenLists\0"
   "\0"
   /* _mesa_function_pool[5799]: SetFragmentShaderConstantATI (will be remapped) */
   "ip\0"
   "glSetFragmentShaderConstantATI\0"
   "\0"
   /* _mesa_function_pool[5834]: GetMapAttribParameterivNV (dynamic) */
   "iiip\0"
   "glGetMapAttribParameterivNV\0"
   "\0"
   /* _mesa_function_pool[5868]: CreateShaderObjectARB (will be remapped) */
   "i\0"
   "glCreateShaderObjectARB\0"
   "\0"
   /* _mesa_function_pool[5895]: GetSharpenTexFuncSGIS (dynamic) */
   "ip\0"
   "glGetSharpenTexFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[5923]: BufferDataARB (will be remapped) */
   "iipi\0"
   "glBufferData\0"
   "glBufferDataARB\0"
   "\0"
   /* _mesa_function_pool[5958]: FlushVertexArrayRangeNV (will be remapped) */
   "\0"
   "glFlushVertexArrayRangeNV\0"
   "\0"
   /* _mesa_function_pool[5986]: MapGrid2d (offset 226) */
   "iddidd\0"
   "glMapGrid2d\0"
   "\0"
   /* _mesa_function_pool[6006]: MapGrid2f (offset 227) */
   "iffiff\0"
   "glMapGrid2f\0"
   "\0"
   /* _mesa_function_pool[6026]: SampleMapATI (will be remapped) */
   "iii\0"
   "glSampleMapATI\0"
   "\0"
   /* _mesa_function_pool[6046]: VertexPointerEXT (will be remapped) */
   "iiiip\0"
   "glVertexPointerEXT\0"
   "\0"
   /* _mesa_function_pool[6072]: GetTexFilterFuncSGIS (dynamic) */
   "iip\0"
   "glGetTexFilterFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[6100]: Scissor (offset 176) */
   "iiii\0"
   "glScissor\0"
   "\0"
   /* _mesa_function_pool[6116]: Fogf (offset 153) */
   "if\0"
   "glFogf\0"
   "\0"
   /* _mesa_function_pool[6127]: GetCombinerOutputParameterfvNV (will be remapped) */
   "iiip\0"
   "glGetCombinerOutputParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[6166]: TexSubImage1D (offset 332) */
   "iiiiiip\0"
   "glTexSubImage1D\0"
   "glTexSubImage1DEXT\0"
   "\0"
   /* _mesa_function_pool[6210]: VertexAttrib1sARB (will be remapped) */
   "ii\0"
   "glVertexAttrib1s\0"
   "glVertexAttrib1sARB\0"
   "\0"
   /* _mesa_function_pool[6251]: FenceSync (will be remapped) */
   "ii\0"
   "glFenceSync\0"
   "\0"
   /* _mesa_function_pool[6267]: Color4usv (offset 40) */
   "p\0"
   "glColor4usv\0"
   "\0"
   /* _mesa_function_pool[6282]: Fogi (offset 155) */
   "ii\0"
   "glFogi\0"
   "\0"
   /* _mesa_function_pool[6293]: DepthRange (offset 288) */
   "dd\0"
   "glDepthRange\0"
   "\0"
   /* _mesa_function_pool[6310]: RasterPos3iv (offset 75) */
   "p\0"
   "glRasterPos3iv\0"
   "\0"
   /* _mesa_function_pool[6328]: FinalCombinerInputNV (will be remapped) */
   "iiii\0"
   "glFinalCombinerInputNV\0"
   "\0"
   /* _mesa_function_pool[6357]: TexCoord2i (offset 106) */
   "ii\0"
   "glTexCoord2i\0"
   "\0"
   /* _mesa_function_pool[6374]: PixelMapfv (offset 251) */
   "iip\0"
   "glPixelMapfv\0"
   "\0"
   /* _mesa_function_pool[6392]: Color4ui (offset 37) */
   "iiii\0"
   "glColor4ui\0"
   "\0"
   /* _mesa_function_pool[6409]: RasterPos3s (offset 76) */
   "iii\0"
   "glRasterPos3s\0"
   "\0"
   /* _mesa_function_pool[6428]: Color3usv (offset 24) */
   "p\0"
   "glColor3usv\0"
   "\0"
   /* _mesa_function_pool[6443]: FlushRasterSGIX (dynamic) */
   "\0"
   "glFlushRasterSGIX\0"
   "\0"
   /* _mesa_function_pool[6463]: TexCoord2f (offset 104) */
   "ff\0"
   "glTexCoord2f\0"
   "\0"
   /* _mesa_function_pool[6480]: ReplacementCodeuiTexCoord2fVertex3fSUN (dynamic) */
   "ifffff\0"
   "glReplacementCodeuiTexCoord2fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[6529]: TexCoord2d (offset 102) */
   "dd\0"
   "glTexCoord2d\0"
   "\0"
   /* _mesa_function_pool[6546]: RasterPos3d (offset 70) */
   "ddd\0"
   "glRasterPos3d\0"
   "\0"
   /* _mesa_function_pool[6565]: RasterPos3f (offset 72) */
   "fff\0"
   "glRasterPos3f\0"
   "\0"
   /* _mesa_function_pool[6584]: Uniform1fARB (will be remapped) */
   "if\0"
   "glUniform1f\0"
   "glUniform1fARB\0"
   "\0"
   /* _mesa_function_pool[6615]: AreTexturesResident (offset 322) */
   "ipp\0"
   "glAreTexturesResident\0"
   "glAreTexturesResidentEXT\0"
   "\0"
   /* _mesa_function_pool[6667]: TexCoord2s (offset 108) */
   "ii\0"
   "glTexCoord2s\0"
   "\0"
   /* _mesa_function_pool[6684]: StencilOpSeparate (will be remapped) */
   "iiii\0"
   "glStencilOpSeparate\0"
   "glStencilOpSeparateATI\0"
   "\0"
   /* _mesa_function_pool[6733]: ColorTableParameteriv (offset 341) */
   "iip\0"
   "glColorTableParameteriv\0"
   "glColorTableParameterivSGI\0"
   "\0"
   /* _mesa_function_pool[6789]: FogCoordPointerListIBM (dynamic) */
   "iipi\0"
   "glFogCoordPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[6820]: WindowPos3dMESA (will be remapped) */
   "ddd\0"
   "glWindowPos3d\0"
   "glWindowPos3dARB\0"
   "glWindowPos3dMESA\0"
   "\0"
   /* _mesa_function_pool[6874]: Color4us (offset 39) */
   "iiii\0"
   "glColor4us\0"
   "\0"
   /* _mesa_function_pool[6891]: PointParameterfvEXT (will be remapped) */
   "ip\0"
   "glPointParameterfv\0"
   "glPointParameterfvARB\0"
   "glPointParameterfvEXT\0"
   "glPointParameterfvSGIS\0"
   "\0"
   /* _mesa_function_pool[6981]: Color3bv (offset 10) */
   "p\0"
   "glColor3bv\0"
   "\0"
   /* _mesa_function_pool[6995]: WindowPos2fvMESA (will be remapped) */
   "p\0"
   "glWindowPos2fv\0"
   "glWindowPos2fvARB\0"
   "glWindowPos2fvMESA\0"
   "\0"
   /* _mesa_function_pool[7050]: SecondaryColor3bvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3bv\0"
   "glSecondaryColor3bvEXT\0"
   "\0"
   /* _mesa_function_pool[7096]: VertexPointerListIBM (dynamic) */
   "iiipi\0"
   "glVertexPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[7126]: GetProgramLocalParameterfvARB (will be remapped) */
   "iip\0"
   "glGetProgramLocalParameterfvARB\0"
   "\0"
   /* _mesa_function_pool[7163]: FragmentMaterialfSGIX (dynamic) */
   "iif\0"
   "glFragmentMaterialfSGIX\0"
   "\0"
   /* _mesa_function_pool[7192]: TexCoord2fNormal3fVertex3fSUN (dynamic) */
   "ffffffff\0"
   "glTexCoord2fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[7234]: RenderbufferStorageEXT (will be remapped) */
   "iiii\0"
   "glRenderbufferStorage\0"
   "glRenderbufferStorageEXT\0"
   "\0"
   /* _mesa_function_pool[7287]: IsFenceNV (will be remapped) */
   "i\0"
   "glIsFenceNV\0"
   "\0"
   /* _mesa_function_pool[7302]: AttachObjectARB (will be remapped) */
   "ii\0"
   "glAttachObjectARB\0"
   "\0"
   /* _mesa_function_pool[7324]: GetFragmentLightivSGIX (dynamic) */
   "iip\0"
   "glGetFragmentLightivSGIX\0"
   "\0"
   /* _mesa_function_pool[7354]: UniformMatrix2fvARB (will be remapped) */
   "iiip\0"
   "glUniformMatrix2fv\0"
   "glUniformMatrix2fvARB\0"
   "\0"
   /* _mesa_function_pool[7401]: MultiTexCoord2fARB (offset 386) */
   "iff\0"
   "glMultiTexCoord2f\0"
   "glMultiTexCoord2fARB\0"
   "\0"
   /* _mesa_function_pool[7445]: ColorTable (offset 339) */
   "iiiiip\0"
   "glColorTable\0"
   "glColorTableSGI\0"
   "glColorTableEXT\0"
   "\0"
   /* _mesa_function_pool[7498]: IndexPointer (offset 314) */
   "iip\0"
   "glIndexPointer\0"
   "\0"
   /* _mesa_function_pool[7518]: Accum (offset 213) */
   "if\0"
   "glAccum\0"
   "\0"
   /* _mesa_function_pool[7530]: GetTexImage (offset 281) */
   "iiiip\0"
   "glGetTexImage\0"
   "\0"
   /* _mesa_function_pool[7551]: MapControlPointsNV (dynamic) */
   "iiiiiiiip\0"
   "glMapControlPointsNV\0"
   "\0"
   /* _mesa_function_pool[7583]: ConvolutionFilter2D (offset 349) */
   "iiiiiip\0"
   "glConvolutionFilter2D\0"
   "glConvolutionFilter2DEXT\0"
   "\0"
   /* _mesa_function_pool[7639]: Finish (offset 216) */
   "\0"
   "glFinish\0"
   "\0"
   /* _mesa_function_pool[7650]: MapParameterfvNV (dynamic) */
   "iip\0"
   "glMapParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[7674]: ClearStencil (offset 207) */
   "i\0"
   "glClearStencil\0"
   "\0"
   /* _mesa_function_pool[7692]: VertexAttrib3dvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib3dv\0"
   "glVertexAttrib3dvARB\0"
   "\0"
   /* _mesa_function_pool[7735]: HintPGI (dynamic) */
   "ii\0"
   "glHintPGI\0"
   "\0"
   /* _mesa_function_pool[7749]: ConvolutionParameteriv (offset 353) */
   "iip\0"
   "glConvolutionParameteriv\0"
   "glConvolutionParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[7807]: Color4s (offset 33) */
   "iiii\0"
   "glColor4s\0"
   "\0"
   /* _mesa_function_pool[7823]: InterleavedArrays (offset 317) */
   "iip\0"
   "glInterleavedArrays\0"
   "\0"
   /* _mesa_function_pool[7848]: RasterPos2fv (offset 65) */
   "p\0"
   "glRasterPos2fv\0"
   "\0"
   /* _mesa_function_pool[7866]: TexCoord1fv (offset 97) */
   "p\0"
   "glTexCoord1fv\0"
   "\0"
   /* _mesa_function_pool[7883]: Vertex2d (offset 126) */
   "dd\0"
   "glVertex2d\0"
   "\0"
   /* _mesa_function_pool[7898]: CullParameterdvEXT (will be remapped) */
   "ip\0"
   "glCullParameterdvEXT\0"
   "\0"
   /* _mesa_function_pool[7923]: ProgramNamedParameter4fNV (will be remapped) */
   "iipffff\0"
   "glProgramNamedParameter4fNV\0"
   "\0"
   /* _mesa_function_pool[7960]: Color3fVertex3fSUN (dynamic) */
   "ffffff\0"
   "glColor3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[7989]: ProgramEnvParameter4fvARB (will be remapped) */
   "iip\0"
   "glProgramEnvParameter4fvARB\0"
   "glProgramParameter4fvNV\0"
   "\0"
   /* _mesa_function_pool[8046]: Color4i (offset 31) */
   "iiii\0"
   "glColor4i\0"
   "\0"
   /* _mesa_function_pool[8062]: Color4f (offset 29) */
   "ffff\0"
   "glColor4f\0"
   "\0"
   /* _mesa_function_pool[8078]: RasterPos4fv (offset 81) */
   "p\0"
   "glRasterPos4fv\0"
   "\0"
   /* _mesa_function_pool[8096]: Color4d (offset 27) */
   "dddd\0"
   "glColor4d\0"
   "\0"
   /* _mesa_function_pool[8112]: ClearIndex (offset 205) */
   "f\0"
   "glClearIndex\0"
   "\0"
   /* _mesa_function_pool[8128]: Color4b (offset 25) */
   "iiii\0"
   "glColor4b\0"
   "\0"
   /* _mesa_function_pool[8144]: LoadMatrixd (offset 292) */
   "p\0"
   "glLoadMatrixd\0"
   "\0"
   /* _mesa_function_pool[8161]: FragmentLightModeliSGIX (dynamic) */
   "ii\0"
   "glFragmentLightModeliSGIX\0"
   "\0"
   /* _mesa_function_pool[8191]: RasterPos2dv (offset 63) */
   "p\0"
   "glRasterPos2dv\0"
   "\0"
   /* _mesa_function_pool[8209]: ConvolutionParameterfv (offset 351) */
   "iip\0"
   "glConvolutionParameterfv\0"
   "glConvolutionParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[8267]: TbufferMask3DFX (dynamic) */
   "i\0"
   "glTbufferMask3DFX\0"
   "\0"
   /* _mesa_function_pool[8288]: GetTexGendv (offset 278) */
   "iip\0"
   "glGetTexGendv\0"
   "\0"
   /* _mesa_function_pool[8307]: GetVertexAttribfvNV (will be remapped) */
   "iip\0"
   "glGetVertexAttribfvNV\0"
   "\0"
   /* _mesa_function_pool[8334]: BeginTransformFeedbackEXT (will be remapped) */
   "i\0"
   "glBeginTransformFeedbackEXT\0"
   "glBeginTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[8390]: LoadProgramNV (will be remapped) */
   "iiip\0"
   "glLoadProgramNV\0"
   "\0"
   /* _mesa_function_pool[8412]: WaitSync (will be remapped) */
   "iii\0"
   "glWaitSync\0"
   "\0"
   /* _mesa_function_pool[8428]: EndList (offset 1) */
   "\0"
   "glEndList\0"
   "\0"
   /* _mesa_function_pool[8440]: VertexAttrib4fvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib4fvNV\0"
   "\0"
   /* _mesa_function_pool[8464]: GetAttachedObjectsARB (will be remapped) */
   "iipp\0"
   "glGetAttachedObjectsARB\0"
   "\0"
   /* _mesa_function_pool[8494]: Uniform3fvARB (will be remapped) */
   "iip\0"
   "glUniform3fv\0"
   "glUniform3fvARB\0"
   "\0"
   /* _mesa_function_pool[8528]: EvalCoord1fv (offset 231) */
   "p\0"
   "glEvalCoord1fv\0"
   "\0"
   /* _mesa_function_pool[8546]: DrawRangeElements (offset 338) */
   "iiiiip\0"
   "glDrawRangeElements\0"
   "glDrawRangeElementsEXT\0"
   "\0"
   /* _mesa_function_pool[8597]: EvalMesh2 (offset 238) */
   "iiiii\0"
   "glEvalMesh2\0"
   "\0"
   /* _mesa_function_pool[8616]: Vertex4fv (offset 145) */
   "p\0"
   "glVertex4fv\0"
   "\0"
   /* _mesa_function_pool[8631]: SpriteParameterfvSGIX (dynamic) */
   "ip\0"
   "glSpriteParameterfvSGIX\0"
   "\0"
   /* _mesa_function_pool[8659]: CheckFramebufferStatusEXT (will be remapped) */
   "i\0"
   "glCheckFramebufferStatus\0"
   "glCheckFramebufferStatusEXT\0"
   "\0"
   /* _mesa_function_pool[8715]: GlobalAlphaFactoruiSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactoruiSUN\0"
   "\0"
   /* _mesa_function_pool[8743]: GetHandleARB (will be remapped) */
   "i\0"
   "glGetHandleARB\0"
   "\0"
   /* _mesa_function_pool[8761]: GetVertexAttribivARB (will be remapped) */
   "iip\0"
   "glGetVertexAttribiv\0"
   "glGetVertexAttribivARB\0"
   "\0"
   /* _mesa_function_pool[8809]: GetCombinerInputParameterfvNV (will be remapped) */
   "iiiip\0"
   "glGetCombinerInputParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[8848]: CreateProgram (will be remapped) */
   "\0"
   "glCreateProgram\0"
   "\0"
   /* _mesa_function_pool[8866]: LoadTransposeMatrixdARB (will be remapped) */
   "p\0"
   "glLoadTransposeMatrixd\0"
   "glLoadTransposeMatrixdARB\0"
   "\0"
   /* _mesa_function_pool[8918]: GetMinmax (offset 364) */
   "iiiip\0"
   "glGetMinmax\0"
   "glGetMinmaxEXT\0"
   "\0"
   /* _mesa_function_pool[8952]: StencilFuncSeparate (will be remapped) */
   "iiii\0"
   "glStencilFuncSeparate\0"
   "\0"
   /* _mesa_function_pool[8980]: SecondaryColor3sEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3s\0"
   "glSecondaryColor3sEXT\0"
   "\0"
   /* _mesa_function_pool[9026]: Color3fVertex3fvSUN (dynamic) */
   "pp\0"
   "glColor3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[9052]: Normal3fv (offset 57) */
   "p\0"
   "glNormal3fv\0"
   "\0"
   /* _mesa_function_pool[9067]: GlobalAlphaFactorbSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactorbSUN\0"
   "\0"
   /* _mesa_function_pool[9094]: Color3us (offset 23) */
   "iii\0"
   "glColor3us\0"
   "\0"
   /* _mesa_function_pool[9110]: ImageTransformParameterfvHP (dynamic) */
   "iip\0"
   "glImageTransformParameterfvHP\0"
   "\0"
   /* _mesa_function_pool[9145]: VertexAttrib4ivARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4iv\0"
   "glVertexAttrib4ivARB\0"
   "\0"
   /* _mesa_function_pool[9188]: End (offset 43) */
   "\0"
   "glEnd\0"
   "\0"
   /* _mesa_function_pool[9196]: VertexAttrib3fNV (will be remapped) */
   "ifff\0"
   "glVertexAttrib3fNV\0"
   "\0"
   /* _mesa_function_pool[9221]: VertexAttribs2dvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs2dvNV\0"
   "\0"
   /* _mesa_function_pool[9247]: GetQueryObjectui64vEXT (will be remapped) */
   "iip\0"
   "glGetQueryObjectui64vEXT\0"
   "\0"
   /* _mesa_function_pool[9277]: MultiTexCoord3fvARB (offset 395) */
   "ip\0"
   "glMultiTexCoord3fv\0"
   "glMultiTexCoord3fvARB\0"
   "\0"
   /* _mesa_function_pool[9322]: SecondaryColor3dEXT (will be remapped) */
   "ddd\0"
   "glSecondaryColor3d\0"
   "glSecondaryColor3dEXT\0"
   "\0"
   /* _mesa_function_pool[9368]: Color3ub (offset 19) */
   "iii\0"
   "glColor3ub\0"
   "\0"
   /* _mesa_function_pool[9384]: GetProgramParameterfvNV (will be remapped) */
   "iiip\0"
   "glGetProgramParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[9416]: TangentPointerEXT (dynamic) */
   "iip\0"
   "glTangentPointerEXT\0"
   "\0"
   /* _mesa_function_pool[9441]: Color4fNormal3fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glColor4fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[9476]: GetInstrumentsSGIX (dynamic) */
   "\0"
   "glGetInstrumentsSGIX\0"
   "\0"
   /* _mesa_function_pool[9499]: Color3ui (offset 21) */
   "iii\0"
   "glColor3ui\0"
   "\0"
   /* _mesa_function_pool[9515]: EvalMapsNV (dynamic) */
   "ii\0"
   "glEvalMapsNV\0"
   "\0"
   /* _mesa_function_pool[9532]: TexSubImage2D (offset 333) */
   "iiiiiiiip\0"
   "glTexSubImage2D\0"
   "glTexSubImage2DEXT\0"
   "\0"
   /* _mesa_function_pool[9578]: FragmentLightivSGIX (dynamic) */
   "iip\0"
   "glFragmentLightivSGIX\0"
   "\0"
   /* _mesa_function_pool[9605]: GetTexParameterPointervAPPLE (will be remapped) */
   "iip\0"
   "glGetTexParameterPointervAPPLE\0"
   "\0"
   /* _mesa_function_pool[9641]: TexGenfv (offset 191) */
   "iip\0"
   "glTexGenfv\0"
   "\0"
   /* _mesa_function_pool[9657]: GetTransformFeedbackVaryingEXT (will be remapped) */
   "iiipppp\0"
   "glGetTransformFeedbackVaryingEXT\0"
   "glGetTransformFeedbackVarying\0"
   "\0"
   /* _mesa_function_pool[9729]: VertexAttrib4bvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4bv\0"
   "glVertexAttrib4bvARB\0"
   "\0"
   /* _mesa_function_pool[9772]: AlphaFragmentOp2ATI (will be remapped) */
   "iiiiiiiii\0"
   "glAlphaFragmentOp2ATI\0"
   "\0"
   /* _mesa_function_pool[9805]: GetIntegerIndexedvEXT (will be remapped) */
   "iip\0"
   "glGetIntegerIndexedvEXT\0"
   "\0"
   /* _mesa_function_pool[9834]: MultiTexCoord4sARB (offset 406) */
   "iiiii\0"
   "glMultiTexCoord4s\0"
   "glMultiTexCoord4sARB\0"
   "\0"
   /* _mesa_function_pool[9880]: GetFragmentMaterialivSGIX (dynamic) */
   "iip\0"
   "glGetFragmentMaterialivSGIX\0"
   "\0"
   /* _mesa_function_pool[9913]: WindowPos4dMESA (will be remapped) */
   "dddd\0"
   "glWindowPos4dMESA\0"
   "\0"
   /* _mesa_function_pool[9937]: WeightPointerARB (dynamic) */
   "iiip\0"
   "glWeightPointerARB\0"
   "\0"
   /* _mesa_function_pool[9962]: WindowPos2dMESA (will be remapped) */
   "dd\0"
   "glWindowPos2d\0"
   "glWindowPos2dARB\0"
   "glWindowPos2dMESA\0"
   "\0"
   /* _mesa_function_pool[10015]: FramebufferTexture3DEXT (will be remapped) */
   "iiiiii\0"
   "glFramebufferTexture3D\0"
   "glFramebufferTexture3DEXT\0"
   "\0"
   /* _mesa_function_pool[10072]: BlendEquation (offset 337) */
   "i\0"
   "glBlendEquation\0"
   "glBlendEquationEXT\0"
   "\0"
   /* _mesa_function_pool[10110]: VertexAttrib3dNV (will be remapped) */
   "iddd\0"
   "glVertexAttrib3dNV\0"
   "\0"
   /* _mesa_function_pool[10135]: VertexAttrib3dARB (will be remapped) */
   "iddd\0"
   "glVertexAttrib3d\0"
   "glVertexAttrib3dARB\0"
   "\0"
   /* _mesa_function_pool[10178]: ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN (dynamic) */
   "ppppp\0"
   "glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[10242]: VertexAttrib4fARB (will be remapped) */
   "iffff\0"
   "glVertexAttrib4f\0"
   "glVertexAttrib4fARB\0"
   "\0"
   /* _mesa_function_pool[10286]: GetError (offset 261) */
   "\0"
   "glGetError\0"
   "\0"
   /* _mesa_function_pool[10299]: IndexFuncEXT (dynamic) */
   "if\0"
   "glIndexFuncEXT\0"
   "\0"
   /* _mesa_function_pool[10318]: TexCoord3dv (offset 111) */
   "p\0"
   "glTexCoord3dv\0"
   "\0"
   /* _mesa_function_pool[10335]: Indexdv (offset 45) */
   "p\0"
   "glIndexdv\0"
   "\0"
   /* _mesa_function_pool[10348]: FramebufferTexture2DEXT (will be remapped) */
   "iiiii\0"
   "glFramebufferTexture2D\0"
   "glFramebufferTexture2DEXT\0"
   "\0"
   /* _mesa_function_pool[10404]: Normal3s (offset 60) */
   "iii\0"
   "glNormal3s\0"
   "\0"
   /* _mesa_function_pool[10420]: GetObjectParameterivAPPLE (will be remapped) */
   "iiip\0"
   "glGetObjectParameterivAPPLE\0"
   "\0"
   /* _mesa_function_pool[10454]: PushName (offset 201) */
   "i\0"
   "glPushName\0"
   "\0"
   /* _mesa_function_pool[10468]: MultiTexCoord2dvARB (offset 385) */
   "ip\0"
   "glMultiTexCoord2dv\0"
   "glMultiTexCoord2dvARB\0"
   "\0"
   /* _mesa_function_pool[10513]: CullParameterfvEXT (will be remapped) */
   "ip\0"
   "glCullParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[10538]: Normal3i (offset 58) */
   "iii\0"
   "glNormal3i\0"
   "\0"
   /* _mesa_function_pool[10554]: ProgramNamedParameter4fvNV (will be remapped) */
   "iipp\0"
   "glProgramNamedParameter4fvNV\0"
   "\0"
   /* _mesa_function_pool[10589]: SecondaryColorPointerEXT (will be remapped) */
   "iiip\0"
   "glSecondaryColorPointer\0"
   "glSecondaryColorPointerEXT\0"
   "\0"
   /* _mesa_function_pool[10646]: VertexAttrib4fvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4fv\0"
   "glVertexAttrib4fvARB\0"
   "\0"
   /* _mesa_function_pool[10689]: ColorPointerListIBM (dynamic) */
   "iiipi\0"
   "glColorPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[10718]: GetActiveUniformARB (will be remapped) */
   "iiipppp\0"
   "glGetActiveUniform\0"
   "glGetActiveUniformARB\0"
   "\0"
   /* _mesa_function_pool[10768]: ImageTransformParameteriHP (dynamic) */
   "iii\0"
   "glImageTransformParameteriHP\0"
   "\0"
   /* _mesa_function_pool[10802]: Normal3b (offset 52) */
   "iii\0"
   "glNormal3b\0"
   "\0"
   /* _mesa_function_pool[10818]: Normal3d (offset 54) */
   "ddd\0"
   "glNormal3d\0"
   "\0"
   /* _mesa_function_pool[10834]: Normal3f (offset 56) */
   "fff\0"
   "glNormal3f\0"
   "\0"
   /* _mesa_function_pool[10850]: MultiTexCoord1svARB (offset 383) */
   "ip\0"
   "glMultiTexCoord1sv\0"
   "glMultiTexCoord1svARB\0"
   "\0"
   /* _mesa_function_pool[10895]: Indexi (offset 48) */
   "i\0"
   "glIndexi\0"
   "\0"
   /* _mesa_function_pool[10907]: EGLImageTargetTexture2DOES (will be remapped) */
   "ip\0"
   "glEGLImageTargetTexture2DOES\0"
   "\0"
   /* _mesa_function_pool[10940]: EndQueryARB (will be remapped) */
   "i\0"
   "glEndQuery\0"
   "glEndQueryARB\0"
   "\0"
   /* _mesa_function_pool[10968]: DeleteFencesNV (will be remapped) */
   "ip\0"
   "glDeleteFencesNV\0"
   "\0"
   /* _mesa_function_pool[10989]: DeformationMap3dSGIX (dynamic) */
   "iddiiddiiddiip\0"
   "glDeformationMap3dSGIX\0"
   "\0"
   /* _mesa_function_pool[11028]: BindBufferRangeEXT (will be remapped) */
   "iiiii\0"
   "glBindBufferRangeEXT\0"
   "glBindBufferRange\0"
   "\0"
   /* _mesa_function_pool[11074]: DepthMask (offset 211) */
   "i\0"
   "glDepthMask\0"
   "\0"
   /* _mesa_function_pool[11089]: IsShader (will be remapped) */
   "i\0"
   "glIsShader\0"
   "\0"
   /* _mesa_function_pool[11103]: Indexf (offset 46) */
   "f\0"
   "glIndexf\0"
   "\0"
   /* _mesa_function_pool[11115]: GetImageTransformParameterivHP (dynamic) */
   "iip\0"
   "glGetImageTransformParameterivHP\0"
   "\0"
   /* _mesa_function_pool[11153]: Indexd (offset 44) */
   "d\0"
   "glIndexd\0"
   "\0"
   /* _mesa_function_pool[11165]: GetMaterialiv (offset 270) */
   "iip\0"
   "glGetMaterialiv\0"
   "\0"
   /* _mesa_function_pool[11186]: StencilOp (offset 244) */
   "iii\0"
   "glStencilOp\0"
   "\0"
   /* _mesa_function_pool[11203]: WindowPos4ivMESA (will be remapped) */
   "p\0"
   "glWindowPos4ivMESA\0"
   "\0"
   /* _mesa_function_pool[11225]: MultiTexCoord3svARB (offset 399) */
   "ip\0"
   "glMultiTexCoord3sv\0"
   "glMultiTexCoord3svARB\0"
   "\0"
   /* _mesa_function_pool[11270]: TexEnvfv (offset 185) */
   "iip\0"
   "glTexEnvfv\0"
   "\0"
   /* _mesa_function_pool[11286]: MultiTexCoord4iARB (offset 404) */
   "iiiii\0"
   "glMultiTexCoord4i\0"
   "glMultiTexCoord4iARB\0"
   "\0"
   /* _mesa_function_pool[11332]: Indexs (offset 50) */
   "i\0"
   "glIndexs\0"
   "\0"
   /* _mesa_function_pool[11344]: Binormal3ivEXT (dynamic) */
   "p\0"
   "glBinormal3ivEXT\0"
   "\0"
   /* _mesa_function_pool[11364]: ResizeBuffersMESA (will be remapped) */
   "\0"
   "glResizeBuffersMESA\0"
   "\0"
   /* _mesa_function_pool[11386]: GetUniformivARB (will be remapped) */
   "iip\0"
   "glGetUniformiv\0"
   "glGetUniformivARB\0"
   "\0"
   /* _mesa_function_pool[11424]: PixelTexGenParameteriSGIS (will be remapped) */
   "ii\0"
   "glPixelTexGenParameteriSGIS\0"
   "\0"
   /* _mesa_function_pool[11456]: VertexPointervINTEL (dynamic) */
   "iip\0"
   "glVertexPointervINTEL\0"
   "\0"
   /* _mesa_function_pool[11483]: Vertex2i (offset 130) */
   "ii\0"
   "glVertex2i\0"
   "\0"
   /* _mesa_function_pool[11498]: LoadMatrixf (offset 291) */
   "p\0"
   "glLoadMatrixf\0"
   "\0"
   /* _mesa_function_pool[11515]: Vertex2f (offset 128) */
   "ff\0"
   "glVertex2f\0"
   "\0"
   /* _mesa_function_pool[11530]: ReplacementCodeuiColor4fNormal3fVertex3fvSUN (dynamic) */
   "pppp\0"
   "glReplacementCodeuiColor4fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[11583]: Color4bv (offset 26) */
   "p\0"
   "glColor4bv\0"
   "\0"
   /* _mesa_function_pool[11597]: VertexPointer (offset 321) */
   "iiip\0"
   "glVertexPointer\0"
   "\0"
   /* _mesa_function_pool[11619]: SecondaryColor3uiEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3ui\0"
   "glSecondaryColor3uiEXT\0"
   "\0"
   /* _mesa_function_pool[11667]: StartInstrumentsSGIX (dynamic) */
   "\0"
   "glStartInstrumentsSGIX\0"
   "\0"
   /* _mesa_function_pool[11692]: SecondaryColor3usvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3usv\0"
   "glSecondaryColor3usvEXT\0"
   "\0"
   /* _mesa_function_pool[11740]: VertexAttrib2fvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib2fvNV\0"
   "\0"
   /* _mesa_function_pool[11764]: ProgramLocalParameter4dvARB (will be remapped) */
   "iip\0"
   "glProgramLocalParameter4dvARB\0"
   "\0"
   /* _mesa_function_pool[11799]: DeleteLists (offset 4) */
   "ii\0"
   "glDeleteLists\0"
   "\0"
   /* _mesa_function_pool[11817]: LogicOp (offset 242) */
   "i\0"
   "glLogicOp\0"
   "\0"
   /* _mesa_function_pool[11830]: MatrixIndexuivARB (dynamic) */
   "ip\0"
   "glMatrixIndexuivARB\0"
   "\0"
   /* _mesa_function_pool[11854]: Vertex2s (offset 132) */
   "ii\0"
   "glVertex2s\0"
   "\0"
   /* _mesa_function_pool[11869]: RenderbufferStorageMultisample (will be remapped) */
   "iiiii\0"
   "glRenderbufferStorageMultisample\0"
   "glRenderbufferStorageMultisampleEXT\0"
   "\0"
   /* _mesa_function_pool[11945]: TexCoord4fv (offset 121) */
   "p\0"
   "glTexCoord4fv\0"
   "\0"
   /* _mesa_function_pool[11962]: Tangent3sEXT (dynamic) */
   "iii\0"
   "glTangent3sEXT\0"
   "\0"
   /* _mesa_function_pool[11982]: GlobalAlphaFactorfSUN (dynamic) */
   "f\0"
   "glGlobalAlphaFactorfSUN\0"
   "\0"
   /* _mesa_function_pool[12009]: MultiTexCoord3iARB (offset 396) */
   "iiii\0"
   "glMultiTexCoord3i\0"
   "glMultiTexCoord3iARB\0"
   "\0"
   /* _mesa_function_pool[12054]: IsProgram (will be remapped) */
   "i\0"
   "glIsProgram\0"
   "\0"
   /* _mesa_function_pool[12069]: TexCoordPointerListIBM (dynamic) */
   "iiipi\0"
   "glTexCoordPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[12101]: GlobalAlphaFactorusSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactorusSUN\0"
   "\0"
   /* _mesa_function_pool[12129]: VertexAttrib2dvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib2dvNV\0"
   "\0"
   /* _mesa_function_pool[12153]: FramebufferRenderbufferEXT (will be remapped) */
   "iiii\0"
   "glFramebufferRenderbuffer\0"
   "glFramebufferRenderbufferEXT\0"
   "\0"
   /* _mesa_function_pool[12214]: VertexAttrib1dvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib1dvNV\0"
   "\0"
   /* _mesa_function_pool[12238]: GenTextures (offset 328) */
   "ip\0"
   "glGenTextures\0"
   "glGenTexturesEXT\0"
   "\0"
   /* _mesa_function_pool[12273]: SetFenceNV (will be remapped) */
   "ii\0"
   "glSetFenceNV\0"
   "\0"
   /* _mesa_function_pool[12290]: FramebufferTexture1DEXT (will be remapped) */
   "iiiii\0"
   "glFramebufferTexture1D\0"
   "glFramebufferTexture1DEXT\0"
   "\0"
   /* _mesa_function_pool[12346]: GetCombinerOutputParameterivNV (will be remapped) */
   "iiip\0"
   "glGetCombinerOutputParameterivNV\0"
   "\0"
   /* _mesa_function_pool[12385]: PixelTexGenParameterivSGIS (will be remapped) */
   "ip\0"
   "glPixelTexGenParameterivSGIS\0"
   "\0"
   /* _mesa_function_pool[12418]: TextureNormalEXT (dynamic) */
   "i\0"
   "glTextureNormalEXT\0"
   "\0"
   /* _mesa_function_pool[12440]: IndexPointerListIBM (dynamic) */
   "iipi\0"
   "glIndexPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[12468]: WeightfvARB (dynamic) */
   "ip\0"
   "glWeightfvARB\0"
   "\0"
   /* _mesa_function_pool[12486]: RasterPos2sv (offset 69) */
   "p\0"
   "glRasterPos2sv\0"
   "\0"
   /* _mesa_function_pool[12504]: Color4ubv (offset 36) */
   "p\0"
   "glColor4ubv\0"
   "\0"
   /* _mesa_function_pool[12519]: DrawBuffer (offset 202) */
   "i\0"
   "glDrawBuffer\0"
   "\0"
   /* _mesa_function_pool[12535]: TexCoord2fv (offset 105) */
   "p\0"
   "glTexCoord2fv\0"
   "\0"
   /* _mesa_function_pool[12552]: WindowPos4fMESA (will be remapped) */
   "ffff\0"
   "glWindowPos4fMESA\0"
   "\0"
   /* _mesa_function_pool[12576]: TexCoord1sv (offset 101) */
   "p\0"
   "glTexCoord1sv\0"
   "\0"
   /* _mesa_function_pool[12593]: WindowPos3dvMESA (will be remapped) */
   "p\0"
   "glWindowPos3dv\0"
   "glWindowPos3dvARB\0"
   "glWindowPos3dvMESA\0"
   "\0"
   /* _mesa_function_pool[12648]: DepthFunc (offset 245) */
   "i\0"
   "glDepthFunc\0"
   "\0"
   /* _mesa_function_pool[12663]: PixelMapusv (offset 253) */
   "iip\0"
   "glPixelMapusv\0"
   "\0"
   /* _mesa_function_pool[12682]: GetQueryObjecti64vEXT (will be remapped) */
   "iip\0"
   "glGetQueryObjecti64vEXT\0"
   "\0"
   /* _mesa_function_pool[12711]: MultiTexCoord1dARB (offset 376) */
   "id\0"
   "glMultiTexCoord1d\0"
   "glMultiTexCoord1dARB\0"
   "\0"
   /* _mesa_function_pool[12754]: PointParameterivNV (will be remapped) */
   "ip\0"
   "glPointParameteriv\0"
   "glPointParameterivNV\0"
   "\0"
   /* _mesa_function_pool[12798]: BlendFunc (offset 241) */
   "ii\0"
   "glBlendFunc\0"
   "\0"
   /* _mesa_function_pool[12814]: EndTransformFeedbackEXT (will be remapped) */
   "\0"
   "glEndTransformFeedbackEXT\0"
   "glEndTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[12865]: Uniform2fvARB (will be remapped) */
   "iip\0"
   "glUniform2fv\0"
   "glUniform2fvARB\0"
   "\0"
   /* _mesa_function_pool[12899]: BufferParameteriAPPLE (will be remapped) */
   "iii\0"
   "glBufferParameteriAPPLE\0"
   "\0"
   /* _mesa_function_pool[12928]: MultiTexCoord3dvARB (offset 393) */
   "ip\0"
   "glMultiTexCoord3dv\0"
   "glMultiTexCoord3dvARB\0"
   "\0"
   /* _mesa_function_pool[12973]: ReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN (dynamic) */
   "pppp\0"
   "glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[13029]: DeleteObjectARB (will be remapped) */
   "i\0"
   "glDeleteObjectARB\0"
   "\0"
   /* _mesa_function_pool[13050]: MatrixIndexPointerARB (dynamic) */
   "iiip\0"
   "glMatrixIndexPointerARB\0"
   "\0"
   /* _mesa_function_pool[13080]: ProgramNamedParameter4dvNV (will be remapped) */
   "iipp\0"
   "glProgramNamedParameter4dvNV\0"
   "\0"
   /* _mesa_function_pool[13115]: Tangent3fvEXT (dynamic) */
   "p\0"
   "glTangent3fvEXT\0"
   "\0"
   /* _mesa_function_pool[13134]: Flush (offset 217) */
   "\0"
   "glFlush\0"
   "\0"
   /* _mesa_function_pool[13144]: Color4uiv (offset 38) */
   "p\0"
   "glColor4uiv\0"
   "\0"
   /* _mesa_function_pool[13159]: GenVertexArrays (will be remapped) */
   "ip\0"
   "glGenVertexArrays\0"
   "\0"
   /* _mesa_function_pool[13181]: RasterPos3sv (offset 77) */
   "p\0"
   "glRasterPos3sv\0"
   "\0"
   /* _mesa_function_pool[13199]: BindFramebufferEXT (will be remapped) */
   "ii\0"
   "glBindFramebuffer\0"
   "glBindFramebufferEXT\0"
   "\0"
   /* _mesa_function_pool[13242]: ReferencePlaneSGIX (dynamic) */
   "p\0"
   "glReferencePlaneSGIX\0"
   "\0"
   /* _mesa_function_pool[13266]: PushAttrib (offset 219) */
   "i\0"
   "glPushAttrib\0"
   "\0"
   /* _mesa_function_pool[13282]: RasterPos2i (offset 66) */
   "ii\0"
   "glRasterPos2i\0"
   "\0"
   /* _mesa_function_pool[13300]: ValidateProgramARB (will be remapped) */
   "i\0"
   "glValidateProgram\0"
   "glValidateProgramARB\0"
   "\0"
   /* _mesa_function_pool[13342]: TexParameteriv (offset 181) */
   "iip\0"
   "glTexParameteriv\0"
   "\0"
   /* _mesa_function_pool[13364]: UnlockArraysEXT (will be remapped) */
   "\0"
   "glUnlockArraysEXT\0"
   "\0"
   /* _mesa_function_pool[13384]: TexCoord2fColor3fVertex3fSUN (dynamic) */
   "ffffffff\0"
   "glTexCoord2fColor3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[13425]: WindowPos3fvMESA (will be remapped) */
   "p\0"
   "glWindowPos3fv\0"
   "glWindowPos3fvARB\0"
   "glWindowPos3fvMESA\0"
   "\0"
   /* _mesa_function_pool[13480]: RasterPos2f (offset 64) */
   "ff\0"
   "glRasterPos2f\0"
   "\0"
   /* _mesa_function_pool[13498]: VertexAttrib1svNV (will be remapped) */
   "ip\0"
   "glVertexAttrib1svNV\0"
   "\0"
   /* _mesa_function_pool[13522]: RasterPos2d (offset 62) */
   "dd\0"
   "glRasterPos2d\0"
   "\0"
   /* _mesa_function_pool[13540]: RasterPos3fv (offset 73) */
   "p\0"
   "glRasterPos3fv\0"
   "\0"
   /* _mesa_function_pool[13558]: CopyTexSubImage3D (offset 373) */
   "iiiiiiiii\0"
   "glCopyTexSubImage3D\0"
   "glCopyTexSubImage3DEXT\0"
   "\0"
   /* _mesa_function_pool[13612]: VertexAttrib2dARB (will be remapped) */
   "idd\0"
   "glVertexAttrib2d\0"
   "glVertexAttrib2dARB\0"
   "\0"
   /* _mesa_function_pool[13654]: Color4ub (offset 35) */
   "iiii\0"
   "glColor4ub\0"
   "\0"
   /* _mesa_function_pool[13671]: GetInteger64v (will be remapped) */
   "ip\0"
   "glGetInteger64v\0"
   "\0"
   /* _mesa_function_pool[13691]: TextureColorMaskSGIS (dynamic) */
   "iiii\0"
   "glTextureColorMaskSGIS\0"
   "\0"
   /* _mesa_function_pool[13720]: RasterPos2s (offset 68) */
   "ii\0"
   "glRasterPos2s\0"
   "\0"
   /* _mesa_function_pool[13738]: GetColorTable (offset 343) */
   "iiip\0"
   "glGetColorTable\0"
   "glGetColorTableSGI\0"
   "glGetColorTableEXT\0"
   "\0"
   /* _mesa_function_pool[13798]: SelectBuffer (offset 195) */
   "ip\0"
   "glSelectBuffer\0"
   "\0"
   /* _mesa_function_pool[13817]: Indexiv (offset 49) */
   "p\0"
   "glIndexiv\0"
   "\0"
   /* _mesa_function_pool[13830]: TexCoord3i (offset 114) */
   "iii\0"
   "glTexCoord3i\0"
   "\0"
   /* _mesa_function_pool[13848]: CopyColorTable (offset 342) */
   "iiiii\0"
   "glCopyColorTable\0"
   "glCopyColorTableSGI\0"
   "\0"
   /* _mesa_function_pool[13892]: GetHistogramParameterfv (offset 362) */
   "iip\0"
   "glGetHistogramParameterfv\0"
   "glGetHistogramParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[13952]: Frustum (offset 289) */
   "dddddd\0"
   "glFrustum\0"
   "\0"
   /* _mesa_function_pool[13970]: GetString (offset 275) */
   "i\0"
   "glGetString\0"
   "\0"
   /* _mesa_function_pool[13985]: ColorPointervINTEL (dynamic) */
   "iip\0"
   "glColorPointervINTEL\0"
   "\0"
   /* _mesa_function_pool[14011]: TexEnvf (offset 184) */
   "iif\0"
   "glTexEnvf\0"
   "\0"
   /* _mesa_function_pool[14026]: TexCoord3d (offset 110) */
   "ddd\0"
   "glTexCoord3d\0"
   "\0"
   /* _mesa_function_pool[14044]: AlphaFragmentOp1ATI (will be remapped) */
   "iiiiii\0"
   "glAlphaFragmentOp1ATI\0"
   "\0"
   /* _mesa_function_pool[14074]: TexCoord3f (offset 112) */
   "fff\0"
   "glTexCoord3f\0"
   "\0"
   /* _mesa_function_pool[14092]: MultiTexCoord3ivARB (offset 397) */
   "ip\0"
   "glMultiTexCoord3iv\0"
   "glMultiTexCoord3ivARB\0"
   "\0"
   /* _mesa_function_pool[14137]: MultiTexCoord2sARB (offset 390) */
   "iii\0"
   "glMultiTexCoord2s\0"
   "glMultiTexCoord2sARB\0"
   "\0"
   /* _mesa_function_pool[14181]: VertexAttrib1dvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib1dv\0"
   "glVertexAttrib1dvARB\0"
   "\0"
   /* _mesa_function_pool[14224]: DeleteTextures (offset 327) */
   "ip\0"
   "glDeleteTextures\0"
   "glDeleteTexturesEXT\0"
   "\0"
   /* _mesa_function_pool[14265]: TexCoordPointerEXT (will be remapped) */
   "iiiip\0"
   "glTexCoordPointerEXT\0"
   "\0"
   /* _mesa_function_pool[14293]: TexSubImage4DSGIS (dynamic) */
   "iiiiiiiiiiiip\0"
   "glTexSubImage4DSGIS\0"
   "\0"
   /* _mesa_function_pool[14328]: TexCoord3s (offset 116) */
   "iii\0"
   "glTexCoord3s\0"
   "\0"
   /* _mesa_function_pool[14346]: GetTexLevelParameteriv (offset 285) */
   "iiip\0"
   "glGetTexLevelParameteriv\0"
   "\0"
   /* _mesa_function_pool[14377]: CombinerStageParameterfvNV (dynamic) */
   "iip\0"
   "glCombinerStageParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[14411]: StopInstrumentsSGIX (dynamic) */
   "i\0"
   "glStopInstrumentsSGIX\0"
   "\0"
   /* _mesa_function_pool[14436]: TexCoord4fColor4fNormal3fVertex4fSUN (dynamic) */
   "fffffffffffffff\0"
   "glTexCoord4fColor4fNormal3fVertex4fSUN\0"
   "\0"
   /* _mesa_function_pool[14492]: ClearAccum (offset 204) */
   "ffff\0"
   "glClearAccum\0"
   "\0"
   /* _mesa_function_pool[14511]: DeformSGIX (dynamic) */
   "i\0"
   "glDeformSGIX\0"
   "\0"
   /* _mesa_function_pool[14527]: GetVertexAttribfvARB (will be remapped) */
   "iip\0"
   "glGetVertexAttribfv\0"
   "glGetVertexAttribfvARB\0"
   "\0"
   /* _mesa_function_pool[14575]: SecondaryColor3ivEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3iv\0"
   "glSecondaryColor3ivEXT\0"
   "\0"
   /* _mesa_function_pool[14621]: TexCoord4iv (offset 123) */
   "p\0"
   "glTexCoord4iv\0"
   "\0"
   /* _mesa_function_pool[14638]: UniformMatrix4x2fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix4x2fv\0"
   "\0"
   /* _mesa_function_pool[14665]: GetDetailTexFuncSGIS (dynamic) */
   "ip\0"
   "glGetDetailTexFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[14692]: GetCombinerStageParameterfvNV (dynamic) */
   "iip\0"
   "glGetCombinerStageParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[14729]: PolygonOffset (offset 319) */
   "ff\0"
   "glPolygonOffset\0"
   "\0"
   /* _mesa_function_pool[14749]: BindVertexArray (will be remapped) */
   "i\0"
   "glBindVertexArray\0"
   "\0"
   /* _mesa_function_pool[14770]: Color4ubVertex2fvSUN (dynamic) */
   "pp\0"
   "glColor4ubVertex2fvSUN\0"
   "\0"
   /* _mesa_function_pool[14797]: Rectd (offset 86) */
   "dddd\0"
   "glRectd\0"
   "\0"
   /* _mesa_function_pool[14811]: TexFilterFuncSGIS (dynamic) */
   "iiip\0"
   "glTexFilterFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[14837]: SampleMaskSGIS (will be remapped) */
   "fi\0"
   "glSampleMaskSGIS\0"
   "glSampleMaskEXT\0"
   "\0"
   /* _mesa_function_pool[14874]: GetAttribLocationARB (will be remapped) */
   "ip\0"
   "glGetAttribLocation\0"
   "glGetAttribLocationARB\0"
   "\0"
   /* _mesa_function_pool[14921]: RasterPos3i (offset 74) */
   "iii\0"
   "glRasterPos3i\0"
   "\0"
   /* _mesa_function_pool[14940]: VertexAttrib4ubvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4ubv\0"
   "glVertexAttrib4ubvARB\0"
   "\0"
   /* _mesa_function_pool[14985]: DetailTexFuncSGIS (dynamic) */
   "iip\0"
   "glDetailTexFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[15010]: Normal3fVertex3fSUN (dynamic) */
   "ffffff\0"
   "glNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[15040]: CopyTexImage2D (offset 324) */
   "iiiiiiii\0"
   "glCopyTexImage2D\0"
   "glCopyTexImage2DEXT\0"
   "\0"
   /* _mesa_function_pool[15087]: GetBufferPointervARB (will be remapped) */
   "iip\0"
   "glGetBufferPointerv\0"
   "glGetBufferPointervARB\0"
   "\0"
   /* _mesa_function_pool[15135]: ProgramEnvParameter4fARB (will be remapped) */
   "iiffff\0"
   "glProgramEnvParameter4fARB\0"
   "glProgramParameter4fNV\0"
   "\0"
   /* _mesa_function_pool[15193]: Uniform3ivARB (will be remapped) */
   "iip\0"
   "glUniform3iv\0"
   "glUniform3ivARB\0"
   "\0"
   /* _mesa_function_pool[15227]: Lightfv (offset 160) */
   "iip\0"
   "glLightfv\0"
   "\0"
   /* _mesa_function_pool[15242]: ClearDepth (offset 208) */
   "d\0"
   "glClearDepth\0"
   "\0"
   /* _mesa_function_pool[15258]: GetFenceivNV (will be remapped) */
   "iip\0"
   "glGetFenceivNV\0"
   "\0"
   /* _mesa_function_pool[15278]: WindowPos4dvMESA (will be remapped) */
   "p\0"
   "glWindowPos4dvMESA\0"
   "\0"
   /* _mesa_function_pool[15300]: ColorSubTable (offset 346) */
   "iiiiip\0"
   "glColorSubTable\0"
   "glColorSubTableEXT\0"
   "\0"
   /* _mesa_function_pool[15343]: Color4fv (offset 30) */
   "p\0"
   "glColor4fv\0"
   "\0"
   /* _mesa_function_pool[15357]: MultiTexCoord4ivARB (offset 405) */
   "ip\0"
   "glMultiTexCoord4iv\0"
   "glMultiTexCoord4ivARB\0"
   "\0"
   /* _mesa_function_pool[15402]: ProgramLocalParameters4fvEXT (will be remapped) */
   "iiip\0"
   "glProgramLocalParameters4fvEXT\0"
   "\0"
   /* _mesa_function_pool[15439]: ColorPointer (offset 308) */
   "iiip\0"
   "glColorPointer\0"
   "\0"
   /* _mesa_function_pool[15460]: Rects (offset 92) */
   "iiii\0"
   "glRects\0"
   "\0"
   /* _mesa_function_pool[15474]: GetMapAttribParameterfvNV (dynamic) */
   "iiip\0"
   "glGetMapAttribParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[15508]: Lightiv (offset 162) */
   "iip\0"
   "glLightiv\0"
   "\0"
   /* _mesa_function_pool[15523]: VertexAttrib4sARB (will be remapped) */
   "iiiii\0"
   "glVertexAttrib4s\0"
   "glVertexAttrib4sARB\0"
   "\0"
   /* _mesa_function_pool[15567]: GetQueryObjectuivARB (will be remapped) */
   "iip\0"
   "glGetQueryObjectuiv\0"
   "glGetQueryObjectuivARB\0"
   "\0"
   /* _mesa_function_pool[15615]: GetTexParameteriv (offset 283) */
   "iip\0"
   "glGetTexParameteriv\0"
   "\0"
   /* _mesa_function_pool[15640]: MapParameterivNV (dynamic) */
   "iip\0"
   "glMapParameterivNV\0"
   "\0"
   /* _mesa_function_pool[15664]: GenRenderbuffersEXT (will be remapped) */
   "ip\0"
   "glGenRenderbuffers\0"
   "glGenRenderbuffersEXT\0"
   "\0"
   /* _mesa_function_pool[15709]: VertexAttrib2dvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib2dv\0"
   "glVertexAttrib2dvARB\0"
   "\0"
   /* _mesa_function_pool[15752]: EdgeFlagPointerEXT (will be remapped) */
   "iip\0"
   "glEdgeFlagPointerEXT\0"
   "\0"
   /* _mesa_function_pool[15778]: VertexAttribs2svNV (will be remapped) */
   "iip\0"
   "glVertexAttribs2svNV\0"
   "\0"
   /* _mesa_function_pool[15804]: WeightbvARB (dynamic) */
   "ip\0"
   "glWeightbvARB\0"
   "\0"
   /* _mesa_function_pool[15822]: VertexAttrib2fvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib2fv\0"
   "glVertexAttrib2fvARB\0"
   "\0"
   /* _mesa_function_pool[15865]: GetBufferParameterivARB (will be remapped) */
   "iip\0"
   "glGetBufferParameteriv\0"
   "glGetBufferParameterivARB\0"
   "\0"
   /* _mesa_function_pool[15919]: Rectdv (offset 87) */
   "pp\0"
   "glRectdv\0"
   "\0"
   /* _mesa_function_pool[15932]: ListParameteriSGIX (dynamic) */
   "iii\0"
   "glListParameteriSGIX\0"
   "\0"
   /* _mesa_function_pool[15958]: ReplacementCodeuiColor4fNormal3fVertex3fSUN (dynamic) */
   "iffffffffff\0"
   "glReplacementCodeuiColor4fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[16017]: InstrumentsBufferSGIX (dynamic) */
   "ip\0"
   "glInstrumentsBufferSGIX\0"
   "\0"
   /* _mesa_function_pool[16045]: VertexAttrib4NivARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Niv\0"
   "glVertexAttrib4NivARB\0"
   "\0"
   /* _mesa_function_pool[16090]: GetAttachedShaders (will be remapped) */
   "iipp\0"
   "glGetAttachedShaders\0"
   "\0"
   /* _mesa_function_pool[16117]: GenVertexArraysAPPLE (will be remapped) */
   "ip\0"
   "glGenVertexArraysAPPLE\0"
   "\0"
   /* _mesa_function_pool[16144]: Materialiv (offset 172) */
   "iip\0"
   "glMaterialiv\0"
   "\0"
   /* _mesa_function_pool[16162]: PushClientAttrib (offset 335) */
   "i\0"
   "glPushClientAttrib\0"
   "\0"
   /* _mesa_function_pool[16184]: ProgramEnvParameters4fvEXT (will be remapped) */
   "iiip\0"
   "glProgramEnvParameters4fvEXT\0"
   "\0"
   /* _mesa_function_pool[16219]: TexCoord2fColor4fNormal3fVertex3fvSUN (dynamic) */
   "pppp\0"
   "glTexCoord2fColor4fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[16265]: WindowPos2iMESA (will be remapped) */
   "ii\0"
   "glWindowPos2i\0"
   "glWindowPos2iARB\0"
   "glWindowPos2iMESA\0"
   "\0"
   /* _mesa_function_pool[16318]: SecondaryColor3fvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3fv\0"
   "glSecondaryColor3fvEXT\0"
   "\0"
   /* _mesa_function_pool[16364]: PolygonMode (offset 174) */
   "ii\0"
   "glPolygonMode\0"
   "\0"
   /* _mesa_function_pool[16382]: CompressedTexSubImage1DARB (will be remapped) */
   "iiiiiip\0"
   "glCompressedTexSubImage1D\0"
   "glCompressedTexSubImage1DARB\0"
   "\0"
   /* _mesa_function_pool[16446]: GetVertexAttribivNV (will be remapped) */
   "iip\0"
   "glGetVertexAttribivNV\0"
   "\0"
   /* _mesa_function_pool[16473]: GetProgramStringARB (will be remapped) */
   "iip\0"
   "glGetProgramStringARB\0"
   "\0"
   /* _mesa_function_pool[16500]: TexBumpParameterfvATI (will be remapped) */
   "ip\0"
   "glTexBumpParameterfvATI\0"
   "\0"
   /* _mesa_function_pool[16528]: CompileShaderARB (will be remapped) */
   "i\0"
   "glCompileShader\0"
   "glCompileShaderARB\0"
   "\0"
   /* _mesa_function_pool[16566]: DeleteShader (will be remapped) */
   "i\0"
   "glDeleteShader\0"
   "\0"
   /* _mesa_function_pool[16584]: DisableClientState (offset 309) */
   "i\0"
   "glDisableClientState\0"
   "\0"
   /* _mesa_function_pool[16608]: TexGeni (offset 192) */
   "iii\0"
   "glTexGeni\0"
   "\0"
   /* _mesa_function_pool[16623]: TexGenf (offset 190) */
   "iif\0"
   "glTexGenf\0"
   "\0"
   /* _mesa_function_pool[16638]: Uniform3fARB (will be remapped) */
   "ifff\0"
   "glUniform3f\0"
   "glUniform3fARB\0"
   "\0"
   /* _mesa_function_pool[16671]: TexGend (offset 188) */
   "iid\0"
   "glTexGend\0"
   "\0"
   /* _mesa_function_pool[16686]: ListParameterfvSGIX (dynamic) */
   "iip\0"
   "glListParameterfvSGIX\0"
   "\0"
   /* _mesa_function_pool[16713]: GetPolygonStipple (offset 274) */
   "p\0"
   "glGetPolygonStipple\0"
   "\0"
   /* _mesa_function_pool[16736]: Tangent3dvEXT (dynamic) */
   "p\0"
   "glTangent3dvEXT\0"
   "\0"
   /* _mesa_function_pool[16755]: BindBufferOffsetEXT (will be remapped) */
   "iiii\0"
   "glBindBufferOffsetEXT\0"
   "\0"
   /* _mesa_function_pool[16783]: WindowPos3sMESA (will be remapped) */
   "iii\0"
   "glWindowPos3s\0"
   "glWindowPos3sARB\0"
   "glWindowPos3sMESA\0"
   "\0"
   /* _mesa_function_pool[16837]: VertexAttrib2svNV (will be remapped) */
   "ip\0"
   "glVertexAttrib2svNV\0"
   "\0"
   /* _mesa_function_pool[16861]: DisableIndexedEXT (will be remapped) */
   "ii\0"
   "glDisableIndexedEXT\0"
   "\0"
   /* _mesa_function_pool[16885]: BindBufferBaseEXT (will be remapped) */
   "iii\0"
   "glBindBufferBaseEXT\0"
   "glBindBufferBase\0"
   "\0"
   /* _mesa_function_pool[16927]: TexCoord2fVertex3fvSUN (dynamic) */
   "pp\0"
   "glTexCoord2fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[16956]: WindowPos4sMESA (will be remapped) */
   "iiii\0"
   "glWindowPos4sMESA\0"
   "\0"
   /* _mesa_function_pool[16980]: VertexAttrib4NuivARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Nuiv\0"
   "glVertexAttrib4NuivARB\0"
   "\0"
   /* _mesa_function_pool[17027]: ClientActiveTextureARB (offset 375) */
   "i\0"
   "glClientActiveTexture\0"
   "glClientActiveTextureARB\0"
   "\0"
   /* _mesa_function_pool[17077]: PixelTexGenSGIX (will be remapped) */
   "i\0"
   "glPixelTexGenSGIX\0"
   "\0"
   /* _mesa_function_pool[17098]: ReplacementCodeusvSUN (dynamic) */
   "p\0"
   "glReplacementCodeusvSUN\0"
   "\0"
   /* _mesa_function_pool[17125]: Uniform4fARB (will be remapped) */
   "iffff\0"
   "glUniform4f\0"
   "glUniform4fARB\0"
   "\0"
   /* _mesa_function_pool[17159]: Color4sv (offset 34) */
   "p\0"
   "glColor4sv\0"
   "\0"
   /* _mesa_function_pool[17173]: FlushMappedBufferRange (will be remapped) */
   "iii\0"
   "glFlushMappedBufferRange\0"
   "\0"
   /* _mesa_function_pool[17203]: IsProgramNV (will be remapped) */
   "i\0"
   "glIsProgramARB\0"
   "glIsProgramNV\0"
   "\0"
   /* _mesa_function_pool[17235]: FlushMappedBufferRangeAPPLE (will be remapped) */
   "iii\0"
   "glFlushMappedBufferRangeAPPLE\0"
   "\0"
   /* _mesa_function_pool[17270]: PixelZoom (offset 246) */
   "ff\0"
   "glPixelZoom\0"
   "\0"
   /* _mesa_function_pool[17286]: ReplacementCodePointerSUN (dynamic) */
   "iip\0"
   "glReplacementCodePointerSUN\0"
   "\0"
   /* _mesa_function_pool[17319]: ProgramEnvParameter4dARB (will be remapped) */
   "iidddd\0"
   "glProgramEnvParameter4dARB\0"
   "glProgramParameter4dNV\0"
   "\0"
   /* _mesa_function_pool[17377]: ColorTableParameterfv (offset 340) */
   "iip\0"
   "glColorTableParameterfv\0"
   "glColorTableParameterfvSGI\0"
   "\0"
   /* _mesa_function_pool[17433]: FragmentLightModelfSGIX (dynamic) */
   "if\0"
   "glFragmentLightModelfSGIX\0"
   "\0"
   /* _mesa_function_pool[17463]: Binormal3bvEXT (dynamic) */
   "p\0"
   "glBinormal3bvEXT\0"
   "\0"
   /* _mesa_function_pool[17483]: PixelMapuiv (offset 252) */
   "iip\0"
   "glPixelMapuiv\0"
   "\0"
   /* _mesa_function_pool[17502]: Color3dv (offset 12) */
   "p\0"
   "glColor3dv\0"
   "\0"
   /* _mesa_function_pool[17516]: IsTexture (offset 330) */
   "i\0"
   "glIsTexture\0"
   "glIsTextureEXT\0"
   "\0"
   /* _mesa_function_pool[17546]: VertexWeightfvEXT (dynamic) */
   "p\0"
   "glVertexWeightfvEXT\0"
   "\0"
   /* _mesa_function_pool[17569]: VertexAttrib1dARB (will be remapped) */
   "id\0"
   "glVertexAttrib1d\0"
   "glVertexAttrib1dARB\0"
   "\0"
   /* _mesa_function_pool[17610]: ImageTransformParameterivHP (dynamic) */
   "iip\0"
   "glImageTransformParameterivHP\0"
   "\0"
   /* _mesa_function_pool[17645]: TexCoord4i (offset 122) */
   "iiii\0"
   "glTexCoord4i\0"
   "\0"
   /* _mesa_function_pool[17664]: DeleteQueriesARB (will be remapped) */
   "ip\0"
   "glDeleteQueries\0"
   "glDeleteQueriesARB\0"
   "\0"
   /* _mesa_function_pool[17703]: Color4ubVertex2fSUN (dynamic) */
   "iiiiff\0"
   "glColor4ubVertex2fSUN\0"
   "\0"
   /* _mesa_function_pool[17733]: FragmentColorMaterialSGIX (dynamic) */
   "ii\0"
   "glFragmentColorMaterialSGIX\0"
   "\0"
   /* _mesa_function_pool[17765]: CurrentPaletteMatrixARB (dynamic) */
   "i\0"
   "glCurrentPaletteMatrixARB\0"
   "\0"
   /* _mesa_function_pool[17794]: GetMapdv (offset 266) */
   "iip\0"
   "glGetMapdv\0"
   "\0"
   /* _mesa_function_pool[17810]: ObjectPurgeableAPPLE (will be remapped) */
   "iii\0"
   "glObjectPurgeableAPPLE\0"
   "\0"
   /* _mesa_function_pool[17838]: SamplePatternSGIS (will be remapped) */
   "i\0"
   "glSamplePatternSGIS\0"
   "glSamplePatternEXT\0"
   "\0"
   /* _mesa_function_pool[17880]: PixelStoref (offset 249) */
   "if\0"
   "glPixelStoref\0"
   "\0"
   /* _mesa_function_pool[17898]: IsQueryARB (will be remapped) */
   "i\0"
   "glIsQuery\0"
   "glIsQueryARB\0"
   "\0"
   /* _mesa_function_pool[17924]: ReplacementCodeuiColor4ubVertex3fSUN (dynamic) */
   "iiiiifff\0"
   "glReplacementCodeuiColor4ubVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[17973]: PixelStorei (offset 250) */
   "ii\0"
   "glPixelStorei\0"
   "\0"
   /* _mesa_function_pool[17991]: VertexAttrib4usvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4usv\0"
   "glVertexAttrib4usvARB\0"
   "\0"
   /* _mesa_function_pool[18036]: LinkProgramARB (will be remapped) */
   "i\0"
   "glLinkProgram\0"
   "glLinkProgramARB\0"
   "\0"
   /* _mesa_function_pool[18070]: VertexAttrib2fNV (will be remapped) */
   "iff\0"
   "glVertexAttrib2fNV\0"
   "\0"
   /* _mesa_function_pool[18094]: ShaderSourceARB (will be remapped) */
   "iipp\0"
   "glShaderSource\0"
   "glShaderSourceARB\0"
   "\0"
   /* _mesa_function_pool[18133]: FragmentMaterialiSGIX (dynamic) */
   "iii\0"
   "glFragmentMaterialiSGIX\0"
   "\0"
   /* _mesa_function_pool[18162]: EvalCoord2dv (offset 233) */
   "p\0"
   "glEvalCoord2dv\0"
   "\0"
   /* _mesa_function_pool[18180]: VertexAttrib3svARB (will be remapped) */
   "ip\0"
   "glVertexAttrib3sv\0"
   "glVertexAttrib3svARB\0"
   "\0"
   /* _mesa_function_pool[18223]: ColorMaterial (offset 151) */
   "ii\0"
   "glColorMaterial\0"
   "\0"
   /* _mesa_function_pool[18243]: CompressedTexSubImage3DARB (will be remapped) */
   "iiiiiiiiiip\0"
   "glCompressedTexSubImage3D\0"
   "glCompressedTexSubImage3DARB\0"
   "\0"
   /* _mesa_function_pool[18311]: WindowPos2ivMESA (will be remapped) */
   "p\0"
   "glWindowPos2iv\0"
   "glWindowPos2ivARB\0"
   "glWindowPos2ivMESA\0"
   "\0"
   /* _mesa_function_pool[18366]: IsFramebufferEXT (will be remapped) */
   "i\0"
   "glIsFramebuffer\0"
   "glIsFramebufferEXT\0"
   "\0"
   /* _mesa_function_pool[18404]: Uniform4ivARB (will be remapped) */
   "iip\0"
   "glUniform4iv\0"
   "glUniform4ivARB\0"
   "\0"
   /* _mesa_function_pool[18438]: GetVertexAttribdvARB (will be remapped) */
   "iip\0"
   "glGetVertexAttribdv\0"
   "glGetVertexAttribdvARB\0"
   "\0"
   /* _mesa_function_pool[18486]: TexBumpParameterivATI (will be remapped) */
   "ip\0"
   "glTexBumpParameterivATI\0"
   "\0"
   /* _mesa_function_pool[18514]: GetSeparableFilter (offset 359) */
   "iiippp\0"
   "glGetSeparableFilter\0"
   "glGetSeparableFilterEXT\0"
   "\0"
   /* _mesa_function_pool[18567]: Binormal3dEXT (dynamic) */
   "ddd\0"
   "glBinormal3dEXT\0"
   "\0"
   /* _mesa_function_pool[18588]: SpriteParameteriSGIX (dynamic) */
   "ii\0"
   "glSpriteParameteriSGIX\0"
   "\0"
   /* _mesa_function_pool[18615]: RequestResidentProgramsNV (will be remapped) */
   "ip\0"
   "glRequestResidentProgramsNV\0"
   "\0"
   /* _mesa_function_pool[18647]: TagSampleBufferSGIX (dynamic) */
   "\0"
   "glTagSampleBufferSGIX\0"
   "\0"
   /* _mesa_function_pool[18671]: TransformFeedbackVaryingsEXT (will be remapped) */
   "iipi\0"
   "glTransformFeedbackVaryingsEXT\0"
   "glTransformFeedbackVaryings\0"
   "\0"
   /* _mesa_function_pool[18736]: FeedbackBuffer (offset 194) */
   "iip\0"
   "glFeedbackBuffer\0"
   "\0"
   /* _mesa_function_pool[18758]: RasterPos2iv (offset 67) */
   "p\0"
   "glRasterPos2iv\0"
   "\0"
   /* _mesa_function_pool[18776]: TexImage1D (offset 182) */
   "iiiiiiip\0"
   "glTexImage1D\0"
   "\0"
   /* _mesa_function_pool[18799]: ListParameterivSGIX (dynamic) */
   "iip\0"
   "glListParameterivSGIX\0"
   "\0"
   /* _mesa_function_pool[18826]: MultiDrawElementsEXT (will be remapped) */
   "ipipi\0"
   "glMultiDrawElements\0"
   "glMultiDrawElementsEXT\0"
   "\0"
   /* _mesa_function_pool[18876]: Color3s (offset 17) */
   "iii\0"
   "glColor3s\0"
   "\0"
   /* _mesa_function_pool[18891]: Uniform1ivARB (will be remapped) */
   "iip\0"
   "glUniform1iv\0"
   "glUniform1ivARB\0"
   "\0"
   /* _mesa_function_pool[18925]: WindowPos2sMESA (will be remapped) */
   "ii\0"
   "glWindowPos2s\0"
   "glWindowPos2sARB\0"
   "glWindowPos2sMESA\0"
   "\0"
   /* _mesa_function_pool[18978]: WeightusvARB (dynamic) */
   "ip\0"
   "glWeightusvARB\0"
   "\0"
   /* _mesa_function_pool[18997]: TexCoordPointer (offset 320) */
   "iiip\0"
   "glTexCoordPointer\0"
   "\0"
   /* _mesa_function_pool[19021]: FogCoordPointerEXT (will be remapped) */
   "iip\0"
   "glFogCoordPointer\0"
   "glFogCoordPointerEXT\0"
   "\0"
   /* _mesa_function_pool[19065]: IndexMaterialEXT (dynamic) */
   "ii\0"
   "glIndexMaterialEXT\0"
   "\0"
   /* _mesa_function_pool[19088]: Color3i (offset 15) */
   "iii\0"
   "glColor3i\0"
   "\0"
   /* _mesa_function_pool[19103]: FrontFace (offset 157) */
   "i\0"
   "glFrontFace\0"
   "\0"
   /* _mesa_function_pool[19118]: EvalCoord2d (offset 232) */
   "dd\0"
   "glEvalCoord2d\0"
   "\0"
   /* _mesa_function_pool[19136]: SecondaryColor3ubvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3ubv\0"
   "glSecondaryColor3ubvEXT\0"
   "\0"
   /* _mesa_function_pool[19184]: EvalCoord2f (offset 234) */
   "ff\0"
   "glEvalCoord2f\0"
   "\0"
   /* _mesa_function_pool[19202]: VertexAttrib4dvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4dv\0"
   "glVertexAttrib4dvARB\0"
   "\0"
   /* _mesa_function_pool[19245]: BindAttribLocationARB (will be remapped) */
   "iip\0"
   "glBindAttribLocation\0"
   "glBindAttribLocationARB\0"
   "\0"
   /* _mesa_function_pool[19295]: Color3b (offset 9) */
   "iii\0"
   "glColor3b\0"
   "\0"
   /* _mesa_function_pool[19310]: MultiTexCoord2dARB (offset 384) */
   "idd\0"
   "glMultiTexCoord2d\0"
   "glMultiTexCoord2dARB\0"
   "\0"
   /* _mesa_function_pool[19354]: ExecuteProgramNV (will be remapped) */
   "iip\0"
   "glExecuteProgramNV\0"
   "\0"
   /* _mesa_function_pool[19378]: Color3f (offset 13) */
   "fff\0"
   "glColor3f\0"
   "\0"
   /* _mesa_function_pool[19393]: LightEnviSGIX (dynamic) */
   "ii\0"
   "glLightEnviSGIX\0"
   "\0"
   /* _mesa_function_pool[19413]: Color3d (offset 11) */
   "ddd\0"
   "glColor3d\0"
   "\0"
   /* _mesa_function_pool[19428]: Normal3dv (offset 55) */
   "p\0"
   "glNormal3dv\0"
   "\0"
   /* _mesa_function_pool[19443]: Lightf (offset 159) */
   "iif\0"
   "glLightf\0"
   "\0"
   /* _mesa_function_pool[19457]: ReplacementCodeuiSUN (dynamic) */
   "i\0"
   "glReplacementCodeuiSUN\0"
   "\0"
   /* _mesa_function_pool[19483]: MatrixMode (offset 293) */
   "i\0"
   "glMatrixMode\0"
   "\0"
   /* _mesa_function_pool[19499]: GetPixelMapusv (offset 273) */
   "ip\0"
   "glGetPixelMapusv\0"
   "\0"
   /* _mesa_function_pool[19520]: Lighti (offset 161) */
   "iii\0"
   "glLighti\0"
   "\0"
   /* _mesa_function_pool[19534]: VertexAttribPointerNV (will be remapped) */
   "iiiip\0"
   "glVertexAttribPointerNV\0"
   "\0"
   /* _mesa_function_pool[19565]: GetBooleanIndexedvEXT (will be remapped) */
   "iip\0"
   "glGetBooleanIndexedvEXT\0"
   "\0"
   /* _mesa_function_pool[19594]: GetFramebufferAttachmentParameterivEXT (will be remapped) */
   "iiip\0"
   "glGetFramebufferAttachmentParameteriv\0"
   "glGetFramebufferAttachmentParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[19679]: PixelTransformParameterfEXT (dynamic) */
   "iif\0"
   "glPixelTransformParameterfEXT\0"
   "\0"
   /* _mesa_function_pool[19714]: MultiTexCoord4dvARB (offset 401) */
   "ip\0"
   "glMultiTexCoord4dv\0"
   "glMultiTexCoord4dvARB\0"
   "\0"
   /* _mesa_function_pool[19759]: PixelTransformParameteriEXT (dynamic) */
   "iii\0"
   "glPixelTransformParameteriEXT\0"
   "\0"
   /* _mesa_function_pool[19794]: GetDoublev (offset 260) */
   "ip\0"
   "glGetDoublev\0"
   "\0"
   /* _mesa_function_pool[19811]: MultMatrixd (offset 295) */
   "p\0"
   "glMultMatrixd\0"
   "\0"
   /* _mesa_function_pool[19828]: MultMatrixf (offset 294) */
   "p\0"
   "glMultMatrixf\0"
   "\0"
   /* _mesa_function_pool[19845]: TexCoord2fColor4ubVertex3fSUN (dynamic) */
   "ffiiiifff\0"
   "glTexCoord2fColor4ubVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[19888]: Uniform1iARB (will be remapped) */
   "ii\0"
   "glUniform1i\0"
   "glUniform1iARB\0"
   "\0"
   /* _mesa_function_pool[19919]: VertexAttribPointerARB (will be remapped) */
   "iiiiip\0"
   "glVertexAttribPointer\0"
   "glVertexAttribPointerARB\0"
   "\0"
   /* _mesa_function_pool[19974]: SharpenTexFuncSGIS (dynamic) */
   "iip\0"
   "glSharpenTexFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[20000]: MultiTexCoord4fvARB (offset 403) */
   "ip\0"
   "glMultiTexCoord4fv\0"
   "glMultiTexCoord4fvARB\0"
   "\0"
   /* _mesa_function_pool[20045]: UniformMatrix2x3fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix2x3fv\0"
   "\0"
   /* _mesa_function_pool[20072]: TrackMatrixNV (will be remapped) */
   "iiii\0"
   "glTrackMatrixNV\0"
   "\0"
   /* _mesa_function_pool[20094]: CombinerParameteriNV (will be remapped) */
   "ii\0"
   "glCombinerParameteriNV\0"
   "\0"
   /* _mesa_function_pool[20121]: DeleteAsyncMarkersSGIX (dynamic) */
   "ii\0"
   "glDeleteAsyncMarkersSGIX\0"
   "\0"
   /* _mesa_function_pool[20150]: ReplacementCodeusSUN (dynamic) */
   "i\0"
   "glReplacementCodeusSUN\0"
   "\0"
   /* _mesa_function_pool[20176]: IsAsyncMarkerSGIX (dynamic) */
   "i\0"
   "glIsAsyncMarkerSGIX\0"
   "\0"
   /* _mesa_function_pool[20199]: FrameZoomSGIX (dynamic) */
   "i\0"
   "glFrameZoomSGIX\0"
   "\0"
   /* _mesa_function_pool[20218]: Normal3fVertex3fvSUN (dynamic) */
   "pp\0"
   "glNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[20245]: RasterPos4sv (offset 85) */
   "p\0"
   "glRasterPos4sv\0"
   "\0"
   /* _mesa_function_pool[20263]: VertexAttrib4NsvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Nsv\0"
   "glVertexAttrib4NsvARB\0"
   "\0"
   /* _mesa_function_pool[20308]: VertexAttrib3fvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib3fv\0"
   "glVertexAttrib3fvARB\0"
   "\0"
   /* _mesa_function_pool[20351]: ClearColor (offset 206) */
   "ffff\0"
   "glClearColor\0"
   "\0"
   /* _mesa_function_pool[20370]: GetSynciv (will be remapped) */
   "iiipp\0"
   "glGetSynciv\0"
   "\0"
   /* _mesa_function_pool[20389]: DeleteFramebuffersEXT (will be remapped) */
   "ip\0"
   "glDeleteFramebuffers\0"
   "glDeleteFramebuffersEXT\0"
   "\0"
   /* _mesa_function_pool[20438]: GlobalAlphaFactorsSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactorsSUN\0"
   "\0"
   /* _mesa_function_pool[20465]: IsEnabledIndexedEXT (will be remapped) */
   "ii\0"
   "glIsEnabledIndexedEXT\0"
   "\0"
   /* _mesa_function_pool[20491]: TexEnviv (offset 187) */
   "iip\0"
   "glTexEnviv\0"
   "\0"
   /* _mesa_function_pool[20507]: TexSubImage3D (offset 372) */
   "iiiiiiiiiip\0"
   "glTexSubImage3D\0"
   "glTexSubImage3DEXT\0"
   "\0"
   /* _mesa_function_pool[20555]: Tangent3fEXT (dynamic) */
   "fff\0"
   "glTangent3fEXT\0"
   "\0"
   /* _mesa_function_pool[20575]: SecondaryColor3uivEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3uiv\0"
   "glSecondaryColor3uivEXT\0"
   "\0"
   /* _mesa_function_pool[20623]: MatrixIndexubvARB (dynamic) */
   "ip\0"
   "glMatrixIndexubvARB\0"
   "\0"
   /* _mesa_function_pool[20647]: Color4fNormal3fVertex3fSUN (dynamic) */
   "ffffffffff\0"
   "glColor4fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[20688]: PixelTexGenParameterfSGIS (will be remapped) */
   "if\0"
   "glPixelTexGenParameterfSGIS\0"
   "\0"
   /* _mesa_function_pool[20720]: CreateShader (will be remapped) */
   "i\0"
   "glCreateShader\0"
   "\0"
   /* _mesa_function_pool[20738]: GetColorTableParameterfv (offset 344) */
   "iip\0"
   "glGetColorTableParameterfv\0"
   "glGetColorTableParameterfvSGI\0"
   "glGetColorTableParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[20830]: FragmentLightModelfvSGIX (dynamic) */
   "ip\0"
   "glFragmentLightModelfvSGIX\0"
   "\0"
   /* _mesa_function_pool[20861]: Bitmap (offset 8) */
   "iiffffp\0"
   "glBitmap\0"
   "\0"
   /* _mesa_function_pool[20879]: MultiTexCoord3fARB (offset 394) */
   "ifff\0"
   "glMultiTexCoord3f\0"
   "glMultiTexCoord3fARB\0"
   "\0"
   /* _mesa_function_pool[20924]: GetTexLevelParameterfv (offset 284) */
   "iiip\0"
   "glGetTexLevelParameterfv\0"
   "\0"
   /* _mesa_function_pool[20955]: GetPixelTexGenParameterfvSGIS (will be remapped) */
   "ip\0"
   "glGetPixelTexGenParameterfvSGIS\0"
   "\0"
   /* _mesa_function_pool[20991]: GenFramebuffersEXT (will be remapped) */
   "ip\0"
   "glGenFramebuffers\0"
   "glGenFramebuffersEXT\0"
   "\0"
   /* _mesa_function_pool[21034]: GetProgramParameterdvNV (will be remapped) */
   "iiip\0"
   "glGetProgramParameterdvNV\0"
   "\0"
   /* _mesa_function_pool[21066]: Vertex2sv (offset 133) */
   "p\0"
   "glVertex2sv\0"
   "\0"
   /* _mesa_function_pool[21081]: GetIntegerv (offset 263) */
   "ip\0"
   "glGetIntegerv\0"
   "\0"
   /* _mesa_function_pool[21099]: IsVertexArrayAPPLE (will be remapped) */
   "i\0"
   "glIsVertexArray\0"
   "glIsVertexArrayAPPLE\0"
   "\0"
   /* _mesa_function_pool[21139]: FragmentLightfvSGIX (dynamic) */
   "iip\0"
   "glFragmentLightfvSGIX\0"
   "\0"
   /* _mesa_function_pool[21166]: DetachShader (will be remapped) */
   "ii\0"
   "glDetachShader\0"
   "\0"
   /* _mesa_function_pool[21185]: VertexAttrib4NubARB (will be remapped) */
   "iiiii\0"
   "glVertexAttrib4Nub\0"
   "glVertexAttrib4NubARB\0"
   "\0"
   /* _mesa_function_pool[21233]: GetProgramEnvParameterfvARB (will be remapped) */
   "iip\0"
   "glGetProgramEnvParameterfvARB\0"
   "\0"
   /* _mesa_function_pool[21268]: GetTrackMatrixivNV (will be remapped) */
   "iiip\0"
   "glGetTrackMatrixivNV\0"
   "\0"
   /* _mesa_function_pool[21295]: VertexAttrib3svNV (will be remapped) */
   "ip\0"
   "glVertexAttrib3svNV\0"
   "\0"
   /* _mesa_function_pool[21319]: Uniform4fvARB (will be remapped) */
   "iip\0"
   "glUniform4fv\0"
   "glUniform4fvARB\0"
   "\0"
   /* _mesa_function_pool[21353]: MultTransposeMatrixfARB (will be remapped) */
   "p\0"
   "glMultTransposeMatrixf\0"
   "glMultTransposeMatrixfARB\0"
   "\0"
   /* _mesa_function_pool[21405]: GetTexEnviv (offset 277) */
   "iip\0"
   "glGetTexEnviv\0"
   "\0"
   /* _mesa_function_pool[21424]: ColorFragmentOp1ATI (will be remapped) */
   "iiiiiii\0"
   "glColorFragmentOp1ATI\0"
   "\0"
   /* _mesa_function_pool[21455]: GetUniformfvARB (will be remapped) */
   "iip\0"
   "glGetUniformfv\0"
   "glGetUniformfvARB\0"
   "\0"
   /* _mesa_function_pool[21493]: EGLImageTargetRenderbufferStorageOES (will be remapped) */
   "ip\0"
   "glEGLImageTargetRenderbufferStorageOES\0"
   "\0"
   /* _mesa_function_pool[21536]: PopClientAttrib (offset 334) */
   "\0"
   "glPopClientAttrib\0"
   "\0"
   /* _mesa_function_pool[21556]: ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN (dynamic) */
   "iffffffffffff\0"
   "glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[21627]: DetachObjectARB (will be remapped) */
   "ii\0"
   "glDetachObjectARB\0"
   "\0"
   /* _mesa_function_pool[21649]: VertexBlendARB (dynamic) */
   "i\0"
   "glVertexBlendARB\0"
   "\0"
   /* _mesa_function_pool[21669]: WindowPos3iMESA (will be remapped) */
   "iii\0"
   "glWindowPos3i\0"
   "glWindowPos3iARB\0"
   "glWindowPos3iMESA\0"
   "\0"
   /* _mesa_function_pool[21723]: SeparableFilter2D (offset 360) */
   "iiiiiipp\0"
   "glSeparableFilter2D\0"
   "glSeparableFilter2DEXT\0"
   "\0"
   /* _mesa_function_pool[21776]: ReplacementCodeuiColor4ubVertex3fvSUN (dynamic) */
   "ppp\0"
   "glReplacementCodeuiColor4ubVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[21821]: Map1d (offset 220) */
   "iddiip\0"
   "glMap1d\0"
   "\0"
   /* _mesa_function_pool[21837]: Map1f (offset 221) */
   "iffiip\0"
   "glMap1f\0"
   "\0"
   /* _mesa_function_pool[21853]: CompressedTexImage2DARB (will be remapped) */
   "iiiiiiip\0"
   "glCompressedTexImage2D\0"
   "glCompressedTexImage2DARB\0"
   "\0"
   /* _mesa_function_pool[21912]: ArrayElement (offset 306) */
   "i\0"
   "glArrayElement\0"
   "glArrayElementEXT\0"
   "\0"
   /* _mesa_function_pool[21948]: TexImage2D (offset 183) */
   "iiiiiiiip\0"
   "glTexImage2D\0"
   "\0"
   /* _mesa_function_pool[21972]: DepthBoundsEXT (will be remapped) */
   "dd\0"
   "glDepthBoundsEXT\0"
   "\0"
   /* _mesa_function_pool[21993]: ProgramParameters4fvNV (will be remapped) */
   "iiip\0"
   "glProgramParameters4fvNV\0"
   "\0"
   /* _mesa_function_pool[22024]: DeformationMap3fSGIX (dynamic) */
   "iffiiffiiffiip\0"
   "glDeformationMap3fSGIX\0"
   "\0"
   /* _mesa_function_pool[22063]: GetProgramivNV (will be remapped) */
   "iip\0"
   "glGetProgramivNV\0"
   "\0"
   /* _mesa_function_pool[22085]: GetMinmaxParameteriv (offset 366) */
   "iip\0"
   "glGetMinmaxParameteriv\0"
   "glGetMinmaxParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[22139]: PixelTransferf (offset 247) */
   "if\0"
   "glPixelTransferf\0"
   "\0"
   /* _mesa_function_pool[22160]: CopyTexImage1D (offset 323) */
   "iiiiiii\0"
   "glCopyTexImage1D\0"
   "glCopyTexImage1DEXT\0"
   "\0"
   /* _mesa_function_pool[22206]: PushMatrix (offset 298) */
   "\0"
   "glPushMatrix\0"
   "\0"
   /* _mesa_function_pool[22221]: Fogiv (offset 156) */
   "ip\0"
   "glFogiv\0"
   "\0"
   /* _mesa_function_pool[22233]: TexCoord1dv (offset 95) */
   "p\0"
   "glTexCoord1dv\0"
   "\0"
   /* _mesa_function_pool[22250]: AlphaFragmentOp3ATI (will be remapped) */
   "iiiiiiiiiiii\0"
   "glAlphaFragmentOp3ATI\0"
   "\0"
   /* _mesa_function_pool[22286]: PixelTransferi (offset 248) */
   "ii\0"
   "glPixelTransferi\0"
   "\0"
   /* _mesa_function_pool[22307]: GetVertexAttribdvNV (will be remapped) */
   "iip\0"
   "glGetVertexAttribdvNV\0"
   "\0"
   /* _mesa_function_pool[22334]: VertexAttrib3fvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib3fvNV\0"
   "\0"
   /* _mesa_function_pool[22358]: Rotatef (offset 300) */
   "ffff\0"
   "glRotatef\0"
   "\0"
   /* _mesa_function_pool[22374]: GetFinalCombinerInputParameterivNV (will be remapped) */
   "iip\0"
   "glGetFinalCombinerInputParameterivNV\0"
   "\0"
   /* _mesa_function_pool[22416]: Vertex3i (offset 138) */
   "iii\0"
   "glVertex3i\0"
   "\0"
   /* _mesa_function_pool[22432]: Vertex3f (offset 136) */
   "fff\0"
   "glVertex3f\0"
   "\0"
   /* _mesa_function_pool[22448]: Clear (offset 203) */
   "i\0"
   "glClear\0"
   "\0"
   /* _mesa_function_pool[22459]: Vertex3d (offset 134) */
   "ddd\0"
   "glVertex3d\0"
   "\0"
   /* _mesa_function_pool[22475]: GetMapParameterivNV (dynamic) */
   "iip\0"
   "glGetMapParameterivNV\0"
   "\0"
   /* _mesa_function_pool[22502]: Uniform4iARB (will be remapped) */
   "iiiii\0"
   "glUniform4i\0"
   "glUniform4iARB\0"
   "\0"
   /* _mesa_function_pool[22536]: ReadBuffer (offset 254) */
   "i\0"
   "glReadBuffer\0"
   "\0"
   /* _mesa_function_pool[22552]: ConvolutionParameteri (offset 352) */
   "iii\0"
   "glConvolutionParameteri\0"
   "glConvolutionParameteriEXT\0"
   "\0"
   /* _mesa_function_pool[22608]: Ortho (offset 296) */
   "dddddd\0"
   "glOrtho\0"
   "\0"
   /* _mesa_function_pool[22624]: Binormal3sEXT (dynamic) */
   "iii\0"
   "glBinormal3sEXT\0"
   "\0"
   /* _mesa_function_pool[22645]: ListBase (offset 6) */
   "i\0"
   "glListBase\0"
   "\0"
   /* _mesa_function_pool[22659]: Vertex3s (offset 140) */
   "iii\0"
   "glVertex3s\0"
   "\0"
   /* _mesa_function_pool[22675]: ConvolutionParameterf (offset 350) */
   "iif\0"
   "glConvolutionParameterf\0"
   "glConvolutionParameterfEXT\0"
   "\0"
   /* _mesa_function_pool[22731]: GetColorTableParameteriv (offset 345) */
   "iip\0"
   "glGetColorTableParameteriv\0"
   "glGetColorTableParameterivSGI\0"
   "glGetColorTableParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[22823]: ProgramEnvParameter4dvARB (will be remapped) */
   "iip\0"
   "glProgramEnvParameter4dvARB\0"
   "glProgramParameter4dvNV\0"
   "\0"
   /* _mesa_function_pool[22880]: ShadeModel (offset 177) */
   "i\0"
   "glShadeModel\0"
   "\0"
   /* _mesa_function_pool[22896]: VertexAttribs2fvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs2fvNV\0"
   "\0"
   /* _mesa_function_pool[22922]: Rectiv (offset 91) */
   "pp\0"
   "glRectiv\0"
   "\0"
   /* _mesa_function_pool[22935]: UseProgramObjectARB (will be remapped) */
   "i\0"
   "glUseProgram\0"
   "glUseProgramObjectARB\0"
   "\0"
   /* _mesa_function_pool[22973]: GetMapParameterfvNV (dynamic) */
   "iip\0"
   "glGetMapParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[23000]: EndConditionalRenderNV (will be remapped) */
   "\0"
   "glEndConditionalRenderNV\0"
   "\0"
   /* _mesa_function_pool[23027]: PassTexCoordATI (will be remapped) */
   "iii\0"
   "glPassTexCoordATI\0"
   "\0"
   /* _mesa_function_pool[23050]: DeleteProgram (will be remapped) */
   "i\0"
   "glDeleteProgram\0"
   "\0"
   /* _mesa_function_pool[23069]: Tangent3ivEXT (dynamic) */
   "p\0"
   "glTangent3ivEXT\0"
   "\0"
   /* _mesa_function_pool[23088]: Tangent3dEXT (dynamic) */
   "ddd\0"
   "glTangent3dEXT\0"
   "\0"
   /* _mesa_function_pool[23108]: SecondaryColor3dvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3dv\0"
   "glSecondaryColor3dvEXT\0"
   "\0"
   /* _mesa_function_pool[23154]: Vertex2fv (offset 129) */
   "p\0"
   "glVertex2fv\0"
   "\0"
   /* _mesa_function_pool[23169]: MultiDrawArraysEXT (will be remapped) */
   "ippi\0"
   "glMultiDrawArrays\0"
   "glMultiDrawArraysEXT\0"
   "\0"
   /* _mesa_function_pool[23214]: BindRenderbufferEXT (will be remapped) */
   "ii\0"
   "glBindRenderbuffer\0"
   "glBindRenderbufferEXT\0"
   "\0"
   /* _mesa_function_pool[23259]: MultiTexCoord4dARB (offset 400) */
   "idddd\0"
   "glMultiTexCoord4d\0"
   "glMultiTexCoord4dARB\0"
   "\0"
   /* _mesa_function_pool[23305]: Vertex3sv (offset 141) */
   "p\0"
   "glVertex3sv\0"
   "\0"
   /* _mesa_function_pool[23320]: SecondaryColor3usEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3us\0"
   "glSecondaryColor3usEXT\0"
   "\0"
   /* _mesa_function_pool[23368]: ProgramLocalParameter4fvARB (will be remapped) */
   "iip\0"
   "glProgramLocalParameter4fvARB\0"
   "\0"
   /* _mesa_function_pool[23403]: DeleteProgramsNV (will be remapped) */
   "ip\0"
   "glDeleteProgramsARB\0"
   "glDeleteProgramsNV\0"
   "\0"
   /* _mesa_function_pool[23446]: EvalMesh1 (offset 236) */
   "iii\0"
   "glEvalMesh1\0"
   "\0"
   /* _mesa_function_pool[23463]: MultiTexCoord1sARB (offset 382) */
   "ii\0"
   "glMultiTexCoord1s\0"
   "glMultiTexCoord1sARB\0"
   "\0"
   /* _mesa_function_pool[23506]: ReplacementCodeuiColor3fVertex3fSUN (dynamic) */
   "iffffff\0"
   "glReplacementCodeuiColor3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[23553]: GetVertexAttribPointervNV (will be remapped) */
   "iip\0"
   "glGetVertexAttribPointerv\0"
   "glGetVertexAttribPointervARB\0"
   "glGetVertexAttribPointervNV\0"
   "\0"
   /* _mesa_function_pool[23641]: VertexAttribs1fvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs1fvNV\0"
   "\0"
   /* _mesa_function_pool[23667]: MultiTexCoord1dvARB (offset 377) */
   "ip\0"
   "glMultiTexCoord1dv\0"
   "glMultiTexCoord1dvARB\0"
   "\0"
   /* _mesa_function_pool[23712]: Uniform2iARB (will be remapped) */
   "iii\0"
   "glUniform2i\0"
   "glUniform2iARB\0"
   "\0"
   /* _mesa_function_pool[23744]: Vertex2iv (offset 131) */
   "p\0"
   "glVertex2iv\0"
   "\0"
   /* _mesa_function_pool[23759]: GetProgramStringNV (will be remapped) */
   "iip\0"
   "glGetProgramStringNV\0"
   "\0"
   /* _mesa_function_pool[23785]: ColorPointerEXT (will be remapped) */
   "iiiip\0"
   "glColorPointerEXT\0"
   "\0"
   /* _mesa_function_pool[23810]: LineWidth (offset 168) */
   "f\0"
   "glLineWidth\0"
   "\0"
   /* _mesa_function_pool[23825]: MapBufferARB (will be remapped) */
   "ii\0"
   "glMapBuffer\0"
   "glMapBufferARB\0"
   "\0"
   /* _mesa_function_pool[23856]: MultiDrawElementsBaseVertex (will be remapped) */
   "ipipip\0"
   "glMultiDrawElementsBaseVertex\0"
   "\0"
   /* _mesa_function_pool[23894]: Binormal3svEXT (dynamic) */
   "p\0"
   "glBinormal3svEXT\0"
   "\0"
   /* _mesa_function_pool[23914]: ApplyTextureEXT (dynamic) */
   "i\0"
   "glApplyTextureEXT\0"
   "\0"
   /* _mesa_function_pool[23935]: TexGendv (offset 189) */
   "iip\0"
   "glTexGendv\0"
   "\0"
   /* _mesa_function_pool[23951]: EnableIndexedEXT (will be remapped) */
   "ii\0"
   "glEnableIndexedEXT\0"
   "\0"
   /* _mesa_function_pool[23974]: TextureMaterialEXT (dynamic) */
   "ii\0"
   "glTextureMaterialEXT\0"
   "\0"
   /* _mesa_function_pool[23999]: TextureLightEXT (dynamic) */
   "i\0"
   "glTextureLightEXT\0"
   "\0"
   /* _mesa_function_pool[24020]: ResetMinmax (offset 370) */
   "i\0"
   "glResetMinmax\0"
   "glResetMinmaxEXT\0"
   "\0"
   /* _mesa_function_pool[24054]: SpriteParameterfSGIX (dynamic) */
   "if\0"
   "glSpriteParameterfSGIX\0"
   "\0"
   /* _mesa_function_pool[24081]: EnableClientState (offset 313) */
   "i\0"
   "glEnableClientState\0"
   "\0"
   /* _mesa_function_pool[24104]: VertexAttrib4sNV (will be remapped) */
   "iiiii\0"
   "glVertexAttrib4sNV\0"
   "\0"
   /* _mesa_function_pool[24130]: GetConvolutionParameterfv (offset 357) */
   "iip\0"
   "glGetConvolutionParameterfv\0"
   "glGetConvolutionParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[24194]: VertexAttribs4dvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs4dvNV\0"
   "\0"
   /* _mesa_function_pool[24220]: MultiModeDrawArraysIBM (will be remapped) */
   "pppii\0"
   "glMultiModeDrawArraysIBM\0"
   "\0"
   /* _mesa_function_pool[24252]: VertexAttrib4dARB (will be remapped) */
   "idddd\0"
   "glVertexAttrib4d\0"
   "glVertexAttrib4dARB\0"
   "\0"
   /* _mesa_function_pool[24296]: GetTexBumpParameterfvATI (will be remapped) */
   "ip\0"
   "glGetTexBumpParameterfvATI\0"
   "\0"
   /* _mesa_function_pool[24327]: ProgramNamedParameter4dNV (will be remapped) */
   "iipdddd\0"
   "glProgramNamedParameter4dNV\0"
   "\0"
   /* _mesa_function_pool[24364]: GetMaterialfv (offset 269) */
   "iip\0"
   "glGetMaterialfv\0"
   "\0"
   /* _mesa_function_pool[24385]: VertexWeightfEXT (dynamic) */
   "f\0"
   "glVertexWeightfEXT\0"
   "\0"
   /* _mesa_function_pool[24407]: Binormal3fEXT (dynamic) */
   "fff\0"
   "glBinormal3fEXT\0"
   "\0"
   /* _mesa_function_pool[24428]: CallList (offset 2) */
   "i\0"
   "glCallList\0"
   "\0"
   /* _mesa_function_pool[24442]: Materialfv (offset 170) */
   "iip\0"
   "glMaterialfv\0"
   "\0"
   /* _mesa_function_pool[24460]: TexCoord3fv (offset 113) */
   "p\0"
   "glTexCoord3fv\0"
   "\0"
   /* _mesa_function_pool[24477]: FogCoordfvEXT (will be remapped) */
   "p\0"
   "glFogCoordfv\0"
   "glFogCoordfvEXT\0"
   "\0"
   /* _mesa_function_pool[24509]: MultiTexCoord1ivARB (offset 381) */
   "ip\0"
   "glMultiTexCoord1iv\0"
   "glMultiTexCoord1ivARB\0"
   "\0"
   /* _mesa_function_pool[24554]: SecondaryColor3ubEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3ub\0"
   "glSecondaryColor3ubEXT\0"
   "\0"
   /* _mesa_function_pool[24602]: MultiTexCoord2ivARB (offset 389) */
   "ip\0"
   "glMultiTexCoord2iv\0"
   "glMultiTexCoord2ivARB\0"
   "\0"
   /* _mesa_function_pool[24647]: FogFuncSGIS (dynamic) */
   "ip\0"
   "glFogFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[24665]: CopyTexSubImage2D (offset 326) */
   "iiiiiiii\0"
   "glCopyTexSubImage2D\0"
   "glCopyTexSubImage2DEXT\0"
   "\0"
   /* _mesa_function_pool[24718]: GetObjectParameterivARB (will be remapped) */
   "iip\0"
   "glGetObjectParameterivARB\0"
   "\0"
   /* _mesa_function_pool[24749]: Color3iv (offset 16) */
   "p\0"
   "glColor3iv\0"
   "\0"
   /* _mesa_function_pool[24763]: TexCoord4fVertex4fSUN (dynamic) */
   "ffffffff\0"
   "glTexCoord4fVertex4fSUN\0"
   "\0"
   /* _mesa_function_pool[24797]: DrawElements (offset 311) */
   "iiip\0"
   "glDrawElements\0"
   "\0"
   /* _mesa_function_pool[24818]: BindVertexArrayAPPLE (will be remapped) */
   "i\0"
   "glBindVertexArrayAPPLE\0"
   "\0"
   /* _mesa_function_pool[24844]: GetProgramLocalParameterdvARB (will be remapped) */
   "iip\0"
   "glGetProgramLocalParameterdvARB\0"
   "\0"
   /* _mesa_function_pool[24881]: GetHistogramParameteriv (offset 363) */
   "iip\0"
   "glGetHistogramParameteriv\0"
   "glGetHistogramParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[24941]: MultiTexCoord1iARB (offset 380) */
   "ii\0"
   "glMultiTexCoord1i\0"
   "glMultiTexCoord1iARB\0"
   "\0"
   /* _mesa_function_pool[24984]: GetConvolutionFilter (offset 356) */
   "iiip\0"
   "glGetConvolutionFilter\0"
   "glGetConvolutionFilterEXT\0"
   "\0"
   /* _mesa_function_pool[25039]: GetProgramivARB (will be remapped) */
   "iip\0"
   "glGetProgramivARB\0"
   "\0"
   /* _mesa_function_pool[25062]: BlendFuncSeparateEXT (will be remapped) */
   "iiii\0"
   "glBlendFuncSeparate\0"
   "glBlendFuncSeparateEXT\0"
   "glBlendFuncSeparateINGR\0"
   "\0"
   /* _mesa_function_pool[25135]: MapBufferRange (will be remapped) */
   "iiii\0"
   "glMapBufferRange\0"
   "\0"
   /* _mesa_function_pool[25158]: ProgramParameters4dvNV (will be remapped) */
   "iiip\0"
   "glProgramParameters4dvNV\0"
   "\0"
   /* _mesa_function_pool[25189]: TexCoord2fColor3fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glTexCoord2fColor3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[25226]: EvalPoint2 (offset 239) */
   "ii\0"
   "glEvalPoint2\0"
   "\0"
   /* _mesa_function_pool[25243]: EvalPoint1 (offset 237) */
   "i\0"
   "glEvalPoint1\0"
   "\0"
   /* _mesa_function_pool[25259]: Binormal3dvEXT (dynamic) */
   "p\0"
   "glBinormal3dvEXT\0"
   "\0"
   /* _mesa_function_pool[25279]: PopMatrix (offset 297) */
   "\0"
   "glPopMatrix\0"
   "\0"
   /* _mesa_function_pool[25293]: FinishFenceNV (will be remapped) */
   "i\0"
   "glFinishFenceNV\0"
   "\0"
   /* _mesa_function_pool[25312]: GetFogFuncSGIS (dynamic) */
   "p\0"
   "glGetFogFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[25332]: GetUniformLocationARB (will be remapped) */
   "ip\0"
   "glGetUniformLocation\0"
   "glGetUniformLocationARB\0"
   "\0"
   /* _mesa_function_pool[25381]: SecondaryColor3fEXT (will be remapped) */
   "fff\0"
   "glSecondaryColor3f\0"
   "glSecondaryColor3fEXT\0"
   "\0"
   /* _mesa_function_pool[25427]: GetTexGeniv (offset 280) */
   "iip\0"
   "glGetTexGeniv\0"
   "\0"
   /* _mesa_function_pool[25446]: CombinerInputNV (will be remapped) */
   "iiiiii\0"
   "glCombinerInputNV\0"
   "\0"
   /* _mesa_function_pool[25472]: VertexAttrib3sARB (will be remapped) */
   "iiii\0"
   "glVertexAttrib3s\0"
   "glVertexAttrib3sARB\0"
   "\0"
   /* _mesa_function_pool[25515]: ColorMaskIndexedEXT (will be remapped) */
   "iiiii\0"
   "glColorMaskIndexedEXT\0"
   "\0"
   /* _mesa_function_pool[25544]: ReplacementCodeuiNormal3fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glReplacementCodeuiNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[25589]: Map2d (offset 222) */
   "iddiiddiip\0"
   "glMap2d\0"
   "\0"
   /* _mesa_function_pool[25609]: Map2f (offset 223) */
   "iffiiffiip\0"
   "glMap2f\0"
   "\0"
   /* _mesa_function_pool[25629]: ProgramStringARB (will be remapped) */
   "iiip\0"
   "glProgramStringARB\0"
   "\0"
   /* _mesa_function_pool[25654]: Vertex4s (offset 148) */
   "iiii\0"
   "glVertex4s\0"
   "\0"
   /* _mesa_function_pool[25671]: TexCoord4fVertex4fvSUN (dynamic) */
   "pp\0"
   "glTexCoord4fVertex4fvSUN\0"
   "\0"
   /* _mesa_function_pool[25700]: VertexAttrib3sNV (will be remapped) */
   "iiii\0"
   "glVertexAttrib3sNV\0"
   "\0"
   /* _mesa_function_pool[25725]: VertexAttrib1fNV (will be remapped) */
   "if\0"
   "glVertexAttrib1fNV\0"
   "\0"
   /* _mesa_function_pool[25748]: Vertex4f (offset 144) */
   "ffff\0"
   "glVertex4f\0"
   "\0"
   /* _mesa_function_pool[25765]: EvalCoord1d (offset 228) */
   "d\0"
   "glEvalCoord1d\0"
   "\0"
   /* _mesa_function_pool[25782]: Vertex4d (offset 142) */
   "dddd\0"
   "glVertex4d\0"
   "\0"
   /* _mesa_function_pool[25799]: RasterPos4dv (offset 79) */
   "p\0"
   "glRasterPos4dv\0"
   "\0"
   /* _mesa_function_pool[25817]: FragmentLightfSGIX (dynamic) */
   "iif\0"
   "glFragmentLightfSGIX\0"
   "\0"
   /* _mesa_function_pool[25843]: GetCompressedTexImageARB (will be remapped) */
   "iip\0"
   "glGetCompressedTexImage\0"
   "glGetCompressedTexImageARB\0"
   "\0"
   /* _mesa_function_pool[25899]: GetTexGenfv (offset 279) */
   "iip\0"
   "glGetTexGenfv\0"
   "\0"
   /* _mesa_function_pool[25918]: Vertex4i (offset 146) */
   "iiii\0"
   "glVertex4i\0"
   "\0"
   /* _mesa_function_pool[25935]: VertexWeightPointerEXT (dynamic) */
   "iiip\0"
   "glVertexWeightPointerEXT\0"
   "\0"
   /* _mesa_function_pool[25966]: GetHistogram (offset 361) */
   "iiiip\0"
   "glGetHistogram\0"
   "glGetHistogramEXT\0"
   "\0"
   /* _mesa_function_pool[26006]: ActiveStencilFaceEXT (will be remapped) */
   "i\0"
   "glActiveStencilFaceEXT\0"
   "\0"
   /* _mesa_function_pool[26032]: StencilFuncSeparateATI (will be remapped) */
   "iiii\0"
   "glStencilFuncSeparateATI\0"
   "\0"
   /* _mesa_function_pool[26063]: Materialf (offset 169) */
   "iif\0"
   "glMaterialf\0"
   "\0"
   /* _mesa_function_pool[26080]: GetShaderSourceARB (will be remapped) */
   "iipp\0"
   "glGetShaderSource\0"
   "glGetShaderSourceARB\0"
   "\0"
   /* _mesa_function_pool[26125]: IglooInterfaceSGIX (dynamic) */
   "ip\0"
   "glIglooInterfaceSGIX\0"
   "\0"
   /* _mesa_function_pool[26150]: Materiali (offset 171) */
   "iii\0"
   "glMateriali\0"
   "\0"
   /* _mesa_function_pool[26167]: VertexAttrib4dNV (will be remapped) */
   "idddd\0"
   "glVertexAttrib4dNV\0"
   "\0"
   /* _mesa_function_pool[26193]: MultiModeDrawElementsIBM (will be remapped) */
   "ppipii\0"
   "glMultiModeDrawElementsIBM\0"
   "\0"
   /* _mesa_function_pool[26228]: Indexsv (offset 51) */
   "p\0"
   "glIndexsv\0"
   "\0"
   /* _mesa_function_pool[26241]: MultiTexCoord4svARB (offset 407) */
   "ip\0"
   "glMultiTexCoord4sv\0"
   "glMultiTexCoord4svARB\0"
   "\0"
   /* _mesa_function_pool[26286]: LightModelfv (offset 164) */
   "ip\0"
   "glLightModelfv\0"
   "\0"
   /* _mesa_function_pool[26305]: TexCoord2dv (offset 103) */
   "p\0"
   "glTexCoord2dv\0"
   "\0"
   /* _mesa_function_pool[26322]: GenQueriesARB (will be remapped) */
   "ip\0"
   "glGenQueries\0"
   "glGenQueriesARB\0"
   "\0"
   /* _mesa_function_pool[26355]: EvalCoord1dv (offset 229) */
   "p\0"
   "glEvalCoord1dv\0"
   "\0"
   /* _mesa_function_pool[26373]: ReplacementCodeuiVertex3fSUN (dynamic) */
   "ifff\0"
   "glReplacementCodeuiVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[26410]: Translated (offset 303) */
   "ddd\0"
   "glTranslated\0"
   "\0"
   /* _mesa_function_pool[26428]: Translatef (offset 304) */
   "fff\0"
   "glTranslatef\0"
   "\0"
   /* _mesa_function_pool[26446]: StencilMask (offset 209) */
   "i\0"
   "glStencilMask\0"
   "\0"
   /* _mesa_function_pool[26463]: Tangent3iEXT (dynamic) */
   "iii\0"
   "glTangent3iEXT\0"
   "\0"
   /* _mesa_function_pool[26483]: GetLightiv (offset 265) */
   "iip\0"
   "glGetLightiv\0"
   "\0"
   /* _mesa_function_pool[26501]: DrawMeshArraysSUN (dynamic) */
   "iiii\0"
   "glDrawMeshArraysSUN\0"
   "\0"
   /* _mesa_function_pool[26527]: IsList (offset 287) */
   "i\0"
   "glIsList\0"
   "\0"
   /* _mesa_function_pool[26539]: IsSync (will be remapped) */
   "i\0"
   "glIsSync\0"
   "\0"
   /* _mesa_function_pool[26551]: RenderMode (offset 196) */
   "i\0"
   "glRenderMode\0"
   "\0"
   /* _mesa_function_pool[26567]: GetMapControlPointsNV (dynamic) */
   "iiiiiip\0"
   "glGetMapControlPointsNV\0"
   "\0"
   /* _mesa_function_pool[26600]: DrawBuffersARB (will be remapped) */
   "ip\0"
   "glDrawBuffers\0"
   "glDrawBuffersARB\0"
   "glDrawBuffersATI\0"
   "\0"
   /* _mesa_function_pool[26652]: ProgramLocalParameter4fARB (will be remapped) */
   "iiffff\0"
   "glProgramLocalParameter4fARB\0"
   "\0"
   /* _mesa_function_pool[26689]: SpriteParameterivSGIX (dynamic) */
   "ip\0"
   "glSpriteParameterivSGIX\0"
   "\0"
   /* _mesa_function_pool[26717]: ProvokingVertexEXT (will be remapped) */
   "i\0"
   "glProvokingVertexEXT\0"
   "glProvokingVertex\0"
   "\0"
   /* _mesa_function_pool[26759]: MultiTexCoord1fARB (offset 378) */
   "if\0"
   "glMultiTexCoord1f\0"
   "glMultiTexCoord1fARB\0"
   "\0"
   /* _mesa_function_pool[26802]: LoadName (offset 198) */
   "i\0"
   "glLoadName\0"
   "\0"
   /* _mesa_function_pool[26816]: VertexAttribs4ubvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs4ubvNV\0"
   "\0"
   /* _mesa_function_pool[26843]: WeightsvARB (dynamic) */
   "ip\0"
   "glWeightsvARB\0"
   "\0"
   /* _mesa_function_pool[26861]: Uniform1fvARB (will be remapped) */
   "iip\0"
   "glUniform1fv\0"
   "glUniform1fvARB\0"
   "\0"
   /* _mesa_function_pool[26895]: CopyTexSubImage1D (offset 325) */
   "iiiiii\0"
   "glCopyTexSubImage1D\0"
   "glCopyTexSubImage1DEXT\0"
   "\0"
   /* _mesa_function_pool[26946]: CullFace (offset 152) */
   "i\0"
   "glCullFace\0"
   "\0"
   /* _mesa_function_pool[26960]: BindTexture (offset 307) */
   "ii\0"
   "glBindTexture\0"
   "glBindTextureEXT\0"
   "\0"
   /* _mesa_function_pool[26995]: BeginFragmentShaderATI (will be remapped) */
   "\0"
   "glBeginFragmentShaderATI\0"
   "\0"
   /* _mesa_function_pool[27022]: MultiTexCoord4fARB (offset 402) */
   "iffff\0"
   "glMultiTexCoord4f\0"
   "glMultiTexCoord4fARB\0"
   "\0"
   /* _mesa_function_pool[27068]: VertexAttribs3svNV (will be remapped) */
   "iip\0"
   "glVertexAttribs3svNV\0"
   "\0"
   /* _mesa_function_pool[27094]: StencilFunc (offset 243) */
   "iii\0"
   "glStencilFunc\0"
   "\0"
   /* _mesa_function_pool[27113]: CopyPixels (offset 255) */
   "iiiii\0"
   "glCopyPixels\0"
   "\0"
   /* _mesa_function_pool[27133]: Rectsv (offset 93) */
   "pp\0"
   "glRectsv\0"
   "\0"
   /* _mesa_function_pool[27146]: ReplacementCodeuivSUN (dynamic) */
   "p\0"
   "glReplacementCodeuivSUN\0"
   "\0"
   /* _mesa_function_pool[27173]: EnableVertexAttribArrayARB (will be remapped) */
   "i\0"
   "glEnableVertexAttribArray\0"
   "glEnableVertexAttribArrayARB\0"
   "\0"
   /* _mesa_function_pool[27231]: NormalPointervINTEL (dynamic) */
   "ip\0"
   "glNormalPointervINTEL\0"
   "\0"
   /* _mesa_function_pool[27257]: CopyConvolutionFilter2D (offset 355) */
   "iiiiii\0"
   "glCopyConvolutionFilter2D\0"
   "glCopyConvolutionFilter2DEXT\0"
   "\0"
   /* _mesa_function_pool[27320]: WindowPos3ivMESA (will be remapped) */
   "p\0"
   "glWindowPos3iv\0"
   "glWindowPos3ivARB\0"
   "glWindowPos3ivMESA\0"
   "\0"
   /* _mesa_function_pool[27375]: CopyBufferSubData (will be remapped) */
   "iiiii\0"
   "glCopyBufferSubData\0"
   "\0"
   /* _mesa_function_pool[27402]: NormalPointer (offset 318) */
   "iip\0"
   "glNormalPointer\0"
   "\0"
   /* _mesa_function_pool[27423]: TexParameterfv (offset 179) */
   "iip\0"
   "glTexParameterfv\0"
   "\0"
   /* _mesa_function_pool[27445]: IsBufferARB (will be remapped) */
   "i\0"
   "glIsBuffer\0"
   "glIsBufferARB\0"
   "\0"
   /* _mesa_function_pool[27473]: WindowPos4iMESA (will be remapped) */
   "iiii\0"
   "glWindowPos4iMESA\0"
   "\0"
   /* _mesa_function_pool[27497]: VertexAttrib4uivARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4uiv\0"
   "glVertexAttrib4uivARB\0"
   "\0"
   /* _mesa_function_pool[27542]: Tangent3bvEXT (dynamic) */
   "p\0"
   "glTangent3bvEXT\0"
   "\0"
   /* _mesa_function_pool[27561]: UniformMatrix3x4fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix3x4fv\0"
   "\0"
   /* _mesa_function_pool[27588]: ClipPlane (offset 150) */
   "ip\0"
   "glClipPlane\0"
   "\0"
   /* _mesa_function_pool[27604]: Recti (offset 90) */
   "iiii\0"
   "glRecti\0"
   "\0"
   /* _mesa_function_pool[27618]: DrawRangeElementsBaseVertex (will be remapped) */
   "iiiiipi\0"
   "glDrawRangeElementsBaseVertex\0"
   "\0"
   /* _mesa_function_pool[27657]: TexCoordPointervINTEL (dynamic) */
   "iip\0"
   "glTexCoordPointervINTEL\0"
   "\0"
   /* _mesa_function_pool[27686]: DeleteBuffersARB (will be remapped) */
   "ip\0"
   "glDeleteBuffers\0"
   "glDeleteBuffersARB\0"
   "\0"
   /* _mesa_function_pool[27725]: PixelTransformParameterfvEXT (dynamic) */
   "iip\0"
   "glPixelTransformParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[27761]: WindowPos4fvMESA (will be remapped) */
   "p\0"
   "glWindowPos4fvMESA\0"
   "\0"
   /* _mesa_function_pool[27783]: GetPixelMapuiv (offset 272) */
   "ip\0"
   "glGetPixelMapuiv\0"
   "\0"
   /* _mesa_function_pool[27804]: Rectf (offset 88) */
   "ffff\0"
   "glRectf\0"
   "\0"
   /* _mesa_function_pool[27818]: VertexAttrib1sNV (will be remapped) */
   "ii\0"
   "glVertexAttrib1sNV\0"
   "\0"
   /* _mesa_function_pool[27841]: Indexfv (offset 47) */
   "p\0"
   "glIndexfv\0"
   "\0"
   /* _mesa_function_pool[27854]: SecondaryColor3svEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3sv\0"
   "glSecondaryColor3svEXT\0"
   "\0"
   /* _mesa_function_pool[27900]: LoadTransposeMatrixfARB (will be remapped) */
   "p\0"
   "glLoadTransposeMatrixf\0"
   "glLoadTransposeMatrixfARB\0"
   "\0"
   /* _mesa_function_pool[27952]: GetPointerv (offset 329) */
   "ip\0"
   "glGetPointerv\0"
   "glGetPointervEXT\0"
   "\0"
   /* _mesa_function_pool[27987]: Tangent3bEXT (dynamic) */
   "iii\0"
   "glTangent3bEXT\0"
   "\0"
   /* _mesa_function_pool[28007]: CombinerParameterfNV (will be remapped) */
   "if\0"
   "glCombinerParameterfNV\0"
   "\0"
   /* _mesa_function_pool[28034]: IndexMask (offset 212) */
   "i\0"
   "glIndexMask\0"
   "\0"
   /* _mesa_function_pool[28049]: BindProgramNV (will be remapped) */
   "ii\0"
   "glBindProgramARB\0"
   "glBindProgramNV\0"
   "\0"
   /* _mesa_function_pool[28086]: VertexAttrib4svARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4sv\0"
   "glVertexAttrib4svARB\0"
   "\0"
   /* _mesa_function_pool[28129]: GetFloatv (offset 262) */
   "ip\0"
   "glGetFloatv\0"
   "\0"
   /* _mesa_function_pool[28145]: CreateDebugObjectMESA (dynamic) */
   "\0"
   "glCreateDebugObjectMESA\0"
   "\0"
   /* _mesa_function_pool[28171]: GetShaderiv (will be remapped) */
   "iip\0"
   "glGetShaderiv\0"
   "\0"
   /* _mesa_function_pool[28190]: ClientWaitSync (will be remapped) */
   "iii\0"
   "glClientWaitSync\0"
   "\0"
   /* _mesa_function_pool[28212]: TexCoord4s (offset 124) */
   "iiii\0"
   "glTexCoord4s\0"
   "\0"
   /* _mesa_function_pool[28231]: TexCoord3sv (offset 117) */
   "p\0"
   "glTexCoord3sv\0"
   "\0"
   /* _mesa_function_pool[28248]: BindFragmentShaderATI (will be remapped) */
   "i\0"
   "glBindFragmentShaderATI\0"
   "\0"
   /* _mesa_function_pool[28275]: PopAttrib (offset 218) */
   "\0"
   "glPopAttrib\0"
   "\0"
   /* _mesa_function_pool[28289]: Fogfv (offset 154) */
   "ip\0"
   "glFogfv\0"
   "\0"
   /* _mesa_function_pool[28301]: UnmapBufferARB (will be remapped) */
   "i\0"
   "glUnmapBuffer\0"
   "glUnmapBufferARB\0"
   "\0"
   /* _mesa_function_pool[28335]: InitNames (offset 197) */
   "\0"
   "glInitNames\0"
   "\0"
   /* _mesa_function_pool[28349]: Normal3sv (offset 61) */
   "p\0"
   "glNormal3sv\0"
   "\0"
   /* _mesa_function_pool[28364]: Minmax (offset 368) */
   "iii\0"
   "glMinmax\0"
   "glMinmaxEXT\0"
   "\0"
   /* _mesa_function_pool[28390]: TexCoord4d (offset 118) */
   "dddd\0"
   "glTexCoord4d\0"
   "\0"
   /* _mesa_function_pool[28409]: TexCoord4f (offset 120) */
   "ffff\0"
   "glTexCoord4f\0"
   "\0"
   /* _mesa_function_pool[28428]: FogCoorddvEXT (will be remapped) */
   "p\0"
   "glFogCoorddv\0"
   "glFogCoorddvEXT\0"
   "\0"
   /* _mesa_function_pool[28460]: FinishTextureSUNX (dynamic) */
   "\0"
   "glFinishTextureSUNX\0"
   "\0"
   /* _mesa_function_pool[28482]: GetFragmentLightfvSGIX (dynamic) */
   "iip\0"
   "glGetFragmentLightfvSGIX\0"
   "\0"
   /* _mesa_function_pool[28512]: Binormal3fvEXT (dynamic) */
   "p\0"
   "glBinormal3fvEXT\0"
   "\0"
   /* _mesa_function_pool[28532]: GetBooleanv (offset 258) */
   "ip\0"
   "glGetBooleanv\0"
   "\0"
   /* _mesa_function_pool[28550]: ColorFragmentOp3ATI (will be remapped) */
   "iiiiiiiiiiiii\0"
   "glColorFragmentOp3ATI\0"
   "\0"
   /* _mesa_function_pool[28587]: Hint (offset 158) */
   "ii\0"
   "glHint\0"
   "\0"
   /* _mesa_function_pool[28598]: Color4dv (offset 28) */
   "p\0"
   "glColor4dv\0"
   "\0"
   /* _mesa_function_pool[28612]: VertexAttrib2svARB (will be remapped) */
   "ip\0"
   "glVertexAttrib2sv\0"
   "glVertexAttrib2svARB\0"
   "\0"
   /* _mesa_function_pool[28655]: AreProgramsResidentNV (will be remapped) */
   "ipp\0"
   "glAreProgramsResidentNV\0"
   "\0"
   /* _mesa_function_pool[28684]: WindowPos3svMESA (will be remapped) */
   "p\0"
   "glWindowPos3sv\0"
   "glWindowPos3svARB\0"
   "glWindowPos3svMESA\0"
   "\0"
   /* _mesa_function_pool[28739]: CopyColorSubTable (offset 347) */
   "iiiii\0"
   "glCopyColorSubTable\0"
   "glCopyColorSubTableEXT\0"
   "\0"
   /* _mesa_function_pool[28789]: WeightdvARB (dynamic) */
   "ip\0"
   "glWeightdvARB\0"
   "\0"
   /* _mesa_function_pool[28807]: DeleteRenderbuffersEXT (will be remapped) */
   "ip\0"
   "glDeleteRenderbuffers\0"
   "glDeleteRenderbuffersEXT\0"
   "\0"
   /* _mesa_function_pool[28858]: VertexAttrib4NubvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Nubv\0"
   "glVertexAttrib4NubvARB\0"
   "\0"
   /* _mesa_function_pool[28905]: VertexAttrib3dvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib3dvNV\0"
   "\0"
   /* _mesa_function_pool[28929]: GetObjectParameterfvARB (will be remapped) */
   "iip\0"
   "glGetObjectParameterfvARB\0"
   "\0"
   /* _mesa_function_pool[28960]: Vertex4iv (offset 147) */
   "p\0"
   "glVertex4iv\0"
   "\0"
   /* _mesa_function_pool[28975]: GetProgramEnvParameterdvARB (will be remapped) */
   "iip\0"
   "glGetProgramEnvParameterdvARB\0"
   "\0"
   /* _mesa_function_pool[29010]: TexCoord4dv (offset 119) */
   "p\0"
   "glTexCoord4dv\0"
   "\0"
   /* _mesa_function_pool[29027]: LockArraysEXT (will be remapped) */
   "ii\0"
   "glLockArraysEXT\0"
   "\0"
   /* _mesa_function_pool[29047]: Begin (offset 7) */
   "i\0"
   "glBegin\0"
   "\0"
   /* _mesa_function_pool[29058]: LightModeli (offset 165) */
   "ii\0"
   "glLightModeli\0"
   "\0"
   /* _mesa_function_pool[29076]: Rectfv (offset 89) */
   "pp\0"
   "glRectfv\0"
   "\0"
   /* _mesa_function_pool[29089]: LightModelf (offset 163) */
   "if\0"
   "glLightModelf\0"
   "\0"
   /* _mesa_function_pool[29107]: GetTexParameterfv (offset 282) */
   "iip\0"
   "glGetTexParameterfv\0"
   "\0"
   /* _mesa_function_pool[29132]: GetLightfv (offset 264) */
   "iip\0"
   "glGetLightfv\0"
   "\0"
   /* _mesa_function_pool[29150]: PixelTransformParameterivEXT (dynamic) */
   "iip\0"
   "glPixelTransformParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[29186]: BinormalPointerEXT (dynamic) */
   "iip\0"
   "glBinormalPointerEXT\0"
   "\0"
   /* _mesa_function_pool[29212]: VertexAttrib1dNV (will be remapped) */
   "id\0"
   "glVertexAttrib1dNV\0"
   "\0"
   /* _mesa_function_pool[29235]: GetCombinerInputParameterivNV (will be remapped) */
   "iiiip\0"
   "glGetCombinerInputParameterivNV\0"
   "\0"
   /* _mesa_function_pool[29274]: Disable (offset 214) */
   "i\0"
   "glDisable\0"
   "\0"
   /* _mesa_function_pool[29287]: MultiTexCoord2fvARB (offset 387) */
   "ip\0"
   "glMultiTexCoord2fv\0"
   "glMultiTexCoord2fvARB\0"
   "\0"
   /* _mesa_function_pool[29332]: GetRenderbufferParameterivEXT (will be remapped) */
   "iip\0"
   "glGetRenderbufferParameteriv\0"
   "glGetRenderbufferParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[29398]: CombinerParameterivNV (will be remapped) */
   "ip\0"
   "glCombinerParameterivNV\0"
   "\0"
   /* _mesa_function_pool[29426]: GenFragmentShadersATI (will be remapped) */
   "i\0"
   "glGenFragmentShadersATI\0"
   "\0"
   /* _mesa_function_pool[29453]: DrawArrays (offset 310) */
   "iii\0"
   "glDrawArrays\0"
   "glDrawArraysEXT\0"
   "\0"
   /* _mesa_function_pool[29487]: WeightuivARB (dynamic) */
   "ip\0"
   "glWeightuivARB\0"
   "\0"
   /* _mesa_function_pool[29506]: VertexAttrib2sARB (will be remapped) */
   "iii\0"
   "glVertexAttrib2s\0"
   "glVertexAttrib2sARB\0"
   "\0"
   /* _mesa_function_pool[29548]: ColorMask (offset 210) */
   "iiii\0"
   "glColorMask\0"
   "\0"
   /* _mesa_function_pool[29566]: GenAsyncMarkersSGIX (dynamic) */
   "i\0"
   "glGenAsyncMarkersSGIX\0"
   "\0"
   /* _mesa_function_pool[29591]: Tangent3svEXT (dynamic) */
   "p\0"
   "glTangent3svEXT\0"
   "\0"
   /* _mesa_function_pool[29610]: GetListParameterivSGIX (dynamic) */
   "iip\0"
   "glGetListParameterivSGIX\0"
   "\0"
   /* _mesa_function_pool[29640]: BindBufferARB (will be remapped) */
   "ii\0"
   "glBindBuffer\0"
   "glBindBufferARB\0"
   "\0"
   /* _mesa_function_pool[29673]: GetInfoLogARB (will be remapped) */
   "iipp\0"
   "glGetInfoLogARB\0"
   "\0"
   /* _mesa_function_pool[29695]: RasterPos4iv (offset 83) */
   "p\0"
   "glRasterPos4iv\0"
   "\0"
   /* _mesa_function_pool[29713]: Enable (offset 215) */
   "i\0"
   "glEnable\0"
   "\0"
   /* _mesa_function_pool[29725]: LineStipple (offset 167) */
   "ii\0"
   "glLineStipple\0"
   "\0"
   /* _mesa_function_pool[29743]: VertexAttribs4svNV (will be remapped) */
   "iip\0"
   "glVertexAttribs4svNV\0"
   "\0"
   /* _mesa_function_pool[29769]: EdgeFlagPointerListIBM (dynamic) */
   "ipi\0"
   "glEdgeFlagPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[29799]: UniformMatrix3x2fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix3x2fv\0"
   "\0"
   /* _mesa_function_pool[29826]: GetMinmaxParameterfv (offset 365) */
   "iip\0"
   "glGetMinmaxParameterfv\0"
   "glGetMinmaxParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[29880]: VertexAttrib1fvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib1fv\0"
   "glVertexAttrib1fvARB\0"
   "\0"
   /* _mesa_function_pool[29923]: GenBuffersARB (will be remapped) */
   "ip\0"
   "glGenBuffers\0"
   "glGenBuffersARB\0"
   "\0"
   /* _mesa_function_pool[29956]: VertexAttribs1svNV (will be remapped) */
   "iip\0"
   "glVertexAttribs1svNV\0"
   "\0"
   /* _mesa_function_pool[29982]: Vertex3fv (offset 137) */
   "p\0"
   "glVertex3fv\0"
   "\0"
   /* _mesa_function_pool[29997]: GetTexBumpParameterivATI (will be remapped) */
   "ip\0"
   "glGetTexBumpParameterivATI\0"
   "\0"
   /* _mesa_function_pool[30028]: Binormal3bEXT (dynamic) */
   "iii\0"
   "glBinormal3bEXT\0"
   "\0"
   /* _mesa_function_pool[30049]: FragmentMaterialivSGIX (dynamic) */
   "iip\0"
   "glFragmentMaterialivSGIX\0"
   "\0"
   /* _mesa_function_pool[30079]: IsRenderbufferEXT (will be remapped) */
   "i\0"
   "glIsRenderbuffer\0"
   "glIsRenderbufferEXT\0"
   "\0"
   /* _mesa_function_pool[30119]: GenProgramsNV (will be remapped) */
   "ip\0"
   "glGenProgramsARB\0"
   "glGenProgramsNV\0"
   "\0"
   /* _mesa_function_pool[30156]: VertexAttrib4dvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib4dvNV\0"
   "\0"
   /* _mesa_function_pool[30180]: EndFragmentShaderATI (will be remapped) */
   "\0"
   "glEndFragmentShaderATI\0"
   "\0"
   /* _mesa_function_pool[30205]: Binormal3iEXT (dynamic) */
   "iii\0"
   "glBinormal3iEXT\0"
   "\0"
   /* _mesa_function_pool[30226]: WindowPos2fMESA (will be remapped) */
   "ff\0"
   "glWindowPos2f\0"
   "glWindowPos2fARB\0"
   "glWindowPos2fMESA\0"
   "\0"
   ;

/* these functions need to be remapped */
static const struct {
   GLint pool_index;
   GLint remap_index;
} MESA_remap_table_functions[] = {
   {  1461, AttachShader_remap_index },
   {  8848, CreateProgram_remap_index },
   { 20720, CreateShader_remap_index },
   { 23050, DeleteProgram_remap_index },
   { 16566, DeleteShader_remap_index },
   { 21166, DetachShader_remap_index },
   { 16090, GetAttachedShaders_remap_index },
   {  4275, GetProgramInfoLog_remap_index },
   {   361, GetProgramiv_remap_index },
   {  5608, GetShaderInfoLog_remap_index },
   { 28171, GetShaderiv_remap_index },
   { 12054, IsProgram_remap_index },
   { 11089, IsShader_remap_index },
   {  8952, StencilFuncSeparate_remap_index },
   {  3487, StencilMaskSeparate_remap_index },
   {  6684, StencilOpSeparate_remap_index },
   { 20045, UniformMatrix2x3fv_remap_index },
   {  2615, UniformMatrix2x4fv_remap_index },
   { 29799, UniformMatrix3x2fv_remap_index },
   { 27561, UniformMatrix3x4fv_remap_index },
   { 14638, UniformMatrix4x2fv_remap_index },
   {  2937, UniformMatrix4x3fv_remap_index },
   {  8866, LoadTransposeMatrixdARB_remap_index },
   { 27900, LoadTransposeMatrixfARB_remap_index },
   {  4848, MultTransposeMatrixdARB_remap_index },
   { 21353, MultTransposeMatrixfARB_remap_index },
   {   172, SampleCoverageARB_remap_index },
   {  5032, CompressedTexImage1DARB_remap_index },
   { 21853, CompressedTexImage2DARB_remap_index },
   {  3550, CompressedTexImage3DARB_remap_index },
   { 16382, CompressedTexSubImage1DARB_remap_index },
   {  1880, CompressedTexSubImage2DARB_remap_index },
   { 18243, CompressedTexSubImage3DARB_remap_index },
   { 25843, GetCompressedTexImageARB_remap_index },
   {  3395, DisableVertexAttribArrayARB_remap_index },
   { 27173, EnableVertexAttribArrayARB_remap_index },
   { 28975, GetProgramEnvParameterdvARB_remap_index },
   { 21233, GetProgramEnvParameterfvARB_remap_index },
   { 24844, GetProgramLocalParameterdvARB_remap_index },
   {  7126, GetProgramLocalParameterfvARB_remap_index },
   { 16473, GetProgramStringARB_remap_index },
   { 25039, GetProgramivARB_remap_index },
   { 18438, GetVertexAttribdvARB_remap_index },
   { 14527, GetVertexAttribfvARB_remap_index },
   {  8761, GetVertexAttribivARB_remap_index },
   { 17319, ProgramEnvParameter4dARB_remap_index },
   { 22823, ProgramEnvParameter4dvARB_remap_index },
   { 15135, ProgramEnvParameter4fARB_remap_index },
   {  7989, ProgramEnvParameter4fvARB_remap_index },
   {  3513, ProgramLocalParameter4dARB_remap_index },
   { 11764, ProgramLocalParameter4dvARB_remap_index },
   { 26652, ProgramLocalParameter4fARB_remap_index },
   { 23368, ProgramLocalParameter4fvARB_remap_index },
   { 25629, ProgramStringARB_remap_index },
   { 17569, VertexAttrib1dARB_remap_index },
   { 14181, VertexAttrib1dvARB_remap_index },
   {  3688, VertexAttrib1fARB_remap_index },
   { 29880, VertexAttrib1fvARB_remap_index },
   {  6210, VertexAttrib1sARB_remap_index },
   {  2054, VertexAttrib1svARB_remap_index },
   { 13612, VertexAttrib2dARB_remap_index },
   { 15709, VertexAttrib2dvARB_remap_index },
   {  1480, VertexAttrib2fARB_remap_index },
   { 15822, VertexAttrib2fvARB_remap_index },
   { 29506, VertexAttrib2sARB_remap_index },
   { 28612, VertexAttrib2svARB_remap_index },
   { 10135, VertexAttrib3dARB_remap_index },
   {  7692, VertexAttrib3dvARB_remap_index },
   {  1567, VertexAttrib3fARB_remap_index },
   { 20308, VertexAttrib3fvARB_remap_index },
   { 25472, VertexAttrib3sARB_remap_index },
   { 18180, VertexAttrib3svARB_remap_index },
   {  4301, VertexAttrib4NbvARB_remap_index },
   { 16045, VertexAttrib4NivARB_remap_index },
   { 20263, VertexAttrib4NsvARB_remap_index },
   { 21185, VertexAttrib4NubARB_remap_index },
   { 28858, VertexAttrib4NubvARB_remap_index },
   { 16980, VertexAttrib4NuivARB_remap_index },
   {  2810, VertexAttrib4NusvARB_remap_index },
   {  9729, VertexAttrib4bvARB_remap_index },
   { 24252, VertexAttrib4dARB_remap_index },
   { 19202, VertexAttrib4dvARB_remap_index },
   { 10242, VertexAttrib4fARB_remap_index },
   { 10646, VertexAttrib4fvARB_remap_index },
   {  9145, VertexAttrib4ivARB_remap_index },
   { 15523, VertexAttrib4sARB_remap_index },
   { 28086, VertexAttrib4svARB_remap_index },
   { 14940, VertexAttrib4ubvARB_remap_index },
   { 27497, VertexAttrib4uivARB_remap_index },
   { 17991, VertexAttrib4usvARB_remap_index },
   { 19919, VertexAttribPointerARB_remap_index },
   { 29640, BindBufferARB_remap_index },
   {  5923, BufferDataARB_remap_index },
   {  1382, BufferSubDataARB_remap_index },
   { 27686, DeleteBuffersARB_remap_index },
   { 29923, GenBuffersARB_remap_index },
   { 15865, GetBufferParameterivARB_remap_index },
   { 15087, GetBufferPointervARB_remap_index },
   {  1335, GetBufferSubDataARB_remap_index },
   { 27445, IsBufferARB_remap_index },
   { 23825, MapBufferARB_remap_index },
   { 28301, UnmapBufferARB_remap_index },
   {   268, BeginQueryARB_remap_index },
   { 17664, DeleteQueriesARB_remap_index },
   { 10940, EndQueryARB_remap_index },
   { 26322, GenQueriesARB_remap_index },
   {  1772, GetQueryObjectivARB_remap_index },
   { 15567, GetQueryObjectuivARB_remap_index },
   {  1624, GetQueryivARB_remap_index },
   { 17898, IsQueryARB_remap_index },
   {  7302, AttachObjectARB_remap_index },
   { 16528, CompileShaderARB_remap_index },
   {  2879, CreateProgramObjectARB_remap_index },
   {  5868, CreateShaderObjectARB_remap_index },
   { 13029, DeleteObjectARB_remap_index },
   { 21627, DetachObjectARB_remap_index },
   { 10718, GetActiveUniformARB_remap_index },
   {  8464, GetAttachedObjectsARB_remap_index },
   {  8743, GetHandleARB_remap_index },
   { 29673, GetInfoLogARB_remap_index },
   { 28929, GetObjectParameterfvARB_remap_index },
   { 24718, GetObjectParameterivARB_remap_index },
   { 26080, GetShaderSourceARB_remap_index },
   { 25332, GetUniformLocationARB_remap_index },
   { 21455, GetUniformfvARB_remap_index },
   { 11386, GetUniformivARB_remap_index },
   { 18036, LinkProgramARB_remap_index },
   { 18094, ShaderSourceARB_remap_index },
   {  6584, Uniform1fARB_remap_index },
   { 26861, Uniform1fvARB_remap_index },
   { 19888, Uniform1iARB_remap_index },
   { 18891, Uniform1ivARB_remap_index },
   {  2003, Uniform2fARB_remap_index },
   { 12865, Uniform2fvARB_remap_index },
   { 23712, Uniform2iARB_remap_index },
   {  2123, Uniform2ivARB_remap_index },
   { 16638, Uniform3fARB_remap_index },
   {  8494, Uniform3fvARB_remap_index },
   {  5542, Uniform3iARB_remap_index },
   { 15193, Uniform3ivARB_remap_index },
   { 17125, Uniform4fARB_remap_index },
   { 21319, Uniform4fvARB_remap_index },
   { 22502, Uniform4iARB_remap_index },
   { 18404, Uniform4ivARB_remap_index },
   {  7354, UniformMatrix2fvARB_remap_index },
   {    17, UniformMatrix3fvARB_remap_index },
   {  2475, UniformMatrix4fvARB_remap_index },
   { 22935, UseProgramObjectARB_remap_index },
   { 13300, ValidateProgramARB_remap_index },
   { 19245, BindAttribLocationARB_remap_index },
   {  4346, GetActiveAttribARB_remap_index },
   { 14874, GetAttribLocationARB_remap_index },
   { 26600, DrawBuffersARB_remap_index },
   { 11869, RenderbufferStorageMultisample_remap_index },
   { 17173, FlushMappedBufferRange_remap_index },
   { 25135, MapBufferRange_remap_index },
   { 14749, BindVertexArray_remap_index },
   { 13159, GenVertexArrays_remap_index },
   { 27375, CopyBufferSubData_remap_index },
   { 28190, ClientWaitSync_remap_index },
   {  2394, DeleteSync_remap_index },
   {  6251, FenceSync_remap_index },
   { 13671, GetInteger64v_remap_index },
   { 20370, GetSynciv_remap_index },
   { 26539, IsSync_remap_index },
   {  8412, WaitSync_remap_index },
   {  3363, DrawElementsBaseVertex_remap_index },
   { 27618, DrawRangeElementsBaseVertex_remap_index },
   { 23856, MultiDrawElementsBaseVertex_remap_index },
   {  4711, PolygonOffsetEXT_remap_index },
   { 20955, GetPixelTexGenParameterfvSGIS_remap_index },
   {  3895, GetPixelTexGenParameterivSGIS_remap_index },
   { 20688, PixelTexGenParameterfSGIS_remap_index },
   {   580, PixelTexGenParameterfvSGIS_remap_index },
   { 11424, PixelTexGenParameteriSGIS_remap_index },
   { 12385, PixelTexGenParameterivSGIS_remap_index },
   { 14837, SampleMaskSGIS_remap_index },
   { 17838, SamplePatternSGIS_remap_index },
   { 23785, ColorPointerEXT_remap_index },
   { 15752, EdgeFlagPointerEXT_remap_index },
   {  5196, IndexPointerEXT_remap_index },
   {  5276, NormalPointerEXT_remap_index },
   { 14265, TexCoordPointerEXT_remap_index },
   {  6046, VertexPointerEXT_remap_index },
   {  3165, PointParameterfEXT_remap_index },
   {  6891, PointParameterfvEXT_remap_index },
   { 29027, LockArraysEXT_remap_index },
   { 13364, UnlockArraysEXT_remap_index },
   {  7898, CullParameterdvEXT_remap_index },
   { 10513, CullParameterfvEXT_remap_index },
   {  1151, SecondaryColor3bEXT_remap_index },
   {  7050, SecondaryColor3bvEXT_remap_index },
   {  9322, SecondaryColor3dEXT_remap_index },
   { 23108, SecondaryColor3dvEXT_remap_index },
   { 25381, SecondaryColor3fEXT_remap_index },
   { 16318, SecondaryColor3fvEXT_remap_index },
   {   426, SecondaryColor3iEXT_remap_index },
   { 14575, SecondaryColor3ivEXT_remap_index },
   {  8980, SecondaryColor3sEXT_remap_index },
   { 27854, SecondaryColor3svEXT_remap_index },
   { 24554, SecondaryColor3ubEXT_remap_index },
   { 19136, SecondaryColor3ubvEXT_remap_index },
   { 11619, SecondaryColor3uiEXT_remap_index },
   { 20575, SecondaryColor3uivEXT_remap_index },
   { 23320, SecondaryColor3usEXT_remap_index },
   { 11692, SecondaryColor3usvEXT_remap_index },
   { 10589, SecondaryColorPointerEXT_remap_index },
   { 23169, MultiDrawArraysEXT_remap_index },
   { 18826, MultiDrawElementsEXT_remap_index },
   { 19021, FogCoordPointerEXT_remap_index },
   {  4044, FogCoorddEXT_remap_index },
   { 28428, FogCoorddvEXT_remap_index },
   {  4136, FogCoordfEXT_remap_index },
   { 24477, FogCoordfvEXT_remap_index },
   { 17077, PixelTexGenSGIX_remap_index },
   { 25062, BlendFuncSeparateEXT_remap_index },
   {  5958, FlushVertexArrayRangeNV_remap_index },
   {  4660, VertexArrayRangeNV_remap_index },
   { 25446, CombinerInputNV_remap_index },
   {  1946, CombinerOutputNV_remap_index },
   { 28007, CombinerParameterfNV_remap_index },
   {  4580, CombinerParameterfvNV_remap_index },
   { 20094, CombinerParameteriNV_remap_index },
   { 29398, CombinerParameterivNV_remap_index },
   {  6328, FinalCombinerInputNV_remap_index },
   {  8809, GetCombinerInputParameterfvNV_remap_index },
   { 29235, GetCombinerInputParameterivNV_remap_index },
   {  6127, GetCombinerOutputParameterfvNV_remap_index },
   { 12346, GetCombinerOutputParameterivNV_remap_index },
   {  5703, GetFinalCombinerInputParameterfvNV_remap_index },
   { 22374, GetFinalCombinerInputParameterivNV_remap_index },
   { 11364, ResizeBuffersMESA_remap_index },
   {  9962, WindowPos2dMESA_remap_index },
   {   944, WindowPos2dvMESA_remap_index },
   { 30226, WindowPos2fMESA_remap_index },
   {  6995, WindowPos2fvMESA_remap_index },
   { 16265, WindowPos2iMESA_remap_index },
   { 18311, WindowPos2ivMESA_remap_index },
   { 18925, WindowPos2sMESA_remap_index },
   {  4946, WindowPos2svMESA_remap_index },
   {  6820, WindowPos3dMESA_remap_index },
   { 12593, WindowPos3dvMESA_remap_index },
   {   472, WindowPos3fMESA_remap_index },
   { 13425, WindowPos3fvMESA_remap_index },
   { 21669, WindowPos3iMESA_remap_index },
   { 27320, WindowPos3ivMESA_remap_index },
   { 16783, WindowPos3sMESA_remap_index },
   { 28684, WindowPos3svMESA_remap_index },
   {  9913, WindowPos4dMESA_remap_index },
   { 15278, WindowPos4dvMESA_remap_index },
   { 12552, WindowPos4fMESA_remap_index },
   { 27761, WindowPos4fvMESA_remap_index },
   { 27473, WindowPos4iMESA_remap_index },
   { 11203, WindowPos4ivMESA_remap_index },
   { 16956, WindowPos4sMESA_remap_index },
   {  2857, WindowPos4svMESA_remap_index },
   { 24220, MultiModeDrawArraysIBM_remap_index },
   { 26193, MultiModeDrawElementsIBM_remap_index },
   { 10968, DeleteFencesNV_remap_index },
   { 25293, FinishFenceNV_remap_index },
   {  3287, GenFencesNV_remap_index },
   { 15258, GetFenceivNV_remap_index },
   {  7287, IsFenceNV_remap_index },
   { 12273, SetFenceNV_remap_index },
   {  3744, TestFenceNV_remap_index },
   { 28655, AreProgramsResidentNV_remap_index },
   { 28049, BindProgramNV_remap_index },
   { 23403, DeleteProgramsNV_remap_index },
   { 19354, ExecuteProgramNV_remap_index },
   { 30119, GenProgramsNV_remap_index },
   { 21034, GetProgramParameterdvNV_remap_index },
   {  9384, GetProgramParameterfvNV_remap_index },
   { 23759, GetProgramStringNV_remap_index },
   { 22063, GetProgramivNV_remap_index },
   { 21268, GetTrackMatrixivNV_remap_index },
   { 23553, GetVertexAttribPointervNV_remap_index },
   { 22307, GetVertexAttribdvNV_remap_index },
   {  8307, GetVertexAttribfvNV_remap_index },
   { 16446, GetVertexAttribivNV_remap_index },
   { 17203, IsProgramNV_remap_index },
   {  8390, LoadProgramNV_remap_index },
   { 25158, ProgramParameters4dvNV_remap_index },
   { 21993, ProgramParameters4fvNV_remap_index },
   { 18615, RequestResidentProgramsNV_remap_index },
   { 20072, TrackMatrixNV_remap_index },
   { 29212, VertexAttrib1dNV_remap_index },
   { 12214, VertexAttrib1dvNV_remap_index },
   { 25725, VertexAttrib1fNV_remap_index },
   {  2245, VertexAttrib1fvNV_remap_index },
   { 27818, VertexAttrib1sNV_remap_index },
   { 13498, VertexAttrib1svNV_remap_index },
   {  4251, VertexAttrib2dNV_remap_index },
   { 12129, VertexAttrib2dvNV_remap_index },
   { 18070, VertexAttrib2fNV_remap_index },
   { 11740, VertexAttrib2fvNV_remap_index },
   {  5106, VertexAttrib2sNV_remap_index },
   { 16837, VertexAttrib2svNV_remap_index },
   { 10110, VertexAttrib3dNV_remap_index },
   { 28905, VertexAttrib3dvNV_remap_index },
   {  9196, VertexAttrib3fNV_remap_index },
   { 22334, VertexAttrib3fvNV_remap_index },
   { 25700, VertexAttrib3sNV_remap_index },
   { 21295, VertexAttrib3svNV_remap_index },
   { 26167, VertexAttrib4dNV_remap_index },
   { 30156, VertexAttrib4dvNV_remap_index },
   {  3945, VertexAttrib4fNV_remap_index },
   {  8440, VertexAttrib4fvNV_remap_index },
   { 24104, VertexAttrib4sNV_remap_index },
   {  1293, VertexAttrib4svNV_remap_index },
   {  4409, VertexAttrib4ubNV_remap_index },
   {   734, VertexAttrib4ubvNV_remap_index },
   { 19534, VertexAttribPointerNV_remap_index },
   {  2097, VertexAttribs1dvNV_remap_index },
   { 23641, VertexAttribs1fvNV_remap_index },
   { 29956, VertexAttribs1svNV_remap_index },
   {  9221, VertexAttribs2dvNV_remap_index },
   { 22896, VertexAttribs2fvNV_remap_index },
   { 15778, VertexAttribs2svNV_remap_index },
   {  4608, VertexAttribs3dvNV_remap_index },
   {  1977, VertexAttribs3fvNV_remap_index },
   { 27068, VertexAttribs3svNV_remap_index },
   { 24194, VertexAttribs4dvNV_remap_index },
   {  4634, VertexAttribs4fvNV_remap_index },
   { 29743, VertexAttribs4svNV_remap_index },
   { 26816, VertexAttribs4ubvNV_remap_index },
   { 24296, GetTexBumpParameterfvATI_remap_index },
   { 29997, GetTexBumpParameterivATI_remap_index },
   { 16500, TexBumpParameterfvATI_remap_index },
   { 18486, TexBumpParameterivATI_remap_index },
   { 14044, AlphaFragmentOp1ATI_remap_index },
   {  9772, AlphaFragmentOp2ATI_remap_index },
   { 22250, AlphaFragmentOp3ATI_remap_index },
   { 26995, BeginFragmentShaderATI_remap_index },
   { 28248, BindFragmentShaderATI_remap_index },
   { 21424, ColorFragmentOp1ATI_remap_index },
   {  3823, ColorFragmentOp2ATI_remap_index },
   { 28550, ColorFragmentOp3ATI_remap_index },
   {  4753, DeleteFragmentShaderATI_remap_index },
   { 30180, EndFragmentShaderATI_remap_index },
   { 29426, GenFragmentShadersATI_remap_index },
   { 23027, PassTexCoordATI_remap_index },
   {  6026, SampleMapATI_remap_index },
   {  5799, SetFragmentShaderConstantATI_remap_index },
   {   319, PointParameteriNV_remap_index },
   { 12754, PointParameterivNV_remap_index },
   { 26006, ActiveStencilFaceEXT_remap_index },
   { 24818, BindVertexArrayAPPLE_remap_index },
   {  2522, DeleteVertexArraysAPPLE_remap_index },
   { 16117, GenVertexArraysAPPLE_remap_index },
   { 21099, IsVertexArrayAPPLE_remap_index },
   {   775, GetProgramNamedParameterdvNV_remap_index },
   {  3128, GetProgramNamedParameterfvNV_remap_index },
   { 24327, ProgramNamedParameter4dNV_remap_index },
   { 13080, ProgramNamedParameter4dvNV_remap_index },
   {  7923, ProgramNamedParameter4fNV_remap_index },
   { 10554, ProgramNamedParameter4fvNV_remap_index },
   { 21972, DepthBoundsEXT_remap_index },
   {  1043, BlendEquationSeparateEXT_remap_index },
   { 13199, BindFramebufferEXT_remap_index },
   { 23214, BindRenderbufferEXT_remap_index },
   {  8659, CheckFramebufferStatusEXT_remap_index },
   { 20389, DeleteFramebuffersEXT_remap_index },
   { 28807, DeleteRenderbuffersEXT_remap_index },
   { 12153, FramebufferRenderbufferEXT_remap_index },
   { 12290, FramebufferTexture1DEXT_remap_index },
   { 10348, FramebufferTexture2DEXT_remap_index },
   { 10015, FramebufferTexture3DEXT_remap_index },
   { 20991, GenFramebuffersEXT_remap_index },
   { 15664, GenRenderbuffersEXT_remap_index },
   {  5745, GenerateMipmapEXT_remap_index },
   { 19594, GetFramebufferAttachmentParameterivEXT_remap_index },
   { 29332, GetRenderbufferParameterivEXT_remap_index },
   { 18366, IsFramebufferEXT_remap_index },
   { 30079, IsRenderbufferEXT_remap_index },
   {  7234, RenderbufferStorageEXT_remap_index },
   {   651, BlitFramebufferEXT_remap_index },
   { 12899, BufferParameteriAPPLE_remap_index },
   { 17235, FlushMappedBufferRangeAPPLE_remap_index },
   {  2701, FramebufferTextureLayerEXT_remap_index },
   { 25515, ColorMaskIndexedEXT_remap_index },
   { 16861, DisableIndexedEXT_remap_index },
   { 23951, EnableIndexedEXT_remap_index },
   { 19565, GetBooleanIndexedvEXT_remap_index },
   {  9805, GetIntegerIndexedvEXT_remap_index },
   { 20465, IsEnabledIndexedEXT_remap_index },
   {  4074, BeginConditionalRenderNV_remap_index },
   { 23000, EndConditionalRenderNV_remap_index },
   {  8334, BeginTransformFeedbackEXT_remap_index },
   { 16885, BindBufferBaseEXT_remap_index },
   { 16755, BindBufferOffsetEXT_remap_index },
   { 11028, BindBufferRangeEXT_remap_index },
   { 12814, EndTransformFeedbackEXT_remap_index },
   {  9657, GetTransformFeedbackVaryingEXT_remap_index },
   { 18671, TransformFeedbackVaryingsEXT_remap_index },
   { 26717, ProvokingVertexEXT_remap_index },
   {  9605, GetTexParameterPointervAPPLE_remap_index },
   {  4436, TextureRangeAPPLE_remap_index },
   { 10420, GetObjectParameterivAPPLE_remap_index },
   { 17810, ObjectPurgeableAPPLE_remap_index },
   {  4900, ObjectUnpurgeableAPPLE_remap_index },
   { 26032, StencilFuncSeparateATI_remap_index },
   { 16184, ProgramEnvParameters4fvEXT_remap_index },
   { 15402, ProgramLocalParameters4fvEXT_remap_index },
   { 12682, GetQueryObjecti64vEXT_remap_index },
   {  9247, GetQueryObjectui64vEXT_remap_index },
   { 21493, EGLImageTargetRenderbufferStorageOES_remap_index },
   { 10907, EGLImageTargetTexture2DOES_remap_index },
   {    -1, -1 }
};

/* these functions are in the ABI, but have alternative names */
static const struct gl_function_remap MESA_alt_functions[] = {
   /* from GL_EXT_blend_color */
   {  2440, _gloffset_BlendColor },
   /* from GL_EXT_blend_minmax */
   { 10072, _gloffset_BlendEquation },
   /* from GL_EXT_color_subtable */
   { 15300, _gloffset_ColorSubTable },
   { 28739, _gloffset_CopyColorSubTable },
   /* from GL_EXT_convolution */
   {   213, _gloffset_ConvolutionFilter1D },
   {  2284, _gloffset_CopyConvolutionFilter1D },
   {  3624, _gloffset_GetConvolutionParameteriv },
   {  7583, _gloffset_ConvolutionFilter2D },
   {  7749, _gloffset_ConvolutionParameteriv },
   {  8209, _gloffset_ConvolutionParameterfv },
   { 18514, _gloffset_GetSeparableFilter },
   { 21723, _gloffset_SeparableFilter2D },
   { 22552, _gloffset_ConvolutionParameteri },
   { 22675, _gloffset_ConvolutionParameterf },
   { 24130, _gloffset_GetConvolutionParameterfv },
   { 24984, _gloffset_GetConvolutionFilter },
   { 27257, _gloffset_CopyConvolutionFilter2D },
   /* from GL_EXT_copy_texture */
   { 13558, _gloffset_CopyTexSubImage3D },
   { 15040, _gloffset_CopyTexImage2D },
   { 22160, _gloffset_CopyTexImage1D },
   { 24665, _gloffset_CopyTexSubImage2D },
   { 26895, _gloffset_CopyTexSubImage1D },
   /* from GL_EXT_draw_range_elements */
   {  8546, _gloffset_DrawRangeElements },
   /* from GL_EXT_histogram */
   {   812, _gloffset_Histogram },
   {  3088, _gloffset_ResetHistogram },
   {  8918, _gloffset_GetMinmax },
   { 13892, _gloffset_GetHistogramParameterfv },
   { 22085, _gloffset_GetMinmaxParameteriv },
   { 24020, _gloffset_ResetMinmax },
   { 24881, _gloffset_GetHistogramParameteriv },
   { 25966, _gloffset_GetHistogram },
   { 28364, _gloffset_Minmax },
   { 29826, _gloffset_GetMinmaxParameterfv },
   /* from GL_EXT_paletted_texture */
   {  7445, _gloffset_ColorTable },
   { 13738, _gloffset_GetColorTable },
   { 20738, _gloffset_GetColorTableParameterfv },
   { 22731, _gloffset_GetColorTableParameteriv },
   /* from GL_EXT_subtexture */
   {  6166, _gloffset_TexSubImage1D },
   {  9532, _gloffset_TexSubImage2D },
   /* from GL_EXT_texture3D */
   {  1658, _gloffset_TexImage3D },
   { 20507, _gloffset_TexSubImage3D },
   /* from GL_EXT_texture_object */
   {  2964, _gloffset_PrioritizeTextures },
   {  6615, _gloffset_AreTexturesResident },
   { 12238, _gloffset_GenTextures },
   { 14224, _gloffset_DeleteTextures },
   { 17516, _gloffset_IsTexture },
   { 26960, _gloffset_BindTexture },
   /* from GL_EXT_vertex_array */
   { 21912, _gloffset_ArrayElement },
   { 27952, _gloffset_GetPointerv },
   { 29453, _gloffset_DrawArrays },
   /* from GL_SGI_color_table */
   {  6733, _gloffset_ColorTableParameteriv },
   {  7445, _gloffset_ColorTable },
   { 13738, _gloffset_GetColorTable },
   { 13848, _gloffset_CopyColorTable },
   { 17377, _gloffset_ColorTableParameterfv },
   { 20738, _gloffset_GetColorTableParameterfv },
   { 22731, _gloffset_GetColorTableParameteriv },
   /* from GL_VERSION_1_3 */
   {   381, _gloffset_MultiTexCoord3sARB },
   {   613, _gloffset_ActiveTextureARB },
   {  3761, _gloffset_MultiTexCoord1fvARB },
   {  5301, _gloffset_MultiTexCoord3dARB },
   {  5346, _gloffset_MultiTexCoord2iARB },
   {  5470, _gloffset_MultiTexCoord2svARB },
   {  7401, _gloffset_MultiTexCoord2fARB },
   {  9277, _gloffset_MultiTexCoord3fvARB },
   {  9834, _gloffset_MultiTexCoord4sARB },
   { 10468, _gloffset_MultiTexCoord2dvARB },
   { 10850, _gloffset_MultiTexCoord1svARB },
   { 11225, _gloffset_MultiTexCoord3svARB },
   { 11286, _gloffset_MultiTexCoord4iARB },
   { 12009, _gloffset_MultiTexCoord3iARB },
   { 12711, _gloffset_MultiTexCoord1dARB },
   { 12928, _gloffset_MultiTexCoord3dvARB },
   { 14092, _gloffset_MultiTexCoord3ivARB },
   { 14137, _gloffset_MultiTexCoord2sARB },
   { 15357, _gloffset_MultiTexCoord4ivARB },
   { 17027, _gloffset_ClientActiveTextureARB },
   { 19310, _gloffset_MultiTexCoord2dARB },
   { 19714, _gloffset_MultiTexCoord4dvARB },
   { 20000, _gloffset_MultiTexCoord4fvARB },
   { 20879, _gloffset_MultiTexCoord3fARB },
   { 23259, _gloffset_MultiTexCoord4dARB },
   { 23463, _gloffset_MultiTexCoord1sARB },
   { 23667, _gloffset_MultiTexCoord1dvARB },
   { 24509, _gloffset_MultiTexCoord1ivARB },
   { 24602, _gloffset_MultiTexCoord2ivARB },
   { 24941, _gloffset_MultiTexCoord1iARB },
   { 26241, _gloffset_MultiTexCoord4svARB },
   { 26759, _gloffset_MultiTexCoord1fARB },
   { 27022, _gloffset_MultiTexCoord4fARB },
   { 29287, _gloffset_MultiTexCoord2fvARB },
   {    -1, -1 }
};

#endif /* need_MESA_remap_table */

#if defined(need_GL_3DFX_tbuffer)
static const struct gl_function_remap GL_3DFX_tbuffer_functions[] = {
   {  8267, -1 }, /* TbufferMask3DFX */
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

#if defined(need_GL_ARB_framebuffer_object)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_framebuffer_object_functions[] = {
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
   { 11830, -1 }, /* MatrixIndexuivARB */
   { 13050, -1 }, /* MatrixIndexPointerARB */
   { 17765, -1 }, /* CurrentPaletteMatrixARB */
   { 20623, -1 }, /* MatrixIndexubvARB */
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
   {  5633, -1 }, /* WeightivARB */
   {  9937, -1 }, /* WeightPointerARB */
   { 12468, -1 }, /* WeightfvARB */
   { 15804, -1 }, /* WeightbvARB */
   { 18978, -1 }, /* WeightusvARB */
   { 21649, -1 }, /* VertexBlendARB */
   { 26843, -1 }, /* WeightsvARB */
   { 28789, -1 }, /* WeightdvARB */
   { 29487, -1 }, /* WeightuivARB */
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
   { 10072, _gloffset_BlendEquation },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_color_subtable)
static const struct gl_function_remap GL_EXT_color_subtable_functions[] = {
   { 15300, _gloffset_ColorSubTable },
   { 28739, _gloffset_CopyColorSubTable },
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
   {  7583, _gloffset_ConvolutionFilter2D },
   {  7749, _gloffset_ConvolutionParameteriv },
   {  8209, _gloffset_ConvolutionParameterfv },
   { 18514, _gloffset_GetSeparableFilter },
   { 21723, _gloffset_SeparableFilter2D },
   { 22552, _gloffset_ConvolutionParameteri },
   { 22675, _gloffset_ConvolutionParameterf },
   { 24130, _gloffset_GetConvolutionParameterfv },
   { 24984, _gloffset_GetConvolutionFilter },
   { 27257, _gloffset_CopyConvolutionFilter2D },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const struct gl_function_remap GL_EXT_coordinate_frame_functions[] = {
   {  9416, -1 }, /* TangentPointerEXT */
   { 11344, -1 }, /* Binormal3ivEXT */
   { 11962, -1 }, /* Tangent3sEXT */
   { 13115, -1 }, /* Tangent3fvEXT */
   { 16736, -1 }, /* Tangent3dvEXT */
   { 17463, -1 }, /* Binormal3bvEXT */
   { 18567, -1 }, /* Binormal3dEXT */
   { 20555, -1 }, /* Tangent3fEXT */
   { 22624, -1 }, /* Binormal3sEXT */
   { 23069, -1 }, /* Tangent3ivEXT */
   { 23088, -1 }, /* Tangent3dEXT */
   { 23894, -1 }, /* Binormal3svEXT */
   { 24407, -1 }, /* Binormal3fEXT */
   { 25259, -1 }, /* Binormal3dvEXT */
   { 26463, -1 }, /* Tangent3iEXT */
   { 27542, -1 }, /* Tangent3bvEXT */
   { 27987, -1 }, /* Tangent3bEXT */
   { 28512, -1 }, /* Binormal3fvEXT */
   { 29186, -1 }, /* BinormalPointerEXT */
   { 29591, -1 }, /* Tangent3svEXT */
   { 30028, -1 }, /* Binormal3bEXT */
   { 30205, -1 }, /* Binormal3iEXT */
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_copy_texture)
static const struct gl_function_remap GL_EXT_copy_texture_functions[] = {
   { 13558, _gloffset_CopyTexSubImage3D },
   { 15040, _gloffset_CopyTexImage2D },
   { 22160, _gloffset_CopyTexImage1D },
   { 24665, _gloffset_CopyTexSubImage2D },
   { 26895, _gloffset_CopyTexSubImage1D },
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

#if defined(need_GL_EXT_draw_range_elements)
static const struct gl_function_remap GL_EXT_draw_range_elements_functions[] = {
   {  8546, _gloffset_DrawRangeElements },
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
   {  8918, _gloffset_GetMinmax },
   { 13892, _gloffset_GetHistogramParameterfv },
   { 22085, _gloffset_GetMinmaxParameteriv },
   { 24020, _gloffset_ResetMinmax },
   { 24881, _gloffset_GetHistogramParameteriv },
   { 25966, _gloffset_GetHistogram },
   { 28364, _gloffset_Minmax },
   { 29826, _gloffset_GetMinmaxParameterfv },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_index_func)
static const struct gl_function_remap GL_EXT_index_func_functions[] = {
   { 10299, -1 }, /* IndexFuncEXT */
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_index_material)
static const struct gl_function_remap GL_EXT_index_material_functions[] = {
   { 19065, -1 }, /* IndexMaterialEXT */
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_light_texture)
static const struct gl_function_remap GL_EXT_light_texture_functions[] = {
   { 23914, -1 }, /* ApplyTextureEXT */
   { 23974, -1 }, /* TextureMaterialEXT */
   { 23999, -1 }, /* TextureLightEXT */
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
   {  7445, _gloffset_ColorTable },
   { 13738, _gloffset_GetColorTable },
   { 20738, _gloffset_GetColorTableParameterfv },
   { 22731, _gloffset_GetColorTableParameteriv },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_pixel_transform)
static const struct gl_function_remap GL_EXT_pixel_transform_functions[] = {
   { 19679, -1 }, /* PixelTransformParameterfEXT */
   { 19759, -1 }, /* PixelTransformParameteriEXT */
   { 27725, -1 }, /* PixelTransformParameterfvEXT */
   { 29150, -1 }, /* PixelTransformParameterivEXT */
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
   {  6166, _gloffset_TexSubImage1D },
   {  9532, _gloffset_TexSubImage2D },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_texture3D)
static const struct gl_function_remap GL_EXT_texture3D_functions[] = {
   {  1658, _gloffset_TexImage3D },
   { 20507, _gloffset_TexSubImage3D },
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
   {  6615, _gloffset_AreTexturesResident },
   { 12238, _gloffset_GenTextures },
   { 14224, _gloffset_DeleteTextures },
   { 17516, _gloffset_IsTexture },
   { 26960, _gloffset_BindTexture },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_texture_perturb_normal)
static const struct gl_function_remap GL_EXT_texture_perturb_normal_functions[] = {
   { 12418, -1 }, /* TextureNormalEXT */
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
   { 21912, _gloffset_ArrayElement },
   { 27952, _gloffset_GetPointerv },
   { 29453, _gloffset_DrawArrays },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_vertex_weighting)
static const struct gl_function_remap GL_EXT_vertex_weighting_functions[] = {
   { 17546, -1 }, /* VertexWeightfvEXT */
   { 24385, -1 }, /* VertexWeightfEXT */
   { 25935, -1 }, /* VertexWeightPointerEXT */
   {    -1, -1 }
};
#endif

#if defined(need_GL_HP_image_transform)
static const struct gl_function_remap GL_HP_image_transform_functions[] = {
   {  2157, -1 }, /* GetImageTransformParameterfvHP */
   {  3305, -1 }, /* ImageTransformParameterfHP */
   {  9110, -1 }, /* ImageTransformParameterfvHP */
   { 10768, -1 }, /* ImageTransformParameteriHP */
   { 11115, -1 }, /* GetImageTransformParameterivHP */
   { 17610, -1 }, /* ImageTransformParameterivHP */
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
   {  5167, -1 }, /* NormalPointerListIBM */
   {  6789, -1 }, /* FogCoordPointerListIBM */
   {  7096, -1 }, /* VertexPointerListIBM */
   { 10689, -1 }, /* ColorPointerListIBM */
   { 12069, -1 }, /* TexCoordPointerListIBM */
   { 12440, -1 }, /* IndexPointerListIBM */
   { 29769, -1 }, /* EdgeFlagPointerListIBM */
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
   { 11456, -1 }, /* VertexPointervINTEL */
   { 13985, -1 }, /* ColorPointervINTEL */
   { 27231, -1 }, /* NormalPointervINTEL */
   { 27657, -1 }, /* TexCoordPointervINTEL */
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
   { 28145, -1 }, /* CreateDebugObjectMESA */
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
   {  5834, -1 }, /* GetMapAttribParameterivNV */
   {  7551, -1 }, /* MapControlPointsNV */
   {  7650, -1 }, /* MapParameterfvNV */
   {  9515, -1 }, /* EvalMapsNV */
   { 15474, -1 }, /* GetMapAttribParameterfvNV */
   { 15640, -1 }, /* MapParameterivNV */
   { 22475, -1 }, /* GetMapParameterivNV */
   { 22973, -1 }, /* GetMapParameterfvNV */
   { 26567, -1 }, /* GetMapControlPointsNV */
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
   { 14377, -1 }, /* CombinerStageParameterfvNV */
   { 14692, -1 }, /* GetCombinerStageParameterfvNV */
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
   {  7735, -1 }, /* HintPGI */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_detail_texture)
static const struct gl_function_remap GL_SGIS_detail_texture_functions[] = {
   { 14665, -1 }, /* GetDetailTexFuncSGIS */
   { 14985, -1 }, /* DetailTexFuncSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_fog_function)
static const struct gl_function_remap GL_SGIS_fog_function_functions[] = {
   { 24647, -1 }, /* FogFuncSGIS */
   { 25312, -1 }, /* GetFogFuncSGIS */
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
   {  5895, -1 }, /* GetSharpenTexFuncSGIS */
   { 19974, -1 }, /* SharpenTexFuncSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_texture4D)
static const struct gl_function_remap GL_SGIS_texture4D_functions[] = {
   {   894, -1 }, /* TexImage4DSGIS */
   { 14293, -1 }, /* TexSubImage4DSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_texture_color_mask)
static const struct gl_function_remap GL_SGIS_texture_color_mask_functions[] = {
   { 13691, -1 }, /* TextureColorMaskSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_texture_filter4)
static const struct gl_function_remap GL_SGIS_texture_filter4_functions[] = {
   {  6072, -1 }, /* GetTexFilterFuncSGIS */
   { 14811, -1 }, /* TexFilterFuncSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_async)
static const struct gl_function_remap GL_SGIX_async_functions[] = {
   {  3014, -1 }, /* AsyncMarkerSGIX */
   {  3997, -1 }, /* FinishAsyncSGIX */
   {  4734, -1 }, /* PollAsyncSGIX */
   { 20121, -1 }, /* DeleteAsyncMarkersSGIX */
   { 20176, -1 }, /* IsAsyncMarkerSGIX */
   { 29566, -1 }, /* GenAsyncMarkersSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_flush_raster)
static const struct gl_function_remap GL_SGIX_flush_raster_functions[] = {
   {  6443, -1 }, /* FlushRasterSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_fragment_lighting)
static const struct gl_function_remap GL_SGIX_fragment_lighting_functions[] = {
   {  2410, -1 }, /* FragmentMaterialfvSGIX */
   {  2906, -1 }, /* FragmentLightModelivSGIX */
   {  4685, -1 }, /* FragmentLightiSGIX */
   {  5575, -1 }, /* GetFragmentMaterialfvSGIX */
   {  7163, -1 }, /* FragmentMaterialfSGIX */
   {  7324, -1 }, /* GetFragmentLightivSGIX */
   {  8161, -1 }, /* FragmentLightModeliSGIX */
   {  9578, -1 }, /* FragmentLightivSGIX */
   {  9880, -1 }, /* GetFragmentMaterialivSGIX */
   { 17433, -1 }, /* FragmentLightModelfSGIX */
   { 17733, -1 }, /* FragmentColorMaterialSGIX */
   { 18133, -1 }, /* FragmentMaterialiSGIX */
   { 19393, -1 }, /* LightEnviSGIX */
   { 20830, -1 }, /* FragmentLightModelfvSGIX */
   { 21139, -1 }, /* FragmentLightfvSGIX */
   { 25817, -1 }, /* FragmentLightfSGIX */
   { 28482, -1 }, /* GetFragmentLightfvSGIX */
   { 30049, -1 }, /* FragmentMaterialivSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_framezoom)
static const struct gl_function_remap GL_SGIX_framezoom_functions[] = {
   { 20199, -1 }, /* FrameZoomSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_igloo_interface)
static const struct gl_function_remap GL_SGIX_igloo_interface_functions[] = {
   { 26125, -1 }, /* IglooInterfaceSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_instruments)
static const struct gl_function_remap GL_SGIX_instruments_functions[] = {
   {  2573, -1 }, /* ReadInstrumentsSGIX */
   {  5651, -1 }, /* PollInstrumentsSGIX */
   {  9476, -1 }, /* GetInstrumentsSGIX */
   { 11667, -1 }, /* StartInstrumentsSGIX */
   { 14411, -1 }, /* StopInstrumentsSGIX */
   { 16017, -1 }, /* InstrumentsBufferSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_list_priority)
static const struct gl_function_remap GL_SGIX_list_priority_functions[] = {
   {  1125, -1 }, /* ListParameterfSGIX */
   {  2763, -1 }, /* GetListParameterfvSGIX */
   { 15932, -1 }, /* ListParameteriSGIX */
   { 16686, -1 }, /* ListParameterfvSGIX */
   { 18799, -1 }, /* ListParameterivSGIX */
   { 29610, -1 }, /* GetListParameterivSGIX */
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
   { 10989, -1 }, /* DeformationMap3dSGIX */
   { 14511, -1 }, /* DeformSGIX */
   { 22024, -1 }, /* DeformationMap3fSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_reference_plane)
static const struct gl_function_remap GL_SGIX_reference_plane_functions[] = {
   { 13242, -1 }, /* ReferencePlaneSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_sprite)
static const struct gl_function_remap GL_SGIX_sprite_functions[] = {
   {  8631, -1 }, /* SpriteParameterfvSGIX */
   { 18588, -1 }, /* SpriteParameteriSGIX */
   { 24054, -1 }, /* SpriteParameterfSGIX */
   { 26689, -1 }, /* SpriteParameterivSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_tag_sample_buffer)
static const struct gl_function_remap GL_SGIX_tag_sample_buffer_functions[] = {
   { 18647, -1 }, /* TagSampleBufferSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGI_color_table)
static const struct gl_function_remap GL_SGI_color_table_functions[] = {
   {  6733, _gloffset_ColorTableParameteriv },
   {  7445, _gloffset_ColorTable },
   { 13738, _gloffset_GetColorTable },
   { 13848, _gloffset_CopyColorTable },
   { 17377, _gloffset_ColorTableParameterfv },
   { 20738, _gloffset_GetColorTableParameterfv },
   { 22731, _gloffset_GetColorTableParameteriv },
   {    -1, -1 }
};
#endif

#if defined(need_GL_SUNX_constant_data)
static const struct gl_function_remap GL_SUNX_constant_data_functions[] = {
   { 28460, -1 }, /* FinishTextureSUNX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SUN_global_alpha)
static const struct gl_function_remap GL_SUN_global_alpha_functions[] = {
   {  3035, -1 }, /* GlobalAlphaFactorubSUN */
   {  4224, -1 }, /* GlobalAlphaFactoriSUN */
   {  5676, -1 }, /* GlobalAlphaFactordSUN */
   {  8715, -1 }, /* GlobalAlphaFactoruiSUN */
   {  9067, -1 }, /* GlobalAlphaFactorbSUN */
   { 11982, -1 }, /* GlobalAlphaFactorfSUN */
   { 12101, -1 }, /* GlobalAlphaFactorusSUN */
   { 20438, -1 }, /* GlobalAlphaFactorsSUN */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SUN_mesh_array)
static const struct gl_function_remap GL_SUN_mesh_array_functions[] = {
   { 26501, -1 }, /* DrawMeshArraysSUN */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SUN_triangle_list)
static const struct gl_function_remap GL_SUN_triangle_list_functions[] = {
   {  3971, -1 }, /* ReplacementCodeubSUN */
   {  5515, -1 }, /* ReplacementCodeubvSUN */
   { 17098, -1 }, /* ReplacementCodeusvSUN */
   { 17286, -1 }, /* ReplacementCodePointerSUN */
   { 19457, -1 }, /* ReplacementCodeuiSUN */
   { 20150, -1 }, /* ReplacementCodeusSUN */
   { 27146, -1 }, /* ReplacementCodeuivSUN */
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
   {  4480, -1 }, /* TexCoord2fColor4fNormal3fVertex3fSUN */
   {  4810, -1 }, /* TexCoord2fNormal3fVertex3fvSUN */
   {  5410, -1 }, /* ReplacementCodeuiTexCoord2fNormal3fVertex3fSUN */
   {  6480, -1 }, /* ReplacementCodeuiTexCoord2fVertex3fSUN */
   {  7192, -1 }, /* TexCoord2fNormal3fVertex3fSUN */
   {  7960, -1 }, /* Color3fVertex3fSUN */
   {  9026, -1 }, /* Color3fVertex3fvSUN */
   {  9441, -1 }, /* Color4fNormal3fVertex3fvSUN */
   { 10178, -1 }, /* ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN */
   { 11530, -1 }, /* ReplacementCodeuiColor4fNormal3fVertex3fvSUN */
   { 12973, -1 }, /* ReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN */
   { 13384, -1 }, /* TexCoord2fColor3fVertex3fSUN */
   { 14436, -1 }, /* TexCoord4fColor4fNormal3fVertex4fSUN */
   { 14770, -1 }, /* Color4ubVertex2fvSUN */
   { 15010, -1 }, /* Normal3fVertex3fSUN */
   { 15958, -1 }, /* ReplacementCodeuiColor4fNormal3fVertex3fSUN */
   { 16219, -1 }, /* TexCoord2fColor4fNormal3fVertex3fvSUN */
   { 16927, -1 }, /* TexCoord2fVertex3fvSUN */
   { 17703, -1 }, /* Color4ubVertex2fSUN */
   { 17924, -1 }, /* ReplacementCodeuiColor4ubVertex3fSUN */
   { 19845, -1 }, /* TexCoord2fColor4ubVertex3fSUN */
   { 20218, -1 }, /* Normal3fVertex3fvSUN */
   { 20647, -1 }, /* Color4fNormal3fVertex3fSUN */
   { 21556, -1 }, /* ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN */
   { 21776, -1 }, /* ReplacementCodeuiColor4ubVertex3fvSUN */
   { 23506, -1 }, /* ReplacementCodeuiColor3fVertex3fSUN */
   { 24763, -1 }, /* TexCoord4fVertex4fSUN */
   { 25189, -1 }, /* TexCoord2fColor3fVertex3fvSUN */
   { 25544, -1 }, /* ReplacementCodeuiNormal3fVertex3fvSUN */
   { 25671, -1 }, /* TexCoord4fVertex4fvSUN */
   { 26373, -1 }, /* ReplacementCodeuiVertex3fSUN */
   {    -1, -1 }
};
#endif

#if defined(need_GL_VERSION_1_3)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_VERSION_1_3_functions[] = {
   {   381, _gloffset_MultiTexCoord3sARB },
   {   613, _gloffset_ActiveTextureARB },
   {  3761, _gloffset_MultiTexCoord1fvARB },
   {  5301, _gloffset_MultiTexCoord3dARB },
   {  5346, _gloffset_MultiTexCoord2iARB },
   {  5470, _gloffset_MultiTexCoord2svARB },
   {  7401, _gloffset_MultiTexCoord2fARB },
   {  9277, _gloffset_MultiTexCoord3fvARB },
   {  9834, _gloffset_MultiTexCoord4sARB },
   { 10468, _gloffset_MultiTexCoord2dvARB },
   { 10850, _gloffset_MultiTexCoord1svARB },
   { 11225, _gloffset_MultiTexCoord3svARB },
   { 11286, _gloffset_MultiTexCoord4iARB },
   { 12009, _gloffset_MultiTexCoord3iARB },
   { 12711, _gloffset_MultiTexCoord1dARB },
   { 12928, _gloffset_MultiTexCoord3dvARB },
   { 14092, _gloffset_MultiTexCoord3ivARB },
   { 14137, _gloffset_MultiTexCoord2sARB },
   { 15357, _gloffset_MultiTexCoord4ivARB },
   { 17027, _gloffset_ClientActiveTextureARB },
   { 19310, _gloffset_MultiTexCoord2dARB },
   { 19714, _gloffset_MultiTexCoord4dvARB },
   { 20000, _gloffset_MultiTexCoord4fvARB },
   { 20879, _gloffset_MultiTexCoord3fARB },
   { 23259, _gloffset_MultiTexCoord4dARB },
   { 23463, _gloffset_MultiTexCoord1sARB },
   { 23667, _gloffset_MultiTexCoord1dvARB },
   { 24509, _gloffset_MultiTexCoord1ivARB },
   { 24602, _gloffset_MultiTexCoord2ivARB },
   { 24941, _gloffset_MultiTexCoord1iARB },
   { 26241, _gloffset_MultiTexCoord4svARB },
   { 26759, _gloffset_MultiTexCoord1fARB },
   { 27022, _gloffset_MultiTexCoord4fARB },
   { 29287, _gloffset_MultiTexCoord2fvARB },
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

