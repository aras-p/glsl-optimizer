#define VL_INTERNAL
#include "vl_context.h"
#include <assert.h>
#include <pipe/p_context.h>
#include <pipe/p_state.h>
#include <util/u_memory.h>
#include "vl_render.h"
#include "vl_r16snorm_mc_buf.h"
#include "vl_csc.h"
#include "vl_basic_csc.h"

static int vlInitCommon(struct vlContext *context)
{
	struct pipe_context			*pipe;
	struct pipe_rasterizer_state		rast;
	struct pipe_blend_state			blend;
	struct pipe_depth_stencil_alpha_state	dsa;
	unsigned int				i;

	assert(context);

	pipe = context->pipe;

	rast.flatshade = 1;
	rast.flatshade_first = 0;
	rast.light_twoside = 0;
	rast.front_winding = PIPE_WINDING_CCW;
	rast.cull_mode = PIPE_WINDING_CW;
	rast.fill_cw = PIPE_POLYGON_MODE_FILL;
	rast.fill_ccw = PIPE_POLYGON_MODE_FILL;
	rast.offset_cw = 0;
	rast.offset_ccw = 0;
	rast.scissor = 0;
	rast.poly_smooth = 0;
	rast.poly_stipple_enable = 0;
	rast.point_sprite = 0;
	rast.point_size_per_vertex = 0;
	rast.multisample = 0;
	rast.line_smooth = 0;
	rast.line_stipple_enable = 0;
	rast.line_stipple_factor = 0;
	rast.line_stipple_pattern = 0;
	rast.line_last_pixel = 0;
	rast.bypass_vs_clip_and_viewport = 0;
	rast.line_width = 1;
	rast.point_smooth = 0;
	rast.point_size = 1;
	rast.offset_units = 1;
	rast.offset_scale = 1;
	/*rast.sprite_coord_mode[i] = ;*/
	context->raster = pipe->create_rasterizer_state(pipe, &rast);
	pipe->bind_rasterizer_state(pipe, context->raster);

	blend.blend_enable = 0;
	blend.rgb_func = PIPE_BLEND_ADD;
	blend.rgb_src_factor = PIPE_BLENDFACTOR_ONE;
	blend.rgb_dst_factor = PIPE_BLENDFACTOR_ONE;
	blend.alpha_func = PIPE_BLEND_ADD;
	blend.alpha_src_factor = PIPE_BLENDFACTOR_ONE;
	blend.alpha_dst_factor = PIPE_BLENDFACTOR_ONE;
	blend.logicop_enable = 0;
	blend.logicop_func = PIPE_LOGICOP_CLEAR;
	/* Needed to allow color writes to FB, even if blending disabled */
	blend.colormask = PIPE_MASK_RGBA;
	blend.dither = 0;
	context->blend = pipe->create_blend_state(pipe, &blend);
	pipe->bind_blend_state(pipe, context->blend);

	dsa.depth.enabled = 0;
	dsa.depth.writemask = 0;
	dsa.depth.func = PIPE_FUNC_ALWAYS;
	dsa.depth.occlusion_count = 0;
	for (i = 0; i < 2; ++i)
	{
		dsa.stencil[i].enabled = 0;
		dsa.stencil[i].func = PIPE_FUNC_ALWAYS;
		dsa.stencil[i].fail_op = PIPE_STENCIL_OP_KEEP;
		dsa.stencil[i].zpass_op = PIPE_STENCIL_OP_KEEP;
		dsa.stencil[i].zfail_op = PIPE_STENCIL_OP_KEEP;
		dsa.stencil[i].ref_value = 0;
		dsa.stencil[i].valuemask = 0;
		dsa.stencil[i].writemask = 0;
	}
	dsa.alpha.enabled = 0;
	dsa.alpha.func = PIPE_FUNC_ALWAYS;
	dsa.alpha.ref_value = 0;
	context->dsa = pipe->create_depth_stencil_alpha_state(pipe, &dsa);
	pipe->bind_depth_stencil_alpha_state(pipe, context->dsa);

	return 0;
}

int vlCreateContext
(
	struct vlScreen *screen,
	struct pipe_context *pipe,
	unsigned int picture_width,
	unsigned int picture_height,
	enum vlFormat picture_format,
	enum vlProfile profile,
	enum vlEntryPoint entry_point,
	struct vlContext **context
)
{
	struct vlContext *ctx;

	assert(screen);
	assert(context);
	assert(pipe);

	ctx = CALLOC_STRUCT(vlContext);

	if (!ctx)
		return 1;

	ctx->screen = screen;
	ctx->pipe = pipe;
	ctx->picture_width = picture_width;
	ctx->picture_height = picture_height;
	ctx->picture_format = picture_format;
	ctx->profile = profile;
	ctx->entry_point = entry_point;

	vlInitCommon(ctx);

	vlCreateR16SNormBufferedMC(pipe, picture_width, picture_height, picture_format, &ctx->render);
	vlCreateBasicCSC(pipe, &ctx->csc);

	*context = ctx;

	return 0;
}

int vlDestroyContext
(
	struct vlContext *context
)
{
	assert(context);

	/* XXX: Must unbind shaders before we can delete them for some reason */
	context->pipe->bind_vs_state(context->pipe, NULL);
	context->pipe->bind_fs_state(context->pipe, NULL);

	context->render->vlDestroy(context->render);
	context->csc->vlDestroy(context->csc);

	context->pipe->delete_blend_state(context->pipe, context->blend);
	context->pipe->delete_rasterizer_state(context->pipe, context->raster);
	context->pipe->delete_depth_stencil_alpha_state(context->pipe, context->dsa);

	FREE(context);

	return 0;
}

struct vlScreen* vlContextGetScreen
(
	struct vlContext *context
)
{
	assert(context);

	return context->screen;
}

struct pipe_context* vlGetPipeContext
(
	struct vlContext *context
)
{
	assert(context);

	return context->pipe;
}

unsigned int vlGetPictureWidth
(
	struct vlContext *context
)
{
	assert(context);

	return context->picture_width;
}

unsigned int vlGetPictureHeight
(
	struct vlContext *context
)
{
	assert(context);

	return context->picture_height;
}

enum vlFormat vlGetPictureFormat
(
	struct vlContext *context
)
{
	assert(context);

	return context->picture_format;
}
