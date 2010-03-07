/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "util/u_inlines.h"
#include "util/u_hash_table.h"
#include "util/u_memory.h"
#include "util/u_simple_list.h"

#include "tr_screen.h"
#include "tr_texture.h"


struct pipe_texture *
trace_texture_create(struct trace_screen *tr_scr,
                     struct pipe_texture *texture)
{
   struct trace_texture *tr_tex;

   if(!texture)
      goto error;

   assert(texture->screen == tr_scr->screen);

   tr_tex = CALLOC_STRUCT(trace_texture);
   if(!tr_tex)
      goto error;

   memcpy(&tr_tex->base, texture, sizeof(struct pipe_texture));

   pipe_reference_init(&tr_tex->base.reference, 1);
   tr_tex->base.screen = &tr_scr->base;
   tr_tex->texture = texture;

   trace_screen_add_to_list(tr_scr, textures, tr_tex);

   return &tr_tex->base;

error:
   pipe_texture_reference(&texture, NULL);
   return NULL;
}


void
trace_texture_destroy(struct trace_texture *tr_tex)
{
   struct trace_screen *tr_scr = trace_screen(tr_tex->base.screen);

   trace_screen_remove_from_list(tr_scr, textures, tr_tex);

   pipe_texture_reference(&tr_tex->texture, NULL);
   FREE(tr_tex);
}


struct pipe_surface *
trace_surface_create(struct trace_texture *tr_tex,
                     struct pipe_surface *surface)
{
   struct trace_screen *tr_scr = trace_screen(tr_tex->base.screen);
   struct trace_surface *tr_surf;

   if(!surface)
      goto error;

   assert(surface->texture == tr_tex->texture);

   tr_surf = CALLOC_STRUCT(trace_surface);
   if(!tr_surf)
      goto error;

   memcpy(&tr_surf->base, surface, sizeof(struct pipe_surface));

   pipe_reference_init(&tr_surf->base.reference, 1);
   tr_surf->base.texture = NULL;
   pipe_texture_reference(&tr_surf->base.texture, &tr_tex->base);
   tr_surf->surface = surface;

   trace_screen_add_to_list(tr_scr, surfaces, tr_surf);

   return &tr_surf->base;

error:
   pipe_surface_reference(&surface, NULL);
   return NULL;
}


void
trace_surface_destroy(struct trace_surface *tr_surf)
{
   struct trace_screen *tr_scr = trace_screen(tr_surf->base.texture->screen);

   trace_screen_remove_from_list(tr_scr, surfaces, tr_surf);

   pipe_texture_reference(&tr_surf->base.texture, NULL);
   pipe_surface_reference(&tr_surf->surface, NULL);
   FREE(tr_surf);
}


struct pipe_transfer *
trace_transfer_create(struct trace_texture *tr_tex,
                     struct pipe_transfer *transfer)
{
   struct trace_screen *tr_scr = trace_screen(tr_tex->base.screen);
   struct trace_transfer *tr_trans;

   if(!transfer)
      goto error;

   assert(transfer->texture == tr_tex->texture);

   tr_trans = CALLOC_STRUCT(trace_transfer);
   if(!tr_trans)
      goto error;

   memcpy(&tr_trans->base, transfer, sizeof(struct pipe_transfer));

   tr_trans->base.texture = NULL;
   pipe_texture_reference(&tr_trans->base.texture, &tr_tex->base);
   tr_trans->transfer = transfer;
   assert(tr_trans->base.texture == &tr_tex->base);

   trace_screen_add_to_list(tr_scr, transfers, tr_trans);

   return &tr_trans->base;

error:
   transfer->texture->screen->tex_transfer_destroy(transfer);
   return NULL;
}


void
trace_transfer_destroy(struct trace_transfer *tr_trans)
{
   struct trace_screen *tr_scr = trace_screen(tr_trans->base.texture->screen);
   struct pipe_screen *screen = tr_trans->transfer->texture->screen;

   trace_screen_remove_from_list(tr_scr, transfers, tr_trans);

   pipe_texture_reference(&tr_trans->base.texture, NULL);
   screen->tex_transfer_destroy(tr_trans->transfer);
   FREE(tr_trans);
}

