/*
 * Mesa 3-D graphics library
 * Version:  7.9
 *
 * Copyright (C) 2010 LunarG Inc.
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

#include "state_tracker/st_gl_api.h"

#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "util/u_format.h"
#include "util/u_pointer.h"
#include "util/u_inlines.h"
#include "util/u_atomic.h"
#include "util/u_surface.h"

#include "main/mtypes.h"
#include "main/context.h"
#include "main/texobj.h"
#include "main/teximage.h"
#include "main/texstate.h"
#include "main/framebuffer.h"
#include "main/fbobject.h"
#include "main/renderbuffer.h"
#include "main/version.h"
#include "st_texture.h"

#include "st_context.h"
#include "st_format.h"
#include "st_cb_fbo.h"
#include "st_cb_flush.h"
#include "st_manager.h"

/**
 * Cast wrapper to convert a struct gl_framebuffer to an st_framebuffer.
 * Return NULL if the struct gl_framebuffer is a user-created framebuffer.
 * We'll only return non-null for window system framebuffers.
 * Note that this function may fail.
 */
static INLINE struct st_framebuffer *
st_ws_framebuffer(struct gl_framebuffer *fb)
{
   /* FBO cannot be casted.  See st_new_framebuffer */
   return (struct st_framebuffer *) ((fb && !fb->Name) ? fb : NULL);
}

/**
 * Map an attachment to a buffer index.
 */
static INLINE gl_buffer_index
attachment_to_buffer_index(enum st_attachment_type statt)
{
   gl_buffer_index index;

   switch (statt) {
   case ST_ATTACHMENT_FRONT_LEFT:
      index = BUFFER_FRONT_LEFT;
      break;
   case ST_ATTACHMENT_BACK_LEFT:
      index = BUFFER_BACK_LEFT;
      break;
   case ST_ATTACHMENT_FRONT_RIGHT:
      index = BUFFER_FRONT_RIGHT;
      break;
   case ST_ATTACHMENT_BACK_RIGHT:
      index = BUFFER_BACK_RIGHT;
      break;
   case ST_ATTACHMENT_DEPTH_STENCIL:
      index = BUFFER_DEPTH;
      break;
   case ST_ATTACHMENT_ACCUM:
      index = BUFFER_ACCUM;
      break;
   case ST_ATTACHMENT_SAMPLE:
   default:
      index = BUFFER_COUNT;
      break;
   }

   return index;
}

/**
 * Map a buffer index to an attachment.
 */
static INLINE enum st_attachment_type
buffer_index_to_attachment(gl_buffer_index index)
{
   enum st_attachment_type statt;

   switch (index) {
   case BUFFER_FRONT_LEFT:
      statt = ST_ATTACHMENT_FRONT_LEFT;
      break;
   case BUFFER_BACK_LEFT:
      statt = ST_ATTACHMENT_BACK_LEFT;
      break;
   case BUFFER_FRONT_RIGHT:
      statt = ST_ATTACHMENT_FRONT_RIGHT;
      break;
   case BUFFER_BACK_RIGHT:
      statt = ST_ATTACHMENT_BACK_RIGHT;
      break;
   case BUFFER_DEPTH:
      statt = ST_ATTACHMENT_DEPTH_STENCIL;
      break;
   case BUFFER_ACCUM:
      statt = ST_ATTACHMENT_ACCUM;
      break;
   default:
      statt = ST_ATTACHMENT_INVALID;
      break;
   }

   return statt;
}

/**
 * Validate a framebuffer to make sure up-to-date pipe_textures are used.
 */
static void
st_framebuffer_validate(struct st_framebuffer *stfb, struct st_context *st)
{
   struct pipe_context *pipe = st->pipe;
   struct pipe_resource *textures[ST_ATTACHMENT_COUNT];
   uint width, height;
   unsigned i;
   boolean changed = FALSE;

   if (!p_atomic_read(&stfb->revalidate))
      return;

   /* validate the fb */
   if (!stfb->iface->validate(stfb->iface, stfb->statts, stfb->num_statts, textures))
      return;

   width = stfb->Base.Width;
   height = stfb->Base.Height;

   for (i = 0; i < stfb->num_statts; i++) {
      struct st_renderbuffer *strb;
      struct pipe_surface *ps, surf_tmpl;
      gl_buffer_index idx;

      if (!textures[i])
         continue;

      idx = attachment_to_buffer_index(stfb->statts[i]);
      if (idx >= BUFFER_COUNT) {
         pipe_resource_reference(&textures[i], NULL);
         continue;
      }

      strb = st_renderbuffer(stfb->Base.Attachment[idx].Renderbuffer);
      assert(strb);
      if (strb->texture == textures[i]) {
         pipe_resource_reference(&textures[i], NULL);
         continue;
      }

      memset(&surf_tmpl, 0, sizeof(surf_tmpl));
      u_surface_default_template(&surf_tmpl, textures[i],
                                 PIPE_BIND_RENDER_TARGET);
      ps = pipe->create_surface(pipe, textures[i], &surf_tmpl);
      if (ps) {
         pipe_surface_reference(&strb->surface, ps);
         pipe_resource_reference(&strb->texture, ps->texture);
         /* ownership transfered */
         pipe_surface_reference(&ps, NULL);

         changed = TRUE;

         strb->Base.Width = strb->surface->width;
         strb->Base.Height = strb->surface->height;

         width = strb->Base.Width;
         height = strb->Base.Height;
      }

      pipe_resource_reference(&textures[i], NULL);
   }

   if (changed) {
      st->dirty.st |= ST_NEW_FRAMEBUFFER;
      _mesa_resize_framebuffer(st->ctx, &stfb->Base, width, height);

      assert(stfb->Base.Width == width);
      assert(stfb->Base.Height == height);
   }

   p_atomic_set(&stfb->revalidate, FALSE);
}

/**
 * Update the attachments to validate by looping the existing renderbuffers.
 */
static void
st_framebuffer_update_attachments(struct st_framebuffer *stfb)
{
   gl_buffer_index idx;

   stfb->num_statts = 0;
   for (idx = 0; idx < BUFFER_COUNT; idx++) {
      struct st_renderbuffer *strb;
      enum st_attachment_type statt;

      strb = st_renderbuffer(stfb->Base.Attachment[idx].Renderbuffer);
      if (!strb || strb->software)
         continue;

      statt = buffer_index_to_attachment(idx);
      if (statt != ST_ATTACHMENT_INVALID &&
          st_visual_have_buffers(stfb->iface->visual, 1 << statt))
         stfb->statts[stfb->num_statts++] = statt;
   }

   p_atomic_set(&stfb->revalidate, TRUE);
}

/**
 * Add a renderbuffer to the framebuffer.
 */
static boolean
st_framebuffer_add_renderbuffer(struct st_framebuffer *stfb,
                                gl_buffer_index idx)
{
   struct gl_renderbuffer *rb;
   enum pipe_format format;
   int samples;
   boolean sw;

   if (!stfb->iface)
      return FALSE;

   /* do not distinguish depth/stencil buffers */
   if (idx == BUFFER_STENCIL)
      idx = BUFFER_DEPTH;

   switch (idx) {
   case BUFFER_DEPTH:
      format = stfb->iface->visual->depth_stencil_format;
      sw = FALSE;
      break;
   case BUFFER_ACCUM:
      format = stfb->iface->visual->accum_format;
      sw = TRUE;
      break;
   default:
      format = stfb->iface->visual->color_format;
      sw = FALSE;
      break;
   }

   if (format == PIPE_FORMAT_NONE)
      return FALSE;

   samples = stfb->iface->visual->samples;
   if (!samples)
      samples = st_get_msaa();

   rb = st_new_renderbuffer_fb(format, samples, sw);
   if (!rb)
      return FALSE;

   if (idx != BUFFER_DEPTH) {
      _mesa_add_renderbuffer(&stfb->Base, idx, rb);
   }
   else {
      if (util_format_get_component_bits(format, UTIL_FORMAT_COLORSPACE_ZS, 0))
         _mesa_add_renderbuffer(&stfb->Base, BUFFER_DEPTH, rb);
      if (util_format_get_component_bits(format, UTIL_FORMAT_COLORSPACE_ZS, 1))
         _mesa_add_renderbuffer(&stfb->Base, BUFFER_STENCIL, rb);
   }

   return TRUE;
}

/**
 * Intialize a struct gl_config from a visual.
 */
static void
st_visual_to_context_mode(const struct st_visual *visual,
                          struct gl_config *mode)
{
   memset(mode, 0, sizeof(*mode));

   if (st_visual_have_buffers(visual, ST_ATTACHMENT_BACK_LEFT_MASK))
      mode->doubleBufferMode = GL_TRUE;
   if (st_visual_have_buffers(visual,
            ST_ATTACHMENT_FRONT_RIGHT_MASK | ST_ATTACHMENT_BACK_RIGHT_MASK))
      mode->stereoMode = GL_TRUE;

   if (visual->color_format != PIPE_FORMAT_NONE) {
      mode->rgbMode = GL_TRUE;

      mode->redBits =
         util_format_get_component_bits(visual->color_format,
               UTIL_FORMAT_COLORSPACE_RGB, 0);
      mode->greenBits =
         util_format_get_component_bits(visual->color_format,
               UTIL_FORMAT_COLORSPACE_RGB, 1);
      mode->blueBits =
         util_format_get_component_bits(visual->color_format,
               UTIL_FORMAT_COLORSPACE_RGB, 2);
      mode->alphaBits =
         util_format_get_component_bits(visual->color_format,
               UTIL_FORMAT_COLORSPACE_RGB, 3);

      mode->rgbBits = mode->redBits +
         mode->greenBits + mode->blueBits + mode->alphaBits;
   }

   if (visual->depth_stencil_format != PIPE_FORMAT_NONE) {
      mode->depthBits =
         util_format_get_component_bits(visual->depth_stencil_format,
               UTIL_FORMAT_COLORSPACE_ZS, 0);
      mode->stencilBits =
         util_format_get_component_bits(visual->depth_stencil_format,
               UTIL_FORMAT_COLORSPACE_ZS, 1);

      mode->haveDepthBuffer = mode->depthBits > 0;
      mode->haveStencilBuffer = mode->stencilBits > 0;
   }

   if (visual->accum_format != PIPE_FORMAT_NONE) {
      mode->haveAccumBuffer = GL_TRUE;

      mode->accumRedBits =
         util_format_get_component_bits(visual->accum_format,
               UTIL_FORMAT_COLORSPACE_RGB, 0);
      mode->accumGreenBits =
         util_format_get_component_bits(visual->accum_format,
               UTIL_FORMAT_COLORSPACE_RGB, 1);
      mode->accumBlueBits =
         util_format_get_component_bits(visual->accum_format,
               UTIL_FORMAT_COLORSPACE_RGB, 2);
      mode->accumAlphaBits =
         util_format_get_component_bits(visual->accum_format,
               UTIL_FORMAT_COLORSPACE_RGB, 3);
   }

   if (visual->samples) {
      mode->sampleBuffers = 1;
      mode->samples = visual->samples;
   }
}

/**
 * Determine the default draw or read buffer from a visual.
 */
static void
st_visual_to_default_buffer(const struct st_visual *visual,
                            GLenum *buffer, GLint *index)
{
   enum st_attachment_type statt;
   GLenum buf;
   gl_buffer_index idx;

   statt = visual->render_buffer;
   /* do nothing if an invalid render buffer is specified */
   if (statt == ST_ATTACHMENT_INVALID ||
       !st_visual_have_buffers(visual, 1 << statt))
      return;

   switch (statt) {
   case ST_ATTACHMENT_FRONT_LEFT:
      buf = GL_FRONT_LEFT;
      idx = BUFFER_FRONT_LEFT;
      break;
   case ST_ATTACHMENT_BACK_LEFT:
      buf = GL_BACK_LEFT;
      idx = BUFFER_BACK_LEFT;
      break;
   case ST_ATTACHMENT_FRONT_RIGHT:
      buf = GL_FRONT_RIGHT;
      idx = BUFFER_FRONT_RIGHT;
      break;
   case ST_ATTACHMENT_BACK_RIGHT:
      buf = GL_BACK_RIGHT;
      idx = BUFFER_BACK_RIGHT;
      break;
   default:
      buf = GL_NONE;
      idx = BUFFER_COUNT;
      break;
   }

   if (buf != GL_NONE) {
      if (buffer)
         *buffer = buf;
      if (index)
         *index = idx;
   }
}

/**
 * Create a framebuffer from a manager interface.
 */
static struct st_framebuffer *
st_framebuffer_create(struct st_framebuffer_iface *stfbi)
{
   struct st_framebuffer *stfb;
   struct gl_config mode;
   gl_buffer_index idx;

   stfb = CALLOC_STRUCT(st_framebuffer);
   if (!stfb)
      return NULL;

   /* for FBO-only context */
   if (!stfbi) {
      struct gl_framebuffer *base = _mesa_get_incomplete_framebuffer();

      stfb->Base = *base;

      return stfb;
   }

   st_visual_to_context_mode(stfbi->visual, &mode);
   _mesa_initialize_window_framebuffer(&stfb->Base, &mode);

   /* modify the draw/read buffers of the fb */
   st_visual_to_default_buffer(stfbi->visual, &stfb->Base.ColorDrawBuffer[0],
         &stfb->Base._ColorDrawBufferIndexes[0]);
   st_visual_to_default_buffer(stfbi->visual, &stfb->Base.ColorReadBuffer,
         &stfb->Base._ColorReadBufferIndex);

   stfb->iface = stfbi;

   /* add the color buffer */
   idx = stfb->Base._ColorDrawBufferIndexes[0];
   if (!st_framebuffer_add_renderbuffer(stfb, idx)) {
      FREE(stfb);
      return NULL;
   }

   st_framebuffer_add_renderbuffer(stfb, BUFFER_DEPTH);
   st_framebuffer_add_renderbuffer(stfb, BUFFER_ACCUM);

   st_framebuffer_update_attachments(stfb);

   stfb->Base.Initialized = GL_TRUE;

   return stfb;
}

/**
 * Reference a framebuffer.
 */
static void
st_framebuffer_reference(struct st_framebuffer **ptr,
                         struct st_framebuffer *stfb)
{
   struct gl_framebuffer *fb = &stfb->Base;
   _mesa_reference_framebuffer((struct gl_framebuffer **) ptr, fb);
}

static void
st_context_notify_invalid_framebuffer(struct st_context_iface *stctxi,
                                      struct st_framebuffer_iface *stfbi)
{
   struct st_context *st = (struct st_context *) stctxi;
   struct st_framebuffer *stfb;

   /* either draw or read winsys fb */
   stfb = st_ws_framebuffer(st->ctx->WinSysDrawBuffer);
   if (!stfb || stfb->iface != stfbi)
      stfb = st_ws_framebuffer(st->ctx->WinSysReadBuffer);

   if (stfb && stfb->iface == stfbi) {
      p_atomic_set(&stfb->revalidate, TRUE);
   }
   else {
      /* This function is probably getting called when we've detected a
       * change in a window's size but the currently bound context is
       * not bound to that window.
       * If the st_framebuffer_iface structure had a pointer to the
       * corresponding st_framebuffer we'd be able to handle this.
       */
   }
}

static void
st_context_flush(struct st_context_iface *stctxi, unsigned flags,
                 struct pipe_fence_handle **fence)
{
   struct st_context *st = (struct st_context *) stctxi;
   st_flush(st, flags, fence);
   if (flags & PIPE_FLUSH_RENDER_CACHE)
      st_manager_flush_frontbuffer(st);
}

static boolean
st_context_teximage(struct st_context_iface *stctxi,
                    enum st_texture_type target,
                    int level, enum pipe_format internal_format,
                    struct pipe_resource *tex, boolean mipmap)
{
   struct st_context *st = (struct st_context *) stctxi;
   struct gl_context *ctx = st->ctx;
   struct gl_texture_unit *texUnit = _mesa_get_current_tex_unit(ctx);
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   struct st_texture_object *stObj;
   struct st_texture_image *stImage;
   GLenum internalFormat;
   GLuint width, height, depth;

   switch (target) {
   case ST_TEXTURE_1D:
      target = GL_TEXTURE_1D;
      break;
   case ST_TEXTURE_2D:
      target = GL_TEXTURE_2D;
      break;
   case ST_TEXTURE_3D:
      target = GL_TEXTURE_3D;
      break;
   case ST_TEXTURE_RECT:
      target = GL_TEXTURE_RECTANGLE_ARB;
      break;
   default:
      return FALSE;
      break;
   }

   texObj = _mesa_select_tex_object(ctx, texUnit, target);
   _mesa_lock_texture(ctx, texObj);

   stObj = st_texture_object(texObj);
   /* switch to surface based */
   if (!stObj->surface_based) {
      _mesa_clear_texture_object(ctx, texObj);
      stObj->surface_based = GL_TRUE;
   }

   texImage = _mesa_get_tex_image(ctx, texObj, target, level);
   stImage = st_texture_image(texImage);
   if (tex) {
      gl_format texFormat;

      /*
       * XXX When internal_format and tex->format differ, st_finalize_texture
       * needs to allocate a new texture with internal_format and copy the
       * texture here into the new one.  It will result in surface_copy being
       * called on surfaces whose formats differ.
       *
       * To avoid that, internal_format is (wrongly) ignored here.  A sane fix
       * is to use a sampler view.
       */
      if (!st_sampler_compat_formats(tex->format, internal_format))
	 internal_format = tex->format;
     
      if (util_format_get_component_bits(internal_format,
               UTIL_FORMAT_COLORSPACE_RGB, 3) > 0)
         internalFormat = GL_RGBA;
      else
         internalFormat = GL_RGB;

      texFormat = st_ChooseTextureFormat(ctx, internalFormat,
                                         GL_RGBA, GL_UNSIGNED_BYTE);

      _mesa_init_teximage_fields(ctx, target, texImage,
                                 tex->width0, tex->height0, 1, 0,
                                 internalFormat, texFormat);

      width = tex->width0;
      height = tex->height0;
      depth = tex->depth0;

      /* grow the image size until we hit level = 0 */
      while (level > 0) {
         if (width != 1)
            width <<= 1;
         if (height != 1)
            height <<= 1;
         if (depth != 1)
            depth <<= 1;
         level--;
      }
   }
   else {
      _mesa_clear_texture_image(ctx, texImage);
      width = height = depth = 0;
   }

   pipe_resource_reference(&stImage->pt, tex);
   stObj->width0 = width;
   stObj->height0 = height;
   stObj->depth0 = depth;

   _mesa_dirty_texobj(ctx, texObj, GL_TRUE);
   _mesa_unlock_texture(ctx, texObj);
   
   return TRUE;
}

static void
st_context_copy(struct st_context_iface *stctxi,
                struct st_context_iface *stsrci, unsigned mask)
{
   struct st_context *st = (struct st_context *) stctxi;
   struct st_context *src = (struct st_context *) stsrci;

   _mesa_copy_context(src->ctx, st->ctx, mask);
}

static boolean
st_context_share(struct st_context_iface *stctxi,
                 struct st_context_iface *stsrci)
{
   struct st_context *st = (struct st_context *) stctxi;
   struct st_context *src = (struct st_context *) stsrci;

   return _mesa_share_state(st->ctx, src->ctx);
}

static void
st_context_destroy(struct st_context_iface *stctxi)
{
   struct st_context *st = (struct st_context *) stctxi;
   st_destroy_context(st);
}

static struct st_context_iface *
st_api_create_context(struct st_api *stapi, struct st_manager *smapi,
                      const struct st_context_attribs *attribs,
                      struct st_context_iface *shared_stctxi)
{
   struct st_context *shared_ctx = (struct st_context *) shared_stctxi;
   struct st_context *st;
   struct pipe_context *pipe;
   struct gl_config mode;
   gl_api api;

   if (!(stapi->profile_mask & (1 << attribs->profile)))
      return NULL;

   switch (attribs->profile) {
   case ST_PROFILE_DEFAULT:
      api = API_OPENGL;
      break;
   case ST_PROFILE_OPENGL_ES1:
      api = API_OPENGLES;
      break;
   case ST_PROFILE_OPENGL_ES2:
      api = API_OPENGLES2;
      break;
   case ST_PROFILE_OPENGL_CORE:
   default:
      return NULL;
      break;
   }

   pipe = smapi->screen->context_create(smapi->screen, NULL);
   if (!pipe)
      return NULL;

   st_visual_to_context_mode(&attribs->visual, &mode);
   st = st_create_context(api, pipe, &mode, shared_ctx);
   if (!st) {
      pipe->destroy(pipe);
      return NULL;
   }

   /* need to perform version check */
   if (attribs->major > 1 || attribs->minor > 0) {
      _mesa_compute_version(st->ctx);

      if (st->ctx->VersionMajor < attribs->major ||
          st->ctx->VersionMajor < attribs->minor) {
         st_destroy_context(st);
         return NULL;
      }
   }

   st->invalidate_on_gl_viewport =
      smapi->get_param(smapi, ST_MANAGER_BROKEN_INVALIDATE);

   st->iface.destroy = st_context_destroy;
   st->iface.notify_invalid_framebuffer =
      st_context_notify_invalid_framebuffer;
   st->iface.flush = st_context_flush;
   st->iface.teximage = st_context_teximage;
   st->iface.copy = st_context_copy;
   st->iface.share = st_context_share;
   st->iface.st_context_private = (void *) smapi;

   return &st->iface;
}

static boolean
st_api_make_current(struct st_api *stapi, struct st_context_iface *stctxi,
                    struct st_framebuffer_iface *stdrawi,
                    struct st_framebuffer_iface *streadi)
{
   struct st_context *st = (struct st_context *) stctxi;
   struct st_framebuffer *stdraw, *stread, *stfb;
   boolean ret;

   _glapi_check_multithread();

   if (st) {
      /* reuse/create the draw fb */
      stfb = st_ws_framebuffer(st->ctx->WinSysDrawBuffer);
      if (stfb && stfb->iface == stdrawi) {
         stdraw = NULL;
         st_framebuffer_reference(&stdraw, stfb);
      }
      else {
         stdraw = st_framebuffer_create(stdrawi);
      }

      /* reuse/create the read fb */
      stfb = st_ws_framebuffer(st->ctx->WinSysReadBuffer);
      if (!stfb || stfb->iface != streadi)
         stfb = stdraw;
      if (stfb && stfb->iface == streadi) {
         stread = NULL;
         st_framebuffer_reference(&stread, stfb);
      }
      else {
         stread = st_framebuffer_create(streadi);
      }

      if (stdraw && stread) {
         st_framebuffer_validate(stdraw, st);
         if (stread != stdraw)
            st_framebuffer_validate(stread, st);

         /* modify the draw/read buffers of the context */
         if (stdraw->iface) {
            st_visual_to_default_buffer(stdraw->iface->visual,
                  &st->ctx->Color.DrawBuffer[0], NULL);
         }
         if (stread->iface) {
            st_visual_to_default_buffer(stread->iface->visual,
                  &st->ctx->Pixel.ReadBuffer, NULL);
         }

         ret = _mesa_make_current(st->ctx, &stdraw->Base, &stread->Base);
      }
      else {
         ret = FALSE;
      }

      st_framebuffer_reference(&stdraw, NULL);
      st_framebuffer_reference(&stread, NULL);
   }
   else {
      ret = _mesa_make_current(NULL, NULL, NULL);
   }

   return ret;
}

static struct st_context_iface *
st_api_get_current(struct st_api *stapi)
{
   GET_CURRENT_CONTEXT(ctx);
   struct st_context *st = (ctx) ? ctx->st : NULL;

   return (st) ? &st->iface : NULL;
}

static st_proc_t
st_api_get_proc_address(struct st_api *stapi, const char *procname)
{
   return (st_proc_t) _glapi_get_proc_address(procname);
}

static void
st_api_destroy(struct st_api *stapi)
{
}

/**
 * Flush the front buffer if the current context renders to the front buffer.
 */
void
st_manager_flush_frontbuffer(struct st_context *st)
{
   struct st_framebuffer *stfb = st_ws_framebuffer(st->ctx->DrawBuffer);
   struct st_renderbuffer *strb = NULL;

   if (stfb)
      strb = st_renderbuffer(stfb->Base.Attachment[BUFFER_FRONT_LEFT].Renderbuffer);
   if (!strb)
      return;

   /* never a dummy fb */
   assert(stfb->iface);
   stfb->iface->flush_front(stfb->iface, ST_ATTACHMENT_FRONT_LEFT);
}

/**
 * Return the surface of an EGLImage.
 * FIXME: I think this should operate on resources, not surfaces
 */
struct pipe_surface *
st_manager_get_egl_image_surface(struct st_context *st,
                                 void *eglimg, unsigned usage)
{
   struct st_manager *smapi =
      (struct st_manager *) st->iface.st_context_private;
   struct st_egl_image stimg;
   struct pipe_surface *ps, surf_tmpl;

   if (!smapi || !smapi->get_egl_image)
      return NULL;

   memset(&stimg, 0, sizeof(stimg));
   if (!smapi->get_egl_image(smapi, eglimg, &stimg))
      return NULL;

   memset(&surf_tmpl, 0, sizeof(surf_tmpl));
   surf_tmpl.format = stimg.texture->format;
   surf_tmpl.usage = usage;
   surf_tmpl.u.tex.level = stimg.level;
   surf_tmpl.u.tex.first_layer = stimg.layer;
   surf_tmpl.u.tex.last_layer = stimg.layer;
   ps = st->pipe->create_surface(st->pipe, stimg.texture, &surf_tmpl);
   pipe_resource_reference(&stimg.texture, NULL);

   return ps;
}

/**
 * Re-validate the framebuffers.
 */
void
st_manager_validate_framebuffers(struct st_context *st)
{
   struct st_framebuffer *stdraw = st_ws_framebuffer(st->ctx->DrawBuffer);
   struct st_framebuffer *stread = st_ws_framebuffer(st->ctx->ReadBuffer);

   if (stdraw)
      st_framebuffer_validate(stdraw, st);
   if (stread && stread != stdraw)
      st_framebuffer_validate(stread, st);
}

/**
 * Add a color renderbuffer on demand.
 */
boolean
st_manager_add_color_renderbuffer(struct st_context *st,
                                  struct gl_framebuffer *fb,
                                  gl_buffer_index idx)
{
   struct st_framebuffer *stfb = st_ws_framebuffer(fb);

   /* FBO */
   if (!stfb)
      return FALSE;

   if (stfb->Base.Attachment[idx].Renderbuffer)
      return TRUE;

   switch (idx) {
   case BUFFER_FRONT_LEFT:
   case BUFFER_BACK_LEFT:
   case BUFFER_FRONT_RIGHT:
   case BUFFER_BACK_RIGHT:
      break;
   default:
      return FALSE;
      break;
   }

   if (!st_framebuffer_add_renderbuffer(stfb, idx))
      return FALSE;

   st_framebuffer_update_attachments(stfb);
   st_invalidate_state(st->ctx, _NEW_BUFFERS);

   return TRUE;
}

static const struct st_api st_gl_api = {
   "Mesa " MESA_VERSION_STRING,
   ST_API_OPENGL,
#if FEATURE_GL
   ST_PROFILE_DEFAULT_MASK |
#endif
#if FEATURE_ES1
   ST_PROFILE_OPENGL_ES1_MASK |
#endif
#if FEATURE_ES2
   ST_PROFILE_OPENGL_ES2_MASK |
#endif
   0,
   st_api_destroy,
   st_api_get_proc_address,
   st_api_create_context,
   st_api_make_current,
   st_api_get_current,
};

struct st_api *
st_gl_api_create(void)
{
   return (struct st_api *) &st_gl_api;
}
