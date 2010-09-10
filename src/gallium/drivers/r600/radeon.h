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
#ifndef RADEON_H
#define RADEON_H

#define RADEON_CTX_MAX_PM4	(64 * 1024 / 4)

#include <stdint.h>

typedef uint64_t		u64;
typedef uint32_t		u32;
typedef uint16_t		u16;
typedef uint8_t			u8;

struct radeon;

enum radeon_family {
	CHIP_UNKNOWN,
	CHIP_R100,
	CHIP_RV100,
	CHIP_RS100,
	CHIP_RV200,
	CHIP_RS200,
	CHIP_R200,
	CHIP_RV250,
	CHIP_RS300,
	CHIP_RV280,
	CHIP_R300,
	CHIP_R350,
	CHIP_RV350,
	CHIP_RV380,
	CHIP_R420,
	CHIP_R423,
	CHIP_RV410,
	CHIP_RS400,
	CHIP_RS480,
	CHIP_RS600,
	CHIP_RS690,
	CHIP_RS740,
	CHIP_RV515,
	CHIP_R520,
	CHIP_RV530,
	CHIP_RV560,
	CHIP_RV570,
	CHIP_R580,
	CHIP_R600,
	CHIP_RV610,
	CHIP_RV630,
	CHIP_RV670,
	CHIP_RV620,
	CHIP_RV635,
	CHIP_RS780,
	CHIP_RS880,
	CHIP_RV770,
	CHIP_RV730,
	CHIP_RV710,
	CHIP_RV740,
	CHIP_CEDAR,
	CHIP_REDWOOD,
	CHIP_JUNIPER,
	CHIP_CYPRESS,
	CHIP_HEMLOCK,
	CHIP_LAST,
};

enum {
	R600_SHADER_PS = 1,
	R600_SHADER_VS,
	R600_SHADER_GS,
	R600_SHADER_FS,
	R600_SHADER_MAX = R600_SHADER_FS,
};

enum radeon_family radeon_get_family(struct radeon *rw);

/*
 * radeon object functions
 */
struct radeon_bo {
	unsigned			refcount;
	unsigned			handle;
	unsigned			size;
	unsigned			alignment;
	unsigned			map_count;
	void				*data;
};
struct radeon_bo *radeon_bo(struct radeon *radeon, unsigned handle,
			unsigned size, unsigned alignment, void *ptr);
int radeon_bo_map(struct radeon *radeon, struct radeon_bo *bo);
void radeon_bo_unmap(struct radeon *radeon, struct radeon_bo *bo);
struct radeon_bo *radeon_bo_incref(struct radeon *radeon, struct radeon_bo *bo);
struct radeon_bo *radeon_bo_decref(struct radeon *radeon, struct radeon_bo *bo);
int radeon_bo_wait(struct radeon *radeon, struct radeon_bo *bo);

struct radeon_stype_info;
/*
 * states functions
 */
struct radeon_state {
	struct radeon			*radeon;
	unsigned			refcount;
	struct radeon_stype_info	*stype;
	unsigned			state_id;
	unsigned			id;
	unsigned			shader_index;
	unsigned			nstates;
	u32				states[64];
	unsigned			npm4;
	unsigned			cpm4;
	u32				pm4_crc;
	u32				pm4[128];
	unsigned			nbo;
	struct radeon_bo		*bo[4];
	unsigned			nreloc;
	unsigned			reloc_pm4_id[8];
	unsigned			reloc_bo_id[8];
	u32				placement[8];
	unsigned			bo_dirty[4];
};

int radeon_state_init(struct radeon_state *rstate, struct radeon *radeon, u32 type, u32 id, u32 shader_class);
void radeon_state_fini(struct radeon_state *state);
int radeon_state_pm4(struct radeon_state *state);
int radeon_state_convert(struct radeon_state *state, u32 stype, u32 id, u32 shader_type);

/*
 * draw functions
 */
struct radeon_draw {
	struct radeon			*radeon;
	struct radeon_state		**state;
};

int radeon_draw_init(struct radeon_draw *draw, struct radeon *radeon);
void radeon_draw_bind(struct radeon_draw *draw, struct radeon_state *state);
void radeon_draw_unbind(struct radeon_draw *draw, struct radeon_state *state);

/*
 * radeon context functions
 */
#pragma pack(1)
struct radeon_cs_reloc {
	uint32_t	handle;
	uint32_t	read_domain;
	uint32_t	write_domain;
	uint32_t	flags;
};
#pragma pack()

struct radeon_ctx {
	struct radeon			*radeon;
	u32				*pm4;
	int				cdwords;
	int				ndwords;
	unsigned			nreloc;
	struct radeon_cs_reloc		*reloc;
	unsigned			nbo;
	struct radeon_bo		**bo;
};

int radeon_ctx_init(struct radeon_ctx *ctx, struct radeon *radeon);
void radeon_ctx_fini(struct radeon_ctx *ctx);
void radeon_ctx_clear(struct radeon_ctx *ctx);
int radeon_ctx_set_draw(struct radeon_ctx *ctx, struct radeon_draw *draw);
int radeon_ctx_submit(struct radeon_ctx *ctx);
void radeon_ctx_dump_bof(struct radeon_ctx *ctx, const char *file);
int radeon_ctx_set_query_state(struct radeon_ctx *ctx, struct radeon_state *state);

/*
 * R600/R700
 */

enum r600_stype {
	R600_STATE_CONFIG,
	R600_STATE_CB_CNTL,
	R600_STATE_RASTERIZER,
	R600_STATE_VIEWPORT,
	R600_STATE_SCISSOR,
	R600_STATE_BLEND,
	R600_STATE_DSA,
	R600_STATE_SHADER,          /* has PS,VS,GS,FS variants */
	R600_STATE_CONSTANT,        /* has PS,VS,GS,FS variants */
	R600_STATE_CBUF,        /* has PS,VS,GS,FS variants */
	R600_STATE_RESOURCE,        /* has PS,VS,GS,FS variants */
	R600_STATE_SAMPLER,         /* has PS,VS,GS,FS variants */
	R600_STATE_SAMPLER_BORDER,  /* has PS,VS,GS,FS variants */
	R600_STATE_CB0,
	R600_STATE_CB1,
	R600_STATE_CB2,
	R600_STATE_CB3,
	R600_STATE_CB4,
	R600_STATE_CB5,
	R600_STATE_CB6,
	R600_STATE_CB7,
	R600_STATE_DB,
	R600_STATE_QUERY_BEGIN,
	R600_STATE_QUERY_END,
	R600_STATE_UCP,
	R600_STATE_VGT,
	R600_STATE_DRAW,
	R600_STATE_CB_FLUSH,
	R600_STATE_DB_FLUSH,
	R600_STATE_MAX,
};

#include "r600_states_inc.h"
#include "eg_states_inc.h"

/* R600 QUERY BEGIN/END */
#define R600_QUERY__OFFSET			0
#define R600_QUERY_SIZE				1
#define R600_QUERY_PM4				128

#endif
