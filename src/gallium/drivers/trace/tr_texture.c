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

#include "pipe/p_inlines.h"
#include "util/u_hash_table.h"
#include "util/u_memory.h"

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
   tr_tex->base.screen = &tr_scr->base;
   tr_tex->texture = texture;
   
   return &tr_tex->base;
   
error:
   pipe_texture_reference(&texture, NULL);
   return NULL;
}


void
trace_texture_destroy(struct trace_screen *tr_scr, 
                      struct pipe_texture *texture)
{
   struct trace_texture *tr_tex = trace_texture(tr_scr, texture); 
   pipe_texture_reference(&tr_tex->texture, NULL);
   FREE(tr_tex);
}


struct pipe_surface *
trace_surface_create(struct trace_texture *tr_tex, 
                     struct pipe_surface *surface)
{
   struct trace_surface *tr_surf;
   
   if(!surface)
      goto error;
   
   assert(surface->texture == tr_tex->texture);
   
   tr_surf = CALLOC_STRUCT(trace_surface);
   if(!tr_surf)
      goto error;
   
   memcpy(&tr_surf->base, surface, sizeof(struct pipe_surface));
   
   tr_surf->base.winsys = tr_tex->base.screen->winsys;
   tr_surf->base.texture = NULL;
   pipe_texture_reference(&tr_surf->base.texture, &tr_tex->base);
   tr_surf->surface = surface;

   return &tr_surf->base;
   
error:
   pipe_surface_reference(&surface, NULL);
   return NULL;
}


void
trace_surface_destroy(struct trace_texture *tr_tex, 
                      struct pipe_surface *surface)
{
   struct trace_surface *tr_surf = trace_surface(tr_tex, surface);
   pipe_texture_reference(&tr_surf->base.texture, NULL);
   pipe_surface_reference(&tr_surf->surface, NULL);
   FREE(tr_surf);
}

