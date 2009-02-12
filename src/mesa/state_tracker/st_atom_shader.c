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

/**
 * State validation for vertex/fragment shaders.
 * Note that we have to delay most vertex/fragment shader translation
 * until rendering time since the linkage between the vertex outputs and
 * fragment inputs can vary depending on the pairing of shaders.
 *
 * Authors:
 *   Brian Paul
 */



#include "main/imports.h"
#include "main/mtypes.h"
#include "main/macros.h"
#include "shader/program.h"

#include "pipe/p_context.h"
#include "pipe/p_shader_tokens.h"

#include "util/u_simple_shaders.h"

#include "cso_cache/cso_context.h"

#include "st_context.h"
#include "st_atom.h"
#include "st_program.h"
#include "st_atom_shader.h"
#include "st_mesa_to_tgsi.h"


/**
 * This represents a vertex program, especially translated to match
 * the inputs of a particular fragment shader.
 */
struct translated_vertex_program
{
   struct st_vertex_program *master;

   /** The fragment shader "signature" this vertex shader is meant for: */
   GLbitfield frag_inputs;

   /** Compared against master vertex program's serialNo: */
   GLuint serialNo;

   /** Maps VERT_RESULT_x to slot */
   GLuint output_to_slot[VERT_RESULT_MAX];
   ubyte output_to_semantic_name[VERT_RESULT_MAX];
   ubyte output_to_semantic_index[VERT_RESULT_MAX];

   /** Pointer to the translated vertex program */
   struct st_vertex_program *vp;

   struct translated_vertex_program *next;  /**< next in linked list */
};



/**
 * Given a vertex program output attribute, return the corresponding
 * fragment program input attribute.
 * \return -1 for vertex outputs that have no corresponding fragment input
 */
static GLint
vp_out_to_fp_in(GLuint vertResult)
{
   if (vertResult >= VERT_RESULT_TEX0 &&
       vertResult < VERT_RESULT_TEX0 + MAX_TEXTURE_COORD_UNITS)
      return FRAG_ATTRIB_TEX0 + (vertResult - VERT_RESULT_TEX0);

   if (vertResult >= VERT_RESULT_VAR0 &&
       vertResult < VERT_RESULT_VAR0 + MAX_VARYING)
      return FRAG_ATTRIB_VAR0 + (vertResult - VERT_RESULT_VAR0);

   switch (vertResult) {
   case VERT_RESULT_HPOS:
      return FRAG_ATTRIB_WPOS;
   case VERT_RESULT_COL0:
      return FRAG_ATTRIB_COL0;
   case VERT_RESULT_COL1:
      return FRAG_ATTRIB_COL1;
   case VERT_RESULT_FOGC:
      return FRAG_ATTRIB_FOGC;
   default:
      /* Back-face colors, edge flags, etc */
      return -1;
   }
}


/**
 * Find a translated vertex program that corresponds to stvp and
 * has outputs matched to stfp's inputs.
 * This performs vertex and fragment translation (to TGSI) when needed.
 */
static struct translated_vertex_program *
find_translated_vp(struct st_context *st,
                   struct st_vertex_program *stvp,
                   struct st_fragment_program *stfp)
{
   static const GLuint UNUSED = ~0;
   struct translated_vertex_program *xvp;
   const GLbitfield fragInputsRead = stfp->Base.Base.InputsRead;

   /*
    * Translate fragment program if needed.
    */
   if (!stfp->state.tokens) {
      GLuint inAttr, numIn = 0;

      for (inAttr = 0; inAttr < FRAG_ATTRIB_MAX; inAttr++) {
         if (fragInputsRead & (1 << inAttr)) {
            stfp->input_to_slot[inAttr] = numIn;
            numIn++;
         }
         else {
            stfp->input_to_slot[inAttr] = UNUSED;
         }
      }

      stfp->num_input_slots = numIn;

      assert(stfp->Base.Base.NumInstructions > 1);

      st_translate_fragment_program(st, stfp, stfp->input_to_slot);
   }


   /* See if we've got a translated vertex program whose outputs match
    * the fragment program's inputs.
    * XXX This could be a hash lookup, using InputsRead as the key.
    */
   for (xvp = stfp->vertex_programs; xvp; xvp = xvp->next) {
      if (xvp->master == stvp && xvp->frag_inputs == fragInputsRead) {
         break;
      }
   }

   /* No?  Allocate translated vp object now */
   if (!xvp) {
      xvp = CALLOC_STRUCT(translated_vertex_program);
      xvp->frag_inputs = fragInputsRead;
      xvp->master = stvp;

      xvp->next = stfp->vertex_programs;
      stfp->vertex_programs = xvp;
   }

   /* See if we need to translate vertex program to TGSI form */
   if (xvp->serialNo != stvp->serialNo) {
      GLuint outAttr, dummySlot;
      const GLbitfield outputsWritten = stvp->Base.Base.OutputsWritten;
      GLuint numVpOuts = 0;
      GLboolean emitPntSize = GL_FALSE, emitBFC0 = GL_FALSE, emitBFC1 = GL_FALSE;
      GLint maxGeneric;

      /* Compute mapping of vertex program outputs to slots, which depends
       * on the fragment program's input->slot mapping.
       */
      for (outAttr = 0; outAttr < VERT_RESULT_MAX; outAttr++) {
         /* set defaults: */
         xvp->output_to_slot[outAttr] = UNUSED;
         xvp->output_to_semantic_name[outAttr] = TGSI_SEMANTIC_COUNT;
         xvp->output_to_semantic_index[outAttr] = 99;

         if (outAttr == VERT_RESULT_HPOS) {
            /* always put xformed position into slot zero */
            xvp->output_to_slot[VERT_RESULT_HPOS] = 0;
            xvp->output_to_semantic_name[outAttr] = TGSI_SEMANTIC_POSITION;
            xvp->output_to_semantic_index[outAttr] = 0;
            numVpOuts++;
         }
         else if (outputsWritten & (1 << outAttr)) {
            /* see if the frag prog wants this vert output */
            GLint fpInAttrib = vp_out_to_fp_in(outAttr);
            if (fpInAttrib >= 0) {
               GLuint fpInSlot = stfp->input_to_slot[fpInAttrib];
               if (fpInSlot != ~0) {
                  /* match this vp output to the fp input */
                  GLuint vpOutSlot = stfp->input_map[fpInSlot];
                  xvp->output_to_slot[outAttr] = vpOutSlot;
                  xvp->output_to_semantic_name[outAttr] = stfp->input_semantic_name[fpInSlot];
                  xvp->output_to_semantic_index[outAttr] = stfp->input_semantic_index[fpInSlot];
                  numVpOuts++;
               }
            }
            else if (outAttr == VERT_RESULT_PSIZ)
               emitPntSize = GL_TRUE;
            else if (outAttr == VERT_RESULT_BFC0)
               emitBFC0 = GL_TRUE;
            else if (outAttr == VERT_RESULT_BFC1)
               emitBFC1 = GL_TRUE;
         }
#if 0 /*debug*/
         printf("assign vp output_to_slot[%d] = %d\n", outAttr, 
                xvp->output_to_slot[outAttr]);
#endif
      }

      /* must do these last */
      if (emitPntSize) {
         xvp->output_to_slot[VERT_RESULT_PSIZ] = numVpOuts++;
         xvp->output_to_semantic_name[VERT_RESULT_PSIZ] = TGSI_SEMANTIC_PSIZE;
         xvp->output_to_semantic_index[VERT_RESULT_PSIZ] = 0;
      }
      if (emitBFC0) {
         xvp->output_to_slot[VERT_RESULT_BFC0] = numVpOuts++;
         xvp->output_to_semantic_name[VERT_RESULT_BFC0] = TGSI_SEMANTIC_COLOR;
         xvp->output_to_semantic_index[VERT_RESULT_BFC0] = 0;
      }
      if (emitBFC1) {
         xvp->output_to_slot[VERT_RESULT_BFC1] = numVpOuts++;
         xvp->output_to_semantic_name[VERT_RESULT_BFC0] = TGSI_SEMANTIC_COLOR;
         xvp->output_to_semantic_index[VERT_RESULT_BFC0] = 1;
      }

      /* Unneeded vertex program outputs will go to this slot.
       * We could use this info to do dead code elimination in the
       * vertex program.
       */
      dummySlot = numVpOuts;

      /* find max GENERIC slot index */
      maxGeneric = -1;
      for (outAttr = 0; outAttr < VERT_RESULT_MAX; outAttr++) {
         if (xvp->output_to_semantic_name[outAttr] == TGSI_SEMANTIC_GENERIC) {
            maxGeneric = MAX2(maxGeneric,
                              xvp->output_to_semantic_index[outAttr]);
         }
      }

      /* Map vert program outputs that aren't used to the dummy slot
       * (and an unused generic attribute slot).
       */
      for (outAttr = 0; outAttr < VERT_RESULT_MAX; outAttr++) {
         if (outputsWritten & (1 << outAttr)) {
            if (xvp->output_to_slot[outAttr] == UNUSED) {
               xvp->output_to_slot[outAttr] = dummySlot;
               xvp->output_to_semantic_name[outAttr] = TGSI_SEMANTIC_GENERIC;
               xvp->output_to_semantic_index[outAttr] = maxGeneric + 1;
            }
         }

#if 0 /*debug*/
         printf("vp output_to_slot[%d] = %d\n", outAttr, 
                xvp->output_to_slot[outAttr]);
#endif
      }

      assert(stvp->Base.Base.NumInstructions > 1);

      st_translate_vertex_program(st, stvp, xvp->output_to_slot,
                                  xvp->output_to_semantic_name,
                                  xvp->output_to_semantic_index);

      xvp->vp = stvp;

      /* translated VP is up to date now */
      xvp->serialNo = stvp->serialNo;
   }

   return xvp;
}


void
st_free_translated_vertex_programs(struct st_context *st,
                                   struct translated_vertex_program *xvp)
{
   struct translated_vertex_program *next;

   while (xvp) {
      next = xvp->next;
      free(xvp);
      xvp = next;
   }
}


static void *
get_passthrough_fs(struct st_context *st)
{
   struct pipe_shader_state shader;

   if (!st->passthrough_fs) {
      st->passthrough_fs =
         util_make_fragment_passthrough_shader(st->pipe, &shader);
#if 0      /* We actually need to keep the tokens around at this time */
      free((void *) shader.tokens);
#endif
   }

   return st->passthrough_fs;
}


static void
update_linkage( struct st_context *st )
{
   struct st_vertex_program *stvp;
   struct st_fragment_program *stfp;
   struct translated_vertex_program *xvp;

   /* find active shader and params -- Should be covered by
    * ST_NEW_VERTEX_PROGRAM
    */
   assert(st->ctx->VertexProgram._Current);
   stvp = st_vertex_program(st->ctx->VertexProgram._Current);
   assert(stvp->Base.Base.Target == GL_VERTEX_PROGRAM_ARB);

   assert(st->ctx->FragmentProgram._Current);
   stfp = st_fragment_program(st->ctx->FragmentProgram._Current);
   assert(stfp->Base.Base.Target == GL_FRAGMENT_PROGRAM_ARB);

   xvp = find_translated_vp(st, stvp, stfp);

   st_reference_vertprog(st, &st->vp, stvp);
   st_reference_fragprog(st, &st->fp, stfp);

   cso_set_vertex_shader_handle(st->cso_context, stvp->driver_shader);

   if (st->missing_textures) {
      /* use a pass-through frag shader that uses no textures */
      void *fs = get_passthrough_fs(st);
      cso_set_fragment_shader_handle(st->cso_context, fs);
   }
   else {
      cso_set_fragment_shader_handle(st->cso_context, stfp->driver_shader);
   }

   st->vertex_result_to_slot = xvp->output_to_slot;
}


const struct st_tracked_state st_update_shader = {
   "st_update_shader",					/* name */
   {							/* dirty */
      0,						/* mesa */
      ST_NEW_VERTEX_PROGRAM | ST_NEW_FRAGMENT_PROGRAM	/* st */
   },
   update_linkage					/* update */
};
