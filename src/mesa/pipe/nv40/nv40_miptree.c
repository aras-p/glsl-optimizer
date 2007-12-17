#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"

#include "nv40_context.h"

static void
nv40_miptree_layout(struct nv40_miptree *nv40mt)
{
	struct pipe_texture *pt = &nv40mt->base;
	boolean swizzled = FALSE;
	uint width = pt->width[0], height = pt->height[0], depth = pt->depth[0];
	uint offset = 0;
	int nr_faces, l, f;

	if (pt->target == PIPE_TEXTURE_CUBE) {
		nr_faces = 6;
	} else
	if (pt->target == PIPE_TEXTURE_3D) {
		nr_faces = pt->depth[0];
	} else {
		nr_faces = 1;
	}
	
	for (l = pt->first_level; l <= pt->last_level; l++) {
		pt->width[l] = width;
		pt->height[l] = height;
		pt->depth[l] = depth;

		if (swizzled)
			nv40mt->level[l].pitch = pt->width[l] * pt->cpp;
		else
			nv40mt->level[l].pitch = pt->width[0] * pt->cpp;
		nv40mt->level[l].pitch = (nv40mt->level[l].pitch + 63) & ~63;

		nv40mt->level[l].image_offset =
			calloc(nr_faces, sizeof(unsigned));

		width  = MAX2(1, width  >> 1);
		height = MAX2(1, height >> 1);
		depth  = MAX2(1, depth  >> 1);

	}

	for (f = 0; f < nr_faces; f++) {
		for (l = pt->first_level; l <= pt->last_level; l++) {
			nv40mt->level[l].image_offset[f] = offset;
			offset += nv40mt->level[l].pitch * pt->height[l];
		}
	}

	nv40mt->total_size = offset;
}

static void
nv40_miptree_create(struct pipe_context *pipe, struct pipe_texture **pt)
{
	struct pipe_winsys *ws = pipe->winsys;
	struct nv40_miptree *nv40mt;

	nv40mt = realloc(*pt, sizeof(struct nv40_miptree));
	if (!nv40mt)
		return;
	*pt = NULL;

	nv40_miptree_layout(nv40mt);

	nv40mt->buffer = ws->buffer_create(ws, 256, 0, 0);
	if (!nv40mt->buffer) {
		free(nv40mt);
		return;
	}
	
	ws->buffer_data(ws, nv40mt->buffer, nv40mt->total_size, NULL,
			PIPE_BUFFER_USAGE_PIXEL);
	*pt = &nv40mt->base;
}

static void
nv40_miptree_release(struct pipe_context *pipe, struct pipe_texture **pt)
{
	struct pipe_winsys *ws = pipe->winsys;
	struct pipe_texture *mt = *pt;

	*pt = NULL;
	if (--mt->refcount <= 0) {
		struct nv40_miptree *nv40mt = (struct nv40_miptree *)mt;
		int l;

		ws->buffer_reference(ws, &nv40mt->buffer, NULL);
		for (l = mt->first_level; l <= mt->last_level; l++) {
			if (nv40mt->level[l].image_offset)
				free(nv40mt->level[l].image_offset);
		}
		free(nv40mt);
	}
}

void
nv40_init_miptree_functions(struct nv40_context *nv40)
{
	nv40->pipe.texture_create = nv40_miptree_create;
	nv40->pipe.texture_release = nv40_miptree_release;
}

