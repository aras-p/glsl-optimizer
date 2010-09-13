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
#include "util/u_mm.h"
#include "util/u_simple_list.h"

#include "r300_context.h"
#include "r300_cs.h"
#include "r300_emit.h"
#include "r300_fs.h"
#include "r300_screen.h"
#include "r300_screen_buffer.h"
#include "r300_vs.h"

void r300_emit_blend_state(struct r300_context* r300,
                           unsigned size, void* state)
{
    struct r300_blend_state* blend = (struct r300_blend_state*)state;
    struct pipe_framebuffer_state* fb =
        (struct pipe_framebuffer_state*)r300->fb_state.state;
    CS_LOCALS(r300);

    if (fb->nr_cbufs) {
        WRITE_CS_TABLE(blend->cb, size);
    } else {
        WRITE_CS_TABLE(blend->cb_no_readwrite, size);
    }
}

void r300_emit_blend_color_state(struct r300_context* r300,
                                 unsigned size, void* state)
{
    struct r300_blend_color_state* bc = (struct r300_blend_color_state*)state;
    CS_LOCALS(r300);

    WRITE_CS_TABLE(bc->cb, size);
}

void r300_emit_clip_state(struct r300_context* r300,
                          unsigned size, void* state)
{
    struct r300_clip_state* clip = (struct r300_clip_state*)state;
    CS_LOCALS(r300);

    WRITE_CS_TABLE(clip->cb, size);
}

void r300_emit_dsa_state(struct r300_context* r300, unsigned size, void* state)
{
    struct r300_dsa_state* dsa = (struct r300_dsa_state*)state;
    struct pipe_framebuffer_state* fb =
        (struct pipe_framebuffer_state*)r300->fb_state.state;
    CS_LOCALS(r300);

    if (fb->zsbuf) {
        WRITE_CS_TABLE(&dsa->cb_begin, size);
    } else {
        WRITE_CS_TABLE(dsa->cb_no_readwrite, size);
    }
}

static const float * get_rc_constant_state(
    struct r300_context * r300,
    struct rc_constant * constant)
{
    struct r300_textures_state* texstate = r300->textures_state.state;
    static float vec[4] = { 0.0, 0.0, 0.0, 1.0 };
    struct pipe_resource *tex;

    assert(constant->Type == RC_CONSTANT_STATE);

    switch (constant->u.State[0]) {
        /* Factor for converting rectangle coords to
         * normalized coords. Should only show up on non-r500. */
        case RC_STATE_R300_TEXRECT_FACTOR:
            tex = texstate->sampler_views[constant->u.State[1]]->base.texture;
            vec[0] = 1.0 / tex->width0;
            vec[1] = 1.0 / tex->height0;
            break;

        case RC_STATE_R300_VIEWPORT_SCALE:
            vec[0] = r300->viewport.scale[0];
            vec[1] = r300->viewport.scale[1];
            vec[2] = r300->viewport.scale[2];
            break;

        case RC_STATE_R300_VIEWPORT_OFFSET:
            vec[0] = r300->viewport.translate[0];
            vec[1] = r300->viewport.translate[1];
            vec[2] = r300->viewport.translate[2];
            break;

        default:
            fprintf(stderr, "r300: Implementation error: "
                "Unknown RC_CONSTANT type %d\n", constant->u.State[0]);
    }

    /* This should either be (0, 0, 0, 1), which should be a relatively safe
     * RGBA or STRQ value, or it could be one of the RC_CONSTANT_STATE
     * state factors. */
    return vec;
}

/* Convert a normal single-precision float into the 7.16 format
 * used by the R300 fragment shader.
 */
uint32_t pack_float24(float f)
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

void r300_emit_fs(struct r300_context* r300, unsigned size, void *state)
{
    struct r300_fragment_shader *fs = r300_fs(r300);
    CS_LOCALS(r300);

    WRITE_CS_TABLE(fs->shader->cb_code, fs->shader->cb_code_size);
}

void r300_emit_fs_constants(struct r300_context* r300, unsigned size, void *state)
{
    struct r300_fragment_shader *fs = r300_fs(r300);
    struct r300_constant_buffer *buf = (struct r300_constant_buffer*)state;
    unsigned count = fs->shader->externals_count;
    unsigned i, j;
    CS_LOCALS(r300);

    if (count == 0)
        return;

    BEGIN_CS(size);
    OUT_CS_REG_SEQ(R300_PFS_PARAM_0_X, count * 4);
    if (buf->remap_table){
        for (i = 0; i < count; i++) {
            float *data = (float*)&buf->ptr[buf->remap_table[i]*4];
            for (j = 0; j < 4; j++)
                OUT_CS(pack_float24(data[j]));
        }
    } else {
        for (i = 0; i < count; i++)
            for (j = 0; j < 4; j++)
                OUT_CS(pack_float24(*(float*)&buf->ptr[i*4+j]));
    }

    END_CS;
}

void r300_emit_fs_rc_constant_state(struct r300_context* r300, unsigned size, void *state)
{
    struct r300_fragment_shader *fs = r300_fs(r300);
    struct rc_constant_list *constants = &fs->shader->code.constants;
    unsigned i;
    unsigned count = fs->shader->rc_state_count;
    unsigned first = fs->shader->externals_count;
    unsigned end = constants->Count;
    unsigned j;
    CS_LOCALS(r300);

    if (count == 0)
        return;

    BEGIN_CS(size);
    for(i = first; i < end; ++i) {
        if (constants->Constants[i].Type == RC_CONSTANT_STATE) {
            const float *data =
                    get_rc_constant_state(r300, &constants->Constants[i]);

            OUT_CS_REG_SEQ(R300_PFS_PARAM_0_X + i * 16, 4);
            for (j = 0; j < 4; j++)
                OUT_CS(pack_float24(data[j]));
        }
    }
    END_CS;
}

void r500_emit_fs(struct r300_context* r300, unsigned size, void *state)
{
    struct r300_fragment_shader *fs = r300_fs(r300);
    CS_LOCALS(r300);

    WRITE_CS_TABLE(fs->shader->cb_code, fs->shader->cb_code_size);
}

void r500_emit_fs_constants(struct r300_context* r300, unsigned size, void *state)
{
    struct r300_fragment_shader *fs = r300_fs(r300);
    struct r300_constant_buffer *buf = (struct r300_constant_buffer*)state;
    unsigned count = fs->shader->externals_count;
    CS_LOCALS(r300);

    if (count == 0)
        return;

    BEGIN_CS(size);
    OUT_CS_REG(R500_GA_US_VECTOR_INDEX, R500_GA_US_VECTOR_INDEX_TYPE_CONST);
    OUT_CS_ONE_REG(R500_GA_US_VECTOR_DATA, count * 4);
    if (buf->remap_table){
        for (unsigned i = 0; i < count; i++) {
            uint32_t *data = &buf->ptr[buf->remap_table[i]*4];
            OUT_CS_TABLE(data, 4);
        }
    } else {
        OUT_CS_TABLE(buf->ptr, count * 4);
    }
    END_CS;
}

void r500_emit_fs_rc_constant_state(struct r300_context* r300, unsigned size, void *state)
{
    struct r300_fragment_shader *fs = r300_fs(r300);
    struct rc_constant_list *constants = &fs->shader->code.constants;
    unsigned i;
    unsigned count = fs->shader->rc_state_count;
    unsigned first = fs->shader->externals_count;
    unsigned end = constants->Count;
    CS_LOCALS(r300);

    if (count == 0)
        return;

    BEGIN_CS(size);
    for(i = first; i < end; ++i) {
        if (constants->Constants[i].Type == RC_CONSTANT_STATE) {
            const float *data =
                    get_rc_constant_state(r300, &constants->Constants[i]);

            OUT_CS_REG(R500_GA_US_VECTOR_INDEX,
                       R500_GA_US_VECTOR_INDEX_TYPE_CONST |
                       (i & R500_GA_US_VECTOR_INDEX_MASK));
            OUT_CS_ONE_REG(R500_GA_US_VECTOR_DATA, 4);
            OUT_CS_TABLE(data, 4);
        }
    }
    END_CS;
}

void r300_emit_gpu_flush(struct r300_context *r300, unsigned size, void *state)
{
    struct r300_gpu_flush *gpuflush = (struct r300_gpu_flush*)state;
    struct pipe_framebuffer_state* fb =
            (struct pipe_framebuffer_state*)r300->fb_state.state;
    uint32_t height = fb->height;
    uint32_t width = fb->width;
    CS_LOCALS(r300);

    if (r300->cbzb_clear) {
        struct r300_surface *surf = r300_surface(fb->cbufs[0]);

        height = surf->cbzb_height;
        width = surf->cbzb_width;
    }

    DBG(r300, DBG_SCISSOR,
	"r300: Scissor width: %i, height: %i, CBZB clear: %s\n",
	width, height, r300->cbzb_clear ? "YES" : "NO");

    BEGIN_CS(size);

    /* Set up scissors.
     * By writing to the SC registers, SC & US assert idle. */
    OUT_CS_REG_SEQ(R300_SC_SCISSORS_TL, 2);
    if (r300->screen->caps.is_r500) {
        OUT_CS(0);
        OUT_CS(((width  - 1) << R300_SCISSORS_X_SHIFT) |
               ((height - 1) << R300_SCISSORS_Y_SHIFT));
    } else {
        OUT_CS((1440 << R300_SCISSORS_X_SHIFT) |
               (1440 << R300_SCISSORS_Y_SHIFT));
        OUT_CS(((width  + 1440-1) << R300_SCISSORS_X_SHIFT) |
               ((height + 1440-1) << R300_SCISSORS_Y_SHIFT));
    }

    /* Flush CB & ZB caches and wait until the 3D engine is idle and clean. */
    OUT_CS_TABLE(gpuflush->cb_flush_clean, 6);
    END_CS;
}

void r300_emit_aa_state(struct r300_context *r300, unsigned size, void *state)
{
    struct r300_aa_state *aa = (struct r300_aa_state*)state;
    CS_LOCALS(r300);

    BEGIN_CS(size);
    OUT_CS_REG(R300_GB_AA_CONFIG, aa->aa_config);

    if (aa->dest) {
        OUT_CS_REG_SEQ(R300_RB3D_AARESOLVE_OFFSET, 1);
        OUT_CS_RELOC(aa->dest->buffer, aa->dest->offset, 0, aa->dest->domain);

        OUT_CS_REG_SEQ(R300_RB3D_AARESOLVE_PITCH, 1);
        OUT_CS_RELOC(aa->dest->buffer, aa->dest->pitch, 0, aa->dest->domain);
    }

    OUT_CS_REG(R300_RB3D_AARESOLVE_CTL, aa->aaresolve_ctl);
    END_CS;
}

void r300_emit_fb_state(struct r300_context* r300, unsigned size, void* state)
{
    struct pipe_framebuffer_state* fb = (struct pipe_framebuffer_state*)state;
    struct r300_surface* surf;
    unsigned i;
    boolean has_hyperz = r300->rws->get_value(r300->rws, R300_CAN_HYPERZ);
    CS_LOCALS(r300);

    BEGIN_CS(size);

    /* NUM_MULTIWRITES replicates COLOR[0] to all colorbuffers, which is not
     * what we usually want. */
    if (r300->screen->caps.is_r500) {
        OUT_CS_REG(R300_RB3D_CCTL,
            R300_RB3D_CCTL_INDEPENDENT_COLORFORMAT_ENABLE_ENABLE);
    } else {
        OUT_CS_REG(R300_RB3D_CCTL, 0);
    }

    /* Set up colorbuffers. */
    for (i = 0; i < fb->nr_cbufs; i++) {
        surf = r300_surface(fb->cbufs[i]);

        OUT_CS_REG_SEQ(R300_RB3D_COLOROFFSET0 + (4 * i), 1);
        OUT_CS_RELOC(surf->buffer, surf->offset, 0, surf->domain);

        OUT_CS_REG_SEQ(R300_RB3D_COLORPITCH0 + (4 * i), 1);
        OUT_CS_RELOC(surf->buffer, surf->pitch, 0, surf->domain);
    }

    /* Set up the ZB part of the CBZB clear. */
    if (r300->cbzb_clear) {
        surf = r300_surface(fb->cbufs[0]);

        OUT_CS_REG(R300_ZB_FORMAT, surf->cbzb_format);

        OUT_CS_REG_SEQ(R300_ZB_DEPTHOFFSET, 1);
        OUT_CS_RELOC(surf->buffer, surf->cbzb_midpoint_offset, 0, surf->domain);

        OUT_CS_REG_SEQ(R300_ZB_DEPTHPITCH, 1);
        OUT_CS_RELOC(surf->buffer, surf->cbzb_pitch, 0, surf->domain);

        DBG(r300, DBG_CBZB,
            "CBZB clearing cbuf %08x %08x\n", surf->cbzb_format,
            surf->cbzb_pitch);
    }
    /* Set up a zbuffer. */
    else if (fb->zsbuf) {
        surf = r300_surface(fb->zsbuf);

        OUT_CS_REG(R300_ZB_FORMAT, surf->format);

        OUT_CS_REG_SEQ(R300_ZB_DEPTHOFFSET, 1);
        OUT_CS_RELOC(surf->buffer, surf->offset, 0, surf->domain);

        OUT_CS_REG_SEQ(R300_ZB_DEPTHPITCH, 1);
        OUT_CS_RELOC(surf->buffer, surf->pitch, 0, surf->domain);

        if (has_hyperz) {
            uint32_t surf_pitch;
            struct r300_texture *tex;
            int level = surf->base.level;
            tex = r300_texture(surf->base.texture);

            surf_pitch = surf->pitch & R300_DEPTHPITCH_MASK;
            /* HiZ RAM. */
            if (r300->screen->caps.hiz_ram) {
                if (tex->hiz_mem[level]) {
                    OUT_CS_REG(R300_ZB_HIZ_OFFSET, tex->hiz_mem[level]->ofs << 2);
                    OUT_CS_REG(R300_ZB_HIZ_PITCH, surf_pitch);
                } else {
                    OUT_CS_REG(R300_ZB_HIZ_OFFSET, 0);
                    OUT_CS_REG(R300_ZB_HIZ_PITCH, 0);
                }
            }
            /* Z Mask RAM. (compressed zbuffer) */
            if (tex->zmask_mem[level]) {
                OUT_CS_REG(R300_ZB_ZMASK_OFFSET, tex->zmask_mem[level]->ofs << 2);
                OUT_CS_REG(R300_ZB_ZMASK_PITCH, surf_pitch);
            } else {
                OUT_CS_REG(R300_ZB_ZMASK_OFFSET, 0);
                OUT_CS_REG(R300_ZB_ZMASK_PITCH, 0);
            }
        }
    }

    END_CS;
}

void r300_emit_hyperz_state(struct r300_context *r300,
                            unsigned size, void *state)
{
    struct r300_hyperz_state *z = state;
    CS_LOCALS(r300);
    if (z->flush)
        WRITE_CS_TABLE(&z->cb_flush_begin, size);
    else
        WRITE_CS_TABLE(&z->cb_begin, size - 2);
}

void r300_emit_hyperz_end(struct r300_context *r300)
{
    struct r300_hyperz_state z =
            *(struct r300_hyperz_state*)r300->hyperz_state.state;

    z.flush = 1;
    z.zb_bw_cntl = 0;
    z.zb_depthclearvalue = 0;
    z.sc_hyperz = R300_SC_HYPERZ_ADJ_2;
    z.gb_z_peq_config = 0;

    r300_emit_hyperz_state(r300, r300->hyperz_state.size, &z);
}

void r300_emit_fb_state_pipelined(struct r300_context *r300,
                                  unsigned size, void *state)
{
    struct pipe_framebuffer_state* fb =
            (struct pipe_framebuffer_state*)r300->fb_state.state;
    unsigned i;
    CS_LOCALS(r300);

    BEGIN_CS(size);

    /* Colorbuffer format in the US block.
     * (must be written after unpipelined regs) */
    OUT_CS_REG_SEQ(R300_US_OUT_FMT_0, 4);
    for (i = 0; i < fb->nr_cbufs; i++) {
        OUT_CS(r300_surface(fb->cbufs[i])->format);
    }
    for (; i < 4; i++) {
        OUT_CS(R300_US_OUT_FMT_UNUSED);
    }

    /* Multisampling. Depends on framebuffer sample count.
     * These are pipelined regs and as such cannot be moved
     * to the AA state. */
    if (r300->rws->get_value(r300->rws, R300_VID_DRM_2_3_0)) {
        unsigned mspos0 = 0x66666666;
        unsigned mspos1 = 0x6666666;

        if (fb->nr_cbufs && fb->cbufs[0]->texture->nr_samples > 1) {
            /* Subsample placement. These may not be optimal. */
            switch (fb->cbufs[0]->texture->nr_samples) {
                case 2:
                    mspos0 = 0x33996633;
                    mspos1 = 0x6666663;
                    break;
                case 3:
                    mspos0 = 0x33936933;
                    mspos1 = 0x6666663;
                    break;
                case 4:
                    mspos0 = 0x33939933;
                    mspos1 = 0x3966663;
                    break;
                case 6:
                    mspos0 = 0x22a2aa22;
                    mspos1 = 0x2a65672;
                    break;
                default:
                    debug_printf("r300: Bad number of multisamples!\n");
            }
        }

        OUT_CS_REG_SEQ(R300_GB_MSPOS0, 2);
        OUT_CS(mspos0);
        OUT_CS(mspos1);
    }
    END_CS;
}

void r300_emit_query_start(struct r300_context *r300, unsigned size, void*state)
{
    struct r300_query *query = r300->query_current;
    CS_LOCALS(r300);

    if (!query)
	return;

    BEGIN_CS(size);
    if (r300->screen->caps.family == CHIP_FAMILY_RV530) {
        OUT_CS_REG(RV530_FG_ZBREG_DEST, RV530_FG_ZBREG_DEST_PIPE_SELECT_ALL);
    } else {
        OUT_CS_REG(R300_SU_REG_DEST, R300_RASTER_PIPE_SELECT_ALL);
    }
    OUT_CS_REG(R300_ZB_ZPASS_DATA, 0);
    END_CS;
    query->begin_emitted = TRUE;
    query->flushed = FALSE;
}

static void r300_emit_query_end_frag_pipes(struct r300_context *r300,
                                           struct r300_query *query)
{
    struct r300_capabilities* caps = &r300->screen->caps;
    struct r300_winsys_buffer *buf = r300->query_current->buffer;
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
            OUT_CS_RELOC(buf, (query->num_results + 3) * 4,
                    0, query->domain);
        case 3:
            /* pipe 2 only */
            OUT_CS_REG(R300_SU_REG_DEST, 1 << 2);
            OUT_CS_REG_SEQ(R300_ZB_ZPASS_ADDR, 1);
            OUT_CS_RELOC(buf, (query->num_results + 2) * 4,
                    0, query->domain);
        case 2:
            /* pipe 1 only */
            /* As mentioned above, accomodate RV380 and older. */
            OUT_CS_REG(R300_SU_REG_DEST,
                    1 << (caps->high_second_pipe ? 3 : 1));
            OUT_CS_REG_SEQ(R300_ZB_ZPASS_ADDR, 1);
            OUT_CS_RELOC(buf, (query->num_results + 1) * 4,
                    0, query->domain);
        case 1:
            /* pipe 0 only */
            OUT_CS_REG(R300_SU_REG_DEST, 1 << 0);
            OUT_CS_REG_SEQ(R300_ZB_ZPASS_ADDR, 1);
            OUT_CS_RELOC(buf, (query->num_results + 0) * 4,
                    0, query->domain);
            break;
        default:
            fprintf(stderr, "r300: Implementation error: Chipset reports %d"
                    " pixel pipes!\n", caps->num_frag_pipes);
            abort();
    }

    /* And, finally, reset it to normal... */
    OUT_CS_REG(R300_SU_REG_DEST, 0xF);
    END_CS;
}

static void rv530_emit_query_end_single_z(struct r300_context *r300,
                                          struct r300_query *query)
{
    struct r300_winsys_buffer *buf = r300->query_current->buffer;
    CS_LOCALS(r300);

    BEGIN_CS(8);
    OUT_CS_REG(RV530_FG_ZBREG_DEST, RV530_FG_ZBREG_DEST_PIPE_SELECT_0);
    OUT_CS_REG_SEQ(R300_ZB_ZPASS_ADDR, 1);
    OUT_CS_RELOC(buf, query->num_results * 4, 0, query->domain);
    OUT_CS_REG(RV530_FG_ZBREG_DEST, RV530_FG_ZBREG_DEST_PIPE_SELECT_ALL);
    END_CS;
}

static void rv530_emit_query_end_double_z(struct r300_context *r300,
                                          struct r300_query *query)
{
    struct r300_winsys_buffer *buf = r300->query_current->buffer;
    CS_LOCALS(r300);

    BEGIN_CS(14);
    OUT_CS_REG(RV530_FG_ZBREG_DEST, RV530_FG_ZBREG_DEST_PIPE_SELECT_0);
    OUT_CS_REG_SEQ(R300_ZB_ZPASS_ADDR, 1);
    OUT_CS_RELOC(buf, (query->num_results + 0) * 4, 0, query->domain);
    OUT_CS_REG(RV530_FG_ZBREG_DEST, RV530_FG_ZBREG_DEST_PIPE_SELECT_1);
    OUT_CS_REG_SEQ(R300_ZB_ZPASS_ADDR, 1);
    OUT_CS_RELOC(buf, (query->num_results + 1) * 4, 0, query->domain);
    OUT_CS_REG(RV530_FG_ZBREG_DEST, RV530_FG_ZBREG_DEST_PIPE_SELECT_ALL);
    END_CS;
}

void r300_emit_query_end(struct r300_context* r300)
{
    struct r300_capabilities *caps = &r300->screen->caps;
    struct r300_query *query = r300->query_current;

    if (!query)
	return;

    if (query->begin_emitted == FALSE)
        return;

    if (caps->family == CHIP_FAMILY_RV530) {
        if (caps->num_z_pipes == 2)
            rv530_emit_query_end_double_z(r300, query);
        else
            rv530_emit_query_end_single_z(r300, query);
    } else 
        r300_emit_query_end_frag_pipes(r300, query);

    query->begin_emitted = FALSE;
    query->num_results += query->num_pipes;

    /* XXX grab all the results and reset the counter. */
    if (query->num_results >= query->buffer_size / 4 - 4) {
        query->num_results = (query->buffer_size / 4) / 2;
        fprintf(stderr, "r300: Rewinding OQBO...\n");
    }
}

void r300_emit_invariant_state(struct r300_context *r300,
                               unsigned size, void *state)
{
    CS_LOCALS(r300);
    WRITE_CS_TABLE(state, size);
}

void r300_emit_rs_state(struct r300_context* r300, unsigned size, void* state)
{
    struct r300_rs_state* rs = state;
    CS_LOCALS(r300);

    BEGIN_CS(size);
    OUT_CS_TABLE(rs->cb_main, 25);
    if (rs->polygon_offset_enable) {
        if (r300->zbuffer_bpp == 16) {
            OUT_CS_TABLE(rs->cb_poly_offset_zb16, 5);
        } else {
            OUT_CS_TABLE(rs->cb_poly_offset_zb24, 5);
        }
    }
    END_CS;
}

void r300_emit_rs_block_state(struct r300_context* r300,
                              unsigned size, void* state)
{
    struct r300_rs_block* rs = (struct r300_rs_block*)state;
    unsigned i;
    /* It's the same for both INST and IP tables */
    unsigned count = (rs->inst_count & R300_RS_INST_COUNT_MASK) + 1;
    CS_LOCALS(r300);

    if (DBG_ON(r300, DBG_RS_BLOCK)) {
        r500_dump_rs_block(rs);

        fprintf(stderr, "r300: RS emit:\n");

        for (i = 0; i < count; i++)
            fprintf(stderr, "    : ip %d: 0x%08x\n", i, rs->ip[i]);

        for (i = 0; i < count; i++)
            fprintf(stderr, "    : inst %d: 0x%08x\n", i, rs->inst[i]);

        fprintf(stderr, "    : count: 0x%08x inst_count: 0x%08x\n",
            rs->count, rs->inst_count);
    }

    BEGIN_CS(size);
    OUT_CS_REG_SEQ(R300_VAP_VTX_STATE_CNTL, 2);
    OUT_CS(rs->vap_vtx_state_cntl);
    OUT_CS(rs->vap_vsm_vtx_assm);
    OUT_CS_REG_SEQ(R300_VAP_OUTPUT_VTX_FMT_0, 2);
    OUT_CS(rs->vap_out_vtx_fmt[0]);
    OUT_CS(rs->vap_out_vtx_fmt[1]);

    if (r300->screen->caps.is_r500) {
        OUT_CS_REG_SEQ(R500_RS_IP_0, count);
    } else {
        OUT_CS_REG_SEQ(R300_RS_IP_0, count);
    }
    OUT_CS_TABLE(rs->ip, count);

    OUT_CS_REG_SEQ(R300_RS_COUNT, 2);
    OUT_CS(rs->count);
    OUT_CS(rs->inst_count);

    if (r300->screen->caps.is_r500) {
        OUT_CS_REG_SEQ(R500_RS_INST_0, count);
    } else {
        OUT_CS_REG_SEQ(R300_RS_INST_0, count);
    }
    OUT_CS_TABLE(rs->inst, count);
    END_CS;
}

void r300_emit_scissor_state(struct r300_context* r300,
                             unsigned size, void* state)
{
    struct pipe_scissor_state* scissor = (struct pipe_scissor_state*)state;
    CS_LOCALS(r300);

    BEGIN_CS(size);
    OUT_CS_REG_SEQ(R300_SC_CLIPRECT_TL_0, 2);
    if (r300->screen->caps.is_r500) {
        OUT_CS((scissor->minx << R300_CLIPRECT_X_SHIFT) |
               (scissor->miny << R300_CLIPRECT_Y_SHIFT));
        OUT_CS(((scissor->maxx - 1) << R300_CLIPRECT_X_SHIFT) |
               ((scissor->maxy - 1) << R300_CLIPRECT_Y_SHIFT));
    } else {
        OUT_CS(((scissor->minx + 1440) << R300_CLIPRECT_X_SHIFT) |
               ((scissor->miny + 1440) << R300_CLIPRECT_Y_SHIFT));
        OUT_CS(((scissor->maxx + 1440-1) << R300_CLIPRECT_X_SHIFT) |
               ((scissor->maxy + 1440-1) << R300_CLIPRECT_Y_SHIFT));
    }
    END_CS;
}

void r300_emit_textures_state(struct r300_context *r300,
                              unsigned size, void *state)
{
    struct r300_textures_state *allstate = (struct r300_textures_state*)state;
    struct r300_texture_sampler_state *texstate;
    struct r300_texture *tex;
    unsigned i;
    CS_LOCALS(r300);

    BEGIN_CS(size);
    OUT_CS_REG(R300_TX_ENABLE, allstate->tx_enable);

    for (i = 0; i < allstate->count; i++) {
        if ((1 << i) & allstate->tx_enable) {
            texstate = &allstate->regs[i];
            tex = r300_texture(allstate->sampler_views[i]->base.texture);

            OUT_CS_REG(R300_TX_FILTER0_0 + (i * 4), texstate->filter0);
            OUT_CS_REG(R300_TX_FILTER1_0 + (i * 4), texstate->filter1);
            OUT_CS_REG(R300_TX_BORDER_COLOR_0 + (i * 4),
                       texstate->border_color);

            OUT_CS_REG(R300_TX_FORMAT0_0 + (i * 4), texstate->format.format0);
            OUT_CS_REG(R300_TX_FORMAT1_0 + (i * 4), texstate->format.format1);
            OUT_CS_REG(R300_TX_FORMAT2_0 + (i * 4), texstate->format.format2);

            OUT_CS_REG_SEQ(R300_TX_OFFSET_0 + (i * 4), 1);
            OUT_CS_TEX_RELOC(tex, texstate->format.tile_config, tex->domain,
                             0);
        }
    }
    END_CS;
}

void r300_emit_aos(struct r300_context* r300, int offset, boolean indexed)
{
    struct pipe_vertex_buffer *vb1, *vb2, *vbuf = r300->vertex_buffer;
    struct pipe_vertex_element *velem = r300->velems->velem;
    struct r300_buffer *buf;
    int i;
    unsigned *hw_format_size = r300->velems->hw_format_size;
    unsigned size1, size2, aos_count = r300->velems->count;
    unsigned packet_size = (aos_count * 3 + 1) / 2;
    CS_LOCALS(r300);

    BEGIN_CS(2 + packet_size + aos_count * 2);
    OUT_CS_PKT3(R300_PACKET3_3D_LOAD_VBPNTR, packet_size);
    OUT_CS(aos_count | (!indexed ? R300_VC_FORCE_PREFETCH : 0));

    for (i = 0; i < aos_count - 1; i += 2) {
        vb1 = &vbuf[velem[i].vertex_buffer_index];
        vb2 = &vbuf[velem[i+1].vertex_buffer_index];
        size1 = hw_format_size[i];
        size2 = hw_format_size[i+1];

        OUT_CS(R300_VBPNTR_SIZE0(size1) | R300_VBPNTR_STRIDE0(vb1->stride) |
               R300_VBPNTR_SIZE1(size2) | R300_VBPNTR_STRIDE1(vb2->stride));
        OUT_CS(vb1->buffer_offset + velem[i].src_offset   + offset * vb1->stride);
        OUT_CS(vb2->buffer_offset + velem[i+1].src_offset + offset * vb2->stride);
    }

    if (aos_count & 1) {
        vb1 = &vbuf[velem[i].vertex_buffer_index];
        size1 = hw_format_size[i];

        OUT_CS(R300_VBPNTR_SIZE0(size1) | R300_VBPNTR_STRIDE0(vb1->stride));
        OUT_CS(vb1->buffer_offset + velem[i].src_offset + offset * vb1->stride);
    }

    for (i = 0; i < aos_count; i++) {
        buf = r300_buffer(vbuf[velem[i].vertex_buffer_index].buffer);
        OUT_CS_BUF_RELOC_NO_OFFSET(&buf->b.b, buf->domain, 0);
    }
    END_CS;
}

void r300_emit_aos_swtcl(struct r300_context *r300, boolean indexed)
{
    CS_LOCALS(r300);

    DBG(r300, DBG_SWTCL, "r300: Preparing vertex buffer %p for render, "
            "vertex size %d\n", r300->vbo,
            r300->vertex_info.size);
    /* Set the pointer to our vertex buffer. The emitted values are this:
     * PACKET3 [3D_LOAD_VBPNTR]
     * COUNT   [1]
     * FORMAT  [size | stride << 8]
     * OFFSET  [offset into BO]
     * VBPNTR  [relocated BO]
     */
    BEGIN_CS(7);
    OUT_CS_PKT3(R300_PACKET3_3D_LOAD_VBPNTR, 3);
    OUT_CS(1 | (!indexed ? R300_VC_FORCE_PREFETCH : 0));
    OUT_CS(r300->vertex_info.size |
            (r300->vertex_info.size << 8));
    OUT_CS(r300->draw_vbo_offset);
    OUT_CS_BUF_RELOC(r300->vbo, 0, r300_buffer(r300->vbo)->domain, 0);
    END_CS;
}

void r300_emit_vertex_stream_state(struct r300_context* r300,
                                   unsigned size, void* state)
{
    struct r300_vertex_stream_state *streams =
        (struct r300_vertex_stream_state*)state;
    unsigned i;
    CS_LOCALS(r300);

    if (DBG_ON(r300, DBG_PSC)) {
        fprintf(stderr, "r300: PSC emit:\n");

        for (i = 0; i < streams->count; i++) {
            fprintf(stderr, "    : prog_stream_cntl%d: 0x%08x\n", i,
                   streams->vap_prog_stream_cntl[i]);
        }

        for (i = 0; i < streams->count; i++) {
            fprintf(stderr, "    : prog_stream_cntl_ext%d: 0x%08x\n", i,
                   streams->vap_prog_stream_cntl_ext[i]);
        }
    }

    BEGIN_CS(size);
    OUT_CS_REG_SEQ(R300_VAP_PROG_STREAM_CNTL_0, streams->count);
    OUT_CS_TABLE(streams->vap_prog_stream_cntl, streams->count);
    OUT_CS_REG_SEQ(R300_VAP_PROG_STREAM_CNTL_EXT_0, streams->count);
    OUT_CS_TABLE(streams->vap_prog_stream_cntl_ext, streams->count);
    END_CS;
}

void r300_emit_pvs_flush(struct r300_context* r300, unsigned size, void* state)
{
    CS_LOCALS(r300);

    BEGIN_CS(size);
    OUT_CS_REG(R300_VAP_PVS_STATE_FLUSH_REG, 0x0);
    END_CS;
}

void r300_emit_vap_invariant_state(struct r300_context *r300,
                                   unsigned size, void *state)
{
    CS_LOCALS(r300);
    WRITE_CS_TABLE(state, size);
}

void r300_emit_vs_state(struct r300_context* r300, unsigned size, void* state)
{
    struct r300_vertex_shader* vs = (struct r300_vertex_shader*)state;
    struct r300_vertex_program_code* code = &vs->code;
    struct r300_screen* r300screen = r300->screen;
    unsigned instruction_count = code->length / 4;
    unsigned i;

    unsigned vtx_mem_size = r300screen->caps.is_r500 ? 128 : 72;
    unsigned input_count = MAX2(util_bitcount(code->InputsRead), 1);
    unsigned output_count = MAX2(util_bitcount(code->OutputsWritten), 1);
    unsigned temp_count = MAX2(code->num_temporaries, 1);

    unsigned pvs_num_slots = MIN3(vtx_mem_size / input_count,
                                  vtx_mem_size / output_count, 10);
    unsigned pvs_num_controllers = MIN2(vtx_mem_size / temp_count, 5);

    unsigned imm_first = vs->externals_count;
    unsigned imm_end = vs->code.constants.Count;
    unsigned imm_count = vs->immediates_count;

    CS_LOCALS(r300);

    BEGIN_CS(size);

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
    OUT_CS_TABLE(code->body.d, code->length);

    OUT_CS_REG(R300_VAP_CNTL, R300_PVS_NUM_SLOTS(pvs_num_slots) |
            R300_PVS_NUM_CNTLRS(pvs_num_controllers) |
            R300_PVS_NUM_FPUS(r300screen->caps.num_vert_fpus) |
            R300_PVS_VF_MAX_VTX_NUM(12) |
            (r300screen->caps.is_r500 ? R500_TCL_STATE_OPTIMIZATION : 0));

    /* Emit immediates. */
    if (imm_count) {
        OUT_CS_REG(R300_VAP_PVS_VECTOR_INDX_REG,
                   (r300->screen->caps.is_r500 ?
                   R500_PVS_CONST_START : R300_PVS_CONST_START) +
                   imm_first);
        OUT_CS_ONE_REG(R300_VAP_PVS_UPLOAD_DATA, imm_count * 4);
        for (i = imm_first; i < imm_end; i++) {
            const float *data = vs->code.constants.Constants[i].u.Immediate;
            OUT_CS_TABLE(data, 4);
        }
    }

    /* Emit flow control instructions. */
    if (code->num_fc_ops) {

        OUT_CS_REG(R300_VAP_PVS_FLOW_CNTL_OPC, code->fc_ops);
        if (r300screen->caps.is_r500) {
            OUT_CS_REG_SEQ(R500_VAP_PVS_FLOW_CNTL_ADDRS_LW_0, code->num_fc_ops * 2);
            OUT_CS_TABLE(code->fc_op_addrs.r500, code->num_fc_ops * 2);
        } else {
            OUT_CS_REG_SEQ(R300_VAP_PVS_FLOW_CNTL_ADDRS_0, code->num_fc_ops);
            OUT_CS_TABLE(code->fc_op_addrs.r300, code->num_fc_ops);
        }
        OUT_CS_REG_SEQ(R300_VAP_PVS_FLOW_CNTL_LOOP_INDEX_0, code->num_fc_ops);
        OUT_CS_TABLE(code->fc_loop_index, code->num_fc_ops);
    }

    END_CS;
}

void r300_emit_vs_constants(struct r300_context* r300,
                            unsigned size, void *state)
{
    unsigned count =
        ((struct r300_vertex_shader*)r300->vs_state.state)->externals_count;
    struct r300_constant_buffer *buf = (struct r300_constant_buffer*)state;
    unsigned i;
    CS_LOCALS(r300);

    if (!count)
        return;

    BEGIN_CS(size);
    OUT_CS_REG(R300_VAP_PVS_VECTOR_INDX_REG,
               (r300->screen->caps.is_r500 ?
               R500_PVS_CONST_START : R300_PVS_CONST_START));
    OUT_CS_ONE_REG(R300_VAP_PVS_UPLOAD_DATA, count * 4);
    if (buf->remap_table){
        for (i = 0; i < count; i++) {
            uint32_t *data = &buf->ptr[buf->remap_table[i]*4];
            OUT_CS_TABLE(data, 4);
        }
    } else {
        OUT_CS_TABLE(buf->ptr, count * 4);
    }
    END_CS;
}

void r300_emit_viewport_state(struct r300_context* r300,
                              unsigned size, void* state)
{
    struct r300_viewport_state* viewport = (struct r300_viewport_state*)state;
    CS_LOCALS(r300);

    BEGIN_CS(size);
    OUT_CS_REG_SEQ(R300_SE_VPORT_XSCALE, 6);
    OUT_CS_TABLE(&viewport->xscale, 6);
    OUT_CS_REG(R300_VAP_VTE_CNTL, viewport->vte_control);
    END_CS;
}

static void r300_emit_hiz_line_clear(struct r300_context *r300, int start, uint16_t count, uint32_t val)
{
    CS_LOCALS(r300);
    BEGIN_CS(4);
    OUT_CS_PKT3(R300_PACKET3_3D_CLEAR_HIZ, 2);
    OUT_CS(start);
    OUT_CS(count);
    OUT_CS(val);
    END_CS;
}

static void r300_emit_zmask_line_clear(struct r300_context *r300, int start, uint16_t count, uint32_t val)
{
    CS_LOCALS(r300);
    BEGIN_CS(4);
    OUT_CS_PKT3(R300_PACKET3_3D_CLEAR_ZMASK, 2);
    OUT_CS(start);
    OUT_CS(count);
    OUT_CS(val);
    END_CS;
}

#define ALIGN_DIVUP(x, y) (((x) + (y) - 1) / (y))

void r300_emit_hiz_clear(struct r300_context *r300, unsigned size, void *state)
{
    struct pipe_framebuffer_state *fb =
        (struct pipe_framebuffer_state*)r300->fb_state.state;
    struct r300_hyperz_state *z =
        (struct r300_hyperz_state*)r300->hyperz_state.state;
    struct r300_screen* r300screen = r300->screen;
    uint32_t stride, offset = 0, height, offset_shift;
    struct r300_texture* tex;
    int i;

    tex = r300_texture(fb->zsbuf->texture);

    offset = tex->hiz_mem[fb->zsbuf->level]->ofs;
    stride = tex->desc.stride_in_pixels[fb->zsbuf->level];

    /* convert from pixels to 4x4 blocks */
    stride = ALIGN_DIVUP(stride, 4);

    stride = ALIGN_DIVUP(stride, r300screen->caps.num_frag_pipes);    
    /* there are 4 blocks per dwords */
    stride = ALIGN_DIVUP(stride, 4);

    height = ALIGN_DIVUP(fb->zsbuf->height, 4);

    offset_shift = 2;
    offset_shift += (r300screen->caps.num_frag_pipes / 2);

    for (i = 0; i < height; i++) {
        offset = i * stride;
        offset <<= offset_shift;
        r300_emit_hiz_line_clear(r300, offset, stride, 0xffffffff);
    }
    z->current_func = -1;

    /* Mark the current zbuffer's hiz ram as in use. */
    tex->hiz_in_use[fb->zsbuf->level] = TRUE;
}

void r300_emit_zmask_clear(struct r300_context *r300, unsigned size, void *state)
{
    struct pipe_framebuffer_state *fb =
        (struct pipe_framebuffer_state*)r300->fb_state.state;
    struct r300_screen* r300screen = r300->screen;
    uint32_t stride, offset = 0;
    struct r300_texture* tex;
    uint32_t i, height;
    int mult, offset_shift;

    tex = r300_texture(fb->zsbuf->texture);
    stride = tex->desc.stride_in_pixels[fb->zsbuf->level];

    offset = tex->zmask_mem[fb->zsbuf->level]->ofs;

    if (r300->z_compression == RV350_Z_COMPRESS_88)
        mult = 8;
    else
        mult = 4;

    height = ALIGN_DIVUP(fb->zsbuf->height, mult);

    offset_shift = 4;
    offset_shift += (r300screen->caps.num_frag_pipes / 2);
    stride = ALIGN_DIVUP(stride, r300screen->caps.num_frag_pipes);

    /* okay have width in pixels - divide by block width */
    stride = ALIGN_DIVUP(stride, mult);
    /* have width in blocks - divide by number of fragment pipes screen width */
    /* 16 blocks per dword */
    stride = ALIGN_DIVUP(stride, 16);

    for (i = 0; i < height; i++) {
        offset = i * stride;
        offset <<= offset_shift;
        r300_emit_zmask_line_clear(r300, offset, stride, 0x0);//0xffffffff);
    }

    /* Mark the current zbuffer's zmask as in use. */
    tex->zmask_in_use[fb->zsbuf->level] = TRUE;
}

void r300_emit_ztop_state(struct r300_context* r300,
                          unsigned size, void* state)
{
    struct r300_ztop_state* ztop = (struct r300_ztop_state*)state;
    CS_LOCALS(r300);

    BEGIN_CS(size);
    OUT_CS_REG(R300_ZB_ZTOP, ztop->z_buffer_top);
    END_CS;
}

void r300_emit_texture_cache_inval(struct r300_context* r300, unsigned size, void* state)
{
    CS_LOCALS(r300);

    BEGIN_CS(size);
    OUT_CS_REG(R300_TX_INVALTAGS, 0);
    END_CS;
}

boolean r300_emit_buffer_validate(struct r300_context *r300,
                                  boolean do_validate_vertex_buffers,
                                  struct pipe_resource *index_buffer)
{
    struct pipe_framebuffer_state* fb =
        (struct pipe_framebuffer_state*)r300->fb_state.state;
    struct r300_textures_state *texstate =
        (struct r300_textures_state*)r300->textures_state.state;
    struct r300_texture* tex;
    struct pipe_vertex_buffer *vbuf = r300->vertex_buffer;
    struct pipe_vertex_element *velem = r300->velems->velem;
    struct pipe_resource *pbuf;
    unsigned i;

    /* upload buffers first */
    if (r300->screen->caps.has_tcl && r300->any_user_vbs) {
        r300_upload_user_buffers(r300);
        r300->any_user_vbs = false;
    }

    /* Clean out BOs. */
    r300->rws->cs_reset_buffers(r300->cs);

    /* Color buffers... */
    for (i = 0; i < fb->nr_cbufs; i++) {
        tex = r300_texture(fb->cbufs[i]->texture);
        assert(tex && tex->buffer && "cbuf is marked, but NULL!");
        r300->rws->cs_add_buffer(r300->cs, tex->buffer, 0,
                                 r300_surface(fb->cbufs[i])->domain);
    }
    /* ...depth buffer... */
    if (fb->zsbuf) {
        tex = r300_texture(fb->zsbuf->texture);
        assert(tex && tex->buffer && "zsbuf is marked, but NULL!");
        r300->rws->cs_add_buffer(r300->cs, tex->buffer, 0,
                                 r300_surface(fb->zsbuf)->domain);
    }
    /* ...textures... */
    for (i = 0; i < texstate->count; i++) {
        if (!(texstate->tx_enable & (1 << i))) {
            continue;
        }

        tex = r300_texture(texstate->sampler_views[i]->base.texture);
        r300->rws->cs_add_buffer(r300->cs, tex->buffer, tex->domain, 0);
    }
    /* ...occlusion query buffer... */
    if (r300->query_current)
        r300->rws->cs_add_buffer(r300->cs, r300->query_current->buffer,
                                 0, r300->query_current->domain);
    /* ...vertex buffer for SWTCL path... */
    if (r300->vbo)
        r300->rws->cs_add_buffer(r300->cs, r300_buffer(r300->vbo)->buf,
                                 r300_buffer(r300->vbo)->domain, 0);
    /* ...vertex buffers for HWTCL path... */
    if (do_validate_vertex_buffers) {
        for (i = 0; i < r300->velems->count; i++) {
            pbuf = vbuf[velem[i].vertex_buffer_index].buffer;

            r300->rws->cs_add_buffer(r300->cs, r300_buffer(pbuf)->buf,
                                     r300_buffer(pbuf)->domain, 0);
        }
    }
    /* ...and index buffer for HWTCL path. */
    if (index_buffer)
        r300->rws->cs_add_buffer(r300->cs, r300_buffer(index_buffer)->buf,
                                 r300_buffer(index_buffer)->domain, 0);

    if (!r300->rws->cs_validate(r300->cs)) {
        return FALSE;
    }

    return TRUE;
}

unsigned r300_get_num_dirty_dwords(struct r300_context *r300)
{
    struct r300_atom* atom;
    unsigned dwords = 0;

    foreach(atom, &r300->atom_list) {
        if (atom->dirty) {
            dwords += atom->size;
        }
    }

    /* let's reserve some more, just in case */
    dwords += 32;

    return dwords;
}

unsigned r300_get_num_cs_end_dwords(struct r300_context *r300)
{
    unsigned dwords = 0;

    /* Emitted in flush. */
    dwords += 26; /* emit_query_end */
    dwords += r300->hyperz_state.size + 2; /* emit_hyperz_end + zcache flush */
    if (r500_index_bias_supported(r300))
        dwords += 2;

    return dwords;
}

/* Emit all dirty state. */
void r300_emit_dirty_state(struct r300_context* r300)
{
    struct r300_atom* atom;

    foreach(atom, &r300->atom_list) {
        if (atom->dirty) {
            atom->emit(r300, atom->size, atom->state);
            if (SCREEN_DBG_ON(r300->screen, DBG_STATS)) {
                atom->counter++;
            }
            atom->dirty = FALSE;
        }
    }

    r300->dirty_hw++;
}
