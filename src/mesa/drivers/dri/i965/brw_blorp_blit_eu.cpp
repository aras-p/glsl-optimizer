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
   : mem_ctx(ralloc_context(NULL)), c(rzalloc(mem_ctx, struct brw_wm_compile)),
     generator(brw, c, NULL, NULL, false)
{
}

brw_blorp_eu_emitter::~brw_blorp_eu_emitter()
{
   ralloc_free(mem_ctx);
}

const unsigned *
brw_blorp_eu_emitter::get_program(unsigned *program_size, FILE *dump_file)
{
   const unsigned *res;

   if (unlikely(INTEL_DEBUG & DEBUG_BLORP)) {
      fprintf(stderr, "Native code for BLORP blit:\n");
      res = generator.generate_assembly(NULL, &insts, program_size, dump_file);
      fprintf(stderr, "\n");
   } else {
      res = generator.generate_assembly(NULL, &insts, program_size);
   }

   return res;
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

   emit_cmp(BRW_CONDITIONAL_GE, x, dst_x0);
   emit_cmp(BRW_CONDITIONAL_GE, y, dst_y0)->predicate = BRW_PREDICATE_NORMAL;
   emit_cmp(BRW_CONDITIONAL_L, x, dst_x1)->predicate = BRW_PREDICATE_NORMAL;
   emit_cmp(BRW_CONDITIONAL_L, y, dst_y1)->predicate = BRW_PREDICATE_NORMAL;

   fs_inst *inst = new (mem_ctx) fs_inst(BRW_OPCODE_AND, g1, f0, g1);
   inst->force_writemask_all = true;
   insts.push_tail(inst);
}

void
brw_blorp_eu_emitter::emit_texture_lookup(const struct brw_reg &dst,
                                          enum opcode op,
                                          unsigned base_mrf,
                                          unsigned msg_length)
{
   fs_inst *inst = new (mem_ctx) fs_inst(op, dst, brw_message_reg(base_mrf));

   inst->base_mrf = base_mrf;
   inst->mlen = msg_length;
   inst->sampler = 0;
   inst->header_present = false;

   insts.push_tail(inst);
}

void
brw_blorp_eu_emitter::emit_render_target_write(const struct brw_reg &src0,
                                               unsigned msg_reg_nr,
                                               unsigned msg_length,
                                               bool use_header)
{
   fs_inst *inst = new (mem_ctx) fs_inst(FS_OPCODE_BLORP_FB_WRITE);

   inst->src[0] = src0;
   inst->base_mrf = msg_reg_nr;
   inst->mlen = msg_length;
   inst->header_present = use_header;
   inst->target = BRW_BLORP_RENDERBUFFER_BINDING_TABLE_INDEX;

   insts.push_tail(inst);
}

void
brw_blorp_eu_emitter::emit_combine(enum opcode combine_opcode,
                                   const struct brw_reg &dst,
                                   const struct brw_reg &src_1,
                                   const struct brw_reg &src_2)
{
   assert(combine_opcode == BRW_OPCODE_ADD || combine_opcode == BRW_OPCODE_AVG);

   insts.push_tail(new (mem_ctx) fs_inst(combine_opcode, dst, src_1, src_2));
}

fs_inst *
brw_blorp_eu_emitter::emit_cmp(int op,
                               const struct brw_reg &x,
                               const struct brw_reg &y)
{
   fs_inst *cmp = new (mem_ctx) fs_inst(BRW_OPCODE_CMP,
                                        vec16(brw_null_reg()), x, y);
   cmp->conditional_mod = op;
   insts.push_tail(cmp);
   return cmp;
}

