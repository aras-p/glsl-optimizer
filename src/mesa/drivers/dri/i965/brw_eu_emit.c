/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics to
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
  *   Keith Whitwell <keithw@vmware.com>
  */


#include "brw_context.h"
#include "brw_defines.h"
#include "brw_eu.h"

#include "util/ralloc.h"

/***********************************************************************
 * Internal helper for constructing instructions
 */

static void guess_execution_size(struct brw_compile *p,
				 brw_inst *insn,
				 struct brw_reg reg)
{
   const struct brw_context *brw = p->brw;

   if (reg.width == BRW_WIDTH_8 && p->compressed) {
      brw_inst_set_exec_size(brw, insn, BRW_EXECUTE_16);
   } else {
      /* Register width definitions are compatible with BRW_EXECUTE_* enums. */
      brw_inst_set_exec_size(brw, insn, reg.width);
   }
}


/**
 * Prior to Sandybridge, the SEND instruction accepted non-MRF source
 * registers, implicitly moving the operand to a message register.
 *
 * On Sandybridge, this is no longer the case.  This function performs the
 * explicit move; it should be called before emitting a SEND instruction.
 */
void
gen6_resolve_implied_move(struct brw_compile *p,
			  struct brw_reg *src,
			  unsigned msg_reg_nr)
{
   struct brw_context *brw = p->brw;
   if (brw->gen < 6)
      return;

   if (src->file == BRW_MESSAGE_REGISTER_FILE)
      return;

   if (src->file != BRW_ARCHITECTURE_REGISTER_FILE || src->nr != BRW_ARF_NULL) {
      brw_push_insn_state(p);
      brw_set_default_mask_control(p, BRW_MASK_DISABLE);
      brw_set_default_compression_control(p, BRW_COMPRESSION_NONE);
      brw_MOV(p, retype(brw_message_reg(msg_reg_nr), BRW_REGISTER_TYPE_UD),
	      retype(*src, BRW_REGISTER_TYPE_UD));
      brw_pop_insn_state(p);
   }
   *src = brw_message_reg(msg_reg_nr);
}

static void
gen7_convert_mrf_to_grf(struct brw_compile *p, struct brw_reg *reg)
{
   /* From the Ivybridge PRM, Volume 4 Part 3, page 218 ("send"):
    * "The send with EOT should use register space R112-R127 for <src>. This is
    *  to enable loading of a new thread into the same slot while the message
    *  with EOT for current thread is pending dispatch."
    *
    * Since we're pretending to have 16 MRFs anyway, we may as well use the
    * registers required for messages with EOT.
    */
   struct brw_context *brw = p->brw;
   if (brw->gen >= 7 && reg->file == BRW_MESSAGE_REGISTER_FILE) {
      reg->file = BRW_GENERAL_REGISTER_FILE;
      reg->nr += GEN7_MRF_HACK_START;
   }
}

/**
 * Convert a brw_reg_type enumeration value into the hardware representation.
 *
 * The hardware encoding may depend on whether the value is an immediate.
 */
unsigned
brw_reg_type_to_hw_type(const struct brw_context *brw,
                        enum brw_reg_type type, unsigned file)
{
   if (file == BRW_IMMEDIATE_VALUE) {
      const static int imm_hw_types[] = {
         [BRW_REGISTER_TYPE_UD] = BRW_HW_REG_TYPE_UD,
         [BRW_REGISTER_TYPE_D]  = BRW_HW_REG_TYPE_D,
         [BRW_REGISTER_TYPE_UW] = BRW_HW_REG_TYPE_UW,
         [BRW_REGISTER_TYPE_W]  = BRW_HW_REG_TYPE_W,
         [BRW_REGISTER_TYPE_F]  = BRW_HW_REG_TYPE_F,
         [BRW_REGISTER_TYPE_UB] = -1,
         [BRW_REGISTER_TYPE_B]  = -1,
         [BRW_REGISTER_TYPE_UV] = BRW_HW_REG_IMM_TYPE_UV,
         [BRW_REGISTER_TYPE_VF] = BRW_HW_REG_IMM_TYPE_VF,
         [BRW_REGISTER_TYPE_V]  = BRW_HW_REG_IMM_TYPE_V,
         [BRW_REGISTER_TYPE_DF] = GEN8_HW_REG_IMM_TYPE_DF,
         [BRW_REGISTER_TYPE_HF] = GEN8_HW_REG_IMM_TYPE_HF,
         [BRW_REGISTER_TYPE_UQ] = GEN8_HW_REG_TYPE_UQ,
         [BRW_REGISTER_TYPE_Q]  = GEN8_HW_REG_TYPE_Q,
      };
      assert(type < ARRAY_SIZE(imm_hw_types));
      assert(imm_hw_types[type] != -1);
      assert(brw->gen >= 8 || type < BRW_REGISTER_TYPE_DF);
      return imm_hw_types[type];
   } else {
      /* Non-immediate registers */
      const static int hw_types[] = {
         [BRW_REGISTER_TYPE_UD] = BRW_HW_REG_TYPE_UD,
         [BRW_REGISTER_TYPE_D]  = BRW_HW_REG_TYPE_D,
         [BRW_REGISTER_TYPE_UW] = BRW_HW_REG_TYPE_UW,
         [BRW_REGISTER_TYPE_W]  = BRW_HW_REG_TYPE_W,
         [BRW_REGISTER_TYPE_UB] = BRW_HW_REG_NON_IMM_TYPE_UB,
         [BRW_REGISTER_TYPE_B]  = BRW_HW_REG_NON_IMM_TYPE_B,
         [BRW_REGISTER_TYPE_F]  = BRW_HW_REG_TYPE_F,
         [BRW_REGISTER_TYPE_UV] = -1,
         [BRW_REGISTER_TYPE_VF] = -1,
         [BRW_REGISTER_TYPE_V]  = -1,
         [BRW_REGISTER_TYPE_DF] = GEN7_HW_REG_NON_IMM_TYPE_DF,
         [BRW_REGISTER_TYPE_HF] = GEN8_HW_REG_NON_IMM_TYPE_HF,
         [BRW_REGISTER_TYPE_UQ] = GEN8_HW_REG_TYPE_UQ,
         [BRW_REGISTER_TYPE_Q]  = GEN8_HW_REG_TYPE_Q,
      };
      assert(type < ARRAY_SIZE(hw_types));
      assert(hw_types[type] != -1);
      assert(brw->gen >= 7 || type < BRW_REGISTER_TYPE_DF);
      assert(brw->gen >= 8 || type < BRW_REGISTER_TYPE_HF);
      return hw_types[type];
   }
}

void
brw_set_dest(struct brw_compile *p, brw_inst *inst, struct brw_reg dest)
{
   const struct brw_context *brw = p->brw;

   if (dest.file != BRW_ARCHITECTURE_REGISTER_FILE &&
       dest.file != BRW_MESSAGE_REGISTER_FILE)
      assert(dest.nr < 128);

   gen7_convert_mrf_to_grf(p, &dest);

   brw_inst_set_dst_reg_file(brw, inst, dest.file);
   brw_inst_set_dst_reg_type(brw, inst, brw_reg_type_to_hw_type(brw, dest.type,
                                                                dest.file));
   brw_inst_set_dst_address_mode(brw, inst, dest.address_mode);

   if (dest.address_mode == BRW_ADDRESS_DIRECT) {
      brw_inst_set_dst_da_reg_nr(brw, inst, dest.nr);

      if (brw_inst_access_mode(brw, inst) == BRW_ALIGN_1) {
         brw_inst_set_dst_da1_subreg_nr(brw, inst, dest.subnr);
	 if (dest.hstride == BRW_HORIZONTAL_STRIDE_0)
	    dest.hstride = BRW_HORIZONTAL_STRIDE_1;
         brw_inst_set_dst_hstride(brw, inst, dest.hstride);
      } else {
         brw_inst_set_dst_da16_subreg_nr(brw, inst, dest.subnr / 16);
         brw_inst_set_da16_writemask(brw, inst, dest.dw1.bits.writemask);
         if (dest.file == BRW_GENERAL_REGISTER_FILE ||
             dest.file == BRW_MESSAGE_REGISTER_FILE) {
            assert(dest.dw1.bits.writemask != 0);
         }
	 /* From the Ivybridge PRM, Vol 4, Part 3, Section 5.2.4.1:
	  *    Although Dst.HorzStride is a don't care for Align16, HW needs
	  *    this to be programmed as "01".
	  */
         brw_inst_set_dst_hstride(brw, inst, 1);
      }
   } else {
      brw_inst_set_dst_ia_subreg_nr(brw, inst, dest.subnr);

      /* These are different sizes in align1 vs align16:
       */
      if (brw_inst_access_mode(brw, inst) == BRW_ALIGN_1) {
         brw_inst_set_dst_ia1_addr_imm(brw, inst,
                                       dest.dw1.bits.indirect_offset);
	 if (dest.hstride == BRW_HORIZONTAL_STRIDE_0)
	    dest.hstride = BRW_HORIZONTAL_STRIDE_1;
         brw_inst_set_dst_hstride(brw, inst, dest.hstride);
      } else {
         brw_inst_set_dst_ia16_addr_imm(brw, inst,
                                        dest.dw1.bits.indirect_offset);
	 /* even ignored in da16, still need to set as '01' */
         brw_inst_set_dst_hstride(brw, inst, 1);
      }
   }

   /* NEW: Set the execution size based on dest.width and
    * inst->compression_control:
    */
   guess_execution_size(p, inst, dest);
}

extern int reg_type_size[];

static void
validate_reg(const struct brw_context *brw, brw_inst *inst, struct brw_reg reg)
{
   int hstride_for_reg[] = {0, 1, 2, 4};
   int vstride_for_reg[] = {0, 1, 2, 4, 8, 16, 32, 64, 128, 256};
   int width_for_reg[] = {1, 2, 4, 8, 16};
   int execsize_for_reg[] = {1, 2, 4, 8, 16};
   int width, hstride, vstride, execsize;

   if (reg.file == BRW_IMMEDIATE_VALUE) {
      /* 3.3.6: Region Parameters.  Restriction: Immediate vectors
       * mean the destination has to be 128-bit aligned and the
       * destination horiz stride has to be a word.
       */
      if (reg.type == BRW_REGISTER_TYPE_V) {
         assert(hstride_for_reg[brw_inst_dst_hstride(brw, inst)] *
                reg_type_size[brw_inst_dst_reg_type(brw, inst)] == 2);
      }

      return;
   }

   if (reg.file == BRW_ARCHITECTURE_REGISTER_FILE &&
       reg.file == BRW_ARF_NULL)
      return;

   assert(reg.hstride >= 0 && reg.hstride < Elements(hstride_for_reg));
   hstride = hstride_for_reg[reg.hstride];

   if (reg.vstride == 0xf) {
      vstride = -1;
   } else {
      assert(reg.vstride >= 0 && reg.vstride < Elements(vstride_for_reg));
      vstride = vstride_for_reg[reg.vstride];
   }

   assert(reg.width >= 0 && reg.width < Elements(width_for_reg));
   width = width_for_reg[reg.width];

   assert(brw_inst_exec_size(brw, inst) >= 0 &&
          brw_inst_exec_size(brw, inst) < Elements(execsize_for_reg));
   execsize = execsize_for_reg[brw_inst_exec_size(brw, inst)];

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

static bool
is_compactable_immediate(unsigned imm)
{
   /* We get the low 12 bits as-is. */
   imm &= ~0xfff;

   /* We get one bit replicated through the top 20 bits. */
   return imm == 0 || imm == 0xfffff000;
}

void
brw_set_src0(struct brw_compile *p, brw_inst *inst, struct brw_reg reg)
{
   struct brw_context *brw = p->brw;

   if (reg.file != BRW_ARCHITECTURE_REGISTER_FILE)
      assert(reg.nr < 128);

   gen7_convert_mrf_to_grf(p, &reg);

   if (brw->gen >= 6 && (brw_inst_opcode(brw, inst) == BRW_OPCODE_SEND ||
                         brw_inst_opcode(brw, inst) == BRW_OPCODE_SENDC)) {
      /* Any source modifiers or regions will be ignored, since this just
       * identifies the MRF/GRF to start reading the message contents from.
       * Check for some likely failures.
       */
      assert(!reg.negate);
      assert(!reg.abs);
      assert(reg.address_mode == BRW_ADDRESS_DIRECT);
   }

   validate_reg(brw, inst, reg);

   brw_inst_set_src0_reg_file(brw, inst, reg.file);
   brw_inst_set_src0_reg_type(brw, inst,
                              brw_reg_type_to_hw_type(brw, reg.type, reg.file));
   brw_inst_set_src0_abs(brw, inst, reg.abs);
   brw_inst_set_src0_negate(brw, inst, reg.negate);
   brw_inst_set_src0_address_mode(brw, inst, reg.address_mode);

   if (reg.file == BRW_IMMEDIATE_VALUE) {
      brw_inst_set_imm_ud(brw, inst, reg.dw1.ud);

      /* The Bspec's section titled "Non-present Operands" claims that if src0
       * is an immediate that src1's type must be the same as that of src0.
       *
       * The SNB+ DataTypeIndex instruction compaction tables contain mappings
       * that do not follow this rule. E.g., from the IVB/HSW table:
       *
       *  DataTypeIndex   18-Bit Mapping       Mapped Meaning
       *        3         001000001011111101   r:f | i:vf | a:ud | <1> | dir |
       *
       * And from the SNB table:
       *
       *  DataTypeIndex   18-Bit Mapping       Mapped Meaning
       *        8         001000000111101100   a:w | i:w | a:ud | <1> | dir |
       *
       * Neither of these cause warnings from the simulator when used,
       * compacted or otherwise. In fact, all compaction mappings that have an
       * immediate in src0 use a:ud for src1.
       *
       * The GM45 instruction compaction tables do not contain mapped meanings
       * so it's not clear whether it has the restriction. We'll assume it was
       * lifted on SNB. (FINISHME: decode the GM45 tables and check.)
       */
      brw_inst_set_src1_reg_file(brw, inst, BRW_ARCHITECTURE_REGISTER_FILE);
      if (brw->gen < 6) {
         brw_inst_set_src1_reg_type(brw, inst,
                                    brw_inst_src0_reg_type(brw, inst));
      } else {
         brw_inst_set_src1_reg_type(brw, inst, BRW_HW_REG_TYPE_UD);
      }

      /* Compacted instructions only have 12-bits (plus 1 for the other 20)
       * for immediate values. Presumably the hardware engineers realized
       * that the only useful floating-point value that could be represented
       * in this format is 0.0, which can also be represented as a VF-typed
       * immediate, so they gave us the previously mentioned mapping on IVB+.
       *
       * Strangely, we do have a mapping for imm:f in src1, so we don't need
       * to do this there.
       *
       * If we see a 0.0:F, change the type to VF so that it can be compacted.
       */
      if (brw_inst_imm_ud(brw, inst) == 0x0 &&
          brw_inst_src0_reg_type(brw, inst) == BRW_HW_REG_TYPE_F) {
         brw_inst_set_src0_reg_type(brw, inst, BRW_HW_REG_IMM_TYPE_VF);
      }

      /* There are no mappings for dst:d | i:d, so if the immediate is suitable
       * set the types to :UD so the instruction can be compacted.
       */
      if (is_compactable_immediate(brw_inst_imm_ud(brw, inst)) &&
          brw_inst_cond_modifier(brw, inst) == BRW_CONDITIONAL_NONE &&
          brw_inst_src0_reg_type(brw, inst) == BRW_HW_REG_TYPE_D &&
          brw_inst_dst_reg_type(brw, inst) == BRW_HW_REG_TYPE_D) {
         brw_inst_set_src0_reg_type(brw, inst, BRW_HW_REG_TYPE_UD);
         brw_inst_set_dst_reg_type(brw, inst, BRW_HW_REG_TYPE_UD);
      }
   } else {
      if (reg.address_mode == BRW_ADDRESS_DIRECT) {
         brw_inst_set_src0_da_reg_nr(brw, inst, reg.nr);
         if (brw_inst_access_mode(brw, inst) == BRW_ALIGN_1) {
             brw_inst_set_src0_da1_subreg_nr(brw, inst, reg.subnr);
	 } else {
            brw_inst_set_src0_da16_subreg_nr(brw, inst, reg.subnr / 16);
	 }
      } else {
         brw_inst_set_src0_ia_subreg_nr(brw, inst, reg.subnr);

         if (brw_inst_access_mode(brw, inst) == BRW_ALIGN_1) {
            brw_inst_set_src0_ia1_addr_imm(brw, inst, reg.dw1.bits.indirect_offset);
	 } else {
            brw_inst_set_src0_ia_subreg_nr(brw, inst, reg.dw1.bits.indirect_offset);
	 }
      }

      if (brw_inst_access_mode(brw, inst) == BRW_ALIGN_1) {
	 if (reg.width == BRW_WIDTH_1 &&
             brw_inst_exec_size(brw, inst) == BRW_EXECUTE_1) {
            brw_inst_set_src0_hstride(brw, inst, BRW_HORIZONTAL_STRIDE_0);
            brw_inst_set_src0_width(brw, inst, BRW_WIDTH_1);
            brw_inst_set_src0_vstride(brw, inst, BRW_VERTICAL_STRIDE_0);
	 } else {
            brw_inst_set_src0_hstride(brw, inst, reg.hstride);
            brw_inst_set_src0_width(brw, inst, reg.width);
            brw_inst_set_src0_vstride(brw, inst, reg.vstride);
	 }
      } else {
         brw_inst_set_src0_da16_swiz_x(brw, inst,
            BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_X));
         brw_inst_set_src0_da16_swiz_y(brw, inst,
            BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_Y));
         brw_inst_set_src0_da16_swiz_z(brw, inst,
            BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_Z));
         brw_inst_set_src0_da16_swiz_w(brw, inst,
            BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_W));

	 /* This is an oddity of the fact we're using the same
	  * descriptions for registers in align_16 as align_1:
	  */
	 if (reg.vstride == BRW_VERTICAL_STRIDE_8)
            brw_inst_set_src0_vstride(brw, inst, BRW_VERTICAL_STRIDE_4);
	 else
            brw_inst_set_src0_vstride(brw, inst, reg.vstride);
      }
   }
}


void
brw_set_src1(struct brw_compile *p, brw_inst *inst, struct brw_reg reg)
{
   const struct brw_context *brw = p->brw;
   assert(reg.file != BRW_MESSAGE_REGISTER_FILE);

   if (reg.file != BRW_ARCHITECTURE_REGISTER_FILE)
      assert(reg.nr < 128);

   gen7_convert_mrf_to_grf(p, &reg);

   validate_reg(brw, inst, reg);

   brw_inst_set_src1_reg_file(brw, inst, reg.file);
   brw_inst_set_src1_reg_type(brw, inst,
                              brw_reg_type_to_hw_type(brw, reg.type, reg.file));
   brw_inst_set_src1_abs(brw, inst, reg.abs);
   brw_inst_set_src1_negate(brw, inst, reg.negate);

   /* Only src1 can be immediate in two-argument instructions.
    */
   assert(brw_inst_src0_reg_file(brw, inst) != BRW_IMMEDIATE_VALUE);

   if (reg.file == BRW_IMMEDIATE_VALUE) {
      brw_inst_set_imm_ud(brw, inst, reg.dw1.ud);
   } else {
      /* This is a hardware restriction, which may or may not be lifted
       * in the future:
       */
      assert (reg.address_mode == BRW_ADDRESS_DIRECT);
      /* assert (reg.file == BRW_GENERAL_REGISTER_FILE); */

      brw_inst_set_src1_da_reg_nr(brw, inst, reg.nr);
      if (brw_inst_access_mode(brw, inst) == BRW_ALIGN_1) {
         brw_inst_set_src1_da1_subreg_nr(brw, inst, reg.subnr);
      } else {
         brw_inst_set_src1_da16_subreg_nr(brw, inst, reg.subnr / 16);
      }

      if (brw_inst_access_mode(brw, inst) == BRW_ALIGN_1) {
	 if (reg.width == BRW_WIDTH_1 &&
             brw_inst_exec_size(brw, inst) == BRW_EXECUTE_1) {
            brw_inst_set_src1_hstride(brw, inst, BRW_HORIZONTAL_STRIDE_0);
            brw_inst_set_src1_width(brw, inst, BRW_WIDTH_1);
            brw_inst_set_src1_vstride(brw, inst, BRW_VERTICAL_STRIDE_0);
	 } else {
            brw_inst_set_src1_hstride(brw, inst, reg.hstride);
            brw_inst_set_src1_width(brw, inst, reg.width);
            brw_inst_set_src1_vstride(brw, inst, reg.vstride);
	 }
      } else {
         brw_inst_set_src1_da16_swiz_x(brw, inst,
            BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_X));
         brw_inst_set_src1_da16_swiz_y(brw, inst,
            BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_Y));
         brw_inst_set_src1_da16_swiz_z(brw, inst,
            BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_Z));
         brw_inst_set_src1_da16_swiz_w(brw, inst,
            BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_W));

	 /* This is an oddity of the fact we're using the same
	  * descriptions for registers in align_16 as align_1:
	  */
	 if (reg.vstride == BRW_VERTICAL_STRIDE_8)
            brw_inst_set_src1_vstride(brw, inst, BRW_VERTICAL_STRIDE_4);
	 else
            brw_inst_set_src1_vstride(brw, inst, reg.vstride);
      }
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
brw_set_message_descriptor(struct brw_compile *p,
			   brw_inst *inst,
			   enum brw_message_target sfid,
			   unsigned msg_length,
			   unsigned response_length,
			   bool header_present,
			   bool end_of_thread)
{
   struct brw_context *brw = p->brw;

   brw_set_src1(p, inst, brw_imm_d(0));

   /* For indirect sends, `inst` will not be the SEND/SENDC instruction
    * itself; instead, it will be a MOV/OR into the address register.
    *
    * In this case, we avoid setting the extended message descriptor bits,
    * since they go on the later SEND/SENDC instead and if set here would
    * instead clobber the conditionalmod bits.
    */
   unsigned opcode = brw_inst_opcode(brw, inst);
   if (opcode == BRW_OPCODE_SEND || opcode == BRW_OPCODE_SENDC) {
      brw_inst_set_sfid(brw, inst, sfid);
   }

   brw_inst_set_mlen(brw, inst, msg_length);
   brw_inst_set_rlen(brw, inst, response_length);
   brw_inst_set_eot(brw, inst, end_of_thread);

   if (brw->gen >= 5) {
      brw_inst_set_header_present(brw, inst, header_present);
   }
}

static void brw_set_math_message( struct brw_compile *p,
				  brw_inst *inst,
				  unsigned function,
				  unsigned integer_type,
				  bool low_precision,
				  unsigned dataType )
{
   struct brw_context *brw = p->brw;
   unsigned msg_length;
   unsigned response_length;

   /* Infer message length from the function */
   switch (function) {
   case BRW_MATH_FUNCTION_POW:
   case BRW_MATH_FUNCTION_INT_DIV_QUOTIENT:
   case BRW_MATH_FUNCTION_INT_DIV_REMAINDER:
   case BRW_MATH_FUNCTION_INT_DIV_QUOTIENT_AND_REMAINDER:
      msg_length = 2;
      break;
   default:
      msg_length = 1;
      break;
   }

   /* Infer response length from the function */
   switch (function) {
   case BRW_MATH_FUNCTION_SINCOS:
   case BRW_MATH_FUNCTION_INT_DIV_QUOTIENT_AND_REMAINDER:
      response_length = 2;
      break;
   default:
      response_length = 1;
      break;
   }


   brw_set_message_descriptor(p, inst, BRW_SFID_MATH,
			      msg_length, response_length, false, false);
   brw_inst_set_math_msg_function(brw, inst, function);
   brw_inst_set_math_msg_signed_int(brw, inst, integer_type);
   brw_inst_set_math_msg_precision(brw, inst, low_precision);
   brw_inst_set_math_msg_saturate(brw, inst, brw_inst_saturate(brw, inst));
   brw_inst_set_math_msg_data_type(brw, inst, dataType);
   brw_inst_set_saturate(brw, inst, 0);
}


static void brw_set_ff_sync_message(struct brw_compile *p,
				    brw_inst *insn,
				    bool allocate,
				    unsigned response_length,
				    bool end_of_thread)
{
   const struct brw_context *brw = p->brw;

   brw_set_message_descriptor(p, insn, BRW_SFID_URB,
			      1, response_length, true, end_of_thread);
   brw_inst_set_urb_opcode(brw, insn, 1); /* FF_SYNC */
   brw_inst_set_urb_allocate(brw, insn, allocate);
   /* The following fields are not used by FF_SYNC: */
   brw_inst_set_urb_global_offset(brw, insn, 0);
   brw_inst_set_urb_swizzle_control(brw, insn, 0);
   brw_inst_set_urb_used(brw, insn, 0);
   brw_inst_set_urb_complete(brw, insn, 0);
}

static void brw_set_urb_message( struct brw_compile *p,
				 brw_inst *insn,
                                 enum brw_urb_write_flags flags,
				 unsigned msg_length,
				 unsigned response_length,
				 unsigned offset,
				 unsigned swizzle_control )
{
   struct brw_context *brw = p->brw;

   assert(brw->gen < 7 || swizzle_control != BRW_URB_SWIZZLE_TRANSPOSE);
   assert(brw->gen < 7 || !(flags & BRW_URB_WRITE_ALLOCATE));
   assert(brw->gen >= 7 || !(flags & BRW_URB_WRITE_PER_SLOT_OFFSET));

   brw_set_message_descriptor(p, insn, BRW_SFID_URB,
			      msg_length, response_length, true,
                              flags & BRW_URB_WRITE_EOT);

   if (flags & BRW_URB_WRITE_OWORD) {
      assert(msg_length == 2); /* header + one OWORD of data */
      brw_inst_set_urb_opcode(brw, insn, BRW_URB_OPCODE_WRITE_OWORD);
   } else {
      brw_inst_set_urb_opcode(brw, insn, BRW_URB_OPCODE_WRITE_HWORD);
   }

   brw_inst_set_urb_global_offset(brw, insn, offset);
   brw_inst_set_urb_swizzle_control(brw, insn, swizzle_control);

   if (brw->gen < 8) {
      brw_inst_set_urb_complete(brw, insn, !!(flags & BRW_URB_WRITE_COMPLETE));
   }

   if (brw->gen < 7) {
      brw_inst_set_urb_allocate(brw, insn, !!(flags & BRW_URB_WRITE_ALLOCATE));
      brw_inst_set_urb_used(brw, insn, !(flags & BRW_URB_WRITE_UNUSED));
   } else {
      brw_inst_set_urb_per_slot_offset(brw, insn,
         !!(flags & BRW_URB_WRITE_PER_SLOT_OFFSET));
   }
}

void
brw_set_dp_write_message(struct brw_compile *p,
			 brw_inst *insn,
			 unsigned binding_table_index,
			 unsigned msg_control,
			 unsigned msg_type,
			 unsigned msg_length,
			 bool header_present,
			 unsigned last_render_target,
			 unsigned response_length,
			 unsigned end_of_thread,
			 unsigned send_commit_msg)
{
   struct brw_context *brw = p->brw;
   unsigned sfid;

   if (brw->gen >= 7) {
      /* Use the Render Cache for RT writes; otherwise use the Data Cache */
      if (msg_type == GEN6_DATAPORT_WRITE_MESSAGE_RENDER_TARGET_WRITE)
	 sfid = GEN6_SFID_DATAPORT_RENDER_CACHE;
      else
	 sfid = GEN7_SFID_DATAPORT_DATA_CACHE;
   } else if (brw->gen == 6) {
      /* Use the render cache for all write messages. */
      sfid = GEN6_SFID_DATAPORT_RENDER_CACHE;
   } else {
      sfid = BRW_SFID_DATAPORT_WRITE;
   }

   brw_set_message_descriptor(p, insn, sfid, msg_length, response_length,
			      header_present, end_of_thread);

   brw_inst_set_binding_table_index(brw, insn, binding_table_index);
   brw_inst_set_dp_write_msg_type(brw, insn, msg_type);
   brw_inst_set_dp_write_msg_control(brw, insn, msg_control);
   brw_inst_set_rt_last(brw, insn, last_render_target);
   if (brw->gen < 7) {
      brw_inst_set_dp_write_commit(brw, insn, send_commit_msg);
   }
}

void
brw_set_dp_read_message(struct brw_compile *p,
			brw_inst *insn,
			unsigned binding_table_index,
			unsigned msg_control,
			unsigned msg_type,
			unsigned target_cache,
			unsigned msg_length,
                        bool header_present,
			unsigned response_length)
{
   struct brw_context *brw = p->brw;
   unsigned sfid;

   if (brw->gen >= 7) {
      sfid = GEN7_SFID_DATAPORT_DATA_CACHE;
   } else if (brw->gen == 6) {
      if (target_cache == BRW_DATAPORT_READ_TARGET_RENDER_CACHE)
	 sfid = GEN6_SFID_DATAPORT_RENDER_CACHE;
      else
	 sfid = GEN6_SFID_DATAPORT_SAMPLER_CACHE;
   } else {
      sfid = BRW_SFID_DATAPORT_READ;
   }

   brw_set_message_descriptor(p, insn, sfid, msg_length, response_length,
			      header_present, false);

   brw_inst_set_binding_table_index(brw, insn, binding_table_index);
   brw_inst_set_dp_read_msg_type(brw, insn, msg_type);
   brw_inst_set_dp_read_msg_control(brw, insn, msg_control);
   if (brw->gen < 6)
      brw_inst_set_dp_read_target_cache(brw, insn, target_cache);
}

void
brw_set_sampler_message(struct brw_compile *p,
                        brw_inst *inst,
                        unsigned binding_table_index,
                        unsigned sampler,
                        unsigned msg_type,
                        unsigned response_length,
                        unsigned msg_length,
                        unsigned header_present,
                        unsigned simd_mode,
                        unsigned return_format)
{
   struct brw_context *brw = p->brw;

   brw_set_message_descriptor(p, inst, BRW_SFID_SAMPLER, msg_length,
			      response_length, header_present, false);

   brw_inst_set_binding_table_index(brw, inst, binding_table_index);
   brw_inst_set_sampler(brw, inst, sampler);
   brw_inst_set_sampler_msg_type(brw, inst, msg_type);
   if (brw->gen >= 5) {
      brw_inst_set_sampler_simd_mode(brw, inst, simd_mode);
   } else if (brw->gen == 4 && !brw->is_g4x) {
      brw_inst_set_sampler_return_format(brw, inst, return_format);
   }
}

void brw_set_indirect_send_descriptor(struct brw_compile *p,
                                      brw_inst *insn,
                                      unsigned sfid,
                                      struct brw_reg descriptor)
{
   /* Only a0.0 may be used as SEND's descriptor operand. */
   assert(descriptor.file == BRW_ARCHITECTURE_REGISTER_FILE);
   assert(descriptor.type == BRW_REGISTER_TYPE_UD);
   assert(descriptor.nr == BRW_ARF_ADDRESS);
   assert(descriptor.subnr == 0);

   brw_set_message_descriptor(p, insn, sfid, 0, 0, false, false);
   brw_set_src1(p, insn, descriptor);
}

static void
gen7_set_dp_scratch_message(struct brw_compile *p,
                            brw_inst *inst,
                            bool write,
                            bool dword,
                            bool invalidate_after_read,
                            unsigned num_regs,
                            unsigned addr_offset,
                            unsigned mlen,
                            unsigned rlen,
                            bool header_present)
{
   const struct brw_context *brw = p->brw;
   assert(num_regs == 1 || num_regs == 2 || num_regs == 4 ||
          (brw->gen >= 8 && num_regs == 8));
   brw_set_message_descriptor(p, inst, GEN7_SFID_DATAPORT_DATA_CACHE,
                              mlen, rlen, header_present, false);
   brw_inst_set_dp_category(brw, inst, 1); /* Scratch Block Read/Write msgs */
   brw_inst_set_scratch_read_write(brw, inst, write);
   brw_inst_set_scratch_type(brw, inst, dword);
   brw_inst_set_scratch_invalidate_after_read(brw, inst, invalidate_after_read);
   brw_inst_set_scratch_block_size(brw, inst, ffs(num_regs) - 1);
   brw_inst_set_scratch_addr_offset(brw, inst, addr_offset);
}

#define next_insn brw_next_insn
brw_inst *
brw_next_insn(struct brw_compile *p, unsigned opcode)
{
   const struct brw_context *brw = p->brw;
   brw_inst *insn;

   if (p->nr_insn + 1 > p->store_size) {
      p->store_size <<= 1;
      p->store = reralloc(p->mem_ctx, p->store, brw_inst, p->store_size);
   }

   p->next_insn_offset += 16;
   insn = &p->store[p->nr_insn++];
   memcpy(insn, p->current, sizeof(*insn));

   brw_inst_set_opcode(brw, insn, opcode);
   return insn;
}

static brw_inst *
brw_alu1(struct brw_compile *p, unsigned opcode,
         struct brw_reg dest, struct brw_reg src)
{
   brw_inst *insn = next_insn(p, opcode);
   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src);
   return insn;
}

static brw_inst *
brw_alu2(struct brw_compile *p, unsigned opcode,
         struct brw_reg dest, struct brw_reg src0, struct brw_reg src1)
{
   brw_inst *insn = next_insn(p, opcode);
   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src0);
   brw_set_src1(p, insn, src1);
   return insn;
}

static int
get_3src_subreg_nr(struct brw_reg reg)
{
   if (reg.vstride == BRW_VERTICAL_STRIDE_0) {
      assert(brw_is_single_value_swizzle(reg.dw1.bits.swizzle));
      return reg.subnr / 4 + BRW_GET_SWZ(reg.dw1.bits.swizzle, 0);
   } else {
      return reg.subnr / 4;
   }
}

static brw_inst *
brw_alu3(struct brw_compile *p, unsigned opcode, struct brw_reg dest,
         struct brw_reg src0, struct brw_reg src1, struct brw_reg src2)
{
   struct brw_context *brw = p->brw;
   brw_inst *inst = next_insn(p, opcode);

   gen7_convert_mrf_to_grf(p, &dest);

   assert(brw_inst_access_mode(brw, inst) == BRW_ALIGN_16);

   assert(dest.file == BRW_GENERAL_REGISTER_FILE ||
	  dest.file == BRW_MESSAGE_REGISTER_FILE);
   assert(dest.nr < 128);
   assert(dest.address_mode == BRW_ADDRESS_DIRECT);
   assert(dest.type == BRW_REGISTER_TYPE_F ||
          dest.type == BRW_REGISTER_TYPE_D ||
          dest.type == BRW_REGISTER_TYPE_UD);
   if (brw->gen == 6) {
      brw_inst_set_3src_dst_reg_file(brw, inst,
                                     dest.file == BRW_MESSAGE_REGISTER_FILE);
   }
   brw_inst_set_3src_dst_reg_nr(brw, inst, dest.nr);
   brw_inst_set_3src_dst_subreg_nr(brw, inst, dest.subnr / 16);
   brw_inst_set_3src_dst_writemask(brw, inst, dest.dw1.bits.writemask);
   guess_execution_size(p, inst, dest);

   assert(src0.file == BRW_GENERAL_REGISTER_FILE);
   assert(src0.address_mode == BRW_ADDRESS_DIRECT);
   assert(src0.nr < 128);
   brw_inst_set_3src_src0_swizzle(brw, inst, src0.dw1.bits.swizzle);
   brw_inst_set_3src_src0_subreg_nr(brw, inst, get_3src_subreg_nr(src0));
   brw_inst_set_3src_src0_reg_nr(brw, inst, src0.nr);
   brw_inst_set_3src_src0_abs(brw, inst, src0.abs);
   brw_inst_set_3src_src0_negate(brw, inst, src0.negate);
   brw_inst_set_3src_src0_rep_ctrl(brw, inst,
                                   src0.vstride == BRW_VERTICAL_STRIDE_0);

   assert(src1.file == BRW_GENERAL_REGISTER_FILE);
   assert(src1.address_mode == BRW_ADDRESS_DIRECT);
   assert(src1.nr < 128);
   brw_inst_set_3src_src1_swizzle(brw, inst, src1.dw1.bits.swizzle);
   brw_inst_set_3src_src1_subreg_nr(brw, inst, get_3src_subreg_nr(src1));
   brw_inst_set_3src_src1_reg_nr(brw, inst, src1.nr);
   brw_inst_set_3src_src1_abs(brw, inst, src1.abs);
   brw_inst_set_3src_src1_negate(brw, inst, src1.negate);
   brw_inst_set_3src_src1_rep_ctrl(brw, inst,
                                   src1.vstride == BRW_VERTICAL_STRIDE_0);

   assert(src2.file == BRW_GENERAL_REGISTER_FILE);
   assert(src2.address_mode == BRW_ADDRESS_DIRECT);
   assert(src2.nr < 128);
   brw_inst_set_3src_src2_swizzle(brw, inst, src2.dw1.bits.swizzle);
   brw_inst_set_3src_src2_subreg_nr(brw, inst, get_3src_subreg_nr(src2));
   brw_inst_set_3src_src2_reg_nr(brw, inst, src2.nr);
   brw_inst_set_3src_src2_abs(brw, inst, src2.abs);
   brw_inst_set_3src_src2_negate(brw, inst, src2.negate);
   brw_inst_set_3src_src2_rep_ctrl(brw, inst,
                                   src2.vstride == BRW_VERTICAL_STRIDE_0);

   if (brw->gen >= 7) {
      /* Set both the source and destination types based on dest.type,
       * ignoring the source register types.  The MAD and LRP emitters ensure
       * that all four types are float.  The BFE and BFI2 emitters, however,
       * may send us mixed D and UD types and want us to ignore that and use
       * the destination type.
       */
      switch (dest.type) {
      case BRW_REGISTER_TYPE_F:
         brw_inst_set_3src_src_type(brw, inst, BRW_3SRC_TYPE_F);
         brw_inst_set_3src_dst_type(brw, inst, BRW_3SRC_TYPE_F);
         break;
      case BRW_REGISTER_TYPE_D:
         brw_inst_set_3src_src_type(brw, inst, BRW_3SRC_TYPE_D);
         brw_inst_set_3src_dst_type(brw, inst, BRW_3SRC_TYPE_D);
         break;
      case BRW_REGISTER_TYPE_UD:
         brw_inst_set_3src_src_type(brw, inst, BRW_3SRC_TYPE_UD);
         brw_inst_set_3src_dst_type(brw, inst, BRW_3SRC_TYPE_UD);
         break;
      }
   }

   return inst;
}


/***********************************************************************
 * Convenience routines.
 */
#define ALU1(OP)					\
brw_inst *brw_##OP(struct brw_compile *p,		\
	      struct brw_reg dest,			\
	      struct brw_reg src0)   			\
{							\
   return brw_alu1(p, BRW_OPCODE_##OP, dest, src0);    	\
}

#define ALU2(OP)					\
brw_inst *brw_##OP(struct brw_compile *p,		\
	      struct brw_reg dest,			\
	      struct brw_reg src0,			\
	      struct brw_reg src1)   			\
{							\
   return brw_alu2(p, BRW_OPCODE_##OP, dest, src0, src1);	\
}

#define ALU3(OP)					\
brw_inst *brw_##OP(struct brw_compile *p,		\
	      struct brw_reg dest,			\
	      struct brw_reg src0,			\
	      struct brw_reg src1,			\
	      struct brw_reg src2)   			\
{							\
   return brw_alu3(p, BRW_OPCODE_##OP, dest, src0, src1, src2);	\
}

#define ALU3F(OP)                                               \
brw_inst *brw_##OP(struct brw_compile *p,         \
                                 struct brw_reg dest,           \
                                 struct brw_reg src0,           \
                                 struct brw_reg src1,           \
                                 struct brw_reg src2)           \
{                                                               \
   assert(dest.type == BRW_REGISTER_TYPE_F);                    \
   assert(src0.type == BRW_REGISTER_TYPE_F);                    \
   assert(src1.type == BRW_REGISTER_TYPE_F);                    \
   assert(src2.type == BRW_REGISTER_TYPE_F);                    \
   return brw_alu3(p, BRW_OPCODE_##OP, dest, src0, src1, src2); \
}

/* Rounding operations (other than RNDD) require two instructions - the first
 * stores a rounded value (possibly the wrong way) in the dest register, but
 * also sets a per-channel "increment bit" in the flag register.  A predicated
 * add of 1.0 fixes dest to contain the desired result.
 *
 * Sandybridge and later appear to round correctly without an ADD.
 */
#define ROUND(OP)							      \
void brw_##OP(struct brw_compile *p,					      \
	      struct brw_reg dest,					      \
	      struct brw_reg src)					      \
{									      \
   struct brw_context *brw = p->brw;					      \
   brw_inst *rnd, *add;							      \
   rnd = next_insn(p, BRW_OPCODE_##OP);					      \
   brw_set_dest(p, rnd, dest);						      \
   brw_set_src0(p, rnd, src);						      \
									      \
   if (brw->gen < 6) {							      \
      /* turn on round-increments */					      \
      brw_inst_set_cond_modifier(brw, rnd, BRW_CONDITIONAL_R);                \
      add = brw_ADD(p, dest, dest, brw_imm_f(1.0f));			      \
      brw_inst_set_pred_control(brw, add, BRW_PREDICATE_NORMAL);              \
   }									      \
}


ALU1(MOV)
ALU2(SEL)
ALU1(NOT)
ALU2(AND)
ALU2(OR)
ALU2(XOR)
ALU2(SHR)
ALU2(SHL)
ALU2(ASR)
ALU1(FRC)
ALU1(RNDD)
ALU2(MAC)
ALU2(MACH)
ALU1(LZD)
ALU2(DP4)
ALU2(DPH)
ALU2(DP3)
ALU2(DP2)
ALU2(LINE)
ALU2(PLN)
ALU3F(MAD)
ALU3F(LRP)
ALU1(BFREV)
ALU3(BFE)
ALU2(BFI1)
ALU3(BFI2)
ALU1(FBH)
ALU1(FBL)
ALU1(CBIT)
ALU2(ADDC)
ALU2(SUBB)

ROUND(RNDZ)
ROUND(RNDE)


brw_inst *
brw_ADD(struct brw_compile *p, struct brw_reg dest,
        struct brw_reg src0, struct brw_reg src1)
{
   /* 6.2.2: add */
   if (src0.type == BRW_REGISTER_TYPE_F ||
       (src0.file == BRW_IMMEDIATE_VALUE &&
	src0.type == BRW_REGISTER_TYPE_VF)) {
      assert(src1.type != BRW_REGISTER_TYPE_UD);
      assert(src1.type != BRW_REGISTER_TYPE_D);
   }

   if (src1.type == BRW_REGISTER_TYPE_F ||
       (src1.file == BRW_IMMEDIATE_VALUE &&
	src1.type == BRW_REGISTER_TYPE_VF)) {
      assert(src0.type != BRW_REGISTER_TYPE_UD);
      assert(src0.type != BRW_REGISTER_TYPE_D);
   }

   return brw_alu2(p, BRW_OPCODE_ADD, dest, src0, src1);
}

brw_inst *
brw_AVG(struct brw_compile *p, struct brw_reg dest,
        struct brw_reg src0, struct brw_reg src1)
{
   assert(dest.type == src0.type);
   assert(src0.type == src1.type);
   switch (src0.type) {
   case BRW_REGISTER_TYPE_B:
   case BRW_REGISTER_TYPE_UB:
   case BRW_REGISTER_TYPE_W:
   case BRW_REGISTER_TYPE_UW:
   case BRW_REGISTER_TYPE_D:
   case BRW_REGISTER_TYPE_UD:
      break;
   default:
      unreachable("Bad type for brw_AVG");
   }

   return brw_alu2(p, BRW_OPCODE_AVG, dest, src0, src1);
}

brw_inst *
brw_MUL(struct brw_compile *p, struct brw_reg dest,
        struct brw_reg src0, struct brw_reg src1)
{
   /* 6.32.38: mul */
   if (src0.type == BRW_REGISTER_TYPE_D ||
       src0.type == BRW_REGISTER_TYPE_UD ||
       src1.type == BRW_REGISTER_TYPE_D ||
       src1.type == BRW_REGISTER_TYPE_UD) {
      assert(dest.type != BRW_REGISTER_TYPE_F);
   }

   if (src0.type == BRW_REGISTER_TYPE_F ||
       (src0.file == BRW_IMMEDIATE_VALUE &&
	src0.type == BRW_REGISTER_TYPE_VF)) {
      assert(src1.type != BRW_REGISTER_TYPE_UD);
      assert(src1.type != BRW_REGISTER_TYPE_D);
   }

   if (src1.type == BRW_REGISTER_TYPE_F ||
       (src1.file == BRW_IMMEDIATE_VALUE &&
	src1.type == BRW_REGISTER_TYPE_VF)) {
      assert(src0.type != BRW_REGISTER_TYPE_UD);
      assert(src0.type != BRW_REGISTER_TYPE_D);
   }

   assert(src0.file != BRW_ARCHITECTURE_REGISTER_FILE ||
	  src0.nr != BRW_ARF_ACCUMULATOR);
   assert(src1.file != BRW_ARCHITECTURE_REGISTER_FILE ||
	  src1.nr != BRW_ARF_ACCUMULATOR);

   return brw_alu2(p, BRW_OPCODE_MUL, dest, src0, src1);
}

brw_inst *
brw_F32TO16(struct brw_compile *p, struct brw_reg dst, struct brw_reg src)
{
   const struct brw_context *brw = p->brw;
   bool align16 = brw_inst_access_mode(brw, p->current) == BRW_ALIGN_16;

   if (align16) {
      assert(dst.type == BRW_REGISTER_TYPE_UD);
   } else {
      assert(dst.type == BRW_REGISTER_TYPE_W ||
             dst.type == BRW_REGISTER_TYPE_UW ||
             dst.type == BRW_REGISTER_TYPE_HF);
   }

   if (brw->gen >= 8) {
      if (align16) {
         /* Emulate the Gen7 zeroing bug (see comments in vec4_visitor's
          * emit_pack_half_2x16 method.)
          */
         brw_MOV(p, retype(dst, BRW_REGISTER_TYPE_UD), brw_imm_ud(0u));
      }
      return brw_MOV(p, retype(dst, BRW_REGISTER_TYPE_HF), src);
   } else {
      assert(brw->gen == 7);
      return brw_alu1(p, BRW_OPCODE_F32TO16, dst, src);
   }
}

brw_inst *
brw_F16TO32(struct brw_compile *p, struct brw_reg dst, struct brw_reg src)
{
   const struct brw_context *brw = p->brw;
   bool align16 = brw_inst_access_mode(brw, p->current) == BRW_ALIGN_16;

   if (align16) {
      assert(src.type == BRW_REGISTER_TYPE_UD);
   } else {
      assert(src.type == BRW_REGISTER_TYPE_W ||
             src.type == BRW_REGISTER_TYPE_UW ||
             src.type == BRW_REGISTER_TYPE_HF);
   }

   if (brw->gen >= 8) {
      return brw_MOV(p, dst, retype(src, BRW_REGISTER_TYPE_HF));
   } else {
      assert(brw->gen == 7);
      return brw_alu1(p, BRW_OPCODE_F16TO32, dst, src);
   }
}


void brw_NOP(struct brw_compile *p)
{
   brw_inst *insn = next_insn(p, BRW_OPCODE_NOP);
   brw_set_dest(p, insn, retype(brw_vec4_grf(0,0), BRW_REGISTER_TYPE_UD));
   brw_set_src0(p, insn, retype(brw_vec4_grf(0,0), BRW_REGISTER_TYPE_UD));
   brw_set_src1(p, insn, brw_imm_ud(0x0));
}





/***********************************************************************
 * Comparisons, if/else/endif
 */

brw_inst *
brw_JMPI(struct brw_compile *p, struct brw_reg index,
         unsigned predicate_control)
{
   const struct brw_context *brw = p->brw;
   struct brw_reg ip = brw_ip_reg();
   brw_inst *inst = brw_alu2(p, BRW_OPCODE_JMPI, ip, ip, index);

   brw_inst_set_exec_size(brw, inst, BRW_EXECUTE_2);
   brw_inst_set_qtr_control(brw, inst, BRW_COMPRESSION_NONE);
   brw_inst_set_mask_control(brw, inst, BRW_MASK_DISABLE);
   brw_inst_set_pred_control(brw, inst, predicate_control);

   return inst;
}

static void
push_if_stack(struct brw_compile *p, brw_inst *inst)
{
   p->if_stack[p->if_stack_depth] = inst - p->store;

   p->if_stack_depth++;
   if (p->if_stack_array_size <= p->if_stack_depth) {
      p->if_stack_array_size *= 2;
      p->if_stack = reralloc(p->mem_ctx, p->if_stack, int,
			     p->if_stack_array_size);
   }
}

static brw_inst *
pop_if_stack(struct brw_compile *p)
{
   p->if_stack_depth--;
   return &p->store[p->if_stack[p->if_stack_depth]];
}

static void
push_loop_stack(struct brw_compile *p, brw_inst *inst)
{
   if (p->loop_stack_array_size < p->loop_stack_depth) {
      p->loop_stack_array_size *= 2;
      p->loop_stack = reralloc(p->mem_ctx, p->loop_stack, int,
			       p->loop_stack_array_size);
      p->if_depth_in_loop = reralloc(p->mem_ctx, p->if_depth_in_loop, int,
				     p->loop_stack_array_size);
   }

   p->loop_stack[p->loop_stack_depth] = inst - p->store;
   p->loop_stack_depth++;
   p->if_depth_in_loop[p->loop_stack_depth] = 0;
}

static brw_inst *
get_inner_do_insn(struct brw_compile *p)
{
   return &p->store[p->loop_stack[p->loop_stack_depth - 1]];
}

/* EU takes the value from the flag register and pushes it onto some
 * sort of a stack (presumably merging with any flag value already on
 * the stack).  Within an if block, the flags at the top of the stack
 * control execution on each channel of the unit, eg. on each of the
 * 16 pixel values in our wm programs.
 *
 * When the matching 'else' instruction is reached (presumably by
 * countdown of the instruction count patched in by our ELSE/ENDIF
 * functions), the relevent flags are inverted.
 *
 * When the matching 'endif' instruction is reached, the flags are
 * popped off.  If the stack is now empty, normal execution resumes.
 */
brw_inst *
brw_IF(struct brw_compile *p, unsigned execute_size)
{
   struct brw_context *brw = p->brw;
   brw_inst *insn;

   insn = next_insn(p, BRW_OPCODE_IF);

   /* Override the defaults for this instruction:
    */
   if (brw->gen < 6) {
      brw_set_dest(p, insn, brw_ip_reg());
      brw_set_src0(p, insn, brw_ip_reg());
      brw_set_src1(p, insn, brw_imm_d(0x0));
   } else if (brw->gen == 6) {
      brw_set_dest(p, insn, brw_imm_w(0));
      brw_inst_set_gen6_jump_count(brw, insn, 0);
      brw_set_src0(p, insn, vec1(retype(brw_null_reg(), BRW_REGISTER_TYPE_D)));
      brw_set_src1(p, insn, vec1(retype(brw_null_reg(), BRW_REGISTER_TYPE_D)));
   } else if (brw->gen == 7) {
      brw_set_dest(p, insn, vec1(retype(brw_null_reg(), BRW_REGISTER_TYPE_D)));
      brw_set_src0(p, insn, vec1(retype(brw_null_reg(), BRW_REGISTER_TYPE_D)));
      brw_set_src1(p, insn, brw_imm_ud(0));
      brw_inst_set_jip(brw, insn, 0);
      brw_inst_set_uip(brw, insn, 0);
   } else {
      brw_set_dest(p, insn, vec1(retype(brw_null_reg(), BRW_REGISTER_TYPE_D)));
      brw_set_src0(p, insn, brw_imm_d(0));
      brw_inst_set_jip(brw, insn, 0);
      brw_inst_set_uip(brw, insn, 0);
   }

   brw_inst_set_exec_size(brw, insn, execute_size);
   brw_inst_set_qtr_control(brw, insn, BRW_COMPRESSION_NONE);
   brw_inst_set_pred_control(brw, insn, BRW_PREDICATE_NORMAL);
   brw_inst_set_mask_control(brw, insn, BRW_MASK_ENABLE);
   if (!p->single_program_flow && brw->gen < 6)
      brw_inst_set_thread_control(brw, insn, BRW_THREAD_SWITCH);

   push_if_stack(p, insn);
   p->if_depth_in_loop[p->loop_stack_depth]++;
   return insn;
}

/* This function is only used for gen6-style IF instructions with an
 * embedded comparison (conditional modifier).  It is not used on gen7.
 */
brw_inst *
gen6_IF(struct brw_compile *p, enum brw_conditional_mod conditional,
	struct brw_reg src0, struct brw_reg src1)
{
   const struct brw_context *brw = p->brw;
   brw_inst *insn;

   insn = next_insn(p, BRW_OPCODE_IF);

   brw_set_dest(p, insn, brw_imm_w(0));
   brw_inst_set_exec_size(brw, insn, p->compressed ? BRW_EXECUTE_16
                                                   : BRW_EXECUTE_8);
   brw_inst_set_gen6_jump_count(brw, insn, 0);
   brw_set_src0(p, insn, src0);
   brw_set_src1(p, insn, src1);

   assert(brw_inst_qtr_control(brw, insn) == BRW_COMPRESSION_NONE);
   assert(brw_inst_pred_control(brw, insn) == BRW_PREDICATE_NONE);
   brw_inst_set_cond_modifier(brw, insn, conditional);

   push_if_stack(p, insn);
   return insn;
}

/**
 * In single-program-flow (SPF) mode, convert IF and ELSE into ADDs.
 */
static void
convert_IF_ELSE_to_ADD(struct brw_compile *p,
                       brw_inst *if_inst, brw_inst *else_inst)
{
   const struct brw_context *brw = p->brw;

   /* The next instruction (where the ENDIF would be, if it existed) */
   brw_inst *next_inst = &p->store[p->nr_insn];

   assert(p->single_program_flow);
   assert(if_inst != NULL && brw_inst_opcode(brw, if_inst) == BRW_OPCODE_IF);
   assert(else_inst == NULL || brw_inst_opcode(brw, else_inst) == BRW_OPCODE_ELSE);
   assert(brw_inst_exec_size(brw, if_inst) == BRW_EXECUTE_1);

   /* Convert IF to an ADD instruction that moves the instruction pointer
    * to the first instruction of the ELSE block.  If there is no ELSE
    * block, point to where ENDIF would be.  Reverse the predicate.
    *
    * There's no need to execute an ENDIF since we don't need to do any
    * stack operations, and if we're currently executing, we just want to
    * continue normally.
    */
   brw_inst_set_opcode(brw, if_inst, BRW_OPCODE_ADD);
   brw_inst_set_pred_inv(brw, if_inst, true);

   if (else_inst != NULL) {
      /* Convert ELSE to an ADD instruction that points where the ENDIF
       * would be.
       */
      brw_inst_set_opcode(brw, else_inst, BRW_OPCODE_ADD);

      brw_inst_set_imm_ud(brw, if_inst, (else_inst - if_inst + 1) * 16);
      brw_inst_set_imm_ud(brw, else_inst, (next_inst - else_inst) * 16);
   } else {
      brw_inst_set_imm_ud(brw, if_inst, (next_inst - if_inst) * 16);
   }
}

/**
 * Patch IF and ELSE instructions with appropriate jump targets.
 */
static void
patch_IF_ELSE(struct brw_compile *p,
              brw_inst *if_inst, brw_inst *else_inst, brw_inst *endif_inst)
{
   struct brw_context *brw = p->brw;

   /* We shouldn't be patching IF and ELSE instructions in single program flow
    * mode when gen < 6, because in single program flow mode on those
    * platforms, we convert flow control instructions to conditional ADDs that
    * operate on IP (see brw_ENDIF).
    *
    * However, on Gen6, writing to IP doesn't work in single program flow mode
    * (see the SandyBridge PRM, Volume 4 part 2, p79: "When SPF is ON, IP may
    * not be updated by non-flow control instructions.").  And on later
    * platforms, there is no significant benefit to converting control flow
    * instructions to conditional ADDs.  So we do patch IF and ELSE
    * instructions in single program flow mode on those platforms.
    */
   if (brw->gen < 6)
      assert(!p->single_program_flow);

   assert(if_inst != NULL && brw_inst_opcode(brw, if_inst) == BRW_OPCODE_IF);
   assert(endif_inst != NULL);
   assert(else_inst == NULL || brw_inst_opcode(brw, else_inst) == BRW_OPCODE_ELSE);

   unsigned br = brw_jump_scale(brw);

   assert(brw_inst_opcode(brw, endif_inst) == BRW_OPCODE_ENDIF);
   brw_inst_set_exec_size(brw, endif_inst, brw_inst_exec_size(brw, if_inst));

   if (else_inst == NULL) {
      /* Patch IF -> ENDIF */
      if (brw->gen < 6) {
	 /* Turn it into an IFF, which means no mask stack operations for
	  * all-false and jumping past the ENDIF.
	  */
         brw_inst_set_opcode(brw, if_inst, BRW_OPCODE_IFF);
         brw_inst_set_gen4_jump_count(brw, if_inst,
                                      br * (endif_inst - if_inst + 1));
         brw_inst_set_gen4_pop_count(brw, if_inst, 0);
      } else if (brw->gen == 6) {
	 /* As of gen6, there is no IFF and IF must point to the ENDIF. */
         brw_inst_set_gen6_jump_count(brw, if_inst, br*(endif_inst - if_inst));
      } else {
         brw_inst_set_uip(brw, if_inst, br * (endif_inst - if_inst));
         brw_inst_set_jip(brw, if_inst, br * (endif_inst - if_inst));
      }
   } else {
      brw_inst_set_exec_size(brw, else_inst, brw_inst_exec_size(brw, if_inst));

      /* Patch IF -> ELSE */
      if (brw->gen < 6) {
         brw_inst_set_gen4_jump_count(brw, if_inst,
                                      br * (else_inst - if_inst));
         brw_inst_set_gen4_pop_count(brw, if_inst, 0);
      } else if (brw->gen == 6) {
         brw_inst_set_gen6_jump_count(brw, if_inst,
                                      br * (else_inst - if_inst + 1));
      }

      /* Patch ELSE -> ENDIF */
      if (brw->gen < 6) {
	 /* BRW_OPCODE_ELSE pre-gen6 should point just past the
	  * matching ENDIF.
	  */
         brw_inst_set_gen4_jump_count(brw, else_inst,
                                      br * (endif_inst - else_inst + 1));
         brw_inst_set_gen4_pop_count(brw, else_inst, 1);
      } else if (brw->gen == 6) {
	 /* BRW_OPCODE_ELSE on gen6 should point to the matching ENDIF. */
         brw_inst_set_gen6_jump_count(brw, else_inst,
                                      br * (endif_inst - else_inst));
      } else {
	 /* The IF instruction's JIP should point just past the ELSE */
         brw_inst_set_jip(brw, if_inst, br * (else_inst - if_inst + 1));
	 /* The IF instruction's UIP and ELSE's JIP should point to ENDIF */
         brw_inst_set_uip(brw, if_inst, br * (endif_inst - if_inst));
         brw_inst_set_jip(brw, else_inst, br * (endif_inst - else_inst));
         if (brw->gen >= 8) {
            /* Since we don't set branch_ctrl, the ELSE's JIP and UIP both
             * should point to ENDIF.
             */
            brw_inst_set_uip(brw, else_inst, br * (endif_inst - else_inst));
         }
      }
   }
}

void
brw_ELSE(struct brw_compile *p)
{
   struct brw_context *brw = p->brw;
   brw_inst *insn;

   insn = next_insn(p, BRW_OPCODE_ELSE);

   if (brw->gen < 6) {
      brw_set_dest(p, insn, brw_ip_reg());
      brw_set_src0(p, insn, brw_ip_reg());
      brw_set_src1(p, insn, brw_imm_d(0x0));
   } else if (brw->gen == 6) {
      brw_set_dest(p, insn, brw_imm_w(0));
      brw_inst_set_gen6_jump_count(brw, insn, 0);
      brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src1(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   } else if (brw->gen == 7) {
      brw_set_dest(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src1(p, insn, brw_imm_ud(0));
      brw_inst_set_jip(brw, insn, 0);
      brw_inst_set_uip(brw, insn, 0);
   } else {
      brw_set_dest(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src0(p, insn, brw_imm_d(0));
      brw_inst_set_jip(brw, insn, 0);
      brw_inst_set_uip(brw, insn, 0);
   }

   brw_inst_set_qtr_control(brw, insn, BRW_COMPRESSION_NONE);
   brw_inst_set_mask_control(brw, insn, BRW_MASK_ENABLE);
   if (!p->single_program_flow && brw->gen < 6)
      brw_inst_set_thread_control(brw, insn, BRW_THREAD_SWITCH);

   push_if_stack(p, insn);
}

void
brw_ENDIF(struct brw_compile *p)
{
   struct brw_context *brw = p->brw;
   brw_inst *insn = NULL;
   brw_inst *else_inst = NULL;
   brw_inst *if_inst = NULL;
   brw_inst *tmp;
   bool emit_endif = true;

   /* In single program flow mode, we can express IF and ELSE instructions
    * equivalently as ADD instructions that operate on IP.  On platforms prior
    * to Gen6, flow control instructions cause an implied thread switch, so
    * this is a significant savings.
    *
    * However, on Gen6, writing to IP doesn't work in single program flow mode
    * (see the SandyBridge PRM, Volume 4 part 2, p79: "When SPF is ON, IP may
    * not be updated by non-flow control instructions.").  And on later
    * platforms, there is no significant benefit to converting control flow
    * instructions to conditional ADDs.  So we only do this trick on Gen4 and
    * Gen5.
    */
   if (brw->gen < 6 && p->single_program_flow)
      emit_endif = false;

   /*
    * A single next_insn() may change the base adress of instruction store
    * memory(p->store), so call it first before referencing the instruction
    * store pointer from an index
    */
   if (emit_endif)
      insn = next_insn(p, BRW_OPCODE_ENDIF);

   /* Pop the IF and (optional) ELSE instructions from the stack */
   p->if_depth_in_loop[p->loop_stack_depth]--;
   tmp = pop_if_stack(p);
   if (brw_inst_opcode(brw, tmp) == BRW_OPCODE_ELSE) {
      else_inst = tmp;
      tmp = pop_if_stack(p);
   }
   if_inst = tmp;

   if (!emit_endif) {
      /* ENDIF is useless; don't bother emitting it. */
      convert_IF_ELSE_to_ADD(p, if_inst, else_inst);
      return;
   }

   if (brw->gen < 6) {
      brw_set_dest(p, insn, retype(brw_vec4_grf(0,0), BRW_REGISTER_TYPE_UD));
      brw_set_src0(p, insn, retype(brw_vec4_grf(0,0), BRW_REGISTER_TYPE_UD));
      brw_set_src1(p, insn, brw_imm_d(0x0));
   } else if (brw->gen == 6) {
      brw_set_dest(p, insn, brw_imm_w(0));
      brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src1(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   } else if (brw->gen == 7) {
      brw_set_dest(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src1(p, insn, brw_imm_ud(0));
   } else {
      brw_set_src0(p, insn, brw_imm_d(0));
   }

   brw_inst_set_qtr_control(brw, insn, BRW_COMPRESSION_NONE);
   brw_inst_set_mask_control(brw, insn, BRW_MASK_ENABLE);
   if (brw->gen < 6)
      brw_inst_set_thread_control(brw, insn, BRW_THREAD_SWITCH);

   /* Also pop item off the stack in the endif instruction: */
   if (brw->gen < 6) {
      brw_inst_set_gen4_jump_count(brw, insn, 0);
      brw_inst_set_gen4_pop_count(brw, insn, 1);
   } else if (brw->gen == 6) {
      brw_inst_set_gen6_jump_count(brw, insn, 2);
   } else {
      brw_inst_set_jip(brw, insn, 2);
   }
   patch_IF_ELSE(p, if_inst, else_inst, insn);
}

brw_inst *
brw_BREAK(struct brw_compile *p)
{
   struct brw_context *brw = p->brw;
   brw_inst *insn;

   insn = next_insn(p, BRW_OPCODE_BREAK);
   if (brw->gen >= 8) {
      brw_set_dest(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src0(p, insn, brw_imm_d(0x0));
   } else if (brw->gen >= 6) {
      brw_set_dest(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src1(p, insn, brw_imm_d(0x0));
   } else {
      brw_set_dest(p, insn, brw_ip_reg());
      brw_set_src0(p, insn, brw_ip_reg());
      brw_set_src1(p, insn, brw_imm_d(0x0));
      brw_inst_set_gen4_pop_count(brw, insn,
                                  p->if_depth_in_loop[p->loop_stack_depth]);
   }
   brw_inst_set_qtr_control(brw, insn, BRW_COMPRESSION_NONE);
   brw_inst_set_exec_size(brw, insn, p->compressed ? BRW_EXECUTE_16
                                                   : BRW_EXECUTE_8);

   return insn;
}

brw_inst *
brw_CONT(struct brw_compile *p)
{
   const struct brw_context *brw = p->brw;
   brw_inst *insn;

   insn = next_insn(p, BRW_OPCODE_CONTINUE);
   brw_set_dest(p, insn, brw_ip_reg());
   if (brw->gen >= 8) {
      brw_set_src0(p, insn, brw_imm_d(0x0));
   } else {
      brw_set_src0(p, insn, brw_ip_reg());
      brw_set_src1(p, insn, brw_imm_d(0x0));
   }

   if (brw->gen < 6) {
      brw_inst_set_gen4_pop_count(brw, insn,
                                  p->if_depth_in_loop[p->loop_stack_depth]);
   }
   brw_inst_set_qtr_control(brw, insn, BRW_COMPRESSION_NONE);
   brw_inst_set_exec_size(brw, insn, p->compressed ? BRW_EXECUTE_16
                                                   : BRW_EXECUTE_8);
   return insn;
}

brw_inst *
gen6_HALT(struct brw_compile *p)
{
   const struct brw_context *brw = p->brw;
   brw_inst *insn;

   insn = next_insn(p, BRW_OPCODE_HALT);
   brw_set_dest(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   if (brw->gen >= 8) {
      brw_set_src0(p, insn, brw_imm_d(0x0));
   } else {
      brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src1(p, insn, brw_imm_d(0x0)); /* UIP and JIP, updated later. */
   }

   if (p->compressed) {
      brw_inst_set_exec_size(brw, insn, BRW_EXECUTE_16);
   } else {
      brw_inst_set_qtr_control(brw, insn, BRW_COMPRESSION_NONE);
      brw_inst_set_exec_size(brw, insn, BRW_EXECUTE_8);
   }
   return insn;
}

/* DO/WHILE loop:
 *
 * The DO/WHILE is just an unterminated loop -- break or continue are
 * used for control within the loop.  We have a few ways they can be
 * done.
 *
 * For uniform control flow, the WHILE is just a jump, so ADD ip, ip,
 * jip and no DO instruction.
 *
 * For non-uniform control flow pre-gen6, there's a DO instruction to
 * push the mask, and a WHILE to jump back, and BREAK to get out and
 * pop the mask.
 *
 * For gen6, there's no more mask stack, so no need for DO.  WHILE
 * just points back to the first instruction of the loop.
 */
brw_inst *
brw_DO(struct brw_compile *p, unsigned execute_size)
{
   struct brw_context *brw = p->brw;

   if (brw->gen >= 6 || p->single_program_flow) {
      push_loop_stack(p, &p->store[p->nr_insn]);
      return &p->store[p->nr_insn];
   } else {
      brw_inst *insn = next_insn(p, BRW_OPCODE_DO);

      push_loop_stack(p, insn);

      /* Override the defaults for this instruction:
       */
      brw_set_dest(p, insn, brw_null_reg());
      brw_set_src0(p, insn, brw_null_reg());
      brw_set_src1(p, insn, brw_null_reg());

      brw_inst_set_qtr_control(brw, insn, BRW_COMPRESSION_NONE);
      brw_inst_set_exec_size(brw, insn, execute_size);
      brw_inst_set_pred_control(brw, insn, BRW_PREDICATE_NONE);

      return insn;
   }
}

/**
 * For pre-gen6, we patch BREAK/CONT instructions to point at the WHILE
 * instruction here.
 *
 * For gen6+, see brw_set_uip_jip(), which doesn't care so much about the loop
 * nesting, since it can always just point to the end of the block/current loop.
 */
static void
brw_patch_break_cont(struct brw_compile *p, brw_inst *while_inst)
{
   struct brw_context *brw = p->brw;
   brw_inst *do_inst = get_inner_do_insn(p);
   brw_inst *inst;
   unsigned br = brw_jump_scale(brw);

   assert(brw->gen < 6);

   for (inst = while_inst - 1; inst != do_inst; inst--) {
      /* If the jump count is != 0, that means that this instruction has already
       * been patched because it's part of a loop inside of the one we're
       * patching.
       */
      if (brw_inst_opcode(brw, inst) == BRW_OPCODE_BREAK &&
          brw_inst_gen4_jump_count(brw, inst) == 0) {
         brw_inst_set_gen4_jump_count(brw, inst, br*((while_inst - inst) + 1));
      } else if (brw_inst_opcode(brw, inst) == BRW_OPCODE_CONTINUE &&
                 brw_inst_gen4_jump_count(brw, inst) == 0) {
         brw_inst_set_gen4_jump_count(brw, inst, br * (while_inst - inst));
      }
   }
}

brw_inst *
brw_WHILE(struct brw_compile *p)
{
   struct brw_context *brw = p->brw;
   brw_inst *insn, *do_insn;
   unsigned br = brw_jump_scale(brw);

   if (brw->gen >= 6) {
      insn = next_insn(p, BRW_OPCODE_WHILE);
      do_insn = get_inner_do_insn(p);

      if (brw->gen >= 8) {
         brw_set_dest(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
         brw_set_src0(p, insn, brw_imm_d(0));
         brw_inst_set_jip(brw, insn, br * (do_insn - insn));
      } else if (brw->gen == 7) {
         brw_set_dest(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
         brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
         brw_set_src1(p, insn, brw_imm_ud(0));
         brw_inst_set_jip(brw, insn, br * (do_insn - insn));
      } else {
         brw_set_dest(p, insn, brw_imm_w(0));
         brw_inst_set_gen6_jump_count(brw, insn, br * (do_insn - insn));
         brw_set_src0(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
         brw_set_src1(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      }

      brw_inst_set_exec_size(brw, insn, p->compressed ? BRW_EXECUTE_16
                                                      : BRW_EXECUTE_8);
   } else {
      if (p->single_program_flow) {
	 insn = next_insn(p, BRW_OPCODE_ADD);
         do_insn = get_inner_do_insn(p);

	 brw_set_dest(p, insn, brw_ip_reg());
	 brw_set_src0(p, insn, brw_ip_reg());
	 brw_set_src1(p, insn, brw_imm_d((do_insn - insn) * 16));
         brw_inst_set_exec_size(brw, insn, BRW_EXECUTE_1);
      } else {
	 insn = next_insn(p, BRW_OPCODE_WHILE);
         do_insn = get_inner_do_insn(p);

         assert(brw_inst_opcode(brw, do_insn) == BRW_OPCODE_DO);

	 brw_set_dest(p, insn, brw_ip_reg());
	 brw_set_src0(p, insn, brw_ip_reg());
	 brw_set_src1(p, insn, brw_imm_d(0));

         brw_inst_set_exec_size(brw, insn, brw_inst_exec_size(brw, do_insn));
         brw_inst_set_gen4_jump_count(brw, insn, br * (do_insn - insn + 1));
         brw_inst_set_gen4_pop_count(brw, insn, 0);

	 brw_patch_break_cont(p, insn);
      }
   }
   brw_inst_set_qtr_control(brw, insn, BRW_COMPRESSION_NONE);

   p->loop_stack_depth--;

   return insn;
}

/* FORWARD JUMPS:
 */
void brw_land_fwd_jump(struct brw_compile *p, int jmp_insn_idx)
{
   struct brw_context *brw = p->brw;
   brw_inst *jmp_insn = &p->store[jmp_insn_idx];
   unsigned jmpi = 1;

   if (brw->gen >= 5)
      jmpi = 2;

   assert(brw_inst_opcode(brw, jmp_insn) == BRW_OPCODE_JMPI);
   assert(brw_inst_src1_reg_file(brw, jmp_insn) == BRW_IMMEDIATE_VALUE);

   brw_inst_set_gen4_jump_count(brw, jmp_insn,
                                jmpi * (p->nr_insn - jmp_insn_idx - 1));
}

/* To integrate with the above, it makes sense that the comparison
 * instruction should populate the flag register.  It might be simpler
 * just to use the flag reg for most WM tasks?
 */
void brw_CMP(struct brw_compile *p,
	     struct brw_reg dest,
	     unsigned conditional,
	     struct brw_reg src0,
	     struct brw_reg src1)
{
   struct brw_context *brw = p->brw;
   brw_inst *insn = next_insn(p, BRW_OPCODE_CMP);

   if (brw->gen >= 8) {
      /* The CMP instruction appears to behave erratically for floating point
       * sources unless the destination type is also float.  Overriding it to
       * match src0 makes it work in all cases.
       */
      dest.type = src0.type;
   }

   brw_inst_set_cond_modifier(brw, insn, conditional);
   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src0);
   brw_set_src1(p, insn, src1);

   /* Item WaCMPInstNullDstForcesThreadSwitch in the Haswell Bspec workarounds
    * page says:
    *    "Any CMP instruction with a null destination must use a {switch}."
    *
    * It also applies to other Gen7 platforms (IVB, BYT) even though it isn't
    * mentioned on their work-arounds pages.
    */
   if (brw->gen == 7) {
      if (dest.file == BRW_ARCHITECTURE_REGISTER_FILE &&
          dest.nr == BRW_ARF_NULL) {
         brw_inst_set_thread_control(brw, insn, BRW_THREAD_SWITCH);
      }
   }
}

/***********************************************************************
 * Helpers for the various SEND message types:
 */

/** Extended math function, float[8].
 */
void gen4_math(struct brw_compile *p,
	       struct brw_reg dest,
	       unsigned function,
	       unsigned msg_reg_nr,
	       struct brw_reg src,
	       unsigned data_type,
	       unsigned precision )
{
   struct brw_context *brw = p->brw;
   brw_inst *insn = next_insn(p, BRW_OPCODE_SEND);

   assert(brw->gen < 6);

   /* Example code doesn't set predicate_control for send
    * instructions.
    */
   brw_inst_set_pred_control(brw, insn, 0);
   brw_inst_set_base_mrf(brw, insn, msg_reg_nr);

   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src);
   brw_set_math_message(p,
                        insn,
                        function,
                        src.type == BRW_REGISTER_TYPE_D,
                        precision,
                        data_type);
}

void gen6_math(struct brw_compile *p,
	       struct brw_reg dest,
	       unsigned function,
	       struct brw_reg src0,
	       struct brw_reg src1)
{
   struct brw_context *brw = p->brw;
   brw_inst *insn = next_insn(p, BRW_OPCODE_MATH);

   assert(brw->gen >= 6);

   assert(dest.file == BRW_GENERAL_REGISTER_FILE ||
          (brw->gen >= 7 && dest.file == BRW_MESSAGE_REGISTER_FILE));
   assert(src0.file == BRW_GENERAL_REGISTER_FILE ||
          (brw->gen >= 8 && src0.file == BRW_IMMEDIATE_VALUE));

   assert(dest.hstride == BRW_HORIZONTAL_STRIDE_1);
   if (brw->gen == 6) {
      assert(src0.hstride == BRW_HORIZONTAL_STRIDE_1);
      assert(src1.hstride == BRW_HORIZONTAL_STRIDE_1);
   }

   if (function == BRW_MATH_FUNCTION_INT_DIV_QUOTIENT ||
       function == BRW_MATH_FUNCTION_INT_DIV_REMAINDER ||
       function == BRW_MATH_FUNCTION_INT_DIV_QUOTIENT_AND_REMAINDER) {
      assert(src0.type != BRW_REGISTER_TYPE_F);
      assert(src1.type != BRW_REGISTER_TYPE_F);
      assert(src1.file == BRW_GENERAL_REGISTER_FILE ||
             (brw->gen >= 8 && src1.file == BRW_IMMEDIATE_VALUE));
   } else {
      assert(src0.type == BRW_REGISTER_TYPE_F);
      assert(src1.type == BRW_REGISTER_TYPE_F);
      if (function == BRW_MATH_FUNCTION_POW) {
         assert(src1.file == BRW_GENERAL_REGISTER_FILE ||
                (brw->gen >= 8 && src1.file == BRW_IMMEDIATE_VALUE));
      } else {
         assert(src1.file == BRW_ARCHITECTURE_REGISTER_FILE &&
                src1.nr == BRW_ARF_NULL);
      }
   }

   /* Source modifiers are ignored for extended math instructions on Gen6. */
   if (brw->gen == 6) {
      assert(!src0.negate);
      assert(!src0.abs);
      assert(!src1.negate);
      assert(!src1.abs);
   }

   brw_inst_set_math_function(brw, insn, function);

   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src0);
   brw_set_src1(p, insn, src1);
}


/**
 * Write a block of OWORDs (half a GRF each) from the scratch buffer,
 * using a constant offset per channel.
 *
 * The offset must be aligned to oword size (16 bytes).  Used for
 * register spilling.
 */
void brw_oword_block_write_scratch(struct brw_compile *p,
				   struct brw_reg mrf,
				   int num_regs,
				   unsigned offset)
{
   struct brw_context *brw = p->brw;
   uint32_t msg_control, msg_type;
   int mlen;

   if (brw->gen >= 6)
      offset /= 16;

   mrf = retype(mrf, BRW_REGISTER_TYPE_UD);

   if (num_regs == 1) {
      msg_control = BRW_DATAPORT_OWORD_BLOCK_2_OWORDS;
      mlen = 2;
   } else {
      msg_control = BRW_DATAPORT_OWORD_BLOCK_4_OWORDS;
      mlen = 3;
   }

   /* Set up the message header.  This is g0, with g0.2 filled with
    * the offset.  We don't want to leave our offset around in g0 or
    * it'll screw up texture samples, so set it up inside the message
    * reg.
    */
   {
      brw_push_insn_state(p);
      brw_set_default_mask_control(p, BRW_MASK_DISABLE);
      brw_set_default_compression_control(p, BRW_COMPRESSION_NONE);

      brw_MOV(p, mrf, retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UD));

      /* set message header global offset field (reg 0, element 2) */
      brw_MOV(p,
	      retype(brw_vec1_reg(BRW_MESSAGE_REGISTER_FILE,
				  mrf.nr,
				  2), BRW_REGISTER_TYPE_UD),
	      brw_imm_ud(offset));

      brw_pop_insn_state(p);
   }

   {
      struct brw_reg dest;
      brw_inst *insn = next_insn(p, BRW_OPCODE_SEND);
      int send_commit_msg;
      struct brw_reg src_header = retype(brw_vec8_grf(0, 0),
					 BRW_REGISTER_TYPE_UW);

      if (brw_inst_qtr_control(brw, insn) != BRW_COMPRESSION_NONE) {
         brw_inst_set_qtr_control(brw, insn, BRW_COMPRESSION_NONE);
	 src_header = vec16(src_header);
      }
      assert(brw_inst_pred_control(brw, insn) == BRW_PREDICATE_NONE);
      if (brw->gen < 6)
         brw_inst_set_base_mrf(brw, insn, mrf.nr);

      /* Until gen6, writes followed by reads from the same location
       * are not guaranteed to be ordered unless write_commit is set.
       * If set, then a no-op write is issued to the destination
       * register to set a dependency, and a read from the destination
       * can be used to ensure the ordering.
       *
       * For gen6, only writes between different threads need ordering
       * protection.  Our use of DP writes is all about register
       * spilling within a thread.
       */
      if (brw->gen >= 6) {
	 dest = retype(vec16(brw_null_reg()), BRW_REGISTER_TYPE_UW);
	 send_commit_msg = 0;
      } else {
	 dest = src_header;
	 send_commit_msg = 1;
      }

      brw_set_dest(p, insn, dest);
      if (brw->gen >= 6) {
	 brw_set_src0(p, insn, mrf);
      } else {
	 brw_set_src0(p, insn, brw_null_reg());
      }

      if (brw->gen >= 6)
	 msg_type = GEN6_DATAPORT_WRITE_MESSAGE_OWORD_BLOCK_WRITE;
      else
	 msg_type = BRW_DATAPORT_WRITE_MESSAGE_OWORD_BLOCK_WRITE;

      brw_set_dp_write_message(p,
			       insn,
			       255, /* binding table index (255=stateless) */
			       msg_control,
			       msg_type,
			       mlen,
			       true, /* header_present */
			       0, /* not a render target */
			       send_commit_msg, /* response_length */
			       0, /* eot */
			       send_commit_msg);
   }
}


/**
 * Read a block of owords (half a GRF each) from the scratch buffer
 * using a constant index per channel.
 *
 * Offset must be aligned to oword size (16 bytes).  Used for register
 * spilling.
 */
void
brw_oword_block_read_scratch(struct brw_compile *p,
			     struct brw_reg dest,
			     struct brw_reg mrf,
			     int num_regs,
			     unsigned offset)
{
   struct brw_context *brw = p->brw;
   uint32_t msg_control;
   int rlen;

   if (brw->gen >= 6)
      offset /= 16;

   mrf = retype(mrf, BRW_REGISTER_TYPE_UD);
   dest = retype(dest, BRW_REGISTER_TYPE_UW);

   if (num_regs == 1) {
      msg_control = BRW_DATAPORT_OWORD_BLOCK_2_OWORDS;
      rlen = 1;
   } else {
      msg_control = BRW_DATAPORT_OWORD_BLOCK_4_OWORDS;
      rlen = 2;
   }

   {
      brw_push_insn_state(p);
      brw_set_default_compression_control(p, BRW_COMPRESSION_NONE);
      brw_set_default_mask_control(p, BRW_MASK_DISABLE);

      brw_MOV(p, mrf, retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UD));

      /* set message header global offset field (reg 0, element 2) */
      brw_MOV(p,
	      retype(brw_vec1_reg(BRW_MESSAGE_REGISTER_FILE,
				  mrf.nr,
				  2), BRW_REGISTER_TYPE_UD),
	      brw_imm_ud(offset));

      brw_pop_insn_state(p);
   }

   {
      brw_inst *insn = next_insn(p, BRW_OPCODE_SEND);

      assert(brw_inst_pred_control(brw, insn) == 0);
      brw_inst_set_qtr_control(brw, insn, BRW_COMPRESSION_NONE);

      brw_set_dest(p, insn, dest);	/* UW? */
      if (brw->gen >= 6) {
	 brw_set_src0(p, insn, mrf);
      } else {
	 brw_set_src0(p, insn, brw_null_reg());
         brw_inst_set_base_mrf(brw, insn, mrf.nr);
      }

      brw_set_dp_read_message(p,
			      insn,
			      255, /* binding table index (255=stateless) */
			      msg_control,
			      BRW_DATAPORT_READ_MESSAGE_OWORD_BLOCK_READ, /* msg_type */
			      BRW_DATAPORT_READ_TARGET_RENDER_CACHE,
			      1, /* msg_length */
                              true, /* header_present */
			      rlen);
   }
}

void
gen7_block_read_scratch(struct brw_compile *p,
                        struct brw_reg dest,
                        int num_regs,
                        unsigned offset)
{
   const struct brw_context *brw = p->brw;
   brw_inst *insn = next_insn(p, BRW_OPCODE_SEND);
   assert(brw_inst_pred_control(brw, insn) == BRW_PREDICATE_NONE);

   brw_inst_set_qtr_control(brw, insn, BRW_COMPRESSION_NONE);
   brw_set_dest(p, insn, retype(dest, BRW_REGISTER_TYPE_UW));

   /* The HW requires that the header is present; this is to get the g0.5
    * scratch offset.
    */
   brw_set_src0(p, insn, brw_vec8_grf(0, 0));

   /* According to the docs, offset is "A 12-bit HWord offset into the memory
    * Immediate Memory buffer as specified by binding table 0xFF."  An HWORD
    * is 32 bytes, which happens to be the size of a register.
    */
   offset /= REG_SIZE;
   assert(offset < (1 << 12));

   gen7_set_dp_scratch_message(p, insn,
                               false, /* scratch read */
                               false, /* OWords */
                               false, /* invalidate after read */
                               num_regs,
                               offset,
                               1,        /* mlen: just g0 */
                               num_regs, /* rlen */
                               true);    /* header present */
}

/**
 * Read a float[4] vector from the data port Data Cache (const buffer).
 * Location (in buffer) should be a multiple of 16.
 * Used for fetching shader constants.
 */
void brw_oword_block_read(struct brw_compile *p,
			  struct brw_reg dest,
			  struct brw_reg mrf,
			  uint32_t offset,
			  uint32_t bind_table_index)
{
   struct brw_context *brw = p->brw;

   /* On newer hardware, offset is in units of owords. */
   if (brw->gen >= 6)
      offset /= 16;

   mrf = retype(mrf, BRW_REGISTER_TYPE_UD);

   brw_push_insn_state(p);
   brw_set_default_predicate_control(p, BRW_PREDICATE_NONE);
   brw_set_default_compression_control(p, BRW_COMPRESSION_NONE);
   brw_set_default_mask_control(p, BRW_MASK_DISABLE);

   brw_MOV(p, mrf, retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UD));

   /* set message header global offset field (reg 0, element 2) */
   brw_MOV(p,
	   retype(brw_vec1_reg(BRW_MESSAGE_REGISTER_FILE,
			       mrf.nr,
			       2), BRW_REGISTER_TYPE_UD),
	   brw_imm_ud(offset));

   brw_inst *insn = next_insn(p, BRW_OPCODE_SEND);

   /* cast dest to a uword[8] vector */
   dest = retype(vec8(dest), BRW_REGISTER_TYPE_UW);

   brw_set_dest(p, insn, dest);
   if (brw->gen >= 6) {
      brw_set_src0(p, insn, mrf);
   } else {
      brw_set_src0(p, insn, brw_null_reg());
      brw_inst_set_base_mrf(brw, insn, mrf.nr);
   }

   brw_set_dp_read_message(p,
			   insn,
			   bind_table_index,
			   BRW_DATAPORT_OWORD_BLOCK_1_OWORDLOW,
			   BRW_DATAPORT_READ_MESSAGE_OWORD_BLOCK_READ,
			   BRW_DATAPORT_READ_TARGET_DATA_CACHE,
			   1, /* msg_length */
                           true, /* header_present */
			   1); /* response_length (1 reg, 2 owords!) */

   brw_pop_insn_state(p);
}


void brw_fb_WRITE(struct brw_compile *p,
		  int dispatch_width,
                  unsigned msg_reg_nr,
                  struct brw_reg src0,
                  unsigned msg_control,
                  unsigned binding_table_index,
                  unsigned msg_length,
                  unsigned response_length,
                  bool eot,
                  bool header_present)
{
   struct brw_context *brw = p->brw;
   brw_inst *insn;
   unsigned msg_type;
   struct brw_reg dest;

   if (dispatch_width == 16)
      dest = retype(vec16(brw_null_reg()), BRW_REGISTER_TYPE_UW);
   else
      dest = retype(vec8(brw_null_reg()), BRW_REGISTER_TYPE_UW);

   if (brw->gen >= 6) {
      insn = next_insn(p, BRW_OPCODE_SENDC);
   } else {
      insn = next_insn(p, BRW_OPCODE_SEND);
   }
   brw_inst_set_qtr_control(brw, insn, BRW_COMPRESSION_NONE);

   if (brw->gen >= 6) {
      /* headerless version, just submit color payload */
      src0 = brw_message_reg(msg_reg_nr);

      msg_type = GEN6_DATAPORT_WRITE_MESSAGE_RENDER_TARGET_WRITE;
   } else {
      brw_inst_set_base_mrf(brw, insn, msg_reg_nr);

      msg_type = BRW_DATAPORT_WRITE_MESSAGE_RENDER_TARGET_WRITE;
   }

   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src0);
   brw_set_dp_write_message(p,
			    insn,
			    binding_table_index,
			    msg_control,
			    msg_type,
			    msg_length,
			    header_present,
			    eot, /* last render target write */
			    response_length,
			    eot,
			    0 /* send_commit_msg */);
}


/**
 * Texture sample instruction.
 * Note: the msg_type plus msg_length values determine exactly what kind
 * of sampling operation is performed.  See volume 4, page 161 of docs.
 */
void brw_SAMPLE(struct brw_compile *p,
		struct brw_reg dest,
		unsigned msg_reg_nr,
		struct brw_reg src0,
		unsigned binding_table_index,
		unsigned sampler,
		unsigned msg_type,
		unsigned response_length,
		unsigned msg_length,
		unsigned header_present,
		unsigned simd_mode,
		unsigned return_format)
{
   struct brw_context *brw = p->brw;
   brw_inst *insn;

   if (msg_reg_nr != -1)
      gen6_resolve_implied_move(p, &src0, msg_reg_nr);

   insn = next_insn(p, BRW_OPCODE_SEND);
   brw_inst_set_pred_control(brw, insn, BRW_PREDICATE_NONE); /* XXX */

   /* From the 965 PRM (volume 4, part 1, section 14.2.41):
    *
    *    "Instruction compression is not allowed for this instruction (that
    *     is, send). The hardware behavior is undefined if this instruction is
    *     set as compressed. However, compress control can be set to "SecHalf"
    *     to affect the EMask generation."
    *
    * No similar wording is found in later PRMs, but there are examples
    * utilizing send with SecHalf.  More importantly, SIMD8 sampler messages
    * are allowed in SIMD16 mode and they could not work without SecHalf.  For
    * these reasons, we allow BRW_COMPRESSION_2NDHALF here.
    */
   if (brw_inst_qtr_control(brw, insn) != BRW_COMPRESSION_2NDHALF)
      brw_inst_set_qtr_control(brw, insn, BRW_COMPRESSION_NONE);

   if (brw->gen < 6)
      brw_inst_set_base_mrf(brw, insn, msg_reg_nr);

   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src0);
   brw_set_sampler_message(p, insn,
                           binding_table_index,
                           sampler,
                           msg_type,
                           response_length,
                           msg_length,
                           header_present,
                           simd_mode,
                           return_format);
}

/* Adjust the message header's sampler state pointer to
 * select the correct group of 16 samplers.
 */
void brw_adjust_sampler_state_pointer(struct brw_compile *p,
                                      struct brw_reg header,
                                      struct brw_reg sampler_index,
                                      struct brw_reg scratch)
{
   /* The "Sampler Index" field can only store values between 0 and 15.
    * However, we can add an offset to the "Sampler State Pointer"
    * field, effectively selecting a different set of 16 samplers.
    *
    * The "Sampler State Pointer" needs to be aligned to a 32-byte
    * offset, and each sampler state is only 16-bytes, so we can't
    * exclusively use the offset - we have to use both.
    */

   struct brw_context *brw = p->brw;

   if (sampler_index.file == BRW_IMMEDIATE_VALUE) {
      const int sampler_state_size = 16; /* 16 bytes */
      uint32_t sampler = sampler_index.dw1.ud;

      if (sampler >= 16) {
         assert(brw->is_haswell || brw->gen >= 8);
         brw_ADD(p,
                 get_element_ud(header, 3),
                 get_element_ud(brw_vec8_grf(0, 0), 3),
                 brw_imm_ud(16 * (sampler / 16) * sampler_state_size));
      }
   } else {
      /* Non-const sampler array indexing case */
      if (brw->gen < 8 && !brw->is_haswell) {
         return;
      }

      struct brw_reg temp = vec1(retype(scratch, BRW_REGISTER_TYPE_UD));

      brw_AND(p, temp, sampler_index, brw_imm_ud(0x0f0));
      brw_SHL(p, temp, temp, brw_imm_ud(4));
      brw_ADD(p,
              get_element_ud(header, 3),
              get_element_ud(brw_vec8_grf(0, 0), 3),
              temp);
   }
}

/* All these variables are pretty confusing - we might be better off
 * using bitmasks and macros for this, in the old style.  Or perhaps
 * just having the caller instantiate the fields in dword3 itself.
 */
void brw_urb_WRITE(struct brw_compile *p,
		   struct brw_reg dest,
		   unsigned msg_reg_nr,
		   struct brw_reg src0,
                   enum brw_urb_write_flags flags,
		   unsigned msg_length,
		   unsigned response_length,
		   unsigned offset,
		   unsigned swizzle)
{
   struct brw_context *brw = p->brw;
   brw_inst *insn;

   gen6_resolve_implied_move(p, &src0, msg_reg_nr);

   if (brw->gen >= 7 && !(flags & BRW_URB_WRITE_USE_CHANNEL_MASKS)) {
      /* Enable Channel Masks in the URB_WRITE_HWORD message header */
      brw_push_insn_state(p);
      brw_set_default_access_mode(p, BRW_ALIGN_1);
      brw_set_default_mask_control(p, BRW_MASK_DISABLE);
      brw_OR(p, retype(brw_vec1_reg(BRW_MESSAGE_REGISTER_FILE, msg_reg_nr, 5),
		       BRW_REGISTER_TYPE_UD),
	        retype(brw_vec1_grf(0, 5), BRW_REGISTER_TYPE_UD),
		brw_imm_ud(0xff00));
      brw_pop_insn_state(p);
   }

   insn = next_insn(p, BRW_OPCODE_SEND);

   assert(msg_length < BRW_MAX_MRF);

   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src0);
   brw_set_src1(p, insn, brw_imm_d(0));

   if (brw->gen < 6)
      brw_inst_set_base_mrf(brw, insn, msg_reg_nr);

   brw_set_urb_message(p,
		       insn,
		       flags,
		       msg_length,
		       response_length,
		       offset,
		       swizzle);
}

static int
brw_find_next_block_end(struct brw_compile *p, int start_offset)
{
   int offset;
   void *store = p->store;
   const struct brw_context *brw = p->brw;

   for (offset = next_offset(brw, store, start_offset);
        offset < p->next_insn_offset;
        offset = next_offset(brw, store, offset)) {
      brw_inst *insn = store + offset;

      switch (brw_inst_opcode(brw, insn)) {
      case BRW_OPCODE_ENDIF:
      case BRW_OPCODE_ELSE:
      case BRW_OPCODE_WHILE:
      case BRW_OPCODE_HALT:
	 return offset;
      }
   }

   return 0;
}

/* There is no DO instruction on gen6, so to find the end of the loop
 * we have to see if the loop is jumping back before our start
 * instruction.
 */
static int
brw_find_loop_end(struct brw_compile *p, int start_offset)
{
   struct brw_context *brw = p->brw;
   int offset;
   int scale = 16 / brw_jump_scale(brw);
   void *store = p->store;

   assert(brw->gen >= 6);

   /* Always start after the instruction (such as a WHILE) we're trying to fix
    * up.
    */
   for (offset = next_offset(brw, store, start_offset);
        offset < p->next_insn_offset;
        offset = next_offset(brw, store, offset)) {
      brw_inst *insn = store + offset;

      if (brw_inst_opcode(brw, insn) == BRW_OPCODE_WHILE) {
         int jip = brw->gen == 6 ? brw_inst_gen6_jump_count(brw, insn)
                                 : brw_inst_jip(brw, insn);
	 if (offset + jip * scale <= start_offset)
	    return offset;
      }
   }
   assert(!"not reached");
   return start_offset;
}

/* After program generation, go back and update the UIP and JIP of
 * BREAK, CONT, and HALT instructions to their correct locations.
 */
void
brw_set_uip_jip(struct brw_compile *p)
{
   struct brw_context *brw = p->brw;
   int offset;
   int br = brw_jump_scale(brw);
   int scale = 16 / br;
   void *store = p->store;

   if (brw->gen < 6)
      return;

   for (offset = 0; offset < p->next_insn_offset;
        offset = next_offset(brw, store, offset)) {
      brw_inst *insn = store + offset;

      if (brw_inst_cmpt_control(brw, insn)) {
	 /* Fixups for compacted BREAK/CONTINUE not supported yet. */
         assert(brw_inst_opcode(brw, insn) != BRW_OPCODE_BREAK &&
                brw_inst_opcode(brw, insn) != BRW_OPCODE_CONTINUE &&
                brw_inst_opcode(brw, insn) != BRW_OPCODE_HALT);
	 continue;
      }

      int block_end_offset = brw_find_next_block_end(p, offset);
      switch (brw_inst_opcode(brw, insn)) {
      case BRW_OPCODE_BREAK:
         assert(block_end_offset != 0);
         brw_inst_set_jip(brw, insn, (block_end_offset - offset) / scale);
	 /* Gen7 UIP points to WHILE; Gen6 points just after it */
         brw_inst_set_uip(brw, insn,
	    (brw_find_loop_end(p, offset) - offset +
             (brw->gen == 6 ? 16 : 0)) / scale);
	 break;
      case BRW_OPCODE_CONTINUE:
         assert(block_end_offset != 0);
         brw_inst_set_jip(brw, insn, (block_end_offset - offset) / scale);
         brw_inst_set_uip(brw, insn,
            (brw_find_loop_end(p, offset) - offset) / scale);

         assert(brw_inst_uip(brw, insn) != 0);
         assert(brw_inst_jip(brw, insn) != 0);
	 break;

      case BRW_OPCODE_ENDIF:
         if (block_end_offset == 0)
            brw_inst_set_jip(brw, insn, 1 * br);
         else
            brw_inst_set_jip(brw, insn, (block_end_offset - offset) / scale);
	 break;

      case BRW_OPCODE_HALT:
	 /* From the Sandy Bridge PRM (volume 4, part 2, section 8.3.19):
	  *
	  *    "In case of the halt instruction not inside any conditional
	  *     code block, the value of <JIP> and <UIP> should be the
	  *     same. In case of the halt instruction inside conditional code
	  *     block, the <UIP> should be the end of the program, and the
	  *     <JIP> should be end of the most inner conditional code block."
	  *
	  * The uip will have already been set by whoever set up the
	  * instruction.
	  */
	 if (block_end_offset == 0) {
            brw_inst_set_jip(brw, insn, brw_inst_uip(brw, insn));
	 } else {
            brw_inst_set_jip(brw, insn, (block_end_offset - offset) / scale);
	 }
         assert(brw_inst_uip(brw, insn) != 0);
         assert(brw_inst_jip(brw, insn) != 0);
	 break;
      }
   }
}

void brw_ff_sync(struct brw_compile *p,
		   struct brw_reg dest,
		   unsigned msg_reg_nr,
		   struct brw_reg src0,
		   bool allocate,
		   unsigned response_length,
		   bool eot)
{
   struct brw_context *brw = p->brw;
   brw_inst *insn;

   gen6_resolve_implied_move(p, &src0, msg_reg_nr);

   insn = next_insn(p, BRW_OPCODE_SEND);
   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src0);
   brw_set_src1(p, insn, brw_imm_d(0));

   if (brw->gen < 6)
      brw_inst_set_base_mrf(brw, insn, msg_reg_nr);

   brw_set_ff_sync_message(p,
			   insn,
			   allocate,
			   response_length,
			   eot);
}

/**
 * Emit the SEND instruction necessary to generate stream output data on Gen6
 * (for transform feedback).
 *
 * If send_commit_msg is true, this is the last piece of stream output data
 * from this thread, so send the data as a committed write.  According to the
 * Sandy Bridge PRM (volume 2 part 1, section 4.5.1):
 *
 *   "Prior to End of Thread with a URB_WRITE, the kernel must ensure all
 *   writes are complete by sending the final write as a committed write."
 */
void
brw_svb_write(struct brw_compile *p,
              struct brw_reg dest,
              unsigned msg_reg_nr,
              struct brw_reg src0,
              unsigned binding_table_index,
              bool   send_commit_msg)
{
   brw_inst *insn;

   gen6_resolve_implied_move(p, &src0, msg_reg_nr);

   insn = next_insn(p, BRW_OPCODE_SEND);
   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, src0);
   brw_set_src1(p, insn, brw_imm_d(0));
   brw_set_dp_write_message(p, insn,
                            binding_table_index,
                            0, /* msg_control: ignored */
                            GEN6_DATAPORT_WRITE_MESSAGE_STREAMED_VB_WRITE,
                            1, /* msg_length */
                            true, /* header_present */
                            0, /* last_render_target: ignored */
                            send_commit_msg, /* response_length */
                            0, /* end_of_thread */
                            send_commit_msg); /* send_commit_msg */
}

static void
brw_set_dp_untyped_atomic_message(struct brw_compile *p,
                                  brw_inst *insn,
                                  unsigned atomic_op,
                                  unsigned bind_table_index,
                                  unsigned msg_length,
                                  unsigned response_length,
                                  bool header_present)
{
   const struct brw_context *brw = p->brw;

   unsigned msg_control =
      atomic_op | /* Atomic Operation Type: BRW_AOP_* */
      (response_length ? 1 << 5 : 0); /* Return data expected */

   if (brw->gen >= 8 || brw->is_haswell) {
      brw_set_message_descriptor(p, insn, HSW_SFID_DATAPORT_DATA_CACHE_1,
                                 msg_length, response_length,
                                 header_present, false);


      if (brw_inst_access_mode(brw, insn) == BRW_ALIGN_1) {
         if (brw_inst_exec_size(brw, insn) != BRW_EXECUTE_16)
            msg_control |= 1 << 4; /* SIMD8 mode */

         brw_inst_set_dp_msg_type(brw, insn,
                                  HSW_DATAPORT_DC_PORT1_UNTYPED_ATOMIC_OP);
      } else {
         brw_inst_set_dp_msg_type(brw, insn,
            HSW_DATAPORT_DC_PORT1_UNTYPED_ATOMIC_OP_SIMD4X2);
      }
   } else {
      brw_set_message_descriptor(p, insn, GEN7_SFID_DATAPORT_DATA_CACHE,
                                 msg_length, response_length,
                                 header_present, false);

      brw_inst_set_dp_msg_type(brw, insn, GEN7_DATAPORT_DC_UNTYPED_ATOMIC_OP);

      if (brw_inst_exec_size(brw, insn) != BRW_EXECUTE_16)
         msg_control |= 1 << 4; /* SIMD8 mode */
   }

   brw_inst_set_binding_table_index(brw, insn, bind_table_index);
   brw_inst_set_dp_msg_control(brw, insn, msg_control);
}

void
brw_untyped_atomic(struct brw_compile *p,
                   struct brw_reg dest,
                   struct brw_reg mrf,
                   unsigned atomic_op,
                   unsigned bind_table_index,
                   unsigned msg_length,
                   unsigned response_length) {
   const struct brw_context *brw = p->brw;
   brw_inst *insn = brw_next_insn(p, BRW_OPCODE_SEND);

   brw_set_dest(p, insn, retype(dest, BRW_REGISTER_TYPE_UD));
   brw_set_src0(p, insn, retype(mrf, BRW_REGISTER_TYPE_UD));
   brw_set_src1(p, insn, brw_imm_d(0));
   brw_set_dp_untyped_atomic_message(
      p, insn, atomic_op, bind_table_index, msg_length, response_length,
      brw_inst_access_mode(brw, insn) == BRW_ALIGN_1);
}

static void
brw_set_dp_untyped_surface_read_message(struct brw_compile *p,
                                        brw_inst *insn,
                                        unsigned bind_table_index,
                                        unsigned msg_length,
                                        unsigned response_length,
                                        bool header_present)
{
   const struct brw_context *brw = p->brw;
   const unsigned dispatch_width =
      (brw_inst_exec_size(brw, insn) == BRW_EXECUTE_16 ? 16 : 8);
   const unsigned num_channels = response_length / (dispatch_width / 8);

   if (brw->gen >= 8 || brw->is_haswell) {
      brw_set_message_descriptor(p, insn, HSW_SFID_DATAPORT_DATA_CACHE_1,
                                 msg_length, response_length,
                                 header_present, false);

      brw_inst_set_dp_msg_type(brw, insn,
                               HSW_DATAPORT_DC_PORT1_UNTYPED_SURFACE_READ);
   } else {
      brw_set_message_descriptor(p, insn, GEN7_SFID_DATAPORT_DATA_CACHE,
                                 msg_length, response_length,
                                 header_present, false);

      brw_inst_set_dp_msg_type(brw, insn,
                               GEN7_DATAPORT_DC_UNTYPED_SURFACE_READ);
   }

   /* Set mask of 32-bit channels to drop. */
   unsigned msg_control = (0xf & (0xf << num_channels));

   if (brw_inst_access_mode(brw, insn) == BRW_ALIGN_1) {
      if (dispatch_width == 16)
         msg_control |= 1 << 4; /* SIMD16 mode */
      else
         msg_control |= 2 << 4; /* SIMD8 mode */
   }

   brw_inst_set_binding_table_index(brw, insn, bind_table_index);
   brw_inst_set_dp_msg_control(brw, insn, msg_control);
}

void
brw_untyped_surface_read(struct brw_compile *p,
                         struct brw_reg dest,
                         struct brw_reg mrf,
                         unsigned bind_table_index,
                         unsigned msg_length,
                         unsigned response_length)
{
   const struct brw_context *brw = p->brw;
   brw_inst *insn = next_insn(p, BRW_OPCODE_SEND);

   brw_set_dest(p, insn, retype(dest, BRW_REGISTER_TYPE_UD));
   brw_set_src0(p, insn, retype(mrf, BRW_REGISTER_TYPE_UD));
   brw_set_dp_untyped_surface_read_message(
      p, insn, bind_table_index, msg_length, response_length,
      brw_inst_access_mode(brw, insn) == BRW_ALIGN_1);
}

void
brw_pixel_interpolator_query(struct brw_compile *p,
                             struct brw_reg dest,
                             struct brw_reg mrf,
                             bool noperspective,
                             unsigned mode,
                             unsigned data,
                             unsigned msg_length,
                             unsigned response_length)
{
   const struct brw_context *brw = p->brw;
   struct brw_inst *insn = next_insn(p, BRW_OPCODE_SEND);

   brw_set_dest(p, insn, dest);
   brw_set_src0(p, insn, mrf);
   brw_set_message_descriptor(p, insn, GEN7_SFID_PIXEL_INTERPOLATOR,
                              msg_length, response_length,
                              false /* header is never present for PI */,
                              false);

   brw_inst_set_pi_simd_mode(
         brw, insn, brw_inst_exec_size(brw, insn) == BRW_EXECUTE_16);
   brw_inst_set_pi_slot_group(brw, insn, 0); /* zero unless 32/64px dispatch */
   brw_inst_set_pi_nopersp(brw, insn, noperspective);
   brw_inst_set_pi_message_type(brw, insn, mode);
   brw_inst_set_pi_message_data(brw, insn, data);
}

/**
 * This instruction is generated as a single-channel align1 instruction by
 * both the VS and FS stages when using INTEL_DEBUG=shader_time.
 *
 * We can't use the typed atomic op in the FS because that has the execution
 * mask ANDed with the pixel mask, but we just want to write the one dword for
 * all the pixels.
 *
 * We don't use the SIMD4x2 atomic ops in the VS because want to just write
 * one u32.  So we use the same untyped atomic write message as the pixel
 * shader.
 *
 * The untyped atomic operation requires a BUFFER surface type with RAW
 * format, and is only accessible through the legacy DATA_CACHE dataport
 * messages.
 */
void brw_shader_time_add(struct brw_compile *p,
                         struct brw_reg payload,
                         uint32_t surf_index)
{
   assert(p->brw->gen >= 7);

   brw_push_insn_state(p);
   brw_set_default_access_mode(p, BRW_ALIGN_1);
   brw_set_default_mask_control(p, BRW_MASK_DISABLE);
   brw_inst *send = brw_next_insn(p, BRW_OPCODE_SEND);
   brw_pop_insn_state(p);

   /* We use brw_vec1_reg and unmasked because we want to increment the given
    * offset only once.
    */
   brw_set_dest(p, send, brw_vec1_reg(BRW_ARCHITECTURE_REGISTER_FILE,
                                      BRW_ARF_NULL, 0));
   brw_set_src0(p, send, brw_vec1_reg(payload.file,
                                      payload.nr, 0));
   brw_set_dp_untyped_atomic_message(p, send, BRW_AOP_ADD, surf_index,
                                     2 /* message length */,
                                     0 /* response length */,
                                     false /* header present */);
}
