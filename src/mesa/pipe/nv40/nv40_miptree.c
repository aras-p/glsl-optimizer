#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"

#include "nv40_context.h"

boolean
nv40_miptree_layout(struct pipe_context *pipe, struct pipe_mipmap_tree *mt)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	boolean swizzled = FALSE;
	uint width = mt->width0, height = mt->height0, depth = mt->depth0;
	uint offset;
	int nr_faces, l, f;

	mt->pitch = mt->width0;
	mt->total_height = 0;

	if (mt->target == PIPE_TEXTURE_CUBE) {
		nr_faces = 6;
	} else
	if (mt->target == PIPE_TEXTURE_3D) {
		nr_faces = mt->depth0;
	} else {
		nr_faces = 1;
	}

	for (l = mt->first_level; l <= mt->last_level; l++) {
		mt->level[l].width = width;
		mt->level[l].height = height;
		mt->level[l].depth = depth;
		mt->level[l].level_offset = 0;

		mt->level[l].nr_images = nr_faces;
		mt->level[l].image_offset = malloc(nr_faces * sizeof(unsigned));
		for (f = 0; f < nr_faces; f++)
			mt->total_height += height;

		width  = MAX2(1, width  >> 1);
		height = MAX2(1, height >> 1);
		depth  = MAX2(1, depth  >> 1);
	}

	offset = 0;
	for (f = 0; f < nr_faces; f++) {
		for (l = mt->first_level; l <= mt->last_level; l++) {
			if (f == 0) {
				mt->level[l].level_offset = offset;
			}

			uint pitch;

			if (swizzled)
				pitch = mt->level[l].width * mt->cpp;
			else
				pitch = mt->width0 * mt->cpp;
			pitch = (pitch + 63) & ~63;

			mt->level[l].image_offset[f] =
				(offset - mt->level[l].level_offset) / mt->cpp;
			offset += pitch * mt->level[l].height;
		}
	}

	return TRUE;
}

