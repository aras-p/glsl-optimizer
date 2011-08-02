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

#include "r600_priv.h"
#include "r600_drm_public.h"
#include "util/u_memory.h"
#include <radeon_drm.h>
#include <xf86drm.h>
#include <errno.h>

#ifndef RADEON_INFO_NUM_TILE_PIPES
#define RADEON_INFO_NUM_TILE_PIPES 0xb
#endif

#ifndef RADEON_INFO_BACKEND_MAP
#define RADEON_INFO_BACKEND_MAP 0xd
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
	return radeon->info.r600_clock_crystal_freq;
}

unsigned r600_get_num_backends(struct radeon *radeon)
{
	return radeon->info.r600_num_backends;
}

unsigned r600_get_num_tile_pipes(struct radeon *radeon)
{
	return radeon->num_tile_pipes;
}

unsigned r600_get_backend_map(struct radeon *radeon)
{
	return radeon->backend_map;
}

unsigned r600_get_minor_version(struct radeon *radeon)
{
	return radeon->info.drm_minor;
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

	switch ((tiling_config & 0xf0) >> 4) {
	case 0:
		radeon->tiling_info.num_banks = 4;
		break;
	case 1:
		radeon->tiling_info.num_banks = 8;
		break;
	case 2:
		radeon->tiling_info.num_banks = 16;
		break;
	default:
		return -EINVAL;

	}

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
	uint32_t tiling_config = radeon->info.r600_tiling_config;

	if (!tiling_config)
		return 0;

	if (radeon->chip_class == R600 || radeon->chip_class == R700) {
		return r600_interpret_tiling(radeon, tiling_config);
	} else {
		return eg_interpret_tiling(radeon, tiling_config);
	}
}

static int radeon_get_num_tile_pipes(struct radeon *radeon)
{
	struct drm_radeon_info info = {};
	uint32_t num_tile_pipes = 0;
	int r;

	info.request = RADEON_INFO_NUM_TILE_PIPES;
	info.value = (uintptr_t)&num_tile_pipes;
	r = drmCommandWriteRead(radeon->info.fd, DRM_RADEON_INFO, &info,
			sizeof(struct drm_radeon_info));
	if (r)
		return r;

	radeon->num_tile_pipes = num_tile_pipes;
	return 0;
}

static int radeon_get_backend_map(struct radeon *radeon)
{
	struct drm_radeon_info info = {};
	uint32_t backend_map = 0;
	int r;

	info.request = RADEON_INFO_BACKEND_MAP;
	info.value = (uintptr_t)&backend_map;
	r = drmCommandWriteRead(radeon->info.fd, DRM_RADEON_INFO, &info,
			sizeof(struct drm_radeon_info));
	if (r)
		return r;

	radeon->backend_map = backend_map;
	radeon->backend_map_valid = TRUE;

	return 0;
}

static int radeon_init_fence(struct radeon *radeon)
{
	radeon->fence = 1;
	radeon->fence_bo = r600_bo(radeon, 4096, 0, 0, 0);
	if (radeon->fence_bo == NULL) {
		return -ENOMEM;
	}
	radeon->cfence = r600_bo_map(radeon, radeon->fence_bo, PIPE_TRANSFER_UNSYNCHRONIZED, NULL);
	*radeon->cfence = 0;
	return 0;
}

struct radeon *radeon_create(struct radeon_winsys *ws)
{
	int r;
	struct radeon *radeon = CALLOC_STRUCT(radeon);
	if (radeon == NULL) {
		return NULL;
	}

	radeon->ws = ws;
	ws->query_info(ws, &radeon->info);

	radeon->family = radeon_family_from_device(radeon->info.pci_id);
	if (radeon->family == CHIP_UNKNOWN) {
		fprintf(stderr, "Unknown chipset 0x%04X\n", radeon->info.pci_id);
		return radeon_destroy(radeon);
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
	case CHIP_SUMO:
	case CHIP_SUMO2:
	case CHIP_BARTS:
	case CHIP_TURKS:
	case CHIP_CAICOS:
		radeon->chip_class = EVERGREEN;
		/* set default group bytes, overridden by tiling info ioctl */
		radeon->tiling_info.group_bytes = 512;
		break;
	case CHIP_CAYMAN:
		radeon->chip_class = CAYMAN;
		/* set default group bytes, overridden by tiling info ioctl */
		radeon->tiling_info.group_bytes = 512;
		break;
	default:
		fprintf(stderr, "%s unknown or unsupported chipset 0x%04X\n",
			__func__, radeon->info.pci_id);
		break;
	}

	if (radeon_drm_get_tiling(radeon))
		return NULL;

	if (radeon->info.drm_minor >= 11) {
		radeon_get_num_tile_pipes(radeon);
		radeon_get_backend_map(radeon);
	}

	radeon->bomgr = r600_bomgr_create(radeon, 1000000);
	if (radeon->bomgr == NULL) {
		return NULL;
	}
	r = radeon_init_fence(radeon);
	if (r) {
		radeon_destroy(radeon);
		return NULL;
	}

	return radeon;
}

struct radeon *radeon_destroy(struct radeon *radeon)
{
	if (radeon == NULL)
		return NULL;

	if (radeon->fence_bo) {
		r600_bo_reference(radeon, &radeon->fence_bo, NULL);
	}

	if (radeon->bomgr)
		r600_bomgr_destroy(radeon->bomgr);

	FREE(radeon);
	return NULL;
}
