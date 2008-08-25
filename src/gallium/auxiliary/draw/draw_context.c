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


#include "util/u_memory.h"
#include "util/u_math.h"
#include "draw_context.h"
#include "draw_vbuf.h"
#include "draw_vs.h"
#include "draw_pt.h"
#include "draw_pipe.h"


struct draw_context *draw_create( void )
{
   struct draw_context *draw = CALLOC_STRUCT( draw_context );
   if (draw == NULL)
      goto fail;

   ASSIGN_4V( draw->plane[0], -1,  0,  0, 1 );
   ASSIGN_4V( draw->plane[1],  1,  0,  0, 1 );
   ASSIGN_4V( draw->plane[2],  0, -1,  0, 1 );
   ASSIGN_4V( draw->plane[3],  0,  1,  0, 1 );
   ASSIGN_4V( draw->plane[4],  0,  0,  1, 1 ); /* yes these are correct */
   ASSIGN_4V( draw->plane[5],  0,  0, -1, 1 ); /* mesa's a bit wonky */
   draw->nr_planes = 6;


   draw->reduced_prim = ~0; /* != any of PIPE_PRIM_x */


   if (!draw_pipeline_init( draw ))
      goto fail;

   if (!draw_pt_init( draw ))
      goto fail;

   if (!draw_vs_init( draw ))
      goto fail;

   return draw;

fail:
   draw_destroy( draw );   
   return NULL;
}


void draw_destroy( struct draw_context *draw )
{
   if (!draw)
      return;



   /* Not so fast -- we're just borrowing this at the moment.
    * 
   if (draw->render)
      draw->render->destroy( draw->render );
   */

   draw_pipeline_destroy( draw );
   draw_pt_destroy( draw );
   draw_vs_destroy( draw );

   FREE( draw );
}



void draw_flush( struct draw_context *draw )
{
   draw_do_flush( draw, DRAW_FLUSH_BACKEND );
}



/**
 * Register new primitive rasterization/rendering state.
 * This causes the drawing pipeline to be rebuilt.
 */
void draw_set_rasterizer_state( struct draw_context *draw,
                                const struct pipe_rasterizer_state *raster )
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );

   draw->rasterizer = raster;
   draw->bypass_clipping =
      ((draw->rasterizer && draw->rasterizer->bypass_clipping) ||
       draw->driver.bypass_clipping);
}


void draw_set_driver_clipping( struct draw_context *draw,
                               boolean bypass_clipping )
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );

   draw->driver.bypass_clipping = bypass_clipping;
   draw->bypass_clipping = (draw->rasterizer->bypass_clipping || 
                            draw->driver.bypass_clipping);
}


/** 
 * Plug in the primitive rendering/rasterization stage (which is the last
 * stage in the drawing pipeline).
 * This is provided by the device driver.
 */
void draw_set_rasterize_stage( struct draw_context *draw,
                               struct draw_stage *stage )
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );

   draw->pipeline.rasterize = stage;
}


/**
 * Set the draw module's clipping state.
 */
void draw_set_clip_state( struct draw_context *draw,
                          const struct pipe_clip_state *clip )
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );

   assert(clip->nr <= PIPE_MAX_CLIP_PLANES);
   memcpy(&draw->plane[6], clip->ucp, clip->nr * sizeof(clip->ucp[0]));
   draw->nr_planes = 6 + clip->nr;
}


/**
 * Set the draw module's viewport state.
 */
void draw_set_viewport_state( struct draw_context *draw,
                              const struct pipe_viewport_state *viewport )
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   draw->viewport = *viewport; /* struct copy */
   draw->identity_viewport = (viewport->scale[0] == 1.0f &&
                              viewport->scale[1] == 1.0f &&
                              viewport->scale[2] == 1.0f &&
                              viewport->scale[3] == 1.0f &&
                              viewport->translate[0] == 0.0f &&
                              viewport->translate[1] == 0.0f &&
                              viewport->translate[2] == 0.0f &&
                              viewport->translate[3] == 0.0f);

   draw_vs_set_viewport( draw, viewport );
}



void
draw_set_vertex_buffers(struct draw_context *draw,
                        unsigned count,
                        const struct pipe_vertex_buffer *buffers)
{
   assert(count <= PIPE_MAX_ATTRIBS);

   memcpy(draw->pt.vertex_buffer, buffers, count * sizeof(buffers[0]));
   draw->pt.nr_vertex_buffers = count;
}


void
draw_set_vertex_elements(struct draw_context *draw,
                         unsigned count,
                         const struct pipe_vertex_element *elements)
{
   assert(count <= PIPE_MAX_ATTRIBS);

   memcpy(draw->pt.vertex_element, elements, count * sizeof(elements[0]));
   draw->pt.nr_vertex_elements = count;
}


/**
 * Tell drawing context where to find mapped vertex buffers.
 */
void
draw_set_mapped_vertex_buffer(struct draw_context *draw,
                              unsigned attr, const void *buffer)
{
   draw->pt.user.vbuffer[attr] = buffer;
}


void
draw_set_mapped_constant_buffer(struct draw_context *draw,
                                const void *buffer, 
                                unsigned size )
{
   draw->pt.user.constants = buffer;
   draw_vs_set_constants( draw, (const float (*)[4])buffer, size );
}


/**
 * Tells the draw module to draw points with triangles if their size
 * is greater than this threshold.
 */
void
draw_wide_point_threshold(struct draw_context *draw, float threshold)
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   draw->pipeline.wide_point_threshold = threshold;
}


/**
 * Tells the draw module to draw lines with triangles if their width
 * is greater than this threshold.
 */
void
draw_wide_line_threshold(struct draw_context *draw, float threshold)
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   draw->pipeline.wide_line_threshold = threshold;
}


/**
 * Tells the draw module whether or not to implement line stipple.
 */
void
draw_enable_line_stipple(struct draw_context *draw, boolean enable)
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   draw->pipeline.line_stipple = enable;
}


/**
 * Tells draw module whether to convert points to quads for sprite mode.
 */
void
draw_enable_point_sprites(struct draw_context *draw, boolean enable)
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   draw->pipeline.point_sprite = enable;
}


/**
 * Ask the draw module for the location/slot of the given vertex attribute in
 * a post-transformed vertex.
 *
 * With this function, drivers that use the draw module should have no reason
 * to track the current vertex shader.
 *
 * Note that the draw module may sometimes generate vertices with extra
 * attributes (such as texcoords for AA lines).  The driver can call this
 * function to find those attributes.
 *
 * Zero is returned if the attribute is not found since this is
 * a don't care / undefined situtation.  Returning -1 would be a bit more
 * work for the drivers.
 */
int
draw_find_vs_output(const struct draw_context *draw,
                    uint semantic_name, uint semantic_index)
{
   const struct draw_vertex_shader *vs = draw->vs.vertex_shader;
   uint i;
   for (i = 0; i < vs->info.num_outputs; i++) {
      if (vs->info.output_semantic_name[i] == semantic_name &&
          vs->info.output_semantic_index[i] == semantic_index)
         return i;
   }

   /* XXX there may be more than one extra vertex attrib.
    * For example, simulated gl_FragCoord and gl_PointCoord.
    */
   if (draw->extra_vp_outputs.semantic_name == semantic_name &&
       draw->extra_vp_outputs.semantic_index == semantic_index) {
      return draw->extra_vp_outputs.slot;
   }
   return 0;
}


/**
 * Return number of vertex shader outputs.
 */
uint
draw_num_vs_outputs(const struct draw_context *draw)
{
   uint count = draw->vs.vertex_shader->info.num_outputs;
   if (draw->extra_vp_outputs.slot > 0)
      count++;
   return count;
}



void draw_set_render( struct draw_context *draw, 
		      struct vbuf_render *render )
{
   draw->render = render;
}

void draw_set_edgeflags( struct draw_context *draw,
                         const unsigned *edgeflag )
{
   draw->pt.user.edgeflag = edgeflag;
}




/**
 * Tell the drawing context about the index/element buffer to use
 * (ala glDrawElements)
 * If no element buffer is to be used (i.e. glDrawArrays) then this
 * should be called with eltSize=0 and elements=NULL.
 *
 * \param draw  the drawing context
 * \param eltSize  size of each element (1, 2 or 4 bytes)
 * \param elements  the element buffer ptr
 */
void
draw_set_mapped_element_buffer_range( struct draw_context *draw,
                                      unsigned eltSize,
                                      unsigned min_index,
                                      unsigned max_index,
                                      void *elements )
{
   draw->pt.user.elts = elements;
   draw->pt.user.eltSize = eltSize;
   draw->pt.user.min_index = min_index;
   draw->pt.user.max_index = max_index;
}


void
draw_set_mapped_element_buffer( struct draw_context *draw,
                                unsigned eltSize,
                                void *elements )
{
   draw->pt.user.elts = elements;
   draw->pt.user.eltSize = eltSize;
   draw->pt.user.min_index = 0;
   draw->pt.user.max_index = 0xffffffff;
}

 
/* Revamp me please:
 */
void draw_do_flush( struct draw_context *draw, unsigned flags )
{
   if (!draw->suspend_flushing)
   {
      assert(!draw->flushing); /* catch inadvertant recursion */

      draw->flushing = TRUE;

      draw_pipeline_flush( draw, flags );

      draw->reduced_prim = ~0; /* is reduced_prim needed any more? */
      
      draw->flushing = FALSE;
   }
}
