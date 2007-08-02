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



/**
 * Do glClear by drawing a quadrilateral.
 */
static void
clear_with_quad(GLcontext *ctx,
                GLboolean color, GLboolean depth,
                GLboolean stencil, GLboolean accum)
{
   struct st_context *st = ctx->st;
   struct pipe_blend_state blend;
   struct pipe_depth_state depth_test;
   struct pipe_stencil_state stencil_test;
   GLfloat z = ctx->Depth.Clear;

   /* depth state: always pass */
   memset(&depth_test, 0, sizeof(depth));
   if (depth) {
      depth_test.enabled = 1;
      depth_test.writemask = 1;
      depth_test.func = PIPE_FUNC_ALWAYS;
   }
   st->pipe->set_depth_state(st->pipe, &depth_test);

   /* stencil state: always set to ref value */
   memset(&stencil_test, 0, sizeof(stencil));
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


   /*
    * XXX Render quad here
    */

   /* Restore GL state */
   st_invalidate_state(ctx, _NEW_COLOR | _NEW_DEPTH | _NEW_STENCIL);
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
   GLboolean fullscreen = 1;	/* :-) */
   GLuint stencilMax = 1 << ctx->DrawBuffer->_StencilBuffer->StencilBits;

   /* This makes sure the softpipe has the latest scissor, etc values */
   st_validate_state( st );

   maskColor = st->state.blend.colormask != PIPE_MASK_RGBA;
   maskStencil = ctx->Stencil.WriteMask[0] != stencilMax;

   if (fullscreen && !maskColor) {
      /* pipe->clear() should clear a particular surface, so that we
       * can iterate over render buffers at this level and clear the
       * ones GL is asking for.  
       *
       * Will probably need something like pipe->clear_z_stencil() to
       * cope with the special case of paired and unpaired z/stencil
       * buffers, though could perhaps deal with them explicitly at
       * this level.
       */
      st->pipe->clear(st->pipe, color, depth, stencil, accum);
   }
   else {
      /* Convert to geometry, etc:
       */
      clear_with_quad(ctx, color, depth, stencil, accum);
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

