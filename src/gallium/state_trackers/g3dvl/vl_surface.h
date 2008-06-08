#ifndef vl_surface_h
#define vl_surface_h

#include <X11/Xlib.h>
#include "vl_types.h"

struct pipe_texture;

struct VL_SURFACE
{
	struct VL_CONTEXT	*context;
	unsigned int		width;
	unsigned int		height;
	enum VL_FORMAT		format;
	struct pipe_texture	*texture;
};

int vlCreateSurface(struct VL_CONTEXT *context, struct VL_SURFACE **surface);

int vlDestroySurface(struct VL_SURFACE *surface);

int vlRenderIMacroBlock
(
	enum VL_PICTURE picture_type,
	enum VL_FIELD_ORDER field_order,
	unsigned int mbx,
	unsigned int mby,
	unsigned int coded_block_pattern,
	enum VL_DCT_TYPE dct_type,
	short *blocks,
	struct VL_SURFACE *surface
);

int vlRenderPMacroBlock
(
	enum VL_PICTURE picture_type,
	enum VL_FIELD_ORDER field_order,
	unsigned int mbx,
	unsigned int mby,
	enum VL_MC_TYPE mc_type,
	struct VL_MOTION_VECTOR *motion_vector,
	unsigned int coded_block_pattern,
	enum VL_DCT_TYPE dct_type,
	short *blocks,
	struct VL_SURFACE *ref_surface,
	struct VL_SURFACE *surface
);

int vlRenderBMacroBlock
(
	enum VL_PICTURE picture_type,
	enum VL_FIELD_ORDER field_order,
	unsigned int mbx,
	unsigned int mby,
	enum VL_MC_TYPE mc_type,
	struct VL_MOTION_VECTOR *motion_vector,
	unsigned int coded_block_pattern,
	enum VL_DCT_TYPE dct_type,
	short *blocks,
	struct VL_SURFACE *past_surface,
	struct VL_SURFACE *future_surface,
	struct VL_SURFACE *surface
);

int vlPutSurface
(
	struct VL_SURFACE *surface,
	Drawable drawable,
	unsigned int srcx,
	unsigned int srcy,
	unsigned int srcw,
	unsigned int srch,
	unsigned int destx,
	unsigned int desty,
	unsigned int destw,
	unsigned int desth,
	enum VL_PICTURE picture_type
);

#endif

