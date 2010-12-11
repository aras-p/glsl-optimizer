/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *      Jerome Glisse
 *      Corbin Simpson <MostAwesomeDude@gmail.com>
 *      Joakim Sindholt <opensource@zhasha.com>
 */
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "util/u_inlines.h"
#include "util/u_debug.h"
#include "r600.h"
#include "r600_priv.h"
#include "r600_drm_public.h"
#include "xf86drm.h"
#include "radeon_drm.h"

#ifndef RADEON_INFO_TILING_CONFIG
#define RADEON_INFO_TILING_CONFIG 0x6
#endif

static struct radeon *radeon_new(int fd, unsigned device);

static int radeon_get_device(struct radeon *radeon)
{
	struct drm_radeon_info info;
	int r;

	radeon->device = 0;
	info.request = RADEON_INFO_DEVICE_ID;
	info.value = (uintptr_t)&radeon->device;
	r = drmCommandWriteRead(radeon->fd, DRM_RADEON_INFO, &info,
			sizeof(struct drm_radeon_info));
	return r;
}

static int radeon_drm_get_tiling(struct radeon *radeon)
{
	struct drm_radeon_info info;
	int r;
	uint32_t tiling_config;

	info.request = RADEON_INFO_TILING_CONFIG;
	info.value = (uintptr_t)&tiling_config;
	r = drmCommandWriteRead(radeon->fd, DRM_RADEON_INFO, &info,
				sizeof(struct drm_radeon_info));

	if (r)
		return 0;

	switch ((tiling_config & 0xe) >> 1) {
	case 0:
		radeon->tiling_info.num_channels = 1;
		break;
	case 1:
		radeon->tiling_info.num_channels = 2;
		break;
	case 2:
		radeon->tiling_info.num_channels = 4;
		break;
	case 3:
		radeon->tiling_info.num_channels = 8;
		break;
	default:
		return -EINVAL;
	}

	switch ((tiling_config & 0x30) >> 4) {
	case 0:
		radeon->tiling_info.num_banks = 4;
		break;
	case 1:
		radeon->tiling_info.num_banks = 8;
		break;
	default:
		return -EINVAL;

	}
	switch ((tiling_config & 0xc0) >> 6) {
	case 0:
		radeon->tiling_info.group_bytes = 256;
		break;
	case 1:
		radeon->tiling_info.group_bytes = 512;
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static struct radeon *radeon_new(int fd, unsigned device)
{
	struct radeon *radeon;
	int r;

	radeon = calloc(1, sizeof(*radeon));
	if (radeon == NULL) {
		return NULL;
	}
	radeon->fd = fd;
	radeon->device = device;
	radeon->refcount = 1;
	if (fd >= 0) {
		r = radeon_get_device(radeon);
		if (r) {
			fprintf(stderr, "Failed to get device id\n");
			return radeon_decref(radeon);
		}
	}
	radeon->family = radeon_family_from_device(radeon->device);
	if (radeon->family == CHIP_UNKNOWN) {
		fprintf(stderr, "Unknown chipset 0x%04X\n", radeon->device);
		return radeon_decref(radeon);
	}
	switch (radeon->family) {
	case CHIP_R600:
	case CHIP_RV610:
	case CHIP_RV630:
	case CHIP_RV670:
	case CHIP_RV620:
	case CHIP_RV635:
	case CHIP_RS780:
	case CHIP_RS880:
	case CHIP_RV770:
	case CHIP_RV730:
	case CHIP_RV710:
	case CHIP_RV740:
	case CHIP_CEDAR:
	case CHIP_REDWOOD:
	case CHIP_JUNIPER:
	case CHIP_CYPRESS:
	case CHIP_HEMLOCK:
	case CHIP_PALM:
		break;
	case CHIP_R100:
	case CHIP_RV100:
	case CHIP_RS100:
	case CHIP_RV200:
	case CHIP_RS200:
	case CHIP_R200:
	case CHIP_RV250:
	case CHIP_RS300:
	case CHIP_RV280:
	case CHIP_R300:
	case CHIP_R350:
	case CHIP_RV350:
	case CHIP_RV380:
	case CHIP_R420:
	case CHIP_R423:
	case CHIP_RV410:
	case CHIP_RS400:
	case CHIP_RS480:
	case CHIP_RS600:
	case CHIP_RS690:
	case CHIP_RS740:
	case CHIP_RV515:
	case CHIP_R520:
	case CHIP_RV530:
	case CHIP_RV560:
	case CHIP_RV570:
	case CHIP_R580:
	default:
		fprintf(stderr, "%s unknown or unsupported chipset 0x%04X\n",
			__func__, radeon->device);
		break;
	}

	/* setup class */
	switch (radeon->family) {
	case CHIP_R600:
	case CHIP_RV610:
	case CHIP_RV630:
	case CHIP_RV670:
	case CHIP_RV620:
	case CHIP_RV635:
	case CHIP_RS780:
	case CHIP_RS880:
		radeon->chip_class = R600;
		/* set default group bytes, overridden by tiling info ioctl */
		radeon->tiling_info.group_bytes = 256;
		break;
	case CHIP_RV770:
	case CHIP_RV730:
	case CHIP_RV710:
	case CHIP_RV740:
		radeon->chip_class = R700;
		/* set default group bytes, overridden by tiling info ioctl */
		radeon->tiling_info.group_bytes = 256;
		break;
	case CHIP_CEDAR:
	case CHIP_REDWOOD:
	case CHIP_JUNIPER:
	case CHIP_CYPRESS:
	case CHIP_HEMLOCK:
	case CHIP_PALM:
		radeon->chip_class = EVERGREEN;
		/* set default group bytes, overridden by tiling info ioctl */
		radeon->tiling_info.group_bytes = 512;
		break;
	default:
		fprintf(stderr, "%s unknown or unsupported chipset 0x%04X\n",
			__func__, radeon->device);
		break;
	}

	if (radeon->chip_class == R600 || radeon->chip_class == R700) {
		if (radeon_drm_get_tiling(radeon))
			return NULL;
	}
	radeon->bomgr = r600_bomgr_create(radeon, 1000000);
	if (radeon->bomgr == NULL) {
		return NULL;
	}
	return radeon;
}

struct radeon *r600_drm_winsys_create(int drmfd)
{
	return radeon_new(drmfd, 0);
}

struct radeon *radeon_decref(struct radeon *radeon)
{
	if (radeon == NULL)
		return NULL;
	if (--radeon->refcount > 0) {
		return NULL;
	}

	if (radeon->bomgr)
		r600_bomgr_destroy(radeon->bomgr);

	if (radeon->fd >= 0)
		drmClose(radeon->fd);

	free(radeon);
	return NULL;
}
