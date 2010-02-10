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

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */

#include "fo_context.h"

/* This looks like a lot of work at the moment - we're keeping a
 * duplicate copy of the state up-to-date.  
 *
 * This can change in two ways:
 * - With constant state objects we would only need to save a pointer,
 *     not the whole object.
 * - By adding a callback in the state tracker to re-emit state.  The
 *     state tracker knows the current state already and can re-emit it 
 *     without additional complexity.
 *
 * This works as a proof-of-concept, but a final version will have
 * lower overheads.
 */


/* Bring the software pipe uptodate with current state.
 * 
 * With constant state objects we would probably just send all state
 * to both rasterizers all the time???
 */
void
failover_state_emit( struct failover_context *failover )
{
   if (failover->dirty & FO_NEW_BLEND)
      failover->sw->bind_blend_state( failover->sw,
                                      failover->blend->sw_state );

   if (failover->dirty & FO_NEW_BLEND_COLOR)
      failover->sw->set_blend_color( failover->sw, &failover->blend_color );

   if (failover->dirty & FO_NEW_CLIP)
      failover->sw->set_clip_state( failover->sw, &failover->clip );

   if (failover->dirty & FO_NEW_DEPTH_STENCIL)
      failover->sw->bind_depth_stencil_alpha_state( failover->sw,
                                                    failover->depth_stencil->sw_state );

   if (failover->dirty & FO_NEW_STENCIL_REF)
      failover->sw->set_stencil_ref( failover->sw, &failover->stencil_ref );

   if (failover->dirty & FO_NEW_FRAMEBUFFER)
      failover->sw->set_framebuffer_state( failover->sw, &failover->framebuffer );

   if (failover->dirty & FO_NEW_FRAGMENT_SHADER)
      failover->sw->bind_fs_state( failover->sw,
                                   failover->fragment_shader->sw_state );

   if (failover->dirty & FO_NEW_VERTEX_SHADER)
      failover->sw->bind_vs_state( failover->sw,
                                   failover->vertex_shader->sw_state );

   if (failover->dirty & FO_NEW_STIPPLE)
      failover->sw->set_polygon_stipple( failover->sw, &failover->poly_stipple );

   if (failover->dirty & FO_NEW_RASTERIZER)
      failover->sw->bind_rasterizer_state( failover->sw,
                                           failover->rasterizer->sw_state );

   if (failover->dirty & FO_NEW_SCISSOR)
      failover->sw->set_scissor_state( failover->sw, &failover->scissor );

   if (failover->dirty & FO_NEW_VIEWPORT)
      failover->sw->set_viewport_state( failover->sw, &failover->viewport );

   if (failover->dirty & FO_NEW_SAMPLER) {
      failover->sw->bind_fragment_sampler_states( failover->sw, failover->num_samplers,
                                                  failover->sw_sampler_state );
      failover->sw->bind_vertex_sampler_states(failover->sw,
                                               failover->num_vertex_samplers,
                                               failover->sw_vertex_sampler_state);
   }

   if (failover->dirty & FO_NEW_TEXTURE) {
      failover->sw->set_fragment_sampler_textures( failover->sw, failover->num_textures, 
                                                   failover->texture );
      failover->sw->set_vertex_sampler_textures(failover->sw,
                                                failover->num_vertex_textures, 
                                                failover->vertex_textures);
   }

   if (failover->dirty & FO_NEW_VERTEX_BUFFER) {
      failover->sw->set_vertex_buffers( failover->sw,
                                        failover->num_vertex_buffers,
                                        failover->vertex_buffers );
   }

   if (failover->dirty & FO_NEW_VERTEX_ELEMENT) {
      failover->sw->set_vertex_elements( failover->sw,
                                         failover->num_vertex_elements,
                                         failover->vertex_elements );
   }

   failover->dirty = 0;
}
