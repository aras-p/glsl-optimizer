/* $Id: s_nvfragprog.c,v 1.6 2003/03/04 16:34:03 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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

#include "s_nvfragprog.h"



/**
 * Fetch a texel.
 */
static void
fetch_texel( GLcontext *ctx, const GLfloat texcoord[4], GLuint unit,
             GLuint targetIndex, GLfloat color[4] )
{
   const GLfloat *lambda = NULL;
   GLchan rgba[4];
   SWcontext *swrast = SWRAST_CONTEXT(ctx);
   const struct gl_texture_object *texObj = NULL;

   switch (targetIndex) {
      case TEXTURE_1D_INDEX:
         texObj = ctx->Texture.Unit[unit].Current1D;
         break;
      case TEXTURE_2D_INDEX:
         texObj = ctx->Texture.Unit[unit].Current2D;
         break;
      case TEXTURE_3D_INDEX:
         texObj = ctx->Texture.Unit[unit].Current3D;
         break;
      case TEXTURE_CUBE_INDEX:
         texObj = ctx->Texture.Unit[unit].CurrentCubeMap;
         break;
      case TEXTURE_RECT_INDEX:
         texObj = ctx->Texture.Unit[unit].CurrentRect;
         break;
      default:
         _mesa_problem(ctx, "Invalid target in fetch_texel");
   }

   swrast->TextureSample[unit](ctx, unit, texObj, 1,
                               (const GLfloat (*)[4]) &texcoord,
                               lambda, &rgba);
}


/**
 * Fetch a texel w/ partial derivatives.
 */
static void
fetch_texel_deriv( GLcontext *ctx, const GLfloat texcoord[4],
                   const GLfloat dtdx[4], const GLfloat dtdy[4],
                   GLuint unit, GLuint targetIndex, GLfloat color[4] )
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
   const GLfloat *src = machine->Registers[source->Register];

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
 * Store 4 floats into a register.  Observe the instructions saturate and
 * set-condition-code flags.
 */
static void
store_vector4( const struct fp_instruction *inst,
               struct fp_machine *machine,
               const GLfloat value[4] )
{
   const struct fp_dst_register *dest = &(inst->DstReg);
   const GLboolean clamp = inst->Saturate;
   const GLboolean updateCC = inst->UpdateCondRegister;
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
 * \return GL_TRUE if program completed or GL_FALSE if program executed KIL.
 */
static GLboolean
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
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_COS:
            {
               GLfloat a[4], result[4];
               fetch_vector1( &inst->SrcReg[0], machine, a );
               result[0] = result[1] = result[2] = result[3] = _mesa_cos(a[0]);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_DDX: /* Partial derivative with respect to X */
            {
               GLfloat a[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               result[0] = 0; /* XXX fix */
               result[1] = 0;
               result[2] = 0;
               result[3] = 0;
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_DDY: /* Partial derivative with respect to Y */
            {
               GLfloat a[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               result[0] = 0; /* XXX fix */
               result[1] = 0;
               result[2] = 0;
               result[3] = 0;
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_DP3:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               fetch_vector4( &inst->SrcReg[1], machine, b );
               result[0] = result[1] = result[2] = result[3] = 
                  a[0] + b[0] + a[1] * b[1] + a[2] * b[2];
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_DP4:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               fetch_vector4( &inst->SrcReg[1], machine, b );
               result[0] = result[1] = result[2] = result[3] = 
                  a[0] + b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_DST: /* Distance vector */
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               fetch_vector4( &inst->SrcReg[1], machine, b );
               result[0] = 1.0F;
               result[1] = a[1] * b[1];
               result[2] = a[2];
               result[3] = b[3];
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_EX2: /* Exponential base 2 */
            {
               GLfloat a[4], result[4];
               fetch_vector1( &inst->SrcReg[0], machine, a );
               result[0] = result[1] = result[2] = result[3] =
                  (GLfloat) _mesa_pow(2.0, a[0]);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_FLR:
            {
               GLfloat a[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               result[0] = FLOORF(a[0]);
               result[1] = FLOORF(a[1]);
               result[2] = FLOORF(a[2]);
               result[3] = FLOORF(a[3]);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_FRC:
            {
               GLfloat a[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               result[0] = a[0] - FLOORF(a[0]);
               result[1] = a[1] - FLOORF(a[1]);
               result[2] = a[2] - FLOORF(a[2]);
               result[3] = a[3] - FLOORF(a[3]);
               store_vector4( inst, machine, result );
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
                  return GL_FALSE;
            }
            break;
         case FP_OPCODE_LG2:  /* log base 2 */
            {
               GLfloat a[4], result[4];
               fetch_vector1( &inst->SrcReg[0], machine, a );
               result[0] = result[1] = result[2] = result[3]
                  = LOG2(a[0]);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_LIT:
            {
               GLfloat a[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               if (a[0] < 0.0F)
                  a[0] = 0.0F;
               if (a[1] < 0.0F)
                  a[1] = 0.0F;
               result[0] = 1.0F;
               result[1] = a[0];
               result[2] = (a[0] > 0.0) ? _mesa_pow(2.0, a[3]) : 0.0F;
               result[3] = 1.0F;
               store_vector4( inst, machine, result );
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
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_MAD:
            {
               GLfloat a[4], b[4], c[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               fetch_vector4( &inst->SrcReg[1], machine, b );
               fetch_vector4( &inst->SrcReg[2], machine, c );
               result[0] = a[0] * b[0] + c[0];
               result[1] = a[1] * b[1] + c[1];
               result[2] = a[2] * b[2] + c[2];
               result[3] = a[3] * b[3] + c[3];
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_MAX:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               fetch_vector4( &inst->SrcReg[1], machine, b );
               result[0] = MAX2(a[0], b[0]);
               result[1] = MAX2(a[1], b[1]);
               result[2] = MAX2(a[2], b[2]);
               result[3] = MAX2(a[3], b[3]);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_MIN:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               fetch_vector4( &inst->SrcReg[1], machine, b );
               result[0] = MIN2(a[0], b[0]);
               result[1] = MIN2(a[1], b[1]);
               result[2] = MIN2(a[2], b[2]);
               result[3] = MIN2(a[3], b[3]);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_MOV:
            {
               GLfloat result[4];
               fetch_vector4( &inst->SrcReg[0], machine, result );
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_MUL:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               fetch_vector4( &inst->SrcReg[1], machine, b );
               result[0] = a[0] * b[0];
               result[1] = a[1] * b[1];
               result[2] = a[2] * b[2];
               result[3] = a[3] * b[3];
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_PK2H: /* pack two 16-bit floats */
            /* XXX this is probably wrong */
            {
               GLfloat a[4], result[4];
               const GLuint *rawBits = (const GLuint *) a;
               GLuint *rawResult = (GLuint *) result;
               fetch_vector4( &inst->SrcReg[0], machine, a );
               rawResult[0] = rawResult[1] = rawResult[2] = rawResult[3]
                  = rawBits[0] | (rawBits[1] << 16);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_PK2US: /* pack two GLushorts */
            {
               GLfloat a[4], result[4];
               GLuint usx, usy, *rawResult = (GLuint *) result;
               fetch_vector4( &inst->SrcReg[0], machine, a );
               a[0] = CLAMP(a[0], 0.0F, 1.0F);
               a[1] = CLAMP(a[0], 0.0F, 1.0F);
               usx = IROUND(a[0] * 65535.0F);
               usy = IROUND(a[1] * 65535.0F);
               rawResult[0] = rawResult[1] = rawResult[2] = rawResult[3]
                  = usx | (usy << 16);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_PK4B: /* pack four GLbytes */
            {
               GLfloat a[4], result[4];
               GLuint ubx, uby, ubz, ubw, *rawResult = (GLuint *) result;
               fetch_vector4( &inst->SrcReg[0], machine, a );
               a[0] = CLAMP(a[0], -128.0F / 127.0F, 1.0F);
               a[1] = CLAMP(a[1], -128.0F / 127.0F, 1.0F);
               a[2] = CLAMP(a[2], -128.0F / 127.0F, 1.0F);
               a[3] = CLAMP(a[3], -128.0F / 127.0F, 1.0F);
               ubx = IROUND(127.0F * a[0] + 128.0F);
               uby = IROUND(127.0F * a[1] + 128.0F);
               ubz = IROUND(127.0F * a[2] + 128.0F);
               ubw = IROUND(127.0F * a[3] + 128.0F);
               rawResult[0] = rawResult[1] = rawResult[2] = rawResult[3]
                  = ubx | (uby << 8) | (ubz << 16) | (ubw << 24);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_PK4UB: /* pack four GLubytes */
            {
               GLfloat a[4], result[4];
               GLuint ubx, uby, ubz, ubw, *rawResult = (GLuint *) result;
               fetch_vector4( &inst->SrcReg[0], machine, a );
               a[0] = CLAMP(a[0], 0.0F, 1.0F);
               a[1] = CLAMP(a[1], 0.0F, 1.0F);
               a[2] = CLAMP(a[2], 0.0F, 1.0F);
               a[3] = CLAMP(a[3], 0.0F, 1.0F);
               ubx = IROUND(255.0F * a[0]);
               uby = IROUND(255.0F * a[1]);
               ubz = IROUND(255.0F * a[2]);
               ubw = IROUND(255.0F * a[3]);
               rawResult[0] = rawResult[1] = rawResult[2] = rawResult[3]
                  = ubx | (uby << 8) | (ubz << 16) | (ubw << 24);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_POW:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector1( &inst->SrcReg[0], machine, a );
               fetch_vector1( &inst->SrcReg[1], machine, b );
               result[0] = result[1] = result[2] = result[3]
                  = _mesa_pow(a[0], b[0]);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_RCP:
            {
               GLfloat a[4], result[4];
               fetch_vector1( &inst->SrcReg[0], machine, a );
               result[0] = result[1] = result[2] = result[3]
                  = 1.0F / a[0];
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_RFL:
            {
               GLfloat axis[4], dir[4], result[4], tmp[4];
               fetch_vector4( &inst->SrcReg[0], machine, axis );
               fetch_vector4( &inst->SrcReg[1], machine, dir );
               tmp[3] = axis[0] * axis[0]
                      + axis[1] * axis[1]
                      + axis[2] * axis[2];
               tmp[0] = (2.0F * (axis[0] * dir[0] +
                                 axis[1] * dir[1] +
                                 axis[2] * dir[2])) / tmp[3];
               result[0] = tmp[0] * axis[0] - dir[0];
               result[1] = tmp[0] * axis[1] - dir[1];
               result[2] = tmp[0] * axis[2] - dir[2];
               /* result[3] is never written! XXX enforce in parser! */
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_RSQ: /* 1 / sqrt() */
            {
               GLfloat a[4], result[4];
               fetch_vector1( &inst->SrcReg[0], machine, a );
               result[0] = result[1] = result[2] = result[3] = INV_SQRTF(a[0]);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_SEQ: /* set on equal */
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               fetch_vector4( &inst->SrcReg[1], machine, b );
               result[0] = (a[0] == b[0]) ? 1.0F : 0.0F;
               result[1] = (a[1] == b[1]) ? 1.0F : 0.0F;
               result[2] = (a[2] == b[2]) ? 1.0F : 0.0F;
               result[3] = (a[3] == b[3]) ? 1.0F : 0.0F;
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_SFL: /* set false, operands ignored */
            {
               static const GLfloat result[4] = { 0.0F, 0.0F, 0.0F, 0.0F };
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_SGE: /* set on greater or equal */
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               fetch_vector4( &inst->SrcReg[1], machine, b );
               result[0] = (a[0] >= b[0]) ? 1.0F : 0.0F;
               result[1] = (a[1] >= b[1]) ? 1.0F : 0.0F;
               result[2] = (a[2] >= b[2]) ? 1.0F : 0.0F;
               result[3] = (a[3] >= b[3]) ? 1.0F : 0.0F;
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_SGT: /* set on greater */
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               fetch_vector4( &inst->SrcReg[1], machine, b );
               result[0] = (a[0] > b[0]) ? 1.0F : 0.0F;
               result[1] = (a[1] > b[1]) ? 1.0F : 0.0F;
               result[2] = (a[2] > b[2]) ? 1.0F : 0.0F;
               result[3] = (a[3] > b[3]) ? 1.0F : 0.0F;
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_SIN:
            {
               GLfloat a[4], result[4];
               fetch_vector1( &inst->SrcReg[0], machine, a );
               result[0] = result[1] = result[2] = result[3] = _mesa_sin(a[0]);
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_SLE: /* set on less or equal */
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               fetch_vector4( &inst->SrcReg[1], machine, b );
               result[0] = (a[0] <= b[0]) ? 1.0F : 0.0F;
               result[1] = (a[1] <= b[1]) ? 1.0F : 0.0F;
               result[2] = (a[2] <= b[2]) ? 1.0F : 0.0F;
               result[3] = (a[3] <= b[3]) ? 1.0F : 0.0F;
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_SLT: /* set on less */
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               fetch_vector4( &inst->SrcReg[1], machine, b );
               result[0] = (a[0] < b[0]) ? 1.0F : 0.0F;
               result[1] = (a[1] < b[1]) ? 1.0F : 0.0F;
               result[2] = (a[2] < b[2]) ? 1.0F : 0.0F;
               result[3] = (a[3] < b[3]) ? 1.0F : 0.0F;
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_SNE: /* set on not equal */
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               fetch_vector4( &inst->SrcReg[1], machine, b );
               result[0] = (a[0] != b[0]) ? 1.0F : 0.0F;
               result[1] = (a[1] != b[1]) ? 1.0F : 0.0F;
               result[2] = (a[2] != b[2]) ? 1.0F : 0.0F;
               result[3] = (a[3] != b[3]) ? 1.0F : 0.0F;
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_STR: /* set true, operands ignored */
            {
               static const GLfloat result[4] = { 1.0F, 1.0F, 1.0F, 1.0F };
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_SUB:
            {
               GLfloat a[4], b[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               fetch_vector4( &inst->SrcReg[1], machine, b );
               result[0] = a[0] - b[0];
               result[1] = a[1] - b[1];
               result[2] = a[2] - b[2];
               result[3] = a[3] - b[3];
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_TEX:
            /* Texel lookup */
            {
               GLfloat texcoord[4], color[4];
               fetch_vector4( &inst->SrcReg[0], machine, texcoord );
               fetch_texel( ctx, texcoord, inst->TexSrcUnit,
                            inst->TexSrcIndex, color );
               store_vector4( inst, machine, color );
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
                                  inst->TexSrcIndex, color );
               store_vector4( inst, machine, color );
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
                            inst->TexSrcIndex, color );
               store_vector4( inst, machine, color );
            }
            break;
         case FP_OPCODE_UP2H: /* unpack two 16-bit floats */
            /* XXX this is probably wrong */
            {
               GLfloat a[4], result[4];
               const GLuint *rawBits = (const GLuint *) a;
               GLuint *rawResult = (GLuint *) result;
               fetch_vector1( &inst->SrcReg[0], machine, a );
               rawResult[0] = rawBits[0] & 0xffff;
               rawResult[1] = (rawBits[0] >> 16) & 0xffff;
               rawResult[2] = rawBits[0] & 0xffff;
               rawResult[3] = (rawBits[0] >> 16) & 0xffff;
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_UP2US: /* unpack two GLushorts */
            {
               GLfloat a[4], result[4];
               const GLuint *rawBits = (const GLuint *) a;
               fetch_vector1( &inst->SrcReg[0], machine, a );
               result[0] = (GLfloat) ((rawBits[0] >>  0) & 0xffff) / 65535.0F;
               result[1] = (GLfloat) ((rawBits[0] >> 16) & 0xffff) / 65535.0F;
               result[2] = result[0];
               result[3] = result[1];
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_UP4B: /* unpack four GLbytes */
            {
               GLfloat a[4], result[4];
               const GLuint *rawBits = (const GLuint *) a;
               fetch_vector1( &inst->SrcReg[0], machine, a );
               result[0] = (((rawBits[0] >>  0) & 0xff) - 128) / 127.0F;
               result[0] = (((rawBits[0] >>  8) & 0xff) - 128) / 127.0F;
               result[0] = (((rawBits[0] >> 16) & 0xff) - 128) / 127.0F;
               result[0] = (((rawBits[0] >> 24) & 0xff) - 128) / 127.0F;
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_UP4UB: /* unpack four GLubytes */
            {
               GLfloat a[4], result[4];
               const GLuint *rawBits = (const GLuint *) a;
               fetch_vector1( &inst->SrcReg[0], machine, a );
               result[0] = ((rawBits[0] >>  0) & 0xff) / 255.0F;
               result[0] = ((rawBits[0] >>  8) & 0xff) / 255.0F;
               result[0] = ((rawBits[0] >> 16) & 0xff) / 255.0F;
               result[0] = ((rawBits[0] >> 24) & 0xff) / 255.0F;
               store_vector4( inst, machine, result );
            }
            break;
         case FP_OPCODE_X2D: /* 2-D matrix transform */
            {
               GLfloat a[4], b[4], c[4], result[4];
               fetch_vector4( &inst->SrcReg[0], machine, a );
               fetch_vector4( &inst->SrcReg[1], machine, b );
               fetch_vector4( &inst->SrcReg[2], machine, c );
               result[0] = a[0] + b[0] * c[0] + b[1] * c[1];
               result[1] = a[1] + b[0] * c[2] + b[1] * c[3];
               result[2] = a[2] + b[0] * c[0] + b[1] * c[1];
               result[3] = a[3] + b[0] * c[2] + b[1] * c[3];
               store_vector4( inst, machine, result );
            }
            break;
         default:
            _mesa_problem(ctx, "Bad opcode in _mesa_exec_fragment_program");
            return GL_TRUE; /* return value doesn't matter */
      }
   }
   return GL_TRUE;
}



void
_swrast_exec_nv_fragment_program( GLcontext *ctx, struct sw_span *span )
{
   GLuint i;

   for (i = 0; i < span->end; i++) {
      if (span->array->mask[i]) {
         GLfloat *wpos = ctx->FragmentProgram.Machine.Registers[0];
         GLfloat *col0 = ctx->FragmentProgram.Machine.Registers[1];
         GLfloat *col1 = ctx->FragmentProgram.Machine.Registers[2];
         GLfloat *fogc = ctx->FragmentProgram.Machine.Registers[3];
         const GLfloat *colOut = ctx->FragmentProgram.Machine.Registers[FP_OUTPUT_REG_START];
         GLuint j;

         /* Clear temporary registers XXX use memzero() */
         _mesa_bzero(ctx->FragmentProgram.Machine.Registers +FP_TEMP_REG_START,
                     MAX_NV_FRAGMENT_PROGRAM_TEMPS * 4 * sizeof(GLfloat));

         /*
          * Load input registers - yes this is all very inefficient for now.
          */
         wpos[0] = span->x + i;
         wpos[1] = span->y + i;
         wpos[2] = (GLfloat) span->array->z[i] / ctx->DepthMaxF;
         wpos[3] = 1.0; /* XXX should be 1/w */

         col0[0] = CHAN_TO_FLOAT(span->array->rgba[i][RCOMP]);
         col0[1] = CHAN_TO_FLOAT(span->array->rgba[i][GCOMP]);
         col0[2] = CHAN_TO_FLOAT(span->array->rgba[i][BCOMP]);
         col0[3] = CHAN_TO_FLOAT(span->array->rgba[i][ACOMP]);

         col1[0] = CHAN_TO_FLOAT(span->array->spec[i][RCOMP]);
         col1[1] = CHAN_TO_FLOAT(span->array->spec[i][GCOMP]);
         col1[2] = CHAN_TO_FLOAT(span->array->spec[i][BCOMP]);
         col1[3] = CHAN_TO_FLOAT(span->array->spec[i][ACOMP]);

         fogc[0] = span->array->fog[i];
         fogc[1] = 0.0F;
         fogc[2] = 0.0F;
         fogc[3] = 0.0F;

         for (j = 0; j < ctx->Const.MaxTextureCoordUnits; j++) {
            if (ctx->Texture.Unit[j]._ReallyEnabled) {
               COPY_4V(ctx->FragmentProgram.Machine.Registers[4 + j],
                       span->array->texcoords[j][i]);
            }
            else {
               COPY_4V(ctx->FragmentProgram.Machine.Registers[4 + j],
                       ctx->Current.Attrib[VERT_ATTRIB_TEX0 + j]);
            }
         }

         if (!execute_program(ctx, ctx->FragmentProgram.Current))
            span->array->mask[i] = GL_FALSE;  /* killed fragment */

         /* Store output registers */
         UNCLAMPED_FLOAT_TO_CHAN(span->array->rgba[i][RCOMP], colOut[0]);
         UNCLAMPED_FLOAT_TO_CHAN(span->array->rgba[i][GCOMP], colOut[1]);
         UNCLAMPED_FLOAT_TO_CHAN(span->array->rgba[i][BCOMP], colOut[2]);
         UNCLAMPED_FLOAT_TO_CHAN(span->array->rgba[i][ACOMP], colOut[3]);
         /* depth value */
         if (ctx->FragmentProgram.Current->OutputsWritten & 2)
            span->array->z[i] = IROUND(ctx->FragmentProgram.Machine.Registers[FP_OUTPUT_REG_START + 2][0] * ctx->DepthMaxF);
      }
   }
}

