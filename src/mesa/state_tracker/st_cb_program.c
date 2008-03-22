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
#include "shader/programopt.h"
#include "shader/shader_api.h"

#include "cso_cache/cso_cache.h"
#include "draw/draw_context.h"

#include "st_context.h"
#include "st_program.h"
#include "st_atom_shader.h"


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



static struct gl_program *st_new_program( GLcontext *ctx,
					  GLenum target, 
					  GLuint id )
{
   switch (target) {
   case GL_VERTEX_PROGRAM_ARB: {
      struct st_vertex_program *prog = CALLOC_STRUCT(st_vertex_program);

      prog->serialNo = SerialNo++;

      return _mesa_init_vertex_program( ctx, 
					&prog->Base,
					target, 
					id );
   }

   case GL_FRAGMENT_PROGRAM_ARB:
   case GL_FRAGMENT_PROGRAM_NV: {
      struct st_fragment_program *prog = CALLOC_STRUCT(st_fragment_program);

      prog->serialNo = SerialNo++;

      return _mesa_init_fragment_program( ctx, 
					  &prog->Base,
					  target, 
					  id );
   }

   default:
      return _mesa_new_program(ctx, target, id);
   }
}


static void st_delete_program( GLcontext *ctx,
			       struct gl_program *prog )
{
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;

   switch( prog->Target ) {
   case GL_VERTEX_PROGRAM_ARB:
      {
         struct st_vertex_program *stvp = (struct st_vertex_program *) prog;
         st_remove_vertex_program(st, stvp);
         if (stvp->driver_shader) {
            pipe->delete_vs_state(pipe, stvp->driver_shader);
            stvp->driver_shader = NULL;
         }
      }
      break;
   case GL_FRAGMENT_PROGRAM_ARB:
      {
         struct st_fragment_program *stfp
            = (struct st_fragment_program *) prog;
         st_remove_fragment_program(st, stfp);
         if (stfp->driver_shader) {
            pipe->delete_fs_state(pipe, stfp->driver_shader);
            stfp->driver_shader = NULL;
         }

         assert(!stfp->vertex_programs);

      }
      break;
   default:
      assert(0); /* problem */
   }

   _mesa_delete_program( ctx, prog );
}


static GLboolean st_is_program_native( GLcontext *ctx,
				       GLenum target, 
				       struct gl_program *prog )
{
   return GL_TRUE;
}


static void st_program_string_notify( GLcontext *ctx,
				      GLenum target,
				      struct gl_program *prog )
{
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;

   if (target == GL_FRAGMENT_PROGRAM_ARB) {
      struct st_fragment_program *stfp = (struct st_fragment_program *) prog;

      stfp->serialNo++;

      if (stfp->driver_shader) {
         pipe->delete_fs_state(pipe, stfp->driver_shader);
         stfp->driver_shader = NULL;
      }

      stfp->param_state = stfp->Base.Base.Parameters->StateFlags;

      if (stfp->state.tokens) {
         FREE((void *) stfp->state.tokens);
         stfp->state.tokens = NULL;
      }

      if (st->fp == stfp)
	 st->dirty.st |= ST_NEW_FRAGMENT_PROGRAM;
   }
   else if (target == GL_VERTEX_PROGRAM_ARB) {
      struct st_vertex_program *stvp = (struct st_vertex_program *) prog;

      stvp->serialNo++;

      if (stvp->driver_shader) {
         pipe->delete_vs_state(pipe, stvp->driver_shader);
         stvp->driver_shader = NULL;
      }

      if (stvp->draw_shader) {
         draw_delete_vertex_shader(st->draw, stvp->draw_shader);
         stvp->draw_shader = NULL;
      }

      stvp->param_state = stvp->Base.Base.Parameters->StateFlags;

      if (stvp->state.tokens) {
         FREE((void *) stvp->state.tokens);
         stvp->state.tokens = NULL;
      }

      if (st->vp == stvp)
	 st->dirty.st |= ST_NEW_VERTEX_PROGRAM;
   }
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
