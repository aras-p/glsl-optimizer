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

#include "state_tracker/xlib_sw_winsys.h"
#include "xm_winsys.h"
#include "util/u_debug.h"
#include "softpipe/sp_public.h"
#include "llvmpipe/lp_public.h"
#include "identity/id_public.h"
#include "trace/tr_public.h"
#include "cell/ppu/cell_public.h"


/* advertise OpenGL support */
PUBLIC const int st_api_OpenGL = 1;


static struct pipe_screen *
create_screen( struct sw_winsys *winsys )
{
#if defined(GALLIUM_CELL)
   if (!debug_get_bool_option("GALLIUM_NOCELL", FALSE)) 
      return cell_create_screen( winsys );
#endif

#if defined(GALLIUM_LLVMPIPE)
   return llvmpipe_create_screen( winsys );
#endif

   return softpipe_create_screen( winsys );
}



static struct pipe_screen *
xlib_create_screen( Display *display )
{
   struct sw_winsys *winsys;
   struct pipe_screen *screen;

   winsys = xlib_create_sw_winsys( display );
   if (winsys == NULL)
      return NULL;

   screen = create_screen(winsys);
   if (screen == NULL)
      goto fail;

   /* Finally we have somewhere to inject layers into the stack in a
    * clean fashion:
    */
   if (debug_get_bool_option("GALLIUM_WRAP", FALSE)) {
      screen = identity_screen_create(screen);
   }

   if (debug_get_bool_option("GALLIUM_TRACE", FALSE)) {
      screen = trace_screen_create( screen );
   }

   return screen;

fail:
   if (winsys)
      winsys->destroy( winsys );

   return NULL;
}


struct xm_driver xlib_driver = 
{
   .create_pipe_screen = xlib_create_screen,
};




/* Build the rendering stack.
 */
static void _init( void ) __attribute__((constructor));
static void _init( void )
{
   /* Initialize the xlib libgl code, pass in the winsys:
    */
   xmesa_set_driver( &xlib_driver );
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
