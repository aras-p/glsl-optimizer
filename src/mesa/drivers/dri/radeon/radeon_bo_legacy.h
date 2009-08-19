/* 
 * Copyright © 2008 Nicolai Haehnle
 * Copyright © 2008 Jérôme Glisse
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 */
/*
 * Authors:
 *      Aapo Tahkola <aet@rasterburn.org>
 *      Nicolai Haehnle <prefect_@gmx.net>
 *      Jérôme Glisse <glisse@freedesktop.org>
 */
#ifndef RADEON_BO_LEGACY_H
#define RADEON_BO_LEGACY_H

#include "radeon_screen.h"

void radeon_bo_legacy_pending(struct radeon_bo *bo, uint32_t pending);
int radeon_bo_legacy_validate(struct radeon_bo *bo,
                              uint32_t *soffset,
                              uint32_t *eoffset);
struct radeon_bo_manager *radeon_bo_manager_legacy_ctor(struct radeon_screen *scrn);
void radeon_bo_manager_legacy_dtor(struct radeon_bo_manager *bom);
void radeon_bo_legacy_texture_age(struct radeon_bo_manager *bom);
unsigned radeon_bo_legacy_relocs_size(struct radeon_bo *bo);
struct radeon_bo *radeon_legacy_bo_alloc_fake(struct radeon_bo_manager *bom,
					      int size,
	                                      uint32_t offset);
void legacy_track_pending(struct radeon_bo_manager *bom, int debug);

#endif
