/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#ifndef U_SURFACE_H
#define U_SURFACE_H


#include "pipe/p_compiler.h"
#include "pipe/p_state.h"


/**
 * Are s1 and s2 the same surface?
 * Surfaces are basically views into textures so check if the two surfaces
 * name the same part of the same texture.
 */
static INLINE boolean
util_same_surface(const struct pipe_surface *s1, const struct pipe_surface *s2)
{
   return (s1->texture == s2->texture &&
           s1->face == s2->face &&
           s1->level == s2->level &&
           s1->zslice == s2->zslice);
}




extern boolean
util_create_rgba_surface(struct pipe_screen *screen,
                         uint width, uint height,
                         struct pipe_texture **textureOut,
                         struct pipe_surface **surfaceOut);


extern void
util_destroy_rgba_surface(struct pipe_texture *texture,
                          struct pipe_surface *surface);


extern boolean
util_framebuffer_state_equal(const struct pipe_framebuffer_state *dst,
                             const struct pipe_framebuffer_state *src);

extern void
util_copy_framebuffer_state(struct pipe_framebuffer_state *dst,
                            const struct pipe_framebuffer_state *src);


extern void
util_unreference_framebuffer_state(struct pipe_framebuffer_state *fb);


#endif /* U_SURFACE_H */
