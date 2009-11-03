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

#define NV50_SU_MAX_TEMP 127
#define NV50_SU_MAX_ADDR 4
//#define NV50_PROGRAM_DUMP

/* $a5 and $a6 always seem to be 0, and using $a7 gives you noise */

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
		P_IMMD,
		P_ADDR
	} type;
	int index;

	int hw;
	int mod;

	int rhw; /* result hw for FP outputs, or interpolant index */
	int acc; /* instruction where this reg is last read (first insn == 1) */
};

#define NV50_MOD_NEG 1
#define NV50_MOD_ABS 2
#define NV50_MOD_SAT 4

/* arbitrary limits */
#define MAX_IF_DEPTH 4
#define MAX_LOOP_DEPTH 4

struct nv50_pc {
	struct nv50_program *p;

	/* hw resources */
	struct nv50_reg *r_temp[NV50_SU_MAX_TEMP];
	struct nv50_reg r_addr[NV50_SU_MAX_ADDR];

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
	struct nv50_reg **addr;
	int addr_nr;

	struct nv50_reg *temp_temp[16];
	unsigned temp_temp_nr;

	/* broadcast and destination replacement regs */
	struct nv50_reg *r_brdc;
	struct nv50_reg *r_dst[4];

	unsigned interp_mode[32];
	/* perspective interpolation registers */
	struct nv50_reg *iv_p;
	struct nv50_reg *iv_c;

	struct nv50_program_exec *if_cond;
	struct nv50_program_exec *if_insn[MAX_IF_DEPTH];
	struct nv50_program_exec *br_join[MAX_IF_DEPTH];
	struct nv50_program_exec *br_loop[MAX_LOOP_DEPTH]; /* for BRK branch */
	int if_lvl, loop_lvl;
	unsigned loop_pos[MAX_LOOP_DEPTH];

	/* current instruction and total number of insns */
	unsigned insn_cur;
	unsigned insn_nr;

	boolean allow32;
};

static INLINE void
ctor_reg(struct nv50_reg *reg, unsigned type, int index, int hw)
{
	reg->type = type;
	reg->index = index;
	reg->hw = hw;
	reg->mod = 0;
	reg->rhw = -1;
	reg->acc = 0;
}

static INLINE unsigned
popcnt4(uint32_t val)
{
	static const unsigned cnt[16]
	= { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };
	return cnt[val & 0xf];
}

static void
terminate_mbb(struct nv50_pc *pc)
{
	int i;

	/* remove records of temporary address register values */
	for (i = 0; i < NV50_SU_MAX_ADDR; ++i)
		if (pc->r_addr[i].index < 0)
			pc->r_addr[i].rhw = -1;
}

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

/* XXX: For shaders that aren't executed linearly (e.g. shaders that
 * contain loops), we need to assign all hw regs to TGSI TEMPs early,
 * lest we risk temp_temps overwriting regs alloc'd "later".
 */
static struct nv50_reg *
alloc_temp(struct nv50_pc *pc, struct nv50_reg *dst)
{
	struct nv50_reg *r;
	int i;

	if (dst && dst->type == P_TEMP && dst->hw == -1)
		return dst;

	for (i = 0; i < NV50_SU_MAX_TEMP; i++) {
		if (!pc->r_temp[i]) {
			r = MALLOC_STRUCT(nv50_reg);
			ctor_reg(r, P_TEMP, -1, i);
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
		dst[i] = MALLOC_STRUCT(nv50_reg);
		ctor_reg(dst[i], P_TEMP, -1, idx + i);
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
	struct nv50_reg *r = MALLOC_STRUCT(nv50_reg);
	unsigned hw;

	for (hw = 0; hw < pc->immd_nr * 4; hw++)
		if (pc->immd_buf[hw] == f)
			break;

	if (hw == pc->immd_nr * 4)
		hw = ctor_immd(pc, f, -f, 0.5 * f, 0) * 4;

	ctor_reg(r, P_IMMD, -1, hw);
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
	if (dst->hw > 63)
		set_long(pc, e);
	e->inst[0] |= (dst->hw << 2);
}

static INLINE void
set_immd(struct nv50_pc *pc, struct nv50_reg *imm, struct nv50_program_exec *e)
{
	unsigned val;
	float f = pc->immd_buf[imm->hw];

	if (imm->mod & NV50_MOD_ABS)
		f = fabsf(f);
	val = fui((imm->mod & NV50_MOD_NEG) ? -f : f);

	set_long(pc, e);
	/*XXX: can't be predicated - bits overlap.. catch cases where both
	 *     are required and avoid them. */
	set_pred(pc, 0, 0, e);
	set_pred_wr(pc, 0, 0, e);

	e->inst[1] |= 0x00000002 | 0x00000001;
	e->inst[0] |= (val & 0x3f) << 16;
	e->inst[1] |= (val >> 6) << 2;
}

static INLINE void
set_addr(struct nv50_program_exec *e, struct nv50_reg *a)
{
	assert(!(e->inst[0] & 0x0c000000));
	assert(!(e->inst[1] & 0x00000004));

	e->inst[0] |= (a->hw & 3) << 26;
	e->inst[1] |= (a->hw >> 2) << 2;
}

static void
emit_add_addr_imm(struct nv50_pc *pc, struct nv50_reg *dst,
		  struct nv50_reg *src0, uint16_t src1_val)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] = 0xd0000000 | (src1_val << 9);
	e->inst[1] = 0x20000000;
	set_long(pc, e);
	e->inst[0] |= dst->hw << 2;
	if (src0) /* otherwise will add to $a0, which is always 0 */
		set_addr(e, src0);

	emit(pc, e);
}

static struct nv50_reg *
alloc_addr(struct nv50_pc *pc, struct nv50_reg *ref)
{
	int i;
	struct nv50_reg *a_tgsi = NULL, *a = NULL;

	if (!ref) {
		/* allocate for TGSI address reg */
		for (i = 0; i < NV50_SU_MAX_ADDR; ++i) {
			if (pc->r_addr[i].index >= 0)
				continue;
			if (pc->r_addr[i].rhw >= 0 &&
			    pc->r_addr[i].acc == pc->insn_cur)
				continue;

			pc->r_addr[i].rhw = -1;
			pc->r_addr[i].index = i;
			return &pc->r_addr[i];
		}
		assert(0);
		return NULL;
	}

	/* Allocate and set an address reg so we can access 'ref'.
	 *
	 * If and r_addr has index < 0, it is not reserved for TGSI,
	 * and index will be the negative of the TGSI addr index the
	 * value in rhw is relative to, or -256 if rhw is an offset
	 * from 0. If rhw < 0, the reg has not been initialized.
	 */
	for (i = NV50_SU_MAX_ADDR - 1; i >= 0; --i) {
		if (pc->r_addr[i].index >= 0) /* occupied for TGSI */
			continue;
		if (pc->r_addr[i].rhw < 0) { /* unused */
			a = &pc->r_addr[i];
			continue;
		}
		if (!a && pc->r_addr[i].acc != pc->insn_cur)
			a = &pc->r_addr[i];

		if (ref->hw - pc->r_addr[i].rhw >= 128)
			continue;

		if ((ref->acc >= 0 && pc->r_addr[i].index == -256) ||
		    (ref->acc < 0 && -pc->r_addr[i].index == ref->index)) {
			pc->r_addr[i].acc = pc->insn_cur;
			return &pc->r_addr[i];
		}
	}
	assert(a);

	if (ref->acc < 0)
		a_tgsi = pc->addr[ref->index];

	emit_add_addr_imm(pc, a, a_tgsi, (ref->hw & ~0x7f) * 4);

	a->rhw = ref->hw & ~0x7f;
	a->acc = pc->insn_cur;
	a->index = a_tgsi ? -ref->index : -256;
	return a;
}

#define INTERP_LINEAR		0
#define INTERP_FLAT		1
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

	e->param.index = src->hw & 127;
	e->param.shift = s;
	e->param.mask = m << (s % 32);

	if (src->hw > 127)
		set_addr(e, alloc_addr(pc, src));
	else
	if (src->acc < 0) {
		assert(src->type == P_CONST);
		set_addr(e, pc->addr[src->index]);
	}

	e->inst[1] |= (((src->type == P_IMMD) ? 0 : 1) << 22);
}

static void
emit_mov(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] = 0x10000000;
	if (!pc->allow32)
		set_long(pc, e);

	set_dst(pc, dst, e);

	if (!is_long(e) && src->type == P_IMMD) {
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
		if (src->hw > 63)
			set_long(pc, e);
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
set_src_0_restricted(struct nv50_pc *pc, struct nv50_reg *src,
		     struct nv50_program_exec *e)
{
	struct nv50_reg *temp;

	if (src->type != P_TEMP) {
		temp = temp_temp(pc);
		emit_mov(pc, temp, src);
		src = temp;
	}

	alloc_reg(pc, src);
	if (src->hw > 63)
		set_long(pc, e);
	e->inst[0] |= (src->hw << 9);
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
	if (src->hw > 63)
		set_long(pc, e);
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
	if (src->hw > 63)
		set_long(pc, e);
	e->inst[0] |= ((src->hw & 127) << 16);
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
	e->inst[1] |= ((src->hw & 127) << 14);
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
		if (src0->mod & NV50_MOD_NEG)
			e->inst[0] |= 0x00008000;
		set_immd(pc, src1, e);
	} else {
		set_src_1(pc, src1, e);
		if ((src0->mod ^ src1->mod) & NV50_MOD_NEG) {
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

	e->inst[0] = 0xb0000000;

	alloc_reg(pc, src1);
	check_swap_src_0_1(pc, &src0, &src1);

	if (!pc->allow32 || (src0->mod | src1->mod) || src1->hw > 63) {
		set_long(pc, e);
		e->inst[1] |= ((src0->mod & NV50_MOD_NEG) << 26) |
			      ((src1->mod & NV50_MOD_NEG) << 27);
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
emit_arl(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src,
	 uint8_t s)
{
	struct nv50_program_exec *e = exec(pc);

	set_long(pc, e);
	e->inst[1] |= 0xc0000000;

	e->inst[0] |= dst->hw << 2;
	e->inst[0] |= s << 16; /* shift left */
	set_src_0_restricted(pc, src, e);

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

	if (src0->mod & NV50_MOD_ABS)
		e->inst[1] |= 0x00100000;
	if (src1->mod & NV50_MOD_ABS)
		e->inst[1] |= 0x00080000;

	emit(pc, e);
}

static INLINE void
emit_sub(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src0,
	 struct nv50_reg *src1)
{
	assert(src0 != src1);
	src1->mod ^= NV50_MOD_NEG;
	emit_add(pc, dst, src0, src1);
	src1->mod ^= NV50_MOD_NEG;
}

static void
emit_bitop2(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src0,
	    struct nv50_reg *src1, unsigned op)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] = 0xd0000000;
	set_long(pc, e);

	check_swap_src_0_1(pc, &src0, &src1);
	set_dst(pc, dst, e);
	set_src_0(pc, src0, e);

	if (op != TGSI_OPCODE_AND && op != TGSI_OPCODE_OR &&
	    op != TGSI_OPCODE_XOR)
		assert(!"invalid bit op");

	if (src1->type == P_IMMD && src0->type == P_TEMP && pc->allow32) {
		set_immd(pc, src1, e);
		if (op == TGSI_OPCODE_OR)
			e->inst[0] |= 0x0100;
		else
		if (op == TGSI_OPCODE_XOR)
			e->inst[0] |= 0x8000;
	} else {
		set_src_1(pc, src1, e);
		e->inst[1] |= 0x04000000; /* 32 bit */
		if (op == TGSI_OPCODE_OR)
			e->inst[1] |= 0x4000;
		else
		if (op == TGSI_OPCODE_XOR)
			e->inst[1] |= 0x8000;
	}

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

	if ((src0->mod ^ src1->mod) & NV50_MOD_NEG)
		e->inst[1] |= 0x04000000;
	if (src2->mod & NV50_MOD_NEG)
		e->inst[1] |= 0x08000000;

	emit(pc, e);
}

static INLINE void
emit_msb(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src0,
	 struct nv50_reg *src1, struct nv50_reg *src2)
{
	assert(src2 != src0 && src2 != src1);
	src2->mod ^= NV50_MOD_NEG;
	emit_mad(pc, dst, src0, src1, src2);
	src2->mod ^= NV50_MOD_NEG;
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

	if (sub == 0 || sub == 2)
		set_src_0_restricted(pc, src, e);
	else
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

/* 0x04 == 32 bit dst */
/* 0x40 == dst is float */
/* 0x80 == src is float */
#define CVT_F32_F32 0xc4
#define CVT_F32_S32 0x44
#define CVT_S32_F32 0x8c
#define CVT_S32_S32 0x0c
#define CVT_NEG     0x20
#define CVT_RI      0x08

static void
emit_cvt(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src,
	 int wp, unsigned cvn, unsigned fmt)
{
	struct nv50_program_exec *e;

	e = exec(pc);
	set_long(pc, e);

	e->inst[0] |= 0xa0000000;
	e->inst[1] |= 0x00004000; /* 32 bit src */
	e->inst[1] |= (cvn << 16);
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

/* nv50 Condition codes:
 *  0x1 = LT
 *  0x2 = EQ
 *  0x3 = LE
 *  0x4 = GT
 *  0x5 = NE
 *  0x6 = GE
 *  0x7 = set condition code ? (used before bra.lt/le/gt/ge)
 *  0x8 = unordered bit (allows NaN)
 */
static void
emit_set(struct nv50_pc *pc, unsigned ccode, struct nv50_reg *dst, int wp,
	 struct nv50_reg *src0, struct nv50_reg *src1)
{
	static const unsigned cc_swapped[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };

	struct nv50_program_exec *e = exec(pc);
	struct nv50_reg *rdst;

	assert(ccode < 16);
	if (check_swap_src_0_1(pc, &src0, &src1))
		ccode = cc_swapped[ccode & 7] | (ccode & 8);

	rdst = dst;
	if (dst && dst->type != P_TEMP)
		dst = alloc_temp(pc, NULL);

	/* set.u32 */
	set_long(pc, e);
	e->inst[0] |= 0xb0000000;
	e->inst[1] |= 0x60000000 | (ccode << 14);

	/* XXX: decuda will disasm as .u16 and use .lo/.hi regs, but
	 * that doesn't seem to match what the hw actually does
	e->inst[1] |= 0x04000000; << breaks things, u32 by default ?
	 */

	if (wp >= 0)
		set_pred_wr(pc, 1, wp, e);
	if (dst)
		set_dst(pc, dst, e);
	else {
		e->inst[0] |= 0x000001fc;
		e->inst[1] |= 0x00000008;
	}

	set_src_0(pc, src0, e);
	set_src_1(pc, src1, e);

	emit(pc, e);
	pc->if_cond = pc->p->exec_tail; /* record for OPCODE_IF */

	/* cvt.f32.u32/s32 (?) if we didn't only write the predicate */
	if (rdst)
		emit_cvt(pc, rdst, dst, -1, CVTOP_ABS | CVTOP_RN, CVT_F32_S32);
	if (rdst && rdst != dst)
		free_temp(pc, dst);
}

static INLINE unsigned
map_tgsi_setop_cc(unsigned op)
{
	switch (op) {
	case TGSI_OPCODE_SLT: return 0x1;
	case TGSI_OPCODE_SGE: return 0x6;
	case TGSI_OPCODE_SEQ: return 0x2;
	case TGSI_OPCODE_SGT: return 0x4;
	case TGSI_OPCODE_SLE: return 0x3;
	case TGSI_OPCODE_SNE: return 0xd;
	default:
		assert(0);
		return 0;
	}
}

static INLINE void
emit_flr(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src)
{
	emit_cvt(pc, dst, src, -1, CVTOP_FLOOR, CVT_F32_F32 | CVT_RI);
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

static INLINE void
emit_sat(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src)
{
	emit_cvt(pc, dst, src, -1, CVTOP_SAT, CVT_F32_F32);
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

static INLINE void
emit_neg(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src)
{
	emit_cvt(pc, dst, src, -1, CVTOP_RN, CVT_F32_F32 | CVT_NEG);
}

static void
emit_kil(struct nv50_pc *pc, struct nv50_reg *src)
{
	struct nv50_program_exec *e;
	const int r_pred = 1;
	unsigned cvn = CVT_F32_F32;

	if (src->mod & NV50_MOD_NEG)
		cvn |= CVT_NEG;
	/* write predicate reg */
	emit_cvt(pc, NULL, src, r_pred, CVTOP_RN, cvn);

	/* conditional discard */
	e = exec(pc);
	e->inst[0] = 0x00000002;
	set_long(pc, e);
	set_pred(pc, 0x1 /* LT */, r_pred, e);
	emit(pc, e);
}

static void
load_cube_tex_coords(struct nv50_pc *pc, struct nv50_reg *t[4],
		     struct nv50_reg **src, boolean proj)
{
	int mod[3] = { src[0]->mod, src[1]->mod, src[2]->mod };

	src[0]->mod |= NV50_MOD_ABS;
	src[1]->mod |= NV50_MOD_ABS;
	src[2]->mod |= NV50_MOD_ABS;

	emit_minmax(pc, 4, t[2], src[0], src[1]);
	emit_minmax(pc, 4, t[2], src[2], t[2]);

	src[0]->mod = mod[0];
	src[1]->mod = mod[1];
	src[2]->mod = mod[2];

	if (proj && 0 /* looks more correct without this */)
		emit_mul(pc, t[2], t[2], src[3]);
	emit_flop(pc, 0, t[2], t[2]);

	emit_mul(pc, t[0], src[0], t[2]);
	emit_mul(pc, t[1], src[1], t[2]);
	emit_mul(pc, t[2], src[2], t[2]);
}

static void
emit_tex(struct nv50_pc *pc, struct nv50_reg **dst, unsigned mask,
	 struct nv50_reg **src, unsigned unit, unsigned type, boolean proj)
{
	struct nv50_reg *t[4];
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

	if (type == TGSI_TEXTURE_CUBE) {
		load_cube_tex_coords(pc, t, src, proj);
	} else
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
		for (c = 0; c < dim; c++)
			emit_mov(pc, t[c], src[c]);
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
	if (dim == 3) {
		e->inst[0] |= 0x00800000;
		if (type == TGSI_TEXTURE_CUBE)
			e->inst[0] |= 0x08000000;
	}

	e->inst[0] |= (mask & 0x3) << 25;
	e->inst[1] |= (mask & 0xc) << 12;

	emit(pc, e);
#if 1
	c = 0;
	if (mask & 1) emit_mov(pc, dst[0], t[c++]);
	if (mask & 2) emit_mov(pc, dst[1], t[c++]);
	if (mask & 4) emit_mov(pc, dst[2], t[c++]);
	if (mask & 8) emit_mov(pc, dst[3], t[c]);

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
emit_branch(struct nv50_pc *pc, int pred, unsigned cc,
	    struct nv50_program_exec **join)
{
	struct nv50_program_exec *e = exec(pc);

	if (join) {
		set_long(pc, e);
		e->inst[0] |= 0xa0000002;
		emit(pc, e);
		*join = e;
		e = exec(pc);
	}

	set_long(pc, e);
	e->inst[0] |= 0x10000002;
	if (pred >= 0)
		set_pred(pc, cc, pred, e);
	emit(pc, e);
}

static void
emit_nop(struct nv50_pc *pc)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] = 0xf0000000;
	set_long(pc, e);
	e->inst[1] = 0xe0000000;
	emit(pc, e);
}

static void
emit_ddx(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src)
{
	struct nv50_program_exec *e = exec(pc);

	assert(src->type == P_TEMP);

	e->inst[0] = 0xc0140000;
	e->inst[1] = 0x89800000;
	set_long(pc, e);
	set_dst(pc, dst, e);
	set_src_0(pc, src, e);
	set_src_2(pc, src, e);

	emit(pc, e);
}

static void
emit_ddy(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src)
{
	struct nv50_program_exec *e = exec(pc);

	assert(src->type == P_TEMP);

	if (!(src->mod & NV50_MOD_NEG)) /* ! double negation */
		emit_neg(pc, src, src);

	e->inst[0] = 0xc0150000;
	e->inst[1] = 0x8a400000;
	set_long(pc, e);
	set_dst(pc, dst, e);
	set_src_0(pc, src, e);
	set_src_2(pc, src, e);

	emit(pc, e);
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

/* Some operations support an optional negation flag. */
static boolean
negate_supported(const struct tgsi_full_instruction *insn, int i)
{
	int s;

	switch (insn->Instruction.Opcode) {
	case TGSI_OPCODE_DDY:
	case TGSI_OPCODE_DP3:
	case TGSI_OPCODE_DP4:
	case TGSI_OPCODE_MUL:
	case TGSI_OPCODE_KIL:
	case TGSI_OPCODE_ADD:
	case TGSI_OPCODE_SUB:
	case TGSI_OPCODE_MAD:
		break;
	case TGSI_OPCODE_POW:
		if (i == 1)
			break;
		return FALSE;
	default:
		return FALSE;
	}

	/* Watch out for possible multiple uses of an nv50_reg, we
	 * can't use nv50_reg::neg in these cases.
	 */
	for (s = 0; s < insn->Instruction.NumSrcRegs; ++s) {
		if (s == i)
			continue;
		if ((insn->FullSrcRegisters[s].SrcRegister.Index ==
		     insn->FullSrcRegisters[i].SrcRegister.Index) &&
		    (insn->FullSrcRegisters[s].SrcRegister.File ==
		     insn->FullSrcRegisters[i].SrcRegister.File))
			return FALSE;
	}

	return TRUE;
}

/* Return a read mask for source registers deduced from opcode & write mask. */
static unsigned
nv50_tgsi_src_mask(const struct tgsi_full_instruction *insn, int c)
{
	unsigned x, mask = insn->FullDstRegisters[0].DstRegister.WriteMask;

	switch (insn->Instruction.Opcode) {
	case TGSI_OPCODE_COS:
	case TGSI_OPCODE_SIN:
		return (mask & 0x8) | ((mask & 0x7) ? 0x1 : 0x0);
	case TGSI_OPCODE_DP3:
		return 0x7;
	case TGSI_OPCODE_DP4:
	case TGSI_OPCODE_DPH:
	case TGSI_OPCODE_KIL: /* WriteMask ignored */
		return 0xf;
	case TGSI_OPCODE_DST:
		return mask & (c ? 0xa : 0x6);
	case TGSI_OPCODE_EX2:
	case TGSI_OPCODE_LG2:
	case TGSI_OPCODE_POW:
	case TGSI_OPCODE_RCP:
	case TGSI_OPCODE_RSQ:
	case TGSI_OPCODE_SCS:
		return 0x1;
	case TGSI_OPCODE_LIT:
		return 0xb;
	case TGSI_OPCODE_TEX:
	case TGSI_OPCODE_TXP:
	{
		const struct tgsi_instruction_ext_texture *tex;

		assert(insn->Instruction.Extended);
		tex = &insn->InstructionExtTexture;

		mask = 0x7;
		if (insn->Instruction.Opcode == TGSI_OPCODE_TXP)
			mask |= 0x8;

		switch (tex->Texture) {
		case TGSI_TEXTURE_1D:
			mask &= 0x9;
			break;
		case TGSI_TEXTURE_2D:
			mask &= 0xb;
			break;
		default:
			break;
		}
	}
		return mask;
	case TGSI_OPCODE_XPD:
		x = 0;
		if (mask & 1) x |= 0x6;
		if (mask & 2) x |= 0x5;
		if (mask & 4) x |= 0x3;
		return x;
	default:
		break;
	}

	return mask;
}

static struct nv50_reg *
tgsi_dst(struct nv50_pc *pc, int c, const struct tgsi_full_dst_register *dst)
{
	switch (dst->DstRegister.File) {
	case TGSI_FILE_TEMPORARY:
		return &pc->temp[dst->DstRegister.Index * 4 + c];
	case TGSI_FILE_OUTPUT:
		return &pc->result[dst->DstRegister.Index * 4 + c];
	case TGSI_FILE_ADDRESS:
	{
		struct nv50_reg *r = pc->addr[dst->DstRegister.Index * 4 + c];
		if (!r) {
			r = alloc_addr(pc, NULL);
			pc->addr[dst->DstRegister.Index * 4 + c] = r;
		}
		assert(r);
		return r;
	}
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
	unsigned sgn, c, swz;

	if (src->SrcRegister.File != TGSI_FILE_CONSTANT)
		assert(!src->SrcRegister.Indirect);

	sgn = tgsi_util_get_full_src_register_sign_mode(src, chan);

	c = tgsi_util_get_full_src_register_swizzle(src, chan);
	switch (c) {
	case TGSI_SWIZZLE_X:
	case TGSI_SWIZZLE_Y:
	case TGSI_SWIZZLE_Z:
	case TGSI_SWIZZLE_W:
		switch (src->SrcRegister.File) {
		case TGSI_FILE_INPUT:
			r = &pc->attr[src->SrcRegister.Index * 4 + c];
			break;
		case TGSI_FILE_TEMPORARY:
			r = &pc->temp[src->SrcRegister.Index * 4 + c];
			break;
		case TGSI_FILE_CONSTANT:
			if (!src->SrcRegister.Indirect) {
				r = &pc->param[src->SrcRegister.Index * 4 + c];
				break;
			}
			/* Indicate indirection by setting r->acc < 0 and
			 * use the index field to select the address reg.
			 */
			r = MALLOC_STRUCT(nv50_reg);
			swz = tgsi_util_get_src_register_swizzle(
						 &src->SrcRegisterInd, 0);
			ctor_reg(r, P_CONST,
				 src->SrcRegisterInd.Index * 4 + swz,
				 src->SrcRegister.Index * 4 + c);
			r->acc = -1;
			break;
		case TGSI_FILE_IMMEDIATE:
			r = &pc->immd[src->SrcRegister.Index * 4 + c];
			break;
		case TGSI_FILE_SAMPLER:
			break;
		case TGSI_FILE_ADDRESS:
			r = pc->addr[src->SrcRegister.Index * 4 + c];
			assert(r);
			break;
		default:
			assert(0);
			break;
		}
		break;
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
			r->mod = NV50_MOD_NEG;
		else {
			temp = temp_temp(pc);
			emit_neg(pc, temp, r);
			r = temp;
		}
		break;
	case TGSI_UTIL_SIGN_SET:
		temp = temp_temp(pc);
		emit_cvt(pc, temp, r, -1, CVTOP_ABS, CVT_F32_F32 | CVT_NEG);
		r = temp;
		break;
	default:
		assert(0);
		break;
	}

	return r;
}

/* return TRUE for ops that produce only a single result */
static boolean
is_scalar_op(unsigned op)
{
	switch (op) {
	case TGSI_OPCODE_COS:
	case TGSI_OPCODE_DP2:
	case TGSI_OPCODE_DP3:
	case TGSI_OPCODE_DP4:
	case TGSI_OPCODE_DPH:
	case TGSI_OPCODE_EX2:
	case TGSI_OPCODE_LG2:
	case TGSI_OPCODE_POW:
	case TGSI_OPCODE_RCP:
	case TGSI_OPCODE_RSQ:
	case TGSI_OPCODE_SIN:
		/*
	case TGSI_OPCODE_KIL:
	case TGSI_OPCODE_LIT:
	case TGSI_OPCODE_SCS:
		*/
		return TRUE;
	default:
		return FALSE;
	}
}

/* Returns a bitmask indicating which dst components depend
 * on source s, component c (reverse of nv50_tgsi_src_mask).
 */
static unsigned
nv50_tgsi_dst_revdep(unsigned op, int s, int c)
{
	if (is_scalar_op(op))
		return 0x1;

	switch (op) {
	case TGSI_OPCODE_DST:
		return (1 << c) & (s ? 0xa : 0x6);
	case TGSI_OPCODE_XPD:
		switch (c) {
		case 0: return 0x6;
		case 1: return 0x5;
		case 2: return 0x3;
		case 3: return 0x0;
		default:
			assert(0);
			return 0x0;
		}
	case TGSI_OPCODE_LIT:
	case TGSI_OPCODE_SCS:
	case TGSI_OPCODE_TEX:
	case TGSI_OPCODE_TXP:
		/* these take care of dangerous swizzles themselves */
		return 0x0;
	case TGSI_OPCODE_IF:
	case TGSI_OPCODE_KIL:
		/* don't call this function for these ops */
		assert(0);
		return 0;
	default:
		/* linear vector instruction */
		return (1 << c);
	}
}

static INLINE boolean
has_pred(struct nv50_program_exec *e, unsigned cc)
{
	if (!is_long(e) || is_immd(e))
		return FALSE;
	return ((e->inst[1] & 0x780) == (cc << 7));
}

/* on ENDIF see if we can do "@p0.neu single_op" instead of:
 *        join_at ENDIF
 *        @p0.eq bra ENDIF
 *        single_op
 * ENDIF: nop.join
 */
static boolean
nv50_kill_branch(struct nv50_pc *pc)
{
	int lvl = pc->if_lvl;

	if (pc->if_insn[lvl]->next != pc->p->exec_tail)
		return FALSE;

	/* if ccode == 'true', the BRA is from an ELSE and the predicate
	 * reg may no longer be valid, since we currently always use $p0
	 */
	if (has_pred(pc->if_insn[lvl], 0xf))
		return FALSE;
	assert(pc->if_insn[lvl] && pc->br_join[lvl]);

	/* We'll use the exec allocated for JOIN_AT (as we can't easily
	 * update prev's next); if exec_tail is BRK, update the pointer.
	 */
	if (pc->loop_lvl && pc->br_loop[pc->loop_lvl - 1] == pc->p->exec_tail)
		pc->br_loop[pc->loop_lvl - 1] = pc->br_join[lvl];

	pc->p->exec_size -= 4; /* remove JOIN_AT and BRA */

	*pc->br_join[lvl] = *pc->p->exec_tail;

	FREE(pc->if_insn[lvl]);
	FREE(pc->p->exec_tail);

	pc->p->exec_tail = pc->br_join[lvl];
	pc->p->exec_tail->next = NULL;
	set_pred(pc, 0xd, 0, pc->p->exec_tail);

	return TRUE;
}

static boolean
nv50_program_tx_insn(struct nv50_pc *pc,
		     const struct tgsi_full_instruction *inst)
{
	struct nv50_reg *rdst[4], *dst[4], *brdc, *src[3][4], *temp;
	unsigned mask, sat, unit;
	int i, c;

	mask = inst->FullDstRegisters[0].DstRegister.WriteMask;
	sat = inst->Instruction.Saturate == TGSI_SAT_ZERO_ONE;

	memset(src, 0, sizeof(src));

	for (c = 0; c < 4; c++) {
		if ((mask & (1 << c)) && !pc->r_dst[c])
			dst[c] = tgsi_dst(pc, c, &inst->FullDstRegisters[0]);
		else
			dst[c] = pc->r_dst[c];
		rdst[c] = dst[c];
	}

	for (i = 0; i < inst->Instruction.NumSrcRegs; i++) {
		const struct tgsi_full_src_register *fs = &inst->FullSrcRegisters[i];
		unsigned src_mask;
		boolean neg_supp;

		src_mask = nv50_tgsi_src_mask(inst, i);
		neg_supp = negate_supported(inst, i);

		if (fs->SrcRegister.File == TGSI_FILE_SAMPLER)
			unit = fs->SrcRegister.Index;

		for (c = 0; c < 4; c++)
			if (src_mask & (1 << c))
				src[i][c] = tgsi_src(pc, c, fs, neg_supp);
	}

	brdc = temp = pc->r_brdc;
	if (brdc && brdc->type != P_TEMP) {
		temp = temp_temp(pc);
		if (sat)
			brdc = temp;
	} else
	if (sat) {
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)) || dst[c]->type == P_TEMP)
				continue;
			/* rdst[c] = dst[c]; */ /* done above */
			dst[c] = temp_temp(pc);
		}
	}

	assert(brdc || !is_scalar_op(inst->Instruction.Opcode));

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
	case TGSI_OPCODE_AND:
	case TGSI_OPCODE_XOR:
	case TGSI_OPCODE_OR:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_bitop2(pc, dst[c], src[0][c], src[1][c],
				    inst->Instruction.Opcode);
		}
		break;
	case TGSI_OPCODE_ARL:
		assert(src[0][0]);
		temp = temp_temp(pc);
		emit_cvt(pc, temp, src[0][0], -1, CVTOP_FLOOR, CVT_S32_F32);
		emit_arl(pc, dst[0], temp, 4);
		break;
	case TGSI_OPCODE_BGNLOOP:
		pc->loop_pos[pc->loop_lvl++] = pc->p->exec_size;
		terminate_mbb(pc);
		break;
	case TGSI_OPCODE_BRK:
		emit_branch(pc, -1, 0, NULL);
		assert(pc->loop_lvl > 0);
		pc->br_loop[pc->loop_lvl - 1] = pc->p->exec_tail;
		break;
	case TGSI_OPCODE_CEIL:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_cvt(pc, dst[c], src[0][c], -1,
				 CVTOP_CEIL, CVT_F32_F32 | CVT_RI);
		}
		break;
	case TGSI_OPCODE_CMP:
		pc->allow32 = FALSE;
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_cvt(pc, NULL, src[0][c], 1, CVTOP_RN, CVT_F32_F32);
			emit_mov(pc, dst[c], src[1][c]);
			set_pred(pc, 0x1, 1, pc->p->exec_tail); /* @SF */
			emit_mov(pc, dst[c], src[2][c]);
			set_pred(pc, 0x6, 1, pc->p->exec_tail); /* @NSF */
		}
		break;
	case TGSI_OPCODE_COS:
		if (mask & 8) {
			emit_precossin(pc, temp, src[0][3]);
			emit_flop(pc, 5, dst[3], temp);
			if (!(mask &= 7))
				break;
			if (temp == dst[3])
				temp = brdc = temp_temp(pc);
		}
		emit_precossin(pc, temp, src[0][0]);
		emit_flop(pc, 5, brdc, temp);
		break;
	case TGSI_OPCODE_DDX:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_ddx(pc, dst[c], src[0][c]);
		}
		break;
	case TGSI_OPCODE_DDY:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_ddy(pc, dst[c], src[0][c]);
		}
		break;
	case TGSI_OPCODE_DP3:
		emit_mul(pc, temp, src[0][0], src[1][0]);
		emit_mad(pc, temp, src[0][1], src[1][1], temp);
		emit_mad(pc, brdc, src[0][2], src[1][2], temp);
		break;
	case TGSI_OPCODE_DP4:
		emit_mul(pc, temp, src[0][0], src[1][0]);
		emit_mad(pc, temp, src[0][1], src[1][1], temp);
		emit_mad(pc, temp, src[0][2], src[1][2], temp);
		emit_mad(pc, brdc, src[0][3], src[1][3], temp);
		break;
	case TGSI_OPCODE_DPH:
		emit_mul(pc, temp, src[0][0], src[1][0]);
		emit_mad(pc, temp, src[0][1], src[1][1], temp);
		emit_mad(pc, temp, src[0][2], src[1][2], temp);
		emit_add(pc, brdc, src[1][3], temp);
		break;
	case TGSI_OPCODE_DST:
		if (mask & (1 << 1))
			emit_mul(pc, dst[1], src[0][1], src[1][1]);
		if (mask & (1 << 2))
			emit_mov(pc, dst[2], src[0][2]);
		if (mask & (1 << 3))
			emit_mov(pc, dst[3], src[1][3]);
		if (mask & (1 << 0))
			emit_mov_immdval(pc, dst[0], 1.0f);
		break;
	case TGSI_OPCODE_ELSE:
		emit_branch(pc, -1, 0, NULL);
		pc->if_insn[--pc->if_lvl]->param.index = pc->p->exec_size;
		pc->if_insn[pc->if_lvl++] = pc->p->exec_tail;
		terminate_mbb(pc);
		break;
	case TGSI_OPCODE_ENDIF:
		pc->if_insn[--pc->if_lvl]->param.index = pc->p->exec_size;

		/* try to replace branch over 1 insn with a predicated insn */
		if (nv50_kill_branch(pc) == TRUE)
			break;

		if (pc->br_join[pc->if_lvl]) {
			pc->br_join[pc->if_lvl]->param.index = pc->p->exec_size;
			pc->br_join[pc->if_lvl] = NULL;
		}
		terminate_mbb(pc);
		/* emit a NOP as join point, we could set it on the next
		 * one, but would have to make sure it is long and !immd
		 */
		emit_nop(pc);
		pc->p->exec_tail->inst[1] |= 2;
		break;
	case TGSI_OPCODE_ENDLOOP:
		emit_branch(pc, -1, 0, NULL);
		pc->p->exec_tail->param.index = pc->loop_pos[--pc->loop_lvl];
		pc->br_loop[pc->loop_lvl]->param.index = pc->p->exec_size;
		terminate_mbb(pc);
		break;
	case TGSI_OPCODE_EX2:
		emit_preex2(pc, temp, src[0][0]);
		emit_flop(pc, 6, brdc, temp);
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
	case TGSI_OPCODE_IF:
		/* emitting a join_at may not be necessary */
		assert(pc->if_lvl < MAX_IF_DEPTH);
		/* set_pred_wr(pc, 1, 0, pc->if_cond); */
		emit_cvt(pc, NULL, src[0][0], 0, CVTOP_ABS | CVTOP_RN,
			 CVT_F32_F32);
		emit_branch(pc, 0, 2, &pc->br_join[pc->if_lvl]);
		pc->if_insn[pc->if_lvl++] = pc->p->exec_tail;
		terminate_mbb(pc);
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
		emit_flop(pc, 3, brdc, src[0][0]);
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
		emit_pow(pc, brdc, src[0][0], src[1][0]);
		break;
	case TGSI_OPCODE_RCP:
		emit_flop(pc, 0, brdc, src[0][0]);
		break;
	case TGSI_OPCODE_RSQ:
		emit_flop(pc, 2, brdc, src[0][0]);
		break;
	case TGSI_OPCODE_SCS:
		temp = temp_temp(pc);
		if (mask & 3)
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
	case TGSI_OPCODE_SIN:
		if (mask & 8) {
			emit_precossin(pc, temp, src[0][3]);
			emit_flop(pc, 4, dst[3], temp);
			if (!(mask &= 7))
				break;
			if (temp == dst[3])
				temp = brdc = temp_temp(pc);
		}
		emit_precossin(pc, temp, src[0][0]);
		emit_flop(pc, 4, brdc, temp);
		break;
	case TGSI_OPCODE_SLT:
	case TGSI_OPCODE_SGE:
	case TGSI_OPCODE_SEQ:
	case TGSI_OPCODE_SGT:
	case TGSI_OPCODE_SLE:
	case TGSI_OPCODE_SNE:
		i = map_tgsi_setop_cc(inst->Instruction.Opcode);
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_set(pc, i, dst[c], -1, src[0][c], src[1][c]);
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
	case TGSI_OPCODE_TRUNC:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_cvt(pc, dst[c], src[0][c], -1,
				 CVTOP_TRUNC, CVT_F32_F32 | CVT_RI);
		}
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

	if (brdc) {
		if (sat)
			emit_sat(pc, brdc, brdc);
		for (c = 0; c < 4; c++)
			if ((mask & (1 << c)) && dst[c] != brdc)
				emit_mov(pc, dst[c], brdc);
	} else
	if (sat) {
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			/* In this case we saturate later, and dst[c] won't
			 * be another temp_temp (and thus lost), since rdst
			 * already is TEMP (see above). */
			if (rdst[c]->type == P_TEMP && rdst[c]->index < 0)
				continue;
			emit_sat(pc, rdst[c], dst[c]);
		}
	}

	for (i = 0; i < inst->Instruction.NumSrcRegs; i++) {
		for (c = 0; c < 4; c++) {
			if (!src[i][c])
				continue;
			src[i][c]->mod = 0;
			if (src[i][c]->index == -1 && src[i][c]->type == P_IMMD)
				FREE(src[i][c]);
			else
			if (src[i][c]->acc < 0 && src[i][c]->type == P_CONST)
				FREE(src[i][c]); /* indirect constant */
		}
	}

	kill_temp_temp(pc);
	return TRUE;
}

static void
prep_inspect_insn(struct nv50_pc *pc, const struct tgsi_full_instruction *insn)
{
	struct nv50_reg *reg = NULL;
	const struct tgsi_full_src_register *src;
	const struct tgsi_dst_register *dst;
	unsigned i, c, k, mask;

	dst = &insn->FullDstRegisters[0].DstRegister;
	mask = dst->WriteMask;

        if (dst->File == TGSI_FILE_TEMPORARY)
                reg = pc->temp;
        else
        if (dst->File == TGSI_FILE_OUTPUT)
                reg = pc->result;

	if (reg) {
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			reg[dst->Index * 4 + c].acc = pc->insn_nr;
		}
	}

	for (i = 0; i < insn->Instruction.NumSrcRegs; i++) {
		src = &insn->FullSrcRegisters[i];

		if (src->SrcRegister.File == TGSI_FILE_TEMPORARY)
			reg = pc->temp;
		else
		if (src->SrcRegister.File == TGSI_FILE_INPUT)
			reg = pc->attr;
		else
			continue;

		mask = nv50_tgsi_src_mask(insn, i);

		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			k = tgsi_util_get_full_src_register_swizzle(src, c);

			reg[src->SrcRegister.Index * 4 + k].acc = pc->insn_nr;
		}
	}
}

/* Returns a bitmask indicating which dst components need to be
 * written to temporaries first to avoid 'corrupting' sources.
 *
 * m[i]   (out) indicate component to write in the i-th position
 * rdep[c] (in) bitmasks of dst[i] that require dst[c] as source
 */
static unsigned
nv50_revdep_reorder(unsigned m[4], unsigned rdep[4])
{
	unsigned i, c, x, unsafe;

	for (c = 0; c < 4; c++)
		m[c] = c;

	/* Swap as long as a dst component written earlier is depended on
	 * by one written later, but the next one isn't depended on by it.
	 */
	for (c = 0; c < 3; c++) {
		if (rdep[m[c + 1]] & (1 << m[c]))
			continue; /* if next one is depended on by us */
		for (i = c + 1; i < 4; i++)
			/* if we are depended on by a later one */
			if (rdep[m[c]] & (1 << m[i]))
				break;
		if (i == 4)
			continue;
		/* now, swap */
		x = m[c];
		m[c] = m[c + 1];
		m[c + 1] = x;

		/* restart */
		c = 0;
	}

	/* mark dependencies that could not be resolved by reordering */
	for (i = 0; i < 3; ++i)
		for (c = i + 1; c < 4; ++c)
			if (rdep[m[i]] & (1 << m[c]))
				unsafe |= (1 << i);

	/* NOTE: $unsafe is with respect to order, not component */
	return unsafe;
}

/* Select a suitable dst register for broadcasting scalar results,
 * or return NULL if we have to allocate an extra TEMP.
 *
 * If e.g. only 1 component is written, we may also emit the final
 * result to a write-only register.
 */
static struct nv50_reg *
tgsi_broadcast_dst(struct nv50_pc *pc,
		   const struct tgsi_full_dst_register *fd, unsigned mask)
{
	if (fd->DstRegister.File == TGSI_FILE_TEMPORARY) {
		int c = ffs(~mask & fd->DstRegister.WriteMask);
		if (c)
			return tgsi_dst(pc, c - 1, fd);
	} else {
		int c = ffs(fd->DstRegister.WriteMask) - 1;
		if ((1 << c) == fd->DstRegister.WriteMask)
			return tgsi_dst(pc, c, fd);
	}

	return NULL;
}

/* Scan source swizzles and return a bitmask indicating dst regs that
 * also occur among the src regs, and fill rdep for nv50_revdep_reoder.
 */
static unsigned
nv50_tgsi_scan_swizzle(const struct tgsi_full_instruction *insn,
		       unsigned rdep[4])
{
	const struct tgsi_full_dst_register *fd = &insn->FullDstRegisters[0];
	const struct tgsi_full_src_register *fs;
	unsigned i, deqs = 0;

	for (i = 0; i < 4; ++i)
		rdep[i] = 0;

	for (i = 0; i < insn->Instruction.NumSrcRegs; i++) {
		unsigned chn, mask = nv50_tgsi_src_mask(insn, i);
		boolean neg_supp = negate_supported(insn, i);

		fs = &insn->FullSrcRegisters[i];
		if (fs->SrcRegister.File != fd->DstRegister.File ||
		    fs->SrcRegister.Index != fd->DstRegister.Index)
			continue;

		for (chn = 0; chn < 4; ++chn) {
			unsigned s, c;

			if (!(mask & (1 << chn))) /* src is not read */
				continue;
			c = tgsi_util_get_full_src_register_swizzle(fs, chn);
			s = tgsi_util_get_full_src_register_sign_mode(fs, chn);

			if (!(fd->DstRegister.WriteMask & (1 << c)))
				continue;

			/* no danger if src is copied to TEMP first */
			if ((s != TGSI_UTIL_SIGN_KEEP) &&
			    (s != TGSI_UTIL_SIGN_TOGGLE || !neg_supp))
				continue;

			rdep[c] |= nv50_tgsi_dst_revdep(
				insn->Instruction.Opcode, i, chn);
			deqs |= (1 << c);
		}
	}

	return deqs;
}

static boolean
nv50_tgsi_insn(struct nv50_pc *pc, const union tgsi_full_token *tok)
{
	struct tgsi_full_instruction insn = tok->FullInstruction;
	const struct tgsi_full_dst_register *fd;
	unsigned i, deqs, rdep[4], m[4];

	fd = &tok->FullInstruction.FullDstRegisters[0];
	deqs = nv50_tgsi_scan_swizzle(&insn, rdep);

	if (is_scalar_op(insn.Instruction.Opcode)) {
		pc->r_brdc = tgsi_broadcast_dst(pc, fd, deqs);
		if (!pc->r_brdc)
			pc->r_brdc = temp_temp(pc);
		return nv50_program_tx_insn(pc, &insn);
	}
	pc->r_brdc = NULL;

	if (!deqs)
		return nv50_program_tx_insn(pc, &insn);

	deqs = nv50_revdep_reorder(m, rdep);

	for (i = 0; i < 4; ++i) {
		assert(pc->r_dst[m[i]] == NULL);

		insn.FullDstRegisters[0].DstRegister.WriteMask =
			fd->DstRegister.WriteMask & (1 << m[i]);

		if (!insn.FullDstRegisters[0].DstRegister.WriteMask)
			continue;

		if (deqs & (1 << i))
			pc->r_dst[m[i]] = alloc_temp(pc, NULL);

		if (!nv50_program_tx_insn(pc, &insn))
			return FALSE;
	}

	for (i = 0; i < 4; i++) {
		struct nv50_reg *reg = pc->r_dst[i];
		if (!reg)
			continue;
		pc->r_dst[i] = NULL;

		if (insn.Instruction.Saturate == TGSI_SAT_ZERO_ONE)
			emit_sat(pc, tgsi_dst(pc, i, fd), reg);
		else
			emit_mov(pc, tgsi_dst(pc, i, fd), reg);
		free_temp(pc, reg);
	}

	return TRUE;
}

static void
load_interpolant(struct nv50_pc *pc, struct nv50_reg *reg)
{
	struct nv50_reg *iv, **ppiv;
	unsigned mode = pc->interp_mode[reg->index];

	ppiv = (mode & INTERP_CENTROID) ? &pc->iv_c : &pc->iv_p;
	iv = *ppiv;

	if ((mode & INTERP_PERSPECTIVE) && !iv) {
		iv = *ppiv = alloc_temp(pc, NULL);
		iv->rhw = popcnt4(pc->p->cfg.regs[1] >> 24) - 1;

		emit_interp(pc, iv, NULL, mode & INTERP_CENTROID);
		emit_flop(pc, 0, iv, iv);

		/* XXX: when loading interpolants dynamically, move these
		 * to the program head, or make sure it can't be skipped.
		 */
	}

	emit_interp(pc, reg, iv, mode);
}

/* The face input is always at v[255] (varying space), with a
 * value of 0 for back-facing, and 0xffffffff for front-facing.
 */
static void
load_frontfacing(struct nv50_pc *pc, struct nv50_reg *a)
{
	struct nv50_reg *one = alloc_immd(pc, 1.0f);

	assert(a->rhw == -1);
	alloc_reg(pc, a); /* do this before rhw is set */
	a->rhw = 255;
	load_interpolant(pc, a);
	emit_bitop2(pc, a, a, one, TGSI_OPCODE_AND);

	FREE(one);
}

static boolean
nv50_program_tx_prep(struct nv50_pc *pc)
{
	struct tgsi_parse_context tp;
	struct nv50_program *p = pc->p;
	boolean ret = FALSE;
	unsigned i, c, flat_nr = 0;

	tgsi_parse_init(&tp, pc->p->pipe.tokens);
	while (!tgsi_parse_end_of_tokens(&tp)) {
		const union tgsi_full_token *tok = &tp.FullToken;

		tgsi_parse_token(&tp);
		switch (tok->Token.Type) {
		case TGSI_TOKEN_TYPE_IMMEDIATE:
		{
			const struct tgsi_full_immediate *imm =
				&tp.FullToken.FullImmediate;

			ctor_immd(pc, imm->u[0].Float,
				      imm->u[1].Float,
				      imm->u[2].Float,
				      imm->u[3].Float);
		}
			break;
		case TGSI_TOKEN_TYPE_DECLARATION:
		{
			const struct tgsi_full_declaration *d;
			unsigned si, last, first, mode;

			d = &tp.FullToken.FullDeclaration;
			first = d->DeclarationRange.First;
			last = d->DeclarationRange.Last;

			switch (d->Declaration.File) {
			case TGSI_FILE_TEMPORARY:
				break;
			case TGSI_FILE_OUTPUT:
				if (!d->Declaration.Semantic ||
				    p->type == PIPE_SHADER_FRAGMENT)
					break;

				si = d->Semantic.SemanticIndex;
				switch (d->Semantic.SemanticName) {
				case TGSI_SEMANTIC_BCOLOR:
					p->cfg.two_side[si].hw = first;
					if (p->cfg.io_nr > first)
						p->cfg.io_nr = first;
					break;
				case TGSI_SEMANTIC_PSIZE:
					p->cfg.psiz = first;
					if (p->cfg.io_nr > first)
						p->cfg.io_nr = first;
					break;
					/*
				case TGSI_SEMANTIC_CLIP_DISTANCE:
					p->cfg.clpd = MIN2(p->cfg.clpd, first);
					break;
					*/
				default:
					break;
				}
				break;
			case TGSI_FILE_INPUT:
			{
				if (p->type != PIPE_SHADER_FRAGMENT)
					break;

				switch (d->Declaration.Interpolate) {
				case TGSI_INTERPOLATE_CONSTANT:
					mode = INTERP_FLAT;
					flat_nr++;
					break;
				case TGSI_INTERPOLATE_PERSPECTIVE:
					mode = INTERP_PERSPECTIVE;
					p->cfg.regs[1] |= 0x08 << 24;
					break;
				default:
					mode = INTERP_LINEAR;
					break;
				}
				if (d->Declaration.Centroid)
					mode |= INTERP_CENTROID;

				assert(last < 32);
				for (i = first; i <= last; i++)
					pc->interp_mode[i] = mode;
			}
				break;
			case TGSI_FILE_ADDRESS:
			case TGSI_FILE_CONSTANT:
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
			prep_inspect_insn(pc, &tok->FullInstruction);
			break;
		default:
			break;
		}
	}

	if (p->type == PIPE_SHADER_VERTEX) {
		int rid = 0;

		for (i = 0; i < pc->attr_nr * 4; ++i) {
			if (pc->attr[i].acc) {
				pc->attr[i].hw = rid++;
				p->cfg.attr[i / 32] |= 1 << (i % 32);
			}
		}

		for (i = 0, rid = 0; i < pc->result_nr; ++i) {
			p->cfg.io[i].hw = rid;
			p->cfg.io[i].id_vp = i;

			for (c = 0; c < 4; ++c) {
				int n = i * 4 + c;
				if (!pc->result[n].acc)
					continue;
				pc->result[n].hw = rid++;
				p->cfg.io[i].mask |= 1 << c;
			}
		}

		for (c = 0; c < 2; ++c)
			if (p->cfg.two_side[c].hw < 0x40)
				p->cfg.two_side[c] = p->cfg.io[
					p->cfg.two_side[c].hw];

		if (p->cfg.psiz < 0x40)
			p->cfg.psiz = p->cfg.io[p->cfg.psiz].hw;
	} else
	if (p->type == PIPE_SHADER_FRAGMENT) {
		int rid, aid;
		unsigned n = 0, m = pc->attr_nr - flat_nr;

		pc->allow32 = TRUE;

		int base = (TGSI_SEMANTIC_POSITION ==
			    p->info.input_semantic_name[0]) ? 0 : 1;

		/* non-flat interpolants have to be mapped to
		 * the lower hardware IDs, so sort them:
		 */
		for (i = 0; i < pc->attr_nr; i++) {
			if (pc->interp_mode[i] == INTERP_FLAT) {
				p->cfg.io[m].id_vp = i + base;
				p->cfg.io[m++].id_fp = i;
			} else {
				if (!(pc->interp_mode[i] & INTERP_PERSPECTIVE))
					p->cfg.io[n].linear = TRUE;
				p->cfg.io[n].id_vp = i + base;
				p->cfg.io[n++].id_fp = i;
			}
		}

		if (!base) /* set w-coordinate mask from perspective interp */
			p->cfg.io[0].mask |= p->cfg.regs[1] >> 24;

		aid = popcnt4( /* if fcrd isn't contained in cfg.io */
			base ? (p->cfg.regs[1] >> 24) : p->cfg.io[0].mask);

		for (n = 0; n < pc->attr_nr; ++n) {
			p->cfg.io[n].hw = rid = aid;
			i = p->cfg.io[n].id_fp;

			if (p->info.input_semantic_name[n] ==
			    TGSI_SEMANTIC_FACE) {
				load_frontfacing(pc, &pc->attr[i * 4]);
				continue;
			}

			for (c = 0; c < 4; ++c) {
				if (!pc->attr[i * 4 + c].acc)
					continue;
				pc->attr[i * 4 + c].rhw = rid++;
				p->cfg.io[n].mask |= 1 << c;

				load_interpolant(pc, &pc->attr[i * 4 + c]);
			}
			aid += popcnt4(p->cfg.io[n].mask);
		}

		if (!base)
			p->cfg.regs[1] |= p->cfg.io[0].mask << 24;

		m = popcnt4(p->cfg.regs[1] >> 24);

		/* set count of non-position inputs and of non-flat
		 * non-position inputs for FP_INTERPOLANT_CTRL
		 */
		p->cfg.regs[1] |= aid - m;

		if (flat_nr) {
			i = p->cfg.io[pc->attr_nr - flat_nr].hw;
			p->cfg.regs[1] |= (i - m) << 16;
		} else
			p->cfg.regs[1] |= p->cfg.regs[1] << 16;

		/* mark color semantic for light-twoside */
		n = 0x40;
		for (i = 0; i < pc->attr_nr; i++) {
			ubyte si, sn;

			sn = p->info.input_semantic_name[p->cfg.io[i].id_fp];
			si = p->info.input_semantic_index[p->cfg.io[i].id_fp];

			if (sn == TGSI_SEMANTIC_COLOR) {
				p->cfg.two_side[si] = p->cfg.io[i];

				/* increase colour count */
				p->cfg.regs[0] += popcnt4(
					p->cfg.two_side[si].mask) << 16;

				n = MIN2(n, p->cfg.io[i].hw - m);
			}
		}
		if (n < 0x40)
			p->cfg.regs[0] += n;

		/* Initialize FP results:
		 * FragDepth is always first TGSI and last hw output
		 */
		i = p->info.writes_z ? 4 : 0;
		for (rid = 0; i < pc->result_nr * 4; i++)
			pc->result[i].rhw = rid++;
		if (p->info.writes_z)
			pc->result[2].rhw = rid;

		p->cfg.high_result = rid;

		/* separate/different colour results for MRTs ? */
		if (pc->result_nr - (p->info.writes_z ? 1 : 0) > 1)
			p->cfg.regs[2] |= 1;
	}

	if (pc->immd_nr) {
		int rid = 0;

		pc->immd = MALLOC(pc->immd_nr * 4 * sizeof(struct nv50_reg));
		if (!pc->immd)
			goto out_err;

		for (i = 0; i < pc->immd_nr; i++) {
			for (c = 0; c < 4; c++, rid++)
				ctor_reg(&pc->immd[rid], P_IMMD, i, rid);
		}
	}

	ret = TRUE;
out_err:
	if (pc->iv_p)
		free_temp(pc, pc->iv_p);
	if (pc->iv_c)
		free_temp(pc, pc->iv_c);

	tgsi_parse_free(&tp);
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
ctor_nv50_pc(struct nv50_pc *pc, struct nv50_program *p)
{
	int i, c;
	unsigned rtype[2] = { P_ATTR, P_RESULT };

	pc->p = p;
	pc->temp_nr = p->info.file_max[TGSI_FILE_TEMPORARY] + 1;
	pc->attr_nr = p->info.file_max[TGSI_FILE_INPUT] + 1;
	pc->result_nr = p->info.file_max[TGSI_FILE_OUTPUT] + 1;
	pc->param_nr = p->info.file_max[TGSI_FILE_CONSTANT] + 1;
	pc->addr_nr = p->info.file_max[TGSI_FILE_ADDRESS] + 1;
	assert(pc->addr_nr <= 2);

	p->cfg.high_temp = 4;

	p->cfg.two_side[0].hw = 0x40;
	p->cfg.two_side[1].hw = 0x40;

	switch (p->type) {
	case PIPE_SHADER_VERTEX:
		p->cfg.psiz = 0x40;
		p->cfg.clpd = 0x40;
		p->cfg.io_nr = pc->result_nr;
		break;
	case PIPE_SHADER_FRAGMENT:
		rtype[0] = rtype[1] = P_TEMP;

		p->cfg.regs[0] = 0x01000004;
		p->cfg.io_nr = pc->attr_nr;

		if (p->info.writes_z) {
			p->cfg.regs[2] |= 0x00000100;
			p->cfg.regs[3] |= 0x00000011;
		}
		if (p->info.uses_kill)
			p->cfg.regs[2] |= 0x00100000;
		break;
	}

	if (pc->temp_nr) {
		pc->temp = MALLOC(pc->temp_nr * 4 * sizeof(struct nv50_reg));
		if (!pc->temp)
			return FALSE;

		for (i = 0; i < pc->temp_nr * 4; ++i)
			ctor_reg(&pc->temp[i], P_TEMP, i / 4, -1);
	}

	if (pc->attr_nr) {
		pc->attr = MALLOC(pc->attr_nr * 4 * sizeof(struct nv50_reg));
		if (!pc->attr)
			return FALSE;

		for (i = 0; i < pc->attr_nr * 4; ++i)
			ctor_reg(&pc->attr[i], rtype[0], i / 4, -1);
	}

	if (pc->result_nr) {
		unsigned nr = pc->result_nr * 4;

		pc->result = MALLOC(nr * sizeof(struct nv50_reg));
		if (!pc->result)
			return FALSE;

		for (i = 0; i < nr; ++i)
			ctor_reg(&pc->result[i], rtype[1], i / 4, -1);
	}

	if (pc->param_nr) {
		int rid = 0;

		pc->param = MALLOC(pc->param_nr * 4 * sizeof(struct nv50_reg));
		if (!pc->param)
			return FALSE;

		for (i = 0; i < pc->param_nr; ++i)
			for (c = 0; c < 4; ++c, ++rid)
				ctor_reg(&pc->param[rid], P_CONST, i, rid);
	}

	if (pc->addr_nr) {
		pc->addr = CALLOC(pc->addr_nr * 4, sizeof(struct nv50_reg *));
		if (!pc->addr)
			return FALSE;
	}
	for (i = 0; i < NV50_SU_MAX_ADDR; ++i)
		ctor_reg(&pc->r_addr[i], P_ADDR, -256, i + 1);

	return TRUE;
}

static void
nv50_fp_move_results(struct nv50_pc *pc)
{
	struct nv50_reg reg;
	unsigned i;

	ctor_reg(&reg, P_TEMP, -1, -1);

	for (i = 0; i < pc->result_nr * 4; ++i) {
		if (pc->result[i].rhw < 0 || pc->result[i].hw < 0)
			continue;
		if (pc->result[i].rhw != pc->result[i].hw) {
			reg.hw = pc->result[i].rhw;
			emit_mov(pc, &reg, &pc->result[i]);
		}
	}
}

static void
nv50_program_fixup_insns(struct nv50_pc *pc)
{
	struct nv50_program_exec *e, *prev = NULL, **bra_list;
	unsigned i, n, pos;

	bra_list = CALLOC(pc->p->exec_size, sizeof(struct nv50_program_exec *));

	/* Collect branch instructions, we need to adjust their offsets
	 * when converting 32 bit instructions to 64 bit ones
	 */
	for (n = 0, e = pc->p->exec_head; e; e = e->next)
		if (e->param.index >= 0 && !e->param.mask)
			bra_list[n++] = e;

	/* Make sure we don't have any single 32 bit instructions. */
	for (e = pc->p->exec_head, pos = 0; e; e = e->next) {
		pos += is_long(e) ? 2 : 1;

		if ((pos & 1) && (!e->next || is_long(e->next))) {
			for (i = 0; i < n; ++i)
				if (bra_list[i]->param.index >= pos)
					bra_list[i]->param.index += 1;
			convert_to_long(pc, e);
			++pos;
		}
		if (e->next)
			prev = e;
	}

	assert(!is_immd(pc->p->exec_head));
	assert(!is_immd(pc->p->exec_tail));

	/* last instruction must be long so it can have the end bit set */
	if (!is_long(pc->p->exec_tail)) {
		convert_to_long(pc, pc->p->exec_tail);
		if (prev)
			convert_to_long(pc, prev);
	}
	assert(!(pc->p->exec_tail->inst[1] & 2));
	/* set the end-bit */
	pc->p->exec_tail->inst[1] |= 1;

	FREE(bra_list);
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

	ret = ctor_nv50_pc(pc, p);
	if (ret == FALSE)
		goto out_cleanup;

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
			ret = nv50_tgsi_insn(pc, tok);
			if (ret == FALSE)
				goto out_err;
			break;
		default:
			break;
		}
	}

	if (pc->p->type == PIPE_SHADER_FRAGMENT)
		nv50_fp_move_results(pc);

	nv50_program_fixup_insns(pc);

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

	assert(p->param_nr <= 512);

	if (p->param_nr) {
		unsigned cb;
		float *map = pipe_buffer_map(pscreen, nv50->constbuf[p->type],
					     PIPE_BUFFER_USAGE_CPU_READ);

		if (p->type == PIPE_SHADER_VERTEX)
			cb = NV50_CB_PVP;
		else
			cb = NV50_CB_PFP;

		nv50_program_upload_data(nv50, map, 0, p->param_nr, cb);
		pipe_buffer_unmap(pscreen, nv50->constbuf[p->type]);
	}
}

static void
nv50_program_validate_code(struct nv50_context *nv50, struct nv50_program *p)
{
	struct nouveau_channel *chan = nv50->screen->base.channel;
	struct nv50_program_exec *e;
	uint32_t *up, i;
	boolean upload = FALSE;

	if (!p->bo) {
		nouveau_bo_new(chan->device, NOUVEAU_BO_VRAM, 0x100,
			       p->exec_size * 4, &p->bo);
		upload = TRUE;
	}

	if (p->data[0] && p->data[0]->start != p->data_start[0])
		upload = TRUE;

	if (!upload)
		return;

	up = MALLOC(p->exec_size * 4);

	for (i = 0, e = p->exec_head; e; e = e->next) {
		unsigned ei, ci, bs;

		if (e->param.index >= 0 && e->param.mask) {
			bs = (e->inst[1] >> 22) & 0x07;
			assert(bs < 2);
			ei = e->param.shift >> 5;
			ci = e->param.index;
			if (bs == 0)
				ci += p->data[bs]->start;

			e->inst[ei] &= ~e->param.mask;
			e->inst[ei] |= (ci << e->param.shift);
		} else
		if (e->param.index >= 0) {
			/* zero mask means param is a jump/branch offset */
			assert(!(e->param.index & 1));
			/* seem to be 8 byte steps */
			ei = (e->param.index >> 1) + 0 /* START_ID */;

			e->inst[0] &= 0xf0000fff;
			e->inst[0] |= ei << 12;
		}

		up[i++] = e->inst[0];
		if (is_long(e))
			up[i++] = e->inst[1];
	}
	assert(i == p->exec_size);

	if (p->data[0])
		p->data_start[0] = p->data[0]->start;

#ifdef NV50_PROGRAM_DUMP
	NOUVEAU_ERR("-------\n");
	for (e = p->exec_head; e; e = e->next) {
		NOUVEAU_ERR("0x%08x\n", e->inst[0]);
		if (is_long(e))
			NOUVEAU_ERR("0x%08x\n", e->inst[1]);
	}
#endif
	nv50_upload_sifc(nv50, p->bo, 0, NOUVEAU_BO_VRAM,
			 NV50_2D_DST_FORMAT_R8_UNORM, 65536, 1, 262144,
			 up, NV50_2D_SIFC_FORMAT_R8_UNORM, 0,
			 0, 0, p->exec_size * 4, 1, 1);

	FREE(up);
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
	so_data  (so, p->cfg.attr[0]);
	so_data  (so, p->cfg.attr[1]);
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
	so_method(so, tesla, NV50TCL_FP_REG_ALLOC_TEMP, 1);
	so_data  (so, p->cfg.high_temp);
	so_method(so, tesla, NV50TCL_FP_RESULT_COUNT, 1);
	so_data  (so, p->cfg.high_result);
	so_method(so, tesla, NV50TCL_FP_CTRL_UNK19A8, 1);
	so_data  (so, p->cfg.regs[2]);
	so_method(so, tesla, NV50TCL_FP_CTRL_UNK196C, 1);
	so_data  (so, p->cfg.regs[3]);
	so_method(so, tesla, NV50TCL_FP_START_ID, 1);
	so_data  (so, 0); /* program start offset */
	so_ref(so, &nv50->state.fragprog);
	so_ref(NULL, &so);
}

static void
nv50_pntc_replace(struct nv50_context *nv50, uint32_t pntc[8], unsigned base)
{
	struct nv50_program *fp = nv50->fragprog;
	struct nv50_program *vp = nv50->vertprog;
	unsigned i, c, m = base;

	/* XXX: This can't work correctly in all cases yet, we either
	 * have to create TGSI_SEMANTIC_PNTC or sprite_coord_mode has
	 * to be per FP input instead of per VP output
	 */
	memset(pntc, 0, 8 * sizeof(uint32_t));

	for (i = 0; i < fp->cfg.io_nr; i++) {
		uint8_t sn, si;
		uint8_t j = fp->cfg.io[i].id_vp, k = fp->cfg.io[i].id_fp;
		unsigned n = popcnt4(fp->cfg.io[i].mask);

		if (fp->info.input_semantic_name[k] != TGSI_SEMANTIC_GENERIC) {
			m += n;
			continue;
		}

		sn = vp->info.input_semantic_name[j];
		si = vp->info.input_semantic_index[j];

		if (j < fp->cfg.io_nr && sn == TGSI_SEMANTIC_GENERIC) {
			ubyte mode =
				nv50->rasterizer->pipe.sprite_coord_mode[si];

			if (mode == PIPE_SPRITE_COORD_NONE) {
				m += n;
				continue;
			}
		}

		/* this is either PointCoord or replaced by sprite coords */
		for (c = 0; c < 4; c++) {
			if (!(fp->cfg.io[i].mask & (1 << c)))
				continue;
			pntc[m / 8] |= (c + 1) << ((m % 8) * 4);
			++m;
		}
	}
}

static int
nv50_sreg4_map(uint32_t *p_map, int mid, uint32_t lin[4],
	       struct nv50_sreg4 *fpi, struct nv50_sreg4 *vpo)
{
	int c;
	uint8_t mv = vpo->mask, mf = fpi->mask, oid = vpo->hw;
	uint8_t *map = (uint8_t *)p_map;

	for (c = 0; c < 4; ++c) {
		if (mf & 1) {
			if (fpi->linear == TRUE)
				lin[mid / 32] |= 1 << (mid % 32);
			map[mid++] = (mv & 1) ? oid : ((c == 3) ? 0x41 : 0x40);
		}

		oid += mv & 1;
		mf >>= 1;
		mv >>= 1;
	}

	return mid;
}

void
nv50_linkage_validate(struct nv50_context *nv50)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nv50_program *vp = nv50->vertprog;
	struct nv50_program *fp = nv50->fragprog;
	struct nouveau_stateobj *so;
	struct nv50_sreg4 dummy, *vpo;
	int i, n, c, m = 0;
	uint32_t map[16], lin[4], reg[5], pcrd[8];

	memset(map, 0, sizeof(map));
	memset(lin, 0, sizeof(lin));

	reg[1] = 0x00000004; /* low and high clip distance map ids */
	reg[2] = 0x00000000; /* layer index map id (disabled, GP only) */
	reg[3] = 0x00000000; /* point size map id & enable */
	reg[0] = fp->cfg.regs[0]; /* colour semantic reg */
	reg[4] = fp->cfg.regs[1]; /* interpolant info */

	dummy.linear = FALSE;
	dummy.mask = 0xf; /* map all components of HPOS */
	m = nv50_sreg4_map(map, m, lin, &dummy, &vp->cfg.io[0]);

	dummy.mask = 0x0;

	if (vp->cfg.clpd < 0x40) {
		for (c = 0; c < vp->cfg.clpd_nr; ++c)
			map[m++] = vp->cfg.clpd + c;
		reg[1] = (m << 8);
	}

	reg[0] |= m << 8; /* adjust BFC0 id */

	/* if light_twoside is active, it seems FFC0_ID == BFC0_ID is bad */
	if (nv50->rasterizer->pipe.light_twoside) {
		vpo = &vp->cfg.two_side[0];

		m = nv50_sreg4_map(map, m, lin, &fp->cfg.two_side[0], &vpo[0]);
		m = nv50_sreg4_map(map, m, lin, &fp->cfg.two_side[1], &vpo[1]);
	}

	reg[0] += m - 4; /* adjust FFC0 id */
	reg[4] |= m << 8; /* set mid where 'normal' FP inputs start */

	i = 0;
	if (fp->info.input_semantic_name[0] == TGSI_SEMANTIC_POSITION)
		i = 1;
	for (; i < fp->cfg.io_nr; i++) {
		ubyte sn = fp->info.input_semantic_name[fp->cfg.io[i].id_fp];
		ubyte si = fp->info.input_semantic_index[fp->cfg.io[i].id_fp];

		n = fp->cfg.io[i].id_vp;
		if (n >= vp->cfg.io_nr ||
		    vp->info.output_semantic_name[n] != sn ||
		    vp->info.output_semantic_index[n] != si)
			vpo = &dummy;
		else
			vpo = &vp->cfg.io[n];

		m = nv50_sreg4_map(map, m, lin, &fp->cfg.io[i], vpo);
	}

	if (nv50->rasterizer->pipe.point_size_per_vertex) {
		map[m / 4] |= vp->cfg.psiz << ((m % 4) * 8);
		reg[3] = (m++ << 4) | 1;
	}

	/* now fill the stateobj */
	so = so_new(64, 0);

	n = (m + 3) / 4;
	so_method(so, tesla, NV50TCL_VP_RESULT_MAP_SIZE, 1);
	so_data  (so, m);
	so_method(so, tesla, NV50TCL_VP_RESULT_MAP(0), n);
	so_datap (so, map, n);

	so_method(so, tesla, NV50TCL_MAP_SEMANTIC_0, 4);
	so_datap (so, reg, 4);

	so_method(so, tesla, NV50TCL_FP_INTERPOLANT_CTRL, 1);
	so_data  (so, reg[4]);

	so_method(so, tesla, 0x1540, 4);
	so_datap (so, lin, 4);

	if (nv50->rasterizer->pipe.point_sprite) {
		nv50_pntc_replace(nv50, pcrd, (reg[4] >> 8) & 0xff);

		so_method(so, tesla, NV50TCL_POINT_COORD_REPLACE_MAP(0), 8);
		so_datap (so, pcrd, 8);
	}

        so_ref(so, &nv50->state.programs);
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

	p->translated = 0;
}
