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
#include "r300_hyperz.h"
#include "r300_screen.h"
#include "r300_shader_semantics.h"
#include "r300_state_derived.h"
#include "r300_state_inlines.h"
#include "r300_texture.h"
#include "r300_vs.h"

/* r300_state_derived: Various bits of state which are dependent upon
 * currently bound CSO data. */

enum r300_rs_swizzle {
    SWIZ_XYZW = 0,
    SWIZ_X001,
    SWIZ_XY01,
    SWIZ_0001,
};

enum r300_rs_col_write_type {
    WRITE_COLOR = 0,
    WRITE_FACE
};

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

    /* Back-face colors. */
    for (i = 0; i < ATTR_COLOR_COUNT; i++) {
        if (vs_outputs->bcolor[i] != ATTR_UNUSED) {
            r300_draw_emit_attrib(r300, EMIT_4F, INTERP_LINEAR,
                                  vs_outputs->bcolor[i]);
        }
    }

    /* Texture coordinates. */
    /* Only 8 generic vertex attributes can be used. If there are more,
     * they won't be rasterized. */
    gen_count = 0;
    for (i = 0; i < ATTR_GENERIC_COUNT && gen_count < 8; i++) {
        if (vs_outputs->generic[i] != ATTR_UNUSED &&
            !(r300->sprite_coord_enable & (1 << i))) {
            r300_draw_emit_attrib(r300, EMIT_4F, INTERP_PERSPECTIVE,
                                  vs_outputs->generic[i]);
            gen_count++;
        }
    }

    /* Fog coordinates. */
    if (gen_count < 8 && vs_outputs->fog != ATTR_UNUSED) {
        r300_draw_emit_attrib(r300, EMIT_4F, INTERP_PERSPECTIVE,
                              vs_outputs->fog);
        gen_count++;
    }

    /* WPOS. */
    if (r300_fs(r300)->shader->inputs.wpos != ATTR_UNUSED && gen_count < 8) {
        DBG(r300, DBG_SWTCL, "draw_emit_attrib: WPOS, index: %i\n",
            vs_outputs->wpos);
        r300_draw_emit_attrib(r300, EMIT_4F, INTERP_PERSPECTIVE,
                              vs_outputs->wpos);
    }
}

/* Update the PSC tables for SW TCL, using Draw. */
static void r300_swtcl_vertex_psc(struct r300_context *r300)
{
    struct r300_vertex_stream_state *vstream = r300->vertex_stream_state.state;
    struct vertex_info *vinfo = &r300->vertex_info;
    uint16_t type, swizzle;
    enum pipe_format format;
    unsigned i, attrib_count;
    int* vs_output_tab = r300->stream_loc_notcl;

    memset(vstream, 0, sizeof(struct r300_vertex_stream_state));

    /* For each Draw attribute, route it to the fragment shader according
     * to the vs_output_tab. */
    attrib_count = vinfo->num_attribs;
    DBG(r300, DBG_SWTCL, "r300: attrib count: %d\n", attrib_count);
    for (i = 0; i < attrib_count; i++) {
        if (vs_output_tab[i] == -1) {
            assert(0);
            abort();
        }

        format = draw_translate_vinfo_format(vinfo->attrib[i].emit);

        DBG(r300, DBG_SWTCL,
            "r300: swtcl_vertex_psc [%i] <- %s\n",
            vs_output_tab[i], util_format_short_name(format));

        /* Obtain the type of data in this attribute. */
        type = r300_translate_vertex_data_type(format);
        if (type == R300_INVALID_FORMAT) {
            fprintf(stderr, "r300: Bad vertex format %s.\n",
                    util_format_short_name(format));
            assert(0);
            abort();
        }

        type |= vs_output_tab[i] << R300_DST_VEC_LOC_SHIFT;

        /* Obtain the swizzle for this attribute. Note that the default
         * swizzle in the hardware is not XYZW! */
        swizzle = r300_translate_vertex_data_swizzle(format);

        /* Add the attribute to the PSC table. */
        if (i & 1) {
            vstream->vap_prog_stream_cntl[i >> 1] |= type << 16;
            vstream->vap_prog_stream_cntl_ext[i >> 1] |= swizzle << 16;
        } else {
            vstream->vap_prog_stream_cntl[i >> 1] |= type;
            vstream->vap_prog_stream_cntl_ext[i >> 1] |= swizzle;
        }
    }

    /* Set the last vector in the PSC. */
    if (i) {
        i -= 1;
    }
    vstream->vap_prog_stream_cntl[i >> 1] |=
        (R300_LAST_VEC << (i & 1 ? 16 : 0));

    vstream->count = (i >> 1) + 1;
    r300->vertex_stream_state.dirty = TRUE;
    r300->vertex_stream_state.size = (1 + vstream->count) * 2;
}

static void r300_rs_col(struct r300_rs_block* rs, int id, int ptr,
                        enum r300_rs_swizzle swiz)
{
    rs->ip[id] |= R300_RS_COL_PTR(ptr);
    if (swiz == SWIZ_0001) {
        rs->ip[id] |= R300_RS_COL_FMT(R300_RS_COL_FMT_0001);
    } else {
        rs->ip[id] |= R300_RS_COL_FMT(R300_RS_COL_FMT_RGBA);
    }
    rs->inst[id] |= R300_RS_INST_COL_ID(id);
}

static void r300_rs_col_write(struct r300_rs_block* rs, int id, int fp_offset,
                              enum r300_rs_col_write_type type)
{
    assert(type == WRITE_COLOR);
    rs->inst[id] |= R300_RS_INST_COL_CN_WRITE |
                    R300_RS_INST_COL_ADDR(fp_offset);
}

static void r300_rs_tex(struct r300_rs_block* rs, int id, int ptr,
                        enum r300_rs_swizzle swiz)
{
    if (swiz == SWIZ_X001) {
        rs->ip[id] |= R300_RS_TEX_PTR(ptr) |
                      R300_RS_SEL_S(R300_RS_SEL_C0) |
                      R300_RS_SEL_T(R300_RS_SEL_K0) |
                      R300_RS_SEL_R(R300_RS_SEL_K0) |
                      R300_RS_SEL_Q(R300_RS_SEL_K1);
    } else if (swiz == SWIZ_XY01) {
        rs->ip[id] |= R300_RS_TEX_PTR(ptr) |
                      R300_RS_SEL_S(R300_RS_SEL_C0) |
                      R300_RS_SEL_T(R300_RS_SEL_C1) |
                      R300_RS_SEL_R(R300_RS_SEL_K0) |
                      R300_RS_SEL_Q(R300_RS_SEL_K1);
    } else {
        rs->ip[id] |= R300_RS_TEX_PTR(ptr) |
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
                        enum r300_rs_swizzle swiz)
{
    rs->ip[id] |= R500_RS_COL_PTR(ptr);
    if (swiz == SWIZ_0001) {
        rs->ip[id] |= R500_RS_COL_FMT(R300_RS_COL_FMT_0001);
    } else {
        rs->ip[id] |= R500_RS_COL_FMT(R300_RS_COL_FMT_RGBA);
    }
    rs->inst[id] |= R500_RS_INST_COL_ID(id);
}

static void r500_rs_col_write(struct r300_rs_block* rs, int id, int fp_offset,
                              enum r300_rs_col_write_type type)
{
    if (type == WRITE_FACE)
        rs->inst[id] |= R500_RS_INST_COL_CN_WRITE_BACKFACE |
                        R500_RS_INST_COL_ADDR(fp_offset);
    else
        rs->inst[id] |= R500_RS_INST_COL_CN_WRITE |
                        R500_RS_INST_COL_ADDR(fp_offset);

}

static void r500_rs_tex(struct r300_rs_block* rs, int id, int ptr,
			enum r300_rs_swizzle swiz)
{
    if (swiz == SWIZ_X001) {
        rs->ip[id] |= R500_RS_SEL_S(ptr) |
                      R500_RS_SEL_T(R500_RS_IP_PTR_K0) |
                      R500_RS_SEL_R(R500_RS_IP_PTR_K0) |
                      R500_RS_SEL_Q(R500_RS_IP_PTR_K1);
    } else if (swiz == SWIZ_XY01) {
        rs->ip[id] |= R500_RS_SEL_S(ptr) |
                      R500_RS_SEL_T(ptr + 1) |
                      R500_RS_SEL_R(R500_RS_IP_PTR_K0) |
                      R500_RS_SEL_Q(R500_RS_IP_PTR_K1);
    } else {
        rs->ip[id] |= R500_RS_SEL_S(ptr) |
                      R500_RS_SEL_T(ptr + 1) |
                      R500_RS_SEL_R(ptr + 2) |
                      R500_RS_SEL_Q(ptr + 3);
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
 * This is the part of the chipset that is responsible for linking vertex
 * and fragment shaders and stuffed texture coordinates.
 *
 * The rasterizer reads data from VAP, which produces vertex shader outputs,
 * and GA, which produces stuffed texture coordinates. VAP outputs have
 * precedence over GA. All outputs must be rasterized otherwise it locks up.
 * If there are more outputs rasterized than is set in VAP/GA, it locks up
 * too. The funky part is that this info has been pretty much obtained by trial
 * and error. */
static void r300_update_rs_block(struct r300_context *r300)
{
    struct r300_vertex_shader *vs = r300->vs_state.state;
    struct r300_shader_semantics *vs_outputs = &vs->outputs;
    struct r300_shader_semantics *fs_inputs = &r300_fs(r300)->shader->inputs;
    struct r300_rs_block rs = {0};
    int i, col_count = 0, tex_count = 0, fp_offset = 0, count, loc = 0, tex_ptr = 0;
    void (*rX00_rs_col)(struct r300_rs_block*, int, int, enum r300_rs_swizzle);
    void (*rX00_rs_col_write)(struct r300_rs_block*, int, int, enum r300_rs_col_write_type);
    void (*rX00_rs_tex)(struct r300_rs_block*, int, int, enum r300_rs_swizzle);
    void (*rX00_rs_tex_write)(struct r300_rs_block*, int, int);
    boolean any_bcolor_used = vs_outputs->bcolor[0] != ATTR_UNUSED ||
                              vs_outputs->bcolor[1] != ATTR_UNUSED;
    int *stream_loc_notcl = r300->stream_loc_notcl;
    uint32_t stuffing_enable = 0;

    if (r300->screen->caps.is_r500) {
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

    /* 0x5555 copied from classic, which means:
     * Select user color 0 for COLOR0 up to COLOR7.
     * What the hell does that mean? */
    rs.vap_vtx_state_cntl = 0x5555;

    /* The position is always present in VAP. */
    rs.vap_vsm_vtx_assm |= R300_INPUT_CNTL_POS;
    rs.vap_out_vtx_fmt[0] |= R300_VAP_OUTPUT_VTX_FMT_0__POS_PRESENT;
    stream_loc_notcl[loc++] = 0;

    /* Set up the point size in VAP. */
    if (vs_outputs->psize != ATTR_UNUSED) {
        rs.vap_out_vtx_fmt[0] |= R300_VAP_OUTPUT_VTX_FMT_0__PT_SIZE_PRESENT;
        stream_loc_notcl[loc++] = 1;
    }

    /* Set up and rasterize colors. */
    for (i = 0; i < ATTR_COLOR_COUNT; i++) {
        if (vs_outputs->color[i] != ATTR_UNUSED || any_bcolor_used ||
            vs_outputs->color[1] != ATTR_UNUSED) {
            /* Set up the color in VAP. */
            rs.vap_vsm_vtx_assm |= R300_INPUT_CNTL_COLOR;
            rs.vap_out_vtx_fmt[0] |=
                    R300_VAP_OUTPUT_VTX_FMT_0__COLOR_0_PRESENT << i;
            stream_loc_notcl[loc++] = 2 + i;

            /* Rasterize it. */
            rX00_rs_col(&rs, col_count, col_count, SWIZ_XYZW);

            /* Write it to the FS input register if it's needed by the FS. */
            if (fs_inputs->color[i] != ATTR_UNUSED) {
                rX00_rs_col_write(&rs, col_count, fp_offset, WRITE_COLOR);
                fp_offset++;

                DBG(r300, DBG_RS,
                    "r300: Rasterized color %i written to FS.\n", i);
            } else {
                DBG(r300, DBG_RS, "r300: Rasterized color %i unused.\n", i);
            }
            col_count++;
        } else {
            /* Skip the FS input register, leave it uninitialized. */
            /* If we try to set it to (0,0,0,1), it will lock up. */
            if (fs_inputs->color[i] != ATTR_UNUSED) {
                fp_offset++;

                DBG(r300, DBG_RS, "r300: FS input color %i unassigned%s.\n",
                    i);
            }
        }
    }

    /* Set up back-face colors. The rasterizer will do the color selection
     * automatically. */
    if (any_bcolor_used) {
        if (r300->two_sided_color) {
            /* Rasterize as back-face colors. */
            for (i = 0; i < ATTR_COLOR_COUNT; i++) {
                rs.vap_vsm_vtx_assm |= R300_INPUT_CNTL_COLOR;
                rs.vap_out_vtx_fmt[0] |= R300_VAP_OUTPUT_VTX_FMT_0__COLOR_0_PRESENT << (2+i);
                stream_loc_notcl[loc++] = 4 + i;
            }
        } else {
            /* Rasterize two fake texcoords to prevent from the two-sided color
             * selection. */
            /* XXX Consider recompiling the vertex shader to save 2 RS units. */
            for (i = 0; i < 2; i++) {
                rs.vap_vsm_vtx_assm |= (R300_INPUT_CNTL_TC0 << tex_count);
                rs.vap_out_vtx_fmt[1] |= (4 << (3 * tex_count));
                stream_loc_notcl[loc++] = 6 + tex_count;

                /* Rasterize it. */
                rX00_rs_tex(&rs, tex_count, tex_ptr, SWIZ_XYZW);
                tex_count++;
                tex_ptr += 4;
            }
        }
    }

    /* gl_FrontFacing.
     * Note that we can use either the two-sided color selection based on
     * the front and back vertex shader colors, or gl_FrontFacing,
     * but not both! It locks up otherwise.
     *
     * In Direct3D 9, the two-sided color selection can be used
     * with shaders 2.0 only, while gl_FrontFacing can be used
     * with shaders 3.0 only. The hardware apparently hasn't been designed
     * to support both at the same time. */
    if (r300->screen->caps.is_r500 && fs_inputs->face != ATTR_UNUSED &&
        !(any_bcolor_used && r300->two_sided_color)) {
        rX00_rs_col(&rs, col_count, col_count, SWIZ_XYZW);
        rX00_rs_col_write(&rs, col_count, fp_offset, WRITE_FACE);
        fp_offset++;
        col_count++;
        DBG(r300, DBG_RS, "r300: Rasterized FACE written to FS.\n");
    }

    /* Rasterize texture coordinates. */
    for (i = 0; i < ATTR_GENERIC_COUNT && tex_count < 8; i++) {
	bool sprite_coord = false;

	if (fs_inputs->generic[i] != ATTR_UNUSED) {
	    sprite_coord = !!(r300->sprite_coord_enable & (1 << i));
	}

        if (vs_outputs->generic[i] != ATTR_UNUSED || sprite_coord) {
            if (!sprite_coord) {
                /* Set up the texture coordinates in VAP. */
                rs.vap_vsm_vtx_assm |= (R300_INPUT_CNTL_TC0 << tex_count);
                rs.vap_out_vtx_fmt[1] |= (4 << (3 * tex_count));
                stream_loc_notcl[loc++] = 6 + tex_count;
            } else
                stuffing_enable |=
                    R300_GB_TEX_ST << (R300_GB_TEX0_SOURCE_SHIFT + (tex_count*2));

            /* Rasterize it. */
            rX00_rs_tex(&rs, tex_count, tex_ptr,
			sprite_coord ? SWIZ_XY01 : SWIZ_XYZW);

            /* Write it to the FS input register if it's needed by the FS. */
            if (fs_inputs->generic[i] != ATTR_UNUSED) {
                rX00_rs_tex_write(&rs, tex_count, fp_offset);
                fp_offset++;

                DBG(r300, DBG_RS,
                    "r300: Rasterized generic %i written to FS%s in texcoord %d.\n",
                    i, sprite_coord ? " (sprite coord)" : "", tex_count);
            } else {
                DBG(r300, DBG_RS,
                    "r300: Rasterized generic %i unused%s.\n",
                    i, sprite_coord ? " (sprite coord)" : "");
            }
            tex_count++;
            tex_ptr += sprite_coord ? 2 : 4;
        } else {
            /* Skip the FS input register, leave it uninitialized. */
            /* If we try to set it to (0,0,0,1), it will lock up. */
            if (fs_inputs->generic[i] != ATTR_UNUSED) {
                fp_offset++;

                DBG(r300, DBG_RS, "r300: FS input generic %i unassigned%s.\n",
                    i, sprite_coord ? " (sprite coord)" : "");
            }
        }
    }

    if (DBG_ON(r300, DBG_RS)) {
        for (; i < ATTR_GENERIC_COUNT; i++) {
            if (fs_inputs->generic[i] != ATTR_UNUSED) {
                DBG(r300, DBG_RS,
                    "r300: FS input generic %i unassigned.\n", i);
            }
        }
    }

    /* Rasterize fog coordinates. */
    if (vs_outputs->fog != ATTR_UNUSED && tex_count < 8) {
        /* Set up the fog coordinates in VAP. */
        rs.vap_vsm_vtx_assm |= (R300_INPUT_CNTL_TC0 << tex_count);
        rs.vap_out_vtx_fmt[1] |= (4 << (3 * tex_count));
        stream_loc_notcl[loc++] = 6 + tex_count;

        /* Rasterize it. */
        rX00_rs_tex(&rs, tex_count, tex_ptr, SWIZ_X001);

        /* Write it to the FS input register if it's needed by the FS. */
        if (fs_inputs->fog != ATTR_UNUSED) {
            rX00_rs_tex_write(&rs, tex_count, fp_offset);
            fp_offset++;

            DBG(r300, DBG_RS, "r300: Rasterized fog written to FS.\n");
        } else {
            DBG(r300, DBG_RS, "r300: Rasterized fog unused.\n");
        }
        tex_count++;
        tex_ptr += 4;
    } else {
        /* Skip the FS input register, leave it uninitialized. */
        /* If we try to set it to (0,0,0,1), it will lock up. */
        if (fs_inputs->fog != ATTR_UNUSED) {
            fp_offset++;

            DBG(r300, DBG_RS, "r300: FS input fog unassigned.\n");
        }
    }

    /* Rasterize WPOS. */
    /* Don't set it in VAP if the FS doesn't need it. */
    if (fs_inputs->wpos != ATTR_UNUSED && tex_count < 8) {
        /* Set up the WPOS coordinates in VAP. */
        rs.vap_vsm_vtx_assm |= (R300_INPUT_CNTL_TC0 << tex_count);
        rs.vap_out_vtx_fmt[1] |= (4 << (3 * tex_count));
        stream_loc_notcl[loc++] = 6 + tex_count;

        /* Rasterize it. */
        rX00_rs_tex(&rs, tex_count, tex_ptr, SWIZ_XYZW);

        /* Write it to the FS input register. */
        rX00_rs_tex_write(&rs, tex_count, fp_offset);

        DBG(r300, DBG_RS, "r300: Rasterized WPOS written to FS.\n");

        fp_offset++;
        tex_count++;
        tex_ptr += 4;
    }

    /* Invalidate the rest of the no-TCL (GA) stream locations. */
    for (; loc < 16;) {
        stream_loc_notcl[loc++] = -1;
    }

    /* Rasterize at least one color, or bad things happen. */
    if (col_count == 0 && tex_count == 0) {
        rX00_rs_col(&rs, 0, 0, SWIZ_0001);
        col_count++;

        DBG(r300, DBG_RS, "r300: Rasterized color 0 to prevent lockups.\n");
    }

    DBG(r300, DBG_RS, "r300: --- Rasterizer status ---: colors: %i, "
        "generics: %i.\n", col_count, tex_count);

    rs.count = MIN2(tex_ptr, 32) | (col_count << R300_IC_COUNT_SHIFT) |
        R300_HIRES_EN;

    count = MAX3(col_count, tex_count, 1);
    rs.inst_count = count - 1;

    /* set the GB enable flags */
    if (r300->sprite_coord_enable)
	stuffing_enable |= R300_GB_POINT_STUFF_ENABLE;

    rs.gb_enable = stuffing_enable;

    /* Now, after all that, see if we actually need to update the state. */
    if (memcmp(r300->rs_block_state.state, &rs, sizeof(struct r300_rs_block))) {
        memcpy(r300->rs_block_state.state, &rs, sizeof(struct r300_rs_block));
        r300->rs_block_state.size = 13 + count*2;
    }
}

static uint32_t r300_get_border_color(enum pipe_format format,
                                      const float border[4])
{
    const struct util_format_description *desc;
    float border_swizzled[4] = {
        border[2],
        border[1],
        border[0],
        border[3]
    };
    uint32_t r;

    desc = util_format_description(format);

    /* We don't use util_pack_format because it does not handle the formats
     * we want, e.g. R4G4B4A4 is non-existent in Gallium. */
    switch (desc->channel[0].size) {
        case 4:
            r = ((float_to_ubyte(border_swizzled[0]) & 0xf0) >> 4) |
                ((float_to_ubyte(border_swizzled[1]) & 0xf0) << 0) |
                ((float_to_ubyte(border_swizzled[2]) & 0xf0) << 4) |
                ((float_to_ubyte(border_swizzled[3]) & 0xf0) << 8);
            break;

        case 5:
            if (desc->channel[1].size == 5) {
                r = ((float_to_ubyte(border_swizzled[0]) & 0xf8) >> 3) |
                    ((float_to_ubyte(border_swizzled[1]) & 0xf8) << 2) |
                    ((float_to_ubyte(border_swizzled[2]) & 0xf8) << 7) |
                    ((float_to_ubyte(border_swizzled[3]) & 0x80) << 8);
            } else if (desc->channel[1].size == 6) {
                r = ((float_to_ubyte(border_swizzled[0]) & 0xf8) >> 3) |
                    ((float_to_ubyte(border_swizzled[1]) & 0xfc) << 3) |
                    ((float_to_ubyte(border_swizzled[2]) & 0xf8) << 8);
            } else {
                assert(0);
                r = 0;
            }
            break;

        case 8:
            r = ((float_to_ubyte(border_swizzled[0]) & 0xff) << 0) |
                ((float_to_ubyte(border_swizzled[1]) & 0xff) << 8) |
                ((float_to_ubyte(border_swizzled[2]) & 0xff) << 16) |
                ((float_to_ubyte(border_swizzled[3]) & 0xff) << 24);
            break;

        case 16:
            r = ((float_to_ubyte(border_swizzled[2]) & 0xff) << 0) |
                ((float_to_ubyte(border_swizzled[1]) & 0xff) << 8) |
                ((float_to_ubyte(border_swizzled[0]) & 0xff) << 16) |
                ((float_to_ubyte(border_swizzled[3]) & 0xff) << 24);
            break;
    }

    return r;
}

static void r300_merge_textures_and_samplers(struct r300_context* r300)
{
    struct r300_textures_state *state =
        (struct r300_textures_state*)r300->textures_state.state;
    struct r300_texture_sampler_state *texstate;
    struct r300_sampler_state *sampler;
    struct r300_sampler_view *view;
    struct r300_texture *tex;
    unsigned min_level, max_level, i, j, size;
    unsigned count = MIN2(state->sampler_view_count,
                          state->sampler_state_count);

    /* The KIL opcode fix, see below. */
    if (!count && !r300->screen->caps.is_r500)
        count = 1;

    state->tx_enable = 0;
    state->count = 0;
    size = 2;

    for (i = 0; i < count; i++) {
        if (state->sampler_views[i] && state->sampler_states[i]) {
            state->tx_enable |= 1 << i;

            view = state->sampler_views[i];
            tex = r300_texture(view->base.texture);
            sampler = state->sampler_states[i];

            texstate = &state->regs[i];
            texstate->format = view->format;
            texstate->filter0 = sampler->filter0;
            texstate->filter1 = sampler->filter1;

            /* Set the border color. */
            texstate->border_color =
                r300_get_border_color(view->base.format,
                                      sampler->state.border_color);

            /* determine min/max levels */
            max_level = MIN3(sampler->max_lod + view->base.first_level,
                             tex->desc.b.b.last_level, view->base.last_level);
            min_level = MIN2(sampler->min_lod + view->base.first_level,
                             max_level);

            if (tex->desc.is_npot && min_level > 0) {
                /* Even though we do not implement mipmapping for NPOT
                 * textures, we should at least honor the minimum level
                 * which is allowed to be displayed. We do this by setting up
                 * an i-th mipmap level as the zero level. */
                r300_texture_setup_format_state(r300->screen, &tex->desc,
                                                min_level,
                                                &texstate->format);
                texstate->format.tile_config |=
                        tex->desc.offset_in_bytes[min_level] & 0xffffffe0;
                assert((tex->desc.offset_in_bytes[min_level] & 0x1f) == 0);
            }

            /* Assign a texture cache region. */
            texstate->format.format1 |= view->texcache_region;

            /* Depth textures are kinda special. */
            if (util_format_is_depth_or_stencil(tex->desc.b.b.format)) {
                unsigned char depth_swizzle[4];

                if (!r300->screen->caps.is_r500 &&
                    util_format_get_blocksizebits(tex->desc.b.b.format) == 32) {
                    /* X24x8 is sampled as Y16X16 on r3xx-r4xx.
                     * The depth here is at the Y component. */
                    for (j = 0; j < 4; j++)
                        depth_swizzle[j] = UTIL_FORMAT_SWIZZLE_Y;
                } else {
                    for (j = 0; j < 4; j++)
                        depth_swizzle[j] = UTIL_FORMAT_SWIZZLE_X;
                }

                /* If compare mode is disabled, sampler view swizzles
                 * are stored in the format.
                 * Otherwise, the swizzles must be applied after the compare
                 * mode in the fragment shader. */
                if (sampler->state.compare_mode == PIPE_TEX_COMPARE_NONE) {
                    texstate->format.format1 |=
                        r300_get_swizzle_combined(depth_swizzle,
                                                  view->swizzle);
                } else {
                    texstate->format.format1 |=
                        r300_get_swizzle_combined(depth_swizzle, 0);
                }
            }

            /* to emulate 1D textures through 2D ones correctly */
            if (tex->desc.b.b.target == PIPE_TEXTURE_1D) {
                texstate->filter0 &= ~R300_TX_WRAP_T_MASK;
                texstate->filter0 |= R300_TX_WRAP_T(R300_TX_CLAMP_TO_EDGE);
            }

            if (tex->desc.is_npot) {
                /* NPOT textures don't support mip filter, unfortunately.
                 * This prevents incorrect rendering. */
                texstate->filter0 &= ~R300_TX_MIN_FILTER_MIP_MASK;

                /* Mask out the mirrored flag. */
                if (texstate->filter0 & R300_TX_WRAP_S(R300_TX_MIRRORED)) {
                    texstate->filter0 &= ~R300_TX_WRAP_S(R300_TX_MIRRORED);
                }
                if (texstate->filter0 & R300_TX_WRAP_T(R300_TX_MIRRORED)) {
                    texstate->filter0 &= ~R300_TX_WRAP_T(R300_TX_MIRRORED);
                }

                /* Change repeat to clamp-to-edge.
                 * (the repeat bit has a value of 0, no masking needed). */
                if ((texstate->filter0 & R300_TX_WRAP_S_MASK) ==
                    R300_TX_WRAP_S(R300_TX_REPEAT)) {
                    texstate->filter0 |= R300_TX_WRAP_S(R300_TX_CLAMP_TO_EDGE);
                }
                if ((texstate->filter0 & R300_TX_WRAP_T_MASK) ==
                    R300_TX_WRAP_T(R300_TX_REPEAT)) {
                    texstate->filter0 |= R300_TX_WRAP_T(R300_TX_CLAMP_TO_EDGE);
                }
            } else {
                /* the MAX_MIP level is the largest (finest) one */
                texstate->format.format0 |= R300_TX_NUM_LEVELS(max_level);
                texstate->filter0 |= R300_TX_MAX_MIP_LEVEL(min_level);
            }

            texstate->filter0 |= i << 28;

            size += 16;
            state->count = i+1;
        } else {
            /* For the KIL opcode to work on r3xx-r4xx, the texture unit
             * assigned to this opcode (it's always the first one) must be
             * enabled. Otherwise the opcode doesn't work.
             *
             * In order to not depend on the fragment shader, we just make
             * the first unit enabled all the time. */
            if (i == 0 && !r300->screen->caps.is_r500) {
                pipe_sampler_view_reference(
                        (struct pipe_sampler_view**)&state->sampler_views[i],
                        &r300->texkill_sampler->base);

                state->tx_enable |= 1 << i;

                texstate = &state->regs[i];

                /* Just set some valid state. */
                texstate->format = r300->texkill_sampler->format;
                texstate->filter0 =
                        r300_translate_tex_filters(PIPE_TEX_FILTER_NEAREST,
                                                   PIPE_TEX_FILTER_NEAREST,
                                                   PIPE_TEX_FILTER_NEAREST,
                                                   FALSE);
                texstate->filter1 = 0;
                texstate->border_color = 0;

                texstate->filter0 |= i << 28;
                size += 16;
                state->count = i+1;
            }
        }
    }

    r300->textures_state.size = size;

    /* Pick a fragment shader based on either the texture compare state
     * or the uses_pitch flag. */
    if (r300->fs.state && count) {
        if (r300_pick_fragment_shader(r300)) {
            r300_mark_fs_code_dirty(r300);
        }
    }
}

/* We can't use compressed zbuffers as samplers. */
static void r300_flush_depth_textures(struct r300_context *r300)
{
    struct r300_textures_state *state =
        (struct r300_textures_state*)r300->textures_state.state;
    unsigned i, level;
    unsigned count = MIN2(state->sampler_view_count,
                          state->sampler_state_count);

    if (r300->z_decomp_rd)
        return;

    for (i = 0; i < count; i++)
        if (state->sampler_views[i] && state->sampler_states[i]) {
            struct pipe_resource *tex = state->sampler_views[i]->base.texture;

            if (tex->target == PIPE_TEXTURE_3D ||
                tex->target == PIPE_TEXTURE_CUBE)
                continue;

            /* Ignore non-depth textures.
             * Also ignore reinterpreted depth textures, e.g. resource_copy. */
            if (!util_format_is_depth_or_stencil(tex->format))
                continue;

            for (level = 0; level <= tex->last_level; level++)
                if (r300_texture(tex)->zmask_in_use[level]) {
                    /* We don't handle 3D textures and cubemaps yet. */
                    r300_flush_depth_stencil(&r300->context, tex,
                                             u_subresource(0, level), 0);
                }
        }
}

void r300_update_derived_state(struct r300_context* r300)
{
    r300_flush_depth_textures(r300);

    if (r300->textures_state.dirty) {
        r300_merge_textures_and_samplers(r300);
    }

    if (r300->rs_block_state.dirty) {
        r300_update_rs_block(r300);

        if (r300->draw) {
            memset(&r300->vertex_info, 0, sizeof(struct vertex_info));
            r300_draw_emit_all_attribs(r300);
            draw_compute_vertex_size(&r300->vertex_info);
            r300_swtcl_vertex_psc(r300);
        }
    }

    r300_update_hyperz_state(r300);
}
