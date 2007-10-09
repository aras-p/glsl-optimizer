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


#include "main/imports.h"
#include "main/mtypes.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/draw/draw_context.h"
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
st_translate_vertex_program(struct st_context *st,
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

   return cso;
}



/**
 * Translate a Mesa fragment shader into a TGSI shader.
 * \param inputMapping  to map fragment program input registers to TGSI
 *                      input slots
 * \param tokensOut  destination for TGSI tokens
 * \return  pointer to cached pipe_shader object.
 */
const struct cso_fragment_shader *
st_translate_fragment_program(struct st_context *st,
                              struct st_fragment_program *stfp,
                              const GLuint inputMapping[],
                              struct tgsi_token *tokensOut,
                              GLuint maxTokens)
{
   GLuint outputMapping[FRAG_RESULT_MAX];
   GLuint defaultInputMapping[FRAG_ATTRIB_MAX];
   struct pipe_shader_state fs;
   const struct cso_fragment_shader *cso;
   GLuint interpMode[16];  /* XXX size? */
   GLuint attr;
   GLbitfield inputsRead = stfp->Base.Base.InputsRead;

   /* For software rendering, we always need the fragment input position
    * in order to calculate interpolated values.
    * For i915, we always want to emit the semantic info for position.
    */
   inputsRead |= FRAG_BIT_WPOS;

   memset(&fs, 0, sizeof(fs));

   /*
    * Convert Mesa program inputs to TGSI input register semantics.
    */
   for (attr = 0; attr < FRAG_ATTRIB_MAX; attr++) {
      if (inputsRead & (1 << attr)) {
         const GLuint slot = fs.num_inputs;

         fs.num_inputs++;

         defaultInputMapping[attr] = slot;

         switch (attr) {
         case FRAG_ATTRIB_WPOS:
            fs.input_semantic_name[slot] = TGSI_SEMANTIC_POSITION;
            fs.input_semantic_index[slot] = 0;
            interpMode[slot] = TGSI_INTERPOLATE_CONSTANT;
            break;
         case FRAG_ATTRIB_COL0:
            fs.input_semantic_name[slot] = TGSI_SEMANTIC_COLOR;
            fs.input_semantic_index[slot] = 0;
            interpMode[slot] = TGSI_INTERPOLATE_LINEAR;
            break;
         case FRAG_ATTRIB_COL1:
            fs.input_semantic_name[slot] = TGSI_SEMANTIC_COLOR;
            fs.input_semantic_index[slot] = 1;
            interpMode[slot] = TGSI_INTERPOLATE_LINEAR;
            break;
         case FRAG_ATTRIB_FOGC:
            fs.input_semantic_name[slot] = TGSI_SEMANTIC_FOG;
            fs.input_semantic_index[slot] = 0;
            interpMode[slot] = TGSI_INTERPOLATE_PERSPECTIVE;
            break;
         case FRAG_ATTRIB_TEX0:
         case FRAG_ATTRIB_TEX1:
         case FRAG_ATTRIB_TEX2:
         case FRAG_ATTRIB_TEX3:
         case FRAG_ATTRIB_TEX4:
         case FRAG_ATTRIB_TEX5:
         case FRAG_ATTRIB_TEX6:
         case FRAG_ATTRIB_TEX7:
            fs.input_semantic_name[slot] = TGSI_SEMANTIC_GENERIC;
            fs.input_semantic_index[slot] = attr - FRAG_ATTRIB_TEX0;
            interpMode[slot] = TGSI_INTERPOLATE_PERSPECTIVE;
            break;
         case FRAG_ATTRIB_VAR0:
            /* fall-through */
         default:
            fs.input_semantic_name[slot] = TGSI_SEMANTIC_GENERIC;
            fs.input_semantic_index[slot] = attr - FRAG_ATTRIB_VAR0;
            interpMode[slot] = TGSI_INTERPOLATE_PERSPECTIVE;
         }
      }
   }

   /*
    * Semantics and mapping for outputs
    */
   {
      uint numColors = 0;
      GLbitfield outputsWritten = stfp->Base.Base.OutputsWritten;

      /* if z is written, emit that first */
      if (outputsWritten & (1 << FRAG_RESULT_DEPR)) {
         fs.output_semantic_name[fs.num_outputs] = TGSI_SEMANTIC_POSITION;
         fs.output_semantic_index[fs.num_outputs] = 0;
         outputMapping[FRAG_RESULT_DEPR] = fs.num_outputs;
         fs.num_outputs++;
         outputsWritten &= ~(1 << FRAG_RESULT_DEPR);
      }

      /* handle remaning outputs (color) */
      for (attr = 0; attr < FRAG_RESULT_MAX; attr++) {
         if (outputsWritten & (1 << attr)) {
            switch (attr) {
            case FRAG_RESULT_DEPR:
               /* handled above */
               assert(0);
               break;
            case FRAG_RESULT_COLR:
               fs.output_semantic_name[fs.num_outputs] = TGSI_SEMANTIC_COLOR;
               fs.output_semantic_index[fs.num_outputs] = numColors;
               outputMapping[attr] = fs.num_outputs;
               numColors++;
               break;
            default:
               assert(0);
            }
            fs.num_outputs++;
         }
      }
   }

   if (!inputMapping)
      inputMapping = defaultInputMapping;

   /* XXX: fix static allocation of tokens:
    */
   tgsi_mesa_compile_fp_program( &stfp->Base,
                                 /* inputs */
                                 fs.num_inputs,
                                 inputMapping,
                                 fs.input_semantic_name,
                                 fs.input_semantic_index,
                                 interpMode,
                                 /* outputs */
                                 fs.num_outputs,
                                 outputMapping,
                                 fs.output_semantic_name,
                                 fs.output_semantic_index,
                                 /* tokenized result */
                                 tokensOut, maxTokens);


   fs.tokens = tokensOut;

   cso = st_cached_fs_state(st, &fs);
   stfp->fs = cso;

   if (TGSI_DEBUG)
      tgsi_dump( tokensOut, 0/*TGSI_DUMP_VERBOSE*/ );

   return cso;
}

