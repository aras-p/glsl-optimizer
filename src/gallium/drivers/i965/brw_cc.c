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


#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"

static void prepare_cc_vp( struct brw_context *brw )
{
   struct brw_cc_viewport ccv;

   memset(&ccv, 0, sizeof(ccv));

   /* _NEW_VIEWPORT */
   ccv.min_depth = ctx->Viewport.Near;
   ccv.max_depth = ctx->Viewport.Far;

   brw->sws->bo_unreference(brw->cc.vp_bo);
   brw->cc.vp_bo = brw_cache_data( &brw->cache, BRW_CC_VP, &ccv, NULL, 0 );
}

const struct brw_tracked_state brw_cc_vp = {
   .dirty = {
      .mesa = PIPE_NEW_VIEWPORT,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .prepare = prepare_cc_vp
};

struct brw_cc_unit_key {
   struct pipe_depth_stencil_alpha_state dsa;
   struct pipe_blend_state blend; /* no color mask */
};

static void
cc_unit_populate_key(struct brw_context *brw, struct brw_cc_unit_key *key)
{
   memset(key, 0, sizeof(*key));
   
   key->dsa = brw->dsa;
   key->blend = brw->blend;

   /* Clear non-respected values:
    */
   key->blend.colormask = 0xf;
}

/**
 * Creates the state cache entry for the given CC unit key.
 */
static struct brw_winsys_buffer *
cc_unit_create_from_key(struct brw_context *brw, struct brw_cc_unit_key *key)
{
   struct brw_cc_unit_state cc;
   struct brw_winsys_buffer *bo;

   memset(&cc, 0, sizeof(cc));

   cc.cc0 = brw->dsa.cc0;
   cc.cc1 = brw->dsa.cc1;
   cc.cc2 = brw->dsa.cc2;
   cc.cc3 = brw->dsa.cc3 | brw->blend.cc3;

   /* CACHE_NEW_CC_VP */
   cc.cc4.cc_viewport_state_offset = brw->cc.vp_bo->offset >> 5; /* reloc */

   cc.cc5 = brw->blend.cc5 | brw->debug.cc5;


   bo = brw_upload_cache(&brw->cache, BRW_CC_UNIT,
			 key, sizeof(*key),
			 &brw->cc.vp_bo, 1,
			 &cc, sizeof(cc),
			 NULL, NULL);

   /* Emit CC viewport relocation */
   dri_bo_emit_reloc(bo,
		     I915_GEM_DOMAIN_INSTRUCTION,
		     0,
		     0,
		     offsetof(struct brw_cc_unit_state, cc4),
		     brw->cc.vp_bo);

   return bo;
}

static void prepare_cc_unit( struct brw_context *brw )
{
   struct brw_cc_unit_key key;

   cc_unit_populate_key(brw, &key);

   brw->sws->bo_unreference(brw->cc.state_bo);
   brw->cc.state_bo = brw_search_cache(&brw->cache, BRW_CC_UNIT,
				       &key, sizeof(key),
				       &brw->cc.vp_bo, 1,
				       NULL);

   if (brw->cc.state_bo == NULL)
      brw->cc.state_bo = cc_unit_create_from_key(brw, &key);
}

const struct brw_tracked_state brw_cc_unit = {
   .dirty = {
      .mesa = PIPE_NEW_DEPTH_STENCIL_ALPHA | PIPE_NEW_BLEND,
      .brw = 0,
      .cache = CACHE_NEW_CC_VP
   },
   .prepare = prepare_cc_unit,
};



