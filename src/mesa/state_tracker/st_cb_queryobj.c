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


struct st_query_object
{
   struct gl_query_object base;
   struct pipe_query_object pq;
};


/**
 * Cast wrapper
 */
static struct st_query_object *
st_query_object(struct gl_query_object *q)
{
   return (struct st_query_object *) q;
}


static struct gl_query_object *
st_NewQueryObject(GLcontext *ctx, GLuint id)
{
   struct st_query_object *stq = CALLOC_STRUCT(st_query_object);
   if (stq) {
      stq->base.Id = id;
      stq->base.Ready = GL_TRUE;
      return &stq->base;
   }
   return NULL;
}


/**
 * Do glReadPixels by getting rows from the framebuffer surface with
 * get_tile().  Convert to requested format/type with Mesa image routines.
 * Image transfer ops are done in software too.
 */
static void
st_BeginQuery(GLcontext *ctx, struct gl_query_object *q)
{
   struct pipe_context *pipe = ctx->st->pipe;
   struct st_query_object *stq = st_query_object(q);

   stq->pq.count = 0;

   switch (q->Target) {
   case GL_SAMPLES_PASSED_ARB:
      stq->pq.type = PIPE_QUERY_OCCLUSION_COUNTER;
      break;
   case GL_PRIMITIVES_GENERATED_NV:
      /* someday */
      stq->pq.type = PIPE_QUERY_PRIMITIVES_GENERATED;
      break;
   case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN_NV:
      /* someday */
      stq->pq.type = PIPE_QUERY_PRIMITIVES_EMITTED;
      break;
   default:
      assert(0);
   }

   pipe->begin_query(pipe, &stq->pq);
}


static void
st_EndQuery(GLcontext *ctx, struct gl_query_object *q)
{
   struct pipe_context *pipe = ctx->st->pipe;
   struct st_query_object *stq = st_query_object(q);

   pipe->end_query(pipe, &stq->pq);
   stq->base.Ready = stq->pq.ready;
   if (stq->base.Ready)
      stq->base.Result = stq->pq.count;
}


static void
st_WaitQuery(GLcontext *ctx, struct gl_query_object *q)
{
   struct pipe_context *pipe = ctx->st->pipe;
   struct st_query_object *stq = st_query_object(q);

   /* this function should only be called if we don't have a ready result */
   assert(!stq->base.Ready);

   pipe->wait_query(pipe, &stq->pq);
   q->Ready = GL_TRUE;
   q->Result = stq->pq.count;
}



void st_init_query_functions(struct dd_function_table *functions)
{
   functions->NewQueryObject = st_NewQueryObject;
   functions->BeginQuery = st_BeginQuery;
   functions->EndQuery = st_EndQuery;
   functions->WaitQuery = st_WaitQuery;
}
