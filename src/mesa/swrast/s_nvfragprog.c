/* $Id: s_nvfragprog.c,v 1.2 2003/02/17 15:38:04 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */



#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "nvfragprog.h"
#include "macros.h"
#include "mmath.h"

#include "s_nvfragprog.h"


/**
 * Fetch a texel.
 */
static void
fetch_texel( GLcontext *ctx, const GLfloat texcoord[4], GLuint unit,
             GLenum target, GLfloat color[4] )
{
   const struct gl_texture_object *texObj;

   /* XXX Use swrast->TextureSample[texUnit]() to sample texture.
    * Needs to be swrast->TextureSample[target][texUnit]() though.
    */

   switch (target) {
      case GL_TEXTURE_1D:
         texObj = ctx->Texture.Unit[unit].Current1D;
         break;
      case GL_TEXTURE_2D:
         texObj = ctx->Texture.Unit[unit].Current2D;
         break;
      case GL_TEXTURE_3D:
         texObj = ctx->Texture.Unit[unit].Current3D;
         break;
      case GL_TEXTURE_CUBE_MAP:
         texObj = ctx->Texture.Unit[unit].CurrentCubeMap;
         break;
      case GL_TEXTURE_RECTANGLE_NV:
         texObj = ctx->Texture.Unit[unit].CurrentRect;
         break;
      default:
         _mesa_problem(ctx, "Invalid target in fetch_texel");
   }

   if (texObj->Complete) {
      const struct gl_texture_image *texImage;
      GLint col, row, img;
      GLchan texel[4];
      col = IROUND(texcoord[0] * texImage->Width); /* XXX temporary! */
      row = IROUND(texcoord[1] * texImage->Height); /* XXX temporary! */
      img = 0;
      texImage->FetchTexel(texImage, col, row, img, texel);
      /* XXX texture format? */
      color[0] = CHAN_TO_FLOAT(texel[0]);
      color[1] = CHAN_TO_FLOAT(texel[1]);
      color[2] = CHAN_TO_FLOAT(texel[2]);
      color[3] = CHAN_TO_FLOAT(texel[3]);
   }
   else {
      ASSIGN_4V(color, 0.0, 0.0, 0.0, 0.0);
   }
}


/**
 * Fetch a texel w/ partial derivatives.
 */
static void
fetch_texel_deriv( GLcontext *ctx, const GLfloat texcoord[4],
                   const GLfloat dtdx[4], const GLfloat dtdy[4],
                   GLuint unit, GLenum target, GLfloat color[4] )
{
   /* XXX to do */

}



/**
 * Fetch a 4-element float vector from the given source register.
 * Apply swizzling and negating as needed.
 */
static void
fetch_vector4( const struct fp_src_register *source,
               const struct fp_machine *machine,
               GLfloat result[4] )
{
   const GLfloat *src;

   /*
   if (source->RelAddr) {
      GLint reg = source->Register + machine->AddressReg;
      if (reg < VP_PROG_REG_START || reg > VP_PROG_REG_END)
         src = zero;
      else
         src = machine->Registers[reg];
   }
   else
   */

   src = machine->Registers[source->Register];

   result[0] = src[source->Swizzle[0]];
   result[1] = src[source->Swizzle[1]];
   result[2] = src[source->Swizzle[2]];
   result[3] = src[source->Swizzle[3]];

   if (source->NegateBase) {
      result[0] = -result[0];
      result[1] = -result[1];
      result[2] = -result[2];
      result[3] = -result[3];
   }
   if (source->Abs) {
      result[0] = FABSF(result[0]);
      result[1] = FABSF(result[1]);
      result[2] = FABSF(result[2]);
      result[3] = FABSF(result[3]);
   }
   if (source->NegateAbs) {
      result[0] = -result[0];
      result[1] = -result[1];
      result[2] = -result[2];
      result[3] = -result[3];
   }
}


/**
 * As above, but only return result[0] element.
 */
static void
fetch_vector1( const struct fp_src_register *source,
               const struct fp_machine *machine,
               GLfloat result[4] )
{
   const GLfloat *src;

   /*
   if (source->RelAddr) {
      GLint reg = source->Register + machine->AddressReg;
      if (reg < VP_PROG_REG_START || reg > VP_PROG_REG_END)
         src = zero;
      else
         src = machine->Registers[reg];
   }
   else
   */

   src = machine->Registers[source->Register];

   result[0] = src[source->Swizzle[0]];

   if (source->NegateBase) {
      result[0] = -result[0];
   }
   if (source->Abs) {
      result[0] = FABSF(result[0]);
   }
   if (source->NegateAbs) {
      result[0] = -result[0];
   }
}


/*
 * Test value against zero and return GT, LT, EQ or UN if NaN.
 */
static INLINE GLuint
generate_cc( float value )
{
   if (value != value)
      return COND_UN;  /* NaN */
   if (value > 0.0F)
      return COND_GT;
   if (value < 0.0F)
      return COND_LT;
   return COND_EQ;
}

/*
 * Test if the ccMaskRule is satisfied by the given condition code.
 * Used to mask destination writes according to the current condition codee.
 */
static INLINE GLboolean
test_cc(GLuint condCode, GLuint ccMaskRule)
{
   switch (ccMaskRule) {
   case COND_EQ: return (condCode == COND_EQ);
   case COND_NE: return (condCode != COND_EQ);
   case COND_LT: return (condCode == COND_LT);
   case COND_GE: return (condCode == COND_GT || condCode == COND_EQ);
   case COND_LE: return (condCode == COND_LT || condCode == COND_EQ);
   case COND_GT: return (condCode == COND_GT);
   case COND_TR: return GL_TRUE;
   case COND_FL: return GL_FALSE;
   default:      return GL_TRUE;
   }
}


/**
 * Store 4 floats into a register.
 */
static void
store_vector4( const struct fp_dst_register *dest, struct fp_machine *machine,
               const GLfloat value[4], GLboolean clamp, GLboolean updateCC )
{
   GLfloat *dstReg = machine->Registers[dest->Register];
   GLfloat clampedValue[4];
   const GLboolean *writeMask = dest->WriteMask;
   GLboolean condWriteMask[4];

   if (clamp) {
      clampedValue[0] = CLAMP(value[0], 0.0F, 1.0F);
      clampedValue[1] = CLAMP(value[1], 0.0F, 1.0F);
      clampedValue[2] = CLAMP(value[2], 0.0F, 1.0F);
      clampedValue[3] = CLAMP(value[3], 0.0F, 1.0F);
      value = clampedValue;
   }

   if (dest->CondMask != COND_TR) {
      condWriteMask[0] = writeMask[0]
         && test_cc(machine->CondCodes[dest->CondSwizzle[0]], dest->CondMask);
      condWriteMask[1] = writeMask[1]
         && test_cc(machine->CondCodes[dest->CondSwizzle[1]], dest->CondMask);
      condWriteMask[2] = writeMask[2]
         && test_cc(machine->CondCodes[dest->CondSwizzle[2]], dest->CondMask);
      condWriteMask[3] = writeMask[3]
         && test_cc(machine->CondCodes[dest->CondSwizzle[3]], dest->CondMask);
      writeMask = condWriteMask;
   }

   if (writeMask[0]) {
      dstReg[0] = value[0];
      if (updateCC)
         machine->CondCodes[0] = generate_cc(value[0]);
   }
   if (writeMask[1]) {
      dstReg[1] = value[1];
      if (updateCC)
         machine->CondCodes[1] = generate_cc(value[1]);
   }
   if (writeMask[2]) {
      dstReg[2] = value[2];
      if (updateCC)
         machine->CondCodes[2] = generate_cc(value[2]);
   }
   if (writeMask[3]) {
      dstReg[3] = value[3];
      if (updateCC)
         machine->CondCodes[3] = generate_cc(value[3]);
   }
}


/**
 * Execute the given vertex program
 */
static void
execute_program(GLcontext *ctx, const struct fragment_program *program)
{
   struct fp_machine *machine = &ctx->FragmentProgram.Machine;
   const struct fp_instruction *inst;

   for (inst = program->Instructions; inst->Opcode != FP_OPCODE_END; inst++) {
      switch (inst->Opcode) {
         case FP_OPCODE_ADD:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               fetch_vector4( &inst->SrcReg[1], machine, b );
               result[0] = a[0] + b[0];
               result[1] = a[1] + b[1];
               result[2] = a[2] + b[2];
               result[3] = a[3] + b[3];
               store_vector4( &inst->DstReg, machine, result, inst->Saturate,
                              inst->UpdateCondRegister );
            }
            break;
         case FP_OPCODE_COS:
            {
               GLfloat a[4], result[4];
               fetch_vector1( &inst->SrcReg[0], machine, a );
               result[0] = result[1] = result[2] = result[3] = cos(a[0]);
               store_vector4( &inst->DstReg, machine, result, inst->Saturate,
                              inst->UpdateCondRegister );
            }
            break;
         case FP_OPCODE_DP3:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               fetch_vector4( &inst->SrcReg[1], machine, b );
               result[0] = result[1] = result[2] = result[3] = 
                  a[0] + b[0] + a[1] * b[1] + a[2] * b[2];
               store_vector4( &inst->DstReg, machine, result, inst->Saturate,
                              inst->UpdateCondRegister );
            }
            break;
         case FP_OPCODE_DP4:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               fetch_vector4( &inst->SrcReg[1], machine, b );
               result[0] = result[1] = result[2] = result[3] = 
                  a[0] + b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
               store_vector4( &inst->DstReg, machine, result, inst->Saturate,
                              inst->UpdateCondRegister );
            }
            break;
         case FP_OPCODE_KIL:
            {
               const GLuint *swizzle = inst->DstReg.CondSwizzle;
               const GLuint condMask = inst->DstReg.CondMask;
               if (test_cc(machine->CondCodes[swizzle[0]], condMask) ||
                   test_cc(machine->CondCodes[swizzle[1]], condMask) ||
                   test_cc(machine->CondCodes[swizzle[2]], condMask) ||
                   test_cc(machine->CondCodes[swizzle[3]], condMask))
                  return;
            }
            break;
         case FP_OPCODE_LRP:
            {
               GLfloat a[4], b[4], c[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               fetch_vector4( &inst->SrcReg[1], machine, b );
               fetch_vector4( &inst->SrcReg[2], machine, c );
               result[0] = a[0] * b[0] + (1.0F - a[0]) * c[0];
               result[1] = a[1] * b[1] + (1.0F - a[1]) * c[1];
               result[2] = a[2] * b[2] + (1.0F - a[2]) * c[2];
               result[3] = a[3] * b[3] + (1.0F - a[3]) * c[3];
               store_vector4( &inst->DstReg, machine, result, inst->Saturate,
                              inst->UpdateCondRegister );
            }
            break;
         case FP_OPCODE_MOV:
            {
               GLfloat t[4];
               fetch_vector4( &inst->SrcReg[0], machine, t );
               store_vector4( &inst->DstReg, machine, t, inst->Saturate,
                              inst->UpdateCondRegister );
            }
            break;
         case FP_OPCODE_SEQ:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               fetch_vector4( &inst->SrcReg[1], machine, b );
               result[0] = (a[0] == b[0]) ? 1.0F : 0.0F;
               result[1] = (a[1] == b[1]) ? 1.0F : 0.0F;
               result[2] = (a[2] == b[2]) ? 1.0F : 0.0F;
               result[3] = (a[3] == b[3]) ? 1.0F : 0.0F;
               store_vector4( &inst->DstReg, machine, result, inst->Saturate,
                              inst->UpdateCondRegister );
            }
            break;
         case FP_OPCODE_TEX:
            /* Texel lookup */
            {
               GLfloat texcoord[4], color[4];
               fetch_vector4( &inst->SrcReg[0], machine, texcoord );
               fetch_texel( ctx, texcoord, inst->TexSrcUnit,
                            inst->TexSrcTarget, color );
               store_vector4( &inst->DstReg, machine, color, inst->Saturate,
                              inst->UpdateCondRegister );
            }
            break;
         case FP_OPCODE_TXD:
            /* Texture lookup w/ partial derivatives for LOD */
            {
               GLfloat texcoord[4], dtdx[4], dtdy[4], color[4];
               fetch_vector4( &inst->SrcReg[0], machine, texcoord );
               fetch_vector4( &inst->SrcReg[1], machine, dtdx );
               fetch_vector4( &inst->SrcReg[2], machine, dtdy );
               fetch_texel_deriv( ctx, texcoord, dtdx, dtdy, inst->TexSrcUnit,
                                  inst->TexSrcTarget, color );
               store_vector4( &inst->DstReg, machine, color, inst->Saturate,
                              inst->UpdateCondRegister );
            }
            break;
         case FP_OPCODE_TXP:
            /* Texture lookup w/ perspective divide */
            {
               GLfloat texcoord[4], color[4];
               fetch_vector4( &inst->SrcReg[0], machine, texcoord );
               texcoord[0] /= texcoord[3];
               texcoord[1] /= texcoord[3];
               texcoord[2] /= texcoord[3];
               fetch_texel( ctx, texcoord, inst->TexSrcUnit,
                            inst->TexSrcTarget, color );
               store_vector4( &inst->DstReg, machine, color, inst->Saturate,
                              inst->UpdateCondRegister );
            }
            break;
         default:
            _mesa_problem(ctx, "Bad opcode in _mesa_exec_fragment_program");
            return;
      }
   }

}



void
_swrast_exec_nv_fragment_program( GLcontext *ctx, struct sw_span *span )
{
   GLuint i;

   for (i = 0; i < span->end; i++) {
      GLfloat *wpos = ctx->FragmentProgram.Machine.Registers[0];
      GLfloat *col0 = ctx->FragmentProgram.Machine.Registers[1];
      GLfloat *col1 = ctx->FragmentProgram.Machine.Registers[2];
      GLfloat *fogc = ctx->FragmentProgram.Machine.Registers[3];
      const GLfloat *colOut = ctx->FragmentProgram.Machine.Registers[FP_OUTPUT_REG_START];
      GLuint j;

      /* Clear temporary registers */
      for (j = 0; j < MAX_NV_FRAGMENT_PROGRAM_TEMPS; j++) {
         ctx->FragmentProgram.Machine.Registers[FP_TEMP_REG_START+j][0] = 0.0F;
         ctx->FragmentProgram.Machine.Registers[FP_TEMP_REG_START+j][1] = 0.0F;
         ctx->FragmentProgram.Machine.Registers[FP_TEMP_REG_START+j][2] = 0.0F;
         ctx->FragmentProgram.Machine.Registers[FP_TEMP_REG_START+j][3] = 0.0F;
      }

      /* Load input registers */
      wpos[0] = span->x + i;
      wpos[1] = span->y + i;
      wpos[2] = span->array->z[i];
      wpos[3] = 1.0;

      col0[0] = CHAN_TO_FLOAT(span->array->rgba[i][RCOMP]);
      col0[1] = CHAN_TO_FLOAT(span->array->rgba[i][GCOMP]);
      col0[2] = CHAN_TO_FLOAT(span->array->rgba[i][BCOMP]);
      col0[3] = CHAN_TO_FLOAT(span->array->rgba[i][ACOMP]);

      col1[0] = CHAN_TO_FLOAT(span->array->spec[i][RCOMP]);
      col1[1] = CHAN_TO_FLOAT(span->array->spec[i][GCOMP]);
      col1[2] = CHAN_TO_FLOAT(span->array->spec[i][BCOMP]);
      col1[3] = CHAN_TO_FLOAT(span->array->spec[i][ACOMP]);

      fogc[0] = span->array->fog[i];

      execute_program(ctx, ctx->FragmentProgram.Current);

      /* Store output registers */
      UNCLAMPED_FLOAT_TO_CHAN(span->array->rgba[i][RCOMP], colOut[0]);
      UNCLAMPED_FLOAT_TO_CHAN(span->array->rgba[i][GCOMP], colOut[1]);
      UNCLAMPED_FLOAT_TO_CHAN(span->array->rgba[i][BCOMP], colOut[2]);
      UNCLAMPED_FLOAT_TO_CHAN(span->array->rgba[i][ACOMP], colOut[3]);
   }
}

