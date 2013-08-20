
#ifndef __NV50_BLIT_H__
#define __NV50_BLIT_H__

#include "util/u_inlines.h"
#include "util/u_format.h"

void *
nv50_blitter_make_fp(struct pipe_context *,
                     unsigned mode,
                     enum pipe_texture_target);

unsigned
nv50_blit_select_mode(const struct pipe_blit_info *);

/* Converted to a pipe->blit. */
void
nv50_resource_resolve(struct pipe_context *, const struct pipe_resolve_info *);

#define NV50_BLIT_MODE_PASS  0 /* pass through TEX $t0/$s0 output */
#define NV50_BLIT_MODE_Z24S8 1 /* encode ZS values for RGBA unorm8 */
#define NV50_BLIT_MODE_S8Z24 2
#define NV50_BLIT_MODE_X24S8 3
#define NV50_BLIT_MODE_S8X24 4
#define NV50_BLIT_MODE_Z24X8 5
#define NV50_BLIT_MODE_X8Z24 6
#define NV50_BLIT_MODE_ZS    7 /* put $t0/$s0 into R, $t1/$s1 into G */
#define NV50_BLIT_MODE_XS    8 /* put $t1/$s1 into G */
#define NV50_BLIT_MODES      9

/* CUBE and RECT textures are reinterpreted as 2D(_ARRAY) */
#define NV50_BLIT_TEXTURE_BUFFER    0
#define NV50_BLIT_TEXTURE_1D        1
#define NV50_BLIT_TEXTURE_2D        2
#define NV50_BLIT_TEXTURE_3D        3
#define NV50_BLIT_TEXTURE_1D_ARRAY  4
#define NV50_BLIT_TEXTURE_2D_ARRAY  5
#define NV50_BLIT_MAX_TEXTURE_TYPES 6

static INLINE unsigned
nv50_blit_texture_type(enum pipe_texture_target target)
{
   switch (target) {
   case PIPE_TEXTURE_1D: return NV50_BLIT_TEXTURE_1D;
   case PIPE_TEXTURE_2D: return NV50_BLIT_TEXTURE_2D;
   case PIPE_TEXTURE_3D: return NV50_BLIT_TEXTURE_3D;
   case PIPE_TEXTURE_1D_ARRAY: return NV50_BLIT_TEXTURE_1D_ARRAY;
   case PIPE_TEXTURE_2D_ARRAY: return NV50_BLIT_TEXTURE_2D_ARRAY;
   default:
      assert(target == PIPE_BUFFER);
      return NV50_BLIT_TEXTURE_BUFFER;
   }
}

static INLINE unsigned
nv50_blit_get_tgsi_texture_target(enum pipe_texture_target target)
{
   switch (target) {
   case PIPE_TEXTURE_1D: return TGSI_TEXTURE_1D;
   case PIPE_TEXTURE_2D: return TGSI_TEXTURE_2D;
   case PIPE_TEXTURE_3D: return TGSI_TEXTURE_3D;
   case PIPE_TEXTURE_1D_ARRAY: return TGSI_TEXTURE_1D_ARRAY;
   case PIPE_TEXTURE_2D_ARRAY: return TGSI_TEXTURE_2D_ARRAY;
   default:
      assert(target == PIPE_BUFFER);
      return TGSI_TEXTURE_BUFFER;
   }
}

static INLINE enum pipe_texture_target
nv50_blit_reinterpret_pipe_texture_target(enum pipe_texture_target target)
{
   switch (target) {
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_CUBE_ARRAY:
      return PIPE_TEXTURE_2D_ARRAY;
   case PIPE_TEXTURE_RECT:
      return PIPE_TEXTURE_2D;
   default:
      return target;
   }
}

static INLINE unsigned
nv50_blit_get_filter(const struct pipe_blit_info *info)
{
   if (info->dst.resource->nr_samples < info->src.resource->nr_samples)
      return util_format_is_depth_or_stencil(info->src.format) ? 0 : 1;

   if (info->filter != PIPE_TEX_FILTER_LINEAR)
      return 0;

   if ((info->dst.box.width ==  info->src.box.width ||
        info->dst.box.width == -info->src.box.width) &&
       (info->dst.box.height ==  info->src.box.height ||
        info->dst.box.height == -info->src.box.height))
      return 0;

   return 1;
}

/* Since shaders cannot export stencil, we cannot copy stencil values when
 * rendering to ZETA, so we attach the ZS surface to a colour render target.
 */
static INLINE enum pipe_format
nv50_blit_zeta_to_colour_format(enum pipe_format format)
{
   switch (format) {
   case PIPE_FORMAT_Z16_UNORM:
      return PIPE_FORMAT_R16_UNORM;
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
   case PIPE_FORMAT_Z24X8_UNORM:
      return PIPE_FORMAT_R8G8B8A8_UNORM;
   case PIPE_FORMAT_Z32_FLOAT:
      return PIPE_FORMAT_R32_FLOAT;
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      return PIPE_FORMAT_R32G32_FLOAT;
   default:
      assert(0);
      return PIPE_FORMAT_NONE;
   }
}


static INLINE uint16_t
nv50_blit_derive_color_mask(const struct pipe_blit_info *info)
{
   const unsigned mask = info->mask;

   uint16_t color_mask = 0;

   switch (info->dst.format) {
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      if (mask & PIPE_MASK_S)
         color_mask |= 0x1000;
      /* fall through */
   case PIPE_FORMAT_Z24X8_UNORM:
      if (mask & PIPE_MASK_Z)
         color_mask |= 0x0111;
      break;
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
      if (mask & PIPE_MASK_Z)
         color_mask |= 0x1110;
      if (mask & PIPE_MASK_S)
         color_mask |= 0x0001;
      break;
   default:
      if (mask & (PIPE_MASK_R | PIPE_MASK_Z)) color_mask |= 0x0001;
      if (mask & (PIPE_MASK_G | PIPE_MASK_S)) color_mask |= 0x0010;
      if (mask & PIPE_MASK_B) color_mask |= 0x0100;
      if (mask & PIPE_MASK_A) color_mask |= 0x1000;
      break;
   }

   return color_mask;
}

static INLINE uint32_t
nv50_blit_eng2d_get_mask(const struct pipe_blit_info *info)
{
   uint32_t mask = 0;

   switch (info->dst.format) {
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
      if (info->mask & PIPE_MASK_Z) mask |= 0x00ffffff;
      if (info->mask & PIPE_MASK_S) mask |= 0xff000000;
      break;
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
      if (info->mask & PIPE_MASK_Z) mask |= 0xffffff00;
      if (info->mask & PIPE_MASK_S) mask |= 0x000000ff;
      break;
   case PIPE_FORMAT_X8Z24_UNORM:
      if (info->mask & PIPE_MASK_Z) mask = 0x00ffffff;
      break;
   default:
      mask = 0xffffffff;
      break;
   }
   return mask;
}

#if NOUVEAU_DRIVER == 0xc0
# define nv50_format_table nvc0_format_table
#endif

/* return TRUE for formats that can be converted among each other by NVC0_2D */
static INLINE boolean
nv50_2d_dst_format_faithful(enum pipe_format format)
{
   const uint64_t mask =
       NV50_ENG2D_SUPPORTED_FORMATS &
      ~NV50_ENG2D_NOCONVERT_FORMATS;
   uint8_t id = nv50_format_table[format].rt;
   return (id >= 0xc0) && (mask & (1ULL << (id - 0xc0)));
}
static INLINE boolean
nv50_2d_src_format_faithful(enum pipe_format format)
{
   const uint64_t mask =
      NV50_ENG2D_SUPPORTED_FORMATS &
    ~(NV50_ENG2D_LUMINANCE_FORMATS | NV50_ENG2D_INTENSITY_FORMATS);
   uint8_t id = nv50_format_table[format].rt;
   return (id >= 0xc0) && (mask & (1ULL << (id - 0xc0)));
}

static INLINE boolean
nv50_2d_format_supported(enum pipe_format format)
{
   uint8_t id = nv50_format_table[format].rt;
   return (id >= 0xc0) &&
      (NV50_ENG2D_SUPPORTED_FORMATS & (1ULL << (id - 0xc0)));
}

static INLINE boolean
nv50_2d_dst_format_ops_supported(enum pipe_format format)
{
   uint8_t id = nv50_format_table[format].rt;
   return (id >= 0xc0) &&
      (NV50_ENG2D_OPERATION_FORMATS & (1ULL << (id - 0xc0)));
}

#endif /* __NV50_BLIT_H__ */
