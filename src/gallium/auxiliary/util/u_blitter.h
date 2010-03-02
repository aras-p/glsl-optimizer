/**************************************************************************
 *
 * Copyright 2009 Marek Olšák <maraeo@gmail.com>
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

#ifndef U_BLITTER_H
#define U_BLITTER_H

#include "util/u_memory.h"

#include "pipe/p_state.h"


#ifdef __cplusplus
extern "C" {
#endif

struct pipe_context;

struct blitter_context
{
   /* Private members, really. */
   void *saved_blend_state;   /**< blend state */
   void *saved_dsa_state;     /**< depth stencil alpha state */
   void *saved_rs_state;      /**< rasterizer state */
   void *saved_fs, *saved_vs; /**< fragment shader, vertex shader */

   struct pipe_framebuffer_state saved_fb_state;  /**< framebuffer state */
   struct pipe_stencil_ref saved_stencil_ref;     /**< stencil ref */
   struct pipe_viewport_state saved_viewport;
   struct pipe_clip_state saved_clip;

   int saved_num_sampler_states;
   void *saved_sampler_states[32];

   int saved_num_textures;
   struct pipe_texture *saved_textures[32]; /* is 32 enough? */
};

/**
 * Create a blitter context.
 */
struct blitter_context *util_blitter_create(struct pipe_context *pipe);

/**
 * Destroy a blitter context.
 */
void util_blitter_destroy(struct blitter_context *blitter);

/*
 * These CSOs must be saved before any of the following functions is called:
 * - blend state
 * - depth stencil alpha state
 * - rasterizer state
 * - vertex shader
 * - fragment shader
 */

/**
 * Clear a specified set of currently bound buffers to specified values.
 */
void util_blitter_clear(struct blitter_context *blitter,
                        unsigned width, unsigned height,
                        unsigned num_cbufs,
                        unsigned clear_buffers,
                        const float *rgba,
                        double depth, unsigned stencil);

/**
 * Copy a block of pixels from one surface to another.
 *
 * You can copy from any color format to any other color format provided
 * the former can be sampled and the latter can be rendered to. Otherwise,
 * a software fallback path is taken and both surfaces must be of the same
 * format.
 *
 * The same holds for depth-stencil formats with the exception that stencil
 * cannot be copied unless you set ignore_stencil to FALSE. In that case,
 * a software fallback path is taken and both surfaces must be of the same
 * format.
 *
 * Use pipe_screen->is_format_supported to know your options.
 *
 * These states must be saved in the blitter in addition to the state objects
 * already required to be saved:
 * - framebuffer state
 * - fragment sampler states
 * - fragment sampler textures
 */
void util_blitter_copy(struct blitter_context *blitter,
                       struct pipe_surface *dst,
                       unsigned dstx, unsigned dsty,
                       struct pipe_surface *src,
                       unsigned srcx, unsigned srcy,
                       unsigned width, unsigned height,
                       boolean ignore_stencil);

/**
 * Fill a region of a surface with a constant value.
 *
 * If the surface cannot be rendered to or it's a depth-stencil format,
 * a software fallback path is taken.
 *
 * These states must be saved in the blitter in addition to the state objects
 * already required to be saved:
 * - framebuffer state
 */
void util_blitter_fill(struct blitter_context *blitter,
                       struct pipe_surface *dst,
                       unsigned dstx, unsigned dsty,
                       unsigned width, unsigned height,
                       unsigned value);

/**
 * Copy all pixels from one surface to another.
 *
 * The rules are the same as in util_blitter_copy with the addition that
 * surfaces must have the same size.
 */
static INLINE
void util_blitter_copy_surface(struct blitter_context *blitter,
                               struct pipe_surface *dst,
                               struct pipe_surface *src,
                               boolean ignore_stencil)
{
   assert(dst->width == src->width && dst->height == src->height);

   util_blitter_copy(blitter, dst, 0, 0, src, 0, 0, src->width, src->height,
                     ignore_stencil);
}


/* The functions below should be used to save currently bound constant state
 * objects inside a driver. The objects are automatically restored at the end
 * of the util_blitter_{clear, fill, copy, copy_surface} functions and then
 * forgotten.
 *
 * CSOs not listed here are not affected by util_blitter. */

static INLINE
void util_blitter_save_blend(struct blitter_context *blitter,
                             void *state)
{
   blitter->saved_blend_state = state;
}

static INLINE
void util_blitter_save_depth_stencil_alpha(struct blitter_context *blitter,
                                           void *state)
{
   blitter->saved_dsa_state = state;
}

static INLINE
void util_blitter_save_stencil_ref(struct blitter_context *blitter,
                                   const struct pipe_stencil_ref *state)
{
   blitter->saved_stencil_ref = *state;
}

static INLINE
void util_blitter_save_rasterizer(struct blitter_context *blitter,
                                  void *state)
{
   blitter->saved_rs_state = state;
}

static INLINE
void util_blitter_save_fragment_shader(struct blitter_context *blitter,
                                       void *fs)
{
   blitter->saved_fs = fs;
}

static INLINE
void util_blitter_save_vertex_shader(struct blitter_context *blitter,
                                     void *vs)
{
   blitter->saved_vs = vs;
}

static INLINE
void util_blitter_save_framebuffer(struct blitter_context *blitter,
                                   struct pipe_framebuffer_state *state)
{
   blitter->saved_fb_state = *state;
}

static INLINE
void util_blitter_save_viewport(struct blitter_context *blitter,
                                struct pipe_viewport_state *state)
{
   blitter->saved_viewport = *state;
}

static INLINE
void util_blitter_save_clip(struct blitter_context *blitter,
                            struct pipe_clip_state *state)
{
   blitter->saved_clip = *state;
}

static INLINE
void util_blitter_save_fragment_sampler_states(
                  struct blitter_context *blitter,
                  int num_sampler_states,
                  void **sampler_states)
{
   assert(num_sampler_states <= Elements(blitter->saved_sampler_states));

   blitter->saved_num_sampler_states = num_sampler_states;
   memcpy(blitter->saved_sampler_states, sampler_states,
          num_sampler_states * sizeof(void *));
}

static INLINE
void util_blitter_save_fragment_sampler_textures(
                  struct blitter_context *blitter,
                  int num_textures,
                  struct pipe_texture **textures)
{
   assert(num_textures <= Elements(blitter->saved_textures));

   blitter->saved_num_textures = num_textures;
   memcpy(blitter->saved_textures, textures,
          num_textures * sizeof(struct pipe_texture *));
}

#ifdef __cplusplus
}
#endif

#endif
