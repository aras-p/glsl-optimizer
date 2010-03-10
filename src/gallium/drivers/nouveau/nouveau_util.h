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

/* Integer base-2 logarithm, rounded towards zero. */
static INLINE unsigned log2i(unsigned i)
{
	unsigned r = 0;

	if (i & 0xffff0000) {
		i >>= 16;
		r += 16;
	}
	if (i & 0x0000ff00) {
		i >>= 8;
		r += 8;
	}
	if (i & 0x000000f0) {
		i >>= 4;
		r += 4;
	}
	if (i & 0x0000000c) {
		i >>= 2;
		r += 2;
	}
	if (i & 0x00000002) {
		r += 1;
	}
	return r;
}

struct u_split_prim {
   void *priv;
   void (*emit)(void *priv, unsigned start, unsigned count);
   void (*edge)(void *priv, boolean enabled);

   unsigned mode;
   unsigned start;
   unsigned p_start;
   unsigned p_end;

   int repeat_first:1;
   int close_first:1;
   int edgeflag_off:1;
};

static inline void
u_split_prim_init(struct u_split_prim *s,
                  unsigned mode, unsigned start, unsigned count)
{
   if (mode == PIPE_PRIM_LINE_LOOP) {
      s->mode = PIPE_PRIM_LINE_STRIP;
      s->close_first = 1;
   } else {
      s->mode = mode;
      s->close_first = 0;
   }
   s->start = start;
   s->p_start = start;
   s->p_end = start + count;
   s->edgeflag_off = 0;
   s->repeat_first = 0;
}

static INLINE boolean
u_split_prim_next(struct u_split_prim *s, unsigned max_verts)
{
   int repeat = 0;

   if (s->repeat_first) {
      s->emit(s->priv, s->start, 1);
      max_verts--;
      if (s->edgeflag_off) {
         s->edge(s->priv, TRUE);
         s->edgeflag_off = FALSE;
      }
   }

   if (s->p_start + s->close_first + max_verts >= s->p_end) {
      s->emit(s->priv, s->p_start, s->p_end - s->p_start);
      if (s->close_first)
         s->emit(s->priv, s->start, 1);
      return TRUE;
   }

   switch (s->mode) {
   case PIPE_PRIM_LINES:
      max_verts &= ~1;
      break;
   case PIPE_PRIM_LINE_STRIP:
      repeat = 1;
      break;
   case PIPE_PRIM_POLYGON:
      max_verts--;
      s->emit(s->priv, s->p_start, max_verts);
      s->edge(s->priv, FALSE);
      s->emit(s->priv, s->p_start + max_verts, 1);
      s->p_start += max_verts;
      s->repeat_first = TRUE;
      s->edgeflag_off = TRUE;
      return FALSE;
   case PIPE_PRIM_TRIANGLES:
      max_verts = max_verts - (max_verts % 3);
      break;
   case PIPE_PRIM_TRIANGLE_STRIP:
      /* to ensure winding stays correct, always split
       * on an even number of generated triangles
       */
      max_verts = max_verts & ~1;
      repeat = 2;
      break;
   case PIPE_PRIM_TRIANGLE_FAN:
      s->repeat_first = TRUE;
      repeat = 1;
      break;
   case PIPE_PRIM_QUADS:
      max_verts &= ~3;
      break;
   case PIPE_PRIM_QUAD_STRIP:
      max_verts &= ~1;
      repeat = 2;
      break;
   default:
      break;
   }

   s->emit (s->priv, s->p_start, max_verts);
   s->p_start += (max_verts - repeat);
   return FALSE;
}

#endif
