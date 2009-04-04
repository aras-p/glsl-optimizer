/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Bismarck, ND., USA
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * 
 **************************************************************************/
/*
 * Authors: Keith Whitwell <keithw-at-tungstengraphics-dot-com>
 */

#include "radeon_winsys_softpipe.h"

struct radeon_softpipe_winsys {
    struct softpipe_winsys  sp_winsys;
    struct radeon_context      *radeon_context;
};

/**
 * Return list of surface formats supported by this driver.
 */
static boolean radeon_is_format_supported(struct softpipe_winsys *sws,
                                          uint format)
{
    switch (format) {
        case PIPE_FORMAT_A8R8G8B8_UNORM:
        case PIPE_FORMAT_R5G6B5_UNORM:
        case PIPE_FORMAT_Z24S8_UNORM:
            return TRUE;
        default:
            break;
    }
    return FALSE;
}

struct pipe_context *radeon_create_softpipe(struct pipe_winsys* winsys)
{
    struct softpipe_winsys *sp_winsys;
    struct pipe_screen *pipe_screen;

    pipe_screen = softpipe_create_screen(winsys);

    sp_winsys = CALLOC_STRUCT(softpipe_winsys);
    if (sp_winsys == NULL) {
        return NULL;
    }

    sp_winsys->is_format_supported = radeon_is_format_supported;
    return softpipe_create(pipe_screen,
                           winsys,
                           sp_winsys);
}
