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
     

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_eu.h"




/***********************************************************************
 * Internal helper for constructing instructions
 */

static void guess_execution_size(struct brw_compile *p,
				 struct brw_instruction *insn,
				 struct brw_reg reg)
{
   if (reg.width == BRW_WIDTH_8 && p->compressed)
      insn->header.execution_size = BRW_EXECUTE_16;
   else
      insn->header.execution_size = reg.width;	/* note - definitions are compatible */
}


/**
 * Prior to Sandybridge, the SEND instruction accepted non-MRF source
 * registers, implicitly moving the operand to a message register.
 *
 * On Sandybridge, this is no longer the case.  This function performs the
 * explicit move; it should be called before emitting a SEND instruction.
 */
static void
gen6_resolve_implied_move(struct brw_compile *p,
			  struct brw_reg *src,
			  GLuint msg_reg_nr)
{
   struct intel_context *intel = &p->brw->intel;
   if (intel->gen != 6)
      return;

   if (src->file != BRW_ARCHITECTURE_REGISTER_FILE || src->nr != BRW_ARF_NULL) {
      brw_push_insn_state(p);
      brw_set_mask_control(p, BRW_MASK_DISABLE);
      brw_set_compression_control(p, BRW_COMPRESSION_NONE);
      brw_MOV(p, retype(brw_message_reg(msg_reg_nr), BRW_REGISTER_TYPE_UD),
	      retype(*src, BRW_REGISTER_TYPE_UD));
      brw_pop_insn_state(p);
   }
   *src = brw_message_reg(msg_reg_nr);
}


static void brw_set_dest(struct brw_compile *p,
			 struct brw_instruction *insn,
			 struct brw_reg dest)
{
   if (dest.file != BRW_ARCHITECTURE_REGISTER_FILE &&
       dest.file != BRW_MESSAGE_REGISTER_FILE)
      assert(dest.nr < 128);

   insn->bits1.da1.dest_reg_file = dest.file;
   insn->bits1.da1.dest_reg_type = dest.type;
   insn->bits1.da1.dest_address_mode = dest.address_mode;

   if (dest.address_mode == BRW_ADDRESS_DIRECT) {   
      insn->bits1.da1.dest_reg_nr = dest.nr;

      if (insn->header.access_mode == BRW_ALIGN_1) {
	 insn->bits1.da1.dest_subreg_nr = dest.subnr;
	 if (dest.hstride == BRW_HORIZONTAL_STRIDE_0)
	    dest.hstride = BRW_HORIZONTAL_STRIDE_1;
	 insn->bits1.da1.dest_horiz_stride = dest.hstride;
      }
      else {
	 insn->bits1.da16.dest_subreg_nr = dest.subnr / 16;
	 insn->bits1.da16.dest_writemask = dest.dw1.bits.writemask;
	 /* even ignored in da16, still need to set as '01' */
	 insn->bits1.da16.dest_horiz_stride = 1;
      }
   }
   else {
      insn->bits1.ia1.dest_subreg_nr = dest.subnr;

      /* These are different sizes in align1 vs align16:
       */
      if (insn->header.access_mode == BRW_ALIGN_1) {
	 insn->bits1.ia1.dest_indirect_offset = dest.dw1.bits.indirect_offset;
	 if (dest.hstride == BRW_HORIZONTAL_STRIDE_0)
	    dest.hstride = BRW_HORIZONTAL_STRIDE_1;
	 insn->bits1.ia1.dest_horiz_stride = dest.hstride;
      }
      else {
	 insn->bits1.ia16.dest_indirect_offset = dest.dw1.bits.indirect_offset;
	 /* even ignored in da16, still need to set as '01' */
	 insn->bits1.ia16.dest_horiz_stride = 1;
      }
   }

   /* NEW: Set the execution size based on dest.width and
    * insn->compression_control:
    */
   guess_execution_size(p, insn, dest);
}

extern int reg_type_size[];

static void
validate_reg(struct brw_instruction *insn, struct brw_reg reg)
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
	 assert(hstride_for_reg[insn->bits1.da1.dest_horiz_stride] *
		reg_type_size[insn->bits1.da1.dest_reg_type] == 2);
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

   assert(insn->header.execution_size >= 0 &&
	  insn->header.execution_size < Elements(execsize_for_reg));
   execsize = execsize_for_reg[insn->header.execution_size];

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

static void brw_set_src0( struct brw_instruction *insn,
                          struct brw_reg reg )
{
   if (reg.type != BRW_ARCHITECTURE_REGISTER_FILE)
      assert(reg.nr < 128);

   validate_reg(insn, reg);

   insn->bits1.da1.src0_reg_file = reg.file;
   insn->bits1.da1.src0_reg_type = reg.type;
   insn->bits2.da1.src0_abs = reg.abs;
   insn->bits2.da1.src0_negate = reg.negate;
   insn->bits2.da1.src0_address_mode = reg.address_mode;

   if (reg.file == BRW_IMMEDIATE_VALUE) {
      insn->bits3.ud = reg.dw1.ud;
   
      /* Required to set some fields in src1 as well:
       */
      insn->bits1.da1.src1_reg_file = 0; /* arf */
      insn->bits1.da1.src1_reg_type = reg.type;
   }
   else 
   {
      if (reg.address_mode == BRW_ADDRESS_DIRECT) {
	 if (insn->header.access_mode == BRW_ALIGN_1) {
	    insn->bits2.da1.src0_subreg_nr = reg.subnr;
	    insn->bits2.da1.src0_reg_nr = reg.nr;
	 }
	 else {
	    insn->bits2.da16.src0_subreg_nr = reg.subnr / 16;
	    insn->bits2.da16.src0_reg_nr = reg.nr;
	 }
      }
      else {
	 insn->bits2.ia1.src0_subreg_nr = reg.subnr;

	 if (insn->header.access_mode == BRW_ALIGN_1) {
	    insn->bits2.ia1.src0_indirect_offset = reg.dw1.bits.indirect_offset; 
	 }
	 else {
	    insn->bits2.ia16.src0_subreg_nr = reg.dw1.bits.indirect_offset;
	 }
      }

      if (insn->header.access_mode == BRW_ALIGN_1) {
	 if (reg.width == BRW_WIDTH_1 && 
	     insn->header.execution_size == BRW_EXECUTE_1) {
	    insn->bits2.da1.src0_horiz_stride = BRW_HORIZONTAL_STRIDE_0;
	    insn->bits2.da1.src0_width = BRW_WIDTH_1;
	    insn->bits2.da1.src0_vert_stride = BRW_VERTICAL_STRIDE_0;
	 }
	 else {
	    insn->bits2.da1.src0_horiz_stride = reg.hstride;
	    insn->bits2.da1.src0_width = reg.width;
	    insn->bits2.da1.src0_vert_stride = reg.vstride;
	 }
      }
      else {
	 insn->bits2.da16.src0_swz_x = BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_X);
	 insn->bits2.da16.src0_swz_y = BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_Y);
	 insn->bits2.da16.src0_swz_z = BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_Z);
	 insn->bits2.da16.src0_swz_w = BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_W);

	 /* This is an oddity of the fact we're using the same
	  * descriptions for registers in align_16 as align_1:
	  */
	 if (reg.vstride == BRW_VERTICAL_STRIDE_8)
	    insn->bits2.da16.src0_vert_stride = BRW_VERTICAL_STRIDE_4;
	 else
	    insn->bits2.da16.src0_vert_stride = reg.vstride;
      }
   }
}


void brw_set_src1( struct brw_instruction *insn,
                   struct brw_reg reg )
{
   assert(reg.file != BRW_MESSAGE_REGISTER_FILE);

   assert(reg.nr < 128);

   validate_reg(insn, reg);

   insn->bits1.da1.src1_reg_file = reg.file;
   insn->bits1.da1.src1_reg_type = reg.type;
   insn->bits3.da1.src1_abs = reg.abs;
   insn->bits3.da1.src1_negate = reg.negate;

   /* Only src1 can be immediate in two-argument instructions.
    */
   assert(insn->bits1.da1.src0_reg_file != BRW_IMMEDIATE_VALUE);

   if (reg.file == BRW_IMMEDIATE_VALUE) {
      insn->bits3.ud = reg.dw1.ud;
   }
   else {
      /* This is a hardware restriction, which may or may not be lifted
       * in the future:
       */
      assert (reg.address_mode == BRW_ADDRESS_DIRECT);
      /* assert (reg.file == BRW_GENERAL_REGISTER_FILE); */

      if (insn->header.access_mode == BRW_ALIGN_1) {
	 insn->bits3.da1.src1_subreg_nr = reg.subnr;
	 insn->bits3.da1.src1_reg_nr = reg.nr;
      }
      else {
	 insn->bits3.da16.src1_subreg_nr = reg.subnr / 16;
	 insn->bits3.da16.src1_reg_nr = reg.nr;
      }

      if (insn->header.access_mode == BRW_ALIGN_1) {
	 if (reg.width == BRW_WIDTH_1 && 
	     insn->header.execution_size == BRW_EXECUTE_1) {
	    insn->bits3.da1.src1_horiz_stride = BRW_HORIZONTAL_STRIDE_0;
	    insn->bits3.da1.src1_width = BRW_WIDTH_1;
	    insn->bits3.da1.src1_vert_stride = BRW_VERTICAL_STRIDE_0;
	 }
	 else {
	    insn->bits3.da1.src1_horiz_stride = reg.hstride;
	    insn->bits3.da1.src1_width = reg.width;
	    insn->bits3.da1.src1_vert_stride = reg.vstride;
	 }
      }
      else {
	 insn->bits3.da16.src1_swz_x = BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_X);
	 insn->bits3.da16.src1_swz_y = BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_Y);
	 insn->bits3.da16.src1_swz_z = BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_Z);
	 insn->bits3.da16.src1_swz_w = BRW_GET_SWZ(reg.dw1.bits.swizzle, BRW_CHANNEL_W);

	 /* This is an oddity of the fact we're using the same
	  * descriptions for registers in align_16 as align_1:
	  */
	 if (reg.vstride == BRW_VERTICAL_STRIDE_8)
	    insn->bits3.da16.src1_vert_stride = BRW_VERTICAL_STRIDE_4;
	 else
	    insn->bits3.da16.src1_vert_stride = reg.vstride;
      }
   }
}



static void brw_set_math_message( struct brw_context *brw,
				  struct brw_instruction *insn,
				  GLuint msg_length,
				  GLuint response_length,
				  GLuint function,
				  GLuint integer_type,
				  GLboolean low_precision,
				  GLboolean saturate,
				  GLuint dataType )
{
   struct intel_context *intel = &brw->intel;
   brw_set_src1(insn, brw_imm_d(0));

   if (intel->gen == 5) {
       insn->bits3.math_gen5.function = function;
       insn->bits3.math_gen5.int_type = integer_type;
       insn->bits3.math_gen5.precision = low_precision;
       insn->bits3.math_gen5.saturate = saturate;
       insn->bits3.math_gen5.data_type = dataType;
       insn->bits3.math_gen5.snapshot = 0;
       insn->bits3.math_gen5.header_present = 0;
       insn->bits3.math_gen5.response_length = response_length;
       insn->bits3.math_gen5.msg_length = msg_length;
       insn->bits3.math_gen5.end_of_thread = 0;
       insn->bits2.send_gen5.sfid = BRW_MESSAGE_TARGET_MATH;
       insn->bits2.send_gen5.end_of_thread = 0;
   } else {
       insn->bits3.math.function = function;
       insn->bits3.math.int_type = integer_type;
       insn->bits3.math.precision = low_precision;
       insn->bits3.math.saturate = saturate;
       insn->bits3.math.data_type = dataType;
       insn->bits3.math.response_length = response_length;
       insn->bits3.math.msg_length = msg_length;
       insn->bits3.math.msg_target = BRW_MESSAGE_TARGET_MATH;
       insn->bits3.math.end_of_thread = 0;
   }
}


static void brw_set_ff_sync_message(struct brw_context *brw,
				    struct brw_instruction *insn,
				    GLboolean allocate,
				    GLuint response_length,
				    GLboolean end_of_thread)
{
	struct intel_context *intel = &brw->intel;
	brw_set_src1(insn, brw_imm_d(0));

	insn->bits3.urb_gen5.opcode = 1; /* FF_SYNC */
	insn->bits3.urb_gen5.offset = 0; /* Not used by FF_SYNC */
	insn->bits3.urb_gen5.swizzle_control = 0; /* Not used by FF_SYNC */
	insn->bits3.urb_gen5.allocate = allocate;
	insn->bits3.urb_gen5.used = 0; /* Not used by FF_SYNC */
	insn->bits3.urb_gen5.complete = 0; /* Not used by FF_SYNC */
	insn->bits3.urb_gen5.header_present = 1;
	insn->bits3.urb_gen5.response_length = response_length; /* may be 1 or 0 */
	insn->bits3.urb_gen5.msg_length = 1;
	insn->bits3.urb_gen5.end_of_thread = end_of_thread;
	if (intel->gen >= 6) {
	   insn->header.destreg__conditionalmod = BRW_MESSAGE_TARGET_URB;
	} else {
	   insn->bits2.send_gen5.sfid = BRW_MESSAGE_TARGET_URB;
	   insn->bits2.send_gen5.end_of_thread = end_of_thread;
	}
}

static void brw_set_urb_message( struct brw_context *brw,
				 struct brw_instruction *insn,
				 GLboolean allocate,
				 GLboolean used,
				 GLuint msg_length,
				 GLuint response_length,
				 GLboolean end_of_thread,
				 GLboolean complete,
				 GLuint offset,
				 GLuint swizzle_control )
{
    struct intel_context *intel = &brw->intel;
    brw_set_src1(insn, brw_imm_d(0));

    if (intel->gen >= 5) {
        insn->bits3.urb_gen5.opcode = 0;	/* ? */
        insn->bits3.urb_gen5.offset = offset;
        insn->bits3.urb_gen5.swizzle_control = swizzle_control;
        insn->bits3.urb_gen5.allocate = allocate;
        insn->bits3.urb_gen5.used = used;	/* ? */
        insn->bits3.urb_gen5.complete = complete;
        insn->bits3.urb_gen5.header_present = 1;
        insn->bits3.urb_gen5.response_length = response_length;
        insn->bits3.urb_gen5.msg_length = msg_length;
        insn->bits3.urb_gen5.end_of_thread = end_of_thread;
	if (intel->gen >= 6) {
	   /* For SNB, the SFID bits moved to the condmod bits, and
	    * EOT stayed in bits3 above.  Does the EOT bit setting
	    * below on Ironlake even do anything?
	    */
	   insn->header.destreg__conditionalmod = BRW_MESSAGE_TARGET_URB;
	} else {
	   insn->bits2.send_gen5.sfid = BRW_MESSAGE_TARGET_URB;
	   insn->bits2.send_gen5.end_of_thread = end_of_thread;
	}
    } else {
        insn->bits3.urb.opcode = 0;	/* ? */
        insn->bits3.urb.offset = offset;
        insn->bits3.urb.swizzle_control = swizzle_control;
        insn->bits3.urb.allocate = allocate;
        insn->bits3.urb.used = used;	/* ? */
        insn->bits3.urb.complete = complete;
        insn->bits3.urb.response_length = response_length;
        insn->bits3.urb.msg_length = msg_length;
        insn->bits3.urb.msg_target = BRW_MESSAGE_TARGET_URB;
        insn->bits3.urb.end_of_thread = end_of_thread;
    }
}

static void brw_set_dp_write_message( struct brw_context *brw,
				      struct brw_instruction *insn,
				      GLuint binding_table_index,
				      GLuint msg_control,
				      GLuint msg_type,
				      GLuint msg_length,
				      GLboolean header_present,
				      GLuint pixel_scoreboard_clear,
				      GLuint response_length,
				      GLuint end_of_thread,
				      GLuint send_commit_msg)
{
   struct intel_context *intel = &brw->intel;
   brw_set_src1(insn, brw_imm_ud(0));

   if (intel->gen >= 6) {
       insn->bits3.dp_render_cache.binding_table_index = binding_table_index;
       insn->bits3.dp_render_cache.msg_control = msg_control;
       insn->bits3.dp_render_cache.pixel_scoreboard_clear = pixel_scoreboard_clear;
       insn->bits3.dp_render_cache.msg_type = msg_type;
       insn->bits3.dp_render_cache.send_commit_msg = send_commit_msg;
       insn->bits3.dp_render_cache.header_present = header_present;
       insn->bits3.dp_render_cache.response_length = response_length;
       insn->bits3.dp_render_cache.msg_length = msg_length;
       insn->bits3.dp_render_cache.end_of_thread = end_of_thread;

       /* We always use the render cache for write messages */
       insn->header.destreg__conditionalmod = BRW_MESSAGE_TARGET_DATAPORT_WRITE;
   } else if (intel->gen == 5) {
       insn->bits3.dp_write_gen5.binding_table_index = binding_table_index;
       insn->bits3.dp_write_gen5.msg_control = msg_control;
       insn->bits3.dp_write_gen5.pixel_scoreboard_clear = pixel_scoreboard_clear;
       insn->bits3.dp_write_gen5.msg_type = msg_type;
       insn->bits3.dp_write_gen5.send_commit_msg = send_commit_msg;
       insn->bits3.dp_write_gen5.header_present = header_present;
       insn->bits3.dp_write_gen5.response_length = response_length;
       insn->bits3.dp_write_gen5.msg_length = msg_length;
       insn->bits3.dp_write_gen5.end_of_thread = end_of_thread;
       insn->bits2.send_gen5.sfid = BRW_MESSAGE_TARGET_DATAPORT_WRITE;
       insn->bits2.send_gen5.end_of_thread = end_of_thread;
   } else {
       insn->bits3.dp_write.binding_table_index = binding_table_index;
       insn->bits3.dp_write.msg_control = msg_control;
       insn->bits3.dp_write.pixel_scoreboard_clear = pixel_scoreboard_clear;
       insn->bits3.dp_write.msg_type = msg_type;
       insn->bits3.dp_write.send_commit_msg = send_commit_msg;
       insn->bits3.dp_write.response_length = response_length;
       insn->bits3.dp_write.msg_length = msg_length;
       insn->bits3.dp_write.msg_target = BRW_MESSAGE_TARGET_DATAPORT_WRITE;
       insn->bits3.dp_write.end_of_thread = end_of_thread;
   }
}

static void
brw_set_dp_read_message(struct brw_context *brw,
			struct brw_instruction *insn,
			GLuint binding_table_index,
			GLuint msg_control,
			GLuint msg_type,
			GLuint target_cache,
			GLuint msg_length,
			GLuint response_length)
{
   struct intel_context *intel = &brw->intel;
   brw_set_src1(insn, brw_imm_d(0));

   if (intel->gen >= 6) {
       uint32_t target_function;

       if (target_cache == BRW_DATAPORT_READ_TARGET_DATA_CACHE)
	  target_function = BRW_MESSAGE_TARGET_DATAPORT_READ; /* data cache */
       else
	  target_function = BRW_MESSAGE_TARGET_DATAPORT_WRITE; /* render cache */

       insn->bits3.dp_render_cache.binding_table_index = binding_table_index;
       insn->bits3.dp_render_cache.msg_control = msg_control;
       insn->bits3.dp_render_cache.pixel_scoreboard_clear = 0;
       insn->bits3.dp_render_cache.msg_type = msg_type;
       insn->bits3.dp_render_cache.send_commit_msg = 0;
       insn->bits3.dp_render_cache.header_present = 1;
       insn->bits3.dp_render_cache.response_length = response_length;
       insn->bits3.dp_render_cache.msg_length = msg_length;
       insn->bits3.dp_render_cache.end_of_thread = 0;
       insn->header.destreg__conditionalmod = target_function;
   } else if (intel->gen == 5) {
       insn->bits3.dp_read_gen5.binding_table_index = binding_table_index;
       insn->bits3.dp_read_gen5.msg_control = msg_control;
       insn->bits3.dp_read_gen5.msg_type = msg_type;
       insn->bits3.dp_read_gen5.target_cache = target_cache;
       insn->bits3.dp_read_gen5.header_present = 1;
       insn->bits3.dp_read_gen5.response_length = response_length;
       insn->bits3.dp_read_gen5.msg_length = msg_length;
       insn->bits3.dp_read_gen5.pad1 = 0;
       insn->bits3.dp_read_gen5.end_of_thread = 0;
       insn->bits2.send_gen5.sfid = BRW_MESSAGE_TARGET_DATAPORT_READ;
       insn->bits2.send_gen5.end_of_thread = 0;
   } else if (intel->is_g4x) {
       insn->bits3.dp_read_g4x.binding_table_index = binding_table_index; /*0:7*/
       insn->bits3.dp_read_g4x.msg_control = msg_control;  /*8:10*/
       insn->bits3.dp_read_g4x.msg_type = msg_type;  /*11:13*/
       insn->bits3.dp_read_g4x.target_cache = target_cache;  /*14:15*/
       insn->bits3.dp_read_g4x.response_length = response_length;  /*16:19*/
       insn->bits3.dp_read_g4x.msg_length = msg_length;  /*20:23*/
       insn->bits3.dp_read_g4x.msg_target = BRW_MESSAGE_TARGET_DATAPORT_READ; /*24:27*/
       insn->bits3.dp_read_g4x.pad1 = 0;
       insn->bits3.dp_read_g4x.end_of_thread = 0;
   } else {
       insn->bits3.dp_read.binding_table_index = binding_table_index; /*0:7*/
       insn->bits3.dp_read.msg_control = msg_control;  /*8:11*/
       insn->bits3.dp_read.msg_type = msg_type;  /*12:13*/
       insn->bits3.dp_read.target_cache = target_cache;  /*14:15*/
       insn->bits3.dp_read.response_length = response_length;  /*16:19*/
       insn->bits3.dp_read.msg_length = msg_length;  /*20:23*/
       insn->bits3.dp_read.msg_target = BRW_MESSAGE_TARGET_DATAPORT_READ; /*24:27*/
       insn->bits3.dp_read.pad1 = 0;  /*28:30*/
       insn->bits3.dp_read.end_of_thread = 0;  /*31*/
   }
}

static void brw_set_sampler_message(struct brw_context *brw,
                                    struct brw_instruction *insn,
                                    GLuint binding_table_index,
                                    GLuint sampler,
                                    GLuint msg_type,
                                    GLuint response_length,
                                    GLuint msg_length,
                                    GLboolean eot,
                                    GLuint header_present,
                                    GLuint simd_mode)
{
   struct intel_context *intel = &brw->intel;
   assert(eot == 0);
   brw_set_src1(insn, brw_imm_d(0));

   if (intel->gen >= 5) {
      insn->bits3.sampler_gen5.binding_table_index = binding_table_index;
      insn->bits3.sampler_gen5.sampler = sampler;
      insn->bits3.sampler_gen5.msg_type = msg_type;
      insn->bits3.sampler_gen5.simd_mode = simd_mode;
      insn->bits3.sampler_gen5.header_present = header_present;
      insn->bits3.sampler_gen5.response_length = response_length;
      insn->bits3.sampler_gen5.msg_length = msg_length;
      insn->bits3.sampler_gen5.end_of_thread = eot;
      if (intel->gen >= 6)
	  insn->header.destreg__conditionalmod = BRW_MESSAGE_TARGET_SAMPLER;
      else {
	  insn->bits2.send_gen5.sfid = BRW_MESSAGE_TARGET_SAMPLER;
	  insn->bits2.send_gen5.end_of_thread = eot;
      }
   } else if (intel->is_g4x) {
      insn->bits3.sampler_g4x.binding_table_index = binding_table_index;
      insn->bits3.sampler_g4x.sampler = sampler;
      insn->bits3.sampler_g4x.msg_type = msg_type;
      insn->bits3.sampler_g4x.response_length = response_length;
      insn->bits3.sampler_g4x.msg_length = msg_length;
      insn->bits3.sampler_g4x.end_of_thread = eot;
      insn->bits3.sampler_g4x.msg_target = BRW_MESSAGE_TARGET_SAMPLER;
   } else {
      insn->bits3.sampler.binding_table_index = binding_table_index;
      insn->bits3.sampler.sampler = sampler;
      insn->bits3.sampler.msg_type = msg_type;
      insn->bits3.sampler.return_format = BRW_SAMPLER_RETURN_FORMAT_FLOAT32;
      insn->bits3.sampler.response_length = response_length;
      insn->bits3.sampler.msg_length = msg_length;
      insn->bits3.sampler.end_of_thread = eot;
      insn->bits3.sampler.msg_target = BRW_MESSAGE_TARGET_SAMPLER;
   }
}



static struct brw_instruction *next_insn( struct brw_compile *p, 
					  GLuint opcode )
{
   struct brw_instruction *insn;

   assert(p->nr_insn + 1 < BRW_EU_MAX_INSN);

   insn = &p->store[p->nr_insn++];
   memcpy(insn, p->current, sizeof(*insn));

   /* Reset this one-shot flag: 
    */

   if (p->current->header.destreg__conditionalmod) {
      p->current->header.destreg__conditionalmod = 0;
      p->current->header.predicate_control = BRW_PREDICATE_NORMAL;
   }

   insn->header.opcode = opcode;
   return insn;
}


static struct brw_instruction *brw_alu1( struct brw_compile *p,
					 GLuint opcode,
					 struct brw_reg dest,
					 struct brw_reg src )
{
   struct brw_instruction *insn = next_insn(p, opcode);
   brw_set_dest(p, insn, dest);
   brw_set_src0(insn, src);   
   return insn;
}

static struct brw_instruction *brw_alu2(struct brw_compile *p,
					GLuint opcode,
					struct brw_reg dest,
					struct brw_reg src0,
					struct brw_reg src1 )
{
   struct brw_instruction *insn = next_insn(p, opcode);   
   brw_set_dest(p, insn, dest);
   brw_set_src0(insn, src0);
   brw_set_src1(insn, src1);
   return insn;
}


/***********************************************************************
 * Convenience routines.
 */
#define ALU1(OP)					\
struct brw_instruction *brw_##OP(struct brw_compile *p,	\
	      struct brw_reg dest,			\
	      struct brw_reg src0)   			\
{							\
   return brw_alu1(p, BRW_OPCODE_##OP, dest, src0);    	\
}

#define ALU2(OP)					\
struct brw_instruction *brw_##OP(struct brw_compile *p,	\
	      struct brw_reg dest,			\
	      struct brw_reg src0,			\
	      struct brw_reg src1)   			\
{							\
   return brw_alu2(p, BRW_OPCODE_##OP, dest, src0, src1);	\
}

/* Rounding operations (other than RNDD) require two instructions - the first
 * stores a rounded value (possibly the wrong way) in the dest register, but
 * also sets a per-channel "increment bit" in the flag register.  A predicated
 * add of 1.0 fixes dest to contain the desired result.
 */
#define ROUND(OP)							      \
void brw_##OP(struct brw_compile *p,					      \
	      struct brw_reg dest,					      \
	      struct brw_reg src)					      \
{									      \
   struct brw_instruction *rnd, *add;					      \
   rnd = next_insn(p, BRW_OPCODE_##OP);					      \
   brw_set_dest(p, rnd, dest);						      \
   brw_set_src0(rnd, src);						      \
   rnd->header.destreg__conditionalmod = 0x7; /* turn on round-increments */  \
									      \
   add = brw_ADD(p, dest, dest, brw_imm_f(1.0f));			      \
   add->header.predicate_control = BRW_PREDICATE_NORMAL;		      \
}


ALU1(MOV)
ALU2(SEL)
ALU1(NOT)
ALU2(AND)
ALU2(OR)
ALU2(XOR)
ALU2(SHR)
ALU2(SHL)
ALU2(RSR)
ALU2(RSL)
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


ROUND(RNDZ)
ROUND(RNDE)


struct brw_instruction *brw_ADD(struct brw_compile *p,
				struct brw_reg dest,
				struct brw_reg src0,
				struct brw_reg src1)
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

struct brw_instruction *brw_MUL(struct brw_compile *p,
				struct brw_reg dest,
				struct brw_reg src0,
				struct brw_reg src1)
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


void brw_NOP(struct brw_compile *p)
{
   struct brw_instruction *insn = next_insn(p, BRW_OPCODE_NOP);   
   brw_set_dest(p, insn, retype(brw_vec4_grf(0,0), BRW_REGISTER_TYPE_UD));
   brw_set_src0(insn, retype(brw_vec4_grf(0,0), BRW_REGISTER_TYPE_UD));
   brw_set_src1(insn, brw_imm_ud(0x0));
}





/***********************************************************************
 * Comparisons, if/else/endif
 */

struct brw_instruction *brw_JMPI(struct brw_compile *p, 
                                 struct brw_reg dest,
                                 struct brw_reg src0,
                                 struct brw_reg src1)
{
   struct brw_instruction *insn = brw_alu2(p, BRW_OPCODE_JMPI, dest, src0, src1);

   insn->header.execution_size = 1;
   insn->header.compression_control = BRW_COMPRESSION_NONE;
   insn->header.mask_control = BRW_MASK_DISABLE;

   p->current->header.predicate_control = BRW_PREDICATE_NONE;

   return insn;
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
 *
 * No attempt is made to deal with stack overflow (14 elements?).
 */
struct brw_instruction *brw_IF(struct brw_compile *p, GLuint execute_size)
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_instruction *insn;

   if (p->single_program_flow) {
      assert(execute_size == BRW_EXECUTE_1);

      insn = next_insn(p, BRW_OPCODE_ADD);
      insn->header.predicate_inverse = 1;
   } else {
      insn = next_insn(p, BRW_OPCODE_IF);
   }

   /* Override the defaults for this instruction:
    */
   if (intel->gen < 6) {
      brw_set_dest(p, insn, brw_ip_reg());
      brw_set_src0(insn, brw_ip_reg());
      brw_set_src1(insn, brw_imm_d(0x0));
   } else {
      brw_set_dest(p, insn, brw_imm_w(0));
      insn->bits1.branch_gen6.jump_count = 0;
      brw_set_src0(insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src1(insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   }

   insn->header.execution_size = execute_size;
   insn->header.compression_control = BRW_COMPRESSION_NONE;
   insn->header.predicate_control = BRW_PREDICATE_NORMAL;
   insn->header.mask_control = BRW_MASK_ENABLE;
   if (!p->single_program_flow)
       insn->header.thread_control = BRW_THREAD_SWITCH;

   p->current->header.predicate_control = BRW_PREDICATE_NONE;

   return insn;
}

struct brw_instruction *
gen6_IF(struct brw_compile *p, uint32_t conditional,
	struct brw_reg src0, struct brw_reg src1)
{
   struct brw_instruction *insn;

   insn = next_insn(p, BRW_OPCODE_IF);

   brw_set_dest(p, insn, brw_imm_w(0));
   insn->header.execution_size = BRW_EXECUTE_8;
   insn->bits1.branch_gen6.jump_count = 0;
   brw_set_src0(insn, src0);
   brw_set_src1(insn, src1);

   assert(insn->header.compression_control == BRW_COMPRESSION_NONE);
   assert(insn->header.predicate_control == BRW_PREDICATE_NONE);
   insn->header.destreg__conditionalmod = conditional;

   if (!p->single_program_flow)
       insn->header.thread_control = BRW_THREAD_SWITCH;

   return insn;
}

struct brw_instruction *brw_ELSE(struct brw_compile *p, 
				 struct brw_instruction *if_insn)
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_instruction *insn;
   GLuint br = 1;

   /* jump count is for 64bit data chunk each, so one 128bit
      instruction requires 2 chunks. */
   if (intel->gen >= 5)
      br = 2;

   if (p->single_program_flow) {
      insn = next_insn(p, BRW_OPCODE_ADD);
   } else {
      insn = next_insn(p, BRW_OPCODE_ELSE);
   }

   if (intel->gen < 6) {
      brw_set_dest(p, insn, brw_ip_reg());
      brw_set_src0(insn, brw_ip_reg());
      brw_set_src1(insn, brw_imm_d(0x0));
   } else {
      brw_set_dest(p, insn, brw_imm_w(0));
      insn->bits1.branch_gen6.jump_count = 0;
      brw_set_src0(insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src1(insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   }

   insn->header.compression_control = BRW_COMPRESSION_NONE;
   insn->header.execution_size = if_insn->header.execution_size;
   insn->header.mask_control = BRW_MASK_ENABLE;
   if (!p->single_program_flow)
       insn->header.thread_control = BRW_THREAD_SWITCH;

   /* Patch the if instruction to point at this instruction.
    */
   if (p->single_program_flow) {
      assert(if_insn->header.opcode == BRW_OPCODE_ADD);

      if_insn->bits3.ud = (insn - if_insn + 1) * 16;
   } else {
      assert(if_insn->header.opcode == BRW_OPCODE_IF);

      if (intel->gen < 6) {
	 if_insn->bits3.if_else.jump_count = br * (insn - if_insn);
	 if_insn->bits3.if_else.pop_count = 0;
	 if_insn->bits3.if_else.pad0 = 0;
      } else {
	 if_insn->bits1.branch_gen6.jump_count = br * (insn - if_insn + 1);
      }
   }

   return insn;
}

void brw_ENDIF(struct brw_compile *p, 
	       struct brw_instruction *patch_insn)
{
   struct intel_context *intel = &p->brw->intel;
   GLuint br = 1;

   if (intel->gen >= 5)
      br = 2; 
 
   if (p->single_program_flow) {
      /* In single program flow mode, there's no need to execute an ENDIF,
       * since we don't need to do any stack operations, and if we're executing
       * currently, we want to just continue executing.
       */
      struct brw_instruction *next = &p->store[p->nr_insn];

      assert(patch_insn->header.opcode == BRW_OPCODE_ADD);

      patch_insn->bits3.ud = (next - patch_insn) * 16;
   } else {
      struct brw_instruction *insn = next_insn(p, BRW_OPCODE_ENDIF);

      if (intel->gen < 6) {
	 brw_set_dest(p, insn, retype(brw_vec4_grf(0,0), BRW_REGISTER_TYPE_UD));
	 brw_set_src0(insn, retype(brw_vec4_grf(0,0), BRW_REGISTER_TYPE_UD));
	 brw_set_src1(insn, brw_imm_d(0x0));
      } else {
	 brw_set_dest(p, insn, brw_imm_w(0));
	 brw_set_src0(insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
	 brw_set_src1(insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      }

      insn->header.compression_control = BRW_COMPRESSION_NONE;
      insn->header.execution_size = patch_insn->header.execution_size;
      insn->header.mask_control = BRW_MASK_ENABLE;
      insn->header.thread_control = BRW_THREAD_SWITCH;

      if (intel->gen < 6)
	 assert(patch_insn->bits3.if_else.jump_count == 0);
      else
	 assert(patch_insn->bits1.branch_gen6.jump_count == 0);

      /* Patch the if or else instructions to point at this or the next
       * instruction respectively.
       */
      if (patch_insn->header.opcode == BRW_OPCODE_IF) {
	 if (intel->gen < 6) {
	    /* Turn it into an IFF, which means no mask stack operations for
	     * all-false and jumping past the ENDIF.
	     */
	    patch_insn->header.opcode = BRW_OPCODE_IFF;
	    patch_insn->bits3.if_else.jump_count = br * (insn - patch_insn + 1);
	    patch_insn->bits3.if_else.pop_count = 0;
	    patch_insn->bits3.if_else.pad0 = 0;
	 } else {
	    /* As of gen6, there is no IFF and IF must point to the ENDIF. */
	    patch_insn->bits1.branch_gen6.jump_count = br * (insn - patch_insn);
	 }
      } else {
	 assert(patch_insn->header.opcode == BRW_OPCODE_ELSE);
	 if (intel->gen < 6) {
	    /* BRW_OPCODE_ELSE pre-gen6 should point just past the
	     * matching ENDIF.
	     */
	    patch_insn->bits3.if_else.jump_count = br * (insn - patch_insn + 1);
	    patch_insn->bits3.if_else.pop_count = 1;
	    patch_insn->bits3.if_else.pad0 = 0;
	 } else {
	    /* BRW_OPCODE_ELSE on gen6 should point to the matching ENDIF. */
	    patch_insn->bits1.branch_gen6.jump_count = br * (insn - patch_insn);
	 }
      }

      /* Also pop item off the stack in the endif instruction:
       */
      if (intel->gen < 6) {
	 insn->bits3.if_else.jump_count = 0;
	 insn->bits3.if_else.pop_count = 1;
	 insn->bits3.if_else.pad0 = 0;
      } else {
	 insn->bits1.branch_gen6.jump_count = 2;
      }
   }
}

struct brw_instruction *brw_BREAK(struct brw_compile *p, int pop_count)
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_instruction *insn;

   insn = next_insn(p, BRW_OPCODE_BREAK);
   if (intel->gen >= 6) {
      brw_set_dest(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src0(insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src1(insn, brw_imm_d(0x0));
   } else {
      brw_set_dest(p, insn, brw_ip_reg());
      brw_set_src0(insn, brw_ip_reg());
      brw_set_src1(insn, brw_imm_d(0x0));
      insn->bits3.if_else.pad0 = 0;
      insn->bits3.if_else.pop_count = pop_count;
   }
   insn->header.compression_control = BRW_COMPRESSION_NONE;
   insn->header.execution_size = BRW_EXECUTE_8;

   return insn;
}

struct brw_instruction *gen6_CONT(struct brw_compile *p,
				  struct brw_instruction *do_insn)
{
   struct brw_instruction *insn;
   int br = 2;

   insn = next_insn(p, BRW_OPCODE_CONTINUE);
   brw_set_dest(p, insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   brw_set_src0(insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
   brw_set_dest(p, insn, brw_ip_reg());
   brw_set_src0(insn, brw_ip_reg());
   brw_set_src1(insn, brw_imm_d(0x0));

   insn->bits3.break_cont.uip = br * (do_insn - insn);

   insn->header.compression_control = BRW_COMPRESSION_NONE;
   insn->header.execution_size = BRW_EXECUTE_8;
   return insn;
}

struct brw_instruction *brw_CONT(struct brw_compile *p, int pop_count)
{
   struct brw_instruction *insn;
   insn = next_insn(p, BRW_OPCODE_CONTINUE);
   brw_set_dest(p, insn, brw_ip_reg());
   brw_set_src0(insn, brw_ip_reg());
   brw_set_src1(insn, brw_imm_d(0x0));
   insn->header.compression_control = BRW_COMPRESSION_NONE;
   insn->header.execution_size = BRW_EXECUTE_8;
   /* insn->header.mask_control = BRW_MASK_DISABLE; */
   insn->bits3.if_else.pad0 = 0;
   insn->bits3.if_else.pop_count = pop_count;
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
struct brw_instruction *brw_DO(struct brw_compile *p, GLuint execute_size)
{
   struct intel_context *intel = &p->brw->intel;

   if (intel->gen >= 6 || p->single_program_flow) {
      return &p->store[p->nr_insn];
   } else {
      struct brw_instruction *insn = next_insn(p, BRW_OPCODE_DO);

      /* Override the defaults for this instruction:
       */
      brw_set_dest(p, insn, brw_null_reg());
      brw_set_src0(insn, brw_null_reg());
      brw_set_src1(insn, brw_null_reg());

      insn->header.compression_control = BRW_COMPRESSION_NONE;
      insn->header.execution_size = execute_size;
      insn->header.predicate_control = BRW_PREDICATE_NONE;
      /* insn->header.mask_control = BRW_MASK_ENABLE; */
      /* insn->header.mask_control = BRW_MASK_DISABLE; */

      return insn;
   }
}



struct brw_instruction *brw_WHILE(struct brw_compile *p, 
                                  struct brw_instruction *do_insn)
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_instruction *insn;
   GLuint br = 1;

   if (intel->gen >= 5)
      br = 2;

   if (intel->gen >= 6) {
      insn = next_insn(p, BRW_OPCODE_WHILE);

      brw_set_dest(p, insn, brw_imm_w(0));
      insn->bits1.branch_gen6.jump_count = br * (do_insn - insn);
      brw_set_src0(insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));
      brw_set_src1(insn, retype(brw_null_reg(), BRW_REGISTER_TYPE_D));

      insn->header.execution_size = do_insn->header.execution_size;
      assert(insn->header.execution_size == BRW_EXECUTE_8);
   } else {
      if (p->single_program_flow) {
	 insn = next_insn(p, BRW_OPCODE_ADD);

	 brw_set_dest(p, insn, brw_ip_reg());
	 brw_set_src0(insn, brw_ip_reg());
	 brw_set_src1(insn, brw_imm_d((do_insn - insn) * 16));
	 insn->header.execution_size = BRW_EXECUTE_1;
      } else {
	 insn = next_insn(p, BRW_OPCODE_WHILE);

	 assert(do_insn->header.opcode == BRW_OPCODE_DO);

	 brw_set_dest(p, insn, brw_ip_reg());
	 brw_set_src0(insn, brw_ip_reg());
	 brw_set_src1(insn, brw_imm_d(0));

	 insn->header.execution_size = do_insn->header.execution_size;
	 insn->bits3.if_else.jump_count = br * (do_insn - insn + 1);
	 insn->bits3.if_else.pop_count = 0;
	 insn->bits3.if_else.pad0 = 0;
      }
   }
   insn->header.compression_control = BRW_COMPRESSION_NONE;
   p->current->header.predicate_control = BRW_PREDICATE_NONE;

   return insn;
}


/* FORWARD JUMPS:
 */
void brw_land_fwd_jump(struct brw_compile *p, 
		       struct brw_instruction *jmp_insn)
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_instruction *landing = &p->store[p->nr_insn];
   GLuint jmpi = 1;

   if (intel->gen >= 5)
       jmpi = 2;

   assert(jmp_insn->header.opcode == BRW_OPCODE_JMPI);
   assert(jmp_insn->bits1.da1.src1_reg_file == BRW_IMMEDIATE_VALUE);

   jmp_insn->bits3.ud = jmpi * ((landing - jmp_insn) - 1);
}



/* To integrate with the above, it makes sense that the comparison
 * instruction should populate the flag register.  It might be simpler
 * just to use the flag reg for most WM tasks?
 */
void brw_CMP(struct brw_compile *p,
	     struct brw_reg dest,
	     GLuint conditional,
	     struct brw_reg src0,
	     struct brw_reg src1)
{
   struct brw_instruction *insn = next_insn(p, BRW_OPCODE_CMP);

   insn->header.destreg__conditionalmod = conditional;
   brw_set_dest(p, insn, dest);
   brw_set_src0(insn, src0);
   brw_set_src1(insn, src1);

/*    guess_execution_size(insn, src0); */


   /* Make it so that future instructions will use the computed flag
    * value until brw_set_predicate_control_flag_value() is called
    * again.  
    */
   if (dest.file == BRW_ARCHITECTURE_REGISTER_FILE &&
       dest.nr == 0) {
      p->current->header.predicate_control = BRW_PREDICATE_NORMAL;
      p->flag_value = 0xff;
   }
}

/* Issue 'wait' instruction for n1, host could program MMIO
   to wake up thread. */
void brw_WAIT (struct brw_compile *p)
{
   struct brw_instruction *insn = next_insn(p, BRW_OPCODE_WAIT);
   struct brw_reg src = brw_notification_1_reg();

   brw_set_dest(p, insn, src);
   brw_set_src0(insn, src);
   brw_set_src1(insn, brw_null_reg());
   insn->header.execution_size = 0; /* must */
   insn->header.predicate_control = 0;
   insn->header.compression_control = 0;
}


/***********************************************************************
 * Helpers for the various SEND message types:
 */

/** Extended math function, float[8].
 */
void brw_math( struct brw_compile *p,
	       struct brw_reg dest,
	       GLuint function,
	       GLuint saturate,
	       GLuint msg_reg_nr,
	       struct brw_reg src,
	       GLuint data_type,
	       GLuint precision )
{
   struct intel_context *intel = &p->brw->intel;

   if (intel->gen >= 6) {
      struct brw_instruction *insn = next_insn(p, BRW_OPCODE_MATH);

      assert(dest.file == BRW_GENERAL_REGISTER_FILE);
      assert(src.file == BRW_GENERAL_REGISTER_FILE);

      assert(dest.hstride == BRW_HORIZONTAL_STRIDE_1);
      assert(src.hstride == BRW_HORIZONTAL_STRIDE_1);

      /* Source modifiers are ignored for extended math instructions. */
      assert(!src.negate);
      assert(!src.abs);

      if (function != BRW_MATH_FUNCTION_INT_DIV_QUOTIENT &&
	  function != BRW_MATH_FUNCTION_INT_DIV_QUOTIENT_AND_REMAINDER) {
	 assert(src.type == BRW_REGISTER_TYPE_F);
      }

      /* Math is the same ISA format as other opcodes, except that CondModifier
       * becomes FC[3:0] and ThreadCtrl becomes FC[5:4].
       */
      insn->header.destreg__conditionalmod = function;
      insn->header.saturate = saturate;

      brw_set_dest(p, insn, dest);
      brw_set_src0(insn, src);
      brw_set_src1(insn, brw_null_reg());
   } else {
      struct brw_instruction *insn = next_insn(p, BRW_OPCODE_SEND);
      GLuint msg_length = (function == BRW_MATH_FUNCTION_POW) ? 2 : 1;
      GLuint response_length = (function == BRW_MATH_FUNCTION_SINCOS) ? 2 : 1;
      /* Example code doesn't set predicate_control for send
       * instructions.
       */
      insn->header.predicate_control = 0;
      insn->header.destreg__conditionalmod = msg_reg_nr;

      brw_set_dest(p, insn, dest);
      brw_set_src0(insn, src);
      brw_set_math_message(p->brw,
			   insn,
			   msg_length, response_length,
			   function,
			   BRW_MATH_INTEGER_UNSIGNED,
			   precision,
			   saturate,
			   data_type);
   }
}

/** Extended math function, float[8].
 */
void brw_math2(struct brw_compile *p,
	       struct brw_reg dest,
	       GLuint function,
	       struct brw_reg src0,
	       struct brw_reg src1)
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_instruction *insn = next_insn(p, BRW_OPCODE_MATH);

   assert(intel->gen >= 6);
   (void) intel;


   assert(dest.file == BRW_GENERAL_REGISTER_FILE);
   assert(src0.file == BRW_GENERAL_REGISTER_FILE);
   assert(src1.file == BRW_GENERAL_REGISTER_FILE);

   assert(dest.hstride == BRW_HORIZONTAL_STRIDE_1);
   assert(src0.hstride == BRW_HORIZONTAL_STRIDE_1);
   assert(src1.hstride == BRW_HORIZONTAL_STRIDE_1);

   if (function != BRW_MATH_FUNCTION_INT_DIV_QUOTIENT &&
       function != BRW_MATH_FUNCTION_INT_DIV_QUOTIENT_AND_REMAINDER) {
      assert(src0.type == BRW_REGISTER_TYPE_F);
      assert(src1.type == BRW_REGISTER_TYPE_F);
   }

   /* Source modifiers are ignored for extended math instructions. */
   assert(!src0.negate);
   assert(!src0.abs);
   assert(!src1.negate);
   assert(!src1.abs);

   /* Math is the same ISA format as other opcodes, except that CondModifier
    * becomes FC[3:0] and ThreadCtrl becomes FC[5:4].
    */
   insn->header.destreg__conditionalmod = function;

   brw_set_dest(p, insn, dest);
   brw_set_src0(insn, src0);
   brw_set_src1(insn, src1);
}

/**
 * Extended math function, float[16].
 * Use 2 send instructions.
 */
void brw_math_16( struct brw_compile *p,
		  struct brw_reg dest,
		  GLuint function,
		  GLuint saturate,
		  GLuint msg_reg_nr,
		  struct brw_reg src,
		  GLuint precision )
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_instruction *insn;
   GLuint msg_length = (function == BRW_MATH_FUNCTION_POW) ? 2 : 1; 
   GLuint response_length = (function == BRW_MATH_FUNCTION_SINCOS) ? 2 : 1; 

   if (intel->gen >= 6) {
      insn = next_insn(p, BRW_OPCODE_MATH);

      /* Math is the same ISA format as other opcodes, except that CondModifier
       * becomes FC[3:0] and ThreadCtrl becomes FC[5:4].
       */
      insn->header.destreg__conditionalmod = function;
      insn->header.saturate = saturate;

      /* Source modifiers are ignored for extended math instructions. */
      assert(!src.negate);
      assert(!src.abs);

      brw_set_dest(p, insn, dest);
      brw_set_src0(insn, src);
      brw_set_src1(insn, brw_null_reg());
      return;
   }

   /* First instruction:
    */
   brw_push_insn_state(p);
   brw_set_predicate_control_flag_value(p, 0xff);
   brw_set_compression_control(p, BRW_COMPRESSION_NONE);

   insn = next_insn(p, BRW_OPCODE_SEND);
   insn->header.destreg__conditionalmod = msg_reg_nr;

   brw_set_dest(p, insn, dest);
   brw_set_src0(insn, src);
   brw_set_math_message(p->brw,
			insn, 
			msg_length, response_length, 
			function,
			BRW_MATH_INTEGER_UNSIGNED,
			precision,
			saturate,
			BRW_MATH_DATA_VECTOR);

   /* Second instruction:
    */
   insn = next_insn(p, BRW_OPCODE_SEND);
   insn->header.compression_control = BRW_COMPRESSION_2NDHALF;
   insn->header.destreg__conditionalmod = msg_reg_nr+1;

   brw_set_dest(p, insn, offset(dest,1));
   brw_set_src0(insn, src);
   brw_set_math_message(p->brw, 
			insn, 
			msg_length, response_length, 
			function,
			BRW_MATH_INTEGER_UNSIGNED,
			precision,
			saturate,
			BRW_MATH_DATA_VECTOR);

   brw_pop_insn_state(p);
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
				   GLuint offset)
{
   struct intel_context *intel = &p->brw->intel;
   uint32_t msg_control, msg_type;
   int mlen;

   if (intel->gen >= 6)
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
      brw_set_mask_control(p, BRW_MASK_DISABLE);
      brw_set_compression_control(p, BRW_COMPRESSION_NONE);

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
      struct brw_instruction *insn = next_insn(p, BRW_OPCODE_SEND);
      int send_commit_msg;
      struct brw_reg src_header = retype(brw_vec8_grf(0, 0),
					 BRW_REGISTER_TYPE_UW);

      if (insn->header.compression_control != BRW_COMPRESSION_NONE) {
	 insn->header.compression_control = BRW_COMPRESSION_NONE;
	 src_header = vec16(src_header);
      }
      assert(insn->header.predicate_control == BRW_PREDICATE_NONE);
      insn->header.destreg__conditionalmod = mrf.nr;

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
      if (intel->gen >= 6) {
	 dest = retype(vec16(brw_null_reg()), BRW_REGISTER_TYPE_UW);
	 send_commit_msg = 0;
      } else {
	 dest = src_header;
	 send_commit_msg = 1;
      }

      brw_set_dest(p, insn, dest);
      if (intel->gen >= 6) {
	 brw_set_src0(insn, mrf);
      } else {
	 brw_set_src0(insn, brw_null_reg());
      }

      if (intel->gen >= 6)
	 msg_type = GEN6_DATAPORT_WRITE_MESSAGE_OWORD_BLOCK_WRITE;
      else
	 msg_type = BRW_DATAPORT_WRITE_MESSAGE_OWORD_BLOCK_WRITE;

      brw_set_dp_write_message(p->brw,
			       insn,
			       255, /* binding table index (255=stateless) */
			       msg_control,
			       msg_type,
			       mlen,
			       GL_TRUE, /* header_present */
			       0, /* pixel scoreboard */
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
			     GLuint offset)
{
   struct intel_context *intel = &p->brw->intel;
   uint32_t msg_control;
   int rlen;

   if (intel->gen >= 6)
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
      brw_set_compression_control(p, BRW_COMPRESSION_NONE);
      brw_set_mask_control(p, BRW_MASK_DISABLE);

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
      struct brw_instruction *insn = next_insn(p, BRW_OPCODE_SEND);

      assert(insn->header.predicate_control == 0);
      insn->header.compression_control = BRW_COMPRESSION_NONE;
      insn->header.destreg__conditionalmod = mrf.nr;

      brw_set_dest(p, insn, dest);	/* UW? */
      if (intel->gen >= 6) {
	 brw_set_src0(insn, mrf);
      } else {
	 brw_set_src0(insn, brw_null_reg());
      }

      brw_set_dp_read_message(p->brw,
			      insn,
			      255, /* binding table index (255=stateless) */
			      msg_control,
			      BRW_DATAPORT_READ_MESSAGE_OWORD_BLOCK_READ, /* msg_type */
			      BRW_DATAPORT_READ_TARGET_RENDER_CACHE,
			      1, /* msg_length */
			      rlen);
   }
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
   struct intel_context *intel = &p->brw->intel;

   /* On newer hardware, offset is in units of owords. */
   if (intel->gen >= 6)
      offset /= 16;

   mrf = retype(mrf, BRW_REGISTER_TYPE_UD);

   brw_push_insn_state(p);
   brw_set_predicate_control(p, BRW_PREDICATE_NONE);
   brw_set_compression_control(p, BRW_COMPRESSION_NONE);
   brw_set_mask_control(p, BRW_MASK_DISABLE);

   brw_MOV(p, mrf, retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UD));

   /* set message header global offset field (reg 0, element 2) */
   brw_MOV(p,
	   retype(brw_vec1_reg(BRW_MESSAGE_REGISTER_FILE,
			       mrf.nr,
			       2), BRW_REGISTER_TYPE_UD),
	   brw_imm_ud(offset));

   struct brw_instruction *insn = next_insn(p, BRW_OPCODE_SEND);
   insn->header.destreg__conditionalmod = mrf.nr;

   /* cast dest to a uword[8] vector */
   dest = retype(vec8(dest), BRW_REGISTER_TYPE_UW);

   brw_set_dest(p, insn, dest);
   if (intel->gen >= 6) {
      brw_set_src0(insn, mrf);
   } else {
      brw_set_src0(insn, brw_null_reg());
   }

   brw_set_dp_read_message(p->brw,
			   insn,
			   bind_table_index,
			   BRW_DATAPORT_OWORD_BLOCK_1_OWORDLOW,
			   BRW_DATAPORT_READ_MESSAGE_OWORD_BLOCK_READ,
			   0, /* source cache = data cache */
			   1, /* msg_length */
			   1); /* response_length (1 reg, 2 owords!) */

   brw_pop_insn_state(p);
}

/**
 * Read a set of dwords from the data port Data Cache (const buffer).
 *
 * Location (in buffer) appears as UD offsets in the register after
 * the provided mrf header reg.
 */
void brw_dword_scattered_read(struct brw_compile *p,
			      struct brw_reg dest,
			      struct brw_reg mrf,
			      uint32_t bind_table_index)
{
   mrf = retype(mrf, BRW_REGISTER_TYPE_UD);

   brw_push_insn_state(p);
   brw_set_predicate_control(p, BRW_PREDICATE_NONE);
   brw_set_compression_control(p, BRW_COMPRESSION_NONE);
   brw_set_mask_control(p, BRW_MASK_DISABLE);
   brw_MOV(p, mrf, retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UD));
   brw_pop_insn_state(p);

   struct brw_instruction *insn = next_insn(p, BRW_OPCODE_SEND);
   insn->header.destreg__conditionalmod = mrf.nr;

   /* cast dest to a uword[8] vector */
   dest = retype(vec8(dest), BRW_REGISTER_TYPE_UW);

   brw_set_dest(p, insn, dest);
   brw_set_src0(insn, brw_null_reg());

   brw_set_dp_read_message(p->brw,
			   insn,
			   bind_table_index,
			   BRW_DATAPORT_DWORD_SCATTERED_BLOCK_8DWORDS,
			   BRW_DATAPORT_READ_MESSAGE_DWORD_SCATTERED_READ,
			   0, /* source cache = data cache */
			   2, /* msg_length */
			   1); /* response_length */
}



/**
 * Read float[4] constant(s) from VS constant buffer.
 * For relative addressing, two float[4] constants will be read into 'dest'.
 * Otherwise, one float[4] constant will be read into the lower half of 'dest'.
 */
void brw_dp_READ_4_vs(struct brw_compile *p,
                      struct brw_reg dest,
                      GLuint location,
                      GLuint bind_table_index)
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_instruction *insn;
   GLuint msg_reg_nr = 1;

   if (intel->gen >= 6)
      location /= 16;

   /* Setup MRF[1] with location/offset into const buffer */
   brw_push_insn_state(p);
   brw_set_access_mode(p, BRW_ALIGN_1);
   brw_set_compression_control(p, BRW_COMPRESSION_NONE);
   brw_set_mask_control(p, BRW_MASK_DISABLE);
   brw_set_predicate_control(p, BRW_PREDICATE_NONE);
   brw_MOV(p, retype(brw_vec1_reg(BRW_MESSAGE_REGISTER_FILE, msg_reg_nr, 2),
		     BRW_REGISTER_TYPE_UD),
	   brw_imm_ud(location));
   brw_pop_insn_state(p);

   insn = next_insn(p, BRW_OPCODE_SEND);

   insn->header.predicate_control = BRW_PREDICATE_NONE;
   insn->header.compression_control = BRW_COMPRESSION_NONE;
   insn->header.destreg__conditionalmod = msg_reg_nr;
   insn->header.mask_control = BRW_MASK_DISABLE;

   brw_set_dest(p, insn, dest);
   if (intel->gen >= 6) {
      brw_set_src0(insn, brw_message_reg(msg_reg_nr));
   } else {
      brw_set_src0(insn, brw_null_reg());
   }

   brw_set_dp_read_message(p->brw,
			   insn,
			   bind_table_index,
			   0,
			   BRW_DATAPORT_READ_MESSAGE_OWORD_BLOCK_READ, /* msg_type */
			   0, /* source cache = data cache */
			   1, /* msg_length */
			   1); /* response_length (1 Oword) */
}

/**
 * Read a float[4] constant per vertex from VS constant buffer, with
 * relative addressing.
 */
void brw_dp_READ_4_vs_relative(struct brw_compile *p,
			       struct brw_reg dest,
			       struct brw_reg addr_reg,
			       GLuint offset,
			       GLuint bind_table_index)
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_reg src = brw_vec8_grf(0, 0);
   int msg_type;

   /* Setup MRF[1] with offset into const buffer */
   brw_push_insn_state(p);
   brw_set_access_mode(p, BRW_ALIGN_1);
   brw_set_compression_control(p, BRW_COMPRESSION_NONE);
   brw_set_mask_control(p, BRW_MASK_DISABLE);
   brw_set_predicate_control(p, BRW_PREDICATE_NONE);

   /* M1.0 is block offset 0, M1.4 is block offset 1, all other
    * fields ignored.
    */
   brw_ADD(p, retype(brw_message_reg(1), BRW_REGISTER_TYPE_D),
	   addr_reg, brw_imm_d(offset));
   brw_pop_insn_state(p);

   gen6_resolve_implied_move(p, &src, 0);
   struct brw_instruction *insn = next_insn(p, BRW_OPCODE_SEND);

   insn->header.predicate_control = BRW_PREDICATE_NONE;
   insn->header.compression_control = BRW_COMPRESSION_NONE;
   insn->header.destreg__conditionalmod = 0;
   insn->header.mask_control = BRW_MASK_DISABLE;

   brw_set_dest(p, insn, dest);
   brw_set_src0(insn, src);

   if (intel->gen == 6)
      msg_type = GEN6_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ;
   else if (intel->gen == 5 || intel->is_g4x)
      msg_type = G45_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ;
   else
      msg_type = BRW_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ;

   brw_set_dp_read_message(p->brw,
			   insn,
			   bind_table_index,
			   BRW_DATAPORT_OWORD_DUAL_BLOCK_1OWORD,
			   msg_type,
			   BRW_DATAPORT_READ_TARGET_DATA_CACHE,
			   2, /* msg_length */
			   1); /* response_length */
}



void brw_fb_WRITE(struct brw_compile *p,
		  int dispatch_width,
                  struct brw_reg dest,
                  GLuint msg_reg_nr,
                  struct brw_reg src0,
                  GLuint binding_table_index,
                  GLuint msg_length,
                  GLuint response_length,
                  GLboolean eot,
                  GLboolean header_present)
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_instruction *insn;
   GLuint msg_control, msg_type;

   if (intel->gen >= 6 && binding_table_index == 0) {
      insn = next_insn(p, BRW_OPCODE_SENDC);
   } else {
      insn = next_insn(p, BRW_OPCODE_SEND);
   }
   /* The execution mask is ignored for render target writes. */
   insn->header.predicate_control = 0;
   insn->header.compression_control = BRW_COMPRESSION_NONE;

   if (intel->gen >= 6) {
       /* headerless version, just submit color payload */
       src0 = brw_message_reg(msg_reg_nr);

       msg_type = GEN6_DATAPORT_WRITE_MESSAGE_RENDER_TARGET_WRITE;
   } else {
      insn->header.destreg__conditionalmod = msg_reg_nr;

      msg_type = BRW_DATAPORT_WRITE_MESSAGE_RENDER_TARGET_WRITE;
   }

   if (dispatch_width == 16)
      msg_control = BRW_DATAPORT_RENDER_TARGET_WRITE_SIMD16_SINGLE_SOURCE;
   else
      msg_control = BRW_DATAPORT_RENDER_TARGET_WRITE_SIMD8_SINGLE_SOURCE_SUBSPAN01;

   brw_set_dest(p, insn, dest);
   brw_set_src0(insn, src0);
   brw_set_dp_write_message(p->brw,
			    insn,
			    binding_table_index,
			    msg_control,
			    msg_type,
			    msg_length,
			    header_present,
			    1,	/* pixel scoreboard */
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
		GLuint msg_reg_nr,
		struct brw_reg src0,
		GLuint binding_table_index,
		GLuint sampler,
		GLuint writemask,
		GLuint msg_type,
		GLuint response_length,
		GLuint msg_length,
		GLboolean eot,
		GLuint header_present,
		GLuint simd_mode)
{
   struct intel_context *intel = &p->brw->intel;
   GLboolean need_stall = 0;

   if (writemask == 0) {
      /*printf("%s: zero writemask??\n", __FUNCTION__); */
      return;
   }
   
   /* Hardware doesn't do destination dependency checking on send
    * instructions properly.  Add a workaround which generates the
    * dependency by other means.  In practice it seems like this bug
    * only crops up for texture samples, and only where registers are
    * written by the send and then written again later without being
    * read in between.  Luckily for us, we already track that
    * information and use it to modify the writemask for the
    * instruction, so that is a guide for whether a workaround is
    * needed.
    */
   if (writemask != WRITEMASK_XYZW) {
      GLuint dst_offset = 0;
      GLuint i, newmask = 0, len = 0;

      for (i = 0; i < 4; i++) {
	 if (writemask & (1<<i))
	    break;
	 dst_offset += 2;
      }
      for (; i < 4; i++) {
	 if (!(writemask & (1<<i)))
	    break;
	 newmask |= 1<<i;
	 len++;
      }

      if (newmask != writemask) {
	 need_stall = 1;
         /* printf("need stall %x %x\n", newmask , writemask); */
      }
      else {
	 GLboolean dispatch_16 = GL_FALSE;

	 struct brw_reg m1 = brw_message_reg(msg_reg_nr);

	 guess_execution_size(p, p->current, dest);
	 if (p->current->header.execution_size == BRW_EXECUTE_16)
	    dispatch_16 = GL_TRUE;

	 newmask = ~newmask & WRITEMASK_XYZW;

	 brw_push_insn_state(p);

	 brw_set_compression_control(p, BRW_COMPRESSION_NONE);
	 brw_set_mask_control(p, BRW_MASK_DISABLE);

	 brw_MOV(p, retype(m1, BRW_REGISTER_TYPE_UD),
		 retype(brw_vec8_grf(0,0), BRW_REGISTER_TYPE_UD));
  	 brw_MOV(p, get_element_ud(m1, 2), brw_imm_ud(newmask << 12)); 

	 brw_pop_insn_state(p);

  	 src0 = retype(brw_null_reg(), BRW_REGISTER_TYPE_UW); 
	 dest = offset(dest, dst_offset);

	 /* For 16-wide dispatch, masked channels are skipped in the
	  * response.  For 8-wide, masked channels still take up slots,
	  * and are just not written to.
	  */
	 if (dispatch_16)
	    response_length = len * 2;
      }
   }

   {
      struct brw_instruction *insn;
   
      gen6_resolve_implied_move(p, &src0, msg_reg_nr);

      insn = next_insn(p, BRW_OPCODE_SEND);
      insn->header.predicate_control = 0; /* XXX */
      insn->header.compression_control = BRW_COMPRESSION_NONE;
      if (intel->gen < 6)
	  insn->header.destreg__conditionalmod = msg_reg_nr;

      brw_set_dest(p, insn, dest);
      brw_set_src0(insn, src0);
      brw_set_sampler_message(p->brw, insn,
			      binding_table_index,
			      sampler,
			      msg_type,
			      response_length, 
			      msg_length,
			      eot,
			      header_present,
			      simd_mode);
   }

   if (need_stall) {
      struct brw_reg reg = vec8(offset(dest, response_length-1));

      /*  mov (8) r9.0<1>:f    r9.0<8;8,1>:f    { Align1 }
       */
      brw_push_insn_state(p);
      brw_set_compression_control(p, BRW_COMPRESSION_NONE);
      brw_MOV(p, retype(reg, BRW_REGISTER_TYPE_UD),
	      retype(reg, BRW_REGISTER_TYPE_UD));
      brw_pop_insn_state(p);
   }

}

/* All these variables are pretty confusing - we might be better off
 * using bitmasks and macros for this, in the old style.  Or perhaps
 * just having the caller instantiate the fields in dword3 itself.
 */
void brw_urb_WRITE(struct brw_compile *p,
		   struct brw_reg dest,
		   GLuint msg_reg_nr,
		   struct brw_reg src0,
		   GLboolean allocate,
		   GLboolean used,
		   GLuint msg_length,
		   GLuint response_length,
		   GLboolean eot,
		   GLboolean writes_complete,
		   GLuint offset,
		   GLuint swizzle)
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_instruction *insn;

   gen6_resolve_implied_move(p, &src0, msg_reg_nr);

   insn = next_insn(p, BRW_OPCODE_SEND);

   assert(msg_length < BRW_MAX_MRF);

   brw_set_dest(p, insn, dest);
   brw_set_src0(insn, src0);
   brw_set_src1(insn, brw_imm_d(0));

   if (intel->gen < 6)
      insn->header.destreg__conditionalmod = msg_reg_nr;

   brw_set_urb_message(p->brw,
		       insn,
		       allocate,
		       used,
		       msg_length,
		       response_length, 
		       eot, 
		       writes_complete, 
		       offset,
		       swizzle);
}

static int
brw_find_next_block_end(struct brw_compile *p, int start)
{
   int ip;

   for (ip = start + 1; ip < p->nr_insn; ip++) {
      struct brw_instruction *insn = &p->store[ip];

      switch (insn->header.opcode) {
      case BRW_OPCODE_ENDIF:
      case BRW_OPCODE_ELSE:
      case BRW_OPCODE_WHILE:
	 return ip;
      }
   }
   assert(!"not reached");
   return start + 1;
}

/* There is no DO instruction on gen6, so to find the end of the loop
 * we have to see if the loop is jumping back before our start
 * instruction.
 */
static int
brw_find_loop_end(struct brw_compile *p, int start)
{
   int ip;
   int br = 2;

   for (ip = start + 1; ip < p->nr_insn; ip++) {
      struct brw_instruction *insn = &p->store[ip];

      if (insn->header.opcode == BRW_OPCODE_WHILE) {
	 if (ip + insn->bits1.branch_gen6.jump_count / br < start)
	    return ip;
      }
   }
   assert(!"not reached");
   return start + 1;
}

/* After program generation, go back and update the UIP and JIP of
 * BREAK and CONT instructions to their correct locations.
 */
void
brw_set_uip_jip(struct brw_compile *p)
{
   struct intel_context *intel = &p->brw->intel;
   int ip;
   int br = 2;

   if (intel->gen < 6)
      return;

   for (ip = 0; ip < p->nr_insn; ip++) {
      struct brw_instruction *insn = &p->store[ip];

      switch (insn->header.opcode) {
      case BRW_OPCODE_BREAK:
	 insn->bits3.break_cont.jip = br * (brw_find_next_block_end(p, ip) - ip);
	 insn->bits3.break_cont.uip = br * (brw_find_loop_end(p, ip) - ip + 1);
	 break;
      case BRW_OPCODE_CONTINUE:
	 /* JIP is set at CONTINUE emit time, since that's when we
	  * know where the start of the loop is.
	  */
	 insn->bits3.break_cont.jip = br * (brw_find_next_block_end(p, ip) - ip);
	 assert(insn->bits3.break_cont.uip != 0);
	 assert(insn->bits3.break_cont.jip != 0);
	 break;
      }
   }
}

void brw_ff_sync(struct brw_compile *p,
		   struct brw_reg dest,
		   GLuint msg_reg_nr,
		   struct brw_reg src0,
		   GLboolean allocate,
		   GLuint response_length,
		   GLboolean eot)
{
   struct intel_context *intel = &p->brw->intel;
   struct brw_instruction *insn;

   gen6_resolve_implied_move(p, &src0, msg_reg_nr);

   insn = next_insn(p, BRW_OPCODE_SEND);
   brw_set_dest(p, insn, dest);
   brw_set_src0(insn, src0);
   brw_set_src1(insn, brw_imm_d(0));

   if (intel->gen < 6)
       insn->header.destreg__conditionalmod = msg_reg_nr;

   brw_set_ff_sync_message(p->brw,
			   insn,
			   allocate,
			   response_length,
			   eot);
}
