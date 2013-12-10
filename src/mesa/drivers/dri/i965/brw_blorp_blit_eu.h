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

#ifndef BRW_BLORP_BLIT_EU_H
#define BRW_BLORP_BLIT_EU_H

#include "brw_context.h"
#include "brw_fs.h"

class brw_blorp_eu_emitter
{
protected:
   explicit brw_blorp_eu_emitter(struct brw_context *brw);
   ~brw_blorp_eu_emitter();

   const unsigned *get_program(unsigned *program_size, FILE *dump_file);

   void emit_kill_if_outside_rect(const struct brw_reg &x,
                                  const struct brw_reg &y,
                                  const struct brw_reg &dst_x0,
                                  const struct brw_reg &dst_x1,
                                  const struct brw_reg &dst_y0,
                                  const struct brw_reg &dst_y1);

   void emit_texture_lookup(const struct brw_reg &dst,
                            enum opcode op,
                            unsigned base_mrf,
                            unsigned msg_length);

   void emit_render_target_write(const struct brw_reg &src0,
                                 unsigned msg_reg_nr,
                                 unsigned msg_length,
                                 bool use_header);

   void emit_combine(enum opcode combine_opcode,
                     const struct brw_reg &dst,
                     const struct brw_reg &src_1,
                     const struct brw_reg &src_2);

   inline void emit_cond_mov(const struct brw_reg &x,
                             const struct brw_reg &y,
                             int op,
                             const struct brw_reg &dst,
                             const struct brw_reg &src)
   {
      emit_cmp(op, x, y);

      fs_inst *mv = new (mem_ctx) fs_inst(BRW_OPCODE_MOV, dst, src);
      mv->predicate = BRW_PREDICATE_NORMAL;
      insts.push_tail(mv);
   }

   inline void emit_if_eq_mov(const struct brw_reg &x, unsigned y,
                              const struct brw_reg &dst, unsigned src)
   {
      emit_cond_mov(x, brw_imm_d(y), BRW_CONDITIONAL_EQ, dst, brw_imm_d(src));
   }

   inline void emit_lrp(const struct brw_reg &dst,
                        const struct brw_reg &src1,
                        const struct brw_reg &src2,
                        const struct brw_reg &src3)
   {
      insts.push_tail(
         new (mem_ctx) fs_inst(BRW_OPCODE_LRP, dst, src1, src2, src3));
   }

   inline void emit_mov(const struct brw_reg& dst, const struct brw_reg& src)
   {
      insts.push_tail(new (mem_ctx) fs_inst(BRW_OPCODE_MOV, dst, src));
   }

   inline void emit_mov_8(const struct brw_reg& dst, const struct brw_reg& src)
   {
      fs_inst *mv = new (mem_ctx) fs_inst(BRW_OPCODE_MOV, dst, src);
      mv->force_uncompressed = true;
      insts.push_tail(mv);
   }

   inline void emit_and(const struct brw_reg& dst,
                        const struct brw_reg& src1,
                        const struct brw_reg& src2)
   {
      insts.push_tail(new (mem_ctx) fs_inst(BRW_OPCODE_AND, dst, src1, src2));
   }

   inline void emit_add(const struct brw_reg& dst,
                        const struct brw_reg& src1,
                        const struct brw_reg& src2)
   {
      insts.push_tail(new (mem_ctx) fs_inst(BRW_OPCODE_ADD, dst, src1, src2));
   }

   inline void emit_add_8(const struct brw_reg& dst,
                          const struct brw_reg& src1,
                          const struct brw_reg& src2)
   {
      fs_inst *add = new (mem_ctx) fs_inst(BRW_OPCODE_ADD, dst, src1, src2);
      add->force_uncompressed = true;
      insts.push_tail(add);
   }

   inline void emit_mul(const struct brw_reg& dst,
                        const struct brw_reg& src1,
                        const struct brw_reg& src2)
   {
      insts.push_tail(new (mem_ctx) fs_inst(BRW_OPCODE_MUL, dst, src1, src2));
   }

   inline void emit_shr(const struct brw_reg& dst,
                        const struct brw_reg& src1,
                        const struct brw_reg& src2)
   {
      insts.push_tail(new (mem_ctx) fs_inst(BRW_OPCODE_SHR, dst, src1, src2));
   }

   inline void emit_shl(const struct brw_reg& dst,
                        const struct brw_reg& src1,
                        const struct brw_reg& src2)
   {
      insts.push_tail(new (mem_ctx) fs_inst(BRW_OPCODE_SHL, dst, src1, src2));
   }

   inline void emit_or(const struct brw_reg& dst,
                       const struct brw_reg& src1,
                       const struct brw_reg& src2)
   {
      insts.push_tail(new (mem_ctx) fs_inst(BRW_OPCODE_OR, dst, src1, src2));
   }

   inline void emit_frc(const struct brw_reg& dst,
                        const struct brw_reg& src)
   {
      insts.push_tail(new (mem_ctx) fs_inst(BRW_OPCODE_FRC, dst, src));
   }

   inline void emit_rndd(const struct brw_reg& dst,
                         const struct brw_reg& src)
   {
      insts.push_tail(new (mem_ctx) fs_inst(BRW_OPCODE_RNDD, dst, src));
   }

   inline void emit_cmp_if(int op,
                           const struct brw_reg &x,
                           const struct brw_reg &y)
   {
      emit_cmp(op, x, y);
      insts.push_tail(new (mem_ctx) fs_inst(BRW_OPCODE_IF));
   }

   inline void emit_else(void)
   {
      insts.push_tail(new (mem_ctx) fs_inst(BRW_OPCODE_ELSE));
   }

   inline void emit_endif(void)
   {
      insts.push_tail(new (mem_ctx) fs_inst(BRW_OPCODE_ENDIF));
   }

private:
   fs_inst *emit_cmp(int op, const struct brw_reg &x, const struct brw_reg &y);

   void *mem_ctx;
   struct brw_wm_compile *c;
   exec_list insts;
   fs_generator generator;
};

#endif /* BRW_BLORP_BLIT_EU_H */
