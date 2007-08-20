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

/* Author:
 *    Brian Paul
 *    Keith Whitwell
 */


#include "pipe/p_defines.h"
#include "pipe/p_context.h"
#include "pipe/p_winsys.h"
#include "pipe/p_util.h"

#include "pipe/draw/draw_private.h"
#include "pipe/draw/draw_context.h"
#include "pipe/draw/draw_prim.h"

#include "pipe/tgsi/core/tgsi_exec.h"
#include "pipe/tgsi/core/tgsi_build.h"
#include "pipe/tgsi/core/tgsi_util.h"


#if defined __GNUC__
#define ALIGN16_DECL(TYPE, NAME, SIZE)  TYPE NAME[SIZE] __attribute__(( aligned( 16 ) ))
#define ALIGN16_ASSIGN(P) P
#else
#define ALIGN16_DECL(TYPE, NAME, SIZE)  TYPE NAME[SIZE + 1]
#define ALIGN16_ASSIGN(P) align16(P)
#endif



static INLINE unsigned
compute_clipmask(float cx, float cy, float cz, float cw)
{
   unsigned mask;
#if defined(macintosh) || defined(__powerpc__)
   /* on powerpc cliptest is 17% faster in this way. */
   mask =  (((cw <  cx) << CLIP_RIGHT_SHIFT));
   mask |= (((cw < -cx) << CLIP_LEFT_SHIFT));
   mask |= (((cw <  cy) << CLIP_TOP_SHIFT));
   mask |= (((cw < -cy) << CLIP_BOTTOM_SHIFT));
   mask |= (((cw <  cz) << CLIP_FAR_SHIFT));
   mask |= (((cw < -cz) << CLIP_NEAR_SHIFT));
#else /* !defined(macintosh)) */
   mask = 0x0;
   if (-cx + cw < 0) mask |= CLIP_RIGHT_BIT;
   if ( cx + cw < 0) mask |= CLIP_LEFT_BIT;
   if (-cy + cw < 0) mask |= CLIP_TOP_BIT;
   if ( cy + cw < 0) mask |= CLIP_BOTTOM_BIT;
   if (-cz + cw < 0) mask |= CLIP_FAR_BIT;
   if ( cz + cw < 0) mask |= CLIP_NEAR_BIT;
#endif /* defined(macintosh) */
   return mask;
}


/**
 * Fetch a float[4] vertex attribute from memory, doing format/type
 * conversion as needed.
 * XXX this might be a temporary thing.
 */
static void
fetch_attrib4(const void *ptr, unsigned format, float attrib[4])
{
   /* defaults */
   attrib[1] = 0.0;
   attrib[2] = 0.0;
   attrib[3] = 1.0;
   switch (format) {
   case PIPE_FORMAT_R32G32B32A32_FLOAT:
      attrib[3] = ((float *) ptr)[3];
      /* fall-through */
   case PIPE_FORMAT_R32G32B32_FLOAT:
      attrib[2] = ((float *) ptr)[2];
      /* fall-through */
   case PIPE_FORMAT_R32G32_FLOAT:
      attrib[1] = ((float *) ptr)[1];
      /* fall-through */
   case PIPE_FORMAT_R32_FLOAT:
      attrib[0] = ((float *) ptr)[0];
      break;
   default:
      assert(0);
   }
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
run_vertex_program(struct draw_context *draw,
                   unsigned elts[4], unsigned count,
                   struct vertex_header *vOut[])
{
   struct tgsi_exec_machine machine;
   unsigned int j;

   ALIGN16_DECL(struct tgsi_exec_vector, inputs, PIPE_ATTRIB_MAX);
   ALIGN16_DECL(struct tgsi_exec_vector, outputs, PIPE_ATTRIB_MAX);
   const float *scale = draw->viewport.scale;
   const float *trans = draw->viewport.translate;

   assert(count <= 4);

#ifdef DEBUG
   memset( &machine, 0, sizeof( machine ) );
#endif

   /* init machine state */
   tgsi_exec_machine_init(&machine,
                          draw->vertex_shader.tokens,
                          PIPE_MAX_SAMPLERS,
                          NULL /*samplers*/ );

   /* Consts does not require 16 byte alignment. */
   machine.Consts = draw->vertex_shader.constants->constant;

   machine.Inputs = ALIGN16_ASSIGN(inputs);
   machine.Outputs = ALIGN16_ASSIGN(outputs);


   if (0)
   {
      unsigned attr;
      for (attr = 0; attr < 16; attr++) {
         if (draw->vertex_shader.inputs_read & (1 << attr)) {
            printf("attr %d: buf_off %d  src_off %d  pitch %d\n",
                   attr, 
                   draw->vertex_buffer[attr].buffer_offset,
                   draw->vertex_element[attr].src_offset,
                   draw->vertex_buffer[attr].pitch);
         }
      }
   }

   /* load machine inputs */
   for (j = 0; j < count; j++) {
      unsigned attr;
      for (attr = 0; attr < 16; attr++) {
         if (draw->vertex_shader.inputs_read & (1 << attr)) {
            unsigned buf = draw->vertex_element[attr].vertex_buffer_index;
            const void *src
               = (const void *) ((const ubyte *) draw->mapped_vbuffer[buf]
                                 + draw->vertex_buffer[buf].buffer_offset
                                 + draw->vertex_element[attr].src_offset
                                 + elts[j] * draw->vertex_buffer[buf].pitch);
            float p[4];

            fetch_attrib4(src, draw->vertex_element[attr].src_format, p);

            machine.Inputs[attr].xyzw[0].f[j] = p[0]; /*X*/
            machine.Inputs[attr].xyzw[1].f[j] = p[1]; /*Y*/
            machine.Inputs[attr].xyzw[2].f[j] = p[2]; /*Z*/
            machine.Inputs[attr].xyzw[3].f[j] = p[3]; /*W*/
#if 0
            if (attr == 0) {
               printf("Input vertex %d: %f %f %f\n",
                      j, p[0], p[1], p[2]);
            }
#endif
         }
      }
   }

#if 0
   printf("Consts:\n");
   for (i = 0; i < 4; i++) {
      printf(" %d: %f %f %f %f\n", i,
             machine.Consts[i][0],
             machine.Consts[i][1],
             machine.Consts[i][2],
             machine.Consts[i][3]);
   }
#endif

   /* run shader */
   tgsi_exec_machine_run( &machine );

#if 0
   printf("VS result: %f %f %f %f\n",
          outputs[0].xyzw[0].f[0],
          outputs[0].xyzw[1].f[0],
          outputs[0].xyzw[2].f[0],
          outputs[0].xyzw[3].f[0]);
#endif

   /* store machine results */
   assert(draw->vertex_shader.outputs_written & (1 << VERT_RESULT_HPOS));
   for (j = 0; j < count; j++) {
      unsigned attr, slot;
      float x, y, z, w;

      /* Handle attr[0] (position) specially: */
      x = vOut[j]->clip[0] = outputs[0].xyzw[0].f[j];
      y = vOut[j]->clip[1] = outputs[0].xyzw[1].f[j];
      z = vOut[j]->clip[2] = outputs[0].xyzw[2].f[j];
      w = vOut[j]->clip[3] = outputs[0].xyzw[3].f[j];

      vOut[j]->clipmask = compute_clipmask(x, y, z, w);
      vOut[j]->edgeflag = 1;

      /* divide by w */
      w = 1.0 / w;
      x *= w;
      y *= w;
      z *= w;

      /* Viewport mapping */
      vOut[j]->data[0][0] = x * scale[0] + trans[0];
      vOut[j]->data[0][1] = y * scale[1] + trans[1];
      vOut[j]->data[0][2] = z * scale[2] + trans[2];
      vOut[j]->data[0][3] = w;
#if 0
      printf("wincoord: %f %f %f\n",
             vOut[j]->data[0][0],
             vOut[j]->data[0][1],
             vOut[j]->data[0][2]);
#endif

      /* remaining attributes: */
      /* pack into sequential post-transform attrib slots */
      slot = 1;
      for (attr = 1; attr < VERT_RESULT_MAX; attr++) {
         if (draw->vertex_shader.outputs_written & (1 << attr)) {
            assert(slot < draw->nr_attrs);
            vOut[j]->data[slot][0] = outputs[attr].xyzw[0].f[j];
            vOut[j]->data[slot][1] = outputs[attr].xyzw[1].f[j];
            vOut[j]->data[slot][2] = outputs[attr].xyzw[2].f[j];
            vOut[j]->data[slot][3] = outputs[attr].xyzw[3].f[j];
            slot++;
         }
      }
   }

#if 0
   memcpy(
      quad->outputs.color,
      &machine.Outputs[1].xyzw[0].f[0],
      sizeof( quad->outputs.color ) );
#endif
}


/**
 * Called by the draw module when the vertx cache needs to be flushed.
 * This involves running the vertex shader.
 */
static void vs_flush( struct draw_context *draw )
{
   unsigned i, j;

   /* run vertex shader on vertex cache entries, four per invokation */
   for (i = 0; i < draw->vs.queue_nr; i += 4) {
      struct vertex_header *dests[4];
      unsigned elts[4];
      int n;

      for (j = 0; j < 4; j++) {
         elts[j] = draw->vs.queue[i + j].elt;
         dests[j] = draw->vs.queue[i + j].dest;
      }

      n = MIN2(4, draw->vs.queue_nr - i);
      assert(n > 0);
      assert(n <= 4);

      run_vertex_program(draw, elts, n, dests);
   }

   draw->vs.queue_nr = 0;
}


void draw_set_mapped_vertex_buffer(struct draw_context *draw,
                                   unsigned attr, const void *buffer)
{
   draw->mapped_vbuffer[attr] = buffer;
}


/**
 * Draw vertex arrays
 * This is the main entrypoint into the drawing module.
 * \param prim  one of PIPE_PRIM_x
 * \param start  index of first vertex to draw
 * \param count  number of vertices to draw
 */
void
draw_arrays(struct draw_context *draw, unsigned prim,
            unsigned start, unsigned count)
{
   /* tell drawing pipeline we're beginning drawing */
   draw->pipeline.first->begin( draw->pipeline.first );

   draw->vs_flush = vs_flush;

   draw_invalidate_vcache( draw );

   draw_set_prim( draw, prim );

   /* drawing done here: */
   draw_prim(draw, start, count);

   /* draw any left-over buffered prims */
   draw_flush(draw);

   /* tell drawing pipeline we're done drawing */
   draw->pipeline.first->end( draw->pipeline.first );
}


/* XXX move this into draw_context.c? */

#define EMIT_ATTR( VF_ATTR, STYLE, SIZE )			\
do {								\
   if (draw->nr_attrs >= 2)					\
      draw->vf_attr_to_slot[VF_ATTR] = draw->nr_attrs - 2;	\
   draw->attrs[draw->nr_attrs].attrib = VF_ATTR;		\
   draw->attrs[draw->nr_attrs].format = STYLE;			\
   draw->nr_attrs++;						\
   draw->vertex_size += SIZE;					\
} while (0)


/**
 * XXX very similar to same func in draw_vb.c (which will go away)
 */
void
draw_set_vertex_attributes( struct draw_context *draw,
                            const unsigned *slot_to_vf_attr,
                            unsigned nr_attrs )
{
   unsigned i;

   memset(draw->vf_attr_to_slot, 0, sizeof(draw->vf_attr_to_slot));
   draw->nr_attrs = 0;
   draw->vertex_size = 0;

   /*
    * First three attribs are always the same: header, clip pos, winpos
    */
   EMIT_ATTR(VF_ATTRIB_VERTEX_HEADER, EMIT_1F, 1);
   EMIT_ATTR(VF_ATTRIB_CLIP_POS, EMIT_4F, 4);

   assert(slot_to_vf_attr[0] == VF_ATTRIB_POS);
   EMIT_ATTR(slot_to_vf_attr[0], EMIT_4F_VIEWPORT, 4);

   /*
    * Remaining attribs (color, texcoords, etc)
    */
   for (i = 1; i < nr_attrs; i++) 
      EMIT_ATTR(slot_to_vf_attr[i], EMIT_4F, 4);

   draw->vertex_size *= 4; /* floats to bytes */
}
