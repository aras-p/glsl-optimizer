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
#include "st_format.h"
#include "st_public.h"
#include "st_texture.h"

#include "util/u_format.h"
#include "util/u_rect.h"
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
   struct pipe_screen *screen = ctx->st->pipe->screen;
   struct st_renderbuffer *strb = st_renderbuffer(rb);
   enum pipe_format format;

   if (strb->format != PIPE_FORMAT_NONE)
      format = strb->format;
   else
      format = st_choose_renderbuffer_format(screen, internalFormat);
      
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
      template.width0 = width;
      template.height0 = height;
      template.depth0 = 1;
      template.last_level = 0;
      template.nr_samples = rb->NumSamples;
      if (util_format_is_depth_or_stencil(format)) {
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

      strb->texture = screen->texture_create(screen, &template);

      if (!strb->texture) 
         return FALSE;

      strb->surface = screen->get_tex_surface(screen,
                                              strb->texture,
                                              0, 0, 0,
                                              surface_usage);
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
   pipe_texture_reference(&strb->texture, NULL);
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
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_A8R8G8B8_UNORM:
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
   case PIPE_FORMAT_Z24S8_UNORM:
   case PIPE_FORMAT_S8Z24_UNORM:
   case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM:
      strb->Base.InternalFormat = GL_DEPTH24_STENCIL8_EXT;
      break;
   case PIPE_FORMAT_S8_UNORM:
      strb->Base.InternalFormat = GL_STENCIL_INDEX8_EXT;
      break;
   case PIPE_FORMAT_R16G16B16A16_SNORM:
      strb->Base.InternalFormat = GL_RGBA16;
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
   struct pipe_screen *screen = ctx->st->pipe->screen;
   struct st_renderbuffer *strb;
   struct gl_renderbuffer *rb;
   struct pipe_texture *pt = st_get_texobj_texture(att->Texture);
   struct st_texture_object *stObj;
   const struct gl_texture_image *texImage;
   GLint pt_level;

   /* When would this fail?  Perhaps assert? */
   if (!pt) 
      return;

   /* The first gallium texture level = Mesa BaseLevel */
   pt_level = MAX2(0, (GLint) att->TextureLevel - att->Texture->BaseLevel);
   texImage = att->Texture->Image[att->CubeMapFace][pt_level];

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
   strb->rtt_level = pt_level;
   strb->rtt_face = att->CubeMapFace;
   strb->rtt_slice = att->Zoffset;

   rb->Width = texImage->Width2;
   rb->Height = texImage->Height2;
   rb->_BaseFormat = texImage->_BaseFormat;
   /*printf("***** render to texture level %d: %d x %d\n", att->TextureLevel, rb->Width, rb->Height);*/

   /*printf("***** pipe texture %d x %d\n", pt->width0, pt->height0);*/

   pipe_texture_reference( &strb->texture, pt );

   pipe_surface_reference(&strb->surface, NULL);

   assert(strb->rtt_level <= strb->texture->last_level);

   /* new surface for rendering into the texture */
   strb->surface = screen->get_tex_surface(screen,
                                           strb->texture,
                                           strb->rtt_face,
                                           strb->rtt_level,
                                           strb->rtt_slice,
                                           PIPE_BUFFER_USAGE_GPU_READ |
                                           PIPE_BUFFER_USAGE_GPU_WRITE);

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
   struct st_renderbuffer *strb = st_renderbuffer(att->Renderbuffer);

   if (!strb)
      return;

   st_flush( ctx->st, PIPE_FLUSH_RENDER_CACHE, NULL );

   strb->rtt = NULL;

   /*
   printf("FINISH RENDER TO TEXTURE surf=%p\n", strb->surface);
   */

   /* restore previous framebuffer state */
   st_invalidate_state(ctx, _NEW_BUFFERS);
}


/**
 * Validate a renderbuffer attachment for a particular usage.
 */

static GLboolean
st_validate_attachment(struct pipe_screen *screen,
		       const struct gl_renderbuffer_attachment *att,
		       GLuint usage)
{
   const struct st_texture_object *stObj =
      st_texture_object(att->Texture);

   /**
    * Only validate texture attachments for now, since
    * st_renderbuffer_alloc_storage makes sure that
    * the format is supported.
    */

   if (att->Type != GL_TEXTURE)
      return GL_TRUE;

   if (!stObj)
      return GL_FALSE;

   return screen->is_format_supported(screen, stObj->pt->format,
				      PIPE_TEXTURE_2D,
				      usage, 0);
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
   struct pipe_screen *screen = ctx->st->pipe->screen;
   const struct gl_renderbuffer *depthRb =
      fb->Attachment[BUFFER_DEPTH].Renderbuffer;
   const struct gl_renderbuffer *stencilRb =
      fb->Attachment[BUFFER_STENCIL].Renderbuffer;
   GLuint i;

   if (stencilRb && depthRb && stencilRb != depthRb) {
      fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
      return;
   }

   if (!st_validate_attachment(screen,
			       &fb->Attachment[BUFFER_DEPTH],
			       PIPE_TEXTURE_USAGE_DEPTH_STENCIL)) {
      fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
      return;
   }
   if (!st_validate_attachment(screen,
			       &fb->Attachment[BUFFER_STENCIL],
			       PIPE_TEXTURE_USAGE_DEPTH_STENCIL)) {
      fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
      return;
   }
   for (i = 0; i < ctx->Const.MaxColorAttachments; i++) {
      if (!st_validate_attachment(screen,
				  &fb->Attachment[BUFFER_COLOR0 + i],
				  PIPE_TEXTURE_USAGE_RENDER_TARGET)) {
	 fb->_Status = GL_FRAMEBUFFER_UNSUPPORTED_EXT;
	 return;
      }
   }
}


/**
 * Copy back color buffer to front color buffer.
 */
static void
copy_back_to_front(struct st_context *st,
                   struct gl_framebuffer *fb,
                   gl_buffer_index frontIndex,
                   gl_buffer_index backIndex)

{
   struct st_framebuffer *stfb = (struct st_framebuffer *) fb;
   struct pipe_surface *surf_front, *surf_back;

   (void) st_get_framebuffer_surface(stfb, frontIndex, &surf_front);
   (void) st_get_framebuffer_surface(stfb, backIndex, &surf_back);

   if (surf_front && surf_back) {
      if (st->pipe->surface_copy) {
         st->pipe->surface_copy(st->pipe,
                                surf_front, 0, 0,  /* dest */
                                surf_back, 0, 0,   /* src */
                                fb->Width, fb->Height);
      } else {
         util_surface_copy(st->pipe, FALSE,
                           surf_front, 0, 0,
                           surf_back, 0, 0,
                           fb->Width, fb->Height);
      }
   }
}


/**
 * Check if we're drawing into, or read from, a front color buffer.  If the
 * front buffer is missing, create it now.
 *
 * The back color buffer must exist since we'll use its format/samples info
 * for creating the front buffer.
 *
 * \param frontIndex  either BUFFER_FRONT_LEFT or BUFFER_FRONT_RIGHT
 * \param backIndex  either BUFFER_BACK_LEFT or BUFFER_BACK_RIGHT
 */
static void
check_create_front_buffer(GLcontext *ctx, struct gl_framebuffer *fb,
                          gl_buffer_index frontIndex,
                          gl_buffer_index backIndex)
{
   if (fb->Attachment[frontIndex].Renderbuffer == NULL) {
      GLboolean create = GL_FALSE;

      /* check if drawing to or reading from front buffer */
      if (fb->_ColorReadBufferIndex == frontIndex) {
         create = GL_TRUE;
      }
      else {
         GLuint b;
         for (b = 0; b < fb->_NumColorDrawBuffers; b++) {
            if (fb->_ColorDrawBufferIndexes[b] == frontIndex) {
               create = GL_TRUE;
               break;
            }
         }
      }

      if (create) {
         struct st_renderbuffer *back;
         struct gl_renderbuffer *front;
         enum pipe_format colorFormat;
         uint samples;

         if (0)
            _mesa_debug(ctx, "Allocate new front buffer\n");

         /* get back renderbuffer info */
         back = st_renderbuffer(fb->Attachment[backIndex].Renderbuffer);
         colorFormat = back->format;
         samples = back->Base.NumSamples;

         /* create front renderbuffer */
         front = st_new_renderbuffer_fb(colorFormat, samples, FALSE);
         _mesa_add_renderbuffer(fb, frontIndex, front);

         /* alloc texture/surface for new front buffer */
         front->AllocStorage(ctx, front, front->InternalFormat,
                             fb->Width, fb->Height);

         /* initialize the front color buffer contents by copying
          * the back buffer.
          */
         copy_back_to_front(ctx->st, fb, frontIndex, backIndex);
      }
   }
}


/**
 * If front left/right color buffers are missing, create them now.
 */
static void
check_create_front_buffers(GLcontext *ctx, struct gl_framebuffer *fb)
{
   /* check if we need to create the front left buffer now */
   check_create_front_buffer(ctx, fb, BUFFER_FRONT_LEFT, BUFFER_BACK_LEFT);

   if (fb->Visual.stereoMode) {
      check_create_front_buffer(ctx, fb, BUFFER_FRONT_RIGHT, BUFFER_BACK_RIGHT);
   }

   st_invalidate_state(ctx, _NEW_BUFFERS);
}


/**
 * Called via glDrawBuffer.
 */
static void
st_DrawBuffers(GLcontext *ctx, GLsizei count, const GLenum *buffers)
{
   (void) count;
   (void) buffers;
   check_create_front_buffers(ctx, ctx->DrawBuffer);
}


/**
 * Called via glReadBuffer.
 */
static void
st_ReadBuffer(GLcontext *ctx, GLenum buffer)
{
   (void) buffer;
   check_create_front_buffers(ctx, ctx->ReadBuffer);
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

   functions->DrawBuffers = st_DrawBuffers;
   functions->ReadBuffer = st_ReadBuffer;
}
