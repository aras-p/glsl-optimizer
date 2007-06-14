/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
 
#include "st_context.h"
#include "softpipe/sp_context.h"
#include "st_atom.h"

extern GLboolean xmesa_get_cbuf_details( GLcontext *ctx,
					 void **ptr,
					 GLuint *cpp,
					 GLint *stride,
					 GLuint *format );


/* This is a hack to work with the X11 driver as a test harness
 */
static void update_cbuf_state( struct st_context *st )
{
   struct softpipe_surface cbuf;
   GLboolean ok;

   ok = xmesa_get_cbuf_details( st->ctx,
				(void **)&cbuf.ptr,
				&cbuf.cpp,
				&cbuf.stride,
				&cbuf.format );

   assert(ok);

   if (memcmp(&cbuf, &st->state.cbuf, sizeof(cbuf)) != 0) {
      st->state.cbuf = cbuf;
      st->softpipe->set_cbuf_state( st->softpipe, &cbuf );
   }
}

const struct st_tracked_state st_update_cbuf = {
   .dirty = {
      .mesa = _NEW_BUFFERS,
      .st  = 0,
   },
   .update = update_cbuf_state
};

