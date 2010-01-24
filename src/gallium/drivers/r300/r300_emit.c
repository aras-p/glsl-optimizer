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

/* r300_emit: Functions for emitting state. */

#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_simple_list.h"

#include "r300_context.h"
#include "r300_cs.h"
#include "r300_emit.h"
#include "r300_fs.h"
#include "r300_screen.h"
#include "r300_state_derived.h"
#include "r300_state_inlines.h"
#include "r300_texture.h"
#include "r300_vs.h"

void r300_emit_blend_state(struct r300_context* r300, void* state)
{
    struct r300_blend_state* blend = (struct r300_blend_state*)state;
    CS_LOCALS(r300);

    BEGIN_CS(8);
    OUT_CS_REG(R300_RB3D_ROPCNTL, blend->rop);
    OUT_CS_REG_SEQ(R300_RB3D_CBLEND, 3);
    if (r300->framebuffer_state.nr_cbufs) {
        OUT_CS(blend->blend_control);
        OUT_CS(blend->alpha_blend_control);
        OUT_CS(blend->color_channel_mask);
    } else {
        OUT_CS(0);
        OUT_CS(0);
        OUT_CS(0);
        /* XXX also disable fastfill here once it's supported */
    }
    OUT_CS_REG(R300_RB3D_DITHER_CTL, blend->dither);
    END_CS;
}

void r300_emit_blend_color_state(struct r300_context* r300, void* state)
{
    struct r300_blend_color_state* bc = (struct r300_blend_color_state*)state;
    struct r300_screen* r300screen = r300_screen(r300->context.screen);
    CS_LOCALS(r300);

    if (r300screen->caps->is_r500) {
        BEGIN_CS(3);
        OUT_CS_REG_SEQ(R500_RB3D_CONSTANT_COLOR_AR, 2);
        OUT_CS(bc->blend_color_red_alpha);
        OUT_CS(bc->blend_color_green_blue);
        END_CS;
    } else {
        BEGIN_CS(2);
        OUT_CS_REG(R300_RB3D_BLEND_COLOR, bc->blend_color);
        END_CS;
    }
}

void r300_emit_clip_state(struct r300_context* r300, void* state)
{
    struct pipe_clip_state* clip = (struct pipe_clip_state*)state;
    int i;
    struct r300_screen* r300screen = r300_screen(r300->context.screen);
    CS_LOCALS(r300);

    if (r300screen->caps->has_tcl) {
        BEGIN_CS(5 + (6 * 4));
        OUT_CS_REG(R300_VAP_PVS_VECTOR_INDX_REG,
                (r300screen->caps->is_r500 ?
                 R500_PVS_UCP_START : R300_PVS_UCP_START));
        OUT_CS_ONE_REG(R300_VAP_PVS_UPLOAD_DATA, 6 * 4);
        for (i = 0; i < 6; i++) {
            OUT_CS_32F(clip->ucp[i][0]);
            OUT_CS_32F(clip->ucp[i][1]);
            OUT_CS_32F(clip->ucp[i][2]);
            OUT_CS_32F(clip->ucp[i][3]);
        }
        OUT_CS_REG(R300_VAP_CLIP_CNTL, ((1 << clip->nr) - 1) |
                R300_PS_UCP_MODE_CLIP_AS_TRIFAN);
        END_CS;
    } else {
        BEGIN_CS(2);
        OUT_CS_REG(R300_VAP_CLIP_CNTL, R300_CLIP_DISABLE);
        END_CS;
    }

}

void r300_emit_dsa_state(struct r300_context* r300, void* state)
{
    struct r300_dsa_state* dsa = (struct r300_dsa_state*)state;
    struct r300_screen* r300screen = r300_screen(r300->context.screen);
    CS_LOCALS(r300);

    BEGIN_CS(r300screen->caps->is_r500 ? 8 : 6);
    OUT_CS_REG(R300_FG_ALPHA_FUNC, dsa->alpha_function);

    /* not needed since we use the 8bit alpha ref */
    /*if (r300screen->caps->is_r500) {
        OUT_CS_REG(R500_FG_ALPHA_VALUE, dsa->alpha_reference);
    }*/

    OUT_CS_REG_SEQ(R300_ZB_CNTL, 3);

    if (r300->framebuffer_state.zsbuf) {
        OUT_CS(dsa->z_buffer_control);
        OUT_CS(dsa->z_stencil_control);
    } else {
        OUT_CS(0);
        OUT_CS(0);
    }

    OUT_CS(dsa->stencil_ref_mask);

    /* XXX it seems r3xx doesn't support STENCILREFMASK_BF */
    if (r300screen->caps->is_r500) {
        OUT_CS_REG(R500_ZB_STENCILREFMASK_BF, dsa->stencil_ref_bf);
    }
    END_CS;
}

static const float * get_shader_constant(
    struct r300_context * r300,
    struct rc_constant * constant,
    struct r300_constant_buffer * externals)
{
    struct r300_viewport_state* viewport =
        (struct r300_viewport_state*)r300->viewport_state.state;
    static float vec[4] = { 0.0, 0.0, 0.0, 1.0 };
    struct pipe_texture *tex;

    switch(constant->Type) {
        case RC_CONSTANT_EXTERNAL:
            return externals->constants[constant->u.External];

        case RC_CONSTANT_IMMEDIATE:
            return constant->u.Immediate;

        case RC_CONSTANT_STATE:
            switch (constant->u.State[0]) {
                /* Factor for converting rectangle coords to
                 * normalized coords. Should only show up on non-r500. */
                case RC_STATE_R300_TEXRECT_FACTOR:
                    tex = &r300->textures[constant->u.State[1]]->tex;
                    vec[0] = 1.0 / tex->width0;
                    vec[1] = 1.0 / tex->height0;
                    break;

                /* Texture compare-fail value. */
                /* XXX Since Gallium doesn't support GL_ARB_shadow_ambient,
                 * this is always (0,0,0,0), right? */
                case RC_STATE_SHADOW_AMBIENT:
                    vec[3] = 0;
                    break;

                case RC_STATE_R300_VIEWPORT_SCALE:
                    if (r300->tcl_bypass) {
                        vec[0] = 1;
                        vec[1] = 1;
                        vec[2] = 1;
                    } else {
                        vec[0] = viewport->xscale;
                        vec[1] = viewport->yscale;
                        vec[2] = viewport->zscale;
                    }
                    break;

                case RC_STATE_R300_VIEWPORT_OFFSET:
                    if (!r300->tcl_bypass) {
                        vec[0] = viewport->xoffset;
                        vec[1] = viewport->yoffset;
                        vec[2] = viewport->zoffset;
                    }
                    break;

                default:
                    debug_printf("r300: Implementation error: "
                        "Unknown RC_CONSTANT type %d\n", constant->u.State[0]);
            }
            break;

        default:
            debug_printf("r300: Implementation error: "
                "Unhandled constant type %d\n", constant->Type);
    }

    /* This should either be (0, 0, 0, 1), which should be a relatively safe
     * RGBA or STRQ value, or it could be one of the RC_CONSTANT_STATE
     * state factors. */
    return vec;
}

/* Convert a normal single-precision float into the 7.16 format
 * used by the R300 fragment shader.
 */
static uint32_t pack_float24(float f)
{
    union {
        float fl;
        uint32_t u;
    } u;
    float mantissa;
    int exponent;
    uint32_t float24 = 0;

    if (f == 0.0)
        return 0;

    u.fl = f;

    mantissa = frexpf(f, &exponent);

    /* Handle -ve */
    if (mantissa < 0) {
        float24 |= (1 << 23);
        mantissa = mantissa * -1.0;
    }
    /* Handle exponent, bias of 63 */
    exponent += 62;
    float24 |= (exponent << 16);
    /* Kill 7 LSB of mantissa */
    float24 |= (u.u & 0x7FFFFF) >> 7;

    return float24;
}

void r300_emit_fragment_program_code(struct r300_context* r300,
                                     struct rX00_fragment_program_code* generic_code)
{
    struct r300_fragment_program_code * code = &generic_code->code.r300;
    int i;
    CS_LOCALS(r300);

    BEGIN_CS(15 +
             code->alu.length * 4 +
             (code->tex.length ? (1 + code->tex.length) : 0));

    OUT_CS_REG(R300_US_CONFIG, code->config);
    OUT_CS_REG(R300_US_PIXSIZE, code->pixsize);
    OUT_CS_REG(R300_US_CODE_OFFSET, code->code_offset);

    OUT_CS_REG_SEQ(R300_US_CODE_ADDR_0, 4);
    for(i = 0; i < 4; ++i)
        OUT_CS(code->code_addr[i]);

    OUT_CS_REG_SEQ(R300_US_ALU_RGB_INST_0, code->alu.length);
    for (i = 0; i < code->alu.length; i++)
        OUT_CS(code->alu.inst[i].rgb_inst);

    OUT_CS_REG_SEQ(R300_US_ALU_RGB_ADDR_0, code->alu.length);
    for (i = 0; i < code->alu.length; i++)
        OUT_CS(code->alu.inst[i].rgb_addr);

    OUT_CS_REG_SEQ(R300_US_ALU_ALPHA_INST_0, code->alu.length);
    for (i = 0; i < code->alu.length; i++)
        OUT_CS(code->alu.inst[i].alpha_inst);

    OUT_CS_REG_SEQ(R300_US_ALU_ALPHA_ADDR_0, code->alu.length);
    for (i = 0; i < code->alu.length; i++)
        OUT_CS(code->alu.inst[i].alpha_addr);

    if (code->tex.length) {
        OUT_CS_REG_SEQ(R300_US_TEX_INST_0, code->tex.length);
        for(i = 0; i < code->tex.length; ++i)
            OUT_CS(code->tex.inst[i]);
    }

    END_CS;
}

void r300_emit_fs_constant_buffer(struct r300_context* r300,
                                  struct rc_constant_list* constants)
{
    int i;
    CS_LOCALS(r300);

    if (constants->Count == 0)
        return;

    BEGIN_CS(constants->Count * 4 + 1);
    OUT_CS_REG_SEQ(R300_PFS_PARAM_0_X, constants->Count * 4);
    for(i = 0; i < constants->Count; ++i) {
        const float * data = get_shader_constant(r300,
                                                 &constants->Constants[i],
                                                 &r300->shader_constants[PIPE_SHADER_FRAGMENT]);
        OUT_CS(pack_float24(data[0]));
        OUT_CS(pack_float24(data[1]));
        OUT_CS(pack_float24(data[2]));
        OUT_CS(pack_float24(data[3]));
    }
    END_CS;
}

static void r300_emit_fragment_depth_config(struct r300_context* r300,
                                            struct r300_fragment_shader* fs)
{
    CS_LOCALS(r300);

    BEGIN_CS(4);
    if (r300_fragment_shader_writes_depth(fs)) {
        OUT_CS_REG(R300_FG_DEPTH_SRC, R300_FG_DEPTH_SRC_SHADER);
        OUT_CS_REG(R300_US_W_FMT, R300_W_FMT_W24 | R300_W_SRC_US);
    } else {
        OUT_CS_REG(R300_FG_DEPTH_SRC, R300_FG_DEPTH_SRC_SCAN);
        OUT_CS_REG(R300_US_W_FMT, R300_W_FMT_W0 | R300_W_SRC_US);
    }
    END_CS;
}

void r500_emit_fragment_program_code(struct r300_context* r300,
                                     struct rX00_fragment_program_code* generic_code)
{
    struct r500_fragment_program_code * code = &generic_code->code.r500;
    int i;
    CS_LOCALS(r300);

    BEGIN_CS(13 +
             ((code->inst_end + 1) * 6));
    OUT_CS_REG(R500_US_CONFIG, R500_ZERO_TIMES_ANYTHING_EQUALS_ZERO);
    OUT_CS_REG(R500_US_PIXSIZE, code->max_temp_idx);
    OUT_CS_REG(R500_US_CODE_RANGE,
               R500_US_CODE_RANGE_ADDR(0) | R500_US_CODE_RANGE_SIZE(code->inst_end));
    OUT_CS_REG(R500_US_CODE_OFFSET, 0);
    OUT_CS_REG(R500_US_CODE_ADDR,
               R500_US_CODE_START_ADDR(0) | R500_US_CODE_END_ADDR(code->inst_end));

    OUT_CS_REG(R500_GA_US_VECTOR_INDEX, R500_GA_US_VECTOR_INDEX_TYPE_INSTR);
    OUT_CS_ONE_REG(R500_GA_US_VECTOR_DATA, (code->inst_end + 1) * 6);
    for (i = 0; i <= code->inst_end; i++) {
        OUT_CS(code->inst[i].inst0);
        OUT_CS(code->inst[i].inst1);
        OUT_CS(code->inst[i].inst2);
        OUT_CS(code->inst[i].inst3);
        OUT_CS(code->inst[i].inst4);
        OUT_CS(code->inst[i].inst5);
    }

    END_CS;
}

void r500_emit_fs_constant_buffer(struct r300_context* r300,
                                  struct rc_constant_list* constants)
{
    int i;
    CS_LOCALS(r300);

    if (constants->Count == 0)
        return;

    BEGIN_CS(constants->Count * 4 + 3);
    OUT_CS_REG(R500_GA_US_VECTOR_INDEX, R500_GA_US_VECTOR_INDEX_TYPE_CONST);
    OUT_CS_ONE_REG(R500_GA_US_VECTOR_DATA, constants->Count * 4);
    for (i = 0; i < constants->Count; i++) {
        const float * data = get_shader_constant(r300,
                                                 &constants->Constants[i],
                                                 &r300->shader_constants[PIPE_SHADER_FRAGMENT]);
        OUT_CS_32F(data[0]);
        OUT_CS_32F(data[1]);
        OUT_CS_32F(data[2]);
        OUT_CS_32F(data[3]);
    }
    END_CS;
}

void r300_emit_fb_state(struct r300_context* r300,
                        struct pipe_framebuffer_state* fb)
{
    struct r300_texture* tex;
    struct pipe_surface* surf;
    int i;
    CS_LOCALS(r300);

    /* Shouldn't fail unless there is a bug in the state tracker. */
    assert(fb->nr_cbufs <= 4);

    BEGIN_CS((10 * fb->nr_cbufs) + (2 * (4 - fb->nr_cbufs)) +
             (fb->zsbuf ? 10 : 0) + 6);

    /* Flush and free renderbuffer caches. */
    OUT_CS_REG(R300_RB3D_DSTCACHE_CTLSTAT,
        R300_RB3D_DSTCACHE_CTLSTAT_DC_FREE_FREE_3D_TAGS |
        R300_RB3D_DSTCACHE_CTLSTAT_DC_FLUSH_FLUSH_DIRTY_3D);
    OUT_CS_REG(R300_ZB_ZCACHE_CTLSTAT,
        R300_ZB_ZCACHE_CTLSTAT_ZC_FLUSH_FLUSH_AND_FREE |
        R300_ZB_ZCACHE_CTLSTAT_ZC_FREE_FREE);

    /* Set the number of colorbuffers. */
    OUT_CS_REG(R300_RB3D_CCTL, R300_RB3D_CCTL_NUM_MULTIWRITES(fb->nr_cbufs));

    /* Set up colorbuffers. */
    for (i = 0; i < fb->nr_cbufs; i++) {
        surf = fb->cbufs[i];
        tex = (struct r300_texture*)surf->texture;
        assert(tex && tex->buffer && "cbuf is marked, but NULL!");

        OUT_CS_REG_SEQ(R300_RB3D_COLOROFFSET0 + (4 * i), 1);
        OUT_CS_RELOC(tex->buffer, surf->offset, 0, RADEON_GEM_DOMAIN_VRAM, 0);

        OUT_CS_REG_SEQ(R300_RB3D_COLORPITCH0 + (4 * i), 1);
        OUT_CS_RELOC(tex->buffer, tex->pitch[surf->level] |
                     r300_translate_colorformat(tex->tex.format) |
                     R300_COLOR_TILE(tex->macrotile) |
                     R300_COLOR_MICROTILE(tex->microtile),
                     0, RADEON_GEM_DOMAIN_VRAM, 0);

        OUT_CS_REG(R300_US_OUT_FMT_0 + (4 * i),
            r300_translate_out_fmt(surf->format));
    }

    /* Disable unused colorbuffers. */
    for (; i < 4; i++) {
        OUT_CS_REG(R300_US_OUT_FMT_0 + (4 * i), R300_US_OUT_FMT_UNUSED);
    }

    /* Set up a zbuffer. */
    if (fb->zsbuf) {
        surf = fb->zsbuf;
        tex = (struct r300_texture*)surf->texture;
        assert(tex && tex->buffer && "zsbuf is marked, but NULL!");

        OUT_CS_REG_SEQ(R300_ZB_DEPTHOFFSET, 1);
        OUT_CS_RELOC(tex->buffer, surf->offset, 0, RADEON_GEM_DOMAIN_VRAM, 0);

        OUT_CS_REG(R300_ZB_FORMAT, r300_translate_zsformat(tex->tex.format));

        OUT_CS_REG_SEQ(R300_ZB_DEPTHPITCH, 1);
        OUT_CS_RELOC(tex->buffer, tex->pitch[surf->level] |
                     R300_DEPTHMACROTILE(tex->macrotile) |
                     R300_DEPTHMICROTILE(tex->microtile),
                     0, RADEON_GEM_DOMAIN_VRAM, 0);
    }

    END_CS;
}

static void r300_emit_query_start(struct r300_context *r300)
{
    struct r300_capabilities *caps = r300_screen(r300->context.screen)->caps;
    struct r300_query *query = r300->query_current;
    CS_LOCALS(r300);

    if (!query)
	return;

    BEGIN_CS(4);
    if (caps->family == CHIP_FAMILY_RV530) {
        OUT_CS_REG(RV530_FG_ZBREG_DEST, RV530_FG_ZBREG_DEST_PIPE_SELECT_ALL);
    } else {
        OUT_CS_REG(R300_SU_REG_DEST, R300_RASTER_PIPE_SELECT_ALL);
    }
    OUT_CS_REG(R300_ZB_ZPASS_DATA, 0);
    END_CS;
    query->begin_emitted = TRUE;
}


static void r300_emit_query_finish(struct r300_context *r300,
                                   struct r300_query *query)
{
    struct r300_capabilities* caps = r300_screen(r300->context.screen)->caps;
    CS_LOCALS(r300);

    assert(caps->num_frag_pipes);

    BEGIN_CS(6 * caps->num_frag_pipes + 2);
    /* I'm not so sure I like this switch, but it's hard to be elegant
     * when there's so many special cases...
     *
     * So here's the basic idea. For each pipe, enable writes to it only,
     * then put out the relocation for ZPASS_ADDR, taking into account a
     * 4-byte offset for each pipe. RV380 and older are special; they have
     * only two pipes, and the second pipe's enable is on bit 3, not bit 1,
     * so there's a chipset cap for that. */
    switch (caps->num_frag_pipes) {
        case 4:
            /* pipe 3 only */
            OUT_CS_REG(R300_SU_REG_DEST, 1 << 3);
            OUT_CS_REG_SEQ(R300_ZB_ZPASS_ADDR, 1);
            OUT_CS_RELOC(r300->oqbo, query->offset + (sizeof(uint32_t) * 3),
                    0, RADEON_GEM_DOMAIN_GTT, 0);
        case 3:
            /* pipe 2 only */
            OUT_CS_REG(R300_SU_REG_DEST, 1 << 2);
            OUT_CS_REG_SEQ(R300_ZB_ZPASS_ADDR, 1);
            OUT_CS_RELOC(r300->oqbo, query->offset + (sizeof(uint32_t) * 2),
                    0, RADEON_GEM_DOMAIN_GTT, 0);
        case 2:
            /* pipe 1 only */
            /* As mentioned above, accomodate RV380 and older. */
            OUT_CS_REG(R300_SU_REG_DEST,
                    1 << (caps->high_second_pipe ? 3 : 1));
            OUT_CS_REG_SEQ(R300_ZB_ZPASS_ADDR, 1);
            OUT_CS_RELOC(r300->oqbo, query->offset + (sizeof(uint32_t) * 1),
                    0, RADEON_GEM_DOMAIN_GTT, 0);
        case 1:
            /* pipe 0 only */
            OUT_CS_REG(R300_SU_REG_DEST, 1 << 0);
            OUT_CS_REG_SEQ(R300_ZB_ZPASS_ADDR, 1);
            OUT_CS_RELOC(r300->oqbo, query->offset + (sizeof(uint32_t) * 0),
                    0, RADEON_GEM_DOMAIN_GTT, 0);
            break;
        default:
            debug_printf("r300: Implementation error: Chipset reports %d"
                    " pixel pipes!\n", caps->num_frag_pipes);
            assert(0);
    }

    /* And, finally, reset it to normal... */
    OUT_CS_REG(R300_SU_REG_DEST, 0xF);
    END_CS;
}

static void rv530_emit_query_single(struct r300_context *r300,
                                    struct r300_query *query)
{
    CS_LOCALS(r300);

    BEGIN_CS(8);
    OUT_CS_REG(RV530_FG_ZBREG_DEST, RV530_FG_ZBREG_DEST_PIPE_SELECT_0);
    OUT_CS_REG_SEQ(R300_ZB_ZPASS_ADDR, 1);
    OUT_CS_RELOC(r300->oqbo, query->offset, 0, RADEON_GEM_DOMAIN_GTT, 0);
    OUT_CS_REG(RV530_FG_ZBREG_DEST, RV530_FG_ZBREG_DEST_PIPE_SELECT_ALL);
    END_CS;
}

static void rv530_emit_query_double(struct r300_context *r300,
                                    struct r300_query *query)
{
    CS_LOCALS(r300);

    BEGIN_CS(14);
    OUT_CS_REG(RV530_FG_ZBREG_DEST, RV530_FG_ZBREG_DEST_PIPE_SELECT_0);
    OUT_CS_REG_SEQ(R300_ZB_ZPASS_ADDR, 1);
    OUT_CS_RELOC(r300->oqbo, query->offset, 0, RADEON_GEM_DOMAIN_GTT, 0);
    OUT_CS_REG(RV530_FG_ZBREG_DEST, RV530_FG_ZBREG_DEST_PIPE_SELECT_1);
    OUT_CS_REG_SEQ(R300_ZB_ZPASS_ADDR, 1);
    OUT_CS_RELOC(r300->oqbo, query->offset + sizeof(uint32_t), 0, RADEON_GEM_DOMAIN_GTT, 0);
    OUT_CS_REG(RV530_FG_ZBREG_DEST, RV530_FG_ZBREG_DEST_PIPE_SELECT_ALL);
    END_CS;
}

void r300_emit_query_end(struct r300_context* r300)
{
    struct r300_capabilities *caps = r300_screen(r300->context.screen)->caps;
    struct r300_query *query = r300->query_current;

    if (!query)
	return;

    if (query->begin_emitted == FALSE)
        return;

    if (caps->family == CHIP_FAMILY_RV530) {
        if (caps->num_z_pipes == 2)
            rv530_emit_query_double(r300, query);
        else
            rv530_emit_query_single(r300, query);
    } else 
        r300_emit_query_finish(r300, query);
}

void r300_emit_rs_state(struct r300_context* r300, void* state)
{
    struct r300_rs_state* rs = (struct r300_rs_state*)state;
    float scale, offset;
    CS_LOCALS(r300);

    BEGIN_CS(20 + (rs->polygon_offset_enable ? 5 : 0));
    OUT_CS_REG(R300_VAP_CNTL_STATUS, rs->vap_control_status);

    OUT_CS_REG(R300_GB_AA_CONFIG, rs->antialiasing_config);

    OUT_CS_REG(R300_GA_POINT_SIZE, rs->point_size);
    OUT_CS_REG_SEQ(R300_GA_POINT_MINMAX, 2);
    OUT_CS(rs->point_minmax);
    OUT_CS(rs->line_control);

    if (rs->polygon_offset_enable) {
        scale = rs->depth_scale * 12;
        offset = rs->depth_offset;

        switch (r300->zbuffer_bpp) {
            case 16:
                offset *= 4;
                break;
            case 24:
                offset *= 2;
                break;
        }

        OUT_CS_REG_SEQ(R300_SU_POLY_OFFSET_FRONT_SCALE, 4);
        OUT_CS_32F(scale);
        OUT_CS_32F(offset);
        OUT_CS_32F(scale);
        OUT_CS_32F(offset);
    }

    OUT_CS_REG_SEQ(R300_SU_POLY_OFFSET_ENABLE, 2);
    OUT_CS(rs->polygon_offset_enable);
    OUT_CS(rs->cull_mode);
    OUT_CS_REG(R300_GA_LINE_STIPPLE_CONFIG, rs->line_stipple_config);
    OUT_CS_REG(R300_GA_LINE_STIPPLE_VALUE, rs->line_stipple_value);
    OUT_CS_REG(R300_GA_COLOR_CONTROL, rs->color_control);
    OUT_CS_REG(R300_GA_POLY_MODE, rs->polygon_mode);
    END_CS;
}

void r300_emit_rs_block_state(struct r300_context* r300,
                              struct r300_rs_block* rs)
{
    int i;
    struct r300_screen* r300screen = r300_screen(r300->context.screen);
    CS_LOCALS(r300);

    DBG(r300, DBG_DRAW, "r300: RS emit:\n");

    BEGIN_CS(21);
    if (r300screen->caps->is_r500) {
        OUT_CS_REG_SEQ(R500_RS_IP_0, 8);
    } else {
        OUT_CS_REG_SEQ(R300_RS_IP_0, 8);
    }
    for (i = 0; i < 8; i++) {
        OUT_CS(rs->ip[i]);
        DBG(r300, DBG_DRAW, "    : ip %d: 0x%08x\n", i, rs->ip[i]);
    }

    OUT_CS_REG_SEQ(R300_RS_COUNT, 2);
    OUT_CS(rs->count);
    OUT_CS(rs->inst_count);

    if (r300screen->caps->is_r500) {
        OUT_CS_REG_SEQ(R500_RS_INST_0, 8);
    } else {
        OUT_CS_REG_SEQ(R300_RS_INST_0, 8);
    }
    for (i = 0; i < 8; i++) {
        OUT_CS(rs->inst[i]);
        DBG(r300, DBG_DRAW, "    : inst %d: 0x%08x\n", i, rs->inst[i]);
    }

    DBG(r300, DBG_DRAW, "    : count: 0x%08x inst_count: 0x%08x\n",
        rs->count, rs->inst_count);

    END_CS;
}

void r300_emit_scissor_state(struct r300_context* r300, void* state)
{
    unsigned minx, miny, maxx, maxy;
    uint32_t top_left, bottom_right;
    struct r300_screen* r300screen = r300_screen(r300->context.screen);
    struct pipe_scissor_state* scissor = (struct pipe_scissor_state*)state;
    CS_LOCALS(r300);

    minx = miny = 0;
    maxx = r300->framebuffer_state.width;
    maxy = r300->framebuffer_state.height;

    if (((struct r300_rs_state*)r300->rs_state.state)->rs.scissor) {
        minx = MAX2(minx, scissor->minx);
        miny = MAX2(miny, scissor->miny);
        maxx = MIN2(maxx, scissor->maxx);
        maxy = MIN2(maxy, scissor->maxy);
    }

    if (r300screen->caps->is_r500) {
        top_left =
            (minx << R300_SCISSORS_X_SHIFT) |
            (miny << R300_SCISSORS_Y_SHIFT);
        bottom_right =
            ((maxx - 1) << R300_SCISSORS_X_SHIFT) |
            ((maxy - 1) << R300_SCISSORS_Y_SHIFT);
    } else {
        /* Offset of 1440 in non-R500 chipsets. */
        top_left =
            ((minx + 1440) << R300_SCISSORS_X_SHIFT) |
            ((miny + 1440) << R300_SCISSORS_Y_SHIFT);
        bottom_right =
            (((maxx - 1) + 1440) << R300_SCISSORS_X_SHIFT) |
            (((maxy - 1) + 1440) << R300_SCISSORS_Y_SHIFT);
    }

    BEGIN_CS(3);
    OUT_CS_REG_SEQ(R300_SC_SCISSORS_TL, 2);
    OUT_CS(top_left);
    OUT_CS(bottom_right);
    END_CS;
}

void r300_emit_texture(struct r300_context* r300,
                       struct r300_sampler_state* sampler,
                       struct r300_texture* tex,
                       unsigned offset)
{
    uint32_t filter0 = sampler->filter0;
    uint32_t format0 = tex->state.format0;
    unsigned min_level, max_level;
    CS_LOCALS(r300);

    /* to emulate 1D textures through 2D ones correctly */
    if (tex->tex.target == PIPE_TEXTURE_1D) {
        filter0 &= ~R300_TX_WRAP_T_MASK;
        filter0 |= R300_TX_WRAP_T(R300_TX_CLAMP_TO_EDGE);
    }

    if (tex->is_npot) {
        /* NPOT textures don't support mip filter, unfortunately.
         * This prevents incorrect rendering. */
        filter0 &= ~R300_TX_MIN_FILTER_MIP_MASK;
    } else {
        /* determine min/max levels */
        /* the MAX_MIP level is the largest (finest) one */
        max_level = MIN2(sampler->max_lod, tex->tex.last_level);
        min_level = MIN2(sampler->min_lod, max_level);
        format0 |= R300_TX_NUM_LEVELS(max_level);
        filter0 |= R300_TX_MAX_MIP_LEVEL(min_level);
    }

    BEGIN_CS(16);
    OUT_CS_REG(R300_TX_FILTER0_0 + (offset * 4), filter0 |
        (offset << 28));
    OUT_CS_REG(R300_TX_FILTER1_0 + (offset * 4), sampler->filter1);
    OUT_CS_REG(R300_TX_BORDER_COLOR_0 + (offset * 4), sampler->border_color);

    OUT_CS_REG(R300_TX_FORMAT0_0 + (offset * 4), format0);
    OUT_CS_REG(R300_TX_FORMAT1_0 + (offset * 4), tex->state.format1);
    OUT_CS_REG(R300_TX_FORMAT2_0 + (offset * 4), tex->state.format2);
    OUT_CS_REG_SEQ(R300_TX_OFFSET_0 + (offset * 4), 1);
    OUT_CS_RELOC(tex->buffer,
                 R300_TXO_MACRO_TILE(tex->macrotile) |
                 R300_TXO_MICRO_TILE(tex->microtile),
                 RADEON_GEM_DOMAIN_GTT | RADEON_GEM_DOMAIN_VRAM, 0, 0);
    END_CS;
}

static boolean r300_validate_aos(struct r300_context *r300)
{
    struct pipe_vertex_buffer *vbuf = r300->vertex_buffer;
    struct pipe_vertex_element *velem = r300->vertex_element;
    int i;

    /* Check if formats and strides are aligned to the size of DWORD. */
    for (i = 0; i < r300->vertex_element_count; i++) {
        if (vbuf[velem[i].vertex_buffer_index].stride % 4 != 0 ||
            util_format_get_blocksize(velem[i].src_format) % 4 != 0) {
            return FALSE;
        }
    }
    return TRUE;
}

void r300_emit_aos(struct r300_context* r300, unsigned offset)
{
    struct pipe_vertex_buffer *vb1, *vb2, *vbuf = r300->vertex_buffer;
    struct pipe_vertex_element *velem = r300->vertex_element;
    int i;
    unsigned size1, size2, aos_count = r300->vertex_element_count;
    unsigned packet_size = (aos_count * 3 + 1) / 2;
    CS_LOCALS(r300);

    /* XXX Move this checking to a more approriate place. */
    if (!r300_validate_aos(r300)) {
        /* XXX We should fallback using Draw. */
        assert(0);
    }

    BEGIN_CS(2 + packet_size + aos_count * 2);
    OUT_CS_PKT3(R300_PACKET3_3D_LOAD_VBPNTR, packet_size);
    OUT_CS(aos_count);

    for (i = 0; i < aos_count - 1; i += 2) {
        vb1 = &vbuf[velem[i].vertex_buffer_index];
        vb2 = &vbuf[velem[i+1].vertex_buffer_index];
        size1 = util_format_get_blocksize(velem[i].src_format);
        size2 = util_format_get_blocksize(velem[i+1].src_format);

        OUT_CS(R300_VBPNTR_SIZE0(size1) | R300_VBPNTR_STRIDE0(vb1->stride) |
               R300_VBPNTR_SIZE1(size2) | R300_VBPNTR_STRIDE1(vb2->stride));
        OUT_CS(vb1->buffer_offset + velem[i].src_offset   + offset * vb1->stride);
        OUT_CS(vb2->buffer_offset + velem[i+1].src_offset + offset * vb2->stride);
    }

    if (aos_count & 1) {
        vb1 = &vbuf[velem[i].vertex_buffer_index];
        size1 = util_format_get_blocksize(velem[i].src_format);

        OUT_CS(R300_VBPNTR_SIZE0(size1) | R300_VBPNTR_STRIDE0(vb1->stride));
        OUT_CS(vb1->buffer_offset + velem[i].src_offset + offset * vb1->stride);
    }

    for (i = 0; i < aos_count; i++) {
        OUT_CS_RELOC_NO_OFFSET(vbuf[velem[i].vertex_buffer_index].buffer,
                               RADEON_GEM_DOMAIN_GTT, 0, 0);
    }
    END_CS;
}

void r300_emit_vertex_format_state(struct r300_context* r300)
{
    int i;
    CS_LOCALS(r300);

    DBG(r300, DBG_DRAW, "r300: VAP/PSC emit:\n");

    BEGIN_CS(26);
    OUT_CS_REG(R300_VAP_VTX_SIZE, r300->vertex_info->vinfo.size);

    OUT_CS_REG_SEQ(R300_VAP_VTX_STATE_CNTL, 2);
    OUT_CS(r300->vertex_info->vinfo.hwfmt[0]);
    OUT_CS(r300->vertex_info->vinfo.hwfmt[1]);
    OUT_CS_REG_SEQ(R300_VAP_OUTPUT_VTX_FMT_0, 2);
    OUT_CS(r300->vertex_info->vinfo.hwfmt[2]);
    OUT_CS(r300->vertex_info->vinfo.hwfmt[3]);
    for (i = 0; i < 4; i++) {
       DBG(r300, DBG_DRAW, "    : hwfmt%d: 0x%08x\n", i,
               r300->vertex_info->vinfo.hwfmt[i]);
    }

    OUT_CS_REG_SEQ(R300_VAP_PROG_STREAM_CNTL_0, 8);
    for (i = 0; i < 8; i++) {
        OUT_CS(r300->vertex_info->vap_prog_stream_cntl[i]);
        DBG(r300, DBG_DRAW, "    : prog_stream_cntl%d: 0x%08x\n", i,
               r300->vertex_info->vap_prog_stream_cntl[i]);
    }
    OUT_CS_REG_SEQ(R300_VAP_PROG_STREAM_CNTL_EXT_0, 8);
    for (i = 0; i < 8; i++) {
        OUT_CS(r300->vertex_info->vap_prog_stream_cntl_ext[i]);
        DBG(r300, DBG_DRAW, "    : prog_stream_cntl_ext%d: 0x%08x\n", i,
               r300->vertex_info->vap_prog_stream_cntl_ext[i]);
    }
    END_CS;
}


void r300_emit_vertex_program_code(struct r300_context* r300,
                                   struct r300_vertex_program_code* code)
{
    int i;
    struct r300_screen* r300screen = r300_screen(r300->context.screen);
    unsigned instruction_count = code->length / 4;

    int vtx_mem_size = r300screen->caps->is_r500 ? 128 : 72;
    int input_count = MAX2(util_bitcount(code->InputsRead), 1);
    int output_count = MAX2(util_bitcount(code->OutputsWritten), 1);
    int temp_count = MAX2(code->num_temporaries, 1);
    int pvs_num_slots = MIN3(vtx_mem_size / input_count,
                             vtx_mem_size / output_count, 10);
    int pvs_num_controllers = MIN2(vtx_mem_size / temp_count, 6);

    CS_LOCALS(r300);

    if (!r300screen->caps->has_tcl) {
        debug_printf("r300: Implementation error: emit_vertex_shader called,"
                " but has_tcl is FALSE!\n");
        return;
    }

    BEGIN_CS(9 + code->length);
    /* R300_VAP_PVS_CODE_CNTL_0
     * R300_VAP_PVS_CONST_CNTL
     * R300_VAP_PVS_CODE_CNTL_1
     * See the r5xx docs for instructions on how to use these. */
    OUT_CS_REG_SEQ(R300_VAP_PVS_CODE_CNTL_0, 3);
    OUT_CS(R300_PVS_FIRST_INST(0) |
            R300_PVS_XYZW_VALID_INST(instruction_count - 1) |
            R300_PVS_LAST_INST(instruction_count - 1));
    OUT_CS(R300_PVS_MAX_CONST_ADDR(code->constants.Count - 1));
    OUT_CS(instruction_count - 1);

    OUT_CS_REG(R300_VAP_PVS_VECTOR_INDX_REG, 0);
    OUT_CS_ONE_REG(R300_VAP_PVS_UPLOAD_DATA, code->length);
    for (i = 0; i < code->length; i++)
        OUT_CS(code->body.d[i]);

    OUT_CS_REG(R300_VAP_CNTL, R300_PVS_NUM_SLOTS(pvs_num_slots) |
            R300_PVS_NUM_CNTLRS(pvs_num_controllers) |
            R300_PVS_NUM_FPUS(r300screen->caps->num_vert_fpus) |
            R300_PVS_VF_MAX_VTX_NUM(12) |
            (r300screen->caps->is_r500 ? R500_TCL_STATE_OPTIMIZATION : 0));
    END_CS;
}

void r300_emit_vertex_shader(struct r300_context* r300,
                             struct r300_vertex_shader* vs)
{
    r300_emit_vertex_program_code(r300, &vs->code);
}

void r300_emit_vs_constant_buffer(struct r300_context* r300,
                                  struct rc_constant_list* constants)
{
    int i;
    struct r300_screen* r300screen = r300_screen(r300->context.screen);
    CS_LOCALS(r300);

    if (!r300screen->caps->has_tcl) {
        debug_printf("r300: Implementation error: emit_vertex_shader called,"
        " but has_tcl is FALSE!\n");
        return;
    }

    if (constants->Count == 0)
        return;

    BEGIN_CS(constants->Count * 4 + 3);
    OUT_CS_REG(R300_VAP_PVS_VECTOR_INDX_REG,
               (r300screen->caps->is_r500 ?
               R500_PVS_CONST_START : R300_PVS_CONST_START));
    OUT_CS_ONE_REG(R300_VAP_PVS_UPLOAD_DATA, constants->Count * 4);
    for (i = 0; i < constants->Count; i++) {
        const float * data = get_shader_constant(r300,
                                                 &constants->Constants[i],
                                                 &r300->shader_constants[PIPE_SHADER_VERTEX]);
        OUT_CS_32F(data[0]);
        OUT_CS_32F(data[1]);
        OUT_CS_32F(data[2]);
        OUT_CS_32F(data[3]);
    }
    END_CS;
}

void r300_emit_viewport_state(struct r300_context* r300, void* state)
{
    struct r300_viewport_state* viewport = (struct r300_viewport_state*)state;
    CS_LOCALS(r300);

    if (r300->tcl_bypass) {
        BEGIN_CS(2);
        OUT_CS_REG(R300_VAP_VTE_CNTL, 0);
        END_CS;
    } else {
        BEGIN_CS(9);
        OUT_CS_REG_SEQ(R300_SE_VPORT_XSCALE, 6);
        OUT_CS_32F(viewport->xscale);
        OUT_CS_32F(viewport->xoffset);
        OUT_CS_32F(viewport->yscale);
        OUT_CS_32F(viewport->yoffset);
        OUT_CS_32F(viewport->zscale);
        OUT_CS_32F(viewport->zoffset);
        OUT_CS_REG(R300_VAP_VTE_CNTL, viewport->vte_control);
        END_CS;
    }
}

void r300_emit_texture_count(struct r300_context* r300)
{
    uint32_t tx_enable = 0;
    int i;
    CS_LOCALS(r300);

    /* Notice that texture_count and sampler_count are just sizes
     * of the respective arrays. We still have to check for the individual
     * elements. */
    for (i = 0; i < MIN2(r300->sampler_count, r300->texture_count); i++) {
        if (r300->textures[i]) {
            tx_enable |= 1 << i;
        }
    }

    BEGIN_CS(2);
    OUT_CS_REG(R300_TX_ENABLE, tx_enable);
    END_CS;

}

void r300_emit_ztop_state(struct r300_context* r300, void* state)
{
    struct r300_ztop_state* ztop = (struct r300_ztop_state*)state;
    CS_LOCALS(r300);

    BEGIN_CS(2);
    OUT_CS_REG(R300_ZB_ZTOP, ztop->z_buffer_top);
    END_CS;
}

void r300_flush_textures(struct r300_context* r300)
{
    CS_LOCALS(r300);

    BEGIN_CS(2);
    OUT_CS_REG(R300_TX_INVALTAGS, 0);
    END_CS;
}

static void r300_flush_pvs(struct r300_context* r300)
{
    CS_LOCALS(r300);

    BEGIN_CS(2);
    OUT_CS_REG(R300_VAP_PVS_STATE_FLUSH_REG, 0x0);
    END_CS;
}

void r300_emit_buffer_validate(struct r300_context *r300)
{
    struct r300_texture* tex;
    unsigned i;
    boolean invalid = FALSE;

    /* Clean out BOs. */
    r300->winsys->reset_bos(r300->winsys);

validate:
    /* Color buffers... */
    for (i = 0; i < r300->framebuffer_state.nr_cbufs; i++) {
        tex = (struct r300_texture*)r300->framebuffer_state.cbufs[i]->texture;
        assert(tex && tex->buffer && "cbuf is marked, but NULL!");
        if (!r300->winsys->add_buffer(r300->winsys, tex->buffer,
                    0, RADEON_GEM_DOMAIN_VRAM)) {
            r300->context.flush(&r300->context, 0, NULL);
            goto validate;
        }
    }
    /* ...depth buffer... */
    if (r300->framebuffer_state.zsbuf) {
        tex = (struct r300_texture*)r300->framebuffer_state.zsbuf->texture;
        assert(tex && tex->buffer && "zsbuf is marked, but NULL!");
        if (!r300->winsys->add_buffer(r300->winsys, tex->buffer,
                    0, RADEON_GEM_DOMAIN_VRAM)) {
            r300->context.flush(&r300->context, 0, NULL);
            goto validate;
        }
    }
    /* ...textures... */
    for (i = 0; i < r300->texture_count; i++) {
        tex = r300->textures[i];
        if (!tex)
            continue;
        if (!r300->winsys->add_buffer(r300->winsys, tex->buffer,
                    RADEON_GEM_DOMAIN_GTT | RADEON_GEM_DOMAIN_VRAM, 0)) {
            r300->context.flush(&r300->context, 0, NULL);
            goto validate;
        }
    }
    /* ...occlusion query buffer... */
    if (r300->dirty_state & R300_NEW_QUERY) {
        if (!r300->winsys->add_buffer(r300->winsys, r300->oqbo,
                    0, RADEON_GEM_DOMAIN_GTT)) {
            r300->context.flush(&r300->context, 0, NULL);
            goto validate;
        }
    }
    /* ...and vertex buffer. */
    if (r300->vbo) {
        if (!r300->winsys->add_buffer(r300->winsys, r300->vbo,
                    RADEON_GEM_DOMAIN_GTT, 0)) {
            r300->context.flush(&r300->context, 0, NULL);
            goto validate;
        }
    } else {
        /* debug_printf("No VBO while emitting dirty state!\n"); */
    }
    if (!r300->winsys->validate(r300->winsys)) {
        r300->context.flush(&r300->context, 0, NULL);
        if (invalid) {
            /* Well, hell. */
            debug_printf("r300: Stuck in validation loop, gonna quit now.");
            exit(1);
        }
        invalid = TRUE;
        goto validate;
    }
}

/* Emit all dirty state. */
void r300_emit_dirty_state(struct r300_context* r300)
{
    struct r300_screen* r300screen = r300_screen(r300->context.screen);
    struct r300_atom* atom;
    unsigned i, dwords = 1024;
    int dirty_tex = 0;

    /* Check the required number of dwords against the space remaining in the
     * current CS object. If we need more, then flush. */

    foreach(atom, &r300->atom_list) {
        if (atom->dirty || atom->always_dirty) {
            dwords += atom->size;
        }
    }

    /* Make sure we have at least 2*1024 spare dwords. */
    /* XXX It would be nice to know the number of dwords we really need to
     * XXX emit. */
again:
    if (!r300->winsys->check_cs(r300->winsys, dwords)) {
        r300->context.flush(&r300->context, 0, NULL);
	goto again;
    }

    if (r300->dirty_state & R300_NEW_QUERY) {
        r300_emit_query_start(r300);
        r300->dirty_state &= ~R300_NEW_QUERY;
    }

    foreach(atom, &r300->atom_list) {
        if (atom->dirty || atom->always_dirty) {
            atom->emit(r300, atom->state);
            atom->dirty = FALSE;
        }
    }

    if (r300->dirty_state & R300_NEW_FRAGMENT_SHADER) {
        r300_emit_fragment_depth_config(r300, r300->fs);
        if (r300screen->caps->is_r500) {
            r500_emit_fragment_program_code(r300, &r300->fs->shader->code);
        } else {
            r300_emit_fragment_program_code(r300, &r300->fs->shader->code);
        }
        r300->dirty_state &= ~R300_NEW_FRAGMENT_SHADER;
    }

    if (r300->dirty_state & R300_NEW_FRAGMENT_SHADER_CONSTANTS) {
        if (r300screen->caps->is_r500) {
            r500_emit_fs_constant_buffer(r300,
                                         &r300->fs->shader->code.constants);
        } else {
            r300_emit_fs_constant_buffer(r300,
                                         &r300->fs->shader->code.constants);
        }
        r300->dirty_state &= ~R300_NEW_FRAGMENT_SHADER_CONSTANTS;
    }

    if (r300->dirty_state & R300_NEW_FRAMEBUFFERS) {
        r300_emit_fb_state(r300, &r300->framebuffer_state);
        r300->dirty_state &= ~R300_NEW_FRAMEBUFFERS;
    }

    if (r300->dirty_state & R300_NEW_RS_BLOCK) {
        r300_emit_rs_block_state(r300, r300->rs_block);
        r300->dirty_state &= ~R300_NEW_RS_BLOCK;
    }

    /* Samplers and textures are tracked separately but emitted together. */
    if (r300->dirty_state &
            (R300_ANY_NEW_SAMPLERS | R300_ANY_NEW_TEXTURES)) {
        r300_emit_texture_count(r300);

        for (i = 0; i < MIN2(r300->sampler_count, r300->texture_count); i++) {
  	    if (r300->dirty_state &
		((R300_NEW_SAMPLER << i) | (R300_NEW_TEXTURE << i))) {
		if (r300->textures[i]) 
		    r300_emit_texture(r300,
				      r300->sampler_states[i],
				      r300->textures[i],
				      i);
                r300->dirty_state &=
                    ~((R300_NEW_SAMPLER << i) | (R300_NEW_TEXTURE << i));
                dirty_tex++;
            }
        }
        r300->dirty_state &= ~(R300_ANY_NEW_SAMPLERS | R300_ANY_NEW_TEXTURES);
    }

    if (dirty_tex) {
        r300_flush_textures(r300);
    }

    if (r300->dirty_state & R300_NEW_VERTEX_FORMAT) {
        r300_emit_vertex_format_state(r300);
        r300->dirty_state &= ~R300_NEW_VERTEX_FORMAT;
    }

    if (r300->dirty_state & (R300_NEW_VERTEX_SHADER | R300_NEW_VERTEX_SHADER_CONSTANTS)) {
        r300_flush_pvs(r300);
    }

    if (r300->dirty_state & R300_NEW_VERTEX_SHADER) {
        r300_emit_vertex_shader(r300, r300->vs);
        r300->dirty_state &= ~R300_NEW_VERTEX_SHADER;
    }

    if (r300->dirty_state & R300_NEW_VERTEX_SHADER_CONSTANTS) {
        r300_emit_vs_constant_buffer(r300, &r300->vs->code.constants);
        r300->dirty_state &= ~R300_NEW_VERTEX_SHADER_CONSTANTS;
    }

    /* XXX
    assert(r300->dirty_state == 0);
    */

    /* Finally, emit the VBO. */
    /* r300_emit_vertex_buffer(r300); */

    r300->dirty_hw++;
}
