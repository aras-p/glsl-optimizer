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

#ifndef ILO_3D_PIPELINE_H
#define ILO_3D_PIPELINE_H

#include "ilo_common.h"
#include "ilo_gpe.h"

struct intel_bo;
struct ilo_blitter;
struct ilo_cp;
struct ilo_context;

enum ilo_3d_pipeline_invalidate_flags {
   ILO_3D_PIPELINE_INVALIDATE_HW         = 1 << 0,
   ILO_3D_PIPELINE_INVALIDATE_BATCH_BO   = 1 << 1,
   ILO_3D_PIPELINE_INVALIDATE_STATE_BO   = 1 << 2,
   ILO_3D_PIPELINE_INVALIDATE_KERNEL_BO  = 1 << 3,

   ILO_3D_PIPELINE_INVALIDATE_ALL        = 0xffffffff,
};

enum ilo_3d_pipeline_action {
   ILO_3D_PIPELINE_DRAW,
   ILO_3D_PIPELINE_FLUSH,
   ILO_3D_PIPELINE_WRITE_TIMESTAMP,
   ILO_3D_PIPELINE_WRITE_DEPTH_COUNT,
   ILO_3D_PIPELINE_WRITE_STATISTICS,
   ILO_3D_PIPELINE_RECTLIST,
};

/**
 * 3D pipeline.
 */
struct ilo_3d_pipeline {
   struct ilo_cp *cp;
   const struct ilo_dev_info *dev;

   uint32_t invalidate_flags;

   struct intel_bo *workaround_bo;

   uint32_t packed_sample_position_1x;
   uint32_t packed_sample_position_4x;
   uint32_t packed_sample_position_8x[2];

   int (*estimate_size)(struct ilo_3d_pipeline *pipeline,
                        enum ilo_3d_pipeline_action action,
                        const void *arg);

   void (*emit_draw)(struct ilo_3d_pipeline *pipeline,
                     const struct ilo_context *ilo);

   void (*emit_flush)(struct ilo_3d_pipeline *pipeline);

   void (*emit_write_timestamp)(struct ilo_3d_pipeline *pipeline,
                                struct intel_bo *bo, int index);

   void (*emit_write_depth_count)(struct ilo_3d_pipeline *pipeline,
                                  struct intel_bo *bo, int index);

   void (*emit_write_statistics)(struct ilo_3d_pipeline *pipeline,
                                 struct intel_bo *bo, int index);

   void (*emit_rectlist)(struct ilo_3d_pipeline *pipeline,
                         const struct ilo_blitter *blitter);

   /**
    * HW states.
    */
   struct ilo_3d_pipeline_state {
      bool has_gen6_wa_pipe_control;

      bool primitive_restart;
      int reduced_prim;
      int so_num_vertices, so_max_vertices;

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

struct ilo_3d_pipeline *
ilo_3d_pipeline_create(struct ilo_cp *cp, const struct ilo_dev_info *dev);

void
ilo_3d_pipeline_destroy(struct ilo_3d_pipeline *pipeline);


static inline void
ilo_3d_pipeline_invalidate(struct ilo_3d_pipeline *p, uint32_t flags)
{
   p->invalidate_flags |= flags;
}

/**
 * Estimate the size of an action.
 */
static inline int
ilo_3d_pipeline_estimate_size(struct ilo_3d_pipeline *pipeline,
                              enum ilo_3d_pipeline_action action,
                              const void *arg)
{
   return pipeline->estimate_size(pipeline, action, arg);
}

bool
ilo_3d_pipeline_emit_draw(struct ilo_3d_pipeline *p,
                          const struct ilo_context *ilo,
                          int *prim_generated, int *prim_emitted);

void
ilo_3d_pipeline_emit_flush(struct ilo_3d_pipeline *p);

void
ilo_3d_pipeline_emit_write_timestamp(struct ilo_3d_pipeline *p,
                                     struct intel_bo *bo, int index);

void
ilo_3d_pipeline_emit_write_depth_count(struct ilo_3d_pipeline *p,
                                       struct intel_bo *bo, int index);

void
ilo_3d_pipeline_emit_write_statistics(struct ilo_3d_pipeline *p,
                                      struct intel_bo *bo, int index);

void
ilo_3d_pipeline_emit_rectlist(struct ilo_3d_pipeline *p,
                              const struct ilo_blitter *blitter);

void
ilo_3d_pipeline_get_sample_position(struct ilo_3d_pipeline *p,
                                    unsigned sample_count,
                                    unsigned sample_index,
                                    float *x, float *y);

void
ilo_3d_pipeline_dump(struct ilo_3d_pipeline *p);

#endif /* ILO_3D_PIPELINE_H */
