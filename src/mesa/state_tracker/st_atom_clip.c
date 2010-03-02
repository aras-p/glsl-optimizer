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
 

#include "st_context.h"
#include "pipe/p_context.h"
#include "st_atom.h"

#include "cso_cache/cso_context.h"


/* Second state atom for user clip planes:
 */
static void update_clip( struct st_context *st )
{
   struct pipe_clip_state clip;
   GLuint i;

   memset(&clip, 0, sizeof(clip));

   for (i = 0; i < PIPE_MAX_CLIP_PLANES; i++) {
      if (st->ctx->Transform.ClipPlanesEnabled & (1 << i)) {
	 memcpy(clip.ucp[clip.nr], 
		st->ctx->Transform._ClipUserPlane[i], 
		sizeof(clip.ucp[0]));
	 clip.nr++;
      }
   }
      
   if (memcmp(&clip, &st->state.clip, sizeof(clip)) != 0) {
      st->state.clip = clip;
      cso_set_clip(st->cso_context, &clip);
   }
}


const struct st_tracked_state st_update_clip = {
   "st_update_clip",					/* name */
   {							/* dirty */
      (_NEW_TRANSFORM),					/* mesa */
      0,						/* st */
   },
   update_clip						/* update */
};
