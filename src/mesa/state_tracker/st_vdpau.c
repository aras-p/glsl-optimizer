/**************************************************************************
 *
 * Copyright 2013 Advanced Micro Devices, Inc.
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
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/*
 * Authors:
 *      Christian König <christian.koenig@amd.com>
 *
 */

#include "main/texobj.h"
#include "main/teximage.h"
#include "main/errors.h"
#include "program/prog_instruction.h"

#include "pipe/p_state.h"
#include "pipe/p_video_codec.h"

#include "state_tracker/vdpau_interop.h"

#include "util/u_inlines.h"

#include "st_vdpau.h"
#include "st_context.h"
#include "st_texture.h"
#include "st_format.h"
#include "st_cb_flush.h"

static void
st_vdpau_map_surface(struct gl_context *ctx, GLenum target, GLenum access,
                     GLboolean output, struct gl_texture_object *texObj,
                     struct gl_texture_image *texImage,
                     const GLvoid *vdpSurface, GLuint index)
{
   int (*getProcAddr)(uint32_t device, uint32_t id, void **ptr);
   uint32_t device = (uintptr_t)ctx->vdpDevice;

   struct st_context *st = st_context(ctx);
   struct st_texture_object *stObj = st_texture_object(texObj);
   struct st_texture_image *stImage = st_texture_image(texImage);
 
   struct pipe_resource *res;
   struct pipe_sampler_view templ, **sampler_view;
   mesa_format texFormat;

   getProcAddr = ctx->vdpGetProcAddress;
   if (output) {
      VdpOutputSurfaceGallium *f;
      
      if (getProcAddr(device, VDP_FUNC_ID_OUTPUT_SURFACE_GALLIUM, (void**)&f)) {
         _mesa_error(ctx, GL_INVALID_OPERATION, "VDPAUMapSurfacesNV");
         return;
      }

      res = f((uintptr_t)vdpSurface);

      if (!res) {
         _mesa_error(ctx, GL_INVALID_OPERATION, "VDPAUMapSurfacesNV");
         return;
      }

   } else {
      struct pipe_sampler_view *sv;
      VdpVideoSurfaceGallium *f;

      struct pipe_video_buffer *buffer;
      struct pipe_sampler_view **samplers;

      if (getProcAddr(device, VDP_FUNC_ID_VIDEO_SURFACE_GALLIUM, (void**)&f)) {
         _mesa_error(ctx, GL_INVALID_OPERATION, "VDPAUMapSurfacesNV");
         return;
      }

      buffer = f((uintptr_t)vdpSurface);
      if (!buffer) {
         _mesa_error(ctx, GL_INVALID_OPERATION, "VDPAUMapSurfacesNV");
         return;
      }

      samplers = buffer->get_sampler_view_planes(buffer);
      if (!samplers) {
         _mesa_error(ctx, GL_INVALID_OPERATION, "VDPAUMapSurfacesNV");
         return;
      }

      sv = samplers[index >> 1];
      if (!sv) {
         _mesa_error(ctx, GL_INVALID_OPERATION, "VDPAUMapSurfacesNV");
         return;
      }

      res = sv->texture;
   }

   if (!res) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "VDPAUMapSurfacesNV");
      return;
   }

   /* do we have different screen objects ? */
   if (res->screen != st->pipe->screen) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "VDPAUMapSurfacesNV");
      return;
   }

   /* switch to surface based */
   if (!stObj->surface_based) {
      _mesa_clear_texture_object(ctx, texObj);
      stObj->surface_based = GL_TRUE;
   }

   texFormat = st_pipe_format_to_mesa_format(res->format);

   _mesa_init_teximage_fields(ctx, texImage,
                              res->width0, res->height0, 1, 0, GL_RGBA,
                              texFormat);

   pipe_resource_reference(&stObj->pt, res);
   st_texture_release_all_sampler_views(stObj);
   pipe_resource_reference(&stImage->pt, res);

   u_sampler_view_default_template(&templ, res, res->format);
   templ.u.tex.first_layer = index & 1;
   templ.u.tex.last_layer = index & 1;
   templ.swizzle_r = GET_SWZ(stObj->base._Swizzle, 0);
   templ.swizzle_g = GET_SWZ(stObj->base._Swizzle, 1);
   templ.swizzle_b = GET_SWZ(stObj->base._Swizzle, 2);
   templ.swizzle_a = GET_SWZ(stObj->base._Swizzle, 3);

   sampler_view = st_texture_get_sampler_view(st, stObj);
   *sampler_view = st->pipe->create_sampler_view(st->pipe, res, &templ);

   stObj->width0 = res->width0;
   stObj->height0 = res->height0;
   stObj->depth0 = 1;
   stObj->surface_format = res->format;

   _mesa_dirty_texobj(ctx, texObj);
}

static void
st_vdpau_unmap_surface(struct gl_context *ctx, GLenum target, GLenum access,
                       GLboolean output, struct gl_texture_object *texObj,
                       struct gl_texture_image *texImage,
                       const GLvoid *vdpSurface, GLuint index)
{
   struct st_context *st = st_context(ctx);
   struct st_texture_object *stObj = st_texture_object(texObj);
   struct st_texture_image *stImage = st_texture_image(texImage);

   pipe_resource_reference(&stObj->pt, NULL);
   st_texture_release_all_sampler_views(stObj);
   pipe_resource_reference(&stImage->pt, NULL);

   _mesa_dirty_texobj(ctx, texObj);

   st_flush(st, NULL, 0);
}

void
st_init_vdpau_functions(struct dd_function_table *functions)
{
   functions->VDPAUMapSurface = st_vdpau_map_surface;
   functions->VDPAUUnmapSurface = st_vdpau_unmap_surface;
}
