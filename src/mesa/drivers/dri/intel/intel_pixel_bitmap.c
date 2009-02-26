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
 * next paragraph) shall be included in all copies or substantial portionsalloc
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

#include "main/glheader.h"
#include "main/enums.h"
#include "main/image.h"
#include "main/colormac.h"
#include "main/mtypes.h"
#include "main/macros.h"
#include "main/bufferobj.h"
#include "main/pixelstore.h"
#include "main/state.h"
#include "main/teximage.h"
#include "main/texenv.h"
#include "main/texobj.h"
#include "main/texstate.h"
#include "main/texparam.h"
#include "main/varray.h"
#include "main/attrib.h"
#include "main/enable.h"
#include "shader/arbprogram.h"
#include "glapi/dispatch.h"
#include "swrast/swrast.h"

#include "intel_screen.h"
#include "intel_context.h"
#include "intel_batchbuffer.h"
#include "intel_blit.h"
#include "intel_regions.h"
#include "intel_buffer_objects.h"
#include "intel_buffers.h"
#include "intel_pixel.h"
#include "intel_reg.h"


#define FILE_DEBUG_FLAG DEBUG_PIXEL


/* Unlike the other intel_pixel_* functions, the expectation here is
 * that the incoming data is not in a PBO.  With the XY_TEXT blit
 * method, there's no benefit haveing it in a PBO, but we could
 * implement a path based on XY_MONO_SRC_COPY_BLIT which might benefit
 * PBO bitmaps.  I think they are probably pretty rare though - I
 * wonder if Xgl uses them?
 */
static const GLubyte *map_pbo( GLcontext *ctx,
			       GLsizei width, GLsizei height,
			       const struct gl_pixelstore_attrib *unpack,
			       const GLubyte *bitmap )
{
   GLubyte *buf;

   if (!_mesa_validate_pbo_access(2, unpack, width, height, 1,
				  GL_COLOR_INDEX, GL_BITMAP,
				  (GLvoid *) bitmap)) {
      _mesa_error(ctx, GL_INVALID_OPERATION,"glBitmap(invalid PBO access)");
      return NULL;
   }

   buf = (GLubyte *) ctx->Driver.MapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
					   GL_READ_ONLY_ARB,
					   unpack->BufferObj);
   if (!buf) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBitmap(PBO is mapped)");
      return NULL;
   }

   return ADD_POINTERS(buf, bitmap);
}

static GLboolean test_bit( const GLubyte *src,
			    GLuint bit )
{
   return (src[bit/8] & (1<<(bit % 8))) ? 1 : 0;
}

static GLboolean test_msb_bit(const GLubyte *src, GLuint bit)
{
   return (src[bit/8] & (1<<(7 - (bit % 8)))) ? 1 : 0;
}

static void set_bit( GLubyte *dest,
			  GLuint bit )
{
   dest[bit/8] |= 1 << (bit % 8);
}

/* Extract a rectangle's worth of data from the bitmap.  Called
 * per-cliprect.
 */
static GLuint get_bitmap_rect(GLsizei width, GLsizei height,
			      const struct gl_pixelstore_attrib *unpack,
			      const GLubyte *bitmap,
			      GLuint x, GLuint y, 
			      GLuint w, GLuint h,
			      GLubyte *dest,
			      GLuint row_align,
			      GLboolean invert)
{
   GLuint src_offset = (x + unpack->SkipPixels) & 0x7;
   GLuint mask = unpack->LsbFirst ? 0 : 7;
   GLuint bit = 0;
   GLint row, col;
   GLint first, last;
   GLint incr;
   GLuint count = 0;

   if (INTEL_DEBUG & DEBUG_PIXEL)
      _mesa_printf("%s %d,%d %dx%d bitmap %dx%d skip %d src_offset %d mask %d\n",
		   __FUNCTION__, x,y,w,h,width,height,unpack->SkipPixels, src_offset, mask);

   if (invert) {
      first = h-1;
      last = 0;
      incr = -1;
   }
   else {
      first = 0;
      last = h-1;
      incr = 1;
   }

   /* Require that dest be pre-zero'd.
    */
   for (row = first; row != (last+incr); row += incr) {
      const GLubyte *rowsrc = _mesa_image_address2d(unpack, bitmap, 
						    width, height, 
						    GL_COLOR_INDEX, GL_BITMAP, 
						    y + row, x);

      for (col = 0; col < w; col++, bit++) {
	 if (test_bit(rowsrc, (col + src_offset) ^ mask)) {
	    set_bit(dest, bit ^ 7);
	    count++;
	 }
      }

      if (row_align)
	 bit = ALIGN(bit, row_align);
   }

   return count;
}

/**
 * Returns the low Y value of the vertical range given, flipped according to
 * whether the framebuffer is or not.
 */
static inline int
y_flip(struct gl_framebuffer *fb, int y, int height)
{
   if (fb->Name != 0)
      return y;
   else
      return fb->Height - y - height;
}

/*
 * Render a bitmap.
 */
static GLboolean
do_blit_bitmap( GLcontext *ctx, 
		GLint dstx, GLint dsty,
		GLsizei width, GLsizei height,
		const struct gl_pixelstore_attrib *unpack,
		const GLubyte *bitmap )
{
   struct intel_context *intel = intel_context(ctx);
   struct intel_region *dst = intel_drawbuf_region(intel);
   struct gl_framebuffer *fb = ctx->DrawBuffer;
   GLfloat tmpColor[4];
   GLubyte ubcolor[4];
   GLuint color8888, color565;
   unsigned int num_cliprects;
   drm_clip_rect_t *cliprects;
   int x_off, y_off;
   GLsizei bitmap_width = width;
   GLsizei bitmap_height = height;

   /* Update draw buffer bounds */
   _mesa_update_state(ctx);

   if (ctx->Depth.Test) {
      /* The blit path produces incorrect results when depth testing is on.
       * It seems the blit Z coord is always 1.0 (the far plane) so fragments
       * will likely be obscured by other, closer geometry.
       */
      return GL_FALSE;
   }

   if (!dst)
       return GL_FALSE;

   if (unpack->BufferObj->Name) {
      bitmap = map_pbo(ctx, width, height, unpack, bitmap);
      if (bitmap == NULL)
	 return GL_TRUE;	/* even though this is an error, we're done */
   }

   COPY_4V(tmpColor, ctx->Current.RasterColor);

   if (NEED_SECONDARY_COLOR(ctx)) {
       ADD_3V(tmpColor, tmpColor, ctx->Current.RasterSecondaryColor);
   }

   UNCLAMPED_FLOAT_TO_UBYTE(ubcolor[0], tmpColor[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(ubcolor[1], tmpColor[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(ubcolor[2], tmpColor[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(ubcolor[3], tmpColor[3]);

   color8888 = INTEL_PACKCOLOR8888(ubcolor[0], ubcolor[1], ubcolor[2], ubcolor[3]);
   color565 = INTEL_PACKCOLOR565(ubcolor[0], ubcolor[1], ubcolor[2]);

   if (!intel_check_blit_fragment_ops(ctx, tmpColor[3] == 1.0F))
      return GL_FALSE;

   LOCK_HARDWARE(intel);

   intel_get_cliprects(intel, &cliprects, &num_cliprects, &x_off, &y_off);
   if (num_cliprects != 0) {
      GLuint i;
      GLint orig_dstx = dstx;
      GLint orig_dsty = dsty;

      /* Clip to buffer bounds and scissor. */
      if (!_mesa_clip_to_region(fb->_Xmin, fb->_Ymin,
				fb->_Xmax, fb->_Ymax,
				&dstx, &dsty, &width, &height))
            goto out;

      dstx = x_off + dstx;
      dsty = y_off + y_flip(fb, dsty, height);

      for (i = 0; i < num_cliprects; i++) {
	 int box_x, box_y, box_w, box_h;
	 GLint px, py;
	 GLuint stipple[32];  

	 box_x = dstx;
	 box_y = dsty;
	 box_w = width;
	 box_h = height;

	 /* Clip to drawable cliprect */
         if (!_mesa_clip_to_region(cliprects[i].x1,
				   cliprects[i].y1,
				   cliprects[i].x2,
				   cliprects[i].y2,
				   &box_x, &box_y, &box_w, &box_h))
	    continue;

#define DY 32
#define DX 32

	 /* Then, finally, chop it all into chunks that can be
	  * digested by hardware:
	  */
	 for (py = 0; py < box_h; py += DY) { 
	    for (px = 0; px < box_w; px += DX) { 
	       int h = MIN2(DY, box_h - py);
	       int w = MIN2(DX, box_w - px); 
	       GLuint sz = ALIGN(ALIGN(w,8) * h, 64)/8;
	       GLenum logic_op = ctx->Color.ColorLogicOpEnabled ?
		  ctx->Color.LogicOp : GL_COPY;

	       assert(sz <= sizeof(stipple));
	       memset(stipple, 0, sz);

	       /* May need to adjust this when padding has been introduced in
		* sz above:
		*
		* Have to translate destination coordinates back into source
		* coordinates.
		*/
	       if (get_bitmap_rect(bitmap_width, bitmap_height, unpack,
				   bitmap,
				   -orig_dstx + (box_x + px - x_off),
				   -orig_dsty + y_flip(fb,
						       box_y + py - y_off, h),
				   w, h,
				   (GLubyte *)stipple,
				   8,
				   fb->Name == 0 ? GL_TRUE : GL_FALSE) == 0)
		  continue;

	       /* 
		*/
	       intelEmitImmediateColorExpandBlit( intel,
						  dst->cpp,
						  (GLubyte *)stipple, 
						  sz,
						  (dst->cpp == 2) ? color565 : color8888,
						  dst->pitch,
						  dst->buffer,
						  0,
						  dst->tiling,
						  box_x + px,
						  box_y + py,
						  w, h,
						  logic_op);
	    } 
	 } 
      }
   }
out:
   UNLOCK_HARDWARE(intel);

   if (INTEL_DEBUG & DEBUG_SYNC)
      intel_batchbuffer_flush(intel->batch);

   if (unpack->BufferObj->Name) {
      /* done with PBO so unmap it now */
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                              unpack->BufferObj);
   }

   return GL_TRUE;
}

static GLboolean
intel_texture_bitmap(GLcontext * ctx,
		     GLint dst_x, GLint dst_y,
		     GLsizei width, GLsizei height,
		     const struct gl_pixelstore_attrib *unpack,
		     const GLubyte *bitmap)
{
   struct intel_context *intel = intel_context(ctx);
   static const char *fp =
      "!!ARBfp1.0\n"
      "TEMP val;\n"
      "PARAM color=program.local[0];\n"
      "TEX val, fragment.texcoord[0], texture[0], 2D;\n"
      "ADD val, val.wwww, {-.5, -.5, -.5, -.5};\n"
      "KIL val;\n"
      "MOV result.color, color;\n"
      "END\n";
   GLuint texname;
   GLfloat vertices[4][4];
   GLfloat texcoords[4][2];
   GLint old_active_texture;
   GLubyte *unpacked_bitmap;
   GLubyte *a8_bitmap;
   int x, y;
   GLfloat dst_z;

   /* We need a fragment program for the KIL effect */
   if (!ctx->Extensions.ARB_fragment_program ||
       !ctx->Extensions.ARB_vertex_program) {
      if (INTEL_DEBUG & DEBUG_FALLBACKS)
	 fprintf(stderr,
		 "glBitmap fallback: No fragment/vertex program support\n");
      return GL_FALSE;
   }

   /* We're going to mess with texturing with no regard to existing texture
    * state, so if there is some set up we have to bail.
    */
   if (ctx->Texture._EnabledUnits != 0) {
      if (INTEL_DEBUG & DEBUG_FALLBACKS)
	 fprintf(stderr, "glBitmap fallback: texturing enabled\n");
      return GL_FALSE;
   }

   /* Can't do textured DrawPixels with a fragment program, unless we were
    * to generate a new program that sampled our texture and put the results
    * in the fragment color before the user's program started.
    */
   if (ctx->FragmentProgram.Enabled) {
      if (INTEL_DEBUG & DEBUG_FALLBACKS)
	 fprintf(stderr, "glBitmap fallback: fragment program enabled\n");
      return GL_FALSE;
   }

   if (ctx->VertexProgram.Enabled) {
      if (INTEL_DEBUG & DEBUG_FALLBACKS)
	 fprintf(stderr, "glBitmap fallback: vertex program enabled\n");
      return GL_FALSE;
   }

   /* Check that we can load in a texture this big. */
   if (width > (1 << (ctx->Const.MaxTextureLevels - 1)) ||
       height > (1 << (ctx->Const.MaxTextureLevels - 1))) {
      if (INTEL_DEBUG & DEBUG_FALLBACKS)
	 fprintf(stderr, "glBitmap fallback: bitmap too large (%dx%d)\n",
		 width, height);
      return GL_FALSE;
   }

   /* Convert the A1 bitmap to an A8 format suitable for glTexImage */
   if (unpack->BufferObj->Name) {
      bitmap = map_pbo(ctx, width, height, unpack, bitmap);
      if (bitmap == NULL)
	 return GL_TRUE;	/* even though this is an error, we're done */
   }
   unpacked_bitmap = _mesa_unpack_bitmap(width, height, bitmap,
					 unpack);
   a8_bitmap = _mesa_calloc(width * height);
   for (y = 0; y < height; y++) {
      for (x = 0; x < width; x++) {
	 if (test_msb_bit(unpacked_bitmap, ALIGN(width, 8) * y + x))
	    a8_bitmap[y * width + x] = 0xff;
      }
   }
   _mesa_free(unpacked_bitmap);
   if (unpack->BufferObj->Name) {
      /* done with PBO so unmap it now */
      ctx->Driver.UnmapBuffer(ctx, GL_PIXEL_UNPACK_BUFFER_EXT,
                              unpack->BufferObj);
   }

   /* Save GL state before we start setting up our drawing */
   _mesa_PushAttrib(GL_ENABLE_BIT | GL_CURRENT_BIT |
		    GL_VIEWPORT_BIT);
   _mesa_PushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT |
			  GL_CLIENT_PIXEL_STORE_BIT);
   old_active_texture = ctx->Texture.CurrentUnit;

   _mesa_Disable(GL_POLYGON_STIPPLE);

   /* Upload our bitmap data to an alpha texture */
   _mesa_ActiveTextureARB(GL_TEXTURE0_ARB);
   _mesa_Enable(GL_TEXTURE_2D);
   _mesa_GenTextures(1, &texname);
   _mesa_BindTexture(GL_TEXTURE_2D, texname);
   _mesa_TexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   _mesa_TexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

   _mesa_PixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
   _mesa_PixelStorei(GL_UNPACK_LSB_FIRST, GL_FALSE);
   _mesa_PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
   _mesa_PixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
   _mesa_PixelStorei(GL_UNPACK_SKIP_ROWS, 0);
   _mesa_PixelStorei(GL_UNPACK_ALIGNMENT, 1);
   _mesa_TexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, width, height, 0,
		    GL_ALPHA, GL_UNSIGNED_BYTE, a8_bitmap);
   _mesa_free(a8_bitmap);

   intel_meta_set_fragment_program(intel, &intel->meta.bitmap_fp, fp);
   _mesa_ProgramLocalParameter4fvARB(GL_FRAGMENT_PROGRAM_ARB, 0,
				     ctx->Current.RasterColor);
   intel_meta_set_passthrough_vertex_program(intel);
   intel_meta_set_passthrough_transform(intel);

   /* convert rasterpos Z from [0,1] to NDC coord in [-1,1] */
   dst_z = -1.0 + 2.0 * ctx->Current.RasterPos[2];

   vertices[0][0] = dst_x;
   vertices[0][1] = dst_y;
   vertices[0][2] = dst_z;
   vertices[0][3] = 1.0;
   vertices[1][0] = dst_x + width;
   vertices[1][1] = dst_y;
   vertices[1][2] = dst_z;
   vertices[1][3] = 1.0;
   vertices[2][0] = dst_x + width;
   vertices[2][1] = dst_y + height;
   vertices[2][2] = dst_z;
   vertices[2][3] = 1.0;
   vertices[3][0] = dst_x;
   vertices[3][1] = dst_y + height;
   vertices[3][2] = dst_z;
   vertices[3][3] = 1.0;

   texcoords[0][0] = 0.0;
   texcoords[0][1] = 0.0;
   texcoords[1][0] = 1.0;
   texcoords[1][1] = 0.0;
   texcoords[2][0] = 1.0;
   texcoords[2][1] = 1.0;
   texcoords[3][0] = 0.0;
   texcoords[3][1] = 1.0;

   _mesa_VertexPointer(4, GL_FLOAT, 4 * sizeof(GLfloat), &vertices);
   _mesa_ClientActiveTextureARB(GL_TEXTURE0);
   _mesa_TexCoordPointer(2, GL_FLOAT, 2 * sizeof(GLfloat), &texcoords);
   _mesa_Enable(GL_VERTEX_ARRAY);
   _mesa_Enable(GL_TEXTURE_COORD_ARRAY);
   CALL_DrawArrays(ctx->Exec, (GL_TRIANGLE_FAN, 0, 4));

   intel_meta_restore_transform(intel);
   intel_meta_restore_fragment_program(intel);
   intel_meta_restore_vertex_program(intel);

   _mesa_PopClientAttrib();
   _mesa_Disable(GL_TEXTURE_2D); /* asserted that it was disabled at entry */
   _mesa_ActiveTextureARB(GL_TEXTURE0_ARB + old_active_texture);
   _mesa_PopAttrib();

   _mesa_DeleteTextures(1, &texname);

   return GL_TRUE;
}

/* There are a large number of possible ways to implement bitmap on
 * this hardware, most of them have some sort of drawback.  Here are a
 * few that spring to mind:
 * 
 * Blit:
 *    - XY_MONO_SRC_BLT_CMD
 *         - use XY_SETUP_CLIP_BLT for cliprect clipping.
 *    - XY_TEXT_BLT
 *    - XY_TEXT_IMMEDIATE_BLT
 *         - blit per cliprect, subject to maximum immediate data size.
 *    - XY_COLOR_BLT 
 *         - per pixel or run of pixels
 *    - XY_PIXEL_BLT
 *         - good for sparse bitmaps
 *
 * 3D engine:
 *    - Point per pixel
 *    - Translate bitmap to an alpha texture and render as a quad
 *    - Chop bitmap up into 32x32 squares and render w/polygon stipple.
 */
void
intelBitmap(GLcontext * ctx,
	    GLint x, GLint y,
	    GLsizei width, GLsizei height,
	    const struct gl_pixelstore_attrib *unpack,
	    const GLubyte * pixels)
{
   if (do_blit_bitmap(ctx, x, y, width, height,
                          unpack, pixels))
      return;

   if (intel_texture_bitmap(ctx, x, y, width, height,
			    unpack, pixels))
      return;

   if (INTEL_DEBUG & DEBUG_PIXEL)
      _mesa_printf("%s: fallback to swrast\n", __FUNCTION__);

   _swrast_Bitmap(ctx, x, y, width, height, unpack, pixels);
}
