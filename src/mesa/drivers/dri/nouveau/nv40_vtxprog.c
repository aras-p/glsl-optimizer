#include "glheader.h"
#include "macros.h"
#include "enums.h"
#include "program.h"
#include "program_instruction.h"

#include "nouveau_reg.h"
#include "nouveau_shader.h"
#include "nouveau_msg.h"

#include "nv40_reg.h"

/* TODO:
 *  - Implement support for constants
 *  - Handle SWZ with 0/1 components and partial negate masks
 *  - Handle ARB_position_invarient
 *  - Relative register addressing
 *  - Implement any missing instructions
 */

static int t_dst_mask(int mask);

static int
alloc_hw_temp(nouveau_vertex_program *vp)
{
	return nvsAllocIndex(vp->hwtemps_in_use, 64);
}

static void
free_hw_temp(nouveau_vertex_program *vp, int id)
{
	nvsBitClear(vp->hwtemps_in_use, id);
}

static int
alloc_temp(nouveau_vertex_program *vp)
{
	int idx;

	idx = nvsAllocIndex(vp->temps_in_use, 64);
	if (!idx)
		return -1;

	vp->temps[idx].file  = HW_TEMP;
	vp->temps[idx].hw_id = -1;
	vp->temps[idx].ref   = -1;

	return idx;
}

static void
free_temp(nouveau_vertex_program *vp, nouveau_srcreg *temp)
{
	if (!temp) return;

	if (vp->temps[temp->idx].hw_id != -1)
		free_hw_temp(vp, vp->temps[temp->idx].hw_id);
	nvsBitClear(vp->temps_in_use, temp->idx);
}

static void
make_srcreg(nouveau_vertex_program *vp,
		nouveau_srcreg *src,
		nouveau_regtype type,
		int id)
{
	switch (type) {
	case HW_INPUT:
		src->hw  = &vp->inputs[id];
		src->idx = id;
		break;
	case HW_TEMP:
		src->hw  = &vp->temps[id];
		src->idx = id;
		break;
	case HW_CONST:
		//FIXME: TODO
		break;
	default:
		assert(0);
		break;
	}

	src->negate  = 0;
	src->swizzle = 0x1B /* 00011011 - XYZW */;
}

static void
make_dstreg(nouveau_vertex_program *vp,
		nouveau_dstreg *dest,
		nouveau_regtype type,
		int id)
{
	if (type == HW_TEMP && id == -1)
		dest->idx = alloc_temp(vp);
	else
		dest->idx = id;
	switch (type) {
	case HW_TEMP:
		dest->idx = id;
		if (dest->idx == -1)
			dest->idx = alloc_temp(vp);
		dest->hw = &vp->temps[dest->idx];
		break;
	case HW_OUTPUT:
		dest->hw  = NULL;
		dest->idx = id;
		break;
	default:
		assert(0);
		break;
	}

	dest->mask     = t_dst_mask(WRITEMASK_XYZW);
	dest->condup   = 0;
	dest->condreg  = 0;
	dest->condtest = NV40_VP_INST_COND_TR;
	dest->condswz  = 0x1B /* 00011011 - XYZW */;
}

static unsigned int
src_to_hw(nouveau_vertex_program *vp, nouveau_srcreg *src,
		unsigned int *is, unsigned int *cs)
{
	unsigned int hs = 0;

	if (!src) {
		/* unused sources seem to be INPUT swz XYZW, dont't know if this
		 * actually matters or not...
		 */
		hs |= (NV40_VP_SRC_REG_TYPE_INPUT << NV40_VP_SRC_REG_TYPE_SHIFT);
		hs |= (0x1B << NV40_VP_SRC_SWZ_ALL_SHIFT);
		return hs;
	}

	switch (src->hw->file) {
	case HW_INPUT:
		if (*is != -1) {
			fprintf(stderr, "multiple inputs detected... not good\n");
			return;
		}
		*is = src->hw->hw_id;
		hs |= (NV40_VP_SRC_REG_TYPE_INPUT << NV40_VP_SRC_REG_TYPE_SHIFT);
		break;
	case HW_CONST:
		if (*cs != -1) {
			fprintf(stderr, "multiple consts detected... not good\n");
			return;
		}
		*cs = src->hw->hw_id;
		hs |= (NV40_VP_SRC_REG_TYPE_CONST << NV40_VP_SRC_REG_TYPE_SHIFT);
		break;
	case HW_TEMP:
		if (src->hw->hw_id == -1) {
			fprintf(stderr, "read from unwritten temp!\n");
			return;
		}
		hs |= (NV40_VP_SRC_REG_TYPE_TEMP << NV40_VP_SRC_REG_TYPE_SHIFT) |
			(src->hw->hw_id << NV40_VP_SRC_TEMP_SRC_SHIFT);

		if (--src->hw->ref == 0)
			free_hw_temp(vp, src->hw->hw_id);
	}

	hs |= (src->swizzle << NV40_VP_SRC_SWZ_ALL_SHIFT);
	if (src->negate)
		hs |= NV40_VP_SRC_NEGATE;

	return hs;
}

static void
instruction_store(nouveau_vertex_program *vp, unsigned int inst[])
{
	if ((vp->inst_count+1) > vp->insns_alloced) {
		vp->insns = realloc(vp->insns, sizeof(unsigned int) * (vp->inst_count+1) * 4);
		vp->insns_alloced = vp->inst_count+1;
	}
	vp->insns[(vp->inst_count*4) + 0] = inst[0];
	vp->insns[(vp->inst_count*4) + 1] = inst[1];
	vp->insns[(vp->inst_count*4) + 2] = inst[2];
	vp->insns[(vp->inst_count*4) + 3] = inst[3];
	vp->inst_count++;
}

static void
emit_arith(nouveau_vertex_program *vp, int op,
		nouveau_dstreg *dest,
		nouveau_srcreg *src0,
		nouveau_srcreg *src1,
		nouveau_srcreg *src2,
		int flags)
{
	nouveau_regrec *hwdest = dest->hw;
	unsigned int hs0, hs1, hs2;
	unsigned int hop[4] = { 0, 0, 0, 0 };
	int insrc = -1, constsrc = -1;

	/* Calculate source reg state */
	hs0 = src_to_hw(vp, src0, &insrc, &constsrc);
	hs1 = src_to_hw(vp, src1, &insrc, &constsrc);
	hs2 = src_to_hw(vp, src2, &insrc, &constsrc);

	/* Append it to the instruction */
	hop[1] |= (((hs0 & NV40_VP_SRC0_HIGH_MASK) >> NV40_VP_SRC0_HIGH_SHIFT)
				<< NV40_VP_INST_SRC0H_SHIFT);
	hop[2] |= ((hs0 & NV40_VP_SRC0_LOW_MASK) << NV40_VP_INST_SRC0L_SHIFT) |
			   (hs1 << NV40_VP_INST_SRC1_SHIFT) |
			   (((hs2 & NV40_VP_SRC2_HIGH_MASK) >> NV40_VP_SRC2_HIGH_SHIFT)
				<< NV40_VP_INST_SRC2H_SHIFT);
	hop[3] |= (hs2 & NV40_VP_SRC2_LOW_MASK) << NV40_VP_INST_SRC2L_SHIFT;

	/* bits 127:96 */
	hop[0] |= (dest->condtest << NV40_VP_INST_COND_SHIFT) |
			  (dest->condswz  << NV40_VP_INST_COND_SWZ_ALL_SHIFT);
	if (dest->condtest != NV40_VP_INST_COND_TR)
		hop[0] |= NV40_VP_INST_COND_TEST_ENABLE;
	if (dest->condreg) hop[0] |= NV40_VP_INST_COND_REG_SELECT_1;
	if (dest->condup ) hop[0] |= NV40_VP_INST_COND_UPDATE_ENABLE;

	if (hwdest->file == HW_OUTPUT)
		hop[0] |= NV40_VP_INST0_UNK0;
	else {
		if (hwdest->hw_id == -1)
			hwdest->hw_id = alloc_hw_temp(vp);

		hop[0] = (hwdest->hw_id << NV40_VP_INST_DEST_TEMP_SHIFT);
		if (flags & NOUVEAU_OUT_ABS)
			hop[0] |= NV40_VP_INST_DEST_TEMP_ABS;

		nvsBitSet(vp->hwtemps_written, hwdest->hw_id);
		if (--hwdest->ref == 0)
			free_hw_temp(vp, hwdest->hw_id);
	}

	/* bits 95:64 */
	if (constsrc == -1) constsrc = 0;
	if (insrc    == -1) insrc    = 0;

	constsrc &= 0xFF;
	insrc    &= 0x0F;
	hop[1] |= (op       << NV40_VP_INST_OPCODE_SHIFT) |
			  (constsrc << NV40_VP_INST_CONST_SRC_SHIFT) |
			  (insrc    << NV40_VP_INST_INPUT_SRC_SHIFT);

	/* bits 31:0 */
	if (hwdest->file == HW_OUTPUT) {
		hop[3] |= (dest->mask | (hwdest->hw_id << NV40_VP_INST_DEST_SHIFT));
	} else {
		hop[3] |= (dest->mask | (NV40_VP_INST_DEST_TEMP << NV40_VP_INST_DEST_SHIFT));
	}
	hop[3] |= (0x3F << 7); /*FIXME: what is this?*/

	printf("0x%08x\n", hop[0]);
	printf("0x%08x\n", hop[1]);
	printf("0x%08x\n", hop[2]);
	printf("0x%08x\n", hop[3]);

	instruction_store(vp, hop);
}

static int
t_swizzle(GLuint swz)
{
	int x, y, z, w;
	x = GET_SWZ(swz, 0);
	y = GET_SWZ(swz, 1);
	z = GET_SWZ(swz, 2);
	w = GET_SWZ(swz, 3);

	if ((x<SWIZZLE_ZERO) &&
		(y<SWIZZLE_ZERO) &&
		(z<SWIZZLE_ZERO) &&
		(w<SWIZZLE_ZERO))
		return (x << 6) | (y << 4) | (z << 2) | w;
	return -1;
}

static void
t_src_reg(nouveau_vertex_program *vp, struct prog_src_register *src,
		nouveau_srcreg *ns)
{
	switch (src->File) {
	case PROGRAM_TEMPORARY:
		ns->hw = &vp->temps[src->Index];
		break;
	case PROGRAM_INPUT:
		ns->hw = &vp->inputs[src->Index];
		break;
	default:
		fprintf(stderr, "Unhandled source register file!\n");
		break;
	}

	ns->swizzle = t_swizzle(src->Swizzle);
	if ((src->NegateBase != 0xF && src->NegateBase != 0x0) ||
			ns->swizzle == -1) {
		WARN_ONCE("Unhandled source swizzle/negate, results will be incorrect\n");
		ns->swizzle = 0x1B; // 00 01 10 11 - XYZW
		ns->negate  = (src->NegateBase) ? 1 : 0;
	} else
		ns->negate  = (src->NegateBase) ? 1 : 0;

}

static int
t_dst_mask(int mask)
{
	int hwmask = 0;

	if (mask & WRITEMASK_X) hwmask |= NV40_VP_INST_WRITEMASK_X;
	if (mask & WRITEMASK_Y) hwmask |= NV40_VP_INST_WRITEMASK_Y;
	if (mask & WRITEMASK_Z) hwmask |= NV40_VP_INST_WRITEMASK_Z;
	if (mask & WRITEMASK_W) hwmask |= NV40_VP_INST_WRITEMASK_W;

	return hwmask;
}

static int
t_dst_index(int idx)
{
	int hwidx;

	switch (idx) {
	case VERT_RESULT_HPOS:
		return NV40_VP_INST_DEST_POS;
	case VERT_RESULT_COL0:
		return NV40_VP_INST_DEST_COL0;
	case VERT_RESULT_COL1:
		return NV40_VP_INST_DEST_COL1;
	case VERT_RESULT_FOGC:
		return NV40_VP_INST_DEST_FOGC;
	case VERT_RESULT_TEX0:
	case VERT_RESULT_TEX1:
	case VERT_RESULT_TEX2:
	case VERT_RESULT_TEX3:
	case VERT_RESULT_TEX4:
	case VERT_RESULT_TEX5:
	case VERT_RESULT_TEX6:
	case VERT_RESULT_TEX7:
		return NV40_VP_INST_DEST_TC(idx - VERT_RESULT_TEX0);
	case VERT_RESULT_PSIZ:
		return NV40_VP_INST_DEST_PSZ;
	case VERT_RESULT_BFC0:
		return NV40_VP_INST_DEST_BFC0;
	case VERT_RESULT_BFC1:
		return NV40_VP_INST_DEST_BFC1;
	default:
		fprintf(stderr, "Unknown result reg index!\n");
		return -1;
	}
}

static int
t_cond_test(GLuint test)
{
	switch(test) {
	case COND_GT: return NV40_VP_INST_COND_GT;
	case COND_EQ: return NV40_VP_INST_COND_EQ;
	case COND_LT: return NV40_VP_INST_COND_LT;
	case COND_GE: return NV40_VP_INST_COND_GE;
	case COND_LE: return NV40_VP_INST_COND_LE;
	case COND_NE: return NV40_VP_INST_COND_NE;
	case COND_TR: return NV40_VP_INST_COND_TR;
	case COND_FL: return NV40_VP_INST_COND_FL;
	default:
		WARN_ONCE("unknown CondMask!\n");
		return -1;
	}
}

#define ARITH_1OP(op) do { \
			t_src_reg(vp, &vpi->SrcReg[0], &src0); \
			emit_arith(vp, op, &dest, &src0, NULL, NULL, 0); \
} while(0);
#define ARITH_1OP_SCALAR(op) do { \
			t_src_reg(vp, &vpi->SrcReg[0], &src0); \
			emit_arith(vp, op, &dest, NULL, NULL, &src0, 0); \
} while(0);
#define ARITH_2OP(op) do { \
			t_src_reg(vp, &vpi->SrcReg[0], &src0); \
			t_src_reg(vp, &vpi->SrcReg[1], &src1); \
			emit_arith(vp, op, &dest, &src0, &src1, NULL, 0); \
} while(0);
#define ARITH_3OP(op) do { \
			t_src_reg(vp, &vpi->SrcReg[0], &src0); \
			t_src_reg(vp, &vpi->SrcReg[1], &src1); \
			t_src_reg(vp, &vpi->SrcReg[2], &src2); \
			emit_arith(vp, op, &dest, &src0, &src1, &src2, 0); \
} while(0);

static int
translate(nouveau_vertex_program *vp)
{
	struct vertex_program   *mvp = &vp->mesa_program;
	struct prog_instruction *vpi;

	
	for (vpi=mvp->Base.Instructions; vpi->Opcode!=OPCODE_END; vpi++) {
		nouveau_srcreg src0, src1, src2, sT0;
		nouveau_dstreg dest, dT0;

		switch (vpi->DstReg.File) {
		case PROGRAM_OUTPUT:
			make_dstreg(vp, &dest, HW_OUTPUT, t_dst_index(vpi->DstReg.Index));
			break;
		case PROGRAM_TEMPORARY:
			make_dstreg(vp, &dest, HW_TEMP, vpi->DstReg.Index);
			break;
		default:
			assert(0);
		}
		dest.mask     = t_dst_mask(vpi->DstReg.WriteMask);
		dest.condtest = t_cond_test(vpi->DstReg.CondMask);
		dest.condswz  = t_swizzle(vpi->DstReg.CondSwizzle);
		dest.condreg  = vpi->DstReg.CondSrc;

		switch (vpi->Opcode) {
	/* ARB_vertex_program requirements */
		case OPCODE_ABS:
			t_src_reg(vp, &vpi->SrcReg[0], &src0);
			emit_arith(vp, NV40_VP_INST_OP_MOV, &dest,
					&src0, NULL, NULL,
					NOUVEAU_OUT_ABS
					);
			break;
		case OPCODE_ADD:
			t_src_reg(vp, &vpi->SrcReg[0], &src0);
			t_src_reg(vp, &vpi->SrcReg[1], &src1);
			emit_arith(vp, NV40_VP_INST_OP_ADD,	&dest,
					&src0, NULL, &src1,
					0
					);
			break;
		case OPCODE_ARL:
			break;
		case OPCODE_DP3:
			ARITH_2OP(NV40_VP_INST_OP_DP3);
			break;
		case OPCODE_DP4:
			ARITH_2OP(NV40_VP_INST_OP_DP4);
			break;
		case OPCODE_DPH:
			ARITH_2OP(NV40_VP_INST_OP_DPH);
			break;
		case OPCODE_DST:
			ARITH_2OP(NV40_VP_INST_OP_DST);
			break;
		case OPCODE_EX2:
			ARITH_1OP_SCALAR(NV40_VP_INST_OP_EX2);
			break;
		case OPCODE_EXP:
			ARITH_1OP_SCALAR(NV40_VP_INST_OP_EXP);
			break;
		case OPCODE_FLR:
			ARITH_1OP(NV40_VP_INST_OP_FLR);
			break;
		case OPCODE_FRC:
			ARITH_1OP(NV40_VP_INST_OP_FRC);
			break;
		case OPCODE_LG2:
			ARITH_1OP_SCALAR(NV40_VP_INST_OP_LG2);
			break;
		case OPCODE_LIT:
			t_src_reg(vp, &vpi->SrcReg[0], &src0);
			t_src_reg(vp, &vpi->SrcReg[1], &src1);
			t_src_reg(vp, &vpi->SrcReg[2], &src2);
			emit_arith(vp, NV40_VP_INST_OP_LIT,	&dest,
					&src0, &src1, &src2,
					0
					);
			break;
		case OPCODE_LOG:
			ARITH_1OP_SCALAR(NV40_VP_INST_OP_LOG);
			break;
		case OPCODE_MAD:
			ARITH_3OP(NV40_VP_INST_OP_MAD);
			break;
		case OPCODE_MAX:
			ARITH_2OP(NV40_VP_INST_OP_MAX);
			break;
		case OPCODE_MIN:
			ARITH_2OP(NV40_VP_INST_OP_MIN);
			break;
		case OPCODE_MOV:
			ARITH_1OP(NV40_VP_INST_OP_MOV);
			break;
		case OPCODE_MUL:
			ARITH_2OP(NV40_VP_INST_OP_MOV);
			break;
		case OPCODE_POW:
			t_src_reg(vp, &vpi->SrcReg[0], &src0);
			t_src_reg(vp, &vpi->SrcReg[1], &src1);
			make_dstreg(vp, &dT0, HW_TEMP, -1);
			make_srcreg(vp, &sT0, HW_TEMP, dT0.idx);

			dT0.mask = t_dst_mask(WRITEMASK_X);
			emit_arith(vp, NV40_VP_INST_OP_LG2, &dT0,
					NULL, NULL, &src0,
					0);
			sT0.swizzle = 0x0; /* 00000000 - XXXX */
			emit_arith(vp, NV40_VP_INST_OP_MUL, &dT0,
					&sT0, &src1, NULL,
					0);
			emit_arith(vp, NV40_VP_INST_OP_EX2, &dest,
					NULL, NULL, &sT0,
					0);
			break;
		case OPCODE_RCP:
			ARITH_1OP_SCALAR(NV40_VP_INST_OP_RCP);
			break;
		case OPCODE_RSQ:
			ARITH_1OP_SCALAR(NV40_VP_INST_OP_RSQ);
			break;
		case OPCODE_SGE:
			ARITH_2OP(NV40_VP_INST_OP_SGE);
			break;
		case OPCODE_SLT:
			ARITH_2OP(NV40_VP_INST_OP_SLT);
			break;
		case OPCODE_SUB:
			t_src_reg(vp, &vpi->SrcReg[0], &src0);
			t_src_reg(vp, &vpi->SrcReg[1], &src1);
			src1.negate = !src1.negate;

			emit_arith(vp, NV40_VP_INST_OP_ADD,	&dest,
					&src0, NULL, &src1,
					0
					);
			break;
		case OPCODE_SWZ:
			ARITH_1OP(NV40_VP_INST_OP_MOV);
			break;

		case OPCODE_XPD:
			break;
	/* NV_vertex_program3 requirements */
		case OPCODE_SEQ:
			ARITH_2OP(NV40_VP_INST_OP_SEQ);
			break;
		case OPCODE_SFL:
			ARITH_2OP(NV40_VP_INST_OP_SFL);
			break;
		case OPCODE_SGT:
			ARITH_2OP(NV40_VP_INST_OP_SGT);
			break;
		case OPCODE_SLE:
			ARITH_2OP(NV40_VP_INST_OP_SLE);
			break;
		case OPCODE_SNE:
			ARITH_2OP(NV40_VP_INST_OP_SNE);
			break;
		case OPCODE_STR:
			ARITH_2OP(NV40_VP_INST_OP_STR);
			break;
		case OPCODE_SSG:
			ARITH_1OP(NV40_VP_INST_OP_SSG);
			break;
		case OPCODE_ARL_NV:
			break;
		case OPCODE_ARR:
			break;
		case OPCODE_ARA:
			break;
		case OPCODE_RCC:
			ARITH_1OP_SCALAR(NV40_VP_INST_OP_SSG);
			break;
		case OPCODE_BRA:
			break;
		case OPCODE_CAL:
			break;
		case OPCODE_RET:
			break;
		case OPCODE_PUSHA:
			break;
		case OPCODE_POPA:
			break;
		default:
			break;
		}
	}

	return 0;
}

/* Pre-init vertex program
 *  - Grab reference counts on temps
 *  - Where multiple inputs are used in a single instruction,
 *    emit instructions to move the extras into temps
 */
static int
init(nouveau_vertex_program *vp)
{
	struct vertex_program   *mvp = &vp->mesa_program;
	struct prog_instruction *vpi;
	int i;

	nvsRecInit(&vp->temps_in_use, 64);
	nvsRecInit(&vp->hwtemps_written, 64);
	nvsRecInit(&vp->hwtemps_in_use , 64);

	for (vpi=mvp->Base.Instructions; vpi->Opcode!=OPCODE_END; vpi++) {
		int in_done = 0;
		int in_idx;

		for (i=0;i<3;i++) {
			struct prog_src_register *src = &vpi->SrcReg[i];
			/*FIXME: does not handle relative addressing!*/
			int	idx = src->Index;

			switch (src->File) {
			case PROGRAM_TEMPORARY:
				vp->temps[idx].file  = HW_TEMP;
				vp->temps[idx].hw_id = -1;
				vp->temps[idx].ref++;
				nvsBitSet(vp->temps_in_use, idx);
				break;
			case PROGRAM_INPUT:
				if (vp->inputs[idx].file == HW_TEMP) {
					vp->inputs[idx].ref++;
					break;
				}

				if (!in_done || (in_idx == idx)) {
					vp->inputs[idx].file  = HW_INPUT;
					vp->inputs[idx].hw_id = idx;
					vp->inputs[idx].ref++;
					in_done = 1;
					in_idx  = idx;
				} else {
					vp->inputs[idx].file  = HW_TEMP;
					vp->inputs[idx].ref++;
				}
				break;
			default:
				break;
			}
		}

		switch (vpi->DstReg.File) {
		case PROGRAM_TEMPORARY:
			vp->temps[vpi->DstReg.Index].file  = HW_TEMP;
			vp->temps[vpi->DstReg.Index].hw_id = -1;
			vp->temps[vpi->DstReg.Index].ref++;
			nvsBitSet(vp->temps_in_use, vpi->DstReg.Index);
			break;
		default:
			break;
		}
	}

	/* Now we can move any inputs that need it into temps */
	for (i=0; i<14; i++) {
		if (vp->inputs[i].file == HW_TEMP) {
			nouveau_srcreg src;
			nouveau_dstreg dest;

			make_dstreg(vp, &dest, HW_TEMP , -1);
			make_srcreg(vp, &src , HW_INPUT,  i); 

			emit_arith(vp, NV40_VP_INST_OP_MOV, &dest,
					&src, NULL, NULL,
					0
					);

			vp->inputs[i].file  = HW_TEMP;
			vp->inputs[i].hw_id = dest.hw->hw_id;
		}
	}

	return 0;
}

int
nv40TranslateVertexProgram(nouveau_vertex_program *vp)
{
	int ret;

	ret = init(vp);
	if (ret)
		return ret;

	ret = translate(vp);
	if (ret)
		return ret;

	return 0;
}

int
main(int argc, char **argv)
{
	nouveau_vertex_program *vp  = calloc(1, sizeof(nouveau_vertex_program));
	struct vertex_program  *mvp = &vp->mesa_program;
	struct prog_instruction inst[3];

	/*
        "ADD t0, vertex.color, vertex.position;\n"
        "ADD result.position, t0, vertex.position;\n"
	*/

	inst[0].Opcode = OPCODE_ADD;
	inst[0].SrcReg[0].File       = PROGRAM_INPUT;
	inst[0].SrcReg[0].Index      = VERT_ATTRIB_COLOR0;
	inst[0].SrcReg[0].NegateBase = 0;
	inst[0].SrcReg[0].Swizzle    = MAKE_SWIZZLE4(0, 1, 2, 3);
	inst[0].SrcReg[1].File       = PROGRAM_INPUT;
	inst[0].SrcReg[1].Index      = VERT_ATTRIB_POS;
	inst[0].SrcReg[1].NegateBase = 0;
	inst[0].SrcReg[1].Swizzle    = MAKE_SWIZZLE4(0, 1, 2, 3);
	inst[0].SrcReg[2].File       = PROGRAM_UNDEFINED;
	inst[0].DstReg.File      = PROGRAM_TEMPORARY;
	inst[0].DstReg.Index     = 0;
	inst[0].DstReg.WriteMask = WRITEMASK_XYZW;

	inst[1].Opcode = OPCODE_ADD;
	inst[1].SrcReg[0].File       = PROGRAM_TEMPORARY;
	inst[1].SrcReg[0].Index      = 0;
	inst[1].SrcReg[0].NegateBase = 0;
	inst[1].SrcReg[0].Swizzle    = MAKE_SWIZZLE4(0, 1, 2, 3);
	inst[1].SrcReg[1].File       = PROGRAM_INPUT;
	inst[1].SrcReg[1].Index      = VERT_ATTRIB_POS;
	inst[1].SrcReg[1].NegateBase = 0;
	inst[1].SrcReg[1].Swizzle    = MAKE_SWIZZLE4(0, 1, 2, 3);
	inst[0].SrcReg[2].File       = PROGRAM_UNDEFINED;
	inst[1].DstReg.File      = PROGRAM_OUTPUT;
	inst[1].DstReg.Index     = VERT_RESULT_HPOS;
	inst[1].DstReg.WriteMask = WRITEMASK_XYZW;

	inst[2].Opcode = OPCODE_END;

	mvp->Base.Instructions = inst;

	nv40TranslateVertexProgram(vp);
}

