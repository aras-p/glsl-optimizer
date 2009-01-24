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

static int minify(int i)
{
    return MAX2(1, i >> 1);
}

static void r300_setup_miptree(struct r300_texture* tex)
{
    struct pipe_texture* base = &tex->tex;
    int stride, size, offset;

    for (int i = 0; i <= base->last_level; i++) {
        if (i > 0) {
            base->width[i] = minify(base->width[i-1]);
            base->height[i] = minify(base->height[i-1]);
            base->depth[i] = minify(base->depth[i-1]);
        }

        base->nblocksx[i] = pf_get_nblocksx(&base->block, base->width[i]);
        base->nblocksy[i] = pf_get_nblocksy(&base->block, base->width[i]);

        /* Radeons enjoy things in multiples of 32. */
        /* XXX NPOT -> 64, not 32 */
        stride = (base->nblocksx[i] * base->block.size + 31) & ~31;
        size = stride * base->nblocksy[i] * base->depth[i];

        /* XXX 64 for NPOT */
        tex->offset[i] = (tex->size + 31) & ~31;
        tex->size = tex->offset[i] + size;
    }
}

/* Create a new texture. */
static struct pipe_texture*
    r300_texture_create(struct pipe_screen* screen,
                        const struct pipe_texture* template)
{
    struct r300_screen* r300screen = r300_screen(screen);

    struct r300_texture* tex = CALLOC_STRUCT(r300_texture);

    if (!tex) {
        return NULL;
    }

    tex->tex = *template;
    tex->tex.refcount = 1;
    tex->tex.screen = screen;

    r300_setup_miptree(tex);

    tex->buffer = screen->winsys->buffer_create(screen->winsys, 32,
                                                PIPE_BUFFER_USAGE_PIXEL,
                                                tex->size);

    if (!tex->buffer) {
        FREE(tex);
        return NULL;
    }

    return (struct pipe_texture*)tex;
}

static void r300_texture_release(struct pipe_screen* screen,
                                 struct pipe_texture** texture)
{
    if (!*texture) {
        return;
    }

    (*texture)->refcount--;

    if ((*texture)->refcount <= 0) {
        struct r300_texture* tex = (struct r300_texture*)*texture;

        pipe_buffer_reference(screen, &tex->buffer, NULL);

        FREE(tex);
    }

    *texture = NULL;
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
        surface->refcount = 1;
        surface->winsys = screen->winsys;
        pipe_texture_reference(&surface->texture, texture);
        pipe_buffer_reference(screen, &surface->buffer, tex->buffer);
        surface->format = texture->format;
        surface->width = texture->width[level];
        surface->height = texture->height[level];
        surface->block = texture->block;
        surface->nblocksx = texture->nblocksx[level];
        surface->nblocksy = texture->nblocksy[level];
        /* XXX save the actual stride instead plz kthnxbai */
        surface->stride =
            (texture->nblocksx[level] * texture->block.size + 31) & ~31;
        surface->offset = offset;
        surface->usage = flags;
        surface->status = PIPE_SURFACE_STATUS_DEFINED;
    }

    return surface;
}

static void r300_tex_surface_release(struct pipe_screen* screen,
                                     struct pipe_surface** surface)
{
    struct pipe_surface* s = *surface;

    s->refcount--;

    if (s->refcount <= 0) {
        pipe_texture_reference(&s->texture, NULL);
        pipe_buffer_reference(screen, &s->buffer, NULL);
        FREE(s);
    }

    *surface = NULL;
}

static struct pipe_texture*
    r300_texture_blanket(struct pipe_screen* screen,
                         const struct pipe_texture* base,
                         const unsigned* stride,
                         struct pipe_buffer* buffer)
{
    struct r300_texture* tex;

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
    tex->tex.refcount = 1;
    tex->tex.screen = screen;

    /* XXX tex->stride = *stride; */

    pipe_buffer_reference(screen, &tex->buffer, buffer);

    return (struct pipe_texture*)tex;
}

void r300_init_screen_texture_functions(struct pipe_screen* screen)
{
    screen->texture_create = r300_texture_create;
    screen->texture_release = r300_texture_release;
    screen->get_tex_surface = r300_get_tex_surface;
    screen->tex_surface_release = r300_tex_surface_release;
    screen->texture_blanket = r300_texture_blanket;
}
