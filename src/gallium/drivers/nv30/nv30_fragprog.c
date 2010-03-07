#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"

#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"

#include "nv30_context.h"

#define SWZ_X 0
#define SWZ_Y 1
#define SWZ_Z 2
#define SWZ_W 3
#define MASK_X 1
#define MASK_Y 2
#define MASK_Z 4
#define MASK_W 8
#define MASK_ALL (MASK_X|MASK_Y|MASK_Z|MASK_W)
#define DEF_SCALE NV30_FP_OP_DST_SCALE_1X
#define DEF_CTEST NV30_FP_OP_COND_TR
#include "nv30_shader.h"

#define swz(s,x,y,z,w) nv30_sr_swz((s), SWZ_##x, SWZ_##y, SWZ_##z, SWZ_##w)
#define neg(s) nv30_sr_neg((s))
#define abs(s) nv30_sr_abs((s))
#define scale(s,v) nv30_sr_scale((s), NV30_FP_OP_DST_SCALE_##v)

#define MAX_CONSTS 128
#define MAX_IMM 32
struct nv30_fpc {
	struct nv30_fragment_program *fp;

	uint attrib_map[PIPE_MAX_SHADER_INPUTS];

	int high_temp;
	int temp_temp_count;
	int num_regs;

	uint depth_id;
	uint colour_id;

	unsigned inst_offset;

	struct {
		int pipe;
		float vals[4];
	} consts[MAX_CONSTS];
	int nr_consts;

	struct nv30_sreg imm[MAX_IMM];
	unsigned nr_imm;
};

static INLINE struct nv30_sreg
temp(struct nv30_fpc *fpc)
{
	int idx;

	idx  = fpc->temp_temp_count++;
	idx += fpc->high_temp + 1;
	return nv30_sr(NV30SR_TEMP, idx);
}

static INLINE struct nv30_sreg
constant(struct nv30_fpc *fpc, int pipe, float vals[4])
{
	int idx;

	if (fpc->nr_consts == MAX_CONSTS)
		assert(0);
	idx = fpc->nr_consts++;

	fpc->consts[idx].pipe = pipe;
	if (pipe == -1)
		memcpy(fpc->consts[idx].vals, vals, 4 * sizeof(float));
	return nv30_sr(NV30SR_CONST, idx);
}

#define arith(cc,s,o,d,m,s0,s1,s2) \
	nv30_fp_arith((cc), (s), NV30_FP_OP_OPCODE_##o, \
			(d), (m), (s0), (s1), (s2))
#define tex(cc,s,o,u,d,m,s0,s1,s2) \
	nv30_fp_tex((cc), (s), NV30_FP_OP_OPCODE_##o, (u), \
		    (d), (m), (s0), none, none)

static void
grow_insns(struct nv30_fpc *fpc, int size)
{
	struct nv30_fragment_program *fp = fpc->fp;

	fp->insn_len += size;
	fp->insn = realloc(fp->insn, sizeof(uint32_t) * fp->insn_len);
}

static void
emit_src(struct nv30_fpc *fpc, int pos, struct nv30_sreg src)
{
	struct nv30_fragment_program *fp = fpc->fp;
	uint32_t *hw = &fp->insn[fpc->inst_offset];
	uint32_t sr = 0;

	switch (src.type) {
	case NV30SR_INPUT:
		sr |= (NV30_FP_REG_TYPE_INPUT << NV30_FP_REG_TYPE_SHIFT);
		hw[0] |= (src.index << NV30_FP_OP_INPUT_SRC_SHIFT);
		break;
	case NV30SR_OUTPUT:
		sr |= NV30_FP_REG_SRC_HALF;
		/* fall-through */
	case NV30SR_TEMP:
		sr |= (NV30_FP_REG_TYPE_TEMP << NV30_FP_REG_TYPE_SHIFT);
		sr |= (src.index << NV30_FP_REG_SRC_SHIFT);
		break;
	case NV30SR_CONST:
		grow_insns(fpc, 4);
		hw = &fp->insn[fpc->inst_offset];
		if (fpc->consts[src.index].pipe >= 0) {
			struct nv30_fragment_program_data *fpd;

			fp->consts = realloc(fp->consts, ++fp->nr_consts *
					     sizeof(*fpd));
			fpd = &fp->consts[fp->nr_consts - 1];
			fpd->offset = fpc->inst_offset + 4;
			fpd->index = fpc->consts[src.index].pipe;
			memset(&fp->insn[fpd->offset], 0, sizeof(uint32_t) * 4);
		} else {
			memcpy(&fp->insn[fpc->inst_offset + 4],
				fpc->consts[src.index].vals,
				sizeof(uint32_t) * 4);
		}

		sr |= (NV30_FP_REG_TYPE_CONST << NV30_FP_REG_TYPE_SHIFT);
		break;
	case NV30SR_NONE:
		sr |= (NV30_FP_REG_TYPE_INPUT << NV30_FP_REG_TYPE_SHIFT);
		break;
	default:
		assert(0);
	}

	if (src.negate)
		sr |= NV30_FP_REG_NEGATE;

	if (src.abs)
		hw[1] |= (1 << (29 + pos));

	sr |= ((src.swz[0] << NV30_FP_REG_SWZ_X_SHIFT) |
	       (src.swz[1] << NV30_FP_REG_SWZ_Y_SHIFT) |
	       (src.swz[2] << NV30_FP_REG_SWZ_Z_SHIFT) |
	       (src.swz[3] << NV30_FP_REG_SWZ_W_SHIFT));

	hw[pos + 1] |= sr;
}

static void
emit_dst(struct nv30_fpc *fpc, struct nv30_sreg dst)
{
	struct nv30_fragment_program *fp = fpc->fp;
	uint32_t *hw = &fp->insn[fpc->inst_offset];

	switch (dst.type) {
	case NV30SR_TEMP:
		if (fpc->num_regs < (dst.index + 1))
			fpc->num_regs = dst.index + 1;
		break;
	case NV30SR_OUTPUT:
		if (dst.index == 1) {
			fp->fp_control |= 0xe;
		} else {
			hw[0] |= NV30_FP_OP_OUT_REG_HALF;
		}
		break;
	case NV30SR_NONE:
		hw[0] |= (1 << 30);
		break;
	default:
		assert(0);
	}

	hw[0] |= (dst.index << NV30_FP_OP_OUT_REG_SHIFT);
}

static void
nv30_fp_arith(struct nv30_fpc *fpc, int sat, int op,
	      struct nv30_sreg dst, int mask,
	      struct nv30_sreg s0, struct nv30_sreg s1, struct nv30_sreg s2)
{
	struct nv30_fragment_program *fp = fpc->fp;
	uint32_t *hw;

	fpc->inst_offset = fp->insn_len;
	grow_insns(fpc, 4);
	hw = &fp->insn[fpc->inst_offset];
	memset(hw, 0, sizeof(uint32_t) * 4);

	if (op == NV30_FP_OP_OPCODE_KIL)
		fp->fp_control |= NV34TCL_FP_CONTROL_USES_KIL;
	hw[0] |= (op << NV30_FP_OP_OPCODE_SHIFT);
	hw[0] |= (mask << NV30_FP_OP_OUTMASK_SHIFT);
	hw[2] |= (dst.dst_scale << NV30_FP_OP_DST_SCALE_SHIFT);

	if (sat)
		hw[0] |= NV30_FP_OP_OUT_SAT;

	if (dst.cc_update)
		hw[0] |= NV30_FP_OP_COND_WRITE_ENABLE;
	hw[1] |= (dst.cc_test << NV30_FP_OP_COND_SHIFT);
	hw[1] |= ((dst.cc_swz[0] << NV30_FP_OP_COND_SWZ_X_SHIFT) |
		  (dst.cc_swz[1] << NV30_FP_OP_COND_SWZ_Y_SHIFT) |
		  (dst.cc_swz[2] << NV30_FP_OP_COND_SWZ_Z_SHIFT) |
		  (dst.cc_swz[3] << NV30_FP_OP_COND_SWZ_W_SHIFT));

	emit_dst(fpc, dst);
	emit_src(fpc, 0, s0);
	emit_src(fpc, 1, s1);
	emit_src(fpc, 2, s2);
}

static void
nv30_fp_tex(struct nv30_fpc *fpc, int sat, int op, int unit,
	    struct nv30_sreg dst, int mask,
	    struct nv30_sreg s0, struct nv30_sreg s1, struct nv30_sreg s2)
{
	struct nv30_fragment_program *fp = fpc->fp;

	nv30_fp_arith(fpc, sat, op, dst, mask, s0, s1, s2);

	fp->insn[fpc->inst_offset] |= (unit << NV30_FP_OP_TEX_UNIT_SHIFT);
	fp->samplers |= (1 << unit);
}

static INLINE struct nv30_sreg
tgsi_src(struct nv30_fpc *fpc, const struct tgsi_full_src_register *fsrc)
{
	struct nv30_sreg src;

	switch (fsrc->Register.File) {
	case TGSI_FILE_INPUT:
		src = nv30_sr(NV30SR_INPUT,
			      fpc->attrib_map[fsrc->Register.Index]);
		break;
	case TGSI_FILE_CONSTANT:
		src = constant(fpc, fsrc->Register.Index, NULL);
		break;
	case TGSI_FILE_IMMEDIATE:
		assert(fsrc->Register.Index < fpc->nr_imm);
		src = fpc->imm[fsrc->Register.Index];
		break;
	case TGSI_FILE_TEMPORARY:
		src = nv30_sr(NV30SR_TEMP, fsrc->Register.Index + 1);
		if (fpc->high_temp < src.index)
			fpc->high_temp = src.index;
		break;
	/* This is clearly insane, but gallium hands us shaders like this.
	 * Luckily fragprog results are just temp regs..
	 */
	case TGSI_FILE_OUTPUT:
		if (fsrc->Register.Index == fpc->colour_id)
			return nv30_sr(NV30SR_OUTPUT, 0);
		else
			return nv30_sr(NV30SR_OUTPUT, 1);
		break;
	default:
		NOUVEAU_ERR("bad src file\n");
		break;
	}

	src.abs = fsrc->Register.Absolute;
	src.negate = fsrc->Register.Negate;
	src.swz[0] = fsrc->Register.SwizzleX;
	src.swz[1] = fsrc->Register.SwizzleY;
	src.swz[2] = fsrc->Register.SwizzleZ;
	src.swz[3] = fsrc->Register.SwizzleW;
	return src;
}

static INLINE struct nv30_sreg
tgsi_dst(struct nv30_fpc *fpc, const struct tgsi_full_dst_register *fdst) {
	int idx;

	switch (fdst->Register.File) {
	case TGSI_FILE_OUTPUT:
		if (fdst->Register.Index == fpc->colour_id)
			return nv30_sr(NV30SR_OUTPUT, 0);
		else
			return nv30_sr(NV30SR_OUTPUT, 1);
		break;
	case TGSI_FILE_TEMPORARY:
		idx = fdst->Register.Index + 1;
		if (fpc->high_temp < idx)
			fpc->high_temp = idx;
		return nv30_sr(NV30SR_TEMP, idx);
	case TGSI_FILE_NULL:
		return nv30_sr(NV30SR_NONE, 0);
	default:
		NOUVEAU_ERR("bad dst file %d\n", fdst->Register.File);
		return nv30_sr(NV30SR_NONE, 0);
	}
}

static INLINE int
tgsi_mask(uint tgsi)
{
	int mask = 0;

	if (tgsi & TGSI_WRITEMASK_X) mask |= MASK_X;
	if (tgsi & TGSI_WRITEMASK_Y) mask |= MASK_Y;
	if (tgsi & TGSI_WRITEMASK_Z) mask |= MASK_Z;
	if (tgsi & TGSI_WRITEMASK_W) mask |= MASK_W;
	return mask;
}

static boolean
src_native_swz(struct nv30_fpc *fpc, const struct tgsi_full_src_register *fsrc,
	       struct nv30_sreg *src)
{
	const struct nv30_sreg none = nv30_sr(NV30SR_NONE, 0);
	struct nv30_sreg tgsi = tgsi_src(fpc, fsrc);
	uint mask = 0;
	uint c;

	for (c = 0; c < 4; c++) {
		switch (tgsi_util_get_full_src_register_swizzle(fsrc, c)) {
		case TGSI_SWIZZLE_X:
		case TGSI_SWIZZLE_Y:
		case TGSI_SWIZZLE_Z:
		case TGSI_SWIZZLE_W:
			mask |= (1 << c);
			break;
		default:
			assert(0);
		}
	}

	if (mask == MASK_ALL)
		return TRUE;

	*src = temp(fpc);

	if (mask)
		arith(fpc, 0, MOV, *src, mask, tgsi, none, none);

	return FALSE;
}

static boolean
nv30_fragprog_parse_instruction(struct nv30_fpc *fpc,
				const struct tgsi_full_instruction *finst)
{
	const struct nv30_sreg none = nv30_sr(NV30SR_NONE, 0);
	struct nv30_sreg src[3], dst, tmp;
	int mask, sat, unit = 0;
	int ai = -1, ci = -1;
	int i;

	if (finst->Instruction.Opcode == TGSI_OPCODE_END)
		return TRUE;

	fpc->temp_temp_count = 0;
	for (i = 0; i < finst->Instruction.NumSrcRegs; i++) {
		const struct tgsi_full_src_register *fsrc;

		fsrc = &finst->Src[i];
		if (fsrc->Register.File == TGSI_FILE_TEMPORARY) {
			src[i] = tgsi_src(fpc, fsrc);
		}
	}

	for (i = 0; i < finst->Instruction.NumSrcRegs; i++) {
		const struct tgsi_full_src_register *fsrc;

		fsrc = &finst->Src[i];

		switch (fsrc->Register.File) {
		case TGSI_FILE_INPUT:
		case TGSI_FILE_CONSTANT:
		case TGSI_FILE_TEMPORARY:
			if (!src_native_swz(fpc, fsrc, &src[i]))
				continue;
			break;
		default:
			break;
		}

		switch (fsrc->Register.File) {
		case TGSI_FILE_INPUT:
			if (ai == -1 || ai == fsrc->Register.Index) {
				ai = fsrc->Register.Index;
				src[i] = tgsi_src(fpc, fsrc);
			} else {
				NOUVEAU_MSG("extra src attr %d\n",
					 fsrc->Register.Index);
				src[i] = temp(fpc);
				arith(fpc, 0, MOV, src[i], MASK_ALL,
				      tgsi_src(fpc, fsrc), none, none);
			}
			break;
		case TGSI_FILE_CONSTANT:
		case TGSI_FILE_IMMEDIATE:
			if (ci == -1 || ci == fsrc->Register.Index) {
				ci = fsrc->Register.Index;
				src[i] = tgsi_src(fpc, fsrc);
			} else {
				src[i] = temp(fpc);
				arith(fpc, 0, MOV, src[i], MASK_ALL,
				      tgsi_src(fpc, fsrc), none, none);
			}
			break;
		case TGSI_FILE_TEMPORARY:
			/* handled above */
			break;
		case TGSI_FILE_SAMPLER:
			unit = fsrc->Register.Index;
			break;
		case TGSI_FILE_OUTPUT:
			break;
		default:
			NOUVEAU_ERR("bad src file\n");
			return FALSE;
		}
	}

	dst  = tgsi_dst(fpc, &finst->Dst[0]);
	mask = tgsi_mask(finst->Dst[0].Register.WriteMask);
	sat  = (finst->Instruction.Saturate == TGSI_SAT_ZERO_ONE);

	switch (finst->Instruction.Opcode) {
	case TGSI_OPCODE_ABS:
		arith(fpc, sat, MOV, dst, mask, abs(src[0]), none, none);
		break;
	case TGSI_OPCODE_ADD:
		arith(fpc, sat, ADD, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_CMP:
		tmp = nv30_sr(NV30SR_NONE, 0);
		tmp.cc_update = 1;
		arith(fpc, 0, MOV, tmp, 0xf, src[0], none, none);
		dst.cc_test = NV30_VP_INST_COND_GE;
		arith(fpc, sat, MOV, dst, mask, src[2], none, none);
		dst.cc_test = NV30_VP_INST_COND_LT;
		arith(fpc, sat, MOV, dst, mask, src[1], none, none);
		break;
	case TGSI_OPCODE_COS:
		arith(fpc, sat, COS, dst, mask, src[0], none, none);
		break;
	case TGSI_OPCODE_DP3:
		arith(fpc, sat, DP3, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_DP4:
		arith(fpc, sat, DP4, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_DPH:
		tmp = temp(fpc);
		arith(fpc, 0, DP3, tmp, MASK_X, src[0], src[1], none);
		arith(fpc, sat, ADD, dst, mask, swz(tmp, X, X, X, X),
		      swz(src[1], W, W, W, W), none);
		break;
	case TGSI_OPCODE_DST:
		arith(fpc, sat, DST, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_EX2:
		arith(fpc, sat, EX2, dst, mask, src[0], none, none);
		break;
	case TGSI_OPCODE_FLR:
		arith(fpc, sat, FLR, dst, mask, src[0], none, none);
		break;
	case TGSI_OPCODE_FRC:
		arith(fpc, sat, FRC, dst, mask, src[0], none, none);
		break;
	case TGSI_OPCODE_KILP:
		arith(fpc, 0, KIL, none, 0, none, none, none);
		break;
	case TGSI_OPCODE_KIL:
		dst = nv30_sr(NV30SR_NONE, 0);
		dst.cc_update = 1;
		arith(fpc, 0, MOV, dst, MASK_ALL, src[0], none, none);
		dst.cc_update = 0; dst.cc_test = NV30_FP_OP_COND_LT;
		arith(fpc, 0, KIL, dst, 0, none, none, none);
		break;
	case TGSI_OPCODE_LG2:
		arith(fpc, sat, LG2, dst, mask, src[0], none, none);
		break;
//	case TGSI_OPCODE_LIT:
	case TGSI_OPCODE_LRP:
		arith(fpc, sat, LRP, dst, mask, src[0], src[1], src[2]);
		break;
	case TGSI_OPCODE_MAD:
		arith(fpc, sat, MAD, dst, mask, src[0], src[1], src[2]);
		break;
	case TGSI_OPCODE_MAX:
		arith(fpc, sat, MAX, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_MIN:
		arith(fpc, sat, MIN, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_MOV:
		arith(fpc, sat, MOV, dst, mask, src[0], none, none);
		break;
	case TGSI_OPCODE_MUL:
		arith(fpc, sat, MUL, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_POW:
		arith(fpc, sat, POW, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_RCP:
		arith(fpc, sat, RCP, dst, mask, src[0], none, none);
		break;
	case TGSI_OPCODE_RET:
		assert(0);
		break;
	case TGSI_OPCODE_RFL:
		arith(fpc, 0, RFL, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_RSQ:
		arith(fpc, sat, RSQ, dst, mask, abs(swz(src[0], X, X, X, X)), none, none);
		break;
	case TGSI_OPCODE_SCS:
		/* avoid overwriting the source */
		if(src[0].swz[SWZ_X] != SWZ_X)
		{
			if (mask & MASK_X) {
				arith(fpc, sat, COS, dst, MASK_X,
				      swz(src[0], X, X, X, X), none, none);
			}
			if (mask & MASK_Y) {
				arith(fpc, sat, SIN, dst, MASK_Y,
				      swz(src[0], X, X, X, X), none, none);
			}
		}
		else
		{
			if (mask & MASK_Y) {
				arith(fpc, sat, SIN, dst, MASK_Y,
				      swz(src[0], X, X, X, X), none, none);
			}
			if (mask & MASK_X) {
				arith(fpc, sat, COS, dst, MASK_X,
				      swz(src[0], X, X, X, X), none, none);
			}
		}
		break;
	case TGSI_OPCODE_SIN:
		arith(fpc, sat, SIN, dst, mask, src[0], none, none);
		break;
	case TGSI_OPCODE_SGE:
		arith(fpc, sat, SGE, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_SGT:
		arith(fpc, sat, SGT, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_SLT:
		arith(fpc, sat, SLT, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_SUB:
		arith(fpc, sat, ADD, dst, mask, src[0], neg(src[1]), none);
		break;
	case TGSI_OPCODE_TEX:
		tex(fpc, sat, TEX, unit, dst, mask, src[0], none, none);
		break;
	case TGSI_OPCODE_TXB:
		tex(fpc, sat, TXB, unit, dst, mask, src[0], none, none);
		break;
	case TGSI_OPCODE_TXP:
		tex(fpc, sat, TXP, unit, dst, mask, src[0], none, none);
		break;
	case TGSI_OPCODE_XPD:
		tmp = temp(fpc);
		arith(fpc, 0, MUL, tmp, mask,
		      swz(src[0], Z, X, Y, Y), swz(src[1], Y, Z, X, X), none);
		arith(fpc, sat, MAD, dst, (mask & ~MASK_W),
		      swz(src[0], Y, Z, X, X), swz(src[1], Z, X, Y, Y),
		      neg(tmp));
		break;
	default:
		NOUVEAU_ERR("invalid opcode %d\n", finst->Instruction.Opcode);
		return FALSE;
	}

	return TRUE;
}

static boolean
nv30_fragprog_parse_decl_attrib(struct nv30_fpc *fpc,
				const struct tgsi_full_declaration *fdec)
{
	int hw;

	switch (fdec->Semantic.Name) {
	case TGSI_SEMANTIC_POSITION:
		hw = NV30_FP_OP_INPUT_SRC_POSITION;
		break;
	case TGSI_SEMANTIC_COLOR:
		if (fdec->Semantic.Index == 0) {
			hw = NV30_FP_OP_INPUT_SRC_COL0;
		} else
		if (fdec->Semantic.Index == 1) {
			hw = NV30_FP_OP_INPUT_SRC_COL1;
		} else {
			NOUVEAU_ERR("bad colour semantic index\n");
			return FALSE;
		}
		break;
	case TGSI_SEMANTIC_FOG:
		hw = NV30_FP_OP_INPUT_SRC_FOGC;
		break;
	case TGSI_SEMANTIC_GENERIC:
		if (fdec->Semantic.Index <= 7) {
			hw = NV30_FP_OP_INPUT_SRC_TC(fdec->Semantic.
						     Index);
		} else {
			NOUVEAU_ERR("bad generic semantic index\n");
			return FALSE;
		}
		break;
	default:
		NOUVEAU_ERR("bad input semantic\n");
		return FALSE;
	}

	fpc->attrib_map[fdec->Range.First] = hw;
	return TRUE;
}

static boolean
nv30_fragprog_parse_decl_output(struct nv30_fpc *fpc,
				const struct tgsi_full_declaration *fdec)
{
	switch (fdec->Semantic.Name) {
	case TGSI_SEMANTIC_POSITION:
		fpc->depth_id = fdec->Range.First;
		break;
	case TGSI_SEMANTIC_COLOR:
		fpc->colour_id = fdec->Range.First;
		break;
	default:
		NOUVEAU_ERR("bad output semantic\n");
		return FALSE;
	}

	return TRUE;
}

static boolean
nv30_fragprog_prepare(struct nv30_fpc *fpc)
{
	struct tgsi_parse_context p;
	/*int high_temp = -1, i;*/

	tgsi_parse_init(&p, fpc->fp->pipe.tokens);
	while (!tgsi_parse_end_of_tokens(&p)) {
		const union tgsi_full_token *tok = &p.FullToken;

		tgsi_parse_token(&p);
		switch(tok->Token.Type) {
		case TGSI_TOKEN_TYPE_DECLARATION:
		{
			const struct tgsi_full_declaration *fdec;
			fdec = &p.FullToken.FullDeclaration;
			switch (fdec->Declaration.File) {
			case TGSI_FILE_INPUT:
				if (!nv30_fragprog_parse_decl_attrib(fpc, fdec))
					goto out_err;
				break;
			case TGSI_FILE_OUTPUT:
				if (!nv30_fragprog_parse_decl_output(fpc, fdec))
					goto out_err;
				break;
			/*case TGSI_FILE_TEMPORARY:
				if (fdec->Range.Last > high_temp) {
					high_temp =
						fdec->Range.Last;
				}
				break;*/
			default:
				break;
			}
		}
			break;
		case TGSI_TOKEN_TYPE_IMMEDIATE:
		{
			struct tgsi_full_immediate *imm;
			float vals[4];

			imm = &p.FullToken.FullImmediate;
			assert(imm->Immediate.DataType == TGSI_IMM_FLOAT32);
			assert(fpc->nr_imm < MAX_IMM);

			vals[0] = imm->u[0].Float;
			vals[1] = imm->u[1].Float;
			vals[2] = imm->u[2].Float;
			vals[3] = imm->u[3].Float;
			fpc->imm[fpc->nr_imm++] = constant(fpc, -1, vals);
		}
			break;
		default:
			break;
		}
	}
	tgsi_parse_free(&p);

	/*if (++high_temp) {
		fpc->r_temp = CALLOC(high_temp, sizeof(struct nv30_sreg));
		for (i = 0; i < high_temp; i++)
			fpc->r_temp[i] = temp(fpc);
		fpc->r_temps_discard = 0;
	}*/

	return TRUE;

out_err:
	/*if (fpc->r_temp)
		FREE(fpc->r_temp);*/
	tgsi_parse_free(&p);
	return FALSE;
}

static void
nv30_fragprog_translate(struct nv30_context *nv30,
			struct nv30_fragment_program *fp)
{
	struct tgsi_parse_context parse;
	struct nv30_fpc *fpc = NULL;

	tgsi_dump(fp->pipe.tokens,0);

	fpc = CALLOC(1, sizeof(struct nv30_fpc));
	if (!fpc)
		return;
	fpc->fp = fp;
	fpc->high_temp = -1;
	fpc->num_regs = 2;

	if (!nv30_fragprog_prepare(fpc)) {
		FREE(fpc);
		return;
	}

	tgsi_parse_init(&parse, fp->pipe.tokens);

	while (!tgsi_parse_end_of_tokens(&parse)) {
		tgsi_parse_token(&parse);

		switch (parse.FullToken.Token.Type) {
		case TGSI_TOKEN_TYPE_INSTRUCTION:
		{
			const struct tgsi_full_instruction *finst;

			finst = &parse.FullToken.FullInstruction;
			if (!nv30_fragprog_parse_instruction(fpc, finst))
				goto out_err;
		}
			break;
		default:
			break;
		}
	}

	fp->fp_control |= (fpc->num_regs-1)/2;
	fp->fp_reg_control = (1<<16)|0x4;

	/* Terminate final instruction */
	fp->insn[fpc->inst_offset] |= 0x00000001;

	/* Append NOP + END instruction, may or may not be necessary. */
	fpc->inst_offset = fp->insn_len;
	grow_insns(fpc, 4);
	fp->insn[fpc->inst_offset + 0] = 0x00000001;
	fp->insn[fpc->inst_offset + 1] = 0x00000000;
	fp->insn[fpc->inst_offset + 2] = 0x00000000;
	fp->insn[fpc->inst_offset + 3] = 0x00000000;

	fp->translated = TRUE;
	fp->on_hw = FALSE;
out_err:
	tgsi_parse_free(&parse);
	FREE(fpc);
}

static void
nv30_fragprog_upload(struct nv30_context *nv30,
		     struct nv30_fragment_program *fp)
{
	struct pipe_screen *pscreen = nv30->pipe.screen;
	const uint32_t le = 1;
	uint32_t *map;
	int i;

	map = pipe_buffer_map(pscreen, fp->buffer, PIPE_BUFFER_USAGE_CPU_WRITE);

#if 0
	for (i = 0; i < fp->insn_len; i++) {
		fflush(stdout); fflush(stderr);
		NOUVEAU_ERR("%d 0x%08x\n", i, fp->insn[i]);
		fflush(stdout); fflush(stderr);
	}
#endif

	if ((*(const uint8_t *)&le)) {
		for (i = 0; i < fp->insn_len; i++) {
			map[i] = fp->insn[i];
		}
	} else {
		/* Weird swapping for big-endian chips */
		for (i = 0; i < fp->insn_len; i++) {
			map[i] = ((fp->insn[i] & 0xffff) << 16) |
				  ((fp->insn[i] >> 16) & 0xffff);
		}
	}

	pipe_buffer_unmap(pscreen, fp->buffer);
}

static boolean
nv30_fragprog_validate(struct nv30_context *nv30)
{
	struct nv30_fragment_program *fp = nv30->fragprog;
	struct pipe_buffer *constbuf =
		nv30->constbuf[PIPE_SHADER_FRAGMENT];
	struct pipe_screen *pscreen = nv30->pipe.screen;
	struct nouveau_stateobj *so;
	boolean new_consts = FALSE;
	int i;

	if (fp->translated)
		goto update_constants;

	/*nv30->fallback_swrast &= ~NV30_NEW_FRAGPROG;*/
	nv30_fragprog_translate(nv30, fp);
	if (!fp->translated) {
		/*nv30->fallback_swrast |= NV30_NEW_FRAGPROG;*/
		return FALSE;
	}

	fp->buffer = pscreen->buffer_create(pscreen, 0x100, 0, fp->insn_len * 4);
	nv30_fragprog_upload(nv30, fp);

	so = so_new(4, 4, 1);
	so_method(so, nv30->screen->rankine, NV34TCL_FP_ACTIVE_PROGRAM, 1);
	so_reloc (so, nouveau_bo(fp->buffer), 0, NOUVEAU_BO_VRAM |
		      NOUVEAU_BO_GART | NOUVEAU_BO_RD | NOUVEAU_BO_LOW |
		      NOUVEAU_BO_OR, NV34TCL_FP_ACTIVE_PROGRAM_DMA0,
		      NV34TCL_FP_ACTIVE_PROGRAM_DMA1);
	so_method(so, nv30->screen->rankine, NV34TCL_FP_CONTROL, 1);
	so_data  (so, fp->fp_control);
	so_method(so, nv30->screen->rankine, NV34TCL_FP_REG_CONTROL, 1);
	so_data  (so, fp->fp_reg_control);
	so_method(so, nv30->screen->rankine, NV34TCL_TX_UNITS_ENABLE, 1);
	so_data  (so, fp->samplers);
	so_ref(so, &fp->so);
	so_ref(NULL, &so);

update_constants:
	if (fp->nr_consts) {
		float *map;

		map = pipe_buffer_map(pscreen, constbuf,
				      PIPE_BUFFER_USAGE_CPU_READ);
		for (i = 0; i < fp->nr_consts; i++) {
			struct nv30_fragment_program_data *fpd = &fp->consts[i];
			uint32_t *p = &fp->insn[fpd->offset];
			uint32_t *cb = (uint32_t *)&map[fpd->index * 4];

			if (!memcmp(p, cb, 4 * sizeof(float)))
				continue;
			memcpy(p, cb, 4 * sizeof(float));
			new_consts = TRUE;
		}
		pipe_buffer_unmap(pscreen, constbuf);

		if (new_consts)
			nv30_fragprog_upload(nv30, fp);
	}

	if (new_consts || fp->so != nv30->state.hw[NV30_STATE_FRAGPROG]) {
		so_ref(fp->so, &nv30->state.hw[NV30_STATE_FRAGPROG]);
		return TRUE;
	}

	return FALSE;
}

void
nv30_fragprog_destroy(struct nv30_context *nv30,
		      struct nv30_fragment_program *fp)
{
	if (fp->buffer)
		pipe_buffer_reference(&fp->buffer, NULL);

	if (fp->so)
		so_ref(NULL, &fp->so);

	if (fp->insn_len)
		FREE(fp->insn);
}

struct nv30_state_entry nv30_state_fragprog = {
	.validate = nv30_fragprog_validate,
	.dirty = {
		.pipe = NV30_NEW_FRAGPROG,
		.hw = NV30_STATE_FRAGPROG
	}
};
