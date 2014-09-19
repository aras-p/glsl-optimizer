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

#ifndef ILO_BLIT_H
#define ILO_BLIT_H

#include "ilo_common.h"
#include "ilo_context.h"
#include "ilo_state.h"
#include "ilo_resource.h"

void
ilo_blit_resolve_slices_for_hiz(struct ilo_context *ilo,
                                struct pipe_resource *res, unsigned level,
                                unsigned first_slice, unsigned num_slices,
                                unsigned resolve_flags);

static inline void
ilo_blit_resolve_slices(struct ilo_context *ilo,
                        struct pipe_resource *res, unsigned level,
                        unsigned first_slice, unsigned num_slices,
                        unsigned resolve_flags)
{
   struct ilo_texture *tex;
   unsigned slice_mask;

   if (res->target == PIPE_BUFFER)
      return;

   tex = ilo_texture(res);

   /*
    * This function is called frequently and we need to make it run faster.
    * As it is only used to resolve HiZ right now, return early when there is
    * no HiZ.
    */
   if (!ilo_texture_can_enable_hiz(tex, level, first_slice, num_slices))
      return;

   if (ilo_texture_can_enable_hiz(tex, level, first_slice, num_slices)) {
      ilo_blit_resolve_slices_for_hiz(ilo, res, level,
            first_slice, num_slices, resolve_flags);
   }

   slice_mask =
      ILO_TEXTURE_CPU_WRITE |
      ILO_TEXTURE_BLT_WRITE |
      ILO_TEXTURE_RENDER_WRITE;
   /* when there is a new writer, we may need to clear ILO_TEXTURE_CLEAR */
   if (resolve_flags & slice_mask)
      slice_mask |= ILO_TEXTURE_CLEAR;

   ilo_texture_set_slice_flags(tex, level,
         first_slice, num_slices, slice_mask, resolve_flags);
}

static inline void
ilo_blit_resolve_resource(struct ilo_context *ilo,
                          struct pipe_resource *res,
                          unsigned resolve_flags)
{
   unsigned lv;

   for (lv = 0; lv <= res->last_level; lv++) {
      const unsigned num_slices = (res->target == PIPE_TEXTURE_3D) ?
         u_minify(res->depth0, lv) : res->array_size;

      ilo_blit_resolve_slices(ilo, res, lv, 0, num_slices, resolve_flags);
   }
}

static inline void
ilo_blit_resolve_surface(struct ilo_context *ilo,
                         struct pipe_surface *surf,
                         unsigned resolve_flags)
{
   if (surf->texture->target == PIPE_BUFFER)
      return;

   ilo_blit_resolve_slices(ilo, surf->texture,
         surf->u.tex.level, surf->u.tex.first_layer,
         surf->u.tex.last_layer - surf->u.tex.first_layer + 1,
         resolve_flags);
}

static inline void
ilo_blit_resolve_transfer(struct ilo_context *ilo,
                          const struct pipe_transfer *xfer)
{
   unsigned resolve_flags = 0;

   if (xfer->resource->target == PIPE_BUFFER)
      return;

   if (xfer->usage & PIPE_TRANSFER_READ)
      resolve_flags |= ILO_TEXTURE_CPU_READ;
   if (xfer->usage & PIPE_TRANSFER_WRITE)
      resolve_flags |= ILO_TEXTURE_CPU_WRITE;

   ilo_blit_resolve_slices(ilo, xfer->resource, xfer->level,
         xfer->box.z, xfer->box.depth, resolve_flags);
}

static inline void
ilo_blit_resolve_view(struct ilo_context *ilo,
                      const struct pipe_sampler_view *view)
{
   const unsigned resolve_flags = ILO_TEXTURE_RENDER_READ;
   unsigned lv;

   if (view->texture->target == PIPE_BUFFER)
      return;

   for (lv = view->u.tex.first_level; lv <= view->u.tex.last_level; lv++) {
      unsigned first_slice, num_slices;

      if (view->texture->target == PIPE_TEXTURE_3D) {
         first_slice = 0;
         num_slices = u_minify(view->texture->depth0, lv);
      }
      else {
         first_slice = view->u.tex.first_layer;
         num_slices = view->u.tex.last_layer - view->u.tex.first_layer + 1;
      }

      ilo_blit_resolve_slices(ilo, view->texture,
            lv, first_slice, num_slices, resolve_flags);
   }
}

static inline void
ilo_blit_resolve_framebuffer(struct ilo_context *ilo)
{
   struct ilo_state_vector *vec = &ilo->state_vector;
   const struct pipe_framebuffer_state *fb = &vec->fb.state;
   unsigned sh, i;

   /* Not all bound views are sampled by the shaders.  How do we tell? */
   for (sh = 0; sh < Elements(vec->view); sh++) {
      for (i = 0; i < vec->view[sh].count; i++) {
         if (vec->view[sh].states[i])
            ilo_blit_resolve_view(ilo, vec->view[sh].states[i]);
      }
   }

   for (i = 0; i < fb->nr_cbufs; i++) {
      struct pipe_surface *surf = fb->cbufs[i];
      if (surf)
         ilo_blit_resolve_surface(ilo, surf, ILO_TEXTURE_RENDER_WRITE);
   }

   if (fb->zsbuf)
      ilo_blit_resolve_surface(ilo, fb->zsbuf, ILO_TEXTURE_RENDER_WRITE);
}

void
ilo_init_blit_functions(struct ilo_context *ilo);

#endif /* ILO_BLIT_H */
