/**************************************************************************
 * 
 * Copyright 2009 Younes Manton.
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

#include "vl_shader_build.h"
#include <assert.h>
#include <tgsi/tgsi_parse.h>
#include <tgsi/tgsi_build.h>

struct tgsi_full_declaration vl_decl_input(unsigned int name, unsigned int index, unsigned int first, unsigned int last)
{
   struct tgsi_full_declaration decl = tgsi_default_full_declaration();

   decl.Declaration.File = TGSI_FILE_INPUT;
   decl.Declaration.Semantic = 1;
   decl.Semantic.SemanticName = name;
   decl.Semantic.SemanticIndex = index;
   decl.DeclarationRange.First = first;
   decl.DeclarationRange.Last = last;

   return decl;
}

struct tgsi_full_declaration vl_decl_interpolated_input
(
   unsigned int name,
   unsigned int index,
   unsigned int first,
   unsigned int last,
   int interpolation
)
{
   struct tgsi_full_declaration decl = tgsi_default_full_declaration();

   assert
   (
      interpolation == TGSI_INTERPOLATE_CONSTANT ||
      interpolation == TGSI_INTERPOLATE_LINEAR ||
      interpolation == TGSI_INTERPOLATE_PERSPECTIVE
   );

   decl.Declaration.File = TGSI_FILE_INPUT;
   decl.Declaration.Semantic = 1;
   decl.Semantic.SemanticName = name;
   decl.Semantic.SemanticIndex = index;
   decl.Declaration.Interpolate = interpolation;;
   decl.DeclarationRange.First = first;
   decl.DeclarationRange.Last = last;

   return decl;
}

struct tgsi_full_declaration vl_decl_constants(unsigned int name, unsigned int index, unsigned int first, unsigned int last)
{
   struct tgsi_full_declaration decl = tgsi_default_full_declaration();

   decl.Declaration.File = TGSI_FILE_CONSTANT;
   decl.Declaration.Semantic = 1;
   decl.Semantic.SemanticName = name;
   decl.Semantic.SemanticIndex = index;
   decl.DeclarationRange.First = first;
   decl.DeclarationRange.Last = last;

   return decl;
}

struct tgsi_full_declaration vl_decl_output(unsigned int name, unsigned int index, unsigned int first, unsigned int last)
{
   struct tgsi_full_declaration decl = tgsi_default_full_declaration();

   decl.Declaration.File = TGSI_FILE_OUTPUT;
   decl.Declaration.Semantic = 1;
   decl.Semantic.SemanticName = name;
   decl.Semantic.SemanticIndex = index;
   decl.DeclarationRange.First = first;
   decl.DeclarationRange.Last = last;

   return decl;
}

struct tgsi_full_declaration vl_decl_temps(unsigned int first, unsigned int last)
{
   struct tgsi_full_declaration decl = tgsi_default_full_declaration();

   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_TEMPORARY;
   decl.DeclarationRange.First = first;
   decl.DeclarationRange.Last = last;

   return decl;
}

struct tgsi_full_declaration vl_decl_samplers(unsigned int first, unsigned int last)
{
   struct tgsi_full_declaration decl = tgsi_default_full_declaration();

   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_SAMPLER;
   decl.DeclarationRange.First = first;
   decl.DeclarationRange.Last = last;

   return decl;
}

struct tgsi_full_instruction vl_inst2
(
   int opcode,
   enum tgsi_file_type dst_file,
   unsigned int dst_index,
   enum tgsi_file_type src_file,
   unsigned int src_index
)
{
   struct tgsi_full_instruction inst = tgsi_default_full_instruction();

   inst.Instruction.Opcode = opcode;
   inst.Instruction.NumDstRegs = 1;
   inst.FullDstRegisters[0].DstRegister.File = dst_file;
   inst.FullDstRegisters[0].DstRegister.Index = dst_index;
   inst.Instruction.NumSrcRegs = 1;
   inst.FullSrcRegisters[0].SrcRegister.File = src_file;
   inst.FullSrcRegisters[0].SrcRegister.Index = src_index;

   return inst;
}

struct tgsi_full_instruction vl_inst3
(
   int opcode,
   enum tgsi_file_type dst_file,
   unsigned int dst_index,
   enum tgsi_file_type src1_file,
   unsigned int src1_index,
   enum tgsi_file_type src2_file,
   unsigned int src2_index
)
{
   struct tgsi_full_instruction inst = tgsi_default_full_instruction();

   inst.Instruction.Opcode = opcode;
   inst.Instruction.NumDstRegs = 1;
   inst.FullDstRegisters[0].DstRegister.File = dst_file;
   inst.FullDstRegisters[0].DstRegister.Index = dst_index;
   inst.Instruction.NumSrcRegs = 2;
   inst.FullSrcRegisters[0].SrcRegister.File = src1_file;
   inst.FullSrcRegisters[0].SrcRegister.Index = src1_index;
   inst.FullSrcRegisters[1].SrcRegister.File = src2_file;
   inst.FullSrcRegisters[1].SrcRegister.Index = src2_index;

   return inst;
}

struct tgsi_full_instruction vl_tex
(
   int tex,
   enum tgsi_file_type dst_file,
   unsigned int dst_index,
   enum tgsi_file_type src1_file,
   unsigned int src1_index,
   enum tgsi_file_type src2_file,
   unsigned int src2_index
)
{
   struct tgsi_full_instruction inst = tgsi_default_full_instruction();

   inst.Instruction.Opcode = TGSI_OPCODE_TEX;
   inst.Instruction.NumDstRegs = 1;
   inst.FullDstRegisters[0].DstRegister.File = dst_file;
   inst.FullDstRegisters[0].DstRegister.Index = dst_index;
   inst.Instruction.NumSrcRegs = 2;
   inst.InstructionExtTexture.Texture = tex;
   inst.FullSrcRegisters[0].SrcRegister.File = src1_file;
   inst.FullSrcRegisters[0].SrcRegister.Index = src1_index;
   inst.FullSrcRegisters[1].SrcRegister.File = src2_file;
   inst.FullSrcRegisters[1].SrcRegister.Index = src2_index;

   return inst;
}

struct tgsi_full_instruction vl_inst4
(
   int opcode,
   enum tgsi_file_type dst_file,
   unsigned int dst_index,
   enum tgsi_file_type src1_file,
   unsigned int src1_index,
   enum tgsi_file_type src2_file,
   unsigned int src2_index,
   enum tgsi_file_type src3_file,
   unsigned int src3_index
)
{
   struct tgsi_full_instruction inst = tgsi_default_full_instruction();

   inst.Instruction.Opcode = opcode;
   inst.Instruction.NumDstRegs = 1;
   inst.FullDstRegisters[0].DstRegister.File = dst_file;
   inst.FullDstRegisters[0].DstRegister.Index = dst_index;
   inst.Instruction.NumSrcRegs = 3;
   inst.FullSrcRegisters[0].SrcRegister.File = src1_file;
   inst.FullSrcRegisters[0].SrcRegister.Index = src1_index;
   inst.FullSrcRegisters[1].SrcRegister.File = src2_file;
   inst.FullSrcRegisters[1].SrcRegister.Index = src2_index;
   inst.FullSrcRegisters[2].SrcRegister.File = src3_file;
   inst.FullSrcRegisters[2].SrcRegister.Index = src3_index;

   return inst;
}

struct tgsi_full_instruction vl_end(void)
{
   struct tgsi_full_instruction inst = tgsi_default_full_instruction();

   inst.Instruction.Opcode = TGSI_OPCODE_END;
   inst.Instruction.NumDstRegs = 0;
   inst.Instruction.NumSrcRegs = 0;

   return inst;
}
