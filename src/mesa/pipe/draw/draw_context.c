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

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */


#include "pipe/p_util.h"
#include "draw_context.h"
#include "draw_private.h"



struct draw_context *draw_create( void )
{
   struct draw_context *draw = CALLOC_STRUCT( draw_context );

#if defined(__i386__) || defined(__386__)
   draw->use_sse = getenv("GALLIUM_SSE") != NULL;
#else
   draw->use_sse = false;
#endif

   /* create pipeline stages */
   draw->pipeline.unfilled  = draw_unfilled_stage( draw );
   draw->pipeline.twoside   = draw_twoside_stage( draw );
   draw->pipeline.offset    = draw_offset_stage( draw );
   draw->pipeline.clip      = draw_clip_stage( draw );
   draw->pipeline.flatshade = draw_flatshade_stage( draw );
   draw->pipeline.cull      = draw_cull_stage( draw );
   draw->pipeline.feedback  = draw_feedback_stage( draw );
   draw->pipeline.validate  = draw_validate_stage( draw );
   draw->pipeline.first     = draw->pipeline.validate;

   ASSIGN_4V( draw->plane[0], -1,  0,  0, 1 );
   ASSIGN_4V( draw->plane[1],  1,  0,  0, 1 );
   ASSIGN_4V( draw->plane[2],  0, -1,  0, 1 );
   ASSIGN_4V( draw->plane[3],  0,  1,  0, 1 );
   ASSIGN_4V( draw->plane[4],  0,  0,  1, 1 ); /* yes these are correct */
   ASSIGN_4V( draw->plane[5],  0,  0, -1, 1 ); /* mesa's a bit wonky */
   draw->nr_planes = 6;

   /* Statically allocate maximum sized vertices for the cache - could be cleverer...
    */
   {
      int i;
      char *tmp = malloc(Elements(draw->vcache.vertex) * MAX_VERTEX_SIZE);

      for (i = 0; i < Elements(draw->vcache.vertex); i++)
	 draw->vcache.vertex[i] = (struct vertex_header *)(tmp + i * MAX_VERTEX_SIZE);
   }

   draw->attrib_front0 = -1;
   draw->attrib_back0 = -1;
   draw->attrib_front1 = -1;
   draw->attrib_back1 = -1;

   draw_vertex_cache_invalidate( draw );

   return draw;
}


void draw_destroy( struct draw_context *draw )
{
   free( draw->vcache.vertex[0] ); /* Frees all the vertices. */
   free( draw );
}



void draw_flush( struct draw_context *draw )
{
   if (draw->drawing)
      draw_do_flush( draw, DRAW_FLUSH_DRAW );
}


void draw_set_feedback_state( struct draw_context *draw,
                              const struct pipe_feedback_state *feedback )
{
   draw_flush( draw );
   draw->feedback = *feedback;
}


/**
 * Register new primitive rasterization/rendering state.
 * This causes the drawing pipeline to be rebuilt.
 */
void draw_set_rasterizer_state( struct draw_context *draw,
                                const struct pipe_rasterizer_state *raster )
{
   draw_flush( draw );
   draw->rasterizer = raster;
}


/** 
 * Plug in the primitive rendering/rasterization stage.
 * This is provided by the device driver.
 */
void draw_set_rasterize_stage( struct draw_context *draw,
                               struct draw_stage *stage )
{
   draw_flush( draw );
   draw->pipeline.rasterize = stage;
}


/**
 * Set the draw module's clipping state.
 */
void draw_set_clip_state( struct draw_context *draw,
                          const struct pipe_clip_state *clip )
{
   draw_flush( draw );

   assert(clip->nr <= PIPE_MAX_CLIP_PLANES);
   memcpy(&draw->plane[6], clip->ucp, clip->nr * sizeof(clip->ucp[0]));
   draw->nr_planes = 6 + clip->nr;
   /* bitmask of the enabled user-defined clip planes */
   draw->user_clipmask = ((1 << clip->nr) - 1) << 6;
}


/**
 * Set the draw module's viewport state.
 */
void draw_set_viewport_state( struct draw_context *draw,
                              const struct pipe_viewport_state *viewport )
{
   draw_flush( draw );
   draw->viewport = *viewport; /* struct copy */
}



void
draw_set_vertex_buffer(struct draw_context *draw,
                       unsigned attr,
                       const struct pipe_vertex_buffer *buffer)
{
   draw_flush( draw );

   assert(attr < PIPE_ATTRIB_MAX);
   draw->vertex_buffer[attr] = *buffer;
}


void
draw_set_vertex_element(struct draw_context *draw,
                        unsigned attr,
                        const struct pipe_vertex_element *element)
{
   draw_flush( draw );

   assert(attr < PIPE_ATTRIB_MAX);
   draw->vertex_element[attr] = *element;
}


/**
 * Tell drawing context where to find mapped vertex buffers.
 */
void
draw_set_mapped_vertex_buffer(struct draw_context *draw,
                              unsigned attr, const void *buffer)
{
   draw_flush( draw );

   draw->mapped_vbuffer[attr] = buffer;
}


void
draw_set_mapped_constant_buffer(struct draw_context *draw,
                                const void *buffer)
{
   draw_flush( draw );

   draw->mapped_constants = buffer;
}


void
draw_set_mapped_feedback_buffer(struct draw_context *draw, uint index,
                                void *buffer, uint size)
{
   draw_flush( draw );

   assert(index < PIPE_MAX_FEEDBACK_ATTRIBS);
   draw->mapped_feedback_buffer[index] = buffer;
   draw->mapped_feedback_buffer_size[index] = size; /* in bytes */
}





/**
 * Allocate space for temporary post-transform vertices, such as for clipping.
 */
void draw_alloc_tmps( struct draw_stage *stage, unsigned nr )
{
   stage->nr_tmps = nr;

   if (nr) {
      ubyte *store = (ubyte *) malloc(MAX_VERTEX_SIZE * nr);
      unsigned i;

      stage->tmp = (struct vertex_header **) malloc(sizeof(struct vertex_header *) * nr);
      
      for (i = 0; i < nr; i++)
	 stage->tmp[i] = (struct vertex_header *)(store + i * MAX_VERTEX_SIZE);
   }
}

void draw_free_tmps( struct draw_stage *stage )
{
   if (stage->tmp) {
      free(stage->tmp[0]);
      free(stage->tmp);
   }
}

boolean draw_use_sse(struct draw_context *draw)
{
   return draw->use_sse;
}


