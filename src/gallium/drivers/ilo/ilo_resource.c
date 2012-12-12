/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
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

#include "ilo_context.h"
#include "ilo_screen.h"
#include "ilo_resource.h"

static struct intel_bo *
alloc_buf_bo(const struct ilo_resource *res)
{
   struct ilo_screen *is = ilo_screen(res->base.screen);
   struct intel_bo *bo;
   const char *name;
   const unsigned size = res->bo_width;

   switch (res->base.bind) {
   case PIPE_BIND_VERTEX_BUFFER:
      name = "vertex buffer";
      break;
   case PIPE_BIND_INDEX_BUFFER:
      name = "index buffer";
      break;
   case PIPE_BIND_CONSTANT_BUFFER:
      name = "constant buffer";
      break;
   case PIPE_BIND_STREAM_OUTPUT:
      name = "stream output";
      break;
   default:
      name = "unknown buffer";
      break;
   }

   /* this is what a buffer supposed to be like */
   assert(res->bo_width * res->bo_height * res->bo_cpp == size);
   assert(res->tiling == INTEL_TILING_NONE);
   assert(res->bo_stride == 0);

   if (res->handle) {
      bo = is->winsys->import_handle(is->winsys, name,
            res->bo_width, res->bo_height, res->bo_cpp, res->handle);

      /* since the bo is shared to us, make sure it meets the expectations */
      if (bo) {
         assert(bo->get_size(res->bo) == size);
         assert(bo->get_tiling(res->bo) == res->tiling);
         assert(bo->get_pitch(res->bo) == res->bo_stride);
      }
   }
   else {
      bo = is->winsys->alloc_buffer(is->winsys, name, size, 0);
   }

   return bo;
}

static struct intel_bo *
alloc_tex_bo(const struct ilo_resource *res)
{
   struct ilo_screen *is = ilo_screen(res->base.screen);
   struct intel_bo *bo;
   const char *name;

   switch (res->base.target) {
   case PIPE_TEXTURE_1D:
      name = "1D texture";
      break;
   case PIPE_TEXTURE_2D:
      name = "2D texture";
      break;
   case PIPE_TEXTURE_3D:
      name = "3D texture";
      break;
   case PIPE_TEXTURE_CUBE:
      name = "cube texture";
      break;
   case PIPE_TEXTURE_RECT:
      name = "rectangle texture";
      break;
   case PIPE_TEXTURE_1D_ARRAY:
      name = "1D array texture";
      break;
   case PIPE_TEXTURE_2D_ARRAY:
      name = "2D array texture";
      break;
   case PIPE_TEXTURE_CUBE_ARRAY:
      name = "cube array texture";
      break;
   default:
      name ="unknown texture";
      break;
   }

   if (res->handle) {
      bo = is->winsys->import_handle(is->winsys, name,
            res->bo_width, res->bo_height, res->bo_cpp, res->handle);
   }
   else {
      const bool for_render =
         (res->base.bind & (PIPE_BIND_DEPTH_STENCIL |
                            PIPE_BIND_RENDER_TARGET));
      const unsigned long flags =
         (for_render) ? INTEL_ALLOC_FOR_RENDER : 0;

      bo = is->winsys->alloc(is->winsys, name,
            res->bo_width, res->bo_height, res->bo_cpp,
            res->tiling, flags);
   }

   return bo;
}

static bool
realloc_bo(struct ilo_resource *res)
{
   struct intel_bo *old_bo = res->bo;

   /* a shared bo cannot be reallocated */
   if (old_bo && res->handle)
      return false;

   if (res->base.target == PIPE_BUFFER)
      res->bo = alloc_buf_bo(res);
   else
      res->bo = alloc_tex_bo(res);

   if (!res->bo) {
      res->bo = old_bo;
      return false;
   }

   /* winsys may decide to use a different tiling */
   res->tiling = res->bo->get_tiling(res->bo);
   res->bo_stride = res->bo->get_pitch(res->bo);

   if (old_bo)
      old_bo->unreference(old_bo);

   return true;
}

static bool
alloc_slice_offsets(struct ilo_resource *res)
{
   int depth, lv;

   /* sum the depths of all levels */
   depth = 0;
   for (lv = 0; lv <= res->base.last_level; lv++)
      depth += u_minify(res->base.depth0, lv);

   /*
    * There are (depth * res->base.array_size) slices.  Either depth is one
    * (non-3D) or res->base.array_size is one (non-array), but it does not
    * matter.
    */
   res->slice_offsets[0] =
      CALLOC(depth * res->base.array_size, sizeof(res->slice_offsets[0][0]));
   if (!res->slice_offsets[0])
      return false;

   /* point to the respective positions in the buffer */
   for (lv = 1; lv <= res->base.last_level; lv++) {
      res->slice_offsets[lv] = res->slice_offsets[lv - 1] +
         u_minify(res->base.depth0, lv - 1) * res->base.array_size;
   }

   return true;
}

static void
free_slice_offsets(struct ilo_resource *res)
{
   int lv;

   FREE(res->slice_offsets[0]);
   for (lv = 0; lv <= res->base.last_level; lv++)
      res->slice_offsets[lv] = NULL;
}

struct layout_tex_info {
   bool compressed;
   int block_width, block_height;
   int align_i, align_j;
   int qpitch;

   struct {
      int w, h, d;
   } sizes[PIPE_MAX_TEXTURE_LEVELS];
};

/**
 * Prepare for texture layout.
 */
static void
layout_tex_init(const struct ilo_resource *res, struct layout_tex_info *info)
{
   struct ilo_screen *is = ilo_screen(res->base.screen);
   const enum intel_tiling_mode tiling = res->tiling;
   const struct pipe_resource *templ = &res->base;
   int last_level, lv;

   memset(info, 0, sizeof(*info));

   info->compressed = util_format_is_compressed(templ->format);
   info->block_width = util_format_get_blockwidth(templ->format);
   info->block_height = util_format_get_blockheight(templ->format);

   /*
    * From the Sandy Bridge PRM, volume 1 part 1, page 113:
    *
    *     "surface format           align_i     align_j
    *      YUV 4:2:2 formats        4           *see below
    *      BC1-5                    4           4
    *      FXT1                     8           4
    *      all other formats        4           *see below"
    *
    *     "- align_j = 4 for any depth buffer
    *      - align_j = 2 for separate stencil buffer
    *      - align_j = 4 for any render target surface is multisampled (4x)
    *      - align_j = 4 for any render target surface with Surface Vertical
    *        Alignment = VALIGN_4
    *      - align_j = 2 for any render target surface with Surface Vertical
    *        Alignment = VALIGN_2
    *      - align_j = 2 for all other render target surface
    *      - align_j = 2 for any sampling engine surface with Surface Vertical
    *        Alignment = VALIGN_2
    *      - align_j = 4 for any sampling engine surface with Surface Vertical
    *        Alignment = VALIGN_4"
    *
    * From the Sandy Bridge PRM, volume 4 part 1, page 86:
    *
    *     "This field (Surface Vertical Alignment) must be set to VALIGN_2 if
    *      the Surface Format is 96 bits per element (BPE)."
    *
    * They can be rephrased as
    *
    *                                  align_i        align_j
    *   compressed formats             block width    block height
    *   PIPE_FORMAT_S8_UINT            4              2
    *   other depth/stencil formats    4              4
    *   4x multisampled                4              4
    *   bpp 96                         4              2
    *   others                         4              2 or 4
    */

   /*
    * From the Ivy Bridge PRM, volume 1 part 1, page 110:
    *
    *     "surface defined by      surface format     align_i     align_j
    *      3DSTATE_DEPTH_BUFFER    D16_UNORM          8           4
    *                              not D16_UNORM      4           4
    *      3DSTATE_STENCIL_BUFFER  N/A                8           8
    *      SURFACE_STATE           BC*, ETC*, EAC*    4           4
    *                              FXT1               8           4
    *                              all others         (set by SURFACE_STATE)"
    *
    * From the Ivy Bridge PRM, volume 4 part 1, page 63:
    *
    *     "- This field (Surface Vertical Aligment) is intended to be set to
    *        VALIGN_4 if the surface was rendered as a depth buffer, for a
    *        multisampled (4x) render target, or for a multisampled (8x)
    *        render target, since these surfaces support only alignment of 4.
    *      - Use of VALIGN_4 for other surfaces is supported, but uses more
    *        memory.
    *      - This field must be set to VALIGN_4 for all tiled Y Render Target
    *        surfaces.
    *      - Value of 1 is not supported for format YCRCB_NORMAL (0x182),
    *        YCRCB_SWAPUVY (0x183), YCRCB_SWAPUV (0x18f), YCRCB_SWAPY (0x190)
    *      - If Number of Multisamples is not MULTISAMPLECOUNT_1, this field
    *        must be set to VALIGN_4."
    *      - VALIGN_4 is not supported for surface format R32G32B32_FLOAT."
    *
    *     "- This field (Surface Horizontal Aligment) is intended to be set to
    *        HALIGN_8 only if the surface was rendered as a depth buffer with
    *        Z16 format or a stencil buffer, since these surfaces support only
    *        alignment of 8.
    *      - Use of HALIGN_8 for other surfaces is supported, but uses more
    *        memory.
    *      - This field must be set to HALIGN_4 if the Surface Format is BC*.
    *      - This field must be set to HALIGN_8 if the Surface Format is
    *        FXT1."
    *
    * They can be rephrased as
    *
    *                                  align_i        align_j
    *  compressed formats              block width    block height
    *  PIPE_FORMAT_Z16_UNORM           8              4
    *  PIPE_FORMAT_S8_UINT             8              8
    *  other depth/stencil formats     4 or 8         4
    *  2x or 4x multisampled           4 or 8         4
    *  tiled Y                         4 or 8         4 (if rt)
    *  PIPE_FORMAT_R32G32B32_FLOAT     4 or 8         2
    *  others                          4 or 8         2 or 4
    */

   if (info->compressed) {
      /* this happens to be the case */
      info->align_i = info->block_width;
      info->align_j = info->block_height;
   }
   else if (util_format_is_depth_or_stencil(templ->format)) {
      if (is->gen >= ILO_GEN(7)) {
         switch (templ->format) {
         case PIPE_FORMAT_Z16_UNORM:
            info->align_i = 8;
            info->align_j = 4;
            break;
         case PIPE_FORMAT_S8_UINT:
            info->align_i = 8;
            info->align_j = 8;
            break;
         default:
            /*
             * From the Ivy Bridge PRM, volume 2 part 1, page 319:
             *
             *     "The 3 LSBs of both offsets (Depth Coordinate Offset Y and
             *      Depth Coordinate Offset X) must be zero to ensure correct
             *      alignment"
             *
             * We will make use of them and setting align_i to 8 help us meet
             * the requirement.
             */
            info->align_i = (templ->last_level > 0) ? 8 : 4;
            info->align_j = 4;
            break;
         }
      }
      else {
         switch (templ->format) {
         case PIPE_FORMAT_S8_UINT:
            info->align_i = 4;
            info->align_j = 2;
            break;
         default:
            info->align_i = 4;
            info->align_j = 4;
            break;
         }
      }
   }
   else {
      const bool valign_4 = (templ->nr_samples > 1) ||
         (is->gen >= ILO_GEN(7) &&
          (templ->bind & PIPE_BIND_RENDER_TARGET) &&
          tiling == INTEL_TILING_Y);

      if (valign_4)
         assert(util_format_get_blocksizebits(templ->format) != 96);

      info->align_i = 4;
      info->align_j = (valign_4) ? 4 : 2;
   }

   /*
    * the fact that align i and j are multiples of block width and height
    * respectively is what makes the size of the bo a multiple of the block
    * size, slices start at block boundaries, and many of the computations
    * work.
    */
   assert(info->align_i % info->block_width == 0);
   assert(info->align_j % info->block_height == 0);

   /* make sure align() works */
   assert(util_is_power_of_two(info->align_i) &&
          util_is_power_of_two(info->align_j));
   assert(util_is_power_of_two(info->block_width) &&
          util_is_power_of_two(info->block_height));

   last_level = templ->last_level;
   /* need at least 2 levels to compute qpitch below */
   if (templ->array_size > 1 && last_level == 0 &&
       templ->format != PIPE_FORMAT_S8_UINT)
      last_level++;

   /* compute mip level sizes */
   for (lv = 0; lv <= last_level; lv++) {
      int w, h, d;

      w = u_minify(templ->width0, lv);
      h = u_minify(templ->height0, lv);
      d = u_minify(templ->depth0, lv);

      /*
       * From the Sandy Bridge PRM, volume 1 part 1, page 114:
       *
       *     "The dimensions of the mip maps are first determined by applying
       *      the sizing algorithm presented in Non-Power-of-Two Mipmaps
       *      above. Then, if necessary, they are padded out to compression
       *      block boundaries."
       */
      w = align(w, info->block_width);
      h = align(h, info->block_height);

      /*
       * From the Sandy Bridge PRM, volume 1 part 1, page 111:
       *
       *     "If the surface is multisampled (4x), these values must be
       *      adjusted as follows before proceeding:
       *
       *        W_L = ceiling(W_L / 2) * 4
       *        H_L = ceiling(H_L / 2) * 4"
       */
      if (templ->nr_samples > 1) {
         w = align(w, 2) * 2;
         h = align(h, 2) * 2;
      }

      info->sizes[lv].w = w;
      info->sizes[lv].h = h;
      info->sizes[lv].d = d;
   }

   if (templ->array_size > 1) {
      const int h0 = align(info->sizes[0].h, info->align_j);

      if (templ->format == PIPE_FORMAT_S8_UINT) {
         info->qpitch = h0;
      }
      else {
         const int h1 = align(info->sizes[1].h, info->align_j);

         /*
          * From the Sandy Bridge PRM, volume 1 part 1, page 115:
          *
          *     "The following equation is used for surface formats other than
          *      compressed textures:
          *
          *        QPitch = (h0 + h1 + 11j)"
          *
          *     "The equation for compressed textures (BC* and FXT1 surface
          *      formats) follows:
          *
          *        QPitch = (h0 + h1 + 11j) / 4"
          *
          *     "[DevSNB] Errata: Sampler MSAA Qpitch will be 4 greater than
          *      the value calculated in the equation above, for every other
          *      odd Surface Height starting from 1 i.e. 1,5,9,13"
          *
          * To access the N-th slice, an offset of (Stride * QPitch * N) is
          * added to the base address.  The PRM divides QPitch by 4 for
          * compressed formats because the block height for those formats are
          * 4, and it wants QPitch to mean the number of memory rows, as
          * opposed to texel rows, between slices.  Since we use texel rows in
          * res->slice_offsets, we do not need to divide QPitch by 4.
          */
         info->qpitch = h0 + h1 +
            ((is->gen >= ILO_GEN(7)) ? 12 : 11) * info->align_j;

         if (is->gen == ILO_GEN(6) && templ->nr_samples > 1 &&
               templ->height0 % 4 == 1)
            info->qpitch += 4;
      }
   }
}

/**
 * Layout a 2D texture.
 */
static void
layout_tex_2d(struct ilo_resource *res, const struct layout_tex_info *info)
{
   const struct pipe_resource *templ = &res->base;
   unsigned int level_x, level_y;
   int lv;

   res->bo_width = 0;
   res->bo_height = 0;

   level_x = 0;
   level_y = 0;
   for (lv = 0; lv <= templ->last_level; lv++) {
      const unsigned int level_w = info->sizes[lv].w;
      const unsigned int level_h = info->sizes[lv].h;
      int slice;

      for (slice = 0; slice < templ->array_size; slice++) {
         res->slice_offsets[lv][slice].x = level_x;
         /* slices are qpitch apart in Y-direction */
         res->slice_offsets[lv][slice].y = level_y + info->qpitch * slice;
      }

      /* extend the size of the monolithic bo to cover this mip level */
      if (res->bo_width < level_x + level_w)
         res->bo_width = level_x + level_w;
      if (res->bo_height < level_y + level_h)
         res->bo_height = level_y + level_h;

      /* MIPLAYOUT_BELOW */
      if (lv == 1)
         level_x += align(level_w, info->align_i);
      else
         level_y += align(level_h, info->align_j);
   }

   /* we did not take slices into consideration in the computation above */
   res->bo_height += info->qpitch * (templ->array_size - 1);
}

/**
 * Layout a 3D texture.
 */
static void
layout_tex_3d(struct ilo_resource *res, const struct layout_tex_info *info)
{
   const struct pipe_resource *templ = &res->base;
   unsigned int level_y;
   int lv;

   res->bo_width = 0;
   res->bo_height = 0;

   level_y = 0;
   for (lv = 0; lv <= templ->last_level; lv++) {
      const unsigned int level_w = info->sizes[lv].w;
      const unsigned int level_h = info->sizes[lv].h;
      const unsigned int level_d = info->sizes[lv].d;
      const unsigned int slice_pitch = align(level_w, info->align_i);
      const unsigned int slice_qpitch = align(level_h, info->align_j);
      const unsigned int num_slices_per_row = 1 << lv;
      int slice;

      for (slice = 0; slice < level_d; slice += num_slices_per_row) {
         int i;

         for (i = 0; i < num_slices_per_row && slice + i < level_d; i++) {
            res->slice_offsets[lv][slice + i].x = slice_pitch * i;
            res->slice_offsets[lv][slice + i].y = level_y;
         }

         /* move on to the next slice row */
         level_y += slice_qpitch;
      }

      /* rightmost slice */
      slice = MIN2(num_slices_per_row, level_d) - 1;

      /* extend the size of the monolithic bo to cover this slice */
      if (res->bo_width < slice_pitch * slice + level_w)
         res->bo_width = slice_pitch * slice + level_w;
      if (lv == templ->last_level)
         res->bo_height = (level_y - slice_qpitch) + level_h;
   }
}

/**
 * Guess the texture size.  For large textures, the errors are relative small.
 */
static size_t
guess_tex_size(const struct pipe_resource *templ,
               enum intel_tiling_mode tiling)
{
   int bo_width, bo_height, bo_stride;

   /* HALIGN_8 and VALIGN_4 */
   bo_width = align(templ->width0, 8);
   bo_height = align(templ->height0, 4);

   if (templ->target == PIPE_TEXTURE_3D) {
      const int num_rows = util_next_power_of_two(templ->depth0);
      int lv, sum;

      sum = bo_height * templ->depth0;
      for (lv = 1; lv <= templ->last_level; lv++)
         sum += u_minify(bo_height, lv) * u_minify(num_rows, lv);

      bo_height = sum;
   }
   else if (templ->last_level > 0) {
      /* MIPLAYOUT_BELOW, ignore qpich */
      bo_height = (bo_height + u_minify(bo_height, 1)) * templ->array_size;
   }

   bo_stride = util_format_get_stride(templ->format, bo_width);

   switch (tiling) {
   case INTEL_TILING_X:
      bo_stride = align(bo_stride, 512);
      bo_height = align(bo_height, 8);
      break;
   case INTEL_TILING_Y:
      bo_stride = align(bo_stride, 128);
      bo_height = align(bo_height, 32);
      break;
   default:
      bo_height = align(bo_height, 2);
      break;
   }

   return util_format_get_2d_size(templ->format, bo_stride, bo_height);
}

static enum intel_tiling_mode
get_tex_tiling(const struct ilo_resource *res)
{
   const struct pipe_resource *templ = &res->base;

   /*
    * From the Sandy Bridge PRM, volume 1 part 2, page 32:
    *
    *     "Display/Overlay   Y-Major not supported.
    *                        X-Major required for Async Flips"
    */
   if (unlikely(templ->bind & PIPE_BIND_SCANOUT))
      return INTEL_TILING_X;

   /*
    * From the Sandy Bridge PRM, volume 3 part 2, page 158:
    *
    *     "The cursor surface address must be 4K byte aligned. The cursor must
    *      be in linear memory, it cannot be tiled."
    */
   if (unlikely(templ->bind & PIPE_BIND_CURSOR))
      return INTEL_TILING_NONE;

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 318:
    *
    *     "[DevSNB+]: This field (Tiled Surface) must be set to TRUE. Linear
    *      Depth Buffer is not supported."
    *
    *     "The Depth Buffer, if tiled, must use Y-Major tiling."
    */
   if (templ->bind & PIPE_BIND_DEPTH_STENCIL)
      return INTEL_TILING_Y;

   if (templ->bind & (PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW)) {
      enum intel_tiling_mode tiling = INTEL_TILING_NONE;

      /*
       * From the Sandy Bridge PRM, volume 1 part 2, page 32:
       *
       *     "NOTE: 128BPE Format Color buffer ( render target ) MUST be
       *      either TileX or Linear."
       *
       * Also, heuristically set a minimum width/height for enabling tiling.
       */
      if (util_format_get_blocksizebits(templ->format) == 128 &&
          (templ->bind & PIPE_BIND_RENDER_TARGET) && templ->width0 >= 64)
         tiling = INTEL_TILING_X;
      else if ((templ->width0 >= 32 && templ->height0 >= 16) ||
               (templ->width0 >= 16 && templ->height0 >= 32))
         tiling = INTEL_TILING_Y;

      /* make sure the bo can be mapped through GTT if tiled */
      if (tiling != INTEL_TILING_NONE) {
         /*
          * Usually only the first 256MB of the GTT is mappable.
          *
          * See also how intel_context::max_gtt_map_object_size is calculated.
          */
         const size_t mappable_gtt_size = 256 * 1024 * 1024;
         const size_t size = guess_tex_size(templ, tiling);

         /* be conservative */
         if (size > mappable_gtt_size / 4)
            tiling = INTEL_TILING_NONE;
      }

      return tiling;
   }

   return INTEL_TILING_NONE;
}

static void
init_texture(struct ilo_resource *res)
{
   const enum pipe_format format = res->base.format;
   struct layout_tex_info info;

   /* determine tiling first as it may affect the layout */
   res->tiling = get_tex_tiling(res);

   layout_tex_init(res, &info);

   res->compressed = info.compressed;
   res->block_width = info.block_width;
   res->block_height = info.block_height;
   res->halign_8 = (info.align_i == 8);
   res->valign_4 = (info.align_j == 4);

   switch (res->base.target) {
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_1D_ARRAY:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_CUBE_ARRAY:
      layout_tex_2d(res, &info);
      break;
   case PIPE_TEXTURE_3D:
      layout_tex_3d(res, &info);
      break;
   default:
      assert(!"unknown resource target");
      break;
   }

   /* in blocks */
   assert(res->bo_width % info.block_width == 0);
   assert(res->bo_height % info.block_height == 0);
   res->bo_width /= info.block_width;
   res->bo_height /= info.block_height;
   res->bo_cpp = util_format_get_blocksize(format);
}

static void
init_buffer(struct ilo_resource *res)
{
   res->compressed = false;
   res->block_width = 1;
   res->block_height = 1;
   res->halign_8 = false;
   res->valign_4 = false;

   res->bo_width = res->base.width0;
   res->bo_height = 1;
   res->bo_cpp = 1;
   res->bo_stride = 0;
   res->tiling = INTEL_TILING_NONE;
}

static struct pipe_resource *
create_resource(struct pipe_screen *screen,
                const struct pipe_resource *templ,
                struct winsys_handle *handle)
{
   struct ilo_resource *res;

   res = CALLOC_STRUCT(ilo_resource);
   if (!res)
      return NULL;

   res->base = *templ;
   res->base.screen = screen;
   pipe_reference_init(&res->base.reference, 1);
   res->handle = handle;

   if (!alloc_slice_offsets(res)) {
      FREE(res);
      return NULL;
   }

   if (templ->target == PIPE_BUFFER)
      init_buffer(res);
   else
      init_texture(res);

   if (!realloc_bo(res)) {
      free_slice_offsets(res);
      FREE(res);
      return NULL;
   }

   return &res->base;
}

static boolean
ilo_can_create_resource(struct pipe_screen *screen,
                        const struct pipe_resource *templ)
{
   /*
    * We do not know if we will fail until we try to allocate the bo.
    * So just set a limit on the texture size.
    */
   const size_t max_size = 1 * 1024 * 1024 * 1024;
   const size_t size = guess_tex_size(templ, INTEL_TILING_Y);

   return (size <= max_size);
}

static struct pipe_resource *
ilo_resource_create(struct pipe_screen *screen,
                    const struct pipe_resource *templ)
{
   return create_resource(screen, templ, NULL);
}

static struct pipe_resource *
ilo_resource_from_handle(struct pipe_screen *screen,
                         const struct pipe_resource *templ,
                         struct winsys_handle *handle)
{
   return create_resource(screen, templ, handle);
}

static boolean
ilo_resource_get_handle(struct pipe_screen *screen,
                        struct pipe_resource *r,
                        struct winsys_handle *handle)
{
   struct ilo_resource *res = ilo_resource(r);
   int err;

   err = res->bo->export_handle(res->bo, handle);

   return !err;
}

static void
ilo_resource_destroy(struct pipe_screen *screen,
                     struct pipe_resource *r)
{
   struct ilo_resource *res = ilo_resource(r);

   free_slice_offsets(res);
   res->bo->unreference(res->bo);
   FREE(res);
}

/**
 * Initialize resource-related functions.
 */
void
ilo_init_resource_functions(struct ilo_screen *is)
{
   is->base.can_create_resource = ilo_can_create_resource;
   is->base.resource_create = ilo_resource_create;
   is->base.resource_from_handle = ilo_resource_from_handle;
   is->base.resource_get_handle = ilo_resource_get_handle;
   is->base.resource_destroy = ilo_resource_destroy;
}

/**
 * Initialize transfer-related functions.
 */
void
ilo_init_transfer_functions(struct ilo_context *ilo)
{
   ilo->base.transfer_map = NULL;
   ilo->base.transfer_flush_region = NULL;
   ilo->base.transfer_unmap = NULL;
   ilo->base.transfer_inline_write = NULL;
}
