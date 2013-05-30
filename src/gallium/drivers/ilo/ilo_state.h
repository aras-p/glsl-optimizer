/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#ifndef ILO_STATE_H
#define ILO_STATE_H

#include "ilo_common.h"

/**
 * States that we track.
 *
 * XXX Do we want to count each sampler or vertex buffer as a state?  If that
 * is the case, there are simply not enough bits.
 *
 * XXX We want to treat primitive type and depth clear value as states, but
 * there are not enough bits.
 */
enum ilo_state {
   ILO_STATE_BLEND,
   ILO_STATE_FRAGMENT_SAMPLERS,
   ILO_STATE_VERTEX_SAMPLERS,
   ILO_STATE_GEOMETRY_SAMPLERS,
   ILO_STATE_COMPUTE_SAMPLERS,
   ILO_STATE_RASTERIZER,
   ILO_STATE_DEPTH_STENCIL_ALPHA,
   ILO_STATE_FS,
   ILO_STATE_VS,
   ILO_STATE_GS,
   ILO_STATE_VERTEX_ELEMENTS,
   ILO_STATE_BLEND_COLOR,
   ILO_STATE_STENCIL_REF,
   ILO_STATE_SAMPLE_MASK,
   ILO_STATE_CLIP,
   ILO_STATE_CONSTANT_BUFFER,
   ILO_STATE_FRAMEBUFFER,
   ILO_STATE_POLY_STIPPLE,
   ILO_STATE_SCISSOR,
   ILO_STATE_VIEWPORT,
   ILO_STATE_FRAGMENT_SAMPLER_VIEWS,
   ILO_STATE_VERTEX_SAMPLER_VIEWS,
   ILO_STATE_GEOMETRY_SAMPLER_VIEWS,
   ILO_STATE_COMPUTE_SAMPLER_VIEWS,
   ILO_STATE_SHADER_RESOURCES,
   ILO_STATE_VERTEX_BUFFERS,
   ILO_STATE_INDEX_BUFFER,
   ILO_STATE_STREAM_OUTPUT_TARGETS,
   ILO_STATE_COMPUTE,
   ILO_STATE_COMPUTE_RESOURCES,
   ILO_STATE_GLOBAL_BINDING,

   ILO_STATE_COUNT,
};

/**
 * Dirty flags of the states.
 */
enum ilo_dirty_flags {
   ILO_DIRTY_BLEND                    = 1 << ILO_STATE_BLEND,
   ILO_DIRTY_FRAGMENT_SAMPLERS        = 1 << ILO_STATE_FRAGMENT_SAMPLERS,
   ILO_DIRTY_VERTEX_SAMPLERS          = 1 << ILO_STATE_VERTEX_SAMPLERS,
   ILO_DIRTY_GEOMETRY_SAMPLERS        = 1 << ILO_STATE_GEOMETRY_SAMPLERS,
   ILO_DIRTY_COMPUTE_SAMPLERS         = 1 << ILO_STATE_COMPUTE_SAMPLERS,
   ILO_DIRTY_RASTERIZER               = 1 << ILO_STATE_RASTERIZER,
   ILO_DIRTY_DEPTH_STENCIL_ALPHA      = 1 << ILO_STATE_DEPTH_STENCIL_ALPHA,
   ILO_DIRTY_FS                       = 1 << ILO_STATE_FS,
   ILO_DIRTY_VS                       = 1 << ILO_STATE_VS,
   ILO_DIRTY_GS                       = 1 << ILO_STATE_GS,
   ILO_DIRTY_VERTEX_ELEMENTS          = 1 << ILO_STATE_VERTEX_ELEMENTS,
   ILO_DIRTY_BLEND_COLOR              = 1 << ILO_STATE_BLEND_COLOR,
   ILO_DIRTY_STENCIL_REF              = 1 << ILO_STATE_STENCIL_REF,
   ILO_DIRTY_SAMPLE_MASK              = 1 << ILO_STATE_SAMPLE_MASK,
   ILO_DIRTY_CLIP                     = 1 << ILO_STATE_CLIP,
   ILO_DIRTY_CONSTANT_BUFFER          = 1 << ILO_STATE_CONSTANT_BUFFER,
   ILO_DIRTY_FRAMEBUFFER              = 1 << ILO_STATE_FRAMEBUFFER,
   ILO_DIRTY_POLY_STIPPLE             = 1 << ILO_STATE_POLY_STIPPLE,
   ILO_DIRTY_SCISSOR                  = 1 << ILO_STATE_SCISSOR,
   ILO_DIRTY_VIEWPORT                 = 1 << ILO_STATE_VIEWPORT,
   ILO_DIRTY_FRAGMENT_SAMPLER_VIEWS   = 1 << ILO_STATE_FRAGMENT_SAMPLER_VIEWS,
   ILO_DIRTY_VERTEX_SAMPLER_VIEWS     = 1 << ILO_STATE_VERTEX_SAMPLER_VIEWS,
   ILO_DIRTY_GEOMETRY_SAMPLER_VIEWS   = 1 << ILO_STATE_GEOMETRY_SAMPLER_VIEWS,
   ILO_DIRTY_COMPUTE_SAMPLER_VIEWS    = 1 << ILO_STATE_COMPUTE_SAMPLER_VIEWS,
   ILO_DIRTY_SHADER_RESOURCES         = 1 << ILO_STATE_SHADER_RESOURCES,
   ILO_DIRTY_VERTEX_BUFFERS           = 1 << ILO_STATE_VERTEX_BUFFERS,
   ILO_DIRTY_INDEX_BUFFER             = 1 << ILO_STATE_INDEX_BUFFER,
   ILO_DIRTY_STREAM_OUTPUT_TARGETS    = 1 << ILO_STATE_STREAM_OUTPUT_TARGETS,
   ILO_DIRTY_COMPUTE                  = 1 << ILO_STATE_COMPUTE,
   ILO_DIRTY_COMPUTE_RESOURCES        = 1 << ILO_STATE_COMPUTE_RESOURCES,
   ILO_DIRTY_GLOBAL_BINDING           = 1 << ILO_STATE_GLOBAL_BINDING,
   ILO_DIRTY_ALL                      = 0xffffffff,
};

struct ilo_context;

void
ilo_init_state_functions(struct ilo_context *ilo);

void
ilo_init_states(struct ilo_context *ilo);

void
ilo_cleanup_states(struct ilo_context *ilo);

void
ilo_finalize_states(struct ilo_context *ilo);

#endif /* ILO_STATE_H */
