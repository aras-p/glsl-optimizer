/*
 * Copyright 2008 Ben Skeggs
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

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"

#include "nv50_context.h"

void
nv50_constbufs_validate(struct nv50_context *nv50)
{
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   unsigned s;

   for (s = 0; s < 3; ++s) {
      struct nv04_resource *res;
      int i;
      unsigned p, b;

      if (s == PIPE_SHADER_FRAGMENT)
         p = NV50_3D_SET_PROGRAM_CB_PROGRAM_FRAGMENT;
      else
      if (s == PIPE_SHADER_GEOMETRY)
         p = NV50_3D_SET_PROGRAM_CB_PROGRAM_GEOMETRY;
      else
         p = NV50_3D_SET_PROGRAM_CB_PROGRAM_VERTEX;

      while (nv50->constbuf_dirty[s]) {
         struct nouveau_bo *bo;
         unsigned start = 0;
         unsigned words = 0;

         i = ffs(nv50->constbuf_dirty[s]) - 1;
         nv50->constbuf_dirty[s] &= ~(1 << i);

         res = nv04_resource(nv50->constbuf[s][i]);
         if (!res) {
            if (i != 0) {
               BEGIN_NV04(push, NV50_3D(SET_PROGRAM_CB), 1);
               PUSH_DATA (push, (i << 8) | p | 0);
            }
            continue;
         }

         if (i == 0) {
            b = NV50_CB_PVP + s;

            /* always upload GL uniforms through CB DATA */
            bo = nv50->screen->uniforms;
            words = res->base.width0 / 4;
         } else {
            b = s * 16 + i;

            assert(0);

            if (!nouveau_resource_mapped_by_gpu(&res->base)) {
               nouveau_buffer_migrate(&nv50->base, res, NOUVEAU_BO_VRAM);

               BEGIN_NV04(push, NV50_3D(CODE_CB_FLUSH), 1);
               PUSH_DATA (push, 0);
            }
            BEGIN_NV04(push, NV50_3D(CB_DEF_ADDRESS_HIGH), 3);
            PUSH_DATAh(push, res->address);
            PUSH_DATA (push, res->address);
            PUSH_DATA (push, (b << 16) | (res->base.width0 & 0xffff));
            BEGIN_NV04(push, NV50_3D(SET_PROGRAM_CB), 1);
            PUSH_DATA (push, (b << 12) | (i << 8) | p | 1);

            bo = res->bo;
         }

         if (bo != nv50->screen->uniforms)
            BCTX_REFN(nv50->bufctx_3d, CB(s, i), res, RD);

         while (words) {
            unsigned nr;

            if (!PUSH_SPACE(push, 16))
               break;
            nr = PUSH_AVAIL(push);
            assert(nr >= 16);
            nr = MIN2(MIN2(nr - 3, words), NV04_PFIFO_MAX_PACKET_LEN);

            BEGIN_NV04(push, NV50_3D(CB_ADDR), 1);
            PUSH_DATA (push, (start << 8) | b);
            BEGIN_NI04(push, NV50_3D(CB_DATA(0)), nr);
            PUSH_DATAp(push, &res->data[start * 4], nr);

            start += nr;
            words -= nr;
         }
      }
   }
}

static boolean
nv50_program_validate(struct nv50_context *nv50, struct nv50_program *prog)
{
   if (!prog->translated) {
      prog->translated = nv50_program_translate(
         prog, nv50->screen->base.device->chipset);
      if (!prog->translated)
         return FALSE;
   } else
   if (prog->mem)
      return TRUE;

   return nv50_program_upload_code(nv50, prog);
}

static INLINE void
nv50_program_update_context_state(struct nv50_context *nv50,
                                  struct nv50_program *prog, int stage)
{
   const unsigned flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_RDWR;

   if (prog && prog->uses_lmem) {
      if (!nv50->state.tls_required)
         BCTX_REFN_bo(nv50->bufctx_3d, TLS, flags, nv50->screen->tls_bo);
      nv50->state.tls_required |= 1 << stage;
   } else {
      if (nv50->state.tls_required == (1 << stage))
         nouveau_bufctx_reset(nv50->bufctx_3d, NV50_BIND_TLS);
      nv50->state.tls_required &= ~(1 << stage);
   }
}

void
nv50_vertprog_validate(struct nv50_context *nv50)
{
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct nv50_program *vp = nv50->vertprog;

   if (!nv50_program_validate(nv50, vp))
         return;
   nv50_program_update_context_state(nv50, vp, 0);

   BEGIN_NV04(push, NV50_3D(VP_ATTR_EN(0)), 2);
   PUSH_DATA (push, vp->vp.attrs[0]);
   PUSH_DATA (push, vp->vp.attrs[1]);
   BEGIN_NV04(push, NV50_3D(VP_REG_ALLOC_RESULT), 1);
   PUSH_DATA (push, vp->max_out);
   BEGIN_NV04(push, NV50_3D(VP_REG_ALLOC_TEMP), 1);
   PUSH_DATA (push, vp->max_gpr);
   BEGIN_NV04(push, NV50_3D(VP_START_ID), 1);
   PUSH_DATA (push, vp->code_base);
}

void
nv50_fragprog_validate(struct nv50_context *nv50)
{
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct nv50_program *fp = nv50->fragprog;

   if (!nv50_program_validate(nv50, fp))
         return;
   nv50_program_update_context_state(nv50, fp, 1);

   BEGIN_NV04(push, NV50_3D(FP_REG_ALLOC_TEMP), 1);
   PUSH_DATA (push, fp->max_gpr);
   BEGIN_NV04(push, NV50_3D(FP_RESULT_COUNT), 1);
   PUSH_DATA (push, fp->max_out);
   BEGIN_NV04(push, NV50_3D(FP_CONTROL), 1);
   PUSH_DATA (push, fp->fp.flags[0]);
   BEGIN_NV04(push, NV50_3D(FP_CTRL_UNK196C), 1);
   PUSH_DATA (push, fp->fp.flags[1]);
   BEGIN_NV04(push, NV50_3D(FP_START_ID), 1);
   PUSH_DATA (push, fp->code_base);
}

void
nv50_gmtyprog_validate(struct nv50_context *nv50)
{
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct nv50_program *gp = nv50->gmtyprog;

   if (gp) {
      BEGIN_NV04(push, NV50_3D(GP_REG_ALLOC_TEMP), 1);
      PUSH_DATA (push, gp->max_gpr);
      BEGIN_NV04(push, NV50_3D(GP_REG_ALLOC_RESULT), 1);
      PUSH_DATA (push, gp->max_out);
      BEGIN_NV04(push, NV50_3D(GP_OUTPUT_PRIMITIVE_TYPE), 1);
      PUSH_DATA (push, gp->gp.prim_type);
      BEGIN_NV04(push, NV50_3D(GP_VERTEX_OUTPUT_COUNT), 1);
      PUSH_DATA (push, gp->gp.vert_count);
      BEGIN_NV04(push, NV50_3D(GP_START_ID), 1);
      PUSH_DATA (push, gp->code_base);
   }
   nv50_program_update_context_state(nv50, gp, 2);

   /* GP_ENABLE is updated in linkage validation */
}

static void
nv50_sprite_coords_validate(struct nv50_context *nv50)
{
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   uint32_t pntc[8], mode;
   struct nv50_program *fp = nv50->fragprog;
   unsigned i, c;
   unsigned m = (nv50->state.interpolant_ctrl >> 8) & 0xff;

   if (!nv50->rast->pipe.point_quad_rasterization) {
      if (nv50->state.point_sprite) {
         BEGIN_NV04(push, NV50_3D(POINT_COORD_REPLACE_MAP(0)), 8);
         for (i = 0; i < 8; ++i)
            PUSH_DATA(push, 0);

         nv50->state.point_sprite = FALSE;
      }
      return;
   } else {
      nv50->state.point_sprite = TRUE;
   }

   memset(pntc, 0, sizeof(pntc));

   for (i = 0; i < fp->in_nr; i++) {
      unsigned n = util_bitcount(fp->in[i].mask);

      if (fp->in[i].sn != TGSI_SEMANTIC_GENERIC) {
         m += n;
         continue;
      }
      if (!(nv50->rast->pipe.sprite_coord_enable & (1 << fp->in[i].si))) {
         m += n;
         continue;
      }

      for (c = 0; c < 4; ++c) {
         if (fp->in[i].mask & (1 << c)) {
            pntc[m / 8] |= (c + 1) << ((m % 8) * 4);
            ++m;
         }
      }
   }

   if (nv50->rast->pipe.sprite_coord_mode == PIPE_SPRITE_COORD_LOWER_LEFT)
      mode = 0x00;
   else
      mode = 0x10;

   BEGIN_NV04(push, NV50_3D(POINT_SPRITE_CTRL), 1);
   PUSH_DATA (push, mode);

   BEGIN_NV04(push, NV50_3D(POINT_COORD_REPLACE_MAP(0)), 8);
   PUSH_DATAp(push, pntc, 8);
}

/* Validate state derived from shaders and the rasterizer cso. */
void
nv50_validate_derived_rs(struct nv50_context *nv50)
{
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   uint32_t color, psize;

   nv50_sprite_coords_validate(nv50);

   if (nv50->dirty & NV50_NEW_FRAGPROG)
      return;
   psize = nv50->state.semantic_psize & ~NV50_3D_SEMANTIC_PTSZ_PTSZ_EN__MASK;
   color = nv50->state.semantic_color & ~NV50_3D_SEMANTIC_COLOR_CLMP_EN;

   if (nv50->rast->pipe.clamp_vertex_color)
      color |= NV50_3D_SEMANTIC_COLOR_CLMP_EN;

   if (color != nv50->state.semantic_color) {
      nv50->state.semantic_color = color;
      BEGIN_NV04(push, NV50_3D(SEMANTIC_COLOR), 1);
      PUSH_DATA (push, color);
   }

   if (nv50->rast->pipe.point_size_per_vertex)
      psize |= NV50_3D_SEMANTIC_PTSZ_PTSZ_EN__MASK;

   if (psize != nv50->state.semantic_psize) {
      nv50->state.semantic_psize = psize;
      BEGIN_NV04(push, NV50_3D(SEMANTIC_PTSZ), 1);
      PUSH_DATA (push, psize);
   }
}

static int
nv50_vec4_map(uint8_t *map, int mid, uint32_t lin[4],
              struct nv50_varying *in, struct nv50_varying *out)
{
   int c;
   uint8_t mv = out->mask, mf = in->mask, oid = out->hw;

   for (c = 0; c < 4; ++c) {
      if (mf & 1) {
         if (in->linear)
            lin[mid / 32] |= 1 << (mid % 32);
         if (mv & 1)
            map[mid] = oid;
         else
         if (c == 3)
            map[mid] |= 1;
         ++mid;
      }

      oid += mv & 1;
      mf >>= 1;
      mv >>= 1;
   }

   return mid;
}

void
nv50_fp_linkage_validate(struct nv50_context *nv50)
{
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct nv50_program *vp = nv50->gmtyprog ? nv50->gmtyprog : nv50->vertprog;
   struct nv50_program *fp = nv50->fragprog;
   struct nv50_varying dummy;
   int i, n, c, m;
   uint32_t primid = 0;
   uint32_t psiz = 0x000;
   uint32_t interp = fp->fp.interp;
   uint32_t colors = fp->fp.colors;
   uint32_t lin[4];
   uint8_t map[64];

   if (!(nv50->dirty & (NV50_NEW_VERTPROG |
                        NV50_NEW_FRAGPROG |
                        NV50_NEW_GMTYPROG))) {
      uint8_t bfc, ffc;
      ffc = (nv50->state.semantic_color & NV50_3D_SEMANTIC_COLOR_FFC0_ID__MASK);
      bfc = (nv50->state.semantic_color & NV50_3D_SEMANTIC_COLOR_BFC0_ID__MASK)
         >> 8;
      if (nv50->rast->pipe.light_twoside == ((ffc == bfc) ? 0 : 1))
         return;
   }

   memset(lin, 0x00, sizeof(lin));

   /* XXX: in buggy-endian mode, is the first element of map (u32)0x000000xx
    *  or is it the first byte ?
    */
   memset(map, nv50->gmtyprog ? 0x80 : 0x40, sizeof(map));

   dummy.mask = 0xf; /* map all components of HPOS */
   dummy.linear = 0;
   m = nv50_vec4_map(map, 0, lin, &dummy, &vp->out[0]);

   for (c = 0; c < vp->vp.clpd_nr; ++c)
      map[m++] = vp->vp.clpd[c / 4] + (c % 4);

   colors |= m << 8; /* adjust BFC0 id */

   dummy.mask = 0x0;

   /* if light_twoside is active, FFC0_ID == BFC0_ID is invalid */
   if (nv50->rast->pipe.light_twoside) {
      for (i = 0; i < 2; ++i) {
         n = vp->vp.bfc[i];
         if (fp->vp.bfc[i] >= fp->in_nr)
            continue;
         m = nv50_vec4_map(map, m, lin, &fp->in[fp->vp.bfc[i]],
                           (n < vp->out_nr) ? &vp->out[n] : &dummy);
      }
   }
   colors += m - 4; /* adjust FFC0 id */
   interp |= m << 8; /* set map id where 'normal' FP inputs start */

   for (i = 0; i < fp->in_nr; ++i) {
      for (n = 0; n < vp->out_nr; ++n)
         if (vp->out[n].sn == fp->in[i].sn &&
             vp->out[n].si == fp->in[i].si)
            break;
      m = nv50_vec4_map(map, m, lin,
                        &fp->in[i], (n < vp->out_nr) ? &vp->out[n] : &dummy);
   }

   /* PrimitiveID either is replaced by the system value, or
    * written by the geometry shader into an output register
    */
   if (fp->gp.primid < 0x80) {
      primid = m;
      map[m++] = vp->gp.primid;
   }

   if (nv50->rast->pipe.point_size_per_vertex) {
      psiz = (m << 4) | 1;
      map[m++] = vp->vp.psiz;
   }

   if (nv50->rast->pipe.clamp_vertex_color)
      colors |= NV50_3D_SEMANTIC_COLOR_CLMP_EN;

   n = (m + 3) / 4;
   assert(m <= 64);

   if (unlikely(nv50->gmtyprog)) {
      BEGIN_NV04(push, NV50_3D(GP_RESULT_MAP_SIZE), 1);
      PUSH_DATA (push, m);
      BEGIN_NV04(push, NV50_3D(GP_RESULT_MAP(0)), n);
      PUSH_DATAp(push, map, n);
   } else {
      BEGIN_NV04(push, NV50_3D(VP_GP_BUILTIN_ATTR_EN), 1);
      PUSH_DATA (push, vp->vp.attrs[2]);

      BEGIN_NV04(push, NV50_3D(SEMANTIC_PRIM_ID), 1);
      PUSH_DATA (push, primid);

      BEGIN_NV04(push, NV50_3D(VP_RESULT_MAP_SIZE), 1);
      PUSH_DATA (push, m);
      BEGIN_NV04(push, NV50_3D(VP_RESULT_MAP(0)), n);
      PUSH_DATAp(push, map, n);
   }

   BEGIN_NV04(push, NV50_3D(SEMANTIC_COLOR), 4);
   PUSH_DATA (push, colors);
   PUSH_DATA (push, (vp->vp.clpd_nr << 8) | 4);
   PUSH_DATA (push, 0);
   PUSH_DATA (push, psiz);

   BEGIN_NV04(push, NV50_3D(FP_INTERPOLANT_CTRL), 1);
   PUSH_DATA (push, interp);

   nv50->state.interpolant_ctrl = interp;

   nv50->state.semantic_color = colors;
   nv50->state.semantic_psize = psiz;

   BEGIN_NV04(push, NV50_3D(NOPERSPECTIVE_BITMAP(0)), 4);
   PUSH_DATAp(push, lin, 4);

   BEGIN_NV04(push, NV50_3D(GP_ENABLE), 1);
   PUSH_DATA (push, nv50->gmtyprog ? 1 : 0);
}

static int
nv50_vp_gp_mapping(uint8_t *map, int m,
                   struct nv50_program *vp, struct nv50_program *gp)
{
   int i, j, c;

   for (i = 0; i < gp->in_nr; ++i) {
      uint8_t oid = 0, mv = 0, mg = gp->in[i].mask;

      for (j = 0; j < vp->out_nr; ++j) {
         if (vp->out[j].sn == gp->in[i].sn &&
             vp->out[j].si == gp->in[i].si) {
            mv = vp->out[j].mask;
            oid = vp->out[j].hw;
            break;
         }
      }

      for (c = 0; c < 4; ++c, mv >>= 1, mg >>= 1) {
         if (mg & mv & 1)
            map[m++] = oid;
         else
         if (mg & 1)
            map[m++] = (c == 3) ? 0x41 : 0x40;
         oid += mv & 1;
      }
   }
   return m;
}

void
nv50_gp_linkage_validate(struct nv50_context *nv50)
{
   struct nouveau_pushbuf *push = nv50->base.pushbuf;
   struct nv50_program *vp = nv50->vertprog;
   struct nv50_program *gp = nv50->gmtyprog;
   int m = 0;
   int n;
   uint8_t map[64];

   if (!gp)
      return;
   memset(map, 0, sizeof(map));

   m = nv50_vp_gp_mapping(map, m, vp, gp);

   n = (m + 3) / 4;

   BEGIN_NV04(push, NV50_3D(VP_GP_BUILTIN_ATTR_EN), 1);
   PUSH_DATA (push, vp->vp.attrs[2] | gp->vp.attrs[2]);

   BEGIN_NV04(push, NV50_3D(VP_RESULT_MAP_SIZE), 1);
   PUSH_DATA (push, m);
   BEGIN_NV04(push, NV50_3D(VP_RESULT_MAP(0)), n);
   PUSH_DATAp(push, map, n);
}
