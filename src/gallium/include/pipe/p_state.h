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
 * @file
 * 
 * Abstract graphics pipe state objects.
 *
 * Basic notes:
 *   1. Want compact representations, so we use bitfields.
 *   2. Put bitfields before other (GLfloat) fields.
 */


#ifndef PIPE_STATE_H
#define PIPE_STATE_H

#include "p_compiler.h"
#include "p_defines.h"
#include "p_format.h"
#include "p_screen.h"


#ifdef __cplusplus
extern "C" {
#endif


/**
 * Implementation limits
 */
#define PIPE_MAX_ATTRIBS          32
#define PIPE_MAX_CLIP_PLANES       6
#define PIPE_MAX_COLOR_BUFS        8
#define PIPE_MAX_CONSTANT_BUFFERS 32
#define PIPE_MAX_SAMPLERS         16
#define PIPE_MAX_VERTEX_SAMPLERS  16
#define PIPE_MAX_SHADER_INPUTS    16
#define PIPE_MAX_SHADER_OUTPUTS   16
#define PIPE_MAX_TEXTURE_LEVELS   16


struct pipe_reference
{
   int32_t count; /* atomic */
};


/**
 * The driver will certainly subclass this to include actual memory
 * management information.
 */
struct pipe_buffer
{
   struct pipe_reference  reference;
   unsigned               size;
   struct pipe_screen    *screen;
   unsigned               alignment;
   unsigned               usage;
};


/**
 * Primitive (point/line/tri) rasterization info
 */
struct pipe_rasterizer_state
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
   unsigned sprite_coord_enable:PIPE_MAX_SHADER_OUTPUTS;
   unsigned sprite_coord_mode:1;     /**< PIPE_SPRITE_COORD_ */
   unsigned point_quad_rasterization:1; /** points rasterized as quads or points */
   unsigned point_size_per_vertex:1; /**< size computed in vertex shader */
   unsigned multisample:1;         /* XXX maybe more ms state in future */
   unsigned line_smooth:1;
   unsigned line_stipple_enable:1;
   unsigned line_stipple_factor:8;  /**< [1..256] actually */
   unsigned line_stipple_pattern:16;
   unsigned line_last_pixel:1;

   /** 
    * Use the first vertex of a primitive as the provoking vertex for
    * flat shading.
    */
   unsigned flatshade_first:1;   

   /** 
    * When true, triangle rasterization uses (0.5, 0.5) pixel centers
    * for determining pixel ownership.
    *
    * When false, triangle rasterization uses (0,0) pixel centers for
    * determining pixel ownership.
    *
    * Triangle rasterization always uses a 'top,left' rule for pixel
    * ownership, this just alters which point we consider the pixel
    * center for that test.
    */
   unsigned gl_rasterization_rules:1;

   float line_width;
   float point_size;           /**< used when no per-vertex size */
   float offset_units;
   float offset_scale;
};


struct pipe_poly_stipple
{
   unsigned stipple[32];
};


struct pipe_viewport_state
{
   float scale[4];
   float translate[4];
};


struct pipe_scissor_state
{
   unsigned minx:16;
   unsigned miny:16;
   unsigned maxx:16;
   unsigned maxy:16;
};


struct pipe_clip_state
{
   float ucp[PIPE_MAX_CLIP_PLANES][4];
   unsigned nr;
};


struct pipe_shader_state
{
   const struct tgsi_token *tokens;
};


struct pipe_depth_state 
{
   unsigned enabled:1;         /**< depth test enabled? */
   unsigned writemask:1;       /**< allow depth buffer writes? */
   unsigned func:3;            /**< depth test func (PIPE_FUNC_x) */
};


struct pipe_stencil_state
{
   unsigned enabled:1;  /**< stencil[0]: stencil enabled, stencil[1]: two-side enabled */
   unsigned func:3;     /**< PIPE_FUNC_x */
   unsigned fail_op:3;  /**< PIPE_STENCIL_OP_x */
   unsigned zpass_op:3; /**< PIPE_STENCIL_OP_x */
   unsigned zfail_op:3; /**< PIPE_STENCIL_OP_x */
   unsigned valuemask:8;
   unsigned writemask:8;
};


struct pipe_alpha_state
{
   unsigned enabled:1;
   unsigned func:3;     /**< PIPE_FUNC_x */
   float ref_value;     /**< reference value */
};


struct pipe_depth_stencil_alpha_state
{
   struct pipe_depth_state depth;
   struct pipe_stencil_state stencil[2]; /**< [0] = front, [1] = back */
   struct pipe_alpha_state alpha;
};


struct pipe_rt_blend_state
{
   unsigned blend_enable:1;

   unsigned rgb_func:3;          /**< PIPE_BLEND_x */
   unsigned rgb_src_factor:5;    /**< PIPE_BLENDFACTOR_x */
   unsigned rgb_dst_factor:5;    /**< PIPE_BLENDFACTOR_x */

   unsigned alpha_func:3;        /**< PIPE_BLEND_x */
   unsigned alpha_src_factor:5;  /**< PIPE_BLENDFACTOR_x */
   unsigned alpha_dst_factor:5;  /**< PIPE_BLENDFACTOR_x */

   unsigned colormask:4;         /**< bitmask of PIPE_MASK_R/G/B/A */
};

struct pipe_blend_state
{
   unsigned independent_blend_enable:1;
   unsigned logicop_enable:1;
   unsigned logicop_func:4;      /**< PIPE_LOGICOP_x */
   unsigned dither:1;
   struct pipe_rt_blend_state rt[PIPE_MAX_COLOR_BUFS];
};


struct pipe_blend_color
{
   float color[4];
};

struct pipe_stencil_ref
{
   ubyte ref_value[2];
};

struct pipe_framebuffer_state
{
   unsigned width, height;

   /** multiple colorbuffers for multiple render targets */
   unsigned nr_cbufs;
   struct pipe_surface *cbufs[PIPE_MAX_COLOR_BUFS];

   struct pipe_surface *zsbuf;      /**< Z/stencil buffer */
};


/**
 * Texture sampler state.
 */
struct pipe_sampler_state
{
   unsigned wrap_s:3;            /**< PIPE_TEX_WRAP_x */
   unsigned wrap_t:3;            /**< PIPE_TEX_WRAP_x */
   unsigned wrap_r:3;            /**< PIPE_TEX_WRAP_x */
   unsigned min_img_filter:2;    /**< PIPE_TEX_FILTER_x */
   unsigned min_mip_filter:2;    /**< PIPE_TEX_MIPFILTER_x */
   unsigned mag_img_filter:2;    /**< PIPE_TEX_FILTER_x */
   unsigned compare_mode:1;      /**< PIPE_TEX_COMPARE_x */
   unsigned compare_func:3;      /**< PIPE_FUNC_x */
   unsigned normalized_coords:1; /**< Are coords normalized to [0,1]? */
   unsigned max_anisotropy:6;
   float lod_bias;               /**< LOD/lambda bias */
   float min_lod, max_lod;       /**< LOD clamp range, after bias */
   float border_color[4];
};


/**
 * 2D surface.  This is basically a view into a memory buffer.
 * May be a renderbuffer, texture mipmap level, etc.
 */
struct pipe_surface
{
   struct pipe_reference reference;
   enum pipe_format format;      /**< PIPE_FORMAT_x */
   unsigned width;               /**< logical width in pixels */
   unsigned height;              /**< logical height in pixels */
   unsigned layout;              /**< PIPE_SURFACE_LAYOUT_x */
   unsigned offset;              /**< offset from start of buffer, in bytes */
   unsigned usage;               /**< PIPE_BUFFER_USAGE_*  */

   unsigned zslice;
   struct pipe_texture *texture; /**< texture into which this is a view  */
   unsigned face;
   unsigned level;
};


/**
 * Transfer object.  For data transfer to/from a texture.
 */
struct pipe_transfer
{
   unsigned x;                   /**< x offset from start of texture image */
   unsigned y;                   /**< y offset from start of texture image */
   unsigned width;               /**< logical width in pixels */
   unsigned height;              /**< logical height in pixels */
   unsigned stride;              /**< stride in bytes between rows of blocks */
   enum pipe_transfer_usage usage; /**< PIPE_TRANSFER_*  */

   struct pipe_texture *texture; /**< texture to transfer to/from  */
   unsigned face;
   unsigned level;
   unsigned zslice;
};


/**
 * Texture object.
 */
struct pipe_texture
{ 
   struct pipe_reference reference;

   enum pipe_texture_target target; /**< PIPE_TEXTURE_x */
   enum pipe_format format;         /**< PIPE_FORMAT_x */

   unsigned width0;
   unsigned height0;
   unsigned depth0;

   unsigned last_level:8;    /**< Index of last mipmap level present/defined */

   unsigned nr_samples:8;    /**< for multisampled surfaces, nr of samples */

   unsigned tex_usage;       /* PIPE_TEXTURE_USAGE_* */

   struct pipe_screen *screen; /**< screen that this texture belongs to */
};


/**
 * A vertex buffer.  Typically, all the vertex data/attributes for
 * drawing something will be in one buffer.  But it's also possible, for
 * example, to put colors in one buffer and texcoords in another.
 */
struct pipe_vertex_buffer
{
   unsigned stride;    /**< stride to same attrib in next vertex, in bytes */
   unsigned max_index;   /**< number of vertices in this buffer */
   unsigned buffer_offset;  /**< offset to start of data in buffer, in bytes */
   struct pipe_buffer *buffer;  /**< the actual buffer */
};


/**
 * Information to describe a vertex attribute (position, color, etc)
 */
struct pipe_vertex_element
{
   /** Offset of this attribute, in bytes, from the start of the vertex */
   unsigned src_offset;

   /** Instance data rate divisor. 0 means this is per-vertex data,
    *  n means per-instance data used for n consecutive instances (n > 0).
    */
   unsigned instance_divisor;

   /** Which vertex_buffer (as given to pipe->set_vertex_buffer()) does
    * this attribute live in?
    */
   unsigned vertex_buffer_index:8;
   unsigned nr_components:8;
 
   enum pipe_format src_format; 	   /**< PIPE_FORMAT_* */
};


#ifdef __cplusplus
}
#endif
   
#endif
