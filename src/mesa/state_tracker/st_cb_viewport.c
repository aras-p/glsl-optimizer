/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
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

#include "main/glheader.h"
#include "st_context.h"
#include "st_public.h"
#include "st_cb_viewport.h"

#include "pipe/p_context.h"
#include "pipe/p_inlines.h"
#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/internal/p_winsys_screen.h"


static void st_viewport(GLcontext * ctx, GLint x, GLint y,
                        GLsizei width, GLsizei height)
{
   struct st_context *st = ctx->st;

   if (st->pipe->winsys && st->pipe->winsys->update_buffer)
      st->pipe->winsys->update_buffer( st->pipe->winsys,
                                       st->pipe->priv );
}

void st_init_viewport_functions(struct dd_function_table *functions)
{
   functions->Viewport = st_viewport;
}
