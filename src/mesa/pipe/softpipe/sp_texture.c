/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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
  *   Michel Dänzer <michel@tungstengraphics.com>
  */

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"
#include "pipe/p_util.h"
#include "pipe/p_winsys.h"

#include "sp_context.h"
#include "sp_state.h"
#include "sp_texture.h"


/* Simple, maximally packed layout.
 */

static unsigned minify( unsigned d )
{
   return MAX2(1, d>>1);
}


static void
softpipe_texture_layout(struct softpipe_texture * spt)
{
   struct pipe_texture *pt = &spt->base;
   unsigned level;
   unsigned width = pt->width[0];
   unsigned height = pt->height[0];
   unsigned depth = pt->depth[0];

   spt->buffer_size = 0;

   for ( level = pt->first_level ; level <= pt->last_level ; level++ ) {
      pt->width[level] = width;
      pt->height[level] = height;
      pt->depth[level] = depth;

      spt->level_offset[level] = spt->buffer_size;

      spt->buffer_size += ((pt->compressed) ? MAX2(1, height/4) : height) *
			  ((pt->target == PIPE_TEXTURE_CUBE) ? 6 : depth) *
			  width * pt->cpp;

      width  = minify(width);
      height = minify(height);
      depth = minify(depth);
   }
}


void
softpipe_texture_create(struct pipe_context *pipe, struct pipe_texture **pt)
{
   struct softpipe_texture *spt = REALLOC(*pt, sizeof(struct pipe_texture),
					  sizeof(struct softpipe_texture));

   if (spt) {
      memset(&spt->base + 1, 0,
	     sizeof(struct softpipe_texture) - sizeof(struct pipe_texture));

      softpipe_texture_layout(spt);

      spt->buffer = pipe->winsys->buffer_create(pipe->winsys, 32, 0, 0);

      if (spt->buffer) {
	 pipe->winsys->buffer_data(pipe->winsys, spt->buffer, spt->buffer_size,
				   NULL, PIPE_BUFFER_USAGE_PIXEL);
      }

      if (!spt->buffer) {
	 FREE(spt);
	 spt = NULL;
      }
   }

   *pt = &spt->base;
}

void
softpipe_texture_release(struct pipe_context *pipe, struct pipe_texture **pt)
{
   if (!*pt)
      return;

   /*
   DBG("%s %p refcount will be %d\n",
       __FUNCTION__, (void *) *pt, (*pt)->refcount - 1);
   */
   if (--(*pt)->refcount <= 0) {
      struct softpipe_texture *spt = softpipe_texture(*pt);

      /*
      DBG("%s deleting %p\n", __FUNCTION__, (void *) spt);
      */

      pipe->winsys->buffer_reference(pipe->winsys, &spt->buffer, NULL);

      FREE(spt);
   }
   *pt = NULL;
}
