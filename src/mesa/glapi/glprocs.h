/* DO NOT EDIT - This file generated automatically by gl_procs.py (from Mesa) script */

/*
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
 * BRIAN PAUL, IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* This file is only included by glapi.c and is used for
 * the GetProcAddress() function
 */

typedef struct {
    GLint Name_offset;
#ifdef NEED_FUNCTION_POINTER
    _glapi_proc Address;
#endif
    GLuint Offset;
} glprocs_table_t;

#ifdef NEED_FUNCTION_POINTER
#  define NAME_FUNC_OFFSET(n,f,o) { n , (_glapi_proc) f , o }
#else
#  define NAME_FUNC_OFFSET(n,f,o) { n , o }
#endif


static const char gl_string_table[] =
    "glNewList\0"
    "glEndList\0"
    "glCallList\0"
    "glCallLists\0"
    "glDeleteLists\0"
    "glGenLists\0"
    "glListBase\0"
    "glBegin\0"
    "glBitmap\0"
    "glColor3b\0"
    "glColor3bv\0"
    "glColor3d\0"
    "glColor3dv\0"
    "glColor3f\0"
    "glColor3fv\0"
    "glColor3i\0"
    "glColor3iv\0"
    "glColor3s\0"
    "glColor3sv\0"
    "glColor3ub\0"
    "glColor3ubv\0"
    "glColor3ui\0"
    "glColor3uiv\0"
    "glColor3us\0"
    "glColor3usv\0"
    "glColor4b\0"
    "glColor4bv\0"
    "glColor4d\0"
    "glColor4dv\0"
    "glColor4f\0"
    "glColor4fv\0"
    "glColor4i\0"
    "glColor4iv\0"
    "glColor4s\0"
    "glColor4sv\0"
    "glColor4ub\0"
    "glColor4ubv\0"
    "glColor4ui\0"
    "glColor4uiv\0"
    "glColor4us\0"
    "glColor4usv\0"
    "glEdgeFlag\0"
    "glEdgeFlagv\0"
    "glEnd\0"
    "glIndexd\0"
    "glIndexdv\0"
    "glIndexf\0"
    "glIndexfv\0"
    "glIndexi\0"
    "glIndexiv\0"
    "glIndexs\0"
    "glIndexsv\0"
    "glNormal3b\0"
    "glNormal3bv\0"
    "glNormal3d\0"
    "glNormal3dv\0"
    "glNormal3f\0"
    "glNormal3fv\0"
    "glNormal3i\0"
    "glNormal3iv\0"
    "glNormal3s\0"
    "glNormal3sv\0"
    "glRasterPos2d\0"
    "glRasterPos2dv\0"
    "glRasterPos2f\0"
    "glRasterPos2fv\0"
    "glRasterPos2i\0"
    "glRasterPos2iv\0"
    "glRasterPos2s\0"
    "glRasterPos2sv\0"
    "glRasterPos3d\0"
    "glRasterPos3dv\0"
    "glRasterPos3f\0"
    "glRasterPos3fv\0"
    "glRasterPos3i\0"
    "glRasterPos3iv\0"
    "glRasterPos3s\0"
    "glRasterPos3sv\0"
    "glRasterPos4d\0"
    "glRasterPos4dv\0"
    "glRasterPos4f\0"
    "glRasterPos4fv\0"
    "glRasterPos4i\0"
    "glRasterPos4iv\0"
    "glRasterPos4s\0"
    "glRasterPos4sv\0"
    "glRectd\0"
    "glRectdv\0"
    "glRectf\0"
    "glRectfv\0"
    "glRecti\0"
    "glRectiv\0"
    "glRects\0"
    "glRectsv\0"
    "glTexCoord1d\0"
    "glTexCoord1dv\0"
    "glTexCoord1f\0"
    "glTexCoord1fv\0"
    "glTexCoord1i\0"
    "glTexCoord1iv\0"
    "glTexCoord1s\0"
    "glTexCoord1sv\0"
    "glTexCoord2d\0"
    "glTexCoord2dv\0"
    "glTexCoord2f\0"
    "glTexCoord2fv\0"
    "glTexCoord2i\0"
    "glTexCoord2iv\0"
    "glTexCoord2s\0"
    "glTexCoord2sv\0"
    "glTexCoord3d\0"
    "glTexCoord3dv\0"
    "glTexCoord3f\0"
    "glTexCoord3fv\0"
    "glTexCoord3i\0"
    "glTexCoord3iv\0"
    "glTexCoord3s\0"
    "glTexCoord3sv\0"
    "glTexCoord4d\0"
    "glTexCoord4dv\0"
    "glTexCoord4f\0"
    "glTexCoord4fv\0"
    "glTexCoord4i\0"
    "glTexCoord4iv\0"
    "glTexCoord4s\0"
    "glTexCoord4sv\0"
    "glVertex2d\0"
    "glVertex2dv\0"
    "glVertex2f\0"
    "glVertex2fv\0"
    "glVertex2i\0"
    "glVertex2iv\0"
    "glVertex2s\0"
    "glVertex2sv\0"
    "glVertex3d\0"
    "glVertex3dv\0"
    "glVertex3f\0"
    "glVertex3fv\0"
    "glVertex3i\0"
    "glVertex3iv\0"
    "glVertex3s\0"
    "glVertex3sv\0"
    "glVertex4d\0"
    "glVertex4dv\0"
    "glVertex4f\0"
    "glVertex4fv\0"
    "glVertex4i\0"
    "glVertex4iv\0"
    "glVertex4s\0"
    "glVertex4sv\0"
    "glClipPlane\0"
    "glColorMaterial\0"
    "glCullFace\0"
    "glFogf\0"
    "glFogfv\0"
    "glFogi\0"
    "glFogiv\0"
    "glFrontFace\0"
    "glHint\0"
    "glLightf\0"
    "glLightfv\0"
    "glLighti\0"
    "glLightiv\0"
    "glLightModelf\0"
    "glLightModelfv\0"
    "glLightModeli\0"
    "glLightModeliv\0"
    "glLineStipple\0"
    "glLineWidth\0"
    "glMaterialf\0"
    "glMaterialfv\0"
    "glMateriali\0"
    "glMaterialiv\0"
    "glPointSize\0"
    "glPolygonMode\0"
    "glPolygonStipple\0"
    "glScissor\0"
    "glShadeModel\0"
    "glTexParameterf\0"
    "glTexParameterfv\0"
    "glTexParameteri\0"
    "glTexParameteriv\0"
    "glTexImage1D\0"
    "glTexImage2D\0"
    "glTexEnvf\0"
    "glTexEnvfv\0"
    "glTexEnvi\0"
    "glTexEnviv\0"
    "glTexGend\0"
    "glTexGendv\0"
    "glTexGenf\0"
    "glTexGenfv\0"
    "glTexGeni\0"
    "glTexGeniv\0"
    "glFeedbackBuffer\0"
    "glSelectBuffer\0"
    "glRenderMode\0"
    "glInitNames\0"
    "glLoadName\0"
    "glPassThrough\0"
    "glPopName\0"
    "glPushName\0"
    "glDrawBuffer\0"
    "glClear\0"
    "glClearAccum\0"
    "glClearIndex\0"
    "glClearColor\0"
    "glClearStencil\0"
    "glClearDepth\0"
    "glStencilMask\0"
    "glColorMask\0"
    "glDepthMask\0"
    "glIndexMask\0"
    "glAccum\0"
    "glDisable\0"
    "glEnable\0"
    "glFinish\0"
    "glFlush\0"
    "glPopAttrib\0"
    "glPushAttrib\0"
    "glMap1d\0"
    "glMap1f\0"
    "glMap2d\0"
    "glMap2f\0"
    "glMapGrid1d\0"
    "glMapGrid1f\0"
    "glMapGrid2d\0"
    "glMapGrid2f\0"
    "glEvalCoord1d\0"
    "glEvalCoord1dv\0"
    "glEvalCoord1f\0"
    "glEvalCoord1fv\0"
    "glEvalCoord2d\0"
    "glEvalCoord2dv\0"
    "glEvalCoord2f\0"
    "glEvalCoord2fv\0"
    "glEvalMesh1\0"
    "glEvalPoint1\0"
    "glEvalMesh2\0"
    "glEvalPoint2\0"
    "glAlphaFunc\0"
    "glBlendFunc\0"
    "glLogicOp\0"
    "glStencilFunc\0"
    "glStencilOp\0"
    "glDepthFunc\0"
    "glPixelZoom\0"
    "glPixelTransferf\0"
    "glPixelTransferi\0"
    "glPixelStoref\0"
    "glPixelStorei\0"
    "glPixelMapfv\0"
    "glPixelMapuiv\0"
    "glPixelMapusv\0"
    "glReadBuffer\0"
    "glCopyPixels\0"
    "glReadPixels\0"
    "glDrawPixels\0"
    "glGetBooleanv\0"
    "glGetClipPlane\0"
    "glGetDoublev\0"
    "glGetError\0"
    "glGetFloatv\0"
    "glGetIntegerv\0"
    "glGetLightfv\0"
    "glGetLightiv\0"
    "glGetMapdv\0"
    "glGetMapfv\0"
    "glGetMapiv\0"
    "glGetMaterialfv\0"
    "glGetMaterialiv\0"
    "glGetPixelMapfv\0"
    "glGetPixelMapuiv\0"
    "glGetPixelMapusv\0"
    "glGetPolygonStipple\0"
    "glGetString\0"
    "glGetTexEnvfv\0"
    "glGetTexEnviv\0"
    "glGetTexGendv\0"
    "glGetTexGenfv\0"
    "glGetTexGeniv\0"
    "glGetTexImage\0"
    "glGetTexParameterfv\0"
    "glGetTexParameteriv\0"
    "glGetTexLevelParameterfv\0"
    "glGetTexLevelParameteriv\0"
    "glIsEnabled\0"
    "glIsList\0"
    "glDepthRange\0"
    "glFrustum\0"
    "glLoadIdentity\0"
    "glLoadMatrixf\0"
    "glLoadMatrixd\0"
    "glMatrixMode\0"
    "glMultMatrixf\0"
    "glMultMatrixd\0"
    "glOrtho\0"
    "glPopMatrix\0"
    "glPushMatrix\0"
    "glRotated\0"
    "glRotatef\0"
    "glScaled\0"
    "glScalef\0"
    "glTranslated\0"
    "glTranslatef\0"
    "glViewport\0"
    "glArrayElement\0"
    "glBindTexture\0"
    "glColorPointer\0"
    "glDisableClientState\0"
    "glDrawArrays\0"
    "glDrawElements\0"
    "glEdgeFlagPointer\0"
    "glEnableClientState\0"
    "glIndexPointer\0"
    "glIndexub\0"
    "glIndexubv\0"
    "glInterleavedArrays\0"
    "glNormalPointer\0"
    "glPolygonOffset\0"
    "glTexCoordPointer\0"
    "glVertexPointer\0"
    "glAreTexturesResident\0"
    "glCopyTexImage1D\0"
    "glCopyTexImage2D\0"
    "glCopyTexSubImage1D\0"
    "glCopyTexSubImage2D\0"
    "glDeleteTextures\0"
    "glGenTextures\0"
    "glGetPointerv\0"
    "glIsTexture\0"
    "glPrioritizeTextures\0"
    "glTexSubImage1D\0"
    "glTexSubImage2D\0"
    "glPopClientAttrib\0"
    "glPushClientAttrib\0"
    "glBlendColor\0"
    "glBlendEquation\0"
    "glDrawRangeElements\0"
    "glColorTable\0"
    "glColorTableParameterfv\0"
    "glColorTableParameteriv\0"
    "glCopyColorTable\0"
    "glGetColorTable\0"
    "glGetColorTableParameterfv\0"
    "glGetColorTableParameteriv\0"
    "glColorSubTable\0"
    "glCopyColorSubTable\0"
    "glConvolutionFilter1D\0"
    "glConvolutionFilter2D\0"
    "glConvolutionParameterf\0"
    "glConvolutionParameterfv\0"
    "glConvolutionParameteri\0"
    "glConvolutionParameteriv\0"
    "glCopyConvolutionFilter1D\0"
    "glCopyConvolutionFilter2D\0"
    "glGetConvolutionFilter\0"
    "glGetConvolutionParameterfv\0"
    "glGetConvolutionParameteriv\0"
    "glGetSeparableFilter\0"
    "glSeparableFilter2D\0"
    "glGetHistogram\0"
    "glGetHistogramParameterfv\0"
    "glGetHistogramParameteriv\0"
    "glGetMinmax\0"
    "glGetMinmaxParameterfv\0"
    "glGetMinmaxParameteriv\0"
    "glHistogram\0"
    "glMinmax\0"
    "glResetHistogram\0"
    "glResetMinmax\0"
    "glTexImage3D\0"
    "glTexSubImage3D\0"
    "glCopyTexSubImage3D\0"
    "glActiveTextureARB\0"
    "glClientActiveTextureARB\0"
    "glMultiTexCoord1dARB\0"
    "glMultiTexCoord1dvARB\0"
    "glMultiTexCoord1fARB\0"
    "glMultiTexCoord1fvARB\0"
    "glMultiTexCoord1iARB\0"
    "glMultiTexCoord1ivARB\0"
    "glMultiTexCoord1sARB\0"
    "glMultiTexCoord1svARB\0"
    "glMultiTexCoord2dARB\0"
    "glMultiTexCoord2dvARB\0"
    "glMultiTexCoord2fARB\0"
    "glMultiTexCoord2fvARB\0"
    "glMultiTexCoord2iARB\0"
    "glMultiTexCoord2ivARB\0"
    "glMultiTexCoord2sARB\0"
    "glMultiTexCoord2svARB\0"
    "glMultiTexCoord3dARB\0"
    "glMultiTexCoord3dvARB\0"
    "glMultiTexCoord3fARB\0"
    "glMultiTexCoord3fvARB\0"
    "glMultiTexCoord3iARB\0"
    "glMultiTexCoord3ivARB\0"
    "glMultiTexCoord3sARB\0"
    "glMultiTexCoord3svARB\0"
    "glMultiTexCoord4dARB\0"
    "glMultiTexCoord4dvARB\0"
    "glMultiTexCoord4fARB\0"
    "glMultiTexCoord4fvARB\0"
    "glMultiTexCoord4iARB\0"
    "glMultiTexCoord4ivARB\0"
    "glMultiTexCoord4sARB\0"
    "glMultiTexCoord4svARB\0"
    "glStencilFuncSeparate\0"
    "glStencilMaskSeparate\0"
    "glStencilOpSeparate\0"
    "glLoadTransposeMatrixdARB\0"
    "glLoadTransposeMatrixfARB\0"
    "glMultTransposeMatrixdARB\0"
    "glMultTransposeMatrixfARB\0"
    "glSampleCoverageARB\0"
    "glCompressedTexImage1DARB\0"
    "glCompressedTexImage2DARB\0"
    "glCompressedTexImage3DARB\0"
    "glCompressedTexSubImage1DARB\0"
    "glCompressedTexSubImage2DARB\0"
    "glCompressedTexSubImage3DARB\0"
    "glGetCompressedTexImageARB\0"
    "glDisableVertexAttribArrayARB\0"
    "glEnableVertexAttribArrayARB\0"
    "glGetProgramEnvParameterdvARB\0"
    "glGetProgramEnvParameterfvARB\0"
    "glGetProgramLocalParameterdvARB\0"
    "glGetProgramLocalParameterfvARB\0"
    "glGetProgramStringARB\0"
    "glGetProgramivARB\0"
    "glGetVertexAttribdvARB\0"
    "glGetVertexAttribfvARB\0"
    "glGetVertexAttribivARB\0"
    "glProgramEnvParameter4dARB\0"
    "glProgramEnvParameter4dvARB\0"
    "glProgramEnvParameter4fARB\0"
    "glProgramEnvParameter4fvARB\0"
    "glProgramLocalParameter4dARB\0"
    "glProgramLocalParameter4dvARB\0"
    "glProgramLocalParameter4fARB\0"
    "glProgramLocalParameter4fvARB\0"
    "glProgramStringARB\0"
    "glVertexAttrib1dARB\0"
    "glVertexAttrib1dvARB\0"
    "glVertexAttrib1fARB\0"
    "glVertexAttrib1fvARB\0"
    "glVertexAttrib1sARB\0"
    "glVertexAttrib1svARB\0"
    "glVertexAttrib2dARB\0"
    "glVertexAttrib2dvARB\0"
    "glVertexAttrib2fARB\0"
    "glVertexAttrib2fvARB\0"
    "glVertexAttrib2sARB\0"
    "glVertexAttrib2svARB\0"
    "glVertexAttrib3dARB\0"
    "glVertexAttrib3dvARB\0"
    "glVertexAttrib3fARB\0"
    "glVertexAttrib3fvARB\0"
    "glVertexAttrib3sARB\0"
    "glVertexAttrib3svARB\0"
    "glVertexAttrib4NbvARB\0"
    "glVertexAttrib4NivARB\0"
    "glVertexAttrib4NsvARB\0"
    "glVertexAttrib4NubARB\0"
    "glVertexAttrib4NubvARB\0"
    "glVertexAttrib4NuivARB\0"
    "glVertexAttrib4NusvARB\0"
    "glVertexAttrib4bvARB\0"
    "glVertexAttrib4dARB\0"
    "glVertexAttrib4dvARB\0"
    "glVertexAttrib4fARB\0"
    "glVertexAttrib4fvARB\0"
    "glVertexAttrib4ivARB\0"
    "glVertexAttrib4sARB\0"
    "glVertexAttrib4svARB\0"
    "glVertexAttrib4ubvARB\0"
    "glVertexAttrib4uivARB\0"
    "glVertexAttrib4usvARB\0"
    "glVertexAttribPointerARB\0"
    "glBindBufferARB\0"
    "glBufferDataARB\0"
    "glBufferSubDataARB\0"
    "glDeleteBuffersARB\0"
    "glGenBuffersARB\0"
    "glGetBufferParameterivARB\0"
    "glGetBufferPointervARB\0"
    "glGetBufferSubDataARB\0"
    "glIsBufferARB\0"
    "glMapBufferARB\0"
    "glUnmapBufferARB\0"
    "glBeginQueryARB\0"
    "glDeleteQueriesARB\0"
    "glEndQueryARB\0"
    "glGenQueriesARB\0"
    "glGetQueryObjectivARB\0"
    "glGetQueryObjectuivARB\0"
    "glGetQueryivARB\0"
    "glIsQueryARB\0"
    "glAttachObjectARB\0"
    "glCompileShaderARB\0"
    "glCreateProgramObjectARB\0"
    "glCreateShaderObjectARB\0"
    "glDeleteObjectARB\0"
    "glDetachObjectARB\0"
    "glGetActiveUniformARB\0"
    "glGetAttachedObjectsARB\0"
    "glGetHandleARB\0"
    "glGetInfoLogARB\0"
    "glGetObjectParameterfvARB\0"
    "glGetObjectParameterivARB\0"
    "glGetShaderSourceARB\0"
    "glGetUniformLocationARB\0"
    "glGetUniformfvARB\0"
    "glGetUniformivARB\0"
    "glLinkProgramARB\0"
    "glShaderSourceARB\0"
    "glUniform1fARB\0"
    "glUniform1fvARB\0"
    "glUniform1iARB\0"
    "glUniform1ivARB\0"
    "glUniform2fARB\0"
    "glUniform2fvARB\0"
    "glUniform2iARB\0"
    "glUniform2ivARB\0"
    "glUniform3fARB\0"
    "glUniform3fvARB\0"
    "glUniform3iARB\0"
    "glUniform3ivARB\0"
    "glUniform4fARB\0"
    "glUniform4fvARB\0"
    "glUniform4iARB\0"
    "glUniform4ivARB\0"
    "glUniformMatrix2fvARB\0"
    "glUniformMatrix3fvARB\0"
    "glUniformMatrix4fvARB\0"
    "glUseProgramObjectARB\0"
    "glValidateProgramARB\0"
    "glBindAttribLocationARB\0"
    "glGetActiveAttribARB\0"
    "glGetAttribLocationARB\0"
    "glDrawBuffersARB\0"
    "glPolygonOffsetEXT\0"
    "glGetHistogramEXT\0"
    "glGetHistogramParameterfvEXT\0"
    "glGetHistogramParameterivEXT\0"
    "glGetMinmaxEXT\0"
    "glGetMinmaxParameterfvEXT\0"
    "glGetMinmaxParameterivEXT\0"
    "glGetConvolutionFilterEXT\0"
    "glGetConvolutionParameterfvEXT\0"
    "glGetConvolutionParameterivEXT\0"
    "glGetSeparableFilterEXT\0"
    "glGetColorTableParameterfvSGI\0"
    "glGetColorTableParameterivSGI\0"
    "glGetColorTableSGI\0"
    "glGetPixelTexGenParameterfvSGIS\0"
    "glGetPixelTexGenParameterivSGIS\0"
    "glPixelTexGenParameterfSGIS\0"
    "glPixelTexGenParameterfvSGIS\0"
    "glPixelTexGenParameteriSGIS\0"
    "glPixelTexGenParameterivSGIS\0"
    "glAreTexturesResidentEXT\0"
    "glGenTexturesEXT\0"
    "glIsTextureEXT\0"
    "glSampleMaskSGIS\0"
    "glSamplePatternSGIS\0"
    "glColorPointerEXT\0"
    "glEdgeFlagPointerEXT\0"
    "glIndexPointerEXT\0"
    "glNormalPointerEXT\0"
    "glTexCoordPointerEXT\0"
    "glVertexPointerEXT\0"
    "glPointParameterfEXT\0"
    "glPointParameterfvEXT\0"
    "glLockArraysEXT\0"
    "glUnlockArraysEXT\0"
    "glCullParameterdvEXT\0"
    "glCullParameterfvEXT\0"
    "glSecondaryColor3bEXT\0"
    "glSecondaryColor3bvEXT\0"
    "glSecondaryColor3dEXT\0"
    "glSecondaryColor3dvEXT\0"
    "glSecondaryColor3fEXT\0"
    "glSecondaryColor3fvEXT\0"
    "glSecondaryColor3iEXT\0"
    "glSecondaryColor3ivEXT\0"
    "glSecondaryColor3sEXT\0"
    "glSecondaryColor3svEXT\0"
    "glSecondaryColor3ubEXT\0"
    "glSecondaryColor3ubvEXT\0"
    "glSecondaryColor3uiEXT\0"
    "glSecondaryColor3uivEXT\0"
    "glSecondaryColor3usEXT\0"
    "glSecondaryColor3usvEXT\0"
    "glSecondaryColorPointerEXT\0"
    "glMultiDrawArraysEXT\0"
    "glMultiDrawElementsEXT\0"
    "glFogCoordPointerEXT\0"
    "glFogCoorddEXT\0"
    "glFogCoorddvEXT\0"
    "glFogCoordfEXT\0"
    "glFogCoordfvEXT\0"
    "glPixelTexGenSGIX\0"
    "glBlendFuncSeparateEXT\0"
    "glFlushVertexArrayRangeNV\0"
    "glVertexArrayRangeNV\0"
    "glCombinerInputNV\0"
    "glCombinerOutputNV\0"
    "glCombinerParameterfNV\0"
    "glCombinerParameterfvNV\0"
    "glCombinerParameteriNV\0"
    "glCombinerParameterivNV\0"
    "glFinalCombinerInputNV\0"
    "glGetCombinerInputParameterfvNV\0"
    "glGetCombinerInputParameterivNV\0"
    "glGetCombinerOutputParameterfvNV\0"
    "glGetCombinerOutputParameterivNV\0"
    "glGetFinalCombinerInputParameterfvNV\0"
    "glGetFinalCombinerInputParameterivNV\0"
    "glResizeBuffersMESA\0"
    "glWindowPos2dMESA\0"
    "glWindowPos2dvMESA\0"
    "glWindowPos2fMESA\0"
    "glWindowPos2fvMESA\0"
    "glWindowPos2iMESA\0"
    "glWindowPos2ivMESA\0"
    "glWindowPos2sMESA\0"
    "glWindowPos2svMESA\0"
    "glWindowPos3dMESA\0"
    "glWindowPos3dvMESA\0"
    "glWindowPos3fMESA\0"
    "glWindowPos3fvMESA\0"
    "glWindowPos3iMESA\0"
    "glWindowPos3ivMESA\0"
    "glWindowPos3sMESA\0"
    "glWindowPos3svMESA\0"
    "glWindowPos4dMESA\0"
    "glWindowPos4dvMESA\0"
    "glWindowPos4fMESA\0"
    "glWindowPos4fvMESA\0"
    "glWindowPos4iMESA\0"
    "glWindowPos4ivMESA\0"
    "glWindowPos4sMESA\0"
    "glWindowPos4svMESA\0"
    "glMultiModeDrawArraysIBM\0"
    "glMultiModeDrawElementsIBM\0"
    "glDeleteFencesNV\0"
    "glFinishFenceNV\0"
    "glGenFencesNV\0"
    "glGetFenceivNV\0"
    "glIsFenceNV\0"
    "glSetFenceNV\0"
    "glTestFenceNV\0"
    "glAreProgramsResidentNV\0"
    "glBindProgramNV\0"
    "glDeleteProgramsNV\0"
    "glExecuteProgramNV\0"
    "glGenProgramsNV\0"
    "glGetProgramParameterdvNV\0"
    "glGetProgramParameterfvNV\0"
    "glGetProgramStringNV\0"
    "glGetProgramivNV\0"
    "glGetTrackMatrixivNV\0"
    "glGetVertexAttribPointervNV\0"
    "glGetVertexAttribdvNV\0"
    "glGetVertexAttribfvNV\0"
    "glGetVertexAttribivNV\0"
    "glIsProgramNV\0"
    "glLoadProgramNV\0"
    "glProgramParameter4dNV\0"
    "glProgramParameter4dvNV\0"
    "glProgramParameter4fNV\0"
    "glProgramParameter4fvNV\0"
    "glProgramParameters4dvNV\0"
    "glProgramParameters4fvNV\0"
    "glRequestResidentProgramsNV\0"
    "glTrackMatrixNV\0"
    "glVertexAttrib1dNV\0"
    "glVertexAttrib1dvNV\0"
    "glVertexAttrib1fNV\0"
    "glVertexAttrib1fvNV\0"
    "glVertexAttrib1sNV\0"
    "glVertexAttrib1svNV\0"
    "glVertexAttrib2dNV\0"
    "glVertexAttrib2dvNV\0"
    "glVertexAttrib2fNV\0"
    "glVertexAttrib2fvNV\0"
    "glVertexAttrib2sNV\0"
    "glVertexAttrib2svNV\0"
    "glVertexAttrib3dNV\0"
    "glVertexAttrib3dvNV\0"
    "glVertexAttrib3fNV\0"
    "glVertexAttrib3fvNV\0"
    "glVertexAttrib3sNV\0"
    "glVertexAttrib3svNV\0"
    "glVertexAttrib4dNV\0"
    "glVertexAttrib4dvNV\0"
    "glVertexAttrib4fNV\0"
    "glVertexAttrib4fvNV\0"
    "glVertexAttrib4sNV\0"
    "glVertexAttrib4svNV\0"
    "glVertexAttrib4ubNV\0"
    "glVertexAttrib4ubvNV\0"
    "glVertexAttribPointerNV\0"
    "glVertexAttribs1dvNV\0"
    "glVertexAttribs1fvNV\0"
    "glVertexAttribs1svNV\0"
    "glVertexAttribs2dvNV\0"
    "glVertexAttribs2fvNV\0"
    "glVertexAttribs2svNV\0"
    "glVertexAttribs3dvNV\0"
    "glVertexAttribs3fvNV\0"
    "glVertexAttribs3svNV\0"
    "glVertexAttribs4dvNV\0"
    "glVertexAttribs4fvNV\0"
    "glVertexAttribs4svNV\0"
    "glVertexAttribs4ubvNV\0"
    "glAlphaFragmentOp1ATI\0"
    "glAlphaFragmentOp2ATI\0"
    "glAlphaFragmentOp3ATI\0"
    "glBeginFragmentShaderATI\0"
    "glBindFragmentShaderATI\0"
    "glColorFragmentOp1ATI\0"
    "glColorFragmentOp2ATI\0"
    "glColorFragmentOp3ATI\0"
    "glDeleteFragmentShaderATI\0"
    "glEndFragmentShaderATI\0"
    "glGenFragmentShadersATI\0"
    "glPassTexCoordATI\0"
    "glSampleMapATI\0"
    "glSetFragmentShaderConstantATI\0"
    "glPointParameteriNV\0"
    "glPointParameterivNV\0"
    "glActiveStencilFaceEXT\0"
    "glBindVertexArrayAPPLE\0"
    "glDeleteVertexArraysAPPLE\0"
    "glGenVertexArraysAPPLE\0"
    "glIsVertexArrayAPPLE\0"
    "glGetProgramNamedParameterdvNV\0"
    "glGetProgramNamedParameterfvNV\0"
    "glProgramNamedParameter4dNV\0"
    "glProgramNamedParameter4dvNV\0"
    "glProgramNamedParameter4fNV\0"
    "glProgramNamedParameter4fvNV\0"
    "glDepthBoundsEXT\0"
    "glBlendEquationSeparateEXT\0"
    "glBindFramebufferEXT\0"
    "glBindRenderbufferEXT\0"
    "glCheckFramebufferStatusEXT\0"
    "glDeleteFramebuffersEXT\0"
    "glDeleteRenderbuffersEXT\0"
    "glFramebufferRenderbufferEXT\0"
    "glFramebufferTexture1DEXT\0"
    "glFramebufferTexture2DEXT\0"
    "glFramebufferTexture3DEXT\0"
    "glGenFramebuffersEXT\0"
    "glGenRenderbuffersEXT\0"
    "glGenerateMipmapEXT\0"
    "glGetFramebufferAttachmentParameterivEXT\0"
    "glGetQueryObjecti64vEXT\0"
    "glGetQueryObjectui64vEXT\0"
    "glGetRenderbufferParameterivEXT\0"
    "glIsFramebufferEXT\0"
    "glIsRenderbufferEXT\0"
    "glRenderbufferStorageEXT\0"
    "glBlitFramebufferEXT\0"
    "glProgramEnvParameters4fvEXT\0"
    "glProgramLocalParameters4fvEXT\0"
    "glArrayElementEXT\0"
    "glBindTextureEXT\0"
    "glDrawArraysEXT\0"
    "glCopyTexImage1DEXT\0"
    "glCopyTexImage2DEXT\0"
    "glCopyTexSubImage1DEXT\0"
    "glCopyTexSubImage2DEXT\0"
    "glDeleteTexturesEXT\0"
    "glGetPointervEXT\0"
    "glPrioritizeTexturesEXT\0"
    "glTexSubImage1DEXT\0"
    "glTexSubImage2DEXT\0"
    "glBlendColorEXT\0"
    "glBlendEquationEXT\0"
    "glDrawRangeElementsEXT\0"
    "glColorTableSGI\0"
    "glColorTableEXT\0"
    "glColorTableParameterfvSGI\0"
    "glColorTableParameterivSGI\0"
    "glCopyColorTableSGI\0"
    "glColorSubTableEXT\0"
    "glCopyColorSubTableEXT\0"
    "glConvolutionFilter1DEXT\0"
    "glConvolutionFilter2DEXT\0"
    "glConvolutionParameterfEXT\0"
    "glConvolutionParameterfvEXT\0"
    "glConvolutionParameteriEXT\0"
    "glConvolutionParameterivEXT\0"
    "glCopyConvolutionFilter1DEXT\0"
    "glCopyConvolutionFilter2DEXT\0"
    "glSeparableFilter2DEXT\0"
    "glHistogramEXT\0"
    "glMinmaxEXT\0"
    "glResetHistogramEXT\0"
    "glResetMinmaxEXT\0"
    "glTexImage3DEXT\0"
    "glTexSubImage3DEXT\0"
    "glCopyTexSubImage3DEXT\0"
    "glActiveTexture\0"
    "glClientActiveTexture\0"
    "glMultiTexCoord1d\0"
    "glMultiTexCoord1dv\0"
    "glMultiTexCoord1f\0"
    "glMultiTexCoord1fv\0"
    "glMultiTexCoord1i\0"
    "glMultiTexCoord1iv\0"
    "glMultiTexCoord1s\0"
    "glMultiTexCoord1sv\0"
    "glMultiTexCoord2d\0"
    "glMultiTexCoord2dv\0"
    "glMultiTexCoord2f\0"
    "glMultiTexCoord2fv\0"
    "glMultiTexCoord2i\0"
    "glMultiTexCoord2iv\0"
    "glMultiTexCoord2s\0"
    "glMultiTexCoord2sv\0"
    "glMultiTexCoord3d\0"
    "glMultiTexCoord3dv\0"
    "glMultiTexCoord3f\0"
    "glMultiTexCoord3fv\0"
    "glMultiTexCoord3i\0"
    "glMultiTexCoord3iv\0"
    "glMultiTexCoord3s\0"
    "glMultiTexCoord3sv\0"
    "glMultiTexCoord4d\0"
    "glMultiTexCoord4dv\0"
    "glMultiTexCoord4f\0"
    "glMultiTexCoord4fv\0"
    "glMultiTexCoord4i\0"
    "glMultiTexCoord4iv\0"
    "glMultiTexCoord4s\0"
    "glMultiTexCoord4sv\0"
    "glLoadTransposeMatrixd\0"
    "glLoadTransposeMatrixf\0"
    "glMultTransposeMatrixd\0"
    "glMultTransposeMatrixf\0"
    "glSampleCoverage\0"
    "glCompressedTexImage1D\0"
    "glCompressedTexImage2D\0"
    "glCompressedTexImage3D\0"
    "glCompressedTexSubImage1D\0"
    "glCompressedTexSubImage2D\0"
    "glCompressedTexSubImage3D\0"
    "glGetCompressedTexImage\0"
    "glBindBuffer\0"
    "glBufferData\0"
    "glBufferSubData\0"
    "glDeleteBuffers\0"
    "glGenBuffers\0"
    "glGetBufferParameteriv\0"
    "glGetBufferPointerv\0"
    "glGetBufferSubData\0"
    "glIsBuffer\0"
    "glMapBuffer\0"
    "glUnmapBuffer\0"
    "glBeginQuery\0"
    "glDeleteQueries\0"
    "glEndQuery\0"
    "glGenQueries\0"
    "glGetQueryObjectiv\0"
    "glGetQueryObjectuiv\0"
    "glGetQueryiv\0"
    "glIsQuery\0"
    "glDrawBuffers\0"
    "glDrawBuffersATI\0"
    "glGetColorTableParameterfvEXT\0"
    "glGetColorTableParameterivEXT\0"
    "glGetColorTableEXT\0"
    "glSampleMaskEXT\0"
    "glSamplePatternEXT\0"
    "glPointParameterf\0"
    "glPointParameterfARB\0"
    "glPointParameterfSGIS\0"
    "glPointParameterfv\0"
    "glPointParameterfvARB\0"
    "glPointParameterfvSGIS\0"
    "glSecondaryColor3b\0"
    "glSecondaryColor3bv\0"
    "glSecondaryColor3d\0"
    "glSecondaryColor3dv\0"
    "glSecondaryColor3f\0"
    "glSecondaryColor3fv\0"
    "glSecondaryColor3i\0"
    "glSecondaryColor3iv\0"
    "glSecondaryColor3s\0"
    "glSecondaryColor3sv\0"
    "glSecondaryColor3ub\0"
    "glSecondaryColor3ubv\0"
    "glSecondaryColor3ui\0"
    "glSecondaryColor3uiv\0"
    "glSecondaryColor3us\0"
    "glSecondaryColor3usv\0"
    "glSecondaryColorPointer\0"
    "glMultiDrawArrays\0"
    "glMultiDrawElements\0"
    "glFogCoordPointer\0"
    "glFogCoordd\0"
    "glFogCoorddv\0"
    "glFogCoordf\0"
    "glFogCoordfv\0"
    "glBlendFuncSeparate\0"
    "glBlendFuncSeparateINGR\0"
    "glWindowPos2d\0"
    "glWindowPos2dARB\0"
    "glWindowPos2dv\0"
    "glWindowPos2dvARB\0"
    "glWindowPos2f\0"
    "glWindowPos2fARB\0"
    "glWindowPos2fv\0"
    "glWindowPos2fvARB\0"
    "glWindowPos2i\0"
    "glWindowPos2iARB\0"
    "glWindowPos2iv\0"
    "glWindowPos2ivARB\0"
    "glWindowPos2s\0"
    "glWindowPos2sARB\0"
    "glWindowPos2sv\0"
    "glWindowPos2svARB\0"
    "glWindowPos3d\0"
    "glWindowPos3dARB\0"
    "glWindowPos3dv\0"
    "glWindowPos3dvARB\0"
    "glWindowPos3f\0"
    "glWindowPos3fARB\0"
    "glWindowPos3fv\0"
    "glWindowPos3fvARB\0"
    "glWindowPos3i\0"
    "glWindowPos3iARB\0"
    "glWindowPos3iv\0"
    "glWindowPos3ivARB\0"
    "glWindowPos3s\0"
    "glWindowPos3sARB\0"
    "glWindowPos3sv\0"
    "glWindowPos3svARB\0"
    "glBindProgramARB\0"
    "glDeleteProgramsARB\0"
    "glGenProgramsARB\0"
    "glGetVertexAttribPointervARB\0"
    "glIsProgramARB\0"
    "glPointParameteri\0"
    "glPointParameteriv\0"
    "glBlendEquationSeparate\0"
    "glBlendEquationSeparateATI\0"
    ;

/* FIXME: Having these (incorrect) prototypes here is ugly. */
#ifdef NEED_FUNCTION_POINTER
extern void gl_dispatch_stub_543(void);
extern void gl_dispatch_stub_544(void);
extern void gl_dispatch_stub_545(void);
extern void gl_dispatch_stub_546(void);
extern void gl_dispatch_stub_547(void);
extern void gl_dispatch_stub_548(void);
extern void gl_dispatch_stub_549(void);
extern void gl_dispatch_stub_550(void);
extern void gl_dispatch_stub_551(void);
extern void gl_dispatch_stub_552(void);
extern void gl_dispatch_stub_553(void);
extern void gl_dispatch_stub_554(void);
extern void gl_dispatch_stub_555(void);
extern void gl_dispatch_stub_556(void);
extern void gl_dispatch_stub_557(void);
extern void gl_dispatch_stub_558(void);
extern void gl_dispatch_stub_559(void);
extern void gl_dispatch_stub_560(void);
extern void gl_dispatch_stub_561(void);
extern void gl_dispatch_stub_565(void);
extern void gl_dispatch_stub_566(void);
extern void gl_dispatch_stub_577(void);
extern void gl_dispatch_stub_578(void);
extern void gl_dispatch_stub_603(void);
extern void gl_dispatch_stub_645(void);
extern void gl_dispatch_stub_646(void);
extern void gl_dispatch_stub_647(void);
extern void gl_dispatch_stub_648(void);
extern void gl_dispatch_stub_649(void);
extern void gl_dispatch_stub_650(void);
extern void gl_dispatch_stub_651(void);
extern void gl_dispatch_stub_652(void);
extern void gl_dispatch_stub_653(void);
extern void gl_dispatch_stub_734(void);
extern void gl_dispatch_stub_735(void);
extern void gl_dispatch_stub_736(void);
extern void gl_dispatch_stub_737(void);
extern void gl_dispatch_stub_738(void);
extern void gl_dispatch_stub_745(void);
extern void gl_dispatch_stub_746(void);
extern void gl_dispatch_stub_760(void);
extern void gl_dispatch_stub_761(void);
extern void gl_dispatch_stub_766(void);
extern void gl_dispatch_stub_767(void);
extern void gl_dispatch_stub_768(void);
#endif /* NEED_FUNCTION_POINTER */

static const glprocs_table_t static_functions[] = {
    NAME_FUNC_OFFSET(     0, glNewList, _gloffset_NewList ),
    NAME_FUNC_OFFSET(    10, glEndList, _gloffset_EndList ),
    NAME_FUNC_OFFSET(    20, glCallList, _gloffset_CallList ),
    NAME_FUNC_OFFSET(    31, glCallLists, _gloffset_CallLists ),
    NAME_FUNC_OFFSET(    43, glDeleteLists, _gloffset_DeleteLists ),
    NAME_FUNC_OFFSET(    57, glGenLists, _gloffset_GenLists ),
    NAME_FUNC_OFFSET(    68, glListBase, _gloffset_ListBase ),
    NAME_FUNC_OFFSET(    79, glBegin, _gloffset_Begin ),
    NAME_FUNC_OFFSET(    87, glBitmap, _gloffset_Bitmap ),
    NAME_FUNC_OFFSET(    96, glColor3b, _gloffset_Color3b ),
    NAME_FUNC_OFFSET(   106, glColor3bv, _gloffset_Color3bv ),
    NAME_FUNC_OFFSET(   117, glColor3d, _gloffset_Color3d ),
    NAME_FUNC_OFFSET(   127, glColor3dv, _gloffset_Color3dv ),
    NAME_FUNC_OFFSET(   138, glColor3f, _gloffset_Color3f ),
    NAME_FUNC_OFFSET(   148, glColor3fv, _gloffset_Color3fv ),
    NAME_FUNC_OFFSET(   159, glColor3i, _gloffset_Color3i ),
    NAME_FUNC_OFFSET(   169, glColor3iv, _gloffset_Color3iv ),
    NAME_FUNC_OFFSET(   180, glColor3s, _gloffset_Color3s ),
    NAME_FUNC_OFFSET(   190, glColor3sv, _gloffset_Color3sv ),
    NAME_FUNC_OFFSET(   201, glColor3ub, _gloffset_Color3ub ),
    NAME_FUNC_OFFSET(   212, glColor3ubv, _gloffset_Color3ubv ),
    NAME_FUNC_OFFSET(   224, glColor3ui, _gloffset_Color3ui ),
    NAME_FUNC_OFFSET(   235, glColor3uiv, _gloffset_Color3uiv ),
    NAME_FUNC_OFFSET(   247, glColor3us, _gloffset_Color3us ),
    NAME_FUNC_OFFSET(   258, glColor3usv, _gloffset_Color3usv ),
    NAME_FUNC_OFFSET(   270, glColor4b, _gloffset_Color4b ),
    NAME_FUNC_OFFSET(   280, glColor4bv, _gloffset_Color4bv ),
    NAME_FUNC_OFFSET(   291, glColor4d, _gloffset_Color4d ),
    NAME_FUNC_OFFSET(   301, glColor4dv, _gloffset_Color4dv ),
    NAME_FUNC_OFFSET(   312, glColor4f, _gloffset_Color4f ),
    NAME_FUNC_OFFSET(   322, glColor4fv, _gloffset_Color4fv ),
    NAME_FUNC_OFFSET(   333, glColor4i, _gloffset_Color4i ),
    NAME_FUNC_OFFSET(   343, glColor4iv, _gloffset_Color4iv ),
    NAME_FUNC_OFFSET(   354, glColor4s, _gloffset_Color4s ),
    NAME_FUNC_OFFSET(   364, glColor4sv, _gloffset_Color4sv ),
    NAME_FUNC_OFFSET(   375, glColor4ub, _gloffset_Color4ub ),
    NAME_FUNC_OFFSET(   386, glColor4ubv, _gloffset_Color4ubv ),
    NAME_FUNC_OFFSET(   398, glColor4ui, _gloffset_Color4ui ),
    NAME_FUNC_OFFSET(   409, glColor4uiv, _gloffset_Color4uiv ),
    NAME_FUNC_OFFSET(   421, glColor4us, _gloffset_Color4us ),
    NAME_FUNC_OFFSET(   432, glColor4usv, _gloffset_Color4usv ),
    NAME_FUNC_OFFSET(   444, glEdgeFlag, _gloffset_EdgeFlag ),
    NAME_FUNC_OFFSET(   455, glEdgeFlagv, _gloffset_EdgeFlagv ),
    NAME_FUNC_OFFSET(   467, glEnd, _gloffset_End ),
    NAME_FUNC_OFFSET(   473, glIndexd, _gloffset_Indexd ),
    NAME_FUNC_OFFSET(   482, glIndexdv, _gloffset_Indexdv ),
    NAME_FUNC_OFFSET(   492, glIndexf, _gloffset_Indexf ),
    NAME_FUNC_OFFSET(   501, glIndexfv, _gloffset_Indexfv ),
    NAME_FUNC_OFFSET(   511, glIndexi, _gloffset_Indexi ),
    NAME_FUNC_OFFSET(   520, glIndexiv, _gloffset_Indexiv ),
    NAME_FUNC_OFFSET(   530, glIndexs, _gloffset_Indexs ),
    NAME_FUNC_OFFSET(   539, glIndexsv, _gloffset_Indexsv ),
    NAME_FUNC_OFFSET(   549, glNormal3b, _gloffset_Normal3b ),
    NAME_FUNC_OFFSET(   560, glNormal3bv, _gloffset_Normal3bv ),
    NAME_FUNC_OFFSET(   572, glNormal3d, _gloffset_Normal3d ),
    NAME_FUNC_OFFSET(   583, glNormal3dv, _gloffset_Normal3dv ),
    NAME_FUNC_OFFSET(   595, glNormal3f, _gloffset_Normal3f ),
    NAME_FUNC_OFFSET(   606, glNormal3fv, _gloffset_Normal3fv ),
    NAME_FUNC_OFFSET(   618, glNormal3i, _gloffset_Normal3i ),
    NAME_FUNC_OFFSET(   629, glNormal3iv, _gloffset_Normal3iv ),
    NAME_FUNC_OFFSET(   641, glNormal3s, _gloffset_Normal3s ),
    NAME_FUNC_OFFSET(   652, glNormal3sv, _gloffset_Normal3sv ),
    NAME_FUNC_OFFSET(   664, glRasterPos2d, _gloffset_RasterPos2d ),
    NAME_FUNC_OFFSET(   678, glRasterPos2dv, _gloffset_RasterPos2dv ),
    NAME_FUNC_OFFSET(   693, glRasterPos2f, _gloffset_RasterPos2f ),
    NAME_FUNC_OFFSET(   707, glRasterPos2fv, _gloffset_RasterPos2fv ),
    NAME_FUNC_OFFSET(   722, glRasterPos2i, _gloffset_RasterPos2i ),
    NAME_FUNC_OFFSET(   736, glRasterPos2iv, _gloffset_RasterPos2iv ),
    NAME_FUNC_OFFSET(   751, glRasterPos2s, _gloffset_RasterPos2s ),
    NAME_FUNC_OFFSET(   765, glRasterPos2sv, _gloffset_RasterPos2sv ),
    NAME_FUNC_OFFSET(   780, glRasterPos3d, _gloffset_RasterPos3d ),
    NAME_FUNC_OFFSET(   794, glRasterPos3dv, _gloffset_RasterPos3dv ),
    NAME_FUNC_OFFSET(   809, glRasterPos3f, _gloffset_RasterPos3f ),
    NAME_FUNC_OFFSET(   823, glRasterPos3fv, _gloffset_RasterPos3fv ),
    NAME_FUNC_OFFSET(   838, glRasterPos3i, _gloffset_RasterPos3i ),
    NAME_FUNC_OFFSET(   852, glRasterPos3iv, _gloffset_RasterPos3iv ),
    NAME_FUNC_OFFSET(   867, glRasterPos3s, _gloffset_RasterPos3s ),
    NAME_FUNC_OFFSET(   881, glRasterPos3sv, _gloffset_RasterPos3sv ),
    NAME_FUNC_OFFSET(   896, glRasterPos4d, _gloffset_RasterPos4d ),
    NAME_FUNC_OFFSET(   910, glRasterPos4dv, _gloffset_RasterPos4dv ),
    NAME_FUNC_OFFSET(   925, glRasterPos4f, _gloffset_RasterPos4f ),
    NAME_FUNC_OFFSET(   939, glRasterPos4fv, _gloffset_RasterPos4fv ),
    NAME_FUNC_OFFSET(   954, glRasterPos4i, _gloffset_RasterPos4i ),
    NAME_FUNC_OFFSET(   968, glRasterPos4iv, _gloffset_RasterPos4iv ),
    NAME_FUNC_OFFSET(   983, glRasterPos4s, _gloffset_RasterPos4s ),
    NAME_FUNC_OFFSET(   997, glRasterPos4sv, _gloffset_RasterPos4sv ),
    NAME_FUNC_OFFSET(  1012, glRectd, _gloffset_Rectd ),
    NAME_FUNC_OFFSET(  1020, glRectdv, _gloffset_Rectdv ),
    NAME_FUNC_OFFSET(  1029, glRectf, _gloffset_Rectf ),
    NAME_FUNC_OFFSET(  1037, glRectfv, _gloffset_Rectfv ),
    NAME_FUNC_OFFSET(  1046, glRecti, _gloffset_Recti ),
    NAME_FUNC_OFFSET(  1054, glRectiv, _gloffset_Rectiv ),
    NAME_FUNC_OFFSET(  1063, glRects, _gloffset_Rects ),
    NAME_FUNC_OFFSET(  1071, glRectsv, _gloffset_Rectsv ),
    NAME_FUNC_OFFSET(  1080, glTexCoord1d, _gloffset_TexCoord1d ),
    NAME_FUNC_OFFSET(  1093, glTexCoord1dv, _gloffset_TexCoord1dv ),
    NAME_FUNC_OFFSET(  1107, glTexCoord1f, _gloffset_TexCoord1f ),
    NAME_FUNC_OFFSET(  1120, glTexCoord1fv, _gloffset_TexCoord1fv ),
    NAME_FUNC_OFFSET(  1134, glTexCoord1i, _gloffset_TexCoord1i ),
    NAME_FUNC_OFFSET(  1147, glTexCoord1iv, _gloffset_TexCoord1iv ),
    NAME_FUNC_OFFSET(  1161, glTexCoord1s, _gloffset_TexCoord1s ),
    NAME_FUNC_OFFSET(  1174, glTexCoord1sv, _gloffset_TexCoord1sv ),
    NAME_FUNC_OFFSET(  1188, glTexCoord2d, _gloffset_TexCoord2d ),
    NAME_FUNC_OFFSET(  1201, glTexCoord2dv, _gloffset_TexCoord2dv ),
    NAME_FUNC_OFFSET(  1215, glTexCoord2f, _gloffset_TexCoord2f ),
    NAME_FUNC_OFFSET(  1228, glTexCoord2fv, _gloffset_TexCoord2fv ),
    NAME_FUNC_OFFSET(  1242, glTexCoord2i, _gloffset_TexCoord2i ),
    NAME_FUNC_OFFSET(  1255, glTexCoord2iv, _gloffset_TexCoord2iv ),
    NAME_FUNC_OFFSET(  1269, glTexCoord2s, _gloffset_TexCoord2s ),
    NAME_FUNC_OFFSET(  1282, glTexCoord2sv, _gloffset_TexCoord2sv ),
    NAME_FUNC_OFFSET(  1296, glTexCoord3d, _gloffset_TexCoord3d ),
    NAME_FUNC_OFFSET(  1309, glTexCoord3dv, _gloffset_TexCoord3dv ),
    NAME_FUNC_OFFSET(  1323, glTexCoord3f, _gloffset_TexCoord3f ),
    NAME_FUNC_OFFSET(  1336, glTexCoord3fv, _gloffset_TexCoord3fv ),
    NAME_FUNC_OFFSET(  1350, glTexCoord3i, _gloffset_TexCoord3i ),
    NAME_FUNC_OFFSET(  1363, glTexCoord3iv, _gloffset_TexCoord3iv ),
    NAME_FUNC_OFFSET(  1377, glTexCoord3s, _gloffset_TexCoord3s ),
    NAME_FUNC_OFFSET(  1390, glTexCoord3sv, _gloffset_TexCoord3sv ),
    NAME_FUNC_OFFSET(  1404, glTexCoord4d, _gloffset_TexCoord4d ),
    NAME_FUNC_OFFSET(  1417, glTexCoord4dv, _gloffset_TexCoord4dv ),
    NAME_FUNC_OFFSET(  1431, glTexCoord4f, _gloffset_TexCoord4f ),
    NAME_FUNC_OFFSET(  1444, glTexCoord4fv, _gloffset_TexCoord4fv ),
    NAME_FUNC_OFFSET(  1458, glTexCoord4i, _gloffset_TexCoord4i ),
    NAME_FUNC_OFFSET(  1471, glTexCoord4iv, _gloffset_TexCoord4iv ),
    NAME_FUNC_OFFSET(  1485, glTexCoord4s, _gloffset_TexCoord4s ),
    NAME_FUNC_OFFSET(  1498, glTexCoord4sv, _gloffset_TexCoord4sv ),
    NAME_FUNC_OFFSET(  1512, glVertex2d, _gloffset_Vertex2d ),
    NAME_FUNC_OFFSET(  1523, glVertex2dv, _gloffset_Vertex2dv ),
    NAME_FUNC_OFFSET(  1535, glVertex2f, _gloffset_Vertex2f ),
    NAME_FUNC_OFFSET(  1546, glVertex2fv, _gloffset_Vertex2fv ),
    NAME_FUNC_OFFSET(  1558, glVertex2i, _gloffset_Vertex2i ),
    NAME_FUNC_OFFSET(  1569, glVertex2iv, _gloffset_Vertex2iv ),
    NAME_FUNC_OFFSET(  1581, glVertex2s, _gloffset_Vertex2s ),
    NAME_FUNC_OFFSET(  1592, glVertex2sv, _gloffset_Vertex2sv ),
    NAME_FUNC_OFFSET(  1604, glVertex3d, _gloffset_Vertex3d ),
    NAME_FUNC_OFFSET(  1615, glVertex3dv, _gloffset_Vertex3dv ),
    NAME_FUNC_OFFSET(  1627, glVertex3f, _gloffset_Vertex3f ),
    NAME_FUNC_OFFSET(  1638, glVertex3fv, _gloffset_Vertex3fv ),
    NAME_FUNC_OFFSET(  1650, glVertex3i, _gloffset_Vertex3i ),
    NAME_FUNC_OFFSET(  1661, glVertex3iv, _gloffset_Vertex3iv ),
    NAME_FUNC_OFFSET(  1673, glVertex3s, _gloffset_Vertex3s ),
    NAME_FUNC_OFFSET(  1684, glVertex3sv, _gloffset_Vertex3sv ),
    NAME_FUNC_OFFSET(  1696, glVertex4d, _gloffset_Vertex4d ),
    NAME_FUNC_OFFSET(  1707, glVertex4dv, _gloffset_Vertex4dv ),
    NAME_FUNC_OFFSET(  1719, glVertex4f, _gloffset_Vertex4f ),
    NAME_FUNC_OFFSET(  1730, glVertex4fv, _gloffset_Vertex4fv ),
    NAME_FUNC_OFFSET(  1742, glVertex4i, _gloffset_Vertex4i ),
    NAME_FUNC_OFFSET(  1753, glVertex4iv, _gloffset_Vertex4iv ),
    NAME_FUNC_OFFSET(  1765, glVertex4s, _gloffset_Vertex4s ),
    NAME_FUNC_OFFSET(  1776, glVertex4sv, _gloffset_Vertex4sv ),
    NAME_FUNC_OFFSET(  1788, glClipPlane, _gloffset_ClipPlane ),
    NAME_FUNC_OFFSET(  1800, glColorMaterial, _gloffset_ColorMaterial ),
    NAME_FUNC_OFFSET(  1816, glCullFace, _gloffset_CullFace ),
    NAME_FUNC_OFFSET(  1827, glFogf, _gloffset_Fogf ),
    NAME_FUNC_OFFSET(  1834, glFogfv, _gloffset_Fogfv ),
    NAME_FUNC_OFFSET(  1842, glFogi, _gloffset_Fogi ),
    NAME_FUNC_OFFSET(  1849, glFogiv, _gloffset_Fogiv ),
    NAME_FUNC_OFFSET(  1857, glFrontFace, _gloffset_FrontFace ),
    NAME_FUNC_OFFSET(  1869, glHint, _gloffset_Hint ),
    NAME_FUNC_OFFSET(  1876, glLightf, _gloffset_Lightf ),
    NAME_FUNC_OFFSET(  1885, glLightfv, _gloffset_Lightfv ),
    NAME_FUNC_OFFSET(  1895, glLighti, _gloffset_Lighti ),
    NAME_FUNC_OFFSET(  1904, glLightiv, _gloffset_Lightiv ),
    NAME_FUNC_OFFSET(  1914, glLightModelf, _gloffset_LightModelf ),
    NAME_FUNC_OFFSET(  1928, glLightModelfv, _gloffset_LightModelfv ),
    NAME_FUNC_OFFSET(  1943, glLightModeli, _gloffset_LightModeli ),
    NAME_FUNC_OFFSET(  1957, glLightModeliv, _gloffset_LightModeliv ),
    NAME_FUNC_OFFSET(  1972, glLineStipple, _gloffset_LineStipple ),
    NAME_FUNC_OFFSET(  1986, glLineWidth, _gloffset_LineWidth ),
    NAME_FUNC_OFFSET(  1998, glMaterialf, _gloffset_Materialf ),
    NAME_FUNC_OFFSET(  2010, glMaterialfv, _gloffset_Materialfv ),
    NAME_FUNC_OFFSET(  2023, glMateriali, _gloffset_Materiali ),
    NAME_FUNC_OFFSET(  2035, glMaterialiv, _gloffset_Materialiv ),
    NAME_FUNC_OFFSET(  2048, glPointSize, _gloffset_PointSize ),
    NAME_FUNC_OFFSET(  2060, glPolygonMode, _gloffset_PolygonMode ),
    NAME_FUNC_OFFSET(  2074, glPolygonStipple, _gloffset_PolygonStipple ),
    NAME_FUNC_OFFSET(  2091, glScissor, _gloffset_Scissor ),
    NAME_FUNC_OFFSET(  2101, glShadeModel, _gloffset_ShadeModel ),
    NAME_FUNC_OFFSET(  2114, glTexParameterf, _gloffset_TexParameterf ),
    NAME_FUNC_OFFSET(  2130, glTexParameterfv, _gloffset_TexParameterfv ),
    NAME_FUNC_OFFSET(  2147, glTexParameteri, _gloffset_TexParameteri ),
    NAME_FUNC_OFFSET(  2163, glTexParameteriv, _gloffset_TexParameteriv ),
    NAME_FUNC_OFFSET(  2180, glTexImage1D, _gloffset_TexImage1D ),
    NAME_FUNC_OFFSET(  2193, glTexImage2D, _gloffset_TexImage2D ),
    NAME_FUNC_OFFSET(  2206, glTexEnvf, _gloffset_TexEnvf ),
    NAME_FUNC_OFFSET(  2216, glTexEnvfv, _gloffset_TexEnvfv ),
    NAME_FUNC_OFFSET(  2227, glTexEnvi, _gloffset_TexEnvi ),
    NAME_FUNC_OFFSET(  2237, glTexEnviv, _gloffset_TexEnviv ),
    NAME_FUNC_OFFSET(  2248, glTexGend, _gloffset_TexGend ),
    NAME_FUNC_OFFSET(  2258, glTexGendv, _gloffset_TexGendv ),
    NAME_FUNC_OFFSET(  2269, glTexGenf, _gloffset_TexGenf ),
    NAME_FUNC_OFFSET(  2279, glTexGenfv, _gloffset_TexGenfv ),
    NAME_FUNC_OFFSET(  2290, glTexGeni, _gloffset_TexGeni ),
    NAME_FUNC_OFFSET(  2300, glTexGeniv, _gloffset_TexGeniv ),
    NAME_FUNC_OFFSET(  2311, glFeedbackBuffer, _gloffset_FeedbackBuffer ),
    NAME_FUNC_OFFSET(  2328, glSelectBuffer, _gloffset_SelectBuffer ),
    NAME_FUNC_OFFSET(  2343, glRenderMode, _gloffset_RenderMode ),
    NAME_FUNC_OFFSET(  2356, glInitNames, _gloffset_InitNames ),
    NAME_FUNC_OFFSET(  2368, glLoadName, _gloffset_LoadName ),
    NAME_FUNC_OFFSET(  2379, glPassThrough, _gloffset_PassThrough ),
    NAME_FUNC_OFFSET(  2393, glPopName, _gloffset_PopName ),
    NAME_FUNC_OFFSET(  2403, glPushName, _gloffset_PushName ),
    NAME_FUNC_OFFSET(  2414, glDrawBuffer, _gloffset_DrawBuffer ),
    NAME_FUNC_OFFSET(  2427, glClear, _gloffset_Clear ),
    NAME_FUNC_OFFSET(  2435, glClearAccum, _gloffset_ClearAccum ),
    NAME_FUNC_OFFSET(  2448, glClearIndex, _gloffset_ClearIndex ),
    NAME_FUNC_OFFSET(  2461, glClearColor, _gloffset_ClearColor ),
    NAME_FUNC_OFFSET(  2474, glClearStencil, _gloffset_ClearStencil ),
    NAME_FUNC_OFFSET(  2489, glClearDepth, _gloffset_ClearDepth ),
    NAME_FUNC_OFFSET(  2502, glStencilMask, _gloffset_StencilMask ),
    NAME_FUNC_OFFSET(  2516, glColorMask, _gloffset_ColorMask ),
    NAME_FUNC_OFFSET(  2528, glDepthMask, _gloffset_DepthMask ),
    NAME_FUNC_OFFSET(  2540, glIndexMask, _gloffset_IndexMask ),
    NAME_FUNC_OFFSET(  2552, glAccum, _gloffset_Accum ),
    NAME_FUNC_OFFSET(  2560, glDisable, _gloffset_Disable ),
    NAME_FUNC_OFFSET(  2570, glEnable, _gloffset_Enable ),
    NAME_FUNC_OFFSET(  2579, glFinish, _gloffset_Finish ),
    NAME_FUNC_OFFSET(  2588, glFlush, _gloffset_Flush ),
    NAME_FUNC_OFFSET(  2596, glPopAttrib, _gloffset_PopAttrib ),
    NAME_FUNC_OFFSET(  2608, glPushAttrib, _gloffset_PushAttrib ),
    NAME_FUNC_OFFSET(  2621, glMap1d, _gloffset_Map1d ),
    NAME_FUNC_OFFSET(  2629, glMap1f, _gloffset_Map1f ),
    NAME_FUNC_OFFSET(  2637, glMap2d, _gloffset_Map2d ),
    NAME_FUNC_OFFSET(  2645, glMap2f, _gloffset_Map2f ),
    NAME_FUNC_OFFSET(  2653, glMapGrid1d, _gloffset_MapGrid1d ),
    NAME_FUNC_OFFSET(  2665, glMapGrid1f, _gloffset_MapGrid1f ),
    NAME_FUNC_OFFSET(  2677, glMapGrid2d, _gloffset_MapGrid2d ),
    NAME_FUNC_OFFSET(  2689, glMapGrid2f, _gloffset_MapGrid2f ),
    NAME_FUNC_OFFSET(  2701, glEvalCoord1d, _gloffset_EvalCoord1d ),
    NAME_FUNC_OFFSET(  2715, glEvalCoord1dv, _gloffset_EvalCoord1dv ),
    NAME_FUNC_OFFSET(  2730, glEvalCoord1f, _gloffset_EvalCoord1f ),
    NAME_FUNC_OFFSET(  2744, glEvalCoord1fv, _gloffset_EvalCoord1fv ),
    NAME_FUNC_OFFSET(  2759, glEvalCoord2d, _gloffset_EvalCoord2d ),
    NAME_FUNC_OFFSET(  2773, glEvalCoord2dv, _gloffset_EvalCoord2dv ),
    NAME_FUNC_OFFSET(  2788, glEvalCoord2f, _gloffset_EvalCoord2f ),
    NAME_FUNC_OFFSET(  2802, glEvalCoord2fv, _gloffset_EvalCoord2fv ),
    NAME_FUNC_OFFSET(  2817, glEvalMesh1, _gloffset_EvalMesh1 ),
    NAME_FUNC_OFFSET(  2829, glEvalPoint1, _gloffset_EvalPoint1 ),
    NAME_FUNC_OFFSET(  2842, glEvalMesh2, _gloffset_EvalMesh2 ),
    NAME_FUNC_OFFSET(  2854, glEvalPoint2, _gloffset_EvalPoint2 ),
    NAME_FUNC_OFFSET(  2867, glAlphaFunc, _gloffset_AlphaFunc ),
    NAME_FUNC_OFFSET(  2879, glBlendFunc, _gloffset_BlendFunc ),
    NAME_FUNC_OFFSET(  2891, glLogicOp, _gloffset_LogicOp ),
    NAME_FUNC_OFFSET(  2901, glStencilFunc, _gloffset_StencilFunc ),
    NAME_FUNC_OFFSET(  2915, glStencilOp, _gloffset_StencilOp ),
    NAME_FUNC_OFFSET(  2927, glDepthFunc, _gloffset_DepthFunc ),
    NAME_FUNC_OFFSET(  2939, glPixelZoom, _gloffset_PixelZoom ),
    NAME_FUNC_OFFSET(  2951, glPixelTransferf, _gloffset_PixelTransferf ),
    NAME_FUNC_OFFSET(  2968, glPixelTransferi, _gloffset_PixelTransferi ),
    NAME_FUNC_OFFSET(  2985, glPixelStoref, _gloffset_PixelStoref ),
    NAME_FUNC_OFFSET(  2999, glPixelStorei, _gloffset_PixelStorei ),
    NAME_FUNC_OFFSET(  3013, glPixelMapfv, _gloffset_PixelMapfv ),
    NAME_FUNC_OFFSET(  3026, glPixelMapuiv, _gloffset_PixelMapuiv ),
    NAME_FUNC_OFFSET(  3040, glPixelMapusv, _gloffset_PixelMapusv ),
    NAME_FUNC_OFFSET(  3054, glReadBuffer, _gloffset_ReadBuffer ),
    NAME_FUNC_OFFSET(  3067, glCopyPixels, _gloffset_CopyPixels ),
    NAME_FUNC_OFFSET(  3080, glReadPixels, _gloffset_ReadPixels ),
    NAME_FUNC_OFFSET(  3093, glDrawPixels, _gloffset_DrawPixels ),
    NAME_FUNC_OFFSET(  3106, glGetBooleanv, _gloffset_GetBooleanv ),
    NAME_FUNC_OFFSET(  3120, glGetClipPlane, _gloffset_GetClipPlane ),
    NAME_FUNC_OFFSET(  3135, glGetDoublev, _gloffset_GetDoublev ),
    NAME_FUNC_OFFSET(  3148, glGetError, _gloffset_GetError ),
    NAME_FUNC_OFFSET(  3159, glGetFloatv, _gloffset_GetFloatv ),
    NAME_FUNC_OFFSET(  3171, glGetIntegerv, _gloffset_GetIntegerv ),
    NAME_FUNC_OFFSET(  3185, glGetLightfv, _gloffset_GetLightfv ),
    NAME_FUNC_OFFSET(  3198, glGetLightiv, _gloffset_GetLightiv ),
    NAME_FUNC_OFFSET(  3211, glGetMapdv, _gloffset_GetMapdv ),
    NAME_FUNC_OFFSET(  3222, glGetMapfv, _gloffset_GetMapfv ),
    NAME_FUNC_OFFSET(  3233, glGetMapiv, _gloffset_GetMapiv ),
    NAME_FUNC_OFFSET(  3244, glGetMaterialfv, _gloffset_GetMaterialfv ),
    NAME_FUNC_OFFSET(  3260, glGetMaterialiv, _gloffset_GetMaterialiv ),
    NAME_FUNC_OFFSET(  3276, glGetPixelMapfv, _gloffset_GetPixelMapfv ),
    NAME_FUNC_OFFSET(  3292, glGetPixelMapuiv, _gloffset_GetPixelMapuiv ),
    NAME_FUNC_OFFSET(  3309, glGetPixelMapusv, _gloffset_GetPixelMapusv ),
    NAME_FUNC_OFFSET(  3326, glGetPolygonStipple, _gloffset_GetPolygonStipple ),
    NAME_FUNC_OFFSET(  3346, glGetString, _gloffset_GetString ),
    NAME_FUNC_OFFSET(  3358, glGetTexEnvfv, _gloffset_GetTexEnvfv ),
    NAME_FUNC_OFFSET(  3372, glGetTexEnviv, _gloffset_GetTexEnviv ),
    NAME_FUNC_OFFSET(  3386, glGetTexGendv, _gloffset_GetTexGendv ),
    NAME_FUNC_OFFSET(  3400, glGetTexGenfv, _gloffset_GetTexGenfv ),
    NAME_FUNC_OFFSET(  3414, glGetTexGeniv, _gloffset_GetTexGeniv ),
    NAME_FUNC_OFFSET(  3428, glGetTexImage, _gloffset_GetTexImage ),
    NAME_FUNC_OFFSET(  3442, glGetTexParameterfv, _gloffset_GetTexParameterfv ),
    NAME_FUNC_OFFSET(  3462, glGetTexParameteriv, _gloffset_GetTexParameteriv ),
    NAME_FUNC_OFFSET(  3482, glGetTexLevelParameterfv, _gloffset_GetTexLevelParameterfv ),
    NAME_FUNC_OFFSET(  3507, glGetTexLevelParameteriv, _gloffset_GetTexLevelParameteriv ),
    NAME_FUNC_OFFSET(  3532, glIsEnabled, _gloffset_IsEnabled ),
    NAME_FUNC_OFFSET(  3544, glIsList, _gloffset_IsList ),
    NAME_FUNC_OFFSET(  3553, glDepthRange, _gloffset_DepthRange ),
    NAME_FUNC_OFFSET(  3566, glFrustum, _gloffset_Frustum ),
    NAME_FUNC_OFFSET(  3576, glLoadIdentity, _gloffset_LoadIdentity ),
    NAME_FUNC_OFFSET(  3591, glLoadMatrixf, _gloffset_LoadMatrixf ),
    NAME_FUNC_OFFSET(  3605, glLoadMatrixd, _gloffset_LoadMatrixd ),
    NAME_FUNC_OFFSET(  3619, glMatrixMode, _gloffset_MatrixMode ),
    NAME_FUNC_OFFSET(  3632, glMultMatrixf, _gloffset_MultMatrixf ),
    NAME_FUNC_OFFSET(  3646, glMultMatrixd, _gloffset_MultMatrixd ),
    NAME_FUNC_OFFSET(  3660, glOrtho, _gloffset_Ortho ),
    NAME_FUNC_OFFSET(  3668, glPopMatrix, _gloffset_PopMatrix ),
    NAME_FUNC_OFFSET(  3680, glPushMatrix, _gloffset_PushMatrix ),
    NAME_FUNC_OFFSET(  3693, glRotated, _gloffset_Rotated ),
    NAME_FUNC_OFFSET(  3703, glRotatef, _gloffset_Rotatef ),
    NAME_FUNC_OFFSET(  3713, glScaled, _gloffset_Scaled ),
    NAME_FUNC_OFFSET(  3722, glScalef, _gloffset_Scalef ),
    NAME_FUNC_OFFSET(  3731, glTranslated, _gloffset_Translated ),
    NAME_FUNC_OFFSET(  3744, glTranslatef, _gloffset_Translatef ),
    NAME_FUNC_OFFSET(  3757, glViewport, _gloffset_Viewport ),
    NAME_FUNC_OFFSET(  3768, glArrayElement, _gloffset_ArrayElement ),
    NAME_FUNC_OFFSET(  3783, glBindTexture, _gloffset_BindTexture ),
    NAME_FUNC_OFFSET(  3797, glColorPointer, _gloffset_ColorPointer ),
    NAME_FUNC_OFFSET(  3812, glDisableClientState, _gloffset_DisableClientState ),
    NAME_FUNC_OFFSET(  3833, glDrawArrays, _gloffset_DrawArrays ),
    NAME_FUNC_OFFSET(  3846, glDrawElements, _gloffset_DrawElements ),
    NAME_FUNC_OFFSET(  3861, glEdgeFlagPointer, _gloffset_EdgeFlagPointer ),
    NAME_FUNC_OFFSET(  3879, glEnableClientState, _gloffset_EnableClientState ),
    NAME_FUNC_OFFSET(  3899, glIndexPointer, _gloffset_IndexPointer ),
    NAME_FUNC_OFFSET(  3914, glIndexub, _gloffset_Indexub ),
    NAME_FUNC_OFFSET(  3924, glIndexubv, _gloffset_Indexubv ),
    NAME_FUNC_OFFSET(  3935, glInterleavedArrays, _gloffset_InterleavedArrays ),
    NAME_FUNC_OFFSET(  3955, glNormalPointer, _gloffset_NormalPointer ),
    NAME_FUNC_OFFSET(  3971, glPolygonOffset, _gloffset_PolygonOffset ),
    NAME_FUNC_OFFSET(  3987, glTexCoordPointer, _gloffset_TexCoordPointer ),
    NAME_FUNC_OFFSET(  4005, glVertexPointer, _gloffset_VertexPointer ),
    NAME_FUNC_OFFSET(  4021, glAreTexturesResident, _gloffset_AreTexturesResident ),
    NAME_FUNC_OFFSET(  4043, glCopyTexImage1D, _gloffset_CopyTexImage1D ),
    NAME_FUNC_OFFSET(  4060, glCopyTexImage2D, _gloffset_CopyTexImage2D ),
    NAME_FUNC_OFFSET(  4077, glCopyTexSubImage1D, _gloffset_CopyTexSubImage1D ),
    NAME_FUNC_OFFSET(  4097, glCopyTexSubImage2D, _gloffset_CopyTexSubImage2D ),
    NAME_FUNC_OFFSET(  4117, glDeleteTextures, _gloffset_DeleteTextures ),
    NAME_FUNC_OFFSET(  4134, glGenTextures, _gloffset_GenTextures ),
    NAME_FUNC_OFFSET(  4148, glGetPointerv, _gloffset_GetPointerv ),
    NAME_FUNC_OFFSET(  4162, glIsTexture, _gloffset_IsTexture ),
    NAME_FUNC_OFFSET(  4174, glPrioritizeTextures, _gloffset_PrioritizeTextures ),
    NAME_FUNC_OFFSET(  4195, glTexSubImage1D, _gloffset_TexSubImage1D ),
    NAME_FUNC_OFFSET(  4211, glTexSubImage2D, _gloffset_TexSubImage2D ),
    NAME_FUNC_OFFSET(  4227, glPopClientAttrib, _gloffset_PopClientAttrib ),
    NAME_FUNC_OFFSET(  4245, glPushClientAttrib, _gloffset_PushClientAttrib ),
    NAME_FUNC_OFFSET(  4264, glBlendColor, _gloffset_BlendColor ),
    NAME_FUNC_OFFSET(  4277, glBlendEquation, _gloffset_BlendEquation ),
    NAME_FUNC_OFFSET(  4293, glDrawRangeElements, _gloffset_DrawRangeElements ),
    NAME_FUNC_OFFSET(  4313, glColorTable, _gloffset_ColorTable ),
    NAME_FUNC_OFFSET(  4326, glColorTableParameterfv, _gloffset_ColorTableParameterfv ),
    NAME_FUNC_OFFSET(  4350, glColorTableParameteriv, _gloffset_ColorTableParameteriv ),
    NAME_FUNC_OFFSET(  4374, glCopyColorTable, _gloffset_CopyColorTable ),
    NAME_FUNC_OFFSET(  4391, glGetColorTable, _gloffset_GetColorTable ),
    NAME_FUNC_OFFSET(  4407, glGetColorTableParameterfv, _gloffset_GetColorTableParameterfv ),
    NAME_FUNC_OFFSET(  4434, glGetColorTableParameteriv, _gloffset_GetColorTableParameteriv ),
    NAME_FUNC_OFFSET(  4461, glColorSubTable, _gloffset_ColorSubTable ),
    NAME_FUNC_OFFSET(  4477, glCopyColorSubTable, _gloffset_CopyColorSubTable ),
    NAME_FUNC_OFFSET(  4497, glConvolutionFilter1D, _gloffset_ConvolutionFilter1D ),
    NAME_FUNC_OFFSET(  4519, glConvolutionFilter2D, _gloffset_ConvolutionFilter2D ),
    NAME_FUNC_OFFSET(  4541, glConvolutionParameterf, _gloffset_ConvolutionParameterf ),
    NAME_FUNC_OFFSET(  4565, glConvolutionParameterfv, _gloffset_ConvolutionParameterfv ),
    NAME_FUNC_OFFSET(  4590, glConvolutionParameteri, _gloffset_ConvolutionParameteri ),
    NAME_FUNC_OFFSET(  4614, glConvolutionParameteriv, _gloffset_ConvolutionParameteriv ),
    NAME_FUNC_OFFSET(  4639, glCopyConvolutionFilter1D, _gloffset_CopyConvolutionFilter1D ),
    NAME_FUNC_OFFSET(  4665, glCopyConvolutionFilter2D, _gloffset_CopyConvolutionFilter2D ),
    NAME_FUNC_OFFSET(  4691, glGetConvolutionFilter, _gloffset_GetConvolutionFilter ),
    NAME_FUNC_OFFSET(  4714, glGetConvolutionParameterfv, _gloffset_GetConvolutionParameterfv ),
    NAME_FUNC_OFFSET(  4742, glGetConvolutionParameteriv, _gloffset_GetConvolutionParameteriv ),
    NAME_FUNC_OFFSET(  4770, glGetSeparableFilter, _gloffset_GetSeparableFilter ),
    NAME_FUNC_OFFSET(  4791, glSeparableFilter2D, _gloffset_SeparableFilter2D ),
    NAME_FUNC_OFFSET(  4811, glGetHistogram, _gloffset_GetHistogram ),
    NAME_FUNC_OFFSET(  4826, glGetHistogramParameterfv, _gloffset_GetHistogramParameterfv ),
    NAME_FUNC_OFFSET(  4852, glGetHistogramParameteriv, _gloffset_GetHistogramParameteriv ),
    NAME_FUNC_OFFSET(  4878, glGetMinmax, _gloffset_GetMinmax ),
    NAME_FUNC_OFFSET(  4890, glGetMinmaxParameterfv, _gloffset_GetMinmaxParameterfv ),
    NAME_FUNC_OFFSET(  4913, glGetMinmaxParameteriv, _gloffset_GetMinmaxParameteriv ),
    NAME_FUNC_OFFSET(  4936, glHistogram, _gloffset_Histogram ),
    NAME_FUNC_OFFSET(  4948, glMinmax, _gloffset_Minmax ),
    NAME_FUNC_OFFSET(  4957, glResetHistogram, _gloffset_ResetHistogram ),
    NAME_FUNC_OFFSET(  4974, glResetMinmax, _gloffset_ResetMinmax ),
    NAME_FUNC_OFFSET(  4988, glTexImage3D, _gloffset_TexImage3D ),
    NAME_FUNC_OFFSET(  5001, glTexSubImage3D, _gloffset_TexSubImage3D ),
    NAME_FUNC_OFFSET(  5017, glCopyTexSubImage3D, _gloffset_CopyTexSubImage3D ),
    NAME_FUNC_OFFSET(  5037, glActiveTextureARB, _gloffset_ActiveTextureARB ),
    NAME_FUNC_OFFSET(  5056, glClientActiveTextureARB, _gloffset_ClientActiveTextureARB ),
    NAME_FUNC_OFFSET(  5081, glMultiTexCoord1dARB, _gloffset_MultiTexCoord1dARB ),
    NAME_FUNC_OFFSET(  5102, glMultiTexCoord1dvARB, _gloffset_MultiTexCoord1dvARB ),
    NAME_FUNC_OFFSET(  5124, glMultiTexCoord1fARB, _gloffset_MultiTexCoord1fARB ),
    NAME_FUNC_OFFSET(  5145, glMultiTexCoord1fvARB, _gloffset_MultiTexCoord1fvARB ),
    NAME_FUNC_OFFSET(  5167, glMultiTexCoord1iARB, _gloffset_MultiTexCoord1iARB ),
    NAME_FUNC_OFFSET(  5188, glMultiTexCoord1ivARB, _gloffset_MultiTexCoord1ivARB ),
    NAME_FUNC_OFFSET(  5210, glMultiTexCoord1sARB, _gloffset_MultiTexCoord1sARB ),
    NAME_FUNC_OFFSET(  5231, glMultiTexCoord1svARB, _gloffset_MultiTexCoord1svARB ),
    NAME_FUNC_OFFSET(  5253, glMultiTexCoord2dARB, _gloffset_MultiTexCoord2dARB ),
    NAME_FUNC_OFFSET(  5274, glMultiTexCoord2dvARB, _gloffset_MultiTexCoord2dvARB ),
    NAME_FUNC_OFFSET(  5296, glMultiTexCoord2fARB, _gloffset_MultiTexCoord2fARB ),
    NAME_FUNC_OFFSET(  5317, glMultiTexCoord2fvARB, _gloffset_MultiTexCoord2fvARB ),
    NAME_FUNC_OFFSET(  5339, glMultiTexCoord2iARB, _gloffset_MultiTexCoord2iARB ),
    NAME_FUNC_OFFSET(  5360, glMultiTexCoord2ivARB, _gloffset_MultiTexCoord2ivARB ),
    NAME_FUNC_OFFSET(  5382, glMultiTexCoord2sARB, _gloffset_MultiTexCoord2sARB ),
    NAME_FUNC_OFFSET(  5403, glMultiTexCoord2svARB, _gloffset_MultiTexCoord2svARB ),
    NAME_FUNC_OFFSET(  5425, glMultiTexCoord3dARB, _gloffset_MultiTexCoord3dARB ),
    NAME_FUNC_OFFSET(  5446, glMultiTexCoord3dvARB, _gloffset_MultiTexCoord3dvARB ),
    NAME_FUNC_OFFSET(  5468, glMultiTexCoord3fARB, _gloffset_MultiTexCoord3fARB ),
    NAME_FUNC_OFFSET(  5489, glMultiTexCoord3fvARB, _gloffset_MultiTexCoord3fvARB ),
    NAME_FUNC_OFFSET(  5511, glMultiTexCoord3iARB, _gloffset_MultiTexCoord3iARB ),
    NAME_FUNC_OFFSET(  5532, glMultiTexCoord3ivARB, _gloffset_MultiTexCoord3ivARB ),
    NAME_FUNC_OFFSET(  5554, glMultiTexCoord3sARB, _gloffset_MultiTexCoord3sARB ),
    NAME_FUNC_OFFSET(  5575, glMultiTexCoord3svARB, _gloffset_MultiTexCoord3svARB ),
    NAME_FUNC_OFFSET(  5597, glMultiTexCoord4dARB, _gloffset_MultiTexCoord4dARB ),
    NAME_FUNC_OFFSET(  5618, glMultiTexCoord4dvARB, _gloffset_MultiTexCoord4dvARB ),
    NAME_FUNC_OFFSET(  5640, glMultiTexCoord4fARB, _gloffset_MultiTexCoord4fARB ),
    NAME_FUNC_OFFSET(  5661, glMultiTexCoord4fvARB, _gloffset_MultiTexCoord4fvARB ),
    NAME_FUNC_OFFSET(  5683, glMultiTexCoord4iARB, _gloffset_MultiTexCoord4iARB ),
    NAME_FUNC_OFFSET(  5704, glMultiTexCoord4ivARB, _gloffset_MultiTexCoord4ivARB ),
    NAME_FUNC_OFFSET(  5726, glMultiTexCoord4sARB, _gloffset_MultiTexCoord4sARB ),
    NAME_FUNC_OFFSET(  5747, glMultiTexCoord4svARB, _gloffset_MultiTexCoord4svARB ),
    NAME_FUNC_OFFSET(  5769, glStencilFuncSeparate, _gloffset_StencilFuncSeparate ),
    NAME_FUNC_OFFSET(  5791, glStencilMaskSeparate, _gloffset_StencilMaskSeparate ),
    NAME_FUNC_OFFSET(  5813, glStencilOpSeparate, _gloffset_StencilOpSeparate ),
    NAME_FUNC_OFFSET(  5833, glLoadTransposeMatrixdARB, _gloffset_LoadTransposeMatrixdARB ),
    NAME_FUNC_OFFSET(  5859, glLoadTransposeMatrixfARB, _gloffset_LoadTransposeMatrixfARB ),
    NAME_FUNC_OFFSET(  5885, glMultTransposeMatrixdARB, _gloffset_MultTransposeMatrixdARB ),
    NAME_FUNC_OFFSET(  5911, glMultTransposeMatrixfARB, _gloffset_MultTransposeMatrixfARB ),
    NAME_FUNC_OFFSET(  5937, glSampleCoverageARB, _gloffset_SampleCoverageARB ),
    NAME_FUNC_OFFSET(  5957, glCompressedTexImage1DARB, _gloffset_CompressedTexImage1DARB ),
    NAME_FUNC_OFFSET(  5983, glCompressedTexImage2DARB, _gloffset_CompressedTexImage2DARB ),
    NAME_FUNC_OFFSET(  6009, glCompressedTexImage3DARB, _gloffset_CompressedTexImage3DARB ),
    NAME_FUNC_OFFSET(  6035, glCompressedTexSubImage1DARB, _gloffset_CompressedTexSubImage1DARB ),
    NAME_FUNC_OFFSET(  6064, glCompressedTexSubImage2DARB, _gloffset_CompressedTexSubImage2DARB ),
    NAME_FUNC_OFFSET(  6093, glCompressedTexSubImage3DARB, _gloffset_CompressedTexSubImage3DARB ),
    NAME_FUNC_OFFSET(  6122, glGetCompressedTexImageARB, _gloffset_GetCompressedTexImageARB ),
    NAME_FUNC_OFFSET(  6149, glDisableVertexAttribArrayARB, _gloffset_DisableVertexAttribArrayARB ),
    NAME_FUNC_OFFSET(  6179, glEnableVertexAttribArrayARB, _gloffset_EnableVertexAttribArrayARB ),
    NAME_FUNC_OFFSET(  6208, glGetProgramEnvParameterdvARB, _gloffset_GetProgramEnvParameterdvARB ),
    NAME_FUNC_OFFSET(  6238, glGetProgramEnvParameterfvARB, _gloffset_GetProgramEnvParameterfvARB ),
    NAME_FUNC_OFFSET(  6268, glGetProgramLocalParameterdvARB, _gloffset_GetProgramLocalParameterdvARB ),
    NAME_FUNC_OFFSET(  6300, glGetProgramLocalParameterfvARB, _gloffset_GetProgramLocalParameterfvARB ),
    NAME_FUNC_OFFSET(  6332, glGetProgramStringARB, _gloffset_GetProgramStringARB ),
    NAME_FUNC_OFFSET(  6354, glGetProgramivARB, _gloffset_GetProgramivARB ),
    NAME_FUNC_OFFSET(  6372, glGetVertexAttribdvARB, _gloffset_GetVertexAttribdvARB ),
    NAME_FUNC_OFFSET(  6395, glGetVertexAttribfvARB, _gloffset_GetVertexAttribfvARB ),
    NAME_FUNC_OFFSET(  6418, glGetVertexAttribivARB, _gloffset_GetVertexAttribivARB ),
    NAME_FUNC_OFFSET(  6441, glProgramEnvParameter4dARB, _gloffset_ProgramEnvParameter4dARB ),
    NAME_FUNC_OFFSET(  6468, glProgramEnvParameter4dvARB, _gloffset_ProgramEnvParameter4dvARB ),
    NAME_FUNC_OFFSET(  6496, glProgramEnvParameter4fARB, _gloffset_ProgramEnvParameter4fARB ),
    NAME_FUNC_OFFSET(  6523, glProgramEnvParameter4fvARB, _gloffset_ProgramEnvParameter4fvARB ),
    NAME_FUNC_OFFSET(  6551, glProgramLocalParameter4dARB, _gloffset_ProgramLocalParameter4dARB ),
    NAME_FUNC_OFFSET(  6580, glProgramLocalParameter4dvARB, _gloffset_ProgramLocalParameter4dvARB ),
    NAME_FUNC_OFFSET(  6610, glProgramLocalParameter4fARB, _gloffset_ProgramLocalParameter4fARB ),
    NAME_FUNC_OFFSET(  6639, glProgramLocalParameter4fvARB, _gloffset_ProgramLocalParameter4fvARB ),
    NAME_FUNC_OFFSET(  6669, glProgramStringARB, _gloffset_ProgramStringARB ),
    NAME_FUNC_OFFSET(  6688, glVertexAttrib1dARB, _gloffset_VertexAttrib1dARB ),
    NAME_FUNC_OFFSET(  6708, glVertexAttrib1dvARB, _gloffset_VertexAttrib1dvARB ),
    NAME_FUNC_OFFSET(  6729, glVertexAttrib1fARB, _gloffset_VertexAttrib1fARB ),
    NAME_FUNC_OFFSET(  6749, glVertexAttrib1fvARB, _gloffset_VertexAttrib1fvARB ),
    NAME_FUNC_OFFSET(  6770, glVertexAttrib1sARB, _gloffset_VertexAttrib1sARB ),
    NAME_FUNC_OFFSET(  6790, glVertexAttrib1svARB, _gloffset_VertexAttrib1svARB ),
    NAME_FUNC_OFFSET(  6811, glVertexAttrib2dARB, _gloffset_VertexAttrib2dARB ),
    NAME_FUNC_OFFSET(  6831, glVertexAttrib2dvARB, _gloffset_VertexAttrib2dvARB ),
    NAME_FUNC_OFFSET(  6852, glVertexAttrib2fARB, _gloffset_VertexAttrib2fARB ),
    NAME_FUNC_OFFSET(  6872, glVertexAttrib2fvARB, _gloffset_VertexAttrib2fvARB ),
    NAME_FUNC_OFFSET(  6893, glVertexAttrib2sARB, _gloffset_VertexAttrib2sARB ),
    NAME_FUNC_OFFSET(  6913, glVertexAttrib2svARB, _gloffset_VertexAttrib2svARB ),
    NAME_FUNC_OFFSET(  6934, glVertexAttrib3dARB, _gloffset_VertexAttrib3dARB ),
    NAME_FUNC_OFFSET(  6954, glVertexAttrib3dvARB, _gloffset_VertexAttrib3dvARB ),
    NAME_FUNC_OFFSET(  6975, glVertexAttrib3fARB, _gloffset_VertexAttrib3fARB ),
    NAME_FUNC_OFFSET(  6995, glVertexAttrib3fvARB, _gloffset_VertexAttrib3fvARB ),
    NAME_FUNC_OFFSET(  7016, glVertexAttrib3sARB, _gloffset_VertexAttrib3sARB ),
    NAME_FUNC_OFFSET(  7036, glVertexAttrib3svARB, _gloffset_VertexAttrib3svARB ),
    NAME_FUNC_OFFSET(  7057, glVertexAttrib4NbvARB, _gloffset_VertexAttrib4NbvARB ),
    NAME_FUNC_OFFSET(  7079, glVertexAttrib4NivARB, _gloffset_VertexAttrib4NivARB ),
    NAME_FUNC_OFFSET(  7101, glVertexAttrib4NsvARB, _gloffset_VertexAttrib4NsvARB ),
    NAME_FUNC_OFFSET(  7123, glVertexAttrib4NubARB, _gloffset_VertexAttrib4NubARB ),
    NAME_FUNC_OFFSET(  7145, glVertexAttrib4NubvARB, _gloffset_VertexAttrib4NubvARB ),
    NAME_FUNC_OFFSET(  7168, glVertexAttrib4NuivARB, _gloffset_VertexAttrib4NuivARB ),
    NAME_FUNC_OFFSET(  7191, glVertexAttrib4NusvARB, _gloffset_VertexAttrib4NusvARB ),
    NAME_FUNC_OFFSET(  7214, glVertexAttrib4bvARB, _gloffset_VertexAttrib4bvARB ),
    NAME_FUNC_OFFSET(  7235, glVertexAttrib4dARB, _gloffset_VertexAttrib4dARB ),
    NAME_FUNC_OFFSET(  7255, glVertexAttrib4dvARB, _gloffset_VertexAttrib4dvARB ),
    NAME_FUNC_OFFSET(  7276, glVertexAttrib4fARB, _gloffset_VertexAttrib4fARB ),
    NAME_FUNC_OFFSET(  7296, glVertexAttrib4fvARB, _gloffset_VertexAttrib4fvARB ),
    NAME_FUNC_OFFSET(  7317, glVertexAttrib4ivARB, _gloffset_VertexAttrib4ivARB ),
    NAME_FUNC_OFFSET(  7338, glVertexAttrib4sARB, _gloffset_VertexAttrib4sARB ),
    NAME_FUNC_OFFSET(  7358, glVertexAttrib4svARB, _gloffset_VertexAttrib4svARB ),
    NAME_FUNC_OFFSET(  7379, glVertexAttrib4ubvARB, _gloffset_VertexAttrib4ubvARB ),
    NAME_FUNC_OFFSET(  7401, glVertexAttrib4uivARB, _gloffset_VertexAttrib4uivARB ),
    NAME_FUNC_OFFSET(  7423, glVertexAttrib4usvARB, _gloffset_VertexAttrib4usvARB ),
    NAME_FUNC_OFFSET(  7445, glVertexAttribPointerARB, _gloffset_VertexAttribPointerARB ),
    NAME_FUNC_OFFSET(  7470, glBindBufferARB, _gloffset_BindBufferARB ),
    NAME_FUNC_OFFSET(  7486, glBufferDataARB, _gloffset_BufferDataARB ),
    NAME_FUNC_OFFSET(  7502, glBufferSubDataARB, _gloffset_BufferSubDataARB ),
    NAME_FUNC_OFFSET(  7521, glDeleteBuffersARB, _gloffset_DeleteBuffersARB ),
    NAME_FUNC_OFFSET(  7540, glGenBuffersARB, _gloffset_GenBuffersARB ),
    NAME_FUNC_OFFSET(  7556, glGetBufferParameterivARB, _gloffset_GetBufferParameterivARB ),
    NAME_FUNC_OFFSET(  7582, glGetBufferPointervARB, _gloffset_GetBufferPointervARB ),
    NAME_FUNC_OFFSET(  7605, glGetBufferSubDataARB, _gloffset_GetBufferSubDataARB ),
    NAME_FUNC_OFFSET(  7627, glIsBufferARB, _gloffset_IsBufferARB ),
    NAME_FUNC_OFFSET(  7641, glMapBufferARB, _gloffset_MapBufferARB ),
    NAME_FUNC_OFFSET(  7656, glUnmapBufferARB, _gloffset_UnmapBufferARB ),
    NAME_FUNC_OFFSET(  7673, glBeginQueryARB, _gloffset_BeginQueryARB ),
    NAME_FUNC_OFFSET(  7689, glDeleteQueriesARB, _gloffset_DeleteQueriesARB ),
    NAME_FUNC_OFFSET(  7708, glEndQueryARB, _gloffset_EndQueryARB ),
    NAME_FUNC_OFFSET(  7722, glGenQueriesARB, _gloffset_GenQueriesARB ),
    NAME_FUNC_OFFSET(  7738, glGetQueryObjectivARB, _gloffset_GetQueryObjectivARB ),
    NAME_FUNC_OFFSET(  7760, glGetQueryObjectuivARB, _gloffset_GetQueryObjectuivARB ),
    NAME_FUNC_OFFSET(  7783, glGetQueryivARB, _gloffset_GetQueryivARB ),
    NAME_FUNC_OFFSET(  7799, glIsQueryARB, _gloffset_IsQueryARB ),
    NAME_FUNC_OFFSET(  7812, glAttachObjectARB, _gloffset_AttachObjectARB ),
    NAME_FUNC_OFFSET(  7830, glCompileShaderARB, _gloffset_CompileShaderARB ),
    NAME_FUNC_OFFSET(  7849, glCreateProgramObjectARB, _gloffset_CreateProgramObjectARB ),
    NAME_FUNC_OFFSET(  7874, glCreateShaderObjectARB, _gloffset_CreateShaderObjectARB ),
    NAME_FUNC_OFFSET(  7898, glDeleteObjectARB, _gloffset_DeleteObjectARB ),
    NAME_FUNC_OFFSET(  7916, glDetachObjectARB, _gloffset_DetachObjectARB ),
    NAME_FUNC_OFFSET(  7934, glGetActiveUniformARB, _gloffset_GetActiveUniformARB ),
    NAME_FUNC_OFFSET(  7956, glGetAttachedObjectsARB, _gloffset_GetAttachedObjectsARB ),
    NAME_FUNC_OFFSET(  7980, glGetHandleARB, _gloffset_GetHandleARB ),
    NAME_FUNC_OFFSET(  7995, glGetInfoLogARB, _gloffset_GetInfoLogARB ),
    NAME_FUNC_OFFSET(  8011, glGetObjectParameterfvARB, _gloffset_GetObjectParameterfvARB ),
    NAME_FUNC_OFFSET(  8037, glGetObjectParameterivARB, _gloffset_GetObjectParameterivARB ),
    NAME_FUNC_OFFSET(  8063, glGetShaderSourceARB, _gloffset_GetShaderSourceARB ),
    NAME_FUNC_OFFSET(  8084, glGetUniformLocationARB, _gloffset_GetUniformLocationARB ),
    NAME_FUNC_OFFSET(  8108, glGetUniformfvARB, _gloffset_GetUniformfvARB ),
    NAME_FUNC_OFFSET(  8126, glGetUniformivARB, _gloffset_GetUniformivARB ),
    NAME_FUNC_OFFSET(  8144, glLinkProgramARB, _gloffset_LinkProgramARB ),
    NAME_FUNC_OFFSET(  8161, glShaderSourceARB, _gloffset_ShaderSourceARB ),
    NAME_FUNC_OFFSET(  8179, glUniform1fARB, _gloffset_Uniform1fARB ),
    NAME_FUNC_OFFSET(  8194, glUniform1fvARB, _gloffset_Uniform1fvARB ),
    NAME_FUNC_OFFSET(  8210, glUniform1iARB, _gloffset_Uniform1iARB ),
    NAME_FUNC_OFFSET(  8225, glUniform1ivARB, _gloffset_Uniform1ivARB ),
    NAME_FUNC_OFFSET(  8241, glUniform2fARB, _gloffset_Uniform2fARB ),
    NAME_FUNC_OFFSET(  8256, glUniform2fvARB, _gloffset_Uniform2fvARB ),
    NAME_FUNC_OFFSET(  8272, glUniform2iARB, _gloffset_Uniform2iARB ),
    NAME_FUNC_OFFSET(  8287, glUniform2ivARB, _gloffset_Uniform2ivARB ),
    NAME_FUNC_OFFSET(  8303, glUniform3fARB, _gloffset_Uniform3fARB ),
    NAME_FUNC_OFFSET(  8318, glUniform3fvARB, _gloffset_Uniform3fvARB ),
    NAME_FUNC_OFFSET(  8334, glUniform3iARB, _gloffset_Uniform3iARB ),
    NAME_FUNC_OFFSET(  8349, glUniform3ivARB, _gloffset_Uniform3ivARB ),
    NAME_FUNC_OFFSET(  8365, glUniform4fARB, _gloffset_Uniform4fARB ),
    NAME_FUNC_OFFSET(  8380, glUniform4fvARB, _gloffset_Uniform4fvARB ),
    NAME_FUNC_OFFSET(  8396, glUniform4iARB, _gloffset_Uniform4iARB ),
    NAME_FUNC_OFFSET(  8411, glUniform4ivARB, _gloffset_Uniform4ivARB ),
    NAME_FUNC_OFFSET(  8427, glUniformMatrix2fvARB, _gloffset_UniformMatrix2fvARB ),
    NAME_FUNC_OFFSET(  8449, glUniformMatrix3fvARB, _gloffset_UniformMatrix3fvARB ),
    NAME_FUNC_OFFSET(  8471, glUniformMatrix4fvARB, _gloffset_UniformMatrix4fvARB ),
    NAME_FUNC_OFFSET(  8493, glUseProgramObjectARB, _gloffset_UseProgramObjectARB ),
    NAME_FUNC_OFFSET(  8515, glValidateProgramARB, _gloffset_ValidateProgramARB ),
    NAME_FUNC_OFFSET(  8536, glBindAttribLocationARB, _gloffset_BindAttribLocationARB ),
    NAME_FUNC_OFFSET(  8560, glGetActiveAttribARB, _gloffset_GetActiveAttribARB ),
    NAME_FUNC_OFFSET(  8581, glGetAttribLocationARB, _gloffset_GetAttribLocationARB ),
    NAME_FUNC_OFFSET(  8604, glDrawBuffersARB, _gloffset_DrawBuffersARB ),
    NAME_FUNC_OFFSET(  8621, glPolygonOffsetEXT, _gloffset_PolygonOffsetEXT ),
    NAME_FUNC_OFFSET(  8640, gl_dispatch_stub_543, _gloffset_GetHistogramEXT ),
    NAME_FUNC_OFFSET(  8658, gl_dispatch_stub_544, _gloffset_GetHistogramParameterfvEXT ),
    NAME_FUNC_OFFSET(  8687, gl_dispatch_stub_545, _gloffset_GetHistogramParameterivEXT ),
    NAME_FUNC_OFFSET(  8716, gl_dispatch_stub_546, _gloffset_GetMinmaxEXT ),
    NAME_FUNC_OFFSET(  8731, gl_dispatch_stub_547, _gloffset_GetMinmaxParameterfvEXT ),
    NAME_FUNC_OFFSET(  8757, gl_dispatch_stub_548, _gloffset_GetMinmaxParameterivEXT ),
    NAME_FUNC_OFFSET(  8783, gl_dispatch_stub_549, _gloffset_GetConvolutionFilterEXT ),
    NAME_FUNC_OFFSET(  8809, gl_dispatch_stub_550, _gloffset_GetConvolutionParameterfvEXT ),
    NAME_FUNC_OFFSET(  8840, gl_dispatch_stub_551, _gloffset_GetConvolutionParameterivEXT ),
    NAME_FUNC_OFFSET(  8871, gl_dispatch_stub_552, _gloffset_GetSeparableFilterEXT ),
    NAME_FUNC_OFFSET(  8895, gl_dispatch_stub_553, _gloffset_GetColorTableParameterfvSGI ),
    NAME_FUNC_OFFSET(  8925, gl_dispatch_stub_554, _gloffset_GetColorTableParameterivSGI ),
    NAME_FUNC_OFFSET(  8955, gl_dispatch_stub_555, _gloffset_GetColorTableSGI ),
    NAME_FUNC_OFFSET(  8974, gl_dispatch_stub_556, _gloffset_GetPixelTexGenParameterfvSGIS ),
    NAME_FUNC_OFFSET(  9006, gl_dispatch_stub_557, _gloffset_GetPixelTexGenParameterivSGIS ),
    NAME_FUNC_OFFSET(  9038, gl_dispatch_stub_558, _gloffset_PixelTexGenParameterfSGIS ),
    NAME_FUNC_OFFSET(  9066, gl_dispatch_stub_559, _gloffset_PixelTexGenParameterfvSGIS ),
    NAME_FUNC_OFFSET(  9095, gl_dispatch_stub_560, _gloffset_PixelTexGenParameteriSGIS ),
    NAME_FUNC_OFFSET(  9123, gl_dispatch_stub_561, _gloffset_PixelTexGenParameterivSGIS ),
    NAME_FUNC_OFFSET(  9152, glAreTexturesResidentEXT, _gloffset_AreTexturesResidentEXT ),
    NAME_FUNC_OFFSET(  9177, glGenTexturesEXT, _gloffset_GenTexturesEXT ),
    NAME_FUNC_OFFSET(  9194, glIsTextureEXT, _gloffset_IsTextureEXT ),
    NAME_FUNC_OFFSET(  9209, gl_dispatch_stub_565, _gloffset_SampleMaskSGIS ),
    NAME_FUNC_OFFSET(  9226, gl_dispatch_stub_566, _gloffset_SamplePatternSGIS ),
    NAME_FUNC_OFFSET(  9246, glColorPointerEXT, _gloffset_ColorPointerEXT ),
    NAME_FUNC_OFFSET(  9264, glEdgeFlagPointerEXT, _gloffset_EdgeFlagPointerEXT ),
    NAME_FUNC_OFFSET(  9285, glIndexPointerEXT, _gloffset_IndexPointerEXT ),
    NAME_FUNC_OFFSET(  9303, glNormalPointerEXT, _gloffset_NormalPointerEXT ),
    NAME_FUNC_OFFSET(  9322, glTexCoordPointerEXT, _gloffset_TexCoordPointerEXT ),
    NAME_FUNC_OFFSET(  9343, glVertexPointerEXT, _gloffset_VertexPointerEXT ),
    NAME_FUNC_OFFSET(  9362, glPointParameterfEXT, _gloffset_PointParameterfEXT ),
    NAME_FUNC_OFFSET(  9383, glPointParameterfvEXT, _gloffset_PointParameterfvEXT ),
    NAME_FUNC_OFFSET(  9405, glLockArraysEXT, _gloffset_LockArraysEXT ),
    NAME_FUNC_OFFSET(  9421, glUnlockArraysEXT, _gloffset_UnlockArraysEXT ),
    NAME_FUNC_OFFSET(  9439, gl_dispatch_stub_577, _gloffset_CullParameterdvEXT ),
    NAME_FUNC_OFFSET(  9460, gl_dispatch_stub_578, _gloffset_CullParameterfvEXT ),
    NAME_FUNC_OFFSET(  9481, glSecondaryColor3bEXT, _gloffset_SecondaryColor3bEXT ),
    NAME_FUNC_OFFSET(  9503, glSecondaryColor3bvEXT, _gloffset_SecondaryColor3bvEXT ),
    NAME_FUNC_OFFSET(  9526, glSecondaryColor3dEXT, _gloffset_SecondaryColor3dEXT ),
    NAME_FUNC_OFFSET(  9548, glSecondaryColor3dvEXT, _gloffset_SecondaryColor3dvEXT ),
    NAME_FUNC_OFFSET(  9571, glSecondaryColor3fEXT, _gloffset_SecondaryColor3fEXT ),
    NAME_FUNC_OFFSET(  9593, glSecondaryColor3fvEXT, _gloffset_SecondaryColor3fvEXT ),
    NAME_FUNC_OFFSET(  9616, glSecondaryColor3iEXT, _gloffset_SecondaryColor3iEXT ),
    NAME_FUNC_OFFSET(  9638, glSecondaryColor3ivEXT, _gloffset_SecondaryColor3ivEXT ),
    NAME_FUNC_OFFSET(  9661, glSecondaryColor3sEXT, _gloffset_SecondaryColor3sEXT ),
    NAME_FUNC_OFFSET(  9683, glSecondaryColor3svEXT, _gloffset_SecondaryColor3svEXT ),
    NAME_FUNC_OFFSET(  9706, glSecondaryColor3ubEXT, _gloffset_SecondaryColor3ubEXT ),
    NAME_FUNC_OFFSET(  9729, glSecondaryColor3ubvEXT, _gloffset_SecondaryColor3ubvEXT ),
    NAME_FUNC_OFFSET(  9753, glSecondaryColor3uiEXT, _gloffset_SecondaryColor3uiEXT ),
    NAME_FUNC_OFFSET(  9776, glSecondaryColor3uivEXT, _gloffset_SecondaryColor3uivEXT ),
    NAME_FUNC_OFFSET(  9800, glSecondaryColor3usEXT, _gloffset_SecondaryColor3usEXT ),
    NAME_FUNC_OFFSET(  9823, glSecondaryColor3usvEXT, _gloffset_SecondaryColor3usvEXT ),
    NAME_FUNC_OFFSET(  9847, glSecondaryColorPointerEXT, _gloffset_SecondaryColorPointerEXT ),
    NAME_FUNC_OFFSET(  9874, glMultiDrawArraysEXT, _gloffset_MultiDrawArraysEXT ),
    NAME_FUNC_OFFSET(  9895, glMultiDrawElementsEXT, _gloffset_MultiDrawElementsEXT ),
    NAME_FUNC_OFFSET(  9918, glFogCoordPointerEXT, _gloffset_FogCoordPointerEXT ),
    NAME_FUNC_OFFSET(  9939, glFogCoorddEXT, _gloffset_FogCoorddEXT ),
    NAME_FUNC_OFFSET(  9954, glFogCoorddvEXT, _gloffset_FogCoorddvEXT ),
    NAME_FUNC_OFFSET(  9970, glFogCoordfEXT, _gloffset_FogCoordfEXT ),
    NAME_FUNC_OFFSET(  9985, glFogCoordfvEXT, _gloffset_FogCoordfvEXT ),
    NAME_FUNC_OFFSET( 10001, gl_dispatch_stub_603, _gloffset_PixelTexGenSGIX ),
    NAME_FUNC_OFFSET( 10019, glBlendFuncSeparateEXT, _gloffset_BlendFuncSeparateEXT ),
    NAME_FUNC_OFFSET( 10042, glFlushVertexArrayRangeNV, _gloffset_FlushVertexArrayRangeNV ),
    NAME_FUNC_OFFSET( 10068, glVertexArrayRangeNV, _gloffset_VertexArrayRangeNV ),
    NAME_FUNC_OFFSET( 10089, glCombinerInputNV, _gloffset_CombinerInputNV ),
    NAME_FUNC_OFFSET( 10107, glCombinerOutputNV, _gloffset_CombinerOutputNV ),
    NAME_FUNC_OFFSET( 10126, glCombinerParameterfNV, _gloffset_CombinerParameterfNV ),
    NAME_FUNC_OFFSET( 10149, glCombinerParameterfvNV, _gloffset_CombinerParameterfvNV ),
    NAME_FUNC_OFFSET( 10173, glCombinerParameteriNV, _gloffset_CombinerParameteriNV ),
    NAME_FUNC_OFFSET( 10196, glCombinerParameterivNV, _gloffset_CombinerParameterivNV ),
    NAME_FUNC_OFFSET( 10220, glFinalCombinerInputNV, _gloffset_FinalCombinerInputNV ),
    NAME_FUNC_OFFSET( 10243, glGetCombinerInputParameterfvNV, _gloffset_GetCombinerInputParameterfvNV ),
    NAME_FUNC_OFFSET( 10275, glGetCombinerInputParameterivNV, _gloffset_GetCombinerInputParameterivNV ),
    NAME_FUNC_OFFSET( 10307, glGetCombinerOutputParameterfvNV, _gloffset_GetCombinerOutputParameterfvNV ),
    NAME_FUNC_OFFSET( 10340, glGetCombinerOutputParameterivNV, _gloffset_GetCombinerOutputParameterivNV ),
    NAME_FUNC_OFFSET( 10373, glGetFinalCombinerInputParameterfvNV, _gloffset_GetFinalCombinerInputParameterfvNV ),
    NAME_FUNC_OFFSET( 10410, glGetFinalCombinerInputParameterivNV, _gloffset_GetFinalCombinerInputParameterivNV ),
    NAME_FUNC_OFFSET( 10447, glResizeBuffersMESA, _gloffset_ResizeBuffersMESA ),
    NAME_FUNC_OFFSET( 10467, glWindowPos2dMESA, _gloffset_WindowPos2dMESA ),
    NAME_FUNC_OFFSET( 10485, glWindowPos2dvMESA, _gloffset_WindowPos2dvMESA ),
    NAME_FUNC_OFFSET( 10504, glWindowPos2fMESA, _gloffset_WindowPos2fMESA ),
    NAME_FUNC_OFFSET( 10522, glWindowPos2fvMESA, _gloffset_WindowPos2fvMESA ),
    NAME_FUNC_OFFSET( 10541, glWindowPos2iMESA, _gloffset_WindowPos2iMESA ),
    NAME_FUNC_OFFSET( 10559, glWindowPos2ivMESA, _gloffset_WindowPos2ivMESA ),
    NAME_FUNC_OFFSET( 10578, glWindowPos2sMESA, _gloffset_WindowPos2sMESA ),
    NAME_FUNC_OFFSET( 10596, glWindowPos2svMESA, _gloffset_WindowPos2svMESA ),
    NAME_FUNC_OFFSET( 10615, glWindowPos3dMESA, _gloffset_WindowPos3dMESA ),
    NAME_FUNC_OFFSET( 10633, glWindowPos3dvMESA, _gloffset_WindowPos3dvMESA ),
    NAME_FUNC_OFFSET( 10652, glWindowPos3fMESA, _gloffset_WindowPos3fMESA ),
    NAME_FUNC_OFFSET( 10670, glWindowPos3fvMESA, _gloffset_WindowPos3fvMESA ),
    NAME_FUNC_OFFSET( 10689, glWindowPos3iMESA, _gloffset_WindowPos3iMESA ),
    NAME_FUNC_OFFSET( 10707, glWindowPos3ivMESA, _gloffset_WindowPos3ivMESA ),
    NAME_FUNC_OFFSET( 10726, glWindowPos3sMESA, _gloffset_WindowPos3sMESA ),
    NAME_FUNC_OFFSET( 10744, glWindowPos3svMESA, _gloffset_WindowPos3svMESA ),
    NAME_FUNC_OFFSET( 10763, glWindowPos4dMESA, _gloffset_WindowPos4dMESA ),
    NAME_FUNC_OFFSET( 10781, glWindowPos4dvMESA, _gloffset_WindowPos4dvMESA ),
    NAME_FUNC_OFFSET( 10800, glWindowPos4fMESA, _gloffset_WindowPos4fMESA ),
    NAME_FUNC_OFFSET( 10818, glWindowPos4fvMESA, _gloffset_WindowPos4fvMESA ),
    NAME_FUNC_OFFSET( 10837, glWindowPos4iMESA, _gloffset_WindowPos4iMESA ),
    NAME_FUNC_OFFSET( 10855, glWindowPos4ivMESA, _gloffset_WindowPos4ivMESA ),
    NAME_FUNC_OFFSET( 10874, glWindowPos4sMESA, _gloffset_WindowPos4sMESA ),
    NAME_FUNC_OFFSET( 10892, glWindowPos4svMESA, _gloffset_WindowPos4svMESA ),
    NAME_FUNC_OFFSET( 10911, gl_dispatch_stub_645, _gloffset_MultiModeDrawArraysIBM ),
    NAME_FUNC_OFFSET( 10936, gl_dispatch_stub_646, _gloffset_MultiModeDrawElementsIBM ),
    NAME_FUNC_OFFSET( 10963, gl_dispatch_stub_647, _gloffset_DeleteFencesNV ),
    NAME_FUNC_OFFSET( 10980, gl_dispatch_stub_648, _gloffset_FinishFenceNV ),
    NAME_FUNC_OFFSET( 10996, gl_dispatch_stub_649, _gloffset_GenFencesNV ),
    NAME_FUNC_OFFSET( 11010, gl_dispatch_stub_650, _gloffset_GetFenceivNV ),
    NAME_FUNC_OFFSET( 11025, gl_dispatch_stub_651, _gloffset_IsFenceNV ),
    NAME_FUNC_OFFSET( 11037, gl_dispatch_stub_652, _gloffset_SetFenceNV ),
    NAME_FUNC_OFFSET( 11050, gl_dispatch_stub_653, _gloffset_TestFenceNV ),
    NAME_FUNC_OFFSET( 11064, glAreProgramsResidentNV, _gloffset_AreProgramsResidentNV ),
    NAME_FUNC_OFFSET( 11088, glBindProgramNV, _gloffset_BindProgramNV ),
    NAME_FUNC_OFFSET( 11104, glDeleteProgramsNV, _gloffset_DeleteProgramsNV ),
    NAME_FUNC_OFFSET( 11123, glExecuteProgramNV, _gloffset_ExecuteProgramNV ),
    NAME_FUNC_OFFSET( 11142, glGenProgramsNV, _gloffset_GenProgramsNV ),
    NAME_FUNC_OFFSET( 11158, glGetProgramParameterdvNV, _gloffset_GetProgramParameterdvNV ),
    NAME_FUNC_OFFSET( 11184, glGetProgramParameterfvNV, _gloffset_GetProgramParameterfvNV ),
    NAME_FUNC_OFFSET( 11210, glGetProgramStringNV, _gloffset_GetProgramStringNV ),
    NAME_FUNC_OFFSET( 11231, glGetProgramivNV, _gloffset_GetProgramivNV ),
    NAME_FUNC_OFFSET( 11248, glGetTrackMatrixivNV, _gloffset_GetTrackMatrixivNV ),
    NAME_FUNC_OFFSET( 11269, glGetVertexAttribPointervNV, _gloffset_GetVertexAttribPointervNV ),
    NAME_FUNC_OFFSET( 11297, glGetVertexAttribdvNV, _gloffset_GetVertexAttribdvNV ),
    NAME_FUNC_OFFSET( 11319, glGetVertexAttribfvNV, _gloffset_GetVertexAttribfvNV ),
    NAME_FUNC_OFFSET( 11341, glGetVertexAttribivNV, _gloffset_GetVertexAttribivNV ),
    NAME_FUNC_OFFSET( 11363, glIsProgramNV, _gloffset_IsProgramNV ),
    NAME_FUNC_OFFSET( 11377, glLoadProgramNV, _gloffset_LoadProgramNV ),
    NAME_FUNC_OFFSET( 11393, glProgramParameter4dNV, _gloffset_ProgramParameter4dNV ),
    NAME_FUNC_OFFSET( 11416, glProgramParameter4dvNV, _gloffset_ProgramParameter4dvNV ),
    NAME_FUNC_OFFSET( 11440, glProgramParameter4fNV, _gloffset_ProgramParameter4fNV ),
    NAME_FUNC_OFFSET( 11463, glProgramParameter4fvNV, _gloffset_ProgramParameter4fvNV ),
    NAME_FUNC_OFFSET( 11487, glProgramParameters4dvNV, _gloffset_ProgramParameters4dvNV ),
    NAME_FUNC_OFFSET( 11512, glProgramParameters4fvNV, _gloffset_ProgramParameters4fvNV ),
    NAME_FUNC_OFFSET( 11537, glRequestResidentProgramsNV, _gloffset_RequestResidentProgramsNV ),
    NAME_FUNC_OFFSET( 11565, glTrackMatrixNV, _gloffset_TrackMatrixNV ),
    NAME_FUNC_OFFSET( 11581, glVertexAttrib1dNV, _gloffset_VertexAttrib1dNV ),
    NAME_FUNC_OFFSET( 11600, glVertexAttrib1dvNV, _gloffset_VertexAttrib1dvNV ),
    NAME_FUNC_OFFSET( 11620, glVertexAttrib1fNV, _gloffset_VertexAttrib1fNV ),
    NAME_FUNC_OFFSET( 11639, glVertexAttrib1fvNV, _gloffset_VertexAttrib1fvNV ),
    NAME_FUNC_OFFSET( 11659, glVertexAttrib1sNV, _gloffset_VertexAttrib1sNV ),
    NAME_FUNC_OFFSET( 11678, glVertexAttrib1svNV, _gloffset_VertexAttrib1svNV ),
    NAME_FUNC_OFFSET( 11698, glVertexAttrib2dNV, _gloffset_VertexAttrib2dNV ),
    NAME_FUNC_OFFSET( 11717, glVertexAttrib2dvNV, _gloffset_VertexAttrib2dvNV ),
    NAME_FUNC_OFFSET( 11737, glVertexAttrib2fNV, _gloffset_VertexAttrib2fNV ),
    NAME_FUNC_OFFSET( 11756, glVertexAttrib2fvNV, _gloffset_VertexAttrib2fvNV ),
    NAME_FUNC_OFFSET( 11776, glVertexAttrib2sNV, _gloffset_VertexAttrib2sNV ),
    NAME_FUNC_OFFSET( 11795, glVertexAttrib2svNV, _gloffset_VertexAttrib2svNV ),
    NAME_FUNC_OFFSET( 11815, glVertexAttrib3dNV, _gloffset_VertexAttrib3dNV ),
    NAME_FUNC_OFFSET( 11834, glVertexAttrib3dvNV, _gloffset_VertexAttrib3dvNV ),
    NAME_FUNC_OFFSET( 11854, glVertexAttrib3fNV, _gloffset_VertexAttrib3fNV ),
    NAME_FUNC_OFFSET( 11873, glVertexAttrib3fvNV, _gloffset_VertexAttrib3fvNV ),
    NAME_FUNC_OFFSET( 11893, glVertexAttrib3sNV, _gloffset_VertexAttrib3sNV ),
    NAME_FUNC_OFFSET( 11912, glVertexAttrib3svNV, _gloffset_VertexAttrib3svNV ),
    NAME_FUNC_OFFSET( 11932, glVertexAttrib4dNV, _gloffset_VertexAttrib4dNV ),
    NAME_FUNC_OFFSET( 11951, glVertexAttrib4dvNV, _gloffset_VertexAttrib4dvNV ),
    NAME_FUNC_OFFSET( 11971, glVertexAttrib4fNV, _gloffset_VertexAttrib4fNV ),
    NAME_FUNC_OFFSET( 11990, glVertexAttrib4fvNV, _gloffset_VertexAttrib4fvNV ),
    NAME_FUNC_OFFSET( 12010, glVertexAttrib4sNV, _gloffset_VertexAttrib4sNV ),
    NAME_FUNC_OFFSET( 12029, glVertexAttrib4svNV, _gloffset_VertexAttrib4svNV ),
    NAME_FUNC_OFFSET( 12049, glVertexAttrib4ubNV, _gloffset_VertexAttrib4ubNV ),
    NAME_FUNC_OFFSET( 12069, glVertexAttrib4ubvNV, _gloffset_VertexAttrib4ubvNV ),
    NAME_FUNC_OFFSET( 12090, glVertexAttribPointerNV, _gloffset_VertexAttribPointerNV ),
    NAME_FUNC_OFFSET( 12114, glVertexAttribs1dvNV, _gloffset_VertexAttribs1dvNV ),
    NAME_FUNC_OFFSET( 12135, glVertexAttribs1fvNV, _gloffset_VertexAttribs1fvNV ),
    NAME_FUNC_OFFSET( 12156, glVertexAttribs1svNV, _gloffset_VertexAttribs1svNV ),
    NAME_FUNC_OFFSET( 12177, glVertexAttribs2dvNV, _gloffset_VertexAttribs2dvNV ),
    NAME_FUNC_OFFSET( 12198, glVertexAttribs2fvNV, _gloffset_VertexAttribs2fvNV ),
    NAME_FUNC_OFFSET( 12219, glVertexAttribs2svNV, _gloffset_VertexAttribs2svNV ),
    NAME_FUNC_OFFSET( 12240, glVertexAttribs3dvNV, _gloffset_VertexAttribs3dvNV ),
    NAME_FUNC_OFFSET( 12261, glVertexAttribs3fvNV, _gloffset_VertexAttribs3fvNV ),
    NAME_FUNC_OFFSET( 12282, glVertexAttribs3svNV, _gloffset_VertexAttribs3svNV ),
    NAME_FUNC_OFFSET( 12303, glVertexAttribs4dvNV, _gloffset_VertexAttribs4dvNV ),
    NAME_FUNC_OFFSET( 12324, glVertexAttribs4fvNV, _gloffset_VertexAttribs4fvNV ),
    NAME_FUNC_OFFSET( 12345, glVertexAttribs4svNV, _gloffset_VertexAttribs4svNV ),
    NAME_FUNC_OFFSET( 12366, glVertexAttribs4ubvNV, _gloffset_VertexAttribs4ubvNV ),
    NAME_FUNC_OFFSET( 12388, glAlphaFragmentOp1ATI, _gloffset_AlphaFragmentOp1ATI ),
    NAME_FUNC_OFFSET( 12410, glAlphaFragmentOp2ATI, _gloffset_AlphaFragmentOp2ATI ),
    NAME_FUNC_OFFSET( 12432, glAlphaFragmentOp3ATI, _gloffset_AlphaFragmentOp3ATI ),
    NAME_FUNC_OFFSET( 12454, glBeginFragmentShaderATI, _gloffset_BeginFragmentShaderATI ),
    NAME_FUNC_OFFSET( 12479, glBindFragmentShaderATI, _gloffset_BindFragmentShaderATI ),
    NAME_FUNC_OFFSET( 12503, glColorFragmentOp1ATI, _gloffset_ColorFragmentOp1ATI ),
    NAME_FUNC_OFFSET( 12525, glColorFragmentOp2ATI, _gloffset_ColorFragmentOp2ATI ),
    NAME_FUNC_OFFSET( 12547, glColorFragmentOp3ATI, _gloffset_ColorFragmentOp3ATI ),
    NAME_FUNC_OFFSET( 12569, glDeleteFragmentShaderATI, _gloffset_DeleteFragmentShaderATI ),
    NAME_FUNC_OFFSET( 12595, glEndFragmentShaderATI, _gloffset_EndFragmentShaderATI ),
    NAME_FUNC_OFFSET( 12618, glGenFragmentShadersATI, _gloffset_GenFragmentShadersATI ),
    NAME_FUNC_OFFSET( 12642, glPassTexCoordATI, _gloffset_PassTexCoordATI ),
    NAME_FUNC_OFFSET( 12660, glSampleMapATI, _gloffset_SampleMapATI ),
    NAME_FUNC_OFFSET( 12675, glSetFragmentShaderConstantATI, _gloffset_SetFragmentShaderConstantATI ),
    NAME_FUNC_OFFSET( 12706, glPointParameteriNV, _gloffset_PointParameteriNV ),
    NAME_FUNC_OFFSET( 12726, glPointParameterivNV, _gloffset_PointParameterivNV ),
    NAME_FUNC_OFFSET( 12747, gl_dispatch_stub_734, _gloffset_ActiveStencilFaceEXT ),
    NAME_FUNC_OFFSET( 12770, gl_dispatch_stub_735, _gloffset_BindVertexArrayAPPLE ),
    NAME_FUNC_OFFSET( 12793, gl_dispatch_stub_736, _gloffset_DeleteVertexArraysAPPLE ),
    NAME_FUNC_OFFSET( 12819, gl_dispatch_stub_737, _gloffset_GenVertexArraysAPPLE ),
    NAME_FUNC_OFFSET( 12842, gl_dispatch_stub_738, _gloffset_IsVertexArrayAPPLE ),
    NAME_FUNC_OFFSET( 12863, glGetProgramNamedParameterdvNV, _gloffset_GetProgramNamedParameterdvNV ),
    NAME_FUNC_OFFSET( 12894, glGetProgramNamedParameterfvNV, _gloffset_GetProgramNamedParameterfvNV ),
    NAME_FUNC_OFFSET( 12925, glProgramNamedParameter4dNV, _gloffset_ProgramNamedParameter4dNV ),
    NAME_FUNC_OFFSET( 12953, glProgramNamedParameter4dvNV, _gloffset_ProgramNamedParameter4dvNV ),
    NAME_FUNC_OFFSET( 12982, glProgramNamedParameter4fNV, _gloffset_ProgramNamedParameter4fNV ),
    NAME_FUNC_OFFSET( 13010, glProgramNamedParameter4fvNV, _gloffset_ProgramNamedParameter4fvNV ),
    NAME_FUNC_OFFSET( 13039, gl_dispatch_stub_745, _gloffset_DepthBoundsEXT ),
    NAME_FUNC_OFFSET( 13056, gl_dispatch_stub_746, _gloffset_BlendEquationSeparateEXT ),
    NAME_FUNC_OFFSET( 13083, glBindFramebufferEXT, _gloffset_BindFramebufferEXT ),
    NAME_FUNC_OFFSET( 13104, glBindRenderbufferEXT, _gloffset_BindRenderbufferEXT ),
    NAME_FUNC_OFFSET( 13126, glCheckFramebufferStatusEXT, _gloffset_CheckFramebufferStatusEXT ),
    NAME_FUNC_OFFSET( 13154, glDeleteFramebuffersEXT, _gloffset_DeleteFramebuffersEXT ),
    NAME_FUNC_OFFSET( 13178, glDeleteRenderbuffersEXT, _gloffset_DeleteRenderbuffersEXT ),
    NAME_FUNC_OFFSET( 13203, glFramebufferRenderbufferEXT, _gloffset_FramebufferRenderbufferEXT ),
    NAME_FUNC_OFFSET( 13232, glFramebufferTexture1DEXT, _gloffset_FramebufferTexture1DEXT ),
    NAME_FUNC_OFFSET( 13258, glFramebufferTexture2DEXT, _gloffset_FramebufferTexture2DEXT ),
    NAME_FUNC_OFFSET( 13284, glFramebufferTexture3DEXT, _gloffset_FramebufferTexture3DEXT ),
    NAME_FUNC_OFFSET( 13310, glGenFramebuffersEXT, _gloffset_GenFramebuffersEXT ),
    NAME_FUNC_OFFSET( 13331, glGenRenderbuffersEXT, _gloffset_GenRenderbuffersEXT ),
    NAME_FUNC_OFFSET( 13353, glGenerateMipmapEXT, _gloffset_GenerateMipmapEXT ),
    NAME_FUNC_OFFSET( 13373, glGetFramebufferAttachmentParameterivEXT, _gloffset_GetFramebufferAttachmentParameterivEXT ),
    NAME_FUNC_OFFSET( 13414, gl_dispatch_stub_760, _gloffset_GetQueryObjecti64vEXT ),
    NAME_FUNC_OFFSET( 13438, gl_dispatch_stub_761, _gloffset_GetQueryObjectui64vEXT ),
    NAME_FUNC_OFFSET( 13463, glGetRenderbufferParameterivEXT, _gloffset_GetRenderbufferParameterivEXT ),
    NAME_FUNC_OFFSET( 13495, glIsFramebufferEXT, _gloffset_IsFramebufferEXT ),
    NAME_FUNC_OFFSET( 13514, glIsRenderbufferEXT, _gloffset_IsRenderbufferEXT ),
    NAME_FUNC_OFFSET( 13534, glRenderbufferStorageEXT, _gloffset_RenderbufferStorageEXT ),
    NAME_FUNC_OFFSET( 13559, gl_dispatch_stub_766, _gloffset_BlitFramebufferEXT ),
    NAME_FUNC_OFFSET( 13580, gl_dispatch_stub_767, _gloffset_ProgramEnvParameters4fvEXT ),
    NAME_FUNC_OFFSET( 13609, gl_dispatch_stub_768, _gloffset_ProgramLocalParameters4fvEXT ),
    NAME_FUNC_OFFSET( 13640, glArrayElement, _gloffset_ArrayElement ),
    NAME_FUNC_OFFSET( 13658, glBindTexture, _gloffset_BindTexture ),
    NAME_FUNC_OFFSET( 13675, glDrawArrays, _gloffset_DrawArrays ),
    NAME_FUNC_OFFSET( 13691, glCopyTexImage1D, _gloffset_CopyTexImage1D ),
    NAME_FUNC_OFFSET( 13711, glCopyTexImage2D, _gloffset_CopyTexImage2D ),
    NAME_FUNC_OFFSET( 13731, glCopyTexSubImage1D, _gloffset_CopyTexSubImage1D ),
    NAME_FUNC_OFFSET( 13754, glCopyTexSubImage2D, _gloffset_CopyTexSubImage2D ),
    NAME_FUNC_OFFSET( 13777, glDeleteTextures, _gloffset_DeleteTextures ),
    NAME_FUNC_OFFSET( 13797, glGetPointerv, _gloffset_GetPointerv ),
    NAME_FUNC_OFFSET( 13814, glPrioritizeTextures, _gloffset_PrioritizeTextures ),
    NAME_FUNC_OFFSET( 13838, glTexSubImage1D, _gloffset_TexSubImage1D ),
    NAME_FUNC_OFFSET( 13857, glTexSubImage2D, _gloffset_TexSubImage2D ),
    NAME_FUNC_OFFSET( 13876, glBlendColor, _gloffset_BlendColor ),
    NAME_FUNC_OFFSET( 13892, glBlendEquation, _gloffset_BlendEquation ),
    NAME_FUNC_OFFSET( 13911, glDrawRangeElements, _gloffset_DrawRangeElements ),
    NAME_FUNC_OFFSET( 13934, glColorTable, _gloffset_ColorTable ),
    NAME_FUNC_OFFSET( 13950, glColorTable, _gloffset_ColorTable ),
    NAME_FUNC_OFFSET( 13966, glColorTableParameterfv, _gloffset_ColorTableParameterfv ),
    NAME_FUNC_OFFSET( 13993, glColorTableParameteriv, _gloffset_ColorTableParameteriv ),
    NAME_FUNC_OFFSET( 14020, glCopyColorTable, _gloffset_CopyColorTable ),
    NAME_FUNC_OFFSET( 14040, glColorSubTable, _gloffset_ColorSubTable ),
    NAME_FUNC_OFFSET( 14059, glCopyColorSubTable, _gloffset_CopyColorSubTable ),
    NAME_FUNC_OFFSET( 14082, glConvolutionFilter1D, _gloffset_ConvolutionFilter1D ),
    NAME_FUNC_OFFSET( 14107, glConvolutionFilter2D, _gloffset_ConvolutionFilter2D ),
    NAME_FUNC_OFFSET( 14132, glConvolutionParameterf, _gloffset_ConvolutionParameterf ),
    NAME_FUNC_OFFSET( 14159, glConvolutionParameterfv, _gloffset_ConvolutionParameterfv ),
    NAME_FUNC_OFFSET( 14187, glConvolutionParameteri, _gloffset_ConvolutionParameteri ),
    NAME_FUNC_OFFSET( 14214, glConvolutionParameteriv, _gloffset_ConvolutionParameteriv ),
    NAME_FUNC_OFFSET( 14242, glCopyConvolutionFilter1D, _gloffset_CopyConvolutionFilter1D ),
    NAME_FUNC_OFFSET( 14271, glCopyConvolutionFilter2D, _gloffset_CopyConvolutionFilter2D ),
    NAME_FUNC_OFFSET( 14300, glSeparableFilter2D, _gloffset_SeparableFilter2D ),
    NAME_FUNC_OFFSET( 14323, glHistogram, _gloffset_Histogram ),
    NAME_FUNC_OFFSET( 14338, glMinmax, _gloffset_Minmax ),
    NAME_FUNC_OFFSET( 14350, glResetHistogram, _gloffset_ResetHistogram ),
    NAME_FUNC_OFFSET( 14370, glResetMinmax, _gloffset_ResetMinmax ),
    NAME_FUNC_OFFSET( 14387, glTexImage3D, _gloffset_TexImage3D ),
    NAME_FUNC_OFFSET( 14403, glTexSubImage3D, _gloffset_TexSubImage3D ),
    NAME_FUNC_OFFSET( 14422, glCopyTexSubImage3D, _gloffset_CopyTexSubImage3D ),
    NAME_FUNC_OFFSET( 14445, glActiveTextureARB, _gloffset_ActiveTextureARB ),
    NAME_FUNC_OFFSET( 14461, glClientActiveTextureARB, _gloffset_ClientActiveTextureARB ),
    NAME_FUNC_OFFSET( 14483, glMultiTexCoord1dARB, _gloffset_MultiTexCoord1dARB ),
    NAME_FUNC_OFFSET( 14501, glMultiTexCoord1dvARB, _gloffset_MultiTexCoord1dvARB ),
    NAME_FUNC_OFFSET( 14520, glMultiTexCoord1fARB, _gloffset_MultiTexCoord1fARB ),
    NAME_FUNC_OFFSET( 14538, glMultiTexCoord1fvARB, _gloffset_MultiTexCoord1fvARB ),
    NAME_FUNC_OFFSET( 14557, glMultiTexCoord1iARB, _gloffset_MultiTexCoord1iARB ),
    NAME_FUNC_OFFSET( 14575, glMultiTexCoord1ivARB, _gloffset_MultiTexCoord1ivARB ),
    NAME_FUNC_OFFSET( 14594, glMultiTexCoord1sARB, _gloffset_MultiTexCoord1sARB ),
    NAME_FUNC_OFFSET( 14612, glMultiTexCoord1svARB, _gloffset_MultiTexCoord1svARB ),
    NAME_FUNC_OFFSET( 14631, glMultiTexCoord2dARB, _gloffset_MultiTexCoord2dARB ),
    NAME_FUNC_OFFSET( 14649, glMultiTexCoord2dvARB, _gloffset_MultiTexCoord2dvARB ),
    NAME_FUNC_OFFSET( 14668, glMultiTexCoord2fARB, _gloffset_MultiTexCoord2fARB ),
    NAME_FUNC_OFFSET( 14686, glMultiTexCoord2fvARB, _gloffset_MultiTexCoord2fvARB ),
    NAME_FUNC_OFFSET( 14705, glMultiTexCoord2iARB, _gloffset_MultiTexCoord2iARB ),
    NAME_FUNC_OFFSET( 14723, glMultiTexCoord2ivARB, _gloffset_MultiTexCoord2ivARB ),
    NAME_FUNC_OFFSET( 14742, glMultiTexCoord2sARB, _gloffset_MultiTexCoord2sARB ),
    NAME_FUNC_OFFSET( 14760, glMultiTexCoord2svARB, _gloffset_MultiTexCoord2svARB ),
    NAME_FUNC_OFFSET( 14779, glMultiTexCoord3dARB, _gloffset_MultiTexCoord3dARB ),
    NAME_FUNC_OFFSET( 14797, glMultiTexCoord3dvARB, _gloffset_MultiTexCoord3dvARB ),
    NAME_FUNC_OFFSET( 14816, glMultiTexCoord3fARB, _gloffset_MultiTexCoord3fARB ),
    NAME_FUNC_OFFSET( 14834, glMultiTexCoord3fvARB, _gloffset_MultiTexCoord3fvARB ),
    NAME_FUNC_OFFSET( 14853, glMultiTexCoord3iARB, _gloffset_MultiTexCoord3iARB ),
    NAME_FUNC_OFFSET( 14871, glMultiTexCoord3ivARB, _gloffset_MultiTexCoord3ivARB ),
    NAME_FUNC_OFFSET( 14890, glMultiTexCoord3sARB, _gloffset_MultiTexCoord3sARB ),
    NAME_FUNC_OFFSET( 14908, glMultiTexCoord3svARB, _gloffset_MultiTexCoord3svARB ),
    NAME_FUNC_OFFSET( 14927, glMultiTexCoord4dARB, _gloffset_MultiTexCoord4dARB ),
    NAME_FUNC_OFFSET( 14945, glMultiTexCoord4dvARB, _gloffset_MultiTexCoord4dvARB ),
    NAME_FUNC_OFFSET( 14964, glMultiTexCoord4fARB, _gloffset_MultiTexCoord4fARB ),
    NAME_FUNC_OFFSET( 14982, glMultiTexCoord4fvARB, _gloffset_MultiTexCoord4fvARB ),
    NAME_FUNC_OFFSET( 15001, glMultiTexCoord4iARB, _gloffset_MultiTexCoord4iARB ),
    NAME_FUNC_OFFSET( 15019, glMultiTexCoord4ivARB, _gloffset_MultiTexCoord4ivARB ),
    NAME_FUNC_OFFSET( 15038, glMultiTexCoord4sARB, _gloffset_MultiTexCoord4sARB ),
    NAME_FUNC_OFFSET( 15056, glMultiTexCoord4svARB, _gloffset_MultiTexCoord4svARB ),
    NAME_FUNC_OFFSET( 15075, glLoadTransposeMatrixdARB, _gloffset_LoadTransposeMatrixdARB ),
    NAME_FUNC_OFFSET( 15098, glLoadTransposeMatrixfARB, _gloffset_LoadTransposeMatrixfARB ),
    NAME_FUNC_OFFSET( 15121, glMultTransposeMatrixdARB, _gloffset_MultTransposeMatrixdARB ),
    NAME_FUNC_OFFSET( 15144, glMultTransposeMatrixfARB, _gloffset_MultTransposeMatrixfARB ),
    NAME_FUNC_OFFSET( 15167, glSampleCoverageARB, _gloffset_SampleCoverageARB ),
    NAME_FUNC_OFFSET( 15184, glCompressedTexImage1DARB, _gloffset_CompressedTexImage1DARB ),
    NAME_FUNC_OFFSET( 15207, glCompressedTexImage2DARB, _gloffset_CompressedTexImage2DARB ),
    NAME_FUNC_OFFSET( 15230, glCompressedTexImage3DARB, _gloffset_CompressedTexImage3DARB ),
    NAME_FUNC_OFFSET( 15253, glCompressedTexSubImage1DARB, _gloffset_CompressedTexSubImage1DARB ),
    NAME_FUNC_OFFSET( 15279, glCompressedTexSubImage2DARB, _gloffset_CompressedTexSubImage2DARB ),
    NAME_FUNC_OFFSET( 15305, glCompressedTexSubImage3DARB, _gloffset_CompressedTexSubImage3DARB ),
    NAME_FUNC_OFFSET( 15331, glGetCompressedTexImageARB, _gloffset_GetCompressedTexImageARB ),
    NAME_FUNC_OFFSET( 15355, glBindBufferARB, _gloffset_BindBufferARB ),
    NAME_FUNC_OFFSET( 15368, glBufferDataARB, _gloffset_BufferDataARB ),
    NAME_FUNC_OFFSET( 15381, glBufferSubDataARB, _gloffset_BufferSubDataARB ),
    NAME_FUNC_OFFSET( 15397, glDeleteBuffersARB, _gloffset_DeleteBuffersARB ),
    NAME_FUNC_OFFSET( 15413, glGenBuffersARB, _gloffset_GenBuffersARB ),
    NAME_FUNC_OFFSET( 15426, glGetBufferParameterivARB, _gloffset_GetBufferParameterivARB ),
    NAME_FUNC_OFFSET( 15449, glGetBufferPointervARB, _gloffset_GetBufferPointervARB ),
    NAME_FUNC_OFFSET( 15469, glGetBufferSubDataARB, _gloffset_GetBufferSubDataARB ),
    NAME_FUNC_OFFSET( 15488, glIsBufferARB, _gloffset_IsBufferARB ),
    NAME_FUNC_OFFSET( 15499, glMapBufferARB, _gloffset_MapBufferARB ),
    NAME_FUNC_OFFSET( 15511, glUnmapBufferARB, _gloffset_UnmapBufferARB ),
    NAME_FUNC_OFFSET( 15525, glBeginQueryARB, _gloffset_BeginQueryARB ),
    NAME_FUNC_OFFSET( 15538, glDeleteQueriesARB, _gloffset_DeleteQueriesARB ),
    NAME_FUNC_OFFSET( 15554, glEndQueryARB, _gloffset_EndQueryARB ),
    NAME_FUNC_OFFSET( 15565, glGenQueriesARB, _gloffset_GenQueriesARB ),
    NAME_FUNC_OFFSET( 15578, glGetQueryObjectivARB, _gloffset_GetQueryObjectivARB ),
    NAME_FUNC_OFFSET( 15597, glGetQueryObjectuivARB, _gloffset_GetQueryObjectuivARB ),
    NAME_FUNC_OFFSET( 15617, glGetQueryivARB, _gloffset_GetQueryivARB ),
    NAME_FUNC_OFFSET( 15630, glIsQueryARB, _gloffset_IsQueryARB ),
    NAME_FUNC_OFFSET( 15640, glDrawBuffersARB, _gloffset_DrawBuffersARB ),
    NAME_FUNC_OFFSET( 15654, glDrawBuffersARB, _gloffset_DrawBuffersARB ),
    NAME_FUNC_OFFSET( 15671, gl_dispatch_stub_553, _gloffset_GetColorTableParameterfvSGI ),
    NAME_FUNC_OFFSET( 15701, gl_dispatch_stub_554, _gloffset_GetColorTableParameterivSGI ),
    NAME_FUNC_OFFSET( 15731, gl_dispatch_stub_555, _gloffset_GetColorTableSGI ),
    NAME_FUNC_OFFSET( 15750, gl_dispatch_stub_565, _gloffset_SampleMaskSGIS ),
    NAME_FUNC_OFFSET( 15766, gl_dispatch_stub_566, _gloffset_SamplePatternSGIS ),
    NAME_FUNC_OFFSET( 15785, glPointParameterfEXT, _gloffset_PointParameterfEXT ),
    NAME_FUNC_OFFSET( 15803, glPointParameterfEXT, _gloffset_PointParameterfEXT ),
    NAME_FUNC_OFFSET( 15824, glPointParameterfEXT, _gloffset_PointParameterfEXT ),
    NAME_FUNC_OFFSET( 15846, glPointParameterfvEXT, _gloffset_PointParameterfvEXT ),
    NAME_FUNC_OFFSET( 15865, glPointParameterfvEXT, _gloffset_PointParameterfvEXT ),
    NAME_FUNC_OFFSET( 15887, glPointParameterfvEXT, _gloffset_PointParameterfvEXT ),
    NAME_FUNC_OFFSET( 15910, glSecondaryColor3bEXT, _gloffset_SecondaryColor3bEXT ),
    NAME_FUNC_OFFSET( 15929, glSecondaryColor3bvEXT, _gloffset_SecondaryColor3bvEXT ),
    NAME_FUNC_OFFSET( 15949, glSecondaryColor3dEXT, _gloffset_SecondaryColor3dEXT ),
    NAME_FUNC_OFFSET( 15968, glSecondaryColor3dvEXT, _gloffset_SecondaryColor3dvEXT ),
    NAME_FUNC_OFFSET( 15988, glSecondaryColor3fEXT, _gloffset_SecondaryColor3fEXT ),
    NAME_FUNC_OFFSET( 16007, glSecondaryColor3fvEXT, _gloffset_SecondaryColor3fvEXT ),
    NAME_FUNC_OFFSET( 16027, glSecondaryColor3iEXT, _gloffset_SecondaryColor3iEXT ),
    NAME_FUNC_OFFSET( 16046, glSecondaryColor3ivEXT, _gloffset_SecondaryColor3ivEXT ),
    NAME_FUNC_OFFSET( 16066, glSecondaryColor3sEXT, _gloffset_SecondaryColor3sEXT ),
    NAME_FUNC_OFFSET( 16085, glSecondaryColor3svEXT, _gloffset_SecondaryColor3svEXT ),
    NAME_FUNC_OFFSET( 16105, glSecondaryColor3ubEXT, _gloffset_SecondaryColor3ubEXT ),
    NAME_FUNC_OFFSET( 16125, glSecondaryColor3ubvEXT, _gloffset_SecondaryColor3ubvEXT ),
    NAME_FUNC_OFFSET( 16146, glSecondaryColor3uiEXT, _gloffset_SecondaryColor3uiEXT ),
    NAME_FUNC_OFFSET( 16166, glSecondaryColor3uivEXT, _gloffset_SecondaryColor3uivEXT ),
    NAME_FUNC_OFFSET( 16187, glSecondaryColor3usEXT, _gloffset_SecondaryColor3usEXT ),
    NAME_FUNC_OFFSET( 16207, glSecondaryColor3usvEXT, _gloffset_SecondaryColor3usvEXT ),
    NAME_FUNC_OFFSET( 16228, glSecondaryColorPointerEXT, _gloffset_SecondaryColorPointerEXT ),
    NAME_FUNC_OFFSET( 16252, glMultiDrawArraysEXT, _gloffset_MultiDrawArraysEXT ),
    NAME_FUNC_OFFSET( 16270, glMultiDrawElementsEXT, _gloffset_MultiDrawElementsEXT ),
    NAME_FUNC_OFFSET( 16290, glFogCoordPointerEXT, _gloffset_FogCoordPointerEXT ),
    NAME_FUNC_OFFSET( 16308, glFogCoorddEXT, _gloffset_FogCoorddEXT ),
    NAME_FUNC_OFFSET( 16320, glFogCoorddvEXT, _gloffset_FogCoorddvEXT ),
    NAME_FUNC_OFFSET( 16333, glFogCoordfEXT, _gloffset_FogCoordfEXT ),
    NAME_FUNC_OFFSET( 16345, glFogCoordfvEXT, _gloffset_FogCoordfvEXT ),
    NAME_FUNC_OFFSET( 16358, glBlendFuncSeparateEXT, _gloffset_BlendFuncSeparateEXT ),
    NAME_FUNC_OFFSET( 16378, glBlendFuncSeparateEXT, _gloffset_BlendFuncSeparateEXT ),
    NAME_FUNC_OFFSET( 16402, glWindowPos2dMESA, _gloffset_WindowPos2dMESA ),
    NAME_FUNC_OFFSET( 16416, glWindowPos2dMESA, _gloffset_WindowPos2dMESA ),
    NAME_FUNC_OFFSET( 16433, glWindowPos2dvMESA, _gloffset_WindowPos2dvMESA ),
    NAME_FUNC_OFFSET( 16448, glWindowPos2dvMESA, _gloffset_WindowPos2dvMESA ),
    NAME_FUNC_OFFSET( 16466, glWindowPos2fMESA, _gloffset_WindowPos2fMESA ),
    NAME_FUNC_OFFSET( 16480, glWindowPos2fMESA, _gloffset_WindowPos2fMESA ),
    NAME_FUNC_OFFSET( 16497, glWindowPos2fvMESA, _gloffset_WindowPos2fvMESA ),
    NAME_FUNC_OFFSET( 16512, glWindowPos2fvMESA, _gloffset_WindowPos2fvMESA ),
    NAME_FUNC_OFFSET( 16530, glWindowPos2iMESA, _gloffset_WindowPos2iMESA ),
    NAME_FUNC_OFFSET( 16544, glWindowPos2iMESA, _gloffset_WindowPos2iMESA ),
    NAME_FUNC_OFFSET( 16561, glWindowPos2ivMESA, _gloffset_WindowPos2ivMESA ),
    NAME_FUNC_OFFSET( 16576, glWindowPos2ivMESA, _gloffset_WindowPos2ivMESA ),
    NAME_FUNC_OFFSET( 16594, glWindowPos2sMESA, _gloffset_WindowPos2sMESA ),
    NAME_FUNC_OFFSET( 16608, glWindowPos2sMESA, _gloffset_WindowPos2sMESA ),
    NAME_FUNC_OFFSET( 16625, glWindowPos2svMESA, _gloffset_WindowPos2svMESA ),
    NAME_FUNC_OFFSET( 16640, glWindowPos2svMESA, _gloffset_WindowPos2svMESA ),
    NAME_FUNC_OFFSET( 16658, glWindowPos3dMESA, _gloffset_WindowPos3dMESA ),
    NAME_FUNC_OFFSET( 16672, glWindowPos3dMESA, _gloffset_WindowPos3dMESA ),
    NAME_FUNC_OFFSET( 16689, glWindowPos3dvMESA, _gloffset_WindowPos3dvMESA ),
    NAME_FUNC_OFFSET( 16704, glWindowPos3dvMESA, _gloffset_WindowPos3dvMESA ),
    NAME_FUNC_OFFSET( 16722, glWindowPos3fMESA, _gloffset_WindowPos3fMESA ),
    NAME_FUNC_OFFSET( 16736, glWindowPos3fMESA, _gloffset_WindowPos3fMESA ),
    NAME_FUNC_OFFSET( 16753, glWindowPos3fvMESA, _gloffset_WindowPos3fvMESA ),
    NAME_FUNC_OFFSET( 16768, glWindowPos3fvMESA, _gloffset_WindowPos3fvMESA ),
    NAME_FUNC_OFFSET( 16786, glWindowPos3iMESA, _gloffset_WindowPos3iMESA ),
    NAME_FUNC_OFFSET( 16800, glWindowPos3iMESA, _gloffset_WindowPos3iMESA ),
    NAME_FUNC_OFFSET( 16817, glWindowPos3ivMESA, _gloffset_WindowPos3ivMESA ),
    NAME_FUNC_OFFSET( 16832, glWindowPos3ivMESA, _gloffset_WindowPos3ivMESA ),
    NAME_FUNC_OFFSET( 16850, glWindowPos3sMESA, _gloffset_WindowPos3sMESA ),
    NAME_FUNC_OFFSET( 16864, glWindowPos3sMESA, _gloffset_WindowPos3sMESA ),
    NAME_FUNC_OFFSET( 16881, glWindowPos3svMESA, _gloffset_WindowPos3svMESA ),
    NAME_FUNC_OFFSET( 16896, glWindowPos3svMESA, _gloffset_WindowPos3svMESA ),
    NAME_FUNC_OFFSET( 16914, glBindProgramNV, _gloffset_BindProgramNV ),
    NAME_FUNC_OFFSET( 16931, glDeleteProgramsNV, _gloffset_DeleteProgramsNV ),
    NAME_FUNC_OFFSET( 16951, glGenProgramsNV, _gloffset_GenProgramsNV ),
    NAME_FUNC_OFFSET( 16968, glGetVertexAttribPointervNV, _gloffset_GetVertexAttribPointervNV ),
    NAME_FUNC_OFFSET( 16997, glIsProgramNV, _gloffset_IsProgramNV ),
    NAME_FUNC_OFFSET( 17012, glPointParameteriNV, _gloffset_PointParameteriNV ),
    NAME_FUNC_OFFSET( 17030, glPointParameterivNV, _gloffset_PointParameterivNV ),
    NAME_FUNC_OFFSET( 17049, gl_dispatch_stub_746, _gloffset_BlendEquationSeparateEXT ),
    NAME_FUNC_OFFSET( 17073, gl_dispatch_stub_746, _gloffset_BlendEquationSeparateEXT ),
    NAME_FUNC_OFFSET( -1, NULL, 0 )
};

#undef NAME_FUNC_OFFSET
