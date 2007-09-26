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

/**
 * Primitive/vertex feedback (and/or discard) stage.
 * Used to implement transformation feedback/streaming and other things
 * which require a post-transformed vertex position (such as rasterpos,
 * selection and feedback modes).
 *
 * Authors:
 *  Brian Paul
 */


#include "pipe/p_util.h"
#include "draw_private.h"


struct feedback_stage {
   struct draw_stage stage;      /**< base class */
   uint num_prim_generated;      /**< number of primitives received */
   uint num_prim_emitted;        /**< number of primitives fed back */
   uint num_vert_emitted;        /**< number of vertices fed back */
   uint max_vert_emit;           /**< max number of verts we can emit */
   float *dest[PIPE_MAX_FEEDBACK_ATTRIBS];  /**< dests for vertex attribs */
};



/**
 * Check if there's space to store 'numVerts' in the feedback buffer(s).
 */
static boolean
check_space(const struct draw_stage *stage, uint numVerts)
{
   const struct feedback_stage *fs = (struct feedback_stage *) stage;
   return fs->num_vert_emitted + numVerts <= fs->max_vert_emit;
}


/**
 * Record the given vertex's attributes into the feedback buffer(s).
 */
static void
feedback_vertex(struct draw_stage *stage, const struct vertex_header *vertex)
{
   struct feedback_stage *fs = (struct feedback_stage *) stage;

#if 0
   const struct pipe_feedback_state *feedback = &stage->draw->feedback;
   const uint select = feedback->interleaved ? 0 : 1;
   uint i;

   /*
    * Note: 'select' is either 0 or 1.  By multiplying 'i' by 'select'
    * we can either address output buffer 0 (for interleaving) or
    * output buffer i (for non-interleaved).
    */
   for (i = 0; i < feedback->num_attribs; i++) {
      const uint slot = feedback->attrib[i];
      const float *src = slot ? vertex->data[slot] : vertex->clip;
      const uint size = feedback->size[i];
      float *dest = fs->dest[i * select];

      switch (size) {
      case 4:
         dest[3] = src[3];
         /* fall-through */
      case 3:
         dest[2] = src[2];
         /* fall-through */
      case 2:
         dest[1] = src[1];
         /* fall-through */
      case 1:
         dest[0] = src[0];
         /* fall-through */
      default:
         ;
      }
      fs->dest[i * select] += size;
   }

   fs->num_vert_emitted++;
}


static void feedback_begin( struct draw_stage *stage )
{
   struct feedback_stage *fs = (struct feedback_stage *) stage;
   const struct pipe_feedback_state *feedback = &stage->draw->feedback;

   fs->num_prim_generated = 0;
   fs->num_prim_emitted = 0;
   fs->num_vert_emitted = 0;

   assert(feedback->enabled);

   /* Compute max_vert_emit, the max number of vertices we can emit.
    * And, setup dest[] pointers.
    */
   if (stage->draw->feedback.interleaved) {
      uint i, vertex_size = 0;
      /* compute size of each interleaved vertex, in floats */
      for (i = 0; i < feedback->num_attribs; i++) {
         vertex_size += feedback->size[i];
      }
      /* compute max number of vertices we can feedback */
      fs->max_vert_emit = stage->draw->mapped_feedback_buffer_size[0]
         / sizeof(float) / vertex_size;

      fs->dest[0] = (float *) stage->draw->mapped_feedback_buffer[0];
   }
   else {
      uint i;
      uint max = ~0;
      for (i = 0; i < feedback->num_attribs; i++) {
         uint n = stage->draw->mapped_feedback_buffer_size[i]
            / sizeof(float) / feedback->size[i];
         if (n < max)
            max = n;
         fs->dest[i] = (float *) stage->draw->mapped_feedback_buffer[i];
      }
      fs->max_vert_emit = max;
   }

   if (!feedback->discard)
      stage->next->begin( stage->next );
}


static void feedback_tri( struct draw_stage *stage,
                          struct prim_header *header )
{
   struct feedback_stage *fs = (struct feedback_stage *) stage;

   fs->num_prim_generated++;

   if (stage->draw->feedback.enabled && check_space(stage, 3)) {
      feedback_vertex(stage, header->v[0]);
      feedback_vertex(stage, header->v[1]);
      feedback_vertex(stage, header->v[2]);
      fs->num_prim_emitted++;
   }

   if (!stage->draw->feedback.discard)
      stage->next->tri( stage->next, header );
}


static void feedback_line( struct draw_stage *stage,
                           struct prim_header *header )
{
   struct feedback_stage *fs = (struct feedback_stage *) stage;

   fs->num_prim_generated++;

   if (stage->draw->feedback.enabled && check_space(stage, 2)) {
      feedback_vertex(stage, header->v[0]);
      feedback_vertex(stage, header->v[1]);
      fs->num_prim_emitted++;
   }

   if (!stage->draw->feedback.discard)
      stage->next->line( stage->next, header );
}


static void feedback_point( struct draw_stage *stage,
                            struct prim_header *header )
{
   struct feedback_stage *fs = (struct feedback_stage *) stage;

   fs->num_prim_generated++;

   if (stage->draw->feedback.enabled && check_space(stage, 1)) {
      feedback_vertex(stage, header->v[0]);
      fs->num_prim_emitted++;
   }

   if (!stage->draw->feedback.discard)
      stage->next->point( stage->next, header );
}


static void feedback_end( struct draw_stage *stage )
{

   /* XXX Unmap the vertex feedback buffers so we can write to them */


   if (!stage->draw->feedback.discard)
      stage->next->end( stage->next );
}



static void feedback_reset_stipple_counter( struct draw_stage *stage )
{
   if (!stage->draw->feedback.discard)
      stage->next->reset_stipple_counter( stage->next );
}


/**
 * Create feedback drawing stage.
 */
struct draw_stage *draw_feedback_stage( struct draw_context *draw )
{
   struct feedback_stage *feedback = CALLOC_STRUCT(feedback_stage);

   feedback->stage.draw = draw;
   feedback->stage.next = NULL;
   feedback->stage.begin = feedback_begin;
   feedback->stage.point = feedback_point;
   feedback->stage.line = feedback_line;
   feedback->stage.tri = feedback_tri;
   feedback->stage.end = feedback_end;
   feedback->stage.reset_stipple_counter = feedback_reset_stipple_counter;

   return &feedback->stage;
}


