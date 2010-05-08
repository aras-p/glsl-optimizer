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
#include "r600_context.h"
#include "r600_sq.h"


struct r600_shader_alu_translate {
	unsigned			copcode;
	unsigned			opcode;
	unsigned			is_op3;
};

static int r600_shader_alu_translate(struct r600_shader *rshader,
					struct c_instruction *instruction);
struct r600_shader_alu_translate r600_alu_translate_list[C_OPCODE_LAST];

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
	c_list_for_each_safe(v, nv, &shader->files[C_FILE_INPUT].vectors) {
		if (v == vi)
			continue;
		vr = c_shader_vector_new(shader, C_FILE_RESOURCE, C_SEMANTIC_GENERIC, -1);
		if (vr == NULL)
			return -ENOMEM;
		memset(&instruction, 0, sizeof(struct c_instruction));
		instruction.opcode = C_OPCODE_VFETCH;
		instruction.write_mask = 0xF;
		instruction.ninput = 2;
		instruction.output.vector = v;
		instruction.input[0].vector = vi;
		instruction.input[1].vector = vr;
		instruction.output.swizzle[0] = C_SWIZZLE_X;
		instruction.output.swizzle[1] = C_SWIZZLE_Y;
		instruction.output.swizzle[2] = C_SWIZZLE_Z;
		instruction.output.swizzle[3] = C_SWIZZLE_W;
		r = c_node_add_new_instruction_head(&shader->entry, &instruction);
		if (r)
			return r;
		c_list_del(v);
		shader->files[C_FILE_INPUT].nvectors--;
		c_list_add_tail(v, &shader->files[C_FILE_TEMPORARY].vectors);
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
	c_list_for_each_safe(n, nn, &rshader->nodes) {
		c_list_del(n);
		c_list_for_each_safe(vf, nvf, &n->vfetch) {
			c_list_del(vf);
			free(vf);
		}
		c_list_for_each_safe(alu, nalu, &n->alu) {
			c_list_del(alu);
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
	rshader->bcode[id++] = S_SQ_VTX_WORD0_BUFFER_ID(vfetch->sel[2]) |
				S_SQ_VTX_WORD0_SRC_GPR(vfetch->sel[1]) |
				S_SQ_VTX_WORD0_SRC_SEL_X(vfetch->chan[1]) |
				S_SQ_VTX_WORD0_MEGA_FETCH_COUNT(0x1F);
	rshader->bcode[id++] = S_SQ_VTX_WORD1_DST_SEL_X(vfetch->dsel[0]) |
				S_SQ_VTX_WORD1_DST_SEL_Y(vfetch->dsel[1]) |
				S_SQ_VTX_WORD1_DST_SEL_Z(vfetch->dsel[2]) |
				S_SQ_VTX_WORD1_DST_SEL_W(vfetch->dsel[3]) |
				S_SQ_VTX_WORD1_USE_CONST_FIELDS(1) |
				S_SQ_VTX_WORD1_GPR_DST_GPR(vfetch->sel[0]);
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
	c_list_for_each(rnode, &rshader->nodes) {
		c_list_for_each(vfetch, &rnode->vfetch) {
			const struct util_format_description *desc;
			i = vfetch->cf_addr + 1;
			rshader->bcode[i] &= C_SQ_VTX_WORD1_DST_SEL_X;
			rshader->bcode[i] &= C_SQ_VTX_WORD1_DST_SEL_Y;
			rshader->bcode[i] &= C_SQ_VTX_WORD1_DST_SEL_Z;
			rshader->bcode[i] &= C_SQ_VTX_WORD1_DST_SEL_W;
			desc = util_format_description(resource_format[vfetch->sel[2]]);
			if (desc == NULL) {
				fprintf(stderr, "%s unknown format %d\n", __func__, resource_format[vfetch->sel[2]]);
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
	c_list_for_each(v, &rshader->cshader.files[C_FILE_INPUT].vectors) {
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
		c_list_for_each(v, &rshader->cshader.files[i].vectors) {
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

int r600_shader_find_gpr(struct r600_shader *rshader, struct c_vector *v,
			unsigned swizzle, unsigned *sel, unsigned *chan)
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
	*sel = 248;
	*chan = 0;
	if (v == NULL)
		return 0;
	if (v->file == C_FILE_IMMEDIATE) {
		*sel = 253;
	} else {
		tmp = rshader->gpr[v->id];
		if (tmp == NULL) {
			fprintf(stderr, "%s %d unknown register\n", __FILE__, __LINE__);
			return -EINVAL;
		}
		*sel = tmp->id;
	}
	*chan = swizzle;
	switch (swizzle) {
	case C_SWIZZLE_X:
	case C_SWIZZLE_Y:
	case C_SWIZZLE_Z:
	case C_SWIZZLE_W:
		break;
	case C_SWIZZLE_0:
		*sel = 248;
		*chan = 0;
		break;
	case C_SWIZZLE_1:
		*sel = 249;
		*chan = 0;
		break;
	default:
		fprintf(stderr, "%s %d invalid swizzle %d\n", __FILE__, __LINE__, swizzle);
		return -EINVAL;
	}
	return 0;
}

static int r600_shader_new_node(struct r600_shader *rshader, struct c_node *node)
{
	struct r600_shader_node *rnode;
	rnode = CALLOC_STRUCT(r600_shader_node);
	if (rnode == NULL)
		return -ENOMEM;
	rnode->node = node;
	c_list_init(&rnode->vfetch);
	c_list_init(&rnode->alu);
	c_list_add_tail(rnode, &rshader->nodes);
	rshader->cur_node = rnode;
	return 0;
}

static int r600_shader_new_alu(struct r600_shader *rshader, struct r600_shader_alu *alu)
{
	struct r600_shader_alu *nalu;

	nalu = CALLOC_STRUCT(r600_shader_alu);
	if (nalu == NULL)
		return -ENOMEM;
	memcpy(nalu, alu, sizeof(struct r600_shader_alu));
	c_list_add_tail(nalu, &rshader->cur_node->alu);
	return 0;
}

static int r600_shader_add_vfetch(struct r600_shader *rshader,
				struct c_instruction *instruction)
{
	struct r600_shader_vfetch *vfetch;
	int r;

	if (instruction == NULL)
		return 0;
	if (instruction->opcode != C_OPCODE_VFETCH)
		return 0;
	if (!c_list_empty(&rshader->cur_node->alu)) {
		r = r600_shader_new_node(rshader, rshader->cur_node->node);
		if (r)
			return r;
	}
	vfetch = calloc(1, sizeof(struct r600_shader_vfetch));
	if (vfetch == NULL)
		return -ENOMEM;
	r = r600_shader_find_gpr(rshader, instruction->output.vector, 0, &vfetch->sel[0], &vfetch->chan[0]);
	if (r)
		return r;
	r = r600_shader_find_gpr(rshader, instruction->input[0].vector, 0, &vfetch->sel[1], &vfetch->chan[1]);
	if (r)
		return r;
	r = r600_shader_find_gpr(rshader, instruction->input[1].vector, 0, &vfetch->sel[2], &vfetch->chan[2]);
	if (r)
		return r;
	vfetch->dsel[0] = C_SWIZZLE_X;
	vfetch->dsel[1] = C_SWIZZLE_Y;
	vfetch->dsel[2] = C_SWIZZLE_Z;
	vfetch->dsel[3] = C_SWIZZLE_W;
	c_list_add_tail(vfetch, &rshader->cur_node->vfetch);
	rshader->cur_node->nslot += 2;
	return 0;
}

static int r600_node_translate(struct r600_shader *rshader, struct c_node *node)
{
	struct c_instruction *instruction;
	int r;

	r = r600_shader_new_node(rshader, node);
	if (r)
		return r;
	c_list_for_each(instruction, &node->insts) {
		switch (instruction->opcode) {
		case C_OPCODE_VFETCH:
			r = r600_shader_add_vfetch(rshader, instruction);
			if (r)
				return r;
			break;
		default:
			r = r600_shader_alu_translate(rshader, instruction);
			if (r)
				return r;
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
	c_list_for_each(link, &node->childs) {
		r = r600_shader_translate_rec(rshader, link->node);
		if (r)
			return r;
	}
	return 0;
}

static int r600_shader_alu_translate(struct r600_shader *rshader,
						struct c_instruction *instruction)
{
	struct r600_shader_alu_translate *info = &r600_alu_translate_list[instruction->opcode];
	struct r600_shader_alu alu;
	unsigned rmask;
	int r, i, j, c;

	if (!c_list_empty(&rshader->cur_node->vfetch)) {
		r = r600_shader_new_node(rshader, rshader->cur_node->node);
		if (r)
			return r;
	}
	memset(&alu, 0, sizeof(struct r600_shader_alu));
	for (i = 0, c = 0; i < 4; i++) {
		if (!(instruction->write_mask) || instruction->output.swizzle[i] == C_SWIZZLE_D)
			continue;
		alu.alu[c].opcode = instruction->opcode;
		alu.alu[c].inst = info->opcode;
		alu.alu[c].is_op3 = info->is_op3;
		rmask = ~((1 << (i + 1)) - 1);
		r = r600_shader_find_gpr(rshader, instruction->output.vector,
						instruction->output.swizzle[i],
						&alu.alu[c].dst.sel, &alu.alu[c].dst.chan);
		if (r) {
			fprintf(stderr, "%s %d register failed\n", __FILE__, __LINE__);
			return r;
		}
		for (j = 0; j < instruction->ninput; j++) {
			r = r600_shader_find_gpr(rshader, instruction->input[j].vector,
					instruction->input[j].swizzle[i],
					&alu.alu[c].src[j].sel, &alu.alu[c].src[j].chan);
			if (r) {
				fprintf(stderr, "%s %d register failed\n", __FILE__, __LINE__);
				return r;
			}
			if (instruction->input[j].vector->file == C_FILE_IMMEDIATE) {
				alu.literal[0] = instruction->input[j].vector->channel[0]->value;
				alu.literal[1] = instruction->input[j].vector->channel[1]->value;
				alu.literal[2] = instruction->input[j].vector->channel[2]->value;
				alu.literal[3] = instruction->input[j].vector->channel[3]->value;
				if (alu.alu[c].src[j].chan > 1) {
					alu.nliteral = 4;
				} else {
					alu.nliteral = 2;
				}
			}
		}
		c++;
	}
	alu.nalu = c;
	alu.alu[c - 1].last = 1;
	r = r600_shader_new_alu(rshader, &alu);
	return r;
}

void r600_shader_node_place(struct r600_shader *rshader)
{
	struct r600_shader_node *node, *nnode;
	struct r600_shader_alu *alu, *nalu;
	struct r600_shader_vfetch *vfetch, *nvfetch;
	unsigned cf_id = 0, cf_addr = 0;

	rshader->ncf = 0;
	rshader->nslot = 0;
	c_list_for_each_safe(node, nnode, &rshader->nodes) {
		c_list_for_each_safe(alu, nalu, &node->alu) {
			node->nslot += alu->nalu;
			node->nslot += alu->nliteral >> 1;
		}
		node->nfetch = 0;
		c_list_for_each_safe(vfetch, nvfetch, &node->vfetch) {
			node->nslot += 2;
			node->nfetch += 1;
		}
		if (!c_list_empty(&node->vfetch)) {
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
	c_list_for_each_safe(node, nnode, &rshader->nodes) {
		node->cf_addr += cf_id * 2;
	}
	rshader->ncf += rshader->cshader.files[C_FILE_OUTPUT].nvectors;
	rshader->ndw = rshader->ncf * 2 + rshader->nslot * 2;
}

int r600_shader_legalize(struct r600_shader *rshader)
{
	struct r600_shader_node *node, *nnode;
	struct r600_shader_alu *alu, *nalu;
	unsigned i;

	c_list_for_each_safe(node, nnode, &rshader->nodes) {
		c_list_for_each_safe(alu, nalu, &node->alu) {
			for (i = 0; i < alu->nalu; i++) {
				switch (alu->alu[i].opcode) {
				case C_OPCODE_DP3:
					if (alu->alu[i].last) {
						/* force W component to be 0 */
						alu->alu[i].src[0].chan = alu->alu[i].src[1].chan = 0;
						alu->alu[i].src[0].sel = alu->alu[i].src[1].sel = 248;
					}
					break;
				case C_OPCODE_SUB:
					alu->alu[i].src[1].neg ^= 1;
					break;
				case C_OPCODE_ABS:
					alu->alu[i].src[0].abs |= 1;
					break;
				default:
					break;
				}
			}
		}
	}
	return 0;
}

#if 0
static int r600_shader_alu_translate_lit(struct r600_shader *rshader,
						struct c_instruction *instruction,
						struct r600_shader_alu_translate *info)
{
	struct r600_shader_alu alu;
	int r;

	if (rshader->cur_node->nvfetch) {
		r = r600_shader_new_node(rshader, rshader->cur_node->node);
		if (r)
			return r;
	}
	/* dst.z = log(src0.y) */
	memset(&alu, 0, sizeof(struct r600_shader_alu));
	alu.opcode = V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_LOG_CLAMPED;
	alu.is_op3 = 0;
	alu.last = 1;
	alu.vector[0] = instruction->vectors[0];
	alu.vector[1] = instruction->vectors[1];
	alu.component[0] = C_SWIZZLE_Z;
	alu.component[1] = C_SWIZZLE_Y;
	r = r600_shader_new_alu(rshader, &alu);
	if (r)
		return r;
	/* dst.z = MUL_LIT(src0.w, dst.z, src0.x)  */
	memset(&alu, 0, sizeof(struct r600_shader_alu));
	alu.opcode = V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MUL_LIT;
	alu.is_op3 = 0;
	alu.last = 1;
	alu.vector[0] = instruction->vectors[0];
	alu.vector[1] = instruction->vectors[1];
	alu.vector[2] = instruction->vectors[0];
	alu.vector[3] = instruction->vectors[1];
	alu.component[0] = C_SWIZZLE_Z;
	alu.component[1] = C_SWIZZLE_W;
	alu.component[2] = C_SWIZZLE_Z;
	alu.component[3] = C_SWIZZLE_X;
	r = r600_shader_new_alu(rshader, &alu);
	if (r)
		return r;
	/* dst.z = exp(dst.z) */
	memset(&alu, 0, sizeof(struct r600_shader_alu));
	alu.opcode = V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_EXP_IEEE;
	alu.is_op3 = 0;
	alu.last = 1;
	alu.vector[0] = instruction->vectors[0];
	alu.vector[1] = instruction->vectors[0];
	alu.component[0] = C_SWIZZLE_Z;
	alu.component[1] = C_SWIZZLE_Z;
	r = r600_shader_new_alu(rshader, &alu);
	if (r)
		return r;
	/* dst.x = 1 */
	memset(&alu, 0, sizeof(struct r600_shader_alu));
	alu.opcode = V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV;
	alu.is_op3 = 0;
	alu.last = 0;
	alu.vector[0] = instruction->vectors[0];
	alu.vector[1] = instruction->vectors[1];
	alu.component[0] = C_SWIZZLE_X;
	alu.component[1] = C_SWIZZLE_1;
	r = r600_shader_new_alu(rshader, &alu);
	if (r)
		return r;
	/* dst.w = 1 */
	memset(&alu, 0, sizeof(struct r600_shader_alu));
	alu.opcode = V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV;
	alu.is_op3 = 0;
	alu.last = 0;
	alu.vector[0] = instruction->vectors[0];
	alu.vector[1] = instruction->vectors[1];
	alu.component[0] = C_SWIZZLE_W;
	alu.component[1] = C_SWIZZLE_1;
	r = r600_shader_new_alu(rshader, &alu);
	if (r)
		return r;
	/* dst.y = max(src0.x, 0.0) */
	memset(&alu, 0, sizeof(struct r600_shader_alu));
	alu.opcode = V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX;
	alu.is_op3 = 0;
	alu.last = 1;
	alu.vector[0] = instruction->vectors[0];
	alu.vector[1] = instruction->vectors[1];
	alu.vector[2] = instruction->vectors[1];
	alu.component[0] = C_SWIZZLE_Y;
	alu.component[1] = C_SWIZZLE_X;
	alu.component[2] = C_SWIZZLE_0;
	r = r600_shader_new_alu(rshader, &alu);
	if (r)
		return r;
	return 0;
}
#endif
struct r600_shader_alu_translate r600_alu_translate_list[C_OPCODE_LAST] = {
	{C_OPCODE_ARL, 0, 0},
	{C_OPCODE_MOV, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV, 0},
	{C_OPCODE_LIT, 0, 0},
	{C_OPCODE_RCP, 0, 0},
	{C_OPCODE_RSQ, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_RECIPSQRT_FF, 0},
	{C_OPCODE_EXP, 0, 0},
	{C_OPCODE_LOG, 0, 0},
	{C_OPCODE_MUL, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MUL, 0},
	{C_OPCODE_ADD, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD, 0},
	{C_OPCODE_DP3, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4, 0},
	{C_OPCODE_DP4, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_DOT4, 0},
	{C_OPCODE_DST, 0, 0},
	{C_OPCODE_MIN, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MIN, 0},
	{C_OPCODE_MAX, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MAX, 0},
	{C_OPCODE_SLT, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SETGT, 0},
	{C_OPCODE_SGE, 0, 0},
	{C_OPCODE_MAD, V_SQ_ALU_WORD1_OP3_SQ_OP3_INST_MULADD, 1},
	{C_OPCODE_SUB, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_ADD, 0},
	{C_OPCODE_LRP, 0, 0},
	{C_OPCODE_CND, 0, 0},
	{20, 0, 0},
	{C_OPCODE_DP2A, 0, 0},
	{22, 0, 0},
	{23, 0, 0},
	{C_OPCODE_FRC, 0, 0},
	{C_OPCODE_CLAMP, 0, 0},
	{C_OPCODE_FLR, 0, 0},
	{C_OPCODE_ROUND, 0, 0},
	{C_OPCODE_EX2, 0, 0},
	{C_OPCODE_LG2, 0, 0},
	{C_OPCODE_POW, 0, 0},
	{C_OPCODE_XPD, 0, 0},
	{32, 0, 0},
	{C_OPCODE_ABS, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_MOV, 0},
	{C_OPCODE_RCC, 0, 0},
	{C_OPCODE_DPH, 0, 0},
	{C_OPCODE_COS, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_COS, 0},
	{C_OPCODE_DDX, 0, 0},
	{C_OPCODE_DDY, 0, 0},
	{C_OPCODE_KILP, 0, 0},
	{C_OPCODE_PK2H, 0, 0},
	{C_OPCODE_PK2US, 0, 0},
	{C_OPCODE_PK4B, 0, 0},
	{C_OPCODE_PK4UB, 0, 0},
	{C_OPCODE_RFL, 0, 0},
	{C_OPCODE_SEQ, 0, 0},
	{C_OPCODE_SFL, 0, 0},
	{C_OPCODE_SGT, 0, 0},
	{C_OPCODE_SIN, V_SQ_ALU_WORD1_OP2_SQ_OP2_INST_SIN, 0},
	{C_OPCODE_SLE, 0, 0},
	{C_OPCODE_SNE, 0, 0},
	{C_OPCODE_STR, 0, 0},
	{C_OPCODE_TEX, 0, 0},
	{C_OPCODE_TXD, 0, 0},
	{C_OPCODE_TXP, 0, 0},
	{C_OPCODE_UP2H, 0, 0},
	{C_OPCODE_UP2US, 0, 0},
	{C_OPCODE_UP4B, 0, 0},
	{C_OPCODE_UP4UB, 0, 0},
	{C_OPCODE_X2D, 0, 0},
	{C_OPCODE_ARA, 0, 0},
	{C_OPCODE_ARR, 0, 0},
	{C_OPCODE_BRA, 0, 0},
	{C_OPCODE_CAL, 0, 0},
	{C_OPCODE_RET, 0, 0},
	{C_OPCODE_SSG, 0, 0},
	{C_OPCODE_CMP, 0, 0},
	{C_OPCODE_SCS, 0, 0},
	{C_OPCODE_TXB, 0, 0},
	{C_OPCODE_NRM, 0, 0},
	{C_OPCODE_DIV, 0, 0},
	{C_OPCODE_DP2, 0, 0},
	{C_OPCODE_TXL, 0, 0},
	{C_OPCODE_BRK, 0, 0},
	{C_OPCODE_IF, 0, 0},
	{C_OPCODE_BGNFOR, 0, 0},
	{C_OPCODE_REP, 0, 0},
	{C_OPCODE_ELSE, 0, 0},
	{C_OPCODE_ENDIF, 0, 0},
	{C_OPCODE_ENDFOR, 0, 0},
	{C_OPCODE_ENDREP, 0, 0},
	{C_OPCODE_PUSHA, 0, 0},
	{C_OPCODE_POPA, 0, 0},
	{C_OPCODE_CEIL, 0, 0},
	{C_OPCODE_I2F, 0, 0},
	{C_OPCODE_NOT, 0, 0},
	{C_OPCODE_TRUNC, 0, 0},
	{C_OPCODE_SHL, 0, 0},
	{88, 0, 0},
	{C_OPCODE_AND, 0, 0},
	{C_OPCODE_OR, 0, 0},
	{C_OPCODE_MOD, 0, 0},
	{C_OPCODE_XOR, 0, 0},
	{C_OPCODE_SAD, 0, 0},
	{C_OPCODE_TXF, 0, 0},
	{C_OPCODE_TXQ, 0, 0},
	{C_OPCODE_CONT, 0, 0},
	{C_OPCODE_EMIT, 0, 0},
	{C_OPCODE_ENDPRIM, 0, 0},
	{C_OPCODE_BGNLOOP, 0, 0},
	{C_OPCODE_BGNSUB, 0, 0},
	{C_OPCODE_ENDLOOP, 0, 0},
	{C_OPCODE_ENDSUB, 0, 0},
	{103, 0, 0},
	{104, 0, 0},
	{105, 0, 0},
	{106, 0, 0},
	{C_OPCODE_NOP, 0, 0},
	{108, 0, 0},
	{109, 0, 0},
	{110, 0, 0},
	{111, 0, 0},
	{C_OPCODE_NRM4, 0, 0},
	{C_OPCODE_CALLNZ, 0, 0},
	{C_OPCODE_IFC, 0, 0},
	{C_OPCODE_BREAKC, 0, 0},
	{C_OPCODE_KIL, 0, 0},
	{C_OPCODE_END, 0, 0},
	{118, 0, 0},
	{C_OPCODE_F2I, 0, 0},
	{C_OPCODE_IDIV, 0, 0},
	{C_OPCODE_IMAX, 0, 0},
	{C_OPCODE_IMIN, 0, 0},
	{C_OPCODE_INEG, 0, 0},
	{C_OPCODE_ISGE, 0, 0},
	{C_OPCODE_ISHR, 0, 0},
	{C_OPCODE_ISLT, 0, 0},
	{C_OPCODE_F2U, 0, 0},
	{C_OPCODE_U2F, 0, 0},
	{C_OPCODE_UADD, 0, 0},
	{C_OPCODE_UDIV, 0, 0},
	{C_OPCODE_UMAD, 0, 0},
	{C_OPCODE_UMAX, 0, 0},
	{C_OPCODE_UMIN, 0, 0},
	{C_OPCODE_UMOD, 0, 0},
	{C_OPCODE_UMUL, 0, 0},
	{C_OPCODE_USEQ, 0, 0},
	{C_OPCODE_USGE, 0, 0},
	{C_OPCODE_USHR, 0, 0},
	{C_OPCODE_USLT, 0, 0},
	{C_OPCODE_USNE, 0, 0},
	{C_OPCODE_SWITCH, 0, 0},
	{C_OPCODE_CASE, 0, 0},
	{C_OPCODE_DEFAULT, 0, 0},
	{C_OPCODE_ENDSWITCH, 0, 0},
	{C_OPCODE_VFETCH, 0, 0},
	{C_OPCODE_ENTRY, 0, 0},
};
