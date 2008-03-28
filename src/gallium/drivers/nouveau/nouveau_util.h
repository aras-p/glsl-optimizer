#ifndef __NOUVEAU_UTIL_H__
#define __NOUVEAU_UTIL_H__

/* Determine how many vertices can be pushed into the command stream.
 * Where the remaining space isn't large enough to represent all verices,
 * split the buffer at primitive boundaries.
 *
 * Returns a count of vertices that can be rendered, and an index to
 * restart drawing at after a flush.
 */
static INLINE unsigned
nouveau_vbuf_split(unsigned remaining, unsigned overhead, unsigned vpp,
		   unsigned mode, unsigned start, unsigned count,
		   unsigned *restart)
{
	int max, adj = 0;

	max  = remaining - overhead;
	if (max < 0)
		return 0;

	max *= vpp;
	if (max >= count)
		return count;

	switch (mode) {
	case PIPE_PRIM_POINTS:
		break;
	case PIPE_PRIM_LINES:
		max = max & 1;
		break;
	case PIPE_PRIM_TRIANGLES:
		max = max - (max % 3);
		break;
	case PIPE_PRIM_QUADS:
		max = max & 3;
		break;
	case PIPE_PRIM_LINE_LOOP:
	case PIPE_PRIM_LINE_STRIP:
		if (max < 2)
			max = 0;
		adj = 1;
		break;
	case PIPE_PRIM_POLYGON:
	case PIPE_PRIM_TRIANGLE_STRIP:
	case PIPE_PRIM_TRIANGLE_FAN:
		if (max < 3)
			max = 0;
		adj = 2;
		break;
	case PIPE_PRIM_QUAD_STRIP:
		if (max < 4)
			max = 0;
		adj = 3;
		break;
	default:
		assert(0);
	}

	*restart = start + max - adj;
	return max;
}

#endif
