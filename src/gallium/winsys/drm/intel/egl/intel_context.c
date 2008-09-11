/**************************************************************************
 *
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "i915simple/i915_screen.h"

#include "intel_device.h"
#include "intel_context.h"
#include "intel_batchbuffer.h"

#include "state_tracker/st_public.h"
#include "pipe/p_defines.h"
#include "pipe/p_context.h"
#include "intel_egl.h"
#include "utils.h"

#ifdef DEBUG
int __intel_debug = 0;
#endif


#define need_GL_ARB_multisample
#define need_GL_ARB_point_parameters
#define need_GL_ARB_texture_compression
#define need_GL_ARB_vertex_buffer_object
#define need_GL_ARB_vertex_program
#define need_GL_ARB_window_pos
#define need_GL_EXT_blend_color
#define need_GL_EXT_blend_equation_separate
#define need_GL_EXT_blend_func_separate
#define need_GL_EXT_blend_minmax
#define need_GL_EXT_cull_vertex
#define need_GL_EXT_fog_coord
#define need_GL_EXT_framebuffer_object
#define need_GL_EXT_multi_draw_arrays
#define need_GL_EXT_secondary_color
#define need_GL_NV_vertex_program
#include "extension_helper.h"


/**
 * Extension strings exported by the intel driver.
 *
 * \note
 * It appears that ARB_texture_env_crossbar has "disappeared" compared to the
 * old i830-specific driver.
 */
const struct dri_extension card_extensions[] = {
	{"GL_ARB_multisample", GL_ARB_multisample_functions},
	{"GL_ARB_multitexture", NULL},
	{"GL_ARB_point_parameters", GL_ARB_point_parameters_functions},
	{"GL_ARB_texture_border_clamp", NULL},
	{"GL_ARB_texture_compression", GL_ARB_texture_compression_functions},
	{"GL_ARB_texture_cube_map", NULL},
	{"GL_ARB_texture_env_add", NULL},
	{"GL_ARB_texture_env_combine", NULL},
	{"GL_ARB_texture_env_dot3", NULL},
	{"GL_ARB_texture_mirrored_repeat", NULL},
	{"GL_ARB_texture_rectangle", NULL},
	{"GL_ARB_vertex_buffer_object", GL_ARB_vertex_buffer_object_functions},
	{"GL_ARB_pixel_buffer_object", NULL},
	{"GL_ARB_vertex_program", GL_ARB_vertex_program_functions},
	{"GL_ARB_window_pos", GL_ARB_window_pos_functions},
	{"GL_EXT_blend_color", GL_EXT_blend_color_functions},
	{"GL_EXT_blend_equation_separate", GL_EXT_blend_equation_separate_functions},
	{"GL_EXT_blend_func_separate", GL_EXT_blend_func_separate_functions},
	{"GL_EXT_blend_minmax", GL_EXT_blend_minmax_functions},
	{"GL_EXT_blend_subtract", NULL},
	{"GL_EXT_cull_vertex", GL_EXT_cull_vertex_functions},
	{"GL_EXT_fog_coord", GL_EXT_fog_coord_functions},
	{"GL_EXT_framebuffer_object", GL_EXT_framebuffer_object_functions},
	{"GL_EXT_multi_draw_arrays", GL_EXT_multi_draw_arrays_functions},
	{"GL_EXT_packed_depth_stencil", NULL},
	{"GL_EXT_pixel_buffer_object", NULL},
	{"GL_EXT_secondary_color", GL_EXT_secondary_color_functions},
	{"GL_EXT_stencil_wrap", NULL},
	{"GL_EXT_texture_edge_clamp", NULL},
	{"GL_EXT_texture_env_combine", NULL},
	{"GL_EXT_texture_env_dot3", NULL},
	{"GL_EXT_texture_filter_anisotropic", NULL},
	{"GL_EXT_texture_lod_bias", NULL},
	{"GL_3DFX_texture_compression_FXT1", NULL},
	{"GL_APPLE_client_storage", NULL},
	{"GL_MESA_pack_invert", NULL},
	{"GL_MESA_ycbcr_texture", NULL},
	{"GL_NV_blend_square", NULL},
	{"GL_NV_vertex_program", GL_NV_vertex_program_functions},
	{"GL_NV_vertex_program1_1", NULL},
	{"GL_SGIS_generate_mipmap", NULL },
	{NULL, NULL}
};


/*
 * Hardware lock functions.
 * Doesn't do anything in EGL
 */

static void
intel_lock_hardware(struct intel_be_context *context)
{
	(void)context;
}

static void
intel_unlock_hardware(struct intel_be_context *context)
{
	(void)context;
}

static boolean
intel_locked_hardware(struct intel_be_context *context)
{
	(void)context;
	return FALSE;
}


/*
 * Misc functions.
 */

int
intel_create_context(struct egl_drm_context *egl_context, const __GLcontextModes *visual, void *sharedContextPrivate)
{
	struct intel_context *intel = CALLOC_STRUCT(intel_context);
	struct intel_device *device = (struct intel_device *)egl_context->device->priv;
	struct pipe_context *pipe;
	struct st_context *st_share = NULL;

	egl_context->priv = intel;

	intel->intel_device = device;
	intel->egl_context = egl_context;
	intel->egl_device = egl_context->device;

	intel->base.hardware_lock = intel_lock_hardware;
	intel->base.hardware_unlock = intel_unlock_hardware;
	intel->base.hardware_locked = intel_locked_hardware;

	intel_be_init_context(&intel->base, &device->base);

#if 0
	pipe = intel_create_softpipe(intel, screen->winsys);
#else
	pipe = i915_create_context(device->pipe, &device->base.base, &intel->base.base);
#endif

	pipe->priv = intel;

	intel->st = st_create_context(pipe, visual, st_share);

	device->dummy = intel;

	return TRUE;
}

int
intel_destroy_context(struct egl_drm_context *egl_context)
{
	struct intel_context *intel = egl_context->priv;

	if (intel->intel_device->dummy == intel)
		intel->intel_device->dummy = NULL;

	st_destroy_context(intel->st);
	intel_be_destroy_context(&intel->base);
	free(intel);
	return TRUE;
}

void
intel_make_current(struct egl_drm_context *context, struct egl_drm_drawable *draw, struct egl_drm_drawable *read)
{
	if (context) {
		struct intel_context *intel = (struct intel_context *)context->priv;
		struct intel_framebuffer *draw_fb = (struct intel_framebuffer *)draw->priv;
		struct intel_framebuffer *read_fb = (struct intel_framebuffer *)read->priv;

		assert(draw_fb->stfb);
		assert(read_fb->stfb);

		st_make_current(intel->st, draw_fb->stfb, read_fb->stfb);

		intel->egl_drawable = draw;

		st_resize_framebuffer(draw_fb->stfb, draw->w, draw->h);

		if (draw != read)
			st_resize_framebuffer(read_fb->stfb, read->w, read->h);

	} else {
		st_make_current(NULL, NULL, NULL);
	}
}

void
intel_bind_frontbuffer(struct egl_drm_drawable *draw, struct egl_drm_frontbuffer *front)
{
	struct intel_device *device = (struct intel_device *)draw->device->priv;
	struct intel_framebuffer *draw_fb = (struct intel_framebuffer *)draw->priv;

	if (draw_fb->front_buffer)
		driBOUnReference(draw_fb->front_buffer);

	draw_fb->front_buffer = NULL;
	draw_fb->front = NULL;

	/* to unbind just call this function with front == NULL */
	if (!front)
		return;

	draw_fb->front = front;

	driGenBuffers(device->base.staticPool, "front", 1, &draw_fb->front_buffer, 0, 0, 0);
	driBOSetReferenced(draw_fb->front_buffer, front->handle);

	st_resize_framebuffer(draw_fb->stfb, draw->w, draw->h);
}
