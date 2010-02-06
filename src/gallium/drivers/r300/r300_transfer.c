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

static struct pipe_transfer*
r300_get_tex_transfer(struct pipe_screen *screen,
                      struct pipe_texture *texture,
                      unsigned face, unsigned level, unsigned zslice,
                      enum pipe_transfer_usage usage, unsigned x, unsigned y,
                      unsigned w, unsigned h)
{
    struct r300_texture *tex = (struct r300_texture *)texture;
    struct r300_transfer *trans;
    struct r300_screen *rscreen = r300_screen(screen);
    unsigned offset;

    offset = r300_texture_get_offset(tex, level, zslice, face);  /* in bytes */

    trans = CALLOC_STRUCT(r300_transfer);
    if (trans) {
        pipe_texture_reference(&trans->transfer.texture, texture);
        trans->transfer.x = x;
        trans->transfer.y = y;
        trans->transfer.width = w;
        trans->transfer.height = h;
        trans->transfer.stride = r300_texture_get_stride(rscreen, tex, level);
        trans->transfer.usage = usage;
        trans->transfer.zslice = zslice;
        trans->transfer.face = face;

        trans->offset = offset;
    }
    return &trans->transfer;
}

static void r300_tex_transfer_destroy(struct pipe_transfer *trans)
{
   pipe_texture_reference(&trans->texture, NULL);
   FREE(trans);
}

static void* r300_transfer_map(struct pipe_screen *screen,
                              struct pipe_transfer *transfer)
{
    struct r300_texture *tex = (struct r300_texture*)transfer->texture;
    char *map;
    enum pipe_format format = tex->tex.format;

    map = pipe_buffer_map(screen, tex->buffer,
                          pipe_transfer_buffer_flags(transfer));

    if (!map) {
        return NULL;
    }

    return map + r300_transfer(transfer)->offset +
        transfer->y / util_format_get_blockheight(format) * transfer->stride +
        transfer->x / util_format_get_blockwidth(format) * util_format_get_blocksize(format);
}

static void r300_transfer_unmap(struct pipe_screen *screen,
                                struct pipe_transfer *transfer)
{
    struct r300_texture *tex = (struct r300_texture*)transfer->texture;
    pipe_buffer_unmap(screen, tex->buffer);
}

void r300_init_screen_transfer_functions(struct pipe_screen *screen)
{
    screen->get_tex_transfer = r300_get_tex_transfer;
    screen->tex_transfer_destroy = r300_tex_transfer_destroy;
    screen->transfer_map = r300_transfer_map;
    screen->transfer_unmap = r300_transfer_unmap;
}
