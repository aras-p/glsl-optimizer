/*
 * Copyright 2008 Ben Skeggs
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

#include <stdint.h>

#include "pipe/p_defines.h"

#include "util/u_inlines.h"
#include "util/u_pack_color.h"
#include "util/u_format.h"
#include "util/u_surface.h"

#include "tgsi/tgsi_ureg.h"

#include "os/os_thread.h"

#include "nv50_context.h"
#include "nv50_resource.h"

#include "nv50_defs.xml.h"
#include "nv50_texture.xml.h"

/* these are used in nv50_blit.h */
#define NV50_ENG2D_SUPPORTED_FORMATS 0xff0843e080608409ULL
#define NV50_ENG2D_NOCONVERT_FORMATS 0x0008402000000000ULL
#define NV50_ENG2D_LUMINANCE_FORMATS 0x0008402000000000ULL
#define NV50_ENG2D_INTENSITY_FORMATS 0x0000000000000000ULL
#define NV50_ENG2D_OPERATION_FORMATS 0x060001c000608000ULL

#define NOUVEAU_DRIVER 0x50
#include "nv50_blit.h"

static INLINE uint8_t
nv50_2d_format(enum pipe_format format, boolean dst, boolean dst_src_equal)
{
   uint8_t id = nv50_format_table[format].rt;

   /* Hardware values for color formats range from 0xc0 to 0xff,
    * but the 2D engine doesn't support all of them.
    */
   if ((id >= 0xc0) && (NV50_ENG2D_SUPPORTED_FORMATS & (1ULL << (id - 0xc0))))
      return id;
   assert(dst_src_equal);

   switch (util_format_get_blocksize(format)) {
   case 1:
      return NV50_SURFACE_FORMAT_R8_UNORM;
   case 2:
      return NV50_SURFACE_FORMAT_R16_UNORM;
   case 4:
      return NV50_SURFACE_FORMAT_BGRA8_UNORM;
   default:
      return 0;
   }
}

static int
nv50_2d_texture_set(struct nouveau_pushbuf *push, int dst,
                    struct nv50_miptree *mt, unsigned level, unsigned layer,
                    enum pipe_format pformat, boolean dst_src_pformat_equal)
{
   struct nouveau_bo *bo = mt->base.bo;
   uint32_t width, height, depth;
   uint32_t format;
   uint32_t mthd = dst ? NV50_2D_DST_FORMAT : NV50_2D_SRC_FORMAT;
   uint32_t offset = mt->level[level].offset;

   format = nv50_2d_format(pformat, dst, dst_src_pformat_equal);
   if (!format) {
      NOUVEAU_ERR("invalid/unsupported surface format: %s\n",
                  util_format_name(pformat));
      return 1;
   }

   width = u_minify(mt->base.base.width0, level) << mt->ms_x;
   height = u_minify(mt->base.base.height0, level) << mt->ms_y;
   depth = u_minify(mt->base.base.depth0, level);

   offset = mt->level[level].offset;
   if (!mt->layout_3d) {
      offset += mt->layer_stride * layer;
      depth = 1;
      layer = 0;
   } else
   if (!dst) {
      offset += nv50_mt_zslice_offset(mt, level, layer);
      layer = 0;
   }

   if (!nouveau_bo_memtype(bo)) {
      BEGIN_NV04(push, SUBC_2D(mthd), 2);
      PUSH_DATA (push, format);
      PUSH_DATA (push, 1);
      BEGIN_NV04(push, SUBC_2D(mthd + 0x14), 5);
      PUSH_DATA (push, mt->level[level].pitch);
      PUSH_DATA (push, width);
      PUSH_DATA (push, height);
      PUSH_DATAh(push, bo->offset + offset);
      PUSH_DATA (push, bo->offset + offset);
   } else {
      BEGIN_NV04(push, SUBC_2D(mthd), 5);
      PUSH_DATA (push, format);
      PUSH_DATA (push, 0);
      PUSH_DATA (push, mt->level[level].tile_mode);
      PUSH_DATA (push, depth);
      PUSH_DATA (push, layer);
      BEGIN_NV04(push, SUBC_2D(mthd + 0x18), 4);
      PUSH_DATA (push, width);
      PUSH_DATA (push, height);
      PUSH_DATAh(push, bo->offset + offset);
      PUSH_DATA (push, bo->offset + offset);
   }

#if 0
   if (dst) {
      BEGIN_NV04(push, SUBC_2D(NV50_2D_CLIP_X), 4);
      PUSH_DATA (push, 0);
      PUSH_DATA (push, 0);
      PUSH_DATA (push, width);
      PUSH_DATA (push, height);
   }
#endif
   return 0;
}

static int
nv50_2d_texture_do_copy(struct nouveau_pushbuf *push,
                        struct nv50_miptree *dst, unsigned dst_level,
                        unsigned dx, unsigned dy, unsigned dz,
                        struct nv50_miptree *src, unsigned src_level,
                        unsigned sx, unsigned sy, unsigned sz,
                        unsigned w, unsigned h)
{
   const enum pipe_format dfmt = dst->base.base.format;
   const enum pipe_format sfmt = src->base.base.format;
   int ret;
   boolean eqfmt = dfmt == sfmt;

   if (!PUSH_SPACE(push, 2 * 16 + 32))
      return PIPE_ERROR;

   ret = nv50_2d_texture_set(push, 1, dst, dst_level, dz, dfmt, eqfmt);
   if (ret)
      return ret;

   ret = nv50_2d_texture_set(push, 0, src, src_level, sz, sfmt, eqfmt);
   if (ret)
      return ret;

   BEGIN_NV04(push, NV50_2D(BLIT_CONTROL), 1);
   PUSH_DATA (push, NV50_2D_BLIT_CONTROL_FILTER_POINT_SAMPLE);
   BEGIN_NV04(push, NV50_2D(BLIT_DST_X), 4);
   PUSH_DATA (push, dx << dst->ms_x);
   PUSH_DATA (push, dy << dst->ms_y);
   PUSH_DATA (push, w << dst->ms_x);
   PUSH_DATA (push, h << dst->ms_y);
   BEGIN_NV04(push, NV50_2D(BLIT_DU_DX_FRACT), 4);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 1);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_2D(BLIT_SRC_X_FRACT), 4);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, sx << src->ms_x);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, sy << src->ms_y);

   return 0;
}

static void
nv50_resource_copy_region(struct pipe_context *pipe,
                          struct pipe_resource *dst, unsigned dst_level,
                          unsigned dstx, unsigned dsty, unsigned dstz,
                          struct pipe_resource *src, unsigned src_level,
                          const struct pipe_box *src_box)
{
   struct nv50_context *nv50 = nv50_context(pipe);
   int ret;
   boolean m2mf;
   unsigned dst_layer = dstz, src_layer = src_box->z;

   if (dst->target == PIPE_BUFFER && src->target == PIPE_BUFFER) {
      nouveau_copy_buffer(&nv50->base,
                          nv04_resource(dst), dstx,
                          nv04_resource(src), src_box->x, src_box->width);
      return;
   }

   /* 0 and 1 are equal, only supporting 0/1, 2, 4 and 8 */
   assert((src->nr_samples | 1) == (dst->nr_samples | 1));

   m2mf = (src->format == dst->format) ||
      (util_format_get_blocksizebits(src->format) ==
       util_format_get_blocksizebits(dst->format));

   nv04_resource(dst)->status |= NOUVEAU_BUFFER_STATUS_GPU_WRITING;

   if (m2mf) {
      struct nv50_m2mf_rect drect, srect;
      unsigned i;
      unsigned nx = util_format_get_nblocksx(src->format, src_box->width);
      unsigned ny = util_format_get_nblocksy(src->format, src_box->height);

      nv50_m2mf_rect_setup(&drect, dst, dst_level, dstx, dsty, dstz);
      nv50_m2mf_rect_setup(&srect, src, src_level,
                           src_box->x, src_box->y, src_box->z);

      for (i = 0; i < src_box->depth; ++i) {
         nv50_m2mf_transfer_rect(nv50, &drect, &srect, nx, ny);

         if (nv50_miptree(dst)->layout_3d)
            drect.z++;
         else
            drect.base += nv50_miptree(dst)->layer_stride;

         if (nv50_miptree(src)->layout_3d)
            srect.z++;
         else
            srect.base += nv50_miptree(src)->layer_stride;
      }
      return;
   }

   assert((src->format == dst->format) ||
          (nv50_2d_src_format_faithful(src->format) &&
           nv50_2d_dst_format_faithful(dst->format)));

   BCTX_REFN(nv50->bufctx, 2D, nv04_resource(src), RD);
   BCTX_REFN(nv50->bufctx, 2D, nv04_resource(dst), WR);
   nouveau_pushbuf_bufctx(nv50->base.pushbuf, nv50->bufctx);
   nouveau_pushbuf_validate(nv50->base.pushbuf);

   for (; dst_layer < dstz + src_box->depth; ++dst_layer, ++src_layer) {
      ret = nv50_2d_texture_do_copy(nv50->base.pushbuf,
                                    nv50_miptree(dst), dst_level,
                                    dstx, dsty, dst_layer,
                                    nv50_miptree(src), src_level,
                                    src_box->x, src_box->y, src_layer,
                                    src_box->width, src_box->height);
      if (ret)
         break;
   }
   nouveau_bufctx_reset(nv50->bufctx, NV50_BIND_2D);
}

static void
nv50_clear_render_target(struct pipe_context *pipe,
                         struct pipe_surface *dst,
			 const union pipe_color_union *color,
                         unsigned dstx, unsigned dsty,
                         unsigned width, unsigned height)
{
   struct nv50_context *nv50 = nv50_context(pipe);
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct nv50_miptree *mt = nv50_miptree(dst->texture);
   struct nv50_surface *sf = nv50_surface(dst);
   struct nouveau_bo *bo = mt->base.bo;
   unsigned z;

   BEGIN_NV04(push, NV50_3D(CLEAR_COLOR(0)), 4);
   PUSH_DATAf(push, color->f[0]);
   PUSH_DATAf(push, color->f[1]);
   PUSH_DATAf(push, color->f[2]);
   PUSH_DATAf(push, color->f[3]);

   if (nouveau_pushbuf_space(push, 32 + sf->depth, 1, 0))
      return;

   PUSH_REFN(push, bo, mt->base.domain | NOUVEAU_BO_WR);

   BEGIN_NV04(push, NV50_3D(RT_CONTROL), 1);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_3D(RT_ADDRESS_HIGH(0)), 5);
   PUSH_DATAh(push, bo->offset + sf->offset);
   PUSH_DATA (push, bo->offset + sf->offset);
   PUSH_DATA (push, nv50_format_table[dst->format].rt);
   PUSH_DATA (push, mt->level[sf->base.u.tex.level].tile_mode);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(RT_HORIZ(0)), 2);
   if (nouveau_bo_memtype(bo))
      PUSH_DATA(push, sf->width);
   else
      PUSH_DATA(push, NV50_3D_RT_HORIZ_LINEAR | mt->level[0].pitch);
   PUSH_DATA (push, sf->height);
   BEGIN_NV04(push, NV50_3D(RT_ARRAY_MODE), 1);
   PUSH_DATA (push, 1);

   if (!nouveau_bo_memtype(bo)) {
      BEGIN_NV04(push, NV50_3D(ZETA_ENABLE), 1);
      PUSH_DATA (push, 0);
   }

   /* NOTE: only works with D3D clear flag (5097/0x143c bit 4) */

   BEGIN_NV04(push, NV50_3D(VIEWPORT_HORIZ(0)), 2);
   PUSH_DATA (push, (width << 16) | dstx);
   PUSH_DATA (push, (height << 16) | dsty);

   BEGIN_NI04(push, NV50_3D(CLEAR_BUFFERS), sf->depth);
   for (z = 0; z < sf->depth; ++z) {
      PUSH_DATA (push, 0x3c |
                 (z << NV50_3D_CLEAR_BUFFERS_LAYER__SHIFT));
   }

   nv50->dirty |= NV50_NEW_FRAMEBUFFER;
}

static void
nv50_clear_depth_stencil(struct pipe_context *pipe,
                         struct pipe_surface *dst,
                         unsigned clear_flags,
                         double depth,
                         unsigned stencil,
                         unsigned dstx, unsigned dsty,
                         unsigned width, unsigned height)
{
   struct nv50_context *nv50 = nv50_context(pipe);
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct nv50_miptree *mt = nv50_miptree(dst->texture);
   struct nv50_surface *sf = nv50_surface(dst);
   struct nouveau_bo *bo = mt->base.bo;
   uint32_t mode = 0;
   unsigned z;

   assert(nouveau_bo_memtype(bo)); /* ZETA cannot be linear */

   if (clear_flags & PIPE_CLEAR_DEPTH) {
      BEGIN_NV04(push, NV50_3D(CLEAR_DEPTH), 1);
      PUSH_DATAf(push, depth);
      mode |= NV50_3D_CLEAR_BUFFERS_Z;
   }

   if (clear_flags & PIPE_CLEAR_STENCIL) {
      BEGIN_NV04(push, NV50_3D(CLEAR_STENCIL), 1);
      PUSH_DATA (push, stencil & 0xff);
      mode |= NV50_3D_CLEAR_BUFFERS_S;
   }

   if (nouveau_pushbuf_space(push, 32 + sf->depth, 1, 0))
      return;

   PUSH_REFN(push, bo, mt->base.domain | NOUVEAU_BO_WR);

   BEGIN_NV04(push, NV50_3D(ZETA_ADDRESS_HIGH), 5);
   PUSH_DATAh(push, bo->offset + sf->offset);
   PUSH_DATA (push, bo->offset + sf->offset);
   PUSH_DATA (push, nv50_format_table[dst->format].rt);
   PUSH_DATA (push, mt->level[sf->base.u.tex.level].tile_mode);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(ZETA_ENABLE), 1);
   PUSH_DATA (push, 1);
   BEGIN_NV04(push, NV50_3D(ZETA_HORIZ), 3);
   PUSH_DATA (push, sf->width);
   PUSH_DATA (push, sf->height);
   PUSH_DATA (push, (1 << 16) | 1);

   BEGIN_NV04(push, NV50_3D(VIEWPORT_HORIZ(0)), 2);
   PUSH_DATA (push, (width << 16) | dstx);
   PUSH_DATA (push, (height << 16) | dsty);

   BEGIN_NI04(push, NV50_3D(CLEAR_BUFFERS), sf->depth);
   for (z = 0; z < sf->depth; ++z) {
      PUSH_DATA (push, mode |
                 (z << NV50_3D_CLEAR_BUFFERS_LAYER__SHIFT));
   }

   nv50->dirty |= NV50_NEW_FRAMEBUFFER;
}

void
nv50_clear(struct pipe_context *pipe, unsigned buffers,
           const union pipe_color_union *color,
           double depth, unsigned stencil)
{
   struct nv50_context *nv50 = nv50_context(pipe);
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct pipe_framebuffer_state *fb = &nv50->framebuffer;
   unsigned i;
   uint32_t mode = 0;

   /* don't need NEW_BLEND, COLOR_MASK doesn't affect CLEAR_BUFFERS */
   if (!nv50_state_validate(nv50, NV50_NEW_FRAMEBUFFER, 9 + (fb->nr_cbufs * 2)))
      return;

   if (buffers & PIPE_CLEAR_COLOR && fb->nr_cbufs) {
      BEGIN_NV04(push, NV50_3D(CLEAR_COLOR(0)), 4);
      PUSH_DATAf(push, color->f[0]);
      PUSH_DATAf(push, color->f[1]);
      PUSH_DATAf(push, color->f[2]);
      PUSH_DATAf(push, color->f[3]);
      mode =
         NV50_3D_CLEAR_BUFFERS_R | NV50_3D_CLEAR_BUFFERS_G |
         NV50_3D_CLEAR_BUFFERS_B | NV50_3D_CLEAR_BUFFERS_A;
   }

   if (buffers & PIPE_CLEAR_DEPTH) {
      BEGIN_NV04(push, NV50_3D(CLEAR_DEPTH), 1);
      PUSH_DATA (push, fui(depth));
      mode |= NV50_3D_CLEAR_BUFFERS_Z;
   }

   if (buffers & PIPE_CLEAR_STENCIL) {
      BEGIN_NV04(push, NV50_3D(CLEAR_STENCIL), 1);
      PUSH_DATA (push, stencil & 0xff);
      mode |= NV50_3D_CLEAR_BUFFERS_S;
   }

   BEGIN_NV04(push, NV50_3D(CLEAR_BUFFERS), 1);
   PUSH_DATA (push, mode);

   for (i = 1; i < fb->nr_cbufs; i++) {
      BEGIN_NV04(push, NV50_3D(CLEAR_BUFFERS), 1);
      PUSH_DATA (push, (i << 6) | 0x3c);
   }
}


/* =============================== BLIT CODE ===================================
 */

struct nv50_blitter
{
   struct nv50_program *fp[NV50_BLIT_MAX_TEXTURE_TYPES][NV50_BLIT_MODES];
   struct nv50_program vp;

   struct nv50_tsc_entry sampler[2]; /* nearest, bilinear */

   pipe_mutex mutex;
};

struct nv50_blitctx
{
   struct nv50_context *nv50;
   struct nv50_program *fp;
   uint8_t mode;
   uint16_t color_mask;
   uint8_t filter;
   enum pipe_texture_target target;
   struct {
      struct pipe_framebuffer_state fb;
      struct nv50_program *vp;
      struct nv50_program *gp;
      struct nv50_program *fp;
      unsigned num_textures[3];
      unsigned num_samplers[3];
      struct pipe_sampler_view *texture[2];
      struct nv50_tsc_entry *sampler[2];
      uint32_t dirty;
   } saved;
};

static void
nv50_blitter_make_vp(struct nv50_blitter *blit)
{
   static const uint32_t code[] =
   {
      0x10000001, 0x0423c788, /* mov b32 o[0x00] s[0x00] */ /* HPOS.x */
      0x10000205, 0x0423c788, /* mov b32 o[0x04] s[0x04] */ /* HPOS.y */
      0x10000409, 0x0423c788, /* mov b32 o[0x08] s[0x08] */ /* TEXC.x */
      0x1000060d, 0x0423c788, /* mov b32 o[0x0c] s[0x0c] */ /* TEXC.y */
      0x10000811, 0x0423c789, /* mov b32 o[0x10] s[0x10] */ /* TEXC.z */
   };

   blit->vp.type = PIPE_SHADER_VERTEX;
   blit->vp.translated = TRUE;
   blit->vp.code = (uint32_t *)code; /* const_cast */
   blit->vp.code_size = sizeof(code);
   blit->vp.max_gpr = 4;
   blit->vp.max_out = 5;
   blit->vp.out_nr = 2;
   blit->vp.out[0].mask = 0x3;
   blit->vp.out[0].sn = TGSI_SEMANTIC_POSITION;
   blit->vp.out[1].hw = 2;
   blit->vp.out[1].mask = 0x7;
   blit->vp.out[1].sn = TGSI_SEMANTIC_GENERIC;
   blit->vp.out[1].si = 0;
   blit->vp.vp.attrs[0] = 0x73;
   blit->vp.vp.psiz = 0x40;
   blit->vp.vp.edgeflag = 0x40;
}

void *
nv50_blitter_make_fp(struct pipe_context *pipe,
                     unsigned mode,
                     enum pipe_texture_target ptarg)
{
   struct ureg_program *ureg;
   struct ureg_src tc;
   struct ureg_dst out;
   struct ureg_dst data;

   const unsigned target = nv50_blit_get_tgsi_texture_target(ptarg);

   boolean tex_rgbaz = FALSE;
   boolean tex_s = FALSE;
   boolean cvt_un8 = FALSE;

   if (mode != NV50_BLIT_MODE_PASS &&
       mode != NV50_BLIT_MODE_Z24X8 &&
       mode != NV50_BLIT_MODE_X8Z24)
      tex_s = TRUE;

   if (mode != NV50_BLIT_MODE_X24S8 &&
       mode != NV50_BLIT_MODE_S8X24 &&
       mode != NV50_BLIT_MODE_XS)
      tex_rgbaz = TRUE;

   if (mode != NV50_BLIT_MODE_PASS &&
       mode != NV50_BLIT_MODE_ZS &&
       mode != NV50_BLIT_MODE_XS)
      cvt_un8 = TRUE;

   ureg = ureg_create(TGSI_PROCESSOR_FRAGMENT);
   if (!ureg)
      return NULL;

   out = ureg_DECL_output(ureg, TGSI_SEMANTIC_COLOR, 0);
   tc = ureg_DECL_fs_input(
      ureg, TGSI_SEMANTIC_GENERIC, 0, TGSI_INTERPOLATE_LINEAR);

   data = ureg_DECL_temporary(ureg);

   if (tex_s) {
      ureg_TEX(ureg, ureg_writemask(data, TGSI_WRITEMASK_X),
               target, tc, ureg_DECL_sampler(ureg, 1));
      ureg_MOV(ureg, ureg_writemask(data, TGSI_WRITEMASK_Y),
               ureg_scalar(ureg_src(data), TGSI_SWIZZLE_X));
   }
   if (tex_rgbaz) {
      const unsigned mask = (mode == NV50_BLIT_MODE_PASS) ?
         TGSI_WRITEMASK_XYZW : TGSI_WRITEMASK_X;
      ureg_TEX(ureg, ureg_writemask(data, mask),
               target, tc, ureg_DECL_sampler(ureg, 0));
   }

   if (cvt_un8) {
      struct ureg_src mask;
      struct ureg_src scale;
      struct ureg_dst outz;
      struct ureg_dst outs;
      struct ureg_dst zdst3 = ureg_writemask(data, TGSI_WRITEMASK_XYZ);
      struct ureg_dst zdst = ureg_writemask(data, TGSI_WRITEMASK_X);
      struct ureg_dst sdst = ureg_writemask(data, TGSI_WRITEMASK_Y);
      struct ureg_src zsrc3 = ureg_src(data);
      struct ureg_src zsrc = ureg_scalar(zsrc3, TGSI_SWIZZLE_X);
      struct ureg_src ssrc = ureg_scalar(zsrc3, TGSI_SWIZZLE_Y);
      struct ureg_src zshuf;

      mask = ureg_imm3u(ureg, 0x0000ff, 0x00ff00, 0xff0000);
      scale = ureg_imm4f(ureg,
                         1.0f / 0x0000ff, 1.0f / 0x00ff00, 1.0f / 0xff0000,
                         (1 << 24) - 1);

      if (mode == NV50_BLIT_MODE_Z24S8 ||
          mode == NV50_BLIT_MODE_X24S8 ||
          mode == NV50_BLIT_MODE_Z24X8) {
         outz = ureg_writemask(out, TGSI_WRITEMASK_XYZ);
         outs = ureg_writemask(out, TGSI_WRITEMASK_W);
         zshuf = ureg_src(data);
      } else {
         outz = ureg_writemask(out, TGSI_WRITEMASK_YZW);
         outs = ureg_writemask(out, TGSI_WRITEMASK_X);
         zshuf = ureg_swizzle(zsrc3, TGSI_SWIZZLE_W,
                              TGSI_SWIZZLE_X, TGSI_SWIZZLE_Y, TGSI_SWIZZLE_Z);
      }

      if (tex_s) {
         ureg_I2F(ureg, sdst, ssrc);
         ureg_MUL(ureg, outs, ssrc, ureg_scalar(scale, TGSI_SWIZZLE_X));
      }

      if (tex_rgbaz) {
         ureg_MUL(ureg, zdst, zsrc, ureg_scalar(scale, TGSI_SWIZZLE_W));
         ureg_F2I(ureg, zdst, zsrc);
         ureg_AND(ureg, zdst3, zsrc, mask);
         ureg_I2F(ureg, zdst3, zsrc3);
         ureg_MUL(ureg, zdst3, zsrc3, scale);
         ureg_MOV(ureg, outz, zshuf);
      }
   } else {
      unsigned mask = TGSI_WRITEMASK_XYZW;

      if (mode != NV50_BLIT_MODE_PASS) {
         mask &= ~TGSI_WRITEMASK_ZW;
         if (!tex_s)
            mask = TGSI_WRITEMASK_X;
         if (!tex_rgbaz)
            mask = TGSI_WRITEMASK_Y;
      }
      ureg_MOV(ureg, ureg_writemask(out, mask), ureg_src(data));
   }
   ureg_END(ureg);

   return ureg_create_shader_and_destroy(ureg, pipe);
}

static void
nv50_blitter_make_sampler(struct nv50_blitter *blit)
{
   /* clamp to edge, min/max lod = 0, nearest filtering */

   blit->sampler[0].id = -1;

   blit->sampler[0].tsc[0] = NV50_TSC_0_SRGB_CONVERSION_ALLOWED |
      (NV50_TSC_WRAP_CLAMP_TO_EDGE << NV50_TSC_0_WRAPS__SHIFT) |
      (NV50_TSC_WRAP_CLAMP_TO_EDGE << NV50_TSC_0_WRAPT__SHIFT) |
      (NV50_TSC_WRAP_CLAMP_TO_EDGE << NV50_TSC_0_WRAPR__SHIFT);
   blit->sampler[0].tsc[1] =
      NV50_TSC_1_MAGF_NEAREST | NV50_TSC_1_MINF_NEAREST | NV50_TSC_1_MIPF_NONE;

   /* clamp to edge, min/max lod = 0, bilinear filtering */

   blit->sampler[1].id = -1;

   blit->sampler[1].tsc[0] = blit->sampler[0].tsc[0];
   blit->sampler[1].tsc[1] =
      NV50_TSC_1_MAGF_LINEAR | NV50_TSC_1_MINF_LINEAR | NV50_TSC_1_MIPF_NONE;
}

unsigned
nv50_blit_select_mode(const struct pipe_blit_info *info)
{
   const unsigned mask = info->mask;

   switch (info->dst.resource->format) {
   case PIPE_FORMAT_Z24_UNORM_S8_UINT:
   case PIPE_FORMAT_Z24X8_UNORM:
      switch (mask & PIPE_MASK_ZS) {
      case PIPE_MASK_ZS: return NV50_BLIT_MODE_Z24S8;
      case PIPE_MASK_Z:  return NV50_BLIT_MODE_Z24X8;
      default:
         return NV50_BLIT_MODE_X24S8;
      }
   case PIPE_FORMAT_S8_UINT_Z24_UNORM:
      switch (mask & PIPE_MASK_ZS) {
      case PIPE_MASK_ZS: return NV50_BLIT_MODE_S8Z24;
      case PIPE_MASK_Z:  return NV50_BLIT_MODE_X8Z24;
      default:
         return NV50_BLIT_MODE_S8X24;
      }
   case PIPE_FORMAT_Z32_FLOAT:
   case PIPE_FORMAT_Z32_FLOAT_S8X24_UINT:
      switch (mask & PIPE_MASK_ZS) {
      case PIPE_MASK_ZS: return NV50_BLIT_MODE_ZS;
      case PIPE_MASK_Z:  return NV50_BLIT_MODE_PASS;
      default:
         return NV50_BLIT_MODE_XS;
      }
   default:
      return NV50_BLIT_MODE_PASS;
   }
}

static void
nv50_blit_select_fp(struct nv50_blitctx *ctx, const struct pipe_blit_info *info)
{
   struct nv50_blitter *blitter = ctx->nv50->screen->blitter;

   const enum pipe_texture_target ptarg =
      nv50_blit_reinterpret_pipe_texture_target(info->src.resource->target);

   const unsigned targ = nv50_blit_texture_type(ptarg);
   const unsigned mode = ctx->mode;

   if (!blitter->fp[targ][mode]) {
      pipe_mutex_lock(blitter->mutex);
      if (!blitter->fp[targ][mode])
         blitter->fp[targ][mode] =
            nv50_blitter_make_fp(&ctx->nv50->base.pipe, mode, ptarg);
      pipe_mutex_unlock(blitter->mutex);
   }
   ctx->fp = blitter->fp[targ][mode];
}

static void
nv50_blit_set_dst(struct nv50_blitctx *ctx,
                  struct pipe_resource *res, unsigned level, unsigned layer,
                  enum pipe_format format)
{
   struct nv50_context *nv50 = ctx->nv50;
   struct pipe_context *pipe = &nv50->base.pipe;
   struct pipe_surface templ;

   if (util_format_is_depth_or_stencil(format))
      templ.format = nv50_blit_zeta_to_colour_format(format);
   else
      templ.format = format;

   templ.u.tex.level = level;
   templ.u.tex.first_layer = templ.u.tex.last_layer = layer;

   if (layer == -1) {
      templ.u.tex.first_layer = 0;
      templ.u.tex.last_layer =
         (res->target == PIPE_TEXTURE_3D ? res->depth0 : res->array_size) - 1;
   }

   nv50->framebuffer.cbufs[0] = nv50_miptree_surface_new(pipe, res, &templ);
   nv50->framebuffer.nr_cbufs = 1;
   nv50->framebuffer.zsbuf = NULL;
   nv50->framebuffer.width = nv50->framebuffer.cbufs[0]->width;
   nv50->framebuffer.height = nv50->framebuffer.cbufs[0]->height;
}

static void
nv50_blit_set_src(struct nv50_blitctx *blit,
                  struct pipe_resource *res, unsigned level, unsigned layer,
                  enum pipe_format format, const uint8_t filter)
{
   struct nv50_context *nv50 = blit->nv50;
   struct pipe_context *pipe = &nv50->base.pipe;
   struct pipe_sampler_view templ;
   uint32_t flags;
   enum pipe_texture_target target;

   target = nv50_blit_reinterpret_pipe_texture_target(res->target);

   templ.format = format;
   templ.u.tex.first_level = templ.u.tex.last_level = level;
   templ.u.tex.first_layer = templ.u.tex.last_layer = layer;
   templ.swizzle_r = PIPE_SWIZZLE_RED;
   templ.swizzle_g = PIPE_SWIZZLE_GREEN;
   templ.swizzle_b = PIPE_SWIZZLE_BLUE;
   templ.swizzle_a = PIPE_SWIZZLE_ALPHA;

   if (layer == -1) {
      templ.u.tex.first_layer = 0;
      templ.u.tex.last_layer =
         (res->target == PIPE_TEXTURE_3D ? res->depth0 : res->array_size) - 1;
   }

   flags = res->last_level ? 0 : NV50_TEXVIEW_SCALED_COORDS;
   if (filter && res->nr_samples == 8)
      flags |= NV50_TEXVIEW_FILTER_MSAA8;

   nv50->textures[2][0] = nv50_create_texture_view(
      pipe, res, &templ, flags, target);
   nv50->textures[2][1] = NULL;

   nv50->num_textures[0] = nv50->num_textures[1] = 0;
   nv50->num_textures[2] = 1;

   templ.format = nv50_zs_to_s_format(format);
   if (templ.format != res->format) {
      nv50->textures[2][1] = nv50_create_texture_view(
         pipe, res, &templ, flags, target);
      nv50->num_textures[2] = 2;
   }
}

static void
nv50_blitctx_prepare_state(struct nv50_blitctx *blit)
{
   struct nouveau_pushbuf *push = blit->nv50->base.pushbuf;

   if (blit->nv50->cond_query) {
      BEGIN_NV04(push, NV50_3D(COND_MODE), 1);
      PUSH_DATA (push, NV50_3D_COND_MODE_ALWAYS);
   }

   /* blend state */
   BEGIN_NV04(push, NV50_3D(COLOR_MASK(0)), 1);
   PUSH_DATA (push, blit->color_mask);
   BEGIN_NV04(push, NV50_3D(BLEND_ENABLE(0)), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(LOGIC_OP_ENABLE), 1);
   PUSH_DATA (push, 0);

   /* rasterizer state */
#ifndef NV50_SCISSORS_CLIPPING
   BEGIN_NV04(push, NV50_3D(SCISSOR_ENABLE(0)), 1);
   PUSH_DATA (push, 1);
#endif
   BEGIN_NV04(push, NV50_3D(VERTEX_TWO_SIDE_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(FRAG_COLOR_CLAMP_EN), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(MULTISAMPLE_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(MSAA_MASK(0)), 4);
   PUSH_DATA (push, 0xffff);
   PUSH_DATA (push, 0xffff);
   PUSH_DATA (push, 0xffff);
   PUSH_DATA (push, 0xffff);
   BEGIN_NV04(push, NV50_3D(POLYGON_MODE_FRONT), 3);
   PUSH_DATA (push, NV50_3D_POLYGON_MODE_FRONT_FILL);
   PUSH_DATA (push, NV50_3D_POLYGON_MODE_BACK_FILL);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(CULL_FACE_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(POLYGON_STIPPLE_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(POLYGON_OFFSET_FILL_ENABLE), 1);
   PUSH_DATA (push, 0);

   /* zsa state */
   BEGIN_NV04(push, NV50_3D(DEPTH_TEST_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(STENCIL_ENABLE), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(ALPHA_TEST_ENABLE), 1);
   PUSH_DATA (push, 0);
}

static void
nv50_blitctx_pre_blit(struct nv50_blitctx *ctx)
{
   struct nv50_context *nv50 = ctx->nv50;
   struct nv50_blitter *blitter = nv50->screen->blitter;
   int s;

   ctx->saved.fb.width = nv50->framebuffer.width;
   ctx->saved.fb.height = nv50->framebuffer.height;
   ctx->saved.fb.nr_cbufs = nv50->framebuffer.nr_cbufs;
   ctx->saved.fb.cbufs[0] = nv50->framebuffer.cbufs[0];
   ctx->saved.fb.zsbuf = nv50->framebuffer.zsbuf;

   ctx->saved.vp = nv50->vertprog;
   ctx->saved.gp = nv50->gmtyprog;
   ctx->saved.fp = nv50->fragprog;

   nv50->vertprog = &blitter->vp;
   nv50->gmtyprog = NULL;
   nv50->fragprog = ctx->fp;

   for (s = 0; s < 3; ++s) {
      ctx->saved.num_textures[s] = nv50->num_textures[s];
      ctx->saved.num_samplers[s] = nv50->num_samplers[s];
   }
   ctx->saved.texture[0] = nv50->textures[2][0];
   ctx->saved.texture[1] = nv50->textures[2][1];
   ctx->saved.sampler[0] = nv50->samplers[2][0];
   ctx->saved.sampler[1] = nv50->samplers[2][1];

   nv50->samplers[2][0] = &blitter->sampler[ctx->filter];
   nv50->samplers[2][1] = &blitter->sampler[ctx->filter];

   nv50->num_samplers[0] = nv50->num_samplers[1] = 0;
   nv50->num_samplers[2] = 2;

   ctx->saved.dirty = nv50->dirty;

   nouveau_bufctx_reset(nv50->bufctx_3d, NV50_BIND_FB);
   nouveau_bufctx_reset(nv50->bufctx_3d, NV50_BIND_TEXTURES);

   nv50->dirty =
      NV50_NEW_FRAMEBUFFER |
      NV50_NEW_VERTPROG | NV50_NEW_FRAGPROG | NV50_NEW_GMTYPROG |
      NV50_NEW_TEXTURES | NV50_NEW_SAMPLERS;
}

static void
nv50_blitctx_post_blit(struct nv50_blitctx *blit)
{
   struct nv50_context *nv50 = blit->nv50;
   int s;

   pipe_surface_reference(&nv50->framebuffer.cbufs[0], NULL);

   nv50->framebuffer.width = blit->saved.fb.width;
   nv50->framebuffer.height = blit->saved.fb.height;
   nv50->framebuffer.nr_cbufs = blit->saved.fb.nr_cbufs;
   nv50->framebuffer.cbufs[0] = blit->saved.fb.cbufs[0];
   nv50->framebuffer.zsbuf = blit->saved.fb.zsbuf;

   nv50->vertprog = blit->saved.vp;
   nv50->gmtyprog = blit->saved.gp;
   nv50->fragprog = blit->saved.fp;

   pipe_sampler_view_reference(&nv50->textures[2][0], NULL);
   pipe_sampler_view_reference(&nv50->textures[2][1], NULL);

   for (s = 0; s < 3; ++s) {
      nv50->num_textures[s] = blit->saved.num_textures[s];
      nv50->num_samplers[s] = blit->saved.num_samplers[s];
   }
   nv50->textures[2][0] = blit->saved.texture[0];
   nv50->textures[2][1] = blit->saved.texture[1];
   nv50->samplers[2][0] = blit->saved.sampler[0];
   nv50->samplers[2][1] = blit->saved.sampler[1];

   if (nv50->cond_query)
      nv50->base.pipe.render_condition(&nv50->base.pipe, nv50->cond_query,
                                       nv50->cond_mode);

   nouveau_bufctx_reset(nv50->bufctx_3d, NV50_BIND_FB);
   nouveau_bufctx_reset(nv50->bufctx_3d, NV50_BIND_TEXTURES);

   nv50->dirty = blit->saved.dirty |
      (NV50_NEW_FRAMEBUFFER | NV50_NEW_SCISSOR | NV50_NEW_SAMPLE_MASK |
       NV50_NEW_RASTERIZER | NV50_NEW_ZSA | NV50_NEW_BLEND |
       NV50_NEW_TEXTURES | NV50_NEW_SAMPLERS |
       NV50_NEW_VERTPROG | NV50_NEW_GMTYPROG | NV50_NEW_FRAGPROG);
}


static void
nv50_blit_3d(struct nv50_context *nv50, const struct pipe_blit_info *info)
{
   struct nv50_blitctx *blit = nv50->blit;
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct pipe_resource *src = info->src.resource;
   struct pipe_resource *dst = info->dst.resource;
   int32_t minx, maxx, miny, maxy;
   int32_t i;
   float x0, x1, y0, y1, z;
   float dz;
   float x_range, y_range;

   blit->mode = nv50_blit_select_mode(info);
   blit->color_mask = nv50_blit_derive_color_mask(info);
   blit->filter = nv50_blit_get_filter(info);

   nv50_blit_select_fp(blit, info);
   nv50_blitctx_pre_blit(blit);

   nv50_blit_set_dst(blit, dst, info->dst.level, -1, info->dst.format);
   nv50_blit_set_src(blit, src, info->src.level, -1, info->src.format,
                     blit->filter);

   nv50_blitctx_prepare_state(blit);

   nv50_state_validate(nv50, ~0, 36);

   x_range = (float)info->src.box.width / (float)info->dst.box.width;
   y_range = (float)info->src.box.height / (float)info->dst.box.height;

   x0 = (float)info->src.box.x - x_range * (float)info->dst.box.x;
   y0 = (float)info->src.box.y - y_range * (float)info->dst.box.y;

   x1 = x0 + 16384.0f * x_range;
   y1 = y0 + 16384.0f * y_range;

   x0 *= (float)(1 << nv50_miptree(src)->ms_x);
   x1 *= (float)(1 << nv50_miptree(src)->ms_x);
   y0 *= (float)(1 << nv50_miptree(src)->ms_y);
   y1 *= (float)(1 << nv50_miptree(src)->ms_y);

   if (src->last_level > 0) {
      /* If there are mip maps, GPU always assumes normalized coordinates. */
      const unsigned l = info->src.level;
      const float fh = u_minify(src->width0 << nv50_miptree(src)->ms_x, l);
      const float fv = u_minify(src->height0 << nv50_miptree(src)->ms_y, l);
      x0 /= fh;
      x1 /= fh;
      y0 /= fv;
      y1 /= fv;
   }

   /* XXX: multiply by 6 for cube arrays ? */
   dz = (float)info->src.box.depth / (float)info->dst.box.depth;
   z = (float)info->src.box.z;
   if (nv50_miptree(src)->layout_3d)
      z += 0.5f * dz;

   BEGIN_NV04(push, NV50_3D(VIEWPORT_TRANSFORM_EN), 1);
   PUSH_DATA (push, 0);
   BEGIN_NV04(push, NV50_3D(VIEW_VOLUME_CLIP_CTRL), 1);
   PUSH_DATA (push, 0x1);

   /* Draw a large triangle in screen coordinates covering the whole
    * render target, with scissors defining the destination region.
    * The vertex is supplied with non-normalized texture coordinates
    * arranged in a way to yield the desired offset and scale.
    */

   minx = info->dst.box.x;
   maxx = info->dst.box.x + info->dst.box.width;
   miny = info->dst.box.y;
   maxy = info->dst.box.y + info->dst.box.height;
   if (info->scissor_enable) {
      minx = MAX2(minx, info->scissor.minx);
      maxx = MIN2(maxx, info->scissor.maxx);
      miny = MAX2(miny, info->scissor.miny);
      maxy = MIN2(maxy, info->scissor.maxy);
   }
   BEGIN_NV04(push, NV50_3D(SCISSOR_HORIZ(0)), 2);
   PUSH_DATA (push, (maxx << 16) | minx);
   PUSH_DATA (push, (maxy << 16) | miny);

   for (i = 0; i < info->dst.box.depth; ++i, z += dz) {
      if (info->dst.box.z + i) {
         BEGIN_NV04(push, NV50_3D(LAYER), 1);
         PUSH_DATA (push, info->dst.box.z + i);
      }
      PUSH_SPACE(push, 32);
      BEGIN_NV04(push, NV50_3D(VERTEX_BEGIN_GL), 1);
      PUSH_DATA (push, NV50_3D_VERTEX_BEGIN_GL_PRIMITIVE_TRIANGLES);
      BEGIN_NV04(push, NV50_3D(VTX_ATTR_3F_X(1)), 3);
      PUSH_DATAf(push, x0);
      PUSH_DATAf(push, y0);
      PUSH_DATAf(push, z);
      BEGIN_NV04(push, NV50_3D(VTX_ATTR_2F_X(0)), 2);
      PUSH_DATAf(push, 0.0f);
      PUSH_DATAf(push, 0.0f);
      BEGIN_NV04(push, NV50_3D(VTX_ATTR_3F_X(1)), 3);
      PUSH_DATAf(push, x1);
      PUSH_DATAf(push, y0);
      PUSH_DATAf(push, z);
      BEGIN_NV04(push, NV50_3D(VTX_ATTR_2F_X(0)), 2);
      PUSH_DATAf(push, 16384 << nv50_miptree(dst)->ms_x);
      PUSH_DATAf(push, 0.0f);
      BEGIN_NV04(push, NV50_3D(VTX_ATTR_3F_X(1)), 3);
      PUSH_DATAf(push, x0);
      PUSH_DATAf(push, y1);
      PUSH_DATAf(push, z);
      BEGIN_NV04(push, NV50_3D(VTX_ATTR_2F_X(0)), 2);
      PUSH_DATAf(push, 0.0f);
      PUSH_DATAf(push, 16384 << nv50_miptree(dst)->ms_y);
      BEGIN_NV04(push, NV50_3D(VERTEX_END_GL), 1);
      PUSH_DATA (push, 0);
   }
   if (info->dst.box.z + info->dst.box.depth - 1) {
      BEGIN_NV04(push, NV50_3D(LAYER), 1);
      PUSH_DATA (push, 0);
   }

   /* re-enable normally constant state */

   BEGIN_NV04(push, NV50_3D(VIEWPORT_TRANSFORM_EN), 1);
   PUSH_DATA (push, 1);

   nv50_blitctx_post_blit(blit);
}

static void
nv50_blit_eng2d(struct nv50_context *nv50, const struct pipe_blit_info *info)
{
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct nv50_miptree *dst = nv50_miptree(info->dst.resource);
   struct nv50_miptree *src = nv50_miptree(info->src.resource);
   const int32_t srcx_adj = info->src.box.width < 0 ? -1 : 0;
   const int32_t srcy_adj = info->src.box.height < 0 ? -1 : 0;
   const int32_t dz = info->dst.box.z;
   const int32_t sz = info->src.box.z;
   uint32_t dstw, dsth;
   int32_t dstx, dsty;
   int64_t srcx, srcy;
   int64_t du_dx, dv_dy;
   int i;
   uint32_t mode;
   uint32_t mask = nv50_blit_eng2d_get_mask(info);
   boolean b;

   mode = nv50_blit_get_filter(info) ?
      NV50_2D_BLIT_CONTROL_FILTER_BILINEAR :
      NV50_2D_BLIT_CONTROL_FILTER_POINT_SAMPLE;
   mode |= (src->base.base.nr_samples > dst->base.base.nr_samples) ?
      NV50_2D_BLIT_CONTROL_ORIGIN_CORNER : NV50_2D_BLIT_CONTROL_ORIGIN_CENTER;

   du_dx = ((int64_t)info->src.box.width << 32) / info->dst.box.width;
   dv_dy = ((int64_t)info->src.box.height << 32) / info->dst.box.height;

   b = info->dst.format == info->src.format;
   nv50_2d_texture_set(push, 1, dst, info->dst.level, dz, info->dst.format, b);
   nv50_2d_texture_set(push, 0, src, info->src.level, sz, info->src.format, b);

   if (info->scissor_enable) {
      BEGIN_NV04(push, NV50_2D(CLIP_X), 5);
      PUSH_DATA (push, info->scissor.minx << dst->ms_x);
      PUSH_DATA (push, info->scissor.miny << dst->ms_y);
      PUSH_DATA (push, (info->scissor.maxx - info->scissor.minx) << dst->ms_x);
      PUSH_DATA (push, (info->scissor.maxy - info->scissor.miny) << dst->ms_y);
      PUSH_DATA (push, 1); /* enable */
   }

   if (mask != 0xffffffff) {
      BEGIN_NV04(push, NV50_2D(ROP), 1);
      PUSH_DATA (push, 0xca); /* DPSDxax */
      BEGIN_NV04(push, NV50_2D(PATTERN_COLOR_FORMAT), 1);
      PUSH_DATA (push, NV50_2D_PATTERN_COLOR_FORMAT_32BPP);
      BEGIN_NV04(push, NV50_2D(PATTERN_COLOR(0)), 4);
      PUSH_DATA (push, 0x00000000);
      PUSH_DATA (push, mask);
      PUSH_DATA (push, 0xffffffff);
      PUSH_DATA (push, 0xffffffff);
      BEGIN_NV04(push, NV50_2D(OPERATION), 1);
      PUSH_DATA (push, NV50_2D_OPERATION_ROP);
   } else
   if (info->src.format != info->dst.format) {
      if (info->src.format == PIPE_FORMAT_R8_UNORM ||
          info->src.format == PIPE_FORMAT_R16_UNORM ||
          info->src.format == PIPE_FORMAT_R16_FLOAT ||
          info->src.format == PIPE_FORMAT_R32_FLOAT) {
         mask = 0xffff0000; /* also makes condition for OPERATION reset true */
         BEGIN_NV04(push, NV50_2D(BETA4), 2);
         PUSH_DATA (push, mask);
         PUSH_DATA (push, NV50_2D_OPERATION_SRCCOPY_PREMULT);
      }
   }

   if (src->ms_x > dst->ms_x || src->ms_y > dst->ms_y) {
      /* ms_x is always >= ms_y */
      du_dx <<= src->ms_x - dst->ms_x;
      dv_dy <<= src->ms_y - dst->ms_y;
   } else {
      du_dx >>= dst->ms_x - src->ms_x;
      dv_dy >>= dst->ms_y - src->ms_y;
   }

   srcx = (int64_t)(info->src.box.x + srcx_adj) << (src->ms_x + 32);
   srcy = (int64_t)(info->src.box.y + srcy_adj) << (src->ms_y + 32);

   if (src->base.base.nr_samples > dst->base.base.nr_samples) {
      /* center src coorinates for proper MS resolve filtering */
      srcx += (int64_t)src->ms_x << 32;
      srcy += (int64_t)src->ms_y << 32;
   }

   dstx = info->dst.box.x << dst->ms_x;
   dsty = info->dst.box.y << dst->ms_y;

   dstw = info->dst.box.width << dst->ms_x;
   dsth = info->dst.box.height << dst->ms_y;

   if (dstx < 0) {
      dstw += dstx;
      srcx -= du_dx * dstx;
      dstx = 0;
   }
   if (dsty < 0) {
      dsth += dsty;
      srcy -= dv_dy * dsty;
      dsty = 0;
   }

   BEGIN_NV04(push, NV50_2D(BLIT_CONTROL), 1);
   PUSH_DATA (push, mode);
   BEGIN_NV04(push, NV50_2D(BLIT_DST_X), 4);
   PUSH_DATA (push, dstx);
   PUSH_DATA (push, dsty);
   PUSH_DATA (push, dstw);
   PUSH_DATA (push, dsth);
   BEGIN_NV04(push, NV50_2D(BLIT_DU_DX_FRACT), 4);
   PUSH_DATA (push, du_dx);
   PUSH_DATA (push, du_dx >> 32);
   PUSH_DATA (push, dv_dy);
   PUSH_DATA (push, dv_dy >> 32);

   BCTX_REFN(nv50->bufctx, 2D, &dst->base, WR);
   BCTX_REFN(nv50->bufctx, 2D, &src->base, RD);
   nouveau_pushbuf_bufctx(nv50->base.pushbuf, nv50->bufctx);
   if (nouveau_pushbuf_validate(nv50->base.pushbuf))
      return;

   for (i = 0; i < info->dst.box.depth; ++i) {
      if (i > 0) {
         /* no scaling in z-direction possible for eng2d blits */
         if (dst->layout_3d) {
            BEGIN_NV04(push, NV50_2D(DST_LAYER), 1);
            PUSH_DATA (push, info->dst.box.z + i);
         } else {
            const unsigned z = info->dst.box.z + i;
            BEGIN_NV04(push, NV50_2D(DST_ADDRESS_HIGH), 2);
            PUSH_DATAh(push, dst->base.address + z * dst->layer_stride);
            PUSH_DATA (push, dst->base.address + z * dst->layer_stride);
         }
         if (src->layout_3d) {
            /* not possible because of depth tiling */
            assert(0);
         } else {
            const unsigned z = info->src.box.z + i;
            BEGIN_NV04(push, NV50_2D(SRC_ADDRESS_HIGH), 2);
            PUSH_DATAh(push, src->base.address + z * src->layer_stride);
            PUSH_DATA (push, src->base.address + z * src->layer_stride);
         }
         BEGIN_NV04(push, NV50_2D(BLIT_SRC_Y_INT), 1); /* trigger */
         PUSH_DATA (push, srcy >> 32);
      } else {
         BEGIN_NV04(push, NV50_2D(BLIT_SRC_X_FRACT), 4);
         PUSH_DATA (push, srcx);
         PUSH_DATA (push, srcx >> 32);
         PUSH_DATA (push, srcy);
         PUSH_DATA (push, srcy >> 32);
      }
   }
   nv50_bufctx_fence(nv50->bufctx, FALSE);

   nouveau_bufctx_reset(nv50->bufctx, NV50_BIND_2D);

   if (info->scissor_enable) {
      BEGIN_NV04(push, NV50_2D(CLIP_ENABLE), 1);
      PUSH_DATA (push, 0);
   }
   if (mask != 0xffffffff) {
      BEGIN_NV04(push, NV50_2D(OPERATION), 1);
      PUSH_DATA (push, NV50_2D_OPERATION_SRCCOPY);
   }
}

static void
nv50_blit(struct pipe_context *pipe, const struct pipe_blit_info *info)
{
   struct nv50_context *nv50 = nv50_context(pipe);
   boolean eng3d = FALSE;

   if (util_format_is_depth_or_stencil(info->dst.resource->format)) {
      if (!(info->mask & PIPE_MASK_ZS))
         return;
      if (info->dst.resource->format == PIPE_FORMAT_Z32_FLOAT ||
          info->dst.resource->format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT)
         eng3d = TRUE;
      if (info->filter != PIPE_TEX_FILTER_NEAREST)
         eng3d = TRUE;
   } else {
      if (!(info->mask & PIPE_MASK_RGBA))
         return;
      if (info->mask != PIPE_MASK_RGBA)
         eng3d = TRUE;
   }

   if (nv50_miptree(info->src.resource)->layout_3d) {
      eng3d = TRUE;
   } else
   if (info->src.box.depth != info->dst.box.depth) {
      eng3d = TRUE;
      debug_printf("blit: cannot filter array or cube textures in z direction");
   }

   if (!eng3d && info->dst.format != info->src.format) {
      if (!nv50_2d_dst_format_faithful(info->dst.format) ||
          !nv50_2d_src_format_faithful(info->src.format)) {
         eng3d = TRUE;
      } else
      if (!nv50_2d_src_format_faithful(info->src.format)) {
         if (!util_format_is_luminance(info->src.format)) {
            if (util_format_is_intensity(info->src.format))
               eng3d = TRUE;
            else
            if (!nv50_2d_dst_format_ops_supported(info->dst.format))
               eng3d = TRUE;
            else
               eng3d = !nv50_2d_format_supported(info->src.format);
         }
      } else
      if (util_format_is_luminance_alpha(info->src.format))
         eng3d = TRUE;
   }

   if (info->src.resource->nr_samples == 8 &&
       info->dst.resource->nr_samples <= 1)
      eng3d = TRUE;

   /* FIXME: can't make this work with eng2d anymore */
   if (info->src.resource->nr_samples > 1 ||
       info->dst.resource->nr_samples > 1)
      eng3d = TRUE;

   /* FIXME: find correct src coordinate adjustments */
   if ((info->src.box.width !=  info->dst.box.width &&
        info->src.box.width != -info->dst.box.width) ||
       (info->src.box.height !=  info->dst.box.height &&
        info->src.box.height != -info->dst.box.height))
      eng3d = TRUE;

   if (!eng3d)
      nv50_blit_eng2d(nv50, info);
   else
      nv50_blit_3d(nv50, info);
}

boolean
nv50_blitter_create(struct nv50_screen *screen)
{
   screen->blitter = CALLOC_STRUCT(nv50_blitter);
   if (!screen->blitter) {
      NOUVEAU_ERR("failed to allocate blitter struct\n");
      return FALSE;
   }

   pipe_mutex_init(screen->blitter->mutex);

   nv50_blitter_make_vp(screen->blitter);
   nv50_blitter_make_sampler(screen->blitter);

   return TRUE;
}

void
nv50_blitter_destroy(struct nv50_screen *screen)
{
   struct nv50_blitter *blitter = screen->blitter;
   unsigned i, m;

   for (i = 0; i < NV50_BLIT_MAX_TEXTURE_TYPES; ++i) {
      for (m = 0; m < NV50_BLIT_MODES; ++m) {
         struct nv50_program *prog = blitter->fp[i][m];
         if (prog) {
            nv50_program_destroy(NULL, prog);
            FREE((void *)prog->pipe.tokens);
            FREE(prog);
         }
      }
   }

   FREE(blitter);
}

boolean
nv50_blitctx_create(struct nv50_context *nv50)
{
   nv50->blit = CALLOC_STRUCT(nv50_blitctx);
   if (!nv50->blit) {
      NOUVEAU_ERR("failed to allocate blit context\n");
      return FALSE;
   }

   nv50->blit->nv50 = nv50;

   return TRUE;
}

void
nv50_init_surface_functions(struct nv50_context *nv50)
{
   struct pipe_context *pipe = &nv50->base.pipe;

   pipe->resource_copy_region = nv50_resource_copy_region;
   pipe->blit = nv50_blit;
   pipe->clear_render_target = nv50_clear_render_target;
   pipe->clear_depth_stencil = nv50_clear_depth_stencil;
}


