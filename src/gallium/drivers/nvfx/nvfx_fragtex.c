#include "nvfx_context.h"
#include "nvfx_resource.h"

void
nvfx_fragtex_validate(struct nvfx_context *nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	unsigned samplers, unit;

	samplers = nvfx->dirty_samplers;
	if(!samplers)
		return;

	while (samplers) {
		unit = ffs(samplers) - 1;
		samplers &= ~(1 << unit);

		if(nvfx->fragment_sampler_views[unit] && nvfx->tex_sampler[unit]) {
			if(!nvfx->is_nv4x)
				nv30_fragtex_set(nvfx, unit);
			else
				nv40_fragtex_set(nvfx, unit);
		} else {
			WAIT_RING(chan, 2);
			/* this is OK for nv40 too */
			OUT_RING(chan, RING_3D(NV34TCL_TX_ENABLE(unit), 1));
			OUT_RING(chan, 0);
			nvfx->hw_samplers &= ~(1 << unit);
		}
	}
	nvfx->dirty_samplers = 0;
}

void
nvfx_fragtex_relocate(struct nvfx_context *nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	unsigned samplers, unit;
	unsigned tex_flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD;

	samplers = nvfx->hw_samplers;
	while (samplers) {
		struct nvfx_miptree* mt;
		struct nouveau_bo *bo;

		unit = ffs(samplers) - 1;
		samplers &= ~(1 << unit);

		mt = (struct nvfx_miptree*)nvfx->fragment_sampler_views[unit]->texture;
		bo = mt->base.bo;

		MARK_RING(chan, 3, 3);
		OUT_RELOC(chan, bo, RING_3D(NV34TCL_TX_OFFSET(unit), 2), tex_flags | NOUVEAU_BO_DUMMY, 0, 0);
		OUT_RELOC(chan, bo, 0, tex_flags | NOUVEAU_BO_LOW | NOUVEAU_BO_DUMMY, 0, 0);
		OUT_RELOC(chan, bo, nvfx->hw_txf[unit], tex_flags | NOUVEAU_BO_OR | NOUVEAU_BO_DUMMY,
				NV34TCL_TX_FORMAT_DMA0, NV34TCL_TX_FORMAT_DMA1);
	}
}
