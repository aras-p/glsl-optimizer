/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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

#include "util/u_inlines.h"
#include "util/u_memory.h"

#include "id_screen.h"
#include "id_objects.h"

struct pipe_buffer *
identity_buffer_create(struct identity_screen *id_screen,
                       struct pipe_buffer *buffer)
{
   struct identity_buffer *id_buffer;

   if(!buffer)
      goto error;

   assert(buffer->screen == id_screen->screen);

   id_buffer = CALLOC_STRUCT(identity_buffer);
   if(!id_buffer)
      goto error;

   memcpy(&id_buffer->base, buffer, sizeof(struct pipe_buffer));

   pipe_reference_init(&id_buffer->base.reference, 1);
   id_buffer->base.screen = &id_screen->base;
   id_buffer->buffer = buffer;

   return &id_buffer->base;

error:
   pipe_buffer_reference(&buffer, NULL);
   return NULL;
}

void
identity_buffer_destroy(struct identity_buffer *id_buffer)
{
   pipe_buffer_reference(&id_buffer->buffer, NULL);
   FREE(id_buffer);
}


struct pipe_texture *
identity_texture_create(struct identity_screen *id_screen,
                        struct pipe_texture *texture)
{
   struct identity_texture *id_texture;

   if(!texture)
      goto error;

   assert(texture->screen == id_screen->screen);

   id_texture = CALLOC_STRUCT(identity_texture);
   if(!id_texture)
      goto error;

   memcpy(&id_texture->base, texture, sizeof(struct pipe_texture));

   pipe_reference_init(&id_texture->base.reference, 1);
   id_texture->base.screen = &id_screen->base;
   id_texture->texture = texture;

   return &id_texture->base;

error:
   pipe_texture_reference(&texture, NULL);
   return NULL;
}

void
identity_texture_destroy(struct identity_texture *id_texture)
{
   pipe_texture_reference(&id_texture->texture, NULL);
   FREE(id_texture);
}


struct pipe_surface *
identity_surface_create(struct identity_texture *id_texture,
                        struct pipe_surface *surface)
{
   struct identity_surface *id_surface;

   if(!surface)
      goto error;

   assert(surface->texture == id_texture->texture);

   id_surface = CALLOC_STRUCT(identity_surface);
   if(!id_surface)
      goto error;

   memcpy(&id_surface->base, surface, sizeof(struct pipe_surface));

   pipe_reference_init(&id_surface->base.reference, 1);
   id_surface->base.texture = NULL;
   pipe_texture_reference(&id_surface->base.texture, &id_texture->base);
   id_surface->surface = surface;

   return &id_surface->base;

error:
   pipe_surface_reference(&surface, NULL);
   return NULL;
}

void
identity_surface_destroy(struct identity_surface *id_surface)
{
   pipe_texture_reference(&id_surface->base.texture, NULL);
   pipe_surface_reference(&id_surface->surface, NULL);
   FREE(id_surface);
}


struct pipe_transfer *
identity_transfer_create(struct identity_texture *id_texture,
                         struct pipe_transfer *transfer)
{
   struct identity_transfer *id_transfer;

   if(!transfer)
      goto error;

   assert(transfer->texture == id_texture->texture);

   id_transfer = CALLOC_STRUCT(identity_transfer);
   if(!id_transfer)
      goto error;

   memcpy(&id_transfer->base, transfer, sizeof(struct pipe_transfer));

   id_transfer->base.texture = NULL;
   pipe_texture_reference(&id_transfer->base.texture, &id_texture->base);
   id_transfer->transfer = transfer;
   assert(id_transfer->base.texture == &id_texture->base);

   return &id_transfer->base;

error:
   transfer->texture->screen->tex_transfer_destroy(transfer);
   return NULL;
}

void
identity_transfer_destroy(struct identity_transfer *id_transfer)
{
   struct identity_screen *id_screen = identity_screen(id_transfer->base.texture->screen);
   struct pipe_screen *screen = id_screen->screen;

   pipe_texture_reference(&id_transfer->base.texture, NULL);
   screen->tex_transfer_destroy(id_transfer->transfer);
   FREE(id_transfer);
}

struct pipe_video_surface *
identity_video_surface_create(struct identity_screen *id_screen,
                              struct pipe_video_surface *video_surface)
{
   struct identity_video_surface *id_video_surface;

   if (!video_surface) {
      goto error;
   }

   assert(video_surface->screen == id_screen->screen);

   id_video_surface = CALLOC_STRUCT(identity_video_surface);
   if (!id_video_surface) {
      goto error;
   }

   memcpy(&id_video_surface->base,
          video_surface,
          sizeof(struct pipe_video_surface));

   pipe_reference_init(&id_video_surface->base.reference, 1);
   id_video_surface->base.screen = &id_screen->base;
   id_video_surface->video_surface = video_surface;

   return &id_video_surface->base;

error:
   pipe_video_surface_reference(&video_surface, NULL);
   return NULL;
}

void
identity_video_surface_destroy(struct identity_video_surface *id_video_surface)
{
   pipe_video_surface_reference(&id_video_surface->video_surface, NULL);
   FREE(id_video_surface);
}
