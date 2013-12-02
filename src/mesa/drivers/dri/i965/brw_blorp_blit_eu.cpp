/*
 * Copyright © 2013 Intel Corporation
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
 */

#include "glsl/ralloc.h"
#include "brw_blorp_blit_eu.h"
#include "brw_blorp.h"

brw_blorp_eu_emitter::brw_blorp_eu_emitter(struct brw_context *brw)
   : mem_ctx(ralloc_context(NULL))
{
   brw_init_compile(brw, &func, mem_ctx);

   /*
    * By default everything is emitted as 16-wide with only a few expections
    * handled explicitly either here in the compiler or by one of the specific
    * code emission calls.
    * It should be also noted that here in this file any alterations of the
    * compression control settings are only used to affect the execution size
    * of the instructions. The instruction template used to initialise all the
    * instructions is effectively not altered -- the value stays at zero
    * representing either GEN6_COMPRESSION_1Q or GEN6_COMPRESSION_1H depending
    * on the context.
    * If any other settings are used in the instruction headers, they are set
    * elsewhere by the individual code emission calls.
    */
   brw_set_compression_control(&func, BRW_COMPRESSION_COMPRESSED);
}

brw_blorp_eu_emitter::~brw_blorp_eu_emitter()
{
   ralloc_free(mem_ctx);
}

const unsigned *
brw_blorp_eu_emitter::get_program(unsigned *program_size, FILE *dump_file)
{
   brw_set_uip_jip(&func);

   if (unlikely(INTEL_DEBUG & DEBUG_BLORP)) {
      printf("Native code for BLORP blit:\n");
      brw_dump_compile(&func, dump_file, 0, func.next_insn_offset);
      printf("\n");
   }

   return brw_get_program(&func, program_size);
}

/**
 * Emit code that kills pixels whose X and Y coordinates are outside the
 * boundary of the rectangle defined by the push constants (dst_x0, dst_y0,
 * dst_x1, dst_y1).
 */
void
brw_blorp_eu_emitter::emit_kill_if_outside_rect(const struct brw_reg &x,
                                                const struct brw_reg &y,
                                                const struct brw_reg &dst_x0,
                                                const struct brw_reg &dst_x1,
                                                const struct brw_reg &dst_y0,
                                                const struct brw_reg &dst_y1)
{
   struct brw_reg f0 = brw_flag_reg(0, 0);
   struct brw_reg g1 = retype(brw_vec1_grf(1, 7), BRW_REGISTER_TYPE_UW);
   struct brw_reg null32 = vec16(retype(brw_null_reg(), BRW_REGISTER_TYPE_UD));

   brw_CMP(&func, null32, BRW_CONDITIONAL_GE, x, dst_x0);
   brw_CMP(&func, null32, BRW_CONDITIONAL_GE, y, dst_y0);
   brw_CMP(&func, null32, BRW_CONDITIONAL_L, x, dst_x1);
   brw_CMP(&func, null32, BRW_CONDITIONAL_L, y, dst_y1);

   brw_set_predicate_control(&func, BRW_PREDICATE_NONE);

   struct brw_instruction *inst = brw_AND(&func, g1, f0, g1);
   inst->header.mask_control = BRW_MASK_DISABLE;
}

void
brw_blorp_eu_emitter::emit_texture_lookup(const struct brw_reg &dst,
                                          enum opcode op,
                                          unsigned base_mrf,
                                          unsigned msg_length)
{
   unsigned msg_type;

   switch (op) {
   case SHADER_OPCODE_TEX:
      msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE;
      break;
   case SHADER_OPCODE_TXF:
      msg_type = GEN5_SAMPLER_MESSAGE_SAMPLE_LD;
      break;
   case SHADER_OPCODE_TXF_CMS:
      msg_type = GEN7_SAMPLER_MESSAGE_SAMPLE_LD2DMS;
      break;
   case SHADER_OPCODE_TXF_UMS:
      msg_type = GEN7_SAMPLER_MESSAGE_SAMPLE_LD2DSS;
      break;
   case SHADER_OPCODE_TXF_MCS:
      msg_type = GEN7_SAMPLER_MESSAGE_SAMPLE_LD_MCS;
      break;
   default:
      assert(!"Unsupported texture lookup operation");
   }

   brw_SAMPLE(&func,
              retype(dst, BRW_REGISTER_TYPE_UW) /* dest */,
              base_mrf /* msg_reg_nr */,
              brw_message_reg(base_mrf) /* src0 */,
              BRW_BLORP_TEXTURE_BINDING_TABLE_INDEX,
              0 /* sampler */,
              msg_type,
              8 /* response_length.  TODO: should be smaller for non-RGBA formats? */,
              msg_length,
              0 /* header_present */,
              BRW_SAMPLER_SIMD_MODE_SIMD16,
              BRW_SAMPLER_RETURN_FORMAT_FLOAT32);
}

void
brw_blorp_eu_emitter::emit_render_target_write(const struct brw_reg &src0,
                                               unsigned msg_reg_nr,
                                               unsigned msg_length,
                                               bool use_header)
{
   brw_fb_WRITE(&func,
                16 /* dispatch_width */,
                msg_reg_nr,
                src0,
                BRW_DATAPORT_RENDER_TARGET_WRITE_SIMD16_SINGLE_SOURCE,
                BRW_BLORP_RENDERBUFFER_BINDING_TABLE_INDEX,
                msg_length,
                0 /* response_length */,
                true /* eot */,
                use_header);
}
