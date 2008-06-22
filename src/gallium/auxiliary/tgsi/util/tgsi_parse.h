/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#ifndef TGSI_PARSE_H
#define TGSI_PARSE_H

#include "pipe/p_shader_tokens.h"

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
   struct tgsi_declaration_range DeclarationRange;
   struct tgsi_declaration_semantic Semantic;
};

struct tgsi_full_immediate
{
   struct tgsi_immediate   Immediate;
   union
   {
      const void                          *Pointer;
      const struct tgsi_immediate_float32 *ImmediateFloat32;
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

struct tgsi_token *
tgsi_dup_tokens(const struct tgsi_token *tokens);

#if defined __cplusplus
}
#endif

#endif /* TGSI_PARSE_H */

