/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 * Copyright 2009 Marek Olšák <maraeo@gmail.com>
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
#include "r300_shader_semantics.h"
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
    unsigned vs = (intptr_t)shader_key->vs;
    unsigned fs = (intptr_t)shader_key->fs;

    return (vs << 16) | (fs & 0xffff);
}

int r300_shader_key_compare(void* key1, void* key2) {
    struct r300_shader_key* shader_key1 = (struct r300_shader_key*)key1;
    struct r300_shader_key* shader_key2 = (struct r300_shader_key*)key2;

    return (shader_key1->vs == shader_key2->vs) &&
        (shader_key1->fs == shader_key2->fs);
}

static void r300_draw_emit_attrib(struct r300_context* r300,
                                  enum attrib_emit emit,
                                  enum interp_mode interp,
                                  int index)
{
    struct tgsi_shader_info* info = &r300->vs->info;
    int output;

    output = draw_find_vs_output(r300->draw,
                                 info->output_semantic_name[index],
                                 info->output_semantic_index[index]);
    draw_emit_vertex_attr(&r300->vertex_info->vinfo, emit, interp, output);
}

static void r300_draw_emit_all_attribs(struct r300_context* r300)
{
    struct r300_shader_semantics* vs_outputs = &r300->vs->outputs;
    int i, gen_count;

    /* Position. */
    if (vs_outputs->pos != ATTR_UNUSED) {
        r300_draw_emit_attrib(r300, EMIT_4F, INTERP_PERSPECTIVE,
                              vs_outputs->pos);
    } else {
        assert(0);
    }

    /* Point size. */
    if (vs_outputs->psize != ATTR_UNUSED) {
        r300_draw_emit_attrib(r300, EMIT_1F_PSIZE, INTERP_POS,
                              vs_outputs->psize);
    }

    /* Colors. */
    for (i = 0; i < ATTR_COLOR_COUNT; i++) {
        if (vs_outputs->color[i] != ATTR_UNUSED) {
            r300_draw_emit_attrib(r300, EMIT_4F, INTERP_LINEAR,
                                  vs_outputs->color[i]);
        }
    }

    /* XXX Back-face colors. */

    /* Texture coordinates. */
    gen_count = 0;
    for (i = 0; i < ATTR_GENERIC_COUNT; i++) {
        if (vs_outputs->generic[i] != ATTR_UNUSED) {
            r300_draw_emit_attrib(r300, EMIT_4F, INTERP_PERSPECTIVE,
                                  vs_outputs->generic[i]);
            gen_count++;
        }
    }

    /* Fog coordinates. */
    if (vs_outputs->fog != ATTR_UNUSED) {
        r300_draw_emit_attrib(r300, EMIT_4F, INTERP_PERSPECTIVE,
                              vs_outputs->fog);
        gen_count++;
    }

    /* XXX magic */
    assert(gen_count <= 8);
}

/* Update the PSC tables. */
static void r300_vertex_psc(struct r300_context* r300)
{
    struct r300_vertex_info *vformat = r300->vertex_info;
    uint16_t type, swizzle;
    enum pipe_format format;
    unsigned i;

    /* Vertex shaders have no semantics on their inputs,
     * so PSC should just route stuff based on the vertex elements,
     * and not on attrib information. */
    DBG(r300, DBG_DRAW, "r300: vs expects %d attribs, routing %d elements"
            " in psc\n",
            r300->vs->info.num_inputs,
            r300->vertex_element_count);

    for (i = 0; i < r300->vertex_element_count; i++) {
        format = r300->vertex_element[i].src_format;

        type = r300_translate_vertex_data_type(format) |
            (i << R300_DST_VEC_LOC_SHIFT);
        swizzle = r300_translate_vertex_data_swizzle(format);

        if (i % 2) {
            vformat->vap_prog_stream_cntl[i >> 1] |= type << 16;
            vformat->vap_prog_stream_cntl_ext[i >> 1] |= swizzle << 16;
        } else {
            vformat->vap_prog_stream_cntl[i >> 1] |= type;
            vformat->vap_prog_stream_cntl_ext[i >> 1] |= swizzle;
        }
    }


    assert(i <= 15);

    /* Set the last vector in the PSC. */
    if (i) {
        i -= 1;
    }
    vformat->vap_prog_stream_cntl[i >> 1] |=
        (R300_LAST_VEC << (i & 1 ? 16 : 0));
}

/* Update the PSC tables for SW TCL, using Draw. */
static void r300_swtcl_vertex_psc(struct r300_context* r300)
{
    struct r300_vertex_info *vformat = r300->vertex_info;
    struct vertex_info* vinfo = &vformat->vinfo;
    uint16_t type, swizzle;
    enum pipe_format format;
    unsigned i, attrib_count;
    int* vs_output_tab = r300->vs->output_stream_loc_swtcl;

    /* For each Draw attribute, route it to the fragment shader according
     * to the vs_output_tab. */
    attrib_count = vinfo->num_attribs;
    DBG(r300, DBG_DRAW, "r300: attrib count: %d\n", attrib_count);
    for (i = 0; i < attrib_count; i++) {
        DBG(r300, DBG_DRAW, "r300: attrib: offset %d, interp %d, size %d,"
               " vs_output_tab %d\n", vinfo->attrib[i].src_index,
               vinfo->attrib[i].interp_mode, vinfo->attrib[i].emit,
               vs_output_tab[i]);
    }

    for (i = 0; i < attrib_count; i++) {
        /* Make sure we have a proper destination for our attribute. */
        assert(vs_output_tab[i] != -1);

        format = draw_translate_vinfo_format(vinfo->attrib[i].emit);

        /* Obtain the type of data in this attribute. */
        type = r300_translate_vertex_data_type(format) |
            vs_output_tab[i] << R300_DST_VEC_LOC_SHIFT;

        /* Obtain the swizzle for this attribute. Note that the default
         * swizzle in the hardware is not XYZW! */
        swizzle = r300_translate_vertex_data_swizzle(format);

        /* Add the attribute to the PSC table. */
        if (i & 1) {
            vformat->vap_prog_stream_cntl[i >> 1] |= type << 16;
            vformat->vap_prog_stream_cntl_ext[i >> 1] |= swizzle << 16;
        } else {
            vformat->vap_prog_stream_cntl[i >> 1] |= type;
            vformat->vap_prog_stream_cntl_ext[i >> 1] |= swizzle;
        }
    }

    /* Set the last vector in the PSC. */
    if (i) {
        i -= 1;
    }
    vformat->vap_prog_stream_cntl[i >> 1] |=
        (R300_LAST_VEC << (i & 1 ? 16 : 0));
}

static void r300_rs_col(struct r300_rs_block* rs, int id, int ptr,
                        boolean swizzle_0001)
{
    rs->ip[id] |= R300_RS_COL_PTR(ptr);
    if (swizzle_0001) {
        rs->ip[id] |= R300_RS_COL_FMT(R300_RS_COL_FMT_0001);
    } else {
        rs->ip[id] |= R300_RS_COL_FMT(R300_RS_COL_FMT_RGBA);
    }
    rs->inst[id] |= R300_RS_INST_COL_ID(id);
}

static void r300_rs_col_write(struct r300_rs_block* rs, int id, int fp_offset)
{
    rs->inst[id] |= R300_RS_INST_COL_CN_WRITE |
                    R300_RS_INST_COL_ADDR(fp_offset);
}

static void r300_rs_tex(struct r300_rs_block* rs, int id, int ptr,
                        boolean swizzle_X001)
{
    if (swizzle_X001) {
        rs->ip[id] |= R300_RS_TEX_PTR(ptr*4) |
                      R300_RS_SEL_S(R300_RS_SEL_C0) |
                      R300_RS_SEL_T(R300_RS_SEL_K0) |
                      R300_RS_SEL_R(R300_RS_SEL_K0) |
                      R300_RS_SEL_Q(R300_RS_SEL_K1);
    } else {
        rs->ip[id] |= R300_RS_TEX_PTR(ptr*4) |
                      R300_RS_SEL_S(R300_RS_SEL_C0) |
                      R300_RS_SEL_T(R300_RS_SEL_C1) |
                      R300_RS_SEL_R(R300_RS_SEL_C2) |
                      R300_RS_SEL_Q(R300_RS_SEL_C3);
    }
    rs->inst[id] |= R300_RS_INST_TEX_ID(id);
}

static void r300_rs_tex_write(struct r300_rs_block* rs, int id, int fp_offset)
{
    rs->inst[id] |= R300_RS_INST_TEX_CN_WRITE |
                    R300_RS_INST_TEX_ADDR(fp_offset);
}

static void r500_rs_col(struct r300_rs_block* rs, int id, int ptr,
                        boolean swizzle_0001)
{
    rs->ip[id] |= R500_RS_COL_PTR(ptr);
    if (swizzle_0001) {
        rs->ip[id] |= R500_RS_COL_FMT(R300_RS_COL_FMT_0001);
    } else {
        rs->ip[id] |= R500_RS_COL_FMT(R300_RS_COL_FMT_RGBA);
    }
    rs->inst[id] |= R500_RS_INST_COL_ID(id);
}

static void r500_rs_col_write(struct r300_rs_block* rs, int id, int fp_offset)
{
    rs->inst[id] |= R500_RS_INST_COL_CN_WRITE |
                    R500_RS_INST_COL_ADDR(fp_offset);
}

static void r500_rs_tex(struct r300_rs_block* rs, int id, int ptr,
                        boolean swizzle_X001)
{
    int rs_tex_comp = ptr*4;

    if (swizzle_X001) {
        rs->ip[id] |= R500_RS_SEL_S(rs_tex_comp) |
                      R500_RS_SEL_T(R500_RS_IP_PTR_K0) |
                      R500_RS_SEL_R(R500_RS_IP_PTR_K0) |
                      R500_RS_SEL_Q(R500_RS_IP_PTR_K1);
    } else {
        rs->ip[id] |= R500_RS_SEL_S(rs_tex_comp) |
                      R500_RS_SEL_T(rs_tex_comp + 1) |
                      R500_RS_SEL_R(rs_tex_comp + 2) |
                      R500_RS_SEL_Q(rs_tex_comp + 3);
    }
    rs->inst[id] |= R500_RS_INST_TEX_ID(id);
}

static void r500_rs_tex_write(struct r300_rs_block* rs, int id, int fp_offset)
{
    rs->inst[id] |= R500_RS_INST_TEX_CN_WRITE |
                    R500_RS_INST_TEX_ADDR(fp_offset);
}

/* Set up the RS block.
 *
 * This is the part of the chipset that actually does the rasterization
 * of vertices into fragments. This is also the part of the chipset that
 * locks up if any part of it is even slightly wrong. */
static void r300_update_rs_block(struct r300_context* r300,
                                 struct r300_shader_semantics* vs_outputs,
                                 struct r300_shader_semantics* fs_inputs)
{
    struct r300_rs_block* rs = r300->rs_block;
    int i, col_count = 0, tex_count = 0, fp_offset = 0;
    void (*rX00_rs_col)(struct r300_rs_block*, int, int, boolean);
    void (*rX00_rs_col_write)(struct r300_rs_block*, int, int);
    void (*rX00_rs_tex)(struct r300_rs_block*, int, int, boolean);
    void (*rX00_rs_tex_write)(struct r300_rs_block*, int, int);

    if (r300_screen(r300->context.screen)->caps->is_r500) {
        rX00_rs_col       = r500_rs_col;
        rX00_rs_col_write = r500_rs_col_write;
        rX00_rs_tex       = r500_rs_tex;
        rX00_rs_tex_write = r500_rs_tex_write;
    } else {
        rX00_rs_col       = r300_rs_col;
        rX00_rs_col_write = r300_rs_col_write;
        rX00_rs_tex       = r300_rs_tex;
        rX00_rs_tex_write = r300_rs_tex_write;
    }

    /* Rasterize colors. */
    for (i = 0; i < ATTR_COLOR_COUNT; i++) {
        if (vs_outputs->color[i] != ATTR_UNUSED) {
            /* Always rasterize if it's written by the VS,
             * otherwise it locks up. */
            rX00_rs_col(rs, col_count, i, FALSE);

            /* Write it to the FS input register if it's used by the FS. */
            if (fs_inputs->color[i] != ATTR_UNUSED) {
                rX00_rs_col_write(rs, col_count, fp_offset);
                fp_offset++;
            }
            col_count++;
        } else {
            /* Skip the FS input register, leave it uninitialized. */
            /* If we try to set it to (0,0,0,1), it will lock up. */
            if (fs_inputs->color[i] != ATTR_UNUSED) {
                fp_offset++;
            }
        }
    }

    /* Rasterize texture coordinates. */
    for (i = 0; i < ATTR_GENERIC_COUNT; i++) {
        if (vs_outputs->generic[i] != ATTR_UNUSED) {
            /* Always rasterize if it's written by the VS,
             * otherwise it locks up. */
            rX00_rs_tex(rs, tex_count, tex_count, FALSE);

            /* Write it to the FS input register if it's used by the FS. */
            if (fs_inputs->generic[i] != ATTR_UNUSED) {
                rX00_rs_tex_write(rs, tex_count, fp_offset);
                fp_offset++;
            }
            tex_count++;
        } else {
            /* Skip the FS input register, leave it uninitialized. */
            /* If we try to set it to (0,0,0,1), it will lock up. */
            if (fs_inputs->generic[i] != ATTR_UNUSED) {
                fp_offset++;
            }
        }
    }

    /* Rasterize fog coordinates. */
    if (vs_outputs->fog != ATTR_UNUSED) {
        /* Always rasterize if it's written by the VS,
         * otherwise it locks up. */
        rX00_rs_tex(rs, tex_count, tex_count, TRUE);

        /* Write it to the FS input register if it's used by the FS. */
        if (fs_inputs->fog != ATTR_UNUSED) {
            rX00_rs_tex_write(rs, tex_count, fp_offset);
            fp_offset++;
        }
        tex_count++;
    } else {
        /* Skip the FS input register, leave it uninitialized. */
        /* If we try to set it to (0,0,0,1), it will lock up. */
        if (fs_inputs->fog != ATTR_UNUSED) {
            fp_offset++;
        }
    }

    /* Rasterize at least one color, or bad things happen. */
    if (col_count == 0 && tex_count == 0) {
        rX00_rs_col(rs, 0, 0, TRUE);
        col_count++;
    }

    rs->count = (tex_count*4) | (col_count << R300_IC_COUNT_SHIFT) |
        R300_HIRES_EN;

    rs->inst_count = MAX3(col_count - 1, tex_count - 1, 0);
}

/* Update the vertex format. */
static void r300_update_derived_shader_state(struct r300_context* r300)
{
    struct r300_screen* r300screen = r300_screen(r300->context.screen);

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

    /* Reset structures */
    memset(r300->rs_block, 0, sizeof(struct r300_rs_block));
    memset(r300->vertex_info, 0, sizeof(struct r300_vertex_info));
    memcpy(r300->vertex_info->vinfo.hwfmt, r300->vs->hwfmt, sizeof(uint)*4);

    r300_update_rs_block(r300, &r300->vs->outputs, &r300->fs->inputs);

    if (r300screen->caps->has_tcl) {
        r300_vertex_psc(r300);
    } else {
        r300_draw_emit_all_attribs(r300);
        draw_compute_vertex_size(&r300->vertex_info->vinfo);
        r300_swtcl_vertex_psc(r300);
    }

    r300->dirty_state |= R300_NEW_RS_BLOCK;
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
    if (r300->dirty_state &
        (R300_NEW_FRAGMENT_SHADER | R300_NEW_VERTEX_SHADER |
         R300_NEW_VERTEX_FORMAT)) {
        r300_update_derived_shader_state(r300);
    }

    if (r300->dirty_state &
            (R300_NEW_DSA | R300_NEW_FRAGMENT_SHADER | R300_NEW_QUERY)) {
        r300_update_ztop(r300);
    }
}
