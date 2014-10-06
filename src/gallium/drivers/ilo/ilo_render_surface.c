/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2014 LunarG, Inc.
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

#include "ilo_common.h"
#include "ilo_blitter.h"
#include "ilo_builder_3d.h"
#include "ilo_state.h"
#include "ilo_render_gen.h"

#define DIRTY(state) (session->pipe_dirty & ILO_DIRTY_ ## state)

static void
gen6_emit_draw_surface_rt(struct ilo_render *r,
                          const struct ilo_state_vector *vec,
                          struct ilo_render_draw_session *session)
{
   const struct ilo_shader_state *fs = vec->fs;
   const struct ilo_fb_state *fb = &vec->fb;
   uint32_t *surface_state;
   int base, count, i;

   ILO_DEV_ASSERT(r->dev, 6, 7.5);

   if (!DIRTY(FS) && !DIRTY(FB))
      return;
   if (!fs)
      return;

   session->binding_table_fs_changed = true;

   base = ilo_shader_get_kernel_param(fs, ILO_KERNEL_FS_SURFACE_RT_BASE);
   count = ilo_shader_get_kernel_param(fs, ILO_KERNEL_FS_SURFACE_RT_COUNT);

   /* SURFACE_STATEs for render targets */
   surface_state = &r->state.wm.SURFACE_STATE[base];
   for (i = 0; i < count; i++) {
      if (i < fb->state.nr_cbufs && fb->state.cbufs[i]) {
         const struct ilo_surface_cso *surface =
            (const struct ilo_surface_cso *) fb->state.cbufs[i];

         assert(surface->is_rt);
         surface_state[i] =
            gen6_SURFACE_STATE(r->builder, &surface->u.rt, true);
      } else {
         surface_state[i] =
            gen6_SURFACE_STATE(r->builder, &fb->null_rt, true);
      }
   }
}

static void
gen6_emit_draw_surface_so(struct ilo_render *r,
                          const struct ilo_state_vector *vec,
                          struct ilo_render_draw_session *session)
{
   const struct ilo_shader_state *vs = vec->vs;
   const struct ilo_shader_state *gs = vec->gs;
   const struct ilo_so_state *so = &vec->so;
   const struct pipe_stream_output_info *so_info;
   uint32_t *surface_state;
   int base, count, i;

   ILO_DEV_ASSERT(r->dev, 6, 6);

   if (!DIRTY(VS) && !DIRTY(GS) && !DIRTY(SO))
      return;

   if (gs) {
      so_info = ilo_shader_get_kernel_so_info(gs);
      base = ilo_shader_get_kernel_param(gs,
            ILO_KERNEL_GS_GEN6_SURFACE_SO_BASE);
      count = ilo_shader_get_kernel_param(gs,
            ILO_KERNEL_GS_GEN6_SURFACE_SO_COUNT);
   } else if (vs) {
      so_info = ilo_shader_get_kernel_so_info(vs);
      base = 0;
      count = ilo_shader_get_kernel_param(vs,
            ILO_KERNEL_VS_GEN6_SO_SURFACE_COUNT);
   } else {
      return;
   }

   session->binding_table_gs_changed = true;

   /* SURFACE_STATEs for stream output targets */
   surface_state = &r->state.gs.SURFACE_STATE[base];
   for (i = 0; i < count; i++) {
      if (so_info && i < so_info->num_outputs &&
          so_info->output[i].output_buffer < so->count &&
          so->states[so_info->output[i].output_buffer]) {
         const struct pipe_stream_output_target *so_target =
            so->states[so_info->output[i].output_buffer];

         surface_state[i] = gen6_so_SURFACE_STATE(r->builder,
               so_target, so_info, i);
      } else {
         surface_state[i] = 0;
      }
   }
}

static void
gen6_emit_draw_surface_view(struct ilo_render *r,
                            const struct ilo_state_vector *vec,
                            int shader_type,
                            struct ilo_render_draw_session *session)
{
   const struct ilo_view_state *view = &vec->view[shader_type];
   const struct ilo_shader_state *sh;
   uint32_t *surface_state;
   int base, count, i;

   ILO_DEV_ASSERT(r->dev, 6, 7.5);

   switch (shader_type) {
   case PIPE_SHADER_VERTEX:
      if (!DIRTY(VS) && !DIRTY(VIEW_VS))
         return;
      if (!vec->vs)
         return;

      sh = vec->vs;
      surface_state = r->state.vs.SURFACE_STATE;
      session->binding_table_vs_changed = true;
      break;
   case PIPE_SHADER_FRAGMENT:
      if (!DIRTY(FS) && !DIRTY(VIEW_FS))
         return;
      if (!vec->fs)
         return;

      sh = vec->fs;
      surface_state = r->state.wm.SURFACE_STATE;
      session->binding_table_fs_changed = true;
      break;
   default:
      return;
      break;
   }

   base = ilo_shader_get_kernel_param(sh, ILO_KERNEL_SURFACE_TEX_BASE);
   count = ilo_shader_get_kernel_param(sh, ILO_KERNEL_SURFACE_TEX_COUNT);

   /* SURFACE_STATEs for sampler views */
   surface_state += base;
   for (i = 0; i < count; i++) {
      if (i < view->count && view->states[i]) {
         const struct ilo_view_cso *cso =
            (const struct ilo_view_cso *) view->states[i];

         surface_state[i] =
            gen6_SURFACE_STATE(r->builder, &cso->surface, false);
      } else {
         surface_state[i] = 0;
      }
   }
}

static void
gen6_emit_draw_surface_const(struct ilo_render *r,
                             const struct ilo_state_vector *vec,
                             int shader_type,
                             struct ilo_render_draw_session *session)
{
   const struct ilo_cbuf_state *cbuf = &vec->cbuf[shader_type];
   const struct ilo_shader_state *sh;
   uint32_t *surface_state;
   int base, count, i;

   ILO_DEV_ASSERT(r->dev, 6, 7.5);

   switch (shader_type) {
   case PIPE_SHADER_VERTEX:
      if (!DIRTY(VS) && !DIRTY(CBUF))
         return;
      if (!vec->vs)
         return;

      sh = vec->vs;
      surface_state = r->state.vs.SURFACE_STATE;
      session->binding_table_vs_changed = true;
      break;
   case PIPE_SHADER_FRAGMENT:
      if (!DIRTY(FS) && !DIRTY(CBUF))
         return;
      if (!vec->fs)
         return;

      sh = vec->fs;
      surface_state = r->state.wm.SURFACE_STATE;
      session->binding_table_fs_changed = true;
      break;
   default:
      return;
      break;
   }

   base = ilo_shader_get_kernel_param(sh, ILO_KERNEL_SURFACE_CONST_BASE);
   count = ilo_shader_get_kernel_param(sh, ILO_KERNEL_SURFACE_CONST_COUNT);

   /* SURFACE_STATEs for constant buffers */
   surface_state += base;
   for (i = 0; i < count; i++) {
      const struct ilo_cbuf_cso *cso = &cbuf->cso[i];

      if (cso->resource) {
         surface_state[i] = gen6_SURFACE_STATE(r->builder,
               &cso->surface, false);
      } else {
         surface_state[i] = 0;
      }
   }
}

static void
gen6_emit_draw_surface_binding_tables(struct ilo_render *r,
                                      const struct ilo_state_vector *vec,
                                      int shader_type,
                                      struct ilo_render_draw_session *session)
{
   int count;

   ILO_DEV_ASSERT(r->dev, 6, 7.5);

   /* BINDING_TABLE_STATE */
   switch (shader_type) {
   case PIPE_SHADER_VERTEX:
      if (!session->binding_table_vs_changed)
         return;
      if (!vec->vs)
         return;

      count = ilo_shader_get_kernel_param(vec->vs,
            ILO_KERNEL_SURFACE_TOTAL_COUNT);

      r->state.vs.BINDING_TABLE_STATE = gen6_BINDING_TABLE_STATE(r->builder,
            r->state.vs.SURFACE_STATE, count);
      break;
   case PIPE_SHADER_GEOMETRY:
      if (!session->binding_table_gs_changed)
         return;
      if (vec->gs) {
         count = ilo_shader_get_kernel_param(vec->gs,
               ILO_KERNEL_SURFACE_TOTAL_COUNT);
      } else if (ilo_dev_gen(r->dev) == ILO_GEN(6) && vec->vs) {
         count = ilo_shader_get_kernel_param(vec->vs,
               ILO_KERNEL_VS_GEN6_SO_SURFACE_COUNT);
      } else {
         return;
      }

      r->state.gs.BINDING_TABLE_STATE = gen6_BINDING_TABLE_STATE(r->builder,
            r->state.gs.SURFACE_STATE, count);
      break;
   case PIPE_SHADER_FRAGMENT:
      if (!session->binding_table_fs_changed)
         return;
      if (!vec->fs)
         return;

      count = ilo_shader_get_kernel_param(vec->fs,
            ILO_KERNEL_SURFACE_TOTAL_COUNT);

      r->state.wm.BINDING_TABLE_STATE = gen6_BINDING_TABLE_STATE(r->builder,
            r->state.wm.SURFACE_STATE, count);
      break;
   default:
      break;
   }
}

#undef DIRTY

int
ilo_render_get_draw_surface_states_len(const struct ilo_render *render,
                                       const struct ilo_state_vector *vec)
{
   int sh_type, len;

   ILO_DEV_ASSERT(render->dev, 6, 7.5);

   len = 0;

   for (sh_type = 0; sh_type < PIPE_SHADER_TYPES; sh_type++) {
      const int alignment = 32 / 4;
      int num_surfaces = 0;

      switch (sh_type) {
      case PIPE_SHADER_VERTEX:
         if (vec->vs) {
            num_surfaces = ilo_shader_get_kernel_param(vec->vs,
                  ILO_KERNEL_SURFACE_TOTAL_COUNT);

            if (ilo_dev_gen(render->dev) == ILO_GEN(6)) {
               num_surfaces += ilo_shader_get_kernel_param(vec->vs,
                     ILO_KERNEL_VS_GEN6_SO_SURFACE_COUNT);
            }
         }
         break;
      case PIPE_SHADER_GEOMETRY:
         if (vec->gs) {
            num_surfaces = ilo_shader_get_kernel_param(vec->gs,
                  ILO_KERNEL_SURFACE_TOTAL_COUNT);
         }
         break;
      case PIPE_SHADER_FRAGMENT:
         if (vec->fs) {
            num_surfaces = ilo_shader_get_kernel_param(vec->fs,
                  ILO_KERNEL_SURFACE_TOTAL_COUNT);
         }
         break;
      default:
         break;
      }

      /* BINDING_TABLE_STATE and SURFACE_STATEs */
      if (num_surfaces) {
         len += align(num_surfaces, alignment) +
            align(GEN6_SURFACE_STATE__SIZE, alignment) * num_surfaces;
      }
   }

   return len;
}

void
ilo_render_emit_draw_surface_states(struct ilo_render *render,
                                    const struct ilo_state_vector *vec,
                                    struct ilo_render_draw_session *session)
{
   const unsigned surface_used = ilo_builder_surface_used(render->builder);
   int shader_type;

   ILO_DEV_ASSERT(render->dev, 6, 7.5);

   /*
    * upload all SURAFCE_STATEs together so that we know there are minimal
    * paddings
    */

   gen6_emit_draw_surface_rt(render, vec, session);

   if (ilo_dev_gen(render->dev) == ILO_GEN(6))
      gen6_emit_draw_surface_so(render, vec, session);

   for (shader_type = 0; shader_type < PIPE_SHADER_TYPES; shader_type++) {
      gen6_emit_draw_surface_view(render, vec, shader_type, session);
      gen6_emit_draw_surface_const(render, vec, shader_type, session);
   }

   /* this must be called after all SURFACE_STATEs have been uploaded */
   for (shader_type = 0; shader_type < PIPE_SHADER_TYPES; shader_type++) {
      gen6_emit_draw_surface_binding_tables(render, vec,
            shader_type, session);
   }

   assert(ilo_builder_surface_used(render->builder) <= surface_used +
         ilo_render_get_draw_surface_states_len(render, vec));
}
