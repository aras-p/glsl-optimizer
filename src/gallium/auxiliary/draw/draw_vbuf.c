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


#include "pipe/p_debug.h"
#include "pipe/p_util.h"

#include "draw_vbuf.h"
#include "draw_private.h"
#include "draw_vertex.h"
#include "draw_vf.h"


/**
 * Vertex buffer emit stage.
 */
struct vbuf_stage {
   struct draw_stage stage; /**< This must be first (base class) */

   struct vbuf_render *render;
   
   const struct vertex_info *vinfo;
   
   /** Vertex size in bytes */
   unsigned vertex_size;

   struct draw_vertex_fetch *vf;
   
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


static void vbuf_flush_indices( struct vbuf_stage *vbuf );
static void vbuf_flush_vertices( struct vbuf_stage *vbuf );
static void vbuf_alloc_vertices( struct vbuf_stage *vbuf );


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
      vbuf_flush_vertices(vbuf);
      vbuf_alloc_vertices(vbuf);
   }

   if (vbuf->nr_indices + nr > vbuf->max_indices )
      vbuf_flush_indices(vbuf);
}


#if 0
static INLINE void
dump_emitted_vertex(const struct vertex_info *vinfo, const uint8_t *data)
{
   assert(vinfo == vbuf->render->get_vertex_info(vbuf->render));
   unsigned i, j, k;

   for (i = 0; i < vinfo->num_attribs; i++) {
      j = vinfo->src_index[i];
      switch (vinfo->emit[i]) {
      case EMIT_OMIT:
         debug_printf("EMIT_OMIT:");
         break;
      case EMIT_1F:
         debug_printf("EMIT_1F:\t");
         debug_printf("%f ", *(float *)data); data += sizeof(float);
         break;
      case EMIT_1F_PSIZE:
         debug_printf("EMIT_1F_PSIZE:\t");
         debug_printf("%f ", *(float *)data); data += sizeof(float);
         break;
      case EMIT_2F:
         debug_printf("EMIT_2F:\t");
         debug_printf("%f ", *(float *)data); data += sizeof(float);
         debug_printf("%f ", *(float *)data); data += sizeof(float);
         break;
      case EMIT_3F:
         debug_printf("EMIT_3F:\t");
         debug_printf("%f ", *(float *)data); data += sizeof(float);
         debug_printf("%f ", *(float *)data); data += sizeof(float);
         debug_printf("%f ", *(float *)data); data += sizeof(float);
         data += sizeof(float);
         break;
      case EMIT_4F:
         debug_printf("EMIT_4F:\t");
         debug_printf("%f ", *(float *)data); data += sizeof(float);
         debug_printf("%f ", *(float *)data); data += sizeof(float);
         debug_printf("%f ", *(float *)data); data += sizeof(float);
         debug_printf("%f ", *(float *)data); data += sizeof(float);
         break;
      case EMIT_4UB:
         debug_printf("EMIT_4UB:\t");
         debug_printf("%u ", *data++);
         debug_printf("%u ", *data++);
         debug_printf("%u ", *data++);
         debug_printf("%u ", *data++);
         break;
      default:
         assert(0);
      }
      debug_printf("\n");
   }
   debug_printf("\n");
}
#endif


/**
 * Extract the needed fields from post-transformed vertex and emit
 * a hardware(driver) vertex.
 * Recall that the vertices are constructed by the 'draw' module and
 * have a couple of slots at the beginning (1-dword header, 4-dword
 * clip pos) that we ignore here.  We only use the vertex->data[] fields.
 */
static INLINE void 
emit_vertex( struct vbuf_stage *vbuf,
             struct vertex_header *vertex )
{
#if 0
   debug_printf("emit vertex %d to %p\n", 
           vbuf->nr_vertices, vbuf->vertex_ptr);
#endif

   if(vertex->vertex_id != UNDEFINED_VERTEX_ID) {
      if(vertex->vertex_id < vbuf->nr_vertices)
	 return;
      else
	 debug_printf("Bad vertex id 0x%04x (>= 0x%04x)\n", 
	         vertex->vertex_id, vbuf->nr_vertices);
      return;
   }
      
   vertex->vertex_id = vbuf->nr_vertices++;

   if(!vbuf->vf) {
      const struct vertex_info *vinfo = vbuf->vinfo;
      uint i;
      uint count = 0;  /* for debug/sanity */
      
      assert(vinfo == vbuf->render->get_vertex_info(vbuf->render));

      for (i = 0; i < vinfo->num_attribs; i++) {
         uint j = vinfo->src_index[i];
         switch (vinfo->emit[i]) {
         case EMIT_OMIT:
            /* no-op */
            break;
         case EMIT_1F:
            *vbuf->vertex_ptr++ = fui(vertex->data[j][0]);
            count++;
            break;
         case EMIT_1F_PSIZE:
            *vbuf->vertex_ptr++ = fui(vbuf->stage.draw->rasterizer->point_size);
            count++;
            break;
         case EMIT_2F:
            *vbuf->vertex_ptr++ = fui(vertex->data[j][0]);
            *vbuf->vertex_ptr++ = fui(vertex->data[j][1]);
            count += 2;
            break;
         case EMIT_3F:
            *vbuf->vertex_ptr++ = fui(vertex->data[j][0]);
            *vbuf->vertex_ptr++ = fui(vertex->data[j][1]);
            *vbuf->vertex_ptr++ = fui(vertex->data[j][2]);
            count += 3;
            break;
         case EMIT_4F:
            *vbuf->vertex_ptr++ = fui(vertex->data[j][0]);
            *vbuf->vertex_ptr++ = fui(vertex->data[j][1]);
            *vbuf->vertex_ptr++ = fui(vertex->data[j][2]);
            *vbuf->vertex_ptr++ = fui(vertex->data[j][3]);
            count += 4;
            break;
         case EMIT_4UB:
            *vbuf->vertex_ptr++ = pack_ub4(float_to_ubyte( vertex->data[j][2] ),
                                           float_to_ubyte( vertex->data[j][1] ),
                                           float_to_ubyte( vertex->data[j][0] ),
                                           float_to_ubyte( vertex->data[j][3] ));
            count += 1;
            break;
         default:
            assert(0);
         }
      }
      assert(count == vinfo->size);
#if 0
      {
	 static float data[256]; 
	 draw_vf_emit_vertex(vbuf->vf, vertex, data);
	 if(memcmp((uint8_t *)vbuf->vertex_ptr - vbuf->vertex_size, data, vbuf->vertex_size)) {
            debug_printf("With VF:\n");
            dump_emitted_vertex(vbuf->vinfo, (uint8_t *)data);
	    debug_printf("Without VF:\n");
	    dump_emitted_vertex(vbuf->vinfo, (uint8_t *)vbuf->vertex_ptr - vbuf->vertex_size);
	    assert(0);
	 }
      }
#endif
   }
   else {
      draw_vf_emit_vertex(vbuf->vf, vertex, vbuf->vertex_ptr);
   
      vbuf->vertex_ptr += vbuf->vertex_size/4;
   }
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


/**
 * Set the prim type for subsequent vertices.
 * This may result in a new vertex size.  The existing vbuffer (if any)
 * will be flushed if needed and a new one allocated.
 */
static void
vbuf_set_prim( struct vbuf_stage *vbuf, uint newprim )
{
   const struct vertex_info *vinfo;
   unsigned vertex_size;

   assert(newprim == PIPE_PRIM_POINTS ||
          newprim == PIPE_PRIM_LINES ||
          newprim == PIPE_PRIM_TRIANGLES);

   vbuf->prim = newprim;
   vbuf->render->set_primitive(vbuf->render, newprim);

   vinfo = vbuf->render->get_vertex_info(vbuf->render);
   vertex_size = vinfo->size * sizeof(float);

   if (vertex_size != vbuf->vertex_size)
      vbuf_flush_vertices(vbuf);

   vbuf->vinfo = vinfo;
   vbuf->vertex_size = vertex_size;
   if(vbuf->vf)
      draw_vf_set_vertex_info(vbuf->vf, 
                              vbuf->vinfo,
                              vbuf->stage.draw->rasterizer->point_size);
   
   if (!vbuf->vertices)
      vbuf_alloc_vertices(vbuf);
}


static void 
vbuf_first_tri( struct draw_stage *stage,
                struct prim_header *prim )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   vbuf_flush_indices( vbuf );   
   stage->tri = vbuf_tri;
   vbuf_set_prim(vbuf, PIPE_PRIM_TRIANGLES);
   stage->tri( stage, prim );
}


static void 
vbuf_first_line( struct draw_stage *stage,
                 struct prim_header *prim )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   vbuf_flush_indices( vbuf );
   stage->line = vbuf_line;
   vbuf_set_prim(vbuf, PIPE_PRIM_LINES);
   stage->line( stage, prim );
}


static void 
vbuf_first_point( struct draw_stage *stage,
                  struct prim_header *prim )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   vbuf_flush_indices( vbuf );
   stage->point = vbuf_point;
   vbuf_set_prim(vbuf, PIPE_PRIM_POINTS);
   stage->point( stage, prim );
}


static void 
vbuf_flush_indices( struct vbuf_stage *vbuf ) 
{
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

   /* don't need to reset point/line/tri functions */
#if 0
   stage->point = vbuf_first_point;
   stage->line = vbuf_first_line;
   stage->tri = vbuf_first_tri;
#endif
}


/**
 * Flush existing vertex buffer and allocate a new one.
 * 
 * XXX: We separate flush-on-index-full and flush-on-vb-full, but may 
 * raise issues uploading vertices if the hardware wants to flush when
 * we flush.
 */
static void 
vbuf_flush_vertices( struct vbuf_stage *vbuf )
{
   if(vbuf->vertices) {      
      vbuf_flush_indices(vbuf);
      
      /* Reset temporary vertices ids */
      if(vbuf->nr_vertices)
	 draw_reset_vertex_ids( vbuf->stage.draw );
      
      /* Free the vertex buffer */
      vbuf->render->release_vertices(vbuf->render,
                                     vbuf->vertices,
                                     vbuf->vertex_size,
                                     vbuf->nr_vertices);
      vbuf->max_vertices = vbuf->nr_vertices = 0;
      vbuf->vertex_ptr = vbuf->vertices = NULL;
      
   }
}
   

static void 
vbuf_alloc_vertices( struct vbuf_stage *vbuf )
{
   assert(!vbuf->nr_indices);
   assert(!vbuf->vertices);
   
   /* Allocate a new vertex buffer */
   vbuf->max_vertices = vbuf->render->max_vertex_buffer_bytes / vbuf->vertex_size;
   vbuf->vertices = (uint *) vbuf->render->allocate_vertices(vbuf->render,
                                                    (ushort) vbuf->vertex_size,
                                                    (ushort) vbuf->max_vertices);
   vbuf->vertex_ptr = vbuf->vertices;
}



static void 
vbuf_flush( struct draw_stage *stage, unsigned flags )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   vbuf_flush_indices( vbuf );

   stage->point = vbuf_first_point;
   stage->line = vbuf_first_line;
   stage->tri = vbuf_first_tri;

   if (flags & DRAW_FLUSH_BACKEND)
      vbuf_flush_vertices( vbuf );
}


static void 
vbuf_reset_stipple_counter( struct draw_stage *stage )
{
   /* XXX: Need to do something here for hardware with linestipple.
    */
   (void) stage;
}


static void vbuf_destroy( struct draw_stage *stage )
{
   struct vbuf_stage *vbuf = vbuf_stage( stage );

   if(vbuf->indices)
      align_free( vbuf->indices );
   
   if(vbuf->vf)
      draw_vf_destroy( vbuf->vf );

   if (vbuf->render)
      vbuf->render->destroy( vbuf->render );

   FREE( stage );
}


/**
 * Create a new primitive vbuf/render stage.
 */
struct draw_stage *draw_vbuf_stage( struct draw_context *draw,
                                    struct vbuf_render *render )
{
   struct vbuf_stage *vbuf = CALLOC_STRUCT(vbuf_stage);

   if(!vbuf)
      return NULL;
   
   vbuf->stage.draw = draw;
   vbuf->stage.point = vbuf_first_point;
   vbuf->stage.line = vbuf_first_line;
   vbuf->stage.tri = vbuf_first_tri;
   vbuf->stage.flush = vbuf_flush;
   vbuf->stage.reset_stipple_counter = vbuf_reset_stipple_counter;
   vbuf->stage.destroy = vbuf_destroy;
   
   vbuf->render = render;

   assert(render->max_indices < UNDEFINED_VERTEX_ID);
   vbuf->max_indices = render->max_indices;
   vbuf->indices = (ushort *)
      align_malloc( vbuf->max_indices * sizeof(vbuf->indices[0]), 16 );
   if(!vbuf->indices)
      vbuf_destroy(&vbuf->stage);
   
   vbuf->vertices = NULL;
   vbuf->vertex_ptr = vbuf->vertices;

   vbuf->prim = ~0;
   
   if(!GETENV("GALLIUM_NOVF"))
      vbuf->vf = draw_vf_create();
   
   return &vbuf->stage;
}
