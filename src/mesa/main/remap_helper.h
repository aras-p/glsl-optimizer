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
   /* _mesa_function_pool[216]: GetCombinerOutputParameterfvNV (will be remapped) */
   "iiip\0"
   "glGetCombinerOutputParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[255]: SampleCoverageARB (will be remapped) */
   "fi\0"
   "glSampleCoverage\0"
   "glSampleCoverageARB\0"
   "\0"
   /* _mesa_function_pool[296]: ConvolutionFilter1D (offset 348) */
   "iiiiip\0"
   "glConvolutionFilter1D\0"
   "glConvolutionFilter1DEXT\0"
   "\0"
   /* _mesa_function_pool[351]: BeginQueryARB (will be remapped) */
   "ii\0"
   "glBeginQuery\0"
   "glBeginQueryARB\0"
   "\0"
   /* _mesa_function_pool[384]: RasterPos3dv (offset 71) */
   "p\0"
   "glRasterPos3dv\0"
   "\0"
   /* _mesa_function_pool[402]: PointParameteriNV (will be remapped) */
   "ii\0"
   "glPointParameteri\0"
   "glPointParameteriNV\0"
   "\0"
   /* _mesa_function_pool[444]: GetProgramiv (will be remapped) */
   "iip\0"
   "glGetProgramiv\0"
   "\0"
   /* _mesa_function_pool[464]: MultiTexCoord3sARB (offset 398) */
   "iiii\0"
   "glMultiTexCoord3s\0"
   "glMultiTexCoord3sARB\0"
   "\0"
   /* _mesa_function_pool[509]: SecondaryColor3iEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3i\0"
   "glSecondaryColor3iEXT\0"
   "\0"
   /* _mesa_function_pool[555]: WindowPos3fMESA (will be remapped) */
   "fff\0"
   "glWindowPos3f\0"
   "glWindowPos3fARB\0"
   "glWindowPos3fMESA\0"
   "\0"
   /* _mesa_function_pool[609]: TexCoord1iv (offset 99) */
   "p\0"
   "glTexCoord1iv\0"
   "\0"
   /* _mesa_function_pool[626]: TexCoord4sv (offset 125) */
   "p\0"
   "glTexCoord4sv\0"
   "\0"
   /* _mesa_function_pool[643]: RasterPos4s (offset 84) */
   "iiii\0"
   "glRasterPos4s\0"
   "\0"
   /* _mesa_function_pool[663]: PixelTexGenParameterfvSGIS (will be remapped) */
   "ip\0"
   "glPixelTexGenParameterfvSGIS\0"
   "\0"
   /* _mesa_function_pool[696]: ActiveTextureARB (offset 374) */
   "i\0"
   "glActiveTexture\0"
   "glActiveTextureARB\0"
   "\0"
   /* _mesa_function_pool[734]: BlitFramebufferEXT (will be remapped) */
   "iiiiiiiiii\0"
   "glBlitFramebuffer\0"
   "glBlitFramebufferEXT\0"
   "\0"
   /* _mesa_function_pool[785]: TexCoord1f (offset 96) */
   "f\0"
   "glTexCoord1f\0"
   "\0"
   /* _mesa_function_pool[801]: TexCoord1d (offset 94) */
   "d\0"
   "glTexCoord1d\0"
   "\0"
   /* _mesa_function_pool[817]: VertexAttrib4ubvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib4ubvNV\0"
   "\0"
   /* _mesa_function_pool[842]: TexCoord1i (offset 98) */
   "i\0"
   "glTexCoord1i\0"
   "\0"
   /* _mesa_function_pool[858]: GetProgramNamedParameterdvNV (will be remapped) */
   "iipp\0"
   "glGetProgramNamedParameterdvNV\0"
   "\0"
   /* _mesa_function_pool[895]: Histogram (offset 367) */
   "iiii\0"
   "glHistogram\0"
   "glHistogramEXT\0"
   "\0"
   /* _mesa_function_pool[928]: TexCoord1s (offset 100) */
   "i\0"
   "glTexCoord1s\0"
   "\0"
   /* _mesa_function_pool[944]: GetMapfv (offset 267) */
   "iip\0"
   "glGetMapfv\0"
   "\0"
   /* _mesa_function_pool[960]: EvalCoord1f (offset 230) */
   "f\0"
   "glEvalCoord1f\0"
   "\0"
   /* _mesa_function_pool[977]: FramebufferTexture (will be remapped) */
   "iiii\0"
   "glFramebufferTexture\0"
   "\0"
   /* _mesa_function_pool[1004]: GetGraphicsResetStatusARB (will be remapped) */
   "\0"
   "glGetGraphicsResetStatusARB\0"
   "\0"
   /* _mesa_function_pool[1034]: TexImage4DSGIS (dynamic) */
   "iiiiiiiiiip\0"
   "glTexImage4DSGIS\0"
   "\0"
   /* _mesa_function_pool[1064]: PolygonStipple (offset 175) */
   "p\0"
   "glPolygonStipple\0"
   "\0"
   /* _mesa_function_pool[1084]: WindowPos2dvMESA (will be remapped) */
   "p\0"
   "glWindowPos2dv\0"
   "glWindowPos2dvARB\0"
   "glWindowPos2dvMESA\0"
   "\0"
   /* _mesa_function_pool[1139]: ReplacementCodeuiColor3fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glReplacementCodeuiColor3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[1183]: BlendEquationSeparateEXT (will be remapped) */
   "ii\0"
   "glBlendEquationSeparate\0"
   "glBlendEquationSeparateEXT\0"
   "glBlendEquationSeparateATI\0"
   "\0"
   /* _mesa_function_pool[1265]: ListParameterfSGIX (dynamic) */
   "iif\0"
   "glListParameterfSGIX\0"
   "\0"
   /* _mesa_function_pool[1291]: SecondaryColor3bEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3b\0"
   "glSecondaryColor3bEXT\0"
   "\0"
   /* _mesa_function_pool[1337]: TexCoord4fColor4fNormal3fVertex4fvSUN (dynamic) */
   "pppp\0"
   "glTexCoord4fColor4fNormal3fVertex4fvSUN\0"
   "\0"
   /* _mesa_function_pool[1383]: GetnPolygonStippleARB (will be remapped) */
   "ip\0"
   "glGetnPolygonStippleARB\0"
   "\0"
   /* _mesa_function_pool[1411]: GetPixelMapfv (offset 271) */
   "ip\0"
   "glGetPixelMapfv\0"
   "\0"
   /* _mesa_function_pool[1431]: Color3uiv (offset 22) */
   "p\0"
   "glColor3uiv\0"
   "\0"
   /* _mesa_function_pool[1446]: IsEnabled (offset 286) */
   "i\0"
   "glIsEnabled\0"
   "\0"
   /* _mesa_function_pool[1461]: VertexAttrib4svNV (will be remapped) */
   "ip\0"
   "glVertexAttrib4svNV\0"
   "\0"
   /* _mesa_function_pool[1485]: EvalCoord2fv (offset 235) */
   "p\0"
   "glEvalCoord2fv\0"
   "\0"
   /* _mesa_function_pool[1503]: GetBufferSubDataARB (will be remapped) */
   "iiip\0"
   "glGetBufferSubData\0"
   "glGetBufferSubDataARB\0"
   "\0"
   /* _mesa_function_pool[1550]: BufferSubDataARB (will be remapped) */
   "iiip\0"
   "glBufferSubData\0"
   "glBufferSubDataARB\0"
   "\0"
   /* _mesa_function_pool[1591]: TexCoord2fColor4ubVertex3fvSUN (dynamic) */
   "ppp\0"
   "glTexCoord2fColor4ubVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[1629]: AttachShader (will be remapped) */
   "ii\0"
   "glAttachShader\0"
   "\0"
   /* _mesa_function_pool[1648]: GetCombinerInputParameterfvNV (will be remapped) */
   "iiiip\0"
   "glGetCombinerInputParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[1687]: MultiTexCoordP2ui (will be remapped) */
   "iii\0"
   "glMultiTexCoordP2ui\0"
   "\0"
   /* _mesa_function_pool[1712]: VertexAttrib2fARB (will be remapped) */
   "iff\0"
   "glVertexAttrib2f\0"
   "glVertexAttrib2fARB\0"
   "\0"
   /* _mesa_function_pool[1754]: GetDebugLogLengthMESA (dynamic) */
   "iii\0"
   "glGetDebugLogLengthMESA\0"
   "\0"
   /* _mesa_function_pool[1783]: GetMapiv (offset 268) */
   "iip\0"
   "glGetMapiv\0"
   "\0"
   /* _mesa_function_pool[1799]: VertexAttrib3fARB (will be remapped) */
   "ifff\0"
   "glVertexAttrib3f\0"
   "glVertexAttrib3fARB\0"
   "\0"
   /* _mesa_function_pool[1842]: Indexubv (offset 316) */
   "p\0"
   "glIndexubv\0"
   "\0"
   /* _mesa_function_pool[1856]: GetQueryivARB (will be remapped) */
   "iip\0"
   "glGetQueryiv\0"
   "glGetQueryivARB\0"
   "\0"
   /* _mesa_function_pool[1890]: TexImage3D (offset 371) */
   "iiiiiiiiip\0"
   "glTexImage3D\0"
   "glTexImage3DEXT\0"
   "\0"
   /* _mesa_function_pool[1931]: BindFragDataLocationEXT (will be remapped) */
   "iip\0"
   "glBindFragDataLocationEXT\0"
   "glBindFragDataLocation\0"
   "\0"
   /* _mesa_function_pool[1985]: TexCoordP4ui (will be remapped) */
   "ii\0"
   "glTexCoordP4ui\0"
   "\0"
   /* _mesa_function_pool[2004]: ReplacementCodeuiVertex3fvSUN (dynamic) */
   "pp\0"
   "glReplacementCodeuiVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[2040]: EdgeFlagPointer (offset 312) */
   "ip\0"
   "glEdgeFlagPointer\0"
   "\0"
   /* _mesa_function_pool[2062]: Color3ubv (offset 20) */
   "p\0"
   "glColor3ubv\0"
   "\0"
   /* _mesa_function_pool[2077]: GetQueryObjectivARB (will be remapped) */
   "iip\0"
   "glGetQueryObjectiv\0"
   "glGetQueryObjectivARB\0"
   "\0"
   /* _mesa_function_pool[2123]: Vertex3dv (offset 135) */
   "p\0"
   "glVertex3dv\0"
   "\0"
   /* _mesa_function_pool[2138]: ReplacementCodeuiTexCoord2fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glReplacementCodeuiTexCoord2fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[2185]: CompressedTexSubImage2DARB (will be remapped) */
   "iiiiiiiip\0"
   "glCompressedTexSubImage2D\0"
   "glCompressedTexSubImage2DARB\0"
   "\0"
   /* _mesa_function_pool[2251]: CombinerOutputNV (will be remapped) */
   "iiiiiiiiii\0"
   "glCombinerOutputNV\0"
   "\0"
   /* _mesa_function_pool[2282]: VertexAttribs3fvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs3fvNV\0"
   "\0"
   /* _mesa_function_pool[2308]: GetnMapivARB (will be remapped) */
   "iiip\0"
   "glGetnMapivARB\0"
   "\0"
   /* _mesa_function_pool[2329]: Uniform2fARB (will be remapped) */
   "iff\0"
   "glUniform2f\0"
   "glUniform2fARB\0"
   "\0"
   /* _mesa_function_pool[2361]: LightModeliv (offset 166) */
   "ip\0"
   "glLightModeliv\0"
   "\0"
   /* _mesa_function_pool[2380]: VertexAttrib1svARB (will be remapped) */
   "ip\0"
   "glVertexAttrib1sv\0"
   "glVertexAttrib1svARB\0"
   "\0"
   /* _mesa_function_pool[2423]: VertexAttribs1dvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs1dvNV\0"
   "\0"
   /* _mesa_function_pool[2449]: VertexP3ui (will be remapped) */
   "ii\0"
   "glVertexP3ui\0"
   "\0"
   /* _mesa_function_pool[2466]: Uniform2ivARB (will be remapped) */
   "iip\0"
   "glUniform2iv\0"
   "glUniform2ivARB\0"
   "\0"
   /* _mesa_function_pool[2500]: GetImageTransformParameterfvHP (dynamic) */
   "iip\0"
   "glGetImageTransformParameterfvHP\0"
   "\0"
   /* _mesa_function_pool[2538]: Normal3bv (offset 53) */
   "p\0"
   "glNormal3bv\0"
   "\0"
   /* _mesa_function_pool[2553]: TexGeniv (offset 193) */
   "iip\0"
   "glTexGeniv\0"
   "\0"
   /* _mesa_function_pool[2569]: TexCoordP1uiv (will be remapped) */
   "ip\0"
   "glTexCoordP1uiv\0"
   "\0"
   /* _mesa_function_pool[2589]: WeightubvARB (dynamic) */
   "ip\0"
   "glWeightubvARB\0"
   "\0"
   /* _mesa_function_pool[2608]: VertexAttrib1fvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib1fvNV\0"
   "\0"
   /* _mesa_function_pool[2632]: Vertex3iv (offset 139) */
   "p\0"
   "glVertex3iv\0"
   "\0"
   /* _mesa_function_pool[2647]: CopyConvolutionFilter1D (offset 354) */
   "iiiii\0"
   "glCopyConvolutionFilter1D\0"
   "glCopyConvolutionFilter1DEXT\0"
   "\0"
   /* _mesa_function_pool[2709]: VertexAttribI1uiEXT (will be remapped) */
   "ii\0"
   "glVertexAttribI1uiEXT\0"
   "glVertexAttribI1ui\0"
   "\0"
   /* _mesa_function_pool[2754]: ReplacementCodeuiNormal3fVertex3fSUN (dynamic) */
   "iffffff\0"
   "glReplacementCodeuiNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[2802]: TexCoordP1ui (will be remapped) */
   "ii\0"
   "glTexCoordP1ui\0"
   "\0"
   /* _mesa_function_pool[2821]: DeleteSync (will be remapped) */
   "i\0"
   "glDeleteSync\0"
   "\0"
   /* _mesa_function_pool[2837]: FragmentMaterialfvSGIX (dynamic) */
   "iip\0"
   "glFragmentMaterialfvSGIX\0"
   "\0"
   /* _mesa_function_pool[2867]: BlendColor (offset 336) */
   "ffff\0"
   "glBlendColor\0"
   "glBlendColorEXT\0"
   "\0"
   /* _mesa_function_pool[2902]: UniformMatrix4fvARB (will be remapped) */
   "iiip\0"
   "glUniformMatrix4fv\0"
   "glUniformMatrix4fvARB\0"
   "\0"
   /* _mesa_function_pool[2949]: DeleteVertexArraysAPPLE (will be remapped) */
   "ip\0"
   "glDeleteVertexArrays\0"
   "glDeleteVertexArraysAPPLE\0"
   "\0"
   /* _mesa_function_pool[3000]: TexBuffer (will be remapped) */
   "iii\0"
   "glTexBuffer\0"
   "\0"
   /* _mesa_function_pool[3017]: ReadInstrumentsSGIX (dynamic) */
   "i\0"
   "glReadInstrumentsSGIX\0"
   "\0"
   /* _mesa_function_pool[3042]: CallLists (offset 3) */
   "iip\0"
   "glCallLists\0"
   "\0"
   /* _mesa_function_pool[3059]: DeformationMap3dSGIX (dynamic) */
   "iddiiddiiddiip\0"
   "glDeformationMap3dSGIX\0"
   "\0"
   /* _mesa_function_pool[3098]: UniformMatrix2x4fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix2x4fv\0"
   "\0"
   /* _mesa_function_pool[3125]: ReadnPixelsARB (will be remapped) */
   "iiiiiiip\0"
   "glReadnPixelsARB\0"
   "\0"
   /* _mesa_function_pool[3152]: Color4ubVertex3fvSUN (dynamic) */
   "pp\0"
   "glColor4ubVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[3179]: Normal3iv (offset 59) */
   "p\0"
   "glNormal3iv\0"
   "\0"
   /* _mesa_function_pool[3194]: PassThrough (offset 199) */
   "f\0"
   "glPassThrough\0"
   "\0"
   /* _mesa_function_pool[3211]: GetnPixelMapusvARB (will be remapped) */
   "iip\0"
   "glGetnPixelMapusvARB\0"
   "\0"
   /* _mesa_function_pool[3237]: TexParameterIivEXT (will be remapped) */
   "iip\0"
   "glTexParameterIivEXT\0"
   "glTexParameterIiv\0"
   "\0"
   /* _mesa_function_pool[3281]: FramebufferTextureLayerEXT (will be remapped) */
   "iiiii\0"
   "glFramebufferTextureLayer\0"
   "glFramebufferTextureLayerARB\0"
   "glFramebufferTextureLayerEXT\0"
   "\0"
   /* _mesa_function_pool[3372]: GetListParameterfvSGIX (dynamic) */
   "iip\0"
   "glGetListParameterfvSGIX\0"
   "\0"
   /* _mesa_function_pool[3402]: Viewport (offset 305) */
   "iiii\0"
   "glViewport\0"
   "\0"
   /* _mesa_function_pool[3419]: VertexAttrib4NusvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Nusv\0"
   "glVertexAttrib4NusvARB\0"
   "\0"
   /* _mesa_function_pool[3466]: VertexP4uiv (will be remapped) */
   "ip\0"
   "glVertexP4uiv\0"
   "\0"
   /* _mesa_function_pool[3484]: VertexAttribP3ui (will be remapped) */
   "iiii\0"
   "glVertexAttribP3ui\0"
   "\0"
   /* _mesa_function_pool[3509]: WindowPos4svMESA (will be remapped) */
   "p\0"
   "glWindowPos4svMESA\0"
   "\0"
   /* _mesa_function_pool[3531]: CreateProgramObjectARB (will be remapped) */
   "\0"
   "glCreateProgramObjectARB\0"
   "\0"
   /* _mesa_function_pool[3558]: DeleteTransformFeedbacks (will be remapped) */
   "ip\0"
   "glDeleteTransformFeedbacks\0"
   "\0"
   /* _mesa_function_pool[3589]: UniformMatrix4x3fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix4x3fv\0"
   "\0"
   /* _mesa_function_pool[3616]: PrioritizeTextures (offset 331) */
   "ipp\0"
   "glPrioritizeTextures\0"
   "glPrioritizeTexturesEXT\0"
   "\0"
   /* _mesa_function_pool[3666]: VertexAttribI3uiEXT (will be remapped) */
   "iiii\0"
   "glVertexAttribI3uiEXT\0"
   "glVertexAttribI3ui\0"
   "\0"
   /* _mesa_function_pool[3713]: AsyncMarkerSGIX (dynamic) */
   "i\0"
   "glAsyncMarkerSGIX\0"
   "\0"
   /* _mesa_function_pool[3734]: GlobalAlphaFactorubSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactorubSUN\0"
   "\0"
   /* _mesa_function_pool[3762]: ClearColorIuiEXT (will be remapped) */
   "iiii\0"
   "glClearColorIuiEXT\0"
   "\0"
   /* _mesa_function_pool[3787]: ClearDebugLogMESA (dynamic) */
   "iii\0"
   "glClearDebugLogMESA\0"
   "\0"
   /* _mesa_function_pool[3812]: Uniform4uiEXT (will be remapped) */
   "iiiii\0"
   "glUniform4uiEXT\0"
   "glUniform4ui\0"
   "\0"
   /* _mesa_function_pool[3848]: ResetHistogram (offset 369) */
   "i\0"
   "glResetHistogram\0"
   "glResetHistogramEXT\0"
   "\0"
   /* _mesa_function_pool[3888]: GetProgramNamedParameterfvNV (will be remapped) */
   "iipp\0"
   "glGetProgramNamedParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[3925]: PointParameterfEXT (will be remapped) */
   "if\0"
   "glPointParameterf\0"
   "glPointParameterfARB\0"
   "glPointParameterfEXT\0"
   "glPointParameterfSGIS\0"
   "\0"
   /* _mesa_function_pool[4011]: MultiTexCoordP4uiv (will be remapped) */
   "iip\0"
   "glMultiTexCoordP4uiv\0"
   "\0"
   /* _mesa_function_pool[4037]: LoadIdentityDeformationMapSGIX (dynamic) */
   "i\0"
   "glLoadIdentityDeformationMapSGIX\0"
   "\0"
   /* _mesa_function_pool[4073]: GenFencesNV (will be remapped) */
   "ip\0"
   "glGenFencesNV\0"
   "\0"
   /* _mesa_function_pool[4091]: ImageTransformParameterfHP (dynamic) */
   "iif\0"
   "glImageTransformParameterfHP\0"
   "\0"
   /* _mesa_function_pool[4125]: MatrixIndexusvARB (dynamic) */
   "ip\0"
   "glMatrixIndexusvARB\0"
   "\0"
   /* _mesa_function_pool[4149]: DrawElementsBaseVertex (will be remapped) */
   "iiipi\0"
   "glDrawElementsBaseVertex\0"
   "\0"
   /* _mesa_function_pool[4181]: DisableVertexAttribArrayARB (will be remapped) */
   "i\0"
   "glDisableVertexAttribArray\0"
   "glDisableVertexAttribArrayARB\0"
   "\0"
   /* _mesa_function_pool[4241]: VertexAttribI4ubvEXT (will be remapped) */
   "ip\0"
   "glVertexAttribI4ubvEXT\0"
   "glVertexAttribI4ubv\0"
   "\0"
   /* _mesa_function_pool[4288]: GetnConvolutionFilterARB (will be remapped) */
   "iiiip\0"
   "glGetnConvolutionFilterARB\0"
   "\0"
   /* _mesa_function_pool[4322]: TexCoord2sv (offset 109) */
   "p\0"
   "glTexCoord2sv\0"
   "\0"
   /* _mesa_function_pool[4339]: ColorP4uiv (will be remapped) */
   "ip\0"
   "glColorP4uiv\0"
   "\0"
   /* _mesa_function_pool[4356]: Vertex4dv (offset 143) */
   "p\0"
   "glVertex4dv\0"
   "\0"
   /* _mesa_function_pool[4371]: StencilMaskSeparate (will be remapped) */
   "ii\0"
   "glStencilMaskSeparate\0"
   "\0"
   /* _mesa_function_pool[4397]: ProgramLocalParameter4dARB (will be remapped) */
   "iidddd\0"
   "glProgramLocalParameter4dARB\0"
   "\0"
   /* _mesa_function_pool[4434]: CompressedTexImage3DARB (will be remapped) */
   "iiiiiiiip\0"
   "glCompressedTexImage3D\0"
   "glCompressedTexImage3DARB\0"
   "\0"
   /* _mesa_function_pool[4494]: Color3sv (offset 18) */
   "p\0"
   "glColor3sv\0"
   "\0"
   /* _mesa_function_pool[4508]: GetConvolutionParameteriv (offset 358) */
   "iip\0"
   "glGetConvolutionParameteriv\0"
   "glGetConvolutionParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[4572]: DeleteSamplers (will be remapped) */
   "ip\0"
   "glDeleteSamplers\0"
   "\0"
   /* _mesa_function_pool[4593]: VertexAttrib1fARB (will be remapped) */
   "if\0"
   "glVertexAttrib1f\0"
   "glVertexAttrib1fARB\0"
   "\0"
   /* _mesa_function_pool[4634]: Vertex2dv (offset 127) */
   "p\0"
   "glVertex2dv\0"
   "\0"
   /* _mesa_function_pool[4649]: TestFenceNV (will be remapped) */
   "i\0"
   "glTestFenceNV\0"
   "\0"
   /* _mesa_function_pool[4666]: MultiTexCoord1fvARB (offset 379) */
   "ip\0"
   "glMultiTexCoord1fv\0"
   "glMultiTexCoord1fvARB\0"
   "\0"
   /* _mesa_function_pool[4711]: TexCoord3iv (offset 115) */
   "p\0"
   "glTexCoord3iv\0"
   "\0"
   /* _mesa_function_pool[4728]: Uniform2uivEXT (will be remapped) */
   "iip\0"
   "glUniform2uivEXT\0"
   "glUniform2uiv\0"
   "\0"
   /* _mesa_function_pool[4764]: ColorFragmentOp2ATI (will be remapped) */
   "iiiiiiiiii\0"
   "glColorFragmentOp2ATI\0"
   "\0"
   /* _mesa_function_pool[4798]: SecondaryColorPointerListIBM (dynamic) */
   "iiipi\0"
   "glSecondaryColorPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[4836]: GetPixelTexGenParameterivSGIS (will be remapped) */
   "ip\0"
   "glGetPixelTexGenParameterivSGIS\0"
   "\0"
   /* _mesa_function_pool[4872]: Color3fv (offset 14) */
   "p\0"
   "glColor3fv\0"
   "\0"
   /* _mesa_function_pool[4886]: GetnPixelMapfvARB (will be remapped) */
   "iip\0"
   "glGetnPixelMapfvARB\0"
   "\0"
   /* _mesa_function_pool[4911]: ReplacementCodeubSUN (dynamic) */
   "i\0"
   "glReplacementCodeubSUN\0"
   "\0"
   /* _mesa_function_pool[4937]: FinishAsyncSGIX (dynamic) */
   "p\0"
   "glFinishAsyncSGIX\0"
   "\0"
   /* _mesa_function_pool[4958]: GetnUniformfvARB (will be remapped) */
   "iiip\0"
   "glGetnUniformfvARB\0"
   "\0"
   /* _mesa_function_pool[4983]: GetDebugLogMESA (dynamic) */
   "iiiipp\0"
   "glGetDebugLogMESA\0"
   "\0"
   /* _mesa_function_pool[5009]: FogCoorddEXT (will be remapped) */
   "d\0"
   "glFogCoordd\0"
   "glFogCoorddEXT\0"
   "\0"
   /* _mesa_function_pool[5039]: BeginConditionalRenderNV (will be remapped) */
   "ii\0"
   "glBeginConditionalRenderNV\0"
   "glBeginConditionalRender\0"
   "\0"
   /* _mesa_function_pool[5095]: Color4ubVertex3fSUN (dynamic) */
   "iiiifff\0"
   "glColor4ubVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[5126]: FogCoordfEXT (will be remapped) */
   "f\0"
   "glFogCoordf\0"
   "glFogCoordfEXT\0"
   "\0"
   /* _mesa_function_pool[5156]: PointSize (offset 173) */
   "f\0"
   "glPointSize\0"
   "\0"
   /* _mesa_function_pool[5171]: VertexAttribI2uivEXT (will be remapped) */
   "ip\0"
   "glVertexAttribI2uivEXT\0"
   "glVertexAttribI2uiv\0"
   "\0"
   /* _mesa_function_pool[5218]: TexCoord2fVertex3fSUN (dynamic) */
   "fffff\0"
   "glTexCoord2fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[5249]: PopName (offset 200) */
   "\0"
   "glPopName\0"
   "\0"
   /* _mesa_function_pool[5261]: GetSamplerParameterfv (will be remapped) */
   "iip\0"
   "glGetSamplerParameterfv\0"
   "\0"
   /* _mesa_function_pool[5290]: GlobalAlphaFactoriSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactoriSUN\0"
   "\0"
   /* _mesa_function_pool[5317]: VertexAttrib2dNV (will be remapped) */
   "idd\0"
   "glVertexAttrib2dNV\0"
   "\0"
   /* _mesa_function_pool[5341]: GetProgramInfoLog (will be remapped) */
   "iipp\0"
   "glGetProgramInfoLog\0"
   "\0"
   /* _mesa_function_pool[5367]: VertexP2ui (will be remapped) */
   "ii\0"
   "glVertexP2ui\0"
   "\0"
   /* _mesa_function_pool[5384]: VertexAttrib4NbvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Nbv\0"
   "glVertexAttrib4NbvARB\0"
   "\0"
   /* _mesa_function_pool[5429]: GetActiveAttribARB (will be remapped) */
   "iiipppp\0"
   "glGetActiveAttrib\0"
   "glGetActiveAttribARB\0"
   "\0"
   /* _mesa_function_pool[5477]: Vertex4sv (offset 149) */
   "p\0"
   "glVertex4sv\0"
   "\0"
   /* _mesa_function_pool[5492]: VertexAttrib4ubNV (will be remapped) */
   "iiiii\0"
   "glVertexAttrib4ubNV\0"
   "\0"
   /* _mesa_function_pool[5519]: VertexAttribI1ivEXT (will be remapped) */
   "ip\0"
   "glVertexAttribI1ivEXT\0"
   "glVertexAttribI1iv\0"
   "\0"
   /* _mesa_function_pool[5564]: ClampColor (will be remapped) */
   "ii\0"
   "glClampColor\0"
   "\0"
   /* _mesa_function_pool[5581]: TextureRangeAPPLE (will be remapped) */
   "iip\0"
   "glTextureRangeAPPLE\0"
   "\0"
   /* _mesa_function_pool[5606]: GetTexEnvfv (offset 276) */
   "iip\0"
   "glGetTexEnvfv\0"
   "\0"
   /* _mesa_function_pool[5625]: VertexAttribP2uiv (will be remapped) */
   "iiip\0"
   "glVertexAttribP2uiv\0"
   "\0"
   /* _mesa_function_pool[5651]: BindTransformFeedback (will be remapped) */
   "ii\0"
   "glBindTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[5679]: TexCoord2fColor4fNormal3fVertex3fSUN (dynamic) */
   "ffffffffffff\0"
   "glTexCoord2fColor4fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[5732]: Indexub (offset 315) */
   "i\0"
   "glIndexub\0"
   "\0"
   /* _mesa_function_pool[5745]: VertexAttrib4fNV (will be remapped) */
   "iffff\0"
   "glVertexAttrib4fNV\0"
   "\0"
   /* _mesa_function_pool[5771]: TexEnvi (offset 186) */
   "iii\0"
   "glTexEnvi\0"
   "\0"
   /* _mesa_function_pool[5786]: GetClipPlane (offset 259) */
   "ip\0"
   "glGetClipPlane\0"
   "\0"
   /* _mesa_function_pool[5805]: CombinerParameterfvNV (will be remapped) */
   "ip\0"
   "glCombinerParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[5833]: TexCoordP4uiv (will be remapped) */
   "ip\0"
   "glTexCoordP4uiv\0"
   "\0"
   /* _mesa_function_pool[5853]: VertexAttribs3dvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs3dvNV\0"
   "\0"
   /* _mesa_function_pool[5879]: VertexAttribI2uiEXT (will be remapped) */
   "iii\0"
   "glVertexAttribI2uiEXT\0"
   "glVertexAttribI2ui\0"
   "\0"
   /* _mesa_function_pool[5925]: VertexAttribs4fvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs4fvNV\0"
   "\0"
   /* _mesa_function_pool[5951]: VertexArrayRangeNV (will be remapped) */
   "ip\0"
   "glVertexArrayRangeNV\0"
   "\0"
   /* _mesa_function_pool[5976]: FragmentLightiSGIX (dynamic) */
   "iii\0"
   "glFragmentLightiSGIX\0"
   "\0"
   /* _mesa_function_pool[6002]: PolygonOffsetEXT (will be remapped) */
   "ff\0"
   "glPolygonOffsetEXT\0"
   "\0"
   /* _mesa_function_pool[6025]: VertexAttribI4uivEXT (will be remapped) */
   "ip\0"
   "glVertexAttribI4uivEXT\0"
   "glVertexAttribI4uiv\0"
   "\0"
   /* _mesa_function_pool[6072]: PollAsyncSGIX (dynamic) */
   "p\0"
   "glPollAsyncSGIX\0"
   "\0"
   /* _mesa_function_pool[6091]: DeleteFragmentShaderATI (will be remapped) */
   "i\0"
   "glDeleteFragmentShaderATI\0"
   "\0"
   /* _mesa_function_pool[6120]: Scaled (offset 301) */
   "ddd\0"
   "glScaled\0"
   "\0"
   /* _mesa_function_pool[6134]: ResumeTransformFeedback (will be remapped) */
   "\0"
   "glResumeTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[6162]: Scalef (offset 302) */
   "fff\0"
   "glScalef\0"
   "\0"
   /* _mesa_function_pool[6176]: TexCoord2fNormal3fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glTexCoord2fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[6214]: ProgramEnvParameters4fvEXT (will be remapped) */
   "iiip\0"
   "glProgramEnvParameters4fvEXT\0"
   "\0"
   /* _mesa_function_pool[6249]: MultTransposeMatrixdARB (will be remapped) */
   "p\0"
   "glMultTransposeMatrixd\0"
   "glMultTransposeMatrixdARB\0"
   "\0"
   /* _mesa_function_pool[6301]: ObjectUnpurgeableAPPLE (will be remapped) */
   "iii\0"
   "glObjectUnpurgeableAPPLE\0"
   "\0"
   /* _mesa_function_pool[6331]: TexCoordP3ui (will be remapped) */
   "ii\0"
   "glTexCoordP3ui\0"
   "\0"
   /* _mesa_function_pool[6350]: AlphaFunc (offset 240) */
   "if\0"
   "glAlphaFunc\0"
   "\0"
   /* _mesa_function_pool[6366]: WindowPos2svMESA (will be remapped) */
   "p\0"
   "glWindowPos2sv\0"
   "glWindowPos2svARB\0"
   "glWindowPos2svMESA\0"
   "\0"
   /* _mesa_function_pool[6421]: EdgeFlag (offset 41) */
   "i\0"
   "glEdgeFlag\0"
   "\0"
   /* _mesa_function_pool[6435]: TexCoord2iv (offset 107) */
   "p\0"
   "glTexCoord2iv\0"
   "\0"
   /* _mesa_function_pool[6452]: CompressedTexImage1DARB (will be remapped) */
   "iiiiiip\0"
   "glCompressedTexImage1D\0"
   "glCompressedTexImage1DARB\0"
   "\0"
   /* _mesa_function_pool[6510]: Rotated (offset 299) */
   "dddd\0"
   "glRotated\0"
   "\0"
   /* _mesa_function_pool[6526]: GetTexParameterIuivEXT (will be remapped) */
   "iip\0"
   "glGetTexParameterIuivEXT\0"
   "glGetTexParameterIuiv\0"
   "\0"
   /* _mesa_function_pool[6578]: VertexAttrib2sNV (will be remapped) */
   "iii\0"
   "glVertexAttrib2sNV\0"
   "\0"
   /* _mesa_function_pool[6602]: ReadPixels (offset 256) */
   "iiiiiip\0"
   "glReadPixels\0"
   "\0"
   /* _mesa_function_pool[6624]: VertexAttribDivisorARB (will be remapped) */
   "ii\0"
   "glVertexAttribDivisorARB\0"
   "\0"
   /* _mesa_function_pool[6653]: EdgeFlagv (offset 42) */
   "p\0"
   "glEdgeFlagv\0"
   "\0"
   /* _mesa_function_pool[6668]: NormalPointerListIBM (dynamic) */
   "iipi\0"
   "glNormalPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[6697]: IndexPointerEXT (will be remapped) */
   "iiip\0"
   "glIndexPointerEXT\0"
   "\0"
   /* _mesa_function_pool[6721]: Color4iv (offset 32) */
   "p\0"
   "glColor4iv\0"
   "\0"
   /* _mesa_function_pool[6735]: TexParameterf (offset 178) */
   "iif\0"
   "glTexParameterf\0"
   "\0"
   /* _mesa_function_pool[6756]: TexParameteri (offset 180) */
   "iii\0"
   "glTexParameteri\0"
   "\0"
   /* _mesa_function_pool[6777]: NormalPointerEXT (will be remapped) */
   "iiip\0"
   "glNormalPointerEXT\0"
   "\0"
   /* _mesa_function_pool[6802]: MultiTexCoord3dARB (offset 392) */
   "iddd\0"
   "glMultiTexCoord3d\0"
   "glMultiTexCoord3dARB\0"
   "\0"
   /* _mesa_function_pool[6847]: MultiTexCoord2iARB (offset 388) */
   "iii\0"
   "glMultiTexCoord2i\0"
   "glMultiTexCoord2iARB\0"
   "\0"
   /* _mesa_function_pool[6891]: DrawPixels (offset 257) */
   "iiiip\0"
   "glDrawPixels\0"
   "\0"
   /* _mesa_function_pool[6911]: ReplacementCodeuiTexCoord2fNormal3fVertex3fSUN (dynamic) */
   "iffffffff\0"
   "glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[6971]: MultiTexCoord2svARB (offset 391) */
   "ip\0"
   "glMultiTexCoord2sv\0"
   "glMultiTexCoord2svARB\0"
   "\0"
   /* _mesa_function_pool[7016]: ReplacementCodeubvSUN (dynamic) */
   "p\0"
   "glReplacementCodeubvSUN\0"
   "\0"
   /* _mesa_function_pool[7043]: Uniform3iARB (will be remapped) */
   "iiii\0"
   "glUniform3i\0"
   "glUniform3iARB\0"
   "\0"
   /* _mesa_function_pool[7076]: DrawTransformFeedback (will be remapped) */
   "ii\0"
   "glDrawTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[7104]: DrawElementsInstancedARB (will be remapped) */
   "iiipi\0"
   "glDrawElementsInstancedARB\0"
   "glDrawElementsInstancedEXT\0"
   "glDrawElementsInstanced\0"
   "\0"
   /* _mesa_function_pool[7189]: GetShaderInfoLog (will be remapped) */
   "iipp\0"
   "glGetShaderInfoLog\0"
   "\0"
   /* _mesa_function_pool[7214]: WeightivARB (dynamic) */
   "ip\0"
   "glWeightivARB\0"
   "\0"
   /* _mesa_function_pool[7232]: PollInstrumentsSGIX (dynamic) */
   "p\0"
   "glPollInstrumentsSGIX\0"
   "\0"
   /* _mesa_function_pool[7257]: GlobalAlphaFactordSUN (dynamic) */
   "d\0"
   "glGlobalAlphaFactordSUN\0"
   "\0"
   /* _mesa_function_pool[7284]: GetFinalCombinerInputParameterfvNV (will be remapped) */
   "iip\0"
   "glGetFinalCombinerInputParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[7326]: GenerateMipmapEXT (will be remapped) */
   "i\0"
   "glGenerateMipmap\0"
   "glGenerateMipmapEXT\0"
   "\0"
   /* _mesa_function_pool[7366]: GenLists (offset 5) */
   "i\0"
   "glGenLists\0"
   "\0"
   /* _mesa_function_pool[7380]: DepthRangef (will be remapped) */
   "ff\0"
   "glDepthRangef\0"
   "\0"
   /* _mesa_function_pool[7398]: GetMapAttribParameterivNV (dynamic) */
   "iiip\0"
   "glGetMapAttribParameterivNV\0"
   "\0"
   /* _mesa_function_pool[7432]: CreateShaderObjectARB (will be remapped) */
   "i\0"
   "glCreateShaderObjectARB\0"
   "\0"
   /* _mesa_function_pool[7459]: GetSharpenTexFuncSGIS (dynamic) */
   "ip\0"
   "glGetSharpenTexFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[7487]: BufferDataARB (will be remapped) */
   "iipi\0"
   "glBufferData\0"
   "glBufferDataARB\0"
   "\0"
   /* _mesa_function_pool[7522]: FlushVertexArrayRangeNV (will be remapped) */
   "\0"
   "glFlushVertexArrayRangeNV\0"
   "\0"
   /* _mesa_function_pool[7550]: MapGrid2d (offset 226) */
   "iddidd\0"
   "glMapGrid2d\0"
   "\0"
   /* _mesa_function_pool[7570]: MapGrid2f (offset 227) */
   "iffiff\0"
   "glMapGrid2f\0"
   "\0"
   /* _mesa_function_pool[7590]: SampleMapATI (will be remapped) */
   "iii\0"
   "glSampleMapATI\0"
   "\0"
   /* _mesa_function_pool[7610]: VertexPointerEXT (will be remapped) */
   "iiiip\0"
   "glVertexPointerEXT\0"
   "\0"
   /* _mesa_function_pool[7636]: GetTexFilterFuncSGIS (dynamic) */
   "iip\0"
   "glGetTexFilterFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[7664]: Scissor (offset 176) */
   "iiii\0"
   "glScissor\0"
   "\0"
   /* _mesa_function_pool[7680]: Fogf (offset 153) */
   "if\0"
   "glFogf\0"
   "\0"
   /* _mesa_function_pool[7691]: ReplacementCodeuiColor4ubVertex3fvSUN (dynamic) */
   "ppp\0"
   "glReplacementCodeuiColor4ubVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[7736]: TexSubImage1D (offset 332) */
   "iiiiiip\0"
   "glTexSubImage1D\0"
   "glTexSubImage1DEXT\0"
   "\0"
   /* _mesa_function_pool[7780]: VertexAttrib1sARB (will be remapped) */
   "ii\0"
   "glVertexAttrib1s\0"
   "glVertexAttrib1sARB\0"
   "\0"
   /* _mesa_function_pool[7821]: FenceSync (will be remapped) */
   "ii\0"
   "glFenceSync\0"
   "\0"
   /* _mesa_function_pool[7837]: Color4usv (offset 40) */
   "p\0"
   "glColor4usv\0"
   "\0"
   /* _mesa_function_pool[7852]: Fogi (offset 155) */
   "ii\0"
   "glFogi\0"
   "\0"
   /* _mesa_function_pool[7863]: DepthRange (offset 288) */
   "dd\0"
   "glDepthRange\0"
   "\0"
   /* _mesa_function_pool[7880]: RasterPos3iv (offset 75) */
   "p\0"
   "glRasterPos3iv\0"
   "\0"
   /* _mesa_function_pool[7898]: FinalCombinerInputNV (will be remapped) */
   "iiii\0"
   "glFinalCombinerInputNV\0"
   "\0"
   /* _mesa_function_pool[7927]: TexCoord2i (offset 106) */
   "ii\0"
   "glTexCoord2i\0"
   "\0"
   /* _mesa_function_pool[7944]: PixelMapfv (offset 251) */
   "iip\0"
   "glPixelMapfv\0"
   "\0"
   /* _mesa_function_pool[7962]: Color4ui (offset 37) */
   "iiii\0"
   "glColor4ui\0"
   "\0"
   /* _mesa_function_pool[7979]: RasterPos3s (offset 76) */
   "iii\0"
   "glRasterPos3s\0"
   "\0"
   /* _mesa_function_pool[7998]: Color3usv (offset 24) */
   "p\0"
   "glColor3usv\0"
   "\0"
   /* _mesa_function_pool[8013]: FlushRasterSGIX (dynamic) */
   "\0"
   "glFlushRasterSGIX\0"
   "\0"
   /* _mesa_function_pool[8033]: TexCoord2f (offset 104) */
   "ff\0"
   "glTexCoord2f\0"
   "\0"
   /* _mesa_function_pool[8050]: ReplacementCodeuiTexCoord2fVertex3fSUN (dynamic) */
   "ifffff\0"
   "glReplacementCodeuiTexCoord2fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[8099]: TexCoord2d (offset 102) */
   "dd\0"
   "glTexCoord2d\0"
   "\0"
   /* _mesa_function_pool[8116]: RasterPos3d (offset 70) */
   "ddd\0"
   "glRasterPos3d\0"
   "\0"
   /* _mesa_function_pool[8135]: RasterPos3f (offset 72) */
   "fff\0"
   "glRasterPos3f\0"
   "\0"
   /* _mesa_function_pool[8154]: Uniform1fARB (will be remapped) */
   "if\0"
   "glUniform1f\0"
   "glUniform1fARB\0"
   "\0"
   /* _mesa_function_pool[8185]: AreTexturesResident (offset 322) */
   "ipp\0"
   "glAreTexturesResident\0"
   "glAreTexturesResidentEXT\0"
   "\0"
   /* _mesa_function_pool[8237]: TexCoord2s (offset 108) */
   "ii\0"
   "glTexCoord2s\0"
   "\0"
   /* _mesa_function_pool[8254]: StencilOpSeparate (will be remapped) */
   "iiii\0"
   "glStencilOpSeparate\0"
   "glStencilOpSeparateATI\0"
   "\0"
   /* _mesa_function_pool[8303]: ColorTableParameteriv (offset 341) */
   "iip\0"
   "glColorTableParameteriv\0"
   "glColorTableParameterivSGI\0"
   "\0"
   /* _mesa_function_pool[8359]: FogCoordPointerListIBM (dynamic) */
   "iipi\0"
   "glFogCoordPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[8390]: WindowPos3dMESA (will be remapped) */
   "ddd\0"
   "glWindowPos3d\0"
   "glWindowPos3dARB\0"
   "glWindowPos3dMESA\0"
   "\0"
   /* _mesa_function_pool[8444]: Color4us (offset 39) */
   "iiii\0"
   "glColor4us\0"
   "\0"
   /* _mesa_function_pool[8461]: PointParameterfvEXT (will be remapped) */
   "ip\0"
   "glPointParameterfv\0"
   "glPointParameterfvARB\0"
   "glPointParameterfvEXT\0"
   "glPointParameterfvSGIS\0"
   "\0"
   /* _mesa_function_pool[8551]: Color3bv (offset 10) */
   "p\0"
   "glColor3bv\0"
   "\0"
   /* _mesa_function_pool[8565]: GetnCompressedTexImageARB (will be remapped) */
   "iiip\0"
   "glGetnCompressedTexImageARB\0"
   "\0"
   /* _mesa_function_pool[8599]: WindowPos2fvMESA (will be remapped) */
   "p\0"
   "glWindowPos2fv\0"
   "glWindowPos2fvARB\0"
   "glWindowPos2fvMESA\0"
   "\0"
   /* _mesa_function_pool[8654]: SecondaryColor3bvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3bv\0"
   "glSecondaryColor3bvEXT\0"
   "\0"
   /* _mesa_function_pool[8700]: VertexPointerListIBM (dynamic) */
   "iiipi\0"
   "glVertexPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[8730]: GetProgramLocalParameterfvARB (will be remapped) */
   "iip\0"
   "glGetProgramLocalParameterfvARB\0"
   "\0"
   /* _mesa_function_pool[8767]: FragmentMaterialfSGIX (dynamic) */
   "iif\0"
   "glFragmentMaterialfSGIX\0"
   "\0"
   /* _mesa_function_pool[8796]: BindSampler (will be remapped) */
   "ii\0"
   "glBindSampler\0"
   "\0"
   /* _mesa_function_pool[8814]: RenderbufferStorageEXT (will be remapped) */
   "iiii\0"
   "glRenderbufferStorage\0"
   "glRenderbufferStorageEXT\0"
   "\0"
   /* _mesa_function_pool[8867]: IsFenceNV (will be remapped) */
   "i\0"
   "glIsFenceNV\0"
   "\0"
   /* _mesa_function_pool[8882]: AttachObjectARB (will be remapped) */
   "ii\0"
   "glAttachObjectARB\0"
   "\0"
   /* _mesa_function_pool[8904]: GetFragmentLightivSGIX (dynamic) */
   "iip\0"
   "glGetFragmentLightivSGIX\0"
   "\0"
   /* _mesa_function_pool[8934]: UniformMatrix2fvARB (will be remapped) */
   "iiip\0"
   "glUniformMatrix2fv\0"
   "glUniformMatrix2fvARB\0"
   "\0"
   /* _mesa_function_pool[8981]: MultiTexCoord2fARB (offset 386) */
   "iff\0"
   "glMultiTexCoord2f\0"
   "glMultiTexCoord2fARB\0"
   "\0"
   /* _mesa_function_pool[9025]: ColorTable (offset 339) */
   "iiiiip\0"
   "glColorTable\0"
   "glColorTableSGI\0"
   "glColorTableEXT\0"
   "\0"
   /* _mesa_function_pool[9078]: IndexPointer (offset 314) */
   "iip\0"
   "glIndexPointer\0"
   "\0"
   /* _mesa_function_pool[9098]: Accum (offset 213) */
   "if\0"
   "glAccum\0"
   "\0"
   /* _mesa_function_pool[9110]: GetTexImage (offset 281) */
   "iiiip\0"
   "glGetTexImage\0"
   "\0"
   /* _mesa_function_pool[9131]: MapControlPointsNV (dynamic) */
   "iiiiiiiip\0"
   "glMapControlPointsNV\0"
   "\0"
   /* _mesa_function_pool[9163]: ConvolutionFilter2D (offset 349) */
   "iiiiiip\0"
   "glConvolutionFilter2D\0"
   "glConvolutionFilter2DEXT\0"
   "\0"
   /* _mesa_function_pool[9219]: Finish (offset 216) */
   "\0"
   "glFinish\0"
   "\0"
   /* _mesa_function_pool[9230]: MapParameterfvNV (dynamic) */
   "iip\0"
   "glMapParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[9254]: ClearStencil (offset 207) */
   "i\0"
   "glClearStencil\0"
   "\0"
   /* _mesa_function_pool[9272]: VertexAttrib3dvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib3dv\0"
   "glVertexAttrib3dvARB\0"
   "\0"
   /* _mesa_function_pool[9315]: Uniform4uivEXT (will be remapped) */
   "iip\0"
   "glUniform4uivEXT\0"
   "glUniform4uiv\0"
   "\0"
   /* _mesa_function_pool[9351]: HintPGI (dynamic) */
   "ii\0"
   "glHintPGI\0"
   "\0"
   /* _mesa_function_pool[9365]: ConvolutionParameteriv (offset 353) */
   "iip\0"
   "glConvolutionParameteriv\0"
   "glConvolutionParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[9423]: Color4s (offset 33) */
   "iiii\0"
   "glColor4s\0"
   "\0"
   /* _mesa_function_pool[9439]: InterleavedArrays (offset 317) */
   "iip\0"
   "glInterleavedArrays\0"
   "\0"
   /* _mesa_function_pool[9464]: RasterPos2fv (offset 65) */
   "p\0"
   "glRasterPos2fv\0"
   "\0"
   /* _mesa_function_pool[9482]: TexCoord1fv (offset 97) */
   "p\0"
   "glTexCoord1fv\0"
   "\0"
   /* _mesa_function_pool[9499]: Vertex2d (offset 126) */
   "dd\0"
   "glVertex2d\0"
   "\0"
   /* _mesa_function_pool[9514]: CullParameterdvEXT (dynamic) */
   "ip\0"
   "glCullParameterdvEXT\0"
   "\0"
   /* _mesa_function_pool[9539]: ProgramNamedParameter4fNV (will be remapped) */
   "iipffff\0"
   "glProgramNamedParameter4fNV\0"
   "\0"
   /* _mesa_function_pool[9576]: Color3fVertex3fSUN (dynamic) */
   "ffffff\0"
   "glColor3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[9605]: ProgramEnvParameter4fvARB (will be remapped) */
   "iip\0"
   "glProgramEnvParameter4fvARB\0"
   "glProgramParameter4fvNV\0"
   "\0"
   /* _mesa_function_pool[9662]: Color4i (offset 31) */
   "iiii\0"
   "glColor4i\0"
   "\0"
   /* _mesa_function_pool[9678]: Color4f (offset 29) */
   "ffff\0"
   "glColor4f\0"
   "\0"
   /* _mesa_function_pool[9694]: RasterPos4fv (offset 81) */
   "p\0"
   "glRasterPos4fv\0"
   "\0"
   /* _mesa_function_pool[9712]: Color4d (offset 27) */
   "dddd\0"
   "glColor4d\0"
   "\0"
   /* _mesa_function_pool[9728]: ClearIndex (offset 205) */
   "f\0"
   "glClearIndex\0"
   "\0"
   /* _mesa_function_pool[9744]: Color4b (offset 25) */
   "iiii\0"
   "glColor4b\0"
   "\0"
   /* _mesa_function_pool[9760]: LoadMatrixd (offset 292) */
   "p\0"
   "glLoadMatrixd\0"
   "\0"
   /* _mesa_function_pool[9777]: FragmentLightModeliSGIX (dynamic) */
   "ii\0"
   "glFragmentLightModeliSGIX\0"
   "\0"
   /* _mesa_function_pool[9807]: RasterPos2dv (offset 63) */
   "p\0"
   "glRasterPos2dv\0"
   "\0"
   /* _mesa_function_pool[9825]: ConvolutionParameterfv (offset 351) */
   "iip\0"
   "glConvolutionParameterfv\0"
   "glConvolutionParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[9883]: TbufferMask3DFX (dynamic) */
   "i\0"
   "glTbufferMask3DFX\0"
   "\0"
   /* _mesa_function_pool[9904]: GetTexGendv (offset 278) */
   "iip\0"
   "glGetTexGendv\0"
   "\0"
   /* _mesa_function_pool[9923]: GetVertexAttribfvNV (will be remapped) */
   "iip\0"
   "glGetVertexAttribfvNV\0"
   "\0"
   /* _mesa_function_pool[9950]: BeginTransformFeedbackEXT (will be remapped) */
   "i\0"
   "glBeginTransformFeedbackEXT\0"
   "glBeginTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[10006]: LoadProgramNV (will be remapped) */
   "iiip\0"
   "glLoadProgramNV\0"
   "\0"
   /* _mesa_function_pool[10028]: WaitSync (will be remapped) */
   "iii\0"
   "glWaitSync\0"
   "\0"
   /* _mesa_function_pool[10044]: EndList (offset 1) */
   "\0"
   "glEndList\0"
   "\0"
   /* _mesa_function_pool[10056]: VertexP4ui (will be remapped) */
   "ii\0"
   "glVertexP4ui\0"
   "\0"
   /* _mesa_function_pool[10073]: VertexAttrib4fvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib4fvNV\0"
   "\0"
   /* _mesa_function_pool[10097]: GetAttachedObjectsARB (will be remapped) */
   "iipp\0"
   "glGetAttachedObjectsARB\0"
   "\0"
   /* _mesa_function_pool[10127]: Uniform3fvARB (will be remapped) */
   "iip\0"
   "glUniform3fv\0"
   "glUniform3fvARB\0"
   "\0"
   /* _mesa_function_pool[10161]: EvalCoord1fv (offset 231) */
   "p\0"
   "glEvalCoord1fv\0"
   "\0"
   /* _mesa_function_pool[10179]: DrawRangeElements (offset 338) */
   "iiiiip\0"
   "glDrawRangeElements\0"
   "glDrawRangeElementsEXT\0"
   "\0"
   /* _mesa_function_pool[10230]: EvalMesh2 (offset 238) */
   "iiiii\0"
   "glEvalMesh2\0"
   "\0"
   /* _mesa_function_pool[10249]: Vertex4fv (offset 145) */
   "p\0"
   "glVertex4fv\0"
   "\0"
   /* _mesa_function_pool[10264]: GenTransformFeedbacks (will be remapped) */
   "ip\0"
   "glGenTransformFeedbacks\0"
   "\0"
   /* _mesa_function_pool[10292]: SpriteParameterfvSGIX (dynamic) */
   "ip\0"
   "glSpriteParameterfvSGIX\0"
   "\0"
   /* _mesa_function_pool[10320]: CheckFramebufferStatusEXT (will be remapped) */
   "i\0"
   "glCheckFramebufferStatus\0"
   "glCheckFramebufferStatusEXT\0"
   "\0"
   /* _mesa_function_pool[10376]: GlobalAlphaFactoruiSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactoruiSUN\0"
   "\0"
   /* _mesa_function_pool[10404]: GetHandleARB (will be remapped) */
   "i\0"
   "glGetHandleARB\0"
   "\0"
   /* _mesa_function_pool[10422]: GetVertexAttribivARB (will be remapped) */
   "iip\0"
   "glGetVertexAttribiv\0"
   "glGetVertexAttribivARB\0"
   "\0"
   /* _mesa_function_pool[10470]: BlendFunciARB (will be remapped) */
   "iii\0"
   "glBlendFunciARB\0"
   "glBlendFuncIndexedAMD\0"
   "\0"
   /* _mesa_function_pool[10513]: VertexAttribP1ui (will be remapped) */
   "iiii\0"
   "glVertexAttribP1ui\0"
   "\0"
   /* _mesa_function_pool[10538]: GetnUniformivARB (will be remapped) */
   "iiip\0"
   "glGetnUniformivARB\0"
   "\0"
   /* _mesa_function_pool[10563]: GetTexParameterIivEXT (will be remapped) */
   "iip\0"
   "glGetTexParameterIivEXT\0"
   "glGetTexParameterIiv\0"
   "\0"
   /* _mesa_function_pool[10613]: CreateProgram (will be remapped) */
   "\0"
   "glCreateProgram\0"
   "\0"
   /* _mesa_function_pool[10631]: LoadTransposeMatrixdARB (will be remapped) */
   "p\0"
   "glLoadTransposeMatrixd\0"
   "glLoadTransposeMatrixdARB\0"
   "\0"
   /* _mesa_function_pool[10683]: ReleaseShaderCompiler (will be remapped) */
   "\0"
   "glReleaseShaderCompiler\0"
   "\0"
   /* _mesa_function_pool[10709]: GetMinmax (offset 364) */
   "iiiip\0"
   "glGetMinmax\0"
   "glGetMinmaxEXT\0"
   "\0"
   /* _mesa_function_pool[10743]: StencilFuncSeparate (will be remapped) */
   "iiii\0"
   "glStencilFuncSeparate\0"
   "\0"
   /* _mesa_function_pool[10771]: SecondaryColor3sEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3s\0"
   "glSecondaryColor3sEXT\0"
   "\0"
   /* _mesa_function_pool[10817]: Color3fVertex3fvSUN (dynamic) */
   "pp\0"
   "glColor3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[10843]: GetInteger64i_v (will be remapped) */
   "iip\0"
   "glGetInteger64i_v\0"
   "\0"
   /* _mesa_function_pool[10866]: GetVertexAttribdvNV (will be remapped) */
   "iip\0"
   "glGetVertexAttribdvNV\0"
   "\0"
   /* _mesa_function_pool[10893]: Normal3fv (offset 57) */
   "p\0"
   "glNormal3fv\0"
   "\0"
   /* _mesa_function_pool[10908]: GlobalAlphaFactorbSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactorbSUN\0"
   "\0"
   /* _mesa_function_pool[10935]: Color3us (offset 23) */
   "iii\0"
   "glColor3us\0"
   "\0"
   /* _mesa_function_pool[10951]: ImageTransformParameterfvHP (dynamic) */
   "iip\0"
   "glImageTransformParameterfvHP\0"
   "\0"
   /* _mesa_function_pool[10986]: VertexAttrib4ivARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4iv\0"
   "glVertexAttrib4ivARB\0"
   "\0"
   /* _mesa_function_pool[11029]: End (offset 43) */
   "\0"
   "glEnd\0"
   "\0"
   /* _mesa_function_pool[11037]: VertexAttrib3fNV (will be remapped) */
   "ifff\0"
   "glVertexAttrib3fNV\0"
   "\0"
   /* _mesa_function_pool[11062]: MultiTexCoordP3uiv (will be remapped) */
   "iip\0"
   "glMultiTexCoordP3uiv\0"
   "\0"
   /* _mesa_function_pool[11088]: VertexAttribs2dvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs2dvNV\0"
   "\0"
   /* _mesa_function_pool[11114]: GetQueryObjectui64vEXT (will be remapped) */
   "iip\0"
   "glGetQueryObjectui64vEXT\0"
   "\0"
   /* _mesa_function_pool[11144]: MultiTexCoord3fvARB (offset 395) */
   "ip\0"
   "glMultiTexCoord3fv\0"
   "glMultiTexCoord3fvARB\0"
   "\0"
   /* _mesa_function_pool[11189]: SecondaryColor3dEXT (will be remapped) */
   "ddd\0"
   "glSecondaryColor3d\0"
   "glSecondaryColor3dEXT\0"
   "\0"
   /* _mesa_function_pool[11235]: Color3ub (offset 19) */
   "iii\0"
   "glColor3ub\0"
   "\0"
   /* _mesa_function_pool[11251]: GetProgramParameterfvNV (will be remapped) */
   "iiip\0"
   "glGetProgramParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[11283]: TangentPointerEXT (dynamic) */
   "iip\0"
   "glTangentPointerEXT\0"
   "\0"
   /* _mesa_function_pool[11308]: Color4fNormal3fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glColor4fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[11343]: GetInstrumentsSGIX (dynamic) */
   "\0"
   "glGetInstrumentsSGIX\0"
   "\0"
   /* _mesa_function_pool[11366]: GetUniformuivEXT (will be remapped) */
   "iip\0"
   "glGetUniformuivEXT\0"
   "glGetUniformuiv\0"
   "\0"
   /* _mesa_function_pool[11406]: Color3ui (offset 21) */
   "iii\0"
   "glColor3ui\0"
   "\0"
   /* _mesa_function_pool[11422]: EvalMapsNV (dynamic) */
   "ii\0"
   "glEvalMapsNV\0"
   "\0"
   /* _mesa_function_pool[11439]: TexSubImage2D (offset 333) */
   "iiiiiiiip\0"
   "glTexSubImage2D\0"
   "glTexSubImage2DEXT\0"
   "\0"
   /* _mesa_function_pool[11485]: FragmentLightivSGIX (dynamic) */
   "iip\0"
   "glFragmentLightivSGIX\0"
   "\0"
   /* _mesa_function_pool[11512]: GetTexParameterPointervAPPLE (will be remapped) */
   "iip\0"
   "glGetTexParameterPointervAPPLE\0"
   "\0"
   /* _mesa_function_pool[11548]: TexGenfv (offset 191) */
   "iip\0"
   "glTexGenfv\0"
   "\0"
   /* _mesa_function_pool[11564]: GetTransformFeedbackVaryingEXT (will be remapped) */
   "iiipppp\0"
   "glGetTransformFeedbackVaryingEXT\0"
   "glGetTransformFeedbackVarying\0"
   "\0"
   /* _mesa_function_pool[11636]: VertexAttrib4bvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4bv\0"
   "glVertexAttrib4bvARB\0"
   "\0"
   /* _mesa_function_pool[11679]: ShaderBinary (will be remapped) */
   "ipipi\0"
   "glShaderBinary\0"
   "\0"
   /* _mesa_function_pool[11701]: GetIntegerIndexedvEXT (will be remapped) */
   "iip\0"
   "glGetIntegerIndexedvEXT\0"
   "glGetIntegeri_v\0"
   "\0"
   /* _mesa_function_pool[11746]: MultiTexCoord4sARB (offset 406) */
   "iiiii\0"
   "glMultiTexCoord4s\0"
   "glMultiTexCoord4sARB\0"
   "\0"
   /* _mesa_function_pool[11792]: GetFragmentMaterialivSGIX (dynamic) */
   "iip\0"
   "glGetFragmentMaterialivSGIX\0"
   "\0"
   /* _mesa_function_pool[11825]: WindowPos4dMESA (will be remapped) */
   "dddd\0"
   "glWindowPos4dMESA\0"
   "\0"
   /* _mesa_function_pool[11849]: WeightPointerARB (dynamic) */
   "iiip\0"
   "glWeightPointerARB\0"
   "\0"
   /* _mesa_function_pool[11874]: WindowPos2dMESA (will be remapped) */
   "dd\0"
   "glWindowPos2d\0"
   "glWindowPos2dARB\0"
   "glWindowPos2dMESA\0"
   "\0"
   /* _mesa_function_pool[11927]: FramebufferTexture3DEXT (will be remapped) */
   "iiiiii\0"
   "glFramebufferTexture3D\0"
   "glFramebufferTexture3DEXT\0"
   "\0"
   /* _mesa_function_pool[11984]: BlendEquation (offset 337) */
   "i\0"
   "glBlendEquation\0"
   "glBlendEquationEXT\0"
   "\0"
   /* _mesa_function_pool[12022]: VertexAttrib3dNV (will be remapped) */
   "iddd\0"
   "glVertexAttrib3dNV\0"
   "\0"
   /* _mesa_function_pool[12047]: VertexAttrib3dARB (will be remapped) */
   "iddd\0"
   "glVertexAttrib3d\0"
   "glVertexAttrib3dARB\0"
   "\0"
   /* _mesa_function_pool[12090]: VertexAttribI4usvEXT (will be remapped) */
   "ip\0"
   "glVertexAttribI4usvEXT\0"
   "glVertexAttribI4usv\0"
   "\0"
   /* _mesa_function_pool[12137]: ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN (dynamic) */
   "ppppp\0"
   "glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[12201]: VertexAttrib4fARB (will be remapped) */
   "iffff\0"
   "glVertexAttrib4f\0"
   "glVertexAttrib4fARB\0"
   "\0"
   /* _mesa_function_pool[12245]: GetError (offset 261) */
   "\0"
   "glGetError\0"
   "\0"
   /* _mesa_function_pool[12258]: IndexFuncEXT (dynamic) */
   "if\0"
   "glIndexFuncEXT\0"
   "\0"
   /* _mesa_function_pool[12277]: TexCoord3dv (offset 111) */
   "p\0"
   "glTexCoord3dv\0"
   "\0"
   /* _mesa_function_pool[12294]: Indexdv (offset 45) */
   "p\0"
   "glIndexdv\0"
   "\0"
   /* _mesa_function_pool[12307]: FramebufferTexture2DEXT (will be remapped) */
   "iiiii\0"
   "glFramebufferTexture2D\0"
   "glFramebufferTexture2DEXT\0"
   "\0"
   /* _mesa_function_pool[12363]: Normal3s (offset 60) */
   "iii\0"
   "glNormal3s\0"
   "\0"
   /* _mesa_function_pool[12379]: GetObjectParameterivAPPLE (will be remapped) */
   "iiip\0"
   "glGetObjectParameterivAPPLE\0"
   "\0"
   /* _mesa_function_pool[12413]: PushName (offset 201) */
   "i\0"
   "glPushName\0"
   "\0"
   /* _mesa_function_pool[12427]: MultiTexCoord2dvARB (offset 385) */
   "ip\0"
   "glMultiTexCoord2dv\0"
   "glMultiTexCoord2dvARB\0"
   "\0"
   /* _mesa_function_pool[12472]: CullParameterfvEXT (dynamic) */
   "ip\0"
   "glCullParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[12497]: Normal3i (offset 58) */
   "iii\0"
   "glNormal3i\0"
   "\0"
   /* _mesa_function_pool[12513]: ProgramNamedParameter4fvNV (will be remapped) */
   "iipp\0"
   "glProgramNamedParameter4fvNV\0"
   "\0"
   /* _mesa_function_pool[12548]: SecondaryColorPointerEXT (will be remapped) */
   "iiip\0"
   "glSecondaryColorPointer\0"
   "glSecondaryColorPointerEXT\0"
   "\0"
   /* _mesa_function_pool[12605]: VertexAttrib4fvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4fv\0"
   "glVertexAttrib4fvARB\0"
   "\0"
   /* _mesa_function_pool[12648]: PixelTexGenSGIX (will be remapped) */
   "i\0"
   "glPixelTexGenSGIX\0"
   "\0"
   /* _mesa_function_pool[12669]: GetActiveUniformARB (will be remapped) */
   "iiipppp\0"
   "glGetActiveUniform\0"
   "glGetActiveUniformARB\0"
   "\0"
   /* _mesa_function_pool[12719]: ImageTransformParameteriHP (dynamic) */
   "iii\0"
   "glImageTransformParameteriHP\0"
   "\0"
   /* _mesa_function_pool[12753]: Normal3b (offset 52) */
   "iii\0"
   "glNormal3b\0"
   "\0"
   /* _mesa_function_pool[12769]: Normal3d (offset 54) */
   "ddd\0"
   "glNormal3d\0"
   "\0"
   /* _mesa_function_pool[12785]: Uniform1uiEXT (will be remapped) */
   "ii\0"
   "glUniform1uiEXT\0"
   "glUniform1ui\0"
   "\0"
   /* _mesa_function_pool[12818]: Normal3f (offset 56) */
   "fff\0"
   "glNormal3f\0"
   "\0"
   /* _mesa_function_pool[12834]: MultiTexCoord1svARB (offset 383) */
   "ip\0"
   "glMultiTexCoord1sv\0"
   "glMultiTexCoord1svARB\0"
   "\0"
   /* _mesa_function_pool[12879]: Indexi (offset 48) */
   "i\0"
   "glIndexi\0"
   "\0"
   /* _mesa_function_pool[12891]: EGLImageTargetTexture2DOES (will be remapped) */
   "ip\0"
   "glEGLImageTargetTexture2DOES\0"
   "\0"
   /* _mesa_function_pool[12924]: EndQueryARB (will be remapped) */
   "i\0"
   "glEndQuery\0"
   "glEndQueryARB\0"
   "\0"
   /* _mesa_function_pool[12952]: DeleteFencesNV (will be remapped) */
   "ip\0"
   "glDeleteFencesNV\0"
   "\0"
   /* _mesa_function_pool[12973]: ColorPointerListIBM (dynamic) */
   "iiipi\0"
   "glColorPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[13002]: BindBufferRangeEXT (will be remapped) */
   "iiiii\0"
   "glBindBufferRangeEXT\0"
   "glBindBufferRange\0"
   "\0"
   /* _mesa_function_pool[13048]: DepthMask (offset 211) */
   "i\0"
   "glDepthMask\0"
   "\0"
   /* _mesa_function_pool[13063]: IsShader (will be remapped) */
   "i\0"
   "glIsShader\0"
   "\0"
   /* _mesa_function_pool[13077]: Indexf (offset 46) */
   "f\0"
   "glIndexf\0"
   "\0"
   /* _mesa_function_pool[13089]: GetImageTransformParameterivHP (dynamic) */
   "iip\0"
   "glGetImageTransformParameterivHP\0"
   "\0"
   /* _mesa_function_pool[13127]: Indexd (offset 44) */
   "d\0"
   "glIndexd\0"
   "\0"
   /* _mesa_function_pool[13139]: GetMaterialiv (offset 270) */
   "iip\0"
   "glGetMaterialiv\0"
   "\0"
   /* _mesa_function_pool[13160]: StencilOp (offset 244) */
   "iii\0"
   "glStencilOp\0"
   "\0"
   /* _mesa_function_pool[13177]: WindowPos4ivMESA (will be remapped) */
   "p\0"
   "glWindowPos4ivMESA\0"
   "\0"
   /* _mesa_function_pool[13199]: MultiTexCoord3svARB (offset 399) */
   "ip\0"
   "glMultiTexCoord3sv\0"
   "glMultiTexCoord3svARB\0"
   "\0"
   /* _mesa_function_pool[13244]: TexEnvfv (offset 185) */
   "iip\0"
   "glTexEnvfv\0"
   "\0"
   /* _mesa_function_pool[13260]: MultiTexCoord4iARB (offset 404) */
   "iiiii\0"
   "glMultiTexCoord4i\0"
   "glMultiTexCoord4iARB\0"
   "\0"
   /* _mesa_function_pool[13306]: Indexs (offset 50) */
   "i\0"
   "glIndexs\0"
   "\0"
   /* _mesa_function_pool[13318]: Binormal3ivEXT (dynamic) */
   "p\0"
   "glBinormal3ivEXT\0"
   "\0"
   /* _mesa_function_pool[13338]: ResizeBuffersMESA (will be remapped) */
   "\0"
   "glResizeBuffersMESA\0"
   "\0"
   /* _mesa_function_pool[13360]: MultiTexCoordP1uiv (will be remapped) */
   "iip\0"
   "glMultiTexCoordP1uiv\0"
   "\0"
   /* _mesa_function_pool[13386]: BlendFuncSeparateiARB (will be remapped) */
   "iiiii\0"
   "glBlendFuncSeparateiARB\0"
   "glBlendFuncSeparateIndexedAMD\0"
   "\0"
   /* _mesa_function_pool[13447]: GetUniformivARB (will be remapped) */
   "iip\0"
   "glGetUniformiv\0"
   "glGetUniformivARB\0"
   "\0"
   /* _mesa_function_pool[13485]: PixelTexGenParameteriSGIS (will be remapped) */
   "ii\0"
   "glPixelTexGenParameteriSGIS\0"
   "\0"
   /* _mesa_function_pool[13517]: VertexPointervINTEL (dynamic) */
   "iip\0"
   "glVertexPointervINTEL\0"
   "\0"
   /* _mesa_function_pool[13544]: Vertex2i (offset 130) */
   "ii\0"
   "glVertex2i\0"
   "\0"
   /* _mesa_function_pool[13559]: LoadMatrixf (offset 291) */
   "p\0"
   "glLoadMatrixf\0"
   "\0"
   /* _mesa_function_pool[13576]: VertexAttribI1uivEXT (will be remapped) */
   "ip\0"
   "glVertexAttribI1uivEXT\0"
   "glVertexAttribI1uiv\0"
   "\0"
   /* _mesa_function_pool[13623]: Vertex2f (offset 128) */
   "ff\0"
   "glVertex2f\0"
   "\0"
   /* _mesa_function_pool[13638]: ReplacementCodeuiColor4fNormal3fVertex3fvSUN (dynamic) */
   "pppp\0"
   "glReplacementCodeuiColor4fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[13691]: Color4bv (offset 26) */
   "p\0"
   "glColor4bv\0"
   "\0"
   /* _mesa_function_pool[13705]: VertexPointer (offset 321) */
   "iiip\0"
   "glVertexPointer\0"
   "\0"
   /* _mesa_function_pool[13727]: SecondaryColor3uiEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3ui\0"
   "glSecondaryColor3uiEXT\0"
   "\0"
   /* _mesa_function_pool[13775]: StartInstrumentsSGIX (dynamic) */
   "\0"
   "glStartInstrumentsSGIX\0"
   "\0"
   /* _mesa_function_pool[13800]: SecondaryColor3usvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3usv\0"
   "glSecondaryColor3usvEXT\0"
   "\0"
   /* _mesa_function_pool[13848]: VertexAttrib2fvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib2fvNV\0"
   "\0"
   /* _mesa_function_pool[13872]: ProgramLocalParameter4dvARB (will be remapped) */
   "iip\0"
   "glProgramLocalParameter4dvARB\0"
   "\0"
   /* _mesa_function_pool[13907]: DeleteLists (offset 4) */
   "ii\0"
   "glDeleteLists\0"
   "\0"
   /* _mesa_function_pool[13925]: LogicOp (offset 242) */
   "i\0"
   "glLogicOp\0"
   "\0"
   /* _mesa_function_pool[13938]: MatrixIndexuivARB (dynamic) */
   "ip\0"
   "glMatrixIndexuivARB\0"
   "\0"
   /* _mesa_function_pool[13962]: Vertex2s (offset 132) */
   "ii\0"
   "glVertex2s\0"
   "\0"
   /* _mesa_function_pool[13977]: RenderbufferStorageMultisample (will be remapped) */
   "iiiii\0"
   "glRenderbufferStorageMultisample\0"
   "glRenderbufferStorageMultisampleEXT\0"
   "\0"
   /* _mesa_function_pool[14053]: TexCoord4fv (offset 121) */
   "p\0"
   "glTexCoord4fv\0"
   "\0"
   /* _mesa_function_pool[14070]: Tangent3sEXT (dynamic) */
   "iii\0"
   "glTangent3sEXT\0"
   "\0"
   /* _mesa_function_pool[14090]: GlobalAlphaFactorfSUN (dynamic) */
   "f\0"
   "glGlobalAlphaFactorfSUN\0"
   "\0"
   /* _mesa_function_pool[14117]: MultiTexCoord3iARB (offset 396) */
   "iiii\0"
   "glMultiTexCoord3i\0"
   "glMultiTexCoord3iARB\0"
   "\0"
   /* _mesa_function_pool[14162]: IsProgram (will be remapped) */
   "i\0"
   "glIsProgram\0"
   "\0"
   /* _mesa_function_pool[14177]: TexCoordPointerListIBM (dynamic) */
   "iiipi\0"
   "glTexCoordPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[14209]: VertexAttribI4svEXT (will be remapped) */
   "ip\0"
   "glVertexAttribI4svEXT\0"
   "glVertexAttribI4sv\0"
   "\0"
   /* _mesa_function_pool[14254]: GlobalAlphaFactorusSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactorusSUN\0"
   "\0"
   /* _mesa_function_pool[14282]: VertexAttrib2dvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib2dvNV\0"
   "\0"
   /* _mesa_function_pool[14306]: FramebufferRenderbufferEXT (will be remapped) */
   "iiii\0"
   "glFramebufferRenderbuffer\0"
   "glFramebufferRenderbufferEXT\0"
   "\0"
   /* _mesa_function_pool[14367]: ClearBufferuiv (will be remapped) */
   "iip\0"
   "glClearBufferuiv\0"
   "\0"
   /* _mesa_function_pool[14389]: VertexAttrib1dvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib1dvNV\0"
   "\0"
   /* _mesa_function_pool[14413]: GenTextures (offset 328) */
   "ip\0"
   "glGenTextures\0"
   "glGenTexturesEXT\0"
   "\0"
   /* _mesa_function_pool[14448]: FramebufferTextureARB (will be remapped) */
   "iiii\0"
   "glFramebufferTextureARB\0"
   "\0"
   /* _mesa_function_pool[14478]: SetFenceNV (will be remapped) */
   "ii\0"
   "glSetFenceNV\0"
   "\0"
   /* _mesa_function_pool[14495]: FramebufferTexture1DEXT (will be remapped) */
   "iiiii\0"
   "glFramebufferTexture1D\0"
   "glFramebufferTexture1DEXT\0"
   "\0"
   /* _mesa_function_pool[14551]: GetCombinerOutputParameterivNV (will be remapped) */
   "iiip\0"
   "glGetCombinerOutputParameterivNV\0"
   "\0"
   /* _mesa_function_pool[14590]: PixelTexGenParameterivSGIS (will be remapped) */
   "ip\0"
   "glPixelTexGenParameterivSGIS\0"
   "\0"
   /* _mesa_function_pool[14623]: TextureNormalEXT (dynamic) */
   "i\0"
   "glTextureNormalEXT\0"
   "\0"
   /* _mesa_function_pool[14645]: IndexPointerListIBM (dynamic) */
   "iipi\0"
   "glIndexPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[14673]: WeightfvARB (dynamic) */
   "ip\0"
   "glWeightfvARB\0"
   "\0"
   /* _mesa_function_pool[14691]: MultiTexCoordP3ui (will be remapped) */
   "iii\0"
   "glMultiTexCoordP3ui\0"
   "\0"
   /* _mesa_function_pool[14716]: RasterPos2sv (offset 69) */
   "p\0"
   "glRasterPos2sv\0"
   "\0"
   /* _mesa_function_pool[14734]: Color4ubv (offset 36) */
   "p\0"
   "glColor4ubv\0"
   "\0"
   /* _mesa_function_pool[14749]: DrawBuffer (offset 202) */
   "i\0"
   "glDrawBuffer\0"
   "\0"
   /* _mesa_function_pool[14765]: TexCoord2fv (offset 105) */
   "p\0"
   "glTexCoord2fv\0"
   "\0"
   /* _mesa_function_pool[14782]: WindowPos4fMESA (will be remapped) */
   "ffff\0"
   "glWindowPos4fMESA\0"
   "\0"
   /* _mesa_function_pool[14806]: TexCoord1sv (offset 101) */
   "p\0"
   "glTexCoord1sv\0"
   "\0"
   /* _mesa_function_pool[14823]: WindowPos3dvMESA (will be remapped) */
   "p\0"
   "glWindowPos3dv\0"
   "glWindowPos3dvARB\0"
   "glWindowPos3dvMESA\0"
   "\0"
   /* _mesa_function_pool[14878]: VertexAttribP1uiv (will be remapped) */
   "iiip\0"
   "glVertexAttribP1uiv\0"
   "\0"
   /* _mesa_function_pool[14904]: VertexAttribP4ui (will be remapped) */
   "iiii\0"
   "glVertexAttribP4ui\0"
   "\0"
   /* _mesa_function_pool[14929]: DepthFunc (offset 245) */
   "i\0"
   "glDepthFunc\0"
   "\0"
   /* _mesa_function_pool[14944]: PixelMapusv (offset 253) */
   "iip\0"
   "glPixelMapusv\0"
   "\0"
   /* _mesa_function_pool[14963]: GetQueryObjecti64vEXT (will be remapped) */
   "iip\0"
   "glGetQueryObjecti64vEXT\0"
   "\0"
   /* _mesa_function_pool[14992]: MultiTexCoord1dARB (offset 376) */
   "id\0"
   "glMultiTexCoord1d\0"
   "glMultiTexCoord1dARB\0"
   "\0"
   /* _mesa_function_pool[15035]: PointParameterivNV (will be remapped) */
   "ip\0"
   "glPointParameteriv\0"
   "glPointParameterivNV\0"
   "\0"
   /* _mesa_function_pool[15079]: IsSampler (will be remapped) */
   "i\0"
   "glIsSampler\0"
   "\0"
   /* _mesa_function_pool[15094]: BlendFunc (offset 241) */
   "ii\0"
   "glBlendFunc\0"
   "\0"
   /* _mesa_function_pool[15110]: EndTransformFeedbackEXT (will be remapped) */
   "\0"
   "glEndTransformFeedbackEXT\0"
   "glEndTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[15161]: Uniform2fvARB (will be remapped) */
   "iip\0"
   "glUniform2fv\0"
   "glUniform2fvARB\0"
   "\0"
   /* _mesa_function_pool[15195]: BufferParameteriAPPLE (will be remapped) */
   "iii\0"
   "glBufferParameteriAPPLE\0"
   "\0"
   /* _mesa_function_pool[15224]: MultiTexCoord3dvARB (offset 393) */
   "ip\0"
   "glMultiTexCoord3dv\0"
   "glMultiTexCoord3dvARB\0"
   "\0"
   /* _mesa_function_pool[15269]: ReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN (dynamic) */
   "pppp\0"
   "glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[15325]: DeleteObjectARB (will be remapped) */
   "i\0"
   "glDeleteObjectARB\0"
   "\0"
   /* _mesa_function_pool[15346]: GetShaderPrecisionFormat (will be remapped) */
   "iipp\0"
   "glGetShaderPrecisionFormat\0"
   "\0"
   /* _mesa_function_pool[15379]: MatrixIndexPointerARB (dynamic) */
   "iiip\0"
   "glMatrixIndexPointerARB\0"
   "\0"
   /* _mesa_function_pool[15409]: ProgramNamedParameter4dvNV (will be remapped) */
   "iipp\0"
   "glProgramNamedParameter4dvNV\0"
   "\0"
   /* _mesa_function_pool[15444]: Tangent3fvEXT (dynamic) */
   "p\0"
   "glTangent3fvEXT\0"
   "\0"
   /* _mesa_function_pool[15463]: Flush (offset 217) */
   "\0"
   "glFlush\0"
   "\0"
   /* _mesa_function_pool[15473]: Color4uiv (offset 38) */
   "p\0"
   "glColor4uiv\0"
   "\0"
   /* _mesa_function_pool[15488]: VertexAttribI4iEXT (will be remapped) */
   "iiiii\0"
   "glVertexAttribI4iEXT\0"
   "glVertexAttribI4i\0"
   "\0"
   /* _mesa_function_pool[15534]: GenVertexArrays (will be remapped) */
   "ip\0"
   "glGenVertexArrays\0"
   "\0"
   /* _mesa_function_pool[15556]: Uniform3uivEXT (will be remapped) */
   "iip\0"
   "glUniform3uivEXT\0"
   "glUniform3uiv\0"
   "\0"
   /* _mesa_function_pool[15592]: RasterPos3sv (offset 77) */
   "p\0"
   "glRasterPos3sv\0"
   "\0"
   /* _mesa_function_pool[15610]: TexCoordP2ui (will be remapped) */
   "ii\0"
   "glTexCoordP2ui\0"
   "\0"
   /* _mesa_function_pool[15629]: BindFramebufferEXT (will be remapped) */
   "ii\0"
   "glBindFramebuffer\0"
   "glBindFramebufferEXT\0"
   "\0"
   /* _mesa_function_pool[15672]: ReferencePlaneSGIX (dynamic) */
   "p\0"
   "glReferencePlaneSGIX\0"
   "\0"
   /* _mesa_function_pool[15696]: PushAttrib (offset 219) */
   "i\0"
   "glPushAttrib\0"
   "\0"
   /* _mesa_function_pool[15712]: RasterPos2i (offset 66) */
   "ii\0"
   "glRasterPos2i\0"
   "\0"
   /* _mesa_function_pool[15730]: ValidateProgramARB (will be remapped) */
   "i\0"
   "glValidateProgram\0"
   "glValidateProgramARB\0"
   "\0"
   /* _mesa_function_pool[15772]: TexParameteriv (offset 181) */
   "iip\0"
   "glTexParameteriv\0"
   "\0"
   /* _mesa_function_pool[15794]: UnlockArraysEXT (will be remapped) */
   "\0"
   "glUnlockArraysEXT\0"
   "\0"
   /* _mesa_function_pool[15814]: TexCoord2fColor3fVertex3fSUN (dynamic) */
   "ffffffff\0"
   "glTexCoord2fColor3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[15855]: WindowPos3fvMESA (will be remapped) */
   "p\0"
   "glWindowPos3fv\0"
   "glWindowPos3fvARB\0"
   "glWindowPos3fvMESA\0"
   "\0"
   /* _mesa_function_pool[15910]: RasterPos2f (offset 64) */
   "ff\0"
   "glRasterPos2f\0"
   "\0"
   /* _mesa_function_pool[15928]: VertexAttrib1svNV (will be remapped) */
   "ip\0"
   "glVertexAttrib1svNV\0"
   "\0"
   /* _mesa_function_pool[15952]: RasterPos2d (offset 62) */
   "dd\0"
   "glRasterPos2d\0"
   "\0"
   /* _mesa_function_pool[15970]: RasterPos3fv (offset 73) */
   "p\0"
   "glRasterPos3fv\0"
   "\0"
   /* _mesa_function_pool[15988]: CopyTexSubImage3D (offset 373) */
   "iiiiiiiii\0"
   "glCopyTexSubImage3D\0"
   "glCopyTexSubImage3DEXT\0"
   "\0"
   /* _mesa_function_pool[16042]: VertexAttrib2dARB (will be remapped) */
   "idd\0"
   "glVertexAttrib2d\0"
   "glVertexAttrib2dARB\0"
   "\0"
   /* _mesa_function_pool[16084]: Color4ub (offset 35) */
   "iiii\0"
   "glColor4ub\0"
   "\0"
   /* _mesa_function_pool[16101]: GetInteger64v (will be remapped) */
   "ip\0"
   "glGetInteger64v\0"
   "\0"
   /* _mesa_function_pool[16121]: TextureColorMaskSGIS (dynamic) */
   "iiii\0"
   "glTextureColorMaskSGIS\0"
   "\0"
   /* _mesa_function_pool[16150]: RasterPos2s (offset 68) */
   "ii\0"
   "glRasterPos2s\0"
   "\0"
   /* _mesa_function_pool[16168]: GetColorTable (offset 343) */
   "iiip\0"
   "glGetColorTable\0"
   "glGetColorTableSGI\0"
   "glGetColorTableEXT\0"
   "\0"
   /* _mesa_function_pool[16228]: SelectBuffer (offset 195) */
   "ip\0"
   "glSelectBuffer\0"
   "\0"
   /* _mesa_function_pool[16247]: Indexiv (offset 49) */
   "p\0"
   "glIndexiv\0"
   "\0"
   /* _mesa_function_pool[16260]: TexCoord3i (offset 114) */
   "iii\0"
   "glTexCoord3i\0"
   "\0"
   /* _mesa_function_pool[16278]: CopyColorTable (offset 342) */
   "iiiii\0"
   "glCopyColorTable\0"
   "glCopyColorTableSGI\0"
   "\0"
   /* _mesa_function_pool[16322]: GetHistogramParameterfv (offset 362) */
   "iip\0"
   "glGetHistogramParameterfv\0"
   "glGetHistogramParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[16382]: Frustum (offset 289) */
   "dddddd\0"
   "glFrustum\0"
   "\0"
   /* _mesa_function_pool[16400]: GetString (offset 275) */
   "i\0"
   "glGetString\0"
   "\0"
   /* _mesa_function_pool[16415]: ColorPointervINTEL (dynamic) */
   "iip\0"
   "glColorPointervINTEL\0"
   "\0"
   /* _mesa_function_pool[16441]: TexEnvf (offset 184) */
   "iif\0"
   "glTexEnvf\0"
   "\0"
   /* _mesa_function_pool[16456]: VertexP2uiv (will be remapped) */
   "ip\0"
   "glVertexP2uiv\0"
   "\0"
   /* _mesa_function_pool[16474]: TexCoord3d (offset 110) */
   "ddd\0"
   "glTexCoord3d\0"
   "\0"
   /* _mesa_function_pool[16492]: AlphaFragmentOp1ATI (will be remapped) */
   "iiiiii\0"
   "glAlphaFragmentOp1ATI\0"
   "\0"
   /* _mesa_function_pool[16522]: TexCoord3f (offset 112) */
   "fff\0"
   "glTexCoord3f\0"
   "\0"
   /* _mesa_function_pool[16540]: MultiTexCoord3ivARB (offset 397) */
   "ip\0"
   "glMultiTexCoord3iv\0"
   "glMultiTexCoord3ivARB\0"
   "\0"
   /* _mesa_function_pool[16585]: MultiTexCoord2sARB (offset 390) */
   "iii\0"
   "glMultiTexCoord2s\0"
   "glMultiTexCoord2sARB\0"
   "\0"
   /* _mesa_function_pool[16629]: VertexAttrib1dvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib1dv\0"
   "glVertexAttrib1dvARB\0"
   "\0"
   /* _mesa_function_pool[16672]: GetnHistogramARB (will be remapped) */
   "iiiiip\0"
   "glGetnHistogramARB\0"
   "\0"
   /* _mesa_function_pool[16699]: DeleteTextures (offset 327) */
   "ip\0"
   "glDeleteTextures\0"
   "glDeleteTexturesEXT\0"
   "\0"
   /* _mesa_function_pool[16740]: TexCoordPointerEXT (will be remapped) */
   "iiiip\0"
   "glTexCoordPointerEXT\0"
   "\0"
   /* _mesa_function_pool[16768]: TexSubImage4DSGIS (dynamic) */
   "iiiiiiiiiiiip\0"
   "glTexSubImage4DSGIS\0"
   "\0"
   /* _mesa_function_pool[16803]: TexCoord3s (offset 116) */
   "iii\0"
   "glTexCoord3s\0"
   "\0"
   /* _mesa_function_pool[16821]: GetTexLevelParameteriv (offset 285) */
   "iiip\0"
   "glGetTexLevelParameteriv\0"
   "\0"
   /* _mesa_function_pool[16852]: CombinerStageParameterfvNV (dynamic) */
   "iip\0"
   "glCombinerStageParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[16886]: VertexAttribP4uiv (will be remapped) */
   "iiip\0"
   "glVertexAttribP4uiv\0"
   "\0"
   /* _mesa_function_pool[16912]: StopInstrumentsSGIX (dynamic) */
   "i\0"
   "glStopInstrumentsSGIX\0"
   "\0"
   /* _mesa_function_pool[16937]: TexCoord4fColor4fNormal3fVertex4fSUN (dynamic) */
   "fffffffffffffff\0"
   "glTexCoord4fColor4fNormal3fVertex4fSUN\0"
   "\0"
   /* _mesa_function_pool[16993]: ClearAccum (offset 204) */
   "ffff\0"
   "glClearAccum\0"
   "\0"
   /* _mesa_function_pool[17012]: DeformSGIX (dynamic) */
   "i\0"
   "glDeformSGIX\0"
   "\0"
   /* _mesa_function_pool[17028]: GetVertexAttribfvARB (will be remapped) */
   "iip\0"
   "glGetVertexAttribfv\0"
   "glGetVertexAttribfvARB\0"
   "\0"
   /* _mesa_function_pool[17076]: SecondaryColor3ivEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3iv\0"
   "glSecondaryColor3ivEXT\0"
   "\0"
   /* _mesa_function_pool[17122]: TexCoord4iv (offset 123) */
   "p\0"
   "glTexCoord4iv\0"
   "\0"
   /* _mesa_function_pool[17139]: VertexAttribI4uiEXT (will be remapped) */
   "iiiii\0"
   "glVertexAttribI4uiEXT\0"
   "glVertexAttribI4ui\0"
   "\0"
   /* _mesa_function_pool[17187]: GetFragmentMaterialfvSGIX (dynamic) */
   "iip\0"
   "glGetFragmentMaterialfvSGIX\0"
   "\0"
   /* _mesa_function_pool[17220]: UniformMatrix4x2fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix4x2fv\0"
   "\0"
   /* _mesa_function_pool[17247]: GetDetailTexFuncSGIS (dynamic) */
   "ip\0"
   "glGetDetailTexFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[17274]: GetCombinerStageParameterfvNV (dynamic) */
   "iip\0"
   "glGetCombinerStageParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[17311]: SamplerParameterIiv (will be remapped) */
   "iip\0"
   "glSamplerParameterIiv\0"
   "\0"
   /* _mesa_function_pool[17338]: PolygonOffset (offset 319) */
   "ff\0"
   "glPolygonOffset\0"
   "\0"
   /* _mesa_function_pool[17358]: BindVertexArray (will be remapped) */
   "i\0"
   "glBindVertexArray\0"
   "\0"
   /* _mesa_function_pool[17379]: Color4ubVertex2fvSUN (dynamic) */
   "pp\0"
   "glColor4ubVertex2fvSUN\0"
   "\0"
   /* _mesa_function_pool[17406]: VertexP3uiv (will be remapped) */
   "ip\0"
   "glVertexP3uiv\0"
   "\0"
   /* _mesa_function_pool[17424]: Rectd (offset 86) */
   "dddd\0"
   "glRectd\0"
   "\0"
   /* _mesa_function_pool[17438]: TexFilterFuncSGIS (dynamic) */
   "iiip\0"
   "glTexFilterFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[17464]: TextureBarrierNV (will be remapped) */
   "\0"
   "glTextureBarrierNV\0"
   "\0"
   /* _mesa_function_pool[17485]: SamplerParameterfv (will be remapped) */
   "iip\0"
   "glSamplerParameterfv\0"
   "\0"
   /* _mesa_function_pool[17511]: ColorMaskIndexedEXT (will be remapped) */
   "iiiii\0"
   "glColorMaskIndexedEXT\0"
   "glColorMaski\0"
   "\0"
   /* _mesa_function_pool[17553]: GetAttribLocationARB (will be remapped) */
   "ip\0"
   "glGetAttribLocation\0"
   "glGetAttribLocationARB\0"
   "\0"
   /* _mesa_function_pool[17600]: RasterPos3i (offset 74) */
   "iii\0"
   "glRasterPos3i\0"
   "\0"
   /* _mesa_function_pool[17619]: VertexAttrib4ubvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4ubv\0"
   "glVertexAttrib4ubvARB\0"
   "\0"
   /* _mesa_function_pool[17664]: DetailTexFuncSGIS (dynamic) */
   "iip\0"
   "glDetailTexFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[17689]: Normal3fVertex3fSUN (dynamic) */
   "ffffff\0"
   "glNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[17719]: CopyTexImage2D (offset 324) */
   "iiiiiiii\0"
   "glCopyTexImage2D\0"
   "glCopyTexImage2DEXT\0"
   "\0"
   /* _mesa_function_pool[17766]: GetBufferPointervARB (will be remapped) */
   "iip\0"
   "glGetBufferPointerv\0"
   "glGetBufferPointervARB\0"
   "\0"
   /* _mesa_function_pool[17814]: ProgramEnvParameter4fARB (will be remapped) */
   "iiffff\0"
   "glProgramEnvParameter4fARB\0"
   "glProgramParameter4fNV\0"
   "\0"
   /* _mesa_function_pool[17872]: Uniform3ivARB (will be remapped) */
   "iip\0"
   "glUniform3iv\0"
   "glUniform3ivARB\0"
   "\0"
   /* _mesa_function_pool[17906]: Lightfv (offset 160) */
   "iip\0"
   "glLightfv\0"
   "\0"
   /* _mesa_function_pool[17921]: PrimitiveRestartIndexNV (will be remapped) */
   "i\0"
   "glPrimitiveRestartIndexNV\0"
   "glPrimitiveRestartIndex\0"
   "\0"
   /* _mesa_function_pool[17974]: ClearDepth (offset 208) */
   "d\0"
   "glClearDepth\0"
   "\0"
   /* _mesa_function_pool[17990]: GetFenceivNV (will be remapped) */
   "iip\0"
   "glGetFenceivNV\0"
   "\0"
   /* _mesa_function_pool[18010]: WindowPos4dvMESA (will be remapped) */
   "p\0"
   "glWindowPos4dvMESA\0"
   "\0"
   /* _mesa_function_pool[18032]: ColorSubTable (offset 346) */
   "iiiiip\0"
   "glColorSubTable\0"
   "glColorSubTableEXT\0"
   "\0"
   /* _mesa_function_pool[18075]: Color4fv (offset 30) */
   "p\0"
   "glColor4fv\0"
   "\0"
   /* _mesa_function_pool[18089]: MultiTexCoord4ivARB (offset 405) */
   "ip\0"
   "glMultiTexCoord4iv\0"
   "glMultiTexCoord4ivARB\0"
   "\0"
   /* _mesa_function_pool[18134]: GetnMinmaxARB (will be remapped) */
   "iiiiip\0"
   "glGetnMinmaxARB\0"
   "\0"
   /* _mesa_function_pool[18158]: ProgramLocalParameters4fvEXT (will be remapped) */
   "iiip\0"
   "glProgramLocalParameters4fvEXT\0"
   "\0"
   /* _mesa_function_pool[18195]: ColorPointer (offset 308) */
   "iiip\0"
   "glColorPointer\0"
   "\0"
   /* _mesa_function_pool[18216]: Rects (offset 92) */
   "iiii\0"
   "glRects\0"
   "\0"
   /* _mesa_function_pool[18230]: GetMapAttribParameterfvNV (dynamic) */
   "iiip\0"
   "glGetMapAttribParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[18264]: CreateShaderProgramEXT (will be remapped) */
   "ip\0"
   "glCreateShaderProgramEXT\0"
   "\0"
   /* _mesa_function_pool[18293]: ActiveProgramEXT (will be remapped) */
   "i\0"
   "glActiveProgramEXT\0"
   "\0"
   /* _mesa_function_pool[18315]: Lightiv (offset 162) */
   "iip\0"
   "glLightiv\0"
   "\0"
   /* _mesa_function_pool[18330]: VertexAttrib4sARB (will be remapped) */
   "iiiii\0"
   "glVertexAttrib4s\0"
   "glVertexAttrib4sARB\0"
   "\0"
   /* _mesa_function_pool[18374]: GetQueryObjectuivARB (will be remapped) */
   "iip\0"
   "glGetQueryObjectuiv\0"
   "glGetQueryObjectuivARB\0"
   "\0"
   /* _mesa_function_pool[18422]: GetTexParameteriv (offset 283) */
   "iip\0"
   "glGetTexParameteriv\0"
   "\0"
   /* _mesa_function_pool[18447]: MapParameterivNV (dynamic) */
   "iip\0"
   "glMapParameterivNV\0"
   "\0"
   /* _mesa_function_pool[18471]: GenRenderbuffersEXT (will be remapped) */
   "ip\0"
   "glGenRenderbuffers\0"
   "glGenRenderbuffersEXT\0"
   "\0"
   /* _mesa_function_pool[18516]: ClearBufferfv (will be remapped) */
   "iip\0"
   "glClearBufferfv\0"
   "\0"
   /* _mesa_function_pool[18537]: VertexAttrib2dvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib2dv\0"
   "glVertexAttrib2dvARB\0"
   "\0"
   /* _mesa_function_pool[18580]: EdgeFlagPointerEXT (will be remapped) */
   "iip\0"
   "glEdgeFlagPointerEXT\0"
   "\0"
   /* _mesa_function_pool[18606]: VertexAttribs2svNV (will be remapped) */
   "iip\0"
   "glVertexAttribs2svNV\0"
   "\0"
   /* _mesa_function_pool[18632]: WeightbvARB (dynamic) */
   "ip\0"
   "glWeightbvARB\0"
   "\0"
   /* _mesa_function_pool[18650]: VertexAttrib2fvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib2fv\0"
   "glVertexAttrib2fvARB\0"
   "\0"
   /* _mesa_function_pool[18693]: GetBufferParameterivARB (will be remapped) */
   "iip\0"
   "glGetBufferParameteriv\0"
   "glGetBufferParameterivARB\0"
   "\0"
   /* _mesa_function_pool[18747]: Rectdv (offset 87) */
   "pp\0"
   "glRectdv\0"
   "\0"
   /* _mesa_function_pool[18760]: ListParameteriSGIX (dynamic) */
   "iii\0"
   "glListParameteriSGIX\0"
   "\0"
   /* _mesa_function_pool[18786]: BlendEquationiARB (will be remapped) */
   "ii\0"
   "glBlendEquationiARB\0"
   "glBlendEquationIndexedAMD\0"
   "\0"
   /* _mesa_function_pool[18836]: ReplacementCodeuiColor4fNormal3fVertex3fSUN (dynamic) */
   "iffffffffff\0"
   "glReplacementCodeuiColor4fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[18895]: InstrumentsBufferSGIX (dynamic) */
   "ip\0"
   "glInstrumentsBufferSGIX\0"
   "\0"
   /* _mesa_function_pool[18923]: VertexAttrib4NivARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Niv\0"
   "glVertexAttrib4NivARB\0"
   "\0"
   /* _mesa_function_pool[18968]: DrawArraysInstancedARB (will be remapped) */
   "iiii\0"
   "glDrawArraysInstancedARB\0"
   "glDrawArraysInstancedEXT\0"
   "glDrawArraysInstanced\0"
   "\0"
   /* _mesa_function_pool[19046]: GetAttachedShaders (will be remapped) */
   "iipp\0"
   "glGetAttachedShaders\0"
   "\0"
   /* _mesa_function_pool[19073]: GenVertexArraysAPPLE (will be remapped) */
   "ip\0"
   "glGenVertexArraysAPPLE\0"
   "\0"
   /* _mesa_function_pool[19100]: ClearBufferfi (will be remapped) */
   "iifi\0"
   "glClearBufferfi\0"
   "\0"
   /* _mesa_function_pool[19122]: Materialiv (offset 172) */
   "iip\0"
   "glMaterialiv\0"
   "\0"
   /* _mesa_function_pool[19140]: PushClientAttrib (offset 335) */
   "i\0"
   "glPushClientAttrib\0"
   "\0"
   /* _mesa_function_pool[19162]: SamplerParameteriv (will be remapped) */
   "iip\0"
   "glSamplerParameteriv\0"
   "\0"
   /* _mesa_function_pool[19188]: TexCoord2fColor4fNormal3fVertex3fvSUN (dynamic) */
   "pppp\0"
   "glTexCoord2fColor4fNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[19234]: WindowPos2iMESA (will be remapped) */
   "ii\0"
   "glWindowPos2i\0"
   "glWindowPos2iARB\0"
   "glWindowPos2iMESA\0"
   "\0"
   /* _mesa_function_pool[19287]: SampleMaskSGIS (will be remapped) */
   "fi\0"
   "glSampleMaskSGIS\0"
   "glSampleMaskEXT\0"
   "\0"
   /* _mesa_function_pool[19324]: SecondaryColor3fvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3fv\0"
   "glSecondaryColor3fvEXT\0"
   "\0"
   /* _mesa_function_pool[19370]: PolygonMode (offset 174) */
   "ii\0"
   "glPolygonMode\0"
   "\0"
   /* _mesa_function_pool[19388]: CompressedTexSubImage1DARB (will be remapped) */
   "iiiiiip\0"
   "glCompressedTexSubImage1D\0"
   "glCompressedTexSubImage1DARB\0"
   "\0"
   /* _mesa_function_pool[19452]: VertexAttribI1iEXT (will be remapped) */
   "ii\0"
   "glVertexAttribI1iEXT\0"
   "glVertexAttribI1i\0"
   "\0"
   /* _mesa_function_pool[19495]: TexCoord2fNormal3fVertex3fSUN (dynamic) */
   "ffffffff\0"
   "glTexCoord2fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[19537]: GetVertexAttribivNV (will be remapped) */
   "iip\0"
   "glGetVertexAttribivNV\0"
   "\0"
   /* _mesa_function_pool[19564]: GetProgramStringARB (will be remapped) */
   "iip\0"
   "glGetProgramStringARB\0"
   "\0"
   /* _mesa_function_pool[19591]: GetnUniformdvARB (will be remapped) */
   "iiip\0"
   "glGetnUniformdvARB\0"
   "\0"
   /* _mesa_function_pool[19616]: DrawElementsInstancedBaseVertex (will be remapped) */
   "iiipii\0"
   "glDrawElementsInstancedBaseVertex\0"
   "\0"
   /* _mesa_function_pool[19658]: VertexAttribIPointerEXT (will be remapped) */
   "iiiip\0"
   "glVertexAttribIPointerEXT\0"
   "glVertexAttribIPointer\0"
   "\0"
   /* _mesa_function_pool[19714]: TexBumpParameterfvATI (will be remapped) */
   "ip\0"
   "glTexBumpParameterfvATI\0"
   "\0"
   /* _mesa_function_pool[19742]: Tangent3ivEXT (dynamic) */
   "p\0"
   "glTangent3ivEXT\0"
   "\0"
   /* _mesa_function_pool[19761]: CompileShaderARB (will be remapped) */
   "i\0"
   "glCompileShader\0"
   "glCompileShaderARB\0"
   "\0"
   /* _mesa_function_pool[19799]: DeleteShader (will be remapped) */
   "i\0"
   "glDeleteShader\0"
   "\0"
   /* _mesa_function_pool[19817]: DisableClientState (offset 309) */
   "i\0"
   "glDisableClientState\0"
   "\0"
   /* _mesa_function_pool[19841]: TexGeni (offset 192) */
   "iii\0"
   "glTexGeni\0"
   "\0"
   /* _mesa_function_pool[19856]: TexGenf (offset 190) */
   "iif\0"
   "glTexGenf\0"
   "\0"
   /* _mesa_function_pool[19871]: Uniform3fARB (will be remapped) */
   "ifff\0"
   "glUniform3f\0"
   "glUniform3fARB\0"
   "\0"
   /* _mesa_function_pool[19904]: TexGend (offset 188) */
   "iid\0"
   "glTexGend\0"
   "\0"
   /* _mesa_function_pool[19919]: ListParameterfvSGIX (dynamic) */
   "iip\0"
   "glListParameterfvSGIX\0"
   "\0"
   /* _mesa_function_pool[19946]: GetPolygonStipple (offset 274) */
   "p\0"
   "glGetPolygonStipple\0"
   "\0"
   /* _mesa_function_pool[19969]: ColorP3ui (will be remapped) */
   "ii\0"
   "glColorP3ui\0"
   "\0"
   /* _mesa_function_pool[19985]: Tangent3dvEXT (dynamic) */
   "p\0"
   "glTangent3dvEXT\0"
   "\0"
   /* _mesa_function_pool[20004]: BindBufferOffsetEXT (will be remapped) */
   "iiii\0"
   "glBindBufferOffsetEXT\0"
   "\0"
   /* _mesa_function_pool[20032]: WindowPos3sMESA (will be remapped) */
   "iii\0"
   "glWindowPos3s\0"
   "glWindowPos3sARB\0"
   "glWindowPos3sMESA\0"
   "\0"
   /* _mesa_function_pool[20086]: VertexAttrib2svNV (will be remapped) */
   "ip\0"
   "glVertexAttrib2svNV\0"
   "\0"
   /* _mesa_function_pool[20110]: DisableIndexedEXT (will be remapped) */
   "ii\0"
   "glDisableIndexedEXT\0"
   "glDisablei\0"
   "\0"
   /* _mesa_function_pool[20145]: NormalP3uiv (will be remapped) */
   "ip\0"
   "glNormalP3uiv\0"
   "\0"
   /* _mesa_function_pool[20163]: SecondaryColorP3uiv (will be remapped) */
   "ip\0"
   "glSecondaryColorP3uiv\0"
   "\0"
   /* _mesa_function_pool[20189]: BindBufferBaseEXT (will be remapped) */
   "iii\0"
   "glBindBufferBaseEXT\0"
   "glBindBufferBase\0"
   "\0"
   /* _mesa_function_pool[20231]: TexCoord2fVertex3fvSUN (dynamic) */
   "pp\0"
   "glTexCoord2fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[20260]: WindowPos4sMESA (will be remapped) */
   "iiii\0"
   "glWindowPos4sMESA\0"
   "\0"
   /* _mesa_function_pool[20284]: GetnPixelMapuivARB (will be remapped) */
   "iip\0"
   "glGetnPixelMapuivARB\0"
   "\0"
   /* _mesa_function_pool[20310]: VertexAttrib4NuivARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Nuiv\0"
   "glVertexAttrib4NuivARB\0"
   "\0"
   /* _mesa_function_pool[20357]: ClientActiveTextureARB (offset 375) */
   "i\0"
   "glClientActiveTexture\0"
   "glClientActiveTextureARB\0"
   "\0"
   /* _mesa_function_pool[20407]: GetSamplerParameterIuiv (will be remapped) */
   "iip\0"
   "glGetSamplerParameterIuiv\0"
   "\0"
   /* _mesa_function_pool[20438]: ReplacementCodeusvSUN (dynamic) */
   "p\0"
   "glReplacementCodeusvSUN\0"
   "\0"
   /* _mesa_function_pool[20465]: Uniform4fARB (will be remapped) */
   "iffff\0"
   "glUniform4f\0"
   "glUniform4fARB\0"
   "\0"
   /* _mesa_function_pool[20499]: Color4sv (offset 34) */
   "p\0"
   "glColor4sv\0"
   "\0"
   /* _mesa_function_pool[20513]: FlushMappedBufferRange (will be remapped) */
   "iii\0"
   "glFlushMappedBufferRange\0"
   "\0"
   /* _mesa_function_pool[20543]: IsProgramNV (will be remapped) */
   "i\0"
   "glIsProgramARB\0"
   "glIsProgramNV\0"
   "\0"
   /* _mesa_function_pool[20575]: FlushMappedBufferRangeAPPLE (will be remapped) */
   "iii\0"
   "glFlushMappedBufferRangeAPPLE\0"
   "\0"
   /* _mesa_function_pool[20610]: PixelZoom (offset 246) */
   "ff\0"
   "glPixelZoom\0"
   "\0"
   /* _mesa_function_pool[20626]: ReplacementCodePointerSUN (dynamic) */
   "iip\0"
   "glReplacementCodePointerSUN\0"
   "\0"
   /* _mesa_function_pool[20659]: ProgramEnvParameter4dARB (will be remapped) */
   "iidddd\0"
   "glProgramEnvParameter4dARB\0"
   "glProgramParameter4dNV\0"
   "\0"
   /* _mesa_function_pool[20717]: ColorTableParameterfv (offset 340) */
   "iip\0"
   "glColorTableParameterfv\0"
   "glColorTableParameterfvSGI\0"
   "\0"
   /* _mesa_function_pool[20773]: FragmentLightModelfSGIX (dynamic) */
   "if\0"
   "glFragmentLightModelfSGIX\0"
   "\0"
   /* _mesa_function_pool[20803]: Binormal3bvEXT (dynamic) */
   "p\0"
   "glBinormal3bvEXT\0"
   "\0"
   /* _mesa_function_pool[20823]: PixelMapuiv (offset 252) */
   "iip\0"
   "glPixelMapuiv\0"
   "\0"
   /* _mesa_function_pool[20842]: Color3dv (offset 12) */
   "p\0"
   "glColor3dv\0"
   "\0"
   /* _mesa_function_pool[20856]: IsTexture (offset 330) */
   "i\0"
   "glIsTexture\0"
   "glIsTextureEXT\0"
   "\0"
   /* _mesa_function_pool[20886]: GenSamplers (will be remapped) */
   "ip\0"
   "glGenSamplers\0"
   "\0"
   /* _mesa_function_pool[20904]: VertexAttribP3uiv (will be remapped) */
   "iiip\0"
   "glVertexAttribP3uiv\0"
   "\0"
   /* _mesa_function_pool[20930]: VertexWeightfvEXT (dynamic) */
   "p\0"
   "glVertexWeightfvEXT\0"
   "\0"
   /* _mesa_function_pool[20953]: VertexAttrib1dARB (will be remapped) */
   "id\0"
   "glVertexAttrib1d\0"
   "glVertexAttrib1dARB\0"
   "\0"
   /* _mesa_function_pool[20994]: ImageTransformParameterivHP (dynamic) */
   "iip\0"
   "glImageTransformParameterivHP\0"
   "\0"
   /* _mesa_function_pool[21029]: TexCoord4i (offset 122) */
   "iiii\0"
   "glTexCoord4i\0"
   "\0"
   /* _mesa_function_pool[21048]: DeleteQueriesARB (will be remapped) */
   "ip\0"
   "glDeleteQueries\0"
   "glDeleteQueriesARB\0"
   "\0"
   /* _mesa_function_pool[21087]: Color4ubVertex2fSUN (dynamic) */
   "iiiiff\0"
   "glColor4ubVertex2fSUN\0"
   "\0"
   /* _mesa_function_pool[21117]: FragmentColorMaterialSGIX (dynamic) */
   "ii\0"
   "glFragmentColorMaterialSGIX\0"
   "\0"
   /* _mesa_function_pool[21149]: CurrentPaletteMatrixARB (dynamic) */
   "i\0"
   "glCurrentPaletteMatrixARB\0"
   "\0"
   /* _mesa_function_pool[21178]: GetMapdv (offset 266) */
   "iip\0"
   "glGetMapdv\0"
   "\0"
   /* _mesa_function_pool[21194]: ObjectPurgeableAPPLE (will be remapped) */
   "iii\0"
   "glObjectPurgeableAPPLE\0"
   "\0"
   /* _mesa_function_pool[21222]: GetStringi (will be remapped) */
   "ii\0"
   "glGetStringi\0"
   "\0"
   /* _mesa_function_pool[21239]: SamplePatternSGIS (will be remapped) */
   "i\0"
   "glSamplePatternSGIS\0"
   "glSamplePatternEXT\0"
   "\0"
   /* _mesa_function_pool[21281]: PixelStoref (offset 249) */
   "if\0"
   "glPixelStoref\0"
   "\0"
   /* _mesa_function_pool[21299]: IsQueryARB (will be remapped) */
   "i\0"
   "glIsQuery\0"
   "glIsQueryARB\0"
   "\0"
   /* _mesa_function_pool[21325]: ReplacementCodeuiColor4ubVertex3fSUN (dynamic) */
   "iiiiifff\0"
   "glReplacementCodeuiColor4ubVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[21374]: PixelStorei (offset 250) */
   "ii\0"
   "glPixelStorei\0"
   "\0"
   /* _mesa_function_pool[21392]: VertexAttrib4usvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4usv\0"
   "glVertexAttrib4usvARB\0"
   "\0"
   /* _mesa_function_pool[21437]: LinkProgramARB (will be remapped) */
   "i\0"
   "glLinkProgram\0"
   "glLinkProgramARB\0"
   "\0"
   /* _mesa_function_pool[21471]: VertexAttrib2fNV (will be remapped) */
   "iff\0"
   "glVertexAttrib2fNV\0"
   "\0"
   /* _mesa_function_pool[21495]: ShaderSourceARB (will be remapped) */
   "iipp\0"
   "glShaderSource\0"
   "glShaderSourceARB\0"
   "\0"
   /* _mesa_function_pool[21534]: FragmentMaterialiSGIX (dynamic) */
   "iii\0"
   "glFragmentMaterialiSGIX\0"
   "\0"
   /* _mesa_function_pool[21563]: EvalCoord2dv (offset 233) */
   "p\0"
   "glEvalCoord2dv\0"
   "\0"
   /* _mesa_function_pool[21581]: VertexAttrib3svARB (will be remapped) */
   "ip\0"
   "glVertexAttrib3sv\0"
   "glVertexAttrib3svARB\0"
   "\0"
   /* _mesa_function_pool[21624]: ColorMaterial (offset 151) */
   "ii\0"
   "glColorMaterial\0"
   "\0"
   /* _mesa_function_pool[21644]: CompressedTexSubImage3DARB (will be remapped) */
   "iiiiiiiiiip\0"
   "glCompressedTexSubImage3D\0"
   "glCompressedTexSubImage3DARB\0"
   "\0"
   /* _mesa_function_pool[21712]: WindowPos2ivMESA (will be remapped) */
   "p\0"
   "glWindowPos2iv\0"
   "glWindowPos2ivARB\0"
   "glWindowPos2ivMESA\0"
   "\0"
   /* _mesa_function_pool[21767]: IsFramebufferEXT (will be remapped) */
   "i\0"
   "glIsFramebuffer\0"
   "glIsFramebufferEXT\0"
   "\0"
   /* _mesa_function_pool[21805]: Uniform4ivARB (will be remapped) */
   "iip\0"
   "glUniform4iv\0"
   "glUniform4ivARB\0"
   "\0"
   /* _mesa_function_pool[21839]: GetVertexAttribdvARB (will be remapped) */
   "iip\0"
   "glGetVertexAttribdv\0"
   "glGetVertexAttribdvARB\0"
   "\0"
   /* _mesa_function_pool[21887]: TexBumpParameterivATI (will be remapped) */
   "ip\0"
   "glTexBumpParameterivATI\0"
   "\0"
   /* _mesa_function_pool[21915]: GetSeparableFilter (offset 359) */
   "iiippp\0"
   "glGetSeparableFilter\0"
   "glGetSeparableFilterEXT\0"
   "\0"
   /* _mesa_function_pool[21968]: Binormal3dEXT (dynamic) */
   "ddd\0"
   "glBinormal3dEXT\0"
   "\0"
   /* _mesa_function_pool[21989]: SpriteParameteriSGIX (dynamic) */
   "ii\0"
   "glSpriteParameteriSGIX\0"
   "\0"
   /* _mesa_function_pool[22016]: RequestResidentProgramsNV (will be remapped) */
   "ip\0"
   "glRequestResidentProgramsNV\0"
   "\0"
   /* _mesa_function_pool[22048]: TagSampleBufferSGIX (dynamic) */
   "\0"
   "glTagSampleBufferSGIX\0"
   "\0"
   /* _mesa_function_pool[22072]: TransformFeedbackVaryingsEXT (will be remapped) */
   "iipi\0"
   "glTransformFeedbackVaryingsEXT\0"
   "glTransformFeedbackVaryings\0"
   "\0"
   /* _mesa_function_pool[22137]: FeedbackBuffer (offset 194) */
   "iip\0"
   "glFeedbackBuffer\0"
   "\0"
   /* _mesa_function_pool[22159]: RasterPos2iv (offset 67) */
   "p\0"
   "glRasterPos2iv\0"
   "\0"
   /* _mesa_function_pool[22177]: TexImage1D (offset 182) */
   "iiiiiiip\0"
   "glTexImage1D\0"
   "\0"
   /* _mesa_function_pool[22200]: ListParameterivSGIX (dynamic) */
   "iip\0"
   "glListParameterivSGIX\0"
   "\0"
   /* _mesa_function_pool[22227]: MultiDrawElementsEXT (will be remapped) */
   "ipipi\0"
   "glMultiDrawElements\0"
   "glMultiDrawElementsEXT\0"
   "\0"
   /* _mesa_function_pool[22277]: Color3s (offset 17) */
   "iii\0"
   "glColor3s\0"
   "\0"
   /* _mesa_function_pool[22292]: Uniform1ivARB (will be remapped) */
   "iip\0"
   "glUniform1iv\0"
   "glUniform1ivARB\0"
   "\0"
   /* _mesa_function_pool[22326]: WindowPos2sMESA (will be remapped) */
   "ii\0"
   "glWindowPos2s\0"
   "glWindowPos2sARB\0"
   "glWindowPos2sMESA\0"
   "\0"
   /* _mesa_function_pool[22379]: WeightusvARB (dynamic) */
   "ip\0"
   "glWeightusvARB\0"
   "\0"
   /* _mesa_function_pool[22398]: ColorP4ui (will be remapped) */
   "ii\0"
   "glColorP4ui\0"
   "\0"
   /* _mesa_function_pool[22414]: TexCoordPointer (offset 320) */
   "iiip\0"
   "glTexCoordPointer\0"
   "\0"
   /* _mesa_function_pool[22438]: FogCoordPointerEXT (will be remapped) */
   "iip\0"
   "glFogCoordPointer\0"
   "glFogCoordPointerEXT\0"
   "\0"
   /* _mesa_function_pool[22482]: GetnSeparableFilterARB (will be remapped) */
   "iiiipipp\0"
   "glGetnSeparableFilterARB\0"
   "\0"
   /* _mesa_function_pool[22517]: IndexMaterialEXT (dynamic) */
   "ii\0"
   "glIndexMaterialEXT\0"
   "\0"
   /* _mesa_function_pool[22540]: Color3i (offset 15) */
   "iii\0"
   "glColor3i\0"
   "\0"
   /* _mesa_function_pool[22555]: FrontFace (offset 157) */
   "i\0"
   "glFrontFace\0"
   "\0"
   /* _mesa_function_pool[22570]: EvalCoord2d (offset 232) */
   "dd\0"
   "glEvalCoord2d\0"
   "\0"
   /* _mesa_function_pool[22588]: SecondaryColor3ubvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3ubv\0"
   "glSecondaryColor3ubvEXT\0"
   "\0"
   /* _mesa_function_pool[22636]: EvalCoord2f (offset 234) */
   "ff\0"
   "glEvalCoord2f\0"
   "\0"
   /* _mesa_function_pool[22654]: VertexAttrib4dvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4dv\0"
   "glVertexAttrib4dvARB\0"
   "\0"
   /* _mesa_function_pool[22697]: BindAttribLocationARB (will be remapped) */
   "iip\0"
   "glBindAttribLocation\0"
   "glBindAttribLocationARB\0"
   "\0"
   /* _mesa_function_pool[22747]: Color3b (offset 9) */
   "iii\0"
   "glColor3b\0"
   "\0"
   /* _mesa_function_pool[22762]: MultiTexCoord2dARB (offset 384) */
   "idd\0"
   "glMultiTexCoord2d\0"
   "glMultiTexCoord2dARB\0"
   "\0"
   /* _mesa_function_pool[22806]: ExecuteProgramNV (will be remapped) */
   "iip\0"
   "glExecuteProgramNV\0"
   "\0"
   /* _mesa_function_pool[22830]: Color3f (offset 13) */
   "fff\0"
   "glColor3f\0"
   "\0"
   /* _mesa_function_pool[22845]: LightEnviSGIX (dynamic) */
   "ii\0"
   "glLightEnviSGIX\0"
   "\0"
   /* _mesa_function_pool[22865]: Color3d (offset 11) */
   "ddd\0"
   "glColor3d\0"
   "\0"
   /* _mesa_function_pool[22880]: Normal3dv (offset 55) */
   "p\0"
   "glNormal3dv\0"
   "\0"
   /* _mesa_function_pool[22895]: Lightf (offset 159) */
   "iif\0"
   "glLightf\0"
   "\0"
   /* _mesa_function_pool[22909]: ReplacementCodeuiSUN (dynamic) */
   "i\0"
   "glReplacementCodeuiSUN\0"
   "\0"
   /* _mesa_function_pool[22935]: MatrixMode (offset 293) */
   "i\0"
   "glMatrixMode\0"
   "\0"
   /* _mesa_function_pool[22951]: GetPixelMapusv (offset 273) */
   "ip\0"
   "glGetPixelMapusv\0"
   "\0"
   /* _mesa_function_pool[22972]: Lighti (offset 161) */
   "iii\0"
   "glLighti\0"
   "\0"
   /* _mesa_function_pool[22986]: VertexAttribPointerNV (will be remapped) */
   "iiiip\0"
   "glVertexAttribPointerNV\0"
   "\0"
   /* _mesa_function_pool[23017]: ClearDepthf (will be remapped) */
   "f\0"
   "glClearDepthf\0"
   "\0"
   /* _mesa_function_pool[23034]: GetBooleanIndexedvEXT (will be remapped) */
   "iip\0"
   "glGetBooleanIndexedvEXT\0"
   "glGetBooleani_v\0"
   "\0"
   /* _mesa_function_pool[23079]: GetFramebufferAttachmentParameterivEXT (will be remapped) */
   "iiip\0"
   "glGetFramebufferAttachmentParameteriv\0"
   "glGetFramebufferAttachmentParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[23164]: PixelTransformParameterfEXT (dynamic) */
   "iif\0"
   "glPixelTransformParameterfEXT\0"
   "\0"
   /* _mesa_function_pool[23199]: MultiTexCoord4dvARB (offset 401) */
   "ip\0"
   "glMultiTexCoord4dv\0"
   "glMultiTexCoord4dvARB\0"
   "\0"
   /* _mesa_function_pool[23244]: PixelTransformParameteriEXT (dynamic) */
   "iii\0"
   "glPixelTransformParameteriEXT\0"
   "\0"
   /* _mesa_function_pool[23279]: GetDoublev (offset 260) */
   "ip\0"
   "glGetDoublev\0"
   "\0"
   /* _mesa_function_pool[23296]: MultiTexCoordP4ui (will be remapped) */
   "iii\0"
   "glMultiTexCoordP4ui\0"
   "\0"
   /* _mesa_function_pool[23321]: TexCoordP2uiv (will be remapped) */
   "ip\0"
   "glTexCoordP2uiv\0"
   "\0"
   /* _mesa_function_pool[23341]: MultMatrixd (offset 295) */
   "p\0"
   "glMultMatrixd\0"
   "\0"
   /* _mesa_function_pool[23358]: MultMatrixf (offset 294) */
   "p\0"
   "glMultMatrixf\0"
   "\0"
   /* _mesa_function_pool[23375]: VertexAttribI4bvEXT (will be remapped) */
   "ip\0"
   "glVertexAttribI4bvEXT\0"
   "glVertexAttribI4bv\0"
   "\0"
   /* _mesa_function_pool[23420]: TexCoord2fColor4ubVertex3fSUN (dynamic) */
   "ffiiiifff\0"
   "glTexCoord2fColor4ubVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[23463]: Uniform1iARB (will be remapped) */
   "ii\0"
   "glUniform1i\0"
   "glUniform1iARB\0"
   "\0"
   /* _mesa_function_pool[23494]: GetnMapfvARB (will be remapped) */
   "iiip\0"
   "glGetnMapfvARB\0"
   "\0"
   /* _mesa_function_pool[23515]: VertexAttribPointerARB (will be remapped) */
   "iiiiip\0"
   "glVertexAttribPointer\0"
   "glVertexAttribPointerARB\0"
   "\0"
   /* _mesa_function_pool[23570]: VertexAttrib3sNV (will be remapped) */
   "iiii\0"
   "glVertexAttrib3sNV\0"
   "\0"
   /* _mesa_function_pool[23595]: SharpenTexFuncSGIS (dynamic) */
   "iip\0"
   "glSharpenTexFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[23621]: MultiTexCoord4fvARB (offset 403) */
   "ip\0"
   "glMultiTexCoord4fv\0"
   "glMultiTexCoord4fvARB\0"
   "\0"
   /* _mesa_function_pool[23666]: Uniform2uiEXT (will be remapped) */
   "iii\0"
   "glUniform2uiEXT\0"
   "glUniform2ui\0"
   "\0"
   /* _mesa_function_pool[23700]: TexCoordP3uiv (will be remapped) */
   "ip\0"
   "glTexCoordP3uiv\0"
   "\0"
   /* _mesa_function_pool[23720]: UniformMatrix2x3fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix2x3fv\0"
   "\0"
   /* _mesa_function_pool[23747]: SamplerParameteri (will be remapped) */
   "iii\0"
   "glSamplerParameteri\0"
   "\0"
   /* _mesa_function_pool[23772]: SamplerParameterf (will be remapped) */
   "iif\0"
   "glSamplerParameterf\0"
   "\0"
   /* _mesa_function_pool[23797]: CombinerParameteriNV (will be remapped) */
   "ii\0"
   "glCombinerParameteriNV\0"
   "\0"
   /* _mesa_function_pool[23824]: DeleteAsyncMarkersSGIX (dynamic) */
   "ii\0"
   "glDeleteAsyncMarkersSGIX\0"
   "\0"
   /* _mesa_function_pool[23853]: ReplacementCodeusSUN (dynamic) */
   "i\0"
   "glReplacementCodeusSUN\0"
   "\0"
   /* _mesa_function_pool[23879]: IsAsyncMarkerSGIX (dynamic) */
   "i\0"
   "glIsAsyncMarkerSGIX\0"
   "\0"
   /* _mesa_function_pool[23902]: FrameZoomSGIX (dynamic) */
   "i\0"
   "glFrameZoomSGIX\0"
   "\0"
   /* _mesa_function_pool[23921]: Normal3fVertex3fvSUN (dynamic) */
   "pp\0"
   "glNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[23948]: GetnUniformuivARB (will be remapped) */
   "iiip\0"
   "glGetnUniformuivARB\0"
   "\0"
   /* _mesa_function_pool[23974]: RasterPos4sv (offset 85) */
   "p\0"
   "glRasterPos4sv\0"
   "\0"
   /* _mesa_function_pool[23992]: VertexAttrib4NsvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Nsv\0"
   "glVertexAttrib4NsvARB\0"
   "\0"
   /* _mesa_function_pool[24037]: VertexAttrib3fvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib3fv\0"
   "glVertexAttrib3fvARB\0"
   "\0"
   /* _mesa_function_pool[24080]: ClearColor (offset 206) */
   "ffff\0"
   "glClearColor\0"
   "\0"
   /* _mesa_function_pool[24099]: GetSynciv (will be remapped) */
   "iiipp\0"
   "glGetSynciv\0"
   "\0"
   /* _mesa_function_pool[24118]: ClearColorIiEXT (will be remapped) */
   "iiii\0"
   "glClearColorIiEXT\0"
   "\0"
   /* _mesa_function_pool[24142]: DeleteFramebuffersEXT (will be remapped) */
   "ip\0"
   "glDeleteFramebuffers\0"
   "glDeleteFramebuffersEXT\0"
   "\0"
   /* _mesa_function_pool[24191]: GlobalAlphaFactorsSUN (dynamic) */
   "i\0"
   "glGlobalAlphaFactorsSUN\0"
   "\0"
   /* _mesa_function_pool[24218]: IsEnabledIndexedEXT (will be remapped) */
   "ii\0"
   "glIsEnabledIndexedEXT\0"
   "glIsEnabledi\0"
   "\0"
   /* _mesa_function_pool[24257]: TexEnviv (offset 187) */
   "iip\0"
   "glTexEnviv\0"
   "\0"
   /* _mesa_function_pool[24273]: TexSubImage3D (offset 372) */
   "iiiiiiiiiip\0"
   "glTexSubImage3D\0"
   "glTexSubImage3DEXT\0"
   "\0"
   /* _mesa_function_pool[24321]: Tangent3fEXT (dynamic) */
   "fff\0"
   "glTangent3fEXT\0"
   "\0"
   /* _mesa_function_pool[24341]: SecondaryColor3uivEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3uiv\0"
   "glSecondaryColor3uivEXT\0"
   "\0"
   /* _mesa_function_pool[24389]: MatrixIndexubvARB (dynamic) */
   "ip\0"
   "glMatrixIndexubvARB\0"
   "\0"
   /* _mesa_function_pool[24413]: Color4fNormal3fVertex3fSUN (dynamic) */
   "ffffffffff\0"
   "glColor4fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[24454]: PixelTexGenParameterfSGIS (will be remapped) */
   "if\0"
   "glPixelTexGenParameterfSGIS\0"
   "\0"
   /* _mesa_function_pool[24486]: CreateShader (will be remapped) */
   "i\0"
   "glCreateShader\0"
   "\0"
   /* _mesa_function_pool[24504]: GetColorTableParameterfv (offset 344) */
   "iip\0"
   "glGetColorTableParameterfv\0"
   "glGetColorTableParameterfvSGI\0"
   "glGetColorTableParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[24596]: FragmentLightModelfvSGIX (dynamic) */
   "ip\0"
   "glFragmentLightModelfvSGIX\0"
   "\0"
   /* _mesa_function_pool[24627]: Bitmap (offset 8) */
   "iiffffp\0"
   "glBitmap\0"
   "\0"
   /* _mesa_function_pool[24645]: MultiTexCoord3fARB (offset 394) */
   "ifff\0"
   "glMultiTexCoord3f\0"
   "glMultiTexCoord3fARB\0"
   "\0"
   /* _mesa_function_pool[24690]: GetTexLevelParameterfv (offset 284) */
   "iiip\0"
   "glGetTexLevelParameterfv\0"
   "\0"
   /* _mesa_function_pool[24721]: GetPixelTexGenParameterfvSGIS (will be remapped) */
   "ip\0"
   "glGetPixelTexGenParameterfvSGIS\0"
   "\0"
   /* _mesa_function_pool[24757]: GenFramebuffersEXT (will be remapped) */
   "ip\0"
   "glGenFramebuffers\0"
   "glGenFramebuffersEXT\0"
   "\0"
   /* _mesa_function_pool[24800]: VertexAttribDivisor (will be remapped) */
   "ii\0"
   "glVertexAttribDivisor\0"
   "\0"
   /* _mesa_function_pool[24826]: GetProgramParameterdvNV (will be remapped) */
   "iiip\0"
   "glGetProgramParameterdvNV\0"
   "\0"
   /* _mesa_function_pool[24858]: Vertex2sv (offset 133) */
   "p\0"
   "glVertex2sv\0"
   "\0"
   /* _mesa_function_pool[24873]: GetIntegerv (offset 263) */
   "ip\0"
   "glGetIntegerv\0"
   "\0"
   /* _mesa_function_pool[24891]: IsVertexArrayAPPLE (will be remapped) */
   "i\0"
   "glIsVertexArray\0"
   "glIsVertexArrayAPPLE\0"
   "\0"
   /* _mesa_function_pool[24931]: FragmentLightfvSGIX (dynamic) */
   "iip\0"
   "glFragmentLightfvSGIX\0"
   "\0"
   /* _mesa_function_pool[24958]: GetnMapdvARB (will be remapped) */
   "iiip\0"
   "glGetnMapdvARB\0"
   "\0"
   /* _mesa_function_pool[24979]: DetachShader (will be remapped) */
   "ii\0"
   "glDetachShader\0"
   "\0"
   /* _mesa_function_pool[24998]: VertexAttrib4NubARB (will be remapped) */
   "iiiii\0"
   "glVertexAttrib4Nub\0"
   "glVertexAttrib4NubARB\0"
   "\0"
   /* _mesa_function_pool[25046]: GetProgramEnvParameterfvARB (will be remapped) */
   "iip\0"
   "glGetProgramEnvParameterfvARB\0"
   "\0"
   /* _mesa_function_pool[25081]: GetTrackMatrixivNV (will be remapped) */
   "iiip\0"
   "glGetTrackMatrixivNV\0"
   "\0"
   /* _mesa_function_pool[25108]: VertexAttrib3svNV (will be remapped) */
   "ip\0"
   "glVertexAttrib3svNV\0"
   "\0"
   /* _mesa_function_pool[25132]: Uniform4fvARB (will be remapped) */
   "iip\0"
   "glUniform4fv\0"
   "glUniform4fvARB\0"
   "\0"
   /* _mesa_function_pool[25166]: MultTransposeMatrixfARB (will be remapped) */
   "p\0"
   "glMultTransposeMatrixf\0"
   "glMultTransposeMatrixfARB\0"
   "\0"
   /* _mesa_function_pool[25218]: GetTexEnviv (offset 277) */
   "iip\0"
   "glGetTexEnviv\0"
   "\0"
   /* _mesa_function_pool[25237]: ColorFragmentOp1ATI (will be remapped) */
   "iiiiiii\0"
   "glColorFragmentOp1ATI\0"
   "\0"
   /* _mesa_function_pool[25268]: GetUniformfvARB (will be remapped) */
   "iip\0"
   "glGetUniformfv\0"
   "glGetUniformfvARB\0"
   "\0"
   /* _mesa_function_pool[25306]: EGLImageTargetRenderbufferStorageOES (will be remapped) */
   "ip\0"
   "glEGLImageTargetRenderbufferStorageOES\0"
   "\0"
   /* _mesa_function_pool[25349]: VertexAttribI2ivEXT (will be remapped) */
   "ip\0"
   "glVertexAttribI2ivEXT\0"
   "glVertexAttribI2iv\0"
   "\0"
   /* _mesa_function_pool[25394]: PopClientAttrib (offset 334) */
   "\0"
   "glPopClientAttrib\0"
   "\0"
   /* _mesa_function_pool[25414]: ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN (dynamic) */
   "iffffffffffff\0"
   "glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[25485]: DetachObjectARB (will be remapped) */
   "ii\0"
   "glDetachObjectARB\0"
   "\0"
   /* _mesa_function_pool[25507]: VertexBlendARB (dynamic) */
   "i\0"
   "glVertexBlendARB\0"
   "\0"
   /* _mesa_function_pool[25527]: WindowPos3iMESA (will be remapped) */
   "iii\0"
   "glWindowPos3i\0"
   "glWindowPos3iARB\0"
   "glWindowPos3iMESA\0"
   "\0"
   /* _mesa_function_pool[25581]: SeparableFilter2D (offset 360) */
   "iiiiiipp\0"
   "glSeparableFilter2D\0"
   "glSeparableFilter2DEXT\0"
   "\0"
   /* _mesa_function_pool[25634]: ProgramParameteriARB (will be remapped) */
   "iii\0"
   "glProgramParameteriARB\0"
   "\0"
   /* _mesa_function_pool[25662]: Map1d (offset 220) */
   "iddiip\0"
   "glMap1d\0"
   "\0"
   /* _mesa_function_pool[25678]: Map1f (offset 221) */
   "iffiip\0"
   "glMap1f\0"
   "\0"
   /* _mesa_function_pool[25694]: CompressedTexImage2DARB (will be remapped) */
   "iiiiiiip\0"
   "glCompressedTexImage2D\0"
   "glCompressedTexImage2DARB\0"
   "\0"
   /* _mesa_function_pool[25753]: ArrayElement (offset 306) */
   "i\0"
   "glArrayElement\0"
   "glArrayElementEXT\0"
   "\0"
   /* _mesa_function_pool[25789]: TexImage2D (offset 183) */
   "iiiiiiiip\0"
   "glTexImage2D\0"
   "\0"
   /* _mesa_function_pool[25813]: DepthBoundsEXT (will be remapped) */
   "dd\0"
   "glDepthBoundsEXT\0"
   "\0"
   /* _mesa_function_pool[25834]: ProgramParameters4fvNV (will be remapped) */
   "iiip\0"
   "glProgramParameters4fvNV\0"
   "\0"
   /* _mesa_function_pool[25865]: DeformationMap3fSGIX (dynamic) */
   "iffiiffiiffiip\0"
   "glDeformationMap3fSGIX\0"
   "\0"
   /* _mesa_function_pool[25904]: GetProgramivNV (will be remapped) */
   "iip\0"
   "glGetProgramivNV\0"
   "\0"
   /* _mesa_function_pool[25926]: GetFragDataLocationEXT (will be remapped) */
   "ip\0"
   "glGetFragDataLocationEXT\0"
   "glGetFragDataLocation\0"
   "\0"
   /* _mesa_function_pool[25977]: GetMinmaxParameteriv (offset 366) */
   "iip\0"
   "glGetMinmaxParameteriv\0"
   "glGetMinmaxParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[26031]: PixelTransferf (offset 247) */
   "if\0"
   "glPixelTransferf\0"
   "\0"
   /* _mesa_function_pool[26052]: CopyTexImage1D (offset 323) */
   "iiiiiii\0"
   "glCopyTexImage1D\0"
   "glCopyTexImage1DEXT\0"
   "\0"
   /* _mesa_function_pool[26098]: PushMatrix (offset 298) */
   "\0"
   "glPushMatrix\0"
   "\0"
   /* _mesa_function_pool[26113]: Fogiv (offset 156) */
   "ip\0"
   "glFogiv\0"
   "\0"
   /* _mesa_function_pool[26125]: TexCoord1dv (offset 95) */
   "p\0"
   "glTexCoord1dv\0"
   "\0"
   /* _mesa_function_pool[26142]: AlphaFragmentOp3ATI (will be remapped) */
   "iiiiiiiiiiii\0"
   "glAlphaFragmentOp3ATI\0"
   "\0"
   /* _mesa_function_pool[26178]: PixelTransferi (offset 248) */
   "ii\0"
   "glPixelTransferi\0"
   "\0"
   /* _mesa_function_pool[26199]: GetnColorTableARB (will be remapped) */
   "iiiip\0"
   "glGetnColorTableARB\0"
   "\0"
   /* _mesa_function_pool[26226]: VertexAttrib3fvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib3fvNV\0"
   "\0"
   /* _mesa_function_pool[26250]: Rotatef (offset 300) */
   "ffff\0"
   "glRotatef\0"
   "\0"
   /* _mesa_function_pool[26266]: GetFinalCombinerInputParameterivNV (will be remapped) */
   "iip\0"
   "glGetFinalCombinerInputParameterivNV\0"
   "\0"
   /* _mesa_function_pool[26308]: Vertex3i (offset 138) */
   "iii\0"
   "glVertex3i\0"
   "\0"
   /* _mesa_function_pool[26324]: SecondaryColorP3ui (will be remapped) */
   "ii\0"
   "glSecondaryColorP3ui\0"
   "\0"
   /* _mesa_function_pool[26349]: Vertex3f (offset 136) */
   "fff\0"
   "glVertex3f\0"
   "\0"
   /* _mesa_function_pool[26365]: Clear (offset 203) */
   "i\0"
   "glClear\0"
   "\0"
   /* _mesa_function_pool[26376]: Vertex3d (offset 134) */
   "ddd\0"
   "glVertex3d\0"
   "\0"
   /* _mesa_function_pool[26392]: GetMapParameterivNV (dynamic) */
   "iip\0"
   "glGetMapParameterivNV\0"
   "\0"
   /* _mesa_function_pool[26419]: Uniform4iARB (will be remapped) */
   "iiiii\0"
   "glUniform4i\0"
   "glUniform4iARB\0"
   "\0"
   /* _mesa_function_pool[26453]: ReadBuffer (offset 254) */
   "i\0"
   "glReadBuffer\0"
   "\0"
   /* _mesa_function_pool[26469]: ConvolutionParameteri (offset 352) */
   "iii\0"
   "glConvolutionParameteri\0"
   "glConvolutionParameteriEXT\0"
   "\0"
   /* _mesa_function_pool[26525]: Ortho (offset 296) */
   "dddddd\0"
   "glOrtho\0"
   "\0"
   /* _mesa_function_pool[26541]: Binormal3sEXT (dynamic) */
   "iii\0"
   "glBinormal3sEXT\0"
   "\0"
   /* _mesa_function_pool[26562]: ListBase (offset 6) */
   "i\0"
   "glListBase\0"
   "\0"
   /* _mesa_function_pool[26576]: VertexAttribI3ivEXT (will be remapped) */
   "ip\0"
   "glVertexAttribI3ivEXT\0"
   "glVertexAttribI3iv\0"
   "\0"
   /* _mesa_function_pool[26621]: MultiTexCoordP1ui (will be remapped) */
   "iii\0"
   "glMultiTexCoordP1ui\0"
   "\0"
   /* _mesa_function_pool[26646]: Vertex3s (offset 140) */
   "iii\0"
   "glVertex3s\0"
   "\0"
   /* _mesa_function_pool[26662]: ConvolutionParameterf (offset 350) */
   "iif\0"
   "glConvolutionParameterf\0"
   "glConvolutionParameterfEXT\0"
   "\0"
   /* _mesa_function_pool[26718]: GetColorTableParameteriv (offset 345) */
   "iip\0"
   "glGetColorTableParameteriv\0"
   "glGetColorTableParameterivSGI\0"
   "glGetColorTableParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[26810]: ProgramEnvParameter4dvARB (will be remapped) */
   "iip\0"
   "glProgramEnvParameter4dvARB\0"
   "glProgramParameter4dvNV\0"
   "\0"
   /* _mesa_function_pool[26867]: ShadeModel (offset 177) */
   "i\0"
   "glShadeModel\0"
   "\0"
   /* _mesa_function_pool[26883]: VertexAttribs2fvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs2fvNV\0"
   "\0"
   /* _mesa_function_pool[26909]: Rectiv (offset 91) */
   "pp\0"
   "glRectiv\0"
   "\0"
   /* _mesa_function_pool[26922]: UseProgramObjectARB (will be remapped) */
   "i\0"
   "glUseProgram\0"
   "glUseProgramObjectARB\0"
   "\0"
   /* _mesa_function_pool[26960]: GetMapParameterfvNV (dynamic) */
   "iip\0"
   "glGetMapParameterfvNV\0"
   "\0"
   /* _mesa_function_pool[26987]: EndConditionalRenderNV (will be remapped) */
   "\0"
   "glEndConditionalRenderNV\0"
   "glEndConditionalRender\0"
   "\0"
   /* _mesa_function_pool[27037]: PassTexCoordATI (will be remapped) */
   "iii\0"
   "glPassTexCoordATI\0"
   "\0"
   /* _mesa_function_pool[27060]: DeleteProgram (will be remapped) */
   "i\0"
   "glDeleteProgram\0"
   "\0"
   /* _mesa_function_pool[27079]: GetSamplerParameteriv (will be remapped) */
   "iip\0"
   "glGetSamplerParameteriv\0"
   "\0"
   /* _mesa_function_pool[27108]: Tangent3dEXT (dynamic) */
   "ddd\0"
   "glTangent3dEXT\0"
   "\0"
   /* _mesa_function_pool[27128]: SecondaryColor3dvEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3dv\0"
   "glSecondaryColor3dvEXT\0"
   "\0"
   /* _mesa_function_pool[27174]: AlphaFragmentOp2ATI (will be remapped) */
   "iiiiiiiii\0"
   "glAlphaFragmentOp2ATI\0"
   "\0"
   /* _mesa_function_pool[27207]: Vertex2fv (offset 129) */
   "p\0"
   "glVertex2fv\0"
   "\0"
   /* _mesa_function_pool[27222]: MultiDrawArraysEXT (will be remapped) */
   "ippi\0"
   "glMultiDrawArrays\0"
   "glMultiDrawArraysEXT\0"
   "\0"
   /* _mesa_function_pool[27267]: BindRenderbufferEXT (will be remapped) */
   "ii\0"
   "glBindRenderbuffer\0"
   "glBindRenderbufferEXT\0"
   "\0"
   /* _mesa_function_pool[27312]: MultiTexCoord4dARB (offset 400) */
   "idddd\0"
   "glMultiTexCoord4d\0"
   "glMultiTexCoord4dARB\0"
   "\0"
   /* _mesa_function_pool[27358]: FramebufferTextureFaceARB (will be remapped) */
   "iiiii\0"
   "glFramebufferTextureFaceARB\0"
   "\0"
   /* _mesa_function_pool[27393]: Vertex3sv (offset 141) */
   "p\0"
   "glVertex3sv\0"
   "\0"
   /* _mesa_function_pool[27408]: SecondaryColor3usEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3us\0"
   "glSecondaryColor3usEXT\0"
   "\0"
   /* _mesa_function_pool[27456]: ProgramLocalParameter4fvARB (will be remapped) */
   "iip\0"
   "glProgramLocalParameter4fvARB\0"
   "\0"
   /* _mesa_function_pool[27491]: DeleteProgramsNV (will be remapped) */
   "ip\0"
   "glDeleteProgramsARB\0"
   "glDeleteProgramsNV\0"
   "\0"
   /* _mesa_function_pool[27534]: EvalMesh1 (offset 236) */
   "iii\0"
   "glEvalMesh1\0"
   "\0"
   /* _mesa_function_pool[27551]: PauseTransformFeedback (will be remapped) */
   "\0"
   "glPauseTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[27578]: MultiTexCoord1sARB (offset 382) */
   "ii\0"
   "glMultiTexCoord1s\0"
   "glMultiTexCoord1sARB\0"
   "\0"
   /* _mesa_function_pool[27621]: ReplacementCodeuiColor3fVertex3fSUN (dynamic) */
   "iffffff\0"
   "glReplacementCodeuiColor3fVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[27668]: GetVertexAttribPointervNV (will be remapped) */
   "iip\0"
   "glGetVertexAttribPointerv\0"
   "glGetVertexAttribPointervARB\0"
   "glGetVertexAttribPointervNV\0"
   "\0"
   /* _mesa_function_pool[27756]: VertexAttribs1fvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs1fvNV\0"
   "\0"
   /* _mesa_function_pool[27782]: MultiTexCoord1dvARB (offset 377) */
   "ip\0"
   "glMultiTexCoord1dv\0"
   "glMultiTexCoord1dvARB\0"
   "\0"
   /* _mesa_function_pool[27827]: Uniform2iARB (will be remapped) */
   "iii\0"
   "glUniform2i\0"
   "glUniform2iARB\0"
   "\0"
   /* _mesa_function_pool[27859]: Vertex2iv (offset 131) */
   "p\0"
   "glVertex2iv\0"
   "\0"
   /* _mesa_function_pool[27874]: GetProgramStringNV (will be remapped) */
   "iip\0"
   "glGetProgramStringNV\0"
   "\0"
   /* _mesa_function_pool[27900]: ColorPointerEXT (will be remapped) */
   "iiiip\0"
   "glColorPointerEXT\0"
   "\0"
   /* _mesa_function_pool[27925]: LineWidth (offset 168) */
   "f\0"
   "glLineWidth\0"
   "\0"
   /* _mesa_function_pool[27940]: MapBufferARB (will be remapped) */
   "ii\0"
   "glMapBuffer\0"
   "glMapBufferARB\0"
   "\0"
   /* _mesa_function_pool[27971]: MultiDrawElementsBaseVertex (will be remapped) */
   "ipipip\0"
   "glMultiDrawElementsBaseVertex\0"
   "\0"
   /* _mesa_function_pool[28009]: TexParameterIuivEXT (will be remapped) */
   "iip\0"
   "glTexParameterIuivEXT\0"
   "glTexParameterIuiv\0"
   "\0"
   /* _mesa_function_pool[28055]: Binormal3svEXT (dynamic) */
   "p\0"
   "glBinormal3svEXT\0"
   "\0"
   /* _mesa_function_pool[28075]: ApplyTextureEXT (dynamic) */
   "i\0"
   "glApplyTextureEXT\0"
   "\0"
   /* _mesa_function_pool[28096]: GetBufferParameteri64v (will be remapped) */
   "iip\0"
   "glGetBufferParameteri64v\0"
   "\0"
   /* _mesa_function_pool[28126]: TexGendv (offset 189) */
   "iip\0"
   "glTexGendv\0"
   "\0"
   /* _mesa_function_pool[28142]: VertexAttribI3iEXT (will be remapped) */
   "iiii\0"
   "glVertexAttribI3iEXT\0"
   "glVertexAttribI3i\0"
   "\0"
   /* _mesa_function_pool[28187]: EnableIndexedEXT (will be remapped) */
   "ii\0"
   "glEnableIndexedEXT\0"
   "glEnablei\0"
   "\0"
   /* _mesa_function_pool[28220]: TextureMaterialEXT (dynamic) */
   "ii\0"
   "glTextureMaterialEXT\0"
   "\0"
   /* _mesa_function_pool[28245]: TextureLightEXT (dynamic) */
   "i\0"
   "glTextureLightEXT\0"
   "\0"
   /* _mesa_function_pool[28266]: ResetMinmax (offset 370) */
   "i\0"
   "glResetMinmax\0"
   "glResetMinmaxEXT\0"
   "\0"
   /* _mesa_function_pool[28300]: SpriteParameterfSGIX (dynamic) */
   "if\0"
   "glSpriteParameterfSGIX\0"
   "\0"
   /* _mesa_function_pool[28327]: EnableClientState (offset 313) */
   "i\0"
   "glEnableClientState\0"
   "\0"
   /* _mesa_function_pool[28350]: VertexAttrib4sNV (will be remapped) */
   "iiiii\0"
   "glVertexAttrib4sNV\0"
   "\0"
   /* _mesa_function_pool[28376]: GetConvolutionParameterfv (offset 357) */
   "iip\0"
   "glGetConvolutionParameterfv\0"
   "glGetConvolutionParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[28440]: VertexAttribs4dvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs4dvNV\0"
   "\0"
   /* _mesa_function_pool[28466]: MultiModeDrawArraysIBM (will be remapped) */
   "pppii\0"
   "glMultiModeDrawArraysIBM\0"
   "\0"
   /* _mesa_function_pool[28498]: VertexAttrib4dARB (will be remapped) */
   "idddd\0"
   "glVertexAttrib4d\0"
   "glVertexAttrib4dARB\0"
   "\0"
   /* _mesa_function_pool[28542]: GetTexBumpParameterfvATI (will be remapped) */
   "ip\0"
   "glGetTexBumpParameterfvATI\0"
   "\0"
   /* _mesa_function_pool[28573]: ProgramNamedParameter4dNV (will be remapped) */
   "iipdddd\0"
   "glProgramNamedParameter4dNV\0"
   "\0"
   /* _mesa_function_pool[28610]: GetMaterialfv (offset 269) */
   "iip\0"
   "glGetMaterialfv\0"
   "\0"
   /* _mesa_function_pool[28631]: VertexWeightfEXT (dynamic) */
   "f\0"
   "glVertexWeightfEXT\0"
   "\0"
   /* _mesa_function_pool[28653]: SetFragmentShaderConstantATI (will be remapped) */
   "ip\0"
   "glSetFragmentShaderConstantATI\0"
   "\0"
   /* _mesa_function_pool[28688]: Binormal3fEXT (dynamic) */
   "fff\0"
   "glBinormal3fEXT\0"
   "\0"
   /* _mesa_function_pool[28709]: CallList (offset 2) */
   "i\0"
   "glCallList\0"
   "\0"
   /* _mesa_function_pool[28723]: Materialfv (offset 170) */
   "iip\0"
   "glMaterialfv\0"
   "\0"
   /* _mesa_function_pool[28741]: TexCoord3fv (offset 113) */
   "p\0"
   "glTexCoord3fv\0"
   "\0"
   /* _mesa_function_pool[28758]: FogCoordfvEXT (will be remapped) */
   "p\0"
   "glFogCoordfv\0"
   "glFogCoordfvEXT\0"
   "\0"
   /* _mesa_function_pool[28790]: MultiTexCoord1ivARB (offset 381) */
   "ip\0"
   "glMultiTexCoord1iv\0"
   "glMultiTexCoord1ivARB\0"
   "\0"
   /* _mesa_function_pool[28835]: SecondaryColor3ubEXT (will be remapped) */
   "iii\0"
   "glSecondaryColor3ub\0"
   "glSecondaryColor3ubEXT\0"
   "\0"
   /* _mesa_function_pool[28883]: MultiTexCoord2ivARB (offset 389) */
   "ip\0"
   "glMultiTexCoord2iv\0"
   "glMultiTexCoord2ivARB\0"
   "\0"
   /* _mesa_function_pool[28928]: FogFuncSGIS (dynamic) */
   "ip\0"
   "glFogFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[28946]: CopyTexSubImage2D (offset 326) */
   "iiiiiiii\0"
   "glCopyTexSubImage2D\0"
   "glCopyTexSubImage2DEXT\0"
   "\0"
   /* _mesa_function_pool[28999]: GetObjectParameterivARB (will be remapped) */
   "iip\0"
   "glGetObjectParameterivARB\0"
   "\0"
   /* _mesa_function_pool[29030]: Color3iv (offset 16) */
   "p\0"
   "glColor3iv\0"
   "\0"
   /* _mesa_function_pool[29044]: TexCoord4fVertex4fSUN (dynamic) */
   "ffffffff\0"
   "glTexCoord4fVertex4fSUN\0"
   "\0"
   /* _mesa_function_pool[29078]: DrawElements (offset 311) */
   "iiip\0"
   "glDrawElements\0"
   "\0"
   /* _mesa_function_pool[29099]: BindVertexArrayAPPLE (will be remapped) */
   "i\0"
   "glBindVertexArrayAPPLE\0"
   "\0"
   /* _mesa_function_pool[29125]: GetProgramLocalParameterdvARB (will be remapped) */
   "iip\0"
   "glGetProgramLocalParameterdvARB\0"
   "\0"
   /* _mesa_function_pool[29162]: GetHistogramParameteriv (offset 363) */
   "iip\0"
   "glGetHistogramParameteriv\0"
   "glGetHistogramParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[29222]: MultiTexCoord1iARB (offset 380) */
   "ii\0"
   "glMultiTexCoord1i\0"
   "glMultiTexCoord1iARB\0"
   "\0"
   /* _mesa_function_pool[29265]: GetConvolutionFilter (offset 356) */
   "iiip\0"
   "glGetConvolutionFilter\0"
   "glGetConvolutionFilterEXT\0"
   "\0"
   /* _mesa_function_pool[29320]: GetProgramivARB (will be remapped) */
   "iip\0"
   "glGetProgramivARB\0"
   "\0"
   /* _mesa_function_pool[29343]: TexBufferARB (will be remapped) */
   "iii\0"
   "glTexBufferARB\0"
   "\0"
   /* _mesa_function_pool[29363]: BlendFuncSeparateEXT (will be remapped) */
   "iiii\0"
   "glBlendFuncSeparate\0"
   "glBlendFuncSeparateEXT\0"
   "glBlendFuncSeparateINGR\0"
   "\0"
   /* _mesa_function_pool[29436]: MapBufferRange (will be remapped) */
   "iiii\0"
   "glMapBufferRange\0"
   "\0"
   /* _mesa_function_pool[29459]: ProgramParameters4dvNV (will be remapped) */
   "iiip\0"
   "glProgramParameters4dvNV\0"
   "\0"
   /* _mesa_function_pool[29490]: TexCoord2fColor3fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glTexCoord2fColor3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[29527]: EvalPoint2 (offset 239) */
   "ii\0"
   "glEvalPoint2\0"
   "\0"
   /* _mesa_function_pool[29544]: Uniform1uivEXT (will be remapped) */
   "iip\0"
   "glUniform1uivEXT\0"
   "glUniform1uiv\0"
   "\0"
   /* _mesa_function_pool[29580]: EvalPoint1 (offset 237) */
   "i\0"
   "glEvalPoint1\0"
   "\0"
   /* _mesa_function_pool[29596]: Binormal3dvEXT (dynamic) */
   "p\0"
   "glBinormal3dvEXT\0"
   "\0"
   /* _mesa_function_pool[29616]: PopMatrix (offset 297) */
   "\0"
   "glPopMatrix\0"
   "\0"
   /* _mesa_function_pool[29630]: GetVertexAttribIuivEXT (will be remapped) */
   "iip\0"
   "glGetVertexAttribIuivEXT\0"
   "glGetVertexAttribIuiv\0"
   "\0"
   /* _mesa_function_pool[29682]: FinishFenceNV (will be remapped) */
   "i\0"
   "glFinishFenceNV\0"
   "\0"
   /* _mesa_function_pool[29701]: GetFogFuncSGIS (dynamic) */
   "p\0"
   "glGetFogFuncSGIS\0"
   "\0"
   /* _mesa_function_pool[29721]: GetUniformLocationARB (will be remapped) */
   "ip\0"
   "glGetUniformLocation\0"
   "glGetUniformLocationARB\0"
   "\0"
   /* _mesa_function_pool[29770]: SecondaryColor3fEXT (will be remapped) */
   "fff\0"
   "glSecondaryColor3f\0"
   "glSecondaryColor3fEXT\0"
   "\0"
   /* _mesa_function_pool[29816]: GetTexGeniv (offset 280) */
   "iip\0"
   "glGetTexGeniv\0"
   "\0"
   /* _mesa_function_pool[29835]: CombinerInputNV (will be remapped) */
   "iiiiii\0"
   "glCombinerInputNV\0"
   "\0"
   /* _mesa_function_pool[29861]: VertexAttrib3sARB (will be remapped) */
   "iiii\0"
   "glVertexAttrib3s\0"
   "glVertexAttrib3sARB\0"
   "\0"
   /* _mesa_function_pool[29904]: IsTransformFeedback (will be remapped) */
   "i\0"
   "glIsTransformFeedback\0"
   "\0"
   /* _mesa_function_pool[29929]: ReplacementCodeuiNormal3fVertex3fvSUN (dynamic) */
   "ppp\0"
   "glReplacementCodeuiNormal3fVertex3fvSUN\0"
   "\0"
   /* _mesa_function_pool[29974]: Map2d (offset 222) */
   "iddiiddiip\0"
   "glMap2d\0"
   "\0"
   /* _mesa_function_pool[29994]: Map2f (offset 223) */
   "iffiiffiip\0"
   "glMap2f\0"
   "\0"
   /* _mesa_function_pool[30014]: ProgramStringARB (will be remapped) */
   "iiip\0"
   "glProgramStringARB\0"
   "\0"
   /* _mesa_function_pool[30039]: Vertex4s (offset 148) */
   "iiii\0"
   "glVertex4s\0"
   "\0"
   /* _mesa_function_pool[30056]: TexCoord4fVertex4fvSUN (dynamic) */
   "pp\0"
   "glTexCoord4fVertex4fvSUN\0"
   "\0"
   /* _mesa_function_pool[30085]: FragmentLightModelivSGIX (dynamic) */
   "ip\0"
   "glFragmentLightModelivSGIX\0"
   "\0"
   /* _mesa_function_pool[30116]: VertexAttrib1fNV (will be remapped) */
   "if\0"
   "glVertexAttrib1fNV\0"
   "\0"
   /* _mesa_function_pool[30139]: Vertex4f (offset 144) */
   "ffff\0"
   "glVertex4f\0"
   "\0"
   /* _mesa_function_pool[30156]: EvalCoord1d (offset 228) */
   "d\0"
   "glEvalCoord1d\0"
   "\0"
   /* _mesa_function_pool[30173]: Vertex4d (offset 142) */
   "dddd\0"
   "glVertex4d\0"
   "\0"
   /* _mesa_function_pool[30190]: RasterPos4dv (offset 79) */
   "p\0"
   "glRasterPos4dv\0"
   "\0"
   /* _mesa_function_pool[30208]: UseShaderProgramEXT (will be remapped) */
   "ii\0"
   "glUseShaderProgramEXT\0"
   "\0"
   /* _mesa_function_pool[30234]: FragmentLightfSGIX (dynamic) */
   "iif\0"
   "glFragmentLightfSGIX\0"
   "\0"
   /* _mesa_function_pool[30260]: GetCompressedTexImageARB (will be remapped) */
   "iip\0"
   "glGetCompressedTexImage\0"
   "glGetCompressedTexImageARB\0"
   "\0"
   /* _mesa_function_pool[30316]: GetTexGenfv (offset 279) */
   "iip\0"
   "glGetTexGenfv\0"
   "\0"
   /* _mesa_function_pool[30335]: Vertex4i (offset 146) */
   "iiii\0"
   "glVertex4i\0"
   "\0"
   /* _mesa_function_pool[30352]: VertexWeightPointerEXT (dynamic) */
   "iiip\0"
   "glVertexWeightPointerEXT\0"
   "\0"
   /* _mesa_function_pool[30383]: GetHistogram (offset 361) */
   "iiiip\0"
   "glGetHistogram\0"
   "glGetHistogramEXT\0"
   "\0"
   /* _mesa_function_pool[30423]: ActiveStencilFaceEXT (will be remapped) */
   "i\0"
   "glActiveStencilFaceEXT\0"
   "\0"
   /* _mesa_function_pool[30449]: StencilFuncSeparateATI (will be remapped) */
   "iiii\0"
   "glStencilFuncSeparateATI\0"
   "\0"
   /* _mesa_function_pool[30480]: Materialf (offset 169) */
   "iif\0"
   "glMaterialf\0"
   "\0"
   /* _mesa_function_pool[30497]: GetShaderSourceARB (will be remapped) */
   "iipp\0"
   "glGetShaderSource\0"
   "glGetShaderSourceARB\0"
   "\0"
   /* _mesa_function_pool[30542]: IglooInterfaceSGIX (dynamic) */
   "ip\0"
   "glIglooInterfaceSGIX\0"
   "\0"
   /* _mesa_function_pool[30567]: Materiali (offset 171) */
   "iii\0"
   "glMateriali\0"
   "\0"
   /* _mesa_function_pool[30584]: VertexAttrib4dNV (will be remapped) */
   "idddd\0"
   "glVertexAttrib4dNV\0"
   "\0"
   /* _mesa_function_pool[30610]: MultiModeDrawElementsIBM (will be remapped) */
   "ppipii\0"
   "glMultiModeDrawElementsIBM\0"
   "\0"
   /* _mesa_function_pool[30645]: Indexsv (offset 51) */
   "p\0"
   "glIndexsv\0"
   "\0"
   /* _mesa_function_pool[30658]: MultiTexCoord4svARB (offset 407) */
   "ip\0"
   "glMultiTexCoord4sv\0"
   "glMultiTexCoord4svARB\0"
   "\0"
   /* _mesa_function_pool[30703]: LightModelfv (offset 164) */
   "ip\0"
   "glLightModelfv\0"
   "\0"
   /* _mesa_function_pool[30722]: TexCoord2dv (offset 103) */
   "p\0"
   "glTexCoord2dv\0"
   "\0"
   /* _mesa_function_pool[30739]: GenQueriesARB (will be remapped) */
   "ip\0"
   "glGenQueries\0"
   "glGenQueriesARB\0"
   "\0"
   /* _mesa_function_pool[30772]: EvalCoord1dv (offset 229) */
   "p\0"
   "glEvalCoord1dv\0"
   "\0"
   /* _mesa_function_pool[30790]: ReplacementCodeuiVertex3fSUN (dynamic) */
   "ifff\0"
   "glReplacementCodeuiVertex3fSUN\0"
   "\0"
   /* _mesa_function_pool[30827]: Translated (offset 303) */
   "ddd\0"
   "glTranslated\0"
   "\0"
   /* _mesa_function_pool[30845]: Translatef (offset 304) */
   "fff\0"
   "glTranslatef\0"
   "\0"
   /* _mesa_function_pool[30863]: Uniform3uiEXT (will be remapped) */
   "iiii\0"
   "glUniform3uiEXT\0"
   "glUniform3ui\0"
   "\0"
   /* _mesa_function_pool[30898]: StencilMask (offset 209) */
   "i\0"
   "glStencilMask\0"
   "\0"
   /* _mesa_function_pool[30915]: Tangent3iEXT (dynamic) */
   "iii\0"
   "glTangent3iEXT\0"
   "\0"
   /* _mesa_function_pool[30935]: ClampColorARB (will be remapped) */
   "ii\0"
   "glClampColorARB\0"
   "\0"
   /* _mesa_function_pool[30955]: GetLightiv (offset 265) */
   "iip\0"
   "glGetLightiv\0"
   "\0"
   /* _mesa_function_pool[30973]: GetSamplerParameterIiv (will be remapped) */
   "iip\0"
   "glGetSamplerParameterIiv\0"
   "\0"
   /* _mesa_function_pool[31003]: DrawMeshArraysSUN (dynamic) */
   "iiii\0"
   "glDrawMeshArraysSUN\0"
   "\0"
   /* _mesa_function_pool[31029]: IsList (offset 287) */
   "i\0"
   "glIsList\0"
   "\0"
   /* _mesa_function_pool[31041]: IsSync (will be remapped) */
   "i\0"
   "glIsSync\0"
   "\0"
   /* _mesa_function_pool[31053]: RenderMode (offset 196) */
   "i\0"
   "glRenderMode\0"
   "\0"
   /* _mesa_function_pool[31069]: GetMapControlPointsNV (dynamic) */
   "iiiiiip\0"
   "glGetMapControlPointsNV\0"
   "\0"
   /* _mesa_function_pool[31102]: DrawBuffersARB (will be remapped) */
   "ip\0"
   "glDrawBuffers\0"
   "glDrawBuffersARB\0"
   "glDrawBuffersATI\0"
   "\0"
   /* _mesa_function_pool[31154]: ClearBufferiv (will be remapped) */
   "iip\0"
   "glClearBufferiv\0"
   "\0"
   /* _mesa_function_pool[31175]: ProgramLocalParameter4fARB (will be remapped) */
   "iiffff\0"
   "glProgramLocalParameter4fARB\0"
   "\0"
   /* _mesa_function_pool[31212]: SpriteParameterivSGIX (dynamic) */
   "ip\0"
   "glSpriteParameterivSGIX\0"
   "\0"
   /* _mesa_function_pool[31240]: ProvokingVertexEXT (will be remapped) */
   "i\0"
   "glProvokingVertexEXT\0"
   "glProvokingVertex\0"
   "\0"
   /* _mesa_function_pool[31282]: MultiTexCoord1fARB (offset 378) */
   "if\0"
   "glMultiTexCoord1f\0"
   "glMultiTexCoord1fARB\0"
   "\0"
   /* _mesa_function_pool[31325]: LoadName (offset 198) */
   "i\0"
   "glLoadName\0"
   "\0"
   /* _mesa_function_pool[31339]: VertexAttribs4ubvNV (will be remapped) */
   "iip\0"
   "glVertexAttribs4ubvNV\0"
   "\0"
   /* _mesa_function_pool[31366]: WeightsvARB (dynamic) */
   "ip\0"
   "glWeightsvARB\0"
   "\0"
   /* _mesa_function_pool[31384]: NormalP3ui (will be remapped) */
   "ii\0"
   "glNormalP3ui\0"
   "\0"
   /* _mesa_function_pool[31401]: Uniform1fvARB (will be remapped) */
   "iip\0"
   "glUniform1fv\0"
   "glUniform1fvARB\0"
   "\0"
   /* _mesa_function_pool[31435]: CopyTexSubImage1D (offset 325) */
   "iiiiii\0"
   "glCopyTexSubImage1D\0"
   "glCopyTexSubImage1DEXT\0"
   "\0"
   /* _mesa_function_pool[31486]: CullFace (offset 152) */
   "i\0"
   "glCullFace\0"
   "\0"
   /* _mesa_function_pool[31500]: BindTexture (offset 307) */
   "ii\0"
   "glBindTexture\0"
   "glBindTextureEXT\0"
   "\0"
   /* _mesa_function_pool[31535]: BeginFragmentShaderATI (will be remapped) */
   "\0"
   "glBeginFragmentShaderATI\0"
   "\0"
   /* _mesa_function_pool[31562]: MultiTexCoord4fARB (offset 402) */
   "iffff\0"
   "glMultiTexCoord4f\0"
   "glMultiTexCoord4fARB\0"
   "\0"
   /* _mesa_function_pool[31608]: VertexAttribs3svNV (will be remapped) */
   "iip\0"
   "glVertexAttribs3svNV\0"
   "\0"
   /* _mesa_function_pool[31634]: StencilFunc (offset 243) */
   "iii\0"
   "glStencilFunc\0"
   "\0"
   /* _mesa_function_pool[31653]: CopyPixels (offset 255) */
   "iiiii\0"
   "glCopyPixels\0"
   "\0"
   /* _mesa_function_pool[31673]: Rectsv (offset 93) */
   "pp\0"
   "glRectsv\0"
   "\0"
   /* _mesa_function_pool[31686]: ReplacementCodeuivSUN (dynamic) */
   "p\0"
   "glReplacementCodeuivSUN\0"
   "\0"
   /* _mesa_function_pool[31713]: MultiTexCoordP2uiv (will be remapped) */
   "iip\0"
   "glMultiTexCoordP2uiv\0"
   "\0"
   /* _mesa_function_pool[31739]: EnableVertexAttribArrayARB (will be remapped) */
   "i\0"
   "glEnableVertexAttribArray\0"
   "glEnableVertexAttribArrayARB\0"
   "\0"
   /* _mesa_function_pool[31797]: NormalPointervINTEL (dynamic) */
   "ip\0"
   "glNormalPointervINTEL\0"
   "\0"
   /* _mesa_function_pool[31823]: CopyConvolutionFilter2D (offset 355) */
   "iiiiii\0"
   "glCopyConvolutionFilter2D\0"
   "glCopyConvolutionFilter2DEXT\0"
   "\0"
   /* _mesa_function_pool[31886]: WindowPos3ivMESA (will be remapped) */
   "p\0"
   "glWindowPos3iv\0"
   "glWindowPos3ivARB\0"
   "glWindowPos3ivMESA\0"
   "\0"
   /* _mesa_function_pool[31941]: CopyBufferSubData (will be remapped) */
   "iiiii\0"
   "glCopyBufferSubData\0"
   "\0"
   /* _mesa_function_pool[31968]: NormalPointer (offset 318) */
   "iip\0"
   "glNormalPointer\0"
   "\0"
   /* _mesa_function_pool[31989]: TexParameterfv (offset 179) */
   "iip\0"
   "glTexParameterfv\0"
   "\0"
   /* _mesa_function_pool[32011]: IsBufferARB (will be remapped) */
   "i\0"
   "glIsBuffer\0"
   "glIsBufferARB\0"
   "\0"
   /* _mesa_function_pool[32039]: WindowPos4iMESA (will be remapped) */
   "iiii\0"
   "glWindowPos4iMESA\0"
   "\0"
   /* _mesa_function_pool[32063]: VertexAttrib4uivARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4uiv\0"
   "glVertexAttrib4uivARB\0"
   "\0"
   /* _mesa_function_pool[32108]: Tangent3bvEXT (dynamic) */
   "p\0"
   "glTangent3bvEXT\0"
   "\0"
   /* _mesa_function_pool[32127]: VertexAttribI3uivEXT (will be remapped) */
   "ip\0"
   "glVertexAttribI3uivEXT\0"
   "glVertexAttribI3uiv\0"
   "\0"
   /* _mesa_function_pool[32174]: UniformMatrix3x4fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix3x4fv\0"
   "\0"
   /* _mesa_function_pool[32201]: ClipPlane (offset 150) */
   "ip\0"
   "glClipPlane\0"
   "\0"
   /* _mesa_function_pool[32217]: Recti (offset 90) */
   "iiii\0"
   "glRecti\0"
   "\0"
   /* _mesa_function_pool[32231]: TrackMatrixNV (will be remapped) */
   "iiii\0"
   "glTrackMatrixNV\0"
   "\0"
   /* _mesa_function_pool[32253]: DrawRangeElementsBaseVertex (will be remapped) */
   "iiiiipi\0"
   "glDrawRangeElementsBaseVertex\0"
   "\0"
   /* _mesa_function_pool[32292]: SamplerParameterIuiv (will be remapped) */
   "iip\0"
   "glSamplerParameterIuiv\0"
   "\0"
   /* _mesa_function_pool[32320]: TexCoordPointervINTEL (dynamic) */
   "iip\0"
   "glTexCoordPointervINTEL\0"
   "\0"
   /* _mesa_function_pool[32349]: DeleteBuffersARB (will be remapped) */
   "ip\0"
   "glDeleteBuffers\0"
   "glDeleteBuffersARB\0"
   "\0"
   /* _mesa_function_pool[32388]: PixelTransformParameterfvEXT (dynamic) */
   "iip\0"
   "glPixelTransformParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[32424]: PrimitiveRestartNV (will be remapped) */
   "\0"
   "glPrimitiveRestartNV\0"
   "\0"
   /* _mesa_function_pool[32447]: WindowPos4fvMESA (will be remapped) */
   "p\0"
   "glWindowPos4fvMESA\0"
   "\0"
   /* _mesa_function_pool[32469]: GetPixelMapuiv (offset 272) */
   "ip\0"
   "glGetPixelMapuiv\0"
   "\0"
   /* _mesa_function_pool[32490]: Rectf (offset 88) */
   "ffff\0"
   "glRectf\0"
   "\0"
   /* _mesa_function_pool[32504]: VertexAttrib1sNV (will be remapped) */
   "ii\0"
   "glVertexAttrib1sNV\0"
   "\0"
   /* _mesa_function_pool[32527]: Indexfv (offset 47) */
   "p\0"
   "glIndexfv\0"
   "\0"
   /* _mesa_function_pool[32540]: ColorP3uiv (will be remapped) */
   "ip\0"
   "glColorP3uiv\0"
   "\0"
   /* _mesa_function_pool[32557]: SecondaryColor3svEXT (will be remapped) */
   "p\0"
   "glSecondaryColor3sv\0"
   "glSecondaryColor3svEXT\0"
   "\0"
   /* _mesa_function_pool[32603]: LoadTransposeMatrixfARB (will be remapped) */
   "p\0"
   "glLoadTransposeMatrixf\0"
   "glLoadTransposeMatrixfARB\0"
   "\0"
   /* _mesa_function_pool[32655]: GetPointerv (offset 329) */
   "ip\0"
   "glGetPointerv\0"
   "glGetPointervEXT\0"
   "\0"
   /* _mesa_function_pool[32690]: Tangent3bEXT (dynamic) */
   "iii\0"
   "glTangent3bEXT\0"
   "\0"
   /* _mesa_function_pool[32710]: CombinerParameterfNV (will be remapped) */
   "if\0"
   "glCombinerParameterfNV\0"
   "\0"
   /* _mesa_function_pool[32737]: IndexMask (offset 212) */
   "i\0"
   "glIndexMask\0"
   "\0"
   /* _mesa_function_pool[32752]: BindProgramNV (will be remapped) */
   "ii\0"
   "glBindProgramARB\0"
   "glBindProgramNV\0"
   "\0"
   /* _mesa_function_pool[32789]: VertexAttrib4svARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4sv\0"
   "glVertexAttrib4svARB\0"
   "\0"
   /* _mesa_function_pool[32832]: GetFloatv (offset 262) */
   "ip\0"
   "glGetFloatv\0"
   "\0"
   /* _mesa_function_pool[32848]: CreateDebugObjectMESA (dynamic) */
   "\0"
   "glCreateDebugObjectMESA\0"
   "\0"
   /* _mesa_function_pool[32874]: GetShaderiv (will be remapped) */
   "iip\0"
   "glGetShaderiv\0"
   "\0"
   /* _mesa_function_pool[32893]: ClientWaitSync (will be remapped) */
   "iii\0"
   "glClientWaitSync\0"
   "\0"
   /* _mesa_function_pool[32915]: TexCoord4s (offset 124) */
   "iiii\0"
   "glTexCoord4s\0"
   "\0"
   /* _mesa_function_pool[32934]: TexCoord3sv (offset 117) */
   "p\0"
   "glTexCoord3sv\0"
   "\0"
   /* _mesa_function_pool[32951]: BindFragmentShaderATI (will be remapped) */
   "i\0"
   "glBindFragmentShaderATI\0"
   "\0"
   /* _mesa_function_pool[32978]: PopAttrib (offset 218) */
   "\0"
   "glPopAttrib\0"
   "\0"
   /* _mesa_function_pool[32992]: Fogfv (offset 154) */
   "ip\0"
   "glFogfv\0"
   "\0"
   /* _mesa_function_pool[33004]: UnmapBufferARB (will be remapped) */
   "i\0"
   "glUnmapBuffer\0"
   "glUnmapBufferARB\0"
   "\0"
   /* _mesa_function_pool[33038]: InitNames (offset 197) */
   "\0"
   "glInitNames\0"
   "\0"
   /* _mesa_function_pool[33052]: Normal3sv (offset 61) */
   "p\0"
   "glNormal3sv\0"
   "\0"
   /* _mesa_function_pool[33067]: Minmax (offset 368) */
   "iii\0"
   "glMinmax\0"
   "glMinmaxEXT\0"
   "\0"
   /* _mesa_function_pool[33093]: TexCoord4d (offset 118) */
   "dddd\0"
   "glTexCoord4d\0"
   "\0"
   /* _mesa_function_pool[33112]: TexCoord4f (offset 120) */
   "ffff\0"
   "glTexCoord4f\0"
   "\0"
   /* _mesa_function_pool[33131]: FogCoorddvEXT (will be remapped) */
   "p\0"
   "glFogCoorddv\0"
   "glFogCoorddvEXT\0"
   "\0"
   /* _mesa_function_pool[33163]: FinishTextureSUNX (dynamic) */
   "\0"
   "glFinishTextureSUNX\0"
   "\0"
   /* _mesa_function_pool[33185]: GetFragmentLightfvSGIX (dynamic) */
   "iip\0"
   "glGetFragmentLightfvSGIX\0"
   "\0"
   /* _mesa_function_pool[33215]: Binormal3fvEXT (dynamic) */
   "p\0"
   "glBinormal3fvEXT\0"
   "\0"
   /* _mesa_function_pool[33235]: GetBooleanv (offset 258) */
   "ip\0"
   "glGetBooleanv\0"
   "\0"
   /* _mesa_function_pool[33253]: ColorFragmentOp3ATI (will be remapped) */
   "iiiiiiiiiiiii\0"
   "glColorFragmentOp3ATI\0"
   "\0"
   /* _mesa_function_pool[33290]: Hint (offset 158) */
   "ii\0"
   "glHint\0"
   "\0"
   /* _mesa_function_pool[33301]: Color4dv (offset 28) */
   "p\0"
   "glColor4dv\0"
   "\0"
   /* _mesa_function_pool[33315]: VertexAttrib2svARB (will be remapped) */
   "ip\0"
   "glVertexAttrib2sv\0"
   "glVertexAttrib2svARB\0"
   "\0"
   /* _mesa_function_pool[33358]: AreProgramsResidentNV (will be remapped) */
   "ipp\0"
   "glAreProgramsResidentNV\0"
   "\0"
   /* _mesa_function_pool[33387]: WindowPos3svMESA (will be remapped) */
   "p\0"
   "glWindowPos3sv\0"
   "glWindowPos3svARB\0"
   "glWindowPos3svMESA\0"
   "\0"
   /* _mesa_function_pool[33442]: CopyColorSubTable (offset 347) */
   "iiiii\0"
   "glCopyColorSubTable\0"
   "glCopyColorSubTableEXT\0"
   "\0"
   /* _mesa_function_pool[33492]: WeightdvARB (dynamic) */
   "ip\0"
   "glWeightdvARB\0"
   "\0"
   /* _mesa_function_pool[33510]: DeleteRenderbuffersEXT (will be remapped) */
   "ip\0"
   "glDeleteRenderbuffers\0"
   "glDeleteRenderbuffersEXT\0"
   "\0"
   /* _mesa_function_pool[33561]: VertexAttrib4NubvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib4Nubv\0"
   "glVertexAttrib4NubvARB\0"
   "\0"
   /* _mesa_function_pool[33608]: VertexAttrib3dvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib3dvNV\0"
   "\0"
   /* _mesa_function_pool[33632]: GetObjectParameterfvARB (will be remapped) */
   "iip\0"
   "glGetObjectParameterfvARB\0"
   "\0"
   /* _mesa_function_pool[33663]: Vertex4iv (offset 147) */
   "p\0"
   "glVertex4iv\0"
   "\0"
   /* _mesa_function_pool[33678]: GetProgramEnvParameterdvARB (will be remapped) */
   "iip\0"
   "glGetProgramEnvParameterdvARB\0"
   "\0"
   /* _mesa_function_pool[33713]: TexCoord4dv (offset 119) */
   "p\0"
   "glTexCoord4dv\0"
   "\0"
   /* _mesa_function_pool[33730]: LockArraysEXT (will be remapped) */
   "ii\0"
   "glLockArraysEXT\0"
   "\0"
   /* _mesa_function_pool[33750]: Begin (offset 7) */
   "i\0"
   "glBegin\0"
   "\0"
   /* _mesa_function_pool[33761]: LightModeli (offset 165) */
   "ii\0"
   "glLightModeli\0"
   "\0"
   /* _mesa_function_pool[33779]: VertexAttribI4ivEXT (will be remapped) */
   "ip\0"
   "glVertexAttribI4ivEXT\0"
   "glVertexAttribI4iv\0"
   "\0"
   /* _mesa_function_pool[33824]: Rectfv (offset 89) */
   "pp\0"
   "glRectfv\0"
   "\0"
   /* _mesa_function_pool[33837]: BlendEquationSeparateiARB (will be remapped) */
   "iii\0"
   "glBlendEquationSeparateiARB\0"
   "glBlendEquationSeparateIndexedAMD\0"
   "\0"
   /* _mesa_function_pool[33904]: LightModelf (offset 163) */
   "if\0"
   "glLightModelf\0"
   "\0"
   /* _mesa_function_pool[33922]: GetTexParameterfv (offset 282) */
   "iip\0"
   "glGetTexParameterfv\0"
   "\0"
   /* _mesa_function_pool[33947]: GetLightfv (offset 264) */
   "iip\0"
   "glGetLightfv\0"
   "\0"
   /* _mesa_function_pool[33965]: PixelTransformParameterivEXT (dynamic) */
   "iip\0"
   "glPixelTransformParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[34001]: BinormalPointerEXT (dynamic) */
   "iip\0"
   "glBinormalPointerEXT\0"
   "\0"
   /* _mesa_function_pool[34027]: VertexAttrib1dNV (will be remapped) */
   "id\0"
   "glVertexAttrib1dNV\0"
   "\0"
   /* _mesa_function_pool[34050]: GetCombinerInputParameterivNV (will be remapped) */
   "iiiip\0"
   "glGetCombinerInputParameterivNV\0"
   "\0"
   /* _mesa_function_pool[34089]: Disable (offset 214) */
   "i\0"
   "glDisable\0"
   "\0"
   /* _mesa_function_pool[34102]: MultiTexCoord2fvARB (offset 387) */
   "ip\0"
   "glMultiTexCoord2fv\0"
   "glMultiTexCoord2fvARB\0"
   "\0"
   /* _mesa_function_pool[34147]: GetRenderbufferParameterivEXT (will be remapped) */
   "iip\0"
   "glGetRenderbufferParameteriv\0"
   "glGetRenderbufferParameterivEXT\0"
   "\0"
   /* _mesa_function_pool[34213]: CombinerParameterivNV (will be remapped) */
   "ip\0"
   "glCombinerParameterivNV\0"
   "\0"
   /* _mesa_function_pool[34241]: GenFragmentShadersATI (will be remapped) */
   "i\0"
   "glGenFragmentShadersATI\0"
   "\0"
   /* _mesa_function_pool[34268]: DrawArrays (offset 310) */
   "iii\0"
   "glDrawArrays\0"
   "glDrawArraysEXT\0"
   "\0"
   /* _mesa_function_pool[34302]: WeightuivARB (dynamic) */
   "ip\0"
   "glWeightuivARB\0"
   "\0"
   /* _mesa_function_pool[34321]: GetVertexAttribIivEXT (will be remapped) */
   "iip\0"
   "glGetVertexAttribIivEXT\0"
   "glGetVertexAttribIiv\0"
   "\0"
   /* _mesa_function_pool[34371]: VertexAttrib2sARB (will be remapped) */
   "iii\0"
   "glVertexAttrib2s\0"
   "glVertexAttrib2sARB\0"
   "\0"
   /* _mesa_function_pool[34413]: GetnTexImageARB (will be remapped) */
   "iiiiip\0"
   "glGetnTexImageARB\0"
   "\0"
   /* _mesa_function_pool[34439]: ColorMask (offset 210) */
   "iiii\0"
   "glColorMask\0"
   "\0"
   /* _mesa_function_pool[34457]: GenAsyncMarkersSGIX (dynamic) */
   "i\0"
   "glGenAsyncMarkersSGIX\0"
   "\0"
   /* _mesa_function_pool[34482]: Tangent3svEXT (dynamic) */
   "p\0"
   "glTangent3svEXT\0"
   "\0"
   /* _mesa_function_pool[34501]: GetListParameterivSGIX (dynamic) */
   "iip\0"
   "glGetListParameterivSGIX\0"
   "\0"
   /* _mesa_function_pool[34531]: BindBufferARB (will be remapped) */
   "ii\0"
   "glBindBuffer\0"
   "glBindBufferARB\0"
   "\0"
   /* _mesa_function_pool[34564]: GetInfoLogARB (will be remapped) */
   "iipp\0"
   "glGetInfoLogARB\0"
   "\0"
   /* _mesa_function_pool[34586]: RasterPos4iv (offset 83) */
   "p\0"
   "glRasterPos4iv\0"
   "\0"
   /* _mesa_function_pool[34604]: Enable (offset 215) */
   "i\0"
   "glEnable\0"
   "\0"
   /* _mesa_function_pool[34616]: LineStipple (offset 167) */
   "ii\0"
   "glLineStipple\0"
   "\0"
   /* _mesa_function_pool[34634]: VertexAttribP2ui (will be remapped) */
   "iiii\0"
   "glVertexAttribP2ui\0"
   "\0"
   /* _mesa_function_pool[34659]: VertexAttribs4svNV (will be remapped) */
   "iip\0"
   "glVertexAttribs4svNV\0"
   "\0"
   /* _mesa_function_pool[34685]: EdgeFlagPointerListIBM (dynamic) */
   "ipi\0"
   "glEdgeFlagPointerListIBM\0"
   "\0"
   /* _mesa_function_pool[34715]: UniformMatrix3x2fv (will be remapped) */
   "iiip\0"
   "glUniformMatrix3x2fv\0"
   "\0"
   /* _mesa_function_pool[34742]: GetMinmaxParameterfv (offset 365) */
   "iip\0"
   "glGetMinmaxParameterfv\0"
   "glGetMinmaxParameterfvEXT\0"
   "\0"
   /* _mesa_function_pool[34796]: VertexAttrib1fvARB (will be remapped) */
   "ip\0"
   "glVertexAttrib1fv\0"
   "glVertexAttrib1fvARB\0"
   "\0"
   /* _mesa_function_pool[34839]: GenBuffersARB (will be remapped) */
   "ip\0"
   "glGenBuffers\0"
   "glGenBuffersARB\0"
   "\0"
   /* _mesa_function_pool[34872]: VertexAttribs1svNV (will be remapped) */
   "iip\0"
   "glVertexAttribs1svNV\0"
   "\0"
   /* _mesa_function_pool[34898]: Vertex3fv (offset 137) */
   "p\0"
   "glVertex3fv\0"
   "\0"
   /* _mesa_function_pool[34913]: GetTexBumpParameterivATI (will be remapped) */
   "ip\0"
   "glGetTexBumpParameterivATI\0"
   "\0"
   /* _mesa_function_pool[34944]: Binormal3bEXT (dynamic) */
   "iii\0"
   "glBinormal3bEXT\0"
   "\0"
   /* _mesa_function_pool[34965]: FragmentMaterialivSGIX (dynamic) */
   "iip\0"
   "glFragmentMaterialivSGIX\0"
   "\0"
   /* _mesa_function_pool[34995]: IsRenderbufferEXT (will be remapped) */
   "i\0"
   "glIsRenderbuffer\0"
   "glIsRenderbufferEXT\0"
   "\0"
   /* _mesa_function_pool[35035]: GenProgramsNV (will be remapped) */
   "ip\0"
   "glGenProgramsARB\0"
   "glGenProgramsNV\0"
   "\0"
   /* _mesa_function_pool[35072]: VertexAttrib4dvNV (will be remapped) */
   "ip\0"
   "glVertexAttrib4dvNV\0"
   "\0"
   /* _mesa_function_pool[35096]: EndFragmentShaderATI (will be remapped) */
   "\0"
   "glEndFragmentShaderATI\0"
   "\0"
   /* _mesa_function_pool[35121]: Binormal3iEXT (dynamic) */
   "iii\0"
   "glBinormal3iEXT\0"
   "\0"
   /* _mesa_function_pool[35142]: WindowPos2fMESA (will be remapped) */
   "ff\0"
   "glWindowPos2f\0"
   "glWindowPos2fARB\0"
   "glWindowPos2fMESA\0"
   "\0"
   ;

/* these functions need to be remapped */
static const struct gl_function_pool_remap MESA_remap_table_functions[] = {
   {  1629, AttachShader_remap_index },
   { 10613, CreateProgram_remap_index },
   { 24486, CreateShader_remap_index },
   { 27060, DeleteProgram_remap_index },
   { 19799, DeleteShader_remap_index },
   { 24979, DetachShader_remap_index },
   { 19046, GetAttachedShaders_remap_index },
   {  5341, GetProgramInfoLog_remap_index },
   {   444, GetProgramiv_remap_index },
   {  7189, GetShaderInfoLog_remap_index },
   { 32874, GetShaderiv_remap_index },
   { 14162, IsProgram_remap_index },
   { 13063, IsShader_remap_index },
   { 10743, StencilFuncSeparate_remap_index },
   {  4371, StencilMaskSeparate_remap_index },
   {  8254, StencilOpSeparate_remap_index },
   { 23720, UniformMatrix2x3fv_remap_index },
   {  3098, UniformMatrix2x4fv_remap_index },
   { 34715, UniformMatrix3x2fv_remap_index },
   { 32174, UniformMatrix3x4fv_remap_index },
   { 17220, UniformMatrix4x2fv_remap_index },
   {  3589, UniformMatrix4x3fv_remap_index },
   {  5564, ClampColor_remap_index },
   { 19100, ClearBufferfi_remap_index },
   { 18516, ClearBufferfv_remap_index },
   { 31154, ClearBufferiv_remap_index },
   { 14367, ClearBufferuiv_remap_index },
   { 21222, GetStringi_remap_index },
   {  3000, TexBuffer_remap_index },
   {   977, FramebufferTexture_remap_index },
   { 28096, GetBufferParameteri64v_remap_index },
   { 10843, GetInteger64i_v_remap_index },
   { 24800, VertexAttribDivisor_remap_index },
   { 10631, LoadTransposeMatrixdARB_remap_index },
   { 32603, LoadTransposeMatrixfARB_remap_index },
   {  6249, MultTransposeMatrixdARB_remap_index },
   { 25166, MultTransposeMatrixfARB_remap_index },
   {   255, SampleCoverageARB_remap_index },
   {  6452, CompressedTexImage1DARB_remap_index },
   { 25694, CompressedTexImage2DARB_remap_index },
   {  4434, CompressedTexImage3DARB_remap_index },
   { 19388, CompressedTexSubImage1DARB_remap_index },
   {  2185, CompressedTexSubImage2DARB_remap_index },
   { 21644, CompressedTexSubImage3DARB_remap_index },
   { 30260, GetCompressedTexImageARB_remap_index },
   {  4181, DisableVertexAttribArrayARB_remap_index },
   { 31739, EnableVertexAttribArrayARB_remap_index },
   { 33678, GetProgramEnvParameterdvARB_remap_index },
   { 25046, GetProgramEnvParameterfvARB_remap_index },
   { 29125, GetProgramLocalParameterdvARB_remap_index },
   {  8730, GetProgramLocalParameterfvARB_remap_index },
   { 19564, GetProgramStringARB_remap_index },
   { 29320, GetProgramivARB_remap_index },
   { 21839, GetVertexAttribdvARB_remap_index },
   { 17028, GetVertexAttribfvARB_remap_index },
   { 10422, GetVertexAttribivARB_remap_index },
   { 20659, ProgramEnvParameter4dARB_remap_index },
   { 26810, ProgramEnvParameter4dvARB_remap_index },
   { 17814, ProgramEnvParameter4fARB_remap_index },
   {  9605, ProgramEnvParameter4fvARB_remap_index },
   {  4397, ProgramLocalParameter4dARB_remap_index },
   { 13872, ProgramLocalParameter4dvARB_remap_index },
   { 31175, ProgramLocalParameter4fARB_remap_index },
   { 27456, ProgramLocalParameter4fvARB_remap_index },
   { 30014, ProgramStringARB_remap_index },
   { 20953, VertexAttrib1dARB_remap_index },
   { 16629, VertexAttrib1dvARB_remap_index },
   {  4593, VertexAttrib1fARB_remap_index },
   { 34796, VertexAttrib1fvARB_remap_index },
   {  7780, VertexAttrib1sARB_remap_index },
   {  2380, VertexAttrib1svARB_remap_index },
   { 16042, VertexAttrib2dARB_remap_index },
   { 18537, VertexAttrib2dvARB_remap_index },
   {  1712, VertexAttrib2fARB_remap_index },
   { 18650, VertexAttrib2fvARB_remap_index },
   { 34371, VertexAttrib2sARB_remap_index },
   { 33315, VertexAttrib2svARB_remap_index },
   { 12047, VertexAttrib3dARB_remap_index },
   {  9272, VertexAttrib3dvARB_remap_index },
   {  1799, VertexAttrib3fARB_remap_index },
   { 24037, VertexAttrib3fvARB_remap_index },
   { 29861, VertexAttrib3sARB_remap_index },
   { 21581, VertexAttrib3svARB_remap_index },
   {  5384, VertexAttrib4NbvARB_remap_index },
   { 18923, VertexAttrib4NivARB_remap_index },
   { 23992, VertexAttrib4NsvARB_remap_index },
   { 24998, VertexAttrib4NubARB_remap_index },
   { 33561, VertexAttrib4NubvARB_remap_index },
   { 20310, VertexAttrib4NuivARB_remap_index },
   {  3419, VertexAttrib4NusvARB_remap_index },
   { 11636, VertexAttrib4bvARB_remap_index },
   { 28498, VertexAttrib4dARB_remap_index },
   { 22654, VertexAttrib4dvARB_remap_index },
   { 12201, VertexAttrib4fARB_remap_index },
   { 12605, VertexAttrib4fvARB_remap_index },
   { 10986, VertexAttrib4ivARB_remap_index },
   { 18330, VertexAttrib4sARB_remap_index },
   { 32789, VertexAttrib4svARB_remap_index },
   { 17619, VertexAttrib4ubvARB_remap_index },
   { 32063, VertexAttrib4uivARB_remap_index },
   { 21392, VertexAttrib4usvARB_remap_index },
   { 23515, VertexAttribPointerARB_remap_index },
   { 34531, BindBufferARB_remap_index },
   {  7487, BufferDataARB_remap_index },
   {  1550, BufferSubDataARB_remap_index },
   { 32349, DeleteBuffersARB_remap_index },
   { 34839, GenBuffersARB_remap_index },
   { 18693, GetBufferParameterivARB_remap_index },
   { 17766, GetBufferPointervARB_remap_index },
   {  1503, GetBufferSubDataARB_remap_index },
   { 32011, IsBufferARB_remap_index },
   { 27940, MapBufferARB_remap_index },
   { 33004, UnmapBufferARB_remap_index },
   {   351, BeginQueryARB_remap_index },
   { 21048, DeleteQueriesARB_remap_index },
   { 12924, EndQueryARB_remap_index },
   { 30739, GenQueriesARB_remap_index },
   {  2077, GetQueryObjectivARB_remap_index },
   { 18374, GetQueryObjectuivARB_remap_index },
   {  1856, GetQueryivARB_remap_index },
   { 21299, IsQueryARB_remap_index },
   {  8882, AttachObjectARB_remap_index },
   { 19761, CompileShaderARB_remap_index },
   {  3531, CreateProgramObjectARB_remap_index },
   {  7432, CreateShaderObjectARB_remap_index },
   { 15325, DeleteObjectARB_remap_index },
   { 25485, DetachObjectARB_remap_index },
   { 12669, GetActiveUniformARB_remap_index },
   { 10097, GetAttachedObjectsARB_remap_index },
   { 10404, GetHandleARB_remap_index },
   { 34564, GetInfoLogARB_remap_index },
   { 33632, GetObjectParameterfvARB_remap_index },
   { 28999, GetObjectParameterivARB_remap_index },
   { 30497, GetShaderSourceARB_remap_index },
   { 29721, GetUniformLocationARB_remap_index },
   { 25268, GetUniformfvARB_remap_index },
   { 13447, GetUniformivARB_remap_index },
   { 21437, LinkProgramARB_remap_index },
   { 21495, ShaderSourceARB_remap_index },
   {  8154, Uniform1fARB_remap_index },
   { 31401, Uniform1fvARB_remap_index },
   { 23463, Uniform1iARB_remap_index },
   { 22292, Uniform1ivARB_remap_index },
   {  2329, Uniform2fARB_remap_index },
   { 15161, Uniform2fvARB_remap_index },
   { 27827, Uniform2iARB_remap_index },
   {  2466, Uniform2ivARB_remap_index },
   { 19871, Uniform3fARB_remap_index },
   { 10127, Uniform3fvARB_remap_index },
   {  7043, Uniform3iARB_remap_index },
   { 17872, Uniform3ivARB_remap_index },
   { 20465, Uniform4fARB_remap_index },
   { 25132, Uniform4fvARB_remap_index },
   { 26419, Uniform4iARB_remap_index },
   { 21805, Uniform4ivARB_remap_index },
   {  8934, UniformMatrix2fvARB_remap_index },
   {    17, UniformMatrix3fvARB_remap_index },
   {  2902, UniformMatrix4fvARB_remap_index },
   { 26922, UseProgramObjectARB_remap_index },
   { 15730, ValidateProgramARB_remap_index },
   { 22697, BindAttribLocationARB_remap_index },
   {  5429, GetActiveAttribARB_remap_index },
   { 17553, GetAttribLocationARB_remap_index },
   { 31102, DrawBuffersARB_remap_index },
   { 30935, ClampColorARB_remap_index },
   { 18968, DrawArraysInstancedARB_remap_index },
   {  7104, DrawElementsInstancedARB_remap_index },
   { 13977, RenderbufferStorageMultisample_remap_index },
   { 14448, FramebufferTextureARB_remap_index },
   { 27358, FramebufferTextureFaceARB_remap_index },
   { 25634, ProgramParameteriARB_remap_index },
   {  6624, VertexAttribDivisorARB_remap_index },
   { 20513, FlushMappedBufferRange_remap_index },
   { 29436, MapBufferRange_remap_index },
   { 29343, TexBufferARB_remap_index },
   { 17358, BindVertexArray_remap_index },
   { 15534, GenVertexArrays_remap_index },
   { 31941, CopyBufferSubData_remap_index },
   { 32893, ClientWaitSync_remap_index },
   {  2821, DeleteSync_remap_index },
   {  7821, FenceSync_remap_index },
   { 16101, GetInteger64v_remap_index },
   { 24099, GetSynciv_remap_index },
   { 31041, IsSync_remap_index },
   { 10028, WaitSync_remap_index },
   {  4149, DrawElementsBaseVertex_remap_index },
   { 19616, DrawElementsInstancedBaseVertex_remap_index },
   { 32253, DrawRangeElementsBaseVertex_remap_index },
   { 27971, MultiDrawElementsBaseVertex_remap_index },
   { 33837, BlendEquationSeparateiARB_remap_index },
   { 18786, BlendEquationiARB_remap_index },
   { 13386, BlendFuncSeparateiARB_remap_index },
   { 10470, BlendFunciARB_remap_index },
   {  8796, BindSampler_remap_index },
   {  4572, DeleteSamplers_remap_index },
   { 20886, GenSamplers_remap_index },
   { 30973, GetSamplerParameterIiv_remap_index },
   { 20407, GetSamplerParameterIuiv_remap_index },
   {  5261, GetSamplerParameterfv_remap_index },
   { 27079, GetSamplerParameteriv_remap_index },
   { 15079, IsSampler_remap_index },
   { 17311, SamplerParameterIiv_remap_index },
   { 32292, SamplerParameterIuiv_remap_index },
   { 23772, SamplerParameterf_remap_index },
   { 17485, SamplerParameterfv_remap_index },
   { 23747, SamplerParameteri_remap_index },
   { 19162, SamplerParameteriv_remap_index },
   { 19969, ColorP3ui_remap_index },
   { 32540, ColorP3uiv_remap_index },
   { 22398, ColorP4ui_remap_index },
   {  4339, ColorP4uiv_remap_index },
   { 26621, MultiTexCoordP1ui_remap_index },
   { 13360, MultiTexCoordP1uiv_remap_index },
   {  1687, MultiTexCoordP2ui_remap_index },
   { 31713, MultiTexCoordP2uiv_remap_index },
   { 14691, MultiTexCoordP3ui_remap_index },
   { 11062, MultiTexCoordP3uiv_remap_index },
   { 23296, MultiTexCoordP4ui_remap_index },
   {  4011, MultiTexCoordP4uiv_remap_index },
   { 31384, NormalP3ui_remap_index },
   { 20145, NormalP3uiv_remap_index },
   { 26324, SecondaryColorP3ui_remap_index },
   { 20163, SecondaryColorP3uiv_remap_index },
   {  2802, TexCoordP1ui_remap_index },
   {  2569, TexCoordP1uiv_remap_index },
   { 15610, TexCoordP2ui_remap_index },
   { 23321, TexCoordP2uiv_remap_index },
   {  6331, TexCoordP3ui_remap_index },
   { 23700, TexCoordP3uiv_remap_index },
   {  1985, TexCoordP4ui_remap_index },
   {  5833, TexCoordP4uiv_remap_index },
   { 10513, VertexAttribP1ui_remap_index },
   { 14878, VertexAttribP1uiv_remap_index },
   { 34634, VertexAttribP2ui_remap_index },
   {  5625, VertexAttribP2uiv_remap_index },
   {  3484, VertexAttribP3ui_remap_index },
   { 20904, VertexAttribP3uiv_remap_index },
   { 14904, VertexAttribP4ui_remap_index },
   { 16886, VertexAttribP4uiv_remap_index },
   {  5367, VertexP2ui_remap_index },
   { 16456, VertexP2uiv_remap_index },
   {  2449, VertexP3ui_remap_index },
   { 17406, VertexP3uiv_remap_index },
   { 10056, VertexP4ui_remap_index },
   {  3466, VertexP4uiv_remap_index },
   {  5651, BindTransformFeedback_remap_index },
   {  3558, DeleteTransformFeedbacks_remap_index },
   {  7076, DrawTransformFeedback_remap_index },
   { 10264, GenTransformFeedbacks_remap_index },
   { 29904, IsTransformFeedback_remap_index },
   { 27551, PauseTransformFeedback_remap_index },
   {  6134, ResumeTransformFeedback_remap_index },
   { 23017, ClearDepthf_remap_index },
   {  7380, DepthRangef_remap_index },
   { 15346, GetShaderPrecisionFormat_remap_index },
   { 10683, ReleaseShaderCompiler_remap_index },
   { 11679, ShaderBinary_remap_index },
   {  1004, GetGraphicsResetStatusARB_remap_index },
   { 26199, GetnColorTableARB_remap_index },
   {  8565, GetnCompressedTexImageARB_remap_index },
   {  4288, GetnConvolutionFilterARB_remap_index },
   { 16672, GetnHistogramARB_remap_index },
   { 24958, GetnMapdvARB_remap_index },
   { 23494, GetnMapfvARB_remap_index },
   {  2308, GetnMapivARB_remap_index },
   { 18134, GetnMinmaxARB_remap_index },
   {  4886, GetnPixelMapfvARB_remap_index },
   { 20284, GetnPixelMapuivARB_remap_index },
   {  3211, GetnPixelMapusvARB_remap_index },
   {  1383, GetnPolygonStippleARB_remap_index },
   { 22482, GetnSeparableFilterARB_remap_index },
   { 34413, GetnTexImageARB_remap_index },
   { 19591, GetnUniformdvARB_remap_index },
   {  4958, GetnUniformfvARB_remap_index },
   { 10538, GetnUniformivARB_remap_index },
   { 23948, GetnUniformuivARB_remap_index },
   {  3125, ReadnPixelsARB_remap_index },
   {  6002, PolygonOffsetEXT_remap_index },
   { 24721, GetPixelTexGenParameterfvSGIS_remap_index },
   {  4836, GetPixelTexGenParameterivSGIS_remap_index },
   { 24454, PixelTexGenParameterfSGIS_remap_index },
   {   663, PixelTexGenParameterfvSGIS_remap_index },
   { 13485, PixelTexGenParameteriSGIS_remap_index },
   { 14590, PixelTexGenParameterivSGIS_remap_index },
   { 19287, SampleMaskSGIS_remap_index },
   { 21239, SamplePatternSGIS_remap_index },
   { 27900, ColorPointerEXT_remap_index },
   { 18580, EdgeFlagPointerEXT_remap_index },
   {  6697, IndexPointerEXT_remap_index },
   {  6777, NormalPointerEXT_remap_index },
   { 16740, TexCoordPointerEXT_remap_index },
   {  7610, VertexPointerEXT_remap_index },
   {  3925, PointParameterfEXT_remap_index },
   {  8461, PointParameterfvEXT_remap_index },
   { 33730, LockArraysEXT_remap_index },
   { 15794, UnlockArraysEXT_remap_index },
   {  1291, SecondaryColor3bEXT_remap_index },
   {  8654, SecondaryColor3bvEXT_remap_index },
   { 11189, SecondaryColor3dEXT_remap_index },
   { 27128, SecondaryColor3dvEXT_remap_index },
   { 29770, SecondaryColor3fEXT_remap_index },
   { 19324, SecondaryColor3fvEXT_remap_index },
   {   509, SecondaryColor3iEXT_remap_index },
   { 17076, SecondaryColor3ivEXT_remap_index },
   { 10771, SecondaryColor3sEXT_remap_index },
   { 32557, SecondaryColor3svEXT_remap_index },
   { 28835, SecondaryColor3ubEXT_remap_index },
   { 22588, SecondaryColor3ubvEXT_remap_index },
   { 13727, SecondaryColor3uiEXT_remap_index },
   { 24341, SecondaryColor3uivEXT_remap_index },
   { 27408, SecondaryColor3usEXT_remap_index },
   { 13800, SecondaryColor3usvEXT_remap_index },
   { 12548, SecondaryColorPointerEXT_remap_index },
   { 27222, MultiDrawArraysEXT_remap_index },
   { 22227, MultiDrawElementsEXT_remap_index },
   { 22438, FogCoordPointerEXT_remap_index },
   {  5009, FogCoorddEXT_remap_index },
   { 33131, FogCoorddvEXT_remap_index },
   {  5126, FogCoordfEXT_remap_index },
   { 28758, FogCoordfvEXT_remap_index },
   { 12648, PixelTexGenSGIX_remap_index },
   { 29363, BlendFuncSeparateEXT_remap_index },
   {  7522, FlushVertexArrayRangeNV_remap_index },
   {  5951, VertexArrayRangeNV_remap_index },
   { 29835, CombinerInputNV_remap_index },
   {  2251, CombinerOutputNV_remap_index },
   { 32710, CombinerParameterfNV_remap_index },
   {  5805, CombinerParameterfvNV_remap_index },
   { 23797, CombinerParameteriNV_remap_index },
   { 34213, CombinerParameterivNV_remap_index },
   {  7898, FinalCombinerInputNV_remap_index },
   {  1648, GetCombinerInputParameterfvNV_remap_index },
   { 34050, GetCombinerInputParameterivNV_remap_index },
   {   216, GetCombinerOutputParameterfvNV_remap_index },
   { 14551, GetCombinerOutputParameterivNV_remap_index },
   {  7284, GetFinalCombinerInputParameterfvNV_remap_index },
   { 26266, GetFinalCombinerInputParameterivNV_remap_index },
   { 13338, ResizeBuffersMESA_remap_index },
   { 11874, WindowPos2dMESA_remap_index },
   {  1084, WindowPos2dvMESA_remap_index },
   { 35142, WindowPos2fMESA_remap_index },
   {  8599, WindowPos2fvMESA_remap_index },
   { 19234, WindowPos2iMESA_remap_index },
   { 21712, WindowPos2ivMESA_remap_index },
   { 22326, WindowPos2sMESA_remap_index },
   {  6366, WindowPos2svMESA_remap_index },
   {  8390, WindowPos3dMESA_remap_index },
   { 14823, WindowPos3dvMESA_remap_index },
   {   555, WindowPos3fMESA_remap_index },
   { 15855, WindowPos3fvMESA_remap_index },
   { 25527, WindowPos3iMESA_remap_index },
   { 31886, WindowPos3ivMESA_remap_index },
   { 20032, WindowPos3sMESA_remap_index },
   { 33387, WindowPos3svMESA_remap_index },
   { 11825, WindowPos4dMESA_remap_index },
   { 18010, WindowPos4dvMESA_remap_index },
   { 14782, WindowPos4fMESA_remap_index },
   { 32447, WindowPos4fvMESA_remap_index },
   { 32039, WindowPos4iMESA_remap_index },
   { 13177, WindowPos4ivMESA_remap_index },
   { 20260, WindowPos4sMESA_remap_index },
   {  3509, WindowPos4svMESA_remap_index },
   { 28466, MultiModeDrawArraysIBM_remap_index },
   { 30610, MultiModeDrawElementsIBM_remap_index },
   { 12952, DeleteFencesNV_remap_index },
   { 29682, FinishFenceNV_remap_index },
   {  4073, GenFencesNV_remap_index },
   { 17990, GetFenceivNV_remap_index },
   {  8867, IsFenceNV_remap_index },
   { 14478, SetFenceNV_remap_index },
   {  4649, TestFenceNV_remap_index },
   { 33358, AreProgramsResidentNV_remap_index },
   { 32752, BindProgramNV_remap_index },
   { 27491, DeleteProgramsNV_remap_index },
   { 22806, ExecuteProgramNV_remap_index },
   { 35035, GenProgramsNV_remap_index },
   { 24826, GetProgramParameterdvNV_remap_index },
   { 11251, GetProgramParameterfvNV_remap_index },
   { 27874, GetProgramStringNV_remap_index },
   { 25904, GetProgramivNV_remap_index },
   { 25081, GetTrackMatrixivNV_remap_index },
   { 27668, GetVertexAttribPointervNV_remap_index },
   { 10866, GetVertexAttribdvNV_remap_index },
   {  9923, GetVertexAttribfvNV_remap_index },
   { 19537, GetVertexAttribivNV_remap_index },
   { 20543, IsProgramNV_remap_index },
   { 10006, LoadProgramNV_remap_index },
   { 29459, ProgramParameters4dvNV_remap_index },
   { 25834, ProgramParameters4fvNV_remap_index },
   { 22016, RequestResidentProgramsNV_remap_index },
   { 32231, TrackMatrixNV_remap_index },
   { 34027, VertexAttrib1dNV_remap_index },
   { 14389, VertexAttrib1dvNV_remap_index },
   { 30116, VertexAttrib1fNV_remap_index },
   {  2608, VertexAttrib1fvNV_remap_index },
   { 32504, VertexAttrib1sNV_remap_index },
   { 15928, VertexAttrib1svNV_remap_index },
   {  5317, VertexAttrib2dNV_remap_index },
   { 14282, VertexAttrib2dvNV_remap_index },
   { 21471, VertexAttrib2fNV_remap_index },
   { 13848, VertexAttrib2fvNV_remap_index },
   {  6578, VertexAttrib2sNV_remap_index },
   { 20086, VertexAttrib2svNV_remap_index },
   { 12022, VertexAttrib3dNV_remap_index },
   { 33608, VertexAttrib3dvNV_remap_index },
   { 11037, VertexAttrib3fNV_remap_index },
   { 26226, VertexAttrib3fvNV_remap_index },
   { 23570, VertexAttrib3sNV_remap_index },
   { 25108, VertexAttrib3svNV_remap_index },
   { 30584, VertexAttrib4dNV_remap_index },
   { 35072, VertexAttrib4dvNV_remap_index },
   {  5745, VertexAttrib4fNV_remap_index },
   { 10073, VertexAttrib4fvNV_remap_index },
   { 28350, VertexAttrib4sNV_remap_index },
   {  1461, VertexAttrib4svNV_remap_index },
   {  5492, VertexAttrib4ubNV_remap_index },
   {   817, VertexAttrib4ubvNV_remap_index },
   { 22986, VertexAttribPointerNV_remap_index },
   {  2423, VertexAttribs1dvNV_remap_index },
   { 27756, VertexAttribs1fvNV_remap_index },
   { 34872, VertexAttribs1svNV_remap_index },
   { 11088, VertexAttribs2dvNV_remap_index },
   { 26883, VertexAttribs2fvNV_remap_index },
   { 18606, VertexAttribs2svNV_remap_index },
   {  5853, VertexAttribs3dvNV_remap_index },
   {  2282, VertexAttribs3fvNV_remap_index },
   { 31608, VertexAttribs3svNV_remap_index },
   { 28440, VertexAttribs4dvNV_remap_index },
   {  5925, VertexAttribs4fvNV_remap_index },
   { 34659, VertexAttribs4svNV_remap_index },
   { 31339, VertexAttribs4ubvNV_remap_index },
   { 28542, GetTexBumpParameterfvATI_remap_index },
   { 34913, GetTexBumpParameterivATI_remap_index },
   { 19714, TexBumpParameterfvATI_remap_index },
   { 21887, TexBumpParameterivATI_remap_index },
   { 16492, AlphaFragmentOp1ATI_remap_index },
   { 27174, AlphaFragmentOp2ATI_remap_index },
   { 26142, AlphaFragmentOp3ATI_remap_index },
   { 31535, BeginFragmentShaderATI_remap_index },
   { 32951, BindFragmentShaderATI_remap_index },
   { 25237, ColorFragmentOp1ATI_remap_index },
   {  4764, ColorFragmentOp2ATI_remap_index },
   { 33253, ColorFragmentOp3ATI_remap_index },
   {  6091, DeleteFragmentShaderATI_remap_index },
   { 35096, EndFragmentShaderATI_remap_index },
   { 34241, GenFragmentShadersATI_remap_index },
   { 27037, PassTexCoordATI_remap_index },
   {  7590, SampleMapATI_remap_index },
   { 28653, SetFragmentShaderConstantATI_remap_index },
   {   402, PointParameteriNV_remap_index },
   { 15035, PointParameterivNV_remap_index },
   { 30423, ActiveStencilFaceEXT_remap_index },
   { 29099, BindVertexArrayAPPLE_remap_index },
   {  2949, DeleteVertexArraysAPPLE_remap_index },
   { 19073, GenVertexArraysAPPLE_remap_index },
   { 24891, IsVertexArrayAPPLE_remap_index },
   {   858, GetProgramNamedParameterdvNV_remap_index },
   {  3888, GetProgramNamedParameterfvNV_remap_index },
   { 28573, ProgramNamedParameter4dNV_remap_index },
   { 15409, ProgramNamedParameter4dvNV_remap_index },
   {  9539, ProgramNamedParameter4fNV_remap_index },
   { 12513, ProgramNamedParameter4fvNV_remap_index },
   { 17921, PrimitiveRestartIndexNV_remap_index },
   { 32424, PrimitiveRestartNV_remap_index },
   { 25813, DepthBoundsEXT_remap_index },
   {  1183, BlendEquationSeparateEXT_remap_index },
   { 15629, BindFramebufferEXT_remap_index },
   { 27267, BindRenderbufferEXT_remap_index },
   { 10320, CheckFramebufferStatusEXT_remap_index },
   { 24142, DeleteFramebuffersEXT_remap_index },
   { 33510, DeleteRenderbuffersEXT_remap_index },
   { 14306, FramebufferRenderbufferEXT_remap_index },
   { 14495, FramebufferTexture1DEXT_remap_index },
   { 12307, FramebufferTexture2DEXT_remap_index },
   { 11927, FramebufferTexture3DEXT_remap_index },
   { 24757, GenFramebuffersEXT_remap_index },
   { 18471, GenRenderbuffersEXT_remap_index },
   {  7326, GenerateMipmapEXT_remap_index },
   { 23079, GetFramebufferAttachmentParameterivEXT_remap_index },
   { 34147, GetRenderbufferParameterivEXT_remap_index },
   { 21767, IsFramebufferEXT_remap_index },
   { 34995, IsRenderbufferEXT_remap_index },
   {  8814, RenderbufferStorageEXT_remap_index },
   {   734, BlitFramebufferEXT_remap_index },
   { 15195, BufferParameteriAPPLE_remap_index },
   { 20575, FlushMappedBufferRangeAPPLE_remap_index },
   {  1931, BindFragDataLocationEXT_remap_index },
   { 25926, GetFragDataLocationEXT_remap_index },
   { 11366, GetUniformuivEXT_remap_index },
   { 34321, GetVertexAttribIivEXT_remap_index },
   { 29630, GetVertexAttribIuivEXT_remap_index },
   { 12785, Uniform1uiEXT_remap_index },
   { 29544, Uniform1uivEXT_remap_index },
   { 23666, Uniform2uiEXT_remap_index },
   {  4728, Uniform2uivEXT_remap_index },
   { 30863, Uniform3uiEXT_remap_index },
   { 15556, Uniform3uivEXT_remap_index },
   {  3812, Uniform4uiEXT_remap_index },
   {  9315, Uniform4uivEXT_remap_index },
   { 19452, VertexAttribI1iEXT_remap_index },
   {  5519, VertexAttribI1ivEXT_remap_index },
   {  2709, VertexAttribI1uiEXT_remap_index },
   { 13576, VertexAttribI1uivEXT_remap_index },
   {    81, VertexAttribI2iEXT_remap_index },
   { 25349, VertexAttribI2ivEXT_remap_index },
   {  5879, VertexAttribI2uiEXT_remap_index },
   {  5171, VertexAttribI2uivEXT_remap_index },
   { 28142, VertexAttribI3iEXT_remap_index },
   { 26576, VertexAttribI3ivEXT_remap_index },
   {  3666, VertexAttribI3uiEXT_remap_index },
   { 32127, VertexAttribI3uivEXT_remap_index },
   { 23375, VertexAttribI4bvEXT_remap_index },
   { 15488, VertexAttribI4iEXT_remap_index },
   { 33779, VertexAttribI4ivEXT_remap_index },
   { 14209, VertexAttribI4svEXT_remap_index },
   {  4241, VertexAttribI4ubvEXT_remap_index },
   { 17139, VertexAttribI4uiEXT_remap_index },
   {  6025, VertexAttribI4uivEXT_remap_index },
   { 12090, VertexAttribI4usvEXT_remap_index },
   { 19658, VertexAttribIPointerEXT_remap_index },
   {  3281, FramebufferTextureLayerEXT_remap_index },
   { 17511, ColorMaskIndexedEXT_remap_index },
   { 20110, DisableIndexedEXT_remap_index },
   { 28187, EnableIndexedEXT_remap_index },
   { 23034, GetBooleanIndexedvEXT_remap_index },
   { 11701, GetIntegerIndexedvEXT_remap_index },
   { 24218, IsEnabledIndexedEXT_remap_index },
   { 24118, ClearColorIiEXT_remap_index },
   {  3762, ClearColorIuiEXT_remap_index },
   { 10563, GetTexParameterIivEXT_remap_index },
   {  6526, GetTexParameterIuivEXT_remap_index },
   {  3237, TexParameterIivEXT_remap_index },
   { 28009, TexParameterIuivEXT_remap_index },
   {  5039, BeginConditionalRenderNV_remap_index },
   { 26987, EndConditionalRenderNV_remap_index },
   {  9950, BeginTransformFeedbackEXT_remap_index },
   { 20189, BindBufferBaseEXT_remap_index },
   { 20004, BindBufferOffsetEXT_remap_index },
   { 13002, BindBufferRangeEXT_remap_index },
   { 15110, EndTransformFeedbackEXT_remap_index },
   { 11564, GetTransformFeedbackVaryingEXT_remap_index },
   { 22072, TransformFeedbackVaryingsEXT_remap_index },
   { 31240, ProvokingVertexEXT_remap_index },
   { 11512, GetTexParameterPointervAPPLE_remap_index },
   {  5581, TextureRangeAPPLE_remap_index },
   { 12379, GetObjectParameterivAPPLE_remap_index },
   { 21194, ObjectPurgeableAPPLE_remap_index },
   {  6301, ObjectUnpurgeableAPPLE_remap_index },
   { 18293, ActiveProgramEXT_remap_index },
   { 18264, CreateShaderProgramEXT_remap_index },
   { 30208, UseShaderProgramEXT_remap_index },
   { 17464, TextureBarrierNV_remap_index },
   { 30449, StencilFuncSeparateATI_remap_index },
   {  6214, ProgramEnvParameters4fvEXT_remap_index },
   { 18158, ProgramLocalParameters4fvEXT_remap_index },
   { 14963, GetQueryObjecti64vEXT_remap_index },
   { 11114, GetQueryObjectui64vEXT_remap_index },
   { 25306, EGLImageTargetRenderbufferStorageOES_remap_index },
   { 12891, EGLImageTargetTexture2DOES_remap_index },
   {    -1, -1 }
};

/* these functions are in the ABI, but have alternative names */
static const struct gl_function_remap MESA_alt_functions[] = {
   /* from GL_EXT_blend_color */
   {  2867, _gloffset_BlendColor },
   /* from GL_EXT_blend_minmax */
   { 11984, _gloffset_BlendEquation },
   /* from GL_EXT_color_subtable */
   { 18032, _gloffset_ColorSubTable },
   { 33442, _gloffset_CopyColorSubTable },
   /* from GL_EXT_convolution */
   {   296, _gloffset_ConvolutionFilter1D },
   {  2647, _gloffset_CopyConvolutionFilter1D },
   {  4508, _gloffset_GetConvolutionParameteriv },
   {  9163, _gloffset_ConvolutionFilter2D },
   {  9365, _gloffset_ConvolutionParameteriv },
   {  9825, _gloffset_ConvolutionParameterfv },
   { 21915, _gloffset_GetSeparableFilter },
   { 25581, _gloffset_SeparableFilter2D },
   { 26469, _gloffset_ConvolutionParameteri },
   { 26662, _gloffset_ConvolutionParameterf },
   { 28376, _gloffset_GetConvolutionParameterfv },
   { 29265, _gloffset_GetConvolutionFilter },
   { 31823, _gloffset_CopyConvolutionFilter2D },
   /* from GL_EXT_copy_texture */
   { 15988, _gloffset_CopyTexSubImage3D },
   { 17719, _gloffset_CopyTexImage2D },
   { 26052, _gloffset_CopyTexImage1D },
   { 28946, _gloffset_CopyTexSubImage2D },
   { 31435, _gloffset_CopyTexSubImage1D },
   /* from GL_EXT_draw_range_elements */
   { 10179, _gloffset_DrawRangeElements },
   /* from GL_EXT_histogram */
   {   895, _gloffset_Histogram },
   {  3848, _gloffset_ResetHistogram },
   { 10709, _gloffset_GetMinmax },
   { 16322, _gloffset_GetHistogramParameterfv },
   { 25977, _gloffset_GetMinmaxParameteriv },
   { 28266, _gloffset_ResetMinmax },
   { 29162, _gloffset_GetHistogramParameteriv },
   { 30383, _gloffset_GetHistogram },
   { 33067, _gloffset_Minmax },
   { 34742, _gloffset_GetMinmaxParameterfv },
   /* from GL_EXT_paletted_texture */
   {  9025, _gloffset_ColorTable },
   { 16168, _gloffset_GetColorTable },
   { 24504, _gloffset_GetColorTableParameterfv },
   { 26718, _gloffset_GetColorTableParameteriv },
   /* from GL_EXT_subtexture */
   {  7736, _gloffset_TexSubImage1D },
   { 11439, _gloffset_TexSubImage2D },
   /* from GL_EXT_texture3D */
   {  1890, _gloffset_TexImage3D },
   { 24273, _gloffset_TexSubImage3D },
   /* from GL_EXT_texture_object */
   {  3616, _gloffset_PrioritizeTextures },
   {  8185, _gloffset_AreTexturesResident },
   { 14413, _gloffset_GenTextures },
   { 16699, _gloffset_DeleteTextures },
   { 20856, _gloffset_IsTexture },
   { 31500, _gloffset_BindTexture },
   /* from GL_EXT_vertex_array */
   { 25753, _gloffset_ArrayElement },
   { 32655, _gloffset_GetPointerv },
   { 34268, _gloffset_DrawArrays },
   /* from GL_SGI_color_table */
   {  8303, _gloffset_ColorTableParameteriv },
   {  9025, _gloffset_ColorTable },
   { 16168, _gloffset_GetColorTable },
   { 16278, _gloffset_CopyColorTable },
   { 20717, _gloffset_ColorTableParameterfv },
   { 24504, _gloffset_GetColorTableParameterfv },
   { 26718, _gloffset_GetColorTableParameteriv },
   /* from GL_VERSION_1_3 */
   {   464, _gloffset_MultiTexCoord3sARB },
   {   696, _gloffset_ActiveTextureARB },
   {  4666, _gloffset_MultiTexCoord1fvARB },
   {  6802, _gloffset_MultiTexCoord3dARB },
   {  6847, _gloffset_MultiTexCoord2iARB },
   {  6971, _gloffset_MultiTexCoord2svARB },
   {  8981, _gloffset_MultiTexCoord2fARB },
   { 11144, _gloffset_MultiTexCoord3fvARB },
   { 11746, _gloffset_MultiTexCoord4sARB },
   { 12427, _gloffset_MultiTexCoord2dvARB },
   { 12834, _gloffset_MultiTexCoord1svARB },
   { 13199, _gloffset_MultiTexCoord3svARB },
   { 13260, _gloffset_MultiTexCoord4iARB },
   { 14117, _gloffset_MultiTexCoord3iARB },
   { 14992, _gloffset_MultiTexCoord1dARB },
   { 15224, _gloffset_MultiTexCoord3dvARB },
   { 16540, _gloffset_MultiTexCoord3ivARB },
   { 16585, _gloffset_MultiTexCoord2sARB },
   { 18089, _gloffset_MultiTexCoord4ivARB },
   { 20357, _gloffset_ClientActiveTextureARB },
   { 22762, _gloffset_MultiTexCoord2dARB },
   { 23199, _gloffset_MultiTexCoord4dvARB },
   { 23621, _gloffset_MultiTexCoord4fvARB },
   { 24645, _gloffset_MultiTexCoord3fARB },
   { 27312, _gloffset_MultiTexCoord4dARB },
   { 27578, _gloffset_MultiTexCoord1sARB },
   { 27782, _gloffset_MultiTexCoord1dvARB },
   { 28790, _gloffset_MultiTexCoord1ivARB },
   { 28883, _gloffset_MultiTexCoord2ivARB },
   { 29222, _gloffset_MultiTexCoord1iARB },
   { 30658, _gloffset_MultiTexCoord4svARB },
   { 31282, _gloffset_MultiTexCoord1fARB },
   { 31562, _gloffset_MultiTexCoord4fARB },
   { 34102, _gloffset_MultiTexCoord2fvARB },
   {    -1, -1 }
};

#endif /* need_MESA_remap_table */

#if defined(need_GL_3DFX_tbuffer)
static const struct gl_function_remap GL_3DFX_tbuffer_functions[] = {
   {  9883, -1 }, /* TbufferMask3DFX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_AMD_draw_buffers_blend)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_AMD_draw_buffers_blend_functions[] = {
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
   {  4125, -1 }, /* MatrixIndexusvARB */
   { 13938, -1 }, /* MatrixIndexuivARB */
   { 15379, -1 }, /* MatrixIndexPointerARB */
   { 21149, -1 }, /* CurrentPaletteMatrixARB */
   { 24389, -1 }, /* MatrixIndexubvARB */
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

#if defined(need_GL_ARB_robustness)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_robustness_functions[] = {
   {    -1, -1 }
};
#endif

#if defined(need_GL_ARB_sampler_objects)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_sampler_objects_functions[] = {
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
   {  2589, -1 }, /* WeightubvARB */
   {  7214, -1 }, /* WeightivARB */
   { 11849, -1 }, /* WeightPointerARB */
   { 14673, -1 }, /* WeightfvARB */
   { 18632, -1 }, /* WeightbvARB */
   { 22379, -1 }, /* WeightusvARB */
   { 25507, -1 }, /* VertexBlendARB */
   { 31366, -1 }, /* WeightsvARB */
   { 33492, -1 }, /* WeightdvARB */
   { 34302, -1 }, /* WeightuivARB */
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

#if defined(need_GL_ARB_vertex_type_2_10_10_10_rev)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_ARB_vertex_type_2_10_10_10_rev_functions[] = {
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
   {  2867, _gloffset_BlendColor },
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
   { 11984, _gloffset_BlendEquation },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_color_subtable)
static const struct gl_function_remap GL_EXT_color_subtable_functions[] = {
   { 18032, _gloffset_ColorSubTable },
   { 33442, _gloffset_CopyColorSubTable },
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
   {   296, _gloffset_ConvolutionFilter1D },
   {  2647, _gloffset_CopyConvolutionFilter1D },
   {  4508, _gloffset_GetConvolutionParameteriv },
   {  9163, _gloffset_ConvolutionFilter2D },
   {  9365, _gloffset_ConvolutionParameteriv },
   {  9825, _gloffset_ConvolutionParameterfv },
   { 21915, _gloffset_GetSeparableFilter },
   { 25581, _gloffset_SeparableFilter2D },
   { 26469, _gloffset_ConvolutionParameteri },
   { 26662, _gloffset_ConvolutionParameterf },
   { 28376, _gloffset_GetConvolutionParameterfv },
   { 29265, _gloffset_GetConvolutionFilter },
   { 31823, _gloffset_CopyConvolutionFilter2D },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_coordinate_frame)
static const struct gl_function_remap GL_EXT_coordinate_frame_functions[] = {
   { 11283, -1 }, /* TangentPointerEXT */
   { 13318, -1 }, /* Binormal3ivEXT */
   { 14070, -1 }, /* Tangent3sEXT */
   { 15444, -1 }, /* Tangent3fvEXT */
   { 19742, -1 }, /* Tangent3ivEXT */
   { 19985, -1 }, /* Tangent3dvEXT */
   { 20803, -1 }, /* Binormal3bvEXT */
   { 21968, -1 }, /* Binormal3dEXT */
   { 24321, -1 }, /* Tangent3fEXT */
   { 26541, -1 }, /* Binormal3sEXT */
   { 27108, -1 }, /* Tangent3dEXT */
   { 28055, -1 }, /* Binormal3svEXT */
   { 28688, -1 }, /* Binormal3fEXT */
   { 29596, -1 }, /* Binormal3dvEXT */
   { 30915, -1 }, /* Tangent3iEXT */
   { 32108, -1 }, /* Tangent3bvEXT */
   { 32690, -1 }, /* Tangent3bEXT */
   { 33215, -1 }, /* Binormal3fvEXT */
   { 34001, -1 }, /* BinormalPointerEXT */
   { 34482, -1 }, /* Tangent3svEXT */
   { 34944, -1 }, /* Binormal3bEXT */
   { 35121, -1 }, /* Binormal3iEXT */
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_copy_texture)
static const struct gl_function_remap GL_EXT_copy_texture_functions[] = {
   { 15988, _gloffset_CopyTexSubImage3D },
   { 17719, _gloffset_CopyTexImage2D },
   { 26052, _gloffset_CopyTexImage1D },
   { 28946, _gloffset_CopyTexSubImage2D },
   { 31435, _gloffset_CopyTexSubImage1D },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_cull_vertex)
static const struct gl_function_remap GL_EXT_cull_vertex_functions[] = {
   {  9514, -1 }, /* CullParameterdvEXT */
   { 12472, -1 }, /* CullParameterfvEXT */
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
   { 10179, _gloffset_DrawRangeElements },
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
   {   895, _gloffset_Histogram },
   {  3848, _gloffset_ResetHistogram },
   { 10709, _gloffset_GetMinmax },
   { 16322, _gloffset_GetHistogramParameterfv },
   { 25977, _gloffset_GetMinmaxParameteriv },
   { 28266, _gloffset_ResetMinmax },
   { 29162, _gloffset_GetHistogramParameteriv },
   { 30383, _gloffset_GetHistogram },
   { 33067, _gloffset_Minmax },
   { 34742, _gloffset_GetMinmaxParameterfv },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_index_func)
static const struct gl_function_remap GL_EXT_index_func_functions[] = {
   { 12258, -1 }, /* IndexFuncEXT */
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_index_material)
static const struct gl_function_remap GL_EXT_index_material_functions[] = {
   { 22517, -1 }, /* IndexMaterialEXT */
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_light_texture)
static const struct gl_function_remap GL_EXT_light_texture_functions[] = {
   { 28075, -1 }, /* ApplyTextureEXT */
   { 28220, -1 }, /* TextureMaterialEXT */
   { 28245, -1 }, /* TextureLightEXT */
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
   {  9025, _gloffset_ColorTable },
   { 16168, _gloffset_GetColorTable },
   { 24504, _gloffset_GetColorTableParameterfv },
   { 26718, _gloffset_GetColorTableParameteriv },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_pixel_transform)
static const struct gl_function_remap GL_EXT_pixel_transform_functions[] = {
   { 23164, -1 }, /* PixelTransformParameterfEXT */
   { 23244, -1 }, /* PixelTransformParameteriEXT */
   { 32388, -1 }, /* PixelTransformParameterfvEXT */
   { 33965, -1 }, /* PixelTransformParameterivEXT */
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
   {  7736, _gloffset_TexSubImage1D },
   { 11439, _gloffset_TexSubImage2D },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_texture3D)
static const struct gl_function_remap GL_EXT_texture3D_functions[] = {
   {  1890, _gloffset_TexImage3D },
   { 24273, _gloffset_TexSubImage3D },
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
   {  3616, _gloffset_PrioritizeTextures },
   {  8185, _gloffset_AreTexturesResident },
   { 14413, _gloffset_GenTextures },
   { 16699, _gloffset_DeleteTextures },
   { 20856, _gloffset_IsTexture },
   { 31500, _gloffset_BindTexture },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_texture_perturb_normal)
static const struct gl_function_remap GL_EXT_texture_perturb_normal_functions[] = {
   { 14623, -1 }, /* TextureNormalEXT */
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
   { 25753, _gloffset_ArrayElement },
   { 32655, _gloffset_GetPointerv },
   { 34268, _gloffset_DrawArrays },
   {    -1, -1 }
};
#endif

#if defined(need_GL_EXT_vertex_weighting)
static const struct gl_function_remap GL_EXT_vertex_weighting_functions[] = {
   { 20930, -1 }, /* VertexWeightfvEXT */
   { 28631, -1 }, /* VertexWeightfEXT */
   { 30352, -1 }, /* VertexWeightPointerEXT */
   {    -1, -1 }
};
#endif

#if defined(need_GL_HP_image_transform)
static const struct gl_function_remap GL_HP_image_transform_functions[] = {
   {  2500, -1 }, /* GetImageTransformParameterfvHP */
   {  4091, -1 }, /* ImageTransformParameterfHP */
   { 10951, -1 }, /* ImageTransformParameterfvHP */
   { 12719, -1 }, /* ImageTransformParameteriHP */
   { 13089, -1 }, /* GetImageTransformParameterivHP */
   { 20994, -1 }, /* ImageTransformParameterivHP */
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
   {  4798, -1 }, /* SecondaryColorPointerListIBM */
   {  6668, -1 }, /* NormalPointerListIBM */
   {  8359, -1 }, /* FogCoordPointerListIBM */
   {  8700, -1 }, /* VertexPointerListIBM */
   { 12973, -1 }, /* ColorPointerListIBM */
   { 14177, -1 }, /* TexCoordPointerListIBM */
   { 14645, -1 }, /* IndexPointerListIBM */
   { 34685, -1 }, /* EdgeFlagPointerListIBM */
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
   { 13517, -1 }, /* VertexPointervINTEL */
   { 16415, -1 }, /* ColorPointervINTEL */
   { 31797, -1 }, /* NormalPointervINTEL */
   { 32320, -1 }, /* TexCoordPointervINTEL */
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
   {  1754, -1 }, /* GetDebugLogLengthMESA */
   {  3787, -1 }, /* ClearDebugLogMESA */
   {  4983, -1 }, /* GetDebugLogMESA */
   { 32848, -1 }, /* CreateDebugObjectMESA */
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
   {  7398, -1 }, /* GetMapAttribParameterivNV */
   {  9131, -1 }, /* MapControlPointsNV */
   {  9230, -1 }, /* MapParameterfvNV */
   { 11422, -1 }, /* EvalMapsNV */
   { 18230, -1 }, /* GetMapAttribParameterfvNV */
   { 18447, -1 }, /* MapParameterivNV */
   { 26392, -1 }, /* GetMapParameterivNV */
   { 26960, -1 }, /* GetMapParameterfvNV */
   { 31069, -1 }, /* GetMapControlPointsNV */
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
   { 16852, -1 }, /* CombinerStageParameterfvNV */
   { 17274, -1 }, /* GetCombinerStageParameterfvNV */
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
   {  9351, -1 }, /* HintPGI */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_detail_texture)
static const struct gl_function_remap GL_SGIS_detail_texture_functions[] = {
   { 17247, -1 }, /* GetDetailTexFuncSGIS */
   { 17664, -1 }, /* DetailTexFuncSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_fog_function)
static const struct gl_function_remap GL_SGIS_fog_function_functions[] = {
   { 28928, -1 }, /* FogFuncSGIS */
   { 29701, -1 }, /* GetFogFuncSGIS */
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
   {  7459, -1 }, /* GetSharpenTexFuncSGIS */
   { 23595, -1 }, /* SharpenTexFuncSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_texture4D)
static const struct gl_function_remap GL_SGIS_texture4D_functions[] = {
   {  1034, -1 }, /* TexImage4DSGIS */
   { 16768, -1 }, /* TexSubImage4DSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_texture_color_mask)
static const struct gl_function_remap GL_SGIS_texture_color_mask_functions[] = {
   { 16121, -1 }, /* TextureColorMaskSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIS_texture_filter4)
static const struct gl_function_remap GL_SGIS_texture_filter4_functions[] = {
   {  7636, -1 }, /* GetTexFilterFuncSGIS */
   { 17438, -1 }, /* TexFilterFuncSGIS */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_async)
static const struct gl_function_remap GL_SGIX_async_functions[] = {
   {  3713, -1 }, /* AsyncMarkerSGIX */
   {  4937, -1 }, /* FinishAsyncSGIX */
   {  6072, -1 }, /* PollAsyncSGIX */
   { 23824, -1 }, /* DeleteAsyncMarkersSGIX */
   { 23879, -1 }, /* IsAsyncMarkerSGIX */
   { 34457, -1 }, /* GenAsyncMarkersSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_flush_raster)
static const struct gl_function_remap GL_SGIX_flush_raster_functions[] = {
   {  8013, -1 }, /* FlushRasterSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_fragment_lighting)
static const struct gl_function_remap GL_SGIX_fragment_lighting_functions[] = {
   {  2837, -1 }, /* FragmentMaterialfvSGIX */
   {  5976, -1 }, /* FragmentLightiSGIX */
   {  8767, -1 }, /* FragmentMaterialfSGIX */
   {  8904, -1 }, /* GetFragmentLightivSGIX */
   {  9777, -1 }, /* FragmentLightModeliSGIX */
   { 11485, -1 }, /* FragmentLightivSGIX */
   { 11792, -1 }, /* GetFragmentMaterialivSGIX */
   { 17187, -1 }, /* GetFragmentMaterialfvSGIX */
   { 20773, -1 }, /* FragmentLightModelfSGIX */
   { 21117, -1 }, /* FragmentColorMaterialSGIX */
   { 21534, -1 }, /* FragmentMaterialiSGIX */
   { 22845, -1 }, /* LightEnviSGIX */
   { 24596, -1 }, /* FragmentLightModelfvSGIX */
   { 24931, -1 }, /* FragmentLightfvSGIX */
   { 30085, -1 }, /* FragmentLightModelivSGIX */
   { 30234, -1 }, /* FragmentLightfSGIX */
   { 33185, -1 }, /* GetFragmentLightfvSGIX */
   { 34965, -1 }, /* FragmentMaterialivSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_framezoom)
static const struct gl_function_remap GL_SGIX_framezoom_functions[] = {
   { 23902, -1 }, /* FrameZoomSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_igloo_interface)
static const struct gl_function_remap GL_SGIX_igloo_interface_functions[] = {
   { 30542, -1 }, /* IglooInterfaceSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_instruments)
static const struct gl_function_remap GL_SGIX_instruments_functions[] = {
   {  3017, -1 }, /* ReadInstrumentsSGIX */
   {  7232, -1 }, /* PollInstrumentsSGIX */
   { 11343, -1 }, /* GetInstrumentsSGIX */
   { 13775, -1 }, /* StartInstrumentsSGIX */
   { 16912, -1 }, /* StopInstrumentsSGIX */
   { 18895, -1 }, /* InstrumentsBufferSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_list_priority)
static const struct gl_function_remap GL_SGIX_list_priority_functions[] = {
   {  1265, -1 }, /* ListParameterfSGIX */
   {  3372, -1 }, /* GetListParameterfvSGIX */
   { 18760, -1 }, /* ListParameteriSGIX */
   { 19919, -1 }, /* ListParameterfvSGIX */
   { 22200, -1 }, /* ListParameterivSGIX */
   { 34501, -1 }, /* GetListParameterivSGIX */
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
   {  3059, -1 }, /* DeformationMap3dSGIX */
   {  4037, -1 }, /* LoadIdentityDeformationMapSGIX */
   { 17012, -1 }, /* DeformSGIX */
   { 25865, -1 }, /* DeformationMap3fSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_reference_plane)
static const struct gl_function_remap GL_SGIX_reference_plane_functions[] = {
   { 15672, -1 }, /* ReferencePlaneSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_sprite)
static const struct gl_function_remap GL_SGIX_sprite_functions[] = {
   { 10292, -1 }, /* SpriteParameterfvSGIX */
   { 21989, -1 }, /* SpriteParameteriSGIX */
   { 28300, -1 }, /* SpriteParameterfSGIX */
   { 31212, -1 }, /* SpriteParameterivSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGIX_tag_sample_buffer)
static const struct gl_function_remap GL_SGIX_tag_sample_buffer_functions[] = {
   { 22048, -1 }, /* TagSampleBufferSGIX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SGI_color_table)
static const struct gl_function_remap GL_SGI_color_table_functions[] = {
   {  8303, _gloffset_ColorTableParameteriv },
   {  9025, _gloffset_ColorTable },
   { 16168, _gloffset_GetColorTable },
   { 16278, _gloffset_CopyColorTable },
   { 20717, _gloffset_ColorTableParameterfv },
   { 24504, _gloffset_GetColorTableParameterfv },
   { 26718, _gloffset_GetColorTableParameteriv },
   {    -1, -1 }
};
#endif

#if defined(need_GL_SUNX_constant_data)
static const struct gl_function_remap GL_SUNX_constant_data_functions[] = {
   { 33163, -1 }, /* FinishTextureSUNX */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SUN_global_alpha)
static const struct gl_function_remap GL_SUN_global_alpha_functions[] = {
   {  3734, -1 }, /* GlobalAlphaFactorubSUN */
   {  5290, -1 }, /* GlobalAlphaFactoriSUN */
   {  7257, -1 }, /* GlobalAlphaFactordSUN */
   { 10376, -1 }, /* GlobalAlphaFactoruiSUN */
   { 10908, -1 }, /* GlobalAlphaFactorbSUN */
   { 14090, -1 }, /* GlobalAlphaFactorfSUN */
   { 14254, -1 }, /* GlobalAlphaFactorusSUN */
   { 24191, -1 }, /* GlobalAlphaFactorsSUN */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SUN_mesh_array)
static const struct gl_function_remap GL_SUN_mesh_array_functions[] = {
   { 31003, -1 }, /* DrawMeshArraysSUN */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SUN_triangle_list)
static const struct gl_function_remap GL_SUN_triangle_list_functions[] = {
   {  4911, -1 }, /* ReplacementCodeubSUN */
   {  7016, -1 }, /* ReplacementCodeubvSUN */
   { 20438, -1 }, /* ReplacementCodeusvSUN */
   { 20626, -1 }, /* ReplacementCodePointerSUN */
   { 22909, -1 }, /* ReplacementCodeuiSUN */
   { 23853, -1 }, /* ReplacementCodeusSUN */
   { 31686, -1 }, /* ReplacementCodeuivSUN */
   {    -1, -1 }
};
#endif

#if defined(need_GL_SUN_vertex)
static const struct gl_function_remap GL_SUN_vertex_functions[] = {
   {  1139, -1 }, /* ReplacementCodeuiColor3fVertex3fvSUN */
   {  1337, -1 }, /* TexCoord4fColor4fNormal3fVertex4fvSUN */
   {  1591, -1 }, /* TexCoord2fColor4ubVertex3fvSUN */
   {  2004, -1 }, /* ReplacementCodeuiVertex3fvSUN */
   {  2138, -1 }, /* ReplacementCodeuiTexCoord2fVertex3fvSUN */
   {  2754, -1 }, /* ReplacementCodeuiNormal3fVertex3fSUN */
   {  3152, -1 }, /* Color4ubVertex3fvSUN */
   {  5095, -1 }, /* Color4ubVertex3fSUN */
   {  5218, -1 }, /* TexCoord2fVertex3fSUN */
   {  5679, -1 }, /* TexCoord2fColor4fNormal3fVertex3fSUN */
   {  6176, -1 }, /* TexCoord2fNormal3fVertex3fvSUN */
   {  6911, -1 }, /* ReplacementCodeuiTexCoord2fNormal3fVertex3fSUN */
   {  7691, -1 }, /* ReplacementCodeuiColor4ubVertex3fvSUN */
   {  8050, -1 }, /* ReplacementCodeuiTexCoord2fVertex3fSUN */
   {  9576, -1 }, /* Color3fVertex3fSUN */
   { 10817, -1 }, /* Color3fVertex3fvSUN */
   { 11308, -1 }, /* Color4fNormal3fVertex3fvSUN */
   { 12137, -1 }, /* ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN */
   { 13638, -1 }, /* ReplacementCodeuiColor4fNormal3fVertex3fvSUN */
   { 15269, -1 }, /* ReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN */
   { 15814, -1 }, /* TexCoord2fColor3fVertex3fSUN */
   { 16937, -1 }, /* TexCoord4fColor4fNormal3fVertex4fSUN */
   { 17379, -1 }, /* Color4ubVertex2fvSUN */
   { 17689, -1 }, /* Normal3fVertex3fSUN */
   { 18836, -1 }, /* ReplacementCodeuiColor4fNormal3fVertex3fSUN */
   { 19188, -1 }, /* TexCoord2fColor4fNormal3fVertex3fvSUN */
   { 19495, -1 }, /* TexCoord2fNormal3fVertex3fSUN */
   { 20231, -1 }, /* TexCoord2fVertex3fvSUN */
   { 21087, -1 }, /* Color4ubVertex2fSUN */
   { 21325, -1 }, /* ReplacementCodeuiColor4ubVertex3fSUN */
   { 23420, -1 }, /* TexCoord2fColor4ubVertex3fSUN */
   { 23921, -1 }, /* Normal3fVertex3fvSUN */
   { 24413, -1 }, /* Color4fNormal3fVertex3fSUN */
   { 25414, -1 }, /* ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN */
   { 27621, -1 }, /* ReplacementCodeuiColor3fVertex3fSUN */
   { 29044, -1 }, /* TexCoord4fVertex4fSUN */
   { 29490, -1 }, /* TexCoord2fColor3fVertex3fvSUN */
   { 29929, -1 }, /* ReplacementCodeuiNormal3fVertex3fvSUN */
   { 30056, -1 }, /* TexCoord4fVertex4fvSUN */
   { 30790, -1 }, /* ReplacementCodeuiVertex3fSUN */
   {    -1, -1 }
};
#endif

#if defined(need_GL_VERSION_1_3)
/* functions defined in MESA_remap_table_functions are excluded */
static const struct gl_function_remap GL_VERSION_1_3_functions[] = {
   {   464, _gloffset_MultiTexCoord3sARB },
   {   696, _gloffset_ActiveTextureARB },
   {  4666, _gloffset_MultiTexCoord1fvARB },
   {  6802, _gloffset_MultiTexCoord3dARB },
   {  6847, _gloffset_MultiTexCoord2iARB },
   {  6971, _gloffset_MultiTexCoord2svARB },
   {  8981, _gloffset_MultiTexCoord2fARB },
   { 11144, _gloffset_MultiTexCoord3fvARB },
   { 11746, _gloffset_MultiTexCoord4sARB },
   { 12427, _gloffset_MultiTexCoord2dvARB },
   { 12834, _gloffset_MultiTexCoord1svARB },
   { 13199, _gloffset_MultiTexCoord3svARB },
   { 13260, _gloffset_MultiTexCoord4iARB },
   { 14117, _gloffset_MultiTexCoord3iARB },
   { 14992, _gloffset_MultiTexCoord1dARB },
   { 15224, _gloffset_MultiTexCoord3dvARB },
   { 16540, _gloffset_MultiTexCoord3ivARB },
   { 16585, _gloffset_MultiTexCoord2sARB },
   { 18089, _gloffset_MultiTexCoord4ivARB },
   { 20357, _gloffset_ClientActiveTextureARB },
   { 22762, _gloffset_MultiTexCoord2dARB },
   { 23199, _gloffset_MultiTexCoord4dvARB },
   { 23621, _gloffset_MultiTexCoord4fvARB },
   { 24645, _gloffset_MultiTexCoord3fARB },
   { 27312, _gloffset_MultiTexCoord4dARB },
   { 27578, _gloffset_MultiTexCoord1sARB },
   { 27782, _gloffset_MultiTexCoord1dvARB },
   { 28790, _gloffset_MultiTexCoord1ivARB },
   { 28883, _gloffset_MultiTexCoord2ivARB },
   { 29222, _gloffset_MultiTexCoord1iARB },
   { 30658, _gloffset_MultiTexCoord4svARB },
   { 31282, _gloffset_MultiTexCoord1fARB },
   { 31562, _gloffset_MultiTexCoord4fARB },
   { 34102, _gloffset_MultiTexCoord2fvARB },
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

