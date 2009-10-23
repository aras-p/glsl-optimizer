/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * Copyright 2008 VMware, Inc.  All rights Reserved.
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

/**
 * TGSI program scan utility.
 * Used to determine which registers and instructions are used by a shader.
 *
 * Authors:  Brian Paul
 */


#include "util/u_math.h"
#include "tgsi/tgsi_build.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_scan.h"




/**
 * Scan the given TGSI shader to collect information such as number of
 * registers used, special instructions used, etc.
 * \return info  the result of the scan
 */
void
tgsi_scan_shader(const struct tgsi_token *tokens,
                 struct tgsi_shader_info *info)
{
   uint procType, i;
   struct tgsi_parse_context parse;

   memset(info, 0, sizeof(*info));
   for (i = 0; i < TGSI_FILE_COUNT; i++)
      info->file_max[i] = -1;

   /**
    ** Setup to begin parsing input shader
    **/
   if (tgsi_parse_init( &parse, tokens ) != TGSI_PARSE_OK) {
      debug_printf("tgsi_parse_init() failed in tgsi_scan_shader()!\n");
      return;
   }
   procType = parse.FullHeader.Processor.Processor;
   assert(procType == TGSI_PROCESSOR_FRAGMENT ||
          procType == TGSI_PROCESSOR_VERTEX ||
          procType == TGSI_PROCESSOR_GEOMETRY);


   /**
    ** Loop over incoming program tokens/instructions
    */
   while( !tgsi_parse_end_of_tokens( &parse ) ) {

      info->num_tokens++;

      tgsi_parse_token( &parse );

      switch( parse.FullToken.Token.Type ) {
      case TGSI_TOKEN_TYPE_INSTRUCTION:
         {
            const struct tgsi_full_instruction *fullinst
               = &parse.FullToken.FullInstruction;

            assert(fullinst->Instruction.Opcode < TGSI_OPCODE_LAST);
            info->opcode_count[fullinst->Instruction.Opcode]++;

            /* special case: scan fragment shaders for use of the fog
             * input/attribute.  The X component is fog, the Y component
             * is the front/back-face flag.
             */
            if (procType == TGSI_PROCESSOR_FRAGMENT) {
               uint i;
               for (i = 0; i < fullinst->Instruction.NumSrcRegs; i++) {
                  const struct tgsi_full_src_register *src =
                     &fullinst->FullSrcRegisters[i];
                  if (src->SrcRegister.File == TGSI_FILE_INPUT) {
                     const int ind = src->SrcRegister.Index;
                     if (info->input_semantic_name[ind] == TGSI_SEMANTIC_FOG) {
                        if (src->SrcRegister.SwizzleX == TGSI_SWIZZLE_X) {
                           info->uses_fogcoord = TRUE;
                        }
                        else if (src->SrcRegister.SwizzleX == TGSI_SWIZZLE_Y) {
                           info->uses_frontfacing = TRUE;
                        }
                     }
                  }
               }
            }
         }
         break;

      case TGSI_TOKEN_TYPE_DECLARATION:
         {
            const struct tgsi_full_declaration *fulldecl
               = &parse.FullToken.FullDeclaration;
            const uint file = fulldecl->Declaration.File;
            uint reg;
            for (reg = fulldecl->DeclarationRange.First;
                 reg <= fulldecl->DeclarationRange.Last;
                 reg++) {

               /* only first 32 regs will appear in this bitfield */
               info->file_mask[file] |= (1 << reg);
               info->file_count[file]++;
               info->file_max[file] = MAX2(info->file_max[file], (int)reg);

               if (file == TGSI_FILE_INPUT) {
                  info->input_semantic_name[reg] = (ubyte)fulldecl->Semantic.SemanticName;
                  info->input_semantic_index[reg] = (ubyte)fulldecl->Semantic.SemanticIndex;
                  info->input_interpolate[reg] = (ubyte)fulldecl->Declaration.Interpolate;
                  info->num_inputs++;
               }
               else if (file == TGSI_FILE_OUTPUT) {
                  info->output_semantic_name[reg] = (ubyte)fulldecl->Semantic.SemanticName;
                  info->output_semantic_index[reg] = (ubyte)fulldecl->Semantic.SemanticIndex;
                  info->num_outputs++;
               }

               /* special case */
               if (procType == TGSI_PROCESSOR_FRAGMENT &&
                   file == TGSI_FILE_OUTPUT &&
                   fulldecl->Semantic.SemanticName == TGSI_SEMANTIC_POSITION) {
                  info->writes_z = TRUE;
               }
            }
         }
         break;

      case TGSI_TOKEN_TYPE_IMMEDIATE:
         {
            uint reg = info->immediate_count++;
            uint file = TGSI_FILE_IMMEDIATE;

            info->file_mask[file] |= (1 << reg);
            info->file_count[file]++;
            info->file_max[file] = MAX2(info->file_max[file], (int)reg);
         }
         break;

      default:
         assert( 0 );
      }
   }

   info->uses_kill = (info->opcode_count[TGSI_OPCODE_KIL] ||
                      info->opcode_count[TGSI_OPCODE_KILP]);

   tgsi_parse_free (&parse);
}



/**
 * Check if the given shader is a "passthrough" shader consisting of only
 * MOV instructions of the form:  MOV OUT[n], IN[n]
 *  
 */
boolean
tgsi_is_passthrough_shader(const struct tgsi_token *tokens)
{
   struct tgsi_parse_context parse;

   /**
    ** Setup to begin parsing input shader
    **/
   if (tgsi_parse_init(&parse, tokens) != TGSI_PARSE_OK) {
      debug_printf("tgsi_parse_init() failed in tgsi_is_passthrough_shader()!\n");
      return FALSE;
   }

   /**
    ** Loop over incoming program tokens/instructions
    */
   while (!tgsi_parse_end_of_tokens(&parse)) {

      tgsi_parse_token(&parse);

      switch (parse.FullToken.Token.Type) {
      case TGSI_TOKEN_TYPE_INSTRUCTION:
         {
            struct tgsi_full_instruction *fullinst =
               &parse.FullToken.FullInstruction;
            const struct tgsi_full_src_register *src =
               &fullinst->FullSrcRegisters[0];
            const struct tgsi_full_dst_register *dst =
               &fullinst->FullDstRegisters[0];

            /* Do a whole bunch of checks for a simple move */
            if (fullinst->Instruction.Opcode != TGSI_OPCODE_MOV ||
                src->SrcRegister.File != TGSI_FILE_INPUT ||
                dst->DstRegister.File != TGSI_FILE_OUTPUT ||
                src->SrcRegister.Index != dst->DstRegister.Index ||

                src->SrcRegister.Negate ||
                src->SrcRegisterExtMod.Negate ||
                src->SrcRegisterExtMod.Absolute ||
                src->SrcRegisterExtMod.Scale2X ||
                src->SrcRegisterExtMod.Bias ||
                src->SrcRegisterExtMod.Complement ||

                src->SrcRegister.SwizzleX != TGSI_SWIZZLE_X ||
                src->SrcRegister.SwizzleY != TGSI_SWIZZLE_Y ||
                src->SrcRegister.SwizzleZ != TGSI_SWIZZLE_Z ||
                src->SrcRegister.SwizzleW != TGSI_SWIZZLE_W ||

                dst->DstRegister.WriteMask != TGSI_WRITEMASK_XYZW)
            {
               tgsi_parse_free(&parse);
               return FALSE;
            }
         }
         break;

      case TGSI_TOKEN_TYPE_DECLARATION:
         /* fall-through */
      case TGSI_TOKEN_TYPE_IMMEDIATE:
         /* fall-through */
      default:
         ; /* no-op */
      }
   }

   tgsi_parse_free(&parse);

   /* if we get here, it's a pass-through shader */
   return TRUE;
}
