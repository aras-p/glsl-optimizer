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


static enum pipe_error prepare_cc_vp( struct brw_context *brw )
{
   return brw_cache_data( &brw->cache, 
                         BRW_CC_VP,
                         &brw->curr.ccv,
                         NULL, 0,
                         &brw->cc.reloc[CC_RELOC_VP].bo );
}

const struct brw_tracked_state brw_cc_vp = {
   .dirty = {
      .mesa = PIPE_NEW_VIEWPORT,
      .brw = BRW_NEW_CONTEXT,
      .cache = 0
   },
   .prepare = prepare_cc_vp
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

static INLINE struct brw_cc1
combine_cc1( struct brw_cc1 a, struct brw_cc1 b )
{
   union { struct brw_cc1 cc1; unsigned i; } ca, cb;
   ca.cc1 = a;
   cb.cc1 = b;
   ca.i |= cb.i;
   return ca.cc1;
}

static INLINE struct brw_cc2
combine_cc2( struct brw_cc2 a, struct brw_cc2 b )
{
   union { struct brw_cc2 cc2; unsigned i; } ca, cb;
   ca.cc2 = a;
   cb.cc2 = b;
   ca.i |= cb.i;
   return ca.cc2;
}

static int prepare_cc_unit( struct brw_context *brw )
{
   brw->cc.cc.cc0 = brw->curr.zstencil->cc0;
   brw->cc.cc.cc1 = combine_cc1( brw->curr.zstencil->cc1, brw->curr.cc1_stencil_ref );
   brw->cc.cc.cc2 = combine_cc2( brw->curr.zstencil->cc2, brw->curr.blend->cc2 );
   brw->cc.cc.cc3 = combine_cc3( brw->curr.zstencil->cc3, brw->curr.blend->cc3 );

   brw->cc.cc.cc5 = brw->curr.blend->cc5;
   brw->cc.cc.cc6 = brw->curr.blend->cc6;
   brw->cc.cc.cc7 = brw->curr.zstencil->cc7;

   return brw_cache_data_sz(&brw->cache, BRW_CC_UNIT,
                           &brw->cc.cc, sizeof(brw->cc.cc),
                           brw->cc.reloc, 1,
                           &brw->cc.state_bo);
}

const struct brw_tracked_state brw_cc_unit = {
   .dirty = {
      .mesa = PIPE_NEW_DEPTH_STENCIL_ALPHA | PIPE_NEW_BLEND,
      .brw = 0,
      .cache = CACHE_NEW_CC_VP
   },
   .prepare = prepare_cc_unit,
};


void brw_hw_cc_init( struct brw_context *brw )
{
   make_reloc(&brw->cc.reloc[0],
              BRW_USAGE_STATE,
              0,
              offsetof(struct brw_cc_unit_state, cc4),
              NULL);
}


void brw_hw_cc_cleanup( struct brw_context *brw )
{
   bo_reference(&brw->cc.state_bo, NULL);
   bo_reference(&brw->cc.reloc[0].bo, NULL);
}
