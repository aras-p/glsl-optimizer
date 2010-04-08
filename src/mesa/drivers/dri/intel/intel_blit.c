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


/* Copy BitBlt
 */
GLboolean
intelEmitCopyBlit(struct intel_context *intel,
		  GLuint cpp,
		  GLshort src_pitch,
		  dri_bo *src_buffer,
		  GLuint src_offset,
		  uint32_t src_tiling,
		  GLshort dst_pitch,
		  dri_bo *dst_buffer,
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
   dri_bo *aper_array[3];
   BATCH_LOCALS;

   /* Blits are in a different ringbuffer so we don't use them. */
   if (intel->gen >= 6)
      return GL_FALSE;

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
       aper_array[0] = intel->batch->buf;
       aper_array[1] = dst_buffer;
       aper_array[2] = src_buffer;

       if (dri_bufmgr_check_aperture_space(aper_array, 3) != 0) {
           intel_batchbuffer_flush(intel->batch);
           pass++;
       } else
           break;
   } while (pass < 2);

   if (pass >= 2) {
      drm_intel_gem_bo_map_gtt(dst_buffer);
      drm_intel_gem_bo_map_gtt(src_buffer);
      _mesa_copy_rect((GLubyte *)dst_buffer->virtual + dst_offset,
		      cpp,
		      dst_pitch,
		      dst_x, dst_y,
		      w, h,
		      (GLubyte *)src_buffer->virtual + src_offset,
		      src_pitch,
		      src_x, src_y);
      drm_intel_gem_bo_unmap_gtt(src_buffer);
      drm_intel_gem_bo_unmap_gtt(dst_buffer);

      return GL_TRUE;
   }

   intel_batchbuffer_require_space(intel->batch, 8 * 4);
   DBG("%s src:buf(%p)/%d+%d %d,%d dst:buf(%p)/%d+%d %d,%d sz:%dx%d\n",
       __FUNCTION__,
       src_buffer, src_pitch, src_offset, src_x, src_y,
       dst_buffer, dst_pitch, dst_offset, dst_x, dst_y, w, h);

   src_pitch *= cpp;
   dst_pitch *= cpp;

   BR13 = translate_raster_op(logic_op) << 16;

   switch (cpp) {
   case 1:
      CMD = XY_SRC_COPY_BLT_CMD;
      break;
   case 2:
      BR13 |= BR13_565;
      CMD = XY_SRC_COPY_BLT_CMD;
      break;
   case 4:
      BR13 |= BR13_8888;
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

   BEGIN_BATCH(8);
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

   intel_batchbuffer_emit_mi_flush(intel->batch);

   return GL_TRUE;
}


/**
 * Use blitting to clear the renderbuffers named by 'flags'.
 * Note: we can't use the ctx->DrawBuffer->_ColorDrawBufferIndexes field
 * since that might include software renderbuffers or renderbuffers
 * which we're clearing with triangles.
 * \param mask  bitmask of BUFFER_BIT_* values indicating buffers to clear
 */
void
intelClearWithBlit(GLcontext *ctx, GLbitfield mask)
{
   struct intel_context *intel = intel_context(ctx);
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   GLuint clear_depth;
   GLboolean all;
   GLint cx, cy, cw, ch;
   BATCH_LOCALS;

   /* Blits are in a different ringbuffer so we don't use them. */
   assert(intel->gen < 6);

   /*
    * Compute values for clearing the buffers.
    */
   clear_depth = 0;
   if (mask & BUFFER_BIT_DEPTH) {
      clear_depth = (GLuint) (fb->_DepthMax * ctx->Depth.Clear);
   }
   if (mask & BUFFER_BIT_STENCIL) {
      clear_depth |= (ctx->Stencil.Clear & 0xff) << 24;
   }

   cx = fb->_Xmin;
   if (fb->Name == 0)
      cy = ctx->DrawBuffer->Height - fb->_Ymax;
   else
      cy = fb->_Ymin;
   cw = fb->_Xmax - fb->_Xmin;
   ch = fb->_Ymax - fb->_Ymin;

   if (cw == 0 || ch == 0)
      return;

   GLuint buf;
   all = (cw == fb->Width && ch == fb->Height);

   /* Loop over all renderbuffers */
   for (buf = 0; buf < BUFFER_COUNT && mask; buf++) {
      const GLbitfield bufBit = 1 << buf;
      struct intel_renderbuffer *irb;
      drm_intel_bo *write_buffer;
      int x1, y1, x2, y2;
      uint32_t clear_val;
      uint32_t BR13, CMD;
      int pitch, cpp;
      drm_intel_bo *aper_array[2];

      if (!(mask & bufBit))
	 continue;

      /* OK, clear this renderbuffer */
      irb = intel_get_renderbuffer(fb, buf);
      write_buffer = intel_region_buffer(intel, irb->region,
					 all ? INTEL_WRITE_FULL :
					 INTEL_WRITE_PART);
      x1 = cx + irb->region->draw_x;
      y1 = cy + irb->region->draw_y;
      x2 = cx + cw + irb->region->draw_x;
      y2 = cy + ch + irb->region->draw_y;

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
	 BR13 |= BR13_8888;
	 if (buf == BUFFER_DEPTH || buf == BUFFER_STENCIL) {
	    if (mask & BUFFER_BIT_DEPTH)
	       CMD |= XY_BLT_WRITE_RGB;
	    if (mask & BUFFER_BIT_STENCIL)
	       CMD |= XY_BLT_WRITE_ALPHA;
	 } else {
	    /* clearing RGBA */
	    CMD |= XY_BLT_WRITE_ALPHA | XY_BLT_WRITE_RGB;
	 }
      } else {
	 ASSERT(cpp == 2);
	 BR13 |= BR13_565;
      }

      assert(irb->region->tiling != I915_TILING_Y);

#ifndef I915
      if (irb->region->tiling != I915_TILING_NONE) {
	 CMD |= XY_DST_TILED;
	 pitch /= 4;
      }
#endif
      BR13 |= (pitch * cpp);

      if (buf == BUFFER_DEPTH || buf == BUFFER_STENCIL) {
	 clear_val = clear_depth;
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
	 default:
	    _mesa_problem(ctx, "Unexpected renderbuffer format: %d\n",
			  irb->Base.Format);
	    clear_val = 0;
	 }
      }

      assert(x1 < x2);
      assert(y1 < y2);

      /* do space check before going any further */
      aper_array[0] = intel->batch->buf;
      aper_array[1] = write_buffer;

      if (drm_intel_bufmgr_check_aperture_space(aper_array,
						ARRAY_SIZE(aper_array)) != 0) {
	 intel_batchbuffer_flush(intel->batch);
      }

      BEGIN_BATCH(6);
      OUT_BATCH(CMD);
      OUT_BATCH(BR13);
      OUT_BATCH((y1 << 16) | x1);
      OUT_BATCH((y2 << 16) | x2);
      OUT_RELOC_FENCED(write_buffer,
		       I915_GEM_DOMAIN_RENDER, I915_GEM_DOMAIN_RENDER,
		       0);
      OUT_BATCH(clear_val);
      ADVANCE_BATCH();

      if (buf == BUFFER_DEPTH || buf == BUFFER_STENCIL)
	 mask &= ~(BUFFER_BIT_DEPTH | BUFFER_BIT_STENCIL);
      else
	 mask &= ~bufBit;    /* turn off bit, for faster loop exit */
   }
}

GLboolean
intelEmitImmediateColorExpandBlit(struct intel_context *intel,
				  GLuint cpp,
				  GLubyte *src_bits, GLuint src_size,
				  GLuint fg_color,
				  GLshort dst_pitch,
				  dri_bo *dst_buffer,
				  GLuint dst_offset,
				  uint32_t dst_tiling,
				  GLshort x, GLshort y,
				  GLshort w, GLshort h,
				  GLenum logic_op)
{
   int dwords = ALIGN(src_size, 8) / 4;
   uint32_t opcode, br13, blit_cmd;

   /* Blits are in a different ringbuffer so we don't use them. */
   if (intel->gen >= 6)
      return GL_FALSE;

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

   intel_batchbuffer_require_space( intel->batch,
				    (8 * 4) +
				    (3 * 4) +
				    dwords * 4 );

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
   if (cpp == 2)
      br13 |= BR13_565;
   else
      br13 |= BR13_8888;

   blit_cmd = XY_TEXT_IMMEDIATE_BLIT_CMD | XY_TEXT_BYTE_PACKED; /* packing? */
   if (dst_tiling != I915_TILING_NONE)
      blit_cmd |= XY_DST_TILED;

   BEGIN_BATCH(8 + 3);
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

   intel_batchbuffer_data( intel->batch,
			   src_bits,
			   dwords * 4 );

   intel_batchbuffer_emit_mi_flush(intel->batch);

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

   /* Blits are in a different ringbuffer so we don't use them. */
   assert(intel->gen < 6);

   /* The pitch is a signed value. */
   pitch = MIN2(size, (1 << 15) - 1);
   height = size / pitch;
   intelEmitCopyBlit(intel, 1,
		     pitch, src_bo, src_offset, I915_TILING_NONE,
		     pitch, dst_bo, dst_offset, I915_TILING_NONE,
		     0, 0, /* src x/y */
		     0, 0, /* dst x/y */
		     pitch, height, /* w, h */
		     GL_COPY);

   src_offset += pitch * height;
   dst_offset += pitch * height;
   size -= pitch * height;
   assert (size < (1 << 15));
   if (size != 0) {
      intelEmitCopyBlit(intel, 1,
			size, src_bo, src_offset, I915_TILING_NONE,
			size, dst_bo, dst_offset, I915_TILING_NONE,
			0, 0, /* src x/y */
			0, 0, /* dst x/y */
			size, 1, /* w, h */
			GL_COPY);
   }
}
