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

#include "p_compiler.h"

/**
 * Implementation limits
 */
#define PIPE_MAX_SAMPLERS     8
#define PIPE_MAX_CLIP_PLANES  6
#define PIPE_MAX_CONSTANT    32
#define PIPE_ATTRIB_MAX      32
#define PIPE_MAX_COLOR_BUFS   8
#define PIPE_MAX_TEXTURE_LEVELS  16


/* fwd decl */
struct pipe_surface;

/* opaque type */
struct pipe_buffer_handle;


/***
 *** State objects
 ***/


/**
 * Primitive (point/line/tri) setup info
 */
struct pipe_setup_state
{
   unsigned flatshade:1;
   unsigned light_twoside:1;
   unsigned front_winding:2;  /**< PIPE_WINDING_x */
   unsigned cull_mode:2;      /**< PIPE_WINDING_x */
   unsigned fill_cw:2;        /**< PIPE_POLYGON_MODE_x */
   unsigned fill_ccw:2;       /**< PIPE_POLYGON_MODE_x */
   unsigned offset_cw:1;
   unsigned offset_ccw:1;
   unsigned scissor:1;
   unsigned poly_smooth:1;
   unsigned poly_stipple_enable:1;
   unsigned point_smooth:1;
   unsigned multisample:1;         /* XXX maybe more ms state in future */
   unsigned line_smooth:1;
   unsigned line_stipple_enable:1;
   unsigned line_stipple_factor:8;  /**< [1..256] actually */
   unsigned line_stipple_pattern:16;

   float line_width;
   float point_size;           /**< used when no per-vertex size */
   float offset_units;
   float offset_scale;
};

struct pipe_poly_stipple {
   unsigned stipple[32];
};


struct pipe_viewport_state {
   float scale[4];
   float translate[4];
};

struct pipe_scissor_state {
   unsigned minx:16;
   unsigned miny:16;
   unsigned maxx:16;
   unsigned maxy:16;
};

struct pipe_clip_state {
   float ucp[PIPE_MAX_CLIP_PLANES][4];
   unsigned nr;
};


struct pipe_constant_buffer {
   float constant[PIPE_MAX_CONSTANT][4];
   unsigned nr_constants;
};


struct pipe_fs_state {
   unsigned inputs_read;		/* FRAG_ATTRIB_* */
   const struct tgsi_token *tokens;
   struct pipe_constant_buffer *constants; /* XXX temporary? */
};

struct pipe_depth_state
{
   unsigned enabled:1;   /**< depth test enabled? */
   unsigned writemask:1; /**< allow depth buffer writes? */
   unsigned func:3;      /**< depth test func (PIPE_FUNC_x) */
   unsigned occlusion_count:1; /**< XXX move this elsewhere? */
   float clear;      /**< Clear value in [0,1] (XXX correct place?) */
};

struct pipe_alpha_test_state {
   unsigned enabled:1;
   unsigned func:3;    /**< PIPE_FUNC_x */
   float ref;      /**< reference value */
};

struct pipe_blend_state {
   unsigned blend_enable:1;

   unsigned rgb_func:3;          /**< PIPE_BLEND_x */
   unsigned rgb_src_factor:5;    /**< PIPE_BLENDFACTOR_x */
   unsigned rgb_dst_factor:5;    /**< PIPE_BLENDFACTOR_x */

   unsigned alpha_func:3;        /**< PIPE_BLEND_x */
   unsigned alpha_src_factor:5;  /**< PIPE_BLENDFACTOR_x */
   unsigned alpha_dst_factor:5;  /**< PIPE_BLENDFACTOR_x */

   unsigned logicop_enable:1;
   unsigned logicop_func:4;      /**< PIPE_LOGICOP_x */

   unsigned colormask:4;         /**< bitmask of PIPE_MASK_R/G/B/A */
   unsigned dither:1;
};

struct pipe_blend_color {
   float color[4];
};

struct pipe_clear_color_state
{
   float color[4];
};

struct pipe_stencil_state {
   unsigned front_enabled:1;
   unsigned front_func:3;     /**< PIPE_FUNC_x */
   unsigned front_fail_op:3;  /**< PIPE_STENCIL_OP_x */
   unsigned front_zpass_op:3; /**< PIPE_STENCIL_OP_x */
   unsigned front_zfail_op:3; /**< PIPE_STENCIL_OP_x */
   unsigned back_enabled:1;
   unsigned back_func:3;      /**< PIPE_FUNC_x */
   unsigned back_fail_op:3;   /**< PIPE_STENCIL_OP_x */
   unsigned back_zpass_op:3;  /**< PIPE_STENCIL_OP_x */
   unsigned back_zfail_op:3;  /**< PIPE_STENCIL_OP_x */
   ubyte ref_value[2];    /**< [0] = front, [1] = back */
   ubyte value_mask[2];
   ubyte write_mask[2];
   ubyte clear_value;
};


struct pipe_framebuffer_state
{
   /** multiple colorbuffers for multiple render targets */
   unsigned num_cbufs;
   struct pipe_surface *cbufs[PIPE_MAX_COLOR_BUFS];

   struct pipe_surface *zbuf;      /**< Z buffer */
   struct pipe_surface *sbuf;      /**< Stencil buffer */
};


/**
 * Texture sampler state.
 */
struct pipe_sampler_state
{
   unsigned wrap_s:3;        /**< PIPE_TEX_WRAP_x */
   unsigned wrap_t:3;        /**< PIPE_TEX_WRAP_x */
   unsigned wrap_r:3;        /**< PIPE_TEX_WRAP_x */
   unsigned min_img_filter:2;    /**< PIPE_TEX_FILTER_x */
   unsigned min_mip_filter:2;    /**< PIPE_TEX_MIPFILTER_x */
   unsigned mag_img_filter:2;    /**< PIPE_TEX_FILTER_x */
   unsigned compare:1;       /**< shadow/depth compare enabled? */
   unsigned compare_mode:1;  /**< PIPE_TEX_COMPARE_x */
   unsigned compare_func:3;  /**< PIPE_FUNC_x */
   float shadow_ambient; /**< shadow test fail color/intensity */
   float min_lod;
   float max_lod;
   float lod_bias;
#if 0 /* need these? */
   int BaseLevel;     /**< min mipmap level, OpenGL 1.2 */
   int MaxLevel;      /**< max mipmap level, OpenGL 1.2 */
   float border_color[4];
#endif
   float max_anisotropy;
};


/***
 *** Resource Objects
 ***/

struct pipe_region
{
   struct pipe_buffer_handle *buffer; /**< driver private buffer handle */

   unsigned refcount; /**< Reference count for region */
   unsigned cpp;      /**< bytes per pixel */
   unsigned pitch;    /**< in pixels */
   unsigned height;   /**< in pixels */
   ubyte *map;    /**< only non-NULL when region is actually mapped */
   unsigned map_refcount;  /**< Reference count for mapping */
};


/**
 * 2D surface.  This is basically a view into a pipe_region (memory buffer).
 * May be a renderbuffer, texture mipmap level, etc.
 */
struct pipe_surface
{
   struct pipe_region *region;
   unsigned format:5;            /**< PIPE_FORMAT_x */
   unsigned width, height;
   unsigned offset;              /**< offset from start of region, in bytes */
   unsigned refcount;

   /** get block/tile of pixels from surface */
   void (*get_tile)(struct pipe_surface *ps,
                    unsigned x, unsigned y, unsigned w, unsigned h, float *p);

   /** put block/tile of pixels into surface */
   void (*put_tile)(struct pipe_surface *ps,
                    unsigned x, unsigned y, unsigned w, unsigned h, const float *p);
};


/**
 * Describes the location of each texture image within a texture region.
 */
struct pipe_mipmap_level
{
   unsigned level_offset;
   unsigned width;
   unsigned height;
   unsigned depth;
   unsigned nr_images;

   /* Explicitly store the offset of each image for each cube face or
    * depth value.  Pretty much have to accept that hardware formats
    * are going to be so diverse that there is no unified way to
    * compute the offsets of depth/cube images within a mipmap level,
    * so have to store them as a lookup table:
    */
   unsigned *image_offset;   /**< array [depth] of offsets */
};

struct pipe_mipmap_tree
{
   /* Effectively the key:
    */
   unsigned target;            /* XXX convert to PIPE_TEXTURE_x */
   unsigned internal_format;   /* XXX convert to PIPE_FORMAT_x */

   unsigned format;            /**< PIPE_FORMAT_x */
   unsigned first_level;
   unsigned last_level;

   unsigned width0, height0, depth0; /**< Level zero image dimensions */
   unsigned cpp;

   unsigned compressed:1;

   /* Derived from the above:
    */
   unsigned pitch;
   unsigned depth_pitch;          /* per-image on i945? */
   unsigned total_height;

   /* Includes image offset tables:
    */
   struct pipe_mipmap_level level[PIPE_MAX_TEXTURE_LEVELS];

   /* The data is held here:
    */
   struct pipe_region *region;

   /* These are also refcounted:
    */
   unsigned refcount;
};


/**
 * A vertex buffer.  Typically, all the vertex data/attributes for
 * drawing something will be in one buffer.  But it's also possible, for
 * example, to put colors in one buffer and texcoords in another.
 */
struct pipe_vertex_buffer
{
   unsigned pitch:11;    /**< stride to same attrib in next vertex, in bytes */
   unsigned max_index;   /**< number of vertices in this buffer */
   unsigned buffer_offset;  /**< offset to start of data in buffer, in bytes */
   struct pipe_buffer_handle *buffer;  /**< the actual buffer */
};


/**
 * Information to describe a vertex attribute (position, color, etc)
 */
struct pipe_vertex_element
{
   /** Offset of this attribute, in bytes, from the start of the vertex */
   unsigned src_offset:11;

   /** Which vertex_buffer (as given to pipe->set_vertex_buffer()) does
    * this attribute live in?
    */
   unsigned vertex_buffer_index:5;

   unsigned dst_offset:8; 
   unsigned src_format:8; 	   /**< PIPE_FORMAT_* */
};



#endif
