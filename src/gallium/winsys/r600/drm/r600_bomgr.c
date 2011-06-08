/*
 * Copyright 2010 VMWare.
 * Copyright 2010 Red Hat Inc.
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
 *      Jose Fonseca <jrfonseca-at-vmware-dot-com>
 *      Thomas Hellström <thomas-at-vmware-dot-com>
 *      Jerome Glisse <jglisse@redhat.com>
 */
#include <util/u_memory.h>
#include <util/u_double_list.h>
#include <util/u_time.h>
#include <pipebuffer/pb_bufmgr.h>
#include "r600_priv.h"

static void r600_bomgr_timeout_flush(struct r600_bomgr *mgr)
{
	struct r600_bo *bo, *tmp;
	int64_t now;

	now = os_time_get();
	LIST_FOR_EACH_ENTRY_SAFE(bo, tmp, &mgr->delayed, list) {
		if(!os_time_timeout(bo->start, bo->end, now))
			break;

		mgr->num_delayed--;
		bo->manager_id = 0;
		LIST_DEL(&bo->list);
		r600_bo_destroy(mgr->radeon, bo);
	}
}

static INLINE int r600_bo_is_compat(struct r600_bomgr *mgr,
					struct r600_bo *bo,
					unsigned size,
					unsigned alignment,
					unsigned cfence)
{
	if(bo->size < size) {
		return 0;
	}

	/* be lenient with size */
	if(bo->size >= 2*size) {
		return 0;
	}

	if(!pb_check_alignment(alignment, bo->alignment)) {
		return 0;
	}

	if (!fence_is_after(cfence, bo->fence)) {
		return 0;
	}

	return 1;
}

struct r600_bo *r600_bomgr_bo_create(struct r600_bomgr *mgr,
					unsigned size,
					unsigned alignment,
					unsigned cfence)
{
	struct r600_bo *bo, *tmp;
	int64_t now;


	pipe_mutex_lock(mgr->mutex);

	now = os_time_get();
	LIST_FOR_EACH_ENTRY_SAFE(bo, tmp, &mgr->delayed, list) {
		if(r600_bo_is_compat(mgr, bo, size, alignment, cfence)) {
			LIST_DEL(&bo->list);
			--mgr->num_delayed;
			r600_bomgr_timeout_flush(mgr);
			pipe_mutex_unlock(mgr->mutex);
			LIST_INITHEAD(&bo->list);
			pipe_reference_init(&bo->reference, 1);
			return bo;
		}

		if(os_time_timeout(bo->start, bo->end, now)) {
			mgr->num_delayed--;
			bo->manager_id = 0;
			LIST_DEL(&bo->list);
			r600_bo_destroy(mgr->radeon, bo);
		}
	}

	pipe_mutex_unlock(mgr->mutex);
	return NULL;
}

void r600_bomgr_bo_init(struct r600_bomgr *mgr, struct r600_bo *bo)
{
	LIST_INITHEAD(&bo->list);
	bo->manager_id = 1;
}

boolean r600_bomgr_bo_destroy(struct r600_bomgr *mgr, struct r600_bo *bo)
{
	bo->start = os_time_get();
	bo->end = bo->start + mgr->usecs;
	pipe_mutex_lock(mgr->mutex);
	LIST_ADDTAIL(&bo->list, &mgr->delayed);
	++mgr->num_delayed;
	pipe_mutex_unlock(mgr->mutex);
	return FALSE;
}

void r600_bomgr_destroy(struct r600_bomgr *mgr)
{
	struct r600_bo *bo, *tmp;

	pipe_mutex_lock(mgr->mutex);
	LIST_FOR_EACH_ENTRY_SAFE(bo, tmp, &mgr->delayed, list) {
		mgr->num_delayed--;
		bo->manager_id = 0;
		LIST_DEL(&bo->list);
		r600_bo_destroy(mgr->radeon, bo);
	}
	pipe_mutex_unlock(mgr->mutex);

	FREE(mgr);
}

struct r600_bomgr *r600_bomgr_create(struct radeon *radeon, unsigned usecs)
{
	struct r600_bomgr *mgr;

	mgr = CALLOC_STRUCT(r600_bomgr);
	if (mgr == NULL)
		return NULL;

	mgr->radeon = radeon;
	mgr->usecs = usecs;
	LIST_INITHEAD(&mgr->delayed);
	mgr->num_delayed = 0;
	pipe_mutex_init(mgr->mutex);

	return mgr;
}
