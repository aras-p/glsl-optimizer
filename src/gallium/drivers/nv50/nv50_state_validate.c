#include "nv50_context.h"
#include "nouveau/nouveau_stateobj.h"

boolean
nv50_state_validate(struct nv50_context *nv50)
{
	struct nouveau_winsys *nvws = nv50->screen->nvws;

	if (nv50->dirty & NV50_NEW_BLEND)
		so_emit(nvws, nv50->blend->so);

	return TRUE;
}

