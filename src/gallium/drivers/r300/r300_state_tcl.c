/*
 * Copyright 2009 Corbin Simpson <MostAwesomeDude@gmail.com>
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

#include "r300_state_tcl.h"

static void r300_vs_declare(struct r300_vs_asm* assembler,
                            struct tgsi_full_declaration* decl)
{
    switch (decl->Declaration.File) {
        case TGSI_FILE_INPUT:
            break;
        case TGSI_FILE_OUTPUT:
            switch (decl->Semantic.SemanticName) {
                case TGSI_SEMANTIC_POSITION:
                    assembler->tab[decl->DeclarationRange.First] = 0;
                    break;
                case TGSI_SEMANTIC_COLOR:
                    assembler->tab[decl->DeclarationRange.First] =
                        (assembler->point_size ? 1 : 0) +
                        assembler->out_colors;
                    break;
                case TGSI_SEMANTIC_FOG:
                case TGSI_SEMANTIC_GENERIC:
                    /* XXX multiple? */
                    assembler->tab[decl->DeclarationRange.First] =
                        (assembler->point_size ? 1 : 0) +
                        assembler->out_colors +
                        assembler->out_texcoords;
                    break;
                case TGSI_SEMANTIC_PSIZE:
                    assembler->tab[decl->DeclarationRange.First] = 1;
                    break;
                default:
                    debug_printf("r300: vs: Bad semantic declaration %d\n",
                        decl->Semantic.SemanticName);
                    break;
            }
            break;
        case TGSI_FILE_CONSTANT:
            break;
        case TGSI_FILE_TEMPORARY:
            assembler->temp_count++;
            break;
        default:
            debug_printf("r300: vs: Bad file %d\n", decl->Declaration.File);
            break;
    }
}

static INLINE unsigned r300_vs_src_type(struct r300_vs_asm* assembler,
                                        struct tgsi_src_register* src)
{
    switch (src->File) {
        case TGSI_FILE_NULL:
            /* Probably a zero or one swizzle */
            return R300_PVS_SRC_REG_INPUT;
            break;
        case TGSI_FILE_INPUT:
            return R300_PVS_SRC_REG_INPUT;
            break;
        case TGSI_FILE_TEMPORARY:
            return R300_PVS_SRC_REG_TEMPORARY;
            break;
        case TGSI_FILE_CONSTANT:
            return R300_PVS_SRC_REG_CONSTANT;
        default:
            debug_printf("r300: vs: Unimplemented src type %d\n", src->File);
            break;
    }
    return 0;
}

static INLINE unsigned r300_vs_dst_type(struct r300_vs_asm* assembler,
                                        struct tgsi_dst_register* dst)
{
    switch (dst->File) {
        case TGSI_FILE_TEMPORARY:
            return R300_PVS_DST_REG_TEMPORARY;
            break;
        case TGSI_FILE_OUTPUT:
            return R300_PVS_DST_REG_OUT;
            break;
        default:
            debug_printf("r300: vs: Unimplemented dst type %d\n", dst->File);
            break;
    }
    return 0;
}

static INLINE unsigned r300_vs_dst(struct r300_vs_asm* assembler,
                                   struct tgsi_dst_register* dst)
{
    switch (dst->File) {
        case TGSI_FILE_TEMPORARY:
            return dst->Index;
            break;
        case TGSI_FILE_OUTPUT:
            return assembler->tab[dst->Index];
            break;
        default:
            debug_printf("r300: vs: Unimplemented dst %d\n", dst->File);
            break;
    }
    return 0;
}

static uint32_t r300_vs_op(unsigned op)
{
    switch (op) {
        case TGSI_OPCODE_DP3:
        case TGSI_OPCODE_DP4:
            return R300_VE_DOT_PRODUCT;
        case TGSI_OPCODE_MUL:
            return R300_VE_MULTIPLY;
        case TGSI_OPCODE_ADD:
        case TGSI_OPCODE_MOV:
        case TGSI_OPCODE_SWZ:
            return R300_VE_ADD;
        case TGSI_OPCODE_MAD:
            return R300_PVS_DST_MACRO_INST | R300_PVS_MACRO_OP_2CLK_MADD;
        default:
            break;
    }
    return 0;
}

static uint32_t r300_vs_swiz(struct tgsi_full_src_register* reg)
{
    if (reg->SrcRegister.Extended) {
        return reg->SrcRegisterExtSwz.ExtSwizzleX |
            (reg->SrcRegisterExtSwz.ExtSwizzleY << 3) |
            (reg->SrcRegisterExtSwz.ExtSwizzleZ << 6) |
            (reg->SrcRegisterExtSwz.ExtSwizzleW << 9);
    } else {
        return reg->SrcRegister.SwizzleX |
            (reg->SrcRegister.SwizzleY << 3) |
            (reg->SrcRegister.SwizzleZ << 6) |
            (reg->SrcRegister.SwizzleW << 9);
    }
}

static void r300_vs_emit_inst(struct r300_vertex_shader* vs,
                              struct r300_vs_asm* assembler,
                              struct tgsi_full_src_register* src,
                              struct tgsi_full_dst_register* dst,
                              unsigned op,
                              unsigned count)
{
    int i = vs->instruction_count;
    vs->instructions[i].inst0 = R300_PVS_DST_OPCODE(r300_vs_op(op)) |
        R300_PVS_DST_REG_TYPE(r300_vs_dst_type(assembler, &dst->DstRegister)) |
        R300_PVS_DST_OFFSET(r300_vs_dst(assembler, &dst->DstRegister)) |
        R300_PVS_DST_WE_XYZW;
    switch (count) {
        case 3:
            vs->instructions[i].inst3 =
                R300_PVS_SRC_REG_TYPE(r300_vs_src_type(assembler,
                            &src[2].SrcRegister)) |
                R300_PVS_SRC_OFFSET(src[2].SrcRegister.Index) |
                R300_PVS_SRC_SWIZZLE(r300_vs_swiz(&src[2]));
            /* Fall through */
        case 2:
            vs->instructions[i].inst2 =
                R300_PVS_SRC_REG_TYPE(r300_vs_src_type(assembler,
                            &src[1].SrcRegister)) |
                R300_PVS_SRC_OFFSET(src[1].SrcRegister.Index) |
                R300_PVS_SRC_SWIZZLE(r300_vs_swiz(&src[1]));
            /* Fall through */
        case 1:
            vs->instructions[i].inst1 =
                R300_PVS_SRC_REG_TYPE(r300_vs_src_type(assembler,
                            &src[0].SrcRegister)) |
                R300_PVS_SRC_OFFSET(src[0].SrcRegister.Index) |
                R300_PVS_SRC_SWIZZLE(r300_vs_swiz(&src[0]));
            break;
    }
    vs->instruction_count++;
}

static void r300_vs_instruction(struct r300_vertex_shader* vs,
                                struct r300_vs_asm* assembler,
                                struct tgsi_full_instruction* inst)
{
    switch (inst->Instruction.Opcode) {
        case TGSI_OPCODE_ADD:
        case TGSI_OPCODE_MUL:
            r300_vs_emit_inst(vs, assembler, inst->FullSrcRegisters,
                    &inst->FullDstRegisters[0], inst->Instruction.Opcode,
                    2);
            break;
        case TGSI_OPCODE_DP3:
            /* Set alpha swizzle to zero for src0 and src1 */
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
                TGSI_EXTSWIZZLE_ZERO;
            if (!inst->FullSrcRegisters[1].SrcRegister.Extended) {
                inst->FullSrcRegisters[1].SrcRegister.Extended = TRUE;
                inst->FullSrcRegisters[1].SrcRegisterExtSwz.ExtSwizzleX =
                    inst->FullSrcRegisters[1].SrcRegister.SwizzleX;
                inst->FullSrcRegisters[1].SrcRegisterExtSwz.ExtSwizzleY =
                    inst->FullSrcRegisters[1].SrcRegister.SwizzleY;
                inst->FullSrcRegisters[1].SrcRegisterExtSwz.ExtSwizzleZ =
                    inst->FullSrcRegisters[1].SrcRegister.SwizzleZ;
            }
            inst->FullSrcRegisters[1].SrcRegisterExtSwz.ExtSwizzleW =
                TGSI_EXTSWIZZLE_ZERO;
            /* Fall through */
        case TGSI_OPCODE_DP4:
            r300_vs_emit_inst(vs, assembler, inst->FullSrcRegisters,
                    &inst->FullDstRegisters[0], inst->Instruction.Opcode,
                    2);
            break;
        case TGSI_OPCODE_MOV:
        case TGSI_OPCODE_SWZ:
            inst->FullSrcRegisters[1] = r300_constant_zero;
            r300_vs_emit_inst(vs, assembler, inst->FullSrcRegisters,
                    &inst->FullDstRegisters[0], inst->Instruction.Opcode,
                    2);
            break;
        case TGSI_OPCODE_MAD:
            r300_vs_emit_inst(vs, assembler, inst->FullSrcRegisters,
                    &inst->FullDstRegisters[0], inst->Instruction.Opcode,
                    3);
            break;
        case TGSI_OPCODE_END:
            break;
        default:
            debug_printf("r300: vs: Bad opcode %d\n",
                    inst->Instruction.Opcode);
            break;
    }
}

static void r300_vs_init(struct r300_vertex_shader* vs,
                         struct r300_vs_asm* assembler)
{
    struct tgsi_shader_info* info = &vs->info;
    int i;

    for (i = 0; i < info->num_outputs; i++) {
        switch (info->output_semantic_name[i]) {
            case TGSI_SEMANTIC_PSIZE:
                assembler->point_size = TRUE;
                break;
            case TGSI_SEMANTIC_COLOR:
                assembler->out_colors++;
                break;
            case TGSI_SEMANTIC_FOG:
            case TGSI_SEMANTIC_GENERIC:
                assembler->out_texcoords++;
                break;
        }
    }
}

void r300_translate_vertex_shader(struct r300_context* r300,
                                  struct r300_vertex_shader* vs)
{
    struct tgsi_parse_context parser;
    int i;
    struct r300_constant_buffer* consts =
        &r300->shader_constants[PIPE_SHADER_VERTEX];

    struct r300_vs_asm* assembler = CALLOC_STRUCT(r300_vs_asm);
    if (assembler == NULL) {
        return;
    }

    /* Init assembler. */
    r300_vs_init(vs, assembler);

    /* Setup starting offset for immediates. */
    assembler->imm_offset = consts->user_count;

    tgsi_parse_init(&parser, vs->state.tokens);

    while (!tgsi_parse_end_of_tokens(&parser)) {
        tgsi_parse_token(&parser);

        /* This is seriously the lamest way to create fragment programs ever.
         * I blame TGSI. */
        switch (parser.FullToken.Token.Type) {
            case TGSI_TOKEN_TYPE_DECLARATION:
                /* Allocated registers sitting at the beginning
                 * of the program. */
                r300_vs_declare(assembler, &parser.FullToken.FullDeclaration);
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
                r300_vs_instruction(vs, assembler,
                        &parser.FullToken.FullInstruction);
                break;
        }
    }

    debug_printf("r300: vs: %d texs and %d colors, first free reg is %d\n",
            assembler->tex_count, assembler->color_count,
            assembler->tex_count + assembler->color_count);

    consts->count = consts->user_count + assembler->imm_count;
    debug_printf("r300: vs: %d total constants, "
            "%d from user and %d from immediates\n", consts->count,
            consts->user_count, assembler->imm_count);

    debug_printf("r300: vs: tab: %d %d %d %d\n", assembler->tab[0],
            assembler->tab[1], assembler->tab[2], assembler->tab[3]);

    tgsi_dump(vs->state.tokens);
    /* XXX finish r300 vertex shader dumper */
    r300_vs_dump(vs);

    tgsi_parse_free(&parser);
    FREE(assembler);
}
