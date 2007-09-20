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
  *   Zack  Rusin
  */
 

#include "st_context.h"
#include "st_cache.h"
#include "st_atom.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"


/**
 * Convert GLenum stencil func tokens to pipe tokens.
 */
static GLuint
gl_stencil_func_to_sp(GLenum func)
{
   /* Same values, just biased */
   assert(PIPE_FUNC_NEVER == GL_NEVER - GL_NEVER);
   assert(PIPE_FUNC_LESS == GL_LESS - GL_NEVER);
   assert(PIPE_FUNC_EQUAL == GL_EQUAL - GL_NEVER);
   assert(PIPE_FUNC_LEQUAL == GL_LEQUAL - GL_NEVER);
   assert(PIPE_FUNC_GREATER == GL_GREATER - GL_NEVER);
   assert(PIPE_FUNC_NOTEQUAL == GL_NOTEQUAL - GL_NEVER);
   assert(PIPE_FUNC_GEQUAL == GL_GEQUAL - GL_NEVER);
   assert(PIPE_FUNC_ALWAYS == GL_ALWAYS - GL_NEVER);
   assert(func >= GL_NEVER);
   assert(func <= GL_ALWAYS);
   return func - GL_NEVER;
}


/**
 * Convert GLenum stencil op tokens to pipe tokens.
 */
static GLuint
gl_stencil_op_to_sp(GLenum func)
{
   switch (func) {
   case GL_KEEP:
      return PIPE_STENCIL_OP_KEEP;
   case GL_ZERO:
      return PIPE_STENCIL_OP_ZERO;
   case GL_REPLACE:
      return PIPE_STENCIL_OP_REPLACE;
   case GL_INCR:
      return PIPE_STENCIL_OP_INCR;
   case GL_DECR:
      return PIPE_STENCIL_OP_DECR;
   case GL_INCR_WRAP:
      return PIPE_STENCIL_OP_INCR_WRAP;
   case GL_DECR_WRAP:
      return PIPE_STENCIL_OP_DECR_WRAP;
   case GL_INVERT:
      return PIPE_STENCIL_OP_INVERT;
   default:
      assert("invalid GL token in gl_stencil_op_to_sp()" == NULL);
      return 0;
   }
}

/**
 * Convert GLenum depth func tokens to pipe tokens.
 */
static GLuint
gl_depth_func_to_sp(GLenum func)
{
   /* Same values, just biased */
   assert(PIPE_FUNC_NEVER == GL_NEVER - GL_NEVER);
   assert(PIPE_FUNC_LESS == GL_LESS - GL_NEVER);
   assert(PIPE_FUNC_EQUAL == GL_EQUAL - GL_NEVER);
   assert(PIPE_FUNC_LEQUAL == GL_LEQUAL - GL_NEVER);
   assert(PIPE_FUNC_GREATER == GL_GREATER - GL_NEVER);
   assert(PIPE_FUNC_NOTEQUAL == GL_NOTEQUAL - GL_NEVER);
   assert(PIPE_FUNC_GEQUAL == GL_GEQUAL - GL_NEVER);
   assert(PIPE_FUNC_ALWAYS == GL_ALWAYS - GL_NEVER);
   assert(func >= GL_NEVER);
   assert(func <= GL_ALWAYS);
   return func - GL_NEVER;
}


static void
update_depth_stencil(struct st_context *st)
{
   struct pipe_depth_stencil_state depth_stencil;
   const struct cso_depth_stencil *cso;

   memset(&depth_stencil, 0, sizeof(depth_stencil));

   depth_stencil.depth.enabled = st->ctx->Depth.Test;
   depth_stencil.depth.writemask = st->ctx->Depth.Mask;
   depth_stencil.depth.func = gl_depth_func_to_sp(st->ctx->Depth.Func);
   depth_stencil.depth.clear = st->ctx->Depth.Clear;

   if (st->ctx->Query.CurrentOcclusionObject &&
       st->ctx->Query.CurrentOcclusionObject->Active)
      depth_stencil.depth.occlusion_count = 1;

   if (st->ctx->Stencil.Enabled) {
      depth_stencil.stencil.front_enabled = 1;
      depth_stencil.stencil.front_func = gl_stencil_func_to_sp(st->ctx->Stencil.Function[0]);
      depth_stencil.stencil.front_fail_op = gl_stencil_op_to_sp(st->ctx->Stencil.FailFunc[0]);
      depth_stencil.stencil.front_zfail_op = gl_stencil_op_to_sp(st->ctx->Stencil.ZFailFunc[0]);
      depth_stencil.stencil.front_zpass_op = gl_stencil_op_to_sp(st->ctx->Stencil.ZPassFunc[0]);
      depth_stencil.stencil.ref_value[0] = st->ctx->Stencil.Ref[0] & 0xff;
      depth_stencil.stencil.value_mask[0] = st->ctx->Stencil.ValueMask[0] & 0xff;
      depth_stencil.stencil.write_mask[0] = st->ctx->Stencil.WriteMask[0] & 0xff;
      if (st->ctx->Stencil.TestTwoSide) {
         depth_stencil.stencil.back_enabled = 1;
         depth_stencil.stencil.back_func = gl_stencil_func_to_sp(st->ctx->Stencil.Function[1]);
         depth_stencil.stencil.back_fail_op = gl_stencil_op_to_sp(st->ctx->Stencil.FailFunc[1]);
         depth_stencil.stencil.back_zfail_op = gl_stencil_op_to_sp(st->ctx->Stencil.ZFailFunc[1]);
         depth_stencil.stencil.back_zpass_op = gl_stencil_op_to_sp(st->ctx->Stencil.ZPassFunc[1]);
         depth_stencil.stencil.ref_value[1] = st->ctx->Stencil.Ref[1] & 0xff;
         depth_stencil.stencil.value_mask[1] = st->ctx->Stencil.ValueMask[1] & 0xff;
         depth_stencil.stencil.write_mask[1] = st->ctx->Stencil.WriteMask[1] & 0xff;
      }
      depth_stencil.stencil.clear_value = st->ctx->Stencil.Clear & 0xff;
   }

   cso = st_cached_depth_stencil_state(st, &depth_stencil);
   if (st->state.depth_stencil != cso) {
      /* state has changed */
      st->state.depth_stencil = cso;
      st->pipe->bind_depth_stencil_state(st->pipe, cso->data); /* bind new state */
   }
}


const struct st_tracked_state st_update_depth_stencil = {
   .name = "st_update_depth_stencil",
   .dirty = {
      .mesa = (_NEW_DEPTH|_NEW_STENCIL),
      .st  = 0,
   },
   .update = update_depth_stencil
};
