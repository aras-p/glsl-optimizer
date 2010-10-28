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

#include <pipe/p_compiler.h>
#include <pipe/p_video_context.h>
#include <util/u_debug.h>
#include <va/va.h>
#include <va/va_backend.h>
#include "va_private.h"

//struct VADriverVTable vlVaGetVtable();

PUBLIC
VAStatus __vaDriverInit_0_31 (VADriverContextP ctx)
{
	if (!ctx)
		return VA_STATUS_ERROR_INVALID_CONTEXT;

	ctx->str_vendor = "mesa gallium vaapi";
	ctx->vtable = vlVaGetVtable();
	ctx->max_attributes = 1;
	ctx->max_display_attributes = 1;
	ctx->max_entrypoints = 1;
	ctx->max_image_formats = 1;
	ctx->max_profiles = 1;
	ctx->max_subpic_formats = 1;
	ctx->version_major = 3;
	ctx->version_minor = 1;

	VA_INFO("vl_screen_pointer %p\n",ctx->native_dpy);

	return VA_STATUS_SUCCESS;
}

VAStatus vlVaCreateContext(       VADriverContextP ctx,
                                  VAConfigID config_id,
                                  int picture_width,
                                  int picture_height,
                                  int flag,
                                  VASurfaceID *render_targets,
                                  int num_render_targets,
                                  VAContextID *conext)
{
	if (!ctx)
		return VA_STATUS_ERROR_INVALID_CONTEXT;

	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus vlVaDestroyContext(       VADriverContextP ctx,
                                   VAContextID context)
{
	if (!ctx)
		return VA_STATUS_ERROR_INVALID_CONTEXT;

	return VA_STATUS_ERROR_UNIMPLEMENTED;
}

VAStatus vlVaTerminate(       VADriverContextP ctx)
{
	if (!ctx)
		return VA_STATUS_ERROR_INVALID_CONTEXT;
	return VA_STATUS_ERROR_UNIMPLEMENTED;
}
