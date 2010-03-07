/*
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

#ifndef RADEON_TEXTURE_H
#define RADEON_TEXTURE_H

#include "main/formats.h"

void copy_rows(void* dst, GLuint dststride, const void* src, GLuint srcstride,
	GLuint numrows, GLuint rowsize);
struct gl_texture_image *radeonNewTextureImage(GLcontext *ctx);
void radeonFreeTexImageData(GLcontext *ctx, struct gl_texture_image *timage);

void radeon_teximage_map(radeon_texture_image *image, GLboolean write_enable);
void radeon_teximage_unmap(radeon_texture_image *image);
void radeonMapTexture(GLcontext *ctx, struct gl_texture_object *texObj);
void radeonUnmapTexture(GLcontext *ctx, struct gl_texture_object *texObj);
void radeonGenerateMipmap(GLcontext* ctx, GLenum target, struct gl_texture_object *texObj);
int radeon_validate_texture_miptree(GLcontext * ctx, struct gl_texture_object *texObj);

gl_format radeonChooseTextureFormat_mesa(GLcontext * ctx,
                                         GLint internalFormat,
                                         GLenum format,
                                         GLenum type);

gl_format radeonChooseTextureFormat(GLcontext * ctx,
                                    GLint internalFormat,
                                    GLenum format,
                                    GLenum type, GLboolean fbo);

void radeonTexImage1D(GLcontext * ctx, GLenum target, GLint level,
		      GLint internalFormat,
		      GLint width, GLint border,
		      GLenum format, GLenum type, const GLvoid * pixels,
		      const struct gl_pixelstore_attrib *packing,
		      struct gl_texture_object *texObj,
		      struct gl_texture_image *texImage);
void radeonTexImage2D(GLcontext * ctx, GLenum target, GLint level,
		      GLint internalFormat,
		      GLint width, GLint height, GLint border,
		      GLenum format, GLenum type, const GLvoid * pixels,
		      const struct gl_pixelstore_attrib *packing,
		      struct gl_texture_object *texObj,
		      struct gl_texture_image *texImage);
void radeonCompressedTexImage2D(GLcontext * ctx, GLenum target,
				GLint level, GLint internalFormat,
				GLint width, GLint height, GLint border,
				GLsizei imageSize, const GLvoid * data,
				struct gl_texture_object *texObj,
				struct gl_texture_image *texImage);
void radeonTexImage3D(GLcontext * ctx, GLenum target, GLint level,
		      GLint internalFormat,
		      GLint width, GLint height, GLint depth,
		      GLint border,
		      GLenum format, GLenum type, const GLvoid * pixels,
		      const struct gl_pixelstore_attrib *packing,
		      struct gl_texture_object *texObj,
		      struct gl_texture_image *texImage);
void radeonTexSubImage1D(GLcontext * ctx, GLenum target, GLint level,
			 GLint xoffset,
			 GLsizei width,
			 GLenum format, GLenum type,
			 const GLvoid * pixels,
			 const struct gl_pixelstore_attrib *packing,
			 struct gl_texture_object *texObj,
			 struct gl_texture_image *texImage);
void radeonTexSubImage2D(GLcontext * ctx, GLenum target, GLint level,
				GLint xoffset, GLint yoffset,
				GLsizei width, GLsizei height,
				GLenum format, GLenum type,
				const GLvoid * pixels,
				const struct gl_pixelstore_attrib *packing,
				struct gl_texture_object *texObj,
				struct gl_texture_image *texImage);
void radeonCompressedTexSubImage2D(GLcontext * ctx, GLenum target,
				   GLint level, GLint xoffset,
				   GLint yoffset, GLsizei width,
				   GLsizei height, GLenum format,
				   GLsizei imageSize, const GLvoid * data,
				   struct gl_texture_object *texObj,
				   struct gl_texture_image *texImage);

void radeonTexSubImage3D(GLcontext * ctx, GLenum target, GLint level,
			 GLint xoffset, GLint yoffset, GLint zoffset,
			 GLsizei width, GLsizei height, GLsizei depth,
			 GLenum format, GLenum type,
			 const GLvoid * pixels,
			 const struct gl_pixelstore_attrib *packing,
			 struct gl_texture_object *texObj,
			 struct gl_texture_image *texImage);

void radeonGetTexImage(GLcontext * ctx, GLenum target, GLint level,
		       GLenum format, GLenum type, GLvoid * pixels,
		       struct gl_texture_object *texObj,
		       struct gl_texture_image *texImage);
void radeonGetCompressedTexImage(GLcontext *ctx, GLenum target, GLint level,
				 GLvoid *pixels,
				 struct gl_texture_object *texObj,
				 struct gl_texture_image *texImage);

void radeonCopyTexImage2D(GLcontext *ctx, GLenum target, GLint level,
			GLenum internalFormat,
			GLint x, GLint y, GLsizei width, GLsizei height,
			GLint border);

void radeonCopyTexSubImage2D(GLcontext *ctx, GLenum target, GLint level,
			GLint xoffset, GLint yoffset,
			GLint x, GLint y,
			GLsizei width, GLsizei height);

#endif
