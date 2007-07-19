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

#include "st_context.h"
#include "st_program.h"    

#include "glheader.h"
#include "macros.h"
#include "enums.h"
#include "prog_instruction.h"
#include "prog_parameter.h"
#include "program.h"
#include "programopt.h"
#include "tnl/tnl.h"
#include "pipe/tgsi/mesa/tgsi_mesa.h"


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

static struct gl_program *st_new_program( GLcontext *ctx,
					  GLenum target, 
					  GLuint id )
{
   struct st_context *st = st_context(ctx);

   switch (target) {
   case GL_VERTEX_PROGRAM_ARB: {
      struct st_vertex_program *prog = CALLOC_STRUCT(st_vertex_program);

      prog->id = st->program_id++;
      prog->dirty = 1;

      return _mesa_init_vertex_program( ctx, 
					&prog->Base,
					target, 
					id );
   }

   case GL_FRAGMENT_PROGRAM_ARB: {
      struct st_fragment_program *prog = CALLOC_STRUCT(st_fragment_program);

      prog->id = st->program_id++;
      prog->dirty = 1;

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
      struct st_fragment_program *p = (struct st_fragment_program *)prog;

      if (prog == &ctx->FragmentProgram._Current->Base)
	 st->dirty.st |= ST_NEW_FRAGMENT_PROGRAM;

      p->id = st->program_id++;      
      p->param_state = p->Base.Base.Parameters->StateFlags;
   }
   else if (target == GL_VERTEX_PROGRAM_ARB) {
      struct st_vertex_program *p = (struct st_vertex_program *)prog;

      if (prog == &ctx->VertexProgram._Current->Base)
	 st->dirty.st |= ST_NEW_VERTEX_PROGRAM;

      p->id = st->program_id++;      
      p->param_state = p->Base.Base.Parameters->StateFlags;

      /* Also tell tnl about it:
       */
      _tnl_program_string(ctx, target, prog);
   }
}



void st_init_cb_program( struct st_context *st )
{
   struct dd_function_table *functions = &st->ctx->Driver;

   /* Need these flags:
    */
   st->ctx->FragmentProgram._MaintainTexEnvProgram = GL_TRUE;
   st->ctx->FragmentProgram._UseTexEnvProgram = GL_TRUE;

   assert(functions->ProgramStringNotify == _tnl_program_string); 
   functions->BindProgram = st_bind_program;
   functions->NewProgram = st_new_program;
   functions->DeleteProgram = st_delete_program;
   functions->IsProgramNative = st_is_program_native;
   functions->ProgramStringNotify = st_program_string_notify;
}


void st_destroy_cb_program( struct st_context *st )
{
}

