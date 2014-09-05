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

#include "main/mtypes.h"
#include "main/macros.h"
#include "main/context.h"
#include "main/objectlabel.h"
#include "main/shaderapi.h"
#include "main/arrayobj.h"
#include "main/bufferobj.h"
#include "main/buffers.h"
#include "main/blend.h"
#include "main/enable.h"
#include "main/depth.h"
#include "main/stencil.h"
#include "main/varray.h"
#include "main/uniforms.h"
#include "main/fbobject.h"
#include "main/texobj.h"

#include "main/api_validate.h"
#include "main/state.h"

#include "vbo/vbo_context.h"

#include "drivers/common/meta.h"

#include "brw_defines.h"
#include "brw_context.h"
#include "brw_draw.h"
#include "intel_fbo.h"
#include "intel_batchbuffer.h"

#include "brw_blorp.h"

struct brw_fast_clear_state {
   GLuint vao;
   GLuint vbo;
   GLuint shader_prog;
   GLint color_location;
};

static bool
brw_fast_clear_init(struct brw_context *brw)
{
   struct brw_fast_clear_state *clear;

   if (brw->fast_clear_state) {
      clear = brw->fast_clear_state;
      _mesa_BindVertexArray(clear->vao);
      _mesa_BindBuffer(GL_ARRAY_BUFFER, clear->vbo);
      return true;
   }

   brw->fast_clear_state = clear = malloc(sizeof *clear);
   if (clear == NULL)
      return false;

   memset(clear, 0, sizeof *clear);
   _mesa_GenVertexArrays(1, &clear->vao);
   _mesa_BindVertexArray(clear->vao);
   _mesa_GenBuffers(1, &clear->vbo);
   _mesa_BindBuffer(GL_ARRAY_BUFFER, clear->vbo);
   _mesa_VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);
   _mesa_EnableVertexAttribArray(0);

   return true;
}

static void
brw_bind_rep_write_shader(struct brw_context *brw, float *color)
{
   const char *vs_source =
      "#extension GL_AMD_vertex_shader_layer : enable\n"
      "#extension GL_ARB_draw_instanced : enable\n"
      "attribute vec4 position;\n"
      "uniform int layer;\n"
      "void main()\n"
      "{\n"
      "#ifdef GL_AMD_vertex_shader_layer\n"
      "   gl_Layer = gl_InstanceID;\n"
      "#endif\n"
      "   gl_Position = position;\n"
      "}\n";
   const char *fs_source =
      "uniform vec4 color;\n"
      "void main()\n"
      "{\n"
      "   gl_FragColor = color;\n"
      "}\n";

   GLuint vs, fs;
   struct brw_fast_clear_state *clear = brw->fast_clear_state;
   struct gl_context *ctx = &brw->ctx;

   if (clear->shader_prog) {
      _mesa_UseProgram(clear->shader_prog);
      _mesa_Uniform4fv(clear->color_location, 1, color);
      return;
   }

   vs = _mesa_meta_compile_shader_with_debug(ctx, GL_VERTEX_SHADER, vs_source);
   fs = _mesa_meta_compile_shader_with_debug(ctx, GL_FRAGMENT_SHADER, fs_source);

   clear->shader_prog = _mesa_CreateProgram();
   _mesa_AttachShader(clear->shader_prog, fs);
   _mesa_DeleteShader(fs);
   _mesa_AttachShader(clear->shader_prog, vs);
   _mesa_DeleteShader(vs);
   _mesa_BindAttribLocation(clear->shader_prog, 0, "position");
   _mesa_ObjectLabel(GL_PROGRAM, clear->shader_prog, -1, "meta clear");
   _mesa_LinkProgram(clear->shader_prog);

   clear->color_location =
      _mesa_GetUniformLocation(clear->shader_prog, "color");

   _mesa_UseProgram(clear->shader_prog);
   _mesa_Uniform4fv(clear->color_location, 1, color);
}

void
brw_meta_fast_clear_free(struct brw_context *brw)
{
   struct brw_fast_clear_state *clear = brw->fast_clear_state;
   GET_CURRENT_CONTEXT(old_context);

   if (clear == NULL)
      return;

   _mesa_make_current(&brw->ctx, NULL, NULL);

   _mesa_DeleteVertexArrays(1, &clear->vao);
   _mesa_DeleteBuffers(1, &clear->vbo);
   _mesa_DeleteProgram(clear->shader_prog);
   free(clear);

   if (old_context)
      _mesa_make_current(old_context, old_context->WinSysDrawBuffer, old_context->WinSysReadBuffer);
   else
      _mesa_make_current(NULL, NULL, NULL);
}

struct rect {
   int x0, y0, x1, y1;
};

static void
brw_draw_rectlist(struct gl_context *ctx, struct rect *rect, int num_instances)
{
   int start = 0, count = 3;
   struct _mesa_prim prim;
   float verts[6];

   verts[0] = rect->x1;
   verts[1] = rect->y1;
   verts[2] = rect->x0;
   verts[3] = rect->y1;
   verts[4] = rect->x0;
   verts[5] = rect->y0;

   /* upload new vertex data */
   _mesa_BufferData(GL_ARRAY_BUFFER_ARB, sizeof(verts), verts,
                    GL_DYNAMIC_DRAW_ARB);

   if (ctx->NewState)
      _mesa_update_state(ctx);

   vbo_bind_arrays(ctx);

   memset(&prim, 0, sizeof prim);
   prim.begin = 1;
   prim.end = 1;
   prim.mode = BRW_PRIM_OFFSET + _3DPRIM_RECTLIST;
   prim.num_instances = num_instances;
   prim.start = start;
   prim.count = count;

   /* Make sure our internal prim value doesn't clash with a valid GL value. */
   assert(!_mesa_is_valid_prim_mode(ctx, prim.mode));

   brw_draw_prims(ctx, &prim, 1, NULL,
                  GL_TRUE, start, start + count - 1,
                  NULL, NULL);
}

static void
get_fast_clear_rect(struct brw_context *brw, struct gl_framebuffer *fb,
                    struct intel_renderbuffer *irb, struct rect *rect)
{
   unsigned int x_align, y_align;
   unsigned int x_scaledown, y_scaledown;

   if (irb->mt->msaa_layout == INTEL_MSAA_LAYOUT_NONE) {
      /* From the Ivy Bridge PRM, Vol2 Part1 11.7 "MCS Buffer for Render
       * Target(s)", beneath the "Fast Color Clear" bullet (p327):
       *
       *     Clear pass must have a clear rectangle that must follow
       *     alignment rules in terms of pixels and lines as shown in the
       *     table below. Further, the clear-rectangle height and width
       *     must be multiple of the following dimensions. If the height
       *     and width of the render target being cleared do not meet these
       *     requirements, an MCS buffer can be created such that it
       *     follows the requirement and covers the RT.
       *
       * The alignment size in the table that follows is related to the
       * alignment size returned by intel_get_non_msrt_mcs_alignment(), but
       * with X alignment multiplied by 16 and Y alignment multiplied by 32.
       */
      intel_get_non_msrt_mcs_alignment(brw, irb->mt, &x_align, &y_align);
      x_align *= 16;
      y_align *= 32;

      /* From the Ivy Bridge PRM, Vol2 Part1 11.7 "MCS Buffer for Render
       * Target(s)", beneath the "Fast Color Clear" bullet (p327):
       *
       *     In order to optimize the performance MCS buffer (when bound to
       *     1X RT) clear similarly to MCS buffer clear for MSRT case,
       *     clear rect is required to be scaled by the following factors
       *     in the horizontal and vertical directions:
       *
       * The X and Y scale down factors in the table that follows are each
       * equal to half the alignment value computed above.
       */
      x_scaledown = x_align / 2;
      y_scaledown = y_align / 2;

      /* From BSpec: 3D-Media-GPGPU Engine > 3D Pipeline > Pixel > Pixel
       * Backend > MCS Buffer for Render Target(s) [DevIVB+] > Table "Color
       * Clear of Non-MultiSampled Render Target Restrictions":
       *
       *   Clear rectangle must be aligned to two times the number of
       *   pixels in the table shown below due to 16x16 hashing across the
       *   slice.
       */
      x_align *= 2;
      y_align *= 2;
   } else {
      /* From the Ivy Bridge PRM, Vol2 Part1 11.7 "MCS Buffer for Render
       * Target(s)", beneath the "MSAA Compression" bullet (p326):
       *
       *     Clear pass for this case requires that scaled down primitive
       *     is sent down with upper left co-ordinate to coincide with
       *     actual rectangle being cleared. For MSAA, clear rectangle’s
       *     height and width need to as show in the following table in
       *     terms of (width,height) of the RT.
       *
       *     MSAA  Width of Clear Rect  Height of Clear Rect
       *      4X     Ceil(1/8*width)      Ceil(1/2*height)
       *      8X     Ceil(1/2*width)      Ceil(1/2*height)
       *
       * The text "with upper left co-ordinate to coincide with actual
       * rectangle being cleared" is a little confusing--it seems to imply
       * that to clear a rectangle from (x,y) to (x+w,y+h), one needs to
       * feed the pipeline using the rectangle (x,y) to
       * (x+Ceil(w/N),y+Ceil(h/2)), where N is either 2 or 8 depending on
       * the number of samples.  Experiments indicate that this is not
       * quite correct; actually, what the hardware appears to do is to
       * align whatever rectangle is sent down the pipeline to the nearest
       * multiple of 2x2 blocks, and then scale it up by a factor of N
       * horizontally and 2 vertically.  So the resulting alignment is 4
       * vertically and either 4 or 16 horizontally, and the scaledown
       * factor is 2 vertically and either 2 or 8 horizontally.
       */
      switch (irb->mt->num_samples) {
      case 2:
      case 4:
         x_scaledown = 8;
         break;
      case 8:
         x_scaledown = 2;
         break;
      default:
         unreachable("Unexpected sample count for fast clear");
      }
      y_scaledown = 2;
      x_align = x_scaledown * 2;
      y_align = y_scaledown * 2;
   }

   rect->x0 = fb->_Xmin;
   rect->x1 = fb->_Xmax;
   if (fb->Name != 0) {
      rect->y0 = fb->_Ymin;
      rect->y1 = fb->_Ymax;
   } else {
      rect->y0 = fb->Height - fb->_Ymax;
      rect->y1 = fb->Height - fb->_Ymin;
   }

   rect->x0 = ROUND_DOWN_TO(rect->x0,  x_align) / x_scaledown;
   rect->y0 = ROUND_DOWN_TO(rect->y0, y_align) / y_scaledown;
   rect->x1 = ALIGN(rect->x1, x_align) / x_scaledown;
   rect->y1 = ALIGN(rect->y1, y_align) / y_scaledown;
}

static void
get_buffer_rect(struct brw_context *brw, struct gl_framebuffer *fb,
                struct intel_renderbuffer *irb, struct rect *rect)
{
   rect->x0 = fb->_Xmin;
   rect->x1 = fb->_Xmax;
   if (fb->Name != 0) {
      rect->y0 = fb->_Ymin;
      rect->y1 = fb->_Ymax;
   } else {
      rect->y0 = fb->Height - fb->_Ymax;
      rect->y1 = fb->Height - fb->_Ymin;
   }
}

/**
 * Determine if fast color clear supports the given clear color.
 *
 * Fast color clear can only clear to color values of 1.0 or 0.0.  At the
 * moment we only support floating point, unorm, and snorm buffers.
 */
static bool
is_color_fast_clear_compatible(struct brw_context *brw,
                               mesa_format format,
                               const union gl_color_union *color)
{
   if (_mesa_is_format_integer_color(format))
      return false;

   for (int i = 0; i < 4; i++) {
      if (color->f[i] != 0.0 && color->f[i] != 1.0 &&
          _mesa_format_has_color_component(format, i)) {
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
      /* Testing for non-0 works for integer and float colors */
      if (color->f[i] != 0.0)
         bits |= 1 << (GEN7_SURFACE_CLEAR_COLOR_SHIFT + (3 - i));
   }
   return bits;
}

static const uint32_t fast_clear_color[4] = { ~0, ~0, ~0, ~0 };

static void
set_fast_clear_op(struct brw_context *brw, uint32_t op)
{
   /* Set op and dirty BRW_NEW_FRAGMENT_PROGRAM to make sure we re-emit
    * 3DSTATE_PS.
    */
   brw->wm.fast_clear_op = op;
   brw->state.dirty.brw |= BRW_NEW_FRAGMENT_PROGRAM;
}

static void
use_rectlist(struct brw_context *brw, bool enable)
{
   /* Set custom state to let us use _3DPRIM_RECTLIST and the replicated
    * rendertarget write.  When we enable reclist mode, we disable the
    * viewport transform, disable clipping, enable the rep16 write
    * optimization and disable simd8 dispatch in the PS.
    */
   brw->sf.viewport_transform_enable = !enable;
   brw->use_rep_send = enable;
   brw->no_simd8 = enable;

   /* Dirty state to make sure we reemit the state packages affected by the
    * custom state.  We dirty BRW_NEW_FRAGMENT_PROGRAM to emit 3DSTATE_PS for
    * disabling simd8 dispatch, _NEW_LIGHT to emit 3DSTATE_SF for disabling
    * the viewport transform and 3DSTATE_CLIP to disable clipping for the
    * reclist primitive.  This is a little messy - it would be nicer to
    * BRW_NEW_FAST_CLEAR flag or so, but we're out of brw state bits.  Dirty
    * _NEW_BUFFERS to make sure we emit new SURFACE_STATE with the new fast
    * clear color value.
    */
   brw->state.dirty.mesa |= _NEW_LIGHT | _NEW_BUFFERS;
   brw->state.dirty.brw |= BRW_NEW_FRAGMENT_PROGRAM;
}

bool
brw_meta_fast_clear(struct brw_context *brw, struct gl_framebuffer *fb,
                    GLbitfield buffers, bool partial_clear)
{
   struct gl_context *ctx = &brw->ctx;
   mesa_format format;
   enum { FAST_CLEAR, REP_CLEAR, PLAIN_CLEAR } clear_type;
   GLbitfield plain_clear_buffers, meta_save, rep_clear_buffers, fast_clear_buffers;
   struct rect fast_clear_rect, clear_rect;
   int layers;

   fast_clear_buffers = rep_clear_buffers = plain_clear_buffers = 0;

   /* First we loop through the color draw buffers and determine which ones
    * can be fast cleared, which ones can use the replicated write and which
    * ones have to fall back to regular color clear.
    */
   for (unsigned buf = 0; buf < fb->_NumColorDrawBuffers; buf++) {
      struct gl_renderbuffer *rb = fb->_ColorDrawBuffers[buf];
      struct intel_renderbuffer *irb = intel_renderbuffer(rb);
      int index = fb->_ColorDrawBufferIndexes[buf];

      /* Only clear the buffers present in the provided mask */
      if (((1 << index) & buffers) == 0)
         continue;

      /* If this is an ES2 context or GL_ARB_ES2_compatibility is supported,
       * the framebuffer can be complete with some attachments missing.  In
       * this case the _ColorDrawBuffers pointer will be NULL.
       */
      if (rb == NULL)
         continue;

      clear_type = FAST_CLEAR;

      /* We don't have fast clear until gen7. */
      if (brw->gen < 7)
         clear_type = REP_CLEAR;

      if (irb->mt->fast_clear_state == INTEL_FAST_CLEAR_STATE_NO_MCS)
         clear_type = REP_CLEAR;

      /* We can't do scissored fast clears because of the restrictions on the
       * fast clear rectangle size.
       */
      if (partial_clear)
         clear_type = REP_CLEAR;

      /* Fast clear is only supported for colors where all components are
       * either 0 or 1.
       */
      format = _mesa_get_render_format(ctx, irb->mt->format);
      if (!is_color_fast_clear_compatible(brw, format, &ctx->Color.ClearColor))
         clear_type = REP_CLEAR;

      /* From the SNB PRM (Vol4_Part1):
       *
       *     "Replicated data (Message Type = 111) is only supported when
       *      accessing tiled memory.  Using this Message Type to access
       *      linear (untiled) memory is UNDEFINED."
       */
      if (irb->mt->tiling == I915_TILING_NONE) {
         perf_debug("falling back to plain clear because buffers are untiled\n");
         clear_type = PLAIN_CLEAR;
      }

      /* Constant color writes ignore everything in blend and color calculator
       * state.  This is not documented.
       */
      GLubyte *color_mask = ctx->Color.ColorMask[buf];
      for (int i = 0; i < 4; i++) {
         if (_mesa_format_has_color_component(irb->mt->format, i) &&
             !color_mask[i]) {
            perf_debug("falling back to plain clear because of color mask\n");
            clear_type = PLAIN_CLEAR;
         }
      }

      /* Allocate the MCS for non MSRT surfaces now if we're doing a fast
       * clear and we don't have the MCS yet.  On failure, fall back to
       * replicated clear.
       */
      if (clear_type == FAST_CLEAR && irb->mt->mcs_mt == NULL)
         if (!intel_miptree_alloc_non_msrt_mcs(brw, irb->mt))
            clear_type = REP_CLEAR;

      switch (clear_type) {
      case FAST_CLEAR:
         irb->mt->fast_clear_color_value =
            compute_fast_clear_color_bits(&ctx->Color.ClearColor);
         irb->need_downsample = true;

         /* If the buffer is already in INTEL_FAST_CLEAR_STATE_CLEAR, the
          * clear is redundant and can be skipped.  Only skip after we've
          * updated the fast clear color above though.
          */
         if (irb->mt->fast_clear_state == INTEL_FAST_CLEAR_STATE_CLEAR)
            continue;

         /* Set fast_clear_state to RESOLVED so we don't try resolve them when
          * we draw, in case the mt is also bound as a texture.
          */
         irb->mt->fast_clear_state = INTEL_FAST_CLEAR_STATE_RESOLVED;
         irb->need_downsample = true;
         fast_clear_buffers |= 1 << index;
         get_fast_clear_rect(brw, fb, irb, &fast_clear_rect);
         break;

      case REP_CLEAR:
         rep_clear_buffers |= 1 << index;
         get_buffer_rect(brw, fb, irb, &clear_rect);
         break;

      case PLAIN_CLEAR:
         plain_clear_buffers |= 1 << index;
         get_buffer_rect(brw, fb, irb, &clear_rect);
         continue;
      }
   }

   if (!(fast_clear_buffers | rep_clear_buffers)) {
      if (plain_clear_buffers)
         /* If we only have plain clears, skip the meta save/restore. */
         goto out;
      else
         /* Nothing left to do.  This happens when we hit the redundant fast
          * clear case above and nothing else.
          */
         return true;
   }

   meta_save =
      MESA_META_ALPHA_TEST |
      MESA_META_BLEND |
      MESA_META_DEPTH_TEST |
      MESA_META_RASTERIZATION |
      MESA_META_SHADER |
      MESA_META_STENCIL_TEST |
      MESA_META_VERTEX |
      MESA_META_VIEWPORT |
      MESA_META_CLIP |
      MESA_META_CLAMP_FRAGMENT_COLOR |
      MESA_META_MULTISAMPLE |
      MESA_META_OCCLUSION_QUERY |
      MESA_META_DRAW_BUFFERS;

   _mesa_meta_begin(ctx, meta_save);

   if (!brw_fast_clear_init(brw)) {
      /* This is going to be hard to recover from, most likely out of memory.
       * Bail and let meta try and (probably) fail for us.
       */
      plain_clear_buffers = buffers;
      goto bail_to_meta;
   }

   /* Clears never have the color clamped. */
   if (ctx->Extensions.ARB_color_buffer_float)
      _mesa_ClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_FALSE);

   _mesa_set_enable(ctx, GL_DEPTH_TEST, GL_FALSE);
   _mesa_DepthMask(GL_FALSE);
   _mesa_set_enable(ctx, GL_STENCIL_TEST, GL_FALSE);

   use_rectlist(brw, true);

   layers = MAX2(1, fb->MaxNumLayers);
   if (fast_clear_buffers) {
      _mesa_meta_drawbuffers_from_bitfield(fast_clear_buffers);
      brw_bind_rep_write_shader(brw, (float *) fast_clear_color);
      set_fast_clear_op(brw, GEN7_PS_RENDER_TARGET_FAST_CLEAR_ENABLE);
      brw_draw_rectlist(ctx, &fast_clear_rect, layers);
      set_fast_clear_op(brw, 0);
   }

   if (rep_clear_buffers) {
      _mesa_meta_drawbuffers_from_bitfield(rep_clear_buffers);
      brw_bind_rep_write_shader(brw, ctx->Color.ClearColor.f);
      brw_draw_rectlist(ctx, &clear_rect, layers);
   }

   /* Now set the mts we cleared to INTEL_FAST_CLEAR_STATE_CLEAR so we'll
    * resolve them eventually.
    */
   for (unsigned buf = 0; buf < fb->_NumColorDrawBuffers; buf++) {
      struct gl_renderbuffer *rb = fb->_ColorDrawBuffers[buf];
      struct intel_renderbuffer *irb = intel_renderbuffer(rb);
      int index = fb->_ColorDrawBufferIndexes[buf];

      if ((1 << index) & fast_clear_buffers)
         irb->mt->fast_clear_state = INTEL_FAST_CLEAR_STATE_CLEAR;
   }

 bail_to_meta:
   /* Dirty _NEW_BUFFERS so we reemit SURFACE_STATE which sets the fast clear
    * color before resolve and sets irb->mt->fast_clear_state to UNRESOLVED if
    * we render to it.
    */
   brw->state.dirty.mesa |= _NEW_BUFFERS;


   /* Set the custom state back to normal and dirty the same bits as above */
   use_rectlist(brw, false);

   _mesa_meta_end(ctx);

   /* From BSpec: Render Target Fast Clear:
    *
    *     After Render target fast clear, pipe-control with color cache
    *     write-flush must be issued before sending any DRAW commands on that
    *     render target.
    */
   intel_batchbuffer_emit_mi_flush(brw);

   /* If we had to fall back to plain clear for any buffers, clear those now
    * by calling into meta.
    */
 out:
   if (plain_clear_buffers)
      _mesa_meta_glsl_Clear(&brw->ctx, plain_clear_buffers);

   return true;
}

static void
get_resolve_rect(struct brw_context *brw,
                 struct intel_mipmap_tree *mt, struct rect *rect)
{
   unsigned x_align, y_align;
   unsigned x_scaledown, y_scaledown;

   /* From the Ivy Bridge PRM, Vol2 Part1 11.9 "Render Target Resolve":
    *
    *     A rectangle primitive must be scaled down by the following factors
    *     with respect to render target being resolved.
    *
    * The scaledown factors in the table that follows are related to the
    * alignment size returned by intel_get_non_msrt_mcs_alignment() by a
    * multiplier.  For IVB and HSW, we divide by two, for BDW we multiply
    * by 8 and 16.
    */

   intel_get_non_msrt_mcs_alignment(brw, mt, &x_align, &y_align);
   if (brw->gen >= 8) {
      x_scaledown = x_align * 8;
      y_scaledown = y_align * 16;
   } else {
      x_scaledown = x_align / 2;
      y_scaledown = y_align / 2;
   }
   rect->x0 = rect->y0 = 0;
   rect->x1 = ALIGN(mt->logical_width0, x_scaledown) / x_scaledown;
   rect->y1 = ALIGN(mt->logical_height0, y_scaledown) / y_scaledown;
}

void
brw_meta_resolve_color(struct brw_context *brw,
                       struct intel_mipmap_tree *mt)
{
   struct gl_context *ctx = &brw->ctx;
   GLuint fbo, rbo;
   struct rect rect;

   intel_batchbuffer_emit_mi_flush(brw);

   _mesa_meta_begin(ctx, MESA_META_ALL);

   _mesa_GenFramebuffers(1, &fbo);
   rbo = brw_get_rb_for_slice(brw, mt, 0, 0, false);

   _mesa_BindFramebuffer(GL_DRAW_FRAMEBUFFER, fbo);
   _mesa_FramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER,
                                 GL_COLOR_ATTACHMENT0,
                                 GL_RENDERBUFFER, rbo);
   _mesa_DrawBuffer(GL_COLOR_ATTACHMENT0);

   brw_fast_clear_init(brw);

   use_rectlist(brw, true);

   brw_bind_rep_write_shader(brw, (float *) fast_clear_color);

   set_fast_clear_op(brw, GEN7_PS_RENDER_TARGET_RESOLVE_ENABLE);

   mt->fast_clear_state = INTEL_FAST_CLEAR_STATE_RESOLVED;
   get_resolve_rect(brw, mt, &rect);

   brw_draw_rectlist(ctx, &rect, 1);

   set_fast_clear_op(brw, 0);
   use_rectlist(brw, false);

   _mesa_DeleteRenderbuffers(1, &rbo);
   _mesa_DeleteFramebuffers(1, &fbo);

   _mesa_meta_end(ctx);

   /* We're typically called from intel_update_state() and we're supposed to
    * return with the state all updated to what it was before
    * brw_meta_resolve_color() was called.  The meta rendering will have
    * messed up the state and we need to call _mesa_update_state() again to
    * get back to where we were supposed to be when resolve was called.
    */
   if (ctx->NewState)
      _mesa_update_state(ctx);
}
