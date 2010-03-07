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

 /*
  * Authors:
  *   Brian Paul
  */

#include "main/imports.h"
#include "main/image.h"
#include "main/macros.h"
#include "shader/program.h"

#include "st_context.h"
#include "st_texture.h"
#include "st_cb_blit.h"
#include "st_cb_fbo.h"

#include "util/u_blit.h"
#include "util/u_inlines.h"


void
st_init_blit(struct st_context *st)
{
   st->blit = util_create_blit(st->pipe, st->cso_context);
}


void
st_destroy_blit(struct st_context *st)
{
   util_destroy_blit(st->blit);
   st->blit = NULL;
}


#if FEATURE_EXT_framebuffer_blit
static void
st_BlitFramebuffer(GLcontext *ctx,
                   GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                   GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                   GLbitfield mask, GLenum filter)
{
   const GLbitfield depthStencil = (GL_DEPTH_BUFFER_BIT |
                                    GL_STENCIL_BUFFER_BIT);
   struct st_context *st = ctx->st;
   const uint pFilter = ((filter == GL_NEAREST)
                         ? PIPE_TEX_MIPFILTER_NEAREST
                         : PIPE_TEX_MIPFILTER_LINEAR);
   struct gl_framebuffer *readFB = ctx->ReadBuffer;
   struct gl_framebuffer *drawFB = ctx->DrawBuffer;

   if (!_mesa_clip_blit(ctx, &srcX0, &srcY0, &srcX1, &srcY1,
                        &dstX0, &dstY0, &dstX1, &dstY1)) {
      return; /* nothing to draw/blit */
   }

   if (st_fb_orientation(drawFB) == Y_0_TOP) {
      /* invert Y for dest */
      dstY0 = drawFB->Height - dstY0;
      dstY1 = drawFB->Height - dstY1;
   }

   if (st_fb_orientation(readFB) == Y_0_TOP) {
      /* invert Y for src */
      srcY0 = readFB->Height - srcY0;
      srcY1 = readFB->Height - srcY1;
   }

   if (srcY0 > srcY1 && dstY0 > dstY1) {
      /* Both src and dst are upside down.  Swap Y to make it
       * right-side up to increase odds of using a fast path.
       * Recall that all Gallium raster coords have Y=0=top.
       */
      GLint tmp;
      tmp = srcY0;
      srcY0 = srcY1;
      srcY1 = tmp;
      tmp = dstY0;
      dstY0 = dstY1;
      dstY1 = tmp;
   }

   if (mask & GL_COLOR_BUFFER_BIT) {
      struct gl_renderbuffer_attachment *srcAtt =
         &readFB->Attachment[readFB->_ColorReadBufferIndex];

      if(srcAtt->Type == GL_TEXTURE) {
         struct pipe_screen *screen = ctx->st->pipe->screen;
         const struct st_texture_object *srcObj =
            st_texture_object(srcAtt->Texture);
         struct st_renderbuffer *dstRb =
            st_renderbuffer(drawFB->_ColorDrawBuffers[0]);
         struct pipe_surface *srcSurf;
         struct pipe_surface *dstSurf = dstRb->surface;

         if (!srcObj->pt)
            return;

         srcSurf = screen->get_tex_surface(screen,
                                           srcObj->pt,
                                           srcAtt->CubeMapFace,
                                           srcAtt->TextureLevel,
                                           srcAtt->Zoffset,
                                           PIPE_BUFFER_USAGE_GPU_READ);
         if(!srcSurf)
            return;

         util_blit_pixels(st->blit,
                          srcSurf, srcX0, srcY0, srcX1, srcY1,
                          dstSurf, dstX0, dstY0, dstX1, dstY1,
                          0.0, pFilter);

         pipe_surface_reference(&srcSurf, NULL);
      }
      else {
         struct st_renderbuffer *srcRb =
            st_renderbuffer(readFB->_ColorReadBuffer);
         struct st_renderbuffer *dstRb =
            st_renderbuffer(drawFB->_ColorDrawBuffers[0]);
         struct pipe_surface *srcSurf = srcRb->surface;
         struct pipe_surface *dstSurf = dstRb->surface;

         util_blit_pixels(st->blit,
                          srcSurf, srcX0, srcY0, srcX1, srcY1,
                          dstSurf, dstX0, dstY0, dstX1, dstY1,
                          0.0, pFilter);
      }
   }

   if (mask & depthStencil) {
      /* depth and/or stencil blit */

      /* get src/dst depth surfaces */
      struct st_renderbuffer *srcDepthRb = 
         st_renderbuffer(readFB->Attachment[BUFFER_DEPTH].Renderbuffer);
      struct st_renderbuffer *dstDepthRb = 
         st_renderbuffer(drawFB->Attachment[BUFFER_DEPTH].Renderbuffer);
      struct pipe_surface *srcDepthSurf =
         srcDepthRb ? srcDepthRb->surface : NULL;
      struct pipe_surface *dstDepthSurf =
         dstDepthRb ? dstDepthRb->surface : NULL;

      /* get src/dst stencil surfaces */
      struct st_renderbuffer *srcStencilRb = 
         st_renderbuffer(readFB->Attachment[BUFFER_STENCIL].Renderbuffer);
      struct st_renderbuffer *dstStencilRb = 
         st_renderbuffer(drawFB->Attachment[BUFFER_STENCIL].Renderbuffer);
      struct pipe_surface *srcStencilSurf =
         srcStencilRb ? srcStencilRb->surface : NULL;
      struct pipe_surface *dstStencilSurf =
         dstStencilRb ? dstStencilRb->surface : NULL;

      if ((mask & depthStencil) == depthStencil &&
          srcDepthSurf == srcStencilSurf &&
          dstDepthSurf == dstStencilSurf) {
         /* Blitting depth and stencil values between combined
          * depth/stencil buffers.  This is the ideal case for such buffers.
          */
         util_blit_pixels(st->blit,
                          srcDepthSurf, srcX0, srcY0, srcX1, srcY1,
                          dstDepthSurf, dstX0, dstY0, dstX1, dstY1,
                          0.0, pFilter);
      }
      else {
         /* blitting depth and stencil separately */

         if (mask & GL_DEPTH_BUFFER_BIT) {
            /* blit Z only */
            _mesa_problem(ctx, "st_BlitFramebuffer(DEPTH) not completed");
         }

         if (mask & GL_STENCIL_BUFFER_BIT) {
            /* blit stencil only */
            _mesa_problem(ctx, "st_BlitFramebuffer(STENCIL) not completed");
         }
      }
   }
}
#endif /* FEATURE_EXT_framebuffer_blit */



void
st_init_blit_functions(struct dd_function_table *functions)
{
#if FEATURE_EXT_framebuffer_blit
   functions->BlitFramebuffer = st_BlitFramebuffer;
#endif
}
