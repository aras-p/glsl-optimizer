#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"

#include "pipe/tgsi/exec/tgsi_token.h"
#include "pipe/tgsi/exec/tgsi_parse.h"

#include "nv40_context.h"
#include "nv40_dma.h"

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

static uint32_t
passthrough_fp_data[] = {
	0x01403e81, 0x1c9dc801, 0x0001c800, 0x3fe1c800
};

static struct nv40_fragment_program
passthrough_fp = {
	.pipe = NULL,
	.translated = TRUE,
	.insn = passthrough_fp_data,
	.insn_len = sizeof(passthrough_fp_data) / sizeof(uint32_t),
	.buffer = NULL,
	.uses_kil = 0,
	.num_regs = 2,
};

struct nv40_fpc {
	struct nv40_fragment_program *fp;

	uint attrib_map[PIPE_MAX_SHADER_INPUTS];

	int high_temp;
	int temp_temp_count;

	uint depth_id;
	uint colour_id;

	boolean inst_has_const;
	int     inst_const_id;
};

static INLINE struct nv40_sreg
nv40_sr_temp(struct nv40_fpc *fpc)
{
	int idx;

	idx  = fpc->temp_temp_count++;
	idx += fpc->high_temp + 1;
	return nv40_sr(0, NV40_FP_REG_TYPE_TEMP, idx);
}

#define arith(cc,s,o,d,m,s0,s1,s2) \
	nv40_fp_arith((cc), (s), NV40_FP_OP_OPCODE_##o, \
			(d), (m), (s0), (s1), (s2))
#define tex(cc,s,o,u,d,m,s0,s1,s2) \
	nv40_fp_tex((cc), (s), NV40_FP_OP_OPCODE_##o, (u), \
		    (d), (m), (s0), none, none)
#define temp(fpc) nv40_sr_temp((fpc))

static void
emit_src(struct nv40_fpc *fpc, uint32_t *hw, int pos, struct nv40_sreg src)
{
	uint32_t sr = 0;

	sr |= (src.type << NV40_FP_REG_TYPE_SHIFT);
	if (src.type == NV40_FP_REG_TYPE_INPUT) {
		hw[0] |= (src.index << NV40_FP_OP_INPUT_SRC_SHIFT);
	} else
	if (src.type == NV40_FP_REG_TYPE_CONST) {
		fpc->inst_has_const = TRUE;
	} else
	if (src.type == NV40_FP_REG_TYPE_TEMP) {
		sr |= (src.index << NV40_FP_REG_SRC_SHIFT);
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
emit_dst(struct nv40_fpc *fpc, uint32_t *hw, struct nv40_sreg dst)
{
	struct nv40_fragment_program *fp = fpc->fp;

	if (dst.output) {
		if (dst.index == 1) {
			fp->writes_depth = 1;
		} else {
			hw[0] |= NV40_FP_OP_UNK0_7;
		}
	} else {
		if (fp->num_regs < (dst.index + 1))
			fp->num_regs = dst.index + 1;
	}
	hw[0] |= (dst.index << NV40_FP_OP_OUT_REG_SHIFT);
}

static void
nv40_fp_arith(struct nv40_fpc *fpc, int sat, int op,
	      struct nv40_sreg dst, int mask,
	      struct nv40_sreg s0, struct nv40_sreg s1, struct nv40_sreg s2)
{
	struct nv40_fragment_program *fp = fpc->fp;
	uint32_t *hw = &fp->insn[fp->insn_len];

	fpc->inst_has_const = FALSE;

	if (op == NV40_FP_OP_OPCODE_KIL)
		fp->uses_kil = TRUE;
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

	emit_dst(fpc, hw, dst);
	emit_src(fpc, hw, 0, s0);
	emit_src(fpc, hw, 1, s1);
	emit_src(fpc, hw, 2, s2);

	fp->insn_len += 4;
	if (fpc->inst_has_const) {
		fp->consts[fp->num_consts].pipe_id = fpc->inst_const_id;
		fp->consts[fp->num_consts].hw_id = fp->insn_len;
		fp->num_consts++;
		fp->insn_len += 4;
	}
}

static void
nv40_fp_tex(struct nv40_fpc *fpc, int sat, int op, int unit,
	    struct nv40_sreg dst, int mask,
	    struct nv40_sreg s0, struct nv40_sreg s1, struct nv40_sreg s2)
{
	struct nv40_fragment_program *fp = fpc->fp;
	uint32_t *hw = &fp->insn[fp->insn_len];

	nv40_fp_arith(fpc, sat, op, dst, mask, s0, s1, s2);
	hw[0] |= (unit << NV40_FP_OP_TEX_UNIT_SHIFT);
}

static INLINE struct nv40_sreg
tgsi_src(struct nv40_fpc *fpc, const struct tgsi_full_src_register *fsrc)
{
	struct nv40_sreg src;
	uint type, index;

	switch (fsrc->SrcRegister.File) {
	case TGSI_FILE_INPUT:
		type   = NV40_FP_REG_TYPE_INPUT;
		index  = fpc->attrib_map[fsrc->SrcRegister.Index];
		break;
	case TGSI_FILE_CONSTANT:
		type   = NV40_FP_REG_TYPE_CONST;
		index  = fsrc->SrcRegister.Index;
		break;
	case TGSI_FILE_TEMPORARY:
		type   = NV40_FP_REG_TYPE_TEMP;
		index  = fsrc->SrcRegister.Index + 1;
		if (fpc->high_temp < index)
			fpc->high_temp = index;
		break;
	default:
		NOUVEAU_ERR("bad src file\n");
		break;
	}

	src = nv40_sr(0, type, index);
	src.abs = fsrc->SrcRegisterExtMod.Absolute;
	src.negate = fsrc->SrcRegister.Negate;
	src.swz[0] = fsrc->SrcRegister.SwizzleX;
	src.swz[1] = fsrc->SrcRegister.SwizzleY;
	src.swz[2] = fsrc->SrcRegister.SwizzleZ;
	src.swz[3] = fsrc->SrcRegister.SwizzleW;
	return src;
}

static INLINE struct nv40_sreg
tgsi_dst(struct nv40_fpc *fpc, const struct tgsi_full_dst_register *fdst) {
	int out, idx;

	switch (fdst->DstRegister.File) {
	case TGSI_FILE_OUTPUT:
		out = 1;
		if (fdst->DstRegister.Index == fpc->colour_id)
			idx = 0;
		else
			idx = 1;
		break;
	case TGSI_FILE_TEMPORARY:
		out = 0;
		idx = fdst->DstRegister.Index + 1;
		if (fpc->high_temp < idx)
			fpc->high_temp = idx;
		break;
	case TGSI_FILE_NULL:
		break;
	default:
		NOUVEAU_ERR("bad dst file %d\n", fdst->DstRegister.File);
		break;
	}

	return nv40_sr(out, NV40_FP_REG_TYPE_TEMP, idx);
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
nv40_fragprog_parse_instruction(struct nv40_fpc *fpc,
				const struct tgsi_full_instruction *finst)
{
	struct nv40_sreg src[3], dst, tmp;
	struct nv40_sreg none = nv40_sr(0, NV40_FP_REG_TYPE_INPUT, 0);
	int mask, sat, unit;
	int ai = -1, ci = -1;
	int i;

	if (finst->Instruction.Opcode == TGSI_OPCODE_RET)
		return TRUE;

	fpc->temp_temp_count = 0;
	for (i = 0; i < finst->Instruction.NumSrcRegs; i++) {
		const struct tgsi_full_src_register *fsrc;

		fsrc = &finst->FullSrcRegisters[i];
		if (fsrc->SrcRegister.File == TGSI_FILE_TEMPORARY) {
			src[i] = tgsi_src(fpc, fsrc);
		}
	}

	for (i = 0; i < finst->Instruction.NumSrcRegs; i++) {
		const struct tgsi_full_src_register *fsrc;

		fsrc = &finst->FullSrcRegisters[i];
		switch (fsrc->SrcRegister.File) {
		case TGSI_FILE_INPUT:
			if (ai == -1 || ai == fsrc->SrcRegister.Index) {
				ai = fsrc->SrcRegister.Index;
				src[i] = tgsi_src(fpc, fsrc);
			} else {
				NOUVEAU_MSG("extra src attr %d\n",
					 fsrc->SrcRegister.Index);
				src[i] = temp(fpc);
				arith(fpc, 0, MOV, src[i], MASK_ALL,
				      tgsi_src(fpc, fsrc), none, none);
			}
			break;
		case TGSI_FILE_CONSTANT:
			if (ci == -1 || ci == fsrc->SrcRegister.Index) {
				ci = fsrc->SrcRegister.Index;
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
			unit = fsrc->SrcRegister.Index;
			break;
		default:
			NOUVEAU_ERR("bad src file\n");
			return FALSE;
		}
	}

	dst  = tgsi_dst(fpc, &finst->FullDstRegisters[0]);
	mask = tgsi_mask(finst->FullDstRegisters[0].DstRegister.WriteMask);
	sat  = (finst->Instruction.Saturate == TGSI_SAT_ZERO_ONE);

	switch (finst->Instruction.Opcode) {
	case TGSI_OPCODE_ABS:
		arith(fpc, sat, MOV, dst, mask, abs(src[0]), none, none);
		break;
	case TGSI_OPCODE_ADD:
		arith(fpc, sat, ADD, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_CMP:
		tmp = temp(fpc);
		arith(fpc, sat, MOV, dst, mask, src[2], none, none);
		tmp.cc_update = 1;
		arith(fpc, 0, MOV, tmp, 0xf, src[0], none, none);
		dst.cc_test = NV40_VP_INST_COND_LT;
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
	case TGSI_OPCODE_KIL:
	case TGSI_OPCODE_KILP:
		/*XXX: Which is NV, which is ARB kil? ARB implemented here.
		 *XXX: Don't need temp, can update CC0 without writing dst
		 */
		tmp = temp(fpc);
		tmp.cc_update = 1;
		arith(fpc, 0, MOV, tmp, MASK_ALL, src[0], none, none);
		dst.cc_test = NV40_FP_OP_COND_LT;
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
		if (mask & MASK_X) {
			arith(fpc, sat, COS, dst, MASK_X,
			      swz(src[0], X, X, X, X), none, none);
		}
		if (mask & MASK_Y) {
			arith(fpc, sat, SIN, dst, MASK_Y,
			      swz(src[0], X, X, X, X), none, none);
		}
		break;
	case TGSI_OPCODE_SIN:
		arith(fpc, sat, SIN, dst, mask, src[0], none, none);
		break;
	case TGSI_OPCODE_SGE:
		arith(fpc, sat, SGE, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_SLT:
		arith(fpc, sat, SLT, dst, mask, src[0], src[1], none);
		break;
	case TGSI_OPCODE_SUB:
		arith(fpc, sat, ADD, dst, mask, src[0], neg(src[1]), none);
		break;
	case TGSI_OPCODE_TEX:
		if (finst->FullSrcRegisters[0].SrcRegisterExtSwz.ExtDivide ==
				TGSI_EXTSWIZZLE_W) {
			tex(fpc, sat, TXP, unit, dst, mask, src[0], none, none);
		} else
			tex(fpc, sat, TEX, unit, dst, mask, src[0], none, none);
		break;
	case TGSI_OPCODE_TXB:
		tex(fpc, sat, TXB, unit, dst, mask, src[0], none, none);
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
nv40_fragprog_parse_decl_attrib(struct nv40_fpc *fpc,
				const struct tgsi_full_declaration *fdec)
{
	int hw;

	switch (fdec->Semantic.SemanticName) {
	case TGSI_SEMANTIC_POSITION:
		hw = NV40_FP_OP_INPUT_SRC_POSITION;
		break;
	case TGSI_SEMANTIC_COLOR:
		if (fdec->Semantic.SemanticIndex == 0) {
			hw = NV40_FP_OP_INPUT_SRC_COL0;
		} else
		if (fdec->Semantic.SemanticIndex == 1) {
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
		if (fdec->Semantic.SemanticIndex <= 7) {
			hw = NV40_FP_OP_INPUT_SRC_TC(fdec->Semantic.
						     SemanticIndex);
		} else {
			NOUVEAU_ERR("bad generic semantic index\n");
			return FALSE;
		}
		break;
	default:
		NOUVEAU_ERR("bad input semantic\n");
		return FALSE;
	}

	fpc->attrib_map[fdec->u.DeclarationRange.First] = hw;
	return TRUE;
}

static boolean
nv40_fragprog_parse_decl_output(struct nv40_fpc *fpc,
				const struct tgsi_full_declaration *fdec)
{
	switch (fdec->Semantic.SemanticName) {
	case TGSI_SEMANTIC_POSITION:
		fpc->depth_id = fdec->u.DeclarationRange.First;
		break;
	case TGSI_SEMANTIC_COLOR:
		fpc->colour_id = fdec->u.DeclarationRange.First;
		break;
	default:
		NOUVEAU_ERR("bad output semantic\n");
		return FALSE;
	}

	return TRUE;
}

void
nv40_fragprog_translate(struct nv40_context *nv40,
			struct nv40_fragment_program *fp)
{
	struct tgsi_parse_context parse;
	struct nv40_fpc *fpc = NULL;
	int i;

	fpc = calloc(1, sizeof(struct nv40_fpc));
	if (!fpc)
		return;
	fp->insn = calloc(1, 128*4*sizeof(uint32_t));
	fpc->fp = fp;
	fpc->high_temp = -1;
	fp->num_regs = 2;

	tgsi_parse_init(&parse, fp->pipe->tokens);

	while (!tgsi_parse_end_of_tokens(&parse)) {
		tgsi_parse_token(&parse);

		switch (parse.FullToken.Token.Type) {
		case TGSI_TOKEN_TYPE_DECLARATION:
		{
			const struct tgsi_full_declaration *fdec;
			fdec = &parse.FullToken.FullDeclaration;
			switch (fdec->Declaration.File) {
			case TGSI_FILE_INPUT:
				if (!nv40_fragprog_parse_decl_attrib(fpc, fdec))
					goto out_err;
				break;
			case TGSI_FILE_OUTPUT:
				if (!nv40_fragprog_parse_decl_output(fpc, fdec))
					goto out_err;
				break;
			default:
				break;
			}
		}
			break;
		case TGSI_TOKEN_TYPE_IMMEDIATE:
			break;
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

	if (fpc->inst_has_const == FALSE)
		fp->insn[fp->insn_len - 4] |= 0x00000001;
	else
		fp->insn[fp->insn_len - 8] |= 0x00000001;
	fp->insn[fp->insn_len++]  = 0x00000001;

	fp->translated = TRUE;
	fp->on_hw = FALSE;
out_err:
	tgsi_parse_free(&parse);
	free(fpc);
}

void
nv40_fragprog_bind(struct nv40_context *nv40, struct nv40_fragment_program *fp)
{
	struct pipe_winsys *ws = nv40->pipe.winsys;
	uint32_t fp_control;

	if (!fp->translated) {
		NOUVEAU_ERR("fragprog invalid, using passthrough shader\n");
		fp = &passthrough_fp;
	}

	if (!fp->on_hw) {
		if (!fp->buffer)
			fp->buffer = ws->buffer_create(ws, 0x100);

		nv40->pipe.winsys->buffer_data(nv40->pipe.winsys, fp->buffer,
					       fp->insn_len * sizeof(uint32_t),
					       fp->insn,
					       PIPE_BUFFER_USAGE_PIXEL);
		fp->on_hw = TRUE;
	}

	fp_control = fp->num_regs << NV40TCL_FP_CONTROL_TEMP_COUNT_SHIFT;
	if (fp->uses_kil)
		fp_control |= NV40TCL_FP_CONTROL_KIL;
	if (fp->writes_depth)
		fp_control |= 0xe;

	BEGIN_RING(curie, NV40TCL_FP_ADDRESS, 1);
	OUT_RELOC (fp->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_GART |
		   NOUVEAU_BO_RD | NOUVEAU_BO_LOW | NOUVEAU_BO_OR,
		   NV40TCL_FP_ADDRESS_DMA0, NV40TCL_FP_ADDRESS_DMA1);
	BEGIN_RING(curie, NV40TCL_FP_CONTROL, 1);
	OUT_RING  (fp_control);

	nv40->fragprog.active_fp = fp;
}

