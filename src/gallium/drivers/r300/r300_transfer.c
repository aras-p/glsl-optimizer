/*
 * Copyright 2008 Corbin Simpson <MostAwesomeDude@gmail.com>
 * Copyright 2010 Marek Olšák <maraeo@gmail.com>
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
#include "r300_transfer.h"
#include "r300_texture.h"
#include "r300_screen.h"

#include "util/u_memory.h"
#include "util/u_format.h"

struct r300_transfer {
    /* Parent class */
    struct pipe_transfer transfer;

    /* Pipe context. */
    struct pipe_context *ctx;

    /* Parameters of get_tex_transfer. */
    unsigned x, y, level, zslice, face;

    /* Offset from start of buffer. */
    unsigned offset;

    /* Detiled texture. */
    struct r300_texture *detiled_texture;

    /* Transfer and format flags. */
    unsigned buffer_usage, render_target_usage;
};

/* Convenience cast wrapper. */
static INLINE struct r300_transfer*
r300_transfer(struct pipe_transfer* transfer)
{
    return (struct r300_transfer*)transfer;
}

/* Copy from a tiled texture to a detiled one. */
static void r300_copy_from_tiled_texture(struct pipe_context *ctx,
                                         struct r300_transfer *r300transfer)
{
    struct pipe_screen *screen = ctx->screen;
    struct pipe_transfer *transfer = (struct pipe_transfer*)r300transfer;
    struct pipe_texture *tex = transfer->texture;
    struct pipe_surface *src, *dst;

    src = screen->get_tex_surface(screen, tex, r300transfer->face,
                                  r300transfer->level, r300transfer->zslice,
                                  PIPE_BUFFER_USAGE_GPU_READ |
                                  PIPE_BUFFER_USAGE_PIXEL);

    dst = screen->get_tex_surface(screen, &r300transfer->detiled_texture->tex,
                                  0, 0, 0,
                                  PIPE_BUFFER_USAGE_GPU_WRITE |
                                  PIPE_BUFFER_USAGE_PIXEL |
                                  r300transfer->buffer_usage);

    ctx->surface_copy(ctx, dst, 0, 0, src, r300transfer->x, r300transfer->y,
                      transfer->width, transfer->height);

    pipe_surface_reference(&src, NULL);
    pipe_surface_reference(&dst, NULL);
}

/* Copy a detiled texture to a tiled one. */
static void r300_copy_into_tiled_texture(struct pipe_context *ctx,
                                         struct r300_transfer *r300transfer)
{
    struct pipe_screen *screen = ctx->screen;
    struct pipe_transfer *transfer = (struct pipe_transfer*)r300transfer;
    struct pipe_texture *tex = transfer->texture;
    struct pipe_surface *src, *dst;

    src = screen->get_tex_surface(screen, &r300transfer->detiled_texture->tex,
                                  0, 0, 0,
                                  PIPE_BUFFER_USAGE_GPU_READ |
                                  PIPE_BUFFER_USAGE_PIXEL);

    dst = screen->get_tex_surface(screen, tex, r300transfer->face,
                                  r300transfer->level, r300transfer->zslice,
                                  PIPE_BUFFER_USAGE_GPU_WRITE |
                                  PIPE_BUFFER_USAGE_PIXEL);

    /* XXX this flush prevents the following DRM error from occuring:
     * [drm:radeon_cs_ioctl] *ERROR* Failed to parse relocation !
     * Reproducible with perf/copytex. */
    ctx->flush(ctx, 0, NULL);

    ctx->surface_copy(ctx, dst, r300transfer->x, r300transfer->y, src, 0, 0,
                      transfer->width, transfer->height);

    /* XXX this flush fixes a few piglit tests (e.g. glean/pixelFormats). */
    ctx->flush(ctx, 0, NULL);

    pipe_surface_reference(&src, NULL);
    pipe_surface_reference(&dst, NULL);
}

static struct pipe_transfer*
r300_get_tex_transfer(struct pipe_context *ctx,
                      struct pipe_texture *texture,
                      unsigned face, unsigned level, unsigned zslice,
                      enum pipe_transfer_usage usage, unsigned x, unsigned y,
                      unsigned w, unsigned h)
{
    struct r300_texture *tex = (struct r300_texture *)texture;
    struct r300_screen *r300screen = r300_screen(ctx->screen);
    struct r300_transfer *trans;
    struct pipe_texture template;

    trans = CALLOC_STRUCT(r300_transfer);
    if (trans) {
        /* Initialize the transfer object. */
        pipe_texture_reference(&trans->transfer.texture, texture);
        trans->transfer.usage = usage;
        trans->transfer.width = w;
        trans->transfer.height = h;
        trans->ctx = ctx;
        trans->x = x;
        trans->y = y;
        trans->level = level;
        trans->zslice = zslice;
        trans->face = face;

        /* If the texture is tiled, we must create a temporary detiled texture
         * for this transfer. */
        if (tex->microtile || tex->macrotile) {
            trans->buffer_usage = pipe_transfer_buffer_flags(&trans->transfer);
            trans->render_target_usage =
                util_format_is_depth_or_stencil(texture->format) ?
                PIPE_TEXTURE_USAGE_DEPTH_STENCIL :
                PIPE_TEXTURE_USAGE_RENDER_TARGET;

            template.target = PIPE_TEXTURE_2D;
            template.format = texture->format;
            template.width0 = w;
            template.height0 = h;
            template.depth0 = 0;
            template.last_level = 0;
            template.nr_samples = 0;
            template.tex_usage = PIPE_TEXTURE_USAGE_DYNAMIC |
                                 R300_TEXTURE_USAGE_TRANSFER;

            /* For texture reading, the temporary (detiled) texture is used as
             * a render target when blitting from a tiled texture. */
            if (usage & PIPE_TRANSFER_READ) {
                template.tex_usage |= trans->render_target_usage;
            }
            /* For texture writing, the temporary texture is used as a sampler
             * when blitting into a tiled texture. */
            if (usage & PIPE_TRANSFER_WRITE) {
                template.tex_usage |= PIPE_TEXTURE_USAGE_SAMPLER;
            }

            /* Create the temporary texture. */
            trans->detiled_texture = (struct r300_texture*)
               ctx->screen->texture_create(ctx->screen,
                                           &template);

            assert(!trans->detiled_texture->microtile &&
                   !trans->detiled_texture->macrotile);

            /* Set the stride.
             * Parameters x, y, level, zslice, and face remain zero. */
            trans->transfer.stride =
                r300_texture_get_stride(r300screen, trans->detiled_texture, 0);

            if (usage & PIPE_TRANSFER_READ) {
                /* We cannot map a tiled texture directly because the data is
                 * in a different order, therefore we do detiling using a blit. */
                r300_copy_from_tiled_texture(ctx, trans);
            }
        } else {
            trans->transfer.x = x;
            trans->transfer.y = y;
            trans->transfer.stride =
                r300_texture_get_stride(r300screen, tex, level);
            trans->transfer.level = level;
            trans->transfer.zslice = zslice;
            trans->transfer.face = face;
            trans->offset = r300_texture_get_offset(tex, level, zslice, face);
        }
    }
    return &trans->transfer;
}

static void r300_tex_transfer_destroy(struct pipe_context *ctx,
                                      struct pipe_transfer *trans)
{
    struct r300_transfer *r300transfer = r300_transfer(trans);

    if (r300transfer->detiled_texture) {
        if (trans->usage & PIPE_TRANSFER_WRITE) {
            r300_copy_into_tiled_texture(r300transfer->ctx, r300transfer);
        }

        pipe_texture_reference(
            (struct pipe_texture**)&r300transfer->detiled_texture, NULL);
    }
    pipe_texture_reference(&trans->texture, NULL);
    FREE(trans);
}

static void* r300_transfer_map(struct pipe_context *ctx,
                               struct pipe_transfer *transfer)
{
    struct r300_transfer *r300transfer = r300_transfer(transfer);
    struct r300_texture *tex = (struct r300_texture*)transfer->texture;
    char *map;
    enum pipe_format format = tex->tex.format;

    if (r300transfer->detiled_texture) {
        /* The detiled texture is of the same size as the region being mapped
         * (no offset needed). */
        return pipe_buffer_map(ctx->screen,
                               r300transfer->detiled_texture->buffer,
                               pipe_transfer_buffer_flags(transfer));
    } else {
        /* Tiling is disabled. */
        map = pipe_buffer_map(ctx->screen, tex->buffer,
                              pipe_transfer_buffer_flags(transfer));

        if (!map) {
            return NULL;
        }

        return map + r300_transfer(transfer)->offset +
            transfer->y / util_format_get_blockheight(format) * transfer->stride +
            transfer->x / util_format_get_blockwidth(format) * util_format_get_blocksize(format);
    }
}

static void r300_transfer_unmap(struct pipe_context *ctx,
                                struct pipe_transfer *transfer)
{
    struct r300_transfer *r300transfer = r300_transfer(transfer);
    struct r300_texture *tex = (struct r300_texture*)transfer->texture;

    if (r300transfer->detiled_texture) {
        pipe_buffer_unmap(ctx->screen, r300transfer->detiled_texture->buffer);
    } else {
        pipe_buffer_unmap(ctx->screen, tex->buffer);
    }
}


void r300_init_transfer_functions( struct r300_context *r300ctx )
{
   struct pipe_context *ctx = &r300ctx->context;

   ctx->get_tex_transfer = r300_get_tex_transfer;
   ctx->tex_transfer_destroy = r300_tex_transfer_destroy;
   ctx->transfer_map = r300_transfer_map;
   ctx->transfer_unmap = r300_transfer_unmap;
}
