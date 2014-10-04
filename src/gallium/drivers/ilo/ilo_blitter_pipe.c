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

#include "util/u_blitter.h"
#include "util/u_surface.h"

#include "ilo_context.h"
#include "ilo_blitter.h"

enum ilo_blitter_pipe_op {
   ILO_BLITTER_PIPE_BLIT,
   ILO_BLITTER_PIPE_COPY,
   ILO_BLITTER_PIPE_CLEAR,
   ILO_BLITTER_PIPE_CLEAR_FB,
};

static void
ilo_blitter_pipe_begin(struct ilo_blitter *blitter,
                       enum ilo_blitter_pipe_op op,
                       bool scissor_enable)
{
   struct blitter_context *b = blitter->pipe_blitter;
   struct ilo_state_vector *vec = &blitter->ilo->state_vector;

   /* vertex states */
   util_blitter_save_vertex_buffer_slot(b, vec->vb.states);
   util_blitter_save_vertex_elements(b, (void *) vec->ve);
   util_blitter_save_vertex_shader(b, vec->vs);
   util_blitter_save_geometry_shader(b, vec->gs);
   util_blitter_save_so_targets(b, vec->so.count, vec->so.states);
   util_blitter_save_rasterizer(b, (void *) vec->rasterizer);

   /* fragment states */
   util_blitter_save_fragment_shader(b, vec->fs);
   util_blitter_save_depth_stencil_alpha(b, (void *) vec->dsa);
   util_blitter_save_blend(b, (void *) vec->blend);
   util_blitter_save_sample_mask(b, vec->sample_mask);
   util_blitter_save_stencil_ref(b, &vec->stencil_ref);
   util_blitter_save_viewport(b, &vec->viewport.viewport0);

   if (scissor_enable)
      util_blitter_save_scissor(b, &vec->scissor.scissor0);

   switch (op) {
   case ILO_BLITTER_PIPE_BLIT:
   case ILO_BLITTER_PIPE_COPY:
      /*
       * We are about to call util_blitter_blit() or
       * util_blitter_copy_texture().  Note that util_blitter uses at most two
       * textures.
       */
      util_blitter_save_fragment_sampler_states(b,
            2, (void **) vec->sampler[PIPE_SHADER_FRAGMENT].cso);

      util_blitter_save_fragment_sampler_views(b,
            vec->view[PIPE_SHADER_FRAGMENT].count,
            vec->view[PIPE_SHADER_FRAGMENT].states);

      util_blitter_save_framebuffer(b, &vec->fb.state);

      /* resource_copy_region() or blit() does not honor render condition */
      util_blitter_save_render_condition(b,
            blitter->ilo->render_condition.query,
            blitter->ilo->render_condition.condition,
            blitter->ilo->render_condition.mode);
      break;
   case ILO_BLITTER_PIPE_CLEAR:
      /*
       * we are about to call util_blitter_clear_render_target() or
       * util_blitter_clear_depth_stencil()
       */
      util_blitter_save_framebuffer(b, &vec->fb.state);
      break;
   case ILO_BLITTER_PIPE_CLEAR_FB:
      /* we are about to call util_blitter_clear() */
      break;
   default:
      break;
   }
}

static void
ilo_blitter_pipe_end(struct ilo_blitter *blitter)
{
}

bool
ilo_blitter_pipe_blit(struct ilo_blitter *blitter,
                      const struct pipe_blit_info *info)
{
   struct blitter_context *b = blitter->pipe_blitter;
   struct pipe_blit_info skip_stencil;

   if (util_try_blit_via_copy_region(&blitter->ilo->base, info))
      return true;

   if (!util_blitter_is_blit_supported(b, info)) {
      /* try without stencil */
      if (info->mask & PIPE_MASK_S) {
         skip_stencil = *info;
         skip_stencil.mask = info->mask & ~PIPE_MASK_S;

         if (util_blitter_is_blit_supported(blitter->pipe_blitter,
                                            &skip_stencil)) {
            ilo_warn("ignore stencil buffer blitting\n");
            info = &skip_stencil;
         }
      }

      if (info != &skip_stencil) {
         ilo_warn("failed to blit with pipe blitter\n");
         return false;
      }
   }

   ilo_blitter_pipe_begin(blitter, ILO_BLITTER_PIPE_BLIT,
         info->scissor_enable);
   util_blitter_blit(b, info);
   ilo_blitter_pipe_end(blitter);

   return true;
}

bool
ilo_blitter_pipe_copy_resource(struct ilo_blitter *blitter,
                               struct pipe_resource *dst, unsigned dst_level,
                               unsigned dst_x, unsigned dst_y, unsigned dst_z,
                               struct pipe_resource *src, unsigned src_level,
                               const struct pipe_box *src_box)
{
   /* not until we allow rendertargets to be buffers */
   if (dst->target == PIPE_BUFFER || src->target == PIPE_BUFFER)
      return false;

   if (!util_blitter_is_copy_supported(blitter->pipe_blitter, dst, src))
      return false;

   ilo_blitter_pipe_begin(blitter, ILO_BLITTER_PIPE_COPY, false);

   util_blitter_copy_texture(blitter->pipe_blitter,
         dst, dst_level, dst_x, dst_y, dst_z,
         src, src_level, src_box);

   ilo_blitter_pipe_end(blitter);

   return true;
}

bool
ilo_blitter_pipe_clear_rt(struct ilo_blitter *blitter,
                          struct pipe_surface *rt,
                          const union pipe_color_union *color,
                          unsigned x, unsigned y,
                          unsigned width, unsigned height)
{
   ilo_blitter_pipe_begin(blitter, ILO_BLITTER_PIPE_CLEAR, false);

   util_blitter_clear_render_target(blitter->pipe_blitter,
         rt, color, x, y, width, height);

   ilo_blitter_pipe_end(blitter);

   return true;
}

bool
ilo_blitter_pipe_clear_zs(struct ilo_blitter *blitter,
                          struct pipe_surface *zs,
                          unsigned clear_flags,
                          double depth, unsigned stencil,
                          unsigned x, unsigned y,
                          unsigned width, unsigned height)
{
   ilo_blitter_pipe_begin(blitter, ILO_BLITTER_PIPE_CLEAR, false);

   util_blitter_clear_depth_stencil(blitter->pipe_blitter,
         zs, clear_flags, depth, stencil, x, y, width, height);

   ilo_blitter_pipe_end(blitter);

   return true;
}

bool
ilo_blitter_pipe_clear_fb(struct ilo_blitter *blitter,
                          unsigned buffers,
                          const union pipe_color_union *color,
                          double depth, unsigned stencil)
{
   struct ilo_state_vector *vec = &blitter->ilo->state_vector;

   /* TODO we should pause/resume some queries */
   ilo_blitter_pipe_begin(blitter, ILO_BLITTER_PIPE_CLEAR_FB, false);

   util_blitter_clear(blitter->pipe_blitter,
         vec->fb.state.width, vec->fb.state.height, 1,
         buffers, color, depth, stencil);

   ilo_blitter_pipe_end(blitter);

   return true;
}
