/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#ifndef ST_FORMAT_H
#define ST_FORMAT_H


struct pipe_format_info
{
   enum pipe_format format;
   GLenum base_format;
   GLenum datatype;
   GLubyte red_bits;
   GLubyte green_bits;
   GLubyte blue_bits;
   GLubyte alpha_bits;
   GLubyte luminance_bits;
   GLubyte intensity_bits;
   GLubyte depth_bits;
   GLubyte stencil_bits;
   GLubyte size;           /**< in bytes */
};


GLboolean
st_get_format_info(enum pipe_format format, struct pipe_format_info *pinfo);


extern GLuint
st_sizeof_format(enum pipe_format format);


extern GLenum
st_format_datatype(enum pipe_format format);


extern enum pipe_format
st_mesa_format_to_pipe_format(GLuint mesaFormat);


extern enum pipe_format
st_choose_format(struct pipe_context *pipe, GLint internalFormat,
                 enum pipe_texture_target target, unsigned tex_usage);

extern enum pipe_format
st_choose_renderbuffer_format(struct pipe_context *pipe, GLint internalFormat);


extern const struct gl_texture_format *
st_ChooseTextureFormat(GLcontext * ctx, GLint internalFormat,
                       GLenum format, GLenum type);


#endif /* ST_CB_TEXIMAGE_H */
