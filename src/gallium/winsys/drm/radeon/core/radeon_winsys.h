/*
 * Copyright © 2009 Corbin Simpson
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS, AUTHORS
 * AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 */
/*
 * Authors:
 *      Corbin Simpson <MostAwesomeDude@gmail.com>
 */
#ifndef RADEON_WINSYS_H
#define RADEON_WINSYS_H

#include "util/u_simple_screen.h"

struct radeon_winsys_priv;

struct radeon_winsys {
    /* Parent class. */
    struct pipe_winsys base;

    /* Winsys private */
    struct radeon_winsys_priv* priv;

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
    boolean (*add_buffer)(struct radeon_winsys* winsys,
                          struct pipe_buffer* pbuffer,
                          uint32_t rd,
                          uint32_t wd);

    /* Revalidate all currently setup pipe_buffers.
     * Returns TRUE if a flush is required. */
    boolean (*validate)(struct radeon_winsys* winsys);

    /* Check to see if there's room for commands. */
    boolean (*check_cs)(struct radeon_winsys* winsys, int size);

    /* Start a command emit. */
    void (*begin_cs)(struct radeon_winsys* winsys,
                     int size,
                     const char* file,
                     const char* function,
                     int line);

    /* Write a dword to the command buffer. */
    void (*write_cs_dword)(struct radeon_winsys* winsys, uint32_t dword);

    /* Write a relocated dword to the command buffer. */
    void (*write_cs_reloc)(struct radeon_winsys* winsys,
                           struct pipe_buffer* bo,
                           uint32_t rd,
                           uint32_t wd,
                           uint32_t flags);

    /* Finish a command emit. */
    void (*end_cs)(struct radeon_winsys* winsys,
                   const char* file,
                   const char* function,
                   int line);

    /* Flush the CS. */
    void (*flush_cs)(struct radeon_winsys* winsys);

    /* winsys flush - callback from winsys when flush required */
    void (*set_flush_cb)(struct radeon_winsys *winsys,
			 void (*flush_cb)(void *), void *data);

    void (*reset_bos)(struct radeon_winsys *winsys);

    void (*buffer_set_tiling)(struct radeon_winsys* winsys,
                              struct pipe_buffer* buffer,
                              uint32_t pitch,
                              boolean microtiled,
                              boolean macrotiled);
};

#endif
