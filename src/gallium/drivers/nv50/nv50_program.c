#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "pipe/p_inlines.h"

#include "pipe/p_shader_tokens.h"
#include "tgsi/util/tgsi_parse.h"
#include "tgsi/util/tgsi_util.h"

#include "nv50_context.h"
#include "nv50_state.h"

#define NV50_SU_MAX_TEMP 64

/* ARL
 * LIT - other buggery
 * POW
 * SWZ - negation ARGH
 * SAT
 *
 * MSB - Like MAD, but MUL+SUB
 * 	- Fuck it off, introduce a way to negate args for ops that
 * 	  support it.
 *
 * Look into inlining IMMD for ops other than MOV
 * 	- Maybe even relax restrictions a bit, can't do P_RESULT + P_IMMD,
 * 	  but can emit to P_TEMP first - then MOV later. NVIDIA does this
 */
struct nv50_reg {
	enum {
		P_TEMP,
		P_ATTR,
		P_RESULT,
		P_CONST,
		P_IMMD
	} type;
	int index;

	int hw;
	int neg;
};

struct nv50_pc {
	struct nv50_program *p;

	/* hw resources */
	struct nv50_reg *r_temp[NV50_SU_MAX_TEMP];

	/* tgsi resources */
	struct nv50_reg *temp;
	int temp_nr;
	struct nv50_reg *attr;
	int attr_nr;
	struct nv50_reg *result;
	int result_nr;
	struct nv50_reg *param;
	int param_nr;
	struct nv50_reg *immd;
	float *immd_buf;
	int immd_nr;

	struct nv50_reg *temp_temp[8];
	unsigned temp_temp_nr;
};

static void
alloc_reg(struct nv50_pc *pc, struct nv50_reg *reg)
{
	int i;

	if (reg->type != P_TEMP)
		return;

	if (reg->hw >= 0) {
		/*XXX: do this here too to catch FP temp-as-attr usage..
		 *     not clean, but works */
		if (pc->p->cfg.high_temp < (reg->hw + 1))
			pc->p->cfg.high_temp = reg->hw + 1;
		return;
	}

	for (i = 0; i < NV50_SU_MAX_TEMP; i++) {
		if (!(pc->r_temp[i])) {
			pc->r_temp[i] = reg;
			reg->hw = i;
			if (pc->p->cfg.high_temp < (i + 1))
				pc->p->cfg.high_temp = i + 1;
			return;
		}
	}

	assert(0);
}

static struct nv50_reg *
alloc_temp(struct nv50_pc *pc, struct nv50_reg *dst)
{
	struct nv50_reg *r;
	int i;

	if (dst && dst->type == P_TEMP && dst->hw == -1)
		return dst;

	for (i = 0; i < NV50_SU_MAX_TEMP; i++) {
		if (!pc->r_temp[i]) {
			r = CALLOC_STRUCT(nv50_reg);
			r->type = P_TEMP;
			r->index = -1;
			r->hw = i;
			pc->r_temp[i] = r;
			return r;
		}
	}

	assert(0);
	return NULL;
}

static void
free_temp(struct nv50_pc *pc, struct nv50_reg *r)
{
	if (r->index == -1) {
		FREE(pc->r_temp[r->hw]);
		pc->r_temp[r->hw] = NULL;
	}
}

static struct nv50_reg *
temp_temp(struct nv50_pc *pc)
{
	if (pc->temp_temp_nr >= 8)
		assert(0);

	pc->temp_temp[pc->temp_temp_nr] = alloc_temp(pc, NULL);
	return pc->temp_temp[pc->temp_temp_nr++];
}

static void
kill_temp_temp(struct nv50_pc *pc)
{
	int i;
	
	for (i = 0; i < pc->temp_temp_nr; i++)
		free_temp(pc, pc->temp_temp[i]);
	pc->temp_temp_nr = 0;
}

static int
ctor_immd(struct nv50_pc *pc, float x, float y, float z, float w)
{
	pc->immd_buf = realloc(pc->immd_buf, (pc->immd_nr + 1) * 4 * 
					     sizeof(float));
	pc->immd_buf[(pc->immd_nr * 4) + 0] = x;
	pc->immd_buf[(pc->immd_nr * 4) + 1] = x;
	pc->immd_buf[(pc->immd_nr * 4) + 2] = x;
	pc->immd_buf[(pc->immd_nr * 4) + 3] = x;
	
	return pc->immd_nr++;
}

static struct nv50_reg *
alloc_immd(struct nv50_pc *pc, float f)
{
	struct nv50_reg *r = CALLOC_STRUCT(nv50_reg);
	unsigned hw;

	hw = ctor_immd(pc, f, 0, 0, 0);
	r->type = P_IMMD;
	r->hw = hw;
	r->index = -1;
	return r;
}

static struct nv50_reg *
tgsi_dst(struct nv50_pc *pc, int c, const struct tgsi_full_dst_register *dst)
{
	switch (dst->DstRegister.File) {
	case TGSI_FILE_TEMPORARY:
		return &pc->temp[dst->DstRegister.Index * 4 + c];
	case TGSI_FILE_OUTPUT:
		return &pc->result[dst->DstRegister.Index * 4 + c];
	case TGSI_FILE_NULL:
		return NULL;
	default:
		break;
	}

	return NULL;
}

static struct nv50_reg *
tgsi_src(struct nv50_pc *pc, int c, const struct tgsi_full_src_register *src)
{
	/* Handle swizzling */
	switch (c) {
	case 0: c = src->SrcRegister.SwizzleX; break;
	case 1: c = src->SrcRegister.SwizzleY; break;
	case 2: c = src->SrcRegister.SwizzleZ; break;
	case 3: c = src->SrcRegister.SwizzleW; break;
	default:
		assert(0);
	}

	switch (src->SrcRegister.File) {
	case TGSI_FILE_INPUT:
		return &pc->attr[src->SrcRegister.Index * 4 + c];
	case TGSI_FILE_TEMPORARY:
		return &pc->temp[src->SrcRegister.Index * 4 + c];
	case TGSI_FILE_CONSTANT:
		return &pc->param[src->SrcRegister.Index * 4 + c];
	case TGSI_FILE_IMMEDIATE:
		return &pc->immd[src->SrcRegister.Index * 4 + c];
	default:
		break;
	}

	return NULL;
}

static void
emit(struct nv50_pc *pc, unsigned *inst)
{
	struct nv50_program *p = pc->p;

	if (inst[0] & 1) {
		p->insns_nr += 2;
		p->insns = realloc(p->insns, sizeof(unsigned) * p->insns_nr);
		memcpy(p->insns + (p->insns_nr - 2), inst, sizeof(unsigned)*2);
	} else {
		p->insns_nr += 1;
		p->insns = realloc(p->insns, sizeof(unsigned) * p->insns_nr);
		memcpy(p->insns + (p->insns_nr - 1), inst, sizeof(unsigned));
	}
}

static INLINE void set_long(struct nv50_pc *, unsigned *);

static boolean
is_long(unsigned *inst)
{
	if (inst[0] & 1)
		return TRUE;
	return FALSE;
}

static boolean
is_immd(unsigned *inst)
{
	if (is_long(inst) && (inst[1] & 3) == 3)
		return TRUE;
	return FALSE;
}

static INLINE void
set_pred(struct nv50_pc *pc, unsigned pred, unsigned idx, unsigned *inst)
{
	set_long(pc, inst);
	inst[1] &= ~((0x1f << 7) | (0x3 << 12));
	inst[1] |= (pred << 7) | (idx << 12);
}

static INLINE void
set_pred_wr(struct nv50_pc *pc, unsigned on, unsigned idx, unsigned *inst)
{
	set_long(pc, inst);
	inst[1] &= ~((0x3 << 4) | (1 << 6));
	inst[1] |= (idx << 4) | (on << 6);
}

static INLINE void
set_long(struct nv50_pc *pc, unsigned *inst)
{
	if (is_long(inst))
		return;

	inst[0] |= 1;
	set_pred(pc, 0xf, 0, inst);
	set_pred_wr(pc, 0, 0, inst);
}

static INLINE void
set_dst(struct nv50_pc *pc, struct nv50_reg *dst, unsigned *inst)
{
	if (dst->type == P_RESULT) {
		set_long(pc, inst);
		inst[1] |= 0x00000008;
	}

	alloc_reg(pc, dst);
	inst[0] |= (dst->hw << 2);
}

static INLINE void
set_immd(struct nv50_pc *pc, struct nv50_reg *imm, unsigned *inst)
{
	unsigned val = fui(pc->immd_buf[imm->hw]); /* XXX */

	set_long(pc, inst);
	/*XXX: can't be predicated - bits overlap.. catch cases where both
	 *     are required and avoid them. */
	set_pred(pc, 0, 0, inst);
	set_pred_wr(pc, 0, 0, inst);

	inst[1] |= 0x00000002 | 0x00000001;
	inst[0] |= (val & 0x3f) << 16;
	inst[1] |= (val >> 6) << 2;
}

static void
emit_interp(struct nv50_pc *pc, struct nv50_reg *dst,
	    struct nv50_reg *src, struct nv50_reg *iv, boolean noperspective)
{
	unsigned inst[2] = { 0, 0 };

	inst[0] |= 0x80000000;
	set_dst(pc, dst, inst);
	alloc_reg(pc, iv);
	inst[0] |= (iv->hw << 9);
	alloc_reg(pc, src);
	inst[0] |= (src->hw << 16);
	if (noperspective)
		inst[0] |= (1 << 25);

	emit(pc, inst);
}

static void
set_cseg(struct nv50_pc *pc, struct nv50_reg *src, unsigned *inst)
{
	set_long(pc, inst);
	if (src->type == P_IMMD) {
		inst[1] |= (NV50_CB_PMISC << 22);
	} else {
		if (pc->p->type == NV50_PROG_VERTEX)
			inst[1] |= (NV50_CB_PVP << 22);
		else
			inst[1] |= (NV50_CB_PFP << 22);
	}
}

static void
emit_mov(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src)
{
	unsigned inst[2] = { 0, 0 };

	inst[0] |= 0x10000000;

	set_dst(pc, dst, inst);

	if (dst->type != P_RESULT && src->type == P_IMMD) {
		set_immd(pc, src, inst);
		/*XXX: 32-bit, but steals part of "half" reg space - need to
		 *     catch and handle this case if/when we do half-regs
		 */
		inst[0] |= 0x00008000;
	} else
	if (src->type == P_IMMD || src->type == P_CONST) {
		set_long(pc, inst);
		set_cseg(pc, src, inst);
		inst[0] |= (src->hw << 9);
		inst[1] |= 0x20000000; /* src0 const? */
	} else {
		if (src->type == P_ATTR) {
			set_long(pc, inst);
			inst[1] |= 0x00200000;
		}

		alloc_reg(pc, src);
		inst[0] |= (src->hw << 9);
	}

	/* We really should support "half" instructions here at some point,
	 * but I don't feel confident enough about them yet.
	 */
	set_long(pc, inst);
	if (is_long(inst) && !is_immd(inst)) {
		inst[1] |= 0x04000000; /* 32-bit */
		inst[1] |= 0x0003c000; /* "subsubop" 0xf == mov */
	}

	emit(pc, inst);
}

static boolean
check_swap_src_0_1(struct nv50_pc *pc,
		   struct nv50_reg **s0, struct nv50_reg **s1)
{
	struct nv50_reg *src0 = *s0, *src1 = *s1;

	if (src0->type == P_CONST) {
		if (src1->type != P_CONST) {
			*s0 = src1;
			*s1 = src0;
			return TRUE;
		}
	} else
	if (src1->type == P_ATTR) {
		if (src0->type != P_ATTR) {
			*s0 = src1;
			*s1 = src0;
			return TRUE;
		}
	}

	return FALSE;
}

static void
set_src_0(struct nv50_pc *pc, struct nv50_reg *src, unsigned *inst)
{
	if (src->type == P_ATTR) {
		set_long(pc, inst);
		inst[1] |= 0x00200000;
	} else
	if (src->type == P_CONST || src->type == P_IMMD) {
		struct nv50_reg *temp = temp_temp(pc);

		emit_mov(pc, temp, src);
		src = temp;
	}

	alloc_reg(pc, src);
	inst[0] |= (src->hw << 9);
}

static void
set_src_1(struct nv50_pc *pc, struct nv50_reg *src, unsigned *inst)
{
	if (src->type == P_ATTR) {
		struct nv50_reg *temp = temp_temp(pc);

		emit_mov(pc, temp, src);
		src = temp;
	} else
	if (src->type == P_CONST || src->type == P_IMMD) {
		set_cseg(pc, src, inst);
		inst[0] |= 0x00800000;
	}

	alloc_reg(pc, src);
	inst[0] |= (src->hw << 16);
}

static void
set_src_2(struct nv50_pc *pc, struct nv50_reg *src, unsigned *inst)
{
	set_long(pc, inst);

	if (src->type == P_ATTR) {
		struct nv50_reg *temp = temp_temp(pc);

		emit_mov(pc, temp, src);
		src = temp;
	} else
	if (src->type == P_CONST || src->type == P_IMMD) {
		set_cseg(pc, src, inst);
		inst[0] |= 0x01000000;
	}

	alloc_reg(pc, src);
	inst[1] |= (src->hw << 14);
}

static void
emit_mul(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src0,
	 struct nv50_reg *src1)
{
	unsigned inst[2] = { 0, 0 };

	inst[0] |= 0xc0000000;

	check_swap_src_0_1(pc, &src0, &src1);
	set_dst(pc, dst, inst);
	set_src_0(pc, src0, inst);
	set_src_1(pc, src1, inst);

	emit(pc, inst);
}

static void
emit_add(struct nv50_pc *pc, struct nv50_reg *dst,
	 struct nv50_reg *src0, struct nv50_reg *src1)
{
	unsigned inst[2] = { 0, 0 };

	inst[0] |= 0xb0000000;

	check_swap_src_0_1(pc, &src0, &src1);
	set_dst(pc, dst, inst);
	set_src_0(pc, src0, inst);
	if (is_long(inst))
		set_src_2(pc, src1, inst);
	else
		set_src_1(pc, src1, inst);

	emit(pc, inst);
}

static void
emit_minmax(struct nv50_pc *pc, unsigned sub, struct nv50_reg *dst,
	    struct nv50_reg *src0, struct nv50_reg *src1)
{
	unsigned inst[2] = { 0, 0 };

	set_long(pc, inst);
	inst[0] |= 0xb0000000;
	inst[1] |= (sub << 29);

	check_swap_src_0_1(pc, &src0, &src1);
	set_dst(pc, dst, inst);
	set_src_0(pc, src0, inst);
	set_src_1(pc, src1, inst);

	emit(pc, inst);
}

static void
emit_sub(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src0,
	 struct nv50_reg *src1)
{
	unsigned inst[2] = { 0, 0 };

	inst[0] |= 0xb0000000;

	set_long(pc, inst);
	if (check_swap_src_0_1(pc, &src0, &src1))
		inst[1] |= 0x04000000;
	else
		inst[1] |= 0x08000000;

	set_dst(pc, dst, inst);
	set_src_0(pc, src0, inst);
	set_src_2(pc, src1, inst);

	emit(pc, inst);
}

static void
emit_mad(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src0,
	 struct nv50_reg *src1, struct nv50_reg *src2)
{
	unsigned inst[2] = { 0, 0 };

	inst[0] |= 0xe0000000;

	check_swap_src_0_1(pc, &src0, &src1);
	set_dst(pc, dst, inst);
	set_src_0(pc, src0, inst);
	set_src_1(pc, src1, inst);
	set_src_2(pc, src2, inst);

	emit(pc, inst);
}

static void
emit_msb(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src0,
	 struct nv50_reg *src1, struct nv50_reg *src2)
{
	unsigned inst[2] = { 0, 0 };

	inst[0] |= 0xe0000000;
	set_long(pc, inst);
	inst[1] |= 0x08000000; /* src0 * src1 - src2 */

	check_swap_src_0_1(pc, &src0, &src1);
	set_dst(pc, dst, inst);
	set_src_0(pc, src0, inst);
	set_src_1(pc, src1, inst);
	set_src_2(pc, src2, inst);

	emit(pc, inst);
}

static void
emit_flop(struct nv50_pc *pc, unsigned sub,
	  struct nv50_reg *dst, struct nv50_reg *src)
{
	unsigned inst[2] = { 0, 0 };

	inst[0] |= 0x90000000;
	if (sub) {
		set_long(pc, inst);
		inst[1] |= (sub << 29);
	}

	set_dst(pc, dst, inst);
	set_src_0(pc, src, inst);

	emit(pc, inst);
}

static void
emit_preex2(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src)
{
	unsigned inst[2] = { 0, 0 };

	inst[0] |= 0xb0000000;

	set_dst(pc, dst, inst);
	set_src_0(pc, src, inst);
	set_long(pc, inst);
	inst[1] |= (6 << 29) | 0x00004000;

	emit(pc, inst);
}
/*XXX: inaccurate results.. why? */
#define ALLOW_SET_SWAP 0

static void
emit_set(struct nv50_pc *pc, unsigned c_op, struct nv50_reg *dst,
	 struct nv50_reg *src0, struct nv50_reg *src1)
{
	unsigned inst[2] = { 0, 0 };
#if ALLOW_SET_SWAP
	unsigned inv_cop[8] = { 0, 6, 2, 4, 3, 5, 1, 7 };
#endif
	struct nv50_reg *rdst;

#if ALLOW_SET_SWAP
	assert(c_op <= 7);
	if (check_swap_src_0_1(pc, &src0, &src1))
		c_op = inv_cop[c_op];
#endif

	rdst = dst;
	if (dst->type != P_TEMP)
		dst = alloc_temp(pc, NULL);

	/* set.u32 */
	set_long(pc, inst);
	inst[0] |= 0xb0000000;
	inst[1] |= (3 << 29);
	inst[1] |= (c_op << 14);
	/*XXX: breaks things, .u32 by default?
	 *     decuda will disasm as .u16 and use .lo/.hi regs, but this
	 *     doesn't seem to match what the hw actually does.
	inst[1] |= 0x04000000; << breaks things.. .u32 by default?
	 */
	set_dst(pc, dst, inst);
	set_src_0(pc, src0, inst);
	set_src_1(pc, src1, inst);
	emit(pc, inst);

	/* cvt.f32.u32 */
	inst[0] = 0xa0000001;
	inst[1] = 0x64014780;
	set_dst(pc, rdst, inst);
	set_src_0(pc, dst, inst);
	emit(pc, inst);

	if (dst != rdst)
		free_temp(pc, dst);
}

static void
emit_flr(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src)
{
	unsigned inst[2] = { 0, 0 };

	set_long(pc, inst);
	inst[0] = 0xa0000000; /* cvt */
	inst[1] |= (6 << 29); /* cvt */
	inst[1] |= 0x08000000; /* integer mode */
	inst[1] |= 0x04000000; /* 32 bit */
	inst[1] |= ((0x1 << 3)) << 14; /* .rn */
	inst[1] |= (1 << 14); /* src .f32 */
	set_dst(pc, dst, inst);
	set_src_0(pc, src, inst);

	emit(pc, inst);
}

static boolean
nv50_program_tx_insn(struct nv50_pc *pc, const union tgsi_full_token *tok)
{
	const struct tgsi_full_instruction *inst = &tok->FullInstruction;
	struct nv50_reg *dst[4], *src[3][4], *temp;
	unsigned mask;
	int i, c;

	NOUVEAU_ERR("insn %p\n", tok);

	mask = inst->FullDstRegisters[0].DstRegister.WriteMask;

	for (c = 0; c < 4; c++) {
		if (mask & (1 << c))
			dst[c] = tgsi_dst(pc, c, &inst->FullDstRegisters[0]);
		else
			dst[c] = NULL;
	}

	for (i = 0; i < inst->Instruction.NumSrcRegs; i++) {
		for (c = 0; c < 4; c++)
			src[i][c] = tgsi_src(pc, c, &inst->FullSrcRegisters[i]);
	}

	switch (inst->Instruction.Opcode) {
	case TGSI_OPCODE_ABS:
		for (c = 0; c < 4; c++) {
			unsigned inst[2] = { 0, 0 };

			set_long(pc, inst);
			inst[0] = 0xa0000000; /* cvt */
			inst[1] |= (6 << 29); /* cvt */
			inst[1] |= 0x04000000; /* 32 bit */
			inst[1] |= (1 << 14); /* src .f32 */
			inst[1] |= ((1 << 6) << 14); /* .abs */
			set_dst(pc, dst[c], inst);
			set_src_0(pc, src[0][c], inst);
			emit(pc, inst);
		}
		break;
	case TGSI_OPCODE_ADD:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_add(pc, dst[c], src[0][c], src[1][c]);
		}
		break;
	case TGSI_OPCODE_COS:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_flop(pc, 5, dst[c], src[0][c]);
		}
		break;
	case TGSI_OPCODE_DP3:
		temp = alloc_temp(pc, NULL);
		emit_mul(pc, temp, src[0][0], src[1][0]);
		emit_mad(pc, temp, src[0][1], src[1][1], temp);
		emit_mad(pc, temp, src[0][2], src[1][2], temp);
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_mov(pc, dst[c], temp);
		}
		free_temp(pc, temp);
		break;
	case TGSI_OPCODE_DP4:
		temp = alloc_temp(pc, NULL);
		emit_mul(pc, temp, src[0][0], src[1][0]);
		emit_mad(pc, temp, src[0][1], src[1][1], temp);
		emit_mad(pc, temp, src[0][2], src[1][2], temp);
		emit_mad(pc, temp, src[0][3], src[1][3], temp);
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_mov(pc, dst[c], temp);
		}
		free_temp(pc, temp);
		break;
	case TGSI_OPCODE_DPH:
		temp = alloc_temp(pc, NULL);
		emit_mul(pc, temp, src[0][0], src[1][0]);
		emit_mad(pc, temp, src[0][1], src[1][1], temp);
		emit_mad(pc, temp, src[0][2], src[1][2], temp);
		emit_add(pc, temp, src[1][3], temp);
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_mov(pc, dst[c], temp);
		}
		free_temp(pc, temp);
		break;
	case TGSI_OPCODE_DST:
	{
		struct nv50_reg *one = alloc_immd(pc, 1.0);
		emit_mov(pc, dst[0], one);
		emit_mul(pc, dst[1], src[0][1], src[1][1]);
		emit_mov(pc, dst[2], src[0][2]);
		emit_mov(pc, dst[3], src[1][3]);
		FREE(one);
	}
		break;
	case TGSI_OPCODE_EX2:
		temp = alloc_temp(pc, NULL);
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_preex2(pc, temp, src[0][c]);
			emit_flop(pc, 6, dst[c], temp);
		}
		free_temp(pc, temp);
		break;
	case TGSI_OPCODE_FLR:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_flr(pc, dst[c], src[0][c]);
		}
		break;
	case TGSI_OPCODE_FRC:
		temp = alloc_temp(pc, NULL);
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_flr(pc, temp, src[0][c]);
			emit_sub(pc, dst[c], src[0][c], temp);
		}
		free_temp(pc, temp);
		break;
	case TGSI_OPCODE_LG2:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_flop(pc, 3, dst[c], src[0][c]);
		}
		break;
	case TGSI_OPCODE_MAD:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_mad(pc, dst[c], src[0][c], src[1][c], src[2][c]);
		}
		break;
	case TGSI_OPCODE_MAX:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_minmax(pc, 4, dst[c], src[0][c], src[1][c]);
		}
		break;
	case TGSI_OPCODE_MIN:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_minmax(pc, 5, dst[c], src[0][c], src[1][c]);
		}
		break;
	case TGSI_OPCODE_MOV:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_mov(pc, dst[c], src[0][c]);
		}
		break;
	case TGSI_OPCODE_MUL:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_mul(pc, dst[c], src[0][c], src[1][c]);
		}
		break;
	case TGSI_OPCODE_RCP:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_flop(pc, 0, dst[c], src[0][c]);
		}
		break;
	case TGSI_OPCODE_RSQ:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_flop(pc, 2, dst[c], src[0][c]);
		}
		break;
	case TGSI_OPCODE_SGE:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_set(pc, 6, dst[c], src[0][c], src[1][c]);
		}
		break;
	case TGSI_OPCODE_SIN:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_flop(pc, 4, dst[c], src[0][c]);
		}
		break;
	case TGSI_OPCODE_SLT:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_set(pc, 1, dst[c], src[0][c], src[1][c]);
		}
		break;
	case TGSI_OPCODE_SUB:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_sub(pc, dst[c], src[0][c], src[1][c]);
		}
		break;
	case TGSI_OPCODE_XPD:
		temp = alloc_temp(pc, NULL);
		emit_mul(pc, temp, src[0][2], src[1][1]);
		emit_msb(pc, dst[0], src[0][1], src[1][2], temp);
		emit_mul(pc, temp, src[0][0], src[1][2]);
		emit_msb(pc, dst[1], src[0][2], src[1][0], temp);
		emit_mul(pc, temp, src[0][1], src[1][0]);
		emit_msb(pc, dst[2], src[0][0], src[1][1], temp);
		free_temp(pc, temp);
		break;
	case TGSI_OPCODE_END:
		break;
	default:
		NOUVEAU_ERR("invalid opcode %d\n", inst->Instruction.Opcode);
		return FALSE;
	}

	kill_temp_temp(pc);
	return TRUE;
}

static boolean
nv50_program_tx_prep(struct nv50_pc *pc)
{
	struct tgsi_parse_context p;
	boolean ret = FALSE;
	unsigned i, c;

	tgsi_parse_init(&p, pc->p->pipe.tokens);
	while (!tgsi_parse_end_of_tokens(&p)) {
		const union tgsi_full_token *tok = &p.FullToken;

		tgsi_parse_token(&p);
		switch (tok->Token.Type) {
		case TGSI_TOKEN_TYPE_IMMEDIATE:
		{
			const struct tgsi_full_immediate *imm =
				&p.FullToken.FullImmediate;

			ctor_immd(pc, imm->u.ImmediateFloat32[0].Float,
				      imm->u.ImmediateFloat32[1].Float,
				      imm->u.ImmediateFloat32[2].Float,
				      imm->u.ImmediateFloat32[3].Float);
		}
			break;
		case TGSI_TOKEN_TYPE_DECLARATION:
		{
			const struct tgsi_full_declaration *d;
			unsigned last;

			d = &p.FullToken.FullDeclaration;
			last = d->u.DeclarationRange.Last;

			switch (d->Declaration.File) {
			case TGSI_FILE_TEMPORARY:
				if (pc->temp_nr < (last + 1))
					pc->temp_nr = last + 1;
				break;
			case TGSI_FILE_OUTPUT:
				if (pc->result_nr < (last + 1))
					pc->result_nr = last + 1;
				break;
			case TGSI_FILE_INPUT:
				if (pc->attr_nr < (last + 1))
					pc->attr_nr = last + 1;
				break;
			case TGSI_FILE_CONSTANT:
				if (pc->param_nr < (last + 1))
					pc->param_nr = last + 1;
				break;
			default:
				NOUVEAU_ERR("bad decl file %d\n",
					    d->Declaration.File);
				goto out_err;
			}
		}
			break;
		case TGSI_TOKEN_TYPE_INSTRUCTION:
			break;
		default:
			break;
		}
	}

	NOUVEAU_ERR("%d temps\n", pc->temp_nr);
	if (pc->temp_nr) {
		pc->temp = calloc(pc->temp_nr * 4, sizeof(struct nv50_reg));
		if (!pc->temp)
			goto out_err;

		for (i = 0; i < pc->temp_nr; i++) {
			for (c = 0; c < 4; c++) {
				pc->temp[i*4+c].type = P_TEMP;
				pc->temp[i*4+c].hw = -1;
				pc->temp[i*4+c].index = i;
			}
		}
	}

	NOUVEAU_ERR("%d attrib regs\n", pc->attr_nr);
	if (pc->attr_nr) {
		struct nv50_reg *iv = NULL, *tmp = NULL;
		int aid = 0;

		pc->attr = calloc(pc->attr_nr * 4, sizeof(struct nv50_reg));
		if (!pc->attr)
			goto out_err;

		if (pc->p->type == NV50_PROG_FRAGMENT) {
			iv = alloc_temp(pc, NULL);
			aid++;
		}

		for (i = 0; i < pc->attr_nr; i++) {
			struct nv50_reg *a = &pc->attr[i*4];

			for (c = 0; c < 4; c++) {
				if (pc->p->type == NV50_PROG_FRAGMENT) {
					struct nv50_reg *at =
						alloc_temp(pc, NULL);
					pc->attr[i*4+c].type = at->type;
					pc->attr[i*4+c].hw = at->hw;
					pc->attr[i*4+c].index = at->index;
				} else {
					pc->p->cfg.vp.attr[aid/32] |=
						(1 << (aid % 32));
					pc->attr[i*4+c].type = P_ATTR;
					pc->attr[i*4+c].hw = aid++;
					pc->attr[i*4+c].index = i;
				}
			}

			if (pc->p->type != NV50_PROG_FRAGMENT)
				continue;

			emit_interp(pc, iv, iv, iv, FALSE);
			tmp = alloc_temp(pc, NULL);
			{
				unsigned inst[2] = { 0, 0 };
				inst[0]  = 0x90000000;
				inst[0] |= (tmp->hw << 2);
				emit(pc, inst);
			}
			emit_interp(pc, &a[0], &a[0], tmp, TRUE);
			emit_interp(pc, &a[1], &a[1], tmp, TRUE);
			emit_interp(pc, &a[2], &a[2], tmp, TRUE);
			emit_interp(pc, &a[3], &a[3], tmp, TRUE);
			free_temp(pc, tmp);
		}

		if (iv)
			free_temp(pc, iv);
	}

	NOUVEAU_ERR("%d result regs\n", pc->result_nr);
	if (pc->result_nr) {
		int rid = 0;

		pc->result = calloc(pc->result_nr * 4, sizeof(struct nv50_reg));
		if (!pc->result)
			goto out_err;

		for (i = 0; i < pc->result_nr; i++) {
			for (c = 0; c < 4; c++) {
				if (pc->p->type == NV50_PROG_FRAGMENT)
					pc->result[i*4+c].type = P_TEMP;
				else
					pc->result[i*4+c].type = P_RESULT;
				pc->result[i*4+c].hw = rid++;
				pc->result[i*4+c].index = i;
			}
		}
	}

	NOUVEAU_ERR("%d param regs\n", pc->param_nr);
	if (pc->param_nr) {
		int rid = 0;

		pc->param = calloc(pc->param_nr * 4, sizeof(struct nv50_reg));
		if (!pc->param)
			goto out_err;

		for (i = 0; i < pc->param_nr; i++) {
			for (c = 0; c < 4; c++) {
				pc->param[i*4+c].type = P_CONST;
				pc->param[i*4+c].hw = rid++;
				pc->param[i*4+c].index = i;
			}
		}
	}

	if (pc->immd_nr) {
		int rid = 0;

		pc->immd = calloc(pc->immd_nr * 4, sizeof(struct nv50_reg));
		if (!pc->immd)
			goto out_err;

		for (i = 0; i < pc->immd_nr; i++) {
			for (c = 0; c < 4; c++) {
				pc->immd[i*4+c].type = P_IMMD;
				pc->immd[i*4+c].hw = rid++;
				pc->immd[i*4+c].index = i;
			}
		}
	}

	ret = TRUE;
out_err:
	tgsi_parse_free(&p);
	return ret;
}

static boolean
nv50_program_tx(struct nv50_program *p)
{
	struct tgsi_parse_context parse;
	struct nv50_pc *pc;
	boolean ret;

	pc = CALLOC_STRUCT(nv50_pc);
	if (!pc)
		return FALSE;
	pc->p = p;
	pc->p->cfg.high_temp = 4;

	ret = nv50_program_tx_prep(pc);
	if (ret == FALSE)
		goto out_cleanup;

	tgsi_parse_init(&parse, pc->p->pipe.tokens);
	while (!tgsi_parse_end_of_tokens(&parse)) {
		const union tgsi_full_token *tok = &parse.FullToken;

		tgsi_parse_token(&parse);

		switch (tok->Token.Type) {
		case TGSI_TOKEN_TYPE_INSTRUCTION:
			ret = nv50_program_tx_insn(pc, tok);
			if (ret == FALSE)
				goto out_err;
			break;
		default:
			break;
		}
	}

	p->immd_nr = pc->immd_nr * 4;
	p->immd = pc->immd_buf;

out_err:
	tgsi_parse_free(&parse);

out_cleanup:
	return ret;
}

static void
nv50_program_validate(struct nv50_context *nv50, struct nv50_program *p)
{
	int i;

	if (nv50_program_tx(p) == FALSE)
		assert(0);
	/* *not* sufficient, it's fine if last inst is long and
	 * NOT immd - otherwise it's fucked fucked fucked */
	p->insns[p->insns_nr - 1] |= 0x00000001;

	if (p->type == NV50_PROG_VERTEX) {
	for (i = 0; i < p->insns_nr; i++)
		NOUVEAU_ERR("VP0x%08x\n", p->insns[i]);
	} else {
	for (i = 0; i < p->insns_nr; i++)
		NOUVEAU_ERR("FP0x%08x\n", p->insns[i]);
	}

	p->translated = TRUE;
}

static void
nv50_program_validate_data(struct nv50_context *nv50, struct nv50_program *p)
{
	int i;

	for (i = 0; i < p->immd_nr; i++) {
		BEGIN_RING(tesla, 0x0f00, 2);
		OUT_RING  ((NV50_CB_PMISC << 16) | (i << 8));
		OUT_RING  (fui(p->immd[i]));
	}
}

static void
nv50_program_validate_code(struct nv50_context *nv50, struct nv50_program *p)
{
	struct pipe_winsys *ws = nv50->pipe.winsys;
	void *map;

	if (!p->buffer)
		p->buffer = ws->buffer_create(ws, 0x100, 0, p->insns_nr * 4);
	map = ws->buffer_map(ws, p->buffer, PIPE_BUFFER_USAGE_CPU_WRITE);
	memcpy(map, p->insns, p->insns_nr * 4);
	ws->buffer_unmap(ws, p->buffer);
}

void
nv50_vertprog_validate(struct nv50_context *nv50)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nv50_program *p = nv50->vertprog;
	struct nouveau_stateobj *so;

	if (!p->translated) {
		nv50_program_validate(nv50, p);
		if (!p->translated)
			assert(0);
	}

	nv50_program_validate_data(nv50, p);
	nv50_program_validate_code(nv50, p);

	so = so_new(11, 2);
	so_method(so, tesla, NV50TCL_VP_ADDRESS_HIGH, 2);
	so_reloc (so, p->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
		  NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, p->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
		  NOUVEAU_BO_LOW, 0, 0);
	so_method(so, tesla, 0x1650, 2);
	so_data  (so, p->cfg.vp.attr[0]);
	so_data  (so, p->cfg.vp.attr[1]);
	so_method(so, tesla, 0x16ac, 2);
	so_data  (so, 8);
	so_data  (so, p->cfg.high_temp);
	so_method(so, tesla, 0x140c, 1);
	so_data  (so, 0); /* program start offset */
	so_emit(nv50->screen->nvws, so);
	so_ref(NULL, &so);
}

void
nv50_fragprog_validate(struct nv50_context *nv50)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nv50_program *p = nv50->fragprog;
	struct nouveau_stateobj *so;

	if (!p->translated) {
		nv50_program_validate(nv50, p);
		if (!p->translated)
			assert(0);
	}

	nv50_program_validate_data(nv50, p);
	nv50_program_validate_code(nv50, p);

	so = so_new(7, 2);
	so_method(so, tesla, NV50TCL_FP_ADDRESS_HIGH, 2);
	so_reloc (so, p->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
		  NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, p->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
		  NOUVEAU_BO_LOW, 0, 0);
	so_method(so, tesla, 0x198c, 1);
	so_data  (so, p->cfg.high_temp);
	so_method(so, tesla, 0x1414, 1);
	so_data  (so, 0); /* program start offset */
	so_emit(nv50->screen->nvws, so);
	so_ref(NULL, &so);
}

void
nv50_program_destroy(struct nv50_context *nv50, struct nv50_program *p)
{
	struct pipe_winsys *ws = nv50->pipe.winsys;

	if (p->insns_nr) {
		if (p->insns)
			FREE(p->insns);
		p->insns_nr = 0;
	}

	if (p->buffer)
		pipe_buffer_reference(ws, &p->buffer, NULL);

	p->translated = 0;
}

