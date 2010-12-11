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

#include "util/u_pack_color.h"
#include "util/u_math.h"

#include "pipe/p_state.h"

#include "brw_batchbuffer.h"
#include "brw_screen.h"
#include "brw_context.h"

#define MASK16 0xffff
#define MASK24 0xffffff


/**
 * Use blitting to clear the renderbuffers named by 'flags'.
 * Note: we can't use the ctx->DrawBuffer->_ColorDrawBufferIndexes field
 * since that might include software renderbuffers or renderbuffers
 * which we're clearing with triangles.
 */
static enum pipe_error
try_clear( struct brw_context *brw,
           struct brw_surface *surface,
           unsigned value,
           unsigned rgba_mask)
{
   uint32_t BR13, CMD;
   int x1 = 0;
   int y1 = 0;
   int x2 = surface->base.width;
   int y2 = surface->base.height;
   int pitch = surface->pitch;
   int cpp = surface->cpp;

   if (x2 == 0 || y2 == 0)
      return 0;

   debug_printf("%s dst:buf(%p)/%d+%d %d,%d sz:%dx%d\n",
                __FUNCTION__,
                (void *)surface->bo, pitch * cpp,
                surface->offset,
                x1, y1, x2 - x1, y2 - y1);

   BR13 = 0xf0 << 16;
   CMD = XY_COLOR_BLT_CMD | rgba_mask;

   /* Setup the blit command */
   if (cpp == 4) {
      BR13 |= BR13_8888;
   }
   else {
      assert(cpp == 2);
      BR13 |= BR13_565;
   }

   /* XXX: nasty hack for clearing depth buffers
    */
   if (surface->tiling == BRW_TILING_Y) {
      x2 = pitch;
   }

   if (surface->tiling == BRW_TILING_X) {
      CMD |= XY_DST_TILED;
      pitch /= 4;
   }

   BR13 |= (pitch * cpp);

   BEGIN_BATCH(6, 0);
   OUT_BATCH(CMD);
   OUT_BATCH(BR13);
   OUT_BATCH((y1 << 16) | x1);
   OUT_BATCH((y2 << 16) | x2);
   OUT_RELOC(surface->bo,
             BRW_USAGE_BLIT_DEST,
             surface->offset);
   OUT_BATCH(value);
   ADVANCE_BATCH();

   return 0;
}




static void color_clear(struct brw_context *brw, 
                        struct brw_surface *bsurface,
                        const float *rgba )
{
   enum pipe_error ret;
   union util_color value;

   util_pack_color( rgba, bsurface->base.format, &value );

   if (bsurface->cpp == 2)
      value.ui |= value.ui << 16;

   ret = try_clear( brw, bsurface, value.ui,
                    XY_BLT_WRITE_RGB | XY_BLT_WRITE_ALPHA );

   if (ret != 0) {
      brw_context_flush( brw );
      ret = try_clear( brw, bsurface, value.ui,
                       XY_BLT_WRITE_RGB | XY_BLT_WRITE_ALPHA );
      assert( ret == 0 );
   }
}

static void zstencil_clear(struct brw_context *brw,
                           struct brw_surface *bsurface,
                           unsigned clear_flags,
                           double depth,
                           unsigned stencil )
{
   enum pipe_error ret;
   unsigned value;
   unsigned mask = 0;
   union fi tmp;

   if (clear_flags & PIPE_CLEAR_DEPTH)
      mask |= XY_BLT_WRITE_RGB;

   switch (bsurface->base.format) {
   case PIPE_FORMAT_Z32_FLOAT:
      tmp.f = (float)depth;
      value = tmp.ui;
      break;
   case PIPE_FORMAT_Z24X8_UNORM:
   case PIPE_FORMAT_Z24_UNORM_S8_USCALED:
      value = ((unsigned)(depth * MASK24) & MASK24);
      break;
   case PIPE_FORMAT_Z16_UNORM:
      value = ((unsigned)(depth * MASK16) & MASK16);
      break;
   default:
      assert(0);
      return;
   }

   switch (bsurface->base.format) {
   case PIPE_FORMAT_Z32_FLOAT:
      mask |= XY_BLT_WRITE_ALPHA;
      break;
   case PIPE_FORMAT_Z24X8_UNORM:
      value = value | (stencil << 24);
      mask |= XY_BLT_WRITE_ALPHA;
      break;
   case PIPE_FORMAT_Z24_UNORM_S8_USCALED:
      value = value | (stencil << 24);
      if (clear_flags & PIPE_CLEAR_STENCIL)
         mask |= XY_BLT_WRITE_ALPHA;
      break;
   case PIPE_FORMAT_Z16_UNORM:
      value = value | (value << 16);
      mask |= XY_BLT_WRITE_ALPHA;
      break;
   default:
      break;
   }

   ret = try_clear( brw, bsurface, value, mask );

   if (ret != 0) {
      brw_context_flush( brw );
      ret = try_clear( brw, bsurface, value, mask );
      assert( ret == 0 );
   }
}



/**
 * Clear the given surface to the specified value.
 * No masking, no scissor (clear entire buffer).
 */
static void brw_clear(struct pipe_context *pipe, 
                      unsigned buffers,
                      const float *rgba,
                      double depth,
                      unsigned stencil)
{
   struct brw_context *brw = brw_context( pipe );
   int i;

   if (buffers & PIPE_CLEAR_COLOR) {
      for (i = 0; i < brw->curr.fb.nr_cbufs; i++) {
         color_clear( brw, 
                      brw_surface(brw->curr.fb.cbufs[i]),
                      rgba );
      }
   }

   if (buffers & PIPE_CLEAR_DEPTHSTENCIL) {
      if (brw->curr.fb.zsbuf) {
         zstencil_clear( brw,
                         brw_surface(brw->curr.fb.zsbuf),
                         buffers & PIPE_CLEAR_DEPTHSTENCIL,
                         depth, stencil );
      }
   }
}

/* XXX should respect region */
static void brw_clear_render_target(struct pipe_context *pipe,
                                    struct pipe_surface *dst,
                                    const float *rgba,
                                    unsigned dstx, unsigned dsty,
                                    unsigned width, unsigned height)
{
   struct brw_context *brw = brw_context( pipe );

   color_clear( brw,
                brw_surface(dst),
                rgba );
}

/* XXX should respect region */
static void brw_clear_depth_stencil(struct pipe_context *pipe,
                                    struct pipe_surface *dst,
                                    unsigned clear_flags,
                                    double depth,
                                    unsigned stencil,
                                    unsigned dstx, unsigned dsty,
                                    unsigned width, unsigned height)
{
   struct brw_context *brw = brw_context( pipe );

   zstencil_clear( brw,
                   brw_surface(dst),
                   clear_flags,
                   depth, stencil );
}

void brw_pipe_clear_init( struct brw_context *brw )
{
   brw->base.clear = brw_clear;
   brw->base.clear_render_target = brw_clear_render_target;
   brw->base.clear_depth_stencil = brw_clear_depth_stencil;
}


void brw_pipe_clear_cleanup( struct brw_context *brw )
{
}
