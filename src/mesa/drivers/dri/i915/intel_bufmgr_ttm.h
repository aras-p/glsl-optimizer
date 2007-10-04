
#ifndef INTEL_BUFMGR_TTM_H
#define INTEL_BUFMGR_TTM_H

#include "dri_bufmgr.h"

extern dri_bo *intel_ttm_bo_create_from_handle(dri_bufmgr *bufmgr, const char *name,
					       unsigned int handle);

dri_fence *intel_ttm_fence_create_from_arg(dri_bufmgr *bufmgr, const char *name,
					   drm_fence_arg_t *arg);


dri_bufmgr *intel_bufmgr_ttm_init(int fd, unsigned int fence_type,
				  unsigned int fence_type_flush);

#endif
