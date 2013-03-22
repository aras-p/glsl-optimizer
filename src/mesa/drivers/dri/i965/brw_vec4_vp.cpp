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

/** @file brw_vec4_vp.cpp
 *
 * A translator from Mesa IR to the i965 driver's Vec4 IR, used to implement
 * ARB_vertex_program and fixed-function vertex processing.
 */

#include "brw_context.h"
#include "brw_vec4.h"
extern "C" {
#include "program/prog_parameter.h"
#include "program/prog_print.h"
}
using namespace brw;

void
vec4_visitor::emit_vp_sop(uint32_t conditional_mod,
                          dst_reg dst, src_reg src0, src_reg src1,
                          src_reg one)
{
   vec4_instruction *inst;

   inst = emit(BRW_OPCODE_CMP, dst_null_d(), src0, src1);
   inst->conditional_mod = conditional_mod;

   inst = emit(BRW_OPCODE_SEL, dst, one, src_reg(0.0f));
   inst->predicate = BRW_PREDICATE_NORMAL;
}

/**
 * Reswizzle a given source register.
 * \sa brw_swizzle().
 */
static inline src_reg
reswizzle(src_reg orig, unsigned x, unsigned y, unsigned z, unsigned w)
{
   src_reg t = orig;
   t.swizzle = BRW_SWIZZLE4(BRW_GET_SWZ(orig.swizzle, x),
                            BRW_GET_SWZ(orig.swizzle, y),
                            BRW_GET_SWZ(orig.swizzle, z),
                            BRW_GET_SWZ(orig.swizzle, w));
   return t;
}

void
vec4_visitor::emit_vertex_program_code()
{
   this->need_all_constants_in_pull_buffer = false;

   setup_vp_regs();

   /* Keep a reg with 1.0 around, for reuse by emit_vs_sop so that it can just
    * be:
    *
    * sel.f0 dst 1.0 0.0
    *
    * instead of
    *
    * mov    dst 0.0
    * mov.f0 dst 1.0
    */
   src_reg one = src_reg(this, glsl_type::float_type);
   emit(MOV(dst_reg(one), src_reg(1.0f)));

   for (unsigned int insn = 0; insn < vp->Base.NumInstructions; insn++) {
      const struct prog_instruction *vpi = &vp->Base.Instructions[insn];
      base_ir = vpi;

      dst_reg dst;
      src_reg src[3];

      /* We always emit into a temporary destination register to avoid
       * aliasing issues.
       */
      dst = dst_reg(this, glsl_type::vec4_type);

      for (int i = 0; i < 3; i++)
         src[i] = get_vp_src_reg(vpi->SrcReg[i]);

      switch (vpi->Opcode) {
      case OPCODE_ABS:
         src[0].abs = true;
         src[0].negate = false;
         emit(MOV(dst, src[0]));
         break;

      case OPCODE_ADD:
         emit(ADD(dst, src[0], src[1]));
         break;

      case OPCODE_ARL:
         if (intel->gen >= 6) {
            dst.writemask = WRITEMASK_X;
            dst_reg dst_f = dst;
            dst_f.type = BRW_REGISTER_TYPE_F;

            emit(RNDD(dst_f, src[0]));
            emit(MOV(dst, src_reg(dst_f)));
         } else {
            emit(RNDD(dst, src[0]));
         }
         break;

      case OPCODE_DP3:
         emit(DP3(dst, src[0], src[1]));
         break;
      case OPCODE_DP4:
         emit(DP4(dst, src[0], src[1]));
         break;
      case OPCODE_DPH:
         emit(DPH(dst, src[0], src[1]));
         break;

      case OPCODE_DST: {
         dst_reg t = dst;
         if (vpi->DstReg.WriteMask & WRITEMASK_X) {
            t.writemask = WRITEMASK_X;
            emit(MOV(t, src_reg(1.0f)));
         }
         if (vpi->DstReg.WriteMask & WRITEMASK_Y) {
            t.writemask = WRITEMASK_Y;
            emit(MUL(t, src[0], src[1]));
         }
         if (vpi->DstReg.WriteMask & WRITEMASK_Z) {
            t.writemask = WRITEMASK_Z;
            emit(MOV(t, src[0]));
         }
         if (vpi->DstReg.WriteMask & WRITEMASK_W) {
            t.writemask = WRITEMASK_W;
            emit(MOV(t, src[1]));
         }
         break;
      }

      case OPCODE_EXP: {
         dst_reg result = dst;
         if (vpi->DstReg.WriteMask & WRITEMASK_X) {
            /* tmp_d = floor(src[0].x) */
            src_reg tmp_d = src_reg(this, glsl_type::ivec4_type);
            assert(tmp_d.type == BRW_REGISTER_TYPE_D);
            emit(RNDD(dst_reg(tmp_d), reswizzle(src[0], 0, 0, 0, 0)));

            /* result[0] = 2.0 ^ tmp */
            /* Adjust exponent for floating point: exp += 127 */
            dst_reg tmp_d_x(GRF, tmp_d.reg, glsl_type::int_type, WRITEMASK_X);
            emit(ADD(tmp_d_x, tmp_d, src_reg(127)));

            /* Install exponent and sign.  Excess drops off the edge: */
            dst_reg res_d_x(GRF, result.reg, glsl_type::int_type, WRITEMASK_X);
            emit(BRW_OPCODE_SHL, res_d_x, tmp_d, src_reg(23));
         }
         if (vpi->DstReg.WriteMask & WRITEMASK_Y) {
            result.writemask = WRITEMASK_Y;
            emit(FRC(result, src[0]));
         }
         if (vpi->DstReg.WriteMask & WRITEMASK_Z) {
            result.writemask = WRITEMASK_Z;
            emit_math(SHADER_OPCODE_EXP2, result, src[0]);
         }
         if (vpi->DstReg.WriteMask & WRITEMASK_W) {
            result.writemask = WRITEMASK_W;
            emit(MOV(result, src_reg(1.0f)));
         }
         break;
      }

      case OPCODE_EX2:
         emit_math(SHADER_OPCODE_EXP2, dst, src[0]);
         break;

      case OPCODE_FLR:
         emit(RNDD(dst, src[0]));
         break;

      case OPCODE_FRC:
         emit(FRC(dst, src[0]));
         break;

      case OPCODE_LG2:
         emit_math(SHADER_OPCODE_LOG2, dst, src[0]);
         break;

      case OPCODE_LIT: {
         dst_reg result = dst;
         /* From the ARB_vertex_program spec:
          *
          *      tmp = VectorLoad(op0);
          *      if (tmp.x < 0) tmp.x = 0;
          *      if (tmp.y < 0) tmp.y = 0;
          *      if (tmp.w < -(128.0-epsilon)) tmp.w = -(128.0-epsilon);
          *      else if (tmp.w > 128-epsilon) tmp.w = 128-epsilon;
          *      result.x = 1.0;
          *      result.y = tmp.x;
          *      result.z = (tmp.x > 0) ? RoughApproxPower(tmp.y, tmp.w) : 0.0;
          *      result.w = 1.0;
          *
          * Note that we don't do the clamping to +/- 128.  We didn't in
          * brw_vs_emit.c either.
          */
         if (vpi->DstReg.WriteMask & WRITEMASK_XW) {
            result.writemask = WRITEMASK_XW;
            emit(MOV(result, src_reg(1.0f)));
         }
         if (vpi->DstReg.WriteMask & WRITEMASK_YZ) {
            result.writemask = WRITEMASK_YZ;
            emit(MOV(result, src_reg(0.0f)));

            src_reg tmp_x = reswizzle(src[0], 0, 0, 0, 0);

            emit(CMP(dst_null_d(), tmp_x, src_reg(0.0f), BRW_CONDITIONAL_G));
            emit(IF(BRW_PREDICATE_NORMAL));

            if (vpi->DstReg.WriteMask & WRITEMASK_Y) {
               result.writemask = WRITEMASK_Y;
               emit(MOV(result, tmp_x));
            }

            if (vpi->DstReg.WriteMask & WRITEMASK_Z) {
               /* if (tmp.y < 0) tmp.y = 0; */
               src_reg tmp_y = reswizzle(src[0], 1, 1, 1, 1);
               result.writemask = WRITEMASK_Z;
               emit_minmax(BRW_CONDITIONAL_G, result, tmp_y, src_reg(0.0f));

               src_reg clamped_y(result);
               clamped_y.swizzle = BRW_SWIZZLE_ZZZZ;

               src_reg tmp_w = reswizzle(src[0], 3, 3, 3, 3);

               emit_math(SHADER_OPCODE_POW, result, clamped_y, tmp_w);
            }
            emit(BRW_OPCODE_ENDIF);
         }
         break;
      }

      case OPCODE_LOG: {
         dst_reg result = dst;
         result.type = BRW_REGISTER_TYPE_UD;
         src_reg result_src = src_reg(result);

         src_reg arg0_ud = reswizzle(src[0], 0, 0, 0, 0);
         arg0_ud.type = BRW_REGISTER_TYPE_UD;

         /* Perform mant = frexpf(fabsf(x), &exp), adjust exp and mnt
          * according to spec:
          *
          * These almost look likey they could be joined up, but not really
          * practical:
          *
          * result[0].f = (x.i & ((1<<31)-1) >> 23) - 127
          * result[1].i = (x.i & ((1<<23)-1)        + (127<<23)
          */
         if (vpi->DstReg.WriteMask & WRITEMASK_XZ) {
            result.writemask = WRITEMASK_X;
            emit(AND(result, arg0_ud, src_reg((1u << 31) - 1)));
            emit(BRW_OPCODE_SHR, result, result_src, src_reg(23u));
            src_reg result_d(result_src);
            result_d.type = BRW_REGISTER_TYPE_D; /* does it matter? */
            result.type = BRW_REGISTER_TYPE_F;
            emit(ADD(result, result_d, src_reg(-127)));
         }

         if (vpi->DstReg.WriteMask & WRITEMASK_YZ) {
            result.writemask = WRITEMASK_Y;
            result.type = BRW_REGISTER_TYPE_UD;
            emit(AND(result, arg0_ud, src_reg((1u << 23) - 1)));
            emit(OR(result, result_src, src_reg(127u << 23)));
         }

         if (vpi->DstReg.WriteMask & WRITEMASK_Z) {
            /* result[2] = result[0] + LOG2(result[1]); */

            /* Why bother?  The above is just a hint how to do this with a
             * taylor series.  Maybe we *should* use a taylor series as by
             * the time all the above has been done it's almost certainly
             * quicker than calling the mathbox, even with low precision.
             *
             * Options are:
             *    - result[0] + mathbox.LOG2(result[1])
             *    - mathbox.LOG2(arg0.x)
             *    - result[0] + inline_taylor_approx(result[1])
             */
            result.type = BRW_REGISTER_TYPE_F;
            result.writemask = WRITEMASK_Z;
            src_reg result_x(result), result_y(result), result_z(result);
            result_x.swizzle = BRW_SWIZZLE_XXXX;
            result_y.swizzle = BRW_SWIZZLE_YYYY;
            result_z.swizzle = BRW_SWIZZLE_ZZZZ;
            emit_math(SHADER_OPCODE_LOG2, result, result_y);
            emit(ADD(result, result_z, result_x));
         }

         if (vpi->DstReg.WriteMask & WRITEMASK_W) {
            result.type = BRW_REGISTER_TYPE_F;
            result.writemask = WRITEMASK_W;
            emit(MOV(result, src_reg(1.0f)));
         }
         break;
      }

      case OPCODE_MAD: {
         src_reg temp = src_reg(this, glsl_type::vec4_type);
         emit(MUL(dst_reg(temp), src[0], src[1]));
         emit(ADD(dst, temp, src[2]));
         break;
      }

      case OPCODE_MAX:
         emit_minmax(BRW_CONDITIONAL_G, dst, src[0], src[1]);
         break;

      case OPCODE_MIN:
         emit_minmax(BRW_CONDITIONAL_L, dst, src[0], src[1]);
         break;

      case OPCODE_MOV:
         emit(MOV(dst, src[0]));
         break;

      case OPCODE_MUL:
         emit(MUL(dst, src[0], src[1]));
         break;

      case OPCODE_POW:
         emit_math(SHADER_OPCODE_POW, dst, src[0], src[1]);
         break;

      case OPCODE_RCP:
         emit_math(SHADER_OPCODE_RCP, dst, src[0]);
         break;

      case OPCODE_RSQ:
         emit_math(SHADER_OPCODE_RSQ, dst, src[0]);
         break;

      case OPCODE_SGE:
         emit_vp_sop(BRW_CONDITIONAL_GE, dst, src[0], src[1], one);
         break;

      case OPCODE_SLT:
         emit_vp_sop(BRW_CONDITIONAL_L, dst, src[0], src[1], one);
         break;

      case OPCODE_SUB: {
         src_reg neg_src1 = src[1];
         neg_src1.negate = !src[1].negate;
         emit(ADD(dst, src[0], neg_src1));
         break;
      }

      case OPCODE_SWZ:
         /* Note that SWZ's extended swizzles are handled in the general
          * get_src_reg() code.
          */
         emit(MOV(dst, src[0]));
         break;

      case OPCODE_XPD: {
         src_reg t1 = src_reg(this, glsl_type::vec4_type);
         src_reg t2 = src_reg(this, glsl_type::vec4_type);

         emit(MUL(dst_reg(t1),
                  reswizzle(src[0], 1, 2, 0, 3),
                  reswizzle(src[1], 2, 0, 1, 3)));
         emit(MUL(dst_reg(t2),
                  reswizzle(src[0], 2, 0, 1, 3),
                  reswizzle(src[1], 1, 2, 0, 3)));
         t2.negate = true;
         emit(ADD(dst, t1, t2));
         break;
      }

      case OPCODE_END:
         break;

      default:
         _mesa_problem(ctx, "Unsupported opcode %s in vertex program\n",
                       _mesa_opcode_string(vpi->Opcode));
      }

      /* Copy the temporary back into the actual destination register. */
      if (vpi->Opcode != OPCODE_END) {
         emit(MOV(get_vp_dst_reg(vpi->DstReg), src_reg(dst)));
      }
   }

   /* If we used relative addressing, we need to upload all constants as
    * pull constants.  Do that now.
    */
   if (this->need_all_constants_in_pull_buffer) {
      const struct gl_program_parameter_list *params = c->vp->program.Base.Parameters;
      unsigned i;
      for (i = 0; i < params->NumParameters * 4; i++) {
         c->prog_data.pull_param[i] = &params->ParameterValues[i / 4][i % 4].f;
      }
      c->prog_data.nr_pull_params = i;
   }
}

void
vec4_visitor::setup_vp_regs()
{
   /* PROGRAM_TEMPORARY */
   int num_temp = vp->Base.NumTemporaries;
   vp_temp_regs = rzalloc_array(mem_ctx, src_reg, num_temp);
   for (int i = 0; i < num_temp; i++)
      vp_temp_regs[i] = src_reg(this, glsl_type::vec4_type);

   /* PROGRAM_STATE_VAR etc. */
   struct gl_program_parameter_list *plist = c->vp->program.Base.Parameters;
   for (unsigned p = 0; p < plist->NumParameters; p++) {
      unsigned components = plist->Parameters[p].Size;

      /* Parameters should be either vec4 uniforms or single component
       * constants; matrices and other larger types should have been broken
       * down earlier.
       */
      assert(components <= 4);

      this->uniform_size[this->uniforms] = 1; /* 1 vec4 */
      this->uniform_vector_size[this->uniforms] = components;
      for (unsigned i = 0; i < 4; i++) {
         c->prog_data.param[this->uniforms * 4 + i] = i >= components ? 0 :
            &plist->ParameterValues[p][i].f;
      }
      this->uniforms++; /* counted in vec4 units */
   }

   /* PROGRAM_OUTPUT */
   for (int slot = 0; slot < c->prog_data.vue_map.num_slots; slot++) {
      int varying = c->prog_data.vue_map.slot_to_varying[slot];
      if (varying == VARYING_SLOT_PSIZ)
         output_reg[varying] = dst_reg(this, glsl_type::float_type);
      else
         output_reg[varying] = dst_reg(this, glsl_type::vec4_type);
      assert(output_reg[varying].type == BRW_REGISTER_TYPE_F);
   }

   /* PROGRAM_ADDRESS */
   this->vp_addr_reg = src_reg(this, glsl_type::int_type);
   assert(this->vp_addr_reg.type == BRW_REGISTER_TYPE_D);
}

dst_reg
vec4_visitor::get_vp_dst_reg(const prog_dst_register &dst)
{
   dst_reg result;

   assert(!dst.RelAddr);

   switch (dst.File) {
   case PROGRAM_TEMPORARY:
      result = dst_reg(vp_temp_regs[dst.Index]);
      break;

   case PROGRAM_OUTPUT:
      result = output_reg[dst.Index];
      break;

   case PROGRAM_ADDRESS: {
      assert(dst.Index == 0);
      result = dst_reg(this->vp_addr_reg);
      break;
   }

   case PROGRAM_UNDEFINED:
      return dst_null_f();

   default:
      assert("vec4_vp: bad destination register file");
      return dst_reg(this, glsl_type::vec4_type);
   }

   result.writemask = dst.WriteMask;
   return result;
}

src_reg
vec4_visitor::get_vp_src_reg(const prog_src_register &src)
{
   struct gl_program_parameter_list *plist = c->vp->program.Base.Parameters;

   src_reg result;

   assert(!src.Abs);

   switch (src.File) {
   case PROGRAM_UNDEFINED:
      return src_reg(brw_null_reg());

   case PROGRAM_TEMPORARY:
      result = vp_temp_regs[src.Index];
      break;

   case PROGRAM_INPUT:
      result = src_reg(ATTR, src.Index, glsl_type::vec4_type);
      result.type = BRW_REGISTER_TYPE_F;
      break;

   case PROGRAM_ADDRESS: {
      assert(src.Index == 0);
      result = this->vp_addr_reg;
      break;
   }

   case PROGRAM_STATE_VAR:
   case PROGRAM_CONSTANT:
      /* From the ARB_vertex_program specification:
       * "Relative addressing can only be used for accessing program
       *  parameter arrays."
       */
      if (src.RelAddr) {
         /* Since we have no idea what the base of the array is, we need to
          * upload ALL constants as push constants.
          */
         this->need_all_constants_in_pull_buffer = true;

         /* Add the small constant index to the address register */
         src_reg reladdr = src_reg(this, glsl_type::int_type);
         dst_reg dst_reladdr = dst_reg(reladdr);
         dst_reladdr.writemask = WRITEMASK_X;
         emit(ADD(dst_reladdr, this->vp_addr_reg, src_reg(src.Index)));

         if (intel->gen < 6)
            emit(MUL(dst_reladdr, reladdr, src_reg(16)));

      #if 0
         assert(src.Index < this->uniforms);
         result = src_reg(dst_reg(UNIFORM, 0));
         result.type = BRW_REGISTER_TYPE_F;
         result.reladdr = new(mem_ctx) src_reg();
         memcpy(result.reladdr, &reladdr, sizeof(src_reg));
      #endif

         result = src_reg(this, glsl_type::vec4_type);
         src_reg surf_index = src_reg(unsigned(SURF_INDEX_VERT_CONST_BUFFER));
         vec4_instruction *load =
            new(mem_ctx) vec4_instruction(this, VS_OPCODE_PULL_CONSTANT_LOAD,
                                          dst_reg(result), surf_index, reladdr);
         load->base_mrf = 14;
         load->mlen = 1;
         emit(load);
         break;
      }

      /* We actually want to look at the type in the Parameters list for this,
       * because this lets us upload constant builtin uniforms as actual
       * constants.
       */
      switch (plist->Parameters[src.Index].Type) {
      case PROGRAM_CONSTANT:
         result = src_reg(this, glsl_type::vec4_type);
         for (int i = 0; i < 4; i++) {
            dst_reg t = dst_reg(result);
            t.writemask = 1 << i;
            emit(MOV(t, src_reg(plist->ParameterValues[src.Index][i].f)));
         }
         break;

      case PROGRAM_STATE_VAR:
         assert(src.Index < this->uniforms);
         result = src_reg(dst_reg(UNIFORM, src.Index));
         result.type = BRW_REGISTER_TYPE_F;
         break;

      default:
         _mesa_problem(ctx, "bad uniform src register file: %s\n",
                       _mesa_register_file_name((gl_register_file)src.File));
         return src_reg(this, glsl_type::vec4_type);
      }
      break;

   default:
      _mesa_problem(ctx, "bad src register file: %s\n",
                    _mesa_register_file_name((gl_register_file)src.File));
      return src_reg(this, glsl_type::vec4_type);
   }

   if (src.Swizzle != SWIZZLE_NOOP || src.Negate) {
      unsigned short zeros_mask = 0;
      unsigned short ones_mask = 0;
      unsigned short src_mask = 0;
      unsigned short src_swiz[4];

      for (int i = 0; i < 4; i++) {
         src_swiz[i] = 0; /* initialize for safety */

         /* The ZERO, ONE, and Negate options are only used for OPCODE_SWZ,
          * but it's simplest to handle it here.
          */
         int s = GET_SWZ(src.Swizzle, i);
         switch (s) {
         case SWIZZLE_X:
         case SWIZZLE_Y:
         case SWIZZLE_Z:
         case SWIZZLE_W:
            src_mask |= 1 << i;
            src_swiz[i] = s;
            break;
         case SWIZZLE_ZERO:
            zeros_mask |= 1 << i;
            break;
         case SWIZZLE_ONE:
            ones_mask |= 1 << i;
            break;
         }
      }

      result.swizzle =
         BRW_SWIZZLE4(src_swiz[0], src_swiz[1], src_swiz[2], src_swiz[3]);

      /* The hardware doesn't natively handle the SWZ instruction's zero/one
       * swizzles or per-component negation, so we need to use a temporary.
       */
      if (zeros_mask || ones_mask || src.Negate) {
         src_reg temp_src(this, glsl_type::vec4_type);
         dst_reg temp(temp_src);

         if (src_mask) {
            temp.writemask = src_mask;
            emit(MOV(temp, result));
         }

         if (zeros_mask) {
            temp.writemask = zeros_mask;
            emit(MOV(temp, src_reg(0.0f)));
         }

         if (ones_mask) {
            temp.writemask = ones_mask;
            emit(MOV(temp, src_reg(1.0f)));
         }

         if (src.Negate) {
            temp.writemask = src.Negate;
            src_reg neg(temp_src);
            neg.negate = true;
            emit(MOV(temp, neg));
         }
         result = temp_src;
      }
   }

   return result;
}
