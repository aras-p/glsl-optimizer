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

#ifndef RENDERER_H
#define RENDERER_H

#include "VG/openvg.h"

struct renderer;

struct vg_context;
struct pipe_texture;
struct pipe_surface;

struct renderer *renderer_create(struct vg_context *owner);
void renderer_destroy(struct renderer *);

void renderer_draw_quad(struct renderer *,
                        VGfloat x1, VGfloat y1,
                        VGfloat x2, VGfloat y2,
                        VGfloat depth);
void renderer_draw_texture(struct renderer *,
                           struct pipe_texture *texture,
                           VGfloat x1offset, VGfloat y1offset,
                           VGfloat x2offset, VGfloat y2offset,
                           VGfloat x1, VGfloat y1,
                           VGfloat x2, VGfloat y2);
void renderer_texture_quad(struct renderer *,
                           struct pipe_texture *texture,
                           VGfloat x1offset, VGfloat y1offset,
                           VGfloat x2offset, VGfloat y2offset,
                           VGfloat x1, VGfloat y1,
                           VGfloat x2, VGfloat y2,
                           VGfloat x3, VGfloat y3,
                           VGfloat x4, VGfloat y4);
void renderer_copy_texture(struct renderer *r,
                           struct pipe_texture *src,
                           VGfloat sx1, VGfloat sy1,
                           VGfloat sx2, VGfloat sy2,
                           struct pipe_texture *dst,
                           VGfloat dx1, VGfloat dy1,
                           VGfloat dx2, VGfloat dy2);
void renderer_copy_surface(struct renderer *r,
                           struct pipe_surface *src,
                           int sx1, int sy1,
                           int sx2, int sy2,
                           struct pipe_surface *dst,
                           int dx1, int dy1,
                           int dx2, int dy2,
                           float z, unsigned filter);


#endif
