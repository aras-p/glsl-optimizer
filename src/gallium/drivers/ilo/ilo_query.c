/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "intel_winsys.h"

#include "ilo_3d.h"
#include "ilo_context.h"
#include "ilo_cp.h"
#include "ilo_query.h"

static const struct {
   const char *name;

   void (*begin)(struct ilo_context *ilo, struct ilo_query *q);
   void (*end)(struct ilo_context *ilo, struct ilo_query *q);
   void (*process)(struct ilo_context *ilo, struct ilo_query *q);
} query_info[PIPE_QUERY_TYPES] = {
#define INFO(prefix, desc) {        \
   .name = desc,                    \
   .begin = prefix ## _begin_query, \
   .end = prefix ## _end_query,     \
   .process = prefix ## _process_query, \
}
#define INFOX(prefix, desc) { desc, NULL, NULL, NULL, }

   [PIPE_QUERY_OCCLUSION_COUNTER]      = INFO(ilo_3d, "occlusion counter"),
   [PIPE_QUERY_OCCLUSION_PREDICATE]    = INFOX(ilo_3d, "occlusion pred."),
   [PIPE_QUERY_TIMESTAMP]              = INFO(ilo_3d, "timestamp"),
   [PIPE_QUERY_TIMESTAMP_DISJOINT]     = INFOX(ilo_3d, "timestamp disjoint"),
   [PIPE_QUERY_TIME_ELAPSED]           = INFO(ilo_3d, "time elapsed"),
   [PIPE_QUERY_PRIMITIVES_GENERATED]   = INFO(ilo_3d, "primitives generated"),
   [PIPE_QUERY_PRIMITIVES_EMITTED]     = INFO(ilo_3d, "primitives emitted"),
   [PIPE_QUERY_SO_STATISTICS]          = INFOX(ilo_3d, "so statistics"),
   [PIPE_QUERY_SO_OVERFLOW_PREDICATE]  = INFOX(ilo_3d, "so overflow pred."),
   [PIPE_QUERY_GPU_FINISHED]           = INFOX(ilo_3d, "gpu finished"),
   [PIPE_QUERY_PIPELINE_STATISTICS]    = INFO(ilo_3d, "pipeline statistics"),

#undef INFO
#undef INFOX
};

static inline struct ilo_query *
ilo_query(struct pipe_query *query)
{
   return (struct ilo_query *) query;
}

static struct pipe_query *
ilo_create_query(struct pipe_context *pipe, unsigned query_type)
{
   struct ilo_query *q;

   switch (query_type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_TIMESTAMP:
   case PIPE_QUERY_TIME_ELAPSED:
   case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_PRIMITIVES_EMITTED:
   case PIPE_QUERY_PIPELINE_STATISTICS:
      break;
   default:
      return NULL;
   }

   q = CALLOC_STRUCT(ilo_query);
   if (!q)
      return NULL;

   q->type = query_type;
   list_inithead(&q->list);

   return (struct pipe_query *) q;
}

static void
ilo_destroy_query(struct pipe_context *pipe, struct pipe_query *query)
{
   struct ilo_query *q = ilo_query(query);

   if (q->bo)
      intel_bo_unreference(q->bo);

   FREE(q);
}

static void
ilo_begin_query(struct pipe_context *pipe, struct pipe_query *query)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_query *q = ilo_query(query);

   q->active = true;

   query_info[q->type].begin(ilo, q);
}

static void
ilo_end_query(struct pipe_context *pipe, struct pipe_query *query)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_query *q = ilo_query(query);

   query_info[q->type].end(ilo, q);

   /*
    * some queries such as timestamp query does not require a call to
    * begin_query() so q->active is always false
    */
   q->active = false;
}

/**
 * The type (union pipe_query_result) indicates only the size of the buffer.
 * Callers expect the result to be "serialized".
 */
static void
serialize_query_data(unsigned type, const union pipe_query_result *data,
                     void *buf)
{
   switch (type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_TIMESTAMP:
   case PIPE_QUERY_TIME_ELAPSED:
   case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      {
         uint64_t *r = buf;
         r[0] = data->u64;
      }
      break;
   case PIPE_QUERY_PIPELINE_STATISTICS:
      {
         uint64_t *r = buf;
         r[0] = data->pipeline_statistics.ia_vertices;
         r[1] = data->pipeline_statistics.ia_primitives;
         r[2] = data->pipeline_statistics.vs_invocations;
         r[3] = data->pipeline_statistics.gs_invocations;
         r[4] = data->pipeline_statistics.gs_primitives;
         r[5] = data->pipeline_statistics.c_invocations;
         r[6] = data->pipeline_statistics.c_primitives;
         r[7] = data->pipeline_statistics.ps_invocations;
         r[8] = data->pipeline_statistics.hs_invocations;
         r[9] = data->pipeline_statistics.ds_invocations;
         r[10] = data->pipeline_statistics.cs_invocations;
      }
      break;
   default:
      memset(buf, 0, sizeof(union pipe_query_result));
      break;
   }
}

static boolean
ilo_get_query_result(struct pipe_context *pipe, struct pipe_query *query,
                     boolean wait, union pipe_query_result *result)
{
   struct ilo_context *ilo = ilo_context(pipe);
   struct ilo_query *q = ilo_query(query);

   if (q->active)
      return false;

   if (q->bo) {
      if (intel_bo_has_reloc(ilo->cp->bo, q->bo))
         ilo_cp_flush(ilo->cp, "syncing for queries");

      if (!wait && intel_bo_is_busy(q->bo))
         return false;

      query_info[q->type].process(ilo, q);
   }

   if (result)
      serialize_query_data(q->type, &q->data, (void *) result);

   return true;
}

/**
 * Allocate a query bo for reading hardware statistics.
 *
 * \param reg_count specifies how many registers need to be read.
 * \param repeat_count specifies how many times the registers are read.  If
 *        zero or negative, a 4KB bo is allocated.
 */
bool
ilo_query_alloc_bo(struct ilo_query *q, int reg_count, int repeat_count,
                   struct intel_winsys *winsys)
{
   const char *name;
   int reg_total;

   name = query_info[q->type].name;

   reg_total = reg_count * repeat_count;
   if (reg_total <= 0)
      reg_total = 4096 / sizeof(uint64_t);

   /* (re-)allocate the bo */
   if (q->reg_total < reg_total) {
      /* registers are 64-bit */
      const int size = reg_total * sizeof(uint64_t);

      if (q->bo)
         intel_bo_unreference(q->bo);

      q->bo = intel_winsys_alloc_buffer(winsys,
            name, size, INTEL_DOMAIN_INSTRUCTION);
      q->reg_total = (q->bo) ? reg_total : 0;
   }

   /* avoid partial reads */
   if (reg_count)
      q->reg_total -= q->reg_total % reg_count;

   q->reg_read = 0;

   return (q->bo != NULL);
}

/**
 * Initialize query-related functions.
 */
void
ilo_init_query_functions(struct ilo_context *ilo)
{
   ilo->base.create_query = ilo_create_query;
   ilo->base.destroy_query = ilo_destroy_query;
   ilo->base.begin_query = ilo_begin_query;
   ilo->base.end_query = ilo_end_query;
   ilo->base.get_query_result = ilo_get_query_result;
}
