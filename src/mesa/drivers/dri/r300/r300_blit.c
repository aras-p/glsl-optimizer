/*
 * Copyright (C) 2009 Maciej Cencora <m.cencora@gmail.com>
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "radeon_common.h"
#include "r300_context.h"

#include "r300_blit.h"
#include "r300_cmdbuf.h"
#include "r300_emit.h"
#include "r300_tex.h"
#include "compiler/radeon_compiler.h"
#include "compiler/radeon_opcodes.h"

static void vp_ins_outs(struct r300_vertex_program_compiler *c)
{
    c->code->inputs[VERT_ATTRIB_POS] = 0;
    c->code->inputs[VERT_ATTRIB_TEX0] = 1;
    c->code->outputs[VERT_RESULT_HPOS] = 0;
    c->code->outputs[VERT_RESULT_TEX0] = 1;
}

static void fp_allocate_hw_inputs(
    struct r300_fragment_program_compiler * c,
    void (*allocate)(void * data, unsigned input, unsigned hwreg),
    void * mydata)
{
    allocate(mydata, FRAG_ATTRIB_TEX0, 0);
}

static void create_vertex_program(struct r300_context *r300)
{
    struct r300_vertex_program_compiler compiler;
    struct rc_instruction *inst;

    rc_init(&compiler.Base);

    inst = rc_insert_new_instruction(&compiler.Base, compiler.Base.Program.Instructions.Prev);
    inst->U.I.Opcode = RC_OPCODE_MOV;
    inst->U.I.DstReg.File = RC_FILE_OUTPUT;
    inst->U.I.DstReg.Index = VERT_RESULT_HPOS;
    inst->U.I.DstReg.RelAddr = 0;
    inst->U.I.DstReg.WriteMask = RC_MASK_XYZW;
    inst->U.I.SrcReg[0].Abs = 0;
    inst->U.I.SrcReg[0].File = RC_FILE_INPUT;
    inst->U.I.SrcReg[0].Index = VERT_ATTRIB_POS;
    inst->U.I.SrcReg[0].Negate = 0;
    inst->U.I.SrcReg[0].RelAddr = 0;
    inst->U.I.SrcReg[0].Swizzle = RC_SWIZZLE_XYZW;

    inst = rc_insert_new_instruction(&compiler.Base, compiler.Base.Program.Instructions.Prev);
    inst->U.I.Opcode = RC_OPCODE_MOV;
    inst->U.I.DstReg.File = RC_FILE_OUTPUT;
    inst->U.I.DstReg.Index = VERT_RESULT_TEX0;
    inst->U.I.DstReg.RelAddr = 0;
    inst->U.I.DstReg.WriteMask = RC_MASK_XYZW;
    inst->U.I.SrcReg[0].Abs = 0;
    inst->U.I.SrcReg[0].File = RC_FILE_INPUT;
    inst->U.I.SrcReg[0].Index = VERT_ATTRIB_TEX0;
    inst->U.I.SrcReg[0].Negate = 0;
    inst->U.I.SrcReg[0].RelAddr = 0;
    inst->U.I.SrcReg[0].Swizzle = RC_SWIZZLE_XYZW;

    compiler.Base.Program.InputsRead = (1 << VERT_ATTRIB_POS) | (1 << VERT_ATTRIB_TEX0);
    compiler.RequiredOutputs = compiler.Base.Program.OutputsWritten = (1 << VERT_RESULT_HPOS) | (1 << VERT_RESULT_TEX0);
    compiler.SetHwInputOutput = vp_ins_outs;
    compiler.code = &r300->blit.vp_code;

    r3xx_compile_vertex_program(&compiler);
}

static void create_fragment_program(struct r300_context *r300)
{
    struct r300_fragment_program_compiler compiler;
    struct rc_instruction *inst;

    memset(&compiler, 0, sizeof(struct r300_fragment_program_compiler));
    rc_init(&compiler.Base);

    inst = rc_insert_new_instruction(&compiler.Base, compiler.Base.Program.Instructions.Prev);
    inst->U.I.Opcode = RC_OPCODE_TEX;
    inst->U.I.TexSrcTarget = RC_TEXTURE_2D;
    inst->U.I.TexSrcUnit = 0;
    inst->U.I.DstReg.File = RC_FILE_OUTPUT;
    inst->U.I.DstReg.Index = FRAG_RESULT_COLOR;
    inst->U.I.DstReg.WriteMask = RC_MASK_XYZW;
    inst->U.I.SrcReg[0].Abs = 0;
    inst->U.I.SrcReg[0].File = RC_FILE_INPUT;
    inst->U.I.SrcReg[0].Index = FRAG_ATTRIB_TEX0;
    inst->U.I.SrcReg[0].Negate = 0;
    inst->U.I.SrcReg[0].RelAddr = 0;
    inst->U.I.SrcReg[0].Swizzle = RC_SWIZZLE_XYZW;

    compiler.Base.Program.InputsRead = (1 << FRAG_ATTRIB_TEX0);
    compiler.OutputColor[0] = FRAG_RESULT_COLOR;
    compiler.OutputDepth = FRAG_RESULT_DEPTH;
    compiler.is_r500 = (r300->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV515);
    compiler.code = &r300->blit.fp_code;
    compiler.AllocateHwInputs = fp_allocate_hw_inputs;

    r3xx_compile_fragment_program(&compiler);
}

void r300_blit_init(struct r300_context *r300)
{
    if (r300->options.hw_tcl_enabled)
	create_vertex_program(r300);
    create_fragment_program(r300);
}

static void r300_emit_tx_setup(struct r300_context *r300,
                               gl_format mesa_format,
                               struct radeon_bo *bo,
                               intptr_t offset,
                               unsigned width,
                               unsigned height,
                               unsigned pitch)
{
    BATCH_LOCALS(&r300->radeon);

    assert(width <= 2048);
    assert(height <= 2048);
    assert(r300TranslateTexFormat(mesa_format) >= 0);
    assert(offset % 32 == 0);

    BEGIN_BATCH(17);
    OUT_BATCH_REGVAL(R300_TX_FILTER0_0,
                     (R300_TX_CLAMP_TO_EDGE  << R300_TX_WRAP_S_SHIFT) |
                     (R300_TX_CLAMP_TO_EDGE  << R300_TX_WRAP_T_SHIFT) |
                     (R300_TX_CLAMP_TO_EDGE  << R300_TX_WRAP_R_SHIFT) |
                     R300_TX_MIN_FILTER_MIP_NONE |
                     R300_TX_MIN_FILTER_NEAREST |
                     R300_TX_MAG_FILTER_NEAREST |
                     (0 << 28));
    OUT_BATCH_REGVAL(R300_TX_FILTER1_0, 0);
    OUT_BATCH_REGVAL(R300_TX_SIZE_0,
                     ((width-1) << R300_TX_WIDTHMASK_SHIFT) |
                     ((height-1) << R300_TX_HEIGHTMASK_SHIFT) |
                     (0 << R300_TX_DEPTHMASK_SHIFT) |
                     (0 << R300_TX_MAX_MIP_LEVEL_SHIFT) |
                     R300_TX_SIZE_TXPITCH_EN);

    OUT_BATCH_REGVAL(R300_TX_FORMAT_0, r300TranslateTexFormat(mesa_format));
    OUT_BATCH_REGVAL(R300_TX_FORMAT2_0, pitch - 1);
    OUT_BATCH_REGSEQ(R300_TX_OFFSET_0, 1);
    OUT_BATCH_RELOC(0, bo, offset, RADEON_GEM_DOMAIN_GTT|RADEON_GEM_DOMAIN_VRAM, 0, 0);

    OUT_BATCH_REGSEQ(R300_TX_INVALTAGS, 2);
    OUT_BATCH(0);
    OUT_BATCH(1);

    END_BATCH();
}

#define EASY_US_FORMAT(FMT, C0, C1, C2, C3, SIGN) \
    (FMT  | R500_C0_SEL_##C0 | R500_C1_SEL_##C1 | \
    R500_C2_SEL_##C2 | R500_C3_SEL_##C3 | R500_OUT_SIGN(SIGN))

static uint32_t mesa_format_to_us_format(gl_format mesa_format)
{
    switch(mesa_format)
    {
        case MESA_FORMAT_RGBA8888: // x
            return EASY_US_FORMAT(R500_OUT_FMT_C4_8, A, B, G, R, 0);
        case MESA_FORMAT_RGB565: // x
        case MESA_FORMAT_ARGB1555: // x
        case MESA_FORMAT_RGBA8888_REV: // x
            return EASY_US_FORMAT(R500_OUT_FMT_C4_8, R, G, B, A, 0);
        case MESA_FORMAT_ARGB8888: // x
            return EASY_US_FORMAT(R500_OUT_FMT_C4_8, B, G, R, A, 0);
        case MESA_FORMAT_ARGB8888_REV:
            return EASY_US_FORMAT(R500_OUT_FMT_C4_8, A, R, G, B, 0);
        case MESA_FORMAT_XRGB8888:
            return EASY_US_FORMAT(R500_OUT_FMT_C4_8, A, R, G, B, 0);

        case MESA_FORMAT_RGB332:
            return EASY_US_FORMAT(R500_OUT_FMT_C_3_3_2, A, R, G, B, 0);

        case MESA_FORMAT_RGBA_FLOAT32:
            return EASY_US_FORMAT(R500_OUT_FMT_C4_32_FP, R, G, B, A, 0);
        case MESA_FORMAT_RGBA_FLOAT16:
            return EASY_US_FORMAT(R500_OUT_FMT_C4_16_FP, R, G, B, A, 0);
        case MESA_FORMAT_ALPHA_FLOAT32:
            return EASY_US_FORMAT(R500_OUT_FMT_C_32_FP, A, A, A, A, 0);
        case MESA_FORMAT_ALPHA_FLOAT16:
            return EASY_US_FORMAT(R500_OUT_FMT_C_16_FP, A, A, A, A, 0);

        case MESA_FORMAT_SIGNED_RGBA8888:
            return EASY_US_FORMAT(R500_OUT_FMT_C4_8, R, G, B, A, 0xf);
        case MESA_FORMAT_SIGNED_RGBA8888_REV:
            return EASY_US_FORMAT(R500_OUT_FMT_C4_8, A, B, G, R, 0xf);
        case MESA_FORMAT_SIGNED_RGBA_16:
            return EASY_US_FORMAT(R500_OUT_FMT_C4_16, R, G, B, A, 0xf);

        default:
            fprintf(stderr, "Unsupported format %s for US output\n", _mesa_get_format_name(mesa_format));
            assert(0);
            return 0;
    }
}
#undef EASY_US_FORMAT

static void r500_emit_fp_setup(struct r300_context *r300,
                               struct r500_fragment_program_code *fp,
                               gl_format dst_format)
{
    r500_emit_fp(r300, (uint32_t *)fp->inst, (fp->inst_end + 1) * 6, 0, 0, 0);
    BATCH_LOCALS(&r300->radeon);

    BEGIN_BATCH(10);
    OUT_BATCH_REGSEQ(R500_US_CODE_ADDR, 3);
    OUT_BATCH(R500_US_CODE_START_ADDR(0) | R500_US_CODE_END_ADDR(fp->inst_end));
    OUT_BATCH(R500_US_CODE_RANGE_ADDR(0) | R500_US_CODE_RANGE_SIZE(fp->inst_end));
    OUT_BATCH(0);
    OUT_BATCH_REGVAL(R500_US_CONFIG, 0);
    OUT_BATCH_REGVAL(R500_US_OUT_FMT_0, mesa_format_to_us_format(dst_format));
    OUT_BATCH_REGVAL(R500_US_PIXSIZE, fp->max_temp_idx);
    END_BATCH();
}

static void r500_emit_rs_setup(struct r300_context *r300)
{
    BATCH_LOCALS(&r300->radeon);

    BEGIN_BATCH(7);
    OUT_BATCH_REGSEQ(R300_RS_COUNT, 2);
    OUT_BATCH((4 << R300_IT_COUNT_SHIFT) | R300_HIRES_EN);
    OUT_BATCH(0);
    OUT_BATCH_REGVAL(R500_RS_INST_0,
                     (0 << R500_RS_INST_TEX_ID_SHIFT) |
                     (0 << R500_RS_INST_TEX_ADDR_SHIFT) |
                     R500_RS_INST_TEX_CN_WRITE |
                     R500_RS_INST_COL_CN_NO_WRITE);
    OUT_BATCH_REGVAL(R500_RS_IP_0,
                     (0 << R500_RS_IP_TEX_PTR_S_SHIFT) |
                     (1 << R500_RS_IP_TEX_PTR_T_SHIFT) |
                     (2 << R500_RS_IP_TEX_PTR_R_SHIFT) |
                     (3 << R500_RS_IP_TEX_PTR_Q_SHIFT));
    END_BATCH();
}

static void r300_emit_fp_setup(struct r300_context *r300,
                               struct r300_fragment_program_code *code,
                               gl_format dst_format)
{
    unsigned i;
    BATCH_LOCALS(&r300->radeon);

    BEGIN_BATCH((code->alu.length + 1) * 4 + code->tex.length + 1 + 11);

    OUT_BATCH_REGSEQ(R300_US_ALU_RGB_INST_0, code->alu.length);
    for (i = 0; i < code->alu.length; i++) {
        OUT_BATCH(code->alu.inst[i].rgb_inst);
    }
    OUT_BATCH_REGSEQ(R300_US_ALU_RGB_ADDR_0, code->alu.length);
    for (i = 0; i < code->alu.length; i++) {
        OUT_BATCH(code->alu.inst[i].rgb_addr);
    }
    OUT_BATCH_REGSEQ(R300_US_ALU_ALPHA_INST_0, code->alu.length);
    for (i = 0; i < code->alu.length; i++) {
        OUT_BATCH(code->alu.inst[i].alpha_inst);
    }
    OUT_BATCH_REGSEQ(R300_US_ALU_ALPHA_ADDR_0, code->alu.length);
    for (i = 0; i < code->alu.length; i++) {
        OUT_BATCH(code->alu.inst[i].alpha_addr);
    }

    OUT_BATCH_REGSEQ(R300_US_TEX_INST_0, code->tex.length);
    OUT_BATCH_TABLE(code->tex.inst, code->tex.length);

    OUT_BATCH_REGSEQ(R300_US_CONFIG, 3);
    OUT_BATCH(R300_PFS_CNTL_FIRST_NODE_HAS_TEX);
    OUT_BATCH(code->pixsize);
    OUT_BATCH(code->code_offset);
    OUT_BATCH_REGSEQ(R300_US_CODE_ADDR_0, 4);
    OUT_BATCH_TABLE(code->code_addr, 4);
    OUT_BATCH_REGVAL(R500_US_OUT_FMT_0, mesa_format_to_us_format(dst_format));
    END_BATCH();
}

static void r300_emit_rs_setup(struct r300_context *r300)
{
    BATCH_LOCALS(&r300->radeon);

    BEGIN_BATCH(7);
    OUT_BATCH_REGSEQ(R300_RS_COUNT, 2);
    OUT_BATCH((4 << R300_IT_COUNT_SHIFT) | R300_HIRES_EN);
    OUT_BATCH(0);
    OUT_BATCH_REGVAL(R300_RS_INST_0,
                     R300_RS_INST_TEX_ID(0) |
                     R300_RS_INST_TEX_ADDR(0) |
                     R300_RS_INST_TEX_CN_WRITE);
    OUT_BATCH_REGVAL(R300_RS_IP_0,
                     R300_RS_TEX_PTR(0) |
                     R300_RS_SEL_S(R300_RS_SEL_C0) |
                     R300_RS_SEL_T(R300_RS_SEL_C1) |
                     R300_RS_SEL_R(R300_RS_SEL_K0) |
                     R300_RS_SEL_Q(R300_RS_SEL_K1));
    END_BATCH();
}

static void emit_pvs_setup(struct r300_context *r300,
                           uint32_t *vp_code,
                           unsigned vp_len)
{
    BATCH_LOCALS(&r300->radeon);

    r300_emit_vpu(r300, vp_code, vp_len * 4, R300_PVS_CODE_START);

    BEGIN_BATCH(4);
    OUT_BATCH_REGSEQ(R300_VAP_PVS_CODE_CNTL_0, 3);
    OUT_BATCH((0 << R300_PVS_FIRST_INST_SHIFT) |
              ((vp_len - 1)  << R300_PVS_XYZW_VALID_INST_SHIFT) |
              ((vp_len - 1)<< R300_PVS_LAST_INST_SHIFT));
    OUT_BATCH(0);
    OUT_BATCH((vp_len - 1) << R300_PVS_LAST_VTX_SRC_INST_SHIFT);
    END_BATCH();
}

static void emit_vap_setup(struct r300_context *r300)
{
    int tex_offset;
    BATCH_LOCALS(&r300->radeon);

    if (r300->options.hw_tcl_enabled)
	tex_offset = 1;
    else
	tex_offset = 6;

    BEGIN_BATCH(12);
    OUT_BATCH_REGSEQ(R300_SE_VTE_CNTL, 2);
    OUT_BATCH(R300_VTX_XY_FMT | R300_VTX_Z_FMT);
    OUT_BATCH(4);

    OUT_BATCH_REGVAL(R300_VAP_PSC_SGN_NORM_CNTL, 0xaaaaaaaa);
    OUT_BATCH_REGVAL(R300_VAP_PROG_STREAM_CNTL_0,
                     ((R300_DATA_TYPE_FLOAT_2 | (0 << R300_DST_VEC_LOC_SHIFT)) << 0) |
                     (((tex_offset << R300_DST_VEC_LOC_SHIFT) | R300_DATA_TYPE_FLOAT_2 | R300_LAST_VEC) << 16));
    OUT_BATCH_REGVAL(R300_VAP_PROG_STREAM_CNTL_EXT_0,
                    ((((R300_SWIZZLE_SELECT_X << R300_SWIZZLE_SELECT_X_SHIFT) |
                       (R300_SWIZZLE_SELECT_Y << R300_SWIZZLE_SELECT_Y_SHIFT) |
                       (R300_SWIZZLE_SELECT_FP_ZERO << R300_SWIZZLE_SELECT_Z_SHIFT) |
                       (R300_SWIZZLE_SELECT_FP_ONE << R300_SWIZZLE_SELECT_W_SHIFT) | 
                       (0xf << R300_WRITE_ENA_SHIFT) ) << 0) |
                     (((R300_SWIZZLE_SELECT_X << R300_SWIZZLE_SELECT_X_SHIFT) |
                       (R300_SWIZZLE_SELECT_Y << R300_SWIZZLE_SELECT_Y_SHIFT) |
                       (R300_SWIZZLE_SELECT_FP_ZERO << R300_SWIZZLE_SELECT_Z_SHIFT) |
                       (R300_SWIZZLE_SELECT_FP_ONE << R300_SWIZZLE_SELECT_W_SHIFT) |
                       (0xf << R300_WRITE_ENA_SHIFT) ) << 16) ) );
    OUT_BATCH_REGSEQ(R300_VAP_OUTPUT_VTX_FMT_0, 2);
    OUT_BATCH(R300_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT);
    OUT_BATCH(R300_VAP_OUTPUT_VTX_FMT_1__4_COMPONENTS);
    END_BATCH();
}

static GLboolean validate_buffers(struct r300_context *r300,
                                  struct radeon_bo *src_bo,
                                  struct radeon_bo *dst_bo)
{
    int ret;

    radeon_cs_space_reset_bos(r300->radeon.cmdbuf.cs);

    ret = radeon_cs_space_check_with_bo(r300->radeon.cmdbuf.cs,
                                        src_bo, RADEON_GEM_DOMAIN_VRAM | RADEON_GEM_DOMAIN_GTT, 0);
    if (ret)
        return GL_FALSE;

    ret = radeon_cs_space_check_with_bo(r300->radeon.cmdbuf.cs,
                                        dst_bo, 0, RADEON_GEM_DOMAIN_VRAM | RADEON_GEM_DOMAIN_GTT);
    if (ret)
        return GL_FALSE;

    return GL_TRUE;
}

/**
 * Calculate texcoords for given image region.
 * Output values are [minx, maxx, miny, maxy]
 */
static void calc_tex_coords(float img_width, float img_height,
                            float x, float y,
                            float reg_width, float reg_height,
                            unsigned flip_y, float *buf)
{
    buf[0] = x / img_width;
    buf[1] = buf[0] + reg_width / img_width;
    buf[2] = y / img_height;
    buf[3] = buf[2] + reg_height / img_height;
    if (flip_y)
    {
        buf[2] = 1.0 - buf[2];
        buf[3] = 1.0 - buf[3];
    }
}

static void emit_draw_packet(struct r300_context *r300,
                             unsigned src_width, unsigned src_height,
                             unsigned src_x_offset, unsigned src_y_offset,
                             unsigned dst_x_offset, unsigned dst_y_offset,
                             unsigned reg_width, unsigned reg_height,
                             unsigned flip_y)
{
    float texcoords[4];

    calc_tex_coords(src_width, src_height,
                    src_x_offset, src_y_offset,
                    reg_width, reg_height,
                    flip_y, texcoords);

    float verts[] = { dst_x_offset, dst_y_offset,
                      texcoords[0], texcoords[2],
                      dst_x_offset, dst_y_offset + reg_height,
                      texcoords[0], texcoords[3],
                      dst_x_offset + reg_width, dst_y_offset + reg_height,
                      texcoords[1], texcoords[3],
                      dst_x_offset + reg_width, dst_y_offset,
                      texcoords[1], texcoords[2] };

    BATCH_LOCALS(&r300->radeon);

    BEGIN_BATCH(19);
    OUT_BATCH_PACKET3(R300_PACKET3_3D_DRAW_IMMD_2, 16);
    OUT_BATCH(R300_VAP_VF_CNTL__PRIM_WALK_VERTEX_EMBEDDED |
              (4 << 16) | R300_VAP_VF_CNTL__PRIM_QUADS);
    OUT_BATCH_TABLE(verts, 16);
    END_BATCH();
}

static void other_stuff(struct r300_context *r300)
{
    BATCH_LOCALS(&r300->radeon);

    BEGIN_BATCH(13);
    OUT_BATCH_REGVAL(R300_GA_POLY_MODE,
                     R300_GA_POLY_MODE_FRONT_PTYPE_TRI | R300_GA_POLY_MODE_BACK_PTYPE_TRI);
    OUT_BATCH_REGVAL(R300_SU_CULL_MODE, R300_FRONT_FACE_CCW);
    OUT_BATCH_REGVAL(R300_FG_FOG_BLEND, 0);
    OUT_BATCH_REGVAL(R300_FG_ALPHA_FUNC, 0);
    OUT_BATCH_REGSEQ(R300_RB3D_CBLEND, 2);
    OUT_BATCH(0x0);
    OUT_BATCH(0x0);
    OUT_BATCH_REGVAL(R300_ZB_CNTL, 0);
    END_BATCH();
    if (r300->options.hw_tcl_enabled) {
        BEGIN_BATCH(2);
        OUT_BATCH_REGVAL(R300_VAP_CLIP_CNTL, R300_CLIP_DISABLE);
        END_BATCH();
    }
}

static void emit_cb_setup(struct r300_context *r300,
                          struct radeon_bo *bo,
                          intptr_t offset,
                          gl_format mesa_format,
                          unsigned pitch,
                          unsigned width,
                          unsigned height)
{
    BATCH_LOCALS(&r300->radeon);

    unsigned x1, y1, x2, y2;
    x1 = 0;
    y1 = 0;
    x2 = width - 1;
    y2 = height - 1;

    if (r300->radeon.radeonScreen->chip_family < CHIP_FAMILY_RV515) {
        x1 += R300_SCISSORS_OFFSET;
        y1 += R300_SCISSORS_OFFSET;
        x2 += R300_SCISSORS_OFFSET;
        y2 += R300_SCISSORS_OFFSET;
    }

    r300_emit_cb_setup(r300, bo, offset, mesa_format,
                       _mesa_get_format_bytes(mesa_format),
                       _mesa_format_row_stride(mesa_format, pitch));

    BEGIN_BATCH_NO_AUTOSTATE(5);
    OUT_BATCH_REGSEQ(R300_SC_SCISSORS_TL, 2);
    OUT_BATCH((x1 << R300_SCISSORS_X_SHIFT)|(y1 << R300_SCISSORS_Y_SHIFT));
    OUT_BATCH((x2 << R300_SCISSORS_X_SHIFT)|(y2 << R300_SCISSORS_Y_SHIFT));
    OUT_BATCH_REGVAL(R300_RB3D_CCTL, 0);
    END_BATCH();
}

unsigned r300_check_blit(gl_format dst_format)
{
    switch (dst_format) {
        case MESA_FORMAT_RGB565:
        case MESA_FORMAT_ARGB1555:
        case MESA_FORMAT_RGBA8888:
        case MESA_FORMAT_RGBA8888_REV:
        case MESA_FORMAT_ARGB8888:
        case MESA_FORMAT_ARGB8888_REV:
        case MESA_FORMAT_XRGB8888:
            break;
        default:
            return 0;
    }

    if (_mesa_get_format_bits(dst_format, GL_DEPTH_BITS) > 0)
        return 0;

    return 1;
}

/**
 * Copy a region of [@a width x @a height] pixels from source buffer
 * to destination buffer.
 * @param[in] r300 r300 context
 * @param[in] src_bo source radeon buffer object
 * @param[in] src_offset offset of the source image in the @a src_bo
 * @param[in] src_mesaformat source image format
 * @param[in] src_pitch aligned source image width
 * @param[in] src_width source image width
 * @param[in] src_height source image height
 * @param[in] src_x_offset x offset in the source image
 * @param[in] src_y_offset y offset in the source image
 * @param[in] dst_bo destination radeon buffer object
 * @param[in] dst_offset offset of the destination image in the @a dst_bo
 * @param[in] dst_mesaformat destination image format
 * @param[in] dst_pitch aligned destination image width
 * @param[in] dst_width destination image width
 * @param[in] dst_height destination image height
 * @param[in] dst_x_offset x offset in the destination image
 * @param[in] dst_y_offset y offset in the destination image
 * @param[in] width region width
 * @param[in] height region height
 * @param[in] flip_y set if y coords of the source image need to be flipped
 */
unsigned r300_blit(GLcontext *ctx,
                   struct radeon_bo *src_bo,
                   intptr_t src_offset,
                   gl_format src_mesaformat,
                   unsigned src_pitch,
                   unsigned src_width,
                   unsigned src_height,
                   unsigned src_x_offset,
                   unsigned src_y_offset,
                   struct radeon_bo *dst_bo,
                   intptr_t dst_offset,
                   gl_format dst_mesaformat,
                   unsigned dst_pitch,
                   unsigned dst_width,
                   unsigned dst_height,
                   unsigned dst_x_offset,
                   unsigned dst_y_offset,
                   unsigned reg_width,
                   unsigned reg_height,
                   unsigned flip_y)
{
    r300ContextPtr r300 = R300_CONTEXT(ctx);

    if (!r300_check_blit(dst_mesaformat))
        return 0;

    /* Make sure that colorbuffer has even width - hw limitation */
    if (dst_pitch % 2 > 0)
        ++dst_pitch;

    /* Rendering to small buffer doesn't work.
     * Looks like a hw limitation.
     */
    if (dst_pitch < 32)
        return 0;

    /* Need to clamp the region size to make sure
     * we don't read outside of the source buffer
     * or write outside of the destination buffer.
     */
    if (reg_width + src_x_offset > src_width)
        reg_width = src_width - src_x_offset;
    if (reg_height + src_y_offset > src_height)
        reg_height = src_height - src_y_offset;
    if (reg_width + dst_x_offset > dst_width)
        reg_width = dst_width - dst_x_offset;
    if (reg_height + dst_y_offset > dst_height)
        reg_height = dst_height - dst_y_offset;

    if (src_bo == dst_bo) {
        return 0;
    }

    if (src_offset % 32 || dst_offset % 32) {
        return GL_FALSE;
    }

    if (0) {
        fprintf(stderr, "src: size [%d x %d], pitch %d, "
                "offset [%d x %d], format %s, bo %p\n",
                src_width, src_height, src_pitch,
                src_x_offset, src_y_offset,
                _mesa_get_format_name(src_mesaformat),
                src_bo);
        fprintf(stderr, "dst: pitch %d, offset[%d x %d], format %s, bo %p\n",
                dst_pitch, dst_x_offset, dst_y_offset,
                _mesa_get_format_name(dst_mesaformat), dst_bo);
        fprintf(stderr, "region: %d x %d\n", reg_width, reg_height);
    }

    /* Flush is needed to make sure that source buffer has correct data */
    radeonFlush(r300->radeon.glCtx);

    if (!validate_buffers(r300, src_bo, dst_bo))
        return 0;

    rcommonEnsureCmdBufSpace(&r300->radeon, 200, __FUNCTION__);

    other_stuff(r300);

    r300_emit_tx_setup(r300, src_mesaformat, src_bo, src_offset, src_width, src_height, src_pitch);

    if (r300->radeon.radeonScreen->chip_family >= CHIP_FAMILY_RV515) {
        r500_emit_fp_setup(r300, &r300->blit.fp_code.code.r500, dst_mesaformat);
        r500_emit_rs_setup(r300);
    } else {
        r300_emit_fp_setup(r300, &r300->blit.fp_code.code.r300, dst_mesaformat);
        r300_emit_rs_setup(r300);
    }

    if (r300->options.hw_tcl_enabled)
	emit_pvs_setup(r300, r300->blit.vp_code.body.d, 2);

    emit_vap_setup(r300);

    emit_cb_setup(r300, dst_bo, dst_offset, dst_mesaformat, dst_pitch, dst_width, dst_height);

    emit_draw_packet(r300, src_width, src_height,
                     src_x_offset, src_y_offset,
                     dst_x_offset, dst_y_offset,
                     reg_width, reg_height,
                     flip_y);

    r300EmitCacheFlush(r300);

    radeonFlush(r300->radeon.glCtx);

    return 1;
}
