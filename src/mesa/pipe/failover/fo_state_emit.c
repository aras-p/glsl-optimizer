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

void
failover_state_emit( struct failover_context *failover )
{
   unsigned i;

   if (failover->dirty & FO_NEW_ALPHA_TEST)
      failover->hw->set_alpha_test_state( failover->hw, &failover->alpha_test );

   if (failover->dirty & FO_NEW_BLEND)
      failover->hw->set_blend_state( failover->hw, &failover->blend );

   if (failover->dirty & FO_NEW_BLEND_COLOR)
      failover->hw->set_blend_color( failover->hw, &failover->blend_color );

   if (failover->dirty & FO_NEW_CLIP)
      failover->hw->set_clip_state( failover->hw, &failover->clip );

   if (failover->dirty & FO_NEW_CLEAR_COLOR)
      failover->hw->set_clear_color_state( failover->hw, &failover->clear_color );

   if (failover->dirty & FO_NEW_DEPTH_TEST)
      failover->hw->set_depth_state( failover->hw, &failover->depth_test );

   if (failover->dirty & FO_NEW_FRAMEBUFFER)
      failover->hw->set_framebuffer_state( failover->hw, &failover->framebuffer );

   if (failover->dirty & FO_NEW_FRAGMENT_SHADER)
      failover->hw->set_fs_state( failover->hw, &failover->fragment_shader );

   if (failover->dirty & FO_NEW_VERTEX_SHADER)
      failover->hw->set_vs_state( failover->hw, &failover->vertex_shader );

   if (failover->dirty & FO_NEW_STIPPLE)
      failover->hw->set_polygon_stipple( failover->hw, &failover->poly_stipple );

   if (failover->dirty & FO_NEW_SETUP)
      failover->hw->set_setup_state( failover->hw, &failover->setup );

   if (failover->dirty & FO_NEW_SCISSOR)
      failover->hw->set_scissor_state( failover->hw, &failover->scissor );

   if (failover->dirty & FO_NEW_STENCIL)
      failover->hw->set_stencil_state( failover->hw, &failover->stencil );

   if (failover->dirty & FO_NEW_VIEWPORT)
      failover->hw->set_viewport_state( failover->hw, &failover->viewport );

   if (failover->dirty & FO_NEW_SAMPLER) {
      for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
	 if (failover->dirty_sampler & (1<<i)) {
	    failover->hw->set_sampler_state( failover->hw, i, 
					     &failover->sampler[i] );
	 }
      }
   }

   if (failover->dirty & FO_NEW_TEXTURE) {
      for (i = 0; i < PIPE_MAX_SAMPLERS; i++) {
	 if (failover->dirty_texture & (1<<i)) {
	    failover->hw->set_texture_state( failover->hw, i, 
					     failover->texture[i] );
	 }
      }
   }

   if (failover->dirty & FO_NEW_VERTEX_BUFFER) {
      for (i = 0; i < PIPE_ATTRIB_MAX; i++) {
	 if (failover->dirty_vertex_buffer & (1<<i)) {
	    failover->hw->set_vertex_buffer( failover->hw, i, 
					     &failover->vertex_buffer[i] );
	 }
      }
   }

   if (failover->dirty & FO_NEW_VERTEX_ELEMENT) {
      for (i = 0; i < PIPE_ATTRIB_MAX; i++) {
	 if (failover->dirty_vertex_element & (1<<i)) {
	    failover->hw->set_vertex_element( failover->hw, i, 
					      &failover->vertex_element[i] );
	 }
      }
   }

   failover->dirty = 0;
}
