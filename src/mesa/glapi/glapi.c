/* $Id: glapi.c,v 1.33 2000/02/12 16:44:25 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 *
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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
 */



#include "glheader.h"
#include "glapi.h"
#include "glapinoop.h"
#include "glapioffsets.h"
#include "glapitable.h"
#if defined(THREADS)
#include "glthread.h"
#endif


/* This is used when thread safety is disabled */
struct _glapi_table *_glapi_Dispatch = &__glapi_noop_table;

/* Used when thread safety disabled */
void *_glapi_Context = NULL;


#if defined(THREADS)

/* Flag to indicate whether thread-safe dispatch is enabled */
static GLboolean ThreadSafe = GL_FALSE;

static _glthread_TSD DispatchTSD;

static _glthread_TSD ContextTSD;

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
      dispatch = &__glapi_noop_table;
   }
#ifdef DEBUG
   else {
      _glapi_check_table(dispatch);
   }
#endif

#if defined(THREADS)
   _glthread_SetTSD(&DispatchTSD, (void*) dispatch);
   if (ThreadSafe)
      _glapi_Dispatch = NULL;
   else
      _glapi_Dispatch = dispatch;
#else
   _glapi_Dispatch = dispatch;
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
   else {
      assert(_glapi_Dispatch);
      return _glapi_Dispatch;
   }
#else
   return _glapi_Dispatch;
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
 */
const char *
_glapi_get_version(void)
{
   return "20000128";  /* YYYYMMDD */
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


#define MAX_EXTENSION_FUNCS 1000


struct _glapi_ext_entrypoint {
   const char *Name;   /* the extension function's name */
   GLuint Offset;      /* relative to start of dispatch table */
   GLvoid *Address;    /* address of dispatch function */
};

static struct _glapi_ext_entrypoint ExtEntryTable[MAX_EXTENSION_FUNCS];
static GLuint NumExtEntryPoints = 0;



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
   GLint index;

   /* Make sure we don't try to add a new entrypoint after someone
    * has already called _glapi_get_dispatch_table_size()!  If that's
    * happened the caller's information will now be out of date.
    */
   assert(!GetSizeCalled);

   /* first check if the named function is already statically present */
   index = get_static_proc_offset(funcName);

   if (index >= 0) {
      return (GLboolean) (index == offset);  /* bad offset! */
   }
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

      /* make sure we have space */
      if (NumExtEntryPoints >= MAX_EXTENSION_FUNCS) {
         return GL_FALSE;
      }
      else {
         void *entrypoint = generate_entrypoint(offset);
         if (!entrypoint)
            return GL_FALSE;

         ExtEntryTable[NumExtEntryPoints].Name = strdup(funcName);
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
         ExtEntryTable[NumExtEntryPoints].Name = strdup(funcName);
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
#endif
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
#ifdef GL_VERSION_1_1
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glAreTexturesResident", (GLvoid *) NAME(glAreTexturesResident) },
	{ "glArrayElement", (GLvoid *) NAME(glArrayElement) },
	{ "glBindTexture", (GLvoid *) NAME(glBindTexture) },
	{ "glColorPointer", (GLvoid *) NAME(glColorPointer) },
	{ "glCopyTexImage1D", (GLvoid *) NAME(glCopyTexImage1D) },
	{ "glCopyTexImage2D", (GLvoid *) NAME(glCopyTexImage2D) },
	{ "glCopyTexSubImage1D", (GLvoid *) NAME(glCopyTexSubImage1D) },
	{ "glCopyTexSubImage2D", (GLvoid *) NAME(glCopyTexSubImage2D) },
	{ "glDeleteTextures", (GLvoid *) NAME(glDeleteTextures) },
	{ "glDisableClientState", (GLvoid *) NAME(glDisableClientState) },
	{ "glDrawArrays", (GLvoid *) NAME(glDrawArrays) },
	{ "glDrawElements", (GLvoid *) NAME(glDrawElements) },
	{ "glEdgeFlagPointer", (GLvoid *) NAME(glEdgeFlagPointer) },
	{ "glEnableClientState", (GLvoid *) NAME(glEnableClientState) },
	{ "glGenTextures", (GLvoid *) NAME(glGenTextures) },
	{ "glGetPointerv", (GLvoid *) NAME(glGetPointerv) },
	{ "glIndexPointer", (GLvoid *) NAME(glIndexPointer) },
	{ "glIndexub", (GLvoid *) NAME(glIndexub) },
	{ "glIndexubv", (GLvoid *) NAME(glIndexubv) },
	{ "glInterleavedArrays", (GLvoid *) NAME(glInterleavedArrays) },
	{ "glIsTexture", (GLvoid *) NAME(glIsTexture) },
	{ "glNormalPointer", (GLvoid *) NAME(glNormalPointer) },
	{ "glPopClientAttrib", (GLvoid *) NAME(glPopClientAttrib) },
	{ "glPrioritizeTextures", (GLvoid *) NAME(glPrioritizeTextures) },
	{ "glPushClientAttrib", (GLvoid *) NAME(glPushClientAttrib) },
	{ "glTexCoordPointer", (GLvoid *) NAME(glTexCoordPointer) },
	{ "glTexSubImage1D", (GLvoid *) NAME(glTexSubImage1D) },
	{ "glTexSubImage2D", (GLvoid *) NAME(glTexSubImage2D) },
	{ "glVertexPointer", (GLvoid *) NAME(glVertexPointer) },
#undef NAME

	/* GL 1.2 */
#ifdef GL_VERSION_1_2
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glCopyTexSubImage3D", (GLvoid *) NAME(glCopyTexSubImage3D) },
	{ "glDrawRangeElements", (GLvoid *) NAME(glDrawRangeElements) },
	{ "glTexImage3D", (GLvoid *) NAME(glTexImage3D) },
	{ "glTexSubImage3D", (GLvoid *) NAME(glTexSubImage3D) },
#undef NAME

	/* GL_ARB_imaging */
#ifdef GL_ARB_imaging
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glBlendColor", (GLvoid *) NAME(glBlendColor) },
	{ "glBlendEquation", (GLvoid *) NAME(glBlendEquation) },
	{ "glColorSubTable", (GLvoid *) NAME(glColorSubTable) },
	{ "glColorTable", (GLvoid *) NAME(glColorTable) },
	{ "glColorTableParameterfv", (GLvoid *) NAME(glColorTableParameterfv) },
	{ "glColorTableParameteriv", (GLvoid *) NAME(glColorTableParameteriv) },
	{ "glConvolutionFilter1D", (GLvoid *) NAME(glConvolutionFilter1D) },
	{ "glConvolutionFilter2D", (GLvoid *) NAME(glConvolutionFilter2D) },
	{ "glConvolutionParameterf", (GLvoid *) NAME(glConvolutionParameterf) },
	{ "glConvolutionParameterfv", (GLvoid *) NAME(glConvolutionParameterfv) },
	{ "glConvolutionParameteri", (GLvoid *) NAME(glConvolutionParameteri) },
	{ "glConvolutionParameteriv", (GLvoid *) NAME(glConvolutionParameteriv) },
	{ "glCopyColorSubTable", (GLvoid *) NAME(glCopyColorSubTable) },
	{ "glCopyColorTable", (GLvoid *) NAME(glCopyColorTable) },
	{ "glCopyConvolutionFilter1D", (GLvoid *) NAME(glCopyConvolutionFilter1D) },
	{ "glCopyConvolutionFilter2D", (GLvoid *) NAME(glCopyConvolutionFilter2D) },
	{ "glGetColorTable", (GLvoid *) NAME(glGetColorTable) },
	{ "glGetColorTableParameterfv", (GLvoid *) NAME(glGetColorTableParameterfv) },
	{ "glGetColorTableParameteriv", (GLvoid *) NAME(glGetColorTableParameteriv) },
	{ "glGetConvolutionFilter", (GLvoid *) NAME(glGetConvolutionFilter) },
	{ "glGetConvolutionParameterfv", (GLvoid *) NAME(glGetConvolutionParameterfv) },
	{ "glGetConvolutionParameteriv", (GLvoid *) NAME(glGetConvolutionParameteriv) },
	{ "glGetHistogram", (GLvoid *) NAME(glGetHistogram) },
	{ "glGetHistogramParameterfv", (GLvoid *) NAME(glGetHistogramParameterfv) },
	{ "glGetHistogramParameteriv", (GLvoid *) NAME(glGetHistogramParameteriv) },
	{ "glGetMinmax", (GLvoid *) NAME(glGetMinmax) },
	{ "glGetMinmaxParameterfv", (GLvoid *) NAME(glGetMinmaxParameterfv) },
	{ "glGetMinmaxParameteriv", (GLvoid *) NAME(glGetMinmaxParameteriv) },
	{ "glGetSeparableFilter", (GLvoid *) NAME(glGetSeparableFilter) },
	{ "glHistogram", (GLvoid *) NAME(glHistogram) },
	{ "glMinmax", (GLvoid *) NAME(glMinmax) },
	{ "glResetHistogram", (GLvoid *) NAME(glResetHistogram) },
	{ "glResetMinmax", (GLvoid *) NAME(glResetMinmax) },
	{ "glSeparableFilter2D", (GLvoid *) NAME(glSeparableFilter2D) },
#undef NAME

	/* GL_ARB_multitexture */
#ifdef GL_ARB_multitexture
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glActiveTextureARB", (GLvoid *) NAME(glActiveTextureARB) },
	{ "glClientActiveTextureARB", (GLvoid *) NAME(glClientActiveTextureARB) },
	{ "glMultiTexCoord1dARB", (GLvoid *) NAME(glMultiTexCoord1dARB) },
	{ "glMultiTexCoord1dvARB", (GLvoid *) NAME(glMultiTexCoord1dvARB) },
	{ "glMultiTexCoord1fARB", (GLvoid *) NAME(glMultiTexCoord1fARB) },
	{ "glMultiTexCoord1fvARB", (GLvoid *) NAME(glMultiTexCoord1fvARB) },
	{ "glMultiTexCoord1iARB", (GLvoid *) NAME(glMultiTexCoord1iARB) },
	{ "glMultiTexCoord1ivARB", (GLvoid *) NAME(glMultiTexCoord1ivARB) },
	{ "glMultiTexCoord1sARB", (GLvoid *) NAME(glMultiTexCoord1sARB) },
	{ "glMultiTexCoord1svARB", (GLvoid *) NAME(glMultiTexCoord1svARB) },
	{ "glMultiTexCoord2dARB", (GLvoid *) NAME(glMultiTexCoord2dARB) },
	{ "glMultiTexCoord2dvARB", (GLvoid *) NAME(glMultiTexCoord2dvARB) },
	{ "glMultiTexCoord2fARB", (GLvoid *) NAME(glMultiTexCoord2fARB) },
	{ "glMultiTexCoord2fvARB", (GLvoid *) NAME(glMultiTexCoord2fvARB) },
	{ "glMultiTexCoord2iARB", (GLvoid *) NAME(glMultiTexCoord2iARB) },
	{ "glMultiTexCoord2ivARB", (GLvoid *) NAME(glMultiTexCoord2ivARB) },
	{ "glMultiTexCoord2sARB", (GLvoid *) NAME(glMultiTexCoord2sARB) },
	{ "glMultiTexCoord2svARB", (GLvoid *) NAME(glMultiTexCoord2svARB) },
	{ "glMultiTexCoord3dARB", (GLvoid *) NAME(glMultiTexCoord3dARB) },
	{ "glMultiTexCoord3dvARB", (GLvoid *) NAME(glMultiTexCoord3dvARB) },
	{ "glMultiTexCoord3fARB", (GLvoid *) NAME(glMultiTexCoord3fARB) },
	{ "glMultiTexCoord3fvARB", (GLvoid *) NAME(glMultiTexCoord3fvARB) },
	{ "glMultiTexCoord3iARB", (GLvoid *) NAME(glMultiTexCoord3iARB) },
	{ "glMultiTexCoord3ivARB", (GLvoid *) NAME(glMultiTexCoord3ivARB) },
	{ "glMultiTexCoord3sARB", (GLvoid *) NAME(glMultiTexCoord3sARB) },
	{ "glMultiTexCoord3svARB", (GLvoid *) NAME(glMultiTexCoord3svARB) },
	{ "glMultiTexCoord4dARB", (GLvoid *) NAME(glMultiTexCoord4dARB) },
	{ "glMultiTexCoord4dvARB", (GLvoid *) NAME(glMultiTexCoord4dvARB) },
	{ "glMultiTexCoord4fARB", (GLvoid *) NAME(glMultiTexCoord4fARB) },
	{ "glMultiTexCoord4fvARB", (GLvoid *) NAME(glMultiTexCoord4fvARB) },
	{ "glMultiTexCoord4iARB", (GLvoid *) NAME(glMultiTexCoord4iARB) },
	{ "glMultiTexCoord4ivARB", (GLvoid *) NAME(glMultiTexCoord4ivARB) },
	{ "glMultiTexCoord4sARB", (GLvoid *) NAME(glMultiTexCoord4sARB) },
	{ "glMultiTexCoord4svARB", (GLvoid *) NAME(glMultiTexCoord4svARB) },
#undef NAME

	/* 2. GL_EXT_blend_color */
#ifdef GL_EXT_blend_color
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glBlendColorEXT", (GLvoid *) NAME(glBlendColorEXT) },
#undef NAME

	/* 3. GL_EXT_polygon_offset */
#ifdef GL_EXT_polygon_offset
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glPolygonOffsetEXT", (GLvoid *) NAME(glPolygonOffsetEXT) },
#undef NAME

	/* 6. GL_EXT_texture3D */
#ifdef GL_EXT_texture3D
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glCopyTexSubImage3DEXT", (GLvoid *) NAME(glCopyTexSubImage3DEXT) },
	{ "glTexImage3DEXT", (GLvoid *) NAME(glTexImage3DEXT) },
	{ "glTexSubImage3DEXT", (GLvoid *) NAME(glTexSubImage3DEXT) },
#undef NAME

	/* 7. GL_SGI_texture_filter4 */
#ifdef GL_SGI_texture_filter4
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glGetTexFilterFuncSGIS", (GLvoid *) NAME(glGetTexFilterFuncSGIS) },
	{ "glTexFilterFuncSGIS", (GLvoid *) NAME(glTexFilterFuncSGIS) },
#undef NAME

	/* 9. GL_EXT_subtexture */
#ifdef GL_EXT_subtexture
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glTexSubImage1DEXT", (GLvoid *) NAME(glTexSubImage1DEXT) },
	{ "glTexSubImage2DEXT", (GLvoid *) NAME(glTexSubImage2DEXT) },
#undef NAME

	/* 10. GL_EXT_copy_texture */
#ifdef GL_EXT_copy_texture
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glCopyTexImage1DEXT", (GLvoid *) NAME(glCopyTexImage1DEXT) },
	{ "glCopyTexImage2DEXT", (GLvoid *) NAME(glCopyTexImage2DEXT) },
	{ "glCopyTexSubImage1DEXT", (GLvoid *) NAME(glCopyTexSubImage1DEXT) },
	{ "glCopyTexSubImage2DEXT", (GLvoid *) NAME(glCopyTexSubImage2DEXT) },
#undef NAME
                              
	/* 11. GL_EXT_histogram */
#ifdef GL_EXT_histogram
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glGetHistogramEXT", (GLvoid *) NAME(glGetHistogramEXT) },
	{ "glGetHistogramParameterfvEXT", (GLvoid *) NAME(glGetHistogramParameterfvEXT) },
	{ "glGetHistogramParameterivEXT", (GLvoid *) NAME(glGetHistogramParameterivEXT) },
	{ "glGetMinmaxEXT", (GLvoid *) NAME(glGetMinmaxEXT) },
	{ "glGetMinmaxParameterfvEXT", (GLvoid *) NAME(glGetMinmaxParameterfvEXT) },
	{ "glGetMinmaxParameterivEXT", (GLvoid *) NAME(glGetMinmaxParameterivEXT) },
	{ "glHistogramEXT", (GLvoid *) NAME(glHistogramEXT) },
	{ "glMinmaxEXT", (GLvoid *) NAME(glMinmaxEXT) },
	{ "glResetHistogramEXT", (GLvoid *) NAME(glResetHistogramEXT) },
	{ "glResetMinmaxEXT", (GLvoid *) NAME(glResetMinmaxEXT) },
#undef NAME

	/* 12. GL_EXT_convolution */
#ifdef GL_EXT_convolution
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glConvolutionFilter1DEXT", (GLvoid *) NAME(glConvolutionFilter1DEXT) },
	{ "glConvolutionFilter2DEXT", (GLvoid *) NAME(glConvolutionFilter2DEXT) },
	{ "glConvolutionParameterfEXT", (GLvoid *) NAME(glConvolutionParameterfEXT) },
	{ "glConvolutionParameterfvEXT", (GLvoid *) NAME(glConvolutionParameterfvEXT) },
	{ "glConvolutionParameteriEXT", (GLvoid *) NAME(glConvolutionParameteriEXT) },
	{ "glConvolutionParameterivEXT", (GLvoid *) NAME(glConvolutionParameterivEXT) },
	{ "glCopyConvolutionFilter1DEXT", (GLvoid *) NAME(glCopyConvolutionFilter1DEXT) },
	{ "glCopyConvolutionFilter2DEXT", (GLvoid *) NAME(glCopyConvolutionFilter2DEXT) },
	{ "glGetConvolutionFilterEXT", (GLvoid *) NAME(glGetConvolutionFilterEXT) },
	{ "glGetConvolutionParameterivEXT", (GLvoid *) NAME(glGetConvolutionParameterivEXT) },
	{ "glGetConvolutionParameterfvEXT", (GLvoid *) NAME(glGetConvolutionParameterfvEXT) },
	{ "glGetSeparableFilterEXT", (GLvoid *) NAME(glGetSeparableFilterEXT) },
	{ "glSeparableFilter2DEXT", (GLvoid *) NAME(glSeparableFilter2DEXT) },
#undef NAME
                    
	/* 14. GL_SGI_color_table */
#ifdef GL_SGI_color_table
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glColorTableSGI", (GLvoid *) NAME(glColorTableSGI) },
	{ "glColorTableParameterfvSGI", (GLvoid *) NAME(glColorTableParameterfvSGI) },
	{ "glColorTableParameterivSGI", (GLvoid *) NAME(glColorTableParameterivSGI) },
	{ "glCopyColorTableSGI", (GLvoid *) NAME(glCopyColorTableSGI) },
	{ "glGetColorTableSGI", (GLvoid *) NAME(glGetColorTableSGI) },
	{ "glGetColorTableParameterfvSGI", (GLvoid *) NAME(glGetColorTableParameterfvSGI) },
	{ "glGetColorTableParameterivSGI", (GLvoid *) NAME(glGetColorTableParameterivSGI) },
#undef NAME

	/* 15. GL_SGIS_pixel_texture */
#ifdef GL_SGIS_pixel_texture
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glPixelTexGenParameterfSGIS", (GLvoid *) NAME(glPixelTexGenParameterfSGIS) },
	{ "glPixelTexGenParameteriSGIS", (GLvoid *) NAME(glPixelTexGenParameteriSGIS) },
	{ "glGetPixelTexGenParameterfvSGIS", (GLvoid *) NAME(glGetPixelTexGenParameterfvSGIS) },
	{ "glGetPixelTexGenParameterivSGIS", (GLvoid *) NAME(glGetPixelTexGenParameterivSGIS) },
#undef NAME

	/* 16. GL_SGIS_texture4D */
#ifdef  GL_SGIS_texture4D
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glTexImage4DSGIS", (GLvoid *) NAME(glTexImage4DSGIS) },
	{ "glTexSubImage4DSGIS", (GLvoid *) NAME(glTexSubImage4DSGIS) },
#undef NAME

	/* 20. GL_EXT_texture_object */
#ifdef GL_EXT_texture_object
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glAreTexturesResidentEXT", (GLvoid *) NAME(glAreTexturesResidentEXT) },
	{ "glBindTextureEXT", (GLvoid *) NAME(glBindTextureEXT) },
	{ "glDeleteTexturesEXT", (GLvoid *) NAME(glDeleteTexturesEXT) },
	{ "glGenTexturesEXT", (GLvoid *) NAME(glGenTexturesEXT) },
	{ "glIsTextureEXT", (GLvoid *) NAME(glIsTextureEXT) },
	{ "glPrioritizeTexturesEXT", (GLvoid *) NAME(glPrioritizeTexturesEXT) },
#undef NAME

	/* 21. GL_SGIS_detail_texture */
#ifdef GL_SGIS_detail_texture
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glDetailTexFuncSGIS", (GLvoid *) NAME(glDetailTexFuncSGIS) },
	{ "glGetDetailTexFuncSGIS", (GLvoid *) NAME(glGetDetailTexFuncSGIS) },
#undef NAME

	/* 22. GL_SGIS_sharpen_texture */
#ifdef GL_SGIS_sharpen_texture
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glGetSharpenTexFuncSGIS", (GLvoid *) NAME(glGetSharpenTexFuncSGIS) },
	{ "glSharpenTexFuncSGIS", (GLvoid *) NAME(glSharpenTexFuncSGIS) },
#undef NAME

	/* 25. GL_SGIS_multisample */
#ifdef GL_SGIS_multisample
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glSampleMaskSGIS", (GLvoid *) NAME(glSampleMaskSGIS) },
	{ "glSamplePatternSGIS", (GLvoid *) NAME(glSamplePatternSGIS) },
#undef NAME

	/* 30. GL_EXT_vertex_array */
#ifdef GL_EXT_vertex_array
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glArrayElementEXT", (GLvoid *) NAME(glArrayElementEXT) },
	{ "glColorPointerEXT", (GLvoid *) NAME(glColorPointerEXT) },
	{ "glDrawArraysEXT", (GLvoid *) NAME(glDrawArraysEXT) },
	{ "glEdgeFlagPointerEXT", (GLvoid *) NAME(glEdgeFlagPointerEXT) },
	{ "glGetPointervEXT", (GLvoid *) NAME(glGetPointervEXT) },
	{ "glIndexPointerEXT", (GLvoid *) NAME(glIndexPointerEXT) },
	{ "glNormalPointerEXT", (GLvoid *) NAME(glNormalPointerEXT) },
	{ "glTexCoordPointerEXT", (GLvoid *) NAME(glTexCoordPointerEXT) },
	{ "glVertexPointerEXT", (GLvoid *) NAME(glVertexPointerEXT) },
#undef NAME

	/* 37. GL_EXT_blend_minmax */
#ifdef GL_EXT_blend_minmax
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glBlendEquationEXT", (GLvoid *) NAME(glBlendEquationEXT) },
#undef NAME

	/* 52. GL_SGIX_sprite */
#ifdef GL_SGIX_sprite
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glSpriteParameterfSGIX", (GLvoid *) NAME(glSpriteParameterfSGIX) },
	{ "glSpriteParameterfvSGIX", (GLvoid *) NAME(glSpriteParameterfvSGIX) },
	{ "glSpriteParameteriSGIX", (GLvoid *) NAME(glSpriteParameteriSGIX) },
	{ "glSpriteParameterivSGIX", (GLvoid *) NAME(glSpriteParameterivSGIX) },
#undef NAME

	/* 54. GL_EXT_point_parameters */
#ifdef GL_EXT_point_parameters
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glPointParameterfEXT", (GLvoid *) NAME(glPointParameterfEXT) },
	{ "glPointParameterfvEXT", (GLvoid *) NAME(glPointParameterfvEXT) },
#undef NAME

	/* 55. GL_SGIX_instruments */
#ifdef GL_SGIX_instruments
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glInstrumentsBufferSGIX", (GLvoid *) NAME(glInstrumentsBufferSGIX) },
	{ "glStartInstrumentsSGIX", (GLvoid *) NAME(glStartInstrumentsSGIX) },
	{ "glStopInstrumentsSGIX", (GLvoid *) NAME(glStopInstrumentsSGIX) },
	{ "glReadInstrumentsSGIX", (GLvoid *) NAME(glReadInstrumentsSGIX) },
	{ "glPollInstrumentsSGIX", (GLvoid *) NAME(glPollInstrumentsSGIX) },
	{ "glGetInstrumentsSGIX", (GLvoid *) NAME(glGetInstrumentsSGIX) },
#undef NAME

	/* 57. GL_SGIX_framezoom */
#ifdef GL_SGIX_framezoom
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glFrameZoomSGIX", (GLvoid *) NAME(glFrameZoomSGIX) },
#undef NAME

	/* 60. GL_SGIX_reference_plane */
#ifdef GL_SGIX_reference_plane
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glReferencePlaneSGIX", (GLvoid *) NAME(glReferencePlaneSGIX) },
#undef NAME

	/* 61. GL_SGIX_flush_raster */
#ifdef GL_SGIX_flush_raster
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glFlushRasterSGIX", (GLvoid *) NAME(glFlushRasterSGIX) },
#undef NAME

	/* 66. GL_HP_image_transform */
#ifdef GL_HP_image_transform
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glGetImageTransformParameterfvHP", (GLvoid *) NAME(glGetImageTransformParameterfvHP) },
	{ "glGetImageTransformParameterivHP", (GLvoid *) NAME(glGetImageTransformParameterivHP) },
	{ "glImageTransformParameterfHP", (GLvoid *) NAME(glImageTransformParameterfHP) },
	{ "glImageTransformParameterfvHP", (GLvoid *) NAME(glImageTransformParameterfvHP) },
	{ "glImageTransformParameteriHP", (GLvoid *) NAME(glImageTransformParameteriHP) },
	{ "glImageTransformParameterivHP", (GLvoid *) NAME(glImageTransformParameterivHP) },
#undef NAME

	/* 74. GL_EXT_color_subtable */
#ifdef GL_EXT_color_subtable
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glColorSubTableEXT", (GLvoid *) NAME(glColorSubTableEXT) },
	{ "glCopyColorSubTableEXT", (GLvoid *) NAME(glCopyColorSubTableEXT) },
#undef NAME

	/* 77. GL_PGI_misc_hints */
#ifdef GL_PGI_misc_hints
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glHintPGI", (GLvoid *) NAME(glHintPGI) },
#undef NAME

	/* 78. GL_EXT_paletted_texture */
#ifdef GL_EXT_paletted_texture
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glColorTableEXT", (GLvoid *) NAME(glColorTableEXT) },
	{ "glGetColorTableEXT", (GLvoid *) NAME(glGetColorTableEXT) },
	{ "glGetColorTableParameterfvEXT", (GLvoid *) NAME(glGetColorTableParameterfvEXT) },
	{ "glGetColorTableParameterivEXT", (GLvoid *) NAME(glGetColorTableParameterivEXT) },
#undef NAME

	/* 80. GL_SGIX_list_priority */
#ifdef GL_SGIX_list_priority
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glGetListParameterfvSGIX", (GLvoid *) NAME(glGetListParameterfvSGIX) },
	{ "glGetListParameterivSGIX", (GLvoid *) NAME(glGetListParameterivSGIX) },
	{ "glListParameterfSGIX", (GLvoid *) NAME(glListParameterfSGIX) },
	{ "glListParameterfvSGIX", (GLvoid *) NAME(glListParameterfvSGIX) },
	{ "glListParameteriSGIX", (GLvoid *) NAME(glListParameteriSGIX) },
	{ "glListParameterivSGIX", (GLvoid *) NAME(glListParameterivSGIX) },
#undef NAME

	/* 94. GL_EXT_index_material */
#ifdef GL_EXT_index_material
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glIndexMaterialEXT", (GLvoid *) NAME(glIndexMaterialEXT) },
#undef NAME

	/* 95. GL_EXT_index_func */
#ifdef GL_EXT_index_func
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glIndexFuncEXT", (GLvoid *) NAME(glIndexFuncEXT) },
#undef NAME

	/* 97. GL_EXT_compiled_vertex_array */
#ifdef GL_EXT_compiled_vertex_array
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glLockArraysEXT", (GLvoid *) NAME(glLockArraysEXT) },
	{ "glUnlockArraysEXT", (GLvoid *) NAME(glUnlockArraysEXT) },
#undef NAME

	/* 98. GL_EXT_cull_vertex */
#ifdef GL_EXT_cull_vertex
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glCullParameterfvEXT", (GLvoid *) NAME(glCullParameterfvEXT) },
	{ "glCullParameterdvEXT", (GLvoid *) NAME(glCullParameterdvEXT) },
#undef NAME

	/* 173. GL_EXT/INGR_blend_func_separate */
#ifdef GL_INGR_blend_func_separate
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glBlendFuncSeparateINGR", (GLvoid *) NAME(glBlendFuncSeparateINGR) },
#undef NAME

	/* GL_MESA_window_pos */
#ifdef MESA_window_pos
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glWindowPos4fMESA", (GLvoid *) NAME(glWindowPos4fMESA) },
#undef NAME

	/* GL_MESA_resize_buffers */
#ifdef MESA_resize_buffers
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glResizeBuffersMESA", (GLvoid *) NAME(glResizeBuffersMESA) },
#undef NAME

	/* GL_ARB_transpose_matrix */
#ifdef GL_ARB_transpose_matrix
#define NAME(X) X
#else
#define NAME(X) NotImplemented
#endif
	{ "glLoadTransposeMatrixdARB", (GLvoid *) NAME(glLoadTransposeMatrixdARB) },
	{ "glLoadTransposeMatrixfARB", (GLvoid *) NAME(glLoadTransposeMatrixfARB) },
	{ "glMultTransposeMatrixdARB", (GLvoid *) NAME(glMultTransposeMatrixdARB) },
	{ "glMultTransposeMatrixfARB", (GLvoid *) NAME(glMultTransposeMatrixfARB) },
#undef NAME

        /*
         * XXX many more extenstion functions to add.
         */

	{ NULL, NULL }  /* end of list marker */
};

