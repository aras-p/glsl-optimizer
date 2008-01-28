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
#include <stddef.h>

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
   const struct vertex_info *vinfo = vbuf->vinfo;

   uint i;
   uint count = 0;  /* for debug/sanity */
   
   assert(vinfo == vbuf->render->get_vertex_info(vbuf->render));

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
      uint j = vinfo->src_index[i];
      switch (vinfo->emit[i]) {
      case EMIT_OMIT:
         /* no-op */
         break;
      case EMIT_ALL:
         /* just copy the whole vertex as-is to the vbuf */
         assert(i == 0);
         assert(j == 0);
         memcpy(vbuf->vertex_ptr, vertex, vinfo->size * 4);
         vbuf->vertex_ptr += vinfo->size;
         count += vinfo->size;
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
#else
   if(vertex->vertex_id != UNDEFINED_VERTEX_ID) {
      if(vertex->vertex_id < vbuf->nr_vertices)
	 return;
      else
	 fprintf(stderr, "Bad vertex id 0x%04x (>= 0x%04x)\n", 
	         vertex->vertex_id, vbuf->nr_vertices);
      return;
   }
      
   vertex->vertex_id = vbuf->nr_vertices++;

   draw_vf_set_data(vbuf->vf, vertex->data);
   draw_vf_emit_vertices(vbuf->vf, 1, vbuf->vertex_ptr);

   vbuf->vertex_ptr += vbuf->vertex_size/4;
#endif
}


static void
vbuf_set_vf_attributes(struct vbuf_stage *vbuf ) 
{
   const struct vertex_info *vinfo = vbuf->vinfo;
   struct draw_vf_attr_map attrs[PIPE_MAX_SHADER_INPUTS];
   uint i;
   uint count = 0;  /* for debug/sanity */
   unsigned nr_attrs = 0;
   
//   fprintf(stderr, "emit vertex %d to %p\n", 
//           vbuf->nr_vertices, vbuf->vertex_ptr);

#if 0
   if(vertex->vertex_id != UNDEFINED_VERTEX_ID) {
      if(vertex->vertex_id < vbuf->nr_vertices)
	 return;
      else
	 fprintf(stderr, "Bad vertex id 0x%04x (>= 0x%04x)\n", 
	         vertex->vertex_id, vbuf->nr_vertices);
      return;
   }
#endif
   
   for (i = 0; i < vinfo->num_attribs; i++) {
      uint j = vinfo->src_index[i];
      switch (vinfo->emit[i]) {
      case EMIT_OMIT:
         /* no-op */
         break;
      case EMIT_ALL: {
         /* just copy the whole vertex as-is to the vbuf */
	 unsigned k, s = vinfo->size;
         assert(i == 0);
         assert(j == 0);
         /* copy the vertex header */
         /* XXX: we actually don't copy the header, just pad it */
	 attrs[nr_attrs].attrib = 0;
	 attrs[nr_attrs].format = DRAW_EMIT_PAD;
	 attrs[nr_attrs].offset = offsetof(struct vertex_header, data);
	 s -= offsetof(struct vertex_header, data)/4;
         count += offsetof(struct vertex_header, data)/4;
	 nr_attrs++;
	 /* copy the vertex data */
         for(k = 0; k < (s & ~0x3); k += 4) {
      	    attrs[nr_attrs].attrib = k/4;
      	    attrs[nr_attrs].format = DRAW_EMIT_4F;
      	    attrs[nr_attrs].offset = 0;
      	    nr_attrs++;
            count += 4;
         }
         /* tail */
         /* XXX: actually, this shouldn't be needed */
 	 attrs[nr_attrs].attrib = k/4;
  	 attrs[nr_attrs].offset = 0;
         switch(s & 0x3) {
         case 0:
            break;
         case 1:
      	    attrs[nr_attrs].format = DRAW_EMIT_1F;
      	    nr_attrs++;
            count += 1;
            break;
         case 2:
      	    attrs[nr_attrs].format = DRAW_EMIT_2F;
      	    nr_attrs++;
            count += 2;
            break;
         case 3:
      	    attrs[nr_attrs].format = DRAW_EMIT_3F;
      	    nr_attrs++;
            count += 3;
            break;
         }
         break;
      }
      case EMIT_1F:
	 attrs[nr_attrs].attrib = j;
	 attrs[nr_attrs].format = DRAW_EMIT_1F;
	 attrs[nr_attrs].offset = 0;
	 nr_attrs++;
         count++;
         break;
      case EMIT_1F_PSIZE:
	 /* FIXME */
	 assert(0);
	 attrs[nr_attrs].attrib = j;
	 attrs[nr_attrs].format = DRAW_EMIT_PAD;
	 attrs[nr_attrs].offset = 0;
	 nr_attrs++;
         count++;
         break;
      case EMIT_2F:
	 attrs[nr_attrs].attrib = j;
	 attrs[nr_attrs].format = DRAW_EMIT_2F;
	 attrs[nr_attrs].offset = 0;
	 nr_attrs++;
         count += 2;
         break;
      case EMIT_3F:
	 attrs[nr_attrs].attrib = j;
	 attrs[nr_attrs].format = DRAW_EMIT_3F;
	 attrs[nr_attrs].offset = 0;
	 nr_attrs++;
         count += 3;
         break;
      case EMIT_4F:
	 attrs[nr_attrs].attrib = j;
	 attrs[nr_attrs].format = DRAW_EMIT_4F;
	 attrs[nr_attrs].offset = 0;
	 nr_attrs++;
         count += 4;
         break;
      case EMIT_4UB:
	 attrs[nr_attrs].attrib = j;
	 attrs[nr_attrs].format = DRAW_EMIT_4UB_4F_BGRA;
	 attrs[nr_attrs].offset = 0;
	 nr_attrs++;
         count += 1;
         break;
      default:
         assert(0);
      }
   }
   
   assert(count == vinfo->size);  
   
   draw_vf_set_vertex_attributes(vbuf->vf, 
                                 attrs, 
                                 nr_attrs, 
                                 vbuf->vertex_size);
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
   vbuf_set_vf_attributes(vbuf);
   
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
   
   vbuf->vf = draw_vf_create();
   if(!vbuf->vf)
      vbuf_destroy(&vbuf->stage);
   
   return &vbuf->stage;
}
