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
 *
 * On Gen5+ hardware, we have two sources of performance counter data:
 * the Observability Architecture counters (MI_REPORT_PERF_COUNT), and
 * the Pipeline Statistics Registers.  We expose both sets of raw data,
 * as well as some useful processed values.
 *
 * The Observability Architecture (OA) counters for Gen6+ are documented
 * in a separate document from the rest of the PRMs.  It is available at:
 * https://01.org/linuxgraphics/documentation/driver-documentation-prms
 * => 2013 Intel Core Processor Family => Observability Performance Counters
 * (This one volume covers Sandybridge, Ivybridge, Baytrail, and Haswell.)
 *
 * On Ironlake, the OA counters were called "CHAPS" counters.  Sadly, no public
 * documentation exists; our implementation is based on the source code for the
 * intel_perf_counters utility (which is available as part of intel-gpu-tools).
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

   /**
    * BO containing starting and ending snapshots for any active pipeline
    * statistics counters.
    */
   drm_intel_bo *pipeline_stats_bo;

   /**
    * Storage for final pipeline statistics counter results.
    */
   uint64_t *pipeline_stats_results;
};

/** Downcasting convenience macro. */
static inline struct brw_perf_monitor_object *
brw_perf_monitor(struct gl_perf_monitor_object *m)
{
   return (struct brw_perf_monitor_object *) m;
}

#define SECOND_SNAPSHOT_OFFSET_IN_BYTES 2048

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

/** Performance Monitor Group IDs */
enum brw_counter_groups {
   OA_COUNTERS, /* Observability Architecture (MI_REPORT_PERF_COUNT) Counters */
   PIPELINE_STATS_COUNTERS, /* Pipeline Statistics Register Counters */
};

/**
 * Ironlake:
 *  @{
 *
 * The list of CHAPS counters unfortunately does not appear in any public
 * documentation, but is available by reading the source code for the
 * intel_perf_counters utility (shipped as part of intel-gpu-tools).
 */
const static struct gl_perf_monitor_counter gen5_raw_chaps_counters[] = {
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

const static int gen5_oa_snapshot_layout[] =
{
   -1, /* Report ID */
   -1, /* TIMESTAMP (64-bit) */
   -1, /* ...second half... */
    0, /* cycles the CS unit is starved */
    1, /* cycles the CS unit is stalled */
    2, /* cycles the VF unit is starved */
    3, /* cycles the VF unit is stalled */
    4, /* cycles the VS unit is starved */
    5, /* cycles the VS unit is stalled */
    6, /* cycles the GS unit is starved */
    7, /* cycles the GS unit is stalled */
    8, /* cycles the CL unit is starved */
    9, /* cycles the CL unit is stalled */
   10, /* cycles the SF unit is starved */
   11, /* cycles the SF unit is stalled */
   12, /* cycles the WZ unit is starved */
   13, /* cycles the WZ unit is stalled */
   14, /* Z buffer read/write */
   15, /* cycles each EU was active */
   16, /* cycles each EU was suspended */
   17, /* cycles threads loaded all EUs */
   18, /* cycles filtering active */
   19, /* cycles PS threads executed */
   20, /* subspans written to RC */
   21, /* bytes read for texture reads */
   22, /* texels returned from sampler */
   23, /* polygons not culled */
   24, /* clocks MASF has valid message */
   25, /* 64b writes/reads from RC */
   26, /* reads on dataport */
   27, /* clocks MASF has valid msg not consumed by sampler */
   28, /* cycles any EU is stalled for math */
};

const static struct gl_perf_monitor_group gen5_groups[] = {
   [OA_COUNTERS] = GROUP("CHAPS Counters", INT_MAX, gen5_raw_chaps_counters),
   /* Our pipeline statistics counter handling requires hardware contexts. */
};
/** @} */

/**
 * Sandybridge:
 *  @{
 *
 * A few of the counters here (A17-A20) are not included in the latest
 * documentation, but are described in the Ironlake PRM (which strangely
 * documents Sandybridge's performance counter system, not Ironlake's).
 * It's unclear whether they work or not; empirically, they appear to.
 */

/**
 * Aggregating counters A0-A28:
 */
const static struct gl_perf_monitor_counter gen6_raw_oa_counters[] = {
   /* A0:   0 */ COUNTER("Aggregated Core Array Active"),
   /* A1:   1 */ COUNTER("Aggregated Core Array Stalled"),
   /* A2:   2 */ COUNTER("Vertex Shader Active Time"),
   /* A3: Not actually hooked up on Sandybridge. */
   /* A4:   3 */ COUNTER("Vertex Shader Stall Time - Core Stall"),
   /* A5:   4 */ COUNTER("# VS threads loaded"),
   /* A6:   5 */ COUNTER("Vertex Shader Ready but not running Time"),
   /* A7:   6 */ COUNTER("Geometry Shader Active Time"),
   /* A8: Not actually hooked up on Sandybridge. */
   /* A9:   7 */ COUNTER("Geometry Shader Stall Time - Core Stall"),
   /* A10:  8 */ COUNTER("# GS threads loaded"),
   /* A11:  9 */ COUNTER("Geometry Shader Ready but not running Time"),
   /* A12: 10 */ COUNTER("Pixel Shader Active Time"),
   /* A13: Not actually hooked up on Sandybridge. */
   /* A14: 11 */ COUNTER("Pixel Shader Stall Time - Core Stall"),
   /* A15: 12 */ COUNTER("# PS threads loaded"),
   /* A16: 13 */ COUNTER("Pixel Shader Ready but not running Time"),
   /* A17: 14 */ COUNTER("Early Z Test Pixels Passing"),
   /* A18: 15 */ COUNTER("Early Z Test Pixels Failing"),
   /* A19: 16 */ COUNTER("Early Stencil Test Pixels Passing"),
   /* A20: 17 */ COUNTER("Early Stencil Test Pixels Failing"),
   /* A21: 18 */ COUNTER("Pixel Kill Count"),
   /* A22: 19 */ COUNTER("Alpha Test Pixels Failed"),
   /* A23: 20 */ COUNTER("Post PS Stencil Pixels Failed"),
   /* A24: 21 */ COUNTER("Post PS Z buffer Pixels Failed"),
   /* A25: 22 */ COUNTER("Pixels/samples Written in the frame buffer"),
   /* A26: 23 */ COUNTER("GPU Busy"),
   /* A27: 24 */ COUNTER("CL active and not stalled"),
   /* A28: 25 */ COUNTER("SF active and stalled"),
};

/**
 * Sandybridge: Counter Select = 001
 * A0   A1   A2   A3   A4   TIMESTAMP RPT_ID
 * A5   A6   A7   A8   A9   A10  A11  A12
 * A13  A14  A15  A16  A17  A18  A19  A20
 * A21  A22  A23  A24  A25  A26  A27  A28
 *
 * (Yes, this is a strange order.)  We also have to remap for missing counters.
 */
const static int gen6_oa_snapshot_layout[] =
{
   -1, /* Report ID */
   -1, /* TIMESTAMP (64-bit) */
   -1, /* ...second half... */
    3, /* A4:  Vertex Shader Stall Time - Core Stall */
   -1, /* A3:  (not available) */
    2, /* A2:  Vertex Shader Active Time */
    1, /* A1:  Aggregated Core Array Stalled */
    0, /* A0:  Aggregated Core Array Active */
   10, /* A12: Pixel Shader Active Time */
    9, /* A11: Geometry Shader ready but not running Time */
    8, /* A10: # GS threads loaded */
    7, /* A9:  Geometry Shader Stall Time - Core Stall */
   -1, /* A8:  (not available) */
    6, /* A7:  Geometry Shader Active Time */
    5, /* A6:  Vertex Shader ready but not running Time */
    4, /* A5:  # VS Threads Loaded */
   17, /* A20: Early Stencil Test Pixels Failing */
   16, /* A19: Early Stencil Test Pixels Passing */
   15, /* A18: Early Z Test Pixels Failing */
   14, /* A17: Early Z Test Pixels Passing */
   13, /* A16: Pixel Shader ready but not running Time */
   12, /* A15: # PS threads loaded */
   11, /* A14: Pixel Shader Stall Time - Core Stall */
   -1, /* A13: (not available) */
   25, /* A28: SF active and stalled */
   24, /* A27: CL active and not stalled */
   23, /* A26: GPU Busy */
   22, /* A25: Pixels/samples Written in the frame buffer */
   21, /* A24: Post PS Z buffer Pixels Failed */
   20, /* A23: Post PS Stencil Pixels Failed */
   19, /* A22: Alpha Test Pixels Failed */
   18, /* A21: Pixel Kill Count */
};

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
   GROUP("Observability Architecture Counters", INT_MAX, gen6_raw_oa_counters),
   GROUP("Pipeline Statistics Registers", INT_MAX, gen6_statistics_counters),
};
/** @} */

/**
 * Ivybridge/Baytrail/Haswell:
 *  @{
 */
const static struct gl_perf_monitor_counter gen7_raw_oa_counters[] = {
   COUNTER("Aggregated Core Array Active"),
   COUNTER("Aggregated Core Array Stalled"),
   COUNTER("Vertex Shader Active Time"),
   COUNTER("Vertex Shader Stall Time - Core Stall"),
   COUNTER("# VS threads loaded"),
   COUNTER("Hull Shader Active Time"),
   COUNTER("Hull Shader Stall Time - Core Stall"),
   COUNTER("# HS threads loaded"),
   COUNTER("Domain Shader Active Time"),
   COUNTER("Domain Shader Stall Time - Core Stall"),
   COUNTER("# DS threads loaded"),
   COUNTER("Compute Shader Active Time"),
   COUNTER("Compute Shader Stall Time - Core Stall"),
   COUNTER("# CS threads loaded"),
   COUNTER("Geometry Shader Active Time"),
   COUNTER("Geometry Shader Stall Time - Core Stall"),
   COUNTER("# GS threads loaded"),
   COUNTER("Pixel Shader Active Time"),
   COUNTER("Pixel Shader Stall Time - Core Stall"),
   COUNTER("# PS threads loaded"),
   COUNTER("HiZ Fast Z Test Pixels Passing"),
   COUNTER("HiZ Fast Z Test Pixels Failing"),
   COUNTER("Slow Z Test Pixels Passing"),
   COUNTER("Slow Z Test Pixels Failing"),
   COUNTER("Pixel Kill Count"),
   COUNTER("Alpha Test Pixels Failed"),
   COUNTER("Post PS Stencil Pixels Failed"),
   COUNTER("Post PS Z buffer Pixels Failed"),
   COUNTER("3D/GPGPU Render Target Writes"),
   COUNTER("Render Engine Busy"),
   COUNTER("VS bottleneck"),
   COUNTER("GS bottleneck"),
};

/**
 * Ivybridge/Baytrail/Haswell: Counter Select = 101
 * A4   A3   A2   A1   A0   TIMESTAMP  ReportID
 * A12  A11  A10  A9   A8   A7   A6    A5
 * A20  A19  A18  A17  A16  A15  A14   A13
 * A28  A27  A26  A25  A24  A23  A22   A21
 * A36  A35  A34  A33  A32  A31  A30   A29
 * A44  A43  A42  A41  A40  A39  A38   A37
 * B7   B6   B5   B4   B3   B2   B1    B0
 * Rsv  Rsv  Rsv  Rsv  Rsv  Rsv  Rsv   Rsv
 */
const static int gen7_oa_snapshot_layout[] =
{
   -1, /* Report ID */
   -1, /* TIMESTAMP (64-bit) */
   -1, /* ...second half... */
    0, /* A0:  Aggregated Core Array Active */
    1, /* A1:  Aggregated Core Array Stalled */
    2, /* A2:  Vertex Shader Active Time */
   -1, /* A3:  Reserved */
    3, /* A4:  Vertex Shader Stall Time - Core Stall */
    4, /* A5:  # VS threads loaded */
   -1, /* A6:  Reserved */
    5, /* A7:  Hull Shader Active Time */
   -1, /* A8:  Reserved */
    6, /* A9:  Hull Shader Stall Time - Core Stall */
    7, /* A10: # HS threads loaded */
   -1, /* A11: Reserved */
    8, /* A12: Domain Shader Active Time */
   -1, /* A13: Reserved */
    9, /* A14: Domain Shader Stall Time - Core Stall */
   10, /* A15: # DS threads loaded */
   -1, /* A16: Reserved */
   11, /* A17: Compute Shader Active Time */
   -1, /* A18: Reserved */
   12, /* A19: Compute Shader Stall Time - Core Stall */
   13, /* A20: # CS threads loaded */
   -1, /* A21: Reserved */
   14, /* A22: Geometry Shader Active Time */
   -1, /* A23: Reserved */
   15, /* A24: Geometry Shader Stall Time - Core Stall */
   16, /* A25: # GS threads loaded */
   -1, /* A26: Reserved */
   17, /* A27: Pixel Shader Active Time */
   -1, /* A28: Reserved */
   18, /* A29: Pixel Shader Stall Time - Core Stall */
   19, /* A30: # PS threads loaded */
   -1, /* A31: Reserved */
   20, /* A32: HiZ Fast Z Test Pixels Passing */
   21, /* A33: HiZ Fast Z Test Pixels Failing */
   22, /* A34: Slow Z Test Pixels Passing */
   23, /* A35: Slow Z Test Pixels Failing */
   24, /* A36: Pixel Kill Count */
   25, /* A37: Alpha Test Pixels Failed */
   26, /* A38: Post PS Stencil Pixels Failed */
   27, /* A39: Post PS Z buffer Pixels Failed */
   28, /* A40: 3D/GPGPU Render Target Writes */
   29, /* A41: Render Engine Busy */
   30, /* A42: VS bottleneck */
   31, /* A43: GS bottleneck */
   -1, /* A44: Reserved */
   -1, /* B0 */
   -1, /* B1 */
   -1, /* B2 */
   -1, /* B3 */
   -1, /* B4 */
   -1, /* B5 */
   -1, /* B6 */
   -1, /* B7 */
   -1, /* Reserved */
   -1, /* Reserved */
   -1, /* Reserved */
   -1, /* Reserved */
   -1, /* Reserved */
   -1, /* Reserved */
   -1, /* Reserved */
   -1, /* Reserved */
};

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
   GROUP("Observability Architecture Counters", INT_MAX, gen7_raw_oa_counters),
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
   struct brw_perf_monitor_object *monitor = monitor_void;

   DBG("%4d  %-7s %-6s %-11s  %-9s\n",
       name,
       m->Active ? "Active" : "",
       m->Ended ? "Ended" : "",
       brw_is_perf_monitor_result_available(ctx, m) ? "Available" : "",
       monitor->pipeline_stats_bo ? "Stats BO" : "");
}

void
brw_dump_perf_monitors(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   DBG("Monitors:\n");
   _mesa_HashWalk(ctx->PerfMonitor.Monitors, dump_perf_monitor_callback, brw);
}

/******************************************************************************/

static bool
monitor_needs_statistics_registers(struct brw_context *brw,
                                   struct gl_perf_monitor_object *m)
{
   return brw->gen >= 6 && m->ActiveGroups[PIPELINE_STATS_COUNTERS];
}

/**
 * Take a snapshot of any monitored pipeline statistics counters.
 */
static void
snapshot_statistics_registers(struct brw_context *brw,
                              struct brw_perf_monitor_object *monitor,
                              uint32_t offset_in_bytes)
{
   struct gl_context *ctx = &brw->ctx;
   const int offset = offset_in_bytes / sizeof(uint64_t);
   const int group = PIPELINE_STATS_COUNTERS;
   const int num_counters = ctx->PerfMonitor.Groups[group].NumCounters;

   intel_batchbuffer_emit_mi_flush(brw);

   for (int i = 0; i < num_counters; i++) {
      if (BITSET_TEST(monitor->base.ActiveCounters[group], i)) {
         assert(ctx->PerfMonitor.Groups[group].Counters[i].Type ==
                GL_UNSIGNED_INT64_AMD);

         brw_store_register_mem64(brw, monitor->pipeline_stats_bo,
                                  brw->perfmon.statistics_registers[i],
                                  offset + i);
      }
   }
}

/**
 * Gather results from pipeline_stats_bo, storing the final values.
 *
 * This allows us to free pipeline_stats_bo (which is 4K) in favor of a much
 * smaller array of final results.
 */
static void
gather_statistics_results(struct brw_context *brw,
                          struct brw_perf_monitor_object *monitor)
{
   struct gl_context *ctx = &brw->ctx;
   const int num_counters =
      ctx->PerfMonitor.Groups[PIPELINE_STATS_COUNTERS].NumCounters;

   monitor->pipeline_stats_results = calloc(num_counters, sizeof(uint64_t));

   drm_intel_bo_map(monitor->pipeline_stats_bo, false);
   uint64_t *start = monitor->pipeline_stats_bo->virtual;
   uint64_t *end = start + (SECOND_SNAPSHOT_OFFSET_IN_BYTES / sizeof(uint64_t));

   for (int i = 0; i < num_counters; i++) {
      monitor->pipeline_stats_results[i] = end[i] - start[i];
   }
   drm_intel_bo_unmap(monitor->pipeline_stats_bo);
   drm_intel_bo_unreference(monitor->pipeline_stats_bo);
   monitor->pipeline_stats_bo = NULL;
}

/******************************************************************************/

/**
 * Initialize a monitor to sane starting state; throw away old buffers.
 */
static void
reinitialize_perf_monitor(struct brw_context *brw,
                          struct brw_perf_monitor_object *monitor)
{
   if (monitor->pipeline_stats_bo) {
      drm_intel_bo_unreference(monitor->pipeline_stats_bo);
      monitor->pipeline_stats_bo = NULL;
   }

   free(monitor->pipeline_stats_results);
   monitor->pipeline_stats_results = NULL;
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

   if (monitor_needs_statistics_registers(brw, m)) {
      monitor->pipeline_stats_bo =
         drm_intel_bo_alloc(brw->bufmgr, "perf. monitor stats bo", 4096, 64);

      /* Take starting snapshots. */
      snapshot_statistics_registers(brw, monitor, 0);
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

   DBG("End(%d)\n", m->Name);

   if (monitor_needs_statistics_registers(brw, m)) {
      /* Take ending snapshots. */
      snapshot_statistics_registers(brw, monitor,
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
   struct brw_context *brw = brw_context(ctx);
   struct brw_perf_monitor_object *monitor = brw_perf_monitor(m);

   bool stats_available = true;

   if (monitor_needs_statistics_registers(brw, m)) {
      stats_available = !monitor->pipeline_stats_bo ||
         (!drm_intel_bo_references(brw->batch.bo, monitor->pipeline_stats_bo) &&
          !drm_intel_bo_busy(monitor->pipeline_stats_bo));
   }

   return stats_available;
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

   DBG("GetResult(%d)\n", m->Name);
   brw_dump_perf_monitors(brw);

   /* This hook should only be called when results are available. */
   assert(m->Ended);

   /* Copy data to the supplied array (data).
    *
    * The output data format is: <group ID, counter ID, value> for each
    * active counter.  The API allows counters to appear in any order.
    */
   GLsizei offset = 0;

   if (monitor_needs_statistics_registers(brw, m)) {
      const int num_counters =
         ctx->PerfMonitor.Groups[PIPELINE_STATS_COUNTERS].NumCounters;

      if (!monitor->pipeline_stats_results)
         gather_statistics_results(brw, monitor);

      for (int i = 0; i < num_counters; i++) {
         if (BITSET_TEST(m->ActiveCounters[PIPELINE_STATS_COUNTERS], i)) {
            data[offset++] = PIPELINE_STATS_COUNTERS;
            data[offset++] = i;
            *((uint64_t *) (&data[offset])) = monitor->pipeline_stats_results[i];
            offset += 2;
         }
      }
   }

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
      brw->perfmon.oa_snapshot_layout = gen5_oa_snapshot_layout;
      brw->perfmon.entries_per_oa_snapshot = ARRAY_SIZE(gen5_oa_snapshot_layout);
   } else if (brw->gen == 6) {
      ctx->PerfMonitor.Groups = gen6_groups;
      ctx->PerfMonitor.NumGroups = ARRAY_SIZE(gen6_groups);
      brw->perfmon.oa_snapshot_layout = gen6_oa_snapshot_layout;
      brw->perfmon.entries_per_oa_snapshot = ARRAY_SIZE(gen6_oa_snapshot_layout);
      brw->perfmon.statistics_registers = gen6_statistics_register_addresses;
   } else if (brw->gen == 7) {
      ctx->PerfMonitor.Groups = gen7_groups;
      ctx->PerfMonitor.NumGroups = ARRAY_SIZE(gen7_groups);
      brw->perfmon.oa_snapshot_layout = gen7_oa_snapshot_layout;
      brw->perfmon.entries_per_oa_snapshot = ARRAY_SIZE(gen7_oa_snapshot_layout);
      brw->perfmon.statistics_registers = gen7_statistics_register_addresses;
   }
}
