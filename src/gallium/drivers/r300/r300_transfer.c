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

#include "r300_transfer.h"
#include "r300_texture_desc.h"
#include "r300_screen_buffer.h"

#include "util/u_memory.h"
#include "util/u_format.h"

struct r300_transfer {
    /* Parent class */
    struct pipe_transfer transfer;

    /* Offset from start of buffer. */
    unsigned offset;

    /* Linear texture. */
    struct r300_texture *linear_texture;
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
    struct pipe_transfer *transfer = (struct pipe_transfer*)r300transfer;
    struct pipe_resource *tex = transfer->resource;
    struct pipe_subresource subdst;

    subdst.face = 0;
    subdst.level = 0;

    ctx->resource_copy_region(ctx, &r300transfer->linear_texture->desc.b.b, subdst,
			      0, 0, 0,
			      tex, transfer->sr,
			      transfer->box.x, transfer->box.y, transfer->box.z,
			      transfer->box.width, transfer->box.height);
}

/* Copy a detiled texture to a tiled one. */
static void r300_copy_into_tiled_texture(struct pipe_context *ctx,
                                         struct r300_transfer *r300transfer)
{
    struct pipe_transfer *transfer = (struct pipe_transfer*)r300transfer;
    struct pipe_resource *tex = transfer->resource;
    struct pipe_subresource subsrc;

    subsrc.face = 0;
    subsrc.level = 0;

    ctx->resource_copy_region(ctx, tex, transfer->sr,
			      transfer->box.x, transfer->box.y, transfer->box.z,
                              &r300transfer->linear_texture->desc.b.b, subsrc,
			      0, 0, 0,
			      transfer->box.width, transfer->box.height);

    ctx->flush(ctx, 0, NULL);
}

struct pipe_transfer*
r300_texture_get_transfer(struct pipe_context *ctx,
			  struct pipe_resource *texture,
			  struct pipe_subresource sr,
			  unsigned usage,
			  const struct pipe_box *box)
{
    struct r300_context *r300 = r300_context(ctx);
    struct r300_texture *tex = r300_texture(texture);
    struct r300_transfer *trans;
    struct pipe_resource base;
    boolean referenced_cs, referenced_hw, blittable;

    referenced_cs =
        r300->rws->cs_is_buffer_referenced(r300->cs,
                                           tex->buffer, R300_REF_CS);
    if (referenced_cs) {
        referenced_hw = TRUE;
    } else {
        referenced_hw =
            r300->rws->cs_is_buffer_referenced(r300->cs,
                                               tex->buffer, R300_REF_HW);
    }

    blittable = ctx->screen->is_format_supported(
            ctx->screen, texture->format, texture->target, 0,
            PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET, 0);

    trans = CALLOC_STRUCT(r300_transfer);
    if (trans) {
        /* Initialize the transfer object. */
        pipe_resource_reference(&trans->transfer.resource, texture);
        trans->transfer.sr = sr;
        trans->transfer.usage = usage;
        trans->transfer.box = *box;

        /* If the texture is tiled, we must create a temporary detiled texture
         * for this transfer.
         * Also make write transfers pipelined. */
        if (tex->desc.microtile || tex->desc.macrotile[sr.level] ||
            ((referenced_hw & !(usage & PIPE_TRANSFER_READ)) && blittable)) {
            base.target = PIPE_TEXTURE_2D;
            base.format = texture->format;
            base.width0 = box->width;
            base.height0 = box->height;
            base.depth0 = 0;
            base.last_level = 0;
            base.nr_samples = 0;
            base.usage = PIPE_USAGE_DYNAMIC;
            base.bind = 0;
	    base.flags = R300_RESOURCE_FLAG_TRANSFER;

            /* For texture reading, the temporary (detiled) texture is used as
             * a render target when blitting from a tiled texture. */
            if (usage & PIPE_TRANSFER_READ) {
                base.bind |= PIPE_BIND_RENDER_TARGET;
            }
            /* For texture writing, the temporary texture is used as a sampler
             * when blitting into a tiled texture. */
            if (usage & PIPE_TRANSFER_WRITE) {
                base.bind |= PIPE_BIND_SAMPLER_VIEW;
            }

            /* Create the temporary texture. */
            trans->linear_texture = r300_texture(
               ctx->screen->resource_create(ctx->screen,
                                            &base));

            if (!trans->linear_texture) {
                /* Oh crap, the thing can't create the texture.
                 * Let's flush and try again. */
                ctx->flush(ctx, 0, NULL);

                trans->linear_texture = r300_texture(
                   ctx->screen->resource_create(ctx->screen,
                                                &base));

                if (!trans->linear_texture) {
                    /* For linear textures, it's safe to fallback to
                     * an unpipelined transfer. */
                    if (!tex->desc.microtile && !tex->desc.macrotile[sr.level]) {
                        goto unpipelined;
                    }

                    /* Otherwise, go to hell. */
                    fprintf(stderr,
                        "r300: Failed to create a transfer object, praise.\n");
                    FREE(trans);
                    return NULL;
                }
            }

            assert(!trans->linear_texture->desc.microtile &&
                   !trans->linear_texture->desc.macrotile[0]);

            /* Set the stride.
	     *
	     * Even though we are using an internal texture for this,
	     * the transfer sr, box and usage parameters still reflect
	     * the arguments received to get_transfer.  We just do the
	     * right thing internally.
	     */
            trans->transfer.stride =
                    trans->linear_texture->desc.stride_in_bytes[0];

            if (usage & PIPE_TRANSFER_READ) {
                /* We cannot map a tiled texture directly because the data is
                 * in a different order, therefore we do detiling using a blit. */
                r300_copy_from_tiled_texture(ctx, trans);

                /* Always referenced in the blit. */
                ctx->flush(ctx, 0, NULL);
            }
            return &trans->transfer;
        }

    unpipelined:
        /* Unpipelined transfer. */
        trans->transfer.stride = tex->desc.stride_in_bytes[sr.level];
        trans->offset = r300_texture_get_offset(&tex->desc,
                                                sr.level, box->z, sr.face);

        if (referenced_cs)
            ctx->flush(ctx, PIPE_FLUSH_RENDER_CACHE, NULL);
        return &trans->transfer;
    }
    return NULL;
}

void r300_texture_transfer_destroy(struct pipe_context *ctx,
				   struct pipe_transfer *trans)
{
    struct r300_transfer *r300transfer = r300_transfer(trans);

    if (r300transfer->linear_texture) {
        if (trans->usage & PIPE_TRANSFER_WRITE) {
            r300_copy_into_tiled_texture(ctx, r300transfer);
        }

        pipe_resource_reference(
            (struct pipe_resource**)&r300transfer->linear_texture, NULL);
    }
    pipe_resource_reference(&trans->resource, NULL);
    FREE(trans);
}

void* r300_texture_transfer_map(struct pipe_context *ctx,
				struct pipe_transfer *transfer)
{
    struct r300_context *r300 = r300_context(ctx);
    struct r300_winsys_screen *rws = (struct r300_winsys_screen *)ctx->winsys;
    struct r300_transfer *r300transfer = r300_transfer(transfer);
    struct r300_texture *tex = r300_texture(transfer->resource);
    char *map;
    enum pipe_format format = tex->desc.b.b.format;

    if (r300transfer->linear_texture) {
        /* The detiled texture is of the same size as the region being mapped
         * (no offset needed). */
        return rws->buffer_map(rws,
                               r300transfer->linear_texture->buffer,
                               r300->cs,
                               transfer->usage);
    } else {
        /* Tiling is disabled. */
        map = rws->buffer_map(rws, tex->buffer, r300->cs,
                              transfer->usage);

        if (!map) {
            return NULL;
        }

        return map + r300_transfer(transfer)->offset +
            transfer->box.y / util_format_get_blockheight(format) * transfer->stride +
            transfer->box.x / util_format_get_blockwidth(format) * util_format_get_blocksize(format);
    }
}

void r300_texture_transfer_unmap(struct pipe_context *ctx,
				 struct pipe_transfer *transfer)
{
    struct r300_winsys_screen *rws = (struct r300_winsys_screen *)ctx->winsys;
    struct r300_transfer *r300transfer = r300_transfer(transfer);
    struct r300_texture *tex = r300_texture(transfer->resource);

    if (r300transfer->linear_texture) {
        rws->buffer_unmap(rws, r300transfer->linear_texture->buffer);
    } else {
        rws->buffer_unmap(rws, tex->buffer);
    }
}
