/* $Id: glapi.c,v 1.56 2001/06/06 22:55:28 davem69 Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
 *
 * It's intended that this file and the other glapi*.[ch] files are
 * flexible enough to be reused in several places:  XFree86, DRI-
 * based libGL.so, and perhaps the SGI SI.
 *
 * There are no dependencies on Mesa in this code.
 *
 * Versions (API changes):
 *   2000/02/23  - original version for Mesa 3.3 and XFree86 4.0
 *   2001/01/16  - added dispatch override feature for Mesa 3.5
 */



#include "glheader.h"
#include "glapi.h"
#include "glapioffsets.h"
#include "glapitable.h"
#include "glthread.h"

/***** BEGIN NO-OP DISPATCH *****/

static GLboolean WarnFlag = GL_FALSE;

void
_glapi_noop_enable_warnings(GLboolean enable)
{
   WarnFlag = enable;
}

static GLboolean
warn(void)
{
   if (WarnFlag || getenv("MESA_DEBUG") || getenv("LIBGL_DEBUG"))
      return GL_TRUE;
   else
      return GL_FALSE;
}


#define KEYWORD1 static
#define KEYWORD2
#define NAME(func)  NoOp##func

#define F stderr

#define DISPATCH(func, args, msg)			\
   if (warn()) {					\
      fprintf(stderr, "GL User Error: calling ");	\
      fprintf msg;					\
      fprintf(stderr, " without a current context\n");	\
   }

#define RETURN_DISPATCH(func, args, msg)		\
   if (warn()) {					\
      fprintf(stderr, "GL User Error: calling ");	\
      fprintf msg;					\
      fprintf(stderr, " without a current context\n");	\
   }							\
   return 0

#define DISPATCH_TABLE_NAME __glapi_noop_table
#define UNUSED_TABLE_NAME __usused_noop_functions

#define TABLE_ENTRY(name) (void *) NoOp##name

static int NoOpUnused(void)
{
   if (warn()) {
      fprintf(stderr, "GL User Error: calling extension function without a current context\n");
   }
   return 0;
}

#include "glapitemp.h"

/***** END NO-OP DISPATCH *****/



/***** BEGIN THREAD-SAFE DISPATCH *****/
/* if we support thread-safety, build a special dispatch table for use
 * in thread-safety mode (ThreadSafe == GL_TRUE).  Each entry in the
 * dispatch table will call _glthread_GetTSD() to get the actual dispatch
 * table bound to the current thread, then jump through that table.
 */

#if defined(THREADS)

static GLboolean ThreadSafe = GL_FALSE;  /* In thread-safe mode? */
static _glthread_TSD DispatchTSD;        /* Per-thread dispatch pointer */
static _glthread_TSD RealDispatchTSD;    /* only when using override */
static _glthread_TSD ContextTSD;         /* Per-thread context pointer */


#define KEYWORD1 static
#define KEYWORD2 GLAPIENTRY
#define NAME(func)  _ts_##func

#define DISPATCH(FUNC, ARGS, MESSAGE)					\
   struct _glapi_table *dispatch;					\
   dispatch = (struct _glapi_table *) _glthread_GetTSD(&DispatchTSD);	\
   if (!dispatch)							\
      dispatch = (struct _glapi_table *) __glapi_noop_table;		\
   (dispatch->FUNC) ARGS

#define RETURN_DISPATCH(FUNC, ARGS, MESSAGE) 				\
   struct _glapi_table *dispatch;					\
   dispatch = (struct _glapi_table *) _glthread_GetTSD(&DispatchTSD);	\
   if (!dispatch)							\
      dispatch = (struct _glapi_table *) __glapi_noop_table;		\
   return (dispatch->FUNC) ARGS

#define DISPATCH_TABLE_NAME __glapi_threadsafe_table
#define UNUSED_TABLE_NAME __usused_threadsafe_functions

#define TABLE_ENTRY(name) (void *) _ts_##name

static int _ts_Unused(void)
{
   return 0;
}

#include "glapitemp.h"

#endif

/***** END THREAD-SAFE DISPATCH *****/



struct _glapi_table *_glapi_Dispatch = (struct _glapi_table *) __glapi_noop_table;
struct _glapi_table *_glapi_RealDispatch = (struct _glapi_table *) __glapi_noop_table;

/* Used when thread safety disabled */
void *_glapi_Context = NULL;


static GLuint MaxDispatchOffset = sizeof(struct _glapi_table) / sizeof(void *) - 1;
static GLboolean GetSizeCalled = GL_FALSE;

static GLboolean DispatchOverride = GL_FALSE;


/* strdup() is actually not a standard ANSI C or POSIX routine.
 * Irix will not define it if ANSI mode is in effect.
 */
static char *
str_dup(const char *str)
{
   char *copy;
   copy = (char*) malloc(strlen(str) + 1);
   if (!copy)
      return NULL;
   strcpy(copy, str);
   return copy;
}



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
 * Set the current context pointer for this thread.
 * The context pointer is an opaque type which should be cast to
 * void from the real context pointer type.
 */
void
_glapi_set_context(void *context)
{
#if defined(THREADS)
   _glthread_SetTSD(&ContextTSD, context);
   if (ThreadSafe)
      _glapi_Context = NULL;
   else
      _glapi_Context = context;
#else
   _glapi_Context = context;
#endif
}



/*
 * Get the current context pointer for this thread.
 * The context pointer is an opaque type which should be cast from
 * void to the real context pointer type.
 */
void *
_glapi_get_context(void)
{
#if defined(THREADS)
   if (ThreadSafe) {
      return _glthread_GetTSD(&ContextTSD);
   }
   else {
      return _glapi_Context;
   }
#else
   return _glapi_Context;
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
      dispatch = (struct _glapi_table *) __glapi_noop_table;
   }
#ifdef DEBUG
   else {
      _glapi_check_table(dispatch);
   }
#endif

#if defined(THREADS)
   if (DispatchOverride) {
      _glthread_SetTSD(&RealDispatchTSD, (void *) dispatch);
      if (ThreadSafe)
         _glapi_RealDispatch = (struct _glapi_table*) __glapi_threadsafe_table;
      else
         _glapi_RealDispatch = dispatch;
   }
   else {
      /* normal operation */
      _glthread_SetTSD(&DispatchTSD, (void *) dispatch);
      if (ThreadSafe)
         _glapi_Dispatch = (struct _glapi_table *) __glapi_threadsafe_table;
      else
         _glapi_Dispatch = dispatch;
   }
#else /*THREADS*/
   if (DispatchOverride) {
      _glapi_RealDispatch = dispatch;
   }
   else {
      _glapi_Dispatch = dispatch;
   }
#endif /*THREADS*/
}



/*
 * Return pointer to current dispatch table for calling thread.
 */
struct _glapi_table *
_glapi_get_dispatch(void)
{
#if defined(THREADS)
   if (ThreadSafe) {
      if (DispatchOverride) {
         return (struct _glapi_table *) _glthread_GetTSD(&RealDispatchTSD);
      }
      else {
         return (struct _glapi_table *) _glthread_GetTSD(&DispatchTSD);
      }
   }
   else {
      if (DispatchOverride) {
         assert(_glapi_RealDispatch);
         return _glapi_RealDispatch;
      }
      else {
         assert(_glapi_Dispatch);
         return _glapi_Dispatch;
      }
   }
#else
   return _glapi_Dispatch;
#endif
}


/*
 * Notes on dispatch overrride:
 *
 * Dispatch override allows an external agent to hook into the GL dispatch
 * mechanism before execution goes into the core rendering library.  For
 * example, a trace mechanism would insert itself as an overrider, print
 * logging info for each GL function, then dispatch to the real GL function.
 *
 * libGLS (GL Stream library) is another agent that might use override.
 *
 * We don't allow more than one layer of overriding at this time.
 * In the future we may allow nested/layered override.  In that case
 * _glapi_begin_dispatch_override() will return an override layer,
 * _glapi_end_dispatch_override(layer) will remove an override layer
 * and _glapi_get_override_dispatch(layer) will return the dispatch
 * table for a given override layer.  layer = 0 will be the "real"
 * dispatch table.
 */

/*
 * Return: dispatch override layer number.
 */
int
_glapi_begin_dispatch_override(struct _glapi_table *override)
{
   struct _glapi_table *real = _glapi_get_dispatch();

   assert(!DispatchOverride);  /* can't nest at this time */
   DispatchOverride = GL_TRUE;

   _glapi_set_dispatch(real);

#if defined(THREADS)
   _glthread_SetTSD(&DispatchTSD, (void *) override);
   if (ThreadSafe)
      _glapi_Dispatch = (struct _glapi_table *) __glapi_threadsafe_table;
   else
      _glapi_Dispatch = override;
#else
   _glapi_Dispatch = override;
#endif
   return 1;
}


void
_glapi_end_dispatch_override(int layer)
{
   struct _glapi_table *real = _glapi_get_dispatch();
   (void) layer;
   DispatchOverride = GL_FALSE;
   _glapi_set_dispatch(real);
   /* the rest of this isn't needed, just play it safe */
#if defined(THREADS)
   _glthread_SetTSD(&RealDispatchTSD, NULL);
#endif
   _glapi_RealDispatch = NULL;
}


struct _glapi_table *
_glapi_get_override_dispatch(int layer)
{
   if (layer == 0) {
      return _glapi_get_dispatch();
   }
   else {
      if (DispatchOverride) {
#if defined(THREADS)
         return (struct _glapi_table *) _glthread_GetTSD(&DispatchTSD);
#else
         return _glapi_Dispatch;
#endif
      }
      else {
         return NULL;
      }
   }
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
 */
const char *
_glapi_get_version(void)
{
   return "20010116";  /* YYYYMMDD */
}


/*
 * For each entry in static_functions[] which use this function
 * we should implement a dispatch function in glapitemp.h and
 * in glapinoop.c
 */
static int NotImplemented(void)
{
   return 0;
}


struct name_address_offset {
   const char *Name;
   GLvoid *Address;
   GLuint Offset;
};


static struct name_address_offset static_functions[] = {
	/* GL 1.1 */
        { "glNewList", (GLvoid *) glNewList, _gloffset_NewList },
        { "glEndList", (GLvoid *) glEndList, _gloffset_EndList },
        { "glCallList", (GLvoid *) glCallList, _gloffset_CallList },
        { "glCallLists", (GLvoid *) glCallLists, _gloffset_CallLists },
        { "glDeleteLists", (GLvoid *) glDeleteLists, _gloffset_DeleteLists },
        { "glGenLists", (GLvoid *) glGenLists, _gloffset_GenLists },
        { "glListBase", (GLvoid *) glListBase, _gloffset_ListBase },
        { "glBegin", (GLvoid *) glBegin, _gloffset_Begin },
        { "glBitmap", (GLvoid *) glBitmap, _gloffset_Bitmap },
        { "glColor3b", (GLvoid *) glColor3b, _gloffset_Color3b },
        { "glColor3bv", (GLvoid *) glColor3bv, _gloffset_Color3bv },
        { "glColor3d", (GLvoid *) glColor3d, _gloffset_Color3d },
        { "glColor3dv", (GLvoid *) glColor3dv, _gloffset_Color3dv },
        { "glColor3f", (GLvoid *) glColor3f, _gloffset_Color3f },
        { "glColor3fv", (GLvoid *) glColor3fv, _gloffset_Color3fv },
        { "glColor3i", (GLvoid *) glColor3i, _gloffset_Color3i },
        { "glColor3iv", (GLvoid *) glColor3iv, _gloffset_Color3iv },
        { "glColor3s", (GLvoid *) glColor3s, _gloffset_Color3s },
        { "glColor3sv", (GLvoid *) glColor3sv, _gloffset_Color3sv },
        { "glColor3ub", (GLvoid *) glColor3ub, _gloffset_Color3ub },
        { "glColor3ubv", (GLvoid *) glColor3ubv, _gloffset_Color3ubv },
        { "glColor3ui", (GLvoid *) glColor3ui, _gloffset_Color3ui },
        { "glColor3uiv", (GLvoid *) glColor3uiv, _gloffset_Color3uiv },
        { "glColor3us", (GLvoid *) glColor3us, _gloffset_Color3us },
        { "glColor3usv", (GLvoid *) glColor3usv, _gloffset_Color3usv },
        { "glColor4b", (GLvoid *) glColor4b, _gloffset_Color4b },
        { "glColor4bv", (GLvoid *) glColor4bv, _gloffset_Color4bv },
        { "glColor4d", (GLvoid *) glColor4d, _gloffset_Color4d },
        { "glColor4dv", (GLvoid *) glColor4dv, _gloffset_Color4dv },
        { "glColor4f", (GLvoid *) glColor4f, _gloffset_Color4f },
        { "glColor4fv", (GLvoid *) glColor4fv, _gloffset_Color4fv },
        { "glColor4i", (GLvoid *) glColor4i, _gloffset_Color4i },
        { "glColor4iv", (GLvoid *) glColor4iv, _gloffset_Color4iv },
        { "glColor4s", (GLvoid *) glColor4s, _gloffset_Color4s },
        { "glColor4sv", (GLvoid *) glColor4sv, _gloffset_Color4sv },
        { "glColor4ub", (GLvoid *) glColor4ub, _gloffset_Color4ub },
        { "glColor4ubv", (GLvoid *) glColor4ubv, _gloffset_Color4ubv },
        { "glColor4ui", (GLvoid *) glColor4ui, _gloffset_Color4ui },
        { "glColor4uiv", (GLvoid *) glColor4uiv, _gloffset_Color4uiv },
        { "glColor4us", (GLvoid *) glColor4us, _gloffset_Color4us },
        { "glColor4usv", (GLvoid *) glColor4usv, _gloffset_Color4usv },
        { "glEdgeFlag", (GLvoid *) glEdgeFlag, _gloffset_EdgeFlag },
        { "glEdgeFlagv", (GLvoid *) glEdgeFlagv, _gloffset_EdgeFlagv },
        { "glEnd", (GLvoid *) glEnd, _gloffset_End },
        { "glIndexd", (GLvoid *) glIndexd, _gloffset_Indexd },
        { "glIndexdv", (GLvoid *) glIndexdv, _gloffset_Indexdv },
        { "glIndexf", (GLvoid *) glIndexf, _gloffset_Indexf },
        { "glIndexfv", (GLvoid *) glIndexfv, _gloffset_Indexfv },
        { "glIndexi", (GLvoid *) glIndexi, _gloffset_Indexi },
        { "glIndexiv", (GLvoid *) glIndexiv, _gloffset_Indexiv },
        { "glIndexs", (GLvoid *) glIndexs, _gloffset_Indexs },
        { "glIndexsv", (GLvoid *) glIndexsv, _gloffset_Indexsv },
        { "glNormal3b", (GLvoid *) glNormal3b, _gloffset_Normal3b },
        { "glNormal3bv", (GLvoid *) glNormal3bv, _gloffset_Normal3bv },
        { "glNormal3d", (GLvoid *) glNormal3d, _gloffset_Normal3d },
        { "glNormal3dv", (GLvoid *) glNormal3dv, _gloffset_Normal3dv },
        { "glNormal3f", (GLvoid *) glNormal3f, _gloffset_Normal3f },
        { "glNormal3fv", (GLvoid *) glNormal3fv, _gloffset_Normal3fv },
        { "glNormal3i", (GLvoid *) glNormal3i, _gloffset_Normal3i },
        { "glNormal3iv", (GLvoid *) glNormal3iv, _gloffset_Normal3iv },
        { "glNormal3s", (GLvoid *) glNormal3s, _gloffset_Normal3s },
        { "glNormal3sv", (GLvoid *) glNormal3sv, _gloffset_Normal3sv },
        { "glRasterPos2d", (GLvoid *) glRasterPos2d, _gloffset_RasterPos2d },
        { "glRasterPos2dv", (GLvoid *) glRasterPos2dv, _gloffset_RasterPos2dv },
        { "glRasterPos2f", (GLvoid *) glRasterPos2f, _gloffset_RasterPos2f },
        { "glRasterPos2fv", (GLvoid *) glRasterPos2fv, _gloffset_RasterPos2fv },
        { "glRasterPos2i", (GLvoid *) glRasterPos2i, _gloffset_RasterPos2i },
        { "glRasterPos2iv", (GLvoid *) glRasterPos2iv, _gloffset_RasterPos2iv },
        { "glRasterPos2s", (GLvoid *) glRasterPos2s, _gloffset_RasterPos2s },
        { "glRasterPos2sv", (GLvoid *) glRasterPos2sv, _gloffset_RasterPos2sv },
        { "glRasterPos3d", (GLvoid *) glRasterPos3d, _gloffset_RasterPos3d },
        { "glRasterPos3dv", (GLvoid *) glRasterPos3dv, _gloffset_RasterPos3dv },
        { "glRasterPos3f", (GLvoid *) glRasterPos3f, _gloffset_RasterPos3f },
        { "glRasterPos3fv", (GLvoid *) glRasterPos3fv, _gloffset_RasterPos3fv },
        { "glRasterPos3i", (GLvoid *) glRasterPos3i, _gloffset_RasterPos3i },
        { "glRasterPos3iv", (GLvoid *) glRasterPos3iv, _gloffset_RasterPos3iv },
        { "glRasterPos3s", (GLvoid *) glRasterPos3s, _gloffset_RasterPos3s },
        { "glRasterPos3sv", (GLvoid *) glRasterPos3sv, _gloffset_RasterPos3sv },
        { "glRasterPos4d", (GLvoid *) glRasterPos4d, _gloffset_RasterPos4d },
        { "glRasterPos4dv", (GLvoid *) glRasterPos4dv, _gloffset_RasterPos4dv },
        { "glRasterPos4f", (GLvoid *) glRasterPos4f, _gloffset_RasterPos4f },
        { "glRasterPos4fv", (GLvoid *) glRasterPos4fv, _gloffset_RasterPos4fv },
        { "glRasterPos4i", (GLvoid *) glRasterPos4i, _gloffset_RasterPos4i },
        { "glRasterPos4iv", (GLvoid *) glRasterPos4iv, _gloffset_RasterPos4iv },
        { "glRasterPos4s", (GLvoid *) glRasterPos4s, _gloffset_RasterPos4s },
        { "glRasterPos4sv", (GLvoid *) glRasterPos4sv, _gloffset_RasterPos4sv },
        { "glRectd", (GLvoid *) glRectd, _gloffset_Rectd },
        { "glRectdv", (GLvoid *) glRectdv, _gloffset_Rectdv },
        { "glRectf", (GLvoid *) glRectf, _gloffset_Rectf },
        { "glRectfv", (GLvoid *) glRectfv, _gloffset_Rectfv },
        { "glRecti", (GLvoid *) glRecti, _gloffset_Recti },
        { "glRectiv", (GLvoid *) glRectiv, _gloffset_Rectiv },
        { "glRects", (GLvoid *) glRects, _gloffset_Rects },
        { "glRectsv", (GLvoid *) glRectsv, _gloffset_Rectsv },
        { "glTexCoord1d", (GLvoid *) glTexCoord1d, _gloffset_TexCoord1d },
        { "glTexCoord1dv", (GLvoid *) glTexCoord1dv, _gloffset_TexCoord1dv },
        { "glTexCoord1f", (GLvoid *) glTexCoord1f, _gloffset_TexCoord1f },
        { "glTexCoord1fv", (GLvoid *) glTexCoord1fv, _gloffset_TexCoord1fv },
        { "glTexCoord1i", (GLvoid *) glTexCoord1i, _gloffset_TexCoord1i },
        { "glTexCoord1iv", (GLvoid *) glTexCoord1iv, _gloffset_TexCoord1iv },
        { "glTexCoord1s", (GLvoid *) glTexCoord1s, _gloffset_TexCoord1s },
        { "glTexCoord1sv", (GLvoid *) glTexCoord1sv, _gloffset_TexCoord1sv },
        { "glTexCoord2d", (GLvoid *) glTexCoord2d, _gloffset_TexCoord2d },
        { "glTexCoord2dv", (GLvoid *) glTexCoord2dv, _gloffset_TexCoord2dv },
        { "glTexCoord2f", (GLvoid *) glTexCoord2f, _gloffset_TexCoord2f },
        { "glTexCoord2fv", (GLvoid *) glTexCoord2fv, _gloffset_TexCoord2fv },
        { "glTexCoord2i", (GLvoid *) glTexCoord2i, _gloffset_TexCoord2i },
        { "glTexCoord2iv", (GLvoid *) glTexCoord2iv, _gloffset_TexCoord2iv },
        { "glTexCoord2s", (GLvoid *) glTexCoord2s, _gloffset_TexCoord2s },
        { "glTexCoord2sv", (GLvoid *) glTexCoord2sv, _gloffset_TexCoord2sv },
        { "glTexCoord3d", (GLvoid *) glTexCoord3d, _gloffset_TexCoord3d },
        { "glTexCoord3dv", (GLvoid *) glTexCoord3dv, _gloffset_TexCoord3dv },
        { "glTexCoord3f", (GLvoid *) glTexCoord3f, _gloffset_TexCoord3f },
        { "glTexCoord3fv", (GLvoid *) glTexCoord3fv, _gloffset_TexCoord3fv },
        { "glTexCoord3i", (GLvoid *) glTexCoord3i, _gloffset_TexCoord3i },
        { "glTexCoord3iv", (GLvoid *) glTexCoord3iv, _gloffset_TexCoord3iv },
        { "glTexCoord3s", (GLvoid *) glTexCoord3s, _gloffset_TexCoord3s },
        { "glTexCoord3sv", (GLvoid *) glTexCoord3sv, _gloffset_TexCoord3sv },
        { "glTexCoord4d", (GLvoid *) glTexCoord4d, _gloffset_TexCoord4d },
        { "glTexCoord4dv", (GLvoid *) glTexCoord4dv, _gloffset_TexCoord4dv },
        { "glTexCoord4f", (GLvoid *) glTexCoord4f, _gloffset_TexCoord4f },
        { "glTexCoord4fv", (GLvoid *) glTexCoord4fv, _gloffset_TexCoord4fv },
        { "glTexCoord4i", (GLvoid *) glTexCoord4i, _gloffset_TexCoord4i },
        { "glTexCoord4iv", (GLvoid *) glTexCoord4iv, _gloffset_TexCoord4iv },
        { "glTexCoord4s", (GLvoid *) glTexCoord4s, _gloffset_TexCoord4s },
        { "glTexCoord4sv", (GLvoid *) glTexCoord4sv, _gloffset_TexCoord4sv },
        { "glVertex2d", (GLvoid *) glVertex2d, _gloffset_Vertex2d },
        { "glVertex2dv", (GLvoid *) glVertex2dv, _gloffset_Vertex2dv },
        { "glVertex2f", (GLvoid *) glVertex2f, _gloffset_Vertex2f },
        { "glVertex2fv", (GLvoid *) glVertex2fv, _gloffset_Vertex2fv },
        { "glVertex2i", (GLvoid *) glVertex2i, _gloffset_Vertex2i },
        { "glVertex2iv", (GLvoid *) glVertex2iv, _gloffset_Vertex2iv },
        { "glVertex2s", (GLvoid *) glVertex2s, _gloffset_Vertex2s },
        { "glVertex2sv", (GLvoid *) glVertex2sv, _gloffset_Vertex2sv },
        { "glVertex3d", (GLvoid *) glVertex3d, _gloffset_Vertex3d },
        { "glVertex3dv", (GLvoid *) glVertex3dv, _gloffset_Vertex3dv },
        { "glVertex3f", (GLvoid *) glVertex3f, _gloffset_Vertex3f },
        { "glVertex3fv", (GLvoid *) glVertex3fv, _gloffset_Vertex3fv },
        { "glVertex3i", (GLvoid *) glVertex3i, _gloffset_Vertex3i },
        { "glVertex3iv", (GLvoid *) glVertex3iv, _gloffset_Vertex3iv },
        { "glVertex3s", (GLvoid *) glVertex3s, _gloffset_Vertex3s },
        { "glVertex3sv", (GLvoid *) glVertex3sv, _gloffset_Vertex3sv },
        { "glVertex4d", (GLvoid *) glVertex4d, _gloffset_Vertex4d },
        { "glVertex4dv", (GLvoid *) glVertex4dv, _gloffset_Vertex4dv },
        { "glVertex4f", (GLvoid *) glVertex4f, _gloffset_Vertex4f },
        { "glVertex4fv", (GLvoid *) glVertex4fv, _gloffset_Vertex4fv },
        { "glVertex4i", (GLvoid *) glVertex4i, _gloffset_Vertex4i },
        { "glVertex4iv", (GLvoid *) glVertex4iv, _gloffset_Vertex4iv },
        { "glVertex4s", (GLvoid *) glVertex4s, _gloffset_Vertex4s },
        { "glVertex4sv", (GLvoid *) glVertex4sv, _gloffset_Vertex4sv },
        { "glClipPlane", (GLvoid *) glClipPlane, _gloffset_ClipPlane },
        { "glColorMaterial", (GLvoid *) glColorMaterial, _gloffset_ColorMaterial },
        { "glCullFace", (GLvoid *) glCullFace, _gloffset_CullFace },
        { "glFogf", (GLvoid *) glFogf, _gloffset_Fogf },
        { "glFogfv", (GLvoid *) glFogfv, _gloffset_Fogfv },
        { "glFogi", (GLvoid *) glFogi, _gloffset_Fogi },
        { "glFogiv", (GLvoid *) glFogiv, _gloffset_Fogiv },
        { "glFrontFace", (GLvoid *) glFrontFace, _gloffset_FrontFace },
        { "glHint", (GLvoid *) glHint, _gloffset_Hint },
        { "glLightf", (GLvoid *) glLightf, _gloffset_Lightf },
        { "glLightfv", (GLvoid *) glLightfv, _gloffset_Lightfv },
        { "glLighti", (GLvoid *) glLighti, _gloffset_Lighti },
        { "glLightiv", (GLvoid *) glLightiv, _gloffset_Lightiv },
        { "glLightModelf", (GLvoid *) glLightModelf, _gloffset_LightModelf },
        { "glLightModelfv", (GLvoid *) glLightModelfv, _gloffset_LightModelfv },
        { "glLightModeli", (GLvoid *) glLightModeli, _gloffset_LightModeli },
        { "glLightModeliv", (GLvoid *) glLightModeliv, _gloffset_LightModeliv },
        { "glLineStipple", (GLvoid *) glLineStipple, _gloffset_LineStipple },
        { "glLineWidth", (GLvoid *) glLineWidth, _gloffset_LineWidth },
        { "glMaterialf", (GLvoid *) glMaterialf, _gloffset_Materialf },
        { "glMaterialfv", (GLvoid *) glMaterialfv, _gloffset_Materialfv },
        { "glMateriali", (GLvoid *) glMateriali, _gloffset_Materiali },
        { "glMaterialiv", (GLvoid *) glMaterialiv, _gloffset_Materialiv },
        { "glPointSize", (GLvoid *) glPointSize, _gloffset_PointSize },
        { "glPolygonMode", (GLvoid *) glPolygonMode, _gloffset_PolygonMode },
        { "glPolygonStipple", (GLvoid *) glPolygonStipple, _gloffset_PolygonStipple },
        { "glScissor", (GLvoid *) glScissor, _gloffset_Scissor },
        { "glShadeModel", (GLvoid *) glShadeModel, _gloffset_ShadeModel },
        { "glTexParameterf", (GLvoid *) glTexParameterf, _gloffset_TexParameterf },
        { "glTexParameterfv", (GLvoid *) glTexParameterfv, _gloffset_TexParameterfv },
        { "glTexParameteri", (GLvoid *) glTexParameteri, _gloffset_TexParameteri },
        { "glTexParameteriv", (GLvoid *) glTexParameteriv, _gloffset_TexParameteriv },
        { "glTexImage1D", (GLvoid *) glTexImage1D, _gloffset_TexImage1D },
        { "glTexImage2D", (GLvoid *) glTexImage2D, _gloffset_TexImage2D },
        { "glTexEnvf", (GLvoid *) glTexEnvf, _gloffset_TexEnvf },
        { "glTexEnvfv", (GLvoid *) glTexEnvfv, _gloffset_TexEnvfv },
        { "glTexEnvi", (GLvoid *) glTexEnvi, _gloffset_TexEnvi },
        { "glTexEnviv", (GLvoid *) glTexEnviv, _gloffset_TexEnviv },
        { "glTexGend", (GLvoid *) glTexGend, _gloffset_TexGend },
        { "glTexGendv", (GLvoid *) glTexGendv, _gloffset_TexGendv },
        { "glTexGenf", (GLvoid *) glTexGenf, _gloffset_TexGenf },
        { "glTexGenfv", (GLvoid *) glTexGenfv, _gloffset_TexGenfv },
        { "glTexGeni", (GLvoid *) glTexGeni, _gloffset_TexGeni },
        { "glTexGeniv", (GLvoid *) glTexGeniv, _gloffset_TexGeniv },
        { "glFeedbackBuffer", (GLvoid *) glFeedbackBuffer, _gloffset_FeedbackBuffer },
        { "glSelectBuffer", (GLvoid *) glSelectBuffer, _gloffset_SelectBuffer },
        { "glRenderMode", (GLvoid *) glRenderMode, _gloffset_RenderMode },
        { "glInitNames", (GLvoid *) glInitNames, _gloffset_InitNames },
        { "glLoadName", (GLvoid *) glLoadName, _gloffset_LoadName },
        { "glPassThrough", (GLvoid *) glPassThrough, _gloffset_PassThrough },
        { "glPopName", (GLvoid *) glPopName, _gloffset_PopName },
        { "glPushName", (GLvoid *) glPushName, _gloffset_PushName },
        { "glDrawBuffer", (GLvoid *) glDrawBuffer, _gloffset_DrawBuffer },
        { "glClear", (GLvoid *) glClear, _gloffset_Clear },
        { "glClearAccum", (GLvoid *) glClearAccum, _gloffset_ClearAccum },
        { "glClearIndex", (GLvoid *) glClearIndex, _gloffset_ClearIndex },
        { "glClearColor", (GLvoid *) glClearColor, _gloffset_ClearColor },
        { "glClearStencil", (GLvoid *) glClearStencil, _gloffset_ClearStencil },
        { "glClearDepth", (GLvoid *) glClearDepth, _gloffset_ClearDepth },
        { "glStencilMask", (GLvoid *) glStencilMask, _gloffset_StencilMask },
        { "glColorMask", (GLvoid *) glColorMask, _gloffset_ColorMask },
        { "glDepthMask", (GLvoid *) glDepthMask, _gloffset_DepthMask },
        { "glIndexMask", (GLvoid *) glIndexMask, _gloffset_IndexMask },
        { "glAccum", (GLvoid *) glAccum, _gloffset_Accum },
        { "glDisable", (GLvoid *) glDisable, _gloffset_Disable },
        { "glEnable", (GLvoid *) glEnable, _gloffset_Enable },
        { "glFinish", (GLvoid *) glFinish, _gloffset_Finish },
        { "glFlush", (GLvoid *) glFlush, _gloffset_Flush },
        { "glPopAttrib", (GLvoid *) glPopAttrib, _gloffset_PopAttrib },
        { "glPushAttrib", (GLvoid *) glPushAttrib, _gloffset_PushAttrib },
        { "glMap1d", (GLvoid *) glMap1d, _gloffset_Map1d },
        { "glMap1f", (GLvoid *) glMap1f, _gloffset_Map1f },
        { "glMap2d", (GLvoid *) glMap2d, _gloffset_Map2d },
        { "glMap2f", (GLvoid *) glMap2f, _gloffset_Map2f },
        { "glMapGrid1d", (GLvoid *) glMapGrid1d, _gloffset_MapGrid1d },
        { "glMapGrid1f", (GLvoid *) glMapGrid1f, _gloffset_MapGrid1f },
        { "glMapGrid2d", (GLvoid *) glMapGrid2d, _gloffset_MapGrid2d },
        { "glMapGrid2f", (GLvoid *) glMapGrid2f, _gloffset_MapGrid2f },
        { "glEvalCoord1d", (GLvoid *) glEvalCoord1d, _gloffset_EvalCoord1d },
        { "glEvalCoord1dv", (GLvoid *) glEvalCoord1dv, _gloffset_EvalCoord1dv },
        { "glEvalCoord1f", (GLvoid *) glEvalCoord1f, _gloffset_EvalCoord1f },
        { "glEvalCoord1fv", (GLvoid *) glEvalCoord1fv, _gloffset_EvalCoord1fv },
        { "glEvalCoord2d", (GLvoid *) glEvalCoord2d, _gloffset_EvalCoord2d },
        { "glEvalCoord2dv", (GLvoid *) glEvalCoord2dv, _gloffset_EvalCoord2dv },
        { "glEvalCoord2f", (GLvoid *) glEvalCoord2f, _gloffset_EvalCoord2f },
        { "glEvalCoord2fv", (GLvoid *) glEvalCoord2fv, _gloffset_EvalCoord2fv },
        { "glEvalMesh1", (GLvoid *) glEvalMesh1, _gloffset_EvalMesh1 },
        { "glEvalPoint1", (GLvoid *) glEvalPoint1, _gloffset_EvalPoint1 },
        { "glEvalMesh2", (GLvoid *) glEvalMesh2, _gloffset_EvalMesh2 },
        { "glEvalPoint2", (GLvoid *) glEvalPoint2, _gloffset_EvalPoint2 },
        { "glAlphaFunc", (GLvoid *) glAlphaFunc, _gloffset_AlphaFunc },
        { "glBlendFunc", (GLvoid *) glBlendFunc, _gloffset_BlendFunc },
        { "glLogicOp", (GLvoid *) glLogicOp, _gloffset_LogicOp },
        { "glStencilFunc", (GLvoid *) glStencilFunc, _gloffset_StencilFunc },
        { "glStencilOp", (GLvoid *) glStencilOp, _gloffset_StencilOp },
        { "glDepthFunc", (GLvoid *) glDepthFunc, _gloffset_DepthFunc },
        { "glPixelZoom", (GLvoid *) glPixelZoom, _gloffset_PixelZoom },
        { "glPixelTransferf", (GLvoid *) glPixelTransferf, _gloffset_PixelTransferf },
        { "glPixelTransferi", (GLvoid *) glPixelTransferi, _gloffset_PixelTransferi },
        { "glPixelStoref", (GLvoid *) glPixelStoref, _gloffset_PixelStoref },
        { "glPixelStorei", (GLvoid *) glPixelStorei, _gloffset_PixelStorei },
        { "glPixelMapfv", (GLvoid *) glPixelMapfv, _gloffset_PixelMapfv },
        { "glPixelMapuiv", (GLvoid *) glPixelMapuiv, _gloffset_PixelMapuiv },
        { "glPixelMapusv", (GLvoid *) glPixelMapusv, _gloffset_PixelMapusv },
        { "glReadBuffer", (GLvoid *) glReadBuffer, _gloffset_ReadBuffer },
        { "glCopyPixels", (GLvoid *) glCopyPixels, _gloffset_CopyPixels },
        { "glReadPixels", (GLvoid *) glReadPixels, _gloffset_ReadPixels },
        { "glDrawPixels", (GLvoid *) glDrawPixels, _gloffset_DrawPixels },
        { "glGetBooleanv", (GLvoid *) glGetBooleanv, _gloffset_GetBooleanv },
        { "glGetClipPlane", (GLvoid *) glGetClipPlane, _gloffset_GetClipPlane },
        { "glGetDoublev", (GLvoid *) glGetDoublev, _gloffset_GetDoublev },
        { "glGetError", (GLvoid *) glGetError, _gloffset_GetError },
        { "glGetFloatv", (GLvoid *) glGetFloatv, _gloffset_GetFloatv },
        { "glGetIntegerv", (GLvoid *) glGetIntegerv, _gloffset_GetIntegerv },
        { "glGetLightfv", (GLvoid *) glGetLightfv, _gloffset_GetLightfv },
        { "glGetLightiv", (GLvoid *) glGetLightiv, _gloffset_GetLightiv },
        { "glGetMapdv", (GLvoid *) glGetMapdv, _gloffset_GetMapdv },
        { "glGetMapfv", (GLvoid *) glGetMapfv, _gloffset_GetMapfv },
        { "glGetMapiv", (GLvoid *) glGetMapiv, _gloffset_GetMapiv },
        { "glGetMaterialfv", (GLvoid *) glGetMaterialfv, _gloffset_GetMaterialfv },
        { "glGetMaterialiv", (GLvoid *) glGetMaterialiv, _gloffset_GetMaterialiv },
        { "glGetPixelMapfv", (GLvoid *) glGetPixelMapfv, _gloffset_GetPixelMapfv },
        { "glGetPixelMapuiv", (GLvoid *) glGetPixelMapuiv, _gloffset_GetPixelMapuiv },
        { "glGetPixelMapusv", (GLvoid *) glGetPixelMapusv, _gloffset_GetPixelMapusv },
        { "glGetPolygonStipple", (GLvoid *) glGetPolygonStipple, _gloffset_GetPolygonStipple },
        { "glGetString", (GLvoid *) glGetString, _gloffset_GetString },
        { "glGetTexEnvfv", (GLvoid *) glGetTexEnvfv, _gloffset_GetTexEnvfv },
        { "glGetTexEnviv", (GLvoid *) glGetTexEnviv, _gloffset_GetTexEnviv },
        { "glGetTexGendv", (GLvoid *) glGetTexGendv, _gloffset_GetTexGendv },
        { "glGetTexGenfv", (GLvoid *) glGetTexGenfv, _gloffset_GetTexGenfv },
        { "glGetTexGeniv", (GLvoid *) glGetTexGeniv, _gloffset_GetTexGeniv },
        { "glGetTexImage", (GLvoid *) glGetTexImage, _gloffset_GetTexImage },
        { "glGetTexParameterfv", (GLvoid *) glGetTexParameterfv, _gloffset_GetTexParameterfv },
        { "glGetTexParameteriv", (GLvoid *) glGetTexParameteriv, _gloffset_GetTexParameteriv },
        { "glGetTexLevelParameterfv", (GLvoid *) glGetTexLevelParameterfv, _gloffset_GetTexLevelParameterfv },
        { "glGetTexLevelParameteriv", (GLvoid *) glGetTexLevelParameteriv, _gloffset_GetTexLevelParameteriv },
        { "glIsEnabled", (GLvoid *) glIsEnabled, _gloffset_IsEnabled },
        { "glIsList", (GLvoid *) glIsList, _gloffset_IsList },
        { "glDepthRange", (GLvoid *) glDepthRange, _gloffset_DepthRange },
        { "glFrustum", (GLvoid *) glFrustum, _gloffset_Frustum },
        { "glLoadIdentity", (GLvoid *) glLoadIdentity, _gloffset_LoadIdentity },
        { "glLoadMatrixf", (GLvoid *) glLoadMatrixf, _gloffset_LoadMatrixf },
        { "glLoadMatrixd", (GLvoid *) glLoadMatrixd, _gloffset_LoadMatrixd },
        { "glMatrixMode", (GLvoid *) glMatrixMode, _gloffset_MatrixMode },
        { "glMultMatrixf", (GLvoid *) glMultMatrixf, _gloffset_MultMatrixf },
        { "glMultMatrixd", (GLvoid *) glMultMatrixd, _gloffset_MultMatrixd },
        { "glOrtho", (GLvoid *) glOrtho, _gloffset_Ortho },
        { "glPopMatrix", (GLvoid *) glPopMatrix, _gloffset_PopMatrix },
        { "glPushMatrix", (GLvoid *) glPushMatrix, _gloffset_PushMatrix },
        { "glRotated", (GLvoid *) glRotated, _gloffset_Rotated },
        { "glRotatef", (GLvoid *) glRotatef, _gloffset_Rotatef },
        { "glScaled", (GLvoid *) glScaled, _gloffset_Scaled },
        { "glScalef", (GLvoid *) glScalef, _gloffset_Scalef },
        { "glTranslated", (GLvoid *) glTranslated, _gloffset_Translated },
        { "glTranslatef", (GLvoid *) glTranslatef, _gloffset_Translatef },
        { "glViewport", (GLvoid *) glViewport, _gloffset_Viewport },
        /* 1.1 */
        { "glArrayElement", (GLvoid *) glArrayElement, _gloffset_ArrayElement },
        { "glColorPointer", (GLvoid *) glColorPointer, _gloffset_ColorPointer },
        { "glDisableClientState", (GLvoid *) glDisableClientState, _gloffset_DisableClientState },
        { "glDrawArrays", (GLvoid *) glDrawArrays, _gloffset_DrawArrays },
        { "glDrawElements", (GLvoid *) glDrawElements, _gloffset_DrawElements },
        { "glEdgeFlagPointer", (GLvoid *) glEdgeFlagPointer, _gloffset_EdgeFlagPointer },
        { "glEnableClientState", (GLvoid *) glEnableClientState, _gloffset_EnableClientState },
        { "glGetPointerv", (GLvoid *) glGetPointerv, _gloffset_GetPointerv },
        { "glIndexPointer", (GLvoid *) glIndexPointer, _gloffset_IndexPointer },
        { "glInterleavedArrays", (GLvoid *) glInterleavedArrays, _gloffset_InterleavedArrays },
        { "glNormalPointer", (GLvoid *) glNormalPointer, _gloffset_NormalPointer },
        { "glTexCoordPointer", (GLvoid *) glTexCoordPointer, _gloffset_TexCoordPointer },
        { "glVertexPointer", (GLvoid *) glVertexPointer, _gloffset_VertexPointer },
        { "glPolygonOffset", (GLvoid *) glPolygonOffset, _gloffset_PolygonOffset },
        { "glCopyTexImage1D", (GLvoid *) glCopyTexImage1D, _gloffset_CopyTexImage1D },
        { "glCopyTexImage2D", (GLvoid *) glCopyTexImage2D, _gloffset_CopyTexImage2D },
        { "glCopyTexSubImage1D", (GLvoid *) glCopyTexSubImage1D, _gloffset_CopyTexSubImage1D },
        { "glCopyTexSubImage2D", (GLvoid *) glCopyTexSubImage2D, _gloffset_CopyTexSubImage2D },
        { "glTexSubImage1D", (GLvoid *) glTexSubImage1D, _gloffset_TexSubImage1D },
        { "glTexSubImage2D", (GLvoid *) glTexSubImage2D, _gloffset_TexSubImage2D },
        { "glAreTexturesResident", (GLvoid *) glAreTexturesResident, _gloffset_AreTexturesResident },
        { "glBindTexture", (GLvoid *) glBindTexture, _gloffset_BindTexture },
        { "glDeleteTextures", (GLvoid *) glDeleteTextures, _gloffset_DeleteTextures },
        { "glGenTextures", (GLvoid *) glGenTextures, _gloffset_GenTextures },
        { "glIsTexture", (GLvoid *) glIsTexture, _gloffset_IsTexture },
        { "glPrioritizeTextures", (GLvoid *) glPrioritizeTextures, _gloffset_PrioritizeTextures },
        { "glIndexub", (GLvoid *) glIndexub, _gloffset_Indexub },
        { "glIndexubv", (GLvoid *) glIndexubv, _gloffset_Indexubv },
        { "glPopClientAttrib", (GLvoid *) glPopClientAttrib, _gloffset_PopClientAttrib },
        { "glPushClientAttrib", (GLvoid *) glPushClientAttrib, _gloffset_PushClientAttrib },
	/* 1.2 */
#ifdef GL_VERSION_1_2
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glBlendColor", (GLvoid *) NAME(glBlendColor), _gloffset_BlendColor },
	{ "glBlendEquation", (GLvoid *) NAME(glBlendEquation), _gloffset_BlendEquation },
	{ "glDrawRangeElements", (GLvoid *) NAME(glDrawRangeElements), _gloffset_DrawRangeElements },
	{ "glColorTable", (GLvoid *) NAME(glColorTable), _gloffset_ColorTable },
	{ "glColorTableParameterfv", (GLvoid *) NAME(glColorTableParameterfv), _gloffset_ColorTableParameterfv },
	{ "glColorTableParameteriv", (GLvoid *) NAME(glColorTableParameteriv), _gloffset_ColorTableParameteriv },
	{ "glCopyColorTable", (GLvoid *) NAME(glCopyColorTable), _gloffset_CopyColorTable },
	{ "glGetColorTable", (GLvoid *) NAME(glGetColorTable), _gloffset_GetColorTable },
	{ "glGetColorTableParameterfv", (GLvoid *) NAME(glGetColorTableParameterfv), _gloffset_GetColorTableParameterfv },
	{ "glGetColorTableParameteriv", (GLvoid *) NAME(glGetColorTableParameteriv), _gloffset_GetColorTableParameteriv },
	{ "glColorSubTable", (GLvoid *) NAME(glColorSubTable), _gloffset_ColorSubTable },
	{ "glCopyColorSubTable", (GLvoid *) NAME(glCopyColorSubTable), _gloffset_CopyColorSubTable },
	{ "glConvolutionFilter1D", (GLvoid *) NAME(glConvolutionFilter1D), _gloffset_ConvolutionFilter1D },
	{ "glConvolutionFilter2D", (GLvoid *) NAME(glConvolutionFilter2D), _gloffset_ConvolutionFilter2D },
	{ "glConvolutionParameterf", (GLvoid *) NAME(glConvolutionParameterf), _gloffset_ConvolutionParameterf },
	{ "glConvolutionParameterfv", (GLvoid *) NAME(glConvolutionParameterfv), _gloffset_ConvolutionParameterfv },
	{ "glConvolutionParameteri", (GLvoid *) NAME(glConvolutionParameteri), _gloffset_ConvolutionParameteri },
	{ "glConvolutionParameteriv", (GLvoid *) NAME(glConvolutionParameteriv), _gloffset_ConvolutionParameteriv },
	{ "glCopyConvolutionFilter1D", (GLvoid *) NAME(glCopyConvolutionFilter1D), _gloffset_CopyConvolutionFilter1D },
	{ "glCopyConvolutionFilter2D", (GLvoid *) NAME(glCopyConvolutionFilter2D), _gloffset_CopyConvolutionFilter2D },
	{ "glGetConvolutionFilter", (GLvoid *) NAME(glGetConvolutionFilter), _gloffset_GetConvolutionFilter },
	{ "glGetConvolutionParameterfv", (GLvoid *) NAME(glGetConvolutionParameterfv), _gloffset_GetConvolutionParameterfv },
	{ "glGetConvolutionParameteriv", (GLvoid *) NAME(glGetConvolutionParameteriv), _gloffset_GetConvolutionParameteriv },
	{ "glGetSeparableFilter", (GLvoid *) NAME(glGetSeparableFilter), _gloffset_GetSeparableFilter },
	{ "glSeparableFilter2D", (GLvoid *) NAME(glSeparableFilter2D), _gloffset_SeparableFilter2D },
	{ "glGetHistogram", (GLvoid *) NAME(glGetHistogram), _gloffset_GetHistogram },
	{ "glGetHistogramParameterfv", (GLvoid *) NAME(glGetHistogramParameterfv), _gloffset_GetHistogramParameterfv },
	{ "glGetHistogramParameteriv", (GLvoid *) NAME(glGetHistogramParameteriv), _gloffset_GetHistogramParameteriv },
	{ "glGetMinmax", (GLvoid *) NAME(glGetMinmax), _gloffset_GetMinmax },
	{ "glGetMinmaxParameterfv", (GLvoid *) NAME(glGetMinmaxParameterfv), _gloffset_GetMinmaxParameterfv },
	{ "glGetMinmaxParameteriv", (GLvoid *) NAME(glGetMinmaxParameteriv), _gloffset_GetMinmaxParameteriv },
	{ "glHistogram", (GLvoid *) NAME(glHistogram), _gloffset_Histogram },
	{ "glMinmax", (GLvoid *) NAME(glMinmax), _gloffset_Minmax },
	{ "glResetHistogram", (GLvoid *) NAME(glResetHistogram), _gloffset_ResetHistogram },
	{ "glResetMinmax", (GLvoid *) NAME(glResetMinmax), _gloffset_ResetMinmax },
	{ "glTexImage3D", (GLvoid *) NAME(glTexImage3D), _gloffset_TexImage3D },
	{ "glTexSubImage3D", (GLvoid *) NAME(glTexSubImage3D), _gloffset_TexSubImage3D },
	{ "glCopyTexSubImage3D", (GLvoid *) NAME(glCopyTexSubImage3D), _gloffset_CopyTexSubImage3D },
#undef NAME

	/* ARB 1. GL_ARB_multitexture */
#ifdef GL_ARB_multitexture
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glActiveTextureARB", (GLvoid *) NAME(glActiveTextureARB), _gloffset_ActiveTextureARB },
	{ "glClientActiveTextureARB", (GLvoid *) NAME(glClientActiveTextureARB), _gloffset_ClientActiveTextureARB },
	{ "glMultiTexCoord1dARB", (GLvoid *) NAME(glMultiTexCoord1dARB), _gloffset_MultiTexCoord1dARB },
	{ "glMultiTexCoord1dvARB", (GLvoid *) NAME(glMultiTexCoord1dvARB), _gloffset_MultiTexCoord1dvARB },
	{ "glMultiTexCoord1fARB", (GLvoid *) NAME(glMultiTexCoord1fARB), _gloffset_MultiTexCoord1fARB },
	{ "glMultiTexCoord1fvARB", (GLvoid *) NAME(glMultiTexCoord1fvARB), _gloffset_MultiTexCoord1fvARB },
	{ "glMultiTexCoord1iARB", (GLvoid *) NAME(glMultiTexCoord1iARB), _gloffset_MultiTexCoord1iARB },
	{ "glMultiTexCoord1ivARB", (GLvoid *) NAME(glMultiTexCoord1ivARB), _gloffset_MultiTexCoord1ivARB },
	{ "glMultiTexCoord1sARB", (GLvoid *) NAME(glMultiTexCoord1sARB), _gloffset_MultiTexCoord1sARB },
	{ "glMultiTexCoord1svARB", (GLvoid *) NAME(glMultiTexCoord1svARB), _gloffset_MultiTexCoord1svARB },
	{ "glMultiTexCoord2dARB", (GLvoid *) NAME(glMultiTexCoord2dARB), _gloffset_MultiTexCoord2dARB },
	{ "glMultiTexCoord2dvARB", (GLvoid *) NAME(glMultiTexCoord2dvARB), _gloffset_MultiTexCoord2dvARB },
	{ "glMultiTexCoord2fARB", (GLvoid *) NAME(glMultiTexCoord2fARB), _gloffset_MultiTexCoord2fARB },
	{ "glMultiTexCoord2fvARB", (GLvoid *) NAME(glMultiTexCoord2fvARB), _gloffset_MultiTexCoord2fvARB },
	{ "glMultiTexCoord2iARB", (GLvoid *) NAME(glMultiTexCoord2iARB), _gloffset_MultiTexCoord2iARB },
	{ "glMultiTexCoord2ivARB", (GLvoid *) NAME(glMultiTexCoord2ivARB), _gloffset_MultiTexCoord2ivARB },
	{ "glMultiTexCoord2sARB", (GLvoid *) NAME(glMultiTexCoord2sARB), _gloffset_MultiTexCoord2sARB },
	{ "glMultiTexCoord2svARB", (GLvoid *) NAME(glMultiTexCoord2svARB), _gloffset_MultiTexCoord2svARB },
	{ "glMultiTexCoord3dARB", (GLvoid *) NAME(glMultiTexCoord3dARB), _gloffset_MultiTexCoord3dARB },
	{ "glMultiTexCoord3dvARB", (GLvoid *) NAME(glMultiTexCoord3dvARB), _gloffset_MultiTexCoord3dvARB },
	{ "glMultiTexCoord3fARB", (GLvoid *) NAME(glMultiTexCoord3fARB), _gloffset_MultiTexCoord3fARB },
	{ "glMultiTexCoord3fvARB", (GLvoid *) NAME(glMultiTexCoord3fvARB), _gloffset_MultiTexCoord3fvARB },
	{ "glMultiTexCoord3iARB", (GLvoid *) NAME(glMultiTexCoord3iARB), _gloffset_MultiTexCoord3iARB },
	{ "glMultiTexCoord3ivARB", (GLvoid *) NAME(glMultiTexCoord3ivARB), _gloffset_MultiTexCoord3ivARB },
	{ "glMultiTexCoord3sARB", (GLvoid *) NAME(glMultiTexCoord3sARB), _gloffset_MultiTexCoord3sARB },
	{ "glMultiTexCoord3svARB", (GLvoid *) NAME(glMultiTexCoord3svARB), _gloffset_MultiTexCoord3svARB },
	{ "glMultiTexCoord4dARB", (GLvoid *) NAME(glMultiTexCoord4dARB), _gloffset_MultiTexCoord4dARB },
	{ "glMultiTexCoord4dvARB", (GLvoid *) NAME(glMultiTexCoord4dvARB), _gloffset_MultiTexCoord4dvARB },
	{ "glMultiTexCoord4fARB", (GLvoid *) NAME(glMultiTexCoord4fARB), _gloffset_MultiTexCoord4fARB },
	{ "glMultiTexCoord4fvARB", (GLvoid *) NAME(glMultiTexCoord4fvARB), _gloffset_MultiTexCoord4fvARB },
	{ "glMultiTexCoord4iARB", (GLvoid *) NAME(glMultiTexCoord4iARB), _gloffset_MultiTexCoord4iARB },
	{ "glMultiTexCoord4ivARB", (GLvoid *) NAME(glMultiTexCoord4ivARB), _gloffset_MultiTexCoord4ivARB },
	{ "glMultiTexCoord4sARB", (GLvoid *) NAME(glMultiTexCoord4sARB), _gloffset_MultiTexCoord4sARB },
	{ "glMultiTexCoord4svARB", (GLvoid *) NAME(glMultiTexCoord4svARB), _gloffset_MultiTexCoord4svARB },
#undef NAME

	/* ARB 3. GL_ARB_transpose_matrix */
#ifdef GL_ARB_transpose_matrix
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glLoadTransposeMatrixdARB", (GLvoid *) NAME(glLoadTransposeMatrixdARB), _gloffset_LoadTransposeMatrixdARB },
	{ "glLoadTransposeMatrixfARB", (GLvoid *) NAME(glLoadTransposeMatrixfARB), _gloffset_LoadTransposeMatrixfARB },
	{ "glMultTransposeMatrixdARB", (GLvoid *) NAME(glMultTransposeMatrixdARB), _gloffset_MultTransposeMatrixdARB },
	{ "glMultTransposeMatrixfARB", (GLvoid *) NAME(glMultTransposeMatrixfARB), _gloffset_MultTransposeMatrixfARB },
#undef NAME

        /* ARB 5. GL_ARB_multisample */
#ifdef GL_ARB_multisample
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
        { "glSampleCoverageARB", NAME(glSampleCoverageARB), _gloffset_SampleCoverageARB },
#undef NAME

        /* ARB 12. GL_ARB_texture_compression */
#if 000
#if defined(GL_ARB_texture_compression) && defined(_gloffset_CompressedTexImage3DARB)
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
        { "glCompressedTexImage3DARB", NAME(glCompressedTexImage3DARB), _gloffset_CompressedTexImage3DARB },
        { "glCompressedTexImage2DARB", NAME(glCompressedTexImage2DARB), _gloffset_CompressedTexImage2DARB },
        { "glCompressedTexImage1DARB", NAME(glCompressedTexImage1DARB), _gloffset_CompressedTexImage1DARB },
        { "glCompressedTexSubImage3DARB", NAME(glCompressedTexSubImage3DARB), _gloffset_CompressedTexSubImage3DARB },
        { "glCompressedTexSubImage2DARB", NAME(glCompressedTexSubImage2DARB), _gloffset_CompressedTexSubImage2DARB },
        { "glCompressedTexSubImage1DARB", NAME(glCompressedTexSubImage1DARB), _gloffset_CompressedTexSubImage1DARB },
        { "glGetCompressedTexImageARB", NAME(glGetCompressedTexImageARB), _gloffset_GetCompressedTexImageARB },
#undef NAME
#endif

	/* 2. GL_EXT_blend_color */
#ifdef GL_EXT_blend_color
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glBlendColorEXT", NAME(glBlendColorEXT), _gloffset_BlendColor },
#undef NAME

	/* 3. GL_EXT_polygon_offset */
#ifdef GL_EXT_polygon_offset
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glPolygonOffsetEXT", NAME(glPolygonOffsetEXT), _gloffset_PolygonOffsetEXT },
#undef NAME

	/* 6. GL_EXT_texture3D */
#ifdef GL_EXT_texture3D
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glCopyTexSubImage3DEXT", NAME(glCopyTexSubImage3DEXT), _gloffset_CopyTexSubImage3D },
	{ "glTexImage3DEXT", NAME(glTexImage3DEXT), _gloffset_TexImage3D },
	{ "glTexSubImage3DEXT", NAME(glTexSubImage3DEXT), _gloffset_TexSubImage3D },
#undef NAME

	/* 7. GL_SGI_texture_filter4 */
#ifdef GL_SGI_texture_filter4
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glGetTexFilterFuncSGIS", NAME(glGetTexFilterFuncSGIS), _gloffset_GetTexFilterFuncSGIS },
	{ "glTexFilterFuncSGIS", NAME(glTexFilterFuncSGIS), _gloffset_TexFilterFuncSGIS },
#undef NAME

	/* 9. GL_EXT_subtexture */
#ifdef GL_EXT_subtexture
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glTexSubImage1DEXT", NAME(glTexSubImage1DEXT), _gloffset_TexSubImage1D },
	{ "glTexSubImage2DEXT", NAME(glTexSubImage2DEXT), _gloffset_TexSubImage2D },
#undef NAME

	/* 10. GL_EXT_copy_texture */
#ifdef GL_EXT_copy_texture
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glCopyTexImage1DEXT", NAME(glCopyTexImage1DEXT), _gloffset_CopyTexImage1D },
	{ "glCopyTexImage2DEXT", NAME(glCopyTexImage2DEXT), _gloffset_CopyTexImage2D },
	{ "glCopyTexSubImage1DEXT", NAME(glCopyTexSubImage1DEXT), _gloffset_CopyTexSubImage1D },
	{ "glCopyTexSubImage2DEXT", NAME(glCopyTexSubImage2DEXT), _gloffset_CopyTexSubImage2D },
#undef NAME

	/* 11. GL_EXT_histogram */
#ifdef GL_EXT_histogram
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glGetHistogramEXT", NAME(glGetHistogramEXT), _gloffset_GetHistogramEXT },
	{ "glGetHistogramParameterfvEXT", NAME(glGetHistogramParameterfvEXT), _gloffset_GetHistogramParameterfvEXT },
	{ "glGetHistogramParameterivEXT", NAME(glGetHistogramParameterivEXT), _gloffset_GetHistogramParameterivEXT },
	{ "glGetMinmaxEXT", NAME(glGetMinmaxEXT), _gloffset_GetMinmaxEXT },
	{ "glGetMinmaxParameterfvEXT", NAME(glGetMinmaxParameterfvEXT), _gloffset_GetMinmaxParameterfvEXT },
	{ "glGetMinmaxParameterivEXT", NAME(glGetMinmaxParameterivEXT), _gloffset_GetMinmaxParameterivEXT },
	{ "glHistogramEXT", NAME(glHistogramEXT), _gloffset_Histogram },
	{ "glMinmaxEXT", NAME(glMinmaxEXT), _gloffset_Minmax },
	{ "glResetHistogramEXT", NAME(glResetHistogramEXT), _gloffset_ResetHistogram },
	{ "glResetMinmaxEXT", NAME(glResetMinmaxEXT), _gloffset_ResetMinmax },
#undef NAME

	/* 12. GL_EXT_convolution */
#ifdef GL_EXT_convolution
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glConvolutionFilter1DEXT", NAME(glConvolutionFilter1DEXT), _gloffset_ConvolutionFilter1D },
	{ "glConvolutionFilter2DEXT", NAME(glConvolutionFilter2DEXT), _gloffset_ConvolutionFilter2D },
	{ "glConvolutionParameterfEXT", NAME(glConvolutionParameterfEXT), _gloffset_ConvolutionParameterf },
	{ "glConvolutionParameterfvEXT", NAME(glConvolutionParameterfvEXT), _gloffset_ConvolutionParameterfv },
	{ "glConvolutionParameteriEXT", NAME(glConvolutionParameteriEXT), _gloffset_ConvolutionParameteri },
	{ "glConvolutionParameterivEXT", NAME(glConvolutionParameterivEXT), _gloffset_ConvolutionParameteriv },
	{ "glCopyConvolutionFilter1DEXT", NAME(glCopyConvolutionFilter1DEXT), _gloffset_CopyConvolutionFilter1D },
	{ "glCopyConvolutionFilter2DEXT", NAME(glCopyConvolutionFilter2DEXT), _gloffset_CopyConvolutionFilter2D },
	{ "glGetConvolutionFilterEXT", NAME(glGetConvolutionFilterEXT), _gloffset_GetConvolutionFilterEXT },
	{ "glGetConvolutionParameterivEXT", NAME(glGetConvolutionParameterivEXT), _gloffset_GetConvolutionParameterivEXT },
	{ "glGetConvolutionParameterfvEXT", NAME(glGetConvolutionParameterfvEXT), _gloffset_GetConvolutionParameterfvEXT },
	{ "glGetSeparableFilterEXT", NAME(glGetSeparableFilterEXT), _gloffset_GetSeparableFilterEXT },
	{ "glSeparableFilter2DEXT", NAME(glSeparableFilter2DEXT), _gloffset_SeparableFilter2D },
#undef NAME

	/* 14. GL_SGI_color_table */
#ifdef GL_SGI_color_table
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glColorTableSGI", NAME(glColorTableSGI), _gloffset_ColorTable },
	{ "glColorTableParameterfvSGI", NAME(glColorTableParameterfvSGI), _gloffset_ColorTableParameterfv },
	{ "glColorTableParameterivSGI", NAME(glColorTableParameterivSGI), _gloffset_ColorTableParameteriv },
	{ "glCopyColorTableSGI", NAME(glCopyColorTableSGI), _gloffset_CopyColorTable },
	{ "glGetColorTableSGI", NAME(glGetColorTableSGI), _gloffset_GetColorTableSGI },
	{ "glGetColorTableParameterfvSGI", NAME(glGetColorTableParameterfvSGI), _gloffset_GetColorTableParameterfvSGI },
	{ "glGetColorTableParameterivSGI", NAME(glGetColorTableParameterivSGI), _gloffset_GetColorTableParameterivSGI },
#undef NAME

	/* 15. GL_SGIS_pixel_texture */
#ifdef GL_SGIS_pixel_texture
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glPixelTexGenParameterfSGIS", NAME(glPixelTexGenParameterfSGIS), _gloffset_PixelTexGenParameterfSGIS },
	{ "glPixelTexGenParameteriSGIS", NAME(glPixelTexGenParameteriSGIS), _gloffset_PixelTexGenParameteriSGIS },
	{ "glGetPixelTexGenParameterfvSGIS", NAME(glGetPixelTexGenParameterfvSGIS), _gloffset_GetPixelTexGenParameterfvSGIS },
	{ "glGetPixelTexGenParameterivSGIS", NAME(glGetPixelTexGenParameterivSGIS), _gloffset_GetPixelTexGenParameterivSGIS },
#undef NAME

	/* 16. GL_SGIS_texture4D */
#ifdef  GL_SGIS_texture4D
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glTexImage4DSGIS", NAME(glTexImage4DSGIS), _gloffset_TexImage4DSGIS },
	{ "glTexSubImage4DSGIS", NAME(glTexSubImage4DSGIS), _gloffset_TexSubImage4DSGIS },
#undef NAME

	/* 20. GL_EXT_texture_object */
#ifdef GL_EXT_texture_object
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glAreTexturesResidentEXT", NAME(glAreTexturesResidentEXT), _gloffset_AreTexturesResidentEXT },
	{ "glBindTextureEXT", NAME(glBindTextureEXT), _gloffset_BindTexture },
	{ "glDeleteTexturesEXT", NAME(glDeleteTexturesEXT), _gloffset_DeleteTextures },
	{ "glGenTexturesEXT", NAME(glGenTexturesEXT), _gloffset_GenTexturesEXT },
	{ "glIsTextureEXT", NAME(glIsTextureEXT), _gloffset_IsTextureEXT },
	{ "glPrioritizeTexturesEXT", NAME(glPrioritizeTexturesEXT), _gloffset_PrioritizeTextures },
#undef NAME

	/* 21. GL_SGIS_detail_texture */
#ifdef GL_SGIS_detail_texture
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glDetailTexFuncSGIS", NAME(glDetailTexFuncSGIS), _gloffset_DetailTexFuncSGIS },
	{ "glGetDetailTexFuncSGIS", NAME(glGetDetailTexFuncSGIS), _gloffset_GetDetailTexFuncSGIS },
#undef NAME

	/* 22. GL_SGIS_sharpen_texture */
#ifdef GL_SGIS_sharpen_texture
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glGetSharpenTexFuncSGIS", NAME(glGetSharpenTexFuncSGIS), _gloffset_GetSharpenTexFuncSGIS },
	{ "glSharpenTexFuncSGIS", NAME(glSharpenTexFuncSGIS), _gloffset_SharpenTexFuncSGIS },
#undef NAME

	/* 25. GL_SGIS_multisample */
#ifdef GL_SGIS_multisample
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glSampleMaskSGIS", NAME(glSampleMaskSGIS), _gloffset_SampleMaskSGIS },
	{ "glSamplePatternSGIS", NAME(glSamplePatternSGIS), _gloffset_SamplePatternSGIS },
#undef NAME

	/* 30. GL_EXT_vertex_array */
#ifdef GL_EXT_vertex_array
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glArrayElementEXT", NAME(glArrayElementEXT), _gloffset_ArrayElement },
	{ "glColorPointerEXT", NAME(glColorPointerEXT), _gloffset_ColorPointerEXT },
	{ "glDrawArraysEXT", NAME(glDrawArraysEXT), _gloffset_DrawArrays },
	{ "glEdgeFlagPointerEXT", NAME(glEdgeFlagPointerEXT), _gloffset_EdgeFlagPointerEXT },
	{ "glGetPointervEXT", NAME(glGetPointervEXT), _gloffset_GetPointerv },
	{ "glIndexPointerEXT", NAME(glIndexPointerEXT), _gloffset_IndexPointerEXT },
	{ "glNormalPointerEXT", NAME(glNormalPointerEXT), _gloffset_NormalPointerEXT },
	{ "glTexCoordPointerEXT", NAME(glTexCoordPointerEXT), _gloffset_TexCoordPointerEXT },
	{ "glVertexPointerEXT", NAME(glVertexPointerEXT), _gloffset_VertexPointerEXT },
#undef NAME

	/* 37. GL_EXT_blend_minmax */
#ifdef GL_EXT_blend_minmax
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glBlendEquationEXT", NAME(glBlendEquationEXT), _gloffset_BlendEquation },
#undef NAME

	/* 52. GL_SGIX_sprite */
#ifdef GL_SGIX_sprite
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glSpriteParameterfSGIX", NAME(glSpriteParameterfSGIX), _gloffset_SpriteParameterfSGIX },
	{ "glSpriteParameterfvSGIX", NAME(glSpriteParameterfvSGIX), _gloffset_SpriteParameterfvSGIX },
	{ "glSpriteParameteriSGIX", NAME(glSpriteParameteriSGIX), _gloffset_SpriteParameteriSGIX },
	{ "glSpriteParameterivSGIX", NAME(glSpriteParameterivSGIX), _gloffset_SpriteParameterivSGIX },
#undef NAME

	/* 54. GL_EXT_point_parameters */
#ifdef GL_EXT_point_parameters
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glPointParameterfEXT", NAME(glPointParameterfEXT), _gloffset_PointParameterfEXT },
	{ "glPointParameterfvEXT", NAME(glPointParameterfvEXT), _gloffset_PointParameterfvEXT },
	{ "glPointParameterfSGIS", NAME(glPointParameterfSGIS), _gloffset_PointParameterfEXT },
	{ "glPointParameterfvSGIS", NAME(glPointParameterfvSGIS), _gloffset_PointParameterfvEXT },
#undef NAME

	/* 55. GL_SGIX_instruments */
#ifdef GL_SGIX_instruments
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glInstrumentsBufferSGIX", NAME(glInstrumentsBufferSGIX), _gloffset_InstrumentsBufferSGIX },
	{ "glStartInstrumentsSGIX", NAME(glStartInstrumentsSGIX), _gloffset_StartInstrumentsSGIX },
	{ "glStopInstrumentsSGIX", NAME(glStopInstrumentsSGIX), _gloffset_StopInstrumentsSGIX },
	{ "glReadInstrumentsSGIX", NAME(glReadInstrumentsSGIX), _gloffset_ReadInstrumentsSGIX },
	{ "glPollInstrumentsSGIX", NAME(glPollInstrumentsSGIX), _gloffset_PollInstrumentsSGIX },
	{ "glGetInstrumentsSGIX", NAME(glGetInstrumentsSGIX), _gloffset_GetInstrumentsSGIX },
#undef NAME

	/* 57. GL_SGIX_framezoom */
#ifdef GL_SGIX_framezoom
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glFrameZoomSGIX", NAME(glFrameZoomSGIX), _gloffset_FrameZoomSGIX },
#undef NAME

        /* 58. GL_SGIX_tag_sample_buffer */
#ifdef GL_SGIX_tag_sample_buffer
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glTagSampleBufferSGIX", NAME(glTagSampleBufferSGIX), _gloffset_TagSampleBufferSGIX },
#undef NAME

	/* 60. GL_SGIX_reference_plane */
#ifdef GL_SGIX_reference_plane
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glReferencePlaneSGIX", NAME(glReferencePlaneSGIX), _gloffset_ReferencePlaneSGIX },
#undef NAME

	/* 61. GL_SGIX_flush_raster */
#ifdef GL_SGIX_flush_raster
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glFlushRasterSGIX", NAME(glFlushRasterSGIX), _gloffset_FlushRasterSGIX },
#undef NAME

	/* 66. GL_HP_image_transform */
#if 0
#ifdef GL_HP_image_transform
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glGetImageTransformParameterfvHP", NAME(glGetImageTransformParameterfvHP), _gloffset_GetImageTransformParameterfvHP },
	{ "glGetImageTransformParameterivHP", NAME(glGetImageTransformParameterivHP), _gloffset_GetImageTransformParameterivHP },
	{ "glImageTransformParameterfHP", NAME(glImageTransformParameterfHP), _gloffset_ImageTransformParameterfHP },
	{ "glImageTransformParameterfvHP", NAME(glImageTransformParameterfvHP), _gloffset_ImageTransformParameterfvHP },
	{ "glImageTransformParameteriHP", NAME(glImageTransformParameteriHP), _gloffset_ImageTransformParameteriHP },
	{ "glImageTransformParameterivHP", NAME(glImageTransformParameterivHP), _gloffset_ImageTransformParameterivHP },
#undef NAME
#endif

	/* 74. GL_EXT_color_subtable */
#ifdef GL_EXT_color_subtable
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glColorSubTableEXT", NAME(glColorSubTableEXT), _gloffset_ColorSubTable },
	{ "glCopyColorSubTableEXT", NAME(glCopyColorSubTableEXT), _gloffset_CopyColorSubTable },
#undef NAME

	/* 77. GL_PGI_misc_hints */
#ifdef GL_PGI_misc_hints
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glHintPGI", NAME(glHintPGI), _gloffset_HintPGI },
#undef NAME

	/* 78. GL_EXT_paletted_texture */
#ifdef GL_EXT_paletted_texture
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glColorTableEXT", NAME(glColorTableEXT), _gloffset_ColorTable },
	{ "glGetColorTableEXT", NAME(glGetColorTableEXT), _gloffset_GetColorTable },
	{ "glGetColorTableParameterfvEXT", NAME(glGetColorTableParameterfvEXT), _gloffset_GetColorTableParameterfv },
	{ "glGetColorTableParameterivEXT", NAME(glGetColorTableParameterivEXT), _gloffset_GetColorTableParameteriv },
#undef NAME

	/* 80. GL_SGIX_list_priority */
#ifdef GL_SGIX_list_priority
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glGetListParameterfvSGIX", NAME(glGetListParameterfvSGIX), _gloffset_GetListParameterfvSGIX },
	{ "glGetListParameterivSGIX", NAME(glGetListParameterivSGIX), _gloffset_GetListParameterivSGIX },
	{ "glListParameterfSGIX", NAME(glListParameterfSGIX), _gloffset_ListParameterfSGIX },
	{ "glListParameterfvSGIX", NAME(glListParameterfvSGIX), _gloffset_ListParameterfvSGIX },
	{ "glListParameteriSGIX", NAME(glListParameteriSGIX), _gloffset_ListParameteriSGIX },
	{ "glListParameterivSGIX", NAME(glListParameterivSGIX), _gloffset_ListParameterivSGIX },
#undef NAME

	/* 94. GL_EXT_index_material */
#ifdef GL_EXT_index_material
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glIndexMaterialEXT", NAME(glIndexMaterialEXT), _gloffset_IndexMaterialEXT },
#undef NAME

	/* 95. GL_EXT_index_func */
#ifdef GL_EXT_index_func
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glIndexFuncEXT", NAME(glIndexFuncEXT), _gloffset_IndexFuncEXT },
#undef NAME

	/* 97. GL_EXT_compiled_vertex_array */
#ifdef GL_EXT_compiled_vertex_array
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glLockArraysEXT", NAME(glLockArraysEXT), _gloffset_LockArraysEXT },
	{ "glUnlockArraysEXT", NAME(glUnlockArraysEXT), _gloffset_UnlockArraysEXT },
#undef NAME

	/* 98. GL_EXT_cull_vertex */
#ifdef GL_EXT_cull_vertex
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glCullParameterfvEXT", NAME(glCullParameterfvEXT), _gloffset_CullParameterfvEXT },
	{ "glCullParameterdvEXT", NAME(glCullParameterdvEXT), _gloffset_CullParameterdvEXT },
#undef NAME

        /* 102. GL_SGIX_fragment_lighting */
#ifdef GL_SGIX_fragment_lighting
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glFragmentColorMaterialSGIX", NAME(glFragmentColorMaterialSGIX), _gloffset_FragmentColorMaterialSGIX },
	{ "glFragmentLightfSGIX", NAME(glFragmentLightfSGIX), _gloffset_FragmentLightfSGIX },
	{ "glFragmentLightfvSGIX", NAME(glFragmentLightfvSGIX), _gloffset_FragmentLightfvSGIX },
	{ "glFragmentLightiSGIX", NAME(glFragmentLightiSGIX), _gloffset_FragmentLightiSGIX },
	{ "glFragmentLightivSGIX", NAME(glFragmentLightivSGIX), _gloffset_FragmentLightivSGIX },
	{ "glFragmentLightModelfSGIX", NAME(glFragmentLightModelfSGIX), _gloffset_FragmentLightModelfSGIX },
	{ "glFragmentLightModelfvSGIX", NAME(glFragmentLightModelfvSGIX), _gloffset_FragmentLightModelfvSGIX },
	{ "glFragmentLightModeliSGIX", NAME(glFragmentLightModeliSGIX), _gloffset_FragmentLightModeliSGIX },
	{ "glFragmentLightModelivSGIX", NAME(glFragmentLightModelivSGIX), _gloffset_FragmentLightModelivSGIX },
	{ "glFragmentMaterialfSGIX", NAME(glFragmentMaterialfSGIX), _gloffset_FragmentMaterialfSGIX },
	{ "glFragmentMaterialfvSGIX", NAME(glFragmentMaterialfvSGIX), _gloffset_FragmentMaterialfvSGIX },
	{ "glFragmentMaterialiSGIX", NAME(glFragmentMaterialiSGIX), _gloffset_FragmentMaterialiSGIX },
	{ "glFragmentMaterialivSGIX", NAME(glFragmentMaterialivSGIX), _gloffset_FragmentMaterialivSGIX },
	{ "glGetFragmentLightfvSGIX", NAME(glGetFragmentLightfvSGIX), _gloffset_GetFragmentLightfvSGIX },
	{ "glGetFragmentLightivSGIX", NAME(glGetFragmentLightivSGIX), _gloffset_GetFragmentLightivSGIX },
	{ "glGetFragmentMaterialfvSGIX", NAME(glGetFragmentMaterialfvSGIX), _gloffset_GetFragmentMaterialfvSGIX },
	{ "glGetFragmentMaterialivSGIX", NAME(glGetFragmentMaterialivSGIX), _gloffset_GetFragmentMaterialivSGIX },
	{ "glLightEnviSGIX", NAME(glLightEnviSGIX), _gloffset_LightEnviSGIX },
#undef NAME

        /* 112. GL_EXT_draw_range_elements */
#if 000
#ifdef GL_EXT_draw_range_elements
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
        { "glDrawRangeElementsEXT", NAME(glDrawRangeElementsEXT), _gloffset_DrawRangeElementsEXT },
#undef NAME
#endif

        /* 117. GL_EXT_light_texture */
#if 000
#ifdef GL_EXT_light_texture
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
        { "glApplyTextureEXT", NAME(glApplyTextureEXT), _gloffset_ApplyTextureEXT },
        { "glTextureLightEXT", NAME(glTextureLightEXT), _gloffset_TextureLightEXT },
        { "glTextureMaterialEXT", NAME(glTextureMaterialEXT), _gloffset_TextureMaterialEXT },
#undef NAME

        /* 135. GL_INTEL_texture_scissor */
#ifdef GL_INTEL_texture_scissor
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
        { "glTexScissorINTEL", NAME(glTexScissorINTEL), _gloffset_TexScissorINTEL },
        { "glTexScissorFuncINTEL", NAME(glTexScissorFuncINTEL), _gloffset_glTexScissorFuncINTEL },
#undef NAME

        /* 136. GL_INTEL_parallel_arrays */
#ifdef GL_INTEL_parallel_arrays
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
        { "glVertexPointervINTEL", NAME(glVertexPointervINTEL), _gloffset_VertexPointervINTEL },
        { "glNormalPointervINTEL", NAME(glNormalPointervINTEL), _gloffset_NormalPointervINTEL },
        { "glColorPointervINTEL", NAME(glColorPointervINTEL), _gloffset_ColorPointervINTEL },
        { "glTexCoordPointervINTEL", NAME(glTexCoordPointervINTEL), _gloffset_glxCoordPointervINTEL },
#undef NAME
#endif

        /* 138. GL_EXT_pixel_transform */
#if 000
#ifdef GL_EXT_pixel_transform
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
        { "glPixelTransformParameteriEXT", NAME(glPixelTransformParameteriEXT), _gloffset_PixelTransformParameteriEXT },
        { "glPixelTransformParameterfEXT", NAME(glPixelTransformParameterfEXT), _gloffset_PixelTransformParameterfEXT },
        { "glPixelTransformParameterivEXT", NAME(glPixelTransformParameterivEXT), _gloffset_PixelTransformParameterivEXT },
        { "glPixelTransformParameterfvEXT", NAME(glPixelTransformParameterfvEXT), _gloffset_PixelTransformParameterfvEXT },
        { "glGetPixelTransformParameterivEXT", NAME(glGetPixelTransformParameterivEXT), _gloffset_GetPixelTransformParameterivEXT },
        { "glGetPixelTransformParameterfvEXT", NAME(glGetPixelTransformParameterfvEXT), _gloffset_GetPixelTransformParameterfvEXT },
#undef NAME
#endif

        /* 145. GL_EXT_secondary_color */
#ifdef GL_EXT_secondary_color
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
        { "glSecondaryColor3bEXT", NAME(glSecondaryColor3bEXT), _gloffset_SecondaryColor3bEXT },
        { "glSecondaryColor3dEXT", NAME(glSecondaryColor3dEXT), _gloffset_SecondaryColor3dEXT },
        { "glSecondaryColor3fEXT", NAME(glSecondaryColor3fEXT), _gloffset_SecondaryColor3fEXT },
        { "glSecondaryColor3iEXT", NAME(glSecondaryColor3iEXT), _gloffset_SecondaryColor3iEXT },
        { "glSecondaryColor3sEXT", NAME(glSecondaryColor3sEXT), _gloffset_SecondaryColor3sEXT },
        { "glSecondaryColor3ubEXT", NAME(glSecondaryColor3ubEXT), _gloffset_SecondaryColor3ubEXT },
        { "glSecondaryColor3uiEXT", NAME(glSecondaryColor3uiEXT), _gloffset_SecondaryColor3uiEXT },
        { "glSecondaryColor3usEXT", NAME(glSecondaryColor3usEXT), _gloffset_SecondaryColor3usEXT },
        { "glSecondaryColor3bvEXT", NAME(glSecondaryColor3bvEXT), _gloffset_SecondaryColor3bvEXT },
	{ "glSecondaryColor3dvEXT", NAME(glSecondaryColor3dvEXT), _gloffset_SecondaryColor3dvEXT },
	{ "glSecondaryColor3fvEXT", NAME(glSecondaryColor3fvEXT), _gloffset_SecondaryColor3fvEXT },
	{ "glSecondaryColor3ivEXT", NAME(glSecondaryColor3ivEXT), _gloffset_SecondaryColor3ivEXT },
	{ "glSecondaryColor3svEXT", NAME(glSecondaryColor3svEXT), _gloffset_SecondaryColor3svEXT },
	{ "glSecondaryColor3ubvEXT", NAME(glSecondaryColor3ubvEXT), _gloffset_SecondaryColor3ubvEXT },
	{ "glSecondaryColor3uivEXT", NAME(glSecondaryColor3uivEXT), _gloffset_SecondaryColor3uivEXT },
	{ "glSecondaryColor3usvEXT", NAME(glSecondaryColor3usvEXT), _gloffset_SecondaryColor3usvEXT },
	{ "glSecondaryColorPointerEXT", NAME(glSecondaryColorPointerEXT), _gloffset_SecondaryColorPointerEXT },
#undef NAME

	/* 147. GL_EXT_texture_perturb_normal */
#if 000
#ifdef GL_EXT_texture_perturb_normal
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glTextureNormalEXT", NAME(glTextureNormalEXT), _gloffset_TextureNormalEXT },
#undef NAME
#endif

	/* 148. GL_EXT_multi_draw_arrays */
#if 000
#ifdef GL_EXT_multi_draw_arrays
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glMultiDrawArraysEXT", NAME(glMultiDrawArraysEXT), _gloffset_MultiDrawArraysEXT },
#undef NAME
#endif

	/* 149. GL_EXT_fog_coord */
#ifdef GL_EXT_fog_coord
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glFogCoordfEXT", NAME(glFogCoordfEXT), _gloffset_FogCoordfEXT },
	{ "glFogCoordfvEXT", NAME(glFogCoordfvEXT), _gloffset_FogCoordfvEXT },
	{ "glFogCoorddEXT", NAME(glFogCoorddEXT), _gloffset_FogCoorddEXT },
	{ "glFogCoorddEXT", NAME(glFogCoorddEXT), _gloffset_FogCoorddEXT },
	{ "glFogCoordPointerEXT", NAME(glFogCoordPointerEXT), _gloffset_FogCoordPointerEXT },
#undef NAME

	/* 156. GL_EXT_coordinate_frame */
#if 000
#ifdef GL_EXT_coordinate_frame
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glTangent3bEXT", NAME(glTangent3bEXT), _gloffset_Tangent3bEXT },
	{ "glTangent3dEXT", NAME(glTangent3dEXT), _gloffset_Tangent3dEXT },
	{ "glTangent3fEXT", NAME(glTangent3fEXT), _gloffset_Tangent3fEXT },
	{ "glTangent3iEXT", NAME(glTangent3iEXT), _gloffset_Tangent3iEXT },
	{ "glTangent3sEXT", NAME(glTangent3sEXT), _gloffset_Tangent3sEXT },
	{ "glTangent3bvEXT", NAME(glTangent3bvEXT), _gloffset_Tangent3bvEXT },
	{ "glTangent3dvEXT", NAME(glTangent3dvEXT), _gloffset_Tangent3dvEXT },
	{ "glTangent3fvEXT", NAME(glTangent3fvEXT), _gloffset_Tangent3fvEXT },
	{ "glTangent3ivEXT", NAME(glTangent3ivEXT), _gloffset_Tangent3ivEXT },
	{ "glTangent3svEXT", NAME(glTangent3svEXT), _gloffset_Tangent3svEXT },
	{ "glBinormal3bEXT", NAME(glBinormal3bEXT), _gloffset_Binormal3bEXT },
	{ "glBinormal3dEXT", NAME(glBinormal3dEXT), _gloffset_Binormal3dEXT },
	{ "glBinormal3fEXT", NAME(glBinormal3fEXT), _gloffset_Binormal3fEXT },
	{ "glBinormal3iEXT", NAME(glBinormal3iEXT), _gloffset_Binormal3iEXT },
	{ "glBinormal3sEXT", NAME(glBinormal3sEXT), _gloffset_Binormal3sEXT },
	{ "glBinormal3bvEXT", NAME(glBinormal3bvEXT), _gloffset_Binormal3bvEXT },
	{ "glBinormal3dvEXT", NAME(glBinormal3dvEXT), _gloffset_Binormal3dvEXT },
	{ "glBinormal3fvEXT", NAME(glBinormal3fvEXT), _gloffset_Binormal3fvEXT },
	{ "glBinormal3ivEXT", NAME(glBinormal3ivEXT), _gloffset_Binormal3ivEXT },
	{ "glBinormal3svEXT", NAME(glBinormal3svEXT), _gloffset_Binormal3svEXT },
	{ "glTangentPointerEXT", NAME(glTangentPointerEXT), _gloffset_TangentPointerEXT },
	{ "glBinormalPointerEXT", NAME(glBinormalPointerEXT), _gloffset_BinormalPointerEXT },
#undef NAME
#endif

	/* 164. GL_SUN_global_alpha */
#if 000
#ifdef GL_SUN_global_alpha
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glGlobalAlphaFactorbSUN", NAME(glGlobalAlphaFactorbSUN), _gloffset_GlobalAlphaFactorbSUN },
	{ "glGlobalAlphaFactorsSUN", NAME(glGlobalAlphaFactorsSUN), _gloffset_GlobalAlphaFactorsSUN },
	{ "glGlobalAlphaFactoriSUN", NAME(glGlobalAlphaFactoriSUN), _gloffset_GlobalAlphaFactoriSUN },
	{ "glGlobalAlphaFactorfSUN", NAME(glGlobalAlphaFactorfSUN), _gloffset_GlobalAlphaFactorfSUN },
	{ "glGlobalAlphaFactordSUN", NAME(glGlobalAlphaFactordSUN), _gloffset_GlobalAlphaFactordSUN },
	{ "glGlobalAlphaFactorubSUN", NAME(glGlobalAlphaFactorubSUN), _gloffset_GlobalAlphaFactorubSUN },
	{ "glGlobalAlphaFactorusSUN", NAME(glGlobalAlphaFactorusSUN), _gloffset_GlobalAlphaFactorusSUN },
	{ "glGlobalAlphaFactoruiSUN", NAME(glGlobalAlphaFactoruiSUN), _gloffset_GlobalAlphaFactoruiSUN },
#undef NAME
#endif

	/* 165. GL_SUN_triangle_list */
#if 000
#ifdef GL_SUN_triangle_list
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glReplacementCodeuiSUN", NAME(glReplacementCodeuiSUN), _gloffset_ReplacementCodeuiSUN },
	{ "glReplacementCodeusSUN", NAME(glReplacementCodeusSUN), _gloffset_ReplacementCodeusSUN },
	{ "glReplacementCodeubSUN", NAME(glReplacementCodeubSUN), _gloffset_ReplacementCodeubSUN },
	{ "glReplacementCodeuivSUN", NAME(glReplacementCodeuivSUN), _gloffset_ReplacementCodeuivSUN },
	{ "glReplacementCodeusvSUN", NAME(glReplacementCodeusvSUN), _gloffset_ReplacementCodeusvSUN },
	{ "glReplacementCodeubvSUN", NAME(glReplacementCodeubvSUN), _gloffset_ReplacementCodeubvSUN },
	{ "glReplacementCodePointerSUN", NAME(glReplacementCodePointerSUN), _gloffset_ReplacementCodePointerSUN },
#undef NAME
#endif

	/* 166. GL_SUN_vertex */
#if 000
#ifdef GL_SUN_vertex
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glColor4ubVertex2fSUN", NAME(glColor4ubVertex2fSUN), _gloffset_Color4ubVertex2fSUN },
	{ "glColor4ubVertex2fvSUN", NAME(glColor4ubVertex2fvSUN), _gloffset_Color4ubVertex2fvSUN },
	{ "glColor4ubVertex3fSUN", NAME(glColor4ubVertex3fSUN), _gloffset_Color4ubVertex3fSUN },
	{ "glColor4ubVertex3fvSUN", NAME(glColor4ubVertex3fvSUN), _gloffset_Color4ubVertex3fvSUN },
	{ "glColor3fVertex3fSUN", NAME(glColor3fVertex3fSUN), _gloffset_Color3fVertex3fSUN },
	{ "glColor3fVertex3fvSUN", NAME(glColor3fVertex3fvSUN), _gloffset_Color3fVertex3fvSUN },
	{ "glNormal3fVertex3fSUN", NAME(glNormal3fVertex3fSUN), _gloffset_Normal3fVertex3fSUN },
	{ "glNormal3fVertex3fvSUN", NAME(glNormal3fVertex3fvSUN), _gloffset_Normal3fVertex3fvSUN },
	{ "glColor4fNormal3fVertex3fSUN", NAME(glColor4fNormal3fVertex3fSUN), _gloffset_Color4fNormal3fVertex3fSUN },
	{ "glColor4fNormal3fVertex3fvSUN", NAME(glColor4fNormal3fVertex3fvSUN), _gloffset_Color4fNormal3fVertex3fvSUN },
	{ "glTexCoord2fVertex3fSUN", NAME(glTexCoord2fVertex3fSUN), _gloffset_TexCoord2fVertex3fSUN },
	{ "glTexCoord2fVertex3fvSUN", NAME(glTexCoord2fVertex3fvSUN), _gloffset_TexCoord2fVertex3fvSUN },
	{ "glTexCoord4fVertex4fSUN", NAME(glTexCoord4fVertex4fSUN), _gloffset_TexCoord4fVertex4fSUN },
	{ "glTexCoord4fVertex4fvSUN", NAME(glTexCoord4fVertex4fvSUN), _gloffset_TexCoord4fVertex4fvSUN },
	{ "glTexCoord2fColor4ubVertex3fSUN", NAME(glTexCoord2fColor4ubVertex3fSUN), _gloffset_TexCoord2fColor4ubVertex3fSUN },
	{ "glTexCoord2fColor4ubVertex3fvSUN", NAME(glTexCoord2fColor4ubVertex3fvSUN), _gloffset_TexCoord2fColor4ubVertex3fvSUN },
	{ "glTexCoord2fColor3fVertex3fSUN", NAME(glTexCoord2fColor3fVertex3fSUN), _gloffset_TexCoord2fColor3fVertex3fSUN },
	{ "glTexCoord2fColor3fVertex3fvSUN", NAME(glTexCoord2fColor3fVertex3fvSUN), _gloffset_TexCoord2fColor3fVertex3fvSUN },
	{ "glTexCoord2fNormal3fVertex3fSUN", NAME(glTexCoord2fNormal3fVertex3fSUN), _gloffset_TexCoord2fNormal3fVertex3fSUN },
	{ "glTexCoord2fNormal3fVertex3fvSUN", NAME(glTexCoord2fNormal3fVertex3fvSUN), _gloffset_TexCoord2fNormal3fVertex3fvSUN },
	{ "glTexCoord2fColor4fNormal3fVertex3fSUN", NAME(glTexCoord2fColor4fNormal3fVertex3fSUN), _gloffset_TexCoord2fColor4fNormal3fVertex3fSUN },
	{ "glTexCoord2fColor4fNormal3fVertex3fvSUN", NAME(glTexCoord2fColor4fNormal3fVertex3fvSUN), _gloffset_TexCoord2fColor4fNormal3fVertex3fvSUN },
	{ "glTexCoord4fColor4fNormal3fVertex4fSUN", NAME(glTexCoord4fColor4fNormal3fVertex4fSUN), _gloffset_TexCoord4fColor4fNormal3fVertex4fSUN },
	{ "glTexCoord4fColor4fNormal3fVertex4fvSUN", NAME(glTexCoord4fColor4fNormal3fVertex4fvSUN), _gloffset_TexCoord4fColor4fNormal3fVertex4fvSUN },
	{ "glReplacementCodeuiVertex3fSUN", NAME(glReplacementCodeuiVertex3fSUN), _gloffset_ReplacementCodeuiVertex3fSUN },
	{ "glReplacementCodeuiVertex3fvSUN", NAME(glReplacementCodeuiVertex3fvSUN), _gloffset_ReplacementCodeuiVertex3fvSUN },
	{ "glReplacementCodeuiColor4ubVertex3fSUN", NAME(glReplacementCodeuiColor4ubVertex3fSUN), _gloffset_ReplacementCodeuiColor4ubVertex3fSUN },
	{ "glReplacementCodeuiColor4ubVertex3fvSUN", NAME(glReplacementCodeuiColor4ubVertex3fvSUN), _gloffset_ReplacementCodeuiColor4ubVertex3fvSUN },
	{ "glReplacementCodeuiColor3fVertex3fSUN", NAME(glReplacementCodeuiColor3fVertex3fSUN), _gloffset_ReplacementCodeuiColor3fVertex3fSUN },
	{ "glReplacementCodeuiColor3fVertex3fvSUN", NAME(glReplacementCodeuiColor3fVertex3fvSUN), _gloffset_ReplacementCodeuiColor3fVertex3fvSUN },
	{ "glReplacementCodeuiNormal3fVertex3fSUN", NAME(glReplacementCodeuiNormal3fVertex3fSUN), _gloffset_ReplacementCodeuiNormal3fVertex3fSUN },
	{ "glReplacementCodeuiNormal3fVertex3fvSUN", NAME(glReplacementCodeuiNormal3fVertex3fvSUN), _gloffset_ReplacementCodeuiNormal3fVertex3fvSUN },
	{ "glReplacementCodeuiColor4fNormal3fVertex3fSUN", NAME(glReplacementCodeuiColor4fNormal3fVertex3fSUN), _gloffset_ReplacementCodeuiColor4fNormal3fVertex3fSUN },
	{ "glReplacementCodeuiColor4fNormal3fVertex3fvSUN", NAME(glReplacementCodeuiColor4fNormal3fVertex3fvSUN), _gloffset_ReplacementCodeuiColor4fNormal3fVertex3fvSUN },
	{ "glReplacementCodeuiTexCoord2fVertex3fSUN", NAME(glReplacementCodeuiTexCoord2fVertex3fSUN), _gloffset_ReplacementCodeuiTexCoord2fVertex3fSUN },
	{ "glReplacementCodeuiTexCoord2fVertex3fvSUN", NAME(glReplacementCodeuiTexCoord2fVertex3fvSUN), _gloffset_ReplacementCodeuiTexCoord2fVertex3fvSUN },
	{ "glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN", NAME(glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN), _gloffset_ReplacementCodeuiTexCoord2fNormal3fVertex3fSUN },
	{ "glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN", NAME(glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN), _gloffset_ReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN },
	{ "glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN", NAME(glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN), _gloffset_ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN },
	{ "glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN", NAME(glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN), _gloffset_ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN },
#undef NAME
#endif

	/* 173. GL_EXT/INGR_blend_func_separate */
#ifdef GL_EXT_blend_func_separate
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glBlendFuncSeparateEXT", NAME(glBlendFuncSeparateEXT), _gloffset_BlendFuncSeparateEXT },
	{ "glBlendFuncSeparateINGR", NAME(glBlendFuncSeparateEXT), _gloffset_BlendFuncSeparateEXT },
#undef NAME

	/* 188. GL_EXT_vertex_weighting */
#ifdef GL_EXT_vertex_weighting
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glVertexWeightfEXT", NAME(glVertexWeightfEXT), _gloffset_VertexWeightfEXT },
	{ "glVertexWeightfvEXT", NAME(glVertexWeightfvEXT), _gloffset_VertexWeightfvEXT },
	{ "glVertexWeightPointerEXT", NAME(glVertexWeightPointerEXT), _gloffset_VertexWeightPointerEXT },
#undef NAME

        /* 190. GL_NV_vertex_array_range */
#ifdef GL_NV_vertex_array_range
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glFlushVertexArrayRangeNV", NAME(glFlushVertexArrayRangeNV), _gloffset_FlushVertexArrayRangeNV },
	{ "glVertexArrayRangeNV", NAME(glVertexArrayRangeNV), _gloffset_VertexArrayRangeNV },
#undef NAME

	/* 191. GL_NV_register_combiners */
#ifdef GL_NV_register_combiners
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glCombinerParameterfvNV", NAME(glCombinerParameterfvNV), _gloffset_CombinerParameterfvNV },
	{ "glCombinerParameterfNV", NAME(glCombinerParameterfNV), _gloffset_CombinerParameterfNV },
	{ "glCombinerParameterivNV", NAME(glCombinerParameterivNV), _gloffset_CombinerParameterivNV },
	{ "glCombinerParameteriNV", NAME(glCombinerParameteriNV), _gloffset_CombinerParameteriNV },
	{ "glCombinerInputNV", NAME(glCombinerInputNV), _gloffset_CombinerInputNV },
	{ "glCombinerOutputNV", NAME(glCombinerOutputNV), _gloffset_CombinerOutputNV },
	{ "glFinalCombinerInputNV", NAME(glFinalCombinerInputNV), _gloffset_FinalCombinerInputNV },
	{ "glGetCombinerInputParameterfvNV", NAME(glGetCombinerInputParameterfvNV), _gloffset_GetCombinerInputParameterfvNV },
	{ "glGetCombinerInputParameterivNV", NAME(glGetCombinerInputParameterivNV), _gloffset_GetCombinerInputParameterivNV },
	{ "glGetCombinerOutputParameterfvNV", NAME(glGetCombinerOutputParameterfvNV), _gloffset_GetCombinerOutputParameterfvNV },
	{ "glGetCombinerOutputParameterivNV", NAME(glGetCombinerOutputParameterivNV), _gloffset_GetCombinerOutputParameterivNV },
	{ "glGetFinalCombinerInputParameterfvNV", NAME(glGetFinalCombinerInputParameterfvNV), _gloffset_GetFinalCombinerInputParameterfvNV },
	{ "glGetFinalCombinerInputParameterivNV", NAME(glGetFinalCombinerInputParameterivNV), _gloffset_GetFinalCombinerInputParameterivNV },
#undef NAME

	/* 196. GL_MESA_resize_buffers */
#ifdef MESA_resize_buffers
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glResizeBuffersMESA", NAME(glResizeBuffersMESA), _gloffset_ResizeBuffersMESA },
#undef NAME

	/* 197. GL_MESA_window_pos */
#ifdef MESA_window_pos
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
	{ "glWindowPos4fMESA", NAME(glWindowPos4fMESA), _gloffset_WindowPos4fMESA },
#undef NAME

        /* 209. WGL_EXT_multisample */
#ifdef WGL_EXT_multisample
#define NAME(X) (GLvoid *) X
#else
#define NAME(X) (GLvoid *) NotImplemented
#endif
        { "glSampleMaskEXT", NAME(glSampleMaskEXT), _gloffset_SampleMaskSGIS },
        { "glSamplePatternEXT", NAME(glSamplePatternEXT), _gloffset_SamplePatternSGIS },
#undef NAME

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
         return static_functions[i].Offset;
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
   GLint i;
   for (i = 0; static_functions[i].Name; i++) {
      if (strcmp(static_functions[i].Name, funcName) == 0) {
         return static_functions[i].Address;
      }
   }
   return NULL;
}



/**********************************************************************
 * Extension function management.
 */


#define MAX_EXTENSION_FUNCS 1000

static struct name_address_offset ExtEntryTable[MAX_EXTENSION_FUNCS];
static GLuint NumExtEntryPoints = 0;

#ifdef USE_SPARC_ASM
extern void __glapi_sparc_icache_flush(unsigned int *);
#endif

/*
 * Generate a dispatch function (entrypoint) which jumps through
 * the given slot number (offset) in the current dispatch table.
 * We need assembly language in order to accomplish this.
 */
static void *
generate_entrypoint(GLuint functionOffset)
{
#if defined(USE_X86_ASM)
   /*
    * This x86 code contributed by Josh Vanderhoof.
    *
    *  0:   a1 10 32 54 76          movl   __glapi_Dispatch,%eax
    *       00 01 02 03 04
    *  5:   85 c0                   testl  %eax,%eax
    *       05 06
    *  7:   74 06                   je     f <entrypoint+0xf>
    *       07 08
    *  9:   ff a0 10 32 54 76       jmp    *0x76543210(%eax)
    *       09 0a 0b 0c 0d 0e
    *  f:   e8 fc ff ff ff          call   __glapi_get_dispatch
    *       0f 10 11 12 13
    * 14:   ff a0 10 32 54 76       jmp    *0x76543210(%eax)
    *       14 15 16 17 18 19
    */
   static const unsigned char temp[] = {
      0xa1, 0x00, 0x00, 0x00, 0x00,
      0x85, 0xc0,
      0x74, 0x06,
      0xff, 0xa0, 0x00, 0x00, 0x00, 0x00,
      0xe8, 0x00, 0x00, 0x00, 0x00,
      0xff, 0xa0, 0x00, 0x00, 0x00, 0x00
   };
   unsigned char *code = malloc(sizeof(temp));
   unsigned int next_insn;
   if (code) {
      memcpy(code, temp, sizeof(temp));

      *(unsigned int *)(code + 0x01) = (unsigned int)&_glapi_Dispatch;
      *(unsigned int *)(code + 0x0b) = (unsigned int)functionOffset * 4;
      next_insn = (unsigned int)(code + 0x14);
      *(unsigned int *)(code + 0x10) = (unsigned int)_glapi_get_dispatch - next_insn;
      *(unsigned int *)(code + 0x16) = (unsigned int)functionOffset * 4;
   }
   return code;
#elif defined(USE_SPARC_ASM)

#ifdef __sparc_v9__
   static const unsigned int insn_template[] = {
	   0x05000000,	/* sethi	%uhi(_glapi_Dispatch), %g2	*/
	   0x03000000,	/* sethi	%hi(_glapi_Dispatch), %g1	*/
	   0x8410a000,	/* or		%g2, %ulo(_glapi_Dispatch), %g2	*/
	   0x82106000,	/* or		%g1, %lo(_glapi_Dispatch), %g1	*/
	   0x8528b020,	/* sllx		%g2, 32, %g2			*/
	   0xc2584002,	/* ldx		[%g1 + %g2], %g1		*/
	   0x05000000,	/* sethi	%hi(8 * glapioffset), %g2	*/
	   0x8410a000,	/* or		%g2, %lo(8 * glapioffset), %g2	*/
	   0xc6584002,	/* ldx		[%g1 + %g2], %g3		*/
	   0x81c0c000,	/* jmpl		%g3, %g0			*/
	   0x01000000	/*  nop						*/
   };
#else
   static const unsigned int insn_template[] = {
	   0x03000000,	/* sethi	%hi(_glapi_Dispatch), %g1	  */
	   0xc2006000,	/* ld		[%g1 + %lo(_glapi_Dispatch)], %g1 */
	   0xc6006000,	/* ld		[%g1 + %lo(4*glapioffset)], %g3	  */
	   0x81c0c000,	/* jmpl		%g3, %g0			  */
	   0x01000000	/*  nop						  */
   };
#endif
   unsigned int *code = malloc(sizeof(insn_template));
   unsigned long glapi_addr = (unsigned long) &_glapi_Dispatch;
   if (code) {
      memcpy(code, insn_template, sizeof(insn_template));

#ifdef __sparc_v9__
      code[0] |= (glapi_addr >> (32 + 10));
      code[1] |= ((glapi_addr & 0xffffffff) >> 10);
      __glapi_sparc_icache_flush(&code[0]);
      code[2] |= ((glapi_addr >> 32) & ((1 << 10) - 1));
      code[3] |= (glapi_addr & ((1 << 10) - 1));
      __glapi_sparc_icache_flush(&code[2]);
      code[6] |= ((functionOffset * 8) >> 10);
      code[7] |= ((functionOffset * 8) & ((1 << 10) - 1));
      __glapi_sparc_icache_flush(&code[6]);
#else
      code[0] |= (glapi_addr >> 10);
      code[1] |= (glapi_addr & ((1 << 10) - 1));
      __glapi_sparc_icache_flush(&code[0]);
      code[2] |= (functionOffset * 4);
      __glapi_sparc_icache_flush(&code[2]);
#endif
   }
   return code;
#else
   return NULL;
#endif
}



/*
 * Add a new extension function entrypoint.
 * Return: GL_TRUE = success or GL_FALSE = failure
 */
GLboolean
_glapi_add_entrypoint(const char *funcName, GLuint offset)
{
   /* first check if the named function is already statically present */
   {
      GLint index = get_static_proc_offset(funcName);
      if (index >= 0) {
         return (GLboolean) ((GLuint) index == offset);  /* bad offset! */
      }
   }

   {
      /* make sure this offset/name pair is legal */
      const char *name = _glapi_get_proc_name(offset);
      if (name && strcmp(name, funcName) != 0)
         return GL_FALSE;  /* bad name! */
   }

   {
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

      /* Make sure we don't try to add a new entrypoint after someone
       * has already called _glapi_get_dispatch_table_size()!  If that's
       * happened the caller's information would become out of date.
       */
      if (GetSizeCalled)
         return GL_FALSE;

      /* make sure we have space */
      if (NumExtEntryPoints >= MAX_EXTENSION_FUNCS) {
         return GL_FALSE;
      }
      else {
         void *entrypoint = generate_entrypoint(offset);
         if (!entrypoint)
            return GL_FALSE;

         ExtEntryTable[NumExtEntryPoints].Name = str_dup(funcName);
         ExtEntryTable[NumExtEntryPoints].Offset = offset;
         ExtEntryTable[NumExtEntryPoints].Address = entrypoint;
         NumExtEntryPoints++;

         if (offset > MaxDispatchOffset)
            MaxDispatchOffset = offset;

         return GL_TRUE;  /* success */
      }
   }

   /* should never get here, but play it safe */
   return GL_FALSE;
}



#if 0000 /* prototype code for dynamic extension slot allocation */

static int NextFreeOffset = 409; /*XXX*/
#define MAX_DISPATCH_TABLE_SIZE 1000

/*
 * Dynamically allocate a dispatch slot for an extension entrypoint
 * and generate the assembly language dispatch stub.
 * Return the dispatch offset for the function or -1 if no room or error.
 */
GLint
_glapi_add_entrypoint2(const char *funcName)
{
   int offset;

   /* first see if extension func is already known */
   offset = _glapi_get_proc_offset(funcName);
   if (offset >= 0)
      return offset;

   if (NumExtEntryPoints < MAX_EXTENSION_FUNCS
       && NextFreeOffset < MAX_DISPATCH_TABLE_SIZE) {
      void *entryPoint;
      offset = NextFreeOffset;
      entryPoint = generate_entrypoint(offset);
      if (entryPoint) {
         NextFreeOffset++;
         ExtEntryTable[NumExtEntryPoints].Name = str_dup(funcName);
         ExtEntryTable[NumExtEntryPoints].Offset = offset;
         ExtEntryTable[NumExtEntryPoints].Address = entryPoint;
         NumExtEntryPoints++;
         return offset;
      }
   }
   return -1;
}

#endif



/*
 * Return offset of entrypoint for named function within dispatch table.
 */
GLint
_glapi_get_proc_offset(const char *funcName)
{
   /* search extension functions first */
   GLuint i;
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
   GLuint i;
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
   const GLuint n = sizeof(static_functions) / sizeof(struct name_address_offset);
   GLuint i;
   for (i = 0; i < n; i++) {
      if (static_functions[i].Offset == offset)
         return static_functions[i].Name;
   }

   /* search added extension functions */
   for (i = 0; i < NumExtEntryPoints; i++) {
      if (ExtEntryTable[i].Offset == offset) {
         return ExtEntryTable[i].Name;
      }
   }
   return NULL;
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

#ifdef DEBUG
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
      GLuint blendColorOffset = _glapi_get_proc_offset("glBlendColor");
      char *blendColorFunc = (char*) &table->BlendColor;
      GLuint offset = (blendColorFunc - (char *) table) / sizeof(void *);
      assert(blendColorOffset == _gloffset_BlendColor);
      assert(blendColorOffset == offset);
   }
   {
      GLuint istextureOffset = _glapi_get_proc_offset("glIsTextureEXT");
      char *istextureFunc = (char*) &table->IsTextureEXT;
      GLuint offset = (istextureFunc - (char *) table) / sizeof(void *);
      assert(istextureOffset == _gloffset_IsTextureEXT);
      assert(istextureOffset == offset);
   }
   {
      GLuint secondaryColor3fOffset = _glapi_get_proc_offset("glSecondaryColor3fEXT");
      char *secondaryColor3fFunc = (char*) &table->SecondaryColor3fEXT;
      GLuint offset = (secondaryColor3fFunc - (char *) table) / sizeof(void *);
      assert(secondaryColor3fOffset == _gloffset_SecondaryColor3fEXT);
      assert(secondaryColor3fOffset == offset);
      assert(_glapi_get_proc_address("glSecondaryColor3fEXT") == (void *) &glSecondaryColor3fEXT);
   }
#endif
}
