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

#include "VG/openvg.h"

#include "vg_context.h"
#include "paint.h"
#include "image.h"

VGPaint vgCreatePaint(void)
{
   return (VGPaint) paint_create(vg_current_context());
}

void vgDestroyPaint(VGPaint p)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_paint *paint;

   if (p == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   paint = (struct vg_paint *)p;
   paint_destroy(paint);
}

void vgSetPaint(VGPaint paint, VGbitfield paintModes)
{
   struct vg_context *ctx = vg_current_context();

   if (paint == VG_INVALID_HANDLE) {
      /* restore the default */
      paint = (VGPaint)ctx->default_paint;
   } else if (!vg_object_is_valid((void*)paint, VG_OBJECT_PAINT)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   if (!(paintModes & ((VG_FILL_PATH|VG_STROKE_PATH)))) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   if (paintModes & VG_FILL_PATH) {
      ctx->state.vg.fill_paint = (struct vg_paint *)paint;
   }
   if (paintModes & VG_STROKE_PATH) {
      ctx->state.vg.stroke_paint = (struct vg_paint *)paint;
   }
}

VGPaint vgGetPaint(VGPaintMode paintMode)
{
   struct vg_context *ctx = vg_current_context();
   VGPaint paint = VG_INVALID_HANDLE;

   if (paintMode < VG_STROKE_PATH || paintMode > VG_FILL_PATH) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return VG_INVALID_HANDLE;
   }

   if (paintMode == VG_FILL_PATH)
      paint = (VGPaint)ctx->state.vg.fill_paint;
   else if (paintMode == VG_STROKE_PATH)
      paint = (VGPaint)ctx->state.vg.stroke_paint;

   if (paint == (VGPaint)ctx->default_paint)
      paint = VG_INVALID_HANDLE;

   return paint;
}

void vgSetColor(VGPaint paint, VGuint rgba)
{
   struct vg_context *ctx = vg_current_context();

   if (paint == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   if (!vg_object_is_valid((void*)paint, VG_OBJECT_PAINT)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   {
      struct vg_paint *p = (struct vg_paint *)paint;
      paint_set_colori(p, rgba);
   }
}

VGuint vgGetColor(VGPaint paint)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_paint *p;
   VGuint rgba = 0;

   if (paint == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return rgba;
   }

   if (!vg_object_is_valid((void*)paint, VG_OBJECT_PAINT)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return rgba;
   }
   p = (struct vg_paint *)paint;

   return paint_colori(p);
}

void vgPaintPattern(VGPaint paint, VGImage pattern)
{
   struct vg_context *ctx = vg_current_context();

   if (paint == VG_INVALID_HANDLE ||
       !vg_context_is_object_valid(ctx, VG_OBJECT_PAINT, (void *)paint)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   if (pattern == VG_INVALID_HANDLE) {
      paint_set_type((struct vg_paint*)paint, VG_PAINT_TYPE_COLOR);
      return;
   }

   if (!vg_context_is_object_valid(ctx, VG_OBJECT_IMAGE, (void *)pattern)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }


   if (!vg_object_is_valid((void*)paint, VG_OBJECT_PAINT) ||
       !vg_object_is_valid((void*)pattern, VG_OBJECT_IMAGE)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   paint_set_pattern((struct vg_paint*)paint,
                     (struct vg_image*)pattern);
}

