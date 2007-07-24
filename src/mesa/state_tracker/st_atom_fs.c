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
                   
#include "shader/prog_parameter.h"
#include "st_context.h"
#include "pipe/p_context.h"
#include "st_atom.h"
#include "st_program.h"
#include "pipe/tgsi/mesa/mesa_to_tgsi.h"
#include "pipe/tgsi/core/tgsi_dump.h"

static void compile_fs( struct st_context *st,
			struct st_fragment_program *fs )
{
   /* XXX: fix static allocation of tokens:
    */
   tgsi_mesa_compile_fp_program( &fs->Base, fs->tokens, ST_FP_MAX_TOKENS );

   tgsi_dump( fs->tokens, TGSI_DUMP_VERBOSE );
}


static void update_fs( struct st_context *st )
{
   struct pipe_fs_state fs;
   struct st_fragment_program *fp = NULL;
   struct gl_program_parameter_list *params = NULL;

   if (st->ctx->Shader.CurrentProgram &&
       st->ctx->Shader.CurrentProgram->LinkStatus &&
       st->ctx->Shader.CurrentProgram->FragmentProgram) {
      struct gl_fragment_program *f
         = st->ctx->Shader.CurrentProgram->FragmentProgram;
      fp = st_fragment_program(f);
      params = f->Base.Parameters;
   }
   else if (st->ctx->FragmentProgram._Current) {
      fp = st_fragment_program(st->ctx->FragmentProgram._Current);
      params = st->ctx->FragmentProgram._Current->Base.Parameters;
   }

   if (fp && params) {
      /* load program's constants array */
      fp->constants.nr_constants = params->NumParameters;
      memcpy(fp->constants.constant, 
             params->ParameterValues,
             params->NumParameters * sizeof(GLfloat) * 4);
   }

   if (fp->dirty)
      compile_fs( st, fp );

   memset( &fs, 0, sizeof(fs) );
   fs.inputs_read = fp->Base.Base.InputsRead;
   fs.tokens = &fp->tokens[0];
   fs.constants = &fp->constants;
   
   if (memcmp(&fs, &st->state.fs, sizeof(fs)) != 0 ||
       fp->dirty) 
   {
      fp->dirty = 0;
      st->state.fs = fs;
      st->pipe->set_fs_state(st->pipe, &fs);
   }
}


const struct st_tracked_state st_update_fs = {
   .dirty = {
      .mesa  = _NEW_PROGRAM,
      .st   = ST_NEW_FRAGMENT_PROGRAM,
   },
   .update = update_fs
};
