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
    struct r300_screen* r300screen =
        (struct r300_screen*)r300->context.screen;
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
    struct r300_screen* r300screen =
        (struct r300_screen*)r300->context.screen;
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
    CS_LOCALS(r300);
    int i;
    BEGIN_CS(0);

    OUT_CS_REG(R300_US_CONFIG, MAX2(fs->indirections - 1, 0));
    OUT_CS_REG(R300_US_PIXSIZE, fs->shader.stack_size);
    /* XXX figure out exactly how big the sizes are on this reg */
    OUT_CS_REG(R300_US_CODE_OFFSET, 0x0);
    /* XXX figure these ones out a bit better kthnx */
    OUT_CS_REG(R300_US_CODE_ADDR_0, 0x0);
    OUT_CS_REG(R300_US_CODE_ADDR_1, 0x0);
    OUT_CS_REG(R300_US_CODE_ADDR_2, 0x0);
    OUT_CS_REG(R300_US_CODE_ADDR_3, R300_RGBA_OUT);

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
    CS_LOCALS(r300);
    int i = 0;
    BEGIN_CS(11 + (fs->instruction_count * 6));
    OUT_CS_REG(R500_US_CONFIG, R500_ZERO_TIMES_ANYTHING_EQUALS_ZERO);
    OUT_CS_REG(R500_US_PIXSIZE, fs->shader.stack_size);
    OUT_CS_REG(R500_US_CODE_ADDR, R500_US_CODE_START_ADDR(0) |
        R500_US_CODE_END_ADDR(fs->instruction_count));

    OUT_CS_REG(R500_GA_US_VECTOR_INDEX, R500_GA_US_VECTOR_INDEX_TYPE_INSTR);
    OUT_CS_ONE_REG(R500_GA_US_VECTOR_DATA,
        fs->instruction_count * 6);
    for (i = 0; i < fs->instruction_count; i++) {
        OUT_CS(fs->instructions[i].inst0);
        OUT_CS(fs->instructions[i].inst1);
        OUT_CS(fs->instructions[i].inst2);
        OUT_CS(fs->instructions[i].inst3);
        OUT_CS(fs->instructions[i].inst4);
        OUT_CS(fs->instructions[i].inst5);
    }
    R300_PACIFY;
    END_CS;
}

/* Translate pipe_format into US_OUT_FMT. Note that formats are stored from
 * C3 to C0. */
uint32_t translate_out_fmt(enum pipe_format format)
{
    switch (format) {
        case PIPE_FORMAT_A8R8G8B8_UNORM:
            return R300_US_OUT_FMT_C4_8 |
                R300_C0_SEL_B | R300_C1_SEL_G |
                R300_C2_SEL_R | R300_C3_SEL_A;
        default:
            return R300_US_OUT_FMT_UNUSED;
    }
    return 0;
}

/* XXX add pitch, stride, z/stencil buf */
void r300_emit_fb_state(struct r300_context* r300,
                        struct pipe_framebuffer_state* fb)
{
    CS_LOCALS(r300);
    struct r300_texture* tex;
    int i;

    BEGIN_CS((3 * fb->nr_cbufs) + 6);
    for (i = 0; i < fb->nr_cbufs; i++) {
        tex = (struct r300_texture*)fb->cbufs[i]->texture;
        OUT_CS_REG_SEQ(R300_RB3D_COLOROFFSET0 + (4 * i), 1);
        OUT_CS_RELOC(tex->buffer, 0, 0, RADEON_GEM_DOMAIN_VRAM, 0);

        OUT_CS_REG(R300_US_OUT_FMT_0 + (4 * i),
            translate_out_fmt(fb->cbufs[i]->format));
    }

    if (fb->zsbuf) {
        tex = (struct r300_texture*)fb->zsbuf->texture;
        OUT_CS_REG_SEQ(R300_ZB_DEPTHOFFSET, 1);
        OUT_CS_RELOC(tex->buffer, 0, 0, RADEON_GEM_DOMAIN_VRAM, 0);
    }

    OUT_CS_REG(R300_RB3D_DSTCACHE_CTLSTAT,
        R300_RB3D_DSTCACHE_CTLSTAT_DC_FREE_FREE_3D_TAGS |
        R300_RB3D_DSTCACHE_CTLSTAT_DC_FLUSH_FLUSH_DIRTY_3D);
    OUT_CS_REG(R300_ZB_ZCACHE_CTLSTAT,
        R300_ZB_ZCACHE_CTLSTAT_ZC_FLUSH_FLUSH_AND_FREE |
        R300_ZB_ZCACHE_CTLSTAT_ZC_FREE_FREE);
    R300_PACIFY;
    END_CS;
}

void r300_emit_rs_state(struct r300_context* r300, struct r300_rs_state* rs)
{
    struct r300_screen* r300screen =
        (struct r300_screen*)r300->context.screen;
    CS_LOCALS(r300);
    BEGIN_CS(14);
    OUT_CS_REG(R300_VAP_CNTL_STATUS, rs->vap_control_status);
    OUT_CS_REG_SEQ(R300_SU_POLY_OFFSET_FRONT_SCALE, 6);
    OUT_CS(rs->depth_scale_front);
    OUT_CS(rs->depth_offset_front);
    OUT_CS(rs->depth_scale_back);
    OUT_CS(rs->depth_offset_back);
    OUT_CS(rs->polygon_offset_enable);
    OUT_CS(rs->cull_mode);
    OUT_CS_REG(R300_GA_LINE_STIPPLE_CONFIG, rs->line_stipple_config);
    OUT_CS_REG(R300_GA_LINE_STIPPLE_VALUE, rs->line_stipple_value);
    END_CS;
}

static void r300_emit_dirty_state(struct r300_context* r300)
{
    struct r300_screen* r300screen =
        (struct r300_screen*)r300->context.screen;
    CS_LOCALS(r300);

    if (!(r300->dirty_state) && !(r300->dirty_hw)) {
        return;
    }

    /* XXX check size */

    if (r300->dirty_state & R300_NEW_BLEND) {
        r300_emit_blend_state(r300, r300->blend_state);
    }

    if (r300->dirty_state & R300_NEW_BLEND_COLOR) {
        r300_emit_blend_color_state(r300, r300->blend_color_state);
    }

    if (r300->dirty_state & R300_NEW_DSA) {
        r300_emit_dsa_state(r300, r300->dsa_state);
    }

    if (r300->dirty_state & R300_NEW_FRAGMENT_SHADER) {
        if (r300screen->caps->is_r500) {
            r500_emit_fragment_shader(r300,
                (struct r500_fragment_shader*)r300->fs);
        } else {
            r300_emit_fragment_shader(r300,
                (struct r300_fragment_shader*)r300->fs);
        }
    }

    if (r300->dirty_state & R300_NEW_RASTERIZER) {
        r300_emit_rs_state(r300, r300->rs_state);
    }

    if (r300->dirty_state & R300_NEW_SCISSOR) {
        struct r300_scissor_state* scissor = r300->scissor_state;
        /* XXX next two are contiguous regs */
        OUT_CS_REG(R300_SC_SCISSORS_TL, scissor->scissor_top_left);
        OUT_CS_REG(R300_SC_SCISSORS_BR, scissor->scissor_bottom_right);
    }

    r300->dirty_state = 0;
}
