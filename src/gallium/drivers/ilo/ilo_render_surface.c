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
   ILO_DEV_ASSERT(r->dev, 6, 7.5);

   /* SURFACE_STATEs for render targets */
   if (DIRTY(FB)) {
      const struct ilo_fb_state *fb = &vec->fb;
      const int offset = ILO_WM_DRAW_SURFACE(0);
      uint32_t *surface_state = &r->state.wm.SURFACE_STATE[offset];
      int i;

      for (i = 0; i < fb->state.nr_cbufs; i++) {
         const struct ilo_surface_cso *surface =
            (const struct ilo_surface_cso *) fb->state.cbufs[i];

         if (!surface) {
            surface_state[i] =
               gen6_SURFACE_STATE(r->builder, &fb->null_rt, true);
         } else {
            assert(surface && surface->is_rt);
            surface_state[i] =
               gen6_SURFACE_STATE(r->builder, &surface->u.rt, true);
         }
      }

      /*
       * Upload at least one render target, as
       * brw_update_renderbuffer_surfaces() does.  I don't know why.
       */
      if (i == 0) {
         surface_state[i] =
            gen6_SURFACE_STATE(r->builder, &fb->null_rt, true);

         i++;
      }

      memset(&surface_state[i], 0, (ILO_MAX_DRAW_BUFFERS - i) * 4);

      if (i && session->num_surfaces[PIPE_SHADER_FRAGMENT] < offset + i)
         session->num_surfaces[PIPE_SHADER_FRAGMENT] = offset + i;

      session->binding_table_fs_changed = true;
   }
}

static void
gen6_emit_draw_surface_so(struct ilo_render *r,
                          const struct ilo_state_vector *vec,
                          struct ilo_render_draw_session *session)
{
   const struct ilo_so_state *so = &vec->so;

   ILO_DEV_ASSERT(r->dev, 6, 6);

   /* SURFACE_STATEs for stream output targets */
   if (DIRTY(VS) || DIRTY(GS) || DIRTY(SO)) {
      const struct pipe_stream_output_info *so_info =
         (vec->gs) ? ilo_shader_get_kernel_so_info(vec->gs) :
         (vec->vs) ? ilo_shader_get_kernel_so_info(vec->vs) : NULL;
      const int offset = ILO_GS_SO_SURFACE(0);
      uint32_t *surface_state = &r->state.gs.SURFACE_STATE[offset];
      int i;

      for (i = 0; so_info && i < so_info->num_outputs; i++) {
         const int target = so_info->output[i].output_buffer;
         const struct pipe_stream_output_target *so_target =
            (target < so->count) ? so->states[target] : NULL;

         if (so_target) {
            surface_state[i] = gen6_so_SURFACE_STATE(r->builder,
                  so_target, so_info, i);
         } else {
            surface_state[i] = 0;
         }
      }

      memset(&surface_state[i], 0, (ILO_MAX_SO_BINDINGS - i) * 4);

      if (i && session->num_surfaces[PIPE_SHADER_GEOMETRY] < offset + i)
         session->num_surfaces[PIPE_SHADER_GEOMETRY] = offset + i;

      session->binding_table_gs_changed = true;
   }
}

static void
gen6_emit_draw_surface_view(struct ilo_render *r,
                            const struct ilo_state_vector *vec,
                            int shader_type,
                            struct ilo_render_draw_session *session)
{
   const struct ilo_view_state *view = &vec->view[shader_type];
   uint32_t *surface_state;
   int offset, i;
   bool skip = false;

   ILO_DEV_ASSERT(r->dev, 6, 7.5);

   /* SURFACE_STATEs for sampler views */
   switch (shader_type) {
   case PIPE_SHADER_VERTEX:
      if (DIRTY(VIEW_VS)) {
         offset = ILO_VS_TEXTURE_SURFACE(0);
         surface_state = &r->state.vs.SURFACE_STATE[offset];

         session->binding_table_vs_changed = true;
      } else {
         skip = true;
      }
      break;
   case PIPE_SHADER_FRAGMENT:
      if (DIRTY(VIEW_FS)) {
         offset = ILO_WM_TEXTURE_SURFACE(0);
         surface_state = &r->state.wm.SURFACE_STATE[offset];

         session->binding_table_fs_changed = true;
      } else {
         skip = true;
      }
      break;
   default:
      skip = true;
      break;
   }

   if (skip)
      return;

   for (i = 0; i < view->count; i++) {
      if (view->states[i]) {
         const struct ilo_view_cso *cso =
            (const struct ilo_view_cso *) view->states[i];

         surface_state[i] =
            gen6_SURFACE_STATE(r->builder, &cso->surface, false);
      } else {
         surface_state[i] = 0;
      }
   }

   memset(&surface_state[i], 0, (ILO_MAX_SAMPLER_VIEWS - i) * 4);

   if (i && session->num_surfaces[shader_type] < offset + i)
      session->num_surfaces[shader_type] = offset + i;
}

static void
gen6_emit_draw_surface_const(struct ilo_render *r,
                             const struct ilo_state_vector *vec,
                             int shader_type,
                             struct ilo_render_draw_session *session)
{
   const struct ilo_cbuf_state *cbuf = &vec->cbuf[shader_type];
   uint32_t *surface_state;
   bool *binding_table_changed;
   int offset, count, i;

   ILO_DEV_ASSERT(r->dev, 6, 7.5);

   if (!DIRTY(CBUF))
      return;

   /* SURFACE_STATEs for constant buffers */
   switch (shader_type) {
   case PIPE_SHADER_VERTEX:
      offset = ILO_VS_CONST_SURFACE(0);
      surface_state = &r->state.vs.SURFACE_STATE[offset];
      binding_table_changed = &session->binding_table_vs_changed;
      break;
   case PIPE_SHADER_FRAGMENT:
      offset = ILO_WM_CONST_SURFACE(0);
      surface_state = &r->state.wm.SURFACE_STATE[offset];
      binding_table_changed = &session->binding_table_fs_changed;
      break;
   default:
      return;
      break;
   }

   /* constants are pushed via PCB */
   if (cbuf->enabled_mask == 0x1 && !cbuf->cso[0].resource) {
      memset(surface_state, 0, ILO_MAX_CONST_BUFFERS * 4);
      return;
   }

   count = util_last_bit(cbuf->enabled_mask);
   for (i = 0; i < count; i++) {
      if (cbuf->cso[i].resource) {
         surface_state[i] = gen6_SURFACE_STATE(r->builder,
               &cbuf->cso[i].surface, false);
      } else {
         surface_state[i] = 0;
      }
   }

   memset(&surface_state[count], 0, (ILO_MAX_CONST_BUFFERS - count) * 4);

   if (count && session->num_surfaces[shader_type] < offset + count)
      session->num_surfaces[shader_type] = offset + count;

   *binding_table_changed = true;
}

static void
gen6_emit_draw_surface_binding_tables(struct ilo_render *r,
                                      const struct ilo_state_vector *vec,
                                      int shader_type,
                                      struct ilo_render_draw_session *session)
{
   uint32_t *binding_table_state, *surface_state;
   int *binding_table_state_size, size;
   bool skip = false;

   ILO_DEV_ASSERT(r->dev, 6, 7.5);

   /* BINDING_TABLE_STATE */
   switch (shader_type) {
   case PIPE_SHADER_VERTEX:
      surface_state = r->state.vs.SURFACE_STATE;
      binding_table_state = &r->state.vs.BINDING_TABLE_STATE;
      binding_table_state_size = &r->state.vs.BINDING_TABLE_STATE_size;

      skip = !session->binding_table_vs_changed;
      break;
   case PIPE_SHADER_GEOMETRY:
      surface_state = r->state.gs.SURFACE_STATE;
      binding_table_state = &r->state.gs.BINDING_TABLE_STATE;
      binding_table_state_size = &r->state.gs.BINDING_TABLE_STATE_size;

      skip = !session->binding_table_gs_changed;
      break;
   case PIPE_SHADER_FRAGMENT:
      surface_state = r->state.wm.SURFACE_STATE;
      binding_table_state = &r->state.wm.BINDING_TABLE_STATE;
      binding_table_state_size = &r->state.wm.BINDING_TABLE_STATE_size;

      skip = !session->binding_table_fs_changed;
      break;
   default:
      skip = true;
      break;
   }

   if (skip)
      return;

   /*
    * If we have seemingly less SURFACE_STATEs than before, it could be that
    * we did not touch those reside at the tail in this upload.  Loop over
    * them to figure out the real number of SURFACE_STATEs.
    */
   for (size = *binding_table_state_size;
         size > session->num_surfaces[shader_type]; size--) {
      if (surface_state[size - 1])
         break;
   }
   if (size < session->num_surfaces[shader_type])
      size = session->num_surfaces[shader_type];

   *binding_table_state = gen6_BINDING_TABLE_STATE(r->builder,
         surface_state, size);
   *binding_table_state_size = size;
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
         if (vec->view[sh_type].count) {
            num_surfaces = ILO_VS_TEXTURE_SURFACE(vec->view[sh_type].count);
         } else {
            num_surfaces = ILO_VS_CONST_SURFACE(
                  util_last_bit(vec->cbuf[sh_type].enabled_mask));
         }

         if (vec->vs) {
            if (ilo_dev_gen(render->dev) == ILO_GEN(6)) {
               const struct pipe_stream_output_info *so_info =
                  ilo_shader_get_kernel_so_info(vec->vs);

               /* stream outputs */
               num_surfaces += so_info->num_outputs;
            }
         }
         break;
      case PIPE_SHADER_GEOMETRY:
         if (vec->gs && ilo_dev_gen(render->dev) == ILO_GEN(6)) {
            const struct pipe_stream_output_info *so_info =
               ilo_shader_get_kernel_so_info(vec->gs);

            /* stream outputs */
            num_surfaces += so_info->num_outputs;
         }
         break;
      case PIPE_SHADER_FRAGMENT:
         if (vec->view[sh_type].count) {
            num_surfaces = ILO_WM_TEXTURE_SURFACE(vec->view[sh_type].count);
         } else if (vec->cbuf[sh_type].enabled_mask) {
            num_surfaces = ILO_WM_CONST_SURFACE(
                  util_last_bit(vec->cbuf[sh_type].enabled_mask));
         } else {
            num_surfaces = vec->fb.state.nr_cbufs;
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
