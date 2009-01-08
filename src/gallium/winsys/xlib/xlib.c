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

#include "xlib_trace.h"
#include "xlib_softpipe.h"
#include "xlib_brw.h"
#include "xm_winsys.h"

#include <stdlib.h>
#include <assert.h>

/* Todo, replace all this with callback-structs provided by the
 * individual implementations.
 */

enum mode {
   MODE_TRACE,
   MODE_BRW,
   MODE_CELL,
   MODE_SOFTPIPE
};

static enum mode xlib_mode;

static enum mode get_mode()
{
   if (getenv("XMESA_TRACE"))
      return MODE_TRACE;

   if (getenv("XMESA_BRW"))
      return MODE_BRW;

#ifdef GALLIUM_CELL
   if (!getenv("GALLIUM_NOCELL")) 
      return MODE_CELL;
#endif

   return MODE_SOFTPIPE;
}


struct pipe_winsys *
xmesa_create_pipe_winsys( void )
{
   xlib_mode = get_mode();

   switch (xlib_mode) {
   case MODE_TRACE:
      return xlib_create_trace_winsys();
   case MODE_BRW:
      return xlib_create_brw_winsys();
   case MODE_CELL:
      return xlib_create_cell_winsys();
   case MODE_SOFTPIPE:
      return xlib_create_softpipe_winsys();
   default:
      assert(0);
      return NULL;
   }
}

struct pipe_screen *
xmesa_create_pipe_screen( struct pipe_winsys *winsys )
{
   switch (xlib_mode) {
   case MODE_TRACE:
      return xlib_create_trace_screen( winsys );
   case MODE_BRW:
      return xlib_create_brw_screen( winsys );
   case MODE_CELL:
      return xlib_create_cell_screen( winsys );
   case MODE_SOFTPIPE:
      return xlib_create_softpipe_screen( winsys );
   default:
      assert(0);
      return NULL;
   }
}

struct pipe_context *
xmesa_create_pipe_context( struct pipe_screen *screen,
                           void *priv )
{
   switch (xlib_mode) {
   case MODE_TRACE:
      return xlib_create_trace_context( screen, priv );
   case MODE_BRW:
      return xlib_create_brw_context( screen, priv );
   case MODE_CELL:
      return xlib_create_cell_context( screen, priv );
   case MODE_SOFTPIPE:
      return xlib_create_softpipe_context( screen, priv );
   default:
      assert(0);
      return NULL;
   }
}

void
xmesa_display_surface( struct xmesa_buffer *buffer,
                       struct pipe_surface *surf )
{
   switch (xlib_mode) {
   case MODE_TRACE:
      xlib_trace_display_surface( buffer, surf );
      break;
   case MODE_BRW:
      xlib_brw_display_surface( buffer, surf );
      break;
   case MODE_CELL:
      xlib_cell_display_surface( buffer, surf );
      break;
   case MODE_SOFTPIPE:
      xlib_softpipe_display_surface( buffer, surf );
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
