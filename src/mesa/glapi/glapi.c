/* $Id: glapi.c,v 1.10 1999/11/27 21:30:40 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 *
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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
 * This file manages the OpenGL API dispatch layer.
 * The dispatch table (struct _glapi_table) is basically just a list
 * of function pointers.
 * There are functions to set/get the current dispatch table for the
 * current thread and to manage registration/dispatch of dynamically
 * added extension functions.
 */



#include <assert.h>
#include <stdlib.h>  /* to get NULL */
#include <string.h>
#include "glapi.h"
#include "glapinoop.h"



/*
 * Define the DISPATCH_SETUP and DISPATCH macros here dependant on
 * whether or not threading is enabled.
 */
#if defined(THREADS)

/*** Thread-safe API dispatch ***/
#include "mthreads.h"
static MesaTSD mesa_dispatch_tsd;
static void mesa_dispatch_thread_init() {
  MesaInitTSD(&mesa_dispatch_tsd);
}
#define DISPATCH_SETUP struct _glapi_table *dispatch = (struct _glapi_table *) MesaGetTSD(&mesa_dispatch_tsd)
#define DISPATCH(FUNC, ARGS) (dispatch->FUNC) ARGS


#else

/*** Non-threaded API dispatch ***/
static struct _glapi_table *Dispatch = &__glapi_noop_table;
#define DISPATCH_SETUP
#define DISPATCH(FUNC, ARGS) (Dispatch->FUNC) ARGS

#endif



/*
 * Set the global or per-thread dispatch table pointer.
 */
void
_glapi_set_dispatch(struct _glapi_table *dispatch)
{
   if (!dispatch) {
      /* use the no-op functions */
      dispatch = &__glapi_noop_table;
   }
#ifdef DEBUG
   else {
      _glapi_check_table(dispatch);
   }
#endif

#if defined(THREADS)
   MesaSetTSD(&mesa_dispatch_tsd, (void*) dispatch, mesa_dispatch_thread_init);
#else
   Dispatch = dispatch;
#endif
}


/*
 * Get the global or per-thread dispatch table pointer.
 */
struct _glapi_table *
_glapi_get_dispatch(void)
{
#if defined(THREADS)
   /* return this thread's dispatch pointer */
   return (struct _glapi_table *) MesaGetTSD(&mesa_dispatch_tsd);
#else
   return Dispatch;
#endif
}



/*
 * Return size of dispatch table struct as number of functions (or
 * slots).
 */
GLuint
_glapi_get_dispatch_table_size(void)
{
   return sizeof(struct _glapi_table) / sizeof(void *);
}



/*
 * Get API dispatcher version string.
 * XXX this isn't well defined yet.
 */
const char *
_glapi_get_version(void)
{
   return "1.2";
}


/*
 * Return list of the hard-coded extension entrypoints in the dispatch table.
 */
const char **
_glapi_get_extensions(void)
{
   static const char *extensions[] = {
#ifdef _GLAPI_EXT_paletted_texture
      "GL_EXT_paletted_texture",
#endif
#ifdef _GLAPI_EXT_compiled_vertex_array
      "GL_EXT_compiled_vertex_array",
#endif
#ifdef _GLAPI_EXT_point_parameters
      "GL_EXT_point_parameters",
#endif
#ifdef _GLAPI_EXT_polygon_offset
      "GL_EXT_polygon_offset",
#endif
#ifdef _GLAPI_EXT_blend_minmax
      "GL_EXT_blend_minmax",
#endif
#ifdef _GLAPI_EXT_blend_color
      "GL_EXT_blend_color",
#endif
#ifdef _GLAPI_ARB_multitexture
      "GL_ARB_multitexture",
#endif
#ifdef _GLAPI_INGR_blend_func_separate
      "GL_INGR_blend_func_separate",
#endif
#ifdef _GLAPI_MESA_window_pos
      "GL_MESA_window_pos",
#endif
#ifdef _GLAPI_MESA_resize_buffers
      "GL_MESA_resize_buffers",
#endif
      NULL
   };

   return extensions;
}



struct name_address_pair {
   const char *Name;
   GLvoid *Address;
};

static struct name_address_pair static_functions[] = {
	{ "glAccum", (GLvoid *) glAccum },
	{ "glAlphaFunc", (GLvoid *) glAlphaFunc },
	{ "glBegin", (GLvoid *) glBegin },
	{ "glBitmap", (GLvoid *) glBitmap },
	{ "glAccum", (GLvoid *) glAccum },
	{ "glAlphaFunc", (GLvoid *) glAlphaFunc },
	{ "glBegin", (GLvoid *) glBegin },
	{ "glBitmap", (GLvoid *) glBitmap },
	{ "glBlendFunc", (GLvoid *) glBlendFunc },
	{ "glCallList", (GLvoid *) glCallList },
	{ "glCallLists", (GLvoid *) glCallLists },
	{ "glClear", (GLvoid *) glClear },
	{ "glClearAccum", (GLvoid *) glClearAccum },
	{ "glClearColor", (GLvoid *) glClearColor },
	{ "glClearDepth", (GLvoid *) glClearDepth },
	{ "glClearIndex", (GLvoid *) glClearIndex },
	{ "glClearStencil", (GLvoid *) glClearStencil },
	{ "glClipPlane", (GLvoid *) glClipPlane },
	{ "glColor3b", (GLvoid *) glColor3b },
	{ "glColor3bv", (GLvoid *) glColor3bv },
	{ "glColor3d", (GLvoid *) glColor3d },
	{ "glColor3dv", (GLvoid *) glColor3dv },
	{ "glColor3f", (GLvoid *) glColor3f },
	{ "glColor3fv", (GLvoid *) glColor3fv },
	{ "glColor3i", (GLvoid *) glColor3i },
	{ "glColor3iv", (GLvoid *) glColor3iv },
	{ "glColor3s", (GLvoid *) glColor3s },
	{ "glColor3sv", (GLvoid *) glColor3sv },
	{ "glColor3ub", (GLvoid *) glColor3ub },
	{ "glColor3ubv", (GLvoid *) glColor3ubv },
	{ "glColor3ui", (GLvoid *) glColor3ui },
	{ "glColor3uiv", (GLvoid *) glColor3uiv },
	{ "glColor3us", (GLvoid *) glColor3us },
	{ "glColor3usv", (GLvoid *) glColor3usv },
	{ "glColor4b", (GLvoid *) glColor4b },
	{ "glColor4bv", (GLvoid *) glColor4bv },
	{ "glColor4d", (GLvoid *) glColor4d },
	{ "glColor4dv", (GLvoid *) glColor4dv },
	{ "glColor4f", (GLvoid *) glColor4f },
	{ "glColor4fv", (GLvoid *) glColor4fv },
	{ "glColor4i", (GLvoid *) glColor4i },
	{ "glColor4iv", (GLvoid *) glColor4iv },
	{ "glColor4s", (GLvoid *) glColor4s },
	{ "glColor4sv", (GLvoid *) glColor4sv },
	{ "glColor4ub", (GLvoid *) glColor4ub },
	{ "glColor4ubv", (GLvoid *) glColor4ubv },
	{ "glColor4ui", (GLvoid *) glColor4ui },
	{ "glColor4uiv", (GLvoid *) glColor4uiv },
	{ "glColor4us", (GLvoid *) glColor4us },
	{ "glColor4usv", (GLvoid *) glColor4usv },
	{ "glColorMask", (GLvoid *) glColorMask },
	{ "glColorMaterial", (GLvoid *) glColorMaterial },
	{ "glCopyPixels", (GLvoid *) glCopyPixels },
	{ "glCullFace", (GLvoid *) glCullFace },
	{ "glDeleteLists", (GLvoid *) glDeleteLists },
	{ "glDepthFunc", (GLvoid *) glDepthFunc },
	{ "glDepthMask", (GLvoid *) glDepthMask },
	{ "glDepthRange", (GLvoid *) glDepthRange },
	{ "glDisable", (GLvoid *) glDisable },
	{ "glDrawBuffer", (GLvoid *) glDrawBuffer },
	{ "glDrawPixels", (GLvoid *) glDrawPixels },
	{ "glEdgeFlag", (GLvoid *) glEdgeFlag },
	{ "glEdgeFlagv", (GLvoid *) glEdgeFlagv },
	{ "glEnable", (GLvoid *) glEnable },
	{ "glEnd", (GLvoid *) glEnd },
	{ "glEndList", (GLvoid *) glEndList },
	{ "glEvalCoord1d", (GLvoid *) glEvalCoord1d },
	{ "glEvalCoord1dv", (GLvoid *) glEvalCoord1dv },
	{ "glEvalCoord1f", (GLvoid *) glEvalCoord1f },
	{ "glEvalCoord1fv", (GLvoid *) glEvalCoord1fv },
	{ "glEvalCoord2d", (GLvoid *) glEvalCoord2d },
	{ "glEvalCoord2dv", (GLvoid *) glEvalCoord2dv },
	{ "glEvalCoord2f", (GLvoid *) glEvalCoord2f },
	{ "glEvalCoord2fv", (GLvoid *) glEvalCoord2fv },
	{ "glEvalMesh1", (GLvoid *) glEvalMesh1 },
	{ "glEvalMesh2", (GLvoid *) glEvalMesh2 },
	{ "glEvalPoint1", (GLvoid *) glEvalPoint1 },
	{ "glEvalPoint2", (GLvoid *) glEvalPoint2 },
	{ "glFeedbackBuffer", (GLvoid *) glFeedbackBuffer },
	{ "glFinish", (GLvoid *) glFinish },
	{ "glFlush", (GLvoid *) glFlush },
	{ "glFogf", (GLvoid *) glFogf },
	{ "glFogfv", (GLvoid *) glFogfv },
	{ "glFogi", (GLvoid *) glFogi },
	{ "glFogiv", (GLvoid *) glFogiv },
	{ "glFrontFace", (GLvoid *) glFrontFace },
	{ "glFrustum", (GLvoid *) glFrustum },
	{ "glGenLists", (GLvoid *) glGenLists },
	{ "glGetBooleanv", (GLvoid *) glGetBooleanv },
	{ "glGetClipPlane", (GLvoid *) glGetClipPlane },
	{ "glGetDoublev", (GLvoid *) glGetDoublev },
	{ "glGetError", (GLvoid *) glGetError },
	{ "glGetFloatv", (GLvoid *) glGetFloatv },
	{ "glGetIntegerv", (GLvoid *) glGetIntegerv },
	{ "glGetLightfv", (GLvoid *) glGetLightfv },
	{ "glGetLightiv", (GLvoid *) glGetLightiv },
	{ "glGetMapdv", (GLvoid *) glGetMapdv },
	{ "glGetMapfv", (GLvoid *) glGetMapfv },
	{ "glGetMapiv", (GLvoid *) glGetMapiv },
	{ "glGetMaterialfv", (GLvoid *) glGetMaterialfv },
	{ "glGetMaterialiv", (GLvoid *) glGetMaterialiv },
	{ "glGetPixelMapfv", (GLvoid *) glGetPixelMapfv },
	{ "glGetPixelMapuiv", (GLvoid *) glGetPixelMapuiv },
	{ "glGetPixelMapusv", (GLvoid *) glGetPixelMapusv },
	{ "glGetPolygonStipple", (GLvoid *) glGetPolygonStipple },
	{ "glGetString", (GLvoid *) glGetString },
	{ "glGetTexEnvfv", (GLvoid *) glGetTexEnvfv },
	{ "glGetTexEnviv", (GLvoid *) glGetTexEnviv },
	{ "glGetTexGendv", (GLvoid *) glGetTexGendv },
	{ "glGetTexGenfv", (GLvoid *) glGetTexGenfv },
	{ "glGetTexGeniv", (GLvoid *) glGetTexGeniv },
	{ "glGetTexImage", (GLvoid *) glGetTexImage },
	{ "glGetTexLevelParameterfv", (GLvoid *) glGetTexLevelParameterfv },
	{ "glGetTexLevelParameteriv", (GLvoid *) glGetTexLevelParameteriv },
	{ "glGetTexParameterfv", (GLvoid *) glGetTexParameterfv },
	{ "glGetTexParameteriv", (GLvoid *) glGetTexParameteriv },
	{ "glHint", (GLvoid *) glHint },
	{ "glIndexMask", (GLvoid *) glIndexMask },
	{ "glIndexd", (GLvoid *) glIndexd },
	{ "glIndexdv", (GLvoid *) glIndexdv },
	{ "glIndexf", (GLvoid *) glIndexf },
	{ "glIndexfv", (GLvoid *) glIndexfv },
	{ "glIndexi", (GLvoid *) glIndexi },
	{ "glIndexiv", (GLvoid *) glIndexiv },
	{ "glIndexs", (GLvoid *) glIndexs },
	{ "glIndexsv", (GLvoid *) glIndexsv },
	{ "glInitNames", (GLvoid *) glInitNames },
	{ "glIsEnabled", (GLvoid *) glIsEnabled },
	{ "glIsList", (GLvoid *) glIsList },
	{ "glLightModelf", (GLvoid *) glLightModelf },
	{ "glLightModelfv", (GLvoid *) glLightModelfv },
	{ "glLightModeli", (GLvoid *) glLightModeli },
	{ "glLightModeliv", (GLvoid *) glLightModeliv },
	{ "glLightf", (GLvoid *) glLightf },
	{ "glLightfv", (GLvoid *) glLightfv },
	{ "glLighti", (GLvoid *) glLighti },
	{ "glLightiv", (GLvoid *) glLightiv },
	{ "glLineStipple", (GLvoid *) glLineStipple },
	{ "glLineWidth", (GLvoid *) glLineWidth },
	{ "glListBase", (GLvoid *) glListBase },
	{ "glLoadIdentity", (GLvoid *) glLoadIdentity },
	{ "glLoadMatrixd", (GLvoid *) glLoadMatrixd },
	{ "glLoadMatrixf", (GLvoid *) glLoadMatrixf },
	{ "glLoadName", (GLvoid *) glLoadName },
	{ "glLogicOp", (GLvoid *) glLogicOp },
	{ "glMap1d", (GLvoid *) glMap1d },
	{ "glMap1f", (GLvoid *) glMap1f },
	{ "glMap2d", (GLvoid *) glMap2d },
	{ "glMap2f", (GLvoid *) glMap2f },
	{ "glMapGrid1d", (GLvoid *) glMapGrid1d },
	{ "glMapGrid1f", (GLvoid *) glMapGrid1f },
	{ "glMapGrid2d", (GLvoid *) glMapGrid2d },
	{ "glMapGrid2f", (GLvoid *) glMapGrid2f },
	{ "glMaterialf", (GLvoid *) glMaterialf },
	{ "glMaterialfv", (GLvoid *) glMaterialfv },
	{ "glMateriali", (GLvoid *) glMateriali },
	{ "glMaterialiv", (GLvoid *) glMaterialiv },
	{ "glMatrixMode", (GLvoid *) glMatrixMode },
	{ "glMultMatrixd", (GLvoid *) glMultMatrixd },
	{ "glMultMatrixf", (GLvoid *) glMultMatrixf },
	{ "glNewList", (GLvoid *) glNewList },
	{ "glNormal3b", (GLvoid *) glNormal3b },
	{ "glNormal3bv", (GLvoid *) glNormal3bv },
	{ "glNormal3d", (GLvoid *) glNormal3d },
	{ "glNormal3dv", (GLvoid *) glNormal3dv },
	{ "glNormal3f", (GLvoid *) glNormal3f },
	{ "glNormal3fv", (GLvoid *) glNormal3fv },
	{ "glNormal3i", (GLvoid *) glNormal3i },
	{ "glNormal3iv", (GLvoid *) glNormal3iv },
	{ "glNormal3s", (GLvoid *) glNormal3s },
	{ "glNormal3sv", (GLvoid *) glNormal3sv },
	{ "glOrtho", (GLvoid *) glOrtho },
	{ "glPassThrough", (GLvoid *) glPassThrough },
	{ "glPixelMapfv", (GLvoid *) glPixelMapfv },
	{ "glPixelMapuiv", (GLvoid *) glPixelMapuiv },
	{ "glPixelMapusv", (GLvoid *) glPixelMapusv },
	{ "glPixelStoref", (GLvoid *) glPixelStoref },
	{ "glPixelStorei", (GLvoid *) glPixelStorei },
	{ "glPixelTransferf", (GLvoid *) glPixelTransferf },
	{ "glPixelTransferi", (GLvoid *) glPixelTransferi },
	{ "glPixelZoom", (GLvoid *) glPixelZoom },
	{ "glPointSize", (GLvoid *) glPointSize },
	{ "glPolygonMode", (GLvoid *) glPolygonMode },
	{ "glPolygonOffset", (GLvoid *) glPolygonOffset },
	{ "glPolygonStipple", (GLvoid *) glPolygonStipple },
	{ "glPopAttrib", (GLvoid *) glPopAttrib },
	{ "glPopMatrix", (GLvoid *) glPopMatrix },
	{ "glPopName", (GLvoid *) glPopName },
	{ "glPushAttrib", (GLvoid *) glPushAttrib },
	{ "glPushMatrix", (GLvoid *) glPushMatrix },
	{ "glPushName", (GLvoid *) glPushName },
	{ "glRasterPos2d", (GLvoid *) glRasterPos2d },
	{ "glRasterPos2dv", (GLvoid *) glRasterPos2dv },
	{ "glRasterPos2f", (GLvoid *) glRasterPos2f },
	{ "glRasterPos2fv", (GLvoid *) glRasterPos2fv },
	{ "glRasterPos2i", (GLvoid *) glRasterPos2i },
	{ "glRasterPos2iv", (GLvoid *) glRasterPos2iv },
	{ "glRasterPos2s", (GLvoid *) glRasterPos2s },
	{ "glRasterPos2sv", (GLvoid *) glRasterPos2sv },
	{ "glRasterPos3d", (GLvoid *) glRasterPos3d },
	{ "glRasterPos3dv", (GLvoid *) glRasterPos3dv },
	{ "glRasterPos3f", (GLvoid *) glRasterPos3f },
	{ "glRasterPos3fv", (GLvoid *) glRasterPos3fv },
	{ "glRasterPos3i", (GLvoid *) glRasterPos3i },
	{ "glRasterPos3iv", (GLvoid *) glRasterPos3iv },
	{ "glRasterPos3s", (GLvoid *) glRasterPos3s },
	{ "glRasterPos3sv", (GLvoid *) glRasterPos3sv },
	{ "glRasterPos4d", (GLvoid *) glRasterPos4d },
	{ "glRasterPos4dv", (GLvoid *) glRasterPos4dv },
	{ "glRasterPos4f", (GLvoid *) glRasterPos4f },
	{ "glRasterPos4fv", (GLvoid *) glRasterPos4fv },
	{ "glRasterPos4i", (GLvoid *) glRasterPos4i },
	{ "glRasterPos4iv", (GLvoid *) glRasterPos4iv },
	{ "glRasterPos4s", (GLvoid *) glRasterPos4s },
	{ "glRasterPos4sv", (GLvoid *) glRasterPos4sv },
	{ "glReadBuffer", (GLvoid *) glReadBuffer },
	{ "glReadPixels", (GLvoid *) glReadPixels },
	{ "glRectd", (GLvoid *) glRectd },
	{ "glRectdv", (GLvoid *) glRectdv },
	{ "glRectf", (GLvoid *) glRectf },
	{ "glRectfv", (GLvoid *) glRectfv },
	{ "glRecti", (GLvoid *) glRecti },
	{ "glRectiv", (GLvoid *) glRectiv },
	{ "glRects", (GLvoid *) glRects },
	{ "glRectsv", (GLvoid *) glRectsv },
	{ "glRenderMode", (GLvoid *) glRenderMode },
	{ "glRotated", (GLvoid *) glRotated },
	{ "glRotatef", (GLvoid *) glRotatef },
	{ "glScaled", (GLvoid *) glScaled },
	{ "glScalef", (GLvoid *) glScalef },
	{ "glScissor", (GLvoid *) glScissor },
	{ "glSelectBuffer", (GLvoid *) glSelectBuffer },
	{ "glShadeModel", (GLvoid *) glShadeModel },
	{ "glStencilFunc", (GLvoid *) glStencilFunc },
	{ "glStencilMask", (GLvoid *) glStencilMask },
	{ "glStencilOp", (GLvoid *) glStencilOp },
	{ "glTexCoord1d", (GLvoid *) glTexCoord1d },
	{ "glTexCoord1dv", (GLvoid *) glTexCoord1dv },
	{ "glTexCoord1f", (GLvoid *) glTexCoord1f },
	{ "glTexCoord1fv", (GLvoid *) glTexCoord1fv },
	{ "glTexCoord1i", (GLvoid *) glTexCoord1i },
	{ "glTexCoord1iv", (GLvoid *) glTexCoord1iv },
	{ "glTexCoord1s", (GLvoid *) glTexCoord1s },
	{ "glTexCoord1sv", (GLvoid *) glTexCoord1sv },
	{ "glTexCoord2d", (GLvoid *) glTexCoord2d },
	{ "glTexCoord2dv", (GLvoid *) glTexCoord2dv },
	{ "glTexCoord2f", (GLvoid *) glTexCoord2f },
	{ "glTexCoord2fv", (GLvoid *) glTexCoord2fv },
	{ "glTexCoord2i", (GLvoid *) glTexCoord2i },
	{ "glTexCoord2iv", (GLvoid *) glTexCoord2iv },
	{ "glTexCoord2s", (GLvoid *) glTexCoord2s },
	{ "glTexCoord2sv", (GLvoid *) glTexCoord2sv },
	{ "glTexCoord3d", (GLvoid *) glTexCoord3d },
	{ "glTexCoord3dv", (GLvoid *) glTexCoord3dv },
	{ "glTexCoord3f", (GLvoid *) glTexCoord3f },
	{ "glTexCoord3fv", (GLvoid *) glTexCoord3fv },
	{ "glTexCoord3i", (GLvoid *) glTexCoord3i },
	{ "glTexCoord3iv", (GLvoid *) glTexCoord3iv },
	{ "glTexCoord3s", (GLvoid *) glTexCoord3s },
	{ "glTexCoord3sv", (GLvoid *) glTexCoord3sv },
	{ "glTexCoord4d", (GLvoid *) glTexCoord4d },
	{ "glTexCoord4dv", (GLvoid *) glTexCoord4dv },
	{ "glTexCoord4f", (GLvoid *) glTexCoord4f },
	{ "glTexCoord4fv", (GLvoid *) glTexCoord4fv },
	{ "glTexCoord4i", (GLvoid *) glTexCoord4i },
	{ "glTexCoord4iv", (GLvoid *) glTexCoord4iv },
	{ "glTexCoord4s", (GLvoid *) glTexCoord4s },
	{ "glTexCoord4sv", (GLvoid *) glTexCoord4sv },
	{ "glTexEnvf", (GLvoid *) glTexEnvf },
	{ "glTexEnvfv", (GLvoid *) glTexEnvfv },
	{ "glTexEnvi", (GLvoid *) glTexEnvi },
	{ "glTexEnviv", (GLvoid *) glTexEnviv },
	{ "glTexGend", (GLvoid *) glTexGend },
	{ "glTexGendv", (GLvoid *) glTexGendv },
	{ "glTexGenf", (GLvoid *) glTexGenf },
	{ "glTexGenfv", (GLvoid *) glTexGenfv },
	{ "glTexGeni", (GLvoid *) glTexGeni },
	{ "glTexGeniv", (GLvoid *) glTexGeniv },
	{ "glTexImage1D", (GLvoid *) glTexImage1D },
	{ "glTexImage2D", (GLvoid *) glTexImage2D },
	{ "glTexParameterf", (GLvoid *) glTexParameterf },
	{ "glTexParameterfv", (GLvoid *) glTexParameterfv },
	{ "glTexParameteri", (GLvoid *) glTexParameteri },
	{ "glTexParameteriv", (GLvoid *) glTexParameteriv },
	{ "glTranslated", (GLvoid *) glTranslated },
	{ "glTranslatef", (GLvoid *) glTranslatef },
	{ "glVertex2d", (GLvoid *) glVertex2d },
	{ "glVertex2dv", (GLvoid *) glVertex2dv },
	{ "glVertex2f", (GLvoid *) glVertex2f },
	{ "glVertex2fv", (GLvoid *) glVertex2fv },
	{ "glVertex2i", (GLvoid *) glVertex2i },
	{ "glVertex2iv", (GLvoid *) glVertex2iv },
	{ "glVertex2s", (GLvoid *) glVertex2s },
	{ "glVertex2sv", (GLvoid *) glVertex2sv },
	{ "glVertex3d", (GLvoid *) glVertex3d },
	{ "glVertex3dv", (GLvoid *) glVertex3dv },
	{ "glVertex3f", (GLvoid *) glVertex3f },
	{ "glVertex3fv", (GLvoid *) glVertex3fv },
	{ "glVertex3i", (GLvoid *) glVertex3i },
	{ "glVertex3iv", (GLvoid *) glVertex3iv },
	{ "glVertex3s", (GLvoid *) glVertex3s },
	{ "glVertex3sv", (GLvoid *) glVertex3sv },
	{ "glVertex4d", (GLvoid *) glVertex4d },
	{ "glVertex4dv", (GLvoid *) glVertex4dv },
	{ "glVertex4f", (GLvoid *) glVertex4f },
	{ "glVertex4fv", (GLvoid *) glVertex4fv },
	{ "glVertex4i", (GLvoid *) glVertex4i },
	{ "glVertex4iv", (GLvoid *) glVertex4iv },
	{ "glVertex4s", (GLvoid *) glVertex4s },
	{ "glVertex4sv", (GLvoid *) glVertex4sv },
	{ "glViewport", (GLvoid *) glViewport },

#ifdef _GLAPI_VERSION_1_1
	{ "glAreTexturesResident", (GLvoid *) glAreTexturesResident },
	{ "glArrayElement", (GLvoid *) glArrayElement },
	{ "glBindTexture", (GLvoid *) glBindTexture },
	{ "glColorPointer", (GLvoid *) glColorPointer },
	{ "glCopyTexImage1D", (GLvoid *) glCopyTexImage1D },
	{ "glCopyTexImage2D", (GLvoid *) glCopyTexImage2D },
	{ "glCopyTexSubImage1D", (GLvoid *) glCopyTexSubImage1D },
	{ "glCopyTexSubImage2D", (GLvoid *) glCopyTexSubImage2D },
	{ "glDeleteTextures", (GLvoid *) glDeleteTextures },
	{ "glDisableClientState", (GLvoid *) glDisableClientState },
	{ "glDrawArrays", (GLvoid *) glDrawArrays },
	{ "glDrawElements", (GLvoid *) glDrawElements },
	{ "glEdgeFlagPointer", (GLvoid *) glEdgeFlagPointer },
	{ "glEnableClientState", (GLvoid *) glEnableClientState },
	{ "glGenTextures", (GLvoid *) glGenTextures },
	{ "glGetPointerv", (GLvoid *) glGetPointerv },
	{ "glIndexPointer", (GLvoid *) glIndexPointer },
	{ "glIndexub", (GLvoid *) glIndexub },
	{ "glIndexubv", (GLvoid *) glIndexubv },
	{ "glInterleavedArrays", (GLvoid *) glInterleavedArrays },
	{ "glIsTexture", (GLvoid *) glIsTexture },
	{ "glNormalPointer", (GLvoid *) glNormalPointer },
	{ "glPopClientAttrib", (GLvoid *) glPopClientAttrib },
	{ "glPrioritizeTextures", (GLvoid *) glPrioritizeTextures },
	{ "glPushClientAttrib", (GLvoid *) glPushClientAttrib },
	{ "glTexCoordPointer", (GLvoid *) glTexCoordPointer },
	{ "glTexSubImage1D", (GLvoid *) glTexSubImage1D },
	{ "glTexSubImage2D", (GLvoid *) glTexSubImage2D },
	{ "glVertexPointer", (GLvoid *) glVertexPointer },
#endif

#ifdef _GLAPI_VERSION_1_2
	{ "glCopyTexSubImage3D", (GLvoid *) glCopyTexSubImage3D },
	{ "glDrawRangeElements", (GLvoid *) glDrawRangeElements },
	{ "glTexImage3D", (GLvoid *) glTexImage3D },
	{ "glTexSubImage3D", (GLvoid *) glTexSubImage3D },

#ifdef _GLAPI_ARB_imaging
	{ "glBlendColor", (GLvoid *) glBlendColor },
	{ "glBlendEquation", (GLvoid *) glBlendEquation },
	{ "glColorSubTable", (GLvoid *) glColorSubTable },
	{ "glColorTable", (GLvoid *) glColorTable },
	{ "glColorTableParameterfv", (GLvoid *) glColorTableParameterfv },
	{ "glColorTableParameteriv", (GLvoid *) glColorTableParameteriv },
	{ "glConvolutionFilter1D", (GLvoid *) glConvolutionFilter1D },
	{ "glConvolutionFilter2D", (GLvoid *) glConvolutionFilter2D },
	{ "glConvolutionParameterf", (GLvoid *) glConvolutionParameterf },
	{ "glConvolutionParameterfv", (GLvoid *) glConvolutionParameterfv },
	{ "glConvolutionParameteri", (GLvoid *) glConvolutionParameteri },
	{ "glConvolutionParameteriv", (GLvoid *) glConvolutionParameteriv },
	{ "glCopyColorSubTable", (GLvoid *) glCopyColorSubTable },
	{ "glCopyColorTable", (GLvoid *) glCopyColorTable },
	{ "glCopyConvolutionFilter1D", (GLvoid *) glCopyConvolutionFilter1D },
	{ "glCopyConvolutionFilter2D", (GLvoid *) glCopyConvolutionFilter2D },
	{ "glGetColorTable", (GLvoid *) glGetColorTable },
	{ "glGetColorTableParameterfv", (GLvoid *) glGetColorTableParameterfv },
	{ "glGetColorTableParameteriv", (GLvoid *) glGetColorTableParameteriv },
	{ "glGetConvolutionFilter", (GLvoid *) glGetConvolutionFilter },
	{ "glGetConvolutionParameterfv", (GLvoid *) glGetConvolutionParameterfv },
	{ "glGetConvolutionParameteriv", (GLvoid *) glGetConvolutionParameteriv },
	{ "glGetHistogram", (GLvoid *) glGetHistogram },
	{ "glGetHistogramParameterfv", (GLvoid *) glGetHistogramParameterfv },
	{ "glGetHistogramParameteriv", (GLvoid *) glGetHistogramParameteriv },
	{ "glGetMinmax", (GLvoid *) glGetMinmax },
	{ "glGetMinmaxParameterfv", (GLvoid *) glGetMinmaxParameterfv },
	{ "glGetMinmaxParameteriv", (GLvoid *) glGetMinmaxParameteriv },
	{ "glGetSeparableFilter", (GLvoid *) glGetSeparableFilter },
	{ "glHistogram", (GLvoid *) glHistogram },
	{ "glMinmax", (GLvoid *) glMinmax },
	{ "glResetHistogram", (GLvoid *) glResetHistogram },
	{ "glResetMinmax", (GLvoid *) glResetMinmax },
	{ "glSeparableFilter2D", (GLvoid *) glSeparableFilter2D },
#endif
#endif

#ifdef _GLAPI_EXT_paletted_texture
	{ "glColorTableEXT", (GLvoid *) glColorTableEXT },
	{ "glColorSubTableEXT", (GLvoid *) glColorSubTableEXT },
	{ "glGetColorTableEXT", (GLvoid *) glGetColorTableEXT },
	{ "glGetColorTableParameterfvEXT", (GLvoid *) glGetColorTableParameterfvEXT },
	{ "glGetColorTableParameterivEXT", (GLvoid *) glGetColorTableParameterivEXT },
#endif

#ifdef _GLAPI_EXT_compiled_vertex_array
	{ "glLockArraysEXT", (GLvoid *) glLockArraysEXT },
	{ "glUnlockArraysEXT", (GLvoid *) glUnlockArraysEXT },
#endif

#ifdef _GLAPI_EXT_point_parameters
	{ "glPointParameterfEXT", (GLvoid *) glPointParameterfEXT },
	{ "glPointParameterfvEXT", (GLvoid *) glPointParameterfvEXT },
#endif

#ifdef _GLAPI_EXT_polygon_offset
	{ "glPolygonOffsetEXT", (GLvoid *) glPolygonOffsetEXT },
#endif

#ifdef _GLAPI_EXT_blend_minmax
	{ "glBlendEquationEXT", (GLvoid *) glBlendEquationEXT },
#endif

#ifdef _GLAPI_EXT_blend_color
	{ "glBlendColorEXT", (GLvoid *) glBlendColorEXT },
#endif

#ifdef _GLAPI_ARB_multitexture
	{ "glActiveTextureARB", (GLvoid *) glActiveTextureARB },
	{ "glClientActiveTextureARB", (GLvoid *) glClientActiveTextureARB },
	{ "glMultiTexCoord1dARB", (GLvoid *) glMultiTexCoord1dARB },
	{ "glMultiTexCoord1dvARB", (GLvoid *) glMultiTexCoord1dvARB },
	{ "glMultiTexCoord1fARB", (GLvoid *) glMultiTexCoord1fARB },
	{ "glMultiTexCoord1fvARB", (GLvoid *) glMultiTexCoord1fvARB },
	{ "glMultiTexCoord1iARB", (GLvoid *) glMultiTexCoord1iARB },
	{ "glMultiTexCoord1ivARB", (GLvoid *) glMultiTexCoord1ivARB },
	{ "glMultiTexCoord1sARB", (GLvoid *) glMultiTexCoord1sARB },
	{ "glMultiTexCoord1svARB", (GLvoid *) glMultiTexCoord1svARB },
	{ "glMultiTexCoord2dARB", (GLvoid *) glMultiTexCoord2dARB },
	{ "glMultiTexCoord2dvARB", (GLvoid *) glMultiTexCoord2dvARB },
	{ "glMultiTexCoord2fARB", (GLvoid *) glMultiTexCoord2fARB },
	{ "glMultiTexCoord2fvARB", (GLvoid *) glMultiTexCoord2fvARB },
	{ "glMultiTexCoord2iARB", (GLvoid *) glMultiTexCoord2iARB },
	{ "glMultiTexCoord2ivARB", (GLvoid *) glMultiTexCoord2ivARB },
	{ "glMultiTexCoord2sARB", (GLvoid *) glMultiTexCoord2sARB },
	{ "glMultiTexCoord2svARB", (GLvoid *) glMultiTexCoord2svARB },
	{ "glMultiTexCoord3dARB", (GLvoid *) glMultiTexCoord3dARB },
	{ "glMultiTexCoord3dvARB", (GLvoid *) glMultiTexCoord3dvARB },
	{ "glMultiTexCoord3fARB", (GLvoid *) glMultiTexCoord3fARB },
	{ "glMultiTexCoord3fvARB", (GLvoid *) glMultiTexCoord3fvARB },
	{ "glMultiTexCoord3iARB", (GLvoid *) glMultiTexCoord3iARB },
	{ "glMultiTexCoord3ivARB", (GLvoid *) glMultiTexCoord3ivARB },
	{ "glMultiTexCoord3sARB", (GLvoid *) glMultiTexCoord3sARB },
	{ "glMultiTexCoord3svARB", (GLvoid *) glMultiTexCoord3svARB },
	{ "glMultiTexCoord4dARB", (GLvoid *) glMultiTexCoord4dARB },
	{ "glMultiTexCoord4dvARB", (GLvoid *) glMultiTexCoord4dvARB },
	{ "glMultiTexCoord4fARB", (GLvoid *) glMultiTexCoord4fARB },
	{ "glMultiTexCoord4fvARB", (GLvoid *) glMultiTexCoord4fvARB },
	{ "glMultiTexCoord4iARB", (GLvoid *) glMultiTexCoord4iARB },
	{ "glMultiTexCoord4ivARB", (GLvoid *) glMultiTexCoord4ivARB },
	{ "glMultiTexCoord4sARB", (GLvoid *) glMultiTexCoord4sARB },
	{ "glMultiTexCoord4svARB", (GLvoid *) glMultiTexCoord4svARB },
#endif

#ifdef _GLAPI_INGR_blend_func_separate
	{ "glBlendFuncSeparateINGR", (GLvoid *) glBlendFuncSeparateINGR },
#endif

#ifdef _GLAPI_MESA_window_pos
	{ "glWindowPos4fMESA", (GLvoid *) glWindowPos4fMESA },
#endif

#ifdef _GLAPI_MESA_resize_buffers
	{ "glResizeBuffersMESA", (GLvoid *) glResizeBuffersMESA },
#endif

      { NULL, NULL }  /* end of list marker */
};



/*
 * Return dispatch table offset of the named static (built-in) function.
 * Return -1 if function not found.
 */
static GLint
get_static_proc_offset(const char *funcName)
{
   GLuint i;
   for (i = 0; static_functions[i].Name; i++) {
      if (strcmp(static_functions[i].Name, funcName) == 0) {
         return i;
      }
   }
   return -1;
}


/*
 * Return dispatch function address the named static (built-in) function.
 * Return NULL if function not found.
 */
static GLvoid *
get_static_proc_address(const char *funcName)
{
   GLuint i = get_static_proc_offset(funcName);
   if (i >= 0)
      return static_functions[i].Address;
   else
      return NULL;
}



/**********************************************************************
 * Extension function management.
 **********************************************************************/


struct _glapi_ext_entrypoint {
   const char *Name;   /* the extension function's name */
   GLuint Offset;      /* relative to start of dispatch table */
   GLvoid *Address;    /* address of dispatch function */
};

static struct _glapi_ext_entrypoint ExtEntryTable[_GLAPI_EXTRA_SLOTS];
static GLuint NumExtEntryPoints = 0;



/*
 * Generate a dispatch function (entrypoint) which jumps through
 * the given slot number (offset) in the current dispatch table.
 */
static void *
generate_entrypoint(GLuint offset)
{
   /* XXX need to generate some assembly code here */

   return NULL;
}



/*
 * Add a new extension function entrypoint.
 * Return: GL_TRUE = success or GL_FALSE = failure
 */
GLboolean
_glapi_add_entrypoint(const char *funcName, GLuint offset)
{
   /* first check if the named function is already statically present */
   GLint index = get_static_proc_offset(funcName);
   if (index >= 0) {
      assert(index == offset);
      return GL_TRUE;
   }
   else if (offset < _glapi_get_dispatch_table_size()) {
      /* be sure index and name match known data */
      GLuint i;
      for (i = 0; i < NumExtEntryPoints; i++) {
         if (strcmp(ExtEntryTable[i].Name, funcName) == 0) {
            /* function already registered with api */
            if (ExtEntryTable[i].Offset == offset) {
               return GL_TRUE;  /* offsets match */
            }
            else {
               return GL_FALSE;  /* bad offset! */
            }
         }
      }
      assert(NumExtEntryPoints < _GLAPI_EXTRA_SLOTS);
      ExtEntryTable[NumExtEntryPoints].Name = strdup(funcName);
      ExtEntryTable[NumExtEntryPoints].Offset = offset;
      ExtEntryTable[NumExtEntryPoints].Address = generate_entrypoint(offset);
      NumExtEntryPoints++;
      return GL_TRUE;      
   }
   else {
      return GL_FALSE;
   }
}



/*
 * Return offset of entrypoint for named function within dispatch table.
 */
GLint
_glapi_get_proc_offset(const char *funcName)
{
   /* search extension functions first */
   GLint i;
   for (i = 0; i < NumExtEntryPoints; i++) {
      if (strcmp(ExtEntryTable[i].Name, funcName) == 0) {
         return ExtEntryTable[i].Offset;
      }
   }

   /* search static functions */
   return get_static_proc_offset(funcName);
}



/*
 * Return entrypoint for named function.
 */
const GLvoid *
_glapi_get_proc_address(const char *funcName)
{
   /* search extension functions first */
   GLint i;
   for (i = 0; i < NumExtEntryPoints; i++) {
      if (strcmp(ExtEntryTable[i].Name, funcName) == 0) {
         return ExtEntryTable[i].Address;
      }
   }

   /* search static functions */
   return get_static_proc_address(funcName);
}



/*
 * Make sure there are no NULL pointers in the given dispatch table.
 * Intented for debugging purposes.
 */
void
_glapi_check_table(const struct _glapi_table *table)
{
   assert(table->Accum);
   assert(table->AlphaFunc);
   assert(table->Begin);
   assert(table->Bitmap);
   assert(table->BlendFunc);
   assert(table->CallList);
   assert(table->CallLists);
   assert(table->Clear);
   assert(table->ClearAccum);
   assert(table->ClearColor);
   assert(table->ClearDepth);
   assert(table->ClearIndex);
   assert(table->ClearStencil);
   assert(table->ClipPlane);
   assert(table->Color3b);
   assert(table->Color3bv);
   assert(table->Color3d);
   assert(table->Color3dv);
   assert(table->Color3f);
   assert(table->Color3fv);
   assert(table->Color3i);
   assert(table->Color3iv);
   assert(table->Color3s);
   assert(table->Color3sv);
   assert(table->Color3ub);
   assert(table->Color3ubv);
   assert(table->Color3ui);
   assert(table->Color3uiv);
   assert(table->Color3us);
   assert(table->Color3usv);
   assert(table->Color4b);
   assert(table->Color4bv);
   assert(table->Color4d);
   assert(table->Color4dv);
   assert(table->Color4f);
   assert(table->Color4fv);
   assert(table->Color4i);
   assert(table->Color4iv);
   assert(table->Color4s);
   assert(table->Color4sv);
   assert(table->Color4ub);
   assert(table->Color4ubv);
   assert(table->Color4ui);
   assert(table->Color4uiv);
   assert(table->Color4us);
   assert(table->Color4usv);
   assert(table->ColorMask);
   assert(table->ColorMaterial);
   assert(table->CopyPixels);
   assert(table->CullFace);
   assert(table->DeleteLists);
   assert(table->DepthFunc);
   assert(table->DepthMask);
   assert(table->DepthRange);
   assert(table->Disable);
   assert(table->DrawBuffer);
   assert(table->DrawElements);
   assert(table->DrawPixels);
   assert(table->EdgeFlag);
   assert(table->EdgeFlagv);
   assert(table->Enable);
   assert(table->End);
   assert(table->EndList);
   assert(table->EvalCoord1d);
   assert(table->EvalCoord1dv);
   assert(table->EvalCoord1f);
   assert(table->EvalCoord1fv);
   assert(table->EvalCoord2d);
   assert(table->EvalCoord2dv);
   assert(table->EvalCoord2f);
   assert(table->EvalCoord2fv);
   assert(table->EvalMesh1);
   assert(table->EvalMesh2);
   assert(table->EvalPoint1);
   assert(table->EvalPoint2);
   assert(table->FeedbackBuffer);
   assert(table->Finish);
   assert(table->Flush);
   assert(table->Fogf);
   assert(table->Fogfv);
   assert(table->Fogi);
   assert(table->Fogiv);
   assert(table->FrontFace);
   assert(table->Frustum);
   assert(table->GenLists);
   assert(table->GetBooleanv);
   assert(table->GetClipPlane);
   assert(table->GetDoublev);
   assert(table->GetError);
   assert(table->GetFloatv);
   assert(table->GetIntegerv);
   assert(table->GetLightfv);
   assert(table->GetLightiv);
   assert(table->GetMapdv);
   assert(table->GetMapfv);
   assert(table->GetMapiv);
   assert(table->GetMaterialfv);
   assert(table->GetMaterialiv);
   assert(table->GetPixelMapfv);
   assert(table->GetPixelMapuiv);
   assert(table->GetPixelMapusv);
   assert(table->GetPolygonStipple);
   assert(table->GetString);
   assert(table->GetTexEnvfv);
   assert(table->GetTexEnviv);
   assert(table->GetTexGendv);
   assert(table->GetTexGenfv);
   assert(table->GetTexGeniv);
   assert(table->GetTexImage);
   assert(table->GetTexLevelParameterfv);
   assert(table->GetTexLevelParameteriv);
   assert(table->GetTexParameterfv);
   assert(table->GetTexParameteriv);
   assert(table->Hint);
   assert(table->IndexMask);
   assert(table->Indexd);
   assert(table->Indexdv);
   assert(table->Indexf);
   assert(table->Indexfv);
   assert(table->Indexi);
   assert(table->Indexiv);
   assert(table->Indexs);
   assert(table->Indexsv);
   assert(table->InitNames);
   assert(table->IsEnabled);
   assert(table->IsList);
   assert(table->LightModelf);
   assert(table->LightModelfv);
   assert(table->LightModeli);
   assert(table->LightModeliv);
   assert(table->Lightf);
   assert(table->Lightfv);
   assert(table->Lighti);
   assert(table->Lightiv);
   assert(table->LineStipple);
   assert(table->LineWidth);
   assert(table->ListBase);
   assert(table->LoadIdentity);
   assert(table->LoadMatrixd);
   assert(table->LoadMatrixf);
   assert(table->LoadName);
   assert(table->LogicOp);
   assert(table->Map1d);
   assert(table->Map1f);
   assert(table->Map2d);
   assert(table->Map2f);
   assert(table->MapGrid1d);
   assert(table->MapGrid1f);
   assert(table->MapGrid2d);
   assert(table->MapGrid2f);
   assert(table->Materialf);
   assert(table->Materialfv);
   assert(table->Materiali);
   assert(table->Materialiv);
   assert(table->MatrixMode);
   assert(table->MultMatrixd);
   assert(table->MultMatrixf);
   assert(table->NewList);
   assert(table->Normal3b);
   assert(table->Normal3bv);
   assert(table->Normal3d);
   assert(table->Normal3dv);
   assert(table->Normal3f);
   assert(table->Normal3fv);
   assert(table->Normal3i);
   assert(table->Normal3iv);
   assert(table->Normal3s);
   assert(table->Normal3sv);
   assert(table->Ortho);
   assert(table->PassThrough);
   assert(table->PixelMapfv);
   assert(table->PixelMapuiv);
   assert(table->PixelMapusv);
   assert(table->PixelStoref);
   assert(table->PixelStorei);
   assert(table->PixelTransferf);
   assert(table->PixelTransferi);
   assert(table->PixelZoom);
   assert(table->PointSize);
   assert(table->PolygonMode);
   assert(table->PolygonOffset);
   assert(table->PolygonStipple);
   assert(table->PopAttrib);
   assert(table->PopMatrix);
   assert(table->PopName);
   assert(table->PushAttrib);
   assert(table->PushMatrix);
   assert(table->PushName);
   assert(table->RasterPos2d);
   assert(table->RasterPos2dv);
   assert(table->RasterPos2f);
   assert(table->RasterPos2fv);
   assert(table->RasterPos2i);
   assert(table->RasterPos2iv);
   assert(table->RasterPos2s);
   assert(table->RasterPos2sv);
   assert(table->RasterPos3d);
   assert(table->RasterPos3dv);
   assert(table->RasterPos3f);
   assert(table->RasterPos3fv);
   assert(table->RasterPos3i);
   assert(table->RasterPos3iv);
   assert(table->RasterPos3s);
   assert(table->RasterPos3sv);
   assert(table->RasterPos4d);
   assert(table->RasterPos4dv);
   assert(table->RasterPos4f);
   assert(table->RasterPos4fv);
   assert(table->RasterPos4i);
   assert(table->RasterPos4iv);
   assert(table->RasterPos4s);
   assert(table->RasterPos4sv);
   assert(table->ReadBuffer);
   assert(table->ReadPixels);
   assert(table->Rectd);
   assert(table->Rectdv);
   assert(table->Rectf);
   assert(table->Rectfv);
   assert(table->Recti);
   assert(table->Rectiv);
   assert(table->Rects);
   assert(table->Rectsv);
   assert(table->RenderMode);
   assert(table->Rotated);
   assert(table->Rotatef);
   assert(table->Scaled);
   assert(table->Scalef);
   assert(table->Scissor);
   assert(table->SelectBuffer);
   assert(table->ShadeModel);
   assert(table->StencilFunc);
   assert(table->StencilMask);
   assert(table->StencilOp);
   assert(table->TexCoord1d);
   assert(table->TexCoord1dv);
   assert(table->TexCoord1f);
   assert(table->TexCoord1fv);
   assert(table->TexCoord1i);
   assert(table->TexCoord1iv);
   assert(table->TexCoord1s);
   assert(table->TexCoord1sv);
   assert(table->TexCoord2d);
   assert(table->TexCoord2dv);
   assert(table->TexCoord2f);
   assert(table->TexCoord2fv);
   assert(table->TexCoord2i);
   assert(table->TexCoord2iv);
   assert(table->TexCoord2s);
   assert(table->TexCoord2sv);
   assert(table->TexCoord3d);
   assert(table->TexCoord3dv);
   assert(table->TexCoord3f);
   assert(table->TexCoord3fv);
   assert(table->TexCoord3i);
   assert(table->TexCoord3iv);
   assert(table->TexCoord3s);
   assert(table->TexCoord3sv);
   assert(table->TexCoord4d);
   assert(table->TexCoord4dv);
   assert(table->TexCoord4f);
   assert(table->TexCoord4fv);
   assert(table->TexCoord4i);
   assert(table->TexCoord4iv);
   assert(table->TexCoord4s);
   assert(table->TexCoord4sv);
   assert(table->TexEnvf);
   assert(table->TexEnvfv);
   assert(table->TexEnvi);
   assert(table->TexEnviv);
   assert(table->TexGend);
   assert(table->TexGendv);
   assert(table->TexGenf);
   assert(table->TexGenfv);
   assert(table->TexGeni);
   assert(table->TexGeniv);
   assert(table->TexImage1D);
   assert(table->TexImage2D);
   assert(table->TexParameterf);
   assert(table->TexParameterfv);
   assert(table->TexParameteri);
   assert(table->TexParameteriv);
   assert(table->Translated);
   assert(table->Translatef);
   assert(table->Vertex2d);
   assert(table->Vertex2dv);
   assert(table->Vertex2f);
   assert(table->Vertex2fv);
   assert(table->Vertex2i);
   assert(table->Vertex2iv);
   assert(table->Vertex2s);
   assert(table->Vertex2sv);
   assert(table->Vertex3d);
   assert(table->Vertex3dv);
   assert(table->Vertex3f);
   assert(table->Vertex3fv);
   assert(table->Vertex3i);
   assert(table->Vertex3iv);
   assert(table->Vertex3s);
   assert(table->Vertex3sv);
   assert(table->Vertex4d);
   assert(table->Vertex4dv);
   assert(table->Vertex4f);
   assert(table->Vertex4fv);
   assert(table->Vertex4i);
   assert(table->Vertex4iv);
   assert(table->Vertex4s);
   assert(table->Vertex4sv);
   assert(table->Viewport);

#ifdef _GLAPI_VERSION_1_1
   assert(table->AreTexturesResident);
   assert(table->ArrayElement);
   assert(table->BindTexture);
   assert(table->ColorPointer);
   assert(table->CopyTexImage1D);
   assert(table->CopyTexImage2D);
   assert(table->CopyTexSubImage1D);
   assert(table->CopyTexSubImage2D);
   assert(table->DeleteTextures);
   assert(table->DisableClientState);
   assert(table->DrawArrays);
   assert(table->EdgeFlagPointer);
   assert(table->EnableClientState);
   assert(table->GenTextures);
   assert(table->GetPointerv);
   assert(table->IndexPointer);
   assert(table->Indexub);
   assert(table->Indexubv);
   assert(table->InterleavedArrays);
   assert(table->IsTexture);
   assert(table->NormalPointer);
   assert(table->PopClientAttrib);
   assert(table->PrioritizeTextures);
   assert(table->PushClientAttrib);
   assert(table->TexCoordPointer);
   assert(table->TexSubImage1D);
   assert(table->TexSubImage2D);
   assert(table->VertexPointer);
#endif

#ifdef _GLAPI_VERSION_1_2
   assert(table->CopyTexSubImage3D);
   assert(table->DrawRangeElements);
   assert(table->TexImage3D);
   assert(table->TexSubImage3D);
#ifdef _GLAPI_ARB_imaging
   assert(table->BlendColor);
   assert(table->BlendEquation);
   assert(table->ColorSubTable);
   assert(table->ColorTable);
   assert(table->ColorTableParameterfv);
   assert(table->ColorTableParameteriv);
   assert(table->ConvolutionFilter1D);
   assert(table->ConvolutionFilter2D);
   assert(table->ConvolutionParameterf);
   assert(table->ConvolutionParameterfv);
   assert(table->ConvolutionParameteri);
   assert(table->ConvolutionParameteriv);
   assert(table->CopyColorSubTable);
   assert(table->CopyColorTable);
   assert(table->CopyConvolutionFilter1D);
   assert(table->CopyConvolutionFilter2D);
   assert(table->GetColorTable);
   assert(table->GetColorTableParameterfv);
   assert(table->GetColorTableParameteriv);
   assert(table->GetConvolutionFilter);
   assert(table->GetConvolutionParameterfv);
   assert(table->GetConvolutionParameteriv);
   assert(table->GetHistogram);
   assert(table->GetHistogramParameterfv);
   assert(table->GetHistogramParameteriv);
   assert(table->GetMinmax);
   assert(table->GetMinmaxParameterfv);
   assert(table->GetMinmaxParameteriv);
   assert(table->Histogram);
   assert(table->Minmax);
   assert(table->ResetHistogram);
   assert(table->ResetMinmax);
   assert(table->SeparableFilter2D);
#endif
#endif


#ifdef _GLAPI_EXT_paletted_texture
   assert(table->ColorTableEXT);
   assert(table->ColorSubTableEXT);
   assert(table->GetColorTableEXT);
   assert(table->GetColorTableParameterfvEXT);
   assert(table->GetColorTableParameterivEXT);
#endif

#ifdef _GLAPI_EXT_compiled_vertex_array
   assert(table->LockArraysEXT);
   assert(table->UnlockArraysEXT);
#endif

#ifdef _GLAPI_EXT_point_parameter
   assert(table->PointParameterfEXT);
   assert(table->PointParameterfvEXT);
#endif

#ifdef _GLAPI_EXT_polygon_offset
   assert(table->PolygonOffsetEXT);
#endif

#ifdef _GLAPI_ARB_multitexture
   assert(table->ActiveTextureARB);
   assert(table->ClientActiveTextureARB);
   assert(table->MultiTexCoord1dARB);
   assert(table->MultiTexCoord1dvARB);
   assert(table->MultiTexCoord1fARB);
   assert(table->MultiTexCoord1fvARB);
   assert(table->MultiTexCoord1iARB);
   assert(table->MultiTexCoord1ivARB);
   assert(table->MultiTexCoord1sARB);
   assert(table->MultiTexCoord1svARB);
   assert(table->MultiTexCoord2dARB);
   assert(table->MultiTexCoord2dvARB);
   assert(table->MultiTexCoord2fARB);
   assert(table->MultiTexCoord2fvARB);
   assert(table->MultiTexCoord2iARB);
   assert(table->MultiTexCoord2ivARB);
   assert(table->MultiTexCoord2sARB);
   assert(table->MultiTexCoord2svARB);
   assert(table->MultiTexCoord3dARB);
   assert(table->MultiTexCoord3dvARB);
   assert(table->MultiTexCoord3fARB);
   assert(table->MultiTexCoord3fvARB);
   assert(table->MultiTexCoord3iARB);
   assert(table->MultiTexCoord3ivARB);
   assert(table->MultiTexCoord3sARB);
   assert(table->MultiTexCoord3svARB);
   assert(table->MultiTexCoord4dARB);
   assert(table->MultiTexCoord4dvARB);
   assert(table->MultiTexCoord4fARB);
   assert(table->MultiTexCoord4fvARB);
   assert(table->MultiTexCoord4iARB);
   assert(table->MultiTexCoord4ivARB);
   assert(table->MultiTexCoord4sARB);
   assert(table->MultiTexCoord4svARB);
#endif

#ifdef _GLAPI_INGR_blend_func_separate
   assert(table->BlendFuncSeparateINGR);
#endif

#ifdef _GLAPI_MESA_window_pos
   assert(table->WindowPos4fMESA);
#endif

#ifdef _GLAPI_MESA_resize_buffers
   assert(table->ResizeBuffersMESA);
#endif
}



/*
 * Generate the GL entrypoint functions here.
 */

#define KEYWORD1
#define KEYWORD2 GLAPIENTRY
#ifdef USE_MGL_NAMESPACE
#define NAME(func)  mgl##func
#else
#define NAME(func)  gl##func
#endif

#include "glapitemp.h"

