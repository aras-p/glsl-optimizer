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

static void r300_draw_emit_attrib(struct r300_context* r300,
                                  enum attrib_emit emit,
                                  enum interp_mode interp,
                                  int index)
{
    struct r300_vertex_shader* vs = r300->vs_state.state;
    struct tgsi_shader_info* info = &vs->info;
    int output;

    output = draw_find_shader_output(r300->draw,
                                     info->output_semantic_name[index],
                                     info->output_semantic_index[index]);
    draw_emit_vertex_attr(&r300->vertex_info, emit, interp, output);
}

static void r300_draw_emit_all_attribs(struct r300_context* r300)
{
    struct r300_vertex_shader* vs = r300->vs_state.state;
    struct r300_shader_semantics* vs_outputs = &vs->outputs;
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
/* XXX move this function into r300_state.c after TCL-bypass gets removed
 * XXX because this one is dependent only on vertex elements. */
static void r300_vertex_psc(struct r300_context* r300)
{
    struct r300_vertex_shader* vs = r300->vs_state.state;
    struct r300_vertex_stream_state *vformat =
        (struct r300_vertex_stream_state*)r300->vertex_stream_state.state;
    uint16_t type, swizzle;
    enum pipe_format format;
    unsigned i;
    int identity[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    int* stream_tab;

    memset(vformat, 0, sizeof(struct r300_vertex_stream_state));

    stream_tab = identity;

    /* Vertex shaders have no semantics on their inputs,
     * so PSC should just route stuff based on the vertex elements,
     * and not on attrib information. */
    DBG(r300, DBG_DRAW, "r300: vs expects %d attribs, routing %d elements"
            " in psc\n",
            vs->info.num_inputs,
            r300->vertex_element_count);

    for (i = 0; i < r300->vertex_element_count; i++) {
        format = r300->vertex_element[i].src_format;

        type = r300_translate_vertex_data_type(format) |
            (stream_tab[i] << R300_DST_VEC_LOC_SHIFT);
        swizzle = r300_translate_vertex_data_swizzle(format);

        if (i & 1) {
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

    vformat->count = (i >> 1) + 1;
    r300->vertex_stream_state.size = (1 + vformat->count) * 2;
}

/* Update the PSC tables for SW TCL, using Draw. */
static void r300_swtcl_vertex_psc(struct r300_context* r300)
{
    struct r300_vertex_shader* vs = r300->vs_state.state;
    struct r300_vertex_stream_state *vformat =
        (struct r300_vertex_stream_state*)r300->vertex_stream_state.state;
    struct vertex_info* vinfo = &r300->vertex_info;
    uint16_t type, swizzle;
    enum pipe_format format;
    unsigned i, attrib_count;
    int* vs_output_tab = vs->stream_loc_notcl;

    memset(vformat, 0, sizeof(struct r300_vertex_stream_state));

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

    vformat->count = (i >> 1) + 1;
    r300->vertex_stream_state.size = (1 + vformat->count) * 2;
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
    struct r300_rs_block rs = { { 0 } };
    int i, col_count = 0, tex_count = 0, fp_offset = 0, count;
    void (*rX00_rs_col)(struct r300_rs_block*, int, int, boolean);
    void (*rX00_rs_col_write)(struct r300_rs_block*, int, int);
    void (*rX00_rs_tex)(struct r300_rs_block*, int, int, boolean);
    void (*rX00_rs_tex_write)(struct r300_rs_block*, int, int);
    boolean any_bcolor_used = vs_outputs->bcolor[0] != ATTR_UNUSED ||
                              vs_outputs->bcolor[1] != ATTR_UNUSED;

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
        if (vs_outputs->color[i] != ATTR_UNUSED || any_bcolor_used ||
            vs_outputs->color[1] != ATTR_UNUSED) {
            /* Always rasterize if it's written by the VS,
             * otherwise it locks up. */
            rX00_rs_col(&rs, col_count, i, FALSE);

            /* Write it to the FS input register if it's used by the FS. */
            if (fs_inputs->color[i] != ATTR_UNUSED) {
                rX00_rs_col_write(&rs, col_count, fp_offset);
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
            rX00_rs_tex(&rs, tex_count, tex_count, FALSE);

            /* Write it to the FS input register if it's used by the FS. */
            if (fs_inputs->generic[i] != ATTR_UNUSED) {
                rX00_rs_tex_write(&rs, tex_count, fp_offset);
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
        rX00_rs_tex(&rs, tex_count, tex_count, TRUE);

        /* Write it to the FS input register if it's used by the FS. */
        if (fs_inputs->fog != ATTR_UNUSED) {
            rX00_rs_tex_write(&rs, tex_count, fp_offset);
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

    /* Rasterize WPOS. */
    /* If the FS doesn't need it, it's not written by the VS. */
    if (fs_inputs->wpos != ATTR_UNUSED) {
        rX00_rs_tex(&rs, tex_count, tex_count, FALSE);
        rX00_rs_tex_write(&rs, tex_count, fp_offset);

        fp_offset++;
        tex_count++;
    }

    /* Rasterize at least one color, or bad things happen. */
    if (col_count == 0 && tex_count == 0) {
        rX00_rs_col(&rs, 0, 0, TRUE);
        col_count++;
    }

    rs.count = (tex_count*4) | (col_count << R300_IC_COUNT_SHIFT) |
        R300_HIRES_EN;

    count = MAX3(col_count, tex_count, 1);
    rs.inst_count = count - 1;

    /* Now, after all that, see if we actually need to update the state. */
    if (memcmp(r300->rs_block_state.state, &rs, sizeof(struct r300_rs_block))) {
        memcpy(r300->rs_block_state.state, &rs, sizeof(struct r300_rs_block));
        r300->rs_block_state.size = 5 + count*2;
    }
}

/* Update the shader-dependant states. */
static void r300_update_derived_shader_state(struct r300_context* r300)
{
    struct r300_vertex_shader* vs = r300->vs_state.state;
    struct r300_screen* r300screen = r300_screen(r300->context.screen);
    struct r300_vap_output_state *vap_out =
        (struct r300_vap_output_state*)r300->vap_output_state.state;

    /* XXX Mmm, delicious hax */
    memset(&r300->vertex_info, 0, sizeof(struct vertex_info));
    memcpy(vap_out, vs->hwfmt, sizeof(uint)*4);

    r300_update_rs_block(r300, &vs->outputs, &r300->fs->inputs);

    if (r300screen->caps->has_tcl) {
        r300_vertex_psc(r300);
    } else {
        r300_draw_emit_all_attribs(r300);
        draw_compute_vertex_size(&r300->vertex_info);
        r300_swtcl_vertex_psc(r300);
    }
}

static boolean r300_dsa_writes_depth_stencil(struct r300_dsa_state* dsa)
{
    /* We are interested only in the cases when a new depth or stencil value
     * can be written and changed. */

    /* We might optionally check for [Z func: never] and inspect the stencil
     * state in a similar fashion, but it's not terribly important. */
    return (dsa->z_buffer_control & R300_Z_WRITE_ENABLE) ||
           (dsa->stencil_ref_mask & R300_STENCILWRITEMASK_MASK) ||
           ((dsa->z_buffer_control & R500_STENCIL_REFMASK_FRONT_BACK) &&
            (dsa->stencil_ref_bf & R300_STENCILWRITEMASK_MASK));
}

static boolean r300_dsa_alpha_test_enabled(struct r300_dsa_state* dsa)
{
    /* We are interested only in the cases when alpha testing can kill
     * a fragment. */
    uint32_t af = dsa->alpha_function;

    return (af & R300_FG_ALPHA_FUNC_ENABLE) &&
           (af & R300_FG_ALPHA_FUNC_ALWAYS) != R300_FG_ALPHA_FUNC_ALWAYS;
}

static void r300_update_ztop(struct r300_context* r300)
{
    struct r300_ztop_state* ztop_state =
        (struct r300_ztop_state*)r300->ztop_state.state;

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
     * (3) will never apply since we do not support chroma-keyed operations.
     * (4) will need to be re-examined (and this comment updated) if/when
     * Hyper-Z becomes supported.
     *
     * Additionally, the following conditions require disabled ZTOP:
     * 5) Depth writes in fragment shader
     * 6) Outstanding occlusion queries
     *
     * This register causes stalls all the way from SC to CB when changed,
     * but it is buffered on-chip so it does not hurt to write it if it has
     * not changed.
     *
     * ~C.
     */

    /* ZS writes */
    if (r300_dsa_writes_depth_stencil(r300->dsa_state.state) &&
           (r300_dsa_alpha_test_enabled(r300->dsa_state.state) ||/* (1) */
            r300->fs->info.uses_kill)) {                         /* (2) */
        ztop_state->z_buffer_top = R300_ZTOP_DISABLE;
    } else if (r300_fragment_shader_writes_depth(r300->fs)) {    /* (5) */
        ztop_state->z_buffer_top = R300_ZTOP_DISABLE;
    } else if (r300->query_current) {                            /* (6) */
        ztop_state->z_buffer_top = R300_ZTOP_DISABLE;
    } else {
        ztop_state->z_buffer_top = R300_ZTOP_ENABLE;
    }

    r300->ztop_state.dirty = TRUE;
}

static void r300_merge_textures_and_samplers(struct r300_context* r300)
{
    struct r300_textures_state *state =
        (struct r300_textures_state*)r300->textures_state.state;
    struct r300_texture_sampler_state *texstate;
    struct r300_sampler_state *sampler;
    struct r300_texture *tex;
    unsigned min_level, max_level, i, size;
    unsigned count = MIN2(state->texture_count, state->sampler_count);

    state->tx_enable = 0;
    size = 2;

    for (i = 0; i < count; i++) {
        if (state->textures[i] && state->sampler_states[i]) {
            state->tx_enable |= 1 << i;

            tex = state->textures[i];
            sampler = state->sampler_states[i];

            texstate = &state->regs[i];
            memcpy(texstate->format, &tex->state, sizeof(uint32_t)*3);
            texstate->filter[0] = sampler->filter0;
            texstate->filter[1] = sampler->filter1;
            texstate->border_color = sampler->border_color;
            texstate->tile_config = R300_TXO_MACRO_TILE(tex->macrotile) |
                                    R300_TXO_MICRO_TILE(tex->microtile);

            /* to emulate 1D textures through 2D ones correctly */
            if (tex->tex.target == PIPE_TEXTURE_1D) {
                texstate->filter[0] &= ~R300_TX_WRAP_T_MASK;
                texstate->filter[0] |= R300_TX_WRAP_T(R300_TX_CLAMP_TO_EDGE);
            }

            if (tex->is_npot) {
                /* NPOT textures don't support mip filter, unfortunately.
                 * This prevents incorrect rendering. */
                texstate->filter[0] &= ~R300_TX_MIN_FILTER_MIP_MASK;
            } else {
                /* determine min/max levels */
                /* the MAX_MIP level is the largest (finest) one */
                max_level = MIN2(sampler->max_lod, tex->tex.last_level);
                min_level = MIN2(sampler->min_lod, max_level);
                texstate->format[0] |= R300_TX_NUM_LEVELS(max_level);
                texstate->filter[0] |= R300_TX_MAX_MIP_LEVEL(min_level);
            }

            texstate->filter[0] |= i << 28;

            size += 16;
            state->count = i+1;
        }
    }

    r300->textures_state.size = size;
}

void r300_update_derived_state(struct r300_context* r300)
{
    if (r300->rs_block_state.dirty ||
        r300->vertex_stream_state.dirty || /* XXX put updating this state out of this file */
        r300->rs_state.dirty) {  /* XXX and remove this one (tcl_bypass dependency) */
        r300_update_derived_shader_state(r300);
    }

    if (r300->textures_state.dirty) {
        r300_merge_textures_and_samplers(r300);
    }

    r300_update_ztop(r300);
}
