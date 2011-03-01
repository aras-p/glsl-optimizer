/*
 * Copyright 2010 Christoph Bumiller
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "util/u_format_s3tc.h"
#include "pipe/p_screen.h"

#include "nv50_context.h"
#include "nv50_screen.h"

#include "nouveau/nv_object.xml.h"

#ifndef NOUVEAU_GETPARAM_GRAPH_UNITS
# define NOUVEAU_GETPARAM_GRAPH_UNITS 13
#endif

extern int nouveau_device_get_param(struct nouveau_device *dev,
                                    uint64_t param, uint64_t *value);

static boolean
nv50_screen_is_format_supported(struct pipe_screen *pscreen,
                                enum pipe_format format,
                                enum pipe_texture_target target,
                                unsigned sample_count,
                                unsigned bindings, unsigned geom_flags)
{
   if (sample_count > 1)
      return FALSE;

   if (!util_format_s3tc_enabled) {
      switch (format) {
      case PIPE_FORMAT_DXT1_RGB:
      case PIPE_FORMAT_DXT1_RGBA:
      case PIPE_FORMAT_DXT3_RGBA:
      case PIPE_FORMAT_DXT5_RGBA:
         return FALSE;
      default:
         break;
      }
   }

   switch (format) {
   case PIPE_FORMAT_Z16_UNORM:
      if ((nouveau_screen(pscreen)->device->chipset & 0xf0) != 0xa0)
         return FALSE;
      break;
   default:
      break;
   }

   /* transfers & shared are always supported */
   bindings &= ~(PIPE_BIND_TRANSFER_READ |
                 PIPE_BIND_TRANSFER_WRITE |
                 PIPE_BIND_SHARED);

   return (nv50_format_table[format].usage & bindings) == bindings;
}

static int
nv50_screen_get_param(struct pipe_screen *pscreen, enum pipe_cap param)
{
   switch (param) {
   case PIPE_CAP_MAX_TEXTURE_IMAGE_UNITS:
   case PIPE_CAP_MAX_VERTEX_TEXTURE_UNITS:
      return 32;
   case PIPE_CAP_MAX_COMBINED_SAMPLERS:
      return 64;
   case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
      return 13;
   case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
      return 10;
   case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
      return 13;
   case PIPE_CAP_ARRAY_TEXTURES: /* shader support missing */
      return 0;
   case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
   case PIPE_CAP_TEXTURE_MIRROR_REPEAT:
   case PIPE_CAP_TEXTURE_SWIZZLE:
   case PIPE_CAP_TEXTURE_SHADOW_MAP:
   case PIPE_CAP_NPOT_TEXTURES:
   case PIPE_CAP_ANISOTROPIC_FILTER:
      return 1;
   case PIPE_CAP_TWO_SIDED_STENCIL:
   case PIPE_CAP_DEPTH_CLAMP:
   case PIPE_CAP_DEPTHSTENCIL_CLEAR_SEPARATE:
   case PIPE_CAP_POINT_SPRITE:
      return 1;
   case PIPE_CAP_GLSL:
   case PIPE_CAP_SM3:
      return 1;
   case PIPE_CAP_MAX_RENDER_TARGETS:
      return 8;
   case PIPE_CAP_TIMER_QUERY:
   case PIPE_CAP_OCCLUSION_QUERY:
      return 1;
   case PIPE_CAP_STREAM_OUTPUT:
      return 0;
   case PIPE_CAP_BLEND_EQUATION_SEPARATE:
   case PIPE_CAP_INDEP_BLEND_ENABLE:
   case PIPE_CAP_INDEP_BLEND_FUNC:
      return 1;
   case PIPE_CAP_TGSI_FS_COORD_ORIGIN_UPPER_LEFT:
   case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_HALF_INTEGER:
      return 1;
   case PIPE_CAP_TGSI_FS_COORD_ORIGIN_LOWER_LEFT:
   case PIPE_CAP_TGSI_FS_COORD_PIXEL_CENTER_INTEGER:
      return 0;
   case PIPE_CAP_SHADER_STENCIL_EXPORT:
      return 0;
   case PIPE_CAP_PRIMITIVE_RESTART:
   case PIPE_CAP_INSTANCED_DRAWING:
      return 1;
   default:
      NOUVEAU_ERR("unknown PIPE_CAP %d\n", param);
      return 0;
   }
}

static int
nv50_screen_get_shader_param(struct pipe_screen *pscreen, unsigned shader,
                             enum pipe_shader_cap param)
{
   switch (shader) {
   case PIPE_SHADER_VERTEX:
   case PIPE_SHADER_GEOMETRY:
   case PIPE_SHADER_FRAGMENT:
      break;
   default:
      return 0;
   }
   
   switch (param) {
   case PIPE_SHADER_CAP_MAX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_ALU_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INSTRUCTIONS:
   case PIPE_SHADER_CAP_MAX_TEX_INDIRECTIONS:
      return 16384;
   case PIPE_SHADER_CAP_MAX_CONTROL_FLOW_DEPTH:
      return 4;
   case PIPE_SHADER_CAP_MAX_INPUTS:
      if (shader == PIPE_SHADER_VERTEX)
         return 32;
      return 0x300 / 16;
   case PIPE_SHADER_CAP_MAX_CONSTS:
      return 65536 / 16;
   case PIPE_SHADER_CAP_MAX_CONST_BUFFERS:
      return 14;
   case PIPE_SHADER_CAP_MAX_ADDRS:
      return 1;
   case PIPE_SHADER_CAP_INDIRECT_INPUT_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_OUTPUT_ADDR:
      return shader != PIPE_SHADER_FRAGMENT;
   case PIPE_SHADER_CAP_INDIRECT_TEMP_ADDR:
   case PIPE_SHADER_CAP_INDIRECT_CONST_ADDR:
      return 1;
   case PIPE_SHADER_CAP_MAX_PREDS:
      return 0;
   case PIPE_SHADER_CAP_MAX_TEMPS:
      return NV50_CAP_MAX_PROGRAM_TEMPS;
   case PIPE_SHADER_CAP_TGSI_CONT_SUPPORTED:
      return 1;
   case PIPE_SHADER_CAP_SUBROUTINES:
      return 0; /* please inline, or provide function declarations */
   default:
      NOUVEAU_ERR("unknown PIPE_SHADER_CAP %d\n", param);
      return 0;
   }
}

static float
nv50_screen_get_paramf(struct pipe_screen *pscreen, enum pipe_cap param)
{
   switch (param) {
   case PIPE_CAP_MAX_LINE_WIDTH:
   case PIPE_CAP_MAX_LINE_WIDTH_AA:
      return 10.0f;
   case PIPE_CAP_MAX_POINT_WIDTH:
   case PIPE_CAP_MAX_POINT_WIDTH_AA:
      return 64.0f;
   case PIPE_CAP_MAX_TEXTURE_ANISOTROPY:
      return 16.0f;
   case PIPE_CAP_MAX_TEXTURE_LOD_BIAS:
      return 4.0f;
   default:
      NOUVEAU_ERR("unknown PIPE_CAP %d\n", param);
      return 0.0f;
   }
}

static void
nv50_screen_destroy(struct pipe_screen *pscreen)
{
   struct nv50_screen *screen = nv50_screen(pscreen);

   if (screen->base.fence.current) {
      nouveau_fence_wait(screen->base.fence.current);
      nouveau_fence_ref (NULL, &screen->base.fence.current);
   }

   nouveau_bo_ref(NULL, &screen->code);
   nouveau_bo_ref(NULL, &screen->tls_bo);
   nouveau_bo_ref(NULL, &screen->stack_bo);
   nouveau_bo_ref(NULL, &screen->txc);
   nouveau_bo_ref(NULL, &screen->uniforms);
   nouveau_bo_ref(NULL, &screen->fence.bo);

   nouveau_resource_destroy(&screen->vp_code_heap);
   nouveau_resource_destroy(&screen->gp_code_heap);
   nouveau_resource_destroy(&screen->fp_code_heap);

   if (screen->tic.entries)
      FREE(screen->tic.entries);

   nouveau_mm_destroy(screen->mm_VRAM_fe0);

   nouveau_grobj_free(&screen->tesla);
   nouveau_grobj_free(&screen->eng2d);
   nouveau_grobj_free(&screen->m2mf);

   nouveau_notifier_free(&screen->sync);

   nouveau_screen_fini(&screen->base);

   FREE(screen);
}

static void
nv50_screen_fence_emit(struct pipe_screen *pscreen, u32 sequence)
{
   struct nv50_screen *screen = nv50_screen(pscreen);
   struct nouveau_channel *chan = screen->base.channel;

   MARK_RING (chan, 5, 2);
   BEGIN_RING(chan, RING_3D(QUERY_ADDRESS_HIGH), 4);
   OUT_RELOCh(chan, screen->fence.bo, 0, NOUVEAU_BO_WR);
   OUT_RELOCl(chan, screen->fence.bo, 0, NOUVEAU_BO_WR);
   OUT_RING  (chan, sequence);
   OUT_RING  (chan, NV50_3D_QUERY_GET_MODE_WRITE_UNK0 |
                    NV50_3D_QUERY_GET_UNK4 |
                    NV50_3D_QUERY_GET_UNIT_CROP |
                    NV50_3D_QUERY_GET_TYPE_QUERY |
                    NV50_3D_QUERY_GET_QUERY_SELECT_ZERO |
                    NV50_3D_QUERY_GET_SHORT);
}

static u32
nv50_screen_fence_update(struct pipe_screen *pscreen)
{
   struct nv50_screen *screen = nv50_screen(pscreen);
   return screen->fence.map[0];
}

#define FAIL_SCREEN_INIT(str, err)                    \
   do {                                               \
      NOUVEAU_ERR(str, err);                          \
      nv50_screen_destroy(pscreen);                   \
      return NULL;                                    \
   } while(0)

struct pipe_screen *
nv50_screen_create(struct pipe_winsys *ws, struct nouveau_device *dev)
{
   struct nv50_screen *screen;
   struct nouveau_channel *chan;
   struct pipe_screen *pscreen;
   uint64_t value;
   uint32_t tesla_class;
   unsigned stack_size, max_warps, tls_space;
   int ret;
   unsigned i;

   screen = CALLOC_STRUCT(nv50_screen);
   if (!screen)
      return NULL;
   pscreen = &screen->base.base;

   ret = nouveau_screen_init(&screen->base, dev);
   if (ret)
      FAIL_SCREEN_INIT("nouveau_screen_init failed: %d\n", ret);

   chan = screen->base.channel;

   pscreen->winsys = ws;
   pscreen->destroy = nv50_screen_destroy;
   pscreen->context_create = nv50_create;
   pscreen->is_format_supported = nv50_screen_is_format_supported;
   pscreen->get_param = nv50_screen_get_param;
   pscreen->get_shader_param = nv50_screen_get_shader_param;
   pscreen->get_paramf = nv50_screen_get_paramf;

   nv50_screen_init_resource_functions(pscreen);

   screen->base.vertex_buffer_flags = screen->base.index_buffer_flags =
      NOUVEAU_BO_GART;
   screen->base.copy_data = nv50_m2mf_copy_linear;
   screen->base.push_data = nv50_sifc_linear_u8;

   ret = nouveau_bo_new(dev, NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 0, 4096,
                        &screen->fence.bo);
   if (ret)
      goto fail;
   nouveau_bo_map(screen->fence.bo, NOUVEAU_BO_RDWR);
   screen->fence.map = screen->fence.bo->map;
   nouveau_bo_unmap(screen->fence.bo);
   screen->base.fence.emit = nv50_screen_fence_emit;
   screen->base.fence.update = nv50_screen_fence_update;

   ret = nouveau_notifier_alloc(chan, 0xbeef0301, 1, &screen->sync);
   if (ret)
      FAIL_SCREEN_INIT("Error allocating notifier: %d\n", ret);

   ret = nouveau_grobj_alloc(chan, 0xbeef5039, NV50_M2MF, &screen->m2mf);
   if (ret)
      FAIL_SCREEN_INIT("Error allocating PGRAPH context for M2MF: %d\n", ret);

   BIND_RING (chan, screen->m2mf, NV50_SUBCH_MF);
   BEGIN_RING(chan, RING_MF_(NV04_M2MF_DMA_NOTIFY), 3);
   OUT_RING  (chan, screen->sync->handle);
   OUT_RING  (chan, chan->vram->handle);
   OUT_RING  (chan, chan->vram->handle);

   ret = nouveau_grobj_alloc(chan, 0xbeef502d, NV50_2D, &screen->eng2d);
   if (ret)
      FAIL_SCREEN_INIT("Error allocating PGRAPH context for 2D: %d\n", ret);

   BIND_RING (chan, screen->eng2d, NV50_SUBCH_2D);
   BEGIN_RING(chan, RING_2D(DMA_NOTIFY), 4);
   OUT_RING  (chan, screen->sync->handle);
   OUT_RING  (chan, chan->vram->handle);
   OUT_RING  (chan, chan->vram->handle);
   OUT_RING  (chan, chan->vram->handle);
   BEGIN_RING(chan, RING_2D(OPERATION), 1);
   OUT_RING  (chan, NV50_2D_OPERATION_SRCCOPY);
   BEGIN_RING(chan, RING_2D(CLIP_ENABLE), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_2D(COLOR_KEY_ENABLE), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_2D_(0x0888), 1);
   OUT_RING  (chan, 1);

   switch (dev->chipset & 0xf0) {
   case 0x50:
      tesla_class = NV50_3D;
      break;
   case 0x80:
   case 0x90:
      tesla_class = NV84_3D;
      break;
   case 0xa0:
      switch (dev->chipset) {
      case 0xa0:
      case 0xaa:
      case 0xac:
         tesla_class = NVA0_3D;
         break;
      case 0xaf:
         tesla_class = NVAF_3D;
         break;
      default:
         tesla_class = NVA3_3D;
         break;
      }
      break;
   default:
      FAIL_SCREEN_INIT("Not a known NV50 chipset: NV%02x\n", dev->chipset);
      break;
   }

   ret = nouveau_grobj_alloc(chan, 0xbeef5097, tesla_class, &screen->tesla);
   if (ret)
      FAIL_SCREEN_INIT("Error allocating PGRAPH context for 3D: %d\n", ret);

   BIND_RING (chan, screen->tesla, NV50_SUBCH_3D);

   BEGIN_RING(chan, RING_3D(COND_MODE), 1);
   OUT_RING  (chan, NV50_3D_COND_MODE_ALWAYS);

   BEGIN_RING(chan, RING_3D(DMA_NOTIFY), 1);
   OUT_RING  (chan, screen->sync->handle);
   BEGIN_RING(chan, RING_3D(DMA_ZETA), 11);
   for (i = 0; i < 11; ++i)
      OUT_RING(chan, chan->vram->handle);
   BEGIN_RING(chan, RING_3D(DMA_COLOR(0)), NV50_3D_DMA_COLOR__LEN);
   for (i = 0; i < NV50_3D_DMA_COLOR__LEN; ++i)
      OUT_RING(chan, chan->vram->handle);

   BEGIN_RING(chan, RING_3D(REG_MODE), 1);
   OUT_RING  (chan, NV50_3D_REG_MODE_STRIPED);
   BEGIN_RING(chan, RING_3D(UNK1400_LANES), 1);
   OUT_RING  (chan, 0xf);

   BEGIN_RING(chan, RING_3D(RT_CONTROL), 1);
   OUT_RING  (chan, 1);

   BEGIN_RING(chan, RING_3D(CSAA_ENABLE), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D(MULTISAMPLE_ENABLE), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D(MULTISAMPLE_MODE), 1);
   OUT_RING  (chan, NV50_3D_MULTISAMPLE_MODE_MS1);
   BEGIN_RING(chan, RING_3D(MULTISAMPLE_CTRL), 1);
   OUT_RING  (chan, 0);

   BEGIN_RING(chan, RING_3D(SCREEN_Y_CONTROL), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D(WINDOW_OFFSET_X), 2);
   OUT_RING  (chan, 0);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D(ZCULL_REGION), 1); /* deactivate ZCULL */
   OUT_RING  (chan, 0x3f);

   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 16, 3 << 16, &screen->code);
   if (ret)
      goto fail;

   nouveau_resource_init(&screen->vp_code_heap, 0, 1 << 16);
   nouveau_resource_init(&screen->gp_code_heap, 0, 1 << 16);
   nouveau_resource_init(&screen->fp_code_heap, 0, 1 << 16);

   BEGIN_RING(chan, RING_3D(VP_ADDRESS_HIGH), 2);
   OUT_RELOCh(chan, screen->code, 0 << 16, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   OUT_RELOCl(chan, screen->code, 0 << 16, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);

   BEGIN_RING(chan, RING_3D(FP_ADDRESS_HIGH), 2);
   OUT_RELOCh(chan, screen->code, 1 << 16, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   OUT_RELOCl(chan, screen->code, 1 << 16, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);

   BEGIN_RING(chan, RING_3D(GP_ADDRESS_HIGH), 2);
   OUT_RELOCh(chan, screen->code, 2 << 16, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   OUT_RELOCl(chan, screen->code, 2 << 16, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);

   nouveau_device_get_param(dev, NOUVEAU_GETPARAM_GRAPH_UNITS, &value);

   max_warps  = util_bitcount(value & 0xffff);
   max_warps *= util_bitcount((value >> 24) & 0xf) * 32;

   stack_size = max_warps * 64 * 8;

   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 16, stack_size,
                        &screen->stack_bo);
   if (ret)
      FAIL_SCREEN_INIT("Failed to allocate stack bo: %d\n", ret);

   BEGIN_RING(chan, RING_3D(STACK_ADDRESS_HIGH), 3);
   OUT_RELOCh(chan, screen->stack_bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
   OUT_RELOCl(chan, screen->stack_bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
   OUT_RING  (chan, 4);

   tls_space = NV50_CAP_MAX_PROGRAM_TEMPS * 16;

   screen->tls_size = tls_space * max_warps * 32;

   debug_printf("max_warps = %i, tls_size = %lu KiB\n",
                max_warps, screen->tls_size >> 10);

   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 16, screen->tls_size,
                        &screen->tls_bo);
   if (ret)
      FAIL_SCREEN_INIT("Failed to allocate stack bo: %d\n", ret);

   BEGIN_RING(chan, RING_3D(LOCAL_ADDRESS_HIGH), 3);
   OUT_RELOCh(chan, screen->tls_bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
   OUT_RELOCl(chan, screen->tls_bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
   OUT_RING  (chan, util_unsigned_logbase2(tls_space / 8));

   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 16, 4 << 16,
                        &screen->uniforms);
   if (ret)
      goto fail;

   BEGIN_RING(chan, RING_3D(CB_DEF_ADDRESS_HIGH), 3);
   OUT_RELOCh(chan, screen->uniforms, 0 << 16, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   OUT_RELOCl(chan, screen->uniforms, 0 << 16, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   OUT_RING  (chan, (NV50_CB_PVP << 16) | 0x0000);

   BEGIN_RING(chan, RING_3D(CB_DEF_ADDRESS_HIGH), 3);
   OUT_RELOCh(chan, screen->uniforms, 1 << 16, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   OUT_RELOCl(chan, screen->uniforms, 1 << 16, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   OUT_RING  (chan, (NV50_CB_PGP << 16) | 0x0000);

   BEGIN_RING(chan, RING_3D(CB_DEF_ADDRESS_HIGH), 3);
   OUT_RELOCh(chan, screen->uniforms, 2 << 16, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   OUT_RELOCl(chan, screen->uniforms, 2 << 16, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   OUT_RING  (chan, (NV50_CB_PFP << 16) | 0x0000);

   BEGIN_RING(chan, RING_3D(CB_DEF_ADDRESS_HIGH), 3);
   OUT_RELOCh(chan, screen->uniforms, 3 << 16, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   OUT_RELOCl(chan, screen->uniforms, 3 << 16, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   OUT_RING  (chan, (NV50_CB_AUX << 16) | 0x0200);

   BEGIN_RING_NI(chan, RING_3D(SET_PROGRAM_CB), 6);
   OUT_RING  (chan, (NV50_CB_PVP << 12) | 0x001);
   OUT_RING  (chan, (NV50_CB_PGP << 12) | 0x021);
   OUT_RING  (chan, (NV50_CB_PFP << 12) | 0x031);
   OUT_RING  (chan, (NV50_CB_AUX << 12) | 0xf01);
   OUT_RING  (chan, (NV50_CB_AUX << 12) | 0xf21);
   OUT_RING  (chan, (NV50_CB_AUX << 12) | 0xf31);

   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 16, 3 << 16,
                        &screen->txc);
   if (ret)
      FAIL_SCREEN_INIT("Could not allocate TIC/TSC bo: %d\n", ret);

   /* max TIC (bits 4:8) & TSC bindings, per program type */
   for (i = 0; i < 3; ++i) {
      BEGIN_RING(chan, RING_3D(TEX_LIMITS(i)), 1);
      OUT_RING  (chan, 0x54);
   }

   BEGIN_RING(chan, RING_3D(TIC_ADDRESS_HIGH), 3);
   OUT_RELOCh(chan, screen->txc, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   OUT_RELOCl(chan, screen->txc, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   OUT_RING  (chan, NV50_TIC_MAX_ENTRIES - 1);

   BEGIN_RING(chan, RING_3D(TSC_ADDRESS_HIGH), 3);
   OUT_RELOCh(chan, screen->txc, 65536, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   OUT_RELOCl(chan, screen->txc, 65536, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   OUT_RING  (chan, NV50_TSC_MAX_ENTRIES - 1);

   BEGIN_RING(chan, RING_3D(LINKED_TSC), 1);
   OUT_RING  (chan, 0);

   BEGIN_RING(chan, RING_3D(CLIP_RECTS_EN), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D(CLIP_RECTS_MODE), 1);
   OUT_RING  (chan, NV50_3D_CLIP_RECTS_MODE_INSIDE_ANY);
   BEGIN_RING(chan, RING_3D(CLIP_RECT_HORIZ(0)), 8 * 2);
   for (i = 0; i < 8 * 2; ++i)
      OUT_RING(chan, 0);
   BEGIN_RING(chan, RING_3D(CLIPID_ENABLE), 1);
   OUT_RING  (chan, 0);

   BEGIN_RING(chan, RING_3D(VIEWPORT_TRANSFORM_EN), 1);
   OUT_RING  (chan, 1);
   BEGIN_RING(chan, RING_3D(DEPTH_RANGE_NEAR(0)), 2);
   OUT_RINGf (chan, 0.0f);
   OUT_RINGf (chan, 1.0f);

   BEGIN_RING(chan, RING_3D(VIEW_VOLUME_CLIP_CTRL), 1);
#ifdef NV50_SCISSORS_CLIPPING
   OUT_RING  (chan, 0x0000);
#else
   OUT_RING  (chan, 0x1080);
#endif

   BEGIN_RING(chan, RING_3D(CLEAR_FLAGS), 1);
   OUT_RING  (chan, NV50_3D_CLEAR_FLAGS_CLEAR_RECT_VIEWPORT);

   /* We use scissors instead of exact view volume clipping,
    * so they're always enabled.
    */
   BEGIN_RING(chan, RING_3D(SCISSOR_ENABLE(0)), 3);
   OUT_RING  (chan, 1);
   OUT_RING  (chan, 8192 << 16);
   OUT_RING  (chan, 8192 << 16);

   BEGIN_RING(chan, RING_3D(RASTERIZE_ENABLE), 1);
   OUT_RING  (chan, 1);
   BEGIN_RING(chan, RING_3D(POINT_RASTER_RULES), 1);
   OUT_RING  (chan, NV50_3D_POINT_RASTER_RULES_OGL);
   BEGIN_RING(chan, RING_3D(FRAG_COLOR_CLAMP_EN), 1);
   OUT_RING  (chan, 0x11111111);
   BEGIN_RING(chan, RING_3D(EDGEFLAG_ENABLE), 1);
   OUT_RING  (chan, 1);

   FIRE_RING (chan);

   screen->tic.entries = CALLOC(4096, sizeof(void *));
   screen->tsc.entries = screen->tic.entries + 2048;

   screen->mm_VRAM_fe0 = nouveau_mm_create(dev, NOUVEAU_BO_VRAM, 0xfe0);

   nouveau_fence_new(&screen->base, &screen->base.fence.current, FALSE);

   return pscreen;

fail:
   nv50_screen_destroy(pscreen);
   return NULL;
}

void
nv50_screen_make_buffers_resident(struct nv50_screen *screen)
{
   struct nouveau_channel *chan = screen->base.channel;

   const unsigned flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_RD;

   MARK_RING(chan, 5, 5);
   nouveau_bo_validate(chan, screen->code, flags);
   nouveau_bo_validate(chan, screen->uniforms, flags);
   nouveau_bo_validate(chan, screen->txc, flags);
   nouveau_bo_validate(chan, screen->tls_bo, flags);
   nouveau_bo_validate(chan, screen->stack_bo, flags);
}

int
nv50_screen_tic_alloc(struct nv50_screen *screen, void *entry)
{
   int i = screen->tic.next;

   while (screen->tic.lock[i / 32] & (1 << (i % 32)))
      i = (i + 1) & (NV50_TIC_MAX_ENTRIES - 1);

   screen->tic.next = (i + 1) & (NV50_TIC_MAX_ENTRIES - 1);

   if (screen->tic.entries[i])
      nv50_tic_entry(screen->tic.entries[i])->id = -1;

   screen->tic.entries[i] = entry;
   return i;
}

int
nv50_screen_tsc_alloc(struct nv50_screen *screen, void *entry)
{
   int i = screen->tsc.next;

   while (screen->tsc.lock[i / 32] & (1 << (i % 32)))
      i = (i + 1) & (NV50_TSC_MAX_ENTRIES - 1);

   screen->tsc.next = (i + 1) & (NV50_TSC_MAX_ENTRIES - 1);

   if (screen->tsc.entries[i])
      nv50_tsc_entry(screen->tsc.entries[i])->id = -1;

   screen->tsc.entries[i] = entry;
   return i;
}
