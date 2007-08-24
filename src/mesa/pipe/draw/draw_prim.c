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
  */

#include "pipe/p_util.h"
#include "draw_private.h"
#include "draw_context.h"
#include "draw_prim.h"

#include "pipe/tgsi/exec/tgsi_core.h"


#define RP_NONE  0
#define RP_POINT 1
#define RP_LINE  2
#define RP_TRI   3


static unsigned reduced_prim[PIPE_PRIM_POLYGON + 1] = {
   RP_POINT,
   RP_LINE,
   RP_LINE,
   RP_LINE,
   RP_TRI,
   RP_TRI,
   RP_TRI,
   RP_TRI,
   RP_TRI,
   RP_TRI
};


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

#if !defined(XSTDCALL) 
#if defined(WIN32)
#define XSTDCALL __stdcall
#else
#define XSTDCALL
#endif
#endif

#if defined(USE_X86_ASM) || defined(SLANG_X86)
typedef void (XSTDCALL *sse2_function)(
   const struct tgsi_exec_vector *input,
   struct tgsi_exec_vector *output,
   float (*constant)[4],
   struct tgsi_exec_vector *temporary );
#endif

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

#if 0
   FILE *file = stdout;
#endif

   ALIGN16_DECL(struct tgsi_exec_vector, inputs, PIPE_ATTRIB_MAX);
   ALIGN16_DECL(struct tgsi_exec_vector, outputs, PIPE_ATTRIB_MAX);
   const float *scale = draw->viewport.scale;
   const float *trans = draw->viewport.translate;

   assert(count <= 4);
   assert(draw->vertex_shader.outputs_written & (1 << TGSI_ATTRIB_POS));

#if 0
   if( file == NULL ) {
      file = fopen( "vs-exec.txt", "wt" );
   }
#endif

#ifdef DEBUG
   memset( &machine, 0, sizeof( machine ) );
#endif

   /* init machine state */
   tgsi_exec_machine_init(&machine,
                          draw->vertex_shader.tokens,
                          PIPE_MAX_SAMPLERS,
                          NULL /*samplers*/ );

   /* Consts does not require 16 byte alignment. */
   machine.Consts = (float (*)[4]) draw->mapped_constants;

   machine.Inputs = ALIGN16_ASSIGN(inputs);
   machine.Outputs = ALIGN16_ASSIGN(outputs);


#if 0
   {
      unsigned attr;
      for (attr = 0; attr < 16; attr++) {
         if (draw->vertex_shader.inputs_read & (1 << attr)) {
            unsigned buf = draw->vertex_element[attr].vertex_buffer_index;
            fprintf(file, "attr %d: buf_off %d  src_off %d  pitch %d\n",
                   attr, 
                   draw->vertex_buffer[buf].buffer_offset,
                   draw->vertex_element[attr].src_offset,
                   draw->vertex_buffer[buf].pitch);
         }
      }
   }
#endif

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
            fprintf(file, "Input vertex %d: attr %d:  %f %f %f %f\n",
                    j, attr, p[0], p[1], p[2], p[3]);
            fflush( file );
#endif
         }
      }
   }

#if 0
   printf("Vertex shader Constants:\n");
   {
      int i;
      for (i = 0; i < 4; i++) {
         printf(" %d: %f %f %f %f\n", i,
                machine.Consts[i][0],
                machine.Consts[i][1],
                machine.Consts[i][2],
                machine.Consts[i][3]);
      }
   }
#endif

   /* run shader */
   if( draw->vertex_shader.executable != NULL ) {
#if defined(USE_X86_ASM) || defined(SLANG_X86)
      sse2_function func = (sse2_function) draw->vertex_shader.executable;
      func(
         machine.Inputs,
         machine.Outputs,
         machine.Consts,
         machine.Temps );
#else
      assert( 0 );
#endif
   }
   else {
      tgsi_exec_machine_run( &machine );
   }

#if 0
   for (i = 0; i < 4; i++) {
   fprintf(file, "VS result: %f %f %f %f\n",
          machine.Outputs[0].xyzw[0].f[i],
          machine.Outputs[0].xyzw[1].f[i],
          machine.Outputs[0].xyzw[2].f[i],
          machine.Outputs[0].xyzw[3].f[i]);
   }
   fflush( file );
#endif

   /* store machine results */
   for (j = 0; j < count; j++) {
      unsigned slot;
      float x, y, z, w;

      /* Handle attr[0] (position) specially: */
      x = vOut[j]->clip[0] = machine.Outputs[0].xyzw[0].f[j];
      y = vOut[j]->clip[1] = machine.Outputs[0].xyzw[1].f[j];
      z = vOut[j]->clip[2] = machine.Outputs[0].xyzw[2].f[j];
      w = vOut[j]->clip[3] = machine.Outputs[0].xyzw[3].f[j];

      vOut[j]->clipmask = compute_clipmask(x, y, z, w) | draw->user_clipmask;
      vOut[j]->edgeflag = 1;

      /* divide by w */
      w = 1.0f / w;
      x *= w;
      y *= w;
      z *= w;

      /* Viewport mapping */
      vOut[j]->data[0][0] = x * scale[0] + trans[0];
      vOut[j]->data[0][1] = y * scale[1] + trans[1];
      vOut[j]->data[0][2] = z * scale[2] + trans[2];
      vOut[j]->data[0][3] = w;
#if 0
      fprintf(file, "Vert %d: wincoord: %f %f %f %f\n", j,
              vOut[j]->data[0][0],
              vOut[j]->data[0][1],
              vOut[j]->data[0][2],
              vOut[j]->data[0][3]);
      fflush( file );
#endif

      /* remaining attributes are packed into sequential post-transform
       * vertex attrib slots.
       */
      for (slot = 1; slot < draw->vertex_info.num_attribs; slot++) {
         vOut[j]->data[slot][0] = machine.Outputs[slot].xyzw[0].f[j];
         vOut[j]->data[slot][1] = machine.Outputs[slot].xyzw[1].f[j];
         vOut[j]->data[slot][2] = machine.Outputs[slot].xyzw[2].f[j];
         vOut[j]->data[slot][3] = machine.Outputs[slot].xyzw[3].f[j];
#if 0
         fprintf(file, "output attrib slot %d: %f %f %f %f  vert %p\n",
                 slot,
                 vOut[j]->data[slot][0],
                 vOut[j]->data[slot][1],
                 vOut[j]->data[slot][2],
                 vOut[j]->data[slot][3], vOut[j]);
#endif
      }
   } /* loop over vertices */
}


/**
 * Called by the draw module when the vertx cache needs to be flushed.
 * This involves running the vertex shader.
 */
static void transform_vertices( struct draw_context *draw )
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


void draw_flush( struct draw_context *draw )
{
   struct draw_stage *first = draw->pipeline.first;
   unsigned i;

   /* Make sure all vertices are available:
    */
   transform_vertices(draw);

   switch (draw->reduced_prim) {
   case RP_TRI:
      for (i = 0; i < draw->pq.queue_nr; i++) {
	 if (draw->pq.queue[i].reset_line_stipple)
	    first->reset_stipple_counter( first );

	 first->tri( first, &draw->pq.queue[i] );
      }
      break;
   case RP_LINE:
      for (i = 0; i < draw->pq.queue_nr; i++) {
	 if (draw->pq.queue[i].reset_line_stipple)
	    first->reset_stipple_counter( first );

	 first->line( first, &draw->pq.queue[i] );
      }
      break;
   case RP_POINT:
      first->reset_stipple_counter( first );
      for (i = 0; i < draw->pq.queue_nr; i++)
	 first->point( first, &draw->pq.queue[i] );
      break;
   }

   draw->pq.queue_nr = 0;
   draw->vcache.referenced = 0;
   draw->vcache.overflow = 0;
}


void draw_invalidate_vcache( struct draw_context *draw )
{
   unsigned i;

   assert(draw->pq.queue_nr == 0);
   assert(draw->vs.queue_nr == 0);
   assert(draw->vcache.referenced == 0);
   
   for (i = 0; i < Elements( draw->vcache.idx ); i++)
      draw->vcache.idx[i] = ~0;
}


/* Return a pointer to a freshly queued primitive header.  Ensure that
 * there is room in the vertex cache for a maximum of "nr_verts" new
 * vertices.  Flush primitive and/or vertex queues if necessary to
 * make space.
 */
static struct prim_header *get_queued_prim( struct draw_context *draw,
					    unsigned nr_verts )
{
   if (draw->pq.queue_nr + 1 >= PRIM_QUEUE_LENGTH ||
       draw->vcache.overflow + nr_verts >= VCACHE_OVERFLOW) 
      draw_flush( draw );

   /* The vs queue is sized so that this can never happen:
    */
   assert(draw->vs.queue_nr + nr_verts < VS_QUEUE_LENGTH);

   return &draw->pq.queue[draw->pq.queue_nr++];
}


/* Check if vertex is in cache, otherwise add it.  It won't go through
 * VS yet, not until there is a flush operation or the VS queue fills up.  
 */
static struct vertex_header *get_vertex( struct draw_context *draw,
					 unsigned i )
{
   unsigned slot = (i + (i>>5)) & 31;
   
   /* Cache miss?
    */
   if (draw->vcache.idx[slot] != i) {

      /* If slot is in use, use the overflow area:
       */
      if (draw->vcache.referenced & (1 << slot))
	 slot = VCACHE_SIZE + draw->vcache.overflow++;
      else
         draw->vcache.referenced |= (1 << slot);  /* slot now in use */

      draw->vcache.idx[slot] = i;

      /* Add to vertex shader queue:
       */
      draw->vs.queue[draw->vs.queue_nr].dest = draw->vcache.vertex[slot];
      draw->vs.queue[draw->vs.queue_nr].elt = i;
      draw->vs.queue_nr++;

      /* Need to set the vertex's edge flag here.  If we're being called
       * by do_ef_triangle(), that function needs edge flag info!
       */
      draw->vcache.vertex[slot]->edgeflag = 1; /*XXX use user's edge flag! */
   }

   return draw->vcache.vertex[slot];
}


static struct vertex_header *get_uint_elt_vertex( struct draw_context *draw,
                                                  unsigned i )
{
   const unsigned *elts = (const unsigned *) draw->mapped_elts;
   return get_vertex( draw, elts[i] );
}


static struct vertex_header *get_ushort_elt_vertex( struct draw_context *draw,
						    unsigned i )
{
   const ushort *elts = (const ushort *) draw->mapped_elts;
   return get_vertex( draw, elts[i] );
}


static struct vertex_header *get_ubyte_elt_vertex( struct draw_context *draw,
                                                   unsigned i )
{
   const ubyte *elts = (const ubyte *) draw->mapped_elts;
   return get_vertex( draw, elts[i] );
}


static void do_point( struct draw_context *draw,
		      unsigned i0 )
{
   struct prim_header *prim = get_queued_prim( draw, 1 );
   
   prim->reset_line_stipple = 0;
   prim->edgeflags = 1;
   prim->pad = 0;
   prim->v[0] = draw->get_vertex( draw, i0 );
}


static void do_line( struct draw_context *draw,
		     boolean reset_stipple,
		     unsigned i0,
		     unsigned i1 )
{
   struct prim_header *prim = get_queued_prim( draw, 2 );
   
   prim->reset_line_stipple = reset_stipple;
   prim->edgeflags = 1;
   prim->pad = 0;
   prim->v[0] = draw->get_vertex( draw, i0 );
   prim->v[1] = draw->get_vertex( draw, i1 );
}

static void do_triangle( struct draw_context *draw,
			 unsigned i0,
			 unsigned i1,
			 unsigned i2 )
{
   struct prim_header *prim = get_queued_prim( draw, 3 );
   
   prim->reset_line_stipple = 1;
   prim->edgeflags = ~0;
   prim->pad = 0;
   prim->v[0] = draw->get_vertex( draw, i0 );
   prim->v[1] = draw->get_vertex( draw, i1 );
   prim->v[2] = draw->get_vertex( draw, i2 );
}
			  
static void do_ef_triangle( struct draw_context *draw,
			    boolean reset_stipple,
			    unsigned ef_mask,
			    unsigned i0,
			    unsigned i1,
			    unsigned i2 )
{
   struct prim_header *prim = get_queued_prim( draw, 3 );
   struct vertex_header *v0 = draw->get_vertex( draw, i0 );
   struct vertex_header *v1 = draw->get_vertex( draw, i1 );
   struct vertex_header *v2 = draw->get_vertex( draw, i2 );
   
   prim->reset_line_stipple = reset_stipple;

   prim->edgeflags = ef_mask & ((v0->edgeflag << 0) | 
				(v1->edgeflag << 1) | 
				(v2->edgeflag << 2));
   prim->pad = 0;
   prim->v[0] = v0;
   prim->v[1] = v1;
   prim->v[2] = v2;
}


static void do_quad( struct draw_context *draw,
		     unsigned v0,
		     unsigned v1,
		     unsigned v2,
		     unsigned v3 )
{
   const unsigned omitEdge2 = ~(1 << 1);
   const unsigned omitEdge3 = ~(1 << 2);
   do_ef_triangle( draw, 1, omitEdge2, v0, v1, v3 );
   do_ef_triangle( draw, 0, omitEdge3, v1, v2, v3 );
}


/**
 * Main entrypoint to draw some number of points/lines/triangles
 */
void
draw_prim( struct draw_context *draw, unsigned start, unsigned count )
{
   unsigned i;

//   _mesa_printf("%s (%d) %d/%d\n", __FUNCTION__, draw->prim, start, count );

   switch (draw->prim) {
   case PIPE_PRIM_POINTS:
      for (i = 0; i < count; i ++) {
	 do_point( draw,
		   start + i );
      }
      break;

   case PIPE_PRIM_LINES:
      for (i = 0; i+1 < count; i += 2) {
	 do_line( draw, 
		  TRUE,
		  start + i + 0,
		  start + i + 1);
      }
      break;

   case PIPE_PRIM_LINE_LOOP:  
      if (count >= 2) {
	 for (i = 1; i < count; i++) {
	    do_line( draw, 
		     i == 1, 	/* XXX: only if vb not split */
		     start + i - 1,
		     start + i );
	 }

	 do_line( draw, 
		  0,
		  start + count - 1,
		  start + 0 );
      }
      break;

   case PIPE_PRIM_LINE_STRIP:
      if (count >= 2) {
	 for (i = 1; i < count; i++) {
	    do_line( draw,
		     i == 1,
		     start + i - 1,
		     start + i );
	 }
      }
      break;

   case PIPE_PRIM_TRIANGLES:
      for (i = 0; i+2 < count; i += 3) {
	 do_ef_triangle( draw,
			 1, 
			 ~0,
			 start + i + 0,
			 start + i + 1,
			 start + i + 2 );
      }
      break;

   case PIPE_PRIM_TRIANGLE_STRIP:
      for (i = 0; i+2 < count; i++) {
	 if (i & 1) {
	    do_triangle( draw,
			 start + i + 1,
			 start + i + 0,
			 start + i + 2 );
	 }
	 else {
	    do_triangle( draw,
			 start + i + 0,
			 start + i + 1,
			 start + i + 2 );
	 }
      }
      break;

   case PIPE_PRIM_TRIANGLE_FAN:
      if (count >= 3) {
	 for (i = 0; i+2 < count; i++) {
	    do_triangle( draw,
			 start + 0,
			 start + i + 1,
			 start + i + 2 );
	 }
      }
      break;


   case PIPE_PRIM_QUADS:
      for (i = 0; i+3 < count; i += 4) {
	 do_quad( draw,
		  start + i + 0,
		  start + i + 1,
		  start + i + 2,
		  start + i + 3);
      }
      break;

   case PIPE_PRIM_QUAD_STRIP:
      for (i = 0; i+3 < count; i += 2) {
	 do_quad( draw,
		  start + i + 2,
		  start + i + 0,
		  start + i + 1,
		  start + i + 3);
      }
      break;

   case PIPE_PRIM_POLYGON:
      if (count >= 3) {
	 unsigned ef_mask = (1<<2) | (1<<0);

	 for (i = 0; i+2 < count; i++) {

            if (i + 3 >= count)
	       ef_mask |= (1<<1);

	    do_ef_triangle( draw,
			    i == 0,
			    ef_mask,
			    start + i + 1,
			    start + i + 2,
			    start + 0);

	    ef_mask &= ~(1<<2);
	 }
      }
      break;

   default:
      assert(0);
      break;
   }
}


void
draw_set_prim( struct draw_context *draw, unsigned prim )
{
   assert(prim >= PIPE_PRIM_POINTS);
   assert(prim <= PIPE_PRIM_POLYGON);

   if (reduced_prim[prim] != draw->reduced_prim) {
      draw_flush( draw );
      draw->reduced_prim = reduced_prim[prim];
   }

   draw->prim = prim;
}


/**
 * Tell the drawing context about the index/element buffer to use
 * (ala glDrawElements)
 * If no element buffer is to be used (i.e. glDrawArrays) then this
 * should be called with eltSize=0 and elements=NULL.
 *
 * \param draw  the drawing context
 * \param eltSize  size of each element (1, 2 or 4 bytes)
 * \param elements  the element buffer ptr
 */
void
draw_set_mapped_element_buffer( struct draw_context *draw,
                                unsigned eltSize, void *elements )
{
   /* choose the get_vertex() function to use */
   switch (eltSize) {
   case 0:
      draw->get_vertex = get_vertex;
      break;
   case 1:
      draw->get_vertex = get_ubyte_elt_vertex;
      break;
   case 2:
      draw->get_vertex = get_ushort_elt_vertex;
      break;
   case 4:
      draw->get_vertex = get_uint_elt_vertex;
      break;
   default:
      assert(0);
   }
   draw->mapped_elts = elements;
   draw->eltSize = eltSize;
}


/**
 * Tell drawing context where to find mapped vertex buffers.
 */
void draw_set_mapped_vertex_buffer(struct draw_context *draw,
                                   unsigned attr, const void *buffer)
{
   draw->mapped_vbuffer[attr] = buffer;
}


void draw_set_mapped_constant_buffer(struct draw_context *draw,
                                     const void *buffer)
{
   draw->mapped_constants = buffer;
}


unsigned
draw_prim_info(unsigned prim, unsigned *first, unsigned *incr)
{
   assert(prim >= PIPE_PRIM_POINTS);
   assert(prim <= PIPE_PRIM_POLYGON);

   switch (prim) {
   case PIPE_PRIM_POINTS:
      *first = 1;
      *incr = 1;
      return 0;
   case PIPE_PRIM_LINES:
      *first = 2;
      *incr = 2;
      return 0;
   case PIPE_PRIM_LINE_STRIP:
      *first = 2;
      *incr = 1;
      return 0;
   case PIPE_PRIM_LINE_LOOP:
      *first = 2;
      *incr = 1;
      return 1;
   case PIPE_PRIM_TRIANGLES:
      *first = 3;
      *incr = 3;
      return 0;
   case PIPE_PRIM_TRIANGLE_STRIP:
      *first = 3;
      *incr = 1;
      return 0;
   case PIPE_PRIM_TRIANGLE_FAN:
   case PIPE_PRIM_POLYGON:
      *first = 3;
      *incr = 1;
      return 1;
   case PIPE_PRIM_QUADS:
      *first = 4;
      *incr = 4;
      return 0;
   case PIPE_PRIM_QUAD_STRIP:
      *first = 4;
      *incr = 2;
      return 0;
   default:
      assert(0);
      *first = 1;
      *incr = 1;
      return 0;
   }
}


unsigned
draw_trim( unsigned count, unsigned first, unsigned incr )
{
   if (count < first)
      return 0;
   else
      return count - (count - first) % incr; 
}


/**
 * Allocate space for temporary post-transform vertices, such as for clipping.
 */
void draw_alloc_tmps( struct draw_stage *stage, unsigned nr )
{
   stage->nr_tmps = nr;

   if (nr) {
      ubyte *store = (ubyte *) malloc(MAX_VERTEX_SIZE * nr);
      unsigned i;

      stage->tmp = (struct vertex_header **) malloc(sizeof(struct vertex_header *) * nr);
      
      for (i = 0; i < nr; i++)
	 stage->tmp[i] = (struct vertex_header *)(store + i * MAX_VERTEX_SIZE);
   }
}

void draw_free_tmps( struct draw_stage *stage )
{
   if (stage->tmp) {
      free(stage->tmp[0]);
      free(stage->tmp);
   }
}
