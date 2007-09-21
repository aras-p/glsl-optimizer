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
  *   Brian Paul
  */

#include "shader/prog_parameter.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"
#include "pipe/tgsi/mesa/mesa_to_tgsi.h"
#include "pipe/tgsi/exec/tgsi_dump.h"

#include "st_context.h"
#include "st_cache.h"
#include "st_atom.h"
#include "st_program.h"


#define TGSI_DEBUG 1


/**
 * Translate a Mesa fragment shader into a TGSI shader.
 * \return  pointer to cached pipe_shader object.
 */
const struct cso_fragment_shader *
st_translate_fragment_shader(struct st_context *st,
                           struct st_fragment_program *stfp)
{
   GLuint outputMapping[FRAG_RESULT_MAX];
   GLuint inputMapping[PIPE_MAX_SHADER_INPUTS];
   struct pipe_shader_state fs;
   const struct cso_fragment_shader *cso;
   GLuint interpMode[16];  /* XXX size? */
   GLuint attr;
   GLbitfield inputsRead = stfp->Base.Base.InputsRead;

   /* Check if all fragment programs need the fragment position (in order
    * to do perspective-corrected interpolation).
    */
   if (st->pipe->get_param(st->pipe, PIPE_PARAM_FS_NEEDS_POS))
      inputsRead |= FRAG_BIT_WPOS;

   memset(&fs, 0, sizeof(fs));

   /*
    * Convert Mesa program inputs to TGSI input register semantics.
    */
   for (attr = 0; attr < FRAG_ATTRIB_MAX; attr++) {
      if (inputsRead & (1 << attr)) {
         inputMapping[attr] = fs.num_inputs;

         switch (attr) {
         case FRAG_ATTRIB_WPOS:
            fs.input_semantic_name[fs.num_inputs] = TGSI_SEMANTIC_POSITION;
            fs.input_semantic_index[fs.num_inputs] = 0;
            interpMode[fs.num_inputs] = TGSI_INTERPOLATE_CONSTANT;
            break;
         case FRAG_ATTRIB_COL0:
            fs.input_semantic_name[fs.num_inputs] = TGSI_SEMANTIC_COLOR;
            fs.input_semantic_index[fs.num_inputs] = 0;
            interpMode[fs.num_inputs] = TGSI_INTERPOLATE_LINEAR;
            break;
         case FRAG_ATTRIB_COL1:
            fs.input_semantic_name[fs.num_inputs] = TGSI_SEMANTIC_COLOR;
            fs.input_semantic_index[fs.num_inputs] = 1;
            interpMode[fs.num_inputs] = TGSI_INTERPOLATE_LINEAR;
            break;
         case FRAG_ATTRIB_FOGC:
            fs.input_semantic_name[fs.num_inputs] = TGSI_SEMANTIC_FOG;
            fs.input_semantic_index[fs.num_inputs] = 0;
            interpMode[fs.num_inputs] = TGSI_INTERPOLATE_PERSPECTIVE;
            break;
         case FRAG_ATTRIB_TEX0:
         case FRAG_ATTRIB_TEX1:
         case FRAG_ATTRIB_TEX2:
         case FRAG_ATTRIB_TEX3:
         case FRAG_ATTRIB_TEX4:
         case FRAG_ATTRIB_TEX5:
         case FRAG_ATTRIB_TEX6:
         case FRAG_ATTRIB_TEX7:
            fs.input_semantic_name[fs.num_inputs] = TGSI_SEMANTIC_GENERIC;
            fs.input_semantic_index[fs.num_inputs] = attr - FRAG_ATTRIB_TEX0;
            interpMode[fs.num_inputs] = TGSI_INTERPOLATE_PERSPECTIVE;
            break;
         case FRAG_ATTRIB_VAR0:
            /* fall-through */
         default:
            fs.input_semantic_name[fs.num_inputs] = TGSI_SEMANTIC_GENERIC;
            fs.input_semantic_index[fs.num_inputs] = attr - FRAG_ATTRIB_VAR0;
            interpMode[fs.num_inputs] = TGSI_INTERPOLATE_PERSPECTIVE;
         }

         fs.num_inputs++;
      }
   }

   /*
    * Semantics for outputs
    */
   for (attr = 0; attr < FRAG_RESULT_MAX; attr++) {
      if (stfp->Base.Base.OutputsWritten & (1 << attr)) {
         switch (attr) {
         case FRAG_RESULT_DEPR:
            fs.output_semantic_name[fs.num_outputs] = TGSI_SEMANTIC_POSITION;
            outputMapping[attr] = fs.num_outputs;
            break;
         case FRAG_RESULT_COLR:
            fs.output_semantic_name[fs.num_outputs] = TGSI_SEMANTIC_COLOR;
            outputMapping[attr] = fs.num_outputs;
            break;
         default:
            assert(0);
         }
         fs.num_outputs++;
      }
   }

   /* XXX: fix static allocation of tokens:
    */
   tgsi_mesa_compile_fp_program( &stfp->Base,
                                 fs.num_inputs,
                                 inputMapping,
                                 fs.input_semantic_name,
                                 fs.input_semantic_index,
                                 interpMode,
                                 outputMapping,
                                 stfp->tokens, ST_FP_MAX_TOKENS );

   fs.tokens = &stfp->tokens[0];

   cso = st_cached_fs_state(st, &fs);
   stfp->fs = cso;

   if (TGSI_DEBUG)
      tgsi_dump( stfp->tokens, 0/*TGSI_DUMP_VERBOSE*/ );

   stfp->dirty = 0;

   return cso;
}



static void update_fs( struct st_context *st )
{
   struct st_fragment_program *stfp = NULL;

   /* find active shader and params.  Changes to this Mesa state
    * should be covered by ST_NEW_FRAGMENT_PROGRAM, thanks to the
    * logic in st_cb_program.c
    */
   if (st->ctx->Shader.CurrentProgram &&
       st->ctx->Shader.CurrentProgram->LinkStatus &&
       st->ctx->Shader.CurrentProgram->FragmentProgram) {
      struct gl_fragment_program *f
         = st->ctx->Shader.CurrentProgram->FragmentProgram;
      stfp = st_fragment_program(f);
   }
   else {
      assert(st->ctx->FragmentProgram._Current);
      stfp = st_fragment_program(st->ctx->FragmentProgram._Current);
   }

   /* if new binding, or shader has changed */
   if (st->fp != stfp || stfp->dirty) {

      if (stfp->dirty)
         (void) st_translate_fragment_shader( st, stfp );

      /* Bind the vertex program and TGSI shader */
      st->fp = stfp;
      st->state.fs = stfp->fs;

      st->pipe->bind_fs_state(st->pipe, st->state.fs->data);
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
