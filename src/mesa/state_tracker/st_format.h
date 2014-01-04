/**************************************************************************
 * 
 * Copyright 2007 VMware, Inc.
 * Copyright (c) 2010 VMware, Inc.
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


#ifndef ST_FORMAT_H
#define ST_FORMAT_H

#include "main/formats.h"
#include "main/glheader.h"

#include "pipe/p_defines.h"
#include "pipe/p_format.h"

struct gl_context;
struct pipe_screen;


extern enum pipe_format
st_mesa_format_to_pipe_format(mesa_format mesaFormat);

extern mesa_format
st_pipe_format_to_mesa_format(enum pipe_format pipeFormat);


extern enum pipe_format
st_choose_format(struct st_context *st, GLenum internalFormat,
                 GLenum format, GLenum type,
                 enum pipe_texture_target target, unsigned sample_count,
                 unsigned bindings, boolean allow_dxt);

extern enum pipe_format
st_choose_renderbuffer_format(struct st_context *st,
                              GLenum internalFormat, unsigned sample_count);

extern enum pipe_format
st_choose_matching_format(struct pipe_screen *screen, unsigned bind,
			  GLenum format, GLenum type, GLboolean swapBytes);

extern mesa_format
st_ChooseTextureFormat(struct gl_context * ctx, GLenum target,
                       GLint internalFormat,
                       GLenum format, GLenum type);

size_t
st_QuerySamplesForFormat(struct gl_context *ctx, GLenum target,
                         GLenum internalFormat, int samples[16]);



extern void
st_translate_color(const union gl_color_union *colorIn,
                   union pipe_color_union *colorOut,
                   GLenum baseFormat, GLboolean is_integer);

#endif /* ST_FORMAT_H */
