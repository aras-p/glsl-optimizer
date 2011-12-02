#include "draw/draw_context.h"
#include "pipe/p_defines.h"
#include "util/u_framebuffer.h"
#include "vl/vl_decoder.h"
#include "vl/vl_video_buffer.h"

#include "nvfx_context.h"
#include "nvfx_screen.h"
#include "nvfx_resource.h"

static void
nvfx_flush(struct pipe_context *pipe,
	   struct pipe_fence_handle **fence)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	struct nvfx_screen *screen = nvfx->screen;
	struct nouveau_channel *chan = screen->base.channel;
	/*struct nouveau_grobj *eng3d = screen->eng3d;*/

	/* XXX: we need to actually be intelligent here */
        /* XXX This flag wasn't set by the state tracker anyway. */
        /*if (flags & PIPE_FLUSH_TEXTURE_CACHE) {
		BEGIN_RING(chan, eng3d, 0x1fd8, 1);
		OUT_RING(chan, 2);
		BEGIN_RING(chan, eng3d, 0x1fd8, 1);
		OUT_RING(chan, 1);
        }*/

	if (fence) {
		/* horrific hack to make glFinish() work in the absence of
		 * having proper fences in nvfx.  a pending rewrite will
		 * fix this properly, but may be a while off.
		 */
		MARK_RING(chan, 1, 1);
		OUT_RELOC(chan, screen->fence, 0, NOUVEAU_BO_WR |
				NOUVEAU_BO_DUMMY, 0, 0);
		FIRE_RING(chan);
		nouveau_bo_map(screen->fence, NOUVEAU_BO_RDWR);
		nouveau_bo_unmap(screen->fence);
		*fence = NULL;
	} else {
		FIRE_RING(chan);
	}
}

static void
nvfx_destroy(struct pipe_context *pipe)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);

	if(nvfx->dummy_fs)
		pipe->delete_fs_state(pipe, nvfx->dummy_fs);

	for(unsigned i = 0; i < nvfx->vtxbuf_nr; ++i)
		pipe_resource_reference(&nvfx->vtxbuf[i].buffer, 0);
	pipe_resource_reference(&nvfx->idxbuf.buffer, 0);
	util_unreference_framebuffer_state(&nvfx->framebuffer);
	for(unsigned i = 0; i < PIPE_MAX_SAMPLERS; ++i)
		pipe_sampler_view_reference(&nvfx->fragment_sampler_views[i], 0);

	if (nvfx->draw)
		draw_destroy(nvfx->draw);

	if(nvfx->screen->cur_ctx == nvfx)
		nvfx->screen->cur_ctx = NULL;

	FREE(nvfx);
}

struct pipe_context *
nvfx_create(struct pipe_screen *pscreen, void *priv)
{
	struct nvfx_screen *screen = nvfx_screen(pscreen);
	struct nvfx_context *nvfx;

	nvfx = CALLOC(1, sizeof(struct nvfx_context));
	if (!nvfx)
		return NULL;
	nvfx->screen = screen;

	nvfx->pipe.screen = pscreen;
	nvfx->pipe.priv = priv;
	nvfx->pipe.destroy = nvfx_destroy;
	nvfx->pipe.draw_vbo = nvfx_draw_vbo;
	nvfx->pipe.clear = nvfx_clear;
	nvfx->pipe.flush = nvfx_flush;

	nvfx->is_nv4x = screen->is_nv4x;
	nvfx->use_nv4x = screen->use_nv4x;
	/* TODO: it seems that nv30 might have fixed function clipping usable with vertex programs
	 * However, my code for that doesn't work, so use vp clipping for all cards, which works.
	 */
	nvfx->use_vp_clipping = TRUE;

	nvfx_init_query_functions(nvfx);
	nvfx_init_surface_functions(nvfx);
	nvfx_init_state_functions(nvfx);
	nvfx_init_sampling_functions(nvfx);
	nvfx_init_vbo_functions(nvfx);
	nvfx_init_fragprog_functions(nvfx);
	nvfx_init_vertprog_functions(nvfx);
	nvfx_init_resource_functions(&nvfx->pipe);
	nvfx_init_transfer_functions(&nvfx->pipe);

	/* Create, configure, and install fallback swtnl path */
	nvfx->draw = draw_create(&nvfx->pipe);
	draw_wide_point_threshold(nvfx->draw, 9999999.0);
	draw_wide_line_threshold(nvfx->draw, 9999999.0);
	draw_enable_line_stipple(nvfx->draw, FALSE);
	draw_enable_point_sprites(nvfx->draw, FALSE);
	draw_set_rasterize_stage(nvfx->draw, nvfx_draw_render_stage(nvfx));

	/* set these to that we init them on first validation */
	nvfx->state.scissor_enabled = ~0;
	nvfx->hw_pointsprite_control = -1;
	nvfx->hw_vp_output = -1;
	nvfx->use_vertex_buffers = -1;
	nvfx->relocs_needed = NVFX_RELOCATE_ALL;

	LIST_INITHEAD(&nvfx->render_cache);
	nvfx_context_init_vdec(nvfx);

	return &nvfx->pipe;
}
