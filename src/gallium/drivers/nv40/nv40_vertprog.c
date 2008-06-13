#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"

#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"

#include "nv40_context.h"
#include "nv40_state.h"

/* TODO (at least...):
 *  1. Indexed consts  + ARL
 *  3. NV_vp11, NV_vp2, NV_vp3 features
 *       - extra arith opcodes
 *       - branching
 *       - texture sampling
 *       - indexed attribs
 *       - indexed results
 *  4. bugs
 */

#define SWZ_X 0
#define SWZ_Y 1
#define SWZ_Z 2
#define SWZ_W 3
#define MASK_X 8
#define MASK_Y 4
#define MASK_Z 2
#define MASK_W 1
#define MASK_ALL (MASK_X|MASK_Y|MASK_Z|MASK_W)
#define DEF_SCALE 0
#define DEF_CTEST 0
#include "nv40_shader.h"

#define swz(s,x,y,z,w) nv40_sr_swz((s), SWZ_##x, SWZ_##y, SWZ_##z, SWZ_##w)
#define neg(s) nv40_sr_neg((s))
#define abs(s) nv40_sr_abs((s))

#define NV40_VP_INST_DEST_CLIP(n) ((~0 - 6) + (n))

struct nv40_vpc {
	struct nv40_vertex_program *vp;

	struct nv40_vertex_program_exec *vpi;

	unsigned r_temps;
	unsigned r_temps_discard;
	struct nv40_sreg r_result[PIPE_MAX_SHADER_OUTPUTS];
	struct nv40_sreg *r_address;
	struct nv40_sreg *r_temp;

	struct nv40_sreg *imm;
	unsigned nr_imm;

	unsigned hpos_idx;
};

static struct nv40_sreg
temp(struct nv40_vpc *vpc)
{
	int idx = ffs(~vpc->r_temps) - 1;

	if (idx < 0) {
		NOUVEAU_ERR("out of temps!!\n");
		assert(0);
		return nv40_sr(NV40SR_TEMP, 0);
	}

	vpc->r_temps |= (1 << idx);
	vpc->r_temps_discard |= (1 << idx);
	return nv40_sr(NV40SR_TEMP, idx);
}

static INLINE void
release_temps(struct nv40_vpc *vpc)
{
	vpc->r_temps &= ~vpc->r_temps_discard;
	vpc->r_temps_discard = 0;
}

static struct nv40_sreg
constant(struct nv40_vpc *vpc, int pipe, float x, float y, float z, float w)
{
	struct nv40_vertex_program *vp = vpc->vp;
	struct nv40_vertex_program_data *vpd;
	int idx;

	if (pipe >= 0) {
		for (idx = 0; idx < vp->nr_consts; idx++) {
			if (vp->consts[idx].index == pipe)
				return nv40_sr(NV40SR_CONST, idx);
		}
	}

	idx = vp->nr_consts++;
	vp->consts = realloc(vp->consts, sizeof(*vpd) * vp->nr_consts);
	vpd = &vp->consts[idx];

	vpd->index = pipe;
	vpd->value[0] = x;
	vpd->value[1] = y;
	vpd->value[2] = z;
	vpd->value[3] = w;
	return nv40_sr(NV40SR_CONST, idx);
}

#define arith(cc,s,o,d,m,s0,s1,s2) \
	nv40_vp_arith((cc), (s), NV40_VP_INST_##o, (d), (m), (s0), (s1), (s2))

static void
emit_src(struct nv40_vpc *vpc, uint32_t *hw, int pos, struct nv40_sreg src)
{
	struct nv40_vertex_program *vp = vpc->vp;
	uint32_t sr = 0;

	switch (src.type) {
	case NV40SR_TEMP:
		sr |= (NV40_VP_SRC_REG_TYPE_TEMP << NV40_VP_SRC_REG_TYPE_SHIFT);
		sr |= (src.index << NV40_VP_SRC_TEMP_SRC_SHIFT);
		break;
	case NV40SR_INPUT:
		sr |= (NV40_VP_SRC_REG_TYPE_INPUT <<
		       NV40_VP_SRC_REG_TYPE_SHIFT);
		vp->ir |= (1 << src.index);
		hw[1] |= (src.index << NV40_VP_INST_INPUT_SRC_SHIFT);
		break;
	case NV40SR_CONST:
		sr |= (NV40_VP_SRC_REG_TYPE_CONST <<
		       NV40_VP_SRC_REG_TYPE_SHIFT);
		assert(vpc->vpi->const_index == -1 ||
		       vpc->vpi->const_index == src.index);
		vpc->vpi->const_index = src.index;
		break;
	case NV40SR_NONE:
		sr |= (NV40_VP_SRC_REG_TYPE_INPUT <<
		       NV40_VP_SRC_REG_TYPE_SHIFT);
		break;
	default:
		assert(0);
	}

	if (src.negate)
		sr |= NV40_VP_SRC_NEGATE;

	if (src.abs)
		hw[0] |= (1 << (21 + pos));

	sr |= ((src.swz[0] << NV40_VP_SRC_SWZ_X_SHIFT) |
	       (src.swz[1] << NV40_VP_SRC_SWZ_Y_SHIFT) |
	       (src.swz[2] << NV40_VP_SRC_SWZ_Z_SHIFT) |
	       (src.swz[3] << NV40_VP_SRC_SWZ_W_SHIFT));

	switch (pos) {
	case 0:
		hw[1] |= ((sr & NV40_VP_SRC0_HIGH_MASK) >>
			  NV40_VP_SRC0_HIGH_SHIFT) << NV40_VP_INST_SRC0H_SHIFT;
		hw[2] |= (sr & NV40_VP_SRC0_LOW_MASK) <<
			  NV40_VP_INST_SRC0L_SHIFT;
		break;
	case 1:
		hw[2] |= sr << NV40_VP_INST_SRC1_SHIFT;
		break;
	case 2:
		hw[2] |= ((sr & NV40_VP_SRC2_HIGH_MASK) >>
			  NV40_VP_SRC2_HIGH_SHIFT) << NV40_VP_INST_SRC2H_SHIFT;
		hw[3] |= (sr & NV40_VP_SRC2_LOW_MASK) <<
			  NV40_VP_INST_SRC2L_SHIFT;
		break;
	default:
		assert(0);
	}
}

static void
emit_dst(struct nv40_vpc *vpc, uint32_t *hw, int slot, struct nv40_sreg dst)
{
	struct nv40_vertex_program *vp = vpc->vp;

	switch (dst.type) {
	case NV40SR_TEMP:
		hw[3] |= NV40_VP_INST_DEST_MASK;
		if (slot == 0) {
			hw[0] |= (dst.index <<
				  NV40_VP_INST_VEC_DEST_TEMP_SHIFT);
		} else {
			hw[3] |= (dst.index << 
				  NV40_VP_INST_SCA_DEST_TEMP_SHIFT);
		}
		break;
	case NV40SR_OUTPUT:
		switch (dst.index) {
		case NV40_VP_INST_DEST_COL0 : vp->or |= (1 << 0); break;
		case NV40_VP_INST_DEST_COL1 : vp->or |= (1 << 1); break;
		case NV40_VP_INST_DEST_BFC0 : vp->or |= (1 << 2); break;
		case NV40_VP_INST_DEST_BFC1 : vp->or |= (1 << 3); break;
		case NV40_VP_INST_DEST_FOGC : vp->or |= (1 << 4); break;
		case NV40_VP_INST_DEST_PSZ  : vp->or |= (1 << 5); break;
		case NV40_VP_INST_DEST_TC(0): vp->or |= (1 << 14); break;
		case NV40_VP_INST_DEST_TC(1): vp->or |= (1 << 15); break;
		case NV40_VP_INST_DEST_TC(2): vp->or |= (1 << 16); break;
		case NV40_VP_INST_DEST_TC(3): vp->or |= (1 << 17); break;
		case NV40_VP_INST_DEST_TC(4): vp->or |= (1 << 18); break;
		case NV40_VP_INST_DEST_TC(5): vp->or |= (1 << 19); break;
		case NV40_VP_INST_DEST_TC(6): vp->or |= (1 << 20); break;
		case NV40_VP_INST_DEST_TC(7): vp->or |= (1 << 21); break;
		case NV40_VP_INST_DEST_CLIP(0):
			vp->or |= (1 << 6);
			vp->clip_ctrl |= NV40TCL_CLIP_PLANE_ENABLE_PLANE0;
			dst.index = NV40_VP_INST_DEST_FOGC;
			break;
		case NV40_VP_INST_DEST_CLIP(1):
			vp->or |= (1 << 7);
			vp->clip_ctrl |= NV40TCL_CLIP_PLANE_ENABLE_PLANE1;
			dst.index = NV40_VP_INST_DEST_FOGC;
			break;
		case NV40_VP_INST_DEST_CLIP(2):
			vp->or |= (1 << 8);
			vp->clip_ctrl |= NV40TCL_CLIP_PLANE_ENABLE_PLANE2;
			dst.index = NV40_VP_INST_DEST_FOGC;
			break;
		case NV40_VP_INST_DEST_CLIP(3):
			vp->or |= (1 << 9);
			vp->clip_ctrl |= NV40TCL_CLIP_PLANE_ENABLE_PLANE3;
			dst.index = NV40_VP_INST_DEST_PSZ;
			break;
		case NV40_VP_INST_DEST_CLIP(4):
			vp->or |= (1 << 10);
			vp->clip_ctrl |= NV40TCL_CLIP_PLANE_ENABLE_PLANE4;
			dst.index = NV40_VP_INST_DEST_PSZ;
			break;
		case NV40_VP_INST_DEST_CLIP(5):
			vp->or |= (1 << 11);
			vp->clip_ctrl |= NV40TCL_CLIP_PLANE_ENABLE_PLANE5;
			dst.index = NV40_VP_INST_DEST_PSZ;
			break;
		default:
			break;
		}

		hw[3] |= (dst.index << NV40_VP_INST_DEST_SHIFT);
		if (slot == 0) {
			hw[0] |= NV40_VP_INST_VEC_RESULT;
			hw[0] |= NV40_VP_INST_VEC_DEST_TEMP_MASK | (1<<20);
		} else {
			hw[3] |= NV40_VP_INST_SCA_RESULT;
			hw[3] |= NV40_VP_INST_SCA_DEST_TEMP_MASK;
		}
		break;
	default:
		assert(0);
	}
}

static void
nv40_vp_arith(struct nv40_vpc *vpc, int slot, int op,
	      struct nv40_sreg dst, int mask,
	      struct nv40_sreg s0, struct nv40_sreg s1,
	      struct nv40_sreg s2)
{
	struct nv40_vertex_program *vp = vpc->vp;
	uint32_t *hw;

	vp->insns = realloc(vp->insns, ++vp->nr_insns * sizeof(*vpc->vpi));
	vpc->vpi = &vp->insns[vp->nr_insns - 1];
	memset(vpc->vpi, 0, sizeof(*vpc->vpi));
	vpc->vpi->const_index = -1;

	hw = vpc->vpi->data;

	hw[0] |= (NV40_VP_INST_COND_TR << NV40_VP_INST_COND_SHIFT);
	hw[0] |= ((0 << NV40_VP_INST_COND_SWZ_X_SHIFT) |
		  (1 << NV40_VP_INST_COND_SWZ_Y_SHIFT) |
		  (2 << NV40_VP_INST_COND_SWZ_Z_SHIFT) |
		  (3 << NV40_VP_INST_COND_SWZ_W_SHIFT));

	if (slot == 0) {
		hw[1] |= (op << NV40_VP_INST_VEC_OPCODE_SHIFT);
		hw[3] |= NV40_VP_INST_SCA_DEST_TEMP_MASK;
		hw[3] |= (mask << NV40_VP_INST_VEC_WRITEMASK_SHIFT);
	} else {
		hw[1] |= (op << NV40_VP_INST_SCA_OPCODE_SHIFT);
		hw[0] |= (NV40_VP_INST_VEC_DEST_TEMP_MASK | (1 << 20));
		hw[3] |= (mask << NV40_VP_INST_SCA_WRITEMASK_SHIFT);
	}

	emit_dst(vpc, hw, slot, dst);
	emit_src(vpc, hw, 0, s0);
	emit_src(vpc, hw, 1, s1);
	emit_src(vpc, hw, 2, s2);
}

static INLINE struct nv40_sreg
tgsi_src(struct nv40_vpc *vpc, const struct tgsi_full_src_register *fsrc) {
	struct nv40_sreg src;

	switch (fsrc->Register.File) {
	case TGSI_FILE_INPUT:
		src = nv40_sr(NV40SR_INPUT, fsrc->Register.Index);
		break;
	case TGSI_FILE_CONSTANT:
		src = constant(vpc, fsrc->Register.Index, 0, 0, 0, 0);
		break;
	case TGSI_FILE_IMMEDIATE:
		src = vpc->imm[fsrc->Register.Index];
		break;
	case TGSI_FILE_TEMPORARY:
		src = vpc->r_temp[fsrc->Register.Index];
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
tgsi_dst(struct nv40_vpc *vpc, const struct tgsi_full_dst_register *fdst) {
	struct nv40_sreg dst;

	switch (fdst->Register.File) {
	case TGSI_FILE_OUTPUT:
		dst = vpc->r_result[fdst->Register.Index];
		break;
	case TGSI_FILE_TEMPORARY:
		dst = vpc->r_temp[fdst->Register.Index];
		break;
	case TGSI_FILE_ADDRESS:
		dst = vpc->r_address[fdst->Register.Index];
		break;
	default:
		NOUVEAU_ERR("bad dst file\n");
		break;
	}

	return dst;
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
src_native_swz(struct nv40_vpc *vpc, const struct tgsi_full_src_register *fsrc,
	       struct nv40_sreg *src)
{
	const struct nv40_sreg none = nv40_sr(NV40SR_NONE, 0);
	struct nv40_sreg tgsi = tgsi_src(vpc, fsrc);
	uint mask = 0;
	uint c;

	for (c = 0; c < 4; c++) {
		switch (tgsi_util_get_full_src_register_swizzle(fsrc, c)) {
		case TGSI_SWIZZLE_X:
		case TGSI_SWIZZLE_Y:
		case TGSI_SWIZZLE_Z:
		case TGSI_SWIZZLE_W:
			mask |= tgsi_mask(1 << c);
			break;
		default:
			assert(0);
		}
	}

	if (mask == MASK_ALL)
		return TRUE;

	*src = temp(vpc);

	if (mask)
		arith(vpc, 0, OP_MOV, *src, mask, tgsi, none, none);

	return FALSE;
}

static boolean
nv40_vertprog_parse_instruction(struct nv40_vpc *vpc,
				const struct tgsi_full_instruction *finst)
{
	struct nv40_sreg src[3], dst, tmp;
	struct nv40_sreg none = nv40_sr(NV40SR_NONE, 0);
	int mask;
	int ai = -1, ci = -1, ii = -1;
	int i;

	if (finst->Instruction.Opcode == TGSI_OPCODE_END)
		return TRUE;

	for (i = 0; i < finst->Instruction.NumSrcRegs; i++) {
		const struct tgsi_full_src_register *fsrc;

		fsrc = &finst->Src[i];
		if (fsrc->Register.File == TGSI_FILE_TEMPORARY) {
			src[i] = tgsi_src(vpc, fsrc);
		}
	}

	for (i = 0; i < finst->Instruction.NumSrcRegs; i++) {
		const struct tgsi_full_src_register *fsrc;

		fsrc = &finst->Src[i];

		switch (fsrc->Register.File) {
		case TGSI_FILE_INPUT:
		case TGSI_FILE_CONSTANT:
		case TGSI_FILE_TEMPORARY:
			if (!src_native_swz(vpc, fsrc, &src[i]))
				continue;
			break;
		default:
			break;
		}

		switch (fsrc->Register.File) {
		case TGSI_FILE_INPUT:
			if (ai == -1 || ai == fsrc->Register.Index) {
				ai = fsrc->Register.Index;
				src[i] = tgsi_src(vpc, fsrc);
			} else {
				src[i] = temp(vpc);
				arith(vpc, 0, OP_MOV, src[i], MASK_ALL,
				      tgsi_src(vpc, fsrc), none, none);
			}
			break;
		case TGSI_FILE_CONSTANT:
			if ((ci == -1 && ii == -1) ||
			    ci == fsrc->Register.Index) {
				ci = fsrc->Register.Index;
				src[i] = tgsi_src(vpc, fsrc);
			} else {
				src[i] = temp(vpc);
				arith(vpc, 0, OP_MOV, src[i], MASK_ALL,
				      tgsi_src(vpc, fsrc), none, none);
			}
			break;
		case TGSI_FILE_IMMEDIATE:
			if ((ci == -1 && ii == -1) ||
			    ii == fsrc->Register.Index) {
				ii = fsrc->Register.Index;
				src[i] = tgsi_src(vpc, fsrc);
			} else {
				src[i] = temp(vpc);
				arith(vpc, 0, OP_MOV, src[i], MASK_ALL,
				      tgsi_src(vpc, fsrc), none, none);
			}
			break;
		case TGSI_FILE_TEMPORARY:
			/* handled above */
			break;
		default:
			NOUVEAU_ERR("bad src file\n");
			return FALSE;
		}
	}

	dst  = tgsi_dst(vpc, &finst->Dst[0]);
	mask = tgsi_mask(finst->Dst[0].Register.WriteMask);

	switch (finst->Instruction.Opcode) {
	case TGSI_OPCODE_ABS:
		arith(vpc, 0, OP_MOV, dst, mask, abs(src[0]), none, none);
		break;
	case TGSI_OPCODE_ADD:
		arith(vpc, 0, OP_ADD, dst, mask, src[0], none, src[1]);
		break;
	case TGSI_OPCODE_ARL:
		arith(vpc, 0, OP_ARL, dst, mask, src[0], none, none);
		break;
	case TGSI_OPCODE_DP3:
		arith(vpc, 0, OP_DP3, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_DP4:
		arith(vpc, 0, OP_DP4, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_DPH:
		arith(vpc, 0, OP_DPH, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_DST:
		arith(vpc, 0, OP_DST, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_EX2:
		arith(vpc, 1, OP_EX2, dst, mask, none, none, src[0]);
		break;
	case TGSI_OPCODE_EXP:
		arith(vpc, 1, OP_EXP, dst, mask, none, none, src[0]);
		break;
	case TGSI_OPCODE_FLR:
		arith(vpc, 0, OP_FLR, dst, mask, src[0], none, none);
		break;
	case TGSI_OPCODE_FRC:
		arith(vpc, 0, OP_FRC, dst, mask, src[0], none, none);
		break;
	case TGSI_OPCODE_LG2:
		arith(vpc, 1, OP_LG2, dst, mask, none, none, src[0]);
		break;
	case TGSI_OPCODE_LIT:
		arith(vpc, 1, OP_LIT, dst, mask, none, none, src[0]);
		break;
	case TGSI_OPCODE_LOG:
		arith(vpc, 1, OP_LOG, dst, mask, none, none, src[0]);
		break;
	case TGSI_OPCODE_MAD:
		arith(vpc, 0, OP_MAD, dst, mask, src[0], src[1], src[2]);
		break;
	case TGSI_OPCODE_MAX:
		arith(vpc, 0, OP_MAX, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_MIN:
		arith(vpc, 0, OP_MIN, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_MOV:
		arith(vpc, 0, OP_MOV, dst, mask, src[0], none, none);
		break;
	case TGSI_OPCODE_MUL:
		arith(vpc, 0, OP_MUL, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_POW:
		tmp = temp(vpc);
		arith(vpc, 1, OP_LG2, tmp, MASK_X, none, none,
		      swz(src[0], X, X, X, X));
		arith(vpc, 0, OP_MUL, tmp, MASK_X, swz(tmp, X, X, X, X),
		      swz(src[1], X, X, X, X), none);
		arith(vpc, 1, OP_EX2, dst, mask, none, none,
		      swz(tmp, X, X, X, X));
		break;
	case TGSI_OPCODE_RCP:
		arith(vpc, 1, OP_RCP, dst, mask, none, none, src[0]);
		break;
	case TGSI_OPCODE_RET:
		break;
	case TGSI_OPCODE_RSQ:
		arith(vpc, 1, OP_RSQ, dst, mask, none, none, abs(src[0]));
		break;
	case TGSI_OPCODE_SGE:
		arith(vpc, 0, OP_SGE, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_SLT:
		arith(vpc, 0, OP_SLT, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_SUB:
		arith(vpc, 0, OP_ADD, dst, mask, src[0], none, neg(src[1]));
		break;
	case TGSI_OPCODE_XPD:
		tmp = temp(vpc);
		arith(vpc, 0, OP_MUL, tmp, mask,
		      swz(src[0], Z, X, Y, Y), swz(src[1], Y, Z, X, X), none);
		arith(vpc, 0, OP_MAD, dst, (mask & ~MASK_W),
		      swz(src[0], Y, Z, X, X), swz(src[1], Z, X, Y, Y),
		      neg(tmp));
		break;
	default:
		NOUVEAU_ERR("invalid opcode %d\n", finst->Instruction.Opcode);
		return FALSE;
	}

	release_temps(vpc);
	return TRUE;
}

static boolean
nv40_vertprog_parse_decl_output(struct nv40_vpc *vpc,
				const struct tgsi_full_declaration *fdec)
{
	unsigned idx = fdec->Range.First;
	int hw;

	switch (fdec->Semantic.Name) {
	case TGSI_SEMANTIC_POSITION:
		hw = NV40_VP_INST_DEST_POS;
		vpc->hpos_idx = idx;
		break;
	case TGSI_SEMANTIC_COLOR:
		if (fdec->Semantic.Index == 0) {
			hw = NV40_VP_INST_DEST_COL0;
		} else
		if (fdec->Semantic.Index == 1) {
			hw = NV40_VP_INST_DEST_COL1;
		} else {
			NOUVEAU_ERR("bad colour semantic index\n");
			return FALSE;
		}
		break;
	case TGSI_SEMANTIC_BCOLOR:
		if (fdec->Semantic.Index == 0) {
			hw = NV40_VP_INST_DEST_BFC0;
		} else
		if (fdec->Semantic.Index == 1) {
			hw = NV40_VP_INST_DEST_BFC1;
		} else {
			NOUVEAU_ERR("bad bcolour semantic index\n");
			return FALSE;
		}
		break;
	case TGSI_SEMANTIC_FOG:
		hw = NV40_VP_INST_DEST_FOGC;
		break;
	case TGSI_SEMANTIC_PSIZE:
		hw = NV40_VP_INST_DEST_PSZ;
		break;
	case TGSI_SEMANTIC_GENERIC:
		if (fdec->Semantic.Index <= 7) {
			hw = NV40_VP_INST_DEST_TC(fdec->Semantic.Index);
		} else {
			NOUVEAU_ERR("bad generic semantic index\n");
			return FALSE;
		}
		break;
	case TGSI_SEMANTIC_EDGEFLAG:
		/* not really an error just a fallback */
		NOUVEAU_ERR("cannot handle edgeflag output\n");
		return FALSE;
	default:
		NOUVEAU_ERR("bad output semantic\n");
		return FALSE;
	}

	vpc->r_result[idx] = nv40_sr(NV40SR_OUTPUT, hw);
	return TRUE;
}

static boolean
nv40_vertprog_prepare(struct nv40_vpc *vpc)
{
	struct tgsi_parse_context p;
	int high_temp = -1, high_addr = -1, nr_imm = 0, i;

	tgsi_parse_init(&p, vpc->vp->pipe.tokens);
	while (!tgsi_parse_end_of_tokens(&p)) {
		const union tgsi_full_token *tok = &p.FullToken;

		tgsi_parse_token(&p);
		switch(tok->Token.Type) {
		case TGSI_TOKEN_TYPE_IMMEDIATE:
			nr_imm++;
			break;
		case TGSI_TOKEN_TYPE_DECLARATION:
		{
			const struct tgsi_full_declaration *fdec;

			fdec = &p.FullToken.FullDeclaration;
			switch (fdec->Declaration.File) {
			case TGSI_FILE_TEMPORARY:
				if (fdec->Range.Last > high_temp) {
					high_temp =
						fdec->Range.Last;
				}
				break;
#if 0 /* this would be nice.. except gallium doesn't track it */
			case TGSI_FILE_ADDRESS:
				if (fdec->Range.Last > high_addr) {
					high_addr =
						fdec->Range.Last;
				}
				break;
#endif
			case TGSI_FILE_OUTPUT:
				if (!nv40_vertprog_parse_decl_output(vpc, fdec))
					return FALSE;
				break;
			default:
				break;
			}
		}
			break;
#if 1 /* yay, parse instructions looking for address regs instead */
		case TGSI_TOKEN_TYPE_INSTRUCTION:
		{
			const struct tgsi_full_instruction *finst;
			const struct tgsi_full_dst_register *fdst;

			finst = &p.FullToken.FullInstruction;
			fdst = &finst->Dst[0];

			if (fdst->Register.File == TGSI_FILE_ADDRESS) {
				if (fdst->Register.Index > high_addr)
					high_addr = fdst->Register.Index;
			}
		
		}
			break;
#endif
		default:
			break;
		}
	}
	tgsi_parse_free(&p);

	if (nr_imm) {
		vpc->imm = CALLOC(nr_imm, sizeof(struct nv40_sreg));
		assert(vpc->imm);
	}

	if (++high_temp) {
		vpc->r_temp = CALLOC(high_temp, sizeof(struct nv40_sreg));
		for (i = 0; i < high_temp; i++)
			vpc->r_temp[i] = temp(vpc);
	}

	if (++high_addr) {
		vpc->r_address = CALLOC(high_addr, sizeof(struct nv40_sreg));
		for (i = 0; i < high_addr; i++)
			vpc->r_address[i] = temp(vpc);
	}

	vpc->r_temps_discard = 0;
	return TRUE;
}

static void
nv40_vertprog_translate(struct nv40_context *nv40,
			struct nv40_vertex_program *vp)
{
	struct tgsi_parse_context parse;
	struct nv40_vpc *vpc = NULL;
	struct nv40_sreg none = nv40_sr(NV40SR_NONE, 0);
	int i;

	vpc = CALLOC(1, sizeof(struct nv40_vpc));
	if (!vpc)
		return;
	vpc->vp = vp;

	if (!nv40_vertprog_prepare(vpc)) {
		FREE(vpc);
		return;
	}

	/* Redirect post-transform vertex position to a temp if user clip
	 * planes are enabled.  We need to append code to the vtxprog
	 * to handle clip planes later.
	 */
	if (vp->ucp.nr)  {
		vpc->r_result[vpc->hpos_idx] = temp(vpc);
		vpc->r_temps_discard = 0;
	}

	tgsi_parse_init(&parse, vp->pipe.tokens);

	while (!tgsi_parse_end_of_tokens(&parse)) {
		tgsi_parse_token(&parse);

		switch (parse.FullToken.Token.Type) {
		case TGSI_TOKEN_TYPE_IMMEDIATE:
		{
			const struct tgsi_full_immediate *imm;

			imm = &parse.FullToken.FullImmediate;
			assert(imm->Immediate.DataType == TGSI_IMM_FLOAT32);
			assert(imm->Immediate.NrTokens == 4 + 1);
			vpc->imm[vpc->nr_imm++] =
				constant(vpc, -1,
					 imm->u[0].Float,
					 imm->u[1].Float,
					 imm->u[2].Float,
					 imm->u[3].Float);
		}
			break;
		case TGSI_TOKEN_TYPE_INSTRUCTION:
		{
			const struct tgsi_full_instruction *finst;
			finst = &parse.FullToken.FullInstruction;
			if (!nv40_vertprog_parse_instruction(vpc, finst))
				goto out_err;
		}
			break;
		default:
			break;
		}
	}

	/* Write out HPOS if it was redirected to a temp earlier */
	if (vpc->r_result[vpc->hpos_idx].type != NV40SR_OUTPUT) {
		struct nv40_sreg hpos = nv40_sr(NV40SR_OUTPUT,
						NV40_VP_INST_DEST_POS);
		struct nv40_sreg htmp = vpc->r_result[vpc->hpos_idx];

		arith(vpc, 0, OP_MOV, hpos, MASK_ALL, htmp, none, none);
	}

	/* Insert code to handle user clip planes */
	for (i = 0; i < vp->ucp.nr; i++) {
		struct nv40_sreg cdst = nv40_sr(NV40SR_OUTPUT,
						NV40_VP_INST_DEST_CLIP(i));
		struct nv40_sreg ceqn = constant(vpc, -1,
						 nv40->clip.ucp[i][0],
						 nv40->clip.ucp[i][1],
						 nv40->clip.ucp[i][2],
						 nv40->clip.ucp[i][3]);
		struct nv40_sreg htmp = vpc->r_result[vpc->hpos_idx];
		unsigned mask;

		switch (i) {
		case 0: case 3: mask = MASK_Y; break;
		case 1: case 4: mask = MASK_Z; break;
		case 2: case 5: mask = MASK_W; break;
		default:
			NOUVEAU_ERR("invalid clip dist #%d\n", i);
			goto out_err;
		}

		arith(vpc, 0, OP_DP4, cdst, mask, htmp, ceqn, none);
	}

	vp->insns[vp->nr_insns - 1].data[3] |= NV40_VP_INST_LAST;
	vp->translated = TRUE;
out_err:
	tgsi_parse_free(&parse);
	if (vpc->r_temp)
		FREE(vpc->r_temp); 
	if (vpc->r_address)
		FREE(vpc->r_address); 
	if (vpc->imm)	
		FREE(vpc->imm); 
	FREE(vpc);
}

static boolean
nv40_vertprog_validate(struct nv40_context *nv40)
{ 
	struct pipe_screen *pscreen = nv40->pipe.screen;
	struct nv40_screen *screen = nv40->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *curie = screen->curie;
	struct nv40_vertex_program *vp;
	struct pipe_buffer *constbuf;
	boolean upload_code = FALSE, upload_data = FALSE;
	int i;

	if (nv40->render_mode == HW) {
		vp = nv40->vertprog;
		constbuf = nv40->constbuf[PIPE_SHADER_VERTEX];

		if ((nv40->dirty & NV40_NEW_UCP) ||
		    memcmp(&nv40->clip, &vp->ucp, sizeof(vp->ucp))) {
			nv40_vertprog_destroy(nv40, vp);
			memcpy(&vp->ucp, &nv40->clip, sizeof(vp->ucp));
		}
	} else {
		vp = nv40->swtnl.vertprog;
		constbuf = NULL;
	}

	/* Translate TGSI shader into hw bytecode */
	if (vp->translated)
		goto check_gpu_resources;

	nv40->fallback_swtnl &= ~NV40_NEW_VERTPROG;
	nv40_vertprog_translate(nv40, vp);
	if (!vp->translated) {
		nv40->fallback_swtnl |= NV40_NEW_VERTPROG;
		return FALSE;
	}

check_gpu_resources:
	/* Allocate hw vtxprog exec slots */
	if (!vp->exec) {
		struct nouveau_resource *heap = nv40->screen->vp_exec_heap;
		struct nouveau_stateobj *so;
		uint vplen = vp->nr_insns;

		if (nouveau_resource_alloc(heap, vplen, vp, &vp->exec)) {
			while (heap->next && heap->size < vplen) {
				struct nv40_vertex_program *evict;
				
				evict = heap->next->priv;
				nouveau_resource_free(&evict->exec);
			}

			if (nouveau_resource_alloc(heap, vplen, vp, &vp->exec))
				assert(0);
		}

		so = so_new(3, 4, 0);
		so_method(so, curie, NV40TCL_VP_START_FROM_ID, 1);
		so_data  (so, vp->exec->start);
		so_method(so, curie, NV40TCL_VP_ATTRIB_EN, 2);
		so_data  (so, vp->ir);
		so_data  (so, vp->or);
		so_method(so, curie,  NV40TCL_CLIP_PLANE_ENABLE, 1);
		so_data  (so, vp->clip_ctrl);
		so_ref(so, &vp->so);
		so_ref(NULL, &so);

		upload_code = TRUE;
	}

	/* Allocate hw vtxprog const slots */
	if (vp->nr_consts && !vp->data) {
		struct nouveau_resource *heap = nv40->screen->vp_data_heap;

		if (nouveau_resource_alloc(heap, vp->nr_consts, vp, &vp->data)) {
			while (heap->next && heap->size < vp->nr_consts) {
				struct nv40_vertex_program *evict;
				
				evict = heap->next->priv;
				nouveau_resource_free(&evict->data);
			}

			if (nouveau_resource_alloc(heap, vp->nr_consts, vp, &vp->data))
				assert(0);
		}

		/*XXX: handle this some day */
		assert(vp->data->start >= vp->data_start_min);

		upload_data = TRUE;
		if (vp->data_start != vp->data->start)
			upload_code = TRUE;
	}

	/* If exec or data segments moved we need to patch the program to
	 * fixup offsets and register IDs.
	 */
	if (vp->exec_start != vp->exec->start) {
		for (i = 0; i < vp->nr_insns; i++) {
			struct nv40_vertex_program_exec *vpi = &vp->insns[i];

			if (vpi->has_branch_offset) {
				assert(0);
			}
		}

		vp->exec_start = vp->exec->start;
	}

	if (vp->nr_consts && vp->data_start != vp->data->start) {
		for (i = 0; i < vp->nr_insns; i++) {
			struct nv40_vertex_program_exec *vpi = &vp->insns[i];

			if (vpi->const_index >= 0) {
				vpi->data[1] &= ~NV40_VP_INST_CONST_SRC_MASK;
				vpi->data[1] |=
					(vpi->const_index + vp->data->start) <<
					NV40_VP_INST_CONST_SRC_SHIFT;

			}
		}

		vp->data_start = vp->data->start;
	}

	/* Update + Upload constant values */
	if (vp->nr_consts) {
		float *map = NULL;

		if (constbuf) {
			map = pipe_buffer_map(pscreen, constbuf,
					      PIPE_BUFFER_USAGE_CPU_READ);
		}

		for (i = 0; i < vp->nr_consts; i++) {
			struct nv40_vertex_program_data *vpd = &vp->consts[i];

			if (vpd->index >= 0) {
				if (!upload_data &&
				    !memcmp(vpd->value, &map[vpd->index * 4],
					    4 * sizeof(float)))
					continue;
				memcpy(vpd->value, &map[vpd->index * 4],
				       4 * sizeof(float));
			}

			BEGIN_RING(chan, curie, NV40TCL_VP_UPLOAD_CONST_ID, 5);
			OUT_RING  (chan, i + vp->data->start);
			OUT_RINGp (chan, (uint32_t *)vpd->value, 4);
		}

		if (constbuf)
			pscreen->buffer_unmap(pscreen, constbuf);
	}

	/* Upload vtxprog */
	if (upload_code) {
#if 0
		for (i = 0; i < vp->nr_insns; i++) {
			NOUVEAU_MSG("VP %d: 0x%08x\n", i, vp->insns[i].data[0]);
			NOUVEAU_MSG("VP %d: 0x%08x\n", i, vp->insns[i].data[1]);
			NOUVEAU_MSG("VP %d: 0x%08x\n", i, vp->insns[i].data[2]);
			NOUVEAU_MSG("VP %d: 0x%08x\n", i, vp->insns[i].data[3]);
		}
#endif
		BEGIN_RING(chan, curie, NV40TCL_VP_UPLOAD_FROM_ID, 1);
		OUT_RING  (chan, vp->exec->start);
		for (i = 0; i < vp->nr_insns; i++) {
			BEGIN_RING(chan, curie, NV40TCL_VP_UPLOAD_INST(0), 4);
			OUT_RINGp (chan, vp->insns[i].data, 4);
		}
	}

	if (vp->so != nv40->state.hw[NV40_STATE_VERTPROG]) {
		so_ref(vp->so, &nv40->state.hw[NV40_STATE_VERTPROG]);
		return TRUE;
	}

	return FALSE;
}

void
nv40_vertprog_destroy(struct nv40_context *nv40, struct nv40_vertex_program *vp)
{
	vp->translated = FALSE;

	if (vp->nr_insns) {
		FREE(vp->insns);
		vp->insns = NULL;
		vp->nr_insns = 0;
	}

	if (vp->nr_consts) {
		FREE(vp->consts);
		vp->consts = NULL;
		vp->nr_consts = 0;
	}

	nouveau_resource_free(&vp->exec);
	vp->exec_start = 0;
	nouveau_resource_free(&vp->data);
	vp->data_start = 0;
	vp->data_start_min = 0;

	vp->ir = vp->or = vp->clip_ctrl = 0;
	so_ref(NULL, &vp->so);
}

struct nv40_state_entry nv40_state_vertprog = {
	.validate = nv40_vertprog_validate,
	.dirty = {
		.pipe = NV40_NEW_VERTPROG | NV40_NEW_UCP,
		.hw = NV40_STATE_VERTPROG,
	}
};

