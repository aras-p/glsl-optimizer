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

/** @file support for ARB_query_object
 *
 * ARB_query_object is implemented by using the PIPE_CONTROL command to stall
 * execution on the completion of previous depth tests, and write the
 * current PS_DEPTH_COUNT to a buffer object.
 *
 * We use before and after counts when drawing during a query so that
 * we don't pick up other clients' query data in ours.  To reduce overhead,
 * a single BO is used to record the query data for all active queries at
 * once.  This also gives us a simple bound on how much batchbuffer space is
 * required for handling queries, so that we can be sure that we won't
 * have to emit a batchbuffer without getting the ending PS_DEPTH_COUNT.
 */
#include "main/simple_list.h"
#include "main/imports.h"

#include "brw_context.h"
#include "brw_state.h"
#include "intel_batchbuffer.h"
#include "intel_reg.h"

/** Waits on the query object's BO and totals the results for this query */
static void
brw_queryobj_get_results(struct brw_query_object *query)
{
   int i;
   uint64_t *results;

   if (query->bo == NULL)
      return;

   /* Map and count the pixels from the current query BO */
   dri_bo_map(query->bo, GL_FALSE);
   results = query->bo->virtual;
   for (i = query->first_index; i <= query->last_index; i++) {
      query->Base.Result += results[i * 2 + 1] - results[i * 2];
   }
   dri_bo_unmap(query->bo);

   dri_bo_unreference(query->bo);
   query->bo = NULL;
}

static struct gl_query_object *
brw_new_query_object(GLcontext *ctx, GLuint id)
{
   struct brw_query_object *query;

   query = calloc(1, sizeof(struct brw_query_object));

   query->Base.Id = id;
   query->Base.Result = 0;
   query->Base.Active = GL_FALSE;
   query->Base.Ready = GL_TRUE;

   return &query->Base;
}

static void
brw_delete_query(GLcontext *ctx, struct gl_query_object *q)
{
   struct brw_query_object *query = (struct brw_query_object *)q;

   dri_bo_unreference(query->bo);
   free(query);
}

static void
brw_begin_query(GLcontext *ctx, struct gl_query_object *q)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_context *intel = intel_context(ctx);
   struct brw_query_object *query = (struct brw_query_object *)q;

   /* Reset our driver's tracking of query state. */
   dri_bo_unreference(query->bo);
   query->bo = NULL;
   query->first_index = -1;
   query->last_index = -1;

   insert_at_head(&brw->query.active_head, query);
   intel->stats_wm++;
}

/**
 * Begin the ARB_occlusion_query query on a query object.
 */
static void
brw_end_query(GLcontext *ctx, struct gl_query_object *q)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_context *intel = intel_context(ctx);
   struct brw_query_object *query = (struct brw_query_object *)q;

   /* Flush the batchbuffer in case it has writes to our query BO.
    * Have later queries write to a new query BO so that further rendering
    * doesn't delay the collection of our results.
    */
   if (query->bo) {
      brw_emit_query_end(brw);
      intel_batchbuffer_flush(intel->batch);

      dri_bo_unreference(brw->query.bo);
      brw->query.bo = NULL;
   }

   remove_from_list(query);

   intel->stats_wm--;
}

static void brw_wait_query(GLcontext *ctx, struct gl_query_object *q)
{
   struct brw_query_object *query = (struct brw_query_object *)q;

   brw_queryobj_get_results(query);
   query->Base.Ready = GL_TRUE;
}

static void brw_check_query(GLcontext *ctx, struct gl_query_object *q)
{
   struct brw_query_object *query = (struct brw_query_object *)q;

   if (query->bo == NULL || !drm_intel_bo_busy(query->bo)) {
      brw_queryobj_get_results(query);
      query->Base.Ready = GL_TRUE;
   }
}

/** Called to set up the query BO and account for its aperture space */
void
brw_prepare_query_begin(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   /* Skip if we're not doing any queries. */
   if (is_empty_list(&brw->query.active_head))
      return;

   /* Get a new query BO if we're going to need it. */
   if (brw->query.bo == NULL ||
       brw->query.index * 2 + 1 >= 4096 / sizeof(uint64_t)) {
      dri_bo_unreference(brw->query.bo);
      brw->query.bo = NULL;

      brw->query.bo = dri_bo_alloc(intel->bufmgr, "query", 4096, 1);
      brw->query.index = 0;
   }

   brw_add_validated_bo(brw, brw->query.bo);
}

/** Called just before primitive drawing to get a beginning PS_DEPTH_COUNT. */
void
brw_emit_query_begin(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;
   struct brw_query_object *query;

   /* Skip if we're not doing any queries, or we've emitted the start. */
   if (brw->query.active || is_empty_list(&brw->query.active_head))
      return;

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_PIPE_CONTROL |
	     PIPE_CONTROL_DEPTH_STALL |
	     PIPE_CONTROL_WRITE_DEPTH_COUNT);
   /* This object could be mapped cacheable, but we don't have an exposed
    * mechanism to support that.  Since it's going uncached, tell GEM that
    * we're writing to it.  The usual clflush should be all that's required
    * to pick up the results.
    */
   OUT_RELOC(brw->query.bo,
	     I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
	     PIPE_CONTROL_GLOBAL_GTT_WRITE |
	     ((brw->query.index * 2) * sizeof(uint64_t)));
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();

   foreach(query, &brw->query.active_head) {
      if (query->bo != brw->query.bo) {
	 if (query->bo != NULL)
	    brw_queryobj_get_results(query);
	 dri_bo_reference(brw->query.bo);
	 query->bo = brw->query.bo;
	 query->first_index = brw->query.index;
      }
      query->last_index = brw->query.index;
   }
   brw->query.active = GL_TRUE;
}

/** Called at batchbuffer flush to get an ending PS_DEPTH_COUNT */
void
brw_emit_query_end(struct brw_context *brw)
{
   struct intel_context *intel = &brw->intel;

   if (!brw->query.active)
      return;

   BEGIN_BATCH(4);
   OUT_BATCH(_3DSTATE_PIPE_CONTROL |
	     PIPE_CONTROL_DEPTH_STALL |
	     PIPE_CONTROL_WRITE_DEPTH_COUNT);
   OUT_RELOC(brw->query.bo,
	     I915_GEM_DOMAIN_INSTRUCTION, I915_GEM_DOMAIN_INSTRUCTION,
	     PIPE_CONTROL_GLOBAL_GTT_WRITE |
	     ((brw->query.index * 2 + 1) * sizeof(uint64_t)));
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();

   brw->query.active = GL_FALSE;
   brw->query.index++;
}

void brw_init_queryobj_functions(struct dd_function_table *functions)
{
   functions->NewQueryObject = brw_new_query_object;
   functions->DeleteQuery = brw_delete_query;
   functions->BeginQuery = brw_begin_query;
   functions->EndQuery = brw_end_query;
   functions->CheckQuery = brw_check_query;
   functions->WaitQuery = brw_wait_query;
}
