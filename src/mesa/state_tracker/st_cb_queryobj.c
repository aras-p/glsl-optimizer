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


/**
 * glBegin/EndQuery interface to pipe
 *
 * \author Brian Paul
 */


#include "main/imports.h"
#include "main/context.h"
#include "main/image.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "st_context.h"
#include "st_cb_queryobj.h"
#include "st_public.h"


/**
 * Do glReadPixels by getting rows from the framebuffer surface with
 * get_tile().  Convert to requested format/type with Mesa image routines.
 * Image transfer ops are done in software too.
 */
static void
st_BeginQuery(GLcontext *ctx, GLenum target, struct gl_query_object *q)
{
   struct pipe_context *pipe = ctx->st->pipe;
   if (target == GL_SAMPLES_PASSED_ARB) {
      pipe->reset_occlusion_counter(pipe);
   }
}


static void
st_EndQuery(GLcontext *ctx, GLenum target, struct gl_query_object *q)
{
   struct pipe_context *pipe = ctx->st->pipe;
   if (target == GL_SAMPLES_PASSED_ARB) {
      q->Result = pipe->get_occlusion_counter(pipe);
   }
}


void st_init_query_functions(struct dd_function_table *functions)
{
   functions->BeginQuery = st_BeginQuery;
   functions->EndQuery = st_EndQuery;
}
