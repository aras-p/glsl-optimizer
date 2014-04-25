/*
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/**
 * @file brw_object_purgeable.c
 *
 * The driver implementation of the GL_APPLE_object_purgeable extension.
 */

#include "main/imports.h"
#include "main/mtypes.h"
#include "main/macros.h"
#include "main/bufferobj.h"

#include "brw_context.h"
#include "intel_buffer_objects.h"
#include "intel_fbo.h"
#include "intel_mipmap_tree.h"

static GLenum
intel_buffer_purgeable(drm_intel_bo *buffer)
{
   int retained = 0;

   if (buffer != NULL)
      retained = drm_intel_bo_madvise(buffer, I915_MADV_DONTNEED);

   return retained ? GL_VOLATILE_APPLE : GL_RELEASED_APPLE;
}

static GLenum
intel_buffer_object_purgeable(struct gl_context * ctx,
                              struct gl_buffer_object *obj,
                              GLenum option)
{
   struct intel_buffer_object *intel_obj = intel_buffer_object(obj);

   if (intel_obj->buffer != NULL)
      return intel_buffer_purgeable(intel_obj->buffer);

   if (option == GL_RELEASED_APPLE) {
      return GL_RELEASED_APPLE;
   } else {
      /* XXX Create the buffer and madvise(MADV_DONTNEED)? */
      return intel_buffer_purgeable(intel_obj->buffer);
   }
}

static GLenum
intel_texture_object_purgeable(struct gl_context * ctx,
                               struct gl_texture_object *obj,
                               GLenum option)
{
   struct intel_texture_object *intel;

   (void) ctx;
   (void) option;

   intel = intel_texture_object(obj);
   if (intel->mt == NULL || intel->mt->bo == NULL)
      return GL_RELEASED_APPLE;

   return intel_buffer_purgeable(intel->mt->bo);
}

static GLenum
intel_render_object_purgeable(struct gl_context * ctx,
                              struct gl_renderbuffer *obj,
                              GLenum option)
{
   struct intel_renderbuffer *intel;

   (void) ctx;
   (void) option;

   intel = intel_renderbuffer(obj);
   if (intel->mt == NULL)
      return GL_RELEASED_APPLE;

   return intel_buffer_purgeable(intel->mt->bo);
}

static GLenum
intel_buffer_unpurgeable(drm_intel_bo *buffer)
{
   int retained;

   retained = 0;
   if (buffer != NULL)
      retained = drm_intel_bo_madvise(buffer, I915_MADV_WILLNEED);

   return retained ? GL_RETAINED_APPLE : GL_UNDEFINED_APPLE;
}

static GLenum
intel_buffer_object_unpurgeable(struct gl_context * ctx,
                                struct gl_buffer_object *obj,
                                GLenum option)
{
   (void) ctx;
   (void) option;

   return intel_buffer_unpurgeable(intel_buffer_object(obj)->buffer);
}

static GLenum
intel_texture_object_unpurgeable(struct gl_context * ctx,
                                 struct gl_texture_object *obj,
                                 GLenum option)
{
   struct intel_texture_object *intel;

   (void) ctx;
   (void) option;

   intel = intel_texture_object(obj);
   if (intel->mt == NULL || intel->mt->bo == NULL)
      return GL_UNDEFINED_APPLE;

   return intel_buffer_unpurgeable(intel->mt->bo);
}

static GLenum
intel_render_object_unpurgeable(struct gl_context * ctx,
                                struct gl_renderbuffer *obj,
                                GLenum option)
{
   struct intel_renderbuffer *intel;

   (void) ctx;
   (void) option;

   intel = intel_renderbuffer(obj);
   if (intel->mt == NULL)
      return GL_UNDEFINED_APPLE;

   return intel_buffer_unpurgeable(intel->mt->bo);
}

void
brw_init_object_purgeable_functions(struct dd_function_table *functions)
{
   functions->BufferObjectPurgeable = intel_buffer_object_purgeable;
   functions->TextureObjectPurgeable = intel_texture_object_purgeable;
   functions->RenderObjectPurgeable = intel_render_object_purgeable;

   functions->BufferObjectUnpurgeable = intel_buffer_object_unpurgeable;
   functions->TextureObjectUnpurgeable = intel_texture_object_unpurgeable;
   functions->RenderObjectUnpurgeable = intel_render_object_unpurgeable;
}
