/*
 * GLX implementation that uses Apple's OpenGL.framework
 *
 * Copyright (c) 2007-2011 Apple Inc.
 * Copyright (c) 2004 Torrey T. Lyons. All Rights Reserved.
 * Copyright (c) 2002 Greg Parker. All Rights Reserved.
 *
 * Portions of this file are copied from Mesa's xf86glx.c,
 * which contains the following copyright:
 *
 * Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE ABOVE LISTED COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <assert.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <GL/gl.h>

#include "glapi.h"
#include "glapitable.h"
#include "glapidispatch.h"

#include "apple_glx.h"
#include "apple_xgl_api.h"

#ifndef OPENGL_FRAMEWORK_PATH
#define OPENGL_FRAMEWORK_PATH "/System/Library/Frameworks/OpenGL.framework/OpenGL"
#endif

__private_extern__
struct _glapi_table * __ogl_framework_api = NULL;

__private_extern__
struct _glapi_table * __applegl_api = NULL;

__private_extern__
void apple_xgl_init_direct(void) {
    static void *handle;
    const char *opengl_framework_path;

    if(__applegl_api)  {
        _glapi_set_dispatch(__applegl_api);
        return;
    }

    opengl_framework_path = getenv("OPENGL_FRAMEWORK_PATH");
    if (!opengl_framework_path) {
        opengl_framework_path = OPENGL_FRAMEWORK_PATH;
    }

    (void) dlerror();            /*drain dlerror */
    handle = dlopen(opengl_framework_path, RTLD_LOCAL);

    if (!handle) {
        fprintf(stderr, "error: unable to dlopen %s : %s\n",
                opengl_framework_path, dlerror());
        abort();
    }

    __ogl_framework_api = calloc(1, sizeof(struct _glapi_table));
    assert(__ogl_framework_api);

    /* to update:
     * for f in $(grep 'define SET_' ../../../glx/dispatch.h  | cut -f2 -d' ' | cut -f1 -d\( | sort -u); do grep -q $f indirect.c && echo $f ; done | grep -v by_offset | sed 's:SET_\(.*\)$:SET_\1(__ogl_framework_api, dlsym(handle, "gl\1"))\;:'
     */

    SET_Accum(__ogl_framework_api, dlsym(handle, "glAccum"));
    SET_AlphaFunc(__ogl_framework_api, dlsym(handle, "glAlphaFunc"));
    SET_AreTexturesResident(__ogl_framework_api, dlsym(handle, "glAreTexturesResident"));
    SET_ArrayElement(__ogl_framework_api, dlsym(handle, "glArrayElement"));
    SET_Begin(__ogl_framework_api, dlsym(handle, "glBegin"));
    SET_BindTexture(__ogl_framework_api, dlsym(handle, "glBindTexture"));
    SET_Bitmap(__ogl_framework_api, dlsym(handle, "glBitmap"));
    SET_BlendColor(__ogl_framework_api, dlsym(handle, "glBlendColor"));
    SET_BlendEquation(__ogl_framework_api, dlsym(handle, "glBlendEquation"));
    SET_BlendFunc(__ogl_framework_api, dlsym(handle, "glBlendFunc"));
    SET_CallList(__ogl_framework_api, dlsym(handle, "glCallList"));
    SET_CallLists(__ogl_framework_api, dlsym(handle, "glCallLists"));
    SET_Clear(__ogl_framework_api, dlsym(handle, "glClear"));
    SET_ClearAccum(__ogl_framework_api, dlsym(handle, "glClearAccum"));
    SET_ClearColor(__ogl_framework_api, dlsym(handle, "glClearColor"));
    SET_ClearDepth(__ogl_framework_api, dlsym(handle, "glClearDepth"));
    SET_ClearIndex(__ogl_framework_api, dlsym(handle, "glClearIndex"));
    SET_ClearStencil(__ogl_framework_api, dlsym(handle, "glClearStencil"));
    SET_ClipPlane(__ogl_framework_api, dlsym(handle, "glClipPlane"));
    SET_Color3b(__ogl_framework_api, dlsym(handle, "glColor3b"));
    SET_Color3bv(__ogl_framework_api, dlsym(handle, "glColor3bv"));
    SET_Color3d(__ogl_framework_api, dlsym(handle, "glColor3d"));
    SET_Color3dv(__ogl_framework_api, dlsym(handle, "glColor3dv"));
    SET_Color3f(__ogl_framework_api, dlsym(handle, "glColor3f"));
    SET_Color3fv(__ogl_framework_api, dlsym(handle, "glColor3fv"));
    SET_Color3i(__ogl_framework_api, dlsym(handle, "glColor3i"));
    SET_Color3iv(__ogl_framework_api, dlsym(handle, "glColor3iv"));
    SET_Color3s(__ogl_framework_api, dlsym(handle, "glColor3s"));
    SET_Color3sv(__ogl_framework_api, dlsym(handle, "glColor3sv"));
    SET_Color3ub(__ogl_framework_api, dlsym(handle, "glColor3ub"));
    SET_Color3ubv(__ogl_framework_api, dlsym(handle, "glColor3ubv"));
    SET_Color3ui(__ogl_framework_api, dlsym(handle, "glColor3ui"));
    SET_Color3uiv(__ogl_framework_api, dlsym(handle, "glColor3uiv"));
    SET_Color3us(__ogl_framework_api, dlsym(handle, "glColor3us"));
    SET_Color3usv(__ogl_framework_api, dlsym(handle, "glColor3usv"));
    SET_Color4b(__ogl_framework_api, dlsym(handle, "glColor4b"));
    SET_Color4bv(__ogl_framework_api, dlsym(handle, "glColor4bv"));
    SET_Color4d(__ogl_framework_api, dlsym(handle, "glColor4d"));
    SET_Color4dv(__ogl_framework_api, dlsym(handle, "glColor4dv"));
    SET_Color4f(__ogl_framework_api, dlsym(handle, "glColor4f"));
    SET_Color4fv(__ogl_framework_api, dlsym(handle, "glColor4fv"));
    SET_Color4i(__ogl_framework_api, dlsym(handle, "glColor4i"));
    SET_Color4iv(__ogl_framework_api, dlsym(handle, "glColor4iv"));
    SET_Color4s(__ogl_framework_api, dlsym(handle, "glColor4s"));
    SET_Color4sv(__ogl_framework_api, dlsym(handle, "glColor4sv"));
    SET_Color4ub(__ogl_framework_api, dlsym(handle, "glColor4ub"));
    SET_Color4ubv(__ogl_framework_api, dlsym(handle, "glColor4ubv"));
    SET_Color4ui(__ogl_framework_api, dlsym(handle, "glColor4ui"));
    SET_Color4uiv(__ogl_framework_api, dlsym(handle, "glColor4uiv"));
    SET_Color4us(__ogl_framework_api, dlsym(handle, "glColor4us"));
    SET_Color4usv(__ogl_framework_api, dlsym(handle, "glColor4usv"));
    SET_ColorMask(__ogl_framework_api, dlsym(handle, "glColorMask"));
    SET_ColorMaterial(__ogl_framework_api, dlsym(handle, "glColorMaterial"));
    SET_ColorPointer(__ogl_framework_api, dlsym(handle, "glColorPointer"));
    SET_ColorSubTable(__ogl_framework_api, dlsym(handle, "glColorSubTable"));
    SET_ColorTable(__ogl_framework_api, dlsym(handle, "glColorTable"));
    SET_ColorTableParameterfv(__ogl_framework_api, dlsym(handle, "glColorTableParameterfv"));
    SET_ColorTableParameteriv(__ogl_framework_api, dlsym(handle, "glColorTableParameteriv"));
    SET_ConvolutionFilter1D(__ogl_framework_api, dlsym(handle, "glConvolutionFilter1D"));
    SET_ConvolutionFilter2D(__ogl_framework_api, dlsym(handle, "glConvolutionFilter2D"));
    SET_ConvolutionParameterf(__ogl_framework_api, dlsym(handle, "glConvolutionParameterf"));
    SET_ConvolutionParameterfv(__ogl_framework_api, dlsym(handle, "glConvolutionParameterfv"));
    SET_ConvolutionParameteri(__ogl_framework_api, dlsym(handle, "glConvolutionParameteri"));
    SET_ConvolutionParameteriv(__ogl_framework_api, dlsym(handle, "glConvolutionParameteriv"));
    SET_CopyColorSubTable(__ogl_framework_api, dlsym(handle, "glCopyColorSubTable"));
    SET_CopyColorTable(__ogl_framework_api, dlsym(handle, "glCopyColorTable"));
    SET_CopyConvolutionFilter1D(__ogl_framework_api, dlsym(handle, "glCopyConvolutionFilter1D"));
    SET_CopyConvolutionFilter2D(__ogl_framework_api, dlsym(handle, "glCopyConvolutionFilter2D"));
    SET_CopyPixels(__ogl_framework_api, dlsym(handle, "glCopyPixels"));
    SET_CopyTexImage1D(__ogl_framework_api, dlsym(handle, "glCopyTexImage1D"));
    SET_CopyTexImage2D(__ogl_framework_api, dlsym(handle, "glCopyTexImage2D"));
    SET_CopyTexSubImage1D(__ogl_framework_api, dlsym(handle, "glCopyTexSubImage1D"));
    SET_CopyTexSubImage2D(__ogl_framework_api, dlsym(handle, "glCopyTexSubImage2D"));
    SET_CopyTexSubImage3D(__ogl_framework_api, dlsym(handle, "glCopyTexSubImage3D"));
    SET_CullFace(__ogl_framework_api, dlsym(handle, "glCullFace"));
    SET_DeleteLists(__ogl_framework_api, dlsym(handle, "glDeleteLists"));
    SET_DeleteTextures(__ogl_framework_api, dlsym(handle, "glDeleteTextures"));
    SET_DepthFunc(__ogl_framework_api, dlsym(handle, "glDepthFunc"));
    SET_DepthMask(__ogl_framework_api, dlsym(handle, "glDepthMask"));
    SET_DepthRange(__ogl_framework_api, dlsym(handle, "glDepthRange"));
    SET_Disable(__ogl_framework_api, dlsym(handle, "glDisable"));
    SET_DisableClientState(__ogl_framework_api, dlsym(handle, "glDisableClientState"));
    SET_DrawArrays(__ogl_framework_api, dlsym(handle, "glDrawArrays"));
    SET_DrawBuffer(__ogl_framework_api, dlsym(handle, "glDrawBuffer"));
    SET_DrawElements(__ogl_framework_api, dlsym(handle, "glDrawElements"));
    SET_DrawPixels(__ogl_framework_api, dlsym(handle, "glDrawPixels"));
    SET_DrawRangeElements(__ogl_framework_api, dlsym(handle, "glDrawRangeElements"));
    SET_EdgeFlag(__ogl_framework_api, dlsym(handle, "glEdgeFlag"));
    SET_EdgeFlagPointer(__ogl_framework_api, dlsym(handle, "glEdgeFlagPointer"));
    SET_EdgeFlagv(__ogl_framework_api, dlsym(handle, "glEdgeFlagv"));
    SET_Enable(__ogl_framework_api, dlsym(handle, "glEnable"));
    SET_EnableClientState(__ogl_framework_api, dlsym(handle, "glEnableClientState"));
    SET_End(__ogl_framework_api, dlsym(handle, "glEnd"));
    SET_EndList(__ogl_framework_api, dlsym(handle, "glEndList"));
    SET_EvalCoord1d(__ogl_framework_api, dlsym(handle, "glEvalCoord1d"));
    SET_EvalCoord1dv(__ogl_framework_api, dlsym(handle, "glEvalCoord1dv"));
    SET_EvalCoord1f(__ogl_framework_api, dlsym(handle, "glEvalCoord1f"));
    SET_EvalCoord1fv(__ogl_framework_api, dlsym(handle, "glEvalCoord1fv"));
    SET_EvalCoord2d(__ogl_framework_api, dlsym(handle, "glEvalCoord2d"));
    SET_EvalCoord2dv(__ogl_framework_api, dlsym(handle, "glEvalCoord2dv"));
    SET_EvalCoord2f(__ogl_framework_api, dlsym(handle, "glEvalCoord2f"));
    SET_EvalCoord2fv(__ogl_framework_api, dlsym(handle, "glEvalCoord2fv"));
    SET_EvalMesh1(__ogl_framework_api, dlsym(handle, "glEvalMesh1"));
    SET_EvalMesh2(__ogl_framework_api, dlsym(handle, "glEvalMesh2"));
    SET_EvalPoint1(__ogl_framework_api, dlsym(handle, "glEvalPoint1"));
    SET_EvalPoint2(__ogl_framework_api, dlsym(handle, "glEvalPoint2"));
    SET_FeedbackBuffer(__ogl_framework_api, dlsym(handle, "glFeedbackBuffer"));
    SET_Finish(__ogl_framework_api, dlsym(handle, "glFinish"));
    SET_Flush(__ogl_framework_api, dlsym(handle, "glFlush"));
    SET_Fogf(__ogl_framework_api, dlsym(handle, "glFogf"));
    SET_Fogfv(__ogl_framework_api, dlsym(handle, "glFogfv"));
    SET_Fogi(__ogl_framework_api, dlsym(handle, "glFogi"));
    SET_Fogiv(__ogl_framework_api, dlsym(handle, "glFogiv"));
    SET_FrontFace(__ogl_framework_api, dlsym(handle, "glFrontFace"));
    SET_Frustum(__ogl_framework_api, dlsym(handle, "glFrustum"));
    SET_GenLists(__ogl_framework_api, dlsym(handle, "glGenLists"));
    SET_GenTextures(__ogl_framework_api, dlsym(handle, "glGenTextures"));
    SET_GetBooleanv(__ogl_framework_api, dlsym(handle, "glGetBooleanv"));
    SET_GetClipPlane(__ogl_framework_api, dlsym(handle, "glGetClipPlane"));
    SET_GetColorTable(__ogl_framework_api, dlsym(handle, "glGetColorTable"));
    SET_GetColorTableParameterfv(__ogl_framework_api, dlsym(handle, "glGetColorTableParameterfv"));
    SET_GetColorTableParameteriv(__ogl_framework_api, dlsym(handle, "glGetColorTableParameteriv"));
    SET_GetConvolutionFilter(__ogl_framework_api, dlsym(handle, "glGetConvolutionFilter"));
    SET_GetConvolutionParameterfv(__ogl_framework_api, dlsym(handle, "glGetConvolutionParameterfv"));
    SET_GetConvolutionParameteriv(__ogl_framework_api, dlsym(handle, "glGetConvolutionParameteriv"));
    SET_GetDoublev(__ogl_framework_api, dlsym(handle, "glGetDoublev"));
    SET_GetError(__ogl_framework_api, dlsym(handle, "glGetError"));
    SET_GetFloatv(__ogl_framework_api, dlsym(handle, "glGetFloatv"));
    SET_GetHistogram(__ogl_framework_api, dlsym(handle, "glGetHistogram"));
    SET_GetHistogramParameterfv(__ogl_framework_api, dlsym(handle, "glGetHistogramParameterfv"));
    SET_GetHistogramParameteriv(__ogl_framework_api, dlsym(handle, "glGetHistogramParameteriv"));
    SET_GetIntegerv(__ogl_framework_api, dlsym(handle, "glGetIntegerv"));
    SET_GetLightfv(__ogl_framework_api, dlsym(handle, "glGetLightfv"));
    SET_GetLightiv(__ogl_framework_api, dlsym(handle, "glGetLightiv"));
    SET_GetMapdv(__ogl_framework_api, dlsym(handle, "glGetMapdv"));
    SET_GetMapfv(__ogl_framework_api, dlsym(handle, "glGetMapfv"));
    SET_GetMapiv(__ogl_framework_api, dlsym(handle, "glGetMapiv"));
    SET_GetMaterialfv(__ogl_framework_api, dlsym(handle, "glGetMaterialfv"));
    SET_GetMaterialiv(__ogl_framework_api, dlsym(handle, "glGetMaterialiv"));
    SET_GetMinmax(__ogl_framework_api, dlsym(handle, "glGetMinmax"));
    SET_GetMinmaxParameterfv(__ogl_framework_api, dlsym(handle, "glGetMinmaxParameterfv"));
    SET_GetMinmaxParameteriv(__ogl_framework_api, dlsym(handle, "glGetMinmaxParameteriv"));
    SET_GetPixelMapfv(__ogl_framework_api, dlsym(handle, "glGetPixelMapfv"));
    SET_GetPixelMapuiv(__ogl_framework_api, dlsym(handle, "glGetPixelMapuiv"));
    SET_GetPixelMapusv(__ogl_framework_api, dlsym(handle, "glGetPixelMapusv"));
    SET_GetPointerv(__ogl_framework_api, dlsym(handle, "glGetPointerv"));
    SET_GetPolygonStipple(__ogl_framework_api, dlsym(handle, "glGetPolygonStipple"));
    SET_GetSeparableFilter(__ogl_framework_api, dlsym(handle, "glGetSeparableFilter"));
    SET_GetString(__ogl_framework_api, dlsym(handle, "glGetString"));
    SET_GetTexEnvfv(__ogl_framework_api, dlsym(handle, "glGetTexEnvfv"));
    SET_GetTexEnviv(__ogl_framework_api, dlsym(handle, "glGetTexEnviv"));
    SET_GetTexGendv(__ogl_framework_api, dlsym(handle, "glGetTexGendv"));
    SET_GetTexGenfv(__ogl_framework_api, dlsym(handle, "glGetTexGenfv"));
    SET_GetTexGeniv(__ogl_framework_api, dlsym(handle, "glGetTexGeniv"));
    SET_GetTexImage(__ogl_framework_api, dlsym(handle, "glGetTexImage"));
    SET_GetTexLevelParameterfv(__ogl_framework_api, dlsym(handle, "glGetTexLevelParameterfv"));
    SET_GetTexLevelParameteriv(__ogl_framework_api, dlsym(handle, "glGetTexLevelParameteriv"));
    SET_GetTexParameterfv(__ogl_framework_api, dlsym(handle, "glGetTexParameterfv"));
    SET_GetTexParameteriv(__ogl_framework_api, dlsym(handle, "glGetTexParameteriv"));
    SET_Hint(__ogl_framework_api, dlsym(handle, "glHint"));
    SET_Histogram(__ogl_framework_api, dlsym(handle, "glHistogram"));
    SET_IndexMask(__ogl_framework_api, dlsym(handle, "glIndexMask"));
    SET_IndexPointer(__ogl_framework_api, dlsym(handle, "glIndexPointer"));
    SET_Indexd(__ogl_framework_api, dlsym(handle, "glIndexd"));
    SET_Indexdv(__ogl_framework_api, dlsym(handle, "glIndexdv"));
    SET_Indexf(__ogl_framework_api, dlsym(handle, "glIndexf"));
    SET_Indexfv(__ogl_framework_api, dlsym(handle, "glIndexfv"));
    SET_Indexi(__ogl_framework_api, dlsym(handle, "glIndexi"));
    SET_Indexiv(__ogl_framework_api, dlsym(handle, "glIndexiv"));
    SET_Indexs(__ogl_framework_api, dlsym(handle, "glIndexs"));
    SET_Indexsv(__ogl_framework_api, dlsym(handle, "glIndexsv"));
    SET_Indexub(__ogl_framework_api, dlsym(handle, "glIndexub"));
    SET_Indexubv(__ogl_framework_api, dlsym(handle, "glIndexubv"));
    SET_InitNames(__ogl_framework_api, dlsym(handle, "glInitNames"));
    SET_InterleavedArrays(__ogl_framework_api, dlsym(handle, "glInterleavedArrays"));
    SET_IsEnabled(__ogl_framework_api, dlsym(handle, "glIsEnabled"));
    SET_IsList(__ogl_framework_api, dlsym(handle, "glIsList"));
    SET_IsTexture(__ogl_framework_api, dlsym(handle, "glIsTexture"));
    SET_LightModelf(__ogl_framework_api, dlsym(handle, "glLightModelf"));
    SET_LightModelfv(__ogl_framework_api, dlsym(handle, "glLightModelfv"));
    SET_LightModeli(__ogl_framework_api, dlsym(handle, "glLightModeli"));
    SET_LightModeliv(__ogl_framework_api, dlsym(handle, "glLightModeliv"));
    SET_Lightf(__ogl_framework_api, dlsym(handle, "glLightf"));
    SET_Lightfv(__ogl_framework_api, dlsym(handle, "glLightfv"));
    SET_Lighti(__ogl_framework_api, dlsym(handle, "glLighti"));
    SET_Lightiv(__ogl_framework_api, dlsym(handle, "glLightiv"));
    SET_LineStipple(__ogl_framework_api, dlsym(handle, "glLineStipple"));
    SET_LineWidth(__ogl_framework_api, dlsym(handle, "glLineWidth"));
    SET_ListBase(__ogl_framework_api, dlsym(handle, "glListBase"));
    SET_LoadIdentity(__ogl_framework_api, dlsym(handle, "glLoadIdentity"));
    SET_LoadMatrixd(__ogl_framework_api, dlsym(handle, "glLoadMatrixd"));
    SET_LoadMatrixf(__ogl_framework_api, dlsym(handle, "glLoadMatrixf"));
    SET_LoadName(__ogl_framework_api, dlsym(handle, "glLoadName"));
    SET_LogicOp(__ogl_framework_api, dlsym(handle, "glLogicOp"));
    SET_Map1d(__ogl_framework_api, dlsym(handle, "glMap1d"));
    SET_Map1f(__ogl_framework_api, dlsym(handle, "glMap1f"));
    SET_Map2d(__ogl_framework_api, dlsym(handle, "glMap2d"));
    SET_Map2f(__ogl_framework_api, dlsym(handle, "glMap2f"));
    SET_MapGrid1d(__ogl_framework_api, dlsym(handle, "glMapGrid1d"));
    SET_MapGrid1f(__ogl_framework_api, dlsym(handle, "glMapGrid1f"));
    SET_MapGrid2d(__ogl_framework_api, dlsym(handle, "glMapGrid2d"));
    SET_MapGrid2f(__ogl_framework_api, dlsym(handle, "glMapGrid2f"));
    SET_Materialf(__ogl_framework_api, dlsym(handle, "glMaterialf"));
    SET_Materialfv(__ogl_framework_api, dlsym(handle, "glMaterialfv"));
    SET_Materiali(__ogl_framework_api, dlsym(handle, "glMateriali"));
    SET_Materialiv(__ogl_framework_api, dlsym(handle, "glMaterialiv"));
    SET_MatrixMode(__ogl_framework_api, dlsym(handle, "glMatrixMode"));
    SET_Minmax(__ogl_framework_api, dlsym(handle, "glMinmax"));
    SET_MultMatrixd(__ogl_framework_api, dlsym(handle, "glMultMatrixd"));
    SET_MultMatrixf(__ogl_framework_api, dlsym(handle, "glMultMatrixf"));
    SET_NewList(__ogl_framework_api, dlsym(handle, "glNewList"));
    SET_Normal3b(__ogl_framework_api, dlsym(handle, "glNormal3b"));
    SET_Normal3bv(__ogl_framework_api, dlsym(handle, "glNormal3bv"));
    SET_Normal3d(__ogl_framework_api, dlsym(handle, "glNormal3d"));
    SET_Normal3dv(__ogl_framework_api, dlsym(handle, "glNormal3dv"));
    SET_Normal3f(__ogl_framework_api, dlsym(handle, "glNormal3f"));
    SET_Normal3fv(__ogl_framework_api, dlsym(handle, "glNormal3fv"));
    SET_Normal3i(__ogl_framework_api, dlsym(handle, "glNormal3i"));
    SET_Normal3iv(__ogl_framework_api, dlsym(handle, "glNormal3iv"));
    SET_Normal3s(__ogl_framework_api, dlsym(handle, "glNormal3s"));
    SET_Normal3sv(__ogl_framework_api, dlsym(handle, "glNormal3sv"));
    SET_NormalPointer(__ogl_framework_api, dlsym(handle, "glNormalPointer"));
    SET_Ortho(__ogl_framework_api, dlsym(handle, "glOrtho"));
    SET_PassThrough(__ogl_framework_api, dlsym(handle, "glPassThrough"));
    SET_PixelMapfv(__ogl_framework_api, dlsym(handle, "glPixelMapfv"));
    SET_PixelMapuiv(__ogl_framework_api, dlsym(handle, "glPixelMapuiv"));
    SET_PixelMapusv(__ogl_framework_api, dlsym(handle, "glPixelMapusv"));
    SET_PixelStoref(__ogl_framework_api, dlsym(handle, "glPixelStoref"));
    SET_PixelStorei(__ogl_framework_api, dlsym(handle, "glPixelStorei"));
    SET_PixelTransferf(__ogl_framework_api, dlsym(handle, "glPixelTransferf"));
    SET_PixelTransferi(__ogl_framework_api, dlsym(handle, "glPixelTransferi"));
    SET_PixelZoom(__ogl_framework_api, dlsym(handle, "glPixelZoom"));
    SET_PointSize(__ogl_framework_api, dlsym(handle, "glPointSize"));
    SET_PolygonMode(__ogl_framework_api, dlsym(handle, "glPolygonMode"));
    SET_PolygonOffset(__ogl_framework_api, dlsym(handle, "glPolygonOffset"));
    SET_PolygonStipple(__ogl_framework_api, dlsym(handle, "glPolygonStipple"));
    SET_PopAttrib(__ogl_framework_api, dlsym(handle, "glPopAttrib"));
    SET_PopClientAttrib(__ogl_framework_api, dlsym(handle, "glPopClientAttrib"));
    SET_PopMatrix(__ogl_framework_api, dlsym(handle, "glPopMatrix"));
    SET_PopName(__ogl_framework_api, dlsym(handle, "glPopName"));
    SET_PrioritizeTextures(__ogl_framework_api, dlsym(handle, "glPrioritizeTextures"));
    SET_PushAttrib(__ogl_framework_api, dlsym(handle, "glPushAttrib"));
    SET_PushClientAttrib(__ogl_framework_api, dlsym(handle, "glPushClientAttrib"));
    SET_PushMatrix(__ogl_framework_api, dlsym(handle, "glPushMatrix"));
    SET_PushName(__ogl_framework_api, dlsym(handle, "glPushName"));
    SET_RasterPos2d(__ogl_framework_api, dlsym(handle, "glRasterPos2d"));
    SET_RasterPos2dv(__ogl_framework_api, dlsym(handle, "glRasterPos2dv"));
    SET_RasterPos2f(__ogl_framework_api, dlsym(handle, "glRasterPos2f"));
    SET_RasterPos2fv(__ogl_framework_api, dlsym(handle, "glRasterPos2fv"));
    SET_RasterPos2i(__ogl_framework_api, dlsym(handle, "glRasterPos2i"));
    SET_RasterPos2iv(__ogl_framework_api, dlsym(handle, "glRasterPos2iv"));
    SET_RasterPos2s(__ogl_framework_api, dlsym(handle, "glRasterPos2s"));
    SET_RasterPos2sv(__ogl_framework_api, dlsym(handle, "glRasterPos2sv"));
    SET_RasterPos3d(__ogl_framework_api, dlsym(handle, "glRasterPos3d"));
    SET_RasterPos3dv(__ogl_framework_api, dlsym(handle, "glRasterPos3dv"));
    SET_RasterPos3f(__ogl_framework_api, dlsym(handle, "glRasterPos3f"));
    SET_RasterPos3fv(__ogl_framework_api, dlsym(handle, "glRasterPos3fv"));
    SET_RasterPos3i(__ogl_framework_api, dlsym(handle, "glRasterPos3i"));
    SET_RasterPos3iv(__ogl_framework_api, dlsym(handle, "glRasterPos3iv"));
    SET_RasterPos3s(__ogl_framework_api, dlsym(handle, "glRasterPos3s"));
    SET_RasterPos3sv(__ogl_framework_api, dlsym(handle, "glRasterPos3sv"));
    SET_RasterPos4d(__ogl_framework_api, dlsym(handle, "glRasterPos4d"));
    SET_RasterPos4dv(__ogl_framework_api, dlsym(handle, "glRasterPos4dv"));
    SET_RasterPos4f(__ogl_framework_api, dlsym(handle, "glRasterPos4f"));
    SET_RasterPos4fv(__ogl_framework_api, dlsym(handle, "glRasterPos4fv"));
    SET_RasterPos4i(__ogl_framework_api, dlsym(handle, "glRasterPos4i"));
    SET_RasterPos4iv(__ogl_framework_api, dlsym(handle, "glRasterPos4iv"));
    SET_RasterPos4s(__ogl_framework_api, dlsym(handle, "glRasterPos4s"));
    SET_RasterPos4sv(__ogl_framework_api, dlsym(handle, "glRasterPos4sv"));
    SET_ReadBuffer(__ogl_framework_api, dlsym(handle, "glReadBuffer"));
    SET_ReadPixels(__ogl_framework_api, dlsym(handle, "glReadPixels"));
    SET_Rectd(__ogl_framework_api, dlsym(handle, "glRectd"));
    SET_Rectdv(__ogl_framework_api, dlsym(handle, "glRectdv"));
    SET_Rectf(__ogl_framework_api, dlsym(handle, "glRectf"));
    SET_Rectfv(__ogl_framework_api, dlsym(handle, "glRectfv"));
    SET_Recti(__ogl_framework_api, dlsym(handle, "glRecti"));
    SET_Rectiv(__ogl_framework_api, dlsym(handle, "glRectiv"));
    SET_Rects(__ogl_framework_api, dlsym(handle, "glRects"));
    SET_Rectsv(__ogl_framework_api, dlsym(handle, "glRectsv"));
    SET_RenderMode(__ogl_framework_api, dlsym(handle, "glRenderMode"));
    SET_ResetHistogram(__ogl_framework_api, dlsym(handle, "glResetHistogram"));
    SET_ResetMinmax(__ogl_framework_api, dlsym(handle, "glResetMinmax"));
    SET_Rotated(__ogl_framework_api, dlsym(handle, "glRotated"));
    SET_Rotatef(__ogl_framework_api, dlsym(handle, "glRotatef"));
    SET_Scaled(__ogl_framework_api, dlsym(handle, "glScaled"));
    SET_Scalef(__ogl_framework_api, dlsym(handle, "glScalef"));
    SET_Scissor(__ogl_framework_api, dlsym(handle, "glScissor"));
    SET_SelectBuffer(__ogl_framework_api, dlsym(handle, "glSelectBuffer"));
    SET_SeparableFilter2D(__ogl_framework_api, dlsym(handle, "glSeparableFilter2D"));
    SET_ShadeModel(__ogl_framework_api, dlsym(handle, "glShadeModel"));
    SET_StencilFunc(__ogl_framework_api, dlsym(handle, "glStencilFunc"));
    SET_StencilMask(__ogl_framework_api, dlsym(handle, "glStencilMask"));
    SET_StencilOp(__ogl_framework_api, dlsym(handle, "glStencilOp"));
    SET_TexCoord1d(__ogl_framework_api, dlsym(handle, "glTexCoord1d"));
    SET_TexCoord1dv(__ogl_framework_api, dlsym(handle, "glTexCoord1dv"));
    SET_TexCoord1f(__ogl_framework_api, dlsym(handle, "glTexCoord1f"));
    SET_TexCoord1fv(__ogl_framework_api, dlsym(handle, "glTexCoord1fv"));
    SET_TexCoord1i(__ogl_framework_api, dlsym(handle, "glTexCoord1i"));
    SET_TexCoord1iv(__ogl_framework_api, dlsym(handle, "glTexCoord1iv"));
    SET_TexCoord1s(__ogl_framework_api, dlsym(handle, "glTexCoord1s"));
    SET_TexCoord1sv(__ogl_framework_api, dlsym(handle, "glTexCoord1sv"));
    SET_TexCoord2d(__ogl_framework_api, dlsym(handle, "glTexCoord2d"));
    SET_TexCoord2dv(__ogl_framework_api, dlsym(handle, "glTexCoord2dv"));
    SET_TexCoord2f(__ogl_framework_api, dlsym(handle, "glTexCoord2f"));
    SET_TexCoord2fv(__ogl_framework_api, dlsym(handle, "glTexCoord2fv"));
    SET_TexCoord2i(__ogl_framework_api, dlsym(handle, "glTexCoord2i"));
    SET_TexCoord2iv(__ogl_framework_api, dlsym(handle, "glTexCoord2iv"));
    SET_TexCoord2s(__ogl_framework_api, dlsym(handle, "glTexCoord2s"));
    SET_TexCoord2sv(__ogl_framework_api, dlsym(handle, "glTexCoord2sv"));
    SET_TexCoord3d(__ogl_framework_api, dlsym(handle, "glTexCoord3d"));
    SET_TexCoord3dv(__ogl_framework_api, dlsym(handle, "glTexCoord3dv"));
    SET_TexCoord3f(__ogl_framework_api, dlsym(handle, "glTexCoord3f"));
    SET_TexCoord3fv(__ogl_framework_api, dlsym(handle, "glTexCoord3fv"));
    SET_TexCoord3i(__ogl_framework_api, dlsym(handle, "glTexCoord3i"));
    SET_TexCoord3iv(__ogl_framework_api, dlsym(handle, "glTexCoord3iv"));
    SET_TexCoord3s(__ogl_framework_api, dlsym(handle, "glTexCoord3s"));
    SET_TexCoord3sv(__ogl_framework_api, dlsym(handle, "glTexCoord3sv"));
    SET_TexCoord4d(__ogl_framework_api, dlsym(handle, "glTexCoord4d"));
    SET_TexCoord4dv(__ogl_framework_api, dlsym(handle, "glTexCoord4dv"));
    SET_TexCoord4f(__ogl_framework_api, dlsym(handle, "glTexCoord4f"));
    SET_TexCoord4fv(__ogl_framework_api, dlsym(handle, "glTexCoord4fv"));
    SET_TexCoord4i(__ogl_framework_api, dlsym(handle, "glTexCoord4i"));
    SET_TexCoord4iv(__ogl_framework_api, dlsym(handle, "glTexCoord4iv"));
    SET_TexCoord4s(__ogl_framework_api, dlsym(handle, "glTexCoord4s"));
    SET_TexCoord4sv(__ogl_framework_api, dlsym(handle, "glTexCoord4sv"));
    SET_TexCoordPointer(__ogl_framework_api, dlsym(handle, "glTexCoordPointer"));
    SET_TexEnvf(__ogl_framework_api, dlsym(handle, "glTexEnvf"));
    SET_TexEnvfv(__ogl_framework_api, dlsym(handle, "glTexEnvfv"));
    SET_TexEnvi(__ogl_framework_api, dlsym(handle, "glTexEnvi"));
    SET_TexEnviv(__ogl_framework_api, dlsym(handle, "glTexEnviv"));
    SET_TexGend(__ogl_framework_api, dlsym(handle, "glTexGend"));
    SET_TexGendv(__ogl_framework_api, dlsym(handle, "glTexGendv"));
    SET_TexGenf(__ogl_framework_api, dlsym(handle, "glTexGenf"));
    SET_TexGenfv(__ogl_framework_api, dlsym(handle, "glTexGenfv"));
    SET_TexGeni(__ogl_framework_api, dlsym(handle, "glTexGeni"));
    SET_TexGeniv(__ogl_framework_api, dlsym(handle, "glTexGeniv"));
    
    /* Pointer Incompatability:
     * internalformat is a GLenum according to /System/Library/Frameworks/OpenGL.framework/Headers/gl.h
     * extern void glTexImage1D (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
     * extern void glTexImage2D (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
     * extern void glTexImage3D (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
     *
     * and it's a GLint in glx/glapitable.h and according to the man page
     * void ( * TexImage1D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid * pixels);
     * void ( * TexImage2D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid * pixels);
     * void ( * TexImage3D)(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid * pixels);
     *
     * <rdar://problem/6953344> gl.h contains incorrect prototypes for glTexImage[123]D
     */
    
    SET_TexImage1D(__ogl_framework_api, dlsym(handle, "glTexImage1D"));
    SET_TexImage2D(__ogl_framework_api, dlsym(handle, "glTexImage2D"));
    SET_TexImage3D(__ogl_framework_api, dlsym(handle, "glTexImage3D"));
    SET_TexParameterf(__ogl_framework_api, dlsym(handle, "glTexParameterf"));
    SET_TexParameterfv(__ogl_framework_api, dlsym(handle, "glTexParameterfv"));
    SET_TexParameteri(__ogl_framework_api, dlsym(handle, "glTexParameteri"));
    SET_TexParameteriv(__ogl_framework_api, dlsym(handle, "glTexParameteriv"));
    SET_TexSubImage1D(__ogl_framework_api, dlsym(handle, "glTexSubImage1D"));
    SET_TexSubImage2D(__ogl_framework_api, dlsym(handle, "glTexSubImage2D"));
    SET_TexSubImage3D(__ogl_framework_api, dlsym(handle, "glTexSubImage3D"));
    SET_Translated(__ogl_framework_api, dlsym(handle, "glTranslated"));
    SET_Translatef(__ogl_framework_api, dlsym(handle, "glTranslatef"));
    SET_Vertex2d(__ogl_framework_api, dlsym(handle, "glVertex2d"));
    SET_Vertex2dv(__ogl_framework_api, dlsym(handle, "glVertex2dv"));
    SET_Vertex2f(__ogl_framework_api, dlsym(handle, "glVertex2f"));
    SET_Vertex2fv(__ogl_framework_api, dlsym(handle, "glVertex2fv"));
    SET_Vertex2i(__ogl_framework_api, dlsym(handle, "glVertex2i"));
    SET_Vertex2iv(__ogl_framework_api, dlsym(handle, "glVertex2iv"));
    SET_Vertex2s(__ogl_framework_api, dlsym(handle, "glVertex2s"));
    SET_Vertex2sv(__ogl_framework_api, dlsym(handle, "glVertex2sv"));
    SET_Vertex3d(__ogl_framework_api, dlsym(handle, "glVertex3d"));
    SET_Vertex3dv(__ogl_framework_api, dlsym(handle, "glVertex3dv"));
    SET_Vertex3f(__ogl_framework_api, dlsym(handle, "glVertex3f"));
    SET_Vertex3fv(__ogl_framework_api, dlsym(handle, "glVertex3fv"));
    SET_Vertex3i(__ogl_framework_api, dlsym(handle, "glVertex3i"));
    SET_Vertex3iv(__ogl_framework_api, dlsym(handle, "glVertex3iv"));
    SET_Vertex3s(__ogl_framework_api, dlsym(handle, "glVertex3s"));
    SET_Vertex3sv(__ogl_framework_api, dlsym(handle, "glVertex3sv"));
    SET_Vertex4d(__ogl_framework_api, dlsym(handle, "glVertex4d"));
    SET_Vertex4dv(__ogl_framework_api, dlsym(handle, "glVertex4dv"));
    SET_Vertex4f(__ogl_framework_api, dlsym(handle, "glVertex4f"));
    SET_Vertex4fv(__ogl_framework_api, dlsym(handle, "glVertex4fv"));
    SET_Vertex4i(__ogl_framework_api, dlsym(handle, "glVertex4i"));
    SET_Vertex4iv(__ogl_framework_api, dlsym(handle, "glVertex4iv"));
    SET_Vertex4s(__ogl_framework_api, dlsym(handle, "glVertex4s"));
    SET_Vertex4sv(__ogl_framework_api, dlsym(handle, "glVertex4sv"));
    SET_VertexPointer(__ogl_framework_api, dlsym(handle, "glVertexPointer"));
    SET_Viewport(__ogl_framework_api, dlsym(handle, "glViewport"));

    /* GL_VERSION_2_0 */
    SET_AttachShader(__ogl_framework_api, dlsym(handle, "glAttachShader"));
    SET_DeleteShader(__ogl_framework_api, dlsym(handle, "glDeleteShader"));
    SET_DetachShader(__ogl_framework_api, dlsym(handle, "glDetachShader"));
    SET_GetAttachedShaders(__ogl_framework_api, dlsym(handle, "glGetAttachedShaders"));
    SET_GetProgramInfoLog(__ogl_framework_api, dlsym(handle, "glGetProgramInfoLog"));
    SET_GetShaderInfoLog(__ogl_framework_api, dlsym(handle, "glGetShaderInfoLog"));
    SET_GetShaderiv(__ogl_framework_api, dlsym(handle, "glGetShaderiv"));
    SET_IsShader(__ogl_framework_api, dlsym(handle, "glIsShader"));
    SET_StencilFuncSeparate(__ogl_framework_api, dlsym(handle, "glStencilFuncSeparate"));
    SET_StencilMaskSeparate(__ogl_framework_api, dlsym(handle, "glStencilMaskSeparate"));
    SET_StencilOpSeparate(__ogl_framework_api, dlsym(handle, "glStencilOpSeparate"));

    /* GL_VERSION_2_1 */
    SET_UniformMatrix2x3fv(__ogl_framework_api, dlsym(handle, "glUniformMatrix2x3fv"));
    SET_UniformMatrix2x4fv(__ogl_framework_api, dlsym(handle, "glUniformMatrix2x4fv"));
    SET_UniformMatrix3x2fv(__ogl_framework_api, dlsym(handle, "glUniformMatrix3x2fv"));
    SET_UniformMatrix3x4fv(__ogl_framework_api, dlsym(handle, "glUniformMatrix3x4fv"));
    SET_UniformMatrix4x2fv(__ogl_framework_api, dlsym(handle, "glUniformMatrix4x2fv"));
    SET_UniformMatrix4x3fv(__ogl_framework_api, dlsym(handle, "glUniformMatrix4x3fv"));

    /* GL_APPLE_vertex_array_object */
    SET_BindVertexArrayAPPLE(__ogl_framework_api, dlsym(handle, "glBindVertexArrayAPPLE"));
    SET_DeleteVertexArraysAPPLE(__ogl_framework_api, dlsym(handle, "glDeleteVertexArraysAPPLE"));
    SET_GenVertexArraysAPPLE(__ogl_framework_api, dlsym(handle, "glGenVertexArraysAPPLE"));
    SET_IsVertexArrayAPPLE(__ogl_framework_api, dlsym(handle, "glIsVertexArrayAPPLE"));

    /* GL_ARB_draw_buffers */
    SET_DrawBuffersARB(__ogl_framework_api, dlsym(handle, "glDrawBuffersARB"));

    /* GL_ARB_multisample */
    SET_SampleCoverageARB(__ogl_framework_api, dlsym(handle, "glSampleCoverageARB"));

    /* GL_ARB_multitexture */
    SET_ActiveTextureARB(__ogl_framework_api, dlsym(handle, "glActiveTextureARB"));
    SET_ClientActiveTextureARB(__ogl_framework_api, dlsym(handle, "glClientActiveTextureARB"));
    SET_MultiTexCoord1dARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord1dARB"));
    SET_MultiTexCoord1dvARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord1dvARB"));
    SET_MultiTexCoord1fARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord1fARB"));
    SET_MultiTexCoord1fvARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord1fvARB"));
    SET_MultiTexCoord1iARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord1iARB"));
    SET_MultiTexCoord1ivARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord1ivARB"));
    SET_MultiTexCoord1sARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord1sARB"));
    SET_MultiTexCoord1svARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord1svARB"));
    SET_MultiTexCoord2dARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord2dARB"));
    SET_MultiTexCoord2dvARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord2dvARB"));
    SET_MultiTexCoord2fARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord2fARB"));
    SET_MultiTexCoord2fvARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord2fvARB"));
    SET_MultiTexCoord2iARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord2iARB"));
    SET_MultiTexCoord2ivARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord2ivARB"));
    SET_MultiTexCoord2sARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord2sARB"));
    SET_MultiTexCoord2svARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord2svARB"));
    SET_MultiTexCoord3dARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord3dARB"));
    SET_MultiTexCoord3dvARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord3dvARB"));
    SET_MultiTexCoord3fARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord3fARB"));
    SET_MultiTexCoord3fvARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord3fvARB"));
    SET_MultiTexCoord3iARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord3iARB"));
    SET_MultiTexCoord3ivARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord3ivARB"));
    SET_MultiTexCoord3sARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord3sARB"));
    SET_MultiTexCoord3svARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord3svARB"));
    SET_MultiTexCoord4dARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord4dARB"));
    SET_MultiTexCoord4dvARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord4dvARB"));
    SET_MultiTexCoord4fARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord4fARB"));
    SET_MultiTexCoord4fvARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord4fvARB"));
    SET_MultiTexCoord4iARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord4iARB"));
    SET_MultiTexCoord4ivARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord4ivARB"));
    SET_MultiTexCoord4sARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord4sARB"));
    SET_MultiTexCoord4svARB(__ogl_framework_api, dlsym(handle, "glMultiTexCoord4svARB"));

    /* GL_ARB_occlusion_query */
    SET_BeginQueryARB(__ogl_framework_api, dlsym(handle, "glBeginQueryARB"));
    SET_DeleteQueriesARB(__ogl_framework_api, dlsym(handle, "glDeleteQueriesARB"));
    SET_EndQueryARB(__ogl_framework_api, dlsym(handle, "glEndQueryARB"));
    SET_GenQueriesARB(__ogl_framework_api, dlsym(handle, "glGenQueriesARB"));
    SET_GetQueryObjectivARB(__ogl_framework_api, dlsym(handle, "glGetQueryObjectivARB"));
    SET_GetQueryObjectuivARB(__ogl_framework_api, dlsym(handle, "glGetQueryObjectuivARB"));
    SET_GetQueryivARB(__ogl_framework_api, dlsym(handle, "glGetQueryivARB"));
    SET_IsQueryARB(__ogl_framework_api, dlsym(handle, "glIsQueryARB"));

    /* GL_ARB_shader_objects */
    SET_AttachObjectARB(__ogl_framework_api, dlsym(handle, "glAttachObjectARB"));
    SET_CompileShaderARB(__ogl_framework_api, dlsym(handle, "glCompileShaderARB"));
    SET_DeleteObjectARB(__ogl_framework_api, dlsym(handle, "glDeleteObjectARB"));
    SET_GetHandleARB(__ogl_framework_api, dlsym(handle, "glGetHandleARB"));
    SET_DetachObjectARB(__ogl_framework_api, dlsym(handle, "glDetachObjectARB"));
    SET_CreateProgramObjectARB(__ogl_framework_api, dlsym(handle, "glCreateProgramObjectARB"));
    SET_CreateShaderObjectARB(__ogl_framework_api, dlsym(handle, "glCreateShaderObjectARB"));
    SET_GetInfoLogARB(__ogl_framework_api, dlsym(handle, "glGetInfoLogARB"));
    SET_GetActiveUniformARB(__ogl_framework_api, dlsym(handle, "glGetActiveUniformARB"));
    SET_GetAttachedObjectsARB(__ogl_framework_api, dlsym(handle, "glGetAttachedObjectsARB"));
    SET_GetObjectParameterfvARB(__ogl_framework_api, dlsym(handle, "glGetObjectParameterfvARB"));
    SET_GetObjectParameterivARB(__ogl_framework_api, dlsym(handle, "glGetObjectParameterivARB"));
    SET_GetShaderSourceARB(__ogl_framework_api, dlsym(handle, "glGetShaderSourceARB"));
    SET_GetUniformLocationARB(__ogl_framework_api, dlsym(handle, "glGetUniformLocationARB"));
    SET_GetUniformfvARB(__ogl_framework_api, dlsym(handle, "glGetUniformfvARB"));
    SET_GetUniformivARB(__ogl_framework_api, dlsym(handle, "glGetUniformivARB"));
    SET_LinkProgramARB(__ogl_framework_api, dlsym(handle, "glLinkProgramARB"));
    SET_ShaderSourceARB(__ogl_framework_api, dlsym(handle, "glShaderSourceARB"));
    SET_Uniform1fARB(__ogl_framework_api, dlsym(handle, "glUniform1fARB"));
    SET_Uniform1fvARB(__ogl_framework_api, dlsym(handle, "glUniform1fvARB"));
    SET_Uniform1iARB(__ogl_framework_api, dlsym(handle, "glUniform1iARB"));
    SET_Uniform1ivARB(__ogl_framework_api, dlsym(handle, "glUniform1ivARB"));
    SET_Uniform2fARB(__ogl_framework_api, dlsym(handle, "glUniform2fARB"));
    SET_Uniform2fvARB(__ogl_framework_api, dlsym(handle, "glUniform2fvARB"));
    SET_Uniform2iARB(__ogl_framework_api, dlsym(handle, "glUniform2iARB"));
    SET_Uniform2ivARB(__ogl_framework_api, dlsym(handle, "glUniform2ivARB"));
    SET_Uniform3fARB(__ogl_framework_api, dlsym(handle, "glUniform3fARB"));
    SET_Uniform3fvARB(__ogl_framework_api, dlsym(handle, "glUniform3fvARB"));
    SET_Uniform3iARB(__ogl_framework_api, dlsym(handle, "glUniform3iARB"));
    SET_Uniform3ivARB(__ogl_framework_api, dlsym(handle, "glUniform3ivARB"));
    SET_Uniform4fARB(__ogl_framework_api, dlsym(handle, "glUniform4fARB"));
    SET_Uniform4fvARB(__ogl_framework_api, dlsym(handle, "glUniform4fvARB"));
    SET_Uniform4iARB(__ogl_framework_api, dlsym(handle, "glUniform4iARB"));
    SET_Uniform4ivARB(__ogl_framework_api, dlsym(handle, "glUniform4ivARB"));
    SET_UniformMatrix2fvARB(__ogl_framework_api, dlsym(handle, "glUniformMatrix2fvARB"));
    SET_UniformMatrix3fvARB(__ogl_framework_api, dlsym(handle, "glUniformMatrix3fvARB"));
    SET_UniformMatrix4fvARB(__ogl_framework_api, dlsym(handle, "glUniformMatrix4fvARB"));
    SET_UseProgramObjectARB(__ogl_framework_api, dlsym(handle, "glUseProgramObjectARB"));
    SET_ValidateProgramARB(__ogl_framework_api, dlsym(handle, "glValidateProgramARB"));

    /* GL_ARB_texture_compression */
    SET_CompressedTexImage1DARB(__ogl_framework_api, dlsym(handle, "glCompressedTexImage1DARB"));
    SET_CompressedTexImage2DARB(__ogl_framework_api, dlsym(handle, "glCompressedTexImage2DARB"));
    SET_CompressedTexImage3DARB(__ogl_framework_api, dlsym(handle, "glCompressedTexImage3DARB"));
    SET_CompressedTexSubImage1DARB(__ogl_framework_api, dlsym(handle, "glCompressedTexSubImage1DARB"));
    SET_CompressedTexSubImage2DARB(__ogl_framework_api, dlsym(handle, "glCompressedTexSubImage2DARB"));
    SET_CompressedTexSubImage3DARB(__ogl_framework_api, dlsym(handle, "glCompressedTexSubImage3DARB"));
    SET_GetCompressedTexImageARB(__ogl_framework_api, dlsym(handle, "glGetCompressedTexImageARB"));

    /* GL_ARB_transpose_matrix */
    SET_LoadTransposeMatrixdARB(__ogl_framework_api, dlsym(handle, "glLoadTransposeMatrixdARB"));
    SET_LoadTransposeMatrixfARB(__ogl_framework_api, dlsym(handle, "glLoadTransposeMatrixfARB"));
    SET_MultTransposeMatrixdARB(__ogl_framework_api, dlsym(handle, "glMultTransposeMatrixdARB"));
    SET_MultTransposeMatrixfARB(__ogl_framework_api, dlsym(handle, "glMultTransposeMatrixfARB"));

    /* GL_ARB_vertex_buffer_object */
    SET_BindBufferARB(__ogl_framework_api, dlsym(handle, "glBindBufferARB"));
    SET_BufferDataARB(__ogl_framework_api, dlsym(handle, "glBufferDataARB"));
    SET_BufferSubDataARB(__ogl_framework_api, dlsym(handle, "glBufferSubDataARB"));
    SET_DeleteBuffersARB(__ogl_framework_api, dlsym(handle, "glDeleteBuffersARB"));
    SET_GenBuffersARB(__ogl_framework_api, dlsym(handle, "glGenBuffersARB"));
    SET_GetBufferParameterivARB(__ogl_framework_api, dlsym(handle, "glGetBufferParameterivARB"));
    SET_GetBufferPointervARB(__ogl_framework_api, dlsym(handle, "glGetBufferPointervARB"));
    SET_GetBufferSubDataARB(__ogl_framework_api, dlsym(handle, "glGetBufferSubDataARB"));
    SET_IsBufferARB(__ogl_framework_api, dlsym(handle, "glIsBufferARB"));
    SET_MapBufferARB(__ogl_framework_api, dlsym(handle, "glMapBufferARB"));
    SET_UnmapBufferARB(__ogl_framework_api, dlsym(handle, "glUnmapBufferARB"));

    /* GL_ARB_vertex_program */
    SET_DisableVertexAttribArrayARB(__ogl_framework_api, dlsym(handle, "glDisableVertexAttribArrayARB"));
    SET_EnableVertexAttribArrayARB(__ogl_framework_api, dlsym(handle, "glEnableVertexAttribArrayARB"));
    SET_GetProgramEnvParameterdvARB(__ogl_framework_api, dlsym(handle, "glGetProgramEnvParameterdvARB"));
    SET_GetProgramEnvParameterfvARB(__ogl_framework_api, dlsym(handle, "glGetProgramEnvParameterfvARB"));
    SET_GetProgramLocalParameterdvARB(__ogl_framework_api, dlsym(handle, "glGetProgramLocalParameterdvARB"));
    SET_GetProgramLocalParameterfvARB(__ogl_framework_api, dlsym(handle, "glGetProgramLocalParameterfvARB"));
    SET_GetProgramStringARB(__ogl_framework_api, dlsym(handle, "glGetProgramStringARB"));
    SET_GetProgramivARB(__ogl_framework_api, dlsym(handle, "glGetProgramivARB"));
    SET_GetVertexAttribdvARB(__ogl_framework_api, dlsym(handle, "glGetVertexAttribdvARB"));
    SET_GetVertexAttribfvARB(__ogl_framework_api, dlsym(handle, "glGetVertexAttribfvARB"));
    SET_GetVertexAttribivARB(__ogl_framework_api, dlsym(handle, "glGetVertexAttribivARB"));
    SET_ProgramEnvParameter4dARB(__ogl_framework_api, dlsym(handle, "glProgramEnvParameter4dARB"));
    SET_ProgramEnvParameter4dvARB(__ogl_framework_api, dlsym(handle, "glProgramEnvParameter4dvARB"));
    SET_ProgramEnvParameter4fARB(__ogl_framework_api, dlsym(handle, "glProgramEnvParameter4fARB"));
    SET_ProgramEnvParameter4fvARB(__ogl_framework_api, dlsym(handle, "glProgramEnvParameter4fvARB"));
    SET_ProgramLocalParameter4dARB(__ogl_framework_api, dlsym(handle, "glProgramLocalParameter4dARB"));
    SET_ProgramLocalParameter4dvARB(__ogl_framework_api, dlsym(handle, "glProgramLocalParameter4dvARB"));
    SET_ProgramLocalParameter4fARB(__ogl_framework_api, dlsym(handle, "glProgramLocalParameter4fARB"));
    SET_ProgramLocalParameter4fvARB(__ogl_framework_api, dlsym(handle, "glProgramLocalParameter4fvARB"));
    SET_ProgramStringARB(__ogl_framework_api, dlsym(handle, "glProgramStringARB"));
    SET_VertexAttrib1dARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib1dARB"));
    SET_VertexAttrib1dvARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib1dvARB"));
    SET_VertexAttrib1fARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib1fARB"));
    SET_VertexAttrib1fvARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib1fvARB"));
    SET_VertexAttrib1sARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib1sARB"));
    SET_VertexAttrib1svARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib1svARB"));
    SET_VertexAttrib2dARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib2dARB"));
    SET_VertexAttrib2dvARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib2dvARB"));
    SET_VertexAttrib2fARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib2fARB"));
    SET_VertexAttrib2fvARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib2fvARB"));
    SET_VertexAttrib2sARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib2sARB"));
    SET_VertexAttrib2svARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib2svARB"));
    SET_VertexAttrib3dARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib3dARB"));
    SET_VertexAttrib3dvARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib3dvARB"));
    SET_VertexAttrib3fARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib3fARB"));
    SET_VertexAttrib3fvARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib3fvARB"));
    SET_VertexAttrib3sARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib3sARB"));
    SET_VertexAttrib3svARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib3svARB"));
    SET_VertexAttrib4NbvARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib4NbvARB"));
    SET_VertexAttrib4NivARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib4NivARB"));
    SET_VertexAttrib4NsvARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib4NsvARB"));
    SET_VertexAttrib4NubARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib4NubARB"));
    SET_VertexAttrib4NubvARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib4NubvARB"));
    SET_VertexAttrib4NuivARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib4NuivARB"));
    SET_VertexAttrib4NusvARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib4NusvARB"));
    SET_VertexAttrib4bvARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib4bvARB"));
    SET_VertexAttrib4dARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib4dARB"));
    SET_VertexAttrib4dvARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib4dvARB"));
    SET_VertexAttrib4fARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib4fARB"));
    SET_VertexAttrib4fvARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib4fvARB"));
    SET_VertexAttrib4ivARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib4ivARB"));
    SET_VertexAttrib4sARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib4sARB"));
    SET_VertexAttrib4svARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib4svARB"));
    SET_VertexAttrib4ubvARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib4ubvARB"));
    SET_VertexAttrib4uivARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib4uivARB"));
    SET_VertexAttrib4usvARB(__ogl_framework_api, dlsym(handle, "glVertexAttrib4usvARB"));
    SET_VertexAttribPointerARB(__ogl_framework_api, dlsym(handle, "glVertexAttribPointerARB"));

    /* GL_ARB_vertex_shader */
    SET_BindAttribLocationARB(__ogl_framework_api, dlsym(handle, "glBindAttribLocationARB"));
    SET_GetActiveAttribARB(__ogl_framework_api, dlsym(handle, "glGetActiveAttribARB"));
    SET_GetAttribLocationARB(__ogl_framework_api, dlsym(handle, "glGetAttribLocationARB"));

    /* GL_ARB_window_pos */
    SET_WindowPos2dMESA(__ogl_framework_api, dlsym(handle, "glWindowPos2dARB"));
    SET_WindowPos2dvMESA(__ogl_framework_api, dlsym(handle, "glWindowPos2dvARB"));
    SET_WindowPos2fMESA(__ogl_framework_api, dlsym(handle, "glWindowPos2fARB"));
    SET_WindowPos2fvMESA(__ogl_framework_api, dlsym(handle, "glWindowPos2fvARB"));
    SET_WindowPos2iMESA(__ogl_framework_api, dlsym(handle, "glWindowPos2iARB"));
    SET_WindowPos2ivMESA(__ogl_framework_api, dlsym(handle, "glWindowPos2ivARB"));
    SET_WindowPos2sMESA(__ogl_framework_api, dlsym(handle, "glWindowPos2sARB"));
    SET_WindowPos2svMESA(__ogl_framework_api, dlsym(handle, "glWindowPos2svARB"));
    SET_WindowPos3dMESA(__ogl_framework_api, dlsym(handle, "glWindowPos3dARB"));
    SET_WindowPos3dvMESA(__ogl_framework_api, dlsym(handle, "glWindowPos3dvARB"));
    SET_WindowPos3fMESA(__ogl_framework_api, dlsym(handle, "glWindowPos3fARB"));
    SET_WindowPos3fvMESA(__ogl_framework_api, dlsym(handle, "glWindowPos3fvARB"));
    SET_WindowPos3iMESA(__ogl_framework_api, dlsym(handle, "glWindowPos3iARB"));
    SET_WindowPos3ivMESA(__ogl_framework_api, dlsym(handle, "glWindowPos3ivARB"));
    SET_WindowPos3sMESA(__ogl_framework_api, dlsym(handle, "glWindowPos3sARB"));
    SET_WindowPos3svMESA(__ogl_framework_api, dlsym(handle, "glWindowPos3svARB"));

    /* GL_ATI_fragment_shader / GL_EXT_fragment_shader */
    if(dlsym(handle, "glAlphaFragmentOp1ATI")) {
        /* GL_ATI_fragment_shader */
        SET_AlphaFragmentOp1ATI(__ogl_framework_api, dlsym(handle, "glAlphaFragmentOp1ATI"));
        SET_AlphaFragmentOp2ATI(__ogl_framework_api, dlsym(handle, "glAlphaFragmentOp2ATI"));
        SET_AlphaFragmentOp3ATI(__ogl_framework_api, dlsym(handle, "glAlphaFragmentOp3ATI"));
        SET_BeginFragmentShaderATI(__ogl_framework_api, dlsym(handle, "glBeginFragmentShaderATI"));
        SET_BindFragmentShaderATI(__ogl_framework_api, dlsym(handle, "glBindFragmentShaderATI"));
        SET_ColorFragmentOp1ATI(__ogl_framework_api, dlsym(handle, "glColorFragmentOp1ATI"));
        SET_ColorFragmentOp2ATI(__ogl_framework_api, dlsym(handle, "glColorFragmentOp2ATI"));
        SET_ColorFragmentOp3ATI(__ogl_framework_api, dlsym(handle, "glColorFragmentOp3ATI"));
        SET_DeleteFragmentShaderATI(__ogl_framework_api, dlsym(handle, "glDeleteFragmentShaderATI"));
        SET_EndFragmentShaderATI(__ogl_framework_api, dlsym(handle, "glEndFragmentShaderATI"));
        SET_GenFragmentShadersATI(__ogl_framework_api, dlsym(handle, "glGenFragmentShadersATI"));
        SET_PassTexCoordATI(__ogl_framework_api, dlsym(handle, "glPassTexCoordATI"));
        SET_SampleMapATI(__ogl_framework_api, dlsym(handle, "glSampleMapATI"));
        SET_SetFragmentShaderConstantATI(__ogl_framework_api, dlsym(handle, "glSetFragmentShaderConstantATI"));
    } else {
        /* GL_EXT_fragment_shader */
        SET_AlphaFragmentOp1ATI(__ogl_framework_api, dlsym(handle, "glAlphaFragmentOp1EXT"));
        SET_AlphaFragmentOp2ATI(__ogl_framework_api, dlsym(handle, "glAlphaFragmentOp2EXT"));
        SET_AlphaFragmentOp3ATI(__ogl_framework_api, dlsym(handle, "glAlphaFragmentOp3EXT"));
        SET_BeginFragmentShaderATI(__ogl_framework_api, dlsym(handle, "glBeginFragmentShaderEXT"));
        SET_BindFragmentShaderATI(__ogl_framework_api, dlsym(handle, "glBindFragmentShaderEXT"));
        SET_ColorFragmentOp1ATI(__ogl_framework_api, dlsym(handle, "glColorFragmentOp1EXT"));
        SET_ColorFragmentOp2ATI(__ogl_framework_api, dlsym(handle, "glColorFragmentOp2EXT"));
        SET_ColorFragmentOp3ATI(__ogl_framework_api, dlsym(handle, "glColorFragmentOp3EXT"));
        SET_DeleteFragmentShaderATI(__ogl_framework_api, dlsym(handle, "glDeleteFragmentShaderEXT"));
        SET_EndFragmentShaderATI(__ogl_framework_api, dlsym(handle, "glEndFragmentShaderEXT"));
        SET_GenFragmentShadersATI(__ogl_framework_api, dlsym(handle, "glGenFragmentShadersEXT"));
        SET_PassTexCoordATI(__ogl_framework_api, dlsym(handle, "glPassTexCoordEXT"));
        SET_SampleMapATI(__ogl_framework_api, dlsym(handle, "glSampleMapEXT"));
        SET_SetFragmentShaderConstantATI(__ogl_framework_api, dlsym(handle, "glSetFragmentShaderConstantEXT"));
    }

    /* GL_ATI_separate_stencil */
    SET_StencilFuncSeparateATI(__ogl_framework_api, dlsym(handle, "glStencilFuncSeparateATI"));

    /* GL_EXT_blend_equation_separate */
    SET_BlendEquationSeparateEXT(__ogl_framework_api, dlsym(handle, "glBlendEquationSeparateEXT"));

    /* GL_EXT_blend_func_separate */
    SET_BlendFuncSeparateEXT(__ogl_framework_api, dlsym(handle, "glBlendFuncSeparateEXT"));

    /* GL_EXT_depth_bounds_test */
    SET_DepthBoundsEXT(__ogl_framework_api, dlsym(handle, "glDepthBoundsEXT"));

    /* GL_EXT_compiled_vertex_array */
    SET_LockArraysEXT(__ogl_framework_api, dlsym(handle, "glLockArraysEXT"));
    SET_UnlockArraysEXT(__ogl_framework_api, dlsym(handle, "glUnlockArraysEXT"));

    /* GL_EXT_cull_vertex */
//    SET_CullParameterdvEXT(__ogl_framework_api, dlsym(handle, "glCullParameterdvEXT"));
//    SET_CullParameterfvEXT(__ogl_framework_api, dlsym(handle, "glCullParameterfvEXT"));

    /* GL_EXT_fog_coord */
    SET_FogCoordPointerEXT(__ogl_framework_api, dlsym(handle, "glFogCoordPointerEXT"));
    SET_FogCoorddEXT(__ogl_framework_api, dlsym(handle, "glFogCoorddEXT"));
    SET_FogCoorddvEXT(__ogl_framework_api, dlsym(handle, "glFogCoorddvEXT"));
    SET_FogCoordfEXT(__ogl_framework_api, dlsym(handle, "glFogCoordfEXT"));
    SET_FogCoordfvEXT(__ogl_framework_api, dlsym(handle, "glFogCoordfvEXT"));

    /* GL_EXT_framebuffer_blit */
    SET_BlitFramebufferEXT(__ogl_framework_api, dlsym(handle, "glBlitFramebufferEXT"));

    /* GL_EXT_framebuffer_object */
    SET_BindFramebufferEXT(__ogl_framework_api, dlsym(handle, "glBindFramebufferEXT"));
    SET_BindRenderbufferEXT(__ogl_framework_api, dlsym(handle, "glBindRenderbufferEXT"));
    SET_CheckFramebufferStatusEXT(__ogl_framework_api, dlsym(handle, "glCheckFramebufferStatusEXT"));
    SET_DeleteFramebuffersEXT(__ogl_framework_api, dlsym(handle, "glDeleteFramebuffersEXT"));
    SET_DeleteRenderbuffersEXT(__ogl_framework_api, dlsym(handle, "glDeleteRenderbuffersEXT"));
    SET_FramebufferRenderbufferEXT(__ogl_framework_api, dlsym(handle, "glFramebufferRenderbufferEXT"));
    SET_FramebufferTexture1DEXT(__ogl_framework_api, dlsym(handle, "glFramebufferTexture1DEXT"));
    SET_FramebufferTexture2DEXT(__ogl_framework_api, dlsym(handle, "glFramebufferTexture2DEXT"));
    SET_FramebufferTexture3DEXT(__ogl_framework_api, dlsym(handle, "glFramebufferTexture3DEXT"));
    SET_GenerateMipmapEXT(__ogl_framework_api, dlsym(handle, "glGenerateMipmapEXT"));
    SET_GenFramebuffersEXT(__ogl_framework_api, dlsym(handle, "glGenFramebuffersEXT"));
    SET_GenRenderbuffersEXT(__ogl_framework_api, dlsym(handle, "glGenRenderbuffersEXT"));
    SET_GetFramebufferAttachmentParameterivEXT(__ogl_framework_api, dlsym(handle, "glGetFramebufferAttachmentParameterivEXT"));
    SET_GetRenderbufferParameterivEXT(__ogl_framework_api, dlsym(handle, "glGetRenderbufferParameterivEXT"));
    SET_IsFramebufferEXT(__ogl_framework_api, dlsym(handle, "glIsFramebufferEXT"));
    SET_IsRenderbufferEXT(__ogl_framework_api, dlsym(handle, "glIsRenderbufferEXT"));
    SET_RenderbufferStorageEXT(__ogl_framework_api, dlsym(handle, "glRenderbufferStorageEXT"));

    /* GL_EXT_gpu_program_parameters */
    SET_ProgramEnvParameters4fvEXT(__ogl_framework_api, dlsym(handle, "glProgramEnvParameters4fvEXT"));
    SET_ProgramLocalParameters4fvEXT(__ogl_framework_api, dlsym(handle, "glProgramLocalParameters4fvEXT"));

    /* Pointer Incompatability:
     * This warning can be safely ignored.  OpenGL.framework adds const to the
     * two pointers.
     *
     * extern void glMultiDrawArraysEXT (GLenum, const GLint *, const GLsizei *, GLsizei);
     *
     * void ( * MultiDrawArraysEXT)(GLenum mode, GLint * first, GLsizei * count, GLsizei primcount);
     */

    /* GL_EXT_multi_draw_arrays */
    SET_MultiDrawArraysEXT(__ogl_framework_api, (void *)dlsym(handle, "glMultiDrawArraysEXT"));
    SET_MultiDrawElementsEXT(__ogl_framework_api, dlsym(handle, "glMultiDrawElementsEXT"));

    /* GL_EXT_point_parameters / GL_ARB_point_parameters */
    if(dlsym(handle, "glPointParameterfEXT")) {
        /* GL_EXT_point_parameters */
        SET_PointParameterfEXT(__ogl_framework_api, dlsym(handle, "glPointParameterfEXT"));
        SET_PointParameterfvEXT(__ogl_framework_api, dlsym(handle, "glPointParameterfvEXT"));
    } else {
        /* GL_ARB_point_parameters */
        SET_PointParameterfEXT(__ogl_framework_api, dlsym(handle, "glPointParameterfARB"));
        SET_PointParameterfvEXT(__ogl_framework_api, dlsym(handle, "glPointParameterfvARB"));
    }

    /* GL_EXT_polygon_offset */
    SET_PolygonOffsetEXT(__ogl_framework_api, dlsym(handle, "glPolygonOffsetEXT"));

    /* GL_EXT_secondary_color */
    SET_SecondaryColor3bEXT(__ogl_framework_api, dlsym(handle, "glSecondaryColor3bEXT"));
    SET_SecondaryColor3bvEXT(__ogl_framework_api, dlsym(handle, "glSecondaryColor3bvEXT"));
    SET_SecondaryColor3dEXT(__ogl_framework_api, dlsym(handle, "glSecondaryColor3dEXT"));
    SET_SecondaryColor3dvEXT(__ogl_framework_api, dlsym(handle, "glSecondaryColor3dvEXT"));
    SET_SecondaryColor3fEXT(__ogl_framework_api, dlsym(handle, "glSecondaryColor3fEXT"));
    SET_SecondaryColor3fvEXT(__ogl_framework_api, dlsym(handle, "glSecondaryColor3fvEXT"));
    SET_SecondaryColor3iEXT(__ogl_framework_api, dlsym(handle, "glSecondaryColor3iEXT"));
    SET_SecondaryColor3ivEXT(__ogl_framework_api, dlsym(handle, "glSecondaryColor3ivEXT"));
    SET_SecondaryColor3sEXT(__ogl_framework_api, dlsym(handle, "glSecondaryColor3sEXT"));
    SET_SecondaryColor3svEXT(__ogl_framework_api, dlsym(handle, "glSecondaryColor3svEXT"));
    SET_SecondaryColor3ubEXT(__ogl_framework_api, dlsym(handle, "glSecondaryColor3ubEXT"));
    SET_SecondaryColor3ubvEXT(__ogl_framework_api, dlsym(handle, "glSecondaryColor3ubvEXT"));
    SET_SecondaryColor3uiEXT(__ogl_framework_api, dlsym(handle, "glSecondaryColor3uiEXT"));
    SET_SecondaryColor3uivEXT(__ogl_framework_api, dlsym(handle, "glSecondaryColor3uivEXT"));
    SET_SecondaryColor3usEXT(__ogl_framework_api, dlsym(handle, "glSecondaryColor3usEXT"));
    SET_SecondaryColor3usvEXT(__ogl_framework_api, dlsym(handle, "glSecondaryColor3usvEXT"));
    SET_SecondaryColorPointerEXT(__ogl_framework_api, dlsym(handle, "glSecondaryColorPointerEXT"));

    /* GL_EXT_stencil_two_side */
    SET_ActiveStencilFaceEXT(__ogl_framework_api, dlsym(handle, "glActiveStencilFaceEXT"));

    /* GL_EXT_timer_query */
    SET_GetQueryObjecti64vEXT(__ogl_framework_api, dlsym(handle, "glGetQueryObjecti64vEXT"));
    SET_GetQueryObjectui64vEXT(__ogl_framework_api, dlsym(handle, "glGetQueryObjectui64vEXT"));

    /* GL_EXT_vertex_array */
    SET_ColorPointerEXT(__ogl_framework_api, dlsym(handle, "glColorPointerEXT"));
    SET_EdgeFlagPointerEXT(__ogl_framework_api, dlsym(handle, "glEdgeFlagPointerEXT"));
    SET_IndexPointerEXT(__ogl_framework_api, dlsym(handle, "glIndexPointerEXT"));
    SET_NormalPointerEXT(__ogl_framework_api, dlsym(handle, "glNormalPointerEXT"));
    SET_TexCoordPointerEXT(__ogl_framework_api, dlsym(handle, "glTexCoordPointerEXT"));
    SET_VertexPointerEXT(__ogl_framework_api, dlsym(handle, "glVertexPointerEXT"));

    /* GL_IBM_multimode_draw_arrays */
    SET_MultiModeDrawArraysIBM(__ogl_framework_api, dlsym(handle, "glMultiModeDrawArraysIBM"));
    SET_MultiModeDrawElementsIBM(__ogl_framework_api, dlsym(handle, "glMultiModeDrawElementsIBM"));

    /* GL_MESA_resize_buffers */
    SET_ResizeBuffersMESA(__ogl_framework_api, dlsym(handle, "glResizeBuffersMESA"));

    /* GL_MESA_window_pos */
    SET_WindowPos4dMESA(__ogl_framework_api, dlsym(handle, "glWindowPos4dMESA"));
    SET_WindowPos4dvMESA(__ogl_framework_api, dlsym(handle, "glWindowPos4dvMESA"));
    SET_WindowPos4fMESA(__ogl_framework_api, dlsym(handle, "glWindowPos4fMESA"));
    SET_WindowPos4fvMESA(__ogl_framework_api, dlsym(handle, "glWindowPos4fvMESA"));
    SET_WindowPos4iMESA(__ogl_framework_api, dlsym(handle, "glWindowPos4iMESA"));
    SET_WindowPos4ivMESA(__ogl_framework_api, dlsym(handle, "glWindowPos4ivMESA"));
    SET_WindowPos4sMESA(__ogl_framework_api, dlsym(handle, "glWindowPos4sMESA"));
    SET_WindowPos4svMESA(__ogl_framework_api, dlsym(handle, "glWindowPos4svMESA"));

    /* GL_NV_fence */
    SET_DeleteFencesNV(__ogl_framework_api, dlsym(handle, "glDeleteFencesNV"));
    SET_FinishFenceNV(__ogl_framework_api, dlsym(handle, "glFinishFenceNV"));
    SET_GenFencesNV(__ogl_framework_api, dlsym(handle, "glGenFencesNV"));
    SET_GetFenceivNV(__ogl_framework_api, dlsym(handle, "glGetFenceivNV"));
    SET_IsFenceNV(__ogl_framework_api, dlsym(handle, "glIsFenceNV"));
    SET_SetFenceNV(__ogl_framework_api, dlsym(handle, "glSetFenceNV"));
    SET_TestFenceNV(__ogl_framework_api, dlsym(handle, "glTestFenceNV"));

    /* GL_NV_fragment_program */
    SET_GetProgramNamedParameterdvNV(__ogl_framework_api, dlsym(handle, "glGetProgramNamedParameterdvNV"));
    SET_GetProgramNamedParameterfvNV(__ogl_framework_api, dlsym(handle, "glGetProgramNamedParameterfvNV"));
    SET_ProgramNamedParameter4dNV(__ogl_framework_api, dlsym(handle, "glProgramNamedParameter4dNV"));
    SET_ProgramNamedParameter4dvNV(__ogl_framework_api, dlsym(handle, "glProgramNamedParameter4dvNV"));
    SET_ProgramNamedParameter4fNV(__ogl_framework_api, dlsym(handle, "glProgramNamedParameter4fNV"));
    SET_ProgramNamedParameter4fvNV(__ogl_framework_api, dlsym(handle, "glProgramNamedParameter4fvNV"));

    /* GL_NV_geometry_program4 */
    SET_FramebufferTextureLayerEXT(__ogl_framework_api, dlsym(handle, "glFramebufferTextureLayerEXT"));

    /* GL_NV_point_sprite */
    SET_PointParameteriNV(__ogl_framework_api, dlsym(handle, "glPointParameteriNV"));
    SET_PointParameterivNV(__ogl_framework_api, dlsym(handle, "glPointParameterivNV"));

    /* GL_NV_register_combiners */
    SET_CombinerInputNV(__ogl_framework_api, dlsym(handle, "glCombinerInputNV"));
    SET_CombinerOutputNV(__ogl_framework_api, dlsym(handle, "glCombinerOutputNV"));
    SET_CombinerParameterfNV(__ogl_framework_api, dlsym(handle, "glCombinerParameterfNV"));
    SET_CombinerParameterfvNV(__ogl_framework_api, dlsym(handle, "glCombinerParameterfvNV"));
    SET_CombinerParameteriNV(__ogl_framework_api, dlsym(handle, "glCombinerParameteriNV"));
    SET_CombinerParameterivNV(__ogl_framework_api, dlsym(handle, "glCombinerParameterivNV"));
    SET_FinalCombinerInputNV(__ogl_framework_api, dlsym(handle, "glFinalCombinerInputNV"));
    SET_GetCombinerInputParameterfvNV(__ogl_framework_api, dlsym(handle, "glGetCombinerInputParameterfvNV"));
    SET_GetCombinerInputParameterivNV(__ogl_framework_api, dlsym(handle, "glGetCombinerInputParameterivNV"));
    SET_GetCombinerOutputParameterfvNV(__ogl_framework_api, dlsym(handle, "glGetCombinerOutputParameterfvNV"));
    SET_GetCombinerOutputParameterivNV(__ogl_framework_api, dlsym(handle, "glGetCombinerOutputParameterivNV"));
    SET_GetFinalCombinerInputParameterfvNV(__ogl_framework_api, dlsym(handle, "glGetFinalCombinerInputParameterfvNV"));
    SET_GetFinalCombinerInputParameterivNV(__ogl_framework_api, dlsym(handle, "glGetFinalCombinerInputParameterivNV"));

    /* GL_NV_vertex_array_range */
    SET_FlushVertexArrayRangeNV(__ogl_framework_api, dlsym(handle, "glFlushVertexArrayRangeNV"));
    SET_VertexArrayRangeNV(__ogl_framework_api, dlsym(handle, "glVertexArrayRangeNV"));

    /* GL_NV_vertex_program */
    SET_AreProgramsResidentNV(__ogl_framework_api, dlsym(handle, "glAreProgramsResidentNV"));
    SET_BindProgramNV(__ogl_framework_api, dlsym(handle, "glBindProgramNV"));
    SET_DeleteProgramsNV(__ogl_framework_api, dlsym(handle, "glDeleteProgramsNV"));
    SET_ExecuteProgramNV(__ogl_framework_api, dlsym(handle, "glExecuteProgramNV"));
    SET_GenProgramsNV(__ogl_framework_api, dlsym(handle, "glGenProgramsNV"));
    SET_GetProgramParameterdvNV(__ogl_framework_api, dlsym(handle, "glGetProgramParameterdvNV"));
    SET_GetProgramParameterfvNV(__ogl_framework_api, dlsym(handle, "glGetProgramParameterfvNV"));
    SET_GetProgramStringNV(__ogl_framework_api, dlsym(handle, "glGetProgramStringNV"));
    SET_GetProgramivNV(__ogl_framework_api, dlsym(handle, "glGetProgramivNV"));
    SET_GetTrackMatrixivNV(__ogl_framework_api, dlsym(handle, "glGetTrackMatrixivNV"));
    SET_GetVertexAttribPointervNV(__ogl_framework_api, dlsym(handle, "glGetVertexAttribPointervNV"));
    SET_GetVertexAttribdvNV(__ogl_framework_api, dlsym(handle, "glGetVertexAttribdvNV"));
    SET_GetVertexAttribfvNV(__ogl_framework_api, dlsym(handle, "glGetVertexAttribfvNV"));
    SET_GetVertexAttribivNV(__ogl_framework_api, dlsym(handle, "glGetVertexAttribivNV"));
    SET_IsProgramNV(__ogl_framework_api, dlsym(handle, "glIsProgramNV"));
    SET_LoadProgramNV(__ogl_framework_api, dlsym(handle, "glLoadProgramNV"));
    SET_ProgramParameters4dvNV(__ogl_framework_api, dlsym(handle, "glProgramParameters4dvNV"));
    SET_ProgramParameters4fvNV(__ogl_framework_api, dlsym(handle, "glProgramParameters4fvNV"));
    SET_RequestResidentProgramsNV(__ogl_framework_api, dlsym(handle, "glRequestResidentProgramsNV"));
    SET_TrackMatrixNV(__ogl_framework_api, dlsym(handle, "glTrackMatrixNV"));
    SET_VertexAttrib1dNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib1dNV"));
    SET_VertexAttrib1dvNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib1dvNV"));
    SET_VertexAttrib1fNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib1fNV"));
    SET_VertexAttrib1fvNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib1fvNV"));
    SET_VertexAttrib1sNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib1sNV"));
    SET_VertexAttrib1svNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib1svNV"));
    SET_VertexAttrib2dNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib2dNV"));
    SET_VertexAttrib2dvNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib2dvNV"));
    SET_VertexAttrib2fNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib2fNV"));
    SET_VertexAttrib2fvNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib2fvNV"));
    SET_VertexAttrib2sNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib2sNV"));
    SET_VertexAttrib2svNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib2svNV"));
    SET_VertexAttrib3dNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib3dNV"));
    SET_VertexAttrib3dvNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib3dvNV"));
    SET_VertexAttrib3fNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib3fNV"));
    SET_VertexAttrib3fvNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib3fvNV"));
    SET_VertexAttrib3sNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib3sNV"));
    SET_VertexAttrib3svNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib3svNV"));
    SET_VertexAttrib4dNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib4dNV"));
    SET_VertexAttrib4dvNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib4dvNV"));
    SET_VertexAttrib4fNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib4fNV"));
    SET_VertexAttrib4fvNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib4fvNV"));
    SET_VertexAttrib4sNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib4sNV"));
    SET_VertexAttrib4svNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib4svNV"));
    SET_VertexAttrib4ubNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib4ubNV"));
    SET_VertexAttrib4ubvNV(__ogl_framework_api, dlsym(handle, "glVertexAttrib4ubvNV"));
    SET_VertexAttribPointerNV(__ogl_framework_api, dlsym(handle, "glVertexAttribPointerNV"));
    SET_VertexAttribs1dvNV(__ogl_framework_api, dlsym(handle, "glVertexAttribs1dvNV"));
    SET_VertexAttribs1fvNV(__ogl_framework_api, dlsym(handle, "glVertexAttribs1fvNV"));
    SET_VertexAttribs1svNV(__ogl_framework_api, dlsym(handle, "glVertexAttribs1svNV"));
    SET_VertexAttribs2dvNV(__ogl_framework_api, dlsym(handle, "glVertexAttribs2dvNV"));
    SET_VertexAttribs2fvNV(__ogl_framework_api, dlsym(handle, "glVertexAttribs2fvNV"));
    SET_VertexAttribs2svNV(__ogl_framework_api, dlsym(handle, "glVertexAttribs2svNV"));
    SET_VertexAttribs3dvNV(__ogl_framework_api, dlsym(handle, "glVertexAttribs3dvNV"));
    SET_VertexAttribs3fvNV(__ogl_framework_api, dlsym(handle, "glVertexAttribs3fvNV"));
    SET_VertexAttribs3svNV(__ogl_framework_api, dlsym(handle, "glVertexAttribs3svNV"));
    SET_VertexAttribs4dvNV(__ogl_framework_api, dlsym(handle, "glVertexAttribs4dvNV"));
    SET_VertexAttribs4fvNV(__ogl_framework_api, dlsym(handle, "glVertexAttribs4fvNV"));
    SET_VertexAttribs4svNV(__ogl_framework_api, dlsym(handle, "glVertexAttribs4svNV"));
    SET_VertexAttribs4ubvNV(__ogl_framework_api, dlsym(handle, "glVertexAttribs4ubvNV"));

    /* GL_SGIS_multisample */
    SET_SampleMaskSGIS(__ogl_framework_api, dlsym(handle, "glSampleMaskSGIS"));
    SET_SamplePatternSGIS(__ogl_framework_api, dlsym(handle, "glSamplePatternSGIS"));

    /* GL_SGIS_pixel_texture */
    SET_GetPixelTexGenParameterfvSGIS(__ogl_framework_api, dlsym(handle, "glGetPixelTexGenParameterfvSGIS"));
    SET_GetPixelTexGenParameterivSGIS(__ogl_framework_api, dlsym(handle, "glGetPixelTexGenParameterivSGIS"));
    SET_PixelTexGenParameterfSGIS(__ogl_framework_api, dlsym(handle, "glPixelTexGenParameterfSGIS"));
    SET_PixelTexGenParameterfvSGIS(__ogl_framework_api, dlsym(handle, "glPixelTexGenParameterfvSGIS"));
    SET_PixelTexGenParameteriSGIS(__ogl_framework_api, dlsym(handle, "glPixelTexGenParameteriSGIS"));
    SET_PixelTexGenParameterivSGIS(__ogl_framework_api, dlsym(handle, "glPixelTexGenParameterivSGIS"));
    SET_PixelTexGenSGIX(__ogl_framework_api, dlsym(handle, "glPixelTexGenSGIX"));

    __applegl_api = malloc(sizeof(struct _glapi_table));
    assert(__applegl_api);
    memcpy(__applegl_api, __ogl_framework_api, sizeof(struct _glapi_table));

    SET_ReadPixels(__applegl_api, __applegl_glReadPixels);
    SET_CopyPixels(__applegl_api, __applegl_glCopyPixels);
    SET_CopyColorTable(__applegl_api, __applegl_glCopyColorTable);
    SET_DrawBuffer(__applegl_api, __applegl_glDrawBuffer);
    SET_DrawBuffersARB(__applegl_api, __applegl_glDrawBuffersARB);
    SET_Viewport(__applegl_api, __applegl_glViewport);

    _glapi_set_dispatch(__applegl_api);
}
