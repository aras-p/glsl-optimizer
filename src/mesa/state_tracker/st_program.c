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
#include "shader/prog_print.h"
#include "shader/programopt.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_shader_tokens.h"
#include "draw/draw_context.h"
#include "tgsi/tgsi_dump.h"

#include "st_debug.h"
#include "st_context.h"
#include "st_atom.h"
#include "st_program.h"
#include "st_mesa_to_tgsi.h"
#include "cso_cache/cso_context.h"


/**
 * Translate a Mesa vertex shader into a TGSI shader.
 * \param outputMapping  to map vertex program output registers (VERT_RESULT_x)
 *       to TGSI output slots
 * \param tokensOut  destination for TGSI tokens
 * \return  pointer to cached pipe_shader object.
 */
void
st_translate_vertex_program(struct st_context *st,
                            struct st_vertex_program *stvp,
                            const GLuint outputMapping[],
                            const ubyte *outputSemanticName,
                            const ubyte *outputSemanticIndex)
{
   struct pipe_context *pipe = st->pipe;
   GLuint defaultOutputMapping[VERT_RESULT_MAX];
   GLuint attr, i;
   GLuint num_generic = 0;

   ubyte vs_input_semantic_name[PIPE_MAX_SHADER_INPUTS];
   ubyte vs_input_semantic_index[PIPE_MAX_SHADER_INPUTS];
   uint vs_num_inputs = 0;

   ubyte vs_output_semantic_name[PIPE_MAX_SHADER_OUTPUTS];
   ubyte vs_output_semantic_index[PIPE_MAX_SHADER_OUTPUTS];
   uint vs_num_outputs = 0;

   GLbitfield input_flags[MAX_PROGRAM_INPUTS];
   GLbitfield output_flags[MAX_PROGRAM_OUTPUTS];

   /*memset(&vs, 0, sizeof(vs));*/
   memset(input_flags, 0, sizeof(input_flags));
   memset(output_flags, 0, sizeof(output_flags));

   if (stvp->Base.IsPositionInvariant)
      _mesa_insert_mvp_code(st->ctx, &stvp->Base);

   assert(stvp->Base.Base.NumInstructions > 1);

   /*
    * Determine number of inputs, the mappings between VERT_ATTRIB_x
    * and TGSI generic input indexes, plus input attrib semantic info.
    */
   for (attr = 0; attr < VERT_ATTRIB_MAX; attr++) {
      if (stvp->Base.Base.InputsRead & (1 << attr)) {
         const GLuint slot = vs_num_inputs;

         vs_num_inputs++;

         stvp->input_to_index[attr] = slot;
         stvp->index_to_input[slot] = attr;

         switch (attr) {
         case VERT_ATTRIB_POS:
            vs_input_semantic_name[slot] = TGSI_SEMANTIC_POSITION;
            vs_input_semantic_index[slot] = 0;
            break;
         case VERT_ATTRIB_WEIGHT:
            /* fall-through */
         case VERT_ATTRIB_NORMAL:
            /* just label as a generic */
            vs_input_semantic_name[slot] = TGSI_SEMANTIC_GENERIC;
            vs_input_semantic_index[slot] = 0;
            break;
         case VERT_ATTRIB_COLOR0:
            vs_input_semantic_name[slot] = TGSI_SEMANTIC_COLOR;
            vs_input_semantic_index[slot] = 0;
            break;
         case VERT_ATTRIB_COLOR1:
            vs_input_semantic_name[slot] = TGSI_SEMANTIC_COLOR;
            vs_input_semantic_index[slot] = 1;
            break;
         case VERT_ATTRIB_FOG:
            vs_input_semantic_name[slot] = TGSI_SEMANTIC_FOG;
            vs_input_semantic_index[slot] = 0;
            break;
         case VERT_ATTRIB_POINT_SIZE:
            vs_input_semantic_name[slot] = TGSI_SEMANTIC_PSIZE;
            vs_input_semantic_index[slot] = 0;
            break;
         case VERT_ATTRIB_TEX0:
         case VERT_ATTRIB_TEX1:
         case VERT_ATTRIB_TEX2:
         case VERT_ATTRIB_TEX3:
         case VERT_ATTRIB_TEX4:
         case VERT_ATTRIB_TEX5:
         case VERT_ATTRIB_TEX6:
         case VERT_ATTRIB_TEX7:
            assert(slot < Elements(vs_input_semantic_name));
            vs_input_semantic_name[slot] = TGSI_SEMANTIC_GENERIC;
            vs_input_semantic_index[slot] = num_generic++;
            break;
         case VERT_ATTRIB_GENERIC0:
         case VERT_ATTRIB_GENERIC1:
         case VERT_ATTRIB_GENERIC2:
         case VERT_ATTRIB_GENERIC3:
         case VERT_ATTRIB_GENERIC4:
         case VERT_ATTRIB_GENERIC5:
         case VERT_ATTRIB_GENERIC6:
         case VERT_ATTRIB_GENERIC7:
         case VERT_ATTRIB_GENERIC8:
         case VERT_ATTRIB_GENERIC9:
         case VERT_ATTRIB_GENERIC10:
         case VERT_ATTRIB_GENERIC11:
         case VERT_ATTRIB_GENERIC12:
         case VERT_ATTRIB_GENERIC13:
         case VERT_ATTRIB_GENERIC14:
         case VERT_ATTRIB_GENERIC15:
            assert(attr < VERT_ATTRIB_MAX);
            assert(slot < Elements(vs_input_semantic_name));
            vs_input_semantic_name[slot] = TGSI_SEMANTIC_GENERIC;
            vs_input_semantic_index[slot] = num_generic++;
            break;
         default:
            assert(0);
         }

         input_flags[slot] = stvp->Base.Base.InputFlags[attr];
      }
   }

#if 0
   if (outputMapping && outputSemanticName) {
      printf("VERT_RESULT  written  out_slot  semantic_name  semantic_index\n");
      for (attr = 0; attr < VERT_RESULT_MAX; attr++) {
         printf("    %-2d          %c       %3d          %2d              %2d\n",
                attr, 
                ((stvp->Base.Base.OutputsWritten & (1 << attr)) ? 'Y' : ' '),
                outputMapping[attr],
                outputSemanticName[attr],
                outputSemanticIndex[attr]);
      }
   }
#endif

   /* initialize output semantics to defaults */
   for (i = 0; i < PIPE_MAX_SHADER_OUTPUTS; i++) {
      assert(i < Elements(vs_output_semantic_name));
      vs_output_semantic_name[i] = TGSI_SEMANTIC_GENERIC;
      vs_output_semantic_index[i] = 0;
      output_flags[i] = 0x0;
   }

   num_generic = 0;
   /*
    * Determine number of outputs, the (default) output register
    * mapping and the semantic information for each output.
    */
   for (attr = 0; attr < VERT_RESULT_MAX; attr++) {
      if (stvp->Base.Base.OutputsWritten & (1 << attr)) {
         GLuint slot;

         /* XXX
          * Pass in the fragment program's input's semantic info.
          * Use the generic semantic indexes from there, instead of
          * guessing below.
          */

         if (outputMapping) {
            slot = outputMapping[attr];
            assert(slot != ~0);
         }
         else {
            slot = vs_num_outputs;
            vs_num_outputs++;
            defaultOutputMapping[attr] = slot;
         }

         switch (attr) {
         case VERT_RESULT_HPOS:
            assert(slot == 0);
            vs_output_semantic_name[slot] = TGSI_SEMANTIC_POSITION;
            vs_output_semantic_index[slot] = 0;
            break;
         case VERT_RESULT_COL0:
            vs_output_semantic_name[slot] = TGSI_SEMANTIC_COLOR;
            vs_output_semantic_index[slot] = 0;
            break;
         case VERT_RESULT_COL1:
            vs_output_semantic_name[slot] = TGSI_SEMANTIC_COLOR;
            vs_output_semantic_index[slot] = 1;
            break;
         case VERT_RESULT_BFC0:
            vs_output_semantic_name[slot] = TGSI_SEMANTIC_BCOLOR;
            vs_output_semantic_index[slot] = 0;
            break;
         case VERT_RESULT_BFC1:
            vs_output_semantic_name[slot] = TGSI_SEMANTIC_BCOLOR;
            vs_output_semantic_index[slot] = 1;
            break;
         case VERT_RESULT_FOGC:
            vs_output_semantic_name[slot] = TGSI_SEMANTIC_FOG;
            vs_output_semantic_index[slot] = 0;
            break;
         case VERT_RESULT_PSIZ:
            vs_output_semantic_name[slot] = TGSI_SEMANTIC_PSIZE;
            vs_output_semantic_index[slot] = 0;
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
            /* fall-through */
         case VERT_RESULT_VAR0:
            /* fall-through */
         default:
            assert(slot < Elements(vs_output_semantic_name));
            if (outputSemanticName) {
               /* use provided semantic into */
               assert(outputSemanticName[attr] != TGSI_SEMANTIC_COUNT);
               vs_output_semantic_name[slot] = outputSemanticName[attr];
               vs_output_semantic_index[slot] = outputSemanticIndex[attr];
            }
            else {
               /* use default semantic info */
               vs_output_semantic_name[slot] = TGSI_SEMANTIC_GENERIC;
               vs_output_semantic_index[slot] = num_generic++;
            }
         }

         assert(slot < Elements(output_flags));
         output_flags[slot] = stvp->Base.Base.OutputFlags[attr];
      }
   }

   if (outputMapping) {
      /* find max output slot referenced to compute vs_num_outputs */
      GLuint maxSlot = 0;
      for (attr = 0; attr < VERT_RESULT_MAX; attr++) {
         if (outputMapping[attr] != ~0 && outputMapping[attr] > maxSlot)
            maxSlot = outputMapping[attr];
      }
      vs_num_outputs = maxSlot + 1;
   }
   else {
      outputMapping = defaultOutputMapping;
   }

#if 0 /* debug */
   {
      GLuint i;
      printf("outputMapping? %d\n", outputMapping ? 1 : 0);
      if (outputMapping) {
         printf("attr -> slot\n");
         for (i = 0; i < 16;  i++) {
            printf(" %2d       %3d\n", i, outputMapping[i]);
         }
      }
      printf("slot    sem_name  sem_index\n");
      for (i = 0; i < vs_num_outputs; i++) {
         printf(" %2d         %d         %d\n",
                i,
                vs_output_semantic_name[i],
                vs_output_semantic_index[i]);
      }
   }
#endif

   /* free old shader state, if any */
   if (stvp->state.tokens) {
      st_free_tokens(stvp->state.tokens);
      stvp->state.tokens = NULL;
   }
   if (stvp->driver_shader) {
      cso_delete_vertex_shader(st->cso_context, stvp->driver_shader);
      stvp->driver_shader = NULL;
   }

   stvp->state.tokens = 
      st_translate_mesa_program(st->ctx,
                                TGSI_PROCESSOR_VERTEX,
                                &stvp->Base.Base,
                                /* inputs */
                                vs_num_inputs,
                                stvp->input_to_index,
                                vs_input_semantic_name,
                                vs_input_semantic_index,
                                NULL,
                                input_flags,
                                /* outputs */
                                vs_num_outputs,
                                outputMapping,
                                vs_output_semantic_name,
                                vs_output_semantic_index,
                                output_flags );

   stvp->num_inputs = vs_num_inputs;
   stvp->driver_shader = pipe->create_vs_state(pipe, &stvp->state);

   if ((ST_DEBUG & DEBUG_TGSI) && (ST_DEBUG & DEBUG_MESA)) {
      _mesa_print_program(&stvp->Base.Base);
      debug_printf("\n");
   }

   if (ST_DEBUG & DEBUG_TGSI) {
      tgsi_dump( stvp->state.tokens, 0 );
      debug_printf("\n");
   }
}



/**
 * Translate a Mesa fragment shader into a TGSI shader.
 * \param inputMapping  to map fragment program input registers to TGSI
 *                      input slots
 * \return  pointer to cached pipe_shader object.
 */
void
st_translate_fragment_program(struct st_context *st,
                              struct st_fragment_program *stfp,
                              const GLuint inputMapping[])
{
   struct pipe_context *pipe = st->pipe;
   GLuint outputMapping[FRAG_RESULT_MAX];
   GLuint defaultInputMapping[FRAG_ATTRIB_MAX];
   GLuint interpMode[16];  /* XXX size? */
   GLuint attr;
   const GLbitfield inputsRead = stfp->Base.Base.InputsRead;
   GLuint vslot = 0;
   GLuint num_generic = 0;

   uint fs_num_inputs = 0;

   ubyte fs_output_semantic_name[PIPE_MAX_SHADER_OUTPUTS];
   ubyte fs_output_semantic_index[PIPE_MAX_SHADER_OUTPUTS];
   uint fs_num_outputs = 0;

   GLbitfield input_flags[MAX_PROGRAM_INPUTS];
   GLbitfield output_flags[MAX_PROGRAM_OUTPUTS];

   /*memset(&fs, 0, sizeof(fs));*/
   memset(input_flags, 0, sizeof(input_flags));
   memset(output_flags, 0, sizeof(output_flags));

   /* which vertex output goes to the first fragment input: */
   if (inputsRead & FRAG_BIT_WPOS)
      vslot = 0;
   else
      vslot = 1;

   /*
    * Convert Mesa program inputs to TGSI input register semantics.
    */
   for (attr = 0; attr < FRAG_ATTRIB_MAX; attr++) {
      if (inputsRead & (1 << attr)) {
         const GLuint slot = fs_num_inputs;

         defaultInputMapping[attr] = slot;

         stfp->input_map[slot] = vslot++;

         fs_num_inputs++;

         switch (attr) {
         case FRAG_ATTRIB_WPOS:
            stfp->input_semantic_name[slot] = TGSI_SEMANTIC_POSITION;
            stfp->input_semantic_index[slot] = 0;
            interpMode[slot] = TGSI_INTERPOLATE_LINEAR;
            break;
         case FRAG_ATTRIB_COL0:
            stfp->input_semantic_name[slot] = TGSI_SEMANTIC_COLOR;
            stfp->input_semantic_index[slot] = 0;
            interpMode[slot] = TGSI_INTERPOLATE_LINEAR;
            break;
         case FRAG_ATTRIB_COL1:
            stfp->input_semantic_name[slot] = TGSI_SEMANTIC_COLOR;
            stfp->input_semantic_index[slot] = 1;
            interpMode[slot] = TGSI_INTERPOLATE_LINEAR;
            break;
         case FRAG_ATTRIB_FOGC:
            stfp->input_semantic_name[slot] = TGSI_SEMANTIC_FOG;
            stfp->input_semantic_index[slot] = 0;
            interpMode[slot] = TGSI_INTERPOLATE_PERSPECTIVE;
            break;
         case FRAG_ATTRIB_FACE:
            stfp->input_semantic_name[slot] = TGSI_SEMANTIC_FACE;
            stfp->input_semantic_index[slot] = num_generic++;
            interpMode[slot] = TGSI_INTERPOLATE_CONSTANT;
            break;
         case FRAG_ATTRIB_PNTC:
            stfp->input_semantic_name[slot] = TGSI_SEMANTIC_GENERIC;
            stfp->input_semantic_index[slot] = num_generic++;
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
            stfp->input_semantic_name[slot] = TGSI_SEMANTIC_GENERIC;
            stfp->input_semantic_index[slot] = num_generic++;
            interpMode[slot] = TGSI_INTERPOLATE_PERSPECTIVE;
            break;
         case FRAG_ATTRIB_VAR0:
            /* fall-through */
         default:
            stfp->input_semantic_name[slot] = TGSI_SEMANTIC_GENERIC;
            stfp->input_semantic_index[slot] = num_generic++;
            interpMode[slot] = TGSI_INTERPOLATE_PERSPECTIVE;
         }

         input_flags[slot] = stfp->Base.Base.InputFlags[attr];
      }
   }

   /*
    * Semantics and mapping for outputs
    */
   {
      uint numColors = 0;
      GLbitfield64 outputsWritten = stfp->Base.Base.OutputsWritten;

      /* if z is written, emit that first */
      if (outputsWritten & (1 << FRAG_RESULT_DEPTH)) {
         fs_output_semantic_name[fs_num_outputs] = TGSI_SEMANTIC_POSITION;
         fs_output_semantic_index[fs_num_outputs] = 0;
         outputMapping[FRAG_RESULT_DEPTH] = fs_num_outputs;
         fs_num_outputs++;
         outputsWritten &= ~(1 << FRAG_RESULT_DEPTH);
      }

      /* handle remaning outputs (color) */
      for (attr = 0; attr < FRAG_RESULT_MAX; attr++) {
         if (outputsWritten & (1 << attr)) {
            switch (attr) {
            case FRAG_RESULT_DEPTH:
               /* handled above */
               assert(0);
               break;
            default:
               assert(attr == FRAG_RESULT_COLOR ||
                      (FRAG_RESULT_DATA0 <= attr && attr < FRAG_RESULT_MAX));
               fs_output_semantic_name[fs_num_outputs] = TGSI_SEMANTIC_COLOR;
               fs_output_semantic_index[fs_num_outputs] = numColors;
               outputMapping[attr] = fs_num_outputs;
               numColors++;
               break;
            }

            output_flags[fs_num_outputs] = stfp->Base.Base.OutputFlags[attr];

            fs_num_outputs++;
         }
      }
   }

   if (!inputMapping)
      inputMapping = defaultInputMapping;

   stfp->state.tokens = 
      st_translate_mesa_program(st->ctx,
                                TGSI_PROCESSOR_FRAGMENT,
                                &stfp->Base.Base,
                                /* inputs */
                                fs_num_inputs,
                                inputMapping,
                                stfp->input_semantic_name,
                                stfp->input_semantic_index,
                                interpMode,
                                input_flags,
                                /* outputs */
                                fs_num_outputs,
                                outputMapping,
                                fs_output_semantic_name,
                                fs_output_semantic_index,
                                output_flags );

   stfp->driver_shader = pipe->create_fs_state(pipe, &stfp->state);

   if ((ST_DEBUG & DEBUG_TGSI) && (ST_DEBUG & DEBUG_MESA)) {
      _mesa_print_program(&stfp->Base.Base);
      debug_printf("\n");
   }

   if (ST_DEBUG & DEBUG_TGSI) {
      tgsi_dump( stfp->state.tokens, 0/*TGSI_DUMP_VERBOSE*/ );
      debug_printf("\n");
   }
}


/**
 * Debug- print current shader text
 */
void
st_print_shaders(GLcontext *ctx)
{
   struct gl_shader_program *shProg = ctx->Shader.CurrentProgram;
   if (shProg) {
      GLuint i;
      for (i = 0; i < shProg->NumShaders; i++) {
         printf("GLSL shader %u of %u:\n", i, shProg->NumShaders);
         printf("%s\n", shProg->Shaders[i]->Source);
      }
   }
}
