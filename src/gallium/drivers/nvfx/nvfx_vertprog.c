#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "util/u_linkage.h"
#include "util/u_debug.h"

#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_util.h"
#include "tgsi/tgsi_ureg.h"

#include "draw/draw_context.h"

#include "nvfx_context.h"
#include "nvfx_state.h"
#include "nvfx_resource.h"

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

#include "nv30_vertprog.h"
#include "nv40_vertprog.h"

struct nvfx_loop_entry
{
	unsigned brk_target;
	unsigned cont_target;
};

struct nvfx_vpc {
	struct nvfx_context* nvfx;
	struct pipe_shader_state pipe;
	struct nvfx_vertex_program *vp;
	struct tgsi_shader_info* info;

	struct nvfx_vertex_program_exec *vpi;

	unsigned r_temps;
	unsigned r_temps_discard;
	struct nvfx_reg r_result[PIPE_MAX_SHADER_OUTPUTS];
	struct nvfx_reg *r_address;
	struct nvfx_reg *r_temp;
	struct nvfx_reg *r_const;
	struct nvfx_reg r_0_1;

	struct nvfx_reg *imm;
	unsigned nr_imm;

	unsigned hpos_idx;

	struct util_dynarray label_relocs;
	struct util_dynarray loop_stack;
};

static struct nvfx_reg
temp(struct nvfx_vpc *vpc)
{
	int idx = ffs(~vpc->r_temps) - 1;

	if (idx < 0) {
		NOUVEAU_ERR("out of temps!!\n");
		assert(0);
		return nvfx_reg(NVFXSR_TEMP, 0);
	}

	vpc->r_temps |= (1 << idx);
	vpc->r_temps_discard |= (1 << idx);
	return nvfx_reg(NVFXSR_TEMP, idx);
}

static inline void
release_temps(struct nvfx_vpc *vpc)
{
	vpc->r_temps &= ~vpc->r_temps_discard;
	vpc->r_temps_discard = 0;
}

static struct nvfx_reg
constant(struct nvfx_vpc *vpc, int pipe, float x, float y, float z, float w)
{
	struct nvfx_vertex_program *vp = vpc->vp;
	struct nvfx_vertex_program_data *vpd;
	int idx;

	if (pipe >= 0) {
		for (idx = 0; idx < vp->nr_consts; idx++) {
			if (vp->consts[idx].index == pipe)
				return nvfx_reg(NVFXSR_CONST, idx);
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
	return nvfx_reg(NVFXSR_CONST, idx);
}

#define arith(s,t,o,d,m,s0,s1,s2) \
	nvfx_insn((s), (NVFX_VP_INST_SLOT_##t << 7) | NVFX_VP_INST_##t##_OP_##o, -1, (d), (m), (s0), (s1), (s2))

static void
emit_src(struct nvfx_context* nvfx, struct nvfx_vpc *vpc, uint32_t *hw, int pos, struct nvfx_src src)
{
	struct nvfx_vertex_program *vp = vpc->vp;
	uint32_t sr = 0;
	struct nvfx_relocation reloc;

	switch (src.reg.type) {
	case NVFXSR_TEMP:
		sr |= (NVFX_VP(SRC_REG_TYPE_TEMP) << NVFX_VP(SRC_REG_TYPE_SHIFT));
		sr |= (src.reg.index << NVFX_VP(SRC_TEMP_SRC_SHIFT));
		break;
	case NVFXSR_INPUT:
		sr |= (NVFX_VP(SRC_REG_TYPE_INPUT) <<
		       NVFX_VP(SRC_REG_TYPE_SHIFT));
		vp->ir |= (1 << src.reg.index);
		hw[1] |= (src.reg.index << NVFX_VP(INST_INPUT_SRC_SHIFT));
		break;
	case NVFXSR_CONST:
		sr |= (NVFX_VP(SRC_REG_TYPE_CONST) <<
		       NVFX_VP(SRC_REG_TYPE_SHIFT));
		reloc.location = vp->nr_insns - 1;
		reloc.target = src.reg.index;
		util_dynarray_append(&vp->const_relocs, struct nvfx_relocation, reloc);
		break;
	case NVFXSR_NONE:
		sr |= (NVFX_VP(SRC_REG_TYPE_INPUT) <<
		       NVFX_VP(SRC_REG_TYPE_SHIFT));
		break;
	default:
		assert(0);
	}

	if (src.negate)
		sr |= NVFX_VP(SRC_NEGATE);

	if (src.abs)
		hw[0] |= (1 << (21 + pos));

	sr |= ((src.swz[0] << NVFX_VP(SRC_SWZ_X_SHIFT)) |
	       (src.swz[1] << NVFX_VP(SRC_SWZ_Y_SHIFT)) |
	       (src.swz[2] << NVFX_VP(SRC_SWZ_Z_SHIFT)) |
	       (src.swz[3] << NVFX_VP(SRC_SWZ_W_SHIFT)));

	if(src.indirect) {
		if(src.reg.type == NVFXSR_CONST)
			hw[3] |= NVFX_VP(INST_INDEX_CONST);
		else if(src.reg.type == NVFXSR_INPUT)
			hw[0] |= NVFX_VP(INST_INDEX_INPUT);
		else
			assert(0);
		if(src.indirect_reg)
			hw[0] |= NVFX_VP(INST_ADDR_REG_SELECT_1);
		hw[0] |= src.indirect_swz << NVFX_VP(INST_ADDR_SWZ_SHIFT);
	}

	switch (pos) {
	case 0:
		hw[1] |= ((sr & NVFX_VP(SRC0_HIGH_MASK)) >>
			  NVFX_VP(SRC0_HIGH_SHIFT)) << NVFX_VP(INST_SRC0H_SHIFT);
		hw[2] |= (sr & NVFX_VP(SRC0_LOW_MASK)) <<
			  NVFX_VP(INST_SRC0L_SHIFT);
		break;
	case 1:
		hw[2] |= sr << NVFX_VP(INST_SRC1_SHIFT);
		break;
	case 2:
		hw[2] |= ((sr & NVFX_VP(SRC2_HIGH_MASK)) >>
			  NVFX_VP(SRC2_HIGH_SHIFT)) << NVFX_VP(INST_SRC2H_SHIFT);
		hw[3] |= (sr & NVFX_VP(SRC2_LOW_MASK)) <<
			  NVFX_VP(INST_SRC2L_SHIFT);
		break;
	default:
		assert(0);
	}
}

static void
emit_dst(struct nvfx_context* nvfx, struct nvfx_vpc *vpc, uint32_t *hw, int slot, struct nvfx_reg dst)
{
	struct nvfx_vertex_program *vp = vpc->vp;

	switch (dst.type) {
	case NVFXSR_NONE:
		if(!nvfx->is_nv4x)
			hw[0] |= NV30_VP_INST_DEST_TEMP_ID_MASK;
		else {
			hw[3] |= NV40_VP_INST_DEST_MASK;
			if (slot == 0)
				hw[0] |= NV40_VP_INST_VEC_DEST_TEMP_MASK;
			else
				hw[3] |= NV40_VP_INST_SCA_DEST_TEMP_MASK;
		}
		break;
	case NVFXSR_TEMP:
		if(!nvfx->is_nv4x)
			hw[0] |= (dst.index << NV30_VP_INST_DEST_TEMP_ID_SHIFT);
		else {
			hw[3] |= NV40_VP_INST_DEST_MASK;
			if (slot == 0)
				hw[0] |= (dst.index << NV40_VP_INST_VEC_DEST_TEMP_SHIFT);
			else
				hw[3] |= (dst.index << NV40_VP_INST_SCA_DEST_TEMP_SHIFT);
		}
		break;
	case NVFXSR_OUTPUT:
		/* TODO: this may be wrong because on nv30 COL0 and BFC0 are swapped */
		if(nvfx->is_nv4x) {
			switch (dst.index) {
			case NV30_VP_INST_DEST_CLP(0):
				dst.index = NVFX_VP(INST_DEST_FOGC);
				break;
			case NV30_VP_INST_DEST_CLP(1):
				dst.index = NVFX_VP(INST_DEST_FOGC);
				break;
			case NV30_VP_INST_DEST_CLP(2):
				dst.index = NVFX_VP(INST_DEST_FOGC);
				break;
			case NV30_VP_INST_DEST_CLP(3):
				dst.index = NVFX_VP(INST_DEST_PSZ);
				break;
			case NV30_VP_INST_DEST_CLP(4):
				dst.index = NVFX_VP(INST_DEST_PSZ);
				break;
			case NV30_VP_INST_DEST_CLP(5):
				dst.index = NVFX_VP(INST_DEST_PSZ);
				break;
			case NV40_VP_INST_DEST_COL0 : vp->or |= (1 << 0); break;
			case NV40_VP_INST_DEST_COL1 : vp->or |= (1 << 1); break;
			case NV40_VP_INST_DEST_BFC0 : vp->or |= (1 << 2); break;
			case NV40_VP_INST_DEST_BFC1 : vp->or |= (1 << 3); break;
			case NV40_VP_INST_DEST_FOGC: vp->or |= (1 << 4); break;
			case NV40_VP_INST_DEST_PSZ  : vp->or |= (1 << 5); break;
			}
		}

		if(!nvfx->is_nv4x) {
			hw[3] |= (dst.index << NV30_VP_INST_DEST_SHIFT);
			hw[0] |= NV30_VP_INST_VEC_DEST_TEMP_MASK;

			/*XXX: no way this is entirely correct, someone needs to
			 *     figure out what exactly it is.
			 */
			hw[3] |= 0x800;
		} else {
			hw[3] |= (dst.index << NV40_VP_INST_DEST_SHIFT);
			if (slot == 0) {
				hw[0] |= NV40_VP_INST_VEC_RESULT;
				hw[0] |= NV40_VP_INST_VEC_DEST_TEMP_MASK;
			} else {
				hw[3] |= NV40_VP_INST_SCA_RESULT;
				hw[3] |= NV40_VP_INST_SCA_DEST_TEMP_MASK;
			}
		}
		break;
	default:
		assert(0);
	}
}

static void
nvfx_vp_emit(struct nvfx_vpc *vpc, struct nvfx_insn insn)
{
	struct nvfx_context* nvfx = vpc->nvfx;
	struct nvfx_vertex_program *vp = vpc->vp;
	unsigned slot = insn.op >> 7;
	unsigned op = insn.op & 0x7f;
	uint32_t *hw;

	vp->insns = realloc(vp->insns, ++vp->nr_insns * sizeof(*vpc->vpi));
	vpc->vpi = &vp->insns[vp->nr_insns - 1];
	memset(vpc->vpi, 0, sizeof(*vpc->vpi));

	hw = vpc->vpi->data;

	hw[0] |= (insn.cc_test << NVFX_VP(INST_COND_SHIFT));
	hw[0] |= ((insn.cc_swz[0] << NVFX_VP(INST_COND_SWZ_X_SHIFT)) |
		  (insn.cc_swz[1] << NVFX_VP(INST_COND_SWZ_Y_SHIFT)) |
		  (insn.cc_swz[2] << NVFX_VP(INST_COND_SWZ_Z_SHIFT)) |
		  (insn.cc_swz[3] << NVFX_VP(INST_COND_SWZ_W_SHIFT)));
	if(insn.cc_update)
		hw[0] |= NVFX_VP(INST_COND_UPDATE_ENABLE);

	if(insn.sat)
	{
		assert(nvfx->use_nv4x);
		if(nvfx->use_nv4x)
			hw[0] |= NV40_VP_INST_SATURATE;
	}

	if(!nvfx->is_nv4x) {
		if(slot == 0)
			hw[1] |= (op << NV30_VP_INST_VEC_OPCODE_SHIFT);
		else
		{
			hw[0] |= ((op >> 4) << NV30_VP_INST_SCA_OPCODEH_SHIFT);
			hw[1] |= ((op & 0xf) << NV30_VP_INST_SCA_OPCODEL_SHIFT);
		}
//		hw[3] |= NVFX_VP(INST_SCA_DEST_TEMP_MASK);
//		hw[3] |= (mask << NVFX_VP(INST_VEC_WRITEMASK_SHIFT));

		if (insn.dst.type == NVFXSR_OUTPUT) {
			if (slot)
				hw[3] |= (insn.mask << NV30_VP_INST_SDEST_WRITEMASK_SHIFT);
			else
				hw[3] |= (insn.mask << NV30_VP_INST_VDEST_WRITEMASK_SHIFT);
		} else {
			if (slot)
				hw[3] |= (insn.mask << NV30_VP_INST_STEMP_WRITEMASK_SHIFT);
			else
				hw[3] |= (insn.mask << NV30_VP_INST_VTEMP_WRITEMASK_SHIFT);
		}
	 } else {
		if (slot == 0) {
			hw[1] |= (op << NV40_VP_INST_VEC_OPCODE_SHIFT);
			hw[3] |= NV40_VP_INST_SCA_DEST_TEMP_MASK;
			hw[3] |= (insn.mask << NV40_VP_INST_VEC_WRITEMASK_SHIFT);
	    } else {
			hw[1] |= (op << NV40_VP_INST_SCA_OPCODE_SHIFT);
			hw[0] |= NV40_VP_INST_VEC_DEST_TEMP_MASK ;
			hw[3] |= (insn.mask << NV40_VP_INST_SCA_WRITEMASK_SHIFT);
		}
	}

	emit_dst(nvfx, vpc, hw, slot, insn.dst);
	emit_src(nvfx, vpc, hw, 0, insn.src[0]);
	emit_src(nvfx, vpc, hw, 1, insn.src[1]);
	emit_src(nvfx, vpc, hw, 2, insn.src[2]);

//	if(insn.src[0].indirect || op == NVFX_VP_INST_VEC_OP_ARL)
//		hw[3] |= NV40_VP_INST_SCA_RESULT;
}

static inline struct nvfx_src
tgsi_src(struct nvfx_vpc *vpc, const struct tgsi_full_src_register *fsrc) {
	struct nvfx_src src;

	switch (fsrc->Register.File) {
	case TGSI_FILE_INPUT:
		src.reg = nvfx_reg(NVFXSR_INPUT, fsrc->Register.Index);
		break;
	case TGSI_FILE_CONSTANT:
		src.reg = vpc->r_const[fsrc->Register.Index];
		break;
	case TGSI_FILE_IMMEDIATE:
		src.reg = vpc->imm[fsrc->Register.Index];
		break;
	case TGSI_FILE_TEMPORARY:
		src.reg = vpc->r_temp[fsrc->Register.Index];
		break;
	default:
		NOUVEAU_ERR("bad src file\n");
		src.reg.index = 0;
		src.reg.type = -1;
		break;
	}

	src.abs = fsrc->Register.Absolute;
	src.negate = fsrc->Register.Negate;
	src.swz[0] = fsrc->Register.SwizzleX;
	src.swz[1] = fsrc->Register.SwizzleY;
	src.swz[2] = fsrc->Register.SwizzleZ;
	src.swz[3] = fsrc->Register.SwizzleW;
	src.indirect = 0;
	src.indirect_reg = 0;
	src.indirect_swz = 0;

	if(fsrc->Register.Indirect) {
		if(fsrc->Indirect.File == TGSI_FILE_ADDRESS &&
				(fsrc->Register.File == TGSI_FILE_CONSTANT || fsrc->Register.File == TGSI_FILE_INPUT))
		{
			src.indirect = 1;
			src.indirect_reg = fsrc->Indirect.Index;
			src.indirect_swz = fsrc->Indirect.SwizzleX;
		}
		else
		{
			src.reg.index = 0;
			src.reg.type = -1;
		}
	}
	return src;
}

static INLINE struct nvfx_reg
tgsi_dst(struct nvfx_vpc *vpc, const struct tgsi_full_dst_register *fdst) {
	struct nvfx_reg dst;

	switch (fdst->Register.File) {
	case TGSI_FILE_NULL:
		dst = nvfx_reg(NVFXSR_NONE, 0);
		break;
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
		NOUVEAU_ERR("bad dst file %i\n", fdst->Register.File);
		dst.index = 0;
		dst.type = 0;
		break;
	}

	return dst;
}

static inline int
tgsi_mask(uint tgsi)
{
	int mask = 0;

	if (tgsi & TGSI_WRITEMASK_X) mask |= NVFX_VP_MASK_X;
	if (tgsi & TGSI_WRITEMASK_Y) mask |= NVFX_VP_MASK_Y;
	if (tgsi & TGSI_WRITEMASK_Z) mask |= NVFX_VP_MASK_Z;
	if (tgsi & TGSI_WRITEMASK_W) mask |= NVFX_VP_MASK_W;
	return mask;
}

static boolean
nvfx_vertprog_parse_instruction(struct nvfx_context* nvfx, struct nvfx_vpc *vpc,
				unsigned idx, const struct tgsi_full_instruction *finst)
{
	struct nvfx_src src[3], tmp;
	struct nvfx_reg dst;
	struct nvfx_reg final_dst;
	struct nvfx_src none = nvfx_src(nvfx_reg(NVFXSR_NONE, 0));
	struct nvfx_insn insn;
	struct nvfx_relocation reloc;
	struct nvfx_loop_entry loop;
	boolean sat = FALSE;
	int mask;
	int ai = -1, ci = -1, ii = -1;
	int i;
	unsigned sub_depth = 0;

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
			if (ai == -1 || ai == fsrc->Register.Index) {
				ai = fsrc->Register.Index;
				src[i] = tgsi_src(vpc, fsrc);
			} else {
				src[i] = nvfx_src(temp(vpc));
				nvfx_vp_emit(vpc, arith(0, VEC, MOV, src[i].reg, NVFX_VP_MASK_ALL, tgsi_src(vpc, fsrc), none, none));
			}
			break;
		case TGSI_FILE_CONSTANT:
			if ((ci == -1 && ii == -1) ||
			    ci == fsrc->Register.Index) {
				ci = fsrc->Register.Index;
				src[i] = tgsi_src(vpc, fsrc);
			} else {
				src[i] = nvfx_src(temp(vpc));
				nvfx_vp_emit(vpc, arith(0, VEC, MOV, src[i].reg, NVFX_VP_MASK_ALL, tgsi_src(vpc, fsrc), none, none));
			}
			break;
		case TGSI_FILE_IMMEDIATE:
			if ((ci == -1 && ii == -1) ||
			    ii == fsrc->Register.Index) {
				ii = fsrc->Register.Index;
				src[i] = tgsi_src(vpc, fsrc);
			} else {
				src[i] = nvfx_src(temp(vpc));
				nvfx_vp_emit(vpc, arith(0, VEC, MOV, src[i].reg, NVFX_VP_MASK_ALL, tgsi_src(vpc, fsrc), none, none));
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

	for (i = 0; i < finst->Instruction.NumSrcRegs; i++) {
		if(src[i].reg.type < 0)
			return FALSE;
	}

	if(finst->Dst[0].Register.File == TGSI_FILE_ADDRESS &&
			finst->Instruction.Opcode != TGSI_OPCODE_ARL)
		return FALSE;

	final_dst = dst  = tgsi_dst(vpc, &finst->Dst[0]);
	mask = tgsi_mask(finst->Dst[0].Register.WriteMask);
	if(finst->Instruction.Saturate == TGSI_SAT_ZERO_ONE)
	{
		assert(finst->Instruction.Opcode != TGSI_OPCODE_ARL);
		if(nvfx->use_nv4x)
			sat = TRUE;
		else if(dst.type != NVFXSR_TEMP)
			dst = temp(vpc);
	}

	switch (finst->Instruction.Opcode) {
	case TGSI_OPCODE_ABS:
		nvfx_vp_emit(vpc, arith(sat, VEC, MOV, dst, mask, abs(src[0]), none, none));
		break;
	case TGSI_OPCODE_ADD:
		nvfx_vp_emit(vpc, arith(sat, VEC, ADD, dst, mask, src[0], none, src[1]));
		break;
	case TGSI_OPCODE_ARL:
		nvfx_vp_emit(vpc, arith(0, VEC, ARL, dst, mask, src[0], none, none));
		break;
	case TGSI_OPCODE_CMP:
		insn = arith(0, VEC, MOV, none.reg, mask, src[0], none, none);
		insn.cc_update = 1;
		nvfx_vp_emit(vpc, insn);

		insn = arith(sat, VEC, MOV, dst, mask, src[2], none, none);
		insn.cc_test = NVFX_COND_GE;
		nvfx_vp_emit(vpc, insn);

		insn = arith(sat, VEC, MOV, dst, mask, src[1], none, none);
		insn.cc_test = NVFX_COND_LT;
		nvfx_vp_emit(vpc, insn);
		break;
	case TGSI_OPCODE_COS:
		nvfx_vp_emit(vpc, arith(sat, SCA, COS, dst, mask, none, none, src[0]));
		break;
        case TGSI_OPCODE_DP2:
                tmp = nvfx_src(temp(vpc));
                nvfx_vp_emit(vpc, arith(0, VEC, MUL, tmp.reg, NVFX_VP_MASK_X | NVFX_VP_MASK_Y, src[0], src[1], none));
                nvfx_vp_emit(vpc, arith(sat, VEC, ADD, dst, mask, swz(tmp, X, X, X, X), none, swz(tmp, Y, Y, Y, Y)));
                break;
	case TGSI_OPCODE_DP3:
		nvfx_vp_emit(vpc, arith(sat, VEC, DP3, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_DP4:
		nvfx_vp_emit(vpc, arith(sat, VEC, DP4, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_DPH:
		nvfx_vp_emit(vpc, arith(sat, VEC, DPH, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_DST:
		nvfx_vp_emit(vpc, arith(sat, VEC, DST, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_EX2:
		nvfx_vp_emit(vpc, arith(sat, SCA, EX2, dst, mask, none, none, src[0]));
		break;
	case TGSI_OPCODE_EXP:
		nvfx_vp_emit(vpc, arith(sat, SCA, EXP, dst, mask, none, none, src[0]));
		break;
	case TGSI_OPCODE_FLR:
		nvfx_vp_emit(vpc, arith(sat, VEC, FLR, dst, mask, src[0], none, none));
		break;
	case TGSI_OPCODE_FRC:
		nvfx_vp_emit(vpc, arith(sat, VEC, FRC, dst, mask, src[0], none, none));
		break;
	case TGSI_OPCODE_LG2:
		nvfx_vp_emit(vpc, arith(sat, SCA, LG2, dst, mask, none, none, src[0]));
		break;
	case TGSI_OPCODE_LIT:
		nvfx_vp_emit(vpc, arith(sat, SCA, LIT, dst, mask, none, none, src[0]));
		break;
	case TGSI_OPCODE_LOG:
		nvfx_vp_emit(vpc, arith(sat, SCA, LOG, dst, mask, none, none, src[0]));
		break;
	case TGSI_OPCODE_LRP:
		tmp = nvfx_src(temp(vpc));
		nvfx_vp_emit(vpc, arith(0, VEC, MAD, tmp.reg, mask, neg(src[0]), src[2], src[2]));
		nvfx_vp_emit(vpc, arith(sat, VEC, MAD, dst, mask, src[0], src[1], tmp));
		break;
	case TGSI_OPCODE_MAD:
		nvfx_vp_emit(vpc, arith(sat, VEC, MAD, dst, mask, src[0], src[1], src[2]));
		break;
	case TGSI_OPCODE_MAX:
		nvfx_vp_emit(vpc, arith(sat, VEC, MAX, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_MIN:
		nvfx_vp_emit(vpc, arith(sat, VEC, MIN, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_MOV:
		nvfx_vp_emit(vpc, arith(sat, VEC, MOV, dst, mask, src[0], none, none));
		break;
	case TGSI_OPCODE_MUL:
		nvfx_vp_emit(vpc, arith(sat, VEC, MUL, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_NOP:
		break;
	case TGSI_OPCODE_POW:
		tmp = nvfx_src(temp(vpc));
		nvfx_vp_emit(vpc, arith(0, SCA, LG2, tmp.reg, NVFX_VP_MASK_X, none, none, swz(src[0], X, X, X, X)));
		nvfx_vp_emit(vpc, arith(0, VEC, MUL, tmp.reg, NVFX_VP_MASK_X, swz(tmp, X, X, X, X), swz(src[1], X, X, X, X), none));
		nvfx_vp_emit(vpc, arith(sat, SCA, EX2, dst, mask, none, none, swz(tmp, X, X, X, X)));
		break;
	case TGSI_OPCODE_RCP:
		nvfx_vp_emit(vpc, arith(sat, SCA, RCP, dst, mask, none, none, src[0]));
		break;
	case TGSI_OPCODE_RSQ:
		nvfx_vp_emit(vpc, arith(sat, SCA, RSQ, dst, mask, none, none, abs(src[0])));
		break;
	case TGSI_OPCODE_SEQ:
		nvfx_vp_emit(vpc, arith(sat, VEC, SEQ, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_SFL:
		nvfx_vp_emit(vpc, arith(sat, VEC, SFL, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_SGE:
		nvfx_vp_emit(vpc, arith(sat, VEC, SGE, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_SGT:
		nvfx_vp_emit(vpc, arith(sat, VEC, SGT, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_SIN:
		nvfx_vp_emit(vpc, arith(sat, SCA, SIN, dst, mask, none, none, src[0]));
		break;
	case TGSI_OPCODE_SLE:
		nvfx_vp_emit(vpc, arith(sat, VEC, SLE, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_SLT:
		nvfx_vp_emit(vpc, arith(sat, VEC, SLT, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_SNE:
		nvfx_vp_emit(vpc, arith(sat, VEC, SNE, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_SSG:
		nvfx_vp_emit(vpc, arith(sat, VEC, SSG, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_STR:
		nvfx_vp_emit(vpc, arith(sat, VEC, STR, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_SUB:
		nvfx_vp_emit(vpc, arith(sat, VEC, ADD, dst, mask, src[0], none, neg(src[1])));
		break;
        case TGSI_OPCODE_TRUNC:
                tmp = nvfx_src(temp(vpc));
                insn = arith(0, VEC, MOV, none.reg, mask, src[0], none, none);
                insn.cc_update = 1;
                nvfx_vp_emit(vpc, insn);

                nvfx_vp_emit(vpc, arith(0, VEC, FLR, tmp.reg, mask, abs(src[0]), none, none));
                nvfx_vp_emit(vpc, arith(sat, VEC, MOV, dst, mask, tmp, none, none));

                insn = arith(sat, VEC, MOV, dst, mask, neg(tmp), none, none);
                insn.cc_test = NVFX_COND_LT;
                nvfx_vp_emit(vpc, insn);
                break;
	case TGSI_OPCODE_XPD:
		tmp = nvfx_src(temp(vpc));
		nvfx_vp_emit(vpc, arith(0, VEC, MUL, tmp.reg, mask, swz(src[0], Z, X, Y, Y), swz(src[1], Y, Z, X, X), none));
		nvfx_vp_emit(vpc, arith(sat, VEC, MAD, dst, (mask & ~NVFX_VP_MASK_W), swz(src[0], Y, Z, X, X), swz(src[1], Z, X, Y, Y), neg(tmp)));
		break;

	case TGSI_OPCODE_IF:
		insn = arith(0, VEC, MOV, none.reg, NVFX_VP_MASK_X, src[0], none, none);
		insn.cc_update = 1;
		nvfx_vp_emit(vpc, insn);

		reloc.location = vpc->vp->nr_insns;
		reloc.target = finst->Label.Label + 1;
		util_dynarray_append(&vpc->label_relocs, struct nvfx_relocation, reloc);

		insn = arith(0, SCA, BRA, none.reg, 0, none, none, none);
		insn.cc_test = NVFX_COND_EQ;
		insn.cc_swz[0] = insn.cc_swz[1] = insn.cc_swz[2] = insn.cc_swz[3] = 0;
		nvfx_vp_emit(vpc, insn);
		break;

	case TGSI_OPCODE_ELSE:
	case TGSI_OPCODE_BRA:
	case TGSI_OPCODE_CAL:
		reloc.location = vpc->vp->nr_insns;
		reloc.target = finst->Label.Label;
		util_dynarray_append(&vpc->label_relocs, struct nvfx_relocation, reloc);

		if(finst->Instruction.Opcode == TGSI_OPCODE_CAL)
			insn = arith(0, SCA, CAL, none.reg, 0, none, none, none);
		else
			insn = arith(0, SCA, BRA, none.reg, 0, none, none, none);
		nvfx_vp_emit(vpc, insn);
		break;

	case TGSI_OPCODE_RET:
		if(sub_depth || !nvfx->use_vp_clipping) {
			tmp = none;
			tmp.swz[0] = tmp.swz[1] = tmp.swz[2] = tmp.swz[3] = 0;
			nvfx_vp_emit(vpc, arith(0, SCA, RET, none.reg, 0, none, none, tmp));
		} else {
			reloc.location = vpc->vp->nr_insns;
			reloc.target = vpc->info->num_instructions;
			util_dynarray_append(&vpc->label_relocs, struct nvfx_relocation, reloc);
			nvfx_vp_emit(vpc, arith(0, SCA, BRA, none.reg, 0, none, none, none));
		}
		break;

	case TGSI_OPCODE_BGNSUB:
		++sub_depth;
		break;
	case TGSI_OPCODE_ENDSUB:
		--sub_depth;
		break;
	case TGSI_OPCODE_ENDIF:
		/* nothing to do here */
		break;

	case TGSI_OPCODE_BGNLOOP:
		loop.cont_target = idx;
		loop.brk_target = finst->Label.Label + 1;
		util_dynarray_append(&vpc->loop_stack, struct nvfx_loop_entry, loop);
		break;

	case TGSI_OPCODE_ENDLOOP:
		loop = util_dynarray_pop(&vpc->loop_stack, struct nvfx_loop_entry);

		reloc.location = vpc->vp->nr_insns;
		reloc.target = loop.cont_target;
		util_dynarray_append(&vpc->label_relocs, struct nvfx_relocation, reloc);

		nvfx_vp_emit(vpc, arith(0, SCA, BRA, none.reg, 0, none, none, none));
		break;

	case TGSI_OPCODE_CONT:
		loop = util_dynarray_top(&vpc->loop_stack, struct nvfx_loop_entry);

		reloc.location = vpc->vp->nr_insns;
		reloc.target = loop.cont_target;
		util_dynarray_append(&vpc->label_relocs, struct nvfx_relocation, reloc);

		nvfx_vp_emit(vpc, arith(0, SCA, BRA, none.reg, 0, none, none, none));
		break;

	case TGSI_OPCODE_BRK:
		loop = util_dynarray_top(&vpc->loop_stack, struct nvfx_loop_entry);

		reloc.location = vpc->vp->nr_insns;
		reloc.target = loop.brk_target;
		util_dynarray_append(&vpc->label_relocs, struct nvfx_relocation, reloc);

		nvfx_vp_emit(vpc, arith(0, SCA, BRA, none.reg, 0, none, none, none));
		break;

	case TGSI_OPCODE_END:
		assert(!sub_depth);
		if(nvfx->use_vp_clipping) {
			if(idx != (vpc->info->num_instructions - 1)) {
				reloc.location = vpc->vp->nr_insns;
				reloc.target = vpc->info->num_instructions;
				util_dynarray_append(&vpc->label_relocs, struct nvfx_relocation, reloc);
				nvfx_vp_emit(vpc, arith(0, SCA, BRA, none.reg, 0, none, none, none));
			}
		} else {
			if(vpc->vp->nr_insns)
				vpc->vp->insns[vpc->vp->nr_insns - 1].data[3] |= NVFX_VP_INST_LAST;
			nvfx_vp_emit(vpc, arith(0, VEC, NOP, none.reg, 0, none, none, none));
			vpc->vp->insns[vpc->vp->nr_insns - 1].data[3] |= NVFX_VP_INST_LAST;
		}
		break;

	default:
		NOUVEAU_ERR("invalid opcode %d\n", finst->Instruction.Opcode);
		return FALSE;
	}

	if(finst->Instruction.Saturate == TGSI_SAT_ZERO_ONE && !nvfx->use_nv4x)
	{
		if(!vpc->r_0_1.type)
			vpc->r_0_1 = constant(vpc, -1, 0, 1, 0, 0);
		nvfx_vp_emit(vpc, arith(0, VEC, MAX, dst, mask, nvfx_src(dst), swz(nvfx_src(vpc->r_0_1), X, X, X, X), none));
		nvfx_vp_emit(vpc, arith(0, VEC, MIN, final_dst, mask, nvfx_src(dst), swz(nvfx_src(vpc->r_0_1), Y, Y, Y, Y), none));
	}

	release_temps(vpc);
	return TRUE;
}

static boolean
nvfx_vertprog_parse_decl_output(struct nvfx_context* nvfx, struct nvfx_vpc *vpc,
				const struct tgsi_full_declaration *fdec)
{
	unsigned idx = fdec->Range.First;
	int hw;

	switch (fdec->Semantic.Name) {
	case TGSI_SEMANTIC_POSITION:
		hw = NVFX_VP(INST_DEST_POS);
		vpc->hpos_idx = idx;
		break;
	case TGSI_SEMANTIC_COLOR:
		if (fdec->Semantic.Index == 0) {
			hw = NVFX_VP(INST_DEST_COL0);
		} else
		if (fdec->Semantic.Index == 1) {
			hw = NVFX_VP(INST_DEST_COL1);
		} else {
			NOUVEAU_ERR("bad colour semantic index\n");
			return FALSE;
		}
		break;
	case TGSI_SEMANTIC_BCOLOR:
		if (fdec->Semantic.Index == 0) {
			hw = NVFX_VP(INST_DEST_BFC0);
		} else
		if (fdec->Semantic.Index == 1) {
			hw = NVFX_VP(INST_DEST_BFC1);
		} else {
			NOUVEAU_ERR("bad bcolour semantic index\n");
			return FALSE;
		}
		break;
	case TGSI_SEMANTIC_FOG:
		hw = NVFX_VP(INST_DEST_FOGC);
		break;
	case TGSI_SEMANTIC_PSIZE:
		hw = NVFX_VP(INST_DEST_PSZ);
		break;
	case TGSI_SEMANTIC_GENERIC:
		hw = (vpc->vp->generic_to_fp_input[fdec->Semantic.Index] & 0xf) - NVFX_FP_OP_INPUT_SRC_TC(0);
		if(hw <= 8)
			hw = NVFX_VP(INST_DEST_TC(hw));
		else if(hw == 9) /* TODO: this is correct, but how does this overlapping work exactly? */
			hw = NV40_VP_INST_DEST_PSZ;
		else
			assert(0);
		break;
	case TGSI_SEMANTIC_EDGEFLAG:
		/* not really an error just a fallback */
		NOUVEAU_ERR("cannot handle edgeflag output\n");
		return FALSE;
	default:
		NOUVEAU_ERR("bad output semantic\n");
		return FALSE;
	}

	vpc->r_result[idx] = nvfx_reg(NVFXSR_OUTPUT, hw);
	return TRUE;
}

static boolean
nvfx_vertprog_prepare(struct nvfx_context* nvfx, struct nvfx_vpc *vpc)
{
	struct tgsi_parse_context p;
	int high_const = -1, high_temp = -1, high_addr = -1, nr_imm = 0, i;
	struct util_semantic_set set;
	unsigned char sem_layout[10];
	unsigned num_outputs;
	unsigned num_texcoords = nvfx->is_nv4x ? 10 : 8;

	num_outputs = util_semantic_set_from_program_file(&set, vpc->pipe.tokens, TGSI_FILE_OUTPUT);

	if(num_outputs > num_texcoords) {
		NOUVEAU_ERR("too many vertex program outputs: %i\n", num_outputs);
		return FALSE;
	}
	util_semantic_layout_from_set(sem_layout, &set, num_texcoords, num_texcoords);

	/* hope 0xf is (0, 0, 0, 1) initialized; otherwise, we are _probably_ not required to do this */
	memset(vpc->vp->generic_to_fp_input, 0x0f, sizeof(vpc->vp->generic_to_fp_input));
	for(int i = 0; i < num_texcoords; ++i) {
		if(sem_layout[i] == 0xff)
			continue;
		//printf("vp: GENERIC[%i] to fpreg %i\n", sem_layout[i], NVFX_FP_OP_INPUT_SRC_TC(0) + i);
		vpc->vp->generic_to_fp_input[sem_layout[i]] = 0xf0 | NVFX_FP_OP_INPUT_SRC_TC(i);
	}

	vpc->vp->sprite_fp_input = -1;
	for(int i = 0; i < num_texcoords; ++i)
	{
		if(sem_layout[i] == 0xff)
		{
			vpc->vp->sprite_fp_input = NVFX_FP_OP_INPUT_SRC_TC(i);
			break;
		}
	}

	tgsi_parse_init(&p, vpc->pipe.tokens);
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
			case TGSI_FILE_ADDRESS:
				if (fdec->Range.Last > high_addr) {
					high_addr =
						fdec->Range.Last;
				}
				break;
			case TGSI_FILE_CONSTANT:
				if (fdec->Range.Last > high_const) {
					high_const =
							fdec->Range.Last;
				}
				break;
			case TGSI_FILE_OUTPUT:
				if (!nvfx_vertprog_parse_decl_output(nvfx, vpc, fdec))
					return FALSE;
				break;
			default:
				break;
			}
		}
			break;
		default:
			break;
		}
	}
	tgsi_parse_free(&p);

	if (nr_imm) {
		vpc->imm = CALLOC(nr_imm, sizeof(struct nvfx_reg));
		assert(vpc->imm);
	}

	if (++high_temp) {
		vpc->r_temp = CALLOC(high_temp, sizeof(struct nvfx_reg));
		for (i = 0; i < high_temp; i++)
			vpc->r_temp[i] = temp(vpc);
	}

	if (++high_addr) {
		vpc->r_address = CALLOC(high_addr, sizeof(struct nvfx_reg));
		for (i = 0; i < high_addr; i++)
			vpc->r_address[i] = nvfx_reg(NVFXSR_TEMP, i);
	}

	if(++high_const) {
		vpc->r_const = CALLOC(high_const, sizeof(struct nvfx_reg));
		for (i = 0; i < high_const; i++)
			vpc->r_const[i] = constant(vpc, i, 0, 0, 0, 0);
	}

	vpc->r_temps_discard = 0;
	return TRUE;
}

DEBUG_GET_ONCE_BOOL_OPTION(nvfx_dump_vp, "NVFX_DUMP_VP", FALSE)

static struct nvfx_vertex_program*
nvfx_vertprog_translate(struct nvfx_context *nvfx, const struct pipe_shader_state* vps, struct tgsi_shader_info* info)
{
	struct tgsi_parse_context parse;
	struct nvfx_vertex_program* vp = NULL;
	struct nvfx_vpc *vpc = NULL;
	struct nvfx_src none = nvfx_src(nvfx_reg(NVFXSR_NONE, 0));
	struct util_dynarray insns;
	int i;

	tgsi_parse_init(&parse, vps->tokens);

	vp = CALLOC_STRUCT(nvfx_vertex_program);
	if(!vp)
		goto out_err;

	vpc = CALLOC_STRUCT(nvfx_vpc);
	if (!vpc)
		goto out_err;

	vpc->nvfx = nvfx;
	vpc->vp = vp;
	vpc->pipe = *vps;
	vpc->info = info;

	{
		// TODO: use a 64-bit atomic here!
		static unsigned long long id = 0;
		vp->id = ++id;
	}

	/* reserve space for ucps */
	if(nvfx->use_vp_clipping)
	{
		for(i = 0; i < 6; ++i)
			constant(vpc, -1, 0, 0, 0, 0);
	}

	if (!nvfx_vertprog_prepare(nvfx, vpc)) {
		FREE(vpc);
		return NULL;
	}

	/* Redirect post-transform vertex position to a temp if user clip
	 * planes are enabled.  We need to append code to the vtxprog
	 * to handle clip planes later.
	 */
	/* TODO: maybe support patching this depending on whether there are ucps: not sure if it is really matters much */
	if (nvfx->use_vp_clipping)  {
		vpc->r_result[vpc->hpos_idx] = temp(vpc);
		vpc->r_temps_discard = 0;
	}

	util_dynarray_init(&insns);
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
			unsigned idx = insns.size >> 2;
			util_dynarray_append(&insns, unsigned, vp->nr_insns);
			finst = &parse.FullToken.FullInstruction;
			if (!nvfx_vertprog_parse_instruction(nvfx, vpc, idx, finst))
				goto out_err;
		}
			break;
		default:
			break;
		}
	}

	util_dynarray_append(&insns, unsigned, vp->nr_insns);

	for(unsigned i = 0; i < vpc->label_relocs.size; i += sizeof(struct nvfx_relocation))
	{
		struct nvfx_relocation* label_reloc = (struct nvfx_relocation*)((char*)vpc->label_relocs.data + i);
		struct nvfx_relocation hw_reloc;

		hw_reloc.location = label_reloc->location;
		hw_reloc.target = ((unsigned*)insns.data)[label_reloc->target];

		//debug_printf("hw %u -> tgsi %u = hw %u\n", hw_reloc.location, label_reloc->target, hw_reloc.target);

		util_dynarray_append(&vp->branch_relocs, struct nvfx_relocation, hw_reloc);
	}
	util_dynarray_fini(&insns);
	util_dynarray_trim(&vp->branch_relocs);

	/* XXX: what if we add a RET before?!  make sure we jump here...*/

	/* Write out HPOS if it was redirected to a temp earlier */
	if (vpc->r_result[vpc->hpos_idx].type != NVFXSR_OUTPUT) {
		struct nvfx_reg hpos = nvfx_reg(NVFXSR_OUTPUT,
						NVFX_VP(INST_DEST_POS));
		struct nvfx_src htmp = nvfx_src(vpc->r_result[vpc->hpos_idx]);

		nvfx_vp_emit(vpc, arith(0, VEC, MOV, hpos, NVFX_VP_MASK_ALL, htmp, none, none));
	}

	/* Insert code to handle user clip planes */
	if(nvfx->use_vp_clipping)
	{
		for (i = 0; i < 6; i++) {
			struct nvfx_reg cdst = nvfx_reg(NVFXSR_OUTPUT, NV30_VP_INST_DEST_CLP(i));
			struct nvfx_src ceqn = nvfx_src(nvfx_reg(NVFXSR_CONST, i));
			struct nvfx_src htmp = nvfx_src(vpc->r_result[vpc->hpos_idx]);
			unsigned mask;

			if(nvfx->is_nv4x)
			{
				switch (i) {
				case 0: case 3: mask = NVFX_VP_MASK_Y; break;
				case 1: case 4: mask = NVFX_VP_MASK_Z; break;
				case 2: case 5: mask = NVFX_VP_MASK_W; break;
				default:
					NOUVEAU_ERR("invalid clip dist #%d\n", i);
					goto out_err;
				}
			}
			else
				mask = NVFX_VP_MASK_X;

			nvfx_vp_emit(vpc, arith(0, VEC, DP4, cdst, mask, htmp, ceqn, none));
		}
	}

	if(debug_get_option_nvfx_dump_vp())
	{
		debug_printf("\n");
		tgsi_dump(vpc->pipe.tokens, 0);

		debug_printf("\n%s vertex program:\n", nvfx->is_nv4x ? "nv4x" : "nv3x");
		for (i = 0; i < vp->nr_insns; i++)
			debug_printf("%3u: %08x %08x %08x %08x\n", i, vp->insns[i].data[0], vp->insns[i].data[1], vp->insns[i].data[2], vp->insns[i].data[3]);
		debug_printf("\n");
	}

	vp->clip_nr = -1;
	vp->exec_start = -1;

out:
	tgsi_parse_free(&parse);
	if(vpc) {
		util_dynarray_fini(&vpc->label_relocs);
		util_dynarray_fini(&vpc->loop_stack);
		FREE(vpc->r_temp);
		FREE(vpc->r_address);
		FREE(vpc->r_const);
		FREE(vpc->imm);
		FREE(vpc);
	}
	return vp;

out_err:
	FREE(vp);
	vp = NULL;
	goto out;
}

static struct nvfx_vertex_program*
nvfx_vertprog_translate_draw_vp(struct nvfx_context *nvfx, struct nvfx_pipe_vertex_program* pvp)
{
	struct nvfx_vertex_program* vp = NULL;
	struct pipe_shader_state vps;
	struct tgsi_shader_info info;
	struct ureg_program *ureg = NULL;
	unsigned num_outputs = MIN2(pvp->info.num_outputs, 16);

	ureg = ureg_create( TGSI_PROCESSOR_VERTEX );
	if(ureg == NULL)
		return 0;

	for (unsigned i = 0; i < num_outputs; i++)
		ureg_MOV(ureg, ureg_DECL_output(ureg, pvp->info.output_semantic_name[i], pvp->info.output_semantic_index[i]), ureg_DECL_vs_input(ureg, i));

	ureg_END( ureg );

	vps.tokens = ureg_get_tokens(ureg, 0);
	tgsi_scan_shader(vps.tokens, &info);
	vp = nvfx_vertprog_translate(nvfx, &vps, &info);
	ureg_free_tokens(vps.tokens);
	ureg_destroy(ureg);

	return vp;
}

boolean
nvfx_vertprog_validate(struct nvfx_context *nvfx)
{
	struct nvfx_screen *screen = nvfx->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *eng3d = screen->eng3d;
	struct nvfx_pipe_vertex_program *pvp = nvfx->vertprog;
	struct nvfx_vertex_program* vp;
	struct pipe_resource *constbuf;
	boolean upload_code = FALSE, upload_data = FALSE;
	int i;

	if (nvfx->render_mode == HW) {
		nvfx->fallback_swtnl &= ~NVFX_NEW_VERTPROG;
		vp = pvp->vp;

		if(!vp) {
			vp = nvfx_vertprog_translate(nvfx, &pvp->pipe, &pvp->info);
			if(!vp)
				vp = NVFX_VP_FAILED;
			pvp->vp = vp;
		}

		if(vp == NVFX_VP_FAILED) {
			nvfx->fallback_swtnl |= NVFX_NEW_VERTPROG;
			return FALSE;
		}

		constbuf = nvfx->constbuf[PIPE_SHADER_VERTEX];
	} else {
		vp = pvp->draw_vp;
		if(!vp)
		{
			pvp->draw_vp = vp = nvfx_vertprog_translate_draw_vp(nvfx, pvp);
			if(!vp) {
				_debug_printf("Error: unable to create a swtnl passthrough vertex shader: aborting.");
				abort();
			}
		}
		constbuf = NULL;
	}

	nvfx->hw_vertprog = vp;

	/* Allocate hw vtxprog exec slots */
	if (!vp->exec) {
		struct nouveau_resource *heap = nvfx->screen->vp_exec_heap;
		uint vplen = vp->nr_insns;

		if (nouveau_resource_alloc(heap, vplen, vp, &vp->exec)) {
			while (heap->next && heap->size < vplen) {
				struct nvfx_vertex_program *evict;

				evict = heap->next->priv;
				nouveau_resource_free(&evict->exec);
			}

			if (nouveau_resource_alloc(heap, vplen, vp, &vp->exec))
			{
				debug_printf("Vertex shader too long: %u instructions\n", vplen);
				nvfx->fallback_swtnl |= NVFX_NEW_VERTPROG;
				return FALSE;
			}
		}

		upload_code = TRUE;
	}

	/* Allocate hw vtxprog const slots */
	if (vp->nr_consts && !vp->data) {
		struct nouveau_resource *heap = nvfx->screen->vp_data_heap;

		if (nouveau_resource_alloc(heap, vp->nr_consts, vp, &vp->data)) {
			while (heap->next && heap->size < vp->nr_consts) {
				struct nvfx_vertex_program *evict;

				evict = heap->next->priv;
				nouveau_resource_free(&evict->data);
			}

			if (nouveau_resource_alloc(heap, vp->nr_consts, vp, &vp->data))
                        {
                                debug_printf("Vertex shader uses too many constants: %u constants\n", vp->nr_consts);
                                nvfx->fallback_swtnl |= NVFX_NEW_VERTPROG;
                                return FALSE;
                        }
		}

		//printf("start at %u nc %u\n", vp->data->start, vp->nr_consts);

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
		//printf("vp_relocs %u -> %u\n", vp->exec_start, vp->exec->start);
		for(unsigned i = 0; i < vp->branch_relocs.size; i += sizeof(struct nvfx_relocation))
		{
			struct nvfx_relocation* reloc = (struct nvfx_relocation*)((char*)vp->branch_relocs.data + i);
			uint32_t* hw = vp->insns[reloc->location].data;
			unsigned target = vp->exec->start + reloc->target;

			//debug_printf("vp_reloc hw %u -> hw %u\n", reloc->location, target);

			if(!nvfx->is_nv4x)
			{
				hw[2] &=~ NV30_VP_INST_IADDR_MASK;
				hw[2] |= (target & 0x1ff) << NV30_VP_INST_IADDR_SHIFT;
			}
			else
			{
				hw[3] &=~ NV40_VP_INST_IADDRL_MASK;
				hw[3] |= (target & 7) << NV40_VP_INST_IADDRL_SHIFT;

				hw[2] &=~ NV40_VP_INST_IADDRH_MASK;
				hw[2] |= ((target >> 3) & 0x3f) << NV40_VP_INST_IADDRH_SHIFT;
			}
		}

		vp->exec_start = vp->exec->start;
	}

	if (vp->data_start != vp->data->start) {
		for(unsigned i = 0; i < vp->const_relocs.size; i += sizeof(struct nvfx_relocation))
		{
			struct nvfx_relocation* reloc = (struct nvfx_relocation*)((char*)vp->const_relocs.data + i);
			struct nvfx_vertex_program_exec *vpi = &vp->insns[reloc->location];

			//printf("reloc %i to %i + %i\n", reloc->location, vp->data->start, reloc->target);

			vpi->data[1] &= ~NVFX_VP(INST_CONST_SRC_MASK);
			vpi->data[1] |=
					(reloc->target + vp->data->start) <<
					NVFX_VP(INST_CONST_SRC_SHIFT);
		}

		vp->data_start = vp->data->start;
		upload_code = TRUE;
	}

	/* Update + Upload constant values */
	if (vp->nr_consts) {
		float *map = NULL;

		if (constbuf)
			map = (float*)nvfx_buffer(constbuf)->data;

		/*
		 * WAIT_RING(chan, 512 * 6);
		for (i = 0; i < 512; i++) {
			float v[4] = {0.1, 0,2, 0.3, 0.4};
			OUT_RING(chan, RING_3D(NV30_3D_VP_UPLOAD_CONST_ID, 5));
			OUT_RING(chan, i);
			OUT_RINGp(chan, (uint32_t *)v, 4);
			printf("frob %i\n", i);
		}
		*/

		for (i = nvfx->use_vp_clipping ? 6 : 0; i < vp->nr_consts; i++) {
			struct nvfx_vertex_program_data *vpd = &vp->consts[i];

			if (vpd->index >= 0) {
				if (!upload_data &&
				    !memcmp(vpd->value, &map[vpd->index * 4],
					    4 * sizeof(float)))
					continue;
				memcpy(vpd->value, &map[vpd->index * 4],
				       4 * sizeof(float));
			}

			//printf("upload into %i + %i: %f %f %f %f\n", vp->data->start, i, vpd->value[0], vpd->value[1], vpd->value[2], vpd->value[3]);

			BEGIN_RING(chan, eng3d, NV30_3D_VP_UPLOAD_CONST_ID, 5);
			OUT_RING(chan, i + vp->data->start);
			OUT_RINGp(chan, (uint32_t *)vpd->value, 4);
		}
	}

	/* Upload vtxprog */
	if (upload_code) {
		BEGIN_RING(chan, eng3d, NV30_3D_VP_UPLOAD_FROM_ID, 1);
		OUT_RING(chan, vp->exec->start);
		for (i = 0; i < vp->nr_insns; i++) {
			BEGIN_RING(chan, eng3d, NV30_3D_VP_UPLOAD_INST(0), 4);
			//printf("%08x %08x %08x %08x\n", vp->insns[i].data[0], vp->insns[i].data[1], vp->insns[i].data[2], vp->insns[i].data[3]);
			OUT_RINGp(chan, vp->insns[i].data, 4);
		}
		vp->clip_nr = -1;
	}

	if(nvfx->dirty & (NVFX_NEW_VERTPROG))
	{
		BEGIN_RING(chan, eng3d, NV30_3D_VP_START_FROM_ID, 1);
		OUT_RING(chan, vp->exec->start);
		if(nvfx->is_nv4x) {
			BEGIN_RING(chan, eng3d, NV40_3D_VP_ATTRIB_EN, 1);
			OUT_RING(chan, vp->ir);
		}
	}

	return TRUE;
}

void
nvfx_vertprog_destroy(struct nvfx_context *nvfx, struct nvfx_vertex_program *vp)
{
	if (vp->nr_insns)
		FREE(vp->insns);

	if (vp->nr_consts)
		FREE(vp->consts);

	nouveau_resource_free(&vp->exec);
	nouveau_resource_free(&vp->data);

	util_dynarray_fini(&vp->branch_relocs);
	util_dynarray_fini(&vp->const_relocs);
	FREE(vp);
}

static void *
nvfx_vp_state_create(struct pipe_context *pipe, const struct pipe_shader_state *cso)
{
        struct nvfx_pipe_vertex_program *pvp;

        pvp = CALLOC(1, sizeof(struct nvfx_pipe_vertex_program));
        pvp->pipe.tokens = tgsi_dup_tokens(cso->tokens);
        tgsi_scan_shader(pvp->pipe.tokens, &pvp->info);
        pvp->draw_elements = MAX2(1, MIN2(pvp->info.num_outputs, 16));
        pvp->draw_no_elements = pvp->info.num_outputs == 0;

        return (void *)pvp;
}

static void
nvfx_vp_state_bind(struct pipe_context *pipe, void *hwcso)
{
        struct nvfx_context *nvfx = nvfx_context(pipe);

        nvfx->vertprog = hwcso;
        nvfx->dirty |= NVFX_NEW_VERTPROG;
        nvfx->draw_dirty |= NVFX_NEW_VERTPROG;
}

static void
nvfx_vp_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	struct nvfx_pipe_vertex_program *pvp = hwcso;

	if(pvp->draw_vs)
		draw_delete_vertex_shader(nvfx->draw, pvp->draw_vs);
	if(pvp->vp && pvp->vp != NVFX_VP_FAILED)
		nvfx_vertprog_destroy(nvfx, pvp->vp);
	if(pvp->draw_vp)
		nvfx_vertprog_destroy(nvfx, pvp->draw_vp);
	FREE((void*)pvp->pipe.tokens);
	FREE(pvp);
}

void
nvfx_init_vertprog_functions(struct nvfx_context *nvfx)
{
        nvfx->pipe.create_vs_state = nvfx_vp_state_create;
        nvfx->pipe.bind_vs_state = nvfx_vp_state_bind;
        nvfx->pipe.delete_vs_state = nvfx_vp_state_delete;
}
