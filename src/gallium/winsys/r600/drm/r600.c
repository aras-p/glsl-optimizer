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
 */
#include "xf86drm.h"
#include "radeon_drm.h"
#include "pipe/p_compiler.h"
#include "util/u_inlines.h"
#include <pipebuffer/pb_bufmgr.h>
#include "r600_priv.h"

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

static int r600_get_device(struct radeon *r600)
{
	struct drm_radeon_info info;

	r600->device = 0;
	info.request = RADEON_INFO_DEVICE_ID;
	info.value = (uintptr_t)&r600->device;
	return drmCommandWriteRead(r600->fd, DRM_RADEON_INFO, &info, sizeof(struct drm_radeon_info));
}

struct radeon *r600_new(int fd, unsigned device)
{
	struct radeon *r600;
	int r;

	r600 = calloc(1, sizeof(*r600));
	if (r600 == NULL) {
		return NULL;
	}
	r600->fd = fd;
	r600->device = device;
	if (fd >= 0) {
		r = r600_get_device(r600);
		if (r) {
			R600_ERR("Failed to get device id\n");
			r600_delete(r600);
			return NULL;
		}
	}
	r600->family = radeon_family_from_device(r600->device);
	if (r600->family == CHIP_UNKNOWN) {
		R600_ERR("Unknown chipset 0x%04X\n", r600->device);
		r600_delete(r600);
		return NULL;
	}
	switch (r600->family) {
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
		R600_ERR("unknown or unsupported chipset 0x%04X\n", r600->device);
		break;
	}

	/* setup class */
	switch (r600->family) {
	case CHIP_R600:
	case CHIP_RV610:
	case CHIP_RV630:
	case CHIP_RV670:
	case CHIP_RV620:
	case CHIP_RV635:
	case CHIP_RS780:
	case CHIP_RS880:
		r600->chip_class = R600;
		break;
	case CHIP_RV770:
	case CHIP_RV730:
	case CHIP_RV710:
	case CHIP_RV740:
		r600->chip_class = R700;
		break;
	case CHIP_CEDAR:
	case CHIP_REDWOOD:
	case CHIP_JUNIPER:
	case CHIP_CYPRESS:
	case CHIP_HEMLOCK:
		r600->chip_class = EVERGREEN;
		break;
	default:
		R600_ERR("unknown or unsupported chipset 0x%04X\n", r600->device);
		break;
	}

	return r600;
}

void r600_delete(struct radeon *r600)
{
	if (r600 == NULL)
		return;
	drmClose(r600->fd);
	free(r600);
}
