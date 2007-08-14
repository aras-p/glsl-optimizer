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

#include "imports.h"
#include "macros.h"

#include "tnl/t_context.h"
#include "vf/vf.h"

#include "draw_private.h"
#include "draw_context.h"


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




/* This file is a temporary set of hooks to allow us to use the tnl/
 * and vf/ modules until we have replacements in pipe.
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
      unsigned elt = draw->vs.queue[i].elt;
      struct vertex_header *dest = draw->vs.queue[i].dest;

      /* Magic:
       */
      memcpy(dest, 
	     draw->verts + elt * draw->vertex_size,
	     draw->vertex_size);
	     
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


static struct vertex_header *get_uint_elt_vertex( struct draw_context *draw,
						    GLuint i )
{
   const GLuint *elts = (const GLuint *)draw->elts;
   return get_vertex( draw, elts[i] );
}

#if 0
static struct vertex_header *get_ushort_elt_vertex( struct draw_context *draw,
						    const void *elts,
						    GLuint i )
{
   const GLushort *elts = (const GLushort *)draw->elts;
   return get_vertex( draw, elts[i] );
}

static struct vertex_header *get_ubyte_elt_vertex( struct draw_context *draw,
						    const void *elts,
						    GLuint i )
{
   const GLubyte *elts = (const GLubyte *)draw->elts;
   return get_vertex( draw, elts[i] );
}
#endif


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


static void draw_allocate_vertices( struct draw_context *draw,
				    GLuint nr_vertices )
{
   draw->nr_vertices = nr_vertices;
   draw->verts = (GLubyte *) malloc( nr_vertices * draw->vertex_size );
   draw_invalidate_vcache( draw );
}



static void draw_release_vertices( struct draw_context *draw )
{
   free(draw->verts);
   draw->verts = NULL;
}


struct header_dword {
   GLuint clipmask:12;
   GLuint edgeflag:1;
   GLuint pad:19;
};


static void 
build_vertex_headers( struct draw_context *draw,
		      struct vertex_buffer *VB )
{
   if (draw->header.storage == NULL) {
      draw->header.stride = sizeof(GLfloat);
      draw->header.size = 1;
      draw->header.storage = ALIGN_MALLOC( VB->Size * sizeof(GLfloat), 32 );
      draw->header.data = draw->header.storage;
      draw->header.count = 0;
      draw->header.flags = VEC_SIZE_1 | VEC_MALLOC;
   }

   /* Build vertex header attribute.
    * 
    */

   {
      GLuint i;
      struct header_dword *header = (struct header_dword *)draw->header.storage;

      /* yes its a hack
       */
      assert(sizeof(*header) == sizeof(GLfloat));

      draw->header.count = VB->Count;

      if (VB->EdgeFlag) {
	 for (i = 0; i < VB->Count; i++) {
	    header[i].clipmask = VB->ClipMask[i];
	    header[i].edgeflag = VB->EdgeFlag[i]; 
	    header[i].pad = 0;
	 }
      }
      else if (VB->ClipOrMask) {
	 for (i = 0; i < VB->Count; i++) {
	    header[i].clipmask = VB->ClipMask[i];
	    header[i].edgeflag = 0; 
	    header[i].pad = 0;
	 }
      }
      else {
	 for (i = 0; i < VB->Count; i++) {
	    header[i].clipmask = 0;
	    header[i].edgeflag = 0; 
	    header[i].pad = 0;
	 }
      }
   }

   VB->AttribPtr[VF_ATTRIB_VERTEX_HEADER] = &draw->header;
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


/* This is a hack & will all go away.
 */
void draw_vb(struct draw_context *draw,
	     struct vertex_buffer *VB )
{
   GLuint i;

   VB->AttribPtr[VF_ATTRIB_POS] = VB->NdcPtr;
   VB->AttribPtr[VF_ATTRIB_BFC0] = VB->ColorPtr[1];
   VB->AttribPtr[VF_ATTRIB_BFC1] = VB->SecondaryColorPtr[1];
   VB->AttribPtr[VF_ATTRIB_CLIP_POS] = VB->ClipPtr;

   /* Build vertex headers: 
    */
   build_vertex_headers( draw, VB );

   draw->in_vb = 1;

   draw->pipeline.first->begin( draw->pipeline.first );

   /* Allocate the vertices:
    */
   draw_allocate_vertices( draw, VB->Count );

   /* Bind the vb outputs:
    */
   vf_set_sources( draw->vf, VB->AttribPtr, 0 );
   vf_emit_vertices( draw->vf, VB->Count, draw->verts );

   draw->elts = VB->Elts;

   if (VB->Elts) 
      draw->get_vertex = get_uint_elt_vertex;
   else
      draw->get_vertex = get_vertex;

   for (i = 0; i < VB->PrimitiveCount; i++) {

      GLenum mode = VB->Primitive[i].mode;
      GLuint start = VB->Primitive[i].start;
      GLuint length, first, incr;

      /* Trim the primitive down to a legal size.  
       */
      draw_prim_info( mode, &first, &incr );
      length = trim( VB->Primitive[i].count, first, incr );

      if (!length)
	 continue;

      if (draw->prim != mode) 
	 draw_set_prim( draw, mode );

      draw_prim( draw, start, length );
   }

   draw_flush(draw);
   draw->pipeline.first->end( draw->pipeline.first );
   draw_release_vertices( draw );
   draw->verts = NULL;
   draw->in_vb = 0;
   draw->elts = NULL;
}


/**
 * XXX Temporary mechanism to draw simple vertex arrays.
 * All attribs are GLfloat[4].  Arrays are interleaved, in GL-speak.
 */
void
draw_vertices(struct draw_context *draw,
              GLuint mode,
              GLuint numVerts, const GLfloat *vertices,
              GLuint numAttrs, const GLuint attribs[])
{
   /*GLuint first, incr;*/
   GLuint i, j;

   assert(mode <= GL_POLYGON);

   draw->get_vertex = get_vertex;
   draw->vertex_size
      = sizeof(struct vertex_header) + numAttrs * 4 * sizeof(GLfloat);




   /*draw_prim_info(mode, &first, &incr);*/
   draw_allocate_vertices( draw, numVerts );
   draw->pipeline.first->begin( draw->pipeline.first );

   if (draw->prim != mode) 
      draw_set_prim( draw, mode );

   /* setup attr info */
   draw->nr_attrs = numAttrs + 2;
   draw->attrs[0].attrib = VF_ATTRIB_VERTEX_HEADER;
   draw->attrs[0].format = EMIT_1F;
   draw->attrs[1].attrib = VF_ATTRIB_CLIP_POS;
   draw->attrs[1].format = EMIT_4F;
   for (j = 0; j < numAttrs; j++) {
      draw->vf_attr_to_slot[attribs[j]] = 2+j;
      draw->attrs[2+j].attrib = attribs[j];
      draw->attrs[2+j].format = EMIT_4F;
   }

   /* build vertices */
   for (i = 0; i < numVerts; i++) {
      struct vertex_header *v
         = (struct vertex_header *) (draw->verts + i * draw->vertex_size);
      v->clipmask = 0x0;
      v->edgeflag = 0;
      for (j = 0; j < numAttrs; j++) {
         COPY_4FV(v->data[j], vertices + (i * numAttrs + j) * 4);
      }
   }

   /* draw */
   draw_prim(draw, 0, numVerts);
   draw_flush(draw);
   draw->pipeline.first->end( draw->pipeline.first );


   /* clean up */
   draw_release_vertices( draw );
   draw->verts = NULL;
   draw->in_vb = 0;
}



/**
 * Accumulate another attribute's info.
 * Note the "- 2" factor here.  We need this because the vertex->data[]
 * array does not include the first two attributes we emit (VERTEX_HEADER
 * and CLIP_POS).  So, the 3rd attribute actually winds up in the 1st
 * position of the data[] array.
 */
#define EMIT_ATTR( VF_ATTR, STYLE )				\
do {								\
   if (draw->nr_attrs >= 2)					\
      draw->vf_attr_to_slot[VF_ATTR] = draw->nr_attrs - 2;	\
   draw->attrs[draw->nr_attrs].attrib = VF_ATTR;		\
   draw->attrs[draw->nr_attrs].format = STYLE;			\
   draw->nr_attrs++;						\
} while (0)


/**
 * Tell the draw module about the layout of attributes in the vertex.
 * We need this in order to know which vertex slot has color0, etc.
 *
 * \param slot_to_vf_attr  an array which maps slot indexes to vertex
 *                         format tokens (VF_*).
 * \param nr_attrs  the size of the slot_to_vf_attr array
 *                  (and number of attributes)
 */
void draw_set_vertex_attributes( struct draw_context *draw,
				 const GLuint *slot_to_vf_attr,
				 GLuint nr_attrs )
{
   GLuint i;

   memset(draw->vf_attr_to_slot, 0, sizeof(draw->vf_attr_to_slot));
   draw->nr_attrs = 0;

   /*
    * First three attribs are always the same: header, clip pos, winpos
    */
   EMIT_ATTR(VF_ATTRIB_VERTEX_HEADER, EMIT_1F);
   EMIT_ATTR(VF_ATTRIB_CLIP_POS, EMIT_4F);

   assert(slot_to_vf_attr[0] == VF_ATTRIB_POS);
   EMIT_ATTR(slot_to_vf_attr[0], EMIT_4F_VIEWPORT);

   /*
    * Remaining attribs (color, texcoords, etc)
    */
   for (i = 1; i < nr_attrs; i++) 
      EMIT_ATTR(slot_to_vf_attr[i], EMIT_4F);

   /* tell the vertex format module how to construct vertices for us */
   draw->vertex_size = vf_set_vertex_attributes( draw->vf, draw->attrs,
                                                 draw->nr_attrs, 0 );
}
			    

void draw_alloc_tmps( struct draw_stage *stage, GLuint nr )
{
   stage->nr_tmps = nr;

   if (nr) {
      GLubyte *store = MALLOC(MAX_VERTEX_SIZE * nr);
      GLuint i;

      stage->tmp = MALLOC(sizeof(struct vertex_header *) * nr);
      
      for (i = 0; i < nr; i++)
	 stage->tmp[i] = (struct vertex_header *)(store + i * MAX_VERTEX_SIZE);
   }
}

void draw_free_tmps( struct draw_stage *stage )
{
   if (stage->tmp) {
      FREE(stage->tmp[0]);
      FREE(stage->tmp);
   }
}
