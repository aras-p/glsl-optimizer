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
 * Interface between 'draw' module's output and the llvmpipe rasterizer/setup
 * code.  When the 'draw' module has finished filling a vertex buffer, the
 * draw_arrays() functions below will be called.  Loop over the vertices and
 * call the point/line/tri setup functions.
 *
 * Authors
 *  Brian Paul
 */


#include "lp_context.h"
#include "lp_setup.h"
#include "lp_state.h"
#include "lp_prim_vbuf.h"
#include "draw/draw_context.h"
#include "draw/draw_vbuf.h"
#include "util/u_memory.h"
#include "util/u_prim.h"


#define LP_MAX_VBUF_INDEXES 1024
#define LP_MAX_VBUF_SIZE    4096

typedef const float (*cptrf4)[4];

/**
 * Subclass of vbuf_render.
 */
struct llvmpipe_vbuf_render
{
   struct vbuf_render base;
   struct llvmpipe_context *llvmpipe;
   struct setup_context *setup;

   uint prim;
   uint vertex_size;
   uint nr_vertices;
   uint vertex_buffer_size;
   void *vertex_buffer;
};


/** cast wrapper */
static struct llvmpipe_vbuf_render *
llvmpipe_vbuf_render(struct vbuf_render *vbr)
{
   return (struct llvmpipe_vbuf_render *) vbr;
}







static const struct vertex_info *
lp_vbuf_get_vertex_info(struct vbuf_render *vbr)
{
   struct llvmpipe_vbuf_render *cvbr = llvmpipe_vbuf_render(vbr);
   return llvmpipe_get_vbuf_vertex_info(cvbr->llvmpipe);
}


static boolean
lp_vbuf_allocate_vertices(struct vbuf_render *vbr,
                          ushort vertex_size, ushort nr_vertices)
{
   struct llvmpipe_vbuf_render *cvbr = llvmpipe_vbuf_render(vbr);
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
lp_vbuf_release_vertices(struct vbuf_render *vbr)
{
   /* keep the old allocation for next time */
}

static void *
lp_vbuf_map_vertices(struct vbuf_render *vbr)
{
   struct llvmpipe_vbuf_render *cvbr = llvmpipe_vbuf_render(vbr);
   return cvbr->vertex_buffer;
}

static void 
lp_vbuf_unmap_vertices(struct vbuf_render *vbr, 
                       ushort min_index,
                       ushort max_index )
{
   struct llvmpipe_vbuf_render *cvbr = llvmpipe_vbuf_render(vbr);
   assert( cvbr->vertex_buffer_size >= (max_index+1) * cvbr->vertex_size );
   /* do nothing */
}


static boolean
lp_vbuf_set_primitive(struct vbuf_render *vbr, unsigned prim)
{
   struct llvmpipe_vbuf_render *cvbr = llvmpipe_vbuf_render(vbr);
   struct setup_context *setup_ctx = cvbr->setup;
   
   llvmpipe_setup_prepare( setup_ctx );

   cvbr->llvmpipe->reduced_prim = u_reduced_prim(prim);
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
lp_vbuf_draw(struct vbuf_render *vbr, const ushort *indices, uint nr)
{
   struct llvmpipe_vbuf_render *cvbr = llvmpipe_vbuf_render(vbr);
   struct llvmpipe_context *llvmpipe = cvbr->llvmpipe;
   const unsigned stride = llvmpipe->vertex_info_vbuf.size * sizeof(float);
   const void *vertex_buffer = cvbr->vertex_buffer;
   struct setup_context *setup_ctx = cvbr->setup;
   unsigned i;

   switch (cvbr->prim) {
   case PIPE_PRIM_POINTS:
      for (i = 0; i < nr; i++) {
         llvmpipe_setup_point( setup_ctx,
                      get_vert(vertex_buffer, indices[i-0], stride) );
      }
      break;

   case PIPE_PRIM_LINES:
      for (i = 1; i < nr; i += 2) {
         llvmpipe_setup_line( setup_ctx,
                     get_vert(vertex_buffer, indices[i-1], stride),
                     get_vert(vertex_buffer, indices[i-0], stride) );
      }
      break;

   case PIPE_PRIM_LINE_STRIP:
      for (i = 1; i < nr; i ++) {
         llvmpipe_setup_line( setup_ctx,
                     get_vert(vertex_buffer, indices[i-1], stride),
                     get_vert(vertex_buffer, indices[i-0], stride) );
      }
      break;

   case PIPE_PRIM_LINE_LOOP:
      for (i = 1; i < nr; i ++) {
         llvmpipe_setup_line( setup_ctx,
                     get_vert(vertex_buffer, indices[i-1], stride),
                     get_vert(vertex_buffer, indices[i-0], stride) );
      }
      if (nr) {
         llvmpipe_setup_line( setup_ctx,
                     get_vert(vertex_buffer, indices[nr-1], stride),
                     get_vert(vertex_buffer, indices[0], stride) );
      }
      break;

   case PIPE_PRIM_TRIANGLES:
      if (llvmpipe->rasterizer->flatshade_first) {
         for (i = 2; i < nr; i += 3) {
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i-1], stride),
                       get_vert(vertex_buffer, indices[i-0], stride),
                       get_vert(vertex_buffer, indices[i-2], stride) );
         }
      }
      else {
         for (i = 2; i < nr; i += 3) {
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i-2], stride),
                       get_vert(vertex_buffer, indices[i-1], stride),
                       get_vert(vertex_buffer, indices[i-0], stride) );
         }
      }
      break;

   case PIPE_PRIM_TRIANGLE_STRIP:
      if (llvmpipe->rasterizer->flatshade_first) {
         for (i = 2; i < nr; i += 1) {
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i+(i&1)-1], stride),
                       get_vert(vertex_buffer, indices[i-(i&1)], stride),
                       get_vert(vertex_buffer, indices[i-2], stride) );
         }
      }
      else {
         for (i = 2; i < nr; i += 1) {
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i+(i&1)-2], stride),
                       get_vert(vertex_buffer, indices[i-(i&1)-1], stride),
                       get_vert(vertex_buffer, indices[i-0], stride) );
         }
      }
      break;

   case PIPE_PRIM_TRIANGLE_FAN:
      if (llvmpipe->rasterizer->flatshade_first) {
         for (i = 2; i < nr; i += 1) {
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i-0], stride),
                       get_vert(vertex_buffer, indices[0], stride),
                       get_vert(vertex_buffer, indices[i-1], stride) );
         }
      }
      else {
         for (i = 2; i < nr; i += 1) {
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[0], stride),
                       get_vert(vertex_buffer, indices[i-1], stride),
                       get_vert(vertex_buffer, indices[i-0], stride) );
         }
      }
      break;

   case PIPE_PRIM_QUADS:
      if (llvmpipe->rasterizer->flatshade_first) {
         for (i = 3; i < nr; i += 4) {
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i-2], stride),
                       get_vert(vertex_buffer, indices[i-1], stride),
                       get_vert(vertex_buffer, indices[i-3], stride) );
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i-1], stride),
                       get_vert(vertex_buffer, indices[i-0], stride),
                       get_vert(vertex_buffer, indices[i-3], stride) );
         }
      }
      else {
         for (i = 3; i < nr; i += 4) {
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i-3], stride),
                       get_vert(vertex_buffer, indices[i-2], stride),
                       get_vert(vertex_buffer, indices[i-0], stride) );

            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i-2], stride),
                       get_vert(vertex_buffer, indices[i-1], stride),
                       get_vert(vertex_buffer, indices[i-0], stride) );
         }
      }
      break;

   case PIPE_PRIM_QUAD_STRIP:
      if (llvmpipe->rasterizer->flatshade_first) {
         for (i = 3; i < nr; i += 2) {
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i-0], stride),
                       get_vert(vertex_buffer, indices[i-1], stride),
                       get_vert(vertex_buffer, indices[i-3], stride));
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i-2], stride),
                       get_vert(vertex_buffer, indices[i-0], stride),
                       get_vert(vertex_buffer, indices[i-3], stride) );
         }
      }
      else {
         for (i = 3; i < nr; i += 2) {
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, indices[i-3], stride),
                       get_vert(vertex_buffer, indices[i-2], stride),
                       get_vert(vertex_buffer, indices[i-0], stride) );
            llvmpipe_setup_tri( setup_ctx,
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
         llvmpipe_setup_tri( setup_ctx,
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
lp_vbuf_draw_arrays(struct vbuf_render *vbr, uint start, uint nr)
{
   struct llvmpipe_vbuf_render *cvbr = llvmpipe_vbuf_render(vbr);
   struct llvmpipe_context *llvmpipe = cvbr->llvmpipe;
   struct setup_context *setup_ctx = cvbr->setup;
   const unsigned stride = llvmpipe->vertex_info_vbuf.size * sizeof(float);
   const void *vertex_buffer =
      (void *) get_vert(cvbr->vertex_buffer, start, stride);
   unsigned i;

   switch (cvbr->prim) {
   case PIPE_PRIM_POINTS:
      for (i = 0; i < nr; i++) {
         llvmpipe_setup_point( setup_ctx,
                      get_vert(vertex_buffer, i-0, stride) );
      }
      break;

   case PIPE_PRIM_LINES:
      for (i = 1; i < nr; i += 2) {
         llvmpipe_setup_line( setup_ctx,
                     get_vert(vertex_buffer, i-1, stride),
                     get_vert(vertex_buffer, i-0, stride) );
      }
      break;

   case PIPE_PRIM_LINE_STRIP:
      for (i = 1; i < nr; i ++) {
         llvmpipe_setup_line( setup_ctx,
                     get_vert(vertex_buffer, i-1, stride),
                     get_vert(vertex_buffer, i-0, stride) );
      }
      break;

   case PIPE_PRIM_LINE_LOOP:
      for (i = 1; i < nr; i ++) {
         llvmpipe_setup_line( setup_ctx,
                     get_vert(vertex_buffer, i-1, stride),
                     get_vert(vertex_buffer, i-0, stride) );
      }
      if (nr) {
         llvmpipe_setup_line( setup_ctx,
                     get_vert(vertex_buffer, nr-1, stride),
                     get_vert(vertex_buffer, 0, stride) );
      }
      break;

   case PIPE_PRIM_TRIANGLES:
      if (llvmpipe->rasterizer->flatshade_first) {
         for (i = 2; i < nr; i += 3) {
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i-1, stride),
                       get_vert(vertex_buffer, i-0, stride),
                       get_vert(vertex_buffer, i-2, stride) );
         }
      }
      else {
         for (i = 2; i < nr; i += 3) {
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i-2, stride),
                       get_vert(vertex_buffer, i-1, stride),
                       get_vert(vertex_buffer, i-0, stride) );
         }
      }
      break;

   case PIPE_PRIM_TRIANGLE_STRIP:
      if (llvmpipe->rasterizer->flatshade_first) {
         for (i = 2; i < nr; i++) {
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i+(i&1)-1, stride),
                       get_vert(vertex_buffer, i-(i&1), stride),
                       get_vert(vertex_buffer, i-2, stride) );
         }
      }
      else {
         for (i = 2; i < nr; i++) {
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i+(i&1)-2, stride),
                       get_vert(vertex_buffer, i-(i&1)-1, stride),
                       get_vert(vertex_buffer, i-0, stride) );
         }
      }
      break;

   case PIPE_PRIM_TRIANGLE_FAN:
      if (llvmpipe->rasterizer->flatshade_first) {
         for (i = 2; i < nr; i += 1) {
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i-0, stride),
                       get_vert(vertex_buffer, 0, stride),
                       get_vert(vertex_buffer, i-1, stride) );
         }
      }
      else {
         for (i = 2; i < nr; i += 1) {
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, 0, stride),
                       get_vert(vertex_buffer, i-1, stride),
                       get_vert(vertex_buffer, i-0, stride) );
         }
      }
      break;

   case PIPE_PRIM_QUADS:
      if (llvmpipe->rasterizer->flatshade_first) {
         for (i = 3; i < nr; i += 4) {
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i-2, stride),
                       get_vert(vertex_buffer, i-1, stride),
                       get_vert(vertex_buffer, i-3, stride) );
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i-1, stride),
                       get_vert(vertex_buffer, i-0, stride),
                       get_vert(vertex_buffer, i-3, stride) );
         }
      }
      else {
         for (i = 3; i < nr; i += 4) {
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i-3, stride),
                       get_vert(vertex_buffer, i-2, stride),
                       get_vert(vertex_buffer, i-0, stride) );
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i-2, stride),
                       get_vert(vertex_buffer, i-1, stride),
                       get_vert(vertex_buffer, i-0, stride) );
         }
      }
      break;

   case PIPE_PRIM_QUAD_STRIP:
      if (llvmpipe->rasterizer->flatshade_first) {
         for (i = 3; i < nr; i += 2) {
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i-0, stride),
                       get_vert(vertex_buffer, i-1, stride),
                       get_vert(vertex_buffer, i-3, stride) );
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i-2, stride),
                       get_vert(vertex_buffer, i-0, stride),
                       get_vert(vertex_buffer, i-3, stride) );
         }
      }
      else {
         for (i = 3; i < nr; i += 2) {
            llvmpipe_setup_tri( setup_ctx,
                       get_vert(vertex_buffer, i-3, stride),
                       get_vert(vertex_buffer, i-2, stride),
                       get_vert(vertex_buffer, i-0, stride) );
            llvmpipe_setup_tri( setup_ctx,
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
         llvmpipe_setup_tri( setup_ctx,
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
lp_vbuf_destroy(struct vbuf_render *vbr)
{
   struct llvmpipe_vbuf_render *cvbr = llvmpipe_vbuf_render(vbr);
   llvmpipe_setup_destroy_context(cvbr->setup);
   FREE(cvbr);
}


/**
 * Create the post-transform vertex handler for the given context.
 */
struct vbuf_render *
lp_create_vbuf_backend(struct llvmpipe_context *lp)
{
   struct llvmpipe_vbuf_render *cvbr = CALLOC_STRUCT(llvmpipe_vbuf_render);

   assert(lp->draw);


   cvbr->base.max_indices = LP_MAX_VBUF_INDEXES;
   cvbr->base.max_vertex_buffer_bytes = LP_MAX_VBUF_SIZE;

   cvbr->base.get_vertex_info = lp_vbuf_get_vertex_info;
   cvbr->base.allocate_vertices = lp_vbuf_allocate_vertices;
   cvbr->base.map_vertices = lp_vbuf_map_vertices;
   cvbr->base.unmap_vertices = lp_vbuf_unmap_vertices;
   cvbr->base.set_primitive = lp_vbuf_set_primitive;
   cvbr->base.draw = lp_vbuf_draw;
   cvbr->base.draw_arrays = lp_vbuf_draw_arrays;
   cvbr->base.release_vertices = lp_vbuf_release_vertices;
   cvbr->base.destroy = lp_vbuf_destroy;

   cvbr->llvmpipe = lp;

   cvbr->setup = llvmpipe_setup_create_context(cvbr->llvmpipe);

   return &cvbr->base;
}
