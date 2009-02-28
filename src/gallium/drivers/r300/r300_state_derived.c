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

#include "r300_state_derived.h"

/* r300_state_derived: Various bits of state which are dependent upon
 * currently bound CSO data. */

static uint32_t translate_vertex_data_type(int type) {
    switch (type) {
        case EMIT_1F:
        case EMIT_1F_PSIZE:
            return R300_DATA_TYPE_FLOAT_1;
            break;
        case EMIT_2F:
            return R300_DATA_TYPE_FLOAT_2;
            break;
        case EMIT_3F:
            return R300_DATA_TYPE_FLOAT_3;
            break;
        case EMIT_4F:
            return R300_DATA_TYPE_FLOAT_4;
            break;
        default:
            debug_printf("r300: Implementation error: "
                    "Bad vertex data type!\n");
            break;
    }

    return 0;
}

/* Update the vertex_info struct in our r300_context.
 *
 * The vertex_info struct describes the post-TCL format of vertices. It is
 * required for Draw when doing SW TCL, and also for describing the
 * dreaded RS block on R300 chipsets. */
/* XXX this function should be able to handle vert shaders as well as draw */
static void r300_update_vertex_layout(struct r300_context* r300)
{
    struct r300_vertex_format vformat;
    struct vertex_info vinfo;
    boolean pos = FALSE, psize = FALSE, fog = FALSE;
    int i, texs = 0, cols = 0;
    int tab[16];

    struct tgsi_shader_info* info = &r300->fs->info;

    memset(&vinfo, 0, sizeof(vinfo));
    for (i = 0; i < 16; i++) {
        tab[i] = -1;
    }

    assert(info->num_inputs <= 16);

    for (i = 0; i < info->num_inputs; i++) {
        switch (info->input_semantic_name[i]) {
            case TGSI_SEMANTIC_POSITION:
                pos = TRUE;
                tab[i] = 0;
                break;
            case TGSI_SEMANTIC_COLOR:
                tab[i] = 2 + cols++;
                break;
            case TGSI_SEMANTIC_FOG:
                fog = TRUE;
                break;
            case TGSI_SEMANTIC_PSIZE:
                psize = TRUE;
                tab[i] = 1;
                break;
            case TGSI_SEMANTIC_GENERIC:
                tab[i] = 6 + texs++;
                break;
            default:
                debug_printf("r300: Unknown vertex input %d\n",
                    info->input_semantic_name[i]);
                break;
        }
    }

    /* Do the actual vertex_info setup.
     *
     * vertex_info has four uints of hardware-specific data in it.
     * vinfo.hwfmt[0] is R300_VAP_VTX_STATE_CNTL
     * vinfo.hwfmt[1] is R300_VAP_VSM_VTX_ASSM
     * vinfo.hwfmt[2] is R300_VAP_OUTPUT_VTX_FMT_0
     * vinfo.hwfmt[3] is R300_VAP_OUTPUT_VTX_FMT_1 */

    vinfo.hwfmt[0] = 0x5555; /* XXX this is classic Mesa bonghits */

    if (!pos) {
        debug_printf("r300: Forcing vertex position attribute emit...\n");
        tab[0] = 0;
    }

    draw_emit_vertex_attr(&vinfo, EMIT_4F, INTERP_POS,
        draw_find_vs_output(r300->draw, TGSI_SEMANTIC_POSITION, 0));
    vinfo.hwfmt[1] |= R300_INPUT_CNTL_POS;
    vinfo.hwfmt[2] |= R300_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT;

    if (psize) {
        draw_emit_vertex_attr(&vinfo, EMIT_1F_PSIZE, INTERP_LINEAR,
            draw_find_vs_output(r300->draw, TGSI_SEMANTIC_PSIZE, 0));
        vinfo.hwfmt[2] |= R300_VAP_OUTPUT_VTX_FMT_0__PT_SIZE_PRESENT;
    }

    for (i = 0; i < cols; i++) {
        draw_emit_vertex_attr(&vinfo, EMIT_4F, INTERP_LINEAR,
            draw_find_vs_output(r300->draw, TGSI_SEMANTIC_COLOR, i));
        vinfo.hwfmt[1] |= R300_INPUT_CNTL_COLOR;
        vinfo.hwfmt[2] |= (R300_VAP_OUTPUT_VTX_FMT_0__COLOR_0_PRESENT << i);
    }

    if (fog) {
        draw_emit_vertex_attr(&vinfo, EMIT_4F, INTERP_PERSPECTIVE,
            draw_find_vs_output(r300->draw, TGSI_SEMANTIC_FOG, 0));
        vinfo.hwfmt[2] |=
            (R300_VAP_OUTPUT_VTX_FMT_0__COLOR_0_PRESENT << cols);
    }

    for (i = 0; i < texs; i++) {
        draw_emit_vertex_attr(&vinfo, EMIT_4F, INTERP_PERSPECTIVE,
            draw_find_vs_output(r300->draw, TGSI_SEMANTIC_GENERIC, i));
        vinfo.hwfmt[1] |= (R300_INPUT_CNTL_TC0 << i);
        vinfo.hwfmt[3] |= (4 << (3 * i));
    }

    draw_compute_vertex_size(&vinfo);

    if (memcmp(&r300->vertex_info, &vinfo, sizeof(struct vertex_info))) {
        uint32_t temp;

#define BORING_SWIZZLE \
        ((R300_SWIZZLE_SELECT_X << R300_SWIZZLE_SELECT_X_SHIFT) | \
         (R300_SWIZZLE_SELECT_Y << R300_SWIZZLE_SELECT_Y_SHIFT) | \
         (R300_SWIZZLE_SELECT_Z << R300_SWIZZLE_SELECT_Z_SHIFT) | \
         (R300_SWIZZLE_SELECT_W << R300_SWIZZLE_SELECT_W_SHIFT) | \
         (0xf << R300_WRITE_ENA_SHIFT))

        for (i = 0; i < vinfo.num_attribs; i++) {
            /* Make sure we have a proper destination for our attribute */
            if (tab[i] != -1) {
                assert(0);
            }

            temp = translate_vertex_data_type(vinfo.attrib[i].emit) |
                (tab[i] << R300_DST_VEC_LOC_SHIFT) | R300_SIGNED;
            if (i & 1) {
                r300->vertex_info.vap_prog_stream_cntl[i >> 1] &= 0xffff0000;
                r300->vertex_info.vap_prog_stream_cntl[i >> 1] |=
                        temp << 16;
            } else {
                r300->vertex_info.vap_prog_stream_cntl[i >> 1] &= 0xffff;
                r300->vertex_info.vap_prog_stream_cntl[i >> 1] |=
                    temp;
            }

            r300->vertex_info.vap_prog_stream_cntl_ext[i >> 1] |=
                (BORING_SWIZZLE << (i & 1 ? 16 : 0));
        }
        r300->vertex_info.vap_prog_stream_cntl[i >> 1] |= (R300_LAST_VEC <<
                (i & 1 ? 16 : 0));

        memcpy(&r300->vertex_info, &vinfo, sizeof(struct vertex_info));
        r300->dirty_state |= R300_NEW_VERTEX_FORMAT;
    }
}

/* Set up the RS block. This is the part of the chipset that actually does
 * the rasterization of vertices into fragments. This is also the part of the
 * chipset that locks up if any part of it is even slightly wrong. */
static void r300_update_rs_block(struct r300_context* r300)
{
    struct r300_rs_block* rs = r300->rs_block;
    struct vertex_info* vinfo = &r300->vertex_info.vinfo;
    int col_count = 0, fp_offset = 0, i, tex_count = 0;

    memset(rs, 0, sizeof(struct r300_rs_block));

    if (r300_screen(r300->context.screen)->caps->is_r500) {
        for (i = 0; i < vinfo->num_attribs; i++) {
            switch (vinfo->attrib[i].interp_mode) {
                case INTERP_LINEAR:
                    rs->ip[col_count] |=
                        R500_RS_COL_PTR(vinfo->attrib[i].src_index) |
                        R500_RS_COL_FMT(R300_RS_COL_FMT_RGBA);
                    col_count++;
                    break;
                case INTERP_PERSPECTIVE:
                    rs->ip[tex_count] |=
                        R500_RS_SEL_S(vinfo->attrib[i].src_index) |
                        R500_RS_SEL_T(vinfo->attrib[i].src_index + 1) |
                        R500_RS_SEL_R(vinfo->attrib[i].src_index + 2) |
                        R500_RS_SEL_Q(vinfo->attrib[i].src_index + 3);
                    tex_count++;
                    break;
            }
        }

        for (i = 0; i < tex_count; i++) {
            rs->inst[i] |= R500_RS_INST_TEX_ID(i) | R500_RS_INST_TEX_CN_WRITE |
                R500_RS_INST_TEX_ADDR(fp_offset);
            fp_offset++;
        }

        for (i = 0; i < col_count; i++) {
            rs->inst[i] |= R500_RS_INST_COL_ID(i) | R500_RS_INST_COL_CN_WRITE |
                R500_RS_INST_COL_ADDR(fp_offset);
            fp_offset++;
        }

        rs->inst_count = MAX2(col_count, tex_count);
    } else {
        for (i = 0; i < vinfo->num_attribs; i++) {
            switch (vinfo->attrib[i].interp_mode) {
                case INTERP_LINEAR:
                    rs->ip[col_count] |=
                        R300_RS_COL_PTR(vinfo->attrib[i].src_index) |
                        R300_RS_COL_FMT(R300_RS_COL_FMT_RGBA);
                    col_count++;
                    break;
                case INTERP_PERSPECTIVE:
                    rs->ip[tex_count] |=
                        R300_RS_TEX_PTR(vinfo->attrib[i].src_index) |
                        R300_RS_SEL_S(R300_RS_SEL_C0) |
                        R300_RS_SEL_T(R300_RS_SEL_C1) |
                        R300_RS_SEL_R(R300_RS_SEL_C2) |
                        R300_RS_SEL_Q(R300_RS_SEL_C3);
                    tex_count += 4;
                    break;
            }
        }

        for (i = 0; i < tex_count; i++) {
            rs->inst[i] |= R300_RS_INST_TEX_ID(i) | R300_RS_INST_TEX_CN_WRITE |
                R300_RS_INST_TEX_ADDR(fp_offset);
            fp_offset++;
        }

        for (i = 0; i < col_count; i++) {
            rs->inst[i] |= R300_RS_INST_COL_ID(i) | R300_RS_INST_COL_CN_WRITE |
                R300_RS_INST_COL_ADDR(fp_offset);
            fp_offset++;
        }
    }

    rs->count = (tex_count * 4) | (col_count << R300_IC_COUNT_SHIFT) |
        R300_HIRES_EN;

    rs->inst_count = MAX2(col_count, tex_count);
}

void r300_update_derived_state(struct r300_context* r300)
{
    if (r300->dirty_state & R300_NEW_FRAGMENT_SHADER) {
        r300_update_vertex_layout(r300);
    }

    if (r300->dirty_state & R300_NEW_VERTEX_FORMAT) {
        r300_update_rs_block(r300);
    }
}
