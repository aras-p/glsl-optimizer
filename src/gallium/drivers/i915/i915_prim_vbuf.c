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


#include "draw/draw_context.h"
#include "draw/draw_vbuf.h"
#include "util/u_debug.h"
#include "util/u_inlines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_fifo.h"

#include "i915_context.h"
#include "i915_reg.h"
#include "i915_batch.h"
#include "i915_state.h"


#undef VBUF_USE_FIFO
#undef VBUF_MAP_BUFFER

/**
 * Primitive renderer for i915.
 */
struct i915_vbuf_render {
   struct vbuf_render base;

   struct i915_context *i915;

   /** Vertex size in bytes */
   size_t vertex_size;

   /** Software primitive */
   unsigned prim;

   /** Hardware primitive */
   unsigned hwprim;

   /** Genereate a vertex list */
   unsigned fallback;

   /* Stuff for the vbo */
   struct intel_buffer *vbo;
   size_t vbo_size; /**< current size of allocated buffer */
   size_t vbo_alloc_size; /**< minimum buffer size to allocate */
   size_t vbo_offset;
   void *vbo_ptr;
   size_t vbo_max_used;

#ifndef VBUF_MAP_BUFFER
   size_t map_used_start;
   size_t map_used_end;
   size_t map_size;
#endif

#ifdef VBUF_USE_FIFO
   /* Stuff for the pool */
   struct util_fifo *pool_fifo;
   unsigned pool_used;
   unsigned pool_buffer_size;
   boolean pool_not_used;
#endif
};


/**
 * Basically a cast wrapper.
 */
static INLINE struct i915_vbuf_render *
i915_vbuf_render(struct vbuf_render *render)
{
   assert(render);
   return (struct i915_vbuf_render *)render;
}

static const struct vertex_info *
i915_vbuf_render_get_vertex_info(struct vbuf_render *render)
{
   struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
   struct i915_context *i915 = i915_render->i915;

   if (i915->dirty) {
      /* make sure we have up to date vertex layout */
      i915_update_derived(i915);
   }

   return &i915->current.vertex_info;
}

static boolean
i915_vbuf_render_reserve(struct i915_vbuf_render *i915_render, size_t size)
{
   struct i915_context *i915 = i915_render->i915;

   if (i915_render->vbo_size < size + i915_render->vbo_offset)
      return FALSE;

   if (i915->vbo_flushed)
      return FALSE;

   return TRUE;
}

static void
i915_vbuf_render_new_buf(struct i915_vbuf_render *i915_render, size_t size)
{
   struct i915_context *i915 = i915_render->i915;
   struct intel_winsys *iws = i915->iws;

   if (i915_render->vbo) {
#ifdef VBUF_USE_FIFO
      if (i915_render->pool_not_used)
         iws->buffer_destroy(iws, i915_render->vbo);
      else
         u_fifo_add(i915_render->pool_fifo, i915_render->vbo);
      i915_render->vbo = NULL;
#else
      iws->buffer_destroy(iws, i915_render->vbo);
#endif
   }

   i915->vbo_flushed = 0;

   i915_render->vbo_size = MAX2(size, i915_render->vbo_alloc_size);
   i915_render->vbo_offset = 0;

#ifndef VBUF_MAP_BUFFER
   if (i915_render->vbo_size > i915_render->map_size) {
      i915_render->map_size = i915_render->vbo_size;
      FREE(i915_render->vbo_ptr);
      i915_render->vbo_ptr = MALLOC(i915_render->map_size);
   }
#endif

#ifdef VBUF_USE_FIFO
   if (i915_render->vbo_size != i915_render->pool_buffer_size) {
      i915_render->pool_not_used = TRUE;
      i915_render->vbo = iws->buffer_create(iws, i915_render->vbo_size, 64,
            INTEL_NEW_VERTEX);
   } else {
      i915_render->pool_not_used = FALSE;

      if (i915_render->pool_used >= 2) {
         FLUSH_BATCH(NULL);
         i915->vbo_flushed = 0;
         i915_render->pool_used = 0;
      }
      u_fifo_pop(i915_render->pool_fifo, (void**)&i915_render->vbo);
   }
#else
   i915_render->vbo = iws->buffer_create(iws, i915_render->vbo_size,
                                         64, INTEL_NEW_VERTEX);
#endif
}

static boolean
i915_vbuf_render_allocate_vertices(struct vbuf_render *render,
                                   ushort vertex_size,
                                   ushort nr_vertices)
{
   struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
   struct i915_context *i915 = i915_render->i915;
   size_t size = (size_t)vertex_size * (size_t)nr_vertices;

   /* FIXME: handle failure */
   assert(!i915->vbo);

   if (!i915_vbuf_render_reserve(i915_render, size)) {
#ifdef VBUF_USE_FIFO
      /* incase we flushed reset the number of pool buffers used */
      if (i915->vbo_flushed)
         i915_render->pool_used = 0;
#endif
      i915_vbuf_render_new_buf(i915_render, size);
   }

   i915_render->vertex_size = vertex_size;
   i915->vbo = i915_render->vbo;
   i915->vbo_offset = i915_render->vbo_offset;
   i915->dirty |= I915_NEW_VBO;

   if (!i915_render->vbo)
      return FALSE;
   return TRUE;
}

static void *
i915_vbuf_render_map_vertices(struct vbuf_render *render)
{
   struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
   struct i915_context *i915 = i915_render->i915;
   struct intel_winsys *iws = i915->iws;

   if (i915->vbo_flushed)
      debug_printf("%s bad vbo flush occured stalling on hw\n", __FUNCTION__);

#ifdef VBUF_MAP_BUFFER
   i915_render->vbo_ptr = iws->buffer_map(iws, i915_render->vbo, TRUE);
   return (unsigned char *)i915_render->vbo_ptr + i915_render->vbo_offset;
#else
   (void)iws;
   return (unsigned char *)i915_render->vbo_ptr;
#endif
}

static void
i915_vbuf_render_unmap_vertices(struct vbuf_render *render,
                                ushort min_index,
                                ushort max_index)
{
   struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
   struct i915_context *i915 = i915_render->i915;
   struct intel_winsys *iws = i915->iws;

   i915_render->vbo_max_used = MAX2(i915_render->vbo_max_used, i915_render->vertex_size * (max_index + 1));
#ifdef VBUF_MAP_BUFFER
   iws->buffer_unmap(iws, i915_render->vbo);
#else
   i915_render->map_used_start = i915_render->vertex_size * min_index;
   i915_render->map_used_end = i915_render->vertex_size * (max_index + 1);
   iws->buffer_write(iws, i915_render->vbo,
                     i915_render->map_used_start + i915_render->vbo_offset,
                     i915_render->map_used_end - i915_render->map_used_start,
                     (unsigned char *)i915_render->vbo_ptr + i915_render->map_used_start);

#endif
}

static boolean
i915_vbuf_render_set_primitive(struct vbuf_render *render, 
                               unsigned prim)
{
   struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
   i915_render->prim = prim;

   switch(prim) {
   case PIPE_PRIM_POINTS:
      i915_render->hwprim = PRIM3D_POINTLIST;
      i915_render->fallback = 0;
      return TRUE;
   case PIPE_PRIM_LINES:
      i915_render->hwprim = PRIM3D_LINELIST;
      i915_render->fallback = 0;
      return TRUE;
   case PIPE_PRIM_LINE_LOOP:
      i915_render->hwprim = PRIM3D_LINELIST;
      i915_render->fallback = PIPE_PRIM_LINE_LOOP;
      return TRUE;
   case PIPE_PRIM_LINE_STRIP:
      i915_render->hwprim = PRIM3D_LINESTRIP;
      i915_render->fallback = 0;
      return TRUE;
   case PIPE_PRIM_TRIANGLES:
      i915_render->hwprim = PRIM3D_TRILIST;
      i915_render->fallback = 0;
      return TRUE;
   case PIPE_PRIM_TRIANGLE_STRIP:
      i915_render->hwprim = PRIM3D_TRISTRIP;
      i915_render->fallback = 0;
      return TRUE;
   case PIPE_PRIM_TRIANGLE_FAN:
      i915_render->hwprim = PRIM3D_TRIFAN;
      i915_render->fallback = 0;
      return TRUE;
   case PIPE_PRIM_QUADS:
      i915_render->hwprim = PRIM3D_TRILIST;
      i915_render->fallback = PIPE_PRIM_QUADS;
      return TRUE;
   case PIPE_PRIM_QUAD_STRIP:
      i915_render->hwprim = PRIM3D_TRILIST;
      i915_render->fallback = PIPE_PRIM_QUAD_STRIP;
      return TRUE;
   case PIPE_PRIM_POLYGON:
      i915_render->hwprim = PRIM3D_POLY;
      i915_render->fallback = 0;
      return TRUE;
   default:
      /* FIXME: Actually, can handle a lot more just fine... */
      return FALSE;
   }
}

/**
 * Used for fallbacks in draw_arrays
 */
static void
draw_arrays_generate_indices(struct vbuf_render *render,
                             unsigned start, uint nr,
                             unsigned type)
{
   struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
   struct i915_context *i915 = i915_render->i915;
   unsigned i;
   unsigned end = start + nr;
   switch(type) {
   case 0:
      for (i = start; i+1 < end; i += 2)
         OUT_BATCH((i+0) | (i+1) << 16);
      if (i < end)
         OUT_BATCH(i);
      break;
   case PIPE_PRIM_LINE_LOOP:
      if (nr >= 2) {
         for (i = start + 1; i < end; i++)
            OUT_BATCH((i-0) | (i+0) << 16);
         OUT_BATCH((i-0) | ( start) << 16);
      }
      break;
   case PIPE_PRIM_QUADS:
      for (i = start; i + 3 < end; i += 4) {
         OUT_BATCH((i+0) | (i+1) << 16);
         OUT_BATCH((i+3) | (i+1) << 16);
         OUT_BATCH((i+2) | (i+3) << 16);
      }
      break;
   case PIPE_PRIM_QUAD_STRIP:
      for (i = start; i + 3 < end; i += 2) {
         OUT_BATCH((i+0) | (i+1) << 16);
         OUT_BATCH((i+3) | (i+2) << 16);
         OUT_BATCH((i+0) | (i+3) << 16);
      }
      break;
   default:
      assert(0);
   }
}

static unsigned
draw_arrays_calc_nr_indices(uint nr, unsigned type)
{
   switch (type) {
   case 0:
      return nr;
   case PIPE_PRIM_LINE_LOOP:
      if (nr >= 2)
         return nr * 2;
      else
         return 0;
   case PIPE_PRIM_QUADS:
      return (nr / 4) * 6;
   case PIPE_PRIM_QUAD_STRIP:
      return ((nr - 2) / 2) * 6;
   default:
      assert(0);
      return 0;
   }
}

static void
draw_arrays_fallback(struct vbuf_render *render,
                     unsigned start,
                     uint nr)
{
   struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
   struct i915_context *i915 = i915_render->i915;
   unsigned nr_indices;

   if (i915->dirty)
      i915_update_derived(i915);

   if (i915->hardware_dirty)
      i915_emit_hardware_state(i915);

   nr_indices = draw_arrays_calc_nr_indices(nr, i915_render->fallback);
   if (!nr_indices)
      return;

   if (!BEGIN_BATCH(1 + (nr_indices + 1)/2, 1)) {
      FLUSH_BATCH(NULL);

      /* Make sure state is re-emitted after a flush:
       */
      i915_update_derived(i915);
      i915_emit_hardware_state(i915);
      i915->vbo_flushed = 1;

      if (!BEGIN_BATCH(1 + (nr_indices + 1)/2, 1)) {
         assert(0);
         goto out;
      }
   }
   OUT_BATCH(_3DPRIMITIVE |
             PRIM_INDIRECT |
             i915_render->hwprim |
             PRIM_INDIRECT_ELTS |
             nr_indices);

   draw_arrays_generate_indices(render, start, nr, i915_render->fallback);

out:
   return;
}

static void
i915_vbuf_render_draw_arrays(struct vbuf_render *render,
                             unsigned start,
                             uint nr)
{
   struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
   struct i915_context *i915 = i915_render->i915;

   if (i915_render->fallback) {
      draw_arrays_fallback(render, start, nr);
      return;
   }

   if (i915->dirty)
      i915_update_derived(i915);

   if (i915->hardware_dirty)
      i915_emit_hardware_state(i915);

   if (!BEGIN_BATCH(2, 0)) {
      FLUSH_BATCH(NULL);

      /* Make sure state is re-emitted after a flush:
       */
      i915_update_derived(i915);
      i915_emit_hardware_state(i915);
      i915->vbo_flushed = 1;

      if (!BEGIN_BATCH(2, 0)) {
         assert(0);
         goto out;
      }
   }

   OUT_BATCH(_3DPRIMITIVE |
             PRIM_INDIRECT |
             PRIM_INDIRECT_SEQUENTIAL |
             i915_render->hwprim |
             nr);
   OUT_BATCH(start); /* Beginning vertex index */

out:
   return;
}

/**
 * Used for normal and fallback emitting of indices
 * If type is zero normal operation assumed.
 */
static void
draw_generate_indices(struct vbuf_render *render,
                      const ushort *indices,
                      uint nr_indices,
                      unsigned type)
{
   struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
   struct i915_context *i915 = i915_render->i915;
   unsigned i;

   switch(type) {
   case 0:
      for (i = 0; i + 1 < nr_indices; i += 2) {
         OUT_BATCH(indices[i] | indices[i+1] << 16);
      }
      if (i < nr_indices) {
         OUT_BATCH(indices[i]);
      }
      break;
   case PIPE_PRIM_LINE_LOOP:
      if (nr_indices >= 2) {
         for (i = 1; i < nr_indices; i++)
            OUT_BATCH(indices[i-1] | indices[i] << 16);
         OUT_BATCH(indices[i-1] | indices[0] << 16);
      }
      break;
   case PIPE_PRIM_QUADS:
      for (i = 0; i + 3 < nr_indices; i += 4) {
         OUT_BATCH(indices[i+0] | indices[i+1] << 16);
         OUT_BATCH(indices[i+3] | indices[i+1] << 16);
         OUT_BATCH(indices[i+2] | indices[i+3] << 16);
      }
      break;
   case PIPE_PRIM_QUAD_STRIP:
      for (i = 0; i + 3 < nr_indices; i += 2) {
         OUT_BATCH(indices[i+0] | indices[i+1] << 16);
         OUT_BATCH(indices[i+3] | indices[i+2] << 16);
         OUT_BATCH(indices[i+0] | indices[i+3] << 16);
      }
      break;
   default:
      assert(0);
      break;
   }
}

static unsigned
draw_calc_nr_indices(uint nr_indices, unsigned type)
{
   switch (type) {
   case 0:
      return nr_indices;
   case PIPE_PRIM_LINE_LOOP:
      if (nr_indices >= 2)
         return nr_indices * 2;
      else
         return 0;
   case PIPE_PRIM_QUADS:
      return (nr_indices / 4) * 6;
   case PIPE_PRIM_QUAD_STRIP:
      return ((nr_indices - 2) / 2) * 6;
   default:
      assert(0);
      return 0;
   }
}

static void 
i915_vbuf_render_draw(struct vbuf_render *render,
                      const ushort *indices,
                      uint nr_indices)
{
   struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
   struct i915_context *i915 = i915_render->i915;
   unsigned save_nr_indices;

   save_nr_indices = nr_indices;

   nr_indices = draw_calc_nr_indices(nr_indices, i915_render->fallback);
   if (!nr_indices)
      return;

   if (i915->dirty)
      i915_update_derived(i915);

   if (i915->hardware_dirty)
      i915_emit_hardware_state(i915);

   if (!BEGIN_BATCH(1 + (nr_indices + 1)/2, 1)) {
      FLUSH_BATCH(NULL);

      /* Make sure state is re-emitted after a flush: 
       */
      i915_update_derived(i915);
      i915_emit_hardware_state(i915);
      i915->vbo_flushed = 1;

      if (!BEGIN_BATCH(1 + (nr_indices + 1)/2, 1)) {
         assert(0);
         goto out;
      }
   }

   OUT_BATCH(_3DPRIMITIVE |
             PRIM_INDIRECT |
             i915_render->hwprim |
             PRIM_INDIRECT_ELTS |
             nr_indices);
   draw_generate_indices(render,
                         indices,
                         save_nr_indices,
                         i915_render->fallback);

out:
   return;
}

static void
i915_vbuf_render_release_vertices(struct vbuf_render *render)
{
   struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
   struct i915_context *i915 = i915_render->i915;

   assert(i915->vbo);

   i915_render->vbo_offset += i915_render->vbo_max_used;
   i915_render->vbo_max_used = 0;
   i915->vbo = NULL;
   i915->dirty |= I915_NEW_VBO;
}

static void
i915_vbuf_render_destroy(struct vbuf_render *render)
{
   struct i915_vbuf_render *i915_render = i915_vbuf_render(render);
   FREE(i915_render);
}

/**
 * Create a new primitive render.
 */
static struct vbuf_render *
i915_vbuf_render_create(struct i915_context *i915)
{
   struct i915_vbuf_render *i915_render = CALLOC_STRUCT(i915_vbuf_render);
   struct intel_winsys *iws = i915->iws;
   int i;

   i915_render->i915 = i915;

   i915_render->base.max_vertex_buffer_bytes = 16*4096;

   /* NOTE: it must be such that state and vertices indices fit in a single 
    * batch buffer.
    */
   i915_render->base.max_indices = 16*1024;

   i915_render->base.get_vertex_info = i915_vbuf_render_get_vertex_info;
   i915_render->base.allocate_vertices = i915_vbuf_render_allocate_vertices;
   i915_render->base.map_vertices = i915_vbuf_render_map_vertices;
   i915_render->base.unmap_vertices = i915_vbuf_render_unmap_vertices;
   i915_render->base.set_primitive = i915_vbuf_render_set_primitive;
   i915_render->base.draw = i915_vbuf_render_draw;
   i915_render->base.draw_arrays = i915_vbuf_render_draw_arrays;
   i915_render->base.release_vertices = i915_vbuf_render_release_vertices;
   i915_render->base.destroy = i915_vbuf_render_destroy;

#ifndef VBUF_MAP_BUFFER
   i915_render->map_size = 0;
   i915_render->map_used_start = 0;
   i915_render->map_used_end = 0;
#endif

   i915_render->vbo = NULL;
   i915_render->vbo_ptr = NULL;
   i915_render->vbo_size = 0;
   i915_render->vbo_offset = 0;
   i915_render->vbo_alloc_size = i915_render->base.max_vertex_buffer_bytes * 4;

#ifdef VBUF_USE_POOL
   i915_render->pool_used = FALSE;
   i915_render->pool_buffer_size = i915_render->vbo_alloc_size;
   i915_render->pool_fifo = u_fifo_create(6);
   for (i = 0; i < 6; i++)
      u_fifo_add(i915_render->pool_fifo,
                 iws->buffer_create(iws, i915_render->pool_buffer_size, 64,
                                    INTEL_NEW_VERTEX));
#else
   (void)i;
   (void)iws;
#endif

   return &i915_render->base;
}

/**
 * Create a new primitive vbuf/render stage.
 */
struct draw_stage *i915_draw_vbuf_stage(struct i915_context *i915)
{
   struct vbuf_render *render;
   struct draw_stage *stage;
   
   render = i915_vbuf_render_create(i915);
   if(!render)
      return NULL;
   
   stage = draw_vbuf_stage(i915->draw, render);
   if(!stage) {
      render->destroy(render);
      return NULL;
   }
   /** TODO JB: this shouldn't be here */
   draw_set_render(i915->draw, render);

   return stage;
}
