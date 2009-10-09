#include "xorg_composite.h"

#include "xorg_renderer.h"
#include "xorg_exa_tgsi.h"

#include "cso_cache/cso_context.h"
#include "util/u_draw_quad.h"
#include "util/u_math.h"

#include "pipe/p_inlines.h"

struct xorg_composite_blend {
   int op:8;

   unsigned rgb_src_factor:5;    /**< PIPE_BLENDFACTOR_x */
   unsigned alpha_src_factor:5;  /**< PIPE_BLENDFACTOR_x */

   unsigned rgb_dst_factor:5;    /**< PIPE_BLENDFACTOR_x */
   unsigned alpha_dst_factor:5;  /**< PIPE_BLENDFACTOR_x */
};

#define BLEND_OP_OVER 3
static const struct xorg_composite_blend xorg_blends[] = {
   { PictOpClear,
     PIPE_BLENDFACTOR_CONST_COLOR, PIPE_BLENDFACTOR_CONST_ALPHA,
     PIPE_BLENDFACTOR_ZERO, PIPE_BLENDFACTOR_ZERO },

   { PictOpSrc,
     PIPE_BLENDFACTOR_ONE, PIPE_BLENDFACTOR_ONE,
     PIPE_BLENDFACTOR_ZERO, PIPE_BLENDFACTOR_ZERO },

   { PictOpDst,
     PIPE_BLENDFACTOR_ZERO, PIPE_BLENDFACTOR_ZERO,
     PIPE_BLENDFACTOR_ONE, PIPE_BLENDFACTOR_ONE },

   { PictOpOver,
     PIPE_BLENDFACTOR_SRC_ALPHA, PIPE_BLENDFACTOR_ONE,
     PIPE_BLENDFACTOR_INV_SRC_ALPHA, PIPE_BLENDFACTOR_INV_SRC_ALPHA },

   { PictOpOverReverse,
     PIPE_BLENDFACTOR_SRC_ALPHA, PIPE_BLENDFACTOR_ONE,
     PIPE_BLENDFACTOR_INV_SRC_ALPHA, PIPE_BLENDFACTOR_INV_SRC_ALPHA },
};


static INLINE void
pixel_to_float4(Pixel pixel, float *color)
{
   CARD32	    r, g, b, a;

   a = (pixel >> 24) & 0xff;
   r = (pixel >> 16) & 0xff;
   g = (pixel >>  8) & 0xff;
   b = (pixel >>  0) & 0xff;
   color[0] = ((float)r) / 255.;
   color[1] = ((float)g) / 255.;
   color[2] = ((float)b) / 255.;
   color[3] = ((float)a) / 255.;
}

struct acceleration_info {
   int op : 16;
   int with_mask : 1;
   int component_alpha : 1;
};
static const struct acceleration_info accelerated_ops[] = {
   {PictOpClear,       1, 0},
   {PictOpSrc,         1, 0},
   {PictOpDst,         1, 0},
   {PictOpOver,        1, 0},
   {PictOpOverReverse, 1, 0},
   {PictOpIn,          1, 0},
   {PictOpInReverse,   1, 0},
   {PictOpOut,         1, 0},
   {PictOpOutReverse,  1, 0},
   {PictOpAtop,        1, 0},
   {PictOpAtopReverse, 1, 0},
   {PictOpXor,         1, 0},
   {PictOpAdd,         1, 0},
   {PictOpSaturate,    1, 0},
};

static struct xorg_composite_blend
blend_for_op(int op)
{
   const int num_blends =
      sizeof(xorg_blends)/sizeof(struct xorg_composite_blend);
   int i;

   for (i = 0; i < num_blends; ++i) {
      if (xorg_blends[i].op == op)
         return xorg_blends[i];
   }
   return xorg_blends[BLEND_OP_OVER];
}

static INLINE int
render_repeat_to_gallium(int mode)
{
   switch(mode) {
   case RepeatNone:
      return PIPE_TEX_WRAP_CLAMP;
   case RepeatNormal:
      return PIPE_TEX_WRAP_REPEAT;
   case RepeatReflect:
      return PIPE_TEX_WRAP_MIRROR_REPEAT;
   case RepeatPad:
      return PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   default:
      debug_printf("Unsupported repeat mode\n");
   }
   return PIPE_TEX_WRAP_REPEAT;
}

boolean xorg_composite_accelerated(int op,
                                   PicturePtr pSrcPicture,
                                   PicturePtr pMaskPicture,
                                   PicturePtr pDstPicture)
{
   ScreenPtr pScreen = pDstPicture->pDrawable->pScreen;
   ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
   modesettingPtr ms = modesettingPTR(pScrn);
   unsigned i;
   unsigned accel_ops_count =
      sizeof(accelerated_ops)/sizeof(struct acceleration_info);

   if (pSrcPicture->pSourcePict) {
      /* Gradients not yet supported */
      if (pSrcPicture->pSourcePict->type != SourcePictTypeSolidFill)
         XORG_FALLBACK("gradients not yet supported");

      /* Solid source with mask not yet handled properly */
      if (pMaskPicture)
         XORG_FALLBACK("solid source with mask not yet handled properly");
   }

   for (i = 0; i < accel_ops_count; ++i) {
      if (op == accelerated_ops[i].op) {
         /* Check for unsupported component alpha */
         if ((pSrcPicture->componentAlpha &&
              !accelerated_ops[i].component_alpha) ||
             (pMaskPicture &&
              (!accelerated_ops[i].with_mask ||
               (pMaskPicture->componentAlpha &&
                !accelerated_ops[i].component_alpha))))
            XORG_FALLBACK("component alpha unsupported");
         return TRUE;
      }
   }
   XORG_FALLBACK("unsupported operation");
}

static void
bind_blend_state(struct exa_context *exa, int op,
                 PicturePtr pSrcPicture, PicturePtr pMaskPicture)
{
   struct xorg_composite_blend blend_opt;
   struct pipe_blend_state blend;

   blend_opt = blend_for_op(op);

   memset(&blend, 0, sizeof(struct pipe_blend_state));
   blend.blend_enable = 1;
   blend.colormask |= PIPE_MASK_R;
   blend.colormask |= PIPE_MASK_G;
   blend.colormask |= PIPE_MASK_B;
   blend.colormask |= PIPE_MASK_A;

   blend.rgb_src_factor   = blend_opt.rgb_src_factor;
   blend.alpha_src_factor = blend_opt.alpha_src_factor;
   blend.rgb_dst_factor   = blend_opt.rgb_dst_factor;
   blend.alpha_dst_factor = blend_opt.alpha_dst_factor;

   cso_set_blend(exa->renderer->cso, &blend);
}


static void
bind_shaders(struct exa_context *exa, int op,
             PicturePtr pSrcPicture, PicturePtr pMaskPicture)
{
   unsigned vs_traits = 0, fs_traits = 0;
   struct xorg_shader shader;

   exa->has_solid_color = FALSE;

   if (pSrcPicture) {
      if (pSrcPicture->pSourcePict) {
         if (pSrcPicture->pSourcePict->type == SourcePictTypeSolidFill) {
            fs_traits |= FS_SOLID_FILL;
            vs_traits |= VS_SOLID_FILL;
            debug_assert(pSrcPicture->format == PICT_a8r8g8b8);
            pixel_to_float4(pSrcPicture->pSourcePict->solidFill.color,
                            exa->solid_color);
            exa->has_solid_color = TRUE;
         } else {
            debug_assert("!gradients not supported");
         }
      } else {
         fs_traits |= FS_COMPOSITE;
         vs_traits |= VS_COMPOSITE;
      }
   }

   if (pMaskPicture) {
      vs_traits |= VS_MASK;
      fs_traits |= FS_MASK;
   }

   shader = xorg_shaders_get(exa->renderer->shaders, vs_traits, fs_traits);
   cso_set_vertex_shader_handle(exa->renderer->cso, shader.vs);
   cso_set_fragment_shader_handle(exa->renderer->cso, shader.fs);
}


static void
bind_samplers(struct exa_context *exa, int op,
              PicturePtr pSrcPicture, PicturePtr pMaskPicture,
              PicturePtr pDstPicture,
              struct exa_pixmap_priv *pSrc,
              struct exa_pixmap_priv *pMask,
              struct exa_pixmap_priv *pDst)
{
   struct pipe_sampler_state *samplers[PIPE_MAX_SAMPLERS];
   struct pipe_sampler_state src_sampler, mask_sampler;

   exa->num_bound_samplers = 0;

   memset(&src_sampler, 0, sizeof(struct pipe_sampler_state));
   memset(&mask_sampler, 0, sizeof(struct pipe_sampler_state));

   if ((pSrc && exa->pipe->is_texture_referenced(exa->pipe, pSrc->tex, 0, 0) &
        PIPE_REFERENCED_FOR_WRITE) ||
       (pMask && exa->pipe->is_texture_referenced(exa->pipe, pMask->tex, 0, 0) &
        PIPE_REFERENCED_FOR_WRITE))
      exa->pipe->flush(exa->pipe, PIPE_FLUSH_RENDER_CACHE, NULL);

   if (pSrcPicture && pSrc) {
      unsigned src_wrap = render_repeat_to_gallium(
         pSrcPicture->repeatType);
      src_sampler.wrap_s = src_wrap;
      src_sampler.wrap_t = src_wrap;
      src_sampler.min_img_filter = PIPE_TEX_MIPFILTER_NEAREST;
      src_sampler.mag_img_filter = PIPE_TEX_MIPFILTER_NEAREST;
      src_sampler.normalized_coords = 1;
      samplers[0] = &src_sampler;
      exa->bound_textures[0] = pSrc->tex;
      ++exa->num_bound_samplers;
   }

   if (pMaskPicture && pMask) {
      unsigned mask_wrap = render_repeat_to_gallium(
         pMaskPicture->repeatType);
      mask_sampler.wrap_s = mask_wrap;
      mask_sampler.wrap_t = mask_wrap;
      mask_sampler.min_img_filter = PIPE_TEX_MIPFILTER_NEAREST;
      mask_sampler.mag_img_filter = PIPE_TEX_MIPFILTER_NEAREST;
      mask_sampler.normalized_coords = 1;
      samplers[1] = &mask_sampler;
      exa->bound_textures[1] = pMask->tex;
      ++exa->num_bound_samplers;
   }

   cso_set_samplers(exa->renderer->cso, exa->num_bound_samplers,
                    (const struct pipe_sampler_state **)samplers);
   cso_set_sampler_textures(exa->renderer->cso, exa->num_bound_samplers,
                            exa->bound_textures);
}

static void
setup_vs_constant_buffer(struct exa_context *exa,
                         int width, int height)
{
   const int param_bytes = 8 * sizeof(float);
   float vs_consts[8] = {
      2.f/width, 2.f/height, 1, 1,
      -1, -1, 0, 0
   };
   renderer_set_constants(exa->renderer, PIPE_SHADER_VERTEX,
                          vs_consts, param_bytes);
}


static void
setup_fs_constant_buffer(struct exa_context *exa)
{
   const int param_bytes = 4 * sizeof(float);
   const float fs_consts[8] = {
      0, 0, 0, 1,
   };
   renderer_set_constants(exa->renderer, PIPE_SHADER_FRAGMENT,
                          fs_consts, param_bytes);
}

static void
setup_constant_buffers(struct exa_context *exa, struct exa_pixmap_priv *pDst)
{
   int width = pDst->tex->width[0];
   int height = pDst->tex->height[0];

   setup_vs_constant_buffer(exa, width, height);
   setup_fs_constant_buffer(exa);
}

boolean xorg_composite_bind_state(struct exa_context *exa,
                                  int op,
                                  PicturePtr pSrcPicture,
                                  PicturePtr pMaskPicture,
                                  PicturePtr pDstPicture,
                                  struct exa_pixmap_priv *pSrc,
                                  struct exa_pixmap_priv *pMask,
                                  struct exa_pixmap_priv *pDst)
{
   renderer_bind_framebuffer(exa->renderer, pDst);
   renderer_bind_viewport(exa->renderer, pDst);
   bind_blend_state(exa, op, pSrcPicture, pMaskPicture);
   renderer_bind_rasterizer(exa->renderer);
   bind_shaders(exa, op, pSrcPicture, pMaskPicture);
   bind_samplers(exa, op, pSrcPicture, pMaskPicture,
                 pDstPicture, pSrc, pMask, pDst);
   setup_constant_buffers(exa, pDst);

   return FALSE;
}

void xorg_composite(struct exa_context *exa,
                    struct exa_pixmap_priv *dst,
                    int srcX, int srcY, int maskX, int maskY,
                    int dstX, int dstY, int width, int height)
{
   if (exa->num_bound_samplers == 0 ) { /* solid fill */
      renderer_draw_solid_rect(exa->renderer,
                               dstX, dstY, dstX + width, dstY + height,
                               exa->solid_color);
   } else {
      int pos[6] = {srcX, srcY, maskX, maskY, dstX, dstY};
      renderer_draw_textures(exa->renderer,
                             pos, width, height,
                             exa->bound_textures,
                             exa->num_bound_samplers);
   }
}

boolean xorg_solid_bind_state(struct exa_context *exa,
                              struct exa_pixmap_priv *pixmap,
                              Pixel fg)
{
   unsigned vs_traits, fs_traits;
   struct xorg_shader shader;

   pixel_to_float4(fg, exa->solid_color);
   exa->has_solid_color = TRUE;

   exa->solid_color[3] = 1.f;

#if 0
   debug_printf("Color Pixel=(%d, %d, %d, %d), RGBA=(%f, %f, %f, %f)\n",
                (fg >> 24) & 0xff, (fg >> 16) & 0xff,
                (fg >> 8) & 0xff,  (fg >> 0) & 0xff,
                exa->solid_color[0], exa->solid_color[1],
                exa->solid_color[2], exa->solid_color[3]);
#endif

   vs_traits = VS_SOLID_FILL;
   fs_traits = FS_SOLID_FILL;

   renderer_bind_framebuffer(exa->renderer, pixmap);
   renderer_bind_viewport(exa->renderer, pixmap);
   renderer_bind_rasterizer(exa->renderer);
   bind_blend_state(exa, PictOpSrc, NULL, NULL);
   setup_constant_buffers(exa, pixmap);

   shader = xorg_shaders_get(exa->renderer->shaders, vs_traits, fs_traits);
   cso_set_vertex_shader_handle(exa->renderer->cso, shader.vs);
   cso_set_fragment_shader_handle(exa->renderer->cso, shader.fs);

   return TRUE;
}

void xorg_solid(struct exa_context *exa,
                struct exa_pixmap_priv *pixmap,
                int x0, int y0, int x1, int y1)
{
   renderer_draw_solid_rect(exa->renderer,
                            x0, y0, x1, y1, exa->solid_color);
}

