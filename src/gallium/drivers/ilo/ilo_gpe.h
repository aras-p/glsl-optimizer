/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2013 LunarG, Inc.
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

#ifndef ILO_GPE_H
#define ILO_GPE_H

#include "ilo_common.h"

/**
 * \see brw_context.h
 */
#define ILO_MAX_DRAW_BUFFERS    8
#define ILO_MAX_CONST_BUFFERS   (1 + 12)
#define ILO_MAX_SAMPLER_VIEWS   16
#define ILO_MAX_SAMPLERS        16
#define ILO_MAX_SO_BINDINGS     64
#define ILO_MAX_SO_BUFFERS      4
#define ILO_MAX_VIEWPORTS       1

#define ILO_MAX_VS_SURFACES        (ILO_MAX_CONST_BUFFERS + ILO_MAX_SAMPLER_VIEWS)
#define ILO_VS_CONST_SURFACE(i)    (i)
#define ILO_VS_TEXTURE_SURFACE(i)  (ILO_MAX_CONST_BUFFERS  + i)

#define ILO_MAX_GS_SURFACES        (ILO_MAX_SO_BINDINGS)
#define ILO_GS_SO_SURFACE(i)       (i)

#define ILO_MAX_WM_SURFACES        (ILO_MAX_DRAW_BUFFERS + ILO_MAX_CONST_BUFFERS + ILO_MAX_SAMPLER_VIEWS)
#define ILO_WM_DRAW_SURFACE(i)     (i)
#define ILO_WM_CONST_SURFACE(i)    (ILO_MAX_DRAW_BUFFERS + i)
#define ILO_WM_TEXTURE_SURFACE(i)  (ILO_MAX_DRAW_BUFFERS + ILO_MAX_CONST_BUFFERS  + i)

struct ilo_vb_state {
   struct pipe_vertex_buffer states[PIPE_MAX_ATTRIBS];
   uint32_t enabled_mask;
};

struct ilo_ib_state {
   struct pipe_index_buffer state;
};

struct ilo_ve_state {
   struct pipe_vertex_element states[PIPE_MAX_ATTRIBS];
   unsigned count;
};

struct ilo_so_state {
   struct pipe_stream_output_target *states[ILO_MAX_SO_BUFFERS];
   unsigned count;
   unsigned append_bitmask;

   bool enabled;
};

struct ilo_viewport_cso {
   /* matrix form */
   float m00, m11, m22, m30, m31, m32;

   /* guardband in NDC space */
   float min_gbx, min_gby, max_gbx, max_gby;

   /* viewport in screen space */
   float min_x, min_y, min_z;
   float max_x, max_y, max_z;
};

struct ilo_viewport_state {
   struct ilo_viewport_cso cso[ILO_MAX_VIEWPORTS];
   unsigned count;

   struct pipe_viewport_state viewport0;
};

struct ilo_scissor_state {
   struct pipe_scissor_state states[ILO_MAX_VIEWPORTS];
};

struct ilo_rasterizer_state {
   struct pipe_rasterizer_state state;
};

struct ilo_dsa_state {
   struct pipe_depth_stencil_alpha_state state;
};

struct ilo_blend_state {
   struct pipe_blend_state state;
};

struct ilo_sampler_state {
   struct pipe_sampler_state *states[ILO_MAX_SAMPLERS];
   unsigned count;
};

struct ilo_view_state {
   struct pipe_sampler_view *states[ILO_MAX_SAMPLER_VIEWS];
   unsigned count;
};

struct ilo_cbuf_state {
   struct pipe_constant_buffer states[ILO_MAX_CONST_BUFFERS];
   unsigned count;
};

struct ilo_resource_state {
   struct pipe_surface *states[PIPE_MAX_SHADER_RESOURCES];
   unsigned count;
};

struct ilo_fb_state {
   struct pipe_framebuffer_state state;

   unsigned num_samples;
};

struct ilo_global_binding {
   /*
    * XXX These should not be treated as real resources (and there could be
    * thousands of them).  They should be treated as regions in GLOBAL
    * resource, which is the only real resource.
    *
    * That is, a resource here should instead be
    *
    *   struct ilo_global_region {
    *     struct pipe_resource base;
    *     int offset;
    *     int size;
    *   };
    *
    * and it describes the region [offset, offset + size) in GLOBAL
    * resource.
    */
   struct pipe_resource *resources[PIPE_MAX_SHADER_RESOURCES];
   uint32_t *handles[PIPE_MAX_SHADER_RESOURCES];
   unsigned count;
};

void
ilo_gpe_set_viewport_cso(const struct ilo_dev_info *dev,
                         const struct pipe_viewport_state *state,
                         struct ilo_viewport_cso *vp);

#endif /* ILO_GPE_H */
