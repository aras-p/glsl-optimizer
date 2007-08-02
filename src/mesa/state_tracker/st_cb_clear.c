/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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
  *   Brian Paul
  */

#include "st_context.h"
#include "glheader.h"
#include "macros.h"
#include "enums.h"
#include "st_context.h"
#include "st_atom.h"
#include "pipe/p_context.h"



/* XXX: doesn't pick up the differences between front/back/left/right
 * clears.  Need to sort that out...
 */
static void st_clear(GLcontext *ctx, GLbitfield mask)
{
   struct st_context *st = ctx->st;
   GLboolean color = (mask & BUFFER_BITS_COLOR) ? GL_TRUE : GL_FALSE;
   GLboolean depth = (mask & BUFFER_BIT_DEPTH) ? GL_TRUE : GL_FALSE;
   GLboolean stencil = (mask & BUFFER_BIT_STENCIL) ? GL_TRUE : GL_FALSE;
   GLboolean accum = (mask & BUFFER_BIT_ACCUM) ? GL_TRUE : GL_FALSE;
   GLboolean fullscreen = 1;	/* :-) */

   /* This makes sure the softpipe has the latest scissor, etc values */
   st_validate_state( st );

   if (fullscreen) {
      /* pipe->clear() should clear a particular surface, so that we
       * can iterate over render buffers at this level and clear the
       * ones GL is asking for.  
       *
       * Will probably need something like pipe->clear_z_stencil() to
       * cope with the special case of paired and unpaired z/stencil
       * buffers, though could perhaps deal with them explicitly at
       * this level.
       */
      st->pipe->clear(st->pipe, color, depth, stencil, accum);
   }
   else {
      /* Convert to geometry, etc:
       */
   }
}


void st_init_cb_clear( struct st_context *st )
{
   struct dd_function_table *functions = &st->ctx->Driver;

   functions->Clear = st_clear;
}


void st_destroy_cb_clear( struct st_context *st )
{
}

