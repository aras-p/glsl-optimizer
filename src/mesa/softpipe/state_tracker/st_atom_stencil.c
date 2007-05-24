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
 

#include "st_context.h"
#include "st_atom.h"
#include "softpipe/sp_context.h"
#include "softpipe/sp_defines.h"


/**
 * Convert GLenum stencil func tokens to softpipe tokens.
 */
static GLuint
gl_stencil_func_to_sp(GLenum func)
{
   /* Same values, just biased */
   assert(SP_STENCIL_FUNC_NEVER == GL_NEVER - GL_NEVER);
   assert(SP_STENCIL_FUNC_LESS == GL_LESS - GL_NEVER);
   assert(SP_STENCIL_FUNC_EQUAL == GL_EQUAL - GL_NEVER);
   assert(SP_STENCIL_FUNC_LEQUAL == GL_LEQUAL - GL_NEVER);
   assert(SP_STENCIL_FUNC_GREATER == GL_GREATER - GL_NEVER);
   assert(SP_STENCIL_FUNC_NOTEQUAL == GL_NOTEQUAL - GL_NEVER);
   assert(SP_STENCIL_FUNC_GEQUAL == GL_GEQUAL - GL_NEVER);
   assert(SP_STENCIL_FUNC_ALWAYS == GL_ALWAYS - GL_NEVER);
   assert(func >= GL_NEVER);
   assert(func <= GL_ALWAYS);
   return func - GL_NEVER;
}


/**
 * Convert GLenum stencil op tokens to softpipe tokens.
 */
static GLuint
gl_stencil_op_to_sp(GLenum func)
{
   switch (func) {
   case GL_KEEP:
      return SP_STENCIL_OP_KEEP;
   case GL_ZERO:
      return SP_STENCIL_OP_ZERO;
   case GL_REPLACE:
      return SP_STENCIL_OP_REPLACE;
   case GL_INCR:
      return SP_STENCIL_OP_INCR;
   case GL_DECR:
      return SP_STENCIL_OP_DECR;
   case GL_INCR_WRAP:
      return SP_STENCIL_OP_INCR_WRAP;
   case GL_DECR_WRAP:
      return SP_STENCIL_OP_DECR_WRAP;
   case GL_INVERT:
      return SP_STENCIL_OP_INVERT;
   default:
      assert("invalid GL token in gl_stencil_op_to_sp()" == NULL);
      return 0;
   }
}


static void 
update_stencil( struct st_context *st )
{
   struct softpipe_stencil_state stencil;

   memset(&stencil, 0, sizeof(stencil));

   if (st->ctx->Stencil.Enabled) {
      stencil.front_enabled = 1;
      stencil.front_func = gl_stencil_func_to_sp(st->ctx->Stencil.Function[0]);
      stencil.front_fail_op = gl_stencil_op_to_sp(st->ctx->Stencil.FailFunc[0]);
      stencil.front_zfail_op = gl_stencil_op_to_sp(st->ctx->Stencil.ZFailFunc[0]);
      stencil.front_zpass_op = gl_stencil_op_to_sp(st->ctx->Stencil.ZPassFunc[0]);
      stencil.ref_value[0] = st->ctx->Stencil.Ref[0];
      stencil.value_mask[0] = st->ctx->Stencil.ValueMask[0];
      stencil.write_mask[0] = st->ctx->Stencil.WriteMask[0];
      if (st->ctx->Stencil.TestTwoSide) {
         stencil.back_enabled = 1;
         stencil.back_func = gl_stencil_func_to_sp(st->ctx->Stencil.Function[1]);
         stencil.back_fail_op = gl_stencil_op_to_sp(st->ctx->Stencil.FailFunc[1]);
         stencil.back_zfail_op = gl_stencil_op_to_sp(st->ctx->Stencil.ZFailFunc[1]);
         stencil.back_zpass_op = gl_stencil_op_to_sp(st->ctx->Stencil.ZPassFunc[1]);
         stencil.ref_value[1] = st->ctx->Stencil.Ref[1];
         stencil.value_mask[1] = st->ctx->Stencil.ValueMask[1];
         stencil.write_mask[1] = st->ctx->Stencil.WriteMask[1];
      }
      stencil.clear_value = st->ctx->Stencil.Clear;
   }

   if (memcmp(&stencil, &st->state.stencil, sizeof(stencil)) != 0) {
      /* state has changed */
      st->state.stencil = stencil;  /* struct copy */
      st->softpipe->set_stencil_state(st->softpipe, &stencil); /* set new state */
   }
}


const struct st_tracked_state st_update_stencil = {
   .dirty = {
      .mesa = (_NEW_STENCIL),
      .st  = 0,
   },
   .update = update_stencil
};





