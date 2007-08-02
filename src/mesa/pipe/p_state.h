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


/**
 * Abstract graphics pipe state objects.
 *
 * Basic notes:
 *   1. Want compact representations, so we use bitfields.
 *   2. Put bitfields before other (GLfloat) fields.
 */


#ifndef PIPE_STATE_H
#define PIPE_STATE_H

#include "mtypes.h"



/**
 * Implementation limits
 */
#define PIPE_MAX_SAMPLERS     8
#define PIPE_MAX_CLIP_PLANES  6
#define PIPE_MAX_CONSTANT    32
#define PIPE_ATTRIB_MAX      32
#define PIPE_MAX_COLOR_BUFS   8


/* fwd decl */
struct pipe_surface;


/***
 *** State objects
 ***/


/**
 * Primitive (point/line/tri) setup info
 */
struct pipe_setup_state
{
   GLuint flatshade:1;
   GLuint light_twoside:1;

   GLuint front_winding:2;  /**< PIPE_WINDING_x */

   GLuint cull_mode:2;      /**< PIPE_WINDING_x */

   GLuint fill_cw:2;        /**< PIPE_POLYGON_MODE_x */
   GLuint fill_ccw:2;       /**< PIPE_POLYGON_MODE_x */

   GLuint offset_cw:1;
   GLuint offset_ccw:1;

   GLuint scissor:1;

   GLuint poly_smooth:1;
   GLuint poly_stipple_enable:1;

   GLuint line_smooth:1;
   GLuint line_stipple_enable:1;

   GLuint point_smooth:1;

   GLuint multisample:1;         /* XXX maybe more ms state in future */

   GLubyte line_stipple_factor;  /**< [1..256] actually */
   GLushort line_stipple_pattern;
   GLfloat line_width;
   GLfloat point_size;           /**< used when no per-vertex size */
   GLfloat offset_units;
   GLfloat offset_scale;
};

struct pipe_poly_stipple {
   GLuint stipple[32];
};


struct pipe_viewport_state {
   GLfloat scale[4];
   GLfloat translate[4];
};

struct pipe_scissor_state {
   GLshort minx;
   GLshort miny;
   GLshort maxx;
   GLshort maxy;
};

struct pipe_clip_state {
   GLfloat ucp[PIPE_MAX_CLIP_PLANES][4];
   GLuint nr;
};


struct pipe_constant_buffer {
   GLfloat constant[PIPE_MAX_CONSTANT][4];
   GLuint nr_constants;
};


struct pipe_fs_state {
   GLbitfield inputs_read;		/* FRAG_ATTRIB_* */
   const struct tgsi_token *tokens;
   struct pipe_constant_buffer *constants; /* XXX temporary? */
};

struct pipe_depth_state
{
   GLuint enabled:1;   /**< depth test enabled? */
   GLuint writemask:1; /**< allow depth buffer writes? */
   GLuint func:3;      /**< depth test func (PIPE_FUNC_x) */
   GLuint occlusion_count:1; /**< XXX move this elsewhere? */
   GLfloat clear;      /**< Clear value in [0,1] (XXX correct place?) */
};

struct pipe_alpha_test_state {
   GLuint enabled:1;
   GLuint func:3;    /**< PIPE_FUNC_x */
   GLfloat ref;      /**< reference value */
};

struct pipe_blend_state {
   GLuint blend_enable:1;

   GLuint rgb_func:3;          /**< PIPE_BLEND_x */
   GLuint rgb_src_factor:5;    /**< PIPE_BLENDFACTOR_x */
   GLuint rgb_dst_factor:5;    /**< PIPE_BLENDFACTOR_x */

   GLuint alpha_func:3;        /**< PIPE_BLEND_x */
   GLuint alpha_src_factor:5;  /**< PIPE_BLENDFACTOR_x */
   GLuint alpha_dst_factor:5;  /**< PIPE_BLENDFACTOR_x */

   GLuint logicop_enable:1;
   GLuint logicop_func:4;      /**< PIPE_LOGICOP_x */

   GLuint colormask:4;         /**< bitmask of PIPE_MASK_R/G/B/A */
   GLuint dither:1;
};

struct pipe_blend_color {
   GLfloat color[4];
};

struct pipe_clear_color_state
{
   GLfloat color[4];
};

struct pipe_stencil_state {
   GLuint front_enabled:1;
   GLuint front_func:3;     /**< PIPE_FUNC_x */
   GLuint front_fail_op:3;  /**< PIPE_STENCIL_OP_x */
   GLuint front_zpass_op:3; /**< PIPE_STENCIL_OP_x */
   GLuint front_zfail_op:3; /**< PIPE_STENCIL_OP_x */
   GLuint back_enabled:1;
   GLuint back_func:3;      /**< PIPE_FUNC_x */
   GLuint back_fail_op:3;   /**< PIPE_STENCIL_OP_x */
   GLuint back_zpass_op:3;  /**< PIPE_STENCIL_OP_x */
   GLuint back_zfail_op:3;  /**< PIPE_STENCIL_OP_x */
   GLubyte ref_value[2];    /**< [0] = front, [1] = back */
   GLubyte value_mask[2];
   GLubyte write_mask[2];
   GLubyte clear_value;
};


struct pipe_framebuffer_state
{
   /** multiple colorbuffers for multiple render targets */
   GLuint num_cbufs;
   struct pipe_surface *cbufs[PIPE_MAX_COLOR_BUFS];

   struct pipe_surface *zbuf;      /**< Z buffer */
   struct pipe_surface *sbuf;      /**< Stencil buffer */
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
   GLuint compare:1;       /**< shadow/depth compare enabled? */
   GLenum compare_mode:1;  /**< PIPE_TEX_COMPARE_x */
   GLenum compare_func:3;  /**< PIPE_FUNC_x */
   GLfloat shadow_ambient; /**< shadow test fail color/intensity */
   GLfloat min_lod;
   GLfloat max_lod;
   GLfloat lod_bias;
#if 0 /* need these? */
   GLint BaseLevel;     /**< min mipmap level, OpenGL 1.2 */
   GLint MaxLevel;      /**< max mipmap level, OpenGL 1.2 */
   GLfloat border_color[4];
#endif
   GLfloat max_anisotropy;
};


/***
 *** Resource Objects
 ***/


struct _DriBufferObject;

struct pipe_region
{
   struct _DriBufferObject *buffer;   /**< buffer manager's buffer ID */

   GLuint refcount; /**< Reference count for region */
   GLuint cpp;      /**< bytes per pixel */
   GLuint pitch;    /**< in pixels */
   GLuint height;   /**< in pixels */
   GLubyte *map;    /**< only non-NULL when region is actually mapped */
   GLuint map_refcount;  /**< Reference count for mapping */

   GLuint draw_offset; /**< Offset of drawing address within the region */
};


/**
 * 2D surface.
 * May be a renderbuffer, texture mipmap level, etc.
 */
struct pipe_surface
{
   struct pipe_region *region;
   GLuint format:5;            /**< PIPE_FORMAT_x */
   GLuint width, height;

   void *rb;  /**< Ptr back to renderbuffer (temporary?) */
};


/**
 * Texture object.
 * Mipmap levels, cube faces, 3D slices can be accessed as surfaces.
 */
struct pipe_texture_object
{
   GLuint type:2;      /**< PIPE_TEXTURE_x */
   GLuint format:5;    /**< PIPE_FORMAT_x */
   GLuint width:13;    /**< 13 bits = 8K max size */
   GLuint height:13;
   GLuint depth:13;
   GLuint mipmapped:1;

   /** to access a 1D or 2D texture object as a surface */
   struct pipe_surface *(*get_2d_surface)(struct pipe_texture_object *pto,
                                          GLuint level);
   /** to access a 3D texture object as a surface */
   struct pipe_surface *(*get_3d_surface)(struct pipe_texture_object *pto,
                                          GLuint level, GLuint slice);
   /** to access a cube texture object as a surface */
   struct pipe_surface *(*get_cube_surface)(struct pipe_texture_object *pto,
                                            GLuint face, GLuint level);
   /** when finished with surface: */
   void (*release_surface)(struct pipe_texture_object *pto,
                           struct pipe_surface *ps);
};


#endif
