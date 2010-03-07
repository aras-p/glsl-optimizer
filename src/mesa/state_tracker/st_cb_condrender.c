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
 * IN NO EVENT SHALL THE AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


/**
 * glBegin/EndCondtionalRender functions
 *
 * \author Brian Paul
 */


#include "main/imports.h"
#include "main/context.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "st_context.h"
#include "st_cb_queryobj.h"
#include "st_cb_condrender.h"


/**
 * Called via ctx->Driver.BeginConditionalRender()
 */
static void
st_BeginConditionalRender(GLcontext *ctx, struct gl_query_object *q,
                          GLenum mode)
{
   struct st_query_object *stq = st_query_object(q);
   struct pipe_context *pipe = ctx->st->pipe;
   uint m;

   switch (mode) {
   case GL_QUERY_WAIT:
      m = PIPE_RENDER_COND_WAIT;
      break;
   case GL_QUERY_NO_WAIT:
      m = PIPE_RENDER_COND_NO_WAIT;
      break;
   case GL_QUERY_BY_REGION_WAIT:
      m = PIPE_RENDER_COND_BY_REGION_WAIT;
      break;
   case GL_QUERY_BY_REGION_NO_WAIT:
      m = PIPE_RENDER_COND_BY_REGION_NO_WAIT;
      break;
   default:
      assert(0 && "bad mode in st_BeginConditionalRender");
      m = PIPE_RENDER_COND_WAIT;
   }

   pipe->render_condition(pipe, stq->pq, m);
}


/**
 * Called via ctx->Driver.BeginConditionalRender()
 */
static void
st_EndConditionalRender(GLcontext *ctx, struct gl_query_object *q)
{
   struct pipe_context *pipe = ctx->st->pipe;
   (void) q;
   pipe->render_condition(pipe, NULL, 0);
}



void st_init_cond_render_functions(struct dd_function_table *functions)
{
   functions->BeginConditionalRender = st_BeginConditionalRender;
   functions->EndConditionalRender = st_EndConditionalRender;
}
