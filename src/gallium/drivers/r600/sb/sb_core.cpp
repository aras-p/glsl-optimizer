/*
 * Copyright 2013 Vadim Girlin <vadimgirlin@gmail.com>
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
 *      Vadim Girlin
 */

#define SB_RA_SCHED_CHECK DEBUG

extern "C" {
#include "os/os_time.h"
#include "r600_pipe.h"
#include "r600_shader.h"

#include "sb_public.h"
}

#include <stack>
#include <map>

#include "sb_bc.h"
#include "sb_shader.h"
#include "sb_pass.h"
#include "sb_sched.h"

using namespace r600_sb;

static sb_hw_class translate_chip_class(enum chip_class cc);
static sb_hw_chip translate_chip(enum radeon_family rf);

sb_context *r600_sb_context_create(struct r600_context *rctx) {

	sb_context *sctx = new sb_context();

	if (sctx->init(rctx->isa, translate_chip(rctx->b.family),
			translate_chip_class(rctx->b.chip_class))) {
		delete sctx;
		sctx = NULL;
	}

	unsigned df = rctx->screen->b.debug_flags;

	sb_context::dump_pass = df & DBG_SB_DUMP;
	sb_context::dump_stat = df & DBG_SB_STAT;
	sb_context::dry_run = df & DBG_SB_DRY_RUN;
	sb_context::no_fallback = df & DBG_SB_NO_FALLBACK;
	sb_context::safe_math = df & DBG_SB_SAFEMATH;

	sb_context::dskip_start = debug_get_num_option("R600_SB_DSKIP_START", 0);
	sb_context::dskip_end = debug_get_num_option("R600_SB_DSKIP_END", 0);
	sb_context::dskip_mode = debug_get_num_option("R600_SB_DSKIP_MODE", 0);

	return sctx;
}

void r600_sb_context_destroy(void * sctx) {
	if (sctx) {
		sb_context *ctx = static_cast<sb_context*>(sctx);

		if (sb_context::dump_stat) {
			sblog << "\ncontext src stats: ";
			ctx->src_stats.dump();
			sblog << "context opt stats: ";
			ctx->opt_stats.dump();
			sblog << "context diff: ";
			ctx->src_stats.dump_diff(ctx->opt_stats);
		}

		delete ctx;
	}
}

int r600_sb_bytecode_process(struct r600_context *rctx,
                             struct r600_bytecode *bc,
                             struct r600_shader *pshader,
                             int dump_bytecode,
                             int optimize) {
	int r = 0;
	unsigned shader_id = bc->debug_id;

	sb_context *ctx = (sb_context *)rctx->sb_context;
	if (!ctx) {
		rctx->sb_context = ctx = r600_sb_context_create(rctx);
	}

	int64_t time_start = 0;
	if (sb_context::dump_stat) {
		time_start = os_time_get_nano();
	}

	SB_DUMP_STAT( sblog << "\nsb: shader " << shader_id << "\n"; );

	bc_parser parser(*ctx, bc, pshader);

	if ((r = parser.decode())) {
		assert(!"sb: bytecode decoding error");
		return r;
	}

	shader *sh = parser.get_shader();

	if (dump_bytecode) {
		bc_dump(*sh, bc->bytecode, bc->ndw).run();
	}

	if (!optimize) {
		delete sh;
		return 0;
	}

	if (sh->target != TARGET_FETCH) {
		sh->src_stats.ndw = bc->ndw;
		sh->collect_stats(false);
	}

	/* skip some shaders (use shaders from default backend)
	 * dskip_start - range start, dskip_end - range_end,
	 * e.g. start = 5, end = 6 means shaders 5 & 6
	 *
	 * dskip_mode == 0 - disabled,
	 * dskip_mode == 1 - don't process the shaders from the [start;end] range
	 * dskip_mode == 2 - process only the shaders from the range
	 */
	if (sb_context::dskip_mode) {
		if ((sb_context::dskip_start <= shader_id &&
				shader_id <= sb_context::dskip_end) ==
						(sb_context::dskip_mode == 1)) {
			sblog << "sb: skipped shader " << shader_id << " : " << "["
					<< sb_context::dskip_start << "; "
					<< sb_context::dskip_end << "] mode "
					<< sb_context::dskip_mode << "\n";
			return 0;
		}
	}

	if ((r = parser.prepare())) {
		assert(!"sb: bytecode parsing error");
		return r;
	}

	SB_DUMP_PASS( sblog << "\n\n###### after parse\n"; sh->dump_ir(); );

#define SB_RUN_PASS(n, dump) \
	do { \
		r = n(*sh).run(); \
		if (r) { \
			sblog << "sb: error (" << r << ") in the " << #n << " pass.\n"; \
			if (sb_context::no_fallback) \
				return r; \
			sblog << "sb: using unoptimized bytecode...\n"; \
			delete sh; \
			return 0; \
		} \
		if (dump) { \
			SB_DUMP_PASS( sblog << "\n\n###### after " << #n << "\n"; \
				sh->dump_ir();); \
		} \
		assert(!r); \
	} while (0)

	SB_RUN_PASS(ssa_prepare,		0);
	SB_RUN_PASS(ssa_rename,			1);

	if (sh->has_alu_predication)
		SB_RUN_PASS(psi_ops,		1);

	SB_RUN_PASS(liveness,			0);

	sh->dce_flags = DF_REMOVE_DEAD | DF_EXPAND;
	SB_RUN_PASS(dce_cleanup,		0);
	SB_RUN_PASS(def_use,			0);

	sh->set_undef(sh->root->live_before);

	SB_RUN_PASS(if_conversion,		1);

	// if_conversion breaks info about uses, but next pass (peephole)
	// doesn't need it, so we can skip def/use update here
	// until it's really required
	//SB_RUN_PASS(def_use,			0);

	SB_RUN_PASS(peephole,			1);
	SB_RUN_PASS(def_use,			0);

	SB_RUN_PASS(gvn,				1);

	SB_RUN_PASS(def_use,			1);

	sh->dce_flags = DF_REMOVE_DEAD | DF_REMOVE_UNUSED;
	SB_RUN_PASS(dce_cleanup,		1);

	SB_RUN_PASS(ra_split,			0);
	SB_RUN_PASS(def_use,			0);

	// create 'basic blocks'. it's not like we build CFG, they are just
	// container nodes in the correct locations for code placement
	sh->create_bbs();

	SB_RUN_PASS(gcm,				1);

	sh->compute_interferences = true;
	SB_RUN_PASS(liveness,			0);

	sh->dce_flags = DF_REMOVE_DEAD;
	SB_RUN_PASS(dce_cleanup,		1);

	SB_RUN_PASS(ra_coalesce,		1);
	SB_RUN_PASS(ra_init,			1);

	SB_RUN_PASS(post_scheduler,		1);

	sh->expand_bbs();

#if SB_RA_SCHED_CHECK
	// check code correctness after regalloc/scheduler
	SB_RUN_PASS(ra_checker,			0);
#endif

	SB_RUN_PASS(bc_finalizer,		0);

	sh->optimized = true;

	bc_builder builder(*sh);

	if ((r = builder.build())) {
		assert(0);
		return r;
	}

	bytecode &nbc = builder.get_bytecode();

	if (dump_bytecode) {
		bc_dump(*sh, &nbc).run();
	}

	if (!sb_context::dry_run) {

		free(bc->bytecode);
		bc->ndw = nbc.ndw();
		bc->bytecode = (uint32_t*) malloc(bc->ndw << 2);
		nbc.write_data(bc->bytecode);

		bc->ngpr = sh->ngpr;
		bc->nstack = sh->nstack;
	} else {
		SB_DUMP_STAT( sblog << "sb: dry run: optimized bytecode is not used\n"; );
	}

	if (sb_context::dump_stat) {
		int64_t t = os_time_get_nano() - time_start;

		sblog << "sb: processing shader " << shader_id << " done ( "
				<< ((double)t)/1000000.0 << " ms ).\n";

		sh->opt_stats.ndw = bc->ndw;
		sh->collect_stats(true);

		sblog << "src stats: ";
		sh->src_stats.dump();
		sblog << "opt stats: ";
		sh->opt_stats.dump();
		sblog << "diff: ";
		sh->src_stats.dump_diff(sh->opt_stats);
	}

	delete sh;
	return 0;
}

static sb_hw_chip translate_chip(enum radeon_family rf) {
	switch (rf) {

#define TRANSLATE_CHIP(c) case CHIP_##c: return HW_CHIP_##c
		TRANSLATE_CHIP(R600);
		TRANSLATE_CHIP(RV610);
		TRANSLATE_CHIP(RV630);
		TRANSLATE_CHIP(RV670);
		TRANSLATE_CHIP(RV620);
		TRANSLATE_CHIP(RV635);
		TRANSLATE_CHIP(RS780);
		TRANSLATE_CHIP(RS880);
		TRANSLATE_CHIP(RV770);
		TRANSLATE_CHIP(RV730);
		TRANSLATE_CHIP(RV710);
		TRANSLATE_CHIP(RV740);
		TRANSLATE_CHIP(CEDAR);
		TRANSLATE_CHIP(REDWOOD);
		TRANSLATE_CHIP(JUNIPER);
		TRANSLATE_CHIP(CYPRESS);
		TRANSLATE_CHIP(HEMLOCK);
		TRANSLATE_CHIP(PALM);
		TRANSLATE_CHIP(SUMO);
		TRANSLATE_CHIP(SUMO2);
		TRANSLATE_CHIP(BARTS);
		TRANSLATE_CHIP(TURKS);
		TRANSLATE_CHIP(CAICOS);
		TRANSLATE_CHIP(CAYMAN);
		TRANSLATE_CHIP(ARUBA);
#undef TRANSLATE_CHIP

		default:
			assert(!"unknown chip");
			return HW_CHIP_UNKNOWN;
	}
}

static sb_hw_class translate_chip_class(enum chip_class cc) {
	switch(cc) {
		case R600: return HW_CLASS_R600;
		case R700: return HW_CLASS_R700;
		case EVERGREEN: return HW_CLASS_EVERGREEN;
		case CAYMAN: return HW_CLASS_CAYMAN;

		default:
			assert(!"unknown chip class");
			return HW_CLASS_UNKNOWN;
	}
}
