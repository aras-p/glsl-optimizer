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
 *   Brian Paul
 */


/* #include "glxheader.h" */
/* #include "xmesaP.h" */

#include "util/u_simple_screen.h"
#include "util/u_inlines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "i965simple/brw_winsys.h"
#include "xlib_brw_aub.h"
#include "xlib_brw.h"




#define XBCWS_BATCHBUFFER_SIZE 1024


/* The backend to the brw driver (ie struct brw_winsys) is actually a
 * per-context entity.
 */
struct xlib_brw_context_winsys {
   struct brw_winsys brw_context_winsys;   /**< batch buffer funcs */
   struct aub_context *aub;
                         
   struct pipe_winsys *pipe_winsys;

   unsigned batch_data[XBCWS_BATCHBUFFER_SIZE];
   unsigned batch_nr;
   unsigned batch_size;
   unsigned batch_alloc;
};


/* Turn a brw_winsys into an xlib_brw_context_winsys:
 */
static inline struct xlib_brw_context_winsys *
xlib_brw_context_winsys( struct brw_winsys *sws )
{
   return (struct xlib_brw_context_winsys *)sws;
}


/* Simple batchbuffer interface:
 */

static unsigned *xbcws_batch_start( struct brw_winsys *sws,
					 unsigned dwords,
					 unsigned relocs )
{
   struct xlib_brw_context_winsys *xbcws = xlib_brw_context_winsys(sws);

   if (xbcws->batch_size < xbcws->batch_nr + dwords)
      return NULL;

   xbcws->batch_alloc = xbcws->batch_nr + dwords;
   return (void *)1;			/* not a valid pointer! */
}

static void xbcws_batch_dword( struct brw_winsys *sws,
				    unsigned dword )
{
   struct xlib_brw_context_winsys *xbcws = xlib_brw_context_winsys(sws);

   assert(xbcws->batch_nr < xbcws->batch_alloc);
   xbcws->batch_data[xbcws->batch_nr++] = dword;
}

static void xbcws_batch_reloc( struct brw_winsys *sws,
			     struct pipe_buffer *buf,
			     unsigned access_flags,
			     unsigned delta )
{
   struct xlib_brw_context_winsys *xbcws = xlib_brw_context_winsys(sws);

   assert(xbcws->batch_nr < xbcws->batch_alloc);
   xbcws->batch_data[xbcws->batch_nr++] = 
      ( xlib_brw_get_buffer_offset( NULL, buf, access_flags ) +
        delta );
}

static void xbcws_batch_end( struct brw_winsys *sws )
{
   struct xlib_brw_context_winsys *xbcws = xlib_brw_context_winsys(sws);

   assert(xbcws->batch_nr <= xbcws->batch_alloc);
   xbcws->batch_alloc = 0;
}

static void xbcws_batch_flush( struct brw_winsys *sws,
				    struct pipe_fence_handle **fence )
{
   struct xlib_brw_context_winsys *xbcws = xlib_brw_context_winsys(sws);
   assert(xbcws->batch_nr <= xbcws->batch_size);

   if (xbcws->batch_nr) {
      xlib_brw_commands_aub( xbcws->pipe_winsys,
                             xbcws->batch_data,
                             xbcws->batch_nr );
   }

   xbcws->batch_nr = 0;
}

  

/* Really a per-device function, just pass through:
 */
static unsigned xbcws_get_buffer_offset( struct brw_winsys *sws,
                                         struct pipe_buffer *buf,
                                         unsigned access_flags )
{
   struct xlib_brw_context_winsys *xbcws = xlib_brw_context_winsys(sws);

   return xlib_brw_get_buffer_offset( xbcws->pipe_winsys,
                                      buf,
                                      access_flags );
}


/* Really a per-device function, just pass through:
 */
static void xbcws_buffer_subdata_typed( struct brw_winsys *sws,
                                       struct pipe_buffer *buf,
                                       unsigned long offset, 
                                       unsigned long size, 
                                       const void *data,
                                       unsigned data_type )
{
   struct xlib_brw_context_winsys *xbcws = xlib_brw_context_winsys(sws);

   xlib_brw_buffer_subdata_typed( xbcws->pipe_winsys,
                                  buf,
                                  offset,
                                  size,
                                  data,
                                  data_type );
}


/**
 * Create i965 hardware rendering context, but plugged into a
 * dump-to-aubfile backend.
 */
struct pipe_context *
xlib_create_brw_context( struct pipe_screen *screen,
                         void *unused )
{
   struct xlib_brw_context_winsys *xbcws = CALLOC_STRUCT( xlib_brw_context_winsys );
   
   /* Fill in this struct with callbacks that i965simple will need to
    * communicate with the window system, buffer manager, etc. 
    */
   xbcws->brw_context_winsys.batch_start = xbcws_batch_start;
   xbcws->brw_context_winsys.batch_dword = xbcws_batch_dword;
   xbcws->brw_context_winsys.batch_reloc = xbcws_batch_reloc;
   xbcws->brw_context_winsys.batch_end = xbcws_batch_end;
   xbcws->brw_context_winsys.batch_flush = xbcws_batch_flush;
   xbcws->brw_context_winsys.buffer_subdata_typed = xbcws_buffer_subdata_typed;
   xbcws->brw_context_winsys.get_buffer_offset = xbcws_get_buffer_offset;

   xbcws->pipe_winsys = screen->winsys; /* redundant */

   xbcws->batch_size = XBCWS_BATCHBUFFER_SIZE;

   /* Create the i965simple context:
    */
#ifdef GALLIUM_CELL
   return NULL;
#else
   return brw_create( screen,
		      &xbcws->brw_context_winsys,
		      0 );
#endif
}
