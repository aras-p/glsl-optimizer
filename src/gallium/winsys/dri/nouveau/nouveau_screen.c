#include "utils.h"
#include "vblank.h"
#include "xmlpool.h"

#include "pipe/p_context.h"
#include "state_tracker/st_public.h"
#include "state_tracker/st_cb_fbo.h"

#include "nouveau_context.h"
#include "nouveau_drm.h"
#include "nouveau_dri.h"
#include "nouveau_local.h"
#include "nouveau_screen.h"
#include "nouveau_swapbuffers.h"

#if NOUVEAU_DRM_HEADER_PATCHLEVEL != 10
#error nouveau_drm.h version does not match expected version
#endif

/* Extension stuff, enabling of extensions handled by Gallium's GL state
 * tracker.  But, we still need to define the entry points we want.
 */
#define need_GL_ARB_fragment_program
#define need_GL_ARB_multisample
#define need_GL_ARB_occlusion_query
#define need_GL_ARB_point_parameters
#define need_GL_ARB_shader_objects
#define need_GL_ARB_texture_compression
#define need_GL_ARB_vertex_program
#define need_GL_ARB_vertex_shader
#define need_GL_ARB_vertex_buffer_object
#define need_GL_EXT_compiled_vertex_array
#define need_GL_EXT_fog_coord
#define need_GL_EXT_secondary_color
#define need_GL_EXT_framebuffer_object
#define need_GL_VERSION_2_0
#define need_GL_VERSION_2_1
#include "extension_helper.h"

const struct dri_extension card_extensions[] =
{
	{ "GL_ARB_multisample", GL_ARB_multisample_functions },
	{ "GL_ARB_occlusion_query", GL_ARB_occlusion_query_functions },
	{ "GL_ARB_point_parameters", GL_ARB_point_parameters_functions },
	{ "GL_ARB_shader_objects", GL_ARB_shader_objects_functions },
	{ "GL_ARB_shading_language_100", GL_VERSION_2_0_functions },
	{ "GL_ARB_shading_language_120", GL_VERSION_2_1_functions },
	{ "GL_ARB_texture_compression", GL_ARB_texture_compression_functions },
	{ "GL_ARB_vertex_program", GL_ARB_vertex_program_functions },
	{ "GL_ARB_vertex_shader", GL_ARB_vertex_shader_functions },
	{ "GL_ARB_vertex_buffer_object", GL_ARB_vertex_buffer_object_functions },
	{ "GL_EXT_compiled_vertex_array", GL_EXT_compiled_vertex_array_functions },
	{ "GL_EXT_fog_coord", GL_EXT_fog_coord_functions },
	{ "GL_EXT_framebuffer_object", GL_EXT_framebuffer_object_functions },
	{ "GL_EXT_secondary_color", GL_EXT_secondary_color_functions },
	{ NULL, 0 }
};

PUBLIC const char __driConfigOptions[] =
DRI_CONF_BEGIN
DRI_CONF_END;
static const GLuint __driNConfigOptions = 0;

extern const struct dri_extension common_extensions[];
extern const struct dri_extension nv40_extensions[];

static GLboolean
nouveau_screen_create(__DRIscreenPrivate *driScrnPriv)
{
	struct nouveau_dri *nv_dri = driScrnPriv->pDevPriv;
	struct nouveau_screen *nv_screen;
	int ret;

	if (driScrnPriv->devPrivSize != sizeof(struct nouveau_dri)) {
		NOUVEAU_ERR("DRI struct mismatch between DDX/DRI\n");
		return GL_FALSE;
	}

	nv_screen = CALLOC_STRUCT(nouveau_screen);
	if (!nv_screen)
		return GL_FALSE;
	nv_screen->driScrnPriv = driScrnPriv;
	driScrnPriv->private = (void *)nv_screen;

	driParseOptionInfo(&nv_screen->option_cache,
			   __driConfigOptions, __driNConfigOptions);

	if ((ret = nouveau_device_open_existing(&nv_screen->device, 0,
						driScrnPriv->fd, 0))) {
		NOUVEAU_ERR("Failed opening nouveau device: %d\n", ret);
		return GL_FALSE;
	}

	nv_screen->front_offset = nv_dri->front_offset;
	nv_screen->front_pitch  = nv_dri->front_pitch * (nv_dri->bpp / 8);
	nv_screen->front_cpp = nv_dri->bpp / 8;
	nv_screen->front_height = nv_dri->height;

	return GL_TRUE;
}

static void
nouveau_screen_destroy(__DRIscreenPrivate *driScrnPriv)
{
	struct nouveau_screen *nv_screen = driScrnPriv->private;

	driScrnPriv->private = NULL;
	FREE(nv_screen);
}

static GLboolean
nouveau_create_buffer(__DRIscreenPrivate * driScrnPriv,
		      __DRIdrawablePrivate * driDrawPriv,
		      const __GLcontextModes *glVis, GLboolean pixmapBuffer)
{
	struct nouveau_framebuffer *nvfb;
	enum pipe_format colour, depth, stencil;

	if (pixmapBuffer)
		return GL_FALSE;

	nvfb = CALLOC_STRUCT(nouveau_framebuffer);
	if (!nvfb)
		return GL_FALSE;

	if (glVis->redBits == 5)
		colour = PIPE_FORMAT_R5G6B5_UNORM;
	else
		colour = PIPE_FORMAT_A8R8G8B8_UNORM;

	if (glVis->depthBits == 16)
		depth = PIPE_FORMAT_Z16_UNORM;
	else if (glVis->depthBits == 24)
		depth = PIPE_FORMAT_Z24S8_UNORM;
	else
		depth = PIPE_FORMAT_NONE;

	if (glVis->stencilBits == 8)
		stencil = PIPE_FORMAT_Z24S8_UNORM;
	else
		stencil = PIPE_FORMAT_NONE;

	nvfb->stfb = st_create_framebuffer(glVis, colour, depth, stencil,
					   driDrawPriv->w, driDrawPriv->h,
					   (void*)nvfb);
	if (!nvfb->stfb) {
		free(nvfb);
		return  GL_FALSE;
	}

	driDrawPriv->driverPrivate = (void *)nvfb;
	return GL_TRUE;
}

static void
nouveau_destroy_buffer(__DRIdrawablePrivate * driDrawPriv)
{
	struct nouveau_framebuffer *nvfb;
	
	nvfb = (struct nouveau_framebuffer *)driDrawPriv->driverPrivate;
	st_unreference_framebuffer(&nvfb->stfb);
	free(nvfb);
}

static struct __DriverAPIRec
nouveau_api = {
	.InitDriver	= nouveau_screen_create,
	.DestroyScreen	= nouveau_screen_destroy,
	.CreateContext	= nouveau_context_create,
	.DestroyContext	= nouveau_context_destroy,
	.CreateBuffer	= nouveau_create_buffer,
	.DestroyBuffer	= nouveau_destroy_buffer,
	.SwapBuffers	= nouveau_swap_buffers,
	.MakeCurrent	= nouveau_context_bind,
	.UnbindContext	= nouveau_context_unbind,
	.GetSwapInfo	= NULL,
	.GetMSC		= NULL,
	.WaitForMSC	= NULL,
	.WaitForSBC	= NULL,
	.SwapBuffersMSC	= NULL,
	.CopySubBuffer	= nouveau_copy_sub_buffer,
	.setTexOffset	= NULL
};

static __GLcontextModes *
nouveau_fill_in_modes(unsigned pixel_bits, unsigned depth_bits,
		      unsigned stencil_bits, GLboolean have_back_buffer)
{
	__GLcontextModes * modes;
	__GLcontextModes * m;
	unsigned num_modes;
	unsigned depth_buffer_factor;
	unsigned back_buffer_factor;
	int i;

	static const struct {
		GLenum format;
		GLenum type;
	} fb_format_array[] = {
		{ GL_RGB , GL_UNSIGNED_SHORT_5_6_5     },
		{ GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV },
		{ GL_BGR , GL_UNSIGNED_INT_8_8_8_8_REV },
	};

	/* GLX_SWAP_COPY_OML is only supported because the Intel driver doesn't
	 * support pageflipping at all.
	 */
	static const GLenum back_buffer_modes[] = {
		GLX_NONE, GLX_SWAP_UNDEFINED_OML, GLX_SWAP_COPY_OML
	};

	uint8_t depth_bits_array[4]   = { 0, 16, 24, 24 };
	uint8_t stencil_bits_array[4] = { 0,  0,  0, 8 };
	uint8_t msaa_samples_array[1] = { 0 };

	depth_buffer_factor = 4;
	back_buffer_factor  = (have_back_buffer) ? 3 : 1;

	num_modes = ((pixel_bits==16) ? 1 : 2) *
		depth_buffer_factor * back_buffer_factor * 4;
	modes = (*dri_interface->createContextModes)(num_modes,
						     sizeof(__GLcontextModes));
	m = modes;

	for (i=((pixel_bits==16)?0:1);i<((pixel_bits==16)?1:3);i++) {
		if (!driFillInModes(&m, fb_format_array[i].format,
					fb_format_array[i].type,
					depth_bits_array,
					stencil_bits_array,
					depth_buffer_factor,
					back_buffer_modes,
					back_buffer_factor,
					msaa_samples_array, 1,
					GLX_TRUE_COLOR)) {
		fprintf( stderr, "[%s:%u] Error creating FBConfig!\n",
				__func__, __LINE__ );
		return NULL;
		}

		if (!driFillInModes(&m, fb_format_array[i].format,
					fb_format_array[i].type,
					depth_bits_array,
					stencil_bits_array,
					depth_buffer_factor,
					back_buffer_modes,
					back_buffer_factor,
					msaa_samples_array, 1,
					GLX_DIRECT_COLOR)) {
		fprintf( stderr, "[%s:%u] Error creating FBConfig!\n",
				__func__, __LINE__ );
		return NULL;
		}
	}

	return modes;
}
PUBLIC void *
__driCreateNewScreen_20050727(__DRInativeDisplay *dpy, int scrn,
			      __DRIscreen *psc, const __GLcontextModes * modes,
			      const __DRIversion * ddx_version,
			      const __DRIversion * dri_version,
			      const __DRIversion * drm_version,
			      const __DRIframebuffer * frame_buffer,
			      void * pSAREA, int fd, int internal_api_version,
			      const __DRIinterfaceMethods * interface,
			      __GLcontextModes ** driver_modes)
{
	__DRIscreenPrivate *psp;
	static const __DRIversion ddx_expected =
		{ 0, 0, NOUVEAU_DRM_HEADER_PATCHLEVEL };
	static const __DRIversion dri_expected = { 4, 0, 0 };
	static const __DRIversion drm_expected =
		{ 0, 0, NOUVEAU_DRM_HEADER_PATCHLEVEL };
	struct nouveau_dri *nv_dri = NULL;

	dri_interface = interface;

	if (!driCheckDriDdxDrmVersions2("nouveau",
					dri_version, &dri_expected,
					ddx_version, &ddx_expected,
					drm_version, &drm_expected)) {
		return NULL;
	}

	if (drm_expected.patch != drm_version->patch) {
		fprintf(stderr, "Incompatible DRM patch level.\n"
				"Expected: %d\n" "Current : %d\n",
			drm_expected.patch, drm_version->patch);
		return NULL;
	}

	psp = __driUtilCreateNewScreen(dpy, scrn, psc, NULL,
				       ddx_version, dri_version, drm_version,
				       frame_buffer, pSAREA, fd,
				       internal_api_version,
				       &nouveau_api);
	if (psp == NULL)
		return NULL;
	nv_dri = psp->pDevPriv;

	*driver_modes = nouveau_fill_in_modes(nv_dri->bpp,
					      (nv_dri->bpp == 16) ? 16 : 24,
					      (nv_dri->bpp == 16) ? 0 : 8,
					      1);

	driInitExtensions(NULL, card_extensions, GL_FALSE);

	return (void *)psp;
}

