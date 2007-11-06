/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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
#include "main/context.h"
#include "main/framebuffer.h"
#include "main/renderbuffer.h"
#include "st_public.h"
#include "st_context.h"
#include "st_cb_fbo.h"


struct st_framebuffer *st_create_framebuffer( const __GLcontextModes *visual,
                                              boolean createRenderbuffers,
                                              void *private)
{
   struct st_framebuffer *stfb
      = CALLOC_STRUCT(st_framebuffer);
   if (stfb) {
      GLboolean swStencil = (visual->stencilBits > 0 &&
	                     visual->stencilBits != 8 &&
                             visual->depthBits != 24);
      GLenum rgbFormat = (visual->redBits == 5 ? GL_RGB5 : GL_RGBA8);

      _mesa_initialize_framebuffer(&stfb->Base, visual);

      if (createRenderbuffers) {
         {
            /* fake frontbuffer */
            /* XXX allocation should only happen in the unusual case
               it's actually needed */
            struct gl_renderbuffer *rb = st_new_renderbuffer_fb(rgbFormat);
            _mesa_add_renderbuffer(&stfb->Base, BUFFER_FRONT_LEFT, rb);
         }

         if (visual->doubleBufferMode) {
            struct gl_renderbuffer *rb = st_new_renderbuffer_fb(rgbFormat);
            _mesa_add_renderbuffer(&stfb->Base, BUFFER_BACK_LEFT, rb);
         }

         if (visual->depthBits == 24) {
            /* combined depth/stencil buffer */
            struct gl_renderbuffer *depthStencilRb
               = st_new_renderbuffer_fb(GL_DEPTH24_STENCIL8_EXT);
            /* note: bind RB to two attachment points */
            _mesa_add_renderbuffer(&stfb->Base, BUFFER_DEPTH, depthStencilRb);
            if(visual->stencilBits == 8)
               _mesa_add_renderbuffer(&stfb->Base, BUFFER_STENCIL,depthStencilRb);
         }
         else if (visual->depthBits == 16) {
            /* just 16-bit depth buffer, no hw stencil */
            struct gl_renderbuffer *depthRb
               = st_new_renderbuffer_fb(GL_DEPTH_COMPONENT16);
            _mesa_add_renderbuffer(&stfb->Base, BUFFER_DEPTH, depthRb);
         }
         else
            assert(0);

         /* now add any/all software-based renderbuffers we may need */
         _mesa_add_soft_renderbuffers(&stfb->Base,
                                      GL_FALSE, /* never sw color */
                                      GL_FALSE, /* never sw depth */
                                      swStencil, visual->accumRedBits > 0,
                                      GL_FALSE, /* never sw alpha */
                                      GL_FALSE  /* never sw aux */ );
      }

      stfb->Base.Initialized = GL_TRUE;

      stfb->Private = private;
   }
   return stfb;
}


void st_resize_framebuffer( struct st_framebuffer *stfb,
                            uint width, uint height )
{
   if (stfb->Base.Width != width || stfb->Base.Height != height) {
      GET_CURRENT_CONTEXT(ctx);
      if (ctx) {
         _mesa_resize_framebuffer(ctx, &stfb->Base, width, height);

         assert(stfb->Base.Width == width);
         assert(stfb->Base.Height == height);
      }
   }
}


void st_unreference_framebuffer( struct st_framebuffer **stfb )
{
   _mesa_unreference_framebuffer((struct gl_framebuffer **) stfb);
}



/**
 * Return the pipe_surface for the given renderbuffer.
 */
struct pipe_surface *
st_get_framebuffer_surface(struct st_framebuffer *stfb, uint surfIndex)
{
   struct st_renderbuffer *strb;

   assert(surfIndex <= ST_SURFACE_BACK_RIGHT);

   /* sanity checks, ST tokens should match Mesa tokens */
   assert(ST_SURFACE_FRONT_LEFT == BUFFER_FRONT_LEFT);
   assert(ST_SURFACE_BACK_RIGHT == BUFFER_BACK_RIGHT);

   strb = st_renderbuffer(stfb->Base.Attachment[surfIndex].Renderbuffer);
   if (strb)
      return strb->surface;
   return NULL;
}


/**
 * This function is to be called prior to SwapBuffers on the given
 * framebuffer.  It checks if the current context is bound to the framebuffer
 * and flushes rendering if needed.
 */
void
st_notify_swapbuffers(struct st_framebuffer *stfb)
{
   GET_CURRENT_CONTEXT(ctx);

   if (ctx && ctx->DrawBuffer == &stfb->Base) {
      st_flush(ctx->st);
   }
}


void *st_framebuffer_private( struct st_framebuffer *stfb )
{
   return stfb->Private;
}

