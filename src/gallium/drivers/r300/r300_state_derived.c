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

#include "draw/draw_context.h"

#include "util/u_math.h"
#include "util/u_memory.h"

#include "r300_context.h"
#include "r300_fs.h"
#include "r300_screen.h"
#include "r300_state_derived.h"
#include "r300_state_inlines.h"
#include "r300_vs.h"

/* r300_state_derived: Various bits of state which are dependent upon
 * currently bound CSO data. */

struct r300_shader_key {
    struct r300_vertex_shader* vs;
    struct r300_fragment_shader* fs;
};

struct r300_shader_derived_value {
    struct r300_vertex_format* vformat;
    struct r300_rs_block* rs_block;
};

unsigned r300_shader_key_hash(void* key) {
    struct r300_shader_key* shader_key = (struct r300_shader_key*)key;
    unsigned vs = (unsigned)shader_key->vs;
    unsigned fs = (unsigned)shader_key->fs;

    return (vs << 16) | (fs & 0xffff);
}

int r300_shader_key_compare(void* key1, void* key2) {
    struct r300_shader_key* shader_key1 = (struct r300_shader_key*)key1;
    struct r300_shader_key* shader_key2 = (struct r300_shader_key*)key2;

    return (shader_key1->vs == shader_key2->vs) &&
        (shader_key1->fs == shader_key2->fs);
}

/* Set up the vs_tab and routes. */
static void r300_vs_tab_routes(struct r300_context* r300,
                               struct r300_vertex_info* vformat)
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

    if (!r300screen->caps->has_tcl || !r300->rs_state->enable_vte)
    {
        for (i = 0; i < info->num_inputs; i++) {
            switch (r300->vs->code.inputs[i]) {
                case TGSI_SEMANTIC_POSITION:
                    pos = TRUE;
                    tab[i] = 0;
                    break;
                case TGSI_SEMANTIC_COLOR:
                    tab[i] = 2 + cols;
                    cols++;
                    break;
                case TGSI_SEMANTIC_PSIZE:
                    assert(psize == FALSE);
                    psize = TRUE;
                    tab[i] = 15;
                    break;
                case TGSI_SEMANTIC_FOG:
                    assert(fog == FALSE);
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
    else
    {
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
                    /* Fall through */
                case TGSI_SEMANTIC_GENERIC:
                    texs++;
                    break;
                default:
                    debug_printf("r300: Unknown vertex output %d\n",
                        info->output_semantic_name[i]);
                    break;
            }
        }
    }

    /* XXX magic */
    assert(texs <= 8);

    /* Do the actual vertex_info setup.
     *
     * vertex_info has four uints of hardware-specific data in it.
     * vinfo.hwfmt[0] is R300_VAP_VTX_STATE_CNTL
     * vinfo.hwfmt[1] is R300_VAP_VSM_VTX_ASSM
     * vinfo.hwfmt[2] is R300_VAP_OUTPUT_VTX_FMT_0
     * vinfo.hwfmt[3] is R300_VAP_OUTPUT_VTX_FMT_1 */

    vinfo->hwfmt[0] = 0x5555; /* XXX this is classic Mesa bonghits */

    /* We need to add vertex position attribute only for SW TCL case,
     * for HW TCL case it could be generated by vertex shader */
    if (!pos && !r300screen->caps->has_tcl) {
        debug_printf("r300: Forcing vertex position attribute emit...\n");
        /* Make room for the position attribute
         * at the beginning of the tab. */
        for (i = 15; i > 0; i--) {
            tab[i] = tab[i-1];
        }
        tab[0] = 0;
    }

    /* Position. */
    if (r300->draw) {
        draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_PERSPECTIVE,
            draw_find_vs_output(r300->draw, TGSI_SEMANTIC_POSITION, 0));
    }
    vinfo->hwfmt[1] |= R300_INPUT_CNTL_POS;
    vinfo->hwfmt[2] |= R300_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT;

    /* Point size. */
    if (psize) {
        if (r300->draw) {
            draw_emit_vertex_attr(vinfo, EMIT_1F_PSIZE, INTERP_POS,
                draw_find_vs_output(r300->draw, TGSI_SEMANTIC_PSIZE, 0));
        }
        vinfo->hwfmt[2] |= R300_VAP_OUTPUT_VTX_FMT_0__PT_SIZE_PRESENT;
    }

    /* Colors. */
    for (i = 0; i < cols; i++) {
        if (r300->draw) {
            draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_LINEAR,
                draw_find_vs_output(r300->draw, TGSI_SEMANTIC_COLOR, i));
        }
        vinfo->hwfmt[1] |= R300_INPUT_CNTL_COLOR;
        vinfo->hwfmt[2] |= (R300_VAP_OUTPUT_VTX_FMT_0__COLOR_0_PRESENT << i);
    }

    /* Init i right here, increment it if fog is enabled.
     * This gets around a double-increment problem. */
    i = 0;

    /* Fog. This is a special-cased texcoord. */
    if (fog) {
        i++;
        if (r300->draw) {
            draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_PERSPECTIVE,
                draw_find_vs_output(r300->draw, TGSI_SEMANTIC_FOG, 0));
        }
        vinfo->hwfmt[1] |= (R300_INPUT_CNTL_TC0 << i);
        vinfo->hwfmt[3] |= (4 << (3 * i));
    }

    /* Texcoords. */
    for (; i < texs; i++) {
        if (r300->draw) {
            draw_emit_vertex_attr(vinfo, EMIT_4F, INTERP_PERSPECTIVE,
                draw_find_vs_output(r300->draw, TGSI_SEMANTIC_GENERIC, i));
        }
        vinfo->hwfmt[1] |= (R300_INPUT_CNTL_TC0 << i);
        vinfo->hwfmt[3] |= (4 << (3 * i));
    }

    draw_compute_vertex_size(vinfo);
}

/* Update the PSC tables. */
static void r300_vertex_psc(struct r300_context* r300,
                            struct r300_vertex_info* vformat)
{
    struct r300_screen* r300screen = r300_screen(r300->context.screen);
    struct vertex_info* vinfo = &vformat->vinfo;
    int* tab = vformat->vs_tab;
    uint16_t type, swizzle;
    enum pipe_format format;
    unsigned i, attrib_count;

    /* Vertex shaders have no semantics on their inputs,
     * so PSC should just route stuff based on their info,
     * and not on attrib information. */
    if (r300screen->caps->has_tcl) {
        attrib_count = r300->vs->info.num_inputs;
        DBG(r300, DBG_DRAW, "r300: routing %d attribs in psc for vs\n",
                attrib_count);
    } else {
        attrib_count = vinfo->num_attribs;
        DBG(r300, DBG_DRAW, "r300: attrib count: %d\n", attrib_count);
        for (i = 0; i < attrib_count; i++) {
            DBG(r300, DBG_DRAW, "r300: attrib: offset %d, interp %d, size %d,"
                   " tab %d\n", vinfo->attrib[i].src_index,
                   vinfo->attrib[i].interp_mode, vinfo->attrib[i].emit,
                   tab[i]);
        }
    }

    for (i = 0; i < attrib_count; i++) {
        /* Make sure we have a proper destination for our attribute. */
        assert(tab[i] != -1);

        format = draw_translate_vinfo_format(vinfo->attrib[i].emit);

        /* Obtain the type of data in this attribute. */
        type = r300_translate_vertex_data_type(format) |
            tab[i] << R300_DST_VEC_LOC_SHIFT;

        /* Obtain the swizzle for this attribute. Note that the default
         * swizzle in the hardware is not XYZW! */
        swizzle = r300_translate_vertex_data_swizzle(format);

        /* Add the attribute to the PSC table. */
        if (i & 1) {
            vformat->vap_prog_stream_cntl[i >> 1] |= type << 16;

            vformat->vap_prog_stream_cntl_ext[i >> 1] |= swizzle << 16;
        } else {
            vformat->vap_prog_stream_cntl[i >> 1] |= type <<  0;

            vformat->vap_prog_stream_cntl_ext[i >> 1] |= swizzle << 0;
        }
    }

    /* Set the last vector in the PSC. */
    if (i) {
        i -= 1;
    }
    vformat->vap_prog_stream_cntl[i >> 1] |=
        (R300_LAST_VEC << (i & 1 ? 16 : 0));
}

/* Set up the mappings from GB to US, for RS block. */
static void r300_update_fs_tab(struct r300_context* r300,
                               struct r300_vertex_info* vformat)
{
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
    DBG(r300, DBG_DRAW, "r300: fp input count: %d\n", info->num_inputs);
    for (i = 0; i < info->num_inputs; i++) {
        switch (tab[i]) {
            case INTERP_LINEAR:
                DBG(r300, DBG_DRAW, "r300: attrib: "
                        "stack offset %d, color,    tab %d\n",
                        i, cols_emitted);
                tab[i] = cols_emitted;
                cols_emitted++;
                break;
            case INTERP_PERSPECTIVE:
                DBG(r300, DBG_DRAW, "r300: attrib: "
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
static void r300_update_rs_block(struct r300_context* r300,
                                 struct r300_rs_block* rs)
{
    struct tgsi_shader_info* info = &r300->fs->info;
    int col_count = 0, fp_offset = 0, i, tex_count = 0;
    int rs_tex_comp = 0;

    if (r300_screen(r300->context.screen)->caps->is_r500) {
        for (i = 0; i < info->num_inputs; i++) {
            switch (info->input_semantic_name[i]) {
                case TGSI_SEMANTIC_COLOR:
                    rs->ip[col_count] |=
                        R500_RS_COL_PTR(col_count) |
                        R500_RS_COL_FMT(R300_RS_COL_FMT_RGBA);
                    col_count++;
                    break;
                case TGSI_SEMANTIC_GENERIC:
                    rs->ip[tex_count] |=
                        R500_RS_SEL_S(rs_tex_comp) |
                        R500_RS_SEL_T(rs_tex_comp + 1) |
                        R500_RS_SEL_R(rs_tex_comp + 2) |
                        R500_RS_SEL_Q(rs_tex_comp + 3);
                    tex_count++;
                    rs_tex_comp += 4;
                    break;
                default:
                    break;
            }
        }

        /* Rasterize at least one color, or bad things happen. */
        if ((col_count == 0) && (tex_count == 0)) {
            rs->ip[0] |= R500_RS_COL_FMT(R300_RS_COL_FMT_0001);
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
            switch (info->input_semantic_name[i]) {
                case TGSI_SEMANTIC_COLOR:
                    rs->ip[col_count] |=
                        R300_RS_COL_PTR(col_count) |
                        R300_RS_COL_FMT(R300_RS_COL_FMT_RGBA);
                    col_count++;
                    break;
                case TGSI_SEMANTIC_GENERIC:
                    rs->ip[tex_count] |=
                        R300_RS_TEX_PTR(rs_tex_comp) |
                        R300_RS_SEL_S(R300_RS_SEL_C0) |
                        R300_RS_SEL_T(R300_RS_SEL_C1) |
                        R300_RS_SEL_R(R300_RS_SEL_C2) |
                        R300_RS_SEL_Q(R300_RS_SEL_C3);
                    tex_count++;
                    rs_tex_comp+=4;
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

    rs->count = (rs_tex_comp) | (col_count << R300_IC_COUNT_SHIFT) |
        R300_HIRES_EN;

    rs->inst_count = MAX2(MAX2(col_count - 1, tex_count - 1), 0);
}

/* Update the vertex format. */
static void r300_update_derived_shader_state(struct r300_context* r300)
{
    struct r300_screen* r300screen = r300_screen(r300->context.screen);
    struct r300_vertex_info* vformat;
    struct r300_rs_block* rs_block;
    int i;

    /*
    struct r300_shader_key* key;
    struct r300_shader_derived_value* value;
    key = CALLOC_STRUCT(r300_shader_key);
    key->vs = r300->vs;
    key->fs = r300->fs;

    value = (struct r300_shader_derived_value*)
        util_hash_table_get(r300->shader_hash_table, (void*)key);
    if (value) {
        //vformat = value->vformat;
        rs_block = value->rs_block;

        FREE(key);
    } else {
        rs_block = CALLOC_STRUCT(r300_rs_block);
        value = CALLOC_STRUCT(r300_shader_derived_value);

        r300_update_rs_block(r300, rs_block);

        //value->vformat = vformat;
        value->rs_block = rs_block;
        util_hash_table_set(r300->shader_hash_table,
            (void*)key, (void*)value);
    } */

    /* XXX This will be refactored ASAP. */
    vformat = CALLOC_STRUCT(r300_vertex_info);
    rs_block = CALLOC_STRUCT(r300_rs_block);

    for (i = 0; i < 16; i++) {
        vformat->vs_tab[i] = -1;
        vformat->fs_tab[i] = -1;
    }

    r300_vs_tab_routes(r300, vformat);
    r300_vertex_psc(r300, vformat);
    r300_update_fs_tab(r300, vformat);

    r300_update_rs_block(r300, rs_block);

    FREE(r300->vertex_info);
    FREE(r300->rs_block);

    r300->vertex_info = vformat;
    r300->rs_block = rs_block;
    r300->dirty_state |= (R300_NEW_VERTEX_FORMAT | R300_NEW_RS_BLOCK);
}

static void r300_update_ztop(struct r300_context* r300)
{
    r300->ztop_state.z_buffer_top = R300_ZTOP_ENABLE;

    /* This is important enough that I felt it warranted a comment.
     *
     * According to the docs, these are the conditions where ZTOP must be
     * disabled:
     * 1) Alpha testing enabled
     * 2) Texture kill instructions in fragment shader
     * 3) Chroma key culling enabled
     * 4) W-buffering enabled
     *
     * The docs claim that for the first three cases, if no ZS writes happen,
     * then ZTOP can be used.
     *
     * Additionally, the following conditions require disabled ZTOP:
     * ~) Depth writes in fragment shader
     * ~) Outstanding occlusion queries
     *
     * ~C.
     */
    if (r300->dsa_state->alpha_function) {
        r300->ztop_state.z_buffer_top = R300_ZTOP_DISABLE;
    } else if (r300->fs->info.uses_kill) {
        r300->ztop_state.z_buffer_top = R300_ZTOP_DISABLE;
    } else if (r300_fragment_shader_writes_depth(r300->fs)) {
        r300->ztop_state.z_buffer_top = R300_ZTOP_DISABLE;
    } else if (r300->query_current) {
        r300->ztop_state.z_buffer_top = R300_ZTOP_DISABLE;
    }
}

void r300_update_derived_state(struct r300_context* r300)
{
    /* XXX */
    if (TRUE || r300->dirty_state &
        (R300_NEW_FRAGMENT_SHADER | R300_NEW_VERTEX_SHADER)) {
        r300_update_derived_shader_state(r300);
    }

    if (r300->dirty_state &
            (R300_NEW_DSA | R300_NEW_FRAGMENT_SHADER | R300_NEW_QUERY)) {
        r300_update_ztop(r300);
    }
}
