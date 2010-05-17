/**************************************************************************
 *
 * Copyright 2009, VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/
/*
 * Management of pipe objects (surface / pipe / fences) used by DRI1 and DRISW.
 *
 * Author: Keith Whitwell <keithw@vmware.com>
 * Author: Jakob Bornecrantz <wallbraker@gmail.com>
 */

#include "util/u_inlines.h"
#include "pipe/p_context.h"

#include "dri_screen.h"
#include "dri_context.h"
#include "dri_drawable.h"
#include "dri1_helper.h"

struct pipe_fence_handle *
dri1_swap_fences_pop_front(struct dri_drawable *draw)
{
   struct pipe_screen *screen = dri_screen(draw->sPriv)->base.screen;
   struct pipe_fence_handle *fence = NULL;

   if (draw->cur_fences >= draw->desired_fences) {
      screen->fence_reference(screen, &fence, draw->swap_fences[draw->tail]);
      screen->fence_reference(screen, &draw->swap_fences[draw->tail++], NULL);
      --draw->cur_fences;
      draw->tail &= DRI_SWAP_FENCES_MASK;
   }
   return fence;
}

void
dri1_swap_fences_push_back(struct dri_drawable *draw,
                           struct pipe_fence_handle *fence)
{
   struct pipe_screen *screen = dri_screen(draw->sPriv)->base.screen;

   if (!fence)
      return;

   if (draw->cur_fences < DRI_SWAP_FENCES_MAX) {
      draw->cur_fences++;
      screen->fence_reference(screen, &draw->swap_fences[draw->head++],
			      fence);
      draw->head &= DRI_SWAP_FENCES_MASK;
   }
}

void
dri1_swap_fences_clear(struct dri_drawable *drawable)
{
   struct pipe_screen *screen = dri_screen(drawable->sPriv)->base.screen;
   struct pipe_fence_handle *fence;

   while (drawable->cur_fences) {
      fence = dri1_swap_fences_pop_front(drawable);
      screen->fence_reference(screen, &fence, NULL);
   }
}

struct pipe_surface *
dri1_get_pipe_surface(struct dri_drawable *drawable, struct pipe_resource *ptex)
{
   struct pipe_screen *pipe_screen = dri_screen(drawable->sPriv)->base.screen;
   struct pipe_surface *psurf = drawable->dri1_surface;

   if (!psurf || psurf->texture != ptex) {
      pipe_surface_reference(&drawable->dri1_surface, NULL);

      drawable->dri1_surface = pipe_screen->get_tex_surface(pipe_screen,
            ptex, 0, 0, 0, 0/* no bind flag???*/);

      psurf = drawable->dri1_surface;
   }

   return psurf;
}

void
dri1_destroy_pipe_surface(struct dri_drawable *drawable)
{
   pipe_surface_reference(&drawable->dri1_surface, NULL);
}

struct pipe_context *
dri1_get_pipe_context(struct dri_screen *screen)
{
   struct pipe_context *pipe = screen->dri1_pipe;

   if (!pipe) {
      screen->dri1_pipe =
         screen->base.screen->context_create(screen->base.screen, NULL);
      pipe = screen->dri1_pipe;
   }

   return pipe;
}

void
dri1_destroy_pipe_context(struct dri_screen *screen)
{
   if (screen->dri1_pipe)
      screen->dri1_pipe->destroy(screen->dri1_pipe);
}
