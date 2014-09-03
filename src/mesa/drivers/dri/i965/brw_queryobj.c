/*
 * Copyright © 2008 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

/** @file brw_queryobj.c
 *
 * Support for query objects (GL_ARB_occlusion_query, GL_ARB_timer_query,
 * GL_EXT_transform_feedback, and friends).
 *
 * The hardware provides a PIPE_CONTROL command that can report the number of
 * fragments that passed the depth test, or the hardware timer.  They are
 * appropriately synced with the stage of the pipeline for our extensions'
 * needs.
 */
#include "main/imports.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_state.h"
#include "intel_batchbuffer.h"
#include "intel_reg.h"

/**
 * Emit PIPE_CONTROLs to write the current GPU timestamp into a buffer.
 */
void
brw_write_timestamp(struct brw_context *brw, drm_intel_bo *query_bo, int idx)
{
   if (brw->gen == 6) {
      /* Emit Sandybridge workaround flush: */
      brw_emit_pipe_control_flush(brw,
                                  PIPE_CONTROL_CS_STALL |
                                  PIPE_CONTROL_STALL_AT_SCOREBOARD);
   }

   brw_emit_pipe_control_write(brw, PIPE_CONTROL_WRITE_TIMESTAMP,
                               query_bo, idx * sizeof(uint64_t), 0, 0);
}

/**
 * Emit PIPE_CONTROLs to write the PS_DEPTH_COUNT register into a buffer.
 */
void
brw_write_depth_count(struct brw_context *brw, drm_intel_bo *query_bo, int idx)
{
   /* Emit Sandybridge workaround flush: */
   if (brw->gen == 6)
      intel_emit_post_sync_nonzero_flush(brw);

   brw_emit_pipe_control_write(brw,
                               PIPE_CONTROL_WRITE_DEPTH_COUNT
                               | PIPE_CONTROL_DEPTH_STALL,
                               query_bo, idx * sizeof(uint64_t), 0, 0);
}

/**
 * Wait on the query object's BO and calculate the final result.
 */
static void
brw_queryobj_get_results(struct gl_context *ctx,
			 struct brw_query_object *query)
{
   struct brw_context *brw = brw_context(ctx);

   int i;
   uint64_t *results;

   assert(brw->gen < 6);

   if (query->bo == NULL)
      return;

   /* If the application has requested the query result, but this batch is
    * still contributing to it, flush it now so the results will be present
    * when mapped.
    */
   if (drm_intel_bo_references(brw->batch.bo, query->bo))
      intel_batchbuffer_flush(brw);

   if (unlikely(brw->perf_debug)) {
      if (drm_intel_bo_busy(query->bo)) {
         perf_debug("Stalling on the GPU waiting for a query object.\n");
      }
   }

   drm_intel_bo_map(query->bo, false);
   results = query->bo->virtual;
   switch (query->Base.Target) {
   case GL_TIME_ELAPSED_EXT:
      /* The query BO contains the starting and ending timestamps.
       * Subtract the two and convert to nanoseconds.
       */
      query->Base.Result += 1000 * ((results[1] >> 32) - (results[0] >> 32));
      break;

   case GL_TIMESTAMP:
      /* The query BO contains a single timestamp value in results[0]. */
      query->Base.Result = 1000 * (results[0] >> 32);
      break;

   case GL_SAMPLES_PASSED_ARB:
      /* Loop over pairs of values from the BO, which are the PS_DEPTH_COUNT
       * value at the start and end of the batchbuffer.  Subtract them to
       * get the number of fragments which passed the depth test in each
       * individual batch, and add those differences up to get the number
       * of fragments for the entire query.
       *
       * Note that query->Base.Result may already be non-zero.  We may have
       * run out of space in the query's BO and allocated a new one.  If so,
       * this function was already called to accumulate the results so far.
       */
      for (i = 0; i < query->last_index; i++) {
	 query->Base.Result += results[i * 2 + 1] - results[i * 2];
      }
      break;

   case GL_ANY_SAMPLES_PASSED:
   case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
      /* If the starting and ending PS_DEPTH_COUNT from any of the batches
       * differ, then some fragments passed the depth test.
       */
      for (i = 0; i < query->last_index; i++) {
	 if (results[i * 2 + 1] != results[i * 2]) {
            query->Base.Result = GL_TRUE;
            break;
         }
      }
      break;

   default:
      unreachable("Unrecognized query target in brw_queryobj_get_results()");
   }
   drm_intel_bo_unmap(query->bo);

   /* Now that we've processed the data stored in the query's buffer object,
    * we can release it.
    */
   drm_intel_bo_unreference(query->bo);
   query->bo = NULL;
}

/**
 * The NewQueryObject() driver hook.
 *
 * Allocates and initializes a new query object.
 */
static struct gl_query_object *
brw_new_query_object(struct gl_context *ctx, GLuint id)
{
   struct brw_query_object *query;

   query = calloc(1, sizeof(struct brw_query_object));

   query->Base.Id = id;
   query->Base.Result = 0;
   query->Base.Active = false;
   query->Base.Ready = true;

   return &query->Base;
}

/**
 * The DeleteQuery() driver hook.
 */
static void
brw_delete_query(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_query_object *query = (struct brw_query_object *)q;

   drm_intel_bo_unreference(query->bo);
   free(query);
}

/**
 * Gen4-5 driver hook for glBeginQuery().
 *
 * Initializes driver structures and emits any GPU commands required to begin
 * recording data for the query.
 */
static void
brw_begin_query(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_query_object *query = (struct brw_query_object *)q;

   assert(brw->gen < 6);

   switch (query->Base.Target) {
   case GL_TIME_ELAPSED_EXT:
      /* For timestamp queries, we record the starting time right away so that
       * we measure the full time between BeginQuery and EndQuery.  There's
       * some debate about whether this is the right thing to do.  Our decision
       * is based on the following text from the ARB_timer_query extension:
       *
       * "(5) Should the extension measure total time elapsed between the full
       *      completion of the BeginQuery and EndQuery commands, or just time
       *      spent in the graphics library?
       *
       *  RESOLVED:  This extension will measure the total time elapsed
       *  between the full completion of these commands.  Future extensions
       *  may implement a query to determine time elapsed at different stages
       *  of the graphics pipeline."
       *
       * We write a starting timestamp now (at index 0).  At EndQuery() time,
       * we'll write a second timestamp (at index 1), and subtract the two to
       * obtain the time elapsed.  Notably, this includes time elapsed while
       * the system was doing other work, such as running other applications.
       */
      drm_intel_bo_unreference(query->bo);
      query->bo = drm_intel_bo_alloc(brw->bufmgr, "timer query", 4096, 4096);
      brw_write_timestamp(brw, query->bo, 0);
      break;

   case GL_ANY_SAMPLES_PASSED:
   case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
   case GL_SAMPLES_PASSED_ARB:
      /* For occlusion queries, we delay taking an initial sample until the
       * first drawing occurs in this batch.  See the reasoning in the comments
       * for brw_emit_query_begin() below.
       *
       * Since we're starting a new query, we need to be sure to throw away
       * any previous occlusion query results.
       */
      drm_intel_bo_unreference(query->bo);
      query->bo = NULL;
      query->last_index = -1;

      brw->query.obj = query;

      /* Depth statistics on Gen4 require strange workarounds, so we try to
       * avoid them when necessary.  They're required for occlusion queries,
       * so turn them on now.
       */
      brw->stats_wm++;
      brw->state.dirty.brw |= BRW_NEW_STATS_WM;
      break;

   default:
      unreachable("Unrecognized query target in brw_begin_query()");
   }
}

/**
 * Gen4-5 driver hook for glEndQuery().
 *
 * Emits GPU commands to record a final query value, ending any data capturing.
 * However, the final result isn't necessarily available until the GPU processes
 * those commands.  brw_queryobj_get_results() processes the captured data to
 * produce the final result.
 */
static void
brw_end_query(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_query_object *query = (struct brw_query_object *)q;

   assert(brw->gen < 6);

   switch (query->Base.Target) {
   case GL_TIME_ELAPSED_EXT:
      /* Write the final timestamp. */
      brw_write_timestamp(brw, query->bo, 1);
      break;

   case GL_ANY_SAMPLES_PASSED:
   case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
   case GL_SAMPLES_PASSED_ARB:

      /* No query->bo means that EndQuery was called after BeginQuery with no
       * intervening drawing. Rather than doing nothing at all here in this
       * case, we emit the query_begin and query_end state to the
       * hardware. This is to guarantee that waiting on the result of this
       * empty state will cause all previous queries to complete at all, as
       * required by the specification:
       *
       * 	It must always be true that if any query object
       *	returns a result available of TRUE, all queries of the
       *	same type issued prior to that query must also return
       *	TRUE. [Open GL 4.3 (Core Profile) Section 4.2.1]
       */
      if (!query->bo) {
         brw_emit_query_begin(brw);
      }

      assert(query->bo);

      brw_emit_query_end(brw);

      brw->query.obj = NULL;

      brw->stats_wm--;
      brw->state.dirty.brw |= BRW_NEW_STATS_WM;
      break;

   default:
      unreachable("Unrecognized query target in brw_end_query()");
   }
}

/**
 * The Gen4-5 WaitQuery() driver hook.
 *
 * Wait for a query result to become available and return it.  This is the
 * backing for glGetQueryObjectiv() with the GL_QUERY_RESULT pname.
 */
static void brw_wait_query(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_query_object *query = (struct brw_query_object *)q;

   assert(brw_context(ctx)->gen < 6);

   brw_queryobj_get_results(ctx, query);
   query->Base.Ready = true;
}

/**
 * The Gen4-5 CheckQuery() driver hook.
 *
 * Checks whether a query result is ready yet.  If not, flushes.
 * This is the backing for glGetQueryObjectiv()'s QUERY_RESULT_AVAILABLE pname.
 */
static void brw_check_query(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_query_object *query = (struct brw_query_object *)q;

   assert(brw->gen < 6);

   /* From the GL_ARB_occlusion_query spec:
    *
    *     "Instead of allowing for an infinite loop, performing a
    *      QUERY_RESULT_AVAILABLE_ARB will perform a flush if the result is
    *      not ready yet on the first time it is queried.  This ensures that
    *      the async query will return true in finite time.
    */
   if (query->bo && drm_intel_bo_references(brw->batch.bo, query->bo))
      intel_batchbuffer_flush(brw);

   if (query->bo == NULL || !drm_intel_bo_busy(query->bo)) {
      brw_queryobj_get_results(ctx, query);
      query->Base.Ready = true;
   }
}

/**
 * Ensure there query's BO has enough space to store a new pair of values.
 *
 * If not, gather the existing BO's results and create a new buffer of the
 * same size.
 */
static void
ensure_bo_has_space(struct gl_context *ctx, struct brw_query_object *query)
{
   struct brw_context *brw = brw_context(ctx);

   assert(brw->gen < 6);

   if (!query->bo || query->last_index * 2 + 1 >= 4096 / sizeof(uint64_t)) {

      if (query->bo != NULL) {
         /* The old query BO did not have enough space, so we allocated a new
          * one.  Gather the results so far (adding up the differences) and
          * release the old BO.
          */
         brw_queryobj_get_results(ctx, query);
      }

      query->bo = drm_intel_bo_alloc(brw->bufmgr, "query", 4096, 1);
      query->last_index = 0;
   }
}

/**
 * Record the PS_DEPTH_COUNT value (for occlusion queries) just before
 * primitive drawing.
 *
 * In a pre-hardware context world, the single PS_DEPTH_COUNT register is
 * shared among all applications using the GPU.  However, our query value
 * needs to only include fragments generated by our application/GL context.
 *
 * To accommodate this, we record PS_DEPTH_COUNT at the start and end of
 * each batchbuffer (technically, the first primitive drawn and flush time).
 * Subtracting each pair of values calculates the change in PS_DEPTH_COUNT
 * caused by a batchbuffer.  Since there is no preemption inside batches,
 * this is guaranteed to only measure the effects of our current application.
 *
 * Adding each of these differences (in case drawing is done over many batches)
 * produces the final expected value.
 *
 * In a world with hardware contexts, PS_DEPTH_COUNT is saved and restored
 * as part of the context state, so this is unnecessary, and skipped.
 */
void
brw_emit_query_begin(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   struct brw_query_object *query = brw->query.obj;

   if (brw->hw_ctx)
      return;

   /* Skip if we're not doing any queries, or we've already recorded the
    * initial query value for this batchbuffer.
    */
   if (!query || brw->query.begin_emitted)
      return;

   ensure_bo_has_space(ctx, query);

   brw_write_depth_count(brw, query->bo, query->last_index * 2);

   brw->query.begin_emitted = true;
}

/**
 * Called at batchbuffer flush to get an ending PS_DEPTH_COUNT
 * (for non-hardware context platforms).
 *
 * See the explanation in brw_emit_query_begin().
 */
void
brw_emit_query_end(struct brw_context *brw)
{
   struct brw_query_object *query = brw->query.obj;

   if (brw->hw_ctx)
      return;

   if (!brw->query.begin_emitted)
      return;

   brw_write_depth_count(brw, query->bo, query->last_index * 2 + 1);

   brw->query.begin_emitted = false;
   query->last_index++;
}

/**
 * Driver hook for glQueryCounter().
 *
 * This handles GL_TIMESTAMP queries, which perform a pipelined read of the
 * current GPU time.  This is unlike GL_TIME_ELAPSED, which measures the
 * time while the query is active.
 */
static void
brw_query_counter(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_query_object *query = (struct brw_query_object *) q;

   assert(q->Target == GL_TIMESTAMP);

   drm_intel_bo_unreference(query->bo);
   query->bo = drm_intel_bo_alloc(brw->bufmgr, "timestamp query", 4096, 4096);
   brw_write_timestamp(brw, query->bo, 0);
}

/**
 * Read the TIMESTAMP register immediately (in a non-pipelined fashion).
 *
 * This is used to implement the GetTimestamp() driver hook.
 */
static uint64_t
brw_get_timestamp(struct gl_context *ctx)
{
   struct brw_context *brw = brw_context(ctx);
   uint64_t result = 0;

   drm_intel_reg_read(brw->bufmgr, TIMESTAMP, &result);

   /* See logic in brw_queryobj_get_results() */
   result = result >> 32;
   result *= 80;
   result &= (1ull << 36) - 1;

   return result;
}

/* Initialize query object functions used on all generations. */
void brw_init_common_queryobj_functions(struct dd_function_table *functions)
{
   functions->NewQueryObject = brw_new_query_object;
   functions->DeleteQuery = brw_delete_query;
   functions->QueryCounter = brw_query_counter;
   functions->GetTimestamp = brw_get_timestamp;
}

/* Initialize Gen4/5-specific query object functions. */
void gen4_init_queryobj_functions(struct dd_function_table *functions)
{
   functions->BeginQuery = brw_begin_query;
   functions->EndQuery = brw_end_query;
   functions->CheckQuery = brw_check_query;
   functions->WaitQuery = brw_wait_query;
}
