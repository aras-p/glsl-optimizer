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
#include "r600_compiler.h"

struct c_vector *c_vector_new(void)
{
	struct c_vector *v = calloc(1, sizeof(struct c_vector));

	if (v == NULL) {
		return NULL;
	}
	LIST_INITHEAD(&v->head);
	return v;
}

static unsigned c_opcode_is_alu(unsigned opcode)
{
	switch (opcode) {
	case C_OPCODE_MOV:
	case C_OPCODE_MUL:
	case C_OPCODE_MAD:
	case C_OPCODE_ARL:
	case C_OPCODE_LIT:
	case C_OPCODE_RCP:
	case C_OPCODE_RSQ:
	case C_OPCODE_EXP:
	case C_OPCODE_LOG:
	case C_OPCODE_ADD:
	case C_OPCODE_DP3:
	case C_OPCODE_DP4:
	case C_OPCODE_DST:
	case C_OPCODE_MIN:
	case C_OPCODE_MAX:
	case C_OPCODE_SLT:
	case C_OPCODE_SGE:
	case C_OPCODE_SUB:
	case C_OPCODE_LRP:
	case C_OPCODE_CND:
	case C_OPCODE_DP2A:
	case C_OPCODE_FRC:
	case C_OPCODE_CLAMP:
	case C_OPCODE_FLR:
	case C_OPCODE_ROUND:
	case C_OPCODE_EX2:
	case C_OPCODE_LG2:
	case C_OPCODE_POW:
	case C_OPCODE_XPD:
	case C_OPCODE_ABS:
	case C_OPCODE_RCC:
	case C_OPCODE_DPH:
	case C_OPCODE_COS:
	case C_OPCODE_DDX:
	case C_OPCODE_DDY:
	case C_OPCODE_PK2H:
	case C_OPCODE_PK2US:
	case C_OPCODE_PK4B:
	case C_OPCODE_PK4UB:
	case C_OPCODE_RFL:
	case C_OPCODE_SEQ:
	case C_OPCODE_SFL:
	case C_OPCODE_SGT:
	case C_OPCODE_SIN:
	case C_OPCODE_SLE:
	case C_OPCODE_SNE:
	case C_OPCODE_STR:
	case C_OPCODE_UP2H:
	case C_OPCODE_UP2US:
	case C_OPCODE_UP4B:
	case C_OPCODE_UP4UB:
	case C_OPCODE_X2D:
	case C_OPCODE_ARA:
	case C_OPCODE_ARR:
	case C_OPCODE_BRA:
	case C_OPCODE_SSG:
	case C_OPCODE_CMP:
	case C_OPCODE_SCS:
	case C_OPCODE_NRM:
	case C_OPCODE_DIV:
	case C_OPCODE_DP2:
	case C_OPCODE_CEIL:
	case C_OPCODE_I2F:
	case C_OPCODE_NOT:
	case C_OPCODE_TRUNC:
	case C_OPCODE_SHL:
	case C_OPCODE_AND:
	case C_OPCODE_OR:
	case C_OPCODE_MOD:
	case C_OPCODE_XOR:
	case C_OPCODE_SAD:
	case C_OPCODE_NRM4:
	case C_OPCODE_F2I:
	case C_OPCODE_IDIV:
	case C_OPCODE_IMAX:
	case C_OPCODE_IMIN:
	case C_OPCODE_INEG:
	case C_OPCODE_ISGE:
	case C_OPCODE_ISHR:
	case C_OPCODE_ISLT:
	case C_OPCODE_F2U:
	case C_OPCODE_U2F:
	case C_OPCODE_UADD:
	case C_OPCODE_UDIV:
	case C_OPCODE_UMAD:
	case C_OPCODE_UMAX:
	case C_OPCODE_UMIN:
	case C_OPCODE_UMOD:
	case C_OPCODE_UMUL:
	case C_OPCODE_USEQ:
	case C_OPCODE_USGE:
	case C_OPCODE_USHR:
	case C_OPCODE_USLT:
	case C_OPCODE_USNE:
		return 1;
	case C_OPCODE_END:
	case C_OPCODE_VFETCH:
	case C_OPCODE_KILP:
	case C_OPCODE_CAL:
	case C_OPCODE_RET:
	case C_OPCODE_TXB:
	case C_OPCODE_TXL:
	case C_OPCODE_BRK:
	case C_OPCODE_IF:
	case C_OPCODE_BGNFOR:
	case C_OPCODE_REP:
	case C_OPCODE_ELSE:
	case C_OPCODE_ENDIF:
	case C_OPCODE_ENDFOR:
	case C_OPCODE_ENDREP:
	case C_OPCODE_PUSHA:
	case C_OPCODE_POPA:
	case C_OPCODE_TXF:
	case C_OPCODE_TXQ:
	case C_OPCODE_CONT:
	case C_OPCODE_EMIT:
	case C_OPCODE_ENDPRIM:
	case C_OPCODE_BGNLOOP:
	case C_OPCODE_BGNSUB:
	case C_OPCODE_ENDLOOP:
	case C_OPCODE_ENDSUB:
	case C_OPCODE_NOP:
	case C_OPCODE_CALLNZ:
	case C_OPCODE_IFC:
	case C_OPCODE_BREAKC:
	case C_OPCODE_KIL:
	case C_OPCODE_TEX:
	case C_OPCODE_TXD:
	case C_OPCODE_TXP:
	case C_OPCODE_SWITCH:
	case C_OPCODE_CASE:
	case C_OPCODE_DEFAULT:
	case C_OPCODE_ENDSWITCH:
	default:
		return 0;
	}
}


/* NEW */
void c_node_init(struct c_node *node)
{
	memset(node, 0, sizeof(struct c_node));
	LIST_INITHEAD(&node->predecessors);
	LIST_INITHEAD(&node->successors);
	LIST_INITHEAD(&node->childs);
	LIST_INITHEAD(&node->insts);
	node->parent = NULL;
}

static struct c_node_link *c_node_link_new(struct c_node *node)
{
	struct c_node_link *link;

	link = calloc(1, sizeof(struct c_node_link));
	if (link == NULL)
		return NULL;
	LIST_INITHEAD(&link->head);
	link->node = node;
	return link;
}

int c_node_cfg_link(struct c_node *predecessor, struct c_node *successor)
{
	struct c_node_link *pedge, *sedge;

	pedge = c_node_link_new(successor);
	sedge = c_node_link_new(predecessor);
	if (sedge == NULL || pedge == NULL) {
		free(sedge);
		free(pedge);
		return -ENOMEM;
	}
	LIST_ADDTAIL(&pedge->head, &predecessor->successors);
	LIST_ADDTAIL(&sedge->head, &successor->predecessors);

	return 0;
}

int c_node_add_new_instruction_head(struct c_node *node, struct c_instruction *instruction)
{
	struct c_instruction *inst = malloc(sizeof(struct c_instruction));

	if (inst == NULL)
		return -ENOMEM;
	memcpy(inst, instruction, sizeof(struct c_instruction));
	LIST_ADD(&inst->head, &node->insts);
	return 0;
}

int c_node_add_new_instruction(struct c_node *node, struct c_instruction *instruction)
{
	struct c_instruction *inst = malloc(sizeof(struct c_instruction));

	if (inst == NULL)
		return -ENOMEM;
	memcpy(inst, instruction, sizeof(struct c_instruction));
	LIST_ADDTAIL(&inst->head, &node->insts);
	return 0;
}

struct c_node *c_shader_cfg_new_node_after(struct c_shader *shader, struct c_node *predecessor)
{
	struct c_node *node = calloc(1, sizeof(struct c_node));

	if (node == NULL)
		return NULL;
	c_node_init(node);
	if (c_node_cfg_link(predecessor, node)) {
		free(node);
		return NULL;
	}
	LIST_ADDTAIL(&node->head, &shader->nodes);
	return node;
}

int c_shader_init(struct c_shader *shader, unsigned type)
{
	unsigned i;
	int r;

	shader->type = type;
	for (i = 0; i < C_FILE_COUNT; i++) {
		shader->files[i].nvectors = 0;
		LIST_INITHEAD(&shader->files[i].vectors);
	}
	LIST_INITHEAD(&shader->nodes);
	c_node_init(&shader->entry);
	c_node_init(&shader->end);
	shader->entry.opcode = C_OPCODE_ENTRY;
	shader->end.opcode = C_OPCODE_END;
	r = c_node_cfg_link(&shader->entry, &shader->end);
	if (r)
		return r;
	return 0;
}

struct c_vector *c_shader_vector_new(struct c_shader *shader, unsigned file, unsigned name, int sid)
{
	struct c_vector *v = calloc(1, sizeof(struct c_vector));
	int i;

	if (v == NULL) {
		return NULL;
	}
	for (i = 0; i < 4; i++) {
		v->channel[i] = calloc(1, sizeof(struct c_channel));
		if (v->channel[i] == NULL)
			goto out_err;
		v->channel[i]->vindex = i;
		v->channel[i]->vector = v;
	}
	v->file = file;
	v->name = name;
	v->sid = sid;
	shader->files[v->file].nvectors++;
	v->id = shader->nvectors++;
	LIST_ADDTAIL(&v->head, &shader->files[v->file].vectors);
	return v;
out_err:
	for (i = 0; i < 4; i++) {
		free(v->channel[i]);
	}
	free(v);
	return NULL;
}

static void c_node_remove_link(struct list_head *head, struct c_node *node)
{
	struct c_node_link *link, *tmp;

	LIST_FOR_EACH_ENTRY_SAFE(link, tmp, head, head) {
		if (link->node == node) {
			LIST_DEL(&link->head);
			free(link);
		}
	}
}

static void c_node_destroy(struct c_node *node)
{
	struct c_instruction *i, *ni;
	struct c_node_link *link, *tmp;

	LIST_FOR_EACH_ENTRY_SAFE(i, ni, &node->insts, head) {
		LIST_DEL(&i->head);
		free(i);
	}
	if (node->parent)
		c_node_remove_link(&node->parent->childs, node);
	node->parent = NULL;
	LIST_FOR_EACH_ENTRY_SAFE(link, tmp, &node->predecessors, head) {
		c_node_remove_link(&link->node->successors, node);
		LIST_DEL(&link->head);
		free(link);
	}
	LIST_FOR_EACH_ENTRY_SAFE(link, tmp, &node->successors, head) {
		c_node_remove_link(&link->node->predecessors, node);
		LIST_DEL(&link->head);
		free(link);
	}
	LIST_FOR_EACH_ENTRY_SAFE(link, tmp, &node->childs, head) {
		link->node->parent = NULL;
		LIST_DEL(&link->head);
		free(link);
	}
}

void c_shader_destroy(struct c_shader *shader)
{
	struct c_node *n, *nn;
	struct c_vector *v, *nv;
	unsigned i;

	for (i = 0; i < C_FILE_COUNT; i++) {
		shader->files[i].nvectors = 0;
		LIST_FOR_EACH_ENTRY_SAFE(v, nv, &shader->files[i].vectors, head) {
			LIST_DEL(&v->head);
			free(v->channel[0]);
			free(v->channel[1]);
			free(v->channel[2]);
			free(v->channel[3]);
			free(v);
		}
	}
	LIST_FOR_EACH_ENTRY_SAFE(n, nn, &shader->nodes, head) {
		LIST_DEL(&n->head);
		c_node_destroy(n);
	}
	memset(shader, 0, sizeof(struct c_shader));
}

static void c_shader_dfs_without_rec(struct c_node *entry, struct c_node *node)
{
	struct c_node_link *link;

	if (entry == node || entry->visited)
		return;
	entry->visited = 1;
	LIST_FOR_EACH_ENTRY(link, &entry->successors, head) {
		c_shader_dfs_without_rec(link->node, node);
	}
}

static void c_shader_dfs_without(struct c_shader *shader, struct c_node *node)
{
	struct c_node *n;

	shader->entry.visited = 0;
	shader->end.visited = 0;
	LIST_FOR_EACH_ENTRY(n, &shader->nodes, head) {
		n->visited = 0;
	}
	c_shader_dfs_without_rec(&shader->entry, node);
}

static int c_shader_build_dominator_tree_rec(struct c_shader *shader, struct c_node *node)
{
	struct c_node_link *link, *nlink;
	unsigned found = 0;
	int r;

	if (node->done)
		return 0;
	node->done = 1;
	LIST_FOR_EACH_ENTRY(link, &node->predecessors, head) {
		/* if we remove this predecessor can we reach the current node ? */
		c_shader_dfs_without(shader, link->node);
		if (node->visited == 0) {
			/* we were unable to visit current node thus current
			 * predecessor  is the immediate dominator of node, as
			 * their can be only one immediate dominator we break
			 */
			node->parent = link->node;
			nlink = c_node_link_new(node);
			if (nlink == NULL)
				return -ENOMEM;
			LIST_ADDTAIL(&nlink->head, &link->node->childs);
			found = 1;
			break;
		}
	}
	/* this shouldn't happen there should at least be 1 denominator for each node */
	if (!found && node->opcode != C_OPCODE_ENTRY) {
		fprintf(stderr, "invalid flow control graph node %p (%d) has no immediate dominator\n",
			node, node->opcode);
		return -EINVAL;
	}
	LIST_FOR_EACH_ENTRY(link, &node->predecessors, head) {
		r = c_shader_build_dominator_tree_rec(shader, link->node);
		if (r)
			return r;
	}
	return 0;
}

int c_shader_build_dominator_tree(struct c_shader *shader)
{
	struct c_node *node;
	LIST_FOR_EACH_ENTRY(node, &shader->nodes, head) {
		node->done = 0;
	}
	return c_shader_build_dominator_tree_rec(shader, &shader->end);
}
