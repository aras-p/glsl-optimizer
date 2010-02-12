/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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
  */

#include "main/glheader.h"
#include "main/macros.h"
#include "main/enums.h"
#include "shader/prog_instruction.h"
#include "shader/prog_parameter.h"
#include "shader/program.h"
#include "shader/shader_api.h"

#include "cso_cache/cso_context.h"
#include "draw/draw_context.h"

#include "st_context.h"
#include "st_program.h"
#include "st_mesa_to_tgsi.h"
#include "st_cb_program.h"


static GLuint SerialNo = 1;


/**
 * Called via ctx->Driver.BindProgram() to bind an ARB vertex or
 * fragment program.
 */
static void st_bind_program( GLcontext *ctx,
			     GLenum target, 
			     struct gl_program *prog )
{
   struct st_context *st = st_context(ctx);

   switch (target) {
   case GL_VERTEX_PROGRAM_ARB: 
      st->dirty.st |= ST_NEW_VERTEX_PROGRAM;
      break;
   case GL_FRAGMENT_PROGRAM_ARB:
      st->dirty.st |= ST_NEW_FRAGMENT_PROGRAM;
      break;
   }
}


/**
 * Called via ctx->Driver.UseProgram() to bind a linked GLSL program
 * (vertex shader + fragment shader).
 */
static void st_use_program( GLcontext *ctx,
			    GLuint program )
{
   struct st_context *st = st_context(ctx);

   st->dirty.st |= ST_NEW_FRAGMENT_PROGRAM;
   st->dirty.st |= ST_NEW_VERTEX_PROGRAM;

   _mesa_use_program(ctx, program);
}



/**
 * Called via ctx->Driver.NewProgram() to allocate a new vertex or
 * fragment program.
 */
static struct gl_program *st_new_program( GLcontext *ctx,
					  GLenum target,
					  GLuint id )
{
   switch (target) {
   case GL_VERTEX_PROGRAM_ARB: {
      struct st_vertex_program *prog = ST_CALLOC_STRUCT(st_vertex_program);

      prog->serialNo = SerialNo++;

      return _mesa_init_vertex_program( ctx, 
					&prog->Base,
					target, 
					id );
   }

   case GL_FRAGMENT_PROGRAM_ARB:
   case GL_FRAGMENT_PROGRAM_NV: {
      struct st_fragment_program *prog = ST_CALLOC_STRUCT(st_fragment_program);

      prog->serialNo = SerialNo++;

      return _mesa_init_fragment_program( ctx, 
					  &prog->Base,
					  target, 
					  id );
   }

   default:
      assert(0);
      return NULL;
   }
}


void
st_delete_program(GLcontext *ctx, struct gl_program *prog)
{
   struct st_context *st = st_context(ctx);

   switch( prog->Target ) {
   case GL_VERTEX_PROGRAM_ARB:
      {
         struct st_vertex_program *stvp = (struct st_vertex_program *) prog;
         st_vp_release_varients( st, stvp );
      }
      break;
   case GL_FRAGMENT_PROGRAM_ARB:
      {
         struct st_fragment_program *stfp = (struct st_fragment_program *) prog;

         if (stfp->driver_shader) {
            cso_delete_fragment_shader(st->cso_context, stfp->driver_shader);
            stfp->driver_shader = NULL;
         }
         
         if (stfp->tgsi.tokens) {
            st_free_tokens(stfp->tgsi.tokens);
            stfp->tgsi.tokens = NULL;
         }

         if (stfp->bitmap_program) {
            struct gl_program *prg = &stfp->bitmap_program->Base.Base;
            _mesa_reference_program(ctx, &prg, NULL);
            stfp->bitmap_program = NULL;
         }
      }
      break;
   default:
      assert(0); /* problem */
   }

   /* delete base class */
   _mesa_delete_program( ctx, prog );
}


static GLboolean st_is_program_native( GLcontext *ctx,
				       GLenum target, 
				       struct gl_program *prog )
{
   return GL_TRUE;
}


static GLboolean st_program_string_notify( GLcontext *ctx,
                                           GLenum target,
                                           struct gl_program *prog )
{
   struct st_context *st = st_context(ctx);

   if (target == GL_FRAGMENT_PROGRAM_ARB) {
      struct st_fragment_program *stfp = (struct st_fragment_program *) prog;

      stfp->serialNo++;

      if (stfp->driver_shader) {
         cso_delete_fragment_shader(st->cso_context, stfp->driver_shader);
         stfp->driver_shader = NULL;
      }

      if (stfp->tgsi.tokens) {
         st_free_tokens(stfp->tgsi.tokens);
         stfp->tgsi.tokens = NULL;
      }

      if (st->fp == stfp)
	 st->dirty.st |= ST_NEW_FRAGMENT_PROGRAM;
   }
   else if (target == GL_VERTEX_PROGRAM_ARB) {
      struct st_vertex_program *stvp = (struct st_vertex_program *) prog;

      stvp->serialNo++;

      st_vp_release_varients( st, stvp );

      if (st->vp == stvp)
	 st->dirty.st |= ST_NEW_VERTEX_PROGRAM;
   }

   /* XXX check if program is legal, within limits */
   return GL_TRUE;
}



void st_init_program_functions(struct dd_function_table *functions)
{
   functions->BindProgram = st_bind_program;
   functions->UseProgram = st_use_program;
   functions->NewProgram = st_new_program;
   functions->DeleteProgram = st_delete_program;
   functions->IsProgramNative = st_is_program_native;
   functions->ProgramStringNotify = st_program_string_notify;
}
