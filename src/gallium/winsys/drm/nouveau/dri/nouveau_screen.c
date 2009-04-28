#include <utils.h>
#include <vblank.h>
#include <xmlpool.h>

#include <pipe/p_context.h>
#include <state_tracker/st_public.h>
#include <state_tracker/st_cb_fbo.h>
#include <state_tracker/drm_api.h>

#include "nouveau_context.h"
#include "nouveau_screen.h"
#include "nouveau_swapbuffers.h"
#include "nouveau_dri.h"

#include "nouveau_drm.h"
#include "nouveau_drmif.h"

#if NOUVEAU_DRM_HEADER_PATCHLEVEL != 12
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
	st_unreference_framebuffer(nvfb->stfb);
	free(nvfb);
}

static __DRIconfig **
nouveau_fill_in_modes(__DRIscreenPrivate *psp,
		      unsigned pixel_bits, unsigned depth_bits,
		      unsigned stencil_bits, GLboolean have_back_buffer)
{
	__DRIconfig **configs;
	unsigned depth_buffer_factor;
	unsigned back_buffer_factor;
	GLenum fb_format;
	GLenum fb_type;

	static const GLenum back_buffer_modes[] = {
		GLX_NONE, GLX_SWAP_UNDEFINED_OML,
	};

	uint8_t depth_bits_array[3];
	uint8_t stencil_bits_array[3];
	uint8_t msaa_samples_array[1];

	depth_bits_array[0] = 0;
	depth_bits_array[1] = depth_bits;
	depth_bits_array[2] = depth_bits;

	/* Just like with the accumulation buffer, always provide some modes
	 * with a stencil buffer.  It will be a sw fallback, but some apps won't
	 * care about that.
	 */
	stencil_bits_array[0] = 0;
	stencil_bits_array[1] = 0;
	if (depth_bits == 24)
		stencil_bits_array[1] = (stencil_bits == 0) ? 8 : stencil_bits;
	stencil_bits_array[2] = (stencil_bits == 0) ? 8 : stencil_bits;

	msaa_samples_array[0] = 0;

	depth_buffer_factor =
		((depth_bits != 0) || (stencil_bits != 0)) ? 3 : 1;
	back_buffer_factor = (have_back_buffer) ? 3 : 1;

	if (pixel_bits == 16) {
		fb_format = GL_RGB;
		fb_type = GL_UNSIGNED_SHORT_5_6_5;
	}
	else {
		fb_format = GL_BGRA;
		fb_type = GL_UNSIGNED_INT_8_8_8_8_REV;
	}

	configs = driCreateConfigs(fb_format, fb_type,
				   depth_bits_array, stencil_bits_array,
				   depth_buffer_factor, back_buffer_modes,
				   back_buffer_factor, msaa_samples_array, 1);
	if (configs == NULL) {
	 fprintf(stderr, "[%s:%u] Error creating FBConfig!\n",
			 __func__, __LINE__);
		return NULL;
	}

	return configs;
}

static struct pipe_surface *
dri_surface_from_handle(struct pipe_screen *screen,
                        unsigned handle,
                        enum pipe_format format,
                        unsigned width,
                        unsigned height,
                        unsigned pitch)
{
   struct pipe_surface *surface = NULL;
   struct pipe_texture *texture = NULL;
   struct pipe_texture templat;
   struct pipe_buffer *buf = NULL;

   buf = drm_api_hooks.buffer_from_handle(screen, "front buffer", handle);
   if (!buf)
      return NULL;

   memset(&templat, 0, sizeof(templat));
   templat.tex_usage = PIPE_TEXTURE_USAGE_PRIMARY |
                       NOUVEAU_TEXTURE_USAGE_LINEAR;
   templat.target = PIPE_TEXTURE_2D;
   templat.last_level = 0;
   templat.depth[0] = 1;
   templat.format = format;
   templat.width[0] = width;
   templat.height[0] = height;
   pf_get_block(templat.format, &templat.block);

   texture = screen->texture_blanket(screen,
                                     &templat,
                                     &pitch,
                                     buf);

   /* we don't need the buffer from this point on */
   pipe_buffer_reference(&buf, NULL);

   if (!texture)
      return NULL;

   surface = screen->get_tex_surface(screen, texture, 0, 0, 0,
                                     PIPE_BUFFER_USAGE_GPU_READ |
                                     PIPE_BUFFER_USAGE_GPU_WRITE);

   /* we don't need the texture from this point on */
   pipe_texture_reference(&texture, NULL);
   return surface;
}

static const __DRIconfig **
nouveau_screen_create(__DRIscreenPrivate *psp)
{
	struct nouveau_dri *nv_dri = psp->pDevPriv;
	struct nouveau_screen *nv_screen;
	static const __DRIversion ddx_expected =
		{ 0, 0, NOUVEAU_DRM_HEADER_PATCHLEVEL };
	static const __DRIversion dri_expected = { 4, 0, 0 };
	static const __DRIversion drm_expected =
		{ 0, 0, NOUVEAU_DRM_HEADER_PATCHLEVEL };

	if (!driCheckDriDdxDrmVersions2("nouveau",
					&psp->dri_version, &dri_expected,
					&psp->ddx_version, &ddx_expected,
					&psp->drm_version, &drm_expected)) {
		return NULL;
	}

	if (drm_expected.patch != psp->drm_version.patch) {
		fprintf(stderr, "Incompatible DRM patch level.\n"
				"Expected: %d\n" "Current : %d\n",
			drm_expected.patch, psp->drm_version.patch);
		return NULL;
	}

	driInitExtensions(NULL, card_extensions, GL_FALSE);

	if (psp->devPrivSize != sizeof(struct nouveau_dri)) {
		NOUVEAU_ERR("DRI struct mismatch between DDX/DRI\n");
		return NULL;
	}

	nv_screen = CALLOC_STRUCT(nouveau_screen);
	if (!nv_screen)
		return NULL;

	nouveau_device_open_existing(&nv_screen->device, 0, psp->fd, 0);

	nv_screen->pscreen = drm_api_hooks.create_screen(psp->fd, NULL);
	if (!nv_screen->pscreen) {
		FREE(nv_screen);
		return NULL;
	}
	nv_screen->pscreen->flush_frontbuffer = nouveau_flush_frontbuffer;

	{
		enum pipe_format format;

		if (nv_dri->bpp == 16)
			format = PIPE_FORMAT_R5G6B5_UNORM;
		else
			format = PIPE_FORMAT_A8R8G8B8_UNORM;

		nv_screen->fb = dri_surface_from_handle(nv_screen->pscreen,
							nv_dri->front_offset,
							format,
							nv_dri->width,
							nv_dri->height,
							nv_dri->front_pitch *
							nv_dri->bpp / 8);
	}
						
	driParseOptionInfo(&nv_screen->option_cache,
			   __driConfigOptions, __driNConfigOptions);

	nv_screen->driScrnPriv = psp;
	psp->private = (void *)nv_screen;

	return (const __DRIconfig **)
		nouveau_fill_in_modes(psp, nv_dri->bpp,
				      (nv_dri->bpp == 16) ? 16 : 24,
				      (nv_dri->bpp == 16) ? 0 : 8, 1);
}

static void
nouveau_screen_destroy(__DRIscreenPrivate *driScrnPriv)
{
	struct nouveau_screen *nv_screen = driScrnPriv->private;

	driScrnPriv->private = NULL;
	FREE(nv_screen);
}

const struct __DriverAPIRec
driDriverAPI = {
	.InitScreen	= nouveau_screen_create,
	.DestroyScreen	= nouveau_screen_destroy,
	.CreateContext	= nouveau_context_create,
	.DestroyContext	= nouveau_context_destroy,
	.CreateBuffer	= nouveau_create_buffer,
	.DestroyBuffer	= nouveau_destroy_buffer,
	.SwapBuffers	= nouveau_swap_buffers,
	.MakeCurrent	= nouveau_context_bind,
	.UnbindContext	= nouveau_context_unbind,
	.CopySubBuffer	= nouveau_copy_sub_buffer,

	.InitScreen2	= NULL, /* one day, I promise! */
};

