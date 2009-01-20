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

#ifndef R300_WINSYS_H
#define R300_WINSYS_H

#ifdef __cplusplus
extern "C" {
#endif

/* The public interface header for the r300 pipe driver.
 * Any winsys hosting this pipe needs to implement r300_winsys and then
 * call r300_create_context to start things. */

#include "pipe/p_defines.h"
#include "pipe/p_state.h"

struct radeon_cs;

struct r300_winsys {

    /* PCI ID */
    uint32_t pci_id;

    /* CS object. This is very much like Intel's batchbuffer.
     * Fill it full of dwords and relocs and then submit.
     * Repeat as needed. */
    /* Note: Unlike Mesa's version of this, we don't keep a copy of the CSM
     * that was used to create this CS. Is this a good idea? */
    /* Note: The pipe driver doesn't know how to use this. This is purely
     * for the winsys. */
    struct radeon_cs* cs;

    /* Check to see if there's room for commands. */
    boolean (*check_cs)(struct radeon_cs* cs, int size);

    /* Start a command emit. */
    void (*begin_cs)(struct radeon_cs* cs,
           int size,
           const char* file,
           const char* function,
           int line);

    /* Write a dword to the command buffer. */
    /* XXX is this an okay name for this handle? */
    void (*write_cs_dword)(struct radeon_cs* cs, uint32_t dword);

    /* Write a relocated dword to the command buffer. */
    void (*write_cs_reloc)(struct radeon_cs* cs,
           struct pipe_buffer* bo,
           uint32_t rd,
           uint32_t wd,
           uint32_t flags);

    /* Finish a command emit. */
    void (*end_cs)(struct radeon_cs* cs,
           const char* file,
           const char* function,
           int line);

    /* Flush the CS. */
    void (*flush_cs)(struct radeon_cs* cs);
};

struct pipe_context* r300_create_context(struct pipe_screen* screen,
                                         struct pipe_winsys* winsys,
                                         struct r300_winsys* r300_winsys);

#ifdef __cplusplus
}
#endif

#endif /* R300_WINSYS_H */