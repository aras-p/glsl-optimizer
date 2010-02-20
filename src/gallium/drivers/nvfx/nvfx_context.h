#ifndef __NVFX_CONTEXT_H__
#define __NVFX_CONTEXT_H__

#include <stdio.h>

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "pipe/p_compiler.h"

#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_inlines.h"

#include "draw/draw_vertex.h"

#include "nouveau/nouveau_winsys.h"
#include "nouveau/nouveau_gldefs.h"
#include "nouveau/nouveau_context.h"
#include "nouveau/nouveau_stateobj.h"

#include "nvfx_state.h"

#define NOUVEAU_ERR(fmt, args...) \
	fprintf(stderr, "%s:%d -  "fmt, __func__, __LINE__, ##args);
#define NOUVEAU_MSG(fmt, args...) \
	fprintf(stderr, "nouveau: "fmt, ##args);

enum nvfx_state_index {
	NVFX_STATE_FB = 0,
	NVFX_STATE_VIEWPORT = 1,
	NVFX_STATE_BLEND = 2,
	NVFX_STATE_RAST = 3,
	NVFX_STATE_ZSA = 4,
	NVFX_STATE_BCOL = 5,
	NVFX_STATE_CLIP = 6,
	NVFX_STATE_SCISSOR = 7,
	NVFX_STATE_STIPPLE = 8,
	NVFX_STATE_FRAGPROG = 9,
	NVFX_STATE_VERTPROG = 10,
	NVFX_STATE_FRAGTEX0 = 11,
	NVFX_STATE_FRAGTEX1 = 12,
	NVFX_STATE_FRAGTEX2 = 13,
	NVFX_STATE_FRAGTEX3 = 14,
	NVFX_STATE_FRAGTEX4 = 15,
	NVFX_STATE_FRAGTEX5 = 16,
	NVFX_STATE_FRAGTEX6 = 17,
	NVFX_STATE_FRAGTEX7 = 18,
	NVFX_STATE_FRAGTEX8 = 19,
	NVFX_STATE_FRAGTEX9 = 20,
	NVFX_STATE_FRAGTEX10 = 21,
	NVFX_STATE_FRAGTEX11 = 22,
	NVFX_STATE_FRAGTEX12 = 23,
	NVFX_STATE_FRAGTEX13 = 24,
	NVFX_STATE_FRAGTEX14 = 25,
	NVFX_STATE_FRAGTEX15 = 26,
	NVFX_STATE_VERTTEX0 = 27,
	NVFX_STATE_VERTTEX1 = 28,
	NVFX_STATE_VERTTEX2 = 29,
	NVFX_STATE_VERTTEX3 = 30,
	NVFX_STATE_VTXBUF = 31,
	NVFX_STATE_VTXFMT = 32,
	NVFX_STATE_VTXATTR = 33,
	NVFX_STATE_SR = 34,
	NVFX_STATE_MAX = 35
};

#include "nvfx_screen.h"

#define NVFX_NEW_BLEND		(1 <<  0)
#define NVFX_NEW_RAST		(1 <<  1)
#define NVFX_NEW_ZSA		(1 <<  2)
#define NVFX_NEW_SAMPLER	(1 <<  3)
#define NVFX_NEW_FB		(1 <<  4)
#define NVFX_NEW_STIPPLE	(1 <<  5)
#define NVFX_NEW_SCISSOR	(1 <<  6)
#define NVFX_NEW_VIEWPORT	(1 <<  7)
#define NVFX_NEW_BCOL		(1 <<  8)
#define NVFX_NEW_VERTPROG	(1 <<  9)
#define NVFX_NEW_FRAGPROG	(1 << 10)
#define NVFX_NEW_ARRAYS		(1 << 11)
#define NVFX_NEW_UCP		(1 << 12)
#define NVFX_NEW_SR		(1 << 13)

struct nvfx_rasterizer_state {
	struct pipe_rasterizer_state pipe;
	struct nouveau_stateobj *so;
};

struct nvfx_zsa_state {
	struct pipe_depth_stencil_alpha_state pipe;
	struct nouveau_stateobj *so;
};

struct nvfx_blend_state {
	struct pipe_blend_state pipe;
	struct nouveau_stateobj *so;
};


struct nvfx_state {
	unsigned scissor_enabled;
	unsigned stipple_enabled;
	unsigned fp_samplers;

	uint64_t dirty;
	struct nouveau_stateobj *hw[NVFX_STATE_MAX];
};

struct nvfx_vtxelt_state {
	struct pipe_vertex_element pipe[16];
	unsigned num_elements;
};

struct nvfx_context {
	struct pipe_context pipe;

	struct nouveau_winsys *nvws;
	struct nvfx_screen *screen;

	unsigned is_nv4x; /* either 0 or ~0 */

	struct draw_context *draw;

	/* HW state derived from pipe states */
	struct nvfx_state state;
	struct {
		struct nvfx_vertex_program *vertprog;

		unsigned nr_attribs;
		unsigned hw[PIPE_MAX_SHADER_INPUTS];
		unsigned draw[PIPE_MAX_SHADER_INPUTS];
		unsigned emit[PIPE_MAX_SHADER_INPUTS];
	} swtnl;

	enum {
		HW, SWTNL, SWRAST
	} render_mode;
	unsigned fallback_swtnl;
	unsigned fallback_swrast;

	/* Context state */
	unsigned dirty, draw_dirty;
	struct pipe_scissor_state scissor;
	unsigned stipple[32];
	struct pipe_clip_state clip;
	struct nvfx_vertex_program *vertprog;
	struct nvfx_fragment_program *fragprog;
	struct pipe_buffer *constbuf[PIPE_SHADER_TYPES];
	unsigned constbuf_nr[PIPE_SHADER_TYPES];
	struct nvfx_rasterizer_state *rasterizer;
	struct nvfx_zsa_state *zsa;
	struct nvfx_blend_state *blend;
	struct pipe_blend_color blend_colour;
	struct pipe_stencil_ref stencil_ref;
	struct pipe_viewport_state viewport;
	struct pipe_framebuffer_state framebuffer;
	struct pipe_buffer *idxbuf;
	unsigned idxbuf_format;
	struct nvfx_sampler_state *tex_sampler[PIPE_MAX_SAMPLERS];
	struct nvfx_miptree *tex_miptree[PIPE_MAX_SAMPLERS];
	unsigned nr_samplers;
	unsigned nr_textures;
	unsigned dirty_samplers;
	struct pipe_vertex_buffer vtxbuf[PIPE_MAX_ATTRIBS];
	unsigned vtxbuf_nr;
	struct nvfx_vtxelt_state *vtxelt;
};

static INLINE struct nvfx_context *
nvfx_context(struct pipe_context *pipe)
{
	return (struct nvfx_context *)pipe;
}

struct nvfx_state_entry {
	boolean (*validate)(struct nvfx_context *nvfx);
	struct {
		unsigned pipe;
		unsigned hw;
	} dirty;
};

extern struct nvfx_state_entry nvfx_state_blend;
extern struct nvfx_state_entry nvfx_state_blend_colour;
extern struct nvfx_state_entry nvfx_state_rasterizer;

/* nvfx_clear.c */
extern void nvfx_clear(struct pipe_context *pipe, unsigned buffers,
		       const float *rgba, double depth, unsigned stencil);

/* nvfx_state_emit.c */
extern void nvfx_state_flush_notify(struct nouveau_channel *chan);
extern boolean nvfx_state_validate(struct nvfx_context *nvfx);
extern boolean nvfx_state_validate_swtnl(struct nvfx_context *nvfx);
extern void nvfx_state_emit(struct nvfx_context *nvfx);

/* nvfx_transfer.c */
extern void nvfx_init_transfer_functions(struct nvfx_context *nvfx);

#endif
