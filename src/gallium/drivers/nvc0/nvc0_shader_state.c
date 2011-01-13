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

static boolean
nvc0_program_validate(struct nvc0_context *nvc0, struct nvc0_program *prog)
{
   int ret;
   unsigned size;

   if (prog->translated)
      return TRUE;

   prog->translated = nvc0_program_translate(prog);
   if (!prog->translated)
      return FALSE;

   size = align(prog->code_size + NVC0_SHADER_HEADER_SIZE, 0x100);

   ret = nouveau_resource_alloc(nvc0->screen->text_heap, size, prog,
                                &prog->res);
   if (ret)
      return FALSE;

   prog->code_base = prog->res->start;

   nvc0_m2mf_push_linear(nvc0, nvc0->screen->text, NOUVEAU_BO_VRAM,
                         prog->code_base, NVC0_SHADER_HEADER_SIZE, prog->hdr);
   nvc0_m2mf_push_linear(nvc0, nvc0->screen->text, NOUVEAU_BO_VRAM,
                         prog->code_base + NVC0_SHADER_HEADER_SIZE,
                         prog->code_size, prog->code);

   BEGIN_RING(nvc0->screen->base.channel, RING_3D_(0x021c), 1);
   OUT_RING  (nvc0->screen->base.channel, 0x1111);

   return TRUE;
}

void
nvc0_vertprog_validate(struct nvc0_context *nvc0)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   struct nvc0_program *vp = nvc0->vertprog;

   if (nvc0->clip.nr > vp->vp.num_ucps) {
      assert(nvc0->clip.nr <= 6);
      vp->vp.num_ucps = 6;

      if (vp->translated)
         nvc0_program_destroy(nvc0, vp);
   }

   if (!nvc0_program_validate(nvc0, vp))
         return;

   BEGIN_RING(chan, RING_3D(SP_SELECT(1)), 2);
   OUT_RING  (chan, 0x11);
   OUT_RING  (chan, vp->code_base);
   BEGIN_RING(chan, RING_3D(SP_GPR_ALLOC(1)), 1);
   OUT_RING  (chan, vp->max_gpr);

   // BEGIN_RING(chan, RING_3D_(0x163c), 1);
   // OUT_RING  (chan, 0);
   BEGIN_RING(chan, RING_3D(VERT_COLOR_CLAMP_EN), 1);
   OUT_RING  (chan, 1);
}

void
nvc0_fragprog_validate(struct nvc0_context *nvc0)
{
   struct nouveau_channel *chan = nvc0->screen->base.channel;
   struct nvc0_program *fp = nvc0->fragprog;

   if (!nvc0_program_validate(nvc0, fp))
         return;

   BEGIN_RING(chan, RING_3D(EARLY_FRAGMENT_TESTS), 1);
   OUT_RING  (chan, fp->fp.early_z);
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

   BEGIN_RING(chan, RING_3D(SP_SELECT(2)), 2);
   OUT_RING  (chan, 0x21);
   OUT_RING  (chan, tp->code_base);
   BEGIN_RING(chan, RING_3D(SP_GPR_ALLOC(2)), 1);
   OUT_RING  (chan, tp->max_gpr);   
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

   if (!gp) {
      BEGIN_RING(chan, RING_3D(GP_SELECT), 1);
      OUT_RING  (chan, 0x40);
      return;
   }
   if (!nvc0_program_validate(nvc0, gp))
         return;

   BEGIN_RING(chan, RING_3D(GP_SELECT), 1);
   OUT_RING  (chan, 0x41);
   BEGIN_RING(chan, RING_3D(SP_START_ID(4)), 1);
   OUT_RING  (chan, gp->code_base);
   BEGIN_RING(chan, RING_3D(SP_GPR_ALLOC(4)), 1);
   OUT_RING  (chan, gp->max_gpr);   
}
