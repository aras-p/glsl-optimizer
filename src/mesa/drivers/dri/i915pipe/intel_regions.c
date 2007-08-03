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
 *   - 2d semantics and blit operations
 *   - refcounting of buffers for multiple images in a buffer.
 *   - refcounting of buffer mappings.
 *   - some logic for moving the buffers to the best memory pools for
 *     given operations.
 *
 * Most of this is to make it easier to implement the fixed-layout
 * mipmap tree required by intel hardware in the face of GL's
 * programming interface where each image can be specifed in random
 * order and it isn't clear what layout the tree should have until the
 * last moment.
 */

#include "pipe/p_state.h"
#include "pipe/p_context.h"

#include "intel_context.h"
#include "intel_blit.h"
#include "intel_buffer_objects.h"
#include "dri_bufmgr.h"
#include "intel_batchbuffer.h"


#define FILE_DEBUG_FLAG DEBUG_REGION


/** XXX temporary helper */
static intelScreenPrivate *
pipe_screen(struct pipe_context *pipe)
{
   return (intelScreenPrivate *) pipe->screen;
}


static void
intel_region_idle(struct pipe_context *pipe, struct pipe_region *region)
{
   DBG("%s\n", __FUNCTION__);
   if (region && region->buffer)
      driBOWaitIdle(region->buffer, GL_FALSE);
}

/* XXX: Thread safety?
 */
static GLubyte *
intel_region_map(struct pipe_context *pipe, struct pipe_region *region)
{
   DBG("%s\n", __FUNCTION__);
   if (!region->map_refcount++) {
      region->map = driBOMap(region->buffer,
                             DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE, 0);
   }

   return region->map;
}

static void
intel_region_unmap(struct pipe_context *pipe, struct pipe_region *region)
{
   DBG("%s\n", __FUNCTION__);
   if (!--region->map_refcount) {
      driBOUnmap(region->buffer);
      region->map = NULL;
   }
}

static struct pipe_region *
intel_region_alloc(struct pipe_context *pipe,
                   GLuint cpp, GLuint pitch, GLuint height)
{
   intelScreenPrivate *intelScreen = pipe_screen(pipe);
   struct pipe_region *region = calloc(sizeof(*region), 1);
   struct intel_context *intel = intelScreenContext(intelScreen);

   DBG("%s\n", __FUNCTION__);

   region->cpp = cpp;
   region->pitch = pitch;
   region->height = height;     /* needed? */
   region->refcount = 1;

   driGenBuffers(intelScreen->regionPool,
                 "region", 1, &region->buffer, 64,
		 0,
		 0);

   LOCK_HARDWARE(intel);
   driBOData(region->buffer, pitch * cpp * height, NULL, 0);
   UNLOCK_HARDWARE(intel);
   return region;
}

static void
intel_region_release(struct pipe_context *pipe, struct pipe_region **region)
{
   if (!*region)
      return;

   DBG("%s %d\n", __FUNCTION__, (*region)->refcount - 1);

   ASSERT((*region)->refcount > 0);
   (*region)->refcount--;

   if ((*region)->refcount == 0) {
      assert((*region)->map_refcount == 0);

      driBOUnReference((*region)->buffer);
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
                GLuint src_pitch, GLuint src_x, GLuint src_y)
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
intel_region_data(struct pipe_context *pipe,
                  struct pipe_region *dst,
                  GLuint dst_offset,
                  GLuint dstx, GLuint dsty,
                  const void *src, GLuint src_pitch,
                  GLuint srcx, GLuint srcy, GLuint width, GLuint height)
{
   intelScreenPrivate *intelScreen = pipe_screen(pipe);
   struct intel_context *intel = intelScreenContext(intelScreen);

   DBG("%s\n", __FUNCTION__);

   if (intel == NULL)
      return;

   LOCK_HARDWARE(intel);

   _mesa_copy_rect(pipe->region_map(pipe, dst) + dst_offset,
                   dst->cpp,
                   dst->pitch,
                   dstx, dsty, width, height, src, src_pitch, srcx, srcy);

   pipe->region_unmap(pipe, dst);

   UNLOCK_HARDWARE(intel);

}

/* Copy rectangular sub-regions. Need better logic about when to
 * push buffers into AGP - will currently do so whenever possible.
 */
static void
intel_region_copy(struct pipe_context *pipe,
                  struct pipe_region *dst,
                  GLuint dst_offset,
                  GLuint dstx, GLuint dsty,
                  const struct pipe_region *src,
                  GLuint src_offset,
                  GLuint srcx, GLuint srcy, GLuint width, GLuint height)
{
   intelScreenPrivate *intelScreen = pipe_screen(pipe);
   struct intel_context *intel = intelScreenContext(intelScreen);

   DBG("%s\n", __FUNCTION__);

   if (intel == NULL)
      return;

   assert(src->cpp == dst->cpp);

   intelEmitCopyBlit(intel,
                     dst->cpp,
                     src->pitch, src->buffer, src_offset,
                     dst->pitch, dst->buffer, dst_offset,
                     srcx, srcy, dstx, dsty, width, height,
		     GL_COPY);
}

/* Fill a rectangular sub-region.  Need better logic about when to
 * push buffers into AGP - will currently do so whenever possible.
 */
static void
intel_region_fill(struct pipe_context *pipe,
                  struct pipe_region *dst,
                  GLuint dst_offset,
                  GLuint dstx, GLuint dsty,
                  GLuint width, GLuint height,
                  GLuint value, GLuint mask)
{
   intelScreenPrivate *intelScreen = pipe_screen(pipe);
   struct intel_context *intel = intelScreenContext(intelScreen);

   DBG("%s\n", __FUNCTION__);

   if (intel == NULL)
      return;   

   intelEmitFillBlit(intel,
                     dst->cpp,
                     dst->pitch, dst->buffer, dst_offset,
                     dstx, dsty, width, height, value, mask);
}



static struct _DriBufferObject *
intel_region_buffer(struct pipe_context *pipe,
                    struct pipe_region *region, GLuint flag)
{
   return region->buffer;
}



void
intel_init_region_functions(struct pipe_context *pipe)
{
   pipe->region_idle = intel_region_idle;
   pipe->region_map = intel_region_map;
   pipe->region_unmap = intel_region_unmap;
   pipe->region_alloc = intel_region_alloc;
   pipe->region_release = intel_region_release;
   pipe->region_data = intel_region_data;
   pipe->region_copy = intel_region_copy;
   pipe->region_fill = intel_region_fill;
   pipe->region_buffer = intel_region_buffer;
}

