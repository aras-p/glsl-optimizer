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


#include "draw/draw_vbuf.h"
#include "pipe/p_debug.h"
#include "pipe/p_util.h"
#include "pipe/p_inlines.h"
#include "pipe/p_winsys.h"

#include "i915_context.h"
#include "i915_reg.h"
#include "i915_winsys.h"
#include "i915_batch.h"
#include "i915_state.h"


/**
 * Primitive renderer for i915.
 */
struct i915_vbuf_render {
   struct vbuf_render base;

   struct i915_context *i915;   

   /** Vertex size in bytes */
   unsigned vertex_size;

   /** Hardware primitive */
   unsigned hwprim;
};


/**
 * Basically a cast wrapper.
 */
static INLINE struct i915_vbuf_render *
i915_vbuf_render( struct vbuf_render *render )
{
   assert(render);
   return (struct i915_vbuf_render *)render;
}


static const struct vertex_info *
i915_vbuf_render_get_vertex_info( struct vbuf_render *render )
{
   struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
   struct i915_context *i915 = i915_render->i915;

   if (i915->dirty) {
      /* make sure we have up to date vertex layout */
      i915_update_derived( i915 );
   }

   return &i915->current.vertex_info;
}


static void *
i915_vbuf_render_allocate_vertices( struct vbuf_render *render,
			            ushort vertex_size,
				    ushort nr_vertices )
{
   struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
   struct i915_context *i915 = i915_render->i915;
   struct pipe_winsys *winsys = i915->pipe.winsys;
   size_t size = (size_t)vertex_size * (size_t)nr_vertices;

   /* FIXME: handle failure */
   assert(!i915->vbo);
   i915->vbo = winsys->buffer_create(winsys, 64, I915_BUFFER_USAGE_LIT_VERTEX,
                                     size);
   
   i915->dirty |= I915_NEW_VBO;
   
   return winsys->buffer_map(winsys, 
                             i915->vbo, 
                             PIPE_BUFFER_USAGE_CPU_WRITE);
}


static boolean
i915_vbuf_render_set_primitive( struct vbuf_render *render, 
                                unsigned prim )
{
   struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
   
   switch(prim) {
   case PIPE_PRIM_POINTS:
      i915_render->hwprim = PRIM3D_POINTLIST;
      return TRUE;
   case PIPE_PRIM_LINES:
      i915_render->hwprim = PRIM3D_LINELIST;
      return TRUE;
   case PIPE_PRIM_TRIANGLES:
      i915_render->hwprim = PRIM3D_TRILIST;
      return TRUE;
   default:
      /* Actually, can handle a lot more just fine...  Fixme.
       */
      return FALSE;
   }
}


static void 
i915_vbuf_render_draw( struct vbuf_render *render,
                       const ushort *indices,
                       uint nr_indices)
{
   struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
   struct i915_context *i915 = i915_render->i915;
   unsigned i;

   assert(nr_indices);

   /* this seems to be bogus, since we validate state right after this */
   /*assert((i915->dirty & ~I915_NEW_VBO) == 0);*/
   
   if (i915->dirty)
      i915_update_derived( i915 );

   if (i915->hardware_dirty)
      i915_emit_hardware_state( i915 );

   if (!BEGIN_BATCH( 1 + (nr_indices + 1)/2, 1 )) {
      FLUSH_BATCH();

      /* Make sure state is re-emitted after a flush: 
       */
      i915_update_derived( i915 );
      i915_emit_hardware_state( i915 );

      if (!BEGIN_BATCH( 1 + (nr_indices + 1)/2, 1 )) {
	 assert(0);
	 return;
      }
   }

   OUT_BATCH( _3DPRIMITIVE |
              PRIM_INDIRECT |
              i915_render->hwprim |
   	      PRIM_INDIRECT_ELTS |
   	      nr_indices );
   for (i = 0; i + 1 < nr_indices; i += 2) {
      OUT_BATCH( indices[i] |
                 (indices[i + 1] << 16) );
   }
   if (i < nr_indices) {
      OUT_BATCH( indices[i] );
   }
}


static void
i915_vbuf_render_release_vertices( struct vbuf_render *render,
			           void *vertices, 
			           unsigned vertex_size,
			           unsigned vertices_used )
{
   struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
   struct i915_context *i915 = i915_render->i915;
   struct pipe_winsys *winsys = i915->pipe.winsys;

   assert(i915->vbo);
   winsys->buffer_unmap(winsys, i915->vbo);
   pipe_buffer_reference(winsys, &i915->vbo, NULL);
}


static void
i915_vbuf_render_destroy( struct vbuf_render *render )
{
   struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
   FREE(i915_render);
}


/**
 * Create a new primitive render.
 */
static struct vbuf_render *
i915_vbuf_render_create( struct i915_context *i915 )
{
   struct i915_vbuf_render *i915_render = CALLOC_STRUCT(i915_vbuf_render);

   i915_render->i915 = i915;
   
   i915_render->base.max_vertex_buffer_bytes = 128*1024;
   
   /* NOTE: it must be such that state and vertices indices fit in a single 
    * batch buffer.
    */
   i915_render->base.max_indices = 16*1024;
   
   i915_render->base.get_vertex_info = i915_vbuf_render_get_vertex_info;
   i915_render->base.allocate_vertices = i915_vbuf_render_allocate_vertices;
   i915_render->base.set_primitive = i915_vbuf_render_set_primitive;
   i915_render->base.draw = i915_vbuf_render_draw;
   i915_render->base.release_vertices = i915_vbuf_render_release_vertices;
   i915_render->base.destroy = i915_vbuf_render_destroy;
   
   return &i915_render->base;
}


/**
 * Create a new primitive vbuf/render stage.
 */
struct draw_stage *i915_draw_vbuf_stage( struct i915_context *i915 )
{
   struct vbuf_render *render;
   struct draw_stage *stage;
   
   render = i915_vbuf_render_create(i915);
   if(!render)
      return NULL;
   
   stage = draw_vbuf_stage( i915->draw, render );
   if(!stage) {
      render->destroy(render);
      return NULL;
   }
    
   return stage;
}
