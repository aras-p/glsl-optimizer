#include "main/glheader.h"
#include "glapi/glthread.h"
#include <GL/internal/glcore.h>

#include "state_tracker/st_public.h"
#include "pipe/p_defines.h"
#include "pipe/p_context.h"

#include "nouveau_context.h"
#include "nouveau_dri.h"
#include "nouveau_local.h"
#include "nouveau_screen.h"
#include "nouveau_winsys_pipe.h"

#define need_GL_ARB_fragment_program
#define need_GL_ARB_multisample
#define need_GL_ARB_occlusion_query
#define need_GL_ARB_point_parameters
#define need_GL_ARB_texture_compression
#define need_GL_ARB_vertex_program
#define need_GL_ARB_vertex_buffer_object
#define need_GL_EXT_compiled_vertex_array
#define need_GL_EXT_fog_coord
#define need_GL_EXT_secondary_color
#define need_GL_EXT_framebuffer_object
#include "extension_helper.h"

const struct dri_extension common_extensions[] =
{
	{ NULL, 0 }
};

const struct dri_extension nv40_extensions[] =
{
	{ "GL_ARB_fragment_program", NULL },
	{ "GL_ARB_multisample", GL_ARB_multisample_functions },
	{ "GL_ARB_occlusion_query", GL_ARB_occlusion_query_functions },
	{ "GL_ARB_point_parameters", GL_ARB_point_parameters_functions },
	{ "GL_ARB_texture_border_clamp", NULL },
	{ "GL_ARB_texture_compression", GL_ARB_texture_compression_functions },
	{ "GL_ARB_texture_cube_map", NULL },
	{ "GL_ARB_texture_env_add", NULL },
	{ "GL_ARB_texture_env_combine", NULL },
	{ "GL_ARB_texture_env_crossbar", NULL },
	{ "GL_ARB_texture_env_dot3", NULL },
	{ "GL_ARB_texture_mirrored_repeat", NULL },
	{ "GL_ARB_texture_non_power_of_two", NULL },
	{ "GL_ARB_vertex_program", GL_ARB_vertex_program_functions },
	{ "GL_ARB_vertex_buffer_object", GL_ARB_vertex_buffer_object_functions },
	{ "GL_ATI_texture_env_combine3", NULL },
	{ "GL_EXT_compiled_vertex_array", GL_EXT_compiled_vertex_array_functions },
	{ "GL_EXT_fog_coord", GL_EXT_fog_coord_functions },
	{ "GL_EXT_framebuffer_object", GL_EXT_framebuffer_object_functions },
	{ "GL_EXT_secondary_color", GL_EXT_secondary_color_functions },
	{ "GL_EXT_texture_edge_clamp", NULL },
	{ "GL_EXT_texture_env_add", NULL },
	{ "GL_EXT_texture_env_combine", NULL },
	{ "GL_EXT_texture_env_dot3", NULL },
	{ "GL_EXT_texture_mirror_clamp", NULL },
	{ "GL_NV_texture_rectangle", NULL },
	{ NULL, 0 }
};

#ifdef DEBUG
static const struct dri_debug_control debug_control[] = {
	{ "bo", DEBUG_BO },
	{ NULL, 0 }
};
int __nouveau_debug = 0;
#endif

GLboolean
nouveau_context_create(const __GLcontextModes *glVis,
		       __DRIcontextPrivate *driContextPriv,
		       void *sharedContextPrivate)
{
	__DRIscreenPrivate *driScrnPriv = driContextPriv->driScreenPriv;
	struct nouveau_screen  *nv_screen = driScrnPriv->private;
	struct nouveau_context *nv = CALLOC_STRUCT(nouveau_context);
	struct nouveau_device_priv *nvdev;
	struct pipe_context *pipe = NULL;
	struct st_context *st_share = NULL;
	int ret;

	if (sharedContextPrivate) {
		st_share = ((struct nouveau_context *)sharedContextPrivate)->st;
	}

	if ((ret = nouveau_device_get_param(nv_screen->device,
					    NOUVEAU_GETPARAM_CHIPSET_ID,
					    &nv->chipset))) {
		NOUVEAU_ERR("Error determining chipset id: %d\n", ret);
		return GL_FALSE;
	}

	if ((ret = nouveau_channel_alloc(nv_screen->device,
					 0x8003d001, 0x8003d002,
					 &nv->channel))) {
		NOUVEAU_ERR("Error creating GPU channel: %d\n", ret);
		return GL_FALSE;
	}

	driContextPriv->driverPrivate = (void *)nv;
	nv->nv_screen  = nv_screen;
	nv->dri_screen = driScrnPriv;

	nvdev = nouveau_device(nv_screen->device);
	nvdev->ctx  = driContextPriv->hHWContext;
	nvdev->lock = (drmLock *)&driScrnPriv->pSAREA->lock;

	driParseConfigFiles(&nv->dri_option_cache, &nv_screen->option_cache,
			    nv->dri_screen->myNum, "nouveau");
#ifdef DEBUG
	__nouveau_debug = driParseDebugString(getenv("NOUVEAU_DEBUG"),
					      debug_control);
#endif

	/*XXX: Hack up a fake region and buffer object for front buffer.
	 *     This will go away with TTM, replaced with a simple reference
	 *     of the front buffer handle passed to us by the DDX.
	 */
	{
		struct pipe_surface *fb_surf;
		struct nouveau_bo_priv *fb_bo;

		fb_bo = calloc(1, sizeof(struct nouveau_bo_priv));
		fb_bo->drm.offset = nv_screen->front_offset;
		fb_bo->drm.flags = NOUVEAU_MEM_FB;
		fb_bo->drm.size = nv_screen->front_pitch * 
				  nv_screen->front_height;
		fb_bo->refcount = 1;
		fb_bo->base.flags = NOUVEAU_BO_PIN | NOUVEAU_BO_VRAM;
		fb_bo->base.offset = fb_bo->drm.offset;
		fb_bo->base.handle = (unsigned long)fb_bo;
		fb_bo->base.size = fb_bo->drm.size;
		fb_bo->base.device = nv_screen->device;

		fb_surf = calloc(1, sizeof(struct pipe_surface));
		fb_surf->cpp = nv_screen->front_cpp;
		fb_surf->pitch = nv_screen->front_pitch / fb_surf->cpp;
		fb_surf->height = nv_screen->front_height;
		fb_surf->refcount = 1;
		fb_surf->buffer = (void *)fb_bo;

		nv->frontbuffer = fb_surf;
	}

	if ((ret = nouveau_grobj_alloc(nv->channel, 0x00000000, 0x30,
				       &nv->NvNull))) {
		NOUVEAU_ERR("Error creating NULL object: %d\n", ret);
		return GL_FALSE;
	}
	nv->next_handle = 0x80000000;

	if ((ret = nouveau_notifier_alloc(nv->channel, nv->next_handle++, 1,
					  &nv->sync_notifier))) {
		NOUVEAU_ERR("Error creating channel sync notifier: %d\n", ret);
		return GL_FALSE;
	}

	if (nv->chipset < 0x50)
		ret = nouveau_surface_init_nv04(nv);
	else
		ret = nouveau_surface_init_nv50(nv);
	if (ret) {
		return GL_FALSE;
	}

	if (!getenv("NOUVEAU_FORCE_SOFTPIPE")) {
		pipe = nouveau_pipe_create(nv);
		if (!pipe)
			NOUVEAU_ERR("Couldn't create hw pipe\n");
	}

	if (!pipe) {
		NOUVEAU_MSG("Using softpipe\n");
		pipe = nouveau_create_softpipe(nv);
		if (!pipe) {
			NOUVEAU_ERR("Error creating pipe, bailing\n");
			return GL_FALSE;
		}
	}

	pipe->priv = nv;
	nv->st = st_create_context(pipe, glVis, st_share);
	return GL_TRUE;
}

void
nouveau_context_destroy(__DRIcontextPrivate *driContextPriv)
{
	struct nouveau_context *nv = driContextPriv->driverPrivate;

	assert(nv);

	st_flush(nv->st, PIPE_FLUSH_WAIT);
	st_destroy_context(nv->st);

	nouveau_grobj_free(&nv->NvCtxSurf2D);
	nouveau_grobj_free(&nv->NvImageBlit);
	nouveau_channel_free(&nv->channel);

	free(nv);
}

GLboolean
nouveau_context_bind(__DRIcontextPrivate *driContextPriv,
		     __DRIdrawablePrivate *driDrawPriv,
		     __DRIdrawablePrivate *driReadPriv)
{
	struct nouveau_context *nv;
	struct nouveau_framebuffer *draw, *read;

	if (!driContextPriv) {
		st_make_current(NULL, NULL, NULL);
		return GL_TRUE;
	}

	nv = driContextPriv->driverPrivate;
	draw = driDrawPriv->driverPrivate;
	read = driReadPriv->driverPrivate;

	st_make_current(nv->st, draw->stfb, read->stfb);

	if ((nv->dri_drawable != driDrawPriv) ||
	    (nv->last_stamp != driDrawPriv->lastStamp)) {
		nv->dri_drawable = driDrawPriv;
		st_resize_framebuffer(draw->stfb, driDrawPriv->w,
				      driDrawPriv->h);
		nv->last_stamp = driDrawPriv->lastStamp;
	}

	if (driDrawPriv != driReadPriv) {
		st_resize_framebuffer(read->stfb, driReadPriv->w,
				      driReadPriv->h);
	}

	return GL_TRUE;
}

GLboolean
nouveau_context_unbind(__DRIcontextPrivate *driContextPriv)
{
	struct nouveau_context *nv = driContextPriv->driverPrivate;
	(void)nv;

	st_flush(nv->st, 0);
	return GL_TRUE;
}

