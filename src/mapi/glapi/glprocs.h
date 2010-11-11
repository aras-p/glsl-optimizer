/* DO NOT EDIT - This file generated automatically by gl_procs.py (from Mesa) script */

/*
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
 * (C) Copyright IBM Corporation 2004, 2006
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
#if defined(NEED_FUNCTION_POINTER) || defined(GLX_INDIRECT_RENDERING)
    _glapi_proc Address;
#endif
    GLuint Offset;
} glprocs_table_t;

#if   !defined(NEED_FUNCTION_POINTER) && !defined(GLX_INDIRECT_RENDERING)
#  define NAME_FUNC_OFFSET(n,f1,f2,f3,o) { n , o }
#elif  defined(NEED_FUNCTION_POINTER) && !defined(GLX_INDIRECT_RENDERING)
#  define NAME_FUNC_OFFSET(n,f1,f2,f3,o) { n , (_glapi_proc) f1 , o }
#elif  defined(NEED_FUNCTION_POINTER) &&  defined(GLX_INDIRECT_RENDERING)
#  define NAME_FUNC_OFFSET(n,f1,f2,f3,o) { n , (_glapi_proc) f2 , o }
#elif !defined(NEED_FUNCTION_POINTER) &&  defined(GLX_INDIRECT_RENDERING)
#  define NAME_FUNC_OFFSET(n,f1,f2,f3,o) { n , (_glapi_proc) f3 , o }
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
    "glAttachShader\0"
    "glCreateProgram\0"
    "glCreateShader\0"
    "glDeleteProgram\0"
    "glDeleteShader\0"
    "glDetachShader\0"
    "glGetAttachedShaders\0"
    "glGetProgramInfoLog\0"
    "glGetProgramiv\0"
    "glGetShaderInfoLog\0"
    "glGetShaderiv\0"
    "glIsProgram\0"
    "glIsShader\0"
    "glStencilFuncSeparate\0"
    "glStencilMaskSeparate\0"
    "glStencilOpSeparate\0"
    "glUniformMatrix2x3fv\0"
    "glUniformMatrix2x4fv\0"
    "glUniformMatrix3x2fv\0"
    "glUniformMatrix3x4fv\0"
    "glUniformMatrix4x2fv\0"
    "glUniformMatrix4x3fv\0"
    "glDrawArraysInstanced\0"
    "glDrawElementsInstanced\0"
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
    "glRenderbufferStorageMultisample\0"
    "glFramebufferTextureARB\0"
    "glFramebufferTextureFaceARB\0"
    "glProgramParameteriARB\0"
    "glFlushMappedBufferRange\0"
    "glMapBufferRange\0"
    "glBindVertexArray\0"
    "glGenVertexArrays\0"
    "glCopyBufferSubData\0"
    "glClientWaitSync\0"
    "glDeleteSync\0"
    "glFenceSync\0"
    "glGetInteger64v\0"
    "glGetSynciv\0"
    "glIsSync\0"
    "glWaitSync\0"
    "glDrawElementsBaseVertex\0"
    "glDrawRangeElementsBaseVertex\0"
    "glMultiDrawElementsBaseVertex\0"
    "glBindTransformFeedback\0"
    "glDeleteTransformFeedbacks\0"
    "glDrawTransformFeedback\0"
    "glGenTransformFeedbacks\0"
    "glIsTransformFeedback\0"
    "glPauseTransformFeedback\0"
    "glResumeTransformFeedback\0"
    "glPolygonOffsetEXT\0"
    "glGetPixelTexGenParameterfvSGIS\0"
    "glGetPixelTexGenParameterivSGIS\0"
    "glPixelTexGenParameterfSGIS\0"
    "glPixelTexGenParameterfvSGIS\0"
    "glPixelTexGenParameteriSGIS\0"
    "glPixelTexGenParameterivSGIS\0"
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
    "glGetTexBumpParameterfvATI\0"
    "glGetTexBumpParameterivATI\0"
    "glTexBumpParameterfvATI\0"
    "glTexBumpParameterivATI\0"
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
    "glPrimitiveRestartIndexNV\0"
    "glPrimitiveRestartNV\0"
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
    "glGetRenderbufferParameterivEXT\0"
    "glIsFramebufferEXT\0"
    "glIsRenderbufferEXT\0"
    "glRenderbufferStorageEXT\0"
    "glBlitFramebufferEXT\0"
    "glBufferParameteriAPPLE\0"
    "glFlushMappedBufferRangeAPPLE\0"
    "glBindFragDataLocationEXT\0"
    "glGetFragDataLocationEXT\0"
    "glGetUniformuivEXT\0"
    "glGetVertexAttribIivEXT\0"
    "glGetVertexAttribIuivEXT\0"
    "glUniform1uiEXT\0"
    "glUniform1uivEXT\0"
    "glUniform2uiEXT\0"
    "glUniform2uivEXT\0"
    "glUniform3uiEXT\0"
    "glUniform3uivEXT\0"
    "glUniform4uiEXT\0"
    "glUniform4uivEXT\0"
    "glVertexAttribI1iEXT\0"
    "glVertexAttribI1ivEXT\0"
    "glVertexAttribI1uiEXT\0"
    "glVertexAttribI1uivEXT\0"
    "glVertexAttribI2iEXT\0"
    "glVertexAttribI2ivEXT\0"
    "glVertexAttribI2uiEXT\0"
    "glVertexAttribI2uivEXT\0"
    "glVertexAttribI3iEXT\0"
    "glVertexAttribI3ivEXT\0"
    "glVertexAttribI3uiEXT\0"
    "glVertexAttribI3uivEXT\0"
    "glVertexAttribI4bvEXT\0"
    "glVertexAttribI4iEXT\0"
    "glVertexAttribI4ivEXT\0"
    "glVertexAttribI4svEXT\0"
    "glVertexAttribI4ubvEXT\0"
    "glVertexAttribI4uiEXT\0"
    "glVertexAttribI4uivEXT\0"
    "glVertexAttribI4usvEXT\0"
    "glVertexAttribIPointerEXT\0"
    "glFramebufferTextureLayerEXT\0"
    "glColorMaskIndexedEXT\0"
    "glDisableIndexedEXT\0"
    "glEnableIndexedEXT\0"
    "glGetBooleanIndexedvEXT\0"
    "glGetIntegerIndexedvEXT\0"
    "glIsEnabledIndexedEXT\0"
    "glClearColorIiEXT\0"
    "glClearColorIuiEXT\0"
    "glGetTexParameterIivEXT\0"
    "glGetTexParameterIuivEXT\0"
    "glTexParameterIivEXT\0"
    "glTexParameterIuivEXT\0"
    "glBeginConditionalRenderNV\0"
    "glEndConditionalRenderNV\0"
    "glBeginTransformFeedbackEXT\0"
    "glBindBufferBaseEXT\0"
    "glBindBufferOffsetEXT\0"
    "glBindBufferRangeEXT\0"
    "glEndTransformFeedbackEXT\0"
    "glGetTransformFeedbackVaryingEXT\0"
    "glTransformFeedbackVaryingsEXT\0"
    "glProvokingVertexEXT\0"
    "glGetTexParameterPointervAPPLE\0"
    "glTextureRangeAPPLE\0"
    "glGetObjectParameterivAPPLE\0"
    "glObjectPurgeableAPPLE\0"
    "glObjectUnpurgeableAPPLE\0"
    "glActiveProgramEXT\0"
    "glCreateShaderProgramEXT\0"
    "glUseShaderProgramEXT\0"
    "glStencilFuncSeparateATI\0"
    "glProgramEnvParameters4fvEXT\0"
    "glProgramLocalParameters4fvEXT\0"
    "glGetQueryObjecti64vEXT\0"
    "glGetQueryObjectui64vEXT\0"
    "glEGLImageTargetRenderbufferStorageOES\0"
    "glEGLImageTargetTexture2DOES\0"
    "glArrayElementEXT\0"
    "glBindTextureEXT\0"
    "glDrawArraysEXT\0"
    "glAreTexturesResidentEXT\0"
    "glCopyTexImage1DEXT\0"
    "glCopyTexImage2DEXT\0"
    "glCopyTexSubImage1DEXT\0"
    "glCopyTexSubImage2DEXT\0"
    "glDeleteTexturesEXT\0"
    "glGenTexturesEXT\0"
    "glGetPointervEXT\0"
    "glIsTextureEXT\0"
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
    "glGetColorTableSGI\0"
    "glGetColorTableEXT\0"
    "glGetColorTableParameterfvSGI\0"
    "glGetColorTableParameterfvEXT\0"
    "glGetColorTableParameterivSGI\0"
    "glGetColorTableParameterivEXT\0"
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
    "glGetConvolutionFilterEXT\0"
    "glGetConvolutionParameterfvEXT\0"
    "glGetConvolutionParameterivEXT\0"
    "glGetSeparableFilterEXT\0"
    "glSeparableFilter2DEXT\0"
    "glGetHistogramEXT\0"
    "glGetHistogramParameterfvEXT\0"
    "glGetHistogramParameterivEXT\0"
    "glGetMinmaxEXT\0"
    "glGetMinmaxParameterfvEXT\0"
    "glGetMinmaxParameterivEXT\0"
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
    "glStencilOpSeparateATI\0"
    "glDrawArraysInstancedARB\0"
    "glDrawArraysInstancedEXT\0"
    "glDrawElementsInstancedARB\0"
    "glDrawElementsInstancedEXT\0"
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
    "glDisableVertexAttribArray\0"
    "glEnableVertexAttribArray\0"
    "glGetVertexAttribdv\0"
    "glGetVertexAttribfv\0"
    "glGetVertexAttribiv\0"
    "glProgramParameter4dNV\0"
    "glProgramParameter4dvNV\0"
    "glProgramParameter4fNV\0"
    "glProgramParameter4fvNV\0"
    "glVertexAttrib1d\0"
    "glVertexAttrib1dv\0"
    "glVertexAttrib1f\0"
    "glVertexAttrib1fv\0"
    "glVertexAttrib1s\0"
    "glVertexAttrib1sv\0"
    "glVertexAttrib2d\0"
    "glVertexAttrib2dv\0"
    "glVertexAttrib2f\0"
    "glVertexAttrib2fv\0"
    "glVertexAttrib2s\0"
    "glVertexAttrib2sv\0"
    "glVertexAttrib3d\0"
    "glVertexAttrib3dv\0"
    "glVertexAttrib3f\0"
    "glVertexAttrib3fv\0"
    "glVertexAttrib3s\0"
    "glVertexAttrib3sv\0"
    "glVertexAttrib4Nbv\0"
    "glVertexAttrib4Niv\0"
    "glVertexAttrib4Nsv\0"
    "glVertexAttrib4Nub\0"
    "glVertexAttrib4Nubv\0"
    "glVertexAttrib4Nuiv\0"
    "glVertexAttrib4Nusv\0"
    "glVertexAttrib4bv\0"
    "glVertexAttrib4d\0"
    "glVertexAttrib4dv\0"
    "glVertexAttrib4f\0"
    "glVertexAttrib4fv\0"
    "glVertexAttrib4iv\0"
    "glVertexAttrib4s\0"
    "glVertexAttrib4sv\0"
    "glVertexAttrib4ubv\0"
    "glVertexAttrib4uiv\0"
    "glVertexAttrib4usv\0"
    "glVertexAttribPointer\0"
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
    "glCompileShader\0"
    "glGetActiveUniform\0"
    "glGetShaderSource\0"
    "glGetUniformLocation\0"
    "glGetUniformfv\0"
    "glGetUniformiv\0"
    "glLinkProgram\0"
    "glShaderSource\0"
    "glUniform1f\0"
    "glUniform1fv\0"
    "glUniform1i\0"
    "glUniform1iv\0"
    "glUniform2f\0"
    "glUniform2fv\0"
    "glUniform2i\0"
    "glUniform2iv\0"
    "glUniform3f\0"
    "glUniform3fv\0"
    "glUniform3i\0"
    "glUniform3iv\0"
    "glUniform4f\0"
    "glUniform4fv\0"
    "glUniform4i\0"
    "glUniform4iv\0"
    "glUniformMatrix2fv\0"
    "glUniformMatrix3fv\0"
    "glUniformMatrix4fv\0"
    "glUseProgram\0"
    "glValidateProgram\0"
    "glBindAttribLocation\0"
    "glGetActiveAttrib\0"
    "glGetAttribLocation\0"
    "glDrawBuffers\0"
    "glDrawBuffersATI\0"
    "glRenderbufferStorageMultisampleEXT\0"
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
    "glGetVertexAttribPointerv\0"
    "glGetVertexAttribPointervARB\0"
    "glIsProgramARB\0"
    "glPointParameteri\0"
    "glPointParameteriv\0"
    "glDeleteVertexArrays\0"
    "glIsVertexArray\0"
    "glBlendEquationSeparate\0"
    "glBlendEquationSeparateATI\0"
    "glBindFramebuffer\0"
    "glBindRenderbuffer\0"
    "glCheckFramebufferStatus\0"
    "glDeleteFramebuffers\0"
    "glDeleteRenderbuffers\0"
    "glFramebufferRenderbuffer\0"
    "glFramebufferTexture1D\0"
    "glFramebufferTexture2D\0"
    "glFramebufferTexture3D\0"
    "glGenFramebuffers\0"
    "glGenRenderbuffers\0"
    "glGenerateMipmap\0"
    "glGetFramebufferAttachmentParameteriv\0"
    "glGetRenderbufferParameteriv\0"
    "glIsFramebuffer\0"
    "glIsRenderbuffer\0"
    "glRenderbufferStorage\0"
    "glBlitFramebuffer\0"
    "glFramebufferTextureLayer\0"
    "glBeginTransformFeedback\0"
    "glBindBufferBase\0"
    "glBindBufferRange\0"
    "glEndTransformFeedback\0"
    "glGetTransformFeedbackVarying\0"
    "glTransformFeedbackVaryings\0"
    "glProvokingVertex\0"
    ;


#ifdef USE_MGL_NAMESPACE
#define gl_dispatch_stub_343 mgl_dispatch_stub_343
#define gl_dispatch_stub_344 mgl_dispatch_stub_344
#define gl_dispatch_stub_345 mgl_dispatch_stub_345
#define gl_dispatch_stub_356 mgl_dispatch_stub_356
#define gl_dispatch_stub_357 mgl_dispatch_stub_357
#define gl_dispatch_stub_358 mgl_dispatch_stub_358
#define gl_dispatch_stub_359 mgl_dispatch_stub_359
#define gl_dispatch_stub_361 mgl_dispatch_stub_361
#define gl_dispatch_stub_362 mgl_dispatch_stub_362
#define gl_dispatch_stub_363 mgl_dispatch_stub_363
#define gl_dispatch_stub_364 mgl_dispatch_stub_364
#define gl_dispatch_stub_365 mgl_dispatch_stub_365
#define gl_dispatch_stub_366 mgl_dispatch_stub_366
#define gl_dispatch_stub_590 mgl_dispatch_stub_590
#define gl_dispatch_stub_591 mgl_dispatch_stub_591
#define gl_dispatch_stub_592 mgl_dispatch_stub_592
#define gl_dispatch_stub_593 mgl_dispatch_stub_593
#define gl_dispatch_stub_594 mgl_dispatch_stub_594
#define gl_dispatch_stub_595 mgl_dispatch_stub_595
#define gl_dispatch_stub_596 mgl_dispatch_stub_596
#define gl_dispatch_stub_597 mgl_dispatch_stub_597
#define gl_dispatch_stub_632 mgl_dispatch_stub_632
#define gl_dispatch_stub_674 mgl_dispatch_stub_674
#define gl_dispatch_stub_675 mgl_dispatch_stub_675
#define gl_dispatch_stub_676 mgl_dispatch_stub_676
#define gl_dispatch_stub_677 mgl_dispatch_stub_677
#define gl_dispatch_stub_678 mgl_dispatch_stub_678
#define gl_dispatch_stub_679 mgl_dispatch_stub_679
#define gl_dispatch_stub_680 mgl_dispatch_stub_680
#define gl_dispatch_stub_681 mgl_dispatch_stub_681
#define gl_dispatch_stub_682 mgl_dispatch_stub_682
#define gl_dispatch_stub_763 mgl_dispatch_stub_763
#define gl_dispatch_stub_764 mgl_dispatch_stub_764
#define gl_dispatch_stub_765 mgl_dispatch_stub_765
#define gl_dispatch_stub_766 mgl_dispatch_stub_766
#define gl_dispatch_stub_767 mgl_dispatch_stub_767
#define gl_dispatch_stub_776 mgl_dispatch_stub_776
#define gl_dispatch_stub_777 mgl_dispatch_stub_777
#define gl_dispatch_stub_795 mgl_dispatch_stub_795
#define gl_dispatch_stub_796 mgl_dispatch_stub_796
#define gl_dispatch_stub_797 mgl_dispatch_stub_797
#define gl_dispatch_stub_855 mgl_dispatch_stub_855
#define gl_dispatch_stub_856 mgl_dispatch_stub_856
#define gl_dispatch_stub_863 mgl_dispatch_stub_863
#define gl_dispatch_stub_864 mgl_dispatch_stub_864
#define gl_dispatch_stub_865 mgl_dispatch_stub_865
#define gl_dispatch_stub_866 mgl_dispatch_stub_866
#define gl_dispatch_stub_867 mgl_dispatch_stub_867
#endif /* USE_MGL_NAMESPACE */


#if defined(NEED_FUNCTION_POINTER) || defined(GLX_INDIRECT_RENDERING)
void GLAPIENTRY gl_dispatch_stub_343(GLenum target, GLenum format, GLenum type, GLvoid * table);
void GLAPIENTRY gl_dispatch_stub_344(GLenum target, GLenum pname, GLfloat * params);
void GLAPIENTRY gl_dispatch_stub_345(GLenum target, GLenum pname, GLint * params);
void GLAPIENTRY gl_dispatch_stub_356(GLenum target, GLenum format, GLenum type, GLvoid * image);
void GLAPIENTRY gl_dispatch_stub_357(GLenum target, GLenum pname, GLfloat * params);
void GLAPIENTRY gl_dispatch_stub_358(GLenum target, GLenum pname, GLint * params);
void GLAPIENTRY gl_dispatch_stub_359(GLenum target, GLenum format, GLenum type, GLvoid * row, GLvoid * column, GLvoid * span);
void GLAPIENTRY gl_dispatch_stub_361(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values);
void GLAPIENTRY gl_dispatch_stub_362(GLenum target, GLenum pname, GLfloat * params);
void GLAPIENTRY gl_dispatch_stub_363(GLenum target, GLenum pname, GLint * params);
void GLAPIENTRY gl_dispatch_stub_364(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid * values);
void GLAPIENTRY gl_dispatch_stub_365(GLenum target, GLenum pname, GLfloat * params);
void GLAPIENTRY gl_dispatch_stub_366(GLenum target, GLenum pname, GLint * params);
void GLAPIENTRY gl_dispatch_stub_590(GLenum pname, GLfloat * params);
void GLAPIENTRY gl_dispatch_stub_591(GLenum pname, GLint * params);
void GLAPIENTRY gl_dispatch_stub_592(GLenum pname, GLfloat param);
void GLAPIENTRY gl_dispatch_stub_593(GLenum pname, const GLfloat * params);
void GLAPIENTRY gl_dispatch_stub_594(GLenum pname, GLint param);
void GLAPIENTRY gl_dispatch_stub_595(GLenum pname, const GLint * params);
void GLAPIENTRY gl_dispatch_stub_596(GLclampf value, GLboolean invert);
void GLAPIENTRY gl_dispatch_stub_597(GLenum pattern);
void GLAPIENTRY gl_dispatch_stub_632(GLenum mode);
void GLAPIENTRY gl_dispatch_stub_674(const GLenum * mode, const GLint * first, const GLsizei * count, GLsizei primcount, GLint modestride);
void GLAPIENTRY gl_dispatch_stub_675(const GLenum * mode, const GLsizei * count, GLenum type, const GLvoid * const * indices, GLsizei primcount, GLint modestride);
void GLAPIENTRY gl_dispatch_stub_676(GLsizei n, const GLuint * fences);
void GLAPIENTRY gl_dispatch_stub_677(GLuint fence);
void GLAPIENTRY gl_dispatch_stub_678(GLsizei n, GLuint * fences);
void GLAPIENTRY gl_dispatch_stub_679(GLuint fence, GLenum pname, GLint * params);
GLboolean GLAPIENTRY gl_dispatch_stub_680(GLuint fence);
void GLAPIENTRY gl_dispatch_stub_681(GLuint fence, GLenum condition);
GLboolean GLAPIENTRY gl_dispatch_stub_682(GLuint fence);
void GLAPIENTRY gl_dispatch_stub_763(GLenum face);
void GLAPIENTRY gl_dispatch_stub_764(GLuint array);
void GLAPIENTRY gl_dispatch_stub_765(GLsizei n, const GLuint * arrays);
void GLAPIENTRY gl_dispatch_stub_766(GLsizei n, GLuint * arrays);
GLboolean GLAPIENTRY gl_dispatch_stub_767(GLuint array);
void GLAPIENTRY gl_dispatch_stub_776(GLclampd zmin, GLclampd zmax);
void GLAPIENTRY gl_dispatch_stub_777(GLenum modeRGB, GLenum modeA);
void GLAPIENTRY gl_dispatch_stub_795(GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
void GLAPIENTRY gl_dispatch_stub_796(GLenum target, GLenum pname, GLint param);
void GLAPIENTRY gl_dispatch_stub_797(GLenum target, GLintptr offset, GLsizeiptr size);
void GLAPIENTRY gl_dispatch_stub_855(GLenum target, GLenum pname, GLvoid ** params);
void GLAPIENTRY gl_dispatch_stub_856(GLenum target, GLsizei length, GLvoid * pointer);
void GLAPIENTRY gl_dispatch_stub_863(GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask);
void GLAPIENTRY gl_dispatch_stub_864(GLenum target, GLuint index, GLsizei count, const GLfloat * params);
void GLAPIENTRY gl_dispatch_stub_865(GLenum target, GLuint index, GLsizei count, const GLfloat * params);
void GLAPIENTRY gl_dispatch_stub_866(GLuint id, GLenum pname, GLint64EXT * params);
void GLAPIENTRY gl_dispatch_stub_867(GLuint id, GLenum pname, GLuint64EXT * params);
#endif /* defined(NEED_FUNCTION_POINTER) || defined(GLX_INDIRECT_RENDERING) */

static const glprocs_table_t static_functions[] = {
    NAME_FUNC_OFFSET(    0, glNewList, glNewList, NULL, 0),
    NAME_FUNC_OFFSET(   10, glEndList, glEndList, NULL, 1),
    NAME_FUNC_OFFSET(   20, glCallList, glCallList, NULL, 2),
    NAME_FUNC_OFFSET(   31, glCallLists, glCallLists, NULL, 3),
    NAME_FUNC_OFFSET(   43, glDeleteLists, glDeleteLists, NULL, 4),
    NAME_FUNC_OFFSET(   57, glGenLists, glGenLists, NULL, 5),
    NAME_FUNC_OFFSET(   68, glListBase, glListBase, NULL, 6),
    NAME_FUNC_OFFSET(   79, glBegin, glBegin, NULL, 7),
    NAME_FUNC_OFFSET(   87, glBitmap, glBitmap, NULL, 8),
    NAME_FUNC_OFFSET(   96, glColor3b, glColor3b, NULL, 9),
    NAME_FUNC_OFFSET(  106, glColor3bv, glColor3bv, NULL, 10),
    NAME_FUNC_OFFSET(  117, glColor3d, glColor3d, NULL, 11),
    NAME_FUNC_OFFSET(  127, glColor3dv, glColor3dv, NULL, 12),
    NAME_FUNC_OFFSET(  138, glColor3f, glColor3f, NULL, 13),
    NAME_FUNC_OFFSET(  148, glColor3fv, glColor3fv, NULL, 14),
    NAME_FUNC_OFFSET(  159, glColor3i, glColor3i, NULL, 15),
    NAME_FUNC_OFFSET(  169, glColor3iv, glColor3iv, NULL, 16),
    NAME_FUNC_OFFSET(  180, glColor3s, glColor3s, NULL, 17),
    NAME_FUNC_OFFSET(  190, glColor3sv, glColor3sv, NULL, 18),
    NAME_FUNC_OFFSET(  201, glColor3ub, glColor3ub, NULL, 19),
    NAME_FUNC_OFFSET(  212, glColor3ubv, glColor3ubv, NULL, 20),
    NAME_FUNC_OFFSET(  224, glColor3ui, glColor3ui, NULL, 21),
    NAME_FUNC_OFFSET(  235, glColor3uiv, glColor3uiv, NULL, 22),
    NAME_FUNC_OFFSET(  247, glColor3us, glColor3us, NULL, 23),
    NAME_FUNC_OFFSET(  258, glColor3usv, glColor3usv, NULL, 24),
    NAME_FUNC_OFFSET(  270, glColor4b, glColor4b, NULL, 25),
    NAME_FUNC_OFFSET(  280, glColor4bv, glColor4bv, NULL, 26),
    NAME_FUNC_OFFSET(  291, glColor4d, glColor4d, NULL, 27),
    NAME_FUNC_OFFSET(  301, glColor4dv, glColor4dv, NULL, 28),
    NAME_FUNC_OFFSET(  312, glColor4f, glColor4f, NULL, 29),
    NAME_FUNC_OFFSET(  322, glColor4fv, glColor4fv, NULL, 30),
    NAME_FUNC_OFFSET(  333, glColor4i, glColor4i, NULL, 31),
    NAME_FUNC_OFFSET(  343, glColor4iv, glColor4iv, NULL, 32),
    NAME_FUNC_OFFSET(  354, glColor4s, glColor4s, NULL, 33),
    NAME_FUNC_OFFSET(  364, glColor4sv, glColor4sv, NULL, 34),
    NAME_FUNC_OFFSET(  375, glColor4ub, glColor4ub, NULL, 35),
    NAME_FUNC_OFFSET(  386, glColor4ubv, glColor4ubv, NULL, 36),
    NAME_FUNC_OFFSET(  398, glColor4ui, glColor4ui, NULL, 37),
    NAME_FUNC_OFFSET(  409, glColor4uiv, glColor4uiv, NULL, 38),
    NAME_FUNC_OFFSET(  421, glColor4us, glColor4us, NULL, 39),
    NAME_FUNC_OFFSET(  432, glColor4usv, glColor4usv, NULL, 40),
    NAME_FUNC_OFFSET(  444, glEdgeFlag, glEdgeFlag, NULL, 41),
    NAME_FUNC_OFFSET(  455, glEdgeFlagv, glEdgeFlagv, NULL, 42),
    NAME_FUNC_OFFSET(  467, glEnd, glEnd, NULL, 43),
    NAME_FUNC_OFFSET(  473, glIndexd, glIndexd, NULL, 44),
    NAME_FUNC_OFFSET(  482, glIndexdv, glIndexdv, NULL, 45),
    NAME_FUNC_OFFSET(  492, glIndexf, glIndexf, NULL, 46),
    NAME_FUNC_OFFSET(  501, glIndexfv, glIndexfv, NULL, 47),
    NAME_FUNC_OFFSET(  511, glIndexi, glIndexi, NULL, 48),
    NAME_FUNC_OFFSET(  520, glIndexiv, glIndexiv, NULL, 49),
    NAME_FUNC_OFFSET(  530, glIndexs, glIndexs, NULL, 50),
    NAME_FUNC_OFFSET(  539, glIndexsv, glIndexsv, NULL, 51),
    NAME_FUNC_OFFSET(  549, glNormal3b, glNormal3b, NULL, 52),
    NAME_FUNC_OFFSET(  560, glNormal3bv, glNormal3bv, NULL, 53),
    NAME_FUNC_OFFSET(  572, glNormal3d, glNormal3d, NULL, 54),
    NAME_FUNC_OFFSET(  583, glNormal3dv, glNormal3dv, NULL, 55),
    NAME_FUNC_OFFSET(  595, glNormal3f, glNormal3f, NULL, 56),
    NAME_FUNC_OFFSET(  606, glNormal3fv, glNormal3fv, NULL, 57),
    NAME_FUNC_OFFSET(  618, glNormal3i, glNormal3i, NULL, 58),
    NAME_FUNC_OFFSET(  629, glNormal3iv, glNormal3iv, NULL, 59),
    NAME_FUNC_OFFSET(  641, glNormal3s, glNormal3s, NULL, 60),
    NAME_FUNC_OFFSET(  652, glNormal3sv, glNormal3sv, NULL, 61),
    NAME_FUNC_OFFSET(  664, glRasterPos2d, glRasterPos2d, NULL, 62),
    NAME_FUNC_OFFSET(  678, glRasterPos2dv, glRasterPos2dv, NULL, 63),
    NAME_FUNC_OFFSET(  693, glRasterPos2f, glRasterPos2f, NULL, 64),
    NAME_FUNC_OFFSET(  707, glRasterPos2fv, glRasterPos2fv, NULL, 65),
    NAME_FUNC_OFFSET(  722, glRasterPos2i, glRasterPos2i, NULL, 66),
    NAME_FUNC_OFFSET(  736, glRasterPos2iv, glRasterPos2iv, NULL, 67),
    NAME_FUNC_OFFSET(  751, glRasterPos2s, glRasterPos2s, NULL, 68),
    NAME_FUNC_OFFSET(  765, glRasterPos2sv, glRasterPos2sv, NULL, 69),
    NAME_FUNC_OFFSET(  780, glRasterPos3d, glRasterPos3d, NULL, 70),
    NAME_FUNC_OFFSET(  794, glRasterPos3dv, glRasterPos3dv, NULL, 71),
    NAME_FUNC_OFFSET(  809, glRasterPos3f, glRasterPos3f, NULL, 72),
    NAME_FUNC_OFFSET(  823, glRasterPos3fv, glRasterPos3fv, NULL, 73),
    NAME_FUNC_OFFSET(  838, glRasterPos3i, glRasterPos3i, NULL, 74),
    NAME_FUNC_OFFSET(  852, glRasterPos3iv, glRasterPos3iv, NULL, 75),
    NAME_FUNC_OFFSET(  867, glRasterPos3s, glRasterPos3s, NULL, 76),
    NAME_FUNC_OFFSET(  881, glRasterPos3sv, glRasterPos3sv, NULL, 77),
    NAME_FUNC_OFFSET(  896, glRasterPos4d, glRasterPos4d, NULL, 78),
    NAME_FUNC_OFFSET(  910, glRasterPos4dv, glRasterPos4dv, NULL, 79),
    NAME_FUNC_OFFSET(  925, glRasterPos4f, glRasterPos4f, NULL, 80),
    NAME_FUNC_OFFSET(  939, glRasterPos4fv, glRasterPos4fv, NULL, 81),
    NAME_FUNC_OFFSET(  954, glRasterPos4i, glRasterPos4i, NULL, 82),
    NAME_FUNC_OFFSET(  968, glRasterPos4iv, glRasterPos4iv, NULL, 83),
    NAME_FUNC_OFFSET(  983, glRasterPos4s, glRasterPos4s, NULL, 84),
    NAME_FUNC_OFFSET(  997, glRasterPos4sv, glRasterPos4sv, NULL, 85),
    NAME_FUNC_OFFSET( 1012, glRectd, glRectd, NULL, 86),
    NAME_FUNC_OFFSET( 1020, glRectdv, glRectdv, NULL, 87),
    NAME_FUNC_OFFSET( 1029, glRectf, glRectf, NULL, 88),
    NAME_FUNC_OFFSET( 1037, glRectfv, glRectfv, NULL, 89),
    NAME_FUNC_OFFSET( 1046, glRecti, glRecti, NULL, 90),
    NAME_FUNC_OFFSET( 1054, glRectiv, glRectiv, NULL, 91),
    NAME_FUNC_OFFSET( 1063, glRects, glRects, NULL, 92),
    NAME_FUNC_OFFSET( 1071, glRectsv, glRectsv, NULL, 93),
    NAME_FUNC_OFFSET( 1080, glTexCoord1d, glTexCoord1d, NULL, 94),
    NAME_FUNC_OFFSET( 1093, glTexCoord1dv, glTexCoord1dv, NULL, 95),
    NAME_FUNC_OFFSET( 1107, glTexCoord1f, glTexCoord1f, NULL, 96),
    NAME_FUNC_OFFSET( 1120, glTexCoord1fv, glTexCoord1fv, NULL, 97),
    NAME_FUNC_OFFSET( 1134, glTexCoord1i, glTexCoord1i, NULL, 98),
    NAME_FUNC_OFFSET( 1147, glTexCoord1iv, glTexCoord1iv, NULL, 99),
    NAME_FUNC_OFFSET( 1161, glTexCoord1s, glTexCoord1s, NULL, 100),
    NAME_FUNC_OFFSET( 1174, glTexCoord1sv, glTexCoord1sv, NULL, 101),
    NAME_FUNC_OFFSET( 1188, glTexCoord2d, glTexCoord2d, NULL, 102),
    NAME_FUNC_OFFSET( 1201, glTexCoord2dv, glTexCoord2dv, NULL, 103),
    NAME_FUNC_OFFSET( 1215, glTexCoord2f, glTexCoord2f, NULL, 104),
    NAME_FUNC_OFFSET( 1228, glTexCoord2fv, glTexCoord2fv, NULL, 105),
    NAME_FUNC_OFFSET( 1242, glTexCoord2i, glTexCoord2i, NULL, 106),
    NAME_FUNC_OFFSET( 1255, glTexCoord2iv, glTexCoord2iv, NULL, 107),
    NAME_FUNC_OFFSET( 1269, glTexCoord2s, glTexCoord2s, NULL, 108),
    NAME_FUNC_OFFSET( 1282, glTexCoord2sv, glTexCoord2sv, NULL, 109),
    NAME_FUNC_OFFSET( 1296, glTexCoord3d, glTexCoord3d, NULL, 110),
    NAME_FUNC_OFFSET( 1309, glTexCoord3dv, glTexCoord3dv, NULL, 111),
    NAME_FUNC_OFFSET( 1323, glTexCoord3f, glTexCoord3f, NULL, 112),
    NAME_FUNC_OFFSET( 1336, glTexCoord3fv, glTexCoord3fv, NULL, 113),
    NAME_FUNC_OFFSET( 1350, glTexCoord3i, glTexCoord3i, NULL, 114),
    NAME_FUNC_OFFSET( 1363, glTexCoord3iv, glTexCoord3iv, NULL, 115),
    NAME_FUNC_OFFSET( 1377, glTexCoord3s, glTexCoord3s, NULL, 116),
    NAME_FUNC_OFFSET( 1390, glTexCoord3sv, glTexCoord3sv, NULL, 117),
    NAME_FUNC_OFFSET( 1404, glTexCoord4d, glTexCoord4d, NULL, 118),
    NAME_FUNC_OFFSET( 1417, glTexCoord4dv, glTexCoord4dv, NULL, 119),
    NAME_FUNC_OFFSET( 1431, glTexCoord4f, glTexCoord4f, NULL, 120),
    NAME_FUNC_OFFSET( 1444, glTexCoord4fv, glTexCoord4fv, NULL, 121),
    NAME_FUNC_OFFSET( 1458, glTexCoord4i, glTexCoord4i, NULL, 122),
    NAME_FUNC_OFFSET( 1471, glTexCoord4iv, glTexCoord4iv, NULL, 123),
    NAME_FUNC_OFFSET( 1485, glTexCoord4s, glTexCoord4s, NULL, 124),
    NAME_FUNC_OFFSET( 1498, glTexCoord4sv, glTexCoord4sv, NULL, 125),
    NAME_FUNC_OFFSET( 1512, glVertex2d, glVertex2d, NULL, 126),
    NAME_FUNC_OFFSET( 1523, glVertex2dv, glVertex2dv, NULL, 127),
    NAME_FUNC_OFFSET( 1535, glVertex2f, glVertex2f, NULL, 128),
    NAME_FUNC_OFFSET( 1546, glVertex2fv, glVertex2fv, NULL, 129),
    NAME_FUNC_OFFSET( 1558, glVertex2i, glVertex2i, NULL, 130),
    NAME_FUNC_OFFSET( 1569, glVertex2iv, glVertex2iv, NULL, 131),
    NAME_FUNC_OFFSET( 1581, glVertex2s, glVertex2s, NULL, 132),
    NAME_FUNC_OFFSET( 1592, glVertex2sv, glVertex2sv, NULL, 133),
    NAME_FUNC_OFFSET( 1604, glVertex3d, glVertex3d, NULL, 134),
    NAME_FUNC_OFFSET( 1615, glVertex3dv, glVertex3dv, NULL, 135),
    NAME_FUNC_OFFSET( 1627, glVertex3f, glVertex3f, NULL, 136),
    NAME_FUNC_OFFSET( 1638, glVertex3fv, glVertex3fv, NULL, 137),
    NAME_FUNC_OFFSET( 1650, glVertex3i, glVertex3i, NULL, 138),
    NAME_FUNC_OFFSET( 1661, glVertex3iv, glVertex3iv, NULL, 139),
    NAME_FUNC_OFFSET( 1673, glVertex3s, glVertex3s, NULL, 140),
    NAME_FUNC_OFFSET( 1684, glVertex3sv, glVertex3sv, NULL, 141),
    NAME_FUNC_OFFSET( 1696, glVertex4d, glVertex4d, NULL, 142),
    NAME_FUNC_OFFSET( 1707, glVertex4dv, glVertex4dv, NULL, 143),
    NAME_FUNC_OFFSET( 1719, glVertex4f, glVertex4f, NULL, 144),
    NAME_FUNC_OFFSET( 1730, glVertex4fv, glVertex4fv, NULL, 145),
    NAME_FUNC_OFFSET( 1742, glVertex4i, glVertex4i, NULL, 146),
    NAME_FUNC_OFFSET( 1753, glVertex4iv, glVertex4iv, NULL, 147),
    NAME_FUNC_OFFSET( 1765, glVertex4s, glVertex4s, NULL, 148),
    NAME_FUNC_OFFSET( 1776, glVertex4sv, glVertex4sv, NULL, 149),
    NAME_FUNC_OFFSET( 1788, glClipPlane, glClipPlane, NULL, 150),
    NAME_FUNC_OFFSET( 1800, glColorMaterial, glColorMaterial, NULL, 151),
    NAME_FUNC_OFFSET( 1816, glCullFace, glCullFace, NULL, 152),
    NAME_FUNC_OFFSET( 1827, glFogf, glFogf, NULL, 153),
    NAME_FUNC_OFFSET( 1834, glFogfv, glFogfv, NULL, 154),
    NAME_FUNC_OFFSET( 1842, glFogi, glFogi, NULL, 155),
    NAME_FUNC_OFFSET( 1849, glFogiv, glFogiv, NULL, 156),
    NAME_FUNC_OFFSET( 1857, glFrontFace, glFrontFace, NULL, 157),
    NAME_FUNC_OFFSET( 1869, glHint, glHint, NULL, 158),
    NAME_FUNC_OFFSET( 1876, glLightf, glLightf, NULL, 159),
    NAME_FUNC_OFFSET( 1885, glLightfv, glLightfv, NULL, 160),
    NAME_FUNC_OFFSET( 1895, glLighti, glLighti, NULL, 161),
    NAME_FUNC_OFFSET( 1904, glLightiv, glLightiv, NULL, 162),
    NAME_FUNC_OFFSET( 1914, glLightModelf, glLightModelf, NULL, 163),
    NAME_FUNC_OFFSET( 1928, glLightModelfv, glLightModelfv, NULL, 164),
    NAME_FUNC_OFFSET( 1943, glLightModeli, glLightModeli, NULL, 165),
    NAME_FUNC_OFFSET( 1957, glLightModeliv, glLightModeliv, NULL, 166),
    NAME_FUNC_OFFSET( 1972, glLineStipple, glLineStipple, NULL, 167),
    NAME_FUNC_OFFSET( 1986, glLineWidth, glLineWidth, NULL, 168),
    NAME_FUNC_OFFSET( 1998, glMaterialf, glMaterialf, NULL, 169),
    NAME_FUNC_OFFSET( 2010, glMaterialfv, glMaterialfv, NULL, 170),
    NAME_FUNC_OFFSET( 2023, glMateriali, glMateriali, NULL, 171),
    NAME_FUNC_OFFSET( 2035, glMaterialiv, glMaterialiv, NULL, 172),
    NAME_FUNC_OFFSET( 2048, glPointSize, glPointSize, NULL, 173),
    NAME_FUNC_OFFSET( 2060, glPolygonMode, glPolygonMode, NULL, 174),
    NAME_FUNC_OFFSET( 2074, glPolygonStipple, glPolygonStipple, NULL, 175),
    NAME_FUNC_OFFSET( 2091, glScissor, glScissor, NULL, 176),
    NAME_FUNC_OFFSET( 2101, glShadeModel, glShadeModel, NULL, 177),
    NAME_FUNC_OFFSET( 2114, glTexParameterf, glTexParameterf, NULL, 178),
    NAME_FUNC_OFFSET( 2130, glTexParameterfv, glTexParameterfv, NULL, 179),
    NAME_FUNC_OFFSET( 2147, glTexParameteri, glTexParameteri, NULL, 180),
    NAME_FUNC_OFFSET( 2163, glTexParameteriv, glTexParameteriv, NULL, 181),
    NAME_FUNC_OFFSET( 2180, glTexImage1D, glTexImage1D, NULL, 182),
    NAME_FUNC_OFFSET( 2193, glTexImage2D, glTexImage2D, NULL, 183),
    NAME_FUNC_OFFSET( 2206, glTexEnvf, glTexEnvf, NULL, 184),
    NAME_FUNC_OFFSET( 2216, glTexEnvfv, glTexEnvfv, NULL, 185),
    NAME_FUNC_OFFSET( 2227, glTexEnvi, glTexEnvi, NULL, 186),
    NAME_FUNC_OFFSET( 2237, glTexEnviv, glTexEnviv, NULL, 187),
    NAME_FUNC_OFFSET( 2248, glTexGend, glTexGend, NULL, 188),
    NAME_FUNC_OFFSET( 2258, glTexGendv, glTexGendv, NULL, 189),
    NAME_FUNC_OFFSET( 2269, glTexGenf, glTexGenf, NULL, 190),
    NAME_FUNC_OFFSET( 2279, glTexGenfv, glTexGenfv, NULL, 191),
    NAME_FUNC_OFFSET( 2290, glTexGeni, glTexGeni, NULL, 192),
    NAME_FUNC_OFFSET( 2300, glTexGeniv, glTexGeniv, NULL, 193),
    NAME_FUNC_OFFSET( 2311, glFeedbackBuffer, glFeedbackBuffer, NULL, 194),
    NAME_FUNC_OFFSET( 2328, glSelectBuffer, glSelectBuffer, NULL, 195),
    NAME_FUNC_OFFSET( 2343, glRenderMode, glRenderMode, NULL, 196),
    NAME_FUNC_OFFSET( 2356, glInitNames, glInitNames, NULL, 197),
    NAME_FUNC_OFFSET( 2368, glLoadName, glLoadName, NULL, 198),
    NAME_FUNC_OFFSET( 2379, glPassThrough, glPassThrough, NULL, 199),
    NAME_FUNC_OFFSET( 2393, glPopName, glPopName, NULL, 200),
    NAME_FUNC_OFFSET( 2403, glPushName, glPushName, NULL, 201),
    NAME_FUNC_OFFSET( 2414, glDrawBuffer, glDrawBuffer, NULL, 202),
    NAME_FUNC_OFFSET( 2427, glClear, glClear, NULL, 203),
    NAME_FUNC_OFFSET( 2435, glClearAccum, glClearAccum, NULL, 204),
    NAME_FUNC_OFFSET( 2448, glClearIndex, glClearIndex, NULL, 205),
    NAME_FUNC_OFFSET( 2461, glClearColor, glClearColor, NULL, 206),
    NAME_FUNC_OFFSET( 2474, glClearStencil, glClearStencil, NULL, 207),
    NAME_FUNC_OFFSET( 2489, glClearDepth, glClearDepth, NULL, 208),
    NAME_FUNC_OFFSET( 2502, glStencilMask, glStencilMask, NULL, 209),
    NAME_FUNC_OFFSET( 2516, glColorMask, glColorMask, NULL, 210),
    NAME_FUNC_OFFSET( 2528, glDepthMask, glDepthMask, NULL, 211),
    NAME_FUNC_OFFSET( 2540, glIndexMask, glIndexMask, NULL, 212),
    NAME_FUNC_OFFSET( 2552, glAccum, glAccum, NULL, 213),
    NAME_FUNC_OFFSET( 2560, glDisable, glDisable, NULL, 214),
    NAME_FUNC_OFFSET( 2570, glEnable, glEnable, NULL, 215),
    NAME_FUNC_OFFSET( 2579, glFinish, glFinish, NULL, 216),
    NAME_FUNC_OFFSET( 2588, glFlush, glFlush, NULL, 217),
    NAME_FUNC_OFFSET( 2596, glPopAttrib, glPopAttrib, NULL, 218),
    NAME_FUNC_OFFSET( 2608, glPushAttrib, glPushAttrib, NULL, 219),
    NAME_FUNC_OFFSET( 2621, glMap1d, glMap1d, NULL, 220),
    NAME_FUNC_OFFSET( 2629, glMap1f, glMap1f, NULL, 221),
    NAME_FUNC_OFFSET( 2637, glMap2d, glMap2d, NULL, 222),
    NAME_FUNC_OFFSET( 2645, glMap2f, glMap2f, NULL, 223),
    NAME_FUNC_OFFSET( 2653, glMapGrid1d, glMapGrid1d, NULL, 224),
    NAME_FUNC_OFFSET( 2665, glMapGrid1f, glMapGrid1f, NULL, 225),
    NAME_FUNC_OFFSET( 2677, glMapGrid2d, glMapGrid2d, NULL, 226),
    NAME_FUNC_OFFSET( 2689, glMapGrid2f, glMapGrid2f, NULL, 227),
    NAME_FUNC_OFFSET( 2701, glEvalCoord1d, glEvalCoord1d, NULL, 228),
    NAME_FUNC_OFFSET( 2715, glEvalCoord1dv, glEvalCoord1dv, NULL, 229),
    NAME_FUNC_OFFSET( 2730, glEvalCoord1f, glEvalCoord1f, NULL, 230),
    NAME_FUNC_OFFSET( 2744, glEvalCoord1fv, glEvalCoord1fv, NULL, 231),
    NAME_FUNC_OFFSET( 2759, glEvalCoord2d, glEvalCoord2d, NULL, 232),
    NAME_FUNC_OFFSET( 2773, glEvalCoord2dv, glEvalCoord2dv, NULL, 233),
    NAME_FUNC_OFFSET( 2788, glEvalCoord2f, glEvalCoord2f, NULL, 234),
    NAME_FUNC_OFFSET( 2802, glEvalCoord2fv, glEvalCoord2fv, NULL, 235),
    NAME_FUNC_OFFSET( 2817, glEvalMesh1, glEvalMesh1, NULL, 236),
    NAME_FUNC_OFFSET( 2829, glEvalPoint1, glEvalPoint1, NULL, 237),
    NAME_FUNC_OFFSET( 2842, glEvalMesh2, glEvalMesh2, NULL, 238),
    NAME_FUNC_OFFSET( 2854, glEvalPoint2, glEvalPoint2, NULL, 239),
    NAME_FUNC_OFFSET( 2867, glAlphaFunc, glAlphaFunc, NULL, 240),
    NAME_FUNC_OFFSET( 2879, glBlendFunc, glBlendFunc, NULL, 241),
    NAME_FUNC_OFFSET( 2891, glLogicOp, glLogicOp, NULL, 242),
    NAME_FUNC_OFFSET( 2901, glStencilFunc, glStencilFunc, NULL, 243),
    NAME_FUNC_OFFSET( 2915, glStencilOp, glStencilOp, NULL, 244),
    NAME_FUNC_OFFSET( 2927, glDepthFunc, glDepthFunc, NULL, 245),
    NAME_FUNC_OFFSET( 2939, glPixelZoom, glPixelZoom, NULL, 246),
    NAME_FUNC_OFFSET( 2951, glPixelTransferf, glPixelTransferf, NULL, 247),
    NAME_FUNC_OFFSET( 2968, glPixelTransferi, glPixelTransferi, NULL, 248),
    NAME_FUNC_OFFSET( 2985, glPixelStoref, glPixelStoref, NULL, 249),
    NAME_FUNC_OFFSET( 2999, glPixelStorei, glPixelStorei, NULL, 250),
    NAME_FUNC_OFFSET( 3013, glPixelMapfv, glPixelMapfv, NULL, 251),
    NAME_FUNC_OFFSET( 3026, glPixelMapuiv, glPixelMapuiv, NULL, 252),
    NAME_FUNC_OFFSET( 3040, glPixelMapusv, glPixelMapusv, NULL, 253),
    NAME_FUNC_OFFSET( 3054, glReadBuffer, glReadBuffer, NULL, 254),
    NAME_FUNC_OFFSET( 3067, glCopyPixels, glCopyPixels, NULL, 255),
    NAME_FUNC_OFFSET( 3080, glReadPixels, glReadPixels, NULL, 256),
    NAME_FUNC_OFFSET( 3093, glDrawPixels, glDrawPixels, NULL, 257),
    NAME_FUNC_OFFSET( 3106, glGetBooleanv, glGetBooleanv, NULL, 258),
    NAME_FUNC_OFFSET( 3120, glGetClipPlane, glGetClipPlane, NULL, 259),
    NAME_FUNC_OFFSET( 3135, glGetDoublev, glGetDoublev, NULL, 260),
    NAME_FUNC_OFFSET( 3148, glGetError, glGetError, NULL, 261),
    NAME_FUNC_OFFSET( 3159, glGetFloatv, glGetFloatv, NULL, 262),
    NAME_FUNC_OFFSET( 3171, glGetIntegerv, glGetIntegerv, NULL, 263),
    NAME_FUNC_OFFSET( 3185, glGetLightfv, glGetLightfv, NULL, 264),
    NAME_FUNC_OFFSET( 3198, glGetLightiv, glGetLightiv, NULL, 265),
    NAME_FUNC_OFFSET( 3211, glGetMapdv, glGetMapdv, NULL, 266),
    NAME_FUNC_OFFSET( 3222, glGetMapfv, glGetMapfv, NULL, 267),
    NAME_FUNC_OFFSET( 3233, glGetMapiv, glGetMapiv, NULL, 268),
    NAME_FUNC_OFFSET( 3244, glGetMaterialfv, glGetMaterialfv, NULL, 269),
    NAME_FUNC_OFFSET( 3260, glGetMaterialiv, glGetMaterialiv, NULL, 270),
    NAME_FUNC_OFFSET( 3276, glGetPixelMapfv, glGetPixelMapfv, NULL, 271),
    NAME_FUNC_OFFSET( 3292, glGetPixelMapuiv, glGetPixelMapuiv, NULL, 272),
    NAME_FUNC_OFFSET( 3309, glGetPixelMapusv, glGetPixelMapusv, NULL, 273),
    NAME_FUNC_OFFSET( 3326, glGetPolygonStipple, glGetPolygonStipple, NULL, 274),
    NAME_FUNC_OFFSET( 3346, glGetString, glGetString, NULL, 275),
    NAME_FUNC_OFFSET( 3358, glGetTexEnvfv, glGetTexEnvfv, NULL, 276),
    NAME_FUNC_OFFSET( 3372, glGetTexEnviv, glGetTexEnviv, NULL, 277),
    NAME_FUNC_OFFSET( 3386, glGetTexGendv, glGetTexGendv, NULL, 278),
    NAME_FUNC_OFFSET( 3400, glGetTexGenfv, glGetTexGenfv, NULL, 279),
    NAME_FUNC_OFFSET( 3414, glGetTexGeniv, glGetTexGeniv, NULL, 280),
    NAME_FUNC_OFFSET( 3428, glGetTexImage, glGetTexImage, NULL, 281),
    NAME_FUNC_OFFSET( 3442, glGetTexParameterfv, glGetTexParameterfv, NULL, 282),
    NAME_FUNC_OFFSET( 3462, glGetTexParameteriv, glGetTexParameteriv, NULL, 283),
    NAME_FUNC_OFFSET( 3482, glGetTexLevelParameterfv, glGetTexLevelParameterfv, NULL, 284),
    NAME_FUNC_OFFSET( 3507, glGetTexLevelParameteriv, glGetTexLevelParameteriv, NULL, 285),
    NAME_FUNC_OFFSET( 3532, glIsEnabled, glIsEnabled, NULL, 286),
    NAME_FUNC_OFFSET( 3544, glIsList, glIsList, NULL, 287),
    NAME_FUNC_OFFSET( 3553, glDepthRange, glDepthRange, NULL, 288),
    NAME_FUNC_OFFSET( 3566, glFrustum, glFrustum, NULL, 289),
    NAME_FUNC_OFFSET( 3576, glLoadIdentity, glLoadIdentity, NULL, 290),
    NAME_FUNC_OFFSET( 3591, glLoadMatrixf, glLoadMatrixf, NULL, 291),
    NAME_FUNC_OFFSET( 3605, glLoadMatrixd, glLoadMatrixd, NULL, 292),
    NAME_FUNC_OFFSET( 3619, glMatrixMode, glMatrixMode, NULL, 293),
    NAME_FUNC_OFFSET( 3632, glMultMatrixf, glMultMatrixf, NULL, 294),
    NAME_FUNC_OFFSET( 3646, glMultMatrixd, glMultMatrixd, NULL, 295),
    NAME_FUNC_OFFSET( 3660, glOrtho, glOrtho, NULL, 296),
    NAME_FUNC_OFFSET( 3668, glPopMatrix, glPopMatrix, NULL, 297),
    NAME_FUNC_OFFSET( 3680, glPushMatrix, glPushMatrix, NULL, 298),
    NAME_FUNC_OFFSET( 3693, glRotated, glRotated, NULL, 299),
    NAME_FUNC_OFFSET( 3703, glRotatef, glRotatef, NULL, 300),
    NAME_FUNC_OFFSET( 3713, glScaled, glScaled, NULL, 301),
    NAME_FUNC_OFFSET( 3722, glScalef, glScalef, NULL, 302),
    NAME_FUNC_OFFSET( 3731, glTranslated, glTranslated, NULL, 303),
    NAME_FUNC_OFFSET( 3744, glTranslatef, glTranslatef, NULL, 304),
    NAME_FUNC_OFFSET( 3757, glViewport, glViewport, NULL, 305),
    NAME_FUNC_OFFSET( 3768, glArrayElement, glArrayElement, NULL, 306),
    NAME_FUNC_OFFSET( 3783, glBindTexture, glBindTexture, NULL, 307),
    NAME_FUNC_OFFSET( 3797, glColorPointer, glColorPointer, NULL, 308),
    NAME_FUNC_OFFSET( 3812, glDisableClientState, glDisableClientState, NULL, 309),
    NAME_FUNC_OFFSET( 3833, glDrawArrays, glDrawArrays, NULL, 310),
    NAME_FUNC_OFFSET( 3846, glDrawElements, glDrawElements, NULL, 311),
    NAME_FUNC_OFFSET( 3861, glEdgeFlagPointer, glEdgeFlagPointer, NULL, 312),
    NAME_FUNC_OFFSET( 3879, glEnableClientState, glEnableClientState, NULL, 313),
    NAME_FUNC_OFFSET( 3899, glIndexPointer, glIndexPointer, NULL, 314),
    NAME_FUNC_OFFSET( 3914, glIndexub, glIndexub, NULL, 315),
    NAME_FUNC_OFFSET( 3924, glIndexubv, glIndexubv, NULL, 316),
    NAME_FUNC_OFFSET( 3935, glInterleavedArrays, glInterleavedArrays, NULL, 317),
    NAME_FUNC_OFFSET( 3955, glNormalPointer, glNormalPointer, NULL, 318),
    NAME_FUNC_OFFSET( 3971, glPolygonOffset, glPolygonOffset, NULL, 319),
    NAME_FUNC_OFFSET( 3987, glTexCoordPointer, glTexCoordPointer, NULL, 320),
    NAME_FUNC_OFFSET( 4005, glVertexPointer, glVertexPointer, NULL, 321),
    NAME_FUNC_OFFSET( 4021, glAreTexturesResident, glAreTexturesResident, NULL, 322),
    NAME_FUNC_OFFSET( 4043, glCopyTexImage1D, glCopyTexImage1D, NULL, 323),
    NAME_FUNC_OFFSET( 4060, glCopyTexImage2D, glCopyTexImage2D, NULL, 324),
    NAME_FUNC_OFFSET( 4077, glCopyTexSubImage1D, glCopyTexSubImage1D, NULL, 325),
    NAME_FUNC_OFFSET( 4097, glCopyTexSubImage2D, glCopyTexSubImage2D, NULL, 326),
    NAME_FUNC_OFFSET( 4117, glDeleteTextures, glDeleteTextures, NULL, 327),
    NAME_FUNC_OFFSET( 4134, glGenTextures, glGenTextures, NULL, 328),
    NAME_FUNC_OFFSET( 4148, glGetPointerv, glGetPointerv, NULL, 329),
    NAME_FUNC_OFFSET( 4162, glIsTexture, glIsTexture, NULL, 330),
    NAME_FUNC_OFFSET( 4174, glPrioritizeTextures, glPrioritizeTextures, NULL, 331),
    NAME_FUNC_OFFSET( 4195, glTexSubImage1D, glTexSubImage1D, NULL, 332),
    NAME_FUNC_OFFSET( 4211, glTexSubImage2D, glTexSubImage2D, NULL, 333),
    NAME_FUNC_OFFSET( 4227, glPopClientAttrib, glPopClientAttrib, NULL, 334),
    NAME_FUNC_OFFSET( 4245, glPushClientAttrib, glPushClientAttrib, NULL, 335),
    NAME_FUNC_OFFSET( 4264, glBlendColor, glBlendColor, NULL, 336),
    NAME_FUNC_OFFSET( 4277, glBlendEquation, glBlendEquation, NULL, 337),
    NAME_FUNC_OFFSET( 4293, glDrawRangeElements, glDrawRangeElements, NULL, 338),
    NAME_FUNC_OFFSET( 4313, glColorTable, glColorTable, NULL, 339),
    NAME_FUNC_OFFSET( 4326, glColorTableParameterfv, glColorTableParameterfv, NULL, 340),
    NAME_FUNC_OFFSET( 4350, glColorTableParameteriv, glColorTableParameteriv, NULL, 341),
    NAME_FUNC_OFFSET( 4374, glCopyColorTable, glCopyColorTable, NULL, 342),
    NAME_FUNC_OFFSET( 4391, glGetColorTable, glGetColorTable, NULL, 343),
    NAME_FUNC_OFFSET( 4407, glGetColorTableParameterfv, glGetColorTableParameterfv, NULL, 344),
    NAME_FUNC_OFFSET( 4434, glGetColorTableParameteriv, glGetColorTableParameteriv, NULL, 345),
    NAME_FUNC_OFFSET( 4461, glColorSubTable, glColorSubTable, NULL, 346),
    NAME_FUNC_OFFSET( 4477, glCopyColorSubTable, glCopyColorSubTable, NULL, 347),
    NAME_FUNC_OFFSET( 4497, glConvolutionFilter1D, glConvolutionFilter1D, NULL, 348),
    NAME_FUNC_OFFSET( 4519, glConvolutionFilter2D, glConvolutionFilter2D, NULL, 349),
    NAME_FUNC_OFFSET( 4541, glConvolutionParameterf, glConvolutionParameterf, NULL, 350),
    NAME_FUNC_OFFSET( 4565, glConvolutionParameterfv, glConvolutionParameterfv, NULL, 351),
    NAME_FUNC_OFFSET( 4590, glConvolutionParameteri, glConvolutionParameteri, NULL, 352),
    NAME_FUNC_OFFSET( 4614, glConvolutionParameteriv, glConvolutionParameteriv, NULL, 353),
    NAME_FUNC_OFFSET( 4639, glCopyConvolutionFilter1D, glCopyConvolutionFilter1D, NULL, 354),
    NAME_FUNC_OFFSET( 4665, glCopyConvolutionFilter2D, glCopyConvolutionFilter2D, NULL, 355),
    NAME_FUNC_OFFSET( 4691, glGetConvolutionFilter, glGetConvolutionFilter, NULL, 356),
    NAME_FUNC_OFFSET( 4714, glGetConvolutionParameterfv, glGetConvolutionParameterfv, NULL, 357),
    NAME_FUNC_OFFSET( 4742, glGetConvolutionParameteriv, glGetConvolutionParameteriv, NULL, 358),
    NAME_FUNC_OFFSET( 4770, glGetSeparableFilter, glGetSeparableFilter, NULL, 359),
    NAME_FUNC_OFFSET( 4791, glSeparableFilter2D, glSeparableFilter2D, NULL, 360),
    NAME_FUNC_OFFSET( 4811, glGetHistogram, glGetHistogram, NULL, 361),
    NAME_FUNC_OFFSET( 4826, glGetHistogramParameterfv, glGetHistogramParameterfv, NULL, 362),
    NAME_FUNC_OFFSET( 4852, glGetHistogramParameteriv, glGetHistogramParameteriv, NULL, 363),
    NAME_FUNC_OFFSET( 4878, glGetMinmax, glGetMinmax, NULL, 364),
    NAME_FUNC_OFFSET( 4890, glGetMinmaxParameterfv, glGetMinmaxParameterfv, NULL, 365),
    NAME_FUNC_OFFSET( 4913, glGetMinmaxParameteriv, glGetMinmaxParameteriv, NULL, 366),
    NAME_FUNC_OFFSET( 4936, glHistogram, glHistogram, NULL, 367),
    NAME_FUNC_OFFSET( 4948, glMinmax, glMinmax, NULL, 368),
    NAME_FUNC_OFFSET( 4957, glResetHistogram, glResetHistogram, NULL, 369),
    NAME_FUNC_OFFSET( 4974, glResetMinmax, glResetMinmax, NULL, 370),
    NAME_FUNC_OFFSET( 4988, glTexImage3D, glTexImage3D, NULL, 371),
    NAME_FUNC_OFFSET( 5001, glTexSubImage3D, glTexSubImage3D, NULL, 372),
    NAME_FUNC_OFFSET( 5017, glCopyTexSubImage3D, glCopyTexSubImage3D, NULL, 373),
    NAME_FUNC_OFFSET( 5037, glActiveTextureARB, glActiveTextureARB, NULL, 374),
    NAME_FUNC_OFFSET( 5056, glClientActiveTextureARB, glClientActiveTextureARB, NULL, 375),
    NAME_FUNC_OFFSET( 5081, glMultiTexCoord1dARB, glMultiTexCoord1dARB, NULL, 376),
    NAME_FUNC_OFFSET( 5102, glMultiTexCoord1dvARB, glMultiTexCoord1dvARB, NULL, 377),
    NAME_FUNC_OFFSET( 5124, glMultiTexCoord1fARB, glMultiTexCoord1fARB, NULL, 378),
    NAME_FUNC_OFFSET( 5145, glMultiTexCoord1fvARB, glMultiTexCoord1fvARB, NULL, 379),
    NAME_FUNC_OFFSET( 5167, glMultiTexCoord1iARB, glMultiTexCoord1iARB, NULL, 380),
    NAME_FUNC_OFFSET( 5188, glMultiTexCoord1ivARB, glMultiTexCoord1ivARB, NULL, 381),
    NAME_FUNC_OFFSET( 5210, glMultiTexCoord1sARB, glMultiTexCoord1sARB, NULL, 382),
    NAME_FUNC_OFFSET( 5231, glMultiTexCoord1svARB, glMultiTexCoord1svARB, NULL, 383),
    NAME_FUNC_OFFSET( 5253, glMultiTexCoord2dARB, glMultiTexCoord2dARB, NULL, 384),
    NAME_FUNC_OFFSET( 5274, glMultiTexCoord2dvARB, glMultiTexCoord2dvARB, NULL, 385),
    NAME_FUNC_OFFSET( 5296, glMultiTexCoord2fARB, glMultiTexCoord2fARB, NULL, 386),
    NAME_FUNC_OFFSET( 5317, glMultiTexCoord2fvARB, glMultiTexCoord2fvARB, NULL, 387),
    NAME_FUNC_OFFSET( 5339, glMultiTexCoord2iARB, glMultiTexCoord2iARB, NULL, 388),
    NAME_FUNC_OFFSET( 5360, glMultiTexCoord2ivARB, glMultiTexCoord2ivARB, NULL, 389),
    NAME_FUNC_OFFSET( 5382, glMultiTexCoord2sARB, glMultiTexCoord2sARB, NULL, 390),
    NAME_FUNC_OFFSET( 5403, glMultiTexCoord2svARB, glMultiTexCoord2svARB, NULL, 391),
    NAME_FUNC_OFFSET( 5425, glMultiTexCoord3dARB, glMultiTexCoord3dARB, NULL, 392),
    NAME_FUNC_OFFSET( 5446, glMultiTexCoord3dvARB, glMultiTexCoord3dvARB, NULL, 393),
    NAME_FUNC_OFFSET( 5468, glMultiTexCoord3fARB, glMultiTexCoord3fARB, NULL, 394),
    NAME_FUNC_OFFSET( 5489, glMultiTexCoord3fvARB, glMultiTexCoord3fvARB, NULL, 395),
    NAME_FUNC_OFFSET( 5511, glMultiTexCoord3iARB, glMultiTexCoord3iARB, NULL, 396),
    NAME_FUNC_OFFSET( 5532, glMultiTexCoord3ivARB, glMultiTexCoord3ivARB, NULL, 397),
    NAME_FUNC_OFFSET( 5554, glMultiTexCoord3sARB, glMultiTexCoord3sARB, NULL, 398),
    NAME_FUNC_OFFSET( 5575, glMultiTexCoord3svARB, glMultiTexCoord3svARB, NULL, 399),
    NAME_FUNC_OFFSET( 5597, glMultiTexCoord4dARB, glMultiTexCoord4dARB, NULL, 400),
    NAME_FUNC_OFFSET( 5618, glMultiTexCoord4dvARB, glMultiTexCoord4dvARB, NULL, 401),
    NAME_FUNC_OFFSET( 5640, glMultiTexCoord4fARB, glMultiTexCoord4fARB, NULL, 402),
    NAME_FUNC_OFFSET( 5661, glMultiTexCoord4fvARB, glMultiTexCoord4fvARB, NULL, 403),
    NAME_FUNC_OFFSET( 5683, glMultiTexCoord4iARB, glMultiTexCoord4iARB, NULL, 404),
    NAME_FUNC_OFFSET( 5704, glMultiTexCoord4ivARB, glMultiTexCoord4ivARB, NULL, 405),
    NAME_FUNC_OFFSET( 5726, glMultiTexCoord4sARB, glMultiTexCoord4sARB, NULL, 406),
    NAME_FUNC_OFFSET( 5747, glMultiTexCoord4svARB, glMultiTexCoord4svARB, NULL, 407),
    NAME_FUNC_OFFSET( 5769, glAttachShader, glAttachShader, NULL, 408),
    NAME_FUNC_OFFSET( 5784, glCreateProgram, glCreateProgram, NULL, 409),
    NAME_FUNC_OFFSET( 5800, glCreateShader, glCreateShader, NULL, 410),
    NAME_FUNC_OFFSET( 5815, glDeleteProgram, glDeleteProgram, NULL, 411),
    NAME_FUNC_OFFSET( 5831, glDeleteShader, glDeleteShader, NULL, 412),
    NAME_FUNC_OFFSET( 5846, glDetachShader, glDetachShader, NULL, 413),
    NAME_FUNC_OFFSET( 5861, glGetAttachedShaders, glGetAttachedShaders, NULL, 414),
    NAME_FUNC_OFFSET( 5882, glGetProgramInfoLog, glGetProgramInfoLog, NULL, 415),
    NAME_FUNC_OFFSET( 5902, glGetProgramiv, glGetProgramiv, NULL, 416),
    NAME_FUNC_OFFSET( 5917, glGetShaderInfoLog, glGetShaderInfoLog, NULL, 417),
    NAME_FUNC_OFFSET( 5936, glGetShaderiv, glGetShaderiv, NULL, 418),
    NAME_FUNC_OFFSET( 5950, glIsProgram, glIsProgram, NULL, 419),
    NAME_FUNC_OFFSET( 5962, glIsShader, glIsShader, NULL, 420),
    NAME_FUNC_OFFSET( 5973, glStencilFuncSeparate, glStencilFuncSeparate, NULL, 421),
    NAME_FUNC_OFFSET( 5995, glStencilMaskSeparate, glStencilMaskSeparate, NULL, 422),
    NAME_FUNC_OFFSET( 6017, glStencilOpSeparate, glStencilOpSeparate, NULL, 423),
    NAME_FUNC_OFFSET( 6037, glUniformMatrix2x3fv, glUniformMatrix2x3fv, NULL, 424),
    NAME_FUNC_OFFSET( 6058, glUniformMatrix2x4fv, glUniformMatrix2x4fv, NULL, 425),
    NAME_FUNC_OFFSET( 6079, glUniformMatrix3x2fv, glUniformMatrix3x2fv, NULL, 426),
    NAME_FUNC_OFFSET( 6100, glUniformMatrix3x4fv, glUniformMatrix3x4fv, NULL, 427),
    NAME_FUNC_OFFSET( 6121, glUniformMatrix4x2fv, glUniformMatrix4x2fv, NULL, 428),
    NAME_FUNC_OFFSET( 6142, glUniformMatrix4x3fv, glUniformMatrix4x3fv, NULL, 429),
    NAME_FUNC_OFFSET( 6163, glDrawArraysInstanced, glDrawArraysInstanced, NULL, 430),
    NAME_FUNC_OFFSET( 6185, glDrawElementsInstanced, glDrawElementsInstanced, NULL, 431),
    NAME_FUNC_OFFSET( 6209, glLoadTransposeMatrixdARB, glLoadTransposeMatrixdARB, NULL, 432),
    NAME_FUNC_OFFSET( 6235, glLoadTransposeMatrixfARB, glLoadTransposeMatrixfARB, NULL, 433),
    NAME_FUNC_OFFSET( 6261, glMultTransposeMatrixdARB, glMultTransposeMatrixdARB, NULL, 434),
    NAME_FUNC_OFFSET( 6287, glMultTransposeMatrixfARB, glMultTransposeMatrixfARB, NULL, 435),
    NAME_FUNC_OFFSET( 6313, glSampleCoverageARB, glSampleCoverageARB, NULL, 436),
    NAME_FUNC_OFFSET( 6333, glCompressedTexImage1DARB, glCompressedTexImage1DARB, NULL, 437),
    NAME_FUNC_OFFSET( 6359, glCompressedTexImage2DARB, glCompressedTexImage2DARB, NULL, 438),
    NAME_FUNC_OFFSET( 6385, glCompressedTexImage3DARB, glCompressedTexImage3DARB, NULL, 439),
    NAME_FUNC_OFFSET( 6411, glCompressedTexSubImage1DARB, glCompressedTexSubImage1DARB, NULL, 440),
    NAME_FUNC_OFFSET( 6440, glCompressedTexSubImage2DARB, glCompressedTexSubImage2DARB, NULL, 441),
    NAME_FUNC_OFFSET( 6469, glCompressedTexSubImage3DARB, glCompressedTexSubImage3DARB, NULL, 442),
    NAME_FUNC_OFFSET( 6498, glGetCompressedTexImageARB, glGetCompressedTexImageARB, NULL, 443),
    NAME_FUNC_OFFSET( 6525, glDisableVertexAttribArrayARB, glDisableVertexAttribArrayARB, NULL, 444),
    NAME_FUNC_OFFSET( 6555, glEnableVertexAttribArrayARB, glEnableVertexAttribArrayARB, NULL, 445),
    NAME_FUNC_OFFSET( 6584, glGetProgramEnvParameterdvARB, glGetProgramEnvParameterdvARB, NULL, 446),
    NAME_FUNC_OFFSET( 6614, glGetProgramEnvParameterfvARB, glGetProgramEnvParameterfvARB, NULL, 447),
    NAME_FUNC_OFFSET( 6644, glGetProgramLocalParameterdvARB, glGetProgramLocalParameterdvARB, NULL, 448),
    NAME_FUNC_OFFSET( 6676, glGetProgramLocalParameterfvARB, glGetProgramLocalParameterfvARB, NULL, 449),
    NAME_FUNC_OFFSET( 6708, glGetProgramStringARB, glGetProgramStringARB, NULL, 450),
    NAME_FUNC_OFFSET( 6730, glGetProgramivARB, glGetProgramivARB, NULL, 451),
    NAME_FUNC_OFFSET( 6748, glGetVertexAttribdvARB, glGetVertexAttribdvARB, NULL, 452),
    NAME_FUNC_OFFSET( 6771, glGetVertexAttribfvARB, glGetVertexAttribfvARB, NULL, 453),
    NAME_FUNC_OFFSET( 6794, glGetVertexAttribivARB, glGetVertexAttribivARB, NULL, 454),
    NAME_FUNC_OFFSET( 6817, glProgramEnvParameter4dARB, glProgramEnvParameter4dARB, NULL, 455),
    NAME_FUNC_OFFSET( 6844, glProgramEnvParameter4dvARB, glProgramEnvParameter4dvARB, NULL, 456),
    NAME_FUNC_OFFSET( 6872, glProgramEnvParameter4fARB, glProgramEnvParameter4fARB, NULL, 457),
    NAME_FUNC_OFFSET( 6899, glProgramEnvParameter4fvARB, glProgramEnvParameter4fvARB, NULL, 458),
    NAME_FUNC_OFFSET( 6927, glProgramLocalParameter4dARB, glProgramLocalParameter4dARB, NULL, 459),
    NAME_FUNC_OFFSET( 6956, glProgramLocalParameter4dvARB, glProgramLocalParameter4dvARB, NULL, 460),
    NAME_FUNC_OFFSET( 6986, glProgramLocalParameter4fARB, glProgramLocalParameter4fARB, NULL, 461),
    NAME_FUNC_OFFSET( 7015, glProgramLocalParameter4fvARB, glProgramLocalParameter4fvARB, NULL, 462),
    NAME_FUNC_OFFSET( 7045, glProgramStringARB, glProgramStringARB, NULL, 463),
    NAME_FUNC_OFFSET( 7064, glVertexAttrib1dARB, glVertexAttrib1dARB, NULL, 464),
    NAME_FUNC_OFFSET( 7084, glVertexAttrib1dvARB, glVertexAttrib1dvARB, NULL, 465),
    NAME_FUNC_OFFSET( 7105, glVertexAttrib1fARB, glVertexAttrib1fARB, NULL, 466),
    NAME_FUNC_OFFSET( 7125, glVertexAttrib1fvARB, glVertexAttrib1fvARB, NULL, 467),
    NAME_FUNC_OFFSET( 7146, glVertexAttrib1sARB, glVertexAttrib1sARB, NULL, 468),
    NAME_FUNC_OFFSET( 7166, glVertexAttrib1svARB, glVertexAttrib1svARB, NULL, 469),
    NAME_FUNC_OFFSET( 7187, glVertexAttrib2dARB, glVertexAttrib2dARB, NULL, 470),
    NAME_FUNC_OFFSET( 7207, glVertexAttrib2dvARB, glVertexAttrib2dvARB, NULL, 471),
    NAME_FUNC_OFFSET( 7228, glVertexAttrib2fARB, glVertexAttrib2fARB, NULL, 472),
    NAME_FUNC_OFFSET( 7248, glVertexAttrib2fvARB, glVertexAttrib2fvARB, NULL, 473),
    NAME_FUNC_OFFSET( 7269, glVertexAttrib2sARB, glVertexAttrib2sARB, NULL, 474),
    NAME_FUNC_OFFSET( 7289, glVertexAttrib2svARB, glVertexAttrib2svARB, NULL, 475),
    NAME_FUNC_OFFSET( 7310, glVertexAttrib3dARB, glVertexAttrib3dARB, NULL, 476),
    NAME_FUNC_OFFSET( 7330, glVertexAttrib3dvARB, glVertexAttrib3dvARB, NULL, 477),
    NAME_FUNC_OFFSET( 7351, glVertexAttrib3fARB, glVertexAttrib3fARB, NULL, 478),
    NAME_FUNC_OFFSET( 7371, glVertexAttrib3fvARB, glVertexAttrib3fvARB, NULL, 479),
    NAME_FUNC_OFFSET( 7392, glVertexAttrib3sARB, glVertexAttrib3sARB, NULL, 480),
    NAME_FUNC_OFFSET( 7412, glVertexAttrib3svARB, glVertexAttrib3svARB, NULL, 481),
    NAME_FUNC_OFFSET( 7433, glVertexAttrib4NbvARB, glVertexAttrib4NbvARB, NULL, 482),
    NAME_FUNC_OFFSET( 7455, glVertexAttrib4NivARB, glVertexAttrib4NivARB, NULL, 483),
    NAME_FUNC_OFFSET( 7477, glVertexAttrib4NsvARB, glVertexAttrib4NsvARB, NULL, 484),
    NAME_FUNC_OFFSET( 7499, glVertexAttrib4NubARB, glVertexAttrib4NubARB, NULL, 485),
    NAME_FUNC_OFFSET( 7521, glVertexAttrib4NubvARB, glVertexAttrib4NubvARB, NULL, 486),
    NAME_FUNC_OFFSET( 7544, glVertexAttrib4NuivARB, glVertexAttrib4NuivARB, NULL, 487),
    NAME_FUNC_OFFSET( 7567, glVertexAttrib4NusvARB, glVertexAttrib4NusvARB, NULL, 488),
    NAME_FUNC_OFFSET( 7590, glVertexAttrib4bvARB, glVertexAttrib4bvARB, NULL, 489),
    NAME_FUNC_OFFSET( 7611, glVertexAttrib4dARB, glVertexAttrib4dARB, NULL, 490),
    NAME_FUNC_OFFSET( 7631, glVertexAttrib4dvARB, glVertexAttrib4dvARB, NULL, 491),
    NAME_FUNC_OFFSET( 7652, glVertexAttrib4fARB, glVertexAttrib4fARB, NULL, 492),
    NAME_FUNC_OFFSET( 7672, glVertexAttrib4fvARB, glVertexAttrib4fvARB, NULL, 493),
    NAME_FUNC_OFFSET( 7693, glVertexAttrib4ivARB, glVertexAttrib4ivARB, NULL, 494),
    NAME_FUNC_OFFSET( 7714, glVertexAttrib4sARB, glVertexAttrib4sARB, NULL, 495),
    NAME_FUNC_OFFSET( 7734, glVertexAttrib4svARB, glVertexAttrib4svARB, NULL, 496),
    NAME_FUNC_OFFSET( 7755, glVertexAttrib4ubvARB, glVertexAttrib4ubvARB, NULL, 497),
    NAME_FUNC_OFFSET( 7777, glVertexAttrib4uivARB, glVertexAttrib4uivARB, NULL, 498),
    NAME_FUNC_OFFSET( 7799, glVertexAttrib4usvARB, glVertexAttrib4usvARB, NULL, 499),
    NAME_FUNC_OFFSET( 7821, glVertexAttribPointerARB, glVertexAttribPointerARB, NULL, 500),
    NAME_FUNC_OFFSET( 7846, glBindBufferARB, glBindBufferARB, NULL, 501),
    NAME_FUNC_OFFSET( 7862, glBufferDataARB, glBufferDataARB, NULL, 502),
    NAME_FUNC_OFFSET( 7878, glBufferSubDataARB, glBufferSubDataARB, NULL, 503),
    NAME_FUNC_OFFSET( 7897, glDeleteBuffersARB, glDeleteBuffersARB, NULL, 504),
    NAME_FUNC_OFFSET( 7916, glGenBuffersARB, glGenBuffersARB, NULL, 505),
    NAME_FUNC_OFFSET( 7932, glGetBufferParameterivARB, glGetBufferParameterivARB, NULL, 506),
    NAME_FUNC_OFFSET( 7958, glGetBufferPointervARB, glGetBufferPointervARB, NULL, 507),
    NAME_FUNC_OFFSET( 7981, glGetBufferSubDataARB, glGetBufferSubDataARB, NULL, 508),
    NAME_FUNC_OFFSET( 8003, glIsBufferARB, glIsBufferARB, NULL, 509),
    NAME_FUNC_OFFSET( 8017, glMapBufferARB, glMapBufferARB, NULL, 510),
    NAME_FUNC_OFFSET( 8032, glUnmapBufferARB, glUnmapBufferARB, NULL, 511),
    NAME_FUNC_OFFSET( 8049, glBeginQueryARB, glBeginQueryARB, NULL, 512),
    NAME_FUNC_OFFSET( 8065, glDeleteQueriesARB, glDeleteQueriesARB, NULL, 513),
    NAME_FUNC_OFFSET( 8084, glEndQueryARB, glEndQueryARB, NULL, 514),
    NAME_FUNC_OFFSET( 8098, glGenQueriesARB, glGenQueriesARB, NULL, 515),
    NAME_FUNC_OFFSET( 8114, glGetQueryObjectivARB, glGetQueryObjectivARB, NULL, 516),
    NAME_FUNC_OFFSET( 8136, glGetQueryObjectuivARB, glGetQueryObjectuivARB, NULL, 517),
    NAME_FUNC_OFFSET( 8159, glGetQueryivARB, glGetQueryivARB, NULL, 518),
    NAME_FUNC_OFFSET( 8175, glIsQueryARB, glIsQueryARB, NULL, 519),
    NAME_FUNC_OFFSET( 8188, glAttachObjectARB, glAttachObjectARB, NULL, 520),
    NAME_FUNC_OFFSET( 8206, glCompileShaderARB, glCompileShaderARB, NULL, 521),
    NAME_FUNC_OFFSET( 8225, glCreateProgramObjectARB, glCreateProgramObjectARB, NULL, 522),
    NAME_FUNC_OFFSET( 8250, glCreateShaderObjectARB, glCreateShaderObjectARB, NULL, 523),
    NAME_FUNC_OFFSET( 8274, glDeleteObjectARB, glDeleteObjectARB, NULL, 524),
    NAME_FUNC_OFFSET( 8292, glDetachObjectARB, glDetachObjectARB, NULL, 525),
    NAME_FUNC_OFFSET( 8310, glGetActiveUniformARB, glGetActiveUniformARB, NULL, 526),
    NAME_FUNC_OFFSET( 8332, glGetAttachedObjectsARB, glGetAttachedObjectsARB, NULL, 527),
    NAME_FUNC_OFFSET( 8356, glGetHandleARB, glGetHandleARB, NULL, 528),
    NAME_FUNC_OFFSET( 8371, glGetInfoLogARB, glGetInfoLogARB, NULL, 529),
    NAME_FUNC_OFFSET( 8387, glGetObjectParameterfvARB, glGetObjectParameterfvARB, NULL, 530),
    NAME_FUNC_OFFSET( 8413, glGetObjectParameterivARB, glGetObjectParameterivARB, NULL, 531),
    NAME_FUNC_OFFSET( 8439, glGetShaderSourceARB, glGetShaderSourceARB, NULL, 532),
    NAME_FUNC_OFFSET( 8460, glGetUniformLocationARB, glGetUniformLocationARB, NULL, 533),
    NAME_FUNC_OFFSET( 8484, glGetUniformfvARB, glGetUniformfvARB, NULL, 534),
    NAME_FUNC_OFFSET( 8502, glGetUniformivARB, glGetUniformivARB, NULL, 535),
    NAME_FUNC_OFFSET( 8520, glLinkProgramARB, glLinkProgramARB, NULL, 536),
    NAME_FUNC_OFFSET( 8537, glShaderSourceARB, glShaderSourceARB, NULL, 537),
    NAME_FUNC_OFFSET( 8555, glUniform1fARB, glUniform1fARB, NULL, 538),
    NAME_FUNC_OFFSET( 8570, glUniform1fvARB, glUniform1fvARB, NULL, 539),
    NAME_FUNC_OFFSET( 8586, glUniform1iARB, glUniform1iARB, NULL, 540),
    NAME_FUNC_OFFSET( 8601, glUniform1ivARB, glUniform1ivARB, NULL, 541),
    NAME_FUNC_OFFSET( 8617, glUniform2fARB, glUniform2fARB, NULL, 542),
    NAME_FUNC_OFFSET( 8632, glUniform2fvARB, glUniform2fvARB, NULL, 543),
    NAME_FUNC_OFFSET( 8648, glUniform2iARB, glUniform2iARB, NULL, 544),
    NAME_FUNC_OFFSET( 8663, glUniform2ivARB, glUniform2ivARB, NULL, 545),
    NAME_FUNC_OFFSET( 8679, glUniform3fARB, glUniform3fARB, NULL, 546),
    NAME_FUNC_OFFSET( 8694, glUniform3fvARB, glUniform3fvARB, NULL, 547),
    NAME_FUNC_OFFSET( 8710, glUniform3iARB, glUniform3iARB, NULL, 548),
    NAME_FUNC_OFFSET( 8725, glUniform3ivARB, glUniform3ivARB, NULL, 549),
    NAME_FUNC_OFFSET( 8741, glUniform4fARB, glUniform4fARB, NULL, 550),
    NAME_FUNC_OFFSET( 8756, glUniform4fvARB, glUniform4fvARB, NULL, 551),
    NAME_FUNC_OFFSET( 8772, glUniform4iARB, glUniform4iARB, NULL, 552),
    NAME_FUNC_OFFSET( 8787, glUniform4ivARB, glUniform4ivARB, NULL, 553),
    NAME_FUNC_OFFSET( 8803, glUniformMatrix2fvARB, glUniformMatrix2fvARB, NULL, 554),
    NAME_FUNC_OFFSET( 8825, glUniformMatrix3fvARB, glUniformMatrix3fvARB, NULL, 555),
    NAME_FUNC_OFFSET( 8847, glUniformMatrix4fvARB, glUniformMatrix4fvARB, NULL, 556),
    NAME_FUNC_OFFSET( 8869, glUseProgramObjectARB, glUseProgramObjectARB, NULL, 557),
    NAME_FUNC_OFFSET( 8891, glValidateProgramARB, glValidateProgramARB, NULL, 558),
    NAME_FUNC_OFFSET( 8912, glBindAttribLocationARB, glBindAttribLocationARB, NULL, 559),
    NAME_FUNC_OFFSET( 8936, glGetActiveAttribARB, glGetActiveAttribARB, NULL, 560),
    NAME_FUNC_OFFSET( 8957, glGetAttribLocationARB, glGetAttribLocationARB, NULL, 561),
    NAME_FUNC_OFFSET( 8980, glDrawBuffersARB, glDrawBuffersARB, NULL, 562),
    NAME_FUNC_OFFSET( 8997, glRenderbufferStorageMultisample, glRenderbufferStorageMultisample, NULL, 563),
    NAME_FUNC_OFFSET( 9030, glFramebufferTextureARB, glFramebufferTextureARB, NULL, 564),
    NAME_FUNC_OFFSET( 9054, glFramebufferTextureFaceARB, glFramebufferTextureFaceARB, NULL, 565),
    NAME_FUNC_OFFSET( 9082, glProgramParameteriARB, glProgramParameteriARB, NULL, 566),
    NAME_FUNC_OFFSET( 9105, glFlushMappedBufferRange, glFlushMappedBufferRange, NULL, 567),
    NAME_FUNC_OFFSET( 9130, glMapBufferRange, glMapBufferRange, NULL, 568),
    NAME_FUNC_OFFSET( 9147, glBindVertexArray, glBindVertexArray, NULL, 569),
    NAME_FUNC_OFFSET( 9165, glGenVertexArrays, glGenVertexArrays, NULL, 570),
    NAME_FUNC_OFFSET( 9183, glCopyBufferSubData, glCopyBufferSubData, NULL, 571),
    NAME_FUNC_OFFSET( 9203, glClientWaitSync, glClientWaitSync, NULL, 572),
    NAME_FUNC_OFFSET( 9220, glDeleteSync, glDeleteSync, NULL, 573),
    NAME_FUNC_OFFSET( 9233, glFenceSync, glFenceSync, NULL, 574),
    NAME_FUNC_OFFSET( 9245, glGetInteger64v, glGetInteger64v, NULL, 575),
    NAME_FUNC_OFFSET( 9261, glGetSynciv, glGetSynciv, NULL, 576),
    NAME_FUNC_OFFSET( 9273, glIsSync, glIsSync, NULL, 577),
    NAME_FUNC_OFFSET( 9282, glWaitSync, glWaitSync, NULL, 578),
    NAME_FUNC_OFFSET( 9293, glDrawElementsBaseVertex, glDrawElementsBaseVertex, NULL, 579),
    NAME_FUNC_OFFSET( 9318, glDrawRangeElementsBaseVertex, glDrawRangeElementsBaseVertex, NULL, 580),
    NAME_FUNC_OFFSET( 9348, glMultiDrawElementsBaseVertex, glMultiDrawElementsBaseVertex, NULL, 581),
    NAME_FUNC_OFFSET( 9378, glBindTransformFeedback, glBindTransformFeedback, NULL, 582),
    NAME_FUNC_OFFSET( 9402, glDeleteTransformFeedbacks, glDeleteTransformFeedbacks, NULL, 583),
    NAME_FUNC_OFFSET( 9429, glDrawTransformFeedback, glDrawTransformFeedback, NULL, 584),
    NAME_FUNC_OFFSET( 9453, glGenTransformFeedbacks, glGenTransformFeedbacks, NULL, 585),
    NAME_FUNC_OFFSET( 9477, glIsTransformFeedback, glIsTransformFeedback, NULL, 586),
    NAME_FUNC_OFFSET( 9499, glPauseTransformFeedback, glPauseTransformFeedback, NULL, 587),
    NAME_FUNC_OFFSET( 9524, glResumeTransformFeedback, glResumeTransformFeedback, NULL, 588),
    NAME_FUNC_OFFSET( 9550, glPolygonOffsetEXT, glPolygonOffsetEXT, NULL, 589),
    NAME_FUNC_OFFSET( 9569, gl_dispatch_stub_590, gl_dispatch_stub_590, NULL, 590),
    NAME_FUNC_OFFSET( 9601, gl_dispatch_stub_591, gl_dispatch_stub_591, NULL, 591),
    NAME_FUNC_OFFSET( 9633, gl_dispatch_stub_592, gl_dispatch_stub_592, NULL, 592),
    NAME_FUNC_OFFSET( 9661, gl_dispatch_stub_593, gl_dispatch_stub_593, NULL, 593),
    NAME_FUNC_OFFSET( 9690, gl_dispatch_stub_594, gl_dispatch_stub_594, NULL, 594),
    NAME_FUNC_OFFSET( 9718, gl_dispatch_stub_595, gl_dispatch_stub_595, NULL, 595),
    NAME_FUNC_OFFSET( 9747, gl_dispatch_stub_596, gl_dispatch_stub_596, NULL, 596),
    NAME_FUNC_OFFSET( 9764, gl_dispatch_stub_597, gl_dispatch_stub_597, NULL, 597),
    NAME_FUNC_OFFSET( 9784, glColorPointerEXT, glColorPointerEXT, NULL, 598),
    NAME_FUNC_OFFSET( 9802, glEdgeFlagPointerEXT, glEdgeFlagPointerEXT, NULL, 599),
    NAME_FUNC_OFFSET( 9823, glIndexPointerEXT, glIndexPointerEXT, NULL, 600),
    NAME_FUNC_OFFSET( 9841, glNormalPointerEXT, glNormalPointerEXT, NULL, 601),
    NAME_FUNC_OFFSET( 9860, glTexCoordPointerEXT, glTexCoordPointerEXT, NULL, 602),
    NAME_FUNC_OFFSET( 9881, glVertexPointerEXT, glVertexPointerEXT, NULL, 603),
    NAME_FUNC_OFFSET( 9900, glPointParameterfEXT, glPointParameterfEXT, NULL, 604),
    NAME_FUNC_OFFSET( 9921, glPointParameterfvEXT, glPointParameterfvEXT, NULL, 605),
    NAME_FUNC_OFFSET( 9943, glLockArraysEXT, glLockArraysEXT, NULL, 606),
    NAME_FUNC_OFFSET( 9959, glUnlockArraysEXT, glUnlockArraysEXT, NULL, 607),
    NAME_FUNC_OFFSET( 9977, glSecondaryColor3bEXT, glSecondaryColor3bEXT, NULL, 608),
    NAME_FUNC_OFFSET( 9999, glSecondaryColor3bvEXT, glSecondaryColor3bvEXT, NULL, 609),
    NAME_FUNC_OFFSET(10022, glSecondaryColor3dEXT, glSecondaryColor3dEXT, NULL, 610),
    NAME_FUNC_OFFSET(10044, glSecondaryColor3dvEXT, glSecondaryColor3dvEXT, NULL, 611),
    NAME_FUNC_OFFSET(10067, glSecondaryColor3fEXT, glSecondaryColor3fEXT, NULL, 612),
    NAME_FUNC_OFFSET(10089, glSecondaryColor3fvEXT, glSecondaryColor3fvEXT, NULL, 613),
    NAME_FUNC_OFFSET(10112, glSecondaryColor3iEXT, glSecondaryColor3iEXT, NULL, 614),
    NAME_FUNC_OFFSET(10134, glSecondaryColor3ivEXT, glSecondaryColor3ivEXT, NULL, 615),
    NAME_FUNC_OFFSET(10157, glSecondaryColor3sEXT, glSecondaryColor3sEXT, NULL, 616),
    NAME_FUNC_OFFSET(10179, glSecondaryColor3svEXT, glSecondaryColor3svEXT, NULL, 617),
    NAME_FUNC_OFFSET(10202, glSecondaryColor3ubEXT, glSecondaryColor3ubEXT, NULL, 618),
    NAME_FUNC_OFFSET(10225, glSecondaryColor3ubvEXT, glSecondaryColor3ubvEXT, NULL, 619),
    NAME_FUNC_OFFSET(10249, glSecondaryColor3uiEXT, glSecondaryColor3uiEXT, NULL, 620),
    NAME_FUNC_OFFSET(10272, glSecondaryColor3uivEXT, glSecondaryColor3uivEXT, NULL, 621),
    NAME_FUNC_OFFSET(10296, glSecondaryColor3usEXT, glSecondaryColor3usEXT, NULL, 622),
    NAME_FUNC_OFFSET(10319, glSecondaryColor3usvEXT, glSecondaryColor3usvEXT, NULL, 623),
    NAME_FUNC_OFFSET(10343, glSecondaryColorPointerEXT, glSecondaryColorPointerEXT, NULL, 624),
    NAME_FUNC_OFFSET(10370, glMultiDrawArraysEXT, glMultiDrawArraysEXT, NULL, 625),
    NAME_FUNC_OFFSET(10391, glMultiDrawElementsEXT, glMultiDrawElementsEXT, NULL, 626),
    NAME_FUNC_OFFSET(10414, glFogCoordPointerEXT, glFogCoordPointerEXT, NULL, 627),
    NAME_FUNC_OFFSET(10435, glFogCoorddEXT, glFogCoorddEXT, NULL, 628),
    NAME_FUNC_OFFSET(10450, glFogCoorddvEXT, glFogCoorddvEXT, NULL, 629),
    NAME_FUNC_OFFSET(10466, glFogCoordfEXT, glFogCoordfEXT, NULL, 630),
    NAME_FUNC_OFFSET(10481, glFogCoordfvEXT, glFogCoordfvEXT, NULL, 631),
    NAME_FUNC_OFFSET(10497, gl_dispatch_stub_632, gl_dispatch_stub_632, NULL, 632),
    NAME_FUNC_OFFSET(10515, glBlendFuncSeparateEXT, glBlendFuncSeparateEXT, NULL, 633),
    NAME_FUNC_OFFSET(10538, glFlushVertexArrayRangeNV, glFlushVertexArrayRangeNV, NULL, 634),
    NAME_FUNC_OFFSET(10564, glVertexArrayRangeNV, glVertexArrayRangeNV, NULL, 635),
    NAME_FUNC_OFFSET(10585, glCombinerInputNV, glCombinerInputNV, NULL, 636),
    NAME_FUNC_OFFSET(10603, glCombinerOutputNV, glCombinerOutputNV, NULL, 637),
    NAME_FUNC_OFFSET(10622, glCombinerParameterfNV, glCombinerParameterfNV, NULL, 638),
    NAME_FUNC_OFFSET(10645, glCombinerParameterfvNV, glCombinerParameterfvNV, NULL, 639),
    NAME_FUNC_OFFSET(10669, glCombinerParameteriNV, glCombinerParameteriNV, NULL, 640),
    NAME_FUNC_OFFSET(10692, glCombinerParameterivNV, glCombinerParameterivNV, NULL, 641),
    NAME_FUNC_OFFSET(10716, glFinalCombinerInputNV, glFinalCombinerInputNV, NULL, 642),
    NAME_FUNC_OFFSET(10739, glGetCombinerInputParameterfvNV, glGetCombinerInputParameterfvNV, NULL, 643),
    NAME_FUNC_OFFSET(10771, glGetCombinerInputParameterivNV, glGetCombinerInputParameterivNV, NULL, 644),
    NAME_FUNC_OFFSET(10803, glGetCombinerOutputParameterfvNV, glGetCombinerOutputParameterfvNV, NULL, 645),
    NAME_FUNC_OFFSET(10836, glGetCombinerOutputParameterivNV, glGetCombinerOutputParameterivNV, NULL, 646),
    NAME_FUNC_OFFSET(10869, glGetFinalCombinerInputParameterfvNV, glGetFinalCombinerInputParameterfvNV, NULL, 647),
    NAME_FUNC_OFFSET(10906, glGetFinalCombinerInputParameterivNV, glGetFinalCombinerInputParameterivNV, NULL, 648),
    NAME_FUNC_OFFSET(10943, glResizeBuffersMESA, glResizeBuffersMESA, NULL, 649),
    NAME_FUNC_OFFSET(10963, glWindowPos2dMESA, glWindowPos2dMESA, NULL, 650),
    NAME_FUNC_OFFSET(10981, glWindowPos2dvMESA, glWindowPos2dvMESA, NULL, 651),
    NAME_FUNC_OFFSET(11000, glWindowPos2fMESA, glWindowPos2fMESA, NULL, 652),
    NAME_FUNC_OFFSET(11018, glWindowPos2fvMESA, glWindowPos2fvMESA, NULL, 653),
    NAME_FUNC_OFFSET(11037, glWindowPos2iMESA, glWindowPos2iMESA, NULL, 654),
    NAME_FUNC_OFFSET(11055, glWindowPos2ivMESA, glWindowPos2ivMESA, NULL, 655),
    NAME_FUNC_OFFSET(11074, glWindowPos2sMESA, glWindowPos2sMESA, NULL, 656),
    NAME_FUNC_OFFSET(11092, glWindowPos2svMESA, glWindowPos2svMESA, NULL, 657),
    NAME_FUNC_OFFSET(11111, glWindowPos3dMESA, glWindowPos3dMESA, NULL, 658),
    NAME_FUNC_OFFSET(11129, glWindowPos3dvMESA, glWindowPos3dvMESA, NULL, 659),
    NAME_FUNC_OFFSET(11148, glWindowPos3fMESA, glWindowPos3fMESA, NULL, 660),
    NAME_FUNC_OFFSET(11166, glWindowPos3fvMESA, glWindowPos3fvMESA, NULL, 661),
    NAME_FUNC_OFFSET(11185, glWindowPos3iMESA, glWindowPos3iMESA, NULL, 662),
    NAME_FUNC_OFFSET(11203, glWindowPos3ivMESA, glWindowPos3ivMESA, NULL, 663),
    NAME_FUNC_OFFSET(11222, glWindowPos3sMESA, glWindowPos3sMESA, NULL, 664),
    NAME_FUNC_OFFSET(11240, glWindowPos3svMESA, glWindowPos3svMESA, NULL, 665),
    NAME_FUNC_OFFSET(11259, glWindowPos4dMESA, glWindowPos4dMESA, NULL, 666),
    NAME_FUNC_OFFSET(11277, glWindowPos4dvMESA, glWindowPos4dvMESA, NULL, 667),
    NAME_FUNC_OFFSET(11296, glWindowPos4fMESA, glWindowPos4fMESA, NULL, 668),
    NAME_FUNC_OFFSET(11314, glWindowPos4fvMESA, glWindowPos4fvMESA, NULL, 669),
    NAME_FUNC_OFFSET(11333, glWindowPos4iMESA, glWindowPos4iMESA, NULL, 670),
    NAME_FUNC_OFFSET(11351, glWindowPos4ivMESA, glWindowPos4ivMESA, NULL, 671),
    NAME_FUNC_OFFSET(11370, glWindowPos4sMESA, glWindowPos4sMESA, NULL, 672),
    NAME_FUNC_OFFSET(11388, glWindowPos4svMESA, glWindowPos4svMESA, NULL, 673),
    NAME_FUNC_OFFSET(11407, gl_dispatch_stub_674, gl_dispatch_stub_674, NULL, 674),
    NAME_FUNC_OFFSET(11432, gl_dispatch_stub_675, gl_dispatch_stub_675, NULL, 675),
    NAME_FUNC_OFFSET(11459, gl_dispatch_stub_676, gl_dispatch_stub_676, NULL, 676),
    NAME_FUNC_OFFSET(11476, gl_dispatch_stub_677, gl_dispatch_stub_677, NULL, 677),
    NAME_FUNC_OFFSET(11492, gl_dispatch_stub_678, gl_dispatch_stub_678, NULL, 678),
    NAME_FUNC_OFFSET(11506, gl_dispatch_stub_679, gl_dispatch_stub_679, NULL, 679),
    NAME_FUNC_OFFSET(11521, gl_dispatch_stub_680, gl_dispatch_stub_680, NULL, 680),
    NAME_FUNC_OFFSET(11533, gl_dispatch_stub_681, gl_dispatch_stub_681, NULL, 681),
    NAME_FUNC_OFFSET(11546, gl_dispatch_stub_682, gl_dispatch_stub_682, NULL, 682),
    NAME_FUNC_OFFSET(11560, glAreProgramsResidentNV, glAreProgramsResidentNV, NULL, 683),
    NAME_FUNC_OFFSET(11584, glBindProgramNV, glBindProgramNV, NULL, 684),
    NAME_FUNC_OFFSET(11600, glDeleteProgramsNV, glDeleteProgramsNV, NULL, 685),
    NAME_FUNC_OFFSET(11619, glExecuteProgramNV, glExecuteProgramNV, NULL, 686),
    NAME_FUNC_OFFSET(11638, glGenProgramsNV, glGenProgramsNV, NULL, 687),
    NAME_FUNC_OFFSET(11654, glGetProgramParameterdvNV, glGetProgramParameterdvNV, NULL, 688),
    NAME_FUNC_OFFSET(11680, glGetProgramParameterfvNV, glGetProgramParameterfvNV, NULL, 689),
    NAME_FUNC_OFFSET(11706, glGetProgramStringNV, glGetProgramStringNV, NULL, 690),
    NAME_FUNC_OFFSET(11727, glGetProgramivNV, glGetProgramivNV, NULL, 691),
    NAME_FUNC_OFFSET(11744, glGetTrackMatrixivNV, glGetTrackMatrixivNV, NULL, 692),
    NAME_FUNC_OFFSET(11765, glGetVertexAttribPointervNV, glGetVertexAttribPointervNV, NULL, 693),
    NAME_FUNC_OFFSET(11793, glGetVertexAttribdvNV, glGetVertexAttribdvNV, NULL, 694),
    NAME_FUNC_OFFSET(11815, glGetVertexAttribfvNV, glGetVertexAttribfvNV, NULL, 695),
    NAME_FUNC_OFFSET(11837, glGetVertexAttribivNV, glGetVertexAttribivNV, NULL, 696),
    NAME_FUNC_OFFSET(11859, glIsProgramNV, glIsProgramNV, NULL, 697),
    NAME_FUNC_OFFSET(11873, glLoadProgramNV, glLoadProgramNV, NULL, 698),
    NAME_FUNC_OFFSET(11889, glProgramParameters4dvNV, glProgramParameters4dvNV, NULL, 699),
    NAME_FUNC_OFFSET(11914, glProgramParameters4fvNV, glProgramParameters4fvNV, NULL, 700),
    NAME_FUNC_OFFSET(11939, glRequestResidentProgramsNV, glRequestResidentProgramsNV, NULL, 701),
    NAME_FUNC_OFFSET(11967, glTrackMatrixNV, glTrackMatrixNV, NULL, 702),
    NAME_FUNC_OFFSET(11983, glVertexAttrib1dNV, glVertexAttrib1dNV, NULL, 703),
    NAME_FUNC_OFFSET(12002, glVertexAttrib1dvNV, glVertexAttrib1dvNV, NULL, 704),
    NAME_FUNC_OFFSET(12022, glVertexAttrib1fNV, glVertexAttrib1fNV, NULL, 705),
    NAME_FUNC_OFFSET(12041, glVertexAttrib1fvNV, glVertexAttrib1fvNV, NULL, 706),
    NAME_FUNC_OFFSET(12061, glVertexAttrib1sNV, glVertexAttrib1sNV, NULL, 707),
    NAME_FUNC_OFFSET(12080, glVertexAttrib1svNV, glVertexAttrib1svNV, NULL, 708),
    NAME_FUNC_OFFSET(12100, glVertexAttrib2dNV, glVertexAttrib2dNV, NULL, 709),
    NAME_FUNC_OFFSET(12119, glVertexAttrib2dvNV, glVertexAttrib2dvNV, NULL, 710),
    NAME_FUNC_OFFSET(12139, glVertexAttrib2fNV, glVertexAttrib2fNV, NULL, 711),
    NAME_FUNC_OFFSET(12158, glVertexAttrib2fvNV, glVertexAttrib2fvNV, NULL, 712),
    NAME_FUNC_OFFSET(12178, glVertexAttrib2sNV, glVertexAttrib2sNV, NULL, 713),
    NAME_FUNC_OFFSET(12197, glVertexAttrib2svNV, glVertexAttrib2svNV, NULL, 714),
    NAME_FUNC_OFFSET(12217, glVertexAttrib3dNV, glVertexAttrib3dNV, NULL, 715),
    NAME_FUNC_OFFSET(12236, glVertexAttrib3dvNV, glVertexAttrib3dvNV, NULL, 716),
    NAME_FUNC_OFFSET(12256, glVertexAttrib3fNV, glVertexAttrib3fNV, NULL, 717),
    NAME_FUNC_OFFSET(12275, glVertexAttrib3fvNV, glVertexAttrib3fvNV, NULL, 718),
    NAME_FUNC_OFFSET(12295, glVertexAttrib3sNV, glVertexAttrib3sNV, NULL, 719),
    NAME_FUNC_OFFSET(12314, glVertexAttrib3svNV, glVertexAttrib3svNV, NULL, 720),
    NAME_FUNC_OFFSET(12334, glVertexAttrib4dNV, glVertexAttrib4dNV, NULL, 721),
    NAME_FUNC_OFFSET(12353, glVertexAttrib4dvNV, glVertexAttrib4dvNV, NULL, 722),
    NAME_FUNC_OFFSET(12373, glVertexAttrib4fNV, glVertexAttrib4fNV, NULL, 723),
    NAME_FUNC_OFFSET(12392, glVertexAttrib4fvNV, glVertexAttrib4fvNV, NULL, 724),
    NAME_FUNC_OFFSET(12412, glVertexAttrib4sNV, glVertexAttrib4sNV, NULL, 725),
    NAME_FUNC_OFFSET(12431, glVertexAttrib4svNV, glVertexAttrib4svNV, NULL, 726),
    NAME_FUNC_OFFSET(12451, glVertexAttrib4ubNV, glVertexAttrib4ubNV, NULL, 727),
    NAME_FUNC_OFFSET(12471, glVertexAttrib4ubvNV, glVertexAttrib4ubvNV, NULL, 728),
    NAME_FUNC_OFFSET(12492, glVertexAttribPointerNV, glVertexAttribPointerNV, NULL, 729),
    NAME_FUNC_OFFSET(12516, glVertexAttribs1dvNV, glVertexAttribs1dvNV, NULL, 730),
    NAME_FUNC_OFFSET(12537, glVertexAttribs1fvNV, glVertexAttribs1fvNV, NULL, 731),
    NAME_FUNC_OFFSET(12558, glVertexAttribs1svNV, glVertexAttribs1svNV, NULL, 732),
    NAME_FUNC_OFFSET(12579, glVertexAttribs2dvNV, glVertexAttribs2dvNV, NULL, 733),
    NAME_FUNC_OFFSET(12600, glVertexAttribs2fvNV, glVertexAttribs2fvNV, NULL, 734),
    NAME_FUNC_OFFSET(12621, glVertexAttribs2svNV, glVertexAttribs2svNV, NULL, 735),
    NAME_FUNC_OFFSET(12642, glVertexAttribs3dvNV, glVertexAttribs3dvNV, NULL, 736),
    NAME_FUNC_OFFSET(12663, glVertexAttribs3fvNV, glVertexAttribs3fvNV, NULL, 737),
    NAME_FUNC_OFFSET(12684, glVertexAttribs3svNV, glVertexAttribs3svNV, NULL, 738),
    NAME_FUNC_OFFSET(12705, glVertexAttribs4dvNV, glVertexAttribs4dvNV, NULL, 739),
    NAME_FUNC_OFFSET(12726, glVertexAttribs4fvNV, glVertexAttribs4fvNV, NULL, 740),
    NAME_FUNC_OFFSET(12747, glVertexAttribs4svNV, glVertexAttribs4svNV, NULL, 741),
    NAME_FUNC_OFFSET(12768, glVertexAttribs4ubvNV, glVertexAttribs4ubvNV, NULL, 742),
    NAME_FUNC_OFFSET(12790, glGetTexBumpParameterfvATI, glGetTexBumpParameterfvATI, NULL, 743),
    NAME_FUNC_OFFSET(12817, glGetTexBumpParameterivATI, glGetTexBumpParameterivATI, NULL, 744),
    NAME_FUNC_OFFSET(12844, glTexBumpParameterfvATI, glTexBumpParameterfvATI, NULL, 745),
    NAME_FUNC_OFFSET(12868, glTexBumpParameterivATI, glTexBumpParameterivATI, NULL, 746),
    NAME_FUNC_OFFSET(12892, glAlphaFragmentOp1ATI, glAlphaFragmentOp1ATI, NULL, 747),
    NAME_FUNC_OFFSET(12914, glAlphaFragmentOp2ATI, glAlphaFragmentOp2ATI, NULL, 748),
    NAME_FUNC_OFFSET(12936, glAlphaFragmentOp3ATI, glAlphaFragmentOp3ATI, NULL, 749),
    NAME_FUNC_OFFSET(12958, glBeginFragmentShaderATI, glBeginFragmentShaderATI, NULL, 750),
    NAME_FUNC_OFFSET(12983, glBindFragmentShaderATI, glBindFragmentShaderATI, NULL, 751),
    NAME_FUNC_OFFSET(13007, glColorFragmentOp1ATI, glColorFragmentOp1ATI, NULL, 752),
    NAME_FUNC_OFFSET(13029, glColorFragmentOp2ATI, glColorFragmentOp2ATI, NULL, 753),
    NAME_FUNC_OFFSET(13051, glColorFragmentOp3ATI, glColorFragmentOp3ATI, NULL, 754),
    NAME_FUNC_OFFSET(13073, glDeleteFragmentShaderATI, glDeleteFragmentShaderATI, NULL, 755),
    NAME_FUNC_OFFSET(13099, glEndFragmentShaderATI, glEndFragmentShaderATI, NULL, 756),
    NAME_FUNC_OFFSET(13122, glGenFragmentShadersATI, glGenFragmentShadersATI, NULL, 757),
    NAME_FUNC_OFFSET(13146, glPassTexCoordATI, glPassTexCoordATI, NULL, 758),
    NAME_FUNC_OFFSET(13164, glSampleMapATI, glSampleMapATI, NULL, 759),
    NAME_FUNC_OFFSET(13179, glSetFragmentShaderConstantATI, glSetFragmentShaderConstantATI, NULL, 760),
    NAME_FUNC_OFFSET(13210, glPointParameteriNV, glPointParameteriNV, NULL, 761),
    NAME_FUNC_OFFSET(13230, glPointParameterivNV, glPointParameterivNV, NULL, 762),
    NAME_FUNC_OFFSET(13251, gl_dispatch_stub_763, gl_dispatch_stub_763, NULL, 763),
    NAME_FUNC_OFFSET(13274, gl_dispatch_stub_764, gl_dispatch_stub_764, NULL, 764),
    NAME_FUNC_OFFSET(13297, gl_dispatch_stub_765, gl_dispatch_stub_765, NULL, 765),
    NAME_FUNC_OFFSET(13323, gl_dispatch_stub_766, gl_dispatch_stub_766, NULL, 766),
    NAME_FUNC_OFFSET(13346, gl_dispatch_stub_767, gl_dispatch_stub_767, NULL, 767),
    NAME_FUNC_OFFSET(13367, glGetProgramNamedParameterdvNV, glGetProgramNamedParameterdvNV, NULL, 768),
    NAME_FUNC_OFFSET(13398, glGetProgramNamedParameterfvNV, glGetProgramNamedParameterfvNV, NULL, 769),
    NAME_FUNC_OFFSET(13429, glProgramNamedParameter4dNV, glProgramNamedParameter4dNV, NULL, 770),
    NAME_FUNC_OFFSET(13457, glProgramNamedParameter4dvNV, glProgramNamedParameter4dvNV, NULL, 771),
    NAME_FUNC_OFFSET(13486, glProgramNamedParameter4fNV, glProgramNamedParameter4fNV, NULL, 772),
    NAME_FUNC_OFFSET(13514, glProgramNamedParameter4fvNV, glProgramNamedParameter4fvNV, NULL, 773),
    NAME_FUNC_OFFSET(13543, glPrimitiveRestartIndexNV, glPrimitiveRestartIndexNV, NULL, 774),
    NAME_FUNC_OFFSET(13569, glPrimitiveRestartNV, glPrimitiveRestartNV, NULL, 775),
    NAME_FUNC_OFFSET(13590, gl_dispatch_stub_776, gl_dispatch_stub_776, NULL, 776),
    NAME_FUNC_OFFSET(13607, gl_dispatch_stub_777, gl_dispatch_stub_777, NULL, 777),
    NAME_FUNC_OFFSET(13634, glBindFramebufferEXT, glBindFramebufferEXT, NULL, 778),
    NAME_FUNC_OFFSET(13655, glBindRenderbufferEXT, glBindRenderbufferEXT, NULL, 779),
    NAME_FUNC_OFFSET(13677, glCheckFramebufferStatusEXT, glCheckFramebufferStatusEXT, NULL, 780),
    NAME_FUNC_OFFSET(13705, glDeleteFramebuffersEXT, glDeleteFramebuffersEXT, NULL, 781),
    NAME_FUNC_OFFSET(13729, glDeleteRenderbuffersEXT, glDeleteRenderbuffersEXT, NULL, 782),
    NAME_FUNC_OFFSET(13754, glFramebufferRenderbufferEXT, glFramebufferRenderbufferEXT, NULL, 783),
    NAME_FUNC_OFFSET(13783, glFramebufferTexture1DEXT, glFramebufferTexture1DEXT, NULL, 784),
    NAME_FUNC_OFFSET(13809, glFramebufferTexture2DEXT, glFramebufferTexture2DEXT, NULL, 785),
    NAME_FUNC_OFFSET(13835, glFramebufferTexture3DEXT, glFramebufferTexture3DEXT, NULL, 786),
    NAME_FUNC_OFFSET(13861, glGenFramebuffersEXT, glGenFramebuffersEXT, NULL, 787),
    NAME_FUNC_OFFSET(13882, glGenRenderbuffersEXT, glGenRenderbuffersEXT, NULL, 788),
    NAME_FUNC_OFFSET(13904, glGenerateMipmapEXT, glGenerateMipmapEXT, NULL, 789),
    NAME_FUNC_OFFSET(13924, glGetFramebufferAttachmentParameterivEXT, glGetFramebufferAttachmentParameterivEXT, NULL, 790),
    NAME_FUNC_OFFSET(13965, glGetRenderbufferParameterivEXT, glGetRenderbufferParameterivEXT, NULL, 791),
    NAME_FUNC_OFFSET(13997, glIsFramebufferEXT, glIsFramebufferEXT, NULL, 792),
    NAME_FUNC_OFFSET(14016, glIsRenderbufferEXT, glIsRenderbufferEXT, NULL, 793),
    NAME_FUNC_OFFSET(14036, glRenderbufferStorageEXT, glRenderbufferStorageEXT, NULL, 794),
    NAME_FUNC_OFFSET(14061, gl_dispatch_stub_795, gl_dispatch_stub_795, NULL, 795),
    NAME_FUNC_OFFSET(14082, gl_dispatch_stub_796, gl_dispatch_stub_796, NULL, 796),
    NAME_FUNC_OFFSET(14106, gl_dispatch_stub_797, gl_dispatch_stub_797, NULL, 797),
    NAME_FUNC_OFFSET(14136, glBindFragDataLocationEXT, glBindFragDataLocationEXT, NULL, 798),
    NAME_FUNC_OFFSET(14162, glGetFragDataLocationEXT, glGetFragDataLocationEXT, NULL, 799),
    NAME_FUNC_OFFSET(14187, glGetUniformuivEXT, glGetUniformuivEXT, NULL, 800),
    NAME_FUNC_OFFSET(14206, glGetVertexAttribIivEXT, glGetVertexAttribIivEXT, NULL, 801),
    NAME_FUNC_OFFSET(14230, glGetVertexAttribIuivEXT, glGetVertexAttribIuivEXT, NULL, 802),
    NAME_FUNC_OFFSET(14255, glUniform1uiEXT, glUniform1uiEXT, NULL, 803),
    NAME_FUNC_OFFSET(14271, glUniform1uivEXT, glUniform1uivEXT, NULL, 804),
    NAME_FUNC_OFFSET(14288, glUniform2uiEXT, glUniform2uiEXT, NULL, 805),
    NAME_FUNC_OFFSET(14304, glUniform2uivEXT, glUniform2uivEXT, NULL, 806),
    NAME_FUNC_OFFSET(14321, glUniform3uiEXT, glUniform3uiEXT, NULL, 807),
    NAME_FUNC_OFFSET(14337, glUniform3uivEXT, glUniform3uivEXT, NULL, 808),
    NAME_FUNC_OFFSET(14354, glUniform4uiEXT, glUniform4uiEXT, NULL, 809),
    NAME_FUNC_OFFSET(14370, glUniform4uivEXT, glUniform4uivEXT, NULL, 810),
    NAME_FUNC_OFFSET(14387, glVertexAttribI1iEXT, glVertexAttribI1iEXT, NULL, 811),
    NAME_FUNC_OFFSET(14408, glVertexAttribI1ivEXT, glVertexAttribI1ivEXT, NULL, 812),
    NAME_FUNC_OFFSET(14430, glVertexAttribI1uiEXT, glVertexAttribI1uiEXT, NULL, 813),
    NAME_FUNC_OFFSET(14452, glVertexAttribI1uivEXT, glVertexAttribI1uivEXT, NULL, 814),
    NAME_FUNC_OFFSET(14475, glVertexAttribI2iEXT, glVertexAttribI2iEXT, NULL, 815),
    NAME_FUNC_OFFSET(14496, glVertexAttribI2ivEXT, glVertexAttribI2ivEXT, NULL, 816),
    NAME_FUNC_OFFSET(14518, glVertexAttribI2uiEXT, glVertexAttribI2uiEXT, NULL, 817),
    NAME_FUNC_OFFSET(14540, glVertexAttribI2uivEXT, glVertexAttribI2uivEXT, NULL, 818),
    NAME_FUNC_OFFSET(14563, glVertexAttribI3iEXT, glVertexAttribI3iEXT, NULL, 819),
    NAME_FUNC_OFFSET(14584, glVertexAttribI3ivEXT, glVertexAttribI3ivEXT, NULL, 820),
    NAME_FUNC_OFFSET(14606, glVertexAttribI3uiEXT, glVertexAttribI3uiEXT, NULL, 821),
    NAME_FUNC_OFFSET(14628, glVertexAttribI3uivEXT, glVertexAttribI3uivEXT, NULL, 822),
    NAME_FUNC_OFFSET(14651, glVertexAttribI4bvEXT, glVertexAttribI4bvEXT, NULL, 823),
    NAME_FUNC_OFFSET(14673, glVertexAttribI4iEXT, glVertexAttribI4iEXT, NULL, 824),
    NAME_FUNC_OFFSET(14694, glVertexAttribI4ivEXT, glVertexAttribI4ivEXT, NULL, 825),
    NAME_FUNC_OFFSET(14716, glVertexAttribI4svEXT, glVertexAttribI4svEXT, NULL, 826),
    NAME_FUNC_OFFSET(14738, glVertexAttribI4ubvEXT, glVertexAttribI4ubvEXT, NULL, 827),
    NAME_FUNC_OFFSET(14761, glVertexAttribI4uiEXT, glVertexAttribI4uiEXT, NULL, 828),
    NAME_FUNC_OFFSET(14783, glVertexAttribI4uivEXT, glVertexAttribI4uivEXT, NULL, 829),
    NAME_FUNC_OFFSET(14806, glVertexAttribI4usvEXT, glVertexAttribI4usvEXT, NULL, 830),
    NAME_FUNC_OFFSET(14829, glVertexAttribIPointerEXT, glVertexAttribIPointerEXT, NULL, 831),
    NAME_FUNC_OFFSET(14855, glFramebufferTextureLayerEXT, glFramebufferTextureLayerEXT, NULL, 832),
    NAME_FUNC_OFFSET(14884, glColorMaskIndexedEXT, glColorMaskIndexedEXT, NULL, 833),
    NAME_FUNC_OFFSET(14906, glDisableIndexedEXT, glDisableIndexedEXT, NULL, 834),
    NAME_FUNC_OFFSET(14926, glEnableIndexedEXT, glEnableIndexedEXT, NULL, 835),
    NAME_FUNC_OFFSET(14945, glGetBooleanIndexedvEXT, glGetBooleanIndexedvEXT, NULL, 836),
    NAME_FUNC_OFFSET(14969, glGetIntegerIndexedvEXT, glGetIntegerIndexedvEXT, NULL, 837),
    NAME_FUNC_OFFSET(14993, glIsEnabledIndexedEXT, glIsEnabledIndexedEXT, NULL, 838),
    NAME_FUNC_OFFSET(15015, glClearColorIiEXT, glClearColorIiEXT, NULL, 839),
    NAME_FUNC_OFFSET(15033, glClearColorIuiEXT, glClearColorIuiEXT, NULL, 840),
    NAME_FUNC_OFFSET(15052, glGetTexParameterIivEXT, glGetTexParameterIivEXT, NULL, 841),
    NAME_FUNC_OFFSET(15076, glGetTexParameterIuivEXT, glGetTexParameterIuivEXT, NULL, 842),
    NAME_FUNC_OFFSET(15101, glTexParameterIivEXT, glTexParameterIivEXT, NULL, 843),
    NAME_FUNC_OFFSET(15122, glTexParameterIuivEXT, glTexParameterIuivEXT, NULL, 844),
    NAME_FUNC_OFFSET(15144, glBeginConditionalRenderNV, glBeginConditionalRenderNV, NULL, 845),
    NAME_FUNC_OFFSET(15171, glEndConditionalRenderNV, glEndConditionalRenderNV, NULL, 846),
    NAME_FUNC_OFFSET(15196, glBeginTransformFeedbackEXT, glBeginTransformFeedbackEXT, NULL, 847),
    NAME_FUNC_OFFSET(15224, glBindBufferBaseEXT, glBindBufferBaseEXT, NULL, 848),
    NAME_FUNC_OFFSET(15244, glBindBufferOffsetEXT, glBindBufferOffsetEXT, NULL, 849),
    NAME_FUNC_OFFSET(15266, glBindBufferRangeEXT, glBindBufferRangeEXT, NULL, 850),
    NAME_FUNC_OFFSET(15287, glEndTransformFeedbackEXT, glEndTransformFeedbackEXT, NULL, 851),
    NAME_FUNC_OFFSET(15313, glGetTransformFeedbackVaryingEXT, glGetTransformFeedbackVaryingEXT, NULL, 852),
    NAME_FUNC_OFFSET(15346, glTransformFeedbackVaryingsEXT, glTransformFeedbackVaryingsEXT, NULL, 853),
    NAME_FUNC_OFFSET(15377, glProvokingVertexEXT, glProvokingVertexEXT, NULL, 854),
    NAME_FUNC_OFFSET(15398, gl_dispatch_stub_855, gl_dispatch_stub_855, NULL, 855),
    NAME_FUNC_OFFSET(15429, gl_dispatch_stub_856, gl_dispatch_stub_856, NULL, 856),
    NAME_FUNC_OFFSET(15449, glGetObjectParameterivAPPLE, glGetObjectParameterivAPPLE, NULL, 857),
    NAME_FUNC_OFFSET(15477, glObjectPurgeableAPPLE, glObjectPurgeableAPPLE, NULL, 858),
    NAME_FUNC_OFFSET(15500, glObjectUnpurgeableAPPLE, glObjectUnpurgeableAPPLE, NULL, 859),
    NAME_FUNC_OFFSET(15525, glActiveProgramEXT, glActiveProgramEXT, NULL, 860),
    NAME_FUNC_OFFSET(15544, glCreateShaderProgramEXT, glCreateShaderProgramEXT, NULL, 861),
    NAME_FUNC_OFFSET(15569, glUseShaderProgramEXT, glUseShaderProgramEXT, NULL, 862),
    NAME_FUNC_OFFSET(15591, gl_dispatch_stub_863, gl_dispatch_stub_863, NULL, 863),
    NAME_FUNC_OFFSET(15616, gl_dispatch_stub_864, gl_dispatch_stub_864, NULL, 864),
    NAME_FUNC_OFFSET(15645, gl_dispatch_stub_865, gl_dispatch_stub_865, NULL, 865),
    NAME_FUNC_OFFSET(15676, gl_dispatch_stub_866, gl_dispatch_stub_866, NULL, 866),
    NAME_FUNC_OFFSET(15700, gl_dispatch_stub_867, gl_dispatch_stub_867, NULL, 867),
    NAME_FUNC_OFFSET(15725, glEGLImageTargetRenderbufferStorageOES, glEGLImageTargetRenderbufferStorageOES, NULL, 868),
    NAME_FUNC_OFFSET(15764, glEGLImageTargetTexture2DOES, glEGLImageTargetTexture2DOES, NULL, 869),
    NAME_FUNC_OFFSET(15793, glArrayElement, glArrayElement, NULL, 306),
    NAME_FUNC_OFFSET(15811, glBindTexture, glBindTexture, NULL, 307),
    NAME_FUNC_OFFSET(15828, glDrawArrays, glDrawArrays, NULL, 310),
    NAME_FUNC_OFFSET(15844, glAreTexturesResident, glAreTexturesResidentEXT, glAreTexturesResidentEXT, 322),
    NAME_FUNC_OFFSET(15869, glCopyTexImage1D, glCopyTexImage1D, NULL, 323),
    NAME_FUNC_OFFSET(15889, glCopyTexImage2D, glCopyTexImage2D, NULL, 324),
    NAME_FUNC_OFFSET(15909, glCopyTexSubImage1D, glCopyTexSubImage1D, NULL, 325),
    NAME_FUNC_OFFSET(15932, glCopyTexSubImage2D, glCopyTexSubImage2D, NULL, 326),
    NAME_FUNC_OFFSET(15955, glDeleteTextures, glDeleteTexturesEXT, glDeleteTexturesEXT, 327),
    NAME_FUNC_OFFSET(15975, glGenTextures, glGenTexturesEXT, glGenTexturesEXT, 328),
    NAME_FUNC_OFFSET(15992, glGetPointerv, glGetPointerv, NULL, 329),
    NAME_FUNC_OFFSET(16009, glIsTexture, glIsTextureEXT, glIsTextureEXT, 330),
    NAME_FUNC_OFFSET(16024, glPrioritizeTextures, glPrioritizeTextures, NULL, 331),
    NAME_FUNC_OFFSET(16048, glTexSubImage1D, glTexSubImage1D, NULL, 332),
    NAME_FUNC_OFFSET(16067, glTexSubImage2D, glTexSubImage2D, NULL, 333),
    NAME_FUNC_OFFSET(16086, glBlendColor, glBlendColor, NULL, 336),
    NAME_FUNC_OFFSET(16102, glBlendEquation, glBlendEquation, NULL, 337),
    NAME_FUNC_OFFSET(16121, glDrawRangeElements, glDrawRangeElements, NULL, 338),
    NAME_FUNC_OFFSET(16144, glColorTable, glColorTable, NULL, 339),
    NAME_FUNC_OFFSET(16160, glColorTable, glColorTable, NULL, 339),
    NAME_FUNC_OFFSET(16176, glColorTableParameterfv, glColorTableParameterfv, NULL, 340),
    NAME_FUNC_OFFSET(16203, glColorTableParameteriv, glColorTableParameteriv, NULL, 341),
    NAME_FUNC_OFFSET(16230, glCopyColorTable, glCopyColorTable, NULL, 342),
    NAME_FUNC_OFFSET(16250, glGetColorTable, glGetColorTableEXT, glGetColorTableEXT, 343),
    NAME_FUNC_OFFSET(16269, glGetColorTable, glGetColorTableEXT, glGetColorTableEXT, 343),
    NAME_FUNC_OFFSET(16288, glGetColorTableParameterfv, glGetColorTableParameterfvEXT, glGetColorTableParameterfvEXT, 344),
    NAME_FUNC_OFFSET(16318, glGetColorTableParameterfv, glGetColorTableParameterfvEXT, glGetColorTableParameterfvEXT, 344),
    NAME_FUNC_OFFSET(16348, glGetColorTableParameteriv, glGetColorTableParameterivEXT, glGetColorTableParameterivEXT, 345),
    NAME_FUNC_OFFSET(16378, glGetColorTableParameteriv, glGetColorTableParameterivEXT, glGetColorTableParameterivEXT, 345),
    NAME_FUNC_OFFSET(16408, glColorSubTable, glColorSubTable, NULL, 346),
    NAME_FUNC_OFFSET(16427, glCopyColorSubTable, glCopyColorSubTable, NULL, 347),
    NAME_FUNC_OFFSET(16450, glConvolutionFilter1D, glConvolutionFilter1D, NULL, 348),
    NAME_FUNC_OFFSET(16475, glConvolutionFilter2D, glConvolutionFilter2D, NULL, 349),
    NAME_FUNC_OFFSET(16500, glConvolutionParameterf, glConvolutionParameterf, NULL, 350),
    NAME_FUNC_OFFSET(16527, glConvolutionParameterfv, glConvolutionParameterfv, NULL, 351),
    NAME_FUNC_OFFSET(16555, glConvolutionParameteri, glConvolutionParameteri, NULL, 352),
    NAME_FUNC_OFFSET(16582, glConvolutionParameteriv, glConvolutionParameteriv, NULL, 353),
    NAME_FUNC_OFFSET(16610, glCopyConvolutionFilter1D, glCopyConvolutionFilter1D, NULL, 354),
    NAME_FUNC_OFFSET(16639, glCopyConvolutionFilter2D, glCopyConvolutionFilter2D, NULL, 355),
    NAME_FUNC_OFFSET(16668, glGetConvolutionFilter, gl_dispatch_stub_356, gl_dispatch_stub_356, 356),
    NAME_FUNC_OFFSET(16694, glGetConvolutionParameterfv, gl_dispatch_stub_357, gl_dispatch_stub_357, 357),
    NAME_FUNC_OFFSET(16725, glGetConvolutionParameteriv, gl_dispatch_stub_358, gl_dispatch_stub_358, 358),
    NAME_FUNC_OFFSET(16756, glGetSeparableFilter, gl_dispatch_stub_359, gl_dispatch_stub_359, 359),
    NAME_FUNC_OFFSET(16780, glSeparableFilter2D, glSeparableFilter2D, NULL, 360),
    NAME_FUNC_OFFSET(16803, glGetHistogram, gl_dispatch_stub_361, gl_dispatch_stub_361, 361),
    NAME_FUNC_OFFSET(16821, glGetHistogramParameterfv, gl_dispatch_stub_362, gl_dispatch_stub_362, 362),
    NAME_FUNC_OFFSET(16850, glGetHistogramParameteriv, gl_dispatch_stub_363, gl_dispatch_stub_363, 363),
    NAME_FUNC_OFFSET(16879, glGetMinmax, gl_dispatch_stub_364, gl_dispatch_stub_364, 364),
    NAME_FUNC_OFFSET(16894, glGetMinmaxParameterfv, gl_dispatch_stub_365, gl_dispatch_stub_365, 365),
    NAME_FUNC_OFFSET(16920, glGetMinmaxParameteriv, gl_dispatch_stub_366, gl_dispatch_stub_366, 366),
    NAME_FUNC_OFFSET(16946, glHistogram, glHistogram, NULL, 367),
    NAME_FUNC_OFFSET(16961, glMinmax, glMinmax, NULL, 368),
    NAME_FUNC_OFFSET(16973, glResetHistogram, glResetHistogram, NULL, 369),
    NAME_FUNC_OFFSET(16993, glResetMinmax, glResetMinmax, NULL, 370),
    NAME_FUNC_OFFSET(17010, glTexImage3D, glTexImage3D, NULL, 371),
    NAME_FUNC_OFFSET(17026, glTexSubImage3D, glTexSubImage3D, NULL, 372),
    NAME_FUNC_OFFSET(17045, glCopyTexSubImage3D, glCopyTexSubImage3D, NULL, 373),
    NAME_FUNC_OFFSET(17068, glActiveTextureARB, glActiveTextureARB, NULL, 374),
    NAME_FUNC_OFFSET(17084, glClientActiveTextureARB, glClientActiveTextureARB, NULL, 375),
    NAME_FUNC_OFFSET(17106, glMultiTexCoord1dARB, glMultiTexCoord1dARB, NULL, 376),
    NAME_FUNC_OFFSET(17124, glMultiTexCoord1dvARB, glMultiTexCoord1dvARB, NULL, 377),
    NAME_FUNC_OFFSET(17143, glMultiTexCoord1fARB, glMultiTexCoord1fARB, NULL, 378),
    NAME_FUNC_OFFSET(17161, glMultiTexCoord1fvARB, glMultiTexCoord1fvARB, NULL, 379),
    NAME_FUNC_OFFSET(17180, glMultiTexCoord1iARB, glMultiTexCoord1iARB, NULL, 380),
    NAME_FUNC_OFFSET(17198, glMultiTexCoord1ivARB, glMultiTexCoord1ivARB, NULL, 381),
    NAME_FUNC_OFFSET(17217, glMultiTexCoord1sARB, glMultiTexCoord1sARB, NULL, 382),
    NAME_FUNC_OFFSET(17235, glMultiTexCoord1svARB, glMultiTexCoord1svARB, NULL, 383),
    NAME_FUNC_OFFSET(17254, glMultiTexCoord2dARB, glMultiTexCoord2dARB, NULL, 384),
    NAME_FUNC_OFFSET(17272, glMultiTexCoord2dvARB, glMultiTexCoord2dvARB, NULL, 385),
    NAME_FUNC_OFFSET(17291, glMultiTexCoord2fARB, glMultiTexCoord2fARB, NULL, 386),
    NAME_FUNC_OFFSET(17309, glMultiTexCoord2fvARB, glMultiTexCoord2fvARB, NULL, 387),
    NAME_FUNC_OFFSET(17328, glMultiTexCoord2iARB, glMultiTexCoord2iARB, NULL, 388),
    NAME_FUNC_OFFSET(17346, glMultiTexCoord2ivARB, glMultiTexCoord2ivARB, NULL, 389),
    NAME_FUNC_OFFSET(17365, glMultiTexCoord2sARB, glMultiTexCoord2sARB, NULL, 390),
    NAME_FUNC_OFFSET(17383, glMultiTexCoord2svARB, glMultiTexCoord2svARB, NULL, 391),
    NAME_FUNC_OFFSET(17402, glMultiTexCoord3dARB, glMultiTexCoord3dARB, NULL, 392),
    NAME_FUNC_OFFSET(17420, glMultiTexCoord3dvARB, glMultiTexCoord3dvARB, NULL, 393),
    NAME_FUNC_OFFSET(17439, glMultiTexCoord3fARB, glMultiTexCoord3fARB, NULL, 394),
    NAME_FUNC_OFFSET(17457, glMultiTexCoord3fvARB, glMultiTexCoord3fvARB, NULL, 395),
    NAME_FUNC_OFFSET(17476, glMultiTexCoord3iARB, glMultiTexCoord3iARB, NULL, 396),
    NAME_FUNC_OFFSET(17494, glMultiTexCoord3ivARB, glMultiTexCoord3ivARB, NULL, 397),
    NAME_FUNC_OFFSET(17513, glMultiTexCoord3sARB, glMultiTexCoord3sARB, NULL, 398),
    NAME_FUNC_OFFSET(17531, glMultiTexCoord3svARB, glMultiTexCoord3svARB, NULL, 399),
    NAME_FUNC_OFFSET(17550, glMultiTexCoord4dARB, glMultiTexCoord4dARB, NULL, 400),
    NAME_FUNC_OFFSET(17568, glMultiTexCoord4dvARB, glMultiTexCoord4dvARB, NULL, 401),
    NAME_FUNC_OFFSET(17587, glMultiTexCoord4fARB, glMultiTexCoord4fARB, NULL, 402),
    NAME_FUNC_OFFSET(17605, glMultiTexCoord4fvARB, glMultiTexCoord4fvARB, NULL, 403),
    NAME_FUNC_OFFSET(17624, glMultiTexCoord4iARB, glMultiTexCoord4iARB, NULL, 404),
    NAME_FUNC_OFFSET(17642, glMultiTexCoord4ivARB, glMultiTexCoord4ivARB, NULL, 405),
    NAME_FUNC_OFFSET(17661, glMultiTexCoord4sARB, glMultiTexCoord4sARB, NULL, 406),
    NAME_FUNC_OFFSET(17679, glMultiTexCoord4svARB, glMultiTexCoord4svARB, NULL, 407),
    NAME_FUNC_OFFSET(17698, glStencilOpSeparate, glStencilOpSeparate, NULL, 423),
    NAME_FUNC_OFFSET(17721, glDrawArraysInstanced, glDrawArraysInstanced, NULL, 430),
    NAME_FUNC_OFFSET(17746, glDrawArraysInstanced, glDrawArraysInstanced, NULL, 430),
    NAME_FUNC_OFFSET(17771, glDrawElementsInstanced, glDrawElementsInstanced, NULL, 431),
    NAME_FUNC_OFFSET(17798, glDrawElementsInstanced, glDrawElementsInstanced, NULL, 431),
    NAME_FUNC_OFFSET(17825, glLoadTransposeMatrixdARB, glLoadTransposeMatrixdARB, NULL, 432),
    NAME_FUNC_OFFSET(17848, glLoadTransposeMatrixfARB, glLoadTransposeMatrixfARB, NULL, 433),
    NAME_FUNC_OFFSET(17871, glMultTransposeMatrixdARB, glMultTransposeMatrixdARB, NULL, 434),
    NAME_FUNC_OFFSET(17894, glMultTransposeMatrixfARB, glMultTransposeMatrixfARB, NULL, 435),
    NAME_FUNC_OFFSET(17917, glSampleCoverageARB, glSampleCoverageARB, NULL, 436),
    NAME_FUNC_OFFSET(17934, glCompressedTexImage1DARB, glCompressedTexImage1DARB, NULL, 437),
    NAME_FUNC_OFFSET(17957, glCompressedTexImage2DARB, glCompressedTexImage2DARB, NULL, 438),
    NAME_FUNC_OFFSET(17980, glCompressedTexImage3DARB, glCompressedTexImage3DARB, NULL, 439),
    NAME_FUNC_OFFSET(18003, glCompressedTexSubImage1DARB, glCompressedTexSubImage1DARB, NULL, 440),
    NAME_FUNC_OFFSET(18029, glCompressedTexSubImage2DARB, glCompressedTexSubImage2DARB, NULL, 441),
    NAME_FUNC_OFFSET(18055, glCompressedTexSubImage3DARB, glCompressedTexSubImage3DARB, NULL, 442),
    NAME_FUNC_OFFSET(18081, glGetCompressedTexImageARB, glGetCompressedTexImageARB, NULL, 443),
    NAME_FUNC_OFFSET(18105, glDisableVertexAttribArrayARB, glDisableVertexAttribArrayARB, NULL, 444),
    NAME_FUNC_OFFSET(18132, glEnableVertexAttribArrayARB, glEnableVertexAttribArrayARB, NULL, 445),
    NAME_FUNC_OFFSET(18158, glGetVertexAttribdvARB, glGetVertexAttribdvARB, NULL, 452),
    NAME_FUNC_OFFSET(18178, glGetVertexAttribfvARB, glGetVertexAttribfvARB, NULL, 453),
    NAME_FUNC_OFFSET(18198, glGetVertexAttribivARB, glGetVertexAttribivARB, NULL, 454),
    NAME_FUNC_OFFSET(18218, glProgramEnvParameter4dARB, glProgramEnvParameter4dARB, NULL, 455),
    NAME_FUNC_OFFSET(18241, glProgramEnvParameter4dvARB, glProgramEnvParameter4dvARB, NULL, 456),
    NAME_FUNC_OFFSET(18265, glProgramEnvParameter4fARB, glProgramEnvParameter4fARB, NULL, 457),
    NAME_FUNC_OFFSET(18288, glProgramEnvParameter4fvARB, glProgramEnvParameter4fvARB, NULL, 458),
    NAME_FUNC_OFFSET(18312, glVertexAttrib1dARB, glVertexAttrib1dARB, NULL, 464),
    NAME_FUNC_OFFSET(18329, glVertexAttrib1dvARB, glVertexAttrib1dvARB, NULL, 465),
    NAME_FUNC_OFFSET(18347, glVertexAttrib1fARB, glVertexAttrib1fARB, NULL, 466),
    NAME_FUNC_OFFSET(18364, glVertexAttrib1fvARB, glVertexAttrib1fvARB, NULL, 467),
    NAME_FUNC_OFFSET(18382, glVertexAttrib1sARB, glVertexAttrib1sARB, NULL, 468),
    NAME_FUNC_OFFSET(18399, glVertexAttrib1svARB, glVertexAttrib1svARB, NULL, 469),
    NAME_FUNC_OFFSET(18417, glVertexAttrib2dARB, glVertexAttrib2dARB, NULL, 470),
    NAME_FUNC_OFFSET(18434, glVertexAttrib2dvARB, glVertexAttrib2dvARB, NULL, 471),
    NAME_FUNC_OFFSET(18452, glVertexAttrib2fARB, glVertexAttrib2fARB, NULL, 472),
    NAME_FUNC_OFFSET(18469, glVertexAttrib2fvARB, glVertexAttrib2fvARB, NULL, 473),
    NAME_FUNC_OFFSET(18487, glVertexAttrib2sARB, glVertexAttrib2sARB, NULL, 474),
    NAME_FUNC_OFFSET(18504, glVertexAttrib2svARB, glVertexAttrib2svARB, NULL, 475),
    NAME_FUNC_OFFSET(18522, glVertexAttrib3dARB, glVertexAttrib3dARB, NULL, 476),
    NAME_FUNC_OFFSET(18539, glVertexAttrib3dvARB, glVertexAttrib3dvARB, NULL, 477),
    NAME_FUNC_OFFSET(18557, glVertexAttrib3fARB, glVertexAttrib3fARB, NULL, 478),
    NAME_FUNC_OFFSET(18574, glVertexAttrib3fvARB, glVertexAttrib3fvARB, NULL, 479),
    NAME_FUNC_OFFSET(18592, glVertexAttrib3sARB, glVertexAttrib3sARB, NULL, 480),
    NAME_FUNC_OFFSET(18609, glVertexAttrib3svARB, glVertexAttrib3svARB, NULL, 481),
    NAME_FUNC_OFFSET(18627, glVertexAttrib4NbvARB, glVertexAttrib4NbvARB, NULL, 482),
    NAME_FUNC_OFFSET(18646, glVertexAttrib4NivARB, glVertexAttrib4NivARB, NULL, 483),
    NAME_FUNC_OFFSET(18665, glVertexAttrib4NsvARB, glVertexAttrib4NsvARB, NULL, 484),
    NAME_FUNC_OFFSET(18684, glVertexAttrib4NubARB, glVertexAttrib4NubARB, NULL, 485),
    NAME_FUNC_OFFSET(18703, glVertexAttrib4NubvARB, glVertexAttrib4NubvARB, NULL, 486),
    NAME_FUNC_OFFSET(18723, glVertexAttrib4NuivARB, glVertexAttrib4NuivARB, NULL, 487),
    NAME_FUNC_OFFSET(18743, glVertexAttrib4NusvARB, glVertexAttrib4NusvARB, NULL, 488),
    NAME_FUNC_OFFSET(18763, glVertexAttrib4bvARB, glVertexAttrib4bvARB, NULL, 489),
    NAME_FUNC_OFFSET(18781, glVertexAttrib4dARB, glVertexAttrib4dARB, NULL, 490),
    NAME_FUNC_OFFSET(18798, glVertexAttrib4dvARB, glVertexAttrib4dvARB, NULL, 491),
    NAME_FUNC_OFFSET(18816, glVertexAttrib4fARB, glVertexAttrib4fARB, NULL, 492),
    NAME_FUNC_OFFSET(18833, glVertexAttrib4fvARB, glVertexAttrib4fvARB, NULL, 493),
    NAME_FUNC_OFFSET(18851, glVertexAttrib4ivARB, glVertexAttrib4ivARB, NULL, 494),
    NAME_FUNC_OFFSET(18869, glVertexAttrib4sARB, glVertexAttrib4sARB, NULL, 495),
    NAME_FUNC_OFFSET(18886, glVertexAttrib4svARB, glVertexAttrib4svARB, NULL, 496),
    NAME_FUNC_OFFSET(18904, glVertexAttrib4ubvARB, glVertexAttrib4ubvARB, NULL, 497),
    NAME_FUNC_OFFSET(18923, glVertexAttrib4uivARB, glVertexAttrib4uivARB, NULL, 498),
    NAME_FUNC_OFFSET(18942, glVertexAttrib4usvARB, glVertexAttrib4usvARB, NULL, 499),
    NAME_FUNC_OFFSET(18961, glVertexAttribPointerARB, glVertexAttribPointerARB, NULL, 500),
    NAME_FUNC_OFFSET(18983, glBindBufferARB, glBindBufferARB, NULL, 501),
    NAME_FUNC_OFFSET(18996, glBufferDataARB, glBufferDataARB, NULL, 502),
    NAME_FUNC_OFFSET(19009, glBufferSubDataARB, glBufferSubDataARB, NULL, 503),
    NAME_FUNC_OFFSET(19025, glDeleteBuffersARB, glDeleteBuffersARB, NULL, 504),
    NAME_FUNC_OFFSET(19041, glGenBuffersARB, glGenBuffersARB, NULL, 505),
    NAME_FUNC_OFFSET(19054, glGetBufferParameterivARB, glGetBufferParameterivARB, NULL, 506),
    NAME_FUNC_OFFSET(19077, glGetBufferPointervARB, glGetBufferPointervARB, NULL, 507),
    NAME_FUNC_OFFSET(19097, glGetBufferSubDataARB, glGetBufferSubDataARB, NULL, 508),
    NAME_FUNC_OFFSET(19116, glIsBufferARB, glIsBufferARB, NULL, 509),
    NAME_FUNC_OFFSET(19127, glMapBufferARB, glMapBufferARB, NULL, 510),
    NAME_FUNC_OFFSET(19139, glUnmapBufferARB, glUnmapBufferARB, NULL, 511),
    NAME_FUNC_OFFSET(19153, glBeginQueryARB, glBeginQueryARB, NULL, 512),
    NAME_FUNC_OFFSET(19166, glDeleteQueriesARB, glDeleteQueriesARB, NULL, 513),
    NAME_FUNC_OFFSET(19182, glEndQueryARB, glEndQueryARB, NULL, 514),
    NAME_FUNC_OFFSET(19193, glGenQueriesARB, glGenQueriesARB, NULL, 515),
    NAME_FUNC_OFFSET(19206, glGetQueryObjectivARB, glGetQueryObjectivARB, NULL, 516),
    NAME_FUNC_OFFSET(19225, glGetQueryObjectuivARB, glGetQueryObjectuivARB, NULL, 517),
    NAME_FUNC_OFFSET(19245, glGetQueryivARB, glGetQueryivARB, NULL, 518),
    NAME_FUNC_OFFSET(19258, glIsQueryARB, glIsQueryARB, NULL, 519),
    NAME_FUNC_OFFSET(19268, glCompileShaderARB, glCompileShaderARB, NULL, 521),
    NAME_FUNC_OFFSET(19284, glGetActiveUniformARB, glGetActiveUniformARB, NULL, 526),
    NAME_FUNC_OFFSET(19303, glGetShaderSourceARB, glGetShaderSourceARB, NULL, 532),
    NAME_FUNC_OFFSET(19321, glGetUniformLocationARB, glGetUniformLocationARB, NULL, 533),
    NAME_FUNC_OFFSET(19342, glGetUniformfvARB, glGetUniformfvARB, NULL, 534),
    NAME_FUNC_OFFSET(19357, glGetUniformivARB, glGetUniformivARB, NULL, 535),
    NAME_FUNC_OFFSET(19372, glLinkProgramARB, glLinkProgramARB, NULL, 536),
    NAME_FUNC_OFFSET(19386, glShaderSourceARB, glShaderSourceARB, NULL, 537),
    NAME_FUNC_OFFSET(19401, glUniform1fARB, glUniform1fARB, NULL, 538),
    NAME_FUNC_OFFSET(19413, glUniform1fvARB, glUniform1fvARB, NULL, 539),
    NAME_FUNC_OFFSET(19426, glUniform1iARB, glUniform1iARB, NULL, 540),
    NAME_FUNC_OFFSET(19438, glUniform1ivARB, glUniform1ivARB, NULL, 541),
    NAME_FUNC_OFFSET(19451, glUniform2fARB, glUniform2fARB, NULL, 542),
    NAME_FUNC_OFFSET(19463, glUniform2fvARB, glUniform2fvARB, NULL, 543),
    NAME_FUNC_OFFSET(19476, glUniform2iARB, glUniform2iARB, NULL, 544),
    NAME_FUNC_OFFSET(19488, glUniform2ivARB, glUniform2ivARB, NULL, 545),
    NAME_FUNC_OFFSET(19501, glUniform3fARB, glUniform3fARB, NULL, 546),
    NAME_FUNC_OFFSET(19513, glUniform3fvARB, glUniform3fvARB, NULL, 547),
    NAME_FUNC_OFFSET(19526, glUniform3iARB, glUniform3iARB, NULL, 548),
    NAME_FUNC_OFFSET(19538, glUniform3ivARB, glUniform3ivARB, NULL, 549),
    NAME_FUNC_OFFSET(19551, glUniform4fARB, glUniform4fARB, NULL, 550),
    NAME_FUNC_OFFSET(19563, glUniform4fvARB, glUniform4fvARB, NULL, 551),
    NAME_FUNC_OFFSET(19576, glUniform4iARB, glUniform4iARB, NULL, 552),
    NAME_FUNC_OFFSET(19588, glUniform4ivARB, glUniform4ivARB, NULL, 553),
    NAME_FUNC_OFFSET(19601, glUniformMatrix2fvARB, glUniformMatrix2fvARB, NULL, 554),
    NAME_FUNC_OFFSET(19620, glUniformMatrix3fvARB, glUniformMatrix3fvARB, NULL, 555),
    NAME_FUNC_OFFSET(19639, glUniformMatrix4fvARB, glUniformMatrix4fvARB, NULL, 556),
    NAME_FUNC_OFFSET(19658, glUseProgramObjectARB, glUseProgramObjectARB, NULL, 557),
    NAME_FUNC_OFFSET(19671, glValidateProgramARB, glValidateProgramARB, NULL, 558),
    NAME_FUNC_OFFSET(19689, glBindAttribLocationARB, glBindAttribLocationARB, NULL, 559),
    NAME_FUNC_OFFSET(19710, glGetActiveAttribARB, glGetActiveAttribARB, NULL, 560),
    NAME_FUNC_OFFSET(19728, glGetAttribLocationARB, glGetAttribLocationARB, NULL, 561),
    NAME_FUNC_OFFSET(19748, glDrawBuffersARB, glDrawBuffersARB, NULL, 562),
    NAME_FUNC_OFFSET(19762, glDrawBuffersARB, glDrawBuffersARB, NULL, 562),
    NAME_FUNC_OFFSET(19779, glRenderbufferStorageMultisample, glRenderbufferStorageMultisample, NULL, 563),
    NAME_FUNC_OFFSET(19815, gl_dispatch_stub_596, gl_dispatch_stub_596, NULL, 596),
    NAME_FUNC_OFFSET(19831, gl_dispatch_stub_597, gl_dispatch_stub_597, NULL, 597),
    NAME_FUNC_OFFSET(19850, glPointParameterfEXT, glPointParameterfEXT, NULL, 604),
    NAME_FUNC_OFFSET(19868, glPointParameterfEXT, glPointParameterfEXT, NULL, 604),
    NAME_FUNC_OFFSET(19889, glPointParameterfEXT, glPointParameterfEXT, NULL, 604),
    NAME_FUNC_OFFSET(19911, glPointParameterfvEXT, glPointParameterfvEXT, NULL, 605),
    NAME_FUNC_OFFSET(19930, glPointParameterfvEXT, glPointParameterfvEXT, NULL, 605),
    NAME_FUNC_OFFSET(19952, glPointParameterfvEXT, glPointParameterfvEXT, NULL, 605),
    NAME_FUNC_OFFSET(19975, glSecondaryColor3bEXT, glSecondaryColor3bEXT, NULL, 608),
    NAME_FUNC_OFFSET(19994, glSecondaryColor3bvEXT, glSecondaryColor3bvEXT, NULL, 609),
    NAME_FUNC_OFFSET(20014, glSecondaryColor3dEXT, glSecondaryColor3dEXT, NULL, 610),
    NAME_FUNC_OFFSET(20033, glSecondaryColor3dvEXT, glSecondaryColor3dvEXT, NULL, 611),
    NAME_FUNC_OFFSET(20053, glSecondaryColor3fEXT, glSecondaryColor3fEXT, NULL, 612),
    NAME_FUNC_OFFSET(20072, glSecondaryColor3fvEXT, glSecondaryColor3fvEXT, NULL, 613),
    NAME_FUNC_OFFSET(20092, glSecondaryColor3iEXT, glSecondaryColor3iEXT, NULL, 614),
    NAME_FUNC_OFFSET(20111, glSecondaryColor3ivEXT, glSecondaryColor3ivEXT, NULL, 615),
    NAME_FUNC_OFFSET(20131, glSecondaryColor3sEXT, glSecondaryColor3sEXT, NULL, 616),
    NAME_FUNC_OFFSET(20150, glSecondaryColor3svEXT, glSecondaryColor3svEXT, NULL, 617),
    NAME_FUNC_OFFSET(20170, glSecondaryColor3ubEXT, glSecondaryColor3ubEXT, NULL, 618),
    NAME_FUNC_OFFSET(20190, glSecondaryColor3ubvEXT, glSecondaryColor3ubvEXT, NULL, 619),
    NAME_FUNC_OFFSET(20211, glSecondaryColor3uiEXT, glSecondaryColor3uiEXT, NULL, 620),
    NAME_FUNC_OFFSET(20231, glSecondaryColor3uivEXT, glSecondaryColor3uivEXT, NULL, 621),
    NAME_FUNC_OFFSET(20252, glSecondaryColor3usEXT, glSecondaryColor3usEXT, NULL, 622),
    NAME_FUNC_OFFSET(20272, glSecondaryColor3usvEXT, glSecondaryColor3usvEXT, NULL, 623),
    NAME_FUNC_OFFSET(20293, glSecondaryColorPointerEXT, glSecondaryColorPointerEXT, NULL, 624),
    NAME_FUNC_OFFSET(20317, glMultiDrawArraysEXT, glMultiDrawArraysEXT, NULL, 625),
    NAME_FUNC_OFFSET(20335, glMultiDrawElementsEXT, glMultiDrawElementsEXT, NULL, 626),
    NAME_FUNC_OFFSET(20355, glFogCoordPointerEXT, glFogCoordPointerEXT, NULL, 627),
    NAME_FUNC_OFFSET(20373, glFogCoorddEXT, glFogCoorddEXT, NULL, 628),
    NAME_FUNC_OFFSET(20385, glFogCoorddvEXT, glFogCoorddvEXT, NULL, 629),
    NAME_FUNC_OFFSET(20398, glFogCoordfEXT, glFogCoordfEXT, NULL, 630),
    NAME_FUNC_OFFSET(20410, glFogCoordfvEXT, glFogCoordfvEXT, NULL, 631),
    NAME_FUNC_OFFSET(20423, glBlendFuncSeparateEXT, glBlendFuncSeparateEXT, NULL, 633),
    NAME_FUNC_OFFSET(20443, glBlendFuncSeparateEXT, glBlendFuncSeparateEXT, NULL, 633),
    NAME_FUNC_OFFSET(20467, glWindowPos2dMESA, glWindowPos2dMESA, NULL, 650),
    NAME_FUNC_OFFSET(20481, glWindowPos2dMESA, glWindowPos2dMESA, NULL, 650),
    NAME_FUNC_OFFSET(20498, glWindowPos2dvMESA, glWindowPos2dvMESA, NULL, 651),
    NAME_FUNC_OFFSET(20513, glWindowPos2dvMESA, glWindowPos2dvMESA, NULL, 651),
    NAME_FUNC_OFFSET(20531, glWindowPos2fMESA, glWindowPos2fMESA, NULL, 652),
    NAME_FUNC_OFFSET(20545, glWindowPos2fMESA, glWindowPos2fMESA, NULL, 652),
    NAME_FUNC_OFFSET(20562, glWindowPos2fvMESA, glWindowPos2fvMESA, NULL, 653),
    NAME_FUNC_OFFSET(20577, glWindowPos2fvMESA, glWindowPos2fvMESA, NULL, 653),
    NAME_FUNC_OFFSET(20595, glWindowPos2iMESA, glWindowPos2iMESA, NULL, 654),
    NAME_FUNC_OFFSET(20609, glWindowPos2iMESA, glWindowPos2iMESA, NULL, 654),
    NAME_FUNC_OFFSET(20626, glWindowPos2ivMESA, glWindowPos2ivMESA, NULL, 655),
    NAME_FUNC_OFFSET(20641, glWindowPos2ivMESA, glWindowPos2ivMESA, NULL, 655),
    NAME_FUNC_OFFSET(20659, glWindowPos2sMESA, glWindowPos2sMESA, NULL, 656),
    NAME_FUNC_OFFSET(20673, glWindowPos2sMESA, glWindowPos2sMESA, NULL, 656),
    NAME_FUNC_OFFSET(20690, glWindowPos2svMESA, glWindowPos2svMESA, NULL, 657),
    NAME_FUNC_OFFSET(20705, glWindowPos2svMESA, glWindowPos2svMESA, NULL, 657),
    NAME_FUNC_OFFSET(20723, glWindowPos3dMESA, glWindowPos3dMESA, NULL, 658),
    NAME_FUNC_OFFSET(20737, glWindowPos3dMESA, glWindowPos3dMESA, NULL, 658),
    NAME_FUNC_OFFSET(20754, glWindowPos3dvMESA, glWindowPos3dvMESA, NULL, 659),
    NAME_FUNC_OFFSET(20769, glWindowPos3dvMESA, glWindowPos3dvMESA, NULL, 659),
    NAME_FUNC_OFFSET(20787, glWindowPos3fMESA, glWindowPos3fMESA, NULL, 660),
    NAME_FUNC_OFFSET(20801, glWindowPos3fMESA, glWindowPos3fMESA, NULL, 660),
    NAME_FUNC_OFFSET(20818, glWindowPos3fvMESA, glWindowPos3fvMESA, NULL, 661),
    NAME_FUNC_OFFSET(20833, glWindowPos3fvMESA, glWindowPos3fvMESA, NULL, 661),
    NAME_FUNC_OFFSET(20851, glWindowPos3iMESA, glWindowPos3iMESA, NULL, 662),
    NAME_FUNC_OFFSET(20865, glWindowPos3iMESA, glWindowPos3iMESA, NULL, 662),
    NAME_FUNC_OFFSET(20882, glWindowPos3ivMESA, glWindowPos3ivMESA, NULL, 663),
    NAME_FUNC_OFFSET(20897, glWindowPos3ivMESA, glWindowPos3ivMESA, NULL, 663),
    NAME_FUNC_OFFSET(20915, glWindowPos3sMESA, glWindowPos3sMESA, NULL, 664),
    NAME_FUNC_OFFSET(20929, glWindowPos3sMESA, glWindowPos3sMESA, NULL, 664),
    NAME_FUNC_OFFSET(20946, glWindowPos3svMESA, glWindowPos3svMESA, NULL, 665),
    NAME_FUNC_OFFSET(20961, glWindowPos3svMESA, glWindowPos3svMESA, NULL, 665),
    NAME_FUNC_OFFSET(20979, glBindProgramNV, glBindProgramNV, NULL, 684),
    NAME_FUNC_OFFSET(20996, glDeleteProgramsNV, glDeleteProgramsNV, NULL, 685),
    NAME_FUNC_OFFSET(21016, glGenProgramsNV, glGenProgramsNV, NULL, 687),
    NAME_FUNC_OFFSET(21033, glGetVertexAttribPointervNV, glGetVertexAttribPointervNV, NULL, 693),
    NAME_FUNC_OFFSET(21059, glGetVertexAttribPointervNV, glGetVertexAttribPointervNV, NULL, 693),
    NAME_FUNC_OFFSET(21088, glIsProgramNV, glIsProgramNV, NULL, 697),
    NAME_FUNC_OFFSET(21103, glPointParameteriNV, glPointParameteriNV, NULL, 761),
    NAME_FUNC_OFFSET(21121, glPointParameterivNV, glPointParameterivNV, NULL, 762),
    NAME_FUNC_OFFSET(21140, gl_dispatch_stub_765, gl_dispatch_stub_765, NULL, 765),
    NAME_FUNC_OFFSET(21161, gl_dispatch_stub_767, gl_dispatch_stub_767, NULL, 767),
    NAME_FUNC_OFFSET(21177, gl_dispatch_stub_777, gl_dispatch_stub_777, NULL, 777),
    NAME_FUNC_OFFSET(21201, gl_dispatch_stub_777, gl_dispatch_stub_777, NULL, 777),
    NAME_FUNC_OFFSET(21228, glBindFramebufferEXT, glBindFramebufferEXT, NULL, 778),
    NAME_FUNC_OFFSET(21246, glBindRenderbufferEXT, glBindRenderbufferEXT, NULL, 779),
    NAME_FUNC_OFFSET(21265, glCheckFramebufferStatusEXT, glCheckFramebufferStatusEXT, NULL, 780),
    NAME_FUNC_OFFSET(21290, glDeleteFramebuffersEXT, glDeleteFramebuffersEXT, NULL, 781),
    NAME_FUNC_OFFSET(21311, glDeleteRenderbuffersEXT, glDeleteRenderbuffersEXT, NULL, 782),
    NAME_FUNC_OFFSET(21333, glFramebufferRenderbufferEXT, glFramebufferRenderbufferEXT, NULL, 783),
    NAME_FUNC_OFFSET(21359, glFramebufferTexture1DEXT, glFramebufferTexture1DEXT, NULL, 784),
    NAME_FUNC_OFFSET(21382, glFramebufferTexture2DEXT, glFramebufferTexture2DEXT, NULL, 785),
    NAME_FUNC_OFFSET(21405, glFramebufferTexture3DEXT, glFramebufferTexture3DEXT, NULL, 786),
    NAME_FUNC_OFFSET(21428, glGenFramebuffersEXT, glGenFramebuffersEXT, NULL, 787),
    NAME_FUNC_OFFSET(21446, glGenRenderbuffersEXT, glGenRenderbuffersEXT, NULL, 788),
    NAME_FUNC_OFFSET(21465, glGenerateMipmapEXT, glGenerateMipmapEXT, NULL, 789),
    NAME_FUNC_OFFSET(21482, glGetFramebufferAttachmentParameterivEXT, glGetFramebufferAttachmentParameterivEXT, NULL, 790),
    NAME_FUNC_OFFSET(21520, glGetRenderbufferParameterivEXT, glGetRenderbufferParameterivEXT, NULL, 791),
    NAME_FUNC_OFFSET(21549, glIsFramebufferEXT, glIsFramebufferEXT, NULL, 792),
    NAME_FUNC_OFFSET(21565, glIsRenderbufferEXT, glIsRenderbufferEXT, NULL, 793),
    NAME_FUNC_OFFSET(21582, glRenderbufferStorageEXT, glRenderbufferStorageEXT, NULL, 794),
    NAME_FUNC_OFFSET(21604, gl_dispatch_stub_795, gl_dispatch_stub_795, NULL, 795),
    NAME_FUNC_OFFSET(21622, glFramebufferTextureLayerEXT, glFramebufferTextureLayerEXT, NULL, 832),
    NAME_FUNC_OFFSET(21648, glBeginTransformFeedbackEXT, glBeginTransformFeedbackEXT, NULL, 847),
    NAME_FUNC_OFFSET(21673, glBindBufferBaseEXT, glBindBufferBaseEXT, NULL, 848),
    NAME_FUNC_OFFSET(21690, glBindBufferRangeEXT, glBindBufferRangeEXT, NULL, 850),
    NAME_FUNC_OFFSET(21708, glEndTransformFeedbackEXT, glEndTransformFeedbackEXT, NULL, 851),
    NAME_FUNC_OFFSET(21731, glGetTransformFeedbackVaryingEXT, glGetTransformFeedbackVaryingEXT, NULL, 852),
    NAME_FUNC_OFFSET(21761, glTransformFeedbackVaryingsEXT, glTransformFeedbackVaryingsEXT, NULL, 853),
    NAME_FUNC_OFFSET(21789, glProvokingVertexEXT, glProvokingVertexEXT, NULL, 854),
    NAME_FUNC_OFFSET(-1, NULL, NULL, NULL, 0)
};

#undef NAME_FUNC_OFFSET
