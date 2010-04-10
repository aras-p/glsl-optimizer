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

#include "i915_surface.h"
#include "i915_resource.h"
#include "i915_blit.h"
#include "i915_screen.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_math.h"
#include "util/u_format.h"
#include "util/u_memory.h"


/* Assumes all values are within bounds -- no checking at this level -
 * do it higher up if required.
 */
static void
i915_surface_copy(struct pipe_context *pipe,
		  struct pipe_surface *dst,
		  unsigned dstx, unsigned dsty,
		  struct pipe_surface *src,
		  unsigned srcx, unsigned srcy, unsigned width, unsigned height)
{
   struct i915_texture *dst_tex = i915_texture(dst->texture);
   struct i915_texture *src_tex = i915_texture(src->texture);
   struct pipe_resource *dpt = &dst_tex->b.b;
   struct pipe_resource *spt = &src_tex->b.b;

   assert( dst != src );
   assert( util_format_get_blocksize(dpt->format) == util_format_get_blocksize(spt->format) );
   assert( util_format_get_blockwidth(dpt->format) == util_format_get_blockwidth(spt->format) );
   assert( util_format_get_blockheight(dpt->format) == util_format_get_blockheight(spt->format) );
   assert( util_format_get_blockwidth(dpt->format) == 1 );
   assert( util_format_get_blockheight(dpt->format) == 1 );

   i915_copy_blit( i915_context(pipe),
                   FALSE,
                   util_format_get_blocksize(dpt->format),
                   (unsigned short) src_tex->stride, src_tex->buffer, src->offset,
                   (unsigned short) dst_tex->stride, dst_tex->buffer, dst->offset,
                   (short) srcx, (short) srcy, (short) dstx, (short) dsty, (short) width, (short) height );
}


static void
i915_surface_fill(struct pipe_context *pipe,
		  struct pipe_surface *dst,
		  unsigned dstx, unsigned dsty,
		  unsigned width, unsigned height, unsigned value)
{
   struct i915_texture *tex = i915_texture(dst->texture);
   struct pipe_resource *pt = &tex->b.b;

   assert(util_format_get_blockwidth(pt->format) == 1);
   assert(util_format_get_blockheight(pt->format) == 1);

   i915_fill_blit( i915_context(pipe),
                   util_format_get_blocksize(pt->format),
                   (unsigned short) tex->stride,
                   tex->buffer, dst->offset,
                   (short) dstx, (short) dsty,
                   (short) width, (short) height,
                   value );
}


/*
 * Screen surface functions
 */


static struct pipe_surface *
i915_get_tex_surface(struct pipe_screen *screen,
                     struct pipe_resource *pt,
                     unsigned face, unsigned level, unsigned zslice,
                     unsigned flags)
{
   struct i915_texture *tex = i915_texture(pt);
   struct pipe_surface *ps;
   unsigned offset;  /* in bytes */

   if (pt->target == PIPE_TEXTURE_CUBE) {
      offset = tex->image_offset[level][face];
   }
   else if (pt->target == PIPE_TEXTURE_3D) {
      offset = tex->image_offset[level][zslice];
   }
   else {
      offset = tex->image_offset[level][0];
      assert(face == 0);
      assert(zslice == 0);
   }

   ps = CALLOC_STRUCT(pipe_surface);
   if (ps) {
      pipe_reference_init(&ps->reference, 1);
      pipe_resource_reference(&ps->texture, pt);
      ps->format = pt->format;
      ps->width = u_minify(pt->width0, level);
      ps->height = u_minify(pt->height0, level);
      ps->offset = offset;
      ps->usage = flags;
   }
   return ps;
}

static void
i915_tex_surface_destroy(struct pipe_surface *surf)
{
   pipe_resource_reference(&surf->texture, NULL);
   FREE(surf);
}


/* Probably going to make blits work on textures rather than surfaces.
 */
void
i915_init_surface_functions(struct i915_context *i915)
{
   i915->base.surface_copy = i915_surface_copy;
   i915->base.surface_fill = i915_surface_fill;
}

/* No good reason for these to be in the screen.
 */
void
i915_init_screen_surface_functions(struct i915_screen *is)
{
   is->base.get_tex_surface = i915_get_tex_surface;
   is->base.tex_surface_destroy = i915_tex_surface_destroy;
}
