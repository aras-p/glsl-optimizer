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
#include "pipe/internal/p_winsys_screen.h"

struct r300_winsys {
    /* Parent class */
    struct pipe_winsys base;

    /* Opaque Radeon-specific winsys object. */
    void* radeon_winsys;

    /* PCI ID */
    uint32_t pci_id;

    /* GB pipe count */
    uint32_t gb_pipes;

    /* Z pipe count (rv530 only) */
    uint32_t z_pipes;

    /* GART size. */
    uint32_t gart_size;

    /* VRAM size. */
    uint32_t vram_size;

    /* Add a pipe_buffer to the list of buffer objects to validate. */
    boolean (*add_buffer)(struct r300_winsys* winsys,
                          struct pipe_buffer* pbuffer,
                          uint32_t rd,
                          uint32_t wd);

    /* Revalidate all currently setup pipe_buffers.
     * Returns TRUE if a flush is required. */
    boolean (*validate)(struct r300_winsys* winsys);

    /* Check to see if there's room for commands. */
    boolean (*check_cs)(struct r300_winsys* winsys, int size);

    /* Start a command emit. */
    void (*begin_cs)(struct r300_winsys* winsys,
                     int size,
                     const char* file,
                     const char* function,
                     int line);

    /* Write a dword to the command buffer. */
    void (*write_cs_dword)(struct r300_winsys* winsys, uint32_t dword);

    /* Write a relocated dword to the command buffer. */
    void (*write_cs_reloc)(struct r300_winsys* winsys,
                           struct pipe_buffer* bo,
                           uint32_t rd,
                           uint32_t wd,
                           uint32_t flags);

    /* Finish a command emit. */
    void (*end_cs)(struct r300_winsys* winsys,
                   const char* file,
                   const char* function,
                   int line);

    /* Flush the CS. */
    void (*flush_cs)(struct r300_winsys* winsys);

    /* winsys flush - callback from winsys when flush required */
    void (*set_flush_cb)(struct r300_winsys *winsys,
			 void (*flush_cb)(void *), void *data);

    void (*reset_bos)(struct r300_winsys *winsys);
};

struct pipe_context* r300_create_context(struct pipe_screen* screen,
                                         struct r300_winsys* r300_winsys);

boolean r300_get_texture_buffer(struct pipe_texture* texture,
                                struct pipe_buffer** buffer,
                                unsigned* stride);

#ifdef __cplusplus
}
#endif

#endif /* R300_WINSYS_H */
