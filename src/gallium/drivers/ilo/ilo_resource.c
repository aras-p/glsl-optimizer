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

struct tex_layout {
   const struct ilo_dev_info *dev;
   const struct pipe_resource *templ;

   bool has_depth, has_stencil;
   bool hiz, separate_stencil;

   enum pipe_format format;
   unsigned block_width, block_height, block_size;
   bool compressed;

   enum intel_tiling_mode tiling;
   bool can_be_linear;

   bool array_spacing_full;
   bool interleaved;

   struct {
      int w, h, d;
      struct ilo_texture_slice *slices;
   } levels[PIPE_MAX_TEXTURE_LEVELS];

   int align_i, align_j;
   int qpitch;

   int width, height;
};

static void
tex_layout_init_qpitch(struct tex_layout *layout)
{
   const struct pipe_resource *templ = layout->templ;
   int h0, h1;

   if (templ->array_size <= 1)
      return;

   h0 = align(layout->levels[0].h, layout->align_j);

   if (!layout->array_spacing_full) {
      layout->qpitch = h0;
      return;
   }

   h1 = align(layout->levels[1].h, layout->align_j);

   /*
    * From the Sandy Bridge PRM, volume 1 part 1, page 115:
    *
    *     "The following equation is used for surface formats other than
    *      compressed textures:
    *
    *        QPitch = (h0 + h1 + 11j)"
    *
    *     "The equation for compressed textures (BC* and FXT1 surface formats)
    *      follows:
    *
    *        QPitch = (h0 + h1 + 11j) / 4"
    *
    *     "[DevSNB] Errata: Sampler MSAA Qpitch will be 4 greater than the
    *      value calculated in the equation above, for every other odd Surface
    *      Height starting from 1 i.e. 1,5,9,13"
    *
    * From the Ivy Bridge PRM, volume 1 part 1, page 111-112:
    *
    *     "If Surface Array Spacing is set to ARYSPC_FULL (note that the depth
    *      buffer and stencil buffer have an implied value of ARYSPC_FULL):
    *
    *        QPitch = (h0 + h1 + 12j)
    *        QPitch = (h0 + h1 + 12j) / 4 (compressed)
    *
    *      (There are many typos or missing words here...)"
    *
    * To access the N-th slice, an offset of (Stride * QPitch * N) is added to
    * the base address.  The PRM divides QPitch by 4 for compressed formats
    * because the block height for those formats are 4, and it wants QPitch to
    * mean the number of memory rows, as opposed to texel rows, between
    * slices.  Since we use texel rows in tex->slice_offsets, we do not need
    * to divide QPitch by 4.
    */
   layout->qpitch = h0 + h1 +
      ((layout->dev->gen >= ILO_GEN(7)) ? 12 : 11) * layout->align_j;

   if (layout->dev->gen == ILO_GEN(6) && templ->nr_samples > 1 &&
       templ->height0 % 4 == 1)
      layout->qpitch += 4;
}

static void
tex_layout_init_alignments(struct tex_layout *layout)
{
   const struct pipe_resource *templ = layout->templ;

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

   if (layout->compressed) {
      /* this happens to be the case */
      layout->align_i = layout->block_width;
      layout->align_j = layout->block_height;
   }
   else if (layout->has_depth || layout->has_stencil) {
      if (layout->dev->gen >= ILO_GEN(7)) {
         switch (layout->format) {
         case PIPE_FORMAT_Z16_UNORM:
            layout->align_i = 8;
            layout->align_j = 4;
            break;
         case PIPE_FORMAT_S8_UINT:
            layout->align_i = 8;
            layout->align_j = 8;
            break;
         default:
            layout->align_i = 4;
            layout->align_j = 4;
            break;
         }
      }
      else {
         switch (layout->format) {
         case PIPE_FORMAT_S8_UINT:
            layout->align_i = 4;
            layout->align_j = 2;
            break;
         default:
            layout->align_i = 4;
            layout->align_j = 4;
            break;
         }
      }
   }
   else {
      const bool valign_4 = (templ->nr_samples > 1) ||
         (layout->dev->gen >= ILO_GEN(7) &&
          layout->tiling == INTEL_TILING_Y &&
          (templ->bind & PIPE_BIND_RENDER_TARGET));

      if (valign_4)
         assert(layout->block_size != 12);

      layout->align_i = 4;
      layout->align_j = (valign_4) ? 4 : 2;
   }

   /*
    * the fact that align i and j are multiples of block width and height
    * respectively is what makes the size of the bo a multiple of the block
    * size, slices start at block boundaries, and many of the computations
    * work.
    */
   assert(layout->align_i % layout->block_width == 0);
   assert(layout->align_j % layout->block_height == 0);

   /* make sure align() works */
   assert(util_is_power_of_two(layout->align_i) &&
          util_is_power_of_two(layout->align_j));
   assert(util_is_power_of_two(layout->block_width) &&
          util_is_power_of_two(layout->block_height));
}

static void
tex_layout_init_levels(struct tex_layout *layout)
{
   const struct pipe_resource *templ = layout->templ;
   int last_level, lv;

   last_level = templ->last_level;

   /* need at least 2 levels to compute full qpitch */
   if (last_level == 0 && templ->array_size > 1 && layout->array_spacing_full)
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
      w = align(w, layout->block_width);
      h = align(h, layout->block_height);

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
      if (layout->interleaved) {
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

      layout->levels[lv].w = w;
      layout->levels[lv].h = h;
      layout->levels[lv].d = d;
   }
}

static void
tex_layout_init_spacing(struct tex_layout *layout)
{
   const struct pipe_resource *templ = layout->templ;

   if (layout->dev->gen >= ILO_GEN(7)) {
      /*
       * It is not explicitly states, but render targets are expected to be
       * UMS/CMS (samples non-interleaved) and depth/stencil buffers are
       * expected to be IMS (samples interleaved).
       *
       * See "Multisampled Surface Storage Format" field of SURFACE_STATE.
       */
      if (layout->has_depth || layout->has_stencil) {
         layout->interleaved = true;

         /*
          * From the Ivy Bridge PRM, volume 1 part 1, page 111:
          *
          *     "note that the depth buffer and stencil buffer have an implied
          *      value of ARYSPC_FULL"
          */
         layout->array_spacing_full = true;
      }
      else {
         layout->interleaved = false;

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
         layout->array_spacing_full = (templ->last_level > 0);
      }
   }
   else {
      /* GEN6 supports only interleaved samples */
      layout->interleaved = true;

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
      layout->array_spacing_full = (layout->format != PIPE_FORMAT_S8_UINT);
   }
}

static void
tex_layout_init_tiling(struct tex_layout *layout)
{
   const struct pipe_resource *templ = layout->templ;
   const enum pipe_format format = layout->format;
   const unsigned tile_none = 1 << INTEL_TILING_NONE;
   const unsigned tile_x = 1 << INTEL_TILING_X;
   const unsigned tile_y = 1 << INTEL_TILING_Y;
   unsigned valid_tilings = tile_none | tile_x | tile_y;

   /*
    * From the Sandy Bridge PRM, volume 1 part 2, page 32:
    *
    *     "Display/Overlay   Y-Major not supported.
    *                        X-Major required for Async Flips"
    */
   if (unlikely(templ->bind & PIPE_BIND_SCANOUT))
      valid_tilings &= tile_x;

   /*
    * From the Sandy Bridge PRM, volume 3 part 2, page 158:
    *
    *     "The cursor surface address must be 4K byte aligned. The cursor must
    *      be in linear memory, it cannot be tiled."
    */
   if (unlikely(templ->bind & (PIPE_BIND_CURSOR | PIPE_BIND_LINEAR)))
      valid_tilings &= tile_none;

   /*
    * From the Ivy Bridge PRM, volume 4 part 1, page 76:
    *
    *     "The MCS surface must be stored as Tile Y."
    */
   if (templ->bind & ILO_BIND_MCS)
      valid_tilings &= tile_y;

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 318:
    *
    *     "[DevSNB+]: This field (Tiled Surface) must be set to TRUE. Linear
    *      Depth Buffer is not supported."
    *
    *     "The Depth Buffer, if tiled, must use Y-Major tiling."
    *
    * From the Sandy Bridge PRM, volume 1 part 2, page 22:
    *
    *     "W-Major Tile Format is used for separate stencil."
    *
    * Since the HW does not support W-tiled fencing, we have to do it in the
    * driver.
    */
   if (templ->bind & PIPE_BIND_DEPTH_STENCIL) {
      switch (format) {
      case PIPE_FORMAT_S8_UINT:
         valid_tilings &= tile_none;
         break;
      default:
         valid_tilings &= tile_y;
         break;
      }
   }

   if (templ->bind & (PIPE_BIND_RENDER_TARGET | PIPE_BIND_SAMPLER_VIEW)) {
      if (templ->bind & PIPE_BIND_RENDER_TARGET) {
         /*
          * From the Sandy Bridge PRM, volume 1 part 2, page 32:
          *
          *     "NOTE: 128BPE Format Color buffer ( render target ) MUST be
          *      either TileX or Linear."
          */
         if (layout->block_size == 16)
            valid_tilings &= ~tile_y;

         /*
          * From the Ivy Bridge PRM, volume 4 part 1, page 63:
          *
          *     "This field (Surface Vertical Aligment) must be set to
          *      VALIGN_4 for all tiled Y Render Target surfaces."
          *
          *     "VALIGN_4 is not supported for surface format
          *      R32G32B32_FLOAT."
          */
         if (layout->dev->gen >= ILO_GEN(7) && layout->block_size == 12)
            valid_tilings &= ~tile_y;
      }

      /*
       * Also, heuristically set a minimum width/height for enabling tiling.
       */
      if (templ->width0 < 64 && (valid_tilings & ~tile_x))
         valid_tilings &= ~tile_x;

      if ((templ->width0 < 32 || templ->height0 < 16) &&
          (templ->width0 < 16 || templ->height0 < 32) &&
          (valid_tilings & ~tile_y))
         valid_tilings &= ~tile_y;
   }
   else {
      /* force linear if we are not sure where the texture is bound to */
      if (valid_tilings & tile_none)
         valid_tilings &= tile_none;
   }

   /* no conflicting binding flags */
   assert(valid_tilings);

   /* prefer tiled than linear */
   if (valid_tilings & tile_y)
      layout->tiling = INTEL_TILING_Y;
   else if (valid_tilings & tile_x)
      layout->tiling = INTEL_TILING_X;
   else
      layout->tiling = INTEL_TILING_NONE;

   layout->can_be_linear = valid_tilings & tile_none;
}

static void
tex_layout_init_format(struct tex_layout *layout)
{
   const struct pipe_resource *templ = layout->templ;
   enum pipe_format format;

   switch (templ->format) {
   case PIPE_FORMAT_ETC1_RGB8:
      format = PIPE_FORMAT_R8G8B8X8_UNORM;
      break;
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      if (layout->separate_stencil)
         format = PIPE_FORMAT_Z24X8_UNORM;
      else
         format = templ->format;
      break;
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      if (layout->separate_stencil)
         format = PIPE_FORMAT_Z32_FLOAT;
      else
         format = templ->format;
      break;
   default:
      format = templ->format;
      break;
   }

   layout->format = format;

   layout->block_width = util_format_get_blockwidth(format);
   layout->block_height = util_format_get_blockheight(format);
   layout->block_size = util_format_get_blocksize(format);
   layout->compressed = util_format_is_compressed(format);
}

static void
tex_layout_init_hiz(struct tex_layout *layout)
{
   const struct pipe_resource *templ = layout->templ;
   const struct util_format_description *desc;

   desc = util_format_description(templ->format);
   layout->has_depth = util_format_has_depth(desc);
   layout->has_stencil = util_format_has_stencil(desc);

   if (!layout->has_depth)
      return;

   layout->hiz = true;

   /* no point in having HiZ */
   if (templ->usage == PIPE_USAGE_STAGING)
      layout->hiz = false;

   if (layout->dev->gen == ILO_GEN(6)) {
      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 312:
       *
       *     "The hierarchical depth buffer does not support the LOD field, it
       *      is assumed by hardware to be zero. A separate hierarachical
       *      depth buffer is required for each LOD used, and the
       *      corresponding buffer's state delivered to hardware each time a
       *      new depth buffer state with modified LOD is delivered."
       *
       * But we have a stronger requirement.  Because of layer offsetting
       * (check out the callers of ilo_texture_get_slice_offset()), we already
       * have to require the texture to be non-mipmapped and non-array.
       */
      if (templ->last_level > 0 || templ->array_size > 1 || templ->depth0 > 1)
         layout->hiz = false;
   }

   if (ilo_debug & ILO_DEBUG_NOHIZ)
      layout->hiz = false;

   if (layout->has_stencil) {
      /*
       * From the Sandy Bridge PRM, volume 2 part 1, page 317:
       *
       *     "This field (Separate Stencil Buffer Enable) must be set to the
       *      same value (enabled or disabled) as Hierarchical Depth Buffer
       *      Enable."
       *
       * GEN7+ requires separate stencil buffers.
       */
      if (layout->dev->gen >= ILO_GEN(7))
         layout->separate_stencil = true;
      else
         layout->separate_stencil = layout->hiz;

      if (layout->separate_stencil)
         layout->has_stencil = false;
   }
}

static void
tex_layout_init(struct tex_layout *layout,
                struct pipe_screen *screen,
                const struct pipe_resource *templ,
                struct ilo_texture_slice **slices)
{
   struct ilo_screen *is = ilo_screen(screen);

   memset(layout, 0, sizeof(*layout));

   layout->dev = &is->dev;
   layout->templ = templ;

   /* note that there are dependencies between these functions */
   tex_layout_init_hiz(layout);
   tex_layout_init_format(layout);
   tex_layout_init_tiling(layout);
   tex_layout_init_spacing(layout);
   tex_layout_init_levels(layout);
   tex_layout_init_alignments(layout);
   tex_layout_init_qpitch(layout);

   if (slices) {
      int lv;

      for (lv = 0; lv <= templ->last_level; lv++)
         layout->levels[lv].slices = slices[lv];
   }
}

static bool
tex_layout_force_linear(struct tex_layout *layout)
{
   if (!layout->can_be_linear)
      return false;

   /*
    * we may be able to switch from VALIGN_4 to VALIGN_2 when the layout was
    * Y-tiled, but let's keep it simple
    */
   layout->tiling = INTEL_TILING_NONE;

   return true;
}

/**
 * Layout a 2D texture.
 */
static void
tex_layout_2d(struct tex_layout *layout)
{
   const struct pipe_resource *templ = layout->templ;
   unsigned int level_x, level_y, num_slices;
   int lv;

   level_x = 0;
   level_y = 0;
   for (lv = 0; lv <= templ->last_level; lv++) {
      const unsigned int level_w = layout->levels[lv].w;
      const unsigned int level_h = layout->levels[lv].h;
      int slice;

      /* set slice offsets */
      if (layout->levels[lv].slices) {
         for (slice = 0; slice < templ->array_size; slice++) {
            layout->levels[lv].slices[slice].x = level_x;
            /* slices are qpitch apart in Y-direction */
            layout->levels[lv].slices[slice].y =
               level_y + layout->qpitch * slice;
         }
      }

      /* extend the size of the monolithic bo to cover this mip level */
      if (layout->width < level_x + level_w)
         layout->width = level_x + level_w;
      if (layout->height < level_y + level_h)
         layout->height = level_y + level_h;

      /* MIPLAYOUT_BELOW */
      if (lv == 1)
         level_x += align(level_w, layout->align_i);
      else
         level_y += align(level_h, layout->align_j);
   }

   num_slices = templ->array_size;
   /* samples of the same index are stored in a slice */
   if (templ->nr_samples > 1 && !layout->interleaved)
      num_slices *= templ->nr_samples;

   /* we did not take slices into consideration in the computation above */
   layout->height += layout->qpitch * (num_slices - 1);
}

/**
 * Layout a 3D texture.
 */
static void
tex_layout_3d(struct tex_layout *layout)
{
   const struct pipe_resource *templ = layout->templ;
   unsigned int level_y;
   int lv;

   level_y = 0;
   for (lv = 0; lv <= templ->last_level; lv++) {
      const unsigned int level_w = layout->levels[lv].w;
      const unsigned int level_h = layout->levels[lv].h;
      const unsigned int level_d = layout->levels[lv].d;
      const unsigned int slice_pitch = align(level_w, layout->align_i);
      const unsigned int slice_qpitch = align(level_h, layout->align_j);
      const unsigned int num_slices_per_row = 1 << lv;
      int slice;

      for (slice = 0; slice < level_d; slice += num_slices_per_row) {
         int i;

         /* set slice offsets */
         if (layout->levels[lv].slices) {
            for (i = 0; i < num_slices_per_row && slice + i < level_d; i++) {
               layout->levels[lv].slices[slice + i].x = slice_pitch * i;
               layout->levels[lv].slices[slice + i].y = level_y;
            }
         }

         /* move on to the next slice row */
         level_y += slice_qpitch;
      }

      /* rightmost slice */
      slice = MIN2(num_slices_per_row, level_d) - 1;

      /* extend the size of the monolithic bo to cover this slice */
      if (layout->width < slice_pitch * slice + level_w)
         layout->width = slice_pitch * slice + level_w;
      if (lv == templ->last_level)
         layout->height = (level_y - slice_qpitch) + level_h;
   }
}

static void
tex_layout_validate(struct tex_layout *layout)
{
   /*
    * From the Sandy Bridge PRM, volume 1 part 1, page 118:
    *
    *     "To determine the necessary padding on the bottom and right side of
    *      the surface, refer to the table in Section 7.18.3.4 for the i and j
    *      parameters for the surface format in use. The surface must then be
    *      extended to the next multiple of the alignment unit size in each
    *      dimension, and all texels contained in this extended surface must
    *      have valid GTT entries."
    *
    *     "For cube surfaces, an additional two rows of padding are required
    *      at the bottom of the surface. This must be ensured regardless of
    *      whether the surface is stored tiled or linear.  This is due to the
    *      potential rotation of cache line orientation from memory to cache."
    *
    *     "For compressed textures (BC* and FXT1 surface formats), padding at
    *      the bottom of the surface is to an even compressed row, which is
    *      equal to a multiple of 8 uncompressed texel rows. Thus, for padding
    *      purposes, these surfaces behave as if j = 8 only for surface
    *      padding purposes. The value of 4 for j still applies for mip level
    *      alignment and QPitch calculation."
    */
   if (layout->templ->bind & PIPE_BIND_SAMPLER_VIEW) {
      layout->width = align(layout->width, layout->align_i);
      layout->height = align(layout->height, layout->align_j);

      if (layout->templ->target == PIPE_TEXTURE_CUBE)
         layout->height += 2;

      if (layout->compressed)
         layout->height = align(layout->height, layout->align_j * 2);
   }

   /*
    * From the Sandy Bridge PRM, volume 1 part 1, page 118:
    *
    *     "If the surface contains an odd number of rows of data, a final row
    *      below the surface must be allocated."
    */
   if (layout->templ->bind & PIPE_BIND_RENDER_TARGET)
      layout->height = align(layout->height, 2);

   /*
    * From the Sandy Bridge PRM, volume 1 part 2, page 22:
    *
    *     "A 4KB tile is subdivided into 8-high by 8-wide array of Blocks for
    *      W-Major Tiles (W Tiles). Each Block is 8 rows by 8 bytes."
    *
    * Since we ask for INTEL_TILING_NONE instead of the non-existent
    * INTEL_TILING_W, we need to manually align the width and height to the
    * tile boundaries.
    */
   if (layout->templ->format == PIPE_FORMAT_S8_UINT) {
      layout->width = align(layout->width, 64);
      layout->height = align(layout->height, 64);
   }

   /*
    * Depth Buffer Clear/Resolve works in 8x4 sample blocks.  In
    * ilo_texture_can_enable_hiz(), we always return true for the first slice.
    * To avoid out-of-bound access, we have to pad.
    */
   if (layout->hiz) {
      layout->width = align(layout->width, 8);
      layout->height = align(layout->height, 4);
   }

   assert(layout->width % layout->block_width == 0);
   assert(layout->height % layout->block_height == 0);
   assert(layout->qpitch % layout->block_height == 0);
}

static size_t
tex_layout_estimate_size(const struct tex_layout *layout)
{
   unsigned stride, height;

   stride = (layout->width / layout->block_width) * layout->block_size;
   height = layout->height / layout->block_height;

   switch (layout->tiling) {
   case INTEL_TILING_X:
      stride = align(stride, 512);
      height = align(height, 8);
      break;
   case INTEL_TILING_Y:
      stride = align(stride, 128);
      height = align(height, 32);
      break;
   default:
      height = align(height, 2);
      break;
   }

   return stride * height;
}

static void
tex_layout_apply(const struct tex_layout *layout, struct ilo_texture *tex)
{
   tex->bo_format = layout->format;

   /* in blocks */
   tex->bo_width = layout->width / layout->block_width;
   tex->bo_height = layout->height / layout->block_height;
   tex->bo_cpp = layout->block_size;
   tex->tiling = layout->tiling;

   tex->compressed = layout->compressed;
   tex->block_width = layout->block_width;
   tex->block_height = layout->block_height;

   tex->halign_8 = (layout->align_i == 8);
   tex->valign_4 = (layout->align_j == 4);
   tex->array_spacing_full = layout->array_spacing_full;
   tex->interleaved = layout->interleaved;
}

static void
tex_free_slices(struct ilo_texture *tex)
{
   FREE(tex->slices[0]);
}

static bool
tex_alloc_slices(struct ilo_texture *tex)
{
   const struct pipe_resource *templ = &tex->base;
   struct ilo_texture_slice *slices;
   int depth, lv;

   /* sum the depths of all levels */
   depth = 0;
   for (lv = 0; lv <= templ->last_level; lv++)
      depth += u_minify(templ->depth0, lv);

   /*
    * There are (depth * tex->base.array_size) slices in total.  Either depth
    * is one (non-3D) or templ->array_size is one (non-array), but it does
    * not matter.
    */
   slices = CALLOC(depth * templ->array_size, sizeof(*slices));
   if (!slices)
      return false;

   tex->slices[0] = slices;

   /* point to the respective positions in the buffer */
   for (lv = 1; lv <= templ->last_level; lv++) {
      tex->slices[lv] = tex->slices[lv - 1] +
         u_minify(templ->depth0, lv - 1) * templ->array_size;
   }

   return true;
}

static bool
tex_create_bo(struct ilo_texture *tex,
              const struct winsys_handle *handle)
{
   struct ilo_screen *is = ilo_screen(tex->base.screen);
   const char *name;
   struct intel_bo *bo;
   enum intel_tiling_mode tiling;
   unsigned long pitch;

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

   if (handle) {
      bo = intel_winsys_import_handle(is->winsys, name, handle,
            tex->bo_width, tex->bo_height, tex->bo_cpp,
            &tiling, &pitch);
   }
   else {
      const uint32_t initial_domain =
         (tex->base.bind & (PIPE_BIND_DEPTH_STENCIL |
                            PIPE_BIND_RENDER_TARGET)) ?
         INTEL_DOMAIN_RENDER : 0;

      bo = intel_winsys_alloc_texture(is->winsys, name,
            tex->bo_width, tex->bo_height, tex->bo_cpp,
            tex->tiling, initial_domain, &pitch);

      tiling = tex->tiling;
   }

   if (!bo)
      return false;

   if (tex->bo)
      intel_bo_unreference(tex->bo);

   tex->bo = bo;
   tex->tiling = tiling;
   tex->bo_stride = pitch;

   return true;
}

static bool
tex_create_separate_stencil(struct ilo_texture *tex)
{
   struct pipe_resource templ = tex->base;
   struct pipe_resource *s8;

   /*
    * Unless PIPE_BIND_DEPTH_STENCIL is set, the resource may have other
    * tilings.  But that should be fine since it will never be bound as the
    * stencil buffer, and our transfer code can handle all tilings.
    */
   templ.format = PIPE_FORMAT_S8_UINT;

   s8 = tex->base.screen->resource_create(tex->base.screen, &templ);
   if (!s8)
      return false;

   tex->separate_s8 = ilo_texture(s8);

   assert(tex->separate_s8->bo_format == PIPE_FORMAT_S8_UINT);

   return true;
}

static bool
tex_create_hiz(struct ilo_texture *tex, const struct tex_layout *layout)
{
   struct ilo_screen *is = ilo_screen(tex->base.screen);
   const struct pipe_resource *templ = layout->templ;
   const int hz_align_j = 8;
   unsigned hz_width, hz_height, lv;
   unsigned long pitch;

   /*
    * See the Sandy Bridge PRM, volume 2 part 1, page 312, and the Ivy Bridge
    * PRM, volume 2 part 1, page 312-313.
    *
    * It seems HiZ buffer is aligned to 8x8, with every two rows packed into a
    * memory row.
    */

   hz_width = align(layout->levels[0].w, 16);

   if (templ->target == PIPE_TEXTURE_3D) {
      hz_height = 0;

      for (lv = 0; lv <= templ->last_level; lv++) {
         const unsigned h = align(layout->levels[lv].h, hz_align_j);
         hz_height += h * layout->levels[lv].d;
      }

      hz_height /= 2;
   }
   else {
      const unsigned h0 = align(layout->levels[0].h, hz_align_j);
      unsigned hz_qpitch = h0;

      if (layout->array_spacing_full) {
         const unsigned h1 = align(layout->levels[1].h, hz_align_j);
         const unsigned htail =
            ((layout->dev->gen >= ILO_GEN(7)) ? 12 : 11) * hz_align_j;

         hz_qpitch += h1 + htail;
      }

      hz_height = hz_qpitch * templ->array_size / 2;

      if (layout->dev->gen >= ILO_GEN(7))
         hz_height = align(hz_height, 8);
   }

   tex->hiz.bo = intel_winsys_alloc_texture(is->winsys,
         "hiz texture", hz_width, hz_height, 1,
         INTEL_TILING_Y, INTEL_DOMAIN_RENDER, &pitch);
   if (!tex->hiz.bo)
      return false;

   tex->hiz.bo_stride = pitch;

   /*
    * From the Sandy Bridge PRM, volume 2 part 1, page 313-314:
    *
    *     "A rectangle primitive representing the clear area is delivered. The
    *      primitive must adhere to the following restrictions on size:
    *
    *      - If Number of Multisamples is NUMSAMPLES_1, the rectangle must be
    *        aligned to an 8x4 pixel block relative to the upper left corner
    *        of the depth buffer, and contain an integer number of these pixel
    *        blocks, and all 8x4 pixels must be lit.
    *
    *      - If Number of Multisamples is NUMSAMPLES_4, the rectangle must be
    *        aligned to a 4x2 pixel block (8x4 sample block) relative to the
    *        upper left corner of the depth buffer, and contain an integer
    *        number of these pixel blocks, and all samples of the 4x2 pixels
    *        must be lit
    *
    *      - If Number of Multisamples is NUMSAMPLES_8, the rectangle must be
    *        aligned to a 2x2 pixel block (8x4 sample block) relative to the
    *        upper left corner of the depth buffer, and contain an integer
    *        number of these pixel blocks, and all samples of the 2x2 pixels
    *        must be list."
    *
    *     "The following is required when performing a depth buffer resolve:
    *
    *      - A rectangle primitive of the same size as the previous depth
    *        buffer clear operation must be delivered, and depth buffer state
    *        cannot have changed since the previous depth buffer clear
    *        operation."
    *
    * Experiments on Haswell show that depth buffer resolves have the same
    * alignment requirements, and aligning the RECTLIST primitive and
    * 3DSTATE_DRAWING_RECTANGLE alone are not enough.  The mipmap size must be
    * aligned.
    */
   for (lv = 0; lv <= templ->last_level; lv++) {
      unsigned align_w = 8, align_h = 4;
      unsigned flags = 0;

      switch (templ->nr_samples) {
      case 0:
      case 1:
         break;
      case 2:
         align_w /= 2;
         break;
      case 4:
         align_w /= 2;
         align_h /= 2;
         break;
      case 8:
      default:
         align_w /= 4;
         align_h /= 2;
         break;
      }

      if (u_minify(templ->width0, lv) % align_w == 0 &&
          u_minify(templ->height0, lv) % align_h == 0) {
         flags |= ILO_TEXTURE_HIZ;

         /* this will trigger a HiZ resolve */
         if (tex->imported)
            flags |= ILO_TEXTURE_CPU_WRITE;
      }

      if (flags) {
         const unsigned num_slices = (templ->target == PIPE_TEXTURE_3D) ?
            u_minify(templ->depth0, lv) : templ->array_size;
         ilo_texture_set_slice_flags(tex, lv, 0, num_slices, flags, flags);
      }
   }

   return true;
}

static void
tex_destroy(struct ilo_texture *tex)
{
   if (tex->hiz.bo)
      intel_bo_unreference(tex->hiz.bo);

   if (tex->separate_s8)
      tex_destroy(tex->separate_s8);

   intel_bo_unreference(tex->bo);
   tex_free_slices(tex);
   FREE(tex);
}

static struct pipe_resource *
tex_create(struct pipe_screen *screen,
           const struct pipe_resource *templ,
           const struct winsys_handle *handle)
{
   struct tex_layout layout;
   struct ilo_texture *tex;

   tex = CALLOC_STRUCT(ilo_texture);
   if (!tex)
      return NULL;

   tex->base = *templ;
   tex->base.screen = screen;
   pipe_reference_init(&tex->base.reference, 1);

   if (!tex_alloc_slices(tex)) {
      FREE(tex);
      return NULL;
   }

   tex->imported = (handle != NULL);

   tex_layout_init(&layout, screen, templ, tex->slices);

   switch (templ->target) {
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_1D_ARRAY:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_CUBE_ARRAY:
      tex_layout_2d(&layout);
      break;
   case PIPE_TEXTURE_3D:
      tex_layout_3d(&layout);
      break;
   default:
      assert(!"unknown resource target");
      break;
   }

   tex_layout_validate(&layout);

   /* make sure the bo can be mapped through GTT if tiled */
   if (layout.tiling != INTEL_TILING_NONE) {
      /*
       * Usually only the first 256MB of the GTT is mappable.
       *
       * See also how intel_context::max_gtt_map_object_size is calculated.
       */
      const size_t mappable_gtt_size = 256 * 1024 * 1024;
      const size_t size = tex_layout_estimate_size(&layout);

      /* be conservative */
      if (size > mappable_gtt_size / 4)
         tex_layout_force_linear(&layout);
   }

   tex_layout_apply(&layout, tex);

   if (!tex_create_bo(tex, handle)) {
      tex_free_slices(tex);
      FREE(tex);
      return NULL;
   }

   /* allocate separate stencil resource */
   if (layout.separate_stencil && !tex_create_separate_stencil(tex)) {
      tex_destroy(tex);
      return NULL;
   }

   if (layout.hiz && !tex_create_hiz(tex, &layout)) {
      /* Separate Stencil Buffer requires HiZ to be enabled */
      if (layout.dev->gen == ILO_GEN(6) && layout.separate_stencil) {
         tex_destroy(tex);
         return NULL;
      }
   }

   return &tex->base;
}

static bool
tex_get_handle(struct ilo_texture *tex, struct winsys_handle *handle)
{
   struct ilo_screen *is = ilo_screen(tex->base.screen);
   int err;

   err = intel_winsys_export_handle(is->winsys, tex->bo,
         tex->tiling, tex->bo_stride, handle);

   return !err;
}

/**
 * Estimate the texture size.  For large textures, the errors should be pretty
 * small.
 */
static size_t
tex_estimate_size(struct pipe_screen *screen,
                  const struct pipe_resource *templ)
{
   struct tex_layout layout;

   tex_layout_init(&layout, screen, templ, NULL);

   switch (templ->target) {
   case PIPE_TEXTURE_3D:
      tex_layout_3d(&layout);
      break;
   default:
      tex_layout_2d(&layout);
      break;
   }

   tex_layout_validate(&layout);

   return tex_layout_estimate_size(&layout);
}

static bool
buf_create_bo(struct ilo_buffer *buf)
{
   const uint32_t initial_domain =
      (buf->base.bind & PIPE_BIND_STREAM_OUTPUT) ?
      INTEL_DOMAIN_RENDER : 0;
   struct ilo_screen *is = ilo_screen(buf->base.screen);
   const char *name;
   struct intel_bo *bo;

   switch (buf->base.bind) {
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

   bo = intel_winsys_alloc_buffer(is->winsys,
         name, buf->bo_size, initial_domain);
   if (!bo)
      return false;

   if (buf->bo)
      intel_bo_unreference(buf->bo);

   buf->bo = bo;

   return true;
}

static void
buf_destroy(struct ilo_buffer *buf)
{
   intel_bo_unreference(buf->bo);
   FREE(buf);
}

static struct pipe_resource *
buf_create(struct pipe_screen *screen, const struct pipe_resource *templ)
{
   struct ilo_buffer *buf;

   buf = CALLOC_STRUCT(ilo_buffer);
   if (!buf)
      return NULL;

   buf->base = *templ;
   buf->base.screen = screen;
   pipe_reference_init(&buf->base.reference, 1);

   buf->bo_size = templ->width0;

   /*
    * From the Sandy Bridge PRM, volume 1 part 1, page 118:
    *
    *     "For buffers, which have no inherent "height," padding requirements
    *      are different. A buffer must be padded to the next multiple of 256
    *      array elements, with an additional 16 bytes added beyond that to
    *      account for the L1 cache line."
    */
   if (templ->bind & PIPE_BIND_SAMPLER_VIEW)
      buf->bo_size = align(buf->bo_size, 256) + 16;

   if (templ->bind & PIPE_BIND_VERTEX_BUFFER) {
      /*
       * As noted in ilo_translate_format(), we treat some 3-component formats
       * as 4-component formats to work around hardware limitations.  Imagine
       * the case where the vertex buffer holds a single
       * PIPE_FORMAT_R16G16B16_FLOAT vertex, and buf->bo_size is 6.  The
       * hardware would fail to fetch it at boundary check because the vertex
       * buffer is expected to hold a PIPE_FORMAT_R16G16B16A16_FLOAT vertex
       * and that takes at least 8 bytes.
       *
       * For the workaround to work, we should add 2 to the bo size.  But that
       * would waste a page when the bo size is already page aligned.  Let's
       * round it to page size for now and revisit this when needed.
       */
      buf->bo_size = align(buf->bo_size, 4096);
   }

   if (!buf_create_bo(buf)) {
      FREE(buf);
      return NULL;
   }

   return &buf->base;
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
   size_t size;

   if (templ->target == PIPE_BUFFER)
      size = templ->width0;
   else
      size = tex_estimate_size(screen, templ);

   return (size <= max_size);
}

static struct pipe_resource *
ilo_resource_create(struct pipe_screen *screen,
                    const struct pipe_resource *templ)
{
   if (templ->target == PIPE_BUFFER)
      return buf_create(screen, templ);
   else
      return tex_create(screen, templ, NULL);
}

static struct pipe_resource *
ilo_resource_from_handle(struct pipe_screen *screen,
                         const struct pipe_resource *templ,
                         struct winsys_handle *handle)
{
   if (templ->target == PIPE_BUFFER)
      return NULL;
   else
      return tex_create(screen, templ, handle);
}

static boolean
ilo_resource_get_handle(struct pipe_screen *screen,
                        struct pipe_resource *res,
                        struct winsys_handle *handle)
{
   if (res->target == PIPE_BUFFER)
      return false;
   else
      return tex_get_handle(ilo_texture(res), handle);

}

static void
ilo_resource_destroy(struct pipe_screen *screen,
                     struct pipe_resource *res)
{
   if (res->target == PIPE_BUFFER)
      buf_destroy(ilo_buffer(res));
   else
      tex_destroy(ilo_texture(res));
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

bool
ilo_buffer_alloc_bo(struct ilo_buffer *buf)
{
   return buf_create_bo(buf);
}

bool
ilo_texture_alloc_bo(struct ilo_texture *tex)
{
   /* a shared bo cannot be reallocated */
   if (tex->imported)
      return false;

   return tex_create_bo(tex, NULL);
}

/**
 * Return the offset (in bytes) to a slice within the bo.
 *
 * The returned offset is aligned to tile size.  Since slices are not
 * guaranteed to start at tile boundaries, the X and Y offsets (in pixels)
 * from the tile origin to the slice are also returned.  X offset is always a
 * multiple of 4 and Y offset is always a multiple of 2.
 */
unsigned
ilo_texture_get_slice_offset(const struct ilo_texture *tex,
                             unsigned level, unsigned slice,
                             unsigned *x_offset, unsigned *y_offset)
{
   const struct ilo_texture_slice *s =
      ilo_texture_get_slice(tex, level, slice);
   unsigned tile_w, tile_h, tile_size, row_size;
   unsigned x, y, slice_offset;

   /* see the Sandy Bridge PRM, volume 1 part 2, page 24 */

   switch (tex->tiling) {
   case INTEL_TILING_NONE:
      /* W-tiled */
      if (tex->bo_format == PIPE_FORMAT_S8_UINT) {
         tile_w = 64;
         tile_h = 64;
      }
      else {
         tile_w = 1;
         tile_h = 1;
      }
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
      tile_w = 1;
      tile_h = 1;
      break;
   }

   tile_size = tile_w * tile_h;
   row_size = tex->bo_stride * tile_h;

   /* in bytes */
   x = s->x / tex->block_width * tex->bo_cpp;
   y = s->y / tex->block_height;
   slice_offset = row_size * (y / tile_h) + tile_size * (x / tile_w);

   /*
    * Since tex->bo_stride is a multiple of tile_w, slice_offset should be
    * aligned at this point.
    */
   assert(slice_offset % tile_size == 0);

   /*
    * because of the possible values of align_i and align_j in
    * tex_layout_init_alignments(), x_offset is guaranteed to be a multiple of
    * 4 and y_offset is guaranteed to be a multiple of 2.
    */
   if (x_offset) {
      /* in pixels */
      x = (x % tile_w) / tex->bo_cpp * tex->block_width;
      assert(x % 4 == 0);

      *x_offset = x;
   }

   if (y_offset) {
      /* in pixels */
      y = (y % tile_h) * tex->block_height;
      assert(y % 2 == 0);

      *y_offset = y;
   }

   return slice_offset;
}
