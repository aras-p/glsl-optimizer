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
 * Interface between 'draw' module's output and the softpipe rasterizer/setup
 * code.  When the 'draw' module has finished filling a vertex buffer, the
 * draw_arrays() functions below will be called.  Loop over the vertices and
 * call the point/line/tri setup functions.
 *
 * Authors
 *  Brian Paul
 */


#include "sp_context.h"
#include "sp_setup.h"
#include "sp_state.h"
#include "sp_prim_vbuf.h"
#include "draw/draw_context.h"
#include "draw/draw_vbuf.h"
#include "util/u_memory.h"
#include "util/u_prim.h"


#define SP_MAX_VBUF_INDEXES 1024
#define SP_MAX_VBUF_SIZE    4096

typedef const float (*cptrf4)[4];

/**
 * Subclass of vbuf_render.
 */
struct softpipe_vbuf_render
{
   struct vbuf_render base;
   struct softpipe_context *softpipe;
   struct setup_context *setup;

   uint prim;
   uint vertex_size;
   uint nr_vertices;
   uint vertex_buffer_size;
   void *vertex_buffer;
};


/** cast wrapper */
static struct softpipe_vbuf_render *
softpipe_vbuf_render(struct vbuf_render *vbr)
{
   return (struct softpipe_vbuf_render *) vbr;
}







static const struct vertex_info *
sp_vbuf_get_vertex_info(struct vbuf_render *vbr)
{
   struct softpipe_vbuf_render *cvbr = softpipe_vbuf_render(vbr);
   return softpipe_get_vbuf_vertex_info(cvbr->softpipe);
}


static boolean
sp_vbuf_allocate_vertices(struct vbuf_render *vbr,
                          ushort vertex_size, ushort nr_vertices)
{
   struct softpipe_vbuf_render *cvbr = softpipe_vbuf_render(vbr);
   unsigned size = vertex_size * nr_vertices;

   if (cvbr->vertex_buffer_size < size) {
      align_free(cvbr->vertex_buffer);
      cvbr->vertex_buffer = align_malloc(size, 16);
      cvbr->vertex_buffer_size = size;
   }

   cvbr->vertex_size = vertex_size;
   cvbr->nr_vertices = nr_vertices;
   
   return cvbr->vertex_buffer != NULL;
}

static void
sp_vbuf_release_vertices(struct vbuf_render *vbr)
{
   /* keep the old allocation for next time */
}

static void *
sp_vbuf_map_vertices(struct vbuf_render *vbr)
{
   struct softpipe_vbuf_render *cvbr = softpipe_vbuf_render(vbr);
   return cvbr->vertex_buffer;
}

static void 
sp_vbuf_unmap_vertices(struct vbuf_render *vbr, 
                       ushort min_index,
                       ushort max_index )
{
   struct softpipe_vbuf_render *cvbr = softpipe_vbuf_render(vbr);
   assert( cvbr->vertex_buffer_size >= (max_index+1) * cvbr->vertex_size );
   (void) cvbr;
   /* do nothing */
}


static boolean
sp_vbuf_set_primitive(struct vbuf_render *vbr, unsigned prim)
{
   struct softpipe_vbuf_render *cvbr = softpipe_vbuf_render(vbr);
   struct setup_context *setup_ctx = cvbr->setup;
   
   sp_setup_prepare( setup_ctx );

   cvbr->softpipe->reduced_prim = u_reduced_prim(prim);
   cvbr->prim = prim;
   return TRUE;

}


static INLINE cptrf4 get_vert( const void *vertex_buffer,
                               int index,
                               int stride )
{
   return (cptrf4)((char *)vertex_buffer + index * stride);
}


/**
 * draw elements / indexed primitives
 */
static void
sp_vbuf_draw(struct vbuf_render *vbr, const ushort *indices, uint nr)
{
   struct softpipe_vbuf_render *cvbr = softpipe_vbuf_render(vbr);
   struct softpipe_context *softpipe = cvbr->softpipe;
   const unsigned stride = softpipe->vertex_info_vbuf.size * sizeof(float);
   const void *vertex_buffer = cvbr->vertex_buffer;
   struct setup_context *setup_ctx = cvbr->setup;
   unsigned i;

   switch (cvbr->prim) {
   case PIPE_PRIM_POINTS:
      for (i = 0; i < nr; i++) {
         sp_setup_point( setup_ctx,
                      get_vert(vertex_buffer, indices[i-0], stride) );
      }
      break;

   case PIPE_PRIM_LINES:
      for (i = 1; i < nr; i += 2) {
         sp_setup_line( setup_ctx,
                     get_vert(vertex_buffer, indices[i-1], stride),
                     get_vert(vertex_buffer, indices[i-0], stride) );
      }
      break;

   case PIPE_PRIM_LINE_STRIP:
      for (i = 1; i < nr; i ++) {
         sp_setup_line( setup_ctx,
                     get_vert(vertex_buffer, indices[i-1], stride),
                     get_vert(vertex_buffer, indices[i-0], stride) );
      }
      break;

   case PIPE_PRIM_LINE_LOOP:
      for (i = 1; i < nr; i ++) {
         sp_setup_line( setup_ctx,
                     get_vert(vertex_buffer, indices[i-1], stride),
                     get_vert(vertex_buffer, indices[i-0], stride) );
      }
      if (nr) {
         sp_setup_line( setup_ctx,
                     get_vert(vertex_buffer, indices[nr-1], stride),
                     get_vert(vertex_buffer, indices[0], stride) );
      }
      break;

   case PIPE_PRIM_TRIANGLES:
      if (softpipe->rasterizer->flatshade_first) {
         for (i = 2; i < nr; i += 3) {
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i-1], stride),
                       get_vert(vertex_buffer, indices[i-0], stride),
                       get_vert(vertex_buffer, indices[i-2], stride) );
         }
      }
      else {
         for (i = 2; i < nr; i += 3) {
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i-2], stride),
                       get_vert(vertex_buffer, indices[i-1], stride),
                       get_vert(vertex_buffer, indices[i-0], stride) );
         }
      }
      break;

   case PIPE_PRIM_TRIANGLE_STRIP:
      if (softpipe->rasterizer->flatshade_first) {
         for (i = 2; i < nr; i += 1) {
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i+(i&1)-1], stride),
                       get_vert(vertex_buffer, indices[i-(i&1)], stride),
                       get_vert(vertex_buffer, indices[i-2], stride) );
         }
      }
      else {
         for (i = 2; i < nr; i += 1) {
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i+(i&1)-2], stride),
                       get_vert(vertex_buffer, indices[i-(i&1)-1], stride),
                       get_vert(vertex_buffer, indices[i-0], stride) );
         }
      }
      break;

   case PIPE_PRIM_TRIANGLE_FAN:
      if (softpipe->rasterizer->flatshade_first) {
         for (i = 2; i < nr; i += 1) {
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i-0], stride),
                       get_vert(vertex_buffer, indices[0], stride),
                       get_vert(vertex_buffer, indices[i-1], stride) );
         }
      }
      else {
         for (i = 2; i < nr; i += 1) {
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[0], stride),
                       get_vert(vertex_buffer, indices[i-1], stride),
                       get_vert(vertex_buffer, indices[i-0], stride) );
         }
      }
      break;

   case PIPE_PRIM_QUADS:
      if (softpipe->rasterizer->flatshade_first) {
         for (i = 3; i < nr; i += 4) {
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i-2], stride),
                       get_vert(vertex_buffer, indices[i-1], stride),
                       get_vert(vertex_buffer, indices[i-3], stride) );
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i-1], stride),
                       get_vert(vertex_buffer, indices[i-0], stride),
                       get_vert(vertex_buffer, indices[i-3], stride) );
         }
      }
      else {
         for (i = 3; i < nr; i += 4) {
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i-3], stride),
                       get_vert(vertex_buffer, indices[i-2], stride),
                       get_vert(vertex_buffer, indices[i-0], stride) );

            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i-2], stride),
                       get_vert(vertex_buffer, indices[i-1], stride),
                       get_vert(vertex_buffer, indices[i-0], stride) );
         }
      }
      break;

   case PIPE_PRIM_QUAD_STRIP:
      if (softpipe->rasterizer->flatshade_first) {
         for (i = 3; i < nr; i += 2) {
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i-0], stride),
                       get_vert(vertex_buffer, indices[i-1], stride),
                       get_vert(vertex_buffer, indices[i-3], stride));
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i-2], stride),
                       get_vert(vertex_buffer, indices[i-0], stride),
                       get_vert(vertex_buffer, indices[i-3], stride) );
         }
      }
      else {
         for (i = 3; i < nr; i += 2) {
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i-3], stride),
                       get_vert(vertex_buffer, indices[i-2], stride),
                       get_vert(vertex_buffer, indices[i-0], stride) );
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i-1], stride),
                       get_vert(vertex_buffer, indices[i-3], stride),
                       get_vert(vertex_buffer, indices[i-0], stride) );
         }
      }
      break;

   case PIPE_PRIM_POLYGON:
      /* Almost same as tri fan but the _first_ vertex specifies the flat
       * shading color.  Note that the first polygon vertex is passed as
       * the last triangle vertex here.
       * flatshade_first state makes no difference.
       */
      for (i = 2; i < nr; i += 1) {
         sp_setup_tri( setup_ctx,
                    get_vert(vertex_buffer, indices[i-0], stride),
                    get_vert(vertex_buffer, indices[i-1], stride),
                    get_vert(vertex_buffer, indices[0], stride) );
      }
      break;

   default:
      assert(0);
   }
}


/**
 * This function is hit when the draw module is working in pass-through mode.
 * It's up to us to convert the vertex array into point/line/tri prims.
 */
static void
sp_vbuf_draw_arrays(struct vbuf_render *vbr, uint start, uint nr)
{
   struct softpipe_vbuf_render *cvbr = softpipe_vbuf_render(vbr);
   struct softpipe_context *softpipe = cvbr->softpipe;
   struct setup_context *setup_ctx = cvbr->setup;
   const unsigned stride = softpipe->vertex_info_vbuf.size * sizeof(float);
   const void *vertex_buffer =
      (void *) get_vert(cvbr->vertex_buffer, start, stride);
   unsigned i;

   switch (cvbr->prim) {
   case PIPE_PRIM_POINTS:
      for (i = 0; i < nr; i++) {
         sp_setup_point( setup_ctx,
                      get_vert(vertex_buffer, i-0, stride) );
      }
      break;

   case PIPE_PRIM_LINES:
      for (i = 1; i < nr; i += 2) {
         sp_setup_line( setup_ctx,
                     get_vert(vertex_buffer, i-1, stride),
                     get_vert(vertex_buffer, i-0, stride) );
      }
      break;

   case PIPE_PRIM_LINE_STRIP:
      for (i = 1; i < nr; i ++) {
         sp_setup_line( setup_ctx,
                     get_vert(vertex_buffer, i-1, stride),
                     get_vert(vertex_buffer, i-0, stride) );
      }
      break;

   case PIPE_PRIM_LINE_LOOP:
      for (i = 1; i < nr; i ++) {
         sp_setup_line( setup_ctx,
                     get_vert(vertex_buffer, i-1, stride),
                     get_vert(vertex_buffer, i-0, stride) );
      }
      if (nr) {
         sp_setup_line( setup_ctx,
                     get_vert(vertex_buffer, nr-1, stride),
                     get_vert(vertex_buffer, 0, stride) );
      }
      break;

   case PIPE_PRIM_TRIANGLES:
      if (softpipe->rasterizer->flatshade_first) {
         for (i = 2; i < nr; i += 3) {
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i-1, stride),
                       get_vert(vertex_buffer, i-0, stride),
                       get_vert(vertex_buffer, i-2, stride) );
         }
      }
      else {
         for (i = 2; i < nr; i += 3) {
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i-2, stride),
                       get_vert(vertex_buffer, i-1, stride),
                       get_vert(vertex_buffer, i-0, stride) );
         }
      }
      break;

   case PIPE_PRIM_TRIANGLE_STRIP:
      if (softpipe->rasterizer->flatshade_first) {
         for (i = 2; i < nr; i++) {
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i+(i&1)-1, stride),
                       get_vert(vertex_buffer, i-(i&1), stride),
                       get_vert(vertex_buffer, i-2, stride) );
         }
      }
      else {
         for (i = 2; i < nr; i++) {
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i+(i&1)-2, stride),
                       get_vert(vertex_buffer, i-(i&1)-1, stride),
                       get_vert(vertex_buffer, i-0, stride) );
         }
      }
      break;

   case PIPE_PRIM_TRIANGLE_FAN:
      if (softpipe->rasterizer->flatshade_first) {
         for (i = 2; i < nr; i += 1) {
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i-0, stride),
                       get_vert(vertex_buffer, 0, stride),
                       get_vert(vertex_buffer, i-1, stride) );
         }
      }
      else {
         for (i = 2; i < nr; i += 1) {
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, 0, stride),
                       get_vert(vertex_buffer, i-1, stride),
                       get_vert(vertex_buffer, i-0, stride) );
         }
      }
      break;

   case PIPE_PRIM_QUADS:
      if (softpipe->rasterizer->flatshade_first) {
         for (i = 3; i < nr; i += 4) {
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i-2, stride),
                       get_vert(vertex_buffer, i-1, stride),
                       get_vert(vertex_buffer, i-3, stride) );
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i-1, stride),
                       get_vert(vertex_buffer, i-0, stride),
                       get_vert(vertex_buffer, i-3, stride) );
         }
      }
      else {
         for (i = 3; i < nr; i += 4) {
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i-3, stride),
                       get_vert(vertex_buffer, i-2, stride),
                       get_vert(vertex_buffer, i-0, stride) );
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i-2, stride),
                       get_vert(vertex_buffer, i-1, stride),
                       get_vert(vertex_buffer, i-0, stride) );
         }
      }
      break;

   case PIPE_PRIM_QUAD_STRIP:
      if (softpipe->rasterizer->flatshade_first) {
         for (i = 3; i < nr; i += 2) {
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i-0, stride),
                       get_vert(vertex_buffer, i-1, stride),
                       get_vert(vertex_buffer, i-3, stride) );
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i-2, stride),
                       get_vert(vertex_buffer, i-0, stride),
                       get_vert(vertex_buffer, i-3, stride) );
         }
      }
      else {
         for (i = 3; i < nr; i += 2) {
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i-3, stride),
                       get_vert(vertex_buffer, i-2, stride),
                       get_vert(vertex_buffer, i-0, stride) );
            sp_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i-1, stride),
                       get_vert(vertex_buffer, i-3, stride),
                       get_vert(vertex_buffer, i-0, stride) );
         }
      }
      break;

   case PIPE_PRIM_POLYGON:
      /* Almost same as tri fan but the _first_ vertex specifies the flat
       * shading color.  Note that the first polygon vertex is passed as
       * the last triangle vertex here.
       * flatshade_first state makes no difference.
       */
      for (i = 2; i < nr; i += 1) {
         sp_setup_tri( setup_ctx,
                    get_vert(vertex_buffer, i-1, stride),
                    get_vert(vertex_buffer, i-0, stride),
                    get_vert(vertex_buffer, 0, stride) );
      }
      break;

   default:
      assert(0);
   }
}



static void
sp_vbuf_destroy(struct vbuf_render *vbr)
{
   struct softpipe_vbuf_render *cvbr = softpipe_vbuf_render(vbr);
   if(cvbr->vertex_buffer)
      align_free(cvbr->vertex_buffer);
   sp_setup_destroy_context(cvbr->setup);
   FREE(cvbr);
}


/**
 * Create the post-transform vertex handler for the given context.
 */
struct vbuf_render *
sp_create_vbuf_backend(struct softpipe_context *sp)
{
   struct softpipe_vbuf_render *cvbr = CALLOC_STRUCT(softpipe_vbuf_render);

   assert(sp->draw);

   cvbr->base.max_indices = SP_MAX_VBUF_INDEXES;
   cvbr->base.max_vertex_buffer_bytes = SP_MAX_VBUF_SIZE;

   cvbr->base.get_vertex_info = sp_vbuf_get_vertex_info;
   cvbr->base.allocate_vertices = sp_vbuf_allocate_vertices;
   cvbr->base.map_vertices = sp_vbuf_map_vertices;
   cvbr->base.unmap_vertices = sp_vbuf_unmap_vertices;
   cvbr->base.set_primitive = sp_vbuf_set_primitive;
   cvbr->base.draw = sp_vbuf_draw;
   cvbr->base.draw_arrays = sp_vbuf_draw_arrays;
   cvbr->base.release_vertices = sp_vbuf_release_vertices;
   cvbr->base.destroy = sp_vbuf_destroy;

   cvbr->softpipe = sp;

   cvbr->setup = sp_setup_create_context(cvbr->softpipe);

   return &cvbr->base;
}
