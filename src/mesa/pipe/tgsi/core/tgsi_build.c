#include "tgsi_platform.h"
#include "tgsi_core.h"

/*
 * version
 */

struct tgsi_version
tgsi_build_version( void )
{
   struct tgsi_version  version;

   version.MajorVersion = 1;
   version.MinorVersion = 1;
   version.Padding = 0;

   return version;
}

/*
 * header
 */

struct tgsi_header
tgsi_build_header( void )
{
   struct tgsi_header header;

   header.HeaderSize = 1;
   header.BodySize = 0;

   return header;
}

static void
header_headersize_grow( struct tgsi_header *header )
{
   assert (header->HeaderSize < 0xFF);
   assert (header->BodySize == 0);

   header->HeaderSize++;
}

static void
header_bodysize_grow( struct tgsi_header *header )
{
   assert (header->BodySize < 0xFFFFFF);

   header->BodySize++;
}

struct tgsi_processor
tgsi_default_processor( void )
{
   struct tgsi_processor processor;

   processor.Processor = TGSI_PROCESSOR_FRAGMENT;
   processor.Padding = 0;

   return processor;
}

struct tgsi_processor
tgsi_build_processor(
   GLuint type,
   struct tgsi_header *header )
{
   struct tgsi_processor processor;

   processor = tgsi_default_processor();
   processor.Processor = type;

   header_headersize_grow( header );

   return processor;
}

/*
 * declaration
 */

struct tgsi_declaration
tgsi_default_declaration( void )
{
   struct tgsi_declaration declaration;

   declaration.Type = TGSI_TOKEN_TYPE_DECLARATION;
   declaration.Size = 1;
   declaration.File = TGSI_FILE_NULL;
   declaration.Declare = TGSI_DECLARE_RANGE;
   declaration.Interpolate = 0;
   declaration.Padding = 0;
   declaration.Extended = 0;

   return declaration;
}

struct tgsi_declaration
tgsi_build_declaration(
   GLuint file,
   GLuint declare,
   GLuint interpolate,
   struct tgsi_header *header )
{
   struct tgsi_declaration declaration;

   assert (file <= TGSI_FILE_IMMEDIATE);
   assert (declare <= TGSI_DECLARE_MASK);

   declaration = tgsi_default_declaration();
   declaration.File = file;
   declaration.Declare = declare;
   declaration.Interpolate = interpolate;

   header_bodysize_grow( header );

   return declaration;
}

static void
declaration_grow(
   struct tgsi_declaration *declaration,
   struct tgsi_header *header )
{
   assert (declaration->Size < 0xFF);

   declaration->Size++;

   header_bodysize_grow( header );
}

struct tgsi_full_declaration
tgsi_default_full_declaration( void )
{
   struct tgsi_full_declaration  full_declaration;

   full_declaration.Declaration  = tgsi_default_declaration();
   full_declaration.Interpolation = tgsi_default_declaration_interpolation();

   return full_declaration;
}

GLuint
tgsi_build_full_declaration(
   const struct   tgsi_full_declaration *full_decl,
   struct tgsi_token *tokens,
   struct tgsi_header *header,
   GLuint maxsize )
{
   GLuint size = 0;
   struct tgsi_declaration *declaration;

   if( maxsize <= size )
     return 0;
   declaration = (struct tgsi_declaration *) &tokens[size];
   size++;

   *declaration = tgsi_build_declaration(
      full_decl->Declaration.File,
      full_decl->Declaration.Declare,
      full_decl->Declaration.Interpolate,
      header );

   switch( full_decl->Declaration.Declare )  {
   case  TGSI_DECLARE_RANGE:
   {
      struct tgsi_declaration_range *dr;

      if( maxsize <=   size )
         return 0;
      dr = (struct tgsi_declaration_range *) &tokens[size];
      size++;

      *dr = tgsi_build_declaration_range(
         full_decl->u.DeclarationRange.First,
         full_decl->u.DeclarationRange.Last,
         declaration,
         header );
      break;
    }

   case  TGSI_DECLARE_MASK:
   {
      struct  tgsi_declaration_mask *dm;

      if( maxsize <=   size )
         return 0;
      dm = (struct tgsi_declaration_mask  *) &tokens[size];
      size++;

      *dm = tgsi_build_declaration_mask(
         full_decl->u.DeclarationMask.Mask,
         declaration,
         header );
      break;
   }

   default:
      assert( 0 );
   }

   if( full_decl->Declaration.Interpolate ) {
      struct tgsi_declaration_interpolation *di;

      if( maxsize <= size )
         return  0;
      di = (struct tgsi_declaration_interpolation *) &tokens[size];
      size++;

      *di = tgsi_build_declaration_interpolation(
         full_decl->Interpolation.Interpolate,
         declaration,
         header );
   }

   return size;
}

struct tgsi_declaration_range
tgsi_build_declaration_range(
   GLuint first,
   GLuint last,
   struct tgsi_declaration *declaration,
   struct tgsi_header *header )
{
   struct tgsi_declaration_range declaration_range;

   assert (last >=   first);
   assert (last <=   0xFFFF);

   declaration_range.First = first;
   declaration_range.Last = last;

   declaration_grow( declaration, header );

   return declaration_range;
}

struct tgsi_declaration_mask
tgsi_build_declaration_mask(
   GLuint mask,
   struct tgsi_declaration *declaration,
   struct tgsi_header *header )
{
   struct tgsi_declaration_mask  declaration_mask;

   declaration_mask.Mask = mask;

   declaration_grow( declaration, header );

   return declaration_mask;
}

struct tgsi_declaration_interpolation
tgsi_default_declaration_interpolation( void )
{
   struct tgsi_declaration_interpolation di;

   di.Interpolate = TGSI_INTERPOLATE_CONSTANT;
   di.Padding = 0;

   return di;
}

struct tgsi_declaration_interpolation
tgsi_build_declaration_interpolation(
   GLuint interpolate,
   struct tgsi_declaration *declaration,
   struct tgsi_header *header )
{
   struct tgsi_declaration_interpolation di;

   assert( interpolate <= TGSI_INTERPOLATE_PERSPECTIVE );

   di = tgsi_default_declaration_interpolation();
   di.Interpolate = interpolate;

   declaration_grow( declaration, header );

   return di;
}

/*
 * immediate
 */

struct tgsi_immediate
tgsi_default_immediate( void )
{
   struct tgsi_immediate immediate;

   immediate.Type = TGSI_TOKEN_TYPE_IMMEDIATE;
   immediate.Size = 1;
   immediate.DataType = TGSI_IMM_FLOAT32;
   immediate.Padding = 0;
   immediate.Extended = 0;

   return immediate;
}

struct tgsi_immediate
tgsi_build_immediate(
   struct tgsi_header *header )
{
   struct tgsi_immediate immediate;

   immediate = tgsi_default_immediate();

   header_bodysize_grow( header  );

   return immediate;
}

struct tgsi_full_immediate
tgsi_default_full_immediate( void )
{
   struct tgsi_full_immediate fullimm;

   fullimm.Immediate = tgsi_default_immediate();
   fullimm.u.Pointer = (void *) 0;

   return fullimm;
}

static void
immediate_grow(
   struct tgsi_immediate *immediate,
   struct tgsi_header *header )
{
   assert( immediate->Size < 0xFF );

   immediate->Size++;

   header_bodysize_grow( header );
}

struct tgsi_immediate_float32
tgsi_build_immediate_float32(
   GLfloat value,
   struct tgsi_immediate *immediate,
   struct tgsi_header *header )
{
   struct tgsi_immediate_float32 immediate_float32;

   immediate_float32.Float = value;

   immediate_grow( immediate, header );

   return immediate_float32;
}

GLuint
tgsi_build_full_immediate(
   const struct tgsi_full_immediate *full_imm,
   struct tgsi_token *tokens,
   struct tgsi_header *header,
   GLuint maxsize )
{
   GLuint size = 0,  i;
   struct tgsi_immediate *immediate;

   if( maxsize <= size )
      return 0;
   immediate = (struct tgsi_immediate *) &tokens[size];
   size++;

   *immediate = tgsi_build_immediate( header );

   for( i = 0; i < full_imm->Immediate.Size - 1; i++ ) {
      struct tgsi_immediate_float32 *if32;

      if( maxsize <= size )
         return  0;
      if32 = (struct tgsi_immediate_float32 *) &tokens[size];
      size++;

      *if32  = tgsi_build_immediate_float32(
         full_imm->u.ImmediateFloat32[i].Float,
         immediate,
         header );
   }

   return size;
}

/*
 * instruction
 */

struct tgsi_instruction
tgsi_default_instruction( void )
{
   struct tgsi_instruction instruction;

   instruction.Type = TGSI_TOKEN_TYPE_INSTRUCTION;
   instruction.Size = 1;
   instruction.Opcode = TGSI_OPCODE_MOV;
   instruction.Saturate = TGSI_SAT_NONE;
   instruction.NumDstRegs = 1;
   instruction.NumSrcRegs = 1;
   instruction.Padding  = 0;
   instruction.Extended = 0;

   return instruction;
}

struct tgsi_instruction
tgsi_build_instruction(
   GLuint opcode,
   GLuint saturate,
   GLuint num_dst_regs,
   GLuint num_src_regs,
   struct tgsi_header *header )
{
   struct tgsi_instruction instruction;

   assert (opcode <= TGSI_OPCODE_LAST);
   assert (saturate <= TGSI_SAT_MINUS_PLUS_ONE);
   assert (num_dst_regs <= 3);
   assert (num_src_regs <= 15);

   instruction = tgsi_default_instruction();
   instruction.Opcode = opcode;
   instruction.Saturate = saturate;
   instruction.NumDstRegs = num_dst_regs;
   instruction.NumSrcRegs = num_src_regs;

   header_bodysize_grow( header );

   return instruction;
}

static void
instruction_grow(
   struct tgsi_instruction *instruction,
   struct tgsi_header *header )
{
   assert (instruction->Size <   0xFF);

   instruction->Size++;

   header_bodysize_grow( header );
}

struct tgsi_full_instruction
tgsi_default_full_instruction( void )
{
   struct tgsi_full_instruction full_instruction;
   GLuint i;

   full_instruction.Instruction = tgsi_default_instruction();
   full_instruction.InstructionExtNv = tgsi_default_instruction_ext_nv();
   full_instruction.InstructionExtLabel = tgsi_default_instruction_ext_label();
   full_instruction.InstructionExtTexture = tgsi_default_instruction_ext_texture();
   for( i = 0;  i < TGSI_FULL_MAX_DST_REGISTERS; i++ ) {
      full_instruction.FullDstRegisters[i] = tgsi_default_full_dst_register();
   }
   for( i = 0;  i < TGSI_FULL_MAX_SRC_REGISTERS; i++ ) {
      full_instruction.FullSrcRegisters[i] = tgsi_default_full_src_register();
   }

   return full_instruction;
}

GLuint
tgsi_build_full_instruction(
   const struct tgsi_full_instruction *full_inst,
   struct  tgsi_token *tokens,
   struct  tgsi_header *header,
   GLuint  maxsize )
{
   GLuint size = 0;
   GLuint i;
   struct tgsi_instruction *instruction;
   struct tgsi_token *prev_token;

   if( maxsize <= size )
      return 0;
   instruction = (struct tgsi_instruction *) &tokens[size];
   size++;

   *instruction = tgsi_build_instruction(
      full_inst->Instruction.Opcode,
      full_inst->Instruction.Saturate,
      full_inst->Instruction.NumDstRegs,
      full_inst->Instruction.NumSrcRegs,
      header );
   prev_token = (struct tgsi_token  *) instruction;

   if( tgsi_compare_instruction_ext_nv(
         full_inst->InstructionExtNv,
         tgsi_default_instruction_ext_nv() ) ) {
      struct tgsi_instruction_ext_nv *instruction_ext_nv;

      if( maxsize <= size )
         return 0;
      instruction_ext_nv =
         (struct  tgsi_instruction_ext_nv *) &tokens[size];
      size++;

      *instruction_ext_nv  = tgsi_build_instruction_ext_nv(
         full_inst->InstructionExtNv.Precision,
         full_inst->InstructionExtNv.CondDstIndex,
         full_inst->InstructionExtNv.CondFlowIndex,
         full_inst->InstructionExtNv.CondMask,
         full_inst->InstructionExtNv.CondSwizzleX,
         full_inst->InstructionExtNv.CondSwizzleY,
         full_inst->InstructionExtNv.CondSwizzleZ,
         full_inst->InstructionExtNv.CondSwizzleW,
         full_inst->InstructionExtNv.CondDstUpdate,
         full_inst->InstructionExtNv.CondFlowEnable,
         prev_token,
         instruction,
         header );
      prev_token = (struct tgsi_token  *) instruction_ext_nv;
   }

   if( tgsi_compare_instruction_ext_label(
         full_inst->InstructionExtLabel,
         tgsi_default_instruction_ext_label() ) ) {
      struct tgsi_instruction_ext_label *instruction_ext_label;

      if( maxsize <= size )
         return 0;
      instruction_ext_label =
         (struct  tgsi_instruction_ext_label *) &tokens[size];
      size++;

      *instruction_ext_label = tgsi_build_instruction_ext_label(
         full_inst->InstructionExtLabel.Label,
         prev_token,
         instruction,
         header );
      prev_token = (struct tgsi_token  *) instruction_ext_label;
   }

   if( tgsi_compare_instruction_ext_texture(
         full_inst->InstructionExtTexture,
         tgsi_default_instruction_ext_texture() ) ) {
      struct tgsi_instruction_ext_texture *instruction_ext_texture;

      if( maxsize <= size )
         return 0;
      instruction_ext_texture =
         (struct  tgsi_instruction_ext_texture *) &tokens[size];
      size++;

      *instruction_ext_texture = tgsi_build_instruction_ext_texture(
         full_inst->InstructionExtTexture.Texture,
         prev_token,
         instruction,
         header   );
      prev_token = (struct tgsi_token  *) instruction_ext_texture;
   }

   for( i = 0;  i <   full_inst->Instruction.NumDstRegs; i++ ) {
      const struct tgsi_full_dst_register *reg = &full_inst->FullDstRegisters[i];
      struct tgsi_dst_register *dst_register;
      struct tgsi_token *prev_token;

      if( maxsize <= size )
         return 0;
      dst_register = (struct tgsi_dst_register *) &tokens[size];
      size++;

      *dst_register = tgsi_build_dst_register(
         reg->DstRegister.File,
         reg->DstRegister.WriteMask,
         reg->DstRegister.Index,
         instruction,
         header );
      prev_token = (struct tgsi_token  *) dst_register;

      if( tgsi_compare_dst_register_ext_concode(
            reg->DstRegisterExtConcode,
            tgsi_default_dst_register_ext_concode() ) ) {
         struct tgsi_dst_register_ext_concode *dst_register_ext_concode;

         if( maxsize <= size )
            return 0;
         dst_register_ext_concode =
            (struct  tgsi_dst_register_ext_concode *) &tokens[size];
         size++;

         *dst_register_ext_concode =   tgsi_build_dst_register_ext_concode(
            reg->DstRegisterExtConcode.CondMask,
            reg->DstRegisterExtConcode.CondSwizzleX,
            reg->DstRegisterExtConcode.CondSwizzleY,
            reg->DstRegisterExtConcode.CondSwizzleZ,
            reg->DstRegisterExtConcode.CondSwizzleW,
            reg->DstRegisterExtConcode.CondSrcIndex,
            prev_token,
            instruction,
            header );
         prev_token = (struct tgsi_token  *) dst_register_ext_concode;
      }

      if( tgsi_compare_dst_register_ext_modulate(
            reg->DstRegisterExtModulate,
            tgsi_default_dst_register_ext_modulate() ) ) {
         struct tgsi_dst_register_ext_modulate *dst_register_ext_modulate;

         if( maxsize <= size )
            return 0;
         dst_register_ext_modulate =
            (struct  tgsi_dst_register_ext_modulate *) &tokens[size];
         size++;

         *dst_register_ext_modulate = tgsi_build_dst_register_ext_modulate(
            reg->DstRegisterExtModulate.Modulate,
            prev_token,
            instruction,
            header );
         prev_token = (struct tgsi_token  *) dst_register_ext_modulate;
      }
   }

   for( i = 0;  i < full_inst->Instruction.NumSrcRegs; i++ ) {
      const struct tgsi_full_src_register *reg = &full_inst->FullSrcRegisters[i];
      struct tgsi_src_register *src_register;
      struct tgsi_token *prev_token;

      if( maxsize <= size )
         return 0;
      src_register = (struct tgsi_src_register *)  &tokens[size];
      size++;

      *src_register = tgsi_build_src_register(
         reg->SrcRegister.File,
         reg->SrcRegister.SwizzleX,
         reg->SrcRegister.SwizzleY,
         reg->SrcRegister.SwizzleZ,
         reg->SrcRegister.SwizzleW,
         reg->SrcRegister.Negate,
         reg->SrcRegister.Indirect,
         reg->SrcRegister.Dimension,
         reg->SrcRegister.Index,
         instruction,
         header );
      prev_token = (struct tgsi_token  *) src_register;

      if( tgsi_compare_src_register_ext_swz(
            reg->SrcRegisterExtSwz,
            tgsi_default_src_register_ext_swz() ) ) {
         struct tgsi_src_register_ext_swz *src_register_ext_swz;

         if( maxsize <= size )
            return 0;
         src_register_ext_swz =
            (struct  tgsi_src_register_ext_swz *) &tokens[size];
         size++;

         *src_register_ext_swz = tgsi_build_src_register_ext_swz(
            reg->SrcRegisterExtSwz.ExtSwizzleX,
            reg->SrcRegisterExtSwz.ExtSwizzleY,
            reg->SrcRegisterExtSwz.ExtSwizzleZ,
            reg->SrcRegisterExtSwz.ExtSwizzleW,
            reg->SrcRegisterExtSwz.NegateX,
            reg->SrcRegisterExtSwz.NegateY,
            reg->SrcRegisterExtSwz.NegateZ,
            reg->SrcRegisterExtSwz.NegateW,
            reg->SrcRegisterExtSwz.ExtDivide,
            prev_token,
            instruction,
            header );
         prev_token = (struct tgsi_token  *) src_register_ext_swz;
      }

      if( tgsi_compare_src_register_ext_mod(
            reg->SrcRegisterExtMod,
            tgsi_default_src_register_ext_mod() ) ) {
         struct tgsi_src_register_ext_mod *src_register_ext_mod;

         if( maxsize <= size )
            return 0;
         src_register_ext_mod =
            (struct  tgsi_src_register_ext_mod *) &tokens[size];
         size++;

         *src_register_ext_mod = tgsi_build_src_register_ext_mod(
            reg->SrcRegisterExtMod.Complement,
            reg->SrcRegisterExtMod.Bias,
            reg->SrcRegisterExtMod.Scale2X,
            reg->SrcRegisterExtMod.Absolute,
            reg->SrcRegisterExtMod.Negate,
            prev_token,
            instruction,
            header );
         prev_token = (struct tgsi_token  *) src_register_ext_mod;
      }

      if( reg->SrcRegister.Indirect ) {
         struct  tgsi_src_register *ind;

         if( maxsize <= size )
            return 0;
         ind = (struct tgsi_src_register *) &tokens[size];
         size++;

         *ind = tgsi_build_src_register(
            reg->SrcRegisterInd.File,
            reg->SrcRegisterInd.SwizzleX,
            reg->SrcRegisterInd.SwizzleY,
            reg->SrcRegisterInd.SwizzleZ,
            reg->SrcRegisterInd.SwizzleW,
            reg->SrcRegisterInd.Negate,
            reg->SrcRegisterInd.Indirect,
            reg->SrcRegisterInd.Dimension,
            reg->SrcRegisterInd.Index,
            instruction,
            header );
      }

      if( reg->SrcRegister.Dimension ) {
         struct  tgsi_dimension *dim;

         assert( !reg->SrcRegisterDim.Dimension );

         if( maxsize <= size )
            return 0;
         dim = (struct tgsi_dimension *) &tokens[size];
         size++;

         *dim = tgsi_build_dimension(
            reg->SrcRegisterDim.Indirect,
            reg->SrcRegisterDim.Index,
            instruction,
            header );

         if( reg->SrcRegisterDim.Indirect ) {
            struct tgsi_src_register *ind;

            if( maxsize <= size )
               return 0;
            ind = (struct tgsi_src_register *) &tokens[size];
            size++;

            *ind = tgsi_build_src_register(
               reg->SrcRegisterDimInd.File,
               reg->SrcRegisterDimInd.SwizzleX,
               reg->SrcRegisterDimInd.SwizzleY,
               reg->SrcRegisterDimInd.SwizzleZ,
               reg->SrcRegisterDimInd.SwizzleW,
               reg->SrcRegisterDimInd.Negate,
               reg->SrcRegisterDimInd.Indirect,
               reg->SrcRegisterDimInd.Dimension,
               reg->SrcRegisterDimInd.Index,
               instruction,
               header );
         }
      }
   }

   return size;
}

struct tgsi_instruction_ext_nv
tgsi_default_instruction_ext_nv( void )
{
   struct tgsi_instruction_ext_nv instruction_ext_nv;

   instruction_ext_nv.Type = TGSI_INSTRUCTION_EXT_TYPE_NV;
   instruction_ext_nv.Precision = TGSI_PRECISION_DEFAULT;
   instruction_ext_nv.CondDstIndex = 0;
   instruction_ext_nv.CondFlowIndex = 0;
   instruction_ext_nv.CondMask = TGSI_CC_TR;
   instruction_ext_nv.CondSwizzleX = TGSI_SWIZZLE_X;
   instruction_ext_nv.CondSwizzleY = TGSI_SWIZZLE_Y;
   instruction_ext_nv.CondSwizzleZ = TGSI_SWIZZLE_Z;
   instruction_ext_nv.CondSwizzleW = TGSI_SWIZZLE_W;
   instruction_ext_nv.CondDstUpdate = 0;
   instruction_ext_nv.CondFlowEnable = 0;
   instruction_ext_nv.Padding = 0;
   instruction_ext_nv.Extended = 0;

   return instruction_ext_nv;
}

union token_u32
{
   GLuint   u32;
};

GLuint
tgsi_compare_instruction_ext_nv(
   struct tgsi_instruction_ext_nv a,
   struct tgsi_instruction_ext_nv b )
{
   a.Padding = b.Padding = 0;
   a.Extended = b.Extended = 0;
   return ((union token_u32 *) &a)->u32 != ((union token_u32 *) &b)->u32;
}

struct tgsi_instruction_ext_nv
tgsi_build_instruction_ext_nv(
   GLuint precision,
   GLuint cond_dst_index,
   GLuint cond_flow_index,
   GLuint cond_mask,
   GLuint cond_swizzle_x,
   GLuint cond_swizzle_y,
   GLuint cond_swizzle_z,
   GLuint cond_swizzle_w,
   GLuint cond_dst_update,
   GLuint cond_flow_update,
   struct tgsi_token *prev_token,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header )
{
   struct tgsi_instruction_ext_nv instruction_ext_nv;

   instruction_ext_nv = tgsi_default_instruction_ext_nv();
   instruction_ext_nv.Precision = precision;
   instruction_ext_nv.CondDstIndex = cond_dst_index;
   instruction_ext_nv.CondFlowIndex = cond_flow_index;
   instruction_ext_nv.CondMask = cond_mask;
   instruction_ext_nv.CondSwizzleX = cond_swizzle_x;
   instruction_ext_nv.CondSwizzleY = cond_swizzle_y;
   instruction_ext_nv.CondSwizzleZ = cond_swizzle_z;
   instruction_ext_nv.CondSwizzleW = cond_swizzle_w;
   instruction_ext_nv.CondDstUpdate = cond_dst_update;
   instruction_ext_nv.CondFlowEnable = cond_flow_update;

   prev_token->Extended = 1;
   instruction_grow( instruction, header );

   return instruction_ext_nv;
}

struct tgsi_instruction_ext_label
tgsi_default_instruction_ext_label( void )
{
   struct tgsi_instruction_ext_label instruction_ext_label;

   instruction_ext_label.Type = TGSI_INSTRUCTION_EXT_TYPE_LABEL;
   instruction_ext_label.Label = 0;
   instruction_ext_label.Padding = 0;
   instruction_ext_label.Extended = 0;

   return instruction_ext_label;
}

GLuint
tgsi_compare_instruction_ext_label(
   struct tgsi_instruction_ext_label a,
   struct tgsi_instruction_ext_label b )
{
   a.Padding = b.Padding = 0;
   a.Extended = b.Extended = 0;
   return *(GLuint *) &a != *(GLuint *) &b;
}

struct tgsi_instruction_ext_label
tgsi_build_instruction_ext_label(
   GLuint label,
   struct tgsi_token  *prev_token,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header )
{
   struct tgsi_instruction_ext_label instruction_ext_label;

   instruction_ext_label = tgsi_default_instruction_ext_label();
   instruction_ext_label.Label = label;

   prev_token->Extended = 1;
   instruction_grow( instruction, header );

   return instruction_ext_label;
}

struct tgsi_instruction_ext_texture
tgsi_default_instruction_ext_texture( void )
{
   struct tgsi_instruction_ext_texture instruction_ext_texture;

   instruction_ext_texture.Type = TGSI_INSTRUCTION_EXT_TYPE_TEXTURE;
   instruction_ext_texture.Texture = TGSI_TEXTURE_UNKNOWN;
   instruction_ext_texture.Padding = 0;
   instruction_ext_texture.Extended = 0;

   return instruction_ext_texture;
}

GLuint
tgsi_compare_instruction_ext_texture(
   struct tgsi_instruction_ext_texture a,
   struct tgsi_instruction_ext_texture b )
{
   a.Padding = b.Padding = 0;
   a.Extended = b.Extended = 0;
   return *(GLuint *) &a != *(GLuint *) &b;
}

struct tgsi_instruction_ext_texture
tgsi_build_instruction_ext_texture(
   GLuint texture,
   struct tgsi_token *prev_token,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header )
{
   struct tgsi_instruction_ext_texture instruction_ext_texture;

   instruction_ext_texture = tgsi_default_instruction_ext_texture();
   instruction_ext_texture.Texture = texture;

   prev_token->Extended = 1;
   instruction_grow( instruction, header );

   return instruction_ext_texture;
}

struct tgsi_src_register
tgsi_default_src_register( void )
{
   struct tgsi_src_register src_register;

   src_register.File = TGSI_FILE_NULL;
   src_register.SwizzleX = TGSI_SWIZZLE_X;
   src_register.SwizzleY = TGSI_SWIZZLE_Y;
   src_register.SwizzleZ = TGSI_SWIZZLE_Z;
   src_register.SwizzleW = TGSI_SWIZZLE_W;
   src_register.Negate = 0;
   src_register.Indirect = 0;
   src_register.Dimension = 0;
   src_register.Index = 0;
   src_register.Extended = 0;

   return src_register;
}

struct tgsi_src_register
tgsi_build_src_register(
   GLuint file,
   GLuint swizzle_x,
   GLuint swizzle_y,
   GLuint swizzle_z,
   GLuint swizzle_w,
   GLuint negate,
   GLuint indirect,
   GLuint dimension,
   GLint index,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header )
{
   struct tgsi_src_register   src_register;

   assert( file <= TGSI_FILE_IMMEDIATE );
   assert( swizzle_x <= TGSI_SWIZZLE_W );
   assert( swizzle_y <= TGSI_SWIZZLE_W );
   assert( swizzle_z <= TGSI_SWIZZLE_W );
   assert( swizzle_w <= TGSI_SWIZZLE_W );
   assert( negate <= 1 );
   assert( index >= -0x8000 && index <= 0x7FFF );

   src_register = tgsi_default_src_register();
   src_register.File = file;
   src_register.SwizzleX = swizzle_x;
   src_register.SwizzleY = swizzle_y;
   src_register.SwizzleZ = swizzle_z;
   src_register.SwizzleW = swizzle_w;
   src_register.Negate = negate;
   src_register.Indirect = indirect;
   src_register.Dimension = dimension;
   src_register.Index = index;

   instruction_grow( instruction, header );

   return src_register;
}

struct tgsi_full_src_register
tgsi_default_full_src_register( void )
{
   struct tgsi_full_src_register full_src_register;

   full_src_register.SrcRegister = tgsi_default_src_register();
   full_src_register.SrcRegisterExtSwz = tgsi_default_src_register_ext_swz();
   full_src_register.SrcRegisterExtMod = tgsi_default_src_register_ext_mod();
   full_src_register.SrcRegisterInd = tgsi_default_src_register();
   full_src_register.SrcRegisterDim = tgsi_default_dimension();
   full_src_register.SrcRegisterDimInd = tgsi_default_src_register();

   return full_src_register;
}

struct tgsi_src_register_ext_swz
tgsi_default_src_register_ext_swz( void )
{
   struct tgsi_src_register_ext_swz src_register_ext_swz;

   src_register_ext_swz.Type = TGSI_SRC_REGISTER_EXT_TYPE_SWZ;
   src_register_ext_swz.ExtSwizzleX = TGSI_EXTSWIZZLE_X;
   src_register_ext_swz.ExtSwizzleY = TGSI_EXTSWIZZLE_Y;
   src_register_ext_swz.ExtSwizzleZ = TGSI_EXTSWIZZLE_Z;
   src_register_ext_swz.ExtSwizzleW = TGSI_EXTSWIZZLE_W;
   src_register_ext_swz.NegateX = 0;
   src_register_ext_swz.NegateY = 0;
   src_register_ext_swz.NegateZ = 0;
   src_register_ext_swz.NegateW = 0;
   src_register_ext_swz.ExtDivide = TGSI_EXTSWIZZLE_ONE;
   src_register_ext_swz.Padding = 0;
   src_register_ext_swz.Extended = 0;

   return src_register_ext_swz;
}

GLuint
tgsi_compare_src_register_ext_swz(
   struct tgsi_src_register_ext_swz a,
   struct tgsi_src_register_ext_swz b )
{
   a.Padding = b.Padding = 0;
   a.Extended = b.Extended = 0;
   return *(GLuint *) &a != *(GLuint *) &b;
}

struct tgsi_src_register_ext_swz
tgsi_build_src_register_ext_swz(
   GLuint ext_swizzle_x,
   GLuint ext_swizzle_y,
   GLuint ext_swizzle_z,
   GLuint ext_swizzle_w,
   GLuint negate_x,
   GLuint negate_y,
   GLuint negate_z,
   GLuint negate_w,
   GLuint ext_divide,
   struct tgsi_token *prev_token,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header )
{
   struct tgsi_src_register_ext_swz src_register_ext_swz;

   assert (ext_swizzle_x <= TGSI_EXTSWIZZLE_ONE);
   assert (ext_swizzle_y <= TGSI_EXTSWIZZLE_ONE);
   assert (ext_swizzle_z <= TGSI_EXTSWIZZLE_ONE);
   assert (ext_swizzle_w <= TGSI_EXTSWIZZLE_ONE);
   assert (negate_x <= 1);
   assert (negate_y <= 1);
   assert (negate_z <= 1);
   assert (negate_w <= 1);
   assert (ext_divide <= TGSI_EXTSWIZZLE_ONE);

   src_register_ext_swz = tgsi_default_src_register_ext_swz();
   src_register_ext_swz.ExtSwizzleX = ext_swizzle_x;
   src_register_ext_swz.ExtSwizzleY = ext_swizzle_y;
   src_register_ext_swz.ExtSwizzleZ = ext_swizzle_z;
   src_register_ext_swz.ExtSwizzleW = ext_swizzle_w;
   src_register_ext_swz.NegateX = negate_x;
   src_register_ext_swz.NegateY = negate_y;
   src_register_ext_swz.NegateZ = negate_z;
   src_register_ext_swz.NegateW = negate_w;
   src_register_ext_swz.ExtDivide = ext_divide;

   prev_token->Extended = 1;
   instruction_grow( instruction, header );

   return src_register_ext_swz;
}

struct tgsi_src_register_ext_mod
tgsi_default_src_register_ext_mod( void )
{
   struct tgsi_src_register_ext_mod src_register_ext_mod;

   src_register_ext_mod.Type = TGSI_SRC_REGISTER_EXT_TYPE_MOD;
   src_register_ext_mod.Complement = 0;
   src_register_ext_mod.Bias = 0;
   src_register_ext_mod.Scale2X = 0;
   src_register_ext_mod.Absolute = 0;
   src_register_ext_mod.Negate = 0;
   src_register_ext_mod.Padding = 0;
   src_register_ext_mod.Extended = 0;

   return src_register_ext_mod;
}

GLuint
tgsi_compare_src_register_ext_mod(
   struct tgsi_src_register_ext_mod a,
   struct tgsi_src_register_ext_mod b )
{
   a.Padding = b.Padding = 0;
   a.Extended = b.Extended = 0;
   return *(GLuint *) &a != *(GLuint *) &b;
}

struct tgsi_src_register_ext_mod
tgsi_build_src_register_ext_mod(
   GLuint  complement,
   GLuint  bias,
   GLuint  scale_2x,
   GLuint  absolute,
   GLuint  negate,
   struct  tgsi_token *prev_token,
   struct  tgsi_instruction *instruction,
   struct  tgsi_header *header )
{
   struct tgsi_src_register_ext_mod src_register_ext_mod;

   assert (complement <= 1);
   assert (bias <= 1);
   assert (scale_2x <= 1);
   assert (absolute <= 1);
   assert (negate <= 1);

   src_register_ext_mod = tgsi_default_src_register_ext_mod();
   src_register_ext_mod.Complement = complement;
   src_register_ext_mod.Bias = bias;
   src_register_ext_mod.Scale2X = scale_2x;
   src_register_ext_mod.Absolute = absolute;
   src_register_ext_mod.Negate = negate;

   prev_token->Extended = 1;
   instruction_grow( instruction, header );

   return src_register_ext_mod;
}

struct tgsi_dimension
tgsi_default_dimension( void )
{
   struct tgsi_dimension dimension;

   dimension.Indirect = 0;
   dimension.Dimension = 0;
   dimension.Padding = 0;
   dimension.Index = 0;
   dimension.Extended = 0;

   return dimension;
}

struct tgsi_dimension
tgsi_build_dimension(
   GLuint indirect,
   GLuint index,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header )
{
   struct tgsi_dimension dimension;

   dimension = tgsi_default_dimension();
   dimension.Indirect = indirect;
   dimension.Index = index;

   instruction_grow( instruction, header );

   return dimension;
}

struct tgsi_dst_register
tgsi_default_dst_register( void )
{
   struct tgsi_dst_register dst_register;

   dst_register.File = TGSI_FILE_NULL;
   dst_register.WriteMask = TGSI_WRITEMASK_XYZW;
   dst_register.Indirect = 0;
   dst_register.Dimension = 0;
   dst_register.Index = 0;
   dst_register.Padding = 0;
   dst_register.Extended = 0;

   return dst_register;
}

struct tgsi_dst_register
tgsi_build_dst_register(
   GLuint file,
   GLuint mask,
   GLint index,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header )
{
   struct tgsi_dst_register dst_register;

   assert (file <= TGSI_FILE_IMMEDIATE);
   assert (mask <= TGSI_WRITEMASK_XYZW);
   assert (index >= -32768 && index <= 32767);

   dst_register = tgsi_default_dst_register();
   dst_register.File = file;
   dst_register.WriteMask = mask;
   dst_register.Index = index;

   instruction_grow( instruction, header );

   return dst_register;
}

struct tgsi_full_dst_register
tgsi_default_full_dst_register( void )
{
   struct tgsi_full_dst_register full_dst_register;

   full_dst_register.DstRegister = tgsi_default_dst_register();
   full_dst_register.DstRegisterExtConcode =
      tgsi_default_dst_register_ext_concode();
   full_dst_register.DstRegisterExtModulate =
      tgsi_default_dst_register_ext_modulate();

   return full_dst_register;
}

struct tgsi_dst_register_ext_concode
tgsi_default_dst_register_ext_concode( void )
{
   struct tgsi_dst_register_ext_concode dst_register_ext_concode;

   dst_register_ext_concode.Type = TGSI_DST_REGISTER_EXT_TYPE_CONDCODE;
   dst_register_ext_concode.CondMask = TGSI_CC_TR;
   dst_register_ext_concode.CondSwizzleX = TGSI_SWIZZLE_X;
   dst_register_ext_concode.CondSwizzleY = TGSI_SWIZZLE_Y;
   dst_register_ext_concode.CondSwizzleZ = TGSI_SWIZZLE_Z;
   dst_register_ext_concode.CondSwizzleW = TGSI_SWIZZLE_W;
   dst_register_ext_concode.CondSrcIndex = 0;
   dst_register_ext_concode.Padding = 0;
   dst_register_ext_concode.Extended = 0;

   return dst_register_ext_concode;
}

GLuint
tgsi_compare_dst_register_ext_concode(
   struct tgsi_dst_register_ext_concode a,
   struct tgsi_dst_register_ext_concode b )
{
   a.Padding = b.Padding = 0;
   a.Extended = b.Extended = 0;
   return *(GLuint *) &a != *(GLuint *) &b;
}

struct tgsi_dst_register_ext_concode
tgsi_build_dst_register_ext_concode(
   GLuint  cc,
   GLuint  swizzle_x,
   GLuint  swizzle_y,
   GLuint  swizzle_z,
   GLuint  swizzle_w,
   GLint index,
   struct  tgsi_token *prev_token,
   struct  tgsi_instruction *instruction,
   struct  tgsi_header *header )
{
   struct tgsi_dst_register_ext_concode dst_register_ext_concode;

   assert (cc <= TGSI_CC_FL);
   assert (swizzle_x <= TGSI_SWIZZLE_W);
   assert (swizzle_y <= TGSI_SWIZZLE_W);
   assert (swizzle_z <= TGSI_SWIZZLE_W);
   assert (swizzle_w <= TGSI_SWIZZLE_W);
   assert (index >= -32768 && index <= 32767);

   dst_register_ext_concode = tgsi_default_dst_register_ext_concode();
   dst_register_ext_concode.CondMask = cc;
   dst_register_ext_concode.CondSwizzleX = swizzle_x;
   dst_register_ext_concode.CondSwizzleY = swizzle_y;
   dst_register_ext_concode.CondSwizzleZ = swizzle_z;
   dst_register_ext_concode.CondSwizzleW = swizzle_w;
   dst_register_ext_concode.CondSrcIndex = index;

   prev_token->Extended = 1;
   instruction_grow( instruction, header );

   return dst_register_ext_concode;
}

struct tgsi_dst_register_ext_modulate
tgsi_default_dst_register_ext_modulate( void )
{
   struct tgsi_dst_register_ext_modulate dst_register_ext_modulate;

   dst_register_ext_modulate.Type = TGSI_DST_REGISTER_EXT_TYPE_MODULATE;
   dst_register_ext_modulate.Modulate = TGSI_MODULATE_1X;
   dst_register_ext_modulate.Padding = 0;
   dst_register_ext_modulate.Extended = 0;

   return dst_register_ext_modulate;
}

GLuint
tgsi_compare_dst_register_ext_modulate(
   struct tgsi_dst_register_ext_modulate a,
   struct tgsi_dst_register_ext_modulate b )
{
   a.Padding = b.Padding = 0;
   a.Extended = b.Extended = 0;
   return *(GLuint *) &a != *(GLuint *) &b;
}

struct tgsi_dst_register_ext_modulate
tgsi_build_dst_register_ext_modulate(
   GLuint modulate,
   struct tgsi_token  *prev_token,
   struct tgsi_instruction *instruction,
   struct tgsi_header *header )
{
   struct tgsi_dst_register_ext_modulate dst_register_ext_modulate;

   assert (modulate <=  TGSI_MODULATE_EIGHTH);

   dst_register_ext_modulate = tgsi_default_dst_register_ext_modulate();
   dst_register_ext_modulate.Modulate = modulate;

   prev_token->Extended = 1;
   instruction_grow( instruction, header );

   return dst_register_ext_modulate;
}

