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

#include "brw_meta_util.h"
#include "main/fbobject.h"

/**
 * Helper function for handling mirror image blits.
 *
 * If coord0 > coord1, swap them and invert the "mirror" boolean.
 */
static inline void
fixup_mirroring(bool *mirror, float *coord0, float *coord1)
{
   if (*coord0 > *coord1) {
      *mirror = !*mirror;
      float tmp = *coord0;
      *coord0 = *coord1;
      *coord1 = tmp;
   }
}

/**
 * Adjust {src,dst}_x{0,1} to account for clipping and scissoring of
 * destination coordinates.
 *
 * Return true if there is still blitting to do, false if all pixels got
 * rejected by the clip and/or scissor.
 *
 * For clarity, the nomenclature of this function assumes we are clipping and
 * scissoring the X coordinate; the exact same logic applies for Y
 * coordinates.
 *
 * Note: this function may also be used to account for clipping of source
 * coordinates, by swapping the roles of src and dst.
 */
static inline bool
clip_or_scissor(bool mirror,
                GLfloat *src_x0, GLfloat *src_x1,
                GLfloat *dst_x0, GLfloat *dst_x1,
                GLfloat fb_xmin, GLfloat fb_xmax)
{
   float scale = (float) (*src_x1 - *src_x0) / (*dst_x1 - *dst_x0);
   /* If we are going to scissor everything away, stop. */
   if (!(fb_xmin < fb_xmax &&
         *dst_x0 < fb_xmax &&
         fb_xmin < *dst_x1 &&
         *dst_x0 < *dst_x1)) {
      return false;
   }

   /* Clip the destination rectangle, and keep track of how many pixels we
    * clipped off of the left and right sides of it.
    */
   int pixels_clipped_left = 0;
   int pixels_clipped_right = 0;
   if (*dst_x0 < fb_xmin) {
      pixels_clipped_left = fb_xmin - *dst_x0;
      *dst_x0 = fb_xmin;
   }
   if (fb_xmax < *dst_x1) {
      pixels_clipped_right = *dst_x1 - fb_xmax;
      *dst_x1 = fb_xmax;
   }

   /* If we are mirrored, then before applying pixels_clipped_{left,right} to
    * the source coordinates, we need to flip them to account for the
    * mirroring.
    */
   if (mirror) {
      int tmp = pixels_clipped_left;
      pixels_clipped_left = pixels_clipped_right;
      pixels_clipped_right = tmp;
   }

   /* Adjust the source rectangle to remove the pixels corresponding to those
    * that were clipped/scissored out of the destination rectangle.
    */
   *src_x0 += pixels_clipped_left * scale;
   *src_x1 -= pixels_clipped_right * scale;

   return true;
}

bool
brw_meta_mirror_clip_and_scissor(const struct gl_context *ctx,
                                 GLfloat *srcX0, GLfloat *srcY0,
                                 GLfloat *srcX1, GLfloat *srcY1,
                                 GLfloat *dstX0, GLfloat *dstY0,
                                 GLfloat *dstX1, GLfloat *dstY1,
                                 bool *mirror_x, bool *mirror_y)
{
   const struct gl_framebuffer *read_fb = ctx->ReadBuffer;
   const struct gl_framebuffer *draw_fb = ctx->DrawBuffer;

   *mirror_x = false;
   *mirror_y = false;

   /* Detect if the blit needs to be mirrored */
   fixup_mirroring(mirror_x, srcX0, srcX1);
   fixup_mirroring(mirror_x, dstX0, dstX1);
   fixup_mirroring(mirror_y, srcY0, srcY1);
   fixup_mirroring(mirror_y, dstY0, dstY1);

   /* If the destination rectangle needs to be clipped or scissored, do so. */
   if (!(clip_or_scissor(*mirror_x, srcX0, srcX1, dstX0, dstX1,
                         draw_fb->_Xmin, draw_fb->_Xmax) &&
         clip_or_scissor(*mirror_y, srcY0, srcY1, dstY0, dstY1,
                         draw_fb->_Ymin, draw_fb->_Ymax))) {
      /* Everything got clipped/scissored away, so the blit was successful. */
      return true;
   }

   /* If the source rectangle needs to be clipped or scissored, do so. */
   if (!(clip_or_scissor(*mirror_x, dstX0, dstX1, srcX0, srcX1,
                         0, read_fb->Width) &&
         clip_or_scissor(*mirror_y, dstY0, dstY1, srcY0, srcY1,
                         0, read_fb->Height))) {
      /* Everything got clipped/scissored away, so the blit was successful. */
      return true;
   }

   /* Account for the fact that in the system framebuffer, the origin is at
    * the lower left.
    */
   if (_mesa_is_winsys_fbo(read_fb)) {
      GLint tmp = read_fb->Height - *srcY0;
      *srcY0 = read_fb->Height - *srcY1;
      *srcY1 = tmp;
      *mirror_y = !*mirror_y;
   }
   if (_mesa_is_winsys_fbo(draw_fb)) {
      GLint tmp = draw_fb->Height - *dstY0;
      *dstY0 = draw_fb->Height - *dstY1;
      *dstY1 = tmp;
      *mirror_y = !*mirror_y;
   }

   return false;
}
