#include "xorg_composite.h"

#include <cso_cache/cso_context.h>

#include <pipe/p_inlines.h>

struct xorg_composite_blend {
   int op:8;

   unsigned rgb_src_factor:5;    /**< PIPE_BLENDFACTOR_x */
   unsigned rgb_dst_factor:5;    /**< PIPE_BLENDFACTOR_x */

   unsigned alpha_src_factor:5;  /**< PIPE_BLENDFACTOR_x */
   unsigned alpha_dst_factor:5;  /**< PIPE_BLENDFACTOR_x */
};

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

static void
draw_texture(struct exa_context *exa)
{
#if 0
   if (buf) {
      util_draw_vertex_buffer(pipe, buf, 0,
                              PIPE_PRIM_TRIANGLE_FAN,
                              4,  /* verts */
                              2); /* attribs/vert */

      pipe_buffer_reference(&buf, NULL);
   }
#endif
}

boolean xorg_composite_accelerated(int op,
                                   PicturePtr pSrcPicture,
                                   PicturePtr pMaskPicture,
                                   PicturePtr pDstPicture)
{
   unsigned i;
   unsigned accel_ops_count =
      sizeof(accelerated_ops)/sizeof(struct acceleration_info);

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
   const int param_bytes = 8 * sizeof(float);
   int width = pDstPicture->pDrawable->width;
   int height = pDstPicture->pDrawable->height;
   float vs_consts[8] = {
      2.f/width, 2.f/height, 1, 1,
      -1, -1, 0, 0
   };
   struct pipe_constant_buffer *cbuf = &exa->vs_const_buffer;

   set_viewport(exa, width, height, Y0_BOTTOM);

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
bind_blend_state()
{
}

static void
bind_rasterizer_state()
{
}

static void
bind_shaders()
{
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
   bind_blend_state();
   bind_rasterizer_state();
   bind_shaders();

   return FALSE;
}

void xorg_composite(struct exa_context *exa,
                    struct exa_pixmap_priv *dst,
                    int srcX, int srcY, int maskX, int maskY,
                    int dstX, int dstY, int width, int height)
{
}

