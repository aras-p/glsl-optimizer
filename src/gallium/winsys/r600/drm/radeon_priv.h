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
struct radeon_register {
	unsigned			offset;
	unsigned			need_reloc;
	unsigned			bo_id;
	char				name[64];
};

struct radeon_sub_type {
	int				shader_type;
	const struct radeon_register	*regs;
	unsigned			nstates;
};

struct radeon_stype_info {
	unsigned			stype;
	unsigned			num;
	unsigned			stride;
	radeon_state_pm4_t		pm4;
	struct radeon_sub_type		reginfo[R600_SHADER_MAX];
	unsigned			base_id;
	unsigned			npm4;
};

struct radeon {
	int				fd;
	int				refcount;
	unsigned			device;
	unsigned			family;
	unsigned			nstype;
	struct radeon_stype_info	*stype;
	unsigned max_states;
};

extern struct radeon *radeon_new(int fd, unsigned device);
extern struct radeon *radeon_incref(struct radeon *radeon);
extern struct radeon *radeon_decref(struct radeon *radeon);
extern unsigned radeon_family_from_device(unsigned device);
extern int radeon_is_family_compatible(unsigned family1, unsigned family2);

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
extern int radeon_state_reloc(struct radeon_state *state, unsigned id, unsigned bo_id);

/*
 * radeon draw functions
 */
extern int radeon_draw_pm4(struct radeon_draw *draw);

#endif
