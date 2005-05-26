/*
 * Copyright (C) 2005 Ben Skeggs.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/*
 * Authors:
 *   Ben Skeggs <darktama@iinet.net.au>
 */

/*TODO'S
 *
 * - Implement remaining arb_f_p opcodes
 * - Depth write
 * - Negate on individual components (implement in swizzle code?)
 * - Reuse input/temp regs, if they're no longer needed.
 * - Find out whether there's any benifit in ordering registers the way
 *   fglrx does (see r300_reg.h).
 * - Verify results of opcodes for accuracy, I've only checked them
 *   in specific cases.
 * - and more...
 */

#include "glheader.h"
#include "macros.h"
#include "enums.h"

#include "program.h"
#include "nvfragprog.h"
#include "r300_context.h"
#if USE_ARB_F_P == 1
#include "r300_fragprog.h"
#include "r300_reg.h"

#define PFS_INVAL 0xFFFFFFFF

static void dump_program(struct r300_fragment_program *rp);
static void emit_arith(struct r300_fragment_program *rp, int op,
				pfs_reg_t dest, int mask,
				pfs_reg_t src0, pfs_reg_t src1, pfs_reg_t src2,
				int flags);

/***************************************
 * begin: useful data structions for fragment program generation
 ***************************************/

/* description of r300 native hw instructions */
const struct {
	const char *name;
	int argc;
	int v_op;
	int s_op;
} r300_fpop[] = {
	{ "MAD", 3, R300_FPI0_OUTC_MAD, R300_FPI2_OUTA_MAD },
	{ "DP3", 2, R300_FPI0_OUTC_DP3, PFS_INVAL },
	{ "DP4", 2, R300_FPI0_OUTC_DP4, R300_FPI2_OUTA_DP4 },
	{ "MIN", 2, R300_FPI0_OUTC_MIN, R300_FPI2_OUTA_MIN },
	{ "MAX", 2, R300_FPI0_OUTC_MAX, R300_FPI2_OUTA_MAX },
	{ "CMP", 3, R300_FPI0_OUTC_CMP, R300_FPI2_OUTA_CMP },
	{ "FRC", 1, R300_FPI0_OUTC_FRC, R300_FPI2_OUTA_FRC },
	{ "EX2", 1, R300_FPI0_OUTC_REPL_ALPHA, R300_FPI2_OUTA_EX2 },
	{ "LG2", 1, R300_FPI0_OUTC_REPL_ALPHA, R300_FPI2_OUTA_LG2 },
	{ "RCP", 1, R300_FPI0_OUTC_REPL_ALPHA, R300_FPI2_OUTA_RCP },
	{ "RSQ", 1, R300_FPI0_OUTC_REPL_ALPHA, R300_FPI2_OUTA_RSQ },
	{ "REPL_ALPHA", 1, R300_FPI0_OUTC_REPL_ALPHA, PFS_INVAL }
};

#define MAKE_SWZ3(x, y, z) (MAKE_SWIZZLE4(SWIZZLE_##x, \
											SWIZZLE_##y, \
											SWIZZLE_##z, \
											SWIZZLE_ZERO))

/* vector swizzles r300 can support natively, with a couple of
 * cases we handle specially
 *
 * pfs_reg_t.v_swz/pfs_reg_t.s_swz is an index into this table
 **/
static const struct r300_pfv_swizzle {
	const char *name;
	GLuint hash;	/* swizzle value this matches */
	GLboolean native;
	GLuint base;	/* base value for hw swizzle */
	GLuint stride;	/* difference in base between arg0/1/2 */
	GLboolean dep_sca;
} v_swiz[] = {
/* native swizzles */
	{ "xyz", MAKE_SWZ3(X, Y, Z), GL_TRUE, R300_FPI0_ARGC_SRC0C_XYZ, 4, GL_FALSE },
	{ "xxx", MAKE_SWZ3(X, X, X), GL_TRUE, R300_FPI0_ARGC_SRC0C_XXX, 4, GL_FALSE },
	{ "yyy", MAKE_SWZ3(Y, Y, Y), GL_TRUE, R300_FPI0_ARGC_SRC0C_YYY, 4, GL_FALSE },
	{ "zzz", MAKE_SWZ3(Z, Z, Z), GL_TRUE, R300_FPI0_ARGC_SRC0C_ZZZ, 4, GL_FALSE },
	{ "yzx", MAKE_SWZ3(Y, Z, X), GL_TRUE, R300_FPI0_ARGC_SRC0C_YZX, 1, GL_FALSE },
	{ "zxy", MAKE_SWZ3(Z, X, Y), GL_TRUE, R300_FPI0_ARGC_SRC0C_ZXY, 1, GL_FALSE },
	{ "wzy", MAKE_SWZ3(W, Z, Y), GL_TRUE, R300_FPI0_ARGC_SRC0CA_WZY, 1, GL_TRUE },
/* special cases */
	{ NULL, MAKE_SWZ3(W, W, W), GL_FALSE, 0, 0, GL_FALSE},
	{ NULL, MAKE_SWZ3(ONE, ONE, ONE), GL_FALSE, R300_FPI0_ARGC_ONE, 0, GL_FALSE},
	{ NULL, MAKE_SWZ3(ZERO, ZERO, ZERO), GL_FALSE, R300_FPI0_ARGC_ZERO, 0, GL_FALSE},
	{ NULL, PFS_INVAL, GL_FALSE, R300_FPI0_ARGC_HALF, 0, GL_FALSE},
	{ NULL, PFS_INVAL, GL_FALSE, 0, 0, 0 },
};
#define SWIZZLE_XYZ		0
#define SWIZZLE_XXX		1
#define SWIZZLE_WZY		6
#define SWIZZLE_111		8
#define SWIZZLE_000		9
#define SWIZZLE_HHH		10

#define SWZ_X_MASK (7 << 0)
#define SWZ_Y_MASK (7 << 3)
#define SWZ_Z_MASK (7 << 6)
#define SWZ_W_MASK (7 << 9)
/* used during matching of non-native swizzles */
static const struct {
	GLuint hash;	/* used to mask matching swizzle components */
	int mask;		/* actual outmask */
	int count;		/* count of components matched */
} s_mask[] = {
    { SWZ_X_MASK|SWZ_Y_MASK|SWZ_Z_MASK, 1|2|4, 3},
    { SWZ_X_MASK|SWZ_Y_MASK, 1|2, 2},
    { SWZ_X_MASK|SWZ_Z_MASK, 1|4, 2},
    { SWZ_Y_MASK|SWZ_Z_MASK, 2|4, 2},
    { SWZ_X_MASK, 1, 1},
    { SWZ_Y_MASK, 2, 1},
    { SWZ_Z_MASK, 4, 1},
    { PFS_INVAL, PFS_INVAL, PFS_INVAL}
};

/* mapping from SWIZZLE_* to r300 native values for scalar insns */
static const struct {
	const char *name;
	int base;	/* hw value of swizzle */
	int stride;	/* difference between SRC0/1/2 */
	GLboolean dep_vec;
} s_swiz[] = {
	{ "x", R300_FPI2_ARGA_SRC0C_X, 3, GL_TRUE },
	{ "y", R300_FPI2_ARGA_SRC0C_Y, 3, GL_TRUE },
	{ "z", R300_FPI2_ARGA_SRC0C_Z, 3, GL_TRUE },
	{ "w", R300_FPI2_ARGA_SRC0A	, 1, GL_FALSE },
	{ "0", R300_FPI2_ARGA_ZERO	, 0, GL_FALSE },
	{ "1", R300_FPI2_ARGA_ONE	, 0, GL_FALSE },
	{ ".5", R300_FPI2_ARGA_HALF, 0, GL_FALSE }
};
#define SWIZZLE_HALF 6

/* boiler-plate reg, for convenience */
const pfs_reg_t pfs_default_reg = {
	type: REG_TYPE_TEMP,
	index: 0,
	v_swz: 0 /* matches XYZ in table */,
	s_swz: SWIZZLE_W,
	vcross: 0,
	scross: 0,
	negate: 0,
	has_w: GL_FALSE,
	valid: GL_FALSE
};

/* constant zero source */
const pfs_reg_t pfs_one = {
	type: REG_TYPE_CONST,
	index: 0,
	v_swz: SWIZZLE_111,
	s_swz: SWIZZLE_ONE,
	valid: GL_TRUE
};

/* constant one source */
const pfs_reg_t pfs_zero = {
	type: REG_TYPE_CONST,
	index: 0,
	v_swz: SWIZZLE_000,
	s_swz: SWIZZLE_ZERO,
	valid: GL_TRUE
};

/***************************************
 * end: data structures
 ***************************************/

#define ERROR(fmt, args...) do { \
		fprintf(stderr, "%s::%s(): " fmt "\n", __FILE__, __func__, ##args); \
		rp->error = GL_TRUE; \
} while(0)

static int get_hw_temp(struct r300_fragment_program *rp)
{
	int r = ffs(~rp->hwreg_in_use);
	if (!r) {
		ERROR("Out of hardware temps\n");
		return 0;
	}

	rp->hwreg_in_use |= (1 << --r);
	if (r > rp->max_temp_idx)
		rp->max_temp_idx = r;

	return r;
}

static void free_hw_temp(struct r300_fragment_program *rp, int idx)
{
	rp->hwreg_in_use &= ~(1<<idx);
}

static pfs_reg_t get_temp_reg(struct r300_fragment_program *rp)
{
	pfs_reg_t r = pfs_default_reg;

	r.index = ffs(~rp->temp_in_use);
	if (!r.index) {
		ERROR("Out of program temps\n");
		return r;
	}
	rp->temp_in_use |= (1 << --r.index);
	
	rp->temps[r.index] = get_hw_temp(rp);
	r.valid = GL_TRUE;
	return r;
}

static void free_temp(struct r300_fragment_program *rp, pfs_reg_t r)
{
	if (!rp || !(rp->temp_in_use & (1<<r.index))) return;
	
	free_hw_temp(rp, rp->temps[r.index]);	
	rp->temp_in_use &= ~(1<<r.index);
}

static pfs_reg_t emit_param4fv(struct r300_fragment_program *rp, GLfloat *values)
{
	pfs_reg_t r = pfs_default_reg;
		r.type = REG_TYPE_CONST;
	int pidx;

	pidx = rp->param_nr++;
	r.index = rp->const_nr++;
	if (pidx >= PFS_NUM_CONST_REGS || r.index >= PFS_NUM_CONST_REGS) {
		ERROR("Out of const/param slots!\n");
		return r;
	}
	
	rp->param[pidx].idx = r.index;
	rp->param[pidx].values = values;
	rp->params_uptodate = GL_FALSE;

	r.valid = GL_TRUE;
	return r;
}

static pfs_reg_t emit_const4fv(struct r300_fragment_program *rp, GLfloat *cp)
{ 
	pfs_reg_t r = pfs_default_reg;
		r.type = REG_TYPE_CONST;

	r.index = rp->const_nr++;
	if (r.index >= PFS_NUM_CONST_REGS) {
		ERROR("Out of hw constants!\n");
		return r;
	}
	
	COPY_4V(rp->constant[r.index], cp);

	r.valid = GL_TRUE;
	return r;
}

static pfs_reg_t negate(pfs_reg_t r)
{
	r.negate = 1;
	return r;
}

static int swz_native(struct r300_fragment_program *rp,
				pfs_reg_t src, pfs_reg_t *r)
{
	/* Native swizzle, nothing to see here */
	*r = src;
	r->has_w = GL_TRUE;
	return 3;
}

static int swz_emit_partial(struct r300_fragment_program *rp,
				pfs_reg_t src, pfs_reg_t *r, int mask)
{
	if (!r->valid)
		*r = get_temp_reg(rp);

	/* A partial match, src.v_swz/mask define what parts of the
	 * desired swizzle we match */
	emit_arith(rp, PFS_OP_MAD, *r, s_mask[mask].mask, src, pfs_one, pfs_zero, 0);
	
	return s_mask[mask].count;
}

static int swz_special_case(struct r300_fragment_program *rp,
				pfs_reg_t src, pfs_reg_t *r, int mask)
{
	pfs_reg_t ssrc = pfs_default_reg;

	switch(GET_SWZ(v_swiz[src.v_swz].hash, 0)) {
	case SWIZZLE_W:
		ssrc = get_temp_reg(rp);
		src.v_swz = SWIZZLE_WZY;
		src.vcross = GL_TRUE;
		if (s_mask[mask].count == 3) {
			emit_arith(rp, PFS_OP_MAD, ssrc, WRITEMASK_XW, src, pfs_one, pfs_zero, 0);
			*r = ssrc;
			r->v_swz = SWIZZLE_XXX;
			r->s_swz = SWIZZLE_W;
			r->has_w = GL_TRUE;
		} else {
			if (!r->valid)
				*r = get_temp_reg(rp);
			emit_arith(rp, PFS_OP_MAD, ssrc, WRITEMASK_X, src, pfs_one, pfs_zero, 0);
			ssrc.v_swz = SWIZZLE_XXX;
			emit_arith(rp, PFS_OP_MAD, *r, s_mask[mask].mask, ssrc, pfs_one, pfs_zero, 0);
			free_temp(rp, ssrc);
		}
		break;
	case SWIZZLE_ONE:
	case SWIZZLE_ZERO:
	default:
		ERROR("Unknown special-case swizzle! %d\n", src.v_swz);
		return 0;
	}

	return s_mask[mask].count;
}

static pfs_reg_t swizzle(struct r300_fragment_program *rp,
				pfs_reg_t src, 
				GLuint arbswz)
{
	pfs_reg_t r = pfs_default_reg;
	
	int c_mask = 0;
	int v_matched = 0;
	src.v_swz = SWIZZLE_XYZ;
	src.s_swz = GET_SWZ(arbswz, 3);
	if (src.s_swz >= SWIZZLE_X && src.s_swz <= SWIZZLE_Z)
		src.scross = GL_TRUE;

	do {
		do {
#define CUR_HASH (v_swiz[src.v_swz].hash & s_mask[c_mask].hash)
			if (CUR_HASH == (arbswz & s_mask[c_mask].hash)) {
				if (v_swiz[src.v_swz].native == GL_FALSE)
					v_matched += swz_special_case(rp, src, &r, c_mask);
				else if (s_mask[c_mask].count == 3)
					v_matched += swz_native(rp, src, &r);
				else
					v_matched += swz_emit_partial(rp, src, &r, c_mask);

				if (v_matched == 3) {
					if (!r.has_w) {
						emit_arith(rp, PFS_OP_MAD, r, WRITEMASK_W, src, pfs_one, pfs_zero, 0);
						r.s_swz = SWIZZLE_W;
					}
					
					if (r.type != REG_TYPE_CONST) {
						if (r.v_swz == SWIZZLE_WZY)
							r.vcross = GL_TRUE;
						if (r.s_swz >= SWIZZLE_X && r.s_swz <= SWIZZLE_Z)
							r.scross = GL_TRUE;
					}
					return r;
				}
				
				arbswz &= ~s_mask[c_mask].hash;
			}
		} while(v_swiz[++src.v_swz].hash != PFS_INVAL);
	} while (s_mask[++c_mask].hash != PFS_INVAL);

	ERROR("should NEVER get here\n");
	return r;
}
				
static pfs_reg_t t_src(struct r300_fragment_program *rp,
						struct fp_src_register fpsrc) {
	pfs_reg_t r = pfs_default_reg;

	switch (fpsrc.File) {
	case PROGRAM_TEMPORARY:
		r.index = fpsrc.Index;
		r.valid = GL_TRUE;
		break;
	case PROGRAM_INPUT:
		r.index = fpsrc.Index;
		r.type = REG_TYPE_INPUT;
		r.valid = GL_TRUE;
		break;
	case PROGRAM_LOCAL_PARAM:
		r = emit_param4fv(rp, rp->mesa_program.Base.LocalParams[fpsrc.Index]);
		break;
	case PROGRAM_ENV_PARAM:
		r = emit_param4fv(rp, rp->ctx->FragmentProgram.Parameters[fpsrc.Index]);
		break;
	case PROGRAM_STATE_VAR:
	case PROGRAM_NAMED_PARAM:
		r = emit_param4fv(rp, rp->mesa_program.Parameters->ParameterValues[fpsrc.Index]);
		break;
	default:
		ERROR("unknown SrcReg->File %x\n", fpsrc.File);
		return r;
	}
	
	/* no point swizzling ONE/ZERO/HALF constants... */
	if (r.v_swz < SWIZZLE_111 && r.s_swz < SWIZZLE_ZERO)
		r = swizzle(rp, r, fpsrc.Swizzle);
	
	/* WRONG! Need to be able to do individual component negation,
	 * should probably handle this in the swizzling code unless
	 * all components are negated, then we can do this natively */
	if (fpsrc.NegateBase)
		r.negate = GL_TRUE;

	return r;
}

static pfs_reg_t t_dst(struct r300_fragment_program *rp,
				struct fp_dst_register dest) {
	pfs_reg_t r = pfs_default_reg;
	
	switch (dest.File) {
	case PROGRAM_TEMPORARY:
		r.index = dest.Index;
		r.valid = GL_TRUE;
		return r;
	case PROGRAM_OUTPUT:
		r.type = REG_TYPE_OUTPUT;
		switch (dest.Index) {
		case 0:
			r.valid = GL_TRUE;
			return r;
		case 1:
			ERROR("I don't know how to write depth!\n");
			return r;
		default:
			ERROR("Bad DstReg->Index 0x%x\n", dest.Index);
			return r;
		}
	default:
		ERROR("Bad DstReg->File 0x%x\n", dest.File);
		return r;
	}
}

static void sync_streams(struct r300_fragment_program *rp) {
	/* Bring vector/scalar streams into sync, inserting nops into
	 * whatever stream is lagging behind
	 *
	 * I'm using "MAD t0, t0, 1.0, 0.0" as a NOP
	 */
	while (rp->v_pos != rp->s_pos) {
		if (rp->s_pos > rp->v_pos) {
			rp->alu.inst[rp->v_pos].inst0 = 0x00050A80;
			rp->alu.inst[rp->v_pos].inst1 = 0x03820800;
			rp->v_pos++;
		} else {
			rp->alu.inst[rp->s_pos].inst2 = 0x00040889;
			rp->alu.inst[rp->s_pos].inst3 = 0x00820800;
			rp->s_pos++;
		}
	}	
}

static void emit_tex(struct r300_fragment_program *rp,
				struct fp_instruction *fpi,
				int opcode)
{
	pfs_reg_t coord = t_src(rp, fpi->SrcReg[0]);
	pfs_reg_t dest = t_dst(rp, fpi->DstReg);
	int unit = fpi->TexSrcUnit;
	int hwsrc, hwdest, flags = 0;

	switch (coord.type) {
	case REG_TYPE_TEMP:
		hwsrc = rp->temps[coord.index];
		break;
	case REG_TYPE_INPUT:
		hwsrc = rp->inputs[coord.index];
		break;
	case REG_TYPE_CONST:
		hwsrc = coord.index;
		flags = R300_FPITX_SRC_CONST;
		break;
	default:
		ERROR("Unknown coord.type = %d\n", coord.type);
		return;
	}
	hwdest = rp->temps[dest.index];

	/* Indirection if source has been written in this node, or if the dest has
	 * been read/written in this node
	 */
	if ((coord.type != REG_TYPE_CONST && (rp->dest_in_node & (1<<hwsrc))) ||
					(rp->used_in_node & (1<<hwdest))) {
			
		if (rp->cur_node == 3) { /* We only support 4 natively */
			ERROR("too many levels of texture indirection\n");
			return;
		}
		/* Finish off current node */
		sync_streams(rp);
		rp->node[rp->cur_node].alu_end = rp->v_pos - 1;

		/* Start new node */
		rp->cur_node++;
		rp->used_in_node = 0;
		rp->dest_in_node = 0;
		rp->node[rp->cur_node].tex_offset = rp->tex.length;
		rp->node[rp->cur_node].alu_offset = rp->v_pos;
		rp->node[rp->cur_node].tex_end = -1;
		rp->node[rp->cur_node].alu_end = -1;	
	}
	
	if (rp->cur_node == 0) rp->first_node_has_tex = 1;

    rp->tex.inst[rp->tex.length++] = 0
        | (hwsrc << R300_FPITX_SRC_SHIFT)
        | (hwdest << R300_FPITX_DST_SHIFT)
        | (unit << R300_FPITX_IMAGE_SHIFT)
        | (opcode << R300_FPITX_OPCODE_SHIFT) /* not entirely sure about this */
		| flags;
	rp->dest_in_node |= (1 << hwdest);
	
	rp->node[rp->cur_node].tex_end++;
}

static void emit_arith(struct r300_fragment_program *rp, int op,
				pfs_reg_t dest, int mask,
				pfs_reg_t src0, pfs_reg_t src1, pfs_reg_t src2,
				int flags)
{
	pfs_reg_t src[3] = { src0, src1, src2 };
	int hwdest, hwsrc;
	int argc;
	int v_idx = rp->v_pos, s_idx = rp->s_pos;
	GLuint inst[4] = { 0, 0, 0, 0 }; 
	int vop, sop;
	int i;

#define ARG_NEG	(1<<5)
#define ARG_ABS (1<<6)
#define ARG_STRIDE 7
#define SRC_CONST (1<<5)
#define SRC_STRIDE 6

	if (!dest.valid || !src0.valid || !src1.valid || !src2.valid) {
		ERROR("invalid register.  dest/src0/src1/src2 valid = %d/%d/%d/%d\n",
						dest.valid, src0.valid, src1.valid, src2.valid);
		return;
	}

	/* check opcode */
	if (op > MAX_PFS_OP) {
		ERROR("unknown opcode!\n");
		return;
	}
	argc = r300_fpop[op].argc;
	vop = r300_fpop[op].v_op;
	sop = r300_fpop[op].s_op;

	/* grab hwregs of dest */
	switch (dest.type) {
	case REG_TYPE_TEMP:
		hwdest = rp->temps[dest.index];
		rp->dest_in_node |= (1 << hwdest);
		rp->used_in_node |= (1 << hwdest);
		break;
	case REG_TYPE_OUTPUT:
		hwdest = 0;
		break;
	default:
		ERROR("invalid dest reg type %d\n", dest.type);
		return;
	}

	/* grab hwregs of sources */
	for (i=0;i<3;i++) {
		if (i<argc) {
			/* Decide on hardware source index */
			switch (src[i].type) {
			case REG_TYPE_INPUT:
				hwsrc = rp->inputs[src[i].index];
				rp->used_in_node |= (1 << hwsrc);

				inst[1] |= hwsrc << (i * SRC_STRIDE);
				inst[3] |= hwsrc << (i * SRC_STRIDE);
				break;
			case REG_TYPE_TEMP:
				/* make sure insn ordering is right... */
				if ((src[i].vcross && v_idx < s_idx) ||
					(src[i].scross && s_idx < v_idx)) {
					sync_streams(rp);
					v_idx = s_idx = rp->v_pos;
				}
		
				hwsrc = rp->temps[src[i].index];
				rp->used_in_node |= (1 << hwsrc);

				inst[1] |= hwsrc << (i * SRC_STRIDE);
				inst[3] |= hwsrc << (i * SRC_STRIDE);
				break;
			case REG_TYPE_CONST:
				hwsrc = src[i].index;

				inst[1] |= ((hwsrc | SRC_CONST) << (i * SRC_STRIDE));
				inst[3] |= ((hwsrc | SRC_CONST) << (i * SRC_STRIDE));
				break;
			default:
				ERROR("invalid source reg\n");
				return;
			}

			/* Swizzling/Negation */
			if (vop == R300_FPI0_OUTC_REPL_ALPHA)
				inst[0] |= R300_FPI0_ARGC_ZERO << (i * ARG_STRIDE);
			else
				inst[0] |= (v_swiz[src[i].v_swz].base + (i * v_swiz[src[i].v_swz].stride)) << (i*ARG_STRIDE);
			inst[2] |= (s_swiz[src[i].s_swz].base + (i * s_swiz[src[i].s_swz].stride)) << (i*ARG_STRIDE);

			if (src[i].negate) {
				inst[0] |= ARG_NEG << (i * ARG_STRIDE);
				inst[2] |= ARG_NEG << (i * ARG_STRIDE);
			}
			
			if (flags & PFS_FLAG_ABS) {
				inst[0] |= ARG_ABS << (i * ARG_STRIDE);
				inst[2] |= ARG_ABS << (i * ARG_STRIDE);	
			}
		} else {
			/* read constant 0, use zero swizzle aswell */
			inst[0] |= R300_FPI0_ARGC_ZERO << (i*ARG_STRIDE);
			inst[1] |= SRC_CONST << (i*SRC_STRIDE);
			inst[2] |= R300_FPI2_ARGA_ZERO << (i*ARG_STRIDE);
			inst[3] |= SRC_CONST << (i*SRC_STRIDE);
		}
	}

	if (flags & PFS_FLAG_SAT) {
		vop |= R300_FPI0_OUTC_SAT;
		sop |= R300_FPI2_OUTA_SAT;
	}
		
	if (mask & WRITEMASK_XYZ) {
		if (r300_fpop[op].v_op == R300_FPI0_OUTC_REPL_ALPHA) {
			sync_streams(rp);
			s_idx = v_idx = rp->v_pos;
		}
		rp->alu.inst[v_idx].inst0 = inst[0] | vop;
		rp->alu.inst[v_idx].inst1 = inst[1] |
				(hwdest << R300_FPI1_DSTC_SHIFT) |
				((mask & WRITEMASK_XYZ) << (dest.type == REG_TYPE_OUTPUT ? 26 : 23));
		rp->v_pos = v_idx + 1;
	}
	
	if ((mask & WRITEMASK_W) || r300_fpop[op].v_op == R300_FPI0_OUTC_REPL_ALPHA) {
		rp->alu.inst[s_idx].inst2 = inst[2] | sop;
		rp->alu.inst[s_idx].inst3 = inst[3] |
				(hwdest << R300_FPI3_DSTA_SHIFT) |
				(((mask & WRITEMASK_W)?1:0) << (dest.type == REG_TYPE_OUTPUT ? 24 : 23));
		rp->s_pos = s_idx + 1;
	}

//	sync_streams(rp);
	return;
};
	
static GLboolean parse_program(struct r300_fragment_program *rp)
{	
	struct fragment_program *mp = &rp->mesa_program;
	const struct fp_instruction *inst = mp->Instructions;
	struct fp_instruction *fpi;
	pfs_reg_t src0, src1, src2, dest, temp;
	int flags = 0;

	if (!inst || inst[0].Opcode == FP_OPCODE_END) {
		ERROR("empty program?\n");
		return GL_FALSE;
	}

	for (fpi=mp->Instructions; fpi->Opcode != FP_OPCODE_END; fpi++) {
		if (fpi->Saturate) {
			flags = PFS_FLAG_SAT;
		}
		
		switch (fpi->Opcode) {
		case FP_OPCODE_ABS:
			ERROR("unknown fpi->Opcode %d\n", fpi->Opcode);
			break;
		case FP_OPCODE_ADD:
			emit_arith(rp, PFS_OP_MAD, t_dst(rp, fpi->DstReg), fpi->DstReg.WriteMask,
							t_src(rp, fpi->SrcReg[0]),
							pfs_one,
							t_src(rp, fpi->SrcReg[1]),
							flags);
			break;
		case FP_OPCODE_CMP:
		case FP_OPCODE_COS:
			ERROR("unknown fpi->Opcode %d\n", fpi->Opcode);
			break;
		case FP_OPCODE_DP3:
			dest = t_dst(rp, fpi->DstReg);
			if (fpi->DstReg.WriteMask & WRITEMASK_W) {
				/* I assume these need to share the same alu slot */
				sync_streams(rp);
				emit_arith(rp, PFS_OP_DP4, dest, WRITEMASK_W, 
								pfs_zero, pfs_zero, pfs_zero,
								flags);
			}
			emit_arith(rp, PFS_OP_DP3, t_dst(rp, fpi->DstReg),
							fpi->DstReg.WriteMask & WRITEMASK_XYZ,
							t_src(rp, fpi->SrcReg[0]),
							t_src(rp, fpi->SrcReg[1]),
							pfs_zero, flags);
			break;
		case FP_OPCODE_DP4:
		case FP_OPCODE_DPH:
		case FP_OPCODE_DST:
		case FP_OPCODE_EX2:
		case FP_OPCODE_FLR:
		case FP_OPCODE_FRC:
		case FP_OPCODE_KIL:
		case FP_OPCODE_LG2:
		case FP_OPCODE_LIT:
			ERROR("unknown fpi->Opcode %d\n", fpi->Opcode);
			break;
		case FP_OPCODE_LRP:
			/* TODO: use the special LRP form if possible */
			src0 = t_src(rp, fpi->SrcReg[0]);
			src1 = t_src(rp, fpi->SrcReg[1]);
			src2 = t_src(rp, fpi->SrcReg[2]);
			// result = tmp0tmp1 + (1 - tmp0)tmp2
			//        = tmp0tmp1 + tmp2 + (-tmp0)tmp2
			//     MAD temp, -tmp0, tmp2, tmp2
			//     MAD result, tmp0, tmp1, temp
			temp = get_temp_reg(rp);
			emit_arith(rp, PFS_OP_MAD, temp, WRITEMASK_XYZW,
							negate(src0), src2, src2, 0);
			emit_arith(rp, PFS_OP_MAD, t_dst(rp, fpi->DstReg), fpi->DstReg.WriteMask,
							src0, src1, temp, flags);
			free_temp(rp, temp);
			break;			
		case FP_OPCODE_MAD:
			emit_arith(rp, PFS_OP_MAD, t_dst(rp, fpi->DstReg), fpi->DstReg.WriteMask,
							t_src(rp, fpi->SrcReg[0]),
							t_src(rp, fpi->SrcReg[1]),
							t_src(rp, fpi->SrcReg[2]),
							flags);
			break;
		case FP_OPCODE_MAX:
		case FP_OPCODE_MIN:
			ERROR("unknown fpi->Opcode %d\n", fpi->Opcode);
			break;
		case FP_OPCODE_MOV:
			emit_arith(rp, PFS_OP_MAD, t_dst(rp, fpi->DstReg), fpi->DstReg.WriteMask,
							t_src(rp, fpi->SrcReg[0]), pfs_one, pfs_zero, 
							flags);
			break;
		case FP_OPCODE_MUL:
			emit_arith(rp, PFS_OP_MAD, t_dst(rp, fpi->DstReg), fpi->DstReg.WriteMask,
							t_src(rp, fpi->SrcReg[0]),
							t_src(rp, fpi->SrcReg[1]),
							pfs_zero,
							flags);
			break;
		case FP_OPCODE_POW:
			/* I don't like this, and it's probably wrong in some
			 * circumstances... Needs checking */
			src0 = t_src(rp, fpi->SrcReg[0]);
			src1 = t_src(rp, fpi->SrcReg[1]);
			dest = t_dst(rp, fpi->DstReg);
			temp = get_temp_reg(rp);
			temp.s_swz = SWIZZLE_X; /* cheat, bypass swizzle code */

			emit_arith(rp, PFS_OP_LG2, temp, WRITEMASK_X,
							src0, pfs_zero, pfs_zero, 0);
			emit_arith(rp, PFS_OP_MAD, temp, WRITEMASK_X,
							temp, src1, pfs_zero, 0);
			emit_arith(rp, PFS_OP_EX2, dest, fpi->DstReg.WriteMask,
							temp, pfs_zero, pfs_zero, 0);
			free_temp(rp, temp);
			break;
		case FP_OPCODE_RCP:
			ERROR("unknown fpi->Opcode %d\n", fpi->Opcode);
			break;
		case FP_OPCODE_RSQ:
			emit_arith(rp, PFS_OP_RSQ, t_dst(rp, fpi->DstReg),
							fpi->DstReg.WriteMask,
							t_src(rp, fpi->SrcReg[0]), pfs_zero, pfs_zero,
							flags | PFS_FLAG_ABS);
			break;
		case FP_OPCODE_SCS:
		case FP_OPCODE_SGE:
		case FP_OPCODE_SIN:
		case FP_OPCODE_SLT:
			ERROR("unknown fpi->Opcode %d\n", fpi->Opcode);
			break;
		case FP_OPCODE_SUB:
			emit_arith(rp, PFS_OP_MAD, t_dst(rp, fpi->DstReg), fpi->DstReg.WriteMask,
							t_src(rp, fpi->SrcReg[0]),
							pfs_one,
							negate(t_src(rp, fpi->SrcReg[1])),
							flags);
			break;
		case FP_OPCODE_SWZ:
			ERROR("unknown fpi->Opcode %d\n", fpi->Opcode);
			break;
		case FP_OPCODE_TEX:
			emit_tex(rp, fpi, R300_FPITX_OP_TEX);
			break;
		case FP_OPCODE_TXB:
			emit_tex(rp, fpi, R300_FPITX_OP_TXB);
			break;
		case FP_OPCODE_TXP:
			emit_tex(rp, fpi, R300_FPITX_OP_TXP);
			break;
		case FP_OPCODE_XPD:
			ERROR("unknown fpi->Opcode %d\n", fpi->Opcode);
			break;
		default:
			ERROR("unknown fpi->Opcode %d\n", fpi->Opcode);
			break;
		}

		if (rp->error)
			return GL_FALSE;
	}

	return GL_TRUE;
}

/* - Init structures
 * - Determine what hwregs each input corresponds to
 */
void init_program(struct r300_fragment_program *rp)
{
	struct fragment_program *mp = &rp->mesa_program;	
	struct fp_instruction *fpi;
	GLuint InputsRead = mp->InputsRead;
	GLuint fp_reg = 0;
	GLuint temps_used = 0; /* for rp->temps[] */
	int i;

	rp->translated = GL_FALSE;
	rp->error = GL_FALSE;

	rp->v_pos = 0;
	rp->s_pos = 0;
	
	rp->tex.length = 0;
	rp->node[0].alu_offset = 0;
	rp->node[0].alu_end = -1;
	rp->node[0].tex_offset = 0;
	rp->node[0].tex_end = -1;
	rp->cur_node = 0;
	rp->first_node_has_tex = 0;
	rp->used_in_node = 0;
	rp->dest_in_node = 0;

	rp->const_nr = 0;
	rp->param_nr = 0;
	rp->params_uptodate = GL_FALSE;

	rp->temp_in_use = 0;
	rp->hwreg_in_use = 0;
	rp->max_temp_idx = 0;
	
	/* Work out what temps the Mesa inputs correspond to, this must match
	 * what setup_rs_unit does, which shouldn't be a problem as rs_unit
	 * configures itself based on the fragprog's InputsRead
	 */

	/* Texcoords come first */
	for (i=0;i<rp->ctx->Const.MaxTextureUnits;i++) {
		if (InputsRead & (FRAG_BIT_TEX0 << i)) {
			rp->hwreg_in_use |= (1<<fp_reg);
			rp->inputs[FRAG_ATTRIB_TEX0+i] = fp_reg++;
		}
	}
	InputsRead &= ~FRAG_BITS_TEX_ANY;

	/* Then primary colour */
	if (InputsRead & FRAG_BIT_COL0) {
		rp->hwreg_in_use |= (1<<fp_reg);
		rp->inputs[FRAG_ATTRIB_COL0] = fp_reg++;
	}
	InputsRead &= ~FRAG_BIT_COL0;
	
	/* Anything else */
	if (InputsRead) {
		WARN_ONCE("Don't know how to handle inputs 0x%x\n", InputsRead);
		/* force read from hwreg 0 for now */
		for (i=0;i<32;i++)
			if (InputsRead & (1<<i)) rp->inputs[i] = 0;
	}

	/* Possibly the worst part of how I went about this... Find out what
	 * temps are used by the mesa program so we don't clobber something
	 * when we need a temp for other reasons.
	 *
	 * Possibly not too bad actually, as we could add to this later and
	 * find out when inputs are last used so we can reuse them as temps.
	 */
	if (!mp->Instructions) {
		ERROR("No instructions found in program\n");
		return;
	}	
	for (fpi=mp->Instructions;fpi->Opcode != FP_OPCODE_END; fpi++) {
		for (i=0;i<3;i++) {
			if (fpi->SrcReg[i].File == PROGRAM_TEMPORARY) {
				if (!(temps_used & (1 << fpi->SrcReg[i].Index))) {
					temps_used |= (1 << fpi->SrcReg[i].Index);
					rp->temps[fpi->SrcReg[i].Index] = get_hw_temp(rp);
				}
			}
		}
		/* needed? surely if a program writes a temp it'll read it again */
		if (fpi->DstReg.File == PROGRAM_TEMPORARY) {
			if (!(temps_used & (1 << fpi->DstReg.Index))) {
				temps_used |= (1 << fpi->DstReg.Index);
				rp->temps[fpi->DstReg.Index] = get_hw_temp(rp);
			}
		}
	}
	rp->temp_in_use = temps_used;
}

void update_params(struct r300_fragment_program *rp) {
	struct fragment_program *mp = &rp->mesa_program;
	int i;

	/* Ask Mesa nicely to fill in ParameterValues for us */
	if (rp->param_nr)
		_mesa_load_state_parameters(rp->ctx, mp->Parameters);

	for (i=0;i<rp->param_nr;i++)
		COPY_4V(rp->constant[rp->param[i].idx], rp->param[i].values);

	rp->params_uptodate = GL_TRUE;
}

void translate_fragment_shader(struct r300_fragment_program *rp)
{
	int i;

	if (!rp->translated) {
		init_program(rp);
	
		if (parse_program(rp) == GL_FALSE) {
			dump_program(rp);
			return;
		}

		/* Finish off */
		sync_streams(rp);
		rp->node[rp->cur_node].alu_end	= rp->v_pos - 1;
		rp->alu_offset					= 0;
		rp->alu_end						= rp->v_pos - 1;
		rp->tex_offset					= 0;
		rp->tex_end						= rp->tex.length - 1;
	
		rp->translated = GL_TRUE;
		if (0) dump_program(rp);
	}

	update_params(rp);
}

/* just some random things... */
static void dump_program(struct r300_fragment_program *rp)
{
	int i;
	static int pc = 0;

	fprintf(stderr, "pc=%d*************************************\n", pc++);
			
	fprintf(stderr, "Mesa program:\n");
	fprintf(stderr, "-------------\n");
		_mesa_debug_fp_inst(rp->mesa_program.NumTexInstructions +
						rp->mesa_program.NumAluInstructions,
		                rp->mesa_program.Instructions);
	fflush(stdout);

	fprintf(stderr, "Hardware program\n");
	fprintf(stderr, "----------------\n");	
	for (i=0;i<(rp->cur_node+1);i++) {
		fprintf(stderr, "NODE %d: alu_offset: %d, tex_offset: %d, alu_end: %d, tex_end: %d\n", i,
						rp->node[i].alu_offset,
						rp->node[i].tex_offset,
						rp->node[i].alu_end,
						rp->node[i].tex_end);
	}
	
/* dump program in pretty_print_command_stream.tcl-readable format */
    fprintf(stderr, "%08x\n", ((rp->alu_end << 16) | (R300_PFS_INSTR0_0 >> 2)));
    for (i=0;i<=rp->alu_end;i++)
        fprintf(stderr, "%08x\n", rp->alu.inst[i].inst0);
    fprintf(stderr, "%08x\n", ((rp->alu_end << 16) | (R300_PFS_INSTR1_0 >> 2)));
    for (i=0;i<=rp->alu_end;i++)
        fprintf(stderr, "%08x\n", rp->alu.inst[i].inst1);
    fprintf(stderr, "%08x\n", ((rp->alu_end << 16) | (R300_PFS_INSTR2_0 >> 2)));
    for (i=0;i<=rp->alu_end;i++)
        fprintf(stderr, "%08x\n", rp->alu.inst[i].inst2);
    fprintf(stderr, "%08x\n", ((rp->alu_end << 16) | (R300_PFS_INSTR3_0 >> 2)));
    for (i=0;i<=rp->alu_end;i++)
        fprintf(stderr, "%08x\n", rp->alu.inst[i].inst3);
    fprintf(stderr, "00000000\n");
	
}
#endif // USE_ARB_F_P == 1
