/**************************************************************************
 * 
 * Copyright 2003 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#include "main/mtypes.h"
#include "main/context.h"
#include "main/enums.h"
#include "main/colormac.h"
#include "main/fbobject.h"

#include "intel_blit.h"
#include "intel_buffers.h"
#include "intel_context.h"
#include "intel_fbo.h"
#include "intel_reg.h"
#include "intel_regions.h"
#include "intel_batchbuffer.h"
#include "intel_mipmap_tree.h"

#define FILE_DEBUG_FLAG DEBUG_BLIT

static void
intel_miptree_set_alpha_to_one(struct intel_context *intel,
                               struct intel_mipmap_tree *mt,
                               int x, int y, int width, int height);

static GLuint translate_raster_op(GLenum logicop)
{
   switch(logicop) {
   case GL_CLEAR: return 0x00;
   case GL_AND: return 0x88;
   case GL_AND_REVERSE: return 0x44;
   case GL_COPY: return 0xCC;
   case GL_AND_INVERTED: return 0x22;
   case GL_NOOP: return 0xAA;
   case GL_XOR: return 0x66;
   case GL_OR: return 0xEE;
   case GL_NOR: return 0x11;
   case GL_EQUIV: return 0x99;
   case GL_INVERT: return 0x55;
   case GL_OR_REVERSE: return 0xDD;
   case GL_COPY_INVERTED: return 0x33;
   case GL_OR_INVERTED: return 0xBB;
   case GL_NAND: return 0x77;
   case GL_SET: return 0xFF;
   default: return 0;
   }
}

static uint32_t
br13_for_cpp(int cpp)
{
   switch (cpp) {
   case 4:
      return BR13_8888;
      break;
   case 2:
      return BR13_565;
      break;
   case 1:
      return BR13_8;
      break;
   default:
      assert(0);
      return 0;
   }
}

/**
 * Implements a rectangular block transfer (blit) of pixels between two
 * miptrees.
 *
 * Our blitter can operate on 1, 2, or 4-byte-per-pixel data, with generous,
 * but limited, pitches and sizes allowed.
 *
 * The src/dst coordinates are relative to the given level/slice of the
 * miptree.
 *
 * If @src_flip or @dst_flip is set, then the rectangle within that miptree
 * will be inverted (including scanline order) when copying.  This is common
 * in GL when copying between window system and user-created
 * renderbuffers/textures.
 */
bool
intel_miptree_blit(struct intel_context *intel,
                   struct intel_mipmap_tree *src_mt,
                   int src_level, int src_slice,
                   uint32_t src_x, uint32_t src_y, bool src_flip,
                   struct intel_mipmap_tree *dst_mt,
                   int dst_level, int dst_slice,
                   uint32_t dst_x, uint32_t dst_y, bool dst_flip,
                   uint32_t width, uint32_t height,
                   GLenum logicop)
{
   /* No sRGB decode or encode is done by the hardware blitter, which is
    * consistent with what we want in the callers (glCopyTexSubImage(),
    * glBlitFramebuffer(), texture validation, etc.).
    */
   mesa_format src_format = _mesa_get_srgb_format_linear(src_mt->format);
   mesa_format dst_format = _mesa_get_srgb_format_linear(dst_mt->format);

   /* The blitter doesn't support doing any format conversions.  We do also
    * support blitting ARGB8888 to XRGB8888 (trivial, the values dropped into
    * the X channel don't matter), and XRGB8888 to ARGB8888 by setting the A
    * channel to 1.0 at the end.
    */
   if (src_format != dst_format &&
      ((src_format != MESA_FORMAT_B8G8R8A8_UNORM &&
        src_format != MESA_FORMAT_B8G8R8X8_UNORM) ||
       (dst_format != MESA_FORMAT_B8G8R8A8_UNORM &&
        dst_format != MESA_FORMAT_B8G8R8X8_UNORM))) {
      perf_debug("%s: Can't use hardware blitter from %s to %s, "
                 "falling back.\n", __FUNCTION__,
                 _mesa_get_format_name(src_format),
                 _mesa_get_format_name(dst_format));
      return false;
   }

   /* According to the Ivy Bridge PRM, Vol1 Part4, section 1.2.1.2 (Graphics
    * Data Size Limitations):
    *
    *    The BLT engine is capable of transferring very large quantities of
    *    graphics data. Any graphics data read from and written to the
    *    destination is permitted to represent a number of pixels that
    *    occupies up to 65,536 scan lines and up to 32,768 bytes per scan line
    *    at the destination. The maximum number of pixels that may be
    *    represented per scan line’s worth of graphics data depends on the
    *    color depth.
    *
    * Furthermore, intelEmitCopyBlit (which is called below) uses a signed
    * 16-bit integer to represent buffer pitch, so it can only handle buffer
    * pitches < 32k.
    *
    * As a result of these two limitations, we can only use the blitter to do
    * this copy when the region's pitch is less than 32k.
    */
   if (src_mt->region->pitch > 32768 ||
       dst_mt->region->pitch > 32768) {
      perf_debug("Falling back due to >32k pitch\n");
      return false;
   }

   if (src_flip)
      src_y = src_mt->level[src_level].height - src_y - height;

   if (dst_flip)
      dst_y = dst_mt->level[dst_level].height - dst_y - height;

   int src_pitch = src_mt->region->pitch;
   if (src_flip != dst_flip)
      src_pitch = -src_pitch;

   uint32_t src_image_x, src_image_y;
   intel_miptree_get_image_offset(src_mt, src_level, src_slice,
                                  &src_image_x, &src_image_y);
   src_x += src_image_x;
   src_y += src_image_y;

   uint32_t dst_image_x, dst_image_y;
   intel_miptree_get_image_offset(dst_mt, dst_level, dst_slice,
                                  &dst_image_x, &dst_image_y);
   dst_x += dst_image_x;
   dst_y += dst_image_y;

   if (!intelEmitCopyBlit(intel,
                          src_mt->cpp,
                          src_pitch,
                          src_mt->region->bo, src_mt->offset,
                          src_mt->region->tiling,
                          dst_mt->region->pitch,
                          dst_mt->region->bo, dst_mt->offset,
                          dst_mt->region->tiling,
                          src_x, src_y,
                          dst_x, dst_y,
                          width, height,
                          logicop)) {
      return false;
   }

   if (src_mt->format == MESA_FORMAT_B8G8R8X8_UNORM &&
       dst_mt->format == MESA_FORMAT_B8G8R8A8_UNORM) {
      intel_miptree_set_alpha_to_one(intel, dst_mt,
                                     dst_x, dst_y,
                                     width, height);
   }

   return true;
}

/* Copy BitBlt
 */
bool
intelEmitCopyBlit(struct intel_context *intel,
		  GLuint cpp,
		  GLshort src_pitch,
		  drm_intel_bo *src_buffer,
		  GLuint src_offset,
		  uint32_t src_tiling,
		  GLshort dst_pitch,
		  drm_intel_bo *dst_buffer,
		  GLuint dst_offset,
		  uint32_t dst_tiling,
		  GLshort src_x, GLshort src_y,
		  GLshort dst_x, GLshort dst_y,
		  GLshort w, GLshort h,
		  GLenum logic_op)
{
   GLuint CMD, BR13, pass = 0;
   int dst_y2 = dst_y + h;
   int dst_x2 = dst_x + w;
   drm_intel_bo *aper_array[3];
   bool dst_y_tiled = dst_tiling == I915_TILING_Y;
   bool src_y_tiled = src_tiling == I915_TILING_Y;
   BATCH_LOCALS;

   if (dst_tiling != I915_TILING_NONE) {
      if (dst_offset & 4095)
	 return false;
   }
   if (src_tiling != I915_TILING_NONE) {
      if (src_offset & 4095)
	 return false;
   }
   if (dst_y_tiled || src_y_tiled)
      return false;

   /* do space check before going any further */
   do {
       aper_array[0] = intel->batch.bo;
       aper_array[1] = dst_buffer;
       aper_array[2] = src_buffer;

       if (dri_bufmgr_check_aperture_space(aper_array, 3) != 0) {
           intel_batchbuffer_flush(intel);
           pass++;
       } else
           break;
   } while (pass < 2);

   if (pass >= 2)
      return false;

   intel_batchbuffer_require_space(intel, 8 * 4);
   DBG("%s src:buf(%p)/%d+%d %d,%d dst:buf(%p)/%d+%d %d,%d sz:%dx%d\n",
       __FUNCTION__,
       src_buffer, src_pitch, src_offset, src_x, src_y,
       dst_buffer, dst_pitch, dst_offset, dst_x, dst_y, w, h);

   /* Blit pitch must be dword-aligned.  Otherwise, the hardware appears to drop
    * the low bits.
    */
   if (src_pitch % 4 != 0 || dst_pitch % 4 != 0)
      return false;

   /* For big formats (such as floating point), do the copy using 16 or 32bpp
    * and multiply the coordinates.
    */
   if (cpp > 4) {
      if (cpp % 4 == 2) {
         dst_x *= cpp / 2;
         dst_x2 *= cpp / 2;
         src_x *= cpp / 2;
         cpp = 2;
      } else {
         assert(cpp % 4 == 0);
         dst_x *= cpp / 4;
         dst_x2 *= cpp / 4;
         src_x *= cpp / 4;
         cpp = 4;
      }
   }

   BR13 = br13_for_cpp(cpp) | translate_raster_op(logic_op) << 16;

   switch (cpp) {
   case 1:
   case 2:
      CMD = XY_SRC_COPY_BLT_CMD;
      break;
   case 4:
      CMD = XY_SRC_COPY_BLT_CMD | XY_BLT_WRITE_ALPHA | XY_BLT_WRITE_RGB;
      break;
   default:
      return false;
   }

   if (dst_y2 <= dst_y || dst_x2 <= dst_x) {
      return true;
   }

   assert(dst_x < dst_x2);
   assert(dst_y < dst_y2);

   BEGIN_BATCH(8);

   OUT_BATCH(CMD | (8 - 2));
   OUT_BATCH(BR13 | (uint16_t)dst_pitch);
   OUT_BATCH((dst_y << 16) | dst_x);
   OUT_BATCH((dst_y2 << 16) | dst_x2);
   OUT_RELOC_FENCED(dst_buffer,
		    I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		    dst_offset);
   OUT_BATCH((src_y << 16) | src_x);
   OUT_BATCH((uint16_t)src_pitch);
   OUT_RELOC_FENCED(src_buffer,
		    I915_GEM_DOMAIN_RENDER, 0,
		    src_offset);

   ADVANCE_BATCH();

   intel_batchbuffer_emit_mi_flush(intel);

   return true;
}


/**
 * Use blitting to clear the renderbuffers named by 'flags'.
 * Note: we can't use the ctx->DrawBuffer->_ColorDrawBufferIndexes field
 * since that might include software renderbuffers or renderbuffers
 * which we're clearing with triangles.
 * \param mask  bitmask of BUFFER_BIT_* values indicating buffers to clear
 */
GLbitfield
intelClearWithBlit(struct gl_context *ctx, GLbitfield mask)
{
   struct intel_context *intel = intel_context(ctx);
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   GLuint clear_depth_value, clear_depth_mask;
   GLint cx, cy, cw, ch;
   GLbitfield fail_mask = 0;
   BATCH_LOCALS;

   /* Note: we don't use this function on Gen7+ hardware, so we can safely
    * ignore fast color clear issues.
    */
   assert(intel->gen < 7);

   /*
    * Compute values for clearing the buffers.
    */
   clear_depth_value = 0;
   clear_depth_mask = 0;
   if (mask & BUFFER_BIT_DEPTH) {
      clear_depth_value = (GLuint) (fb->_DepthMax * ctx->Depth.Clear);
      clear_depth_mask = XY_BLT_WRITE_RGB;
   }
   if (mask & BUFFER_BIT_STENCIL) {
      clear_depth_value |= (ctx->Stencil.Clear & 0xff) << 24;
      clear_depth_mask |= XY_BLT_WRITE_ALPHA;
   }

   cx = fb->_Xmin;
   if (_mesa_is_winsys_fbo(fb))
      cy = ctx->DrawBuffer->Height - fb->_Ymax;
   else
      cy = fb->_Ymin;
   cw = fb->_Xmax - fb->_Xmin;
   ch = fb->_Ymax - fb->_Ymin;

   if (cw == 0 || ch == 0)
      return 0;

   /* Loop over all renderbuffers */
   mask &= (1 << BUFFER_COUNT) - 1;
   while (mask) {
      GLuint buf = ffs(mask) - 1;
      bool is_depth_stencil = buf == BUFFER_DEPTH || buf == BUFFER_STENCIL;
      struct intel_renderbuffer *irb;
      int x1, y1, x2, y2;
      uint32_t clear_val;
      uint32_t BR13, CMD;
      struct intel_region *region;
      int pitch, cpp;
      drm_intel_bo *aper_array[2];

      mask &= ~(1 << buf);

      irb = intel_get_renderbuffer(fb, buf);
      if (irb && irb->mt) {
	 region = irb->mt->region;
	 assert(region);
	 assert(region->bo);
      } else {
         fail_mask |= 1 << buf;
         continue;
      }

      /* OK, clear this renderbuffer */
      x1 = cx + irb->draw_x;
      y1 = cy + irb->draw_y;
      x2 = cx + cw + irb->draw_x;
      y2 = cy + ch + irb->draw_y;

      pitch = region->pitch;
      cpp = region->cpp;

      DBG("%s dst:buf(%p)/%d %d,%d sz:%dx%d\n",
	  __FUNCTION__,
	  region->bo, pitch,
	  x1, y1, x2 - x1, y2 - y1);

      BR13 = 0xf0 << 16;
      CMD = XY_COLOR_BLT_CMD;

      /* Setup the blit command */
      if (cpp == 4) {
	 if (is_depth_stencil) {
	    CMD |= clear_depth_mask;
	 } else {
	    /* clearing RGBA */
	    CMD |= XY_BLT_WRITE_ALPHA | XY_BLT_WRITE_RGB;
	 }
      }

      assert(region->tiling != I915_TILING_Y);

      BR13 |= pitch;

      if (is_depth_stencil) {
	 clear_val = clear_depth_value;
      } else {
	 uint8_t clear[4];
	 GLfloat *color = ctx->Color.ClearColor.f;

	 _mesa_unclamped_float_rgba_to_ubyte(clear, color);

	 switch (intel_rb_format(irb)) {
	 case MESA_FORMAT_B8G8R8A8_UNORM:
	 case MESA_FORMAT_B8G8R8X8_UNORM:
	    clear_val = PACK_COLOR_8888(clear[3], clear[0],
					clear[1], clear[2]);
	    break;
	 case MESA_FORMAT_B5G6R5_UNORM:
	    clear_val = PACK_COLOR_565(clear[0], clear[1], clear[2]);
	    break;
	 case MESA_FORMAT_B4G4R4A4_UNORM:
	    clear_val = PACK_COLOR_4444(clear[3], clear[0],
					clear[1], clear[2]);
	    break;
	 case MESA_FORMAT_B5G5R5A1_UNORM:
	    clear_val = PACK_COLOR_1555(clear[3], clear[0],
					clear[1], clear[2]);
	    break;
	 case MESA_FORMAT_A_UNORM8:
	    clear_val = PACK_COLOR_8888(clear[3], clear[3],
					clear[3], clear[3]);
	    break;
	 default:
	    fail_mask |= 1 << buf;
	    continue;
	 }
      }

      BR13 |= br13_for_cpp(cpp);

      assert(x1 < x2);
      assert(y1 < y2);

      /* do space check before going any further */
      aper_array[0] = intel->batch.bo;
      aper_array[1] = region->bo;

      if (drm_intel_bufmgr_check_aperture_space(aper_array,
						ARRAY_SIZE(aper_array)) != 0) {
	 intel_batchbuffer_flush(intel);
      }

      BEGIN_BATCH(6);
      OUT_BATCH(CMD | (6 - 2));
      OUT_BATCH(BR13);
      OUT_BATCH((y1 << 16) | x1);
      OUT_BATCH((y2 << 16) | x2);
      OUT_RELOC_FENCED(region->bo,
		       I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		       0);
      OUT_BATCH(clear_val);
      ADVANCE_BATCH();

      if (intel->always_flush_cache)
	 intel_batchbuffer_emit_mi_flush(intel);

      if (buf == BUFFER_DEPTH || buf == BUFFER_STENCIL)
	 mask &= ~(BUFFER_BIT_DEPTH | BUFFER_BIT_STENCIL);
   }

   return fail_mask;
}

bool
intelEmitImmediateColorExpandBlit(struct intel_context *intel,
				  GLuint cpp,
				  GLubyte *src_bits, GLuint src_size,
				  GLuint fg_color,
				  GLshort dst_pitch,
				  drm_intel_bo *dst_buffer,
				  GLuint dst_offset,
				  uint32_t dst_tiling,
				  GLshort x, GLshort y,
				  GLshort w, GLshort h,
				  GLenum logic_op)
{
   int dwords = ALIGN(src_size, 8) / 4;
   uint32_t opcode, br13, blit_cmd;

   if (dst_tiling != I915_TILING_NONE) {
      if (dst_offset & 4095)
	 return false;
      if (dst_tiling == I915_TILING_Y)
	 return false;
   }

   assert((logic_op >= GL_CLEAR) && (logic_op <= (GL_CLEAR + 0x0f)));
   assert(dst_pitch > 0);

   if (w < 0 || h < 0)
      return true;

   DBG("%s dst:buf(%p)/%d+%d %d,%d sz:%dx%d, %d bytes %d dwords\n",
       __FUNCTION__,
       dst_buffer, dst_pitch, dst_offset, x, y, w, h, src_size, dwords);

   intel_batchbuffer_require_space(intel,
				   (8 * 4) +
				   (3 * 4) +
				   dwords * 4);

   opcode = XY_SETUP_BLT_CMD;
   if (cpp == 4)
      opcode |= XY_BLT_WRITE_ALPHA | XY_BLT_WRITE_RGB;

   br13 = dst_pitch | (translate_raster_op(logic_op) << 16) | (1 << 29);
   br13 |= br13_for_cpp(cpp);

   blit_cmd = XY_TEXT_IMMEDIATE_BLIT_CMD | XY_TEXT_BYTE_PACKED; /* packing? */
   if (dst_tiling != I915_TILING_NONE)
      blit_cmd |= XY_DST_TILED;

   BEGIN_BATCH(8 + 3);
   OUT_BATCH(opcode | (8 - 2));
   OUT_BATCH(br13);
   OUT_BATCH((0 << 16) | 0); /* clip x1, y1 */
   OUT_BATCH((100 << 16) | 100); /* clip x2, y2 */
   OUT_RELOC_FENCED(dst_buffer,
		    I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		    dst_offset);
   OUT_BATCH(0); /* bg */
   OUT_BATCH(fg_color); /* fg */
   OUT_BATCH(0); /* pattern base addr */

   OUT_BATCH(blit_cmd | ((3 - 2) + dwords));
   OUT_BATCH((y << 16) | x);
   OUT_BATCH(((y + h) << 16) | (x + w));
   ADVANCE_BATCH();

   intel_batchbuffer_data(intel, src_bits, dwords * 4);

   intel_batchbuffer_emit_mi_flush(intel);

   return true;
}

/* We don't have a memmove-type blit like some other hardware, so we'll do a
 * rectangular blit covering a large space, then emit 1-scanline blit at the
 * end to cover the last if we need.
 */
void
intel_emit_linear_blit(struct intel_context *intel,
		       drm_intel_bo *dst_bo,
		       unsigned int dst_offset,
		       drm_intel_bo *src_bo,
		       unsigned int src_offset,
		       unsigned int size)
{
   struct gl_context *ctx = &intel->ctx;
   GLuint pitch, height;
   bool ok;

   /* The pitch given to the GPU must be DWORD aligned, and
    * we want width to match pitch. Max width is (1 << 15 - 1),
    * rounding that down to the nearest DWORD is 1 << 15 - 4
    */
   pitch = ROUND_DOWN_TO(MIN2(size, (1 << 15) - 1), 4);
   height = (pitch == 0) ? 1 : size / pitch;
   ok = intelEmitCopyBlit(intel, 1,
			  pitch, src_bo, src_offset, I915_TILING_NONE,
			  pitch, dst_bo, dst_offset, I915_TILING_NONE,
			  0, 0, /* src x/y */
			  0, 0, /* dst x/y */
			  pitch, height, /* w, h */
			  GL_COPY);
   if (!ok)
      _mesa_problem(ctx, "Failed to linear blit %dx%d\n", pitch, height);

   src_offset += pitch * height;
   dst_offset += pitch * height;
   size -= pitch * height;
   assert (size < (1 << 15));
   pitch = ALIGN(size, 4);
   if (size != 0) {
      ok = intelEmitCopyBlit(intel, 1,
			     pitch, src_bo, src_offset, I915_TILING_NONE,
			     pitch, dst_bo, dst_offset, I915_TILING_NONE,
			     0, 0, /* src x/y */
			     0, 0, /* dst x/y */
			     size, 1, /* w, h */
			     GL_COPY);
      if (!ok)
         _mesa_problem(ctx, "Failed to linear blit %dx%d\n", size, 1);
   }
}

/**
 * Used to initialize the alpha value of an ARGB8888 miptree after copying
 * into it from an XRGB8888 source.
 *
 * This is very common with glCopyTexImage2D().  Note that the coordinates are
 * relative to the start of the miptree, not relative to a slice within the
 * miptree.
 */
static void
intel_miptree_set_alpha_to_one(struct intel_context *intel,
                              struct intel_mipmap_tree *mt,
                              int x, int y, int width, int height)
{
   struct intel_region *region = mt->region;
   uint32_t BR13, CMD;
   int pitch, cpp;
   drm_intel_bo *aper_array[2];
   BATCH_LOCALS;

   pitch = region->pitch;
   cpp = region->cpp;

   DBG("%s dst:buf(%p)/%d %d,%d sz:%dx%d\n",
       __FUNCTION__, region->bo, pitch, x, y, width, height);

   BR13 = br13_for_cpp(cpp) | 0xf0 << 16;
   CMD = XY_COLOR_BLT_CMD;
   CMD |= XY_BLT_WRITE_ALPHA;

   BR13 |= pitch;

   /* do space check before going any further */
   aper_array[0] = intel->batch.bo;
   aper_array[1] = region->bo;

   if (drm_intel_bufmgr_check_aperture_space(aper_array,
					     ARRAY_SIZE(aper_array)) != 0) {
      intel_batchbuffer_flush(intel);
   }

   BEGIN_BATCH(6);
   OUT_BATCH(CMD | (6 - 2));
   OUT_BATCH(BR13);
   OUT_BATCH((y << 16) | x);
   OUT_BATCH(((y + height) << 16) | (x + width));
   OUT_RELOC_FENCED(region->bo,
		    I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		    0);
   OUT_BATCH(0xffffffff); /* white, but only alpha gets written */
   ADVANCE_BATCH();

   intel_batchbuffer_emit_mi_flush(intel);
}
