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
        case TGSI_FILE_INPUT:
            /* XXX may be wrong */
            return src->Index;
            break;
        case TGSI_FILE_TEMPORARY:
            return src->Index + assembler->temp_offset;
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
    return r500_rgba_swiz(reg) & 0x1ff;
}

static INLINE uint32_t r500_alpha_swiz(struct tgsi_full_src_register* reg)
{
    /* Only the last 3 bits... */
    return r500_rgba_swiz(reg) >> 9;
}

static INLINE void r500_emit_mov(struct r500_fragment_shader* fs,
                                 struct r300_fs_asm* assembler,
                                 struct tgsi_full_src_register* src,
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

    fs->instructions[i].inst0 |=
        R500_INST_TEX_SEM_WAIT |
        R500_INST_RGB_CLAMP | R500_INST_ALPHA_CLAMP;

    fs->instructions[i].inst1 =
        R500_RGB_ADDR0(r300_fs_src(assembler, &src->SrcRegister));
    fs->instructions[i].inst2 =
        R500_ALPHA_ADDR0(r300_fs_src(assembler, &src->SrcRegister));
    fs->instructions[i].inst3 = R500_ALU_RGB_SEL_A_SRC0 |
        R500_SWIZ_RGB_A(r500_rgb_swiz(src)) |
        R500_ALU_RGB_SEL_B_SRC0 |
        R500_SWIZ_RGB_B(r500_rgb_swiz(src));
    fs->instructions[i].inst4 = R500_ALPHA_OP_CMP |
        R500_SWIZ_ALPHA_A(r500_alpha_swiz(src)) |
        R500_SWIZ_ALPHA_B(r500_alpha_swiz(src));
    fs->instructions[i].inst5 =
        R500_ALU_RGBA_OP_CMP | R500_ALU_RGBA_R_SWIZ_0 |
        R500_ALU_RGBA_G_SWIZ_0 | R500_ALU_RGBA_B_SWIZ_0 |
        R500_ALU_RGBA_A_SWIZ_0;

    fs->instruction_count++;
}

static INLINE void r500_emit_tex(struct r500_fragment_shader* fs,
                                 struct r300_fs_asm* assembler,
                                 uint32_t op,
                                 struct tgsi_full_src_register* src,
                                 struct tgsi_full_dst_register* dst)
{
    int i = fs->instruction_count;

    fs->instructions[i].inst0 = R500_INST_TYPE_TEX |
        R500_TEX_WMASK(dst->DstRegister.WriteMask) |
        R500_INST_TEX_SEM_WAIT;
    fs->instructions[i].inst1 = R500_TEX_ID(0) |
        R500_TEX_SEM_ACQUIRE | R500_TEX_IGNORE_UNCOVERED |
        R500_TEX_INST_PROJ;
    fs->instructions[i].inst2 =
        R500_TEX_SRC_ADDR(r300_fs_src(assembler, &src->SrcRegister)) |
        R500_SWIZ_TEX_STRQ(r500_strq_swiz(src)) |
        R500_TEX_DST_ADDR(r300_fs_dst(assembler, &dst->DstRegister)) |
        R500_TEX_DST_R_SWIZ_R | R500_TEX_DST_G_SWIZ_G |
        R500_TEX_DST_B_SWIZ_B | R500_TEX_DST_A_SWIZ_A;

    fs->instruction_count++;
}

static void r500_fs_instruction(struct r500_fragment_shader* fs,
                                struct r300_fs_asm* assembler,
                                struct tgsi_full_instruction* inst)
{
    /* Switch between opcodes. When possible, prefer using the official
     * AMD/ATI names for opcodes, please, as it facilitates using the
     * documentation. */
    switch (inst->Instruction.Opcode) {
        case TGSI_OPCODE_MOV:
        case TGSI_OPCODE_SWZ:
            r500_emit_mov(fs, assembler, &inst->FullSrcRegisters[0],
                    &inst->FullDstRegisters[0]);
            break;
        case TGSI_OPCODE_TXP:
            r500_emit_tex(fs, assembler, 0, &inst->FullSrcRegisters[0],
                    &inst->FullDstRegisters[0]);
            break;
        case TGSI_OPCODE_END:
            break;
        default:
            debug_printf("r300: fs: Bad opcode %d\n",
                    inst->Instruction.Opcode);
            break;
    }
}

static void r500_fs_finalize(struct r500_fragment_shader* fs,
                             struct r300_fs_asm* assembler)
{
    /* XXX subtly wrong */
    fs->shader.stack_size = assembler->temp_offset;

    /* XXX should this just go with OPCODE_END? */
    fs->instructions[fs->instruction_count - 1].inst0 |=
        R500_INST_LAST;
}

void r300_translate_fragment_shader(struct r300_context* r300,
                                    struct r300_fragment_shader* fs)
{
    struct tgsi_parse_context parser;

    tgsi_parse_init(&parser, fs->shader.state.tokens);

    while (!tgsi_parse_end_of_tokens(&parser)) {
        tgsi_parse_token(&parser);
    }

    r300_copy_passthrough_shader(fs);
}

void r500_translate_fragment_shader(struct r300_context* r300,
                                    struct r500_fragment_shader* fs)
{
    struct r300_fs_asm* assembler = CALLOC_STRUCT(r300_fs_asm);
    if (assembler == NULL) {
        return;
    }
    struct tgsi_parse_context parser;

    tgsi_parse_init(&parser, fs->shader.state.tokens);

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
            case TGSI_TOKEN_TYPE_INSTRUCTION:
                r500_fs_instruction(fs, assembler,
                        &parser.FullToken.FullInstruction);
        }

    }

    debug_printf("%d texs and %d colors, first free reg is %d\n",
            assembler->tex_count, assembler->color_count,
            assembler->tex_count + assembler->color_count);

    r500_fs_finalize(fs, assembler);

    tgsi_dump(fs->shader.state.tokens);
    r500_fs_dump(fs);

    //r500_copy_passthrough_shader(fs);

    tgsi_parse_free(&parser);
    FREE(assembler);
}
