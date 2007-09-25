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
 * Structure to describe a (vertex program, fragment program) pair
 * which is linked together (used together to render something).  This
 * linkage basically servers the same purpose as the OpenGL Shading
 * Language linker, but also applies to ARB programs and Mesa's
 * fixed-function-generated programs.
 *
 * More background:
 *
 * The translation from Mesa programs to TGSI programs depends on the
 * linkage between the vertex program and the fragment program.  This is
 * because we tightly pack the inputs and outputs of shaders into
 * consecutive "slots".
 *
 * Suppose an app uses one vertex program "VP" (outputting pos, color and tex0)
 * and two fragment programs:
 *    FP1: uses tex0 input only (input slot 0)
 *    FP2: uses color input only (input slot 0)
 *
 * When VP is used with FP1 we want VP.output[2] to match FP1.input[0], but
 * when VP is used with FP2 we want VP.output[1] to match FP1.input[0].
 *
 * We don't want to re-translate the vertex and/or fragment programs
 * each time the VP/FP bindings/linkings change.  The solution is this
 * structure which stores the translated TGSI shaders on a per-linkage
 * basis.
 * 
 */
struct linked_program_pair
{
   struct st_vertex_program *vprog;  /**< never changes */
   struct st_fragment_program *fprog;  /**< never changes */

   struct tgsi_token vs_tokens[ST_FP_MAX_TOKENS];
   struct tgsi_token fs_tokens[ST_FP_MAX_TOKENS];

   const struct cso_vertex_shader *vs;
   const struct cso_fragment_shader *fs;

   GLuint vertSerialNo, fragSerialNo;

   /** maps a Mesa VERT_ATTRIB_x to a packed TGSI input index */
   GLuint vp_input_to_index[MAX_VERTEX_PROGRAM_ATTRIBS];
   /** maps a TGSI input index back to a Mesa VERT_ATTRIB_x */
   GLuint vp_index_to_input[MAX_VERTEX_PROGRAM_ATTRIBS];

   GLuint vp_result_to_slot[VERT_RESULT_MAX];

   struct linked_program_pair *next;
};


/** XXX temporary - use some kind of hash table instead */
static struct linked_program_pair *Pairs = NULL;


static void
find_and_remove(struct gl_program *prog)
{
   struct linked_program_pair *pair, *prev = NULL, *next;
   for (pair = Pairs; pair; pair = next) {
      next = pair->next;
      if (pair->vprog == (struct st_vertex_program *) prog ||
          pair->fprog == (struct st_fragment_program *) prog) {
         /* unlink */
         if (prev)
            prev->next = next;
         else
            Pairs = next;
         /* delete pair->vs */
         /* delete pair->fs */
         free(pair);
      }
      else {
         prev = pair;
      }
   }
}


/**
 * Delete any known program pairs that use the given vertex program.
 */
void
st_remove_vertex_program(struct st_context *st, struct st_vertex_program *stvp)
{
   find_and_remove(&stvp->Base.Base);
}


/**
 * Delete any known program pairs that use the given fragment program.
 */
void
st_remove_fragment_program(struct st_context *st,
                          struct st_fragment_program *stfp)
{
   find_and_remove(&stfp->Base.Base);
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
 * Examine the outputs written by a vertex program and the inputs read
 * by a fragment program to determine which match up and where they
 * should be mapped into the generic shader output/input slots.
 * \param vert_output_map  returns the vertex output register mapping
 * \param frag_input_map  returns the fragment input register mapping
 */
static GLuint
link_outputs_to_inputs(GLbitfield outputsWritten,
                       GLbitfield inputsRead,
                       GLuint vert_output_map[],
                       GLuint frag_input_map[])
{
   static const GLuint UNUSED = ~0;
   GLint vert_slot_to_attr[50], frag_slot_to_attr[50];
   GLuint outAttr, inAttr;
   GLuint numIn = 0, dummySlot;

   for (inAttr = 0; inAttr < FRAG_ATTRIB_MAX; inAttr++) {
      if (inputsRead & (1 << inAttr)) {
         frag_input_map[inAttr] = numIn;
         frag_slot_to_attr[numIn] = inAttr;
         numIn++;
      }
      else {
         frag_input_map[inAttr] = UNUSED;
      }
   }

   for (outAttr = 0; outAttr < VERT_RESULT_MAX; outAttr++) {
      if (outputsWritten & (1 << outAttr)) {
         /* see if the frag prog wants this vert output */
         GLint fpIn = vp_out_to_fp_in(outAttr);

         if (fpIn >= 0) {
            GLuint frag_slot = frag_input_map[fpIn];
            vert_output_map[outAttr] = frag_slot;
            vert_slot_to_attr[frag_slot] = outAttr;
         }
         else {
            vert_output_map[outAttr] = UNUSED;
         }
      }
      else { 
         vert_output_map[outAttr] = UNUSED;
      }
   }

   /*
    * We'll map all unused vertex program outputs to this slot.
    * We'll also map all undefined fragment program inputs to this slot.
    */
   dummySlot = numIn;

   /* Map vert program outputs that aren't used to the dummy slot */
   for (outAttr = 0; outAttr < VERT_RESULT_MAX; outAttr++) {
      if (outputsWritten & (1 << outAttr)) {
         if (vert_output_map[outAttr] == UNUSED)
            vert_output_map[outAttr] = dummySlot;
      }
   }

   /* Map frag program inputs that aren't defined to the dummy slot */
   for (inAttr = 0; inAttr < FRAG_ATTRIB_MAX; inAttr++) {
      if (inputsRead & (1 << inAttr)) {
         if (frag_input_map[inAttr] == UNUSED)
            frag_input_map[inAttr] = dummySlot;
      }
   }

#if 0
   printf("vOut  W  slot\n");
   for (outAttr = 0; outAttr < VERT_RESULT_MAX; outAttr++) {
      printf("%4d  %c %4d\n", outAttr,
             " *"[(outputsWritten >> outAttr) & 1],
             vert_output_map[outAttr]);
   }
   printf("vIn  R  slot\n");
   for (inAttr = 0; inAttr < FRAG_ATTRIB_MAX; inAttr++) {
      printf("%3d  %c %4d\n", inAttr,
             " *"[(inputsRead >> inAttr) & 1],
             frag_input_map[inAttr]);
   }
#endif

   return numIn;
}


static struct linked_program_pair *
lookup_program_pair(struct st_context *st,
                    struct st_vertex_program *vprog,
                    struct st_fragment_program *fprog)
{
   struct linked_program_pair *pair;

   /* search */
   for (pair = Pairs; pair; pair = pair->next) {
      if (pair->vprog == vprog && pair->fprog == fprog) {
         /* found it */
         break;
      }
   }

   /*
    * Examine the outputs of the vertex shader and the inputs of the
    * fragment shader to determine how to match both to a common set
    * of slots.
    */
   if (!pair) {
      pair = CALLOC_STRUCT(linked_program_pair);
      if (pair) {
         pair->vprog = vprog;
         pair->fprog = fprog;
      }
   }

   return pair;
}


static void
link_shaders(struct st_context *st, struct linked_program_pair *pair)
{
   struct st_vertex_program *vprog = pair->vprog;
   struct st_fragment_program *fprog = pair->fprog;

   assert(vprog);
   assert(fprog);

   if (pair->vertSerialNo != vprog->serialNo ||
       pair->fragSerialNo != fprog->serialNo) {
      /* re-link and re-translate */
      GLuint vert_output_mapping[VERT_RESULT_MAX];
      GLuint frag_input_mapping[FRAG_ATTRIB_MAX];

      link_outputs_to_inputs(vprog->Base.Base.OutputsWritten,
                             fprog->Base.Base.InputsRead | FRAG_BIT_WPOS,
                             vert_output_mapping,
                             frag_input_mapping);

      /* xlate vp to vs + vs tokens */
      st_translate_vertex_program(st, vprog,
                                  vert_output_mapping,
                                  pair->vs_tokens, ST_FP_MAX_TOKENS);

      pair->vprog = vprog;
      /* temp hacks */
      pair->vs = vprog->vs;
      vprog->vs = NULL;


      /* xlate fp to fs + fs tokens */
      st_translate_fragment_program(st, fprog,
                                    frag_input_mapping,
                                    pair->fs_tokens, ST_FP_MAX_TOKENS);
      pair->fprog = fprog;
      /* temp hacks */
      pair->fs = fprog->fs;
      fprog->fs = NULL;

      /* save pair */
      pair->next = Pairs;
      Pairs = pair;

      pair->vertSerialNo = vprog->serialNo;
      pair->fragSerialNo = fprog->serialNo;
   }
}


static void
update_linkage( struct st_context *st )
{
   struct linked_program_pair *pair;
   struct st_vertex_program *stvp;
   struct st_fragment_program *stfp;

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


   pair = lookup_program_pair(st, stvp, stfp);
   assert(pair);
   link_shaders(st, pair);


   /* Bind the vertex program and TGSI shader */
   st->vp = stvp;
   st->state.vs = pair->vs;
   st->pipe->bind_vs_state(st->pipe, st->state.vs->data);

   /* Bind the fragment program and TGSI shader */
   st->fp = stfp;
   st->state.fs = pair->fs;
   st->pipe->bind_fs_state(st->pipe, st->state.fs->data);

   st->vertex_result_to_slot = pair->vp_result_to_slot;
}


const struct st_tracked_state st_update_shader = {
   .name = "st_update_shader",
   .dirty = {
      .mesa  = 0,
      .st   = ST_NEW_LINKAGE
   },
   .update = update_linkage
};
