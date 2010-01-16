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
  *   Ian Romanick <idr@us.ibm.com>
  */

#include <spu_mfcio.h>

#include "pipe/p_state.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_math.h"
#include "draw/draw_private.h"
#include "draw/draw_context.h"
#include "cell/common.h"
#include "spu_vertex_shader.h"
#include "spu_exec.h"
#include "spu_main.h"


#define MAX_VERTEX_SIZE ((2 + PIPE_MAX_SHADER_OUTPUTS) * 4 * sizeof(float))


#define CLIP_RIGHT_BIT 0x01
#define CLIP_LEFT_BIT 0x02
#define CLIP_TOP_BIT 0x04
#define CLIP_BOTTOM_BIT 0x08
#define CLIP_FAR_BIT 0x10
#define CLIP_NEAR_BIT 0x20


static INLINE float
dot4(const float *a, const float *b)
{
   return (a[0]*b[0] +
           a[1]*b[1] +
           a[2]*b[2] +
           a[3]*b[3]);
}

static INLINE unsigned
compute_clipmask(const float *clip, /*const*/ float plane[][4], unsigned nr)
{
   unsigned mask = 0;
   unsigned i;

   /* Do the hardwired planes first:
    */
   if (-clip[0] + clip[3] < 0) mask |= CLIP_RIGHT_BIT;
   if ( clip[0] + clip[3] < 0) mask |= CLIP_LEFT_BIT;
   if (-clip[1] + clip[3] < 0) mask |= CLIP_TOP_BIT;
   if ( clip[1] + clip[3] < 0) mask |= CLIP_BOTTOM_BIT;
   if (-clip[2] + clip[3] < 0) mask |= CLIP_FAR_BIT;
   if ( clip[2] + clip[3] < 0) mask |= CLIP_NEAR_BIT;

   /* Followed by any remaining ones:
    */
   for (i = 6; i < nr; i++) {
      if (dot4(clip, plane[i]) < 0) 
         mask |= (1<<i);
   }

   return mask;
}


/**
 * Transform vertices with the current vertex program/shader
 * Up to four vertices can be shaded at a time.
 * \param vbuffer  the input vertex data
 * \param elts  indexes of four input vertices
 * \param count  number of vertices to shade [1..4]
 * \param vOut  array of pointers to four output vertices
 */
static void
run_vertex_program(struct spu_vs_context *draw,
                   unsigned elts[4], unsigned count,
                   const uint64_t *vOut)
{
   struct spu_exec_machine *machine = &draw->machine;
   unsigned int j;

   PIPE_ALIGN_VAR(16) struct spu_exec_vector inputs[PIPE_MAX_ATTRIBS];
   PIPE_ALIGN_VAR(16) struct spu_exec_vector outputs[PIPE_MAX_ATTRIBS];
   const float *scale = draw->viewport.scale;
   const float *trans = draw->viewport.translate;

   ASSERT(count <= 4);

   machine->Processor = TGSI_PROCESSOR_VERTEX;

   ASSERT_ALIGN16(draw->constants);
   machine->Consts = (float (*)[4]) draw->constants;

   machine->Inputs = inputs;
   machine->Outputs = outputs;

   spu_vertex_fetch( draw, machine, elts, count );

   /* run shader */
   spu_exec_machine_run( machine );


   /* store machine results */
   for (j = 0; j < count; j++) {
      unsigned slot;
      float x, y, z, w;
      PIPE_ALIGN_VAR(16)
      unsigned char buffer[sizeof(struct vertex_header)
          + MAX_VERTEX_SIZE];
      struct vertex_header *const tmpOut =
          (struct vertex_header *) buffer;
      const unsigned vert_size = ROUNDUP16(sizeof(struct vertex_header)
                                           + (sizeof(float) * 4 
                                              * draw->num_vs_outputs));

      mfc_get(tmpOut, vOut[j], vert_size, TAG_VERTEX_BUFFER, 0, 0);
      wait_on_mask(1 << TAG_VERTEX_BUFFER);


      /* Handle attr[0] (position) specially:
       *
       * XXX: Computing the clipmask should be done in the vertex
       * program as a set of DP4 instructions appended to the
       * user-provided code.
       */
      x = tmpOut->clip[0] = machine->Outputs[0].xyzw[0].f[j];
      y = tmpOut->clip[1] = machine->Outputs[0].xyzw[1].f[j];
      z = tmpOut->clip[2] = machine->Outputs[0].xyzw[2].f[j];
      w = tmpOut->clip[3] = machine->Outputs[0].xyzw[3].f[j];

      tmpOut->clipmask = compute_clipmask(tmpOut->clip, draw->plane,
					   draw->nr_planes);
      tmpOut->edgeflag = 1;

      /* divide by w */
      w = 1.0f / w;
      x *= w;
      y *= w;
      z *= w;

      /* Viewport mapping */
      tmpOut->data[0][0] = x * scale[0] + trans[0];
      tmpOut->data[0][1] = y * scale[1] + trans[1];
      tmpOut->data[0][2] = z * scale[2] + trans[2];
      tmpOut->data[0][3] = w;

      /* Remaining attributes are packed into sequential post-transform
       * vertex attrib slots.
       */
      for (slot = 1; slot < draw->num_vs_outputs; slot++) {
         tmpOut->data[slot][0] = machine->Outputs[slot].xyzw[0].f[j];
         tmpOut->data[slot][1] = machine->Outputs[slot].xyzw[1].f[j];
         tmpOut->data[slot][2] = machine->Outputs[slot].xyzw[2].f[j];
         tmpOut->data[slot][3] = machine->Outputs[slot].xyzw[3].f[j];
      }

      mfc_put(tmpOut, vOut[j], vert_size, TAG_VERTEX_BUFFER, 0, 0);
   } /* loop over vertices */
}


PIPE_ALIGN_VAR(16) unsigned char
immediates[(sizeof(float) * 4 * TGSI_EXEC_NUM_IMMEDIATES) + 32]);


void
spu_bind_vertex_shader(struct spu_vs_context *draw,
		       struct cell_shader_info *vs)
{
   const unsigned immediate_addr = vs->immediates;
   const unsigned immediate_size = 
       ROUNDUP16((sizeof(float) * 4 * vs->num_immediates)
		 + (immediate_addr & 0x0f));
 

   mfc_get(immediates, immediate_addr & ~0x0f, immediate_size,
           TAG_VERTEX_BUFFER, 0, 0);

   draw->machine.Instructions = (struct tgsi_full_instruction *)
       vs->instructions;
   draw->machine.NumInstructions = vs->num_instructions;

   draw->machine.Declarations = (struct tgsi_full_declaration *)
       vs->declarations;
   draw->machine.NumDeclarations = vs->num_declarations;

   draw->num_vs_outputs = vs->num_outputs;

   /* specify the shader to interpret/execute */
   spu_exec_machine_init(&draw->machine,
			 PIPE_MAX_SAMPLERS,
			 NULL /*samplers*/,
			 PIPE_SHADER_VERTEX);

   wait_on_mask(1 << TAG_VERTEX_BUFFER);

   (void) memcpy(& draw->machine.Imms, &immediates[immediate_addr & 0x0f],
                 sizeof(float) * 4 * vs->num_immediates);
}


void
spu_execute_vertex_shader(struct spu_vs_context *draw,
                          const struct cell_command_vs *vs)
{
   unsigned i;

   (void) memcpy(draw->plane, vs->plane, sizeof(float) * 4 * vs->nr_planes);
   draw->nr_planes = vs->nr_planes;
   draw->vertex_fetch.nr_attrs = vs->nr_attrs;

   for (i = 0; i < vs->num_elts; i += 4) {
      const unsigned batch_size = MIN2(vs->num_elts - i, 4);

      run_vertex_program(draw, & vs->elts[i], batch_size, &vs->vOut[i]);
   }
}
