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

#include "r300_context.h"
#include "r300_hyperz.h"
#include "r300_reg.h"
#include "r300_fs.h"
#include "r300_winsys.h"

#include "util/u_format.h"
#include "util/u_mm.h"

/*
  HiZ rules - taken from various docs 
   1. HiZ only works on depth values
   2. Cannot HiZ if stencil fail or zfail is !KEEP
   3. on R300/400, HiZ is disabled if depth test is EQUAL
   4. comparison changes without clears usually mean disabling HiZ
*/
/*****************************************************************************/
/* The HyperZ setup                                                          */
/*****************************************************************************/

static bool r300_get_sc_hz_max(struct r300_context *r300)
{
    struct r300_dsa_state *dsa_state = r300->dsa_state.state;
    int func = dsa_state->z_stencil_control & 0x7;
    int ret = R300_SC_HYPERZ_MIN;

    if (func >= 4 && func <= 7)
       ret = R300_SC_HYPERZ_MAX;
    return ret;
}

static bool r300_zfunc_same_direction(int func1, int func2)
{
    /* func1 is less/lessthan */
    if (func1 == 1 || func1 == 2)
        if (func2 == 3 || func2 == 4 || func2 == 5)
            return FALSE;

    if (func2 == 1 || func2 == 2)
        if (func1 == 4 || func1 == 5)
            return FALSE;
    return TRUE;
}
    
static int r300_get_hiz_min(struct r300_context *r300)
{
    struct r300_dsa_state *dsa_state = r300->dsa_state.state;
    int func = dsa_state->z_stencil_control & 0x7;
    int ret = R300_HIZ_MIN;

    if (func == 1 || func == 2)
       ret = R300_HIZ_MAX;
    return ret;
}

static boolean r300_dsa_stencil_op_not_keep(struct pipe_stencil_state *s)
{
    if (s->enabled && (s->fail_op != PIPE_STENCIL_OP_KEEP ||
                       s->zfail_op != PIPE_STENCIL_OP_KEEP))
        return TRUE;
    return FALSE;
}

static boolean r300_can_hiz(struct r300_context *r300)
{
    struct r300_dsa_state *dsa_state = r300->dsa_state.state;
    struct pipe_depth_stencil_alpha_state *dsa = &dsa_state->dsa;
    struct r300_screen* r300screen = r300->screen;
    struct r300_hyperz_state *z = r300->hyperz_state.state;

    /* shader writes depth - no HiZ */
    if (r300_fragment_shader_writes_depth(r300_fs(r300))) /* (5) */
        return FALSE;

    if (r300->query_current)
        return FALSE;
    /* if stencil fail/zfail op is not KEEP */
    if (r300_dsa_stencil_op_not_keep(&dsa->stencil[0]) ||
        r300_dsa_stencil_op_not_keep(&dsa->stencil[1]))
        return FALSE;

    if (dsa->depth.enabled) {
        /* if depth func is EQUAL pre-r500 */
        if (dsa->depth.func == PIPE_FUNC_EQUAL && !r300screen->caps.is_r500)
            return FALSE;
        /* if depth func is NOTEQUAL */
        if (dsa->depth.func == PIPE_FUNC_NOTEQUAL)
            return FALSE;
    }
    /* depth comparison function - if just cleared save and return okay */
    if (z->current_func == -1) {
        int func = dsa_state->z_stencil_control & 0x7;
        if (func != 0 && func != 7)
            z->current_func = dsa_state->z_stencil_control & 0x7;
    } else {
        /* simple don't change */
        if (!r300_zfunc_same_direction(z->current_func, (dsa_state->z_stencil_control & 0x7))) {
            DBG(r300, DBG_HYPERZ, "z func changed direction - disabling hyper-z %d -> %d\n", z->current_func, dsa_state->z_stencil_control);
            return FALSE;
        }
    }    
    return TRUE;
}

static void r300_update_hyperz(struct r300_context* r300)
{
    struct r300_hyperz_state *z =
        (struct r300_hyperz_state*)r300->hyperz_state.state;
    struct pipe_framebuffer_state *fb =
        (struct pipe_framebuffer_state*)r300->fb_state.state;
    struct r300_texture *zstex =
            fb->zsbuf ? r300_texture(fb->zsbuf->texture) : NULL;
    boolean zmask_in_use = FALSE;
    boolean hiz_in_use = FALSE;

    z->gb_z_peq_config = 0;
    z->zb_bw_cntl = 0;
    z->sc_hyperz = R300_SC_HYPERZ_ADJ_2;
    z->flush = 0;

    if (r300->cbzb_clear) {
        z->zb_bw_cntl |= R300_ZB_CB_CLEAR_CACHE_LINE_WRITE_ONLY;
        return;
    }

    if (!zstex)
        return;

    if (!r300->rws->get_value(r300->rws, R300_CAN_HYPERZ))
        return;

    zmask_in_use = zstex->zmask_in_use[fb->zsbuf->level];
    hiz_in_use = zstex->hiz_in_use[fb->zsbuf->level];

    /* Z fastfill. */
    if (zmask_in_use) {
        z->zb_bw_cntl |= R300_FAST_FILL_ENABLE; /*  | R300_FORCE_COMPRESSED_STENCIL_VALUE_ENABLE;*/
    }

    /* Zbuffer compression. */
    if (zmask_in_use && r300->z_compression) {
        z->zb_bw_cntl |= R300_RD_COMP_ENABLE;
        if (r300->z_decomp_rd == false)
            z->zb_bw_cntl |= R300_WR_COMP_ENABLE;
    }
    /* RV350 and up optimizations. */
    /* The section 10.4.9 in the docs is a lie. */
    if (r300->z_compression == RV350_Z_COMPRESS_88)
        z->gb_z_peq_config |= R300_GB_Z_PEQ_CONFIG_Z_PEQ_SIZE_8_8;

    if (hiz_in_use) {
        bool can_hiz = r300_can_hiz(r300);
        if (can_hiz) {
            z->zb_bw_cntl |= R300_HIZ_ENABLE;
            z->sc_hyperz |= R300_SC_HYPERZ_ENABLE;
            z->sc_hyperz |= r300_get_sc_hz_max(r300);
            z->zb_bw_cntl |= r300_get_hiz_min(r300);
        }
    }

    /* R500-specific features and optimizations. */
    if (r300->screen->caps.is_r500) {
        z->zb_bw_cntl |= R500_HIZ_FP_EXP_BITS_3;
        z->zb_bw_cntl |=
                R500_HIZ_EQUAL_REJECT_ENABLE |
                R500_PEQ_PACKING_ENABLE |
                R500_COVERED_PTR_MASKING_ENABLE;
    }
}

/*****************************************************************************/
/* The ZTOP state                                                            */
/*****************************************************************************/

static boolean r300_dsa_writes_stencil(
        struct pipe_stencil_state *s)
{
    return s->enabled && s->writemask &&
           (s->fail_op  != PIPE_STENCIL_OP_KEEP ||
            s->zfail_op != PIPE_STENCIL_OP_KEEP ||
            s->zpass_op != PIPE_STENCIL_OP_KEEP);
}

static boolean r300_dsa_writes_depth_stencil(
        struct pipe_depth_stencil_alpha_state *dsa)
{
    /* We are interested only in the cases when a depth or stencil value
     * can be changed. */

    if (dsa->depth.enabled && dsa->depth.writemask &&
        dsa->depth.func != PIPE_FUNC_NEVER)
        return TRUE;

    if (r300_dsa_writes_stencil(&dsa->stencil[0]) ||
        r300_dsa_writes_stencil(&dsa->stencil[1]))
        return TRUE;

    return FALSE;
}

static boolean r300_dsa_alpha_test_enabled(
        struct pipe_depth_stencil_alpha_state *dsa)
{
    /* We are interested only in the cases when alpha testing can kill
     * a fragment. */

    return dsa->alpha.enabled && dsa->alpha.func != PIPE_FUNC_ALWAYS;
}

static void r300_update_ztop(struct r300_context* r300)
{
    struct r300_ztop_state* ztop_state =
        (struct r300_ztop_state*)r300->ztop_state.state;
    uint32_t old_ztop = ztop_state->z_buffer_top;

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
           (r300_dsa_alpha_test_enabled(r300->dsa_state.state) ||  /* (1) */
            r300_fs(r300)->shader->info.uses_kill)) {              /* (2) */
        ztop_state->z_buffer_top = R300_ZTOP_DISABLE;
    } else if (r300_fragment_shader_writes_depth(r300_fs(r300))) { /* (5) */
        ztop_state->z_buffer_top = R300_ZTOP_DISABLE;
    } else if (r300->query_current) {                              /* (6) */
        ztop_state->z_buffer_top = R300_ZTOP_DISABLE;
    } else {
        ztop_state->z_buffer_top = R300_ZTOP_ENABLE;
    }
    if (ztop_state->z_buffer_top != old_ztop)
        r300->ztop_state.dirty = TRUE;
}

#define ALIGN_DIVUP(x, y) (((x) + (y) - 1) / (y))

static void r300_update_hiz_clear(struct r300_context *r300)
{
    struct pipe_framebuffer_state *fb =
        (struct pipe_framebuffer_state*)r300->fb_state.state;
    uint32_t height;

    height = ALIGN_DIVUP(fb->zsbuf->height, 4);
    r300->hiz_clear.size = height * 4;
}

static void r300_update_zmask_clear(struct r300_context *r300)
{
    struct pipe_framebuffer_state *fb =
        (struct pipe_framebuffer_state*)r300->fb_state.state;
    uint32_t height;
    int mult;

    if (r300->z_compression == RV350_Z_COMPRESS_88)
        mult = 8;
    else
        mult = 4;

    height = ALIGN_DIVUP(fb->zsbuf->height, mult);

    r300->zmask_clear.size = height * 4;
}

void r300_update_hyperz_state(struct r300_context* r300)
{
    r300_update_ztop(r300);
    if (r300->hyperz_state.dirty) {
        r300_update_hyperz(r300);
    }

    if (r300->hiz_clear.dirty) {
       r300_update_hiz_clear(r300);
    }
    if (r300->zmask_clear.dirty) {
       r300_update_zmask_clear(r300);
    }
}

void r300_hiz_alloc_block(struct r300_context *r300, struct r300_surface *surf)
{
    struct r300_texture *tex;
    uint32_t zsize, ndw;
    int level = surf->base.level;

    tex = r300_texture(surf->base.texture);

    if (tex->hiz_mem[level])
        return;

    zsize = tex->desc.layer_size_in_bytes[level];
    zsize /= util_format_get_blocksize(tex->desc.b.b.format);
    ndw = ALIGN_DIVUP(zsize, 64);

    tex->hiz_mem[level] = u_mmAllocMem(r300->hiz_mm, ndw, 0, 0);
    return;
}

void r300_zmask_alloc_block(struct r300_context *r300, struct r300_surface *surf, int compress)
{
    int bsize = 256;
    uint32_t zsize, ndw;
    int level = surf->base.level;
    struct r300_texture *tex;

    tex = r300_texture(surf->base.texture);

    /* We currently don't handle decompression for 3D textures and cubemaps
     * correctly. */
    if (tex->desc.b.b.target != PIPE_TEXTURE_1D &&
        tex->desc.b.b.target != PIPE_TEXTURE_2D &&
        tex->desc.b.b.target != PIPE_TEXTURE_RECT)
        return;

    /* Cannot flush zmask of 16-bit zbuffers. */
    if (util_format_get_blocksizebits(tex->desc.b.b.format) == 16)
        return;

    if (tex->zmask_mem[level])
        return;

    zsize = tex->desc.layer_size_in_bytes[level];
    zsize /= util_format_get_blocksize(tex->desc.b.b.format);

    /* each zmask dword represents 16 4x4 blocks - which is 256 pixels
       or 16 8x8 depending on the gb peq flag = 1024 pixels */
    if (compress == RV350_Z_COMPRESS_88)
        bsize = 1024;

    ndw = ALIGN_DIVUP(zsize, bsize);
    tex->zmask_mem[level] = u_mmAllocMem(r300->zmask_mm, ndw, 0, 0);
    return;
}

boolean r300_hyperz_init_mm(struct r300_context *r300)
{
    struct r300_screen* r300screen = r300->screen;
    int frag_pipes = r300screen->caps.num_frag_pipes;

    r300->zmask_mm = u_mmInit(0, r300screen->caps.zmask_ram * frag_pipes);
    if (!r300->zmask_mm)
      return FALSE;

    if (r300screen->caps.hiz_ram) {
      r300->hiz_mm = u_mmInit(0, r300screen->caps.hiz_ram * frag_pipes);
      if (!r300->hiz_mm) {
        u_mmDestroy(r300->zmask_mm);
        r300->zmask_mm = NULL;
        return FALSE;
      }
    }

    return TRUE;
}

void r300_hyperz_destroy_mm(struct r300_context *r300)
{
    struct r300_screen* r300screen = r300->screen;

    if (r300screen->caps.hiz_ram) {
      u_mmDestroy(r300->hiz_mm);
      r300->hiz_mm = NULL;
    }

    u_mmDestroy(r300->zmask_mm);
    r300->zmask_mm = NULL;
}
