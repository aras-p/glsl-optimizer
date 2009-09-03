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

	int rhw; /* result hw for FP outputs, or interpolant index */
	int acc; /* instruction where this reg is last read (first insn == 1) */
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

	unsigned interp_mode[32];
	/* perspective interpolation registers */
	struct nv50_reg *iv_p;
	struct nv50_reg *iv_c;

	/* current instruction and total number of insns */
	unsigned insn_cur;
	unsigned insn_nr;

	boolean allow32;
};

static void
alloc_reg(struct nv50_pc *pc, struct nv50_reg *reg)
{
	int i = 0;

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

	if (reg->rhw != -1) {
		/* try to allocate temporary with index rhw first */
		if (!(pc->r_temp[reg->rhw])) {
			pc->r_temp[reg->rhw] = reg;
			reg->hw = reg->rhw;
			if (pc->p->cfg.high_temp < (reg->rhw + 1))
				pc->p->cfg.high_temp = reg->rhw + 1;
			return;
		}
		/* make sure we don't get things like $r0 needs to go
		 * in $r1 and $r1 in $r0
		 */
		i = pc->result_nr * 4;
	}

	for (; i < NV50_SU_MAX_TEMP; i++) {
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
			r->rhw = -1;
			pc->r_temp[i] = r;
			return r;
		}
	}

	assert(0);
	return NULL;
}

/* Assign the hw of the discarded temporary register src
 * to the tgsi register dst and free src.
 */
static void
assimilate_temp(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src)
{
	assert(src->index == -1 && src->hw != -1);

	if (dst->hw != -1)
		pc->r_temp[dst->hw] = NULL;
	pc->r_temp[src->hw] = dst;
	dst->hw = src->hw;

	FREE(src);
}

/* release the hardware resource held by r */
static void
release_hw(struct nv50_pc *pc, struct nv50_reg *r)
{
	assert(r->type == P_TEMP);
	if (r->hw == -1)
		return;

	assert(pc->r_temp[r->hw] == r);
	pc->r_temp[r->hw] = NULL;

	r->acc = 0;
	if (r->index == -1)
		FREE(r);
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
		return alloc_temp4(pc, dst, idx + 4);

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
	pc->immd_buf = REALLOC(pc->immd_buf, (pc->immd_nr * 4 * sizeof(float)),
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

	for (hw = 0; hw < pc->immd_nr * 4; hw++)
		if (pc->immd_buf[hw] == f)
			break;

	if (hw == pc->immd_nr * 4)
		hw = ctor_immd(pc, f, -f, 0.5 * f, 0) * 4;

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
	float f = pc->immd_buf[imm->hw];
	unsigned val = fui(imm->neg ? -f : f);

	set_long(pc, e);
	/*XXX: can't be predicated - bits overlap.. catch cases where both
	 *     are required and avoid them. */
	set_pred(pc, 0, 0, e);
	set_pred_wr(pc, 0, 0, e);

	e->inst[1] |= 0x00000002 | 0x00000001;
	e->inst[0] |= (val & 0x3f) << 16;
	e->inst[1] |= (val >> 6) << 2;
}


#define INTERP_LINEAR		0
#define INTERP_FLAT			1
#define INTERP_PERSPECTIVE	2
#define INTERP_CENTROID		4

/* interpolant index has been stored in dst->rhw */
static void
emit_interp(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *iv,
		unsigned mode)
{
	assert(dst->rhw != -1);
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] |= 0x80000000;
	set_dst(pc, dst, e);
	e->inst[0] |= (dst->rhw << 16);

	if (mode & INTERP_FLAT) {
		e->inst[0] |= (1 << 8);
	} else {
		if (mode & INTERP_PERSPECTIVE) {
			e->inst[0] |= (1 << 25);
			alloc_reg(pc, iv);
			e->inst[0] |= (iv->hw << 9);
		}

		if (mode & INTERP_CENTROID)
			e->inst[0] |= (1 << 24);
	}

	emit(pc, e);
}

static void
set_data(struct nv50_pc *pc, struct nv50_reg *src, unsigned m, unsigned s,
	 struct nv50_program_exec *e)
{
	set_long(pc, e);

	e->param.index = src->hw;
	e->param.shift = s;
	e->param.mask = m << (s % 32);

	e->inst[1] |= (((src->type == P_IMMD) ? 0 : 1) << 22);
}

static void
emit_mov(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] |= 0x10000000;

	set_dst(pc, dst, e);

	if (pc->allow32 && dst->type != P_RESULT && src->type == P_IMMD) {
		set_immd(pc, src, e);
		/*XXX: 32-bit, but steals part of "half" reg space - need to
		 *     catch and handle this case if/when we do half-regs
		 */
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

	if (is_long(e) && !is_immd(e)) {
		e->inst[1] |= 0x04000000; /* 32-bit */
		e->inst[1] |= 0x0000c000; /* "subsubop" 0x3 */
		if (!(e->inst[1] & 0x20000000))
			e->inst[1] |= 0x00030000; /* "subsubop" 0xf */
	} else
		e->inst[0] |= 0x00008000;

	emit(pc, e);
}

static INLINE void
emit_mov_immdval(struct nv50_pc *pc, struct nv50_reg *dst, float f)
{
	struct nv50_reg *imm = alloc_immd(pc, f);
	emit_mov(pc, dst, imm);
	FREE(imm);
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

	if (!pc->allow32)
		set_long(pc, e);

	check_swap_src_0_1(pc, &src0, &src1);
	set_dst(pc, dst, e);
	set_src_0(pc, src0, e);
	if (src1->type == P_IMMD && !is_long(e)) {
		if (src0->neg)
			e->inst[0] |= 0x00008000;
		set_immd(pc, src1, e);
	} else {
		set_src_1(pc, src1, e);
		if (src0->neg ^ src1->neg) {
			if (is_long(e))
				e->inst[1] |= 0x08000000;
			else
				e->inst[0] |= 0x00008000;
		}
	}

	emit(pc, e);
}

static void
emit_add(struct nv50_pc *pc, struct nv50_reg *dst,
	 struct nv50_reg *src0, struct nv50_reg *src1)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] |= 0xb0000000;

	check_swap_src_0_1(pc, &src0, &src1);

	if (!pc->allow32 || src0->neg || src1->neg) {
		set_long(pc, e);
		e->inst[1] |= (src0->neg << 26) | (src1->neg << 27);
	}

	set_dst(pc, dst, e);
	set_src_0(pc, src0, e);
	if (src1->type == P_CONST || src1->type == P_ATTR || is_long(e))
		set_src_2(pc, src1, e);
	else
	if (src1->type == P_IMMD)
		set_immd(pc, src1, e);
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

static INLINE void
emit_sub(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src0,
	 struct nv50_reg *src1)
{
	src1->neg ^= 1;
	emit_add(pc, dst, src0, src1);
	src1->neg ^= 1;
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

	if (src0->neg ^ src1->neg)
		e->inst[1] |= 0x04000000;
	if (src2->neg)
		e->inst[1] |= 0x08000000;

	emit(pc, e);
}

static INLINE void
emit_msb(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src0,
	 struct nv50_reg *src1, struct nv50_reg *src2)
{
	src2->neg ^= 1;
	emit_mad(pc, dst, src0, src1, src2);
	src2->neg ^= 1;
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

#define CVTOP_RN	0x01
#define CVTOP_FLOOR	0x03
#define CVTOP_CEIL	0x05
#define CVTOP_TRUNC	0x07
#define CVTOP_SAT	0x08
#define CVTOP_ABS	0x10

#define CVT_F32_F32 0xc4
#define CVT_F32_S32 0x44
#define CVT_F32_U32 0x64
#define CVT_S32_F32 0x8c
#define CVT_S32_S32 0x0c
#define CVT_F32_F32_ROP 0xcc

static void
emit_cvt(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src,
	 int wp, unsigned cop, unsigned fmt)
{
	struct nv50_program_exec *e;

	e = exec(pc);
	set_long(pc, e);

	e->inst[0] |= 0xa0000000;
	e->inst[1] |= 0x00004000;
	e->inst[1] |= (cop << 16);
	e->inst[1] |= (fmt << 24);
	set_src_0(pc, src, e);

	if (wp >= 0)
		set_pred_wr(pc, 1, wp, e);

	if (dst)
		set_dst(pc, dst, e);
	else {
		e->inst[0] |= 0x000001fc;
		e->inst[1] |= 0x00000008;
	}

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

static INLINE void
emit_flr(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src)
{
	emit_cvt(pc, dst, src, -1, CVTOP_FLOOR, CVT_F32_F32_ROP);
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

static INLINE void
emit_abs(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src)
{
	emit_cvt(pc, dst, src, -1, CVTOP_ABS, CVT_F32_F32);
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
	boolean allow32 = pc->allow32;

	pc->allow32 = FALSE;

	if (mask & (3 << 1)) {
		tmp[0] = alloc_temp(pc, NULL);
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

	if (mask & (1 << 1))
		assimilate_temp(pc, dst[1], tmp[0]);
	else
	if (mask & (1 << 2))
		free_temp(pc, tmp[0]);

	pc->allow32 = allow32;

	/* do this last, in case src[i,j] == dst[0,3] */
	if (mask & (1 << 0))
		emit_mov(pc, dst[0], one);

	if (mask & (1 << 3))
		emit_mov(pc, dst[3], one);

	FREE(pos128);
	FREE(neg128);
	FREE(zero);
	FREE(one);
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
	if (src->neg)
		e->inst[1] |= 0x20000000;
	emit(pc, e);

	/* This is probably KILP */
	e = exec(pc);
	e->inst[0] = 0x000001fe;
	set_long(pc, e);
	set_pred(pc, 1 /* LT? */, r_pred, e);
	emit(pc, e);
}

static void
emit_tex(struct nv50_pc *pc, struct nv50_reg **dst, unsigned mask,
	 struct nv50_reg **src, unsigned unit, unsigned type, boolean proj)
{
	struct nv50_reg *temp, *t[4];
	struct nv50_program_exec *e;

	unsigned c, mode, dim;

	switch (type) {
	case TGSI_TEXTURE_1D:
		dim = 1;
		break;
	case TGSI_TEXTURE_UNKNOWN:
	case TGSI_TEXTURE_2D:
	case TGSI_TEXTURE_SHADOW1D: /* XXX: x, z */
	case TGSI_TEXTURE_RECT:
		dim = 2;
		break;
	case TGSI_TEXTURE_3D:
	case TGSI_TEXTURE_CUBE:
	case TGSI_TEXTURE_SHADOW2D:
	case TGSI_TEXTURE_SHADOWRECT: /* XXX */
		dim = 3;
		break;
	default:
		assert(0);
		break;
	}

	/* some cards need t[0]'s hw index to be a multiple of 4 */
	alloc_temp4(pc, t, 0);

	if (proj) {
		if (src[0]->type == P_TEMP && src[0]->rhw != -1) {
			mode = pc->interp_mode[src[0]->index];

			t[3]->rhw = src[3]->rhw;
			emit_interp(pc, t[3], NULL, (mode & INTERP_CENTROID));
			emit_flop(pc, 0, t[3], t[3]);

			for (c = 0; c < dim; c++) {
				t[c]->rhw = src[c]->rhw;
				emit_interp(pc, t[c], t[3],
					    (mode | INTERP_PERSPECTIVE));
			}
		} else {
			emit_flop(pc, 0, t[3], src[3]);
			for (c = 0; c < dim; c++)
				emit_mul(pc, t[c], src[c], t[3]);

			/* XXX: for some reason the blob sometimes uses MAD:
			 * emit_mad(pc, t[c], src[0][c], t[3], t[3])
			 * pc->p->exec_tail->inst[1] |= 0x080fc000;
			 */
		}
	} else {
		if (type == TGSI_TEXTURE_CUBE) {
			temp = temp_temp(pc);
			emit_minmax(pc, 4, temp, src[0], src[1]);
			emit_minmax(pc, 4, temp, temp, src[2]);
			emit_flop(pc, 0, temp, temp);
			for (c = 0; c < 3; c++)
				emit_mul(pc, t[c], src[c], temp);
		} else {
			for (c = 0; c < dim; c++)
				emit_mov(pc, t[c], src[c]);
		}
	}

	e = exec(pc);
	set_long(pc, e);
	e->inst[0] |= 0xf0000000;
	e->inst[1] |= 0x00000004;
	set_dst(pc, t[0], e);
	e->inst[0] |= (unit << 9);

	if (dim == 2)
		e->inst[0] |= 0x00400000;
	else
	if (dim == 3)
		e->inst[0] |= 0x00800000;

	e->inst[0] |= (mask & 0x3) << 25;
	e->inst[1] |= (mask & 0xc) << 12;

	emit(pc, e);

#if 1
	if (mask & 1) emit_mov(pc, dst[0], t[0]);
	if (mask & 2) emit_mov(pc, dst[1], t[1]);
	if (mask & 4) emit_mov(pc, dst[2], t[2]);
	if (mask & 8) emit_mov(pc, dst[3], t[3]);

	free_temp4(pc, t);
#else
	/* XXX: if p.e. MUL is used directly after TEX, it would still use
	 * the texture coordinates, not the fetched values: latency ? */

	for (c = 0; c < 4; c++) {
		if (mask & (1 << c))
			assimilate_temp(pc, dst[c], t[c]);
		else
			free_temp(pc, t[c]);
	}
#endif
}

static void
convert_to_long(struct nv50_pc *pc, struct nv50_program_exec *e)
{
	unsigned q = 0, m = ~0;

	assert(!is_long(e));

	switch (e->inst[0] >> 28) {
	case 0x1:
		/* MOV */
		q = 0x0403c000;
		m = 0xffff7fff;
		break;
	case 0x8:
		/* INTERP (move centroid, perspective and flat bits) */
		m = ~0x03000100;
		q = (e->inst[0] & (3 << 24)) >> (24 - 16);
		q |= (e->inst[0] & (1 << 8)) << (18 - 8);
		break;
	case 0x9:
		/* RCP */
		break;
	case 0xB:
		/* ADD */
		m = ~(127 << 16);
		q = ((e->inst[0] & (~m)) >> 2);
		break;
	case 0xC:
		/* MUL */
		m = ~0x00008000;
		q = ((e->inst[0] & (~m)) << 12);
		break;
	case 0xE:
		/* MAD (if src2 == dst) */
		q = ((e->inst[0] & 0x1fc) << 12);
		break;
	default:
		assert(0);
		break;
	}

	set_long(pc, e);
	pc->p->exec_size++;

	e->inst[0] &= m;
	e->inst[1] |= q;
}

static boolean
negate_supported(const struct tgsi_full_instruction *insn, int i)
{
	switch (insn->Instruction.Opcode) {
	case TGSI_OPCODE_DP3:
	case TGSI_OPCODE_DP4:
	case TGSI_OPCODE_MUL:
	case TGSI_OPCODE_KIL:
	case TGSI_OPCODE_ADD:
	case TGSI_OPCODE_SUB:
	case TGSI_OPCODE_MAD:
		return TRUE;
	case TGSI_OPCODE_POW:
		return (i == 1) ? TRUE : FALSE;
	default:
		return FALSE;
	}
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
tgsi_src(struct nv50_pc *pc, int chan, const struct tgsi_full_src_register *src,
	 boolean neg)
{
	struct nv50_reg *r = NULL;
	struct nv50_reg *temp;
	unsigned sgn, c;

	sgn = tgsi_util_get_full_src_register_sign_mode(src, chan);

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
		return r;
	case TGSI_EXTSWIZZLE_ONE:
		if (sgn == TGSI_UTIL_SIGN_TOGGLE || sgn == TGSI_UTIL_SIGN_SET)
			return alloc_immd(pc, -1.0);
		return alloc_immd(pc, 1.0);
	default:
		assert(0);
		break;
	}

	switch (sgn) {
	case TGSI_UTIL_SIGN_KEEP:
		break;
	case TGSI_UTIL_SIGN_CLEAR:
		temp = temp_temp(pc);
		emit_abs(pc, temp, r);
		r = temp;
		break;
	case TGSI_UTIL_SIGN_TOGGLE:
		if (neg)
			r->neg = 1;
		else {
			temp = temp_temp(pc);
			emit_neg(pc, temp, r);
			r = temp;
		}
		break;
	case TGSI_UTIL_SIGN_SET:
		temp = temp_temp(pc);
		emit_abs(pc, temp, r);
		if (neg)
			temp->neg = 1;
		else
			emit_neg(pc, temp, temp);
		r = temp;
		break;
	default:
		assert(0);
		break;
	}

	return r;
}

/* returns TRUE if instruction can overwrite sources before they're read */
static boolean
direct2dest_op(const struct tgsi_full_instruction *insn)
{
	if (insn->Instruction.Saturate)
		return FALSE;

	switch (insn->Instruction.Opcode) {
	case TGSI_OPCODE_COS:
	case TGSI_OPCODE_DP3:
	case TGSI_OPCODE_DP4:
	case TGSI_OPCODE_DPH:
	case TGSI_OPCODE_KIL:
	case TGSI_OPCODE_LIT:
	case TGSI_OPCODE_POW:
	case TGSI_OPCODE_RCP:
	case TGSI_OPCODE_RSQ:
	case TGSI_OPCODE_SCS:
	case TGSI_OPCODE_SIN:
	case TGSI_OPCODE_TEX:
	case TGSI_OPCODE_TXP:
		return FALSE;
	default:
		return TRUE;
	}
}

static boolean
nv50_program_tx_insn(struct nv50_pc *pc, const union tgsi_full_token *tok)
{
	const struct tgsi_full_instruction *inst = &tok->FullInstruction;
	struct nv50_reg *rdst[4], *dst[4], *src[3][4], *temp;
	unsigned mask, sat, unit;
	boolean assimilate = FALSE;
	int i, c;

	mask = inst->FullDstRegisters[0].DstRegister.WriteMask;
	sat = inst->Instruction.Saturate == TGSI_SAT_ZERO_ONE;

	for (c = 0; c < 4; c++) {
		if (mask & (1 << c))
			dst[c] = tgsi_dst(pc, c, &inst->FullDstRegisters[0]);
		else
			dst[c] = NULL;
		rdst[c] = NULL;
		src[0][c] = NULL;
		src[1][c] = NULL;
		src[2][c] = NULL;
	}

	for (i = 0; i < inst->Instruction.NumSrcRegs; i++) {
		const struct tgsi_full_src_register *fs = &inst->FullSrcRegisters[i];

		if (fs->SrcRegister.File == TGSI_FILE_SAMPLER)
			unit = fs->SrcRegister.Index;

		for (c = 0; c < 4; c++)
			src[i][c] = tgsi_src(pc, c, fs,
					     negate_supported(inst, i));
	}

	if (sat) {
		for (c = 0; c < 4; c++) {
			rdst[c] = dst[c];
			dst[c] = temp_temp(pc);
		}
	} else
	if (direct2dest_op(inst)) {
		for (c = 0; c < 4; c++) {
			if (!dst[c] || dst[c]->type != P_TEMP)
				continue;

			for (i = c + 1; i < 4; i++) {
				if (dst[c] == src[0][i] ||
				    dst[c] == src[1][i] ||
				    dst[c] == src[2][i])
					break;
			}
			if (i == 4)
				continue;

			assimilate = TRUE;
			rdst[c] = dst[c];
			dst[c] = alloc_temp(pc, NULL);
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
		temp = temp_temp(pc);
		emit_precossin(pc, temp, src[0][0]);
		emit_flop(pc, 5, temp, temp);
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_mov(pc, dst[c], temp);
		}
		break;
	case TGSI_OPCODE_DP3:
		temp = temp_temp(pc);
		emit_mul(pc, temp, src[0][0], src[1][0]);
		emit_mad(pc, temp, src[0][1], src[1][1], temp);
		emit_mad(pc, temp, src[0][2], src[1][2], temp);
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_mov(pc, dst[c], temp);
		}
		break;
	case TGSI_OPCODE_DP4:
		temp = temp_temp(pc);
		emit_mul(pc, temp, src[0][0], src[1][0]);
		emit_mad(pc, temp, src[0][1], src[1][1], temp);
		emit_mad(pc, temp, src[0][2], src[1][2], temp);
		emit_mad(pc, temp, src[0][3], src[1][3], temp);
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_mov(pc, dst[c], temp);
		}
		break;
	case TGSI_OPCODE_DPH:
		temp = temp_temp(pc);
		emit_mul(pc, temp, src[0][0], src[1][0]);
		emit_mad(pc, temp, src[0][1], src[1][1], temp);
		emit_mad(pc, temp, src[0][2], src[1][2], temp);
		emit_add(pc, temp, src[1][3], temp);
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_mov(pc, dst[c], temp);
		}
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
		temp = temp_temp(pc);
		emit_preex2(pc, temp, src[0][0]);
		emit_flop(pc, 6, temp, temp);
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_mov(pc, dst[c], temp);
		}
		break;
	case TGSI_OPCODE_FLR:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_flr(pc, dst[c], src[0][c]);
		}
		break;
	case TGSI_OPCODE_FRC:
		temp = temp_temp(pc);
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_flr(pc, temp, src[0][c]);
			emit_sub(pc, dst[c], src[0][c], temp);
		}
		break;
	case TGSI_OPCODE_KIL:
		emit_kil(pc, src[0][0]);
		emit_kil(pc, src[0][1]);
		emit_kil(pc, src[0][2]);
		emit_kil(pc, src[0][3]);
		pc->p->cfg.fp.regs[2] |= 0x00100000;
		break;
	case TGSI_OPCODE_LIT:
		emit_lit(pc, &dst[0], mask, &src[0][0]);
		break;
	case TGSI_OPCODE_LG2:
		temp = temp_temp(pc);
		emit_flop(pc, 3, temp, src[0][0]);
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_mov(pc, dst[c], temp);
		}
		break;
	case TGSI_OPCODE_LRP:
		temp = temp_temp(pc);
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_sub(pc, temp, src[1][c], src[2][c]);
			emit_mad(pc, dst[c], temp, src[0][c], src[2][c]);
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
	case TGSI_OPCODE_SWZ:
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
		temp = temp_temp(pc);
		emit_pow(pc, temp, src[0][0], src[1][0]);
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_mov(pc, dst[c], temp);
		}
		break;
	case TGSI_OPCODE_RCP:
		for (c = 3; c >= 0; c--) {
			if (!(mask & (1 << c)))
				continue;
			emit_flop(pc, 0, dst[c], src[0][0]);
		}
		break;
	case TGSI_OPCODE_RSQ:
		for (c = 3; c >= 0; c--) {
			if (!(mask & (1 << c)))
				continue;
			emit_flop(pc, 2, dst[c], src[0][0]);
		}
		break;
	case TGSI_OPCODE_SCS:
		temp = temp_temp(pc);
		emit_precossin(pc, temp, src[0][0]);
		if (mask & (1 << 0))
			emit_flop(pc, 5, dst[0], temp);
		if (mask & (1 << 1))
			emit_flop(pc, 4, dst[1], temp);
		if (mask & (1 << 2))
			emit_mov_immdval(pc, dst[2], 0.0);
		if (mask & (1 << 3))
			emit_mov_immdval(pc, dst[3], 1.0);
		break;
	case TGSI_OPCODE_SGE:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_set(pc, 6, dst[c], src[0][c], src[1][c]);
		}
		break;
	case TGSI_OPCODE_SIN:
		temp = temp_temp(pc);
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
		emit_tex(pc, dst, mask, src[0], unit,
			 inst->InstructionExtTexture.Texture, FALSE);
		break;
	case TGSI_OPCODE_TXP:
		emit_tex(pc, dst, mask, src[0], unit,
			 inst->InstructionExtTexture.Texture, TRUE);
		break;
	case TGSI_OPCODE_XPD:
		temp = temp_temp(pc);
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
		if (mask & (1 << 3))
			emit_mov_immdval(pc, dst[3], 1.0);
		break;
	case TGSI_OPCODE_END:
		break;
	default:
		NOUVEAU_ERR("invalid opcode %d\n", inst->Instruction.Opcode);
		return FALSE;
	}

	if (sat) {
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_cvt(pc, rdst[c], dst[c], -1, CVTOP_SAT,
				 CVT_F32_F32);
		}
	} else if (assimilate) {
		for (c = 0; c < 4; c++)
			if (rdst[c])
				assimilate_temp(pc, rdst[c], dst[c]);
	}

	for (i = 0; i < inst->Instruction.NumSrcRegs; i++) {
		for (c = 0; c < 4; c++) {
			if (!src[i][c])
				continue;
			if (src[i][c]->index == -1 && src[i][c]->type == P_IMMD)
				FREE(src[i][c]);
			else
			if (src[i][c]->acc == pc->insn_cur)
				release_hw(pc, src[i][c]);
		}
	}

	kill_temp_temp(pc);
	return TRUE;
}

/* Adjust a bitmask that indicates what components of a source are used,
 * we use this in tx_prep so we only load interpolants that are needed.
 */
static void
insn_adjust_mask(const struct tgsi_full_instruction *insn, unsigned *mask)
{
	const struct tgsi_instruction_ext_texture *tex;

	switch (insn->Instruction.Opcode) {
	case TGSI_OPCODE_DP3:
		*mask = 0x7;
		break;
	case TGSI_OPCODE_DP4:
	case TGSI_OPCODE_DPH:
		*mask = 0xF;
		break;
	case TGSI_OPCODE_LIT:
		*mask = 0xB;
		break;
	case TGSI_OPCODE_RCP:
	case TGSI_OPCODE_RSQ:
		*mask = 0x1;
		break;
	case TGSI_OPCODE_TEX:
	case TGSI_OPCODE_TXP:
		assert(insn->Instruction.Extended);
		tex = &insn->InstructionExtTexture;

		*mask = 0x7;
		if (tex->Texture == TGSI_TEXTURE_1D)
			*mask = 0x1;
		else
		if (tex->Texture == TGSI_TEXTURE_2D)
			*mask = 0x3;

		if (insn->Instruction.Opcode == TGSI_OPCODE_TXP)
			*mask |= 0x8;
		break;
	default:
		break;
	}
}

static void
prep_inspect_insn(struct nv50_pc *pc, const union tgsi_full_token *tok,
		  unsigned *r_usage[2])
{
	const struct tgsi_full_instruction *insn;
	const struct tgsi_full_src_register *src;
	const struct tgsi_dst_register *dst;

	unsigned i, c, k, n, mask, *acc_p;

	insn = &tok->FullInstruction;
	dst = &insn->FullDstRegisters[0].DstRegister;
	mask = dst->WriteMask;

	if (!r_usage[0])
		r_usage[0] = CALLOC(pc->temp_nr * 4, sizeof(unsigned));
	if (!r_usage[1])
		r_usage[1] = CALLOC(pc->attr_nr * 4, sizeof(unsigned));

	if (dst->File == TGSI_FILE_TEMPORARY) {
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			r_usage[0][dst->Index * 4 + c] = pc->insn_nr;
		}
	}

	for (i = 0; i < insn->Instruction.NumSrcRegs; i++) {
		src = &insn->FullSrcRegisters[i];

		switch (src->SrcRegister.File) {
		case TGSI_FILE_TEMPORARY:
			acc_p = r_usage[0];
			break;
		case TGSI_FILE_INPUT:
			acc_p = r_usage[1];
			break;
		default:
			continue;
		}

		insn_adjust_mask(insn, &mask);

		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;

			k = tgsi_util_get_full_src_register_extswizzle(src, c);
			switch (k) {
			case TGSI_EXTSWIZZLE_X:
			case TGSI_EXTSWIZZLE_Y:
			case TGSI_EXTSWIZZLE_Z:
			case TGSI_EXTSWIZZLE_W:
				n = src->SrcRegister.Index * 4 + k;
				acc_p[n] = pc->insn_nr;
				break;
			default:
				break;
			}
		}
	}
}

static unsigned
load_fp_attrib(struct nv50_pc *pc, int i, unsigned *acc, int *mid,
	       int *aid, int *p_oid)
{
	struct nv50_reg *iv;
	int oid, c, n;
	unsigned mask = 0;

	iv = (pc->interp_mode[i] & INTERP_CENTROID) ? pc->iv_c : pc->iv_p;

	for (c = 0, n = i * 4; c < 4; c++, n++) {
		oid = (*p_oid)++;
		pc->attr[n].type = P_TEMP;
		pc->attr[n].index = i;

		if (pc->attr[n].acc == acc[n])
			continue;
		mask |= (1 << c);

		pc->attr[n].acc = acc[n];
		pc->attr[n].rhw = pc->attr[n].hw = -1;
		alloc_reg(pc, &pc->attr[n]);

		pc->attr[n].rhw = (*aid)++;
		emit_interp(pc, &pc->attr[n], iv, pc->interp_mode[i]);

		pc->p->cfg.fp.map[(*mid) / 4] |= oid << (8 * ((*mid) % 4));
		(*mid)++;
		pc->p->cfg.fp.regs[1] += 0x00010001;
	}

	return mask;
}

static boolean
nv50_program_tx_prep(struct nv50_pc *pc)
{
	struct tgsi_parse_context p;
	boolean ret = FALSE;
	unsigned i, c;
	unsigned fcol, bcol, fcrd, depr;

	/* count (centroid) perspective interpolations */
	unsigned centroid_loads = 0;
	unsigned perspect_loads = 0;

	/* track register access for temps and attrs */
	unsigned *r_usage[2];
	r_usage[0] = NULL;
	r_usage[1] = NULL;

	depr = fcol = bcol = fcrd = 0xffff;

	if (pc->p->type == PIPE_SHADER_FRAGMENT) {
		pc->p->cfg.fp.regs[0] = 0x01000404;
		pc->p->cfg.fp.regs[1] = 0x00000400;
	}

	tgsi_parse_init(&p, pc->p->pipe.tokens);
	while (!tgsi_parse_end_of_tokens(&p)) {
		const union tgsi_full_token *tok = &p.FullToken;

		tgsi_parse_token(&p);
		switch (tok->Token.Type) {
		case TGSI_TOKEN_TYPE_IMMEDIATE:
		{
			const struct tgsi_full_immediate *imm =
				&p.FullToken.FullImmediate;

			ctor_immd(pc, imm->u[0].Float,
				      imm->u[1].Float,
				      imm->u[2].Float,
				      imm->u[3].Float);
		}
			break;
		case TGSI_TOKEN_TYPE_DECLARATION:
		{
			const struct tgsi_full_declaration *d;
			unsigned last, first, mode;

			d = &p.FullToken.FullDeclaration;
			first = d->DeclarationRange.First;
			last = d->DeclarationRange.Last;

			switch (d->Declaration.File) {
			case TGSI_FILE_TEMPORARY:
				if (pc->temp_nr < (last + 1))
					pc->temp_nr = last + 1;
				break;
			case TGSI_FILE_OUTPUT:
				if (pc->result_nr < (last + 1))
					pc->result_nr = last + 1;

				if (!d->Declaration.Semantic)
					break;

				switch (d->Semantic.SemanticName) {
				case TGSI_SEMANTIC_POSITION:
					depr = first;
					pc->p->cfg.fp.regs[2] |= 0x00000100;
					pc->p->cfg.fp.regs[3] |= 0x00000011;
					break;
				default:
					break;
				}

				break;
			case TGSI_FILE_INPUT:
			{
				if (pc->attr_nr < (last + 1))
					pc->attr_nr = last + 1;

				if (pc->p->type != PIPE_SHADER_FRAGMENT)
					break;

				switch (d->Declaration.Interpolate) {
				case TGSI_INTERPOLATE_CONSTANT:
					mode = INTERP_FLAT;
					break;
				case TGSI_INTERPOLATE_PERSPECTIVE:
					mode = INTERP_PERSPECTIVE;
					break;
				default:
					mode = INTERP_LINEAR;
					break;
				}

				if (d->Declaration.Semantic) {
					switch (d->Semantic.SemanticName) {
					case TGSI_SEMANTIC_POSITION:
						fcrd = first;
						break;
					case TGSI_SEMANTIC_COLOR:
						fcol = first;
						mode = INTERP_PERSPECTIVE;
						break;
					case TGSI_SEMANTIC_BCOLOR:
						bcol = first;
						mode = INTERP_PERSPECTIVE;
						break;
					}
				}

				if (d->Declaration.Centroid) {
					mode |= INTERP_CENTROID;
					if (mode & INTERP_PERSPECTIVE)
						centroid_loads++;
				} else
				if (mode & INTERP_PERSPECTIVE)
					perspect_loads++;

				assert(last < 32);
				for (i = first; i <= last; i++)
					pc->interp_mode[i] = mode;
			}
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
			pc->insn_nr++;
			prep_inspect_insn(pc, tok, r_usage);
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
				pc->temp[i*4+c].rhw = -1;
				pc->temp[i*4+c].index = i;
				pc->temp[i*4+c].acc = r_usage[0][i*4+c];
			}
		}
	}

	if (pc->attr_nr) {
		int oid = 4, mid = 4, aid = 0;
		/* oid = VP output id
		 * aid = FP attribute/interpolant id
		 * mid = VP output mapping field ID
		 */

		pc->attr = CALLOC(pc->attr_nr * 4, sizeof(struct nv50_reg));
		if (!pc->attr)
			goto out_err;

		if (pc->p->type == PIPE_SHADER_FRAGMENT) {
			/* position should be loaded first */
			if (fcrd != 0xffff) {
				unsigned mask;
				mid = 0;
				mask = load_fp_attrib(pc, fcrd, r_usage[1],
						      &mid, &aid, &oid);
				oid = 0;
				pc->p->cfg.fp.regs[1] |= (mask << 24);
				pc->p->cfg.fp.map[0] = 0x04040404 * fcrd;
			}
			pc->p->cfg.fp.map[0] += 0x03020100;

			/* should do MAD fcrd.xy, fcrd, SOME_CONST, fcrd */

			if (perspect_loads) {
				pc->iv_p = alloc_temp(pc, NULL);

				if (!(pc->p->cfg.fp.regs[1] & 0x08000000)) {
					pc->p->cfg.fp.regs[1] |= 0x08000000;
					pc->iv_p->rhw = aid++;
					emit_interp(pc, pc->iv_p, NULL,
						    INTERP_LINEAR);
					emit_flop(pc, 0, pc->iv_p, pc->iv_p);
				} else {
					pc->iv_p->rhw = aid - 1;
					emit_flop(pc, 0, pc->iv_p,
						  &pc->attr[fcrd * 4 + 3]);
				}
			}

			if (centroid_loads) {
				pc->iv_c = alloc_temp(pc, NULL);
				pc->iv_c->rhw = pc->iv_p ? aid - 1 : aid++;
				emit_interp(pc, pc->iv_c, NULL,
					    INTERP_CENTROID);
				emit_flop(pc, 0, pc->iv_c, pc->iv_c);
				pc->p->cfg.fp.regs[1] |= 0x08000000;
			}

			for (c = 0; c < 4; c++) {
				/* I don't know what these values do, but
				 * let's set them like the blob does:
				 */
				if (fcol != 0xffff && r_usage[1][fcol * 4 + c])
					pc->p->cfg.fp.regs[0] += 0x00010000;
				if (bcol != 0xffff && r_usage[1][bcol * 4 + c])
					pc->p->cfg.fp.regs[0] += 0x00010000;
			}

			for (i = 0; i < pc->attr_nr; i++)
				load_fp_attrib(pc, i, r_usage[1],
					       &mid, &aid, &oid);

			if (pc->iv_p)
				free_temp(pc, pc->iv_p);
			if (pc->iv_c)
				free_temp(pc, pc->iv_c);

			pc->p->cfg.fp.high_map = (mid / 4);
			pc->p->cfg.fp.high_map += ((mid % 4) ? 1 : 0);
		} else {
			/* vertex program */
			for (i = 0; i < pc->attr_nr * 4; i++) {
				pc->p->cfg.vp.attr[aid / 32] |=
					(1 << (aid % 32));
				pc->attr[i].type = P_ATTR;
				pc->attr[i].hw = aid++;
				pc->attr[i].index = i / 4;
			}
		}
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
					pc->result[i*4+c].rhw = (i == depr) ?
						-1 : rid++;
				} else {
					pc->result[i*4+c].type = P_RESULT;
					pc->result[i*4+c].hw = rid++;
				}
				pc->result[i*4+c].index = i;
			}

			if (pc->p->type == PIPE_SHADER_FRAGMENT &&
			    depr != 0xffff) {
				pc->result[depr * 4 + 2].rhw =
					(pc->result_nr - 1) * 4;
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
		int rid = 0;

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
	if (r_usage[0])
		FREE(r_usage[0]);
	if (r_usage[1])
		FREE(r_usage[1]);

	tgsi_parse_free(&p);
	return ret;
}

static void
free_nv50_pc(struct nv50_pc *pc)
{
	if (pc->immd)
		FREE(pc->immd);
	if (pc->param)
		FREE(pc->param);
	if (pc->result)
		FREE(pc->result);
	if (pc->attr)
		FREE(pc->attr);
	if (pc->temp)
		FREE(pc->temp);

	FREE(pc);
}

static boolean
nv50_program_tx(struct nv50_program *p)
{
	struct tgsi_parse_context parse;
	struct nv50_pc *pc;
	unsigned k;
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

		/* don't allow half insn/immd on first and last instruction */
		pc->allow32 = TRUE;
		if (pc->insn_cur == 0 || pc->insn_cur + 2 == pc->insn_nr)
			pc->allow32 = FALSE;

		tgsi_parse_token(&parse);

		switch (tok->Token.Type) {
		case TGSI_TOKEN_TYPE_INSTRUCTION:
			++pc->insn_cur;
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
		for (k = 0; k < pc->result_nr * 4; k++) {
			if (pc->result[k].rhw == -1)
				continue;
			if (pc->result[k].hw != pc->result[k].rhw) {
				out.hw = pc->result[k].rhw;
				emit_mov(pc, &out, &pc->result[k]);
			}
			if (pc->p->cfg.high_result < (pc->result[k].rhw + 1))
				pc->p->cfg.high_result = pc->result[k].rhw + 1;
		}
	}

	/* look for single half instructions and make them long */
	struct nv50_program_exec *e, *e_prev;

	for (k = 0, e = pc->p->exec_head, e_prev = NULL; e; e = e->next) {
		if (!is_long(e))
			k++;

		if (!e->next || is_long(e->next)) {
			if (k & 1)
				convert_to_long(pc, e);
			k = 0;
		}

		if (e->next)
			e_prev = e;
	}

	if (!is_long(pc->p->exec_tail)) {
		/* this may occur if moving FP results */
		assert(e_prev && !is_long(e_prev));
		convert_to_long(pc, e_prev);
		convert_to_long(pc, pc->p->exec_tail);
	}

	assert(is_long(pc->p->exec_tail) && !is_immd(pc->p->exec_head));
	pc->p->exec_tail->inst[1] |= 0x00000001;

	p->param_nr = pc->param_nr * 4;
	p->immd_nr = pc->immd_nr * 4;
	p->immd = pc->immd_buf;

out_err:
	tgsi_parse_free(&parse);

out_cleanup:
	free_nv50_pc(pc);
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
			unsigned start, unsigned count, unsigned cbuf)
{
	struct nouveau_channel *chan = nv50->screen->base.channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;

	while (count) {
		unsigned nr = count > 2047 ? 2047 : count;

		BEGIN_RING(chan, tesla, NV50TCL_CB_ADDR, 1);
		OUT_RING  (chan, (cbuf << 0) | (start << 8));
		BEGIN_RING(chan, tesla, NV50TCL_CB_DATA(0) | 0x40000000, nr);
		OUT_RINGp (chan, map, nr);

		map += nr;
		start += nr;
		count -= nr;
	}
}

static void
nv50_program_validate_data(struct nv50_context *nv50, struct nv50_program *p)
{
	struct pipe_screen *pscreen = nv50->pipe.screen;

	if (!p->data[0] && p->immd_nr) {
		struct nouveau_resource *heap = nv50->screen->immd_heap[0];

		if (nouveau_resource_alloc(heap, p->immd_nr, p, &p->data[0])) {
			while (heap->next && heap->size < p->immd_nr) {
				struct nv50_program *evict = heap->next->priv;
				nouveau_resource_free(&evict->data[0]);
			}

			if (nouveau_resource_alloc(heap, p->immd_nr, p,
						   &p->data[0]))
				assert(0);
		}

		/* immediates only need to be uploaded again when freed */
		nv50_program_upload_data(nv50, p->immd, p->data[0]->start,
					 p->immd_nr, NV50_CB_PMISC);
	}

	if (!p->data[1] && p->param_nr) {
		struct nouveau_resource *heap =
			nv50->screen->parm_heap[p->type];

		if (nouveau_resource_alloc(heap, p->param_nr, p, &p->data[1])) {
			while (heap->next && heap->size < p->param_nr) {
				struct nv50_program *evict = heap->next->priv;
				nouveau_resource_free(&evict->data[1]);
			}

			if (nouveau_resource_alloc(heap, p->param_nr, p,
						   &p->data[1]))
				assert(0);
		}
	}

	if (p->param_nr) {
		unsigned cbuf = NV50_CB_PVP;
		float *map = pipe_buffer_map(pscreen, nv50->constbuf[p->type],
					     PIPE_BUFFER_USAGE_CPU_READ);
		if (p->type == PIPE_SHADER_FRAGMENT)
			cbuf = NV50_CB_PFP;
		nv50_program_upload_data(nv50, map, p->data[1]->start,
					 p->param_nr, cbuf);
		pipe_buffer_unmap(pscreen, nv50->constbuf[p->type]);
	}
}

static void
nv50_program_validate_code(struct nv50_context *nv50, struct nv50_program *p)
{
	struct nouveau_channel *chan = nv50->screen->base.channel;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nv50_program_exec *e;
	struct nouveau_stateobj *so;
	const unsigned flags = NOUVEAU_BO_VRAM | NOUVEAU_BO_WR;
	unsigned start, count, *up, *ptr;
	boolean upload = FALSE;

	if (!p->bo) {
		nouveau_bo_new(chan->device, NOUVEAU_BO_VRAM, 0x100,
			       p->exec_size * 4, &p->bo);
		upload = TRUE;
	}

	if ((p->data[0] && p->data[0]->start != p->data_start[0]) ||
		(p->data[1] && p->data[1]->start != p->data_start[1])) {
		for (e = p->exec_head; e; e = e->next) {
			unsigned ei, ci, bs;

			if (e->param.index < 0)
				continue;
			bs = (e->inst[1] >> 22) & 0x07;
			assert(bs < 2);
			ei = e->param.shift >> 5;
			ci = e->param.index + p->data[bs]->start;

			e->inst[ei] &= ~e->param.mask;
			e->inst[ei] |= (ci << e->param.shift);
		}

		if (p->data[0])
			p->data_start[0] = p->data[0]->start;
		if (p->data[1])
			p->data_start[1] = p->data[1]->start;

		upload = TRUE;
	}

	if (!upload)
		return;

#ifdef NV50_PROGRAM_DUMP
	NOUVEAU_ERR("-------\n");
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
	so_method(so, nv50->screen->tesla, NV50TCL_CB_DEF_ADDRESS_HIGH, 3);
	so_reloc (so, p->bo, 0, flags | NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, p->bo, 0, flags | NOUVEAU_BO_LOW, 0, 0);
	so_data  (so, (NV50_CB_PUPLOAD << 16) | 0x0800); //(p->exec_size * 4));

	start = 0; count = p->exec_size;
	while (count) {
		struct nouveau_channel *chan = nv50->screen->base.channel;
		unsigned nr;

		so_emit(chan, so);

		nr = MIN2(count, 2047);
		nr = MIN2(chan->pushbuf->remaining, nr);
		if (chan->pushbuf->remaining < (nr + 3)) {
			FIRE_RING(chan);
			continue;
		}

		BEGIN_RING(chan, tesla, NV50TCL_CB_ADDR, 1);
		OUT_RING  (chan, (start << 8) | NV50_CB_PUPLOAD);
		BEGIN_RING(chan, tesla, NV50TCL_CB_DATA(0) | 0x40000000, nr);
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
	so_reloc (so, p->bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
		      NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, p->bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
		      NOUVEAU_BO_LOW, 0, 0);
	so_method(so, tesla, NV50TCL_VP_ATTR_EN_0, 2);
	so_data  (so, p->cfg.vp.attr[0]);
	so_data  (so, p->cfg.vp.attr[1]);
	so_method(so, tesla, NV50TCL_VP_REG_ALLOC_RESULT, 1);
	so_data  (so, p->cfg.high_result);
	so_method(so, tesla, NV50TCL_VP_RESULT_MAP_SIZE, 2);
	so_data  (so, p->cfg.high_result); //8);
	so_data  (so, p->cfg.high_temp);
	so_method(so, tesla, NV50TCL_VP_START_ID, 1);
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
	unsigned i;

	if (!p->translated) {
		nv50_program_validate(nv50, p);
		if (!p->translated)
			assert(0);
	}

	nv50_program_validate_data(nv50, p);
	nv50_program_validate_code(nv50, p);

	so = so_new(64, 2);
	so_method(so, tesla, NV50TCL_FP_ADDRESS_HIGH, 2);
	so_reloc (so, p->bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
		      NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, p->bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
		      NOUVEAU_BO_LOW, 0, 0);
	so_method(so, tesla, NV50TCL_MAP_SEMANTIC_0, 4);
	so_data  (so, p->cfg.fp.regs[0]); /* 0x01000404 / 0x00040404 */
	so_data  (so, 0x00000004);
	so_data  (so, 0x00000000);
	so_data  (so, 0x00000000);
	so_method(so, tesla, NV50TCL_VP_RESULT_MAP(0), p->cfg.fp.high_map);
	for (i = 0; i < p->cfg.fp.high_map; i++)
		so_data(so, p->cfg.fp.map[i]);
	so_method(so, tesla, NV50TCL_FP_INTERPOLANT_CTRL, 2);
	so_data  (so, p->cfg.fp.regs[1]); /* 0x08040404 / 0x0f000401 */
	so_data  (so, p->cfg.high_temp);
	so_method(so, tesla, NV50TCL_FP_RESULT_COUNT, 1);
	so_data  (so, p->cfg.high_result);
	so_method(so, tesla, NV50TCL_FP_CTRL_UNK19A8, 1);
	so_data  (so, p->cfg.fp.regs[2]);
	so_method(so, tesla, NV50TCL_FP_CTRL_UNK196C, 1);
	so_data  (so, p->cfg.fp.regs[3]);
	so_method(so, tesla, NV50TCL_FP_START_ID, 1);
	so_data  (so, 0); /* program start offset */
	so_ref(so, &nv50->state.fragprog);
	so_ref(NULL, &so);
}

void
nv50_program_destroy(struct nv50_context *nv50, struct nv50_program *p)
{
	while (p->exec_head) {
		struct nv50_program_exec *e = p->exec_head;

		p->exec_head = e->next;
		FREE(e);
	}
	p->exec_tail = NULL;
	p->exec_size = 0;

	nouveau_bo_ref(NULL, &p->bo);

	nouveau_resource_free(&p->data[0]);
	nouveau_resource_free(&p->data[1]);

	p->translated = 0;
}
