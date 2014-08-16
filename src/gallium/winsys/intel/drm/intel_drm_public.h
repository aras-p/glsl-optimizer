#ifndef INTEL_DRM_PUBLIC_H
#define INTEL_DRM_PUBLIC_H

struct intel_winsys;

struct intel_winsys *intel_winsys_create_for_fd(int fd);

#endif
