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
#include "sp_state.h"
#include "sp_prim_vbuf.h"
#include "sp_prim_setup.h"
#include "sp_setup.h"
#include "draw/draw_context.h"
#include "draw/draw_vbuf.h"
#include "util/u_memory.h"


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
#if 0
   {
      struct softpipe_vbuf_render *cvbr = softpipe_vbuf_render(vbr);
      const struct vertex_info *info = 
         softpipe_get_vbuf_vertex_info(cvbr->softpipe);
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
   /* do nothing */
}


static boolean
sp_vbuf_set_primitive(struct vbuf_render *vbr, unsigned prim)
{
   struct softpipe_vbuf_render *cvbr = softpipe_vbuf_render(vbr);

   /* XXX: break this dependency - make setup_context live under
    * softpipe, rename the old "setup" draw stage to something else.
    */
   struct setup_context *setup_ctx = sp_draw_setup_context(cvbr->softpipe->setup);
   
   setup_prepare( setup_ctx );

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
   unsigned i;

   /* XXX: break this dependency - make setup_context live under
    * softpipe, rename the old "setup" draw stage to something else.
    */
   struct draw_stage *setup = softpipe->setup;
   struct setup_context *setup_ctx = sp_draw_setup_context(setup);

   switch (cvbr->prim) {
   case PIPE_PRIM_POINTS:
      for (i = 0; i < nr; i++) {
         setup_point( setup_ctx,
                      get_vert(vertex_buffer, indices[i-0], stride) );
      }
      break;

   case PIPE_PRIM_LINES:
      for (i = 1; i < nr; i += 2) {
         setup_line( setup_ctx,
                     get_vert(vertex_buffer, indices[i-1], stride),
                     get_vert(vertex_buffer, indices[i-0], stride) );
      }
      break;

   case PIPE_PRIM_LINE_STRIP:
      for (i = 1; i < nr; i ++) {
         setup_line( setup_ctx,
                     get_vert(vertex_buffer, indices[i-1], stride),
                     get_vert(vertex_buffer, indices[i-0], stride) );
      }
      break;

   case PIPE_PRIM_LINE_LOOP:
      for (i = 1; i < nr; i ++) {
         setup_line( setup_ctx,
                     get_vert(vertex_buffer, indices[i-1], stride),
                     get_vert(vertex_buffer, indices[i-0], stride) );
      }
      if (nr) {
         setup_line( setup_ctx,
                     get_vert(vertex_buffer, indices[nr-1], stride),
                     get_vert(vertex_buffer, indices[0], stride) );
      }
      break;

   case PIPE_PRIM_TRIANGLES:
      for (i = 2; i < nr; i += 3) {
         setup_tri( setup_ctx,
                    get_vert(vertex_buffer, indices[i-2], stride),
                    get_vert(vertex_buffer, indices[i-1], stride),
                    get_vert(vertex_buffer, indices[i-0], stride));
      }
      break;

   case PIPE_PRIM_TRIANGLE_STRIP:
      for (i = 2; i < nr; i += 1) {
         setup_tri( setup_ctx,
                    get_vert(vertex_buffer, indices[i+(i&1)-2], stride),
                    get_vert(vertex_buffer, indices[i-(i&1)-1], stride),
                    get_vert(vertex_buffer, indices[i-0], stride));
      }
      break;

   case PIPE_PRIM_TRIANGLE_FAN:
      for (i = 2; i < nr; i += 1) {
         setup_tri( setup_ctx,
                    get_vert(vertex_buffer, indices[0], stride),
                    get_vert(vertex_buffer, indices[i-1], stride),
                    get_vert(vertex_buffer, indices[i-0], stride));
      }
      break;

   case PIPE_PRIM_QUADS:
      for (i = 3; i < nr; i += 4) {
         setup_tri( setup_ctx,
                    get_vert(vertex_buffer, indices[i-3], stride),
                    get_vert(vertex_buffer, indices[i-2], stride),
                    get_vert(vertex_buffer, indices[i-0], stride));

         setup_tri( setup_ctx,
                    get_vert(vertex_buffer, indices[i-2], stride),
                    get_vert(vertex_buffer, indices[i-1], stride),
                    get_vert(vertex_buffer, indices[i-0], stride));
      }
      break;

   case PIPE_PRIM_QUAD_STRIP:
      for (i = 3; i < nr; i += 2) {
         setup_tri( setup_ctx,
                    get_vert(vertex_buffer, indices[i-3], stride),
                    get_vert(vertex_buffer, indices[i-2], stride),
                    get_vert(vertex_buffer, indices[i-0], stride));

         setup_tri( setup_ctx,
                    get_vert(vertex_buffer, indices[i-1], stride),
                    get_vert(vertex_buffer, indices[i-3], stride),
                    get_vert(vertex_buffer, indices[i-0], stride));
      }
      break;

   case PIPE_PRIM_POLYGON:
      for (i = 2; i < nr; i += 1) {
         setup_tri( setup_ctx,
                    get_vert(vertex_buffer, indices[0-1], stride),
                    get_vert(vertex_buffer, indices[i-0], stride),
                    get_vert(vertex_buffer, indices[0], stride));
      }
      break;

   default:
      assert(0);
   }

   /* XXX: why are we calling this???  If we had to call something, it
    * would be a function in sp_setup.c:
    */
   sp_draw_flush( setup );
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
   const unsigned stride = softpipe->vertex_info_vbuf.size * sizeof(float);
   const void *vertex_buffer =
      (void *) get_vert(cvbr->vertex_buffer, start, stride);
   unsigned i;

   /* XXX: break this dependency - make setup_context live under
    * softpipe, rename the old "setup" draw stage to something else.
    */
   struct draw_stage *setup = softpipe->setup;
   struct setup_context *setup_ctx = sp_draw_setup_context(setup);

   switch (cvbr->prim) {
   case PIPE_PRIM_POINTS:
      for (i = 0; i < nr; i++) {
         setup_point( setup_ctx,
                      get_vert(vertex_buffer, i-0, stride) );
      }
      break;

   case PIPE_PRIM_LINES:
      for (i = 1; i < nr; i += 2) {
         setup_line( setup_ctx,
                     get_vert(vertex_buffer, i-1, stride),
                     get_vert(vertex_buffer, i-0, stride) );
      }
      break;

   case PIPE_PRIM_LINE_STRIP:
      for (i = 1; i < nr; i ++) {
         setup_line( setup_ctx,
                     get_vert(vertex_buffer, i-1, stride),
                     get_vert(vertex_buffer, i-0, stride) );
      }
      break;

   case PIPE_PRIM_LINE_LOOP:
      for (i = 1; i < nr; i ++) {
         setup_line( setup_ctx,
                     get_vert(vertex_buffer, i-1, stride),
                     get_vert(vertex_buffer, i-0, stride) );
      }
      if (nr) {
         setup_line( setup_ctx,
                     get_vert(vertex_buffer, nr-1, stride),
                     get_vert(vertex_buffer, 0, stride) );
      }
      break;


   case PIPE_PRIM_TRIANGLES:
      for (i = 2; i < nr; i += 3) {
         setup_tri( setup_ctx,
                    get_vert(vertex_buffer, i-2, stride),
                    get_vert(vertex_buffer, i-1, stride),
                    get_vert(vertex_buffer, i-0, stride));
      }
      break;

   case PIPE_PRIM_TRIANGLE_STRIP:
      for (i = 2; i < nr; i += 1) {
         setup_tri( setup_ctx,
                    get_vert(vertex_buffer, i+(i&1)-2, stride),
                    get_vert(vertex_buffer, i-(i&1)-1, stride),
                    get_vert(vertex_buffer, i-0, stride));
      }
      break;

   case PIPE_PRIM_TRIANGLE_FAN:
      for (i = 2; i < nr; i += 1) {
         setup_tri( setup_ctx,
                    get_vert(vertex_buffer, 0, stride),
                    get_vert(vertex_buffer, i-1, stride),
                    get_vert(vertex_buffer, i-0, stride));
      }
      break;

   case PIPE_PRIM_QUADS:
      for (i = 3; i < nr; i += 4) {
         setup_tri( setup_ctx,
                    get_vert(vertex_buffer, i-3, stride),
                    get_vert(vertex_buffer, i-2, stride),
                    get_vert(vertex_buffer, i-0, stride));

         setup_tri( setup_ctx,
                    get_vert(vertex_buffer, i-2, stride),
                    get_vert(vertex_buffer, i-1, stride),
                    get_vert(vertex_buffer, i-0, stride));
      }
      break;
   case PIPE_PRIM_QUAD_STRIP:
      for (i = 3; i < nr; i += 2) {
         setup_tri( setup_ctx,
                    get_vert(vertex_buffer, i-3, stride),
                    get_vert(vertex_buffer, i-2, stride),
                    get_vert(vertex_buffer, i-0, stride));

         setup_tri( setup_ctx,
                    get_vert(vertex_buffer, i-1, stride),
                    get_vert(vertex_buffer, i-3, stride),
                    get_vert(vertex_buffer, i-0, stride));
      }
      break;

   case PIPE_PRIM_POLYGON:
      /* Almost same as tri fan but the _first_ vertex specifies the flat
       * shading color.  Note that the first polygon vertex is passed as
       * the last triangle vertex here.
       */
      for (i = 2; i < nr; i += 1) {
         setup_tri( setup_ctx,
                    get_vert(vertex_buffer, i-1, stride),
                    get_vert(vertex_buffer, i-0, stride),
                    get_vert(vertex_buffer, 0, stride));
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
   cvbr->softpipe->vbuf_render = NULL;
   FREE(cvbr);
}


/**
 * Initialize the post-transform vertex buffer information for the given
 * context.
 */
void
sp_init_vbuf(struct softpipe_context *sp)
{
   assert(sp->draw);

   sp->vbuf_render = CALLOC_STRUCT(softpipe_vbuf_render);

   sp->vbuf_render->base.max_indices = SP_MAX_VBUF_INDEXES;
   sp->vbuf_render->base.max_vertex_buffer_bytes = SP_MAX_VBUF_SIZE;

   sp->vbuf_render->base.get_vertex_info = sp_vbuf_get_vertex_info;
   sp->vbuf_render->base.allocate_vertices = sp_vbuf_allocate_vertices;
   sp->vbuf_render->base.map_vertices = sp_vbuf_map_vertices;
   sp->vbuf_render->base.unmap_vertices = sp_vbuf_unmap_vertices;
   sp->vbuf_render->base.set_primitive = sp_vbuf_set_primitive;
   sp->vbuf_render->base.draw = sp_vbuf_draw;
   sp->vbuf_render->base.draw_arrays = sp_vbuf_draw_arrays;
   sp->vbuf_render->base.release_vertices = sp_vbuf_release_vertices;
   sp->vbuf_render->base.destroy = sp_vbuf_destroy;

   sp->vbuf_render->softpipe = sp;

   sp->vbuf = draw_vbuf_stage(sp->draw, &sp->vbuf_render->base);

   draw_set_rasterize_stage(sp->draw, sp->vbuf);

   draw_set_render(sp->draw, &sp->vbuf_render->base);
}
