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
#include "pipe/p_winsys.h"

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
static void vbuf_flush_vertices( struct draw_stage *stage );


#define VBUF_SIZE (64*1024)
#define IBUF_SIZE (16*1024)


/**
 * Vertex buffer emit stage.
 */
struct vbuf_stage {
   struct draw_stage stage; /**< This must be first (base class) */

   /** Vertex size in bytes */
   unsigned vertex_size;

   /* FIXME: we have no guarantee that 'unsigned' is 32bit */

   /** Vertices in hardware format */
   unsigned *vertex_map;
   unsigned *vertex_ptr;
   unsigned max_vertices;
   unsigned nr_vertices;
   
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


static void check_space( struct vbuf_stage *vbuf )
{
   if (overflow( vbuf->vertex_map, 
                 vbuf->vertex_ptr,  
                 vbuf->vertex_size, 
                 VBUF_SIZE ))
      vbuf_flush_vertices(&vbuf->stage);

   if (vbuf->nr_elements + 4 > IBUF_SIZE / sizeof(ushort) )
      vbuf_flush_elements(&vbuf->stage);
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

   check_space( vbuf );

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

   check_space( vbuf );

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

   check_space( vbuf );

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


static void vbuf_flush_elements( struct draw_stage *stage ) 
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );
   struct i915_context *i915 = vbuf->i915;
   unsigned nr = vbuf->nr_elements;
   unsigned vertex_size = i915->current.vertex_info.size * 4; /* in bytes */
   unsigned hwprim;
   unsigned i;
   
   if(!nr)
      return;
   
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
   
   assert(vbuf->vertex_ptr - vbuf->vertex_map == vbuf->nr_vertices * vertex_size / 4);

   assert((i915->dirty & ~I915_NEW_VBO) == 0);
   
   if (i915->dirty)
      i915_update_derived( i915 );

   if (i915->hardware_dirty)
      i915_emit_hardware_state( i915 );

   if (!BEGIN_BATCH( 1 + (nr + 1)/2, 1 )) {
      FLUSH_BATCH();

      /* Make sure state is re-emitted after a flush: 
       */
      i915_update_derived( i915 );
      i915_emit_hardware_state( i915 );

      if (!BEGIN_BATCH( 1 + (nr + 1)/2, 1 )) {
	 assert(0);
	 return;
      }
   }

   OUT_BATCH( _3DPRIMITIVE |
              PRIM_INDIRECT |
   	      hwprim |
   	      PRIM_INDIRECT_ELTS |
   	      nr );
   for (i = 0; i + 1 < nr; i += 2) {
      OUT_BATCH( vbuf->element_map[i] |
                 (vbuf->element_map[i + 1] << 16) );
   }
   if (i < nr) {
      OUT_BATCH( vbuf->element_map[i] );
   }
   
   vbuf->nr_elements = 0;
}



/**
 * Flush vertex buffer.
 */
static void vbuf_flush_vertices( struct draw_stage *stage )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );
   struct i915_context *i915 = vbuf->i915;
   struct pipe_winsys *winsys = i915->pipe.winsys;

   if(vbuf->nr_vertices) {
      
      vbuf_flush_elements(stage);
      
      winsys->buffer_unmap(winsys, i915->vbo);
   
      vbuf->nr_vertices = 0;
   
      /**
       * XXX: Reset vertex ids?  Actually, want to not do that unless our
       * vertex buffer is full.  Would like separate
       * flush-on-index-full and flush-on-vb-full, but may raise
       * issues uploading vertices if the hardware wants to flush when
       * we flush.
       */
      draw_vertex_cache_reset_vertex_ids( vbuf->i915->draw );
   }
   
   /* FIXME: handle failure */
   if(!i915->vbo)
      i915->vbo = winsys->buffer_create(winsys, 64);
   winsys->buffer_data( winsys, i915->vbo, 
                        VBUF_SIZE, NULL, 
                        I915_BUFFER_USAGE_LIT_VERTEX );
   
   i915->dirty |= I915_NEW_VBO;
   
   vbuf->vertex_map = winsys->buffer_map(winsys, 
                                         i915->vbo, 
                                         PIPE_BUFFER_FLAG_WRITE );
   vbuf->vertex_ptr = vbuf->vertex_map;
}



static void vbuf_begin( struct draw_stage *stage )
{
   struct vbuf_stage *vbuf = vbuf_stage(stage);
   struct i915_context *i915 = vbuf->i915;
   unsigned vertex_size = i915->current.vertex_info.size * 4;

   assert(!i915->dirty);
   
   if(vbuf->vertex_size != vertex_size)
      vbuf_flush_vertices(stage);
      
   vbuf->vertex_size = vertex_size;
}


static void vbuf_end( struct draw_stage *stage )
{
   /* Overkill.
    */
   vbuf_flush_elements( stage );
   
   stage->point = vbuf_first_point;
   stage->line = vbuf_first_line;
   stage->tri = vbuf_first_tri;
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
   vbuf->vertex_map = NULL;
   
   vbuf->vertex_ptr = vbuf->vertex_map;
   
   return &vbuf->stage;
}
