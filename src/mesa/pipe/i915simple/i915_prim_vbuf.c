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
 * Build post-transformation, post-clipping vertex buffers and element
 * lists by hooking into the end of the primitive pipeline and
 * manipulating the vertex_id field in the vertex headers.
 *
 * XXX: work in progress 
 * 
 * \author José Fonseca <jrfonseca@tungstengraphics.com>
 * \author Keith Whitwell <keith@tungstengraphics.com>
 */


#include "pipe/draw/draw_private.h"
#include "pipe/draw/draw_vertex.h"
#include "pipe/p_util.h"

#include "softpipe/sp_context.h"
#include "softpipe/sp_headers.h"
#include "softpipe/sp_quad.h"
#include "softpipe/sp_prim_setup.h"

#include "i915_context.h"


static void vbuf_flush_elements( struct draw_stage *stage );


#define VBUF_SIZE (64*1024)
#define IBUF_SIZE (16*1024)


/**
 * Vertex buffer emit stage.
 */
struct vbuf_stage {
   struct draw_stage stage; /**< This must be first (base class) */

   vbuf_draw_func draw;

   /* Vertices are passed in as an array of floats making up each
    * attribute in turn.  Will eventually convert to hardware format
    * in this stage.
    */
   char *vertex_map;
   char *vertex_ptr;
   unsigned vertex_size;
   unsigned nr_vertices;

   unsigned max_vertices;

   ushort *element_map;
   unsigned nr_elements;

   unsigned prim;

   struct i915_context *i915;   
};


/**
 * Basically a cast wrapper.
 */
static INLINE struct vbuf_stage *vbuf_stage( struct draw_stage *stage )
{
   return (struct vbuf_stage *)stage;
}


static boolean overflow( void *map, void *ptr, unsigned bytes, unsigned bufsz )
{
   unsigned long used = (char *)ptr - (char *)map;
   return (used + bytes) > bufsz;
}


static boolean check_space( struct vbuf_stage *vbuf )
{
   if (overflow( vbuf->vertex_map, 
                 vbuf->vertex_ptr,  
                 4 * vbuf->vertex_size, 
                 VBUF_SIZE ))
      return FALSE;

   
   if (vbuf->nr_elements + 4 > IBUF_SIZE / sizeof(ushort) )
      return FALSE;
   
   return TRUE;
}


static void emit_vertex( struct vbuf_stage *vbuf,
                         struct vertex_header *vertex )
{
//   fprintf(stderr, "emit vertex %d to %p\n", 
//           vbuf->nr_vertices, vbuf->vertex_ptr);

   vertex->vertex_id = vbuf->nr_vertices++;

   //vbuf->emit_vertex( vbuf->vertex_ptr, vertex );
   memcpy(vbuf->vertex_ptr, vertex, vbuf->vertex_size);

   vbuf->vertex_ptr += vbuf->vertex_size;
}


static void vbuf_tri( struct draw_stage *stage,
                      struct prim_header *prim )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );
   unsigned i;

   if (!check_space( vbuf ))
      vbuf_flush_elements( stage );

   for (i = 0; i < 3; i++) {
      if (prim->v[i]->vertex_id == 0xffff) 
         emit_vertex( vbuf, prim->v[i] );
      
      vbuf->element_map[vbuf->nr_elements++] = prim->v[i]->vertex_id;
   }
}


static void vbuf_line(struct draw_stage *stage, 
                      struct prim_header *prim)
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );
   unsigned i;

   if (!check_space( vbuf ))
      vbuf_flush_elements( stage );

   for (i = 0; i < 2; i++) {
      if (prim->v[i]->vertex_id == 0xffff) 
         emit_vertex( vbuf, prim->v[i] );

      vbuf->element_map[vbuf->nr_elements++] = prim->v[i]->vertex_id;
   }   
}


static void vbuf_point(struct draw_stage *stage, 
                       struct prim_header *prim)
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   if (!check_space( vbuf ))
      vbuf_flush_elements( stage );

   if (prim->v[0]->vertex_id == 0xffff) 
      emit_vertex( vbuf, prim->v[0] );
   
   vbuf->element_map[vbuf->nr_elements++] = prim->v[0]->vertex_id;
}


static void vbuf_first_tri( struct draw_stage *stage,
                            struct prim_header *prim )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   vbuf_flush_elements( stage );   
   stage->tri = vbuf_tri;
   stage->tri( stage, prim );
   vbuf->prim = PIPE_PRIM_TRIANGLES;
}


static void vbuf_first_line( struct draw_stage *stage,
                             struct prim_header *prim )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   vbuf_flush_elements( stage );
   stage->line = vbuf_line;
   stage->line( stage, prim );
   vbuf->prim = PIPE_PRIM_LINES;
}


static void vbuf_first_point( struct draw_stage *stage,
                              struct prim_header *prim )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   vbuf_flush_elements( stage );
   stage->point = vbuf_point;
   stage->point( stage, prim );
   vbuf->prim = PIPE_PRIM_POINTS;
}


static void vbuf_draw( struct pipe_context *pipe,
                       unsigned prim,
                       const ushort *elements,
                       unsigned nr_elements,
                       const void *vertex_buffer,
                       unsigned nr_vertices ) 
{
   /* FIXME: */
}


static void vbuf_flush_elements( struct draw_stage *stage )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   if (vbuf->nr_elements) {
      fprintf(stderr, "%s (%d elts)\n", __FUNCTION__, vbuf->nr_elements);

      /* Draw now or add to list of primitives???
       */
      vbuf_draw( &vbuf->i915->pipe,
                  vbuf->prim,
                  vbuf->element_map,
                  vbuf->nr_elements,
                  vbuf->vertex_map,
                  (vbuf->vertex_ptr - vbuf->vertex_map) / vbuf->vertex_size );
      
      vbuf->nr_elements = 0;

      vbuf->vertex_ptr = vbuf->vertex_map;
      vbuf->nr_vertices = 0;

      /* Reset vertex ids?  Actually, want to not do that unless our
       * vertex buffer is full.  Would like separate
       * flush-on-index-full and flush-on-vb-full, but may raise
       * issues uploading vertices if the hardware wants to flush when
       * we flush.
       */
      draw_vertex_cache_reset_vertex_ids( vbuf->i915->draw );
   }

   stage->tri = vbuf_first_tri;
   stage->line = vbuf_first_line;
   stage->point = vbuf_first_point;
}


static void vbuf_begin( struct draw_stage *stage )
{
   struct vbuf_stage *vbuf = vbuf_stage(stage);

   vbuf->vertex_size = vbuf->i915->draw->vertex_info.size * sizeof(float);
}


static void vbuf_end( struct draw_stage *stage )
{
   /* Overkill.
    */
   vbuf_flush_elements( stage );
}


static void reset_stipple_counter( struct draw_stage *stage )
{
   /* XXX:  This doesn't work.
    */
}


/**
 * Create a new primitive vbuf/render stage.
 */
struct draw_stage *i915_draw_vbuf_stage( struct i915_context *i915 )
{
   struct vbuf_stage *vbuf = CALLOC_STRUCT(vbuf_stage);

   vbuf->i915 = i915;
   vbuf->stage.draw = i915->draw;
   vbuf->stage.begin = vbuf_begin;
   vbuf->stage.point = vbuf_first_point;
   vbuf->stage.line = vbuf_first_line;
   vbuf->stage.tri = vbuf_first_tri;
   vbuf->stage.end = vbuf_end;
   vbuf->stage.reset_stipple_counter = reset_stipple_counter;

   vbuf->element_map = malloc( IBUF_SIZE );
   vbuf->vertex_map = malloc( VBUF_SIZE );
   
   vbuf->vertex_ptr = vbuf->vertex_map;
   
   return &vbuf->stage;
}
