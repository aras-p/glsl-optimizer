#include "xorg_composite.h"

#include "xorg_exa_tgsi.h"

#include "cso_cache/cso_context.h"
#include "util/u_draw_quad.h"

#include "pipe/p_inlines.h"

struct xorg_composite_blend {
   int op:8;

   unsigned rgb_src_factor:5;    /**< PIPE_BLENDFACTOR_x */
   unsigned rgb_dst_factor:5;    /**< PIPE_BLENDFACTOR_x */

   unsigned alpha_src_factor:5;  /**< PIPE_BLENDFACTOR_x */
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
pixel_to_float4(PictFormatPtr format,
                CARD32 pixel, float *color)
{
   CARD32	    r, g, b, a;

   debug_assert(format->type == PictTypeDirect);

   r = (pixel >> format->direct.red) & format->direct.redMask;
   g = (pixel >> format->direct.green) & format->direct.greenMask;
   b = (pixel >> format->direct.blue) & format->direct.blueMask;
   a = (pixel >> format->direct.alpha) & format->direct.alphaMask;
   color[0] = ((float)r) / ((float)format->direct.redMask);
   color[1] = ((float)g) / ((float)format->direct.greenMask);
   color[2] = ((float)b) / ((float)format->direct.blueMask);
   color[3] = ((float)a) / ((float)format->direct.alphaMask);
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


static INLINE void
setup_vertex0(float vertex[2][4], float x, float y,
              float color[4])
{
   vertex[0][0] = x;
   vertex[0][1] = y;
   vertex[0][2] = 0.f; /*z*/
   vertex[0][3] = 1.f; /*w*/

   vertex[1][0] = color[0]; /*r*/
   vertex[1][1] = color[1]; /*g*/
   vertex[1][2] = color[2]; /*b*/
   vertex[1][3] = color[3]; /*a*/
}

static struct pipe_buffer *
setup_vertex_data0(struct exa_context *ctx,
                   int srcX, int srcY, int maskX, int maskY,
                   int dstX, int dstY, int width, int height)
{
   float vertices[4][2][4];

   /* 1st vertex */
   setup_vertex0(vertices[0], dstX, dstY,
                 ctx->solid_color);
   /* 2nd vertex */
   setup_vertex0(vertices[1], dstX + width, dstY,
                 ctx->solid_color);
   /* 3rd vertex */
   setup_vertex0(vertices[2], dstX + width, dstY + height,
                 ctx->solid_color);
   /* 4th vertex */
   setup_vertex0(vertices[3], dstX, dstY + height,
                 ctx->solid_color);

   return pipe_user_buffer_create(ctx->ctx->screen,
                                  vertices,
                                  sizeof(vertices));
}

static INLINE void
setup_vertex1(float vertex[2][4], float x, float y, float s, float t)
{
   vertex[0][0] = x;
   vertex[0][1] = y;
   vertex[0][2] = 0.f; /*z*/
   vertex[0][3] = 1.f; /*w*/

   vertex[1][0] = s;   /*s*/
   vertex[1][1] = t;   /*t*/
   vertex[1][2] = 0.f; /*r*/
   vertex[1][3] = 1.f; /*q*/
}

static struct pipe_buffer *
setup_vertex_data1(struct exa_context *ctx,
                   int srcX, int srcY, int maskX, int maskY,
                   int dstX, int dstY, int width, int height)
{
   float vertices[4][2][4];
   float s0, t0, s1, t1;
   struct pipe_texture *src = ctx->bound_textures[0];

   s0 = srcX / src->width[0];
   s1 = srcX + width / src->width[0];
   t0 = srcY / src->height[0];
   t1 = srcY + height / src->height[0];

   /* 1st vertex */
   setup_vertex1(vertices[0], dstX, dstY,
                 s0, t0);
   /* 2nd vertex */
   setup_vertex1(vertices[1], dstX + width, dstY,
                 s1, t0);
   /* 3rd vertex */
   setup_vertex1(vertices[2], dstX + width, dstY + height,
                 s1, t1);
   /* 4th vertex */
   setup_vertex1(vertices[3], dstX, dstY + height,
                 s0, t1);

   return pipe_user_buffer_create(ctx->ctx->screen,
                                  vertices,
                                  sizeof(vertices));
}


static INLINE void
setup_vertex2(float vertex[3][4], float x, float y,
              float s0, float t0, float s1, float t1)
{
   vertex[0][0] = x;
   vertex[0][1] = y;
   vertex[0][2] = 0.f; /*z*/
   vertex[0][3] = 1.f; /*w*/

   vertex[1][0] = s0;  /*s*/
   vertex[1][1] = t0;  /*t*/
   vertex[1][2] = 0.f; /*r*/
   vertex[1][3] = 1.f; /*q*/

   vertex[2][0] = s1;  /*s*/
   vertex[2][1] = t1;  /*t*/
   vertex[2][2] = 0.f; /*r*/
   vertex[2][3] = 1.f; /*q*/
}

static struct pipe_buffer *
setup_vertex_data2(struct exa_context *ctx,
                   int srcX, int srcY, int maskX, int maskY,
                   int dstX, int dstY, int width, int height)
{
   float vertices[4][3][4];
   float st0[4], st1[4];
   struct pipe_texture *src = ctx->bound_textures[0];
   struct pipe_texture *mask = ctx->bound_textures[0];

   st0[0] = srcX / src->width[0];
   st0[1] = srcY / src->height[0];
   st0[2] = srcX + width / src->width[0];
   st0[3] = srcY + height / src->height[0];

   st1[0] = maskX / mask->width[0];
   st1[1] = maskY / mask->height[0];
   st1[2] = maskX + width / mask->width[0];
   st1[3] = maskY + height / mask->height[0];

   /* 1st vertex */
   setup_vertex2(vertices[0], dstX, dstY,
                 st0[0], st0[1], st1[0], st1[1]);
   /* 2nd vertex */
   setup_vertex2(vertices[1], dstX + width, dstY,
                 st0[2], st0[1], st1[2], st1[1]);
   /* 3rd vertex */
   setup_vertex2(vertices[2], dstX + width, dstY + height,
                 st0[2], st0[3], st1[2], st1[3]);
   /* 4th vertex */
   setup_vertex2(vertices[3], dstX, dstY + height,
                 st0[0], st0[3], st1[0], st1[3]);

   return pipe_user_buffer_create(ctx->ctx->screen,
                                  vertices,
                                  sizeof(vertices));
}

boolean xorg_composite_accelerated(int op,
                                   PicturePtr pSrcPicture,
                                   PicturePtr pMaskPicture,
                                   PicturePtr pDstPicture)
{
   unsigned i;
   unsigned accel_ops_count =
      sizeof(accelerated_ops)/sizeof(struct acceleration_info);


   /*FIXME: currently accel is disabled */
   return FALSE;

   if (pSrcPicture) {
      /* component alpha not supported */
      if (pSrcPicture->componentAlpha)
         return FALSE;
      /* fills not supported */
      if (pSrcPicture->pSourcePict)
         return FALSE;
   }

   for (i = 0; i < accel_ops_count; ++i) {
      if (op == accelerated_ops[i].op) {
         if (pMaskPicture && !accelerated_ops[i].with_mask)
            return FALSE;
         return TRUE;
      }
   }
   return FALSE;
}

static void
bind_framebuffer_state(struct exa_context *exa, PicturePtr pDstPicture,
                       struct exa_pixmap_priv *pDst)
{
   unsigned i;
   struct pipe_framebuffer_state state;
   struct pipe_surface *surface = exa_gpu_surface(exa, pDst);
   memset(&state, 0, sizeof(struct pipe_framebuffer_state));

   state.width  = pDstPicture->pDrawable->width;
   state.height = pDstPicture->pDrawable->height;

   state.nr_cbufs = 1;
   state.cbufs[0] = surface;
   for (i = 1; i < PIPE_MAX_COLOR_BUFS; ++i)
      state.cbufs[i] = 0;

   /* currently we don't use depth/stencil */
   state.zsbuf = 0;

   cso_set_framebuffer(exa->cso, &state);
}

enum AxisOrientation {
   Y0_BOTTOM,
   Y0_TOP
};

static void
set_viewport(struct exa_context *exa, int width, int height,
             enum AxisOrientation orientation)
{
   struct pipe_viewport_state viewport;
   float y_scale = (orientation == Y0_BOTTOM) ? -2.f : 2.f;

   viewport.scale[0] =  width / 2.f;
   viewport.scale[1] =  height / y_scale;
   viewport.scale[2] =  1.0;
   viewport.scale[3] =  1.0;
   viewport.translate[0] = width / 2.f;
   viewport.translate[1] = height / 2.f;
   viewport.translate[2] = 0.0;
   viewport.translate[3] = 0.0;

   cso_set_viewport(exa->cso, &viewport);
}

static void
bind_viewport_state(struct exa_context *exa, PicturePtr pDstPicture)
{
   int width = pDstPicture->pDrawable->width;
   int height = pDstPicture->pDrawable->height;

   set_viewport(exa, width, height, Y0_TOP);
}

static void
bind_blend_state(struct exa_context *exa, int op,
                 PicturePtr pSrcPicture, PicturePtr pMaskPicture)
{
   boolean component_alpha = pSrcPicture->componentAlpha;
   struct xorg_composite_blend blend_opt;
   struct pipe_blend_state blend;

   if (component_alpha) {
      op = PictOpOver;
   }
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

   cso_set_blend(exa->cso, &blend);
}

static void
bind_rasterizer_state(struct exa_context *exa)
{
   struct pipe_rasterizer_state raster;
   memset(&raster, 0, sizeof(struct pipe_rasterizer_state));
   raster.gl_rasterization_rules = 1;
   cso_set_rasterizer(exa->cso, &raster);
}

static void
bind_shaders(struct exa_context *exa, int op,
             PicturePtr pSrcPicture, PicturePtr pMaskPicture)
{
   unsigned vs_traits = 0, fs_traits = 0;
   struct xorg_shader shader;

   if (pSrcPicture) {
      if (pSrcPicture->pSourcePict) {
         if (pSrcPicture->pSourcePict->type == SourcePictTypeSolidFill) {
            fs_traits |= FS_SOLID_FILL;
            vs_traits |= VS_SOLID_FILL;
            pixel_to_float4(pSrcPicture->pFormat,
                            pSrcPicture->pSourcePict->solidFill.color,
                            exa->solid_color);
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

   shader = xorg_shaders_get(exa->shaders, vs_traits, fs_traits);
   cso_set_vertex_shader_handle(exa->cso, shader.vs);
   cso_set_fragment_shader_handle(exa->cso, shader.fs);
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

   cso_set_samplers(exa->cso, exa->num_bound_samplers,
                    (const struct pipe_sampler_state **)samplers);
   cso_set_sampler_textures(exa->cso, exa->num_bound_samplers,
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
   struct pipe_constant_buffer *cbuf = &exa->vs_const_buffer;

   pipe_buffer_reference(&cbuf->buffer, NULL);
   cbuf->buffer = pipe_buffer_create(exa->ctx->screen, 16,
                                     PIPE_BUFFER_USAGE_CONSTANT,
                                     param_bytes);

   if (cbuf->buffer) {
      pipe_buffer_write(exa->ctx->screen, cbuf->buffer,
                        0, param_bytes, vs_consts);
   }
   exa->ctx->set_constant_buffer(exa->ctx, PIPE_SHADER_VERTEX, 0, cbuf);
}


static void
setup_fs_constant_buffer(struct exa_context *exa)
{
   const int param_bytes = 4 * sizeof(float);
   float fs_consts[8] = {
      0, 0, 0, 1,
   };
   struct pipe_constant_buffer *cbuf = &exa->fs_const_buffer;

   pipe_buffer_reference(&cbuf->buffer, NULL);
   cbuf->buffer = pipe_buffer_create(exa->ctx->screen, 16,
                                     PIPE_BUFFER_USAGE_CONSTANT,
                                     param_bytes);

   if (cbuf->buffer) {
      pipe_buffer_write(exa->ctx->screen, cbuf->buffer,
                        0, param_bytes, fs_consts);
   }
   exa->ctx->set_constant_buffer(exa->ctx, PIPE_SHADER_FRAGMENT, 0, cbuf);
}

static void
setup_constant_buffers(struct exa_context *exa, PicturePtr pDstPicture)
{
   int width = pDstPicture->pDrawable->width;
   int height = pDstPicture->pDrawable->height;

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
   bind_framebuffer_state(exa, pDstPicture, pDst);
   bind_viewport_state(exa, pDstPicture);
   bind_blend_state(exa, op, pSrcPicture, pMaskPicture);
   bind_rasterizer_state(exa);
   bind_shaders(exa, op, pSrcPicture, pMaskPicture);
   bind_samplers(exa, op, pSrcPicture, pMaskPicture,
                 pDstPicture, pSrc, pMask, pDst);

   setup_constant_buffers(exa, pDstPicture);

   return FALSE;
}

void xorg_composite(struct exa_context *exa,
                    struct exa_pixmap_priv *dst,
                    int srcX, int srcY, int maskX, int maskY,
                    int dstX, int dstY, int width, int height)
{
   struct pipe_context *pipe = exa->ctx;
   struct pipe_buffer *buf = 0;

   if (exa->num_bound_samplers == 0 ) { /* solid fill */
      buf = setup_vertex_data0(exa,
                               srcX, srcY, maskX, maskY,
                               dstX, dstY, width, height);
   } else if (exa->num_bound_samplers == 1 ) /* src */
      buf = setup_vertex_data1(exa,
                               srcX, srcY, maskX, maskY,
                               dstX, dstY, width, height);
   else if (exa->num_bound_samplers == 2) /* src + mask */
      buf = setup_vertex_data2(exa,
                               srcX, srcY, maskX, maskY,
                               dstX, dstY, width, height);
   else if (exa->num_bound_samplers == 3) { /* src + mask + dst */
      debug_assert(!"src/mask/dst not handled right now");
#if 0
      buf = setup_vertex_data2(exa,
                               srcX, srcY, maskX, maskY,
                               dstX, dstY, width, height);
#endif
   }

   if (buf) {
      util_draw_vertex_buffer(pipe, buf, 0,
                              PIPE_PRIM_TRIANGLE_FAN,
                              4,  /* verts */
                              1 + exa->num_bound_samplers); /* attribs/vert */

      pipe_buffer_reference(&buf, NULL);
   }
}

