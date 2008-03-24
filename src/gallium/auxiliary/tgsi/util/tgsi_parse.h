#if !defined TGSI_PARSE_H
#define TGSI_PARSE_H

#if defined __cplusplus
extern "C" {
#endif

struct tgsi_full_version
{
   struct tgsi_version  Version;
};

struct tgsi_full_header
{
   struct tgsi_header      Header;
   struct tgsi_processor   Processor;
};

struct tgsi_full_dst_register
{
   struct tgsi_dst_register               DstRegister;
   struct tgsi_dst_register_ext_concode   DstRegisterExtConcode;
   struct tgsi_dst_register_ext_modulate  DstRegisterExtModulate;
};

struct tgsi_full_src_register
{
   struct tgsi_src_register         SrcRegister;
   struct tgsi_src_register_ext_swz SrcRegisterExtSwz;
   struct tgsi_src_register_ext_mod SrcRegisterExtMod;
   struct tgsi_src_register         SrcRegisterInd;
   struct tgsi_dimension            SrcRegisterDim;
   struct tgsi_src_register         SrcRegisterDimInd;
};

struct tgsi_full_declaration
{
   struct tgsi_declaration Declaration;
   union
   {
      struct tgsi_declaration_range DeclarationRange;
      struct tgsi_declaration_mask  DeclarationMask;
   } u;
   struct tgsi_declaration_interpolation  Interpolation;
   struct tgsi_declaration_semantic       Semantic;
};

struct tgsi_full_immediate
{
   struct tgsi_immediate   Immediate;
   union
   {
      void                          *Pointer;
      struct tgsi_immediate_float32 *ImmediateFloat32;
   } u;
};

#define TGSI_FULL_MAX_DST_REGISTERS 2
#define TGSI_FULL_MAX_SRC_REGISTERS 4 /* TXD has 4 */

struct tgsi_full_instruction
{
   struct tgsi_instruction             Instruction;
   struct tgsi_instruction_ext_nv      InstructionExtNv;
   struct tgsi_instruction_ext_label   InstructionExtLabel;
   struct tgsi_instruction_ext_texture InstructionExtTexture;
   struct tgsi_full_dst_register       FullDstRegisters[TGSI_FULL_MAX_DST_REGISTERS];
   struct tgsi_full_src_register       FullSrcRegisters[TGSI_FULL_MAX_SRC_REGISTERS];
};

union tgsi_full_token
{
   struct tgsi_token             Token;
   struct tgsi_full_declaration  FullDeclaration;
   struct tgsi_full_immediate    FullImmediate;
   struct tgsi_full_instruction  FullInstruction;
};

void
tgsi_full_token_init(
   union tgsi_full_token *full_token );

void
tgsi_full_token_free(
   union tgsi_full_token *full_token );

struct tgsi_parse_context
{
   const struct tgsi_token    *Tokens;
   unsigned                   Position;
   struct tgsi_full_version   FullVersion;
   struct tgsi_full_header    FullHeader;
   union tgsi_full_token      FullToken;
};

#define TGSI_PARSE_OK      0
#define TGSI_PARSE_ERROR   1

unsigned
tgsi_parse_init(
   struct tgsi_parse_context *ctx,
   const struct tgsi_token *tokens );

void
tgsi_parse_free(
   struct tgsi_parse_context *ctx );

boolean
tgsi_parse_end_of_tokens(
   struct tgsi_parse_context *ctx );

void
tgsi_parse_token(
   struct tgsi_parse_context *ctx );

unsigned
tgsi_num_tokens(const struct tgsi_token *tokens);


#if defined __cplusplus
}
#endif

#endif /* TGSI_PARSE_H */

