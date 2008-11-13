#include <assert.h>
#include <X11/Xlib.h>
#include <X11/extensions/XvMC.h>
#include <vl_display.h>
#include <vl_screen.h>
#include <vl_context.h>
#include <vl_surface.h>
#include <vl_types.h>

static enum vlMacroBlockType TypeToVL(int xvmc_mb_type)
{
	if (xvmc_mb_type & XVMC_MB_TYPE_INTRA)
		return vlMacroBlockTypeIntra;
	if ((xvmc_mb_type & (XVMC_MB_TYPE_MOTION_FORWARD | XVMC_MB_TYPE_MOTION_BACKWARD)) == XVMC_MB_TYPE_MOTION_FORWARD)
		return vlMacroBlockTypeFwdPredicted;
	if ((xvmc_mb_type & (XVMC_MB_TYPE_MOTION_FORWARD | XVMC_MB_TYPE_MOTION_BACKWARD)) == XVMC_MB_TYPE_MOTION_BACKWARD)
		return vlMacroBlockTypeBkwdPredicted;
	if ((xvmc_mb_type & (XVMC_MB_TYPE_MOTION_FORWARD | XVMC_MB_TYPE_MOTION_BACKWARD)) == (XVMC_MB_TYPE_MOTION_FORWARD | XVMC_MB_TYPE_MOTION_BACKWARD))
		return vlMacroBlockTypeBiPredicted;

	assert(0);

	return -1;
}

static enum vlPictureType PictureToVL(int xvmc_pic)
{
	switch (xvmc_pic)
	{
		case XVMC_TOP_FIELD:
			return vlPictureTypeTopField;
		case XVMC_BOTTOM_FIELD:
			return vlPictureTypeBottomField;
		case XVMC_FRAME_PICTURE:
			return vlPictureTypeFrame;
		default:
			assert(0);
	}

	return -1;
}

static enum vlMotionType MotionToVL(int xvmc_motion_type, int xvmc_dct_type)
{
	switch (xvmc_motion_type)
	{
		case XVMC_PREDICTION_FRAME:
			return xvmc_dct_type == XVMC_DCT_TYPE_FIELD ? vlMotionType16x8 : vlMotionTypeFrame;
		case XVMC_PREDICTION_FIELD:
			return vlMotionTypeField;
		case XVMC_PREDICTION_DUAL_PRIME:
			return vlMotionTypeDualPrime;
		default:
			assert(0);
	}

	return -1;
}

Status XvMCCreateSurface(Display *display, XvMCContext *context, XvMCSurface *surface)
{
	struct vlContext *vl_ctx;
	struct vlSurface *vl_sfc;

	assert(display);

	if (!context)
		return XvMCBadContext;
	if (!surface)
		return XvMCBadSurface;

	vl_ctx = context->privData;

	assert(display == vlGetNativeDisplay(vlGetDisplay(vlContextGetScreen(vl_ctx))));

	vlCreateSurface
	(
		vlContextGetScreen(vl_ctx),
		context->width,
		context->height,
		vlGetPictureFormat(vl_ctx),
		&vl_sfc
	);

	vlBindToContext(vl_sfc, vl_ctx);

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
	struct vlContext		*vl_ctx;
	struct vlSurface		*target_vl_surface;
	struct vlSurface		*past_vl_surface;
	struct vlSurface		*future_vl_surface;
	struct vlMpeg2MacroBlockBatch	batch;
	struct vlMpeg2MacroBlock	vl_macroblocks[num_macroblocks];
	unsigned int			i;

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

	assert(display == vlGetNativeDisplay(vlGetDisplay(vlContextGetScreen(vl_ctx))));

	target_vl_surface = target_surface->privData;
	past_vl_surface = past_surface ? past_surface->privData : NULL;
	future_vl_surface = future_surface ? future_surface->privData : NULL;

	assert(context->context_id == target_surface->context_id);
	assert(!past_surface || context->context_id == past_surface->context_id);
	assert(!future_surface || context->context_id == future_surface->context_id);

	assert(macroblocks);
	assert(blocks);

	assert(macroblocks->context_id == context->context_id);
	assert(blocks->context_id == context->context_id);

	assert(flags == 0 || flags == XVMC_SECOND_FIELD);

	batch.past_surface = past_vl_surface;
	batch.future_surface = future_vl_surface;
	batch.picture_type = PictureToVL(picture_structure);
	batch.field_order = flags & XVMC_SECOND_FIELD ? vlFieldOrderSecond : vlFieldOrderFirst;
	batch.num_macroblocks = num_macroblocks;
	batch.macroblocks = vl_macroblocks;

	for (i = 0; i < num_macroblocks; ++i)
	{
		unsigned int j = first_macroblock + i;

		unsigned int k, l, m;

		batch.macroblocks[i].mbx = macroblocks->macro_blocks[j].x;
		batch.macroblocks[i].mby = macroblocks->macro_blocks[j].y;
		batch.macroblocks[i].mb_type = TypeToVL(macroblocks->macro_blocks[j].macroblock_type);
		if (batch.macroblocks[i].mb_type != vlMacroBlockTypeIntra)
			batch.macroblocks[i].mo_type = MotionToVL(macroblocks->macro_blocks[j].motion_type, macroblocks->macro_blocks[j].dct_type);
		batch.macroblocks[i].dct_type = macroblocks->macro_blocks[j].dct_type == XVMC_DCT_TYPE_FIELD ? vlDCTTypeFieldCoded : vlDCTTypeFrameCoded;

		for (k = 0; k < 2; ++k)
			for (l = 0; l < 2; ++l)
				for (m = 0; m < 2; ++m)
					batch.macroblocks[i].PMV[k][l][m] = macroblocks->macro_blocks[j].PMV[k][l][m];

		batch.macroblocks[i].cbp = macroblocks->macro_blocks[j].coded_block_pattern;
		batch.macroblocks[i].blocks = blocks->blocks + (macroblocks->macro_blocks[j].index * 64);
	}

	vlRenderMacroBlocksMpeg2(&batch, target_vl_surface);

	return Success;
}

Status XvMCFlushSurface(Display *display, XvMCSurface *surface)
{
	struct vlSurface *vl_sfc;

	assert(display);

	if (!surface)
		return XvMCBadSurface;

	vl_sfc = surface->privData;

	assert(display == vlGetNativeDisplay(vlGetDisplay(vlSurfaceGetScreen(vl_sfc))));

	vlSurfaceFlush(vl_sfc);

	return Success;
}

Status XvMCSyncSurface(Display *display, XvMCSurface *surface)
{
	struct vlSurface *vl_sfc;

	assert(display);

	if (!surface)
		return XvMCBadSurface;

	vl_sfc = surface->privData;

	assert(display == vlGetNativeDisplay(vlGetDisplay(vlSurfaceGetScreen(vl_sfc))));

	vlSurfaceSync(vl_sfc);

	return Success;
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
	struct vlSurface	*vl_sfc;

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

	vlPutPicture(vl_sfc, drawable, srcx, srcy, srcw, srch, destx, desty, destw, desth, width, height, PictureToVL(flags));

	return Success;
}

Status XvMCGetSurfaceStatus(Display *display, XvMCSurface *surface, int *status)
{
	struct vlSurface	*vl_sfc;
	enum vlResourceStatus	res_status;

	assert(display);

	if (!surface)
		return XvMCBadSurface;

	assert(status);

	vl_sfc = surface->privData;

	assert(display == vlGetNativeDisplay(vlGetDisplay(vlSurfaceGetScreen(vl_sfc))));

	vlSurfaceGetStatus(vl_sfc, &res_status);

	switch (res_status)
	{
		case vlResourceStatusFree:
		{
			*status = 0;
			break;
		}
		case vlResourceStatusRendering:
		{
			*status = XVMC_RENDERING;
			break;
		}
		case vlResourceStatusDisplaying:
		{
			*status = XVMC_DISPLAYING;
			break;
		}
		default:
			assert(0);
	}

	return Success;
}

Status XvMCDestroySurface(Display *display, XvMCSurface *surface)
{
	struct vlSurface *vl_sfc;

	assert(display);

	if (!surface)
		return XvMCBadSurface;

	vl_sfc = surface->privData;

	assert(display == vlGetNativeDisplay(vlGetDisplay(vlSurfaceGetScreen(vl_sfc))));

	vlDestroySurface(vl_sfc);

	return Success;
}

Status XvMCHideSurface(Display *display, XvMCSurface *surface)
{
	struct vlSurface *vl_sfc;

	assert(display);

	if (!surface)
		return XvMCBadSurface;

	vl_sfc = surface->privData;

	assert(display == vlGetNativeDisplay(vlGetDisplay(vlSurfaceGetScreen(vl_sfc))));

	/* No op, only for overlaid rendering */

	return Success;
}
