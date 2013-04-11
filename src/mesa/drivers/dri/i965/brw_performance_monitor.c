/*
 * Copyright © 2012 Intel Corporation
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file brw_performance_monitor.c
 *
 * Implementation of the GL_AMD_performance_monitor extension.
 *
 * Currently only for Ironlake.
 */

#include <limits.h>

#include "main/bitset.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/performance_monitor.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "intel_batchbuffer.h"

/**
 * i965 representation of a performance monitor object.
 */
struct brw_perf_monitor_object
{
   /** The base class. */
   struct gl_perf_monitor_object base;

   /**
    * BO containing raw counter data in a hardware specific form.
    */
   drm_intel_bo *bo;
};

/** Downcasting convenience macro. */
static inline struct brw_perf_monitor_object *
brw_perf_monitor(struct gl_perf_monitor_object *m)
{
   return (struct brw_perf_monitor_object *) m;
}

#define SECOND_SNAPSHOT_OFFSET_IN_BYTES 2048

/* Two random values used to ensure we're getting valid snapshots. */
#define FIRST_SNAPSHOT_REPORT_ID  0xd2e9c607
#define SECOND_SNAPSHOT_REPORT_ID 0xad584b1d

/******************************************************************************/

#define COUNTER(name)           \
   {                            \
      .Name = name,             \
      .Type = GL_UNSIGNED_INT,  \
      .Minimum = { .u32 =  0 }, \
      .Maximum = { .u32 = ~0 }, \
   }

#define GROUP(name, max_active, counter_list)  \
   {                                           \
      .Name = name,                            \
      .MaxActiveCounters = max_active,         \
      .Counters = counter_list,                \
      .NumCounters = ARRAY_SIZE(counter_list), \
   }

struct brw_perf_bo_layout {
   int group;
   int counter;
};

/**
 * Ironlake:
 *  @{
 */
const static struct gl_perf_monitor_counter gen5_raw_aggregating_counters[] = {
   COUNTER("cycles the CS unit is starved"),
   COUNTER("cycles the CS unit is stalled"),
   COUNTER("cycles the VF unit is starved"),
   COUNTER("cycles the VF unit is stalled"),
   COUNTER("cycles the VS unit is starved"),
   COUNTER("cycles the VS unit is stalled"),
   COUNTER("cycles the GS unit is starved"),
   COUNTER("cycles the GS unit is stalled"),
   COUNTER("cycles the CL unit is starved"),
   COUNTER("cycles the CL unit is stalled"),
   COUNTER("cycles the SF unit is starved"),
   COUNTER("cycles the SF unit is stalled"),
   COUNTER("cycles the WZ unit is starved"),
   COUNTER("cycles the WZ unit is stalled"),
   COUNTER("Z buffer read/write"),
   COUNTER("cycles each EU was active"),
   COUNTER("cycles each EU was suspended"),
   COUNTER("cycles threads loaded all EUs"),
   COUNTER("cycles filtering active"),
   COUNTER("cycles PS threads executed"),
   COUNTER("subspans written to RC"),
   COUNTER("bytes read for texture reads"),
   COUNTER("texels returned from sampler"),
   COUNTER("polygons not culled"),
   COUNTER("clocks MASF has valid message"),
   COUNTER("64b writes/reads from RC"),
   COUNTER("reads on dataport"),
   COUNTER("clocks MASF has valid msg not consumed by sampler"),
   COUNTER("cycles any EU is stalled for math"),
};

const static struct gl_perf_monitor_group gen5_groups[] = {
   GROUP("Aggregating Counters", INT_MAX, gen5_raw_aggregating_counters),
};

const static struct brw_perf_bo_layout gen5_perf_bo_layout[] =
{
   { -1, -1, }, /* Report ID */
   { -1, -1, }, /* TIMESTAMP (64-bit) */
   { -1, -1, }, /* ...second half... */
   {  0,  0, }, /* cycles the CS unit is starved */
   {  0,  1, }, /* cycles the CS unit is stalled */
   {  0,  2, }, /* cycles the VF unit is starved */
   {  0,  3, }, /* cycles the VF unit is stalled */
   {  0,  4, }, /* cycles the VS unit is starved */
   {  0,  5, }, /* cycles the VS unit is stalled */
   {  0,  6, }, /* cycles the GS unit is starved */
   {  0,  7, }, /* cycles the GS unit is stalled */
   {  0,  8, }, /* cycles the CL unit is starved */
   {  0,  9, }, /* cycles the CL unit is stalled */
   {  0, 10, }, /* cycles the SF unit is starved */
   {  0, 11, }, /* cycles the SF unit is stalled */
   {  0, 12, }, /* cycles the WZ unit is starved */
   {  0, 13, }, /* cycles the WZ unit is stalled */
   {  0, 14, }, /* Z buffer read/write */
   {  0, 15, }, /* cycles each EU was active */
   {  0, 16, }, /* cycles each EU was suspended */
   {  0, 17, }, /* cycles threads loaded all EUs */
   {  0, 18, }, /* cycles filtering active */
   {  0, 19, }, /* cycles PS threads executed */
   {  0, 20, }, /* subspans written to RC */
   {  0, 21, }, /* bytes read for texture reads */
   {  0, 22, }, /* texels returned from sampler */
   {  0, 23, }, /* polygons not culled */
   {  0, 24, }, /* clocks MASF has valid message */
   {  0, 25, }, /* 64b writes/reads from RC */
   {  0, 26, }, /* reads on dataport */
   {  0, 27, }, /* clocks MASF has valid msg not consumed by sampler */
   {  0, 28, }, /* cycles any EU is stalled for math */
};

/** @} */

/******************************************************************************/

static void
snapshot_aggregating_counters(struct brw_context *brw,
                              drm_intel_bo *bo, uint32_t offset_in_bytes)
{
   uint32_t report_id = offset_in_bytes == 0 ? FIRST_SNAPSHOT_REPORT_ID
                                             : SECOND_SNAPSHOT_REPORT_ID;

   if (brw->gen == 5) {
      /* Ironlake requires two MI_REPORT_PERF_COUNT commands to write all
       * the counters.  The report ID is ignored in the second set.
       */
      BEGIN_BATCH(6);
      OUT_BATCH(GEN5_MI_REPORT_PERF_COUNT | GEN5_MI_COUNTER_SET_0);
      OUT_RELOC(bo,
                I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                offset_in_bytes);
      OUT_BATCH(report_id);

      OUT_BATCH(GEN5_MI_REPORT_PERF_COUNT | GEN5_MI_COUNTER_SET_1);
      OUT_RELOC(bo,
                I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                offset_in_bytes + 64);
      OUT_BATCH(report_id);
      ADVANCE_BATCH();
   } else {
      assert(!"Unsupported generation for performance counters.");
   }
}

static bool
aggregating_counters_needed(struct brw_context *brw,
                            struct gl_perf_monitor_object *m)
{
   return m->ActiveGroups[0];
}

/******************************************************************************/

/**
 * Create a new performance monitor object.
 */
static struct gl_perf_monitor_object *
brw_new_perf_monitor(struct gl_context *ctx)
{
   return calloc(1, sizeof(struct brw_perf_monitor_object));
}

/**
 * Delete a performance monitor object.
 */
static void
brw_delete_perf_monitor(struct gl_context *ctx, struct gl_perf_monitor_object *m)
{
   struct brw_perf_monitor_object *monitor = brw_perf_monitor(m);

   if (monitor->bo)
      drm_intel_bo_unreference(monitor->bo);

   free(monitor);
}

/**
 * Driver hook for glBeginPerformanceMonitorAMD().
 */
static GLboolean
brw_begin_perf_monitor(struct gl_context *ctx,
                       struct gl_perf_monitor_object *m)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_perf_monitor_object *monitor = brw_perf_monitor(m);

   /* If the BO already exists, throw it away.  It contains old results
    * that we're not interested in any more.
    */
   if (monitor->bo)
      drm_intel_bo_unreference(monitor->bo);

   /* Create a new BO. */
   monitor->bo =
      drm_intel_bo_alloc(brw->bufmgr, "performance monitor", 4096, 64);
   drm_intel_bo_map(monitor->bo, true);
   memset((char *) monitor->bo->virtual, 0xff, 4096);
   drm_intel_bo_unmap(monitor->bo);

   /* Take a shapshot of all active counters */
   if (aggregating_counters_needed(brw, m)) {
      snapshot_aggregating_counters(brw, monitor->bo, 0);
   }

   return true;
}

/**
 * Driver hook for glEndPerformanceMonitorAMD().
 */
static void
brw_end_perf_monitor(struct gl_context *ctx,
                     struct gl_perf_monitor_object *m)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_perf_monitor_object *monitor = brw_perf_monitor(m);
   if (aggregating_counters_needed(brw, m)) {
      snapshot_aggregating_counters(brw, monitor->bo,
                                    SECOND_SNAPSHOT_OFFSET_IN_BYTES);
   }
}

/**
 * Reset a performance monitor, throwing away any results.
 */
static void
brw_reset_perf_monitor(struct gl_context *ctx,
                       struct gl_perf_monitor_object *m)
{
   struct brw_perf_monitor_object *monitor = brw_perf_monitor(m);

   if (monitor->bo) {
      drm_intel_bo_unreference(monitor->bo);
      monitor->bo = NULL;
   }

   if (m->Active) {
      brw_begin_perf_monitor(ctx, m);
   }
}

/**
 * Is a performance monitor result available?
 */
static GLboolean
brw_is_perf_monitor_result_available(struct gl_context *ctx,
                                     struct gl_perf_monitor_object *m)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_perf_monitor_object *monitor = brw_perf_monitor(m);
   return !m->Active && monitor->bo &&
          !drm_intel_bo_references(brw->batch.bo, monitor->bo) &&
          !drm_intel_bo_busy(monitor->bo);
}

/**
 * Get the performance monitor result.
 */
static void
brw_get_perf_monitor_result(struct gl_context *ctx,
                            struct gl_perf_monitor_object *m,
                            GLsizei data_size,
                            GLuint *data,
                            GLint *bytes_written)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_perf_monitor_object *monitor = brw_perf_monitor(m);

   /* This hook should only be called when results are available. */
   assert(monitor->bo != NULL);

   drm_intel_bo_map(monitor->bo, false);
   unsigned *gpu_bo = monitor->bo->virtual;

   /* Copy data from the BO to the supplied array.
    *
    * The output data format is: <group ID, counter ID, value> for each
    * active counter.  The API allows counters to appear in any order.
    */
   GLsizei offset = 0;

   /* Look for expected report ID values to ensure data is present. */
   assert(gpu_bo[0] == FIRST_SNAPSHOT_REPORT_ID);
   assert(gpu_bo[SECOND_SNAPSHOT_OFFSET_IN_BYTES/4] == SECOND_SNAPSHOT_REPORT_ID);

   for (int i = 0; i < brw->perfmon.entries_in_bo; i++) {
      int group = brw->perfmon.bo_layout[i].group;
      int counter = brw->perfmon.bo_layout[i].counter;

      if (group < 0 || !BITSET_TEST(m->ActiveCounters[group], counter))
         continue;

      const struct gl_perf_monitor_group *group_obj =
         &ctx->PerfMonitor.Groups[group];

      const struct gl_perf_monitor_counter *c = &group_obj->Counters[counter];

      data[offset++] = group;
      data[offset++] = counter;

      uint32_t second_snapshot_index =
         SECOND_SNAPSHOT_OFFSET_IN_BYTES / sizeof(uint32_t) + i;

      /* Won't work for uint64_t values, but we don't expose any yet. */
      data[offset] = gpu_bo[second_snapshot_index] - gpu_bo[i];
      offset += _mesa_perf_monitor_counter_size(c) / sizeof(uint32_t);
   }

   drm_intel_bo_unmap(monitor->bo);

   if (bytes_written)
      *bytes_written = offset * sizeof(uint32_t);
}

void
brw_init_performance_monitors(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;

   ctx->Driver.NewPerfMonitor = brw_new_perf_monitor;
   ctx->Driver.DeletePerfMonitor = brw_delete_perf_monitor;
   ctx->Driver.BeginPerfMonitor = brw_begin_perf_monitor;
   ctx->Driver.EndPerfMonitor = brw_end_perf_monitor;
   ctx->Driver.ResetPerfMonitor = brw_reset_perf_monitor;
   ctx->Driver.IsPerfMonitorResultAvailable = brw_is_perf_monitor_result_available;
   ctx->Driver.GetPerfMonitorResult = brw_get_perf_monitor_result;

   if (brw->gen == 5) {
      ctx->PerfMonitor.Groups = gen5_groups;
      ctx->PerfMonitor.NumGroups = ARRAY_SIZE(gen5_groups);
      brw->perfmon.bo_layout = gen5_perf_bo_layout;
      brw->perfmon.entries_in_bo = ARRAY_SIZE(gen5_perf_bo_layout);
   }
}
