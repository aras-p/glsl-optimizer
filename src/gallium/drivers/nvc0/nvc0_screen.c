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

#include "nvc0_fence.h"
#include "nvc0_context.h"
#include "nvc0_screen.h"

#include "nouveau/nv_object.xml.h"
#include "nvc0_graph_macros.h"

static boolean
nvc0_screen_is_format_supported(struct pipe_screen *pscreen,
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

   /* transfers & shared are always supported */
   bindings &= ~(PIPE_BIND_TRANSFER_READ |
                 PIPE_BIND_TRANSFER_WRITE |
                 PIPE_BIND_SHARED);

   return (nvc0_format_table[format].usage & bindings) == bindings;
}

static int
nvc0_screen_get_param(struct pipe_screen *pscreen, enum pipe_cap param)
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
   case PIPE_CAP_ARRAY_TEXTURES:
      return 1;
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
nvc0_screen_get_shader_param(struct pipe_screen *pscreen, unsigned shader,
                             enum pipe_shader_cap param)
{
   switch (shader) {
   case PIPE_SHADER_VERTEX:
      /*
   case PIPE_SHADER_TESSELLATION_CONTROL:
   case PIPE_SHADER_TESSELLATION_EVALUATION:
      */
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
      return NVC0_CAP_MAX_PROGRAM_TEMPS;
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
nvc0_screen_get_paramf(struct pipe_screen *pscreen, enum pipe_cap param)
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
nvc0_screen_destroy(struct pipe_screen *pscreen)
{
   struct nvc0_screen *screen = nvc0_screen(pscreen);

   nvc0_fence_wait(screen->fence.current);
   nvc0_fence_reference(&screen->fence.current, NULL);

   nouveau_bo_ref(NULL, &screen->text);
   nouveau_bo_ref(NULL, &screen->tls);
   nouveau_bo_ref(NULL, &screen->txc);
   nouveau_bo_ref(NULL, &screen->fence.bo);
   nouveau_bo_ref(NULL, &screen->mp_stack_bo);

   nouveau_resource_destroy(&screen->text_heap);

   if (screen->tic.entries)
      FREE(screen->tic.entries);

   nvc0_mm_destroy(screen->mm_GART);
   nvc0_mm_destroy(screen->mm_VRAM);
   nvc0_mm_destroy(screen->mm_VRAM_fe0);

   nouveau_grobj_free(&screen->fermi);
   nouveau_grobj_free(&screen->eng2d);
   nouveau_grobj_free(&screen->m2mf);

   nouveau_screen_fini(&screen->base);

   FREE(screen);
}

static int
nvc0_graph_set_macro(struct nvc0_screen *screen, uint32_t m, unsigned pos,
                     unsigned size, const uint32_t *data)
{
   struct nouveau_channel *chan = screen->base.channel;

   size /= 4;

   BEGIN_RING(chan, RING_3D_(NVC0_GRAPH_MACRO_ID), 2);
   OUT_RING  (chan, (m - 0x3800) / 8);
   OUT_RING  (chan, pos);
   BEGIN_RING_1I(chan, RING_3D_(NVC0_GRAPH_MACRO_UPLOAD_POS), size + 1);
   OUT_RING  (chan, pos);
   OUT_RINGp (chan, data, size);

   return pos + size;
}

static void
nvc0_screen_fence_reference(struct pipe_screen *pscreen,
                            struct pipe_fence_handle **ptr,
                            struct pipe_fence_handle *fence)
{
   nvc0_fence_reference((struct nvc0_fence **)ptr, nvc0_fence(fence));
}

static int
nvc0_screen_fence_signalled(struct pipe_screen *pscreen,
                            struct pipe_fence_handle *fence,
                            unsigned flags)
{
   return !(nvc0_fence_signalled(nvc0_fence(fence)));
}

static int
nvc0_screen_fence_finish(struct pipe_screen *pscreen,
                         struct pipe_fence_handle *fence,
                         unsigned flags)
{
   return nvc0_fence_wait((struct nvc0_fence *)fence) != TRUE;
}

static void
nvc0_magic_3d_init(struct nouveau_channel *chan)
{
   BEGIN_RING(chan, RING_3D_(0x10cc), 1);
   OUT_RING  (chan, 0xff);
   BEGIN_RING(chan, RING_3D_(0x10e0), 2);
   OUT_RING(chan, 0xff);
   OUT_RING(chan, 0xff);
   BEGIN_RING(chan, RING_3D_(0x10ec), 2);
   OUT_RING(chan, 0xff);
   OUT_RING(chan, 0xff);
   BEGIN_RING(chan, RING_3D_(0x074c), 1);
   OUT_RING  (chan, 0x3f);

   BEGIN_RING(chan, RING_3D_(0x10f8), 1);
   OUT_RING  (chan, 0x0101);

   BEGIN_RING(chan, RING_3D_(0x16a8), 1);
   OUT_RING  (chan, (3 << 16) | 3);
   BEGIN_RING(chan, RING_3D_(0x1794), 1);
   OUT_RING  (chan, (2 << 16) | 2);
   BEGIN_RING(chan, RING_3D_(0x0de8), 1);
   OUT_RING  (chan, 1);

#if 0 /* software method */
   BEGIN_RING(chan, RING_3D_(0x1528), 1); /* MP poke */
   OUT_RING  (chan, 0);
#endif

   BEGIN_RING(chan, RING_3D_(0x12ac), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D_(0x0218), 1);
   OUT_RING  (chan, 0x10);
   BEGIN_RING(chan, RING_3D_(0x10fc), 1);
   OUT_RING  (chan, 0x10);
   BEGIN_RING(chan, RING_3D_(0x1290), 1);
   OUT_RING  (chan, 0x10);
   BEGIN_RING(chan, RING_3D_(0x12d8), 2);
   OUT_RING  (chan, 0x10);
   OUT_RING  (chan, 0x10);
   BEGIN_RING(chan, RING_3D_(0x06d4), 1);
   OUT_RING  (chan, 8);
   BEGIN_RING(chan, RING_3D_(0x1140), 1);
   OUT_RING  (chan, 0x10);
   BEGIN_RING(chan, RING_3D_(0x1610), 1);
   OUT_RING  (chan, 0xe);

   BEGIN_RING(chan, RING_3D_(0x164c), 1);
   OUT_RING  (chan, 1 << 12);
   BEGIN_RING(chan, RING_3D_(0x151c), 1);
   OUT_RING  (chan, 1);
   BEGIN_RING(chan, RING_3D_(0x020c), 1);
   OUT_RING  (chan, 1);
   BEGIN_RING(chan, RING_3D_(0x030c), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D_(0x0300), 1);
   OUT_RING  (chan, 3);
#if 0 /* software method */
   BEGIN_RING(chan, RING_3D_(0x1280), 1); /* PGRAPH poke */
   OUT_RING  (chan, 0);
#endif
   BEGIN_RING(chan, RING_3D_(0x02d0), 1);
   OUT_RING  (chan, 0x1f40);
   BEGIN_RING(chan, RING_3D_(0x00fdc), 1);
   OUT_RING  (chan, 1);
   BEGIN_RING(chan, RING_3D_(0x19c0), 1);
   OUT_RING  (chan, 1);
   BEGIN_RING(chan, RING_3D_(0x075c), 1);
   OUT_RING  (chan, 3);

   BEGIN_RING(chan, RING_3D_(0x0fac), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D_(0x0f90), 1);
   OUT_RING  (chan, 0);
}

#define FAIL_SCREEN_INIT(str, err)                    \
   do {                                               \
      NOUVEAU_ERR(str, err);                          \
      nvc0_screen_destroy(pscreen);                   \
      return NULL;                                    \
   } while(0)

struct pipe_screen *
nvc0_screen_create(struct pipe_winsys *ws, struct nouveau_device *dev)
{
   struct nvc0_screen *screen;
   struct nouveau_channel *chan;
   struct pipe_screen *pscreen;
   int ret;
   unsigned i;

   screen = CALLOC_STRUCT(nvc0_screen);
   if (!screen)
      return NULL;
   pscreen = &screen->base.base;

   ret = nouveau_screen_init(&screen->base, dev);
   if (ret) {
      nvc0_screen_destroy(pscreen);
      return NULL;
   }
   chan = screen->base.channel;

   pscreen->winsys = ws;
   pscreen->destroy = nvc0_screen_destroy;
   pscreen->context_create = nvc0_create;
   pscreen->is_format_supported = nvc0_screen_is_format_supported;
   pscreen->get_param = nvc0_screen_get_param;
   pscreen->get_shader_param = nvc0_screen_get_shader_param;
   pscreen->get_paramf = nvc0_screen_get_paramf;
   pscreen->fence_reference = nvc0_screen_fence_reference;
   pscreen->fence_signalled = nvc0_screen_fence_signalled;
   pscreen->fence_finish = nvc0_screen_fence_finish;

   nvc0_screen_init_resource_functions(pscreen);

   screen->base.vertex_buffer_flags = NOUVEAU_BO_GART;
   screen->base.index_buffer_flags = 0;

   ret = nouveau_bo_new(dev, NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 0, 4096,
                        &screen->fence.bo);
   if (ret)
      goto fail;
   nouveau_bo_map(screen->fence.bo, NOUVEAU_BO_RDWR);
   screen->fence.map = screen->fence.bo->map;
   nouveau_bo_unmap(screen->fence.bo);

   for (i = 0; i < NVC0_SCRATCH_NR_BUFFERS; ++i) {
      ret = nouveau_bo_new(dev, NOUVEAU_BO_GART, 0, NVC0_SCRATCH_SIZE,
                           &screen->scratch.bo[i]);
      if (ret)
         goto fail;
   }

   ret = nouveau_grobj_alloc(chan, 0xbeef9039, NVC0_M2MF, &screen->m2mf);
   if (ret)
      FAIL_SCREEN_INIT("Error allocating PGRAPH context for M2MF: %d\n", ret);

   BIND_RING (chan, screen->m2mf, NVC0_SUBCH_MF);
   BEGIN_RING(chan, RING_MF(NOTIFY_ADDRESS_HIGH), 3);
   OUT_RELOCh(chan, screen->fence.bo, 16, NOUVEAU_BO_GART | NOUVEAU_BO_RDWR);
   OUT_RELOCl(chan, screen->fence.bo, 16, NOUVEAU_BO_GART | NOUVEAU_BO_RDWR);
   OUT_RING  (chan, 0);

   ret = nouveau_grobj_alloc(chan, 0xbeef902d, NVC0_2D, &screen->eng2d);
   if (ret)
      FAIL_SCREEN_INIT("Error allocating PGRAPH context for 2D: %d\n", ret);

   BIND_RING (chan, screen->eng2d, NVC0_SUBCH_2D);
   BEGIN_RING(chan, RING_2D(OPERATION), 1);
   OUT_RING  (chan, NVC0_2D_OPERATION_SRCCOPY);
   BEGIN_RING(chan, RING_2D(CLIP_ENABLE), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_2D(COLOR_KEY_ENABLE), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_2D_(0x0884), 1);
   OUT_RING  (chan, 0x3f);
   BEGIN_RING(chan, RING_2D_(0x0888), 1);
   OUT_RING  (chan, 1);

   ret = nouveau_grobj_alloc(chan, 0xbeef9097, NVC0_3D, &screen->fermi);
   if (ret)
      FAIL_SCREEN_INIT("Error allocating PGRAPH context for 3D: %d\n", ret);

   BIND_RING (chan, screen->fermi, NVC0_SUBCH_3D);
   BEGIN_RING(chan, RING_3D(NOTIFY_ADDRESS_HIGH), 3);
   OUT_RELOCh(chan, screen->fence.bo, 32, NOUVEAU_BO_GART | NOUVEAU_BO_RDWR);
   OUT_RELOCl(chan, screen->fence.bo, 32, NOUVEAU_BO_GART | NOUVEAU_BO_RDWR);
   OUT_RING  (chan, 0);

   BEGIN_RING(chan, RING_3D(COND_MODE), 1);
   OUT_RING  (chan, NVC0_3D_COND_MODE_ALWAYS);

   BEGIN_RING(chan, RING_3D(RT_CONTROL), 1);
   OUT_RING  (chan, 1);

   BEGIN_RING(chan, RING_3D(CSAA_ENABLE), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D(MULTISAMPLE_ENABLE), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D(MULTISAMPLE_MODE), 1);
   OUT_RING  (chan, NVC0_3D_MULTISAMPLE_MODE_1X);
   BEGIN_RING(chan, RING_3D(MULTISAMPLE_CTRL), 1);
   OUT_RING  (chan, 0);

   nvc0_magic_3d_init(chan);

   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 17, 1 << 20, &screen->text);
   if (ret)
      goto fail;

   nouveau_resource_init(&screen->text_heap, 0, 1 << 20);

   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 12, 6 << 16,
                        &screen->uniforms);
   if (ret)
      goto fail;

   /* auxiliary constants (6 user clip planes, base instance id) */
   BEGIN_RING(chan, RING_3D(CB_SIZE), 3);
   OUT_RING  (chan, 256);
   OUT_RELOCh(chan, screen->uniforms, 5 << 16, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   OUT_RELOCl(chan, screen->uniforms, 5 << 16, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   for (i = 0; i < 5; ++i) {
      BEGIN_RING(chan, RING_3D(CB_BIND(i)), 1);
      OUT_RING  (chan, (15 << 4) | 1);
   }

   screen->tls_size = 4 * 4 * 32 * 128 * 4;
   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 17,
                        screen->tls_size, &screen->tls);
   if (ret)
      goto fail;

   BEGIN_RING(chan, RING_3D(CODE_ADDRESS_HIGH), 2);
   OUT_RELOCh(chan, screen->text, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   OUT_RELOCl(chan, screen->text, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   BEGIN_RING(chan, RING_3D(LOCAL_ADDRESS_HIGH), 4);
   OUT_RELOCh(chan, screen->tls, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
   OUT_RELOCl(chan, screen->tls, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
   OUT_RING  (chan, screen->tls_size >> 32);
   OUT_RING  (chan, screen->tls_size);
   BEGIN_RING(chan, RING_3D(LOCAL_BASE), 1);
   OUT_RING  (chan, 0);

   for (i = 0; i < 5; ++i) {
      BEGIN_RING(chan, RING_3D(TEX_LIMITS(i)), 1);
      OUT_RING  (chan, 0x54);
   }
   BEGIN_RING(chan, RING_3D(LINKED_TSC), 1);
   OUT_RING  (chan, 0);

   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 17, 1 << 20,
                        &screen->mp_stack_bo);
   if (ret)
      goto fail;

   BEGIN_RING(chan, RING_3D_(0x17bc), 3);
   OUT_RELOCh(chan, screen->mp_stack_bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
   OUT_RELOCl(chan, screen->mp_stack_bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
   OUT_RING  (chan, 1);

   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 17, 1 << 17, &screen->txc);
   if (ret)
      goto fail;

   BEGIN_RING(chan, RING_3D(TIC_ADDRESS_HIGH), 3);
   OUT_RELOCh(chan, screen->txc, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   OUT_RELOCl(chan, screen->txc, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   OUT_RING  (chan, NVC0_TIC_MAX_ENTRIES - 1);

   BEGIN_RING(chan, RING_3D(TSC_ADDRESS_HIGH), 3);
   OUT_RELOCh(chan, screen->txc, 65536, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   OUT_RELOCl(chan, screen->txc, 65536, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
   OUT_RING  (chan, NVC0_TSC_MAX_ENTRIES - 1);

   BEGIN_RING(chan, RING_3D(SCREEN_Y_CONTROL), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D(WINDOW_OFFSET_X), 2);
   OUT_RING  (chan, 0);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D_(0x1590), 1); /* deactivate ZCULL */
   OUT_RING  (chan, 0x3f);

   BEGIN_RING(chan, RING_3D(VIEWPORT_CLIP_RECTS_EN), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D(CLIPID_ENABLE), 1);
   OUT_RING  (chan, 0);

   BEGIN_RING(chan, RING_3D(VIEWPORT_TRANSFORM_EN), 1);
   OUT_RING  (chan, 1);
   BEGIN_RING(chan, RING_3D(DEPTH_RANGE_NEAR(0)), 2);
   OUT_RINGf (chan, 0.0f);
   OUT_RINGf (chan, 1.0f);

   /* We use scissors instead of exact view volume clipping,
    * so they're always enabled.
    */
   BEGIN_RING(chan, RING_3D(SCISSOR_ENABLE(0)), 3);
   OUT_RING  (chan, 1);
   OUT_RING  (chan, 8192 << 16);
   OUT_RING  (chan, 8192 << 16);

   BEGIN_RING(chan, RING_3D_(0x0fac), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D_(0x3484), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D_(0x0dbc), 1);
   OUT_RING  (chan, 0x00010000);
   BEGIN_RING(chan, RING_3D_(0x0dd8), 1);
   OUT_RING  (chan, 0xff800006);
   BEGIN_RING(chan, RING_3D_(0x3488), 1);
   OUT_RING  (chan, 0);

#define MK_MACRO(m, n) i = nvc0_graph_set_macro(screen, m, i, sizeof(n), n);

   i = 0;
   MK_MACRO(NVC0_3D_BLEND_ENABLES, nvc0_9097_blend_enables);
   MK_MACRO(NVC0_3D_VERTEX_ARRAY_SELECT, nvc0_9097_vertex_array_select);
   MK_MACRO(NVC0_3D_TEP_SELECT, nvc0_9097_tep_select);
   MK_MACRO(NVC0_3D_GP_SELECT, nvc0_9097_gp_select);
   MK_MACRO(NVC0_3D_POLYGON_MODE_FRONT, nvc0_9097_poly_mode_front);
   MK_MACRO(NVC0_3D_POLYGON_MODE_BACK, nvc0_9097_poly_mode_back);
   MK_MACRO(NVC0_3D_COLOR_MASK_BROADCAST, nvc0_9097_color_mask_brdc);

   BEGIN_RING(chan, RING_3D(RASTERIZE_ENABLE), 1);
   OUT_RING  (chan, 1);
   BEGIN_RING(chan, RING_3D(GP_SELECT), 1);
   OUT_RING  (chan, 0x40);
   BEGIN_RING(chan, RING_3D(GP_BUILTIN_RESULT_EN), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D(TEP_SELECT), 1);
   OUT_RING  (chan, 0x30);
   BEGIN_RING(chan, RING_3D(PATCH_VERTICES), 1);
   OUT_RING  (chan, 3);
   BEGIN_RING(chan, RING_3D(SP_SELECT(2)), 1);
   OUT_RING  (chan, 0x20);
   BEGIN_RING(chan, RING_3D(SP_SELECT(0)), 1);
   OUT_RING  (chan, 0x00);

   BEGIN_RING(chan, RING_3D(POINT_COORD_REPLACE), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D(POINT_RASTER_RULES), 1);
   OUT_RING  (chan, NVC0_3D_POINT_RASTER_RULES_OGL);

   BEGIN_RING(chan, RING_3D(FRAG_COLOR_CLAMP_EN), 1);
   OUT_RING  (chan, 0x11111111);
   BEGIN_RING(chan, RING_3D(EDGEFLAG_ENABLE), 1);
   OUT_RING  (chan, 1);

   BEGIN_RING(chan, RING_3D(VERTEX_RUNOUT_ADDRESS_HIGH), 2);
   OUT_RING  (chan, 0xab);
   OUT_RING  (chan, 0x00000000);

   FIRE_RING (chan);

   screen->tic.entries = CALLOC(4096, sizeof(void *));
   screen->tsc.entries = screen->tic.entries + 2048;

   screen->mm_GART = nvc0_mm_create(dev, NOUVEAU_BO_GART | NOUVEAU_BO_MAP,
                                    0x000);
   screen->mm_VRAM = nvc0_mm_create(dev, NOUVEAU_BO_VRAM, 0x000);
   screen->mm_VRAM_fe0 = nvc0_mm_create(dev, NOUVEAU_BO_VRAM, 0xfe0);

   nvc0_screen_fence_new(screen, &screen->fence.current, FALSE);

   return pscreen;

fail:
   nvc0_screen_destroy(pscreen);
   return NULL;
}

void
nvc0_screen_make_buffers_resident(struct nvc0_screen *screen)
{
   struct nouveau_channel *chan = screen->base.channel;

   const unsigned flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_RD;

   MARK_RING(chan, 5, 5);
   nouveau_bo_validate(chan, screen->text, flags);
   nouveau_bo_validate(chan, screen->uniforms, flags);
   nouveau_bo_validate(chan, screen->txc, flags);
   nouveau_bo_validate(chan, screen->tls, flags);
   nouveau_bo_validate(chan, screen->mp_stack_bo, flags);
}

int
nvc0_screen_tic_alloc(struct nvc0_screen *screen, void *entry)
{
   int i = screen->tic.next;

   while (screen->tic.lock[i / 32] & (1 << (i % 32)))
      i = (i + 1) & (NVC0_TIC_MAX_ENTRIES - 1);

   screen->tic.next = (i + 1) & (NVC0_TIC_MAX_ENTRIES - 1);

   if (screen->tic.entries[i])
      nvc0_tic_entry(screen->tic.entries[i])->id = -1;

   screen->tic.entries[i] = entry;
   return i;
}

int
nvc0_screen_tsc_alloc(struct nvc0_screen *screen, void *entry)
{
   int i = screen->tsc.next;

   while (screen->tsc.lock[i / 32] & (1 << (i % 32)))
      i = (i + 1) & (NVC0_TSC_MAX_ENTRIES - 1);

   screen->tsc.next = (i + 1) & (NVC0_TSC_MAX_ENTRIES - 1);

   if (screen->tsc.entries[i])
      nvc0_tsc_entry(screen->tsc.entries[i])->id = -1;

   screen->tsc.entries[i] = entry;
   return i;
}
