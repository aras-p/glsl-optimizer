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
#ifndef RADEON_DRM_H
#define RADEON_DRM_H

#include "state_tracker/drm_api.h"


struct pipe_screen* radeon_create_screen(struct drm_api* api,
                                         int drmFB,
					 struct drm_create_screen_arg *arg);


boolean radeon_buffer_from_texture(struct drm_api* api,
                                   struct pipe_screen* screen,
                                   struct pipe_texture* texture,
                                   struct pipe_buffer** buffer,
                                   unsigned* stride);

struct pipe_buffer* radeon_buffer_from_handle(struct drm_api* api,
                                              struct pipe_screen* screen,
                                              const char* name,
                                              unsigned handle);

boolean radeon_handle_from_buffer(struct drm_api* api,
                                  struct pipe_screen* screen,
                                  struct pipe_buffer* buffer,
                                  unsigned* handle);

boolean radeon_global_handle_from_buffer(struct drm_api* api,
                                         struct pipe_screen* screen,
                                         struct pipe_buffer* buffer,
                                         unsigned* handle);

void radeon_destroy_drm_api(struct drm_api* api);

/* Guess at whether this chipset should use r300g.
 *
 * I believe that this check is valid, but I haven't been exhaustive. */
static INLINE boolean is_r3xx(int pciid)
{
    return (pciid > 0x3150) && (pciid < 0x796f);
}

#endif
