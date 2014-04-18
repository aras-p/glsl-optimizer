/*
 * Copyright © 2014 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include "brw_context.h"
#include "intel_batchbuffer.h"
#include "intel_fbo.h"

#include "main/blit.h"
#include "main/buffers.h"
#include "main/enums.h"
#include "main/fbobject.h"

#include "drivers/common/meta.h"

/**
 * @file brw_meta_updownsample.c
 *
 * Implements upsampling and downsampling of miptrees for window system
 * framebuffers.
 */

/**
 * Creates a new named renderbuffer that wraps the first slice
 * of an existing miptree.
 *
 * Clobbers the current renderbuffer binding (ctx->CurrentRenderbuffer).
 */
GLuint
brw_get_rb_for_slice(struct brw_context *brw,
                     struct intel_mipmap_tree *mt,
                     unsigned level, unsigned layer, bool flat)
{
   struct gl_context *ctx = &brw->ctx;
   GLuint rbo;
   struct gl_renderbuffer *rb;
   struct intel_renderbuffer *irb;

   /* This turns the GenRenderbuffers name into an actual struct
    * intel_renderbuffer.
    */
   _mesa_GenRenderbuffers(1, &rbo);
   _mesa_BindRenderbuffer(GL_RENDERBUFFER, rbo);

   rb = ctx->CurrentRenderbuffer;
   irb = intel_renderbuffer(rb);

   rb->Format = mt->format;
   rb->_BaseFormat = _mesa_get_format_base_format(mt->format);

   /* Program takes care of msaa and mip-level access manually for stencil.
    * The surface is also treated as Y-tiled instead of as W-tiled calling for
    * twice the width and half the height in dimensions.
    */
   if (flat) {
      const unsigned halign_stencil = 8;

      rb->NumSamples = 0;
      rb->Width = ALIGN(mt->total_width, halign_stencil) * 2;
      rb->Height = (mt->total_height / mt->physical_depth0) / 2;
      irb->mt_level = 0;
   } else {
      rb->NumSamples = mt->num_samples;
      rb->Width = mt->logical_width0;
      rb->Height = mt->logical_height0;
      irb->mt_level = level;
   }

   irb->mt_layer = layer;

   intel_miptree_reference(&irb->mt, mt);

   return rbo;
}

/**
 * Implementation of up or downsampling for window-system MSAA miptrees.
 */
void
brw_meta_updownsample(struct brw_context *brw,
                      struct intel_mipmap_tree *src_mt,
                      struct intel_mipmap_tree *dst_mt)
{
   struct gl_context *ctx = &brw->ctx;
   GLuint fbos[2], src_rbo, dst_rbo, src_fbo, dst_fbo;
   GLenum drawbuffer;
   GLbitfield attachment, blit_bit;

   if (_mesa_get_format_base_format(src_mt->format) == GL_DEPTH_COMPONENT ||
       _mesa_get_format_base_format(src_mt->format) == GL_DEPTH_STENCIL) {
      attachment = GL_DEPTH_ATTACHMENT;
      drawbuffer = GL_NONE;
      blit_bit = GL_DEPTH_BUFFER_BIT;
   } else {
      attachment = GL_COLOR_ATTACHMENT0;
      drawbuffer = GL_COLOR_ATTACHMENT0;
      blit_bit = GL_COLOR_BUFFER_BIT;
   }

   intel_batchbuffer_emit_mi_flush(brw);

   _mesa_meta_begin(ctx, MESA_META_ALL);
   _mesa_GenFramebuffers(2, fbos);
   src_rbo = brw_get_rb_for_slice(brw, src_mt, 0, 0, false);
   dst_rbo = brw_get_rb_for_slice(brw, dst_mt, 0, 0, false);
   src_fbo = fbos[0];
   dst_fbo = fbos[1];

   _mesa_BindFramebuffer(GL_READ_FRAMEBUFFER, src_fbo);
   _mesa_FramebufferRenderbuffer(GL_READ_FRAMEBUFFER, attachment,
                                 GL_RENDERBUFFER, src_rbo);
   _mesa_ReadBuffer(drawbuffer);

   _mesa_BindFramebuffer(GL_DRAW_FRAMEBUFFER, dst_fbo);
   _mesa_FramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, attachment,
                                 GL_RENDERBUFFER, dst_rbo);
   _mesa_DrawBuffer(drawbuffer);

   _mesa_BlitFramebuffer(0, 0,
                         src_mt->logical_width0, src_mt->logical_height0,
                         0, 0,
                         dst_mt->logical_width0, dst_mt->logical_height0,
                         blit_bit, GL_NEAREST);

   _mesa_DeleteRenderbuffers(1, &src_rbo);
   _mesa_DeleteRenderbuffers(1, &dst_rbo);
   _mesa_DeleteFramebuffers(2, fbos);

   _mesa_meta_end(ctx);

   intel_batchbuffer_emit_mi_flush(brw);
}
