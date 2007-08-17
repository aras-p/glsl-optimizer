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


/** TEMP */
#include "main/context.h"
#include "main/macros.h"

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
 * Transform vertices with the current vertex program/shader
 * Up to four vertices can be shaded at a time.
 * \param vbuffer  the input vertex data
 * \param elts  indexes of four input vertices
 * \param count  number of vertices to shade [1..4]
 * \param vOut  array of pointers to four output vertices
 */
static void
run_vertex_program(struct draw_context *draw,
                   const void *vbuffer, unsigned elts[4], unsigned count,
                   struct vertex_header *vOut[])
{
   struct softpipe_context *sp = softpipe_context(draw->pipe);
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
   tgsi_exec_machine_init(
                          &machine,
                          sp->vs.tokens,
                          PIPE_MAX_SAMPLERS,
                          NULL /*samplers*/ );

   /* Consts does not require 16 byte alignment. */
   machine.Consts = sp->vs.constants->constant;

   machine.Inputs = ALIGN16_ASSIGN(inputs);
   machine.Outputs = ALIGN16_ASSIGN(outputs);

   /* load machine inputs */
   for (j = 0; j < count; j++) {
      unsigned attr;
      for (attr = 0; attr < 16; attr++) {
         if (sp->vs.inputs_read & (1 << attr)) {
            const float *p
                = (const float *) ((const ubyte *) vbuffer
                                   + draw->vertex_buffer[attr].buffer_offset
                                   + draw->vertex_element[attr].src_offset
                                   + elts[j] * draw->vertex_buffer[attr].pitch);

            machine.Inputs[attr].xyzw[0].f[j] = p[0]; /*X*/
            machine.Inputs[attr].xyzw[1].f[j] = p[1]; /*Y*/
            machine.Inputs[attr].xyzw[2].f[j] = p[2]; /*Z*/
            machine.Inputs[attr].xyzw[3].f[j] = 1.0; /*W*/
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
   for (j = 0; j < count; j++) {
      float x, y, z, w;

      x = vOut[j]->clip[0] = outputs[0].xyzw[0].f[j];
      y = vOut[j]->clip[1] = outputs[0].xyzw[1].f[j];
      z = vOut[j]->clip[2] = outputs[0].xyzw[2].f[j];
      w = vOut[j]->clip[3] = outputs[0].xyzw[3].f[j];

      vOut[j]->clipmask = compute_clipmask(x, y, z, w);
      vOut[j]->edgeflag = 0;

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
      vOut[j]->data[1][0] = outputs[1].xyzw[0].f[j];
      vOut[j]->data[1][1] = outputs[1].xyzw[1].f[j];
      vOut[j]->data[1][2] = outputs[1].xyzw[2].f[j];
      vOut[j]->data[1][3] = outputs[1].xyzw[3].f[j];
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
#if 0
static void
run_vertex_program(struct draw_context *draw,
                   const void *vbuffer, unsigned elem,
                   struct vertex_header *vOut)
{
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
}
#endif


/**
 * Called by the draw module when the vertx cache needs to be flushed.
 * This involves running the vertex shader.
 */
static void vs_flush( struct draw_context *draw )
{
   unsigned i, j;

   /* We're not really running a vertex shader yet, so flushing the vs
    * queue is just a matter of building the vertices and returning.
    */ 
   /* Actually, I'm cheating even more and pre-building them still
    * with the mesa/vf module.  So it's very easy...
    */
#if 0
   for (i = 0; i < draw->vs.queue_nr; i++) {
#else
   for (i = 0; i < draw->vs.queue_nr; i+=4) {
#endif
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
#if 0
      const unsigned elt = draw->vs.queue[i].elt;
      struct vertex_header *dest = draw->vs.queue[i].dest;

      run_vertex_program(draw, draw->mapped_vbuffer, elt, dest);
#else
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

      run_vertex_program(draw, draw->mapped_vbuffer, elts, n, dests);
#endif
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
   draw->pipe = pipe;  /* XXX pass pipe to draw_create() */

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
			    

