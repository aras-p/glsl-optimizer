/* $Id: glapi.c,v 1.16 1999/12/16 17:33:43 brianp Exp $ */

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
#include "glapioffsets.h"
#include "glapitable.h"

#include <stdio.h>



/* Flag to indicate whether thread-safe dispatch is enabled */
static GLboolean ThreadSafe = GL_FALSE;

/* This is used when thread safety is disabled */
static struct _glapi_table *Dispatch = &__glapi_noop_table;


#if defined(THREADS)

#include "glthread.h"

static _glthread_TSD DispatchTSD;

static void dispatch_thread_init()
{
   _glthread_InitTSD(&DispatchTSD);
}

#endif



static GLuint MaxDispatchOffset = sizeof(struct _glapi_table) / sizeof(void *) - 1;
static GLboolean GetSizeCalled = GL_FALSE;



/*
 * We should call this periodically from a function such as glXMakeCurrent
 * in order to test if multiple threads are being used.  When we detect
 * that situation we should then call _glapi_enable_thread_safety()
 */
void
_glapi_check_multithread(void)
{
#if defined(THREADS)
   if (!ThreadSafe) {
      static unsigned long knownID;
      static GLboolean firstCall = GL_TRUE;
      if (firstCall) {
         knownID = _glthread_GetID();
         firstCall = GL_FALSE;
      }
      else if (knownID != _glthread_GetID()) {
         ThreadSafe = GL_TRUE;
      }
   }
   if (ThreadSafe) {
      /* make sure that this thread's dispatch pointer isn't null */
      if (!_glapi_get_dispatch()) {
         _glapi_set_dispatch(NULL);
      }
   }
#endif
}



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
   _glthread_SetTSD(&DispatchTSD, (void*) dispatch, dispatch_thread_init);
#else
   Dispatch = dispatch;
#endif
}



/*
 * Return pointer to current dispatch table for calling thread.
 */
struct _glapi_table *
_glapi_get_dispatch(void)
{
#if defined(THREADS)
   if (ThreadSafe) {
      return (struct _glapi_table *) _glthread_GetTSD(&DispatchTSD);
   }
   else
      return Dispatch;
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
   /*   return sizeof(struct _glapi_table) / sizeof(void *);*/
   GetSizeCalled = GL_TRUE;
   return MaxDispatchOffset + 1;
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


struct name_address_pair {
   const char *Name;
   GLvoid *Address;
};

static struct name_address_pair static_functions[1000];



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
 */


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
   GLint index;

   /* Make sure we don't try to add a new entrypoint after someone
    * has already called _glapi_get_dispatch_table_size()!  If that's
    * happened the caller's information will now be out of date.
    */
   assert(!GetSizeCalled);

   /* first check if the named function is already statically present */
   index = get_static_proc_offset(funcName);

   if (index >= 0) {
      assert(index == offset);
      return GL_TRUE;
   }
   /* else if (offset < _glapi_get_dispatch_table_size()) { */
   else {
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

      if (offset > MaxDispatchOffset)
         MaxDispatchOffset = offset;

      return GL_TRUE;      
   }
/*
   else {
      return GL_FALSE;
   }
*/
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
 * Return the name of the function at the given dispatch offset.
 * This is only intended for debugging.
 */
const char *
_glapi_get_proc_name(GLuint offset)
{
   GLuint n = sizeof(static_functions) / sizeof(struct name_address_pair);
   if (offset < n) {
      return static_functions[offset].Name;
   }
   else {
      /* search added extension functions */
      GLuint i;
      for (i = 0; i < NumExtEntryPoints; i++) {
         if (ExtEntryTable[i].Offset == offset) {
            return ExtEntryTable[i].Name;
         }
      }
      return NULL;
   }
}



/*
 * Make sure there are no NULL pointers in the given dispatch table.
 * Intented for debugging purposes.
 */
void
_glapi_check_table(const struct _glapi_table *table)
{
   const GLuint entries = _glapi_get_dispatch_table_size();
   const void **tab = (const void **) table;
   GLuint i;
   for (i = 1; i < entries; i++) {
      assert(tab[i]);
   }

   /* Do some spot checks to be sure that the dispatch table
    * slots are assigned correctly.
    */
   {
      GLuint BeginOffset = _glapi_get_proc_offset("glBegin");
      char *BeginFunc = (char*) &table->Begin;
      GLuint offset = (BeginFunc - (char *) table) / sizeof(void *);
      assert(BeginOffset == _gloffset_Begin);
      assert(BeginOffset == offset);
   }
   {
      GLuint viewportOffset = _glapi_get_proc_offset("glViewport");
      char *viewportFunc = (char*) &table->Viewport;
      GLuint offset = (viewportFunc - (char *) table) / sizeof(void *);
      assert(viewportOffset == _gloffset_Viewport);
      assert(viewportOffset == offset);
   }
   {
      GLuint VertexPointerOffset = _glapi_get_proc_offset("glVertexPointer");
      char *VertexPointerFunc = (char*) &table->VertexPointer;
      GLuint offset = (VertexPointerFunc - (char *) table) / sizeof(void *);
      assert(VertexPointerOffset == _gloffset_VertexPointer);
      assert(VertexPointerOffset == offset);
   }
   {
      GLuint ResetMinMaxOffset = _glapi_get_proc_offset("glResetMinmax");
      char *ResetMinMaxFunc = (char*) &table->ResetMinmax;
      GLuint offset = (ResetMinMaxFunc - (char *) table) / sizeof(void *);
      assert(ResetMinMaxOffset == _gloffset_ResetMinmax);
      assert(ResetMinMaxOffset == offset);
   }
   {
      GLuint blendColorOffset = _glapi_get_proc_offset("glBlendColorEXT");
      char *blendColorFunc = (char*) &table->BlendColorEXT;
      GLuint offset = (blendColorFunc - (char *) table) / sizeof(void *);
      assert(blendColorOffset == _gloffset_BlendColorEXT);
      assert(blendColorOffset == offset);
   }
   {
      GLuint istextureOffset = _glapi_get_proc_offset("glIsTextureEXT");
      char *istextureFunc = (char*) &table->IsTextureEXT;
      GLuint offset = (istextureFunc - (char *) table) / sizeof(void *);
      assert(istextureOffset == _gloffset_IsTextureEXT);
      assert(istextureOffset == offset);
   }
}



/**********************************************************************
 * Generate the GL entrypoint functions here.
 */

#define KEYWORD1
#define KEYWORD2 GLAPIENTRY
#ifdef USE_MGL_NAMESPACE
#define NAME(func)  mgl##func
#else
#define NAME(func)  gl##func
#endif

#define DISPATCH_SETUP			\
   const struct _glapi_table *dispatch;	\
   if (ThreadSafe) {			\
      dispatch = _glapi_get_dispatch();	\
      assert(dispatch);			\
   }					\
   else {				\
      dispatch = Dispatch;		\
   }

#define DISPATCH(FUNC, ARGS) (dispatch->FUNC) ARGS




#include "glapitemp.h"



/*
 * For each entry in static_functions[] which use this function
 * we should implement a dispatch function in glapitemp.h and
 * in glapinoop.c
 */
static void NotImplemented(void)
{
}



static struct name_address_pair static_functions[] = {
	{ "NotImplemented", (GLvoid *) NotImplemented },

	/* GL 1.1 */
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

	/* GL 1.1 */
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

	/* GL 1.2 */
	{ "glCopyTexSubImage3D", (GLvoid *) glCopyTexSubImage3D },
	{ "glDrawRangeElements", (GLvoid *) glDrawRangeElements },
	{ "glTexImage3D", (GLvoid *) glTexImage3D },
	{ "glTexSubImage3D", (GLvoid *) glTexSubImage3D },

	/* GL_ARB_imaging */
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

	/* GL_ARB_multitexture */
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

	/* 2. GL_EXT_blend_color */
	{ "glBlendColorEXT", (GLvoid *) glBlendColorEXT },

	/* 3. GL_EXT_polygon_offset */
	{ "glPolygonOffsetEXT", (GLvoid *) glPolygonOffsetEXT },

	/* 6. GL_EXT_texture3D */
	{ "glCopyTexSubImage3DEXT", (GLvoid *) glCopyTexSubImage3DEXT },
	{ "glTexImage3DEXT", (GLvoid *) glTexImage3DEXT },
	{ "glTexSubImage3DEXT", (GLvoid *) glTexSubImage3DEXT },

	/* 7. GL_SGI_texture_filter4 */
	{ "glGetTexFilterFuncSGIS", (GLvoid *) glGetTexFilterFuncSGIS },
	{ "glTexFilterFuncSGIS", (GLvoid *) glTexFilterFuncSGIS },

	/* 9. GL_EXT_subtexture */
	{ "glTexSubImage1DEXT", (GLvoid *) glTexSubImage1DEXT },
	{ "glTexSubImage2DEXT", (GLvoid *) glTexSubImage2DEXT },

	/* 10. GL_EXT_copy_texture */
	{ "glCopyTexImage1DEXT", (GLvoid *) glCopyTexImage1DEXT },
	{ "glCopyTexImage2DEXT", (GLvoid *) glCopyTexImage2DEXT },
	{ "glCopyTexSubImage1DEXT", (GLvoid *) glCopyTexSubImage1DEXT },
	{ "glCopyTexSubImage2DEXT", (GLvoid *) glCopyTexSubImage2DEXT },
                              
	/* 11. GL_EXT_histogram */
	{ "glGetHistogramEXT", (GLvoid *) glGetHistogramEXT },
	{ "glGetHistogramParameterfvEXT", (GLvoid *) glGetHistogramParameterfvEXT },
	{ "glGetHistogramParameterivEXT", (GLvoid *) glGetHistogramParameterivEXT },
	{ "glGetMinmaxEXT", (GLvoid *) glGetMinmaxEXT },
	{ "glGetMinmaxParameterfvEXT", (GLvoid *) glGetMinmaxParameterfvEXT },
	{ "glGetMinmaxParameterivEXT", (GLvoid *) glGetMinmaxParameterivEXT },
	{ "glHistogramEXT", (GLvoid *) glHistogramEXT },
	{ "glMinmaxEXT", (GLvoid *) glMinmaxEXT },
	{ "glResetHistogramEXT", (GLvoid *) glResetHistogramEXT },
	{ "glResetMinmaxEXT", (GLvoid *) glResetMinmaxEXT },

	/* 12. GL_EXT_convolution */
	{ "glConvolutionFilter1DEXT", (GLvoid *) glConvolutionFilter1DEXT },
	{ "glConvolutionFilter2DEXT", (GLvoid *) glConvolutionFilter2DEXT },
	{ "glConvolutionParameterfEXT", (GLvoid *) glConvolutionParameterfEXT },
	{ "glConvolutionParameterfvEXT", (GLvoid *) glConvolutionParameterfvEXT },
	{ "glConvolutionParameteriEXT", (GLvoid *) glConvolutionParameteriEXT },
	{ "glConvolutionParameterivEXT", (GLvoid *) glConvolutionParameterivEXT },
	{ "glCopyConvolutionFilter1DEXT", (GLvoid *) glCopyConvolutionFilter1DEXT },
	{ "glCopyConvolutionFilter2DEXT", (GLvoid *) glCopyConvolutionFilter2DEXT },
	{ "glGetConvolutionFilterEXT", (GLvoid *) glGetConvolutionFilterEXT },
	{ "glGetConvolutionParameterivEXT", (GLvoid *) glGetConvolutionParameterivEXT },
	{ "glGetConvolutionParameterfvEXT", (GLvoid *) glGetConvolutionParameterfvEXT },
	{ "glGetSeparableFilterEXT", (GLvoid *) glGetSeparableFilterEXT },
	{ "glSeparableFilter2DEXT", (GLvoid *) glSeparableFilter2DEXT },
                    
	/* 14. GL_SGI_color_table */
	{ "glColorTableSGI", (GLvoid *) NotImplemented },
	{ "glColorTableParameterfvSGI", (GLvoid *) NotImplemented },
	{ "glColorTableParameterivSGI", (GLvoid *) NotImplemented },
	{ "glCopyColorTableSGI", (GLvoid *) NotImplemented },
	{ "glGetColorTableSGI", (GLvoid *) NotImplemented },
	{ "glGetColorTableParameterfvSGI", (GLvoid *) NotImplemented },
	{ "glGetColorTableParameterivSGI", (GLvoid *) NotImplemented },

	/* 15. GL_SGIS_pixel_texture */
	{ "glPixelTexGenParameterfSGIS", (GLvoid *) NotImplemented },
	{ "glPixelTexGenParameteriSGIS", (GLvoid *) NotImplemented },
	{ "glGetPixelTexGenParameterfvSGIS", (GLvoid *) NotImplemented },
	{ "glGetPixelTexGenParameterivSGIS", (GLvoid *) NotImplemented },

	/* 16. GL_SGIS_texture4D */
	{ "glTexImage4DSGIS", (GLvoid *) NotImplemented },
	{ "glTexSubImage4DSGIS", (GLvoid *) NotImplemented },

	/* 20. GL_EXT_texture_object */
	{ "glAreTexturesResidentEXT", (GLvoid *) glAreTexturesResidentEXT },
	{ "glBindTextureEXT", (GLvoid *) glBindTextureEXT },
	{ "glDeleteTexturesEXT", (GLvoid *) glDeleteTexturesEXT },
	{ "glGenTexturesEXT", (GLvoid *) glGenTexturesEXT },
	{ "glIsTextureEXT", (GLvoid *) glIsTextureEXT },
	{ "glPrioritizeTexturesEXT", (GLvoid *) glPrioritizeTexturesEXT },

	/* 21. GL_SGIS_detail_texture */
	{ "glDetailTexFuncSGIS", (GLvoid *) NotImplemented },
	{ "glGetDetailTexFuncSGIS", (GLvoid *) NotImplemented },

	/* 22. GL_SGIS_sharpen_texture */
	{ "glGetSharpenTexFuncSGIS", (GLvoid *) NotImplemented },
	{ "glSharpenTexFuncSGIS", (GLvoid *) NotImplemented },

	/* 25. GL_SGIS_multisample */
	{ "glSampleMaskSGIS", (GLvoid *) NotImplemented },
	{ "glSamplePatternSGIS", (GLvoid *) NotImplemented },

	/* 30. GL_EXT_vertex_array */
	{ "glArrayElementEXT", (GLvoid *) glArrayElementEXT },
	{ "glColorPointerEXT", (GLvoid *) glColorPointerEXT },
	{ "glDrawArraysEXT", (GLvoid *) glDrawArraysEXT },
	{ "glEdgeFlagPointerEXT", (GLvoid *) glEdgeFlagPointerEXT },
	{ "glGetPointervEXT", (GLvoid *) glGetPointervEXT },
	{ "glIndexPointerEXT", (GLvoid *) glIndexPointerEXT },
	{ "glNormalPointerEXT", (GLvoid *) glNormalPointerEXT },
	{ "glTexCoordPointerEXT", (GLvoid *) glTexCoordPointerEXT },
	{ "glVertexPointerEXT", (GLvoid *) glVertexPointerEXT },

	/* 37. GL_EXT_blend_minmax */
	{ "glBlendEquationEXT", (GLvoid *) glBlendEquationEXT },

	/* 52. GL_SGIX_sprite */
	{ "glSpriteParameterfSGIX", (GLvoid *) NotImplemented },
	{ "glSpriteParameterfvSGIX", (GLvoid *) NotImplemented },
	{ "glSpriteParameteriSGIX", (GLvoid *) NotImplemented },
	{ "glSpriteParameterivSGIX", (GLvoid *) NotImplemented },

	/* 54. GL_EXT_point_parameters */
	{ "glPointParameterfEXT", (GLvoid *) NotImplemented },
	{ "glPointParameterfvEXT", (GLvoid *) NotImplemented },

	/* 55. GL_SGIX_instruments */
	{ "glInstrumentsBufferSGIX", (GLvoid *) NotImplemented },
	{ "glStartInstrumentsSGIX", (GLvoid *) NotImplemented },
	{ "glStopInstrumentsSGIX", (GLvoid *) NotImplemented },
	{ "glReadInstrumentsSGIX", (GLvoid *) NotImplemented },
	{ "glPollInstrumentsSGIX", (GLvoid *) NotImplemented },
	{ "glGetInstrumentsSGIX", (GLvoid *) NotImplemented },

	/* 57. GL_SGIX_framezoom */
	{ "glFrameZoomSGIX", (GLvoid *) NotImplemented },

	/* 60. GL_SGIX_reference_plane */
	{ "glReferencePlaneSGIX", (GLvoid *) NotImplemented },

	/* 61. GL_SGIX_flush_raster */
	{ "glFlushRasterSGIX", (GLvoid *) NotImplemented },

	/* 66. GL_HP_image_transform */
	{ "glGetImageTransformParameterfvHP", (GLvoid *) NotImplemented },
	{ "glGetImageTransformParameterivHP", (GLvoid *) NotImplemented },
	{ "glImageTransformParameterfHP", (GLvoid *) NotImplemented },
	{ "glImageTransformParameterfvHP", (GLvoid *) NotImplemented },
	{ "glImageTransformParameteriHP", (GLvoid *) NotImplemented },
	{ "glImageTransformParameterivHP", (GLvoid *) NotImplemented },

	/* 74. GL_EXT_color_subtable */
	{ "glColorSubTableEXT", (GLvoid *) NotImplemented },
	{ "glCopyColorSubTableEXT", (GLvoid *) NotImplemented },

	/* 77. GL_PGI_misc_hints */
	{ "glHintPGI", (GLvoid *) NotImplemented },

	/* 78. GL_EXT_paletted_texture */
	{ "glColorTableEXT", (GLvoid *) glColorTableEXT },
	{ "glGetColorTableEXT", (GLvoid *) glGetColorTableEXT },
	{ "glGetColorTableParameterfvEXT", (GLvoid *) glGetColorTableParameterfvEXT },
	{ "glGetColorTableParameterivEXT", (GLvoid *) glGetColorTableParameterivEXT },

	/* 80. GL_SGIX_list_priority */
	{ "glGetListParameterfvSGIX", (GLvoid *) NotImplemented },
	{ "glGetListParameterivSGIX", (GLvoid *) NotImplemented },
	{ "glListParameterfSGIX", (GLvoid *) NotImplemented },
	{ "glListParameterfvSGIX", (GLvoid *) NotImplemented },
	{ "glListParameteriSGIX", (GLvoid *) NotImplemented },
	{ "glListParameterivSGIX", (GLvoid *) NotImplemented },

	/* 94. GL_EXT_index_material */
	{ "glIndexMaterialEXT", (GLvoid *) NotImplemented },

	/* 95. GL_EXT_index_func */
	{ "glIndexFuncEXT", (GLvoid *) NotImplemented },

	/* 97. GL_EXT_compiled_vertex_array */
	{ "glLockArraysEXT", (GLvoid *) glLockArraysEXT },
	{ "glUnlockArraysEXT", (GLvoid *) glUnlockArraysEXT },

	/* 98. GL_EXT_cull_vertex */
	{ "glCullParameterfvEXT", (GLvoid *) NotImplemented },
	{ "glCullParameterdvEXT", (GLvoid *) NotImplemented },

	/* 173. GL_EXT/INGR_blend_func_separate */
	{ "glBlendFuncSeparateINGR", (GLvoid *) glBlendFuncSeparateINGR },

	/* GL_MESA_window_pos */
	{ "glWindowPos4fMESA", (GLvoid *) glWindowPos4fMESA },

	/* GL_MESA_resize_buffers */
	{ "glResizeBuffersMESA", (GLvoid *) glResizeBuffersMESA },

	/* GL_ARB_transpose_matrix */
	{ "glLoadTransposeMatrixdARB", (GLvoid *) glLoadTransposeMatrixdARB },
	{ "glLoadTransposeMatrixfARB", (GLvoid *) glLoadTransposeMatrixfARB },
	{ "glMultTransposeMatrixdARB", (GLvoid *) glMultTransposeMatrixdARB },
	{ "glMultTransposeMatrixfARB", (GLvoid *) glMultTransposeMatrixfARB },

	{ NULL, NULL }  /* end of list marker */
};

