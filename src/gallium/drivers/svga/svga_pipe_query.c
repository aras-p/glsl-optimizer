/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

#include "pipe/p_state.h"
#include "pipe/p_context.h"
#include "util/u_memory.h"

#include "svga_cmd.h"
#include "svga_context.h"
#include "svga_screen.h"
#include "svga_resource_buffer.h"
#include "svga_winsys.h"
#include "svga_debug.h"


/* Fixme: want a public base class for all pipe structs, even if there
 * isn't much in them.
 */
struct pipe_query {
   int dummy;
};


struct svga_query {
   struct pipe_query base;
   unsigned type;                  /**< PIPE_QUERY_x or SVGA_QUERY_x */
   SVGA3dQueryType svga_type;      /**< SVGA3D_QUERYTYPE_x or unused */

   /** For PIPE_QUERY_OCCLUSION_COUNTER / SVGA3D_QUERYTYPE_OCCLUSION */
   struct svga_winsys_buffer *hwbuf;
   volatile SVGA3dQueryResult *queryResult;
   struct pipe_fence_handle *fence;

   /** For non-GPU SVGA_QUERY_x queries */
   uint64_t begin_count, end_count;
};


/** cast wrapper */
static INLINE struct svga_query *
svga_query( struct pipe_query *q )
{
   return (struct svga_query *)q;
}


static boolean
svga_get_query_result(struct pipe_context *pipe,
                      struct pipe_query *q,
                      boolean wait,
                      union pipe_query_result *result);


static struct pipe_query *
svga_create_query(struct pipe_context *pipe,
                  unsigned query_type,
                  unsigned index)
{
   struct svga_context *svga = svga_context( pipe );
   struct svga_screen *svgascreen = svga_screen(pipe->screen);
   struct svga_winsys_screen *sws = svgascreen->sws;
   struct svga_query *sq;

   SVGA_DBG(DEBUG_QUERY, "%s\n", __FUNCTION__);

   sq = CALLOC_STRUCT(svga_query);
   if (!sq)
      goto no_sq;

   switch (query_type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
      sq->svga_type = SVGA3D_QUERYTYPE_OCCLUSION;

      sq->hwbuf = svga_winsys_buffer_create(svga, 1,
                                            SVGA_BUFFER_USAGE_PINNED,
                                            sizeof *sq->queryResult);
      if (!sq->hwbuf) {
         debug_printf("svga: failed to alloc query object!\n");
         goto no_hwbuf;
      }

      sq->queryResult = (SVGA3dQueryResult *)
         sws->buffer_map(sws, sq->hwbuf, PIPE_TRANSFER_WRITE);
      if (!sq->queryResult) {
         debug_printf("svga: failed to map query object!\n");
         goto no_query_result;
      }

      sq->queryResult->totalSize = sizeof *sq->queryResult;
      sq->queryResult->state = SVGA3D_QUERYSTATE_NEW;

      /* We request the buffer to be pinned and assume it is always mapped.
       * The reason is that we don't want to wait for fences when checking the
       * query status.
       */
      sws->buffer_unmap(sws, sq->hwbuf);
      break;
   case SVGA_QUERY_DRAW_CALLS:
   case SVGA_QUERY_FALLBACKS:
   case SVGA_QUERY_MEMORY_USED:
      break;
   default:
      assert(!"unexpected query type in svga_create_query()");
   }

   sq->type = query_type;

   return &sq->base;

no_query_result:
   sws->buffer_destroy(sws, sq->hwbuf);
no_hwbuf:
   FREE(sq);
no_sq:
   return NULL;
}


static void
svga_destroy_query(struct pipe_context *pipe, struct pipe_query *q)
{
   struct svga_screen *svgascreen = svga_screen(pipe->screen);
   struct svga_winsys_screen *sws = svgascreen->sws;
   struct svga_query *sq = svga_query( q );

   SVGA_DBG(DEBUG_QUERY, "%s\n", __FUNCTION__);

   switch (sq->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
      sws->buffer_destroy(sws, sq->hwbuf);
      sq->hwbuf = NULL;
      sws->fence_reference(sws, &sq->fence, NULL);
      break;
   case SVGA_QUERY_DRAW_CALLS:
   case SVGA_QUERY_FALLBACKS:
   case SVGA_QUERY_MEMORY_USED:
      /* nothing */
      break;
   default:
      assert(!"svga: unexpected query type in svga_destroy_query()");
   }

   FREE(sq);
}


static void
svga_begin_query(struct pipe_context *pipe, struct pipe_query *q)
{
   struct svga_screen *svgascreen = svga_screen(pipe->screen);
   struct svga_winsys_screen *sws = svgascreen->sws;
   struct svga_context *svga = svga_context( pipe );
   struct svga_query *sq = svga_query( q );
   enum pipe_error ret;

   SVGA_DBG(DEBUG_QUERY, "%s\n", __FUNCTION__);

   /* Need to flush out buffered drawing commands so that they don't
    * get counted in the query results.
    */
   svga_hwtnl_flush_retry(svga);

   switch (sq->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
      assert(!svga->sq);
      if (sq->queryResult->state == SVGA3D_QUERYSTATE_PENDING) {
         /* The application doesn't care for the pending query result.
          * We cannot let go of the existing buffer and just get a new one
          * because its storage may be reused for other purposes and clobbered
          * by the host when it determines the query result.  So the only
          * option here is to wait for the existing query's result -- not a
          * big deal, given that no sane application would do this.
          */
         uint64_t result;
         svga_get_query_result(pipe, q, TRUE, (void*)&result);
         assert(sq->queryResult->state != SVGA3D_QUERYSTATE_PENDING);
      }

      sq->queryResult->state = SVGA3D_QUERYSTATE_NEW;
      sws->fence_reference(sws, &sq->fence, NULL);

      ret = SVGA3D_BeginQuery(svga->swc, sq->svga_type);
      if (ret != PIPE_OK) {
         svga_context_flush(svga, NULL);
         ret = SVGA3D_BeginQuery(svga->swc, sq->svga_type);
         assert(ret == PIPE_OK);
      }

      svga->sq = sq;
      break;
   case SVGA_QUERY_DRAW_CALLS:
      sq->begin_count = svga->num_draw_calls;
      break;
   case SVGA_QUERY_FALLBACKS:
      sq->begin_count = svga->num_fallbacks;
      break;
   case SVGA_QUERY_MEMORY_USED:
      /* nothing */
      break;
   default:
      assert(!"unexpected query type in svga_begin_query()");
   }
}


static void
svga_end_query(struct pipe_context *pipe, struct pipe_query *q)
{
   struct svga_context *svga = svga_context( pipe );
   struct svga_query *sq = svga_query( q );
   enum pipe_error ret;

   SVGA_DBG(DEBUG_QUERY, "%s\n", __FUNCTION__);

   svga_hwtnl_flush_retry(svga);

   switch (sq->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
      assert(svga->sq == sq);

      /* Set to PENDING before sending EndQuery. */
      sq->queryResult->state = SVGA3D_QUERYSTATE_PENDING;

      ret = SVGA3D_EndQuery( svga->swc, sq->svga_type, sq->hwbuf);
      if (ret != PIPE_OK) {
         svga_context_flush(svga, NULL);
         ret = SVGA3D_EndQuery( svga->swc, sq->svga_type, sq->hwbuf);
         assert(ret == PIPE_OK);
      }

      /* TODO: Delay flushing. We don't really need to flush here, just ensure
       * that there is one flush before svga_get_query_result attempts to get
       * the result.
       */
      svga_context_flush(svga, NULL);

      svga->sq = NULL;
      break;
   case SVGA_QUERY_DRAW_CALLS:
      sq->end_count = svga->num_draw_calls;
      break;
   case SVGA_QUERY_FALLBACKS:
      sq->end_count = svga->num_fallbacks;
      break;
   case SVGA_QUERY_MEMORY_USED:
      /* nothing */
      break;
   default:
      assert(!"unexpected query type in svga_end_query()");
   }
}


static boolean
svga_get_query_result(struct pipe_context *pipe,
                      struct pipe_query *q,
                      boolean wait,
                      union pipe_query_result *vresult)
{
   struct svga_context *svga = svga_context( pipe );
   struct svga_screen *svgascreen = svga_screen( pipe->screen );
   struct svga_winsys_screen *sws = svgascreen->sws;
   struct svga_query *sq = svga_query( q );
   SVGA3dQueryState state;
   uint64_t *result = (uint64_t *) vresult;

   SVGA_DBG(DEBUG_QUERY, "%s wait: %d\n", __FUNCTION__);

   switch (sq->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
      /* The query status won't be updated by the host unless
       * SVGA_3D_CMD_WAIT_FOR_QUERY is emitted. Unfortunately this will cause
       * a synchronous wait on the host.
       */
      if (!sq->fence) {
         enum pipe_error ret;

         ret = SVGA3D_WaitForQuery( svga->swc, sq->svga_type, sq->hwbuf);
         if (ret != PIPE_OK) {
            svga_context_flush(svga, NULL);
            ret = SVGA3D_WaitForQuery( svga->swc, sq->svga_type, sq->hwbuf);
            assert(ret == PIPE_OK);
         }

         svga_context_flush(svga, &sq->fence);

         assert(sq->fence);
      }

      state = sq->queryResult->state;
      if (state == SVGA3D_QUERYSTATE_PENDING) {
         if (!wait)
            return FALSE;
         sws->fence_finish(sws, sq->fence, SVGA_FENCE_FLAG_QUERY);
         state = sq->queryResult->state;
      }

      assert(state == SVGA3D_QUERYSTATE_SUCCEEDED ||
             state == SVGA3D_QUERYSTATE_FAILED);

      *result = (uint64_t) sq->queryResult->result32;
      break;
   case SVGA_QUERY_DRAW_CALLS:
      /* fall-through */
   case SVGA_QUERY_FALLBACKS:
      vresult->u64 = sq->end_count - sq->begin_count;
      break;
   case SVGA_QUERY_MEMORY_USED:
      vresult->u64 = svgascreen->total_resource_bytes;
      break;
   default:
      assert(!"unexpected query type in svga_get_query_result");
   }

   SVGA_DBG(DEBUG_QUERY, "%s result %d\n", __FUNCTION__, (unsigned)*result);

   return TRUE;
}


void
svga_init_query_functions(struct svga_context *svga)
{
   svga->pipe.create_query = svga_create_query;
   svga->pipe.destroy_query = svga_destroy_query;
   svga->pipe.begin_query = svga_begin_query;
   svga->pipe.end_query = svga_end_query;
   svga->pipe.get_query_result = svga_get_query_result;
}
