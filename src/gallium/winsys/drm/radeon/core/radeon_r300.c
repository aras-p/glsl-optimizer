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

#include "radeon_r300.h"
#include "radeon_buffer.h"

#include "radeon_cs_gem.h"

static void radeon_set_flush_cb(struct radeon_winsys *winsys,
                                void (*flush_cb)(void *),
                                void *data)
{
    winsys->priv->flush_cb = flush_cb;
    winsys->priv->flush_data = data;
    radeon_cs_space_set_flush(winsys->priv->cs, flush_cb, data);
}

static boolean radeon_add_buffer(struct radeon_winsys* winsys,
                                 struct pipe_buffer* pbuffer,
                                 uint32_t rd,
                                 uint32_t wd)
{
    struct radeon_bo* bo = ((struct radeon_pipe_buffer*)pbuffer)->bo;

    radeon_cs_space_add_persistent_bo(winsys->priv->cs, bo, rd, wd);
    return TRUE;
}

static boolean radeon_validate(struct radeon_winsys* winsys)
{
    if (radeon_cs_space_check(winsys->priv->cs) < 0) {
        return FALSE;
    }

    /* Things are fine, we can proceed as normal. */
    return TRUE;
}

static boolean radeon_check_cs(struct radeon_winsys* winsys, int size)
{
    struct radeon_cs* cs = winsys->priv->cs;

    return radeon_validate(winsys) && cs->cdw + size <= cs->ndw;
}

static void radeon_begin_cs(struct radeon_winsys* winsys,
                            int size,
                            const char* file,
                            const char* function,
                            int line)
{
    radeon_cs_begin(winsys->priv->cs, size, file, function, line);
}

static void radeon_write_cs_dword(struct radeon_winsys* winsys,
                                  uint32_t dword)
{
    radeon_cs_write_dword(winsys->priv->cs, dword);
}

static void radeon_write_cs_reloc(struct radeon_winsys* winsys,
                                  struct pipe_buffer* pbuffer,
                                  uint32_t rd,
                                  uint32_t wd,
                                  uint32_t flags)
{
    int retval = 0;
    struct radeon_pipe_buffer* radeon_buffer =
        (struct radeon_pipe_buffer*)pbuffer;

    assert(!radeon_buffer->pb);

    retval = radeon_cs_write_reloc(winsys->priv->cs, radeon_buffer->bo,
                                   rd, wd, flags);

    if (retval) {
        debug_printf("radeon: Relocation of %p (%d, %d, %d) failed!\n",
                pbuffer, rd, wd, flags);
    }
}

static void radeon_reset_bos(struct radeon_winsys *winsys)
{
    radeon_cs_space_reset_bos(winsys->priv->cs);
}

static void radeon_end_cs(struct radeon_winsys* winsys,
                          const char* file,
                          const char* function,
                          int line)
{
    radeon_cs_end(winsys->priv->cs, file, function, line);
}

static void radeon_flush_cs(struct radeon_winsys* winsys)
{
    int retval;

    /* Don't flush a zero-sized CS. */
    if (!winsys->priv->cs->cdw) {
        return;
    }

    /* Emit the CS. */
    retval = radeon_cs_emit(winsys->priv->cs);
    if (retval) {
        debug_printf("radeon: Bad CS, dumping...\n");
        radeon_cs_print(winsys->priv->cs, stderr);
    }

    /* Reset CS.
     * Someday, when we care about performance, we should really find a way
     * to rotate between two or three CS objects so that the GPU can be
     * spinning through one CS while another one is being filled. */
    radeon_cs_erase(winsys->priv->cs);
}

void
radeon_setup_winsys(int fd, struct radeon_winsys* winsys)
{
    struct radeon_winsys_priv* priv = winsys->priv;

    priv->csm = radeon_cs_manager_gem_ctor(fd);

    /* Size limit on IBs is 64 kibibytes. */
    priv->cs = radeon_cs_create(priv->csm, 1024 * 64 / 4);
    radeon_cs_set_limit(priv->cs,
            RADEON_GEM_DOMAIN_GTT, winsys->gart_size);
    radeon_cs_set_limit(priv->cs,
            RADEON_GEM_DOMAIN_VRAM, winsys->vram_size);

    winsys->add_buffer = radeon_add_buffer;
    winsys->validate = radeon_validate;

    winsys->check_cs = radeon_check_cs;
    winsys->begin_cs = radeon_begin_cs;
    winsys->write_cs_dword = radeon_write_cs_dword;
    winsys->write_cs_reloc = radeon_write_cs_reloc;
    winsys->end_cs = radeon_end_cs;
    winsys->flush_cs = radeon_flush_cs;
    winsys->reset_bos = radeon_reset_bos;
    winsys->set_flush_cb = radeon_set_flush_cb;
}
