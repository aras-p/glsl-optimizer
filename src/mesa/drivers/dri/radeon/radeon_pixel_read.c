/*
 * Copyright (C) 2010 Maciej Cencora <m.cencora@gmail.com>
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "stdint.h"
#include "main/bufferobj.h"
#include "main/enums.h"
#include "main/image.h"
#include "main/state.h"
#include "swrast/swrast.h"

#include "radeon_common_context.h"
#include "radeon_debug.h"
#include "radeon_mipmap_tree.h"

static gl_format gl_format_and_type_to_mesa_format(GLenum format, GLenum type)
{
    switch (format)
    {
        case GL_RGB:
            switch (type) {
                case GL_UNSIGNED_SHORT_5_6_5:
                    return MESA_FORMAT_RGB565;
                case GL_UNSIGNED_SHORT_5_6_5_REV:
                    return MESA_FORMAT_RGB565_REV;
            }
            break;
        case GL_RGBA:
            switch (type) {
                case GL_UNSIGNED_BYTE:
                    return MESA_FORMAT_RGBA8888_REV;
                case GL_FLOAT:
                    return MESA_FORMAT_RGBA_FLOAT32;
                case GL_UNSIGNED_SHORT_4_4_4_4:
                    return MESA_FORMAT_ARGB4444;
                case GL_UNSIGNED_SHORT_4_4_4_4_REV:
                    return MESA_FORMAT_ARGB4444;
                case GL_UNSIGNED_SHORT_5_5_5_1:
                    return MESA_FORMAT_RGBA5551;
                case GL_UNSIGNED_SHORT_1_5_5_5_REV:
                    return MESA_FORMAT_ARGB1555_REV;
                case GL_UNSIGNED_INT_8_8_8_8:
                    return MESA_FORMAT_ARGB8888;
                case GL_UNSIGNED_INT_8_8_8_8_REV:
                    return MESA_FORMAT_ARGB8888_REV;
            }
            break;
    }

    return MESA_FORMAT_NONE;
}

static GLboolean
do_blit_readpixels(GLcontext * ctx,
                   GLint x, GLint y, GLsizei width, GLsizei height,
                   GLenum format, GLenum type,
                   const struct gl_pixelstore_attrib *pack, GLvoid * pixels)
{
    radeonContextPtr radeon = RADEON_CONTEXT(ctx);
    const struct radeon_renderbuffer *rrb = radeon_renderbuffer(ctx->ReadBuffer->_ColorReadBuffer);
    const gl_format dst_format = gl_format_and_type_to_mesa_format(format, type);
    unsigned dst_rowstride, dst_imagesize, aligned_rowstride, flip_y;
    struct radeon_bo *dst_buffer;
    GLint dst_x = 0, dst_y = 0;

    /* It's not worth if number of pixels to copy is really small */
    if (width * height < 100) {
        return GL_FALSE;
    }

    if (dst_format == MESA_FORMAT_NONE ||
        !radeon->vtbl.check_blit(dst_format) || !radeon->vtbl.blit) {
        return GL_FALSE;
    }

    if (ctx->_ImageTransferState) {
        return GL_FALSE;
    }

    if (pack->SwapBytes || pack->LsbFirst) {
        return GL_FALSE;
    }

    if (pack->RowLength > 0) {
        dst_rowstride = pack->RowLength;
    } else {
        dst_rowstride = width;
    }

    if (!_mesa_clip_copytexsubimage(ctx, &dst_x, &dst_y, &x, &y, &width, &height)) {
        return GL_TRUE;
    }
    assert(x >= 0 && y >= 0);

    aligned_rowstride = get_texture_image_row_stride(radeon, dst_format, dst_rowstride, 0);
    dst_imagesize = get_texture_image_size(dst_format,
                                           aligned_rowstride,
                                           height, 1, 0);
    dst_buffer = radeon_bo_open(radeon->radeonScreen->bom, 0, dst_imagesize, 1024, RADEON_GEM_DOMAIN_GTT, 0);

    /* Disable source Y flipping for FBOs */
    flip_y = (ctx->ReadBuffer->Name == 0);
    if (pack->Invert) {
        y = rrb->base.Height - height - y;
        flip_y = !flip_y;
    }

    if (radeon->vtbl.blit(ctx,
                          rrb->bo,
                          rrb->draw_offset,
                          rrb->base.Format,
                          rrb->pitch / rrb->cpp,
                          rrb->base.Width,
                          rrb->base.Height,
                          x,
                          y,
                          dst_buffer,
                          0, /* dst_offset */
                          dst_format,
                          aligned_rowstride / _mesa_get_format_bytes(dst_format),
                          width,
                          height,
                          0, /* dst_x */
                          0, /* dst_y */
                          width,
                          height,
                          flip_y))
    {
        radeon_bo_map(dst_buffer, 0);
        dst_rowstride *= _mesa_get_format_bytes(dst_format);
        copy_rows(pixels, dst_rowstride, dst_buffer->ptr,
                  aligned_rowstride, height, dst_rowstride);
        radeon_bo_unmap(dst_buffer);
        radeon_bo_unref(dst_buffer);
        return GL_TRUE;
    } else {
        radeon_bo_unref(dst_buffer);
        return GL_FALSE;
    }
}

void
radeonReadPixels(GLcontext * ctx,
                 GLint x, GLint y, GLsizei width, GLsizei height,
                 GLenum format, GLenum type,
                 const struct gl_pixelstore_attrib *pack, GLvoid * pixels)
{
    if (do_blit_readpixels(ctx, x, y, width, height, format, type, pack, pixels))
        return;

    /* Update Mesa state before calling down into _swrast_ReadPixels, as
     * the spans code requires the computed buffer states to be up to date,
     * but _swrast_ReadPixels only updates Mesa state after setting up
     * the spans code.
     */

    radeon_print(RADEON_FALLBACKS, RADEON_NORMAL,
                 "Falling back to sw for ReadPixels (format %s, type %s)\n",
                 _mesa_lookup_enum_by_nr(format), _mesa_lookup_enum_by_nr(type));

    if (ctx->NewState)
        _mesa_update_state(ctx);

    _swrast_ReadPixels(ctx, x, y, width, height, format, type, pack, pixels);
}
