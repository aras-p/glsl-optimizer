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
 * call r300_create_screen to start things. */

#include "pipe/p_defines.h"
#include "pipe/p_state.h"

struct radeon_winsys;

/* Creates a new r300 screen. */
struct pipe_screen* r300_create_screen(struct radeon_winsys* radeon_winsys);


boolean r300_get_texture_buffer(struct pipe_screen* screen,
                                struct pipe_texture* texture,
                                struct pipe_buffer** buffer,
                                unsigned* stride);

#ifdef __cplusplus
}
#endif

#endif /* R300_WINSYS_H */
