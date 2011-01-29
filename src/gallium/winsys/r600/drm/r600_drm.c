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
#include <pipebuffer/pb_bufmgr.h>
#include "r600.h"
#include "r600_priv.h"
#include "r600_drm_public.h"
#include "xf86drm.h"
#include "radeon_drm.h"

#ifndef RADEON_INFO_TILING_CONFIG
#define RADEON_INFO_TILING_CONFIG 0x6
#endif

#ifndef RADEON_INFO_CLOCK_CRYSTAL_FREQ
#define RADEON_INFO_CLOCK_CRYSTAL_FREQ 0x9
#endif

enum radeon_family r600_get_family(struct radeon *r600)
{
	return r600->family;
}

enum chip_class r600_get_family_class(struct radeon *radeon)
{
	return radeon->chip_class;
}

struct r600_tiling_info *r600_get_tiling_info(struct radeon *radeon)
{
	return &radeon->tiling_info;
}

unsigned r600_get_clock_crystal_freq(struct radeon *radeon)
{
	return radeon->clock_crystal_freq;
}

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

static int r600_interpret_tiling(struct radeon *radeon, uint32_t tiling_config)
{
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

static int eg_interpret_tiling(struct radeon *radeon, uint32_t tiling_config)
{
	switch (tiling_config & 0xf) {
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

	radeon->tiling_info.num_banks = (tiling_config & 0xf0) >> 4;

	switch ((tiling_config & 0xf00) >> 8) {
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

static int radeon_drm_get_tiling(struct radeon *radeon)
{
	struct drm_radeon_info info;
	int r;
	uint32_t tiling_config = 0;

	info.request = RADEON_INFO_TILING_CONFIG;
	info.value = (uintptr_t)&tiling_config;
	r = drmCommandWriteRead(radeon->fd, DRM_RADEON_INFO, &info,
				sizeof(struct drm_radeon_info));

	if (r)
		return 0;

	if (radeon->chip_class == R600 || radeon->chip_class == R700) {
		r = r600_interpret_tiling(radeon, tiling_config);
	} else {
		r = eg_interpret_tiling(radeon, tiling_config);
	}
	return r;
}

static int radeon_get_clock_crystal_freq(struct radeon *radeon)
{
	struct drm_radeon_info info;
	uint32_t clock_crystal_freq;
	int r;

	radeon->device = 0;
	info.request = RADEON_INFO_CLOCK_CRYSTAL_FREQ;
	info.value = (uintptr_t)&clock_crystal_freq;
	r = drmCommandWriteRead(radeon->fd, DRM_RADEON_INFO, &info,
			sizeof(struct drm_radeon_info));
	if (r)
		return r;

	radeon->clock_crystal_freq = clock_crystal_freq;
	return 0;
}

static int radeon_init_fence(struct radeon *radeon)
{
	radeon->fence = 1;
	radeon->fence_bo = r600_bo(radeon, 4096, 0, 0, 0);
	if (radeon->fence_bo == NULL) {
		return -ENOMEM;
	}
	radeon->cfence = r600_bo_map(radeon, radeon->fence_bo, PB_USAGE_UNSYNCHRONIZED, NULL);
	*radeon->cfence = 0;
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
	case CHIP_BARTS:
	case CHIP_TURKS:
	case CHIP_CAICOS:
		radeon->chip_class = EVERGREEN;
		/* set default group bytes, overridden by tiling info ioctl */
		radeon->tiling_info.group_bytes = 512;
		break;
	default:
		fprintf(stderr, "%s unknown or unsupported chipset 0x%04X\n",
			__func__, radeon->device);
		break;
	}

	if (radeon_drm_get_tiling(radeon))
		return NULL;

	/* get the GPU counter frequency, failure is non fatal */
	radeon_get_clock_crystal_freq(radeon);

	radeon->bomgr = r600_bomgr_create(radeon, 1000000);
	if (radeon->bomgr == NULL) {
		return NULL;
	}
	r = radeon_init_fence(radeon);
	if (r) {
		radeon_decref(radeon);
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

	if (radeon->fence_bo) {
		r600_bo_reference(radeon, &radeon->fence_bo, NULL);
	}

	if (radeon->bomgr)
		r600_bomgr_destroy(radeon->bomgr);

	if (radeon->fd >= 0)
		drmClose(radeon->fd);

	free(radeon);
	return NULL;
}
