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

#include "util/u_format.h"
#include "util/u_format_s3tc.h"
#include "pipe/p_screen.h"

#include "vl/vl_decoder.h"
#include "vl/vl_video_buffer.h"

#include "nvc0_context.h"
#include "nvc0_screen.h"

#include "nouveau/nv_object.xml.h"
#include "nvc0_graph_macros.h"

static boolean
nvc0_screen_is_format_supported(struct pipe_screen *pscreen,
                                enum pipe_format format,
                                enum pipe_texture_target target,
                                unsigned sample_count,
                                unsigned bindings)
{
   if (!(0x117 & (1 << sample_count))) /* 0, 1, 2, 4 or 8 */
      return FALSE;

   if (!util_format_is_supported(format, bindings))
      return FALSE;

   switch (format) {
   case PIPE_FORMAT_R8G8B8A8_UNORM:
   case PIPE_FORMAT_R8G8B8X8_UNORM:
      /* HACK: GL requires equal formats for MS resolve and window is BGRA */
      if (bindings & PIPE_BIND_RENDER_TARGET)
         return FALSE;
   default:
      break;
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
   case PIPE_CAP_MAX_COMBINED_SAMPLERS:
      return 16 * PIPE_SHADER_TYPES; /* NOTE: should not count COMPUTE */
   case PIPE_CAP_MAX_TEXTURE_2D_LEVELS:
   case PIPE_CAP_MAX_TEXTURE_CUBE_LEVELS:
      return 15;
   case PIPE_CAP_MAX_TEXTURE_3D_LEVELS:
      return 12;
   case PIPE_CAP_MAX_TEXTURE_ARRAY_LAYERS:
      return 2048;
   case PIPE_CAP_MIN_TEXEL_OFFSET:
      return -8;
   case PIPE_CAP_MAX_TEXEL_OFFSET:
      return 7;
   case PIPE_CAP_TEXTURE_MIRROR_CLAMP:
   case PIPE_CAP_TEXTURE_SWIZZLE:
   case PIPE_CAP_TEXTURE_SHADOW_MAP:
   case PIPE_CAP_NPOT_TEXTURES:
   case PIPE_CAP_ANISOTROPIC_FILTER:
   case PIPE_CAP_SEAMLESS_CUBE_MAP:
      return 1;
   case PIPE_CAP_SEAMLESS_CUBE_MAP_PER_TEXTURE:
      return 0;
   case PIPE_CAP_TWO_SIDED_STENCIL:
   case PIPE_CAP_DEPTH_CLIP_DISABLE:
   case PIPE_CAP_DEPTHSTENCIL_CLEAR_SEPARATE:
   case PIPE_CAP_POINT_SPRITE:
      return 1;
   case PIPE_CAP_SM3:
      return 1;
   case PIPE_CAP_MAX_RENDER_TARGETS:
      return 8;
   case PIPE_CAP_FRAGMENT_COLOR_CLAMP_CONTROL:
      return 1;
   case PIPE_CAP_TIMER_QUERY:
   case PIPE_CAP_OCCLUSION_QUERY:
   case PIPE_CAP_STREAM_OUTPUT_PAUSE_RESUME:
      return 1;
   case PIPE_CAP_MAX_STREAM_OUTPUT_BUFFERS:
      return 4;
   case PIPE_CAP_MAX_STREAM_OUTPUT_SEPARATE_COMPONENTS:
   case PIPE_CAP_MAX_STREAM_OUTPUT_INTERLEAVED_COMPONENTS:
      return 128;
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
   case PIPE_CAP_TGSI_INSTANCEID:
   case PIPE_CAP_VERTEX_ELEMENT_INSTANCE_DIVISOR:
   case PIPE_CAP_MIXED_COLORBUFFER_FORMATS:
   case PIPE_CAP_CONDITIONAL_RENDER:
   case PIPE_CAP_TEXTURE_BARRIER:
      return 1;
   case PIPE_CAP_TGSI_CAN_COMPACT_VARYINGS:
   case PIPE_CAP_TGSI_CAN_COMPACT_CONSTANTS:
      return 0; /* state trackers will know better */
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
      return 16;
   case PIPE_SHADER_CAP_MAX_INPUTS:
      if (shader == PIPE_SHADER_VERTEX)
         return 32;
      if (shader == PIPE_SHADER_FRAGMENT)
         return (0x200 + 0x20 + 0x80) / 16; /* generic + colors + TexCoords */
      return (0x200 + 0x40 + 0x80) / 16; /* without 0x60 for per-patch inputs */
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
      return 1; /* but inlining everything, we need function declarations */
   case PIPE_SHADER_CAP_INTEGERS:
      return 1;
   case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS:
      return 16; /* would be 32 in linked (OpenGL-style) mode */
      /*
   case PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLER_VIEWS:
      return 32;
      */
   case PIPE_SHADER_CAP_OUTPUT_READ:
      return 0; /* shader != PIPE_SHADER_TESSELLATION_CONTROL; */
   default:
      NOUVEAU_ERR("unknown PIPE_SHADER_CAP %d\n", param);
      return 0;
   }
}

static float
nvc0_screen_get_paramf(struct pipe_screen *pscreen, enum pipe_capf param)
{
   switch (param) {
   case PIPE_CAPF_MAX_LINE_WIDTH:
   case PIPE_CAPF_MAX_LINE_WIDTH_AA:
      return 10.0f;
   case PIPE_CAPF_MAX_POINT_WIDTH:
      return 63.0f;
   case PIPE_CAPF_MAX_POINT_WIDTH_AA:
      return 63.375f;
   case PIPE_CAPF_MAX_TEXTURE_ANISOTROPY:
      return 16.0f;
   case PIPE_CAPF_MAX_TEXTURE_LOD_BIAS:
      return 15.0f;
   default:
      NOUVEAU_ERR("unknown PIPE_CAP %d\n", param);
      return 0.0f;
   }
}

static void
nvc0_screen_destroy(struct pipe_screen *pscreen)
{
   struct nvc0_screen *screen = nvc0_screen(pscreen);

   if (screen->base.fence.current) {
      nouveau_fence_wait(screen->base.fence.current);
      nouveau_fence_ref(NULL, &screen->base.fence.current);
   }
   if (screen->base.channel)
      screen->base.channel->user_private = NULL;

   if (screen->blitctx)
      FREE(screen->blitctx);

   nouveau_bo_ref(NULL, &screen->text);
   nouveau_bo_ref(NULL, &screen->tls);
   nouveau_bo_ref(NULL, &screen->txc);
   nouveau_bo_ref(NULL, &screen->fence.bo);
   nouveau_bo_ref(NULL, &screen->vfetch_cache);

   nouveau_resource_destroy(&screen->lib_code);
   nouveau_resource_destroy(&screen->text_heap);

   if (screen->tic.entries)
      FREE(screen->tic.entries);

   nouveau_mm_destroy(screen->mm_VRAM_fe0);

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
}

static void
nvc0_screen_fence_emit(struct pipe_screen *pscreen, u32 *sequence)
{
   struct nvc0_screen *screen = nvc0_screen(pscreen);
   struct nouveau_channel *chan = screen->base.channel;

   MARK_RING (chan, 5, 2);

   /* we need to do it after possible flush in MARK_RING */
   *sequence = ++screen->base.fence.sequence;

   BEGIN_RING(chan, RING_3D(QUERY_ADDRESS_HIGH), 4);
   OUT_RELOCh(chan, screen->fence.bo, 0, NOUVEAU_BO_WR);
   OUT_RELOCl(chan, screen->fence.bo, 0, NOUVEAU_BO_WR);
   OUT_RING  (chan, *sequence);
   OUT_RING  (chan, NVC0_3D_QUERY_GET_FENCE | NVC0_3D_QUERY_GET_SHORT |
              (0xf << NVC0_3D_QUERY_GET_UNIT__SHIFT));
}

static u32
nvc0_screen_fence_update(struct pipe_screen *pscreen)
{
   struct nvc0_screen *screen = nvc0_screen(pscreen);
   return screen->fence.map[0];
}

#define FAIL_SCREEN_INIT(str, err)                    \
   do {                                               \
      NOUVEAU_ERR(str, err);                          \
      nvc0_screen_destroy(pscreen);                   \
      return NULL;                                    \
   } while(0)

struct pipe_screen *
nvc0_screen_create(struct nouveau_device *dev)
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

   screen->base.sysmem_bindings = PIPE_BIND_CONSTANT_BUFFER;

   ret = nouveau_screen_init(&screen->base, dev);
   if (ret) {
      nvc0_screen_destroy(pscreen);
      return NULL;
   }
   chan = screen->base.channel;
   chan->user_private = screen;

   pscreen->destroy = nvc0_screen_destroy;
   pscreen->context_create = nvc0_create;
   pscreen->is_format_supported = nvc0_screen_is_format_supported;
   pscreen->get_param = nvc0_screen_get_param;
   pscreen->get_shader_param = nvc0_screen_get_shader_param;
   pscreen->get_paramf = nvc0_screen_get_paramf;

   nvc0_screen_init_resource_functions(pscreen);

   nouveau_screen_init_vdec(&screen->base);

   ret = nouveau_bo_new(dev, NOUVEAU_BO_GART | NOUVEAU_BO_MAP, 0, 4096,
                        &screen->fence.bo);
   if (ret)
      goto fail;
   nouveau_bo_map(screen->fence.bo, NOUVEAU_BO_RDWR);
   screen->fence.map = screen->fence.bo->map;
   nouveau_bo_unmap(screen->fence.bo);
   screen->base.fence.emit = nvc0_screen_fence_emit;
   screen->base.fence.update = nvc0_screen_fence_update;

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

   if (debug_get_bool_option("NOUVEAU_SHADER_WATCHDOG", TRUE)) {
      /* kill shaders after about 1 second (at 100 MHz) */
      BEGIN_RING(chan, RING_3D(WATCHDOG_TIMER), 1);
      OUT_RING  (chan, 0x17);
   }

   BEGIN_RING(chan, RING_3D(RT_CONTROL), 1);
   OUT_RING  (chan, 1);

   BEGIN_RING(chan, RING_3D(CSAA_ENABLE), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D(MULTISAMPLE_ENABLE), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D(MULTISAMPLE_MODE), 1);
   OUT_RING  (chan, NVC0_3D_MULTISAMPLE_MODE_MS1);
   BEGIN_RING(chan, RING_3D(MULTISAMPLE_CTRL), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D(LINE_WIDTH_SEPARATE), 1);
   OUT_RING  (chan, 1);
   BEGIN_RING(chan, RING_3D(LINE_LAST_PIXEL), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D(BLEND_SEPARATE_ALPHA), 1);
   OUT_RING  (chan, 1);
   BEGIN_RING(chan, RING_3D(BLEND_ENABLE_COMMON), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D(TEX_MISC), 1);
   OUT_RING  (chan, NVC0_3D_TEX_MISC_SEAMLESS_CUBE_MAP);

   nvc0_magic_3d_init(chan);

   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 17, 1 << 20, &screen->text);
   if (ret)
      goto fail;

   /* XXX: getting a page fault at the end of the code buffer every few
    *  launches, don't use the last 256 bytes to work around them - prefetch ?
    */
   nouveau_resource_init(&screen->text_heap, 0, (1 << 20) - 0x100);

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

   screen->tls_size = (16 * 32) * (NVC0_CAP_MAX_PROGRAM_TEMPS * 16);
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
   BEGIN_RING(chan, RING_3D_(0x07a0), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D(LOCAL_BASE), 1);
   OUT_RING  (chan, 0);

   for (i = 0; i < 5; ++i) {
      BEGIN_RING(chan, RING_3D(TEX_LIMITS(i)), 1);
      OUT_RING  (chan, 0x54);
   }
   BEGIN_RING(chan, RING_3D(LINKED_TSC), 1);
   OUT_RING  (chan, 0);

   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 1 << 17, 1 << 20,
                        &screen->vfetch_cache);
   if (ret)
      goto fail;

   BEGIN_RING(chan, RING_3D(VERTEX_QUARANTINE_ADDRESS_HIGH), 3);
   OUT_RELOCh(chan, screen->vfetch_cache, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
   OUT_RELOCl(chan, screen->vfetch_cache, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR);
   OUT_RING  (chan, 3);

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

   BEGIN_RING(chan, RING_3D(CLIP_RECTS_MODE), 1);
   OUT_RING  (chan, NVC0_3D_CLIP_RECTS_MODE_INSIDE_ANY);
   BEGIN_RING(chan, RING_3D(CLIP_RECT_HORIZ(0)), 8 * 2);
   for (i = 0; i < 8 * 2; ++i)
      OUT_RING(chan, 0);
   BEGIN_RING(chan, RING_3D(CLIP_RECTS_EN), 1);
   OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D(CLIPID_ENABLE), 1);
   OUT_RING  (chan, 0);

   /* neither scissors, viewport nor stencil mask should affect clears */
   BEGIN_RING(chan, RING_3D(CLEAR_FLAGS), 1);
   OUT_RING  (chan, 0);

   BEGIN_RING(chan, RING_3D(VIEWPORT_TRANSFORM_EN), 1);
   OUT_RING  (chan, 1);
   BEGIN_RING(chan, RING_3D(DEPTH_RANGE_NEAR(0)), 2);
   OUT_RINGf (chan, 0.0f);
   OUT_RINGf (chan, 1.0f);
   BEGIN_RING(chan, RING_3D(VIEW_VOLUME_CLIP_CTRL), 1);
   OUT_RING  (chan, NVC0_3D_VIEW_VOLUME_CLIP_CTRL_UNK1_UNK1);

   /* We use scissors instead of exact view volume clipping,
    * so they're always enabled.
    */
   BEGIN_RING(chan, RING_3D(SCISSOR_ENABLE(0)), 3);
   OUT_RING  (chan, 1);
   OUT_RING  (chan, 8192 << 16);
   OUT_RING  (chan, 8192 << 16);

#define MK_MACRO(m, n) i = nvc0_graph_set_macro(screen, m, i, sizeof(n), n);

   i = 0;
   MK_MACRO(NVC0_3D_BLEND_ENABLES, nvc0_9097_blend_enables);
   MK_MACRO(NVC0_3D_VERTEX_ARRAY_SELECT, nvc0_9097_vertex_array_select);
   MK_MACRO(NVC0_3D_TEP_SELECT, nvc0_9097_tep_select);
   MK_MACRO(NVC0_3D_GP_SELECT, nvc0_9097_gp_select);
   MK_MACRO(NVC0_3D_POLYGON_MODE_FRONT, nvc0_9097_poly_mode_front);
   MK_MACRO(NVC0_3D_POLYGON_MODE_BACK, nvc0_9097_poly_mode_back);

   BEGIN_RING(chan, RING_3D(RASTERIZE_ENABLE), 1);
   OUT_RING  (chan, 1);
   BEGIN_RING(chan, RING_3D(RT_SEPARATE_FRAG_DATA), 1);
   OUT_RING  (chan, 1);
   BEGIN_RING(chan, RING_3D(GP_SELECT), 1);
   OUT_RING  (chan, 0x40);
   BEGIN_RING(chan, RING_3D(LAYER), 1);
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

   BEGIN_RING(chan, RING_3D(EDGEFLAG_ENABLE), 1);
   OUT_RING  (chan, 1);

   BEGIN_RING(chan, RING_3D(VERTEX_RUNOUT_ADDRESS_HIGH), 2);
   OUT_RING  (chan, 0xab);
   OUT_RING  (chan, 0x00000000);

   FIRE_RING (chan);

   screen->tic.entries = CALLOC(4096, sizeof(void *));
   screen->tsc.entries = screen->tic.entries + 2048;

   screen->mm_VRAM_fe0 = nouveau_mm_create(dev, NOUVEAU_BO_VRAM, 0xfe0);

   if (!nvc0_blitctx_create(screen))
      goto fail;

   nouveau_fence_new(&screen->base, &screen->base.fence.current, FALSE);

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

   MARK_RING(chan, 0, 5);
   nouveau_bo_validate(chan, screen->text, flags);
   nouveau_bo_validate(chan, screen->uniforms, flags);
   nouveau_bo_validate(chan, screen->txc, flags);
   nouveau_bo_validate(chan, screen->vfetch_cache, flags);

   if (screen->cur_ctx && screen->cur_ctx->state.tls_required)
      nouveau_bo_validate(chan, screen->tls, flags);
}

int
nvc0_screen_tic_alloc(struct nvc0_screen *screen, void *entry)
{
   int i = screen->tic.next;

   while (screen->tic.lock[i / 32] & (1 << (i % 32)))
      i = (i + 1) & (NVC0_TIC_MAX_ENTRIES - 1);

   screen->tic.next = (i + 1) & (NVC0_TIC_MAX_ENTRIES - 1);

   if (screen->tic.entries[i])
      nv50_tic_entry(screen->tic.entries[i])->id = -1;

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
      nv50_tsc_entry(screen->tsc.entries[i])->id = -1;

   screen->tsc.entries[i] = entry;
   return i;
}
