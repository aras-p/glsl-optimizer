/*
 * Mesa 3-D graphics library
 * Version:  7.1
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
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
 * NOTE: There are no dependencies on Mesa in this code.
 *
 * Versions (API changes):
 *   2000/02/23  - original version for Mesa 3.3 and XFree86 4.0
 *   2001/01/16  - added dispatch override feature for Mesa 3.5
 *   2002/06/28  - added _glapi_set_warning_func(), Mesa 4.1.
 *   2002/10/01  - _glapi_get_proc_address() will now generate new entrypoints
 *                 itself (using offset ~0).  _glapi_add_entrypoint() can be
 *                 called afterward and it'll fill in the correct dispatch
 *                 offset.  This allows DRI libGL to avoid probing for DRI
 *                 drivers!  No changes to the public glapi interface.
 */



#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#include "glapi/mesa.h"
#else
#include "main/glheader.h"
#include "main/compiler.h"
#endif

#include "glapi/glapi.h"
#include "glapi/glapioffsets.h"
#include "glapi/glapitable.h"

extern _glapi_proc __glapi_noop_table[];


/**
 * \name Current dispatch and current context control variables
 *
 * Depending on whether or not multithreading is support, and the type of
 * support available, several variables are used to store the current context
 * pointer and the current dispatch table pointer.  In the non-threaded case,
 * the variables \c _glapi_Dispatch and \c _glapi_Context are used for this
 * purpose.
 *
 * In the "normal" threaded case, the variables \c _glapi_Dispatch and
 * \c _glapi_Context will be \c NULL if an application is detected as being
 * multithreaded.  Single-threaded applications will use \c _glapi_Dispatch
 * and \c _glapi_Context just like the case without any threading support.
 * When \c _glapi_Dispatch and \c _glapi_Context are \c NULL, the thread state
 * data \c _gl_DispatchTSD and \c ContextTSD are used.  Drivers and the
 * static dispatch functions access these variables via \c _glapi_get_dispatch
 * and \c _glapi_get_context.
 *
 * There is a race condition in setting \c _glapi_Dispatch to \c NULL.  It is
 * possible for the original thread to be setting it at the same instant a new
 * thread, perhaps running on a different processor, is clearing it.  Because
 * of that, \c ThreadSafe, which can only ever be changed to \c GL_TRUE, is
 * used to determine whether or not the application is multithreaded.
 * 
 * In the TLS case, the variables \c _glapi_Dispatch and \c _glapi_Context are
 * hardcoded to \c NULL.  Instead the TLS variables \c _glapi_tls_Dispatch and
 * \c _glapi_tls_Context are used.  Having \c _glapi_Dispatch and
 * \c _glapi_Context be hardcoded to \c NULL maintains binary compatability
 * between TLS enabled loaders and non-TLS DRI drivers.
 */
/*@{*/
#if defined(GLX_USE_TLS)

PUBLIC __thread struct _glapi_table * _glapi_tls_Dispatch
    __attribute__((tls_model("initial-exec")))
    = (struct _glapi_table *) __glapi_noop_table;

PUBLIC __thread void * _glapi_tls_Context
    __attribute__((tls_model("initial-exec")));

PUBLIC const struct _glapi_table *_glapi_Dispatch = NULL;
PUBLIC const void *_glapi_Context = NULL;

#else

#if defined(THREADS)

#ifdef WIN32_THREADS
/* _glthread_DECLARE_STATIC_MUTEX is broken on windows.  There will be race! */
#define CHECK_MULTITHREAD_LOCK()
#define CHECK_MULTITHREAD_UNLOCK()
#else
_glthread_DECLARE_STATIC_MUTEX(ThreadCheckMutex);
#define CHECK_MULTITHREAD_LOCK() _glthread_LOCK_MUTEX(ThreadCheckMutex)
#define CHECK_MULTITHREAD_UNLOCK() _glthread_UNLOCK_MUTEX(ThreadCheckMutex)
#endif

static GLboolean ThreadSafe = GL_FALSE;  /**< In thread-safe mode? */
_glthread_TSD _gl_DispatchTSD;           /**< Per-thread dispatch pointer */
static _glthread_TSD ContextTSD;         /**< Per-thread context pointer */

#if defined(WIN32_THREADS)
void FreeTSD(_glthread_TSD *p);
void FreeAllTSD(void)
{
   FreeTSD(&_gl_DispatchTSD);
   FreeTSD(&ContextTSD);
}
#endif /* defined(WIN32_THREADS) */

#endif /* defined(THREADS) */

PUBLIC struct _glapi_table *_glapi_Dispatch = 
  (struct _glapi_table *) __glapi_noop_table;
PUBLIC void *_glapi_Context = NULL;

#endif /* defined(GLX_USE_TLS) */
/*@}*/




/**
 * We should call this periodically from a function such as glXMakeCurrent
 * in order to test if multiple threads are being used.
 */
PUBLIC void
_glapi_check_multithread(void)
{
#if defined(THREADS) && !defined(GLX_USE_TLS)
   static unsigned long knownID;
   static GLboolean firstCall = GL_TRUE;

   if (ThreadSafe)
      return;

   CHECK_MULTITHREAD_LOCK();
   if (firstCall) {
      /* initialize TSDs */
      (void) _glthread_GetTSD(&ContextTSD);
      (void) _glthread_GetTSD(&_gl_DispatchTSD);

      knownID = _glthread_GetID();
      firstCall = GL_FALSE;
   }
   else if (knownID != _glthread_GetID()) {
      ThreadSafe = GL_TRUE;
      _glapi_set_dispatch(NULL);
      _glapi_set_context(NULL);
   }
   CHECK_MULTITHREAD_UNLOCK();
#endif
}



/**
 * Set the current context pointer for this thread.
 * The context pointer is an opaque type which should be cast to
 * void from the real context pointer type.
 */
PUBLIC void
_glapi_set_context(void *context)
{
#if defined(GLX_USE_TLS)
   _glapi_tls_Context = context;
#elif defined(THREADS)
   _glthread_SetTSD(&ContextTSD, context);
   _glapi_Context = (ThreadSafe) ? NULL : context;
#else
   _glapi_Context = context;
#endif
}



/**
 * Get the current context pointer for this thread.
 * The context pointer is an opaque type which should be cast from
 * void to the real context pointer type.
 */
PUBLIC void *
_glapi_get_context(void)
{
#if defined(GLX_USE_TLS)
   return _glapi_tls_Context;
#elif defined(THREADS)
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

#ifdef USE_X86_ASM

#if defined( GLX_USE_TLS )
extern       GLubyte gl_dispatch_functions_start[];
extern       GLubyte gl_dispatch_functions_end[];
#else
extern const GLubyte gl_dispatch_functions_start[];
#endif

#endif /* USE_X86_ASM */


#if defined(USE_X64_64_ASM) && defined(GLX_USE_TLS)
# define DISPATCH_FUNCTION_SIZE  16
#elif defined(USE_X86_ASM)
# if defined(THREADS) && !defined(GLX_USE_TLS)
#  define DISPATCH_FUNCTION_SIZE  32
# else
#  define DISPATCH_FUNCTION_SIZE  16
# endif
#endif

#ifdef USE_SPARC_ASM
#ifdef GLX_USE_TLS
extern unsigned int __glapi_sparc_tls_stub;
#else
extern unsigned int __glapi_sparc_pthread_stub;
#endif
#endif

#if !defined(DISPATCH_FUNCTION_SIZE) && !defined(XFree86Server) && !defined(XGLServer)
# define NEED_FUNCTION_POINTER
#endif

#if defined(PTHREADS) || defined(GLX_USE_TLS)
/**
 * Perform platform-specific GL API entry-point fixups.
 */
static void
init_glapi_relocs( void )
{
#if defined(USE_X86_ASM) && defined(GLX_USE_TLS) && !defined(GLX_X86_READONLY_TEXT)
    extern unsigned long _x86_get_dispatch(void);
    char run_time_patch[] = {
       0x65, 0xa1, 0, 0, 0, 0 /* movl %gs:0,%eax */
    };
    GLuint *offset = (GLuint *) &run_time_patch[2]; /* 32-bits for x86/32 */
    const GLubyte * const get_disp = (const GLubyte *) run_time_patch;
    GLubyte * curr_func = (GLubyte *) gl_dispatch_functions_start;

    *offset = _x86_get_dispatch();
    while ( curr_func != (GLubyte *) gl_dispatch_functions_end ) {
	(void) memcpy( curr_func, get_disp, sizeof(run_time_patch));
	curr_func += DISPATCH_FUNCTION_SIZE;
    }
#endif
#ifdef USE_SPARC_ASM
    extern void __glapi_sparc_icache_flush(unsigned int *);
    static const unsigned int template[] = {
#ifdef GLX_USE_TLS
	0x05000000, /* sethi %hi(_glapi_tls_Dispatch), %g2 */
	0x8730e00a, /* srl %g3, 10, %g3 */
	0x8410a000, /* or %g2, %lo(_glapi_tls_Dispatch), %g2 */
#ifdef __arch64__
	0xc259c002, /* ldx [%g7 + %g2], %g1 */
	0xc2584003, /* ldx [%g1 + %g3], %g1 */
#else
	0xc201c002, /* ld [%g7 + %g2], %g1 */
	0xc2004003, /* ld [%g1 + %g3], %g1 */
#endif
	0x81c04000, /* jmp %g1 */
	0x01000000, /* nop  */
#else
#ifdef __arch64__
	0x03000000, /* 64-bit 0x00 --> sethi %hh(_glapi_Dispatch), %g1 */
	0x05000000, /* 64-bit 0x04 --> sethi %lm(_glapi_Dispatch), %g2 */
	0x82106000, /* 64-bit 0x08 --> or %g1, %hm(_glapi_Dispatch), %g1 */
	0x8730e00a, /* 64-bit 0x0c --> srl %g3, 10, %g3 */
	0x83287020, /* 64-bit 0x10 --> sllx %g1, 32, %g1 */
	0x82004002, /* 64-bit 0x14 --> add %g1, %g2, %g1 */
	0xc2586000, /* 64-bit 0x18 --> ldx [%g1 + %lo(_glapi_Dispatch)], %g1 */
#else
	0x03000000, /* 32-bit 0x00 --> sethi %hi(_glapi_Dispatch), %g1 */
	0x8730e00a, /* 32-bit 0x04 --> srl %g3, 10, %g3 */
	0xc2006000, /* 32-bit 0x08 --> ld [%g1 + %lo(_glapi_Dispatch)], %g1 */
#endif
	0x80a06000, /*             --> cmp %g1, 0 */
	0x02800005, /*             --> be +4*5 */
	0x01000000, /*             -->  nop  */
#ifdef __arch64__
	0xc2584003, /* 64-bit      --> ldx [%g1 + %g3], %g1 */
#else
	0xc2004003, /* 32-bit      --> ld [%g1 + %g3], %g1 */
#endif
	0x81c04000, /*             --> jmp %g1 */
	0x01000000, /*             --> nop  */
#ifdef __arch64__
	0x9de3bf80, /* 64-bit      --> save  %sp, -128, %sp */
#else
	0x9de3bfc0, /* 32-bit      --> save  %sp, -64, %sp */
#endif
	0xa0100003, /*             --> mov  %g3, %l0 */
	0x40000000, /*             --> call _glapi_get_dispatch */
	0x01000000, /*             -->  nop */
	0x82100008, /*             --> mov %o0, %g1 */
	0x86100010, /*             --> mov %l0, %g3 */
	0x10bffff7, /*             --> ba -4*9 */
	0x81e80000, /*             -->  restore  */
#endif
    };
#ifdef GLX_USE_TLS
    extern unsigned long __glapi_sparc_get_dispatch(void);
    unsigned int *code = &__glapi_sparc_tls_stub;
    unsigned long dispatch = __glapi_sparc_get_dispatch();
#else
    unsigned int *code = &__glapi_sparc_pthread_stub;
    unsigned long dispatch = (unsigned long) &_glapi_Dispatch;
    unsigned long call_dest = (unsigned long ) &_glapi_get_dispatch;
    int idx;
#endif

#if defined(GLX_USE_TLS)
    code[0] = template[0] | (dispatch >> 10);
    code[1] = template[1];
    __glapi_sparc_icache_flush(&code[0]);
    code[2] = template[2] | (dispatch & 0x3ff);
    code[3] = template[3];
    __glapi_sparc_icache_flush(&code[2]);
    code[4] = template[4];
    code[5] = template[5];
    __glapi_sparc_icache_flush(&code[4]);
    code[6] = template[6];
    __glapi_sparc_icache_flush(&code[6]);
#else
#if defined(__arch64__)
    code[0] = template[0] | (dispatch >> (32 + 10));
    code[1] = template[1] | ((dispatch & 0xffffffff) >> 10);
    __glapi_sparc_icache_flush(&code[0]);
    code[2] = template[2] | ((dispatch >> 32) & 0x3ff);
    code[3] = template[3];
    __glapi_sparc_icache_flush(&code[2]);
    code[4] = template[4];
    code[5] = template[5];
    __glapi_sparc_icache_flush(&code[4]);
    code[6] = template[6] | (dispatch & 0x3ff);
    idx = 7;
#else
    code[0] = template[0] | (dispatch >> 10);
    code[1] = template[1];
    __glapi_sparc_icache_flush(&code[0]);
    code[2] = template[2] | (dispatch & 0x3ff);
    idx = 3;
#endif
    code[idx + 0] = template[idx + 0];
    __glapi_sparc_icache_flush(&code[idx - 1]);
    code[idx + 1] = template[idx + 1];
    code[idx + 2] = template[idx + 2];
    __glapi_sparc_icache_flush(&code[idx + 1]);
    code[idx + 3] = template[idx + 3];
    code[idx + 4] = template[idx + 4];
    __glapi_sparc_icache_flush(&code[idx + 3]);
    code[idx + 5] = template[idx + 5];
    code[idx + 6] = template[idx + 6];
    __glapi_sparc_icache_flush(&code[idx + 5]);
    code[idx + 7] = template[idx + 7];
    code[idx + 8] = template[idx + 8] |
	    (((call_dest - ((unsigned long) &code[idx + 8]))
	      >> 2) & 0x3fffffff);
    __glapi_sparc_icache_flush(&code[idx + 7]);
    code[idx + 9] = template[idx + 9];
    code[idx + 10] = template[idx + 10];
    __glapi_sparc_icache_flush(&code[idx + 9]);
    code[idx + 11] = template[idx + 11];
    code[idx + 12] = template[idx + 12];
    __glapi_sparc_icache_flush(&code[idx + 11]);
    code[idx + 13] = template[idx + 13];
    __glapi_sparc_icache_flush(&code[idx + 13]);
#endif
#endif
}
#endif /* defined(PTHREADS) || defined(GLX_USE_TLS) */


/**
 * Set the global or per-thread dispatch table pointer.
 * If the dispatch parameter is NULL we'll plug in the no-op dispatch
 * table (__glapi_noop_table).
 */
PUBLIC void
_glapi_set_dispatch(struct _glapi_table *dispatch)
{
#if defined(PTHREADS) || defined(GLX_USE_TLS)
   static pthread_once_t once_control = PTHREAD_ONCE_INIT;
   pthread_once( & once_control, init_glapi_relocs );
#endif

   if (!dispatch) {
      /* use the no-op functions */
      dispatch = (struct _glapi_table *) __glapi_noop_table;
   }
#ifdef DEBUG
   else {
      _glapi_check_table(dispatch);
   }
#endif

#if defined(GLX_USE_TLS)
   _glapi_tls_Dispatch = dispatch;
#elif defined(THREADS)
   _glthread_SetTSD(&_gl_DispatchTSD, (void *) dispatch);
   _glapi_Dispatch = (ThreadSafe) ? NULL : dispatch;
#else /*THREADS*/
   _glapi_Dispatch = dispatch;
#endif /*THREADS*/
}



/**
 * Return pointer to current dispatch table for calling thread.
 */
PUBLIC struct _glapi_table *
_glapi_get_dispatch(void)
{
   struct _glapi_table * api;
#if defined(GLX_USE_TLS)
   api = _glapi_tls_Dispatch;
#elif defined(THREADS)
   api = (ThreadSafe)
     ? (struct _glapi_table *) _glthread_GetTSD(&_gl_DispatchTSD)
     : _glapi_Dispatch;
#else
   api = _glapi_Dispatch;
#endif
   return api;
}




/*
 * The dispatch table size (number of entries) is the size of the
 * _glapi_table struct plus the number of dynamic entries we can add.
 * The extra slots can be filled in by DRI drivers that register new extension
 * functions.
 */
#define DISPATCH_TABLE_SIZE (sizeof(struct _glapi_table) / sizeof(void *) + MAX_EXTENSION_FUNCS)


/**
 * Return size of dispatch table struct as number of functions (or
 * slots).
 */
PUBLIC GLuint
_glapi_get_dispatch_table_size(void)
{
   return DISPATCH_TABLE_SIZE;
}


/**
 * Make sure there are no NULL pointers in the given dispatch table.
 * Intended for debugging purposes.
 */
void
_glapi_check_table(const struct _glapi_table *table)
{
#ifdef EXTRA_DEBUG
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
      GLuint blendColorOffset = _glapi_get_proc_offset("glBlendColor");
      char *blendColorFunc = (char*) &table->BlendColor;
      GLuint offset = (blendColorFunc - (char *) table) / sizeof(void *);
      assert(blendColorOffset == _gloffset_BlendColor);
      assert(blendColorOffset == offset);
   }
   {
      GLuint secondaryColor3fOffset = _glapi_get_proc_offset("glSecondaryColor3fEXT");
      char *secondaryColor3fFunc = (char*) &table->SecondaryColor3fEXT;
      GLuint offset = (secondaryColor3fFunc - (char *) table) / sizeof(void *);
      assert(secondaryColor3fOffset == _gloffset_SecondaryColor3fEXT);
      assert(secondaryColor3fOffset == offset);
   }
   {
      GLuint pointParameterivOffset = _glapi_get_proc_offset("glPointParameterivNV");
      char *pointParameterivFunc = (char*) &table->PointParameterivNV;
      GLuint offset = (pointParameterivFunc - (char *) table) / sizeof(void *);
      assert(pointParameterivOffset == _gloffset_PointParameterivNV);
      assert(pointParameterivOffset == offset);
   }
   {
      GLuint setFenceOffset = _glapi_get_proc_offset("glSetFenceNV");
      char *setFenceFunc = (char*) &table->SetFenceNV;
      GLuint offset = (setFenceFunc - (char *) table) / sizeof(void *);
      assert(setFenceOffset == _gloffset_SetFenceNV);
      assert(setFenceOffset == offset);
   }
#else
   (void) table;
#endif
}
