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

/* Author:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "draw/draw_context.h"
#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "sp_context.h"
#include "sp_query.h"
#include "sp_state.h"

struct softpipe_query {
   uint64_t start;
   uint64_t end;
};


static struct softpipe_query *softpipe_query( struct pipe_query *p )
{
   return (struct softpipe_query *)p;
}

static struct pipe_query *
softpipe_create_query(struct pipe_context *pipe, 
		      unsigned type)
{
   assert(type == PIPE_QUERY_OCCLUSION_COUNTER);
   return (struct pipe_query *)CALLOC_STRUCT( softpipe_query );
}


static void
softpipe_destroy_query(struct pipe_context *pipe, struct pipe_query *q)
{
   FREE(q);
}


static void
softpipe_begin_query(struct pipe_context *pipe, struct pipe_query *q)
{
   struct softpipe_context *softpipe = softpipe_context( pipe );
   struct softpipe_query *sq = softpipe_query(q);
   
   sq->start = softpipe->occlusion_count;
   softpipe->active_query_count++;
   softpipe->dirty |= SP_NEW_QUERY;
}


static void
softpipe_end_query(struct pipe_context *pipe, struct pipe_query *q)
{
   struct softpipe_context *softpipe = softpipe_context( pipe );
   struct softpipe_query *sq = softpipe_query(q);

   softpipe->active_query_count--;
   sq->end = softpipe->occlusion_count;
   softpipe->dirty |= SP_NEW_QUERY;
}


static boolean
softpipe_get_query_result(struct pipe_context *pipe, 
			  struct pipe_query *q,
			  boolean wait,
			  uint64_t *result )
{
   struct softpipe_query *sq = softpipe_query(q);
   *result = sq->end - sq->start;
   return TRUE;
}


/**
 * Called by rendering function to check rendering is conditional.
 * \return TRUE if we should render, FALSE if we should skip rendering
 */
boolean
softpipe_check_render_cond(struct softpipe_context *sp)
{
   struct pipe_context *pipe = &sp->pipe;
   boolean b, wait;
   uint64_t result;

   if (!sp->render_cond_query) {
      return TRUE;  /* no query predicate, draw normally */
   }

   wait = (sp->render_cond_mode == PIPE_RENDER_COND_WAIT ||
           sp->render_cond_mode == PIPE_RENDER_COND_BY_REGION_WAIT);

   b = pipe->get_query_result(pipe, sp->render_cond_query, wait, &result);
   if (b)
      return result > 0;
   else
      return TRUE;
}


void softpipe_init_query_funcs(struct softpipe_context *softpipe )
{
   softpipe->pipe.create_query = softpipe_create_query;
   softpipe->pipe.destroy_query = softpipe_destroy_query;
   softpipe->pipe.begin_query = softpipe_begin_query;
   softpipe->pipe.end_query = softpipe_end_query;
   softpipe->pipe.get_query_result = softpipe_get_query_result;
}


