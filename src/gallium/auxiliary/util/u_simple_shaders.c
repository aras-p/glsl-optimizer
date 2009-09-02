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
 * @file
 * Simple vertex/fragment shader generators.
 *  
 * @author Brian Paul
 */


#include "pipe/p_context.h"
#include "util/u_debug.h"
#include "pipe/p_defines.h"
#include "pipe/p_screen.h"
#include "pipe/p_shader_tokens.h"

#include "util/u_memory.h"
#include "util/u_simple_shaders.h"

#include "tgsi/tgsi_build.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_parse.h"



/**
 * Make simple vertex pass-through shader.
 */
void *
util_make_vertex_passthrough_shader(struct pipe_context *pipe,
                                    uint num_attribs,
                                    const uint *semantic_names,
                                    const uint *semantic_indexes)
                                    
{
   struct pipe_shader_state shader;
   struct tgsi_token tokens[100];
   struct tgsi_header *header;
   struct tgsi_processor *processor;
   struct tgsi_full_declaration decl;
   struct tgsi_full_instruction inst;
   const uint procType = TGSI_PROCESSOR_VERTEX;
   uint ti, i;

   /* shader header
    */
   *(struct tgsi_version *) &tokens[0] = tgsi_build_version();

   header = (struct tgsi_header *) &tokens[1];
   *header = tgsi_build_header();

   processor = (struct tgsi_processor *) &tokens[2];
   *processor = tgsi_build_processor( procType, header );

   ti = 3;

   /* declare inputs */
   for (i = 0; i < num_attribs; i++) {
      decl = tgsi_default_full_declaration();
      decl.Declaration.File = TGSI_FILE_INPUT;

      decl.Declaration.Semantic = 1;
      decl.Semantic.SemanticName = semantic_names[i];
      decl.Semantic.SemanticIndex = semantic_indexes[i];

      decl.DeclarationRange.First = 
      decl.DeclarationRange.Last = i;
      ti += tgsi_build_full_declaration(&decl,
                                        &tokens[ti],
                                        header,
                                        Elements(tokens) - ti);
   }

   /* declare outputs */
   for (i = 0; i < num_attribs; i++) {
      decl = tgsi_default_full_declaration();
      decl.Declaration.File = TGSI_FILE_OUTPUT;
      decl.Declaration.Semantic = 1;
      decl.Semantic.SemanticName = semantic_names[i];
      decl.Semantic.SemanticIndex = semantic_indexes[i];
      decl.DeclarationRange.First = 
         decl.DeclarationRange.Last = i;
      ti += tgsi_build_full_declaration(&decl,
                                        &tokens[ti],
                                        header,
                                        Elements(tokens) - ti);
   }

   /* emit MOV instructions */
   for (i = 0; i < num_attribs; i++) {
      /* MOVE out[i], in[i]; */
      inst = tgsi_default_full_instruction();
      inst.Instruction.Opcode = TGSI_OPCODE_MOV;
      inst.Instruction.NumDstRegs = 1;
      inst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_OUTPUT;
      inst.FullDstRegisters[0].DstRegister.Index = i;
      inst.Instruction.NumSrcRegs = 1;
      inst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_INPUT;
      inst.FullSrcRegisters[0].SrcRegister.Index = i;
      ti += tgsi_build_full_instruction(&inst,
                                        &tokens[ti],
                                        header,
                                        Elements(tokens) - ti );
   }

   /* END instruction */
   inst = tgsi_default_full_instruction();
   inst.Instruction.Opcode = TGSI_OPCODE_END;
   inst.Instruction.NumDstRegs = 0;
   inst.Instruction.NumSrcRegs = 0;
   ti += tgsi_build_full_instruction(&inst,
                                     &tokens[ti],
                                     header,
                                     Elements(tokens) - ti );

#if 0 /*debug*/
   tgsi_dump(tokens, 0);
#endif

   shader.tokens = tokens;

   return pipe->create_vs_state(pipe, &shader);
}




/**
 * Make simple fragment texture shader:
 *  IMM {0,0,0,1}                         // (if writemask != 0xf)
 *  MOV OUT[0], IMM[0]                    // (if writemask != 0xf)
 *  TEX OUT[0].writemask, IN[0], SAMP[0], 2D;
 *  END;
 */
void *
util_make_fragment_tex_shader_writemask(struct pipe_context *pipe,
                                        unsigned writemask )
{
   struct pipe_shader_state shader;
   struct tgsi_token tokens[100];
   struct tgsi_header *header;
   struct tgsi_processor *processor;
   struct tgsi_full_declaration decl;
   struct tgsi_full_instruction inst;
   const uint procType = TGSI_PROCESSOR_FRAGMENT;
   uint ti;

   /* shader header
    */
   *(struct tgsi_version *) &tokens[0] = tgsi_build_version();

   header = (struct tgsi_header *) &tokens[1];
   *header = tgsi_build_header();

   processor = (struct tgsi_processor *) &tokens[2];
   *processor = tgsi_build_processor( procType, header );

   ti = 3;

   /* declare TEX[0] input */
   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_INPUT;
   /* XXX this could be linear... */
   decl.Declaration.Interpolate = TGSI_INTERPOLATE_PERSPECTIVE;
   decl.Declaration.Semantic = 1;
   decl.Semantic.SemanticName = TGSI_SEMANTIC_GENERIC;
   decl.Semantic.SemanticIndex = 0;
   decl.DeclarationRange.First = 
   decl.DeclarationRange.Last = 0;
   ti += tgsi_build_full_declaration(&decl,
                                     &tokens[ti],
                                     header,
                                     Elements(tokens) - ti);

   /* declare color[0] output */
   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_OUTPUT;
   decl.Declaration.Semantic = 1;
   decl.Semantic.SemanticName = TGSI_SEMANTIC_COLOR;
   decl.Semantic.SemanticIndex = 0;
   decl.DeclarationRange.First = 
   decl.DeclarationRange.Last = 0;
   ti += tgsi_build_full_declaration(&decl,
                                     &tokens[ti],
                                     header,
                                     Elements(tokens) - ti);

   /* declare sampler */
   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_SAMPLER;
   decl.DeclarationRange.First = 
   decl.DeclarationRange.Last = 0;
   ti += tgsi_build_full_declaration(&decl,
                                     &tokens[ti],
                                     header,
                                     Elements(tokens) - ti);


   if (writemask != TGSI_WRITEMASK_XYZW) {
      struct tgsi_full_immediate imm;
      static const float value[4] = { 0, 0, 0, 1 };

      imm = tgsi_default_full_immediate();
      imm.Immediate.NrTokens += 4;
      imm.Immediate.DataType = TGSI_IMM_FLOAT32;
      imm.u.Pointer = value;

      ti += tgsi_build_full_immediate(&imm,
                                      &tokens[ti],
                                      header,
                                      Elements(tokens) - ti );

      /* MOV instruction */
      inst = tgsi_default_full_instruction();
      inst.Instruction.Opcode = TGSI_OPCODE_MOV;
      inst.Instruction.NumDstRegs = 1;
      inst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_OUTPUT;
      inst.FullDstRegisters[0].DstRegister.Index = 0;
      inst.Instruction.NumSrcRegs = 1;
      inst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_IMMEDIATE;
      inst.FullSrcRegisters[0].SrcRegister.Index = 0;
      ti += tgsi_build_full_instruction(&inst,
                                        &tokens[ti],
                                        header,
                                        Elements(tokens) - ti );
   }

   /* TEX instruction */
   inst = tgsi_default_full_instruction();
   inst.Instruction.Opcode = TGSI_OPCODE_TEX;
   inst.Instruction.NumDstRegs = 1;
   inst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_OUTPUT;
   inst.FullDstRegisters[0].DstRegister.Index = 0;
   inst.FullDstRegisters[0].DstRegister.WriteMask = writemask;
   inst.Instruction.NumSrcRegs = 2;
   inst.InstructionExtTexture.Texture = TGSI_TEXTURE_2D;
   inst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_INPUT;
   inst.FullSrcRegisters[0].SrcRegister.Index = 0;
   inst.FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_SAMPLER;
   inst.FullSrcRegisters[1].SrcRegister.Index = 0;
   ti += tgsi_build_full_instruction(&inst,
                                     &tokens[ti],
                                     header,
                                     Elements(tokens) - ti );

   /* END instruction */
   inst = tgsi_default_full_instruction();
   inst.Instruction.Opcode = TGSI_OPCODE_END;
   inst.Instruction.NumDstRegs = 0;
   inst.Instruction.NumSrcRegs = 0;
   ti += tgsi_build_full_instruction(&inst,
                                     &tokens[ti],
                                     header,
                                     Elements(tokens) - ti );

#if 0 /*debug*/
   tgsi_dump(tokens, 0);
#endif

   shader.tokens = tokens;

   return pipe->create_fs_state(pipe, &shader);
}

void *
util_make_fragment_tex_shader(struct pipe_context *pipe )
{
   return util_make_fragment_tex_shader_writemask( pipe,
                                                   TGSI_WRITEMASK_XYZW );
}





/**
 * Make simple fragment color pass-through shader.
 */
void *
util_make_fragment_passthrough_shader(struct pipe_context *pipe)
{
   struct pipe_shader_state shader;
   struct tgsi_token tokens[40];
   struct tgsi_header *header;
   struct tgsi_processor *processor;
   struct tgsi_full_declaration decl;
   struct tgsi_full_instruction inst;
   const uint procType = TGSI_PROCESSOR_FRAGMENT;
   uint ti;

   /* shader header
    */
   *(struct tgsi_version *) &tokens[0] = tgsi_build_version();

   header = (struct tgsi_header *) &tokens[1];
   *header = tgsi_build_header();

   processor = (struct tgsi_processor *) &tokens[2];
   *processor = tgsi_build_processor( procType, header );

   ti = 3;

   /* declare input */
   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_INPUT;
   decl.Declaration.Semantic = 1;
   decl.Semantic.SemanticName = TGSI_SEMANTIC_COLOR;
   decl.Semantic.SemanticIndex = 0;
   decl.DeclarationRange.First = 
      decl.DeclarationRange.Last = 0;
   ti += tgsi_build_full_declaration(&decl,
                                     &tokens[ti],
                                     header,
                                     Elements(tokens) - ti);

   /* declare output */
   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_OUTPUT;
   decl.Declaration.Semantic = 1;
   decl.Semantic.SemanticName = TGSI_SEMANTIC_COLOR;
   decl.Semantic.SemanticIndex = 0;
   decl.DeclarationRange.First = 
      decl.DeclarationRange.Last = 0;
   ti += tgsi_build_full_declaration(&decl,
                                     &tokens[ti],
                                     header,
                                     Elements(tokens) - ti);


   /* MOVE out[0], in[0]; */
   inst = tgsi_default_full_instruction();
   inst.Instruction.Opcode = TGSI_OPCODE_MOV;
   inst.Instruction.NumDstRegs = 1;
   inst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_OUTPUT;
   inst.FullDstRegisters[0].DstRegister.Index = 0;
   inst.Instruction.NumSrcRegs = 1;
   inst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_INPUT;
   inst.FullSrcRegisters[0].SrcRegister.Index = 0;
   ti += tgsi_build_full_instruction(&inst,
                                     &tokens[ti],
                                     header,
                                     Elements(tokens) - ti );

   /* END instruction */
   inst = tgsi_default_full_instruction();
   inst.Instruction.Opcode = TGSI_OPCODE_END;
   inst.Instruction.NumDstRegs = 0;
   inst.Instruction.NumSrcRegs = 0;
   ti += tgsi_build_full_instruction(&inst,
                                     &tokens[ti],
                                     header,
                                     Elements(tokens) - ti );

   assert(ti < Elements(tokens));

#if 0 /*debug*/
   tgsi_dump(tokens, 0);
#endif

   shader.tokens = tokens;

   return pipe->create_fs_state(pipe, &shader);
}


