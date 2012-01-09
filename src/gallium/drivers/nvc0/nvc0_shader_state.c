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

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"

#include "nvc0_context.h"

static INLINE void
nvc0_program_update_context_state(struct nvc0_context *nvc0,
                                  struct nvc0_program *prog, int stage)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;

   if (prog->hdr[1])
      nvc0->state.tls_required |= 1 << stage;
   else
      nvc0->state.tls_required &= ~(1 << stage);

   if (prog->immd_size) {
      const unsigned rl = NOUVEAU_BO_VRAM | NOUVEAU_BO_RD;

      BEGIN_RING(chan, RING_3D(CB_SIZE), 3);
      /* NOTE: may overlap code of a different shader */
      OUT_RING  (chan, align(prog->immd_size, 0x100));
      OUT_RELOCh(chan, nvc0->screen->text, prog->immd_base, rl);
      OUT_RELOCl(chan, nvc0->screen->text, prog->immd_base, rl);
      BEGIN_RING(chan, RING_3D(CB_BIND(stage)), 1);
      OUT_RING  (chan, (14 << 4) | 1);

      nvc0->state.c14_bound |= 1 << stage;
   } else
   if (nvc0->state.c14_bound & (1 << stage)) {
      BEGIN_RING(chan, RING_3D(CB_BIND(stage)), 1);
      OUT_RING  (chan, (14 << 4) | 0);

      nvc0->state.c14_bound &= ~(1 << stage);
   }
}

static INLINE boolean
nvc0_program_validate(struct nvc0_context *nvc0, struct nvc0_program *prog)
{
   if (prog->res)
      return TRUE;

   if (!prog->translated) {
      prog->translated = nvc0_program_translate(prog);
      if (!prog->translated)
         return FALSE;
   }

   if (likely(prog->code_size))
      return nvc0_program_upload_code(nvc0, prog);
   return TRUE; /* stream output info only */
}

void
nvc0_vertprog_validate(struct nvc0_context *nvc0)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   struct nvc0_program *vp = nvc0->vertprog;

   if (!nvc0_program_validate(nvc0, vp))
         return;
   nvc0_program_update_context_state(nvc0, vp, 0);

   BEGIN_RING(chan, RING_3D(SP_SELECT(1)), 2);
   OUT_RING  (chan, 0x11);
   OUT_RING  (chan, vp->code_base);
   BEGIN_RING(chan, RING_3D(SP_GPR_ALLOC(1)), 1);
   OUT_RING  (chan, vp->max_gpr);

   // BEGIN_RING(chan, RING_3D_(0x163c), 1);
   // OUT_RING  (chan, 0);
}

void
nvc0_fragprog_validate(struct nvc0_context *nvc0)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   struct nvc0_program *fp = nvc0->fragprog;

   if (!nvc0_program_validate(nvc0, fp))
         return;
   nvc0_program_update_context_state(nvc0, fp, 4);

   BEGIN_RING(chan, RING_3D(SP_SELECT(5)), 2);
   OUT_RING  (chan, 0x51);
   OUT_RING  (chan, fp->code_base);
   BEGIN_RING(chan, RING_3D(SP_GPR_ALLOC(5)), 1);
   OUT_RING  (chan, fp->max_gpr);

   BEGIN_RING(chan, RING_3D_(0x0360), 2);
   OUT_RING  (chan, 0x20164010);
   OUT_RING  (chan, 0x20);
   BEGIN_RING(chan, RING_3D_(0x196c), 1);
   OUT_RING  (chan, fp->flags[0]);
}

void
nvc0_tctlprog_validate(struct nvc0_context *nvc0)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   struct nvc0_program *tp = nvc0->tctlprog;

   if (!tp) {
      BEGIN_RING(chan, RING_3D(SP_SELECT(2)), 1);
      OUT_RING  (chan, 0x20);
      return;
   }
   if (!nvc0_program_validate(nvc0, tp))
         return;
   nvc0_program_update_context_state(nvc0, tp, 1);

   if (tp->tp.tess_mode != ~0) {
      BEGIN_RING(chan, RING_3D(TESS_MODE), 1);
      OUT_RING  (chan, tp->tp.tess_mode);
   }
   BEGIN_RING(chan, RING_3D(SP_SELECT(2)), 2);
   OUT_RING  (chan, 0x21);
   OUT_RING  (chan, tp->code_base);
   BEGIN_RING(chan, RING_3D(SP_GPR_ALLOC(2)), 1);
   OUT_RING  (chan, tp->max_gpr);

   if (tp->tp.input_patch_size <= 32)
      IMMED_RING(chan, RING_3D(PATCH_VERTICES), tp->tp.input_patch_size);
}

void
nvc0_tevlprog_validate(struct nvc0_context *nvc0)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   struct nvc0_program *tp = nvc0->tevlprog;

   if (!tp) {
      BEGIN_RING(chan, RING_3D(TEP_SELECT), 1);
      OUT_RING  (chan, 0x30);
      return;
   }
   if (!nvc0_program_validate(nvc0, tp))
         return;
   nvc0_program_update_context_state(nvc0, tp, 2);

   if (tp->tp.tess_mode != ~0) {
      BEGIN_RING(chan, RING_3D(TESS_MODE), 1);
      OUT_RING  (chan, tp->tp.tess_mode);
   }
   BEGIN_RING(chan, RING_3D(TEP_SELECT), 1);
   OUT_RING  (chan, 0x31);
   BEGIN_RING(chan, RING_3D(SP_START_ID(3)), 1);
   OUT_RING  (chan, tp->code_base);
   BEGIN_RING(chan, RING_3D(SP_GPR_ALLOC(3)), 1);
   OUT_RING  (chan, tp->max_gpr);
}

void
nvc0_gmtyprog_validate(struct nvc0_context *nvc0)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   struct nvc0_program *gp = nvc0->gmtyprog;

   if (gp)
      nvc0_program_validate(nvc0, gp);
   /* we allow GPs with no code for specifying stream output state only */
   if (!gp || !gp->code_size) {
      BEGIN_RING(chan, RING_3D(GP_SELECT), 1);
      OUT_RING  (chan, 0x40);
      IMMED_RING(chan, RING_3D(LAYER), 0);
      return;
   }
   nvc0_program_update_context_state(nvc0, gp, 3);

   BEGIN_RING(chan, RING_3D(GP_SELECT), 1);
   OUT_RING  (chan, 0x41);
   BEGIN_RING(chan, RING_3D(SP_START_ID(4)), 1);
   OUT_RING  (chan, gp->code_base);
   BEGIN_RING(chan, RING_3D(SP_GPR_ALLOC(4)), 1);
   OUT_RING  (chan, gp->max_gpr);
   BEGIN_RING(chan, RING_3D(LAYER), 1);
   OUT_RING  (chan, (gp->hdr[13] & (1 << 9)) ? NVC0_3D_LAYER_USE_GP : 0);
}

void
nvc0_tfb_validate(struct nvc0_context *nvc0)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   struct nvc0_transform_feedback_state *tfb;
   unsigned b, n, i;

   if (nvc0->gmtyprog) tfb = nvc0->gmtyprog->tfb;
   else
   if (nvc0->tevlprog) tfb = nvc0->tevlprog->tfb;
   else
      tfb = nvc0->vertprog->tfb;

   IMMED_RING(chan, RING_3D(TFB_ENABLE), (tfb && nvc0->num_tfbbufs) ? 1 : 0);

   if (tfb && tfb != nvc0->state.tfb) {
      uint8_t var[128];

      for (n = 0, b = 0; b < 4; n += tfb->varying_count[b++]) {
         if (tfb->varying_count[b]) {
            BEGIN_RING(chan, RING_3D(TFB_STREAM(b)), 3);
            OUT_RING  (chan, 0);
            OUT_RING  (chan, tfb->varying_count[b]);
            OUT_RING  (chan, tfb->stride[b]);

            for (i = 0; i < tfb->varying_count[b]; ++i)
               var[i] = tfb->varying_index[n + i];
            for (; i & 3; ++i)
               var[i] = 0; /* zero rest of method word bits */

            BEGIN_RING(chan, RING_3D(TFB_VARYING_LOCS(b, 0)), i / 4);
            OUT_RINGp (chan, var, i / 4);

            if (nvc0->tfbbuf[b])
               nvc0_so_target(nvc0->tfbbuf[b])->stride = tfb->stride[b];
         } else {
            IMMED_RING(chan, RING_3D(TFB_VARYING_COUNT(b)), 0);
         }
      }
   }
   nvc0->state.tfb = tfb;

   if (!(nvc0->dirty & NVC0_NEW_TFB_TARGETS))
      return;
   nvc0_bufctx_reset(nvc0, NVC0_BUFCTX_TFB);

   for (b = 0; b < nvc0->num_tfbbufs; ++b) {
      struct nvc0_so_target *targ = nvc0_so_target(nvc0->tfbbuf[b]);
      struct nv04_resource *buf = nv04_resource(targ->pipe.buffer);

      if (tfb)
         targ->stride = tfb->stride[b];

      if (!(nvc0->tfbbuf_dirty & (1 << b)))
         continue;

      if (!targ->clean)
         nvc0_query_fifo_wait(chan, targ->pq);
      BEGIN_RING(chan, RING_3D(TFB_BUFFER_ENABLE(b)), 5);
      OUT_RING  (chan, 1);
      OUT_RESRCh(chan, buf, targ->pipe.buffer_offset, NOUVEAU_BO_WR);
      OUT_RESRCl(chan, buf, targ->pipe.buffer_offset, NOUVEAU_BO_WR);
      OUT_RING  (chan, targ->pipe.buffer_size);
      if (!targ->clean) {
         nvc0_query_pushbuf_submit(chan, targ->pq, 0x4);
      } else {
         OUT_RING(chan, 0); /* TFB_BUFFER_OFFSET */
         targ->clean = FALSE;
      }
      nvc0_bufctx_add_resident(nvc0, NVC0_BUFCTX_TFB, buf, NOUVEAU_BO_WR);
   }
   for (; b < 4; ++b)
      IMMED_RING(chan, RING_3D(TFB_BUFFER_ENABLE(b)), 0);
}
