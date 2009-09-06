/**************************************************************************
 *
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright 2009 Intel Corporation.
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

#ifndef DRI_METAOPS_H
#define DRI_METAOPS_H


struct dri_metaops {
    GLcontext *ctx;
    GLboolean internal_viewport_call;
    struct gl_fragment_program *bitmap_fp;
    struct gl_vertex_program *passthrough_vp;
    struct gl_buffer_object *texcoord_vbo;
    
    struct gl_fragment_program *saved_fp;
    GLboolean saved_fp_enable;
    struct gl_vertex_program *saved_vp;
    GLboolean saved_vp_enable;

    struct gl_fragment_program *tex2d_fp;
    
    GLboolean saved_texcoord_enable;
    struct gl_buffer_object *saved_array_vbo, *saved_texcoord_vbo;
    GLenum saved_texcoord_type;
    GLsizei saved_texcoord_size, saved_texcoord_stride;
    const void *saved_texcoord_ptr;
    int saved_active_texture;

    GLint saved_vp_x, saved_vp_y;
    GLsizei saved_vp_width, saved_vp_height;
    GLenum saved_matrix_mode;
};


void meta_set_passthrough_transform(struct dri_metaops *meta);

void meta_restore_transform(struct dri_metaops *meta);

void meta_set_passthrough_vertex_program(struct dri_metaops *meta);

void meta_restore_vertex_program(struct dri_metaops *meta);

void meta_set_fragment_program(struct dri_metaops *meta,
			  struct gl_fragment_program **prog,
			  const char *prog_string);

void meta_restore_fragment_program(struct dri_metaops *meta);

void meta_set_default_texrect(struct dri_metaops *meta);

void meta_restore_texcoords(struct dri_metaops *meta);

void meta_init_metaops(GLcontext *ctx, struct dri_metaops *meta);
void meta_destroy_metaops(struct dri_metaops *meta);

#endif
