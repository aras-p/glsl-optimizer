/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Brian Paul
  *   Michel Dänzer
  */

#include "main/glheader.h"
#include "main/formats.h"
#include "main/macros.h"
#include "shader/prog_instruction.h"
#include "st_context.h"
#include "st_atom.h"
#include "st_cb_accum.h"
#include "st_cb_clear.h"
#include "st_cb_fbo.h"
#include "st_program.h"
#include "st_public.h"
#include "st_inlines.h"

#include "pipe/p_context.h"
#include "util/u_inlines.h"
#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_format.h"
#include "util/u_simple_shaders.h"
#include "util/u_draw_quad.h"

#include "cso_cache/cso_context.h"


void
st_init_clear(struct st_context *st)
{
   struct pipe_context *pipe = st->pipe;

   memset(&st->clear, 0, sizeof(st->clear));

   st->clear.raster.gl_rasterization_rules = 1;

   /* fragment shader state: color pass-through program */
   st->clear.fs =
      util_make_fragment_passthrough_shader(pipe);

   /* vertex shader state: color/position pass-through */
   {
      const uint semantic_names[] = { TGSI_SEMANTIC_POSITION,
                                      TGSI_SEMANTIC_COLOR };
      const uint semantic_indexes[] = { 0, 0 };
      st->clear.vs = util_make_vertex_passthrough_shader(pipe, 2,
                                                         semantic_names,
                                                         semantic_indexes);
   }
}


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
   if (st->clear.vbuf) {
      pipe_buffer_reference(&st->clear.vbuf, NULL);
      st->clear.vbuf = NULL;
   }
}


/**
 * Draw a screen-aligned quadrilateral.
 * Coords are clip coords with y=0=bottom.
 */
static void
draw_quad(GLcontext *ctx,
          float x0, float y0, float x1, float y1, GLfloat z,
          const GLfloat color[4])
{
   struct st_context *st = ctx->st;
   struct pipe_context *pipe = st->pipe;

   /* XXX: Need to improve buffer_write to allow NO_WAIT (as well as
    * no_flush) updates to buffers where we know there is no conflict
    * with previous data.  Currently using max_slots > 1 will cause
    * synchronous rendering if the driver flushes its command buffers
    * between one bitmap and the next.  Our flush hook below isn't
    * sufficient to catch this as the driver doesn't tell us when it
    * flushes its own command buffers.  Until this gets fixed, pay the
    * price of allocating a new buffer for each bitmap cache-flush to
    * avoid synchronous rendering.
    */
   const GLuint max_slots = 1; /* 1024 / sizeof(st->clear.vertices); */
   GLuint i;

   if (st->clear.vbuf_slot >= max_slots) {
      pipe_buffer_reference(&st->clear.vbuf, NULL);
      st->clear.vbuf_slot = 0;
   }

   if (!st->clear.vbuf) {
      st->clear.vbuf = pipe_buffer_create(pipe->screen, 32, PIPE_BUFFER_USAGE_VERTEX,
                                          max_slots * sizeof(st->clear.vertices));
   }

   /* positions */
   st->clear.vertices[0][0][0] = x0;
   st->clear.vertices[0][0][1] = y0;

   st->clear.vertices[1][0][0] = x1;
   st->clear.vertices[1][0][1] = y0;

   st->clear.vertices[2][0][0] = x1;
   st->clear.vertices[2][0][1] = y1;

   st->clear.vertices[3][0][0] = x0;
   st->clear.vertices[3][0][1] = y1;

   /* same for all verts: */
   for (i = 0; i < 4; i++) {
      st->clear.vertices[i][0][2] = z;
      st->clear.vertices[i][0][3] = 1.0;
      st->clear.vertices[i][1][0] = color[0];
      st->clear.vertices[i][1][1] = color[1];
      st->clear.vertices[i][1][2] = color[2];
      st->clear.vertices[i][1][3] = color[3];
   }

   /* put vertex data into vbuf */
   st_no_flush_pipe_buffer_write_nooverlap(st, st->clear.vbuf,
                                           st->clear.vbuf_slot * sizeof(st->clear.vertices),
                                           sizeof(st->clear.vertices),
                                           st->clear.vertices);

   /* draw */
   util_draw_vertex_buffer(pipe, 
                           st->clear.vbuf, 
                           st->clear.vbuf_slot * sizeof(st->clear.vertices),
                           PIPE_PRIM_TRIANGLE_FAN,
                           4,  /* verts */
                           2); /* attribs/vert */

   /* Increment slot */
   st->clear.vbuf_slot++;
}



/**
 * Do glClear by drawing a quadrilateral.
 * The vertices of the quad will be computed from the
 * ctx->DrawBuffer->_X/Ymin/max fields.
 */
static void
clear_with_quad(GLcontext *ctx,
                GLboolean color, GLboolean depth, GLboolean stencil)
{
   struct st_context *st = ctx->st;
   const struct gl_framebuffer *fb = ctx->DrawBuffer;
   const GLfloat fb_width = (GLfloat) fb->Width;
   const GLfloat fb_height = (GLfloat) fb->Height;
   const GLfloat x0 = (GLfloat) ctx->DrawBuffer->_Xmin / fb_width * 2.0f - 1.0f;
   const GLfloat x1 = (GLfloat) ctx->DrawBuffer->_Xmax / fb_width * 2.0f - 1.0f;
   const GLfloat y0 = (GLfloat) ctx->DrawBuffer->_Ymin / fb_height * 2.0f - 1.0f;
   const GLfloat y1 = (GLfloat) ctx->DrawBuffer->_Ymax / fb_height * 2.0f - 1.0f;

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
   cso_save_viewport(st->cso_context);
   cso_save_clip(st->cso_context);
   cso_save_fragment_shader(st->cso_context);
   cso_save_vertex_shader(st->cso_context);

   /* blend state: RGBA masking */
   {
      struct pipe_blend_state blend;
      memset(&blend, 0, sizeof(blend));
      blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_ONE;
      blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
      blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
      blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
      if (color) {
         if (ctx->Color.ColorMask[0][0])
            blend.rt[0].colormask |= PIPE_MASK_R;
         if (ctx->Color.ColorMask[0][1])
            blend.rt[0].colormask |= PIPE_MASK_G;
         if (ctx->Color.ColorMask[0][2])
            blend.rt[0].colormask |= PIPE_MASK_B;
         if (ctx->Color.ColorMask[0][3])
            blend.rt[0].colormask |= PIPE_MASK_A;
         if (st->ctx->Color.DitherFlag)
            blend.dither = 1;
      }
      cso_set_blend(st->cso_context, &blend);
   }

   /* depth_stencil state: always pass/set to ref value */
   {
      struct pipe_depth_stencil_alpha_state depth_stencil;
      memset(&depth_stencil, 0, sizeof(depth_stencil));
      if (depth) {
         depth_stencil.depth.enabled = 1;
         depth_stencil.depth.writemask = 1;
         depth_stencil.depth.func = PIPE_FUNC_ALWAYS;
      }

      if (stencil) {
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

   cso_set_clip(st->cso_context, &st->clear.clip);
   cso_set_fragment_shader_handle(st->cso_context, st->clear.fs);
   cso_set_vertex_shader_handle(st->cso_context, st->clear.vs);

   /* draw quad matching scissor rect (XXX verify coord round-off) */
   draw_quad(ctx, x0, y0, x1, y1, (GLfloat) ctx->Depth.Clear, ctx->Color.ClearColor);

   /* Restore pipe state */
   cso_restore_blend(st->cso_context);
   cso_restore_stencil_ref(st->cso_context);
   cso_restore_depth_stencil_alpha(st->cso_context);
   cso_restore_rasterizer(st->cso_context);
   cso_restore_viewport(st->cso_context);
   cso_restore_clip(st->cso_context);
   cso_restore_fragment_shader(st->cso_context);
   cso_restore_vertex_shader(st->cso_context);
}


/**
 * Determine if we need to clear the depth buffer by drawing a quad.
 */
static INLINE GLboolean
check_clear_color_with_quad(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   if (ctx->Scissor.Enabled &&
       (ctx->Scissor.X != 0 ||
        ctx->Scissor.Y != 0 ||
        ctx->Scissor.Width < rb->Width ||
        ctx->Scissor.Height < rb->Height))
      return TRUE;

   if (!ctx->Color.ColorMask[0][0] ||
       !ctx->Color.ColorMask[0][1] ||
       !ctx->Color.ColorMask[0][2] ||
       !ctx->Color.ColorMask[0][3])
      return TRUE;

   return FALSE;
}


static INLINE GLboolean
check_clear_depth_stencil_with_quad(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   const GLuint stencilMax = 0xff;
   GLboolean maskStencil
      = (ctx->Stencil.WriteMask[0] & stencilMax) != stencilMax;

   assert(rb->Format == MESA_FORMAT_S8 ||
          rb->Format == MESA_FORMAT_Z24_S8 ||
          rb->Format == MESA_FORMAT_S8_Z24);

   if (ctx->Scissor.Enabled &&
       (ctx->Scissor.X != 0 ||
        ctx->Scissor.Y != 0 ||
        ctx->Scissor.Width < rb->Width ||
        ctx->Scissor.Height < rb->Height))
      return TRUE;

   if (maskStencil)
      return TRUE;

   return FALSE;
}


/**
 * Determine if we need to clear the depth buffer by drawing a quad.
 */
static INLINE GLboolean
check_clear_depth_with_quad(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   const struct st_renderbuffer *strb = st_renderbuffer(rb);
   const GLboolean isDS = util_format_is_depth_and_stencil(strb->surface->format);

   if (ctx->Scissor.Enabled &&
       (ctx->Scissor.X != 0 ||
        ctx->Scissor.Y != 0 ||
        ctx->Scissor.Width < rb->Width ||
        ctx->Scissor.Height < rb->Height))
      return TRUE;

   if (isDS && 
       ctx->DrawBuffer->Visual.stencilBits > 0)
      return TRUE;

   return FALSE;
}


/**
 * Determine if we need to clear the stencil buffer by drawing a quad.
 */
static INLINE GLboolean
check_clear_stencil_with_quad(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   const struct st_renderbuffer *strb = st_renderbuffer(rb);
   const GLboolean isDS = util_format_is_depth_and_stencil(strb->surface->format);
   const GLuint stencilMax = 0xff;
   const GLboolean maskStencil
      = (ctx->Stencil.WriteMask[0] & stencilMax) != stencilMax;

   assert(rb->Format == MESA_FORMAT_S8 ||
          rb->Format == MESA_FORMAT_Z24_S8 ||
          rb->Format == MESA_FORMAT_S8_Z24);

   if (maskStencil) 
      return TRUE;

   if (ctx->Scissor.Enabled &&
       (ctx->Scissor.X != 0 ||
        ctx->Scissor.Y != 0 ||
        ctx->Scissor.Width < rb->Width ||
        ctx->Scissor.Height < rb->Height))
      return TRUE;

   /* This is correct, but it is necessary to look at the depth clear
    * value held in the surface when it comes time to issue the clear,
    * rather than taking depth and stencil clear values from the
    * current state.
    */
   if (isDS && 
       ctx->DrawBuffer->Visual.depthBits > 0)
      return TRUE;

   return FALSE;
}



void st_flush_clear( struct st_context *st )
{
   /* Release vertex buffer to avoid synchronous rendering if we were
    * to map it in the next frame.
    */
   pipe_buffer_reference(&st->clear.vbuf, NULL);
   st->clear.vbuf_slot = 0;
}
 


/**
 * Called via ctx->Driver.Clear()
 * XXX: doesn't pick up the differences between front/back/left/right
 * clears.  Need to sort that out...
 */
static void st_clear(GLcontext *ctx, GLbitfield mask)
{
   static const GLbitfield BUFFER_BITS_DS
      = (BUFFER_BIT_DEPTH | BUFFER_BIT_STENCIL);
   struct st_context *st = ctx->st;
   struct gl_renderbuffer *depthRb
      = ctx->DrawBuffer->Attachment[BUFFER_DEPTH].Renderbuffer;
   struct gl_renderbuffer *stencilRb
      = ctx->DrawBuffer->Attachment[BUFFER_STENCIL].Renderbuffer;
   GLbitfield quad_buffers = 0;
   GLbitfield clear_buffers = 0;
   GLuint i;

   /* This makes sure the pipe has the latest scissor, etc values */
   st_validate_state( st );

   if (mask & BUFFER_BITS_COLOR) {
      for (i = 0; i < ctx->DrawBuffer->_NumColorDrawBuffers; i++) {
         GLuint b = ctx->DrawBuffer->_ColorDrawBufferIndexes[i];

         if (mask & (1 << b)) {
            struct gl_renderbuffer *rb
               = ctx->DrawBuffer->Attachment[b].Renderbuffer;
            struct st_renderbuffer *strb;

            assert(rb);

            strb = st_renderbuffer(rb);

            if (!strb->surface)
               continue;

            if (check_clear_color_with_quad( ctx, rb ))
               quad_buffers |= PIPE_CLEAR_COLOR;
            else
               clear_buffers |= PIPE_CLEAR_COLOR;
         }
      }
   }

   if ((mask & BUFFER_BITS_DS) == BUFFER_BITS_DS && depthRb == stencilRb) {
      /* clearing combined depth + stencil */
      struct st_renderbuffer *strb = st_renderbuffer(depthRb);

      if (strb->surface) {
         if (check_clear_depth_stencil_with_quad(ctx, depthRb))
            quad_buffers |= PIPE_CLEAR_DEPTHSTENCIL;
         else
            clear_buffers |= PIPE_CLEAR_DEPTHSTENCIL;
      }
   }
   else {
      /* separate depth/stencil clears */
      if (mask & BUFFER_BIT_DEPTH) {
         struct st_renderbuffer *strb = st_renderbuffer(depthRb);

         if (strb->surface) {
            if (check_clear_depth_with_quad(ctx, depthRb))
               quad_buffers |= PIPE_CLEAR_DEPTHSTENCIL;
            else
               clear_buffers |= PIPE_CLEAR_DEPTHSTENCIL;
         }
      }
      if (mask & BUFFER_BIT_STENCIL) {
         struct st_renderbuffer *strb = st_renderbuffer(stencilRb);

         if (strb->surface) {
            if (check_clear_stencil_with_quad(ctx, stencilRb))
               quad_buffers |= PIPE_CLEAR_DEPTHSTENCIL;
            else
               clear_buffers |= PIPE_CLEAR_DEPTHSTENCIL;
         }
      }
   }

   /*
    * If we're going to use clear_with_quad() for any reason, use it for
    * everything possible.
    */
   if (quad_buffers) {
      quad_buffers |= clear_buffers;
      clear_with_quad(ctx,
                      quad_buffers & PIPE_CLEAR_COLOR,
                      mask & BUFFER_BIT_DEPTH,
                      mask & BUFFER_BIT_STENCIL);
   } else if (clear_buffers)
      ctx->st->pipe->clear(ctx->st->pipe, clear_buffers, ctx->Color.ClearColor,
                           ctx->Depth.Clear, ctx->Stencil.Clear);

   if (mask & BUFFER_BIT_ACCUM)
      st_clear_accum_buffer(ctx,
                            ctx->DrawBuffer->Attachment[BUFFER_ACCUM].Renderbuffer);
}


void st_init_clear_functions(struct dd_function_table *functions)
{
   functions->Clear = st_clear;
}
