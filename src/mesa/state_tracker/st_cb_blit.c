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
#include "main/mfeatures.h"

#include "st_context.h"
#include "st_texture.h"
#include "st_cb_blit.h"
#include "st_cb_fbo.h"
#include "st_atom.h"

#include "util/u_format.h"


static void
st_BlitFramebuffer(struct gl_context *ctx,
                   GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                   GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                   GLbitfield mask, GLenum filter)
{
   const GLbitfield depthStencil = (GL_DEPTH_BUFFER_BIT |
                                    GL_STENCIL_BUFFER_BIT);
   struct st_context *st = st_context(ctx);
   const uint pFilter = ((filter == GL_NEAREST)
                         ? PIPE_TEX_FILTER_NEAREST
                         : PIPE_TEX_FILTER_LINEAR);
   struct gl_framebuffer *readFB = ctx->ReadBuffer;
   struct gl_framebuffer *drawFB = ctx->DrawBuffer;
   struct {
      GLint srcX0, srcY0, srcX1, srcY1;
      GLint dstX0, dstY0, dstX1, dstY1;
   } clip;
   struct pipe_blit_info blit;

   st_validate_state(st);

   clip.srcX0 = srcX0;
   clip.srcY0 = srcY0;
   clip.srcX1 = srcX1;
   clip.srcY1 = srcY1;
   clip.dstX0 = dstX0;
   clip.dstY0 = dstY0;
   clip.dstX1 = dstX1;
   clip.dstY1 = dstY1;

   /* NOTE: If the src and dst dimensions don't match, we cannot simply adjust
    * the integer coordinates to account for clipping (or scissors) because that
    * would make us cut off fractional parts, affecting the result of the blit.
    *
    * XXX: This should depend on mask !
    */
   if (!_mesa_clip_blit(ctx,
                        &clip.srcX0, &clip.srcY0, &clip.srcX1, &clip.srcY1,
                        &clip.dstX0, &clip.dstY0, &clip.dstX1, &clip.dstY1)) {
      return; /* nothing to draw/blit */
   }
   blit.scissor_enable =
      (dstX0 != clip.dstX0) ||
      (dstY0 != clip.dstY0) ||
      (dstX1 != clip.dstX1) ||
      (dstY1 != clip.dstY1);

   if (st_fb_orientation(drawFB) == Y_0_TOP) {
      /* invert Y for dest */
      dstY0 = drawFB->Height - dstY0;
      dstY1 = drawFB->Height - dstY1;
      /* invert Y for clip */
      clip.dstY0 = drawFB->Height - clip.dstY0;
      clip.dstY1 = drawFB->Height - clip.dstY1;
   }
   if (blit.scissor_enable) {
      blit.scissor.minx = MIN2(clip.dstX0, clip.dstX1);
      blit.scissor.miny = MIN2(clip.dstY0, clip.dstY1);
      blit.scissor.maxx = MAX2(clip.dstX0, clip.dstX1);
      blit.scissor.maxy = MAX2(clip.dstY0, clip.dstY1);
#if 0
      debug_printf("scissor = (%i,%i)-(%i,%i)\n",
                   blit.scissor.minx,blit.scissor.miny,
                   blit.scissor.maxx,blit.scissor.maxy);
#endif
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

   blit.src.box.depth = 1;
   blit.dst.box.depth = 1;

   /* Destination dimensions have to be positive: */
   if (dstX0 < dstX1) {
      blit.dst.box.x = dstX0;
      blit.src.box.x = srcX0;
      blit.dst.box.width = dstX1 - dstX0;
      blit.src.box.width = srcX1 - srcX0;
   } else {
      blit.dst.box.x = dstX1;
      blit.src.box.x = srcX1;
      blit.dst.box.width = dstX0 - dstX1;
      blit.src.box.width = srcX0 - srcX1;
   }
   if (dstY0 < dstY1) {
      blit.dst.box.y = dstY0;
      blit.src.box.y = srcY0;
      blit.dst.box.height = dstY1 - dstY0;
      blit.src.box.height = srcY1 - srcY0;
   } else {
      blit.dst.box.y = dstY1;
      blit.src.box.y = srcY1;
      blit.dst.box.height = dstY0 - dstY1;
      blit.src.box.height = srcY0 - srcY1;
   }

   blit.filter = pFilter;

   if (mask & GL_COLOR_BUFFER_BIT) {
      struct gl_renderbuffer_attachment *srcAtt =
         &readFB->Attachment[readFB->_ColorReadBufferIndex];

      blit.mask = PIPE_MASK_RGBA;

      if (srcAtt->Type == GL_TEXTURE) {
         struct st_texture_object *srcObj = st_texture_object(srcAtt->Texture);
         GLuint i;

         if (!srcObj || !srcObj->pt) {
            return;
         }

         for (i = 0; i < drawFB->_NumColorDrawBuffers; i++) {
            struct st_renderbuffer *dstRb =
               st_renderbuffer(drawFB->_ColorDrawBuffers[i]);

            if (dstRb) {
               struct pipe_surface *dstSurf = dstRb->surface;

               if (dstSurf) {
                  blit.dst.resource = dstSurf->texture;
                  blit.dst.level = dstSurf->u.tex.level;
                  blit.dst.box.z = dstSurf->u.tex.first_layer;
                  blit.dst.format = util_format_linear(dstSurf->format);

                  blit.src.resource = srcObj->pt;
                  blit.src.level = srcAtt->TextureLevel;
                  blit.src.box.z = srcAtt->Zoffset + srcAtt->CubeMapFace;
                  blit.src.format = util_format_linear(srcObj->pt->format);

                  st->pipe->blit(st->pipe, &blit);
               }
            }
         }
      }
      else {
         struct st_renderbuffer *srcRb =
            st_renderbuffer(readFB->_ColorReadBuffer);
         struct pipe_surface *srcSurf;
         GLuint i;

         if (!srcRb || !srcRb->surface) {
            return;
         }

         srcSurf = srcRb->surface;

         for (i = 0; i < drawFB->_NumColorDrawBuffers; i++) {
            struct st_renderbuffer *dstRb =
               st_renderbuffer(drawFB->_ColorDrawBuffers[i]);

            if (dstRb) {
               struct pipe_surface *dstSurf = dstRb->surface;

               if (dstSurf) {
                  blit.dst.resource = dstSurf->texture;
                  blit.dst.level = dstSurf->u.tex.level;
                  blit.dst.box.z = dstSurf->u.tex.first_layer;
                  blit.dst.format = util_format_linear(dstSurf->format);

                  blit.src.resource = srcSurf->texture;
                  blit.src.level = srcSurf->u.tex.level;
                  blit.src.box.z = srcSurf->u.tex.first_layer;
                  blit.src.format = util_format_linear(srcSurf->format);

                  st->pipe->blit(st->pipe, &blit);
               }
            }
         }
      }
   }

   if (mask & depthStencil) {
      /* depth and/or stencil blit */

      /* get src/dst depth surfaces */
      struct st_renderbuffer *srcDepthRb =
         st_renderbuffer(readFB->Attachment[BUFFER_DEPTH].Renderbuffer);
      struct st_renderbuffer *dstDepthRb = 
         st_renderbuffer(drawFB->Attachment[BUFFER_DEPTH].Renderbuffer);
      struct pipe_surface *dstDepthSurf =
         dstDepthRb ? dstDepthRb->surface : NULL;

      struct st_renderbuffer *srcStencilRb =
         st_renderbuffer(readFB->Attachment[BUFFER_STENCIL].Renderbuffer);
      struct st_renderbuffer *dstStencilRb =
         st_renderbuffer(drawFB->Attachment[BUFFER_STENCIL].Renderbuffer);
      struct pipe_surface *dstStencilSurf =
         dstStencilRb ? dstStencilRb->surface : NULL;

      if (_mesa_has_depthstencil_combined(readFB) &&
          _mesa_has_depthstencil_combined(drawFB)) {
         blit.mask = 0;
         if (mask & GL_DEPTH_BUFFER_BIT)
            blit.mask |= PIPE_MASK_Z;
         if (mask & GL_STENCIL_BUFFER_BIT)
            blit.mask |= PIPE_MASK_S;

         blit.dst.resource = dstDepthSurf->texture;
         blit.dst.level = dstDepthSurf->u.tex.level;
         blit.dst.box.z = dstDepthSurf->u.tex.first_layer;
         blit.dst.format = dstDepthSurf->format;

         blit.src.resource = srcDepthRb->texture;
         blit.src.level = srcDepthRb->surface->u.tex.level;
         blit.src.box.z = srcDepthRb->surface->u.tex.first_layer;
         blit.src.format = srcDepthRb->surface->format;

         st->pipe->blit(st->pipe, &blit);
      }
      else {
         /* blitting depth and stencil separately */

         if (mask & GL_DEPTH_BUFFER_BIT) {
            blit.mask = PIPE_MASK_Z;

            blit.dst.resource = dstDepthSurf->texture;
            blit.dst.level = dstDepthSurf->u.tex.level;
            blit.dst.box.z = dstDepthSurf->u.tex.first_layer;
            blit.dst.format = dstDepthSurf->format;

            blit.src.resource = srcDepthRb->texture;
            blit.src.level = srcDepthRb->surface->u.tex.level;
            blit.src.box.z = srcDepthRb->surface->u.tex.first_layer;
            blit.src.format = srcDepthRb->surface->format;

            st->pipe->blit(st->pipe, &blit);
         }

         if (mask & GL_STENCIL_BUFFER_BIT) {
            blit.mask = PIPE_MASK_S;

            blit.dst.resource = dstStencilSurf->texture;
            blit.dst.level = dstStencilSurf->u.tex.level;
            blit.dst.box.z = dstStencilSurf->u.tex.first_layer;
            blit.dst.format = dstStencilSurf->format;

            blit.src.resource = srcStencilRb->texture;
            blit.src.level = srcStencilRb->surface->u.tex.level;
            blit.src.box.z = srcStencilRb->surface->u.tex.first_layer;
            blit.src.format = srcStencilRb->surface->format;

            st->pipe->blit(st->pipe, &blit);
         }
      }
   }
}


void
st_init_blit_functions(struct dd_function_table *functions)
{
   functions->BlitFramebuffer = st_BlitFramebuffer;
}
