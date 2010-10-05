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
#include "i915_reg.h"
#include "i915_screen.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_math.h"
#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_pack_color.h"


/* Assumes all values are within bounds -- no checking at this level -
 * do it higher up if required.
 */
static void
i915_surface_copy(struct pipe_context *pipe,
                  struct pipe_resource *dst, struct pipe_subresource subdst,
                  unsigned dstx, unsigned dsty, unsigned dstz,
                  struct pipe_resource *src, struct pipe_subresource subsrc,
                  unsigned srcx, unsigned srcy, unsigned srcz,
                  unsigned width, unsigned height)
{
   struct i915_texture *dst_tex = i915_texture(dst);
   struct i915_texture *src_tex = i915_texture(src);
   struct pipe_resource *dpt = &dst_tex->b.b;
   struct pipe_resource *spt = &src_tex->b.b;
   unsigned dst_offset, src_offset;  /* in bytes */

   if (dst->target == PIPE_TEXTURE_CUBE) {
      dst_offset = dst_tex->image_offset[subdst.level][subdst.face];
   }
   else if (dst->target == PIPE_TEXTURE_3D) {
      dst_offset = dst_tex->image_offset[subdst.level][dstz];
   }
   else {
      dst_offset = dst_tex->image_offset[subdst.level][0];
      assert(subdst.face == 0);
      assert(dstz == 0);
   }
   if (src->target == PIPE_TEXTURE_CUBE) {
      src_offset = src_tex->image_offset[subsrc.level][subsrc.face];
   }
   else if (src->target == PIPE_TEXTURE_3D) {
      src_offset = src_tex->image_offset[subsrc.level][srcz];
   }
   else {
      src_offset = src_tex->image_offset[subsrc.level][0];
      assert(subsrc.face == 0);
      assert(srcz == 0);
   }


   assert( dst != src );
   assert( util_format_get_blocksize(dpt->format) == util_format_get_blocksize(spt->format) );
   assert( util_format_get_blockwidth(dpt->format) == util_format_get_blockwidth(spt->format) );
   assert( util_format_get_blockheight(dpt->format) == util_format_get_blockheight(spt->format) );
   assert( util_format_get_blockwidth(dpt->format) == 1 );
   assert( util_format_get_blockheight(dpt->format) == 1 );

   i915_copy_blit( i915_context(pipe),
                   util_format_get_blocksize(dpt->format),
                   (unsigned short) src_tex->stride, src_tex->buffer, src_offset,
                   (unsigned short) dst_tex->stride, dst_tex->buffer, dst_offset,
                   (short) srcx, (short) srcy, (short) dstx, (short) dsty, (short) width, (short) height );
}


static void
i915_clear_render_target(struct pipe_context *pipe,
                         struct pipe_surface *dst,
                         const float *rgba,
                         unsigned dstx, unsigned dsty,
                         unsigned width, unsigned height)
{
   struct i915_texture *tex = i915_texture(dst->texture);
   struct pipe_resource *pt = &tex->b.b;
   union util_color uc;

   assert(util_format_get_blockwidth(pt->format) == 1);
   assert(util_format_get_blockheight(pt->format) == 1);

   util_pack_color(rgba, dst->format, &uc);
   i915_fill_blit( i915_context(pipe),
                   util_format_get_blocksize(pt->format),
                   XY_COLOR_BLT_WRITE_ALPHA | XY_COLOR_BLT_WRITE_RGB,
                   (unsigned short) tex->stride,
                   tex->buffer, dst->offset,
                   (short) dstx, (short) dsty,
                   (short) width, (short) height,
                   uc.ui );
}

static void
i915_clear_depth_stencil(struct pipe_context *pipe,
                         struct pipe_surface *dst,
                         unsigned clear_flags,
                         double depth,
                         unsigned stencil,
                         unsigned dstx, unsigned dsty,
                         unsigned width, unsigned height)
{
   struct i915_texture *tex = i915_texture(dst->texture);
   struct pipe_resource *pt = &tex->b.b;
   unsigned packedds;
   unsigned mask = 0;

   assert(util_format_get_blockwidth(pt->format) == 1);
   assert(util_format_get_blockheight(pt->format) == 1);

   packedds = util_pack_z_stencil(dst->format, depth, stencil);

   if (clear_flags & PIPE_CLEAR_DEPTH)
      mask |= XY_COLOR_BLT_WRITE_RGB;
   /* XXX presumably this does read-modify-write
      (otherwise this won't work anyway). Hence will only want to
      do it if really have stencil and it isn't cleared */
   if ((clear_flags & PIPE_CLEAR_STENCIL) ||
       (dst->format != PIPE_FORMAT_Z24_UNORM_S8_USCALED))
      mask |= XY_COLOR_BLT_WRITE_ALPHA;

   i915_fill_blit( i915_context(pipe),
                   util_format_get_blocksize(pt->format),
                   mask,
                   (unsigned short) tex->stride,
                   tex->buffer, dst->offset,
                   (short) dstx, (short) dsty,
                   (short) width, (short) height,
                   packedds );
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


void
i915_init_surface_functions(struct i915_context *i915)
{
   i915->base.resource_copy_region = i915_surface_copy;
   i915->base.clear_render_target = i915_clear_render_target;
   i915->base.clear_depth_stencil = i915_clear_depth_stencil;
}

/* No good reason for these to be in the screen.
 */
void
i915_init_screen_surface_functions(struct i915_screen *is)
{
   is->base.get_tex_surface = i915_get_tex_surface;
   is->base.tex_surface_destroy = i915_tex_surface_destroy;
}
