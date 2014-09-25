/*
 * Copyright © 2014 Broadcom
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <errno.h>
#include <err.h>
#include <sys/mman.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include "util/u_memory.h"

#include "vc4_context.h"
#include "vc4_screen.h"

struct vc4_bo *
vc4_bo_alloc(struct vc4_screen *screen, uint32_t size, const char *name)
{
        struct vc4_bo *bo = CALLOC_STRUCT(vc4_bo);
        if (!bo)
                return NULL;

        pipe_reference_init(&bo->reference, 1);
        bo->screen = screen;
        bo->size = size;
        bo->name = name;

        struct drm_mode_create_dumb create;
        memset(&create, 0, sizeof(create));

        create.width = 128;
        create.bpp = 8;
        create.height = (size + 127) / 128;

        int ret = drmIoctl(screen->fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);
        if (ret != 0) {
                fprintf(stderr, "create ioctl failure\n");
                abort();
        }

        bo->handle = create.handle;
        assert(create.size >= size);

        return bo;
}

void
vc4_bo_free(struct vc4_bo *bo)
{
        struct vc4_screen *screen = bo->screen;

        if (bo->map) {
#ifdef USE_VC4_SIMULATOR
                if (bo->simulator_winsys_map) {
                        free(bo->map);
                        bo->map = bo->simulator_winsys_map;
                }
#endif
                munmap(bo->map, bo->size);
        }

        struct drm_gem_close c;
        memset(&c, 0, sizeof(c));
        c.handle = bo->handle;
        int ret = drmIoctl(screen->fd, DRM_IOCTL_GEM_CLOSE, &c);
        if (ret != 0)
                fprintf(stderr, "close object %d: %s\n", bo->handle, strerror(errno));

        free(bo);
}

struct vc4_bo *
vc4_bo_open_name(struct vc4_screen *screen, uint32_t name,
                 uint32_t winsys_stride)
{
        struct vc4_bo *bo = CALLOC_STRUCT(vc4_bo);

        struct drm_gem_open o;
        o.name = name;
        int ret = drmIoctl(screen->fd, DRM_IOCTL_GEM_OPEN, &o);
        if (ret) {
                fprintf(stderr, "Failed to open bo %d: %s\n",
                        name, strerror(errno));
                free(bo);
                return NULL;
        }

        pipe_reference_init(&bo->reference, 1);
        bo->screen = screen;
        bo->handle = o.handle;
        bo->size = o.size;
        bo->name = "winsys";

#ifdef USE_VC4_SIMULATOR
        vc4_bo_map(bo);
        bo->simulator_winsys_map = bo->map;
        bo->simulator_winsys_stride = winsys_stride;
        bo->map = malloc(bo->size);
#endif

        return bo;
}

struct vc4_bo *
vc4_bo_alloc_mem(struct vc4_screen *screen, const void *data, uint32_t size,
                 const char *name)
{
        void *map;
        struct vc4_bo *bo;

        bo = vc4_bo_alloc(screen, size, name);
        map = vc4_bo_map(bo);
        memcpy(map, data, size);
        return bo;
}

bool
vc4_bo_flink(struct vc4_bo *bo, uint32_t *name)
{
        struct drm_gem_flink flink = {
                .handle = bo->handle,
        };
        int ret = drmIoctl(bo->screen->fd, DRM_IOCTL_GEM_FLINK, &flink);
        if (ret) {
                fprintf(stderr, "Failed to flink bo %d: %s\n",
                        bo->handle, strerror(errno));
                free(bo);
                return false;
        }

        *name = flink.name;

        return true;
}

void *
vc4_bo_map(struct vc4_bo *bo)
{
        int ret;

        if (bo->map)
                return bo->map;

        struct drm_mode_map_dumb map;
        memset(&map, 0, sizeof(map));
        map.handle = bo->handle;
        ret = drmIoctl(bo->screen->fd, DRM_IOCTL_MODE_MAP_DUMB, &map);
        if (ret != 0) {
                fprintf(stderr, "map ioctl failure\n");
                abort();
        }

        bo->map = mmap(NULL, bo->size, PROT_READ | PROT_WRITE, MAP_SHARED,
                       bo->screen->fd, map.offset);
        if (bo->map == MAP_FAILED) {
                fprintf(stderr, "mmap of bo %d (offset 0x%016llx, size %d) failed\n",
                        bo->handle, (long long)map.offset, bo->size);
                abort();
        }

        return bo->map;
}
