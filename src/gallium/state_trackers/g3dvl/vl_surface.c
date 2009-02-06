#define VL_INTERNAL
#include "vl_surface.h"
#include <assert.h>
#include <string.h>
#include <pipe/p_screen.h>
#include <pipe/p_state.h>
#include <pipe/p_inlines.h>
#include <util/u_memory.h>
#include <vl_winsys.h>
#include "vl_screen.h"
#include "vl_context.h"
#include "vl_render.h"
#include "vl_csc.h"
#include "vl_util.h"

int vlCreateSurface
(
	struct vlScreen *screen,
	unsigned int width,
	unsigned int height,
	enum vlFormat format,
	struct vlSurface **surface
)
{
	struct vlSurface	*sfc;
	struct pipe_texture	template;

	assert(screen);
	assert(surface);

	sfc = CALLOC_STRUCT(vlSurface);

	if (!sfc)
		return 1;

	sfc->screen = screen;
	sfc->width = width;
	sfc->height = height;
	sfc->format = format;

	memset(&template, 0, sizeof(struct pipe_texture));
	template.target = PIPE_TEXTURE_2D;
	template.format = PIPE_FORMAT_A8R8G8B8_UNORM;
	template.last_level = 0;
	template.width[0] = vlRoundUpPOT(sfc->width);
	template.height[0] = vlRoundUpPOT(sfc->height);
	template.depth[0] = 1;
	template.compressed = 0;
	pf_get_block(template.format, &template.block);
	template.tex_usage = PIPE_TEXTURE_USAGE_SAMPLER | PIPE_TEXTURE_USAGE_RENDER_TARGET;

	sfc->texture = vlGetPipeScreen(screen)->texture_create(vlGetPipeScreen(screen), &template);

	if (!sfc->texture)
	{
		FREE(sfc);
		return 1;
	}

	*surface = sfc;

	return 0;
}

int vlDestroySurface
(
	struct vlSurface *surface
)
{
	assert(surface);

	pipe_texture_reference(&surface->texture, NULL);
	FREE(surface);

	return 0;
}

int vlRenderMacroBlocksMpeg2
(
	struct vlMpeg2MacroBlockBatch *batch,
	struct vlSurface *surface
)
{
	assert(batch);
	assert(surface);
	assert(surface->context);

	surface->context->render->vlBegin(surface->context->render);

	surface->context->render->vlRenderMacroBlocksMpeg2
	(
		surface->context->render,
		batch,
		surface
	);

	surface->context->render->vlEnd(surface->context->render);

	return 0;
}

int vlPutPicture
(
	struct vlSurface *surface,
	vlNativeDrawable drawable,
	int srcx,
	int srcy,
	int srcw,
	int srch,
	int destx,
	int desty,
	int destw,
	int desth,
	int drawable_w,
	int drawable_h,
	enum vlPictureType picture_type
)
{
	struct vlCSC		*csc;
	struct pipe_context	*pipe;

	assert(surface);
	assert(surface->context);

	surface->context->render->vlFlush(surface->context->render);

	csc = surface->context->csc;
	pipe = surface->context->pipe;

	csc->vlResizeFrameBuffer(csc, drawable_w, drawable_h);

	csc->vlBegin(csc);

	csc->vlPutPicture
	(
		csc,
		surface,
		srcx,
		srcy,
		srcw,
		srch,
		destx,
		desty,
		destw,
		desth,
		picture_type
	);

	csc->vlEnd(csc);

	pipe->flush(pipe, PIPE_FLUSH_RENDER_CACHE, &surface->disp_fence);

	bind_pipe_drawable(pipe, drawable);

	pipe->screen->flush_frontbuffer
	(
		pipe->screen,
		csc->vlGetFrameBuffer(csc),
		pipe->priv
	);

	return 0;
}

int vlSurfaceGetStatus
(
	struct vlSurface *surface,
	enum vlResourceStatus *status
)
{
	assert(surface);
	assert(surface->context);
	assert(status);

	if (surface->render_fence && !surface->context->pipe->screen->fence_signalled(surface->context->pipe->screen, surface->render_fence, 0))
	{
		*status = vlResourceStatusRendering;
		return 0;
	}

	if (surface->disp_fence && !surface->context->pipe->screen->fence_signalled(surface->context->pipe->screen, surface->disp_fence, 0))
	{
		*status = vlResourceStatusDisplaying;
		return 0;
	}

	*status = vlResourceStatusFree;

	return 0;
}

int vlSurfaceFlush
(
	struct vlSurface *surface
)
{
	assert(surface);
	assert(surface->context);

	surface->context->render->vlFlush(surface->context->render);

	return 0;
}

int vlSurfaceSync
(
	struct vlSurface *surface
)
{
	assert(surface);
	assert(surface->context);
	assert(surface->render_fence);

	surface->context->pipe->screen->fence_finish(surface->context->pipe->screen, surface->render_fence, 0);

	return 0;
}

struct vlScreen* vlSurfaceGetScreen
(
	struct vlSurface *surface
)
{
	assert(surface);

	return surface->screen;
}

struct vlContext* vlBindToContext
(
	struct vlSurface *surface,
	struct vlContext *context
)
{
	struct vlContext *old;

	assert(surface);

	old = surface->context;
	surface->context = context;

	return old;
}
