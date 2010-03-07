
#include "util/u_math.h"
#include "util/u_memory.h"

#include "brw_context.h"
#include "brw_defines.h"

/* XXX: Fixme - include this to get IZ_ defines
 */
#include "brw_wm.h"

static unsigned brw_translate_compare_func(unsigned func)
{
   switch (func) {
   case PIPE_FUNC_NEVER:
      return BRW_COMPAREFUNCTION_NEVER;
   case PIPE_FUNC_LESS:
      return BRW_COMPAREFUNCTION_LESS;
   case PIPE_FUNC_LEQUAL:
      return BRW_COMPAREFUNCTION_LEQUAL;
   case PIPE_FUNC_GREATER:
      return BRW_COMPAREFUNCTION_GREATER;
   case PIPE_FUNC_GEQUAL:
      return BRW_COMPAREFUNCTION_GEQUAL;
   case PIPE_FUNC_NOTEQUAL:
      return BRW_COMPAREFUNCTION_NOTEQUAL;
   case PIPE_FUNC_EQUAL:
      return BRW_COMPAREFUNCTION_EQUAL;
   case PIPE_FUNC_ALWAYS:
      return BRW_COMPAREFUNCTION_ALWAYS;
   default:
      assert(0);
      return BRW_COMPAREFUNCTION_ALWAYS;
   }
}

static unsigned translate_stencil_op(unsigned op)
{
   switch (op) {
   case PIPE_STENCIL_OP_KEEP:
      return BRW_STENCILOP_KEEP;
   case PIPE_STENCIL_OP_ZERO:
      return BRW_STENCILOP_ZERO;
   case PIPE_STENCIL_OP_REPLACE:
      return BRW_STENCILOP_REPLACE;
   case PIPE_STENCIL_OP_INCR:
      return BRW_STENCILOP_INCRSAT;
   case PIPE_STENCIL_OP_DECR:
      return BRW_STENCILOP_DECRSAT;
   case PIPE_STENCIL_OP_INCR_WRAP:
      return BRW_STENCILOP_INCR;
   case PIPE_STENCIL_OP_DECR_WRAP:
      return BRW_STENCILOP_DECR;
   case PIPE_STENCIL_OP_INVERT:
      return BRW_STENCILOP_INVERT;
   default:
      assert(0);
      return BRW_STENCILOP_ZERO;
   }
}

static void create_bcc_state( struct brw_depth_stencil_state *zstencil,
			      const struct pipe_depth_stencil_alpha_state *templ )
{
   if (templ->stencil[0].enabled) {
      zstencil->cc0.stencil_enable = 1;
      zstencil->cc0.stencil_func =
	 brw_translate_compare_func(templ->stencil[0].func);
      zstencil->cc0.stencil_fail_op =
	 translate_stencil_op(templ->stencil[0].fail_op);
      zstencil->cc0.stencil_pass_depth_fail_op =
	 translate_stencil_op(templ->stencil[0].zfail_op);
      zstencil->cc0.stencil_pass_depth_pass_op =
	 translate_stencil_op(templ->stencil[0].zpass_op);
      zstencil->cc1.stencil_write_mask = templ->stencil[0].writemask;
      zstencil->cc1.stencil_test_mask = templ->stencil[0].valuemask;

      if (templ->stencil[1].enabled) {
	 zstencil->cc0.bf_stencil_enable = 1;
	 zstencil->cc0.bf_stencil_func =
	    brw_translate_compare_func(templ->stencil[1].func);
	 zstencil->cc0.bf_stencil_fail_op =
	    translate_stencil_op(templ->stencil[1].fail_op);
	 zstencil->cc0.bf_stencil_pass_depth_fail_op =
	    translate_stencil_op(templ->stencil[1].zfail_op);
	 zstencil->cc0.bf_stencil_pass_depth_pass_op =
	    translate_stencil_op(templ->stencil[1].zpass_op);
	 zstencil->cc2.bf_stencil_write_mask = templ->stencil[1].writemask;
	 zstencil->cc2.bf_stencil_test_mask = templ->stencil[1].valuemask;
      }

      zstencil->cc0.stencil_write_enable = (zstencil->cc1.stencil_write_mask ||
					    zstencil->cc2.bf_stencil_write_mask);
   }


   if (templ->alpha.enabled) {
      zstencil->cc3.alpha_test = 1;
      zstencil->cc3.alpha_test_func = brw_translate_compare_func(templ->alpha.func);
      zstencil->cc3.alpha_test_format = BRW_ALPHATEST_FORMAT_UNORM8;
      zstencil->cc7.alpha_ref.ub[0] = float_to_ubyte(templ->alpha.ref_value);
   }

   if (templ->depth.enabled) {
      zstencil->cc2.depth_test = 1;
      zstencil->cc2.depth_test_function = brw_translate_compare_func(templ->depth.func);
      zstencil->cc2.depth_write_enable = templ->depth.writemask;
   }
}

static void create_wm_iz_state( struct brw_depth_stencil_state *zstencil )
{
   if (zstencil->cc3.alpha_test)
      zstencil->iz_lookup |= IZ_PS_KILL_ALPHATEST_BIT;

   if (zstencil->cc2.depth_test)
      zstencil->iz_lookup |= IZ_DEPTH_TEST_ENABLE_BIT;

   if (zstencil->cc2.depth_write_enable)
      zstencil->iz_lookup |= IZ_DEPTH_WRITE_ENABLE_BIT;

   if (zstencil->cc0.stencil_enable)
      zstencil->iz_lookup |= IZ_STENCIL_TEST_ENABLE_BIT;

   if (zstencil->cc0.stencil_write_enable)
      zstencil->iz_lookup |= IZ_STENCIL_WRITE_ENABLE_BIT;

}


static void *
brw_create_depth_stencil_state( struct pipe_context *pipe,
				const struct pipe_depth_stencil_alpha_state *templ )
{
   struct brw_depth_stencil_state *zstencil = CALLOC_STRUCT(brw_depth_stencil_state);

   create_bcc_state( zstencil, templ );
   create_wm_iz_state( zstencil );

   return (void *)zstencil;
}


static void brw_bind_depth_stencil_state(struct pipe_context *pipe,
					 void *cso)
{
   struct brw_context *brw = brw_context(pipe);
   brw->curr.zstencil = (const struct brw_depth_stencil_state *)cso;
   brw->state.dirty.mesa |= PIPE_NEW_DEPTH_STENCIL_ALPHA;
}

static void brw_delete_depth_stencil_state(struct pipe_context *pipe,
					   void *cso)
{
   struct brw_context *brw = brw_context(pipe);
   assert((const void *)cso != (const void *)brw->curr.zstencil);
   FREE(cso);
}

static void brw_set_stencil_ref(struct pipe_context *pipe,
                                const struct pipe_stencil_ref *stencil_ref)
{
   struct brw_context *brw = brw_context(pipe);
   brw->curr.cc1_stencil_ref.stencil_ref = stencil_ref->ref_value[0];
   brw->curr.cc1_stencil_ref.bf_stencil_ref = stencil_ref->ref_value[1];

   brw->state.dirty.mesa |= PIPE_NEW_DEPTH_STENCIL_ALPHA;
}

void brw_pipe_depth_stencil_init( struct brw_context *brw )
{
   brw->base.set_stencil_ref = brw_set_stencil_ref;
   brw->base.create_depth_stencil_alpha_state = brw_create_depth_stencil_state;
   brw->base.bind_depth_stencil_alpha_state = brw_bind_depth_stencil_state;
   brw->base.delete_depth_stencil_alpha_state = brw_delete_depth_stencil_state;
}

void brw_pipe_depth_stencil_cleanup( struct brw_context *brw )
{
}
