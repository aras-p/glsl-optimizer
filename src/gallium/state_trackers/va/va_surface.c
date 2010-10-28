/**************************************************************************
 *
	* Copyright 2010 Thomas Balling Sørensen & Orasanu Lucian.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include <va/va.h>
#include <va/va_backend.h>
#include "va_private.h"

VAStatus vlVaCreateSurfaces(       VADriverContextP ctx,
                                   int width,
                                   int height,
                                   int format,
                                   int num_surfaces,
                                   VASurfaceID *surfaces)
{
	if (!ctx)
		return VA_STATUS_ERROR_INVALID_CONTEXT;

	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus vlVaDestroySurfaces(       VADriverContextP ctx,
                                    VASurfaceID *surface_list,
                                    int num_surfaces)
{
	if (!ctx)
		return VA_STATUS_ERROR_INVALID_CONTEXT;

	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus vlVaSyncSurface(       VADriverContextP ctx,
                                VASurfaceID render_target)
{
	if (!ctx)
		return VA_STATUS_ERROR_INVALID_CONTEXT;

	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus vlVaQuerySurfaceStatus(       VADriverContextP ctx,
                                       VASurfaceID render_target,
                                       VASurfaceStatus *status)
{
	if (!ctx)
		return VA_STATUS_ERROR_INVALID_CONTEXT;

	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus vlVaPutSurface(       VADriverContextP ctx,
                               VASurfaceID surface,
                               void* draw,
                               short srcx,
                               short srcy,
                               unsigned short srcw,
                               unsigned short srch,
                               short destx,
                               short desty,
                               unsigned short destw,
                               unsigned short desth,
                               VARectangle *cliprects,
                               unsigned int number_cliprects,
                               unsigned int flags)
{
	if (!ctx)
		return VA_STATUS_ERROR_INVALID_CONTEXT;

	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus vlVaLockSurface(	VADriverContextP ctx,
                            VASurfaceID surface,
                            unsigned int *fourcc,
                            unsigned int *luma_stride,
                            unsigned int *chroma_u_stride,
                            unsigned int *chroma_v_stride,
                            unsigned int *luma_offset,
                            unsigned int *chroma_u_offset,
                            unsigned int *chroma_v_offset,
                            unsigned int *buffer_name,
                            void **buffer)
{
	if (!ctx)
		return VA_STATUS_ERROR_INVALID_CONTEXT;

	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus vlVaUnlockSurface(	VADriverContextP ctx,
                            VASurfaceID surface)
{
	if (!ctx)
		return VA_STATUS_ERROR_INVALID_CONTEXT;

	return VA_STATUS_ERROR_UNIMPLEMENTED;
}
