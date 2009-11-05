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


struct sane_viewport {
   float top;
   float left;
   float width;
   float height;
   float near;
   float far;
};

static void calc_sane_viewport( const struct pipe_viewport_state *vp,
				struct sane_viewport *svp )
{
   /* XXX fix me, obviously.
    */
   svp->top = 0;
   svp->left = 0;
   svp->width = 250;
   svp->height = 250;
   svp->near = 0;
   svp->far = 1;
}

static enum pipe_error prepare_cc_vp( struct brw_context *brw )
{
   struct brw_cc_viewport ccv;
   struct sane_viewport svp;
   enum pipe_error ret;

   memset(&ccv, 0, sizeof(ccv));

   /* PIPE_NEW_VIEWPORT */
   calc_sane_viewport( &brw->curr.vp, &svp );

   ccv.min_depth = svp.near;
   ccv.max_depth = svp.far;

   ret = brw_cache_data( &brw->cache, BRW_CC_VP, &ccv, NULL, 0,
                         &brw->cc.vp_bo );
   if (ret)
      return ret;
                
   return PIPE_OK;
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
   struct brw_cc0 cc0;
   struct brw_cc1 cc1;
   struct brw_cc2 cc2;
   struct brw_cc3 cc3;
   struct brw_cc5 cc5;
   struct brw_cc6 cc6;
   struct brw_cc7 cc7;
};

/* A long-winded way to OR two unsigned integers together:
 */
static INLINE struct brw_cc3
combine_cc3( struct brw_cc3 a, struct brw_cc3 b )
{
   union { struct brw_cc3 cc3; unsigned i; } ca, cb;
   ca.cc3 = a;
   cb.cc3 = b;
   ca.i |= cb.i;
   return ca.cc3;
}

static void
cc_unit_populate_key(const struct brw_context *brw,
		     struct brw_cc_unit_key *key)
{
   key->cc0 = brw->curr.zstencil->cc0;
   key->cc1 = brw->curr.zstencil->cc1;
   key->cc2 = brw->curr.zstencil->cc2;
   key->cc3 = combine_cc3( brw->curr.zstencil->cc3, brw->curr.blend->cc3 );
   key->cc5 = brw->curr.blend->cc5;
   key->cc6 = brw->curr.blend->cc6;
   key->cc7 = brw->curr.zstencil->cc7;
}

/**
 * Creates the state cache entry for the given CC unit key.
 */
static enum pipe_error
cc_unit_create_from_key(struct brw_context *brw, 
                        struct brw_cc_unit_key *key,
                        struct brw_winsys_buffer **bo_out)
{
   struct brw_cc_unit_state cc;
   enum pipe_error ret;

   memset(&cc, 0, sizeof(cc));

   cc.cc0 = key->cc0;
   cc.cc1 = key->cc1;
   cc.cc2 = key->cc2;
   cc.cc3 = key->cc3;

   /* CACHE_NEW_CC_VP */
   cc.cc4.cc_viewport_state_offset = *(brw->cc.vp_bo->offset) >> 5; /* reloc */

   cc.cc5 = key->cc5;
   cc.cc6 = key->cc6;
   cc.cc7 = key->cc7;

   ret = brw_upload_cache(&brw->cache, BRW_CC_UNIT,
                          key, sizeof(*key),
                          &brw->cc.vp_bo, 1,
                          &cc, sizeof(cc),
                          NULL, NULL,
                          bo_out);
   if (ret)
      return ret;


   /* Emit CC viewport relocation */
   ret = brw->sws->bo_emit_reloc(*bo_out,
                                 BRW_USAGE_STATE,
                                 0,
                                 offsetof(struct brw_cc_unit_state, cc4),
                                 brw->cc.vp_bo);
   if (ret)
      return ret;

   return PIPE_OK;
}

static int prepare_cc_unit( struct brw_context *brw )
{
   struct brw_cc_unit_key key;
   enum pipe_error ret;

   cc_unit_populate_key(brw, &key);

   if (brw_search_cache(&brw->cache, BRW_CC_UNIT,
                        &key, sizeof(key),
                        &brw->cc.vp_bo, 1,
                        NULL,
                        &brw->cc.state_bo))
      return PIPE_OK;

   ret = cc_unit_create_from_key(brw, &key, 
                                 &brw->cc.state_bo);
   if (ret)
      return ret;
   
   return PIPE_OK;
}

const struct brw_tracked_state brw_cc_unit = {
   .dirty = {
      .mesa = PIPE_NEW_DEPTH_STENCIL_ALPHA | PIPE_NEW_BLEND,
      .brw = 0,
      .cache = CACHE_NEW_CC_VP
   },
   .prepare = prepare_cc_unit,
};



