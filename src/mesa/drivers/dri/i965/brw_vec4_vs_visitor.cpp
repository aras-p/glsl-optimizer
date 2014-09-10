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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


#include "brw_vs.h"
#include "main/context.h"


namespace brw {

void
vec4_vs_visitor::emit_prolog()
{
   dst_reg sign_recovery_shift;
   dst_reg normalize_factor;
   dst_reg es3_normalize_factor;

   for (int i = 0; i < VERT_ATTRIB_MAX; i++) {
      if (vs_prog_data->inputs_read & BITFIELD64_BIT(i)) {
         uint8_t wa_flags = vs_compile->key.gl_attrib_wa_flags[i];
         dst_reg reg(ATTR, i);
         dst_reg reg_d = reg;
         reg_d.type = BRW_REGISTER_TYPE_D;
         dst_reg reg_ud = reg;
         reg_ud.type = BRW_REGISTER_TYPE_UD;

         /* Do GL_FIXED rescaling for GLES2.0.  Our GL_FIXED attributes
          * come in as floating point conversions of the integer values.
          */
         if (wa_flags & BRW_ATTRIB_WA_COMPONENT_MASK) {
            dst_reg dst = reg;
            dst.type = brw_type_for_base_type(glsl_type::vec4_type);
            dst.writemask = (1 << (wa_flags & BRW_ATTRIB_WA_COMPONENT_MASK)) - 1;
            emit(MUL(dst, src_reg(dst), src_reg(1.0f / 65536.0f)));
         }

         /* Do sign recovery for 2101010 formats if required. */
         if (wa_flags & BRW_ATTRIB_WA_SIGN) {
            if (sign_recovery_shift.file == BAD_FILE) {
               /* shift constant: <22,22,22,30> */
               sign_recovery_shift = dst_reg(this, glsl_type::uvec4_type);
               emit(MOV(writemask(sign_recovery_shift, WRITEMASK_XYZ), src_reg(22u)));
               emit(MOV(writemask(sign_recovery_shift, WRITEMASK_W), src_reg(30u)));
            }

            emit(SHL(reg_ud, src_reg(reg_ud), src_reg(sign_recovery_shift)));
            emit(ASR(reg_d, src_reg(reg_d), src_reg(sign_recovery_shift)));
         }

         /* Apply BGRA swizzle if required. */
         if (wa_flags & BRW_ATTRIB_WA_BGRA) {
            src_reg temp = src_reg(reg);
            temp.swizzle = BRW_SWIZZLE4(2,1,0,3);
            emit(MOV(reg, temp));
         }

         if (wa_flags & BRW_ATTRIB_WA_NORMALIZE) {
            /* ES 3.0 has different rules for converting signed normalized
             * fixed-point numbers than desktop GL.
             */
            if (_mesa_is_gles3(ctx) && (wa_flags & BRW_ATTRIB_WA_SIGN)) {
               /* According to equation 2.2 of the ES 3.0 specification,
                * signed normalization conversion is done by:
                *
                * f = c / (2^(b-1)-1)
                */
               if (es3_normalize_factor.file == BAD_FILE) {
                  /* mul constant: 1 / (2^(b-1) - 1) */
                  es3_normalize_factor = dst_reg(this, glsl_type::vec4_type);
                  emit(MOV(writemask(es3_normalize_factor, WRITEMASK_XYZ),
                           src_reg(1.0f / ((1<<9) - 1))));
                  emit(MOV(writemask(es3_normalize_factor, WRITEMASK_W),
                           src_reg(1.0f / ((1<<1) - 1))));
               }

               dst_reg dst = reg;
               dst.type = brw_type_for_base_type(glsl_type::vec4_type);
               emit(MOV(dst, src_reg(reg_d)));
               emit(MUL(dst, src_reg(dst), src_reg(es3_normalize_factor)));
               emit_minmax(BRW_CONDITIONAL_G, dst, src_reg(dst), src_reg(-1.0f));
            } else {
               /* The following equations are from the OpenGL 3.2 specification:
                *
                * 2.1 unsigned normalization
                * f = c/(2^n-1)
                *
                * 2.2 signed normalization
                * f = (2c+1)/(2^n-1)
                *
                * Both of these share a common divisor, which is represented by
                * "normalize_factor" in the code below.
                */
               if (normalize_factor.file == BAD_FILE) {
                  /* 1 / (2^b - 1) for b=<10,10,10,2> */
                  normalize_factor = dst_reg(this, glsl_type::vec4_type);
                  emit(MOV(writemask(normalize_factor, WRITEMASK_XYZ),
                           src_reg(1.0f / ((1<<10) - 1))));
                  emit(MOV(writemask(normalize_factor, WRITEMASK_W),
                           src_reg(1.0f / ((1<<2) - 1))));
               }

               dst_reg dst = reg;
               dst.type = brw_type_for_base_type(glsl_type::vec4_type);
               emit(MOV(dst, src_reg((wa_flags & BRW_ATTRIB_WA_SIGN) ? reg_d : reg_ud)));

               /* For signed normalization, we want the numerator to be 2c+1. */
               if (wa_flags & BRW_ATTRIB_WA_SIGN) {
                  emit(MUL(dst, src_reg(dst), src_reg(2.0f)));
                  emit(ADD(dst, src_reg(dst), src_reg(1.0f)));
               }

               emit(MUL(dst, src_reg(dst), src_reg(normalize_factor)));
            }
         }

         if (wa_flags & BRW_ATTRIB_WA_SCALE) {
            dst_reg dst = reg;
            dst.type = brw_type_for_base_type(glsl_type::vec4_type);
            emit(MOV(dst, src_reg((wa_flags & BRW_ATTRIB_WA_SIGN) ? reg_d : reg_ud)));
         }
      }
   }
}


dst_reg *
vec4_vs_visitor::make_reg_for_system_value(ir_variable *ir)
{
   /* VertexID is stored by the VF as the last vertex element, but
    * we don't represent it with a flag in inputs_read, so we call
    * it VERT_ATTRIB_MAX, which setup_attributes() picks up on.
    */
   dst_reg *reg = new(mem_ctx) dst_reg(ATTR, VERT_ATTRIB_MAX);

   switch (ir->data.location) {
   case SYSTEM_VALUE_BASE_VERTEX:
      reg->writemask = WRITEMASK_X;
      vs_prog_data->uses_vertexid = true;
      break;
   case SYSTEM_VALUE_VERTEX_ID:
   case SYSTEM_VALUE_VERTEX_ID_ZERO_BASE:
      reg->writemask = WRITEMASK_Z;
      vs_prog_data->uses_vertexid = true;
      break;
   case SYSTEM_VALUE_INSTANCE_ID:
      reg->writemask = WRITEMASK_W;
      vs_prog_data->uses_instanceid = true;
      break;
   default:
      unreachable("not reached");
   }

   return reg;
}


void
vec4_vs_visitor::emit_urb_write_header(int mrf)
{
   /* No need to do anything for VS; an implied write to this MRF will be
    * performed by VS_OPCODE_URB_WRITE.
    */
   (void) mrf;
}


vec4_instruction *
vec4_vs_visitor::emit_urb_write_opcode(bool complete)
{
   /* For VS, the URB writes end the thread. */
   if (complete) {
      if (INTEL_DEBUG & DEBUG_SHADER_TIME)
         emit_shader_time_end();
   }

   vec4_instruction *inst = emit(VS_OPCODE_URB_WRITE);
   inst->urb_write_flags = complete ?
      BRW_URB_WRITE_EOT_COMPLETE : BRW_URB_WRITE_NO_FLAGS;

   return inst;
}


void
vec4_vs_visitor::emit_thread_end()
{
   /* For VS, we always end the thread by emitting a single vertex.
    * emit_urb_write_opcode() will take care of setting the eot flag on the
    * SEND instruction.
    */
   emit_vertex();
}


vec4_vs_visitor::vec4_vs_visitor(struct brw_context *brw,
                                 struct brw_vs_compile *vs_compile,
                                 struct brw_vs_prog_data *vs_prog_data,
                                 struct gl_shader_program *prog,
                                 void *mem_ctx)
   : vec4_visitor(brw, &vs_compile->base, &vs_compile->vp->program.Base,
                  &vs_compile->key.base, &vs_prog_data->base, prog,
                  MESA_SHADER_VERTEX,
                  mem_ctx, INTEL_DEBUG & DEBUG_VS, false /* no_spills */,
                  ST_VS, ST_VS_WRITTEN, ST_VS_RESET),
     vs_compile(vs_compile),
     vs_prog_data(vs_prog_data)
{
}


} /* namespace brw */
