#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"

#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"

#include "nv40_context.h"

#define SWZ_X 0
#define SWZ_Y 1
#define SWZ_Z 2
#define SWZ_W 3
#define MASK_X 1
#define MASK_Y 2
#define MASK_Z 4
#define MASK_W 8
#define MASK_ALL (MASK_X|MASK_Y|MASK_Z|MASK_W)
#define DEF_SCALE NV40_FP_OP_DST_SCALE_1X
#define DEF_CTEST NV40_FP_OP_COND_TR
#include "nv40_shader.h"

#define swz(s,x,y,z,w) nv40_sr_swz((s), SWZ_##x, SWZ_##y, SWZ_##z, SWZ_##w)
#define neg(s) nv40_sr_neg((s))
#define abs(s) nv40_sr_abs((s))
#define scale(s,v) nv40_sr_scale((s), NV40_FP_OP_DST_SCALE_##v)

#define MAX_CONSTS 128
#define MAX_IMM 32
struct nv40_fpc {
	struct nv40_fragment_program *fp;

	uint attrib_map[PIPE_MAX_SHADER_INPUTS];

	unsigned r_temps;
	unsigned r_temps_discard;
	struct nv40_sreg r_result[PIPE_MAX_SHADER_OUTPUTS];
	struct nv40_sreg *r_temp;

	int num_regs;

	unsigned inst_offset;
	unsigned have_const;

	struct {
		int pipe;
		float vals[4];
	} consts[MAX_CONSTS];
	int nr_consts;

	struct nv40_sreg imm[MAX_IMM];
	unsigned nr_imm;
};

static INLINE struct nv40_sreg
temp(struct nv40_fpc *fpc)
{
	int idx = ffs(~fpc->r_temps) - 1;

	if (idx < 0) {
		NOUVEAU_ERR("out of temps!!\n");
		assert(0);
		return nv40_sr(NV40SR_TEMP, 0);
	}

	fpc->r_temps |= (1 << idx);
	fpc->r_temps_discard |= (1 << idx);
	return nv40_sr(NV40SR_TEMP, idx);
}

static INLINE void
release_temps(struct nv40_fpc *fpc)
{
	fpc->r_temps &= ~fpc->r_temps_discard;
	fpc->r_temps_discard = 0;
}

static INLINE struct nv40_sreg
constant(struct nv40_fpc *fpc, int pipe, float vals[4])
{
	int idx;

	if (fpc->nr_consts == MAX_CONSTS)
		assert(0);
	idx = fpc->nr_consts++;

	fpc->consts[idx].pipe = pipe;
	if (pipe == -1)
		memcpy(fpc->consts[idx].vals, vals, 4 * sizeof(float));
	return nv40_sr(NV40SR_CONST, idx);
}

#define arith(cc,s,o,d,m,s0,s1,s2) \
	nv40_fp_arith((cc), (s), NV40_FP_OP_OPCODE_##o, \
			(d), (m), (s0), (s1), (s2))
#define tex(cc,s,o,u,d,m,s0,s1,s2) \
	nv40_fp_tex((cc), (s), NV40_FP_OP_OPCODE_##o, (u), \
		    (d), (m), (s0), none, none)

static void
grow_insns(struct nv40_fpc *fpc, int size)
{
	struct nv40_fragment_program *fp = fpc->fp;

	fp->insn_len += size;
	fp->insn = realloc(fp->insn, sizeof(uint32_t) * fp->insn_len);
}

static void
emit_src(struct nv40_fpc *fpc, int pos, struct nv40_sreg src)
{
	struct nv40_fragment_program *fp = fpc->fp;
	uint32_t *hw = &fp->insn[fpc->inst_offset];
	uint32_t sr = 0;

	switch (src.type) {
	case NV40SR_INPUT:
		sr |= (NV40_FP_REG_TYPE_INPUT << NV40_FP_REG_TYPE_SHIFT);
		hw[0] |= (src.index << NV40_FP_OP_INPUT_SRC_SHIFT);
		break;
	case NV40SR_OUTPUT:
		sr |= NV40_FP_REG_SRC_HALF;
		/* fall-through */
	case NV40SR_TEMP:
		sr |= (NV40_FP_REG_TYPE_TEMP << NV40_FP_REG_TYPE_SHIFT);
		sr |= (src.index << NV40_FP_REG_SRC_SHIFT);
		break;
	case NV40SR_CONST:
		if (!fpc->have_const) {
			grow_insns(fpc, 4);
			fpc->have_const = 1;
		}

		hw = &fp->insn[fpc->inst_offset];
		if (fpc->consts[src.index].pipe >= 0) {
			struct nv40_fragment_program_data *fpd;

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

		sr |= (NV40_FP_REG_TYPE_CONST << NV40_FP_REG_TYPE_SHIFT);
		break;
	case NV40SR_NONE:
		sr |= (NV40_FP_REG_TYPE_INPUT << NV40_FP_REG_TYPE_SHIFT);
		break;
	default:
		assert(0);
	}

	if (src.negate)
		sr |= NV40_FP_REG_NEGATE;

	if (src.abs)
		hw[1] |= (1 << (29 + pos));

	sr |= ((src.swz[0] << NV40_FP_REG_SWZ_X_SHIFT) |
	       (src.swz[1] << NV40_FP_REG_SWZ_Y_SHIFT) |
	       (src.swz[2] << NV40_FP_REG_SWZ_Z_SHIFT) |
	       (src.swz[3] << NV40_FP_REG_SWZ_W_SHIFT));

	hw[pos + 1] |= sr;
}

static void
emit_dst(struct nv40_fpc *fpc, struct nv40_sreg dst)
{
	struct nv40_fragment_program *fp = fpc->fp;
	uint32_t *hw = &fp->insn[fpc->inst_offset];

	switch (dst.type) {
	case NV40SR_TEMP:
		if (fpc->num_regs < (dst.index + 1))
			fpc->num_regs = dst.index + 1;
		break;
	case NV40SR_OUTPUT:
		if (dst.index == 1) {
			fp->fp_control |= 0xe;
		} else {
			hw[0] |= NV40_FP_OP_OUT_REG_HALF;
		}
		break;
	case NV40SR_NONE:
		hw[0] |= (1 << 30);
		break;
	default:
		assert(0);
	}

	hw[0] |= (dst.index << NV40_FP_OP_OUT_REG_SHIFT);
}

static void
nv40_fp_arith(struct nv40_fpc *fpc, int sat, int op,
	      struct nv40_sreg dst, int mask,
	      struct nv40_sreg s0, struct nv40_sreg s1, struct nv40_sreg s2)
{
	struct nv40_fragment_program *fp = fpc->fp;
	uint32_t *hw;

	fpc->inst_offset = fp->insn_len;
	fpc->have_const = 0;
	grow_insns(fpc, 4);
	hw = &fp->insn[fpc->inst_offset];
	memset(hw, 0, sizeof(uint32_t) * 4);

	if (op == NV40_FP_OP_OPCODE_KIL)
		fp->fp_control |= NV40TCL_FP_CONTROL_KIL;
	hw[0] |= (op << NV40_FP_OP_OPCODE_SHIFT);
	hw[0] |= (mask << NV40_FP_OP_OUTMASK_SHIFT);
	hw[2] |= (dst.dst_scale << NV40_FP_OP_DST_SCALE_SHIFT);

	if (sat)
		hw[0] |= NV40_FP_OP_OUT_SAT;

	if (dst.cc_update)
		hw[0] |= NV40_FP_OP_COND_WRITE_ENABLE;
	hw[1] |= (dst.cc_test << NV40_FP_OP_COND_SHIFT);
	hw[1] |= ((dst.cc_swz[0] << NV40_FP_OP_COND_SWZ_X_SHIFT) |
		  (dst.cc_swz[1] << NV40_FP_OP_COND_SWZ_Y_SHIFT) |
		  (dst.cc_swz[2] << NV40_FP_OP_COND_SWZ_Z_SHIFT) |
		  (dst.cc_swz[3] << NV40_FP_OP_COND_SWZ_W_SHIFT));

	emit_dst(fpc, dst);
	emit_src(fpc, 0, s0);
	emit_src(fpc, 1, s1);
	emit_src(fpc, 2, s2);
}

static void
nv40_fp_tex(struct nv40_fpc *fpc, int sat, int op, int unit,
	    struct nv40_sreg dst, int mask,
	    struct nv40_sreg s0, struct nv40_sreg s1, struct nv40_sreg s2)
{
	struct nv40_fragment_program *fp = fpc->fp;

	nv40_fp_arith(fpc, sat, op, dst, mask, s0, s1, s2);

	fp->insn[fpc->inst_offset] |= (unit << NV40_FP_OP_TEX_UNIT_SHIFT);
	fp->samplers |= (1 << unit);
}

static INLINE struct nv40_sreg
tgsi_src(struct nv40_fpc *fpc, const struct tgsi_full_src_register *fsrc)
{
	struct nv40_sreg src;

	switch (fsrc->Register.File) {
	case TGSI_FILE_INPUT:
		src = nv40_sr(NV40SR_INPUT,
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
		src = fpc->r_temp[fsrc->Register.Index];
		break;
	/* NV40 fragprog result regs are just temps, so this is simple */
	case TGSI_FILE_OUTPUT:
		src = fpc->r_result[fsrc->Register.Index];
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

static INLINE struct nv40_sreg
tgsi_dst(struct nv40_fpc *fpc, const struct tgsi_full_dst_register *fdst) {
	switch (fdst->Register.File) {
	case TGSI_FILE_OUTPUT:
		return fpc->r_result[fdst->Register.Index];
	case TGSI_FILE_TEMPORARY:
		return fpc->r_temp[fdst->Register.Index];
	case TGSI_FILE_NULL:
		return nv40_sr(NV40SR_NONE, 0);
	default:
		NOUVEAU_ERR("bad dst file %d\n", fdst->Register.File);
		return nv40_sr(NV40SR_NONE, 0);
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
src_native_swz(struct nv40_fpc *fpc, const struct tgsi_full_src_register *fsrc,
	       struct nv40_sreg *src)
{
	const struct nv40_sreg none = nv40_sr(NV40SR_NONE, 0);
	struct nv40_sreg tgsi = tgsi_src(fpc, fsrc);
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
nv40_fragprog_parse_instruction(struct nv40_fpc *fpc,
				const struct tgsi_full_instruction *finst)
{
	const struct nv40_sreg none = nv40_sr(NV40SR_NONE, 0);
	struct nv40_sreg src[3], dst, tmp;
	int mask, sat, unit;
	int ai = -1, ci = -1, ii = -1;
	int i;

	if (finst->Instruction.Opcode == TGSI_OPCODE_END)
		return TRUE;

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
				src[i] = temp(fpc);
				arith(fpc, 0, MOV, src[i], MASK_ALL,
				      tgsi_src(fpc, fsrc), none, none);
			}
			break;
		case TGSI_FILE_CONSTANT:
			if ((ci == -1 && ii == -1) ||
			    ci == fsrc->Register.Index) {
				ci = fsrc->Register.Index;
				src[i] = tgsi_src(fpc, fsrc);
			} else {
				src[i] = temp(fpc);
				arith(fpc, 0, MOV, src[i], MASK_ALL,
				      tgsi_src(fpc, fsrc), none, none);
			}
			break;
		case TGSI_FILE_IMMEDIATE:
			if ((ci == -1 && ii == -1) ||
			    ii == fsrc->Register.Index) {
				ii = fsrc->Register.Index;
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
		tmp = nv40_sr(NV40SR_NONE, 0);
		tmp.cc_update = 1;
		arith(fpc, 0, MOV, tmp, 0xf, src[0], none, none);
		dst.cc_test = NV40_VP_INST_COND_GE;
		arith(fpc, sat, MOV, dst, mask, src[2], none, none);
		dst.cc_test = NV40_VP_INST_COND_LT;
		arith(fpc, sat, MOV, dst, mask, src[1], none, none);
		break;
	case TGSI_OPCODE_COS:
		arith(fpc, sat, COS, dst, mask, src[0], none, none);
		break;
	case TGSI_OPCODE_DDX:
		if (mask & (MASK_Z | MASK_W)) {
			tmp = temp(fpc);
			arith(fpc, sat, DDX, tmp, MASK_X | MASK_Y,
			      swz(src[0], Z, W, Z, W), none, none);
			arith(fpc, 0, MOV, tmp, MASK_Z | MASK_W,
			      swz(tmp, X, Y, X, Y), none, none);
			arith(fpc, sat, DDX, tmp, MASK_X | MASK_Y, src[0],
			      none, none);
			arith(fpc, 0, MOV, dst, mask, tmp, none, none);
		} else {
			arith(fpc, sat, DDX, dst, mask, src[0], none, none);
		}
		break;
	case TGSI_OPCODE_DDY:
		if (mask & (MASK_Z | MASK_W)) {
			tmp = temp(fpc);
			arith(fpc, sat, DDY, tmp, MASK_X | MASK_Y,
			      swz(src[0], Z, W, Z, W), none, none);
			arith(fpc, 0, MOV, tmp, MASK_Z | MASK_W,
			      swz(tmp, X, Y, X, Y), none, none);
			arith(fpc, sat, DDY, tmp, MASK_X | MASK_Y, src[0],
			      none, none);
			arith(fpc, 0, MOV, dst, mask, tmp, none, none);
		} else {
			arith(fpc, sat, DDY, dst, mask, src[0], none, none);
		}
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
		dst = nv40_sr(NV40SR_NONE, 0);
		dst.cc_update = 1;
		arith(fpc, 0, MOV, dst, MASK_ALL, src[0], none, none);
		dst.cc_update = 0; dst.cc_test = NV40_FP_OP_COND_LT;
		arith(fpc, 0, KIL, dst, 0, none, none, none);
		break;
	case TGSI_OPCODE_LG2:
		arith(fpc, sat, LG2, dst, mask, src[0], none, none);
		break;
//	case TGSI_OPCODE_LIT:
	case TGSI_OPCODE_LRP:
		tmp = temp(fpc);
		arith(fpc, 0, MAD, tmp, mask, neg(src[0]), src[2], src[2]);
		arith(fpc, sat, MAD, dst, mask, src[0], src[1], tmp);
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
		tmp = temp(fpc);
		arith(fpc, 0, LG2, tmp, MASK_X,
		      swz(src[0], X, X, X, X), none, none);
		arith(fpc, 0, MUL, tmp, MASK_X, swz(tmp, X, X, X, X),
		      swz(src[1], X, X, X, X), none);
		arith(fpc, sat, EX2, dst, mask,
		      swz(tmp, X, X, X, X), none, none);
		break;
	case TGSI_OPCODE_RCP:
		arith(fpc, sat, RCP, dst, mask, src[0], none, none);
		break;
	case TGSI_OPCODE_RET:
		assert(0);
		break;
	case TGSI_OPCODE_RFL:
		tmp = temp(fpc);
		arith(fpc, 0, DP3, tmp, MASK_X, src[0], src[0], none);
		arith(fpc, 0, DP3, tmp, MASK_Y, src[0], src[1], none);
		arith(fpc, 0, DIV, scale(tmp, 2X), MASK_Z,
		      swz(tmp, Y, Y, Y, Y), swz(tmp, X, X, X, X), none);
		arith(fpc, sat, MAD, dst, mask,
		      swz(tmp, Z, Z, Z, Z), src[0], neg(src[1]));
		break;
	case TGSI_OPCODE_RSQ:
		tmp = temp(fpc);
		arith(fpc, 0, LG2, scale(tmp, INV_2X), MASK_X,
		      abs(swz(src[0], X, X, X, X)), none, none);
		arith(fpc, sat, EX2, dst, mask,
		      neg(swz(tmp, X, X, X, X)), none, none);
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
	case TGSI_OPCODE_SEQ:
		arith(fpc, sat, SEQ, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_SFL:
		arith(fpc, sat, SFL, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_SGE:
		arith(fpc, sat, SGE, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_SGT:
		arith(fpc, sat, SGT, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_SIN:
		arith(fpc, sat, SIN, dst, mask, src[0], none, none);
		break;
	case TGSI_OPCODE_SLE:
		arith(fpc, sat, SLE, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_SLT:
		arith(fpc, sat, SLT, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_SNE:
		arith(fpc, sat, SNE, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_STR:
		arith(fpc, sat, STR, dst, mask, src[0], src[1], none);
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

	release_temps(fpc);
	return TRUE;
}

static boolean
nv40_fragprog_parse_decl_attrib(struct nv40_fpc *fpc,
				const struct tgsi_full_declaration *fdec)
{
	int hw;

	switch (fdec->Semantic.Name) {
	case TGSI_SEMANTIC_POSITION:
		hw = NV40_FP_OP_INPUT_SRC_POSITION;
		break;
	case TGSI_SEMANTIC_COLOR:
		if (fdec->Semantic.Index == 0) {
			hw = NV40_FP_OP_INPUT_SRC_COL0;
		} else
		if (fdec->Semantic.Index == 1) {
			hw = NV40_FP_OP_INPUT_SRC_COL1;
		} else {
			NOUVEAU_ERR("bad colour semantic index\n");
			return FALSE;
		}
		break;
	case TGSI_SEMANTIC_FOG:
		hw = NV40_FP_OP_INPUT_SRC_FOGC;
		break;
	case TGSI_SEMANTIC_GENERIC:
		if (fdec->Semantic.Index <= 7) {
			hw = NV40_FP_OP_INPUT_SRC_TC(fdec->Semantic.
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
nv40_fragprog_parse_decl_output(struct nv40_fpc *fpc,
				const struct tgsi_full_declaration *fdec)
{
	unsigned idx = fdec->Range.First;
	unsigned hw;

	switch (fdec->Semantic.Name) {
	case TGSI_SEMANTIC_POSITION:
		hw = 1;
		break;
	case TGSI_SEMANTIC_COLOR:
		switch (fdec->Semantic.Index) {
		case 0: hw = 0; break;
		case 1: hw = 2; break;
		case 2: hw = 3; break;
		case 3: hw = 4; break;
		default:
			NOUVEAU_ERR("bad rcol index\n");
			return FALSE;
		}
		break;
	default:
		NOUVEAU_ERR("bad output semantic\n");
		return FALSE;
	}

	fpc->r_result[idx] = nv40_sr(NV40SR_OUTPUT, hw);
	fpc->r_temps |= (1 << hw);
	return TRUE;
}

static boolean
nv40_fragprog_prepare(struct nv40_fpc *fpc)
{
	struct tgsi_parse_context p;
	int high_temp = -1, i;

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
				if (!nv40_fragprog_parse_decl_attrib(fpc, fdec))
					goto out_err;
				break;
			case TGSI_FILE_OUTPUT:
				if (!nv40_fragprog_parse_decl_output(fpc, fdec))
					goto out_err;
				break;
			case TGSI_FILE_TEMPORARY:
				if (fdec->Range.Last > high_temp) {
					high_temp =
						fdec->Range.Last;
				}
				break;
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

	if (++high_temp) {
		fpc->r_temp = CALLOC(high_temp, sizeof(struct nv40_sreg));
		for (i = 0; i < high_temp; i++)
			fpc->r_temp[i] = temp(fpc);
		fpc->r_temps_discard = 0;
	}

	return TRUE;

out_err:
	if (fpc->r_temp)
		FREE(fpc->r_temp);
	tgsi_parse_free(&p);
	return FALSE;
}

static void
nv40_fragprog_translate(struct nv40_context *nv40,
			struct nv40_fragment_program *fp)
{
	struct tgsi_parse_context parse;
	struct nv40_fpc *fpc = NULL;

	fpc = CALLOC(1, sizeof(struct nv40_fpc));
	if (!fpc)
		return;
	fpc->fp = fp;
	fpc->num_regs = 2;

	if (!nv40_fragprog_prepare(fpc)) {
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
			if (!nv40_fragprog_parse_instruction(fpc, finst))
				goto out_err;
		}
			break;
		default:
			break;
		}
	}

	fp->fp_control |= fpc->num_regs << NV40TCL_FP_CONTROL_TEMP_COUNT_SHIFT;

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
out_err:
	tgsi_parse_free(&parse);
	if (fpc->r_temp)
		FREE(fpc->r_temp);
	FREE(fpc);
}

static void
nv40_fragprog_upload(struct nv40_context *nv40,
		     struct nv40_fragment_program *fp)
{
	struct pipe_screen *pscreen = nv40->pipe.screen;
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
nv40_fragprog_validate(struct nv40_context *nv40)
{
	struct nv40_fragment_program *fp = nv40->fragprog;
	struct pipe_buffer *constbuf =
		nv40->constbuf[PIPE_SHADER_FRAGMENT];
	struct pipe_screen *pscreen = nv40->pipe.screen;
	struct nouveau_stateobj *so;
	boolean new_consts = FALSE;
	int i;

	if (fp->translated)
		goto update_constants;

	nv40->fallback_swrast &= ~NV40_NEW_FRAGPROG;
	nv40_fragprog_translate(nv40, fp);
	if (!fp->translated) {
		nv40->fallback_swrast |= NV40_NEW_FRAGPROG;
		return FALSE;
	}

	fp->buffer = pscreen->buffer_create(pscreen, 0x100, 0, fp->insn_len * 4);
	nv40_fragprog_upload(nv40, fp);

	so = so_new(2, 2, 1);
	so_method(so, nv40->screen->curie, NV40TCL_FP_ADDRESS, 1);
	so_reloc (so, nouveau_bo(fp->buffer), 0, NOUVEAU_BO_VRAM |
		      NOUVEAU_BO_GART | NOUVEAU_BO_RD | NOUVEAU_BO_LOW |
		      NOUVEAU_BO_OR, NV40TCL_FP_ADDRESS_DMA0,
		      NV40TCL_FP_ADDRESS_DMA1);
	so_method(so, nv40->screen->curie, NV40TCL_FP_CONTROL, 1);
	so_data  (so, fp->fp_control);
	so_ref(so, &fp->so);
	so_ref(NULL, &so);

update_constants:
	if (fp->nr_consts) {
		float *map;

		map = pipe_buffer_map(pscreen, constbuf,
				      PIPE_BUFFER_USAGE_CPU_READ);
		for (i = 0; i < fp->nr_consts; i++) {
			struct nv40_fragment_program_data *fpd = &fp->consts[i];
			uint32_t *p = &fp->insn[fpd->offset];
			uint32_t *cb = (uint32_t *)&map[fpd->index * 4];

			if (!memcmp(p, cb, 4 * sizeof(float)))
				continue;
			memcpy(p, cb, 4 * sizeof(float));
			new_consts = TRUE;
		}
		pipe_buffer_unmap(pscreen, constbuf);

		if (new_consts)
			nv40_fragprog_upload(nv40, fp);
	}

	if (new_consts || fp->so != nv40->state.hw[NV40_STATE_FRAGPROG]) {
		so_ref(fp->so, &nv40->state.hw[NV40_STATE_FRAGPROG]);
		return TRUE;
	}

	return FALSE;
}

void
nv40_fragprog_destroy(struct nv40_context *nv40,
		      struct nv40_fragment_program *fp)
{
	if (fp->buffer)
		pipe_buffer_reference(&fp->buffer, NULL);

	if (fp->so)
		so_ref(NULL, &fp->so);

	if (fp->insn_len)
		FREE(fp->insn);
}

struct nv40_state_entry nv40_state_fragprog = {
	.validate = nv40_fragprog_validate,
	.dirty = {
		.pipe = NV40_NEW_FRAGPROG,
		.hw = NV40_STATE_FRAGPROG
	}
};

