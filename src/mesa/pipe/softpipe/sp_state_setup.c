/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "pipe/p_defines.h"
#include "sp_context.h"
#include "sp_state.h"
#include "pipe/draw/draw_context.h"


#if 0
static void validate_prim_pipe( struct softpipe_context *softpipe )
{
   struct prim_stage *next = softpipe->prim.setup;

   /* TODO: make the current primitive part of the state and build
    * shorter pipelines for lines & points.
    */
   if (softpipe->setup.fill_cw != PIPE_POLYGON_MODE_FILL ||
       softpipe->setup.fill_ccw != PIPE_POLYGON_MODE_FILL) {

      softpipe->prim.unfilled->next = next;
      next = softpipe->prim.unfilled;
   }
	 
   if (softpipe->setup.offset_cw ||
       softpipe->setup.offset_ccw) {
      softpipe->prim.offset->next = next;
      next = softpipe->prim.offset;
   }

   if (softpipe->setup.light_twoside) {
      softpipe->prim.twoside->next = next;
      next = softpipe->prim.twoside;
   }

   /* Always run the cull stage as we calculate determinant there
    * also.  Fix this..
    */
   {
      softpipe->prim.cull->next = next;
      next = softpipe->prim.cull;
   }


   /* Clip stage
    */
   {
      softpipe->prim.clip->next = next;
      next = softpipe->prim.clip;
   }

   /* Do software flatshading prior to clipping.  XXX: should only do
    * this for clipped primitives, ie it is a part of the clip
    * routine.
    */
   if (softpipe->setup.flatshade) {
      softpipe->prim.flatshade->next = next;
      next = softpipe->prim.flatshade;
   }
   

   softpipe->prim.first = next;
}

#endif



void softpipe_set_setup_state( struct pipe_context *pipe,
			      const struct pipe_setup_state *setup )
{
   struct softpipe_context *softpipe = softpipe_context(pipe);

   /* pass-through to draw module */
   draw_set_setup_state(softpipe->draw, setup);

   memcpy( &softpipe->setup, setup, sizeof(*setup) );

#if 0
   validate_prim_pipe( softpipe );
#endif
   softpipe->dirty |= SP_NEW_SETUP;
}


