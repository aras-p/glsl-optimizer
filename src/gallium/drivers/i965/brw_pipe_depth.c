
static void *
brw_create_depth_stencil( struct pipe_context *pipe,
			  const struct pipe_depth_stencil_alpha_state *tmpl )
{
   if (tmpl->stencil[0].enable) {
      cc.cc0.stencil_enable = 1;
      cc.cc0.stencil_func =
	 intel_translate_compare_func(key->stencil_func[0]);
      cc.cc0.stencil_fail_op =
	 intel_translate_stencil_op(key->stencil_fail_op[0]);
      cc.cc0.stencil_pass_depth_fail_op =
	 intel_translate_stencil_op(key->stencil_pass_depth_fail_op[0]);
      cc.cc0.stencil_pass_depth_pass_op =
	 intel_translate_stencil_op(key->stencil_pass_depth_pass_op[0]);
      cc.cc1.stencil_ref = key->stencil_ref[0];
      cc.cc1.stencil_write_mask = key->stencil_write_mask[0];
      cc.cc1.stencil_test_mask = key->stencil_test_mask[0];

      if (tmpl->stencil[1].enable) {
	 cc.cc0.bf_stencil_enable = 1;
	 cc.cc0.bf_stencil_func =
	    intel_translate_compare_func(key->stencil_func[1]);
	 cc.cc0.bf_stencil_fail_op =
	    intel_translate_stencil_op(key->stencil_fail_op[1]);
	 cc.cc0.bf_stencil_pass_depth_fail_op =
	    intel_translate_stencil_op(key->stencil_pass_depth_fail_op[1]);
	 cc.cc0.bf_stencil_pass_depth_pass_op =
	    intel_translate_stencil_op(key->stencil_pass_depth_pass_op[1]);
	 cc.cc1.bf_stencil_ref = key->stencil_ref[1];
	 cc.cc2.bf_stencil_write_mask = key->stencil_write_mask[1];
	 cc.cc2.bf_stencil_test_mask = key->stencil_test_mask[1];
      }

      /* Not really sure about this:
       */
      cc.cc0.stencil_write_enable = (cc.cc1.stencil_write_mask ||
				     cc.cc2.bf_stencil_write_mask);
   }


   if (key->alpha_enabled) {
      cc.cc3.alpha_test = 1;
      cc.cc3.alpha_test_func = intel_translate_compare_func(key->alpha_func);
      cc.cc3.alpha_test_format = BRW_ALPHATEST_FORMAT_UNORM8;

      UNCLAMPED_FLOAT_TO_UBYTE(cc.cc7.alpha_ref.ub[0], key->alpha_ref);
   }

   /* _NEW_DEPTH */
   if (key->depth_test) {
      cc.cc2.depth_test = 1;
      cc.cc2.depth_test_function = intel_translate_compare_func(key->depth_func);
      cc.cc2.depth_write_enable = key->depth_write;
   }


}
