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

/* r300_emit: Functions for emitting state. */

#include "r300_emit.h"

void r300_emit_blend_state(struct r300_context* r300,
                           struct r300_blend_state* blend)
{
    CS_LOCALS(r300);
    BEGIN_CS(7);
    OUT_CS_REG_SEQ(R300_RB3D_CBLEND, 2);
    OUT_CS(blend->blend_control);
    OUT_CS(blend->alpha_blend_control);
    OUT_CS_REG(R300_RB3D_ROPCNTL, blend->rop);
    OUT_CS_REG(R300_RB3D_DITHER_CTL, blend->dither);
    END_CS;
}

void r300_emit_blend_color_state(struct r300_context* r300,
                                 struct r300_blend_color_state* bc)
{
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

void r300_emit_dsa_state(struct r300_context* r300,
                           struct r300_dsa_state* dsa)
{
    struct r300_screen* r300screen = r300_screen(r300->context.screen);
    CS_LOCALS(r300);

    BEGIN_CS(r300screen->caps->is_r500 ? 8 : 8);
    OUT_CS_REG(R300_FG_ALPHA_FUNC, dsa->alpha_function);
    /* XXX figure out the r300 counterpart for this */
    if (r300screen->caps->is_r500) {
        /* OUT_CS_REG(R500_FG_ALPHA_VALUE, dsa->alpha_reference); */
    }
    OUT_CS_REG_SEQ(R300_ZB_CNTL, 3);
    OUT_CS(dsa->z_buffer_control);
    OUT_CS(dsa->z_stencil_control);
    OUT_CS(dsa->stencil_ref_mask);
    OUT_CS_REG(R300_ZB_ZTOP, dsa->z_buffer_top);
    if (r300screen->caps->is_r500) {
        /* OUT_CS_REG(R500_ZB_STENCILREFMASK_BF, dsa->stencil_ref_bf); */
    }
    END_CS;
}

void r300_emit_fragment_shader(struct r300_context* r300,
                               struct r300_fragment_shader* fs)
{
    int i;
    CS_LOCALS(r300);

    BEGIN_CS(22);

    OUT_CS_REG(R300_US_CONFIG, fs->indirections);
    OUT_CS_REG(R300_US_PIXSIZE, fs->shader.stack_size);
    /* XXX figure out exactly how big the sizes are on this reg */
    OUT_CS_REG(R300_US_CODE_OFFSET, 0x40);
    /* XXX figure these ones out a bit better kthnx */
    OUT_CS_REG(R300_US_CODE_ADDR_0, 0x0);
    OUT_CS_REG(R300_US_CODE_ADDR_1, 0x0);
    OUT_CS_REG(R300_US_CODE_ADDR_2, 0x0);
    OUT_CS_REG(R300_US_CODE_ADDR_3, 0x40 | R300_RGBA_OUT);

    for (i = 0; i < fs->alu_instruction_count; i++) {
        OUT_CS_REG(R300_US_ALU_RGB_INST_0 + (4 * i),
            fs->instructions[i].alu_rgb_inst);
        OUT_CS_REG(R300_US_ALU_RGB_ADDR_0 + (4 * i),
            fs->instructions[i].alu_rgb_addr);
        OUT_CS_REG(R300_US_ALU_ALPHA_INST_0 + (4 * i),
            fs->instructions[i].alu_alpha_inst);
        OUT_CS_REG(R300_US_ALU_ALPHA_ADDR_0 + (4 * i),
            fs->instructions[i].alu_alpha_addr);
    }

    END_CS;
}

void r500_emit_fragment_shader(struct r300_context* r300,
                               struct r500_fragment_shader* fs)
{
    int i;
    struct r300_constant_buffer* constants =
        &r300->shader_constants[PIPE_SHADER_FRAGMENT];
    CS_LOCALS(r300);

    BEGIN_CS(9 + (fs->instruction_count * 6) + (constants->count ? 3 : 0) +
            (constants->count * 4));
    OUT_CS_REG(R500_US_CONFIG, R500_ZERO_TIMES_ANYTHING_EQUALS_ZERO);
    OUT_CS_REG(R500_US_PIXSIZE, fs->shader.stack_size);
    OUT_CS_REG(R500_US_CODE_ADDR, R500_US_CODE_START_ADDR(0) |
            R500_US_CODE_END_ADDR(fs->instruction_count));

    OUT_CS_REG(R500_GA_US_VECTOR_INDEX, R500_GA_US_VECTOR_INDEX_TYPE_INSTR);
    OUT_CS_ONE_REG(R500_GA_US_VECTOR_DATA, fs->instruction_count * 6);
    for (i = 0; i < fs->instruction_count; i++) {
        OUT_CS(fs->instructions[i].inst0);
        OUT_CS(fs->instructions[i].inst1);
        OUT_CS(fs->instructions[i].inst2);
        OUT_CS(fs->instructions[i].inst3);
        OUT_CS(fs->instructions[i].inst4);
        OUT_CS(fs->instructions[i].inst5);
    }

    if (constants->count) {
        OUT_CS_REG(R500_GA_US_VECTOR_INDEX,
                R500_GA_US_VECTOR_INDEX_TYPE_CONST);
        OUT_CS_ONE_REG(R500_GA_US_VECTOR_DATA, constants->count * 4);
        for (i = 0; i < constants->count; i++) {
            OUT_CS_32F(constants->constants[i][0]);
            OUT_CS_32F(constants->constants[i][1]);
            OUT_CS_32F(constants->constants[i][2]);
            OUT_CS_32F(constants->constants[i][3]);
        }
    }

    END_CS;
}

void r300_emit_fb_state(struct r300_context* r300,
                        struct pipe_framebuffer_state* fb)
{
    struct r300_texture* tex;
    unsigned pixpitch;
    int i;
    CS_LOCALS(r300);

    BEGIN_CS((8 * fb->nr_cbufs) + (fb->zsbuf ? 8 : 0) + 4);
    for (i = 0; i < fb->nr_cbufs; i++) {
        tex = (struct r300_texture*)fb->cbufs[i]->texture;
        pixpitch = tex->stride / tex->tex.block.size;

        OUT_CS_REG_SEQ(R300_RB3D_COLOROFFSET0 + (4 * i), 1);
        OUT_CS_RELOC(tex->buffer, 0, 0, RADEON_GEM_DOMAIN_VRAM, 0);

        OUT_CS_REG(R300_RB3D_COLORPITCH0 + (4 * i), pixpitch |
            r300_translate_colorformat(tex->tex.format));

        OUT_CS_REG(R300_US_OUT_FMT_0 + (4 * i),
            r300_translate_out_fmt(fb->cbufs[i]->format));
    }

    if (fb->zsbuf) {
        tex = (struct r300_texture*)fb->zsbuf->texture;
        pixpitch = (tex->stride / tex->tex.block.size);

        OUT_CS_REG_SEQ(R300_ZB_DEPTHOFFSET, 1);
        OUT_CS_RELOC(tex->buffer, 0, 0, RADEON_GEM_DOMAIN_VRAM, 0);

        OUT_CS_REG(R300_ZB_FORMAT, r300_translate_zsformat(tex->tex.format));

        OUT_CS_REG(R300_ZB_DEPTHPITCH, pixpitch);
    }

    OUT_CS_REG(R300_RB3D_DSTCACHE_CTLSTAT,
        R300_RB3D_DSTCACHE_CTLSTAT_DC_FREE_FREE_3D_TAGS |
        R300_RB3D_DSTCACHE_CTLSTAT_DC_FLUSH_FLUSH_DIRTY_3D);
    OUT_CS_REG(R300_ZB_ZCACHE_CTLSTAT,
        R300_ZB_ZCACHE_CTLSTAT_ZC_FLUSH_FLUSH_AND_FREE |
        R300_ZB_ZCACHE_CTLSTAT_ZC_FREE_FREE);
    END_CS;
}

void r300_emit_rs_state(struct r300_context* r300, struct r300_rs_state* rs)
{
    CS_LOCALS(r300);

    BEGIN_CS(20);
    OUT_CS_REG(R300_VAP_CNTL_STATUS, rs->vap_control_status);
    OUT_CS_REG(R300_GA_POINT_SIZE, rs->point_size);
    OUT_CS_REG_SEQ(R300_GA_POINT_MINMAX, 2);
    OUT_CS(rs->point_minmax);
    OUT_CS(rs->line_control);
    OUT_CS_REG_SEQ(R300_SU_POLY_OFFSET_FRONT_SCALE, 6);
    OUT_CS(rs->depth_scale_front);
    OUT_CS(rs->depth_offset_front);
    OUT_CS(rs->depth_scale_back);
    OUT_CS(rs->depth_offset_back);
    OUT_CS(rs->polygon_offset_enable);
    OUT_CS(rs->cull_mode);
    OUT_CS_REG(R300_GA_LINE_STIPPLE_CONFIG, rs->line_stipple_config);
    OUT_CS_REG(R300_GA_LINE_STIPPLE_VALUE, rs->line_stipple_value);
    OUT_CS_REG(R300_GA_COLOR_CONTROL, rs->color_control);
    END_CS;
}

void r300_emit_rs_block_state(struct r300_context* r300,
                              struct r300_rs_block* rs)
{
    int i;
    struct r300_screen* r300screen = r300_screen(r300->context.screen);
    CS_LOCALS(r300);

    BEGIN_CS(21);
    if (r300screen->caps->is_r500) {
        OUT_CS_REG_SEQ(R500_RS_IP_0, 8);
    } else {
        OUT_CS_REG_SEQ(R300_RS_IP_0, 8);
    }
    for (i = 0; i < 8; i++) {
        OUT_CS(rs->ip[i]);
        debug_printf("ip %d: 0x%08x\n", i, rs->ip[i]);
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
        debug_printf("inst %d: 0x%08x\n", i, rs->inst[i]);
    }

    debug_printf("count: 0x%08x inst_count: 0x%08x\n", rs->count,
            rs->inst_count);

    END_CS;
}

void r300_emit_sampler(struct r300_context* r300,
                       struct r300_sampler_state* sampler, unsigned offset)
{
    CS_LOCALS(r300);

    BEGIN_CS(6);
    OUT_CS_REG(R300_TX_FILTER0_0 + (offset * 4), sampler->filter0);
    OUT_CS_REG(R300_TX_FILTER1_0 + (offset * 4), sampler->filter1);
    OUT_CS_REG(R300_TX_BORDER_COLOR_0 + (offset * 4), sampler->border_color);
    END_CS;
}

void r300_emit_scissor_state(struct r300_context* r300,
                             struct r300_scissor_state* scissor)
{
    CS_LOCALS(r300);

    BEGIN_CS(3);
    OUT_CS_REG_SEQ(R300_SC_SCISSORS_TL, 2);
    OUT_CS(scissor->scissor_top_left);
    OUT_CS(scissor->scissor_bottom_right);
    END_CS;
}

void r300_emit_texture(struct r300_context* r300,
                       struct r300_texture* tex, unsigned offset)
{
    CS_LOCALS(r300);

    BEGIN_CS(10);
    OUT_CS_REG(R300_TX_FORMAT0_0 + (offset * 4), tex->state.format0);
    OUT_CS_REG(R300_TX_FORMAT1_0 + (offset * 4), tex->state.format1);
    OUT_CS_REG(R300_TX_FORMAT2_0 + (offset * 4), tex->state.format2);
    OUT_CS_REG_SEQ(R300_TX_OFFSET_0 + (offset * 4), 1);
    OUT_CS_RELOC(tex->buffer, 0,
            RADEON_GEM_DOMAIN_GTT | RADEON_GEM_DOMAIN_VRAM, 0, 0);
    END_CS;
}

void r300_emit_vertex_format_state(struct r300_context* r300)
{
    int i;
    CS_LOCALS(r300);

    BEGIN_CS(26);
    OUT_CS_REG(R300_VAP_VTX_SIZE, r300->vertex_info.vinfo.size);

    OUT_CS_REG_SEQ(R300_VAP_VTX_STATE_CNTL, 2);
    OUT_CS(r300->vertex_info.vinfo.hwfmt[0]);
    OUT_CS(r300->vertex_info.vinfo.hwfmt[1]);
    OUT_CS_REG_SEQ(R300_VAP_OUTPUT_VTX_FMT_0, 2);
    OUT_CS(r300->vertex_info.vinfo.hwfmt[2]);
    OUT_CS(r300->vertex_info.vinfo.hwfmt[3]);
    for (i = 0; i < 4; i++) {
        debug_printf("hwfmt%d: 0x%08x\n", i,
                r300->vertex_info.vinfo.hwfmt[i]);
    }

    OUT_CS_REG_SEQ(R300_VAP_PROG_STREAM_CNTL_0, 8);
    for (i = 0; i < 8; i++) {
        OUT_CS(r300->vertex_info.vap_prog_stream_cntl[i]);
        debug_printf("prog_stream_cntl%d: 0x%08x\n", i,
                r300->vertex_info.vap_prog_stream_cntl[i]);
    }
    OUT_CS_REG_SEQ(R300_VAP_PROG_STREAM_CNTL_EXT_0, 8);
    for (i = 0; i < 8; i++) {
        OUT_CS(r300->vertex_info.vap_prog_stream_cntl_ext[i]);
        debug_printf("prog_stream_cntl_ext%d: 0x%08x\n", i,
                r300->vertex_info.vap_prog_stream_cntl_ext[i]);
    }
    END_CS;
}

void r300_emit_vertex_shader(struct r300_context* r300,
                             struct r300_vertex_shader* vs)
{
    int i;
    struct r300_screen* r300screen = r300_screen(r300->context.screen);
    struct r300_constant_buffer* constants =
        &r300->shader_constants[PIPE_SHADER_VERTEX];
    CS_LOCALS(r300);

    if (!r300screen->caps->has_tcl) {
        debug_printf("r300: Implementation error: emit_vertex_shader called,"
                " but has_tcl is FALSE!\n");
        return;
    }

    if (constants->count) {
        BEGIN_CS(16 + (vs->instruction_count * 4) + (constants->count * 4));
    } else {
        BEGIN_CS(13 + (vs->instruction_count * 4) + (constants->count * 4));
    }

    OUT_CS_REG(R300_VAP_PVS_CODE_CNTL_0, R300_PVS_FIRST_INST(0) |
            R300_PVS_LAST_INST(vs->instruction_count - 1));
    OUT_CS_REG(R300_VAP_PVS_CODE_CNTL_1, vs->instruction_count - 1);

    /* XXX */
    OUT_CS_REG(R300_VAP_PVS_CONST_CNTL, 0x0);

    OUT_CS_REG(R300_VAP_PVS_VECTOR_INDX_REG, 0);
    OUT_CS_ONE_REG(R300_VAP_PVS_UPLOAD_DATA, vs->instruction_count * 4);
    for (i = 0; i < vs->instruction_count; i++) {
        OUT_CS(vs->instructions[i].inst0);
        OUT_CS(vs->instructions[i].inst1);
        OUT_CS(vs->instructions[i].inst2);
        OUT_CS(vs->instructions[i].inst3);
    }

    if (constants->count) {
        OUT_CS_REG(R300_VAP_PVS_VECTOR_INDX_REG,
                (r300screen->caps->is_r500 ?
                 R500_PVS_CONST_START : R300_PVS_CONST_START));
        OUT_CS_ONE_REG(R300_VAP_PVS_UPLOAD_DATA, constants->count * 4);
        for (i = 0; i < constants->count; i++) {
            OUT_CS_32F(constants->constants[i][0]);
            OUT_CS_32F(constants->constants[i][1]);
            OUT_CS_32F(constants->constants[i][2]);
            OUT_CS_32F(constants->constants[i][3]);
        }
    }

    OUT_CS_REG(R300_VAP_CNTL, R300_PVS_NUM_SLOTS(10) |
            R300_PVS_NUM_CNTLRS(5) |
            R300_PVS_NUM_FPUS(r300screen->caps->num_vert_fpus) |
            R300_PVS_VF_MAX_VTX_NUM(12));
    OUT_CS_REG(R300_VAP_PVS_STATE_FLUSH_REG, 0x0);
    END_CS;

}

void r300_emit_viewport_state(struct r300_context* r300,
                              struct r300_viewport_state* viewport)
{
    CS_LOCALS(r300);

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

void r300_flush_textures(struct r300_context* r300)
{
    CS_LOCALS(r300);

    BEGIN_CS(4);
    OUT_CS_REG(R300_TX_INVALTAGS, 0);
    OUT_CS_REG(R300_TX_ENABLE, (1 << r300->texture_count) - 1);
    END_CS;
}

/* Emit all dirty state. */
void r300_emit_dirty_state(struct r300_context* r300)
{
    struct r300_screen* r300screen = r300_screen(r300->context.screen);
    int i;
    int dirty_tex = 0;

    if (!(r300->dirty_hw)) {
        return;
    }

    r300_update_derived_state(r300);

    /* XXX check size */
    struct r300_texture* fb_tex =
        (struct r300_texture*)r300->framebuffer_state.cbufs[0];
    r300->winsys->add_buffer(r300->winsys, fb_tex->buffer,
            0, RADEON_GEM_DOMAIN_VRAM);
    if (r300->winsys->validate(r300->winsys)) {
        /* XXX */
        r300->context.flush(&r300->context, 0, NULL);
    }

    if (r300->dirty_state & R300_NEW_BLEND) {
        r300_emit_blend_state(r300, r300->blend_state);
        r300->dirty_state &= ~R300_NEW_BLEND;
    }

    if (r300->dirty_state & R300_NEW_BLEND_COLOR) {
        r300_emit_blend_color_state(r300, r300->blend_color_state);
        r300->dirty_state &= ~R300_NEW_BLEND_COLOR;
    }

    if (r300->dirty_state & R300_NEW_DSA) {
        r300_emit_dsa_state(r300, r300->dsa_state);
        r300->dirty_state &= ~R300_NEW_DSA;
    }

    if (r300->dirty_state & R300_NEW_FRAGMENT_SHADER) {
        if (r300screen->caps->is_r500) {
            r500_emit_fragment_shader(r300,
                (struct r500_fragment_shader*)r300->fs);
        } else {
            r300_emit_fragment_shader(r300,
                (struct r300_fragment_shader*)r300->fs);
        }
        r300->dirty_state &= ~R300_NEW_FRAGMENT_SHADER;
    }

    if (r300->dirty_state & R300_NEW_FRAMEBUFFERS) {
        r300_emit_fb_state(r300, &r300->framebuffer_state);
        r300->dirty_state &= ~R300_NEW_FRAMEBUFFERS;
    }

    if (r300->dirty_state & R300_NEW_RASTERIZER) {
        r300_emit_rs_state(r300, r300->rs_state);
        r300->dirty_state &= ~R300_NEW_RASTERIZER;
    }

    if (r300->dirty_state & R300_NEW_RS_BLOCK) {
        r300_emit_rs_block_state(r300, r300->rs_block);
        r300->dirty_state &= ~R300_NEW_RS_BLOCK;
    }

    if (r300->dirty_state & R300_ANY_NEW_SAMPLERS) {
        for (i = 0; i < r300->sampler_count; i++) {
            if (r300->dirty_state & (R300_NEW_SAMPLER << i)) {
                r300_emit_sampler(r300, r300->sampler_states[i], i);
                r300->dirty_state &= ~(R300_NEW_SAMPLER << i);
                dirty_tex++;
            }
        }
    }

    if (r300->dirty_state & R300_NEW_SCISSOR) {
        r300_emit_scissor_state(r300, r300->scissor_state);
        r300->dirty_state &= ~R300_NEW_SCISSOR;
    }

    if (r300->dirty_state & R300_ANY_NEW_TEXTURES) {
        for (i = 0; i < r300->texture_count; i++) {
            if (r300->dirty_state & (R300_NEW_TEXTURE << i)) {
                r300_emit_texture(r300, r300->textures[i], i);
                r300->dirty_state &= ~(R300_NEW_TEXTURE << i);
                dirty_tex++;
            }
        }
    }

    if (r300->dirty_state & R300_NEW_VIEWPORT) {
        r300_emit_viewport_state(r300, r300->viewport_state);
        r300->dirty_state &= ~R300_NEW_VIEWPORT;
    }

    if (dirty_tex) {
        r300_flush_textures(r300);
    }

    if (r300->dirty_state & R300_NEW_VERTEX_FORMAT) {
        r300_emit_vertex_format_state(r300);
        r300->dirty_state &= ~R300_NEW_VERTEX_FORMAT;
    }
}
