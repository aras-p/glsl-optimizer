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


#include "main/context.h"

#include "pipe/p_defines.h"
#include "pipe/p_context.h"
#include "pipe/p_winsys.h"

#include "sp_context.h"
#include "sp_state.h"

#include "pipe/draw/draw_private.h"
#include "pipe/draw/draw_context.h"
#include "pipe/draw/draw_prim.h"

#include "pipe/tgsi/core/tgsi_exec.h"
#include "pipe/tgsi/core/tgsi_build.h"
#include "pipe/tgsi/core/tgsi_util.h"


#if defined __GNUC__
#define USE_ALIGNED_ATTRIBS   1
#define ALIGN16_SUFFIX        __attribute__(( aligned( 16 ) ))
#else
#define USE_ALIGNED_ATTRIBS   0
#define ALIGN16_SUFFIX
#endif


static struct softpipe_context *sp_global = NULL;


static void
run_vertex_program2(struct draw_context *draw,
                   const void *vbuffer, unsigned elem,
                   struct vertex_header *vOut)
{
#if 1
   struct softpipe_context *sp = sp_global;
#endif
   struct tgsi_exec_machine machine;
   int i;

#if USE_ALIGNED_ATTRIBS
   struct tgsi_exec_vector inputs[PIPE_ATTRIB_MAX] ALIGN16_SUFFIX;
   struct tgsi_exec_vector outputs[PIPE_ATTRIB_MAX] ALIGN16_SUFFIX;
#else
   struct tgsi_exec_vector inputs[PIPE_ATTRIB_MAX + 1];
   struct tgsi_exec_vector outputs[PIPE_ATTRIB_MAX + 1];
#endif

#ifdef DEBUG
   memset( &machine, 0, sizeof( machine ) );
#endif

   /* init machine state */
   tgsi_exec_machine_init(
                          &machine,
                          sp->vs.tokens,
                          PIPE_MAX_SAMPLERS,
                          NULL /*samplers*/ );

   /* Consts does not require 16 byte alignment. */
   machine.Consts = sp->vs.constants->constant;

#if USE_ALIGNED_ATTRIBS
   machine.Inputs = inputs;
   machine.Outputs = outputs;
#else
   machine.Inputs = (struct tgsi_exec_vector *) tgsi_align_128bit( inputs );
   machine.Outputs = (struct tgsi_exec_vector *) tgsi_align_128bit( outputs );
#endif

   {
      const void *mapped = vbuffer;
      const float *vIn, *cIn;
      vIn = (const float *) ((const ubyte *) mapped
                             + draw->vertex_buffer[0].buffer_offset
                             + draw->vertex_element[0].src_offset
                             + elem * draw->vertex_buffer[0].pitch);

      cIn = (const float *) ((const ubyte *) mapped
                             + draw->vertex_buffer[3].buffer_offset
                             + draw->vertex_element[3].src_offset
                             + elem * draw->vertex_buffer[3].pitch);
      /*X*/
      machine.Inputs[0].xyzw[0].f[0] = vIn[0];
      machine.Inputs[0].xyzw[0].f[1] = vIn[0];
      machine.Inputs[0].xyzw[0].f[2] = vIn[0];
      machine.Inputs[0].xyzw[0].f[3] = vIn[0];

      /*Y*/
      machine.Inputs[0].xyzw[1].f[0] = vIn[1];
      machine.Inputs[0].xyzw[1].f[1] = vIn[1];
      machine.Inputs[0].xyzw[1].f[2] = vIn[1];
      machine.Inputs[0].xyzw[1].f[3] = vIn[1];

      /*Z*/
      machine.Inputs[0].xyzw[2].f[0] = vIn[2];
      machine.Inputs[0].xyzw[2].f[1] = vIn[2];
      machine.Inputs[0].xyzw[2].f[2] = vIn[2];
      machine.Inputs[0].xyzw[2].f[3] = vIn[2];

      /*W*/
      machine.Inputs[0].xyzw[3].f[0] = 1.0;
      machine.Inputs[0].xyzw[3].f[1] = 1.0;
      machine.Inputs[0].xyzw[3].f[2] = 1.0;
      machine.Inputs[0].xyzw[3].f[3] = 1.0;

      printf("VS Input: %f %f %f %f\n",
             vIn[0], vIn[1], vIn[2], 1.0);
   }

   printf("Consts:\n");
   for (i = 0; i < 4; i++) {
      printf(" %d: %f %f %f %f\n", i,
             machine.Consts[i][0],
             machine.Consts[i][1],
             machine.Consts[i][2],
             machine.Consts[i][3]);
   }


   /* run shader */
   tgsi_exec_machine_run( &machine );

   /* store result pos */
   printf("VS result: %f %f %f %f\n",
          outputs[0].xyzw[0].f[0],
          outputs[0].xyzw[1].f[0],
          outputs[0].xyzw[2].f[0],
          outputs[0].xyzw[3].f[0]);
   {
      const float *scale = draw->viewport.scale;
      const float *trans = draw->viewport.translate;
      float x, y, z, w;

      x = outputs[0].xyzw[0].f[0];
      y = outputs[0].xyzw[1].f[0];
      z = outputs[0].xyzw[2].f[0];
      w = outputs[0].xyzw[3].f[0];

      /* divide by w */
      x /= w;
      y /= w;
      z /= w;
      w = 1.0 / w;

      /* Viewport */
      vOut->data[0][0] = scale[0] * x + trans[0];
      vOut->data[0][1] = scale[1] * y + trans[1];
      vOut->data[0][2] = scale[2] * z + trans[2];
      vOut->data[0][3] = w;
      printf("wincoord: %f %f %f\n",
             vOut->data[0][0],
             vOut->data[0][1],
             vOut->data[0][2]);

      vOut->data[1][0] = 1.0;
      vOut->data[1][1] = 1.0;
      vOut->data[1][2] = 1.0;
      vOut->data[1][3] = 1.0;

   }

#if 0
   memcpy(
      quad->outputs.color,
      &machine.Outputs[1].xyzw[0].f[0],
      sizeof( quad->outputs.color ) );
#endif
}


/**
 * Stand-in for actual vertex program execution
 * XXX this will probably live in a new file, like "sp_vs.c"
 * \param draw  the drawing context
 * \param vbuffer  the mapped vertex buffer pointer
 * \param elem  which element of the vertex buffer to use as input
 * \param vOut  the output vertex
 */
static void
run_vertex_program(struct draw_context *draw,
                   const void *vbuffer, unsigned elem,
                   struct vertex_header *vOut)
{
   run_vertex_program2(draw, vbuffer, elem, vOut);

#if 0
   const float *vIn, *cIn;
   const float *scale = draw->viewport.scale;
   const float *trans = draw->viewport.translate;
   const void *mapped = vbuffer;

   /* XXX temporary hack: */
   GET_CURRENT_CONTEXT(ctx);
   const float *m = ctx->_ModelProjectMatrix.m;

   vIn = (const float *) ((const ubyte *) mapped
                          + draw->vertex_buffer[0].buffer_offset
                          + draw->vertex_element[0].src_offset
                          + elem * draw->vertex_buffer[0].pitch);

   cIn = (const float *) ((const ubyte *) mapped
                          + draw->vertex_buffer[3].buffer_offset
                          + draw->vertex_element[3].src_offset
                          + elem * draw->vertex_buffer[3].pitch);

   {
      float x = vIn[0];
      float y = vIn[1];
      float z = vIn[2];
      float w = 1.0;

      vOut->clipmask = 0x0;
      vOut->edgeflag = 0;
      /* MVP */
      vOut->clip[0] = m[0] * x + m[4] * y + m[ 8] * z + m[12] * w;
      vOut->clip[1] = m[1] * x + m[5] * y + m[ 9] * z + m[13] * w;
      vOut->clip[2] = m[2] * x + m[6] * y + m[10] * z + m[14] * w;
      vOut->clip[3] = m[3] * x + m[7] * y + m[11] * z + m[15] * w;

      /* divide by w */
      x = vOut->clip[0] / vOut->clip[3];
      y = vOut->clip[1] / vOut->clip[3];
      z = vOut->clip[2] / vOut->clip[3];
      w = 1.0 / vOut->clip[3];

      /* Viewport */
      vOut->data[0][0] = scale[0] * x + trans[0];
      vOut->data[0][1] = scale[1] * y + trans[1];
      vOut->data[0][2] = scale[2] * z + trans[2];
      vOut->data[0][3] = w;

      /* color */
      vOut->data[1][0] = cIn[0];
      vOut->data[1][1] = cIn[1];
      vOut->data[1][2] = cIn[2];
      vOut->data[1][3] = 1.0;
   }
#endif
}


/**
 * Called by the draw module when the vertx cache needs to be flushed.
 * This involves running the vertex shader.
 */
static void vs_flush( struct draw_context *draw )
{
   unsigned i;

   /* We're not really running a vertex shader yet, so flushing the vs
    * queue is just a matter of building the vertices and returning.
    */ 
   /* Actually, I'm cheating even more and pre-building them still
    * with the mesa/vf module.  So it's very easy...
    */
   for (i = 0; i < draw->vs.queue_nr; i++) {
      /* Would do the following steps here:
       *
       * 1) Loop over vertex element descriptors, fetch data from each
       *    to build the pre-tnl vertex.  This might require a new struct
       *    to represent the pre-tnl vertex.
       * 
       * 2) Bundle groups of upto 4 pre-tnl vertices together and pass
       *    to vertex shader.  
       *
       * 3) Do any necessary unswizzling, make sure vertex headers are
       *    correctly populated, store resulting post-transformed
       *    vertices in vcache.
       *
       * In this version, just do the last step:
       */
      const unsigned elt = draw->vs.queue[i].elt;
      struct vertex_header *dest = draw->vs.queue[i].dest;

      run_vertex_program(draw, draw->mapped_vbuffer, elt, dest);
   }
   draw->vs.queue_nr = 0;
}



void
softpipe_draw_arrays(struct pipe_context *pipe, unsigned mode,
                     unsigned start, unsigned count)
{
   struct softpipe_context *sp = softpipe_context(pipe);
   struct draw_context *draw = sp->draw;
   struct pipe_buffer_handle *buf;

   sp_global = sp;

   softpipe_map_surfaces(sp);

   /*
    * Map vertex buffers
    */
   buf = sp->vertex_buffer[0].buffer;
   draw->mapped_vbuffer
      = pipe->winsys->buffer_map(pipe->winsys, buf, PIPE_BUFFER_FLAG_READ);


   /* tell drawing pipeline we're beginning drawing */
   draw->pipeline.first->begin( draw->pipeline.first );

   draw->vs_flush = vs_flush;

   draw_invalidate_vcache( draw );

   draw_set_element_buffer(draw, 0, NULL);  /* no index/element buffer */
   draw_set_prim( draw, mode );

   /* XXX draw_prim_info() and TRIM here */
   draw_prim(draw, start, count);

   /* draw any left-over buffered prims */
   draw_flush(draw);

   /* tell drawing pipeline we're done drawing */
   draw->pipeline.first->end( draw->pipeline.first );

   /*
    * unmap vertex buffer
    */
   pipe->winsys->buffer_unmap(pipe->winsys, buf);

   softpipe_unmap_surfaces(sp);
}



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
draw_set_vertex_attributes2( struct draw_context *draw,
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
			    

