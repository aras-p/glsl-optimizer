#include "xorg_composite.h"

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

static INLINE void
render_pixel_to_float4(PictFormatPtr format,
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
   /* 1st vertex */
   setup_vertex0(ctx->vertices2[0], dstX, dstY,
                 ctx->solid_color);
   /* 2nd vertex */
   setup_vertex0(ctx->vertices2[1], dstX + width, dstY,
                 ctx->solid_color);
   /* 3rd vertex */
   setup_vertex0(ctx->vertices2[2], dstX + width, dstY + height,
                 ctx->solid_color);
   /* 4th vertex */
   setup_vertex0(ctx->vertices2[3], dstX, dstY + height,
                 ctx->solid_color);

   return pipe_user_buffer_create(ctx->pipe->screen,
                                  ctx->vertices2,
                                  sizeof(ctx->vertices2));
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
   float s0, t0, s1, t1;
   struct pipe_texture *src = ctx->bound_textures[0];

   s0 = srcX / src->width[0];
   s1 = srcX + width / src->width[0];
   t0 = srcY / src->height[0];
   t1 = srcY + height / src->height[0];

   /* 1st vertex */
   setup_vertex1(ctx->vertices2[0], dstX, dstY,
                 s0, t0);
   /* 2nd vertex */
   setup_vertex1(ctx->vertices2[1], dstX + width, dstY,
                 s1, t0);
   /* 3rd vertex */
   setup_vertex1(ctx->vertices2[2], dstX + width, dstY + height,
                 s1, t1);
   /* 4th vertex */
   setup_vertex1(ctx->vertices2[3], dstX, dstY + height,
                 s0, t1);

   return pipe_user_buffer_create(ctx->pipe->screen,
                                  ctx->vertices2,
                                  sizeof(ctx->vertices2));
}

static struct pipe_buffer *
setup_vertex_data_tex(struct exa_context *ctx,
                      float x0, float y0, float x1, float y1,
                      float s0, float t0, float s1, float t1,
                      float z)
{
   /* 1st vertex */
   setup_vertex1(ctx->vertices2[0], x0, y0,
                 s0, t0);
   /* 2nd vertex */
   setup_vertex1(ctx->vertices2[1], x1, y0,
                 s1, t0);
   /* 3rd vertex */
   setup_vertex1(ctx->vertices2[2], x1, y1,
                 s1, t1);
   /* 4th vertex */
   setup_vertex1(ctx->vertices2[3], x0, y1,
                 s0, t1);

   return pipe_user_buffer_create(ctx->pipe->screen,
                                  ctx->vertices2,
                                  sizeof(ctx->vertices2));
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
   setup_vertex2(ctx->vertices3[0], dstX, dstY,
                 st0[0], st0[1], st1[0], st1[1]);
   /* 2nd vertex */
   setup_vertex2(ctx->vertices3[1], dstX + width, dstY,
                 st0[2], st0[1], st1[2], st1[1]);
   /* 3rd vertex */
   setup_vertex2(ctx->vertices3[2], dstX + width, dstY + height,
                 st0[2], st0[3], st1[2], st1[3]);
   /* 4th vertex */
   setup_vertex2(ctx->vertices3[3], dstX, dstY + height,
                 st0[0], st0[3], st1[0], st1[3]);

   return pipe_user_buffer_create(ctx->pipe->screen,
                                  ctx->vertices3,
                                  sizeof(ctx->vertices3));
}

boolean xorg_composite_accelerated(int op,
                                   PicturePtr pSrcPicture,
                                   PicturePtr pMaskPicture,
                                   PicturePtr pDstPicture)
{
   unsigned i;
   unsigned accel_ops_count =
      sizeof(accelerated_ops)/sizeof(struct acceleration_info);

   for (i = 0; i < accel_ops_count; ++i) {
      if (op == accelerated_ops[i].op) {
         /* Check for unsupported component alpha */
         if ((pSrcPicture->componentAlpha &&
              !accelerated_ops[i].component_alpha) ||
             (pMaskPicture &&
              (!accelerated_ops[i].with_mask ||
               (pMaskPicture->componentAlpha &&
                !accelerated_ops[i].component_alpha))))
            return FALSE;
         return TRUE;
      }
   }
   return FALSE;
}

static void
bind_clip_state(struct exa_context *exa)
{
}

static void
bind_framebuffer_state(struct exa_context *exa, struct exa_pixmap_priv *pDst)
{
   unsigned i;
   struct pipe_framebuffer_state state;
   struct pipe_surface *surface = exa_gpu_surface(exa, pDst);
   memset(&state, 0, sizeof(struct pipe_framebuffer_state));

   state.width  = pDst->tex->width[0];
   state.height = pDst->tex->height[0];

   state.nr_cbufs = 1;
   state.cbufs[0] = surface;
   for (i = 1; i < PIPE_MAX_COLOR_BUFS; ++i)
      state.cbufs[i] = 0;

   /* currently we don't use depth/stencil */
   state.zsbuf = 0;

   cso_set_framebuffer(exa->cso, &state);

   /* we do fire and forget for the framebuffer, this is the forget part */
   pipe_surface_reference(&surface, NULL);
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
bind_viewport_state(struct exa_context *exa, struct exa_pixmap_priv *pDst)
{
   int width = pDst->tex->width[0];
   int height = pDst->tex->height[0];

   /*debug_printf("Bind viewport (%d, %d)\n", width, height);*/

   set_viewport(exa, width, height, Y0_TOP);
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

   exa->has_solid_color = FALSE;

   if (pSrcPicture) {
      if (pSrcPicture->pSourcePict) {
         if (pSrcPicture->pSourcePict->type == SourcePictTypeSolidFill) {
            fs_traits |= FS_SOLID_FILL;
            vs_traits |= VS_SOLID_FILL;
            render_pixel_to_float4(pSrcPicture->pFormat,
                                   pSrcPicture->pSourcePict->solidFill.color,
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
   cbuf->buffer = pipe_buffer_create(exa->pipe->screen, 16,
                                     PIPE_BUFFER_USAGE_CONSTANT,
                                     param_bytes);

   if (cbuf->buffer) {
      pipe_buffer_write(exa->pipe->screen, cbuf->buffer,
                        0, param_bytes, vs_consts);
   }
   exa->pipe->set_constant_buffer(exa->pipe, PIPE_SHADER_VERTEX, 0, cbuf);
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
   cbuf->buffer = pipe_buffer_create(exa->pipe->screen, 16,
                                     PIPE_BUFFER_USAGE_CONSTANT,
                                     param_bytes);

   if (cbuf->buffer) {
      pipe_buffer_write(exa->pipe->screen, cbuf->buffer,
                        0, param_bytes, fs_consts);
   }
   exa->pipe->set_constant_buffer(exa->pipe, PIPE_SHADER_FRAGMENT, 0, cbuf);
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
   bind_framebuffer_state(exa, pDst);
   bind_viewport_state(exa, pDst);
   bind_blend_state(exa, op, pSrcPicture, pMaskPicture);
   bind_rasterizer_state(exa);
   bind_shaders(exa, op, pSrcPicture, pMaskPicture);
   bind_samplers(exa, op, pSrcPicture, pMaskPicture,
                 pDstPicture, pSrc, pMask, pDst);
   bind_clip_state(exa);
   setup_constant_buffers(exa, pDst);

   return FALSE;
}

void xorg_composite(struct exa_context *exa,
                    struct exa_pixmap_priv *dst,
                    int srcX, int srcY, int maskX, int maskY,
                    int dstX, int dstY, int width, int height)
{
   struct pipe_context *pipe = exa->pipe;
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
      int num_attribs = 1; /*pos*/
      num_attribs += exa->num_bound_samplers;
      if (exa->has_solid_color)
         ++num_attribs;

      util_draw_vertex_buffer(pipe, buf, 0,
                              PIPE_PRIM_TRIANGLE_FAN,
                              4,  /* verts */
                              num_attribs); /* attribs/vert */

      pipe_buffer_reference(&buf, NULL);
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

   bind_framebuffer_state(exa, pixmap);
   bind_viewport_state(exa, pixmap);
   bind_rasterizer_state(exa);
   bind_blend_state(exa, PictOpSrc, NULL, NULL);
   setup_constant_buffers(exa, pixmap);
   bind_clip_state(exa);

   shader = xorg_shaders_get(exa->shaders, vs_traits, fs_traits);
   cso_set_vertex_shader_handle(exa->cso, shader.vs);
   cso_set_fragment_shader_handle(exa->cso, shader.fs);

   return TRUE;
}

void xorg_solid(struct exa_context *exa,
                struct exa_pixmap_priv *pixmap,
                int x0, int y0, int x1, int y1)
{
   struct pipe_context *pipe = exa->pipe;
   struct pipe_buffer *buf = 0;

   /* 1st vertex */
   setup_vertex0(exa->vertices2[0], x0, y0,
                 exa->solid_color);
   /* 2nd vertex */
   setup_vertex0(exa->vertices2[1], x1, y0,
                 exa->solid_color);
   /* 3rd vertex */
   setup_vertex0(exa->vertices2[2], x1, y1,
                 exa->solid_color);
   /* 4th vertex */
   setup_vertex0(exa->vertices2[3], x0, y1,
                 exa->solid_color);

   buf = pipe_user_buffer_create(exa->pipe->screen,
                                 exa->vertices2,
                                 sizeof(exa->vertices2));


   if (buf) {
      util_draw_vertex_buffer(pipe, buf, 0,
                              PIPE_PRIM_TRIANGLE_FAN,
                              4,  /* verts */
                              2); /* attribs/vert */

      pipe_buffer_reference(&buf, NULL);
   }
}


static INLINE void shift_rectx(float coords[4],
                               const float *bounds,
                               const float shift)
{
   coords[0] += shift;
   coords[2] -= shift;
   if (bounds) {
      coords[2] = MIN2(coords[2], bounds[2]);
      /* bound x/y + width/height */
      if ((coords[0] + coords[2]) > (bounds[0] + bounds[2])) {
         coords[2] = (bounds[0] + bounds[2]) - coords[0];
      }
   }
}

static INLINE void shift_recty(float coords[4],
                               const float *bounds,
                               const float shift)
{
   coords[1] += shift;
   coords[3] -= shift;
   if (bounds) {
      coords[3] = MIN2(coords[3], bounds[3]);
      if ((coords[1] + coords[3]) > (bounds[1] + bounds[3])) {
         coords[3] = (bounds[1] + bounds[3]) - coords[1];
      }
   }
}

static INLINE void bound_rect(float coords[4],
                              const float bounds[4],
                              float shift[4])
{
   /* if outside the bounds */
   if (coords[0] > (bounds[0] + bounds[2]) ||
       coords[1] > (bounds[1] + bounds[3]) ||
       (coords[0] + coords[2]) < bounds[0] ||
       (coords[1] + coords[3]) < bounds[1]) {
      coords[0] = 0.f;
      coords[1] = 0.f;
      coords[2] = 0.f;
      coords[3] = 0.f;
      shift[0] = 0.f;
      shift[1] = 0.f;
      return;
   }

   /* bound x */
   if (coords[0] < bounds[0]) {
      shift[0] = bounds[0] - coords[0];
      coords[2] -= shift[0];
      coords[0] = bounds[0];
   } else
      shift[0] = 0.f;

   /* bound y */
   if (coords[1] < bounds[1]) {
      shift[1] = bounds[1] - coords[1];
      coords[3] -= shift[1];
      coords[1] = bounds[1];
   } else
      shift[1] = 0.f;

   shift[2] = bounds[2] - coords[2];
   shift[3] = bounds[3] - coords[3];
   /* bound width/height */
   coords[2] = MIN2(coords[2], bounds[2]);
   coords[3] = MIN2(coords[3], bounds[3]);

   /* bound x/y + width/height */
   if ((coords[0] + coords[2]) > (bounds[0] + bounds[2])) {
      coords[2] = (bounds[0] + bounds[2]) - coords[0];
   }
   if ((coords[1] + coords[3]) > (bounds[1] + bounds[3])) {
      coords[3] = (bounds[1] + bounds[3]) - coords[1];
   }

   /* if outside the bounds */
   if ((coords[0] + coords[2]) < bounds[0] ||
       (coords[1] + coords[3]) < bounds[1]) {
      coords[0] = 0.f;
      coords[1] = 0.f;
      coords[2] = 0.f;
      coords[3] = 0.f;
      return;
   }
}

static INLINE void sync_size(float *src_loc, float *dst_loc)
{
   src_loc[2] = MIN2(src_loc[2], dst_loc[2]);
   src_loc[3] = MIN2(src_loc[3], dst_loc[3]);
   dst_loc[2] = src_loc[2];
   dst_loc[3] = src_loc[3];
}


static void renderer_copy_texture(struct exa_context *exa,
                                  struct pipe_texture *src,
                                  float sx1, float sy1,
                                  float sx2, float sy2,
                                  struct pipe_texture *dst,
                                  float dx1, float dy1,
                                  float dx2, float dy2)
{
   struct pipe_context *pipe = exa->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_buffer *buf;
   struct pipe_surface *dst_surf = screen->get_tex_surface(
      screen, dst, 0, 0, 0,
      PIPE_BUFFER_USAGE_GPU_WRITE);
   struct pipe_framebuffer_state fb;
   float s0, t0, s1, t1;
   struct xorg_shader shader;

   assert(src->width[0] != 0);
   assert(src->height[0] != 0);
   assert(dst->width[0] != 0);
   assert(dst->height[0] != 0);

#if 1
   s0 = sx1 / src->width[0];
   s1 = sx2 / src->width[0];
   t0 = sy1 / src->height[0];
   t1 = sy2 / src->height[0];
#else
   s0 = 0;
   s1 = 1;
   t0 = 0;
   t1 = 1;
#endif

#if 1
   debug_printf("copy texture src=[%f, %f, %f, %f], dst=[%f, %f, %f, %f], tex=[%f, %f, %f, %f]\n",
                sx1, sy1, sx2, sy2, dx1, dy1, dx2, dy2,
                s0, t0, s1, t1);
#endif

   assert(screen->is_format_supported(screen, dst_surf->format,
                                      PIPE_TEXTURE_2D,
                                      PIPE_TEXTURE_USAGE_RENDER_TARGET,
                                      0));

   /* save state (restored below) */
   cso_save_blend(exa->cso);
   cso_save_samplers(exa->cso);
   cso_save_sampler_textures(exa->cso);
   cso_save_framebuffer(exa->cso);
   cso_save_fragment_shader(exa->cso);
   cso_save_vertex_shader(exa->cso);

   cso_save_viewport(exa->cso);


   /* set misc state we care about */
   {
      struct pipe_blend_state blend;
      memset(&blend, 0, sizeof(blend));
      blend.rgb_src_factor = PIPE_BLENDFACTOR_ONE;
      blend.alpha_src_factor = PIPE_BLENDFACTOR_ONE;
      blend.rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
      blend.alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
      blend.colormask = PIPE_MASK_RGBA;
      cso_set_blend(exa->cso, &blend);
   }

   /* sampler */
   {
      struct pipe_sampler_state sampler;
      memset(&sampler, 0, sizeof(sampler));
      sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
      sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
      sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
      sampler.normalized_coords = 1;
      cso_single_sampler(exa->cso, 0, &sampler);
      cso_single_sampler_done(exa->cso);
   }

   set_viewport(exa, dst_surf->width, dst_surf->height, Y0_TOP);

   /* texture */
   cso_set_sampler_textures(exa->cso, 1, &src);

   /* shaders */
   shader = xorg_shaders_get(exa->shaders,
                             VS_COMPOSITE,
                             FS_COMPOSITE);
   cso_set_vertex_shader_handle(exa->cso, shader.vs);
   cso_set_fragment_shader_handle(exa->cso, shader.fs);

   /* drawing dest */
   memset(&fb, 0, sizeof(fb));
   fb.width = dst_surf->width;
   fb.height = dst_surf->height;
   fb.nr_cbufs = 1;
   fb.cbufs[0] = dst_surf;
   {
      int i;
      for (i = 1; i < PIPE_MAX_COLOR_BUFS; ++i)
         fb.cbufs[i] = 0;
   }
   cso_set_framebuffer(exa->cso, &fb);
   setup_vs_constant_buffer(exa, fb.width, fb.height);
   setup_fs_constant_buffer(exa);

   /* draw quad */
   buf = setup_vertex_data_tex(exa,
                               dx1, dy1,
                               dx2, dy2,
                               s0, t0, s1, t1,
                               0.0f);

   if (buf) {
      util_draw_vertex_buffer(exa->pipe, buf, 0,
                              PIPE_PRIM_TRIANGLE_FAN,
                              4,  /* verts */
                              2); /* attribs/vert */

      pipe_buffer_reference(&buf, NULL);
   }

   /* restore state we changed */
   cso_restore_blend(exa->cso);
   cso_restore_samplers(exa->cso);
   cso_restore_sampler_textures(exa->cso);
   cso_restore_framebuffer(exa->cso);
   cso_restore_vertex_shader(exa->cso);
   cso_restore_fragment_shader(exa->cso);
   cso_restore_viewport(exa->cso);

   pipe_surface_reference(&dst_surf, NULL);
}


static struct pipe_texture *
create_sampler_texture(struct exa_context *ctx,
                       struct pipe_texture *src)
{
   enum pipe_format format;
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_texture *pt;
   struct pipe_texture templ;

   pipe->flush(pipe, PIPE_FLUSH_RENDER_CACHE, NULL);

   /* the coming in texture should already have that invariance */
   debug_assert(screen->is_format_supported(screen, src->format,
                                            PIPE_TEXTURE_2D,
                                            PIPE_TEXTURE_USAGE_SAMPLER, 0));

   format = src->format;

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.format = format;
   templ.last_level = 0;
   templ.width[0] = src->width[0];
   templ.height[0] = src->height[0];
   templ.depth[0] = 1;
   pf_get_block(format, &templ.block);
   templ.tex_usage = PIPE_TEXTURE_USAGE_SAMPLER;

   pt = screen->texture_create(screen, &templ);

   debug_assert(!pt || pipe_is_referenced(&pt->reference));

   if (!pt)
      return NULL;

   {
      /* copy source framebuffer surface into texture */
      struct pipe_surface *ps_read = screen->get_tex_surface(
         screen, src, 0, 0, 0, PIPE_BUFFER_USAGE_GPU_READ);
      struct pipe_surface *ps_tex = screen->get_tex_surface(
         screen, pt, 0, 0, 0, PIPE_BUFFER_USAGE_GPU_WRITE );
      pipe->surface_copy(pipe,
			 ps_tex, /* dest */
			 0, 0, /* destx/y */
			 ps_read,
			 0, 0, src->width[0], src->height[0]);
      pipe_surface_reference(&ps_read, NULL);
      pipe_surface_reference(&ps_tex, NULL);
   }

   return pt;
}

void xorg_copy_pixmap(struct exa_context *ctx,
                      struct exa_pixmap_priv *dst_priv, int dx, int dy,
                      struct exa_pixmap_priv *src_priv, int sx, int sy,
                      int width, int height)
{
   float dst_loc[4], src_loc[4];
   float dst_bounds[4], src_bounds[4];
   float src_shift[4], dst_shift[4], shift[4];
   struct pipe_texture *dst = dst_priv->tex;
   struct pipe_texture *src = src_priv->tex;

   xorg_exa_finish(ctx);

   dst_loc[0] = dx;
   dst_loc[1] = dy;
   dst_loc[2] = width;
   dst_loc[3] = height;
   dst_bounds[0] = 0.f;
   dst_bounds[1] = 0.f;
   dst_bounds[2] = dst->width[0];
   dst_bounds[3] = dst->height[0];

   src_loc[0] = sx;
   src_loc[1] = sy;
   src_loc[2] = width;
   src_loc[3] = height;
   src_bounds[0] = 0.f;
   src_bounds[1] = 0.f;
   src_bounds[2] = src->width[0];
   src_bounds[3] = src->height[0];

   bound_rect(src_loc, src_bounds, src_shift);
   bound_rect(dst_loc, dst_bounds, dst_shift);
   shift[0] = src_shift[0] - dst_shift[0];
   shift[1] = src_shift[1] - dst_shift[1];

   if (shift[0] < 0)
      shift_rectx(src_loc, src_bounds, -shift[0]);
   else
      shift_rectx(dst_loc, dst_bounds, shift[0]);

   if (shift[1] < 0)
      shift_recty(src_loc, src_bounds, -shift[1]);
   else
      shift_recty(dst_loc, dst_bounds, shift[1]);

   sync_size(src_loc, dst_loc);

   if (src_loc[2] >= 0 && src_loc[3] >= 0 &&
       dst_loc[2] >= 0 && dst_loc[3] >= 0) {
      struct pipe_texture *temp_src = src;

      if (src == dst)
         temp_src = create_sampler_texture(ctx, src);

      renderer_copy_texture(ctx,
                            temp_src,
                            src_loc[0],
                            src_loc[1],
                            src_loc[0] + src_loc[2],
                            src_loc[1] + src_loc[3],
                            dst,
                            dst_loc[0],
                            dst_loc[1],
                            dst_loc[0] + dst_loc[2],
                            dst_loc[1] + dst_loc[3]);

      if (src == dst)
         pipe_texture_reference(&temp_src, NULL);
   }
}

