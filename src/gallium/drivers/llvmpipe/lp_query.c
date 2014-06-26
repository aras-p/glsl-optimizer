/**************************************************************************
 * 
 * Copyright 2007 VMware, Inc.
 * Copyright 2010 VMware, Inc.
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

/* Authors:
 *    Keith Whitwell, Qicheng Christopher Li, Brian Paul
 */

#include "draw/draw_context.h"
#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "os/os_time.h"
#include "lp_context.h"
#include "lp_flush.h"
#include "lp_fence.h"
#include "lp_query.h"
#include "lp_screen.h"
#include "lp_state.h"
#include "lp_rast.h"


static struct llvmpipe_query *llvmpipe_query( struct pipe_query *p )
{
   return (struct llvmpipe_query *)p;
}

static struct pipe_query *
llvmpipe_create_query(struct pipe_context *pipe, 
                      unsigned type,
                      unsigned index)
{
   struct llvmpipe_query *pq;

   assert(type < PIPE_QUERY_TYPES);

   pq = CALLOC_STRUCT( llvmpipe_query );

   if (pq) {
      pq->type = type;
   }

   return (struct pipe_query *) pq;
}


static void
llvmpipe_destroy_query(struct pipe_context *pipe, struct pipe_query *q)
{
   struct llvmpipe_query *pq = llvmpipe_query(q);

   /* Ideally we would refcount queries & not get destroyed until the
    * last scene had finished with us.
    */
   if (pq->fence) {
      if (!lp_fence_issued(pq->fence))
         llvmpipe_flush(pipe, NULL, __FUNCTION__);

      if (!lp_fence_signalled(pq->fence))
         lp_fence_wait(pq->fence);

      lp_fence_reference(&pq->fence, NULL);
   }

   FREE(pq);
}


static boolean
llvmpipe_get_query_result(struct pipe_context *pipe, 
                          struct pipe_query *q,
                          boolean wait,
                          union pipe_query_result *vresult)
{
   struct llvmpipe_screen *screen = llvmpipe_screen(pipe->screen);
   unsigned num_threads = MAX2(1, screen->num_threads);
   struct llvmpipe_query *pq = llvmpipe_query(q);
   uint64_t *result = (uint64_t *)vresult;
   int i;

   if (pq->fence) {
      /* only have a fence if there was a scene */
      if (!lp_fence_signalled(pq->fence)) {
         if (!lp_fence_issued(pq->fence))
            llvmpipe_flush(pipe, NULL, __FUNCTION__);

         if (!wait)
            return FALSE;

         lp_fence_wait(pq->fence);
      }
   }

   /* Sum the results from each of the threads:
    */
   *result = 0;

   switch (pq->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
      for (i = 0; i < num_threads; i++) {
         *result += pq->end[i];
      }
      break;
   case PIPE_QUERY_OCCLUSION_PREDICATE:
      for (i = 0; i < num_threads; i++) {
         /* safer (still not guaranteed) when there's an overflow */
         vresult->b = vresult->b || pq->end[i];
      }
      break;
   case PIPE_QUERY_TIMESTAMP:
      for (i = 0; i < num_threads; i++) {
         if (pq->end[i] > *result) {
            *result = pq->end[i];
         }
      }
      break;
   case PIPE_QUERY_TIMESTAMP_DISJOINT: {
      struct pipe_query_data_timestamp_disjoint *td =
         (struct pipe_query_data_timestamp_disjoint *)vresult;
      /* os_get_time_nano return nanoseconds */
      td->frequency = UINT64_C(1000000000);
      td->disjoint = FALSE;
   }
      break;
   case PIPE_QUERY_GPU_FINISHED:
      vresult->b = TRUE;
      break;
   case PIPE_QUERY_PRIMITIVES_GENERATED:
      *result = pq->num_primitives_generated;
      break;
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      *result = pq->num_primitives_written;
      break;
   case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      vresult->b = pq->num_primitives_generated > pq->num_primitives_written;
      break;
   case PIPE_QUERY_SO_STATISTICS: {
      struct pipe_query_data_so_statistics *stats =
         (struct pipe_query_data_so_statistics *)vresult;
      stats->num_primitives_written = pq->num_primitives_written;
      stats->primitives_storage_needed = pq->num_primitives_generated;
   }
      break;
   case PIPE_QUERY_PIPELINE_STATISTICS: {
      struct pipe_query_data_pipeline_statistics *stats =
         (struct pipe_query_data_pipeline_statistics *)vresult;
      /* only ps_invocations come from binned query */
      for (i = 0; i < num_threads; i++) {
         pq->stats.ps_invocations += pq->end[i];
      }
      pq->stats.ps_invocations *= LP_RASTER_BLOCK_SIZE * LP_RASTER_BLOCK_SIZE;
      *stats = pq->stats;
   }
      break;
   default:
      assert(0);
      break;
   }

   return TRUE;
}


static void
llvmpipe_begin_query(struct pipe_context *pipe, struct pipe_query *q)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context( pipe );
   struct llvmpipe_query *pq = llvmpipe_query(q);

   /* Check if the query is already in the scene.  If so, we need to
    * flush the scene now.  Real apps shouldn't re-use a query in a
    * frame of rendering.
    */
   if (pq->fence && !lp_fence_issued(pq->fence)) {
      llvmpipe_finish(pipe, __FUNCTION__);
   }


   memset(pq->start, 0, sizeof(pq->start));
   memset(pq->end, 0, sizeof(pq->end));
   lp_setup_begin_query(llvmpipe->setup, pq);

   switch (pq->type) {
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      pq->num_primitives_written = llvmpipe->so_stats.num_primitives_written;
      break;
   case PIPE_QUERY_PRIMITIVES_GENERATED:
      pq->num_primitives_generated = llvmpipe->so_stats.primitives_storage_needed;
      break;
   case PIPE_QUERY_SO_STATISTICS:
      pq->num_primitives_written = llvmpipe->so_stats.num_primitives_written;
      pq->num_primitives_generated = llvmpipe->so_stats.primitives_storage_needed;
      break;
   case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      pq->num_primitives_written = llvmpipe->so_stats.num_primitives_written;
      pq->num_primitives_generated = llvmpipe->so_stats.primitives_storage_needed;
      break;
   case PIPE_QUERY_PIPELINE_STATISTICS:
      /* reset our cache */
      if (llvmpipe->active_statistics_queries == 0) {
         memset(&llvmpipe->pipeline_statistics, 0,
                sizeof(llvmpipe->pipeline_statistics));
      }
      memcpy(&pq->stats, &llvmpipe->pipeline_statistics, sizeof(pq->stats));
      llvmpipe->active_statistics_queries++;
      break;
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
      llvmpipe->active_occlusion_queries++;
      llvmpipe->dirty |= LP_NEW_OCCLUSION_QUERY;
      break;
   default:
      break;
   }
}


static void
llvmpipe_end_query(struct pipe_context *pipe, struct pipe_query *q)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context( pipe );
   struct llvmpipe_query *pq = llvmpipe_query(q);

   lp_setup_end_query(llvmpipe->setup, pq);

   switch (pq->type) {

   case PIPE_QUERY_PRIMITIVES_EMITTED:
      pq->num_primitives_written =
         llvmpipe->so_stats.num_primitives_written - pq->num_primitives_written;
      break;
   case PIPE_QUERY_PRIMITIVES_GENERATED:
      pq->num_primitives_generated =
         llvmpipe->so_stats.primitives_storage_needed - pq->num_primitives_generated;
      break;
   case PIPE_QUERY_SO_STATISTICS:
      pq->num_primitives_written =
         llvmpipe->so_stats.num_primitives_written - pq->num_primitives_written;
      pq->num_primitives_generated =
         llvmpipe->so_stats.primitives_storage_needed - pq->num_primitives_generated;
      break;
   case PIPE_QUERY_SO_OVERFLOW_PREDICATE:
      pq->num_primitives_written =
         llvmpipe->so_stats.num_primitives_written - pq->num_primitives_written;
      pq->num_primitives_generated =
         llvmpipe->so_stats.primitives_storage_needed - pq->num_primitives_generated;
      break;
   case PIPE_QUERY_PIPELINE_STATISTICS:
      pq->stats.ia_vertices =
         llvmpipe->pipeline_statistics.ia_vertices - pq->stats.ia_vertices;
      pq->stats.ia_primitives =
         llvmpipe->pipeline_statistics.ia_primitives - pq->stats.ia_primitives;
      pq->stats.vs_invocations =
         llvmpipe->pipeline_statistics.vs_invocations - pq->stats.vs_invocations;
      pq->stats.gs_invocations =
         llvmpipe->pipeline_statistics.gs_invocations - pq->stats.gs_invocations;
      pq->stats.gs_primitives =
         llvmpipe->pipeline_statistics.gs_primitives - pq->stats.gs_primitives;
      pq->stats.c_invocations =
         llvmpipe->pipeline_statistics.c_invocations - pq->stats.c_invocations;
      pq->stats.c_primitives =
         llvmpipe->pipeline_statistics.c_primitives - pq->stats.c_primitives;
      pq->stats.ps_invocations =
         llvmpipe->pipeline_statistics.ps_invocations - pq->stats.ps_invocations;

      llvmpipe->active_statistics_queries--;
      break;
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_OCCLUSION_PREDICATE:
      assert(llvmpipe->active_occlusion_queries);
      llvmpipe->active_occlusion_queries--;
      llvmpipe->dirty |= LP_NEW_OCCLUSION_QUERY;
      break;
   default:
      break;
   }
}

boolean
llvmpipe_check_render_cond(struct llvmpipe_context *lp)
{
   struct pipe_context *pipe = &lp->pipe;
   boolean b, wait;
   uint64_t result;

   if (!lp->render_cond_query)
      return TRUE; /* no query predicate, draw normally */

   wait = (lp->render_cond_mode == PIPE_RENDER_COND_WAIT ||
           lp->render_cond_mode == PIPE_RENDER_COND_BY_REGION_WAIT);

   b = pipe->get_query_result(pipe, lp->render_cond_query, wait, (void*)&result);
   if (b)
      return (!result == lp->render_cond_cond);
   else
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


