#include "nvfx_context.h"

static boolean
nvfx_fragtex_validate(struct nvfx_context *nvfx)
{
	struct nvfx_fragment_program *fp = nvfx->fragprog;
	struct nvfx_state *state = &nvfx->state;
	struct nouveau_stateobj *so;
	unsigned samplers, unit;

	samplers = state->fp_samplers & ~fp->samplers;
	while (samplers) {
		unit = ffs(samplers) - 1;
		samplers &= ~(1 << unit);

		so = so_new(1, 1, 0);
		so_method(so, nvfx->screen->eng3d, NV34TCL_TX_ENABLE(unit), 1);
		so_data  (so, 0);
		so_ref(so, &nvfx->state.hw[NVFX_STATE_FRAGTEX0 + unit]);
		so_ref(NULL, &so);
		state->dirty |= (1ULL << (NVFX_STATE_FRAGTEX0 + unit));
	}

	samplers = nvfx->dirty_samplers & fp->samplers;
	while (samplers) {
		unit = ffs(samplers) - 1;
		samplers &= ~(1 << unit);

		if(!nvfx->is_nv4x)
			so = nv30_fragtex_build(nvfx, unit);
		else
			so = nv40_fragtex_build(nvfx, unit);

		so_ref(so, &nvfx->state.hw[NVFX_STATE_FRAGTEX0 + unit]);
		so_ref(NULL, &so);
		state->dirty |= (1ULL << (NVFX_STATE_FRAGTEX0 + unit));
	}

	nvfx->state.fp_samplers = fp->samplers;
	return FALSE;
}

struct nvfx_state_entry nvfx_state_fragtex = {
	.validate = nvfx_fragtex_validate,
	.dirty = {
		.pipe = NVFX_NEW_SAMPLER | NVFX_NEW_FRAGPROG,
		.hw = 0
	}
};
