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

static void
nv50_transfer_constbuf(struct nv50_context *nv50,
                       struct pipe_resource *buf, unsigned size, unsigned cbi)
{
   struct pipe_context *pipe = &nv50->pipe;
   struct pipe_transfer *transfer;
   struct nouveau_channel *chan = nv50->screen->base.channel;
   struct nouveau_grobj *tesla = nv50->screen->tesla;
   uint32_t *map;
   unsigned count, start;

   if (buf == NULL)
      return;

   map = pipe_buffer_map(pipe, buf, PIPE_TRANSFER_READ, &transfer);
   if (!map)
      return;

   count = (buf->width0 + 3) / 4;
   start = 0;

   while (count) {
      unsigned nr = AVAIL_RING(chan);

      if (nr < 8) {
         FIRE_RING(chan);
         continue;
      }
      nr = MIN2(count, nr - 7);
      nr = MIN2(nr, NV04_PFIFO_MAX_PACKET_LEN);

      nv50_screen_reloc_constbuf(nv50->screen, cbi);

      BEGIN_RING(chan, tesla, NV50TCL_CB_ADDR, 1);
      OUT_RING  (chan, (start << 8) | cbi);
      BEGIN_RING_NI(chan, tesla, NV50TCL_CB_DATA(0), nr);
      OUT_RINGp (chan, map, nr);

      count -= nr;
      start += nr;
      map += nr;
   }

   pipe_buffer_unmap(pipe, transfer);
}

static void
nv50_program_validate_data(struct nv50_context *nv50, struct nv50_program *p)
{
   struct nouveau_channel *chan = nv50->screen->base.channel;
   struct nouveau_grobj *tesla = nv50->screen->tesla;
   unsigned cbi;

   if (p->immd_size) {
      uint32_t *data = p->immd;
      unsigned count = p->immd_size / 4;
      unsigned start = 0;

      while (count) {
         unsigned nr = AVAIL_RING(chan);

         if (nr < 8) {
            FIRE_RING(chan);
            continue;
         }
         nr = MIN2(count, nr - 7);
         nr = MIN2(nr, NV04_PFIFO_MAX_PACKET_LEN);

         nv50_screen_reloc_constbuf(nv50->screen, NV50_CB_PMISC);

         BEGIN_RING(chan, tesla, NV50TCL_CB_ADDR, 1);
         OUT_RING  (chan, (start << 8) | NV50_CB_PMISC);
         BEGIN_RING_NI(chan, tesla, NV50TCL_CB_DATA(0), nr);
         OUT_RINGp (chan, data, nr);

         count -= nr;
         start += nr;
         data += nr;
      }
   }

   /* If the state tracker doesn't change the constbuf, and it is first
    * validated with a program that doesn't use it, this check prevents
    * it from even being uploaded. */
   /*
   if (p->parm_size == 0)
      return;
   */

   switch (p->type) {
   case PIPE_SHADER_VERTEX:
      cbi = NV50_CB_PVP;
      break;
   case PIPE_SHADER_FRAGMENT:
      cbi = NV50_CB_PFP;
      break;
   case PIPE_SHADER_GEOMETRY:
      cbi = NV50_CB_PGP;
      break;
   default:
      assert(0);
      return;
   }

   nv50_transfer_constbuf(nv50, nv50->constbuf[p->type], p->parm_size, cbi);
}

static void
nv50_program_validate_code(struct nv50_context *nv50, struct nv50_program *p)
{
   struct nouveau_channel *chan = nv50->screen->base.channel;
   struct nouveau_grobj *tesla = nv50->screen->tesla;
   struct nouveau_grobj *eng2d = nv50->screen->eng2d;
   int ret;
   unsigned offset;
   unsigned size = p->code_size;
   uint32_t *data = p->code;

   assert(p->translated);

   /* TODO: use a single bo (for each type) for shader code */
   if (p->bo)
      return;
   ret = nouveau_bo_new(chan->device, NOUVEAU_BO_VRAM, 0x100, size, &p->bo);
   assert(!ret);

   offset = p->code_start = 0;

   BEGIN_RING(chan, eng2d, NV50_2D_DST_FORMAT, 2);
   OUT_RING  (chan, NV50_2D_DST_FORMAT_R8_UNORM);
   OUT_RING  (chan, 1);
   BEGIN_RING(chan, eng2d, NV50_2D_DST_PITCH, 1);
   OUT_RING  (chan, 0x40000);
   BEGIN_RING(chan, eng2d, NV50_2D_DST_WIDTH, 2);
   OUT_RING  (chan, 0x10000);
   OUT_RING  (chan, 1);

   while (size) {
      unsigned nr = size / 4;

      if (AVAIL_RING(chan) < 32)
         FIRE_RING(chan);

      nr = MIN2(nr, AVAIL_RING(chan) - 18);
      nr = MIN2(nr, 1792);
      if (nr < (size / 4))
         nr &= ~0x3f;
      assert(!(size & 3));

      BEGIN_RING(chan, eng2d, NV50_2D_DST_ADDRESS_HIGH, 2);
      OUT_RELOCh(chan, p->bo, offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
      OUT_RELOCl(chan, p->bo, offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
      BEGIN_RING(chan, eng2d, NV50_2D_SIFC_BITMAP_ENABLE, 2);
      OUT_RING  (chan, 0);
      OUT_RING  (chan, NV50_2D_SIFC_FORMAT_R8_UNORM);
      BEGIN_RING(chan, eng2d, NV50_2D_SIFC_WIDTH, 10);
      OUT_RING  (chan, nr * 4);
      OUT_RING  (chan, 1);
      OUT_RING  (chan, 0);
      OUT_RING  (chan, 1);
      OUT_RING  (chan, 0);
      OUT_RING  (chan, 1);
      OUT_RING  (chan, 0);
      OUT_RING  (chan, 0);
      OUT_RING  (chan, 0);
      OUT_RING  (chan, 0);

      BEGIN_RING_NI(chan, eng2d, NV50_2D_SIFC_DATA, nr);
      OUT_RINGp (chan, data, nr);

      data += nr;
      offset += nr * 4;
      size -= nr * 4;
   }

   BEGIN_RING(chan, tesla, NV50TCL_CODE_CB_FLUSH, 1);
   OUT_RING  (chan, 0);
}

static void
nv50_vp_update_stateobj(struct nv50_context *nv50, struct nv50_program *p)
{
   struct nouveau_grobj *tesla = nv50->screen->tesla;
   struct nouveau_stateobj *so = so_new(5, 7, 2);

   nv50_program_validate_code(nv50, p);

   so_method(so, tesla, NV50TCL_VP_ADDRESS_HIGH, 2);
   so_reloc (so, p->bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
             NOUVEAU_BO_HIGH, 0, 0);
   so_reloc (so, p->bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
             NOUVEAU_BO_LOW, 0, 0);
   so_method(so, tesla, NV50TCL_VP_ATTR_EN_0, 2);
   so_data  (so, p->vp.attrs[0]);
   so_data  (so, p->vp.attrs[1]);
   so_method(so, tesla, NV50TCL_VP_REG_ALLOC_RESULT, 1);
   so_data  (so, p->max_out);
   so_method(so, tesla, NV50TCL_VP_REG_ALLOC_TEMP, 1);
   so_data  (so, p->max_gpr);
   so_method(so, tesla, NV50TCL_VP_START_ID, 1);
   so_data  (so, p->code_start);

   so_ref(so, &p->so);
   so_ref(NULL, &so);
}

static void
nv50_fp_update_stateobj(struct nv50_context *nv50, struct nv50_program *p)
{
   struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_stateobj *so = so_new(6, 7, 2);

   nv50_program_validate_code(nv50, p);

   so_method(so, tesla, NV50TCL_FP_ADDRESS_HIGH, 2);
   so_reloc (so, p->bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
             NOUVEAU_BO_HIGH, 0, 0);
   so_reloc (so, p->bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
             NOUVEAU_BO_LOW, 0, 0);
   so_method(so, tesla, NV50TCL_FP_REG_ALLOC_TEMP, 1);
   so_data  (so, p->max_gpr);
   so_method(so, tesla, NV50TCL_FP_RESULT_COUNT, 1);
   so_data  (so, p->max_out);
   so_method(so, tesla, NV50TCL_FP_CONTROL, 1);
   so_data  (so, p->fp.flags[0]);
   so_method(so, tesla, NV50TCL_FP_CTRL_UNK196C, 1);
   so_data  (so, p->fp.flags[1]);
   so_method(so, tesla, NV50TCL_FP_START_ID, 1);
   so_data  (so, p->code_start);

   so_ref(so, &p->so);
   so_ref(NULL, &so);
}

static void
nv50_gp_update_stateobj(struct nv50_context *nv50, struct nv50_program *p)
{
   struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_stateobj *so = so_new(6, 7, 2);

   nv50_program_validate_code(nv50, p);

   so_method(so, tesla, NV50TCL_GP_ADDRESS_HIGH, 2);
   so_reloc (so, p->bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
             NOUVEAU_BO_HIGH, 0, 0);
   so_reloc (so, p->bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
             NOUVEAU_BO_LOW, 0, 0);
   so_method(so, tesla, NV50TCL_GP_REG_ALLOC_TEMP, 1);
   so_data  (so, p->max_gpr);
   so_method(so, tesla, NV50TCL_GP_REG_ALLOC_RESULT, 1);
   so_data  (so, p->max_out);
   so_method(so, tesla, NV50TCL_GP_OUTPUT_PRIMITIVE_TYPE, 1);
   so_data  (so, p->gp.prim_type);
   so_method(so, tesla, NV50TCL_GP_VERTEX_OUTPUT_COUNT, 1);
   so_data  (so, p->gp.vert_count);
   so_method(so, tesla, NV50TCL_GP_START_ID, 1);
   so_data  (so, p->code_start);

   so_ref(so, &p->so);
   so_ref(NULL, &so);
}

static boolean
nv50_program_validate(struct nv50_program *p)
{
   p->translated = nv50_program_tx(p);
   assert(p->translated);
   return p->translated;
}

static INLINE void
nv50_program_validate_common(struct nv50_context *nv50, struct nv50_program *p)
{
   nv50_program_validate_code(nv50, p);

   if (p->uses_lmem)
      nv50->req_lmem |= 1 << p->type;
   else
      nv50->req_lmem &= ~(1 << p->type);
}

struct nouveau_stateobj *
nv50_vertprog_validate(struct nv50_context *nv50)
{
   struct nv50_program *p = nv50->vertprog;
   struct nouveau_stateobj *so = NULL;

   if (!p->translated) {
      if (nv50_program_validate(p))
         nv50_vp_update_stateobj(nv50, p);
      else
         return NULL;
   }

   if (nv50->dirty & NV50_NEW_VERTPROG_CB)
      nv50_program_validate_data(nv50, p);

   if (!(nv50->dirty & NV50_NEW_VERTPROG))
      return NULL;

   nv50_program_validate_common(nv50, p);

   so_ref(p->so, &so);
   return so;
}

struct nouveau_stateobj *
nv50_fragprog_validate(struct nv50_context *nv50)
{
   struct nv50_program *p = nv50->fragprog;
   struct nouveau_stateobj *so = NULL;

   if (!p->translated) {
      if (nv50_program_validate(p))
         nv50_fp_update_stateobj(nv50, p);
      else
         return NULL;
   }

   if (nv50->dirty & NV50_NEW_FRAGPROG_CB)
      nv50_program_validate_data(nv50, p);

   if (!(nv50->dirty & NV50_NEW_FRAGPROG))
      return NULL;

   nv50_program_validate_common(nv50, p);

   so_ref(p->so, &so);
   return so;
}

struct nouveau_stateobj *
nv50_geomprog_validate(struct nv50_context *nv50)
{
   struct nv50_program *p = nv50->geomprog;
   struct nouveau_stateobj *so = NULL;

   /* GP may be NULL, but VP and FP may not */
   if (!p)
      return NULL; /* GP is deactivated in linkage validation */

   if (!p->translated) {
      if (nv50_program_validate(p))
         nv50_gp_update_stateobj(nv50, p);
      else
         return NULL;
   }

   if (nv50->dirty & NV50_NEW_GEOMPROG_CB)
      nv50_program_validate_data(nv50, p);

   if (!(nv50->dirty & NV50_NEW_GEOMPROG))
      return NULL;

   nv50_program_validate_common(nv50, p);

   so_ref(p->so, &so);
   return so;
}

/* XXX: this might not work correctly in all cases yet: we assume that
 * an FP generic input that is not written in the VP is gl_PointCoord.
 */
static uint32_t
nv50_pntc_replace(struct nv50_context *nv50, uint32_t pntc[8], unsigned m)
{
   struct nv50_program *vp = nv50->vertprog;
   struct nv50_program *fp = nv50->fragprog;
   unsigned i, c;

   memset(pntc, 0, 8 * sizeof(uint32_t));

   if (nv50->geomprog)
      vp = nv50->geomprog;

   for (i = 0; i < fp->in_nr; i++) {
      unsigned j, n = util_bitcount(fp->in[i].mask);

      if (fp->in[i].sn != TGSI_SEMANTIC_GENERIC) {
         m += n;
         continue;
      }

      for (j = 0; j < vp->out_nr; ++j)
         if (vp->out[j].sn == fp->in[i].sn && vp->out[j].si == fp->in[i].si)
            break;

      if (j < vp->out_nr) {
         uint32_t en = nv50->rasterizer->pipe.sprite_coord_enable;

         if (!(en & (1 << vp->out[j].si))) {
            m += n;
            continue;
         }
      }

      /* this is either PointCoord or replaced by sprite coords */
      for (c = 0; c < 4; c++) {
         if (!(fp->in[i].mask & (1 << c)))
            continue;
         pntc[m / 8] |= (c + 1) << ((m % 8) * 4);
         ++m;
      }
   }
   if (nv50->rasterizer->pipe.sprite_coord_mode == PIPE_SPRITE_COORD_LOWER_LEFT)
      return 0;
   return (1 << 4);
}

static int
nv50_vec4_map(uint32_t *map32, int mid, uint32_t lin[4],
              struct nv50_varying *in, struct nv50_varying *out)
{
   int c;
   uint8_t mv = out->mask, mf = in->mask, oid = out->hw;
   uint8_t *map = (uint8_t *)map32;

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

struct nouveau_stateobj *
nv50_fp_linkage_validate(struct nv50_context *nv50)
{
   struct nouveau_grobj *tesla = nv50->screen->tesla;
   struct nv50_program *vp;
   struct nv50_program *fp = nv50->fragprog;
   struct nouveau_stateobj *so;
   struct nv50_varying dummy;
   int i, n, c, m;

   uint32_t map[16], lin[4], pntc[8];

   uint32_t interp = fp->fp.interp;
   uint32_t colors = fp->fp.colors;
   uint32_t clip = 0x04;
   uint32_t psiz = 0x000;
   uint32_t primid = 0;
   uint32_t sysval = 0;

   if (nv50->geomprog) {
      vp = nv50->geomprog;
      memset(map, 0x80, sizeof(map));
   } else {
      vp = nv50->vertprog;
      memset(map, 0x40, sizeof(map));
   }
   memset(lin, 0, sizeof(lin));

   dummy.linear = 0;
   dummy.mask = 0xf; /* map all components of HPOS */
   m = nv50_vec4_map(map, 0, lin, &dummy, &vp->out[0]);

   if (vp->vp.clpd < 0x40) {
      for (c = 0; c < vp->vp.clpd_nr; ++c) {
         map[m / 4] |= (vp->vp.clpd + c) << ((m % 4) * 8);
         ++m;
      }
      clip |= vp->vp.clpd_nr << 8;
   }

   colors |= m << 8; /* adjust BFC0 id */

   /* if light_twoside is active, it seems FFC0_ID == BFC0_ID is bad */
   if (nv50->rasterizer->pipe.light_twoside) {
      for (i = 0; i < 2; ++i)
         m = nv50_vec4_map(map, m, lin,
                           &fp->in[fp->vp.bfc[i]],
                           &vp->out[vp->vp.bfc[i]]);
   }

   colors += m - 4; /* adjust FFC0 id */
   interp |= m << 8; /* set mid where 'normal' FP inputs start */

   dummy.mask = 0x0;
   for (i = 0; i < fp->in_nr; i++) {
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
   if (fp->gp.primid < 0x40) {
      i = (m % 4) * 8;
      map[m / 4] = (map[m / 4] & ~(0xff << i)) | (vp->gp.primid << i);
      primid = m++;
   }

   if (nv50->rasterizer->pipe.point_size_per_vertex) {
      i = (m % 4) * 8;
      map[m / 4] = (map[m / 4] & ~(0xff << i)) | (vp->vp.psiz << i);
      psiz = (m++ << 4) | 1;
   }

   /* now fill the stateobj (at most 28 so_data)  */
   so = so_new(10, 54, 0);

   n = (m + 3) / 4;
   assert(m <= 64);
   if (vp->type == PIPE_SHADER_GEOMETRY) {
      so_method(so, tesla, NV50TCL_GP_RESULT_MAP_SIZE, 1);
      so_data  (so, m);
      so_method(so, tesla, NV50TCL_GP_RESULT_MAP(0), n);
      so_datap (so, map, n);
   } else {
      so_method(so, tesla, NV50TCL_VP_GP_BUILTIN_ATTR_EN, 1);
      so_data  (so, vp->vp.attrs[2]);

      so_method(so, tesla, NV50TCL_MAP_SEMANTIC_4, 1);
      so_data  (so, primid);

      so_method(so, tesla, NV50TCL_VP_RESULT_MAP_SIZE, 1);
      so_data  (so, m);
      so_method(so, tesla, NV50TCL_VP_RESULT_MAP(0), n);
      so_datap (so, map, n);
   }

   so_method(so, tesla, NV50TCL_MAP_SEMANTIC_0, 4);
   so_data  (so, colors);
   so_data  (so, clip);
   so_data  (so, sysval);
   so_data  (so, psiz);

   so_method(so, tesla, NV50TCL_FP_INTERPOLANT_CTRL, 1);
   so_data  (so, interp);

   so_method(so, tesla, NV50TCL_NOPERSPECTIVE_BITMAP(0), 4);
   so_datap (so, lin, 4);

   if (nv50->rasterizer->pipe.point_quad_rasterization) {
      so_method(so, tesla, NV50TCL_POINT_SPRITE_CTRL, 1);
      so_data  (so,
                nv50_pntc_replace(nv50, pntc, (interp >> 8) & 0xff));

      so_method(so, tesla, NV50TCL_POINT_COORD_REPLACE_MAP(0), 8);
      so_datap (so, pntc, 8);
   }

   so_method(so, tesla, NV50TCL_GP_ENABLE, 1);
   so_data  (so, (vp->type == PIPE_SHADER_GEOMETRY) ? 1 : 0);

   return so;
}

static int
nv50_vp_gp_mapping(uint32_t *map32, int m,
                   struct nv50_program *vp, struct nv50_program *gp)
{
   uint8_t *map = (uint8_t *)map32;
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

struct nouveau_stateobj *
nv50_gp_linkage_validate(struct nv50_context *nv50)
{
   struct nouveau_grobj *tesla = nv50->screen->tesla;
   struct nouveau_stateobj *so;
   struct nv50_program *vp = nv50->vertprog;
   struct nv50_program *gp = nv50->geomprog;
   uint32_t map[16];
   int m = 0;

   if (!gp)
      return NULL;
   memset(map, 0, sizeof(map));

   m = nv50_vp_gp_mapping(map, m, vp, gp);

   so = so_new(3, 24 - 3, 0);

   so_method(so, tesla, NV50TCL_VP_GP_BUILTIN_ATTR_EN, 1);
   so_data  (so, vp->vp.attrs[2] | gp->vp.attrs[2]);

   assert(m <= 32);
   so_method(so, tesla, NV50TCL_VP_RESULT_MAP_SIZE, 1);
   so_data  (so, m);

   m = (m + 3) / 4;
   so_method(so, tesla, NV50TCL_VP_RESULT_MAP(0), m);
   so_datap (so, map, m);

   return so;
}
