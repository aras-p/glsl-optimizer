/*
 * Copyright © 2009 Corbin Simpson
 * Copyright © 2011 Marek Olšák <maraeo@gmail.com>
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
 *      Marek Olšák <maraeo@gmail.com>
 */

#include "radeon_drm_bo.h"
#include "radeon_drm_cs.h"
#include "radeon_drm_public.h"

#include "pipebuffer/pb_bufmgr.h"
#include "util/u_memory.h"

#include <xf86drm.h>
#include <stdio.h>

#ifndef RADEON_INFO_WANT_HYPERZ
#define RADEON_INFO_WANT_HYPERZ 7
#endif
#ifndef RADEON_INFO_WANT_CMASK
#define RADEON_INFO_WANT_CMASK 8
#endif

/* Enable/disable feature access for one command stream.
 * If enable == TRUE, return TRUE on success.
 * Otherwise, return FALSE.
 *
 * We basically do the same thing kernel does, because we have to deal
 * with multiple contexts (here command streams) backed by one winsys. */
static boolean radeon_set_fd_access(struct radeon_drm_cs *applier,
                                    struct radeon_drm_cs **owner,
                                    pipe_mutex *mutex,
                                    unsigned request, boolean enable)
{
    struct drm_radeon_info info = {0};
    unsigned value = enable ? 1 : 0;

    pipe_mutex_lock(*mutex);

    /* Early exit if we are sure the request will fail. */
    if (enable) {
        if (*owner) {
            pipe_mutex_unlock(*mutex);
            return FALSE;
        }
    } else {
        if (*owner != applier) {
            pipe_mutex_unlock(*mutex);
            return FALSE;
        }
    }

    /* Pass through the request to the kernel. */
    info.value = (unsigned long)&value;
    info.request = request;
    if (drmCommandWriteRead(applier->ws->fd, DRM_RADEON_INFO,
                            &info, sizeof(info)) != 0) {
        pipe_mutex_unlock(*mutex);
        return FALSE;
    }

    /* Update the rights in the winsys. */
    if (enable) {
        if (value) {
            *owner = applier;
            fprintf(stderr, "radeon: Acquired Hyper-Z.\n");
            pipe_mutex_unlock(*mutex);
            return TRUE;
        }
    } else {
        *owner = NULL;
        fprintf(stderr, "radeon: Released Hyper-Z.\n");
    }

    pipe_mutex_unlock(*mutex);
    return FALSE;
}

/* Helper function to do the ioctls needed for setup and init. */
static void do_ioctls(struct radeon_drm_winsys *winsys)
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

    version = drmGetVersion(winsys->fd);
    if (version->version_major != 2 ||
        version->version_minor < 3) {
        fprintf(stderr, "%s: DRM version is %d.%d.%d but this driver is "
                "only compatible with 2.3.x (kernel 2.6.34) and later.\n",
                __FUNCTION__,
                version->version_major,
                version->version_minor,
                version->version_patchlevel);
        drmFreeVersion(version);
        exit(1);
    }

    winsys->drm_major = version->version_major;
    winsys->drm_minor = version->version_minor;
    winsys->drm_patchlevel = version->version_patchlevel;

    info.request = RADEON_INFO_DEVICE_ID;
    retval = drmCommandWriteRead(winsys->fd, DRM_RADEON_INFO, &info, sizeof(info));
    if (retval) {
        fprintf(stderr, "%s: Failed to get PCI ID, "
                "error number %d\n", __FUNCTION__, retval);
        exit(1);
    }
    winsys->pci_id = target;

    info.request = RADEON_INFO_NUM_GB_PIPES;
    retval = drmCommandWriteRead(winsys->fd, DRM_RADEON_INFO, &info, sizeof(info));
    if (retval) {
        fprintf(stderr, "%s: Failed to get GB pipe count, "
                "error number %d\n", __FUNCTION__, retval);
        exit(1);
    }
    winsys->gb_pipes = target;

    info.request = RADEON_INFO_NUM_Z_PIPES;
    retval = drmCommandWriteRead(winsys->fd, DRM_RADEON_INFO, &info, sizeof(info));
    if (retval) {
        fprintf(stderr, "%s: Failed to get Z pipe count, "
                "error number %d\n", __FUNCTION__, retval);
        exit(1);
    }
    winsys->z_pipes = target;

    retval = drmCommandWriteRead(winsys->fd, DRM_RADEON_GEM_INFO,
            &gem_info, sizeof(gem_info));
    if (retval) {
        fprintf(stderr, "%s: Failed to get MM info, error number %d\n",
                __FUNCTION__, retval);
        exit(1);
    }
    winsys->gart_size = gem_info.gart_size;
    winsys->vram_size = gem_info.vram_size;

    drmFreeVersion(version);

    winsys->num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
}

static void radeon_winsys_destroy(struct radeon_winsys *rws)
{
    struct radeon_drm_winsys *ws = (struct radeon_drm_winsys*)rws;

    pipe_mutex_destroy(ws->hyperz_owner_mutex);
    pipe_mutex_destroy(ws->cmask_owner_mutex);

    ws->cman->destroy(ws->cman);
    ws->kman->destroy(ws->kman);
    FREE(rws);
}

static uint32_t radeon_get_value(struct radeon_winsys *rws,
                                 enum radeon_value_id id)
{
    struct radeon_drm_winsys *ws = (struct radeon_drm_winsys *)rws;

    switch(id) {
    case RADEON_VID_PCI_ID:
	return ws->pci_id;
    case RADEON_VID_R300_GB_PIPES:
	return ws->gb_pipes;
    case RADEON_VID_R300_Z_PIPES:
	return ws->z_pipes;
    case RADEON_VID_GART_SIZE:
        return ws->gart_size;
    case RADEON_VID_VRAM_SIZE:
        return ws->vram_size;
    case RADEON_VID_DRM_MAJOR:
        return ws->drm_major;
    case RADEON_VID_DRM_MINOR:
        return ws->drm_minor;
    case RADEON_VID_DRM_PATCHLEVEL:
        return ws->drm_patchlevel;
    case RADEON_VID_DRM_2_6_0:
        return ws->drm_major*100 + ws->drm_minor >= 206;
    case RADEON_VID_DRM_2_8_0:
        return ws->drm_major*100 + ws->drm_minor >= 208;
    }
    return 0;
}

static boolean radeon_cs_request_feature(struct radeon_winsys_cs *rcs,
                                         enum radeon_feature_id fid,
                                         boolean enable)
{
    struct radeon_drm_cs *cs = radeon_drm_cs(rcs);

    switch (fid) {
    case RADEON_FID_HYPERZ_RAM_ACCESS:
        if (debug_get_bool_option("RADEON_HYPERZ", FALSE)) {
            return radeon_set_fd_access(cs, &cs->ws->hyperz_owner,
                                        &cs->ws->hyperz_owner_mutex,
                                        RADEON_INFO_WANT_HYPERZ, enable);
        } else {
            return FALSE;
        }

    case RADEON_FID_CMASK_RAM_ACCESS:
        if (debug_get_bool_option("RADEON_CMASK", FALSE)) {
            return radeon_set_fd_access(cs, &cs->ws->cmask_owner,
                                        &cs->ws->cmask_owner_mutex,
                                        RADEON_INFO_WANT_CMASK, enable);
        } else {
            return FALSE;
        }
    }
    return FALSE;
}

struct radeon_winsys *radeon_drm_winsys_create(int fd)
{
    struct radeon_drm_winsys *ws = CALLOC_STRUCT(radeon_drm_winsys);
    if (!ws) {
        return NULL;
    }

    ws->fd = fd;
    do_ioctls(ws);

    switch (ws->pci_id) {
#define CHIPSET(pci_id, name, family) case pci_id:
#include "pci_ids/r300_pci_ids.h"
#undef CHIPSET
       break;
    default:
       goto fail;
    }

    /* Create managers. */
    ws->kman = radeon_bomgr_create(ws);
    if (!ws->kman)
	goto fail;
    ws->cman = pb_cache_manager_create(ws->kman, 1000000);
    if (!ws->cman)
	goto fail;

    /* Set functions. */
    ws->base.destroy = radeon_winsys_destroy;
    ws->base.get_value = radeon_get_value;
    ws->base.cs_request_feature = radeon_cs_request_feature;

    radeon_bomgr_init_functions(ws);
    radeon_drm_cs_init_functions(ws);

    pipe_mutex_init(ws->hyperz_owner_mutex);
    pipe_mutex_init(ws->cmask_owner_mutex);

    return &ws->base;

fail:
    if (ws->cman)
	ws->cman->destroy(ws->cman);
    if (ws->kman)
	ws->kman->destroy(ws->kman);
    FREE(ws);
    return NULL;
}
