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

#include "util/u_math.h"
#include "util/u_memory.h"

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"
#include "brw_util.h"


static int brw_translate_compare_func(int func)
{
   switch(func) {
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
   }

   debug_printf("Unknown value in %s: %x\n", __FUNCTION__, func);
   return BRW_COMPAREFUNCTION_ALWAYS;
}

static int brw_translate_stencil_op(int op)
{
   switch(op) {
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
      return BRW_STENCILOP_ZERO;
   }
}


static int brw_translate_logic_op(int opcode)
{
   switch(opcode) {
   case PIPE_LOGICOP_CLEAR:
      return BRW_LOGICOPFUNCTION_CLEAR;
   case PIPE_LOGICOP_AND:
      return BRW_LOGICOPFUNCTION_AND;
   case PIPE_LOGICOP_AND_REVERSE:
      return BRW_LOGICOPFUNCTION_AND_REVERSE;
   case PIPE_LOGICOP_COPY:
      return BRW_LOGICOPFUNCTION_COPY;
   case PIPE_LOGICOP_COPY_INVERTED:
      return BRW_LOGICOPFUNCTION_COPY_INVERTED;
   case PIPE_LOGICOP_AND_INVERTED:
      return BRW_LOGICOPFUNCTION_AND_INVERTED;
   case PIPE_LOGICOP_NOOP:
      return BRW_LOGICOPFUNCTION_NOOP;
   case PIPE_LOGICOP_XOR:
      return BRW_LOGICOPFUNCTION_XOR;
   case PIPE_LOGICOP_OR:
      return BRW_LOGICOPFUNCTION_OR;
   case PIPE_LOGICOP_OR_INVERTED:
      return BRW_LOGICOPFUNCTION_OR_INVERTED;
   case PIPE_LOGICOP_NOR:
      return BRW_LOGICOPFUNCTION_NOR;
   case PIPE_LOGICOP_EQUIV:
      return BRW_LOGICOPFUNCTION_EQUIV;
   case PIPE_LOGICOP_INVERT:
      return BRW_LOGICOPFUNCTION_INVERT;
   case PIPE_LOGICOP_OR_REVERSE:
      return BRW_LOGICOPFUNCTION_OR_REVERSE;
   case PIPE_LOGICOP_NAND:
      return BRW_LOGICOPFUNCTION_NAND;
   case PIPE_LOGICOP_SET:
      return BRW_LOGICOPFUNCTION_SET;
   default:
      return BRW_LOGICOPFUNCTION_SET;
   }
}


static void upload_cc_vp( struct brw_context *brw )
{
   struct brw_cc_viewport ccv;

   memset(&ccv, 0, sizeof(ccv));

   ccv.min_depth = 0.0;
   ccv.max_depth = 1.0;

   brw->cc.vp_gs_offset = brw_cache_data( &brw->cache[BRW_CC_VP], &ccv );
}

const struct brw_tracked_state brw_cc_vp = {
   .dirty = {
      .brw = BRW_NEW_SCENE,
      .cache = 0
   },
   .update = upload_cc_vp
};


static void upload_cc_unit( struct brw_context *brw )
{
   struct brw_cc_unit_state cc;

   memset(&cc, 0, sizeof(cc));

   /* BRW_NEW_DEPTH_STENCIL */
   if (brw->attribs.DepthStencil->stencil[0].enabled) {
      cc.cc0.stencil_enable = brw->attribs.DepthStencil->stencil[0].enabled;
      cc.cc0.stencil_func = brw_translate_compare_func(brw->attribs.DepthStencil->stencil[0].func);
      cc.cc0.stencil_fail_op = brw_translate_stencil_op(brw->attribs.DepthStencil->stencil[0].fail_op);
      cc.cc0.stencil_pass_depth_fail_op = brw_translate_stencil_op(
         brw->attribs.DepthStencil->stencil[0].zfail_op);
      cc.cc0.stencil_pass_depth_pass_op = brw_translate_stencil_op(
         brw->attribs.DepthStencil->stencil[0].zpass_op);
      cc.cc1.stencil_ref = brw->attribs.DepthStencil->stencil[0].ref_value;
      cc.cc1.stencil_write_mask = brw->attribs.DepthStencil->stencil[0].write_mask;
      cc.cc1.stencil_test_mask = brw->attribs.DepthStencil->stencil[0].value_mask;

      if (brw->attribs.DepthStencil->stencil[1].enabled) {
	 cc.cc0.bf_stencil_enable = brw->attribs.DepthStencil->stencil[1].enabled;
	 cc.cc0.bf_stencil_func = brw_translate_compare_func(
            brw->attribs.DepthStencil->stencil[1].func);
	 cc.cc0.bf_stencil_fail_op = brw_translate_stencil_op(
            brw->attribs.DepthStencil->stencil[1].fail_op);
	 cc.cc0.bf_stencil_pass_depth_fail_op = brw_translate_stencil_op(
            brw->attribs.DepthStencil->stencil[1].zfail_op);
	 cc.cc0.bf_stencil_pass_depth_pass_op = brw_translate_stencil_op(
            brw->attribs.DepthStencil->stencil[1].zpass_op);
	 cc.cc1.bf_stencil_ref = brw->attribs.DepthStencil->stencil[1].ref_value;
	 cc.cc2.bf_stencil_write_mask = brw->attribs.DepthStencil->stencil[1].write_mask;
	 cc.cc2.bf_stencil_test_mask = brw->attribs.DepthStencil->stencil[1].value_mask;
      }

      /* Not really sure about this:
       */
      if (brw->attribs.DepthStencil->stencil[0].write_mask ||
	  brw->attribs.DepthStencil->stencil[1].write_mask)
	 cc.cc0.stencil_write_enable = 1;
   }

   /* BRW_NEW_BLEND */
   if (brw->attribs.Blend->logicop_enable) {
      cc.cc2.logicop_enable = 1;
      cc.cc5.logicop_func = brw_translate_logic_op( brw->attribs.Blend->logicop_func );
   }
   else if (brw->attribs.Blend->blend_enable) {
      int eqRGB = brw->attribs.Blend->rgb_func;
      int eqA = brw->attribs.Blend->alpha_func;
      int srcRGB = brw->attribs.Blend->rgb_src_factor;
      int dstRGB = brw->attribs.Blend->rgb_dst_factor;
      int srcA = brw->attribs.Blend->alpha_src_factor;
      int dstA = brw->attribs.Blend->alpha_dst_factor;

      if (eqRGB == PIPE_BLEND_MIN || eqRGB == PIPE_BLEND_MAX) {
	 srcRGB = dstRGB = PIPE_BLENDFACTOR_ONE;
      }

      if (eqA == PIPE_BLEND_MIN || eqA == PIPE_BLEND_MAX) {
	 srcA = dstA = PIPE_BLENDFACTOR_ONE;
      }

      cc.cc6.dest_blend_factor = brw_translate_blend_factor(dstRGB);
      cc.cc6.src_blend_factor = brw_translate_blend_factor(srcRGB);
      cc.cc6.blend_function = brw_translate_blend_equation( eqRGB );

      cc.cc5.ia_dest_blend_factor = brw_translate_blend_factor(dstA);
      cc.cc5.ia_src_blend_factor = brw_translate_blend_factor(srcA);
      cc.cc5.ia_blend_function = brw_translate_blend_equation( eqA );

      cc.cc3.blend_enable = 1;
      cc.cc3.ia_blend_enable = (srcA != srcRGB ||
				dstA != dstRGB ||
				eqA != eqRGB);
   }
   
   /* BRW_NEW_ALPHATEST
    */
   if (brw->attribs.DepthStencil->alpha.enabled) {
      cc.cc3.alpha_test = 1;
      cc.cc3.alpha_test_func = 
	 brw_translate_compare_func(brw->attribs.DepthStencil->alpha.func);

      cc.cc7.alpha_ref.ub[0] = float_to_ubyte(brw->attribs.DepthStencil->alpha.ref);

      cc.cc3.alpha_test_format = BRW_ALPHATEST_FORMAT_UNORM8;
   }

   if (brw->attribs.Blend->dither) {
      cc.cc5.dither_enable = 1;
      cc.cc6.y_dither_offset = 0;
      cc.cc6.x_dither_offset = 0;
   }

   if (brw->attribs.DepthStencil->depth.enabled) {
      cc.cc2.depth_test = brw->attribs.DepthStencil->depth.enabled;
      cc.cc2.depth_test_function = brw_translate_compare_func(brw->attribs.DepthStencil->depth.func);
      cc.cc2.depth_write_enable = brw->attribs.DepthStencil->depth.writemask;
   }

   /* CACHE_NEW_CC_VP */
   cc.cc4.cc_viewport_state_offset =  brw->cc.vp_gs_offset >> 5;

   if (BRW_DEBUG & DEBUG_STATS)
      cc.cc5.statistics_enable = 1;

   brw->cc.state_gs_offset = brw_cache_data( &brw->cache[BRW_CC_UNIT], &cc );
}

const struct brw_tracked_state brw_cc_unit = {
   .dirty = {
      .brw = BRW_NEW_DEPTH_STENCIL | BRW_NEW_BLEND | BRW_NEW_ALPHA_TEST,
      .cache = CACHE_NEW_CC_VP
   },
   .update = upload_cc_unit
};

