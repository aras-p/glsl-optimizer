/*
 * Copyright 2013 Nouveau Project
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors: Christoph Bumiller, Samuel Pitoiset
 */

#include "nvc0/nvc0_context.h"
#include "nvc0/nvc0_compute.h"

int
nvc0_screen_compute_setup(struct nvc0_screen *screen,
                          struct nouveau_pushbuf *push)
{
   struct nouveau_object *chan = screen->base.channel;
   struct nouveau_device *dev = screen->base.device;
   uint32_t obj_class;
   int ret;
   int i;

   switch (dev->chipset & ~0xf) {
   case 0xc0:
      if (dev->chipset == 0xc8)
         obj_class = NVC8_COMPUTE_CLASS;
      else
         obj_class = NVC0_COMPUTE_CLASS;
      break;
   case 0xd0:
      obj_class = NVC0_COMPUTE_CLASS;
      break;
   default:
      NOUVEAU_ERR("unsupported chipset: NV%02x\n", dev->chipset);
      return -1;
   }

   ret = nouveau_object_new(chan, 0xbeef90c0, obj_class, NULL, 0,
                            &screen->compute);
   if (ret) {
      NOUVEAU_ERR("Failed to allocate compute object: %d\n", ret);
      return ret;
   }

   ret = nouveau_bo_new(dev, NOUVEAU_BO_VRAM, 0, 1 << 12, NULL,
                        &screen->parm);
   if (ret)
      return ret;

   BEGIN_NVC0(push, SUBC_COMPUTE(NV01_SUBCHAN_OBJECT), 1);
   PUSH_DATA (push, screen->compute->oclass);

   /* hardware limit */
   BEGIN_NVC0(push, NVC0_COMPUTE(MP_LIMIT), 1);
   PUSH_DATA (push, screen->mp_count);
   BEGIN_NVC0(push, NVC0_COMPUTE(CALL_LIMIT_LOG), 1);
   PUSH_DATA (push, 0xf);

   BEGIN_NVC0(push, SUBC_COMPUTE(0x02a0), 1);
   PUSH_DATA (push, 0x8000);

   /* global memory setup */
   BEGIN_NVC0(push, SUBC_COMPUTE(0x02c4), 1);
   PUSH_DATA (push, 0);
   BEGIN_NIC0(push, NVC0_COMPUTE(GLOBAL_BASE), 0x100);
   for (i = 0; i <= 0xff; i++)
      PUSH_DATA (push, (0xc << 28) | (i << 16) | i);
   BEGIN_NVC0(push, SUBC_COMPUTE(0x02c4), 1);
   PUSH_DATA (push, 1);

   /* local memory and cstack setup */
   BEGIN_NVC0(push, NVC0_COMPUTE(TEMP_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->tls->offset);
   PUSH_DATA (push, screen->tls->offset);
   BEGIN_NVC0(push, NVC0_COMPUTE(TEMP_SIZE_HIGH), 2);
   PUSH_DATAh(push, screen->tls->size);
   PUSH_DATA (push, screen->tls->size);
   BEGIN_NVC0(push, NVC0_COMPUTE(WARP_TEMP_ALLOC), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_COMPUTE(LOCAL_BASE), 1);
   PUSH_DATA (push, 1 << 24);

   /* shared memory setup */
   BEGIN_NVC0(push, NVC0_COMPUTE(CACHE_SPLIT), 1);
   PUSH_DATA (push, NVC0_COMPUTE_CACHE_SPLIT_48K_SHARED_16K_L1);
   BEGIN_NVC0(push, NVC0_COMPUTE(SHARED_BASE), 1);
   PUSH_DATA (push, 2 << 24);
   BEGIN_NVC0(push, NVC0_COMPUTE(SHARED_SIZE), 1);
   PUSH_DATA (push, 0);

   /* code segment setup */
   BEGIN_NVC0(push, NVC0_COMPUTE(CODE_ADDRESS_HIGH), 2);
   PUSH_DATAh(push, screen->text->offset);
   PUSH_DATA (push, screen->text->offset);

   /* bind parameters buffer */
   BEGIN_NVC0(push, NVC0_COMPUTE(CB_SIZE), 3);
   PUSH_DATA (push, screen->parm->size);
   PUSH_DATAh(push, screen->parm->offset);
   PUSH_DATA (push, screen->parm->offset);
   BEGIN_NVC0(push, NVC0_COMPUTE(CB_BIND), 1);
   PUSH_DATA (push, (0 << 8) | 1);

   /* TODO: textures & samplers */

   return 0;
}

boolean
nvc0_compute_validate_program(struct nvc0_context *nvc0)
{
   struct nvc0_program *prog = nvc0->compprog;

   if (prog->mem)
      return TRUE;

   if (!prog->translated) {
      prog->translated = nvc0_program_translate(
         prog, nvc0->screen->base.device->chipset);
      if (!prog->translated)
         return FALSE;
   }
   if (unlikely(!prog->code_size))
      return FALSE;

   if (likely(prog->code_size)) {
      if (nvc0_program_upload_code(nvc0, prog)) {
         struct nouveau_pushbuf *push = nvc0->base.pushbuf;
         BEGIN_NVC0(push, NVC0_COMPUTE(FLUSH), 1);
         PUSH_DATA (push, NVC0_COMPUTE_FLUSH_CODE);
         return TRUE;
      }
   }
   return FALSE;
}

static boolean
nvc0_compute_state_validate(struct nvc0_context *nvc0)
{
   if (!nvc0_compute_validate_program(nvc0))
      return FALSE;

   /* TODO: textures, samplers, surfaces, global memory buffers */

   nvc0_bufctx_fence(nvc0, nvc0->bufctx_cp, FALSE);

   nouveau_pushbuf_bufctx(nvc0->base.pushbuf, nvc0->bufctx_cp);
   if (unlikely(nouveau_pushbuf_validate(nvc0->base.pushbuf)))
      return FALSE;
   if (unlikely(nvc0->state.flushed))
      nvc0_bufctx_fence(nvc0, nvc0->bufctx_cp, TRUE);

   return TRUE;

}

static void
nvc0_compute_upload_input(struct nvc0_context *nvc0, const void *input)
{
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_screen *screen = nvc0->screen;
   struct nvc0_program *cp = nvc0->compprog;

   if (cp->parm_size) {
      BEGIN_NVC0(push, NVC0_COMPUTE(CB_SIZE), 3);
      PUSH_DATA (push, align(cp->parm_size, 0x100));
      PUSH_DATAh(push, screen->parm->offset);
      PUSH_DATA (push, screen->parm->offset);
      BEGIN_NVC0(push, NVC0_COMPUTE(CB_BIND), 1);
      PUSH_DATA (push, (0 << 8) | 1);
      /* NOTE: size is limited to 4 KiB, which is < NV04_PFIFO_MAX_PACKET_LEN */
      BEGIN_1IC0(push, NVC0_COMPUTE(CB_POS), 1 + cp->parm_size / 4);
      PUSH_DATA (push, 0);
      PUSH_DATAp(push, input, cp->parm_size / 4);

      BEGIN_NVC0(push, NVC0_COMPUTE(FLUSH), 1);
      PUSH_DATA (push, NVC0_COMPUTE_FLUSH_CB);
   }
}

void
nvc0_launch_grid(struct pipe_context *pipe,
                 const uint *block_layout, const uint *grid_layout,
                 uint32_t label,
                 const void *input)
{
   struct nvc0_context *nvc0 = nvc0_context(pipe);
   struct nouveau_pushbuf *push = nvc0->base.pushbuf;
   struct nvc0_program *cp = nvc0->compprog;
   unsigned s, i;
   int ret;

   ret = !nvc0_compute_state_validate(nvc0);
   if (ret)
      goto out;

   nvc0_compute_upload_input(nvc0, input);

   BEGIN_NVC0(push, NVC0_COMPUTE(CP_START_ID), 1);
   PUSH_DATA (push, nvc0_program_symbol_offset(cp, label));

   BEGIN_NVC0(push, NVC0_COMPUTE(LOCAL_POS_ALLOC), 3);
   PUSH_DATA (push, align(cp->cp.lmem_size, 0x10));
   PUSH_DATA (push, 0);
   PUSH_DATA (push, 0x800); /* WARP_CSTACK_SIZE */

   BEGIN_NVC0(push, NVC0_COMPUTE(SHARED_SIZE), 3);
   PUSH_DATA (push, align(cp->cp.smem_size, 0x100));
   PUSH_DATA (push, block_layout[0] * block_layout[1] * block_layout[2]);
   PUSH_DATA (push, cp->num_barriers);
   BEGIN_NVC0(push, NVC0_COMPUTE(CP_GPR_ALLOC), 1);
   PUSH_DATA (push, cp->num_gprs);

   /* grid/block setup */
   BEGIN_NVC0(push, NVC0_COMPUTE(GRIDDIM_YX), 2);
   PUSH_DATA (push, (grid_layout[1] << 16) | grid_layout[0]);
   PUSH_DATA (push, grid_layout[2]);
   BEGIN_NVC0(push, NVC0_COMPUTE(BLOCKDIM_YX), 2);
   PUSH_DATA (push, (block_layout[1] << 16) | block_layout[0]);
   PUSH_DATA (push, block_layout[2]);

   /* launch preliminary setup */
   BEGIN_NVC0(push, NVC0_COMPUTE(GRIDID), 1);
   PUSH_DATA (push, 0x1);
   BEGIN_NVC0(push, SUBC_COMPUTE(0x036c), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_COMPUTE(FLUSH), 1);
   PUSH_DATA (push, NVC0_COMPUTE_FLUSH_GLOBAL | NVC0_COMPUTE_FLUSH_UNK8);

   /* kernel launching */
   BEGIN_NVC0(push, NVC0_COMPUTE(COMPUTE_BEGIN), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, SUBC_COMPUTE(0x0a08), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, NVC0_COMPUTE(LAUNCH), 1);
   PUSH_DATA (push, 0x1000);
   BEGIN_NVC0(push, NVC0_COMPUTE(COMPUTE_END), 1);
   PUSH_DATA (push, 0);
   BEGIN_NVC0(push, SUBC_COMPUTE(0x0360), 1);
   PUSH_DATA (push, 0x1);

   /* rebind all the 3D constant buffers
    * (looks like binding a CB on COMPUTE clobbers 3D state) */
   nvc0->dirty |= NVC0_NEW_CONSTBUF;
   for (s = 0; s < 6; s++) {
      for (i = 0; i < NVC0_MAX_PIPE_CONSTBUFS; i++)
         if (nvc0->constbuf[s][i].u.buf)
            nvc0->constbuf_dirty[s] |= 1 << i;
   }
   memset(nvc0->state.uniform_buffer_bound, 0,
          sizeof(nvc0->state.uniform_buffer_bound));

out:
   if (ret)
      NOUVEAU_ERR("Failed to launch grid !\n");
}
