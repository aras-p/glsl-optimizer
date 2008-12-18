/* 
 * Copyright © 2008 Jérôme Glisse
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
 *      Jérôme Glisse <glisse@freedesktop.org>
 */
#ifndef AMD_BUFFER_H
#define AMD_BUFFER_H

#include "pipe/p_winsys.h"
#include "amd_screen.h"
#include "amd_context.h"
#include "radeon_bo.h"

struct amd_pipe_buffer {
    struct pipe_buffer  base;
    struct radeon_bo    *bo;
};

struct amd_pipe_winsys {
    struct pipe_winsys      winsys;
    struct amd_screen       *amd_screen;
};

struct pipe_winsys *amd_pipe_winsys(struct amd_screen *amd_screen);
struct pipe_surface *amd_surface_from_handle(struct amd_context *amd_context,
                                             uint32_t handle,
                                             enum pipe_format format,
                                             int w, int h, int pitch);

#endif
