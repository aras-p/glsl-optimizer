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

#include "r300_texture.h"

static void r300_setup_texture_state(struct r300_texture* tex,
                                     unsigned width,
                                     unsigned height,
                                     unsigned pitch,
                                     unsigned levels)
{
    struct r300_texture_state* state = &tex->state;

    state->format0 = R300_TX_WIDTH((width - 1) & 0x7ff) |
        R300_TX_HEIGHT((height - 1) & 0x7ff) |
        R300_TX_NUM_LEVELS(levels) |
        R300_TX_PITCH_EN;

    /* XXX */
    state->format1 = r300_translate_texformat(tex->tex.format);

    state->format2 = pitch - 1;

    /* Assume (somewhat foolishly) that oversized textures will
     * not be permitted by the state tracker. */
    if (width > 2048) {
        state->format2 |= R500_TXWIDTH_BIT11;
    }
    if (height > 2048) {
        state->format2 |= R500_TXHEIGHT_BIT11;
    }

    debug_printf("r300: Set texture state (%dx%d, pitch %d, %d levels)\n",
            width, height, pitch, levels);
}

static void r300_setup_miptree(struct r300_texture* tex)
{
    struct pipe_texture* base = &tex->tex;
    int stride, size, offset;
    int i;

    for (i = 0; i <= base->last_level; i++) {
        if (i > 0) {
            base->width[i] = minify(base->width[i-1]);
            base->height[i] = minify(base->height[i-1]);
            base->depth[i] = minify(base->depth[i-1]);
        }

        base->nblocksx[i] = pf_get_nblocksx(&base->block, base->width[i]);
        base->nblocksy[i] = pf_get_nblocksy(&base->block, base->height[i]);

        /* Radeons enjoy things in multiples of 64.
         *
         * XXX
         * POT, uncompressed, unmippmapped textures can be aligned to 32,
         * instead of 64. */
        stride = align(pf_get_stride(&base->block, base->width[i]), 32);
        size = stride * base->nblocksy[i] * base->depth[i];

        tex->offset[i] = align(tex->size, 32);
        tex->size = tex->offset[i] + size;

        debug_printf("r300: Texture miptree: Level %d "
                "(%dx%dx%d px, pitch %d bytes)\n",
                i, base->width[i], base->height[i], base->depth[i],
                stride);
        /* Save stride of first level to the texture. */
        if (i == 0) {
            tex->stride = stride;
        }
    }
}

/* Create a new texture. */
static struct pipe_texture*
    r300_texture_create(struct pipe_screen* screen,
                        const struct pipe_texture* template)
{
    struct r300_texture* tex = CALLOC_STRUCT(r300_texture);

    if (!tex) {
        return NULL;
    }

    tex->tex = *template;
    pipe_reference_init(&tex->tex.reference, 1);
    tex->tex.screen = screen;

    r300_setup_miptree(tex);

    r300_setup_texture_state(tex, template->width[0], template->height[0],
            template->width[0], template->last_level);

    tex->buffer = screen->buffer_create(screen, 1024,
                                        PIPE_BUFFER_USAGE_PIXEL,
                                        tex->size);

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

    /* XXX this is certainly dependent on tex target */
    offset = tex->offset[level];

    if (surface) {
        pipe_reference_init(&surface->reference, 1);
        pipe_texture_reference(&surface->texture, texture);
        surface->format = texture->format;
        surface->width = texture->width[level];
        surface->height = texture->height[level];
        surface->offset = offset;
        surface->usage = flags;
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

    /* XXX we should start doing mips now... */
    if (base->target != PIPE_TEXTURE_2D ||
        base->last_level != 0 ||
        base->depth[0] != 1) {
        return NULL;
    }

    tex = CALLOC_STRUCT(r300_texture);
    if (!tex) {
        return NULL;
    }

    tex->tex = *base;
    pipe_reference_init(&tex->tex.reference, 1);
    tex->tex.screen = screen;

    tex->stride = *stride;

    /* XXX */
    r300_setup_texture_state(tex, tex->tex.width[0], tex->tex.height[0],
            tex->stride, 0);

    pipe_buffer_reference(&tex->buffer, buffer);

    return (struct pipe_texture*)tex;
}

void r300_init_screen_texture_functions(struct pipe_screen* screen)
{
    screen->texture_create = r300_texture_create;
    screen->texture_destroy = r300_texture_destroy;
    screen->get_tex_surface = r300_get_tex_surface;
    screen->tex_surface_destroy = r300_tex_surface_destroy;
    screen->texture_blanket = r300_texture_blanket;
}

boolean r300_get_texture_buffer(struct pipe_texture* texture,
                                struct pipe_buffer** buffer,
                                unsigned* stride)
{
    struct r300_texture* tex = (struct r300_texture*)texture;
    if (!tex) {
        return FALSE;
    }

    pipe_buffer_reference(buffer, tex->buffer);

    if (stride) {
        *stride = tex->stride;
    }

    return TRUE;
}
