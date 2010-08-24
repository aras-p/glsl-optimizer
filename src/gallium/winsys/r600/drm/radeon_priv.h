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
#ifndef RADEON_PRIV_H
#define RADEON_PRIV_H

#include <stdint.h>
#include "xf86drm.h"
#include "xf86drmMode.h"
#include <errno.h>
#include "radeon.h"

struct radeon;
struct radeon_ctx;

/*
 * radeon functions
 */
typedef int (*radeon_state_pm4_t)(struct radeon_state *state);

struct radeon_type {
	const u32			*header_pm4;
	const u32			header_cpm4;
	const u32			*state_pm4;
	const u32			state_cpm4;
	const u32			flush_flags;
	const u32			dirty_flags;
};

typedef int (*radeon_ctx_bo_flush_t)(struct radeon_ctx *ctx, struct radeon_bo *bo, u32 flags, u32 *placement);

struct radeon {
	int				fd;
	int				refcount;
	unsigned			device;
	unsigned			family;
	unsigned			nstate;
	const struct radeon_type	*type;
	radeon_ctx_bo_flush_t		bo_flush;
};

extern struct radeon *radeon_new(int fd, unsigned device);
extern struct radeon *radeon_incref(struct radeon *radeon);
extern struct radeon *radeon_decref(struct radeon *radeon);
extern unsigned radeon_family_from_device(unsigned device);
extern int radeon_is_family_compatible(unsigned family1, unsigned family2);
extern int radeon_reg_id(struct radeon *radeon, unsigned offset, unsigned *typeid, unsigned *stateid, unsigned *id);
extern unsigned radeon_type_from_id(struct radeon *radeon, unsigned id);

int radeon_ctx_draw(struct radeon_ctx *ctx);
int radeon_ctx_reloc(struct radeon_ctx *ctx, struct radeon_bo *bo,
			unsigned id, unsigned *placement);

/*
 * r600/r700 context functions
 */
extern int r600_init(struct radeon *radeon);
extern int r600_ctx_draw(struct radeon_ctx *ctx);
extern int r600_ctx_next_reloc(struct radeon_ctx *ctx, unsigned *reloc);

/*
 * radeon state functions
 */
extern u32 radeon_state_register_get(struct radeon_state *state, unsigned offset);
extern int radeon_state_register_set(struct radeon_state *state, unsigned offset, u32 value);
extern struct radeon_state *radeon_state_duplicate(struct radeon_state *state);
extern int radeon_state_replace_always(struct radeon_state *ostate, struct radeon_state *nstate);
extern int radeon_state_pm4_generic(struct radeon_state *state);

/*
 * radeon draw functions
 */
extern int radeon_draw_pm4(struct radeon_draw *draw);

#endif
