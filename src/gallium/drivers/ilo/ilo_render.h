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

#ifndef ILO_RENDER_H
#define ILO_RENDER_H

#include "ilo_common.h"
#include "ilo_state.h"

struct intel_bo;
struct ilo_blitter;
struct ilo_cp;
struct ilo_query;
struct ilo_state_vector;

enum ilo_render_action {
   ILO_RENDER_DRAW,
   ILO_RENDER_QUERY,
   ILO_RENDER_RECTLIST,
};

/**
 * Render Engine.
 */
struct ilo_render {
   const struct ilo_dev_info *dev;
   struct ilo_builder *builder;

   struct intel_bo *workaround_bo;

   uint32_t packed_sample_position_1x;
   uint32_t packed_sample_position_4x;
   uint32_t packed_sample_position_8x[2];

   int (*estimate_size)(struct ilo_render *render,
                        enum ilo_render_action action,
                        const void *arg);

   void (*emit_draw)(struct ilo_render *render,
                     const struct ilo_state_vector *vec);

   void (*emit_query)(struct ilo_render *render,
                      struct ilo_query *q, uint32_t offset);

   void (*emit_rectlist)(struct ilo_render *render,
                         const struct ilo_blitter *blitter);

   bool hw_ctx_changed;

   /*
    * Any state that involves resources needs to be re-emitted when the
    * batch bo changed.  This is because we do not pin the resources and
    * their offsets (or existence) may change between batch buffers.
    */
   bool batch_bo_changed;
   bool state_bo_changed;
   bool instruction_bo_changed;

   /**
    * HW states.
    */
   struct ilo_render_state {
      /*
       * When a WA is needed before some command, we always emit the WA right
       * before the command.  Knowing what have already been done since last
       * 3DPRIMITIVE allows us to skip some WAs.
       */
      uint32_t current_pipe_control_dw1;

      /*
       * When a WA is needed after some command, we may have the WA follow the
       * command immediately or defer it.  If this is non-zero, a PIPE_CONTROL
       * will be emitted before 3DPRIMITIVE.
       */
      uint32_t deferred_pipe_control_dw1;

      bool primitive_restart;
      int reduced_prim;
      int so_max_vertices;

      uint32_t SF_VIEWPORT;
      uint32_t CLIP_VIEWPORT;
      uint32_t SF_CLIP_VIEWPORT; /* GEN7+ */
      uint32_t CC_VIEWPORT;

      uint32_t COLOR_CALC_STATE;
      uint32_t BLEND_STATE;
      uint32_t DEPTH_STENCIL_STATE;

      uint32_t SCISSOR_RECT;

      struct {
         uint32_t BINDING_TABLE_STATE;
         int BINDING_TABLE_STATE_size;
         uint32_t SURFACE_STATE[ILO_MAX_VS_SURFACES];
         uint32_t SAMPLER_STATE;
         uint32_t SAMPLER_BORDER_COLOR_STATE[ILO_MAX_SAMPLERS];
         uint32_t PUSH_CONSTANT_BUFFER;
         int PUSH_CONSTANT_BUFFER_size;
      } vs;

      struct {
         uint32_t BINDING_TABLE_STATE;
         int BINDING_TABLE_STATE_size;
         uint32_t SURFACE_STATE[ILO_MAX_GS_SURFACES];
         bool active;
      } gs;

      struct {
         uint32_t BINDING_TABLE_STATE;
         int BINDING_TABLE_STATE_size;
         uint32_t SURFACE_STATE[ILO_MAX_WM_SURFACES];
         uint32_t SAMPLER_STATE;
         uint32_t SAMPLER_BORDER_COLOR_STATE[ILO_MAX_SAMPLERS];
         uint32_t PUSH_CONSTANT_BUFFER;
         int PUSH_CONSTANT_BUFFER_size;
      } wm;
   } state;
};

struct ilo_render *
ilo_render_create(struct ilo_builder *builder);

void
ilo_render_destroy(struct ilo_render *render);

/**
 * Estimate the size of an action.
 */
static inline int
ilo_render_estimate_size(struct ilo_render *render,
                         enum ilo_render_action action,
                         const void *arg)
{
   return render->estimate_size(render, action, arg);
}

/**
 * Emit context states and 3DPRIMITIVE.
 */
static inline void
ilo_render_emit_draw(struct ilo_render *render,
                     const struct ilo_state_vector *vec)
{
   render->emit_draw(render, vec);
}

/**
 * Emit PIPE_CONTROL or MI_STORE_REGISTER_MEM to save register values.
 */
static inline void
ilo_render_emit_query(struct ilo_render *render,
                      struct ilo_query *q, uint32_t offset)
{
   render->emit_query(render, q, offset);
}

static inline void
ilo_render_emit_rectlist(struct ilo_render *render,
                         const struct ilo_blitter *blitter)
{
   render->emit_rectlist(render, blitter);
}

void
ilo_render_get_sample_position(const struct ilo_render *render,
                               unsigned sample_count,
                               unsigned sample_index,
                               float *x, float *y);

void
ilo_render_invalidate_hw(struct ilo_render *render);

void
ilo_render_invalidate_builder(struct ilo_render *render);

int
ilo_render_get_flush_len(const struct ilo_render *render);

void
ilo_render_emit_flush(struct ilo_render *render);

#endif /* ILO_RENDER_H */
