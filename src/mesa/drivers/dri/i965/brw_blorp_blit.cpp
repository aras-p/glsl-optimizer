/*
 * Copyright © 2012 Intel Corporation
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

#include "main/teximage.h"

#include "glsl/ralloc.h"

#include "intel_fbo.h"

#include "brw_blorp.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_state.h"


/**
 * Helper function for handling mirror image blits.
 *
 * If coord0 > coord1, swap them and invert the "mirror" boolean.
 */
static inline void
fixup_mirroring(bool &mirror, GLint &coord0, GLint &coord1)
{
   if (coord0 > coord1) {
      mirror = !mirror;
      GLint tmp = coord0;
      coord0 = coord1;
      coord1 = tmp;
   }
}


static bool
try_blorp_blit(struct intel_context *intel,
               GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
               GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
               GLenum filter, GLbitfield buffer_bit)
{
   struct gl_context *ctx = &intel->ctx;

   /* Sync up the state of window system buffers.  We need to do this before
    * we go looking for the buffers.
    */
   intel_prepare_render(intel);

   /* Find buffers */
   const struct gl_framebuffer *read_fb = ctx->ReadBuffer;
   const struct gl_framebuffer *draw_fb = ctx->DrawBuffer;
   struct gl_renderbuffer *src_rb;
   struct gl_renderbuffer *dst_rb;
   switch (buffer_bit) {
   case GL_COLOR_BUFFER_BIT:
      src_rb = read_fb->_ColorReadBuffer;
      dst_rb =
         draw_fb->Attachment[
            draw_fb->_ColorDrawBufferIndexes[0]].Renderbuffer;
      break;
   case GL_DEPTH_BUFFER_BIT:
      src_rb = read_fb->Attachment[BUFFER_DEPTH].Renderbuffer;
      dst_rb = draw_fb->Attachment[BUFFER_DEPTH].Renderbuffer;
      break;
   case GL_STENCIL_BUFFER_BIT:
      src_rb = read_fb->Attachment[BUFFER_STENCIL].Renderbuffer;
      dst_rb = draw_fb->Attachment[BUFFER_STENCIL].Renderbuffer;
      break;
   default:
      assert(false);
   }

   /* Validate source */
   if (!src_rb) return false;
   struct intel_renderbuffer *src_irb = intel_renderbuffer(src_rb);
   struct intel_mipmap_tree *src_mt = src_irb->mt;
   if (!src_mt) return false;
   if (buffer_bit == GL_STENCIL_BUFFER_BIT && src_mt->stencil_mt)
      src_mt = src_mt->stencil_mt;
   switch (src_mt->format) {
   case MESA_FORMAT_ARGB8888:
   case MESA_FORMAT_X8_Z24:
   case MESA_FORMAT_S8:
      break; /* Supported */
   default:
      /* Unsupported format.
       *
       * TODO: need to support all formats that are allowed as multisample
       * render targets.
       */
      return false;
   }

   /* Validate destination */
   if (!dst_rb) return false;
   struct intel_renderbuffer *dst_irb = intel_renderbuffer(dst_rb);
   struct intel_mipmap_tree *dst_mt = dst_irb->mt;
   if (!dst_mt) return false;
   if (buffer_bit == GL_STENCIL_BUFFER_BIT && dst_mt->stencil_mt)
      dst_mt = dst_mt->stencil_mt;
   switch (dst_mt->format) {
   case MESA_FORMAT_ARGB8888:
   case MESA_FORMAT_X8_Z24:
   case MESA_FORMAT_S8:
      break; /* Supported */
   default:
      /* Unsupported format.
       *
       * TODO: need to support all formats that are allowed as multisample
       * render targets.
       */
      return false;
   }

   /* Account for the fact that in the system framebuffer, the origin is at
    * the lower left.
    */
   if (read_fb->Name == 0) {
      srcY0 = read_fb->Height - srcY0;
      srcY1 = read_fb->Height - srcY1;
   }
   if (draw_fb->Name == 0) {
      dstY0 = draw_fb->Height - dstY0;
      dstY1 = draw_fb->Height - dstY1;
   }

   /* Detect if the blit needs to be mirrored */
   bool mirror_x = false, mirror_y = false;
   fixup_mirroring(mirror_x, srcX0, srcX1);
   fixup_mirroring(mirror_x, dstX0, dstX1);
   fixup_mirroring(mirror_y, srcY0, srcY1);
   fixup_mirroring(mirror_y, dstY0, dstY1);

   /* Make sure width and height match */
   GLsizei width = srcX1 - srcX0;
   GLsizei height = srcY1 - srcY0;
   if (width != dstX1 - dstX0) return false;
   if (height != dstY1 - dstY0) return false;

   /* Make sure width and height don't need to be clipped or scissored.
    * TODO: support clipping and scissoring.
    */
   if (srcX0 < 0 || (GLuint) srcX1 > read_fb->Width) return false;
   if (srcY0 < 0 || (GLuint) srcY1 > read_fb->Height) return false;
   if (dstX0 < 0 || (GLuint) dstX1 > draw_fb->Width) return false;
   if (dstY0 < 0 || (GLuint) dstY1 > draw_fb->Height) return false;
   if (ctx->Scissor.Enabled) return false;

   /* Get ready to blit.  This includes depth resolving the src and dst
    * buffers if necessary.
    */
   intel_renderbuffer_resolve_depth(intel, src_irb);
   intel_renderbuffer_resolve_depth(intel, dst_irb);

   /* Do the blit */
   brw_blorp_blit_params params(src_mt, dst_mt,
                                srcX0, srcY0, dstX0, dstY0, dstX1, dstY1,
                                mirror_x, mirror_y);
   params.exec(intel);

   /* Mark the dst buffer as needing a HiZ resolve if necessary. */
   intel_renderbuffer_set_needs_hiz_resolve(dst_irb);

   return true;
}

GLbitfield
brw_blorp_framebuffer(struct intel_context *intel,
                      GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1,
                      GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
                      GLbitfield mask, GLenum filter)
{
   /* BLORP is only supported on Gen6.  TODO: implement on Gen7. */
   if (intel->gen != 6)
      return mask;

   static GLbitfield buffer_bits[] = {
      GL_COLOR_BUFFER_BIT,
      GL_DEPTH_BUFFER_BIT,
      GL_STENCIL_BUFFER_BIT,
   };

   for (unsigned int i = 0; i < ARRAY_SIZE(buffer_bits); ++i) {
      if ((mask & buffer_bits[i]) &&
       try_blorp_blit(intel,
                      srcX0, srcY0, srcX1, srcY1,
                      dstX0, dstY0, dstX1, dstY1,
                      filter, buffer_bits[i])) {
         mask &= ~buffer_bits[i];
      }
   }

   return mask;
}

/**
 * Generator for WM programs used in BLORP blits.
 *
 * The bulk of the work done by the WM program is to wrap and unwrap the
 * coordinate transformations used by the hardware to store surfaces in
 * memory.  The hardware transforms a pixel location (X, Y) to a memory offset
 * by the following formulas:
 *
 *   offset = tile(tiling_format, X, Y)
 *   (X, Y) = detile(tiling_format, offset)
 *
 * For X tiling, tile() combines together the low-order bits of the X and Y
 * coordinates in the pattern 0byyyxxxxxxxxx, creating 4k tiles that are 512
 * bytes wide and 8 rows high:
 *
 *   tile(x_tiled, X, Y) = A
 *     where A = tile_num << 12 | offset
 *           tile_num = (Y >> 3) * tile_pitch + (X' >> 9)
 *           offset = (Y & 0b111) << 9
 *                    | (X & 0b111111111)
 *           X' = X * cpp
 *   detile(x_tiled, A) = (X, Y)
 *     where X = X' / cpp
 *           Y = (tile_num / tile_pitch) << 3
 *               | (A & 0b111000000000) >> 9
 *           X' = (tile_num % tile_pitch) << 9
 *                | (A & 0b111111111)
 *
 * (In all tiling formulas, cpp is the number of bytes occupied by a single
 * pixel ("chars per pixel"), and tile_pitch is the number of 4k tiles
 * required to fill the width of the surface).
 *
 * For Y tiling, tile() combines together the low-order bits of the X and Y
 * coordinates in the pattern 0bxxxyyyyyxxxx, creating 4k tiles that are 128
 * bytes wide and 32 rows high:
 *
 *   tile(y_tiled, X, Y) = A
 *     where A = tile_num << 12 | offset
 *           tile_num = (Y >> 5) * tile_pitch + (X' >> 7)
 *           offset = (X' & 0b1110000) << 5
 *                    | (Y' & 0b11111) << 4
 *                    | (X' & 0b1111)
 *           X' = X * cpp
 *   detile(y_tiled, A) = (X, Y)
 *     where X = X' / cpp
 *           Y = (tile_num / tile_pitch) << 5
 *               | (A & 0b111110000) >> 4
 *           X' = (tile_num % tile_pitch) << 7
 *                | (A & 0b111000000000) >> 5
 *                | (A & 0b1111)
 *
 * For W tiling, tile() combines together the low-order bits of the X and Y
 * coordinates in the pattern 0bxxxyyyyxyxyx, creating 4k tiles that are 64
 * bytes wide and 64 rows high (note that W tiling is only used for stencil
 * buffers, which always have cpp = 1):
 *
 *   tile(w_tiled, X, Y) = A
 *     where A = tile_num << 12 | offset
 *           tile_num = (Y >> 6) * tile_pitch + (X' >> 6)
 *           offset = (X' & 0b111000) << 6
 *                    | (Y & 0b111100) << 3
 *                    | (X' & 0b100) << 2
 *                    | (Y & 0b10) << 2
 *                    | (X' & 0b10) << 1
 *                    | (Y & 0b1) << 1
 *                    | (X' & 0b1)
 *           X' = X * cpp = X
 *   detile(w_tiled, A) = (X, Y)
 *     where X = X' / cpp = X'
 *           Y = (tile_num / tile_pitch) << 6
 *               | (A & 0b111100000) >> 3
 *               | (A & 0b1000) >> 2
 *               | (A & 0b10) >> 1
 *           X' = (tile_num % tile_pitch) << 6
 *                | (A & 0b111000000000) >> 6
 *                | (A & 0b10000) >> 2
 *                | (A & 0b100) >> 1
 *                | (A & 0b1)
 *
 * Finally, for a non-tiled surface, tile() simply combines together the X and
 * Y coordinates in the natural way:
 *
 *   tile(untiled, X, Y) = A
 *     where A = Y * pitch + X'
 *           X' = X * cpp
 *   detile(untiled, A) = (X, Y)
 *     where X = X' / cpp
 *           Y = A / pitch
 *           X' = A % pitch
 *
 * (In these formulas, pitch is the number of bytes occupied by a single row
 * of pixels).
 */
class brw_blorp_blit_program
{
public:
   brw_blorp_blit_program(struct brw_context *brw,
                          const brw_blorp_blit_prog_key *key);
   ~brw_blorp_blit_program();

   const GLuint *compile(struct brw_context *brw, GLuint *program_size);

   brw_blorp_prog_data prog_data;

private:
   void alloc_regs();
   void alloc_push_const_regs(int base_reg);
   void compute_frag_coords();
   void translate_tiling(bool old_tiled_w, bool new_tiled_w);
   void kill_if_outside_dst_rect();
   void translate_dst_to_src();
   void texel_fetch();
   void texture_lookup(GLuint msg_type,
                       struct brw_reg mrf_u, struct brw_reg mrf_v);
   void render_target_write();

   void *mem_ctx;
   struct brw_context *brw;
   const brw_blorp_blit_prog_key *key;
   struct brw_compile func;

   /* Thread dispatch header */
   struct brw_reg R0;

   /* Pixel X/Y coordinates (always in R1). */
   struct brw_reg R1;

   /* Push constants */
   struct brw_reg dst_x0;
   struct brw_reg dst_x1;
   struct brw_reg dst_y0;
   struct brw_reg dst_y1;
   struct {
      struct brw_reg multiplier;
      struct brw_reg offset;
   } x_transform, y_transform;

   /* Data returned from texture lookup (4 vec16's) */
   struct brw_reg Rdata;

   /* X coordinates.  We have two of them so that we can perform coordinate
    * transformations easily.
    */
   struct brw_reg x_coords[2];

   /* Y coordinates.  We have two of them so that we can perform coordinate
    * transformations easily.
    */
   struct brw_reg y_coords[2];

   /* Which element of x_coords and y_coords is currently in use.
    */
   int xy_coord_index;

   /* Temporaries */
   struct brw_reg t1;
   struct brw_reg t2;

   /* M2-3: u coordinate */
   GLuint base_mrf;
   struct brw_reg mrf_u_float;

   /* M4-5: v coordinate */
   struct brw_reg mrf_v_float;
};

brw_blorp_blit_program::brw_blorp_blit_program(
      struct brw_context *brw,
      const brw_blorp_blit_prog_key *key)
   : mem_ctx(ralloc_context(NULL)),
     brw(brw),
     key(key)
{
   brw_init_compile(brw, &func, mem_ctx);
}

brw_blorp_blit_program::~brw_blorp_blit_program()
{
   ralloc_free(mem_ctx);
}

const GLuint *
brw_blorp_blit_program::compile(struct brw_context *brw,
                                GLuint *program_size)
{
   brw_set_compression_control(&func, BRW_COMPRESSION_NONE);

   alloc_regs();
   compute_frag_coords();

   /* Render target and texture hardware don't support W tiling. */
   const bool rt_tiled_w = false;
   const bool tex_tiled_w = false;

   /* The address that data will be written to is determined by the
    * coordinates supplied to the WM thread and the tiling of the render
    * target, according to the formula:
    *
    * (X, Y) = detile(rt_tiling, offset)
    *
    * If the actual tiling of the destination surface is not the same as the
    * configuration of the render target, then these coordinates are wrong and
    * we have to adjust them to compensate for the difference.
    */
   if (rt_tiled_w != key->dst_tiled_w)
      translate_tiling(rt_tiled_w, key->dst_tiled_w);

   /* Now (X, Y) = detile(dst_tiling, offset).
    *
    * That is: X and Y now contain the true coordinates of the data that the
    * WM thread should output.
    *
    * If we need to kill pixels that are outside the destination rectangle,
    * now is the time to do it.
    */

   if (key->use_kill)
      kill_if_outside_dst_rect();

   /* Next, apply a translation to obtain coordinates in the source image. */
   translate_dst_to_src();

   /* X and Y are now the coordinates of the pixel in the source image that we
    * want to texture from.
    *
    * The address that we want to fetch from is
    * related to the X and Y values according to the formula:
    *
    * (X, Y) = detile(src_tiling, offset).
    *
    * If the actual tiling of the source surface is not the same as the
    * configuration of the texture, then we need to adjust the coordinates to
    * compensate for the difference.
    */
   if (tex_tiled_w != key->src_tiled_w)
      translate_tiling(key->src_tiled_w, tex_tiled_w);

   /* Now (X, Y) = detile(tex_tiling, offset).
    *
    * In other words: X and Y now contain values which, when passed to
    * the texturing unit, will cause data to be read from the correct
    * memory location.  So we can fetch the texel now.
    */
   texel_fetch();

   /* Finally, write the fetched value to the render target and terminate the
    * thread.
    */
   render_target_write();
   return brw_get_program(&func, program_size);
}

void
brw_blorp_blit_program::alloc_push_const_regs(int base_reg)
{
#define CONST_LOC(name) offsetof(brw_blorp_wm_push_constants, name)
#define ALLOC_REG(name) \
   this->name = \
      brw_uw1_reg(BRW_GENERAL_REGISTER_FILE, base_reg, CONST_LOC(name) / 2)

   ALLOC_REG(dst_x0);
   ALLOC_REG(dst_x1);
   ALLOC_REG(dst_y0);
   ALLOC_REG(dst_y1);
   ALLOC_REG(x_transform.multiplier);
   ALLOC_REG(x_transform.offset);
   ALLOC_REG(y_transform.multiplier);
   ALLOC_REG(y_transform.offset);
#undef CONST_LOC
#undef ALLOC_REG
}

void
brw_blorp_blit_program::alloc_regs()
{
   int reg = 0;
   this->R0 = retype(brw_vec8_grf(reg++, 0), BRW_REGISTER_TYPE_UW);
   this->R1 = retype(brw_vec8_grf(reg++, 0), BRW_REGISTER_TYPE_UW);
   prog_data.first_curbe_grf = reg;
   alloc_push_const_regs(reg);
   reg += BRW_BLORP_NUM_PUSH_CONST_REGS;
   this->Rdata = vec16(brw_vec8_grf(reg, 0)); reg += 8;
   for (int i = 0; i < 2; ++i) {
      this->x_coords[i]
         = vec16(retype(brw_vec8_grf(reg++, 0), BRW_REGISTER_TYPE_UW));
      this->y_coords[i]
         = vec16(retype(brw_vec8_grf(reg++, 0), BRW_REGISTER_TYPE_UW));
   }
   this->xy_coord_index = 0;
   this->t1 = vec16(retype(brw_vec8_grf(reg++, 0), BRW_REGISTER_TYPE_UW));
   this->t2 = vec16(retype(brw_vec8_grf(reg++, 0), BRW_REGISTER_TYPE_UW));

   int mrf = 2;
   this->base_mrf = mrf;
   this->mrf_u_float = vec16(brw_message_reg(mrf)); mrf += 2;
   this->mrf_v_float = vec16(brw_message_reg(mrf)); mrf += 2;
}

/* In the code that follows, X and Y can be used to quickly refer to the
 * active elements of x_coords and y_coords, and Xp and Yp ("X prime" and "Y
 * prime") to the inactive elements.
 */
#define X x_coords[xy_coord_index]
#define Y y_coords[xy_coord_index]
#define Xp x_coords[!xy_coord_index]
#define Yp y_coords[!xy_coord_index]

/* Quickly swap the roles of (X, Y) and (Xp, Yp).  Saves us from having to do
 * MOVs to transfor (Xp, Yp) to (X, Y) after a coordinate transformation.
 */
#define SWAP_XY_AND_XPYP() xy_coord_index = !xy_coord_index;

/**
 * Emit code to compute the X and Y coordinates of the pixels being rendered
 * by this WM invocation.
 *
 * Assuming the render target is set up for Y tiling, these (X, Y) values are
 * related to the address offset where outputs will be written by the formula:
 *
 *   (X, Y, S) = decode_msaa(detile(offset)).
 *
 * (See brw_blorp_blit_program).
 */
void
brw_blorp_blit_program::compute_frag_coords()
{
   /* R1.2[15:0] = X coordinate of upper left pixel of subspan 0 (pixel 0)
    * R1.3[15:0] = X coordinate of upper left pixel of subspan 1 (pixel 4)
    * R1.4[15:0] = X coordinate of upper left pixel of subspan 2 (pixel 8)
    * R1.5[15:0] = X coordinate of upper left pixel of subspan 3 (pixel 12)
    *
    * Pixels within a subspan are laid out in this arrangement:
    * 0 1
    * 2 3
    *
    * So, to compute the coordinates of each pixel, we need to read every 2nd
    * 16-bit value (vstride=2) from R1, starting at the 4th 16-bit value
    * (suboffset=4), and duplicate each value 4 times (hstride=0, width=4).
    * In other words, the data we want to access is R1.4<2;4,0>UW.
    *
    * Then, we need to add the repeating sequence (0, 1, 0, 1, ...) to the
    * result, since pixels n+1 and n+3 are in the right half of the subspan.
    */
   brw_ADD(&func, X, stride(suboffset(R1, 4), 2, 4, 0), brw_imm_v(0x10101010));

   /* Similarly, Y coordinates for subspans come from R1.2[31:16] through
    * R1.5[31:16], so to get pixel Y coordinates we need to start at the 5th
    * 16-bit value instead of the 4th (R1.5<2;4,0>UW instead of
    * R1.4<2;4,0>UW).
    *
    * And we need to add the repeating sequence (0, 0, 1, 1, ...), since
    * pixels n+2 and n+3 are in the bottom half of the subspan.
    */
   brw_ADD(&func, Y, stride(suboffset(R1, 5), 2, 4, 0), brw_imm_v(0x11001100));
}

/**
 * Emit code to compensate for the difference between Y and W tiling.
 *
 * This code modifies the X and Y coordinates according to the formula:
 *
 *   (X', Y') = detile(new_tiling, tile(old_tiling, X, Y))
 *
 * (See brw_blorp_blit_program).
 *
 * It can only translate between W and Y tiling, so new_tiling and old_tiling
 * are booleans where true represents W tiling and false represents Y tiling.
 */
void
brw_blorp_blit_program::translate_tiling(bool old_tiled_w, bool new_tiled_w)
{
   if (old_tiled_w == new_tiled_w)
      return;

   if (new_tiled_w) {
      /* Given X and Y coordinates that describe an address using Y tiling,
       * translate to the X and Y coordinates that describe the same address
       * using W tiling.
       *
       * If we break down the low order bits of X and Y, using a
       * single letter to represent each low-order bit:
       *
       *   X = A << 7 | 0bBCDEFGH
       *   Y = J << 5 | 0bKLMNP                                       (1)
       *
       * Then we can apply the Y tiling formula to see the memory offset being
       * addressed:
       *
       *   offset = (J * tile_pitch + A) << 12 | 0bBCDKLMNPEFGH       (2)
       *
       * If we apply the W detiling formula to this memory location, that the
       * corresponding X' and Y' coordinates are:
       *
       *   X' = A << 6 | 0bBCDPFH                                     (3)
       *   Y' = J << 6 | 0bKLMNEG
       *
       * Combining (1) and (3), we see that to transform (X, Y) to (X', Y'),
       * we need to make the following computation:
       *
       *   X' = (X & ~0b1011) >> 1 | (Y & 0b1) << 2 | X & 0b1         (4)
       *   Y' = (Y & ~0b1) << 1 | (X & 0b1000) >> 2 | (X & 0b10) >> 1
       */
      brw_AND(&func, t1, X, brw_imm_uw(0xfff4)); /* X & ~0b1011 */
      brw_SHR(&func, t1, t1, brw_imm_uw(1)); /* (X & ~0b1011) >> 1 */
      brw_AND(&func, t2, Y, brw_imm_uw(1)); /* Y & 0b1 */
      brw_SHL(&func, t2, t2, brw_imm_uw(2)); /* (Y & 0b1) << 2 */
      brw_OR(&func, t1, t1, t2); /* (X & ~0b1011) >> 1 | (Y & 0b1) << 2 */
      brw_AND(&func, t2, X, brw_imm_uw(1)); /* X & 0b1 */
      brw_OR(&func, Xp, t1, t2);
      brw_AND(&func, t1, Y, brw_imm_uw(0xfffe)); /* Y & ~0b1 */
      brw_SHL(&func, t1, t1, brw_imm_uw(1)); /* (Y & ~0b1) << 1 */
      brw_AND(&func, t2, X, brw_imm_uw(8)); /* X & 0b1000 */
      brw_SHR(&func, t2, t2, brw_imm_uw(2)); /* (X & 0b1000) >> 2 */
      brw_OR(&func, t1, t1, t2); /* (Y & ~0b1) << 1 | (X & 0b1000) >> 2 */
      brw_AND(&func, t2, X, brw_imm_uw(2)); /* X & 0b10 */
      brw_SHR(&func, t2, t2, brw_imm_uw(1)); /* (X & 0b10) >> 1 */
      brw_OR(&func, Yp, t1, t2);
      SWAP_XY_AND_XPYP();
   } else {
      /* Applying the same logic as above, but in reverse, we obtain the
       * formulas:
       *
       * X' = (X & ~0b101) << 1 | (Y & 0b10) << 2 | (Y & 0b1) << 1 | X & 0b1
       * Y' = (Y & ~0b11) >> 1 | (X & 0b100) >> 2
       */
      brw_AND(&func, t1, X, brw_imm_uw(0xfffa)); /* X & ~0b101 */
      brw_SHL(&func, t1, t1, brw_imm_uw(1)); /* (X & ~0b101) << 1 */
      brw_AND(&func, t2, Y, brw_imm_uw(2)); /* Y & 0b10 */
      brw_SHL(&func, t2, t2, brw_imm_uw(2)); /* (Y & 0b10) << 2 */
      brw_OR(&func, t1, t1, t2); /* (X & ~0b101) << 1 | (Y & 0b10) << 2 */
      brw_AND(&func, t2, Y, brw_imm_uw(1)); /* Y & 0b1 */
      brw_SHL(&func, t2, t2, brw_imm_uw(1)); /* (Y & 0b1) << 1 */
      brw_OR(&func, t1, t1, t2); /* (X & ~0b101) << 1 | (Y & 0b10) << 2
                                    | (Y & 0b1) << 1 */
      brw_AND(&func, t2, X, brw_imm_uw(1)); /* X & 0b1 */
      brw_OR(&func, Xp, t1, t2);
      brw_AND(&func, t1, Y, brw_imm_uw(0xfffc)); /* Y & ~0b11 */
      brw_SHR(&func, t1, t1, brw_imm_uw(1)); /* (Y & ~0b11) >> 1 */
      brw_AND(&func, t2, X, brw_imm_uw(4)); /* X & 0b100 */
      brw_SHR(&func, t2, t2, brw_imm_uw(2)); /* (X & 0b100) >> 2 */
      brw_OR(&func, Yp, t1, t2);
      SWAP_XY_AND_XPYP();
   }
}

/**
 * Emit code that kills pixels whose X and Y coordinates are outside the
 * boundary of the rectangle defined by the push constants (dst_x0, dst_y0,
 * dst_x1, dst_y1).
 */
void
brw_blorp_blit_program::kill_if_outside_dst_rect()
{
   struct brw_reg f0 = brw_flag_reg();
   struct brw_reg g1 = retype(brw_vec1_grf(1, 7), BRW_REGISTER_TYPE_UW);
   struct brw_reg null16 = vec16(retype(brw_null_reg(), BRW_REGISTER_TYPE_UW));

   brw_CMP(&func, null16, BRW_CONDITIONAL_GE, X, dst_x0);
   brw_CMP(&func, null16, BRW_CONDITIONAL_GE, Y, dst_y0);
   brw_CMP(&func, null16, BRW_CONDITIONAL_L, X, dst_x1);
   brw_CMP(&func, null16, BRW_CONDITIONAL_L, Y, dst_y1);

   brw_set_predicate_control(&func, BRW_PREDICATE_NONE);
   brw_push_insn_state(&func);
   brw_set_mask_control(&func, BRW_MASK_DISABLE);
   brw_AND(&func, g1, f0, g1);
   brw_pop_insn_state(&func);
}

/**
 * Emit code to translate from destination (X, Y) coordinates to source (X, Y)
 * coordinates.
 */
void
brw_blorp_blit_program::translate_dst_to_src()
{
   brw_MUL(&func, Xp, X, x_transform.multiplier);
   brw_MUL(&func, Yp, Y, y_transform.multiplier);
   brw_ADD(&func, Xp, Xp, x_transform.offset);
   brw_ADD(&func, Yp, Yp, y_transform.offset);
   SWAP_XY_AND_XPYP();
}

/**
 * Emit code to look up a value in the texture using the SAMPLE_LD message
 * (which does a simple texel fetch).
 */
void
brw_blorp_blit_program::texel_fetch()
{
   texture_lookup(GEN5_SAMPLER_MESSAGE_SAMPLE_LD,
                  retype(mrf_u_float, BRW_REGISTER_TYPE_UD),
                  retype(mrf_v_float, BRW_REGISTER_TYPE_UD));
}

void
brw_blorp_blit_program::texture_lookup(GLuint msg_type,
                                       struct brw_reg mrf_u,
                                       struct brw_reg mrf_v)
{
   /* Expand X and Y coordinates from 16 bits to 32 bits. */
   brw_MOV(&func, vec8(mrf_u), vec8(X));
   brw_set_compression_control(&func, BRW_COMPRESSION_2NDHALF);
   brw_MOV(&func, offset(vec8(mrf_u), 1), suboffset(vec8(X), 8));
   brw_set_compression_control(&func, BRW_COMPRESSION_NONE);
   brw_MOV(&func, vec8(mrf_v), vec8(Y));
   brw_set_compression_control(&func, BRW_COMPRESSION_2NDHALF);
   brw_MOV(&func, offset(vec8(mrf_v), 1), suboffset(vec8(Y), 8));
   brw_set_compression_control(&func, BRW_COMPRESSION_NONE);

   brw_SAMPLE(&func,
              retype(Rdata, BRW_REGISTER_TYPE_UW) /* dest */,
              base_mrf /* msg_reg_nr */,
              vec8(mrf_u) /* src0 */,
              BRW_BLORP_TEXTURE_BINDING_TABLE_INDEX,
              0 /* sampler -- ignored for SAMPLE_LD message */,
              WRITEMASK_XYZW,
              msg_type,
              8 /* response_length.  TODO: should be smaller for non-RGBA formats? */,
              4 /* msg_length */,
              0 /* header_present */,
              BRW_SAMPLER_SIMD_MODE_SIMD16,
              BRW_SAMPLER_RETURN_FORMAT_FLOAT32);
}

#undef X
#undef Y
#undef U
#undef V
#undef S
#undef SWAP_XY_AND_XPYP

void
brw_blorp_blit_program::render_target_write()
{
   struct brw_reg mrf_rt_write = vec16(brw_message_reg(base_mrf));
   int mrf_offset = 0;

   /* If we may have killed pixels, then we need to send R0 and R1 in a header
    * so that the render target knows which pixels we killed.
    */
   bool use_header = key->use_kill;
   if (use_header) {
      /* Copy R0/1 to MRF */
      brw_MOV(&func, retype(mrf_rt_write, BRW_REGISTER_TYPE_UD),
              retype(R0, BRW_REGISTER_TYPE_UD));
      mrf_offset += 2;
   }

   /* Copy texture data to MRFs */
   for (int i = 0; i < 4; ++i) {
      /* E.g. mov(16) m2.0<1>:f r2.0<8;8,1>:f { Align1, H1 } */
      brw_MOV(&func, offset(mrf_rt_write, mrf_offset), offset(vec8(Rdata), 2*i));
      mrf_offset += 2;
   }

   /* Now write to the render target and terminate the thread */
   brw_fb_WRITE(&func,
                16 /* dispatch_width */,
                base_mrf /* msg_reg_nr */,
                mrf_rt_write /* src0 */,
                BRW_BLORP_RENDERBUFFER_BINDING_TABLE_INDEX,
                mrf_offset /* msg_length.  TODO: Should be smaller for non-RGBA formats. */,
                0 /* response_length */,
                true /* eot */,
                use_header);
}


void
brw_blorp_coord_transform_params::setup(GLuint src0, GLuint dst0, GLuint dst1,
                                        bool mirror)
{
   if (!mirror) {
      /* When not mirroring a coordinate (say, X), we need:
       *   x' - src_x0 = x - dst_x0
       * Therefore:
       *   x' = 1*x + (src_x0 - dst_x0)
       */
      multiplier = 1;
      offset = src0 - dst0;
   } else {
      /* When mirroring X we need:
       *   x' - src_x0 = dst_x1 - x - 1
       * Therefore:
       *   x' = -1*x + (src_x0 + dst_x1 - 1)
       */
      multiplier = -1;
      offset = src0 + dst1 - 1;
   }
}


brw_blorp_blit_params::brw_blorp_blit_params(struct intel_mipmap_tree *src_mt,
                                             struct intel_mipmap_tree *dst_mt,
                                             GLuint src_x0, GLuint src_y0,
                                             GLuint dst_x0, GLuint dst_y0,
                                             GLuint dst_x1, GLuint dst_y1,
                                             bool mirror_x, bool mirror_y)
{
   src.set(src_mt, 0, 0);
   dst.set(dst_mt, 0, 0);

   use_wm_prog = true;
   memset(&wm_prog_key, 0, sizeof(wm_prog_key));

   wm_prog_key.src_tiled_w = src.map_stencil_as_y_tiled;
   wm_prog_key.dst_tiled_w = dst.map_stencil_as_y_tiled;
   x0 = wm_push_consts.dst_x0 = dst_x0;
   y0 = wm_push_consts.dst_y0 = dst_y0;
   x1 = wm_push_consts.dst_x1 = dst_x1;
   y1 = wm_push_consts.dst_y1 = dst_y1;
   wm_push_consts.x_transform.setup(src_x0, dst_x0, dst_x1, mirror_x);
   wm_push_consts.y_transform.setup(src_y0, dst_y0, dst_y1, mirror_y);

   if (dst.map_stencil_as_y_tiled) {
      /* We must modify the rectangle we send through the rendering pipeline,
       * to account for the fact that we are mapping it as Y-tiled when it is
       * in fact W-tiled.  Y tiles have dimensions 128x32 whereas W tiles have
       * dimensions 64x64.  We must also align it to a multiple of the tile
       * size, because the differences between W and Y tiling formats will
       * mean that pixels are scrambled within the tile.
       * TODO: what if this makes the coordinates too large?
       */
      x0 = (x0 * 2) & ~127;
      y0 = (y0 / 2) & ~31;
      x1 = ALIGN(x1 * 2, 128);
      y1 = ALIGN(y1 / 2, 32);
      wm_prog_key.use_kill = true;
   }
}

uint32_t
brw_blorp_blit_params::get_wm_prog(struct brw_context *brw,
                                   brw_blorp_prog_data **prog_data) const
{
   uint32_t prog_offset;
   if (!brw_search_cache(&brw->cache, BRW_BLORP_BLIT_PROG,
                         &this->wm_prog_key, sizeof(this->wm_prog_key),
                         &prog_offset, prog_data)) {
      brw_blorp_blit_program prog(brw, &this->wm_prog_key);
      GLuint program_size;
      const GLuint *program = prog.compile(brw, &program_size);
      brw_upload_cache(&brw->cache, BRW_BLORP_BLIT_PROG,
                       &this->wm_prog_key, sizeof(this->wm_prog_key),
                       program, program_size,
                       &prog.prog_data, sizeof(prog.prog_data),
                       &prog_offset, prog_data);
   }
   return prog_offset;
}
