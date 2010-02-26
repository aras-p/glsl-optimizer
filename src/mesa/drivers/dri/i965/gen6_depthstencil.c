/*
 * Copyright © 2009 Intel Corporation
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

#include "brw_context.h"
#include "brw_state.h"

struct brw_depth_stencil_state_key {
   GLenum depth_func;
   GLboolean depth_test, depth_write;
   GLboolean stencil, stencil_two_side;
   GLenum stencil_func[2], stencil_fail_op[2];
   GLenum stencil_pass_depth_fail_op[2], stencil_pass_depth_pass_op[2];
   GLubyte stencil_write_mask[2], stencil_test_mask[2];
};

static void
depth_stencil_state_populate_key(struct brw_context *brw,
				 struct brw_depth_stencil_state_key *key)
{
   GLcontext *ctx = &brw->intel.ctx;
   const unsigned back = ctx->Stencil._BackFace;

   memset(key, 0, sizeof(*key));

   /* _NEW_STENCIL */
   key->stencil = ctx->Stencil._Enabled;
   key->stencil_two_side = ctx->Stencil._TestTwoSide;

   if (key->stencil) {
      key->stencil_func[0] = ctx->Stencil.Function[0];
      key->stencil_fail_op[0] = ctx->Stencil.FailFunc[0];
      key->stencil_pass_depth_fail_op[0] = ctx->Stencil.ZFailFunc[0];
      key->stencil_pass_depth_pass_op[0] = ctx->Stencil.ZPassFunc[0];
      key->stencil_write_mask[0] = ctx->Stencil.WriteMask[0];
      key->stencil_test_mask[0] = ctx->Stencil.ValueMask[0];
   }
   if (key->stencil_two_side) {
      key->stencil_func[1] = ctx->Stencil.Function[back];
      key->stencil_fail_op[1] = ctx->Stencil.FailFunc[back];
      key->stencil_pass_depth_fail_op[1] = ctx->Stencil.ZFailFunc[back];
      key->stencil_pass_depth_pass_op[1] = ctx->Stencil.ZPassFunc[back];
      key->stencil_write_mask[1] = ctx->Stencil.WriteMask[back];
      key->stencil_test_mask[1] = ctx->Stencil.ValueMask[back];
   }

   key->depth_test = ctx->Depth.Test;
   if (key->depth_test) {
      key->depth_func = ctx->Depth.Func;
      key->depth_write = ctx->Depth.Mask;
   }
}

/**
 * Creates the state cache entry for the given DEPTH_STENCIL_STATE state key.
 */
static dri_bo *
depth_stencil_state_create_from_key(struct brw_context *brw,
				    struct brw_depth_stencil_state_key *key)
{
   struct gen6_depth_stencil_state ds;
   dri_bo *bo;

   memset(&ds, 0, sizeof(ds));

   /* _NEW_STENCIL */
   if (key->stencil) {
      ds.ds0.stencil_enable = 1;
      ds.ds0.stencil_func =
	 intel_translate_compare_func(key->stencil_func[0]);
      ds.ds0.stencil_fail_op =
	 intel_translate_stencil_op(key->stencil_fail_op[0]);
      ds.ds0.stencil_pass_depth_fail_op =
	 intel_translate_stencil_op(key->stencil_pass_depth_fail_op[0]);
      ds.ds0.stencil_pass_depth_pass_op =
	 intel_translate_stencil_op(key->stencil_pass_depth_pass_op[0]);
      ds.ds1.stencil_write_mask = key->stencil_write_mask[0];
      ds.ds1.stencil_test_mask = key->stencil_test_mask[0];

      if (key->stencil_two_side) {
	 ds.ds0.bf_stencil_enable = 1;
	 ds.ds0.bf_stencil_func =
	    intel_translate_compare_func(key->stencil_func[1]);
	 ds.ds0.bf_stencil_fail_op =
	    intel_translate_stencil_op(key->stencil_fail_op[1]);
	 ds.ds0.bf_stencil_pass_depth_fail_op =
	    intel_translate_stencil_op(key->stencil_pass_depth_fail_op[1]);
	 ds.ds0.bf_stencil_pass_depth_pass_op =
	    intel_translate_stencil_op(key->stencil_pass_depth_pass_op[1]);
	 ds.ds1.bf_stencil_write_mask = key->stencil_write_mask[1];
	 ds.ds1.bf_stencil_test_mask = key->stencil_test_mask[1];
      }

      /* Not really sure about this:
       */
      if (key->stencil_write_mask[0] ||
	  (key->stencil_two_side && key->stencil_write_mask[1]))
	 ds.ds0.stencil_write_enable = 1;
   }

   /* _NEW_DEPTH */
   if (key->depth_test) {
      ds.ds2.depth_test_enable = 1;
      ds.ds2.depth_test_func = intel_translate_compare_func(key->depth_func);
      ds.ds2.depth_write_enable = key->depth_write;
   }

   bo = brw_upload_cache(&brw->cache, BRW_DEPTH_STENCIL_STATE,
			 key, sizeof(*key),
			 NULL, 0,
			 &ds, sizeof(ds));

   return bo;
}

static void
prepare_depth_stencil_state(struct brw_context *brw)
{
   struct brw_depth_stencil_state_key key;

   depth_stencil_state_populate_key(brw, &key);

   dri_bo_unreference(brw->cc.depth_stencil_state_bo);
   brw->cc.depth_stencil_state_bo = brw_search_cache(&brw->cache,
						     BRW_DEPTH_STENCIL_STATE,
						     &key, sizeof(key),
						     NULL, 0,
						     NULL);

   if (brw->cc.depth_stencil_state_bo == NULL)
      brw->cc.depth_stencil_state_bo =
	 depth_stencil_state_create_from_key(brw, &key);
}

const struct brw_tracked_state gen6_depth_stencil_state = {
   .dirty = {
      .mesa = _NEW_DEPTH | _NEW_STENCIL,
      .brw = 0,
      .cache = 0,
   },
   .prepare = prepare_depth_stencil_state,
};
