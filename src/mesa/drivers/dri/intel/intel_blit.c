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


#include "main/mtypes.h"
#include "main/context.h"
#include "main/enums.h"
#include "main/colormac.h"

#include "intel_blit.h"
#include "intel_buffers.h"
#include "intel_context.h"
#include "intel_fbo.h"
#include "intel_reg.h"
#include "intel_regions.h"
#include "intel_batchbuffer.h"
#include "intel_mipmap_tree.h"

#define FILE_DEBUG_FLAG DEBUG_BLIT

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

/* Copy BitBlt
 */
GLboolean
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
   BATCH_LOCALS;

   if (dst_tiling != I915_TILING_NONE) {
      if (dst_offset & 4095)
	 return GL_FALSE;
      if (dst_tiling == I915_TILING_Y)
	 return GL_FALSE;
   }
   if (src_tiling != I915_TILING_NONE) {
      if (src_offset & 4095)
	 return GL_FALSE;
      if (src_tiling == I915_TILING_Y)
	 return GL_FALSE;
   }

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
      return GL_FALSE;

   intel_batchbuffer_require_space(intel, 8 * 4, true);
   DBG("%s src:buf(%p)/%d+%d %d,%d dst:buf(%p)/%d+%d %d,%d sz:%dx%d\n",
       __FUNCTION__,
       src_buffer, src_pitch, src_offset, src_x, src_y,
       dst_buffer, dst_pitch, dst_offset, dst_x, dst_y, w, h);

   src_pitch *= cpp;
   dst_pitch *= cpp;

   /* For big formats (such as floating point), do the copy using 32bpp and
    * multiply the coordinates.
    */
   if (cpp > 4) {
      assert(cpp % 4 == 0);
      dst_x *= cpp / 4;
      dst_x2 *= cpp / 4;
      src_x *= cpp / 4;
      cpp = 4;
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
      return GL_FALSE;
   }

#ifndef I915
   if (dst_tiling != I915_TILING_NONE) {
      CMD |= XY_DST_TILED;
      dst_pitch /= 4;
   }
   if (src_tiling != I915_TILING_NONE) {
      CMD |= XY_SRC_TILED;
      src_pitch /= 4;
   }
#endif

   if (dst_y2 <= dst_y || dst_x2 <= dst_x) {
      return GL_TRUE;
   }

   assert(dst_x < dst_x2);
   assert(dst_y < dst_y2);

   BEGIN_BATCH_BLT(8);
   OUT_BATCH(CMD);
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

   return GL_TRUE;
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
   GLboolean all;
   GLint cx, cy, cw, ch;
   GLbitfield fail_mask = 0;
   BATCH_LOCALS;

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
   if (fb->Name == 0)
      cy = ctx->DrawBuffer->Height - fb->_Ymax;
   else
      cy = fb->_Ymin;
   cw = fb->_Xmax - fb->_Xmin;
   ch = fb->_Ymax - fb->_Ymin;

   if (cw == 0 || ch == 0)
      return 0;

   all = (cw == fb->Width && ch == fb->Height);

   /* Loop over all renderbuffers */
   mask &= (1 << BUFFER_COUNT) - 1;
   while (mask) {
      GLuint buf = _mesa_ffs(mask) - 1;
      GLboolean is_depth_stencil = buf == BUFFER_DEPTH || buf == BUFFER_STENCIL;
      struct intel_renderbuffer *irb;
      drm_intel_bo *write_buffer;
      int x1, y1, x2, y2;
      uint32_t clear_val;
      uint32_t BR13, CMD;
      int pitch, cpp;
      drm_intel_bo *aper_array[2];

      mask &= ~(1 << buf);

      irb = intel_get_renderbuffer(fb, buf);
      if (irb == NULL || irb->region == NULL || irb->region->buffer == NULL) {
         fail_mask |= 1 << buf;
         continue;
      }

      /* OK, clear this renderbuffer */
      write_buffer = intel_region_buffer(intel, irb->region,
					 all ? INTEL_WRITE_FULL :
					 INTEL_WRITE_PART);
      x1 = cx + irb->draw_x;
      y1 = cy + irb->draw_y;
      x2 = cx + cw + irb->draw_x;
      y2 = cy + ch + irb->draw_y;

      pitch = irb->region->pitch;
      cpp = irb->region->cpp;

      DBG("%s dst:buf(%p)/%d %d,%d sz:%dx%d\n",
	  __FUNCTION__,
	  irb->region->buffer, (pitch * cpp),
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

      assert(irb->region->tiling != I915_TILING_Y);

#ifndef I915
      if (irb->region->tiling != I915_TILING_NONE) {
	 CMD |= XY_DST_TILED;
	 pitch /= 4;
      }
#endif
      BR13 |= (pitch * cpp);

      if (is_depth_stencil) {
	 clear_val = clear_depth_value;
      } else {
	 uint8_t clear[4];
	 GLclampf *color = ctx->Color.ClearColor;

	 CLAMPED_FLOAT_TO_UBYTE(clear[0], color[0]);
	 CLAMPED_FLOAT_TO_UBYTE(clear[1], color[1]);
	 CLAMPED_FLOAT_TO_UBYTE(clear[2], color[2]);
	 CLAMPED_FLOAT_TO_UBYTE(clear[3], color[3]);

	 switch (irb->Base.Format) {
	 case MESA_FORMAT_ARGB8888:
	 case MESA_FORMAT_XRGB8888:
	    clear_val = PACK_COLOR_8888(clear[3], clear[0],
					clear[1], clear[2]);
	    break;
	 case MESA_FORMAT_RGB565:
	    clear_val = PACK_COLOR_565(clear[0], clear[1], clear[2]);
	    break;
	 case MESA_FORMAT_ARGB4444:
	    clear_val = PACK_COLOR_4444(clear[3], clear[0],
					clear[1], clear[2]);
	    break;
	 case MESA_FORMAT_ARGB1555:
	    clear_val = PACK_COLOR_1555(clear[3], clear[0],
					clear[1], clear[2]);
	    break;
	 case MESA_FORMAT_A8:
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
      aper_array[1] = write_buffer;

      if (drm_intel_bufmgr_check_aperture_space(aper_array,
						ARRAY_SIZE(aper_array)) != 0) {
	 intel_batchbuffer_flush(intel);
      }

      BEGIN_BATCH_BLT(6);
      OUT_BATCH(CMD);
      OUT_BATCH(BR13);
      OUT_BATCH((y1 << 16) | x1);
      OUT_BATCH((y2 << 16) | x2);
      OUT_RELOC_FENCED(write_buffer,
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

GLboolean
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
	 return GL_FALSE;
      if (dst_tiling == I915_TILING_Y)
	 return GL_FALSE;
   }

   assert( logic_op - GL_CLEAR >= 0 );
   assert( logic_op - GL_CLEAR < 0x10 );
   assert(dst_pitch > 0);

   if (w < 0 || h < 0)
      return GL_TRUE;

   dst_pitch *= cpp;

   DBG("%s dst:buf(%p)/%d+%d %d,%d sz:%dx%d, %d bytes %d dwords\n",
       __FUNCTION__,
       dst_buffer, dst_pitch, dst_offset, x, y, w, h, src_size, dwords);

   intel_batchbuffer_require_space(intel,
				   (8 * 4) +
				   (3 * 4) +
				   dwords * 4, true);

   opcode = XY_SETUP_BLT_CMD;
   if (cpp == 4)
      opcode |= XY_BLT_WRITE_ALPHA | XY_BLT_WRITE_RGB;
#ifndef I915
   if (dst_tiling != I915_TILING_NONE) {
      opcode |= XY_DST_TILED;
      dst_pitch /= 4;
   }
#endif

   br13 = dst_pitch | (translate_raster_op(logic_op) << 16) | (1 << 29);
   br13 |= br13_for_cpp(cpp);

   blit_cmd = XY_TEXT_IMMEDIATE_BLIT_CMD | XY_TEXT_BYTE_PACKED; /* packing? */
   if (dst_tiling != I915_TILING_NONE)
      blit_cmd |= XY_DST_TILED;

   BEGIN_BATCH_BLT(8 + 3);
   OUT_BATCH(opcode);
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

   intel_batchbuffer_data(intel, src_bits, dwords * 4, true);

   intel_batchbuffer_emit_mi_flush(intel);

   return GL_TRUE;
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
   GLuint pitch, height;
   GLboolean ok;

   /* The pitch given to the GPU must be DWORD aligned, and
    * we want width to match pitch. Max width is (1 << 15 - 1),
    * rounding that down to the nearest DWORD is 1 << 15 - 4
    */
   pitch = MIN2(size, (1 << 15) - 4);
   height = size / pitch;
   ok = intelEmitCopyBlit(intel, 1,
			  pitch, src_bo, src_offset, I915_TILING_NONE,
			  pitch, dst_bo, dst_offset, I915_TILING_NONE,
			  0, 0, /* src x/y */
			  0, 0, /* dst x/y */
			  pitch, height, /* w, h */
			  GL_COPY);
   assert(ok);

   src_offset += pitch * height;
   dst_offset += pitch * height;
   size -= pitch * height;
   assert (size < (1 << 15));
   assert ((size & 3) == 0); /* Pitch must be DWORD aligned */
   if (size != 0) {
      ok = intelEmitCopyBlit(intel, 1,
			     size, src_bo, src_offset, I915_TILING_NONE,
			     size, dst_bo, dst_offset, I915_TILING_NONE,
			     0, 0, /* src x/y */
			     0, 0, /* dst x/y */
			     size, 1, /* w, h */
			     GL_COPY);
      assert(ok);
   }
}

/**
 * Used to initialize the alpha value of an ARGB8888 teximage after
 * loading it from an XRGB8888 source.
 *
 * This is very common with glCopyTexImage2D().
 */
void
intel_set_teximage_alpha_to_one(struct gl_context *ctx,
				struct intel_texture_image *intel_image)
{
   struct intel_context *intel = intel_context(ctx);
   unsigned int image_x, image_y;
   uint32_t x1, y1, x2, y2;
   uint32_t BR13, CMD;
   int pitch, cpp;
   drm_intel_bo *aper_array[2];
   struct intel_region *region = intel_image->mt->region;
   BATCH_LOCALS;

   assert(intel_image->base.TexFormat == MESA_FORMAT_ARGB8888);

   /* get dest x/y in destination texture */
   intel_miptree_get_image_offset(intel_image->mt,
				  intel_image->level,
				  intel_image->face,
				  0,
				  &image_x, &image_y);

   x1 = image_x;
   y1 = image_y;
   x2 = image_x + intel_image->base.Width;
   y2 = image_y + intel_image->base.Height;

   pitch = region->pitch;
   cpp = region->cpp;

   DBG("%s dst:buf(%p)/%d %d,%d sz:%dx%d\n",
       __FUNCTION__,
       intel_image->mt->region->buffer, (pitch * cpp),
       x1, y1, x2 - x1, y2 - y1);

   BR13 = br13_for_cpp(cpp) | 0xf0 << 16;
   CMD = XY_COLOR_BLT_CMD;
   CMD |= XY_BLT_WRITE_ALPHA;

   assert(region->tiling != I915_TILING_Y);

#ifndef I915
   if (region->tiling != I915_TILING_NONE) {
      CMD |= XY_DST_TILED;
      pitch /= 4;
   }
#endif
   BR13 |= (pitch * cpp);

   /* do space check before going any further */
   aper_array[0] = intel->batch.bo;
   aper_array[1] = region->buffer;

   if (drm_intel_bufmgr_check_aperture_space(aper_array,
					     ARRAY_SIZE(aper_array)) != 0) {
      intel_batchbuffer_flush(intel);
   }

   BEGIN_BATCH_BLT(6);
   OUT_BATCH(CMD);
   OUT_BATCH(BR13);
   OUT_BATCH((y1 << 16) | x1);
   OUT_BATCH((y2 << 16) | x2);
   OUT_RELOC_FENCED(region->buffer,
		    I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		    0);
   OUT_BATCH(0xffffffff); /* white, but only alpha gets written */
   ADVANCE_BATCH();

   intel_batchbuffer_emit_mi_flush(intel);
}
