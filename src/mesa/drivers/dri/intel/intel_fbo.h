/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#ifndef INTEL_FBO_H
#define INTEL_FBO_H

#include "intel_screen.h"

struct intel_context;

/**
 * Intel framebuffer, derived from gl_framebuffer.
 */
struct intel_framebuffer
{
   struct gl_framebuffer Base;

   struct intel_renderbuffer *color_rb[2];

   /* VBI
    */
   GLuint vbl_waited;

   int64_t swap_ust;
   int64_t swap_missed_ust;

   GLuint swap_count;
   GLuint swap_missed_count;
};


/**
 * Intel renderbuffer, derived from gl_renderbuffer.
 * Note: The PairedDepth and PairedStencil fields use renderbuffer IDs,
 * not pointers because in some circumstances a deleted renderbuffer could
 * result in a dangling pointer here.
 */
struct intel_renderbuffer
{
   struct gl_renderbuffer Base;
   struct intel_region *region;
   GLuint pfPitch;              /* possibly paged flipped pitch */
   GLboolean RenderToTexture;   /* RTT? */

   GLuint PairedDepth;   /**< only used if this is a depth renderbuffer */
   GLuint PairedStencil; /**< only used if this is a stencil renderbuffer */

   GLuint vbl_pending;   /**< vblank sequence number of pending flip */

   uint8_t *span_cache;
   unsigned long span_cache_offset;
};

extern struct intel_renderbuffer *intel_renderbuffer(struct gl_renderbuffer
                                                     *rb);

extern void
intel_renderbuffer_set_region(struct intel_renderbuffer *irb,
			      struct intel_region *region);

extern struct intel_renderbuffer *
intel_create_renderbuffer(GLenum intFormat);

extern void intel_fbo_init(struct intel_context *intel);


/* XXX make inline or macro */
extern struct intel_renderbuffer *intel_get_renderbuffer(struct gl_framebuffer
                                                         *fb,
                                                         int attIndex);

extern void intel_flip_renderbuffers(struct intel_framebuffer *intel_fb);


/* XXX make inline or macro */
extern struct intel_region *intel_get_rb_region(struct gl_framebuffer *fb,
                                                GLuint attIndex);



/**
 * Are we currently rendering into a texture?
 */
static INLINE GLboolean
intel_rendering_to_texture(const GLcontext *ctx)
{
   if (ctx->DrawBuffer->Name) {
      /* User-created FBO */
      const struct intel_renderbuffer *irb =
         intel_renderbuffer(ctx->DrawBuffer->_ColorDrawBuffers[0]);
      return irb && irb->RenderToTexture;
   }
   else {
      return GL_FALSE;
   }
}


#endif /* INTEL_FBO_H */
