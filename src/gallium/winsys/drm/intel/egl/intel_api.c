
#include "intel_api.h"

struct drm_api drm_api_hocks =
{
	.create_screen = intel_create_screen,
	.create_context = intel_create_context,
	.buffer_from_handle = intel_be_buffer_from_handle,
	.handle_from_buffer = intel_be_handle_from_buffer,
};
