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


/* This file is a temporary set of hooks to allow us to use the tnl/
 * and vf/ modules until we have replacements in pipe.
 */


static struct vertex_header *get_vertex( struct draw_context *pipe,
					       GLuint i )
{
   return (struct vertex_header *)(pipe->verts + i * pipe->vertex_size);
}



static void draw_allocate_vertices( struct draw_context *draw,
				    GLuint nr_vertices )
{
   draw->nr_vertices = nr_vertices;
   draw->verts = (GLubyte *) malloc( nr_vertices * draw->vertex_size );

   draw->pipeline.first->begin( draw->pipeline.first );
}

static void draw_set_prim( struct draw_context *draw,
			   GLenum prim )
{
   draw->prim = prim;

   /* Not done yet - need to force edgeflags to 1 in strip/fan
    * primitives.
    */
#if 0
   switch (prim) {
   case GL_TRIANGLES:
   case GL_POLYGON:
   case GL_QUADS:
   case GL_QUAD_STRIP:		/* yes, we need this */
      respect_edgeflags( pipe, GL_TRUE );
      break;

   default:
      respect_edgeflags( pipe, GL_FALSE );
      break;
   }
#endif
}
			  


static void do_quad( struct draw_stage *first,
		     struct vertex_header *v0,
		     struct vertex_header *v1,
		     struct vertex_header *v2,
		     struct vertex_header *v3 )
{
   struct prim_header prim;

   {
      GLuint tmp = v1->edgeflag;
      v1->edgeflag = 0;

      prim.v[0] = v0;
      prim.v[1] = v1;
      prim.v[2] = v3;
      first->tri( first, &prim );

      v1->edgeflag = tmp;
   }

   {
      GLuint tmp = v3->edgeflag;
      v3->edgeflag = 0;

      prim.v[0] = v1;
      prim.v[1] = v2;
      prim.v[2] = v3;
      first->tri( first, &prim );

      v3->edgeflag = tmp;
   }
}




static void draw_indexed_prim( struct draw_context *draw,
			       const GLuint *elts,
			       GLuint count )
{
   struct draw_stage * const first = draw->pipeline.first;
   struct prim_header prim;
   GLuint i;

   prim.det = 0;		/* valid from cull stage onwards */
   prim.v[0] = 0;
   prim.v[1] = 0;
   prim.v[2] = 0;

   switch (draw->prim) {
   case GL_POINTS:
      for (i = 0; i < count; i ++) {
	 prim.v[0] = get_vertex( draw, elts[i] );

	 first->point( first, &prim );
      }
      break;

   case GL_LINES:
      for (i = 0; i+1 < count; i += 2) {
	 prim.v[0] = get_vertex( draw, elts[i + 0] );
	 prim.v[1] = get_vertex( draw, elts[i + 1] );
      
         first->reset_stipple_counter( first );
	 first->line( first, &prim );
      }
      break;

   case GL_LINE_LOOP:  
      if (count >= 2) {
         first->reset_stipple_counter( first );
	 for (i = 1; i < count; i++) {
	    prim.v[0] = get_vertex( draw, elts[i-1] );
	    prim.v[1] = get_vertex( draw, elts[i] );	    
	    first->line( first, &prim );
	 }

	 prim.v[0] = get_vertex( draw, elts[count-1] );
	 prim.v[1] = get_vertex( draw, elts[0] );	    
	 first->line( first, &prim );
      }
      break;

   case GL_LINE_STRIP:
      /* I'm guessing it will be necessary to have something like a
       * render->reset_line_stipple() method to properly support
       * splitting strips into primitives like this.  Alternately we
       * could just scan ahead to find individual clipped lines and
       * otherwise leave the strip intact - that might be better, but
       * require more complex code here.
       */
      if (count >= 2) {
         first->reset_stipple_counter( first );
	 prim.v[0] = 0;
	 prim.v[1] = get_vertex( draw, elts[0] );
	 
	 for (i = 1; i < count; i++) {
	    prim.v[0] = prim.v[1];
	    prim.v[1] = get_vertex( draw, elts[i] );
	    
	    first->line( first, &prim );
	 }
      }
      break;

   case GL_TRIANGLES:
      for (i = 0; i+2 < count; i += 3) {
	 prim.v[0] = get_vertex( draw, elts[i + 0] );
	 prim.v[1] = get_vertex( draw, elts[i + 1] );
	 prim.v[2] = get_vertex( draw, elts[i + 2] );
      
	 first->tri( first, &prim );
      }
      break;

   case GL_TRIANGLE_STRIP:
      for (i = 0; i+2 < count; i++) {
	 if (i & 1) {
	    prim.v[0] = get_vertex( draw, elts[i + 1] );
	    prim.v[1] = get_vertex( draw, elts[i + 0] );
	    prim.v[2] = get_vertex( draw, elts[i + 2] );
	 }
	 else {
	    prim.v[0] = get_vertex( draw, elts[i + 0] );
	    prim.v[1] = get_vertex( draw, elts[i + 1] );
	    prim.v[2] = get_vertex( draw, elts[i + 2] );
	 }
	 
	 first->tri( first, &prim );
      }
      break;

   case GL_TRIANGLE_FAN:
      if (count >= 3) {
	 prim.v[0] = get_vertex( draw, elts[0] );
	 prim.v[1] = 0;
	 prim.v[2] = get_vertex( draw, elts[1] );
	 
	 for (i = 0; i+2 < count; i++) {
	    prim.v[1] = prim.v[2];
	    prim.v[2] = get_vertex( draw, elts[i+2] );
      
	    first->tri( first, &prim );
	 }
      }
      break;

   case GL_QUADS:
      for (i = 0; i+3 < count; i += 4) {
	 do_quad( first,
		  get_vertex( draw, elts[i + 0] ),
		  get_vertex( draw, elts[i + 1] ),
		  get_vertex( draw, elts[i + 2] ),
		  get_vertex( draw, elts[i + 3] ));
      }
      break;

   case GL_QUAD_STRIP:
      for (i = 0; i+3 < count; i += 2) {
	 do_quad( first,
		  get_vertex( draw, elts[i + 2] ),
		  get_vertex( draw, elts[i + 0] ),
		  get_vertex( draw, elts[i + 1] ),
		  get_vertex( draw, elts[i + 3] ));
      }
      break;


   case GL_POLYGON:
      if (count >= 3) {
         int e1save, e2save;
	 prim.v[0] = 0;
	 prim.v[1] = get_vertex( draw, elts[1] );
	 prim.v[2] = get_vertex( draw, elts[0] );
	 e2save = prim.v[2]->edgeflag;
	 
	 for (i = 0; i+2 < count; i++) {
	    prim.v[0] = prim.v[1];
	    prim.v[1] = get_vertex( draw, elts[i+2] );
      
            /* save v1 edge flag, and clear if not last triangle */
            e1save = prim.v[1]->edgeflag;
            if (i + 3 < count)
               prim.v[1]->edgeflag = 0;

            /* draw */
	    first->tri( first, &prim );

            prim.v[1]->edgeflag = e1save; /* restore */
            prim.v[2]->edgeflag = 0; /* disable edge after 1st tri */
	 }
         prim.v[2]->edgeflag = e2save;
      }
      break;

   default:
      assert(0);
      break;
   }
}

static void draw_prim( struct draw_context *draw,
		       GLuint start,
		       GLuint count )
{
   struct draw_stage * const first = draw->pipeline.first;
   struct prim_header prim;
   GLuint i;

//   _mesa_printf("%s (%d) %d/%d\n", __FUNCTION__, draw->prim, start, count );

   prim.det = 0;		/* valid from cull stage onwards */
   prim.v[0] = 0;
   prim.v[1] = 0;
   prim.v[2] = 0;

   switch (draw->prim) {
   case GL_POINTS:
      for (i = 0; i < count; i ++) {
	 prim.v[0] = get_vertex( draw, start + i );
	 first->point( first, &prim );
      }
      break;

   case GL_LINES:
      for (i = 0; i+1 < count; i += 2) {
	 prim.v[0] = get_vertex( draw, start + i + 0 );
	 prim.v[1] = get_vertex( draw, start + i + 1 );

         first->reset_stipple_counter( first );
	 first->line( first, &prim );
      }
      break;

   case GL_LINE_LOOP:  
      if (count >= 2) {
         first->reset_stipple_counter( first );
	 for (i = 1; i < count; i++) {
	    prim.v[0] = get_vertex( draw, start + i - 1 );
	    prim.v[1] = get_vertex( draw, start + i );	    
	    first->line( first, &prim );
	 }

	 prim.v[0] = get_vertex( draw, start + count - 1 );
	 prim.v[1] = get_vertex( draw, start + 0 );	    
	 first->line( first, &prim );
      }
      break;

   case GL_LINE_STRIP:
      if (count >= 2) {
         first->reset_stipple_counter( first );
	 prim.v[0] = 0;
	 prim.v[1] = get_vertex( draw, start + 0 );
	 
	 for (i = 1; i < count; i++) {
	    prim.v[0] = prim.v[1];
	    prim.v[1] = get_vertex( draw, start + i );
	    
	    first->line( first, &prim );
	 }
      }
      break;

   case GL_TRIANGLES:
      for (i = 0; i+2 < count; i += 3) {
	 prim.v[0] = get_vertex( draw, start + i + 0 );
	 prim.v[1] = get_vertex( draw, start + i + 1 );
	 prim.v[2] = get_vertex( draw, start + i + 2 );
      
	 first->tri( first, &prim );
      }
      break;

   case GL_TRIANGLE_STRIP:
      for (i = 0; i+2 < count; i++) {
	 if (i & 1) {
	    prim.v[0] = get_vertex( draw, start + i + 1 );
	    prim.v[1] = get_vertex( draw, start + i + 0 );
	    prim.v[2] = get_vertex( draw, start + i + 2 );
	 }
	 else {
	    prim.v[0] = get_vertex( draw, start + i + 0 );
	    prim.v[1] = get_vertex( draw, start + i + 1 );
	    prim.v[2] = get_vertex( draw, start + i + 2 );
	 }
	 
	 first->tri( first, &prim );
      }
      break;

   case GL_TRIANGLE_FAN:
      if (count >= 3) {
	 prim.v[0] = get_vertex( draw, start + 0 );
	 prim.v[1] = 0;
	 prim.v[2] = get_vertex( draw, start + 1 );
	 
	 for (i = 0; i+2 < count; i++) {
	    prim.v[1] = prim.v[2];
	    prim.v[2] = get_vertex( draw, start + i + 2 );
      
	    first->tri( first, &prim );
	 }
      }
      break;


   case GL_QUADS:
      for (i = 0; i+3 < count; i += 4) {
	 do_quad( first,
		  get_vertex( draw, start + i + 0 ),
		  get_vertex( draw, start + i + 1 ),
		  get_vertex( draw, start + i + 2 ),
		  get_vertex( draw, start + i + 3 ));
      }
      break;

   case GL_QUAD_STRIP:
      for (i = 0; i+3 < count; i += 2) {
	 do_quad( first,
		  get_vertex( draw, start + i + 2 ),
		  get_vertex( draw, start + i + 0 ),
		  get_vertex( draw, start + i + 1 ),
		  get_vertex( draw, start + i + 3 ));
      }
      break;

   case GL_POLYGON:
      if (count >= 3) {
         int e1save, e2save;
	 prim.v[0] = 0;
	 prim.v[1] = get_vertex( draw, start + 1 );
	 prim.v[2] = get_vertex( draw, start + 0 );
	 e2save = prim.v[2]->edgeflag;

	 for (i = 0; i+2 < count; i++) {
	    prim.v[0] = prim.v[1];
	    prim.v[1] = get_vertex( draw, start + i + 2 );

            /* save v1 edge flag, and clear if not last triangle */
            e1save = prim.v[1]->edgeflag;
            if (i + 3 < count)
               prim.v[1]->edgeflag = 0;

            /* draw */
	    first->tri( first, &prim );

            prim.v[1]->edgeflag = e1save; /* restore */
            prim.v[2]->edgeflag = 0; /* disable edge after 1st tri */
	 }
         prim.v[2]->edgeflag = e2save;
      }
      break;

   default:
      assert(0);
      break;
   }
}


static void draw_release_vertices( struct draw_context *draw )
{
   draw->pipeline.first->end( draw->pipeline.first );

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

   /* Allocate the vertices:
    */
   draw_allocate_vertices( draw, VB->Count );

   /* Bind the vb outputs:
    */
   vf_set_sources( draw->vf, VB->AttribPtr, 0 );

   /* Build the hardware or prim-pipe vertices: 
    */
   vf_emit_vertices( draw->vf, VB->Count, draw->verts );


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

      if (VB->Elts) {
	 draw_indexed_prim( draw, 
			    VB->Elts + start,
			    length );
      }
      else {
	 draw_prim( draw, 
		    start,
		    length );
      }	 
   }

   draw_release_vertices( draw );
   draw->verts = NULL;
   draw->in_vb = 0;
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

   draw->vertex_size
      = sizeof(struct vertex_header) + numAttrs * 4 * sizeof(GLfloat);

   /*draw_prim_info(mode, &first, &incr);*/
   draw_allocate_vertices( draw, numVerts );
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
			    

#define MAX_VERTEX_SIZE ((2 + FRAG_ATTRIB_MAX) * 4 * sizeof(GLfloat))

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
