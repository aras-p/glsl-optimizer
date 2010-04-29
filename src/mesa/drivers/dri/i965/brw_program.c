/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
  
#include "main/imports.h"
#include "main/enums.h"
#include "shader/prog_parameter.h"
#include "shader/program.h"
#include "shader/programopt.h"
#include "shader/shader_api.h"
#include "tnl/tnl.h"

#include "brw_context.h"
#include "brw_wm.h"

static void brwBindProgram( GLcontext *ctx,
			    GLenum target, 
			    struct gl_program *prog )
{
   struct brw_context *brw = brw_context(ctx);

   switch (target) {
   case GL_VERTEX_PROGRAM_ARB: 
      brw->state.dirty.brw |= BRW_NEW_VERTEX_PROGRAM;
      break;
   case GL_FRAGMENT_PROGRAM_ARB:
      brw->state.dirty.brw |= BRW_NEW_FRAGMENT_PROGRAM;
      break;
   }
}

static struct gl_program *brwNewProgram( GLcontext *ctx,
				      GLenum target, 
				      GLuint id )
{
   struct brw_context *brw = brw_context(ctx);

   switch (target) {
   case GL_VERTEX_PROGRAM_ARB: {
      struct brw_vertex_program *prog = CALLOC_STRUCT(brw_vertex_program);
      if (prog) {
	 prog->id = brw->program_id++;

	 return _mesa_init_vertex_program( ctx, &prog->program,
					     target, id );
      }
      else
	 return NULL;
   }

   case GL_FRAGMENT_PROGRAM_ARB: {
      struct brw_fragment_program *prog = CALLOC_STRUCT(brw_fragment_program);
      if (prog) {
	 prog->id = brw->program_id++;

	 return _mesa_init_fragment_program( ctx, &prog->program,
					     target, id );
      }
      else
	 return NULL;
   }

   default:
      return _mesa_new_program(ctx, target, id);
   }
}

static void brwDeleteProgram( GLcontext *ctx,
			      struct gl_program *prog )
{
   if (prog->Target == GL_FRAGMENT_PROGRAM_ARB) {
      struct gl_fragment_program *fp = (struct gl_fragment_program *) prog;
      struct brw_fragment_program *brw_fp = brw_fragment_program(fp);

      dri_bo_unreference(brw_fp->const_buffer);
   }

   if (prog->Target == GL_VERTEX_PROGRAM_ARB) {
      struct gl_vertex_program *vp = (struct gl_vertex_program *) prog;
      struct brw_vertex_program *brw_vp = brw_vertex_program(vp);

      dri_bo_unreference(brw_vp->const_buffer);
   }

   _mesa_delete_program( ctx, prog );
}


static GLboolean brwIsProgramNative( GLcontext *ctx,
				     GLenum target, 
				     struct gl_program *prog )
{
   return GL_TRUE;
}

static void
shader_error(GLcontext *ctx, struct gl_program *prog, const char *msg)
{
   struct gl_shader_program *shader;

   shader = _mesa_lookup_shader_program(ctx, prog->Id);

   if (shader) {
      if (shader->InfoLog) {
	 free(shader->InfoLog);
      }
      shader->InfoLog = _mesa_strdup(msg);
      shader->LinkStatus = GL_FALSE;
   }
}

static GLboolean brwProgramStringNotify( GLcontext *ctx,
                                         GLenum target,
                                         struct gl_program *prog )
{
   struct brw_context *brw = brw_context(ctx);
   int i;

   if (target == GL_FRAGMENT_PROGRAM_ARB) {
      struct gl_fragment_program *fprog = (struct gl_fragment_program *) prog;
      struct brw_fragment_program *newFP = brw_fragment_program(fprog);
      const struct brw_fragment_program *curFP =
         brw_fragment_program_const(brw->fragment_program);

      if (fprog->FogOption) {
         _mesa_append_fog_code(ctx, fprog);
         fprog->FogOption = GL_NONE;
      }

      if (newFP == curFP)
	 brw->state.dirty.brw |= BRW_NEW_FRAGMENT_PROGRAM;
      newFP->id = brw->program_id++;      
      newFP->isGLSL = brw_wm_is_glsl(fprog);
   }
   else if (target == GL_VERTEX_PROGRAM_ARB) {
      struct gl_vertex_program *vprog = (struct gl_vertex_program *) prog;
      struct brw_vertex_program *newVP = brw_vertex_program(vprog);
      const struct brw_vertex_program *curVP =
         brw_vertex_program_const(brw->vertex_program);

      if (newVP == curVP)
	 brw->state.dirty.brw |= BRW_NEW_VERTEX_PROGRAM;
      if (newVP->program.IsPositionInvariant) {
	 _mesa_insert_mvp_code(ctx, &newVP->program);
      }
      newVP->id = brw->program_id++;      

      /* Also tell tnl about it:
       */
      _tnl_program_string(ctx, target, prog);
   }

   /* Reject programs with subroutines, which are totally broken at the moment
    * (all program flows return when any program flow returns, and
    * the VS also hangs if a function call calls a function.
    *
    * See piglit glsl-{vs,fs}-functions-[23] tests.
    */
   for (i = 0; i < prog->NumInstructions; i++) {
      if (prog->Instructions[i].Opcode == OPCODE_CAL) {
	 shader_error(ctx, prog,
		      "i965 driver doesn't yet support uninlined function "
		      "calls.  Move to using a single return statement at "
		      "the end of the function to work around it.");
	 return GL_FALSE;
      }
   }

   return GL_TRUE;
}

void brwInitFragProgFuncs( struct dd_function_table *functions )
{
   assert(functions->ProgramStringNotify == _tnl_program_string); 

   functions->BindProgram = brwBindProgram;
   functions->NewProgram = brwNewProgram;
   functions->DeleteProgram = brwDeleteProgram;
   functions->IsProgramNative = brwIsProgramNative;
   functions->ProgramStringNotify = brwProgramStringNotify;
}

