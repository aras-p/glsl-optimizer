/*
 * Copyright © 2013 Intel Corporation
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
 */

#include <limits.h>

#include "main/bitset.h"
#include "main/hash.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/performance_monitor.h"

#include "glsl/ralloc.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "intel_batchbuffer.h"

#define FILE_DEBUG_FLAG DEBUG_PERFMON

/**
 * i965 representation of a performance monitor object.
 */
struct brw_perf_monitor_object
{
   /** The base class. */
   struct gl_perf_monitor_object base;
};

/** Downcasting convenience macro. */
static inline struct brw_perf_monitor_object *
brw_perf_monitor(struct gl_perf_monitor_object *m)
{
   return (struct brw_perf_monitor_object *) m;
}

/******************************************************************************/

#define COUNTER(name)           \
   {                            \
      .Name = name,             \
      .Type = GL_UNSIGNED_INT,  \
      .Minimum = { .u32 =  0 }, \
      .Maximum = { .u32 = ~0 }, \
   }

#define COUNTER64(name)              \
   {                                 \
      .Name = name,                  \
      .Type = GL_UNSIGNED_INT64_AMD, \
      .Minimum = { .u64 =  0 },      \
      .Maximum = { .u64 = ~0 },      \
   }

#define GROUP(name, max_active, counter_list)  \
   {                                           \
      .Name = name,                            \
      .MaxActiveCounters = max_active,         \
      .Counters = counter_list,                \
      .NumCounters = ARRAY_SIZE(counter_list), \
   }

/**
 * Ironlake:
 *  @{
 */
const static struct gl_perf_monitor_group gen5_groups[] = {
   /* Our pipeline statistics counter handling requires hardware contexts. */
};
/** @} */

/**
 * Sandybridge:
 *  @{
 */
const static struct gl_perf_monitor_counter gen6_statistics_counters[] = {
   COUNTER64("IA_VERTICES_COUNT"),
   COUNTER64("IA_PRIMITIVES_COUNT"),
   COUNTER64("VS_INVOCATION_COUNT"),
   COUNTER64("GS_INVOCATION_COUNT"),
   COUNTER64("GS_PRIMITIVES_COUNT"),
   COUNTER64("CL_INVOCATION_COUNT"),
   COUNTER64("CL_PRIMITIVES_COUNT"),
   COUNTER64("PS_INVOCATION_COUNT"),
   COUNTER64("PS_DEPTH_COUNT"),
   COUNTER64("SO_NUM_PRIMS_WRITTEN"),
   COUNTER64("SO_PRIM_STORAGE_NEEDED"),
};

/** MMIO register addresses for each pipeline statistics counter. */
const static int gen6_statistics_register_addresses[] = {
   IA_VERTICES_COUNT,
   IA_PRIMITIVES_COUNT,
   VS_INVOCATION_COUNT,
   GS_INVOCATION_COUNT,
   GS_PRIMITIVES_COUNT,
   CL_INVOCATION_COUNT,
   CL_PRIMITIVES_COUNT,
   PS_INVOCATION_COUNT,
   PS_DEPTH_COUNT,
   GEN6_SO_NUM_PRIMS_WRITTEN,
   GEN6_SO_PRIM_STORAGE_NEEDED,
};

const static struct gl_perf_monitor_group gen6_groups[] = {
   GROUP("Pipeline Statistics Registers", INT_MAX, gen6_statistics_counters),
};
/** @} */

/**
 * Ivybridge/Baytrail/Haswell:
 *  @{
 */
const static struct gl_perf_monitor_counter gen7_statistics_counters[] = {
   COUNTER64("IA_VERTICES_COUNT"),
   COUNTER64("IA_PRIMITIVES_COUNT"),
   COUNTER64("VS_INVOCATION_COUNT"),
   COUNTER64("HS_INVOCATION_COUNT"),
   COUNTER64("DS_INVOCATION_COUNT"),
   COUNTER64("GS_INVOCATION_COUNT"),
   COUNTER64("GS_PRIMITIVES_COUNT"),
   COUNTER64("CL_INVOCATION_COUNT"),
   COUNTER64("CL_PRIMITIVES_COUNT"),
   COUNTER64("PS_INVOCATION_COUNT"),
   COUNTER64("PS_DEPTH_COUNT"),
   COUNTER64("SO_NUM_PRIMS_WRITTEN (Stream 0)"),
   COUNTER64("SO_NUM_PRIMS_WRITTEN (Stream 1)"),
   COUNTER64("SO_NUM_PRIMS_WRITTEN (Stream 2)"),
   COUNTER64("SO_NUM_PRIMS_WRITTEN (Stream 3)"),
   COUNTER64("SO_PRIM_STORAGE_NEEDED (Stream 0)"),
   COUNTER64("SO_PRIM_STORAGE_NEEDED (Stream 1)"),
   COUNTER64("SO_PRIM_STORAGE_NEEDED (Stream 2)"),
   COUNTER64("SO_PRIM_STORAGE_NEEDED (Stream 3)"),
};

/** MMIO register addresses for each pipeline statistics counter. */
const static int gen7_statistics_register_addresses[] = {
   IA_VERTICES_COUNT,
   IA_PRIMITIVES_COUNT,
   VS_INVOCATION_COUNT,
   HS_INVOCATION_COUNT,
   DS_INVOCATION_COUNT,
   GS_INVOCATION_COUNT,
   GS_PRIMITIVES_COUNT,
   CL_INVOCATION_COUNT,
   CL_PRIMITIVES_COUNT,
   PS_INVOCATION_COUNT,
   PS_DEPTH_COUNT,
   GEN7_SO_NUM_PRIMS_WRITTEN(0),
   GEN7_SO_NUM_PRIMS_WRITTEN(1),
   GEN7_SO_NUM_PRIMS_WRITTEN(2),
   GEN7_SO_NUM_PRIMS_WRITTEN(3),
   GEN7_SO_PRIM_STORAGE_NEEDED(0),
   GEN7_SO_PRIM_STORAGE_NEEDED(1),
   GEN7_SO_PRIM_STORAGE_NEEDED(2),
   GEN7_SO_PRIM_STORAGE_NEEDED(3),
};

const static struct gl_perf_monitor_group gen7_groups[] = {
   GROUP("Pipeline Statistics Registers", INT_MAX, gen7_statistics_counters),
};
/** @} */

/******************************************************************************/

static GLboolean brw_is_perf_monitor_result_available(struct gl_context *, struct gl_perf_monitor_object *);

static void
dump_perf_monitor_callback(GLuint name, void *monitor_void, void *brw_void)
{
   struct gl_context *ctx = brw_void;
   struct gl_perf_monitor_object *m = monitor_void;

   DBG("%4d  %-7s %-6s %-11s\n",
       name,
       m->Active ? "Active" : "",
       m->Ended ? "Ended" : "",
       brw_is_perf_monitor_result_available(ctx, m) ? "Available" : "");
}

void
brw_dump_perf_monitors(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   DBG("Monitors:\n");
   _mesa_HashWalk(ctx->PerfMonitor.Monitors, dump_perf_monitor_callback, brw);
}

/******************************************************************************/

/**
 * Initialize a monitor to sane starting state; throw away old buffers.
 */
static void
reinitialize_perf_monitor(struct brw_context *brw,
                          struct brw_perf_monitor_object *monitor)
{
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

   DBG("Begin(%d)\n", m->Name);

   reinitialize_perf_monitor(brw, monitor);

   return true;
}

/**
 * Driver hook for glEndPerformanceMonitorAMD().
 */
static void
brw_end_perf_monitor(struct gl_context *ctx,
                     struct gl_perf_monitor_object *m)
{
   DBG("End(%d)\n", m->Name);
}

/**
 * Reset a performance monitor, throwing away any results.
 */
static void
brw_reset_perf_monitor(struct gl_context *ctx,
                       struct gl_perf_monitor_object *m)
{
   struct brw_context *brw = brw_context(ctx);
   struct brw_perf_monitor_object *monitor = brw_perf_monitor(m);

   reinitialize_perf_monitor(brw, monitor);

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
   /* ...need to actually check if counters are available, once we have some. */

   return true;
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
   DBG("GetResult(%d)\n", m->Name);

   /* This hook should only be called when results are available. */
   assert(m->Ended);

   /* Copy data to the supplied array (data).
    *
    * The output data format is: <group ID, counter ID, value> for each
    * active counter.  The API allows counters to appear in any order.
    */
   GLsizei offset = 0;

   /* ...but, we don't actually expose anything yet, so nothing to do here */

   if (bytes_written)
      *bytes_written = offset * sizeof(uint32_t);
}

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
   DBG("Delete(%d)\n", m->Name);
   reinitialize_perf_monitor(brw_context(ctx), monitor);
   free(monitor);
}

/******************************************************************************/

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
   } else if (brw->gen == 6) {
      ctx->PerfMonitor.Groups = gen6_groups;
      ctx->PerfMonitor.NumGroups = ARRAY_SIZE(gen6_groups);
      brw->perfmon.statistics_registers = gen6_statistics_register_addresses;
   } else if (brw->gen == 7) {
      ctx->PerfMonitor.Groups = gen7_groups;
      ctx->PerfMonitor.NumGroups = ARRAY_SIZE(gen7_groups);
      brw->perfmon.statistics_registers = gen7_statistics_register_addresses;
   }
}
