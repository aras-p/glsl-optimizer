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

#ifndef R300_TEXTURE_H
#define R300_TEXTURE_H

#include "util/u_format.h"

struct r300_texture;

uint32_t r300_translate_texformat(enum pipe_format format,
                                  const unsigned char *swizzle);

uint32_t r500_tx_format_msb_bit(enum pipe_format format);

unsigned r300_texture_get_stride(struct r300_screen* screen,
                                 struct r300_texture* tex, unsigned level);

unsigned r300_texture_get_offset(struct r300_texture* tex, unsigned level,
                                 unsigned zslice, unsigned face);

void r300_texture_reinterpret_format(struct pipe_screen *screen,
                                     struct pipe_resource *tex,
                                     enum pipe_format new_format);

boolean r300_is_colorbuffer_format_supported(enum pipe_format format);

boolean r300_is_zs_format_supported(enum pipe_format format);

boolean r300_is_sampler_format_supported(enum pipe_format format);


struct pipe_resource*
r300_texture_from_handle(struct pipe_screen* screen,
			 const struct pipe_resource* base,
			 struct winsys_handle *whandle);

struct pipe_resource*
r300_texture_create(struct pipe_screen* screen,
		    const struct pipe_resource* template);


struct pipe_surface* r300_get_tex_surface(struct pipe_screen* screen,
					  struct pipe_resource* texture,
					  unsigned face,
					  unsigned level,
					  unsigned zslice,
					  unsigned flags);

void r300_tex_surface_destroy(struct pipe_surface* s);

#endif /* R300_TEXTURE_H */
