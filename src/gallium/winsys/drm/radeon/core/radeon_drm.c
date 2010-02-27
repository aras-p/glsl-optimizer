/*
 * Copyright © 2009 Corbin Simpson
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS, AUTHORS
 * AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 */
/*
 * Authors:
 *      Corbin Simpson <MostAwesomeDude@gmail.com>
 *      Joakim Sindholt <opensource@zhasha.com>
 */

#include "radeon_drm.h"
#include "radeon_r300.h"
#include "radeon_buffer.h"

#include "r300_winsys.h"
#include "trace/tr_drm.h"

#include "util/u_memory.h"

#include "xf86drm.h"
#include <sys/ioctl.h>

/* Helper function to do the ioctls needed for setup and init. */
static void do_ioctls(int fd, struct radeon_winsys* winsys)
{
    struct drm_radeon_gem_info gem_info = {0};
    struct drm_radeon_info info = {0};
    int target = 0;
    int retval;
    drmVersionPtr version;

    info.value = (unsigned long)&target;

    /* We do things in a specific order here.
     *
     * DRM version first. We need to be sure we're running on a KMS chipset.
     * This is also for some features.
     *
     * Then, the PCI ID. This is essential and should return usable numbers
     * for all Radeons. If this fails, we probably got handed an FD for some
     * non-Radeon card.
     *
     * The GB and Z pipe requests should always succeed, but they might not
     * return sensical values for all chipsets, but that's alright because
     * the pipe drivers already know that.
     *
     * The GEM info is actually bogus on the kernel side, as well as our side
     * (see radeon_gem_info_ioctl in radeon_gem.c) but that's alright because
     * we don't actually use the info for anything yet. */

    version = drmGetVersion(fd);
    if (version->version_major != 2) {
        fprintf(stderr, "%s: DRM version is %d.%d.%d but this driver is "
                "only compatible with 2.x.x\n", __FUNCTION__,
                version->version_major, version->version_minor,
                version->version_patchlevel);
        drmFreeVersion(version);
        exit(1);
    }

    info.request = RADEON_INFO_DEVICE_ID;
    retval = drmCommandWriteRead(fd, DRM_RADEON_INFO, &info, sizeof(info));
    if (retval) {
        fprintf(stderr, "%s: Failed to get PCI ID, "
                "error number %d\n", __FUNCTION__, retval);
        exit(1);
    }
    winsys->pci_id = target;

    info.request = RADEON_INFO_NUM_GB_PIPES;
    retval = drmCommandWriteRead(fd, DRM_RADEON_INFO, &info, sizeof(info));
    if (retval) {
        fprintf(stderr, "%s: Failed to get GB pipe count, "
                "error number %d\n", __FUNCTION__, retval);
        exit(1);
    }
    winsys->gb_pipes = target;

    info.request = RADEON_INFO_NUM_Z_PIPES;
    retval = drmCommandWriteRead(fd, DRM_RADEON_INFO, &info, sizeof(info));
    if (retval) {
        fprintf(stderr, "%s: Failed to get Z pipe count, "
                "error number %d\n", __FUNCTION__, retval);
        exit(1);
    }
    winsys->z_pipes = target;

    retval = drmCommandWriteRead(fd, DRM_RADEON_GEM_INFO,
            &gem_info, sizeof(gem_info));
    if (retval) {
        fprintf(stderr, "%s: Failed to get MM info, error number %d\n",
                __FUNCTION__, retval);
        exit(1);
    }
    winsys->gart_size = gem_info.gart_size;
    winsys->vram_size = gem_info.vram_size;

    debug_printf("radeon: Successfully grabbed chipset info from kernel!\n"
                 "radeon: DRM version: %d.%d.%d ID: 0x%04x GB: %d Z: %d\n"
                 "radeon: GART size: %d MB VRAM size: %d MB\n",
                 version->version_major, version->version_minor,
                 version->version_patchlevel, winsys->pci_id,
                 winsys->gb_pipes, winsys->z_pipes,
                 winsys->gart_size / 1024 / 1024,
                 winsys->vram_size / 1024 / 1024);

    drmFreeVersion(version);
}

/* Create a pipe_screen. */
struct pipe_screen* radeon_create_screen(struct drm_api* api,
                                         int drmFB,
                                         struct drm_create_screen_arg *arg)
{
    struct radeon_winsys* rwinsys = radeon_pipe_winsys(drmFB);
    do_ioctls(drmFB, rwinsys);

    /* The state tracker can organize a softpipe fallback if no hw
     * driver is found.
     */
    if (is_r3xx(rwinsys->pci_id)) {
        radeon_setup_winsys(drmFB, rwinsys);
        return r300_create_screen(rwinsys);
    } else {
        FREE(rwinsys);
        return NULL;
    }
}


boolean radeon_buffer_from_texture(struct drm_api* api,
                                   struct pipe_screen* screen,
                                   struct pipe_texture* texture,
                                   struct pipe_buffer** buffer,
                                   unsigned* stride)
{
    /* XXX fix this */
    return r300_get_texture_buffer(screen, texture, buffer, stride);
}

/* Create a buffer from a handle. */
/* XXX what's up with name? */
struct pipe_buffer* radeon_buffer_from_handle(struct drm_api* api,
                                              struct pipe_screen* screen,
                                              const char* name,
                                              unsigned handle)
{
    struct radeon_bo_manager* bom =
        ((struct radeon_winsys*)screen->winsys)->priv->bom;
    struct radeon_pipe_buffer* radeon_buffer;
    struct radeon_bo* bo = NULL;

    bo = radeon_bo_open(bom, handle, 0, 0, 0, 0);
    if (bo == NULL) {
        return NULL;
    }

    radeon_buffer = CALLOC_STRUCT(radeon_pipe_buffer);
    if (radeon_buffer == NULL) {
        radeon_bo_unref(bo);
        return NULL;
    }

    pipe_reference_init(&radeon_buffer->base.reference, 1);
    radeon_buffer->base.screen = screen;
    radeon_buffer->base.usage = PIPE_BUFFER_USAGE_PIXEL;
    radeon_buffer->bo = bo;
    return &radeon_buffer->base;
}

static struct pipe_texture*
radeon_texture_from_shared_handle(struct drm_api *api,
                                  struct pipe_screen *screen,
                                  struct pipe_texture *templ,
                                  const char *name,
                                  unsigned stride,
                                  unsigned handle)
{
    struct pipe_buffer *buffer;
    struct pipe_texture *blanket;

    buffer = radeon_buffer_from_handle(api, screen, name, handle);
    if (!buffer) {
        return NULL;
    }

    blanket = screen->texture_blanket(screen, templ, &stride, buffer);

    pipe_buffer_reference(&buffer, NULL);

    return blanket;
}

static boolean radeon_shared_handle_from_texture(struct drm_api *api,
                                                 struct pipe_screen *screen,
                                                 struct pipe_texture *texture,
                                                 unsigned *stride,
                                                 unsigned *handle)
{
    int retval, fd;
    struct drm_gem_flink flink;
    struct radeon_pipe_buffer* radeon_buffer;
    struct pipe_buffer *buffer = NULL;

    if (!radeon_buffer_from_texture(api, screen, texture, &buffer, stride)) {
        return FALSE;
    }

    radeon_buffer = (struct radeon_pipe_buffer*)buffer;
    if (!radeon_buffer->flinked) {
        fd = ((struct radeon_winsys*)screen->winsys)->priv->fd;

        flink.handle = radeon_buffer->bo->handle;

        retval = ioctl(fd, DRM_IOCTL_GEM_FLINK, &flink);
        if (retval) {
            debug_printf("radeon: DRM_IOCTL_GEM_FLINK failed, error %d\n",
                    retval);
            return FALSE;
        }

        radeon_buffer->flink = flink.name;
        radeon_buffer->flinked = TRUE;
    }

    *handle = radeon_buffer->flink;
    return TRUE;
}

static boolean radeon_local_handle_from_texture(struct drm_api *api,
                                                struct pipe_screen *screen,
                                                struct pipe_texture *texture,
                                                unsigned *stride,
                                                unsigned *handle)
{
    struct pipe_buffer *buffer = NULL;
    if (!radeon_buffer_from_texture(api, screen, texture, &buffer, stride)) {
        return FALSE;
    }

    *handle = ((struct radeon_pipe_buffer*)buffer)->bo->handle;

    pipe_buffer_reference(&buffer, NULL);

    return TRUE;
}

static void radeon_drm_api_destroy(struct drm_api *api)
{
    return;
}

struct drm_api drm_api_hooks = {
    .name = "radeon",
    .driver_name = "radeon",
    .create_screen = radeon_create_screen,
    .texture_from_shared_handle = radeon_texture_from_shared_handle,
    .shared_handle_from_texture = radeon_shared_handle_from_texture,
    .local_handle_from_texture = radeon_local_handle_from_texture,
    .destroy = radeon_drm_api_destroy,
};

struct drm_api* drm_api_create()
{
#ifdef DEBUG
    return trace_drm_create(&drm_api_hooks);
#else
    return &drm_api_hooks;
#endif
}
