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

#include "shader/prog_parameter.h"
#include "shader/prog_print.h"
#include "tnl/t_vp_build.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"
#include "pipe/tgsi/mesa/mesa_to_tgsi.h"
#include "pipe/tgsi/exec/tgsi_core.h"

#include "st_context.h"
#include "st_cache.h"
#include "st_atom.h"
#include "st_program.h"


#define TGSI_DEBUG 0


/**
 * Translate a Mesa vertex shader into a TGSI shader.
 * \param outputMapping  to map vertex program output registers to TGSI
 *                       output slots
 * \param tokensOut  destination for TGSI tokens
 * \return  pointer to cached pipe_shader object.
 */
const struct cso_vertex_shader *
st_translate_vertex_shader(const struct st_context *st,
                           struct st_vertex_program *stvp,
                           const GLuint outputMapping[],
                           struct tgsi_token *tokensOut,
                           GLuint maxTokens)
{
   GLuint defaultOutputMapping[VERT_RESULT_MAX];
   struct pipe_shader_state vs;
   const struct cso_vertex_shader *cso;
   GLuint attr, i;

   memset(&vs, 0, sizeof(vs));

   /*
    * Determine number of inputs, the mappings between VERT_ATTRIB_x
    * and TGSI generic input indexes, plus input attrib semantic info.
    */
   for (attr = 0; attr < VERT_ATTRIB_MAX; attr++) {
      if (stvp->Base.Base.InputsRead & (1 << attr)) {
         const GLuint slot = vs.num_inputs;

         vs.num_inputs++;

         stvp->input_to_index[attr] = slot;
         stvp->index_to_input[slot] = attr;

         switch (attr) {
         case VERT_ATTRIB_POS:
            vs.input_semantic_name[slot] = TGSI_SEMANTIC_POSITION;
            vs.input_semantic_index[slot] = 0;
            break;
         case VERT_ATTRIB_WEIGHT:
            /* fall-through */
         case VERT_ATTRIB_NORMAL:
            /* just label as a generic */
            vs.input_semantic_name[slot] = TGSI_SEMANTIC_GENERIC;
            vs.input_semantic_index[slot] = 0;
            break;
         case VERT_ATTRIB_COLOR0:
            vs.input_semantic_name[slot] = TGSI_SEMANTIC_COLOR;
            vs.input_semantic_index[slot] = 0;
            break;
         case VERT_ATTRIB_COLOR1:
            vs.input_semantic_name[slot] = TGSI_SEMANTIC_COLOR;
            vs.input_semantic_index[slot] = 1;
            break;
         case VERT_ATTRIB_FOG:
            vs.input_semantic_name[slot] = TGSI_SEMANTIC_FOG;
            vs.input_semantic_index[slot] = 0;
            break;
         case VERT_ATTRIB_TEX0:
         case VERT_ATTRIB_TEX1:
         case VERT_ATTRIB_TEX2:
         case VERT_ATTRIB_TEX3:
         case VERT_ATTRIB_TEX4:
         case VERT_ATTRIB_TEX5:
         case VERT_ATTRIB_TEX6:
         case VERT_ATTRIB_TEX7:
            vs.input_semantic_name[slot] = TGSI_SEMANTIC_GENERIC;
            vs.input_semantic_index[slot] = attr - VERT_ATTRIB_TEX0;
            break;
         case VERT_ATTRIB_GENERIC0:
         case VERT_ATTRIB_GENERIC1:
         case VERT_ATTRIB_GENERIC2:
         case VERT_ATTRIB_GENERIC3:
         case VERT_ATTRIB_GENERIC4:
         case VERT_ATTRIB_GENERIC5:
         case VERT_ATTRIB_GENERIC6:
         case VERT_ATTRIB_GENERIC7:
            assert(attr < VERT_ATTRIB_MAX);
            vs.input_semantic_name[slot] = TGSI_SEMANTIC_GENERIC;
            vs.input_semantic_index[slot] = attr - VERT_ATTRIB_GENERIC0;
            break;
         default:
            assert(0);
         }
      }
   }

   /* initialize output semantics to defaults */
   for (i = 0; i < PIPE_MAX_SHADER_OUTPUTS; i++) {
      vs.output_semantic_name[i] = TGSI_SEMANTIC_GENERIC;
      vs.output_semantic_index[i] = 0;
   }

   /*
    * Determine number of outputs, the (default) output register
    * mapping and the semantic information for each output.
    */
   for (attr = 0; attr < VERT_RESULT_MAX; attr++) {
      if (stvp->Base.Base.OutputsWritten & (1 << attr)) {
         GLuint slot;

         if (outputMapping) {
            slot = outputMapping[attr];
            assert(slot != ~0);
         }
         else {
            slot = vs.num_outputs;
            vs.num_outputs++;
            defaultOutputMapping[attr] = slot;
         }

         /*
         printf("Output %u -> slot %u\n", attr, slot);
         */

         switch (attr) {
         case VERT_RESULT_HPOS:
            vs.output_semantic_name[slot] = TGSI_SEMANTIC_POSITION;
            vs.output_semantic_index[slot] = 0;
            break;
         case VERT_RESULT_COL0:
            vs.output_semantic_name[slot] = TGSI_SEMANTIC_COLOR;
            vs.output_semantic_index[slot] = 0;
            break;
         case VERT_RESULT_COL1:
            vs.output_semantic_name[slot] = TGSI_SEMANTIC_COLOR;
            vs.output_semantic_index[slot] = 1;
            break;
         case VERT_RESULT_BFC0:
            vs.output_semantic_name[slot] = TGSI_SEMANTIC_BCOLOR;
            vs.output_semantic_index[slot] = 0;
            break;
         case VERT_RESULT_BFC1:
            vs.output_semantic_name[slot] = TGSI_SEMANTIC_BCOLOR;
            vs.output_semantic_index[slot] = 1;
            break;
         case VERT_RESULT_FOGC:
            vs.output_semantic_name[slot] = TGSI_SEMANTIC_FOG;
            vs.output_semantic_index[slot] = 0;
            break;
         case VERT_RESULT_PSIZ:
            vs.output_semantic_name[slot] = TGSI_SEMANTIC_PSIZE;
            vs.output_semantic_index[slot] = 0;
            break;
         case VERT_RESULT_EDGE:
            assert(0);
            break;
         case VERT_RESULT_TEX0:
         case VERT_RESULT_TEX1:
         case VERT_RESULT_TEX2:
         case VERT_RESULT_TEX3:
         case VERT_RESULT_TEX4:
         case VERT_RESULT_TEX5:
         case VERT_RESULT_TEX6:
         case VERT_RESULT_TEX7:
            vs.output_semantic_name[slot] = TGSI_SEMANTIC_GENERIC;
            vs.output_semantic_index[slot] = attr - VERT_RESULT_TEX0;
            break;
         case VERT_RESULT_VAR0:
            /* fall-through */
         default:
            assert(attr - VERT_RESULT_VAR0 < MAX_VARYING);
            vs.output_semantic_name[slot] = TGSI_SEMANTIC_GENERIC;
            vs.output_semantic_index[slot] = attr - VERT_RESULT_VAR0;
         }
      }
   }


   if (outputMapping) {
      /* find max output slot referenced to compute vs.num_outputs */
      GLuint maxSlot = 0;
      for (attr = 0; attr < VERT_RESULT_MAX; attr++) {
         if (outputMapping[attr] != ~0 && outputMapping[attr] > maxSlot)
            maxSlot = outputMapping[attr];
      }
      vs.num_outputs = maxSlot + 1;
   }
   else {
      outputMapping = defaultOutputMapping;
   }

   /* XXX: fix static allocation of tokens:
    */
   tgsi_mesa_compile_vp_program( &stvp->Base,
                                 /* inputs */
                                 vs.num_inputs,
                                 stvp->input_to_index,
                                 vs.input_semantic_name,
                                 vs.input_semantic_index,
                                 /* outputs */
                                 vs.num_outputs,
                                 outputMapping,
                                 vs.output_semantic_name,
                                 vs.output_semantic_index,
                                 /* tokenized result */
                                 tokensOut, maxTokens);

   vs.tokens = tokensOut;
   cso = st_cached_vs_state(st, &vs);
   stvp->vs = cso;

   if (TGSI_DEBUG)
      tgsi_dump( tokensOut, 0 );

#if defined(USE_X86_ASM) || defined(SLANG_X86)
   if (stvp->sse2_program.csr == stvp->sse2_program.store)
      tgsi_emit_sse2( tokensOut, &stvp->sse2_program );

   if (!cso->state.executable)
      ((struct cso_vertex_shader*)cso)->state.executable = (void *) x86_get_func( &stvp->sse2_program );
#endif

   stvp->dirty = 0;

   return cso;
}



static void update_vs( struct st_context *st )
{
   struct st_vertex_program *stvp;

   /* find active shader and params -- Should be covered by
    * ST_NEW_VERTEX_PROGRAM
    */
   if (st->ctx->Shader.CurrentProgram &&
       st->ctx->Shader.CurrentProgram->LinkStatus &&
       st->ctx->Shader.CurrentProgram->VertexProgram) {
      struct gl_vertex_program *f
         = st->ctx->Shader.CurrentProgram->VertexProgram;
      stvp = st_vertex_program(f);
   }
   else {
      assert(st->ctx->VertexProgram._Current);
      stvp = st_vertex_program(st->ctx->VertexProgram._Current);
   }

   if (st->vp != stvp || stvp->dirty) {
#if 0
      if (stvp->dirty)
         (void) st_translate_vertex_shader( st, stvp );

      /* Bind the vertex program and TGSI shader */
      st->vp = stvp;
      st->state.vs = stvp->vs;

#if 0
      printf("###### bind vp tokens: %p %p  num_inp=%u\n",
             stvp, stvp->tokens, stvp->vs->state.num_inputs);
      if (TGSI_DEBUG)
         tgsi_dump( stvp->tokens, 0 );
#endif
      st->pipe->bind_vs_state(st->pipe, st->state.vs->data);
#else
      /* NEW */
      st->dirty.st |= ST_NEW_LINKAGE;

#endif
   }
}

#if 0
const struct st_tracked_state st_update_vs = {
   .name = "st_update_vs",
   .dirty = {
      .mesa  = _NEW_PROGRAM, 
      .st   = ST_NEW_VERTEX_PROGRAM,
   },
   .update = update_vs
};
#endif
