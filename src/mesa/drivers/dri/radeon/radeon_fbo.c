/**************************************************************************
 * 
 * Copyright 2008 Red Hat Inc.
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


#include "main/imports.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "main/fbobject.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "main/context.h"
#include "main/texformat.h"
#include "main/texrender.h"

#include "radeon_common.h"

static struct gl_framebuffer *
radeon_new_framebuffer(GLcontext *ctx, GLuint name)
{
  return _mesa_new_framebuffer(ctx, name);
}

static void
radeon_delete_renderbuffer(struct gl_renderbuffer *rb)
{
  GET_CURRENT_CONTEXT(ctx);
  struct radeon_renderbuffer *rrb = radeon_renderbuffer(rb);

  ASSERT(rrb);

  if (rrb && rrb->bo) {
    radeon_bo_unref(rrb->bo);
  }


  _mesa_free(rrb);
}

static void
radeon_resize_buffers(GLcontext *ctx, struct gl_framebuffer *fb,
		     GLuint width, GLuint height)
{

}

static void *
radeon_get_pointer(GLcontext *ctx, struct gl_renderbuffer *rb,
		   GLint x, GLint y)
{
  return NULL;
}

/**
 * Called via glRenderbufferStorageEXT() to set the format and allocate
 * storage for a user-created renderbuffer.
 */
static GLboolean
radeon_alloc_renderbuffer_storage(GLcontext * ctx, struct gl_renderbuffer *rb,
                                 GLenum internalFormat,
                                 GLuint width, GLuint height)
{
  struct radeon_context *radeon = RADEON_CONTEXT(ctx);
  struct radeon_renderbuffer *rrb = radeon_renderbuffer(rb);
  GLboolean software_buffer = GL_FALSE;
  int cpp;

   ASSERT(rb->Name != 0);

   if (software_buffer) {
      return _mesa_soft_renderbuffer_storage(ctx, rb, internalFormat,
                                             width, height);
   }
   else {
     /* TODO Alloc a BO */
       return GL_TRUE;
   }    
   
}

static struct gl_renderbuffer *
radeon_new_renderbuffer(GLcontext * ctx, GLuint name)
{
  struct radeon_renderbuffer *rrb;

  rrb = CALLOC_STRUCT(radeon_renderbuffer);
  if (!rrb)
    return NULL;

  _mesa_init_renderbuffer(&rrb->base, name);
  rrb->base.ClassID = RADEON_RB_CLASS;

  rrb->base.Delete = radeon_delete_renderbuffer;
  rrb->base.AllocStorage = radeon_alloc_renderbuffer_storage;
  rrb->base.GetPointer = radeon_get_pointer;

  return &rrb->base;
}

static void
radeon_bind_framebuffer(GLcontext * ctx, GLenum target,
                       struct gl_framebuffer *fb, struct gl_framebuffer *fbread)
{
   if (target == GL_FRAMEBUFFER_EXT || target == GL_DRAW_FRAMEBUFFER_EXT) {
      radeon_draw_buffer(ctx, fb);
   }
   else {
      /* don't need to do anything if target == GL_READ_FRAMEBUFFER_EXT */
   }
}

static void
radeon_framebuffer_renderbuffer(GLcontext * ctx,
                               struct gl_framebuffer *fb,
                               GLenum attachment, struct gl_renderbuffer *rb)
{

   radeonFlush(ctx);

   _mesa_framebuffer_renderbuffer(ctx, fb, attachment, rb);
   radeon_draw_buffer(ctx, fb);
}

static struct radeon_renderbuffer *
radeon_wrap_texture(GLcontext * ctx, struct gl_texture_image *texImage)
{

}
static void
radeon_render_texture(GLcontext * ctx,
                     struct gl_framebuffer *fb,
                     struct gl_renderbuffer_attachment *att)
{

}

static void
radeon_finish_render_texture(GLcontext * ctx,
                            struct gl_renderbuffer_attachment *att)
{

}
static void
radeon_validate_framebuffer(GLcontext *ctx, struct gl_framebuffer *fb)
{
}

static void
radeon_blit_framebuffer(GLcontext *ctx,
                       GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                       GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                       GLbitfield mask, GLenum filter)
{
}

void radeon_fbo_init(struct radeon_context *radeon)
{
  radeon->glCtx->Driver.NewFramebuffer = radeon_new_framebuffer;
  radeon->glCtx->Driver.NewRenderbuffer = radeon_new_renderbuffer;
  radeon->glCtx->Driver.BindFramebuffer = radeon_bind_framebuffer;
  radeon->glCtx->Driver.FramebufferRenderbuffer = radeon_framebuffer_renderbuffer;
  radeon->glCtx->Driver.RenderTexture = radeon_render_texture;
  radeon->glCtx->Driver.FinishRenderTexture = radeon_finish_render_texture;
  radeon->glCtx->Driver.ResizeBuffers = radeon_resize_buffers;
  radeon->glCtx->Driver.ValidateFramebuffer = radeon_validate_framebuffer;
  radeon->glCtx->Driver.BlitFramebuffer = radeon_blit_framebuffer;
}

  
  
