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

#include "pipe/p_screen.h"

#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#include "r300_context.h"
#include "r300_texture.h"
#include "r300_screen.h"

#include "radeon_winsys.h"

#define TILE_WIDTH 0
#define TILE_HEIGHT 1

static const unsigned microblock_table[5][3][2] = {
    /*linear  tiled   square-tiled */
    {{32, 1}, {8, 4}, {0, 0}}, /*   8 bits per pixel */
    {{16, 1}, {8, 2}, {4, 4}}, /*  16 bits per pixel */
    {{ 8, 1}, {4, 2}, {0, 0}}, /*  32 bits per pixel */
    {{ 4, 1}, {0, 0}, {2, 2}}, /*  64 bits per pixel */
    {{ 2, 1}, {0, 0}, {0, 0}}  /* 128 bits per pixel */
};

static void r300_setup_texture_state(struct r300_screen* screen, struct r300_texture* tex)
{
    struct r300_texture_state* state = &tex->state;
    struct pipe_texture *pt = &tex->tex;
    boolean is_r500 = screen->caps->is_r500;

    state->format0 = R300_TX_WIDTH((pt->width0 - 1) & 0x7ff) |
                     R300_TX_HEIGHT((pt->height0 - 1) & 0x7ff);

    if (tex->is_npot) {
        /* rectangles love this */
        state->format0 |= R300_TX_PITCH_EN;
        state->format2 = (tex->pitch[0] - 1) & 0x1fff;
    } else {
        /* power of two textures (3D, mipmaps, and no pitch) */
        state->format0 |= R300_TX_DEPTH(util_logbase2(pt->depth0) & 0xf);
    }

    state->format1 = r300_translate_texformat(pt->format);
    if (pt->target == PIPE_TEXTURE_CUBE) {
        state->format1 |= R300_TX_FORMAT_CUBIC_MAP;
    }
    if (pt->target == PIPE_TEXTURE_3D) {
        state->format1 |= R300_TX_FORMAT_3D;
    }

    /* large textures on r500 */
    if (is_r500)
    {
        if (pt->width0 > 2048) {
            state->format2 |= R500_TXWIDTH_BIT11;
        }
        if (pt->height0 > 2048) {
            state->format2 |= R500_TXHEIGHT_BIT11;
        }
    }
    assert(is_r500 || (pt->width0 <= 2048 && pt->height0 <= 2048));

    SCREEN_DBG(screen, DBG_TEX, "r300: Set texture state (%dx%d, %d levels)\n",
               pt->width0, pt->height0, pt->last_level);
}

unsigned r300_texture_get_offset(struct r300_texture* tex, unsigned level,
                                 unsigned zslice, unsigned face)
{
    unsigned offset = tex->offset[level];

    switch (tex->tex.target) {
        case PIPE_TEXTURE_3D:
            assert(face == 0);
            return offset + zslice * tex->layer_size[level];

        case PIPE_TEXTURE_CUBE:
            assert(zslice == 0);
            return offset + face * tex->layer_size[level];

        default:
            assert(zslice == 0 && face == 0);
            return offset;
    }
}

/**
 * Return the width (dim==TILE_WIDTH) or height (dim==TILE_HEIGHT) of one tile
 * of the given texture.
 */
static unsigned r300_texture_get_tile_size(struct r300_texture* tex, int dim)
{
    unsigned pixsize, tile_size;

    pixsize = util_format_get_blocksize(tex->tex.format);
    tile_size = microblock_table[util_logbase2(pixsize)][tex->microtile][dim] *
                (tex->macrotile == R300_BUFFER_TILED ? 8 : 1);

    assert(tile_size);
    return tile_size;
}

/**
 * Return the stride, in bytes, of the texture images of the given texture
 * at the given level.
 */
unsigned r300_texture_get_stride(struct r300_screen* screen,
                                 struct r300_texture* tex, unsigned level)
{
    unsigned tile_width, width;

    if (tex->stride_override)
        return tex->stride_override;

    /* Check the level. */
    if (level > tex->tex.last_level) {
        SCREEN_DBG(screen, DBG_TEX, "%s: level (%u) > last_level (%u)\n",
                   __FUNCTION__, level, tex->tex.last_level);
        return 0;
    }

    width = u_minify(tex->tex.width0, level);

    if (!util_format_is_compressed(tex->tex.format)) {
        tile_width = r300_texture_get_tile_size(tex, TILE_WIDTH);
        width = align(width, tile_width);
        return util_format_get_stride(tex->tex.format, width);
    } else {
        return align(util_format_get_stride(tex->tex.format, width), 32);
    }
}

static unsigned r300_texture_get_nblocksy(struct r300_texture* tex,
                                          unsigned level)
{
    unsigned height, tile_height;

    height = u_minify(tex->tex.height0, level);

    if (!util_format_is_compressed(tex->tex.format)) {
        tile_height = r300_texture_get_tile_size(tex, TILE_HEIGHT);
        height = align(height, tile_height);
    }

    return util_format_get_nblocksy(tex->tex.format, height);
}

static void r300_setup_miptree(struct r300_screen* screen,
                               struct r300_texture* tex)
{
    struct pipe_texture* base = &tex->tex;
    unsigned stride, size, layer_size, nblocksy, i;

    SCREEN_DBG(screen, DBG_TEX, "r300: Making miptree for texture, format %s\n",
               pf_name(base->format));

    for (i = 0; i <= base->last_level; i++) {
        stride = r300_texture_get_stride(screen, tex, i);
        nblocksy = r300_texture_get_nblocksy(tex, i);
        layer_size = stride * nblocksy;

        if (base->target == PIPE_TEXTURE_CUBE)
            size = layer_size * 6;
        else
            size = layer_size * u_minify(base->depth0, i);

        tex->offset[i] = align(tex->size, 32);
        tex->size = tex->offset[i] + size;
        tex->layer_size[i] = layer_size;
        tex->pitch[i] = stride / util_format_get_blocksize(base->format);

        SCREEN_DBG(screen, DBG_TEX, "r300: Texture miptree: Level %d "
                "(%dx%dx%d px, pitch %d bytes) %d bytes total\n",
                i, u_minify(base->width0, i), u_minify(base->height0, i),
                u_minify(base->depth0, i), stride, tex->size);
    }
}

static void r300_setup_flags(struct r300_texture* tex)
{
    tex->is_npot = !util_is_power_of_two(tex->tex.width0) ||
                   !util_is_power_of_two(tex->tex.height0);
}

/* Create a new texture. */
static struct pipe_texture*
    r300_texture_create(struct pipe_screen* screen,
                        const struct pipe_texture* template)
{
    struct r300_texture* tex = CALLOC_STRUCT(r300_texture);
    struct r300_screen* rscreen = r300_screen(screen);
    struct radeon_winsys* winsys = (struct radeon_winsys*)screen->winsys;

    if (!tex) {
        return NULL;
    }

    tex->tex = *template;
    pipe_reference_init(&tex->tex.reference, 1);
    tex->tex.screen = screen;

    r300_setup_flags(tex);
    r300_setup_miptree(rscreen, tex);
    r300_setup_texture_state(rscreen, tex);

    tex->buffer = screen->buffer_create(screen, 2048,
                                        PIPE_BUFFER_USAGE_PIXEL,
                                        tex->size);
    winsys->buffer_set_tiling(winsys, tex->buffer,
                              tex->pitch[0],
                              tex->microtile != R300_BUFFER_LINEAR,
                              tex->macrotile != R300_BUFFER_LINEAR);

    if (!tex->buffer) {
        FREE(tex);
        return NULL;
    }

    return (struct pipe_texture*)tex;
}

static void r300_texture_destroy(struct pipe_texture* texture)
{
    struct r300_texture* tex = (struct r300_texture*)texture;

    pipe_buffer_reference(&tex->buffer, NULL);

    FREE(tex);
}

static struct pipe_surface* r300_get_tex_surface(struct pipe_screen* screen,
                                                 struct pipe_texture* texture,
                                                 unsigned face,
                                                 unsigned level,
                                                 unsigned zslice,
                                                 unsigned flags)
{
    struct r300_texture* tex = (struct r300_texture*)texture;
    struct pipe_surface* surface = CALLOC_STRUCT(pipe_surface);
    unsigned offset;

    offset = r300_texture_get_offset(tex, level, zslice, face);

    if (surface) {
        pipe_reference_init(&surface->reference, 1);
        pipe_texture_reference(&surface->texture, texture);
        surface->format = texture->format;
        surface->width = u_minify(texture->width0, level);
        surface->height = u_minify(texture->height0, level);
        surface->offset = offset;
        surface->usage = flags;
        surface->zslice = zslice;
        surface->texture = texture;
        surface->face = face;
        surface->level = level;
    }

    return surface;
}

static void r300_tex_surface_destroy(struct pipe_surface* s)
{
    pipe_texture_reference(&s->texture, NULL);
    FREE(s);
}

static struct pipe_texture*
    r300_texture_blanket(struct pipe_screen* screen,
                         const struct pipe_texture* base,
                         const unsigned* stride,
                         struct pipe_buffer* buffer)
{
    struct r300_texture* tex;
    struct r300_screen* rscreen = r300_screen(screen);

    /* Support only 2D textures without mipmaps */
    if (base->target != PIPE_TEXTURE_2D ||
        base->depth0 != 1 ||
        base->last_level != 0) {
        return NULL;
    }

    tex = CALLOC_STRUCT(r300_texture);
    if (!tex) {
        return NULL;
    }

    tex->tex = *base;
    pipe_reference_init(&tex->tex.reference, 1);
    tex->tex.screen = screen;

    tex->stride_override = *stride;
    tex->pitch[0] = *stride / util_format_get_blocksize(base->format);

    r300_setup_flags(tex);
    r300_setup_texture_state(rscreen, tex);

    pipe_buffer_reference(&tex->buffer, buffer);

    return (struct pipe_texture*)tex;
}

static struct pipe_video_surface *
r300_video_surface_create(struct pipe_screen *screen,
                          enum pipe_video_chroma_format chroma_format,
                          unsigned width, unsigned height)
{
    struct r300_video_surface *r300_vsfc;
    struct pipe_texture template;

    assert(screen);
    assert(width && height);

    r300_vsfc = CALLOC_STRUCT(r300_video_surface);
    if (!r300_vsfc)
       return NULL;

    pipe_reference_init(&r300_vsfc->base.reference, 1);
    r300_vsfc->base.screen = screen;
    r300_vsfc->base.chroma_format = chroma_format;
    r300_vsfc->base.width = width;
    r300_vsfc->base.height = height;

    memset(&template, 0, sizeof(struct pipe_texture));
    template.target = PIPE_TEXTURE_2D;
    template.format = PIPE_FORMAT_X8R8G8B8_UNORM;
    template.last_level = 0;
    template.width0 = util_next_power_of_two(width);
    template.height0 = util_next_power_of_two(height);
    template.depth0 = 1;
    template.tex_usage = PIPE_TEXTURE_USAGE_SAMPLER |
                         PIPE_TEXTURE_USAGE_RENDER_TARGET;

    r300_vsfc->tex = screen->texture_create(screen, &template);
    if (!r300_vsfc->tex)
    {
        FREE(r300_vsfc);
        return NULL;
    }

    return &r300_vsfc->base;
}

static void r300_video_surface_destroy(struct pipe_video_surface *vsfc)
{
    struct r300_video_surface *r300_vsfc = r300_video_surface(vsfc);
    pipe_texture_reference(&r300_vsfc->tex, NULL);
    FREE(r300_vsfc);
}

void r300_init_screen_texture_functions(struct pipe_screen* screen)
{
    screen->texture_create = r300_texture_create;
    screen->texture_destroy = r300_texture_destroy;
    screen->get_tex_surface = r300_get_tex_surface;
    screen->tex_surface_destroy = r300_tex_surface_destroy;
    screen->texture_blanket = r300_texture_blanket;

    screen->video_surface_create = r300_video_surface_create;
    screen->video_surface_destroy= r300_video_surface_destroy;
}

boolean r300_get_texture_buffer(struct pipe_screen* screen,
                                struct pipe_texture* texture,
                                struct pipe_buffer** buffer,
                                unsigned* stride)
{
    struct r300_texture* tex = (struct r300_texture*)texture;
    if (!tex) {
        return FALSE;
    }

    pipe_buffer_reference(buffer, tex->buffer);

    if (stride) {
        *stride = r300_texture_get_stride(r300_screen(screen), tex, 0);
    }

    return TRUE;
}
