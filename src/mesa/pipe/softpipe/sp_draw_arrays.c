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


#include "main/mtypes.h"
#include "main/context.h"

#include "pipe/p_defines.h"
#include "pipe/p_context.h"
#include "pipe/p_winsys.h"

#include "sp_context.h"
#include "sp_state.h"

#include "pipe/draw/draw_private.h"
#include "pipe/draw/draw_context.h"



#define RP_NONE  0
#define RP_POINT 1
#define RP_LINE  2
#define RP_TRI   3

static unsigned reduced_prim[GL_POLYGON + 1] = {
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
                   const void *vbuffer, GLuint elem,
                   struct vertex_header *vOut)
{
   const float *vIn, *cIn;
   const GLfloat *scale = draw->viewport.scale;
   const GLfloat *trans = draw->viewport.translate;
   const void *mapped = vbuffer;

   GET_CURRENT_CONTEXT(ctx);
   const GLfloat *m = ctx->_ModelProjectMatrix.m;

   vIn = (const float *) ((const GLubyte *) mapped
                          + draw->vertex_buffer[0].buffer_offset
                          + draw->vertex_element[0].src_offset
                          + elem * draw->vertex_buffer[0].pitch);

   cIn = (const float *) ((const GLubyte *) mapped
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


static void draw_flush( struct draw_context *draw )
{
   struct draw_stage *first = draw->pipeline.first;
   unsigned i;

   /* Make sure all vertices are available:
    */
   vs_flush( draw );


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


static void draw_invalidate_vcache( struct draw_context *draw )
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
					    GLuint nr_verts )
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
					 GLuint i )
{
   unsigned slot = (i + (i>>5)) & 31;
   
   /* Cache miss?
    */
   if (draw->vcache.idx[slot] != i) {

      /* If slot is in use, use the overflow area:
       */
      if (draw->vcache.referenced & (1<<slot)) 
	 slot = draw->vcache.overflow++;

      draw->vcache.idx[slot] = i;

      /* Add to vertex shader queue:
       */
      draw->vs.queue[draw->vs.queue_nr].dest = draw->vcache.vertex[slot];
      draw->vs.queue[draw->vs.queue_nr].elt = i;
      draw->vs.queue_nr++;
   }

   /* Mark slot as in-use:
    */
   draw->vcache.referenced |= (1<<slot);
   return draw->vcache.vertex[slot];
}



static void draw_set_prim( struct draw_context *draw,
			   GLenum prim )
{
   if (reduced_prim[prim] != draw->reduced_prim) {
      draw_flush( draw );
      draw->reduced_prim = reduced_prim[prim];
   }

   draw->prim = prim;
}


static void do_point( struct draw_context *draw,
		      GLuint i0 )
{
   struct prim_header *prim = get_queued_prim( draw, 1 );
   
   prim->reset_line_stipple = 0;
   prim->edgeflags = 1;
   prim->pad = 0;
   prim->v[0] = draw->get_vertex( draw, i0 );
}


static void do_line( struct draw_context *draw,
		     GLboolean reset_stipple,
		     GLuint i0,
		     GLuint i1 )
{
   struct prim_header *prim = get_queued_prim( draw, 2 );
   
   prim->reset_line_stipple = reset_stipple;
   prim->edgeflags = 1;
   prim->pad = 0;
   prim->v[0] = draw->get_vertex( draw, i0 );
   prim->v[1] = draw->get_vertex( draw, i1 );
}

static void do_triangle( struct draw_context *draw,
			 GLuint i0,
			 GLuint i1,
			 GLuint i2 )
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
			    GLboolean reset_stipple,
			    GLuint ef_mask,
			    GLuint i0,
			    GLuint i1,
			    GLuint i2 )
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
   do_ef_triangle( draw, 1, ~(1<<0), v0, v1, v3 );
   do_ef_triangle( draw, 0, ~(1<<1), v1, v2, v3 );
}


static void draw_prim( struct draw_context *draw,
		       GLuint start,
		       GLuint count )
{
   GLuint i;

//   _mesa_printf("%s (%d) %d/%d\n", __FUNCTION__, draw->prim, start, count );

   switch (draw->prim) {
   case GL_POINTS:
      for (i = 0; i < count; i ++) {
	 do_point( draw,
		   start + i );
      }
      break;

   case GL_LINES:
      for (i = 0; i+1 < count; i += 2) {
	 do_line( draw, 
		  TRUE,
		  start + i + 0,
		  start + i + 1);
      }
      break;

   case GL_LINE_LOOP:  
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

   case GL_LINE_STRIP:
      if (count >= 2) {
	 for (i = 1; i < count; i++) {
	    do_line( draw,
		     i == 1,
		     start + i - 1,
		     start + i );
	 }
      }
      break;

   case GL_TRIANGLES:
      for (i = 0; i+2 < count; i += 3) {
	 do_ef_triangle( draw,
			 1, 
			 ~0,
			 start + i + 0,
			 start + i + 1,
			 start + i + 2 );
      }
      break;

   case GL_TRIANGLE_STRIP:
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

   case GL_TRIANGLE_FAN:
      if (count >= 3) {
	 for (i = 0; i+2 < count; i++) {
	    do_triangle( draw,
			 start + 0,
			 start + i + 1,
			 start + i + 2 );
	 }
      }
      break;


   case GL_QUADS:
      for (i = 0; i+3 < count; i += 4) {
	 do_quad( draw,
		  start + i + 0,
		  start + i + 1,
		  start + i + 2,
		  start + i + 3);
      }
      break;

   case GL_QUAD_STRIP:
      for (i = 0; i+3 < count; i += 2) {
	 do_quad( draw,
		  start + i + 2,
		  start + i + 0,
		  start + i + 1,
		  start + i + 3);
      }
      break;

   case GL_POLYGON:
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
			    start + i + 0);

	    ef_mask &= ~(1<<2);
	 }
      }
      break;

   default:
      assert(0);
      break;
   }
}



static GLuint draw_prim_info(GLenum mode, GLuint *first, GLuint *incr)
{
   switch (mode) {
   case GL_POINTS:
      *first = 1;
      *incr = 1;
      return 0;
   case GL_LINES:
      *first = 2;
      *incr = 2;
      return 0;
   case GL_LINE_STRIP:
      *first = 2;
      *incr = 1;
      return 0;
   case GL_LINE_LOOP:
      *first = 2;
      *incr = 1;
      return 1;
   case GL_TRIANGLES:
      *first = 3;
      *incr = 3;
      return 0;
   case GL_TRIANGLE_STRIP:
      *first = 3;
      *incr = 1;
      return 0;
   case GL_TRIANGLE_FAN:
   case GL_POLYGON:
      *first = 3;
      *incr = 1;
      return 1;
   case GL_QUADS:
      *first = 4;
      *incr = 4;
      return 0;
   case GL_QUAD_STRIP:
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


static GLuint trim( GLuint count, GLuint first, GLuint incr )
{
   if (count < first)
      return 0;
   else
      return count - (count - first) % incr; 
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

   draw_invalidate_vcache( draw );

#if 0
   if (VB->Elts) 
      draw->get_vertex = get_uint_elt_vertex;
   else
#endif
      draw->get_vertex = get_vertex;

   draw_set_prim( draw, mode );

   /* XXX draw_prim_info() and TRIM here */
   draw_prim(draw, start, count);

   /* draw any left-over buffered prims */
   draw_flush(draw);

   /* tell drawing pipeline we're done drawing */
   draw->pipeline.first->end( draw->pipeline.first );

#if 0
   draw->verts = NULL;
   draw->in_vb = 0;
   draw->elts = NULL;
#endif

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


void draw_set_vertex_attributes2( struct draw_context *draw,
				 const GLuint *slot_to_vf_attr,
				 GLuint nr_attrs )
{
   GLuint i;

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

#if 0
   /* tell the vertex format module how to construct vertices for us */
   draw->vertex_size = vf_set_vertex_attributes( draw->vf, draw->attrs,
                                                 draw->nr_attrs, 0 );
#endif

   draw->vertex_size *= 4; /* floats to bytes */
}
			    

