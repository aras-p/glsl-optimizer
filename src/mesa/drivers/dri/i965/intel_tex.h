/**************************************************************************
 *
 * Copyright 2003 VMware, Inc.
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

#ifndef INTELTEX_INC
#define INTELTEX_INC

#include "main/mtypes.h"
#include "main/formats.h"
#include "brw_context.h"

struct intel_renderbuffer;

void intelInitTextureFuncs(struct dd_function_table *functions);

void intelInitTextureImageFuncs(struct dd_function_table *functions);

void intelInitTextureSubImageFuncs(struct dd_function_table *functions);

void intelInitTextureCopyImageFuncs(struct dd_function_table *functions);

void intelSetTexBuffer(__DRIcontext *pDRICtx,
		       GLint target, __DRIdrawable *pDraw);
void intelSetTexBuffer2(__DRIcontext *pDRICtx,
			GLint target, GLint format, __DRIdrawable *pDraw);

struct intel_mipmap_tree *
intel_miptree_create_for_teximage(struct brw_context *brw,
				  struct intel_texture_object *intelObj,
				  struct intel_texture_image *intelImage,
				  bool expect_accelerated_upload);

GLuint intel_finalize_mipmap_tree(struct brw_context *brw, GLuint unit);

bool
intel_texsubimage_tiled_memcpy(struct gl_context *ctx,
                               GLuint dims,
                               struct gl_texture_image *texImage,
                               GLint xoffset, GLint yoffset, GLint zoffset,
                               GLsizei width, GLsizei height, GLsizei depth,
                               GLenum format, GLenum type,
                               const GLvoid *pixels,
                               const struct gl_pixelstore_attrib *packing,
                               bool for_glTexImage);

#endif
