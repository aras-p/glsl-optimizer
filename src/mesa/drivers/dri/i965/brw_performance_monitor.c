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
    * BO containing OA counter snapshots at monitor Begin/End time.
    */
   drm_intel_bo *oa_bo;

   /** Indexes into bookend_bo (snapshot numbers) for various segments. */
   int oa_head_end;
   int oa_middle_start;
   int oa_tail_start;

   /**
    * Storage for OA results accumulated so far.
    *
    * An array indexed by the counter ID in the OA_COUNTERS group.
    *
    * When we run out of space in bookend_bo, we compute the results so far
    * and add them to the value stored here.  Then, we can discard bookend_bo.
    */
   uint32_t *oa_results;

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

/* A random value used to ensure we're getting valid snapshots. */
#define REPORT_ID 0xd2e9c607

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
   struct brw_context *brw = brw_void;
   struct gl_context *ctx = brw_void;
   struct gl_perf_monitor_object *m = monitor_void;
   struct brw_perf_monitor_object *monitor = monitor_void;

   const char *resolved = "";
   for (int i = 0; i < brw->perfmon.unresolved_elements; i++) {
      if (brw->perfmon.unresolved[i] == monitor) {
         resolved = "Unresolved";
         break;
      }
   }

   DBG("%4d  %-7s %-6s %-10s %-11s <%3d, %3d, %3d>  %-6s %-9s\n",
       name,
       m->Active ? "Active" : "",
       m->Ended ? "Ended" : "",
       resolved,
       brw_is_perf_monitor_result_available(ctx, m) ? "Available" : "",
       monitor->oa_head_end,
       monitor->oa_middle_start,
       monitor->oa_tail_start,
       monitor->oa_bo ? "OA BO" : "",
       monitor->pipeline_stats_bo ? "Stats BO" : "");
}

void
brw_dump_perf_monitors(struct brw_context *brw)
{
   struct gl_context *ctx = &brw->ctx;
   DBG("Monitors: (OA users = %d)\n", brw->perfmon.oa_users);
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

static bool
monitor_needs_oa(struct brw_context *brw,
                 struct gl_perf_monitor_object *m)
{
   return m->ActiveGroups[OA_COUNTERS];
}

/**
 * Enable the Observability Architecture counters by whacking OACONTROL.
 */
static void
start_oa_counters(struct brw_context *brw)
{
   unsigned counter_format;

   /* Pick the counter format which gives us all the counters. */
   switch (brw->gen) {
   case 5:
      return; /* Ironlake counters are always running. */
   case 6:
      counter_format = 1; /* 0b001 */
      break;
   case 7:
      counter_format = 5; /* 0b101 */
      break;
   default:
      assert(!"Tried to enable OA counters on an unsupported generation.");
      return;
   }

   BEGIN_BATCH(3);
   OUT_BATCH(MI_LOAD_REGISTER_IMM | (3 - 2));
   OUT_BATCH(OACONTROL);
   OUT_BATCH(counter_format << OACONTROL_COUNTER_SELECT_SHIFT |
             OACONTROL_ENABLE_COUNTERS);
   ADVANCE_BATCH();
}

/**
 * Disable OA counters.
 */
static void
stop_oa_counters(struct brw_context *brw)
{
   /* Ironlake counters never stop. */
   if (brw->gen == 5)
      return;

   BEGIN_BATCH(3);
   OUT_BATCH(MI_LOAD_REGISTER_IMM | (3 - 2));
   OUT_BATCH(OACONTROL);
   OUT_BATCH(0);
   ADVANCE_BATCH();
}

/**
 * The amount of batch space it takes to emit an MI_REPORT_PERF_COUNT snapshot,
 * including the required PIPE_CONTROL flushes.
 *
 * Sandybridge is the worst case scenario: intel_batchbuffer_emit_mi_flush
 * expands to three PIPE_CONTROLs which are 4 DWords each.  We have to flush
 * before and after MI_REPORT_PERF_COUNT, so multiply by two.  Finally, add
 * the 3 DWords for MI_REPORT_PERF_COUNT itself.
 */
#define MI_REPORT_PERF_COUNT_BATCH_DWORDS (2 * (3 * 4) + 3)

/**
 * Emit an MI_REPORT_PERF_COUNT command packet.
 *
 * This writes the current OA counter values to buffer.
 */
static void
emit_mi_report_perf_count(struct brw_context *brw,
                          drm_intel_bo *bo,
                          uint32_t offset_in_bytes,
                          uint32_t report_id)
{
   assert(offset_in_bytes % 64 == 0);

   /* Make sure the commands to take a snapshot fits in a single batch. */
   intel_batchbuffer_require_space(brw, MI_REPORT_PERF_COUNT_BATCH_DWORDS * 4,
                                   RENDER_RING);
   int batch_used = brw->batch.used;

   /* Reports apparently don't always get written unless we flush first. */
   intel_batchbuffer_emit_mi_flush(brw);

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
   } else if (brw->gen == 6) {
      BEGIN_BATCH(3);
      OUT_BATCH(GEN6_MI_REPORT_PERF_COUNT);
      OUT_RELOC(bo, I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                offset_in_bytes | MI_COUNTER_ADDRESS_GTT);
      OUT_BATCH(report_id);
      ADVANCE_BATCH();
   } else if (brw->gen == 7) {
      BEGIN_BATCH(3);
      OUT_BATCH(GEN6_MI_REPORT_PERF_COUNT);
      OUT_RELOC(bo, I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
                offset_in_bytes);
      OUT_BATCH(report_id);
      ADVANCE_BATCH();
   } else {
      assert(!"Unsupported generation for performance counters.");
   }

   /* Reports apparently don't always get written unless we flush after. */
   intel_batchbuffer_emit_mi_flush(brw);

   (void) batch_used;
   assert(brw->batch.used - batch_used <= MI_REPORT_PERF_COUNT_BATCH_DWORDS * 4);
}

/**
 * Add a monitor to the global list of "unresolved monitors."
 *
 * Monitors are "unresolved" if they refer to OA counter snapshots in
 * bookend_bo.  Results (even partial ones) must be gathered for all
 * unresolved monitors before it's safe to discard bookend_bo.
 */
static void
add_to_unresolved_monitor_list(struct brw_context *brw,
                               struct brw_perf_monitor_object *monitor)
{
   if (brw->perfmon.unresolved_elements >=
       brw->perfmon.unresolved_array_size) {
      brw->perfmon.unresolved_array_size *= 2;
      brw->perfmon.unresolved = reralloc(brw, brw->perfmon.unresolved,
                                         struct brw_perf_monitor_object *,
                                         brw->perfmon.unresolved_array_size);
   }

   brw->perfmon.unresolved[brw->perfmon.unresolved_elements++] = monitor;
}

/**
 * If possible, throw away the contents of bookend BO.
 *
 * When all monitoring stops, and no monitors need data from bookend_bo to
 * compute results, we can discard it and start writing snapshots at the
 * beginning again.  This helps reduce the amount of buffer wraparound.
 */
static void
clean_bookend_bo(struct brw_context *brw)
{
   if (brw->perfmon.unresolved_elements == 0) {
      DBG("***Resetting bookend snapshots to 0\n");
      brw->perfmon.bookend_snapshots = 0;
   }
}

/**
 * Remove a monitor from the global list of "unresolved monitors."
 *
 * This can happen when:
 * - We finish computing a completed monitor's results.
 * - We discard unwanted monitor results.
 * - A monitor's results can be computed without relying on bookend_bo.
 */
static void
drop_from_unresolved_monitor_list(struct brw_context *brw,
                                  struct brw_perf_monitor_object *monitor)
{
   for (int i = 0; i < brw->perfmon.unresolved_elements; i++) {
      if (brw->perfmon.unresolved[i] == monitor) {
         int last_elt = --brw->perfmon.unresolved_elements;

         if (i == last_elt) {
            brw->perfmon.unresolved[i] = NULL;
         } else {
            brw->perfmon.unresolved[i] = brw->perfmon.unresolved[last_elt];
         }

         clean_bookend_bo(brw);
         return;
      }
   }
}

/**
 * Given pointers to starting and ending OA snapshots, add the deltas for each
 * counter to the results.
 */
static void
add_deltas(struct brw_context *brw,
           struct brw_perf_monitor_object *monitor,
           uint32_t *start, uint32_t *end)
{
   /* Look for expected report ID values to ensure data is present. */
   assert(start[0] == REPORT_ID);
   assert(end[0] == REPORT_ID);

   /* Subtract each counter's ending and starting values, then add the
    * difference to the counter's value so far.
    */
   for (int i = 3; i < brw->perfmon.entries_per_oa_snapshot; i++) {
      /* When debugging, it's useful to note when the ending value is less than
       * the starting value; aggregating counters should always increase in
       * value (or remain unchanged).  This happens periodically due to
       * wraparound, but can also indicate serious problems.
       */
#ifdef DEBUG
      if (end[i] < start[i]) {
         int counter = brw->perfmon.oa_snapshot_layout[i];
         if (counter >= 0) {
            DBG("WARNING: \"%s\" ending value was less than the starting "
                "value: %u < %u (end - start = %u)\n",
                brw->ctx.PerfMonitor.Groups[0].Counters[counter].Name,
                end[i], start[i], end[i] - start[i]);
         }
      }
#endif
      monitor->oa_results[i] += end[i] - start[i];
   }
}

/**
 * Gather OA counter results (partial or full) from a series of snapshots.
 *
 * Monitoring can start or stop at any time, likely at some point mid-batch.
 * We write snapshots for both events, storing them in monitor->oa_bo.
 *
 * Ideally, we would simply subtract those two snapshots to obtain the final
 * counter results.  Unfortunately, our hardware doesn't preserve their values
 * across context switches or GPU sleep states.  In order to support multiple
 * concurrent OA clients, as well as reliable data across power management,
 * we have to take snapshots at the start and end of batches as well.
 *
 * This results in a three-part sequence of (start, end) intervals:
 * - The "head" is from the BeginPerfMonitor snapshot to the end of the first
 *   batchbuffer.
 * - The "middle" is a series of (batch start, batch end) snapshots which
 *   bookend any batchbuffers between the ones which start/end monitoring.
 * - The "tail" is from the start of the last batch where monitoring was
 *   active to the EndPerfMonitor snapshot.
 *
 * Due to wrapping in the bookend BO, we may have to accumulate partial results.
 * If so, we handle the "head" and any "middle" results so far.  When monitoring
 * eventually ends, we handle additional "middle" batches and the "tail."
 */
static void
gather_oa_results(struct brw_context *brw,
                  struct brw_perf_monitor_object *monitor,
                  uint32_t *bookend_buffer)
{
   struct gl_perf_monitor_object *m = &monitor->base;
   assert(monitor->oa_bo != NULL);

   drm_intel_bo_map(monitor->oa_bo, false);
   uint32_t *monitor_buffer = monitor->oa_bo->virtual;

   /* If monitoring was entirely contained within a single batch, then the
    * bookend BO is irrelevant.  Just subtract monitor->bo's two snapshots.
    */
   if (monitor->oa_middle_start == -1) {
      add_deltas(brw, monitor,
                 monitor_buffer,
                 monitor_buffer + (SECOND_SNAPSHOT_OFFSET_IN_BYTES /
                                   sizeof(uint32_t)));
      drm_intel_bo_unmap(monitor->oa_bo);
      return;
   }

   const int snapshot_size = brw->perfmon.entries_per_oa_snapshot;

   /* First, add the contributions from the "head" interval:
    * (snapshot taken at BeginPerfMonitor time,
    *  snapshot taken at the end of the first batch after monitoring began)
    */
   if (monitor->oa_head_end != -1) {
      assert(monitor->oa_head_end < brw->perfmon.bookend_snapshots);
      add_deltas(brw, monitor,
                 monitor_buffer,
                 bookend_buffer + snapshot_size * monitor->oa_head_end);

      /* Make sure we don't count the "head" again in the future. */
      monitor->oa_head_end = -1;
   }

   /* Next, count the contributions from the "middle" batches.  These are
    * (batch begin, batch end) deltas while monitoring was active.
    */
   int last_snapshot;
   if (m->Ended)
      last_snapshot = monitor->oa_tail_start;
   else
      last_snapshot = brw->perfmon.bookend_snapshots;

   for (int s = monitor->oa_middle_start; s < last_snapshot; s += 2) {
      add_deltas(brw, monitor,
                 bookend_buffer + snapshot_size * s,
                 bookend_buffer + snapshot_size * (s + 1));
   }

   /* Finally, if the monitor has ended, we need to count the contributions of
    * the "tail" interval:
    * (start of the batch where monitoring ended, EndPerfMonitor snapshot)
    */
   if (m->Ended) {
      assert(monitor->oa_tail_start != -1);
      add_deltas(brw, monitor,
                 bookend_buffer + snapshot_size * monitor->oa_tail_start,
                 monitor_buffer + (SECOND_SNAPSHOT_OFFSET_IN_BYTES /
                                   sizeof(uint32_t)));
   }

   drm_intel_bo_unmap(monitor->oa_bo);

   /* If the monitor has ended, then we've gathered all the results, and
    * can free the monitor's OA BO.
    */
   if (m->Ended) {
      drm_intel_bo_unreference(monitor->oa_bo);
      monitor->oa_bo = NULL;

      /* The monitor's OA result is now resolved. */
      DBG("Marking %d resolved - results gathered\n", m->Name);
      drop_from_unresolved_monitor_list(brw, monitor);
   }
}

/**
 * Handle running out of space in the bookend BO.
 *
 * When we run out of space in the bookend BO, we need to gather up partial
 * results for every unresolved monitor.  This allows us to free the snapshot
 * data in bookend_bo, freeing up the space for reuse.  We call this "wrapping."
 *
 * This will completely compute the result for any unresolved monitors that
 * have ended.
 */
static void
wrap_bookend_bo(struct brw_context *brw)
{
   DBG("****Wrap bookend BO****\n");
   /* Note that wrapping will only occur at the start of a batch, since that's
    * where we reserve space.  So the current batch won't reference bookend_bo
    * or any monitor BOs.  This means we don't need to worry about
    * synchronization.
    *
    * Also, EndPerfMonitor guarantees that only monitors which span multiple
    * batches exist in the unresolved monitor list.
    */
   assert(brw->perfmon.oa_users > 0);

   drm_intel_bo_map(brw->perfmon.bookend_bo, false);
   uint32_t *bookend_buffer = brw->perfmon.bookend_bo->virtual;
   for (int i = 0; i < brw->perfmon.unresolved_elements; i++) {
      struct brw_perf_monitor_object *monitor = brw->perfmon.unresolved[i];
      struct gl_perf_monitor_object *m = &monitor->base;

      gather_oa_results(brw, monitor, bookend_buffer);

      if (m->Ended) {
         /* gather_oa_results() dropped the monitor from the unresolved list,
          * throwing our indices off by one.
          */
         --i;
      } else {
         /* When we create the new bookend_bo, snapshot #0 will be the
          * beginning of another "middle" BO.
          */
         monitor->oa_middle_start = 0;
         assert(monitor->oa_head_end == -1);
         assert(monitor->oa_tail_start == -1);
      }
   }
   drm_intel_bo_unmap(brw->perfmon.bookend_bo);

   brw->perfmon.bookend_snapshots = 0;
}

/* This is fairly arbitrary; the trade off is memory usage vs. extra overhead
 * from wrapping.  On Gen7, 32768 should be enough for for 128 snapshots before
 * wrapping (since each is 256 bytes).
 */
#define BOOKEND_BO_SIZE_BYTES 32768

/**
 * Check whether bookend_bo has space for a given number of snapshots.
 */
static bool
has_space_for_bookend_snapshots(struct brw_context *brw, int snapshots)
{
   int snapshot_bytes = brw->perfmon.entries_per_oa_snapshot * sizeof(uint32_t);

   /* There are brw->perfmon.bookend_snapshots - 1 existing snapshots. */
   int total_snapshots = (brw->perfmon.bookend_snapshots - 1) + snapshots;

   return total_snapshots * snapshot_bytes < BOOKEND_BO_SIZE_BYTES;
}

/**
 * Write an OA counter snapshot to bookend_bo.
 */
static void
emit_bookend_snapshot(struct brw_context *brw)
{
   int snapshot_bytes = brw->perfmon.entries_per_oa_snapshot * sizeof(uint32_t);
   int offset_in_bytes = brw->perfmon.bookend_snapshots * snapshot_bytes;

   emit_mi_report_perf_count(brw, brw->perfmon.bookend_bo, offset_in_bytes,
                             REPORT_ID);
   ++brw->perfmon.bookend_snapshots;
}

/******************************************************************************/

/**
 * Initialize a monitor to sane starting state; throw away old buffers.
 */
static void
reinitialize_perf_monitor(struct brw_context *brw,
                          struct brw_perf_monitor_object *monitor)
{
   if (monitor->oa_bo) {
      drm_intel_bo_unreference(monitor->oa_bo);
      monitor->oa_bo = NULL;
   }

   /* Since the results are now invalid, we don't need to hold on to any
    * snapshots in bookend_bo.  The monitor is effectively "resolved."
    */
   drop_from_unresolved_monitor_list(brw, monitor);

   monitor->oa_head_end = -1;
   monitor->oa_middle_start = -1;
   monitor->oa_tail_start = -1;

   free(monitor->oa_results);
   monitor->oa_results = NULL;

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

   if (monitor_needs_oa(brw, m)) {
      /* If the global OA bookend BO doesn't exist, allocate it.  This should
       * only happen once, but we delay until BeginPerfMonitor time to avoid
       * wasting memory for contexts that don't use performance monitors.
       */
      if (!brw->perfmon.bookend_bo) {
         brw->perfmon.bookend_bo = drm_intel_bo_alloc(brw->bufmgr,
                                                      "OA bookend BO",
                                                      BOOKEND_BO_SIZE_BYTES, 64);
      }

      monitor->oa_bo =
         drm_intel_bo_alloc(brw->bufmgr, "perf. monitor OA bo", 4096, 64);
#ifdef DEBUG
      /* Pre-filling the BO helps debug whether writes landed. */
      drm_intel_bo_map(monitor->oa_bo, true);
      memset((char *) monitor->oa_bo->virtual, 0xff, 4096);
      drm_intel_bo_unmap(monitor->oa_bo);
#endif

      /* Allocate storage for accumulated OA counter values. */
      monitor->oa_results =
         calloc(brw->perfmon.entries_per_oa_snapshot, sizeof(uint32_t));

      /* If the OA counters aren't already on, enable them. */
      if (brw->perfmon.oa_users == 0) {
         /* Ensure the OACONTROL enable and snapshot land in the same batch. */
         int space = (MI_REPORT_PERF_COUNT_BATCH_DWORDS + 3) * 4;
         intel_batchbuffer_require_space(brw, space, RENDER_RING);
         start_oa_counters(brw);
      }

      /* Take a starting OA counter snapshot. */
      emit_mi_report_perf_count(brw, monitor->oa_bo, 0, REPORT_ID);

      monitor->oa_head_end = brw->perfmon.bookend_snapshots;
      monitor->oa_middle_start = brw->perfmon.bookend_snapshots + 1;
      monitor->oa_tail_start = -1;

      /* Add the monitor to the unresolved list. */
      add_to_unresolved_monitor_list(brw, monitor);

      ++brw->perfmon.oa_users;
   }

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

   if (monitor_needs_oa(brw, m)) {
      /* Take an ending OA counter snapshot. */
      emit_mi_report_perf_count(brw, monitor->oa_bo,
                                SECOND_SNAPSHOT_OFFSET_IN_BYTES, REPORT_ID);

      --brw->perfmon.oa_users;

      if (brw->perfmon.oa_users == 0)
         stop_oa_counters(brw);

      if (monitor->oa_head_end == brw->perfmon.bookend_snapshots) {
         assert(monitor->oa_head_end != -1);
         /* We never actually wrote the snapshot for the end of the first batch
          * after BeginPerfMonitor.  This means that monitoring was contained
          * entirely within a single batch, so we can ignore bookend_bo and
          * just compare the monitor's begin/end snapshots directly.
          */
         monitor->oa_head_end = -1;
         monitor->oa_middle_start = -1;
         monitor->oa_tail_start = -1;

         /* We can also mark it resolved since it won't depend on bookend_bo. */
         DBG("Marking %d resolved - entirely in one batch\n", m->Name);
         drop_from_unresolved_monitor_list(brw, monitor);
      } else {
         /* We've written at least one batch end snapshot, so the monitoring
          * spanned multiple batches.  Mark which snapshot corresponds to the
          * start of the current batch.
          */
         monitor->oa_tail_start = brw->perfmon.bookend_snapshots - 1;
      }
   }

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

   bool oa_available = true;
   bool stats_available = true;

   if (monitor_needs_oa(brw, m)) {
      oa_available = !monitor->oa_bo ||
         (!drm_intel_bo_references(brw->batch.bo, monitor->oa_bo) &&
          !drm_intel_bo_busy(monitor->oa_bo));
   }

   if (monitor_needs_statistics_registers(brw, m)) {
      stats_available = !monitor->pipeline_stats_bo ||
         (!drm_intel_bo_references(brw->batch.bo, monitor->pipeline_stats_bo) &&
          !drm_intel_bo_busy(monitor->pipeline_stats_bo));
   }

   return oa_available && stats_available;
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

   if (monitor_needs_oa(brw, m)) {
      /* Gather up the results from the BO, unless we already did due to the
       * bookend BO wrapping.
       */
      if (monitor->oa_bo) {
         /* Since the result is available, all the necessary snapshots will
          * have been written to the bookend BO.  If other monitors are
          * active, the bookend BO may be busy or referenced by the current
          * batch, but only for writing snapshots beyond oa_tail_start,
          * which we don't care about.
          *
          * Using an unsynchronized mapping avoids stalling for an
          * indeterminate amount of time.
          */
         drm_intel_gem_bo_map_unsynchronized(brw->perfmon.bookend_bo);

         gather_oa_results(brw, monitor, brw->perfmon.bookend_bo->virtual);

         drm_intel_bo_unmap(brw->perfmon.bookend_bo);
      }

      for (int i = 0; i < brw->perfmon.entries_per_oa_snapshot; i++) {
         int group = OA_COUNTERS;
         int counter = brw->perfmon.oa_snapshot_layout[i];

         /* We always capture all the OA counters, but the application may
          * have only asked for a subset.  Skip unwanted counters.
          */
         if (counter < 0 || !BITSET_TEST(m->ActiveCounters[group], counter))
            continue;

         data[offset++] = group;
         data[offset++] = counter;
         data[offset++] = monitor->oa_results[i];
      }

      clean_bookend_bo(brw);
   }

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

/**
 * Called at the start of every render ring batch.
 *
 * Enable OA counters and emit the "start of batchbuffer" bookend OA snapshot.
 * Since it's a new batch, there will be plenty of space for the commands.
 */
void
brw_perf_monitor_new_batch(struct brw_context *brw)
{
   assert(brw->batch.ring == RENDER_RING);
   assert(brw->gen < 6 || brw->batch.used == 0);

   if (brw->perfmon.oa_users == 0)
      return;

   start_oa_counters(brw);

   /* Make sure bookend_bo has enough space for a pair of snapshots.
    * If not, "wrap" the BO: gather up any results so far, and start from
    * the beginning of the buffer.  Reserving a pair guarantees that wrapping
    * will only happen at the beginning of a batch, where it's safe to map BOs
    * (as the batch is empty and can't refer to any of them yet).
    */
   if (!has_space_for_bookend_snapshots(brw, 2))
      wrap_bookend_bo(brw);

   DBG("Bookend Begin Snapshot (%d)\n", brw->perfmon.bookend_snapshots);
   emit_bookend_snapshot(brw);
}

/**
 * Called at the end of every render ring batch.
 *
 * Emit the "end of batchbuffer" bookend OA snapshot and disable the counters.
 *
 * This relies on there being enough space in BATCH_RESERVED.
 */
void
brw_perf_monitor_finish_batch(struct brw_context *brw)
{
   assert(brw->batch.ring == RENDER_RING);

   if (brw->perfmon.oa_users == 0)
      return;

   DBG("Bookend End Snapshot (%d)\n", brw->perfmon.bookend_snapshots);

   /* Not safe to wrap; should've reserved space already. */
   assert(has_space_for_bookend_snapshots(brw, 1));

   emit_bookend_snapshot(brw);

   stop_oa_counters(brw);
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

   brw->perfmon.unresolved =
      ralloc_array(brw, struct brw_perf_monitor_object *, 1);
   brw->perfmon.unresolved_elements = 0;
   brw->perfmon.unresolved_array_size = 1;
}
