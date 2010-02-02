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

#include "image.h"

#include "VG/openvg.h"

#include "vg_context.h"
#include "vg_translate.h"
#include "api_consts.h"
#include "image.h"

#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "util/u_inlines.h"
#include "util/u_blit.h"
#include "util/u_tile.h"
#include "util/u_memory.h"

static INLINE VGboolean supported_image_format(VGImageFormat format)
{
   switch(format) {
   case VG_sRGBX_8888:
   case VG_sRGBA_8888:
   case VG_sRGBA_8888_PRE:
   case VG_sRGB_565:
   case VG_sRGBA_5551:
   case VG_sRGBA_4444:
   case VG_sL_8:
   case VG_lRGBX_8888:
   case VG_lRGBA_8888:
   case VG_lRGBA_8888_PRE:
   case VG_lL_8:
   case VG_A_8:
   case VG_BW_1:
#ifdef OPENVG_VERSION_1_1
   case VG_A_1:
   case VG_A_4:
#endif
   case VG_sXRGB_8888:
   case VG_sARGB_8888:
   case VG_sARGB_8888_PRE:
   case VG_sARGB_1555:
   case VG_sARGB_4444:
   case VG_lXRGB_8888:
   case VG_lARGB_8888:
   case VG_lARGB_8888_PRE:
   case VG_sBGRX_8888:
   case VG_sBGRA_8888:
   case VG_sBGRA_8888_PRE:
   case VG_sBGR_565:
   case VG_sBGRA_5551:
   case VG_sBGRA_4444:
   case VG_lBGRX_8888:
   case VG_lBGRA_8888:
   case VG_lBGRA_8888_PRE:
   case VG_sXBGR_8888:
   case VG_sABGR_8888:
   case VG_sABGR_8888_PRE:
   case VG_sABGR_1555:
   case VG_sABGR_4444:
   case VG_lXBGR_8888:
   case VG_lABGR_8888:
   case VG_lABGR_8888_PRE:
      return VG_TRUE;
   default:
      return VG_FALSE;
   }
   return VG_FALSE;
}

VGImage vgCreateImage(VGImageFormat format,
                      VGint width, VGint height,
                      VGbitfield allowedQuality)
{
   struct vg_context *ctx = vg_current_context();

   if (!supported_image_format(format)) {
      vg_set_error(ctx, VG_UNSUPPORTED_IMAGE_FORMAT_ERROR);
      return VG_INVALID_HANDLE;
   }
   if (width <= 0 || height <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return VG_INVALID_HANDLE;
   }
   if (width > vgGeti(VG_MAX_IMAGE_WIDTH) ||
       height > vgGeti(VG_MAX_IMAGE_HEIGHT)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return VG_INVALID_HANDLE;
   }
   if (width * height > vgGeti(VG_MAX_IMAGE_PIXELS)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return VG_INVALID_HANDLE;
   }

   if (!(allowedQuality & ((VG_IMAGE_QUALITY_NONANTIALIASED |
                           VG_IMAGE_QUALITY_FASTER |
                           VG_IMAGE_QUALITY_BETTER)))) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return VG_INVALID_HANDLE;
   }

   return (VGImage)image_create(format, width, height);
}

void vgDestroyImage(VGImage image)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_image *img = (struct vg_image *)image;

   if (image == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (!vg_object_is_valid((void*)image, VG_OBJECT_IMAGE)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   image_destroy(img);
}

void vgClearImage(VGImage image,
                  VGint x, VGint y,
                  VGint width, VGint height)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_image *img;

   if (image == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (width <= 0 || height <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   img = (struct vg_image*)image;

   if (x + width < 0 || y + height < 0)
      return;

   image_clear(img, x, y, width, height);

}

void vgImageSubData(VGImage image,
                    const void * data,
                    VGint dataStride,
                    VGImageFormat dataFormat,
                    VGint x, VGint y,
                    VGint width, VGint height)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_image *img;

   if (image == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (!supported_image_format(dataFormat)) {
      vg_set_error(ctx, VG_UNSUPPORTED_IMAGE_FORMAT_ERROR);
      return;
   }
   if (width <= 0 || height <= 0 || !data || !is_aligned(data)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   img = (struct vg_image*)(image);
   image_sub_data(img, data, dataStride, dataFormat,
                  x, y, width, height);
}

void vgGetImageSubData(VGImage image,
                       void * data,
                       VGint dataStride,
                       VGImageFormat dataFormat,
                       VGint x, VGint y,
                       VGint width, VGint height)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_image *img;

   if (image == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (!supported_image_format(dataFormat)) {
      vg_set_error(ctx, VG_UNSUPPORTED_IMAGE_FORMAT_ERROR);
      return;
   }
   if (width <= 0 || height <= 0 || !data || !is_aligned(data)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   img = (struct vg_image*)image;
   image_get_sub_data(img, data, dataStride, dataFormat,
                      x, y, width, height);
}

VGImage vgChildImage(VGImage parent,
                     VGint x, VGint y,
                     VGint width, VGint height)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_image *p;

   if (parent == VG_INVALID_HANDLE ||
       !vg_context_is_object_valid(ctx, VG_OBJECT_IMAGE, (void*)parent) ||
       !vg_object_is_valid((void*)parent, VG_OBJECT_IMAGE)) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return VG_INVALID_HANDLE;
   }
   if (width <= 0 || height <= 0 || x < 0 || y < 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return VG_INVALID_HANDLE;
   }
   p = (struct vg_image *)parent;
   if (x > p->width  || y > p->height) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return VG_INVALID_HANDLE;
   }
   if (x + width > p->width  || y + height > p->height) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return VG_INVALID_HANDLE;
   }

   return (VGImage)image_child_image(p, x, y, width, height);
}

VGImage vgGetParent(VGImage image)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_image *img;

   if (image == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return VG_INVALID_HANDLE;
   }

   img = (struct vg_image*)image;
   if (img->parent)
      return (VGImage)img->parent;
   else
      return image;
}

void vgCopyImage(VGImage dst, VGint dx, VGint dy,
                 VGImage src, VGint sx, VGint sy,
                 VGint width, VGint height,
                 VGboolean dither)
{
   struct vg_context *ctx = vg_current_context();

   if (src == VG_INVALID_HANDLE || dst == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   if (width <= 0 || height <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   vg_validate_state(ctx);
   image_copy((struct vg_image*)dst, dx, dy,
              (struct vg_image*)src, sx, sy,
              width, height, dither);
}

void vgDrawImage(VGImage image)
{
   struct vg_context *ctx = vg_current_context();

   if (!ctx)
      return;

   if (image == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }

   vg_validate_state(ctx);
   image_draw((struct vg_image*)image);
}

void vgSetPixels(VGint dx, VGint dy,
                 VGImage src, VGint sx, VGint sy,
                 VGint width, VGint height)
{
   struct vg_context *ctx = vg_current_context();

   vg_validate_state(ctx);

   if (src == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (width <= 0 || height <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   image_set_pixels(dx, dy, (struct vg_image*)src, sx, sy, width,
                    height);
}

void vgGetPixels(VGImage dst, VGint dx, VGint dy,
                 VGint sx, VGint sy,
                 VGint width, VGint height)
{
   struct vg_context *ctx = vg_current_context();
   struct vg_image *img;

   if (dst == VG_INVALID_HANDLE) {
      vg_set_error(ctx, VG_BAD_HANDLE_ERROR);
      return;
   }
   if (width <= 0 || height <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   img = (struct vg_image*)dst;

   image_get_pixels(img, dx, dy,
                    sx, sy, width, height);
}

void vgWritePixels(const void * data, VGint dataStride,
                   VGImageFormat dataFormat,
                   VGint dx, VGint dy,
                   VGint width, VGint height)
{
   struct vg_context *ctx = vg_current_context();
   struct pipe_context *pipe = ctx->pipe;

   if (!supported_image_format(dataFormat)) {
      vg_set_error(ctx, VG_UNSUPPORTED_IMAGE_FORMAT_ERROR);
      return;
   }
   if (!data || !is_aligned(data)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (width <= 0 || height <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   vg_validate_state(ctx);
   {
      struct vg_image *img = image_create(dataFormat, width, height);
      image_sub_data(img, data, dataStride, dataFormat, 0, 0,
                     width, height);
#if 0
      struct matrix *matrix = &ctx->state.vg.image_user_to_surface_matrix;
      matrix_translate(matrix, dx, dy);
      image_draw(img);
      matrix_translate(matrix, -dx, -dy);
#else
      /* this looks like a better approach */
      image_set_pixels(dx, dy, img, 0, 0, width, height);
#endif
      image_destroy(img);
   }
   /* make sure rendering has completed */
   pipe->flush(pipe, PIPE_FLUSH_RENDER_CACHE, NULL);
}

void vgReadPixels(void * data, VGint dataStride,
                  VGImageFormat dataFormat,
                  VGint sx, VGint sy,
                  VGint width, VGint height)
{
   struct vg_context *ctx = vg_current_context();
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_screen *screen = pipe->screen;

   struct st_framebuffer *stfb = ctx->draw_buffer;
   struct st_renderbuffer *strb = stfb->strb;
   struct pipe_framebuffer_state *fb = &ctx->state.g3d.fb;

   VGfloat temp[VEGA_MAX_IMAGE_WIDTH][4];
   VGfloat *df = (VGfloat*)temp;
   VGint y = (fb->height - sy) - 1, yStep = -1;
   VGint i;
   VGubyte *dst = (VGubyte *)data;
   VGint xoffset = 0, yoffset = 0;

   if (!supported_image_format(dataFormat)) {
      vg_set_error(ctx, VG_UNSUPPORTED_IMAGE_FORMAT_ERROR);
      return;
   }
   if (!data || !is_aligned(data)) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }
   if (width <= 0 || height <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   /* make sure rendering has completed */
   pipe->flush(pipe, PIPE_FLUSH_RENDER_CACHE, NULL);
   if (sx < 0) {
      xoffset = -sx;
      xoffset *= _vega_size_for_format(dataFormat);
      width += sx;
      sx = 0;
   }
   if (sy < 0) {
      yoffset = -sy;
      height += sy;
      sy = 0;
      y = (fb->height - sy) - 1;
      yoffset *= dataStride;
   }

   {
      struct pipe_transfer *transfer;

      transfer = screen->get_tex_transfer(screen, strb->texture,  0, 0, 0,
                                          PIPE_TRANSFER_READ,
                                          0, 0, width, height);

      /* Do a row at a time to flip image data vertically */
      for (i = 0; i < height; i++) {
#if 0
         debug_printf("%d-%d  == %d\n", sy, height, y);
#endif
         pipe_get_tile_rgba(transfer, sx, y, width, 1, df);
         y += yStep;
         _vega_pack_rgba_span_float(ctx, width, temp, dataFormat,
                                    dst + yoffset + xoffset);
         dst += dataStride;
      }

      screen->tex_transfer_destroy(transfer);
   }
}

void vgCopyPixels(VGint dx, VGint dy,
                  VGint sx, VGint sy,
                  VGint width, VGint height)
{
   struct vg_context *ctx = vg_current_context();
   struct pipe_framebuffer_state *fb = &ctx->state.g3d.fb;
   struct st_renderbuffer *strb = ctx->draw_buffer->strb;

   if (width <= 0 || height <= 0) {
      vg_set_error(ctx, VG_ILLEGAL_ARGUMENT_ERROR);
      return;
   }

   /* do nothing if we copy from outside the fb */
   if (dx >= (VGint)fb->width || dy >= (VGint)fb->height ||
       sx >= (VGint)fb->width || sy >= (VGint)fb->height)
      return;

   vg_validate_state(ctx);
   /* make sure rendering has completed */
   vgFinish();

   vg_copy_surface(ctx, strb->surface, dx, dy,
                   strb->surface, sx, sy, width, height);
}
