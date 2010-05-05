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

#include "main/formats.h"
#include "intel_screen.h"

struct intel_context;

/**
 * Intel renderbuffer, derived from gl_renderbuffer.
 */
struct intel_renderbuffer
{
   struct gl_renderbuffer Base;
   struct intel_region *region;
};


/**
 * gl_renderbuffer is a base class which we subclass.  The Class field
 * is used for simple run-time type checking.
 */
#define INTEL_RB_CLASS 0x12345678


/**
 * Return a gl_renderbuffer ptr casted to intel_renderbuffer.
 * NULL will be returned if the rb isn't really an intel_renderbuffer.
 * This is determined by checking the ClassID.
 */
static INLINE struct intel_renderbuffer *
intel_renderbuffer(struct gl_renderbuffer *rb)
{
   struct intel_renderbuffer *irb = (struct intel_renderbuffer *) rb;
   if (irb && irb->Base.ClassID == INTEL_RB_CLASS) {
      /*_mesa_warning(NULL, "Returning non-intel Rb\n");*/
      return irb;
   }
   else
      return NULL;
}


/**
 * Return a framebuffer's renderbuffer, named by a BUFFER_x index.
 */
static INLINE struct intel_renderbuffer *
intel_get_renderbuffer(struct gl_framebuffer *fb, int attIndex)
{
   if (attIndex >= 0)
      return intel_renderbuffer(fb->Attachment[attIndex].Renderbuffer);
   else
      return NULL;
}


extern void
intel_renderbuffer_set_region(struct intel_context *intel,
			      struct intel_renderbuffer *irb,
			      struct intel_region *region);


extern struct intel_renderbuffer *
intel_create_renderbuffer(gl_format format);


extern void
intel_fbo_init(struct intel_context *intel);


extern void
intel_flip_renderbuffers(struct gl_framebuffer *fb);


static INLINE struct intel_region *
intel_get_rb_region(struct gl_framebuffer *fb, GLuint attIndex)
{
   struct intel_renderbuffer *irb = intel_get_renderbuffer(fb, attIndex);
   if (irb)
      return irb->region;
   else
      return NULL;
}


#endif /* INTEL_FBO_H */
