/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
            



#include "glheader.h"
#include "mtypes.h"
#include "imports.h"
#include "macros.h"
#include "colormac.h"

#include "intel_batchbuffer.h" 
#include "intel_regions.h" 

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_state.h"

#include "brw_draw.h"
#include "brw_state.h"
#include "brw_fallback.h"
#include "brw_vs.h"
#include <stdarg.h>


/* called from intelDestroyContext()
 */
static void brw_destroy_context( struct intel_context *intel )
{
   GLcontext *ctx = &intel->ctx;
   struct brw_context *brw = brw_context(&intel->ctx);

   brw_destroy_metaops(brw);
   brw_destroy_state(brw);
   brw_draw_destroy( brw );

   brw_ProgramCacheDestroy( ctx );
   brw_FrameBufferTexDestroy( brw );
}

/* called from intelDrawBuffer()
 */
static void brw_set_draw_region( struct intel_context *intel, 
				  struct intel_region *draw_regions[],
				  struct intel_region *depth_region,
				GLuint num_regions)
{
   struct brw_context *brw = brw_context(&intel->ctx);
   int i;
   if (brw->state.depth_region != depth_region)
      brw->state.dirty.brw |= BRW_NEW_DEPTH_BUFFER;
   for (i = 0; i < brw->state.nr_draw_regions; i++)
       intel_region_release(&brw->state.draw_regions[i]);
   intel_region_release(&brw->state.depth_region);
   for (i = 0; i < num_regions; i++)
       intel_region_reference(&brw->state.draw_regions[i], draw_regions[i]);
   intel_region_reference(&brw->state.depth_region, depth_region);
   brw->state.nr_draw_regions = num_regions;
}


/* called from intelFlushBatchLocked
 */
static void brw_new_batch( struct intel_context *intel )
{
   struct brw_context *brw = brw_context(&intel->ctx);

   /* Check that we didn't just wrap our batchbuffer at a bad time. */
   assert(!brw->no_batch_wrap);

   dri_bo_unreference(brw->curbe.curbe_bo);
   brw->curbe.curbe_bo = NULL;

   /* Mark all context state as needing to be re-emitted.
    * This is probably not as severe as on 915, since almost all of our state
    * is just in referenced buffers.
    */
   brw->state.dirty.brw |= BRW_NEW_CONTEXT;

   brw->state.dirty.mesa |= ~0;
   brw->state.dirty.brw |= ~0;
   brw->state.dirty.cache |= ~0;

   /* Move to the end of the current upload buffer so that we'll force choosing
    * a new buffer next time.
    */
   if (brw->vb.upload.bo != NULL) {
      dri_bo_unreference(brw->vb.upload.bo);
      brw->vb.upload.bo = NULL;
      brw->vb.upload.offset = 0;
   }
}

static void brw_note_fence( struct intel_context *intel, 
			    GLuint fence )
{
   brw_context(&intel->ctx)->state.dirty.brw |= BRW_NEW_FENCE;
}
 
static void brw_note_unlock( struct intel_context *intel )
{
   struct brw_context *brw = brw_context(&intel->ctx);

   brw_state_cache_check_size(brw);

   brw_context(&intel->ctx)->state.dirty.brw |= BRW_NEW_LOCK;
}


void brw_do_flush( struct brw_context *brw, 
		   GLuint flags )
{
   struct brw_mi_flush flush;
   memset(&flush, 0, sizeof(flush));      
   flush.opcode = CMD_MI_FLUSH;
   flush.flags = flags;
   BRW_BATCH_STRUCT(brw, &flush);
}


static void brw_emit_flush( struct intel_context *intel,
			GLuint unused )
{
   brw_do_flush(brw_context(&intel->ctx),
		BRW_FLUSH_STATE_CACHE|BRW_FLUSH_READ_CACHE);
}


/* called from intelWaitForIdle() and intelFlush()
 *
 * For now, just flush everything.  Could be smarter later.
 */
static GLuint brw_flush_cmd( void )
{
   struct brw_mi_flush flush;
   flush.opcode = CMD_MI_FLUSH;
   flush.pad = 0;
   flush.flags = BRW_FLUSH_READ_CACHE | BRW_FLUSH_STATE_CACHE;
   return *(GLuint *)&flush;
}

static void brw_invalidate_state( struct intel_context *intel, GLuint new_state )
{
   /* nothing */
}


void brwInitVtbl( struct brw_context *brw )
{
   brw->intel.vtbl.check_vertex_size = 0;
   brw->intel.vtbl.emit_state = 0; 
   brw->intel.vtbl.reduced_primitive_state = 0;	
   brw->intel.vtbl.render_start = 0;
   brw->intel.vtbl.update_texture_state = 0; 

   brw->intel.vtbl.invalidate_state = brw_invalidate_state; 
   brw->intel.vtbl.note_fence = brw_note_fence; 
   brw->intel.vtbl.note_unlock = brw_note_unlock; 
   brw->intel.vtbl.new_batch = brw_new_batch;
   brw->intel.vtbl.destroy = brw_destroy_context;
   brw->intel.vtbl.set_draw_region = brw_set_draw_region;
   brw->intel.vtbl.flush_cmd = brw_flush_cmd;
   brw->intel.vtbl.emit_flush = brw_emit_flush;
   brw->intel.vtbl.debug_batch = brw_debug_batch;
}

