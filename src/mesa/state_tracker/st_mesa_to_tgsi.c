/**************************************************************************
 * 
 * Copyright 2007-2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/*
 * \author
 * Michal Krol
 */

#include "pipe/p_compiler.h"
#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_build.h"
#include "tgsi/tgsi_util.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_sanity.h"
#include "st_mesa_to_tgsi.h"
#include "shader/prog_instruction.h"
#include "shader/prog_parameter.h"
#include "shader/prog_print.h"
#include "util/u_debug.h"

/*
 * Map mesa register file to TGSI register file.
 */
static GLuint
map_register_file(
   gl_register_file file,
   GLuint index,
   const GLuint immediateMapping[],
   GLboolean indirectAccess )
{
   switch( file ) {
   case PROGRAM_UNDEFINED:
      return TGSI_FILE_NULL;
   case PROGRAM_TEMPORARY:
      return TGSI_FILE_TEMPORARY;
   /*case PROGRAM_LOCAL_PARAM:*/
   /*case PROGRAM_ENV_PARAM:*/

      /* Because of the longstanding problem with mesa arb shaders
       * where constants, immediates and state variables are all
       * bundled together as PROGRAM_STATE_VAR, we can't tell from the
       * mesa register file whether this is a CONSTANT or an
       * IMMEDIATE, hence we need all the other information.
       */
   case PROGRAM_STATE_VAR:
   case PROGRAM_NAMED_PARAM:
   case PROGRAM_UNIFORM:
      if (!indirectAccess && immediateMapping && immediateMapping[index] != ~0)
         return TGSI_FILE_IMMEDIATE;
      else
	 return TGSI_FILE_CONSTANT;
   case PROGRAM_CONSTANT:
      if (indirectAccess)
         return TGSI_FILE_CONSTANT;
      assert(immediateMapping[index] != ~0);
      return TGSI_FILE_IMMEDIATE;
   case PROGRAM_INPUT:
      return TGSI_FILE_INPUT;
   case PROGRAM_OUTPUT:
      return TGSI_FILE_OUTPUT;
   case PROGRAM_ADDRESS:
      return TGSI_FILE_ADDRESS;
   default:
      assert( 0 );
      return TGSI_FILE_NULL;
   }
}

/**
 * Map mesa register file index to TGSI index.
 * Take special care when processing input and output indices.
 * \param file  one of TGSI_FILE_x
 * \param index  the mesa register file index
 * \param inputMapping  maps Mesa input indexes to TGSI input indexes
 * \param outputMapping  maps Mesa output indexes to TGSI output indexes
 */
static GLuint
map_register_file_index(
   GLuint procType,
   GLuint file,
   GLuint index,
   GLuint *swizzle,
   const GLuint inputMapping[],
   const GLuint outputMapping[],
   const GLuint immediateMapping[],
   GLboolean indirectAccess )
{
   switch( file ) {
   case TGSI_FILE_INPUT:
      if (procType == TGSI_PROCESSOR_FRAGMENT &&
          index == FRAG_ATTRIB_FOGC) {
         if (GET_SWZ(*swizzle, 0) == SWIZZLE_X) {
            /* do nothing we're, ok */
         } else if (GET_SWZ(*swizzle, 0) == SWIZZLE_Y) {
            /* replace the swizzle with xxxx */
            *swizzle = MAKE_SWIZZLE4(SWIZZLE_X,
                                     SWIZZLE_X,
                                     SWIZZLE_X,
                                     SWIZZLE_X);
            /* register after fog */
            return inputMapping[index] + 1;
         } else {
            *swizzle = MAKE_SWIZZLE4(SWIZZLE_Z,
                                     SWIZZLE_W,
                                     SWIZZLE_Z,
                                     SWIZZLE_W);
            /* register after frontface */
            return inputMapping[index] + 2;
         }
      }
      /* inputs are mapped according to the user-defined map */
      return inputMapping[index];

   case TGSI_FILE_OUTPUT:
      return outputMapping[index];

   case TGSI_FILE_IMMEDIATE:
      if (indirectAccess)
         return index;
      assert(immediateMapping[index] != ~0);
      return immediateMapping[index];

   default:
      return index;
   }
}

/*
 * Map mesa texture target to TGSI texture target.
 */
static GLuint
map_texture_target(
    GLuint textarget,
    GLboolean shadow )
{
#if 1
   /* XXX remove this line after we've checked that the rest of gallium
    * can handle the TGSI_TEXTURE_SHADOWx tokens.
    */
   shadow = GL_FALSE;
#endif
   switch( textarget ) {
   case TEXTURE_1D_INDEX:
      if (shadow)
         return TGSI_TEXTURE_SHADOW1D;
      else
         return TGSI_TEXTURE_1D;
   case TEXTURE_2D_INDEX:
      if (shadow)
         return TGSI_TEXTURE_SHADOW2D;
      else
         return TGSI_TEXTURE_2D;
   case TEXTURE_3D_INDEX:
      return TGSI_TEXTURE_3D;
   case TEXTURE_CUBE_INDEX:
      return TGSI_TEXTURE_CUBE;
   case TEXTURE_RECT_INDEX:
      if (shadow)
         return TGSI_TEXTURE_SHADOWRECT;
      else
         return TGSI_TEXTURE_RECT;
   default:
      assert( 0 );
   }

   return TGSI_TEXTURE_1D;
}

static GLuint
convert_sat(
   GLuint sat )
{
   switch( sat ) {
   case SATURATE_OFF:
      return TGSI_SAT_NONE;
   case SATURATE_ZERO_ONE:
      return TGSI_SAT_ZERO_ONE;
   case SATURATE_PLUS_MINUS_ONE:
      return TGSI_SAT_MINUS_PLUS_ONE;
   default:
      assert( 0 );
      return TGSI_SAT_NONE;
   }
}

static GLuint
convert_writemask(
   GLuint writemask )
{
   assert( WRITEMASK_X == TGSI_WRITEMASK_X );
   assert( WRITEMASK_Y == TGSI_WRITEMASK_Y );
   assert( WRITEMASK_Z == TGSI_WRITEMASK_Z );
   assert( WRITEMASK_W == TGSI_WRITEMASK_W );
   assert( (writemask & ~TGSI_WRITEMASK_XYZW) == 0 );

   return writemask;
}

static struct tgsi_full_immediate
make_immediate(const float *value, uint size)
{
   struct tgsi_full_immediate imm;

   imm = tgsi_default_full_immediate();
   imm.Immediate.NrTokens += size;
   imm.Immediate.DataType = TGSI_IMM_FLOAT32;
   imm.u.Pointer = value;
   return imm;
}

static void
compile_instruction(
   const struct prog_instruction *inst,
   struct tgsi_full_instruction *fullinst,
   const GLuint inputMapping[],
   const GLuint outputMapping[],
   const GLuint immediateMapping[],
   GLboolean indirectAccess,
   GLuint preamble_size,
   GLuint procType,
   GLboolean *insideSubroutine,
   GLint wposTemp)
{
   GLuint i;
   struct tgsi_full_dst_register *fulldst;
   struct tgsi_full_src_register *fullsrc;

   *fullinst = tgsi_default_full_instruction();

   fullinst->Instruction.Saturate = convert_sat( inst->SaturateMode );
   fullinst->Instruction.NumDstRegs = _mesa_num_inst_dst_regs( inst->Opcode );
   fullinst->Instruction.NumSrcRegs = _mesa_num_inst_src_regs( inst->Opcode );

   fulldst = &fullinst->FullDstRegisters[0];
   fulldst->DstRegister.File = map_register_file( inst->DstReg.File, 0, NULL, GL_FALSE );
   fulldst->DstRegister.Index = map_register_file_index(
      procType,
      fulldst->DstRegister.File,
      inst->DstReg.Index,
      NULL,
      inputMapping,
      outputMapping,
      NULL,
      GL_FALSE );
   fulldst->DstRegister.WriteMask = convert_writemask( inst->DstReg.WriteMask );
   if (inst->DstReg.RelAddr) {
      fulldst->DstRegister.Indirect = 1;
      fulldst->DstRegisterInd.File = TGSI_FILE_ADDRESS;
      fulldst->DstRegisterInd.Index = 0;
   }

   for (i = 0; i < fullinst->Instruction.NumSrcRegs; i++) {
      GLuint j;
      GLuint swizzle = inst->SrcReg[i].Swizzle;

      fullsrc = &fullinst->FullSrcRegisters[i];

      if (procType == TGSI_PROCESSOR_FRAGMENT &&
          inst->SrcReg[i].File == PROGRAM_INPUT &&
          inst->SrcReg[i].Index == FRAG_ATTRIB_WPOS) {
         /* special case of INPUT[WPOS] */
         fullsrc->SrcRegister.File = TGSI_FILE_TEMPORARY;
         fullsrc->SrcRegister.Index = wposTemp;
      }
      else {
         /* any other src register */
         fullsrc->SrcRegister.File = map_register_file(
            inst->SrcReg[i].File,
            inst->SrcReg[i].Index,
            immediateMapping,
            indirectAccess );
         fullsrc->SrcRegister.Index = map_register_file_index(
            procType,
            fullsrc->SrcRegister.File,
            inst->SrcReg[i].Index,
            &swizzle,
            inputMapping,
            outputMapping,
            immediateMapping,
            indirectAccess );
      }

      /* swizzle (ext swizzle also depends on negation) */
      {
         GLuint swz[4];
         GLboolean extended = (inst->SrcReg[i].Negate != NEGATE_NONE &&
                               inst->SrcReg[i].Negate != NEGATE_XYZW);
         for( j = 0; j < 4; j++ ) {
            swz[j] = GET_SWZ( swizzle, j );
            if (swz[j] > SWIZZLE_W)
               extended = GL_TRUE;
         }
         if (extended) {
            for (j = 0; j < 4; j++) {
               tgsi_util_set_src_register_extswizzle(&fullsrc->SrcRegisterExtSwz,
                                                     swz[j], j);
            }
         }
         else {
            for (j = 0; j < 4; j++) {
               tgsi_util_set_src_register_swizzle(&fullsrc->SrcRegister,
                                                  swz[j], j);
            }
         }
      }

      if( inst->SrcReg[i].Negate == NEGATE_XYZW ) {
         fullsrc->SrcRegister.Negate = 1;
      }
      else if( inst->SrcReg[i].Negate != NEGATE_NONE ) {
         if( inst->SrcReg[i].Negate & NEGATE_X ) {
            fullsrc->SrcRegisterExtSwz.NegateX = 1;
         }
         if( inst->SrcReg[i].Negate & NEGATE_Y ) {
            fullsrc->SrcRegisterExtSwz.NegateY = 1;
         }
         if( inst->SrcReg[i].Negate & NEGATE_Z ) {
            fullsrc->SrcRegisterExtSwz.NegateZ = 1;
         }
         if( inst->SrcReg[i].Negate & NEGATE_W ) {
            fullsrc->SrcRegisterExtSwz.NegateW = 1;
         }
      }

      if( inst->SrcReg[i].Abs ) {
         fullsrc->SrcRegisterExtMod.Absolute = 1;
      }

      if( inst->SrcReg[i].RelAddr ) {
         fullsrc->SrcRegister.Indirect = 1;

         fullsrc->SrcRegisterInd.File = TGSI_FILE_ADDRESS;
         fullsrc->SrcRegisterInd.Index = 0;
      }
   }

   switch( inst->Opcode ) {
   case OPCODE_ARL:
      fullinst->Instruction.Opcode = TGSI_OPCODE_ARL;
      break;
   case OPCODE_ABS:
      fullinst->Instruction.Opcode = TGSI_OPCODE_ABS;
      break;
   case OPCODE_ADD:
      fullinst->Instruction.Opcode = TGSI_OPCODE_ADD;
      break;
   case OPCODE_BGNLOOP:
      fullinst->Instruction.Opcode = TGSI_OPCODE_BGNLOOP2;
      fullinst->InstructionExtLabel.Label = inst->BranchTarget + preamble_size;
      break;
   case OPCODE_BGNSUB:
      fullinst->Instruction.Opcode = TGSI_OPCODE_BGNSUB;
      *insideSubroutine = GL_TRUE;
      break;
   case OPCODE_BRA:
      fullinst->Instruction.Opcode = TGSI_OPCODE_BRA;
      break;
   case OPCODE_BRK:
      fullinst->Instruction.Opcode = TGSI_OPCODE_BRK;
      break;
   case OPCODE_CAL:
      fullinst->Instruction.Opcode = TGSI_OPCODE_CAL;
      fullinst->InstructionExtLabel.Label = inst->BranchTarget + preamble_size;
      break;
   case OPCODE_CMP:
      fullinst->Instruction.Opcode = TGSI_OPCODE_CMP;
      break;
   case OPCODE_CONT:
      fullinst->Instruction.Opcode = TGSI_OPCODE_CONT;
      break;
   case OPCODE_COS:
      fullinst->Instruction.Opcode = TGSI_OPCODE_COS;
      break;
   case OPCODE_DDX:
      fullinst->Instruction.Opcode = TGSI_OPCODE_DDX;
      break;
   case OPCODE_DDY:
      fullinst->Instruction.Opcode = TGSI_OPCODE_DDY;
      break;
   case OPCODE_DP2:
      fullinst->Instruction.Opcode = TGSI_OPCODE_DP2;
      break;
   case OPCODE_DP2A:
      fullinst->Instruction.Opcode = TGSI_OPCODE_DP2A;
      break;
   case OPCODE_DP3:
      fullinst->Instruction.Opcode = TGSI_OPCODE_DP3;
      break;
   case OPCODE_DP4:
      fullinst->Instruction.Opcode = TGSI_OPCODE_DP4;
      break;
   case OPCODE_DPH:
      fullinst->Instruction.Opcode = TGSI_OPCODE_DPH;
      break;
   case OPCODE_DST:
      fullinst->Instruction.Opcode = TGSI_OPCODE_DST;
      break;
   case OPCODE_ELSE:
      fullinst->Instruction.Opcode = TGSI_OPCODE_ELSE;
      fullinst->InstructionExtLabel.Label = inst->BranchTarget + preamble_size;
      break;
   case OPCODE_ENDIF:
      fullinst->Instruction.Opcode = TGSI_OPCODE_ENDIF;
      break;
   case OPCODE_ENDLOOP:
      fullinst->Instruction.Opcode = TGSI_OPCODE_ENDLOOP2;
      fullinst->InstructionExtLabel.Label = inst->BranchTarget + preamble_size;
      break;
   case OPCODE_ENDSUB:
      fullinst->Instruction.Opcode = TGSI_OPCODE_ENDSUB;
      *insideSubroutine = GL_FALSE;
      break;
   case OPCODE_EX2:
      fullinst->Instruction.Opcode = TGSI_OPCODE_EX2;
      break;
   case OPCODE_EXP:
      fullinst->Instruction.Opcode = TGSI_OPCODE_EXP;
      break;
   case OPCODE_FLR:
      fullinst->Instruction.Opcode = TGSI_OPCODE_FLR;
      break;
   case OPCODE_FRC:
      fullinst->Instruction.Opcode = TGSI_OPCODE_FRC;
      break;
   case OPCODE_IF:
      fullinst->Instruction.Opcode = TGSI_OPCODE_IF;
      fullinst->InstructionExtLabel.Label = inst->BranchTarget + preamble_size;
      break;
   case OPCODE_TRUNC:
      fullinst->Instruction.Opcode = TGSI_OPCODE_TRUNC;
      break;
   case OPCODE_KIL:
      /* conditional */
      fullinst->Instruction.Opcode = TGSI_OPCODE_KIL;
      break;
   case OPCODE_KIL_NV:
      /* predicated */
      assert(inst->DstReg.CondMask == COND_TR);
      fullinst->Instruction.Opcode = TGSI_OPCODE_KILP;
      break;
   case OPCODE_LG2:
      fullinst->Instruction.Opcode = TGSI_OPCODE_LG2;
      break;
   case OPCODE_LOG:
      fullinst->Instruction.Opcode = TGSI_OPCODE_LOG;
      break;
   case OPCODE_LIT:
      fullinst->Instruction.Opcode = TGSI_OPCODE_LIT;
      break;
   case OPCODE_LRP:
      fullinst->Instruction.Opcode = TGSI_OPCODE_LRP;
      break;
   case OPCODE_MAD:
      fullinst->Instruction.Opcode = TGSI_OPCODE_MAD;
      break;
   case OPCODE_MAX:
      fullinst->Instruction.Opcode = TGSI_OPCODE_MAX;
      break;
   case OPCODE_MIN:
      fullinst->Instruction.Opcode = TGSI_OPCODE_MIN;
      break;
   case OPCODE_MOV:
      fullinst->Instruction.Opcode = TGSI_OPCODE_MOV;
      break;
   case OPCODE_MUL:
      fullinst->Instruction.Opcode = TGSI_OPCODE_MUL;
      break;
   case OPCODE_NOISE1:
      fullinst->Instruction.Opcode = TGSI_OPCODE_NOISE1;
      break;
   case OPCODE_NOISE2:
      fullinst->Instruction.Opcode = TGSI_OPCODE_NOISE2;
      break;
   case OPCODE_NOISE3:
      fullinst->Instruction.Opcode = TGSI_OPCODE_NOISE3;
      break;
   case OPCODE_NOISE4:
      fullinst->Instruction.Opcode = TGSI_OPCODE_NOISE4;
      break;
   case OPCODE_NOP:
      fullinst->Instruction.Opcode = TGSI_OPCODE_NOP;
      break;
   case OPCODE_NRM3:
      fullinst->Instruction.Opcode = TGSI_OPCODE_NRM;
      break;
   case OPCODE_NRM4:
      fullinst->Instruction.Opcode = TGSI_OPCODE_NRM4;
      break;
   case OPCODE_POW:
      fullinst->Instruction.Opcode = TGSI_OPCODE_POW;
      break;
   case OPCODE_RCP:
      fullinst->Instruction.Opcode = TGSI_OPCODE_RCP;
      break;
   case OPCODE_RET:
      /* If RET is used inside main (not a real subroutine) we may want
       * to execute END instead of RET.  TBD...
       */
      if (1 /*  *insideSubroutine */) {
         fullinst->Instruction.Opcode = TGSI_OPCODE_RET;
      }
      else {
         /* inside main() pseudo-function */
         fullinst->Instruction.Opcode = TGSI_OPCODE_END;
      }
      break;
   case OPCODE_RSQ:
      fullinst->Instruction.Opcode = TGSI_OPCODE_RSQ;
      break;
   case OPCODE_SCS:
      fullinst->Instruction.Opcode = TGSI_OPCODE_SCS;
      fulldst->DstRegister.WriteMask &= TGSI_WRITEMASK_XY;
      break;
   case OPCODE_SEQ:
      fullinst->Instruction.Opcode = TGSI_OPCODE_SEQ;
      break;
   case OPCODE_SGE:
      fullinst->Instruction.Opcode = TGSI_OPCODE_SGE;
      break;
   case OPCODE_SGT:
      fullinst->Instruction.Opcode = TGSI_OPCODE_SGT;
      break;
   case OPCODE_SIN:
      fullinst->Instruction.Opcode = TGSI_OPCODE_SIN;
      break;
   case OPCODE_SLE:
      fullinst->Instruction.Opcode = TGSI_OPCODE_SLE;
      break;
   case OPCODE_SLT:
      fullinst->Instruction.Opcode = TGSI_OPCODE_SLT;
      break;
   case OPCODE_SNE:
      fullinst->Instruction.Opcode = TGSI_OPCODE_SNE;
      break;
   case OPCODE_SSG:
      fullinst->Instruction.Opcode = TGSI_OPCODE_SSG;
      break;
   case OPCODE_SUB:
      fullinst->Instruction.Opcode = TGSI_OPCODE_SUB;
      break;
   case OPCODE_SWZ:
      fullinst->Instruction.Opcode = TGSI_OPCODE_SWZ;
      break;
   case OPCODE_TEX:
      /* ordinary texture lookup */
      fullinst->Instruction.Opcode = TGSI_OPCODE_TEX;
      fullinst->Instruction.NumSrcRegs = 2;
      fullinst->InstructionExtTexture.Texture =
         map_texture_target( inst->TexSrcTarget, inst->TexShadow );
      fullinst->FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_SAMPLER;
      fullinst->FullSrcRegisters[1].SrcRegister.Index = inst->TexSrcUnit;
      break;
   case OPCODE_TXB:
      /* texture lookup with LOD bias */
      fullinst->Instruction.Opcode = TGSI_OPCODE_TXB;
      fullinst->Instruction.NumSrcRegs = 2;
      fullinst->InstructionExtTexture.Texture =
         map_texture_target( inst->TexSrcTarget, inst->TexShadow );
      fullinst->FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_SAMPLER;
      fullinst->FullSrcRegisters[1].SrcRegister.Index = inst->TexSrcUnit;
      break;
   case OPCODE_TXD:
      /* texture lookup with explicit partial derivatives */
      fullinst->Instruction.Opcode = TGSI_OPCODE_TXD;
      fullinst->Instruction.NumSrcRegs = 4;
      fullinst->InstructionExtTexture.Texture =
         map_texture_target( inst->TexSrcTarget, inst->TexShadow );
      /* src[0] = coord, src[1] = d[strq]/dx, src[2] = d[strq]/dy */
      fullinst->FullSrcRegisters[3].SrcRegister.File = TGSI_FILE_SAMPLER;
      fullinst->FullSrcRegisters[3].SrcRegister.Index = inst->TexSrcUnit;
      break;
   case OPCODE_TXL:
      /* texture lookup with explicit LOD */
      fullinst->Instruction.Opcode = TGSI_OPCODE_TXL;
      fullinst->Instruction.NumSrcRegs = 2;
      fullinst->InstructionExtTexture.Texture =
         map_texture_target( inst->TexSrcTarget, inst->TexShadow );
      fullinst->FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_SAMPLER;
      fullinst->FullSrcRegisters[1].SrcRegister.Index = inst->TexSrcUnit;
      break;
   case OPCODE_TXP:
      /* texture lookup with divide by Q component */
      /* convert to TEX w/ special flag for division */
      fullinst->Instruction.Opcode = TGSI_OPCODE_TXP;
      fullinst->Instruction.NumSrcRegs = 2;
      fullinst->InstructionExtTexture.Texture =
         map_texture_target( inst->TexSrcTarget, inst->TexShadow );
      fullinst->FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_SAMPLER;
      fullinst->FullSrcRegisters[1].SrcRegister.Index = inst->TexSrcUnit;
      break;
   case OPCODE_XPD:
      fullinst->Instruction.Opcode = TGSI_OPCODE_XPD;
      fulldst->DstRegister.WriteMask &= TGSI_WRITEMASK_XYZ;
      break;
   case OPCODE_END:
      fullinst->Instruction.Opcode = TGSI_OPCODE_END;
      break;
   default:
      assert( 0 );
   }
}

/**
 * \param usage_mask  bitfield of TGSI_WRITEMASK_{XYZW} tokens
 */
static struct tgsi_full_declaration
make_input_decl(
   GLuint index,
   GLboolean interpolate_info,
   GLuint interpolate,
   GLuint usage_mask,
   GLboolean semantic_info,
   GLuint semantic_name,
   GLbitfield semantic_index,
   GLbitfield input_flags)
{
   struct tgsi_full_declaration decl;

   assert(semantic_name < TGSI_SEMANTIC_COUNT);

   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_INPUT;
   decl.Declaration.UsageMask = usage_mask;
   decl.Declaration.Semantic = semantic_info;
   decl.DeclarationRange.First = index;
   decl.DeclarationRange.Last = index;
   if (semantic_info) {
      decl.Semantic.SemanticName = semantic_name;
      decl.Semantic.SemanticIndex = semantic_index;
   }
   if (interpolate_info) {
      decl.Declaration.Interpolate = interpolate;
   }
   if (input_flags & PROG_PARAM_BIT_CENTROID)
      decl.Declaration.Centroid = 1;
   if (input_flags & PROG_PARAM_BIT_INVARIANT)
      decl.Declaration.Invariant = 1;

   return decl;
}

/**
 * \param usage_mask  bitfield of TGSI_WRITEMASK_{XYZW} tokens
 */
static struct tgsi_full_declaration
make_output_decl(
   GLuint index,
   GLuint semantic_name,
   GLuint semantic_index,
   GLuint usage_mask,
   GLbitfield output_flags)
{
   struct tgsi_full_declaration decl;

   assert(semantic_name < TGSI_SEMANTIC_COUNT);

   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_OUTPUT;
   decl.Declaration.UsageMask = usage_mask;
   decl.Declaration.Semantic = 1;
   decl.DeclarationRange.First = index;
   decl.DeclarationRange.Last = index;
   decl.Semantic.SemanticName = semantic_name;
   decl.Semantic.SemanticIndex = semantic_index;
   if (output_flags & PROG_PARAM_BIT_CENTROID)
      decl.Declaration.Centroid = 1;
   if (output_flags & PROG_PARAM_BIT_INVARIANT)
      decl.Declaration.Invariant = 1;

   return decl;
}


static struct tgsi_full_declaration
make_temp_decl(
   GLuint start_index,
   GLuint end_index )
{
   struct tgsi_full_declaration decl;
   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_TEMPORARY;
   decl.DeclarationRange.First = start_index;
   decl.DeclarationRange.Last = end_index;
   return decl;
}

static struct tgsi_full_declaration
make_addr_decl(
   GLuint start_index,
   GLuint end_index )
{
   struct tgsi_full_declaration decl;

   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_ADDRESS;
   decl.DeclarationRange.First = start_index;
   decl.DeclarationRange.Last = end_index;
   return decl;
}

static struct tgsi_full_declaration
make_sampler_decl(GLuint index)
{
   struct tgsi_full_declaration decl;
   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_SAMPLER;
   decl.DeclarationRange.First = index;
   decl.DeclarationRange.Last = index;
   return decl;
}

/** Reference into a constant buffer */
static struct tgsi_full_declaration
make_constant_decl(GLuint first, GLuint last)
{
   struct tgsi_full_declaration decl;
   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_CONSTANT;
   decl.DeclarationRange.First = first;
   decl.DeclarationRange.Last = last;
   return decl;
}



/**
 * Find the temporaries which are used in the given program.
 */
static void
find_temporaries(const struct gl_program *program,
                 GLboolean tempsUsed[MAX_PROGRAM_TEMPS])
{
   GLuint i, j;

   for (i = 0; i < MAX_PROGRAM_TEMPS; i++)
      tempsUsed[i] = GL_FALSE;

   for (i = 0; i < program->NumInstructions; i++) {
      const struct prog_instruction *inst = program->Instructions + i;
      const GLuint n = _mesa_num_inst_src_regs( inst->Opcode );
      for (j = 0; j < n; j++) {
         if (inst->SrcReg[j].File == PROGRAM_TEMPORARY)
            tempsUsed[inst->SrcReg[j].Index] = GL_TRUE;
         if (inst->DstReg.File == PROGRAM_TEMPORARY)
            tempsUsed[inst->DstReg.Index] = GL_TRUE;
      }
   }
}


/**
 * Find an unused temporary in the tempsUsed array.
 */
static int
find_free_temporary(GLboolean tempsUsed[MAX_PROGRAM_TEMPS])
{
   int i;
   for (i = 0; i < MAX_PROGRAM_TEMPS; i++) {
      if (!tempsUsed[i]) {
         tempsUsed[i] = GL_TRUE;
         return i;
      }
   }
   return -1;
}


/** helper for building simple TGSI instruction, one src register */
static void
build_tgsi_instruction1(struct tgsi_full_instruction *inst,
                        int opcode,
                        int dstFile, int dstIndex, int writemask,
                        int srcFile1, int srcIndex1)
{
   *inst = tgsi_default_full_instruction();

   inst->Instruction.Opcode = opcode;

   inst->Instruction.NumDstRegs = 1;
   inst->FullDstRegisters[0].DstRegister.File = dstFile;
   inst->FullDstRegisters[0].DstRegister.Index = dstIndex;
   inst->FullDstRegisters[0].DstRegister.WriteMask = writemask;

   inst->Instruction.NumSrcRegs = 1;
   inst->FullSrcRegisters[0].SrcRegister.File = srcFile1;
   inst->FullSrcRegisters[0].SrcRegister.Index = srcIndex1;
}


/** helper for building simple TGSI instruction, two src registers */
static void
build_tgsi_instruction2(struct tgsi_full_instruction *inst,
                        int opcode,
                        int dstFile, int dstIndex, int writemask,
                        int srcFile1, int srcIndex1,
                        int srcFile2, int srcIndex2)
{
   *inst = tgsi_default_full_instruction();

   inst->Instruction.Opcode = opcode;

   inst->Instruction.NumDstRegs = 1;
   inst->FullDstRegisters[0].DstRegister.File = dstFile;
   inst->FullDstRegisters[0].DstRegister.Index = dstIndex;
   inst->FullDstRegisters[0].DstRegister.WriteMask = writemask;

   inst->Instruction.NumSrcRegs = 2;
   inst->FullSrcRegisters[0].SrcRegister.File = srcFile1;
   inst->FullSrcRegisters[0].SrcRegister.Index = srcIndex1;
   inst->FullSrcRegisters[1].SrcRegister.File = srcFile2;
   inst->FullSrcRegisters[1].SrcRegister.Index = srcIndex2;
}



/**
 * Emit the TGSI instructions for inverting the WPOS y coordinate.
 */
static int
emit_inverted_wpos(struct tgsi_token *tokens,
                   int wpos_temp,
                   int winsize_const,
                   int wpos_input,
                   struct tgsi_header *header, int maxTokens)
{
   struct tgsi_full_instruction fullinst;
   int ti = 0;

   /* MOV wpos_temp.xzw, input[wpos]; */
   build_tgsi_instruction1(&fullinst,
                           TGSI_OPCODE_MOV,
                           TGSI_FILE_TEMPORARY, wpos_temp, WRITEMASK_XZW,
                           TGSI_FILE_INPUT, 0);

   ti += tgsi_build_full_instruction(&fullinst,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti);

   /* SUB wpos_temp.y, const[winsize_const] - input[wpos_input]; */
   build_tgsi_instruction2(&fullinst,
                           TGSI_OPCODE_SUB,
                           TGSI_FILE_TEMPORARY, wpos_temp, WRITEMASK_Y,
                           TGSI_FILE_CONSTANT, winsize_const,
                           TGSI_FILE_INPUT, wpos_input);

   ti += tgsi_build_full_instruction(&fullinst,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti);

   return ti;
}




/**
 * Translate Mesa program to TGSI format.
 * \param program  the program to translate
 * \param numInputs  number of input registers used
 * \param inputMapping  maps Mesa fragment program inputs to TGSI generic
 *                      input indexes
 * \param inputSemanticName  the TGSI_SEMANTIC flag for each input
 * \param inputSemanticIndex  the semantic index (ex: which texcoord) for each input
 * \param interpMode  the TGSI_INTERPOLATE_LINEAR/PERSP mode for each input

 * \param numOutputs  number of output registers used
 * \param outputMapping  maps Mesa fragment program outputs to TGSI
 *                       generic outputs
 * \param outputSemanticName  the TGSI_SEMANTIC flag for each output
 * \param outputSemanticIndex  the semantic index (ex: which texcoord) for each output
 * \param tokens  array to store translated tokens in
 * \param maxTokens  size of the tokens array
 *
 * \return number of tokens placed in 'tokens' buffer, or zero if error
 */
GLuint
st_translate_mesa_program(
   GLcontext *ctx,
   uint procType,
   const struct gl_program *program,
   GLuint numInputs,
   const GLuint inputMapping[],
   const ubyte inputSemanticName[],
   const ubyte inputSemanticIndex[],
   const GLuint interpMode[],
   const GLbitfield inputFlags[],
   GLuint numOutputs,
   const GLuint outputMapping[],
   const ubyte outputSemanticName[],
   const ubyte outputSemanticIndex[],
   const GLbitfield outputFlags[],
   struct tgsi_token *tokens,
   GLuint maxTokens )
{
   GLuint i;
   GLuint ti;  /* token index */
   struct tgsi_header *header;
   struct tgsi_processor *processor;
   GLuint preamble_size = 0;
   GLuint immediates[1000];
   GLuint numImmediates = 0;
   GLboolean insideSubroutine = GL_FALSE;
   GLboolean indirectAccess = GL_FALSE;
   GLboolean tempsUsed[MAX_PROGRAM_TEMPS + 1];
   GLint wposTemp = -1, winHeightConst = -1;

   assert(procType == TGSI_PROCESSOR_FRAGMENT ||
          procType == TGSI_PROCESSOR_VERTEX);

   find_temporaries(program, tempsUsed);

   if (procType == TGSI_PROCESSOR_FRAGMENT) {
      if (program->InputsRead & FRAG_BIT_WPOS) {
         /* Fragment program uses fragment position input.
          * Need to replace instances of INPUT[WPOS] with temp T
          * where T = INPUT[WPOS] by y is inverted.
          */
         static const gl_state_index winSizeState[STATE_LENGTH]
            = { STATE_INTERNAL, STATE_FB_SIZE, 0, 0, 0 };
         winHeightConst = _mesa_add_state_reference(program->Parameters,
                                                    winSizeState);
         wposTemp = find_free_temporary(tempsUsed);
      }
   }


   *(struct tgsi_version *) &tokens[0] = tgsi_build_version();

   header = (struct tgsi_header *) &tokens[1];
   *header = tgsi_build_header();

   processor = (struct tgsi_processor *) &tokens[2];
   *processor = tgsi_build_processor( procType, header );

   ti = 3;

   /*
    * Declare input attributes.
    */
   if (procType == TGSI_PROCESSOR_FRAGMENT) {
      for (i = 0; i < numInputs; i++) {
         struct tgsi_full_declaration fulldecl;
         fulldecl = make_input_decl(i,
                                    GL_TRUE, interpMode[i],
                                    TGSI_WRITEMASK_XYZW,
                                    GL_TRUE, inputSemanticName[i],
                                    inputSemanticIndex[i],
                                    inputFlags[i]);
         ti += tgsi_build_full_declaration(&fulldecl,
                                           &tokens[ti],
                                           header,
                                           maxTokens - ti );
      }
   }
   else {
      /* vertex prog */
      /* XXX: this could probaby be merged with the clause above.
       * the only difference is the semantic tags.
       */
      for (i = 0; i < numInputs; i++) {
         struct tgsi_full_declaration fulldecl;
         fulldecl = make_input_decl(i,
                                    GL_FALSE, 0,
                                    TGSI_WRITEMASK_XYZW,
                                    GL_FALSE, 0, 0,
                                    inputFlags[i]);
         ti += tgsi_build_full_declaration(&fulldecl,
                                           &tokens[ti],
                                           header,
                                           maxTokens - ti );
      }
   }

   /*
    * Declare output attributes.
    */
   if (procType == TGSI_PROCESSOR_FRAGMENT) {
      for (i = 0; i < numOutputs; i++) {
         struct tgsi_full_declaration fulldecl;
         switch (outputSemanticName[i]) {
         case TGSI_SEMANTIC_POSITION:
            fulldecl = make_output_decl(i,
                                        TGSI_SEMANTIC_POSITION, /* Z / Depth */
                                        outputSemanticIndex[i],
                                        TGSI_WRITEMASK_Z,
                                        outputFlags[i]);
            break;
         case TGSI_SEMANTIC_COLOR:
            fulldecl = make_output_decl(i,
                                        TGSI_SEMANTIC_COLOR,
                                        outputSemanticIndex[i],
                                        TGSI_WRITEMASK_XYZW,
                                        outputFlags[i]);
            break;
         default:
            assert(0);
            return 0;
         }
         ti += tgsi_build_full_declaration(&fulldecl,
                                           &tokens[ti],
                                           header,
                                           maxTokens - ti );
      }
   }
   else {
      /* vertex prog */
      for (i = 0; i < numOutputs; i++) {
         struct tgsi_full_declaration fulldecl;
         fulldecl = make_output_decl(i,
                                     outputSemanticName[i],
                                     outputSemanticIndex[i],
                                     TGSI_WRITEMASK_XYZW,
                                     outputFlags[i]);
         ti += tgsi_build_full_declaration(&fulldecl,
                                           &tokens[ti],
                                           header,
                                           maxTokens - ti );
      }
   }

   /* temporary decls */
   {
      GLboolean inside_range = GL_FALSE;
      GLuint start_range = 0;

      tempsUsed[MAX_PROGRAM_TEMPS] = GL_FALSE;
      for (i = 0; i < MAX_PROGRAM_TEMPS + 1; i++) {
         if (tempsUsed[i] && !inside_range) {
            inside_range = GL_TRUE;
            start_range = i;
         }
         else if (!tempsUsed[i] && inside_range) {
            struct tgsi_full_declaration fulldecl;

            inside_range = GL_FALSE;
            fulldecl = make_temp_decl( start_range, i - 1 );
            ti += tgsi_build_full_declaration(
               &fulldecl,
               &tokens[ti],
               header,
               maxTokens - ti );
         }
      }
   }

   /* Declare address register.
   */
   if (program->NumAddressRegs > 0) {
      struct tgsi_full_declaration fulldecl;

      assert( program->NumAddressRegs == 1 );

      fulldecl = make_addr_decl( 0, 0 );
      ti += tgsi_build_full_declaration(
         &fulldecl,
         &tokens[ti],
         header,
         maxTokens - ti );

      indirectAccess = GL_TRUE;
   }

   /* immediates/literals */
   memset(immediates, ~0, sizeof(immediates));

   /* Emit immediates only when there is no address register in use.
    * FIXME: Be smarter and recognize param arrays -- indirect addressing is
    *        only valid within the referenced array.
    */
   if (program->Parameters && !indirectAccess) {
      for (i = 0; i < program->Parameters->NumParameters; i++) {
         if (program->Parameters->Parameters[i].Type == PROGRAM_CONSTANT) {
            struct tgsi_full_immediate fullimm;

            fullimm = make_immediate( program->Parameters->ParameterValues[i], 4 );
            ti += tgsi_build_full_immediate(
               &fullimm,
               &tokens[ti],
               header,
               maxTokens - ti );
            immediates[i] = numImmediates;
            numImmediates++;
         }
      }
   }

   /* constant buffer refs */
   if (program->Parameters) {
      GLint start = -1, end = -1;

      for (i = 0; i < program->Parameters->NumParameters; i++) {
         GLboolean emit = (i == program->Parameters->NumParameters - 1);
         GLboolean matches;

         switch (program->Parameters->Parameters[i].Type) {
         case PROGRAM_ENV_PARAM:
         case PROGRAM_STATE_VAR:
         case PROGRAM_NAMED_PARAM:
         case PROGRAM_UNIFORM:
            matches = GL_TRUE;
            break;
         case PROGRAM_CONSTANT:
            matches = indirectAccess;
            break;
         default:
            matches = GL_FALSE;
         }

         if (matches) {
            if (start == -1) {
               /* begin a sequence */
               start = i;
               end = i;
            }
            else {
               /* continue sequence */
               end = i;
            }
         }
         else {
            if (start != -1) {
               /* end of sequence */
               emit = GL_TRUE;
            }
         }

         if (emit && start >= 0) {
            struct tgsi_full_declaration fulldecl;

            fulldecl = make_constant_decl( start, end );
            ti += tgsi_build_full_declaration(
               &fulldecl,
               &tokens[ti],
               header,
               maxTokens - ti );
            start = end = -1;
         }
      }
   }

   /* texture samplers */
   for (i = 0; i < ctx->Const.MaxTextureImageUnits; i++) {
      if (program->SamplersUsed & (1 << i)) {
         struct tgsi_full_declaration fulldecl;

         fulldecl = make_sampler_decl( i );
         ti += tgsi_build_full_declaration(
            &fulldecl,
            &tokens[ti],
            header,
            maxTokens - ti );
      }
   }

   /* invert WPOS fragment input */
   if (wposTemp >= 0) {
      ti += emit_inverted_wpos(&tokens[ti], wposTemp, winHeightConst,
                               inputMapping[FRAG_ATTRIB_WPOS],
                               header, maxTokens - ti);
      preamble_size = 2; /* two instructions added */
   }

   for (i = 0; i < program->NumInstructions; i++) {
      struct tgsi_full_instruction fullinst;

      compile_instruction(
         &program->Instructions[i],
         &fullinst,
         inputMapping,
         outputMapping,
         immediates,
         indirectAccess,
         preamble_size,
         procType,
         &insideSubroutine,
         wposTemp);

      ti += tgsi_build_full_instruction(
         &fullinst,
         &tokens[ti],
         header,
         maxTokens - ti );
   }

#if DEBUG
   if(!tgsi_sanity_check(tokens)) {
      debug_printf("Due to sanity check failure(s) above the following shader program is invalid:\n");
      debug_printf("\nOriginal program:\n%s", program->String);
      debug_printf("\nMesa program:\n");
      _mesa_print_program(program);
      debug_printf("\nTGSI program:\n");
      tgsi_dump(tokens, 0);
      assert(0);
   }
#endif

   return ti;
}
