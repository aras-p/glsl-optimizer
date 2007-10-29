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
#include "i915_reg.h"
#include "i915_winsys.h"
#include "i915_batch.h"
#include "i915_state.h"


static void vbuf_flush_elements( struct draw_stage *stage );


#define VBUF_SIZE (64*1024)
#define IBUF_SIZE (16*1024)


/**
 * Vertex buffer emit stage.
 */
struct vbuf_stage {
   struct draw_stage stage; /**< This must be first (base class) */

   /* FIXME: we have no guarantee that 'unsigned' is 32bit */
   
   /* Vertices are passed in as an array of floats making up each
    * attribute in turn.  Will eventually convert to hardware format
    * in this stage.
    */
   unsigned *vertex_map;
   unsigned *vertex_ptr;
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


static inline 
boolean overflow( void *map, void *ptr, unsigned bytes, unsigned bufsz )
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


/**
 * Extract the needed fields from vertex_header and emit i915 dwords.
 * Recall that the vertices are constructed by the 'draw' module and
 * have a couple of slots at the beginning (1-dword header, 4-dword
 * clip pos) that we ignore here.
 */
static inline void 
emit_vertex( struct vbuf_stage *vbuf,
             struct vertex_header *vertex )
{
   struct i915_context *i915 = vbuf->i915;
   const struct vertex_info *vinfo = &i915->current.vertex_info;
   uint i;
   uint count = 0;  /* for debug/sanity */

//   fprintf(stderr, "emit vertex %d to %p\n", 
//           vbuf->nr_vertices, vbuf->vertex_ptr);

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


static void vbuf_draw( struct draw_stage *stage ) 
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );
   struct i915_context *i915 = vbuf->i915;
   unsigned nr = vbuf->nr_elements;
   unsigned vertex_size = i915->current.vertex_info.size * 4; /* in bytes */
   unsigned hwprim;
   unsigned *ptr;
   unsigned i, j;
   
   switch(vbuf->prim) {
   case PIPE_PRIM_POINTS:
      hwprim = PRIM3D_POINTLIST;
      break;
   case PIPE_PRIM_LINES:
      hwprim = PRIM3D_LINELIST;
      break;
   case PIPE_PRIM_TRIANGLES:
      hwprim = PRIM3D_TRILIST;
      break;
   default:
      assert(0);
      return;
   }

   if (i915->dirty)
      i915_update_derived( i915 );

   if (i915->hardware_dirty)
      i915_emit_hardware_state( i915 );

   assert(vbuf->vertex_ptr - vbuf->vertex_map == vbuf->nr_vertices * vertex_size / 4);
   
   ptr = BEGIN_BATCH( 1 + nr * vertex_size / 4, 0 );
   if (ptr == 0) {
      FLUSH_BATCH();

      /* Make sure state is re-emitted after a flush: 
       */
      i915_update_derived( i915 );
      i915_emit_hardware_state( i915 );

      ptr = BEGIN_BATCH( 1 + nr * vertex_size / 4, 0 );
      if (ptr == 0) {
	 assert(0);
	 return;
      }
   }

   /* TODO: Fire a DMA buffer */
   OUT_BATCH(_3DPRIMITIVE | 
	     hwprim |
	     ((4 + vertex_size * nr)/4 - 2));

   for (i = 0; i < nr; i++)
      for (j = 0; j < vertex_size / 4; j++)
        OUT_BATCH(vbuf->vertex_map[vbuf->element_map[i]*vertex_size/4 + j]);
}


static void vbuf_flush_elements( struct draw_stage *stage )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   if (vbuf->nr_elements) {
#if 0
      fprintf(stderr, "%s (%d elts, %d verts)\n", 
                      __FUNCTION__, 
                      vbuf->nr_elements,
                      vbuf->nr_vertices);
#endif

      /* Draw now or add to list of primitives???
       */
      vbuf_draw( stage );
      
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

   vbuf->vertex_size = vbuf->i915->current.vertex_info.size * 4;
}


static void vbuf_end( struct draw_stage *stage )
{
   /* Overkill.
    */
   vbuf_flush_elements( stage );
}


static void reset_stipple_counter( struct draw_stage *stage )
{
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

   /* FIXME: free this memory on takedown */
   vbuf->element_map = malloc( IBUF_SIZE );
   vbuf->vertex_map = malloc( VBUF_SIZE );
   
   vbuf->vertex_ptr = vbuf->vertex_map;
   
   return &vbuf->stage;
}
