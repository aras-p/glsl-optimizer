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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "state_tracker/st_api.h"

#include "pipe/p_context.h"
#include "pipe/p_screen.h"
#include "util/u_format.h"
#include "util/u_pointer.h"
#include "util/u_inlines.h"
#include "util/u_atomic.h"

#include "main/mtypes.h"
#include "main/context.h"
#include "main/texobj.h"
#include "main/teximage.h"
#include "main/texstate.h"
#include "main/texfetch.h"
#include "main/fbobject.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "st_texture.h"

#include "st_context.h"
#include "st_format.h"
#include "st_cb_fbo.h"
#include "st_manager.h"

/* these functions are defined in st_context.c */
struct st_context *
st_create_context(struct pipe_context *pipe,
                  const __GLcontextModes *visual,
                  struct st_context *share);
void st_destroy_context(struct st_context *st);
void st_flush(struct st_context *st, uint pipeFlushFlags,
              struct pipe_fence_handle **fence);

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
 * Validate a framebuffer and update the states of the context.
 */
static void
st_framebuffer_validate(struct st_framebuffer *stfb, struct st_context *st)
{
   struct pipe_screen *screen = st->pipe->screen;
   struct pipe_texture *textures[ST_ATTACHMENT_COUNT];
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
      struct pipe_surface *ps;
      gl_buffer_index idx;

      if (!textures[i])
         continue;

      idx = attachment_to_buffer_index(stfb->statts[i]);
      if (idx >= BUFFER_COUNT) {
         pipe_texture_reference(&textures[i], NULL);
         continue;
      }

      strb = st_renderbuffer(stfb->Base.Attachment[idx].Renderbuffer);
      assert(strb);
      if (strb->texture == textures[i]) {
         pipe_texture_reference(&textures[i], NULL);
         continue;
      }

      ps = screen->get_tex_surface(screen, textures[i], 0, 0, 0,
            PIPE_BUFFER_USAGE_GPU_READ | PIPE_BUFFER_USAGE_GPU_WRITE);
      if (ps) {
         pipe_surface_reference(&strb->surface, ps);
         pipe_texture_reference(&strb->texture, ps->texture);
         /* ownership transfered */
         pipe_surface_reference(&ps, NULL);

         changed = TRUE;

         strb->Base.Width = strb->surface->width;
         strb->Base.Height = strb->surface->height;

         width = strb->Base.Width;
         height = strb->Base.Height;
      }

      pipe_texture_reference(&textures[i], NULL);
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
 * Update the attachments to validate.
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

   _mesa_add_renderbuffer(&stfb->Base, idx, rb);
   if (idx == BUFFER_DEPTH)
      _mesa_add_renderbuffer(&stfb->Base, BUFFER_STENCIL, rb);

   return TRUE;
}

/**
 * Intialize a __GLcontextModes from a visual.
 */
static void
st_visual_to_context_mode(const struct st_visual *visual,
                          __GLcontextModes *mode)
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
      mode->haveDepthBuffer = GL_TRUE;
      mode->haveStencilBuffer = GL_TRUE;

      mode->depthBits =
         util_format_get_component_bits(visual->depth_stencil_format,
               UTIL_FORMAT_COLORSPACE_ZS, 0);
      mode->stencilBits =
         util_format_get_component_bits(visual->depth_stencil_format,
               UTIL_FORMAT_COLORSPACE_ZS, 1);
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
   __GLcontextModes mode;
   gl_buffer_index idx;

   stfb = CALLOC_STRUCT(st_framebuffer);
   if (!stfb)
      return NULL;

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

   p_atomic_set(&stfb->revalidate, TRUE);
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
   GLframebuffer *fb = &stfb->Base;
   _mesa_reference_framebuffer((GLframebuffer **) ptr, fb);
}

static void
st_context_notify_invalid_framebuffer(struct st_context_iface *stctxi,
                                      struct st_framebuffer_iface *stfbi)
{
   struct st_context *st = (struct st_context *) stctxi;
   struct st_framebuffer *stfb;

   /* either draw or read winsys fb */
   stfb = (struct st_framebuffer *) st->ctx->WinSysDrawBuffer;
   if (!stfb || stfb->iface != stfbi)
      stfb = (struct st_framebuffer *) st->ctx->WinSysReadBuffer;
   assert(stfb && stfb->iface == stfbi);

   p_atomic_set(&stfb->revalidate, TRUE);
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
st_context_teximage(struct st_context_iface *stctxi, enum st_texture_type target,
                    int level, enum pipe_format internal_format,
                    struct pipe_texture *tex, boolean mipmap)
{
   struct st_context *st = (struct st_context *) stctxi;
   GLcontext *ctx = st->ctx;
   struct gl_texture_unit *texUnit = _mesa_get_current_tex_unit(ctx);
   struct gl_texture_object *texObj;
   struct gl_texture_image *texImage;
   struct st_texture_object *stObj;
   struct st_texture_image *stImage;
   GLenum internalFormat;

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

   if (util_format_get_component_bits(internal_format,
            UTIL_FORMAT_COLORSPACE_RGB, 3) > 0)
      internalFormat = GL_RGBA;
   else
      internalFormat = GL_RGB;

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
      _mesa_init_teximage_fields(ctx, target, texImage,
            tex->width0, tex->height0, 1, 0, internalFormat);
      texImage->TexFormat = st_ChooseTextureFormat(ctx, internalFormat,
            GL_RGBA, GL_UNSIGNED_BYTE);
      _mesa_set_fetch_functions(texImage, 2);
   }
   else {
      _mesa_clear_texture_image(ctx, texImage);
   }

   pipe_texture_reference(&stImage->pt, tex);

   _mesa_dirty_texobj(ctx, texObj, GL_TRUE);
   _mesa_unlock_texture(ctx, texObj);
   
   return TRUE;
}

static void
st_context_destroy(struct st_context_iface *stctxi)
{
   struct st_context *st = (struct st_context *) stctxi;
   st_destroy_context(st);
}

static struct st_context_iface *
st_api_create_context(struct st_api *stapi, struct st_manager *smapi,
                      const struct st_visual *visual,
                      struct st_context_iface *shared_stctxi)
{
   struct st_context *shared_ctx = (struct st_context *) shared_stctxi;
   struct st_context *st;
   struct pipe_context *pipe;
   __GLcontextModes mode;

   pipe = smapi->screen->context_create(smapi->screen, NULL);
   if (!pipe)
      return NULL;

   st_visual_to_context_mode(visual, &mode);
   st = st_create_context(pipe, &mode, shared_ctx);
   if (!st) {
      pipe->destroy(pipe);
      return NULL;
   }

   st->iface.destroy = st_context_destroy;

   st->iface.notify_invalid_framebuffer =
      st_context_notify_invalid_framebuffer;
   st->iface.flush = st_context_flush;

   st->iface.teximage = st_context_teximage;
   st->iface.copy = NULL;

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
      stfb = (struct st_framebuffer * ) st->ctx->DrawBuffer;
      if (stfb && stfb->iface == stdrawi) {
         stdraw = NULL;
         st_framebuffer_reference(&stdraw, stfb);
      }
      else {
         stdraw = st_framebuffer_create(stdrawi);
      }

      /* reuse/create the read fb */
      stfb = (struct st_framebuffer * ) st->ctx->ReadBuffer;
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
         st_visual_to_default_buffer(stdraw->iface->visual,
               &st->ctx->Color.DrawBuffer[0], NULL);
         st_visual_to_default_buffer(stread->iface->visual,
               &st->ctx->Pixel.ReadBuffer, NULL);

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

static boolean
st_api_is_visual_supported(struct st_api *stapi,
                           const struct st_visual *visual)
{
   return TRUE;
}

static st_proc_t
st_api_get_proc_address(struct st_api *stapi, const char *procname)
{
   return (st_proc_t) _glapi_get_proc_address(procname);
}

static void
st_api_destroy(struct st_api *stapi)
{
   FREE(stapi);
}

/**
 * Flush the front buffer if the current context renders to the front buffer.
 */
void
st_manager_flush_frontbuffer(struct st_context *st)
{
   struct st_framebuffer *stfb = (struct st_framebuffer *) st->ctx->DrawBuffer;
   struct st_renderbuffer *strb = NULL;

   if (stfb)
      strb = st_renderbuffer(stfb->Base.Attachment[BUFFER_FRONT_LEFT].Renderbuffer);
   if (!strb)
      return;

   /* st_public.h or FBO */
   if (!stfb->iface) {
      struct pipe_surface *front_surf = strb->surface;
      st->pipe->screen->flush_frontbuffer(st->pipe->screen,
            front_surf, st->winsys_drawable_handle);
      return;
   }

   stfb->iface->flush_front(stfb->iface, ST_ATTACHMENT_FRONT_LEFT);
}

/**
 * Re-validate the framebuffer.
 */
void
st_manager_validate_framebuffers(struct st_context *st)
{
   struct st_framebuffer *stdraw, *stread;

   stdraw = (struct st_framebuffer *) st->ctx->DrawBuffer;
   stread = (struct st_framebuffer *) st->ctx->ReadBuffer;

   /* st_public.h or FBO */
   if ((stdraw && !stdraw->iface) || (stread && !stread->iface)) {
      struct pipe_screen *screen = st->pipe->screen;
      if (screen->update_buffer)
         screen->update_buffer(screen, st->pipe->priv);
      return;
   }

   if (stdraw)
      st_framebuffer_validate(stdraw, st);
   if (stread && stread != stdraw)
      st_framebuffer_validate(stread, st);
}

/**
 * Add a color buffer on demand.
 */
boolean
st_manager_add_color_renderbuffer(struct st_context *st, GLframebuffer *fb,
                                  gl_buffer_index idx)
{
   struct st_framebuffer *stfb = (struct st_framebuffer *) fb;

   if (stfb->Base.Attachment[idx].Renderbuffer)
      return TRUE;

   /* st_public.h or FBO */
   if (!stfb->iface)
      return FALSE;

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

struct st_api *
st_manager_create_api(void)
{
   struct st_api *stapi;

   stapi = CALLOC_STRUCT(st_api);
   if (stapi) {
      stapi->destroy = st_api_destroy;
      stapi->get_proc_address = st_api_get_proc_address;
      stapi->is_visual_supported = st_api_is_visual_supported;

      stapi->create_context = st_api_create_context;
      stapi->make_current = st_api_make_current;
      stapi->get_current = st_api_get_current;
   }

   return stapi;
}
