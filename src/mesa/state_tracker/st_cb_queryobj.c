/**************************************************************************
 * 
 * Copyright 2007 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
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

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_screen.h"
#include "st_context.h"
#include "st_cb_queryobj.h"
#include "st_cb_bitmap.h"


static struct gl_query_object *
st_NewQueryObject(struct gl_context *ctx, GLuint id)
{
   struct st_query_object *stq = ST_CALLOC_STRUCT(st_query_object);
   if (stq) {
      stq->base.Id = id;
      stq->base.Ready = GL_TRUE;
      stq->pq = NULL;
      stq->type = PIPE_QUERY_TYPES; /* an invalid value */
      return &stq->base;
   }
   return NULL;
}



static void
st_DeleteQuery(struct gl_context *ctx, struct gl_query_object *q)
{
   struct pipe_context *pipe = st_context(ctx)->pipe;
   struct st_query_object *stq = st_query_object(q);

   if (stq->pq) {
      pipe->destroy_query(pipe, stq->pq);
      stq->pq = NULL;
   }

   if (stq->pq_begin) {
      pipe->destroy_query(pipe, stq->pq_begin);
      stq->pq_begin = NULL;
   }

   free(stq);
}


static void
st_BeginQuery(struct gl_context *ctx, struct gl_query_object *q)
{
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct st_query_object *stq = st_query_object(q);
   unsigned type;

   st_flush_bitmap_cache(st_context(ctx));

   /* convert GL query type to Gallium query type */
   switch (q->Target) {
   case GL_ANY_SAMPLES_PASSED:
   case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
      /* fall-through */
   case GL_SAMPLES_PASSED_ARB:
      type = PIPE_QUERY_OCCLUSION_COUNTER;
      break;
   case GL_PRIMITIVES_GENERATED:
      type = PIPE_QUERY_PRIMITIVES_GENERATED;
      break;
   case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
      type = PIPE_QUERY_PRIMITIVES_EMITTED;
      break;
   case GL_TIME_ELAPSED:
      if (st->has_time_elapsed)
         type = PIPE_QUERY_TIME_ELAPSED;
      else
         type = PIPE_QUERY_TIMESTAMP;
      break;
   default:
      assert(0 && "unexpected query target in st_BeginQuery()");
      return;
   }

   if (stq->type != type) {
      /* free old query of different type */
      if (stq->pq) {
         pipe->destroy_query(pipe, stq->pq);
         stq->pq = NULL;
      }
      if (stq->pq_begin) {
         pipe->destroy_query(pipe, stq->pq_begin);
         stq->pq_begin = NULL;
      }
      stq->type = PIPE_QUERY_TYPES; /* an invalid value */
   }

   if (q->Target == GL_TIME_ELAPSED &&
       type == PIPE_QUERY_TIMESTAMP) {
      /* Determine time elapsed by emitting two timestamp queries. */
      if (!stq->pq_begin) {
         stq->pq_begin = pipe->create_query(pipe, type, 0);
         stq->type = type;
      }
      pipe->end_query(pipe, stq->pq_begin);
   } else {
      if (!stq->pq) {
         stq->pq = pipe->create_query(pipe, type, q->Stream);
         stq->type = type;
      }
      if (stq->pq) {
         pipe->begin_query(pipe, stq->pq);
      }
      else {
         _mesa_error(ctx, GL_OUT_OF_MEMORY, "glBeginQuery");
         return;
      }
   }
   assert(stq->type == type);
}


static void
st_EndQuery(struct gl_context *ctx, struct gl_query_object *q)
{
   struct pipe_context *pipe = st_context(ctx)->pipe;
   struct st_query_object *stq = st_query_object(q);

   st_flush_bitmap_cache(st_context(ctx));

   if ((q->Target == GL_TIMESTAMP ||
        q->Target == GL_TIME_ELAPSED) &&
       !stq->pq) {
      stq->pq = pipe->create_query(pipe, PIPE_QUERY_TIMESTAMP, 0);
      stq->type = PIPE_QUERY_TIMESTAMP;
   }

   if (stq->pq)
      pipe->end_query(pipe, stq->pq);
}


static boolean
get_query_result(struct pipe_context *pipe,
                 struct st_query_object *stq,
                 boolean wait)
{
   if (!stq->pq) {
      /* Only needed in case we failed to allocate the gallium query earlier.
       * Return TRUE so we don't spin on this forever.
       */
      return TRUE;
   }

   if (!pipe->get_query_result(pipe,
                               stq->pq,
                               wait,
                               (void *)&stq->base.Result)) {
      return FALSE;
   }

   if (stq->base.Target == GL_TIME_ELAPSED &&
       stq->type == PIPE_QUERY_TIMESTAMP) {
      /* Calculate the elapsed time from the two timestamp queries */
      GLuint64EXT Result0 = 0;
      assert(stq->pq_begin);
      pipe->get_query_result(pipe, stq->pq_begin, TRUE, (void *)&Result0);
      stq->base.Result -= Result0;
   } else {
      assert(!stq->pq_begin);
   }

   return TRUE;
}


static void
st_WaitQuery(struct gl_context *ctx, struct gl_query_object *q)
{
   struct pipe_context *pipe = st_context(ctx)->pipe;
   struct st_query_object *stq = st_query_object(q);

   /* this function should only be called if we don't have a ready result */
   assert(!stq->base.Ready);

   while (!stq->base.Ready &&
	  !get_query_result(pipe, stq, TRUE))
   {
      /* nothing */
   }
			    
   q->Ready = GL_TRUE;
}


static void
st_CheckQuery(struct gl_context *ctx, struct gl_query_object *q)
{
   struct pipe_context *pipe = st_context(ctx)->pipe;
   struct st_query_object *stq = st_query_object(q);
   assert(!q->Ready);   /* we should not get called if Ready is TRUE */
   q->Ready = get_query_result(pipe, stq, FALSE);
}


static uint64_t
st_GetTimestamp(struct gl_context *ctx)
{
   struct pipe_screen *screen = st_context(ctx)->pipe->screen;

   return screen->get_timestamp(screen);
}


void st_init_query_functions(struct dd_function_table *functions)
{
   functions->NewQueryObject = st_NewQueryObject;
   functions->DeleteQuery = st_DeleteQuery;
   functions->BeginQuery = st_BeginQuery;
   functions->EndQuery = st_EndQuery;
   functions->WaitQuery = st_WaitQuery;
   functions->CheckQuery = st_CheckQuery;
   functions->GetTimestamp = st_GetTimestamp;
}
