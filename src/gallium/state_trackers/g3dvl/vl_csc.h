#ifndef vl_csc_h
#define vl_csc_h

#include "vl_types.h"

struct pipe_surface;

struct vlCSC
{
	int (*vlResizeFrameBuffer)
	(
		struct vlCSC *csc,
		unsigned int width,
		unsigned int height
	);

	int (*vlBegin)
	(
		struct vlCSC *csc
	);

	int (*vlPutPicture)
	(
		struct vlCSC *csc,
		struct vlSurface *surface,
		int srcx,
		int srcy,
		int srcw,
		int srch,
		int destx,
		int desty,
		int destw,
		int desth,
		enum vlPictureType picture_type
	);

	int (*vlEnd)
	(
		struct vlCSC *csc
	);

	struct pipe_surface* (*vlGetFrameBuffer)
	(
		struct vlCSC *csc
	);

	int (*vlDestroy)
	(
		struct vlCSC *csc
	);
};

#endif
