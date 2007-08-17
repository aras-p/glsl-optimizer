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
#include "st_cb_texture.h"
#include "st_format.h"
#include "st_public.h"



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
   const struct pipe_format_info *info = st_get_format_info(pipeFormat);
   GLuint cpp;
   GLbitfield flags = PIPE_SURFACE_FLAG_RENDER; /* want to render to surface */

   assert(info);
   if (!info)
      return GL_FALSE;

   switch (pipeFormat) {
   case PIPE_FORMAT_S8_Z24:
      strb->Base.DataType = GL_UNSIGNED_INT;
      break;
   default:
      strb->Base.DataType = GL_UNSIGNED_BYTE; /* XXX fix */
   }

   strb->Base._ActualFormat = info->base_format;
   strb->Base.RedBits = info->red_bits;
   strb->Base.GreenBits = info->green_bits;
   strb->Base.BlueBits = info->blue_bits;
   strb->Base.AlphaBits = info->alpha_bits;
   strb->Base.DepthBits = info->depth_bits;
   strb->Base.StencilBits = info->stencil_bits;

   cpp = info->size;

   if (!strb->surface) {
      strb->surface = pipe->surface_alloc(pipe, pipeFormat);
      assert(strb->surface);
      if (!strb->surface)
         return GL_FALSE;
   }

   /* free old region */
   if (strb->surface->region) {
      pipe->region_release(pipe, &strb->surface->region);
   }

   strb->surface->region = pipe->region_alloc(pipe, cpp, width, height, flags);
   if (!strb->surface->region)
      return GL_FALSE; /* out of memory, try s/w buffer? */

   ASSERT(strb->surface->region->buffer);
   ASSERT(strb->surface->format);

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
   struct st_renderbuffer *strb = st_renderbuffer(rb);
   GET_CURRENT_CONTEXT(ctx);
   if (ctx) {
      struct pipe_context *pipe = ctx->st->pipe;
      ASSERT(strb);
      if (strb && strb->surface) {
         if (strb->surface->region) {
            pipe->region_release(pipe, &strb->surface->region);
         }
         free(strb->surface);
      }
   }
   else {
      _mesa_warning(NULL, "st_renderbuffer_delete() called, but no current context");
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
      return &strb->Base;
   }
   return NULL;
}


#if 000
struct gl_renderbuffer *
st_new_renderbuffer_fb(struct pipe_region *region, GLuint width, GLuint height)
{
   struct st_renderbuffer *strb = CALLOC_STRUCT(st_renderbuffer);
   if (!strb)
      return;

   _mesa_init_renderbuffer(&strb->Base, name);
   strb->Base.Delete = st_renderbuffer_delete;
   strb->Base.AllocStorage = st_renderbuffer_alloc_storage;
   strb->Base.GetPointer = null_get_pointer;
   strb->Base.Width = width;
   strb->Base.Heigth = height;

   strb->region = region;

   return &strb->Base;
}

#else

struct gl_renderbuffer *
st_new_renderbuffer_fb(GLenum intFormat)
{
   struct st_renderbuffer *strb;

   strb = CALLOC_STRUCT(st_renderbuffer);
   if (!strb) {
      _mesa_error(NULL, GL_OUT_OF_MEMORY, "creating renderbuffer");
      return NULL;
   }

   _mesa_init_renderbuffer(&strb->Base, 0);
   strb->Base.ClassID = 0x42; /* XXX temp */
   strb->Base.InternalFormat = intFormat;

   switch (intFormat) {
   case GL_RGB5:
   case GL_RGBA8:
      strb->Base._BaseFormat = GL_RGBA;
      break;
   case GL_DEPTH_COMPONENT16:
   case GL_DEPTH_COMPONENT32:
      strb->Base._BaseFormat = GL_DEPTH_COMPONENT;
      break;
   case GL_DEPTH24_STENCIL8_EXT:
      strb->Base._BaseFormat = GL_DEPTH_STENCIL_EXT;
      break;
   default:
      _mesa_problem(NULL,
		    "Unexpected intFormat in st_new_renderbuffer");
      return NULL;
   }

   /* st-specific methods */
   strb->Base.Delete = st_renderbuffer_delete;
   strb->Base.AllocStorage = st_renderbuffer_alloc_storage;
   strb->Base.GetPointer = null_get_pointer;

   /* surface is allocate in alloc_renderbuffer_storage() */
   strb->surface = NULL;

   return &strb->Base;
}
#endif



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
   struct pipe_mipmap_tree *mt;

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

   /* get the mipmap tree for the texture */
   mt = st_get_texobj_mipmap_tree(att->Texture);
   assert(mt);
   assert(mt->level[0].width);

   rb->Width = mt->level[0].width;
   rb->Height = mt->level[0].height;

   /* the renderbuffer's surface is inside the mipmap_tree: */
   strb->surface = pipe->get_tex_surface(pipe, mt,
                                         att->CubeMapFace,
                                         att->TextureLevel,
                                         att->Zoffset);
   assert(strb->surface);

   /*
   printf("RENDER TO TEXTURE obj=%p mt=%p surf=%p  %d x %d\n",
          att->Texture, mt, strb->surface, rb->Width, rb->Height);
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

   ctx->st->pipe->flush(ctx->st->pipe, 0x0);

   /*
   printf("FINISH RENDER TO TEXTURE surf=%p\n", strb->surface);
   */

   pipe_surface_unreference(&strb->surface);

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
