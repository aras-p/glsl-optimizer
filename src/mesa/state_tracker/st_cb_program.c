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

#include "st_context.h"
#include "st_program.h"
#include "st_atom_shader.h"

#include "tnl/tnl.h"
#include "pipe/tgsi/mesa/tgsi_mesa.h"


/**
 * Called via ctx->Driver.BindProgram() to bind an ARB vertex or
 * fragment program.
 */
static void st_bind_program( GLcontext *ctx,
			     GLenum target, 
			     struct gl_program *prog )
{
   struct st_context *st = st_context(ctx);

   st->dirty.st |= ST_NEW_SHADER;
}


/**
 * Called via ctx->Driver.UseProgram() to bind a linked GLSL program
 * (vertex shader + fragment shader).
 */
static void st_use_program( GLcontext *ctx,
			    GLuint program )
{
   struct st_context *st = st_context(ctx);

   st->dirty.st |= ST_NEW_SHADER;
}



static struct gl_program *st_new_program( GLcontext *ctx,
					  GLenum target, 
					  GLuint id )
{
   struct st_context *st = st_context(ctx);

   switch (target) {
   case GL_VERTEX_PROGRAM_ARB: {
      struct st_vertex_program *prog = CALLOC_STRUCT(st_vertex_program);

      prog->serialNo = 1;

      return _mesa_init_vertex_program( ctx, 
					&prog->Base,
					target, 
					id );
   }

   case GL_FRAGMENT_PROGRAM_ARB:
   case GL_FRAGMENT_PROGRAM_NV: {
      struct st_fragment_program *prog = CALLOC_STRUCT(st_fragment_program);

      prog->serialNo = 1;

#if defined(__i386__) || defined(__386__)
      x86_init_func( &prog->sse2_program );
#endif

      return _mesa_init_fragment_program( ctx, 
					  &prog->Base,
					  target, 
					  id );
   }

   default:
      return _mesa_new_program(ctx, target, id);
   }

   st->dirty.st |= ST_NEW_SHADER;
}


static void st_delete_program( GLcontext *ctx,
			       struct gl_program *prog )
{
   struct st_context *st = st_context(ctx);

   switch( prog->Target ) {
   case GL_VERTEX_PROGRAM_ARB:
      {
         struct st_vertex_program *stvp = (struct st_vertex_program *) prog;
         st_remove_vertex_program(st, stvp);
      }
      break;
   case GL_FRAGMENT_PROGRAM_ARB:
      {
         struct st_fragment_program *stfp
            = (struct st_fragment_program *) prog;
#if defined(__i386__) || defined(__386__)
         x86_release_func( &stfp->sse2_program );
#endif
         st_remove_fragment_program(st, stfp);
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

   if (target == GL_FRAGMENT_PROGRAM_ARB) {
      struct st_fragment_program *stfp = (struct st_fragment_program *) prog;

      stfp->serialNo++;

      stfp->param_state = stfp->Base.Base.Parameters->StateFlags;
   }
   else if (target == GL_VERTEX_PROGRAM_ARB) {
      struct st_vertex_program *stvp = (struct st_vertex_program *) prog;

      stvp->serialNo++;

      stvp->param_state = stvp->Base.Base.Parameters->StateFlags;

      /* Also tell tnl about it:
       */
      _tnl_program_string(ctx, target, prog);
   }

   st->dirty.st |= ST_NEW_SHADER;
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
