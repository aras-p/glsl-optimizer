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

/* Provide additional functionality on top of bufmgr buffers:
 *   - 2d semantics and blit operations (XXX: remove/simplify blits??)
 *   - refcounting of buffers for multiple images in a buffer.
 *   - refcounting of buffer mappings.
 */

#include "sp_context.h"
#include "sp_winsys.h"
#include "sp_region.h"


static void
sp_region_idle(struct pipe_context *pipe, struct pipe_region *region)
{
   
}


static GLubyte *
sp_region_map(struct pipe_context *pipe, struct pipe_region *region)
{
   struct softpipe_context *sp = softpipe_context( pipe );

   if (!region->map_refcount++) {
      region->map = sp->winsys->buffer_map( sp->winsys,
					    region->buffer );
   }

   return region->map;
}

static void
sp_region_unmap(struct pipe_context *pipe, struct pipe_region *region)
{
   struct softpipe_context *sp = softpipe_context( pipe );

   if (!--region->map_refcount) {
      sp->winsys->buffer_unmap( sp->winsys,
				region->buffer );
      region->map = NULL;
   }
}

static struct pipe_region *
sp_region_alloc(struct pipe_context *pipe,
		GLuint cpp, GLuint pitch, GLuint height)
{
   struct softpipe_context *sp = softpipe_context( pipe );
   struct pipe_region *region = calloc(sizeof(*region), 1);

   region->cpp = cpp;
   region->pitch = pitch;
   region->height = height;     /* needed? */
   region->refcount = 1;

   region->buffer = sp->winsys->create_buffer( sp->winsys, 64 );

   sp->winsys->buffer_data( sp->winsys,
			    region->buffer, 
			    pitch * cpp * height, 
			    NULL );

   return region;
}

static void
sp_region_release(struct pipe_context *pipe, struct pipe_region **region)
{
   struct softpipe_context *sp = softpipe_context( pipe );

   if (!*region)
      return;

   ASSERT((*region)->refcount > 0);
   (*region)->refcount--;

   if ((*region)->refcount == 0) {
      assert((*region)->map_refcount == 0);

      sp->winsys->buffer_unreference( sp->winsys,
				      (*region)->buffer );
      free(*region);
   }
   *region = NULL;
}


/*
 * XXX Move this into core Mesa?
 */
static void
_mesa_copy_rect(GLubyte * dst,
                GLuint cpp,
                GLuint dst_pitch,
                GLuint dst_x,
                GLuint dst_y,
                GLuint width,
                GLuint height,
                const GLubyte * src,
                GLuint src_pitch,
		GLuint src_x, 
		GLuint src_y)
{
   GLuint i;

   dst_pitch *= cpp;
   src_pitch *= cpp;
   dst += dst_x * cpp;
   src += src_x * cpp;
   dst += dst_y * dst_pitch;
   src += src_y * dst_pitch;
   width *= cpp;

   if (width == dst_pitch && width == src_pitch)
      memcpy(dst, src, height * width);
   else {
      for (i = 0; i < height; i++) {
         memcpy(dst, src, width);
         dst += dst_pitch;
         src += src_pitch;
      }
   }
}


/* Upload data to a rectangular sub-region.  Lots of choices how to do this:
 *
 * - memcpy by span to current destination
 * - upload data as new buffer and blit
 *
 * Currently always memcpy.
 */
static void
sp_region_data(struct pipe_context *pipe,
	       struct pipe_region *dst,
	       GLuint dst_offset,
	       GLuint dstx, GLuint dsty,
	       const void *src, GLuint src_pitch,
	       GLuint srcx, GLuint srcy, GLuint width, GLuint height)
{
   _mesa_copy_rect(pipe->region_map(pipe, dst) + dst_offset,
                   dst->cpp,
                   dst->pitch,
                   dstx, dsty, width, height, src, src_pitch, srcx, srcy);

   pipe->region_unmap(pipe, dst);
}

/* Assumes all values are within bounds -- no checking at this level -
 * do it higher up if required.
 */
static void
sp_region_copy(struct pipe_context *pipe,
	       struct pipe_region *dst,
	       GLuint dst_offset,
	       GLuint dstx, GLuint dsty,
	       struct pipe_region *src,
	       GLuint src_offset,
	       GLuint srcx, GLuint srcy, GLuint width, GLuint height)
{
   assert( dst != src );
   assert( dst->cpp == src->cpp );

   _mesa_copy_rect(pipe->region_map(pipe, dst) + dst_offset,
                   dst->cpp,
                   dst->pitch,
                   dstx, dsty, 
		   width, height, 
		   pipe->region_map(pipe, src) + src_offset, 
		   src->pitch, 
		   srcx, srcy);

   pipe->region_unmap(pipe, src);
   pipe->region_unmap(pipe, dst);
}

/* Fill a rectangular sub-region.  Need better logic about when to
 * push buffers into AGP - will currently do so whenever possible.
 */
static GLubyte *
get_pointer(struct pipe_region *dst, GLuint x, GLuint y)
{
   return dst->map + (y * dst->pitch + x) * dst->cpp;
}


static void
sp_region_fill(struct pipe_context *pipe,
               struct pipe_region *dst,
               GLuint dst_offset,
               GLuint dstx, GLuint dsty,
               GLuint width, GLuint height, GLuint value)
{
   GLuint i, j;

   (void)pipe->region_map(pipe, dst);

   switch (dst->cpp) {
   case 1: {
      GLubyte *row = get_pointer(dst, dstx, dsty);
      for (i = 0; i < height; i++) {
	 memset(row, value, width);
	 row += dst->pitch;
      }
   }
   break;
   case 2: {
      GLushort *row = (GLushort *) get_pointer(dst, dstx, dsty);
      for (i = 0; i < height; i++) {
	 for (j = 0; j < width; j++)
	    row[j] = value;
	 row += dst->pitch;
      }
   }
   break;
   case 4: {
      GLuint *row = (GLuint *) get_pointer(dst, dstx, dsty);
      for (i = 0; i < height; i++) {
	 for (j = 0; j < width; j++)
	    row[j] = value;
	 row += dst->pitch;
      }
   }
   break;
   default:
      assert(0);
      break;
   }

   pipe->region_unmap( pipe, dst );
}





void
sp_init_region_functions(struct softpipe_context *sp)
{
   sp->pipe.region_idle = sp_region_idle;
   sp->pipe.region_map = sp_region_map;
   sp->pipe.region_unmap = sp_region_unmap;
   sp->pipe.region_alloc = sp_region_alloc;
   sp->pipe.region_release = sp_region_release;
   sp->pipe.region_data = sp_region_data;
   sp->pipe.region_copy = sp_region_copy;
   sp->pipe.region_fill = sp_region_fill;
}

