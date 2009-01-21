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

#include "r300_surface.h"

/* Provides pipe_context's "surface_fill". Commonly used for clearing
 * buffers. */
static void r300_surface_fill(struct pipe_context* pipe,
                              struct pipe_surface* dest,
                              unsigned x, unsigned y,
                              unsigned w, unsigned h,
                              unsigned color)
{
    struct r300_context* context = r300_context(pipe);
    CS_LOCALS(context);
    boolean has_tcl = FALSE;
    boolean is_r500 = FALSE;
    /* Emit a shitload of state, and then draw a point to clear the buffer.
     * XXX it goes without saying that this needs to be cleaned up and
     * shifted around to work with the rest of the driver's state handling.
     */
    /* Sequence starting at R300_VAP_PROG_STREAM_CNTL_0 */
    OUT_CS_REG_SEQ(R300_VAP_PROG_STREAM_CNTL_0, 1);
    if (has_tcl) {
        OUT_CS(((((0 << R300_DST_VEC_LOC_SHIFT) | R300_DATA_TYPE_FLOAT_4) <<
                R300_DATA_TYPE_0_SHIFT) | ((R300_LAST_VEC | (1 <<
                R300_DST_VEC_LOC_SHIFT) | R300_DATA_TYPE_FLOAT_4) <<
                R300_DATA_TYPE_1_SHIFT)));
    } else {
        OUT_CS(((((0 << R300_DST_VEC_LOC_SHIFT) | R300_DATA_TYPE_FLOAT_4) <<
                R300_DATA_TYPE_0_SHIFT) | ((R300_LAST_VEC | (2 <<
                R300_DST_VEC_LOC_SHIFT) | R300_DATA_TYPE_FLOAT_4) <<
                R300_DATA_TYPE_1_SHIFT)));
    }

    /* Disable fog */
    OUT_CS_REG(R300_FG_FOG_BLEND, 0);
    OUT_CS_REG(R300_FG_ALPHA_FUNC, 0);

    OUT_CS_REG(R300_VAP_PROG_STREAM_CNTL_EXT_0,
               ((((R300_SWIZZLE_SELECT_X << R300_SWIZZLE_SELECT_X_SHIFT) |
                       (R300_SWIZZLE_SELECT_Y << R300_SWIZZLE_SELECT_Y_SHIFT) |
                       (R300_SWIZZLE_SELECT_Z << R300_SWIZZLE_SELECT_Z_SHIFT) |
                       (R300_SWIZZLE_SELECT_W << R300_SWIZZLE_SELECT_W_SHIFT) |
                       ((R300_WRITE_ENA_X | R300_WRITE_ENA_Y |
                       R300_WRITE_ENA_Z | R300_WRITE_ENA_W) <<
                       R300_WRITE_ENA_SHIFT)) << R300_SWIZZLE0_SHIFT) |
                       (((R300_SWIZZLE_SELECT_X << R300_SWIZZLE_SELECT_X_SHIFT) |
                       (R300_SWIZZLE_SELECT_Y << R300_SWIZZLE_SELECT_Y_SHIFT) |
                       (R300_SWIZZLE_SELECT_Z << R300_SWIZZLE_SELECT_Z_SHIFT) |
                       (R300_SWIZZLE_SELECT_W << R300_SWIZZLE_SELECT_W_SHIFT) |
                       ((R300_WRITE_ENA_X | R300_WRITE_ENA_Y |
                       R300_WRITE_ENA_Z | R300_WRITE_ENA_W) <<
                       R300_WRITE_ENA_SHIFT)) << R300_SWIZZLE1_SHIFT)));
    /* R300_VAP_INPUT_CNTL_0, R300_VAP_INPUT_CNTL_1 */
    OUT_CS_REG_SEQ(R300_VAP_VTX_STATE_CNTL, 2);
    OUT_CS((R300_SEL_USER_COLOR_0 << R300_COLOR_0_ASSEMBLY_SHIFT));
    OUT_CS(R300_INPUT_CNTL_POS | R300_INPUT_CNTL_COLOR | R300_INPUT_CNTL_TC0);

    /* comes from fglrx startup of clear */
    OUT_CS_REG_SEQ(R300_SE_VTE_CNTL, 2);
    OUT_CS(R300_VTX_W0_FMT | R300_VPORT_X_SCALE_ENA |
            R300_VPORT_X_OFFSET_ENA | R300_VPORT_Y_SCALE_ENA |
            R300_VPORT_Y_OFFSET_ENA | R300_VPORT_Z_SCALE_ENA |
            R300_VPORT_Z_OFFSET_ENA);
    OUT_CS(0x8);

    OUT_CS_REG(R300_VAP_PSC_SGN_NORM_CNTL, 0xaaaaaaaa);

    OUT_CS_REG_SEQ(R300_VAP_OUTPUT_VTX_FMT_0, 2);
    OUT_CS(R300_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT |
            R300_VAP_OUTPUT_VTX_FMT_0__COLOR_0_PRESENT);
    OUT_CS(0); /* no textures */

    OUT_CS_REG(R300_TX_ENABLE, 0);

    OUT_CS_REG_SEQ(R300_SE_VPORT_XSCALE, 6);
    OUT_CS_32F(1.0);
    OUT_CS_32F(x);
    OUT_CS_32F(1.0);
    OUT_CS_32F(y);
    OUT_CS_32F(1.0);
    OUT_CS_32F(0.0);

    OUT_CS_REG_SEQ(R300_RB3D_CBLEND, 2);
    OUT_CS(0x0);
    OUT_CS(0x0);

    OUT_CS_REG(R300_VAP_CLIP_CNTL, R300_PS_UCP_MODE_CLIP_AS_TRIFAN | R300_CLIP_DISABLE);

    OUT_CS_REG(R300_GA_POINT_SIZE, ((w * 6) << R300_POINTSIZE_X_SHIFT) |
            ((h * 6) << R300_POINTSIZE_Y_SHIFT));

    if (is_r500) {
        OUT_CS_REG_SEQ(R500_RS_IP_0, 8);
        for (i = 0; i < 8; ++i) {
            OUT_CS((R500_RS_IP_PTR_K0 << R500_RS_IP_TEX_PTR_S_SHIFT) |
                    (R500_RS_IP_PTR_K0 << R500_RS_IP_TEX_PTR_T_SHIFT) |
                    (R500_RS_IP_PTR_K0 << R500_RS_IP_TEX_PTR_R_SHIFT) |
                    (R500_RS_IP_PTR_K1 << R500_RS_IP_TEX_PTR_Q_SHIFT));
        }

        OUT_CS_REG_SEQ(R300_RS_COUNT, 2);
        /* XXX could hires be disabled for a speed boost? */
        OUT_CS((1 << R300_IC_COUNT_SHIFT) | R300_HIRES_EN);
        OUT_CS(0x0);

        OUT_CS_REG(R500_RS_INST_0, R500_RS_INST_COL_CN_WRITE);
    } else {
        OUT_CS_REG(R300_RS_IP_0, 8);
        for (i = 0; i < 8; ++i) {
            OUT_CS(R300_RS_SEL_T(1) | R300_RS_SEL_R(2) | R300_RS_SEL_Q(3));
        }

        OUT_CS_REG_SEQ(R300_RS_COUNT, 2);
        /* XXX could hires be disabled for a speed boost? */
        OUT_CS((1 << R300_IC_COUNT_SHIFT) | R300_HIRES_EN);
        OUT_CS(0x0);

        OUT_CS_REG(R300_RS_INST_0, R300_RS_INST_COL_CN_WRITE);
    }

    if (is_r500) {
        OUT_CS_REG_SEQ(R500_US_CONFIG, 2);
        OUT_CS(R500_ZERO_TIMES_ANYTHING_EQUALS_ZERO);
        OUT_CS(0x0);
        OUT_CS_REG_SEQ(R500_US_CODE_ADDR, 3);
        OUT_CS(R500_US_CODE_START_ADDR(0) | R500_US_CODE_END_ADDR(1));
        OUT_CS(R500_US_CODE_RANGE_ADDR(0) | R500_US_CODE_RANGE_SIZE(1));
        OUT_CS(R500_US_CODE_OFFSET_ADDR(0));

        OUT_CS_REG(R500_GA_US_VECTOR_INDEX, 0x0);

        OUT_CS_REG(R500_GA_US_VECTOR_DATA, R500_INST_TYPE_OUT |
                R500_INST_TEX_SEM_WAIT |
                R500_INST_LAST |
                R500_INST_RGB_OMASK_R |
                R500_INST_RGB_OMASK_G |
                R500_INST_RGB_OMASK_B |
                R500_INST_ALPHA_OMASK |
                R500_INST_RGB_CLAMP |
                R500_INST_ALPHA_CLAMP);
        OUT_CS_REG(R500_GA_US_VECTOR_DATA, R500_RGB_ADDR0(0) |
                R500_RGB_ADDR1(0) |
                R500_RGB_ADDR1_CONST |
                R500_RGB_ADDR2(0) |
                R500_RGB_ADDR2_CONST);
        OUT_CS_REG(R500_GA_US_VECTOR_DATA, R500_ALPHA_ADDR0(0) |
                R500_ALPHA_ADDR1(0) |
                R500_ALPHA_ADDR1_CONST |
                R500_ALPHA_ADDR2(0) |
                R500_ALPHA_ADDR2_CONST);
        OUT_CS_REG(R500_GA_US_VECTOR_DATA, R500_ALU_RGB_SEL_A_SRC0 |
                R500_ALU_RGB_R_SWIZ_A_R |
                R500_ALU_RGB_G_SWIZ_A_G |
                R500_ALU_RGB_B_SWIZ_A_B |
                R500_ALU_RGB_SEL_B_SRC0 |
                R500_ALU_RGB_R_SWIZ_B_R |
                R500_ALU_RGB_B_SWIZ_B_G |
                R500_ALU_RGB_G_SWIZ_B_B);
        OUT_CS_REG(R500_GA_US_VECTOR_DATA, R500_ALPHA_OP_CMP |
                R500_ALPHA_SWIZ_A_A |
                R500_ALPHA_SWIZ_B_A);
        OUT_CS_REG(R500_GA_US_VECTOR_DATA, R500_ALU_RGBA_OP_CMP |
                R500_ALU_RGBA_R_SWIZ_0 |
                R500_ALU_RGBA_G_SWIZ_0 |
                R500_ALU_RGBA_B_SWIZ_0 |
                R500_ALU_RGBA_A_SWIZ_0);

    } else {
        OUT_CS_REG_SEQ(R300_US_CONFIG, 3);
        OUT_CS(0x0);
        OUT_CS(0x0);
        OUT_CS(0x0);
        OUT_CS_REG_SEQ(R300_US_CODE_ADDR_0, 4);
        OUT_CS(0x0);
        OUT_CS(0x0);
        OUT_CS(0x0);
        OUT_CS(R300_RGBA_OUT);

        OUT_CS_REG(R300_US_ALU_RGB_INST_0,
                   FP_INSTRC(MAD, FP_ARGC(SRC0C_XYZ), FP_ARGC(ONE), FP_ARGC(ZERO)));
        OUT_CS_REG(R300_US_ALU_RGB_ADDR_0,
                   FP_SELC(0, NO, XYZ, FP_TMP(0), 0, 0));
        OUT_CS_REG(R300_US_ALU_ALPHA_INST_0,
                   FP_INSTRA(MAD, FP_ARGA(SRC0A), FP_ARGA(ONE), FP_ARGA(ZERO)));
        OUT_CS_REG(R300_US_ALU_ALPHA_ADDR_0,
                   FP_SELA(0, NO, W, FP_TMP(0), 0, 0));
    }

    /* XXX */
    uint32_t vap_cntl;
    OUT_CS_REG(R300_VAP_PVS_STATE_FLUSH_REG, 0);
    if (has_tcl) {
        vap_cntl = ((10 << R300_PVS_NUM_SLOTS_SHIFT) |
                (5 << R300_PVS_NUM_CNTLRS_SHIFT) |
                (12 << R300_VF_MAX_VTX_NUM_SHIFT));
        if (CHIP_FAMILY_RV515)
            vap_cntl |= R500_TCL_STATE_OPTIMIZATION;
    } else {
        vap_cntl = ((10 << R300_PVS_NUM_SLOTS_SHIFT) |
                (5 << R300_PVS_NUM_CNTLRS_SHIFT) |
                (5 << R300_VF_MAX_VTX_NUM_SHIFT));
    }

    if (CHIP_FAMILY_RV515)
        vap_cntl |= (2 << R300_PVS_NUM_FPUS_SHIFT);
    else if ((CHIP_FAMILY_RV530) ||
              (CHIP_FAMILY_RV560) ||
              (CHIP_FAMILY_RV570))
        vap_cntl |= (5 << R300_PVS_NUM_FPUS_SHIFT);
    else if ((CHIP_FAMILY_RV410) ||
              (CHIP_FAMILY_R420))
        vap_cntl |= (6 << R300_PVS_NUM_FPUS_SHIFT);
    else if ((CHIP_FAMILY_R520) ||
              (CHIP_FAMILY_R580))
        vap_cntl |= (8 << R300_PVS_NUM_FPUS_SHIFT);
    else
        vap_cntl |= (4 << R300_PVS_NUM_FPUS_SHIFT);

    OUT_CS_REG(R300_VAP_CNTL, vap_cntl);

    if (has_tcl) {
        OUT_CS_REG_SEQ(R300_VAP_PVS_CODE_CNTL_0, 3);
        OUT_CS((0 << R300_PVS_FIRST_INST_SHIFT) |
                (0 << R300_PVS_XYZW_VALID_INST_SHIFT) |
                (1 << R300_PVS_LAST_INST_SHIFT));
        OUT_CS((0 << R300_PVS_CONST_BASE_OFFSET_SHIFT) |
                (0 << R300_PVS_MAX_CONST_ADDR_SHIFT));
        OUT_CS(1 << R300_PVS_LAST_VTX_SRC_INST_SHIFT);

        OUT_CS_REG(R300_SC_SCREENDOOR, 0x0);
        OUT_CS_REG(RADEON_WAIT_UNTIL, (1 << 15) | (1 << 28));
        OUT_CS_REG(R300_SC_SCREENDOOR, 0x00FFFFFF);
        OUT_CS_REG(R300_VAP_PVS_STATE_FLUSH_REG, 0x1);
        OUT_CS_REG(R300_VAP_PVS_UPLOAD_ADDRESS, 0x0);

        OUT_CS_REG(R300_VAP_PVS_UPLOAD_DATA, PVS_OP_DST_OPERAND(VE_ADD, GL_FALSE, GL_FALSE,
                                        0, 0xf, PVS_DST_REG_OUT));
        OUT_CS_REG(R300_VAP_PVS_UPLOAD_DATA, PVS_SRC_OPERAND(0, PVS_SRC_SELECT_X, PVS_SRC_SELECT_Y,
                                     PVS_SRC_SELECT_Z, PVS_SRC_SELECT_W,
                                     PVS_SRC_REG_INPUT, VSF_FLAG_NONE));
        OUT_CS_REG(R300_VAP_PVS_UPLOAD_DATA, PVS_SRC_OPERAND(0, PVS_SRC_SELECT_FORCE_0,
                                     PVS_SRC_SELECT_FORCE_0,
                                     PVS_SRC_SELECT_FORCE_0,
                                     PVS_SRC_SELECT_FORCE_0,
                                     PVS_SRC_REG_INPUT, VSF_FLAG_NONE));
        OUT_CS_REG(R300_VAP_PVS_UPLOAD_DATA, 0x0);

        OUT_CS_REG(R300_VAP_PVS_UPLOAD_DATA, PVS_OP_DST_OPERAND(VE_ADD, GL_FALSE, GL_FALSE, 1, 0xf,
                                        PVS_DST_REG_OUT));
        OUT_CS_REG(R300_VAP_PVS_UPLOAD_DATA, PVS_SRC_OPERAND(1, PVS_SRC_SELECT_X,
                                     PVS_SRC_SELECT_Y, PVS_SRC_SELECT_Z,
                                     PVS_SRC_SELECT_W, PVS_SRC_REG_INPUT,
                                     VSF_FLAG_NONE));
        OUT_CS_REG(R300_VAP_PVS_UPLOAD_DATA, PVS_SRC_OPERAND(1, PVS_SRC_SELECT_FORCE_0,
                                     PVS_SRC_SELECT_FORCE_0,
                                     PVS_SRC_SELECT_FORCE_0,
                                     PVS_SRC_SELECT_FORCE_0,
                                     PVS_SRC_REG_INPUT, VSF_FLAG_NONE));
        OUT_CS_REG(R300_VAP_PVS_UPLOAD_DATA, 0x0);
    }
    /* Do the actual emit. */
    if (rrb) {
        cbpitch = (rrb->pitch / rrb->cpp);
        if (rrb->cpp == 4)
            cbpitch |= R300_COLOR_FORMAT_ARGB8888;
        else
            cbpitch |= R300_COLOR_FORMAT_RGB565;

        if (rrb->bo->flags & RADEON_BO_FLAGS_MACRO_TILE){
            cbpitch |= R300_COLOR_TILE_ENABLE;
        }
    }

    /* TODO in bufmgr */
    cp_wait(r300, R300_WAIT_3D | R300_WAIT_3D_CLEAN);
    end_3d(rmesa);

    if (flags & CLEARBUFFER_COLOR) {
        assert(rrb != 0);
        BEGIN_BATCH_NO_AUTOSTATE(4);
        OUT_BATCH_REGSEQ(R300_RB3D_COLOROFFSET0, 1);
        OUT_BATCH_RELOC(0, rrb->bo, 0, 0, RADEON_GEM_DOMAIN_VRAM, 0);
        OUT_BATCH_REGVAL(R300_RB3D_COLORPITCH0, cbpitch);
        END_BATCH();
    }
#if 0
    if (flags & (CLEARBUFFER_DEPTH | CLEARBUFFER_STENCIL)) {
        assert(rrbd != 0);
        cbpitch = (rrbd->pitch / rrbd->cpp);
        if (rrbd->bo->flags & RADEON_BO_FLAGS_MACRO_TILE){
            cbpitch |= R300_DEPTHMACROTILE_ENABLE;
        }
        if (rrbd->bo->flags & RADEON_BO_FLAGS_MICRO_TILE){
            cbpitch |= R300_DEPTHMICROTILE_TILED;
        }
        BEGIN_BATCH_NO_AUTOSTATE(4);
        OUT_BATCH_REGSEQ(R300_ZB_DEPTHOFFSET, 1);
        OUT_BATCH_RELOC(0, rrbd->bo, 0, 0, RADEON_GEM_DOMAIN_VRAM, 0);
        OUT_BATCH_REGVAL(R300_ZB_DEPTHPITCH, cbpitch);
        END_BATCH();
    }

    {
        uint32_t t1, t2;

        t1 = 0x0;
        t2 = 0x0;

        if (flags & CLEARBUFFER_DEPTH) {
            t1 |= R300_Z_ENABLE | R300_Z_WRITE_ENABLE;
            t2 |=
                    (R300_ZS_ALWAYS << R300_Z_FUNC_SHIFT);
        }

        if (flags & CLEARBUFFER_STENCIL) {
            t1 |= R300_STENCIL_ENABLE;
            t2 |=
                    (R300_ZS_ALWAYS <<
                    R300_S_FRONT_FUNC_SHIFT) |
                    (R300_ZS_REPLACE <<
                    R300_S_FRONT_SFAIL_OP_SHIFT) |
                    (R300_ZS_REPLACE <<
                    R300_S_FRONT_ZPASS_OP_SHIFT) |
                    (R300_ZS_REPLACE <<
                    R300_S_FRONT_ZFAIL_OP_SHIFT);
        }

        OUT_BATCH_REGSEQ(R300_ZB_CNTL, 3);
        OUT_BATCH(t1);
        OUT_BATCH(t2);
        OUT_BATCH(((ctx->Stencil.WriteMask[0] & R300_STENCILREF_MASK) <<
                R300_STENCILWRITEMASK_SHIFT) |
                (ctx->Stencil.Clear & R300_STENCILREF_MASK));
        END_BATCH();
    }
#endif

    OUT_CS(CP_PACKET3(R200_3D_DRAW_IMMD_2, 8));
    OUT_CS(R300_PRIM_TYPE_POINT | R300_PRIM_WALK_RING |
               (1 << R300_PRIM_NUM_VERTICES_SHIFT));
    OUT_CS_32F(w / 2.0);
    OUT_CS_32F(h / 2.0);
    /* XXX this should be the depth value to clear to */
    OUT_CS_32F(1.0);
    OUT_CS_32F(1.0);
    OUT_CS_32F(color);
    OUT_CS_32F(color);
    OUT_CS_32F(color);
    OUT_CS_32F(color);

    /* XXX cp_wait(rmesa, R300_WAIT_3D | R300_WAIT_3D_CLEAN); */
}

void r300_init_surface_functions(struct r300_context* r300)
{
    r300->context.surface_fill = r300_surface_fill;
}
