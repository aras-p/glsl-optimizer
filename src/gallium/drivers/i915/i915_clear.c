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

/* Authors:
 *    Brian Paul
 */


#include "util/u_clear.h"
#include "util/u_format.h"
#include "util/u_pack_color.h"
#include "i915_context.h"
#include "i915_screen.h"
#include "i915_reg.h"
#include "i915_batch.h"
#include "i915_resource.h"
#include "i915_state.h"

void
i915_clear_emit(struct pipe_context *pipe, unsigned buffers, const float *rgba,
                double depth, unsigned stencil,
                unsigned destx, unsigned desty, unsigned width, unsigned height)
{
   struct i915_context *i915 = i915_context(pipe);
   uint32_t clear_params, clear_color, clear_depth, clear_stencil,
            clear_color8888, packed_z_stencil;
   union util_color u_color;
   float f_depth = depth;
   struct i915_texture *cbuf_tex, *depth_tex;

   cbuf_tex = depth_tex = NULL;
   clear_params = 0;

   if (buffers & PIPE_CLEAR_COLOR) {
      struct pipe_surface *cbuf = i915->framebuffer.cbufs[0];

      clear_params |= CLEARPARAM_WRITE_COLOR;
      cbuf_tex = i915_texture(cbuf->texture);
      util_pack_color(rgba, cbuf->format, &u_color);
      if (util_format_get_blocksize(cbuf_tex->b.b.format) == 4)
         clear_color = u_color.ui;
      else
         clear_color = (u_color.ui & 0xffff) | (u_color.ui << 16);

      util_pack_color(rgba, cbuf->format, &u_color);
      clear_color8888 = u_color.ui;
   } else
      clear_color = clear_color8888 = 0;

   clear_depth = clear_stencil = 0;
   if (buffers & PIPE_CLEAR_DEPTH) {
      struct pipe_surface *zbuf = i915->framebuffer.zsbuf;

      clear_params |= CLEARPARAM_WRITE_DEPTH;
      depth_tex = i915_texture(zbuf->texture);
      packed_z_stencil = util_pack_z_stencil(depth_tex->b.b.format, depth, stencil);

      if (util_format_get_blocksize(depth_tex->b.b.format) == 4) {
         /* Avoid read-modify-write if there's no stencil. */
         if (buffers & PIPE_CLEAR_STENCIL
               || depth_tex->b.b.format != PIPE_FORMAT_Z24_UNORM_S8_USCALED) {
            clear_params |= CLEARPARAM_WRITE_STENCIL;
            clear_stencil = packed_z_stencil & 0xff;
            clear_depth = packed_z_stencil;
         } else
            clear_depth = packed_z_stencil & 0xffffff00;
      } else {
         clear_depth = (clear_depth & 0xffff) | (clear_depth << 16);
      }
   }

   if (i915->hardware_dirty)
      i915_emit_hardware_state(i915);

   if (!BEGIN_BATCH(7 + 7)) {
      FLUSH_BATCH(NULL);

      i915_emit_hardware_state(i915);
      i915->vbo_flushed = 1;

      assert(BEGIN_BATCH(7 + 7));
   }

   OUT_BATCH(_3DSTATE_CLEAR_PARAMETERS);
   OUT_BATCH(clear_params | CLEARPARAM_CLEAR_RECT);
   OUT_BATCH(clear_color);
   OUT_BATCH(clear_depth);
   OUT_BATCH(clear_color8888);
   OUT_BATCH_F(f_depth);
   OUT_BATCH(clear_stencil);

   OUT_BATCH(_3DPRIMITIVE | PRIM3D_CLEAR_RECT | 5);
   OUT_BATCH_F(destx + width);
   OUT_BATCH_F(desty + height);
   OUT_BATCH_F(destx);
   OUT_BATCH_F(desty + height);
   OUT_BATCH_F(destx);
   OUT_BATCH_F(desty);

   /* Flush after clear, its expected to be a costly operation.
    * This is not required, just a heuristic
    */
   FLUSH_BATCH(NULL);
}

/**
 * Clear the given buffers to the specified values.
 * No masking, no scissor (clear entire buffer).
 */
void
i915_clear_blitter(struct pipe_context *pipe, unsigned buffers, const float *rgba,
                   double depth, unsigned stencil)
{
   util_clear(pipe, &i915_context(pipe)->framebuffer, buffers, rgba, depth,
              stencil);
}

void
i915_clear_render(struct pipe_context *pipe, unsigned buffers, const float *rgba,
                  double depth, unsigned stencil)
{
   struct i915_context *i915 = i915_context(pipe);

   if (i915->dirty)
      i915_update_derived(i915);

   i915_clear_emit(pipe, buffers, rgba, depth, stencil,
                   0, 0, i915->framebuffer.width, i915->framebuffer.height);
}
