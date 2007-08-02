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
#include "enums.h"
#include "st_atom.h"
#include "st_context.h"
#include "st_cb_clear.h"
#include "st_public.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "vf/vf.h"


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
clear_with_quad(GLcontext *ctx,
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
      stencil_test.write_mask[0] = ctx->Stencil.WriteMask[0];
   }
   st->pipe->set_stencil_state(st->pipe, &stencil_test);

   /* draw quad matching scissor rect (XXX verify coord round-off) */
   draw_quad(ctx,
             ctx->Scissor.X, ctx->Scissor.Y,
             ctx->Scissor.X + ctx->Scissor.Width,
             ctx->Scissor.Y + ctx->Scissor.Height,
             ctx->Depth.Clear, ctx->Color.ClearColor);

   /* Restore GL state */
   st->pipe->set_alpha_test_state(st->pipe, &st->state.alpha_test);
   st->pipe->set_blend_state(st->pipe, &st->state.blend);
   st->pipe->set_depth_state(st->pipe, &st->state.depth);
   st->pipe->set_setup_state(st->pipe, &st->state.setup);
   st->pipe->set_stencil_state(st->pipe, &st->state.stencil);
   /* OR:
   st_invalidate_state(ctx, _NEW_COLOR | _NEW_DEPTH | _NEW_STENCIL);
   */
}



/**
 * Called via ctx->Driver.Clear()
 * XXX: doesn't pick up the differences between front/back/left/right
 * clears.  Need to sort that out...
 */
static void st_clear(GLcontext *ctx, GLbitfield mask)
{
   struct st_context *st = ctx->st;
   GLboolean color = (mask & BUFFER_BITS_COLOR) ? GL_TRUE : GL_FALSE;
   GLboolean depth = (mask & BUFFER_BIT_DEPTH) ? GL_TRUE : GL_FALSE;
   GLboolean stencil = (mask & BUFFER_BIT_STENCIL) ? GL_TRUE : GL_FALSE;
   GLboolean accum = (mask & BUFFER_BIT_ACCUM) ? GL_TRUE : GL_FALSE;

   GLboolean maskColor, maskStencil;
   GLboolean fullscreen = !ctx->Scissor.Enabled;
   GLuint stencilMax = stencil ? (1 << ctx->DrawBuffer->_StencilBuffer->StencilBits) : 0;

   /* This makes sure the softpipe has the latest scissor, etc values */
   st_validate_state( st );

   maskColor = color && st->state.blend.colormask != PIPE_MASK_RGBA;
   maskStencil = stencil && ctx->Stencil.WriteMask[0] != stencilMax;

   if (fullscreen && !maskColor && !maskStencil) {
      /* pipe->clear() should clear a particular surface, so that we
       * can iterate over render buffers at this level and clear the
       * ones GL is asking for.  
       *
       * Will probably need something like pipe->clear_z_stencil() to
       * cope with the special case of paired and unpaired z/stencil
       * buffers, though could perhaps deal with them explicitly at
       * this level.
       */
      st->pipe->clear(st->pipe, color, depth, stencil);

      /* And here we would do a clear on whatever surface we are using
       * to implement accum buffers:
       */
      assert(!accum);
   }
   else {
      clear_with_quad(ctx, color, depth, stencil);
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

