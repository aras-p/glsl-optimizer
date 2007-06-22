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

#ifndef PIPE_STATE_H
#define PIPE_STATE_H

#include "mtypes.h"

#define WINDING_NONE 0
#define WINDING_CW   1
#define WINDING_CCW  2
#define WINDING_BOTH (WINDING_CW | WINDING_CCW)

#define FILL_POINT 1
#define FILL_LINE  2
#define FILL_TRI   3

struct pipe_setup_state {
   GLuint flatshade:1;
   GLuint light_twoside:1;

   GLuint front_winding:2;

   GLuint cull_mode:2;

   GLuint fill_cw:2;
   GLuint fill_ccw:2;

   GLuint offset_cw:1;
   GLuint offset_ccw:1;

   GLuint scissor:1;
   GLuint poly_stipple:1;

   GLuint pad:18;

   GLfloat offset_units;
   GLfloat offset_scale;
};

struct pipe_poly_stipple {
   GLuint stipple[32];
};


struct pipe_viewport {
   GLfloat scale[4];
   GLfloat translate[4];
};

struct pipe_scissor_rect {
   GLshort minx;
   GLshort miny;
   GLshort maxx;
   GLshort maxy;
};


#define PIPE_MAX_CLIP_PLANES 6

struct pipe_clip_state {
   GLfloat ucp[PIPE_MAX_CLIP_PLANES][4];
   GLuint nr;
};

struct pipe_fs_state {
   struct gl_fragment_program *fp;
};

#define PIPE_MAX_CONSTANT 32

struct pipe_constant_buffer {
   GLfloat constant[PIPE_MAX_CONSTANT][4];
   GLuint nr_constants;
};


struct pipe_depth_state
{
   GLuint enabled:1;   /**< depth test enabled? */
   GLuint writemask:1; /**< allow depth buffer writes? */
   GLuint func:3;      /**< depth test func (PIPE_FUNC_x) */
   GLfloat clear;      /**< Clear value in [0,1] (XXX correct place?) */
};

struct pipe_alpha_test_state {
   GLuint enabled:1;
   GLuint func:3;    /**< PIPE_FUNC_x */
   GLfloat ref;      /**< reference value */
};

struct pipe_blend_state {
   GLuint blend_enable:1;

   GLuint rgb_func:3;
   GLuint rgb_src_factor:5;
   GLuint rgb_dst_factor:5;

   GLuint alpha_func:3;
   GLuint alpha_src_factor:5;
   GLuint alpha_dst_factor:5;

   GLuint logicop_enable:1;
   GLuint logicop_func:4;
};

struct pipe_blend_color {
   GLfloat color[4];
};

struct pipe_clear_color_state
{
   GLfloat color[4];
};

struct pipe_line_state
{
   GLuint smooth:1;
   GLuint stipple:1;
   GLushort stipple_pattern;
   GLint stipple_factor;
   GLfloat width;
};

struct pipe_point_state
{
   GLuint smooth:1;
   GLfloat size;
   GLfloat min_size, max_size;
   GLfloat attenuation[3];
};

struct pipe_polygon_state {
   GLuint cull_mode:2;     /**< PIPE_POLYGON_CULL_x */
   GLuint front_winding:1; /**< PIPE_POLYGON_FRONT_CCW,CW */
   GLuint front_mode:2;    /**< PIPE_POLYGON_MODE_x */
   GLuint back_mode:2;     /**< PIPE_POLYGON_MODE_x */
   GLuint stipple:1;       /**< enable */
   GLuint smooth:1;        /**< enable */
   /* XXX Polygon offset? */
};

struct pipe_stencil_state {
   GLuint front_enabled:1;
   GLuint front_func:3;     /**< PIPE_FUNC_x */
   GLuint front_fail_op:3;  /**< PIPE_STENCIL_OP_x */
   GLuint front_zpass_op:3; /**< PIPE_STENCIL_OP_x */
   GLuint front_zfail_op:3; /**< PIPE_STENCIL_OP_x */
   GLuint back_enabled:1;
   GLuint back_func:3;
   GLuint back_fail_op:3;
   GLuint back_zpass_op:3;
   GLuint back_zfail_op:3;
   GLubyte ref_value[2];      /**< [0] = front, [1] = back */
   GLubyte value_mask[2];
   GLubyte write_mask[2];
   GLubyte clear_value;
};


/* This will change for hardware pipes...
 */
struct pipe_surface
{
   GLuint width, height;
   GLubyte *ptr;
   GLint stride;
   GLuint cpp;
   GLuint format;
};


struct pipe_framebuffer_state
{
   GLuint num_cbufs;               /**< Number of color bufs to draw to */
   struct pipe_surface *cbufs[4];  /**< OpenGL can write to as many as
                                        4 color buffers at once */
   struct pipe_surface *zbuf;      /**< Z buffer */
   struct pipe_surface *sbuf;      /**< Stencil buffer */
   struct pipe_surface *abuf;      /**< Accum buffer */
};


/**
 * Texture sampler state.
 */
struct pipe_sampler_state
{
   GLuint wrap_s:3;        /**< PIPE_TEX_WRAP_x */
   GLuint wrap_t:3;        /**< PIPE_TEX_WRAP_x */
   GLuint wrap_r:3;        /**< PIPE_TEX_WRAP_x */
   GLuint min_filter:3;    /**< PIPE_TEX_FILTER_x */
   GLuint mag_filter:1;    /**< PIPE_TEX_FILTER_LINEAR or _NEAREST */
   GLfloat min_lod;
   GLfloat max_lod;
   GLfloat lod_bias;
#if 0 /* need these? */
   GLint BaseLevel;     /**< min mipmap level, OpenGL 1.2 */
   GLint MaxLevel;      /**< max mipmap level, OpenGL 1.2 */
#endif
   GLfloat max_anisotropy;
   GLuint compare:1;       /**< shadow/depth compare enabled? */
   GLenum compare_mode:1;  /**< PIPE_TEX_COMPARE_x */
   GLenum compare_func:3;  /**< PIPE_FUNC_x */
   GLfloat shadow_ambient; /**< shadow test fail color/intensity */
};

#endif
