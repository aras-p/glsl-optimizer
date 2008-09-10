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

#include "pipe/p_debug.h"
#include "pipe/p_shader_tokens.h"
#include "tgsi_parse.h"
#include "tgsi_build.h"
#include "util/u_memory.h"

void
tgsi_full_token_init(
   union tgsi_full_token *full_token )
{
   full_token->Token.Type = TGSI_TOKEN_TYPE_DECLARATION;
}

void
tgsi_full_token_free(
   union tgsi_full_token *full_token )
{
   if( full_token->Token.Type == TGSI_TOKEN_TYPE_IMMEDIATE ) {
      FREE( (void *) full_token->FullImmediate.u.Pointer );
   }
}

unsigned
tgsi_parse_init(
   struct tgsi_parse_context *ctx,
   const struct tgsi_token *tokens )
{
   ctx->FullVersion.Version = *(struct tgsi_version *) &tokens[0];
   if( ctx->FullVersion.Version.MajorVersion > 1 ) {
      return TGSI_PARSE_ERROR;
   }

   ctx->FullHeader.Header = *(struct tgsi_header *) &tokens[1];
   if( ctx->FullHeader.Header.HeaderSize >= 2 ) {
      ctx->FullHeader.Processor = *(struct tgsi_processor *) &tokens[2];
   }
   else {
      ctx->FullHeader.Processor = tgsi_default_processor();
   }

   ctx->Tokens = tokens;
   ctx->Position = 1 + ctx->FullHeader.Header.HeaderSize;

   tgsi_full_token_init( &ctx->FullToken );

   return TGSI_PARSE_OK;
}

void
tgsi_parse_free(
   struct tgsi_parse_context *ctx )
{
   tgsi_full_token_free( &ctx->FullToken );
}

boolean
tgsi_parse_end_of_tokens(
   struct tgsi_parse_context *ctx )
{
   return ctx->Position >=
      1 + ctx->FullHeader.Header.HeaderSize + ctx->FullHeader.Header.BodySize;
}

static void
next_token(
   struct tgsi_parse_context *ctx,
   void *token )
{
   assert( !tgsi_parse_end_of_tokens( ctx ) );

   *(struct tgsi_token *) token = ctx->Tokens[ctx->Position++];
}

void
tgsi_parse_token(
   struct tgsi_parse_context *ctx )
{
   struct tgsi_token token;
   unsigned i;

   tgsi_full_token_free( &ctx->FullToken );
   tgsi_full_token_init( &ctx->FullToken );

   next_token( ctx, &token );

   switch( token.Type ) {
   case TGSI_TOKEN_TYPE_DECLARATION:
   {
      struct tgsi_full_declaration *decl = &ctx->FullToken.FullDeclaration;

      *decl = tgsi_default_full_declaration();
      decl->Declaration = *(struct tgsi_declaration *) &token;

      next_token( ctx, &decl->DeclarationRange );

      if( decl->Declaration.Semantic ) {
         next_token( ctx, &decl->Semantic );
      }

      break;
   }

   case TGSI_TOKEN_TYPE_IMMEDIATE:
   {
      struct tgsi_full_immediate *imm = &ctx->FullToken.FullImmediate;

      *imm = tgsi_default_full_immediate();
      imm->Immediate = *(struct tgsi_immediate *) &token;

      assert( !imm->Immediate.Extended );

      switch (imm->Immediate.DataType) {
      case TGSI_IMM_FLOAT32:
         imm->u.Pointer = MALLOC(
            sizeof( struct tgsi_immediate_float32 ) * (imm->Immediate.Size - 1) );
         for( i = 0; i < imm->Immediate.Size - 1; i++ ) {
            next_token( ctx, (struct tgsi_immediate_float32 *) &imm->u.ImmediateFloat32[i] );
         }
         break;

      default:
         assert( 0 );
      }

      break;
   }

   case TGSI_TOKEN_TYPE_INSTRUCTION:
   {
      struct tgsi_full_instruction *inst = &ctx->FullToken.FullInstruction;
      unsigned extended;

      *inst = tgsi_default_full_instruction();
      inst->Instruction = *(struct tgsi_instruction *) &token;

      extended = inst->Instruction.Extended;

      while( extended ) {
         struct tgsi_src_register_ext token;

         next_token( ctx, &token );

         switch( token.Type ) {
         case TGSI_INSTRUCTION_EXT_TYPE_NV:
            inst->InstructionExtNv =
               *(struct tgsi_instruction_ext_nv *) &token;
            break;

         case TGSI_INSTRUCTION_EXT_TYPE_LABEL:
            inst->InstructionExtLabel =
               *(struct tgsi_instruction_ext_label *) &token;
            break;

         case TGSI_INSTRUCTION_EXT_TYPE_TEXTURE:
            inst->InstructionExtTexture =
               *(struct tgsi_instruction_ext_texture *) &token;
            break;

         default:
            assert( 0 );
         }

         extended = token.Extended;
      }

      assert( inst->Instruction.NumDstRegs <= TGSI_FULL_MAX_DST_REGISTERS );

      for(  i = 0; i < inst->Instruction.NumDstRegs; i++ ) {
         unsigned extended;

         next_token( ctx, &inst->FullDstRegisters[i].DstRegister );

         /*
          * No support for indirect or multi-dimensional addressing.
          */
         assert( !inst->FullDstRegisters[i].DstRegister.Indirect );
         assert( !inst->FullDstRegisters[i].DstRegister.Dimension );

         extended = inst->FullDstRegisters[i].DstRegister.Extended;

         while( extended ) {
            struct tgsi_src_register_ext token;

            next_token( ctx, &token );

            switch( token.Type ) {
            case TGSI_DST_REGISTER_EXT_TYPE_CONDCODE:
               inst->FullDstRegisters[i].DstRegisterExtConcode =
                  *(struct tgsi_dst_register_ext_concode *) &token;
               break;

            case TGSI_DST_REGISTER_EXT_TYPE_MODULATE:
               inst->FullDstRegisters[i].DstRegisterExtModulate =
                  *(struct tgsi_dst_register_ext_modulate *) &token;
               break;

            default:
               assert( 0 );
            }

            extended = token.Extended;
         }
      }

      assert( inst->Instruction.NumSrcRegs <= TGSI_FULL_MAX_SRC_REGISTERS );

      for( i = 0; i < inst->Instruction.NumSrcRegs; i++ ) {
         unsigned extended;

         next_token( ctx, &inst->FullSrcRegisters[i].SrcRegister );

         extended = inst->FullSrcRegisters[i].SrcRegister.Extended;

         while( extended ) {
            struct tgsi_src_register_ext token;

            next_token( ctx, &token );

            switch( token.Type ) {
            case TGSI_SRC_REGISTER_EXT_TYPE_SWZ:
               inst->FullSrcRegisters[i].SrcRegisterExtSwz =
                  *(struct tgsi_src_register_ext_swz *) &token;
               break;

            case TGSI_SRC_REGISTER_EXT_TYPE_MOD:
               inst->FullSrcRegisters[i].SrcRegisterExtMod =
                  *(struct tgsi_src_register_ext_mod *) &token;
               break;

            default:
               assert( 0 );
            }

            extended = token.Extended;
         }

         if( inst->FullSrcRegisters[i].SrcRegister.Indirect ) {
            next_token( ctx, &inst->FullSrcRegisters[i].SrcRegisterInd );

            /*
             * No support for indirect or multi-dimensional addressing.
             */
            assert( !inst->FullSrcRegisters[i].SrcRegisterInd.Indirect );
            assert( !inst->FullSrcRegisters[i].SrcRegisterInd.Dimension );
            assert( !inst->FullSrcRegisters[i].SrcRegisterInd.Extended );
         }

         if( inst->FullSrcRegisters[i].SrcRegister.Dimension ) {
            next_token( ctx, &inst->FullSrcRegisters[i].SrcRegisterDim );

            /*
             * No support for multi-dimensional addressing.
             */
            assert( !inst->FullSrcRegisters[i].SrcRegisterDim.Dimension );
            assert( !inst->FullSrcRegisters[i].SrcRegisterDim.Extended );

            if( inst->FullSrcRegisters[i].SrcRegisterDim.Indirect ) {
               next_token( ctx, &inst->FullSrcRegisters[i].SrcRegisterDimInd );

               /*
               * No support for indirect or multi-dimensional addressing.
               */
               assert( !inst->FullSrcRegisters[i].SrcRegisterInd.Indirect );
               assert( !inst->FullSrcRegisters[i].SrcRegisterInd.Dimension );
               assert( !inst->FullSrcRegisters[i].SrcRegisterInd.Extended );
            }
         }
      }

      break;
   }

   default:
      assert( 0 );
   }
}


unsigned
tgsi_num_tokens(const struct tgsi_token *tokens)
{
   struct tgsi_parse_context ctx;
   if (tgsi_parse_init(&ctx, tokens) == TGSI_PARSE_OK) {
      unsigned len = (ctx.FullHeader.Header.HeaderSize +
                      ctx.FullHeader.Header.BodySize +
                      1);
      return len;
   }
   return 0;
}


/**
 * Make a new copy of a token array.
 */
struct tgsi_token *
tgsi_dup_tokens(const struct tgsi_token *tokens)
{
   unsigned n = tgsi_num_tokens(tokens);
   unsigned bytes = n * sizeof(struct tgsi_token);
   struct tgsi_token *new_tokens = (struct tgsi_token *) MALLOC(bytes);
   if (new_tokens)
      memcpy(new_tokens, tokens, bytes);
   return new_tokens;
}
