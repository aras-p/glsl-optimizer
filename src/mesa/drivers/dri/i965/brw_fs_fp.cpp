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

/** @file brw_fs_fp.cpp
 *
 * Implementation of the compiler for GL_ARB_fragment_program shaders on top
 * of the GLSL compiler backend.
 */

#include "brw_context.h"
#include "brw_fs.h"

void
fs_visitor::emit_fp_alu1(enum opcode opcode,
                         const struct prog_instruction *fpi,
                         fs_reg dst, fs_reg src)
{
   for (int i = 0; i < 4; i++) {
      if (fpi->DstReg.WriteMask & (1 << i))
         emit(opcode, offset(dst, i), offset(src, i));
   }
}

void
fs_visitor::emit_fp_alu2(enum opcode opcode,
                         const struct prog_instruction *fpi,
                         fs_reg dst, fs_reg src0, fs_reg src1)
{
   for (int i = 0; i < 4; i++) {
      if (fpi->DstReg.WriteMask & (1 << i))
         emit(opcode, offset(dst, i),
              offset(src0, i), offset(src1, i));
   }
}

void
fs_visitor::emit_fp_minmax(const prog_instruction *fpi,
                           fs_reg dst, fs_reg src0, fs_reg src1)
{
   uint32_t conditionalmod;
   if (fpi->Opcode == OPCODE_MIN)
      conditionalmod = BRW_CONDITIONAL_L;
   else
      conditionalmod = BRW_CONDITIONAL_GE;

   for (int i = 0; i < 4; i++) {
      if (fpi->DstReg.WriteMask & (1 << i)) {
         emit_minmax(conditionalmod, offset(dst, i),
                     offset(src0, i), offset(src1, i));
      }
   }
}

void
fs_visitor::emit_fp_sop(uint32_t conditional_mod,
                        const struct prog_instruction *fpi,
                        fs_reg dst, fs_reg src0, fs_reg src1,
                        fs_reg one)
{
   for (int i = 0; i < 4; i++) {
      if (fpi->DstReg.WriteMask & (1 << i)) {
         fs_inst *inst;

         emit(CMP(reg_null_d, offset(src0, i), offset(src1, i),
                  conditional_mod));

         inst = emit(BRW_OPCODE_SEL, offset(dst, i), one, fs_reg(0.0f));
         inst->predicate = BRW_PREDICATE_NORMAL;
      }
   }
}

void
fs_visitor::emit_fp_scalar_write(const struct prog_instruction *fpi,
                                 fs_reg dst, fs_reg src)
{
   for (int i = 0; i < 4; i++) {
      if (fpi->DstReg.WriteMask & (1 << i))
         emit(MOV(offset(dst, i), src));
   }
}

void
fs_visitor::emit_fp_scalar_math(enum opcode opcode,
                                const struct prog_instruction *fpi,
                                fs_reg dst, fs_reg src)
{
   fs_reg temp = fs_reg(this, glsl_type::float_type);
   emit_math(opcode, temp, src);
   emit_fp_scalar_write(fpi, dst, temp);
}

void
fs_visitor::emit_fragment_program_code()
{
   setup_fp_regs();

   fs_reg null = fs_reg(brw_null_reg());

   /* Keep a reg with 1.0 around, for reuse by emit_fp_sop so that it can just
    * be:
    *
    * sel.f0 dst 1.0 0.0
    *
    * instead of
    *
    * mov    dst 0.0
    * mov.f0 dst 1.0
    */
   fs_reg one = fs_reg(this, glsl_type::float_type);
   emit(MOV(one, fs_reg(1.0f)));

   for (unsigned int insn = 0; insn < prog->NumInstructions; insn++) {
      const struct prog_instruction *fpi = &prog->Instructions[insn];
      base_ir = fpi;

      //_mesa_print_instruction(fpi);

      fs_reg dst;
      fs_reg src[3];

      /* We always emit into a temporary destination register to avoid
       * aliasing issues.
       */
      dst = fs_reg(this, glsl_type::vec4_type);

      for (int i = 0; i < 3; i++)
         src[i] = get_fp_src_reg(&fpi->SrcReg[i]);

      switch (fpi->Opcode) {
      case OPCODE_ABS:
         src[0].abs = true;
         src[0].negate = false;
         emit_fp_alu1(BRW_OPCODE_MOV, fpi, dst, src[0]);
         break;

      case OPCODE_ADD:
         emit_fp_alu2(BRW_OPCODE_ADD, fpi, dst, src[0], src[1]);
         break;

      case OPCODE_CMP:
         for (int i = 0; i < 4; i++) {
            if (fpi->DstReg.WriteMask & (1 << i)) {
               fs_inst *inst;

               emit(CMP(null, offset(src[0], i), fs_reg(0.0f),
                        BRW_CONDITIONAL_L));

               inst = emit(BRW_OPCODE_SEL, offset(dst, i),
                           offset(src[1], i), offset(src[2], i));
               inst->predicate = BRW_PREDICATE_NORMAL;
            }
         }
         break;

      case OPCODE_COS:
         emit_fp_scalar_math(SHADER_OPCODE_COS, fpi, dst, src[0]);
         break;

      case OPCODE_DP2:
      case OPCODE_DP3:
      case OPCODE_DP4:
      case OPCODE_DPH: {
         fs_reg mul = fs_reg(this, glsl_type::float_type);
         fs_reg acc = fs_reg(this, glsl_type::float_type);
         int count;

         switch (fpi->Opcode) {
         case OPCODE_DP2: count = 2; break;
         case OPCODE_DP3: count = 3; break;
         case OPCODE_DP4: count = 4; break;
         case OPCODE_DPH: count = 3; break;
         default: assert(!"not reached"); count = 0; break;
         }

         emit(MUL(acc, offset(src[0], 0), offset(src[1], 0)));
         for (int i = 1; i < count; i++) {
            emit(MUL(mul, offset(src[0], i), offset(src[1], i)));
            emit(ADD(acc, acc, mul));
         }

         if (fpi->Opcode == OPCODE_DPH)
            emit(ADD(acc, acc, offset(src[1], 3)));

         emit_fp_scalar_write(fpi, dst, acc);
         break;
      }

      case OPCODE_DST:
         if (fpi->DstReg.WriteMask & WRITEMASK_X)
            emit(MOV(dst, fs_reg(1.0f)));
         if (fpi->DstReg.WriteMask & WRITEMASK_Y) {
            emit(MUL(offset(dst, 1),
                     offset(src[0], 1), offset(src[1], 1)));
         }
         if (fpi->DstReg.WriteMask & WRITEMASK_Z)
            emit(MOV(offset(dst, 2), offset(src[0], 2)));
         if (fpi->DstReg.WriteMask & WRITEMASK_W)
            emit(MOV(offset(dst, 3), offset(src[1], 3)));
         break;

      case OPCODE_EX2:
         emit_fp_scalar_math(SHADER_OPCODE_EXP2, fpi, dst, src[0]);
         break;

      case OPCODE_FLR:
         emit_fp_alu1(BRW_OPCODE_RNDD, fpi, dst, src[0]);
         break;

      case OPCODE_FRC:
         emit_fp_alu1(BRW_OPCODE_FRC, fpi, dst, src[0]);
         break;

      case OPCODE_KIL: {
         for (int i = 0; i < 4; i++) {
            /* In most cases the argument to a KIL will be something like
             * TEMP[0].wwww, so there's no point in checking whether .w is < 0
             * 4 times in a row.
             */
            if (i > 0 &&
                GET_SWZ(fpi->SrcReg[0].Swizzle, i) ==
                GET_SWZ(fpi->SrcReg[0].Swizzle, i - 1) &&
                ((fpi->SrcReg[0].Negate >> i) & 1) ==
                ((fpi->SrcReg[0].Negate >> (i - 1)) & 1)) {
               continue;
            }


            /* Emit an instruction that's predicated on the current
             * undiscarded pixels, and updates just those pixels to be
             * turned off.
             */
            fs_inst *cmp = emit(CMP(null, offset(src[0], i), fs_reg(0.0f),
                                    BRW_CONDITIONAL_GE));
            cmp->predicate = BRW_PREDICATE_NORMAL;
            cmp->flag_subreg = 1;
         }
         break;
      }

      case OPCODE_LG2:
         emit_fp_scalar_math(SHADER_OPCODE_LOG2, fpi, dst, src[0]);
         break;

      case OPCODE_LIT:
         /* From the ARB_fragment_program spec:
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
          * brw_wm_emit.c either.
          */
         if (fpi->DstReg.WriteMask & WRITEMASK_X)
            emit(MOV(offset(dst, 0), fs_reg(1.0f)));

         if (fpi->DstReg.WriteMask & WRITEMASK_YZ) {
            fs_inst *inst;
            emit(CMP(null, offset(src[0], 0), fs_reg(0.0f),
                     BRW_CONDITIONAL_LE));

            if (fpi->DstReg.WriteMask & WRITEMASK_Y) {
               emit(MOV(offset(dst, 1), offset(src[0], 0)));
               inst = emit(MOV(offset(dst, 1), fs_reg(0.0f)));
               inst->predicate = BRW_PREDICATE_NORMAL;
            }

            if (fpi->DstReg.WriteMask & WRITEMASK_Z) {
               emit_math(SHADER_OPCODE_POW, offset(dst, 2),
                         offset(src[0], 1), offset(src[0], 3));

               inst = emit(MOV(offset(dst, 2), fs_reg(0.0f)));
               inst->predicate = BRW_PREDICATE_NORMAL;
            }
         }

         if (fpi->DstReg.WriteMask & WRITEMASK_W)
            emit(MOV(offset(dst, 3), fs_reg(1.0f)));

         break;

      case OPCODE_LRP:
         for (int i = 0; i < 4; i++) {
            if (fpi->DstReg.WriteMask & (1 << i)) {
               fs_reg a = offset(src[0], i);
               fs_reg y = offset(src[1], i);
               fs_reg x = offset(src[2], i);
               emit_lrp(offset(dst, i), x, y, a);
            }
         }
         break;

      case OPCODE_MAD:
         for (int i = 0; i < 4; i++) {
            if (fpi->DstReg.WriteMask & (1 << i)) {
               fs_reg temp = fs_reg(this, glsl_type::float_type);
               emit(MUL(temp, offset(src[0], i), offset(src[1], i)));
               emit(ADD(offset(dst, i), temp, offset(src[2], i)));
            }
         }
         break;

      case OPCODE_MAX:
         emit_fp_minmax(fpi, dst, src[0], src[1]);
         break;

      case OPCODE_MOV:
         emit_fp_alu1(BRW_OPCODE_MOV, fpi, dst, src[0]);
         break;

      case OPCODE_MIN:
         emit_fp_minmax(fpi, dst, src[0], src[1]);
         break;

      case OPCODE_MUL:
         emit_fp_alu2(BRW_OPCODE_MUL, fpi, dst, src[0], src[1]);
         break;

      case OPCODE_POW: {
         fs_reg temp = fs_reg(this, glsl_type::float_type);
         emit_math(SHADER_OPCODE_POW, temp, src[0], src[1]);
         emit_fp_scalar_write(fpi, dst, temp);
         break;
      }

      case OPCODE_RCP:
         emit_fp_scalar_math(SHADER_OPCODE_RCP, fpi, dst, src[0]);
         break;

      case OPCODE_RSQ:
         emit_fp_scalar_math(SHADER_OPCODE_RSQ, fpi, dst, src[0]);
         break;

      case OPCODE_SCS:
         if (fpi->DstReg.WriteMask & WRITEMASK_X) {
            emit_math(SHADER_OPCODE_COS, offset(dst, 0),
                      offset(src[0], 0));
         }

         if (fpi->DstReg.WriteMask & WRITEMASK_Y) {
            emit_math(SHADER_OPCODE_SIN, offset(dst, 1),
                      offset(src[0], 1));
         }
         break;

      case OPCODE_SGE:
         emit_fp_sop(BRW_CONDITIONAL_GE, fpi, dst, src[0], src[1], one);
         break;

      case OPCODE_SIN:
         emit_fp_scalar_math(SHADER_OPCODE_SIN, fpi, dst, src[0]);
         break;

      case OPCODE_SLT:
         emit_fp_sop(BRW_CONDITIONAL_L, fpi, dst, src[0], src[1], one);
         break;

      case OPCODE_SUB: {
         fs_reg neg_src1 = src[1];
         neg_src1.negate = !src[1].negate;

         emit_fp_alu2(BRW_OPCODE_ADD, fpi, dst, src[0], neg_src1);
         break;
      }

      case OPCODE_TEX:
      case OPCODE_TXB:
      case OPCODE_TXP: {
         /* We piggy-back on the GLSL IR support for texture setup.  To do so,
          * we have to cook up an ir_texture that has the coordinate field
          * with appropriate type, and shadow_comparitor set or not.  All the
          * other properties of ir_texture are passed in as arguments to the
          * emit_texture_gen* function.
          */
         ir_texture *ir = NULL;

         fs_reg lod;
         fs_reg dpdy;
         fs_reg coordinate = src[0];
         fs_reg shadow_c;
         fs_reg sample_index;

         switch (fpi->Opcode) {
         case OPCODE_TEX:
            ir = new(mem_ctx) ir_texture(ir_tex);
            break;
         case OPCODE_TXP: {
            ir = new(mem_ctx) ir_texture(ir_tex);

            coordinate = fs_reg(this, glsl_type::vec3_type);
            fs_reg invproj = fs_reg(this, glsl_type::float_type);
            emit_math(SHADER_OPCODE_RCP, invproj, offset(src[0], 3));
            for (int i = 0; i < 3; i++) {
               emit(MUL(offset(coordinate, i),
                        offset(src[0], i), invproj));
            }
            break;
         }
         case OPCODE_TXB:
            ir = new(mem_ctx) ir_texture(ir_txb);
            lod = offset(src[0], 3);
            break;
         default:
            assert(!"not reached");
            break;
         }

         ir->type = glsl_type::vec4_type;

         const glsl_type *coordinate_type;
         switch (fpi->TexSrcTarget) {
         case TEXTURE_1D_INDEX:
            coordinate_type = glsl_type::float_type;
            break;

         case TEXTURE_2D_INDEX:
         case TEXTURE_1D_ARRAY_INDEX:
         case TEXTURE_RECT_INDEX:
         case TEXTURE_EXTERNAL_INDEX:
            coordinate_type = glsl_type::vec2_type;
            break;

         case TEXTURE_3D_INDEX:
         case TEXTURE_2D_ARRAY_INDEX:
            coordinate_type = glsl_type::vec3_type;
            break;

         case TEXTURE_CUBE_INDEX: {
            coordinate_type = glsl_type::vec3_type;

            fs_reg temp = fs_reg(this, glsl_type::float_type);
            fs_reg cubecoord = fs_reg(this, glsl_type::vec3_type);
            fs_reg abscoord = coordinate;
            abscoord.negate = false;
            abscoord.abs = true;
            emit_minmax(BRW_CONDITIONAL_GE, temp,
                        offset(abscoord, 0), offset(abscoord, 1));
            emit_minmax(BRW_CONDITIONAL_GE, temp,
                        temp, offset(abscoord, 2));
            emit_math(SHADER_OPCODE_RCP, temp, temp);
            for (int i = 0; i < 3; i++) {
               emit(MUL(offset(cubecoord, i),
                        offset(coordinate, i), temp));
            }

            coordinate = cubecoord;
            break;
         }

         default:
            assert(!"not reached");
            coordinate_type = glsl_type::vec2_type;
            break;
         }

         ir_constant_data junk_data;
         ir->coordinate = new(mem_ctx) ir_constant(coordinate_type, &junk_data);

         if (fpi->TexShadow) {
            shadow_c = offset(coordinate, 2);
            ir->shadow_comparitor = new(mem_ctx) ir_constant(0.0f);
         }

         coordinate = rescale_texcoord(ir, coordinate,
                                       fpi->TexSrcTarget == TEXTURE_RECT_INDEX,
                                       fpi->TexSrcUnit, fpi->TexSrcUnit);

         fs_inst *inst;
         if (brw->gen >= 7) {
            inst = emit_texture_gen7(ir, dst, coordinate, shadow_c, lod, dpdy, sample_index, fs_reg(0u), fpi->TexSrcUnit);
         } else if (brw->gen >= 5) {
            inst = emit_texture_gen5(ir, dst, coordinate, shadow_c, lod, dpdy, sample_index);
         } else {
            inst = emit_texture_gen4(ir, dst, coordinate, shadow_c, lod, dpdy);
         }

         inst->sampler = fpi->TexSrcUnit;
         inst->shadow_compare = fpi->TexShadow;

         /* Reuse the GLSL swizzle_result() handler. */
         swizzle_result(ir, dst, fpi->TexSrcUnit);
         dst = this->result;

         break;
      }

      case OPCODE_SWZ:
         /* Note that SWZ's extended swizzles are handled in the general
          * get_src_reg() code.
          */
         emit_fp_alu1(BRW_OPCODE_MOV, fpi, dst, src[0]);
         break;

      case OPCODE_XPD:
         for (int i = 0; i < 3; i++) {
            if (fpi->DstReg.WriteMask & (1 << i)) {
               int i1 = (i + 1) % 3;
               int i2 = (i + 2) % 3;

               fs_reg temp = fs_reg(this, glsl_type::float_type);
               fs_reg neg_src1_1 = offset(src[1], i1);
               neg_src1_1.negate = !neg_src1_1.negate;
               emit(MUL(temp, offset(src[0], i2), neg_src1_1));
               emit(MUL(offset(dst, i),
                        offset(src[0], i1), offset(src[1], i2)));
               emit(ADD(offset(dst, i), offset(dst, i), temp));
            }
         }
         break;

      case OPCODE_END:
         break;

      default:
         _mesa_problem(ctx, "Unsupported opcode %s in fragment program\n",
                       _mesa_opcode_string(fpi->Opcode));
      }

      /* To handle saturates, we emit a MOV with a saturate bit, which
       * optimization should fold into the preceding instructions when safe.
       */
      if (fpi->Opcode != OPCODE_END) {
         fs_reg real_dst = get_fp_dst_reg(&fpi->DstReg);

         for (int i = 0; i < 4; i++) {
            if (fpi->DstReg.WriteMask & (1 << i)) {
               fs_inst *inst = emit(MOV(offset(real_dst, i),
                                        offset(dst, i)));
               inst->saturate = fpi->SaturateMode;
            }
         }
      }
   }

   /* Epilogue:
    *
    * Fragment depth has this strange convention of being the .z component of
    * a vec4.  emit_fb_write() wants to see a float value, instead.
    */
   this->current_annotation = "result.depth write";
   if (frag_depth.file != BAD_FILE) {
      fs_reg temp = fs_reg(this, glsl_type::float_type);
      emit(MOV(temp, offset(frag_depth, 2)));
      frag_depth = temp;
   }
}

void
fs_visitor::setup_fp_regs()
{
   /* PROGRAM_TEMPORARY */
   int num_temp = prog->NumTemporaries;
   fp_temp_regs = rzalloc_array(mem_ctx, fs_reg, num_temp);
   for (int i = 0; i < num_temp; i++)
      fp_temp_regs[i] = fs_reg(this, glsl_type::vec4_type);

   /* PROGRAM_STATE_VAR etc. */
   if (dispatch_width == 8) {
      for (unsigned p = 0;
           p < prog->Parameters->NumParameters; p++) {
         for (unsigned int i = 0; i < 4; i++) {
            stage_prog_data->param[uniforms++] =
               &prog->Parameters->ParameterValues[p][i].f;
         }
      }
   }

   fp_input_regs = rzalloc_array(mem_ctx, fs_reg, VARYING_SLOT_MAX);
   for (int i = 0; i < VARYING_SLOT_MAX; i++) {
      if (prog->InputsRead & BITFIELD64_BIT(i)) {
         /* Make up a dummy instruction to reuse code for emitting
          * interpolation.
          */
         ir_variable *ir = new(mem_ctx) ir_variable(glsl_type::vec4_type,
                                                    "fp_input",
                                                    ir_var_shader_in);
         ir->data.location = i;

         this->current_annotation = ralloc_asprintf(ctx, "interpolate input %d",
                                                    i);

         switch (i) {
         case VARYING_SLOT_POS:
            ir->data.pixel_center_integer = fp->PixelCenterInteger;
            ir->data.origin_upper_left = fp->OriginUpperLeft;
            fp_input_regs[i] = *emit_fragcoord_interpolation(ir);
            break;
         case VARYING_SLOT_FACE:
            fp_input_regs[i] = *emit_frontfacing_interpolation(ir);
            break;
         default:
            fp_input_regs[i] = *emit_general_interpolation(ir);

            if (i == VARYING_SLOT_FOGC) {
               emit(MOV(offset(fp_input_regs[i], 1), fs_reg(0.0f)));
               emit(MOV(offset(fp_input_regs[i], 2), fs_reg(0.0f)));
               emit(MOV(offset(fp_input_regs[i], 3), fs_reg(1.0f)));
            }

            break;
         }

         this->current_annotation = NULL;
      }
   }
}

fs_reg
fs_visitor::get_fp_dst_reg(const prog_dst_register *dst)
{
   switch (dst->File) {
   case PROGRAM_TEMPORARY:
      return fp_temp_regs[dst->Index];

   case PROGRAM_OUTPUT:
      if (dst->Index == FRAG_RESULT_DEPTH) {
         if (frag_depth.file == BAD_FILE)
            frag_depth = fs_reg(this, glsl_type::vec4_type);
         return frag_depth;
      } else if (dst->Index == FRAG_RESULT_COLOR) {
         if (outputs[0].file == BAD_FILE) {
            outputs[0] = fs_reg(this, glsl_type::vec4_type);
            output_components[0] = 4;

            /* Tell emit_fb_writes() to smear fragment.color across all the
             * color attachments.
             */
            for (int i = 1; i < c->key.nr_color_regions; i++) {
               outputs[i] = outputs[0];
               output_components[i] = output_components[0];
            }
         }
         return outputs[0];
      } else {
         int output_index = dst->Index - FRAG_RESULT_DATA0;
         if (outputs[output_index].file == BAD_FILE) {
            outputs[output_index] = fs_reg(this, glsl_type::vec4_type);
         }
         output_components[output_index] = 4;
         return outputs[output_index];
      }

   case PROGRAM_UNDEFINED:
      return fs_reg();

   default:
      _mesa_problem(ctx, "bad dst register file: %s\n",
                    _mesa_register_file_name((gl_register_file)dst->File));
      return fs_reg(this, glsl_type::vec4_type);
   }
}

fs_reg
fs_visitor::get_fp_src_reg(const prog_src_register *src)
{
   struct gl_program_parameter_list *plist = prog->Parameters;

   fs_reg result;

   assert(!src->Abs);

   switch (src->File) {
   case PROGRAM_UNDEFINED:
      return fs_reg();
   case PROGRAM_TEMPORARY:
      result = fp_temp_regs[src->Index];
      break;

   case PROGRAM_INPUT:
      result = fp_input_regs[src->Index];
      break;

   case PROGRAM_STATE_VAR:
   case PROGRAM_UNIFORM:
   case PROGRAM_CONSTANT:
      /* We actually want to look at the type in the Parameters list for this,
       * because this lets us upload constant builtin uniforms, as actual
       * constants.
       */
      switch (plist->Parameters[src->Index].Type) {
      case PROGRAM_CONSTANT: {
         result = fs_reg(this, glsl_type::vec4_type);

         for (int i = 0; i < 4; i++) {
            emit(MOV(offset(result, i),
                     fs_reg(plist->ParameterValues[src->Index][i].f)));
         }
         break;
      }

      case PROGRAM_STATE_VAR:
      case PROGRAM_UNIFORM:
         result = fs_reg(UNIFORM, src->Index * 4);
         break;

      default:
         _mesa_problem(ctx, "bad uniform src register file: %s\n",
                       _mesa_register_file_name((gl_register_file)src->File));
         return fs_reg(this, glsl_type::vec4_type);
      }
      break;

   default:
      _mesa_problem(ctx, "bad src register file: %s\n",
                    _mesa_register_file_name((gl_register_file)src->File));
      return fs_reg(this, glsl_type::vec4_type);
   }

   if (src->Swizzle != SWIZZLE_NOOP || src->Negate) {
      fs_reg unswizzled = result;
      result = fs_reg(this, glsl_type::vec4_type);
      for (int i = 0; i < 4; i++) {
         bool negate = src->Negate & (1 << i);
         /* The ZERO, ONE, and Negate options are only used for OPCODE_SWZ,
          * but it costs us nothing to support it.
          */
         int src_swiz = GET_SWZ(src->Swizzle, i);
         if (src_swiz == SWIZZLE_ZERO) {
            emit(MOV(offset(result, i), fs_reg(0.0f)));
         } else if (src_swiz == SWIZZLE_ONE) {
            emit(MOV(offset(result, i),
                     negate ? fs_reg(-1.0f) : fs_reg(1.0f)));
         } else {
            fs_reg src = offset(unswizzled, src_swiz);
            if (negate)
               src.negate = !src.negate;
            emit(MOV(offset(result, i), src));
         }
      }
   }

   return result;
}
