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

/**
 * \file
 * Vertex buffer drawing stage.
 * 
 * \author José Fonseca <jrfonsec@tungstengraphics.com>
 * \author Keith Whitwell <keith@tungstengraphics.com>
 */


#include <assert.h>

#include "pipe/draw/draw_vbuf.h"
#include "pipe/draw/draw_private.h"
#include "pipe/draw/draw_vertex.h"
#include "pipe/p_util.h"


/**
 * Vertex buffer emit stage.
 */
struct vbuf_stage {
   struct draw_stage stage; /**< This must be first (base class) */

   struct vbuf_render *render;
   
   /** Vertex size in bytes */
   unsigned vertex_size;

   /* FIXME: we have no guarantee that 'unsigned' is 32bit */

   /** Vertices in hardware format */
   unsigned *vertices;
   unsigned *vertex_ptr;
   unsigned max_vertices;
   unsigned nr_vertices;
   
   /** Indices */
   ushort *indices;
   unsigned max_indices;
   unsigned nr_indices;

   /** Pipe primitive */
   unsigned prim;
};


/**
 * Basically a cast wrapper.
 */
static INLINE struct vbuf_stage *
vbuf_stage( struct draw_stage *stage )
{
   assert(stage);
   return (struct vbuf_stage *)stage;
}


static void vbuf_flush_indices( struct draw_stage *stage );
static void vbuf_flush_vertices( struct draw_stage *stage );
static void vbuf_alloc_vertices( struct draw_stage *stage,
                                 unsigned new_vertex_size );


static INLINE boolean 
overflow( void *map, void *ptr, unsigned bytes, unsigned bufsz )
{
   unsigned long used = (unsigned long) ((char *)ptr - (char *)map);
   return (used + bytes) > bufsz;
}


static INLINE void 
check_space( struct vbuf_stage *vbuf, unsigned nr )
{
   if (vbuf->nr_vertices + nr > vbuf->max_vertices ) {
      vbuf_flush_vertices(&vbuf->stage);
      vbuf_alloc_vertices(&vbuf->stage, vbuf->vertex_size);
   }

   if (vbuf->nr_indices + nr > vbuf->max_indices )
      vbuf_flush_indices(&vbuf->stage);
}


/**
 * Extract the needed fields from vertex_header and emit i915 dwords.
 * Recall that the vertices are constructed by the 'draw' module and
 * have a couple of slots at the beginning (1-dword header, 4-dword
 * clip pos) that we ignore here.
 */
static INLINE void 
emit_vertex( struct vbuf_stage *vbuf,
             struct vertex_header *vertex )
{
   const struct vertex_info *vinfo = vbuf->render->get_vertex_info(vbuf->render);

   uint i;
   uint count = 0;  /* for debug/sanity */

//   fprintf(stderr, "emit vertex %d to %p\n", 
//           vbuf->nr_vertices, vbuf->vertex_ptr);

   if(vertex->vertex_id != UNDEFINED_VERTEX_ID) {
      if(vertex->vertex_id < vbuf->nr_vertices)
	 return;
      else
	 fprintf(stderr, "Bad vertex id 0x%04x (>= 0x%04x)\n", 
	         vertex->vertex_id, vbuf->nr_vertices);
      return;
   }
      
   vertex->vertex_id = vbuf->nr_vertices++;

   for (i = 0; i < vinfo->num_attribs; i++) {
      switch (vinfo->format[i]) {
      case FORMAT_OMIT:
         /* no-op */
         break;
      case FORMAT_1F:
         *vbuf->vertex_ptr++ = fui(vertex->data[i][0]);
         count++;
         break;
      case FORMAT_2F:
         *vbuf->vertex_ptr++ = fui(vertex->data[i][0]);
         *vbuf->vertex_ptr++ = fui(vertex->data[i][1]);
         count += 2;
         break;
      case FORMAT_3F:
         *vbuf->vertex_ptr++ = fui(vertex->data[i][0]);
         *vbuf->vertex_ptr++ = fui(vertex->data[i][1]);
         *vbuf->vertex_ptr++ = fui(vertex->data[i][2]);
         count += 3;
         break;
      case FORMAT_4F:
         *vbuf->vertex_ptr++ = fui(vertex->data[i][0]);
         *vbuf->vertex_ptr++ = fui(vertex->data[i][1]);
         *vbuf->vertex_ptr++ = fui(vertex->data[i][2]);
         *vbuf->vertex_ptr++ = fui(vertex->data[i][3]);
         count += 4;
         break;
      case FORMAT_4UB:
	 *vbuf->vertex_ptr++ = pack_ub4(float_to_ubyte( vertex->data[i][2] ),
                                        float_to_ubyte( vertex->data[i][1] ),
                                        float_to_ubyte( vertex->data[i][0] ),
                                        float_to_ubyte( vertex->data[i][3] ));
         count += 1;
         break;
      default:
         assert(0);
      }
   }
   assert(count == vinfo->size);
}


static void 
vbuf_tri( struct draw_stage *stage,
          struct prim_header *prim )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );
   unsigned i;

   check_space( vbuf, 3 );

   for (i = 0; i < 3; i++) {
      emit_vertex( vbuf, prim->v[i] );
      
      vbuf->indices[vbuf->nr_indices++] = (ushort) prim->v[i]->vertex_id;
   }
}


static void 
vbuf_line( struct draw_stage *stage, 
           struct prim_header *prim )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );
   unsigned i;

   check_space( vbuf, 2 );

   for (i = 0; i < 2; i++) {
      emit_vertex( vbuf, prim->v[i] );

      vbuf->indices[vbuf->nr_indices++] = (ushort) prim->v[i]->vertex_id;
   }   
}


static void 
vbuf_point( struct draw_stage *stage, 
            struct prim_header *prim )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   check_space( vbuf, 1 );

   emit_vertex( vbuf, prim->v[0] );
   
   vbuf->indices[vbuf->nr_indices++] = (ushort) prim->v[0]->vertex_id;
}


static void 
vbuf_first_tri( struct draw_stage *stage,
                struct prim_header *prim )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   vbuf_flush_indices( stage );   

   stage->tri = vbuf_tri;
   vbuf->prim = PIPE_PRIM_TRIANGLES;
   vbuf->render->set_primitive(vbuf->render, PIPE_PRIM_TRIANGLES);

   stage->tri( stage, prim );
}


static void 
vbuf_first_line( struct draw_stage *stage,
                 struct prim_header *prim )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   vbuf_flush_indices( stage );
   stage->line = vbuf_line;
   vbuf->prim = PIPE_PRIM_LINES;
   vbuf->render->set_primitive(vbuf->render, PIPE_PRIM_LINES);

   stage->line( stage, prim );
}


static void 
vbuf_first_point( struct draw_stage *stage,
                  struct prim_header *prim )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   vbuf_flush_indices( stage );

   stage->point = vbuf_point;
   vbuf->prim = PIPE_PRIM_POINTS;
   vbuf->render->set_primitive(vbuf->render, PIPE_PRIM_POINTS);

   stage->point( stage, prim );
}


static void 
vbuf_flush_indices( struct draw_stage *stage ) 
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   if(!vbuf->nr_indices)
      return;
   
   assert((uint) (vbuf->vertex_ptr - vbuf->vertices) == 
          vbuf->nr_vertices * vbuf->vertex_size / sizeof(unsigned));

   switch(vbuf->prim) {
   case PIPE_PRIM_POINTS:
      break;
   case PIPE_PRIM_LINES:
      assert(vbuf->nr_indices % 2 == 0);
      break;
   case PIPE_PRIM_TRIANGLES:
      assert(vbuf->nr_indices % 3 == 0);
      break;
   default:
      assert(0);
   }
   
   vbuf->render->draw(vbuf->render, vbuf->indices, vbuf->nr_indices);
   
   vbuf->nr_indices = 0;

   stage->point = vbuf_first_point;
   stage->line = vbuf_first_line;
   stage->tri = vbuf_first_tri;
}


/**
 * Flush existing vertex buffer and allocate a new one.
 * 
 * XXX: We separate flush-on-index-full and flush-on-vb-full, but may 
 * raise issues uploading vertices if the hardware wants to flush when
 * we flush.
 */
static void 
vbuf_flush_vertices( struct draw_stage *stage )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   if(vbuf->vertices) {      
      vbuf_flush_indices(stage);
      
      /* Reset temporary vertices ids */
      if(vbuf->nr_vertices)
	 draw_reset_vertex_ids( vbuf->stage.draw );
      
      /* Free the vertex buffer */
      vbuf->render->release_vertices(vbuf->render,
                                     vbuf->vertices,
                                     vbuf->vertex_size,
                                     vbuf->nr_vertices);
      vbuf->nr_vertices = 0;
      vbuf->vertex_ptr = vbuf->vertices = NULL;
      
   }
}
   

static void 
vbuf_alloc_vertices( struct draw_stage *stage,
		     unsigned new_vertex_size )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   assert(!vbuf->nr_indices);
   assert(!vbuf->vertices);
   
   /* Allocate a new vertex buffer */
   vbuf->vertex_size = new_vertex_size;
   vbuf->max_vertices = vbuf->render->max_vertex_buffer_bytes / vbuf->vertex_size;
   vbuf->vertices = vbuf->render->allocate_vertices(vbuf->render,
                                                    (ushort) vbuf->vertex_size,
                                                    (ushort) vbuf->max_vertices);
   vbuf->vertex_ptr = vbuf->vertices;
}


static void 
vbuf_begin( struct draw_stage *stage )
{
   struct vbuf_stage *vbuf = vbuf_stage(stage);
   const struct vertex_info *vinfo = vbuf->render->get_vertex_info(vbuf->render);
   unsigned vertex_size = vinfo->size * sizeof(float);

   /* XXX: Overkill */
   vbuf_alloc_vertices(&vbuf->stage, vertex_size);
}


static void 
vbuf_end( struct draw_stage *stage )
{
//   vbuf_flush_indices( stage );
   /* XXX: Overkill */
   vbuf_flush_vertices( stage );
   
   stage->point = vbuf_first_point;
   stage->line = vbuf_first_line;
   stage->tri = vbuf_first_tri;
}


static void 
vbuf_reset_stipple_counter( struct draw_stage *stage )
{
}


static void vbuf_destroy( struct draw_stage *stage )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   align_free( vbuf->indices );
   FREE( stage );
}


/**
 * Create a new primitive vbuf/render stage.
 */
struct draw_stage *draw_vbuf_stage( struct draw_context *draw,
                                    struct vbuf_render *render )
{
   struct vbuf_stage *vbuf = CALLOC_STRUCT(vbuf_stage);

   vbuf->stage.draw = draw;
   vbuf->stage.begin = vbuf_begin;
   vbuf->stage.point = vbuf_first_point;
   vbuf->stage.line = vbuf_first_line;
   vbuf->stage.tri = vbuf_first_tri;
   vbuf->stage.end = vbuf_end;
   vbuf->stage.reset_stipple_counter = vbuf_reset_stipple_counter;
   vbuf->stage.destroy = vbuf_destroy;
   
   vbuf->render = render;

   assert(render->max_indices < UNDEFINED_VERTEX_ID);
   vbuf->max_indices = render->max_indices;
   vbuf->indices
      = align_malloc( vbuf->max_indices * sizeof(vbuf->indices[0]), 16 );
   
   vbuf->vertices = NULL;
   vbuf->vertex_ptr = vbuf->vertices;
   
   return &vbuf->stage;
}
