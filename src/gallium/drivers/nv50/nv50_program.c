#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "pipe/p_inlines.h"

#include "pipe/p_shader_tokens.h"
#include "tgsi/util/tgsi_parse.h"
#include "tgsi/util/tgsi_util.h"

#include "nv50_context.h"
#include "nv50_state.h"

#define OP_MOV 0x001
#define OP_RCP 0x009
#define OP_ADD 0x00b
#define OP_MUL 0x00c
#define NV50_SU_MAX_TEMP 64

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
};

static void
alloc_reg(struct nv50_pc *pc, struct nv50_reg *reg)
{
	int i;

	if (reg->type != P_TEMP || reg->hw >= 0)
		return;

	for (i = 0; i < NV50_SU_MAX_TEMP; i++) {
		if (!(pc->r_temp[i])) {
			pc->r_temp[i] = reg;
			reg->hw = i;
			if (pc->p->cfg.vp.high_temp < (i + 1))
				pc->p->cfg.vp.high_temp = i + 1;
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

#if 0
static struct nv50_reg *
constant(struct nv50_pc *pc, int pipe, int c, float v)
{
	struct nv50_reg *r = CALLOC_STRUCT(nv50_reg);
	struct nv50_program *p = pc->p;
	struct nv50_program_data *pd;
	int idx;

	if (pipe >= 0) {
		for (idx = 0; idx < p->nr_consts; idx++) {
			if (p->consts[idx].index == pipe)
				return nv40_sr(NV40SR_CONST, idx);
		}
	}

	idx = p->nr_consts++;
	p->consts = realloc(p->consts, sizeof(*pd) * p->nr_consts);
	pd = &p->consts[idx];

	pd->index = pipe;
	pd->component = c;
	pd->value = v;
	return nv40_sr(NV40SR_CONST, idx);
}
#endif

static void
emit(struct nv50_pc *pc, unsigned op, struct nv50_reg *dst,
     struct nv50_reg *src0, struct nv50_reg *src1, struct nv50_reg *src2)
{
	struct nv50_program *p = pc->p;
	struct nv50_reg *tmp0 = NULL, *tmp = NULL, *tmp2 = NULL;
	unsigned inst[2] = { 0, 0 };

	/* Grr.. Fun restrictions on where attribs can be sourced from.. */
	if (src0 && (src0->type == P_CONST || src0->type == P_IMMD) &&
	    (op == 0xc || op == 0xe)) {
		tmp = src1;
		src1 = src0;
		src0 = tmp;
		tmp = NULL;
	}

	if (src1 && src1->type == P_ATTR) {
		tmp = alloc_temp(pc, dst);
		emit(pc, 1, tmp, src1, NULL, NULL);
		src1 = tmp;
	}

	if (src2 && src2->type == P_ATTR) {
		tmp2 = alloc_temp(pc, dst);
		emit(pc, 1, tmp2, src2, NULL, NULL);
		src2 = tmp2;
	}

	/* Get this out of the way first.  What type of opcode do we
	 * want/need to build?
	 */
	if ((op & 0x3f0) || dst->type == P_RESULT ||
	    (src0 && src0->type == P_ATTR) || src1 || src2)
		inst[0] |= 0x00000001;

	if (inst[0] & 0x00000001) {
		inst[0] |= ((op & 0xf) << 28);
		inst[1] |= ((op >> 4) << 26);

		alloc_reg(pc, dst);
		if (dst->type == P_RESULT)
			inst[1] |= 0x00000008;
		inst[0] |= (dst->hw << 2);

		if (src0) {
			if (src0->type == P_ATTR)
				inst[1] |= 0x00200000;
			else
			if (src0->type == P_CONST || src0->type == P_IMMD)
				assert(0);
			alloc_reg(pc, src0);
			inst[0] |= (src0->hw << 9);
		}

		if (src1) {
			if (src1->type == P_CONST || src1->type == P_IMMD) {
				if (src1->type == P_IMMD)
					inst[1] |= (NV50_CB_PMISC << 22);
				else
					inst[1] |= (NV50_CB_PVP << 22);
				inst[0] |= 0x00800000; /* src1 is const */
				/*XXX: does src1 come from "src2" now? */
				alloc_reg(pc, src1);
				inst[0] |= (src1->hw << 16);
			} else {
				alloc_reg(pc, src1);
				if (op == 0xc || op == 0xe)
					inst[0] |= (src1->hw << 16);
				else
					inst[1] |= (src1->hw << 14);
			}
		} else {
			inst[1] |= 0x0003c000; /*XXX FIXME */
		}

		if (src2) {
			if (src2->type == P_CONST || src2->type == P_IMMD) {
				if (src2->type == P_IMMD)
					inst[1] |= (NV50_CB_PMISC << 22);
				else
					inst[1] |= (NV50_CB_PVP << 22);
				inst[0] |= 0x01000000; /* src2 is const */
				inst[1] |= (src2->hw << 14);
			} else {
				alloc_reg(pc, src2);
				if (inst[0] & 0x00800000 || op ==0xe)
					inst[1] |= (src2->hw << 14);
				else
					inst[0] |= (src2->hw << 16);
			}
		}

		/*XXX: FIXME */
		if (op == 0xb || op == 0xc || op == 0x9 || op == 0xe) {
			/* 0x04000000 negates arg0 */
			/* 0x08000000 negates arg1 */
			/*XXX: true for !0xb also ? */
			inst[1] |= 0x00000780;
		} else {
			/* 0x04000000 == arg0 32 bit, otherwise 16 bit */
			inst[1] |= 0x04000780;
		}
	} else {
		inst[0] |= ((op & 0xf) << 28);

		alloc_reg(pc, dst);
		inst[0] |= (dst->hw << 2);

		if (src0) {
			alloc_reg(pc, src0);
			inst[0] |= (src0->hw << 9);
		}

		/*XXX: NFI if this even works - probably not.. */
		if (src1) {
			alloc_reg(pc, src1);
			inst[0] |= (src1->hw << 16);
		}
	}

	if (tmp0) free_temp(pc, tmp0);
	if (tmp) free_temp(pc, tmp);
	if (tmp2) free_temp(pc, tmp2);

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

static boolean
nv50_program_tx_insn(struct nv50_pc *pc, const union tgsi_full_token *tok)
{
	const struct tgsi_full_instruction *inst = &tok->FullInstruction;
	struct nv50_reg *dst[4], *src[3][4], *none = NULL, *tmp;
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
	case TGSI_OPCODE_ADD:
		for (c = 0; c < 4; c++) {
			if (mask & (1 << c)) {
				emit(pc, 0x0b, dst[c],
				     src[0][c], src[1][c], none);
			}
		}
		break;
	case TGSI_OPCODE_DP3:
		tmp = alloc_temp(pc, NULL);
		emit(pc, 0x0c, tmp, src[0][0], src[1][0], NULL);
		emit(pc, 0x0e, tmp, src[0][1], src[1][1], tmp);
		emit(pc, 0x0e, tmp, src[0][2], src[1][2], tmp);
		for (c = 0; c < 4; c++) {
			if (mask & (1 << c))
				emit(pc, 0x01, dst[c], tmp, none, none);
		}
		free_temp(pc, tmp);
		break;
	case TGSI_OPCODE_DP4:
		tmp = alloc_temp(pc, NULL);
		emit(pc, 0x0c, tmp, src[0][0], src[1][0], NULL);
		emit(pc, 0x0e, tmp, src[0][1], src[1][1], tmp);
		emit(pc, 0x0e, tmp, src[0][2], src[1][2], tmp);
		emit(pc, 0x0e, tmp, src[0][3], src[1][3], tmp);
		for (c = 0; c < 4; c++) {
			if (mask & (1 << c))
				emit(pc, 0x01, dst[c], tmp, none, none);
		}
		free_temp(pc, tmp);
		break;
	case TGSI_OPCODE_MAD:
		for (c = 0; c < 4; c++) {
			if (mask & (1 << c))
				emit(pc, 0x0e, dst[c],
				     src[0][c], src[1][c], src[2][c]);
		}
		break;
	case TGSI_OPCODE_MOV:
		for (c = 0; c < 4; c++) {
			if (mask & (1 << c))
				emit(pc, 0x01, dst[c], src[0][c], none, none);
		}
		break;
	case TGSI_OPCODE_MUL:
		for (c = 0; c < 4; c++) {
			if (mask & (1 << c))
				emit(pc, 0x0c, dst[c],
				     src[0][c], src[1][c], none);
		}
		break;
	case TGSI_OPCODE_RCP:
		for (c = 0; c < 4; c++) {
			if (mask & (1 << c))
				emit(pc, 0x09, dst[c],
				     src[0][c], none, none);
		}
		break;
	case TGSI_OPCODE_END:
		break;
	default:
		NOUVEAU_ERR("invalid opcode %d\n", inst->Instruction.Opcode);
		return FALSE;
	}

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

			pc->immd_nr++;
			pc->immd_buf = realloc(pc->immd_buf, 4 * pc->immd_nr *
							     sizeof(float));
			pc->immd_buf[4 * (pc->immd_nr - 1) + 0] =
				imm->u.ImmediateFloat32[0].Float;
			pc->immd_buf[4 * (pc->immd_nr - 1) + 1] =
				imm->u.ImmediateFloat32[1].Float;
			pc->immd_buf[4 * (pc->immd_nr - 1) + 2] =
				imm->u.ImmediateFloat32[2].Float;
			pc->immd_buf[4 * (pc->immd_nr - 1) + 3] =
				imm->u.ImmediateFloat32[3].Float;
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
		int aid = 0;

		pc->attr = calloc(pc->attr_nr * 4, sizeof(struct nv50_reg));
		if (!pc->attr)
			goto out_err;

		for (i = 0; i < pc->attr_nr; i++) {
			for (c = 0; c < 4; c++) {
				pc->p->cfg.vp.attr[aid/32] |= (1 << (aid % 32));
				pc->attr[i*4+c].type = P_ATTR;
				pc->attr[i*4+c].hw = aid++;
				pc->attr[i*4+c].index = i;
			}
		}
	}

	NOUVEAU_ERR("%d result regs\n", pc->result_nr);
	if (pc->result_nr) {
		int rid = 0;

		pc->result = calloc(pc->result_nr * 4, sizeof(struct nv50_reg));
		if (!pc->result)
			goto out_err;

		for (i = 0; i < pc->result_nr; i++) {
			for (c = 0; c < 4; c++) {
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
	pc->p->cfg.vp.high_temp = 4;

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
	struct tgsi_parse_context pc;

	tgsi_parse_init(&pc, p->pipe.tokens);

	if (pc.FullHeader.Processor.Processor == TGSI_PROCESSOR_FRAGMENT) {
		p->insns_nr = 8;
		p->insns = malloc(p->insns_nr * sizeof(unsigned));
		p->insns[0] = 0x80000000;
		p->insns[1] = 0x9000000c;
		p->insns[2] = 0x82010600;
		p->insns[3] = 0x82020604;
		p->insns[4] = 0x80030609;
		p->insns[5] = 0x00020780;
		p->insns[6] = 0x8004060d;
		p->insns[7] = 0x00020781;
	} else
	if (pc.FullHeader.Processor.Processor == TGSI_PROCESSOR_VERTEX) {
		int i;

		if (nv50_program_tx(p) == FALSE)
			assert(0);
		p->insns[p->insns_nr - 1] |= 0x00000001;

		for (i = 0; i < p->insns_nr; i++)
			NOUVEAU_ERR("%d 0x%08x\n", i, p->insns[i]);
	} else {
		NOUVEAU_ERR("invalid TGSI processor\n");
		tgsi_parse_free(&pc);
		return;
	}

	tgsi_parse_free(&pc);

	p->translated = TRUE;
}

void
nv50_vertprog_validate(struct nv50_context *nv50)
{
	struct pipe_winsys *ws = nv50->pipe.winsys;
	struct nouveau_winsys *nvws = nv50->screen->nvws;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nv50_program *p = nv50->vertprog;
	struct nouveau_stateobj *so;
	void *map;
	int i;

	if (!p->translated) {
		nv50_program_validate(nv50, p);
		if (!p->translated)
			assert(0);
	}

	if (!p->buffer)
		p->buffer = ws->buffer_create(ws, 0x100, 0, p->insns_nr * 4);
	map = ws->buffer_map(ws, p->buffer, PIPE_BUFFER_USAGE_CPU_WRITE);
	memcpy(map, p->insns, p->insns_nr * 4);
	ws->buffer_unmap(ws, p->buffer);

	for (i = 0; i < p->immd_nr; i++) {
		BEGIN_RING(tesla, 0x0f00, 2);
		OUT_RING  ((NV50_CB_PMISC << 16) | (i << 8));
		OUT_RING  (fui(p->immd[i]));
	}

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
	so_data  (so, p->cfg.vp.high_temp);
	so_method(so, tesla, 0x140c, 1);
	so_data  (so, 0); /* program start offset */
	so_emit(nv50->screen->nvws, so);
	so_ref(NULL, &so);
}

void
nv50_fragprog_validate(struct nv50_context *nv50)
{
	struct pipe_winsys *ws = nv50->pipe.winsys;
	struct nouveau_grobj *tesla = nv50->screen->tesla;
	struct nv50_program *p = nv50->fragprog;
	struct nouveau_stateobj *so;
	void *map;

	if (!p->translated) {
		nv50_program_validate(nv50, p);
		if (!p->translated)
			assert(0);
	}

	if (!p->buffer)
		p->buffer = ws->buffer_create(ws, 0x100, 0, p->insns_nr * 4);
	map = ws->buffer_map(ws, p->buffer, PIPE_BUFFER_USAGE_CPU_WRITE);
	memcpy(map, p->insns, p->insns_nr * 4);
	ws->buffer_unmap(ws, p->buffer);

	so = so_new(3, 2);
	so_method(so, tesla, NV50TCL_FP_ADDRESS_HIGH, 2);
	so_reloc (so, p->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
		  NOUVEAU_BO_HIGH, 0, 0);
	so_reloc (so, p->buffer, 0, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD |
		  NOUVEAU_BO_LOW, 0, 0);
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

