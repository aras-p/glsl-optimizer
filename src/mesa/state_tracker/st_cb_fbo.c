/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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


/**
 * Framebuffer/renderbuffer functions.
 *
 * \author Brian Paul
 */


#include "main/imports.h"
#include "main/context.h"
#include "main/fbobject.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"
#include "pipe/p_winsys.h"
#include "st_context.h"
#include "st_cb_fbo.h"
#include "st_cb_texture.h"
#include "st_format.h"
#include "st_public.h"
#include "st_texture.h"



/**
 * Compute the renderbuffer's Red/Green/EtcBit fields from the pipe format.
 */
static int
init_renderbuffer_bits(struct st_renderbuffer *strb,
                       enum pipe_format pipeFormat)
{
   struct pipe_format_info info;

   if (!st_get_format_info( pipeFormat, &info )) {
      assert( 0 );
   }

   strb->Base._ActualFormat = info.base_format;
   strb->Base.RedBits = info.red_bits;
   strb->Base.GreenBits = info.green_bits;
   strb->Base.BlueBits = info.blue_bits;
   strb->Base.AlphaBits = info.alpha_bits;
   strb->Base.DepthBits = info.depth_bits;
   strb->Base.StencilBits = info.stencil_bits;
   strb->Base.DataType = st_format_datatype(pipeFormat);

   return info.size;
}


/**
 * gl_renderbuffer::AllocStorage()
 * This is called to allocate the original drawing surface, and
 * during window resize.
 */
static GLboolean
st_renderbuffer_alloc_storage(GLcontext * ctx, struct gl_renderbuffer *rb,
                              GLenum internalFormat,
                              GLuint width, GLuint height)
{
   struct pipe_context *pipe = ctx->st->pipe;
   struct st_renderbuffer *strb = st_renderbuffer(rb);
   enum pipe_format pipeFormat;
   GLbitfield flags = 0x0; /* XXX needed? */
   int ret;

   if (!strb->surface) {
      /* first time surface creation */
      strb->surface = pipe->winsys->surface_alloc(pipe->winsys);
      assert(strb->surface);
      assert(strb->surface->refcount);
      assert(strb->surface->winsys);
      if (!strb->surface)
         return GL_FALSE;
   }
   else if (strb->surface->buffer) {
      /* release/discard the old surface buffer */
      pipe_buffer_reference(pipe->winsys, &strb->surface->buffer, NULL);
   }

   /* Determine surface format here */
   if (strb->format != PIPE_FORMAT_NONE) {
      assert(strb->format != 0);
      /* we'll hit this for front/back color bufs */
      pipeFormat = strb->format;
   }
   else {
      pipeFormat = st_choose_renderbuffer_format(pipe, internalFormat);
   }

   init_renderbuffer_bits(strb, pipeFormat);

   ret = pipe->winsys->surface_alloc_storage(pipe->winsys,
                                             strb->surface,
                                             width,
                                             height,
                                             pipeFormat,
                                             flags);
   if (ret || !strb->surface->buffer) {
      if (pipeFormat == DEFAULT_ACCUM_PIPE_FORMAT) {
         /* Accum buffer.  Try a different surface format.  Since accum
          * buffers are s/w only for now, the surface pixel format doesn't
          * really matter, only that the buffer is large enough.
          */
         int sz, mult;
         enum pipe_format accum_format;

         /* allocate a buffer of (typically) double height to get 64bpp */
         accum_format = st_choose_renderbuffer_format(pipe, GL_RGBA);
         sz = pf_get_size(accum_format);
         mult = pf_get_size(DEFAULT_ACCUM_PIPE_FORMAT) / sz;

         ret = pipe->winsys->surface_alloc_storage(pipe->winsys,
                                                   strb->surface,
                                                   width, height * mult,
                                                   accum_format, flags);
         if (ret)
            return GL_FALSE; /* we've _really_ failed */

      }
      else {
         return GL_FALSE; /* out of memory, try s/w buffer? */
      }
   }

   ASSERT(strb->surface->buffer);
   ASSERT(strb->surface->format);
   ASSERT(strb->surface->cpp);
   ASSERT(strb->surface->width == width);
   /*ASSERT(strb->surface->height == height);*/
   ASSERT(strb->surface->pitch);

   strb->Base.Width  = width;
   strb->Base.Height = height;

   return GL_TRUE;
}


/**
 * gl_renderbuffer::Delete()
 */
static void
st_renderbuffer_delete(struct gl_renderbuffer *rb)
{
   struct st_renderbuffer *strb = st_renderbuffer(rb);
   ASSERT(strb);
   if (strb->surface) {
      struct pipe_winsys *ws = strb->surface->winsys;
      ws->surface_release(ws, &strb->surface);
   }
   free(strb);
}


/**
 * gl_renderbuffer::GetPointer()
 */
static void *
null_get_pointer(GLcontext * ctx, struct gl_renderbuffer *rb,
                 GLint x, GLint y)
{
   /* By returning NULL we force all software rendering to go through
    * the span routines.
    */
#if 0
   assert(0);  /* Should never get called with softpipe */
#endif
   return NULL;
}


/**
 * Called via ctx->Driver.NewFramebuffer()
 */
static struct gl_framebuffer *
st_new_framebuffer(GLcontext *ctx, GLuint name)
{
   /* XXX not sure we need to subclass gl_framebuffer for pipe */
   return _mesa_new_framebuffer(ctx, name);
}


/**
 * Called via ctx->Driver.NewRenderbuffer()
 */
static struct gl_renderbuffer *
st_new_renderbuffer(GLcontext *ctx, GLuint name)
{
   struct st_renderbuffer *strb = CALLOC_STRUCT(st_renderbuffer);
   if (strb) {
      _mesa_init_renderbuffer(&strb->Base, name);
      strb->Base.Delete = st_renderbuffer_delete;
      strb->Base.AllocStorage = st_renderbuffer_alloc_storage;
      strb->Base.GetPointer = null_get_pointer;
      strb->format = PIPE_FORMAT_NONE;
      return &strb->Base;
   }
   return NULL;
}


/**
 * Allocate a renderbuffer for a an on-screen window (not a user-created
 * renderbuffer).  The window system code determines the format.
 */
struct gl_renderbuffer *
st_new_renderbuffer_fb(enum pipe_format format)
{
   struct st_renderbuffer *strb;

   strb = CALLOC_STRUCT(st_renderbuffer);
   if (!strb) {
      _mesa_error(NULL, GL_OUT_OF_MEMORY, "creating renderbuffer");
      return NULL;
   }

   _mesa_init_renderbuffer(&strb->Base, 0);
   strb->Base.ClassID = 0x4242; /* just a unique value */
   strb->format = format;

   switch (format) {
   case PIPE_FORMAT_A8R8G8B8_UNORM:
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_A1R5G5B5_UNORM:
   case PIPE_FORMAT_A4R4G4B4_UNORM:
   case PIPE_FORMAT_R5G6B5_UNORM:
      strb->Base.InternalFormat = GL_RGBA;
      strb->Base._BaseFormat = GL_RGBA;
      break;
   case PIPE_FORMAT_Z16_UNORM:
      strb->Base.InternalFormat = GL_DEPTH_COMPONENT16;
      strb->Base._BaseFormat = GL_DEPTH_COMPONENT;
      break;
   case PIPE_FORMAT_Z32_UNORM:
      strb->Base.InternalFormat = GL_DEPTH_COMPONENT32;
      strb->Base._BaseFormat = GL_DEPTH_COMPONENT;
      break;
   case PIPE_FORMAT_S8Z24_UNORM:
   case PIPE_FORMAT_Z24S8_UNORM:
      strb->Base.InternalFormat = GL_DEPTH24_STENCIL8_EXT;
      strb->Base._BaseFormat = GL_DEPTH_STENCIL_EXT;
      break;
   case PIPE_FORMAT_S8_UNORM:
      strb->Base.InternalFormat = GL_STENCIL_INDEX8_EXT;
      strb->Base._BaseFormat = GL_STENCIL_INDEX;
      break;
   case DEFAULT_ACCUM_PIPE_FORMAT: /*PIPE_FORMAT_R16G16B16A16_SNORM*/
      strb->Base.InternalFormat = GL_RGBA16;
      strb->Base._BaseFormat = GL_RGBA;
      break;
   default:
      _mesa_problem(NULL,
		    "Unexpected format in st_new_renderbuffer_fb");
      return NULL;
   }

   /* st-specific methods */
   strb->Base.Delete = st_renderbuffer_delete;
   strb->Base.AllocStorage = st_renderbuffer_alloc_storage;
   strb->Base.GetPointer = null_get_pointer;

   /* surface is allocated in st_renderbuffer_alloc_storage() */
   strb->surface = NULL;

   return &strb->Base;
}




/**
 * Called via ctx->Driver.BindFramebufferEXT().
 */
static void
st_bind_framebuffer(GLcontext *ctx, GLenum target,
                    struct gl_framebuffer *fb, struct gl_framebuffer *fbread)
{

}

/**
 * Called by ctx->Driver.FramebufferRenderbuffer
 */
static void
st_framebuffer_renderbuffer(GLcontext *ctx, 
                            struct gl_framebuffer *fb,
                            GLenum attachment,
                            struct gl_renderbuffer *rb)
{
   /* XXX no need for derivation? */
   _mesa_framebuffer_renderbuffer(ctx, fb, attachment, rb);
}


/**
 * Called by ctx->Driver.RenderTexture
 */
static void
st_render_texture(GLcontext *ctx,
                  struct gl_framebuffer *fb,
                  struct gl_renderbuffer_attachment *att)
{
   struct st_context *st = ctx->st;
   struct st_renderbuffer *strb;
   struct gl_renderbuffer *rb;
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_texture *pt;

   assert(!att->Renderbuffer);

   /* create new renderbuffer which wraps the texture image */
   rb = st_new_renderbuffer(ctx, 0);
   if (!rb) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glFramebufferTexture()");
      return;
   }

   _mesa_reference_renderbuffer(&att->Renderbuffer, rb);
   assert(rb->RefCount == 1);
   rb->AllocStorage = NULL; /* should not get called */
   strb = st_renderbuffer(rb);

   /* get the texture for the texture object */
   pt = st_get_texobj_texture(att->Texture);
   assert(pt);
   assert(pt->width[att->TextureLevel]);

   rb->Width = pt->width[att->TextureLevel];
   rb->Height = pt->height[att->TextureLevel];

   /* the renderbuffer's surface is inside the texture */
   strb->surface = screen->get_tex_surface(screen, pt,
                                           att->CubeMapFace,
                                           att->TextureLevel,
                                           att->Zoffset);
   assert(strb->surface);

   init_renderbuffer_bits(strb, pt->format);

   /*
   printf("RENDER TO TEXTURE obj=%p pt=%p surf=%p  %d x %d\n",
          att->Texture, pt, strb->surface, rb->Width, rb->Height);
   */

   /* Invalidate buffer state so that the pipe's framebuffer state
    * gets updated.
    * That's where the new renderbuffer (which we just created) gets
    * passed to the pipe as a (color/depth) render target.
    */
   st_invalidate_state(ctx, _NEW_BUFFERS);
}


/**
 * Called via ctx->Driver.FinishRenderTexture.
 */
static void
st_finish_render_texture(GLcontext *ctx,
                         struct gl_renderbuffer_attachment *att)
{
   struct st_renderbuffer *strb = st_renderbuffer(att->Renderbuffer);

   assert(strb);

   ctx->st->pipe->flush(ctx->st->pipe, PIPE_FLUSH_RENDER_CACHE, NULL);

   /*
   printf("FINISH RENDER TO TEXTURE surf=%p\n", strb->surface);
   */

   pipe_surface_reference(&strb->surface, NULL);

   _mesa_reference_renderbuffer(&att->Renderbuffer, NULL);

   /* restore previous framebuffer state */
   st_invalidate_state(ctx, _NEW_BUFFERS);
}



void st_init_fbo_functions(struct dd_function_table *functions)
{
   functions->NewFramebuffer = st_new_framebuffer;
   functions->NewRenderbuffer = st_new_renderbuffer;
   functions->BindFramebuffer = st_bind_framebuffer;
   functions->FramebufferRenderbuffer = st_framebuffer_renderbuffer;
   functions->RenderTexture = st_render_texture;
   functions->FinishRenderTexture = st_finish_render_texture;
   /* no longer needed by core Mesa, drivers handle resizes...
   functions->ResizeBuffers = st_resize_buffers;
   */
}
