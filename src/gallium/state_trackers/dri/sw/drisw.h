/**************************************************************************
 *
 * Copyright 2009, VMware, Inc.
 * All Rights Reserved.
 * Copyright 2010 George Sapountzis <gsapountzis@gmail.com>
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

#ifndef DRISW_H
#define DRISW_H

#include "dri_context.h"
#include "dri_drawable.h"

#include "state_tracker/st_api.h"
#include "dri_wrapper.h"

const __DRIconfig **
drisw_init_screen(__DRIscreen * sPriv);

void
drisw_update_drawable_info(struct dri_drawable *drawable);

void
drisw_flush_frontbuffer(struct dri_drawable *drawable,
                        enum st_attachment_type statt);

void
drisw_allocate_textures(struct dri_drawable *drawable,
                        unsigned mask);

void drisw_swap_buffers(__DRIdrawable * dPriv);

#endif /* DRISW_H */
