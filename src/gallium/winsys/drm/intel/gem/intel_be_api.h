
#ifndef _INTEL_BE_API_H_
#define _INTEL_BE_API_H_

#include "pipe/p_compiler.h"

#include "state_tracker/drm_api.h"

#include "intel_be_device.h"

struct pipe_screen *intel_be_create_screen(int drmFD,
					   struct drm_create_screen_arg *arg);
struct pipe_context *intel_be_create_context(struct pipe_screen *screen);

#endif
