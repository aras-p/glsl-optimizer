/*
 * Copyright © 2009 Jerome Glisse <glisse@freedesktop.org>
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "xf86drm.h"
#include "radeon_priv.h"
#include "radeon_drm.h"

enum radeon_family radeon_get_family(struct radeon *radeon)
{
	return radeon->family;
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

/* symbol missing drove me crazy hack to get symbol exported */
static void fake(void)
{
	struct radeon_ctx *ctx;
	struct radeon_draw *draw;

	ctx = radeon_ctx(NULL);
	draw = radeon_draw(NULL);
}

struct radeon *radeon_new(int fd, unsigned device)
{
	struct radeon *radeon;
	int r;

	radeon = calloc(1, sizeof(*radeon));
	if (radeon == NULL) {
		fake();
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
		if (r600_init(radeon)) {
			return radeon_decref(radeon);
		}
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
	case CHIP_CEDAR:
	case CHIP_REDWOOD:
	case CHIP_JUNIPER:
	case CHIP_CYPRESS:
	case CHIP_HEMLOCK:
	default:
		fprintf(stderr, "%s unknown or unsupported chipset 0x%04X\n",
			__func__, radeon->device);
		break;
	}
	return radeon;
}

struct radeon *radeon_incref(struct radeon *radeon)
{
	if (radeon == NULL)
		return NULL;
	radeon->refcount++;
	return radeon;
}

struct radeon *radeon_decref(struct radeon *radeon)
{
	if (radeon == NULL)
		return NULL;
	if (--radeon->refcount > 0) {
		return NULL;
	}
	drmClose(radeon->fd);
	free(radeon);
	return NULL;
}

int radeon_reg_id(struct radeon *radeon, unsigned offset, unsigned *typeid, unsigned *stateid, unsigned *id)
{
	unsigned i, j;

	for (i = 0; i < radeon->ntype; i++) {
		if (radeon->type[i].range_start) {
			if (offset >= radeon->type[i].range_start && offset < radeon->type[i].range_end) {
				*typeid = i;
				j = offset - radeon->type[i].range_start;
				j /= radeon->type[i].stride;
				*stateid = radeon->type[i].id + j;
				*id = (offset - radeon->type[i].range_start - radeon->type[i].stride * j) / 4;
				return 0;
			}
		} else {
			for (j = 0; j < radeon->type[i].nstates; j++) {
				if (radeon->type[i].regs[j].offset == offset) {
					*typeid = i;
					*stateid = radeon->type[i].id;
					*id = j;
					return 0;
				}
			}
		}
	}
	fprintf(stderr, "%s unknown register 0x%08X\n", __func__, offset);
	return -EINVAL;
}

unsigned radeon_type_from_id(struct radeon *radeon, unsigned id)
{
	unsigned i;

	for (i = 0; i < radeon->ntype - 1; i++) {
		if (radeon->type[i].id == id)
			return i;
		if (id > radeon->type[i].id && id < radeon->type[i + 1].id)
			return i;
	}
	if (radeon->type[i].id == id)
		return i;
	return -1;
}
