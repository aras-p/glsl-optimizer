/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
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

#include "r300_state_shader.h"

static void r300_copy_passthrough_shader(struct r300_fragment_shader* fs)
{
    struct r300_fragment_shader* pt = &r300_passthrough_fragment_shader;
    fs->shader.stack_size = pt->shader.stack_size;
    fs->alu_instruction_count = pt->alu_instruction_count;
    fs->tex_instruction_count = pt->tex_instruction_count;
    fs->indirections = pt->indirections;
    fs->instructions[0] = pt->instructions[0];
}

static void r500_copy_passthrough_shader(struct r500_fragment_shader* fs)
{
    struct r500_fragment_shader* pt = &r500_passthrough_fragment_shader;
    fs->shader.stack_size = pt->shader.stack_size;
    fs->instruction_count = pt->instruction_count;
    fs->instructions[0] = pt->instructions[0];
}

static void r300_fs_declare(struct r300_fs_asm* assembler,
                            struct tgsi_full_declaration* decl)
{
    switch (decl->Declaration.File) {
        case TGSI_FILE_INPUT:
            switch (decl->Semantic.SemanticName) {
                case TGSI_SEMANTIC_COLOR:
                    assembler->color_count++;
                    break;
                case TGSI_SEMANTIC_GENERIC:
                    assembler->tex_count++;
                    break;
                default:
                    debug_printf("r300: fs: Bad semantic declaration %d\n",
                        decl->Semantic.SemanticName);
                    break;
            }
            break;
        case TGSI_FILE_OUTPUT:
        case TGSI_FILE_CONSTANT:
            break;
        case TGSI_FILE_TEMPORARY:
            assembler->temp_count++;
            break;
        default:
            debug_printf("r300: fs: Bad file %d\n", decl->Declaration.File);
            break;
    }

    assembler->temp_offset = assembler->color_count + assembler->tex_count;
}

static INLINE unsigned r300_fs_src(struct r300_fs_asm* assembler,
                                   struct tgsi_src_register* src)
{
    switch (src->File) {
        case TGSI_FILE_NULL:
            return 0;
        case TGSI_FILE_INPUT:
            /* XXX may be wrong */
            return src->Index;
            break;
        case TGSI_FILE_TEMPORARY:
            return src->Index + assembler->temp_offset;
            break;
        case TGSI_FILE_IMMEDIATE:
            return (src->Index + assembler->imm_offset) | (1 << 8);
            break;
        case TGSI_FILE_CONSTANT:
            /* XXX magic */
            return src->Index | (1 << 8);
            break;
        default:
            debug_printf("r300: fs: Unimplemented src %d\n", src->File);
            break;
    }
    return 0;
}

static INLINE unsigned r300_fs_dst(struct r300_fs_asm* assembler,
                                   struct tgsi_dst_register* dst)
{
    switch (dst->File) {
        case TGSI_FILE_NULL:
            /* This happens during KIL instructions. */
            return 0;
            break;
        case TGSI_FILE_OUTPUT:
            return 0;
            break;
        case TGSI_FILE_TEMPORARY:
            return dst->Index + assembler->temp_offset;
            break;
        default:
            debug_printf("r300: fs: Unimplemented dst %d\n", dst->File);
            break;
    }
    return 0;
}

static INLINE unsigned r500_fix_swiz(unsigned s)
{
    /* For historical reasons, the swizzle values x, y, z, w, and 0 are
     * equivalent to the actual machine code, but 1 is not. Thus, we just
     * adjust it a bit... */
    if (s == TGSI_EXTSWIZZLE_ONE) {
        return R500_SWIZZLE_ONE;
    } else {
        return s;
    }
}

static uint32_t r500_rgba_swiz(struct tgsi_full_src_register* reg)
{
    if (reg->SrcRegister.Extended) {
        return r500_fix_swiz(reg->SrcRegisterExtSwz.ExtSwizzleX) |
            (r500_fix_swiz(reg->SrcRegisterExtSwz.ExtSwizzleY) << 3) |
            (r500_fix_swiz(reg->SrcRegisterExtSwz.ExtSwizzleZ) << 6) |
            (r500_fix_swiz(reg->SrcRegisterExtSwz.ExtSwizzleW) << 9);
    } else {
        return reg->SrcRegister.SwizzleX |
            (reg->SrcRegister.SwizzleY << 3) |
            (reg->SrcRegister.SwizzleZ << 6) |
            (reg->SrcRegister.SwizzleW << 9);
    }
}

static uint32_t r500_strq_swiz(struct tgsi_full_src_register* reg)
{
    return reg->SrcRegister.SwizzleX |
        (reg->SrcRegister.SwizzleY << 2) |
        (reg->SrcRegister.SwizzleZ << 4) |
        (reg->SrcRegister.SwizzleW << 6);
}

static INLINE uint32_t r500_rgb_swiz(struct tgsi_full_src_register* reg)
{
    /* Only the first 9 bits... */
    return (r500_rgba_swiz(reg) & 0x1ff) |
        (reg->SrcRegister.Negate ? (1 << 9) : 0) |
        (reg->SrcRegisterExtMod.Absolute ? (1 << 10) : 0);
}

static INLINE uint32_t r500_alpha_swiz(struct tgsi_full_src_register* reg)
{
    /* Only the last 3 bits... */
    return (r500_rgba_swiz(reg) >> 9) |
        (reg->SrcRegister.Negate ? (1 << 9) : 0) |
        (reg->SrcRegisterExtMod.Absolute ? (1 << 10) : 0);
}

static INLINE uint32_t r300_rgb_op(unsigned op)
{
    switch (op) {
        case TGSI_OPCODE_MOV:
            return R300_ALU_OUTC_CMP;
        default:
            return 0;
    }
}

static INLINE uint32_t r300_alpha_op(unsigned op)
{
    switch (op) {
        case TGSI_OPCODE_MOV:
            return R300_ALU_OUTA_CMP;
        default:
            return 0;
    }
}

static INLINE uint32_t r500_rgba_op(unsigned op)
{
    switch (op) {
        case TGSI_OPCODE_EX2:
        case TGSI_OPCODE_LG2:
        case TGSI_OPCODE_RCP:
        case TGSI_OPCODE_RSQ:
            return R500_ALU_RGBA_OP_SOP;
        case TGSI_OPCODE_FRC:
            return R500_ALU_RGBA_OP_FRC;
        case TGSI_OPCODE_DP3:
            return R500_ALU_RGBA_OP_DP3;
        case TGSI_OPCODE_DP4:
        case TGSI_OPCODE_DPH:
            return R500_ALU_RGBA_OP_DP4;
        case TGSI_OPCODE_ABS:
        case TGSI_OPCODE_CMP:
        case TGSI_OPCODE_MOV:
        case TGSI_OPCODE_SWZ:
            return R500_ALU_RGBA_OP_CMP;
        case TGSI_OPCODE_ADD:
        case TGSI_OPCODE_MAD:
        case TGSI_OPCODE_MUL:
        case TGSI_OPCODE_SUB:
            return R500_ALU_RGBA_OP_MAD;
        default:
            return 0;
    }
}

static INLINE uint32_t r500_alpha_op(unsigned op)
{
    switch (op) {
        case TGSI_OPCODE_EX2:
            return R500_ALPHA_OP_EX2;
        case TGSI_OPCODE_LG2:
            return R500_ALPHA_OP_LN2;
        case TGSI_OPCODE_RCP:
            return R500_ALPHA_OP_RCP;
        case TGSI_OPCODE_RSQ:
            return R500_ALPHA_OP_RSQ;
        case TGSI_OPCODE_FRC:
            return R500_ALPHA_OP_FRC;
        case TGSI_OPCODE_DP3:
        case TGSI_OPCODE_DP4:
        case TGSI_OPCODE_DPH:
            return R500_ALPHA_OP_DP;
        case TGSI_OPCODE_ABS:
        case TGSI_OPCODE_CMP:
        case TGSI_OPCODE_MOV:
        case TGSI_OPCODE_SWZ:
            return R500_ALPHA_OP_CMP;
        case TGSI_OPCODE_ADD:
        case TGSI_OPCODE_MAD:
        case TGSI_OPCODE_MUL:
        case TGSI_OPCODE_SUB:
            return R500_ALPHA_OP_MAD;
        default:
            return 0;
    }
}

static INLINE uint32_t r500_tex_op(unsigned op)
{
    switch (op) {
        case TGSI_OPCODE_KIL:
            return R500_TEX_INST_TEXKILL;
        case TGSI_OPCODE_TEX:
            return R500_TEX_INST_LD;
        case TGSI_OPCODE_TXB:
            return R500_TEX_INST_LODBIAS;
        case TGSI_OPCODE_TXP:
            return R500_TEX_INST_PROJ;
        default:
            return 0;
    }
}

static INLINE void r300_emit_maths(struct r300_fragment_shader* fs,
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
        r300_rgb_op(op);
    fs->instructions[i].alu_rgb_addr = R300_RGB_ADDR0(0) | R300_RGB_ADDR1(0) |
        R300_RGB_ADDR2(0) | R300_ALU_DSTC_OUTPUT_XYZ;
    fs->instructions[i].alu_alpha_inst = R300_ALPHA_SWIZA(R300_ALU_ARGA_SRC0A) |
        R300_ALPHA_SWIZB(R300_ALU_ARGA_SRC0A) |
        R300_ALPHA_SWIZC(R300_ALU_ARGA_ZERO) |
        r300_alpha_op(op);
    fs->instructions[i].alu_alpha_addr = R300_ALPHA_ADDR0(0) |
        R300_ALPHA_ADDR1(0) | R300_ALPHA_ADDR2(0) | R300_ALU_DSTA_OUTPUT;

    fs->alu_instruction_count++;
}

/* Setup an ALU operation. */
static INLINE void r500_emit_alu(struct r500_fragment_shader* fs,
                                 struct r300_fs_asm* assembler,
                                 struct tgsi_full_dst_register* dst)
{
    int i = fs->instruction_count;

    if (dst->DstRegister.File == TGSI_FILE_OUTPUT) {
        fs->instructions[i].inst0 = R500_INST_TYPE_OUT |
        R500_ALU_OMASK(dst->DstRegister.WriteMask);
    } else {
        fs->instructions[i].inst0 = R500_INST_TYPE_ALU |
        R500_ALU_WMASK(dst->DstRegister.WriteMask);
    }

    fs->instructions[i].inst0 |= R500_INST_TEX_SEM_WAIT;

    fs->instructions[i].inst4 =
        R500_ALPHA_ADDRD(r300_fs_dst(assembler, &dst->DstRegister));
    fs->instructions[i].inst5 =
        R500_ALU_RGBA_ADDRD(r300_fs_dst(assembler, &dst->DstRegister));
}

static INLINE void r500_emit_maths(struct r500_fragment_shader* fs,
                                   struct r300_fs_asm* assembler,
                                   struct tgsi_full_src_register* src,
                                   struct tgsi_full_dst_register* dst,
                                   unsigned op,
                                   unsigned count)
{
    int i = fs->instruction_count;

    r500_emit_alu(fs, assembler, dst);

    switch (count) {
        case 3:
            fs->instructions[i].inst1 =
                R500_RGB_ADDR2(r300_fs_src(assembler, &src[2].SrcRegister));
            fs->instructions[i].inst2 =
                R500_ALPHA_ADDR2(r300_fs_src(assembler, &src[2].SrcRegister));
            fs->instructions[i].inst5 |=
                R500_ALU_RGBA_SEL_C_SRC2 |
                R500_SWIZ_RGBA_C(r500_rgb_swiz(&src[2])) |
                R500_ALU_RGBA_ALPHA_SEL_C_SRC2 |
                R500_SWIZ_ALPHA_C(r500_alpha_swiz(&src[2]));
        case 2:
            fs->instructions[i].inst1 |=
                R500_RGB_ADDR1(r300_fs_src(assembler, &src[1].SrcRegister));
            fs->instructions[i].inst2 |=
                R500_ALPHA_ADDR1(r300_fs_src(assembler, &src[1].SrcRegister));
            fs->instructions[i].inst3 =
                R500_ALU_RGB_SEL_B_SRC1 |
                R500_SWIZ_RGB_B(r500_rgb_swiz(&src[1]));
            fs->instructions[i].inst4 |=
                R500_SWIZ_ALPHA_B(r500_alpha_swiz(&src[1])) |
                R500_ALPHA_SEL_B_SRC1;
        case 1:
        case 0:
        default:
            fs->instructions[i].inst1 |=
                R500_RGB_ADDR0(r300_fs_src(assembler, &src[0].SrcRegister));
            fs->instructions[i].inst2 |=
                R500_ALPHA_ADDR0(r300_fs_src(assembler, &src[0].SrcRegister));
            fs->instructions[i].inst3 |=
                R500_ALU_RGB_SEL_A_SRC0 |
                R500_SWIZ_RGB_A(r500_rgb_swiz(&src[0]));
            fs->instructions[i].inst4 |=
                R500_SWIZ_ALPHA_A(r500_alpha_swiz(&src[0])) |
                R500_ALPHA_SEL_A_SRC0;
            break;
    }

    fs->instructions[i].inst4 |= r500_alpha_op(op);
    fs->instructions[i].inst5 |= r500_rgba_op(op);

    fs->instruction_count++;
}

static INLINE void r500_emit_tex(struct r500_fragment_shader* fs,
                                 struct r300_fs_asm* assembler,
                                 struct tgsi_full_src_register* src,
                                 struct tgsi_full_dst_register* dst,
                                 uint32_t op)
{
    int i = fs->instruction_count;

    fs->instructions[i].inst0 = R500_INST_TYPE_TEX |
        R500_TEX_WMASK(dst->DstRegister.WriteMask) |
        R500_INST_TEX_SEM_WAIT;
    fs->instructions[i].inst1 = R500_TEX_ID(0) |
        R500_TEX_SEM_ACQUIRE | //R500_TEX_IGNORE_UNCOVERED |
        r500_tex_op(op);
    fs->instructions[i].inst2 =
        R500_TEX_SRC_ADDR(r300_fs_src(assembler, &src->SrcRegister)) |
        R500_SWIZ_TEX_STRQ(r500_strq_swiz(src)) |
        R500_TEX_DST_ADDR(r300_fs_dst(assembler, &dst->DstRegister)) |
        R500_TEX_DST_R_SWIZ_R | R500_TEX_DST_G_SWIZ_G |
        R500_TEX_DST_B_SWIZ_B | R500_TEX_DST_A_SWIZ_A;

    if (dst->DstRegister.File == TGSI_FILE_OUTPUT) {
        fs->instructions[i].inst2 |=
            R500_TEX_DST_ADDR(assembler->temp_count +
                    assembler->temp_offset);

        fs->instruction_count++;

        /* Setup and emit a MOV. */
        src[0].SrcRegister.Index = assembler->temp_count;
        src[0].SrcRegister.File = TGSI_FILE_TEMPORARY;

        src[1] = src[0];
        src[2] = r500_constant_zero;
        r500_emit_maths(fs, assembler, src, dst, TGSI_OPCODE_MOV, 3);
    } else {
        fs->instruction_count++;
    }
}

static void r300_fs_instruction(struct r300_fragment_shader* fs,
                                struct r300_fs_asm* assembler,
                                struct tgsi_full_instruction* inst)
{
    switch (inst->Instruction.Opcode) {
        case TGSI_OPCODE_MOV:
            /* src0 -> src1 and src2 forced to zero */
            inst->FullSrcRegisters[1] = inst->FullSrcRegisters[0];
            inst->FullSrcRegisters[2] = r500_constant_zero;
            r300_emit_maths(fs, assembler, inst->FullSrcRegisters,
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

static void r500_fs_instruction(struct r500_fragment_shader* fs,
                                struct r300_fs_asm* assembler,
                                struct tgsi_full_instruction* inst)
{
    /* Switch between opcodes. When possible, prefer using the official
     * AMD/ATI names for opcodes, please, as it facilitates using the
     * documentation. */
    switch (inst->Instruction.Opcode) {
        /* The simple scalar ops. */
        case TGSI_OPCODE_EX2:
        case TGSI_OPCODE_LG2:
        case TGSI_OPCODE_RCP:
        case TGSI_OPCODE_RSQ:
            /* Copy red swizzle to alpha for src0 */
            inst->FullSrcRegisters[0].SrcRegisterExtSwz.ExtSwizzleW =
                inst->FullSrcRegisters[0].SrcRegisterExtSwz.ExtSwizzleX;
            inst->FullSrcRegisters[0].SrcRegister.SwizzleW =
                inst->FullSrcRegisters[0].SrcRegister.SwizzleX;
            /* Fall through */
        case TGSI_OPCODE_FRC:
            r500_emit_maths(fs, assembler, inst->FullSrcRegisters,
                    &inst->FullDstRegisters[0], inst->Instruction.Opcode, 1);
            break;

        /* The dot products. */
        case TGSI_OPCODE_DPH:
            /* Set alpha swizzle to one for src0 */
            if (!inst->FullSrcRegisters[0].SrcRegister.Extended) {
                inst->FullSrcRegisters[0].SrcRegister.Extended = TRUE;
                inst->FullSrcRegisters[0].SrcRegisterExtSwz.ExtSwizzleX =
                    inst->FullSrcRegisters[0].SrcRegister.SwizzleX;
                inst->FullSrcRegisters[0].SrcRegisterExtSwz.ExtSwizzleY =
                    inst->FullSrcRegisters[0].SrcRegister.SwizzleY;
                inst->FullSrcRegisters[0].SrcRegisterExtSwz.ExtSwizzleZ =
                    inst->FullSrcRegisters[0].SrcRegister.SwizzleZ;
            }
            inst->FullSrcRegisters[0].SrcRegisterExtSwz.ExtSwizzleW =
                TGSI_EXTSWIZZLE_ONE;
            /* Fall through */
        case TGSI_OPCODE_DP3:
        case TGSI_OPCODE_DP4:
            r500_emit_maths(fs, assembler, inst->FullSrcRegisters,
                    &inst->FullDstRegisters[0], inst->Instruction.Opcode, 2);
            break;

        /* Simple three-source operations. */
        case TGSI_OPCODE_CMP:
            /* Swap src0 and src2 */
            inst->FullSrcRegisters[3] = inst->FullSrcRegisters[2];
            inst->FullSrcRegisters[2] = inst->FullSrcRegisters[0];
            inst->FullSrcRegisters[0] = inst->FullSrcRegisters[3];
            r500_emit_maths(fs, assembler, inst->FullSrcRegisters,
                    &inst->FullDstRegisters[0], inst->Instruction.Opcode, 3);
            break;

        /* The MAD variants. */
        case TGSI_OPCODE_SUB:
            /* Just like ADD, but flip the negation on src1 first */
            inst->FullSrcRegisters[1].SrcRegister.Negate =
                !inst->FullSrcRegisters[1].SrcRegister.Negate;
            /* Fall through */
        case TGSI_OPCODE_ADD:
            /* Force src0 to one, move all registers over */
            inst->FullSrcRegisters[2] = inst->FullSrcRegisters[1];
            inst->FullSrcRegisters[1] = inst->FullSrcRegisters[0];
            inst->FullSrcRegisters[0] = r500_constant_one;
            r500_emit_maths(fs, assembler, inst->FullSrcRegisters,
                    &inst->FullDstRegisters[0], inst->Instruction.Opcode, 3);
            break;
        case TGSI_OPCODE_MUL:
            /* Force our src2 to zero */
            inst->FullSrcRegisters[2] = r500_constant_zero;
            r500_emit_maths(fs, assembler, inst->FullSrcRegisters,
                    &inst->FullDstRegisters[0], inst->Instruction.Opcode, 3);
            break;
        case TGSI_OPCODE_MAD:
            r500_emit_maths(fs, assembler, inst->FullSrcRegisters,
                    &inst->FullDstRegisters[0], inst->Instruction.Opcode, 3);
            break;

        /* The MOV variants. */
        case TGSI_OPCODE_ABS:
            /* Set absolute value modifiers. */
            inst->FullSrcRegisters[0].SrcRegisterExtMod.Absolute = TRUE;
            /* Fall through */
        case TGSI_OPCODE_MOV:
        case TGSI_OPCODE_SWZ:
            /* src0 -> src1 and src2 forced to zero */
            inst->FullSrcRegisters[1] = inst->FullSrcRegisters[0];
            inst->FullSrcRegisters[2] = r500_constant_zero;
            r500_emit_maths(fs, assembler, inst->FullSrcRegisters,
                    &inst->FullDstRegisters[0], inst->Instruction.Opcode, 3);
            break;

        /* The texture instruction set. */
        case TGSI_OPCODE_KIL:
        case TGSI_OPCODE_TEX:
        case TGSI_OPCODE_TXB:
        case TGSI_OPCODE_TXP:
            r500_emit_tex(fs, assembler, &inst->FullSrcRegisters[0],
                    &inst->FullDstRegisters[0], inst->Instruction.Opcode);
            break;

        /* This is the end. My only friend, the end. */
        case TGSI_OPCODE_END:
            break;
        default:
            debug_printf("r300: fs: Bad opcode %d\n",
                    inst->Instruction.Opcode);
            break;
    }

    /* Clamp, if saturation flags are set. */
    if (inst->Instruction.Saturate == TGSI_SAT_ZERO_ONE) {
        fs->instructions[fs->instruction_count - 1].inst0 |=
            R500_INST_RGB_CLAMP | R500_INST_ALPHA_CLAMP;
    }
}

static void r300_fs_finalize(struct r3xx_fragment_shader* fs,
                             struct r300_fs_asm* assembler)
{
    fs->stack_size = assembler->temp_count + assembler->temp_offset;
}

static void r500_fs_finalize(struct r500_fragment_shader* fs,
                             struct r300_fs_asm* assembler)
{
    /* XXX should this just go with OPCODE_END? */
    fs->instructions[fs->instruction_count - 1].inst0 |=
        R500_INST_LAST;
}

void r300_translate_fragment_shader(struct r300_context* r300,
                                    struct r3xx_fragment_shader* fs)
{
    struct tgsi_parse_context parser;
    int i;
    boolean is_r500 = r300_screen(r300->context.screen)->caps->is_r500;
    struct r300_constant_buffer* consts =
        &r300->shader_constants[PIPE_SHADER_FRAGMENT];

    struct r300_fs_asm* assembler = CALLOC_STRUCT(r300_fs_asm);
    if (assembler == NULL) {
        return;
    }
    /* Setup starting offset for immediates. */
    assembler->imm_offset = consts->user_count;

    /* Make sure we start at the beginning of the shader. */
    if (is_r500) {
        ((struct r500_fragment_shader*)fs)->instruction_count = 0;
    }

    tgsi_parse_init(&parser, fs->state.tokens);

    while (!tgsi_parse_end_of_tokens(&parser)) {
        tgsi_parse_token(&parser);

        /* This is seriously the lamest way to create fragment programs ever.
         * I blame TGSI. */
        switch (parser.FullToken.Token.Type) {
            case TGSI_TOKEN_TYPE_DECLARATION:
                /* Allocated registers sitting at the beginning
                 * of the program. */
                r300_fs_declare(assembler, &parser.FullToken.FullDeclaration);
                break;
            case TGSI_TOKEN_TYPE_IMMEDIATE:
                debug_printf("r300: Emitting immediate to constant buffer, "
                        "position %d\n",
                        assembler->imm_offset + assembler->imm_count);
                /* I am not amused by the length of these. */
                for (i = 0; i < 4; i++) {
                    consts->constants[assembler->imm_offset +
                        assembler->imm_count][i] =
                        parser.FullToken.FullImmediate.u.ImmediateFloat32[i]
                        .Float;
                }
                assembler->imm_count++;
                break;
            case TGSI_TOKEN_TYPE_INSTRUCTION:
                if (is_r500) {
                    r500_fs_instruction((struct r500_fragment_shader*)fs,
                            assembler, &parser.FullToken.FullInstruction);
                } else {
                    r300_fs_instruction((struct r300_fragment_shader*)fs,
                            assembler, &parser.FullToken.FullInstruction);
                }
                break;
        }
    }

    debug_printf("r300: fs: %d texs and %d colors, first free reg is %d\n",
            assembler->tex_count, assembler->color_count,
            assembler->tex_count + assembler->color_count);

    consts->count = consts->user_count + assembler->imm_count;
    debug_printf("r300: fs: %d total constants, "
            "%d from user and %d from immediates\n", consts->count,
            consts->user_count, assembler->imm_count);
    r300_fs_finalize(fs, assembler);
    if (is_r500) {
        r500_fs_finalize((struct r500_fragment_shader*)fs, assembler);
    }

    tgsi_dump(fs->state.tokens);
    /* XXX finish r300 dumper too */
    if (is_r500) {
        r500_fs_dump((struct r500_fragment_shader*)fs);
    }

    tgsi_parse_free(&parser);
    FREE(assembler);
}
