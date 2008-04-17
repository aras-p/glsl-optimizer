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
#include "draw_vbuf.h"
#include "draw_vs.h"


struct draw_context *draw_create( void )
{
   struct draw_context *draw = CALLOC_STRUCT( draw_context );
   if (draw == NULL)
      goto fail;

#if defined(__i386__) || defined(__386__)
   draw->use_sse = GETENV( "GALLIUM_NOSSE" ) == NULL;
#else
   draw->use_sse = FALSE;
#endif

   draw->use_pt_shaders = GETENV( "GALLIUM_PT_SHADERS" ) != NULL;

   /* create pipeline stages */
   draw->pipeline.wide_line  = draw_wide_line_stage( draw );
   draw->pipeline.wide_point = draw_wide_point_stage( draw );
   draw->pipeline.stipple   = draw_stipple_stage( draw );
   draw->pipeline.unfilled  = draw_unfilled_stage( draw );
   draw->pipeline.twoside   = draw_twoside_stage( draw );
   draw->pipeline.offset    = draw_offset_stage( draw );
   draw->pipeline.clip      = draw_clip_stage( draw );
   draw->pipeline.flatshade = draw_flatshade_stage( draw );
   draw->pipeline.cull      = draw_cull_stage( draw );
   draw->pipeline.validate  = draw_validate_stage( draw );
   draw->pipeline.first     = draw->pipeline.validate;

   if (!draw->pipeline.wide_line ||
       !draw->pipeline.wide_point ||
       !draw->pipeline.stipple ||
       !draw->pipeline.unfilled ||
       !draw->pipeline.twoside ||
       !draw->pipeline.offset ||
       !draw->pipeline.clip ||
       !draw->pipeline.flatshade ||
       !draw->pipeline.cull ||
       !draw->pipeline.validate)
      goto fail;


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
      char *tmp = align_malloc(VS_QUEUE_LENGTH * MAX_VERTEX_ALLOCATION, 16);
      if (!tmp)
         goto fail;

      draw->vs.vertex_cache = tmp;
   }

   draw->shader_queue_flush = draw_vertex_shader_queue_flush;

   /* these defaults are oriented toward the needs of softpipe */
   draw->wide_point_threshold = 1000000.0; /* infinity */
   draw->wide_line_threshold = 1.0;
   draw->line_stipple = TRUE;
   draw->point_sprite = TRUE;

   draw->reduced_prim = ~0; /* != any of PIPE_PRIM_x */

   draw_vertex_cache_invalidate( draw );
   draw_set_mapped_element_buffer( draw, 0, NULL );

   tgsi_exec_machine_init(&draw->machine);

   /* FIXME: give this machine thing a proper constructor:
    */
   draw->machine.Inputs = align_malloc(PIPE_MAX_ATTRIBS * sizeof(struct tgsi_exec_vector), 16);
   draw->machine.Outputs = align_malloc(PIPE_MAX_ATTRIBS * sizeof(struct tgsi_exec_vector), 16);


   if (!draw_pt_init( draw ))
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

   if (draw->pipeline.wide_line)
      draw->pipeline.wide_line->destroy( draw->pipeline.wide_line );
   if (draw->pipeline.wide_point)
      draw->pipeline.wide_point->destroy( draw->pipeline.wide_point );
   if (draw->pipeline.stipple)
      draw->pipeline.stipple->destroy( draw->pipeline.stipple );
   if (draw->pipeline.unfilled)
      draw->pipeline.unfilled->destroy( draw->pipeline.unfilled );
   if (draw->pipeline.twoside)
      draw->pipeline.twoside->destroy( draw->pipeline.twoside );
   if (draw->pipeline.offset)
      draw->pipeline.offset->destroy( draw->pipeline.offset );
   if (draw->pipeline.clip)
      draw->pipeline.clip->destroy( draw->pipeline.clip );
   if (draw->pipeline.flatshade)
      draw->pipeline.flatshade->destroy( draw->pipeline.flatshade );
   if (draw->pipeline.cull)
      draw->pipeline.cull->destroy( draw->pipeline.cull );
   if (draw->pipeline.validate)
      draw->pipeline.validate->destroy( draw->pipeline.validate );
   if (draw->pipeline.aaline)
      draw->pipeline.aaline->destroy( draw->pipeline.aaline );
   if (draw->pipeline.aapoint)
      draw->pipeline.aapoint->destroy( draw->pipeline.aapoint );
   if (draw->pipeline.pstipple)
      draw->pipeline.pstipple->destroy( draw->pipeline.pstipple );
   if (draw->pipeline.rasterize)
      draw->pipeline.rasterize->destroy( draw->pipeline.rasterize );

   if (draw->machine.Inputs)
      align_free(draw->machine.Inputs);
   if (draw->machine.Outputs)
      align_free(draw->machine.Outputs);
   tgsi_exec_machine_free_data(&draw->machine);


   if (draw->vs.vertex_cache)
      align_free( draw->vs.vertex_cache ); /* Frees all the vertices. */

   /* Not so fast -- we're just borrowing this at the moment.
    * 
   if (draw->render)
      draw->render->destroy( draw->render );
   */

   draw_pt_destroy( draw );

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
}



void
draw_set_vertex_buffers(struct draw_context *draw,
                        unsigned count,
                        const struct pipe_vertex_buffer *buffers)
{
   assert(count <= PIPE_MAX_ATTRIBS);

   draw_do_flush( draw, DRAW_FLUSH_VERTEX_CACHE/*STATE_CHANGE*/ );

   memcpy(draw->vertex_buffer, buffers, count * sizeof(buffers[0]));
   draw->nr_vertex_buffers = count;
}


void
draw_set_vertex_elements(struct draw_context *draw,
                         unsigned count,
                         const struct pipe_vertex_element *elements)
{
   assert(count <= PIPE_MAX_ATTRIBS);

   draw_do_flush( draw, DRAW_FLUSH_VERTEX_CACHE/*STATE_CHANGE*/ );

   memcpy(draw->vertex_element, elements, count * sizeof(elements[0]));
   draw->nr_vertex_elements = count;
}


/**
 * Tell drawing context where to find mapped vertex buffers.
 */
void
draw_set_mapped_vertex_buffer(struct draw_context *draw,
                              unsigned attr, const void *buffer)
{
   draw_do_flush( draw, DRAW_FLUSH_VERTEX_CACHE/*STATE_CHANGE*/ );
   draw->user.vbuffer[attr] = buffer;
}


void
draw_set_mapped_constant_buffer(struct draw_context *draw,
                                const void *buffer)
{
   draw_do_flush( draw, DRAW_FLUSH_VERTEX_CACHE/*STATE_CHANGE*/ );
   draw->user.constants = buffer;
}


/**
 * Tells the draw module to draw points with triangles if their size
 * is greater than this threshold.
 */
void
draw_wide_point_threshold(struct draw_context *draw, float threshold)
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   draw->wide_point_threshold = threshold;
}


/**
 * Tells the draw module to draw lines with triangles if their width
 * is greater than this threshold.
 */
void
draw_wide_line_threshold(struct draw_context *draw, float threshold)
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   draw->wide_line_threshold = threshold;
}


/**
 * Tells the draw module whether or not to implement line stipple.
 */
void
draw_enable_line_stipple(struct draw_context *draw, boolean enable)
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   draw->line_stipple = enable;
}


/**
 * Tells draw module whether to convert points to quads for sprite mode.
 */
void
draw_enable_point_sprites(struct draw_context *draw, boolean enable)
{
   draw_do_flush( draw, DRAW_FLUSH_STATE_CHANGE );
   draw->point_sprite = enable;
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
draw_find_vs_output(struct draw_context *draw,
                    uint semantic_name, uint semantic_index)
{
   const struct draw_vertex_shader *vs = draw->vertex_shader;
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
draw_num_vs_outputs(struct draw_context *draw)
{
   uint count = draw->vertex_shader->info.num_outputs;
   if (draw->extra_vp_outputs.slot > 0)
      count++;
   return count;
}


/**
 * Allocate space for temporary post-transform vertices, such as for clipping.
 */
void draw_alloc_temp_verts( struct draw_stage *stage, unsigned nr )
{
   assert(!stage->tmp);

   stage->nr_tmps = nr;

   if (nr) {
      ubyte *store = (ubyte *) MALLOC( MAX_VERTEX_SIZE * nr );
      unsigned i;

      stage->tmp = (struct vertex_header **) MALLOC( sizeof(struct vertex_header *) * nr );
      
      for (i = 0; i < nr; i++)
	 stage->tmp[i] = (struct vertex_header *)(store + i * MAX_VERTEX_SIZE);
   }
}


void draw_free_temp_verts( struct draw_stage *stage )
{
   if (stage->tmp) {
      FREE( stage->tmp[0] );
      FREE( stage->tmp );
      stage->tmp = NULL;
   }
}


boolean draw_use_sse(struct draw_context *draw)
{
   return (boolean) draw->use_sse;
}


void draw_reset_vertex_ids(struct draw_context *draw)
{
   struct draw_stage *stage = draw->pipeline.first;
   
   while (stage) {
      unsigned i;

      for (i = 0; i < stage->nr_tmps; i++)
	 stage->tmp[i]->vertex_id = UNDEFINED_VERTEX_ID;

      stage = stage->next;
   }

   draw_vertex_cache_reset_vertex_ids(draw); /* going away soon */
   draw_pt_reset_vertex_ids(draw);
}


void draw_set_render( struct draw_context *draw, 
		      struct vbuf_render *render )
{
   draw->render = render;
}

void draw_set_edgeflags( struct draw_context *draw,
                         const unsigned *edgeflag )
{
   draw->user.edgeflag = edgeflag;
}


boolean draw_get_edgeflag( struct draw_context *draw,
                           unsigned idx )
{
   if (draw->user.edgeflag)
      return (draw->user.edgeflag[idx/32] & (1 << (idx%32))) != 0;
   else
      return 1;
}

