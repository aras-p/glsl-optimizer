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
#include "brw_eu.h"

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
      brw_CMP(&func, vec16(brw_null_reg()), op, x, y);
      brw_MOV(&func, dst, src);
      brw_set_predicate_control(&func, BRW_PREDICATE_NONE);
   }

   inline void emit_if_eq_mov(const struct brw_reg &x, unsigned y,
                              const struct brw_reg &dst, unsigned src)
   {
      emit_cond_mov(x, brw_imm_d(y), BRW_CONDITIONAL_EQ, dst, brw_imm_d(src));
   }

   inline void emit_mov(const struct brw_reg& dst, const struct brw_reg& src)
   {
      brw_MOV(&func, dst, src);
   }

   inline void emit_mov_8(const struct brw_reg& dst, const struct brw_reg& src)
   {
      brw_set_compression_control(&func, BRW_COMPRESSION_NONE);
      emit_mov(dst, src);
      brw_set_compression_control(&func, BRW_COMPRESSION_COMPRESSED);
   }

   void *mem_ctx;
   struct brw_compile func;
};

#endif /* BRW_BLORP_BLIT_EU_H */
