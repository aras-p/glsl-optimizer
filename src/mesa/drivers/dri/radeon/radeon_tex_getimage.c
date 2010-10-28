/*
 * Copyright (C) 2009 Maciej Cencora.
 * Copyright (C) 2008 Nicolai Haehnle.
 * Copyright (C) The Weather Channel, Inc.  2002.  All Rights Reserved.
 *
 * The Weather Channel (TM) funded Tungsten Graphics to develop the
 * initial release of the Radeon 8500 driver under the XFree86 license.
 * This notice must be preserved.
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

#include "radeon_common_context.h"
#include "radeon_texture.h"
#include "radeon_mipmap_tree.h"

#include "main/texgetimage.h"

/**
 * Need to map texture image into memory before copying image data,
 * then unmap it.
 */
static void
radeon_get_tex_image(struct gl_context * ctx, GLenum target, GLint level,
             GLenum format, GLenum type, GLvoid * pixels,
             struct gl_texture_object *texObj,
             struct gl_texture_image *texImage, int compressed)
{
    radeon_texture_image *image = get_radeon_texture_image(texImage);

    radeon_print(RADEON_TEXTURE, RADEON_NORMAL,
                 "%s(%p, tex %p, image %p) compressed %d.\n",
                 __func__, ctx, texObj, image, compressed);

    if (image->mt) {
        radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
        /* Map the texture image read-only */
        if (radeon_bo_is_referenced_by_cs(image->mt->bo, rmesa->cmdbuf.cs)) {
            radeon_print(RADEON_TEXTURE, RADEON_VERBOSE,
                "%s: called for texture that is queued for GPU processing\n",
                __func__);
            radeon_firevertices(rmesa);
        }

        radeon_teximage_map(image, GL_FALSE);
    } else {
        /* Image hasn't been uploaded to a miptree yet */
        assert(image->base.Data);
    }

    if (compressed) {
        /* FIXME: this can't work for small textures (mips) which
                 use different hw stride */
        _mesa_get_compressed_teximage(ctx, target, level, pixels,
                          texObj, texImage);
    } else {
        _mesa_get_teximage(ctx, target, level, format, type, pixels,
                   texObj, texImage);
    }

    if (image->mt) {
        radeon_teximage_unmap(image);
    }
}

void
radeonGetTexImage(struct gl_context * ctx, GLenum target, GLint level,
          GLenum format, GLenum type, GLvoid * pixels,
          struct gl_texture_object *texObj,
          struct gl_texture_image *texImage)
{
    radeon_get_tex_image(ctx, target, level, format, type, pixels,
                 texObj, texImage, 0);
}

void
radeonGetCompressedTexImage(struct gl_context *ctx, GLenum target, GLint level,
                GLvoid *pixels,
                struct gl_texture_object *texObj,
                struct gl_texture_image *texImage)
{
    radeon_get_tex_image(ctx, target, level, 0, 0, pixels,
                 texObj, texImage, 1);
}
