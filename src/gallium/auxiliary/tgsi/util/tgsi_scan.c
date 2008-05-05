/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/**
 * TGSI program scan utility.
 * Used to determine which registers and instructions are used by a shader.
 *
 * Authors:  Brian Paul
 */


#include "tgsi_scan.h"
#include "tgsi/util/tgsi_parse.h"
#include "tgsi/util/tgsi_build.h"

#include "pipe/p_util.h"



/**
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
            struct tgsi_full_instruction *fullinst
               = &parse.FullToken.FullInstruction;

            assert(fullinst->Instruction.Opcode < TGSI_OPCODE_LAST);
            info->opcode_count[fullinst->Instruction.Opcode]++;
         }
         break;

      case TGSI_TOKEN_TYPE_DECLARATION:
         {
            struct tgsi_full_declaration *fulldecl
               = &parse.FullToken.FullDeclaration;
            uint file = fulldecl->Declaration.File;
            uint i;
            for (i = fulldecl->u.DeclarationRange.First;
                 i <= fulldecl->u.DeclarationRange.Last;
                 i++) {

               /* only first 32 regs will appear in this bitfield */
               info->file_mask[file] |= (1 << i);
               info->file_count[file]++;
               info->file_max[file] = MAX2(info->file_max[file], (int)i);

               if (file == TGSI_FILE_INPUT) {
                  info->input_semantic_name[i] = (ubyte)fulldecl->Semantic.SemanticName;
                  info->input_semantic_index[i] = (ubyte)fulldecl->Semantic.SemanticIndex;
                  info->num_inputs++;
               }

               if (file == TGSI_FILE_OUTPUT) {
                  info->output_semantic_name[i] = (ubyte)fulldecl->Semantic.SemanticName;
                  info->output_semantic_index[i] = (ubyte)fulldecl->Semantic.SemanticIndex;
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
         info->immediate_count++;
         break;

      default:
         assert( 0 );
      }
   }

   assert( info->file_max[TGSI_FILE_INPUT] + 1 == info->num_inputs );
   assert( info->file_max[TGSI_FILE_OUTPUT] + 1 == info->num_outputs );

   info->uses_kill = (info->opcode_count[TGSI_OPCODE_KIL] ||
                      info->opcode_count[TGSI_OPCODE_KILP]);

   tgsi_parse_free (&parse);
}
