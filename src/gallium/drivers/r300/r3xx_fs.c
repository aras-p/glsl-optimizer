/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 *                Joakim Sindholt <opensource@zhasha.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "r3xx_fs.h"

static INLINE uint32_t r3xx_rgb_op(unsigned op)
{
    switch (op) {
        case TGSI_OPCODE_MOV:
            return R300_ALU_OUTC_CMP;
        default:
            return 0;
    }
}

static INLINE uint32_t r3xx_alpha_op(unsigned op)
{
    switch (op) {
        case TGSI_OPCODE_MOV:
            return R300_ALU_OUTA_CMP;
        default:
            return 0;
    }
}

static INLINE void r3xx_emit_maths(struct r3xx_fragment_shader* fs,
                                   struct r300_fs_asm* assembler,
                                   struct tgsi_full_src_register* src,
                                   struct tgsi_full_dst_register* dst,
                                   unsigned op,
                                   unsigned count)
{
    int i = fs->alu_instruction_count;

    fs->instructions[i].alu_rgb_inst = R300_RGB_SWIZA(R300_ALU_ARGC_SRC0C_XYZ) |
        R300_RGB_SWIZB(R300_ALU_ARGC_SRC0C_XYZ) |
        R300_RGB_SWIZC(R300_ALU_ARGC_ZERO) |
        r3xx_rgb_op(op);
    fs->instructions[i].alu_rgb_addr = R300_RGB_ADDR0(0) | R300_RGB_ADDR1(0) |
        R300_RGB_ADDR2(0) | R300_ALU_DSTC_OUTPUT_XYZ;
    fs->instructions[i].alu_alpha_inst = R300_ALPHA_SWIZA(R300_ALU_ARGA_SRC0A) |
        R300_ALPHA_SWIZB(R300_ALU_ARGA_SRC0A) |
        R300_ALPHA_SWIZC(R300_ALU_ARGA_ZERO) |
        r3xx_alpha_op(op);
    fs->instructions[i].alu_alpha_addr = R300_ALPHA_ADDR0(0) |
        R300_ALPHA_ADDR1(0) | R300_ALPHA_ADDR2(0) | R300_ALU_DSTA_OUTPUT;

    fs->alu_instruction_count++;
}

void r3xx_fs_finalize(struct r300_fragment_shader* fs,
                      struct r300_fs_asm* assembler)
{
    fs->stack_size = assembler->temp_count + assembler->temp_offset + 1;
}

void r3xx_fs_instruction(struct r3xx_fragment_shader* fs,
                         struct r300_fs_asm* assembler,
                         struct tgsi_full_instruction* inst)
{
    switch (inst->Instruction.Opcode) {
        case TGSI_OPCODE_MOV:
            /* src0 -> src1 and src2 forced to zero */
            inst->FullSrcRegisters[1] = inst->FullSrcRegisters[0];
            inst->FullSrcRegisters[2] = r300_constant_zero;
            r3xx_emit_maths(fs, assembler, inst->FullSrcRegisters,
                    &inst->FullDstRegisters[0], inst->Instruction.Opcode, 3);
            break;
        case TGSI_OPCODE_END:
            break;
        default:
            debug_printf("r300: fs: Bad opcode %d\n",
                    inst->Instruction.Opcode);
            break;
    }
}
