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

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"
#include "pipe/tgsi/mesa/mesa_to_tgsi.h"
#include "pipe/tgsi/exec/tgsi_dump.h"

#include "st_context.h"
#include "st_atom.h"
#include "st_program.h"

#define TGSI_DEBUG 0

static void compile_fs( struct st_context *st )
{
   struct st_fragment_program *fp = st->fp;

   /* XXX: fix static allocation of tokens:
    */
   tgsi_mesa_compile_fp_program( &fp->Base, fp->tokens, ST_FP_MAX_TOKENS );

   fp->fs.inputs_read
      = tgsi_mesa_translate_vertex_input_mask(fp->Base.Base.InputsRead);
   fp->fs.outputs_written
      = tgsi_mesa_translate_vertex_output_mask(fp->Base.Base.OutputsWritten);
   fp->fs.tokens = &fp->tokens[0];

   if (TGSI_DEBUG)
      tgsi_dump( fp->tokens, TGSI_DUMP_VERBOSE );

   fp->dirty = 0;
}



static void update_fs( struct st_context *st )
{
   struct st_fragment_program *fp = NULL;

   /* find active shader and params.  Changes to this Mesa state
    * should be covered by ST_NEW_FRAGMENT_PROGRAM, thanks to the
    * logic in st_cb_program.c
    */
   if (st->ctx->Shader.CurrentProgram &&
       st->ctx->Shader.CurrentProgram->LinkStatus &&
       st->ctx->Shader.CurrentProgram->FragmentProgram) {
      struct gl_fragment_program *f
         = st->ctx->Shader.CurrentProgram->FragmentProgram;
      fp = st_fragment_program(f);
   }
   else {
      assert(st->ctx->FragmentProgram._Current);
      fp = st_fragment_program(st->ctx->FragmentProgram._Current);
   }

   /* translate shader to TGSI format */
   if (st->fp != fp || fp->dirty) {
      st->fp = fp;

      if (fp->dirty)
	 compile_fs( st );

      st->state.fs = fp->fs;
      st->pipe->set_fs_state(st->pipe, &st->state.fs);
   }
}


const struct st_tracked_state st_update_fs = {
   .name = "st_update_fs",
   .dirty = {
      .mesa  = 0,
      .st   = ST_NEW_FRAGMENT_PROGRAM,
   },
   .update = update_fs
};
