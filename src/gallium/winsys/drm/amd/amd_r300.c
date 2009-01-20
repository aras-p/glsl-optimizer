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

#include "amd_r300.h"

static boolean amd_r300_check_cs(struct radeon_cs* cs, int size)
{
    /* XXX check size here, lazy ass! */
    return TRUE;
}

static void amd_r300_write_cs_reloc(struct radeon_cs* cs,
                                    struct pipe_buffer* pbuffer,
                                    uint32_t rd,
                                    uint32_t wd,
                                    uint32_t flags)
{
    radeon_cs_write_reloc(cs, ((struct amd_pipe_buffer*)pbuffer)->bo, rd, wd, flags);
}

static void amd_r300_flush_cs(struct radeon_cs* cs)
{
    radeon_cs_emit(cs);
    radeon_cs_erase(cs);
}

struct r300_winsys* amd_create_r300_winsys(int fd, uint32_t pci_id)
{
    struct r300_winsys* winsys = calloc(1, sizeof(struct r300_winsys));

    struct radeon_cs_manager* csm = radeon_cs_manager_gem_ctor(fd);

    winsys->pci_id = pci_id;

    winsys->cs = radeon_cs_create(csm, 1024 * 64 / 4);

    winsys->check_cs = amd_r300_check_cs;
    winsys->begin_cs = radeon_cs_begin;
    winsys->write_cs_dword = radeon_cs_write_dword;
    winsys->write_cs_reloc = amd_r300_write_cs_reloc;
    winsys->end_cs = radeon_cs_end;
    winsys->flush_cs = amd_r300_flush_cs;

    return winsys;
}
