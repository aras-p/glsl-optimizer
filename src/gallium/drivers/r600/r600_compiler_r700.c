/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include "r600_context.h"
#include "r700_sq.h"

static int r700_shader_cf_node_bytecode(struct r600_shader *rshader,
					struct r600_shader_node *rnode,
					unsigned *cid)
{
	unsigned id = *cid;

	if (rnode->nfetch) {
		rshader->bcode[id++] = S_SQ_CF_WORD0_ADDR(rnode->cf_addr >> 1);
		rshader->bcode[id++] = S_SQ_CF_WORD1_CF_INST(V_SQ_CF_WORD1_SQ_CF_INST_VTX) |
					S_SQ_CF_WORD1_COUNT(rnode->nfetch - 1);
	} else {
		rshader->bcode[id++] = S_SQ_CF_ALU_WORD0_ADDR(rnode->cf_addr >> 1);
		rshader->bcode[id++] = S_SQ_CF_ALU_WORD1_CF_INST(V_SQ_CF_ALU_WORD1_SQ_CF_INST_ALU) |
					S_SQ_CF_ALU_WORD1_BARRIER(1) |
					S_SQ_CF_ALU_WORD1_COUNT(rnode->nslot - 1);
	}
	*cid = id;
	return 0;
}

static int r700_shader_cf_output_bytecode(struct r600_shader *rshader,
						struct c_vector *v,
						unsigned *cid,
						unsigned end)
{
	unsigned gpr, chan;
	unsigned id = *cid;
	int r;

	r = r600_shader_find_gpr(rshader, v, 0, &gpr, &chan);
	if (r)
		return r;
	rshader->bcode[id + 0] = S_SQ_CF_ALLOC_EXPORT_WORD0_RW_GPR(gpr) |
				S_SQ_CF_ALLOC_EXPORT_WORD0_ELEM_SIZE(3);
	rshader->bcode[id + 1] = S_SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_X(0) |
		S_SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Y(1) |
		S_SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Z(2) |
		S_SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_W(3) |
		S_SQ_CF_ALLOC_EXPORT_WORD1_BARRIER(1) |
		S_SQ_CF_ALLOC_EXPORT_WORD1_CF_INST(V_SQ_CF_ALLOC_EXPORT_WORD1_SQ_CF_INST_EXPORT_DONE) |
		S_SQ_CF_ALLOC_EXPORT_WORD1_END_OF_PROGRAM(end);
	switch (v->name) {
	case C_SEMANTIC_POSITION:
		rshader->bcode[id + 0] |= S_SQ_CF_ALLOC_EXPORT_WORD0_ARRAY_BASE(60) |
			S_SQ_CF_ALLOC_EXPORT_WORD0_TYPE(V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_POS);
		break;
	case C_SEMANTIC_COLOR:
		if (rshader->cshader.type == C_PROGRAM_TYPE_VS) {
			rshader->output[rshader->noutput].gpr = gpr;
			rshader->output[rshader->noutput].sid = v->sid;
			rshader->output[rshader->noutput].name = v->name;
			rshader->bcode[id + 0] |= S_SQ_CF_ALLOC_EXPORT_WORD0_ARRAY_BASE(rshader->noutput++) |
				S_SQ_CF_ALLOC_EXPORT_WORD0_TYPE(V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PARAM);
		} else {
			rshader->bcode[id + 0] |= S_SQ_CF_ALLOC_EXPORT_WORD0_ARRAY_BASE(0) |
				S_SQ_CF_ALLOC_EXPORT_WORD0_TYPE(V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PIXEL);
		}
		break;
	case C_SEMANTIC_GENERIC:
		rshader->output[rshader->noutput].gpr = gpr;
		rshader->output[rshader->noutput].sid = v->sid;
		rshader->output[rshader->noutput].name = v->name;
		rshader->bcode[id + 0] |= S_SQ_CF_ALLOC_EXPORT_WORD0_ARRAY_BASE(rshader->noutput++) |
			S_SQ_CF_ALLOC_EXPORT_WORD0_TYPE(V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PARAM);
		break;
	default:
		fprintf(stderr, "%s:%d unsupported\n", __func__, __LINE__);
		return -EINVAL;
	}
	*cid = id + 2;
	return 0;
}

static int r700_shader_alu_bytecode(struct r600_shader *rshader,
					struct r600_shader_node *rnode,
					struct r600_shader_inst *alu,
					unsigned *cid)
{
	unsigned id = *cid;

	/* don't replace gpr by pv or ps for destination register */
	if (alu->is_op3) {
		rshader->bcode[id++] = S_SQ_ALU_WORD0_SRC0_SEL(alu->src[0].sel) |
					S_SQ_ALU_WORD0_SRC0_CHAN(alu->src[0].chan) |
					S_SQ_ALU_WORD0_SRC1_SEL(alu->src[1].sel) |
					S_SQ_ALU_WORD0_SRC1_CHAN(alu->src[1].chan) |
					S_SQ_ALU_WORD0_LAST(alu->last);
		rshader->bcode[id++] = S_SQ_ALU_WORD1_DST_GPR(alu->dst.sel) |
					S_SQ_ALU_WORD1_DST_CHAN(alu->dst.chan) |
					S_SQ_ALU_WORD1_OP3_SRC2_SEL(alu->src[2].sel) |
					S_SQ_ALU_WORD1_OP3_SRC2_CHAN(alu->src[2].chan) |
					S_SQ_ALU_WORD1_OP3_SRC2_NEG(alu->src[2].neg) |
					S_SQ_ALU_WORD1_OP3_ALU_INST(alu->inst) |
					S_SQ_ALU_WORD1_BANK_SWIZZLE(0);
	} else {
		rshader->bcode[id++] = S_SQ_ALU_WORD0_SRC0_SEL(alu->src[0].sel) |
					S_SQ_ALU_WORD0_SRC0_CHAN(alu->src[0].chan) |
					S_SQ_ALU_WORD0_SRC0_NEG(alu->src[0].neg) |
					S_SQ_ALU_WORD0_SRC1_SEL(alu->src[1].sel) |
					S_SQ_ALU_WORD0_SRC1_CHAN(alu->src[1].chan) |
					S_SQ_ALU_WORD0_SRC1_NEG(alu->src[1].neg) |
					S_SQ_ALU_WORD0_LAST(alu->last);
		rshader->bcode[id++] = S_SQ_ALU_WORD1_DST_GPR(alu->dst.sel) |
					S_SQ_ALU_WORD1_DST_CHAN(alu->dst.chan) |
					S_SQ_ALU_WORD1_OP2_SRC0_ABS(alu->src[0].abs) |
					S_SQ_ALU_WORD1_OP2_SRC1_ABS(alu->src[1].abs) |
					S_SQ_ALU_WORD1_OP2_WRITE_MASK(1) |
					S_SQ_ALU_WORD1_OP2_ALU_INST(alu->inst) |
					S_SQ_ALU_WORD1_BANK_SWIZZLE(0);
	}
	*cid = id;
	return 0;
}

int r700_shader_translate(struct r600_shader *rshader)
{
	struct c_shader *shader = &rshader->cshader;
	struct r600_shader_node *rnode;
	struct r600_shader_vfetch *vfetch;
	struct r600_shader_alu *alu;
	struct c_vector *v;
	unsigned id, i, end;
	int r;

	r = r600_shader_register(rshader);
	if (r) {
		fprintf(stderr, "%s %d register allocation failed\n", __FILE__, __LINE__);
		return r;
	}
	r = r600_shader_translate_rec(rshader, &shader->entry);
	if (r) {
		fprintf(stderr, "%s %d translation failed\n", __FILE__, __LINE__);
		return r;
	}
	r = r600_shader_legalize(rshader);
	if (r) {
		fprintf(stderr, "%s %d legalize failed\n", __FILE__, __LINE__);
		return r;
	}
	r600_shader_node_place(rshader);
	rshader->bcode = malloc(rshader->ndw * 4);
	if (rshader->bcode == NULL)
		return -ENOMEM;
	c_list_for_each(rnode, &rshader->nodes) {
		id = rnode->cf_addr;
		c_list_for_each(vfetch, &rnode->vfetch) {
			r = r600_shader_vfetch_bytecode(rshader, rnode, vfetch, &id);
			if (r)
				return r;
		}
		c_list_for_each(alu, &rnode->alu) {
			for (i = 0; i < alu->nalu; i++) {
				r = r700_shader_alu_bytecode(rshader, rnode, &alu->alu[i], &id);
				if (r)
					return r;
			}
			for (i = 0; i < alu->nliteral; i++) {
				rshader->bcode[id++] = alu->literal[i];
			}
		}
	}
	id = 0;
	c_list_for_each(rnode, &rshader->nodes) {
		r = r700_shader_cf_node_bytecode(rshader, rnode, &id);
		if (r)
			return r;
	}
	c_list_for_each(v, &rshader->cshader.files[C_FILE_OUTPUT].vectors) {
		end = 0;
		if (v->next == &rshader->cshader.files[C_FILE_OUTPUT].vectors)
			end = 1;
		r = r700_shader_cf_output_bytecode(rshader, v, &id, end);
		if (r)
			return r;
	}
	c_list_for_each(v, &rshader->cshader.files[C_FILE_INPUT].vectors) {
		rshader->input[rshader->ninput].gpr = rshader->ninput;
		rshader->input[rshader->ninput].sid = v->sid;
		rshader->input[rshader->ninput].name = v->name;
		rshader->ninput++;
	}
	return 0;
}
