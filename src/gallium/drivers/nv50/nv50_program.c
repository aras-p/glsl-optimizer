/*
 * Copyright 2008 Ben Skeggs
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "pipe/p_inlines.h"

#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"

#include "nv50_context.h"

#define NV50_SU_MAX_TEMP 64
//#define NV50_PROGRAM_DUMP

/* ARL - gallium craps itself on progs/vp/arl.txt
 *
 * MSB - Like MAD, but MUL+SUB
 * 	- Fuck it off, introduce a way to negate args for ops that
 * 	  support it.
 *
 * Look into inlining IMMD for ops other than MOV (make it general?)
 * 	- Maybe even relax restrictions a bit, can't do P_RESULT + P_IMMD,
 * 	  but can emit to P_TEMP first - then MOV later. NVIDIA does this
 *
 * In ops such as ADD it's possible to construct a bad opcode in the !is_long()
 * case, if the emit_src() causes the inst to suddenly become long.
 *
 * Verify half-insns work where expected - and force disable them where they
 * don't work - MUL has it forcibly disabled atm as it fixes POW..
 *
 * FUCK! watch dst==src vectors, can overwrite components that are needed.
 * 	ie. SUB R0, R0.yzxw, R0
 *
 * Things to check with renouveau:
 * 	FP attr/result assignment - how?
 * 		attrib
 * 			- 0x16bc maps vp output onto fp hpos
 * 			- 0x16c0 maps vp output onto fp col0
 * 		result
 * 			- colr always 0-3
 * 			- depr always 4
 * 0x16bc->0x16e8 --> some binding between vp/fp regs
 * 0x16b8 --> VP output count
 *
 * 0x1298 --> "MOV rcol.x, fcol.y" "MOV depr, fcol.y" = 0x00000005
 * 	      "MOV rcol.x, fcol.y" = 0x00000004
 * 0x19a8 --> as above but 0x00000100 and 0x00000000
 * 	- 0x00100000 used when KIL used
 * 0x196c --> as above but 0x00000011 and 0x00000000
 *
 * 0x1988 --> 0xXXNNNNNN
 * 	- XX == FP high something
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

	struct nv50_reg *temp_temp[16];
	unsigned temp_temp_nr;
};

static void
alloc_reg(struct nv50_pc *pc, struct nv50_reg *reg)
{
	int i;

	if (reg->type == P_RESULT) {
		if (pc->p->cfg.high_result < (reg->hw + 1))
			pc->p->cfg.high_result = reg->hw + 1;
	}

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
		unsigned hw = r->hw;

		FREE(pc->r_temp[hw]);
		pc->r_temp[hw] = NULL;
	}
}

static int
alloc_temp4(struct nv50_pc *pc, struct nv50_reg *dst[4], int idx)
{
	int i;

	if ((idx + 4) >= NV50_SU_MAX_TEMP)
		return 1;

	if (pc->r_temp[idx] || pc->r_temp[idx + 1] ||
	    pc->r_temp[idx + 2] || pc->r_temp[idx + 3])
		return alloc_temp4(pc, dst, idx + 1);

	for (i = 0; i < 4; i++) {
		dst[i] = CALLOC_STRUCT(nv50_reg);
		dst[i]->type = P_TEMP;
		dst[i]->index = -1;
		dst[i]->hw = idx + i;
		pc->r_temp[idx + i] = dst[i];
	}

	return 0;
}

static void
free_temp4(struct nv50_pc *pc, struct nv50_reg *reg[4])
{
	int i;

	for (i = 0; i < 4; i++)
		free_temp(pc, reg[i]);
}

static struct nv50_reg *
temp_temp(struct nv50_pc *pc)
{
	if (pc->temp_temp_nr >= 16)
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
	pc->immd_buf = REALLOC(pc->immd_buf, (pc->immd_nr * r * sizeof(float)),
			       (pc->immd_nr + 1) * 4 * sizeof(float));
	pc->immd_buf[(pc->immd_nr * 4) + 0] = x;
	pc->immd_buf[(pc->immd_nr * 4) + 1] = y;
	pc->immd_buf[(pc->immd_nr * 4) + 2] = z;
	pc->immd_buf[(pc->immd_nr * 4) + 3] = w;
	
	return pc->immd_nr++;
}

static struct nv50_reg *
alloc_immd(struct nv50_pc *pc, float f)
{
	struct nv50_reg *r = CALLOC_STRUCT(nv50_reg);
	unsigned hw;

	hw = ctor_immd(pc, f, 0, 0, 0) * 4;
	r->type = P_IMMD;
	r->hw = hw;
	r->index = -1;
	return r;
}

static struct nv50_program_exec *
exec(struct nv50_pc *pc)
{
	struct nv50_program_exec *e = CALLOC_STRUCT(nv50_program_exec);

	e->param.index = -1;
	return e;
}

static void
emit(struct nv50_pc *pc, struct nv50_program_exec *e)
{
	struct nv50_program *p = pc->p;

	if (p->exec_tail)
		p->exec_tail->next = e;
	if (!p->exec_head)
		p->exec_head = e;
	p->exec_tail = e;
	p->exec_size += (e->inst[0] & 1) ? 2 : 1;
}

static INLINE void set_long(struct nv50_pc *, struct nv50_program_exec *);

static boolean
is_long(struct nv50_program_exec *e)
{
	if (e->inst[0] & 1)
		return TRUE;
	return FALSE;
}

static boolean
is_immd(struct nv50_program_exec *e)
{
	if (is_long(e) && (e->inst[1] & 3) == 3)
		return TRUE;
	return FALSE;
}

static INLINE void
set_pred(struct nv50_pc *pc, unsigned pred, unsigned idx,
	 struct nv50_program_exec *e)
{
	set_long(pc, e);
	e->inst[1] &= ~((0x1f << 7) | (0x3 << 12));
	e->inst[1] |= (pred << 7) | (idx << 12);
}

static INLINE void
set_pred_wr(struct nv50_pc *pc, unsigned on, unsigned idx,
	    struct nv50_program_exec *e)
{
	set_long(pc, e);
	e->inst[1] &= ~((0x3 << 4) | (1 << 6));
	e->inst[1] |= (idx << 4) | (on << 6);
}

static INLINE void
set_long(struct nv50_pc *pc, struct nv50_program_exec *e)
{
	if (is_long(e))
		return;

	e->inst[0] |= 1;
	set_pred(pc, 0xf, 0, e);
	set_pred_wr(pc, 0, 0, e);
}

static INLINE void
set_dst(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_program_exec *e)
{
	if (dst->type == P_RESULT) {
		set_long(pc, e);
		e->inst[1] |= 0x00000008;
	}

	alloc_reg(pc, dst);
	e->inst[0] |= (dst->hw << 2);
}

static INLINE void
set_immd(struct nv50_pc *pc, struct nv50_reg *imm, struct nv50_program_exec *e)
{
	unsigned val = fui(pc->immd_buf[imm->hw]); /* XXX */

	set_long(pc, e);
	/*XXX: can't be predicated - bits overlap.. catch cases where both
	 *     are required and avoid them. */
	set_pred(pc, 0, 0, e);
	set_pred_wr(pc, 0, 0, e);

	e->inst[1] |= 0x00000002 | 0x00000001;
	e->inst[0] |= (val & 0x3f) << 16;
	e->inst[1] |= (val >> 6) << 2;
}

static void
emit_interp(struct nv50_pc *pc, struct nv50_reg *dst,
	    struct nv50_reg *src, struct nv50_reg *iv)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] |= 0x80000000;
	set_dst(pc, dst, e);
	alloc_reg(pc, src);
	e->inst[0] |= (src->hw << 16);
	if (iv) {
		e->inst[0] |= (1 << 25);
		alloc_reg(pc, iv);
		e->inst[0] |= (iv->hw << 9);
	}

	emit(pc, e);
}

static void
set_data(struct nv50_pc *pc, struct nv50_reg *src, unsigned m, unsigned s,
	 struct nv50_program_exec *e)
{
	set_long(pc, e);
#if 1
	e->inst[1] |= (1 << 22);
#else
	if (src->type == P_IMMD) {
		e->inst[1] |= (NV50_CB_PMISC << 22);
	} else {
		if (pc->p->type == PIPE_SHADER_VERTEX)
			e->inst[1] |= (NV50_CB_PVP << 22);
		else
			e->inst[1] |= (NV50_CB_PFP << 22);
	}
#endif

	e->param.index = src->hw;
	e->param.shift = s;
	e->param.mask = m << (s % 32);
}

static void
emit_mov(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] |= 0x10000000;

	set_dst(pc, dst, e);

	if (0 && dst->type != P_RESULT && src->type == P_IMMD) {
		set_immd(pc, src, e);
		/*XXX: 32-bit, but steals part of "half" reg space - need to
		 *     catch and handle this case if/when we do half-regs
		 */
		e->inst[0] |= 0x00008000;
	} else
	if (src->type == P_IMMD || src->type == P_CONST) {
		set_long(pc, e);
		set_data(pc, src, 0x7f, 9, e);
		e->inst[1] |= 0x20000000; /* src0 const? */
	} else {
		if (src->type == P_ATTR) {
			set_long(pc, e);
			e->inst[1] |= 0x00200000;
		}

		alloc_reg(pc, src);
		e->inst[0] |= (src->hw << 9);
	}

	/* We really should support "half" instructions here at some point,
	 * but I don't feel confident enough about them yet.
	 */
	set_long(pc, e);
	if (is_long(e) && !is_immd(e)) {
		e->inst[1] |= 0x04000000; /* 32-bit */
		e->inst[1] |= 0x0003c000; /* "subsubop" 0xf == mov */
	}

	emit(pc, e);
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
set_src_0(struct nv50_pc *pc, struct nv50_reg *src, struct nv50_program_exec *e)
{
	if (src->type == P_ATTR) {
		set_long(pc, e);
		e->inst[1] |= 0x00200000;
	} else
	if (src->type == P_CONST || src->type == P_IMMD) {
		struct nv50_reg *temp = temp_temp(pc);

		emit_mov(pc, temp, src);
		src = temp;
	}

	alloc_reg(pc, src);
	e->inst[0] |= (src->hw << 9);
}

static void
set_src_1(struct nv50_pc *pc, struct nv50_reg *src, struct nv50_program_exec *e)
{
	if (src->type == P_ATTR) {
		struct nv50_reg *temp = temp_temp(pc);

		emit_mov(pc, temp, src);
		src = temp;
	} else
	if (src->type == P_CONST || src->type == P_IMMD) {
		assert(!(e->inst[0] & 0x00800000));
		if (e->inst[0] & 0x01000000) {
			struct nv50_reg *temp = temp_temp(pc);

			emit_mov(pc, temp, src);
			src = temp;
		} else {
			set_data(pc, src, 0x7f, 16, e);
			e->inst[0] |= 0x00800000;
		}
	}

	alloc_reg(pc, src);
	e->inst[0] |= (src->hw << 16);
}

static void
set_src_2(struct nv50_pc *pc, struct nv50_reg *src, struct nv50_program_exec *e)
{
	set_long(pc, e);

	if (src->type == P_ATTR) {
		struct nv50_reg *temp = temp_temp(pc);

		emit_mov(pc, temp, src);
		src = temp;
	} else
	if (src->type == P_CONST || src->type == P_IMMD) {
		assert(!(e->inst[0] & 0x01000000));
		if (e->inst[0] & 0x00800000) {
			struct nv50_reg *temp = temp_temp(pc);

			emit_mov(pc, temp, src);
			src = temp;
		} else {
			set_data(pc, src, 0x7f, 32+14, e);
			e->inst[0] |= 0x01000000;
		}
	}

	alloc_reg(pc, src);
	e->inst[1] |= (src->hw << 14);
}

static void
emit_mul(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src0,
	 struct nv50_reg *src1)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] |= 0xc0000000;
	set_long(pc, e);

	check_swap_src_0_1(pc, &src0, &src1);
	set_dst(pc, dst, e);
	set_src_0(pc, src0, e);
	set_src_1(pc, src1, e);

	emit(pc, e);
}

static void
emit_add(struct nv50_pc *pc, struct nv50_reg *dst,
	 struct nv50_reg *src0, struct nv50_reg *src1)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] |= 0xb0000000;

	check_swap_src_0_1(pc, &src0, &src1);
	set_dst(pc, dst, e);
	set_src_0(pc, src0, e);
	if (is_long(e))
		set_src_2(pc, src1, e);
	else
		set_src_1(pc, src1, e);

	emit(pc, e);
}

static void
emit_minmax(struct nv50_pc *pc, unsigned sub, struct nv50_reg *dst,
	    struct nv50_reg *src0, struct nv50_reg *src1)
{
	struct nv50_program_exec *e = exec(pc);

	set_long(pc, e);
	e->inst[0] |= 0xb0000000;
	e->inst[1] |= (sub << 29);

	check_swap_src_0_1(pc, &src0, &src1);
	set_dst(pc, dst, e);
	set_src_0(pc, src0, e);
	set_src_1(pc, src1, e);

	emit(pc, e);
}

static void
emit_sub(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src0,
	 struct nv50_reg *src1)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] |= 0xb0000000;

	set_long(pc, e);
	if (check_swap_src_0_1(pc, &src0, &src1))
		e->inst[1] |= 0x04000000;
	else
		e->inst[1] |= 0x08000000;

	set_dst(pc, dst, e);
	set_src_0(pc, src0, e);
	set_src_2(pc, src1, e);

	emit(pc, e);
}

static void
emit_mad(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src0,
	 struct nv50_reg *src1, struct nv50_reg *src2)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] |= 0xe0000000;

	check_swap_src_0_1(pc, &src0, &src1);
	set_dst(pc, dst, e);
	set_src_0(pc, src0, e);
	set_src_1(pc, src1, e);
	set_src_2(pc, src2, e);

	emit(pc, e);
}

static void
emit_msb(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src0,
	 struct nv50_reg *src1, struct nv50_reg *src2)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] |= 0xe0000000;
	set_long(pc, e);
	e->inst[1] |= 0x08000000; /* src0 * src1 - src2 */

	check_swap_src_0_1(pc, &src0, &src1);
	set_dst(pc, dst, e);
	set_src_0(pc, src0, e);
	set_src_1(pc, src1, e);
	set_src_2(pc, src2, e);

	emit(pc, e);
}

static void
emit_flop(struct nv50_pc *pc, unsigned sub,
	  struct nv50_reg *dst, struct nv50_reg *src)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] |= 0x90000000;
	if (sub) {
		set_long(pc, e);
		e->inst[1] |= (sub << 29);
	}

	set_dst(pc, dst, e);
	set_src_0(pc, src, e);

	emit(pc, e);
}

static void
emit_preex2(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] |= 0xb0000000;

	set_dst(pc, dst, e);
	set_src_0(pc, src, e);
	set_long(pc, e);
	e->inst[1] |= (6 << 29) | 0x00004000;

	emit(pc, e);
}

static void
emit_precossin(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] |= 0xb0000000;

	set_dst(pc, dst, e);
	set_src_0(pc, src, e);
	set_long(pc, e);
	e->inst[1] |= (6 << 29);

	emit(pc, e);
}

static void
emit_set(struct nv50_pc *pc, unsigned c_op, struct nv50_reg *dst,
	 struct nv50_reg *src0, struct nv50_reg *src1)
{
	struct nv50_program_exec *e = exec(pc);
	unsigned inv_cop[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };
	struct nv50_reg *rdst;

	assert(c_op <= 7);
	if (check_swap_src_0_1(pc, &src0, &src1))
		c_op = inv_cop[c_op];

	rdst = dst;
	if (dst->type != P_TEMP)
		dst = alloc_temp(pc, NULL);

	/* set.u32 */
	set_long(pc, e);
	e->inst[0] |= 0xb0000000;
	e->inst[1] |= (3 << 29);
	e->inst[1] |= (c_op << 14);
	/*XXX: breaks things, .u32 by default?
	 *     decuda will disasm as .u16 and use .lo/.hi regs, but this
	 *     doesn't seem to match what the hw actually does.
	inst[1] |= 0x04000000; << breaks things.. .u32 by default?
	 */
	set_dst(pc, dst, e);
	set_src_0(pc, src0, e);
	set_src_1(pc, src1, e);
	emit(pc, e);

	/* cvt.f32.u32 */
	e = exec(pc);
	e->inst[0] = 0xa0000001;
	e->inst[1] = 0x64014780;
	set_dst(pc, rdst, e);
	set_src_0(pc, dst, e);
	emit(pc, e);

	if (dst != rdst)
		free_temp(pc, dst);
}

static void
emit_flr(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] = 0xa0000000; /* cvt */
	set_long(pc, e);
	e->inst[1] |= (6 << 29); /* cvt */
	e->inst[1] |= 0x08000000; /* integer mode */
	e->inst[1] |= 0x04000000; /* 32 bit */
	e->inst[1] |= ((0x1 << 3)) << 14; /* .rn */
	e->inst[1] |= (1 << 14); /* src .f32 */
	set_dst(pc, dst, e);
	set_src_0(pc, src, e);

	emit(pc, e);
}

static void
emit_pow(struct nv50_pc *pc, struct nv50_reg *dst,
	 struct nv50_reg *v, struct nv50_reg *e)
{
	struct nv50_reg *temp = alloc_temp(pc, NULL);

	emit_flop(pc, 3, temp, v);
	emit_mul(pc, temp, temp, e);
	emit_preex2(pc, temp, temp);
	emit_flop(pc, 6, dst, temp);

	free_temp(pc, temp);
}

static void
emit_abs(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] = 0xa0000000; /* cvt */
	set_long(pc, e);
	e->inst[1] |= (6 << 29); /* cvt */
	e->inst[1] |= 0x04000000; /* 32 bit */
	e->inst[1] |= (1 << 14); /* src .f32 */
	e->inst[1] |= ((1 << 6) << 14); /* .abs */
	set_dst(pc, dst, e);
	set_src_0(pc, src, e);

	emit(pc, e);
}

static void
emit_lit(struct nv50_pc *pc, struct nv50_reg **dst, unsigned mask,
	 struct nv50_reg **src)
{
	struct nv50_reg *one = alloc_immd(pc, 1.0);
	struct nv50_reg *zero = alloc_immd(pc, 0.0);
	struct nv50_reg *neg128 = alloc_immd(pc, -127.999999);
	struct nv50_reg *pos128 = alloc_immd(pc,  127.999999);
	struct nv50_reg *tmp[4];

	if (mask & (1 << 0))
		emit_mov(pc, dst[0], one);

	if (mask & (1 << 3))
		emit_mov(pc, dst[3], one);

	if (mask & (3 << 1)) {
		if (mask & (1 << 1))
			tmp[0] = dst[1];
		else
			tmp[0] = temp_temp(pc);
		emit_minmax(pc, 4, tmp[0], src[0], zero);
	}

	if (mask & (1 << 2)) {
		set_pred_wr(pc, 1, 0, pc->p->exec_tail);

		tmp[1] = temp_temp(pc);
		emit_minmax(pc, 4, tmp[1], src[1], zero);

		tmp[3] = temp_temp(pc);
		emit_minmax(pc, 4, tmp[3], src[3], neg128);
		emit_minmax(pc, 5, tmp[3], tmp[3], pos128);

		emit_pow(pc, dst[2], tmp[1], tmp[3]);
		emit_mov(pc, dst[2], zero);
		set_pred(pc, 3, 0, pc->p->exec_tail);
	}
}

static void
emit_neg(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src)
{
	struct nv50_program_exec *e = exec(pc);

	set_long(pc, e);
	e->inst[0] |= 0xa0000000; /* delta */
	e->inst[1] |= (7 << 29); /* delta */
	e->inst[1] |= 0x04000000; /* negate arg0? probably not */
	e->inst[1] |= (1 << 14); /* src .f32 */
	set_dst(pc, dst, e);
	set_src_0(pc, src, e);

	emit(pc, e);
}

static void
emit_kil(struct nv50_pc *pc, struct nv50_reg *src)
{
	struct nv50_program_exec *e;
	const int r_pred = 1;

	/* Sets predicate reg ? */
	e = exec(pc);
	e->inst[0] = 0xa00001fd;
	e->inst[1] = 0xc4014788;
	set_src_0(pc, src, e);
	set_pred_wr(pc, 1, r_pred, e);
	emit(pc, e);

	/* This is probably KILP */
	e = exec(pc);
	e->inst[0] = 0x000001fe;
	set_long(pc, e);
	set_pred(pc, 1 /* LT? */, r_pred, e);
	emit(pc, e);
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
tgsi_src(struct nv50_pc *pc, int chan, const struct tgsi_full_src_register *src)
{
	struct nv50_reg *r = NULL;
	struct nv50_reg *temp;
	unsigned c;

	c = tgsi_util_get_full_src_register_extswizzle(src, chan);
	switch (c) {
	case TGSI_EXTSWIZZLE_X:
	case TGSI_EXTSWIZZLE_Y:
	case TGSI_EXTSWIZZLE_Z:
	case TGSI_EXTSWIZZLE_W:
		switch (src->SrcRegister.File) {
		case TGSI_FILE_INPUT:
			r = &pc->attr[src->SrcRegister.Index * 4 + c];
			break;
		case TGSI_FILE_TEMPORARY:
			r = &pc->temp[src->SrcRegister.Index * 4 + c];
			break;
		case TGSI_FILE_CONSTANT:
			r = &pc->param[src->SrcRegister.Index * 4 + c];
			break;
		case TGSI_FILE_IMMEDIATE:
			r = &pc->immd[src->SrcRegister.Index * 4 + c];
			break;
		case TGSI_FILE_SAMPLER:
			break;
		default:
			assert(0);
			break;
		}
		break;
	case TGSI_EXTSWIZZLE_ZERO:
		r = alloc_immd(pc, 0.0);
		break;
	case TGSI_EXTSWIZZLE_ONE:
		r = alloc_immd(pc, 1.0);
		break;
	default:
		assert(0);
		break;
	}

	switch (tgsi_util_get_full_src_register_sign_mode(src, chan)) {
	case TGSI_UTIL_SIGN_KEEP:
		break;
	case TGSI_UTIL_SIGN_CLEAR:
		temp = temp_temp(pc);
		emit_abs(pc, temp, r);
		r = temp;
		break;
	case TGSI_UTIL_SIGN_TOGGLE:
		temp = temp_temp(pc);
		emit_neg(pc, temp, r);
		r = temp;
		break;
	case TGSI_UTIL_SIGN_SET:
		temp = temp_temp(pc);
		emit_abs(pc, temp, r);
		emit_neg(pc, temp, r);
		r = temp;
		break;
	default:
		assert(0);
		break;
	}

	return r;
}

static boolean
nv50_program_tx_insn(struct nv50_pc *pc, const union tgsi_full_token *tok)
{
	const struct tgsi_full_instruction *inst = &tok->FullInstruction;
	struct nv50_reg *rdst[4], *dst[4], *src[3][4], *temp;
	unsigned mask, sat, unit;
	int i, c;

	mask = inst->FullDstRegisters[0].DstRegister.WriteMask;
	sat = inst->Instruction.Saturate == TGSI_SAT_ZERO_ONE;

	for (c = 0; c < 4; c++) {
		if (mask & (1 << c))
			dst[c] = tgsi_dst(pc, c, &inst->FullDstRegisters[0]);
		else
			dst[c] = NULL;
	}

	for (i = 0; i < inst->Instruction.NumSrcRegs; i++) {
		const struct tgsi_full_src_register *fs = &inst->FullSrcRegisters[i];

		if (fs->SrcRegister.File == TGSI_FILE_SAMPLER)
			unit = fs->SrcRegister.Index;

		for (c = 0; c < 4; c++)
			src[i][c] = tgsi_src(pc, c, fs);
	}

	if (sat) {
		for (c = 0; c < 4; c++) {
			rdst[c] = dst[c];
			dst[c] = temp_temp(pc);
		}
	}

	switch (inst->Instruction.Opcode) {
	case TGSI_OPCODE_ABS:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_abs(pc, dst[c], src[0][c]);
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
		temp = alloc_temp(pc, NULL);
		emit_precossin(pc, temp, src[0][0]);
		emit_flop(pc, 5, temp, temp);
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_mov(pc, dst[c], temp);
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
		if (mask & (1 << 0))
			emit_mov(pc, dst[0], one);
		if (mask & (1 << 1))
			emit_mul(pc, dst[1], src[0][1], src[1][1]);
		if (mask & (1 << 2))
			emit_mov(pc, dst[2], src[0][2]);
		if (mask & (1 << 3))
			emit_mov(pc, dst[3], src[1][3]);
		FREE(one);
	}
		break;
	case TGSI_OPCODE_EX2:
		temp = alloc_temp(pc, NULL);
		emit_preex2(pc, temp, src[0][0]);
		emit_flop(pc, 6, temp, temp);
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_mov(pc, dst[c], temp);
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
	case TGSI_OPCODE_KIL:
		emit_kil(pc, src[0][0]);
		emit_kil(pc, src[0][1]);
		emit_kil(pc, src[0][2]);
		emit_kil(pc, src[0][3]);
		break;
	case TGSI_OPCODE_LIT:
		emit_lit(pc, &dst[0], mask, &src[0][0]);
		break;
	case TGSI_OPCODE_LG2:
		temp = alloc_temp(pc, NULL);
		emit_flop(pc, 3, temp, src[0][0]);
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_mov(pc, dst[c], temp);
		}
		break;
	case TGSI_OPCODE_LRP:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			/*XXX: we can do better than this */
			temp = alloc_temp(pc, NULL);
			emit_neg(pc, temp, src[0][c]);
			emit_mad(pc, temp, temp, src[2][c], src[2][c]);
			emit_mad(pc, dst[c], src[0][c], src[1][c], temp);
			free_temp(pc, temp);
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
	case TGSI_OPCODE_POW:
		temp = alloc_temp(pc, NULL);
		emit_pow(pc, temp, src[0][0], src[1][0]);
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_mov(pc, dst[c], temp);
		}
		free_temp(pc, temp);
		break;
	case TGSI_OPCODE_RCP:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_flop(pc, 0, dst[c], src[0][0]);
		}
		break;
	case TGSI_OPCODE_RSQ:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_flop(pc, 2, dst[c], src[0][0]);
		}
		break;
	case TGSI_OPCODE_SCS:
		temp = alloc_temp(pc, NULL);
		emit_precossin(pc, temp, src[0][0]);
		if (mask & (1 << 0))
			emit_flop(pc, 5, dst[0], temp);
		if (mask & (1 << 1))
			emit_flop(pc, 4, dst[1], temp);
		break;
	case TGSI_OPCODE_SGE:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_set(pc, 6, dst[c], src[0][c], src[1][c]);
		}
		break;
	case TGSI_OPCODE_SIN:
		temp = alloc_temp(pc, NULL);
		emit_precossin(pc, temp, src[0][0]);
		emit_flop(pc, 4, temp, temp);
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_mov(pc, dst[c], temp);
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
	case TGSI_OPCODE_TEX:
	case TGSI_OPCODE_TXP:
	{
		struct nv50_reg *t[4];
		struct nv50_program_exec *e;

		alloc_temp4(pc, t, 0);
		emit_mov(pc, t[0], src[0][0]);
		emit_mov(pc, t[1], src[0][1]);

		e = exec(pc);
		e->inst[0] = 0xf6400000;
		e->inst[0] |= (unit << 9);
		set_long(pc, e);
		e->inst[1] |= 0x0000c004;
		set_dst(pc, t[0], e);
		emit(pc, e);

		if (mask & (1 << 0)) emit_mov(pc, dst[0], t[0]);
		if (mask & (1 << 1)) emit_mov(pc, dst[1], t[1]);
		if (mask & (1 << 2)) emit_mov(pc, dst[2], t[2]);
		if (mask & (1 << 3)) emit_mov(pc, dst[3], t[3]);

		free_temp4(pc, t);
	}
		break;
	case TGSI_OPCODE_XPD:
		temp = alloc_temp(pc, NULL);
		if (mask & (1 << 0)) {
			emit_mul(pc, temp, src[0][2], src[1][1]);
			emit_msb(pc, dst[0], src[0][1], src[1][2], temp);
		}
		if (mask & (1 << 1)) {
			emit_mul(pc, temp, src[0][0], src[1][2]);
			emit_msb(pc, dst[1], src[0][2], src[1][0], temp);
		}
		if (mask & (1 << 2)) {
			emit_mul(pc, temp, src[0][1], src[1][0]);
			emit_msb(pc, dst[2], src[0][0], src[1][1], temp);
		}
		free_temp(pc, temp);
		break;
	case TGSI_OPCODE_END:
		break;
	default:
		NOUVEAU_ERR("invalid opcode %d\n", inst->Instruction.Opcode);
		return FALSE;
	}

	if (sat) {
		for (c = 0; c < 4; c++) {
			struct nv50_program_exec *e;

			if (!(mask & (1 << c)))
				continue;
			e = exec(pc);

			e->inst[0] = 0xa0000000; /* cvt */
			set_long(pc, e);
			e->inst[1] |= (6 << 29); /* cvt */
			e->inst[1] |= 0x04000000; /* 32 bit */
			e->inst[1] |= (1 << 14); /* src .f32 */
			e->inst[1] |= ((1 << 5) << 14); /* .sat */
			set_dst(pc, rdst[c], e);
			set_src_0(pc, dst[c], e);
			emit(pc, e);
		}
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
			last = d->DeclarationRange.Last;

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
			case TGSI_FILE_SAMPLER:
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

	if (pc->temp_nr) {
		pc->temp = CALLOC(pc->temp_nr * 4, sizeof(struct nv50_reg));
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

	if (pc->attr_nr) {
		struct nv50_reg *iv = NULL;
		int aid = 0;

		pc->attr = CALLOC(pc->attr_nr * 4, sizeof(struct nv50_reg));
		if (!pc->attr)
			goto out_err;

		if (pc->p->type == PIPE_SHADER_FRAGMENT) {
			iv = alloc_temp(pc, NULL);
			emit_interp(pc, iv, iv, NULL);
			emit_flop(pc, 0, iv, iv);
			aid++;
		}

		for (i = 0; i < pc->attr_nr; i++) {
			struct nv50_reg *a = &pc->attr[i*4];

			for (c = 0; c < 4; c++) {
				if (pc->p->type == PIPE_SHADER_FRAGMENT) {
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

			if (pc->p->type != PIPE_SHADER_FRAGMENT)
				continue;

			emit_interp(pc, &a[0], &a[0], iv);
			emit_interp(pc, &a[1], &a[1], iv);
			emit_interp(pc, &a[2], &a[2], iv);
			emit_interp(pc, &a[3], &a[3], iv);
		}

		if (iv)
			free_temp(pc, iv);
	}

	if (pc->result_nr) {
		int rid = 0;

		pc->result = CALLOC(pc->result_nr * 4, sizeof(struct nv50_reg));
		if (!pc->result)
			goto out_err;

		for (i = 0; i < pc->result_nr; i++) {
			for (c = 0; c < 4; c++) {
				if (pc->p->type == PIPE_SHADER_FRAGMENT) {
					pc->result[i*4+c].type = P_TEMP;
					pc->result[i*4+c].hw = -1;
				} else {
					pc->result[i*4+c].type = P_RESULT;
					pc->result[i*4+c].hw = rid++;
				}
				pc->result[i*4+c].index = i;
			}
		}
	}

	if (pc->param_nr) {
		int rid = 0;

		pc->param = CALLOC(pc->param_nr * 4, sizeof(struct nv50_reg));
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
		int rid = pc->param_nr * 4;

		pc->immd = CALLOC(pc->immd_nr * 4, sizeof(struct nv50_reg));
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

	if (p->type == PIPE_SHADER_FRAGMENT) {
		struct nv50_reg out;

		out.type = P_TEMP;
		for (out.hw = 0; out.hw < pc->result_nr * 4; out.hw++)
			emit_mov(pc, &out, &pc->result[out.hw]);
	}

	assert(is_long(pc->p->exec_tail) && !is_immd(pc->p->exec_head));
	pc->p->exec_tail->inst[1] |= 0x00000001;

	p->param_nr = pc->param_nr * 4;
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
	if (nv50_program_tx(p) == FALSE)
		assert(0);
	p->translated = TRUE;
}

static void
nv50_program_upload_data(struct nv50_context *nv50, float *map,
			 unsigned start, unsigned count)
{
	struct nouveau_channel *chan = nv50->screen->nvws->channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;

	while (count) {
		unsigned nr = count > 2047 ? 2047 : count;

		BEGIN_RING(chan, tesla, 0x00000f00, 1);
		OUT_RING  (chan, (NV50_CB_PMISC << 0) | (start << 8));
		BEGIN_RING(chan, tesla, 0x40000f04, nr);
		OUT_RINGp (chan, map, nr);

		map += nr;
		start += nr;
		count -= nr;
	}
}

static void
nv50_program_validate_data(struct nv50_context *nv50, struct nv50_program *p)
{
	struct nouveau_winsys *nvws = nv50->screen->nvws;
	struct pipe_winsys *ws = nv50->pipe.winsys;
	unsigned nr = p->param_nr + p->immd_nr;

	if (!p->data && nr) {
		struct nouveau_resource *heap = nv50->screen->vp_data_heap;

		if (nvws->res_alloc(heap, nr, p, &p->data)) {
			while (heap->next && heap->size < nr) {
				struct nv50_program *evict = heap->next->priv;
				nvws->res_free(&evict->data);
			}

			if (nvws->res_alloc(heap, nr, p, &p->data))
				assert(0);
		}
	}

	if (p->param_nr) {
		float *map = ws->buffer_map(ws, nv50->constbuf[p->type],
					    PIPE_BUFFER_USAGE_CPU_READ);
		nv50_program_upload_data(nv50, map, p->data->start,
					 p->param_nr);
		ws->buffer_unmap(ws, nv50->constbuf[p->type]);
	}

	if (p->immd_nr) {
		nv50_program_upload_data(nv50, p->immd,
					 p->data->start + p->param_nr,
					 p->immd_nr);
	}
}

static void
nv50_program_validate_code(struct nv50_context *nv50, struct nv50_program *p)
{
	struct nouveau_channel *chan = nv50->screen->nvws->channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct pipe_screen *screen = nv50->pipe.screen;
	struct nv50_program_exec *e;
	struct nouveau_stateobj *so;
	const unsigned flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_WR;
	unsigned start, count, *up, *ptr;
	boolean upload = FALSE;

	if (!p->buffer) {
		p->buffer = screen->buffer_create(screen, 0x100, 0, p->exec_size * 4);
		upload = TRUE;
	}

	if (p->data && p->data->start != p->data_start) {
		for (e = p->exec_head; e; e = e->next) {
			unsigned ei, ci;

			if (e->param.index < 0)
				continue;
			ei = e->param.shift >> 5;
			ci = e->param.index + p->data->start;

			e->inst[ei] &= ~e->param.mask;
			e->inst[ei] |= (ci << e->param.shift);
		}

		p->data_start = p->data->start;
		upload = TRUE;
	}

	if (!upload)
		return;

#ifdef NV50_PROGRAM_DUMP
	NOUVEAU_ERR("-------\n");
	up = ptr = MALLOC(p->exec_size * 4);
	for (e = p->exec_head; e; e = e->next) {
		NOUVEAU_ERR("0x%08x\n", e->inst[0]);
		if (is_long(e))
			NOUVEAU_ERR("0x%08x\n", e->inst[1]);
	}

#endif

	up = ptr = MALLOC(p->exec_size * 4);
	for (e = p->exec_head; e; e = e->next) {
		*(ptr++) = e->inst[0];
		if (is_long(e))
			*(ptr++) = e->inst[1];
	}

	so = so_new(4,2);
	so_method(so, nv50->screen->tesla, 0x1280, 3);
	so_reloc (so, p->buffer, 0, flags | NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, p->buffer, 0, flags | NOUVEAU_BO_LOW, 0, 0);
	so_data  (so, (NV50_CB_PUPLOAD << 16) | 0x0800); //(p->exec_size * 4));

	start = 0; count = p->exec_size;
	while (count) {
		struct nouveau_winsys *nvws = nv50->screen->nvws;
		unsigned nr;

		so_emit(nvws, so);

		nr = MIN2(count, 2047);
		nr = MIN2(nvws->channel->pushbuf->remaining, nr);
		if (nvws->channel->pushbuf->remaining < (nr + 3)) {
			FIRE_RING(chan);
			continue;
		}

		BEGIN_RING(chan, tesla, 0x0f00, 1);
		OUT_RING  (chan, (start << 8) | NV50_CB_PUPLOAD);
		BEGIN_RING(chan, tesla, 0x40000f04, nr);	
		OUT_RINGp (chan, up + start, nr);

		start += nr;
		count -= nr;
	}

	FREE(up);
	so_ref(NULL, &so);
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

	so = so_new(13, 2);
	so_method(so, tesla, NV50TCL_VP_ADDRESS_HIGH, 2);
	so_reloc (so, p->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
		  NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, p->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
		  NOUVEAU_BO_LOW, 0, 0);
	so_method(so, tesla, 0x1650, 2);
	so_data  (so, p->cfg.vp.attr[0]);
	so_data  (so, p->cfg.vp.attr[1]);
	so_method(so, tesla, 0x16b8, 1);
	so_data  (so, p->cfg.high_result);
	so_method(so, tesla, 0x16ac, 2);
	so_data  (so, p->cfg.high_result); //8);
	so_data  (so, p->cfg.high_temp);
	so_method(so, tesla, 0x140c, 1);
	so_data  (so, 0); /* program start offset */
	so_ref(so, &nv50->state.vertprog);
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

	so = so_new(64, 2);
	so_method(so, tesla, NV50TCL_FP_ADDRESS_HIGH, 2);
	so_reloc (so, p->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
		  NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, p->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
		  NOUVEAU_BO_LOW, 0, 0);
	so_method(so, tesla, 0x1904, 4);
	so_data  (so, 0x00040404); /* p: 0x01000404 */
	so_data  (so, 0x00000004);
	so_data  (so, 0x00000000);
	so_data  (so, 0x00000000);
	so_method(so, tesla, 0x16bc, 3); /*XXX: fixme */
	so_data  (so, 0x03020100);
	so_data  (so, 0x07060504);
	so_data  (so, 0x0b0a0908);
	so_method(so, tesla, 0x1988, 2);
	so_data  (so, 0x08080408); //0x08040404); /* p: 0x0f000401 */
	so_data  (so, p->cfg.high_temp);
	so_method(so, tesla, 0x1414, 1);
	so_data  (so, 0); /* program start offset */
	so_ref(so, &nv50->state.fragprog);
	so_ref(NULL, &so);
}

void
nv50_program_destroy(struct nv50_context *nv50, struct nv50_program *p)
{
	struct pipe_screen *pscreen = nv50->pipe.screen;

	while (p->exec_head) {
		struct nv50_program_exec *e = p->exec_head;

		p->exec_head = e->next;
		FREE(e);
	}
	p->exec_tail = NULL;
	p->exec_size = 0;

	if (p->buffer)
		pipe_buffer_reference(&p->buffer, NULL);

	nv50->screen->nvws->res_free(&p->data);

	p->translated = 0;
}

