/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Brian Paul
  */

#include "main/glheader.h"
#include "main/macros.h"
#include "shader/prog_instruction.h"
#include "st_atom.h"
#include "st_context.h"
#include "st_cb_accum.h"
#include "st_cb_clear.h"
#include "st_cb_fbo.h"
#include "st_draw.h"
#include "st_program.h"
#include "st_public.h"
#include "st_mesa_to_tgsi.h"

#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"
#include "util/u_pack_color.h"
#include "util/u_simple_shaders.h"
#include "util/u_draw_quad.h"

#include "cso_cache/cso_context.h"


/* XXX for testing draw module vertex passthrough: */
/* XXX this hack is broken now */
#define TEST_DRAW_PASSTHROUGH 0


void
st_destroy_clear(struct st_context *st)
{
   struct pipe_context *pipe = st->pipe;

   if (st->clear.fs) {
      pipe->delete_fs_state(pipe, st->clear.fs);
      st->clear.fs = NULL;
   }
   if (st->clear.vs) {
      pipe->delete_vs_state(pipe, st->clear.vs);
      st->clear.vs = NULL;
   }
   if (st->clear.vbuf) {
      pipe->winsys->buffer_destroy(pipe->winsys, st->clear.vbuf);
      st->clear.vbuf = NULL;
   }
}


static GLboolean
is_depth_stencil_format(enum pipe_format pipeFormat)
{
   switch (pipeFormat) {
   case PIPE_FORMAT_S8Z24_UNORM:
   case PIPE_FORMAT_Z24S8_UNORM:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}



/**
 * Draw a screen-aligned quadrilateral.
 * Coords are window coords with y=0=bottom.  These coords will be transformed
 * by the vertex shader and viewport transform (which will flip Y if needed).
 */
static void
draw_quad(GLcontext *ctx,
          float x0, float y0, float x1, float y1, GLfloat z,
          const GLfloat color[4])
{
   struct st_context *st = ctx->st;
   struct pipe_context *pipe = st->pipe;
   GLuint i;
   void *buf;

   if (!st->clear.vbuf) {
      st->clear.vbuf = pipe->winsys->buffer_create(pipe->winsys, 32,
                                                   PIPE_BUFFER_USAGE_VERTEX,
                                                   sizeof(st->clear.vertices));
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
   buf = pipe->winsys->buffer_map(pipe->winsys, st->clear.vbuf,
                                  PIPE_BUFFER_USAGE_CPU_WRITE);
   memcpy(buf, st->clear.vertices, sizeof(st->clear.vertices));
   pipe->winsys->buffer_unmap(pipe->winsys, st->clear.vbuf);

   /* draw */
   util_draw_vertex_buffer(pipe, st->clear.vbuf,
                           PIPE_PRIM_TRIANGLE_FAN,
                           4,  /* verts */
                           2); /* attribs/vert */
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
   struct pipe_context *pipe = st->pipe;
   const GLfloat x0 = ctx->DrawBuffer->_Xmin;
   const GLfloat y0 = ctx->DrawBuffer->_Ymin;
   const GLfloat x1 = ctx->DrawBuffer->_Xmax;
   const GLfloat y1 = ctx->DrawBuffer->_Ymax;

   /*
   printf("%s %s%s%s %f,%f %f,%f\n", __FUNCTION__, 
	  color ? "color, " : "",
	  depth ? "depth, " : "",
	  stencil ? "stencil" : "",
	  x0, y0,
	  x1, y1);
   */

   cso_save_blend(st->cso_context);
   cso_save_depth_stencil_alpha(st->cso_context);
   cso_save_rasterizer(st->cso_context);
   cso_save_viewport(st->cso_context);

   /* blend state: RGBA masking */
   {
      struct pipe_blend_state blend;
      memset(&blend, 0, sizeof(blend));
      blend.rgb_src_factor = PIPE_BLENDFACTOR_ONE;
      blend.alpha_src_factor = PIPE_BLENDFACTOR_ONE;
      blend.rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
      blend.alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
      if (color) {
         if (ctx->Color.ColorMask[0])
            blend.colormask |= PIPE_MASK_R;
         if (ctx->Color.ColorMask[1])
            blend.colormask |= PIPE_MASK_G;
         if (ctx->Color.ColorMask[2])
            blend.colormask |= PIPE_MASK_B;
         if (ctx->Color.ColorMask[3])
            blend.colormask |= PIPE_MASK_A;
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
         depth_stencil.stencil[0].enabled = 1;
         depth_stencil.stencil[0].func = PIPE_FUNC_ALWAYS;
         depth_stencil.stencil[0].fail_op = PIPE_STENCIL_OP_REPLACE;
         depth_stencil.stencil[0].zpass_op = PIPE_STENCIL_OP_REPLACE;
         depth_stencil.stencil[0].zfail_op = PIPE_STENCIL_OP_REPLACE;
         depth_stencil.stencil[0].ref_value = ctx->Stencil.Clear;
         depth_stencil.stencil[0].value_mask = 0xff;
         depth_stencil.stencil[0].write_mask = ctx->Stencil.WriteMask[0] & 0xff;
      }

      cso_set_depth_stencil_alpha(st->cso_context, &depth_stencil);
   }

   /* rasterizer state: bypass clipping */
   {
      struct pipe_rasterizer_state raster;
      memset(&raster, 0, sizeof(raster));
      raster.bypass_clipping = 1;
#if TEST_DRAW_PASSTHROUGH
      raster.bypass_vs = 1;
#endif
      cso_set_rasterizer(st->cso_context, &raster);
   }

   /* fragment shader state: color pass-through program */
   if (!st->clear.fs) {
      st->clear.fs = util_make_fragment_passthrough_shader(pipe);
   }
   pipe->bind_fs_state(pipe, st->clear.fs);


#if !TEST_DRAW_PASSTHROUGH
   /* vertex shader state: color/position pass-through */
   if (!st->clear.vs) {
      const uint semantic_names[] = { TGSI_SEMANTIC_POSITION,
                                      TGSI_SEMANTIC_COLOR };
      const uint semantic_indexes[] = { 0, 0 };
      st->clear.vs = util_make_vertex_passthrough_shader(pipe, 2,
                                                         semantic_names,
                                                         semantic_indexes);
   }
   pipe->bind_vs_state(pipe, st->clear.vs);
#endif

#if !TEST_DRAW_PASSTHROUGH
   /* viewport state: viewport matching window dims */
   {
      const float width = ctx->DrawBuffer->Width;
      const float height = ctx->DrawBuffer->Height;
      struct pipe_viewport_state vp;
      vp.scale[0] =  0.5 * width;
      vp.scale[1] = -0.5 * height;
      vp.scale[2] = 1.0;
      vp.scale[3] = 1.0;
      vp.translate[0] = 0.5 * width;
      vp.translate[1] = 0.5 * height;
      vp.translate[2] = 0.0;
      vp.translate[3] = 0.0;
      cso_set_viewport(st->cso_context, &vp);
   }
#endif

   /* draw quad matching scissor rect (XXX verify coord round-off) */
   draw_quad(ctx, x0, y0, x1, y1, ctx->Depth.Clear, ctx->Color.ClearColor);

   /* Restore pipe state */
   cso_restore_blend(st->cso_context);
   cso_restore_depth_stencil_alpha(st->cso_context);
   cso_restore_rasterizer(st->cso_context);
   cso_restore_viewport(st->cso_context);
   /* these don't go through cso yet */
   pipe->bind_fs_state(pipe, st->fp->driver_shader);
   pipe->bind_vs_state(pipe, st->vp->driver_shader);
}


/**
 * Determine if we need to clear the depth buffer by drawing a quad.
 */
static INLINE GLboolean
check_clear_color_with_quad(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   const struct st_renderbuffer *strb = st_renderbuffer(rb);

   if (strb->surface->status == PIPE_SURFACE_STATUS_UNDEFINED)
      return FALSE;

   if (ctx->Scissor.Enabled)
      return TRUE;

   if (!ctx->Color.ColorMask[0] ||
       !ctx->Color.ColorMask[1] ||
       !ctx->Color.ColorMask[2] ||
       !ctx->Color.ColorMask[3])
      return TRUE;

   return FALSE;
}


static INLINE GLboolean
check_clear_depth_stencil_with_quad(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   const struct st_renderbuffer *strb = st_renderbuffer(rb);
   const GLuint stencilMax = (1 << rb->StencilBits) - 1;
   GLboolean maskStencil
      = (ctx->Stencil.WriteMask[0] & stencilMax) != stencilMax;

   if (strb->surface->status == PIPE_SURFACE_STATUS_UNDEFINED)
      return FALSE;

   if (ctx->Scissor.Enabled)
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
   const GLboolean isDS = is_depth_stencil_format(strb->surface->format);

   if (strb->surface->status == PIPE_SURFACE_STATUS_UNDEFINED)
      return FALSE;

   if (ctx->Scissor.Enabled)
      return TRUE;

   if (isDS && 
       strb->surface->status == PIPE_SURFACE_STATUS_DEFINED &&
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
   const GLboolean isDS = is_depth_stencil_format(strb->surface->format);
   const GLuint stencilMax = (1 << rb->StencilBits) - 1;
   const GLboolean maskStencil
      = (ctx->Stencil.WriteMask[0] & stencilMax) != stencilMax;

   if (strb->surface->status == PIPE_SURFACE_STATUS_UNDEFINED)
      return FALSE;

   if (maskStencil) 
      return TRUE;

   if (ctx->Scissor.Enabled)
      return TRUE;

   /* This is correct, but it is necessary to look at the depth clear
    * value held in the surface when it comes time to issue the clear,
    * rather than taking depth and stencil clear values from the
    * current state.
    */
   if (isDS && 
       strb->surface->status == PIPE_SURFACE_STATUS_DEFINED &&
       ctx->DrawBuffer->Visual.depthBits > 0)
      return TRUE;

   return FALSE;
}



static void
clear_color_buffer(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   if (check_clear_color_with_quad( ctx, rb )) {
      /* masking or scissoring */
      clear_with_quad(ctx, GL_TRUE, GL_FALSE, GL_FALSE);
   }
   else {
      /* clear whole buffer w/out masking */
      struct st_renderbuffer *strb = st_renderbuffer(rb);
      uint clearValue;
      util_pack_color(ctx->Color.ClearColor, strb->surface->format, &clearValue);
      ctx->st->pipe->clear(ctx->st->pipe, strb->surface, clearValue);
   }
}


static void
clear_depth_buffer(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   if (check_clear_depth_with_quad(ctx, rb)) {
      /* scissoring or we have a combined depth/stencil buffer */
      clear_with_quad(ctx, GL_FALSE, GL_TRUE, GL_FALSE);
   }
   else {
      struct st_renderbuffer *strb = st_renderbuffer(rb);

      /* simple clear of whole buffer */
      uint clearValue = util_pack_z(strb->surface->format, ctx->Depth.Clear);
      ctx->st->pipe->clear(ctx->st->pipe, strb->surface, clearValue);
   }
}


static void
clear_stencil_buffer(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   if (check_clear_stencil_with_quad(ctx, rb)) {
      /* masking or scissoring or combined depth/stencil buffer */
      clear_with_quad(ctx, GL_FALSE, GL_FALSE, GL_TRUE);
   }
   else {
      struct st_renderbuffer *strb = st_renderbuffer(rb);

      /* simple clear of whole buffer */
      GLuint clearValue = ctx->Stencil.Clear;

      switch (strb->surface->format) {
      case PIPE_FORMAT_S8Z24_UNORM:
         clearValue <<= 24;
         break;
      default:
         ; /* no-op, stencil value is in least significant bits */
      }  

      ctx->st->pipe->clear(ctx->st->pipe, strb->surface, clearValue);
   }
}


static void
clear_depth_stencil_buffer(GLcontext *ctx, struct gl_renderbuffer *rb)
{

   if (check_clear_depth_stencil_with_quad(ctx, rb)) {
      /* masking or scissoring */
      clear_with_quad(ctx, GL_FALSE, GL_TRUE, GL_TRUE);
   }
   else {
      struct st_renderbuffer *strb = st_renderbuffer(rb);

      /* clear whole buffer w/out masking */
      GLuint clearValue = util_pack_z(strb->surface->format, ctx->Depth.Clear);

      switch (strb->surface->format) {
      case PIPE_FORMAT_S8Z24_UNORM:
         clearValue |= ctx->Stencil.Clear << 24;
         break;
      case PIPE_FORMAT_Z24S8_UNORM:
         clearValue |= ctx->Stencil.Clear;
         break;
      default:
         assert(0);
      }  

      ctx->st->pipe->clear(ctx->st->pipe, strb->surface, clearValue);
   }
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
   GLbitfield cmask = mask & BUFFER_BITS_COLOR;

   /* This makes sure the softpipe has the latest scissor, etc values */
   st_validate_state( st );

   /*
    * XXX TO-DO:
    * If we're going to use clear_with_quad() for any reason, use it to
    * clear as many other buffers as possible.
    * As it is now, we sometimes call clear_with_quad() three times to clear
    * color/depth/stencil individually...
    */

   if (cmask) {
      GLuint b;
      for (b = 0; cmask; b++) {
         if (cmask & (1 << b)) {
            struct gl_renderbuffer *rb
               = ctx->DrawBuffer->Attachment[b].Renderbuffer;
            assert(rb);
            clear_color_buffer(ctx, rb);
            cmask &= ~(1 << b); /* turn off bit */
         }
         assert(b < BUFFER_COUNT);
      }
   }

   if (mask & BUFFER_BIT_ACCUM) {
      st_clear_accum_buffer(ctx,
                       ctx->DrawBuffer->Attachment[BUFFER_ACCUM].Renderbuffer);
   }

   if ((mask & BUFFER_BITS_DS) == BUFFER_BITS_DS && depthRb == stencilRb) {
      /* clearing combined depth + stencil */
      clear_depth_stencil_buffer(ctx, depthRb);
   }
   else {
      /* separate depth/stencil clears */
      if (mask & BUFFER_BIT_DEPTH) {
         clear_depth_buffer(ctx, depthRb);
      }
      if (mask & BUFFER_BIT_STENCIL) {
         clear_stencil_buffer(ctx, stencilRb);
      }
   }
}


void st_init_clear_functions(struct dd_function_table *functions)
{
   functions->Clear = st_clear;
}
