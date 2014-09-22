/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2013 LunarG, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#include "genhw/genhw.h"
#include "util/u_pack_color.h"

#include "ilo_builder_mi.h"
#include "ilo_builder_blt.h"
#include "ilo_context.h"
#include "ilo_cp.h"
#include "ilo_blit.h"
#include "ilo_resource.h"
#include "ilo_blitter.h"

static uint32_t
ilo_blitter_blt_begin(struct ilo_blitter *blitter, int max_cmd_size,
                      struct intel_bo *dst, enum intel_tiling_mode dst_tiling,
                      struct intel_bo *src, enum intel_tiling_mode src_tiling)
{
   struct ilo_cp *cp = blitter->ilo->cp;
   struct intel_bo *aper_check[2];
   int count;
   uint32_t swctrl;

   /* change owner */
   ilo_cp_set_owner(cp, INTEL_RING_BLT, NULL);

   /* check aperture space */
   aper_check[0] = dst;
   count = 1;

   if (src) {
      aper_check[1] = src;
      count++;
   }

   if (!ilo_builder_validate(&cp->builder, count, aper_check))
      ilo_cp_submit(cp, "out of aperture");

   /* set BCS_SWCTRL */
   swctrl = 0x0;

   if (dst_tiling == INTEL_TILING_Y) {
      swctrl |= GEN6_REG_BCS_SWCTRL_DST_TILING_Y << 16 |
                GEN6_REG_BCS_SWCTRL_DST_TILING_Y;
   }

   if (src && src_tiling == INTEL_TILING_Y) {
      swctrl |= GEN6_REG_BCS_SWCTRL_SRC_TILING_Y << 16 |
                GEN6_REG_BCS_SWCTRL_SRC_TILING_Y;
   }

   /*
    * Most clients expect BLT engine to be stateless.  If we have to set
    * BCS_SWCTRL to a non-default value, we have to set it back in the same
    * batch buffer.
    */
   if (swctrl)
      max_cmd_size += (4 + 3) * 2;

   if (ilo_cp_space(cp) < max_cmd_size) {
      ilo_cp_submit(cp, "out of space");
      assert(ilo_cp_space(cp) >= max_cmd_size);
   }

   if (swctrl) {
      /*
       * From the Ivy Bridge PRM, volume 1 part 4, page 133:
       *
       *     "SW is required to flush the HW before changing the polarity of
       *      this bit (Tile Y Destination/Source)."
       */
      gen6_MI_FLUSH_DW(&cp->builder);
      gen6_MI_LOAD_REGISTER_IMM(&cp->builder, GEN6_REG_BCS_SWCTRL, swctrl);

      swctrl &= ~(GEN6_REG_BCS_SWCTRL_DST_TILING_Y |
                  GEN6_REG_BCS_SWCTRL_SRC_TILING_Y);
   }

   return swctrl;
}

static void
ilo_blitter_blt_end(struct ilo_blitter *blitter, uint32_t swctrl)
{
   struct ilo_builder *builder = &blitter->ilo->cp->builder;

   /* set BCS_SWCTRL back */
   if (swctrl) {
      gen6_MI_FLUSH_DW(builder);
      gen6_MI_LOAD_REGISTER_IMM(builder, GEN6_REG_BCS_SWCTRL, swctrl);
   }
}

static bool
buf_clear_region(struct ilo_blitter *blitter,
                 struct ilo_buffer *buf, unsigned offset,
                 uint32_t val, unsigned size,
                 enum gen6_blt_mask value_mask,
                 enum gen6_blt_mask write_mask)
{
   const uint8_t rop = 0xf0; /* PATCOPY */
   const int cpp = gen6_blt_translate_value_cpp(value_mask);
   struct ilo_builder *builder = &blitter->ilo->cp->builder;
   struct gen6_blt_bo dst;

   if (offset % cpp || size % cpp)
      return false;

   dst.bo = buf->bo;
   dst.offset = offset;

   ilo_blitter_blt_begin(blitter, GEN6_COLOR_BLT__SIZE *
         (1 + size / 32764 / gen6_blt_max_scanlines),
         dst.bo, INTEL_TILING_NONE, NULL, INTEL_TILING_NONE);

   while (size) {
      unsigned width, height;

      width = size;
      height = 1;

      if (width > gen6_blt_max_bytes_per_scanline) {
         /* less than INT16_MAX and dword-aligned */
         width = 32764;
         height = size / width;
         if (height > gen6_blt_max_scanlines)
            height = gen6_blt_max_scanlines;

         dst.pitch = width;
      } else {
         dst.pitch = 0;
      }

      gen6_COLOR_BLT(builder, &dst, val,
            width, height, rop, value_mask, write_mask);

      dst.offset += dst.pitch * height;
      size -= width * height;
   }

   ilo_blitter_blt_end(blitter, 0);

   return true;
}

static bool
buf_copy_region(struct ilo_blitter *blitter,
                struct ilo_buffer *dst_buf, unsigned dst_offset,
                struct ilo_buffer *src_buf, unsigned src_offset,
                unsigned size)
{
   const uint8_t rop = 0xcc; /* SRCCOPY */
   struct ilo_builder *builder = &blitter->ilo->cp->builder;
   struct gen6_blt_bo dst, src;

   dst.bo = dst_buf->bo;
   dst.offset = dst_offset;
   dst.pitch = 0;

   src.bo = src_buf->bo;
   src.offset = src_offset;
   src.pitch = 0;

   ilo_blitter_blt_begin(blitter, GEN6_SRC_COPY_BLT__SIZE *
         (1 + size / 32764 / gen6_blt_max_scanlines),
         dst_buf->bo, INTEL_TILING_NONE, src_buf->bo, INTEL_TILING_NONE);

   while (size) {
      unsigned width, height;

      width = size;
      height = 1;

      if (width > gen6_blt_max_bytes_per_scanline) {
         /* less than INT16_MAX and dword-aligned */
         width = 32764;
         height = size / width;
         if (height > gen6_blt_max_scanlines)
            height = gen6_blt_max_scanlines;

         dst.pitch = width;
         src.pitch = width;
      } else {
         dst.pitch = 0;
         src.pitch = 0;
      }

      gen6_SRC_COPY_BLT(builder, &dst, &src,
            width, height, rop, GEN6_BLT_MASK_8, GEN6_BLT_MASK_8);

      dst.offset += dst.pitch * height;
      src.offset += src.pitch * height;
      size -= width * height;
   }

   ilo_blitter_blt_end(blitter, 0);

   return true;
}

static bool
tex_clear_region(struct ilo_blitter *blitter,
                 struct ilo_texture *dst_tex, unsigned dst_level,
                 const struct pipe_box *dst_box,
                 uint32_t val,
                 enum gen6_blt_mask value_mask,
                 enum gen6_blt_mask write_mask)
{
   const int cpp = gen6_blt_translate_value_cpp(value_mask);
   const unsigned max_extent = 32767; /* INT16_MAX */
   const uint8_t rop = 0xf0; /* PATCOPY */
   struct ilo_builder *builder = &blitter->ilo->cp->builder;
   struct gen6_blt_xy_bo dst;
   uint32_t swctrl;
   int slice;

   /* no W-tiling support */
   if (dst_tex->separate_s8)
      return false;

   if (dst_tex->layout.bo_stride > max_extent)
      return false;

   if (dst_box->width * cpp > gen6_blt_max_bytes_per_scanline)
      return false;

   dst.bo = dst_tex->bo;
   dst.offset = 0;
   dst.pitch = dst_tex->layout.bo_stride;
   dst.tiling = dst_tex->layout.tiling;

   swctrl = ilo_blitter_blt_begin(blitter,
         GEN6_XY_COLOR_BLT__SIZE * dst_box->depth,
         dst_tex->bo, dst_tex->layout.tiling, NULL, INTEL_TILING_NONE);

   for (slice = 0; slice < dst_box->depth; slice++) {
      unsigned x, y;

      ilo_layout_get_slice_pos(&dst_tex->layout,
            dst_level, dst_box->z + slice, &x, &y);

      dst.x = x + dst_box->x;
      dst.y = y + dst_box->y;

      if (dst.x + dst_box->width > max_extent ||
          dst.y + dst_box->height > max_extent)
         break;

      gen6_XY_COLOR_BLT(builder, &dst, val,
            dst_box->width, dst_box->height, rop, value_mask, write_mask);
   }

   ilo_blitter_blt_end(blitter, swctrl);

   return (slice == dst_box->depth);
}

static bool
tex_copy_region(struct ilo_blitter *blitter,
                struct ilo_texture *dst_tex,
                unsigned dst_level,
                unsigned dst_x, unsigned dst_y, unsigned dst_z,
                struct ilo_texture *src_tex,
                unsigned src_level,
                const struct pipe_box *src_box)
{
   const struct util_format_description *desc =
      util_format_description(dst_tex->layout.format);
   const unsigned max_extent = 32767; /* INT16_MAX */
   const uint8_t rop = 0xcc; /* SRCCOPY */
   struct ilo_builder *builder = &blitter->ilo->cp->builder;
   enum gen6_blt_mask mask;
   struct gen6_blt_xy_bo dst, src;
   uint32_t swctrl;
   int cpp, xscale, slice;

   /* no W-tiling support */
   if (dst_tex->separate_s8 || src_tex->separate_s8)
      return false;

   if (dst_tex->layout.bo_stride > max_extent ||
       src_tex->layout.bo_stride > max_extent)
      return false;

   cpp = desc->block.bits / 8;
   xscale = 1;

   /* accommodate for larger cpp */
   if (cpp > 4) {
      if (cpp % 2 == 1)
         return false;

      cpp = (cpp % 4 == 0) ? 4 : 2;
      xscale = (desc->block.bits / 8) / cpp;
   }

   if (src_box->width * cpp * xscale > gen6_blt_max_bytes_per_scanline)
      return false;

   switch (cpp) {
   case 1:
      mask = GEN6_BLT_MASK_8;
      break;
   case 2:
      mask = GEN6_BLT_MASK_16;
      break;
   case 4:
      mask = GEN6_BLT_MASK_32;
      break;
   default:
      return false;
      break;
   }

   dst.bo = dst_tex->bo;
   dst.offset = 0;
   dst.pitch = dst_tex->layout.bo_stride;
   dst.tiling = dst_tex->layout.tiling;

   src.bo = src_tex->bo;
   src.offset = 0;
   src.pitch = src_tex->layout.bo_stride;
   src.tiling = src_tex->layout.tiling;

   swctrl = ilo_blitter_blt_begin(blitter,
         GEN6_XY_SRC_COPY_BLT__SIZE * src_box->depth,
         dst.bo, dst.tiling, src.bo, src.tiling);

   for (slice = 0; slice < src_box->depth; slice++) {
      unsigned dx, dy, sx, sy, width, height;

      ilo_layout_get_slice_pos(&dst_tex->layout,
            dst_level, dst_z + slice, &dx, &dy);
      ilo_layout_get_slice_pos(&src_tex->layout,
            src_level, src_box->z + slice, &sx, &sy);

      dst.x = (dx + dst_x) * xscale;
      dst.y = dy + dst_y;
      src.x = (sx + src_box->x) * xscale;
      src.y = sy + src_box->y;
      width = src_box->width * xscale;
      height = src_box->height;

      /* in blocks */
      dst.x /= desc->block.width;
      dst.y /= desc->block.height;
      src.x /= desc->block.width;
      src.y /= desc->block.height;
      width /= desc->block.width;
      height /= desc->block.height;

      if (src.x + width > max_extent || src.y + height > max_extent ||
          dst.x + width > max_extent || dst.y + height > max_extent)
         break;

      gen6_XY_SRC_COPY_BLT(builder, &dst, &src,
            width, height, rop, mask, mask);
   }

   ilo_blitter_blt_end(blitter, swctrl);

   return (slice == src_box->depth);
}

bool
ilo_blitter_blt_copy_resource(struct ilo_blitter *blitter,
                              struct pipe_resource *dst, unsigned dst_level,
                              unsigned dst_x, unsigned dst_y, unsigned dst_z,
                              struct pipe_resource *src, unsigned src_level,
                              const struct pipe_box *src_box)
{
   bool success;

   ilo_blit_resolve_slices(blitter->ilo, src, src_level,
         src_box->z, src_box->depth, ILO_TEXTURE_BLT_READ);
   ilo_blit_resolve_slices(blitter->ilo, dst, dst_level,
         dst_z, src_box->depth, ILO_TEXTURE_BLT_WRITE);

   if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
      const unsigned dst_offset = dst_x;
      const unsigned src_offset = src_box->x;
      const unsigned size = src_box->width;

      assert(dst_level == 0 && dst_y == 0 && dst_z == 0);
      assert(src_level == 0 &&
             src_box->y == 0 &&
             src_box->z == 0 &&
             src_box->height == 1 &&
             src_box->depth == 1);

      success = buf_copy_region(blitter,
            ilo_buffer(dst), dst_offset, ilo_buffer(src), src_offset, size);
   }
   else if (dst->target != PIPE_BUFFER && src->target != PIPE_BUFFER) {
      success = tex_copy_region(blitter,
            ilo_texture(dst), dst_level, dst_x, dst_y, dst_z,
            ilo_texture(src), src_level, src_box);
   }
   else {
      success = false;
   }

   return success;
}

bool
ilo_blitter_blt_clear_rt(struct ilo_blitter *blitter,
                         struct pipe_surface *rt,
                         const union pipe_color_union *color,
                         unsigned x, unsigned y,
                         unsigned width, unsigned height)
{
   const int cpp = util_format_get_blocksize(rt->format);
   enum gen6_blt_mask mask;
   union util_color packed;
   bool success;

   if (ilo_skip_rendering(blitter->ilo))
      return true;

   switch (cpp) {
   case 1:
      mask = GEN6_BLT_MASK_8;
      break;
   case 2:
      mask = GEN6_BLT_MASK_16;
      break;
   case 4:
      mask = GEN6_BLT_MASK_32;
      break;
   default:
      return false;
      break;
   }

   if (util_format_is_pure_integer(rt->format) ||
       util_format_is_compressed(rt->format))
      return false;

   ilo_blit_resolve_surface(blitter->ilo, rt, ILO_TEXTURE_BLT_WRITE);

   util_pack_color(color->f, rt->format, &packed);

   if (rt->texture->target == PIPE_BUFFER) {
      unsigned offset, end, size;

      assert(y == 0 && height == 1);

      offset = (rt->u.buf.first_element + x) * cpp;
      end = (rt->u.buf.last_element + 1) * cpp;

      size = width * cpp;
      if (offset + size > end)
         size = end - offset;

      success = buf_clear_region(blitter, ilo_buffer(rt->texture),
            offset, packed.ui[0], size, mask, mask);
   }
   else {
      struct pipe_box box;

      u_box_3d(x, y, rt->u.tex.first_layer, width, height,
            rt->u.tex.last_layer - rt->u.tex.first_layer + 1, &box);

      success = tex_clear_region(blitter, ilo_texture(rt->texture),
            rt->u.tex.level, &box, packed.ui[0], mask, mask);
   }

   return success;
}

bool
ilo_blitter_blt_clear_zs(struct ilo_blitter *blitter,
                         struct pipe_surface *zs,
                         unsigned clear_flags,
                         double depth, unsigned stencil,
                         unsigned x, unsigned y,
                         unsigned width, unsigned height)
{
   enum gen6_blt_mask value_mask, write_mask;
   struct pipe_box box;
   uint32_t val;

   if (ilo_skip_rendering(blitter->ilo))
      return true;

   switch (zs->format) {
   case PIPE_FORMAT_Z16_UNORM:
      if (!(clear_flags & PIPE_CLEAR_DEPTH))
         return true;

      value_mask = GEN6_BLT_MASK_16;
      write_mask = GEN6_BLT_MASK_16;
      break;
   case PIPE_FORMAT_Z32_FLOAT:
      if (!(clear_flags & PIPE_CLEAR_DEPTH))
         return true;

      value_mask = GEN6_BLT_MASK_32;
      write_mask = GEN6_BLT_MASK_32;
      break;
   case PIPE_FORMAT_Z24X8_UNORM:
      if (!(clear_flags & PIPE_CLEAR_DEPTH))
         return true;

      value_mask = GEN6_BLT_MASK_32;
      write_mask = GEN6_BLT_MASK_32_LO;
      break;
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      if (!(clear_flags & PIPE_CLEAR_DEPTHSTENCIL))
         return true;

      value_mask = GEN6_BLT_MASK_32;

      if ((clear_flags & PIPE_CLEAR_DEPTHSTENCIL) == PIPE_CLEAR_DEPTHSTENCIL)
         write_mask = GEN6_BLT_MASK_32;
      else if (clear_flags & PIPE_CLEAR_DEPTH)
         write_mask = GEN6_BLT_MASK_32_LO;
      else
         write_mask = GEN6_BLT_MASK_32_HI;
      break;
   default:
      return false;
      break;
   }

   ilo_blit_resolve_surface(blitter->ilo, zs, ILO_TEXTURE_BLT_WRITE);

   val = util_pack_z_stencil(zs->format, depth, stencil);

   u_box_3d(x, y, zs->u.tex.first_layer, width, height,
         zs->u.tex.last_layer - zs->u.tex.first_layer + 1, &box);

   assert(zs->texture->target != PIPE_BUFFER);

   return tex_clear_region(blitter, ilo_texture(zs->texture),
         zs->u.tex.level, &box, val, value_mask, write_mask);
}
