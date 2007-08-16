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
#include "draw_prim.h"




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



/**
 * Allocate storage for post-transformation vertices.
 */
static void
draw_allocate_vertices( struct draw_context *draw, GLuint nr_vertices )
{
   assert(draw->vertex_size > 0);
   draw->nr_vertices = nr_vertices;
   draw->verts = (GLubyte *) malloc( nr_vertices * draw->vertex_size );
   draw_invalidate_vcache( draw );
}



/**
 * Free storage which was allocated by draw_allocate_vertices()
 */
static void
draw_release_vertices( struct draw_context *draw )
{
   free(draw->verts);
   draw->verts = NULL;
}


/**
 * Note: this must match struct vertex_header's layout (I think).
 */
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



/**
 * This is a hack & will all go away.
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

   draw->vs_flush = vs_flush;

   draw->in_vb = 1;

   /* tell drawing pipeline we're beginning drawing */
   draw->pipeline.first->begin( draw->pipeline.first );

   /* Allocate storage for the post-transform vertices:
    */
   draw_allocate_vertices( draw, VB->Count );

   /* Bind the vb outputs:
    */
   vf_set_sources( draw->vf, VB->AttribPtr, 0 );
   vf_emit_vertices( draw->vf, VB->Count, draw->verts );

   if (VB->Elts) 
      draw_set_element_buffer(draw, sizeof(GLuint), VB->Elts);
   else
      draw_set_element_buffer(draw, 0, NULL);

   for (i = 0; i < VB->PrimitiveCount; i++) {
      const GLenum mode = VB->Primitive[i].mode;
      const GLuint start = VB->Primitive[i].start;
      GLuint length, first, incr;

      /* Trim the primitive down to a legal size.  
       */
      draw_prim_info( mode, &first, &incr );
      length = draw_trim( VB->Primitive[i].count, first, incr );

      if (!length)
	 continue;

      if (draw->prim != mode) 
	 draw_set_prim( draw, mode );

      draw_prim( draw, start, length );
   }

   /* draw any left-over buffered prims */
   draw_flush(draw);

   /* tell drawing pipeline we're done drawing */
   draw->pipeline.first->end( draw->pipeline.first );

   /* free the post-transformed vertices */
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

   draw->vs_flush = vs_flush;

   draw->vertex_size
      = sizeof(struct vertex_header) + numAttrs * 4 * sizeof(GLfloat);


   /* no element/index buffer */
   draw_set_element_buffer(draw, 0, NULL);

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
