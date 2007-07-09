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

#include "tnl/t_context.h"
#include "vf/vf.h"

#include "sp_context.h"
#include "sp_prim.h"
#include "sp_headers.h"
#include "sp_draw.h"

/* This file is a temporary set of hooks to allow us to use the tnl/
 * and vf/ modules until we have replacements in pipe.
 */


struct draw_context 
{
   struct softpipe_context *softpipe;

   struct vf_attr_map attrs[VF_ATTRIB_MAX];
   GLuint nr_attrs;
   GLuint vertex_size;
   struct vertex_fetch *vf;

   GLubyte *verts;
   GLuint nr_vertices;
   GLboolean in_vb;

   GLenum prim;

   /* Helper for tnl:
    */
   GLvector4f header;   
};


static struct vertex_header *get_vertex( struct draw_context *pipe,
					       GLuint i )
{
   return (struct vertex_header *)(pipe->verts + i * pipe->vertex_size);
}



static void draw_allocate_vertices( struct draw_context *draw,
				    GLuint nr_vertices )
{
   draw->nr_vertices = nr_vertices;
   draw->verts = MALLOC( nr_vertices * draw->vertex_size );

   draw->softpipe->prim.first->begin( draw->softpipe->prim.first );
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
			  


static void do_quad( struct prim_stage *first,
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
   struct prim_stage * const first = draw->softpipe->prim.first;
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
      
	 first->line( first, &prim );
      }
      break;

   case GL_LINE_LOOP:  
      if (count >= 2) {
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
	 prim.v[0] = 0;
	 prim.v[1] = get_vertex( draw, elts[1] );
	 prim.v[2] = get_vertex( draw, elts[0] );
	 
	 for (i = 0; i+2 < count; i++) {
	    prim.v[0] = prim.v[1];
	    prim.v[1] = get_vertex( draw, elts[i+2] );
      
	    first->tri( first, &prim );
	 }
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
   struct prim_stage * const first = draw->softpipe->prim.first;
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
      
	 first->line( first, &prim );
      }
      break;

   case GL_LINE_LOOP:  
      if (count >= 2) {
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
	 prim.v[0] = 0;
	 prim.v[1] = get_vertex( draw, start + 1 );
	 prim.v[2] = get_vertex( draw, start + 0 );
	 
	 for (i = 0; i+2 < count; i++) {
	    prim.v[0] = prim.v[1];
	    prim.v[1] = get_vertex( draw, start + i + 2 );
      
	    first->tri( first, &prim );
	 }
      }
      break;

   default:
      assert(0);
      break;
   }
}


static void draw_release_vertices( struct draw_context *draw )
{
   draw->softpipe->prim.first->end( draw->softpipe->prim.first );

   FREE(draw->verts);
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

void draw_set_viewport( struct draw_context *draw,
			const GLfloat *scale,
			const GLfloat *translate )
{
   assert(!draw->in_vb);
   vf_set_vp_scale_translate( draw->vf, scale, translate );
}



struct draw_context *draw_create( struct softpipe_context *softpipe )
{
   struct draw_context *draw = CALLOC_STRUCT( draw_context );
   draw->softpipe = softpipe;
   draw->vf = vf_create( GL_TRUE );

   return draw;
}


void draw_destroy( struct draw_context *draw )
{
   if (draw->header.storage)
      ALIGN_FREE( draw->header.storage );

   vf_destroy( draw->vf );

   FREE( draw );
}

#define EMIT_ATTR( ATTR, STYLE )		\
do {						\
   draw->attrs[draw->nr_attrs].attrib = ATTR;	\
   draw->attrs[draw->nr_attrs].format = STYLE;	\
   draw->nr_attrs++;				\
} while (0)


void draw_set_vertex_attributes( struct draw_context *draw,
				 const GLuint *attrs,
				 GLuint nr_attrs )
{
   GLuint i;

   draw->nr_attrs = 0;

   EMIT_ATTR(VF_ATTRIB_VERTEX_HEADER, EMIT_1F);
   EMIT_ATTR(VF_ATTRIB_CLIP_POS, EMIT_4F);

   assert(attrs[0] == VF_ATTRIB_POS);
   EMIT_ATTR(attrs[0], EMIT_4F_VIEWPORT);

   for (i = 1; i < nr_attrs; i++) 
      EMIT_ATTR(attrs[i], EMIT_4F);

   draw->vertex_size = vf_set_vertex_attributes( draw->vf, draw->attrs, draw->nr_attrs, 0 );
}
			    

