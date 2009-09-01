/*
 * Copyright 2009 Nicolai Hähnle <nhaehnle@gmail.com>
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
 * THE COPYRIGHT HOLDER(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "r300_tgsi_to_rc.h"

#include "radeon_compiler.h"
#include "radeon_program.h"

#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_scan.h"
#include "tgsi/tgsi_util.h"


static unsigned translate_opcode(unsigned opcode)
{
    switch(opcode) {
        case TGSI_OPCODE_ARL: return OPCODE_ARL;
        case TGSI_OPCODE_MOV: return OPCODE_MOV;
        case TGSI_OPCODE_LIT: return OPCODE_LIT;
        case TGSI_OPCODE_RCP: return OPCODE_RCP;
        case TGSI_OPCODE_RSQ: return OPCODE_RSQ;
        case TGSI_OPCODE_EXP: return OPCODE_EXP;
        case TGSI_OPCODE_LOG: return OPCODE_LOG;
        case TGSI_OPCODE_MUL: return OPCODE_MUL;
        case TGSI_OPCODE_ADD: return OPCODE_ADD;
        case TGSI_OPCODE_DP3: return OPCODE_DP3;
        case TGSI_OPCODE_DP4: return OPCODE_DP4;
        case TGSI_OPCODE_DST: return OPCODE_DST;
        case TGSI_OPCODE_MIN: return OPCODE_MIN;
        case TGSI_OPCODE_MAX: return OPCODE_MAX;
        case TGSI_OPCODE_SLT: return OPCODE_SLT;
        case TGSI_OPCODE_SGE: return OPCODE_SGE;
        case TGSI_OPCODE_MAD: return OPCODE_MAD;
        case TGSI_OPCODE_SUB: return OPCODE_SUB;
        case TGSI_OPCODE_LRP: return OPCODE_LRP;
     /* case TGSI_OPCODE_CND: return OPCODE_CND; */
        case TGSI_OPCODE_DP2A: return OPCODE_DP2A;
                                        /* gap */
        case TGSI_OPCODE_FRC: return OPCODE_FRC;
     /* case TGSI_OPCODE_CLAMP: return OPCODE_CLAMP; */
        case TGSI_OPCODE_FLR: return OPCODE_FLR;
     /* case TGSI_OPCODE_ROUND: return OPCODE_ROUND; */
        case TGSI_OPCODE_EX2: return OPCODE_EX2;
        case TGSI_OPCODE_LG2: return OPCODE_LG2;
        case TGSI_OPCODE_POW: return OPCODE_POW;
        case TGSI_OPCODE_XPD: return OPCODE_XPD;
                                        /* gap */
        case TGSI_OPCODE_ABS: return OPCODE_ABS;
        case TGSI_OPCODE_RCC: return OPCODE_RCC;
        case TGSI_OPCODE_DPH: return OPCODE_DPH;
        case TGSI_OPCODE_COS: return OPCODE_COS;
        case TGSI_OPCODE_DDX: return OPCODE_DDX;
        case TGSI_OPCODE_DDY: return OPCODE_DDY;
     /* case TGSI_OPCODE_KILP: return OPCODE_KILP; */
        case TGSI_OPCODE_PK2H: return OPCODE_PK2H;
        case TGSI_OPCODE_PK2US: return OPCODE_PK2US;
        case TGSI_OPCODE_PK4B: return OPCODE_PK4B;
        case TGSI_OPCODE_PK4UB: return OPCODE_PK4UB;
        case TGSI_OPCODE_RFL: return OPCODE_RFL;
        case TGSI_OPCODE_SEQ: return OPCODE_SEQ;
        case TGSI_OPCODE_SFL: return OPCODE_SFL;
        case TGSI_OPCODE_SGT: return OPCODE_SGT;
        case TGSI_OPCODE_SIN: return OPCODE_SIN;
        case TGSI_OPCODE_SLE: return OPCODE_SLE;
        case TGSI_OPCODE_SNE: return OPCODE_SNE;
        case TGSI_OPCODE_STR: return OPCODE_STR;
        case TGSI_OPCODE_TEX: return OPCODE_TEX;
        case TGSI_OPCODE_TXD: return OPCODE_TXD;
        case TGSI_OPCODE_TXP: return OPCODE_TXP;
        case TGSI_OPCODE_UP2H: return OPCODE_UP2H;
        case TGSI_OPCODE_UP2US: return OPCODE_UP2US;
        case TGSI_OPCODE_UP4B: return OPCODE_UP4B;
        case TGSI_OPCODE_UP4UB: return OPCODE_UP4UB;
        case TGSI_OPCODE_X2D: return OPCODE_X2D;
        case TGSI_OPCODE_ARA: return OPCODE_ARA;
        case TGSI_OPCODE_ARR: return OPCODE_ARR;
        case TGSI_OPCODE_BRA: return OPCODE_BRA;
        case TGSI_OPCODE_CAL: return OPCODE_CAL;
        case TGSI_OPCODE_RET: return OPCODE_RET;
        case TGSI_OPCODE_SSG: return OPCODE_SSG;
        case TGSI_OPCODE_CMP: return OPCODE_CMP;
        case TGSI_OPCODE_SCS: return OPCODE_SCS;
        case TGSI_OPCODE_TXB: return OPCODE_TXB;
     /* case TGSI_OPCODE_NRM: return OPCODE_NRM; */
     /* case TGSI_OPCODE_DIV: return OPCODE_DIV; */
        case TGSI_OPCODE_DP2: return OPCODE_DP2;
        case TGSI_OPCODE_TXL: return OPCODE_TXL;
        case TGSI_OPCODE_BRK: return OPCODE_BRK;
        case TGSI_OPCODE_IF: return OPCODE_IF;
     /* case TGSI_OPCODE_LOOP: return OPCODE_LOOP; */
     /* case TGSI_OPCODE_REP: return OPCODE_REP; */
        case TGSI_OPCODE_ELSE: return OPCODE_ELSE;
        case TGSI_OPCODE_ENDIF: return OPCODE_ENDIF;
        case TGSI_OPCODE_ENDLOOP: return OPCODE_ENDLOOP;
     /* case TGSI_OPCODE_ENDREP: return OPCODE_ENDREP; */
        case TGSI_OPCODE_PUSHA: return OPCODE_PUSHA;
        case TGSI_OPCODE_POPA: return OPCODE_POPA;
     /* case TGSI_OPCODE_CEIL: return OPCODE_CEIL; */
     /* case TGSI_OPCODE_I2F: return OPCODE_I2F; */
        case TGSI_OPCODE_NOT: return OPCODE_NOT;
        case TGSI_OPCODE_TRUNC: return OPCODE_TRUNC;
     /* case TGSI_OPCODE_SHL: return OPCODE_SHL; */
     /* case TGSI_OPCODE_SHR: return OPCODE_SHR; */
        case TGSI_OPCODE_AND: return OPCODE_AND;
        case TGSI_OPCODE_OR: return OPCODE_OR;
     /* case TGSI_OPCODE_MOD: return OPCODE_MOD; */
        case TGSI_OPCODE_XOR: return OPCODE_XOR;
     /* case TGSI_OPCODE_SAD: return OPCODE_SAD; */
     /* case TGSI_OPCODE_TXF: return OPCODE_TXF; */
     /* case TGSI_OPCODE_TXQ: return OPCODE_TXQ; */
        case TGSI_OPCODE_CONT: return OPCODE_CONT;
     /* case TGSI_OPCODE_EMIT: return OPCODE_EMIT; */
     /* case TGSI_OPCODE_ENDPRIM: return OPCODE_ENDPRIM; */
     /* case TGSI_OPCODE_BGNLOOP2: return OPCODE_BGNLOOP2; */
        case TGSI_OPCODE_BGNSUB: return OPCODE_BGNSUB;
     /* case TGSI_OPCODE_ENDLOOP2: return OPCODE_ENDLOOP2; */
        case TGSI_OPCODE_ENDSUB: return OPCODE_ENDSUB;
        case TGSI_OPCODE_NOISE1: return OPCODE_NOISE1;
        case TGSI_OPCODE_NOISE2: return OPCODE_NOISE2;
        case TGSI_OPCODE_NOISE3: return OPCODE_NOISE3;
        case TGSI_OPCODE_NOISE4: return OPCODE_NOISE4;
        case TGSI_OPCODE_NOP: return OPCODE_NOP;
                                        /* gap */
        case TGSI_OPCODE_NRM4: return OPCODE_NRM4;
     /* case TGSI_OPCODE_CALLNZ: return OPCODE_CALLNZ; */
     /* case TGSI_OPCODE_IFC: return OPCODE_IFC; */
     /* case TGSI_OPCODE_BREAKC: return OPCODE_BREAKC; */
        case TGSI_OPCODE_KIL: return OPCODE_KIL;
        case TGSI_OPCODE_END: return OPCODE_END;
        case TGSI_OPCODE_SWZ: return OPCODE_SWZ;
    }

    fprintf(stderr, "Unknown opcode: %i\n", opcode);
    abort();
}

static unsigned translate_saturate(unsigned saturate)
{
    switch(saturate) {
        case TGSI_SAT_NONE: return SATURATE_OFF;
        case TGSI_SAT_ZERO_ONE: return SATURATE_ZERO_ONE;
        case TGSI_SAT_MINUS_PLUS_ONE: return SATURATE_PLUS_MINUS_ONE;
    }

    fprintf(stderr, "Unknown saturate mode: %i\n", saturate);
    abort();
}

static unsigned translate_register_file(unsigned file)
{
    switch(file) {
        case TGSI_FILE_CONSTANT: return PROGRAM_CONSTANT;
        case TGSI_FILE_IMMEDIATE: return PROGRAM_CONSTANT;
        case TGSI_FILE_INPUT: return PROGRAM_INPUT;
        case TGSI_FILE_OUTPUT: return PROGRAM_OUTPUT;
        case TGSI_FILE_TEMPORARY: return PROGRAM_TEMPORARY;
        case TGSI_FILE_ADDRESS: return PROGRAM_ADDRESS;
    }

    fprintf(stderr, "Unhandled register file: %i\n", file);
    abort();
}

static int translate_register_index(
    struct tgsi_to_rc * ttr,
    unsigned file,
    int index)
{
    if (file == TGSI_FILE_IMMEDIATE)
        return ttr->immediate_offset + index;

    return index;
}

static void transform_dstreg(
    struct tgsi_to_rc * ttr,
    struct prog_dst_register * dst,
    struct tgsi_full_dst_register * src)
{
    dst->File = translate_register_file(src->DstRegister.File);
    dst->Index = translate_register_index(ttr, src->DstRegister.File, src->DstRegister.Index);
    dst->WriteMask = src->DstRegister.WriteMask;
    dst->RelAddr = src->DstRegister.Indirect;
}

static void transform_srcreg(
    struct tgsi_to_rc * ttr,
    struct prog_src_register * dst,
    struct tgsi_full_src_register * src)
{
    dst->File = translate_register_file(src->SrcRegister.File);
    dst->Index = translate_register_index(ttr, src->SrcRegister.File, src->SrcRegister.Index);
    dst->RelAddr = src->SrcRegister.Indirect;
    dst->Swizzle = tgsi_util_get_full_src_register_extswizzle(src, 0);
    dst->Swizzle |= tgsi_util_get_full_src_register_extswizzle(src, 1) << 3;
    dst->Swizzle |= tgsi_util_get_full_src_register_extswizzle(src, 2) << 6;
    dst->Swizzle |= tgsi_util_get_full_src_register_extswizzle(src, 3) << 9;
    dst->Abs = src->SrcRegisterExtMod.Absolute;
    dst->Negate =
        src->SrcRegisterExtSwz.NegateX |
        (src->SrcRegisterExtSwz.NegateY << 1) |
        (src->SrcRegisterExtSwz.NegateZ << 2) |
        (src->SrcRegisterExtSwz.NegateW << 3);
    dst->Negate ^= src->SrcRegister.Negate ? NEGATE_XYZW : 0;
}

static void transform_texture(struct rc_instruction * dst, struct tgsi_instruction_ext_texture src)
{
    switch(src.Texture) {
        case TGSI_TEXTURE_1D:
            dst->I.TexSrcTarget = TEXTURE_1D_INDEX;
            break;
        case TGSI_TEXTURE_2D:
            dst->I.TexSrcTarget = TEXTURE_2D_INDEX;
            break;
        case TGSI_TEXTURE_3D:
            dst->I.TexSrcTarget = TEXTURE_3D_INDEX;
            break;
        case TGSI_TEXTURE_CUBE:
            dst->I.TexSrcTarget = TEXTURE_CUBE_INDEX;
            break;
        case TGSI_TEXTURE_RECT:
            dst->I.TexSrcTarget = TEXTURE_RECT_INDEX;
            break;
        case TGSI_TEXTURE_SHADOW1D:
            dst->I.TexSrcTarget = TEXTURE_1D_INDEX;
            dst->I.TexShadow = 1;
            break;
        case TGSI_TEXTURE_SHADOW2D:
            dst->I.TexSrcTarget = TEXTURE_2D_INDEX;
            dst->I.TexShadow = 1;
            break;
        case TGSI_TEXTURE_SHADOWRECT:
            dst->I.TexSrcTarget = TEXTURE_RECT_INDEX;
            dst->I.TexShadow = 1;
            break;
    }
}

static void transform_instruction(struct tgsi_to_rc * ttr, struct tgsi_full_instruction * src)
{
    if (src->Instruction.Opcode == TGSI_OPCODE_END)
        return;

    struct rc_instruction * dst = rc_insert_new_instruction(ttr->compiler, ttr->compiler->Program.Instructions.Prev);
    int i;

    dst->I.Opcode = translate_opcode(src->Instruction.Opcode);
    dst->I.SaturateMode = translate_saturate(src->Instruction.Saturate);

    if (src->Instruction.NumDstRegs)
        transform_dstreg(ttr, &dst->I.DstReg, &src->FullDstRegisters[0]);

    for(i = 0; i < src->Instruction.NumSrcRegs; ++i) {
        if (src->FullSrcRegisters[i].SrcRegister.File == TGSI_FILE_SAMPLER)
            dst->I.TexSrcUnit = src->FullSrcRegisters[i].SrcRegister.Index;
        else
            transform_srcreg(ttr, &dst->I.SrcReg[i], &src->FullSrcRegisters[i]);
    }

    /* Texturing. */
    transform_texture(dst, src->InstructionExtTexture);
}

static void handle_immediate(struct tgsi_to_rc * ttr, struct tgsi_full_immediate * imm)
{
    struct rc_constant constant;
    int i;

    constant.Type = RC_CONSTANT_IMMEDIATE;
    constant.Size = 4;
    for(i = 0; i < 4; ++i)
        constant.u.Immediate[i] = imm->u[i].Float;
    rc_constants_add(&ttr->compiler->Program.Constants, &constant);
}

void r300_tgsi_to_rc(struct tgsi_to_rc * ttr, const struct tgsi_token * tokens)
{
    struct tgsi_parse_context parser;
    int i;

    /* Allocate constants placeholders.
     *
     * Note: What if declared constants are not contiguous? */
    for(i = 0; i <= ttr->info->file_max[TGSI_FILE_CONSTANT]; ++i) {
        struct rc_constant constant;
        memset(&constant, 0, sizeof(constant));
        constant.Type = RC_CONSTANT_EXTERNAL;
        constant.Size = 4;
        constant.u.External = i;
        rc_constants_add(&ttr->compiler->Program.Constants, &constant);
    }

    ttr->immediate_offset = ttr->compiler->Program.Constants.Count;

    tgsi_parse_init(&parser, tokens);

    while (!tgsi_parse_end_of_tokens(&parser)) {
        tgsi_parse_token(&parser);

        switch (parser.FullToken.Token.Type) {
            case TGSI_TOKEN_TYPE_DECLARATION:
                break;
            case TGSI_TOKEN_TYPE_IMMEDIATE:
                handle_immediate(ttr, &parser.FullToken.FullImmediate);
                break;
            case TGSI_TOKEN_TYPE_INSTRUCTION:
                transform_instruction(ttr, &parser.FullToken.FullInstruction);
                break;
        }
    }

    tgsi_parse_free(&parser);

    rc_calculate_inputs_outputs(ttr->compiler);
}

