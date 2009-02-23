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
    }
}

/* Update the vertex_info struct in our r300_context.
 *
 * The vertex_info struct describes the post-TCL format of vertices. It is
 * required for Draw when doing SW TCL, and also for describing the
 * dreaded RS block on R300 chipsets. */
/* XXX this function should be able to handle vert shaders as well as draw */
static void r300_update_vertex_layout(struct r300_context* r300)
{
    struct vertex_info vinfo;
    boolean pos = false, psize = false, fog = false;
    int i, texs = 0, cols = 0;

    struct tgsi_shader_info* info = &r300->fs->info;
    memset(&vinfo, 0, sizeof(vinfo));

    assert(info->num_inputs <= 16);

    /* This is rather lame. Since draw_find_vs_output doesn't return an error
     * when it can't find an output, we have to pre-iterate and count each
     * output ourselves. */
    for (i = 0; i < info->num_inputs; i++) {
        switch (info->input_semantic_name[i]) {
            case TGSI_SEMANTIC_POSITION:
                pos = true;
                break;
            case TGSI_SEMANTIC_COLOR:
                cols++;
                break;
            case TGSI_SEMANTIC_FOG:
                fog = true;
                break;
            case TGSI_SEMANTIC_PSIZE:
                psize = true;
                break;
            case TGSI_SEMANTIC_GENERIC:
                texs++;
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
        debug_printf("r300: Forcing vertex position attribute emit...");
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
        draw_emit_vertex_attr(&vinfo, EMIT_4F, INTERP_LINEAR,
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
            temp = translate_vertex_data_type(vinfo.attrib[i].emit) |
                R300_SIGNED;
            if (i & 1) {
                r300->vertex_info.vap_prog_stream_cntl[i >> 1] &= 0xffff0000;
                r300->vertex_info.vap_prog_stream_cntl[i >> 1] |=
                        (translate_vertex_data_type(vinfo.attrib[i].emit) |
                        R300_SIGNED) << 16;
            } else {
                r300->vertex_info.vap_prog_stream_cntl[i >> 1] &= 0xffff;
                r300->vertex_info.vap_prog_stream_cntl[i >> 1] |=
                    translate_vertex_data_type(vinfo.attrib[i].emit) |
                    R300_SIGNED;
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
void r300_update_rs_block(struct r300_context* r300)
{
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
