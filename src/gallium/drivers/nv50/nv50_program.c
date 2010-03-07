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
#include "util/u_inlines.h"

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

	int vtx; /* vertex index, for GP inputs (TGSI Dimension.Index) */
	int indirect[2]; /* index into pc->addr, or -1 */

	ubyte buf_index; /* c{0 .. 15}[] or g{0 .. 15}[] */
};

#define NV50_MOD_NEG 1
#define NV50_MOD_ABS 2
#define NV50_MOD_NEG_ABS (NV50_MOD_NEG | NV50_MOD_ABS)
#define NV50_MOD_SAT 4
#define NV50_MOD_I32 8

/* NV50_MOD_I32 is used to indicate integer mode for neg/abs */

/* STACK: Conditionals and loops have to use the (per warp) stack.
 * Stack entries consist of an entry type (divergent path, join at),
 * a mask indicating the active threads of the warp, and an address.
 * MPs can store 12 stack entries internally, if we need more (and
 * we probably do), we have to create a stack buffer in VRAM.
 */
/* impose low limits for now */
#define NV50_MAX_COND_NESTING 4
#define NV50_MAX_LOOP_NESTING 3

#define JOIN_ON(e) e; pc->p->exec_tail->inst[1] |= 2

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
	uint32_t *immd_buf;
	int immd_nr;
	struct nv50_reg **addr;
	int addr_nr;
	struct nv50_reg *sysval;
	int sysval_nr;

	struct nv50_reg *temp_temp[16];
	struct nv50_program_exec *temp_temp_exec[16];
	unsigned temp_temp_nr;

	/* broadcast and destination replacement regs */
	struct nv50_reg *r_brdc;
	struct nv50_reg *r_dst[4];

	struct nv50_reg reg_instances[16];
	unsigned reg_instance_nr;

	unsigned interp_mode[32];
	/* perspective interpolation registers */
	struct nv50_reg *iv_p;
	struct nv50_reg *iv_c;

	struct nv50_program_exec *if_insn[NV50_MAX_COND_NESTING];
	struct nv50_program_exec *if_join[NV50_MAX_COND_NESTING];
	struct nv50_program_exec *loop_brka[NV50_MAX_LOOP_NESTING];
	int if_lvl, loop_lvl;
	unsigned loop_pos[NV50_MAX_LOOP_NESTING];

	unsigned *insn_pos; /* actual program offset of each TGSI insn */
	boolean in_subroutine;

	/* current instruction and total number of insns */
	unsigned insn_cur;
	unsigned insn_nr;

	boolean allow32;

	uint8_t edgeflag_out;
};

static struct nv50_reg *get_address_reg(struct nv50_pc *, struct nv50_reg *);

static INLINE void
ctor_reg(struct nv50_reg *reg, unsigned type, int index, int hw)
{
	reg->type = type;
	reg->index = index;
	reg->hw = hw;
	reg->mod = 0;
	reg->rhw = -1;
	reg->vtx = -1;
	reg->acc = 0;
	reg->indirect[0] = reg->indirect[1] = -1;
	reg->buf_index = (type == P_CONST) ? 1 : 0;
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
			pc->r_addr[i].acc = 0;
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

	NOUVEAU_ERR("out of registers\n");
	abort();
}

static INLINE struct nv50_reg *
reg_instance(struct nv50_pc *pc, struct nv50_reg *reg)
{
	struct nv50_reg *ri;

	assert(pc->reg_instance_nr < 16);
	ri = &pc->reg_instances[pc->reg_instance_nr++];
	if (reg) {
		alloc_reg(pc, reg);
		*ri = *reg;
		reg->indirect[0] = reg->indirect[1] = -1;
		reg->mod = 0;
	}
	return ri;
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

	NOUVEAU_ERR("out of registers\n");
	abort();
	return NULL;
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
temp_temp(struct nv50_pc *pc, struct nv50_program_exec *e)
{
	if (pc->temp_temp_nr >= 16)
		assert(0);

	pc->temp_temp[pc->temp_temp_nr] = alloc_temp(pc, NULL);
	pc->temp_temp_exec[pc->temp_temp_nr] = e;
	return pc->temp_temp[pc->temp_temp_nr++];
}

/* This *must* be called for all nv50_program_exec that have been
 * given as argument to temp_temp, or the temps will be leaked !
 */
static void
kill_temp_temp(struct nv50_pc *pc, struct nv50_program_exec *e)
{
	int i;

	for (i = 0; i < pc->temp_temp_nr; i++)
		if (pc->temp_temp_exec[i] == e)
			free_temp(pc, pc->temp_temp[i]);
	if (!e)
		pc->temp_temp_nr = 0;
}

static int
ctor_immd_4u32(struct nv50_pc *pc,
	       uint32_t x, uint32_t y, uint32_t z, uint32_t w)
{
	unsigned size = pc->immd_nr * 4 * sizeof(uint32_t);

	pc->immd_buf = REALLOC(pc->immd_buf, size, size + 4 * sizeof(uint32_t));

	pc->immd_buf[(pc->immd_nr * 4) + 0] = x;
	pc->immd_buf[(pc->immd_nr * 4) + 1] = y;
	pc->immd_buf[(pc->immd_nr * 4) + 2] = z;
	pc->immd_buf[(pc->immd_nr * 4) + 3] = w;

	return pc->immd_nr++;
}

static INLINE int
ctor_immd_4f32(struct nv50_pc *pc, float x, float y, float z, float w)
{
	return ctor_immd_4u32(pc, fui(x), fui(y), fui(z), fui(w));
}

static struct nv50_reg *
alloc_immd(struct nv50_pc *pc, float f)
{
	struct nv50_reg *r = MALLOC_STRUCT(nv50_reg);
	unsigned hw;

	for (hw = 0; hw < pc->immd_nr * 4; hw++)
		if (pc->immd_buf[hw] == fui(f))
			break;

	if (hw == pc->immd_nr * 4)
		hw = ctor_immd_4f32(pc, f, -f, 0.5 * f, 0) * 4;

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

	kill_temp_temp(pc, e);
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

static boolean
is_join(struct nv50_program_exec *e)
{
	if (is_long(e) && (e->inst[1] & 3) == 2)
		return TRUE;
	return FALSE;
}

static INLINE boolean
is_control_flow(struct nv50_program_exec *e)
{
	return (e->inst[0] & 2);
}

static INLINE void
set_pred(struct nv50_pc *pc, unsigned pred, unsigned idx,
	 struct nv50_program_exec *e)
{
	assert(!is_immd(e));
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
	set_long(pc, e);
	/* XXX: can't be predicated - bits overlap; cases where both
	 * are required should be avoided by using pc->allow32 */
	set_pred(pc, 0, 0, e);
	set_pred_wr(pc, 0, 0, e);

	e->inst[1] |= 0x00000002 | 0x00000001;
	e->inst[0] |= (pc->immd_buf[imm->hw] & 0x3f) << 16;
	e->inst[1] |= (pc->immd_buf[imm->hw] >> 6) << 2;
}

static INLINE void
set_addr(struct nv50_program_exec *e, struct nv50_reg *a)
{
	assert(a->type == P_ADDR);

	assert(!(e->inst[0] & 0x0c000000));
	assert(!(e->inst[1] & 0x00000004));

	e->inst[0] |= (a->hw & 3) << 26;
	e->inst[1] |= a->hw & 4;
}

static void
emit_arl(struct nv50_pc *, struct nv50_reg *, struct nv50_reg *, uint8_t);

static void
emit_shl_imm(struct nv50_pc *, struct nv50_reg *, struct nv50_reg *, int);

static void
emit_mov_from_addr(struct nv50_pc *pc, struct nv50_reg *dst,
		   struct nv50_reg *src)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[1] = 0x40000000;
	set_long(pc, e);
	set_dst(pc, dst, e);
	set_addr(e, src);

	emit(pc, e);
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

	if (src->hw < 0 || src->hw > 127) /* need (additional) address reg */
		set_addr(e, get_address_reg(pc, src));
	else
	if (src->acc < 0) {
		assert(src->type == P_CONST);
		set_addr(e, pc->addr[src->indirect[0]]);
	}

	e->inst[1] |= (src->buf_index << 22);
}

/* Never apply nv50_reg::mod in emit_mov, or carefully check the code !!! */
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
		e->inst[1] |= 0x20000000; /* mov from c[] */
	} else {
		if (src->type == P_ATTR) {
			set_long(pc, e);
			e->inst[1] |= 0x00200000;

			if (src->vtx >= 0) {
				/* indirect (vertex base + c) load from p[] */
				e->inst[0] |= 0x01800000;
				set_addr(e, get_address_reg(pc, src));
			}
		}

		alloc_reg(pc, src);
		if (src->hw > 63)
			set_long(pc, e);
		e->inst[0] |= (src->hw << 9);
	}

	if (is_long(e) && !is_immd(e)) {
		e->inst[1] |= 0x04000000; /* 32-bit */
		e->inst[1] |= 0x0000c000; /* 32-bit c[] load / lane mask 0:1 */
		if (!(e->inst[1] & 0x20000000))
			e->inst[1] |= 0x00030000; /* lane mask 2:3 */
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

/* Assign the hw of the discarded temporary register src
 * to the tgsi register dst and free src.
 */
static void
assimilate_temp(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src)
{
	assert(src->index == -1 && src->hw != -1);

	if (pc->if_lvl || pc->loop_lvl ||
	    (dst->type != P_TEMP) ||
	    (src->hw < pc->result_nr * 4 &&
	     pc->p->type == PIPE_SHADER_FRAGMENT) ||
	    pc->p->info.opcode_count[TGSI_OPCODE_CAL] ||
	    pc->p->info.opcode_count[TGSI_OPCODE_BRA]) {

		emit_mov(pc, dst, src);
		free_temp(pc, src);
		return;
	}

	if (dst->hw != -1)
		pc->r_temp[dst->hw] = NULL;
	pc->r_temp[src->hw] = dst;
	dst->hw = src->hw;

	FREE(src);
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
		temp = temp_temp(pc, e);
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

		if (src->vtx >= 0) {
			e->inst[0] |= 0x01800000; /* src from p[] */
			set_addr(e, get_address_reg(pc, src));
		}
	} else
	if (src->type == P_CONST || src->type == P_IMMD) {
		struct nv50_reg *temp = temp_temp(pc, e);

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
		struct nv50_reg *temp = temp_temp(pc, e);

		emit_mov(pc, temp, src);
		src = temp;
	} else
	if (src->type == P_CONST || src->type == P_IMMD) {
		if (e->inst[0] & 0x01800000) {
			struct nv50_reg *temp = temp_temp(pc, e);

			emit_mov(pc, temp, src);
			src = temp;
		} else {
			assert(!(e->inst[0] & 0x00800000));
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
		struct nv50_reg *temp = temp_temp(pc, e);

		emit_mov(pc, temp, src);
		src = temp;
	} else
	if (src->type == P_CONST || src->type == P_IMMD) {
		if (e->inst[0] & 0x01800000) {
			struct nv50_reg *temp = temp_temp(pc, e);

			emit_mov(pc, temp, src);
			src = temp;
		} else {
			assert(!(e->inst[0] & 0x01000000));
			set_data(pc, src, 0x7f, 32+14, e);
			e->inst[0] |= 0x01000000;
		}
	}

	alloc_reg(pc, src);
	e->inst[1] |= ((src->hw & 127) << 14);
}

static void
set_half_src(struct nv50_pc *pc, struct nv50_reg *src, int lh,
	     struct nv50_program_exec *e, int pos)
{
	struct nv50_reg *r = src;

	alloc_reg(pc, r);
	if (r->type != P_TEMP) {
		r = temp_temp(pc, e);
		emit_mov(pc, r, src);
	}

	if (r->hw > (NV50_SU_MAX_TEMP / 2)) {
		NOUVEAU_ERR("out of low GPRs\n");
		abort();
	}

	e->inst[pos / 32] |= ((src->hw * 2) + lh) << (pos % 32);
}

static void
emit_mov_from_pred(struct nv50_pc *pc, struct nv50_reg *dst, int pred)
{
	struct nv50_program_exec *e = exec(pc);

	assert(dst->type == P_TEMP);
	e->inst[1] = 0x20000000 | (pred << 12);
	set_long(pc, e);
	set_dst(pc, dst, e);

	emit(pc, e);
}

static void
emit_mov_to_pred(struct nv50_pc *pc, int pred, struct nv50_reg *src)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] = 0x000001fc;
	e->inst[1] = 0xa0000008;
	set_long(pc, e);
	set_pred_wr(pc, 1, pred, e);
	set_src_0_restricted(pc, src, e);

	emit(pc, e);
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
		if (src0->mod ^ src1->mod)
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
	set_src_0(pc, src, e);

	emit(pc, e);
}

static boolean
address_reg_suitable(struct nv50_reg *a, struct nv50_reg *r)
{
	if (!r)
		return FALSE;

	if (r->vtx != a->vtx)
		return FALSE;
	if (r->vtx >= 0)
		return (r->indirect[1] == a->indirect[1]);

	if (r->hw < a->rhw || (r->hw - a->rhw) >= 128)
		return FALSE;

	if (a->index >= 0)
		return (a->index == r->indirect[0]);
	return (a->indirect[0] == r->indirect[0]);
}

static void
load_vertex_base(struct nv50_pc *pc, struct nv50_reg *dst,
		 struct nv50_reg *a, int shift)
{
	struct nv50_reg mem, *temp;

	ctor_reg(&mem, P_ATTR, -1, dst->vtx);

	assert(dst->type == P_ADDR);
	if (!a) {
		emit_arl(pc, dst, &mem, 0);
		return;
	}
	temp = alloc_temp(pc, NULL);

	if (shift) {
		emit_mov_from_addr(pc, temp, a);
		if (shift < 0)
			emit_shl_imm(pc, temp, temp, shift);
		emit_arl(pc, dst, temp, MAX2(shift, 0));
	}
	emit_mov(pc, temp, &mem);
	set_addr(pc->p->exec_tail, dst);

	emit_arl(pc, dst, temp, 0);
	free_temp(pc, temp);
}

/* case (ref == NULL): allocate address register for TGSI_FILE_ADDRESS
 * case (vtx >= 0, acc >= 0): load vertex base from a[vtx * 4] to $aX
 * case (vtx >= 0, acc < 0): load vertex base from s[$aY + vtx * 4] to $aX
 * case (vtx < 0, acc >= 0): memory address too high to encode
 * case (vtx < 0, acc < 0): get source register for TGSI_FILE_ADDRESS
 */
static struct nv50_reg *
get_address_reg(struct nv50_pc *pc, struct nv50_reg *ref)
{
	int i;
	struct nv50_reg *a_ref, *a = NULL;

	for (i = 0; i < NV50_SU_MAX_ADDR; ++i) {
		if (pc->r_addr[i].acc == 0)
			a = &pc->r_addr[i]; /* an unused address reg */
		else
		if (address_reg_suitable(&pc->r_addr[i], ref)) {
			pc->r_addr[i].acc = pc->insn_cur;
			return &pc->r_addr[i];
		} else
		if (!a && pc->r_addr[i].index < 0 &&
		    pc->r_addr[i].acc < pc->insn_cur)
			a = &pc->r_addr[i];
	}
	if (!a) {
		/* We'll be able to spill address regs when this
		 * mess is replaced with a proper compiler ...
		 */
		NOUVEAU_ERR("out of address regs\n");
		abort();
		return NULL;
	}

	/* initialize and reserve for this TGSI instruction */
	a->rhw = 0;
	a->index = a->indirect[0] = a->indirect[1] = -1;
	a->acc = pc->insn_cur;

	if (!ref) {
		a->vtx = -1;
		return a;
	}
	a->vtx = ref->vtx;

	/* now put in the correct value ... */

	if (ref->vtx >= 0) {
		a->indirect[1] = ref->indirect[1];

		/* For an indirect vertex index, we need to shift address right
		 * by 2, the address register will contain vtx * 16, we need to
		 * load from a[vtx * 4].
		 */
		load_vertex_base(pc, a, (ref->acc < 0) ?
				 pc->addr[ref->indirect[1]] : NULL, -2);
	} else {
		assert(ref->acc < 0 || ref->indirect[0] < 0);

		a->rhw = ref->hw & ~0x7f;
		a->indirect[0] = ref->indirect[0];
		a_ref = (ref->acc < 0) ? pc->addr[ref->indirect[0]] : NULL;

		emit_add_addr_imm(pc, a, a_ref, a->rhw * 4);
	}
	return a;
}

#define NV50_MAX_F32 0x880
#define NV50_MAX_S32 0x08c
#define NV50_MAX_U32 0x084
#define NV50_MIN_F32 0x8a0
#define NV50_MIN_S32 0x0ac
#define NV50_MIN_U32 0x0a4

static void
emit_minmax(struct nv50_pc *pc, unsigned sub, struct nv50_reg *dst,
	    struct nv50_reg *src0, struct nv50_reg *src1)
{
	struct nv50_program_exec *e = exec(pc);

	set_long(pc, e);
	e->inst[0] |= 0x30000000 | ((sub & 0x800) << 20);
	e->inst[1] |= (sub << 24);

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

	assert(!(src0->mod | src1->mod));

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
emit_not(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] = 0xd0000000;
	e->inst[1] = 0x0402c000;
	set_long(pc, e);
	set_dst(pc, dst, e);
	set_src_1(pc, src, e);

	emit(pc, e);
}

static void
emit_shift(struct nv50_pc *pc, struct nv50_reg *dst,
	   struct nv50_reg *src0, struct nv50_reg *src1, unsigned dir)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] = 0x30000000;
	e->inst[1] = 0xc4000000;

	set_long(pc, e);
	set_dst(pc, dst, e);
	set_src_0(pc, src0, e);

	if (src1->type == P_IMMD) {
		e->inst[1] |= (1 << 20);
		e->inst[0] |= (pc->immd_buf[src1->hw] & 0x7f) << 16;
	} else
		set_src_1(pc, src1, e);

	if (dir != TGSI_OPCODE_SHL)
		e->inst[1] |= (1 << 29);

	if (dir == TGSI_OPCODE_ISHR)
		e->inst[1] |= (1 << 27);

	emit(pc, e);
}

static void
emit_shl_imm(struct nv50_pc *pc, struct nv50_reg *dst,
	     struct nv50_reg *src, int s)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] = 0x30000000;
	e->inst[1] = 0xc4100000;
	if (s < 0) {
		e->inst[1] |= 1 << 29;
		s = -s;
	}
	e->inst[1] |= ((s & 0x7f) << 16);

	set_long(pc, e);
	set_dst(pc, dst, e);
	set_src_0(pc, src, e);

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
	src2->mod ^= NV50_MOD_NEG;
	emit_mad(pc, dst, src0, src1, src2);
	src2->mod ^= NV50_MOD_NEG;
}

#define NV50_FLOP_RCP 0
#define NV50_FLOP_RSQ 2
#define NV50_FLOP_LG2 3
#define NV50_FLOP_SIN 4
#define NV50_FLOP_COS 5
#define NV50_FLOP_EX2 6

/* rcp, rsqrt, lg2 support neg and abs */
static void
emit_flop(struct nv50_pc *pc, unsigned sub,
	  struct nv50_reg *dst, struct nv50_reg *src)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] |= 0x90000000;
	if (sub || src->mod) {
		set_long(pc, e);
		e->inst[1] |= (sub << 29);
	}

	set_dst(pc, dst, e);
	set_src_0_restricted(pc, src, e);

	assert(!src->mod || sub < 4);

	if (src->mod & NV50_MOD_NEG)
		e->inst[1] |= 0x04000000;
	if (src->mod & NV50_MOD_ABS)
		e->inst[1] |= 0x00100000;

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

	if (src->mod & NV50_MOD_NEG)
		e->inst[1] |= 0x04000000;
	if (src->mod & NV50_MOD_ABS)
		e->inst[1] |= 0x00100000;

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

	if (src->mod & NV50_MOD_NEG)
		e->inst[1] |= 0x04000000;
	if (src->mod & NV50_MOD_ABS)
		e->inst[1] |= 0x00100000;

	emit(pc, e);
}

#define CVT_RN    (0x00 << 16)
#define CVT_FLOOR (0x02 << 16)
#define CVT_CEIL  (0x04 << 16)
#define CVT_TRUNC (0x06 << 16)
#define CVT_SAT   (0x08 << 16)
#define CVT_ABS   (0x10 << 16)

#define CVT_X32_X32 0x04004000
#define CVT_X32_S32 0x04014000
#define CVT_F32_F32 ((0xc0 << 24) | CVT_X32_X32)
#define CVT_S32_F32 ((0x88 << 24) | CVT_X32_X32)
#define CVT_U32_F32 ((0x80 << 24) | CVT_X32_X32)
#define CVT_F32_S32 ((0x40 << 24) | CVT_X32_S32)
#define CVT_F32_U32 ((0x40 << 24) | CVT_X32_X32)
#define CVT_S32_S32 ((0x08 << 24) | CVT_X32_S32)
#define CVT_S32_U32 ((0x08 << 24) | CVT_X32_X32)
#define CVT_U32_S32 ((0x00 << 24) | CVT_X32_S32)

#define CVT_NEG 0x20000000
#define CVT_RI  0x08000000

static void
emit_cvt(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src,
	 int wp, uint32_t cvn)
{
	struct nv50_program_exec *e;

	e = exec(pc);

	if (src->mod & NV50_MOD_NEG) cvn |= CVT_NEG;
	if (src->mod & NV50_MOD_ABS) cvn |= CVT_ABS;

	e->inst[0] = 0xa0000000;
	e->inst[1] = cvn;
	set_long(pc, e);
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
 *
 *  mode = 0x04 (u32), 0x0c (s32), 0x80 (f32)
 */
static void
emit_set(struct nv50_pc *pc, unsigned ccode, struct nv50_reg *dst, int wp,
	 struct nv50_reg *src0, struct nv50_reg *src1, uint8_t mode)
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

	set_long(pc, e);
	e->inst[0] |= 0x30000000 | (mode << 24);
	e->inst[1] |= 0x60000000 | (ccode << 14);

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

	if (rdst && mode == 0x80) /* convert to float ? */
		emit_cvt(pc, rdst, dst, -1, CVT_ABS | CVT_F32_S32);
	if (rdst && rdst != dst)
		free_temp(pc, dst);
}

static INLINE void
map_tgsi_setop_hw(unsigned op, uint8_t *cc, uint8_t *ty)
{
	switch (op) {
	case TGSI_OPCODE_SLT: *cc = 0x1; *ty = 0x80; break;
	case TGSI_OPCODE_SGE: *cc = 0x6; *ty = 0x80; break;
	case TGSI_OPCODE_SEQ: *cc = 0x2; *ty = 0x80; break;
	case TGSI_OPCODE_SGT: *cc = 0x4; *ty = 0x80; break;
	case TGSI_OPCODE_SLE: *cc = 0x3; *ty = 0x80; break;
	case TGSI_OPCODE_SNE: *cc = 0xd; *ty = 0x80; break;

	case TGSI_OPCODE_ISLT: *cc = 0x1; *ty = 0x0c; break;
	case TGSI_OPCODE_ISGE: *cc = 0x6; *ty = 0x0c; break;
	case TGSI_OPCODE_USEQ: *cc = 0x2; *ty = 0x04; break;
	case TGSI_OPCODE_USGE: *cc = 0x6; *ty = 0x04; break;
	case TGSI_OPCODE_USLT: *cc = 0x1; *ty = 0x04; break;
	case TGSI_OPCODE_USNE: *cc = 0x5; *ty = 0x04; break;
	default:
		assert(0);
		return;
	}
}

static void
emit_add_b32(struct nv50_pc *pc, struct nv50_reg *dst,
	     struct nv50_reg *src0, struct nv50_reg *rsrc1)
{
	struct nv50_program_exec *e = exec(pc);
	struct nv50_reg *src1;

	e->inst[0] = 0x20000000;

	alloc_reg(pc, rsrc1);
	check_swap_src_0_1(pc, &src0, &rsrc1);

	src1 = rsrc1;
	if (src0->mod & rsrc1->mod & NV50_MOD_NEG) {
		src1 = temp_temp(pc, e);
		emit_cvt(pc, src1, rsrc1, -1, CVT_S32_S32);
	}

	if (!pc->allow32 || src1->hw > 63 ||
	    (src1->type != P_TEMP && src1->type != P_IMMD))
		set_long(pc, e);

	set_dst(pc, dst, e);
	set_src_0(pc, src0, e);

	if (is_long(e)) {
		e->inst[1] |= 1 << 26;
		set_src_2(pc, src1, e);
	} else {
		e->inst[0] |= 0x8000;
		if (src1->type == P_IMMD)
			set_immd(pc, src1, e);
		else
			set_src_1(pc, src1, e);
	}

	if (src0->mod & NV50_MOD_NEG)
		e->inst[0] |= 1 << 28;
	else
	if (src1->mod & NV50_MOD_NEG)
		e->inst[0] |= 1 << 22;

	emit(pc, e);
}

static void
emit_mad_u16(struct nv50_pc *pc, struct nv50_reg *dst,
	     struct nv50_reg *src0, int lh_0, struct nv50_reg *src1, int lh_1,
	     struct nv50_reg *src2)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] = 0x60000000;
	if (!pc->allow32)
		set_long(pc, e);
	set_dst(pc, dst, e);

	set_half_src(pc, src0, lh_0, e, 9);
	set_half_src(pc, src1, lh_1, e, 16);
	alloc_reg(pc, src2);
	if (is_long(e) || (src2->type != P_TEMP) || (src2->hw != dst->hw))
		set_src_2(pc, src2, e);

	emit(pc, e);
}

static void
emit_mul_u16(struct nv50_pc *pc, struct nv50_reg *dst,
	     struct nv50_reg *src0, int lh_0, struct nv50_reg *src1, int lh_1)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] = 0x40000000;
	set_long(pc, e);
	set_dst(pc, dst, e);

	set_half_src(pc, src0, lh_0, e, 9);
	set_half_src(pc, src1, lh_1, e, 16);

	emit(pc, e);
}

static void
emit_sad(struct nv50_pc *pc, struct nv50_reg *dst,
	 struct nv50_reg *src0, struct nv50_reg *src1, struct nv50_reg *src2)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] = 0x50000000;
	if (!pc->allow32)
		set_long(pc, e);
	check_swap_src_0_1(pc, &src0, &src1);
	set_dst(pc, dst, e);
	set_src_0(pc, src0, e);
	set_src_1(pc, src1, e);
	alloc_reg(pc, src2);
	if (is_long(e) || (src2->type != dst->type) || (src2->hw != dst->hw))
		set_src_2(pc, src2, e);

	if (is_long(e))
		e->inst[1] |= 0x0c << 24;
	else
		e->inst[0] |= 0x81 << 8;

	emit(pc, e);
}

static INLINE void
emit_flr(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src)
{
	emit_cvt(pc, dst, src, -1, CVT_FLOOR | CVT_F32_F32 | CVT_RI);
}

static void
emit_pow(struct nv50_pc *pc, struct nv50_reg *dst,
	 struct nv50_reg *v, struct nv50_reg *e)
{
	struct nv50_reg *temp = alloc_temp(pc, NULL);

	emit_flop(pc, NV50_FLOP_LG2, temp, v);
	emit_mul(pc, temp, temp, e);
	emit_preex2(pc, temp, temp);
	emit_flop(pc, NV50_FLOP_EX2, dst, temp);

	free_temp(pc, temp);
}

static INLINE void
emit_sat(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src)
{
	emit_cvt(pc, dst, src, -1, CVT_SAT | CVT_F32_F32);
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
		emit_minmax(pc, NV50_MAX_F32, tmp[0], src[0], zero);
	}

	if (mask & (1 << 2)) {
		set_pred_wr(pc, 1, 0, pc->p->exec_tail);

		tmp[1] = temp_temp(pc, NULL);
		emit_minmax(pc, NV50_MAX_F32, tmp[1], src[1], zero);

		tmp[3] = temp_temp(pc, NULL);
		emit_minmax(pc, NV50_MAX_F32, tmp[3], src[3], neg128);
		emit_minmax(pc, NV50_MIN_F32, tmp[3], tmp[3], pos128);

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
emit_kil(struct nv50_pc *pc, struct nv50_reg *src)
{
	struct nv50_program_exec *e;
	const int r_pred = 1;

	e = exec(pc);
	e->inst[0] = 0x00000002; /* discard */
	set_long(pc, e); /* sets cond code to ALWAYS */

	if (src) {
		set_pred(pc, 0x1 /* cc = LT */, r_pred, e);
		/* write to predicate reg */
		emit_cvt(pc, NULL, src, r_pred, CVT_F32_F32);
	}

	emit(pc, e);
}

static struct nv50_program_exec *
emit_control_flow(struct nv50_pc *pc, unsigned op, int pred, unsigned cc)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] = (op << 28) | 2;
	set_long(pc, e);
	if (pred >= 0)
		set_pred(pc, cc, pred, e);

	emit(pc, e);
	return e;
}

static INLINE struct nv50_program_exec *
emit_breakaddr(struct nv50_pc *pc)
{
	return emit_control_flow(pc, 0x4, -1, 0);
}

static INLINE void
emit_break(struct nv50_pc *pc, int pred, unsigned cc)
{
	emit_control_flow(pc, 0x5, pred, cc);
}

static INLINE struct nv50_program_exec *
emit_joinat(struct nv50_pc *pc)
{
	return emit_control_flow(pc, 0xa, -1, 0);
}

static INLINE struct nv50_program_exec *
emit_branch(struct nv50_pc *pc, int pred, unsigned cc)
{
	return emit_control_flow(pc, 0x1, pred, cc);
}

static INLINE struct nv50_program_exec *
emit_call(struct nv50_pc *pc, int pred, unsigned cc)
{
	return emit_control_flow(pc, 0x2, pred, cc);
}

static INLINE void
emit_ret(struct nv50_pc *pc, int pred, unsigned cc)
{
	emit_control_flow(pc, 0x3, pred, cc);
}

static void
emit_prim_cmd(struct nv50_pc *pc, unsigned cmd)
{
	struct nv50_program_exec *e = exec(pc);

	e->inst[0] = 0xf0000000 | (cmd << 9);
	e->inst[1] = 0xc0000000;
	set_long(pc, e);

	emit(pc, e);
}

#define QOP_ADD 0
#define QOP_SUBR 1
#define QOP_SUB 2
#define QOP_MOV_SRC1 3

/* For a quad of threads / top left, top right, bottom left, bottom right
 * pixels, do a different operation, and take src0 from a specific thread.
 */
static void
emit_quadop(struct nv50_pc *pc, struct nv50_reg *dst, int wp, int lane_src0,
	    struct nv50_reg *src0, struct nv50_reg *src1, ubyte qop)
{
       struct nv50_program_exec *e = exec(pc);

       e->inst[0] = 0xc0000000;
       e->inst[1] = 0x80000000;
       set_long(pc, e);
       e->inst[0] |= lane_src0 << 16;
       set_src_0(pc, src0, e);
       set_src_2(pc, src1, e);

       if (wp >= 0)
	       set_pred_wr(pc, 1, wp, e);

       if (dst)
	       set_dst(pc, dst, e);
       else {
	       e->inst[0] |= 0x000001fc;
	       e->inst[1] |= 0x00000008;
       }

       e->inst[0] |= (qop & 3) << 20;
       e->inst[1] |= (qop >> 2) << 22;

       emit(pc, e);
}

static void
load_cube_tex_coords(struct nv50_pc *pc, struct nv50_reg *t[4],
		     struct nv50_reg **src, unsigned arg, boolean proj)
{
	int mod[3] = { src[0]->mod, src[1]->mod, src[2]->mod };

	src[0]->mod |= NV50_MOD_ABS;
	src[1]->mod |= NV50_MOD_ABS;
	src[2]->mod |= NV50_MOD_ABS;

	emit_minmax(pc, NV50_MAX_F32, t[2], src[0], src[1]);
	emit_minmax(pc, NV50_MAX_F32, t[2], src[2], t[2]);

	src[0]->mod = mod[0];
	src[1]->mod = mod[1];
	src[2]->mod = mod[2];

	if (proj && 0 /* looks more correct without this */)
		emit_mul(pc, t[2], t[2], src[3]);
	else
	if (arg == 4) /* there is no textureProj(samplerCubeShadow) */
		emit_mov(pc, t[3], src[3]);

	emit_flop(pc, NV50_FLOP_RCP, t[2], t[2]);

	emit_mul(pc, t[0], src[0], t[2]);
	emit_mul(pc, t[1], src[1], t[2]);
	emit_mul(pc, t[2], src[2], t[2]);
}

static void
load_proj_tex_coords(struct nv50_pc *pc, struct nv50_reg *t[4],
		     struct nv50_reg **src, unsigned dim, unsigned arg)
{
	unsigned c, mode;

	if (src[0]->type == P_TEMP && src[0]->rhw != -1) {
		mode = pc->interp_mode[src[0]->index] | INTERP_PERSPECTIVE;

		t[3]->rhw = src[3]->rhw;
		emit_interp(pc, t[3], NULL, (mode & INTERP_CENTROID));
		emit_flop(pc, NV50_FLOP_RCP, t[3], t[3]);

		for (c = 0; c < dim; ++c) {
			t[c]->rhw = src[c]->rhw;
			emit_interp(pc, t[c], t[3], mode);
		}
		if (arg != dim) { /* depth reference value */
			t[dim]->rhw = src[2]->rhw;
			emit_interp(pc, t[dim], t[3], mode);
		}
	} else {
		/* XXX: for some reason the blob sometimes uses MAD
		 * (mad f32 $rX $rY $rZ neg $r63)
		 */
		emit_flop(pc, NV50_FLOP_RCP, t[3], src[3]);
		for (c = 0; c < dim; ++c)
			emit_mul(pc, t[c], src[c], t[3]);
		if (arg != dim) /* depth reference value */
			emit_mul(pc, t[dim], src[2], t[3]);
	}
}

static INLINE void
get_tex_dim(unsigned type, unsigned *dim, unsigned *arg)
{
	switch (type) {
	case TGSI_TEXTURE_1D:
		*arg = *dim = 1;
		break;
	case TGSI_TEXTURE_SHADOW1D:
		*dim = 1;
		*arg = 2;
		break;
	case TGSI_TEXTURE_UNKNOWN:
	case TGSI_TEXTURE_2D:
	case TGSI_TEXTURE_RECT:
		*arg = *dim = 2;
		break;
	case TGSI_TEXTURE_SHADOW2D:
	case TGSI_TEXTURE_SHADOWRECT:
		*dim = 2;
		*arg = 3;
		break;
	case TGSI_TEXTURE_3D:
	case TGSI_TEXTURE_CUBE:
		*dim = *arg = 3;
		break;
	default:
		assert(0);
		break;
	}
}

/* We shouldn't execute TEXLOD if any of the pixels in a quad have
 * different LOD values, so branch off groups of equal LOD.
 */
static void
emit_texlod_sequence(struct nv50_pc *pc, struct nv50_reg *tlod,
		     struct nv50_reg *src, struct nv50_program_exec *tex)
{
	struct nv50_program_exec *join_at;
	unsigned i, target = pc->p->exec_size + 9 * 2;

	if (pc->p->type != PIPE_SHADER_FRAGMENT) {
		emit(pc, tex);
		return;
	}
	pc->allow32 = FALSE;

	/* Subtract lod of each pixel from lod of top left pixel, jump
	 * texlod insn if result is 0, then repeat for 2 other pixels.
	 */
	join_at = emit_joinat(pc);
	emit_quadop(pc, NULL, 0, 0, tlod, tlod, 0x55);
	emit_branch(pc, 0, 2)->param.index = target;

	for (i = 1; i < 4; ++i) {
		emit_quadop(pc, NULL, 0, i, tlod, tlod, 0x55);
		emit_branch(pc, 0, 2)->param.index = target;
	}

	emit_mov(pc, tlod, src); /* target */
	emit(pc, tex); /* texlod */

	join_at->param.index = target + 2 * 2;
	JOIN_ON(emit_nop(pc)); /* join _after_ tex */
}

static void
emit_texbias_sequence(struct nv50_pc *pc, struct nv50_reg *t[4], unsigned arg,
		      struct nv50_program_exec *tex)
{
	struct nv50_program_exec *e;
	struct nv50_reg imm_1248, *t123[4][4], *r_bits = alloc_temp(pc, NULL);
	int r_pred = 0;
	unsigned n, c, i, cc[4] = { 0x0a, 0x13, 0x11, 0x10 };

	pc->allow32 = FALSE;
	ctor_reg(&imm_1248, P_IMMD, -1, ctor_immd_4u32(pc, 1, 2, 4, 8) * 4);

	/* Subtract bias value of thread i from bias values of each thread,
	 * store result in r_pred, and set bit i in r_bits if result was 0.
	 */
	assert(arg < 4);
	for (i = 0; i < 4; ++i, ++imm_1248.hw) {
		emit_quadop(pc, NULL, r_pred, i, t[arg], t[arg], 0x55);
		emit_mov(pc, r_bits, &imm_1248);
		set_pred(pc, 2, r_pred, pc->p->exec_tail);
	}
	emit_mov_to_pred(pc, r_pred, r_bits);

	/* The lanes of a quad are now grouped by the bit in r_pred they have
	 * set. Put the input values for TEX into a new register set for each
	 * group and execute TEX only for a specific group.
	 * We cannot use the same register set for each group because we need
	 * the derivatives, which are implicitly calculated, to be correct.
	 */
	for (i = 1; i < 4; ++i) {
		alloc_temp4(pc, t123[i], 0);

		for (c = 0; c <= arg; ++c)
			emit_mov(pc, t123[i][c], t[c]);

		*(e = exec(pc)) = *(tex);
		e->inst[0] &= ~0x01fc;
		set_dst(pc, t123[i][0], e);
		set_pred(pc, cc[i], r_pred, e);
		emit(pc, e);
	}
	/* finally TEX on the original regs (where we kept the input) */
	set_pred(pc, cc[0], r_pred, tex);
	emit(pc, tex);

	/* put the 3 * n other results into regs for lane 0 */
	n = popcnt4(((e->inst[0] >> 25) & 0x3) | ((e->inst[1] >> 12) & 0xc));
	for (i = 1; i < 4; ++i) {
		for (c = 0; c < n; ++c) {
			emit_mov(pc, t[c], t123[i][c]);
			set_pred(pc, cc[i], r_pred, pc->p->exec_tail);
		}
		free_temp4(pc, t123[i]);
	}

	emit_nop(pc);
	free_temp(pc, r_bits);
}

static void
emit_tex(struct nv50_pc *pc, struct nv50_reg **dst, unsigned mask,
	 struct nv50_reg **src, unsigned unit, unsigned type,
	 boolean proj, int bias_lod)
{
	struct nv50_reg *t[4];
	struct nv50_program_exec *e;
	unsigned c, dim, arg;

	/* t[i] must be within a single 128 bit super-reg */
	alloc_temp4(pc, t, 0);

	e = exec(pc);
	e->inst[0] = 0xf0000000;
	set_long(pc, e);
	set_dst(pc, t[0], e);

	/* TIC and TSC binding indices (TSC is ignored as TSC_LINKED = TRUE): */
	e->inst[0] |= (unit << 9) /* | (unit << 17) */;

	/* live flag (don't set if TEX results affect input to another TEX): */
	/* e->inst[0] |= 0x00000004; */

	get_tex_dim(type, &dim, &arg);

	if (type == TGSI_TEXTURE_CUBE) {
		e->inst[0] |= 0x08000000;
		load_cube_tex_coords(pc, t, src, arg, proj);
	} else
	if (proj)
		load_proj_tex_coords(pc, t, src, dim, arg);
	else {
		for (c = 0; c < dim; c++)
			emit_mov(pc, t[c], src[c]);
		if (arg != dim) /* depth reference value (always src.z here) */
			emit_mov(pc, t[dim], src[2]);
	}

	e->inst[0] |= (mask & 0x3) << 25;
	e->inst[1] |= (mask & 0xc) << 12;

	if (!bias_lod) {
		e->inst[0] |= (arg - 1) << 22;
		emit(pc, e);
	} else
	if (bias_lod < 0) {
		assert(pc->p->type == PIPE_SHADER_FRAGMENT);
		e->inst[0] |= arg << 22;
		e->inst[1] |= 0x20000000; /* texbias */
		emit_mov(pc, t[arg], src[3]);
		emit_texbias_sequence(pc, t, arg, e);
	} else {
		e->inst[0] |= arg << 22;
		e->inst[1] |= 0x40000000; /* texlod */
		emit_mov(pc, t[arg], src[3]);
		emit_texlod_sequence(pc, t[arg], src[3], e);
	}

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
emit_ddx(struct nv50_pc *pc, struct nv50_reg *dst, struct nv50_reg *src)
{
	struct nv50_program_exec *e = exec(pc);

	assert(src->type == P_TEMP);

	e->inst[0] = (src->mod & NV50_MOD_NEG) ? 0xc0240000 : 0xc0140000;
	e->inst[1] = (src->mod & NV50_MOD_NEG) ? 0x86400000 : 0x89800000;
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

	e->inst[0] = (src->mod & NV50_MOD_NEG) ? 0xc0250000 : 0xc0150000;
	e->inst[1] = (src->mod & NV50_MOD_NEG) ? 0x85800000 : 0x8a400000;
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
	case 0x2:
	case 0x3:
		/* ADD, SUB, SUBR b32 */
		m = ~(0x8000 | (127 << 16));
		q = ((e->inst[0] & (~m)) >> 2) | (1 << 26);
		break;
	case 0x5:
		/* SAD */
		m = ~(0x81 << 8);
		q = (0x0c << 24) | ((e->inst[0] & (0x7f << 2)) << 12);
		break;
	case 0x6:
		/* MAD u16 */
		q = (e->inst[0] & (0x7f << 2)) << 12;
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
static int
get_supported_mods(const struct tgsi_full_instruction *insn, int i)
{
	switch (insn->Instruction.Opcode) {
	case TGSI_OPCODE_ADD:
	case TGSI_OPCODE_COS:
	case TGSI_OPCODE_DDX:
	case TGSI_OPCODE_DDY:
	case TGSI_OPCODE_DP3:
	case TGSI_OPCODE_DP4:
	case TGSI_OPCODE_EX2:
	case TGSI_OPCODE_KIL:
	case TGSI_OPCODE_LG2:
	case TGSI_OPCODE_MAD:
	case TGSI_OPCODE_MUL:
	case TGSI_OPCODE_POW:
	case TGSI_OPCODE_RCP:
	case TGSI_OPCODE_RSQ: /* ignored, RSQ = rsqrt(abs(src.x)) */
	case TGSI_OPCODE_SCS:
	case TGSI_OPCODE_SIN:
	case TGSI_OPCODE_SUB:
		return NV50_MOD_NEG;
	case TGSI_OPCODE_MAX:
	case TGSI_OPCODE_MIN:
	case TGSI_OPCODE_INEG: /* tgsi src sign toggle/set would be stupid */
		return NV50_MOD_ABS;
	case TGSI_OPCODE_CEIL:
	case TGSI_OPCODE_FLR:
	case TGSI_OPCODE_TRUNC:
		return NV50_MOD_NEG | NV50_MOD_ABS;
	case TGSI_OPCODE_F2I:
	case TGSI_OPCODE_F2U:
	case TGSI_OPCODE_I2F:
	case TGSI_OPCODE_U2F:
		return NV50_MOD_NEG | NV50_MOD_ABS | NV50_MOD_I32;
	case TGSI_OPCODE_UADD:
		return NV50_MOD_NEG | NV50_MOD_I32;
	case TGSI_OPCODE_SAD:
	case TGSI_OPCODE_SHL:
	case TGSI_OPCODE_IMAX:
	case TGSI_OPCODE_IMIN:
	case TGSI_OPCODE_ISHR:
	case TGSI_OPCODE_NOT:
	case TGSI_OPCODE_UMAD:
	case TGSI_OPCODE_UMAX:
	case TGSI_OPCODE_UMIN:
	case TGSI_OPCODE_UMUL:
	case TGSI_OPCODE_USHR:
		return NV50_MOD_I32;
	default:
		return 0;
	}
}

/* Return a read mask for source registers deduced from opcode & write mask. */
static unsigned
nv50_tgsi_src_mask(const struct tgsi_full_instruction *insn, int c)
{
	unsigned x, mask = insn->Dst[0].Register.WriteMask;

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
	case TGSI_OPCODE_EXP:
	case TGSI_OPCODE_LG2:
	case TGSI_OPCODE_LOG:
	case TGSI_OPCODE_POW:
	case TGSI_OPCODE_RCP:
	case TGSI_OPCODE_RSQ:
	case TGSI_OPCODE_SCS:
		return 0x1;
	case TGSI_OPCODE_IF:
		return 0x1;
	case TGSI_OPCODE_LIT:
		return 0xb;
	case TGSI_OPCODE_TEX:
	case TGSI_OPCODE_TXB:
	case TGSI_OPCODE_TXL:
	case TGSI_OPCODE_TXP:
	{
		const struct tgsi_instruction_texture *tex;

		assert(insn->Instruction.Texture);
		tex = &insn->Texture;

		mask = 0x7;
		if (insn->Instruction.Opcode != TGSI_OPCODE_TEX &&
		    insn->Instruction.Opcode != TGSI_OPCODE_TXD)
			mask |= 0x8; /* bias, lod or proj */

		switch (tex->Texture) {
		case TGSI_TEXTURE_1D:
			mask &= 0x9;
			break;
		case TGSI_TEXTURE_SHADOW1D:
			mask &= 0x5;
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
	switch (dst->Register.File) {
	case TGSI_FILE_TEMPORARY:
		return &pc->temp[dst->Register.Index * 4 + c];
	case TGSI_FILE_OUTPUT:
		return &pc->result[dst->Register.Index * 4 + c];
	case TGSI_FILE_ADDRESS:
	{
		struct nv50_reg *r = pc->addr[dst->Register.Index * 4 + c];
		if (!r) {
			r = get_address_reg(pc, NULL);
			r->index = dst->Register.Index * 4 + c;
			pc->addr[r->index] = r;
		}
		assert(r);
		return r;
	}
	case TGSI_FILE_NULL:
		return NULL;
	case TGSI_FILE_SYSTEM_VALUE:
		assert(pc->sysval[dst->Register.Index].type == P_RESULT);
		assert(c == 0);
		return &pc->sysval[dst->Register.Index];
	default:
		break;
	}

	return NULL;
}

static struct nv50_reg *
tgsi_src(struct nv50_pc *pc, int chan, const struct tgsi_full_src_register *src,
	 int mod)
{
	struct nv50_reg *r = NULL;
	struct nv50_reg *temp = NULL;
	unsigned sgn, c, swz, cvn;

	if (src->Register.File != TGSI_FILE_CONSTANT)
		assert(!src->Register.Indirect);

	sgn = tgsi_util_get_full_src_register_sign_mode(src, chan);

	c = tgsi_util_get_full_src_register_swizzle(src, chan);
	switch (c) {
	case TGSI_SWIZZLE_X:
	case TGSI_SWIZZLE_Y:
	case TGSI_SWIZZLE_Z:
	case TGSI_SWIZZLE_W:
		switch (src->Register.File) {
		case TGSI_FILE_INPUT:
			r = &pc->attr[src->Register.Index * 4 + c];

			if (!src->Dimension.Dimension)
				break;
			r = reg_instance(pc, r);
			r->vtx = src->Dimension.Index;

			if (!src->Dimension.Indirect)
				break;
			swz = tgsi_util_get_src_register_swizzle(
				&src->DimIndirect, 0);
			r->acc = -1;
			r->indirect[1] = src->DimIndirect.Index * 4 + swz;
			break;
		case TGSI_FILE_TEMPORARY:
			r = &pc->temp[src->Register.Index * 4 + c];
			break;
		case TGSI_FILE_CONSTANT:
			if (!src->Register.Indirect) {
				r = &pc->param[src->Register.Index * 4 + c];
				break;
			}
			/* Indicate indirection by setting r->acc < 0 and
			 * use the index field to select the address reg.
			 */
			r = reg_instance(pc, NULL);
			ctor_reg(r, P_CONST, -1, src->Register.Index * 4 + c);

			swz = tgsi_util_get_src_register_swizzle(
				&src->Indirect, 0);
			r->acc = -1;
			r->indirect[0] = src->Indirect.Index * 4 + swz;
			break;
		case TGSI_FILE_IMMEDIATE:
			r = &pc->immd[src->Register.Index * 4 + c];
			break;
		case TGSI_FILE_SAMPLER:
			return NULL;
		case TGSI_FILE_ADDRESS:
			r = pc->addr[src->Register.Index * 4 + c];
			assert(r);
			break;
		case TGSI_FILE_SYSTEM_VALUE:
			assert(c == 0);
			r = &pc->sysval[src->Register.Index];
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

	cvn = (mod & NV50_MOD_I32) ? CVT_S32_S32 : CVT_F32_F32;

	switch (sgn) {
	case TGSI_UTIL_SIGN_CLEAR:
		r->mod = NV50_MOD_ABS;
		break;
	case TGSI_UTIL_SIGN_SET:
		r->mod = NV50_MOD_NEG_ABS;
		break;
	case TGSI_UTIL_SIGN_TOGGLE:
		r->mod = NV50_MOD_NEG;
		break;
	default:
		assert(!r->mod && sgn == TGSI_UTIL_SIGN_KEEP);
		break;
	}

	if ((r->mod & mod) != r->mod) {
		temp = temp_temp(pc, NULL);
		emit_cvt(pc, temp, r, -1, cvn);
		r->mod = 0;
		r = temp;
	} else
		r->mod |= mod & NV50_MOD_I32;

	assert(r);
	if (r->acc >= 0 && r->vtx < 0 && r != temp)
		return reg_instance(pc, r); /* will clear r->mod */
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
	case TGSI_OPCODE_EXP:
	case TGSI_OPCODE_LOG:
	case TGSI_OPCODE_LIT:
	case TGSI_OPCODE_SCS:
	case TGSI_OPCODE_TEX:
	case TGSI_OPCODE_TXB:
	case TGSI_OPCODE_TXL:
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
	if (is_immd(pc->p->exec_tail))
		return FALSE;

	/* if ccode == 'true', the BRA is from an ELSE and the predicate
	 * reg may no longer be valid, since we currently always use $p0
	 */
	if (has_pred(pc->if_insn[lvl], 0xf))
		return FALSE;
	assert(pc->if_insn[lvl] && pc->if_join[lvl]);

	/* We'll use the exec allocated for JOIN_AT (we can't easily
	 * access nv50_program_exec's prev).
	 */
	pc->p->exec_size -= 4; /* remove JOIN_AT and BRA */

	*pc->if_join[lvl] = *pc->p->exec_tail;

	FREE(pc->if_insn[lvl]);
	FREE(pc->p->exec_tail);

	pc->p->exec_tail = pc->if_join[lvl];
	pc->p->exec_tail->next = NULL;
	set_pred(pc, 0xd, 0, pc->p->exec_tail);

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

static boolean
nv50_program_tx_insn(struct nv50_pc *pc,
		     const struct tgsi_full_instruction *inst)
{
	struct nv50_reg *rdst[4], *dst[4], *brdc, *src[3][4], *temp;
	unsigned mask, sat, unit;
	int i, c;

	mask = inst->Dst[0].Register.WriteMask;
	sat = inst->Instruction.Saturate == TGSI_SAT_ZERO_ONE;

	memset(src, 0, sizeof(src));

	for (c = 0; c < 4; c++) {
		if ((mask & (1 << c)) && !pc->r_dst[c])
			dst[c] = tgsi_dst(pc, c, &inst->Dst[0]);
		else
			dst[c] = pc->r_dst[c];
		rdst[c] = dst[c];
	}

	for (i = 0; i < inst->Instruction.NumSrcRegs; i++) {
		const struct tgsi_full_src_register *fs = &inst->Src[i];
		unsigned src_mask;
		int mod_supp;

		src_mask = nv50_tgsi_src_mask(inst, i);
		mod_supp = get_supported_mods(inst, i);

		if (fs->Register.File == TGSI_FILE_SAMPLER)
			unit = fs->Register.Index;

		for (c = 0; c < 4; c++)
			if (src_mask & (1 << c))
				src[i][c] = tgsi_src(pc, c, fs, mod_supp);
	}

	brdc = temp = pc->r_brdc;
	if (brdc && brdc->type != P_TEMP) {
		temp = temp_temp(pc, NULL);
		if (sat)
			brdc = temp;
	} else
	if (sat) {
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)) || dst[c]->type == P_TEMP)
				continue;
			/* rdst[c] = dst[c]; */ /* done above */
			dst[c] = temp_temp(pc, NULL);
		}
	}

	assert(brdc || !is_scalar_op(inst->Instruction.Opcode));

	switch (inst->Instruction.Opcode) {
	case TGSI_OPCODE_ABS:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_cvt(pc, dst[c], src[0][c], -1,
				 CVT_ABS | CVT_F32_F32);
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
		temp = temp_temp(pc, NULL);
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_cvt(pc, temp, src[0][c], -1,
				 CVT_FLOOR | CVT_S32_F32);
			emit_arl(pc, dst[c], temp, 4);
		}
		break;
	case TGSI_OPCODE_BGNLOOP:
		pc->loop_brka[pc->loop_lvl] = emit_breakaddr(pc);
		pc->loop_pos[pc->loop_lvl++] = pc->p->exec_size;
		terminate_mbb(pc);
		break;
	case TGSI_OPCODE_BGNSUB:
		assert(!pc->in_subroutine);
		pc->in_subroutine = TRUE;
		/* probably not necessary, but align to 8 byte boundary */
		if (!is_long(pc->p->exec_tail))
			convert_to_long(pc, pc->p->exec_tail);
		break;
	case TGSI_OPCODE_BRK:
		assert(pc->loop_lvl > 0);
		emit_break(pc, -1, 0);
		break;
	case TGSI_OPCODE_CAL:
		assert(inst->Label.Label < pc->insn_nr);
		emit_call(pc, -1, 0)->param.index = inst->Label.Label;
		/* replaced by actual offset in nv50_program_fixup_insns */
		break;
	case TGSI_OPCODE_CEIL:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_cvt(pc, dst[c], src[0][c], -1,
				 CVT_CEIL | CVT_F32_F32 | CVT_RI);
		}
		break;
	case TGSI_OPCODE_CMP:
		pc->allow32 = FALSE;
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_cvt(pc, NULL, src[0][c], 1, CVT_F32_F32);
			emit_mov(pc, dst[c], src[1][c]);
			set_pred(pc, 0x1, 1, pc->p->exec_tail); /* @SF */
			emit_mov(pc, dst[c], src[2][c]);
			set_pred(pc, 0x6, 1, pc->p->exec_tail); /* @NSF */
		}
		break;
	case TGSI_OPCODE_CONT:
		assert(pc->loop_lvl > 0);
		emit_branch(pc, -1, 0)->param.index =
			pc->loop_pos[pc->loop_lvl - 1];
		break;
	case TGSI_OPCODE_COS:
		if (mask & 8) {
			emit_precossin(pc, temp, src[0][3]);
			emit_flop(pc, NV50_FLOP_COS, dst[3], temp);
			if (!(mask &= 7))
				break;
			if (temp == dst[3])
				temp = brdc = temp_temp(pc, NULL);
		}
		emit_precossin(pc, temp, src[0][0]);
		emit_flop(pc, NV50_FLOP_COS, brdc, temp);
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
		emit_branch(pc, -1, 0);
		pc->if_insn[--pc->if_lvl]->param.index = pc->p->exec_size;
		pc->if_insn[pc->if_lvl++] = pc->p->exec_tail;
		terminate_mbb(pc);
		break;
	case TGSI_OPCODE_EMIT:
		emit_prim_cmd(pc, 1);
		break;
	case TGSI_OPCODE_ENDIF:
		pc->if_insn[--pc->if_lvl]->param.index = pc->p->exec_size;

		/* try to replace branch over 1 insn with a predicated insn */
		if (nv50_kill_branch(pc) == TRUE)
			break;

		if (pc->if_join[pc->if_lvl]) {
			pc->if_join[pc->if_lvl]->param.index = pc->p->exec_size;
			pc->if_join[pc->if_lvl] = NULL;
		}
		terminate_mbb(pc);
		/* emit a NOP as join point, we could set it on the next
		 * one, but would have to make sure it is long and !immd
		 */
		JOIN_ON(emit_nop(pc));
		break;
	case TGSI_OPCODE_ENDLOOP:
		emit_branch(pc, -1, 0)->param.index =
			pc->loop_pos[--pc->loop_lvl];
		pc->loop_brka[pc->loop_lvl]->param.index = pc->p->exec_size;
		terminate_mbb(pc);
		break;
	case TGSI_OPCODE_ENDPRIM:
		emit_prim_cmd(pc, 2);
		break;
	case TGSI_OPCODE_ENDSUB:
		assert(pc->in_subroutine);
		terminate_mbb(pc);
		pc->in_subroutine = FALSE;
		break;
	case TGSI_OPCODE_EX2:
		emit_preex2(pc, temp, src[0][0]);
		emit_flop(pc, NV50_FLOP_EX2, brdc, temp);
		break;
	case TGSI_OPCODE_EXP:
	{
		struct nv50_reg *t[2];

		assert(!temp);
		t[0] = temp_temp(pc, NULL);
		t[1] = temp_temp(pc, NULL);

		if (mask & 0x6)
			emit_mov(pc, t[0], src[0][0]);
		if (mask & 0x3)
			emit_flr(pc, t[1], src[0][0]);

		if (mask & (1 << 1))
			emit_sub(pc, dst[1], t[0], t[1]);
		if (mask & (1 << 0)) {
			emit_preex2(pc, t[1], t[1]);
			emit_flop(pc, NV50_FLOP_EX2, dst[0], t[1]);
		}
		if (mask & (1 << 2)) {
			emit_preex2(pc, t[0], t[0]);
			emit_flop(pc, NV50_FLOP_EX2, dst[2], t[0]);
		}
		if (mask & (1 << 3))
			emit_mov_immdval(pc, dst[3], 1.0f);
	}
		break;
	case TGSI_OPCODE_F2I:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_cvt(pc, dst[c], src[0][c], -1,
				 CVT_TRUNC | CVT_S32_F32);
		}
		break;
	case TGSI_OPCODE_F2U:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_cvt(pc, dst[c], src[0][c], -1,
				 CVT_TRUNC | CVT_U32_F32);
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
		temp = temp_temp(pc, NULL);
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_flr(pc, temp, src[0][c]);
			emit_sub(pc, dst[c], src[0][c], temp);
		}
		break;
	case TGSI_OPCODE_I2F:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_cvt(pc, dst[c], src[0][c], -1, CVT_F32_S32);
		}
		break;
	case TGSI_OPCODE_IF:
		assert(pc->if_lvl < NV50_MAX_COND_NESTING);
		emit_cvt(pc, NULL, src[0][0], 0, CVT_ABS | CVT_F32_F32);
		pc->if_join[pc->if_lvl] = emit_joinat(pc);
		pc->if_insn[pc->if_lvl++] = emit_branch(pc, 0, 2);;
		terminate_mbb(pc);
		break;
	case TGSI_OPCODE_IMAX:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_minmax(pc, 0x08c, dst[c], src[0][c], src[1][c]);
		}
		break;
	case TGSI_OPCODE_IMIN:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_minmax(pc, 0x0ac, dst[c], src[0][c], src[1][c]);
		}
		break;
	case TGSI_OPCODE_INEG:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_cvt(pc, dst[c], src[0][c], -1,
				 CVT_S32_S32 | CVT_NEG);
		}
		break;
	case TGSI_OPCODE_KIL:
		assert(src[0][0] && src[0][1] && src[0][2] && src[0][3]);
		emit_kil(pc, src[0][0]);
		emit_kil(pc, src[0][1]);
		emit_kil(pc, src[0][2]);
		emit_kil(pc, src[0][3]);
		break;
	case TGSI_OPCODE_KILP:
		emit_kil(pc, NULL);
		break;
	case TGSI_OPCODE_LIT:
		emit_lit(pc, &dst[0], mask, &src[0][0]);
		break;
	case TGSI_OPCODE_LG2:
		emit_flop(pc, NV50_FLOP_LG2, brdc, src[0][0]);
		break;
	case TGSI_OPCODE_LOG:
	{
		struct nv50_reg *t[2];

		t[0] = temp_temp(pc, NULL);
		if (mask & (1 << 1))
			t[1] = temp_temp(pc, NULL);
		else
			t[1] = t[0];

		emit_cvt(pc, t[0], src[0][0], -1, CVT_ABS | CVT_F32_F32);
		emit_flop(pc, NV50_FLOP_LG2, t[1], t[0]);
		if (mask & (1 << 2))
			emit_mov(pc, dst[2], t[1]);
		emit_flr(pc, t[1], t[1]);
		if (mask & (1 << 0))
			emit_mov(pc, dst[0], t[1]);
		if (mask & (1 << 1)) {
			t[1]->mod = NV50_MOD_NEG;
			emit_preex2(pc, t[1], t[1]);
			t[1]->mod = 0;
			emit_flop(pc, NV50_FLOP_EX2, t[1], t[1]);
			emit_mul(pc, dst[1], t[0], t[1]);
		}
		if (mask & (1 << 3))
			emit_mov_immdval(pc, dst[3], 1.0f);
	}
		break;
	case TGSI_OPCODE_LRP:
		temp = temp_temp(pc, NULL);
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
			emit_minmax(pc, 0x880, dst[c], src[0][c], src[1][c]);
		}
		break;
	case TGSI_OPCODE_MIN:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_minmax(pc, 0x8a0, dst[c], src[0][c], src[1][c]);
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
	case TGSI_OPCODE_NOT:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_not(pc, dst[c], src[0][c]);
		}
		break;
	case TGSI_OPCODE_POW:
		emit_pow(pc, brdc, src[0][0], src[1][0]);
		break;
	case TGSI_OPCODE_RCP:
		if (!sat && popcnt4(mask) == 1)
			brdc = dst[ffs(mask) - 1];
		emit_flop(pc, NV50_FLOP_RCP, brdc, src[0][0]);
		break;
	case TGSI_OPCODE_RET:
		if (pc->p->type == PIPE_SHADER_FRAGMENT && !pc->in_subroutine)
			nv50_fp_move_results(pc);
		emit_ret(pc, -1, 0);
		break;
	case TGSI_OPCODE_RSQ:
		if (!sat && popcnt4(mask) == 1)
			brdc = dst[ffs(mask) - 1];
		src[0][0]->mod |= NV50_MOD_ABS;
		emit_flop(pc, NV50_FLOP_RSQ, brdc, src[0][0]);
		break;
	case TGSI_OPCODE_SAD:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_sad(pc, dst[c], src[0][c], src[1][c], src[2][c]);
		}
		break;
	case TGSI_OPCODE_SCS:
		temp = temp_temp(pc, NULL);
		if (mask & 3)
			emit_precossin(pc, temp, src[0][0]);
		if (mask & (1 << 0))
			emit_flop(pc, NV50_FLOP_COS, dst[0], temp);
		if (mask & (1 << 1))
			emit_flop(pc, NV50_FLOP_SIN, dst[1], temp);
		if (mask & (1 << 2))
			emit_mov_immdval(pc, dst[2], 0.0);
		if (mask & (1 << 3))
			emit_mov_immdval(pc, dst[3], 1.0);
		break;
	case TGSI_OPCODE_SHL:
	case TGSI_OPCODE_ISHR:
	case TGSI_OPCODE_USHR:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_shift(pc, dst[c], src[0][c], src[1][c],
				   inst->Instruction.Opcode);
		}
		break;
	case TGSI_OPCODE_SIN:
		if (mask & 8) {
			emit_precossin(pc, temp, src[0][3]);
			emit_flop(pc, NV50_FLOP_SIN, dst[3], temp);
			if (!(mask &= 7))
				break;
			if (temp == dst[3])
				temp = brdc = temp_temp(pc, NULL);
		}
		emit_precossin(pc, temp, src[0][0]);
		emit_flop(pc, NV50_FLOP_SIN, brdc, temp);
		break;
	case TGSI_OPCODE_SLT:
	case TGSI_OPCODE_SGE:
	case TGSI_OPCODE_SEQ:
	case TGSI_OPCODE_SGT:
	case TGSI_OPCODE_SLE:
	case TGSI_OPCODE_SNE:
	case TGSI_OPCODE_ISLT:
	case TGSI_OPCODE_ISGE:
	case TGSI_OPCODE_USEQ:
	case TGSI_OPCODE_USGE:
	case TGSI_OPCODE_USLT:
	case TGSI_OPCODE_USNE:
	{
		uint8_t cc, ty;

		map_tgsi_setop_hw(inst->Instruction.Opcode, &cc, &ty);

		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_set(pc, cc, dst[c], -1, src[0][c], src[1][c], ty);
		}
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
			 inst->Texture.Texture, FALSE, 0);
		break;
	case TGSI_OPCODE_TXB:
		emit_tex(pc, dst, mask, src[0], unit,
			 inst->Texture.Texture, FALSE, -1);
		break;
	case TGSI_OPCODE_TXL:
		emit_tex(pc, dst, mask, src[0], unit,
			 inst->Texture.Texture, FALSE, 1);
		break;
	case TGSI_OPCODE_TXP:
		emit_tex(pc, dst, mask, src[0], unit,
			 inst->Texture.Texture, TRUE, 0);
		break;
	case TGSI_OPCODE_TRUNC:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_cvt(pc, dst[c], src[0][c], -1,
				 CVT_TRUNC | CVT_F32_F32 | CVT_RI);
		}
		break;
	case TGSI_OPCODE_U2F:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_cvt(pc, dst[c], src[0][c], -1, CVT_F32_U32);
		}
		break;
	case TGSI_OPCODE_UADD:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_add_b32(pc, dst[c], src[0][c], src[1][c]);
		}
		break;
	case TGSI_OPCODE_UMAX:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_minmax(pc, 0x084, dst[c], src[0][c], src[1][c]);
		}
		break;
	case TGSI_OPCODE_UMIN:
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_minmax(pc, 0x0a4, dst[c], src[0][c], src[1][c]);
		}
		break;
	case TGSI_OPCODE_UMAD:
	{
		assert(!temp);
		temp = temp_temp(pc, NULL);
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_mul_u16(pc, temp, src[0][c], 0, src[1][c], 1);
			emit_mad_u16(pc, temp, src[0][c], 1, src[1][c], 0,
				     temp);
			emit_shl_imm(pc, temp, temp, 16);
			emit_mad_u16(pc, temp, src[0][c], 0, src[1][c], 0,
				     temp);
			emit_add_b32(pc, dst[c], temp, src[2][c]);
		}
	}
		break;
	case TGSI_OPCODE_UMUL:
	{
		assert(!temp);
		temp = temp_temp(pc, NULL);
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			emit_mul_u16(pc, temp, src[0][c], 0, src[1][c], 1);
			emit_mad_u16(pc, temp, src[0][c], 1, src[1][c], 0,
				     temp);
			emit_shl_imm(pc, temp, temp, 16);
			emit_mad_u16(pc, dst[c], src[0][c], 0, src[1][c], 0,
				     temp);
		}
	}
		break;
	case TGSI_OPCODE_XPD:
		temp = temp_temp(pc, NULL);
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
		if (pc->p->type == PIPE_SHADER_FRAGMENT)
			nv50_fp_move_results(pc);

		/* last insn must be long so it can have the exit bit set */
		if (!is_long(pc->p->exec_tail))
			convert_to_long(pc, pc->p->exec_tail);
		else
		if (is_immd(pc->p->exec_tail) ||
		    is_join(pc->p->exec_tail) ||
		    is_control_flow(pc->p->exec_tail))
			emit_nop(pc);

		pc->p->exec_tail->inst[1] |= 1; /* set exit bit */

		terminate_mbb(pc);
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

	kill_temp_temp(pc, NULL);
	pc->reg_instance_nr = 0;

	return TRUE;
}

static void
prep_inspect_insn(struct nv50_pc *pc, const struct tgsi_full_instruction *insn)
{
	struct nv50_reg *r, *reg = NULL;
	const struct tgsi_full_src_register *src;
	const struct tgsi_dst_register *dst;
	unsigned i, c, k, mask;

	dst = &insn->Dst[0].Register;
	mask = dst->WriteMask;

        if (dst->File == TGSI_FILE_TEMPORARY)
		reg = pc->temp;
        else
	if (dst->File == TGSI_FILE_OUTPUT) {
		reg = pc->result;

		if (insn->Instruction.Opcode == TGSI_OPCODE_MOV &&
		    dst->Index == pc->edgeflag_out &&
		    insn->Src[0].Register.File == TGSI_FILE_INPUT)
			pc->p->cfg.edgeflag_in = insn->Src[0].Register.Index;
	}

	if (reg) {
		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			reg[dst->Index * 4 + c].acc = pc->insn_nr;
		}
	}

	for (i = 0; i < insn->Instruction.NumSrcRegs; i++) {
		src = &insn->Src[i];

		if (src->Register.File == TGSI_FILE_TEMPORARY)
			reg = pc->temp;
		else
		if (src->Register.File == TGSI_FILE_INPUT)
			reg = pc->attr;
		else
			continue;

		mask = nv50_tgsi_src_mask(insn, i);

		for (c = 0; c < 4; c++) {
			if (!(mask & (1 << c)))
				continue;
			k = tgsi_util_get_full_src_register_swizzle(src, c);

			r = &reg[src->Register.Index * 4 + k];

			/* If used before written, pre-allocate the reg,
			 * lest we overwrite results from a subroutine.
			 */
			if (!r->acc && r->type == P_TEMP)
				alloc_reg(pc, r);

			r->acc = pc->insn_nr;
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
	unsigned i, c, x, unsafe = 0;

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
	if (fd->Register.File == TGSI_FILE_TEMPORARY) {
		int c = ffs(~mask & fd->Register.WriteMask);
		if (c)
			return tgsi_dst(pc, c - 1, fd);
	} else {
		int c = ffs(fd->Register.WriteMask) - 1;
		if ((1 << c) == fd->Register.WriteMask)
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
	const struct tgsi_full_dst_register *fd = &insn->Dst[0];
	const struct tgsi_full_src_register *fs;
	unsigned i, deqs = 0;

	for (i = 0; i < 4; ++i)
		rdep[i] = 0;

	for (i = 0; i < insn->Instruction.NumSrcRegs; i++) {
		unsigned chn, mask = nv50_tgsi_src_mask(insn, i);
		int ms = get_supported_mods(insn, i);

		fs = &insn->Src[i];
		if (fs->Register.File != fd->Register.File ||
		    fs->Register.Index != fd->Register.Index)
			continue;

		for (chn = 0; chn < 4; ++chn) {
			unsigned s, c;

			if (!(mask & (1 << chn))) /* src is not read */
				continue;
			c = tgsi_util_get_full_src_register_swizzle(fs, chn);
			s = tgsi_util_get_full_src_register_sign_mode(fs, chn);

			if (!(fd->Register.WriteMask & (1 << c)))
				continue;

			if (s == TGSI_UTIL_SIGN_TOGGLE && !(ms & NV50_MOD_NEG))
					continue;
			if (s == TGSI_UTIL_SIGN_CLEAR && !(ms & NV50_MOD_ABS))
					continue;
			if ((s == TGSI_UTIL_SIGN_SET) && ((ms & 3) != 3))
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

	fd = &tok->FullInstruction.Dst[0];
	deqs = nv50_tgsi_scan_swizzle(&insn, rdep);

	if (is_scalar_op(insn.Instruction.Opcode)) {
		pc->r_brdc = tgsi_broadcast_dst(pc, fd, deqs);
		if (!pc->r_brdc)
			pc->r_brdc = temp_temp(pc, NULL);
		return nv50_program_tx_insn(pc, &insn);
	}
	pc->r_brdc = NULL;

	if (!deqs || (!rdep[0] && !rdep[1] && !rdep[2] && !rdep[3]))
		return nv50_program_tx_insn(pc, &insn);

	deqs = nv50_revdep_reorder(m, rdep);

	for (i = 0; i < 4; ++i) {
		assert(pc->r_dst[m[i]] == NULL);

		insn.Dst[0].Register.WriteMask =
			fd->Register.WriteMask & (1 << m[i]);

		if (!insn.Dst[0].Register.WriteMask)
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
		emit_flop(pc, NV50_FLOP_RCP, iv, iv);

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
load_frontfacing(struct nv50_pc *pc, struct nv50_reg *sv)
{
	struct nv50_reg *temp = alloc_temp(pc, NULL);
	int r_pred = 0;

	temp->rhw = 255;
	emit_interp(pc, temp, NULL, INTERP_FLAT);

	emit_cvt(pc, sv, temp, r_pred, CVT_ABS | CVT_F32_S32);

	emit_not(pc, temp, temp);
	set_pred(pc, 0x2, r_pred, pc->p->exec_tail);
	emit_cvt(pc, sv, temp, -1, CVT_F32_S32);
	set_pred(pc, 0x2, r_pred, pc->p->exec_tail);

	free_temp(pc, temp);
}

static void
load_instance_id(struct nv50_pc *pc, unsigned index)
{
	struct nv50_reg reg, mem;

	ctor_reg(&reg, P_TEMP, -1, -1);
	ctor_reg(&mem, P_CONST, -1, 24); /* startInstance */
	mem.buf_index = 2;

	emit_add_b32(pc, &reg, &pc->sysval[index], &mem);
	pc->sysval[index] = reg;
}

static void
copy_semantic_info(struct nv50_program *p)
{
	unsigned i, id;

	for (i = 0; i < p->cfg.in_nr; ++i) {
		id = p->cfg.in[i].id;
		p->cfg.in[i].sn = p->info.input_semantic_name[id];
		p->cfg.in[i].si = p->info.input_semantic_index[id];
	}

	for (i = 0; i < p->cfg.out_nr; ++i) {
		id = p->cfg.out[i].id;
		p->cfg.out[i].sn = p->info.output_semantic_name[id];
		p->cfg.out[i].si = p->info.output_semantic_index[id];
	}
}

static boolean
nv50_program_tx_prep(struct nv50_pc *pc)
{
	struct tgsi_parse_context tp;
	struct nv50_program *p = pc->p;
	boolean ret = FALSE;
	unsigned i, c, instance_id, vertex_id, flat_nr = 0;

	tgsi_parse_init(&tp, pc->p->pipe.tokens);
	while (!tgsi_parse_end_of_tokens(&tp)) {
		const union tgsi_full_token *tok = &tp.FullToken;

		tgsi_parse_token(&tp);
		switch (tok->Token.Type) {
		case TGSI_TOKEN_TYPE_IMMEDIATE:
		{
			const struct tgsi_full_immediate *imm =
				&tp.FullToken.FullImmediate;

			ctor_immd_4f32(pc, imm->u[0].Float,
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
			first = d->Range.First;
			last = d->Range.Last;

			switch (d->Declaration.File) {
			case TGSI_FILE_TEMPORARY:
				break;
			case TGSI_FILE_OUTPUT:
				if (!d->Declaration.Semantic ||
				    p->type == PIPE_SHADER_FRAGMENT)
					break;

				si = d->Semantic.Index;
				switch (d->Semantic.Name) {
				case TGSI_SEMANTIC_BCOLOR:
					p->cfg.two_side[si].hw = first;
					if (p->cfg.out_nr > first)
						p->cfg.out_nr = first;
					break;
				case TGSI_SEMANTIC_PSIZE:
					p->cfg.psiz = first;
					if (p->cfg.out_nr > first)
						p->cfg.out_nr = first;
					break;
				case TGSI_SEMANTIC_EDGEFLAG:
					pc->edgeflag_out = first;
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
			case TGSI_FILE_SYSTEM_VALUE:
				assert(d->Declaration.Semantic);
				switch (d->Semantic.Name) {
				case TGSI_SEMANTIC_FACE:
					assert(p->type == PIPE_SHADER_FRAGMENT);
					load_frontfacing(pc,
							 &pc->sysval[first]);
					break;
				case TGSI_SEMANTIC_INSTANCEID:
					assert(p->type == PIPE_SHADER_VERTEX);
					instance_id = first;
					p->cfg.regs[0] |= (1 << 4);
					break;
				case TGSI_SEMANTIC_PRIMID:
					assert(p->type != PIPE_SHADER_VERTEX);
					p->cfg.prim_id = first;
					break;
					/*
				case TGSI_SEMANTIC_PRIMIDIN:
					assert(p->type == PIPE_SHADER_GEOMETRY);
					pc->sysval[first].hw = 6;
					p->cfg.regs[0] |= (1 << 8);
					break;
				case TGSI_SEMANTIC_VERTEXID:
					assert(p->type == PIPE_SHADER_VERTEX);
					vertex_id = first;
					p->cfg.regs[0] |= (1 << 12) | (1 << 0);
					break;
					*/
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

	if (p->type == PIPE_SHADER_VERTEX || p->type == PIPE_SHADER_GEOMETRY) {
		int rid = 0;

		if (p->type == PIPE_SHADER_GEOMETRY) {
			for (i = 0; i < pc->attr_nr; ++i) {
				p->cfg.in[i].hw = rid;
				p->cfg.in[i].id = i;

				for (c = 0; c < 4; ++c) {
					int n = i * 4 + c;
					if (!pc->attr[n].acc)
						continue;
					pc->attr[n].hw = rid++;
					p->cfg.in[i].mask |= 1 << c;
				}
			}
		} else {
			for (i = 0; i < pc->attr_nr * 4; ++i) {
				if (pc->attr[i].acc) {
					pc->attr[i].hw = rid++;
					p->cfg.attr[i / 32] |= 1 << (i % 32);
				}
			}
			if (p->cfg.regs[0] & (1 << 0))
				pc->sysval[vertex_id].hw = rid++;
			if (p->cfg.regs[0] & (1 << 4)) {
				pc->sysval[instance_id].hw = rid++;
				load_instance_id(pc, instance_id);
			}
		}

		for (i = 0, rid = 0; i < pc->result_nr; ++i) {
			p->cfg.out[i].hw = rid;
			p->cfg.out[i].id = i;

			for (c = 0; c < 4; ++c) {
				int n = i * 4 + c;
				if (!pc->result[n].acc)
					continue;
				pc->result[n].hw = rid++;
				p->cfg.out[i].mask |= 1 << c;
			}
		}
		if (p->cfg.prim_id < 0x40) {
			/* GP has to write to PrimitiveID */
			ctor_reg(&pc->sysval[p->cfg.prim_id],
				 P_RESULT, p->cfg.prim_id, rid);
			p->cfg.prim_id = rid++;
		}

		for (c = 0; c < 2; ++c)
			if (p->cfg.two_side[c].hw < 0x40)
				p->cfg.two_side[c] = p->cfg.out[
					p->cfg.two_side[c].hw];

		if (p->cfg.psiz < 0x40)
			p->cfg.psiz = p->cfg.out[p->cfg.psiz].hw;

		copy_semantic_info(p);
	} else
	if (p->type == PIPE_SHADER_FRAGMENT) {
		int rid, aid;
		unsigned n = 0, m = pc->attr_nr - flat_nr;

		pc->allow32 = TRUE;

		/* do we read FragCoord ? */
		if (pc->attr_nr &&
		    p->info.input_semantic_name[0] == TGSI_SEMANTIC_POSITION) {
			/* select FCRD components we want accessible */
			for (c = 0; c < 4; ++c)
				if (pc->attr[c].acc)
					p->cfg.regs[1] |= 1 << (24 + c);
			aid = 0;
		} else /* offset by 1 if FCRD.w is needed for pinterp */
			aid = popcnt4(p->cfg.regs[1] >> 24);

		/* non-flat interpolants have to be mapped to
		 * the lower hardware IDs, so sort them:
		 */
		for (i = 0; i < pc->attr_nr; i++) {
			if (pc->interp_mode[i] == INTERP_FLAT)
				p->cfg.in[m++].id = i;
			else {
				if (!(pc->interp_mode[i] & INTERP_PERSPECTIVE))
					p->cfg.in[n].linear = TRUE;
				p->cfg.in[n++].id = i;
			}
		}
		copy_semantic_info(p);

		for (n = 0; n < pc->attr_nr; ++n) {
			p->cfg.in[n].hw = rid = aid;
			i = p->cfg.in[n].id;

			if (p->info.input_semantic_name[n] ==
			    TGSI_SEMANTIC_FACE) {
				load_frontfacing(pc, &pc->attr[i * 4]);
				continue;
			}

			for (c = 0; c < 4; ++c) {
				if (!pc->attr[i * 4 + c].acc)
					continue;
				pc->attr[i * 4 + c].rhw = rid++;
				p->cfg.in[n].mask |= 1 << c;

				load_interpolant(pc, &pc->attr[i * 4 + c]);
			}
			aid += popcnt4(p->cfg.in[n].mask);
		}

		m = popcnt4(p->cfg.regs[1] >> 24);

		/* set count of non-position inputs and of non-flat
		 * non-position inputs for FP_INTERPOLANT_CTRL
		 */
		p->cfg.regs[1] |= aid - m;

		if (flat_nr) {
			i = p->cfg.in[pc->attr_nr - flat_nr].hw;
			p->cfg.regs[1] |= (i - m) << 16;
		} else
			p->cfg.regs[1] |= p->cfg.regs[1] << 16;

		/* mark color semantic for light-twoside */
		n = 0x80;
		for (i = 0; i < p->cfg.in_nr; i++) {
			if (p->cfg.in[i].sn == TGSI_SEMANTIC_COLOR) {
				n = MIN2(n, p->cfg.in[i].hw - m);
				p->cfg.two_side[p->cfg.in[i].si] = p->cfg.in[i];

				p->cfg.regs[0] += /* increase colour count */
					popcnt4(p->cfg.in[i].mask) << 16;
			}
		}
		if (n < 0x80)
			p->cfg.regs[0] += n;

		if (p->cfg.prim_id < 0x40) {
			pc->sysval[p->cfg.prim_id].rhw = rid++;
			emit_interp(pc, &pc->sysval[p->cfg.prim_id], NULL,
				    INTERP_FLAT);
			/* increase FP_INTERPOLANT_CTRL_COUNT */
			p->cfg.regs[1] += 1;
		}

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
	if (pc->sysval)
		FREE(pc->sysval);
	if (pc->insn_pos)
		FREE(pc->insn_pos);

	FREE(pc);
}

static INLINE uint32_t
nv50_map_gs_output_prim(unsigned pprim)
{
	switch (pprim) {
	case PIPE_PRIM_POINTS:
		return NV50TCL_GP_OUTPUT_PRIMITIVE_TYPE_POINTS;
	case PIPE_PRIM_LINE_STRIP:
		return NV50TCL_GP_OUTPUT_PRIMITIVE_TYPE_LINE_STRIP;
	case PIPE_PRIM_TRIANGLE_STRIP:
		return NV50TCL_GP_OUTPUT_PRIMITIVE_TYPE_TRIANGLE_STRIP;
	default:
		NOUVEAU_ERR("invalid GS_OUTPUT_PRIMITIVE: %u\n", pprim);
		abort();
		return 0;
	}
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
	pc->sysval_nr = p->info.file_max[TGSI_FILE_SYSTEM_VALUE] + 1;

	p->cfg.high_temp = 4;

	p->cfg.two_side[0].hw = 0x40;
	p->cfg.two_side[1].hw = 0x40;
	p->cfg.prim_id = 0x40;

	p->cfg.edgeflag_in = pc->edgeflag_out = 0xff;

	for (i = 0; i < p->info.num_properties; ++i) {
		unsigned *data = &p->info.properties[i].data[0];

		switch (p->info.properties[i].name) {
		case TGSI_PROPERTY_GS_OUTPUT_PRIM:
			p->cfg.prim_type = nv50_map_gs_output_prim(data[0]);
			break;
		case TGSI_PROPERTY_GS_MAX_VERTICES:
			p->cfg.vert_count = data[0];
			break;
		default:
			break;
		}
	}

	switch (p->type) {
	case PIPE_SHADER_VERTEX:
		p->cfg.psiz = 0x40;
		p->cfg.clpd = 0x40;
		p->cfg.out_nr = pc->result_nr;
		break;
	case PIPE_SHADER_GEOMETRY:
		assert(p->cfg.prim_type);
		assert(p->cfg.vert_count);

		p->cfg.psiz = 0x80;
		p->cfg.clpd = 0x80;
		p->cfg.prim_id = 0x80;
		p->cfg.out_nr = pc->result_nr;
		p->cfg.in_nr = pc->attr_nr;

		p->cfg.two_side[0].hw = 0x80;
		p->cfg.two_side[1].hw = 0x80;
		break;
	case PIPE_SHADER_FRAGMENT:
		rtype[0] = rtype[1] = P_TEMP;

		p->cfg.regs[0] = 0x01000004;
		p->cfg.in_nr = pc->attr_nr;

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
		ctor_reg(&pc->r_addr[i], P_ADDR, -1, i + 1);

	if (pc->sysval_nr) {
		pc->sysval = CALLOC(pc->sysval_nr, sizeof(struct nv50_reg *));
		if (!pc->sysval)
			return FALSE;
		/* will only ever use SYSTEM_VALUE[i].x (hopefully) */
		for (i = 0; i < pc->sysval_nr; ++i)
			ctor_reg(&pc->sysval[i], rtype[0], i, -1);
	}

	return TRUE;
}

static void
nv50_program_fixup_insns(struct nv50_pc *pc)
{
	struct nv50_program_exec *e, **bra_list;
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
			for (i = 0; i < pc->insn_nr; ++i)
				if (pc->insn_pos[i] >= pos)
					pc->insn_pos[i] += 1;
			convert_to_long(pc, e);
			++pos;
		}
	}

	FREE(bra_list);

	if (!pc->p->info.opcode_count[TGSI_OPCODE_CAL])
		return;

	/* fill in CALL offsets */
	for (e = pc->p->exec_head; e; e = e->next) {
		if ((e->inst[0] & 2) && (e->inst[0] >> 28) == 0x2)
			e->param.index = pc->insn_pos[e->param.index];
	}
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

	pc->insn_pos = MALLOC(pc->insn_nr * sizeof(unsigned));

	tgsi_parse_init(&parse, pc->p->pipe.tokens);
	while (!tgsi_parse_end_of_tokens(&parse)) {
		const union tgsi_full_token *tok = &parse.FullToken;

		/* previously allow32 was FALSE for first & last instruction */
		pc->allow32 = TRUE;

		tgsi_parse_token(&parse);

		switch (tok->Token.Type) {
		case TGSI_TOKEN_TYPE_INSTRUCTION:
			pc->insn_pos[pc->insn_cur] = pc->p->exec_size;
			++pc->insn_cur;
			ret = nv50_tgsi_insn(pc, tok);
			if (ret == FALSE)
				goto out_err;
			break;
		default:
			break;
		}
	}

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
nv50_program_upload_data(struct nv50_context *nv50, uint32_t *map,
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
		uint32_t *map = pipe_buffer_map(pscreen,
						nv50->constbuf[p->type],
						PIPE_BUFFER_USAGE_CPU_READ);
		switch (p->type) {
		case PIPE_SHADER_GEOMETRY: cb = NV50_CB_PGP; break;
		case PIPE_SHADER_FRAGMENT: cb = NV50_CB_PFP; break;
		default:
			cb = NV50_CB_PVP;
			assert(p->type == PIPE_SHADER_VERTEX);
			break;
		}

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

	so = so_new(5, 7, 2);
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
	so_method(so, tesla, NV50TCL_VP_REG_ALLOC_TEMP, 1);
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

	so = so_new(6, 7, 2);
	so_method(so, tesla, NV50TCL_FP_ADDRESS_HIGH, 2);
	so_reloc (so, p->bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
		      NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, p->bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
		      NOUVEAU_BO_LOW, 0, 0);
	so_method(so, tesla, NV50TCL_FP_REG_ALLOC_TEMP, 1);
	so_data  (so, p->cfg.high_temp);
	so_method(so, tesla, NV50TCL_FP_RESULT_COUNT, 1);
	so_data  (so, p->cfg.high_result);
	so_method(so, tesla, NV50TCL_FP_CONTROL, 1);
	so_data  (so, p->cfg.regs[2]);
	so_method(so, tesla, NV50TCL_FP_CTRL_UNK196C, 1);
	so_data  (so, p->cfg.regs[3]);
	so_method(so, tesla, NV50TCL_FP_START_ID, 1);
	so_data  (so, 0); /* program start offset */
	so_ref(so, &nv50->state.fragprog);
	so_ref(NULL, &so);
}

void
nv50_geomprog_validate(struct nv50_context *nv50)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nv50_program *p = nv50->geomprog;
	struct nouveau_stateobj *so;

	if (!p->translated) {
		nv50_program_validate(nv50, p);
		if (!p->translated)
			assert(0);
	}

	nv50_program_validate_data(nv50, p);
	nv50_program_validate_code(nv50, p);

	so = so_new(6, 7, 2);
	so_method(so, tesla, NV50TCL_GP_ADDRESS_HIGH, 2);
	so_reloc (so, p->bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
		  NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, p->bo, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
		  NOUVEAU_BO_LOW, 0, 0);
	so_method(so, tesla, NV50TCL_GP_REG_ALLOC_TEMP, 1);
	so_data  (so, p->cfg.high_temp);
	so_method(so, tesla, NV50TCL_GP_REG_ALLOC_RESULT, 1);
	so_data  (so, p->cfg.high_result);
	so_method(so, tesla, NV50TCL_GP_OUTPUT_PRIMITIVE_TYPE, 1);
	so_data  (so, p->cfg.prim_type);
	so_method(so, tesla, NV50TCL_GP_VERTEX_OUTPUT_COUNT, 1);
	so_data  (so, p->cfg.vert_count);
	so_method(so, tesla, NV50TCL_GP_START_ID, 1);
	so_data  (so, 0);
	so_ref(so, &nv50->state.geomprog);
	so_ref(NULL, &so);
}

static uint32_t
nv50_pntc_replace(struct nv50_context *nv50, uint32_t pntc[8], unsigned base)
{
	struct nv50_program *vp;
	struct nv50_program *fp = nv50->fragprog;
	unsigned i, c, m = base;
	uint32_t origin = 0x00000010;

	vp = nv50->geomprog ? nv50->geomprog : nv50->vertprog;

	/* XXX: this might not work correctly in all cases yet - we'll
	 * just assume that an FP generic input that is not written in
	 * the VP is PointCoord.
	 */
	memset(pntc, 0, 8 * sizeof(uint32_t));

	for (i = 0; i < fp->cfg.in_nr; i++) {
		unsigned j, n = popcnt4(fp->cfg.in[i].mask);

		if (fp->cfg.in[i].sn != TGSI_SEMANTIC_GENERIC) {
			m += n;
			continue;
		}

		for (j = 0; j < vp->cfg.out_nr; ++j)
			if (vp->cfg.out[j].sn ==  fp->cfg.in[i].sn &&
			    vp->cfg.out[j].si == fp->cfg.in[i].si)
				break;

		if (j < vp->info.num_outputs) {
			ubyte enable =
				 (nv50->rasterizer->pipe.sprite_coord_enable >> vp->cfg.out[j].si) & 1;

			if (enable == 0) {
				m += n;
				continue;
			}
		}

		/* this is either PointCoord or replaced by sprite coords */
		for (c = 0; c < 4; c++) {
			if (!(fp->cfg.in[i].mask & (1 << c)))
				continue;
			pntc[m / 8] |= (c + 1) << ((m % 8) * 4);
			++m;
		}
	}
	return (nv50->rasterizer->pipe.sprite_coord_mode == PIPE_SPRITE_COORD_LOWER_LEFT ? 0 : origin);
}

static int
nv50_vec4_map(uint32_t *map32, int mid, uint8_t zval, uint32_t lin[4],
	      struct nv50_sreg4 *fpi, struct nv50_sreg4 *vpo)
{
	int c;
	uint8_t mv = vpo->mask, mf = fpi->mask, oid = vpo->hw;
	uint8_t *map = (uint8_t *)map32;

	for (c = 0; c < 4; ++c) {
		if (mf & 1) {
			if (fpi->linear == TRUE)
				lin[mid / 32] |= 1 << (mid % 32);
			if (mv & 1)
				map[mid] = oid;
			else
				map[mid] = (c == 3) ? (zval + 1) : zval;
			++mid;
		}

		oid += mv & 1;
		mf >>= 1;
		mv >>= 1;
	}

	return mid;
}

void
nv50_fp_linkage_validate(struct nv50_context *nv50)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nv50_program *vp = nv50->vertprog;
	struct nv50_program *fp = nv50->fragprog;
	struct nouveau_stateobj *so;
	struct nv50_sreg4 dummy;
	int i, n, c, m = 0;
	uint32_t map[16], lin[4], reg[6], pcrd[8];
	uint8_t zval = 0x40;

	if (nv50->geomprog) {
		vp = nv50->geomprog;
		zval = 0x80;
	}
	memset(map, 0, sizeof(map));
	memset(lin, 0, sizeof(lin));

	reg[1] = 0x00000004; /* low and high clip distance map ids */
	reg[2] = 0x00000000; /* layer index map id (disabled, GP only) */
	reg[3] = 0x00000000; /* point size map id & enable */
	reg[5] = 0x00000000; /* primitive ID map slot */
	reg[0] = fp->cfg.regs[0]; /* colour semantic reg */
	reg[4] = fp->cfg.regs[1]; /* interpolant info */

	dummy.linear = FALSE;
	dummy.mask = 0xf; /* map all components of HPOS */
	m = nv50_vec4_map(map, m, zval, lin, &dummy, &vp->cfg.out[0]);

	dummy.mask = 0x0;

	if (vp->cfg.clpd < 0x40) {
		for (c = 0; c < vp->cfg.clpd_nr; ++c) {
			map[m / 4] |= (vp->cfg.clpd + c) << ((m % 4) * 8);
			++m;
		}
		reg[1] = (m << 8);
	}

	reg[0] |= m << 8; /* adjust BFC0 id */

	/* if light_twoside is active, it seems FFC0_ID == BFC0_ID is bad */
	if (nv50->rasterizer->pipe.light_twoside) {
		struct nv50_sreg4 *vpo = &vp->cfg.two_side[0];
		struct nv50_sreg4 *fpi = &fp->cfg.two_side[0];

		m = nv50_vec4_map(map, m, zval, lin, &fpi[0], &vpo[0]);
		m = nv50_vec4_map(map, m, zval, lin, &fpi[1], &vpo[1]);
	}

	reg[0] += m - 4; /* adjust FFC0 id */
	reg[4] |= m << 8; /* set mid where 'normal' FP inputs start */

	for (i = 0; i < fp->cfg.in_nr; i++) {
		/* maybe even remove these from cfg.io */
		if (fp->cfg.in[i].sn == TGSI_SEMANTIC_POSITION ||
		    fp->cfg.in[i].sn == TGSI_SEMANTIC_FACE)
			continue;

		for (n = 0; n < vp->cfg.out_nr; ++n)
			if (vp->cfg.out[n].sn == fp->cfg.in[i].sn &&
			    vp->cfg.out[n].si == fp->cfg.in[i].si)
				break;

		m = nv50_vec4_map(map, m, zval, lin, &fp->cfg.in[i],
				  (n < vp->cfg.out_nr) ?
				  &vp->cfg.out[n] : &dummy);
	}
	/* PrimitiveID either is replaced by the system value, or
	 * written by the geometry shader into an output register
	 */
	if (fp->cfg.prim_id < 0x40) {
		map[m / 4] |= vp->cfg.prim_id << ((m % 4) * 8);
		reg[5] = m++;
	}

	if (nv50->rasterizer->pipe.point_size_per_vertex) {
		map[m / 4] |= vp->cfg.psiz << ((m % 4) * 8);
		reg[3] = (m++ << 4) | 1;
	}

	/* now fill the stateobj (at most 28 so_data)  */
	so = so_new(10, 54, 0);

	n = (m + 3) / 4;
	assert(m <= 32);
	if (vp->type == PIPE_SHADER_GEOMETRY) {
		so_method(so, tesla, NV50TCL_GP_RESULT_MAP_SIZE, 1);
		so_data  (so, m);
		so_method(so, tesla, NV50TCL_GP_RESULT_MAP(0), n);
		so_datap (so, map, n);
	} else {
		so_method(so, tesla, NV50TCL_VP_GP_BUILTIN_ATTR_EN, 1);
		so_data  (so, vp->cfg.regs[0]);

		so_method(so, tesla, NV50TCL_MAP_SEMANTIC_4, 1);
		so_data  (so, reg[5]);

		so_method(so, tesla, NV50TCL_VP_RESULT_MAP_SIZE, 1);
		so_data  (so, m);
		so_method(so, tesla, NV50TCL_VP_RESULT_MAP(0), n);
		so_datap (so, map, n);
	}

	so_method(so, tesla, NV50TCL_MAP_SEMANTIC_0, 4);
	so_datap (so, reg, 4);

	so_method(so, tesla, NV50TCL_FP_INTERPOLANT_CTRL, 1);
	so_data  (so, reg[4]);

	so_method(so, tesla, NV50TCL_NOPERSPECTIVE_BITMAP(0), 4);
	so_datap (so, lin, 4);

	if (nv50->rasterizer->pipe.sprite_coord_enable) {
		so_method(so, tesla, NV50TCL_POINT_SPRITE_CTRL, 1);
		so_data  (so,
			  nv50_pntc_replace(nv50, pcrd, (reg[4] >> 8) & 0xff));

		so_method(so, tesla, NV50TCL_POINT_COORD_REPLACE_MAP(0), 8);
		so_datap (so, pcrd, 8);
	}

	so_method(so, tesla, NV50TCL_GP_ENABLE, 1);
	so_data  (so, (vp->type == PIPE_SHADER_GEOMETRY) ? 1 : 0);

	so_ref(so, &nv50->state.fp_linkage);
	so_ref(NULL, &so);
}

static int
construct_vp_gp_mapping(uint32_t *map32, int m,
			struct nv50_program *vp, struct nv50_program *gp)
{
	uint8_t *map = (uint8_t *)map32;
	int i, j, c;

        for (i = 0; i < gp->cfg.in_nr; ++i) {
                uint8_t oid, mv = 0, mg = gp->cfg.in[i].mask;

                for (j = 0; j < vp->cfg.out_nr; ++j) {
                        if (vp->cfg.out[j].sn == gp->cfg.in[i].sn &&
                            vp->cfg.out[j].si == gp->cfg.in[i].si) {
				mv = vp->cfg.out[j].mask;
				oid = vp->cfg.out[j].hw;
                                break;
			}
		}

                for (c = 0; c < 4; ++c, mv >>= 1, mg >>= 1) {
			if (mg & mv & 1)
				map[m++] = oid;
			else
			if (mg & 1)
				map[m++] = (c == 3) ? 0x41 : 0x40;
                        oid += mv & 1;
                }
        }
	return m;
}

void
nv50_gp_linkage_validate(struct nv50_context *nv50)
{
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nouveau_stateobj *so;
	struct nv50_program *vp = nv50->vertprog;
	struct nv50_program *gp = nv50->geomprog;
	uint32_t map[16];
	int m = 0;

	if (!gp) {
		so_ref(NULL, &nv50->state.gp_linkage);
		return;
	}
	memset(map, 0, sizeof(map));

	m = construct_vp_gp_mapping(map, m, vp, gp);

	so = so_new(3, 24 - 3, 0);

	so_method(so, tesla, NV50TCL_VP_GP_BUILTIN_ATTR_EN, 1);
	so_data  (so, vp->cfg.regs[0] | gp->cfg.regs[0]);

	assert(m <= 32);
	so_method(so, tesla, NV50TCL_VP_RESULT_MAP_SIZE, 1);
	so_data  (so, m);

	m = (m + 3) / 4;
	so_method(so, tesla, NV50TCL_VP_RESULT_MAP(0), m);
	so_datap (so, map, m);

	so_ref(so, &nv50->state.gp_linkage);
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

	FREE(p->immd);
	nouveau_resource_free(&p->data[0]);

	p->translated = 0;
}
