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

#ifndef INTEL_BUFFERS_H
#define INTEL_BUFFERS_H


struct intel_context;
struct intel_framebuffer;

/**
 * Intel framebuffer, derived from gl_framebuffer.
 */
struct intel_framebuffer
{
   struct gl_framebuffer Base;

   /* Drawable page flipping state */
   GLboolean pf_active;
   GLuint pf_seq;
   GLint pf_planes;
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


void intelCopyBuffer(__DRIdrawablePrivate * dPriv,
		     const drm_clip_rect_t * rect);

extern void intel_wait_flips(struct intel_context *intel, GLuint batch_flags);

extern void intelSwapBuffers(__DRIdrawablePrivate * dPriv);

extern void intelWindowMoved(struct intel_context *intel);

#endif /* INTEL_BUFFERS_H */
