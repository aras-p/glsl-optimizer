/**************************************************************************
 *
 * Copyright 2009, VMware, Inc.
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

#ifndef DRI_DRAWABLE_H
#define DRI_DRAWABLE_H

#include "pipe/p_compiler.h"

struct pipe_surface;
struct pipe_fence_handle;
struct st_framebuffer;
struct dri_context;

#define DRI_SWAP_FENCES_MAX  8
#define DRI_SWAP_FENCES_MASK 7

struct dri_drawable
{
   /* dri */
   __DRIdrawable *dPriv;
   __DRIscreen *sPriv;

   unsigned attachments[8];
   unsigned num_attachments;

   boolean is_pixmap;

   __DRIbuffer old[8];
   unsigned old_num;
   unsigned old_w;
   unsigned old_h;

   /* gallium */
   struct st_framebuffer *stfb;
   struct pipe_fence_handle *swap_fences[DRI_SWAP_FENCES_MAX];
   unsigned int head;
   unsigned int tail;
   unsigned int desired_fences;
   unsigned int cur_fences;

   enum pipe_format color_format;
   enum pipe_format depth_stencil_format;
};

static INLINE struct dri_drawable *
dri_drawable(__DRIdrawable * driDrawPriv)
{
   return (struct dri_drawable *)driDrawPriv->driverPrivate;
}

/***********************************************************************
 * dri_drawable.c
 */
boolean
dri_create_buffer(__DRIscreen * sPriv,
		  __DRIdrawable * dPriv,
		  const __GLcontextModes * visual, boolean isPixmap);

void
dri_update_buffer(struct pipe_screen *screen, void *context_private);

void
dri_flush_frontbuffer(struct pipe_screen *screen,
		      struct pipe_surface *surf, void *context_private);

void dri_swap_buffers(__DRIdrawable * dPriv);

void
dri_copy_sub_buffer(__DRIdrawable * dPriv, int x, int y, int w, int h);

void dri_get_buffers(__DRIdrawable * dPriv);

void dri_destroy_buffer(__DRIdrawable * dPriv);

void dri2_set_tex_buffer2(__DRIcontext *pDRICtx, GLint target,
                          GLint glx_texture_format, __DRIdrawable *dPriv);

void dri2_set_tex_buffer(__DRIcontext *pDRICtx, GLint target,
                         __DRIdrawable *dPriv);

void
dri1_update_drawables(struct dri_context *ctx,
		      struct dri_drawable *draw, struct dri_drawable *read);

void
dri1_flush_frontbuffer(struct pipe_screen *screen,
		       struct pipe_surface *surf, void *context_private);
#endif

/* vim: set sw=3 ts=8 sts=3 expandtab: */
