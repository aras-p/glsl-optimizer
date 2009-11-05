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
#include "util/u_simple_list.h"

#include "brw_context.h"
#include "brw_state.h"
#include "brw_batchbuffer.h"
#include "brw_reg.h"

/** Waits on the query object's BO and totals the results for this query */
static boolean
brw_query_get_result(struct pipe_context *pipe,
		     struct pipe_query *q,
		     boolean wait,
		     uint64_t *result)
{
   struct brw_context *brw = brw_context(pipe);
   struct brw_query_object *query = (struct brw_query_object *)q;

   /* Map and count the pixels from the current query BO */
   if (query->bo) {
      int i;
      uint64_t *map;
      
      if (brw->sws->bo_is_busy(query->bo) && !wait)
	 return FALSE;
      
      map = bo_map_read(brw->sws, query->bo);
      if (map == NULL)
	 return FALSE;
      
      for (i = query->first_index; i <= query->last_index; i++) {
	 query->result += map[i * 2 + 1] - map[i * 2];
      }

      brw->sws->bo_unmap(query->bo);
      bo_reference(&query->bo, NULL);
   }

   *result = query->result;
   return TRUE;
}

static struct pipe_query *
brw_query_create(struct pipe_context *pipe, unsigned type )
{
   struct brw_query_object *query;

   switch (type) {
   case PIPE_QUERY_OCCLUSION_COUNTER:
      query = CALLOC_STRUCT( brw_query_object );
      if (query == NULL)
	 return NULL;
      return (struct pipe_query *)query;
      
   default:
      return NULL;
   }
}

static void
brw_query_destroy(struct pipe_context *pipe, struct pipe_query *q)
{
   struct brw_query_object *query = (struct brw_query_object *)q;

   bo_reference(&query->bo, NULL);
   FREE(query);
}

static void
brw_query_begin(struct pipe_context *pipe, struct pipe_query *q)
{
   struct brw_context *brw = brw_context(pipe);
   struct brw_query_object *query = (struct brw_query_object *)q;

   /* Reset our driver's tracking of query state. */
   bo_reference(&query->bo, NULL);
   query->result = 0;
   query->first_index = -1;
   query->last_index = -1;

   insert_at_head(&brw->query.active_head, query);
   brw->query.stats_wm++;
   brw->state.dirty.mesa |= PIPE_NEW_QUERY;
}

static void
brw_query_end(struct pipe_context *pipe, struct pipe_query *q)
{
   struct brw_context *brw = brw_context(pipe);
   struct brw_query_object *query = (struct brw_query_object *)q;

   /* Flush the batchbuffer in case it has writes to our query BO.
    * Have later queries write to a new query BO so that further rendering
    * doesn't delay the collection of our results.
    */
   if (query->bo) {
      brw_emit_query_end(brw);
      brw_context_flush( brw );

      bo_reference(&brw->query.bo, NULL);
   }

   remove_from_list(query);
   brw->query.stats_wm--;
   brw->state.dirty.mesa |= PIPE_NEW_QUERY;
}

/***********************************************************************
 * Internal functions and callbacks to implement queries 
 */

/** Called to set up the query BO and account for its aperture space */
enum pipe_error
brw_prepare_query_begin(struct brw_context *brw)
{
   enum pipe_error ret;

   /* Skip if we're not doing any queries. */
   if (is_empty_list(&brw->query.active_head))
      return PIPE_OK;

   /* Get a new query BO if we're going to need it. */
   if (brw->query.bo == NULL ||
       brw->query.index * 2 + 1 >= 4096 / sizeof(uint64_t)) {

      ret = brw->sws->bo_alloc(brw->sws, BRW_BUFFER_TYPE_QUERY, 4096, 1,
                               &brw->query.bo);
      if (ret)
         return ret;

      brw->query.index = 0;
   }

   brw_add_validated_bo(brw, brw->query.bo);

   return PIPE_OK;
}

/** Called just before primitive drawing to get a beginning PS_DEPTH_COUNT. */
void
brw_emit_query_begin(struct brw_context *brw)
{
   struct brw_query_object *query;

   /* Skip if we're not doing any queries, or we've emitted the start. */
   if (brw->query.active || is_empty_list(&brw->query.active_head))
      return;

   BEGIN_BATCH(4, IGNORE_CLIPRECTS);
   OUT_BATCH(_3DSTATE_PIPE_CONTROL |
	     PIPE_CONTROL_DEPTH_STALL |
	     PIPE_CONTROL_WRITE_DEPTH_COUNT);
   /* This object could be mapped cacheable, but we don't have an exposed
    * mechanism to support that.  Since it's going uncached, tell GEM that
    * we're writing to it.  The usual clflush should be all that's required
    * to pick up the results.
    */
   OUT_RELOC(brw->query.bo,
	     BRW_USAGE_QUERY_RESULT,
	     PIPE_CONTROL_GLOBAL_GTT_WRITE |
	     ((brw->query.index * 2) * sizeof(uint64_t)));
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();

   foreach(query, &brw->query.active_head) {
      if (query->bo != brw->query.bo) {
	 uint64_t tmp;
	 
	 /* Propogate the results from this buffer to all of the
	  * active queries, as the bo is going away.
	  */
	 if (query->bo != NULL)
	    brw_query_get_result( &brw->base, 
				  (struct pipe_query *)query,
				  FALSE,
				  &tmp );

	 bo_reference( &query->bo, brw->query.bo );
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
   if (!brw->query.active)
      return;

   BEGIN_BATCH(4, IGNORE_CLIPRECTS);
   OUT_BATCH(_3DSTATE_PIPE_CONTROL |
	     PIPE_CONTROL_DEPTH_STALL |
	     PIPE_CONTROL_WRITE_DEPTH_COUNT);
   OUT_RELOC(brw->query.bo,
	     BRW_USAGE_QUERY_RESULT,
	     PIPE_CONTROL_GLOBAL_GTT_WRITE |
	     ((brw->query.index * 2 + 1) * sizeof(uint64_t)));
   OUT_BATCH(0);
   OUT_BATCH(0);
   ADVANCE_BATCH();

   brw->query.active = GL_FALSE;
   brw->query.index++;
}

void brw_pipe_query_init( struct brw_context *brw )
{
   brw->base.create_query = brw_query_create;
   brw->base.destroy_query = brw_query_destroy;
   brw->base.begin_query = brw_query_begin;
   brw->base.end_query = brw_query_end;
   brw->base.get_query_result = brw_query_get_result;
}


void brw_pipe_query_cleanup( struct brw_context *brw )
{
   /* Unreference brw->query.bo ??
    */
}
