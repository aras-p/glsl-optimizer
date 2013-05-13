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

#include "ilo_screen.h"
#include "ilo_resource.h"

/* use PIPE_BIND_CUSTOM to indicate MCS */
#define ILO_BIND_MCS PIPE_BIND_CUSTOM

static struct intel_bo *
alloc_buf_bo(const struct ilo_texture *tex)
{
   struct ilo_screen *is = ilo_screen(tex->base.screen);
   struct intel_bo *bo;
   const char *name;
   const unsigned size = tex->bo_width;

   switch (tex->base.bind) {
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
   assert(tex->bo_width * tex->bo_height * tex->bo_cpp == size);
   assert(tex->tiling == INTEL_TILING_NONE);
   assert(tex->bo_stride == 0);

   if (tex->handle) {
      bo = is->winsys->import_handle(is->winsys, name,
            tex->bo_width, tex->bo_height, tex->bo_cpp, tex->handle);

      /* since the bo is shared to us, make sure it meets the expectations */
      if (bo) {
         assert(bo->get_size(tex->bo) == size);
         assert(bo->get_tiling(tex->bo) == tex->tiling);
         assert(bo->get_pitch(tex->bo) == tex->bo_stride);
      }
   }
   else {
      bo = is->winsys->alloc_buffer(is->winsys, name, size, 0);
   }

   return bo;
}

static struct intel_bo *
alloc_tex_bo(const struct ilo_texture *tex)
{
   struct ilo_screen *is = ilo_screen(tex->base.screen);
   struct intel_bo *bo;
   const char *name;

   switch (tex->base.target) {
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

   if (tex->handle) {
      bo = is->winsys->import_handle(is->winsys, name,
            tex->bo_width, tex->bo_height, tex->bo_cpp, tex->handle);
   }
   else {
      const bool for_render =
         (tex->base.bind & (PIPE_BIND_DEPTH_STENCIL |
                            PIPE_BIND_RENDER_TARGET));
      const unsigned long flags =
         (for_render) ? INTEL_ALLOC_FOR_RENDER : 0;

      bo = is->winsys->alloc(is->winsys, name,
            tex->bo_width, tex->bo_height, tex->bo_cpp,
            tex->tiling, flags);
   }

   return bo;
}

bool
ilo_texture_alloc_bo(struct ilo_texture *tex)
{
   struct intel_bo *old_bo = tex->bo;

   /* a shared bo cannot be reallocated */
   if (old_bo && tex->handle)
      return false;

   if (tex->base.target == PIPE_BUFFER)
      tex->bo = alloc_buf_bo(tex);
   else
      tex->bo = alloc_tex_bo(tex);

   if (!tex->bo) {
      tex->bo = old_bo;
      return false;
   }

   /* winsys may decide to use a different tiling */
   tex->tiling = tex->bo->get_tiling(tex->bo);
   tex->bo_stride = tex->bo->get_pitch(tex->bo);

   if (old_bo)
      old_bo->unreference(old_bo);

   return true;
}

static bool
alloc_slice_offsets(struct ilo_texture *tex)
{
   int depth, lv;

   /* sum the depths of all levels */
   depth = 0;
   for (lv = 0; lv <= tex->base.last_level; lv++)
      depth += u_minify(tex->base.depth0, lv);

   /*
    * There are (depth * tex->base.array_size) slices.  Either depth is one
    * (non-3D) or tex->base.array_size is one (non-array), but it does not
    * matter.
    */
   tex->slice_offsets[0] =
      CALLOC(depth * tex->base.array_size, sizeof(tex->slice_offsets[0][0]));
   if (!tex->slice_offsets[0])
      return false;

   /* point to the respective positions in the buffer */
   for (lv = 1; lv <= tex->base.last_level; lv++) {
      tex->slice_offsets[lv] = tex->slice_offsets[lv - 1] +
         u_minify(tex->base.depth0, lv - 1) * tex->base.array_size;
   }

   return true;
}

static void
free_slice_offsets(struct ilo_texture *tex)
{
   int lv;

   FREE(tex->slice_offsets[0]);
   for (lv = 0; lv <= tex->base.last_level; lv++)
      tex->slice_offsets[lv] = NULL;
}

struct layout_tex_info {
   bool compressed;
   int block_width, block_height;
   int align_i, align_j;
   bool array_spacing_full;
   bool interleaved;
   int qpitch;

   struct {
      int w, h, d;
   } sizes[PIPE_MAX_TEXTURE_LEVELS];
};

/**
 * Prepare for texture layout.
 */
static void
layout_tex_init(const struct ilo_texture *tex, struct layout_tex_info *info)
{
   struct ilo_screen *is = ilo_screen(tex->base.screen);
   const enum pipe_format bo_format = tex->bo_format;
   const enum intel_tiling_mode tiling = tex->tiling;
   const struct pipe_resource *templ = &tex->base;
   int last_level, lv;

   memset(info, 0, sizeof(*info));

   info->compressed = util_format_is_compressed(bo_format);
   info->block_width = util_format_get_blockwidth(bo_format);
   info->block_height = util_format_get_blockheight(bo_format);

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
   else if (util_format_is_depth_or_stencil(bo_format)) {
      if (is->dev.gen >= ILO_GEN(7)) {
         switch (bo_format) {
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
         switch (bo_format) {
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
         (is->dev.gen >= ILO_GEN(7) &&
          (templ->bind & PIPE_BIND_RENDER_TARGET) &&
          tiling == INTEL_TILING_Y);

      if (valign_4)
         assert(util_format_get_blocksizebits(bo_format) != 96);

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

   if (is->dev.gen >= ILO_GEN(7)) {
      /*
       * It is not explicitly states, but render targets are expected to be
       * UMS/CMS (samples non-interleaved) and depth/stencil buffers are
       * expected to be IMS (samples interleaved).
       *
       * See "Multisampled Surface Storage Format" field of SURFACE_STATE.
       */
      if (util_format_is_depth_or_stencil(bo_format)) {
         info->interleaved = true;

         /*
          * From the Ivy Bridge PRM, volume 1 part 1, page 111:
          *
          *     "note that the depth buffer and stencil buffer have an implied
          *      value of ARYSPC_FULL"
          */
         info->array_spacing_full = true;
      }
      else {
         info->interleaved = false;

         /*
          * From the Ivy Bridge PRM, volume 4 part 1, page 66:
          *
          *     "If Multisampled Surface Storage Format is MSFMT_MSS and
          *      Number of Multisamples is not MULTISAMPLECOUNT_1, this field
          *      (Surface Array Spacing) must be set to ARYSPC_LOD0."
          *
          * As multisampled resources are not mipmapped, we never use
          * ARYSPC_FULL for them.
          */
         if (templ->nr_samples > 1)
            assert(templ->last_level == 0);
         info->array_spacing_full = (templ->last_level > 0);
      }
   }
   else {
      /* GEN6 supports only interleaved samples */
      info->interleaved = true;

      /*
       * From the Sandy Bridge PRM, volume 1 part 1, page 115:
       *
       *     "The separate stencil buffer does not support mip mapping, thus
       *      the storage for LODs other than LOD 0 is not needed. The
       *      following QPitch equation applies only to the separate stencil
       *      buffer:
       *
       *        QPitch = h_0"
       *
       * GEN6 does not support compact spacing otherwise.
       */
      info->array_spacing_full = (bo_format != PIPE_FORMAT_S8_UINT);
   }

   last_level = templ->last_level;

   /* need at least 2 levels to compute full qpitch */
   if (last_level == 0 && templ->array_size > 1 && info->array_spacing_full)
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
       *
       * From the Ivy Bridge PRM, volume 1 part 1, page 108:
       *
       *     "If the surface is multisampled and it is a depth or stencil
       *      surface or Multisampled Surface StorageFormat in SURFACE_STATE
       *      is MSFMT_DEPTH_STENCIL, W_L and H_L must be adjusted as follows
       *      before proceeding:
       *
       *        #samples  W_L =                    H_L =
       *        2         ceiling(W_L / 2) * 4     HL [no adjustment]
       *        4         ceiling(W_L / 2) * 4     ceiling(H_L / 2) * 4
       *        8         ceiling(W_L / 2) * 8     ceiling(H_L / 2) * 4
       *        16        ceiling(W_L / 2) * 8     ceiling(H_L / 2) * 8"
       *
       * For interleaved samples (4x), where pixels
       *
       *   (x, y  ) (x+1, y  )
       *   (x, y+1) (x+1, y+1)
       *
       * would be is occupied by
       *
       *   (x, y  , si0) (x+1, y  , si0) (x, y  , si1) (x+1, y  , si1)
       *   (x, y+1, si0) (x+1, y+1, si0) (x, y+1, si1) (x+1, y+1, si1)
       *   (x, y  , si2) (x+1, y  , si2) (x, y  , si3) (x+1, y  , si3)
       *   (x, y+1, si2) (x+1, y+1, si2) (x, y+1, si3) (x+1, y+1, si3)
       *
       * Thus the need to
       *
       *   w = align(w, 2) * 2;
       *   y = align(y, 2) * 2;
       */
      if (info->interleaved) {
         switch (templ->nr_samples) {
         case 0:
         case 1:
            break;
         case 2:
            w = align(w, 2) * 2;
            break;
         case 4:
            w = align(w, 2) * 2;
            h = align(h, 2) * 2;
            break;
         case 8:
            w = align(w, 2) * 4;
            h = align(h, 2) * 2;
            break;
         case 16:
            w = align(w, 2) * 4;
            h = align(h, 2) * 4;
            break;
         default:
            assert(!"unsupported sample count");
            break;
         }
      }

      info->sizes[lv].w = w;
      info->sizes[lv].h = h;
      info->sizes[lv].d = d;
   }

   if (templ->array_size > 1) {
      const int h0 = align(info->sizes[0].h, info->align_j);

      if (info->array_spacing_full) {
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
          * From the Ivy Bridge PRM, volume 1 part 1, page 111-112:
          *
          *     "If Surface Array Spacing is set to ARYSPC_FULL (note that the
          *      depth buffer and stencil buffer have an implied value of
          *      ARYSPC_FULL):
          *
          *        QPitch = (h0 + h1 + 12j)
          *        QPitch = (h0 + h1 + 12j) / 4 (compressed)
          *
          *      (There are many typos or missing words here...)"
          *
          * To access the N-th slice, an offset of (Stride * QPitch * N) is
          * added to the base address.  The PRM divides QPitch by 4 for
          * compressed formats because the block height for those formats are
          * 4, and it wants QPitch to mean the number of memory rows, as
          * opposed to texel rows, between slices.  Since we use texel rows in
          * tex->slice_offsets, we do not need to divide QPitch by 4.
          */
         info->qpitch = h0 + h1 +
            ((is->dev.gen >= ILO_GEN(7)) ? 12 : 11) * info->align_j;

         if (is->dev.gen == ILO_GEN(6) && templ->nr_samples > 1 &&
               templ->height0 % 4 == 1)
            info->qpitch += 4;
      }
      else {
         info->qpitch = h0;
      }
   }
}

/**
 * Layout a 2D texture.
 */
static void
layout_tex_2d(struct ilo_texture *tex, const struct layout_tex_info *info)
{
   const struct pipe_resource *templ = &tex->base;
   unsigned int level_x, level_y, num_slices;
   int lv;

   tex->bo_width = 0;
   tex->bo_height = 0;

   level_x = 0;
   level_y = 0;
   for (lv = 0; lv <= templ->last_level; lv++) {
      const unsigned int level_w = info->sizes[lv].w;
      const unsigned int level_h = info->sizes[lv].h;
      int slice;

      for (slice = 0; slice < templ->array_size; slice++) {
         tex->slice_offsets[lv][slice].x = level_x;
         /* slices are qpitch apart in Y-direction */
         tex->slice_offsets[lv][slice].y = level_y + info->qpitch * slice;
      }

      /* extend the size of the monolithic bo to cover this mip level */
      if (tex->bo_width < level_x + level_w)
         tex->bo_width = level_x + level_w;
      if (tex->bo_height < level_y + level_h)
         tex->bo_height = level_y + level_h;

      /* MIPLAYOUT_BELOW */
      if (lv == 1)
         level_x += align(level_w, info->align_i);
      else
         level_y += align(level_h, info->align_j);
   }

   num_slices = templ->array_size;
   /* samples of the same index are stored in a slice */
   if (templ->nr_samples > 1 && !info->interleaved)
      num_slices *= templ->nr_samples;

   /* we did not take slices into consideration in the computation above */
   tex->bo_height += info->qpitch * (num_slices - 1);
}

/**
 * Layout a 3D texture.
 */
static void
layout_tex_3d(struct ilo_texture *tex, const struct layout_tex_info *info)
{
   const struct pipe_resource *templ = &tex->base;
   unsigned int level_y;
   int lv;

   tex->bo_width = 0;
   tex->bo_height = 0;

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
            tex->slice_offsets[lv][slice + i].x = slice_pitch * i;
            tex->slice_offsets[lv][slice + i].y = level_y;
         }

         /* move on to the next slice row */
         level_y += slice_qpitch;
      }

      /* rightmost slice */
      slice = MIN2(num_slices_per_row, level_d) - 1;

      /* extend the size of the monolithic bo to cover this slice */
      if (tex->bo_width < slice_pitch * slice + level_w)
         tex->bo_width = slice_pitch * slice + level_w;
      if (lv == templ->last_level)
         tex->bo_height = (level_y - slice_qpitch) + level_h;
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
get_tex_tiling(const struct ilo_texture *tex)
{
   const struct pipe_resource *templ = &tex->base;
   const enum pipe_format bo_format = tex->bo_format;

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
    * From the Ivy Bridge PRM, volume 4 part 1, page 76:
    *
    *     "The MCS surface must be stored as Tile Y."
    */
   if (templ->bind & ILO_BIND_MCS)
      return INTEL_TILING_Y;

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 318:
    *
    *     "[DevSNB+]: This field (Tiled Surface) must be set to TRUE. Linear
    *      Depth Buffer is not supported."
    *
    *     "The Depth Buffer, if tiled, must use Y-Major tiling."
    */
   if (templ->bind & PIPE_BIND_DEPTH_STENCIL) {
      /* separate stencil uses W-tiling but we do not know how to specify that */
      return (bo_format == PIPE_FORMAT_S8_UINT) ?
         INTEL_TILING_NONE : INTEL_TILING_Y;
   }

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
      if (util_format_get_blocksizebits(bo_format) == 128 &&
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
init_texture(struct ilo_texture *tex)
{
   struct layout_tex_info info;

   switch (tex->base.format) {
   case PIPE_FORMAT_ETC1_RGB8:
      tex->bo_format = PIPE_FORMAT_R8G8B8X8_UNORM;
      break;
   default:
      tex->bo_format = tex->base.format;
      break;
   }

   /* determine tiling first as it may affect the layout */
   tex->tiling = get_tex_tiling(tex);

   layout_tex_init(tex, &info);

   tex->compressed = info.compressed;
   tex->block_width = info.block_width;
   tex->block_height = info.block_height;

   tex->halign_8 = (info.align_i == 8);
   tex->valign_4 = (info.align_j == 4);
   tex->array_spacing_full = info.array_spacing_full;
   tex->interleaved = info.interleaved;

   switch (tex->base.target) {
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_1D_ARRAY:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_CUBE_ARRAY:
      layout_tex_2d(tex, &info);
      break;
   case PIPE_TEXTURE_3D:
      layout_tex_3d(tex, &info);
      break;
   default:
      assert(!"unknown resource target");
      break;
   }

   /*
    * From the Sandy Bridge PRM, volume 1 part 2, page 22:
    *
    *     "A 4KB tile is subdivided into 8-high by 8-wide array of Blocks for
    *      W-Major Tiles (W Tiles). Each Block is 8 rows by 8 bytes."
    *
    * Since we ask for INTEL_TILING_NONE instead lf INTEL_TILING_W, we need to
    * manually align the bo width and height to the tile boundaries.
    */
   if (tex->bo_format == PIPE_FORMAT_S8_UINT) {
      tex->bo_width = align(tex->bo_width, 64);
      tex->bo_height = align(tex->bo_height, 64);
   }

   /* in blocks */
   assert(tex->bo_width % info.block_width == 0);
   assert(tex->bo_height % info.block_height == 0);
   tex->bo_width /= info.block_width;
   tex->bo_height /= info.block_height;
   tex->bo_cpp = util_format_get_blocksize(tex->bo_format);
}

static void
init_buffer(struct ilo_texture *tex)
{
   tex->bo_format = tex->base.format;
   tex->bo_width = tex->base.width0;
   tex->bo_height = 1;
   tex->bo_cpp = 1;
   tex->bo_stride = 0;
   tex->tiling = INTEL_TILING_NONE;

   tex->compressed = false;
   tex->block_width = 1;
   tex->block_height = 1;

   tex->halign_8 = false;
   tex->valign_4 = false;
   tex->array_spacing_full = false;
   tex->interleaved = false;
}

static struct pipe_resource *
create_resource(struct pipe_screen *screen,
                const struct pipe_resource *templ,
                struct winsys_handle *handle)
{
   struct ilo_texture *tex;

   tex = CALLOC_STRUCT(ilo_texture);
   if (!tex)
      return NULL;

   tex->base = *templ;
   tex->base.screen = screen;
   pipe_reference_init(&tex->base.reference, 1);
   tex->handle = handle;

   if (!alloc_slice_offsets(tex)) {
      FREE(tex);
      return NULL;
   }

   if (templ->target == PIPE_BUFFER)
      init_buffer(tex);
   else
      init_texture(tex);

   if (!ilo_texture_alloc_bo(tex)) {
      free_slice_offsets(tex);
      FREE(tex);
      return NULL;
   }

   return &tex->base;
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
                        struct pipe_resource *res,
                        struct winsys_handle *handle)
{
   struct ilo_texture *tex = ilo_texture(res);
   int err;

   err = tex->bo->export_handle(tex->bo, handle);

   return !err;
}

static void
ilo_resource_destroy(struct pipe_screen *screen,
                     struct pipe_resource *res)
{
   struct ilo_texture *tex = ilo_texture(res);

   free_slice_offsets(tex);
   tex->bo->unreference(tex->bo);
   FREE(tex);
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
 * Return the offset (in bytes) to a slice within the bo.
 *
 * When tile_aligned is true, the offset is to the tile containing the start
 * address of the slice.  x_offset and y_offset are offsets (in pixels) from
 * the tile start to slice start.  x_offset is always a multiple of 4 and
 * y_offset is always a multiple of 2.
 */
unsigned
ilo_texture_get_slice_offset(const struct ilo_texture *tex,
                             int level, int slice, bool tile_aligned,
                             unsigned *x_offset, unsigned *y_offset)
{
   const unsigned x = tex->slice_offsets[level][slice].x / tex->block_width;
   const unsigned y = tex->slice_offsets[level][slice].y / tex->block_height;
   unsigned tile_w, tile_h, tile_size, row_size;
   unsigned slice_offset;

   /* see the Sandy Bridge PRM, volume 1 part 2, page 24 */

   switch (tex->tiling) {
   case INTEL_TILING_NONE:
      tile_w = tex->bo_cpp;
      tile_h = 1;
      break;
   case INTEL_TILING_X:
      tile_w = 512;
      tile_h = 8;
      break;
   case INTEL_TILING_Y:
      tile_w = 128;
      tile_h = 32;
      break;
   default:
      assert(!"unknown tiling");
      tile_w = tex->bo_cpp;
      tile_h = 1;
      break;
   }

   tile_size = tile_w * tile_h;
   row_size = tex->bo_stride * tile_h;

   /*
    * for non-tiled resources, this is equivalent to
    *
    *   slice_offset = y * tex->bo_stride + x * tex->bo_cpp;
    */
   slice_offset =
      row_size * (y / tile_h) + tile_size * (x * tex->bo_cpp / tile_w);

   /*
    * Since tex->bo_stride is a multiple of tile_w, slice_offset should be
    * aligned at this point.
    */
   assert(slice_offset % tile_size == 0);

   if (tile_aligned) {
      /*
       * because of the possible values of align_i and align_j in
       * layout_tex_init(), x_offset must be a multiple of 4 and y_offset must
       * be a multiple of 2.
       */
      if (x_offset) {
         assert(tile_w % tex->bo_cpp == 0);
         *x_offset = (x % (tile_w / tex->bo_cpp)) * tex->block_width;
         assert(*x_offset % 4 == 0);
      }
      if (y_offset) {
         *y_offset = (y % tile_h) * tex->block_height;
         assert(*y_offset % 2 == 0);
      }
   }
   else {
      const unsigned tx = (x * tex->bo_cpp) % tile_w;
      const unsigned ty = y % tile_h;

      switch (tex->tiling) {
      case INTEL_TILING_NONE:
         assert(tx == 0 && ty == 0);
         break;
      case INTEL_TILING_X:
         slice_offset += tile_w * ty + tx;
         break;
      case INTEL_TILING_Y:
         slice_offset += tile_h * 16 * (tx / 16) + ty * 16 + (tx % 16);
         break;
      }

      if (x_offset)
         *x_offset = 0;
      if (y_offset)
         *y_offset = 0;
   }

   return slice_offset;
}
