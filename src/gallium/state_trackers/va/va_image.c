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

#include <util/u_memory.h>
#include <util/u_format.h>
#include <va/va.h>
#include <va/va_backend.h>
#include "va_private.h"

VAStatus
vlVaQueryImageFormats ( 	VADriverContextP ctx,
                            VAImageFormat *format_list,
                            int *num_formats)
{
	if (!ctx)
		return VA_STATUS_ERROR_INVALID_CONTEXT;


	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus vlVaCreateImage(	VADriverContextP ctx,
                            VAImageFormat *format,
                            int width,
                            int height,
                            VAImage *image)
{
	if (!ctx)
		return VA_STATUS_ERROR_INVALID_CONTEXT;

	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus vlVaDeriveImage(	VADriverContextP ctx,
                            VASurfaceID surface,
                            VAImage *image)
{
	if (!ctx)
		return VA_STATUS_ERROR_INVALID_CONTEXT;


	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus vlVaDestroyImage(	VADriverContextP ctx,
                            VAImageID image)
{
	if (!ctx)
		return VA_STATUS_ERROR_INVALID_CONTEXT;


	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus vlVaSetImagePalette(	VADriverContextP ctx,
                            VAImageID image,
                            unsigned char *palette)
{
	if (!ctx)
		return VA_STATUS_ERROR_INVALID_CONTEXT;


	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus vlVaGetImage(		VADriverContextP ctx,
                            VASurfaceID surface,
                            int x,
                            int y,
                            unsigned int width,
                            unsigned int height,
                            VAImageID image)
{
	if (!ctx)
		return VA_STATUS_ERROR_INVALID_CONTEXT;


	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus vlVaPutImage(		VADriverContextP ctx,
                            VASurfaceID surface,
                            VAImageID image,
                            int src_x,
                            int src_y,
                            unsigned int src_width,
                            unsigned int src_height,
                            int dest_x,
                            int dest_y,
                            unsigned int dest_width,
                            unsigned int dest_height)
{
	if (!ctx)
		return VA_STATUS_ERROR_INVALID_CONTEXT;


	return VA_STATUS_ERROR_UNIMPLEMENTED;
}
