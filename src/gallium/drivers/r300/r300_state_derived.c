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

/* Set up the vs_tab and routes. */
static void r300_vs_tab_routes(struct r300_context* r300,
                               struct r300_vertex_format* vformat)
{
    struct r300_screen* r300screen = r300_screen(r300->context.screen);
    struct vertex_info* vinfo = &vformat->vinfo;
    int* tab = vformat->vs_tab;
    boolean pos = FALSE, psize = FALSE, fog = FALSE;
    int i, texs = 0, cols = 0;
    struct tgsi_shader_info* info;

    if (r300screen->caps->has_tcl) {
        /* Use vertex shader to determine required routes. */
        info = &r300->vs->info;
    } else {
        /* Use fragment shader to determine required routes. */
        info = &r300->fs->info;
    }

    assert(info->num_inputs <= 16);

    if (r300screen->caps->has_tcl) {
        /* Just copy vert attribs over as-is. */
        for (i = 0; i < info->num_inputs; i++) {
            tab[i] = i;
        }
        for (i = 0; i < info->num_outputs; i++) {
            switch (info->output_semantic_name[i]) {
                case TGSI_SEMANTIC_POSITION:
                    pos = TRUE;
                    break;
                case TGSI_SEMANTIC_COLOR:
                    cols++;
                    break;
                case TGSI_SEMANTIC_PSIZE:
                    psize = TRUE;
                    break;
                case TGSI_SEMANTIC_FOG:
                    fog = TRUE;
                case TGSI_SEMANTIC_GENERIC:
                    texs++;
                    break;
                default:
                    debug_printf("r300: Unknown vertex output %d\n",
                        info->output_semantic_name[i]);
                    break;
            }
        }
    } else {
        for (i = 0; i < info->num_inputs; i++) {
            switch (info->input_semantic_name[i]) {
                case TGSI_SEMANTIC_POSITION:
                    pos = TRUE;
                    tab[i] = 0;
                    break;
                case TGSI_SEMANTIC_COLOR:
                    tab[i] = 2 + cols;
                    cols++;
                    break;
                case TGSI_SEMANTIC_PSIZE:
                    psize = TRUE;
                    tab[i] = 15;
                    break;
                case TGSI_SEMANTIC_FOG:
                    fog = TRUE;
                    /* Fall through */
                case TGSI_SEMANTIC_GENERIC:
                    tab[i] = 6 + texs;
                    texs++;
                    break;
                default:
                    debug_printf("r300: Unknown vertex input %d\n",
                        info->input_semantic_name[i]);
                    break;
            }
        }
    }

    /* Do the actual vertex_info setup.
     *
     * vertex_info has four uints of hardware-specific data in it.
     * vinfo.hwfmt[0] is R300_VAP_VTX_STATE_CNTL
     * vinfo.hwfmt[1] is R300_VAP_VSM_VTX_ASSM
     * vinfo.hwfmt[2] is R300_VAP_OUTPUT_VTX_FMT_0
     * vinfo.hwfmt[3] is R300_VAP_OUTPUT_VTX_FMT_1 */

    vinfo->hwfmt[0] = 0x5555; /* XXX this is classic Mesa bonghits */

    if (!pos) {
        debug_printf("r300: Forcing vertex position attribute emit...\n");
        /* Make room for the position attribute
         * at the beginning of the tab. */
        for (i = 15; i > 0; i--) {
            tab[i] = tab[i-1];
        }
        tab[0] = 0;
    }
    draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_PERSPECTIVE,
        draw_find_vs_output(r300->draw, TGSI_SEMANTIC_POSITION, 0));
    vinfo->hwfmt[1] |= R300_INPUT_CNTL_POS;
    vinfo->hwfmt[2] |= R300_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT;

    if (psize) {
        draw_emit_vertex_attr(vinfo, EMIT_1F_PSIZE, INTERP_POS,
            draw_find_vs_output(r300->draw, TGSI_SEMANTIC_PSIZE, 0));
        vinfo->hwfmt[2] |= R300_VAP_OUTPUT_VTX_FMT_0__PT_SIZE_PRESENT;
    }

    for (i = 0; i < cols; i++) {
        draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_LINEAR,
            draw_find_vs_output(r300->draw, TGSI_SEMANTIC_COLOR, i));
        vinfo->hwfmt[1] |= R300_INPUT_CNTL_COLOR;
        vinfo->hwfmt[2] |= (R300_VAP_OUTPUT_VTX_FMT_0__COLOR_0_PRESENT << i);
    }

    for (i = 0; i < texs; i++) {
        draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_PERSPECTIVE,
            draw_find_vs_output(r300->draw, TGSI_SEMANTIC_GENERIC, i));
        vinfo->hwfmt[1] |= (R300_INPUT_CNTL_TC0 << i);
        vinfo->hwfmt[3] |= (4 << (3 * i));
    }

    if (fog) {
        i++;
        draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_PERSPECTIVE,
            draw_find_vs_output(r300->draw, TGSI_SEMANTIC_FOG, 0));
        vinfo->hwfmt[1] |= (R300_INPUT_CNTL_TC0 << i);
        vinfo->hwfmt[3] |= (4 << (3 * i));
    }

    draw_compute_vertex_size(vinfo);
}

/* Update the PSC tables. */
static void r300_vertex_psc(struct r300_context* r300,
                            struct r300_vertex_format* vformat)
{
    struct vertex_info* vinfo = &vformat->vinfo;
    int* tab = vformat->vs_tab;
    uint32_t temp;
    int i;

    debug_printf("r300: attrib count: %d\n", vinfo->num_attribs);
    for (i = 0; i < vinfo->num_attribs; i++) {
        debug_printf("r300: attrib: offset %d, interp %d, size %d,"
               " tab %d\n", vinfo->attrib[i].src_index,
               vinfo->attrib[i].interp_mode, vinfo->attrib[i].emit,
               tab[i]);
    }

    for (i = 0; i < vinfo->num_attribs; i++) {
        /* Make sure we have a proper destination for our attribute */
        assert(tab[i] != -1);

        /* Add the attribute to the PSC table. */
        temp = translate_vertex_data_type(vinfo->attrib[i].emit) |
            (tab[i] << R300_DST_VEC_LOC_SHIFT);
        if (i & 1) {
            vformat->vap_prog_stream_cntl[i >> 1] &= 0x0000ffff;
            vformat->vap_prog_stream_cntl[i >> 1] |= temp << 16;

            vformat->vap_prog_stream_cntl_ext[i >> 1] |=
                (R300_VAP_SWIZZLE_XYZW << 16);
        } else {
            vformat->vap_prog_stream_cntl[i >> 1] &= 0xffff0000;
            vformat->vap_prog_stream_cntl[i >> 1] |= temp <<  0;

            vformat->vap_prog_stream_cntl_ext[i >> 1] |=
                (R300_VAP_SWIZZLE_XYZW <<  0);
        }
    }

    /* Set the last vector in the PSC. */
    i--;
    vformat->vap_prog_stream_cntl[i >> 1] |=
        (R300_LAST_VEC << (i & 1 ? 16 : 0));
}

/* Update the vertex format. */
static void r300_update_vertex_format(struct r300_context* r300)
{
    struct r300_screen* r300screen = r300_screen(r300->context.screen);
    struct r300_vertex_format vformat;
    int i;

    memset(&vformat, 0, sizeof(struct r300_vertex_format));
    for (i = 0; i < 16; i++) {
        vformat.vs_tab[i] = -1;
        vformat.fs_tab[i] = -1;
    }

    r300_vs_tab_routes(r300, &vformat);

    r300_vertex_psc(r300, &vformat);

    if (memcmp(&r300->vertex_info, &vformat,
                sizeof(struct r300_vertex_format))) {
        memcpy(&r300->vertex_info, &vformat,
                sizeof(struct r300_vertex_format));
        r300->dirty_state |= R300_NEW_VERTEX_FORMAT;
    }
}

/* Set up the mappings from GB to US, for RS block. */
static void r300_update_fs_tab(struct r300_context* r300)
{
    struct r300_vertex_format* vformat = &r300->vertex_info;
    struct tgsi_shader_info* info = &r300->fs->info;
    int i, cols = 0, texs = 0, cols_emitted = 0;
    int* tab = vformat->fs_tab;

    for (i = 0; i < 16; i++) {
        tab[i] = -1;
    }

    assert(info->num_inputs <= 16);
    for (i = 0; i < info->num_inputs; i++) {
        switch (info->input_semantic_name[i]) {
            case TGSI_SEMANTIC_COLOR:
                tab[i] = INTERP_LINEAR;
                cols++;
                break;
            case TGSI_SEMANTIC_POSITION:
            case TGSI_SEMANTIC_PSIZE:
                debug_printf("r300: Implementation error: Can't use "
                        "pos attribs in fragshader yet!\n");
                /* Pass through for now */
            case TGSI_SEMANTIC_FOG:
            case TGSI_SEMANTIC_GENERIC:
                tab[i] = INTERP_PERSPECTIVE;
                break;
            default:
                debug_printf("r300: Unknown vertex input %d\n",
                    info->input_semantic_name[i]);
                break;
        }
    }

    /* Now that we know where everything is... */
    debug_printf("r300: fp input count: %d\n", info->num_inputs);
    for (i = 0; i < info->num_inputs; i++) {
        switch (tab[i]) {
            case INTERP_LINEAR:
                debug_printf("r300: attrib: "
                        "stack offset %d, color,    tab %d\n",
                        i, cols_emitted);
                tab[i] = cols_emitted;
                cols_emitted++;
                break;
            case INTERP_PERSPECTIVE:
                debug_printf("r300: attrib: "
                        "stack offset %d, texcoord, tab %d\n",
                        i, cols + texs);
                tab[i] = cols + texs;
                texs++;
                break;
            case -1:
                debug_printf("r300: Implementation error: Bad fp interp!\n");
            default:
                break;
        }
    }

}

/* Set up the RS block. This is the part of the chipset that actually does
 * the rasterization of vertices into fragments. This is also the part of the
 * chipset that locks up if any part of it is even slightly wrong. */
static void r300_update_rs_block(struct r300_context* r300)
{
    struct r300_rs_block* rs = r300->rs_block;
    struct tgsi_shader_info* info = &r300->fs->info;
    int* tab = r300->vertex_info.fs_tab;
    int col_count = 0, fp_offset = 0, i, memory_pos, tex_count = 0;

    memset(rs, 0, sizeof(struct r300_rs_block));

    if (r300_screen(r300->context.screen)->caps->is_r500) {
        for (i = 0; i < info->num_inputs; i++) {
            assert(tab[i] != -1);
            memory_pos = tab[i] * 4;
            switch (info->input_semantic_name[i]) {
                case TGSI_SEMANTIC_COLOR:
                    rs->ip[col_count] |=
                        R500_RS_COL_PTR(memory_pos) |
                        R500_RS_COL_FMT(R300_RS_COL_FMT_RGBA);
                    col_count++;
                    break;
                case TGSI_SEMANTIC_GENERIC:
                    rs->ip[tex_count] |=
                        R500_RS_SEL_S(memory_pos) |
                        R500_RS_SEL_T(memory_pos + 1) |
                        R500_RS_SEL_R(memory_pos + 2) |
                        R500_RS_SEL_Q(memory_pos + 3);
                    tex_count++;
                    break;
                default:
                    break;
            }
        }

        if (col_count == 0) {
            rs->ip[0] |= R500_RS_COL_FMT(R300_RS_COL_FMT_0001);
        }

        if (tex_count == 0) {
            rs->ip[0] |=
                R500_RS_SEL_S(R500_RS_IP_PTR_K0) |
                R500_RS_SEL_T(R500_RS_IP_PTR_K0) |
                R500_RS_SEL_R(R500_RS_IP_PTR_K0) |
                R500_RS_SEL_Q(R500_RS_IP_PTR_K1);
        }

        /* Rasterize at least one color, or bad things happen. */
        if ((col_count == 0) && (tex_count == 0)) {
            col_count++;
        }

        for (i = 0; i < tex_count; i++) {
            rs->inst[i] |= R500_RS_INST_TEX_ID(i) |
                R500_RS_INST_TEX_CN_WRITE | R500_RS_INST_TEX_ADDR(fp_offset);
            fp_offset++;
        }

        for (i = 0; i < col_count; i++) {
            rs->inst[i] |= R500_RS_INST_COL_ID(i) |
                R500_RS_INST_COL_CN_WRITE | R500_RS_INST_COL_ADDR(fp_offset);
            fp_offset++;
        }
    } else {
        for (i = 0; i < info->num_inputs; i++) {
            assert(tab[i] != -1);
            memory_pos = tab[i] * 4;
            switch (info->input_semantic_name[i]) {
                case TGSI_SEMANTIC_COLOR:
                    rs->ip[col_count] |=
                        R300_RS_COL_PTR(memory_pos) |
                        R300_RS_COL_FMT(R300_RS_COL_FMT_RGBA);
                    col_count++;
                    break;
                case TGSI_SEMANTIC_GENERIC:
                    rs->ip[tex_count] |=
                        R300_RS_TEX_PTR(memory_pos) |
                        R300_RS_SEL_S(R300_RS_SEL_C0) |
                        R300_RS_SEL_T(R300_RS_SEL_C1) |
                        R300_RS_SEL_R(R300_RS_SEL_C2) |
                        R300_RS_SEL_Q(R300_RS_SEL_C3);
                    tex_count++;
                    break;
                default:
                    break;
            }
        }

        if (col_count == 0) {
            rs->ip[0] |= R300_RS_COL_FMT(R300_RS_COL_FMT_0001);
        }

        if (tex_count == 0) {
            rs->ip[0] |=
                R300_RS_SEL_S(R300_RS_SEL_K0) |
                R300_RS_SEL_T(R300_RS_SEL_K0) |
                R300_RS_SEL_R(R300_RS_SEL_K0) |
                R300_RS_SEL_Q(R300_RS_SEL_K1);
        }

        /* Rasterize at least one color, or bad things happen. */
        if ((col_count == 0) && (tex_count == 0)) {
            col_count++;
        }

        for (i = 0; i < tex_count; i++) {
            rs->inst[i] |= R300_RS_INST_TEX_ID(i) |
                R300_RS_INST_TEX_CN_WRITE | R300_RS_INST_TEX_ADDR(fp_offset);
            fp_offset++;
        }

        for (i = 0; i < col_count; i++) {
            rs->inst[i] |= R300_RS_INST_COL_ID(i) |
                R300_RS_INST_COL_CN_WRITE | R300_RS_INST_COL_ADDR(fp_offset);
            fp_offset++;
        }
    }

    rs->count = (tex_count * 4) | (col_count << R300_IC_COUNT_SHIFT) |
        R300_HIRES_EN;

    rs->inst_count = MAX2(MAX2(col_count - 1, tex_count - 1), 0);
}

void r300_update_derived_state(struct r300_context* r300)
{
    if (r300->dirty_state &
            (R300_NEW_FRAGMENT_SHADER | R300_NEW_VERTEX_SHADER)) {
        r300_update_vertex_format(r300);
    }

    if (r300->dirty_state & R300_NEW_VERTEX_FORMAT) {
        r300_update_fs_tab(r300);
        r300_update_rs_block(r300);
    }
}
