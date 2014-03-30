/**************************************************************************
 * 
 * Copyright 2007 VMware, Inc.
 * All Rights Reserved.
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
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

 /*
  * Authors:
  *   Keith Whitwell <keithw@vmware.com>
  *   Brian Paul
  *   Michel Dänzer
  */

#include "main/glheader.h"
#include "main/accum.h"
#include "main/formats.h"
#include "main/macros.h"
#include "main/glformats.h"
#include "program/prog_instruction.h"
#include "st_context.h"
#include "st_atom.h"
#include "st_cb_clear.h"
#include "st_cb_fbo.h"
#include "st_format.h"
#include "st_program.h"

#include "pipe/p_context.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_format.h"
#include "util/u_framebuffer.h"
#include "util/u_inlines.h"
#include "util/u_simple_shaders.h"
#include "util/u_draw_quad.h"
#include "util/u_upload_mgr.h"

#include "cso_cache/cso_context.h"


/**
 * Do per-context initialization for glClear.
 */
void
st_init_clear(struct st_context *st)
{
   memset(&st->clear, 0, sizeof(st->clear));

   st->clear.raster.half_pixel_center = 1;
   st->clear.raster.bottom_edge_rule = 1;
   st->clear.raster.depth_clip = 1;
}


/**
 * Free per-context state for glClear.
 */
void
st_destroy_clear(struct st_context *st)
{
   if (st->clear.fs) {
      cso_delete_fragment_shader(st->cso_context, st->clear.fs);
      st->clear.fs = NULL;
   }
   if (st->clear.vs) {
      cso_delete_vertex_shader(st->cso_context, st->clear.vs);
      st->clear.vs = NULL;
   }
}


/**
 * Helper function to set the fragment shaders.
 */
static INLINE void
set_fragment_shader(struct st_context *st)
{
   if (!st->clear.fs)
      st->clear.fs =
         util_make_fragment_passthrough_shader(st->pipe, TGSI_SEMANTIC_GENERIC,
                                               TGSI_INTERPOLATE_CONSTANT,
                                               TRUE);

   cso_set_fragment_shader_handle(st->cso_context, st->clear.fs);
}


/**
 * Helper function to set the vertex shader.
 */
static INLINE void
set_vertex_shader(struct st_context *st)
{
   /* vertex shader - still required to provide the linkage between
    * fragment shader input semantics and vertex_element/buffers.
    */
   if (!st->clear.vs)
   {
      const uint semantic_names[] = { TGSI_SEMANTIC_POSITION,
                                      TGSI_SEMANTIC_GENERIC };
      const uint semantic_indexes[] = { 0, 0 };
      st->clear.vs = util_make_vertex_passthrough_shader(st->pipe, 2,
                                                         semantic_names,
                                                         semantic_indexes);
   }

   cso_set_vertex_shader_handle(st->cso_context, st->clear.vs);
}


static void
set_vertex_shader_layered(struct st_context *st)
{
   struct pipe_context *pipe = st->pipe;

   if (!pipe->screen->get_param(pipe->screen, PIPE_CAP_TGSI_INSTANCEID) ||
       !pipe->screen->get_param(pipe->screen, PIPE_CAP_TGSI_VS_LAYER)) {
      assert(!"Got layered clear, but the VS layer output is unsupported");
      set_vertex_shader(st);
      return;
   }

   if (!st->clear.vs_layered) {
      st->clear.vs_layered = util_make_layered_clear_vertex_shader(pipe);
   }

   cso_set_vertex_shader_handle(st->cso_context, st->clear.vs_layered);
}


/**
 * Draw a screen-aligned quadrilateral.
 * Coords are clip coords with y=0=bottom.
 */
static void
draw_quad(struct st_context *st,
          float x0, float y0, float x1, float y1, GLfloat z,
          unsigned num_instances,
          const union pipe_color_union *color)
{
   struct cso_context *cso = st->cso_context;
   struct pipe_vertex_buffer vb = {0};
   GLuint i;
   float (*vertices)[2][4];  /**< vertex pos + color */

   vb.stride = 8 * sizeof(float);

   if (u_upload_alloc(st->uploader, 0, 4 * sizeof(vertices[0]),
                      &vb.buffer_offset, &vb.buffer,
                      (void **) &vertices) != PIPE_OK) {
      return;
   }

   /* positions */
   vertices[0][0][0] = x0;
   vertices[0][0][1] = y0;

   vertices[1][0][0] = x1;
   vertices[1][0][1] = y0;

   vertices[2][0][0] = x1;
   vertices[2][0][1] = y1;

   vertices[3][0][0] = x0;
   vertices[3][0][1] = y1;

   /* same for all verts: */
   for (i = 0; i < 4; i++) {
      vertices[i][0][2] = z;
      vertices[i][0][3] = 1.0;
      vertices[i][1][0] = color->f[0];
      vertices[i][1][1] = color->f[1];
      vertices[i][1][2] = color->f[2];
      vertices[i][1][3] = color->f[3];
   }

   u_upload_unmap(st->uploader);

   /* draw */
   cso_set_vertex_buffers(cso, cso_get_aux_vertex_buffer_slot(cso), 1, &vb);
   cso_draw_arrays_instanced(cso, PIPE_PRIM_TRIANGLE_FAN, 0, 4,
                             0, num_instances);
   pipe_resource_reference(&vb.buffer, NULL);
}



/**
 * Do glClear by drawing a quadrilateral.
 * The vertices of the quad will be computed from the
 * ctx->DrawBuffer->_X/Ymin/max fields.
 */
static void
clear_with_quad(struct gl_context *ctx, unsigned clear_buffers)
{
   struct st_context *st = st_context(ctx);
   const struct gl_framebuffer *fb = ctx->DrawBuffer;
   const GLfloat fb_width = (GLfloat) fb->Width;
   const GLfloat fb_height = (GLfloat) fb->Height;
   const GLfloat x0 = (GLfloat) ctx->DrawBuffer->_Xmin / fb_width * 2.0f - 1.0f;
   const GLfloat x1 = (GLfloat) ctx->DrawBuffer->_Xmax / fb_width * 2.0f - 1.0f;
   const GLfloat y0 = (GLfloat) ctx->DrawBuffer->_Ymin / fb_height * 2.0f - 1.0f;
   const GLfloat y1 = (GLfloat) ctx->DrawBuffer->_Ymax / fb_height * 2.0f - 1.0f;
   unsigned num_layers =
      util_framebuffer_get_num_layers(&st->state.framebuffer);

   /*
   printf("%s %s%s%s %f,%f %f,%f\n", __FUNCTION__, 
	  color ? "color, " : "",
	  depth ? "depth, " : "",
	  stencil ? "stencil" : "",
	  x0, y0,
	  x1, y1);
   */

   cso_save_blend(st->cso_context);
   cso_save_stencil_ref(st->cso_context);
   cso_save_depth_stencil_alpha(st->cso_context);
   cso_save_rasterizer(st->cso_context);
   cso_save_sample_mask(st->cso_context);
   cso_save_min_samples(st->cso_context);
   cso_save_viewport(st->cso_context);
   cso_save_fragment_shader(st->cso_context);
   cso_save_stream_outputs(st->cso_context);
   cso_save_vertex_shader(st->cso_context);
   cso_save_geometry_shader(st->cso_context);
   cso_save_vertex_elements(st->cso_context);
   cso_save_aux_vertex_buffer_slot(st->cso_context);

   /* blend state: RGBA masking */
   {
      struct pipe_blend_state blend;
      memset(&blend, 0, sizeof(blend));
      if (clear_buffers & PIPE_CLEAR_COLOR) {
         int num_buffers = ctx->Extensions.EXT_draw_buffers2 ?
                           ctx->DrawBuffer->_NumColorDrawBuffers : 1;
         int i;

         blend.independent_blend_enable = num_buffers > 1;

         for (i = 0; i < num_buffers; i++) {
            if (!(clear_buffers & (PIPE_CLEAR_COLOR0 << i)))
               continue;

            if (ctx->Color.ColorMask[i][0])
               blend.rt[i].colormask |= PIPE_MASK_R;
            if (ctx->Color.ColorMask[i][1])
               blend.rt[i].colormask |= PIPE_MASK_G;
            if (ctx->Color.ColorMask[i][2])
               blend.rt[i].colormask |= PIPE_MASK_B;
            if (ctx->Color.ColorMask[i][3])
               blend.rt[i].colormask |= PIPE_MASK_A;
         }

         if (st->ctx->Color.DitherFlag)
            blend.dither = 1;
      }
      cso_set_blend(st->cso_context, &blend);
   }

   /* depth_stencil state: always pass/set to ref value */
   {
      struct pipe_depth_stencil_alpha_state depth_stencil;
      memset(&depth_stencil, 0, sizeof(depth_stencil));
      if (clear_buffers & PIPE_CLEAR_DEPTH) {
         depth_stencil.depth.enabled = 1;
         depth_stencil.depth.writemask = 1;
         depth_stencil.depth.func = PIPE_FUNC_ALWAYS;
      }

      if (clear_buffers & PIPE_CLEAR_STENCIL) {
         struct pipe_stencil_ref stencil_ref;
         memset(&stencil_ref, 0, sizeof(stencil_ref));
         depth_stencil.stencil[0].enabled = 1;
         depth_stencil.stencil[0].func = PIPE_FUNC_ALWAYS;
         depth_stencil.stencil[0].fail_op = PIPE_STENCIL_OP_REPLACE;
         depth_stencil.stencil[0].zpass_op = PIPE_STENCIL_OP_REPLACE;
         depth_stencil.stencil[0].zfail_op = PIPE_STENCIL_OP_REPLACE;
         depth_stencil.stencil[0].valuemask = 0xff;
         depth_stencil.stencil[0].writemask = ctx->Stencil.WriteMask[0] & 0xff;
         stencil_ref.ref_value[0] = ctx->Stencil.Clear;
         cso_set_stencil_ref(st->cso_context, &stencil_ref);
      }

      cso_set_depth_stencil_alpha(st->cso_context, &depth_stencil);
   }

   cso_set_vertex_elements(st->cso_context, 2, st->velems_util_draw);
   cso_set_stream_outputs(st->cso_context, 0, NULL, NULL);
   cso_set_sample_mask(st->cso_context, ~0);
   cso_set_min_samples(st->cso_context, 1);
   cso_set_rasterizer(st->cso_context, &st->clear.raster);

   /* viewport state: viewport matching window dims */
   {
      const GLboolean invert = (st_fb_orientation(fb) == Y_0_TOP);
      struct pipe_viewport_state vp;
      vp.scale[0] = 0.5f * fb_width;
      vp.scale[1] = fb_height * (invert ? -0.5f : 0.5f);
      vp.scale[2] = 1.0f;
      vp.scale[3] = 1.0f;
      vp.translate[0] = 0.5f * fb_width;
      vp.translate[1] = 0.5f * fb_height;
      vp.translate[2] = 0.0f;
      vp.translate[3] = 0.0f;
      cso_set_viewport(st->cso_context, &vp);
   }

   set_fragment_shader(st);
   cso_set_geometry_shader_handle(st->cso_context, NULL);

   if (num_layers > 1)
      set_vertex_shader_layered(st);
   else
      set_vertex_shader(st);

   /* We can't translate the clear color to the colorbuffer format,
    * because different colorbuffers may have different formats.
    */

   /* draw quad matching scissor rect */
   draw_quad(st, x0, y0, x1, y1, (GLfloat) ctx->Depth.Clear, num_layers,
             (union pipe_color_union*)&ctx->Color.ClearColor);

   /* Restore pipe state */
   cso_restore_blend(st->cso_context);
   cso_restore_stencil_ref(st->cso_context);
   cso_restore_depth_stencil_alpha(st->cso_context);
   cso_restore_rasterizer(st->cso_context);
   cso_restore_sample_mask(st->cso_context);
   cso_restore_min_samples(st->cso_context);
   cso_restore_viewport(st->cso_context);
   cso_restore_fragment_shader(st->cso_context);
   cso_restore_vertex_shader(st->cso_context);
   cso_restore_geometry_shader(st->cso_context);
   cso_restore_vertex_elements(st->cso_context);
   cso_restore_aux_vertex_buffer_slot(st->cso_context);
   cso_restore_stream_outputs(st->cso_context);
}


/**
 * Return if the scissor must be enabled during the clear.
 */
static INLINE GLboolean
is_scissor_enabled(struct gl_context *ctx, struct gl_renderbuffer *rb)
{
   return (ctx->Scissor.EnableFlags & 1) &&
          (ctx->Scissor.ScissorArray[0].X > 0 ||
           ctx->Scissor.ScissorArray[0].Y > 0 ||
           (unsigned) ctx->Scissor.ScissorArray[0].Width < rb->Width ||
           (unsigned) ctx->Scissor.ScissorArray[0].Height < rb->Height);
}


/**
 * Return if all of the color channels are masked.
 */
static INLINE GLboolean
is_color_disabled(struct gl_context *ctx, int i)
{
   return !ctx->Color.ColorMask[i][0] &&
          !ctx->Color.ColorMask[i][1] &&
          !ctx->Color.ColorMask[i][2] &&
          !ctx->Color.ColorMask[i][3];
}


/**
 * Return if any of the color channels are masked.
 */
static INLINE GLboolean
is_color_masked(struct gl_context *ctx, int i)
{
   return !ctx->Color.ColorMask[i][0] ||
          !ctx->Color.ColorMask[i][1] ||
          !ctx->Color.ColorMask[i][2] ||
          !ctx->Color.ColorMask[i][3];
}


/**
 * Return if all of the stencil bits are masked.
 */
static INLINE GLboolean
is_stencil_disabled(struct gl_context *ctx, struct gl_renderbuffer *rb)
{
   const GLuint stencilMax = 0xff;

   assert(_mesa_get_format_bits(rb->Format, GL_STENCIL_BITS) > 0);
   return (ctx->Stencil.WriteMask[0] & stencilMax) == 0;
}


/**
 * Return if any of the stencil bits are masked.
 */
static INLINE GLboolean
is_stencil_masked(struct gl_context *ctx, struct gl_renderbuffer *rb)
{
   const GLuint stencilMax = 0xff;

   assert(_mesa_get_format_bits(rb->Format, GL_STENCIL_BITS) > 0);
   return (ctx->Stencil.WriteMask[0] & stencilMax) != stencilMax;
}


/**
 * Called via ctx->Driver.Clear()
 */
static void
st_Clear(struct gl_context *ctx, GLbitfield mask)
{
   struct st_context *st = st_context(ctx);
   struct gl_renderbuffer *depthRb
      = ctx->DrawBuffer->Attachment[BUFFER_DEPTH].Renderbuffer;
   struct gl_renderbuffer *stencilRb
      = ctx->DrawBuffer->Attachment[BUFFER_STENCIL].Renderbuffer;
   GLbitfield quad_buffers = 0x0;
   GLbitfield clear_buffers = 0x0;
   GLuint i;

   /* This makes sure the pipe has the latest scissor, etc values */
   st_validate_state( st );

   if (mask & BUFFER_BITS_COLOR) {
      for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++) {
         GLint b = ctx->DrawBuffer->_ColorDrawBufferIndexes[i];

         if (b >= 0 && mask & (1 << b)) {
            struct gl_renderbuffer *rb
               = ctx->DrawBuffer->Attachment[b].Renderbuffer;
            struct st_renderbuffer *strb = st_renderbuffer(rb);
            int colormask_index = ctx->Extensions.EXT_draw_buffers2 ? i : 0;

            if (!strb || !strb->surface)
               continue;

            if (is_color_disabled(ctx, colormask_index))
               continue;

            if (is_scissor_enabled(ctx, rb) ||
                is_color_masked(ctx, colormask_index))
               quad_buffers |= PIPE_CLEAR_COLOR0 << i;
            else
               clear_buffers |= PIPE_CLEAR_COLOR0 << i;
         }
      }
   }

   if (mask & BUFFER_BIT_DEPTH) {
      struct st_renderbuffer *strb = st_renderbuffer(depthRb);

      if (strb->surface && ctx->Depth.Mask) {
         if (is_scissor_enabled(ctx, depthRb))
            quad_buffers |= PIPE_CLEAR_DEPTH;
         else
            clear_buffers |= PIPE_CLEAR_DEPTH;
      }
   }
   if (mask & BUFFER_BIT_STENCIL) {
      struct st_renderbuffer *strb = st_renderbuffer(stencilRb);

      if (strb->surface && !is_stencil_disabled(ctx, stencilRb)) {
         if (is_scissor_enabled(ctx, stencilRb) ||
             is_stencil_masked(ctx, stencilRb))
            quad_buffers |= PIPE_CLEAR_STENCIL;
         else
            clear_buffers |= PIPE_CLEAR_STENCIL;
      }
   }

   /* Always clear depth and stencil together.
    * This can only happen when the stencil writemask is not a full mask.
    */
   if (quad_buffers & PIPE_CLEAR_DEPTHSTENCIL &&
       clear_buffers & PIPE_CLEAR_DEPTHSTENCIL) {
      quad_buffers |= clear_buffers & PIPE_CLEAR_DEPTHSTENCIL;
      clear_buffers &= ~PIPE_CLEAR_DEPTHSTENCIL;
   }

   /* Only use quad-based clearing for the renderbuffers which cannot
    * use pipe->clear. We want to always use pipe->clear for the other
    * renderbuffers, because it's likely to be faster.
    */
   if (quad_buffers) {
      clear_with_quad(ctx, quad_buffers);
   }
   if (clear_buffers) {
      /* We can't translate the clear color to the colorbuffer format,
       * because different colorbuffers may have different formats.
       */
      st->pipe->clear(st->pipe, clear_buffers,
                      (union pipe_color_union*)&ctx->Color.ClearColor,
                      ctx->Depth.Clear, ctx->Stencil.Clear);
   }
   if (mask & BUFFER_BIT_ACCUM)
      _mesa_clear_accum_buffer(ctx);
}


void
st_init_clear_functions(struct dd_function_table *functions)
{
   functions->Clear = st_Clear;
}
