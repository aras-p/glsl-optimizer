#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"
#include "pipe/p_inlines.h"

#include "nv30_context.h"

static void
nv30_miptree_layout(struct nv30_miptree *nv30mt)
{
	struct pipe_texture *pt = &nv30mt->base;
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
	
	for (l = 0; l <= pt->last_level; l++) {
		pt->width[l] = width;
		pt->height[l] = height;
		pt->depth[l] = depth;

		if (swizzled)
			nv30mt->level[l].pitch = pt->width[l] * pt->cpp;
		else
			nv30mt->level[l].pitch = pt->width[0] * pt->cpp;
		nv30mt->level[l].pitch = (nv30mt->level[l].pitch + 63) & ~63;

		nv30mt->level[l].image_offset =
			CALLOC(nr_faces, sizeof(unsigned));

		width  = MAX2(1, width  >> 1);
		height = MAX2(1, height >> 1);
		depth  = MAX2(1, depth  >> 1);

	}

	for (f = 0; f < nr_faces; f++) {
		for (l = 0; l <= pt->last_level; l++) {
			nv30mt->level[l].image_offset[f] = offset;
			offset += nv30mt->level[l].pitch * pt->height[l];
		}
	}

	nv30mt->total_size = offset;
}

static void
nv30_miptree_create(struct pipe_context *pipe, struct pipe_texture **pt)
{
	struct pipe_winsys *ws = pipe->winsys;
	struct nv30_miptree *nv30mt;

	nv30mt = realloc(*pt, sizeof(struct nv30_miptree));
	if (!nv30mt)
		return;
	*pt = NULL;

	nv30_miptree_layout(nv30mt);

	nv30mt->buffer = ws->buffer_create(ws, 256, PIPE_BUFFER_USAGE_PIXEL,
					   nv30mt->total_size);
	if (!nv30mt->buffer) {
		free(nv30mt);
		return;
	}
	
	*pt = &nv30mt->base;
}

static void
nv30_miptree_release(struct pipe_context *pipe, struct pipe_texture **pt)
{
	struct pipe_winsys *ws = pipe->winsys;
	struct pipe_texture *mt = *pt;

	*pt = NULL;
	if (--mt->refcount <= 0) {
		struct nv30_miptree *nv30mt = (struct nv30_miptree *)mt;
		int l;

		pipe_buffer_reference(ws, &nv30mt->buffer, NULL);
		for (l = 0; l <= mt->last_level; l++) {
			if (nv30mt->level[l].image_offset)
				free(nv30mt->level[l].image_offset);
		}
		free(nv30mt);
	}
}

void
nv30_init_miptree_functions(struct nv30_context *nv30)
{
	nv30->pipe.texture_create = nv30_miptree_create;
	nv30->pipe.texture_release = nv30_miptree_release;
}

