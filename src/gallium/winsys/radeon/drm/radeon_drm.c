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
#include "radeon_drm_public.h"

#include "r300_winsys.h"

#include "util/u_memory.h"

#include "xf86drm.h"

static struct radeon_libdrm_winsys *
radeon_winsys_create(int fd)
{
    struct radeon_libdrm_winsys *rws;

    rws = CALLOC_STRUCT(radeon_libdrm_winsys);
    if (rws == NULL) {
        return NULL;
    }

    rws->fd = fd;
    return rws;
}

/* Enable/disable Hyper-Z access. Return TRUE on success. */
static boolean radeon_set_hyperz_access(int fd, boolean enable)
{
#ifndef RADEON_INFO_WANT_HYPERZ
#define RADEON_INFO_WANT_HYPERZ 7
#endif

    struct drm_radeon_info info = {0};
    unsigned value = enable ? 1 : 0;

    if (!debug_get_bool_option("RADEON_HYPERZ", FALSE))
        return FALSE;

    info.value = (unsigned long)&value;
    info.request = RADEON_INFO_WANT_HYPERZ;

    if (drmCommandWriteRead(fd, DRM_RADEON_INFO, &info, sizeof(info)) != 0)
        return FALSE;

    if (enable && !value)
        return FALSE;

    return TRUE;
}

/* Helper function to do the ioctls needed for setup and init. */
static void do_ioctls(int fd, struct radeon_libdrm_winsys* winsys)
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

/* XXX Remove this ifdef when libdrm version 2.4.19 becomes mandatory. */
#ifdef RADEON_BO_FLAGS_MICRO_TILE_SQUARE
    // Supported since 2.1.0.
    winsys->squaretiling = version->version_major > 2 ||
                           version->version_minor >= 1;
#endif

    winsys->drm_2_3_0 = version->version_major > 2 ||
                        version->version_minor >= 3;

    winsys->drm_2_6_0 = version->version_major > 2 ||
                        (version->version_major == 2 &&
                         version->version_minor >= 6);

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

    winsys->hyperz = radeon_set_hyperz_access(fd, TRUE);

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
                 "radeon: GART size: %d MB VRAM size: %d MB\n"
                 "radeon: HyperZ: %s\n",
                 version->version_major, version->version_minor,
                 version->version_patchlevel, winsys->pci_id,
                 winsys->gb_pipes, winsys->z_pipes,
                 winsys->gart_size / 1024 / 1024,
                 winsys->vram_size / 1024 / 1024,
                 winsys->hyperz ? "YES" : "NO");

    drmFreeVersion(version);
}

/* Create a pipe_screen. */
struct r300_winsys_screen* r300_drm_winsys_screen_create(int drmFB)
{
    struct radeon_libdrm_winsys* rws; 
    boolean ret;

    rws = radeon_winsys_create(drmFB);
    if (!rws)
	return NULL;

    do_ioctls(drmFB, rws);

    /* The state tracker can organize a softpipe fallback if no hw
     * driver is found.
     */
    if (is_r3xx(rws->pci_id)) {
        ret = radeon_setup_winsys(drmFB, rws);
	if (ret == FALSE)
	    goto fail;
        return &rws->base;
    }

fail:
    FREE(rws);
    return NULL;
}
