
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_wm.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_parse.h"



static int get_scalar_dst_index(struct tgsi_full_instruction *inst)
{
   struct tgsi_dst_register dst = inst->FullDstRegisters[0].DstRegister;
   int i;
   for (i = 0; i < 4; i++)
      if (dst.WriteMask & (1<<i))
	 break;
   return i;
}

static struct brw_reg alloc_tmp(struct brw_wm_compile *c)
{
   c->tmp_index++;
   c->reg_index = MAX2(c->reg_index, c->tmp_index);
   return brw_vec8_grf(c->tmp_start + c->tmp_index, 0);
}

static void release_tmps(struct brw_wm_compile *c)
{
   c->tmp_index = 0;
}


static struct brw_reg
get_reg(struct brw_wm_compile *c, int file, int index, int component )
{
   switch (file) {
   case TGSI_FILE_NULL:
      return brw_null_reg();

   case TGSI_FILE_SAMPLER:
      /* Should never get here:
       */
      assert (0);	       
      return brw_null_reg();

   case TGSI_FILE_IMMEDIATE:
      /* These need a different path:
       */
      assert(0);
      return brw_null_reg();

       
   case TGSI_FILE_CONSTANT:
   case TGSI_FILE_INPUT:
   case TGSI_FILE_OUTPUT:
   case TGSI_FILE_TEMPORARY:
   case TGSI_FILE_ADDRESS:
      return c->wm_regs[file][index][component];

   default:
      assert(0);
      return brw_null_reg();
   }
}


static struct brw_reg get_dst_reg(struct brw_wm_compile *c,
				  struct tgsi_full_instruction *inst, 
				  int component)
{
   return get_reg(c, 
		  inst->FullDstRegisters[0].DstRegister.File, 
		  inst->FullDstRegisters[0].DstRegister.Index,
		  component);
}

static int get_swz( struct tgsi_src_register src, int index )
{
   switch (index & 3) {
   case 0: return src.SwizzleX;
   case 1: return src.SwizzleY;
   case 2: return src.SwizzleZ;
   case 3: return src.SwizzleW;
   default: return 0;
   }
}

static int get_ext_swz( struct tgsi_src_register_ext_swz src, int index )
{
   switch (index & 3) {
   case 0: return src.ExtSwizzleX;
   case 1: return src.ExtSwizzleY;
   case 2: return src.ExtSwizzleZ;
   case 3: return src.ExtSwizzleW;
   default: return 0;
   }
}

static struct brw_reg get_src_reg(struct brw_wm_compile *c,
				  struct tgsi_full_src_register *src, 
				  int index)
{
   struct brw_reg reg;
   int component = index;
   int neg = 0;
   int abs = 0;

   if (src->SrcRegister.Negate)
      neg = 1;

   component = get_swz(src->SrcRegister, component);

   /* Yes, there are multiple negates:
    */
   switch (component & 3) {
   case 0: neg ^= src->SrcRegisterExtSwz.NegateX; break;
   case 1: neg ^= src->SrcRegisterExtSwz.NegateY; break;
   case 2: neg ^= src->SrcRegisterExtSwz.NegateZ; break;
   case 3: neg ^= src->SrcRegisterExtSwz.NegateW; break;
   }

   /* And multiple swizzles, fun isn't it:
    */
   component = get_ext_swz(src->SrcRegisterExtSwz, component);

   /* Not handling indirect lookups yet:
    */
   assert(src->SrcRegister.Indirect == 0);

   /* Don't know what dimension means:
    */
   assert(src->SrcRegister.Dimension == 0);

   /* Will never handle any of this stuff: 
    */
   assert(src->SrcRegisterExtMod.Complement == 0);
   assert(src->SrcRegisterExtMod.Bias == 0);
   assert(src->SrcRegisterExtMod.Scale2X == 0);

   if (src->SrcRegisterExtMod.Absolute)
      abs = 1;

   /* Another negate!  This is a post-absolute negate, which we
    * can't do.  Need to clean the crap out of tgsi somehow.
    */
   assert(src->SrcRegisterExtMod.Negate == 0);

   switch( component ) {
   case TGSI_EXTSWIZZLE_X:
   case TGSI_EXTSWIZZLE_Y:
   case TGSI_EXTSWIZZLE_Z:
   case TGSI_EXTSWIZZLE_W:
      reg = get_reg(c, 
		    src->SrcRegister.File, 
		    src->SrcRegister.Index, 
		    component );

      if (neg) 
	 reg = negate(reg);
   
      if (abs)
	 reg = brw_abs(reg);

      break;

      /* XXX: this won't really work in the general case, but we know
       * that the extended swizzle is only allowed in the SWZ
       * instruction (right??), in which case using an immediate
       * directly will work.
       */
   case TGSI_EXTSWIZZLE_ZERO:
      reg = brw_imm_f(0);
      break;

   case TGSI_EXTSWIZZLE_ONE:
      if (neg && !abs)
	 reg = brw_imm_f(-1.0);
      else
	 reg = brw_imm_f(1.0);
      break;

   default:
      assert(0);
      break;
   }

    
   return reg;
}

static void emit_abs( struct brw_wm_compile *c,
		      struct tgsi_full_instruction *inst)
{
   unsigned mask = inst->FullDstRegisters[0].DstRegister.WriteMask;

   int i;
   struct brw_compile *p = &c->func;
   brw_set_saturate(p, inst->Instruction.Saturate != TGSI_SAT_NONE);
   for (i = 0; i < 4; i++) {
      if (mask & (1<<i)) {
	 struct brw_reg src, dst;
	 dst = get_dst_reg(c, inst, i);
	 src = get_src_reg(c, &inst->FullSrcRegisters[0], i);
	 brw_MOV(p, dst, brw_abs(src)); /* NOTE */
      }
   }
   brw_set_saturate(p, 0);
}


static void emit_xpd(struct brw_wm_compile *c,
		     struct tgsi_full_instruction *inst)
{
   int i;
   struct brw_compile *p = &c->func;
   unsigned mask = inst->FullDstRegisters[0].DstRegister.WriteMask;
   for (i = 0; i < 4; i++) {
      unsigned i2 = (i+2)%3;
      unsigned i1 = (i+1)%3;
      if (mask & (1<<i)) {
	 struct brw_reg src0, src1, dst;
	 dst = get_dst_reg(c, inst, i);
	 src0 = negate(get_src_reg(c, &inst->FullSrcRegisters[0], i2));
	 src1 = get_src_reg(c, &inst->FullSrcRegisters[1], i1);
	 brw_MUL(p, brw_null_reg(), src0, src1);
	 src0 = get_src_reg(c, &inst->FullSrcRegisters[0], i1);
	 src1 = get_src_reg(c, &inst->FullSrcRegisters[1], i2);
	 brw_set_saturate(p, inst->Instruction.Saturate != TGSI_SAT_NONE);
	 brw_MAC(p, dst, src0, src1);
	 brw_set_saturate(p, 0);
      }
   }
   brw_set_saturate(p, 0);
}

static void emit_dp3(struct brw_wm_compile *c,
		     struct tgsi_full_instruction *inst)
{
   struct brw_reg src0[3], src1[3], dst;
   int i;
   struct brw_compile *p = &c->func;
   for (i = 0; i < 3; i++) {
      src0[i] = get_src_reg(c, &inst->FullSrcRegisters[0], i);
      src1[i] = get_src_reg(c, &inst->FullSrcRegisters[1], i);
   }

   dst = get_dst_reg(c, inst, get_scalar_dst_index(inst));
   brw_MUL(p, brw_null_reg(), src0[0], src1[0]);
   brw_MAC(p, brw_null_reg(), src0[1], src1[1]);
   brw_set_saturate(p, (inst->Instruction.Saturate != TGSI_SAT_NONE) ? 1 : 0);
   brw_MAC(p, dst, src0[2], src1[2]);
   brw_set_saturate(p, 0);
}

static void emit_dp4(struct brw_wm_compile *c,
		     struct tgsi_full_instruction *inst)
{
   struct brw_reg src0[4], src1[4], dst;
   int i;
   struct brw_compile *p = &c->func;
   for (i = 0; i < 4; i++) {
      src0[i] = get_src_reg(c, &inst->FullSrcRegisters[0], i);
      src1[i] = get_src_reg(c, &inst->FullSrcRegisters[1], i);
   }
   dst = get_dst_reg(c, inst, get_scalar_dst_index(inst));
   brw_MUL(p, brw_null_reg(), src0[0], src1[0]);
   brw_MAC(p, brw_null_reg(), src0[1], src1[1]);
   brw_MAC(p, brw_null_reg(), src0[2], src1[2]);
   brw_set_saturate(p, (inst->Instruction.Saturate != TGSI_SAT_NONE) ? 1 : 0);
   brw_MAC(p, dst, src0[3], src1[3]);
   brw_set_saturate(p, 0);
}

static void emit_dph(struct brw_wm_compile *c,
		     struct tgsi_full_instruction *inst)
{
   struct brw_reg src0[4], src1[4], dst;
   int i;
   struct brw_compile *p = &c->func;
   for (i = 0; i < 4; i++) {
      src0[i] = get_src_reg(c, &inst->FullSrcRegisters[0], i);
      src1[i] = get_src_reg(c, &inst->FullSrcRegisters[1], i);
   }
   dst = get_dst_reg(c, inst, get_scalar_dst_index(inst));
   brw_MUL(p, brw_null_reg(), src0[0], src1[0]);
   brw_MAC(p, brw_null_reg(), src0[1], src1[1]);
   brw_MAC(p, dst, src0[2], src1[2]);
   brw_set_saturate(p, (inst->Instruction.Saturate != TGSI_SAT_NONE) ? 1 : 0);
   brw_ADD(p, dst, src0[3], src1[3]);
   brw_set_saturate(p, 0);
}

static void emit_math1(struct brw_wm_compile *c,
		       struct tgsi_full_instruction *inst, unsigned func)
{
   struct brw_compile *p = &c->func;
   struct brw_reg src0, dst;

   src0 = get_src_reg(c, &inst->FullSrcRegisters[0], 0);
   dst = get_dst_reg(c, inst, get_scalar_dst_index(inst));
   brw_MOV(p, brw_message_reg(2), src0);
   brw_math(p,
	    dst,
	    func,
	    ((inst->Instruction.Saturate != TGSI_SAT_NONE) 
	     ? BRW_MATH_SATURATE_SATURATE 
	     : BRW_MATH_SATURATE_NONE),
	    2,
	    brw_null_reg(),
	    BRW_MATH_DATA_VECTOR,
	    BRW_MATH_PRECISION_FULL);
}


static void emit_alu2(struct brw_wm_compile *c,		      
		      struct tgsi_full_instruction *inst,
		      unsigned opcode)
{
   struct brw_compile *p = &c->func;
   struct brw_reg src0, src1, dst;
   unsigned mask = inst->FullDstRegisters[0].DstRegister.WriteMask;
   int i;
   brw_set_saturate(p, (inst->Instruction.Saturate != TGSI_SAT_NONE) ? 1 : 0);
   for (i = 0 ; i < 4; i++) {
      if (mask & (1<<i)) {
	 dst = get_dst_reg(c, inst, i);
	 src0 = get_src_reg(c, &inst->FullSrcRegisters[0], i);
	 src1 = get_src_reg(c, &inst->FullSrcRegisters[1], i);
	 brw_alu2(p, opcode, dst, src0, src1);
      }
   }
   brw_set_saturate(p, 0);
}


static void emit_alu1(struct brw_wm_compile *c,
		      struct tgsi_full_instruction *inst,
		      unsigned opcode)
{
   struct brw_compile *p = &c->func;
   struct brw_reg src0, dst;
   unsigned mask = inst->FullDstRegisters[0].DstRegister.WriteMask;
   int i;
   brw_set_saturate(p, (inst->Instruction.Saturate != TGSI_SAT_NONE) ? 1 : 0);
   for (i = 0 ; i < 4; i++) {
      if (mask & (1<<i)) {
	 dst = get_dst_reg(c, inst, i);
	 src0 = get_src_reg(c, &inst->FullSrcRegisters[0], i);
	 brw_alu1(p, opcode, dst, src0);
      }
   }
   if (inst->Instruction.Saturate != TGSI_SAT_NONE)
      brw_set_saturate(p, 0);
}


static void emit_max(struct brw_wm_compile *c,
		     struct tgsi_full_instruction *inst)
{
   struct brw_compile *p = &c->func;
   unsigned mask = inst->FullDstRegisters[0].DstRegister.WriteMask;
   struct brw_reg src0, src1, dst;
   int i;
   brw_push_insn_state(p);
   for (i = 0; i < 4; i++) {
      if (mask & (1<<i)) {
	 dst = get_dst_reg(c, inst, i);
	 src0 = get_src_reg(c, &inst->FullSrcRegisters[0], i);
	 src1 = get_src_reg(c, &inst->FullSrcRegisters[1], i);
	 brw_set_saturate(p, (inst->Instruction.Saturate != TGSI_SAT_NONE) ? 1 : 0);
	 brw_MOV(p, dst, src0);
	 brw_set_saturate(p, 0);

	 brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_L, src0, src1);
	 brw_set_saturate(p, (inst->Instruction.Saturate != TGSI_SAT_NONE) ? 1 : 0);
	 brw_set_predicate_control(p, BRW_PREDICATE_NORMAL);
	 brw_MOV(p, dst, src1);
	 brw_set_saturate(p, 0);
	 brw_set_predicate_control_flag_value(p, 0xff);
      }
   }
   brw_pop_insn_state(p);
}

static void emit_min(struct brw_wm_compile *c,
		     struct tgsi_full_instruction *inst)
{
   struct brw_compile *p = &c->func;
   unsigned mask = inst->FullDstRegisters[0].DstRegister.WriteMask;
   struct brw_reg src0, src1, dst;
   int i;
   brw_push_insn_state(p);
   for (i = 0; i < 4; i++) {
      if (mask & (1<<i)) {
	 dst = get_dst_reg(c, inst, i);
	 src0 = get_src_reg(c, &inst->FullSrcRegisters[0], i);
	 src1 = get_src_reg(c, &inst->FullSrcRegisters[1], i);
	 brw_set_saturate(p, (inst->Instruction.Saturate != TGSI_SAT_NONE) ? 1 : 0);
	 brw_MOV(p, dst, src0);
	 brw_set_saturate(p, 0);

	 brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_L, src1, src0);
	 brw_set_saturate(p, (inst->Instruction.Saturate != TGSI_SAT_NONE) ? 1 : 0);
	 brw_set_predicate_control(p, BRW_PREDICATE_NORMAL);
	 brw_MOV(p, dst, src1);
	 brw_set_saturate(p, 0);
	 brw_set_predicate_control_flag_value(p, 0xff);
      }
   }
   brw_pop_insn_state(p);
}

static void emit_pow(struct brw_wm_compile *c,
		     struct tgsi_full_instruction *inst)
{
   struct brw_compile *p = &c->func;
   struct brw_reg dst, src0, src1;
   dst = get_dst_reg(c, inst, get_scalar_dst_index(inst));
   src0 = get_src_reg(c, &inst->FullSrcRegisters[0], 0);
   src1 = get_src_reg(c, &inst->FullSrcRegisters[1], 0);

   brw_MOV(p, brw_message_reg(2), src0);
   brw_MOV(p, brw_message_reg(3), src1);

   brw_math(p,
	    dst,
	    BRW_MATH_FUNCTION_POW,
	    (inst->Instruction.Saturate != TGSI_SAT_NONE 
	     ? BRW_MATH_SATURATE_SATURATE 
	     : BRW_MATH_SATURATE_NONE),
	    2,
	    brw_null_reg(),
	    BRW_MATH_DATA_VECTOR,
	    BRW_MATH_PRECISION_FULL);
}

static void emit_lrp(struct brw_wm_compile *c,
		     struct tgsi_full_instruction *inst)
{
   struct brw_compile *p = &c->func;
   unsigned mask = inst->FullDstRegisters[0].DstRegister.WriteMask;
   struct brw_reg dst, tmp1, tmp2, src0, src1, src2;
   int i;
   for (i = 0; i < 4; i++) {
      if (mask & (1<<i)) {
	 dst = get_dst_reg(c, inst, i);
	 src0 = get_src_reg(c, &inst->FullSrcRegisters[0], i);

	 src1 = get_src_reg(c, &inst->FullSrcRegisters[1], i);

	 if (src1.nr == dst.nr) {
	    tmp1 = alloc_tmp(c);
	    brw_MOV(p, tmp1, src1);
	 } else
	    tmp1 = src1;

	 src2 = get_src_reg(c, &inst->FullSrcRegisters[2], i);
	 if (src2.nr == dst.nr) {
	    tmp2 = alloc_tmp(c);
	    brw_MOV(p, tmp2, src2);
	 } else
	    tmp2 = src2;

	 brw_ADD(p, dst, negate(src0), brw_imm_f(1.0));
	 brw_MUL(p, brw_null_reg(), dst, tmp2);
	 brw_set_saturate(p, (inst->Instruction.Saturate != TGSI_SAT_NONE) ? 1 : 0);
	 brw_MAC(p, dst, src0, tmp1);
	 brw_set_saturate(p, 0);
      }
      release_tmps(c);
   }
}

static void emit_kil(struct brw_wm_compile *c)
{
   struct brw_compile *p = &c->func;
   struct brw_reg depth = retype(brw_vec1_grf(0, 0), BRW_REGISTER_TYPE_UW);
   brw_push_insn_state(p);
   brw_set_mask_control(p, BRW_MASK_DISABLE);
   brw_NOT(p, c->emit_mask_reg, brw_mask_reg(1)); //IMASK
   brw_AND(p, depth, c->emit_mask_reg, depth);
   brw_pop_insn_state(p);
}

static void emit_mad(struct brw_wm_compile *c,
		     struct tgsi_full_instruction *inst)
{
   struct brw_compile *p = &c->func;
   unsigned mask = inst->FullDstRegisters[0].DstRegister.WriteMask;
   struct brw_reg dst, src0, src1, src2;
   int i;

   for (i = 0; i < 4; i++) {
      if (mask & (1<<i)) {
	 dst = get_dst_reg(c, inst, i);
	 src0 = get_src_reg(c, &inst->FullSrcRegisters[0], i);
	 src1 = get_src_reg(c, &inst->FullSrcRegisters[1], i);
	 src2 = get_src_reg(c, &inst->FullSrcRegisters[2], i);
	 brw_MUL(p, dst, src0, src1);

	 brw_set_saturate(p, (inst->Instruction.Saturate != TGSI_SAT_NONE) ? 1 : 0);
	 brw_ADD(p, dst, dst, src2);
	 brw_set_saturate(p, 0);
      }
   }
}

static void emit_sop(struct brw_wm_compile *c,
		     struct tgsi_full_instruction *inst, unsigned cond)
{
   struct brw_compile *p = &c->func;
   unsigned mask = inst->FullDstRegisters[0].DstRegister.WriteMask;
   struct brw_reg dst, src0, src1;
   int i;

   brw_push_insn_state(p);
   for (i = 0; i < 4; i++) {
      if (mask & (1<<i)) {
	 dst = get_dst_reg(c, inst, i);
	 src0 = get_src_reg(c, &inst->FullSrcRegisters[0], i);
	 src1 = get_src_reg(c, &inst->FullSrcRegisters[1], i);
	 brw_CMP(p, brw_null_reg(), cond, src0, src1);
	 brw_set_predicate_control(p, BRW_PREDICATE_NONE);
	 brw_MOV(p, dst, brw_imm_f(0.0));
	 brw_set_predicate_control(p, BRW_PREDICATE_NORMAL);
	 brw_MOV(p, dst, brw_imm_f(1.0));
      }
   }
   brw_pop_insn_state(p);
}


static void emit_ddx(struct brw_wm_compile *c,
		     struct tgsi_full_instruction *inst)
{
   struct brw_compile *p = &c->func;
   unsigned mask = inst->FullDstRegisters[0].DstRegister.WriteMask;
   struct brw_reg interp[4];
   struct brw_reg dst;
   struct brw_reg src0, w;
   unsigned nr, i;
   src0 = get_src_reg(c, &inst->FullSrcRegisters[0], 0);
   w = get_src_reg(c, &inst->FullSrcRegisters[1], 3);
   nr = src0.nr;
   interp[0] = brw_vec1_grf(nr, 0);
   interp[1] = brw_vec1_grf(nr, 4);
   interp[2] = brw_vec1_grf(nr+1, 0);
   interp[3] = brw_vec1_grf(nr+1, 4);
   brw_set_saturate(p, inst->Instruction.Saturate != TGSI_SAT_NONE);
   for(i = 0; i < 4; i++ ) {
      if (mask & (1<<i)) {
	 dst = get_dst_reg(c, inst, i);
	 brw_MOV(p, dst, interp[i]);
	 brw_MUL(p, dst, dst, w);
      }
   }
   brw_set_saturate(p, 0);
}

static void emit_ddy(struct brw_wm_compile *c,
		     struct tgsi_full_instruction *inst)
{
   struct brw_compile *p = &c->func;
   unsigned mask = inst->FullDstRegisters[0].DstRegister.WriteMask;
   struct brw_reg interp[4];
   struct brw_reg dst;
   struct brw_reg src0, w;
   unsigned nr, i;

   src0 = get_src_reg(c, &inst->FullSrcRegisters[0], 0);
   nr = src0.nr;
   w = get_src_reg(c, &inst->FullSrcRegisters[1], 3);
   interp[0] = brw_vec1_grf(nr, 0);
   interp[1] = brw_vec1_grf(nr, 4);
   interp[2] = brw_vec1_grf(nr+1, 0);
   interp[3] = brw_vec1_grf(nr+1, 4);
   brw_set_saturate(p, inst->Instruction.Saturate != TGSI_SAT_NONE);
   for(i = 0; i < 4; i++ ) {
      if (mask & (1<<i)) {
	 dst = get_dst_reg(c, inst, i);
	 brw_MOV(p, dst, suboffset(interp[i], 1));
	 brw_MUL(p, dst, dst, w);
      }
   }
   brw_set_saturate(p, 0);
}

/* TODO
   BIAS on SIMD8 not workind yet...
*/
static void emit_txb(struct brw_wm_compile *c,
		     struct tgsi_full_instruction *inst)
{
#if 0
   struct brw_compile *p = &c->func;
   struct brw_reg payload_reg = c->payload_depth[0];
   struct brw_reg dst[4], src[4];
   unsigned i;
   for (i = 0; i < 4; i++)
      dst[i] = get_dst_reg(c, inst, i);
   for (i = 0; i < 4; i++)
      src[i] = get_src_reg(c, &inst->FullSrcRegisters[0], i);

#if 0
   switch (inst->TexSrcTarget) {
   case TEXTURE_1D_INDEX:
      brw_MOV(p, brw_message_reg(2), src[0]);
      brw_MOV(p, brw_message_reg(3), brw_imm_f(0));
      brw_MOV(p, brw_message_reg(4), brw_imm_f(0));
      break;
   case TEXTURE_2D_INDEX:
   case TEXTURE_RECT_INDEX:
      brw_MOV(p, brw_message_reg(2), src[0]);
      brw_MOV(p, brw_message_reg(3), src[1]);
      brw_MOV(p, brw_message_reg(4), brw_imm_f(0));
      break;
   default:
      brw_MOV(p, brw_message_reg(2), src[0]);
      brw_MOV(p, brw_message_reg(3), src[1]);
      brw_MOV(p, brw_message_reg(4), src[2]);
      break;
   }
#else
   brw_MOV(p, brw_message_reg(2), src[0]);
   brw_MOV(p, brw_message_reg(3), src[1]);
   brw_MOV(p, brw_message_reg(4), brw_imm_f(0));
#endif

   brw_MOV(p, brw_message_reg(5), src[3]);
   brw_MOV(p, brw_message_reg(6), brw_imm_f(0));
   brw_SAMPLE(p,
	      retype(vec8(dst[0]), BRW_REGISTER_TYPE_UW),
	      1,
	      retype(payload_reg, BRW_REGISTER_TYPE_UW),
	      inst->TexSrcUnit + 1, /* surface */
	      inst->TexSrcUnit,     /* sampler */
	      inst->FullDstRegisters[0].DstRegister.WriteMask,
	      BRW_SAMPLER_MESSAGE_SIMD16_SAMPLE_BIAS,
	      4,
	      4,
	      0);
#endif
}

static void emit_tex(struct brw_wm_compile *c,
		     struct tgsi_full_instruction *inst)
{
#if 0
   struct brw_compile *p = &c->func;
   struct brw_reg payload_reg = c->payload_depth[0];
   struct brw_reg dst[4], src[4];
   unsigned msg_len;
   unsigned i, nr;
   unsigned emit;
   boolean shadow = (c->key.shadowtex_mask & (1<<inst->TexSrcUnit)) ? 1 : 0;

   for (i = 0; i < 4; i++)
      dst[i] = get_dst_reg(c, inst, i);
   for (i = 0; i < 4; i++)
      src[i] = get_src_reg(c, &inst->FullSrcRegisters[0], i);

#if 0
   switch (inst->TexSrcTarget) {
   case TEXTURE_1D_INDEX:
      emit = WRITEMASK_X;
      nr = 1;
      break;
   case TEXTURE_2D_INDEX:
   case TEXTURE_RECT_INDEX:
      emit = WRITEMASK_XY;
      nr = 2;
      break;
   default:
      emit = WRITEMASK_XYZ;
      nr = 3;
      break;
   }
#else
   emit = WRITEMASK_XY;
   nr = 2;
#endif

   msg_len = 1;

   for (i = 0; i < nr; i++) {
      static const unsigned swz[4] = {0,1,2,2};
      if (emit & (1<<i))
	 brw_MOV(p, brw_message_reg(msg_len+1), src[swz[i]]);
      else
	 brw_MOV(p, brw_message_reg(msg_len+1), brw_imm_f(0));
      msg_len += 1;
   }

   if (shadow) {
      brw_MOV(p, brw_message_reg(5), brw_imm_f(0));
      brw_MOV(p, brw_message_reg(6), src[2]);
   }

   brw_SAMPLE(p,
	      retype(vec8(dst[0]), BRW_REGISTER_TYPE_UW),
	      1,
	      retype(payload_reg, BRW_REGISTER_TYPE_UW),
	      inst->TexSrcUnit + 1, /* surface */
	      inst->TexSrcUnit,     /* sampler */
	      inst->FullDstRegisters[0].DstRegister.WriteMask,
	      BRW_SAMPLER_MESSAGE_SIMD8_SAMPLE,
	      4,
	      shadow ? 6 : 4,
	      0);

   if (shadow)
      brw_MOV(p, dst[3], brw_imm_f(1.0));
#endif
}








static void emit_fb_write(struct brw_wm_compile *c,
			  struct tgsi_full_instruction *inst)
{
   struct brw_compile *p = &c->func;
   int nr = 2;
   int channel;
   int base_reg = 0;

   // src0 = output color
   // src1 = payload_depth[0]
   // src2 = output depth
   // dst = ???



   /* Reserve a space for AA - may not be needed:
    */
   if (c->key.aa_dest_stencil_reg)
      nr += 1;

   {
      brw_push_insn_state(p);
      for (channel = 0; channel < 4; channel++) {
	 struct brw_reg src0 = c->wm_regs[TGSI_FILE_OUTPUT][0][channel];

	 /*  mov (8) m2.0<1>:ud   r28.0<8;8,1>:ud  { Align1 } */
	 /*  mov (8) m6.0<1>:ud   r29.0<8;8,1>:ud  { Align1 SecHalf } */
	 brw_MOV(p, brw_message_reg(nr + channel), src0);
      }
      /* skip over the regs populated above: */
      nr += 8;
      brw_pop_insn_state(p);
   }
    

   /* Pass through control information:
    */
   /*  mov (8) m1.0<1>:ud   r1.0<8;8,1>:ud   { Align1 NoMask } */
   {
      brw_push_insn_state(p);
      brw_set_mask_control(p, BRW_MASK_DISABLE); /* ? */
      brw_MOV(p,
	      brw_message_reg(base_reg + 1),
	      brw_vec8_grf(1, 0));
      brw_pop_insn_state(p);
   }

   /* Send framebuffer write message: */
   brw_fb_WRITE(p,
		retype(vec8(brw_null_reg()), BRW_REGISTER_TYPE_UW),
		base_reg,
		retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UW),
		0,              /* render surface always 0 */
		nr,
		0,
		1);

}


static void brw_wm_emit_instruction( struct brw_wm_compile *c,
				     struct tgsi_full_instruction *inst )
{
   struct brw_compile *p = &c->func;

#if 0   
   if (inst->CondUpdate)
      brw_set_conditionalmod(p, BRW_CONDITIONAL_NZ);
   else
      brw_set_conditionalmod(p, BRW_CONDITIONAL_NONE);
#else
   brw_set_conditionalmod(p, BRW_CONDITIONAL_NONE);
#endif

   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_ABS:
      emit_abs(c, inst);
      break;
   case TGSI_OPCODE_ADD:
      emit_alu2(c, inst, BRW_OPCODE_ADD);
      break;
   case TGSI_OPCODE_SUB:
      assert(0);
//      emit_alu2(c, inst, BRW_OPCODE_SUB);
      break;
   case TGSI_OPCODE_FRC:
      emit_alu1(c, inst, BRW_OPCODE_FRC);
      break;
   case TGSI_OPCODE_FLR:
      assert(0);
//      emit_alu1(c, inst, BRW_OPCODE_FLR);
      break;
   case TGSI_OPCODE_LRP:
      emit_lrp(c, inst);
      break;
   case TGSI_OPCODE_INT:
      emit_alu1(c, inst, BRW_OPCODE_RNDD);
      break;
   case TGSI_OPCODE_MOV:
      emit_alu1(c, inst, BRW_OPCODE_MOV);
      break;
   case TGSI_OPCODE_DP3:
      emit_dp3(c, inst);
      break;
   case TGSI_OPCODE_DP4:
      emit_dp4(c, inst);
      break;
   case TGSI_OPCODE_XPD:
      emit_xpd(c, inst);
      break;
   case TGSI_OPCODE_DPH:
      emit_dph(c, inst);
      break;
   case TGSI_OPCODE_RCP:
      emit_math1(c, inst, BRW_MATH_FUNCTION_INV);
      break;
   case TGSI_OPCODE_RSQ:
      emit_math1(c, inst, BRW_MATH_FUNCTION_RSQ);
      break;
   case TGSI_OPCODE_SIN:
      emit_math1(c, inst, BRW_MATH_FUNCTION_SIN);
      break;
   case TGSI_OPCODE_COS:
      emit_math1(c, inst, BRW_MATH_FUNCTION_COS);
      break;
   case TGSI_OPCODE_EX2:
      emit_math1(c, inst, BRW_MATH_FUNCTION_EXP);
      break;
   case TGSI_OPCODE_LG2:
      emit_math1(c, inst, BRW_MATH_FUNCTION_LOG);
      break;
   case TGSI_OPCODE_MAX:
      emit_max(c, inst);
      break;
   case TGSI_OPCODE_MIN:
      emit_min(c, inst);
      break;
   case TGSI_OPCODE_DDX:
      emit_ddx(c, inst);
      break;
   case TGSI_OPCODE_DDY:
      emit_ddy(c, inst);
      break;
   case TGSI_OPCODE_SLT:
      emit_sop(c, inst, BRW_CONDITIONAL_L);
      break;
   case TGSI_OPCODE_SLE:
      emit_sop(c, inst, BRW_CONDITIONAL_LE);
      break;
   case TGSI_OPCODE_SGT:
      emit_sop(c, inst, BRW_CONDITIONAL_G);
      break;
   case TGSI_OPCODE_SGE:
      emit_sop(c, inst, BRW_CONDITIONAL_GE);
      break;
   case TGSI_OPCODE_SEQ:
      emit_sop(c, inst, BRW_CONDITIONAL_EQ);
      break;
   case TGSI_OPCODE_SNE:
      emit_sop(c, inst, BRW_CONDITIONAL_NEQ);
      break;
   case TGSI_OPCODE_MUL:
      emit_alu2(c, inst, BRW_OPCODE_MUL);
      break;
   case TGSI_OPCODE_POW:
      emit_pow(c, inst);
      break;
   case TGSI_OPCODE_MAD:
      emit_mad(c, inst);
      break;
   case TGSI_OPCODE_TEX:
      emit_tex(c, inst);
      break;
   case TGSI_OPCODE_TXB:
      emit_txb(c, inst);
      break;
   case TGSI_OPCODE_TEXKILL:
      emit_kil(c);
      break;
   case TGSI_OPCODE_IF:
      assert(c->if_insn < MAX_IFSN);
      c->if_inst[c->if_insn++] = brw_IF(p, BRW_EXECUTE_8);
      break;
   case TGSI_OPCODE_ELSE:
      c->if_inst[c->if_insn-1]  = brw_ELSE(p, c->if_inst[c->if_insn-1]);
      break;
   case TGSI_OPCODE_ENDIF:
      assert(c->if_insn > 0);
      brw_ENDIF(p, c->if_inst[--c->if_insn]);
      break;
   case TGSI_OPCODE_BGNSUB:
   case TGSI_OPCODE_ENDSUB:
      break;
   case TGSI_OPCODE_CAL:
      brw_push_insn_state(p);
      brw_set_mask_control(p, BRW_MASK_DISABLE);
      brw_set_access_mode(p, BRW_ALIGN_1);
      brw_ADD(p, deref_1ud(c->stack_index, 0), brw_ip_reg(), brw_imm_d(3*16));
      brw_set_access_mode(p, BRW_ALIGN_16);
      brw_ADD(p, 
	      get_addr_reg(c->stack_index),
	      get_addr_reg(c->stack_index), brw_imm_d(4));
//      orig_inst = inst->Data;
//      orig_inst->Data = &p->store[p->nr_insn];
      assert(0);
      brw_ADD(p, brw_ip_reg(), brw_ip_reg(), brw_imm_d(1*16));
      brw_pop_insn_state(p);
      break;

   case TGSI_OPCODE_RET:
#if 0
      brw_push_insn_state(p);
      brw_set_mask_control(p, BRW_MASK_DISABLE);
      brw_ADD(p, 
	      get_addr_reg(c->stack_index),
	      get_addr_reg(c->stack_index), brw_imm_d(-4));
      brw_set_access_mode(p, BRW_ALIGN_1);
      brw_MOV(p, brw_ip_reg(), deref_1ud(c->stack_index, 0));
      brw_set_access_mode(p, BRW_ALIGN_16);
      brw_pop_insn_state(p);
#else
      emit_fb_write(c, inst);
#endif

      break;
   case TGSI_OPCODE_LOOP:
      c->loop_inst[c->loop_insn++] = brw_DO(p, BRW_EXECUTE_8);
      break;
   case TGSI_OPCODE_BRK:
      brw_BREAK(p);
      brw_set_predicate_control(p, BRW_PREDICATE_NONE);
      break;
   case TGSI_OPCODE_CONT:
      brw_CONT(p);
      brw_set_predicate_control(p, BRW_PREDICATE_NONE);
      break;
   case TGSI_OPCODE_ENDLOOP:
      c->loop_insn--;
      c->inst0 = c->inst1 = brw_WHILE(p, c->loop_inst[c->loop_insn]);
      /* patch all the BREAK instructions from
	 last BEGINLOOP */
      while (c->inst0 > c->loop_inst[c->loop_insn]) {
	 c->inst0--;
	 if (c->inst0->header.opcode == BRW_OPCODE_BREAK) {
	    c->inst0->bits3.if_else.jump_count = c->inst1 - c->inst0 + 1;
	    c->inst0->bits3.if_else.pop_count = 0;
	 } else if (c->inst0->header.opcode == BRW_OPCODE_CONTINUE) {
	    c->inst0->bits3.if_else.jump_count = c->inst1 - c->inst0;
	    c->inst0->bits3.if_else.pop_count = 0;
	 }
      }
      break;
   case TGSI_OPCODE_END:
      emit_fb_write(c, inst);
      break;

   default:
      debug_printf("unsupported IR in fragment shader %d\n",
		   inst->Instruction.Opcode);
   }
#if 0
   if (inst->CondUpdate)
      brw_set_predicate_control(p, BRW_PREDICATE_NORMAL);
   else
      brw_set_predicate_control(p, BRW_PREDICATE_NONE);
#endif
}






void brw_wm_glsl_emit(struct brw_wm_compile *c)
{
   struct tgsi_parse_context parse;
   struct brw_compile *p = &c->func;

   brw_init_compile(&c->func);
   brw_set_compression_control(p, BRW_COMPRESSION_NONE);

   c->reg_index = 0;
   c->if_insn = 0;
   c->loop_insn = 0;
   c->stack_index = brw_indirect(0,0);

   /* Do static register allocation and parameter interpolation:
    */
   brw_wm_emit_decls( c );

   /* Emit the actual program.  All done with very direct translation,
    * hopefully we can improve on this shortly...
    */
   brw_MOV(p, get_addr_reg(c->stack_index), brw_address(c->stack));

   tgsi_parse_init( &parse, c->fp->program.tokens );

   while( !tgsi_parse_end_of_tokens( &parse ) ) 
   {
      tgsi_parse_token( &parse );

      switch( parse.FullToken.Token.Type ) {
      case TGSI_TOKEN_TYPE_DECLARATION:
	 /* already done */
	 break;

      case TGSI_TOKEN_TYPE_IMMEDIATE:
         /* not handled yet */
	 assert(0);
         break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         brw_wm_emit_instruction(c, &parse.FullToken.FullInstruction);
         break;

      default:
         assert( 0 );
      }
   }

   tgsi_parse_free (&parse);
   
   /* Fix up call targets:
    */
#if 0
   {
      unsigned nr_insns = c->fp->program.Base.NumInstructions;
      unsigned insn, target_insn;
      struct tgsi_full_instruction *inst1, *inst2;
      struct brw_instruction *brw_inst1, *brw_inst2;
      int offset;
      for (insn = 0; insn < nr_insns; insn++) {
	 inst1 = &c->fp->program.Base.Instructions[insn];
	 brw_inst1 = inst1->Data;
	 switch (inst1->Opcode) {
	 case TGSI_OPCODE_CAL:
	    target_insn = inst1->BranchTarget;
	    inst2 = &c->fp->program.Base.Instructions[target_insn];
	    brw_inst2 = inst2->Data;
	    offset = brw_inst2 - brw_inst1;
	    brw_set_src1(brw_inst1, brw_imm_d(offset*16));
	    break;
	 default:
	    break;
	 }
      }
   }
#endif

   c->prog_data.total_grf = c->reg_index;
   c->prog_data.total_scratch = 0;
}
