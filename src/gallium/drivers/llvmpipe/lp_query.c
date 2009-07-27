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
#include "lp_context.h"
#include "lp_query.h"
#include "lp_state.h"

struct llvmpipe_query {
   uint64_t start;
   uint64_t end;
};


static struct llvmpipe_query *llvmpipe_query( struct pipe_query *p )
{
   return (struct llvmpipe_query *)p;
}

static struct pipe_query *
llvmpipe_create_query(struct pipe_context *pipe, 
		      unsigned type)
{
   assert(type == PIPE_QUERY_OCCLUSION_COUNTER);
   return (struct pipe_query *)CALLOC_STRUCT( llvmpipe_query );
}


static void
llvmpipe_destroy_query(struct pipe_context *pipe, struct pipe_query *q)
{
   FREE(q);
}


static void
llvmpipe_begin_query(struct pipe_context *pipe, struct pipe_query *q)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context( pipe );
   struct llvmpipe_query *sq = llvmpipe_query(q);
   
   sq->start = llvmpipe->occlusion_count;
   llvmpipe->active_query_count++;
   llvmpipe->dirty |= LP_NEW_QUERY;
}


static void
llvmpipe_end_query(struct pipe_context *pipe, struct pipe_query *q)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context( pipe );
   struct llvmpipe_query *sq = llvmpipe_query(q);

   llvmpipe->active_query_count--;
   sq->end = llvmpipe->occlusion_count;
   llvmpipe->dirty |= LP_NEW_QUERY;
}


static boolean
llvmpipe_get_query_result(struct pipe_context *pipe, 
			  struct pipe_query *q,
			  boolean wait,
			  uint64_t *result )
{
   struct llvmpipe_query *sq = llvmpipe_query(q);
   *result = sq->end - sq->start;
   return TRUE;
}


void llvmpipe_init_query_funcs(struct llvmpipe_context *llvmpipe )
{
   llvmpipe->pipe.create_query = llvmpipe_create_query;
   llvmpipe->pipe.destroy_query = llvmpipe_destroy_query;
   llvmpipe->pipe.begin_query = llvmpipe_begin_query;
   llvmpipe->pipe.end_query = llvmpipe_end_query;
   llvmpipe->pipe.get_query_result = llvmpipe_get_query_result;
}


