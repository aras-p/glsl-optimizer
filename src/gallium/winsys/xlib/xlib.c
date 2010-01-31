/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Bismarck, ND., USA
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * 
 **************************************************************************/

/*
 * Authors:
 *   Keith Whitwell
 */

#include "xlib.h"
#include "xm_winsys.h"

#include <stdlib.h>
#include <assert.h>

/* Todo, replace all this with callback-structs provided by the
 * individual implementations.
 */

enum mode {
   MODE_CELL,
   MODE_LLVMPIPE,
   MODE_SOFTPIPE
};

/* advertise OpenGL support */
PUBLIC const int st_api_OpenGL = 1;

static enum mode get_mode()
{
#ifdef GALLIUM_CELL
   if (!getenv("GALLIUM_NOCELL")) 
      return MODE_CELL;
#endif

#if defined(GALLIUM_LLVMPIPE)
   return MODE_LLVMPIPE;
#else
   return MODE_SOFTPIPE;
#endif
}

static void _init( void ) __attribute__((constructor));

static void _init( void )
{
   enum mode xlib_mode = get_mode();

   switch (xlib_mode) {
   case MODE_CELL:
#if defined(GALLIUM_CELL)
      xmesa_set_driver( &xlib_cell_driver );
#endif
      break;
   case MODE_LLVMPIPE:
#if defined(GALLIUM_LLVMPIPE)
      xmesa_set_driver( &xlib_llvmpipe_driver );
#endif
      break;
   case MODE_SOFTPIPE:
#if defined(GALLIUM_SOFTPIPE)
      xmesa_set_driver( &xlib_softpipe_driver );
#endif
      break;
   default:
      assert(0);
      break;
   }
}


/***********************************************************************
 *
 * Butt-ugly hack to convince the linker not to throw away public GL
 * symbols (they are all referenced from getprocaddress, I guess).
 */
extern void (*linker_foo(const unsigned char *procName))();
extern void (*glXGetProcAddress(const unsigned char *procName))();

extern void (*linker_foo(const unsigned char *procName))()
{
   return glXGetProcAddress(procName);
}


/**
 * When GLX_INDIRECT_RENDERING is defined, some symbols are missing in
 * libglapi.a.  We need to define them here.
 */
#ifdef GLX_INDIRECT_RENDERING

#define GL_GLEXT_PROTOTYPES
#include "GL/gl.h"
#include "glapi/glapi.h"
#include "glapi/glapitable.h"
#include "glapi/glapidispatch.h"

#if defined(USE_MGL_NAMESPACE)
#define NAME(func)  mgl##func
#else
#define NAME(func)  gl##func
#endif

#define DISPATCH(FUNC, ARGS, MESSAGE)		\
   CALL_ ## FUNC(GET_DISPATCH(), ARGS);

#define RETURN_DISPATCH(FUNC, ARGS, MESSAGE) 	\
   return CALL_ ## FUNC(GET_DISPATCH(), ARGS);

/* skip normal ones */
#define _GLAPI_SKIP_NORMAL_ENTRY_POINTS
#include "glapi/glapitemp.h"

#endif /* GLX_INDIRECT_RENDERING */
