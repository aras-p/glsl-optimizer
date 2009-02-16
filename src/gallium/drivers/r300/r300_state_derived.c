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
     * vinfo.hwfmt[0] is VAP_OUT_VTX_FMT_0
     * vinfo.hwfmt[1] is VAP_OUT_VTX_FMT_1 */

    if (pos) {
        draw_emit_vertex_attr(&vinfo, EMIT_4F, INTERP_POS,
            draw_find_vs_output(r300->draw, TGSI_SEMANTIC_POSITION, 0));
        vinfo.hwfmt[0] |= R300_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT;
    } else {
        debug_printf("r300: No vertex input for position in SW TCL;\n"
            "    this will probably end poorly.\n");
    }

    if (psize) {
        draw_emit_vertex_attr(&vinfo, EMIT_1F, INTERP_LINEAR,
            draw_find_vs_output(r300->draw, TGSI_SEMANTIC_PSIZE, 0));
        vinfo.hwfmt[0] |= R300_VAP_OUTPUT_VTX_FMT_0__PT_SIZE_PRESENT;
    }

    for (i = 0; i < cols; i++) {
        draw_emit_vertex_attr(&vinfo, EMIT_4F, INTERP_LINEAR,
            draw_find_vs_output(r300->draw, TGSI_SEMANTIC_COLOR, i));
        vinfo.hwfmt[0] |= (R300_VAP_OUTPUT_VTX_FMT_0__COLOR_0_PRESENT << i);
    }

    if (fog) {
        draw_emit_vertex_attr(&vinfo, EMIT_4F, INTERP_PERSPECTIVE,
            draw_find_vs_output(r300->draw, TGSI_SEMANTIC_FOG, 0));
        vinfo.hwfmt[0] |=
            (R300_VAP_OUTPUT_VTX_FMT_0__COLOR_0_PRESENT << cols);
    }

    for (i = 0; i < texs; i++) {
        draw_emit_vertex_attr(&vinfo, EMIT_4F, INTERP_LINEAR,
            draw_find_vs_output(r300->draw, TGSI_SEMANTIC_GENERIC, i));
        vinfo.hwfmt[1] |= (4 << (3 * i));
    }

    draw_compute_vertex_size(&vinfo);

    if (memcmp(&r300->vertex_info, &vinfo, sizeof(struct vertex_info))) {
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
