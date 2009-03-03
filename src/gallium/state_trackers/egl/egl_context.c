
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "egl_tracker.h"

#include "egllog.h"


#include "pipe/p_context.h"
#include "pipe/p_screen.h"

#include "state_tracker/st_public.h"
#include "state_tracker/drm_api.h"

#include "GL/internal/glcore.h"

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
 * TODO HACK! FUGLY!
 * Copied for intel extentions.
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

EGLContext
drm_create_context(_EGLDriver *drv, EGLDisplay dpy, EGLConfig config, EGLContext share_list, const EGLint *attrib_list)
{
	struct drm_device *dev = (struct drm_device *)drv;
	struct drm_context *ctx;
	struct drm_context *share = NULL;
	struct st_context *st_share = NULL;
	_EGLConfig *conf;
	int i;
	__GLcontextModes *visual;

	conf = _eglLookupConfig(drv, dpy, config);
	if (!conf) {
		_eglError(EGL_BAD_CONFIG, "eglCreateContext");
		return EGL_NO_CONTEXT;
	}

	for (i = 0; attrib_list && attrib_list[i] != EGL_NONE; i++) {
		switch (attrib_list[i]) {
			/* no attribs defined for now */
			default:
				_eglError(EGL_BAD_ATTRIBUTE, "eglCreateContext");
				return EGL_NO_CONTEXT;
		}
	}

	ctx = (struct drm_context *) calloc(1, sizeof(struct drm_context));
	if (!ctx)
		goto err_c;

	_eglInitContext(drv, dpy, &ctx->base, config, attrib_list);

	ctx->pipe = drm_api_hooks.create_context(dev->screen);
	if (!ctx->pipe)
		goto err_pipe;

	if (share)
		st_share = share->st;

	visual = drm_visual_from_config(conf);
	ctx->st = st_create_context(ctx->pipe, visual, st_share);
	drm_visual_modes_destroy(visual);

	if (!ctx->st)
		goto err_gl;

	/* generate handle and insert into hash table */
	_eglSaveContext(&ctx->base);
	assert(_eglGetContextHandle(&ctx->base));

	return _eglGetContextHandle(&ctx->base);

err_gl:
	ctx->pipe->destroy(ctx->pipe);
err_pipe:
	free(ctx);
err_c:
	return EGL_NO_CONTEXT;
}

EGLBoolean
drm_destroy_context(_EGLDriver *drv, EGLDisplay dpy, EGLContext context)
{
	struct drm_context *c = lookup_drm_context(context);
	_eglRemoveContext(&c->base);
	if (c->base.IsBound) {
		c->base.DeletePending = EGL_TRUE;
	} else {
		st_destroy_context(c->st);
		c->pipe->destroy(c->pipe);
		free(c);
	}
	return EGL_TRUE;
}

EGLBoolean
drm_make_current(_EGLDriver *drv, EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext context)
{
	struct drm_surface *readSurf = lookup_drm_surface(read);
	struct drm_surface *drawSurf = lookup_drm_surface(draw);
	struct drm_context *ctx = lookup_drm_context(context);
	EGLBoolean b;

	b = _eglMakeCurrent(drv, dpy, draw, read, context);
	if (!b)
		return EGL_FALSE;

	if (ctx) {
		if (!drawSurf || !readSurf)
			return EGL_FALSE;

		drawSurf->user = ctx;
		readSurf->user = ctx;

		st_make_current(ctx->st, drawSurf->stfb, readSurf->stfb);

		/* st_resize_framebuffer needs a bound context to work */
		st_resize_framebuffer(drawSurf->stfb, drawSurf->w, drawSurf->h);
		st_resize_framebuffer(readSurf->stfb, readSurf->w, readSurf->h);
	} else {
		drawSurf->user = NULL;
		readSurf->user = NULL;

		st_make_current(NULL, NULL, NULL);
	}

	return EGL_TRUE;
}
