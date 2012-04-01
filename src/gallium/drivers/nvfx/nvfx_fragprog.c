#include <float.h>
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/u_debug.h"

#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_ureg.h"

#include "nvfx_context.h"
#include "nvfx_shader.h"
#include "nvfx_resource.h"

struct nvfx_fpc {
	struct nvfx_pipe_fragment_program* pfp;
	struct nvfx_fragment_program *fp;

	unsigned max_temps;
	unsigned long long r_temps;
	unsigned long long r_temps_discard;
	struct nvfx_reg r_result[PIPE_MAX_SHADER_OUTPUTS];
	struct nvfx_reg *r_temp;
	unsigned sprite_coord_temp;

	int num_regs;

	unsigned inst_offset;
	unsigned have_const;

	struct util_dynarray imm_data;

	struct nvfx_reg* r_imm;
	unsigned nr_imm;

	unsigned char generic_to_slot[256]; /* semantic idx for each input semantic */

	struct util_dynarray if_stack;
	//struct util_dynarray loop_stack;
	struct util_dynarray label_relocs;
};

static INLINE struct nvfx_reg
temp(struct nvfx_fpc *fpc)
{
	int idx = __builtin_ctzll(~fpc->r_temps);

	if (idx >= fpc->max_temps) {
		NOUVEAU_ERR("out of temps!!\n");
		assert(0);
		return nvfx_reg(NVFXSR_TEMP, 0);
	}

	fpc->r_temps |= (1ULL << idx);
	fpc->r_temps_discard |= (1ULL << idx);
	return nvfx_reg(NVFXSR_TEMP, idx);
}

static INLINE void
release_temps(struct nvfx_fpc *fpc)
{
	fpc->r_temps &= ~fpc->r_temps_discard;
	fpc->r_temps_discard = 0ULL;
}

static inline struct nvfx_reg
nvfx_fp_imm(struct nvfx_fpc *fpc, float a, float b, float c, float d)
{
	float v[4] = {a, b, c, d};
	int idx = fpc->imm_data.size >> 4;

	memcpy(util_dynarray_grow(&fpc->imm_data, sizeof(float) * 4), v, 4 * sizeof(float));
	return nvfx_reg(NVFXSR_IMM, idx);
}

static void
grow_insns(struct nvfx_fpc *fpc, int size)
{
	struct nvfx_fragment_program *fp = fpc->fp;

	fp->insn_len += size;
	fp->insn = realloc(fp->insn, sizeof(uint32_t) * fp->insn_len);
}

static void
emit_src(struct nvfx_fpc *fpc, int pos, struct nvfx_src src)
{
	struct nvfx_fragment_program *fp = fpc->fp;
	uint32_t *hw = &fp->insn[fpc->inst_offset];
	uint32_t sr = 0;

	switch (src.reg.type) {
	case NVFXSR_INPUT:
		sr |= (NVFX_FP_REG_TYPE_INPUT << NVFX_FP_REG_TYPE_SHIFT);
		hw[0] |= (src.reg.index << NVFX_FP_OP_INPUT_SRC_SHIFT);
		break;
	case NVFXSR_OUTPUT:
		sr |= NVFX_FP_REG_SRC_HALF;
		/* fall-through */
	case NVFXSR_TEMP:
		sr |= (NVFX_FP_REG_TYPE_TEMP << NVFX_FP_REG_TYPE_SHIFT);
		sr |= (src.reg.index << NVFX_FP_REG_SRC_SHIFT);
		break;
	case NVFXSR_RELOCATED:
		sr |= (NVFX_FP_REG_TYPE_TEMP << NVFX_FP_REG_TYPE_SHIFT);
		sr |= (fpc->sprite_coord_temp << NVFX_FP_REG_SRC_SHIFT);
		//printf("adding relocation at %x for %x\n", fpc->inst_offset, src.index);
		util_dynarray_append(&fpc->fp->slot_relocations[src.reg.index], unsigned, fpc->inst_offset + pos + 1);
		break;
	case NVFXSR_IMM:
		if (!fpc->have_const) {
			grow_insns(fpc, 4);
			hw = &fp->insn[fpc->inst_offset];
			fpc->have_const = 1;
		}

		memcpy(&fp->insn[fpc->inst_offset + 4],
				(float*)fpc->imm_data.data + src.reg.index * 4,
				sizeof(uint32_t) * 4);

		sr |= (NVFX_FP_REG_TYPE_CONST << NVFX_FP_REG_TYPE_SHIFT);
		break;
	case NVFXSR_CONST:
		if (!fpc->have_const) {
			grow_insns(fpc, 4);
			hw = &fp->insn[fpc->inst_offset];
			fpc->have_const = 1;
		}

		{
			struct nvfx_fragment_program_data *fpd;

			fp->consts = realloc(fp->consts, ++fp->nr_consts *
					     sizeof(*fpd));
			fpd = &fp->consts[fp->nr_consts - 1];
			fpd->offset = fpc->inst_offset + 4;
			fpd->index = src.reg.index;
			memset(&fp->insn[fpd->offset], 0, sizeof(uint32_t) * 4);
		}

		sr |= (NVFX_FP_REG_TYPE_CONST << NVFX_FP_REG_TYPE_SHIFT);
		break;
	case NVFXSR_NONE:
		sr |= (NVFX_FP_REG_TYPE_INPUT << NVFX_FP_REG_TYPE_SHIFT);
		break;
	default:
		assert(0);
	}

	if (src.negate)
		sr |= NVFX_FP_REG_NEGATE;

	if (src.abs)
		hw[1] |= (1 << (29 + pos));

	sr |= ((src.swz[0] << NVFX_FP_REG_SWZ_X_SHIFT) |
	       (src.swz[1] << NVFX_FP_REG_SWZ_Y_SHIFT) |
	       (src.swz[2] << NVFX_FP_REG_SWZ_Z_SHIFT) |
	       (src.swz[3] << NVFX_FP_REG_SWZ_W_SHIFT));

	hw[pos + 1] |= sr;
}

static void
emit_dst(struct nvfx_fpc *fpc, struct nvfx_reg dst)
{
	struct nvfx_fragment_program *fp = fpc->fp;
	uint32_t *hw = &fp->insn[fpc->inst_offset];

	switch (dst.type) {
	case NVFXSR_TEMP:
		if (fpc->num_regs < (dst.index + 1))
			fpc->num_regs = dst.index + 1;
		break;
	case NVFXSR_OUTPUT:
		if (dst.index == 1) {
			fp->fp_control |= 0xe;
		} else {
			hw[0] |= NVFX_FP_OP_OUT_REG_HALF;
		}
		break;
	case NVFXSR_NONE:
		hw[0] |= (1 << 30);
		break;
	default:
		assert(0);
	}

	hw[0] |= (dst.index << NVFX_FP_OP_OUT_REG_SHIFT);
}

static void
nvfx_fp_emit(struct nvfx_fpc *fpc, struct nvfx_insn insn)
{
	struct nvfx_fragment_program *fp = fpc->fp;
	uint32_t *hw;

	fpc->inst_offset = fp->insn_len;
	fpc->have_const = 0;
	grow_insns(fpc, 4);
	hw = &fp->insn[fpc->inst_offset];
	memset(hw, 0, sizeof(uint32_t) * 4);

	if (insn.op == NVFX_FP_OP_OPCODE_KIL)
		fp->fp_control |= NV30_3D_FP_CONTROL_USES_KIL;
	hw[0] |= (insn.op << NVFX_FP_OP_OPCODE_SHIFT);
	hw[0] |= (insn.mask << NVFX_FP_OP_OUTMASK_SHIFT);
	hw[2] |= (insn.scale << NVFX_FP_OP_DST_SCALE_SHIFT);

	if (insn.sat)
		hw[0] |= NVFX_FP_OP_OUT_SAT;

	if (insn.cc_update)
		hw[0] |= NVFX_FP_OP_COND_WRITE_ENABLE;
	hw[1] |= (insn.cc_test << NVFX_FP_OP_COND_SHIFT);
	hw[1] |= ((insn.cc_swz[0] << NVFX_FP_OP_COND_SWZ_X_SHIFT) |
		  (insn.cc_swz[1] << NVFX_FP_OP_COND_SWZ_Y_SHIFT) |
		  (insn.cc_swz[2] << NVFX_FP_OP_COND_SWZ_Z_SHIFT) |
		  (insn.cc_swz[3] << NVFX_FP_OP_COND_SWZ_W_SHIFT));

	if(insn.unit >= 0)
	{
		hw[0] |= (insn.unit << NVFX_FP_OP_TEX_UNIT_SHIFT);
		fp->samplers |= (1 << insn.unit);
	}

	emit_dst(fpc, insn.dst);
	emit_src(fpc, 0, insn.src[0]);
	emit_src(fpc, 1, insn.src[1]);
	emit_src(fpc, 2, insn.src[2]);
}

#define arith(s,o,d,m,s0,s1,s2) \
       nvfx_insn((s), NVFX_FP_OP_OPCODE_##o, -1, \
                       (d), (m), (s0), (s1), (s2))

#define tex(s,o,u,d,m,s0,s1,s2) \
	nvfx_insn((s), NVFX_FP_OP_OPCODE_##o, (u), \
                   (d), (m), (s0), none, none)

/* IF src.x != 0, as TGSI specifies */
static void
nv40_fp_if(struct nvfx_fpc *fpc, struct nvfx_src src)
{
	const struct nvfx_src none = nvfx_src(nvfx_reg(NVFXSR_NONE, 0));
	struct nvfx_insn insn = arith(0, MOV, none.reg, NVFX_FP_MASK_X, src, none, none);
	uint32_t *hw;
	insn.cc_update = 1;
	nvfx_fp_emit(fpc, insn);

	fpc->inst_offset = fpc->fp->insn_len;
	grow_insns(fpc, 4);
	hw = &fpc->fp->insn[fpc->inst_offset];
	/* I really wonder why fp16 precision is used. Presumably the hardware ignores it? */
	hw[0] = (NV40_FP_OP_BRA_OPCODE_IF << NVFX_FP_OP_OPCODE_SHIFT) |
		NV40_FP_OP_OUT_NONE |
		(NVFX_FP_PRECISION_FP16 << NVFX_FP_OP_PRECISION_SHIFT);
	/* Use .xxxx swizzle so that we check only src[0].x*/
	hw[1] = (0 << NVFX_FP_OP_COND_SWZ_X_SHIFT) |
			(0 << NVFX_FP_OP_COND_SWZ_Y_SHIFT) |
			(0 << NVFX_FP_OP_COND_SWZ_Z_SHIFT) |
			(0 << NVFX_FP_OP_COND_SWZ_W_SHIFT) |
			(NVFX_FP_OP_COND_NE << NVFX_FP_OP_COND_SHIFT);
	hw[2] = 0; /* | NV40_FP_OP_OPCODE_IS_BRANCH | else_offset */
	hw[3] = 0; /* | endif_offset */
	util_dynarray_append(&fpc->if_stack, unsigned, fpc->inst_offset);
}

/* IF src.x != 0, as TGSI specifies */
static void
nv40_fp_cal(struct nvfx_fpc *fpc, unsigned target)
{
        struct nvfx_relocation reloc;
        uint32_t *hw;
        fpc->inst_offset = fpc->fp->insn_len;
        grow_insns(fpc, 4);
        hw = &fpc->fp->insn[fpc->inst_offset];
        /* I really wonder why fp16 precision is used. Presumably the hardware ignores it? */
        hw[0] = (NV40_FP_OP_BRA_OPCODE_CAL << NVFX_FP_OP_OPCODE_SHIFT);
        /* Use .xxxx swizzle so that we check only src[0].x*/
        hw[1] = (NVFX_SWZ_IDENTITY << NVFX_FP_OP_COND_SWZ_ALL_SHIFT) |
                        (NVFX_FP_OP_COND_TR << NVFX_FP_OP_COND_SHIFT);
        hw[2] = NV40_FP_OP_OPCODE_IS_BRANCH; /* | call_offset */
        hw[3] = 0;
        reloc.target = target;
        reloc.location = fpc->inst_offset + 2;
        util_dynarray_append(&fpc->label_relocs, struct nvfx_relocation, reloc);
}

static void
nv40_fp_ret(struct nvfx_fpc *fpc)
{
	uint32_t *hw;
	fpc->inst_offset = fpc->fp->insn_len;
	grow_insns(fpc, 4);
	hw = &fpc->fp->insn[fpc->inst_offset];
	/* I really wonder why fp16 precision is used. Presumably the hardware ignores it? */
	hw[0] = (NV40_FP_OP_BRA_OPCODE_RET << NVFX_FP_OP_OPCODE_SHIFT);
	/* Use .xxxx swizzle so that we check only src[0].x*/
	hw[1] = (NVFX_SWZ_IDENTITY << NVFX_FP_OP_COND_SWZ_ALL_SHIFT) |
			(NVFX_FP_OP_COND_TR << NVFX_FP_OP_COND_SHIFT);
	hw[2] = NV40_FP_OP_OPCODE_IS_BRANCH; /* | call_offset */
	hw[3] = 0;
}

static void
nv40_fp_rep(struct nvfx_fpc *fpc, unsigned count, unsigned target)
{
        struct nvfx_relocation reloc;
        uint32_t *hw;
        fpc->inst_offset = fpc->fp->insn_len;
        grow_insns(fpc, 4);
        hw = &fpc->fp->insn[fpc->inst_offset];
        /* I really wonder why fp16 precision is used. Presumably the hardware ignores it? */
        hw[0] = (NV40_FP_OP_BRA_OPCODE_REP << NVFX_FP_OP_OPCODE_SHIFT) |
                        NV40_FP_OP_OUT_NONE |
                        (NVFX_FP_PRECISION_FP16 << NVFX_FP_OP_PRECISION_SHIFT);
        /* Use .xxxx swizzle so that we check only src[0].x*/
        hw[1] = (NVFX_SWZ_IDENTITY << NVFX_FP_OP_COND_SWZ_ALL_SHIFT) |
                        (NVFX_FP_OP_COND_TR << NVFX_FP_OP_COND_SHIFT);
        hw[2] = NV40_FP_OP_OPCODE_IS_BRANCH |
                        (count << NV40_FP_OP_REP_COUNT1_SHIFT) |
                        (count << NV40_FP_OP_REP_COUNT2_SHIFT) |
                        (count << NV40_FP_OP_REP_COUNT3_SHIFT);
        hw[3] = 0; /* | end_offset */
        reloc.target = target;
        reloc.location = fpc->inst_offset + 3;
        util_dynarray_append(&fpc->label_relocs, struct nvfx_relocation, reloc);
        //util_dynarray_append(&fpc->loop_stack, unsigned, target);
}

/* warning: this only works forward, and probably only if not inside any IF */
static void
nv40_fp_bra(struct nvfx_fpc *fpc, unsigned target)
{
        struct nvfx_relocation reloc;
        uint32_t *hw;
        fpc->inst_offset = fpc->fp->insn_len;
        grow_insns(fpc, 4);
        hw = &fpc->fp->insn[fpc->inst_offset];
        /* I really wonder why fp16 precision is used. Presumably the hardware ignores it? */
        hw[0] = (NV40_FP_OP_BRA_OPCODE_IF << NVFX_FP_OP_OPCODE_SHIFT) |
                NV40_FP_OP_OUT_NONE |
                (NVFX_FP_PRECISION_FP16 << NVFX_FP_OP_PRECISION_SHIFT);
        /* Use .xxxx swizzle so that we check only src[0].x*/
        hw[1] = (NVFX_SWZ_IDENTITY << NVFX_FP_OP_COND_SWZ_X_SHIFT) |
                        (NVFX_FP_OP_COND_FL << NVFX_FP_OP_COND_SHIFT);
        hw[2] = NV40_FP_OP_OPCODE_IS_BRANCH; /* | else_offset */
        hw[3] = 0; /* | endif_offset */
        reloc.target = target;
        reloc.location = fpc->inst_offset + 2;
        util_dynarray_append(&fpc->label_relocs, struct nvfx_relocation, reloc);
        reloc.target = target;
        reloc.location = fpc->inst_offset + 3;
        util_dynarray_append(&fpc->label_relocs, struct nvfx_relocation, reloc);
}

static void
nv40_fp_brk(struct nvfx_fpc *fpc)
{
	uint32_t *hw;
	fpc->inst_offset = fpc->fp->insn_len;
	grow_insns(fpc, 4);
	hw = &fpc->fp->insn[fpc->inst_offset];
	/* I really wonder why fp16 precision is used. Presumably the hardware ignores it? */
	hw[0] = (NV40_FP_OP_BRA_OPCODE_BRK << NVFX_FP_OP_OPCODE_SHIFT) |
		NV40_FP_OP_OUT_NONE;
	/* Use .xxxx swizzle so that we check only src[0].x*/
	hw[1] = (NVFX_SWZ_IDENTITY << NVFX_FP_OP_COND_SWZ_X_SHIFT) |
			(NVFX_FP_OP_COND_TR << NVFX_FP_OP_COND_SHIFT);
	hw[2] = NV40_FP_OP_OPCODE_IS_BRANCH;
	hw[3] = 0;
}

static INLINE struct nvfx_src
tgsi_src(struct nvfx_fpc *fpc, const struct tgsi_full_src_register *fsrc)
{
	struct nvfx_src src;

	switch (fsrc->Register.File) {
	case TGSI_FILE_INPUT:
		if(fpc->pfp->info.input_semantic_name[fsrc->Register.Index] == TGSI_SEMANTIC_POSITION) {
			assert(fpc->pfp->info.input_semantic_index[fsrc->Register.Index] == 0);
			src.reg = nvfx_reg(NVFXSR_INPUT, NVFX_FP_OP_INPUT_SRC_POSITION);
		} else if(fpc->pfp->info.input_semantic_name[fsrc->Register.Index] == TGSI_SEMANTIC_COLOR) {
			if(fpc->pfp->info.input_semantic_index[fsrc->Register.Index] == 0)
				src.reg = nvfx_reg(NVFXSR_INPUT, NVFX_FP_OP_INPUT_SRC_COL0);
			else if(fpc->pfp->info.input_semantic_index[fsrc->Register.Index] == 1)
				src.reg = nvfx_reg(NVFXSR_INPUT, NVFX_FP_OP_INPUT_SRC_COL1);
			else
				assert(0);
		} else if(fpc->pfp->info.input_semantic_name[fsrc->Register.Index] == TGSI_SEMANTIC_FOG) {
			assert(fpc->pfp->info.input_semantic_index[fsrc->Register.Index] == 0);
			src.reg = nvfx_reg(NVFXSR_INPUT, NVFX_FP_OP_INPUT_SRC_FOGC);
		} else if(fpc->pfp->info.input_semantic_name[fsrc->Register.Index] == TGSI_SEMANTIC_FACE) {
			/* TODO: check this has the correct values */
			/* XXX: what do we do for nv30 here (assuming it lacks facing)?!  */
			assert(fpc->pfp->info.input_semantic_index[fsrc->Register.Index] == 0);
			src.reg = nvfx_reg(NVFXSR_INPUT, NV40_FP_OP_INPUT_SRC_FACING);
		} else {
			assert(fpc->pfp->info.input_semantic_name[fsrc->Register.Index] == TGSI_SEMANTIC_GENERIC);
			src.reg = nvfx_reg(NVFXSR_RELOCATED, fpc->generic_to_slot[fpc->pfp->info.input_semantic_index[fsrc->Register.Index]]);
		}
		break;
	case TGSI_FILE_CONSTANT:
		src.reg = nvfx_reg(NVFXSR_CONST, fsrc->Register.Index);
		break;
	case TGSI_FILE_IMMEDIATE:
		assert(fsrc->Register.Index < fpc->nr_imm);
		src.reg = fpc->r_imm[fsrc->Register.Index];
		break;
	case TGSI_FILE_TEMPORARY:
		src.reg = fpc->r_temp[fsrc->Register.Index];
		break;
	/* NV40 fragprog result regs are just temps, so this is simple */
	case TGSI_FILE_OUTPUT:
		src.reg = fpc->r_result[fsrc->Register.Index];
		break;
	default:
		NOUVEAU_ERR("bad src file\n");
		src.reg.index = 0;
		src.reg.type = 0;
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
	return src;
}

static INLINE struct nvfx_reg
tgsi_dst(struct nvfx_fpc *fpc, const struct tgsi_full_dst_register *fdst) {
	switch (fdst->Register.File) {
	case TGSI_FILE_OUTPUT:
		return fpc->r_result[fdst->Register.Index];
	case TGSI_FILE_TEMPORARY:
		return fpc->r_temp[fdst->Register.Index];
	case TGSI_FILE_NULL:
		return nvfx_reg(NVFXSR_NONE, 0);
	default:
		NOUVEAU_ERR("bad dst file %d\n", fdst->Register.File);
		return nvfx_reg(NVFXSR_NONE, 0);
	}
}

static INLINE int
tgsi_mask(uint tgsi)
{
	int mask = 0;

	if (tgsi & TGSI_WRITEMASK_X) mask |= NVFX_FP_MASK_X;
	if (tgsi & TGSI_WRITEMASK_Y) mask |= NVFX_FP_MASK_Y;
	if (tgsi & TGSI_WRITEMASK_Z) mask |= NVFX_FP_MASK_Z;
	if (tgsi & TGSI_WRITEMASK_W) mask |= NVFX_FP_MASK_W;
	return mask;
}

static boolean
nvfx_fragprog_parse_instruction(struct nvfx_context* nvfx, struct nvfx_fpc *fpc,
				const struct tgsi_full_instruction *finst)
{
	const struct nvfx_src none = nvfx_src(nvfx_reg(NVFXSR_NONE, 0));
	struct nvfx_insn insn;
	struct nvfx_src src[3], tmp;
	struct nvfx_reg dst;
	int mask, sat, unit = 0;
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
			if(fpc->pfp->info.input_semantic_name[fsrc->Register.Index] == TGSI_SEMANTIC_FOG && (0
					|| fsrc->Register.SwizzleX == PIPE_SWIZZLE_ALPHA
					|| fsrc->Register.SwizzleY == PIPE_SWIZZLE_ALPHA
					|| fsrc->Register.SwizzleZ == PIPE_SWIZZLE_ALPHA
					|| fsrc->Register.SwizzleW == PIPE_SWIZZLE_ALPHA
					)) {
				/* hardware puts 0 in fogcoord.w, but GL/Gallium want 1 there */
				struct nvfx_src addend = nvfx_src(nvfx_fp_imm(fpc, 0, 0, 0, 1));
				addend.swz[0] = fsrc->Register.SwizzleX;
				addend.swz[1] = fsrc->Register.SwizzleY;
				addend.swz[2] = fsrc->Register.SwizzleZ;
				addend.swz[3] = fsrc->Register.SwizzleW;
				src[i] = nvfx_src(temp(fpc));
				nvfx_fp_emit(fpc, arith(0, ADD, src[i].reg, NVFX_FP_MASK_ALL, tgsi_src(fpc, fsrc), addend, none));
			} else if (ai == -1 || ai == fsrc->Register.Index) {
				ai = fsrc->Register.Index;
				src[i] = tgsi_src(fpc, fsrc);
			} else {
				src[i] = nvfx_src(temp(fpc));
				nvfx_fp_emit(fpc, arith(0, MOV, src[i].reg, NVFX_FP_MASK_ALL, tgsi_src(fpc, fsrc), none, none));
			}
			break;
		case TGSI_FILE_CONSTANT:
			if ((ci == -1 && ii == -1) ||
			    ci == fsrc->Register.Index) {
				ci = fsrc->Register.Index;
				src[i] = tgsi_src(fpc, fsrc);
			} else {
				src[i] = nvfx_src(temp(fpc));
				nvfx_fp_emit(fpc, arith(0, MOV, src[i].reg, NVFX_FP_MASK_ALL, tgsi_src(fpc, fsrc), none, none));
			}
			break;
		case TGSI_FILE_IMMEDIATE:
			if ((ci == -1 && ii == -1) ||
			    ii == fsrc->Register.Index) {
				ii = fsrc->Register.Index;
				src[i] = tgsi_src(fpc, fsrc);
			} else {
				src[i] = nvfx_src(temp(fpc));
				nvfx_fp_emit(fpc, arith(0, MOV, src[i].reg, NVFX_FP_MASK_ALL, tgsi_src(fpc, fsrc), none, none));
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
		nvfx_fp_emit(fpc, arith(sat, MOV, dst, mask, abs(src[0]), none, none));
		break;
	case TGSI_OPCODE_ADD:
		nvfx_fp_emit(fpc, arith(sat, ADD, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_CMP:
		insn = arith(0, MOV, none.reg, mask, src[0], none, none);
		insn.cc_update = 1;
		nvfx_fp_emit(fpc, insn);

		insn = arith(sat, MOV, dst, mask, src[2], none, none);
		insn.cc_test = NVFX_COND_GE;
		nvfx_fp_emit(fpc, insn);

		insn = arith(sat, MOV, dst, mask, src[1], none, none);
		insn.cc_test = NVFX_COND_LT;
		nvfx_fp_emit(fpc, insn);
		break;
	case TGSI_OPCODE_COS:
		nvfx_fp_emit(fpc, arith(sat, COS, dst, mask, src[0], none, none));
		break;
	case TGSI_OPCODE_DDX:
		if (mask & (NVFX_FP_MASK_Z | NVFX_FP_MASK_W)) {
			tmp = nvfx_src(temp(fpc));
			nvfx_fp_emit(fpc, arith(sat, DDX, tmp.reg, NVFX_FP_MASK_X | NVFX_FP_MASK_Y, swz(src[0], Z, W, Z, W), none, none));
			nvfx_fp_emit(fpc, arith(0, MOV, tmp.reg, NVFX_FP_MASK_Z | NVFX_FP_MASK_W, swz(tmp, X, Y, X, Y), none, none));
			nvfx_fp_emit(fpc, arith(sat, DDX, tmp.reg, NVFX_FP_MASK_X | NVFX_FP_MASK_Y, src[0], none, none));
			nvfx_fp_emit(fpc, arith(0, MOV, dst, mask, tmp, none, none));
		} else {
			nvfx_fp_emit(fpc, arith(sat, DDX, dst, mask, src[0], none, none));
		}
		break;
	case TGSI_OPCODE_DDY:
		if (mask & (NVFX_FP_MASK_Z | NVFX_FP_MASK_W)) {
			tmp = nvfx_src(temp(fpc));
			nvfx_fp_emit(fpc, arith(sat, DDY, tmp.reg, NVFX_FP_MASK_X | NVFX_FP_MASK_Y, swz(src[0], Z, W, Z, W), none, none));
			nvfx_fp_emit(fpc, arith(0, MOV, tmp.reg, NVFX_FP_MASK_Z | NVFX_FP_MASK_W, swz(tmp, X, Y, X, Y), none, none));
			nvfx_fp_emit(fpc, arith(sat, DDY, tmp.reg, NVFX_FP_MASK_X | NVFX_FP_MASK_Y, src[0], none, none));
			nvfx_fp_emit(fpc, arith(0, MOV, dst, mask, tmp, none, none));
		} else {
			nvfx_fp_emit(fpc, arith(sat, DDY, dst, mask, src[0], none, none));
		}
		break;
	case TGSI_OPCODE_DP2:
		tmp = nvfx_src(temp(fpc));
		nvfx_fp_emit(fpc, arith(0, MUL, tmp.reg, NVFX_FP_MASK_X | NVFX_FP_MASK_Y, src[0], src[1], none));
		nvfx_fp_emit(fpc, arith(0, ADD, dst, mask, swz(tmp, X, X, X, X), swz(tmp, Y, Y, Y, Y), none));
		break;
	case TGSI_OPCODE_DP3:
		nvfx_fp_emit(fpc, arith(sat, DP3, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_DP4:
		nvfx_fp_emit(fpc, arith(sat, DP4, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_DPH:
		tmp = nvfx_src(temp(fpc));
		nvfx_fp_emit(fpc, arith(0, DP3, tmp.reg, NVFX_FP_MASK_X, src[0], src[1], none));
		nvfx_fp_emit(fpc, arith(sat, ADD, dst, mask, swz(tmp, X, X, X, X), swz(src[1], W, W, W, W), none));
		break;
	case TGSI_OPCODE_DST:
		nvfx_fp_emit(fpc, arith(sat, DST, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_EX2:
		nvfx_fp_emit(fpc, arith(sat, EX2, dst, mask, src[0], none, none));
		break;
	case TGSI_OPCODE_FLR:
		nvfx_fp_emit(fpc, arith(sat, FLR, dst, mask, src[0], none, none));
		break;
	case TGSI_OPCODE_FRC:
		nvfx_fp_emit(fpc, arith(sat, FRC, dst, mask, src[0], none, none));
		break;
	case TGSI_OPCODE_KILP:
		nvfx_fp_emit(fpc, arith(0, KIL, none.reg, 0, none, none, none));
		break;
	case TGSI_OPCODE_KIL:
		insn = arith(0, MOV, none.reg, NVFX_FP_MASK_ALL, src[0], none, none);
		insn.cc_update = 1;
		nvfx_fp_emit(fpc, insn);

		insn = arith(0, KIL, none.reg, 0, none, none, none);
		insn.cc_test = NVFX_COND_LT;
		nvfx_fp_emit(fpc, insn);
		break;
	case TGSI_OPCODE_LG2:
		nvfx_fp_emit(fpc, arith(sat, LG2, dst, mask, src[0], none, none));
		break;
	case TGSI_OPCODE_LIT:
		if(!nvfx->is_nv4x)
			nvfx_fp_emit(fpc, arith(sat, LIT_NV30, dst, mask, src[0], src[1], src[2]));
		else {
			/* we use FLT_MIN, so that log2 never gives -infinity, and thus multiplication by
			 * specular 0 always gives 0, so that ex2 gives 1, to satisfy the 0^0 = 1 requirement
			 *
			 * NOTE: if we start using half precision, we might need an fp16 FLT_MIN here instead
			 */
			struct nvfx_src maxs = nvfx_src(nvfx_fp_imm(fpc, 0, FLT_MIN, 0, 0));
			tmp = nvfx_src(temp(fpc));
			if (ci>= 0 || ii >= 0) {
				nvfx_fp_emit(fpc, arith(0, MOV, tmp.reg, NVFX_FP_MASK_X | NVFX_FP_MASK_Y, maxs, none, none));
				maxs = tmp;
			}
			nvfx_fp_emit(fpc, arith(0, MAX, tmp.reg, NVFX_FP_MASK_Y | NVFX_FP_MASK_W, swz(src[0], X, X, X, Y), swz(maxs, X, X, Y, Y), none));
			nvfx_fp_emit(fpc, arith(0, LG2, tmp.reg, NVFX_FP_MASK_W, swz(tmp, W, W, W, W), none, none));
			nvfx_fp_emit(fpc, arith(0, MUL, tmp.reg, NVFX_FP_MASK_W, swz(tmp, W, W, W, W), swz(src[0], W, W, W, W), none));
			nvfx_fp_emit(fpc, arith(sat, LITEX2_NV40, dst, mask, swz(tmp, Y, Y, W, W), none, none));
		}
		break;
	case TGSI_OPCODE_LRP:
		if(!nvfx->is_nv4x)
			nvfx_fp_emit(fpc, arith(sat, LRP_NV30, dst, mask, src[0], src[1], src[2]));
		else {
			tmp = nvfx_src(temp(fpc));
			nvfx_fp_emit(fpc, arith(0, MAD, tmp.reg, mask, neg(src[0]), src[2], src[2]));
			nvfx_fp_emit(fpc, arith(sat, MAD, dst, mask, src[0], src[1], tmp));
		}
		break;
	case TGSI_OPCODE_MAD:
		nvfx_fp_emit(fpc, arith(sat, MAD, dst, mask, src[0], src[1], src[2]));
		break;
	case TGSI_OPCODE_MAX:
		nvfx_fp_emit(fpc, arith(sat, MAX, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_MIN:
		nvfx_fp_emit(fpc, arith(sat, MIN, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_MOV:
		nvfx_fp_emit(fpc, arith(sat, MOV, dst, mask, src[0], none, none));
		break;
	case TGSI_OPCODE_MUL:
		nvfx_fp_emit(fpc, arith(sat, MUL, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_NOP:
		break;
	case TGSI_OPCODE_POW:
		if(!nvfx->is_nv4x)
			nvfx_fp_emit(fpc, arith(sat, POW_NV30, dst, mask, src[0], src[1], none));
		else {
			tmp = nvfx_src(temp(fpc));
			nvfx_fp_emit(fpc, arith(0, LG2, tmp.reg, NVFX_FP_MASK_X, swz(src[0], X, X, X, X), none, none));
			nvfx_fp_emit(fpc, arith(0, MUL, tmp.reg, NVFX_FP_MASK_X, swz(tmp, X, X, X, X), swz(src[1], X, X, X, X), none));
			nvfx_fp_emit(fpc, arith(sat, EX2, dst, mask, swz(tmp, X, X, X, X), none, none));
		}
		break;
	case TGSI_OPCODE_RCP:
		nvfx_fp_emit(fpc, arith(sat, RCP, dst, mask, src[0], none, none));
		break;
	case TGSI_OPCODE_RFL:
		if(!nvfx->is_nv4x)
			nvfx_fp_emit(fpc, arith(0, RFL_NV30, dst, mask, src[0], src[1], none));
		else {
			tmp = nvfx_src(temp(fpc));
			nvfx_fp_emit(fpc, arith(0, DP3, tmp.reg, NVFX_FP_MASK_X, src[0], src[0], none));
			nvfx_fp_emit(fpc, arith(0, DP3, tmp.reg, NVFX_FP_MASK_Y, src[0], src[1], none));
			insn = arith(0, DIV, tmp.reg, NVFX_FP_MASK_Z, swz(tmp, Y, Y, Y, Y), swz(tmp, X, X, X, X), none);
			insn.scale = NVFX_FP_OP_DST_SCALE_2X;
			nvfx_fp_emit(fpc, insn);
			nvfx_fp_emit(fpc, arith(sat, MAD, dst, mask, swz(tmp, Z, Z, Z, Z), src[0], neg(src[1])));
		}
		break;
	case TGSI_OPCODE_RSQ:
		if(!nvfx->is_nv4x)
			nvfx_fp_emit(fpc, arith(sat, RSQ_NV30, dst, mask, abs(swz(src[0], X, X, X, X)), none, none));
		else {
			tmp = nvfx_src(temp(fpc));
			insn = arith(0, LG2, tmp.reg, NVFX_FP_MASK_X, abs(swz(src[0], X, X, X, X)), none, none);
			insn.scale = NVFX_FP_OP_DST_SCALE_INV_2X;
			nvfx_fp_emit(fpc, insn);
			nvfx_fp_emit(fpc, arith(sat, EX2, dst, mask, neg(swz(tmp, X, X, X, X)), none, none));
		}
		break;
	case TGSI_OPCODE_SCS:
		/* avoid overwriting the source */
		if(src[0].swz[NVFX_SWZ_X] != NVFX_SWZ_X)
		{
			if (mask & NVFX_FP_MASK_X)
				nvfx_fp_emit(fpc, arith(sat, COS, dst, NVFX_FP_MASK_X, swz(src[0], X, X, X, X), none, none));
			if (mask & NVFX_FP_MASK_Y)
				nvfx_fp_emit(fpc, arith(sat, SIN, dst, NVFX_FP_MASK_Y, swz(src[0], X, X, X, X), none, none));
		}
		else
		{
			if (mask & NVFX_FP_MASK_Y)
				nvfx_fp_emit(fpc, arith(sat, SIN, dst, NVFX_FP_MASK_Y, swz(src[0], X, X, X, X), none, none));
			if (mask & NVFX_FP_MASK_X)
				nvfx_fp_emit(fpc, arith(sat, COS, dst, NVFX_FP_MASK_X, swz(src[0], X, X, X, X), none, none));
		}
		break;
	case TGSI_OPCODE_SEQ:
		nvfx_fp_emit(fpc, arith(sat, SEQ, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_SFL:
		nvfx_fp_emit(fpc, arith(sat, SFL, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_SGE:
		nvfx_fp_emit(fpc, arith(sat, SGE, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_SGT:
		nvfx_fp_emit(fpc, arith(sat, SGT, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_SIN:
		nvfx_fp_emit(fpc, arith(sat, SIN, dst, mask, src[0], none, none));
		break;
	case TGSI_OPCODE_SLE:
		nvfx_fp_emit(fpc, arith(sat, SLE, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_SLT:
		nvfx_fp_emit(fpc, arith(sat, SLT, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_SNE:
		nvfx_fp_emit(fpc, arith(sat, SNE, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_SSG:
	{
		struct nvfx_src minones = swz(nvfx_src(nvfx_fp_imm(fpc, -1, -1, -1, -1)), X, X, X, X);

		insn = arith(sat, MOV, dst, mask, src[0], none, none);
		insn.cc_update = 1;
		nvfx_fp_emit(fpc, insn);

		insn = arith(0, STR, dst, mask, none, none, none);
		insn.cc_test = NVFX_COND_GT;
		nvfx_fp_emit(fpc, insn);

		if(!sat) {
			insn = arith(0, MOV, dst, mask, minones, none, none);
			insn.cc_test = NVFX_COND_LT;
			nvfx_fp_emit(fpc, insn);
		}
		break;
	}
	case TGSI_OPCODE_STR:
		nvfx_fp_emit(fpc, arith(sat, STR, dst, mask, src[0], src[1], none));
		break;
	case TGSI_OPCODE_SUB:
		nvfx_fp_emit(fpc, arith(sat, ADD, dst, mask, src[0], neg(src[1]), none));
		break;
	case TGSI_OPCODE_TEX:
		nvfx_fp_emit(fpc, tex(sat, TEX, unit, dst, mask, src[0], none, none));
		break;
        case TGSI_OPCODE_TRUNC:
                tmp = nvfx_src(temp(fpc));
                insn = arith(0, MOV, none.reg, mask, src[0], none, none);
                insn.cc_update = 1;
                nvfx_fp_emit(fpc, insn);

                nvfx_fp_emit(fpc, arith(0, FLR, tmp.reg, mask, abs(src[0]), none, none));
                nvfx_fp_emit(fpc, arith(sat, MOV, dst, mask, tmp, none, none));

                insn = arith(sat, MOV, dst, mask, neg(tmp), none, none);
                insn.cc_test = NVFX_COND_LT;
                nvfx_fp_emit(fpc, insn);
                break;
        case TGSI_OPCODE_TXB:
                nvfx_fp_emit(fpc, tex(sat, TXB, unit, dst, mask, src[0], none, none));
                break;
        case TGSI_OPCODE_TXL:
                if(nvfx->is_nv4x)
                        nvfx_fp_emit(fpc, tex(sat, TXL_NV40, unit, dst, mask, src[0], none, none));
                else /* unsupported on nv30, use TEX and hope they like it */
                        nvfx_fp_emit(fpc, tex(sat, TEX, unit, dst, mask, src[0], none, none));
                break;
        case TGSI_OPCODE_TXP:
                nvfx_fp_emit(fpc, tex(sat, TXP, unit, dst, mask, src[0], none, none));
                break;
	case TGSI_OPCODE_XPD:
		tmp = nvfx_src(temp(fpc));
		nvfx_fp_emit(fpc, arith(0, MUL, tmp.reg, mask, swz(src[0], Z, X, Y, Y), swz(src[1], Y, Z, X, X), none));
		nvfx_fp_emit(fpc, arith(sat, MAD, dst, (mask & ~NVFX_FP_MASK_W), swz(src[0], Y, Z, X, X), swz(src[1], Z, X, Y, Y), neg(tmp)));
		break;

	case TGSI_OPCODE_IF:
		// MOVRC0 R31 (TR0.xyzw), R<src>:
		// IF (NE.xxxx) ELSE <else> END <end>
		if(!nvfx->use_nv4x)
			goto nv3x_cflow;
		nv40_fp_if(fpc, src[0]);
		break;

	case TGSI_OPCODE_ELSE:
	{
		uint32_t *hw;
		if(!nvfx->use_nv4x)
			goto nv3x_cflow;
		assert(util_dynarray_contains(&fpc->if_stack, unsigned));
		hw = &fpc->fp->insn[util_dynarray_top(&fpc->if_stack, unsigned)];
		hw[2] = NV40_FP_OP_OPCODE_IS_BRANCH | fpc->fp->insn_len;
		break;
	}

	case TGSI_OPCODE_ENDIF:
	{
		uint32_t *hw;
		if(!nvfx->use_nv4x)
			goto nv3x_cflow;
		assert(util_dynarray_contains(&fpc->if_stack, unsigned));
		hw = &fpc->fp->insn[util_dynarray_pop(&fpc->if_stack, unsigned)];
		if(!hw[2])
			hw[2] = NV40_FP_OP_OPCODE_IS_BRANCH | fpc->fp->insn_len;
		hw[3] = fpc->fp->insn_len;
		break;
	}

	case TGSI_OPCODE_BRA:
		/* This can in limited cases be implemented with an IF with the else and endif labels pointing to the target */
		/* no state tracker uses this, so don't implement this for now */
		assert(0);
		nv40_fp_bra(fpc, finst->Label.Label);
		break;

	case TGSI_OPCODE_BGNSUB:
	case TGSI_OPCODE_ENDSUB:
		/* nothing to do here */
		break;

	case TGSI_OPCODE_CAL:
		if(!nvfx->use_nv4x)
			goto nv3x_cflow;
		nv40_fp_cal(fpc, finst->Label.Label);
		break;

	case TGSI_OPCODE_RET:
		if(!nvfx->use_nv4x)
			goto nv3x_cflow;
		nv40_fp_ret(fpc);
		break;

	case TGSI_OPCODE_BGNLOOP:
		if(!nvfx->use_nv4x)
			goto nv3x_cflow;
		/* TODO: we should support using two nested REPs to allow a > 255 iteration count */
		nv40_fp_rep(fpc, 255, finst->Label.Label);
		break;

	case TGSI_OPCODE_ENDLOOP:
		break;

	case TGSI_OPCODE_BRK:
		if(!nvfx->use_nv4x)
			goto nv3x_cflow;
		nv40_fp_brk(fpc);
		break;

	case TGSI_OPCODE_CONT:
	{
		static int warned = 0;
		if(!warned) {
			NOUVEAU_ERR("Sorry, the continue keyword is not implemented: ignoring it.\n");
			warned = 1;
		}
		break;
	}

        default:
		NOUVEAU_ERR("invalid opcode %d\n", finst->Instruction.Opcode);
		return FALSE;
	}

out:
	release_temps(fpc);
	return TRUE;
nv3x_cflow:
	{
		static int warned = 0;
		if(!warned) {
			NOUVEAU_ERR(
					"Sorry, control flow instructions are not supported in hardware on nv3x: ignoring them\n"
					"If rendering is incorrect, try to disable GLSL support in the application.\n");
			warned = 1;
		}
	}
	goto out;
}

static boolean
nvfx_fragprog_parse_decl_output(struct nvfx_context* nvfx, struct nvfx_fpc *fpc,
				const struct tgsi_full_declaration *fdec)
{
	unsigned idx = fdec->Range.First;
	unsigned hw;

	switch (fdec->Semantic.Name) {
	case TGSI_SEMANTIC_POSITION:
		hw = 1;
		break;
	case TGSI_SEMANTIC_COLOR:
		hw = ~0;
		switch (fdec->Semantic.Index) {
		case 0: hw = 0; break;
		case 1: hw = 2; break;
		case 2: hw = 3; break;
		case 3: hw = 4; break;
		}
		if(hw > ((nvfx->use_nv4x) ? 4 : 2)) {
			NOUVEAU_ERR("bad rcol index\n");
			return FALSE;
		}
		break;
	default:
		NOUVEAU_ERR("bad output semantic\n");
		return FALSE;
	}

	fpc->r_result[idx] = nvfx_reg(NVFXSR_OUTPUT, hw);
	fpc->r_temps |= (1ULL << hw);
	return TRUE;
}

static boolean
nvfx_fragprog_prepare(struct nvfx_context* nvfx, struct nvfx_fpc *fpc)
{
	struct tgsi_parse_context p;
	int high_temp = -1, i;
	struct util_semantic_set set;
	unsigned num_texcoords = nvfx->use_nv4x ? 10 : 8;

	fpc->fp->num_slots = util_semantic_set_from_program_file(&set, fpc->pfp->pipe.tokens, TGSI_FILE_INPUT);
	if(fpc->fp->num_slots > num_texcoords)
		return FALSE;
	util_semantic_layout_from_set(fpc->fp->slot_to_generic, &set, 0, num_texcoords);
	util_semantic_table_from_layout(fpc->generic_to_slot, sizeof fpc->generic_to_slot,
                                        fpc->fp->slot_to_generic, 0, num_texcoords);

	memset(fpc->fp->slot_to_fp_input, 0xff, sizeof(fpc->fp->slot_to_fp_input));

	fpc->r_imm = CALLOC(fpc->pfp->info.immediate_count, sizeof(struct nvfx_reg));

	tgsi_parse_init(&p, fpc->pfp->pipe.tokens);
	while (!tgsi_parse_end_of_tokens(&p)) {
		const union tgsi_full_token *tok = &p.FullToken;

		tgsi_parse_token(&p);
		switch(tok->Token.Type) {
		case TGSI_TOKEN_TYPE_DECLARATION:
		{
			const struct tgsi_full_declaration *fdec;
			fdec = &p.FullToken.FullDeclaration;
			switch (fdec->Declaration.File) {
			case TGSI_FILE_OUTPUT:
				if (!nvfx_fragprog_parse_decl_output(nvfx, fpc, fdec))
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

			imm = &p.FullToken.FullImmediate;
			assert(imm->Immediate.DataType == TGSI_IMM_FLOAT32);
			assert(fpc->nr_imm < fpc->pfp->info.immediate_count);

			fpc->r_imm[fpc->nr_imm++] = nvfx_fp_imm(fpc, imm->u[0].Float, imm->u[1].Float, imm->u[2].Float, imm->u[3].Float);
			break;
		}
		default:
			break;
		}
	}
	tgsi_parse_free(&p);

	if (++high_temp) {
		fpc->r_temp = CALLOC(high_temp, sizeof(struct nvfx_reg));
		for (i = 0; i < high_temp; i++)
			fpc->r_temp[i] = temp(fpc);
		fpc->r_temps_discard = 0ULL;
	}

	return TRUE;

out_err:
	if (fpc->r_temp) {
		FREE(fpc->r_temp);
		fpc->r_temp = NULL;
	}
	tgsi_parse_free(&p);
	return FALSE;
}

DEBUG_GET_ONCE_BOOL_OPTION(nvfx_dump_fp, "NVFX_DUMP_FP", FALSE)

static struct nvfx_fragment_program*
nvfx_fragprog_translate(struct nvfx_context *nvfx,
			struct nvfx_pipe_fragment_program *pfp,
			boolean emulate_sprite_flipping)
{
	struct tgsi_parse_context parse;
	struct nvfx_fpc *fpc = NULL;
	struct util_dynarray insns;
	struct nvfx_fragment_program* fp = NULL;
        const int min_size = 4096;

	fp = CALLOC_STRUCT(nvfx_fragment_program);
	if(!fp)
		goto out_err;

	fpc = CALLOC_STRUCT(nvfx_fpc);
	if (!fpc)
		goto out_err;

	fpc->max_temps = nvfx->use_nv4x ? 48 : 32;
	fpc->pfp = pfp;
	fpc->fp = fp;
	fpc->num_regs = 2;

	for (unsigned i = 0; i < pfp->info.num_properties; ++i) {
		if (pfp->info.properties[i].name == TGSI_PROPERTY_FS_COORD_ORIGIN) {
			if(pfp->info.properties[i].data[0])
				fp->coord_conventions |= NV30_3D_COORD_CONVENTIONS_ORIGIN_INVERTED;
		} else if (pfp->info.properties[i].name == TGSI_PROPERTY_FS_COORD_PIXEL_CENTER) {
			if(pfp->info.properties[i].data[0])
				fp->coord_conventions |= NV30_3D_COORD_CONVENTIONS_CENTER_INTEGER;
		}
	}

	if (!nvfx_fragprog_prepare(nvfx, fpc))
		goto out_err;

	tgsi_parse_init(&parse, pfp->pipe.tokens);
	util_dynarray_init(&insns);

	if(emulate_sprite_flipping)
	{
		struct nvfx_reg reg = temp(fpc);
		struct nvfx_src sprite_input = nvfx_src(nvfx_reg(NVFXSR_RELOCATED, fp->num_slots));
		struct nvfx_src imm = nvfx_src(nvfx_fp_imm(fpc, 1, -1, 0, 0));

		fpc->sprite_coord_temp = reg.index;
		fpc->r_temps_discard = 0ULL;
		nvfx_fp_emit(fpc, arith(0, MAD, reg, NVFX_FP_MASK_ALL, sprite_input, swz(imm, X, Y, X, X), swz(imm, Z, X, Z, Z)));
	}

	while (!tgsi_parse_end_of_tokens(&parse)) {
		tgsi_parse_token(&parse);

		switch (parse.FullToken.Token.Type) {
		case TGSI_TOKEN_TYPE_INSTRUCTION:
		{
			const struct tgsi_full_instruction *finst;

			util_dynarray_append(&insns, unsigned, fp->insn_len);
			finst = &parse.FullToken.FullInstruction;
			if (!nvfx_fragprog_parse_instruction(nvfx, fpc, finst))
				goto out_err;
		}
			break;
		default:
			break;
		}
	}
	util_dynarray_append(&insns, unsigned, fp->insn_len);

	for(unsigned i = 0; i < fpc->label_relocs.size; i += sizeof(struct nvfx_relocation))
	{
		struct nvfx_relocation* label_reloc = (struct nvfx_relocation*)((char*)fpc->label_relocs.data + i);
		fp->insn[label_reloc->location] |= ((unsigned*)insns.data)[label_reloc->target];
	}
	util_dynarray_fini(&insns);

	if(!nvfx->is_nv4x)
		fp->fp_control |= (fpc->num_regs-1)/2;
	else
		fp->fp_control |= fpc->num_regs << NV40_3D_FP_CONTROL_TEMP_COUNT__SHIFT;

	/* Terminate final instruction */
	if(fp->insn)
		fp->insn[fpc->inst_offset] |= 0x00000001;

	/* Append NOP + END instruction for branches to the end of the program */
	fpc->inst_offset = fp->insn_len;
	grow_insns(fpc, 4);
	fp->insn[fpc->inst_offset + 0] = 0x00000001;
	fp->insn[fpc->inst_offset + 1] = 0x00000000;
	fp->insn[fpc->inst_offset + 2] = 0x00000000;
	fp->insn[fpc->inst_offset + 3] = 0x00000000;

	if(debug_get_option_nvfx_dump_fp())
	{
		debug_printf("\n");
		tgsi_dump(pfp->pipe.tokens, 0);

		debug_printf("\n%s fragment program:\n", nvfx->is_nv4x ? "nv4x" : "nv3x");
		for (unsigned i = 0; i < fp->insn_len; i += 4)
			debug_printf("%3u: %08x %08x %08x %08x\n", i >> 2, fp->insn[i], fp->insn[i + 1], fp->insn[i + 2], fp->insn[i + 3]);
		debug_printf("\n");
	}

        fp->prog_size = (fp->insn_len * 4 + 63) & ~63;

        if(fp->prog_size >= min_size)
                fp->progs_per_bo = 1;
        else
                fp->progs_per_bo = min_size / fp->prog_size;
        fp->bo_prog_idx = fp->progs_per_bo - 1;

out:
	tgsi_parse_free(&parse);
	if(fpc)
	{
		if (fpc->r_temp)
			FREE(fpc->r_temp);
		util_dynarray_fini(&fpc->if_stack);
		util_dynarray_fini(&fpc->label_relocs);
		util_dynarray_fini(&fpc->imm_data);
		//util_dynarray_fini(&fpc->loop_stack);
		FREE(fpc);
	}
	return fp;

out_err:
	_debug_printf("Error: failed to compile this fragment program:\n");
	tgsi_dump(pfp->pipe.tokens, 0);

	if(fp)
	{
		FREE(fp);
		fp = NULL;
	}
	goto out;
}

static inline void
nvfx_fp_memcpy(void* dst, const void* src, size_t len)
{
#ifndef PIPE_ARCH_BIG_ENDIAN
	memcpy(dst, src, len);
#else
	size_t i;
	for(i = 0; i < len; i += 4) {
		uint32_t v = *(uint32_t*)((char*)src + i);
		*(uint32_t*)((char*)dst + i) = (v >> 16) | (v << 16);
	}
#endif
}

/* The hardware only supports immediate constants inside the fragment program,
 * and at least on nv30 doesn't support an indirect linkage table.
 *
 * Hence, we need to patch the fragment program itself both to update constants
 * and update linkage.
 *
 * Using a single fragment program would entail unacceptable stalls if the GPU is
 * already rendering with that fragment program.
 * Thus, we instead use a "rotating queue" of buffer objects, each of which is
 * packed with multiple versions of the same program.
 *
 * Whenever we need to patch something, we move to the next program and
 * patch it. If all buffer objects are in use by the GPU, we allocate another one,
 * expanding the queue.
 *
 * As an additional optimization, we record when all the programs have the
 * current input slot configuration, and at that point we stop patching inputs.
 * This happens, for instance, if a given fragment program is always used with
 * the same vertex program (i.e. always with GLSL), or if the layouts match
 * enough (non-GLSL).
 *
 * Note that instead of using multiple programs, we could push commands
 * on the FIFO to patch a single program: it's not fully clear which option is
 * faster, but my guess is that the current way is faster.
 *
 * We also track the previous slot assignments for each version and don't
 * patch if they are the same (this could perhaps be removed).
 */

void
nvfx_fragprog_validate(struct nvfx_context *nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	struct nouveau_grobj *eng3d = nvfx->screen->eng3d;
	struct nvfx_pipe_fragment_program *pfp = nvfx->fragprog;
	struct nvfx_vertex_program* vp;

	// TODO: the multiplication by point_quad_rasterization is probably superfluous
	unsigned sprite_coord_enable = nvfx->rasterizer->pipe.point_quad_rasterization * nvfx->rasterizer->pipe.sprite_coord_enable;

	boolean emulate_sprite_flipping = sprite_coord_enable && nvfx->rasterizer->pipe.sprite_coord_mode;
	unsigned key = emulate_sprite_flipping;
	struct nvfx_fragment_program* fp;

	fp = pfp->fps[key];
	if (!fp)
	{
		fp = nvfx_fragprog_translate(nvfx, pfp, emulate_sprite_flipping);

		if(!fp)
		{
			if(!nvfx->dummy_fs)
			{
				struct ureg_program *ureg = ureg_create( TGSI_PROCESSOR_FRAGMENT );
				if (ureg)
				{
					ureg_END( ureg );
					nvfx->dummy_fs = ureg_create_shader_and_destroy( ureg, &nvfx->pipe );
				}

				if(!nvfx->dummy_fs)
				{
					_debug_printf("Error: unable to create a dummy fragment shader: aborting.");
					abort();
				}
			}

			fp = nvfx_fragprog_translate(nvfx, nvfx->dummy_fs, FALSE);
			emulate_sprite_flipping = FALSE;

			if(!fp)
			{
				_debug_printf("Error: unable to compile even a dummy fragment shader: aborting.");
				abort();
			}
		}

		pfp->fps[key] = fp;
	}

	vp = nvfx->hw_vertprog;

	if (fp->last_vp_id != vp->id || fp->last_sprite_coord_enable != sprite_coord_enable) {
		int sprite_real_input = -1;
		int sprite_reloc_input;
		unsigned i;
		fp->last_vp_id = vp->id;
		fp->last_sprite_coord_enable = sprite_coord_enable;

		if(sprite_coord_enable)
		{
			sprite_real_input = vp->sprite_fp_input;
			if(sprite_real_input < 0)
			{
				unsigned used_texcoords = 0;
				for(unsigned i = 0; i < fp->num_slots; ++i) {
					unsigned generic = fp->slot_to_generic[i];
					if((generic < 32) && !((1 << generic) & sprite_coord_enable))
					{
						unsigned char slot_mask = vp->generic_to_fp_input[generic];
						if(slot_mask >= 0xf0)
							used_texcoords |= 1 << ((slot_mask & 0xf) - NVFX_FP_OP_INPUT_SRC_TC0);
					}
				}

				sprite_real_input = NVFX_FP_OP_INPUT_SRC_TC(__builtin_ctz(~used_texcoords));
			}

			fp->point_sprite_control |= (1 << (sprite_real_input - NVFX_FP_OP_INPUT_SRC_TC0 + 8));
		}
		else
			fp->point_sprite_control = 0;

		if(emulate_sprite_flipping)
		   sprite_reloc_input = 0;
		else
		   sprite_reloc_input = sprite_real_input;

		for(i = 0; i < fp->num_slots; ++i) {
			unsigned generic = fp->slot_to_generic[i];
			if((generic < 32) && ((1 << generic) & sprite_coord_enable))
			{
				if(fp->slot_to_fp_input[i] != sprite_reloc_input)
					goto update_slots;
			}
			else
			{
				unsigned char slot_mask = vp->generic_to_fp_input[generic];
				if((slot_mask >> 4) & (slot_mask ^ fp->slot_to_fp_input[i]))
					goto update_slots;
			}
		}

		if(emulate_sprite_flipping)
		{
			if(fp->slot_to_fp_input[fp->num_slots] != sprite_real_input)
				goto update_slots;
		}

		if(0)
		{
update_slots:
			/* optimization: we start updating from the slot we found the first difference in */
			for(; i < fp->num_slots; ++i)
			{
				unsigned generic = fp->slot_to_generic[i];
				if((generic < 32) && ((1 << generic) & sprite_coord_enable))
					fp->slot_to_fp_input[i] = sprite_reloc_input;
				else
					fp->slot_to_fp_input[i] = vp->generic_to_fp_input[generic] & 0xf;
			}

			fp->slot_to_fp_input[fp->num_slots] = sprite_real_input;

			if(nvfx->is_nv4x)
			{
				fp->or = 0;
				for(i = 0; i <= fp->num_slots; ++i) {
					unsigned fp_input = fp->slot_to_fp_input[i];
					if(fp_input == NVFX_FP_OP_INPUT_SRC_TC(8))
						fp->or |= (1 << 12);
					else if(fp_input == NVFX_FP_OP_INPUT_SRC_TC(9))
						fp->or |= (1 << 13);
					else if(fp_input >= NVFX_FP_OP_INPUT_SRC_TC(0) && fp_input <= NVFX_FP_OP_INPUT_SRC_TC(7))
						fp->or |= (1 << (fp_input - NVFX_FP_OP_INPUT_SRC_TC0 + 14));
				}
			}

			fp->progs_left_with_obsolete_slot_assignments = fp->progs;
			goto update;
		}
	}

	/* We must update constants even on "just" fragprog changes, because
	  * we don't check whether the current constant buffer matches the latest
	  * one bound to this fragment program.
	  * Doing such a check would likely be a pessimization.
	  */
	if ((nvfx->hw_fragprog != fp) || (nvfx->dirty & (NVFX_NEW_FRAGPROG | NVFX_NEW_FRAGCONST))) {
		int offset;
		uint32_t* fpmap;

update:
		++fp->bo_prog_idx;
		if(fp->bo_prog_idx >= fp->progs_per_bo)
		{
			if(fp->fpbo && !nouveau_bo_busy(fp->fpbo->next->bo, NOUVEAU_BO_WR))
			{
				fp->fpbo = fp->fpbo->next;
			}
			else
			{
				struct nvfx_fragment_program_bo* fpbo = os_malloc_aligned(sizeof(struct nvfx_fragment_program) + (fp->prog_size + 8) * fp->progs_per_bo, 16);
				uint8_t* map;
				uint8_t* buf;

				fpbo->slots = (unsigned char*)&fpbo->insn[(fp->prog_size) * fp->progs_per_bo];
				memset(fpbo->slots, 0, 8 * fp->progs_per_bo);
				if(fp->fpbo)
				{
					fpbo->next = fp->fpbo->next;
					fp->fpbo->next = fpbo;
				}
				else
					fpbo->next = fpbo;
				fp->fpbo = fpbo;
				fpbo->bo = 0;
				fp->progs += fp->progs_per_bo;
				fp->progs_left_with_obsolete_slot_assignments += fp->progs_per_bo;
				nouveau_bo_new(nvfx->screen->base.device, NOUVEAU_BO_VRAM | NOUVEAU_BO_MAP, 64, fp->prog_size * fp->progs_per_bo, &fpbo->bo);
				nouveau_bo_map(fpbo->bo, NOUVEAU_BO_NOSYNC);

				map = fpbo->bo->map;
				buf = (uint8_t*)fpbo->insn;
				for(unsigned i = 0; i < fp->progs_per_bo; ++i)
				{
					memcpy(buf, fp->insn, fp->insn_len * 4);
					nvfx_fp_memcpy(map, fp->insn, fp->insn_len * 4);
					map += fp->prog_size;
					buf += fp->prog_size;
				}
			}
			fp->bo_prog_idx = 0;
		}

		offset = fp->bo_prog_idx * fp->prog_size;
		fpmap = (uint32_t*)((char*)fp->fpbo->bo->map + offset);

		if(nvfx->constbuf[PIPE_SHADER_FRAGMENT]) {
			struct pipe_resource* constbuf = nvfx->constbuf[PIPE_SHADER_FRAGMENT];
			uint32_t* map = (uint32_t*)nvfx_buffer(constbuf)->data;
			uint32_t* fpmap = (uint32_t*)((char*)fp->fpbo->bo->map + offset);
			uint32_t* buf = (uint32_t*)((char*)fp->fpbo->insn + offset);
			int i;
			for (i = 0; i < fp->nr_consts; ++i) {
				unsigned off = fp->consts[i].offset;
				unsigned idx = fp->consts[i].index * 4;

				/* TODO: is checking a good idea? */
				if(memcmp(&buf[off], &map[idx], 4 * sizeof(uint32_t))) {
					memcpy(&buf[off], &map[idx], 4 * sizeof(uint32_t));
					nvfx_fp_memcpy(&fpmap[off], &map[idx], 4 * sizeof(uint32_t));
				}
			}
		}

		/* we only do this if we aren't sure that all program versions have the
		 * current slot assignments, otherwise we just update constants for speed
		 */
		if(fp->progs_left_with_obsolete_slot_assignments) {
			unsigned char* fpbo_slots = &fp->fpbo->slots[fp->bo_prog_idx * 8];
			/* also relocate sprite coord slot, if any */
			for(unsigned i = 0; i <= fp->num_slots; ++i) {
				unsigned value = fp->slot_to_fp_input[i];;
				if(value != fpbo_slots[i]) {
					unsigned* p;
					unsigned* begin = (unsigned*)fp->slot_relocations[i].data;
					unsigned* end = (unsigned*)((char*)fp->slot_relocations[i].data + fp->slot_relocations[i].size);
					//printf("fp %p reloc slot %u/%u: %u -> %u\n", fp, i, fp->num_slots, fpbo_slots[i], value);
					if(value == 0)
					{
						/* was relocated to an input, switch type to temporary */
						for(p = begin; p != end; ++p) {
							unsigned off = *p;
							unsigned dw = fp->insn[off];
							dw &=~ NVFX_FP_REG_TYPE_MASK;
							//printf("reloc_tmp at %x\n", off);
							nvfx_fp_memcpy(&fpmap[off], &dw, sizeof(dw));
						}
					} else {
						if(!fpbo_slots[i])
						{
							/* was relocated to a temporary, switch type to input */
							for(p= begin; p != end; ++p) {
								unsigned off = *p;
								unsigned dw = fp->insn[off];
								//printf("reloc_in at %x\n", off);
								dw |= NVFX_FP_REG_TYPE_INPUT << NVFX_FP_REG_TYPE_SHIFT;
								nvfx_fp_memcpy(&fpmap[off], &dw, sizeof(dw));
							}
						}

						/* set the correct input index */
						for(p = begin; p != end; ++p) {
							unsigned off = *p & ~3;
							unsigned dw = fp->insn[off];
							//printf("reloc&~3 at %x\n", off);
							dw = (dw & ~NVFX_FP_OP_INPUT_SRC_MASK) | (value << NVFX_FP_OP_INPUT_SRC_SHIFT);
							nvfx_fp_memcpy(&fpmap[off], &dw, sizeof(dw));
						}
					}
					fpbo_slots[i] = value;
				}
			}
			--fp->progs_left_with_obsolete_slot_assignments;
		}

		nvfx->hw_fragprog = fp;

		MARK_RING(chan, 8, 1);
		BEGIN_RING(chan, eng3d, NV30_3D_FP_ACTIVE_PROGRAM, 1);
		OUT_RELOC(chan, fp->fpbo->bo, offset, NOUVEAU_BO_VRAM |
			      NOUVEAU_BO_GART | NOUVEAU_BO_RD | NOUVEAU_BO_LOW |
			      NOUVEAU_BO_OR, NV30_3D_FP_ACTIVE_PROGRAM_DMA0,
			      NV30_3D_FP_ACTIVE_PROGRAM_DMA1);
		BEGIN_RING(chan, eng3d, NV30_3D_FP_CONTROL, 1);
		OUT_RING(chan, fp->fp_control);
		if(!nvfx->is_nv4x) {
			BEGIN_RING(chan, eng3d, NV30_3D_FP_REG_CONTROL, 1);
			OUT_RING(chan, (1<<16)|0x4);
			BEGIN_RING(chan, eng3d, NV30_3D_TEX_UNITS_ENABLE, 1);
			OUT_RING(chan, fp->samplers);
		}
	}

	{
		unsigned pointsprite_control = fp->point_sprite_control | nvfx->rasterizer->pipe.point_quad_rasterization;
		if(pointsprite_control != nvfx->hw_pointsprite_control)
		{
			BEGIN_RING(chan, eng3d, NV30_3D_POINT_SPRITE, 1);
			OUT_RING(chan, pointsprite_control);
			nvfx->hw_pointsprite_control = pointsprite_control;
		}
	}

	nvfx->relocs_needed &=~ NVFX_RELOCATE_FRAGPROG;
}

void
nvfx_fragprog_relocate(struct nvfx_context *nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	struct nvfx_fragment_program *fp = nvfx->hw_fragprog;
	struct nouveau_bo* bo = fp->fpbo->bo;
	int offset = fp->bo_prog_idx * fp->prog_size;
	unsigned fp_flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_RD; // TODO: GART?
	fp_flags |= NOUVEAU_BO_DUMMY;
	MARK_RING(chan, 2, 2);
	OUT_RELOC(chan, bo, RING_3D(NV30_3D_FP_ACTIVE_PROGRAM, 1), fp_flags, 0, 0);
	OUT_RELOC(chan, bo, offset, fp_flags | NOUVEAU_BO_LOW |
		      NOUVEAU_BO_OR, NV30_3D_FP_ACTIVE_PROGRAM_DMA0,
		      NV30_3D_FP_ACTIVE_PROGRAM_DMA1);
	nvfx->relocs_needed &=~ NVFX_RELOCATE_FRAGPROG;
}

void
nvfx_fragprog_destroy(struct nvfx_context *nvfx,
		      struct nvfx_fragment_program *fp)
{
	unsigned i;
	struct nvfx_fragment_program_bo* fpbo = fp->fpbo;
	if(fpbo)
	{
		do
		{
			struct nvfx_fragment_program_bo* next = fpbo->next;
			nouveau_bo_unmap(fpbo->bo);
			nouveau_bo_ref(0, &fpbo->bo);
			os_free_aligned(fpbo);
			fpbo = next;
		}
		while(fpbo != fp->fpbo);
	}

	for(i = 0; i < Elements(fp->slot_relocations); ++i)
		util_dynarray_fini(&fp->slot_relocations[i]);

	if (fp->insn_len)
		FREE(fp->insn);
}

static void *
nvfx_fp_state_create(struct pipe_context *pipe,
                     const struct pipe_shader_state *cso)
{
        struct nvfx_pipe_fragment_program *pfp;

        pfp = CALLOC(1, sizeof(struct nvfx_pipe_fragment_program));
        pfp->pipe.tokens = tgsi_dup_tokens(cso->tokens);

        tgsi_scan_shader(pfp->pipe.tokens, &pfp->info);

        return (void *)pfp;
}

static void
nvfx_fp_state_bind(struct pipe_context *pipe, void *hwcso)
{
        struct nvfx_context *nvfx = nvfx_context(pipe);

        nvfx->fragprog = hwcso;
        nvfx->dirty |= NVFX_NEW_FRAGPROG;
}

static void
nvfx_fp_state_delete(struct pipe_context *pipe, void *hwcso)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	struct nvfx_pipe_fragment_program *pfp = hwcso;
	unsigned i;

	for(i = 0; i < Elements(pfp->fps); ++i)
	{
		if(pfp->fps[i])
		{
			nvfx_fragprog_destroy(nvfx, pfp->fps[i]);
			FREE(pfp->fps[i]);
		}
	}

        FREE((void*)pfp->pipe.tokens);
        FREE(pfp);
}

void
nvfx_init_fragprog_functions(struct nvfx_context *nvfx)
{
        nvfx->pipe.create_fs_state = nvfx_fp_state_create;
        nvfx->pipe.bind_fs_state = nvfx_fp_state_bind;
        nvfx->pipe.delete_fs_state = nvfx_fp_state_delete;
}
