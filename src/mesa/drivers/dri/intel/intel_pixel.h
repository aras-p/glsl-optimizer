/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef INTEL_PIXEL_H
#define INTEL_PIXEL_H

#include "main/mtypes.h"

void intelInitPixelFuncs(struct dd_function_table *functions);
void intel_meta_set_passthrough_transform(struct intel_context *intel);
void intel_meta_restore_transform(struct intel_context *intel);
void intel_meta_set_passthrough_vertex_program(struct intel_context *intel);
void intel_meta_restore_vertex_program(struct intel_context *intel);
void intel_meta_set_fragment_program(struct intel_context *intel,
				     struct gl_fragment_program **prog,
				     const char *prog_string);
void intel_meta_restore_fragment_program(struct intel_context *intel);
void intel_free_pixel_state(struct intel_context *intel);

GLboolean intel_check_blit_fragment_ops(GLcontext * ctx,
					GLboolean src_alpha_is_one);

GLboolean intel_check_meta_tex_fragment_ops(GLcontext * ctx);

GLboolean intel_check_blit_format(struct intel_region *region,
                                  GLenum format, GLenum type);


void intelReadPixels(GLcontext * ctx,
                     GLint x, GLint y,
                     GLsizei width, GLsizei height,
                     GLenum format, GLenum type,
                     const struct gl_pixelstore_attrib *pack,
                     GLvoid * pixels);

void intelDrawPixels(GLcontext * ctx,
                     GLint x, GLint y,
                     GLsizei width, GLsizei height,
                     GLenum format,
                     GLenum type,
                     const struct gl_pixelstore_attrib *unpack,
                     const GLvoid * pixels);

void intelCopyPixels(GLcontext * ctx,
                     GLint srcx, GLint srcy,
                     GLsizei width, GLsizei height,
                     GLint destx, GLint desty, GLenum type);

void intelBitmap(GLcontext * ctx,
		 GLint x, GLint y,
		 GLsizei width, GLsizei height,
		 const struct gl_pixelstore_attrib *unpack,
		 const GLubyte * pixels);

#endif
