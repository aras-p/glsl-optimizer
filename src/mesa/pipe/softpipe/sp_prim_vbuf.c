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
 * Post-transform vertex buffering.  This is an optional part of the
 * softpipe rendering pipeline.
 * Probably not desired in general, but useful for testing/debuggin.
 * Enabled/Disabled with SP_VBUF env var.
 * 
 * Authors
 *  Brian Paul
 */


#include "sp_context.h"
#include "sp_prim_vbuf.h"
#include "pipe/draw/draw_context.h"
#include "pipe/draw/draw_private.h"
#include "pipe/draw/draw_vbuf.h"


#define SP_MAX_VBUF_INDEXES 1024
#define SP_MAX_VBUF_SIZE    4096


/**
 * Subclass of vbuf_render.
 */
struct softpipe_vbuf_render
{
   struct vbuf_render base;
   struct softpipe_context *softpipe;
   uint prim;
   uint vertex_size;
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
   /* XXX check for state changes? */
   assert(!cvbr->softpipe->dirty );
   return &cvbr->softpipe->vertex_info;
}


static void *
sp_vbuf_allocate_vertices(struct vbuf_render *vbr,
                            ushort vertex_size, ushort nr_vertices)
{
   struct softpipe_vbuf_render *cvbr = softpipe_vbuf_render(vbr);
   assert(!cvbr->vertex_buffer);
   cvbr->vertex_buffer = align_malloc(vertex_size * nr_vertices, 16);
   cvbr->vertex_size = vertex_size;
   return cvbr->vertex_buffer;
}


static void
sp_vbuf_release_vertices(struct vbuf_render *vbr, void *vertices, 
                           unsigned vertex_size, unsigned vertices_used)
{
   struct softpipe_vbuf_render *cvbr = softpipe_vbuf_render(vbr);
   align_free(vertices);
   assert(vertices == cvbr->vertex_buffer);
   cvbr->vertex_buffer = NULL;
}


static void
sp_vbuf_set_primitive(struct vbuf_render *vbr, unsigned prim)
{
   struct softpipe_vbuf_render *cvbr = softpipe_vbuf_render(vbr);
   cvbr->prim = prim;
}


/**
 * Recalculate prim's determinant.
 * XXX is this needed?
 */
static void
calc_det(struct prim_header *header)
{
   /* Window coords: */
   const float *v0 = header->v[0]->data[0];
   const float *v1 = header->v[1]->data[0];
   const float *v2 = header->v[2]->data[0];

   /* edge vectors e = v0 - v2, f = v1 - v2 */
   const float ex = v0[0] - v2[0];
   const float ey = v0[1] - v2[1];
   const float fx = v1[0] - v2[0];
   const float fy = v1[1] - v2[1];
   
   /* det = cross(e,f).z */
   header->det = ex * fy - ey * fx;
}


static void
sp_vbuf_draw(struct vbuf_render *vbr, const ushort *indices, uint nr_indices)
{
   struct softpipe_vbuf_render *cvbr = softpipe_vbuf_render(vbr);
   struct softpipe_context *softpipe = cvbr->softpipe;
   struct draw_stage *setup = softpipe->setup;
   struct prim_header prim;
   unsigned vertex_size = softpipe->vertex_info.size * sizeof(float);
   unsigned i, j;
   void *vertex_buffer = cvbr->vertex_buffer;

   prim.det = 0;
   prim.reset_line_stipple = 0;
   prim.edgeflags = 0;
   prim.pad = 0;

   setup->begin( setup );

   switch (cvbr->prim) {
   case PIPE_PRIM_TRIANGLES:
      for (i = 0; i < nr_indices; i += 3) {
         for (j = 0; j < 3; j++) 
            prim.v[j] = (struct vertex_header *)((char *)vertex_buffer + 
                                                 indices[i+j] * vertex_size);
         
         calc_det(&prim);
         setup->tri( setup, &prim );
      }
      break;

   case PIPE_PRIM_LINES:
      for (i = 0; i < nr_indices; i += 2) {
         for (j = 0; j < 2; j++) 
            prim.v[j] = (struct vertex_header *)((char *)vertex_buffer + 
                                                 indices[i+j] * vertex_size);
         
         setup->line( setup, &prim );
      }
      break;

   case PIPE_PRIM_POINTS:
      for (i = 0; i < nr_indices; i++) {
         prim.v[0] = (struct vertex_header *)((char *)vertex_buffer + 
                                              indices[i] * vertex_size);         
         setup->point( setup, &prim );
      }
      break;
   }

   setup->end( setup );
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
   sp->vbuf_render->base.set_primitive = sp_vbuf_set_primitive;
   sp->vbuf_render->base.draw = sp_vbuf_draw;
   sp->vbuf_render->base.release_vertices = sp_vbuf_release_vertices;
   sp->vbuf_render->base.destroy = sp_vbuf_destroy;

   sp->vbuf_render->softpipe = sp;

   sp->vbuf = draw_vbuf_stage(sp->draw, &sp->vbuf_render->base);

   draw_set_rasterize_stage(sp->draw, sp->vbuf);
}
