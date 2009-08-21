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
#include "lp_state.h"
#include "lp_prim_vbuf.h"
#include "lp_prim_setup.h"
#include "lp_setup.h"
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
#if 0
   {
      struct llvmpipe_vbuf_render *cvbr = llvmpipe_vbuf_render(vbr);
      const struct vertex_info *info = 
         llvmpipe_get_vbuf_vertex_info(cvbr->llvmpipe);
      const float *vtx = (const float *) cvbr->vertex_buffer;
      uint i, j;
      debug_printf("%s (vtx_size = %u,  vtx_used = %u)\n",
             __FUNCTION__, cvbr->vertex_size, cvbr->nr_vertices);
      for (i = 0; i < cvbr->nr_vertices; i++) {
         for (j = 0; j < info->num_attribs; j++) {
            uint k;
            switch (info->attrib[j].emit) {
            case EMIT_4F:  k = 4;   break;
            case EMIT_3F:  k = 3;   break;
            case EMIT_2F:  k = 2;   break;
            case EMIT_1F:  k = 1;   break;
            default: assert(0);
            }
            debug_printf("Vert %u attr %u: ", i, j);
            while (k-- > 0) {
               debug_printf("%g ", vtx[0]);
               vtx++;
            }
            debug_printf("\n");
         }
      }
   }
#endif

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

   /* XXX: break this dependency - make setup_context live under
    * llvmpipe, rename the old "setup" draw stage to something else.
    */
   struct setup_context *setup_ctx = lp_draw_setup_context(cvbr->llvmpipe->setup);
   
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
   unsigned i;

   /* XXX: break this dependency - make setup_context live under
    * llvmpipe, rename the old "setup" draw stage to something else.
    */
   struct draw_stage *setup = llvmpipe->setup;
   struct setup_context *setup_ctx = lp_draw_setup_context(setup);

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

   /* XXX: why are we calling this???  If we had to call something, it
    * would be a function in lp_setup.c:
    */
   lp_draw_flush( setup );
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
   const unsigned stride = llvmpipe->vertex_info_vbuf.size * sizeof(float);
   const void *vertex_buffer =
      (void *) get_vert(cvbr->vertex_buffer, start, stride);
   unsigned i;

   /* XXX: break this dependency - make setup_context live under
    * llvmpipe, rename the old "setup" draw stage to something else.
    */
   struct draw_stage *setup = llvmpipe->setup;
   struct setup_context *setup_ctx = lp_draw_setup_context(setup);

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
   cvbr->llvmpipe->vbuf_render = NULL;
   FREE(cvbr);
}


/**
 * Initialize the post-transform vertex buffer information for the given
 * context.
 */
void
lp_init_vbuf(struct llvmpipe_context *lp)
{
   assert(lp->draw);

   lp->vbuf_render = CALLOC_STRUCT(llvmpipe_vbuf_render);

   lp->vbuf_render->base.max_indices = LP_MAX_VBUF_INDEXES;
   lp->vbuf_render->base.max_vertex_buffer_bytes = LP_MAX_VBUF_SIZE;

   lp->vbuf_render->base.get_vertex_info = lp_vbuf_get_vertex_info;
   lp->vbuf_render->base.allocate_vertices = lp_vbuf_allocate_vertices;
   lp->vbuf_render->base.map_vertices = lp_vbuf_map_vertices;
   lp->vbuf_render->base.unmap_vertices = lp_vbuf_unmap_vertices;
   lp->vbuf_render->base.set_primitive = lp_vbuf_set_primitive;
   lp->vbuf_render->base.draw = lp_vbuf_draw;
   lp->vbuf_render->base.draw_arrays = lp_vbuf_draw_arrays;
   lp->vbuf_render->base.release_vertices = lp_vbuf_release_vertices;
   lp->vbuf_render->base.destroy = lp_vbuf_destroy;

   lp->vbuf_render->llvmpipe = lp;

   lp->vbuf = draw_vbuf_stage(lp->draw, &lp->vbuf_render->base);

   draw_set_rasterize_stage(lp->draw, lp->vbuf);

   draw_set_render(lp->draw, &lp->vbuf_render->base);
}
