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

#include "glheader.h"
#include "macros.h"
#include "st_atom.h"
#include "st_context.h"
#include "st_cb_clear.h"
#include "st_public.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "vf/vf.h"



static GLuint
color_value(GLuint pipeFormat, const GLfloat color[4])
{
   GLubyte r, g, b, a;

   UNCLAMPED_FLOAT_TO_UBYTE(r, color[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(g, color[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(b, color[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(a, color[3]);

   switch (pipeFormat) {
   case PIPE_FORMAT_U_R8_G8_B8_A8:
      return (r << 24) | (g << 16) | (b << 8) | a;
   case PIPE_FORMAT_U_A8_R8_G8_B8:
      return (a << 24) | (r << 16) | (g << 8) | b;
   case PIPE_FORMAT_U_R5_G6_B5:
      return ((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3);
   default:
      return 0;
   }
}
 

static GLuint
depth_value(GLuint pipeFormat, GLfloat value)
{
   GLuint val;
   switch (pipeFormat) {
   case PIPE_FORMAT_U_Z16:
      val = (GLuint) (value * 0xffffff);
      break;
   case PIPE_FORMAT_U_Z32:
      val = (GLuint) (value * 0xffffffff);
      break;
   case PIPE_FORMAT_S8_Z24:
   /*case PIPE_FORMAT_Z24_S8:*/
      val = (GLuint) (value * 0xffffff);
      break;
   default:
      assert(0);
   }
   return val;
}


static GLboolean
is_depth_stencil_format(GLuint pipeFormat)
{
   switch (pipeFormat) {
   case PIPE_FORMAT_S8_Z24:
   /*case PIPE_FORMAT_Z24_S8:*/
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


/**
 * Draw a screen-aligned quadrilateral.
 * Coords are window coords.
 */
static void
draw_quad(GLcontext *ctx,
          float x0, float y0, float x1, float y1, GLfloat z,
          const GLfloat color[4])
{
   static const GLuint attribs[2] = {
      VF_ATTRIB_POS,
      VF_ATTRIB_COLOR0
   };
   GLfloat verts[4][2][4]; /* four verts, two attribs, XYZW */
   GLuint i;

   /* positions */
   verts[0][0][0] = x0;
   verts[0][0][1] = y0;

   verts[1][0][0] = x1;
   verts[1][0][1] = y0;

   verts[2][0][0] = x1;
   verts[2][0][1] = y1;

   verts[3][0][0] = x0;
   verts[3][0][1] = y1;

   /* same for all verts: */
   for (i = 0; i < 4; i++) {
      verts[i][0][2] = z;
      verts[i][0][3] = 1.0;
      verts[i][1][0] = color[0];
      verts[i][1][1] = color[1];
      verts[i][1][2] = color[2];
      verts[i][1][3] = color[3];
   }

   ctx->st->pipe->draw_vertices(ctx->st->pipe, GL_QUADS,
                                4, (GLfloat *) verts, 2, attribs);
}



/**
 * Do glClear by drawing a quadrilateral.
 */
static void
clear_with_quad(GLcontext *ctx, GLuint x0, GLuint y0,
                GLuint x1, GLuint y1,
                GLboolean color, GLboolean depth, GLboolean stencil)
{
   struct st_context *st = ctx->st;
   struct pipe_alpha_test_state alpha_test;
   struct pipe_blend_state blend;
   struct pipe_depth_state depth_test;
   struct pipe_stencil_state stencil_test;
   struct pipe_setup_state setup;

   /* alpha state: disabled */
   memset(&alpha_test, 0, sizeof(alpha_test));
   st->pipe->set_alpha_test_state(st->pipe, &alpha_test);

   /* blend state: RGBA masking */
   memset(&blend, 0, sizeof(blend));
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
   st->pipe->set_blend_state(st->pipe, &blend);

   /* depth state: always pass */
   memset(&depth_test, 0, sizeof(depth_test));
   if (depth) {
      depth_test.enabled = 1;
      depth_test.writemask = 1;
      depth_test.func = PIPE_FUNC_ALWAYS;
   }
   st->pipe->set_depth_state(st->pipe, &depth_test);

   /* setup state: nothing */
   memset(&setup, 0, sizeof(setup));
   if (ctx->Scissor.Enabled)
      setup.scissor = 1;
   st->pipe->set_setup_state(st->pipe, &setup);

   /* stencil state: always set to ref value */
   memset(&stencil_test, 0, sizeof(stencil_test));
   if (stencil) {
      stencil_test.front_enabled = 1;
      stencil_test.front_func = PIPE_FUNC_ALWAYS;
      stencil_test.front_fail_op = PIPE_STENCIL_OP_REPLACE;
      stencil_test.front_zpass_op = PIPE_STENCIL_OP_REPLACE;
      stencil_test.front_zfail_op = PIPE_STENCIL_OP_REPLACE;
      stencil_test.ref_value[0] = ctx->Stencil.Clear;
      stencil_test.value_mask[0] = 0xff;
      stencil_test.write_mask[0] = ctx->Stencil.WriteMask[0] & 0xff;
   }
   st->pipe->set_stencil_state(st->pipe, &stencil_test);

   /* draw quad matching scissor rect (XXX verify coord round-off) */
   draw_quad(ctx, x0, y0, x1, y1, ctx->Depth.Clear, ctx->Color.ClearColor);

   /* Restore pipe state */
   st->pipe->set_alpha_test_state(st->pipe, &st->state.alpha_test);
   st->pipe->set_blend_state(st->pipe, &st->state.blend);
   st->pipe->set_depth_state(st->pipe, &st->state.depth);
   st->pipe->set_setup_state(st->pipe, &st->state.setup);
   st->pipe->set_stencil_state(st->pipe, &st->state.stencil);
   /* OR:
   st_invalidate_state(ctx, _NEW_COLOR | _NEW_DEPTH | _NEW_STENCIL);
   */
}


static void
clear_color_buffer(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   if (ctx->Color.ColorMask[0] &&
       ctx->Color.ColorMask[1] &&
       ctx->Color.ColorMask[2] &&
       ctx->Color.ColorMask[3] &&
       !ctx->Scissor.Enabled)
   {
      /* clear whole buffer w/out masking */
      GLuint clearValue
         = color_value(rb->surface->format, ctx->Color.ClearColor);
      ctx->st->pipe->clear(ctx->st->pipe, rb->surface, clearValue);
   }
   else {
      /* masking or scissoring */
      clear_with_quad(ctx,
                      ctx->DrawBuffer->_Xmin,
                      ctx->DrawBuffer->_Xmin,
                      ctx->DrawBuffer->_Xmax,
                      ctx->DrawBuffer->_Ymax,
                      GL_TRUE, GL_FALSE, GL_FALSE);
   }
}


static void
clear_accum_buffer(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   if (!ctx->Scissor.Enabled) {
      /* clear whole buffer w/out masking */
      GLuint clearValue
         = color_value(rb->surface->format, ctx->Accum.ClearColor);
      /* Note that clearValue is 32 bits but the accum buffer will
       * typically be 64bpp...
       */
      ctx->st->pipe->clear(ctx->st->pipe, rb->surface, clearValue);
   }
   else {
      /* scissoring */
      /* XXX point framebuffer.cbufs[0] at the accum buffer */
      clear_with_quad(ctx,
                      ctx->DrawBuffer->_Xmin,
                      ctx->DrawBuffer->_Xmin,
                      ctx->DrawBuffer->_Xmax,
                      ctx->DrawBuffer->_Ymax,
                      GL_TRUE, GL_FALSE, GL_FALSE);
   }
}


static void
clear_depth_buffer(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   if (!ctx->Scissor.Enabled &&
       !is_depth_stencil_format(rb->surface->format)) {
      /* clear whole depth buffer w/out masking */
      GLuint clearValue = depth_value(rb->surface->format, ctx->Depth.Clear);
      ctx->st->pipe->clear(ctx->st->pipe, rb->surface, clearValue);
   }
   else {
      /* masking or scissoring or combined z/stencil buffer */
      clear_with_quad(ctx,
                      ctx->DrawBuffer->_Xmin,
                      ctx->DrawBuffer->_Xmin,
                      ctx->DrawBuffer->_Xmax,
                      ctx->DrawBuffer->_Ymax,
                      GL_FALSE, GL_TRUE, GL_FALSE);
   }
}


static void
clear_stencil_buffer(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   const GLuint stencilMax = (1 << rb->StencilBits) - 1;
   GLboolean maskStencil = ctx->Stencil.WriteMask[0] != stencilMax;

   if (!maskStencil && !ctx->Scissor.Enabled &&
       !is_depth_stencil_format(rb->surface->format)) {
      /* clear whole stencil buffer w/out masking */
      GLuint clearValue = ctx->Stencil.Clear;
      ctx->st->pipe->clear(ctx->st->pipe, rb->surface, clearValue);
   }
   else {
      /* masking or scissoring */
      clear_with_quad(ctx,
                      ctx->DrawBuffer->_Xmin,
                      ctx->DrawBuffer->_Xmin,
                      ctx->DrawBuffer->_Xmax,
                      ctx->DrawBuffer->_Ymax,
                      GL_FALSE, GL_FALSE, GL_TRUE);
   }
}


static void
clear_depth_stencil_buffer(GLcontext *ctx, struct gl_renderbuffer *rb)
{
   const GLuint stencilMax = 1 << rb->StencilBits;
   GLboolean maskStencil = ctx->Stencil.WriteMask[0] != stencilMax;

   assert(is_depth_stencil_format(rb->surface->format));

   if (!maskStencil && !ctx->Scissor.Enabled) {
      /* clear whole buffer w/out masking */
      GLuint clearValue = depth_value(rb->surface->format, ctx->Depth.Clear);

      switch (rb->surface->format) {
      case PIPE_FORMAT_S8_Z24:
         clearValue |= ctx->Stencil.Clear << 24;
         break;
#if 0
      case PIPE_FORMAT_Z24_S8:
         clearValue = (clearValue << 8) | clearVal;
         break;
#endif
      default:
         assert(0);
      }  

      ctx->st->pipe->clear(ctx->st->pipe, rb->surface, clearValue);
   }
   else {
      /* masking or scissoring */
      clear_with_quad(ctx,
                      ctx->DrawBuffer->_Xmin,
                      ctx->DrawBuffer->_Xmin,
                      ctx->DrawBuffer->_Xmax,
                      ctx->DrawBuffer->_Ymax,
                      GL_FALSE, GL_TRUE, GL_TRUE);
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

   /* This makes sure the softpipe has the latest scissor, etc values */
   st_validate_state( st );

   /*
    * XXX TO-DO:
    * If we're going to use clear_with_quad() for any reason, use it to
    * clear as many other buffers as possible.
    * As it is now, we sometimes call clear_with_quad() three times to clear
    * color/depth/stencil individually...
    */

   if (mask & BUFFER_BITS_COLOR) {
      GLuint b;
      for (b = 0; b < BUFFER_COUNT; b++) {
         if (BUFFER_BITS_COLOR & mask & (1 << b)) {
            clear_color_buffer(ctx,
                               ctx->DrawBuffer->Attachment[b].Renderbuffer);
         }
      }
   }

   if (mask & BUFFER_BIT_ACCUM) {
      clear_accum_buffer(ctx,
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


void st_init_cb_clear( struct st_context *st )
{
   struct dd_function_table *functions = &st->ctx->Driver;

   functions->Clear = st_clear;
}


void st_destroy_cb_clear( struct st_context *st )
{
}

