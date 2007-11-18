#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"

#include "nv40_context.h"

boolean
nv40_miptree_layout(struct pipe_context *pipe, struct pipe_mipmap_tree *mt)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	uint width, height, depth, offset;
	boolean swizzled = FALSE;
	int l;

	mt->pitch = mt->width0;
	mt->total_height = 0;

	width = mt->width0;
	height = mt->height0;
	depth = mt->depth0;
	offset = 0;
	for (l = mt->first_level; l <= mt->last_level; l++) {
		uint pitch, f;

		mt->level[l].width = width;
		mt->level[l].height = height;
		mt->level[l].depth = depth;
		mt->level[l].level_offset = offset;

		if (!swizzled)
			pitch = mt->width0;
		else
			pitch = width;

		if (mt->target == PIPE_TEXTURE_CUBE)
			mt->level[l].nr_images = 6;
		else
		if (mt->target == PIPE_TEXTURE_3D)
			mt->level[l].nr_images = mt->level[l].depth;
		else
			mt->level[l].nr_images = 1;
		mt->level[l].image_offset =
			malloc(mt->level[l].nr_images * sizeof(unsigned));

		for (f = 0; f < mt->level[l].nr_images; f++) {
			mt->level[l].image_offset[f] =
				(offset - mt->level[l].level_offset) / mt->cpp;
			mt->total_height += height;

			offset += (pitch * mt->cpp * height);
		}

		width  = MAX2(1, width  >> 1);
		height = MAX2(1, height >> 1);
		depth  = MAX2(1, depth  >> 1);
	}

	return TRUE;
}

