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


#include "pipe/p_state.h"
#include "pipe/softpipe/sp_surface.h"


struct intel_context;
struct pipe_region;

#if 0
/**
 * Intel framebuffer, derived from gl_framebuffer.
 */
struct intel_framebuffer
{
   struct gl_framebuffer Base;

   /* Drawable page flipping state */
   GLboolean pf_active;
   GLuint pf_seq;
   GLint pf_pipes;
   GLint pf_current_page;
   GLint pf_num_pages;

   /* VBI
    */
   GLuint vbl_seq;
   GLuint vblank_flags;
   GLuint vbl_waited;

   int64_t swap_ust;
   int64_t swap_missed_ust;

   GLuint swap_count;
   GLuint swap_missed_count;

   GLuint vbl_pending[3];  /**< [number of color buffers] */
};
#endif


/**
 * Intel renderbuffer, derived from gl_renderbuffer.
 * Note: The PairedDepth and PairedStencil fields use renderbuffer IDs,
 * not pointers because in some circumstances a deleted renderbuffer could
 * result in a dangling pointer here.
 */
#if 0
struct intel_renderbuffer
{
   struct gl_renderbuffer Base;
};
#endif

#if 0
extern struct intel_renderbuffer *intel_new_renderbuffer_fb(GLuint intFormat);
#endif

extern void intel_fbo_init(struct intel_context *intel);


extern struct pipe_surface *
intel_new_surface(struct pipe_context *pipe, GLuint pipeFormat);


extern const GLuint *
intel_supported_formats(struct pipe_context *pipe, GLuint *numFormats);


#endif /* INTEL_FBO_H */
