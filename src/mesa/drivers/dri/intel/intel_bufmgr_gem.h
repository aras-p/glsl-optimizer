
#ifndef INTEL_BUFMGR_GEM_H
#define INTEL_BUFMGR_GEM_H

#include "dri_bufmgr.h"

extern dri_bo *intel_gem_bo_create_from_handle(dri_bufmgr *bufmgr,
					       const char *name,
					       unsigned int handle);

dri_bufmgr *intel_bufmgr_gem_init(int fd, int batch_size);

void
intel_gem_enable_bo_reuse(dri_bufmgr *bufmgr);

#endif /* INTEL_BUFMGR_GEM_H */
