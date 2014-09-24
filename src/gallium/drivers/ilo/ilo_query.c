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

#include "ilo_context.h"
#include "ilo_cp.h"
#include "ilo_draw.h"
#include "ilo_query.h"

static const struct {
   bool (*init)(struct ilo_context *ilo, struct ilo_query *q);
   void (*begin)(struct ilo_context *ilo, struct ilo_query *q);
   void (*end)(struct ilo_context *ilo, struct ilo_query *q);
   void (*process)(struct ilo_context *ilo, struct ilo_query *q);
} ilo_query_table[PIPE_QUERY_TYPES] = {
#define INFO(mod) {                    \
   .init = ilo_init_ ## mod ## _query,         \
   .begin = ilo_begin_ ## mod ## _query,       \
   .end = ilo_end_ ## mod ## _query,           \
   .process = ilo_process_ ## mod ## _query,   \
}
#define INFOX(prefix) { NULL, NULL, NULL, NULL, }

   [PIPE_QUERY_OCCLUSION_COUNTER]      = INFO(draw),
   [PIPE_QUERY_OCCLUSION_PREDICATE]    = INFOX(draw),
   [PIPE_QUERY_TIMESTAMP]              = INFO(draw),
   [PIPE_QUERY_TIMESTAMP_DISJOINT]     = INFOX(draw),
   [PIPE_QUERY_TIME_ELAPSED]           = INFO(draw),
   [PIPE_QUERY_PRIMITIVES_GENERATED]   = INFO(draw),
   [PIPE_QUERY_PRIMITIVES_EMITTED]     = INFO(draw),
   [PIPE_QUERY_SO_STATISTICS]          = INFOX(draw),
   [PIPE_QUERY_SO_OVERFLOW_PREDICATE]  = INFOX(draw),
   [PIPE_QUERY_GPU_FINISHED]           = INFOX(draw),
   [PIPE_QUERY_PIPELINE_STATISTICS]    = INFO(draw),

#undef INFO
#undef INFOX
};

static inline struct ilo_query *
ilo_query(struct pipe_query *query)
{
   return (struct ilo_query *) query;
}

static struct pipe_query *
ilo_create_query(struct pipe_context *pipe, unsigned query_type, unsigned index)
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
   q->index = index;

   list_inithead(&q->list);

   if (!ilo_query_table[q->type].init(ilo_context(pipe), q)) {
      FREE(q);
      return NULL;
   }

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
   struct ilo_query *q = ilo_query(query);

   if (q->active)
      return;

   util_query_clear_result(&q->result, q->type);
   q->used = 0;
   q->active = true;

   ilo_query_table[q->type].begin(ilo_context(pipe), q);
}

static void
ilo_end_query(struct pipe_context *pipe, struct pipe_query *query)
{
   struct ilo_query *q = ilo_query(query);

   if (!q->active) {
      /* require ilo_begin_query() first */
      if (q->in_pairs)
         return;

      ilo_begin_query(pipe, query);
   }

   q->active = false;

   ilo_query_table[q->type].end(ilo_context(pipe), q);
}

/**
 * Serialize the result.  The size of \p buf is
 * sizeof(union pipe_query_result).
 */
static void
query_serialize(const struct ilo_query *q, void *buf)
{
   switch (q->type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
   case PIPE_QUERY_TIMESTAMP:
   case PIPE_QUERY_TIME_ELAPSED:
   case PIPE_QUERY_PRIMITIVES_GENERATED:
   case PIPE_QUERY_PRIMITIVES_EMITTED:
      {
         uint64_t *dst = buf;
         dst[0] = q->result.u64;
      }
      break;
   case PIPE_QUERY_PIPELINE_STATISTICS:
      {
         const struct pipe_query_data_pipeline_statistics *stats =
            &q->result.pipeline_statistics;
         uint64_t *dst = buf;

         dst[0] = stats->ia_vertices;
         dst[1] = stats->ia_primitives;
         dst[2] = stats->vs_invocations;
         dst[3] = stats->gs_invocations;
         dst[4] = stats->gs_primitives;
         dst[5] = stats->c_invocations;
         dst[6] = stats->c_primitives;
         dst[7] = stats->ps_invocations;
         dst[8] = stats->hs_invocations;
         dst[9] = stats->ds_invocations;
         dst[10] = stats->cs_invocations;
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
   struct ilo_query *q = ilo_query(query);

   if (q->active)
      return false;

   if (q->bo) {
      struct ilo_cp *cp = ilo_context(pipe)->cp;

      if (ilo_builder_has_reloc(&cp->builder, q->bo))
         ilo_cp_submit(cp, "syncing for queries");

      if (!wait && intel_bo_is_busy(q->bo))
         return false;
   }

   ilo_query_table[q->type].process(ilo_context(pipe), q);

   if (result)
      query_serialize(q, (void *) result);

   return true;
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
