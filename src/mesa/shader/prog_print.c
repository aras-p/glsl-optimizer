/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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

/**
 * \file prog_print.c
 * Print vertex/fragment programs - for debugging.
 * \author Brian Paul
 */

#include "glheader.h"
#include "context.h"
#include "imports.h"
#include "prog_instruction.h"
#include "prog_parameter.h"
#include "prog_print.h"
#include "prog_statevars.h"


/**
 * Return string name for given program/register file.
 */
static const char *
program_file_string(enum register_file f)
{
   switch (f) {
   case PROGRAM_TEMPORARY:
      return "TEMP";
   case PROGRAM_LOCAL_PARAM:
      return "LOCAL";
   case PROGRAM_ENV_PARAM:
      return "ENV";
   case PROGRAM_STATE_VAR:
      return "STATE";
   case PROGRAM_INPUT:
      return "INPUT";
   case PROGRAM_OUTPUT:
      return "OUTPUT";
   case PROGRAM_NAMED_PARAM:
      return "NAMED";
   case PROGRAM_CONSTANT:
      return "CONST";
   case PROGRAM_UNIFORM:
      return "UNIFORM";
   case PROGRAM_VARYING:
      return "VARYING";
   case PROGRAM_WRITE_ONLY:
      return "WRITE_ONLY";
   case PROGRAM_ADDRESS:
      return "ADDR";
   default:
      return "Unknown program file!";
   }
}


/**
 * Return a string representation of the given swizzle word.
 * If extended is true, use extended (comma-separated) format.
 * \param swizzle  the swizzle field
 * \param negateBase  4-bit negation vector
 * \param extended  if true, also allow 0, 1 values
 */
static const char *
swizzle_string(GLuint swizzle, GLuint negateBase, GLboolean extended)
{
   static const char swz[] = "xyzw01";
   static char s[20];
   GLuint i = 0;

   if (!extended && swizzle == SWIZZLE_NOOP && negateBase == 0)
      return ""; /* no swizzle/negation */

   if (!extended)
      s[i++] = '.';

   if (negateBase & 0x1)
      s[i++] = '-';
   s[i++] = swz[GET_SWZ(swizzle, 0)];

   if (extended) {
      s[i++] = ',';
   }

   if (negateBase & 0x2)
      s[i++] = '-';
   s[i++] = swz[GET_SWZ(swizzle, 1)];

   if (extended) {
      s[i++] = ',';
   }

   if (negateBase & 0x4)
      s[i++] = '-';
   s[i++] = swz[GET_SWZ(swizzle, 2)];

   if (extended) {
      s[i++] = ',';
   }

   if (negateBase & 0x8)
      s[i++] = '-';
   s[i++] = swz[GET_SWZ(swizzle, 3)];

   s[i] = 0;
   return s;
}


static const char *
writemask_string(GLuint writeMask)
{
   static char s[10];
   GLuint i = 0;

   if (writeMask == WRITEMASK_XYZW)
      return "";

   s[i++] = '.';
   if (writeMask & WRITEMASK_X)
      s[i++] = 'x';
   if (writeMask & WRITEMASK_Y)
      s[i++] = 'y';
   if (writeMask & WRITEMASK_Z)
      s[i++] = 'z';
   if (writeMask & WRITEMASK_W)
      s[i++] = 'w';

   s[i] = 0;
   return s;
}


static const char *
condcode_string(GLuint condcode)
{
   switch (condcode) {
   case COND_GT:  return "GT";
   case COND_EQ:  return "EQ";
   case COND_LT:  return "LT";
   case COND_UN:  return "UN";
   case COND_GE:  return "GE";
   case COND_LE:  return "LE";
   case COND_NE:  return "NE";
   case COND_TR:  return "TR";
   case COND_FL:  return "FL";
   default: return "cond???";
   }
}


static void
print_dst_reg(const struct prog_dst_register *dstReg)
{
   _mesa_printf(" %s[%d]%s",
                program_file_string((enum register_file) dstReg->File),
                dstReg->Index,
                writemask_string(dstReg->WriteMask));
}

static void
print_src_reg(const struct prog_src_register *srcReg)
{
   _mesa_printf("%s[%d]%s",
                program_file_string((enum register_file) srcReg->File),
                srcReg->Index,
                swizzle_string(srcReg->Swizzle,
                               srcReg->NegateBase, GL_FALSE));
}

static void
print_comment(const struct prog_instruction *inst)
{
   if (inst->Comment)
      _mesa_printf(";  # %s\n", inst->Comment);
   else
      _mesa_printf(";\n");
}


void
_mesa_print_alu_instruction(const struct prog_instruction *inst,
			    const char *opcode_string, 
			    GLuint numRegs)
{
   GLuint j;

   _mesa_printf("%s", opcode_string);

   /* frag prog only */
   if (inst->SaturateMode == SATURATE_ZERO_ONE)
      _mesa_printf("_SAT");

   if (inst->DstReg.File != PROGRAM_UNDEFINED) {
      _mesa_printf(" %s[%d]%s",
		   program_file_string((enum register_file) inst->DstReg.File),
		   inst->DstReg.Index,
		   writemask_string(inst->DstReg.WriteMask));
   }
   else {
      _mesa_printf(" ???");
   }

   if (numRegs > 0)
      _mesa_printf(", ");

   for (j = 0; j < numRegs; j++) {
      print_src_reg(inst->SrcReg + j);
      if (j + 1 < numRegs)
	 _mesa_printf(", ");
   }

   print_comment(inst);
}


/**
 * Print a single vertex/fragment program instruction.
 */
void
_mesa_print_instruction(const struct prog_instruction *inst)
{
   switch (inst->Opcode) {
   case OPCODE_PRINT:
      _mesa_printf("PRINT '%s'", inst->Data);
      if (inst->SrcReg[0].File != PROGRAM_UNDEFINED) {
         _mesa_printf(", ");
         _mesa_printf("%s[%d]%s",
                      program_file_string((enum register_file) inst->SrcReg[0].File),
                      inst->SrcReg[0].Index,
                      swizzle_string(inst->SrcReg[0].Swizzle,
                                     inst->SrcReg[0].NegateBase, GL_FALSE));
      }
      if (inst->Comment)
         _mesa_printf("  # %s", inst->Comment);
      print_comment(inst);
      break;
   case OPCODE_SWZ:
      _mesa_printf("SWZ");
      if (inst->SaturateMode == SATURATE_ZERO_ONE)
         _mesa_printf("_SAT");
      print_dst_reg(&inst->DstReg);
      _mesa_printf("%s[%d], %s",
                   program_file_string((enum register_file) inst->SrcReg[0].File),
                   inst->SrcReg[0].Index,
                   swizzle_string(inst->SrcReg[0].Swizzle,
                                  inst->SrcReg[0].NegateBase, GL_TRUE));
      print_comment(inst);
      break;
   case OPCODE_TEX:
   case OPCODE_TXP:
   case OPCODE_TXB:
      _mesa_printf("%s", _mesa_opcode_string(inst->Opcode));
      if (inst->SaturateMode == SATURATE_ZERO_ONE)
         _mesa_printf("_SAT");
      _mesa_printf(" ");
      print_dst_reg(&inst->DstReg);
      _mesa_printf(", ");
      print_src_reg(&inst->SrcReg[0]);
      _mesa_printf(", texture[%d], ", inst->TexSrcUnit);
      switch (inst->TexSrcTarget) {
      case TEXTURE_1D_INDEX:   _mesa_printf("1D");    break;
      case TEXTURE_2D_INDEX:   _mesa_printf("2D");    break;
      case TEXTURE_3D_INDEX:   _mesa_printf("3D");    break;
      case TEXTURE_CUBE_INDEX: _mesa_printf("CUBE");  break;
      case TEXTURE_RECT_INDEX: _mesa_printf("RECT");  break;
      default:
         ;
      }
      print_comment(inst);
      break;
   case OPCODE_ARL:
      _mesa_printf("ARL addr.x, ");
      print_src_reg(&inst->SrcReg[0]);
      print_comment(inst);
      break;
   case OPCODE_BRA:
      _mesa_printf("BRA %u (%s.%s)",
                   inst->BranchTarget,
                   condcode_string(inst->DstReg.CondMask),
                   swizzle_string(inst->DstReg.CondSwizzle, 0, GL_FALSE));
      print_comment(inst);
      break;
   case OPCODE_CAL:
      _mesa_printf("CAL %u", inst->BranchTarget);
      print_comment(inst);
      break;
   case OPCODE_END:
      _mesa_printf("END");
      print_comment(inst);
      break;
   case OPCODE_NOP:
      _mesa_printf("NOP");
      print_comment(inst);
      break;
   /* XXX may need other special-case instructions */
   default:
      /* typical alu instruction */
      _mesa_print_alu_instruction(inst,
				  _mesa_opcode_string(inst->Opcode),
				  _mesa_num_inst_src_regs(inst->Opcode));
      break;
   }
}


/**
 * Print a vertx/fragment program to stdout.
 * XXX this function could be greatly improved.
 */
void
_mesa_print_program(const struct gl_program *prog)
{
   GLuint i;
   for (i = 0; i < prog->NumInstructions; i++) {
      _mesa_printf("%3d: ", i);
      _mesa_print_instruction(prog->Instructions + i);
   }
}


/**
 * Print all of a program's parameters.
 */
void
_mesa_print_program_parameters(GLcontext *ctx, const struct gl_program *prog)
{
   GLint i;

   _mesa_printf("InputsRead: 0x%x\n", prog->InputsRead);
   _mesa_printf("OutputsWritten: 0x%x\n", prog->OutputsWritten);
   _mesa_printf("NumInstructions=%d\n", prog->NumInstructions);
   _mesa_printf("NumTemporaries=%d\n", prog->NumTemporaries);
   _mesa_printf("NumParameters=%d\n", prog->NumParameters);
   _mesa_printf("NumAttributes=%d\n", prog->NumAttributes);
   _mesa_printf("NumAddressRegs=%d\n", prog->NumAddressRegs);
	
   _mesa_load_state_parameters(ctx, prog->Parameters);
			
#if 0
   _mesa_printf("Local Params:\n");
   for (i = 0; i < MAX_PROGRAM_LOCAL_PARAMS; i++){
      const GLfloat *p = prog->LocalParams[i];
      _mesa_printf("%2d: %f, %f, %f, %f\n", i, p[0], p[1], p[2], p[3]);
   }
#endif	

   for (i = 0; i < prog->Parameters->NumParameters; i++){
      struct gl_program_parameter *param = prog->Parameters->Parameters + i;
      const GLfloat *v = prog->Parameters->ParameterValues[i];
      _mesa_printf("param[%d] %s %s = {%.3f, %.3f, %.3f, %.3f};\n",
                   i,
                   program_file_string(prog->Parameters->Parameters[i].Type),
                   param->Name, v[0], v[1], v[2], v[3]);
   }
}
