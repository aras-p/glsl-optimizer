
#include "intel_be_api.h"

struct drm_api drm_api_hocks =
{
	/* intel_be_context.c */
	.create_context = intel_be_create_context,
	/* intel_be_screen.c */
	.create_screen = intel_be_create_screen,
	.buffer_from_handle = intel_be_buffer_from_handle,
	.handle_from_buffer = intel_be_handle_from_buffer,
};
