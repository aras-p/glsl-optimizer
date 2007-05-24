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

#include "g_context.h"
#include "g_state.h"
#include "g_prim.h"




static void validate_prim_pipe( struct generic_context *generic )
{
   struct prim_stage *next = generic->prim.setup;

   /* TODO: make the current primitive part of the state and build
    * shorter pipelines for lines & points.
    */
   if (generic->setup.fill_cw != FILL_TRI ||
       generic->setup.fill_ccw != FILL_TRI) {

      generic->prim.unfilled->next = next;
      next = generic->prim.unfilled;
   }
	 
   if (generic->setup.offset_cw ||
       generic->setup.offset_ccw) {
      generic->prim.offset->next = next;
      next = generic->prim.offset;
   }

   if (generic->setup.light_twoside) {
      generic->prim.twoside->next = next;
      next = generic->prim.twoside;
   }

   /* Always run the cull stage as we calculate determinant there
    * also.  Fix this..
    */
   {
      generic->prim.cull->next = next;
      next = generic->prim.cull;
   }


   /* Clip stage
    */
   {
      generic->prim.clip->next = next;
      next = generic->prim.clip;
   }

   /* Do software flatshading prior to clipping.  XXX: should only do
    * this for clipped primitives, ie it is a part of the clip
    * routine.
    */
   if (generic->setup.flatshade) {
      generic->prim.flatshade->next = next;
      next = generic->prim.flatshade;
   }
   

   generic->prim.first = next;
}




void generic_set_setup_state( struct softpipe_context *softpipe,
			      const struct softpipe_setup_state *setup )
{
   struct generic_context *generic = generic_context(softpipe);

   memcpy( &generic->setup, setup, sizeof(*setup) );

   validate_prim_pipe( generic );
   generic->dirty |= G_NEW_SETUP;
}



void generic_set_scissor_rect( struct softpipe_context *softpipe,
			       const struct softpipe_scissor_rect *scissor )
{
   struct generic_context *generic = generic_context(softpipe);

   memcpy( &generic->scissor, scissor, sizeof(*scissor) );
   generic->dirty |= G_NEW_SCISSOR;
}


void generic_set_polygon_stipple( struct softpipe_context *softpipe,
				  const struct softpipe_poly_stipple *stipple )
{
   struct generic_context *generic = generic_context(softpipe);

   memcpy( &generic->poly_stipple, stipple, sizeof(*stipple) );
   generic->dirty |= G_NEW_STIPPLE;
}
