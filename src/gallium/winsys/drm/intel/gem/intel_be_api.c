
#include "intel_be_api.h"
#include "i915simple/i915_winsys.h"
#include "identity/id_drm.h"
#include "trace/tr_drm.h"

static void destroy(struct drm_api *api)
{

}

struct drm_api intel_be_drm_api =
{
	/* intel_be_context.c */
	.create_context = intel_be_create_context,
	/* intel_be_device.c */
	.create_screen = intel_be_create_screen,
	.texture_from_shared_handle = intel_be_texture_from_shared_handle,
	.shared_handle_from_texture = intel_be_shared_handle_from_texture,
	.local_handle_from_texture = intel_be_local_handle_from_texture,
	.destroy = destroy,
};

struct drm_api *
drm_api_create()
{
#ifdef DEBUG
#if 0
	return identity_drm_create(&intel_be_drm_api);
#else
	return trace_drm_create(&intel_be_drm_api);
#endif
#else
	return &intel_be_drm_api;
#endif
}
