/*
 * Copyright © 2012 Intel Corporation
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

/**
 * @file gen8_instruction.c
 *
 * A representation of a Gen8+ EU instruction, with helper methods to get
 * and set various fields.  This is the actual hardware format.
 */

#include "brw_defines.h"
#include "gen8_instruction.h"

static void
gen8_convert_mrf_to_grf(struct brw_reg *reg)
{
   /* From the Ivybridge PRM, Volume 4 Part 3, page 218 ("send"):
    * "The send with EOT should use register space R112-R127 for <src>. This is
    *  to enable loading of a new thread into the same slot while the message
    *  with EOT for current thread is pending dispatch."
    *
    * Since we're pretending to have 16 MRFs anyway, we may as well use the
    * registers required for messages with EOT.
    */
   if (reg->file == BRW_MESSAGE_REGISTER_FILE) {
      reg->file = BRW_GENERAL_REGISTER_FILE;
      reg->nr += GEN7_MRF_HACK_START;
   }
}

void
gen8_set_dst(const struct brw_context *brw,
             struct gen8_instruction *inst,
             struct brw_reg reg)
{
   gen8_convert_mrf_to_grf(&reg);

   if (reg.file == BRW_GENERAL_REGISTER_FILE)
      assert(reg.nr < BRW_MAX_GRF);

   gen8_set_dst_reg_file(inst, reg.file);
   gen8_set_dst_reg_type(inst, brw_reg_type_to_hw_type(brw, reg.type, reg.file));
   gen8_set_dst_address_mode(inst, reg.address_mode);

   if (reg.address_mode == BRW_ADDRESS_DIRECT) {
      gen8_set_dst_da_reg_nr(inst, reg.nr);

      if (gen8_access_mode(inst) == BRW_ALIGN_1) {
         /* Set Dst.SubRegNum[4:0] */
         gen8_set_dst_da1_subreg_nr(inst, reg.subnr);

         /* Set Dst.HorzStride */
         if (reg.hstride == BRW_HORIZONTAL_STRIDE_0)
            reg.hstride = BRW_HORIZONTAL_STRIDE_1;
         gen8_set_dst_da1_hstride(inst, reg.hstride);
      } else {
         /* Align16 SubRegNum only has a single bit (bit 4; bits 3:0 MBZ). */
         assert(reg.subnr == 0 || reg.subnr == 16);
         gen8_set_dst_da16_subreg_nr(inst, reg.subnr >> 4);
         gen8_set_da16_writemask(inst, reg.dw1.bits.writemask);
      }
   } else {
      /* Indirect addressing */
      assert(gen8_access_mode(inst) == BRW_ALIGN_1);

      /* Set Dst.HorzStride */
      if (reg.hstride == BRW_HORIZONTAL_STRIDE_0)
         reg.hstride = BRW_HORIZONTAL_STRIDE_1;
      gen8_set_dst_da1_hstride(inst, reg.hstride);
      gen8_set_dst_ia1_subreg_nr(inst, reg.subnr);
      gen8_set_dst_ia1_addr_imm(inst, reg.dw1.bits.indirect_offset);
   }

   /* Generators should set a default exec_size of either 8 (SIMD4x2 or SIMD8)
    * or 16 (SIMD16), as that's normally correct.  However, when dealing with
    * small registers, we automatically reduce it to match the register size.
    */
   if (reg.width < BRW_EXECUTE_8)
      gen8_set_exec_size(inst, reg.width);
}

static void
gen8_validate_reg(struct gen8_instruction *inst, struct brw_reg reg)
{
   int hstride_for_reg[] = {0, 1, 2, 4};
   int vstride_for_reg[] = {0, 1, 2, 4, 8, 16, 32, 64, 128, 256};
   int width_for_reg[] = {1, 2, 4, 8, 16};
   int execsize_for_reg[] = {1, 2, 4, 8, 16};
   int width, hstride, vstride, execsize;

   if (reg.file == BRW_IMMEDIATE_VALUE) {
      /* TODO: check immediate vectors */
      return;
   }

   if (reg.file == BRW_ARCHITECTURE_REGISTER_FILE)
      return;

   assert(reg.hstride >= 0 && reg.hstride < ARRAY_SIZE(hstride_for_reg));
   hstride = hstride_for_reg[reg.hstride];

   if (reg.vstride == 0xf) {
      vstride = -1;
   } else {
      assert(reg.vstride >= 0 && reg.vstride < ARRAY_SIZE(vstride_for_reg));
      vstride = vstride_for_reg[reg.vstride];
   }

   assert(reg.width >= 0 && reg.width < ARRAY_SIZE(width_for_reg));
   width = width_for_reg[reg.width];

   assert(gen8_exec_size(inst) >= 0 &&
          gen8_exec_size(inst) < ARRAY_SIZE(execsize_for_reg));
   execsize = execsize_for_reg[gen8_exec_size(inst)];

   /* Restrictions from 3.3.10: Register Region Restrictions. */
   /* 3. */
   assert(execsize >= width);

   /* 4. */
   if (execsize == width && hstride != 0) {
      assert(vstride == -1 || vstride == width * hstride);
   }

   /* 5. */
   if (execsize == width && hstride == 0) {
      /* no restriction on vstride. */
   }

   /* 6. */
   if (width == 1) {
      assert(hstride == 0);
   }

   /* 7. */
   if (execsize == 1 && width == 1) {
      assert(hstride == 0);
      assert(vstride == 0);
   }

   /* 8. */
   if (vstride == 0 && hstride == 0) {
      assert(width == 1);
   }

   /* 10. Check destination issues. */
}

void
gen8_set_src0(const struct brw_context *brw,
              struct gen8_instruction *inst,
              struct brw_reg reg)
{
   gen8_convert_mrf_to_grf(&reg);

   if (reg.file == BRW_GENERAL_REGISTER_FILE)
      assert(reg.nr < BRW_MAX_GRF);

   gen8_validate_reg(inst, reg);

   gen8_set_src0_reg_file(inst, reg.file);
   gen8_set_src0_reg_type(inst,
                          brw_reg_type_to_hw_type(brw, reg.type, reg.file));
   gen8_set_src0_abs(inst, reg.abs);
   gen8_set_src0_negate(inst, reg.negate);

   if (reg.file == BRW_IMMEDIATE_VALUE) {
      inst->data[3] = reg.dw1.ud;

      /* Required to set some fields in src1 as well: */
      gen8_set_src1_reg_file(inst, BRW_ARCHITECTURE_REGISTER_FILE);
      gen8_set_src1_reg_type(inst,
                             brw_reg_type_to_hw_type(brw, reg.type, reg.file));
      return;
   }

   gen8_set_src0_address_mode(inst, reg.address_mode);

   if (reg.address_mode == BRW_ADDRESS_DIRECT) {
      gen8_set_src0_da_reg_nr(inst, reg.nr);

      if (gen8_access_mode(inst) == BRW_ALIGN_1) {
         /* Set Src0.SubRegNum[4:0] */
         gen8_set_src0_da1_subreg_nr(inst, reg.subnr);

         if (reg.width == BRW_WIDTH_1 && gen8_exec_size(inst) == BRW_EXECUTE_1) {
            gen8_set_src0_da1_hstride(inst, BRW_HORIZONTAL_STRIDE_0);
            gen8_set_src0_vert_stride(inst, BRW_VERTICAL_STRIDE_0);
         } else {
            gen8_set_src0_da1_hstride(inst, reg.hstride);
            gen8_set_src0_vert_stride(inst, reg.vstride);
         }
         gen8_set_src0_da1_width(inst, reg.width);

      } else {
         /* Align16 SubRegNum only has a single bit (bit 4; bits 3:0 MBZ). */
         assert(reg.subnr == 0 || reg.subnr == 16);
         gen8_set_src0_da16_subreg_nr(inst, reg.subnr >> 4);

         gen8_set_src0_da16_swiz_x(inst,
                                   BRW_GET_SWZ(reg.dw1.bits.swizzle,
                                               BRW_CHANNEL_X));
         gen8_set_src0_da16_swiz_y(inst,
                                   BRW_GET_SWZ(reg.dw1.bits.swizzle,
                                               BRW_CHANNEL_Y));
         gen8_set_src0_da16_swiz_z(inst,
                                   BRW_GET_SWZ(reg.dw1.bits.swizzle,
                                               BRW_CHANNEL_Z));
         gen8_set_src0_da16_swiz_w(inst,
                                   BRW_GET_SWZ(reg.dw1.bits.swizzle,
                                               BRW_CHANNEL_W));

         /* This is an oddity of the fact that we're using the same
          * descriptions for registers in both Align16 and Align1 modes.
          */
         if (reg.vstride == BRW_VERTICAL_STRIDE_8)
            gen8_set_src0_vert_stride(inst, BRW_VERTICAL_STRIDE_4);
         else
            gen8_set_src0_vert_stride(inst, reg.vstride);
      }
   } else {
      /* Indirect addressing */
      assert(gen8_access_mode(inst) == BRW_ALIGN_1);
      if (reg.width == BRW_WIDTH_1 &&
         gen8_exec_size(inst) == BRW_EXECUTE_1) {
         gen8_set_src0_da1_hstride(inst, BRW_HORIZONTAL_STRIDE_0);
         gen8_set_src0_vert_stride(inst, BRW_VERTICAL_STRIDE_0);
      } else {
         gen8_set_src0_da1_hstride(inst, reg.hstride);
         gen8_set_src0_vert_stride(inst, reg.vstride);
      }

      gen8_set_src0_da1_width(inst, reg.width);
      gen8_set_src0_ia1_subreg_nr(inst, reg.subnr);
      gen8_set_src0_ia1_addr_imm(inst, reg.dw1.bits.indirect_offset);
   }
}

void
gen8_set_src1(const struct brw_context *brw,
              struct gen8_instruction *inst,
              struct brw_reg reg)
{
   gen8_convert_mrf_to_grf(&reg);

   if (reg.file == BRW_GENERAL_REGISTER_FILE)
      assert(reg.nr < BRW_MAX_GRF);

   gen8_validate_reg(inst, reg);

   gen8_set_src1_reg_file(inst, reg.file);
   gen8_set_src1_reg_type(inst,
                          brw_reg_type_to_hw_type(brw, reg.type, reg.file));
   gen8_set_src1_abs(inst, reg.abs);
   gen8_set_src1_negate(inst, reg.negate);

   /* Only src1 can be an immediate in two-argument instructions. */
   assert(gen8_src0_reg_file(inst) != BRW_IMMEDIATE_VALUE);

   if (reg.file == BRW_IMMEDIATE_VALUE) {
      inst->data[3] = reg.dw1.ud;
      return;
   }

   gen8_set_src1_address_mode(inst, reg.address_mode);

   if (reg.address_mode == BRW_ADDRESS_DIRECT) {
      gen8_set_src1_da_reg_nr(inst, reg.nr);

      if (gen8_access_mode(inst) == BRW_ALIGN_1) {
         /* Set Src0.SubRegNum[4:0] */
         gen8_set_src1_da1_subreg_nr(inst, reg.subnr);

         if (reg.width == BRW_WIDTH_1 && gen8_exec_size(inst) == BRW_EXECUTE_1) {
            gen8_set_src1_da1_hstride(inst, BRW_HORIZONTAL_STRIDE_0);
            gen8_set_src1_vert_stride(inst, BRW_VERTICAL_STRIDE_0);
         } else {
            gen8_set_src1_da1_hstride(inst, reg.hstride);
            gen8_set_src1_vert_stride(inst, reg.vstride);
         }
         gen8_set_src1_da1_width(inst, reg.width);
      } else {
         /* Align16 SubRegNum only has a single bit (bit 4; bits 3:0 MBZ). */
         assert(reg.subnr == 0 || reg.subnr == 16);
         gen8_set_src1_da16_subreg_nr(inst, reg.subnr >> 4);

         gen8_set_src1_da16_swiz_x(inst,
                                   BRW_GET_SWZ(reg.dw1.bits.swizzle,
                                               BRW_CHANNEL_X));
         gen8_set_src1_da16_swiz_y(inst,
                                   BRW_GET_SWZ(reg.dw1.bits.swizzle,
                                               BRW_CHANNEL_Y));
         gen8_set_src1_da16_swiz_z(inst,
                                   BRW_GET_SWZ(reg.dw1.bits.swizzle,
                                               BRW_CHANNEL_Z));
         gen8_set_src1_da16_swiz_w(inst,
                                   BRW_GET_SWZ(reg.dw1.bits.swizzle,
                                               BRW_CHANNEL_W));

         /* This is an oddity of the fact that we're using the same
          * descriptions for registers in both Align16 and Align1 modes.
          */
         if (reg.vstride == BRW_VERTICAL_STRIDE_8)
            gen8_set_src1_vert_stride(inst, BRW_VERTICAL_STRIDE_4);
         else
            gen8_set_src1_vert_stride(inst, reg.vstride);
      }
   } else {
      /* Indirect addressing */
      assert(gen8_access_mode(inst) == BRW_ALIGN_1);
      if (reg.width == BRW_WIDTH_1 && gen8_exec_size(inst) == BRW_EXECUTE_1) {
         gen8_set_src1_da1_hstride(inst, BRW_HORIZONTAL_STRIDE_0);
         gen8_set_src1_vert_stride(inst, BRW_VERTICAL_STRIDE_0);
      } else {
         gen8_set_src1_da1_hstride(inst, reg.hstride);
         gen8_set_src1_vert_stride(inst, reg.vstride);
      }

      gen8_set_src1_da1_width(inst, reg.width);
      gen8_set_src1_ia1_subreg_nr(inst, reg.subnr);
      gen8_set_src1_ia1_addr_imm(inst, reg.dw1.bits.indirect_offset);
   }
}

/**
 * Set the Message Descriptor and Extended Message Descriptor fields
 * for SEND messages.
 *
 * \note This zeroes out the Function Control bits, so it must be called
 *       \b before filling out any message-specific data.  Callers can
 *       choose not to fill in irrelevant bits; they will be zero.
 */
static void
gen8_set_message_descriptor(const struct brw_context *brw,
                            struct gen8_instruction *inst,
                            enum brw_message_target sfid,
                            unsigned msg_length,
                            unsigned response_length,
                            bool header_present,
                            bool end_of_thread)
{
   gen8_set_src1(brw, inst, brw_imm_d(0));

   gen8_set_sfid(inst, sfid);
   gen8_set_mlen(inst, msg_length);
   gen8_set_rlen(inst, response_length);
   gen8_set_header_present(inst, header_present);
   gen8_set_eot(inst, end_of_thread);
}

void
gen8_set_urb_message(const struct brw_context *brw,
                     struct gen8_instruction *inst,
                     enum brw_urb_write_flags flags,
                     unsigned msg_length,
                     unsigned response_length,
                     unsigned offset,
                     bool interleave)
{
   gen8_set_message_descriptor(brw, inst, BRW_SFID_URB,
                               msg_length, response_length,
                               true, flags & BRW_URB_WRITE_EOT);
   gen8_set_src0(brw, inst, brw_vec8_grf(GEN7_MRF_HACK_START + 1, 0));
   if (flags & BRW_URB_WRITE_OWORD) {
      assert(msg_length == 2);
      gen8_set_urb_opcode(inst, BRW_URB_OPCODE_WRITE_OWORD);
   } else {
      gen8_set_urb_opcode(inst, BRW_URB_OPCODE_WRITE_HWORD);
   }
   gen8_set_urb_global_offset(inst, offset);
   gen8_set_urb_interleave(inst, interleave);
   gen8_set_urb_per_slot_offset(inst,
                                flags & BRW_URB_WRITE_PER_SLOT_OFFSET ? 1 : 0);
}

void
gen8_set_sampler_message(const struct brw_context *brw,
                         struct gen8_instruction *inst,
                         unsigned binding_table_index,
                         unsigned sampler,
                         unsigned msg_type,
                         unsigned response_length,
                         unsigned msg_length,
                         bool header_present,
                         unsigned simd_mode)
{
   gen8_set_message_descriptor(brw, inst, BRW_SFID_SAMPLER, msg_length,
                               response_length, header_present, false);

   gen8_set_binding_table_index(inst, binding_table_index);
   gen8_set_sampler(inst, sampler);
   gen8_set_sampler_msg_type(inst, msg_type);
   gen8_set_sampler_simd_mode(inst, simd_mode);
}

void
gen8_set_dp_message(const struct brw_context *brw,
                    struct gen8_instruction *inst,
                    enum brw_message_target sfid,
                    unsigned binding_table_index,
                    unsigned msg_type,
                    unsigned msg_control,
                    unsigned mlen,
                    unsigned rlen,
                    bool header_present,
                    bool end_of_thread)
{
   gen8_set_message_descriptor(brw, inst, sfid, mlen, rlen, header_present,
                               end_of_thread);
   gen8_set_binding_table_index(inst, binding_table_index);
   gen8_set_dp_message_type(inst, msg_type);
   gen8_set_dp_message_control(inst, msg_control);
}

void
gen8_set_dp_scratch_message(const struct brw_context *brw,
                            struct gen8_instruction *inst,
                            bool write,
                            bool dword,
                            bool invalidate_after_read,
                            unsigned num_regs,
                            unsigned addr_offset,
                            unsigned mlen,
                            unsigned rlen,
                            bool header_present,
                            bool end_of_thread)
{
   assert(num_regs == 1 || num_regs == 2 || num_regs == 4 || num_regs == 8);
   gen8_set_message_descriptor(brw, inst, GEN7_SFID_DATAPORT_DATA_CACHE,
                               mlen, rlen, header_present, end_of_thread);
   gen8_set_dp_category(inst, 1); /* Scratch Block Read/Write messages */
   gen8_set_scratch_read_write(inst, write);
   gen8_set_scratch_type(inst, dword);
   gen8_set_scratch_invalidate_after_read(inst, invalidate_after_read);
   gen8_set_scratch_block_size(inst, ffs(num_regs) - 1);
   gen8_set_scratch_addr_offset(inst, addr_offset);
}
