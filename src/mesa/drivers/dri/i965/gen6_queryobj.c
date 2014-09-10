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
 *    Kenneth Graunke <kenneth@whitecape.org>
 */

/** @file gen6_queryobj.c
 *
 * Support for query objects (GL_ARB_occlusion_query, GL_ARB_timer_query,
 * GL_EXT_transform_feedback, and friends) on platforms that support
 * hardware contexts (Gen6+).
 */
#include "main/imports.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_state.h"
#include "intel_batchbuffer.h"
#include "intel_reg.h"

/*
 * Write an arbitrary 64-bit register to a buffer via MI_STORE_REGISTER_MEM.
 *
 * Only TIMESTAMP and PS_DEPTH_COUNT have special PIPE_CONTROL support; other
 * counters have to be read via the generic MI_STORE_REGISTER_MEM.
 *
 * Callers must explicitly flush the pipeline to ensure the desired value is
 * available.
 */
void
brw_store_register_mem64(struct brw_context *brw,
                         drm_intel_bo *bo, uint32_t reg, int idx)
{
   assert(brw->gen >= 6);

   /* MI_STORE_REGISTER_MEM only stores a single 32-bit value, so to
    * read a full 64-bit register, we need to do two of them.
    */
   if (brw->gen >= 8) {
      BEGIN_BATCH(8);
      OUT_BATCH(MI_STORE_REGISTER_MEM | (4 - 2));
      OUT_BATCH(reg);
      OUT_RELOC64(bo, I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                  idx * sizeof(uint64_t));
      OUT_BATCH(MI_STORE_REGISTER_MEM | (4 - 2));
      OUT_BATCH(reg + sizeof(uint32_t));
      OUT_RELOC64(bo, I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                  sizeof(uint32_t) + idx * sizeof(uint64_t));
      ADVANCE_BATCH();
   } else {
      BEGIN_BATCH(6);
      OUT_BATCH(MI_STORE_REGISTER_MEM | (3 - 2));
      OUT_BATCH(reg);
      OUT_RELOC(bo, I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                idx * sizeof(uint64_t));
      OUT_BATCH(MI_STORE_REGISTER_MEM | (3 - 2));
      OUT_BATCH(reg + sizeof(uint32_t));
      OUT_RELOC(bo, I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                sizeof(uint32_t) + idx * sizeof(uint64_t));
      ADVANCE_BATCH();
   }
}

static void
write_primitives_generated(struct brw_context *brw,
                           drm_intel_bo *query_bo, int stream, int idx)
{
   intel_batchbuffer_emit_mi_flush(brw);

   if (brw->gen >= 7 && stream > 0) {
      brw_store_register_mem64(brw, query_bo,
                               GEN7_SO_PRIM_STORAGE_NEEDED(stream), idx);
   } else {
      brw_store_register_mem64(brw, query_bo, CL_INVOCATION_COUNT, idx);
   }
}

static void
write_xfb_primitives_written(struct brw_context *brw,
                             drm_intel_bo *bo, int stream, int idx)
{
   intel_batchbuffer_emit_mi_flush(brw);

   if (brw->gen >= 7) {
      brw_store_register_mem64(brw, bo, GEN7_SO_NUM_PRIMS_WRITTEN(stream), idx);
   } else {
      brw_store_register_mem64(brw, bo, GEN6_SO_NUM_PRIMS_WRITTEN, idx);
   }
}

/**
 * Wait on the query object's BO and calculate the final result.
 */
static void
gen6_queryobj_get_results(struct gl_context *ctx,
                          struct brw_query_object *query)
{
   struct brw_context *brw = brw_context(ctx);

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
   uint64_t *results = query->bo->virtual;
   switch (query->Base.Target) {
   case GL_TIME_ELAPSED:
      /* The query BO contains the starting and ending timestamps.
       * Subtract the two and convert to nanoseconds.
       */
      query->Base.Result += 80 * (results[1] - results[0]);
      break;

   case GL_TIMESTAMP:
      /* Our timer is a clock that increments every 80ns (regardless of
       * other clock scaling in the system).  The timestamp register we can
       * read for glGetTimestamp() masks out the top 32 bits, so we do that
       * here too to let the two counters be compared against each other.
       *
       * If we just multiplied that 32 bits of data by 80, it would roll
       * over at a non-power-of-two, so an application couldn't use
       * GL_QUERY_COUNTER_BITS to handle rollover correctly.  Instead, we
       * report 36 bits and truncate at that (rolling over 5 times as often
       * as the HW counter), and when the 32-bit counter rolls over, it
       * happens to also be at a rollover in the reported value from near
       * (1<<36) to 0.
       *
       * The low 32 bits rolls over in ~343 seconds.  Our 36-bit result
       * rolls over every ~69 seconds.
       *
       * The query BO contains a single timestamp value in results[0].
       */
      query->Base.Result = 80 * (results[0] & 0xffffffff);
      query->Base.Result &= (1ull << 36) - 1;
      break;

   case GL_SAMPLES_PASSED_ARB:
      /* We need to use += rather than = here since some BLT-based operations
       * may have added additional samples to our occlusion query value.
       */
      query->Base.Result += results[1] - results[0];
      break;

   case GL_ANY_SAMPLES_PASSED:
   case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
      if (results[0] != results[1])
         query->Base.Result = true;
      break;

   case GL_PRIMITIVES_GENERATED:
   case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
      query->Base.Result = results[1] - results[0];
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
 * Driver hook for glBeginQuery().
 *
 * Initializes driver structures and emits any GPU commands required to begin
 * recording data for the query.
 */
static void
gen6_begin_query(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_query_object *query = (struct brw_query_object *)q;

   /* Since we're starting a new query, we need to throw away old results. */
   drm_intel_bo_unreference(query->bo);
   query->bo = drm_intel_bo_alloc(brw->bufmgr, "query results", 4096, 4096);

   switch (query->Base.Target) {
   case GL_TIME_ELAPSED:
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
      brw_write_timestamp(brw, query->bo, 0);
      break;

   case GL_ANY_SAMPLES_PASSED:
   case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
   case GL_SAMPLES_PASSED_ARB:
      brw_write_depth_count(brw, query->bo, 0);
      break;

   case GL_PRIMITIVES_GENERATED:
      write_primitives_generated(brw, query->bo, query->Base.Stream, 0);
      break;

   case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
      write_xfb_primitives_written(brw, query->bo, query->Base.Stream, 0);
      break;

   default:
      unreachable("Unrecognized query target in brw_begin_query()");
   }
}

/**
 * Driver hook for glEndQuery().
 *
 * Emits GPU commands to record a final query value, ending any data capturing.
 * However, the final result isn't necessarily available until the GPU processes
 * those commands.  brw_queryobj_get_results() processes the captured data to
 * produce the final result.
 */
static void
gen6_end_query(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_query_object *query = (struct brw_query_object *)q;

   switch (query->Base.Target) {
   case GL_TIME_ELAPSED:
      brw_write_timestamp(brw, query->bo, 1);
      break;

   case GL_ANY_SAMPLES_PASSED:
   case GL_ANY_SAMPLES_PASSED_CONSERVATIVE:
   case GL_SAMPLES_PASSED_ARB:
      brw_write_depth_count(brw, query->bo, 1);
      break;

   case GL_PRIMITIVES_GENERATED:
      write_primitives_generated(brw, query->bo, query->Base.Stream, 1);
      break;

   case GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN:
      write_xfb_primitives_written(brw, query->bo, query->Base.Stream, 1);
      break;

   default:
      unreachable("Unrecognized query target in brw_end_query()");
   }
}

/**
 * The WaitQuery() driver hook.
 *
 * Wait for a query result to become available and return it.  This is the
 * backing for glGetQueryObjectiv() with the GL_QUERY_RESULT pname.
 */
static void gen6_wait_query(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_query_object *query = (struct brw_query_object *)q;

   gen6_queryobj_get_results(ctx, query);
   query->Base.Ready = true;
}

/**
 * The CheckQuery() driver hook.
 *
 * Checks whether a query result is ready yet.  If not, flushes.
 * This is the backing for glGetQueryObjectiv()'s QUERY_RESULT_AVAILABLE pname.
 */
static void gen6_check_query(struct gl_context *ctx, struct gl_query_object *q)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_query_object *query = (struct brw_query_object *)q;

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
      gen6_queryobj_get_results(ctx, query);
      query->Base.Ready = true;
   }
}

/* Initialize Gen6+-specific query object functions. */
void gen6_init_queryobj_functions(struct dd_function_table *functions)
{
   functions->BeginQuery = gen6_begin_query;
   functions->EndQuery = gen6_end_query;
   functions->CheckQuery = gen6_check_query;
   functions->WaitQuery = gen6_wait_query;
}
