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

#ifndef ID_OBJECTS_H
#define ID_OBJECTS_H


#include "pipe/p_compiler.h"
#include "pipe/p_state.h"
#include "pipe/p_video_state.h"

#include "id_screen.h"


struct identity_buffer
{
   struct pipe_buffer base;

   struct pipe_buffer *buffer;
};


struct identity_texture
{
   struct pipe_texture base;

   struct pipe_texture *texture;
};


struct identity_surface
{
   struct pipe_surface base;

   struct pipe_surface *surface;
};


struct identity_transfer
{
   struct pipe_transfer base;

   struct pipe_transfer *transfer;
};


struct identity_video_surface
{
   struct pipe_video_surface base;

   struct pipe_video_surface *video_surface;
};


static INLINE struct identity_buffer *
identity_buffer(struct pipe_buffer *_buffer)
{
   if(!_buffer)
      return NULL;
   (void)identity_screen(_buffer->screen);
   return (struct identity_buffer *)_buffer;
}

static INLINE struct identity_texture *
identity_texture(struct pipe_texture *_texture)
{
   if(!_texture)
      return NULL;
   (void)identity_screen(_texture->screen);
   return (struct identity_texture *)_texture;
}

static INLINE struct identity_surface *
identity_surface(struct pipe_surface *_surface)
{
   if(!_surface)
      return NULL;
   (void)identity_texture(_surface->texture);
   return (struct identity_surface *)_surface;
}

static INLINE struct identity_transfer *
identity_transfer(struct pipe_transfer *_transfer)
{
   if(!_transfer)
      return NULL;
   (void)identity_texture(_transfer->texture);
   return (struct identity_transfer *)_transfer;
}

static INLINE struct identity_video_surface *
identity_video_surface(struct pipe_video_surface *_video_surface)
{
   if (!_video_surface) {
      return NULL;
   }
   (void)identity_screen(_video_surface->screen);
   return (struct identity_video_surface *)_video_surface;
}

static INLINE struct pipe_buffer *
identity_buffer_unwrap(struct pipe_buffer *_buffer)
{
   if(!_buffer)
      return NULL;
   return identity_buffer(_buffer)->buffer;
}

static INLINE struct pipe_texture *
identity_texture_unwrap(struct pipe_texture *_texture)
{
   if(!_texture)
      return NULL;
   return identity_texture(_texture)->texture;
}

static INLINE struct pipe_surface *
identity_surface_unwrap(struct pipe_surface *_surface)
{
   if(!_surface)
      return NULL;
   return identity_surface(_surface)->surface;
}

static INLINE struct pipe_transfer *
identity_transfer_unwrap(struct pipe_transfer *_transfer)
{
   if(!_transfer)
      return NULL;
   return identity_transfer(_transfer)->transfer;
}


struct pipe_buffer *
identity_buffer_create(struct identity_screen *id_screen,
                       struct pipe_buffer *buffer);

void
identity_buffer_destroy(struct identity_buffer *id_buffer);

struct pipe_texture *
identity_texture_create(struct identity_screen *id_screen,
                        struct pipe_texture *texture);

void
identity_texture_destroy(struct identity_texture *id_texture);

struct pipe_surface *
identity_surface_create(struct identity_texture *id_texture,
                        struct pipe_surface *surface);

void
identity_surface_destroy(struct identity_surface *id_surface);

struct pipe_transfer *
identity_transfer_create(struct identity_texture *id_texture,
                         struct pipe_transfer *transfer);

void
identity_transfer_destroy(struct identity_transfer *id_transfer);

struct pipe_video_surface *
identity_video_surface_create(struct identity_screen *id_screen,
                              struct pipe_video_surface *video_surface);

void
identity_video_surface_destroy(struct identity_video_surface *id_video_surface);


#endif /* ID_OBJECTS_H */
