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

#include "pipe/p_context.h"
#include "pipe/tgsi/mesa/mesa_to_tgsi.h"
#include "pipe/tgsi/exec/tgsi_core.h"

#include "st_context.h"
#include "st_cache.h"
#include "st_atom.h"
#include "st_program.h"
#include "st_atom_shader.h"


/**
 * This represents a vertex program, especially translated to match
 * the inputs of a particular fragment shader.
 */
struct translated_vertex_program
{
   /** The fragment shader "signature" this vertex shader is meant for: */
   GLbitfield frag_inputs;

   /** Compared against master vertex program's serialNo: */
   GLuint serialNo;

   /** Maps VERT_RESULT_x to slot */
   GLuint output_to_slot[VERT_RESULT_MAX];

   /** The program in TGSI format */
   struct tgsi_token tokens[ST_MAX_SHADER_TOKENS];

   /** Pointer to the translated, cached vertex shader */
   const struct cso_vertex_shader *vs;

   struct translated_vertex_program *next;  /**< next in linked list */
};



/**
 * Free data hanging off the st vert prog.
 */
void
st_remove_vertex_program(struct st_context *st, struct st_vertex_program *stvp)
{
   /* no-op, for now? */
}


/**
 * Free data hanging off the st frag prog.
 */
void
st_remove_fragment_program(struct st_context *st,
                          struct st_fragment_program *stfp)
{
   struct translated_vertex_program *xvp, *next;

   for (xvp = stfp->vertex_programs; xvp; xvp = next) {
      next = xvp->next;
      /* XXX free xvp->vs */
      free(xvp);
   }
}



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
   const GLbitfield fragInputsRead
      = stfp->Base.Base.InputsRead | FRAG_BIT_WPOS;

   /*
    * Translate fragment program if needed.
    */
   if (!stfp->fs) {
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

      (void) st_translate_fragment_program(st, stfp,
                                           stfp->input_to_slot,
                                           stfp->tokens,
                                           ST_MAX_SHADER_TOKENS);
      assert(stfp->fs);
   }


   /* See if we've got a translated vertex program whose outputs match
    * the fragment program's inputs.
    * XXX This could be a hash lookup, using InputsRead as the key.
    */
   for (xvp = stfp->vertex_programs; xvp; xvp = xvp->next) {
      if (xvp->frag_inputs == stfp->Base.Base.InputsRead) {
         break;
      }
   }

   /* No?  Allocate translated vp object now */
   if (!xvp) {
      xvp = CALLOC_STRUCT(translated_vertex_program);
      xvp->frag_inputs = fragInputsRead;

      xvp->next = stfp->vertex_programs;
      stfp->vertex_programs = xvp;
   }

   /* See if we need to translate vertex program to TGSI form */
   if (xvp->serialNo != stvp->serialNo) {
      GLuint outAttr, dummySlot;
      const GLbitfield outputsWritten = stvp->Base.Base.OutputsWritten;

      /* Compute mapping of vertex program outputs to slots, which depends
       * on the fragment program's input->slot mapping.
       */
      for (outAttr = 0; outAttr < VERT_RESULT_MAX; outAttr++) {
         /* set default: */
         xvp->output_to_slot[outAttr] = UNUSED;

         if (outputsWritten & (1 << outAttr)) {
            /* see if the frag prog wants this vert output */
            GLint fpIn = vp_out_to_fp_in(outAttr);
            if (fpIn >= 0) {
               xvp->output_to_slot[outAttr] = stfp->input_to_slot[fpIn];
            }
         }
      }

      /* Unneeded vertex program outputs will go to this slot.
       * We could use this info to do dead code elimination in the
       * vertex program.
       */
      dummySlot = stfp->num_input_slots;

      /* Map vert program outputs that aren't used to the dummy slot */
      for (outAttr = 0; outAttr < VERT_RESULT_MAX; outAttr++) {
         if (outputsWritten & (1 << outAttr)) {
            if (xvp->output_to_slot[outAttr] == UNUSED)
               xvp->output_to_slot[outAttr] = dummySlot;
         }
      }


      xvp->vs = st_translate_vertex_program(st, stvp,
                                            xvp->output_to_slot,
                                            xvp->tokens,
                                            ST_MAX_SHADER_TOKENS);
      assert(xvp->vs);
      stvp->vs = NULL; /* don't want to use this */

      /* translated VP is up to date now */
      xvp->serialNo = stvp->serialNo;
   }

   return xvp;
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

   xvp = find_translated_vp(st, stvp, stfp);

   st->vp = stvp;
   st->state.vs = xvp->vs;
   st->pipe->bind_vs_state(st->pipe, st->state.vs->data);

   st->fp = stfp;
   st->state.fs = stfp->fs;
   st->pipe->bind_fs_state(st->pipe, st->state.fs->data);

   st->vertex_result_to_slot = xvp->output_to_slot;
}


const struct st_tracked_state st_update_shader = {
   .name = "st_update_shader",
   .dirty = {
      .mesa  = _NEW_PROGRAM,
      .st   = ST_NEW_SHADER
   },
   .update = update_linkage
};
