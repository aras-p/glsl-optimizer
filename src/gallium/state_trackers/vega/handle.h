/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.  All Rights Reserved.
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


/**
 * Convert opaque VG object handles into pointers and vice versa.
 * XXX This is not yet 64-bit safe!  All VG handles are 32 bits in size.
 */


#ifndef HANDLE_H
#define HANDLE_H

#include "VG/openvg.h"
#include "pipe/p_compiler.h"


struct vg_mask_layer;
struct vg_font;
struct vg_image;
struct vg_paint;
struct path;


static INLINE VGHandle
image_to_handle(struct vg_image *img)
{
   return (VGHandle) img;
}


static INLINE VGHandle
masklayer_to_handle(struct vg_mask_layer *mask)
{
   return (VGHandle) mask;
}


static INLINE VGHandle
font_to_handle(struct vg_font *font)
{
   return (VGHandle) font;
}

static INLINE VGHandle
paint_to_handle(struct vg_paint *paint)
{
   return (VGHandle) paint;
}

static INLINE VGHandle
path_to_handle(struct path *path)
{
   return (VGHandle) path;
}


static INLINE void *
handle_to_pointer(VGHandle h)
{
   return (void *) h;
}


static INLINE struct vg_font *
handle_to_font(VGHandle h)
{
   return (struct vg_font *) handle_to_pointer(h);
}


static INLINE struct vg_image *
handle_to_image(VGHandle h)
{
   return (struct vg_image *) handle_to_pointer(h);
}


static INLINE struct vg_mask_layer *
handle_to_masklayer(VGHandle h)
{
   return (struct vg_mask_layer *) handle_to_pointer(h);
}


static INLINE struct vg_object *
handle_to_object(VGHandle h)
{
   return (struct vg_object *) handle_to_pointer(h);
}


static INLINE struct vg_paint *
handle_to_paint(VGHandle h)
{
   return (struct vg_paint *) handle_to_pointer(h);
}


static INLINE struct path *
handle_to_path(VGHandle h)
{
   return (struct path *) handle_to_pointer(h);
}


#endif /* HANDLE_H */
