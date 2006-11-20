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
 *   Jerome Glisse <j.glisse@gmail.com>
 */

/*TODO'S
 *
 * - COS/SIN/SCS instructions
 * - Depth write, WPOS/FOGC inputs
 * - FogOption
 * - Verify results of opcodes for accuracy, I've only checked them
 *   in specific cases.
 * - and more...
 */

#include "glheader.h"
#include "macros.h"
#include "enums.h"

#include "program.h"
#include "program_instruction.h"
#include "r300_context.h"
#include "r300_fragprog.h"
#include "r300_reg.h"

#define PFS_INVAL 0xFFFFFFFF
#define COMPILE_STATE struct r300_pfs_compile_state *cs = rp->cs

static void dump_program(struct r300_fragment_program *rp);
static void emit_arith(struct r300_fragment_program *rp, int op,
				pfs_reg_t dest, int mask,
				pfs_reg_t src0, pfs_reg_t src1, pfs_reg_t src2,
				int flags);

/***************************************
 * begin: useful data structions for fragment program generation
 ***************************************/

/* description of r300 native hw instructions */
static const struct {
	const char *name;
	int argc;
	int v_op;
	int s_op;
} r300_fpop[] = {
	{ "MAD", 3, R300_FPI0_OUTC_MAD, R300_FPI2_OUTA_MAD },
	{ "DP3", 2, R300_FPI0_OUTC_DP3, R300_FPI2_OUTA_DP4 },
	{ "DP4", 2, R300_FPI0_OUTC_DP4, R300_FPI2_OUTA_DP4 },
	{ "MIN", 2, R300_FPI0_OUTC_MIN, R300_FPI2_OUTA_MIN },
	{ "MAX", 2, R300_FPI0_OUTC_MAX, R300_FPI2_OUTA_MAX },
	{ "CMP", 3, R300_FPI0_OUTC_CMP, R300_FPI2_OUTA_CMP },
	{ "FRC", 1, R300_FPI0_OUTC_FRC, R300_FPI2_OUTA_FRC },
	{ "EX2", 1, R300_FPI0_OUTC_REPL_ALPHA, R300_FPI2_OUTA_EX2 },
	{ "LG2", 1, R300_FPI0_OUTC_REPL_ALPHA, R300_FPI2_OUTA_LG2 },
	{ "RCP", 1, R300_FPI0_OUTC_REPL_ALPHA, R300_FPI2_OUTA_RCP },
	{ "RSQ", 1, R300_FPI0_OUTC_REPL_ALPHA, R300_FPI2_OUTA_RSQ },
	{ "REPL_ALPHA", 1, R300_FPI0_OUTC_REPL_ALPHA, PFS_INVAL },
	{ "CMPH", 3, R300_FPI0_OUTC_CMPH, PFS_INVAL },
};

#define MAKE_SWZ3(x, y, z) (MAKE_SWIZZLE4(SWIZZLE_##x, \
					  SWIZZLE_##y, \
					  SWIZZLE_##z, \
					  SWIZZLE_ZERO))

#define SLOT_VECTOR	(1<<0)
#define SLOT_SCALAR (1<<3)
#define SLOT_BOTH	(SLOT_VECTOR|SLOT_SCALAR)

/* vector swizzles r300 can support natively, with a couple of
 * cases we handle specially
 *
 * pfs_reg_t.v_swz/pfs_reg_t.s_swz is an index into this table
 **/
static const struct r300_pfs_swizzle {
	GLuint hash;	/* swizzle value this matches */
	GLuint base;	/* base value for hw swizzle */
	GLuint stride;	/* difference in base between arg0/1/2 */
	GLuint flags;
} v_swiz[] = {
/* native swizzles */
	{ MAKE_SWZ3(X, Y, Z), R300_FPI0_ARGC_SRC0C_XYZ, 4, SLOT_VECTOR },
	{ MAKE_SWZ3(X, X, X), R300_FPI0_ARGC_SRC0C_XXX, 4, SLOT_VECTOR },
	{ MAKE_SWZ3(Y, Y, Y), R300_FPI0_ARGC_SRC0C_YYY, 4, SLOT_VECTOR },
	{ MAKE_SWZ3(Z, Z, Z), R300_FPI0_ARGC_SRC0C_ZZZ, 4, SLOT_VECTOR },
	{ MAKE_SWZ3(W, W, W), R300_FPI0_ARGC_SRC0A,     1, SLOT_SCALAR },
	{ MAKE_SWZ3(Y, Z, X), R300_FPI0_ARGC_SRC0C_YZX, 1, SLOT_VECTOR },
	{ MAKE_SWZ3(Z, X, Y), R300_FPI0_ARGC_SRC0C_ZXY, 1, SLOT_VECTOR },
	{ MAKE_SWZ3(W, Z, Y), R300_FPI0_ARGC_SRC0CA_WZY, 1, SLOT_BOTH },
	{ MAKE_SWZ3(ONE, ONE, ONE), R300_FPI0_ARGC_ONE, 0, 0},
	{ MAKE_SWZ3(ZERO, ZERO, ZERO), R300_FPI0_ARGC_ZERO, 0, 0},
	{ PFS_INVAL, R300_FPI0_ARGC_HALF, 0, 0},
	{ PFS_INVAL, 0, 0, 0},
};
#define SWIZZLE_XYZ		0
#define SWIZZLE_XXX		1
#define SWIZZLE_YYY		2
#define SWIZZLE_ZZZ		3
#define SWIZZLE_WWW		4
#define SWIZZLE_YZX		5
#define SWIZZLE_ZXY		6
#define SWIZZLE_WZY		7
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
	int base;	/* hw value of swizzle */
	int stride;	/* difference between SRC0/1/2 */
	GLuint flags;
} s_swiz[] = {
	{ R300_FPI2_ARGA_SRC0C_X, 3, SLOT_VECTOR },
	{ R300_FPI2_ARGA_SRC0C_Y, 3, SLOT_VECTOR },
	{ R300_FPI2_ARGA_SRC0C_Z, 3, SLOT_VECTOR },
	{ R300_FPI2_ARGA_SRC0A  , 1, SLOT_SCALAR },
	{ R300_FPI2_ARGA_ZERO   , 0, 0 },
	{ R300_FPI2_ARGA_ONE    , 0, 0 },
	{ R300_FPI2_ARGA_HALF   , 0, 0 }
};
#define SWIZZLE_HALF 6

/* boiler-plate reg, for convenience */
static const pfs_reg_t undef = {
	type: REG_TYPE_TEMP,
	index: 0,
	v_swz: SWIZZLE_XYZ,
	s_swz: SWIZZLE_W,
	negate_v: 0,
	negate_s: 0,
	absolute: 0,
	no_use: GL_FALSE,
	valid: GL_FALSE
};

/* constant one source */
static const pfs_reg_t pfs_one = {
	type: REG_TYPE_CONST,
	index: 0,
	v_swz: SWIZZLE_111,
	s_swz: SWIZZLE_ONE,
	valid: GL_TRUE
};

/* constant half source */
static const pfs_reg_t pfs_half = {
	type: REG_TYPE_CONST,
	index: 0,
	v_swz: SWIZZLE_HHH,
	s_swz: SWIZZLE_HALF,
	valid: GL_TRUE
};

/* constant zero source */
static const pfs_reg_t pfs_zero = {
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
		fprintf(stderr, "%s::%s(): " fmt "\n",\
			__FILE__, __func__, ##args);  \
		rp->error = GL_TRUE; \
} while(0)

static int get_hw_temp(struct r300_fragment_program *rp)
{
	COMPILE_STATE;
	int r = ffs(~cs->hwreg_in_use);
	if (!r) {
		ERROR("Out of hardware temps\n");
		return 0;
	}

	cs->hwreg_in_use |= (1 << --r);
	if (r > rp->max_temp_idx)
		rp->max_temp_idx = r;

	return r;
}

static int get_hw_temp_tex(struct r300_fragment_program *rp)
{
	COMPILE_STATE;
	int r;

	r = ffs(~(cs->hwreg_in_use | cs->used_in_node));
	if (!r)
		return get_hw_temp(rp); /* Will cause an indirection */

	cs->hwreg_in_use |= (1 << --r);
	if (r > rp->max_temp_idx)
		rp->max_temp_idx = r;

	return r;
}

static void free_hw_temp(struct r300_fragment_program *rp, int idx)
{
	COMPILE_STATE;
	cs->hwreg_in_use &= ~(1<<idx);
}

static pfs_reg_t get_temp_reg(struct r300_fragment_program *rp)
{
	COMPILE_STATE;
	pfs_reg_t r = undef;

	r.index = ffs(~cs->temp_in_use);
	if (!r.index) {
		ERROR("Out of program temps\n");
		return r;
	}
	cs->temp_in_use |= (1 << --r.index);
	
	cs->temps[r.index].refcount = 0xFFFFFFFF;
	cs->temps[r.index].reg = -1;
	r.valid = GL_TRUE;
	return r;
}

static pfs_reg_t get_temp_reg_tex(struct r300_fragment_program *rp)
{
	COMPILE_STATE;
	pfs_reg_t r = undef;

	r.index = ffs(~cs->temp_in_use);
	if (!r.index) {
		ERROR("Out of program temps\n");
		return r;
	}
	cs->temp_in_use |= (1 << --r.index);
	
	cs->temps[r.index].refcount = 0xFFFFFFFF;
	cs->temps[r.index].reg = get_hw_temp_tex(rp);
	r.valid = GL_TRUE;
	return r;
}

static void free_temp(struct r300_fragment_program *rp, pfs_reg_t r)
{
	COMPILE_STATE;
	if (!(cs->temp_in_use & (1<<r.index))) return;
	
	if (r.type == REG_TYPE_TEMP) {
		free_hw_temp(rp, cs->temps[r.index].reg);
		cs->temps[r.index].reg = -1;
		cs->temp_in_use &= ~(1<<r.index);
	} else if (r.type == REG_TYPE_INPUT) {
		free_hw_temp(rp, cs->inputs[r.index].reg);
		cs->inputs[r.index].reg = -1;
	}
}

static pfs_reg_t emit_param4fv(struct r300_fragment_program *rp,
			       GLfloat *values)
{
	pfs_reg_t r = undef;
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
	pfs_reg_t r = undef;
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

static __inline pfs_reg_t negate(pfs_reg_t r)
{
	r.negate_v = 1;
	r.negate_s = 1;
	return r;
}

/* Hack, to prevent clobbering sources used multiple times when
 * emulating non-native instructions
 */
static __inline pfs_reg_t keep(pfs_reg_t r)
{
	r.no_use = GL_TRUE;
	return r;
}

static __inline pfs_reg_t absolute(pfs_reg_t r)
{
	r.absolute = 1;
	return r;
}

static int swz_native(struct r300_fragment_program *rp,
		      pfs_reg_t src, pfs_reg_t *r, GLuint arbneg)
{
	/* Native swizzle, nothing to see here */
	src.negate_s = (arbneg >> 3) & 1;

	if ((arbneg & 0x7) == 0x0) {
		src.negate_v = 0;
		*r = src;
	} else if ((arbneg & 0x7) == 0x7) {
		src.negate_v = 1;
		*r = src;
	} else {
		if (!r->valid)
			*r = get_temp_reg(rp);
		src.negate_v = 1;
		emit_arith(rp, PFS_OP_MAD, *r, arbneg & 0x7,
			   keep(src), pfs_one, pfs_zero, 0);
		src.negate_v = 0;
		emit_arith(rp, PFS_OP_MAD, *r,
			   (arbneg ^ 0x7) | WRITEMASK_W,
			   src, pfs_one, pfs_zero, 0);
	}

	return 3;
}

static int swz_emit_partial(struct r300_fragment_program *rp, pfs_reg_t src,
			    pfs_reg_t *r, int mask, int mc, GLuint arbneg)
{
	GLuint tmp;
	GLuint wmask = 0;

	if (!r->valid)
		*r = get_temp_reg(rp);

	/* A partial match, src.v_swz/mask define what parts of the
	 * desired swizzle we match */
	if (mc + s_mask[mask].count == 3) {
		wmask = WRITEMASK_W;
		src.negate_s = (arbneg >> 3) & 1;
	}

	tmp = arbneg & s_mask[mask].mask;
	if (tmp) {
		tmp = tmp ^ s_mask[mask].mask;
		if (tmp) {
			src.negate_v = 1;
			emit_arith(rp, PFS_OP_MAD, *r,
				   arbneg & s_mask[mask].mask,
				   keep(src), pfs_one, pfs_zero, 0);
			src.negate_v = 0;
			if (!wmask) src.no_use = GL_TRUE;
			else        src.no_use = GL_FALSE;
			emit_arith(rp, PFS_OP_MAD, *r, tmp | wmask,
				   src, pfs_one, pfs_zero, 0);
		} else {
			src.negate_v = 1;
			if (!wmask) src.no_use = GL_TRUE;
			else        src.no_use = GL_FALSE;
			emit_arith(rp, PFS_OP_MAD, *r,
				   (arbneg & s_mask[mask].mask) | wmask,
				   src, pfs_one, pfs_zero, 0);
			src.negate_v = 0;
		}
	} else {
		if (!wmask) src.no_use = GL_TRUE;
		else        src.no_use = GL_FALSE;
		emit_arith(rp, PFS_OP_MAD, *r,
			   s_mask[mask].mask | wmask,
			   src, pfs_one, pfs_zero, 0);
	}

	return s_mask[mask].count;
}

#define swizzle(r, x, y, z, w) do_swizzle(rp, r, \
					  ((SWIZZLE_##x<<0)|	\
					   (SWIZZLE_##y<<3)|	\
					   (SWIZZLE_##z<<6)|	\
					   (SWIZZLE_##w<<9)),	\
					  0)

static pfs_reg_t do_swizzle(struct r300_fragment_program *rp,
			    pfs_reg_t src, GLuint arbswz, GLuint arbneg)
{
	pfs_reg_t r = undef;
	
	int c_mask = 0;
	int v_matched = 0;

	/* If swizzling from something without an XYZW native swizzle,
	 * emit result to a temp, and do new swizzle from the temp.
	 */
	if (src.v_swz != SWIZZLE_XYZ || src.s_swz != SWIZZLE_W) {
		pfs_reg_t temp = get_temp_reg(rp);
		emit_arith(rp, PFS_OP_MAD, temp, WRITEMASK_XYZW, src, pfs_one,
			   pfs_zero, 0);
		src = temp;
	}
	src.s_swz = GET_SWZ(arbswz, 3);

	do {
		do {
#define CUR_HASH (v_swiz[src.v_swz].hash & s_mask[c_mask].hash)
			if (CUR_HASH == (arbswz & s_mask[c_mask].hash)) {
				if (s_mask[c_mask].count == 3)
					v_matched += swz_native(rp, src, &r,
								arbneg);
				else
					v_matched += swz_emit_partial(rp, src,
								      &r,
								      c_mask,
								      v_matched,
								      arbneg);

				if (v_matched == 3)
					return r;

				/* Fill with something invalid.. all 0's was
				 * wrong before, matched SWIZZLE_X.  So all
				 * 1's will be okay for now */
				arbswz |= (PFS_INVAL & s_mask[c_mask].hash);
			}
		} while(v_swiz[++src.v_swz].hash != PFS_INVAL);
		src.v_swz = SWIZZLE_XYZ;
	} while (s_mask[++c_mask].hash != PFS_INVAL);

	ERROR("should NEVER get here\n");
	return r;
}
				
static pfs_reg_t t_src(struct r300_fragment_program *rp,
		       struct prog_src_register fpsrc)
{
	pfs_reg_t r = undef;

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
		r = emit_param4fv(rp,
				  rp->mesa_program.Base.LocalParams[fpsrc.Index]);
		break;
	case PROGRAM_ENV_PARAM:
		r = emit_param4fv(rp,
				  rp->ctx->FragmentProgram.Parameters[fpsrc.Index]);
		break;
	case PROGRAM_STATE_VAR:
	case PROGRAM_NAMED_PARAM:
		r = emit_param4fv(rp,
				  rp->mesa_program.Base.Parameters->ParameterValues[fpsrc.Index]);
		break;
	default:
		ERROR("unknown SrcReg->File %x\n", fpsrc.File);
		return r;
	}

	/* no point swizzling ONE/ZERO/HALF constants... */
	if (r.v_swz < SWIZZLE_111 || r.s_swz < SWIZZLE_ZERO)
		r = do_swizzle(rp, r, fpsrc.Swizzle, fpsrc.NegateBase);
	return r;
}

static pfs_reg_t t_scalar_src(struct r300_fragment_program *rp,
			      struct prog_src_register fpsrc)
{
	struct prog_src_register src = fpsrc;
	int sc = GET_SWZ(fpsrc.Swizzle, 0); /* X */

	src.Swizzle = ((sc<<0)|(sc<<3)|(sc<<6)|(sc<<9));

	return t_src(rp, src);
}

static pfs_reg_t t_dst(struct r300_fragment_program *rp,
		       struct prog_dst_register dest) {
	pfs_reg_t r = undef;
	
	switch (dest.File) {
	case PROGRAM_TEMPORARY:
		r.index = dest.Index;
		r.valid = GL_TRUE;
		return r;
	case PROGRAM_OUTPUT:
		r.type = REG_TYPE_OUTPUT;
		switch (dest.Index) {
		case FRAG_RESULT_COLR:
		case FRAG_RESULT_DEPR:
			r.index = dest.Index;
			r.valid = GL_TRUE;
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

static int t_hw_src(struct r300_fragment_program *rp, pfs_reg_t src,
		    GLboolean tex)
{
	COMPILE_STATE;
	int idx;

	switch (src.type) {
	case REG_TYPE_TEMP:
		/* NOTE: if reg==-1 here, a source is being read that
		 * 	 hasn't been written to. Undefined results */
		if (cs->temps[src.index].reg == -1)
			cs->temps[src.index].reg = get_hw_temp(rp);
		idx = cs->temps[src.index].reg;

		if (!src.no_use && (--cs->temps[src.index].refcount == 0))
			free_temp(rp, src);
		break;
	case REG_TYPE_INPUT:
		idx = cs->inputs[src.index].reg;

		if (!src.no_use && (--cs->inputs[src.index].refcount == 0))
			free_hw_temp(rp, cs->inputs[src.index].reg);
		break;
	case REG_TYPE_CONST:
		return (src.index | SRC_CONST);
	default:
		ERROR("Invalid type for source reg\n");
		return (0 | SRC_CONST);
	}

	if (!tex) cs->used_in_node |= (1 << idx);

	return idx;
}

static int t_hw_dst(struct r300_fragment_program *rp, pfs_reg_t dest,
		    GLboolean tex)
{
	COMPILE_STATE;
	int idx;
	assert(dest.valid);

	switch (dest.type) {
	case REG_TYPE_TEMP:
		if (cs->temps[dest.index].reg == -1) {
			if (!tex)
				cs->temps[dest.index].reg = get_hw_temp(rp);
			else
				cs->temps[dest.index].reg = get_hw_temp_tex(rp);
		}
		idx = cs->temps[dest.index].reg;

		if (!dest.no_use && (--cs->temps[dest.index].refcount == 0))
			free_temp(rp, dest);

		cs->dest_in_node |= (1 << idx);
		cs->used_in_node |= (1 << idx);
		break;
	case REG_TYPE_OUTPUT:
		switch (dest.index) {
		case FRAG_RESULT_COLR:
			rp->node[rp->cur_node].flags |= R300_PFS_NODE_OUTPUT_COLOR;
			break;
		case FRAG_RESULT_DEPR:
			rp->node[rp->cur_node].flags |= R300_PFS_NODE_OUTPUT_DEPTH;
			break;
		}
		return dest.index;
		break;
	default:
		ERROR("invalid dest reg type %d\n", dest.type);
		return 0;
	}
	
	return idx;
}

static void emit_nop(struct r300_fragment_program *rp, GLuint mask,
		     GLboolean sync)
{
	COMPILE_STATE;
	
	if (sync)
		cs->v_pos = cs->s_pos = MAX2(cs->v_pos, cs->s_pos);

	if (mask & WRITEMASK_XYZ) {
		rp->alu.inst[cs->v_pos].inst0 = NOP_INST0;
		rp->alu.inst[cs->v_pos].inst1 = NOP_INST1;
		cs->v_pos++;
	}

	if (mask & WRITEMASK_W) {
		rp->alu.inst[cs->s_pos].inst2 = NOP_INST2;
		rp->alu.inst[cs->s_pos].inst3 = NOP_INST3;
		cs->s_pos++;
	}
}

static void emit_tex(struct r300_fragment_program *rp,
		     struct prog_instruction *fpi,
		     int opcode)
{
	COMPILE_STATE;
	pfs_reg_t coord = t_src(rp, fpi->SrcReg[0]);
	pfs_reg_t dest = undef, rdest = undef;
	GLuint din = cs->dest_in_node, uin = cs->used_in_node;
	int unit = fpi->TexSrcUnit;
	int hwsrc, hwdest;
	
	/* Resolve source/dest to hardware registers */
	hwsrc = t_hw_src(rp, coord, GL_TRUE);
	if (opcode != R300_FPITX_OP_KIL) {
		dest = t_dst(rp, fpi->DstReg);

		/* r300 doesn't seem to be able to do TEX->output reg */
		if (dest.type == REG_TYPE_OUTPUT) {
			rdest = dest;
			dest = get_temp_reg_tex(rp);
		}
		hwdest = t_hw_dst(rp, dest, GL_TRUE);
		
		/* Use a temp that hasn't been used in this node, rather
		 * than causing an indirection
		 */
		if (uin & (1 << hwdest)) {
			free_hw_temp(rp, hwdest);
			hwdest = get_hw_temp_tex(rp);
			cs->temps[dest.index].reg = hwdest;
		}
	} else {
		hwdest = 0;
		unit = 0;
	}
	
	/* Indirection if source has been written in this node, or if the
	 * dest has been read/written in this node
	 */
	if ((coord.type != REG_TYPE_CONST && (din & (1<<hwsrc))) ||
					(uin & (1<<hwdest))) {
			
		/* Finish off current node */
		cs->v_pos = cs->s_pos = MAX2(cs->v_pos, cs->s_pos);
		if (rp->node[rp->cur_node].alu_offset == cs->v_pos) {
			/* No alu instructions in the node? Emit a NOP. */
			emit_nop(rp, WRITEMASK_XYZW, GL_TRUE);
			cs->v_pos = cs->s_pos = MAX2(cs->v_pos, cs->s_pos);
		}
				
		rp->node[rp->cur_node].alu_end =
				cs->v_pos - rp->node[rp->cur_node].alu_offset - 1;
		assert(rp->node[rp->cur_node].alu_end >= 0);

		if (++rp->cur_node >= PFS_MAX_TEX_INDIRECT) {
			ERROR("too many levels of texture indirection\n");
			return;
		}

		/* Start new node */
		rp->node[rp->cur_node].tex_offset = rp->tex.length;
		rp->node[rp->cur_node].alu_offset = cs->v_pos;
		rp->node[rp->cur_node].tex_end = -1;
		rp->node[rp->cur_node].alu_end = -1;	
		rp->node[rp->cur_node].flags = 0;
		cs->used_in_node = 0;
		cs->dest_in_node = 0;
	}
	
	if (rp->cur_node == 0)
		rp->first_node_has_tex = 1;

	rp->tex.inst[rp->tex.length++] = 0
		| (hwsrc << R300_FPITX_SRC_SHIFT)
		| (hwdest << R300_FPITX_DST_SHIFT)
		| (unit << R300_FPITX_IMAGE_SHIFT)
		/* not entirely sure about this */
		| (opcode << R300_FPITX_OPCODE_SHIFT);

	cs->dest_in_node |= (1 << hwdest); 
	if (coord.type != REG_TYPE_CONST)
		cs->used_in_node |= (1 << hwsrc);

	rp->node[rp->cur_node].tex_end++;

	/* Copy from temp to output if needed */
	if (rdest.valid) {
		emit_arith(rp, PFS_OP_MAD, rdest, WRITEMASK_XYZW, dest,
			   pfs_one, pfs_zero, 0);
		free_temp(rp, dest);
	}
}

/* Add sources to FPI1/FPI3 lists.  If source is already on list,
 * reuse the index instead of wasting a source.
 */
static int add_src(struct r300_fragment_program *rp, int reg, int pos,
		   int srcmask)
{
	COMPILE_STATE;
	int csm, i;
	
	/* Look for matches */
	for (i=0,csm=srcmask; i<3; i++,csm=csm<<1) {	
		/* If sources have been allocated in this position(s)... */
		if ((cs->slot[pos].umask & csm) == csm) {
			/* ... and the register number(s) match, re-use the
			   source */
			if (srcmask == SLOT_VECTOR &&
			    cs->slot[pos].vsrc[i] == reg)
				return i;
			if (srcmask == SLOT_SCALAR &&
			    cs->slot[pos].ssrc[i] == reg)
				return i;
			if (srcmask == SLOT_BOTH &&
			    cs->slot[pos].vsrc[i] == reg &&
			    cs->slot[pos].ssrc[i] == reg)
				return i;
		}
	}

	/* Look for free spaces */
	for (i=0,csm=srcmask; i<3; i++,csm=csm<<1) {
		/* If the position(s) haven't been allocated */
		if ((cs->slot[pos].umask & csm) == 0) {
			cs->slot[pos].umask |= csm;

			if (srcmask & SLOT_VECTOR)
				cs->slot[pos].vsrc[i] = reg;
			if (srcmask & SLOT_SCALAR)
				cs->slot[pos].ssrc[i] = reg;
			return i;
		}	
	}
	
	//ERROR("Failed to allocate sources in FPI1/FPI3!\n");
	return 0;
}

/* Determine whether or not to position opcode in the same ALU slot for both
 * vector and scalar portions of an instruction.
 *
 * It's not necessary to force the first case, but it makes disassembled
 * shaders easier to read.
 */
static GLboolean force_same_slot(int vop, int sop,
				 GLboolean emit_vop, GLboolean emit_sop,
				 int argc, pfs_reg_t *src)
{
	int i;

	if (emit_vop && emit_sop)
		return GL_TRUE;

	if (emit_vop && vop == R300_FPI0_OUTC_REPL_ALPHA)
		return GL_TRUE;

	if (emit_vop) {
		for (i=0;i<argc;i++)
			if (src[i].v_swz == SWIZZLE_WZY)
				return GL_TRUE;
	}

	return GL_FALSE;
}

static void emit_arith(struct r300_fragment_program *rp, int op,
		       pfs_reg_t dest, int mask,
		       pfs_reg_t src0, pfs_reg_t src1, pfs_reg_t src2,
		       int flags)
{
	COMPILE_STATE;
	pfs_reg_t src[3] = { src0, src1, src2 };
	int hwsrc[3], sswz[3], vswz[3];
	int hwdest;
	GLboolean emit_vop = GL_FALSE, emit_sop = GL_FALSE;
	int vop, sop, argc;
	int vpos, spos;
	int i;

	vop = r300_fpop[op].v_op;
	sop = r300_fpop[op].s_op;
	argc = r300_fpop[op].argc;

	if ((mask & WRITEMASK_XYZ) || vop == R300_FPI0_OUTC_DP3)
		emit_vop = GL_TRUE;
	if ((mask & WRITEMASK_W) || vop == R300_FPI0_OUTC_REPL_ALPHA)
		emit_sop = GL_TRUE;

	if (dest.type == REG_TYPE_OUTPUT && dest.index == FRAG_RESULT_DEPR)
		emit_vop = GL_FALSE;
					
	if (force_same_slot(vop, sop, emit_vop, emit_sop, argc, src)) {
		vpos = spos = MAX2(cs->v_pos, cs->s_pos);
	} else {
		vpos = cs->v_pos;
		spos = cs->s_pos;
		/* Here is where we'd decide on where a safe place is to
		 * combine this instruction with a previous one.
		 *
		 * This is extremely simple for now.. if a source depends
		 * on the opposite stream, force the same instruction.
		 */
		for (i=0;i<3;i++) {
			if (emit_vop &&
			    (v_swiz[src[i].v_swz].flags & SLOT_SCALAR)) {
				vpos = spos = MAX2(vpos, spos);
				break;
			}
			if (emit_sop &&
			    (s_swiz[src[i].s_swz].flags & SLOT_VECTOR)) {
				vpos = spos = MAX2(vpos, spos);
				break;
			}
		}
	}
	
	/* - Convert src->hwsrc, record for FPI1/FPI3
	 * - Determine ARG parts of FPI0/FPI2, unused args are filled
	 *   with ARG_ZERO.
	 */	
	for (i=0;i<3;i++) {
		int srcpos;
		
		if (i >= argc) {
			vswz[i] = R300_FPI0_ARGC_ZERO;
			sswz[i] = R300_FPI2_ARGA_ZERO;
			continue;
		}
		
		hwsrc[i] = t_hw_src(rp, src[i], GL_FALSE);	

		if (emit_vop && vop != R300_FPI0_OUTC_REPL_ALPHA) {
			srcpos = add_src(rp, hwsrc[i], vpos,
					 v_swiz[src[i].v_swz].flags);	
			vswz[i] = (v_swiz[src[i].v_swz].base +
				   (srcpos * v_swiz[src[i].v_swz].stride)) |
				(src[i].negate_v ? ARG_NEG : 0) |
				(src[i].absolute ? ARG_ABS : 0);
		} else vswz[i] = R300_FPI0_ARGC_ZERO;
		
		if (emit_sop) {
			srcpos = add_src(rp, hwsrc[i], spos,
					 s_swiz[src[i].s_swz].flags);
			sswz[i] = (s_swiz[src[i].s_swz].base +
				   (srcpos * s_swiz[src[i].s_swz].stride)) |
				(src[i].negate_s ? ARG_NEG : 0) |
				(src[i].absolute ? ARG_ABS : 0);	
		} else sswz[i] = R300_FPI2_ARGA_ZERO;
	}
	hwdest = t_hw_dst(rp, dest, GL_FALSE);
	
	if (flags & PFS_FLAG_SAT) {
		vop |= R300_FPI0_OUTC_SAT;
		sop |= R300_FPI2_OUTA_SAT;
	}

	/* Throw the pieces together and get FPI0/1 */
	rp->alu.inst[vpos].inst1 =
			((cs->slot[vpos].vsrc[0] << R300_FPI1_SRC0C_SHIFT) |
			 (cs->slot[vpos].vsrc[1] << R300_FPI1_SRC1C_SHIFT) |
			 (cs->slot[vpos].vsrc[2] << R300_FPI1_SRC2C_SHIFT));
	if (emit_vop) {
		rp->alu.inst[vpos].inst0 = vop |
				(vswz[0] << R300_FPI0_ARG0C_SHIFT) |
				(vswz[1] << R300_FPI0_ARG1C_SHIFT) |
				(vswz[2] << R300_FPI0_ARG2C_SHIFT);

		rp->alu.inst[vpos].inst1 |= hwdest << R300_FPI1_DSTC_SHIFT;
		if (dest.type == REG_TYPE_OUTPUT) {
			if (dest.index == FRAG_RESULT_COLR) {
				rp->alu.inst[vpos].inst1 |=
					(mask & WRITEMASK_XYZ) << R300_FPI1_DSTC_OUTPUT_MASK_SHIFT;
			} else assert(0);
		} else {
			rp->alu.inst[vpos].inst1 |=
					(mask & WRITEMASK_XYZ) << R300_FPI1_DSTC_REG_MASK_SHIFT;
		}
		cs->v_pos = vpos+1;
	} else if (spos >= vpos)
		rp->alu.inst[spos].inst0 = NOP_INST0;

	/* And now FPI2/3 */
	rp->alu.inst[spos].inst3 =
			((cs->slot[spos].ssrc[0] << R300_FPI3_SRC0A_SHIFT) |
			 (cs->slot[spos].ssrc[1] << R300_FPI3_SRC1A_SHIFT) |
			 (cs->slot[spos].ssrc[2] << R300_FPI3_SRC2A_SHIFT));
	if (emit_sop) {
		rp->alu.inst[spos].inst2 = sop |
				sswz[0] << R300_FPI2_ARG0A_SHIFT |
				sswz[1] << R300_FPI2_ARG1A_SHIFT |
				sswz[2] << R300_FPI2_ARG2A_SHIFT;

		if (mask & WRITEMASK_W) {
			if (dest.type == REG_TYPE_OUTPUT) {
				if (dest.index == FRAG_RESULT_COLR) {
					rp->alu.inst[spos].inst3 |= 
							(hwdest << R300_FPI3_DSTA_SHIFT) | R300_FPI3_DSTA_OUTPUT;
				} else if (dest.index == FRAG_RESULT_DEPR) {
					rp->alu.inst[spos].inst3 |= R300_FPI3_DSTA_DEPTH;
				} else assert(0);
			} else {
				rp->alu.inst[spos].inst3 |=
						(hwdest << R300_FPI3_DSTA_SHIFT) | R300_FPI3_DSTA_REG;
			}
		}
		cs->s_pos = spos+1;
	} else if (vpos >= spos)
		rp->alu.inst[vpos].inst2 = NOP_INST2;

	return;
};

#if 0
static pfs_reg_t get_attrib(struct r300_fragment_program *rp, GLuint attr)
{
	struct gl_fragment_program *mp = &rp->mesa_program;
	pfs_reg_t r = undef;

	if (!(mp->Base.InputsRead & (1<<attr))) {
		ERROR("Attribute %d was not provided!\n", attr);
		return undef;
	}

	r.type  = REG_TYPE_INPUT;
	r.index = attr;
	r.valid = GL_TRUE;
	return r;
}
#endif

static GLboolean parse_program(struct r300_fragment_program *rp)
{	
	struct gl_fragment_program *mp = &rp->mesa_program;
	const struct prog_instruction *inst = mp->Base.Instructions;
	struct prog_instruction *fpi;
	pfs_reg_t src[3], dest, temp;
	pfs_reg_t cnst;
	int flags, mask = 0;
	GLfloat cnstv[4] = {0.0, 0.0, 0.0, 0.0};

	if (!inst || inst[0].Opcode == OPCODE_END) {
		ERROR("empty program?\n");
		return GL_FALSE;
	}

	for (fpi=mp->Base.Instructions; fpi->Opcode != OPCODE_END; fpi++) {
		if (fpi->SaturateMode == SATURATE_ZERO_ONE)
			flags = PFS_FLAG_SAT;
		else
			flags = 0;

		if (fpi->Opcode != OPCODE_KIL) {
			dest = t_dst(rp, fpi->DstReg);
			mask = fpi->DstReg.WriteMask;
		}

		switch (fpi->Opcode) {
		case OPCODE_ABS:
			src[0] = t_src(rp, fpi->SrcReg[0]);
			emit_arith(rp, PFS_OP_MAD, dest, mask,
				   absolute(src[0]), pfs_one, pfs_zero,
				   flags);
			break;
		case OPCODE_ADD:
			src[0] = t_src(rp, fpi->SrcReg[0]);
			src[1] = t_src(rp, fpi->SrcReg[1]);
			emit_arith(rp, PFS_OP_MAD, dest, mask,
				   src[0], pfs_one, src[1],
				   flags);
			break;
		case OPCODE_CMP:
			src[0] = t_src(rp, fpi->SrcReg[0]);
			src[1] = t_src(rp, fpi->SrcReg[1]);
			src[2] = t_src(rp, fpi->SrcReg[2]);
			/* ARB_f_p - if src0.c < 0.0 ? src1.c : src2.c
			 *    r300 - if src2.c < 0.0 ? src1.c : src0.c
			 */
			emit_arith(rp, PFS_OP_CMP, dest, mask,
				   src[2], src[1], src[0],
				   flags);
			break;
		case OPCODE_COS:
			/*
			 * cos using taylor serie:
			 * cos(x) = 1 - x^2/2! + x^4/4! - x^6/6!
			 */
			temp = get_temp_reg(rp);
			cnstv[0] = 0.5;
			cnstv[1] = 0.041666667;
			cnstv[2] = 0.001388889;
			cnstv[4] = 0.0;
			cnst = emit_const4fv(rp, cnstv);
			src[0] = t_scalar_src(rp, fpi->SrcReg[0]);

			emit_arith(rp, PFS_OP_MAD, temp,
				   WRITEMASK_XYZ,
				   src[0],
				   src[0],
				   pfs_zero,
				   flags);
			emit_arith(rp, PFS_OP_MAD, temp,
				   WRITEMASK_Y | WRITEMASK_Z,
				   temp, temp,
				   pfs_zero,
				   flags);
			emit_arith(rp, PFS_OP_MAD, temp,
				   WRITEMASK_Z,
				   temp,
				   swizzle(temp, X, X, X, W),
				   pfs_zero,
				   flags);
			emit_arith(rp, PFS_OP_MAD, temp,
				   WRITEMASK_XYZ,
				   temp, cnst,
				   pfs_zero,
				   flags);
			emit_arith(rp, PFS_OP_MAD, temp,
				   WRITEMASK_X,
				   pfs_one,
				   pfs_one,
				   negate(temp),
				   flags);
			emit_arith(rp, PFS_OP_MAD, temp,
				   WRITEMASK_X,
				   temp,
				   pfs_one,
				   swizzle(temp, Y, Y, Y, W),
				   flags);
			emit_arith(rp, PFS_OP_MAD, temp,
				   WRITEMASK_X,
				   temp,
				   pfs_one,
				   negate(swizzle(temp, Z, Z, Z, W)),
				   flags);
			emit_arith(rp, PFS_OP_MAD, dest, mask,
				   swizzle(temp, X, X, X, X),
				   pfs_one,
				   pfs_zero,
				   flags);
			free_temp(rp, temp);
			break;
		case OPCODE_DP3:
			src[0] = t_src(rp, fpi->SrcReg[0]);
			src[1] = t_src(rp, fpi->SrcReg[1]);
			emit_arith(rp, PFS_OP_DP3, dest, mask,
				   src[0], src[1], undef,
				   flags);
			break;
		case OPCODE_DP4:
			src[0] = t_src(rp, fpi->SrcReg[0]);
			src[1] = t_src(rp, fpi->SrcReg[1]);
			emit_arith(rp, PFS_OP_DP4, dest, mask,
				   src[0], src[1], undef,
				   flags);
			break;
		case OPCODE_DPH:
			src[0] = t_src(rp, fpi->SrcReg[0]);
			src[1] = t_src(rp, fpi->SrcReg[1]);
			/* src0.xyz1 -> temp
			 * DP4 dest, temp, src1
			 */
#if 0
			temp = get_temp_reg(rp);
			src[0].s_swz = SWIZZLE_ONE;
			emit_arith(rp, PFS_OP_MAD, temp, mask,
				   src[0], pfs_one, pfs_zero,
				   0);
			emit_arith(rp, PFS_OP_DP4, dest, mask,
				   temp, src[1], undef,
				   flags);	
			free_temp(rp, temp);
#else
			emit_arith(rp, PFS_OP_DP4, dest, mask,
				   swizzle(src[0], X, Y, Z, ONE), src[1],
				   undef, flags);	
#endif
			break;
		case OPCODE_DST:
			src[0] = t_src(rp, fpi->SrcReg[0]);
			src[1] = t_src(rp, fpi->SrcReg[1]);
			/* dest.y = src0.y * src1.y */
			if (mask & WRITEMASK_Y)
				emit_arith(rp, PFS_OP_MAD, dest, WRITEMASK_Y,
					   keep(src[0]), keep(src[1]),
					   pfs_zero, flags);
			/* dest.z = src0.z */
			if (mask & WRITEMASK_Z)
				emit_arith(rp, PFS_OP_MAD, dest, WRITEMASK_Z,
					   src[0], pfs_one, pfs_zero, flags);
			/* result.x = 1.0
			 * result.w = src1.w */
			if (mask & WRITEMASK_XW) {
				src[1].v_swz = SWIZZLE_111; /* Cheat.. */
				emit_arith(rp, PFS_OP_MAD, dest,
					   mask & WRITEMASK_XW,
					   src[1], pfs_one, pfs_zero,
					   flags);
			}
			break;
		case OPCODE_EX2:
			src[0] = t_scalar_src(rp, fpi->SrcReg[0]);
			emit_arith(rp, PFS_OP_EX2, dest, mask,
				   src[0], undef, undef,
				   flags);
			break;
		case OPCODE_FLR:		
			src[0] = t_src(rp, fpi->SrcReg[0]);
			temp = get_temp_reg(rp);
			/* FRC temp, src0
			 * MAD dest, src0, 1.0, -temp
			 */
			emit_arith(rp, PFS_OP_FRC, temp, mask,
				   keep(src[0]), undef, undef,
				   0);
			emit_arith(rp, PFS_OP_MAD, dest, mask,
				   src[0], pfs_one, negate(temp),
				   flags);
			free_temp(rp, temp);
			break;
		case OPCODE_FRC:
			src[0] = t_src(rp, fpi->SrcReg[0]);
			emit_arith(rp, PFS_OP_FRC, dest, mask,
				   src[0], undef, undef,
				   flags);
			break;
		case OPCODE_KIL:
			emit_tex(rp, fpi, R300_FPITX_OP_KIL);
			break;
		case OPCODE_LG2:
			src[0] = t_scalar_src(rp, fpi->SrcReg[0]);
			emit_arith(rp, PFS_OP_LG2, dest, mask,
				   src[0], undef, undef,
				   flags);
			break;
		case OPCODE_LIT:
			/* LIT
			 * if (s.x < 0) t.x = 0; else t.x = s.x;
			 * if (s.y < 0) t.y = 0; else t.y = s.y;
			 * if (s.w >  128.0) t.w =  128.0; else t.w = s.w;
			 * if (s.w < -128.0) t.w = -128.0; else t.w = s.w;
			 * r.x = 1.0
			 * if (t.x > 0) r.y = pow(t.y, t.w); else r.y = 0;
			 * Also r.y = 0 if t.y < 0
			 * For the t.x > 0 FGLRX use the CMPH opcode which
			 * change the compare to (t.x + 0.5) > 0.5 we may
			 * save one instruction by doing CMP -t.x 
			 */
			cnstv[0] = cnstv[1] = cnstv[2] = cnstv[4] = 0.50001;
			src[0] = t_src(rp, fpi->SrcReg[0]);
			temp = get_temp_reg(rp);
			cnst = emit_const4fv(rp, cnstv);
			emit_arith(rp, PFS_OP_CMP, temp,
				   WRITEMASK_X | WRITEMASK_Y,
				   src[0], pfs_zero, src[0], flags);
			emit_arith(rp, PFS_OP_MIN, temp, WRITEMASK_Z,
				   swizzle(keep(src[0]), W, W, W, W),
				   cnst, undef, flags);
			emit_arith(rp, PFS_OP_LG2, temp, WRITEMASK_W,
				   swizzle(temp, Y, Y, Y, Y),
				   undef, undef, flags);
			emit_arith(rp, PFS_OP_MAX, temp, WRITEMASK_Z,
				   temp, negate(cnst), undef, flags);
			emit_arith(rp, PFS_OP_MAD, temp, WRITEMASK_W,
				   temp, swizzle(temp, Z, Z, Z, Z),
				   pfs_zero, flags);
			emit_arith(rp, PFS_OP_EX2, temp, WRITEMASK_W,
				   temp, undef, undef, flags);
			emit_arith(rp, PFS_OP_MAD, dest, WRITEMASK_Y,
				   swizzle(keep(temp), X, X, X, X),
				   pfs_one, pfs_zero, flags);
#if 0
			emit_arith(rp, PFS_OP_MAD, temp, WRITEMASK_X,
				   temp, pfs_one, pfs_half, flags);
			emit_arith(rp, PFS_OP_CMPH, temp, WRITEMASK_Z,
				   swizzle(keep(temp), W, W, W, W),
				   pfs_zero, swizzle(keep(temp), X, X, X, X),
				   flags);
#else
			emit_arith(rp, PFS_OP_CMP, temp, WRITEMASK_Z,
				   pfs_zero,
				   swizzle(keep(temp), W, W, W, W),
				   negate(swizzle(keep(temp), X, X, X, X)),
				   flags);
#endif
			emit_arith(rp, PFS_OP_CMP, dest, WRITEMASK_Z,
				   pfs_zero, temp,
				   negate(swizzle(keep(temp), Y, Y, Y, Y)),
				   flags);
			emit_arith(rp, PFS_OP_MAD, dest,
				   WRITEMASK_X | WRITEMASK_W,
				   pfs_one,
				   pfs_one,
				   pfs_zero,
				   flags);
			free_temp(rp, temp);
			break;
		case OPCODE_LRP:
			src[0] = t_src(rp, fpi->SrcReg[0]);
			src[1] = t_src(rp, fpi->SrcReg[1]);
			src[2] = t_src(rp, fpi->SrcReg[2]);
			/* result = tmp0tmp1 + (1 - tmp0)tmp2
			 *        = tmp0tmp1 + tmp2 + (-tmp0)tmp2
			 *     MAD temp, -tmp0, tmp2, tmp2
			 *     MAD result, tmp0, tmp1, temp
			 */
			temp = get_temp_reg(rp);
			emit_arith(rp, PFS_OP_MAD, temp, mask,
				   negate(keep(src[0])), keep(src[2]), src[2],
				   0);
			emit_arith(rp, PFS_OP_MAD, dest, mask,
				   src[0], src[1], temp,
				   flags);
			free_temp(rp, temp);
			break;			
		case OPCODE_MAD:
			src[0] = t_src(rp, fpi->SrcReg[0]);
			src[1] = t_src(rp, fpi->SrcReg[1]);
			src[2] = t_src(rp, fpi->SrcReg[2]);
			emit_arith(rp, PFS_OP_MAD, dest, mask,
				   src[0], src[1], src[2],
				   flags);
			break;
		case OPCODE_MAX:
			src[0] = t_src(rp, fpi->SrcReg[0]);
			src[1] = t_src(rp, fpi->SrcReg[1]);
			emit_arith(rp, PFS_OP_MAX, dest, mask,
				   src[0], src[1], undef,
				   flags);
			break;
		case OPCODE_MIN:
			src[0] = t_src(rp, fpi->SrcReg[0]);
			src[1] = t_src(rp, fpi->SrcReg[1]);
			emit_arith(rp, PFS_OP_MIN, dest, mask,
				   src[0], src[1], undef,
				   flags);
			break;
		case OPCODE_MOV:
		case OPCODE_SWZ:
			src[0] = t_src(rp, fpi->SrcReg[0]);
			emit_arith(rp, PFS_OP_MAD, dest, mask,
				   src[0], pfs_one, pfs_zero, 
				   flags);
			break;
		case OPCODE_MUL:
			src[0] = t_src(rp, fpi->SrcReg[0]);
			src[1] = t_src(rp, fpi->SrcReg[1]);
			emit_arith(rp, PFS_OP_MAD, dest, mask,
				   src[0], src[1], pfs_zero,
				   flags);
			break;
		case OPCODE_POW:
			src[0] = t_scalar_src(rp, fpi->SrcReg[0]);
			src[1] = t_scalar_src(rp, fpi->SrcReg[1]);
			temp = get_temp_reg(rp);	
			emit_arith(rp, PFS_OP_LG2, temp, WRITEMASK_W,
				   src[0], undef, undef,
				   0);
			emit_arith(rp, PFS_OP_MAD, temp, WRITEMASK_W,
				   temp, src[1], pfs_zero,
				   0);
			emit_arith(rp, PFS_OP_EX2, dest, fpi->DstReg.WriteMask,
				   temp, undef, undef,
				   0);
			free_temp(rp, temp);
			break;
		case OPCODE_RCP:
			src[0] = t_scalar_src(rp, fpi->SrcReg[0]);
			emit_arith(rp, PFS_OP_RCP, dest, mask,
				   src[0], undef, undef,
				   flags);
			break;
		case OPCODE_RSQ:
			src[0] = t_scalar_src(rp, fpi->SrcReg[0]);
			emit_arith(rp, PFS_OP_RSQ, dest, mask,
				   absolute(src[0]), pfs_zero, pfs_zero,
				   flags);
			break;
		case OPCODE_SCS:
			ERROR("SCS not implemented\n");
			break;
		case OPCODE_SGE:
			src[0] = t_src(rp, fpi->SrcReg[0]);
			src[1] = t_src(rp, fpi->SrcReg[1]);
			temp = get_temp_reg(rp);
			/* temp = src0 - src1
			 * dest.c = (temp.c < 0.0) ? 0 : 1
			 */
			emit_arith(rp, PFS_OP_MAD, temp, mask,
				   src[0], pfs_one, negate(src[1]),
				   0);
			emit_arith(rp, PFS_OP_CMP, dest, mask,
				   pfs_one, pfs_zero, temp,
				   0);
			free_temp(rp, temp);
			break;
		case OPCODE_SIN:
			/*
			 * sin using taylor serie:
			 * sin(x) = x - x^3/3! + x^5/5! - x^7/7!
			 */
			temp = get_temp_reg(rp);
			cnstv[0] = 0.333333333;
			cnstv[1] = 0.008333333;
			cnstv[2] = 0.000198413;
			cnstv[4] = 0.0;
			cnst = emit_const4fv(rp, cnstv);
			src[0] = t_scalar_src(rp, fpi->SrcReg[0]);

			emit_arith(rp, PFS_OP_MAD, temp,
				   WRITEMASK_XYZ,
				   src[0],
				   src[0],
				   pfs_zero,
				   flags);
			emit_arith(rp, PFS_OP_MAD, temp,
				   WRITEMASK_Y | WRITEMASK_Z,
				   temp, temp,
				   pfs_zero,
				   flags);
			emit_arith(rp, PFS_OP_MAD, temp,
				   WRITEMASK_Z,
				   temp,
				   swizzle(temp, X, X, X, W),
				   pfs_zero,
				   flags);
			emit_arith(rp, PFS_OP_MAD, temp,
				   WRITEMASK_XYZ,
				   src[0],
				   temp,
				   pfs_zero,
				   flags);
			emit_arith(rp, PFS_OP_MAD, temp,
				   WRITEMASK_XYZ,
				   temp, cnst,
				   pfs_zero,
				   flags);
			emit_arith(rp, PFS_OP_MAD, temp,
				   WRITEMASK_X,
				   src[0],
				   pfs_one,
				   negate(temp),
				   flags);
			emit_arith(rp, PFS_OP_MAD, temp,
				   WRITEMASK_X,
				   temp,
				   pfs_one,
				   swizzle(temp, Y, Y, Y, W),
				   flags);
			emit_arith(rp, PFS_OP_MAD, temp,
				   WRITEMASK_X,
				   temp,
				   pfs_one,
				   negate(swizzle(temp, Z, Z, Z, W)),
				   flags);
			emit_arith(rp, PFS_OP_MAD, dest, mask,
				   swizzle(temp, X, X, X, X),
				   pfs_one,
				   pfs_zero,
				   flags);
			free_temp(rp, temp);
			break;
		case OPCODE_SLT:
			src[0] = t_src(rp, fpi->SrcReg[0]);
			src[1] = t_src(rp, fpi->SrcReg[1]);
			temp = get_temp_reg(rp);
			/* temp = src0 - src1
			 * dest.c = (temp.c < 0.0) ? 1 : 0
			 */
			emit_arith(rp, PFS_OP_MAD, temp, mask,
				   src[0], pfs_one, negate(src[1]),
				   0);
			emit_arith(rp, PFS_OP_CMP, dest, mask,
				   pfs_zero, pfs_one, temp,
				   0);
			free_temp(rp, temp);
			break;
		case OPCODE_SUB:
			src[0] = t_src(rp, fpi->SrcReg[0]);
			src[1] = t_src(rp, fpi->SrcReg[1]);
			emit_arith(rp, PFS_OP_MAD, dest, mask,
				   src[0], pfs_one, negate(src[1]),
				   flags);
			break;
		case OPCODE_TEX:
			emit_tex(rp, fpi, R300_FPITX_OP_TEX);
			break;
		case OPCODE_TXB:
			emit_tex(rp, fpi, R300_FPITX_OP_TXB);
			break;
		case OPCODE_TXP:
			emit_tex(rp, fpi, R300_FPITX_OP_TXP);
			break;
		case OPCODE_XPD: {
			src[0] = t_src(rp, fpi->SrcReg[0]);
			src[1] = t_src(rp, fpi->SrcReg[1]);
			temp = get_temp_reg(rp);
			/* temp = src0.zxy * src1.yzx */
			emit_arith(rp, PFS_OP_MAD, temp, WRITEMASK_XYZ,
				   swizzle(keep(src[0]), Z, X, Y, W),
				   swizzle(keep(src[1]), Y, Z, X, W),
				   pfs_zero,
				   0);
			/* dest.xyz = src0.yzx * src1.zxy - temp 
			 * dest.w	= undefined
			 * */
			emit_arith(rp, PFS_OP_MAD, dest, mask & WRITEMASK_XYZ,
				   swizzle(src[0], Y, Z, X, W),
				   swizzle(src[1], Z, X, Y, W),
				   negate(temp),
				   flags);
			/* cleanup */
			free_temp(rp, temp);
			break;
		}
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
static void init_program(struct r300_fragment_program *rp)
{
	struct r300_pfs_compile_state *cs = NULL;
	struct gl_fragment_program *mp = &rp->mesa_program;	
	struct prog_instruction *fpi;
	GLuint InputsRead = mp->Base.InputsRead;
	GLuint temps_used = 0; /* for rp->temps[] */
	int i,j;

	/* New compile, reset tracking data */
	rp->translated = GL_FALSE;
	rp->error      = GL_FALSE;
	rp->cs = cs	   = &(R300_CONTEXT(rp->ctx)->state.pfs_compile);
	rp->tex.length = 0;
	rp->cur_node   = 0;
	rp->first_node_has_tex = 0;
	rp->const_nr   = 0;
	rp->param_nr   = 0;
	rp->params_uptodate = GL_FALSE;
	rp->max_temp_idx = 0;
	rp->node[0].alu_end = -1;
	rp->node[0].tex_end = -1;
	
	_mesa_memset(cs, 0, sizeof(*rp->cs));
	for (i=0;i<PFS_MAX_ALU_INST;i++) {
		for (j=0;j<3;j++) {
			cs->slot[i].vsrc[j] = SRC_CONST;
			cs->slot[i].ssrc[j] = SRC_CONST;
		}
	}
	
	/* Work out what temps the Mesa inputs correspond to, this must match
	 * what setup_rs_unit does, which shouldn't be a problem as rs_unit
	 * configures itself based on the fragprog's InputsRead
	 *
	 * NOTE: this depends on get_hw_temp() allocating registers in order,
	 * starting from register 0.
	 */

	/* Texcoords come first */
	for (i=0;i<rp->ctx->Const.MaxTextureUnits;i++) {
		if (InputsRead & (FRAG_BIT_TEX0 << i)) {
			cs->inputs[FRAG_ATTRIB_TEX0+i].refcount = 0;
			cs->inputs[FRAG_ATTRIB_TEX0+i].reg = get_hw_temp(rp);
		}
	}
	InputsRead &= ~FRAG_BITS_TEX_ANY;

	/* fragment position treated as a texcoord */
	if (InputsRead & FRAG_BIT_WPOS) {
		cs->inputs[FRAG_ATTRIB_WPOS].refcount = 0;
		cs->inputs[FRAG_ATTRIB_WPOS].reg = get_hw_temp(rp);
	}
	InputsRead &= ~FRAG_BIT_WPOS;

	/* Then primary colour */
	if (InputsRead & FRAG_BIT_COL0) {
		cs->inputs[FRAG_ATTRIB_COL0].refcount = 0;
		cs->inputs[FRAG_ATTRIB_COL0].reg = get_hw_temp(rp);
	}
	InputsRead &= ~FRAG_BIT_COL0;
	
	/* Secondary color */
	if (InputsRead & FRAG_BIT_COL1) {
		cs->inputs[FRAG_ATTRIB_COL1].refcount = 0;
		cs->inputs[FRAG_ATTRIB_COL1].reg = get_hw_temp(rp);
	}
	InputsRead &= ~FRAG_BIT_COL1;

	/* Anything else */
	if (InputsRead) {
		WARN_ONCE("Don't know how to handle inputs 0x%x\n",
			  InputsRead);
		/* force read from hwreg 0 for now */
		for (i=0;i<32;i++)
			if (InputsRead & (1<<i)) cs->inputs[i].reg = 0;
	}

	/* Pre-parse the mesa program, grabbing refcounts on input/temp regs.
	 * That way, we can free up the reg when it's no longer needed
	 */
	if (!mp->Base.Instructions) {
		ERROR("No instructions found in program\n");
		return;
	}

	for (fpi=mp->Base.Instructions;fpi->Opcode != OPCODE_END; fpi++) {
		int idx;
		
		for (i=0;i<3;i++) {
			idx = fpi->SrcReg[i].Index;
			switch (fpi->SrcReg[i].File) {
			case PROGRAM_TEMPORARY:
				if (!(temps_used & (1<<idx))) {
					cs->temps[idx].reg = -1;
					cs->temps[idx].refcount = 1;
					temps_used |= (1 << idx);
				} else
					cs->temps[idx].refcount++;
				break;
			case PROGRAM_INPUT:
				cs->inputs[idx].refcount++;
				break;
			default: break;
			}
		}

		idx = fpi->DstReg.Index;
		if (fpi->DstReg.File == PROGRAM_TEMPORARY) {
			if (!(temps_used & (1<<idx))) {
				cs->temps[idx].reg = -1;
				cs->temps[idx].refcount = 1;
				temps_used |= (1 << idx);
			} else
				cs->temps[idx].refcount++;
		}
	}
	cs->temp_in_use = temps_used;
}

static void update_params(struct r300_fragment_program *rp)
{
	struct gl_fragment_program *mp = &rp->mesa_program;
	int i;

	/* Ask Mesa nicely to fill in ParameterValues for us */
	if (rp->param_nr)
		_mesa_load_state_parameters(rp->ctx, mp->Base.Parameters);

	for (i=0;i<rp->param_nr;i++)
		COPY_4V(rp->constant[rp->param[i].idx], rp->param[i].values);

	rp->params_uptodate = GL_TRUE;
}

void r300_translate_fragment_shader(struct r300_fragment_program *rp)
{
	struct r300_pfs_compile_state *cs = NULL;

	if (!rp->translated) {
		
		init_program(rp);
		cs = rp->cs;

		if (parse_program(rp) == GL_FALSE) {
			dump_program(rp);
			return;
		}
		
		/* Finish off */
		cs->v_pos = cs->s_pos = MAX2(cs->v_pos, cs->s_pos);
		rp->node[rp->cur_node].alu_end =
				cs->v_pos - rp->node[rp->cur_node].alu_offset - 1;
		if (rp->node[rp->cur_node].tex_end < 0)
			rp->node[rp->cur_node].tex_end = 0;
		rp->alu_offset = 0;
		rp->alu_end    = cs->v_pos - 1;
		rp->tex_offset = 0;
		rp->tex_end    = rp->tex.length ? rp->tex.length - 1 : 0;
		assert(rp->node[rp->cur_node].alu_end >= 0);
		assert(rp->alu_end >= 0);
	
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
		_mesa_print_program(&rp->mesa_program.Base);
	fflush(stdout);

	fprintf(stderr, "Hardware program\n");
	fprintf(stderr, "----------------\n");
	
	fprintf(stderr, "tex:\n");
	
	for(i=0;i<rp->tex.length;i++) {
		fprintf(stderr, "%08x\n", rp->tex.inst[i]);
	}
	
	for (i=0;i<(rp->cur_node+1);i++) {
		fprintf(stderr, "NODE %d: alu_offset: %d, tex_offset: %d, "\
			"alu_end: %d, tex_end: %d\n", i,
			rp->node[i].alu_offset,
			rp->node[i].tex_offset,
			rp->node[i].alu_end,
			rp->node[i].tex_end);
	}
	
	fprintf(stderr, "%08x\n",
		((rp->tex_end << 16) | (R300_PFS_TEXI_0 >> 2)));
	for (i=0;i<=rp->tex_end;i++)
		fprintf(stderr, "%08x\n", rp->tex.inst[i]);

	/* dump program in pretty_print_command_stream.tcl-readable format */
	fprintf(stderr, "%08x\n",
		((rp->alu_end << 16) | (R300_PFS_INSTR0_0 >> 2)));
	for (i=0;i<=rp->alu_end;i++)
		fprintf(stderr, "%08x\n", rp->alu.inst[i].inst0);

	fprintf(stderr, "%08x\n",
		((rp->alu_end << 16) | (R300_PFS_INSTR1_0 >> 2)));
	for (i=0;i<=rp->alu_end;i++)
		fprintf(stderr, "%08x\n", rp->alu.inst[i].inst1);

	fprintf(stderr, "%08x\n",
		((rp->alu_end << 16) | (R300_PFS_INSTR2_0 >> 2)));
	for (i=0;i<=rp->alu_end;i++)
		fprintf(stderr, "%08x\n", rp->alu.inst[i].inst2);

	fprintf(stderr, "%08x\n",
		((rp->alu_end << 16) | (R300_PFS_INSTR3_0 >> 2)));
	for (i=0;i<=rp->alu_end;i++)
		fprintf(stderr, "%08x\n", rp->alu.inst[i].inst3);

	fprintf(stderr, "00000000\n");
}
