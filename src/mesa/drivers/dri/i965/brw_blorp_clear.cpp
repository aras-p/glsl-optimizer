/*
 * Copyright © 2013 Intel Corporation
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

extern "C" {
#include "main/teximage.h"
#include "main/blend.h"
#include "main/fbobject.h"
#include "main/renderbuffer.h"
}

#include "glsl/ralloc.h"

#include "intel_fbo.h"

#include "brw_blorp.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_state.h"

#define FILE_DEBUG_FLAG DEBUG_BLORP

struct brw_blorp_const_color_prog_key
{
   bool use_simd16_replicated_data;
   bool pad[3];
};

/**
 * Parameters for a blorp operation where the fragment shader outputs a
 * constant color.  This is used for both fast color clears and color
 * resolves.
 */
class brw_blorp_const_color_params : public brw_blorp_params
{
public:
   virtual uint32_t get_wm_prog(struct brw_context *brw,
                                brw_blorp_prog_data **prog_data) const;

protected:
   brw_blorp_const_color_prog_key wm_prog_key;
};

class brw_blorp_clear_params : public brw_blorp_const_color_params
{
public:
   brw_blorp_clear_params(struct brw_context *brw,
                          struct gl_framebuffer *fb,
                          struct gl_renderbuffer *rb,
                          GLubyte *color_mask,
                          bool partial_clear);
};


/**
 * Parameters for a blorp operation that performs a "render target resolve".
 * This is used to resolve pending fast clear pixels before a color buffer is
 * used for texturing, ReadPixels, or scanout.
 */
class brw_blorp_rt_resolve_params : public brw_blorp_const_color_params
{
public:
   brw_blorp_rt_resolve_params(struct brw_context *brw,
                               struct intel_mipmap_tree *mt);
};


class brw_blorp_const_color_program
{
public:
   brw_blorp_const_color_program(struct brw_context *brw,
                                 const brw_blorp_const_color_prog_key *key);
   ~brw_blorp_const_color_program();

   const GLuint *compile(struct brw_context *brw, GLuint *program_size);

   brw_blorp_prog_data prog_data;

private:
   void alloc_regs();

   void *mem_ctx;
   struct brw_context *brw;
   const brw_blorp_const_color_prog_key *key;
   struct brw_compile func;

   /* Thread dispatch header */
   struct brw_reg R0;

   /* Pixel X/Y coordinates (always in R1). */
   struct brw_reg R1;

   /* Register with push constants (a single vec4) */
   struct brw_reg clear_rgba;

   /* MRF used for render target writes */
   GLuint base_mrf;
};

brw_blorp_const_color_program::brw_blorp_const_color_program(
      struct brw_context *brw,
      const brw_blorp_const_color_prog_key *key)
   : mem_ctx(ralloc_context(NULL)),
     brw(brw),
     key(key),
     R0(),
     R1(),
     clear_rgba(),
     base_mrf(0)
{
   brw_init_compile(brw, &func, mem_ctx);
}

brw_blorp_const_color_program::~brw_blorp_const_color_program()
{
   ralloc_free(mem_ctx);
}


/**
 * Determine if fast color clear supports the given clear color.
 *
 * Fast color clear can only clear to color values of 1.0 or 0.0.  At the
 * moment we only support floating point, unorm, and snorm buffers.
 */
static bool
is_color_fast_clear_compatible(struct brw_context *brw,
                               gl_format format,
                               const union gl_color_union *color)
{
   if (_mesa_is_format_integer_color(format))
      return false;

   for (int i = 0; i < 4; i++) {
      if (color->f[i] != 0.0 && color->f[i] != 1.0) {
         perf_debug("Clear color unsupported by fast color clear.  "
                    "Falling back to slow clear.\n");
         return false;
      }
   }
   return true;
}


/**
 * Convert the given color to a bitfield suitable for ORing into DWORD 7 of
 * SURFACE_STATE.
 */
static uint32_t
compute_fast_clear_color_bits(const union gl_color_union *color)
{
   uint32_t bits = 0;
   for (int i = 0; i < 4; i++) {
      if (color->f[i] != 0.0)
         bits |= 1 << (GEN7_SURFACE_CLEAR_COLOR_SHIFT + (3 - i));
   }
   return bits;
}


brw_blorp_clear_params::brw_blorp_clear_params(struct brw_context *brw,
                                               struct gl_framebuffer *fb,
                                               struct gl_renderbuffer *rb,
                                               GLubyte *color_mask,
                                               bool partial_clear)
{
   struct gl_context *ctx = &brw->ctx;
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);

   dst.set(brw, irb->mt, irb->mt_level, irb->mt_layer);

   /* Override the surface format according to the context's sRGB rules. */
   gl_format format = _mesa_get_render_format(ctx, irb->mt->format);
   dst.brw_surfaceformat = brw->render_target_format[format];

   x0 = fb->_Xmin;
   x1 = fb->_Xmax;
   if (rb->Name != 0) {
      y0 = fb->_Ymin;
      y1 = fb->_Ymax;
   } else {
      y0 = rb->Height - fb->_Ymax;
      y1 = rb->Height - fb->_Ymin;
   }

   float *push_consts = (float *)&wm_push_consts;

   push_consts[0] = ctx->Color.ClearColor.f[0];
   push_consts[1] = ctx->Color.ClearColor.f[1];
   push_consts[2] = ctx->Color.ClearColor.f[2];
   push_consts[3] = ctx->Color.ClearColor.f[3];

   use_wm_prog = true;

   memset(&wm_prog_key, 0, sizeof(wm_prog_key));

   wm_prog_key.use_simd16_replicated_data = true;

   /* From the SNB PRM (Vol4_Part1):
    *
    *     "Replicated data (Message Type = 111) is only supported when
    *      accessing tiled memory.  Using this Message Type to access linear
    *      (untiled) memory is UNDEFINED."
    */
   if (irb->mt->region->tiling == I915_TILING_NONE)
      wm_prog_key.use_simd16_replicated_data = false;

   /* Constant color writes ignore everyting in blend and color calculator
    * state.  This is not documented.
    */
   for (int i = 0; i < 4; i++) {
      if (!color_mask[i]) {
         color_write_disable[i] = true;
         wm_prog_key.use_simd16_replicated_data = false;
      }
   }

   /* If we can do this as a fast color clear, do so. */
   if (irb->mt->mcs_state != INTEL_MCS_STATE_NONE && !partial_clear &&
       wm_prog_key.use_simd16_replicated_data &&
       is_color_fast_clear_compatible(brw, format, &ctx->Color.ClearColor)) {
      memset(push_consts, 0xff, 4*sizeof(float));
      fast_clear_op = GEN7_FAST_CLEAR_OP_FAST_CLEAR;

      /* From the Ivy Bridge PRM, Vol2 Part1 11.7 "MCS Buffer for Render
       * Target(s)", beneath the "Fast Color Clear" bullet (p327):
       *
       *     Clear pass must have a clear rectangle that must follow alignment
       *     rules in terms of pixels and lines as shown in the table
       *     below. Further, the clear-rectangle height and width must be
       *     multiple of the following dimensions. If the height and width of
       *     the render target being cleared do not meet these requirements,
       *     an MCS buffer can be created such that it follows the requirement
       *     and covers the RT.
       *
       * The alignment size in the table that follows is related to the
       * alignment size returned by intel_get_non_msrt_mcs_alignment(), but
       * with X alignment multiplied by 16 and Y alignment multiplied by 32.
       */
      unsigned x_align, y_align;
      intel_get_non_msrt_mcs_alignment(brw, irb->mt, &x_align, &y_align);
      x_align *= 16;
      y_align *= 32;

      if (brw->is_haswell && brw->gt == 3) {
         /* From BSpec: 3D-Media-GPGPU Engine > 3D Pipeline > Pixel > Pixel
          * Backend > MCS Buffer for Render Target(s) [DevIVB+]:
          * [DevHSW:GT3]: Clear rectangle must be aligned to two times the
          * number of pixels in the table shown below...
          * x_align, y_align values computed above are the relevant entries
          * in the referred table.
          */
         x0 = ROUND_DOWN_TO(x0, 2 * x_align);
         y0 = ROUND_DOWN_TO(y0, 2 * y_align);
         x1 = ALIGN(x1, 2 * x_align);
         y1 = ALIGN(y1, 2 * y_align);
      } else {
         x0 = ROUND_DOWN_TO(x0,  x_align);
         y0 = ROUND_DOWN_TO(y0, y_align);
         x1 = ALIGN(x1, x_align);
         y1 = ALIGN(y1, y_align);
      }

      /* From the Ivy Bridge PRM, Vol2 Part1 11.7 "MCS Buffer for Render
       * Target(s)", beneath the "Fast Color Clear" bullet (p327):
       *
       *     In order to optimize the performance MCS buffer (when bound to 1X
       *     RT) clear similarly to MCS buffer clear for MSRT case, clear rect
       *     is required to be scaled by the following factors in the
       *     horizontal and vertical directions:
       *
       * The X and Y scale down factors in the table that follows are each
       * equal to half the alignment value computed above.
       */
      unsigned x_scaledown = x_align / 2;
      unsigned y_scaledown = y_align / 2;
      x0 /= x_scaledown;
      y0 /= y_scaledown;
      x1 /= x_scaledown;
      y1 /= y_scaledown;
   }
}


brw_blorp_rt_resolve_params::brw_blorp_rt_resolve_params(
      struct brw_context *brw,
      struct intel_mipmap_tree *mt)
{
   dst.set(brw, mt, 0 /* level */, 0 /* layer */);

   /* From the Ivy Bridge PRM, Vol2 Part1 11.9 "Render Target Resolve":
    *
    *     A rectangle primitive must be scaled down by the following factors
    *     with respect to render target being resolved.
    *
    * The scaledown factors in the table that follows are related to the
    * alignment size returned by intel_get_non_msrt_mcs_alignment(), but with
    * X and Y alignment each divided by 2.
    */
   unsigned x_align, y_align;
   intel_get_non_msrt_mcs_alignment(brw, mt, &x_align, &y_align);
   unsigned x_scaledown = x_align / 2;
   unsigned y_scaledown = y_align / 2;
   x0 = y0 = 0;
   x1 = ALIGN(mt->logical_width0, x_scaledown) / x_scaledown;
   y1 = ALIGN(mt->logical_height0, y_scaledown) / y_scaledown;

   fast_clear_op = GEN7_FAST_CLEAR_OP_RESOLVE;

   /* Note: there is no need to initialize push constants because it doesn't
    * matter what data gets dispatched to the render target.  However, we must
    * ensure that the fragment shader delivers the data using the "replicated
    * color" message.
    */
   use_wm_prog = true;
   memset(&wm_prog_key, 0, sizeof(wm_prog_key));
   wm_prog_key.use_simd16_replicated_data = true;
}


uint32_t
brw_blorp_const_color_params::get_wm_prog(struct brw_context *brw,
                                          brw_blorp_prog_data **prog_data)
   const
{
   uint32_t prog_offset;
   if (!brw_search_cache(&brw->cache, BRW_BLORP_CONST_COLOR_PROG,
                         &this->wm_prog_key, sizeof(this->wm_prog_key),
                         &prog_offset, prog_data)) {
      brw_blorp_const_color_program prog(brw, &this->wm_prog_key);
      GLuint program_size;
      const GLuint *program = prog.compile(brw, &program_size);
      brw_upload_cache(&brw->cache, BRW_BLORP_CONST_COLOR_PROG,
                       &this->wm_prog_key, sizeof(this->wm_prog_key),
                       program, program_size,
                       &prog.prog_data, sizeof(prog.prog_data),
                       &prog_offset, prog_data);
   }
   return prog_offset;
}

void
brw_blorp_const_color_program::alloc_regs()
{
   int reg = 0;
   this->R0 = retype(brw_vec8_grf(reg++, 0), BRW_REGISTER_TYPE_UW);
   this->R1 = retype(brw_vec8_grf(reg++, 0), BRW_REGISTER_TYPE_UW);

   prog_data.first_curbe_grf = reg;
   clear_rgba = retype(brw_vec4_grf(reg++, 0), BRW_REGISTER_TYPE_F);
   reg += BRW_BLORP_NUM_PUSH_CONST_REGS;

   /* Make sure we didn't run out of registers */
   assert(reg <= GEN7_MRF_HACK_START);

   this->base_mrf = 2;
}

const GLuint *
brw_blorp_const_color_program::compile(struct brw_context *brw,
                                       GLuint *program_size)
{
   /* Set up prog_data */
   memset(&prog_data, 0, sizeof(prog_data));
   prog_data.persample_msaa_dispatch = false;

   alloc_regs();

   brw_set_compression_control(&func, BRW_COMPRESSION_NONE);

   struct brw_reg mrf_rt_write =
      retype(vec16(brw_message_reg(base_mrf)), BRW_REGISTER_TYPE_F);

   uint32_t mlen, msg_type;
   if (key->use_simd16_replicated_data) {
      /* The message payload is a single register with the low 4 floats/ints
       * filled with the constant clear color.
       */
      brw_set_mask_control(&func, BRW_MASK_DISABLE);
      brw_MOV(&func, vec4(brw_message_reg(base_mrf)), clear_rgba);
      brw_set_mask_control(&func, BRW_MASK_ENABLE);

      msg_type = BRW_DATAPORT_RENDER_TARGET_WRITE_SIMD16_SINGLE_SOURCE_REPLICATED;
      mlen = 1;
   } else {
      for (int i = 0; i < 4; i++) {
         /* The message payload is pairs of registers for 16 pixels each of r,
          * g, b, and a.
          */
         brw_set_compression_control(&func, BRW_COMPRESSION_COMPRESSED);
         brw_MOV(&func,
                 brw_message_reg(base_mrf + i * 2),
                 brw_vec1_grf(clear_rgba.nr, i));
         brw_set_compression_control(&func, BRW_COMPRESSION_NONE);
      }

      msg_type = BRW_DATAPORT_RENDER_TARGET_WRITE_SIMD16_SINGLE_SOURCE;
      mlen = 8;
   }

   /* Now write to the render target and terminate the thread */
   brw_fb_WRITE(&func,
                16 /* dispatch_width */,
                base_mrf /* msg_reg_nr */,
                mrf_rt_write /* src0 */,
                msg_type,
                BRW_BLORP_RENDERBUFFER_BINDING_TABLE_INDEX,
                mlen,
                0 /* response_length */,
                true /* eot */,
                false /* header present */);

   if (unlikely(INTEL_DEBUG & DEBUG_BLORP)) {
      printf("Native code for BLORP clear:\n");
      brw_dump_compile(&func, stdout, 0, func.next_insn_offset);
      printf("\n");
   }
   return brw_get_program(&func, program_size);
}

extern "C" {
bool
brw_blorp_clear_color(struct brw_context *brw, struct gl_framebuffer *fb,
                      bool partial_clear)
{
   struct gl_context *ctx = &brw->ctx;

   /* The constant color clear code doesn't work for multisampled surfaces, so
    * we need to support falling back to other clear mechanisms.
    * Unfortunately, our clear code is based on a bitmask that doesn't
    * distinguish individual color attachments, so we walk the attachments to
    * see if any require fallback, and fall back for all if any of them need
    * to.
    */
   for (unsigned buf = 0; buf < ctx->DrawBuffer->_NumColorDrawBuffers; buf++) {
      struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[buf];
      struct intel_renderbuffer *irb = intel_renderbuffer(rb);

      if (irb && irb->mt->msaa_layout != INTEL_MSAA_LAYOUT_NONE)
         return false;
   }

   for (unsigned buf = 0; buf < ctx->DrawBuffer->_NumColorDrawBuffers; buf++) {
      struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[buf];
      struct intel_renderbuffer *irb = intel_renderbuffer(rb);

      /* If this is an ES2 context or GL_ARB_ES2_compatibility is supported,
       * the framebuffer can be complete with some attachments missing.  In
       * this case the _ColorDrawBuffers pointer will be NULL.
       */
      if (rb == NULL)
         continue;

      brw_blorp_clear_params params(brw, fb, rb, ctx->Color.ColorMask[buf],
                                    partial_clear);

      bool is_fast_clear =
         (params.fast_clear_op == GEN7_FAST_CLEAR_OP_FAST_CLEAR);
      if (is_fast_clear) {
         /* Record the clear color in the miptree so that it will be
          * programmed in SURFACE_STATE by later rendering and resolve
          * operations.
          */
         uint32_t new_color_value =
            compute_fast_clear_color_bits(&ctx->Color.ClearColor);
         if (irb->mt->fast_clear_color_value != new_color_value) {
            irb->mt->fast_clear_color_value = new_color_value;
            brw->state.dirty.brw |= BRW_NEW_SURFACES;
         }

         /* If the buffer is already in INTEL_MCS_STATE_CLEAR, the clear is
          * redundant and can be skipped.
          */
         if (irb->mt->mcs_state == INTEL_MCS_STATE_CLEAR)
            continue;

         /* If the MCS buffer hasn't been allocated yet, we need to allocate
          * it now.
          */
         if (!irb->mt->mcs_mt) {
            if (!intel_miptree_alloc_non_msrt_mcs(brw, irb->mt)) {
               /* MCS allocation failed--probably this will only happen in
                * out-of-memory conditions.  But in any case, try to recover
                * by falling back to a non-blorp clear technique.
                */
               return false;
            }
            brw->state.dirty.brw |= BRW_NEW_SURFACES;
         }
      }

      DBG("%s to mt %p level %d layer %d\n", __FUNCTION__,
          irb->mt, irb->mt_level, irb->mt_layer);

      brw_blorp_exec(brw, &params);

      if (is_fast_clear) {
         /* Now that the fast clear has occurred, put the buffer in
          * INTEL_MCS_STATE_CLEAR so that we won't waste time doing redundant
          * clears.
          */
         irb->mt->mcs_state = INTEL_MCS_STATE_CLEAR;
      }
   }

   return true;
}

void
brw_blorp_resolve_color(struct brw_context *brw, struct intel_mipmap_tree *mt)
{
   DBG("%s to mt %p\n", __FUNCTION__, mt);

   brw_blorp_rt_resolve_params params(brw, mt);
   brw_blorp_exec(brw, &params);
   mt->mcs_state = INTEL_MCS_STATE_RESOLVED;
}

} /* extern "C" */
