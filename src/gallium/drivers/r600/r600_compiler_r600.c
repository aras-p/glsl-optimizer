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
#include <util/u_format.h>
#include "r600_screen.h"
#include "r600_context.h"
#include "r600_sq.h"


struct r600_alu_instruction {
	unsigned			copcode;
	enum r600_instruction		instruction;
};

static int r600_shader_alu_translate(struct r600_shader *rshader,
					struct r600_shader_node *node,
					struct c_instruction *instruction);
struct r600_alu_instruction r600_alu_instruction[C_OPCODE_LAST];
struct r600_instruction_info r600_instruction_info[];

int r600_shader_insert_fetch(struct c_shader *shader)
{
	struct c_vector *vi, *vr, *v, *nv;
	struct c_instruction instruction;
	int r;

	if (shader->type != C_PROGRAM_TYPE_VS)
		return 0;
	vi = c_shader_vector_new(shader, C_FILE_INPUT, C_SEMANTIC_VERTEXID, -1);
	if (vi == NULL)
		return -ENOMEM;
	LIST_FOR_EACH_ENTRY_SAFE(v, nv, &shader->files[C_FILE_INPUT].vectors, head) {
		if (v == vi)
			continue;
		vr = c_shader_vector_new(shader, C_FILE_RESOURCE, C_SEMANTIC_GENERIC, -1);
		if (vr == NULL)
			return -ENOMEM;
		memset(&instruction, 0, sizeof(struct c_instruction));
		instruction.nop = 4;
		instruction.op[0].opcode = C_OPCODE_VFETCH;
		instruction.op[1].opcode = C_OPCODE_VFETCH;
		instruction.op[2].opcode = C_OPCODE_VFETCH;
		instruction.op[3].opcode = C_OPCODE_VFETCH;
		instruction.op[0].ninput = 2;
		instruction.op[1].ninput = 2;
		instruction.op[2].ninput = 2;
		instruction.op[3].ninput = 2;
		instruction.op[0].output.vector = v;
		instruction.op[1].output.vector = v;
		instruction.op[2].output.vector = v;
		instruction.op[3].output.vector = v;
		instruction.op[0].input[0].vector = vi;
		instruction.op[0].input[1].vector = vr;
		instruction.op[1].input[0].vector = vi;
		instruction.op[1].input[1].vector = vr;
		instruction.op[2].input[0].vector = vi;
		instruction.op[2].input[1].vector = vr;
		instruction.op[3].input[0].vector = vi;
		instruction.op[3].input[1].vector = vr;
		instruction.op[0].output.swizzle = C_SWIZZLE_X;
		instruction.op[1].output.swizzle = C_SWIZZLE_Y;
		instruction.op[2].output.swizzle = C_SWIZZLE_Z;
		instruction.op[3].output.swizzle = C_SWIZZLE_W;
		r = c_node_add_new_instruction_head(&shader->entry, &instruction);
		if (r)
			return r;
		LIST_DEL(&v->head);
		shader->files[C_FILE_INPUT].nvectors--;
		LIST_ADDTAIL(&v->head, &shader->files[C_FILE_TEMPORARY].vectors);
		shader->files[C_FILE_TEMPORARY].nvectors++;
		v->file = C_FILE_TEMPORARY;
	}
	return 0;
}

void r600_shader_cleanup(struct r600_shader *rshader)
{
	struct r600_shader_node *n, *nn;
	struct r600_shader_vfetch *vf, *nvf;
	struct r600_shader_alu *alu, *nalu;
	int i;

	if (rshader == NULL)
		return;
	if (rshader->gpr) {
		for (i = 0; i < rshader->nvector; i++) {
			free(rshader->gpr[i]);
		}
		free(rshader->gpr);
		rshader->gpr = NULL;
	}
	LIST_FOR_EACH_ENTRY_SAFE(n, nn, &rshader->nodes, head) {
		LIST_DEL(&n->head);
		LIST_FOR_EACH_ENTRY_SAFE(vf, nvf, &n->vfetch, head) {
			LIST_DEL(&vf->head);
			free(vf);
		}
		LIST_FOR_EACH_ENTRY_SAFE(alu, nalu, &n->alu,  head) {
			LIST_DEL(&alu->head);
			free(alu);
		}
		free(n);
	}
	free(rshader->bcode);
	return;
}

int r600_shader_vfetch_bytecode(struct r600_shader *rshader,
				struct r600_shader_node *rnode,
				struct r600_shader_vfetch *vfetch,
				unsigned *cid)
{
	unsigned id = *cid;

	vfetch->cf_addr = id;
	rshader->bcode[id++] = S_SQ_VTX_WORD0_BUFFER_ID(vfetch->src[1].sel) |
				S_SQ_VTX_WORD0_SRC_GPR(vfetch->src[0].sel) |
				S_SQ_VTX_WORD0_SRC_SEL_X(vfetch->src[0].sel) |
				S_SQ_VTX_WORD0_MEGA_FETCH_COUNT(0x1F);
	rshader->bcode[id++] = S_SQ_VTX_WORD1_DST_SEL_X(vfetch->dst[0].chan) |
				S_SQ_VTX_WORD1_DST_SEL_Y(vfetch->dst[1].chan) |
				S_SQ_VTX_WORD1_DST_SEL_Z(vfetch->dst[2].chan) |
				S_SQ_VTX_WORD1_DST_SEL_W(vfetch->dst[3].chan) |
				S_SQ_VTX_WORD1_USE_CONST_FIELDS(1) |
				S_SQ_VTX_WORD1_GPR_DST_GPR(vfetch->dst[0].sel);
	rshader->bcode[id++] = S_SQ_VTX_WORD2_MEGA_FETCH(1);
	rshader->bcode[id++] = 0;
	*cid = id;
	return 0;
}

int r600_shader_update(struct r600_shader *rshader, enum pipe_format *resource_format)
{
	struct r600_shader_node *rnode;
	struct r600_shader_vfetch *vfetch;
	unsigned i;

	memcpy(rshader->resource_format, resource_format,
		rshader->nresource * sizeof(enum pipe_format));
	LIST_FOR_EACH_ENTRY(rnode, &rshader->nodes, head) {
		LIST_FOR_EACH_ENTRY(vfetch, &rnode->vfetch, head) {
			const struct util_format_description *desc;
			i = vfetch->cf_addr + 1;
			rshader->bcode[i] &= C_SQ_VTX_WORD1_DST_SEL_X;
			rshader->bcode[i] &= C_SQ_VTX_WORD1_DST_SEL_Y;
			rshader->bcode[i] &= C_SQ_VTX_WORD1_DST_SEL_Z;
			rshader->bcode[i] &= C_SQ_VTX_WORD1_DST_SEL_W;
			desc = util_format_description(resource_format[vfetch->src[1].sel]);
			if (desc == NULL) {
				fprintf(stderr, "%s unknown format %d\n", __func__, resource_format[vfetch->src[1].sel]);
				continue;
			}
			/* WARNING so far TGSI swizzle match R600 ones */
			rshader->bcode[i] |= S_SQ_VTX_WORD1_DST_SEL_X(desc->swizzle[0]);
			rshader->bcode[i] |= S_SQ_VTX_WORD1_DST_SEL_Y(desc->swizzle[1]);
			rshader->bcode[i] |= S_SQ_VTX_WORD1_DST_SEL_Z(desc->swizzle[2]);
			rshader->bcode[i] |= S_SQ_VTX_WORD1_DST_SEL_W(desc->swizzle[3]);
		}
	}
	return 0;
}

int r600_shader_register(struct r600_shader *rshader)
{
	struct c_vector *v, *nv;
	unsigned tid, cid, rid, i;

	rshader->nvector = rshader->cshader.nvectors;
	rshader->gpr = calloc(rshader->nvector, sizeof(void*));
	if (rshader->gpr == NULL)
		return -ENOMEM;
	tid = 0;
	cid = 0;
	rid = 0;
	/* alloc input first */
	LIST_FOR_EACH_ENTRY(v, &rshader->cshader.files[C_FILE_INPUT].vectors, head) {
		nv = c_vector_new();
		if (nv == NULL) {
			return -ENOMEM;
		}
		memcpy(nv, v, sizeof(struct c_vector));
		nv->id = tid++;
		rshader->gpr[v->id] = nv;
	}
	for (i = 0; i < C_FILE_COUNT; i++) {
		if (i == C_FILE_INPUT || i == C_FILE_IMMEDIATE)
			continue;
		LIST_FOR_EACH_ENTRY(v, &rshader->cshader.files[i].vectors, head) {
			switch (v->file) {
			case C_FILE_OUTPUT:
			case C_FILE_TEMPORARY:
				nv = c_vector_new();
				if (nv == NULL) {
					return -ENOMEM;
				}
				memcpy(nv, v, sizeof(struct c_vector));
				nv->id = tid++;
				rshader->gpr[v->id] = nv;
				break;
			case C_FILE_CONSTANT:
				nv = c_vector_new();
				if (nv == NULL) {
					return -ENOMEM;
				}
				memcpy(nv, v, sizeof(struct c_vector));
				nv->id = (cid++) + 256;
				rshader->gpr[v->id] = nv;
				break;
			case C_FILE_RESOURCE:
				nv = c_vector_new();
				if (nv == NULL) {
					return -ENOMEM;
				}
				memcpy(nv, v, sizeof(struct c_vector));
				nv->id = (rid++);
				rshader->gpr[v->id] = nv;
				break;
			default:
				fprintf(stderr, "%s:%d unsupported file %d\n", __func__, __LINE__, v->file);
				return -EINVAL;
			}
		}
	}
	rshader->ngpr = tid;
	rshader->nconstant = cid;
	rshader->nresource = rid;
	return 0;
}

int r600_shader_find_gpr(struct r600_shader *rshader, struct c_vector *v, unsigned swizzle,
			struct r600_shader_operand *operand)
{
	struct c_vector *tmp;

	/* Values [0,127] correspond to GPR[0..127]. 
	 * Values [256,511] correspond to cfile constants c[0..255]. 
	 * Other special values are shown in the list below.
	 * 248	SQ_ALU_SRC_0: special constant 0.0.
	 * 249	SQ_ALU_SRC_1: special constant 1.0 float.
	 * 250	SQ_ALU_SRC_1_INT: special constant 1 integer.
	 * 251	SQ_ALU_SRC_M_1_INT: special constant -1 integer.
	 * 252	SQ_ALU_SRC_0_5: special constant 0.5 float.
	 * 253	SQ_ALU_SRC_LITERAL: literal constant.
	 * 254	SQ_ALU_SRC_PV: previous vector result.
	 * 255	SQ_ALU_SRC_PS: previous scalar result.
	 */
	operand->vector = v;
	operand->sel = 248;
	operand->chan = 0;
	operand->neg = 0;
	operand->abs = 0;
	if (v == NULL)
		return 0;
	if (v->file == C_FILE_IMMEDIATE) {
		operand->sel = 253;
	} else {
		tmp = rshader->gpr[v->id];
		if (tmp == NULL) {
			fprintf(stderr, "%s %d unknown register\n", __FILE__, __LINE__);
			return -EINVAL;
		}
		operand->sel = tmp->id;
	}
	operand->chan = swizzle;
	switch (swizzle) {
	case C_SWIZZLE_X:
	case C_SWIZZLE_Y:
	case C_SWIZZLE_Z:
	case C_SWIZZLE_W:
		break;
	case C_SWIZZLE_0:
		operand->sel = 248;
		operand->chan = 0;
		break;
	case C_SWIZZLE_1:
		operand->sel = 249;
		operand->chan = 0;
		break;
	default:
		fprintf(stderr, "%s %d invalid swizzle %d\n", __FILE__, __LINE__, swizzle);
		return -EINVAL;
	}
	return 0;
}

static struct r600_shader_node *r600_shader_new_node(struct r600_shader *rshader, struct c_node *node)
{
	struct r600_shader_node *rnode;

	rnode = CALLOC_STRUCT(r600_shader_node);
	if (rnode == NULL)
		return NULL;
	rnode->node = node;
	LIST_INITHEAD(&rnode->vfetch);
	LIST_INITHEAD(&rnode->alu);
	LIST_ADDTAIL(&rnode->head, &rshader->nodes);
	return rnode;
}

static int r600_shader_add_vfetch(struct r600_shader *rshader,
				struct r600_shader_node *node,
				struct c_instruction *instruction)
{
	struct r600_shader_vfetch *vfetch;
	struct r600_shader_node *rnode;
	int r;

	if (instruction == NULL)
		return 0;
	if (instruction->op[0].opcode != C_OPCODE_VFETCH)
		return 0;
	if (!LIST_IS_EMPTY(&node->alu)) {
		rnode = r600_shader_new_node(rshader, node->node);
		if (rnode == NULL)
			return -ENOMEM;
		node = rnode;
	}
	vfetch = calloc(1, sizeof(struct r600_shader_vfetch));
	if (vfetch == NULL)
		return -ENOMEM;
	r = r600_shader_find_gpr(rshader, instruction->op[0].output.vector, 0, &vfetch->dst[0]);
	if (r)
		return r;
	r = r600_shader_find_gpr(rshader, instruction->op[0].input[0].vector, 0, &vfetch->src[0]);
	if (r)
		return r;
	r = r600_shader_find_gpr(rshader, instruction->op[0].input[1].vector, 0, &vfetch->src[1]);
	if (r)
		return r;
	vfetch->dst[0].chan = C_SWIZZLE_X;
	vfetch->dst[1].chan = C_SWIZZLE_Y;
	vfetch->dst[2].chan = C_SWIZZLE_Z;
	vfetch->dst[3].chan = C_SWIZZLE_W;
	LIST_ADDTAIL(&vfetch->head, &node->vfetch);
	node->nslot += 2;
	return 0;
}

static int r600_node_translate(struct r600_shader *rshader, struct c_node *node)
{
	struct c_instruction *instruction;
	struct r600_shader_node *rnode;
	int r;

	rnode = r600_shader_new_node(rshader, node);
	if (rnode == NULL)
		return -ENOMEM;
	LIST_FOR_EACH_ENTRY(instruction, &node->insts, head) {
		switch (instruction->op[0].opcode) {
		case C_OPCODE_VFETCH:
			r = r600_shader_add_vfetch(rshader, rnode, instruction);
			if (r) {
				fprintf(stderr, "%s %d vfetch failed\n", __func__, __LINE__);
				return r;
			}
			break;
		default:
			r = r600_shader_alu_translate(rshader, rnode, instruction);
			if (r) {
				fprintf(stderr, "%s %d alu failed\n", __func__, __LINE__);
				return r;
			}
			break;
		}
	}
	return 0;
}

int r600_shader_translate_rec(struct r600_shader *rshader, struct c_node *node)
{
	struct c_node_link *link;
	int r;

	if (node->opcode == C_OPCODE_END)
		return 0;
	r = r600_node_translate(rshader, node);
	if (r)
		return r;
	LIST_FOR_EACH_ENTRY(link, &node->childs, head) {
		r = r600_shader_translate_rec(rshader, link->node);
		if (r)
			return r;
	}
	return 0;
}

static struct r600_shader_alu *r600_shader_insert_alu(struct r600_shader *rshader, struct r600_shader_node *node)
{
	struct r600_shader_alu *alu;

	alu = CALLOC_STRUCT(r600_shader_alu);
	if (alu == NULL)
		return NULL;
	alu->alu[0].inst = INST_NOP;
	alu->alu[1].inst = INST_NOP;
	alu->alu[2].inst = INST_NOP;
	alu->alu[3].inst = INST_NOP;
	alu->alu[4].inst = INST_NOP;
	alu->alu[0].opcode = V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP;
	alu->alu[1].opcode = V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP;
	alu->alu[2].opcode = V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP;
	alu->alu[3].opcode = V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP;
	alu->alu[4].opcode = V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP;
	LIST_ADDTAIL(&alu->head, &node->alu);
	return alu;
}

static int r600_shader_alu_translate(struct r600_shader *rshader,
					struct r600_shader_node *node,
					struct c_instruction *instruction)
{
	struct r600_shader_node *rnode;
	struct r600_shader_alu *alu;
	int i, j, r, comp, litteral_lastcomp = -1;

	if (!LIST_IS_EMPTY(&node->vfetch)) {
		rnode = r600_shader_new_node(rshader, node->node);
		if (rnode == NULL) {
			fprintf(stderr, "%s %d new node failed\n", __func__, __LINE__);
			return -ENOMEM;
		}
		node = rnode;
	}

	/* initialize alu */
	alu = r600_shader_insert_alu(rshader, node);

	/* check special operation like lit */

	/* go through operation */
	for (i = 0; i < instruction->nop; i++) {
		struct r600_alu_instruction *ainfo = &r600_alu_instruction[instruction->op[i].opcode];
		struct r600_instruction_info *iinfo = &r600_instruction_info[ainfo->instruction];
		unsigned comp;

		/* check that output is a valid component */
		comp = instruction->op[i].output.swizzle;
		switch (comp) {
		case C_SWIZZLE_X:
		case C_SWIZZLE_Y:
		case C_SWIZZLE_Z:
		case C_SWIZZLE_W:
			break;
		case C_SWIZZLE_0:
		case C_SWIZZLE_1:
		default:
			fprintf(stderr, "%s %d invalid output\n", __func__, __LINE__);
			return -EINVAL;
		}
		alu->alu[comp].inst = ainfo->instruction;
		alu->alu[comp].opcode = iinfo->opcode;
		alu->alu[comp].is_op3 = iinfo->is_op3;
		for (j = 0; j < instruction->op[i].ninput; j++) {
			r = r600_shader_find_gpr(rshader, instruction->op[i].input[j].vector,
					instruction->op[i].input[j].swizzle, &alu->alu[comp].src[j]);
			if (r) {
				fprintf(stderr, "%s %d register failed\n", __FILE__, __LINE__);
				return r;
			}
			if (instruction->op[i].input[j].vector->file == C_FILE_IMMEDIATE) {
				r = instruction->op[i].input[j].swizzle;
				switch (r) {
				case C_SWIZZLE_X:
				case C_SWIZZLE_Y:
				case C_SWIZZLE_Z:
				case C_SWIZZLE_W:
					break;
				case C_SWIZZLE_0:
				case C_SWIZZLE_1:
				default:
					fprintf(stderr, "%s %d invalid input\n", __func__, __LINE__);
					return -EINVAL;
				}
				alu->literal[r] = instruction->op[i].input[j].vector->channel[r]->value;
				if (r > litteral_lastcomp) {
					litteral_lastcomp = r;
				}
			}
		}
		r = r600_shader_find_gpr(rshader, instruction->op[i].output.vector,
				instruction->op[i].output.swizzle, &alu->alu[comp].dst);
		if (r) {
			fprintf(stderr, "%s %d register failed\n", __FILE__, __LINE__);
			return r;
		}
	}
	switch (litteral_lastcomp) {
	case 0:
	case 1:
		alu->nliteral = 2;
		break;
	case 2:
	case 3:
		alu->nliteral = 4;
		break;
	case -1:
	default:
		break;
	}
printf("nliteral: %d\n", alu->nliteral);
	for (i = instruction->nop; i >= 0; i--) {
		if (alu->alu[i].inst != INST_NOP) {
			alu->alu[i].last = 1;
			alu->nalu = i + 1;
			break;
		}
	}
	return 0;
}

void r600_shader_node_place(struct r600_shader *rshader)
{
	struct r600_shader_node *node, *nnode;
	struct r600_shader_alu *alu, *nalu;
	struct r600_shader_vfetch *vfetch, *nvfetch;
	unsigned cf_id = 0, cf_addr = 0;

	rshader->ncf = 0;
	rshader->nslot = 0;
	LIST_FOR_EACH_ENTRY_SAFE(node, nnode, &rshader->nodes, head) {
		LIST_FOR_EACH_ENTRY_SAFE(alu, nalu, &node->alu, head) {
			node->nslot += alu->nalu;
			node->nslot += alu->nliteral >> 1;
		}
		node->nfetch = 0;
		LIST_FOR_EACH_ENTRY_SAFE(vfetch, nvfetch, &node->vfetch, head) {
			node->nslot += 2;
			node->nfetch += 1;
		}
		if (!LIST_IS_EMPTY(&node->vfetch)) {
			/* fetch node need to be 16 bytes aligned*/
			cf_addr += 1;
			cf_addr &= 0xFFFFFFFEUL;
		}
		node->cf_id = cf_id;
		node->cf_addr = cf_addr;
		cf_id += 2;
		cf_addr += node->nslot * 2;
		rshader->ncf++;
	}
	rshader->nslot = cf_addr;
	LIST_FOR_EACH_ENTRY_SAFE(node, nnode, &rshader->nodes, head) {
		node->cf_addr += cf_id * 2;
	}
	rshader->ncf += rshader->cshader.files[C_FILE_OUTPUT].nvectors;
	rshader->ndw = rshader->ncf * 2 + rshader->nslot * 2;
}

int r600_shader_legalize(struct r600_shader *rshader)
{
	return 0;
}


static int r600_cshader_legalize_rec(struct c_shader *shader, struct c_node *node)
{
	struct c_node_link *link;
	struct c_instruction *i;
	struct c_operand operand;
	unsigned k;
	int r;

	LIST_FOR_EACH_ENTRY(i, &node->insts, head) {
		for (k = 0; k < i->nop; k++) {
			switch (i->op[k].opcode) {
			case C_OPCODE_SLT:
				i->op[k].opcode = C_OPCODE_SGT;
				memcpy(&operand, &i->op[k].input[0], sizeof(struct c_operand));
				memcpy(&i->op[k].input[0], &i->op[k].input[1], sizeof(struct c_operand));
				memcpy(&i->op[k].input[1], &operand, sizeof(struct c_operand));
				break;
			default:
				break;
			}
		}
	}
	LIST_FOR_EACH_ENTRY(link, &node->childs, head) {
		r = r600_cshader_legalize_rec(shader, link->node);
		if (r) {
			return r;
		}
	}
	return 0;
}

int r600_cshader_legalize(struct c_shader *shader)
{
	return r600_cshader_legalize_rec(shader, &shader->entry);
}


struct r600_instruction_info r600_instruction_info[] = {
	{INST_ADD,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD,			0, 0},
	{INST_MUL,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MUL,			0, 0},
	{INST_MUL_IEEE,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MUL_IEEE,		0, 0},
	{INST_MAX,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX,			0, 0},
	{INST_MIN,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MIN,			0, 0},
	{INST_MAX_DX10,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX_DX10,		0, 0},
	{INST_MIN_DX10,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MIN_DX10,		0, 0},
	{INST_SETE,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETE,			0, 0},
	{INST_SETGT,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGT,			0, 0},
	{INST_SETGE,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGE,			0, 0},
	{INST_SETNE,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETNE,			0, 0},
	{INST_SETE_DX10,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETE_DX10,		0, 0},
	{INST_SETGT_DX10,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGT_DX10,		0, 0},
	{INST_SETGE_DX10,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGE_DX10,		0, 0},
	{INST_SETNE_DX10,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETNE_DX10,		0, 0},
	{INST_FRACT,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FRACT,			0, 0},
	{INST_TRUNC,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_TRUNC,			0, 0},
	{INST_CEIL,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_CEIL,			0, 0},
	{INST_RNDNE,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RNDNE,			0, 0},
	{INST_FLOOR,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLOOR,			0, 0},
	{INST_MOVA,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOVA,			0, 0},
	{INST_MOVA_FLOOR,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOVA_FLOOR,		0, 0},
	{INST_MOVA_INT,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOVA_INT,		0, 0},
	{INST_MOV,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV,			0, 0},
	{INST_NOP,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOP,			0, 0},
	{INST_PRED_SETGT_UINT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGT_UINT,		0, 0},
	{INST_PRED_SETGE_UINT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGE_UINT,		0, 0},
	{INST_PRED_SETE,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETE,		0, 0},
	{INST_PRED_SETGT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGT,		0, 0},
	{INST_PRED_SETGE,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGE,		0, 0},
	{INST_PRED_SETNE,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETNE,		0, 0},
	{INST_PRED_SET_INV,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SET_INV,		0, 0},
	{INST_PRED_SET_POP,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SET_POP,		0, 0},
	{INST_PRED_SET_CLR,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SET_CLR,		0, 0},
	{INST_PRED_SET_RESTORE,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SET_RESTORE,	0, 0},
	{INST_PRED_SETE_PUSH,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETE_PUSH,		0, 0},
	{INST_PRED_SETGT_PUSH,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGT_PUSH,		0, 0},
	{INST_PRED_SETGE_PUSH,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGE_PUSH,		0, 0},
	{INST_PRED_SETNE_PUSH,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETNE_PUSH,		0, 0},
	{INST_KILLE,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLE,			0, 0},
	{INST_KILLGT,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGT,			0, 0},
	{INST_KILLGE,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGE,			0, 0},
	{INST_KILLNE,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLNE,			0, 0},
	{INST_AND_INT,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_AND_INT,			0, 0},
	{INST_OR_INT,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_OR_INT,			0, 0},
	{INST_XOR_INT,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_XOR_INT,			0, 0},
	{INST_NOT_INT,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_NOT_INT,			0, 0},
	{INST_ADD_INT,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD_INT,			0, 0},
	{INST_SUB_INT,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SUB_INT,			0, 0},
	{INST_MAX_INT,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX_INT,			0, 0},
	{INST_MIN_INT,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MIN_INT,			0, 0},
	{INST_MAX_UINT,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX_UINT,		0, 0},
	{INST_MIN_UINT,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MIN_UINT,		0, 0},
	{INST_SETE_INT,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETE_INT,		0, 0},
	{INST_SETGT_INT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGT_INT,		0, 0},
	{INST_SETGE_INT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGE_INT,		0, 0},
	{INST_SETNE_INT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETNE_INT,		0, 0},
	{INST_SETGT_UINT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGT_UINT,		0, 0},
	{INST_SETGE_UINT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGE_UINT,		0, 0},
	{INST_KILLGT_UINT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGT_UINT,		0, 0},
	{INST_KILLGE_UINT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGE_UINT,		0, 0},
	{INST_PRED_SETE_INT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETE_INT,		0, 0},
	{INST_PRED_SETGT_INT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGT_INT,		0, 0},
	{INST_PRED_SETGE_INT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGE_INT,		0, 0},
	{INST_PRED_SETNE_INT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETNE_INT,		0, 0},
	{INST_KILLE_INT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLE_INT,		0, 0},
	{INST_KILLGT_INT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGT_INT,		0, 0},
	{INST_KILLGE_INT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLGE_INT,		0, 0},
	{INST_KILLNE_INT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_KILLNE_INT,		0, 0},
	{INST_PRED_SETE_PUSH_INT,	V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETE_PUSH_INT,	0, 0},
	{INST_PRED_SETGT_PUSH_INT,	V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGT_PUSH_INT,	0, 0},
	{INST_PRED_SETGE_PUSH_INT,	V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETGE_PUSH_INT,	0, 0},
	{INST_PRED_SETNE_PUSH_INT,	V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETNE_PUSH_INT,	0, 0},
	{INST_PRED_SETLT_PUSH_INT,	V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETLT_PUSH_INT,	0, 0},
	{INST_PRED_SETLE_PUSH_INT,	V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_PRED_SETLE_PUSH_INT,	0, 0},
	{INST_DOT4,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4,			0, 0},
	{INST_DOT4_IEEE,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4_IEEE,		0, 0},
	{INST_CUBE,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_CUBE,			0, 0},
	{INST_MAX4,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX4,			0, 0},
	{INST_MOVA_GPR_INT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOVA_GPR_INT,		0, 0},
	{INST_EXP_IEEE,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE,		1, 0},
	{INST_LOG_CLAMPED,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_CLAMPED,		1, 0},
	{INST_LOG_IEEE,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_IEEE,		1, 0},
	{INST_RECIP_CLAMPED,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_CLAMPED,		1, 0},
	{INST_RECIP_FF,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_FF,		1, 0},
	{INST_RECIP_IEEE,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_IEEE,		1, 0},
	{INST_RECIPSQRT_CLAMPED,	V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIPSQRT_CLAMPED,	1, 0},
	{INST_RECIPSQRT_FF,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIPSQRT_FF,		1, 0},
	{INST_RECIPSQRT_IEEE,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIPSQRT_IEEE,		1, 0},
	{INST_SQRT_IEEE,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SQRT_IEEE,		1, 0},
	{INST_FLT_TO_INT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLT_TO_INT,		1, 0},
	{INST_INT_TO_FLT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_INT_TO_FLT,		1, 0},
	{INST_UINT_TO_FLT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_UINT_TO_FLT,		1, 0},
	{INST_SIN,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SIN,			1, 0},
	{INST_COS,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_COS,			1, 0},
	{INST_ASHR_INT,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ASHR_INT,		1, 0},
	{INST_LSHR_INT,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LSHR_INT,		1, 0},
	{INST_LSHL_INT,			V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LSHL_INT,		1, 0},
	{INST_MULLO_INT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULLO_INT,		1, 0},
	{INST_MULHI_INT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULHI_INT,		1, 0},
	{INST_MULLO_UINT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULLO_UINT,		1, 0},
	{INST_MULHI_UINT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MULHI_UINT,		1, 0},
	{INST_RECIP_INT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_INT,		1, 0},
	{INST_RECIP_UINT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIP_UINT,		1, 0},
	{INST_FLT_TO_UINT,		V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_FLT_TO_UINT,		1, 0},
	{INST_MUL_LIT,			V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MUL_LIT,			1, 1},
	{INST_MUL_LIT_M2,		V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MUL_LIT_M2,		1, 1},
	{INST_MUL_LIT_M4,		V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MUL_LIT_M4,		1, 1},
	{INST_MUL_LIT_D2,		V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MUL_LIT_D2,		1, 1},
	{INST_MULADD,			V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD,			0, 1},
	{INST_MULADD_M2,		V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD_M2,		0, 1},
	{INST_MULADD_M4,		V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD_M4,		0, 1},
	{INST_MULADD_D2,		V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD_D2,		0, 1},
	{INST_MULADD_IEEE,		V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD_IEEE,		0, 1},
	{INST_MULADD_IEEE_M2,		V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD_IEEE_M2,		0, 1},
	{INST_MULADD_IEEE_M4,		V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD_IEEE_M4,		0, 1},
	{INST_MULADD_IEEE_D2,		V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD_IEEE_D2,		0, 1},
	{INST_CNDE,			V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_CNDE,			0, 1},
	{INST_CNDGT,			V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_CNDGT,			0, 1},
	{INST_CNDGE,			V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_CNDGE,			0, 1},
	{INST_CNDE_INT,			V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_CNDE_INT,		0, 1},
	{INST_CNDGT_INT,		V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_CNDGT_INT,		0, 1},
	{INST_CNDGE_INT,		V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_CNDGE_INT,		0, 1},
};

struct r600_alu_instruction r600_alu_instruction[C_OPCODE_LAST] = {
	{C_OPCODE_NOP,		INST_NOP},
	{C_OPCODE_MOV,		INST_MOV},
	{C_OPCODE_LIT,		INST_NOP},
	{C_OPCODE_RCP,		INST_RECIP_IEEE},
	{C_OPCODE_RSQ,		INST_RECIPSQRT_IEEE},
	{C_OPCODE_EXP,		INST_EXP_IEEE},
	{C_OPCODE_LOG,		INST_LOG_IEEE},
	{C_OPCODE_MUL,		INST_MUL},
	{C_OPCODE_ADD,		INST_ADD},
	{C_OPCODE_DP3,		INST_DOT4},
	{C_OPCODE_DP4,		INST_DOT4},
	{C_OPCODE_DST,		INST_NOP},
	{C_OPCODE_MIN,		INST_MIN},
	{C_OPCODE_MAX,		INST_MAX},
	{C_OPCODE_SLT,		INST_NOP},
	{C_OPCODE_SGE,		INST_NOP},
	{C_OPCODE_MAD,		INST_MULADD},
	{C_OPCODE_SUB,		INST_COUNT},
	{C_OPCODE_LRP,		INST_NOP},
	{C_OPCODE_CND,		INST_NOP},
	{20,			INST_NOP},
	{C_OPCODE_DP2A,		INST_NOP},
	{22,			INST_NOP},
	{23,			INST_NOP},
	{C_OPCODE_FRC,		INST_NOP},
	{C_OPCODE_CLAMP,	INST_NOP},
	{C_OPCODE_FLR,		INST_NOP},
	{C_OPCODE_ROUND,	INST_NOP},
	{C_OPCODE_EX2,		INST_NOP},
	{C_OPCODE_LG2,		INST_NOP},
	{C_OPCODE_POW,		INST_NOP},
	{C_OPCODE_XPD,		INST_NOP},
	{32,			INST_NOP},
	{C_OPCODE_ABS,		INST_COUNT},
	{C_OPCODE_RCC,		INST_NOP},
	{C_OPCODE_DPH,		INST_NOP},
	{C_OPCODE_COS,		INST_COS},
	{C_OPCODE_DDX,		INST_NOP},
	{C_OPCODE_DDY,		INST_NOP},
	{C_OPCODE_KILP,		INST_NOP},
	{C_OPCODE_PK2H,		INST_NOP},
	{C_OPCODE_PK2US,	INST_NOP},
	{C_OPCODE_PK4B,		INST_NOP},
	{C_OPCODE_PK4UB,	INST_NOP},
	{C_OPCODE_RFL,		INST_NOP},
	{C_OPCODE_SEQ,		INST_NOP},
	{C_OPCODE_SFL,		INST_NOP},
	{C_OPCODE_SGT,		INST_SETGT},
	{C_OPCODE_SIN,		INST_SIN},
	{C_OPCODE_SLE,		INST_NOP},
	{C_OPCODE_SNE,		INST_NOP},
	{C_OPCODE_STR,		INST_NOP},
	{C_OPCODE_TEX,		INST_NOP},
	{C_OPCODE_TXD,		INST_NOP},
	{C_OPCODE_TXP,		INST_NOP},
	{C_OPCODE_UP2H,		INST_NOP},
	{C_OPCODE_UP2US,	INST_NOP},
	{C_OPCODE_UP4B,		INST_NOP},
	{C_OPCODE_UP4UB,	INST_NOP},
	{C_OPCODE_X2D,		INST_NOP},
	{C_OPCODE_ARA,		INST_NOP},
	{C_OPCODE_ARR,		INST_NOP},
	{C_OPCODE_BRA,		INST_NOP},
	{C_OPCODE_CAL,		INST_NOP},
	{C_OPCODE_RET,		INST_NOP},
	{C_OPCODE_SSG,		INST_NOP},
	{C_OPCODE_CMP,		INST_NOP},
	{C_OPCODE_SCS,		INST_NOP},
	{C_OPCODE_TXB,		INST_NOP},
	{C_OPCODE_NRM,		INST_NOP},
	{C_OPCODE_DIV,		INST_NOP},
	{C_OPCODE_DP2,		INST_NOP},
	{C_OPCODE_TXL,		INST_NOP},
	{C_OPCODE_BRK,		INST_NOP},
	{C_OPCODE_IF,		INST_NOP},
	{C_OPCODE_BGNFOR,	INST_NOP},
	{C_OPCODE_REP,		INST_NOP},
	{C_OPCODE_ELSE,		INST_NOP},
	{C_OPCODE_ENDIF,	INST_NOP},
	{C_OPCODE_ENDFOR,	INST_NOP},
	{C_OPCODE_ENDREP,	INST_NOP},
	{C_OPCODE_PUSHA,	INST_NOP},
	{C_OPCODE_POPA,		INST_NOP},
	{C_OPCODE_CEIL,		INST_NOP},
	{C_OPCODE_I2F,		INST_NOP},
	{C_OPCODE_NOT,		INST_NOP},
	{C_OPCODE_TRUNC,	INST_NOP},
	{C_OPCODE_SHL,		INST_NOP},
	{88,			INST_NOP},
	{C_OPCODE_AND,		INST_NOP},
	{C_OPCODE_OR,		INST_NOP},
	{C_OPCODE_MOD,		INST_NOP},
	{C_OPCODE_XOR,		INST_NOP},
	{C_OPCODE_SAD,		INST_NOP},
	{C_OPCODE_TXF,		INST_NOP},
	{C_OPCODE_TXQ,		INST_NOP},
	{C_OPCODE_CONT,		INST_NOP},
	{C_OPCODE_EMIT,		INST_NOP},
	{C_OPCODE_ENDPRIM,	INST_NOP},
	{C_OPCODE_BGNLOOP,	INST_NOP},
	{C_OPCODE_BGNSUB,	INST_NOP},
	{C_OPCODE_ENDLOOP,	INST_NOP},
	{C_OPCODE_ENDSUB,	INST_NOP},
	{103,			INST_NOP},
	{104,			INST_NOP},
	{105,			INST_NOP},
	{106,			INST_NOP},
	{107,			INST_NOP},
	{108,			INST_NOP},
	{109,			INST_NOP},
	{110,			INST_NOP},
	{111,			INST_NOP},
	{C_OPCODE_NRM4,		INST_NOP},
	{C_OPCODE_CALLNZ,	INST_NOP},
	{C_OPCODE_IFC,		INST_NOP},
	{C_OPCODE_BREAKC,	INST_NOP},
	{C_OPCODE_KIL,		INST_NOP},
	{C_OPCODE_END,		INST_NOP},
	{118,			INST_NOP},
	{C_OPCODE_F2I,		INST_NOP},
	{C_OPCODE_IDIV,		INST_NOP},
	{C_OPCODE_IMAX,		INST_NOP},
	{C_OPCODE_IMIN,		INST_NOP},
	{C_OPCODE_INEG,		INST_NOP},
	{C_OPCODE_ISGE,		INST_NOP},
	{C_OPCODE_ISHR,		INST_NOP},
	{C_OPCODE_ISLT,		INST_NOP},
	{C_OPCODE_F2U,		INST_NOP},
	{C_OPCODE_U2F,		INST_NOP},
	{C_OPCODE_UADD,		INST_NOP},
	{C_OPCODE_UDIV,		INST_NOP},
	{C_OPCODE_UMAD,		INST_NOP},
	{C_OPCODE_UMAX,		INST_NOP},
	{C_OPCODE_UMIN,		INST_NOP},
	{C_OPCODE_UMOD,		INST_NOP},
	{C_OPCODE_UMUL,		INST_NOP},
	{C_OPCODE_USEQ,		INST_NOP},
	{C_OPCODE_USGE,		INST_NOP},
	{C_OPCODE_USHR,		INST_NOP},
	{C_OPCODE_USLT,		INST_NOP},
	{C_OPCODE_USNE,		INST_NOP},
	{C_OPCODE_SWITCH,	INST_NOP},
	{C_OPCODE_CASE,		INST_NOP},
	{C_OPCODE_DEFAULT,	INST_NOP},
	{C_OPCODE_ENDSWITCH,	INST_NOP},
	{C_OPCODE_VFETCH,	INST_NOP},
	{C_OPCODE_ENTRY,	INST_NOP},
	{C_OPCODE_ARL,		INST_NOP},
};
