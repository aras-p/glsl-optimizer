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
#include "st_public.h"
#include "st_context.h"


struct st_framebuffer *st_create_framebuffer( const __GLcontextModes *visual )
{
   struct st_framebuffer *stfb
      = CALLOC_STRUCT(st_framebuffer);
   if (stfb) {

#if 0
      /* from intel driver... */

      GLboolean swStencil = (mesaVis->stencilBits > 0 &&
                             mesaVis->depthBits != 24);
      GLenum rgbFormat = (mesaVis->redBits == 5 ? GL_RGB5 : GL_RGBA8);

      struct intel_framebuffer *intel_fb = CALLOC_STRUCT(intel_framebuffer);
      if (!intel_fb)
	 return GL_FALSE;

      _mesa_initialize_framebuffer(&intel_fb->Base, mesaVis);

      /*
       * XXX Create pipe_surfaces for front/back buffers,
       * hand them to state tracker to create a framebuffer object.
       */

      {
	 /* fake frontbuffer */
	 /* XXX allocation should only happen in the unusual case
            it's actually needed */
         struct gl_renderbuffer *rb = st_new_renderbuffer_fb(rgbFormat);
         _mesa_add_renderbuffer(&intel_fb->Base, BUFFER_FRONT_LEFT, rb);
      }

      if (mesaVis->doubleBufferMode) {
         struct gl_renderbuffer *rb = st_new_renderbuffer_fb(rgbFormat);
         _mesa_add_renderbuffer(&intel_fb->Base, BUFFER_BACK_LEFT, rb);
      }

      if (mesaVis->depthBits == 24 && mesaVis->stencilBits == 8) {
         /* combined depth/stencil buffer */
         struct gl_renderbuffer *depthStencilRb
            = st_new_renderbuffer_fb(GL_DEPTH24_STENCIL8_EXT);
         /* note: bind RB to two attachment points */
         _mesa_add_renderbuffer(&intel_fb->Base, BUFFER_DEPTH, depthStencilRb);
         _mesa_add_renderbuffer(&intel_fb->Base, BUFFER_STENCIL,depthStencilRb);
      }
      else if (mesaVis->depthBits == 16) {
         /* just 16-bit depth buffer, no hw stencil */
         struct gl_renderbuffer *depthRb
            = st_new_renderbuffer_fb(GL_DEPTH_COMPONENT16);
         _mesa_add_renderbuffer(&intel_fb->Base, BUFFER_DEPTH, depthRb);
      }


      /* now add any/all software-based renderbuffers we may need */
      _mesa_add_soft_renderbuffers(&intel_fb->Base,
                                   GL_FALSE, /* never sw color */
                                   GL_FALSE, /* never sw depth */
                                   swStencil, mesaVis->accumRedBits > 0,
                                   GL_FALSE, /* never sw alpha */
                                   GL_FALSE  /* never sw aux */ );
      driDrawPriv->driverPrivate = (void *) intel_fb;

#endif

   }
   return stfb;
}


