#include <assert.h>
#include <X11/Xlib.h>
#include <X11/extensions/XvMC.h>
#include <vl_context.h>
#include <vl_surface.h>

static enum VL_PICTURE PictureToVL(int xvmc_pic)
{
	enum VL_PICTURE vl_pic;
	
	switch (xvmc_pic)
	{
		case XVMC_TOP_FIELD:
		{
			vl_pic = VL_TOP_FIELD;
			break;
		}
		case XVMC_BOTTOM_FIELD:
		{
			vl_pic = VL_BOTTOM_FIELD;
			break;
		}
		case XVMC_FRAME_PICTURE:
		{
			vl_pic = VL_FRAME_PICTURE;
			break;
		}
		default:
			assert(0);
	}
	
	return vl_pic;
}

static enum VL_MC_TYPE MotionToVL(int xvmc_motion_type)
{
	enum VL_MC_TYPE vl_mc_type;
	
	switch (xvmc_motion_type)
	{
		case XVMC_PREDICTION_FRAME:
		{
			vl_mc_type = VL_FRAME_MC;
			break;
		}
		case XVMC_PREDICTION_FIELD:
		{
			vl_mc_type = VL_FIELD_MC;
			break;
		}
		case XVMC_PREDICTION_DUAL_PRIME:
		{
			vl_mc_type = VL_DUAL_PRIME_MC;
			break;
		}
		default:
			assert(0);
	}
	
	return vl_mc_type;
}

Status XvMCCreateSurface(Display *display, XvMCContext *context, XvMCSurface *surface)
{
	struct VL_CONTEXT *vl_ctx;
	struct VL_SURFACE *vl_sfc;
	
	assert(display);
	
	if (!context)
		return XvMCBadContext;
	if (!surface)
		return XvMCBadSurface;
	
	vl_ctx = context->privData;
	
	assert(display == vl_ctx->display);
	
	vlCreateSurface(vl_ctx, &vl_sfc);
	
	surface->surface_id = XAllocID(display);
	surface->context_id = context->context_id;
	surface->surface_type_id = context->surface_type_id;
	surface->width = context->width;
	surface->height = context->height;
	surface->privData = vl_sfc;
	
	return Success;
}

Status XvMCRenderSurface
(
	Display *display,
	XvMCContext *context,
	unsigned int picture_structure,
	XvMCSurface *target_surface,
	XvMCSurface *past_surface,
	XvMCSurface *future_surface,
	unsigned int flags,
	unsigned int num_macroblocks,
	unsigned int first_macroblock,
	XvMCMacroBlockArray *macroblocks,
	XvMCBlockArray *blocks
)
{
	struct VL_CONTEXT	*vl_ctx;
	struct VL_SURFACE	*target_vl_surface;
	struct VL_SURFACE	*past_vl_surface;
	struct VL_SURFACE	*future_vl_surface;
	unsigned int		i;
	
	assert(display);
	
	if (!context)
		return XvMCBadContext;
	if (!target_surface)
		return XvMCBadSurface;
	
	if
	(
		picture_structure != XVMC_TOP_FIELD &&
		picture_structure != XVMC_BOTTOM_FIELD &&
		picture_structure != XVMC_FRAME_PICTURE
	)
		return BadValue;
	if (future_surface && !past_surface)
		return BadMatch;
	
	vl_ctx = context->privData;
	
	assert(display == vl_ctx->display);
	
	target_vl_surface = target_surface->privData;
	past_vl_surface = past_surface ? past_surface->privData : NULL;
	future_vl_surface = future_surface ? future_surface->privData : NULL;
	
	assert(vl_ctx == target_vl_surface->context);
	assert(!past_vl_surface || vl_ctx == past_vl_surface->context);
	assert(!future_vl_surface || vl_ctx == future_vl_surface->context);
	
	assert(macroblocks);
	assert(blocks);
	
	assert(macroblocks->context_id == context->context_id);
	assert(blocks->context_id == context->context_id);
	
	assert(flags == 0 || flags == XVMC_SECOND_FIELD);
	
	for (i = first_macroblock; i < first_macroblock + num_macroblocks; ++i)
		if (macroblocks->macro_blocks[i].macroblock_type & XVMC_MB_TYPE_INTRA)
			vlRenderIMacroBlock
			(
				PictureToVL(picture_structure),
				flags == XVMC_SECOND_FIELD ? VL_FIELD_SECOND : VL_FIELD_FIRST,
				macroblocks->macro_blocks[i].x,
				macroblocks->macro_blocks[i].y,
				macroblocks->macro_blocks[i].coded_block_pattern,
				macroblocks->macro_blocks[i].dct_type == XVMC_DCT_TYPE_FIELD ? VL_DCT_FIELD_CODED : VL_DCT_FRAME_CODED,
				blocks->blocks + (macroblocks->macro_blocks[i].index * 64),
				target_vl_surface
			);
		else if
		(
			(macroblocks->macro_blocks[i].macroblock_type & (XVMC_MB_TYPE_MOTION_FORWARD | XVMC_MB_TYPE_MOTION_BACKWARD))
			== XVMC_MB_TYPE_MOTION_FORWARD
		)
		{
			struct VL_MOTION_VECTOR motion_vector =
			{
				{
					macroblocks->macro_blocks[i].PMV[0][0][0],
					macroblocks->macro_blocks[i].PMV[0][0][1],
				},
				{
					macroblocks->macro_blocks[i].PMV[1][0][0],
					macroblocks->macro_blocks[i].PMV[1][0][1],
				}
			};
						
			vlRenderPMacroBlock
			(
				PictureToVL(picture_structure),
				flags == XVMC_SECOND_FIELD ? VL_FIELD_SECOND : VL_FIELD_FIRST,
				macroblocks->macro_blocks[i].x,
				macroblocks->macro_blocks[i].y,
				MotionToVL(macroblocks->macro_blocks[i].motion_type),
				&motion_vector,
				macroblocks->macro_blocks[i].coded_block_pattern,
				macroblocks->macro_blocks[i].dct_type == XVMC_DCT_TYPE_FIELD ? VL_DCT_FIELD_CODED : VL_DCT_FRAME_CODED,
				blocks->blocks + (macroblocks->macro_blocks[i].index * 64),
				past_vl_surface,
				target_vl_surface
			);
		}
		else if
		(
			(macroblocks->macro_blocks[i].macroblock_type & (XVMC_MB_TYPE_MOTION_FORWARD | XVMC_MB_TYPE_MOTION_BACKWARD))
			== XVMC_MB_TYPE_MOTION_BACKWARD
		)
		{
			struct VL_MOTION_VECTOR motion_vector =
			{
				{
					macroblocks->macro_blocks[i].PMV[0][1][0],
					macroblocks->macro_blocks[i].PMV[0][1][1],
				},
				{
					macroblocks->macro_blocks[i].PMV[1][1][0],
					macroblocks->macro_blocks[i].PMV[1][1][1],
				}
			};
			
			vlRenderPMacroBlock
			(
				PictureToVL(picture_structure),
				flags == XVMC_SECOND_FIELD ? VL_FIELD_SECOND : VL_FIELD_FIRST,
				macroblocks->macro_blocks[i].x,
				macroblocks->macro_blocks[i].y,
				MotionToVL(macroblocks->macro_blocks[i].motion_type),
				&motion_vector,
				macroblocks->macro_blocks[i].coded_block_pattern,
				macroblocks->macro_blocks[i].dct_type == XVMC_DCT_TYPE_FIELD ? VL_DCT_FIELD_CODED : VL_DCT_FRAME_CODED,
				blocks->blocks + (macroblocks->macro_blocks[i].index * 64),
				future_vl_surface,
				target_vl_surface
			);
		}
		else if
		(
			(macroblocks->macro_blocks[i].macroblock_type & (XVMC_MB_TYPE_MOTION_FORWARD | XVMC_MB_TYPE_MOTION_BACKWARD))
			== (XVMC_MB_TYPE_MOTION_FORWARD | XVMC_MB_TYPE_MOTION_BACKWARD)
		)
		{
			struct VL_MOTION_VECTOR motion_vector[2] =
			{
				{
					{
						macroblocks->macro_blocks[i].PMV[0][0][0],
						macroblocks->macro_blocks[i].PMV[0][0][1],
					},
					{
						macroblocks->macro_blocks[i].PMV[1][0][0],
						macroblocks->macro_blocks[i].PMV[1][0][1],
					}
				},
				{
					{
						macroblocks->macro_blocks[i].PMV[0][1][0],
						macroblocks->macro_blocks[i].PMV[0][1][1],
					},
					{
						macroblocks->macro_blocks[i].PMV[1][1][0],
						macroblocks->macro_blocks[i].PMV[1][1][1],
					}
				}
			};
			
			vlRenderBMacroBlock
			(
				PictureToVL(picture_structure),
				flags == XVMC_SECOND_FIELD ? VL_FIELD_SECOND : VL_FIELD_FIRST,
				macroblocks->macro_blocks[i].x,
				macroblocks->macro_blocks[i].y,
				MotionToVL(macroblocks->macro_blocks[i].motion_type),
				motion_vector,
				macroblocks->macro_blocks[i].coded_block_pattern,
				macroblocks->macro_blocks[i].dct_type == XVMC_DCT_TYPE_FIELD ? VL_DCT_FIELD_CODED : VL_DCT_FRAME_CODED,
				blocks->blocks + (macroblocks->macro_blocks[i].index * 64),
				past_vl_surface,
				future_vl_surface,
				target_vl_surface
			);
		}
		else
			fprintf(stderr, "Unrecognized macroblock\n");
	
	return Success;
}

Status XvMCFlushSurface(Display *display, XvMCSurface *surface)
{
	/* TODO: Check display & surface match */
	return BadImplementation;
}

Status XvMCSyncSurface(Display *display, XvMCSurface *surface)
{
	/* TODO: Check display & surface match */
	return BadImplementation;
}

Status XvMCPutSurface
(
	Display *display,
	XvMCSurface *surface,
	Drawable drawable,
	short srcx,
	short srcy,
	unsigned short srcw,
	unsigned short srch,
	short destx,
	short desty,
	unsigned short destw,
	unsigned short desth,
	int flags
)
{
	Window			root;
	int			x, y;
	unsigned int		width, height;
	unsigned int		border_width;
	unsigned int		depth;
	struct VL_SURFACE	*vl_sfc;
	
	assert(display);
	
	if (!surface)
		return XvMCBadSurface;
		
	if (XGetGeometry(display, drawable, &root, &x, &y, &width, &height, &border_width, &depth) == BadDrawable)
		return BadDrawable;
	
	assert(flags == XVMC_TOP_FIELD || flags == XVMC_BOTTOM_FIELD || flags == XVMC_FRAME_PICTURE);
	
	/* TODO: Correct for negative srcx,srcy & destx,desty by clipping */
	
	assert(srcx + srcw - 1 < surface->width);
	assert(srcy + srch - 1 < surface->height);
	assert(destx + destw - 1 < width);
	assert(desty + desth - 1 < height);
	
	vl_sfc = surface->privData;
	
	vlPutSurface(vl_sfc, drawable, srcx, srcy, srcw, srch, destx, desty, destw, desth, PictureToVL(flags));
	
	return Success;
}

Status XvMCGetSurfaceStatus(Display *display, XvMCSurface *surface, int *status)
{
	struct VL_CONTEXT *vl_ctx;
	struct VL_SURFACE *vl_sfc;
	
	assert(display);
	
	if (!surface)
		return XvMCBadSurface;
		
	assert(status);
	
	vl_sfc = surface->privData;
	vl_ctx = vl_sfc->context;
	
	assert(display == vl_ctx->display);
	
	/* TODO */
	*status = 0;
	
	return BadImplementation;
}

Status XvMCDestroySurface(Display *display, XvMCSurface *surface)
{
	struct VL_CONTEXT *vl_ctx;
	struct VL_SURFACE *vl_sfc;
	
	assert(display);
	
	if (!surface)
		return XvMCBadSurface;
	
	vl_sfc = surface->privData;
	vl_ctx = vl_sfc->context;
	
	assert(display == vl_ctx->display);
	
	vlDestroySurface(vl_sfc);
	
	return Success;
}

Status XvMCHideSurface(Display *display, XvMCSurface *surface)
{
	struct VL_CONTEXT *vl_ctx;
	struct VL_SURFACE *vl_sfc;
	
	assert(display);
	
	if (!surface)
		return XvMCBadSurface;
	
	vl_sfc = surface->privData;
	vl_ctx = vl_sfc->context;
	
	assert(display == vl_ctx->display);
	
	/* No op, only for overlaid rendering */
	
	return Success;
}

