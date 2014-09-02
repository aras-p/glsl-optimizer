/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2014 Intel Corporation All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Jason Ekstrand <jason.ekstrand@intel.com>
 */

#include "intel_tex.h"
#include "intel_blit.h"
#include "intel_mipmap_tree.h"
#include "main/formats.h"
#include "drivers/common/meta.h"

static bool
copy_image_with_blitter(struct brw_context *brw,
                        struct intel_mipmap_tree *src_mt, int src_level,
                        int src_x, int src_y, int src_z,
                        struct intel_mipmap_tree *dst_mt, int dst_level,
                        int dst_x, int dst_y, int dst_z,
                        int src_width, int src_height)
{
   GLuint bw, bh;
   uint32_t src_image_x, src_image_y, dst_image_x, dst_image_y;
   int cpp;

   /* The blitter doesn't understand multisampling at all. */
   if (src_mt->num_samples > 0 || dst_mt->num_samples > 0)
      return false;

   /* According to the Ivy Bridge PRM, Vol1 Part4, section 1.2.1.2 (Graphics
    * Data Size Limitations):
    *
    *    The BLT engine is capable of transferring very large quantities of
    *    graphics data. Any graphics data read from and written to the
    *    destination is permitted to represent a number of pixels that
    *    occupies up to 65,536 scan lines and up to 32,768 bytes per scan line
    *    at the destination. The maximum number of pixels that may be
    *    represented per scan line’s worth of graphics data depends on the
    *    color depth.
    *
    * Furthermore, intelEmitCopyBlit (which is called below) uses a signed
    * 16-bit integer to represent buffer pitch, so it can only handle buffer
    * pitches < 32k.
    *
    * As a result of these two limitations, we can only use the blitter to do
    * this copy when the miptree's pitch is less than 32k.
    */
   if (src_mt->pitch >= 32768 ||
       dst_mt->pitch >= 32768) {
      perf_debug("Falling back due to >=32k pitch\n");
      return false;
   }

   intel_miptree_get_image_offset(src_mt, src_level, src_z,
                                  &src_image_x, &src_image_y);

   if (_mesa_is_format_compressed(src_mt->format)) {
      _mesa_get_format_block_size(src_mt->format, &bw, &bh);

      assert(src_x % bw == 0);
      assert(src_y % bh == 0);
      assert(src_width % bw == 0);
      assert(src_height % bh == 0);

      src_x /= (int)bw;
      src_y /= (int)bh;
      src_width /= (int)bw;
      src_height /= (int)bh;

      /* Inside of the miptree, the x offsets are stored in pixels while
       * the y offsets are stored in blocks.  We need to scale just the x
       * offset.
       */
      src_image_x /= bw;

      cpp = _mesa_get_format_bytes(src_mt->format);
   } else {
      cpp = src_mt->cpp;
   }
   src_x += src_image_x;
   src_y += src_image_y;

   intel_miptree_get_image_offset(dst_mt, dst_level, dst_z,
                                  &dst_image_x, &dst_image_y);

   if (_mesa_is_format_compressed(dst_mt->format)) {
      _mesa_get_format_block_size(dst_mt->format, &bw, &bh);

      assert(dst_x % bw == 0);
      assert(dst_y % bh == 0);

      dst_x /= (int)bw;
      dst_y /= (int)bh;

      /* Inside of the miptree, the x offsets are stored in pixels while
       * the y offsets are stored in blocks.  We need to scale just the x
       * offset.
       */
      dst_image_x /= bw;
   }
   dst_x += dst_image_x;
   dst_y += dst_image_y;

   return intelEmitCopyBlit(brw,
                            cpp,
                            src_mt->pitch,
                            src_mt->bo, src_mt->offset,
                            src_mt->tiling,
                            dst_mt->pitch,
                            dst_mt->bo, dst_mt->offset,
                            dst_mt->tiling,
                            src_x, src_y,
                            dst_x, dst_y,
                            src_width, src_height,
                            GL_COPY);
}

static void
copy_image_with_memcpy(struct brw_context *brw,
                       struct intel_mipmap_tree *src_mt, int src_level,
                       int src_x, int src_y, int src_z,
                       struct intel_mipmap_tree *dst_mt, int dst_level,
                       int dst_x, int dst_y, int dst_z,
                       int src_width, int src_height)
{
   bool same_slice;
   uint8_t *mapped, *src_mapped, *dst_mapped;
   int src_stride, dst_stride, i, cpp;
   int map_x1, map_y1, map_x2, map_y2;
   GLuint src_bw, src_bh;

   cpp = _mesa_get_format_bytes(src_mt->format);
   _mesa_get_format_block_size(src_mt->format, &src_bw, &src_bh);

   assert(src_width % src_bw == 0);
   assert(src_height % src_bw == 0);
   assert(src_x % src_bw == 0);
   assert(src_y % src_bw == 0);

   /* If we are on the same miptree, same level, and same slice, then
    * intel_miptree_map won't let us map it twice.  We have to do things a
    * bit differently.  In particular, we do a single map large enough for
    * both portions and in read-write mode.
    */
   same_slice = src_mt == dst_mt && src_level == dst_level && src_z == dst_z;

   if (same_slice) {
      assert(dst_x % src_bw == 0);
      assert(dst_y % src_bw == 0);

      map_x1 = MIN2(src_x, dst_x);
      map_y1 = MIN2(src_y, dst_y);
      map_x2 = MAX2(src_x, dst_x) + src_width;
      map_y2 = MAX2(src_y, dst_y) + src_height;

      intel_miptree_map(brw, src_mt, src_level, src_z,
                        map_x1, map_y1, map_x2 - map_x1, map_y2 - map_y1,
                        GL_MAP_READ_BIT | GL_MAP_WRITE_BIT,
                        (void **)&mapped, &src_stride);

      dst_stride = src_stride;

      /* Set the offsets here so we don't have to think about while looping */
      src_mapped = mapped + ((src_y - map_y1) / src_bh) * src_stride +
                            ((src_x - map_x1) / src_bw) * cpp;
      dst_mapped = mapped + ((dst_y - map_y1) / src_bh) * dst_stride +
                            ((dst_x - map_x1) / src_bw) * cpp;
   } else {
      intel_miptree_map(brw, src_mt, src_level, src_z,
                        src_x, src_y, src_width, src_height,
                        GL_MAP_READ_BIT, (void **)&src_mapped, &src_stride);
      intel_miptree_map(brw, dst_mt, dst_level, dst_z,
                        dst_x, dst_y, src_width, src_height,
                        GL_MAP_WRITE_BIT, (void **)&dst_mapped, &dst_stride);
   }

   src_width /= (int)src_bw;
   src_height /= (int)src_bh;

   for (i = 0; i < src_height; ++i) {
      memcpy(dst_mapped, src_mapped, src_width * cpp);
      src_mapped += src_stride;
      dst_mapped += dst_stride;
   }

   if (same_slice) {
      intel_miptree_unmap(brw, src_mt, src_level, src_z);
   } else {
      intel_miptree_unmap(brw, dst_mt, dst_level, dst_z);
      intel_miptree_unmap(brw, src_mt, src_level, src_z);
   }
}

static void
intel_copy_image_sub_data(struct gl_context *ctx,
                          struct gl_texture_image *src_image,
                          int src_x, int src_y, int src_z,
                          struct gl_texture_image *dst_image,
                          int dst_x, int dst_y, int dst_z,
                          int src_width, int src_height)
{
   struct brw_context *brw = brw_context(ctx);
   struct intel_texture_image *intel_src_image = intel_texture_image(src_image);
   struct intel_texture_image *intel_dst_image = intel_texture_image(dst_image);

   if (_mesa_meta_CopyImageSubData_uncompressed(ctx,
                                                src_image, src_x, src_y, src_z,
                                                dst_image, dst_x, dst_y, dst_z,
                                                src_width, src_height)) {
      return;
   }

   if (intel_src_image->mt->num_samples > 0 ||
       intel_dst_image->mt->num_samples > 0) {
      _mesa_problem(ctx, "Failed to copy multisampled texture with meta path\n");
      return;
   }

   /* Cube maps actually have different images per face */
   if (src_image->TexObject->Target == GL_TEXTURE_CUBE_MAP)
      src_z = src_image->Face;
   if (dst_image->TexObject->Target == GL_TEXTURE_CUBE_MAP)
      dst_z = dst_image->Face;

   /* We are now going to try and copy the texture using the blitter.  If
    * that fails, we will fall back mapping the texture and using memcpy.
    * In either case, we need to do a full resolve.
    */
   intel_miptree_all_slices_resolve_hiz(brw, intel_src_image->mt);
   intel_miptree_all_slices_resolve_depth(brw, intel_src_image->mt);
   intel_miptree_resolve_color(brw, intel_src_image->mt);

   intel_miptree_all_slices_resolve_hiz(brw, intel_dst_image->mt);
   intel_miptree_all_slices_resolve_depth(brw, intel_dst_image->mt);
   intel_miptree_resolve_color(brw, intel_dst_image->mt);

   unsigned src_level = src_image->Level + src_image->TexObject->MinLevel;
   unsigned dst_level = dst_image->Level + dst_image->TexObject->MinLevel;
   if (copy_image_with_blitter(brw, intel_src_image->mt, src_level,
                               src_x, src_y, src_z,
                               intel_dst_image->mt, dst_level,
                               dst_x, dst_y, dst_z,
                               src_width, src_height))
      return;

   /* This is a worst-case scenario software fallback that maps the two
    * textures and does a memcpy between them.
    */
   copy_image_with_memcpy(brw, intel_src_image->mt, src_level,
                          src_x, src_y, src_z,
                          intel_dst_image->mt, dst_level,
                          dst_x, dst_y, dst_z,
                          src_width, src_height);
}

void
intelInitCopyImageFuncs(struct dd_function_table *functions)
{
   functions->CopyImageSubData = intel_copy_image_sub_data;
}
