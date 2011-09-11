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
#include "util/u_memory.h"
#include <errno.h>

enum radeon_family r600_get_family(struct radeon *r600)
{
	return r600->family;
}

enum chip_class r600_get_family_class(struct radeon *radeon)
{
	return radeon->chip_class;
}

static unsigned radeon_family_from_device(unsigned device)
{
	switch (device) {
#define CHIPSET(pciid, name, family) case pciid: return CHIP_##family;
#include "pci_ids/r600_pci_ids.h"
#undef CHIPSET
	default:
		return CHIP_UNKNOWN;
	}
}

struct radeon *radeon_create(struct radeon_winsys *ws)
{
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
	if (radeon->family == CHIP_CAYMAN) {
		radeon->chip_class = CAYMAN;
	} else if (radeon->family >= CHIP_CEDAR) {
		radeon->chip_class = EVERGREEN;
	} else if (radeon->family >= CHIP_RV730) {
		radeon->chip_class = R700;
	} else {
		radeon->chip_class = R600;
	}

	return radeon;
}

struct radeon *radeon_destroy(struct radeon *radeon)
{
	if (radeon == NULL)
		return NULL;

	FREE(radeon);
	return NULL;
}
