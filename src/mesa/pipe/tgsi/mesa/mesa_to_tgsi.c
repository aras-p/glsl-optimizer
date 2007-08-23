#include "tgsi_platform.h"
#include "tgsi_mesa.h"
#include "pipe/tgsi/core/tgsi_attribs.h"
#include "pipe/tgsi/mesa/mesa_to_tgsi.h"

#define TGSI_DEBUG 1


/**
 * Convert a VERT_ATTRIB_x to a TGSI_ATTRIB_y
 */
uint
tgsi_mesa_translate_vertex_input(GLuint attrib)
{
   /* XXX these could be implemented with array lookups too.... */
   switch (attrib) {
   case VERT_ATTRIB_POS:
      return TGSI_ATTRIB_POS;
   case VERT_ATTRIB_WEIGHT:
      return TGSI_ATTRIB_WEIGHT;
   case VERT_ATTRIB_NORMAL:
      return TGSI_ATTRIB_NORMAL;
   case VERT_ATTRIB_COLOR0:
      return TGSI_ATTRIB_COLOR0;
   case VERT_ATTRIB_COLOR1:
      return TGSI_ATTRIB_COLOR1;
   case VERT_ATTRIB_FOG:
      return TGSI_ATTRIB_FOG;
   case VERT_ATTRIB_COLOR_INDEX:
      return TGSI_ATTRIB_COLOR_INDEX;
   case VERT_ATTRIB_EDGEFLAG:
      return TGSI_ATTRIB_EDGEFLAG;
   case VERT_ATTRIB_TEX0:
      return TGSI_ATTRIB_TEX0;
   case VERT_ATTRIB_TEX1:
      return TGSI_ATTRIB_TEX1;
   case VERT_ATTRIB_TEX2:
      return TGSI_ATTRIB_TEX2;
   case VERT_ATTRIB_TEX3:
      return TGSI_ATTRIB_TEX3;
   case VERT_ATTRIB_TEX4:
      return TGSI_ATTRIB_TEX4;
   case VERT_ATTRIB_TEX5:
      return TGSI_ATTRIB_TEX5;
   case VERT_ATTRIB_TEX6:
      return TGSI_ATTRIB_TEX6;
   case VERT_ATTRIB_TEX7:
      return TGSI_ATTRIB_TEX7;
   case VERT_ATTRIB_GENERIC0:
      return TGSI_ATTRIB_VAR0;
   case VERT_ATTRIB_GENERIC1:
      return TGSI_ATTRIB_VAR1;
   case VERT_ATTRIB_GENERIC2:
      return TGSI_ATTRIB_VAR2;
   case VERT_ATTRIB_GENERIC3:
      return TGSI_ATTRIB_VAR3;
   case VERT_ATTRIB_GENERIC4:
      return TGSI_ATTRIB_VAR4;
   case VERT_ATTRIB_GENERIC5:
      return TGSI_ATTRIB_VAR5;
   case VERT_ATTRIB_GENERIC6:
      return TGSI_ATTRIB_VAR6;
   case VERT_ATTRIB_GENERIC7:
      return TGSI_ATTRIB_VAR7;
   default:
      assert(0);
      return 0;
   }
}


/**
 * Convert VERT_RESULT_x to TGSI_ATTRIB_y
 */
uint
tgsi_mesa_translate_vertex_output(GLuint attrib)
{
   switch (attrib) {
   case VERT_RESULT_HPOS:
      return TGSI_ATTRIB_POS;
   case VERT_RESULT_COL0:
      return TGSI_ATTRIB_COLOR0;
   case VERT_RESULT_COL1:
      return TGSI_ATTRIB_COLOR1;
   case VERT_RESULT_FOGC:
      return TGSI_ATTRIB_FOG;
   case VERT_RESULT_TEX0:
      return TGSI_ATTRIB_TEX0;
   case VERT_RESULT_TEX1:
      return TGSI_ATTRIB_TEX1;
   case VERT_RESULT_TEX2:
      return TGSI_ATTRIB_TEX2;
   case VERT_RESULT_TEX3:
      return TGSI_ATTRIB_TEX3;
   case VERT_RESULT_TEX4:
      return TGSI_ATTRIB_TEX4;
   case VERT_RESULT_TEX5:
      return TGSI_ATTRIB_TEX5;
   case VERT_RESULT_TEX6:
      return TGSI_ATTRIB_TEX6;
   case VERT_RESULT_TEX7:
      return TGSI_ATTRIB_TEX7;
   case VERT_RESULT_PSIZ:
      return TGSI_ATTRIB_POINTSIZE;
   case VERT_RESULT_BFC0:
      return TGSI_ATTRIB_BFC0;
   case VERT_RESULT_BFC1:
      return TGSI_ATTRIB_BFC1;
   case VERT_RESULT_VAR0:
      return TGSI_ATTRIB_VAR0;
   case VERT_RESULT_VAR0 + 1:
      return TGSI_ATTRIB_VAR1;
   case VERT_RESULT_VAR0 + 2:
      return TGSI_ATTRIB_VAR2;
   case VERT_RESULT_VAR0 + 3:
      return TGSI_ATTRIB_VAR3;
   case VERT_RESULT_VAR0 + 4:
      return TGSI_ATTRIB_VAR4;
   case VERT_RESULT_VAR0 + 5:
      return TGSI_ATTRIB_VAR5;
   case VERT_RESULT_VAR0 + 6:
      return TGSI_ATTRIB_VAR6;
   case VERT_RESULT_VAR0 + 7:
      return TGSI_ATTRIB_VAR7;
   default:
      assert(0);
      return 0;
   }
}


/**
 * Convert a FRAG_ATTRIB_x to a TGSI_ATTRIB_y
 */
uint
tgsi_mesa_translate_fragment_input(GLuint attrib)
{
   switch (attrib) {
   case FRAG_ATTRIB_WPOS:
      return TGSI_ATTRIB_POS;
   case FRAG_ATTRIB_COL0:
      return TGSI_ATTRIB_COLOR0;
   case FRAG_ATTRIB_COL1:
      return TGSI_ATTRIB_COLOR1;
   case FRAG_ATTRIB_FOGC:
      return TGSI_ATTRIB_FOG;
   case FRAG_ATTRIB_TEX0:
      return TGSI_ATTRIB_TEX0;
   case FRAG_ATTRIB_TEX1:
      return TGSI_ATTRIB_TEX1;
   case FRAG_ATTRIB_TEX2:
      return TGSI_ATTRIB_TEX2;
   case FRAG_ATTRIB_TEX3:
      return TGSI_ATTRIB_TEX3;
   case FRAG_ATTRIB_TEX4:
      return TGSI_ATTRIB_TEX4;
   case FRAG_ATTRIB_TEX5:
      return TGSI_ATTRIB_TEX5;
   case FRAG_ATTRIB_TEX6:
      return TGSI_ATTRIB_TEX6;
   case FRAG_ATTRIB_TEX7:
      return TGSI_ATTRIB_TEX7;
   case FRAG_ATTRIB_VAR0:
      return TGSI_ATTRIB_VAR0;
   case FRAG_ATTRIB_VAR0 + 1:
      return TGSI_ATTRIB_VAR1;
   case FRAG_ATTRIB_VAR0 + 2:
      return TGSI_ATTRIB_VAR2;
   case FRAG_ATTRIB_VAR0 + 3:
      return TGSI_ATTRIB_VAR3;
   case FRAG_ATTRIB_VAR0 + 4:
      return TGSI_ATTRIB_VAR4;
   case FRAG_ATTRIB_VAR0 + 5:
      return TGSI_ATTRIB_VAR5;
   case FRAG_ATTRIB_VAR0 + 6:
      return TGSI_ATTRIB_VAR6;
   case FRAG_ATTRIB_VAR0 + 7:
      return TGSI_ATTRIB_VAR7;
   default:
      assert(0);
      return 0;
   }
}


/**
 * Convert FRAG_RESULT_x to TGSI_ATTRIB_y
 */
uint
tgsi_mesa_translate_fragment_output(GLuint attrib)
{
   switch (attrib) {
   case FRAG_RESULT_DEPR:
      return TGSI_ATTRIB_POS;
   case FRAG_RESULT_COLR:
      /* fall-through */
   case FRAG_RESULT_COLH:
      /* fall-through */
   case FRAG_RESULT_DATA0:
      return TGSI_ATTRIB_COLOR0;
   case FRAG_RESULT_DATA0 + 1:
      return TGSI_ATTRIB_COLOR0 + 1;
   case FRAG_RESULT_DATA0 + 2:
      return TGSI_ATTRIB_COLOR0 + 2;
   case FRAG_RESULT_DATA0 + 3:
      return TGSI_ATTRIB_COLOR0 + 3;
   default:
      assert(0);
      return 0;
   }
}


uint
tgsi_mesa_translate_vertex_input_mask(GLbitfield mask)
{
   uint tgsiMask = 0x0;
   uint i;
   for (i = 0; i < VERT_ATTRIB_MAX && mask; i++) {
      if (mask & (1 << i)) {
         tgsiMask |= 1 << tgsi_mesa_translate_vertex_input(i);
      }
      mask &= ~(1 << i);
   }
   return tgsiMask;
}


uint
tgsi_mesa_translate_vertex_output_mask(GLbitfield mask)
{
   uint tgsiMask = 0x0;
   uint i;
   for (i = 0; i < VERT_RESULT_MAX && mask; i++) {
      if (mask & (1 << i)) {
         tgsiMask |= 1 << tgsi_mesa_translate_vertex_output(i);
      }
      mask &= ~(1 << i);
   }
   return tgsiMask;
}

uint
tgsi_mesa_translate_fragment_input_mask(GLbitfield mask)
{
   uint tgsiMask = 0x0;
   uint i;
   for (i = 0; i < FRAG_ATTRIB_MAX && mask; i++) {
      if (mask & (1 << i)) {
         tgsiMask |= 1 << tgsi_mesa_translate_fragment_input(i);
      }
      mask &= ~(1 << i);
   }
   return tgsiMask;
}


uint
tgsi_mesa_translate_fragment_output_mask(GLbitfield mask)
{
   uint tgsiMask = 0x0;
   uint i;
   for (i = 0; i < FRAG_RESULT_MAX && mask; i++) {
      if (mask & (1 << i)) {
         tgsiMask |= 1 << tgsi_mesa_translate_fragment_output(i);
      }
      mask &= ~(1 << i);
   }
   return tgsiMask;
}






/*
 * Map mesa register file to TGSI register file.
 */
static GLuint
map_register_file(
   enum register_file file )
{
   switch( file ) {
   case PROGRAM_UNDEFINED:
      return TGSI_FILE_NULL;
   case PROGRAM_TEMPORARY:
      return TGSI_FILE_TEMPORARY;
   //case PROGRAM_LOCAL_PARAM:
   //case PROGRAM_ENV_PARAM:
   case PROGRAM_STATE_VAR:
   case PROGRAM_NAMED_PARAM:
   case PROGRAM_CONSTANT:
   case PROGRAM_UNIFORM:
      return TGSI_FILE_CONSTANT;
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
 * \param processor  either TGSI_PROCESSOR_FRAGMENT or  TGSI_PROCESSOR_VERTEX
 * \param file  one of TGSI_FILE_x
 * \param index  the mesa register file index
 * \param usage_bitmask  ???
 */
static GLuint
map_register_file_index(
   GLuint processor,
   GLuint file,
   GLuint index,
   GLbitfield usage_bitmask )
{
   GLuint mapped_index;
   GLuint i;

   assert(processor == TGSI_PROCESSOR_FRAGMENT
          || processor == TGSI_PROCESSOR_VERTEX);

   switch( file ) {
   case TGSI_FILE_INPUT:
      /*
       * The fragment/vertex program input indexes (FRAG/VERT_ATTRIB_x) get
       * mapped to a packed sequence of integers.
       * If a program uses one input attribute, the mapped index will be 1.
       * If a program uses two input attribs, the mapped indexes will be 1,2.
       * If a program uses 3 input attribs, the mapped indexes will be 1,2,3.
       * etc.
       */
      assert( index < 32 );
      assert( usage_bitmask & (1 << index) );
      mapped_index = 0;
      for( i = 0; i < index; i++ ) {
         if( usage_bitmask & (1 << i) ) {
            mapped_index++;
         }
      }
      printf("Map input %d to %d\n", index, mapped_index);
      break;

   case TGSI_FILE_OUTPUT:
      assert( usage_bitmask == 0x0 );
      if( processor == TGSI_PROCESSOR_FRAGMENT ) {
         /* depth result  -> index 0
          * color results -> index 1, 2, ...
          */
	 if( index == FRAG_RESULT_DEPR ) {
            mapped_index = TGSI_ATTRIB_POS;
         }
         else {
            assert( index == FRAG_RESULT_COLR );
            mapped_index = TGSI_ATTRIB_COLOR0;
         }
      }
      else {
         /* mapped_index = VERT_RESULT_x */
         mapped_index = index;
      }
      break;

   default:
      mapped_index = index;
   }

   return mapped_index;
}

/*
 * Map mesa texture target to TGSI texture target.
 */
static GLuint
map_texture_target(
   GLuint textarget )
{
   switch( textarget ) {
   case TEXTURE_1D_INDEX:
      return TGSI_TEXTURE_1D;
   case TEXTURE_2D_INDEX:
      return TGSI_TEXTURE_2D;
   case TEXTURE_3D_INDEX:
      return TGSI_TEXTURE_3D;
   case TEXTURE_CUBE_INDEX:
      return TGSI_TEXTURE_CUBE;
   case TEXTURE_RECT_INDEX:
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

static GLboolean
compile_instruction(
   const struct prog_instruction *inst,
   struct tgsi_full_instruction *fullinst,
   GLuint inputs_read,
   GLuint preamble_size,
   GLuint processor )
{
   GLuint i;
   struct tgsi_full_dst_register *fulldst;
   struct tgsi_full_src_register *fullsrc;

   *fullinst = tgsi_default_full_instruction();

   fullinst->Instruction.Saturate = convert_sat( inst->SaturateMode );
   fullinst->Instruction.NumDstRegs = _mesa_num_inst_dst_regs( inst->Opcode );
   fullinst->Instruction.NumSrcRegs = _mesa_num_inst_src_regs( inst->Opcode );

   fulldst = &fullinst->FullDstRegisters[0];
   fulldst->DstRegister.File = map_register_file( inst->DstReg.File );
   fulldst->DstRegister.Index = map_register_file_index(
      processor,
      fulldst->DstRegister.File,
      inst->DstReg.Index,
      0x0 );
   fulldst->DstRegister.WriteMask = convert_writemask( inst->DstReg.WriteMask );

   for( i = 0; i < fullinst->Instruction.NumSrcRegs; i++ ) {
      GLuint j;

      fullsrc = &fullinst->FullSrcRegisters[i];
      fullsrc->SrcRegister.File = map_register_file( inst->SrcReg[i].File );
      fullsrc->SrcRegister.Index = map_register_file_index(
         processor,
         fullsrc->SrcRegister.File,
         inst->SrcReg[i].Index,
         inputs_read );

      for( j = 0; j < 4; j++ ) {
         GLuint swz;

         swz = GET_SWZ( inst->SrcReg[i].Swizzle, j );
         if( swz > SWIZZLE_W ) {
            tgsi_util_set_src_register_extswizzle(
               &fullsrc->SrcRegisterExtSwz,
               swz,
               j );
         }
         else {
            tgsi_util_set_src_register_swizzle(
               &fullsrc->SrcRegister,
               swz,
               j );
         }
      }

      if( inst->SrcReg[i].NegateBase == NEGATE_XYZW ) {
         fullsrc->SrcRegister.Negate = 1;
      }
      else if( inst->SrcReg[i].NegateBase != NEGATE_NONE ) {
         if( inst->SrcReg[i].NegateBase & NEGATE_X ) {
            fullsrc->SrcRegisterExtSwz.NegateX = 1;
         }
         if( inst->SrcReg[i].NegateBase & NEGATE_Y ) {
            fullsrc->SrcRegisterExtSwz.NegateY = 1;
         }
         if( inst->SrcReg[i].NegateBase & NEGATE_Z ) {
            fullsrc->SrcRegisterExtSwz.NegateZ = 1;
         }
         if( inst->SrcReg[i].NegateBase & NEGATE_W ) {
            fullsrc->SrcRegisterExtSwz.NegateW = 1;
         }
      }

      if( inst->SrcReg[i].Abs ) {
         fullsrc->SrcRegisterExtMod.Absolute = 1;
      }

      if( inst->SrcReg[i].NegateAbs ) {
         fullsrc->SrcRegisterExtMod.Negate = 1;
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
      break;
   case OPCODE_BGNSUB:
      fullinst->Instruction.Opcode = TGSI_OPCODE_BGNSUB;
      break;
   case OPCODE_BRA:
      fullinst->Instruction.Opcode = TGSI_OPCODE_BRA;
      break;
   case OPCODE_BRK:
      fullinst->Instruction.Opcode = TGSI_OPCODE_BRK;
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
      break;
   case OPCODE_ENDSUB:
      fullinst->Instruction.Opcode = TGSI_OPCODE_ENDSUB;
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
   case OPCODE_INT:
      fullinst->Instruction.Opcode = TGSI_OPCODE_INT;
      break;
   case OPCODE_KIL:
      fullinst->Instruction.Opcode = TGSI_OPCODE_KIL;
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
   case OPCODE_POW:
      fullinst->Instruction.Opcode = TGSI_OPCODE_POW;
      break;
   case OPCODE_RCP:
      fullinst->Instruction.Opcode = TGSI_OPCODE_RCP;
      break;
   case OPCODE_RSQ:
      fullinst->Instruction.Opcode = TGSI_OPCODE_RSQ;
      tgsi_util_set_full_src_register_sign_mode(
         &fullinst->FullSrcRegisters[0],
         TGSI_UTIL_SIGN_CLEAR );
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
   case OPCODE_SUB:
      fullinst->Instruction.Opcode = TGSI_OPCODE_SUB;
      break;
   case OPCODE_SWZ:
      fullinst->Instruction.Opcode = TGSI_OPCODE_SWZ;
      break;
   case OPCODE_TEX:
      fullinst->Instruction.Opcode = TGSI_OPCODE_TEX;
      fullinst->Instruction.NumSrcRegs = 2;
      fullinst->InstructionExtTexture.Texture = map_texture_target( inst->TexSrcTarget );
      fullinst->FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_SAMPLER;
      fullinst->FullSrcRegisters[1].SrcRegister.Index = inst->TexSrcUnit;
      break;
   case OPCODE_TXB:
      fullinst->Instruction.Opcode = TGSI_OPCODE_TXB;
      fullinst->Instruction.NumSrcRegs = 2;
      fullinst->InstructionExtTexture.Texture = map_texture_target( inst->TexSrcTarget );
      fullinst->FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_SAMPLER;
      fullinst->FullSrcRegisters[1].SrcRegister.Index = inst->TexSrcUnit;
      break;
   case OPCODE_TXD:
      fullinst->Instruction.Opcode = TGSI_OPCODE_TXD;
      fullinst->Instruction.NumSrcRegs = 2;
      fullinst->InstructionExtTexture.Texture = map_texture_target( inst->TexSrcTarget );
      fullinst->FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_SAMPLER;
      fullinst->FullSrcRegisters[1].SrcRegister.Index = inst->TexSrcUnit;
      break;
   case OPCODE_TXL:
      fullinst->Instruction.Opcode = TGSI_OPCODE_TXL;
      fullinst->Instruction.NumSrcRegs = 2;
      fullinst->InstructionExtTexture.Texture = map_texture_target( inst->TexSrcTarget );
      fullinst->FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_SAMPLER;
      fullinst->FullSrcRegisters[1].SrcRegister.Index = inst->TexSrcUnit;
      break;
   case OPCODE_TXP:
      fullinst->Instruction.Opcode = TGSI_OPCODE_TEX;
      fullinst->Instruction.NumSrcRegs = 2;
      fullinst->InstructionExtTexture.Texture = map_texture_target( inst->TexSrcTarget );
      fullinst->FullSrcRegisters[0].SrcRegisterExtSwz.ExtDivide = TGSI_EXTSWIZZLE_W;
      fullinst->FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_SAMPLER;
      fullinst->FullSrcRegisters[1].SrcRegister.Index = inst->TexSrcUnit;
      break;
   case OPCODE_XPD:
      fullinst->Instruction.Opcode = TGSI_OPCODE_XPD;
      fulldst->DstRegister.WriteMask &= TGSI_WRITEMASK_XYZ;
      break;
   case OPCODE_END:
      return GL_TRUE;
   default:
      assert( 0 );
   }

   return GL_FALSE;
}

static struct tgsi_full_declaration
make_frag_input_decl(
   GLuint first,
   GLuint last,
   GLuint interpolate,
   GLuint usage_mask )
{
   struct tgsi_full_declaration decl;

   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_INPUT;
   decl.Declaration.Declare = TGSI_DECLARE_RANGE;
   decl.Declaration.UsageMask = usage_mask;
   decl.Declaration.Interpolate = 1;
   decl.u.DeclarationRange.First = first;
   decl.u.DeclarationRange.Last = last;
   decl.Interpolation.Interpolate = interpolate;

   return decl;
}

static struct tgsi_full_declaration
make_frag_output_decl(
   GLuint index,
   GLuint semantic_name,
   GLuint usage_mask )
{
   struct tgsi_full_declaration decl;

   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_OUTPUT;
   decl.Declaration.Declare = TGSI_DECLARE_RANGE;
   decl.Declaration.UsageMask = usage_mask;
   decl.Declaration.Semantic = 1;
   decl.u.DeclarationRange.First = index;
   decl.u.DeclarationRange.Last = index;
   decl.Semantic.SemanticName = semantic_name;
   decl.Semantic.SemanticIndex = 0;

   return decl;
}

GLboolean
tgsi_mesa_compile_fp_program(
   const struct gl_fragment_program *program,
   struct tgsi_token *tokens,
   GLuint maxTokens )
{
   GLuint i, ti, count;
   struct tgsi_header *header;
   struct tgsi_processor *processor;
   struct tgsi_full_declaration fulldecl;
   struct tgsi_full_instruction fullinst;
   struct tgsi_full_dst_register *fulldst;
   struct tgsi_full_src_register *fullsrc;
   GLuint inputs_read;
   GLboolean reads_wpos;
   GLuint preamble_size = 0;

   *(struct tgsi_version *) &tokens[0] = tgsi_build_version();

   header = (struct tgsi_header *) &tokens[1];
   *header = tgsi_build_header();

   processor = (struct tgsi_processor *) &tokens[2];
   *processor = tgsi_build_processor( TGSI_PROCESSOR_FRAGMENT, header );

   ti = 3;

   reads_wpos = program->Base.InputsRead & (1 << FRAG_ATTRIB_WPOS);
   inputs_read = program->Base.InputsRead | (1 << FRAG_ATTRIB_WPOS);

   /*
    * Declare input attributes. Note that we do not interpolate fragment position.
    */

   /* Fragment position. */
   if( reads_wpos ) {
      fulldecl = make_frag_input_decl(
         0,
         0,
         TGSI_INTERPOLATE_CONSTANT,
         TGSI_WRITEMASK_XY );
      ti += tgsi_build_full_declaration(
         &fulldecl,
         &tokens[ti],
         header,
         maxTokens - ti );
   }

   /* Fragment zw. */
   fulldecl = make_frag_input_decl(
      0,
      0,
      TGSI_INTERPOLATE_LINEAR,
      reads_wpos ? TGSI_WRITEMASK_ZW : TGSI_WRITEMASK_Z );
   ti += tgsi_build_full_declaration(
      &fulldecl,
      &tokens[ti],
      header,
      maxTokens - ti );

   count = 0;
   for( i = 1; i < 32; i++ ) {
      if( inputs_read & (1 << i) ) {
         count++;
      }
   }
   if( count > 0 ) {
      fulldecl = make_frag_input_decl(
         1,
         1 + count - 1,
         TGSI_INTERPOLATE_LINEAR,
         TGSI_WRITEMASK_XYZW );
      ti += tgsi_build_full_declaration(
         &fulldecl,
         &tokens[ti],
         header,
         maxTokens - ti );
   }

   /*
    * Declare output attributes.
    */
   assert(
      program->Base.OutputsWritten ==
      (program->Base.OutputsWritten & ((1 << FRAG_RESULT_COLR) | (1 << FRAG_RESULT_DEPR))) );

   fulldecl = make_frag_output_decl(
      0,
      TGSI_SEMANTIC_DEPTH,
      TGSI_WRITEMASK_Z );
   ti += tgsi_build_full_declaration(
      &fulldecl,
      &tokens[ti],
      header,
      maxTokens - ti );

   if( program->Base.OutputsWritten & (1 << FRAG_RESULT_COLR) ) {
      fulldecl = make_frag_output_decl(
         1,
         TGSI_SEMANTIC_COLOR,
         TGSI_WRITEMASK_XYZW );
      ti += tgsi_build_full_declaration(
         &fulldecl,
         &tokens[ti],
         header,
         maxTokens - ti );
   }

   /*
    * Copy fragment z if the shader does not write it.
    */
#if 0
   if( !(program->Base.OutputsWritten & (1 << FRAG_RESULT_DEPR)) ) {
      fullinst = tgsi_default_full_instruction();

      fullinst.Instruction.Opcode = TGSI_OPCODE_MOV;
      fullinst.Instruction.NumDstRegs = 1;
      fullinst.Instruction.NumSrcRegs = 1;

      fulldst = &fullinst.FullDstRegisters[0];
      fulldst->DstRegister.File = TGSI_FILE_OUTPUT;
      fulldst->DstRegister.Index = 0;
      fulldst->DstRegister.WriteMask = TGSI_WRITEMASK_Z;

      fullsrc = &fullinst.FullSrcRegisters[0];
      fullsrc->SrcRegister.File = TGSI_FILE_INPUT;
      fullsrc->SrcRegister.Index = 0;

      ti += tgsi_build_full_instruction(
         &fullinst,
         &tokens[ti],
         header,
         maxTokens - ti );
      preamble_size++;
   }
#endif

   for( i = 0; i < program->Base.NumInstructions; i++ ) {
      if( compile_instruction(
            &program->Base.Instructions[i],
            &fullinst,
            inputs_read,
            preamble_size,
            TGSI_PROCESSOR_FRAGMENT ) ) {
         assert( i == program->Base.NumInstructions - 1 );

         if( TGSI_DEBUG ) {
            tgsi_dump( tokens, 0 );
         }
         break;
      }

      ti += tgsi_build_full_instruction(
         &fullinst,
         &tokens[ti],
         header,
         maxTokens - ti );
   }

   return GL_TRUE;
}

GLboolean
tgsi_mesa_compile_vp_program(
   const struct gl_vertex_program *program,
   struct tgsi_token *tokens,
   GLuint maxTokens )
{
   GLuint i, ti;
   struct tgsi_header *header;
   struct tgsi_processor *processor;
   struct tgsi_full_instruction fullinst;
   GLuint inputs_read = ~0;

   *(struct tgsi_version *) &tokens[0] = tgsi_build_version();

   header = (struct tgsi_header *) &tokens[1];
   *header = tgsi_build_header();

   processor = (struct tgsi_processor *) &tokens[2];
   *processor = tgsi_build_processor( TGSI_PROCESSOR_VERTEX, header );

   ti = 3;

   for( i = 0; i < program->Base.NumInstructions; i++ ) {
      if( compile_instruction(
            &program->Base.Instructions[i],
            &fullinst,
            inputs_read,
            0,
            TGSI_PROCESSOR_VERTEX ) ) {
         assert( i == program->Base.NumInstructions - 1 );

	 if( TGSI_DEBUG ) {
            tgsi_dump( tokens, 0 );
         }
         break;
      }

      ti += tgsi_build_full_instruction(
         &fullinst,
         &tokens[ti],
         header,
         maxTokens - ti );
   }

   return GL_TRUE;
}

