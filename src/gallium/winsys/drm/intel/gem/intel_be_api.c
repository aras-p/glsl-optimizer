
#include "intel_be_api.h"
#include "i915simple/i915_winsys.h"

static void destroy(struct drm_api *api)
{

}

struct drm_api intel_be_drm_api =
{
	/* intel_be_context.c */
	.create_context = intel_be_create_context,
	/* intel_be_device.c */
	.create_screen = intel_be_create_screen,
	.buffer_from_texture = i915_get_texture_buffer,
	.buffer_from_handle = intel_be_buffer_from_handle,
	.handle_from_buffer = intel_be_handle_from_buffer,
	.global_handle_from_buffer = intel_be_global_handle_from_buffer,
	.destroy = destroy,
};

struct drm_api *
drm_api_create()
{
	return &intel_be_drm_api;
}
