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
#include "pipe/p_screen.h"
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
   enum pipe_format format;

   if (strb->format != PIPE_FORMAT_NONE)
      format = strb->format;
   else
      format = st_choose_renderbuffer_format(pipe, internalFormat);
      
   /* init renderbuffer fields */
   strb->Base.Width  = width;
   strb->Base.Height = height;
   init_renderbuffer_bits(strb, format);

   if(strb->software) {
      struct pipe_format_block block;
      size_t size;
      
      _mesa_free(strb->data);

      assert(strb->format != PIPE_FORMAT_NONE);
      pf_get_block(strb->format, &block);
      
      strb->stride = pf_get_stride(&block, width);
      size = pf_get_2d_size(&block, strb->stride, height);
      
      strb->data = _mesa_malloc(size);
      
      return strb->data != NULL;
   }
   else {
      struct pipe_texture template;
      unsigned surface_usage;
    
      /* Free the old surface and texture
       */
      pipe_surface_reference( &strb->surface, NULL );
      pipe_texture_reference( &strb->texture, NULL );

      /* Setup new texture template.
       */
      memset(&template, 0, sizeof(template));
      template.target = PIPE_TEXTURE_2D;
      template.format = format;
      pf_get_block(format, &template.block);
      template.width[0] = width;
      template.height[0] = height;
      template.depth[0] = 1;
      template.last_level = 0;
      template.nr_samples = rb->NumSamples;
      if (pf_is_depth_stencil(format)) {
         template.tex_usage = PIPE_TEXTURE_USAGE_DEPTH_STENCIL;
      }
      else {
         template.tex_usage = (PIPE_TEXTURE_USAGE_DISPLAY_TARGET |
                               PIPE_TEXTURE_USAGE_RENDER_TARGET);
      }

      /* Probably need dedicated flags for surface usage too: 
       */
      surface_usage = (PIPE_BUFFER_USAGE_GPU_READ |
                       PIPE_BUFFER_USAGE_GPU_WRITE);
#if 0
                       PIPE_BUFFER_USAGE_CPU_READ |
                       PIPE_BUFFER_USAGE_CPU_WRITE);
#endif

      strb->texture = pipe->screen->texture_create( pipe->screen,
                                                    &template );

      if (!strb->texture) 
         return FALSE;

      strb->surface = pipe->screen->get_tex_surface( pipe->screen,
                                                     strb->texture,
                                                     0, 0, 0,
                                                     surface_usage );

      assert(strb->surface->texture);
      assert(strb->surface->format);
      assert(strb->surface->width == width);
      assert(strb->surface->height == height);


      return strb->surface != NULL;
   }
}


/**
 * gl_renderbuffer::Delete()
 */
static void
st_renderbuffer_delete(struct gl_renderbuffer *rb)
{
   struct st_renderbuffer *strb = st_renderbuffer(rb);
   ASSERT(strb);
   pipe_surface_reference(&strb->surface, NULL);
   pipe_texture_reference(&strb->texture, NULL);
   _mesa_free(strb->data);
   _mesa_free(strb);
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
   struct st_renderbuffer *strb = ST_CALLOC_STRUCT(st_renderbuffer);
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
st_new_renderbuffer_fb(enum pipe_format format, int samples, boolean sw)
{
   struct st_renderbuffer *strb;

   strb = ST_CALLOC_STRUCT(st_renderbuffer);
   if (!strb) {
      _mesa_error(NULL, GL_OUT_OF_MEMORY, "creating renderbuffer");
      return NULL;
   }

   _mesa_init_renderbuffer(&strb->Base, 0);
   strb->Base.ClassID = 0x4242; /* just a unique value */
   strb->Base.NumSamples = samples;
   strb->format = format;
   strb->software = sw;
   
   switch (format) {
   case PIPE_FORMAT_A8R8G8B8_UNORM:
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_X8R8G8B8_UNORM:
   case PIPE_FORMAT_B8G8R8X8_UNORM:
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
   case PIPE_FORMAT_X8Z24_UNORM:
   case PIPE_FORMAT_Z24X8_UNORM:
      strb->Base.InternalFormat = GL_DEPTH24_STENCIL8_EXT;
      strb->Base._BaseFormat = GL_DEPTH_STENCIL_EXT;
      break;
   case PIPE_FORMAT_S8_UNORM:
      strb->Base.InternalFormat = GL_STENCIL_INDEX8_EXT;
      strb->Base._BaseFormat = GL_STENCIL_INDEX;
      break;
   case PIPE_FORMAT_R16G16B16A16_SNORM:
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
   struct pipe_screen *screen = ctx->st->pipe->screen;
   struct st_renderbuffer *strb;
   struct gl_renderbuffer *rb;
   struct pipe_texture *pt = st_get_texobj_texture(att->Texture);
   struct st_texture_object *stObj;
   const struct gl_texture_image *texImage =
      att->Texture->Image[att->CubeMapFace][att->TextureLevel];

   if (!pt) 
      return;

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

   assert(strb->Base.RefCount > 0);

   /* get the texture for the texture object */
   stObj = st_texture_object(att->Texture);

   /* point renderbuffer at texobject */
   strb->rtt = stObj;
   strb->rtt_level = att->TextureLevel;
   strb->rtt_face = att->CubeMapFace;
   strb->rtt_slice = att->Zoffset;

   rb->Width = texImage->Width2;
   rb->Height = texImage->Height2;
   /*printf("***** render to texture level %d: %d x %d\n", att->TextureLevel, rb->Width, rb->Height);*/

   /*printf("***** pipe texture %d x %d\n", pt->width[0], pt->height[0]);*/

   pipe_texture_reference( &strb->texture, pt );

   pipe_surface_reference(&strb->surface, NULL);

   /* new surface for rendering into the texture */
   strb->surface = screen->get_tex_surface(screen,
                                           strb->texture,
                                           strb->rtt_face,
                                           strb->rtt_level,
                                           strb->rtt_slice,
                                           PIPE_BUFFER_USAGE_GPU_READ |
                                           PIPE_BUFFER_USAGE_GPU_WRITE);

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

   if (!strb)
      return;

   st_flush( ctx->st, PIPE_FLUSH_RENDER_CACHE, NULL );

   if (strb->surface)
      pipe_surface_reference( &strb->surface, NULL );

   strb->rtt = NULL;

   /*
   printf("FINISH RENDER TO TEXTURE surf=%p\n", strb->surface);
   */

   _mesa_reference_renderbuffer(&att->Renderbuffer, NULL);

   /* restore previous framebuffer state */
   st_invalidate_state(ctx, _NEW_BUFFERS);
}


/**
 * Check that the framebuffer configuration is valid in terms of what
 * the driver can support.
 *
 * For Gallium we only supports combined Z+stencil, not separate buffers.
 */
static void
st_validate_framebuffer(GLcontext *ctx, struct gl_framebuffer *fb)
{
   const struct gl_renderbuffer *depthRb =
      fb->Attachment[BUFFER_DEPTH].Renderbuffer;
   const struct gl_renderbuffer *stencilRb =
      fb->Attachment[BUFFER_STENCIL].Renderbuffer;

   if (stencilRb && depthRb && stencilRb != depthRb) {
      fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
   }
}


void st_init_fbo_functions(struct dd_function_table *functions)
{
   functions->NewFramebuffer = st_new_framebuffer;
   functions->NewRenderbuffer = st_new_renderbuffer;
   functions->BindFramebuffer = st_bind_framebuffer;
   functions->FramebufferRenderbuffer = st_framebuffer_renderbuffer;
   functions->RenderTexture = st_render_texture;
   functions->FinishRenderTexture = st_finish_render_texture;
   functions->ValidateFramebuffer = st_validate_framebuffer;
   /* no longer needed by core Mesa, drivers handle resizes...
   functions->ResizeBuffers = st_resize_buffers;
   */
}
