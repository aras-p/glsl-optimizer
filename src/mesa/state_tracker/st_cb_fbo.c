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
#include "st_context.h"
#include "st_cb_fbo.h"
#include "st_cb_teximage.h"


/**
 * Derived renderbuffer class.  Just need to add a pointer to the
 * pipe surface.
 */
struct st_renderbuffer
{
   struct gl_renderbuffer Base;
   struct pipe_surface *surface;
};


/**
 * Cast wrapper.
 */
static INLINE struct st_renderbuffer *
st_renderbuffer(struct gl_renderbuffer *rb)
{
   return (struct st_renderbuffer *) rb;
}


struct pipe_format_info
{
   GLuint format;
   GLenum base_format;
   GLubyte red_bits;
   GLubyte green_bits;
   GLubyte blue_bits;
   GLubyte alpha_bits;
   GLubyte luminance_bits;
   GLubyte intensity_bits;
   GLubyte depth_bits;
   GLubyte stencil_bits;
   GLubyte size;           /**< in bytes */
};


/*
 * XXX temporary here
 */
static const struct pipe_format_info *
pipe_get_format_info(GLuint format)
{
   static const struct pipe_format_info info[] = {
      {
         PIPE_FORMAT_U_R8_G8_B8_A8,  /* format */
         GL_RGBA,                    /* base_format */
         4, 4, 4, 4, 0, 0,           /* color bits */
         0, 0,                       /* depth, stencil */
         4                           /* size in bytes */
      },
      {
         PIPE_FORMAT_U_A8_R8_G8_B8,
         GL_RGBA,                    /* base_format */
         4, 4, 4, 4, 0, 0,           /* color bits */
         0, 0,                       /* depth, stencil */
         4                           /* size in bytes */
      },
      {
         PIPE_FORMAT_U_A1_R5_G5_B5,
         GL_RGBA,                    /* base_format */
         5, 5, 5, 1, 0, 0,           /* color bits */
         0, 0,                       /* depth, stencil */
         2                           /* size in bytes */
      },
      {
         PIPE_FORMAT_U_R5_G6_B5,
         GL_RGBA,                    /* base_format */
         5, 6, 5, 0, 0, 0,           /* color bits */
         0, 0,                       /* depth, stencil */
         2                           /* size in bytes */
      },
      /* XXX lots more */
      {
         PIPE_FORMAT_S8_Z24,
         GL_DEPTH_STENCIL_EXT,       /* base_format */
         0, 0, 0, 0, 0, 0,           /* color bits */
         24, 8,                      /* depth, stencil */
         4                           /* size in bytes */
      }
   };         
   GLuint i;

   for (i = 0; i < sizeof(info) / sizeof(info[0]); i++) {
      if (info[i].format == format)
         return info + i;
   }
   return NULL;
}


/**
 * gl_renderbuffer::AllocStorage()
 */
static GLboolean
st_renderbuffer_alloc_storage(GLcontext * ctx, struct gl_renderbuffer *rb,
                              GLenum internalFormat,
                              GLuint width, GLuint height)
{
   struct pipe_context *pipe = ctx->st->pipe;
   struct st_renderbuffer *strb = st_renderbuffer(rb);
   const GLuint pipeFormat
      = st_choose_pipe_format(pipe, internalFormat, GL_NONE, GL_NONE);
   const struct pipe_format_info *info = pipe_get_format_info(pipeFormat);
   GLuint cpp, pitch;

   if (!info)
      return GL_FALSE;

   strb->Base._ActualFormat = info->base_format;
   strb->Base.DataType = GL_UNSIGNED_BYTE; /* XXX fix */
   strb->Base.RedBits = info->red_bits;
   strb->Base.GreenBits = info->green_bits;
   strb->Base.BlueBits = info->blue_bits;
   strb->Base.AlphaBits = info->alpha_bits;
   strb->Base.DepthBits = info->depth_bits;
   strb->Base.StencilBits = info->stencil_bits;

   cpp = info->size;

   if (!strb->surface) {
      strb->surface = pipe->surface_alloc(pipe, pipeFormat);
      if (!strb->surface)
         return GL_FALSE;
   }

   /* free old region */
   if (strb->surface->region) {
      pipe->region_release(pipe, &strb->surface->region);
   }

   /* Choose a pitch to match hardware requirements:
    */
   pitch = ((cpp * width + 63) & ~63) / cpp; /* XXX fix: device-specific */

   strb->surface->region = pipe->region_alloc(pipe, cpp, pitch, height);
   if (!strb->surface->region)
      return GL_FALSE; /* out of memory, try s/w buffer? */

   ASSERT(strb->surface->region->buffer);

   strb->Base.Width  = strb->surface->width  = width;
   strb->Base.Height = strb->surface->height = height;

   return GL_TRUE;
}


/**
 * gl_renderbuffer::Delete()
 */
static void
st_renderbuffer_delete(struct gl_renderbuffer *rb)
{
   GET_CURRENT_CONTEXT(ctx);
   struct pipe_context *pipe = ctx->st->pipe;
   struct st_renderbuffer *strb = st_renderbuffer(rb);
   ASSERT(strb);
   if (strb && strb->surface) {
      if (rb->surface->region) {
         pipe->region_release(pipe, &strb->surface->region);
      }
      free(strb->surface);
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
   assert(0);  /* Should never get called with softpipe */
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
      return &strb->Base;
   }
   return NULL;
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
   struct pipe_framebuffer_state framebuffer;
   struct pipe_surface *texsurface;

   texsurface = NULL;  /* find the mipmap level, cube face, etc */

   /*
    * XXX basically like this... set the current color (or depth)
    * drawing surface to be the given texture renderbuffer.
    */
   memset(&framebuffer, 0, sizeof(framebuffer));
   framebuffer.num_cbufs = 1;
   framebuffer.cbufs[0] = texsurface;

   if (memcmp(&framebuffer, &st->state.framebuffer, sizeof(framebuffer)) != 0) {
      st->state.framebuffer = framebuffer;
      st->pipe->set_framebuffer_state( st->pipe, &framebuffer );
   }
}


/**
 * Called via ctx->Driver.FinishRenderTexture.
 */
static void
st_finish_render_texture(GLcontext *ctx,
                         struct gl_renderbuffer_attachment *att)
{
   /* restore drawing to normal framebuffer. may be a no-op */
}



void st_init_cb_fbo( struct st_context *st )
{
   struct dd_function_table *functions = &st->ctx->Driver;

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


void st_destroy_cb_fbo( struct st_context *st )
{
}
