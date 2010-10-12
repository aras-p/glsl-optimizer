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
#include "main/macros.h"
#include "main/renderbuffer.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_screen.h"
#include "st_context.h"
#include "st_cb_fbo.h"
#include "st_cb_flush.h"
#include "st_format.h"
#include "st_texture.h"
#include "st_manager.h"

#include "util/u_format.h"
#include "util/u_inlines.h"


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
   struct st_context *st = st_context(ctx);
   struct pipe_screen *screen = st->pipe->screen;
   struct st_renderbuffer *strb = st_renderbuffer(rb);
   enum pipe_format format;

   if (strb->format != PIPE_FORMAT_NONE)
      format = strb->format;
   else
      format = st_choose_renderbuffer_format(screen, internalFormat, rb->NumSamples);
      
   /* init renderbuffer fields */
   strb->Base.Width  = width;
   strb->Base.Height = height;
   strb->Base.Format = st_pipe_format_to_mesa_format(format);
   strb->Base.DataType = st_format_datatype(format);

   strb->defined = GL_FALSE;  /* undefined contents now */

   if (strb->software) {
      size_t size;
      
      free(strb->data);

      assert(strb->format != PIPE_FORMAT_NONE);
      
      strb->stride = util_format_get_stride(strb->format, width);
      size = util_format_get_2d_size(strb->format, strb->stride, height);
      
      strb->data = malloc(size);
      
      return strb->data != NULL;
   }
   else {
      struct pipe_resource template;
    
      /* Free the old surface and texture
       */
      pipe_surface_reference( &strb->surface, NULL );
      pipe_resource_reference( &strb->texture, NULL );
      pipe_sampler_view_reference(&strb->sampler_view, NULL);

      /* Setup new texture template.
       */
      memset(&template, 0, sizeof(template));
      template.target = st->internal_target;
      template.format = format;
      template.width0 = width;
      template.height0 = height;
      template.depth0 = 1;
      template.last_level = 0;
      template.nr_samples = rb->NumSamples;
      if (util_format_is_depth_or_stencil(format)) {
         template.bind = PIPE_BIND_DEPTH_STENCIL;
      }
      else {
         template.bind = (PIPE_BIND_DISPLAY_TARGET |
			  PIPE_BIND_RENDER_TARGET);
      }

      strb->texture = screen->resource_create(screen, &template);

      if (!strb->texture) 
         return FALSE;

      strb->surface = screen->get_tex_surface(screen,
                                              strb->texture,
                                              0, 0, 0,
                                              template.bind);
      if (strb->surface) {
         assert(strb->surface->texture);
         assert(strb->surface->format);
         assert(strb->surface->width == width);
         assert(strb->surface->height == height);
      }

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
   pipe_resource_reference(&strb->texture, NULL);
   pipe_sampler_view_reference(&strb->sampler_view, NULL);
   free(strb->data);
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
   strb->Base.Format = st_pipe_format_to_mesa_format(format);
   strb->Base.DataType = st_format_datatype(format);
   strb->format = format;
   strb->software = sw;
   
   switch (format) {
   case PIPE_FORMAT_R8G8B8A8_UNORM:
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_A8R8G8B8_UNORM:
   case PIPE_FORMAT_R8G8B8X8_UNORM:
   case PIPE_FORMAT_B8G8R8X8_UNORM:
   case PIPE_FORMAT_X8R8G8B8_UNORM:
   case PIPE_FORMAT_B5G5R5A1_UNORM:
   case PIPE_FORMAT_B4G4R4A4_UNORM:
   case PIPE_FORMAT_B5G6R5_UNORM:
      strb->Base.InternalFormat = GL_RGBA;
      break;
   case PIPE_FORMAT_Z16_UNORM:
      strb->Base.InternalFormat = GL_DEPTH_COMPONENT16;
      break;
   case PIPE_FORMAT_Z32_UNORM:
      strb->Base.InternalFormat = GL_DEPTH_COMPONENT32;
      break;
   case PIPE_FORMAT_Z24_UNORM_S8_USCALED:
   case PIPE_FORMAT_S8_USCALED_Z24_UNORM:
   case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM:
      strb->Base.InternalFormat = GL_DEPTH24_STENCIL8_EXT;
      break;
   case PIPE_FORMAT_S8_USCALED:
      strb->Base.InternalFormat = GL_STENCIL_INDEX8_EXT;
      break;
   case PIPE_FORMAT_R16G16B16A16_SNORM:
      strb->Base.InternalFormat = GL_RGBA16;
      break;
   case PIPE_FORMAT_R8_UNORM:
      strb->Base.InternalFormat = GL_R8;
      break;
   case PIPE_FORMAT_R8G8_UNORM:
      strb->Base.InternalFormat = GL_RG8;
      break;
   case PIPE_FORMAT_R16_UNORM:
      strb->Base.InternalFormat = GL_R16;
      break;
   case PIPE_FORMAT_R16G16_UNORM:
      strb->Base.InternalFormat = GL_RG16;
      break;
   default:
      _mesa_problem(NULL,
		    "Unexpected format in st_new_renderbuffer_fb");
      free(strb);
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
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct st_renderbuffer *strb;
   struct gl_renderbuffer *rb;
   struct pipe_resource *pt = st_get_texobj_resource(att->Texture);
   struct st_texture_object *stObj;
   const struct gl_texture_image *texImage;

   /* When would this fail?  Perhaps assert? */
   if (!pt) 
      return;

   /* get pointer to texture image we're rendeing to */
   texImage = att->Texture->Image[att->CubeMapFace][att->TextureLevel];

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
   rb->_BaseFormat = texImage->_BaseFormat;
   /*printf("***** render to texture level %d: %d x %d\n", att->TextureLevel, rb->Width, rb->Height);*/

   /*printf("***** pipe texture %d x %d\n", pt->width0, pt->height0);*/

   pipe_resource_reference( &strb->texture, pt );

   pipe_surface_reference(&strb->surface, NULL);

   pipe_sampler_view_reference(&strb->sampler_view,
                               st_get_texture_sampler_view(stObj, pipe));

   assert(strb->rtt_level <= strb->texture->last_level);

   /* new surface for rendering into the texture */
   strb->surface = screen->get_tex_surface(screen,
                                           strb->texture,
                                           strb->rtt_face,
                                           strb->rtt_level,
                                           strb->rtt_slice,
                                           PIPE_BIND_RENDER_TARGET);

   strb->format = pt->format;

   strb->Base.Format = st_pipe_format_to_mesa_format(pt->format);
   strb->Base.DataType = st_format_datatype(pt->format);

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
   struct st_context *st = st_context(ctx);
   struct st_renderbuffer *strb = st_renderbuffer(att->Renderbuffer);

   if (!strb)
      return;

   st_flush(st, PIPE_FLUSH_RENDER_CACHE, NULL);

   strb->rtt = NULL;

   /*
   printf("FINISH RENDER TO TEXTURE surf=%p\n", strb->surface);
   */

   /* restore previous framebuffer state */
   st_invalidate_state(ctx, _NEW_BUFFERS);
}


/**
 * Validate a renderbuffer attachment for a particular set of bindings.
 */
static GLboolean
st_validate_attachment(struct pipe_screen *screen,
		       const struct gl_renderbuffer_attachment *att,
		       unsigned bindings)
{
   const struct st_texture_object *stObj = st_texture_object(att->Texture);

   /* Only validate texture attachments for now, since
    * st_renderbuffer_alloc_storage makes sure that
    * the format is supported.
    */
   if (att->Type != GL_TEXTURE)
      return GL_TRUE;

   if (!stObj)
      return GL_FALSE;

   return screen->is_format_supported(screen, stObj->pt->format,
                                      PIPE_TEXTURE_2D,
                                      stObj->pt->nr_samples, bindings, 0);
}


/**
 * Check if two renderbuffer attachments name a combined depth/stencil
 * renderbuffer.
 */
GLboolean
st_is_depth_stencil_combined(const struct gl_renderbuffer_attachment *depth,
                             const struct gl_renderbuffer_attachment *stencil)
{
   assert(depth && stencil);

   if (depth->Type == stencil->Type) {
      if (depth->Type == GL_RENDERBUFFER_EXT &&
          depth->Renderbuffer == stencil->Renderbuffer)
         return GL_TRUE;

      if (depth->Type == GL_TEXTURE &&
          depth->Texture == stencil->Texture)
         return GL_TRUE;
   }

   return GL_FALSE;
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
   struct st_context *st = st_context(ctx);
   struct pipe_screen *screen = st->pipe->screen;
   const struct gl_renderbuffer_attachment *depth =
         &fb->Attachment[BUFFER_DEPTH];
   const struct gl_renderbuffer_attachment *stencil =
         &fb->Attachment[BUFFER_STENCIL];
   GLuint i;

   if (depth->Type && stencil->Type && depth->Type != stencil->Type) {
      fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
      return;
   }
   if (depth->Type == GL_RENDERBUFFER_EXT &&
       stencil->Type == GL_RENDERBUFFER_EXT &&
       depth->Renderbuffer != stencil->Renderbuffer) {
      fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
      return;
   }
   if (depth->Type == GL_TEXTURE &&
       stencil->Type == GL_TEXTURE &&
       depth->Texture != stencil->Texture) {
      fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
      return;
   }

   if (!st_validate_attachment(screen,
                               depth,
			       PIPE_BIND_DEPTH_STENCIL)) {
      fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
      return;
   }
   if (!st_validate_attachment(screen,
                               stencil,
			       PIPE_BIND_DEPTH_STENCIL)) {
      fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
      return;
   }
   for (i = 0; i < ctx->Const.MaxColorAttachments; i++) {
      if (!st_validate_attachment(screen,
				  &fb->Attachment[BUFFER_COLOR0 + i],
				  PIPE_BIND_RENDER_TARGET)) {
	 fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
	 return;
      }
   }
}


/**
 * Called via glDrawBuffer.
 */
static void
st_DrawBuffers(GLcontext *ctx, GLsizei count, const GLenum *buffers)
{
   struct st_context *st = st_context(ctx);
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   GLuint i;

   (void) count;
   (void) buffers;

   /* add the renderbuffers on demand */
   for (i = 0; i < fb->_NumColorDrawBuffers; i++) {
      gl_buffer_index idx = fb->_ColorDrawBufferIndexes[i];
      st_manager_add_color_renderbuffer(st, fb, idx);
   }
}


/**
 * Called via glReadBuffer.
 */
static void
st_ReadBuffer(GLcontext *ctx, GLenum buffer)
{
   struct st_context *st = st_context(ctx);
   struct gl_framebuffer *fb = ctx->ReadBuffer;

   (void) buffer;

   /* add the renderbuffer on demand */
   st_manager_add_color_renderbuffer(st, fb, fb->_ColorReadBufferIndex);
}


void st_init_fbo_functions(struct dd_function_table *functions)
{
#if FEATURE_EXT_framebuffer_object
   functions->NewFramebuffer = st_new_framebuffer;
   functions->NewRenderbuffer = st_new_renderbuffer;
   functions->BindFramebuffer = st_bind_framebuffer;
   functions->FramebufferRenderbuffer = st_framebuffer_renderbuffer;
   functions->RenderTexture = st_render_texture;
   functions->FinishRenderTexture = st_finish_render_texture;
   functions->ValidateFramebuffer = st_validate_framebuffer;
#endif
   /* no longer needed by core Mesa, drivers handle resizes...
   functions->ResizeBuffers = st_resize_buffers;
   */

   functions->DrawBuffers = st_DrawBuffers;
   functions->ReadBuffer = st_ReadBuffer;
}

/* XXX unused ? */
struct pipe_sampler_view *
st_get_renderbuffer_sampler_view(struct st_renderbuffer *rb,
                                 struct pipe_context *pipe)
{
   if (!rb->sampler_view) {
      rb->sampler_view = st_create_texture_sampler_view(pipe, rb->texture);
   }

   return rb->sampler_view;
}
