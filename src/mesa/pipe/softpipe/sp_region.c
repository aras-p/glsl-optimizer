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
 * Software-based region allocation and management.
 * A hardware driver would override these functions.
 */


#include "sp_context.h"
#include "sp_region.h"
#include "sp_surface.h"
#include "main/imports.h"


static struct pipe_region *
sp_region_alloc(struct pipe_context *pipe,
                GLuint cpp, GLuint pitch, GLuint height)
{
   struct pipe_region *region = CALLOC_STRUCT(pipe_region);
   if (!region)
      return NULL;

   region->refcount = 1;
   region->cpp = cpp;
   region->pitch = pitch;
   region->height = height;
   region->map = malloc(cpp * pitch * height);

   return region;
}


static void
sp_region_release(struct pipe_context *pipe, struct pipe_region **region)
{
   assert((*region)->refcount > 0);
   (*region)->refcount--;

   if ((*region)->refcount == 0) {
      assert((*region)->map_refcount == 0);

#if 0
      if ((*region)->pbo)
	 (*region)->pbo->region = NULL;
      (*region)->pbo = NULL;
#endif

      free(*region);
   }
   *region = NULL;
}



static GLubyte *
sp_region_map(struct pipe_context *pipe, struct pipe_region *region)
{
   region->map_refcount++;
   return region->map;
}


static void
sp_region_unmap(struct pipe_context *pipe, struct pipe_region *region)
{
   region->map_refcount--;
}


void
sp_init_region_functions(struct softpipe_context *sp)
{
   sp->pipe.region_alloc = sp_region_alloc;
   sp->pipe.region_release = sp_region_release;

   sp->pipe.region_map = sp_region_map;
   sp->pipe.region_unmap = sp_region_unmap;

   /* XXX lots of other region functions needed... */
}
