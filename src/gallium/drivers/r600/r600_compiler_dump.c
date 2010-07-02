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
#include "r600_compiler.h"

static const char *c_file_swz[] = {
	"x",
	"y",
	"z",
	"w",
	"0",
	"1",
	".",
};

static const char *c_file_str[] = {
	"NULL",
	"CONSTANT",
	"INPUT",
	"OUTPUT",
	"TEMPORARY",
	"SAMPLER",
	"ADDRESS",
	"IMMEDIATE",
	"LOOP",
	"PREDICATE",
	"SYSTEM_VALUE",
};

static const char *c_semantic_str[] = {
	"POSITION",
	"COLOR",
	"BCOLOR",
	"FOG",
	"PSIZE",
	"GENERIC",
	"NORMAL",
	"FACE",
	"EDGEFLAG",
	"PRIMID",
	"INSTANCEID",
};

static const char *c_opcode_str[] = {
	"ARL",
	"MOV",
	"LIT",
	"RCP",
	"RSQ",
	"EXP",
	"LOG",
	"MUL",
	"ADD",
	"DP3",
	"DP4",
	"DST",
	"MIN",
	"MAX",
	"SLT",
	"SGE",
	"MAD",
	"SUB",
	"LRP",
	"CND",
	"(INVALID)",
	"DP2A",
	"(INVALID)",
	"(INVALID)",
	"FRC",
	"CLAMP",
	"FLR",
	"ROUND",
	"EX2",
	"LG2",
	"POW",
	"XPD",
	"(INVALID)",
	"ABS",
	"RCC",
	"DPH",
	"COS",
	"DDX",
	"DDY",
	"KILP",
	"PK2H",
	"PK2US",
	"PK4B",
	"PK4UB",
	"RFL",
	"SEQ",
	"SFL",
	"SGT",
	"SIN",
	"SLE",
	"SNE",
	"STR",
	"TEX",
	"TXD",
	"TXP",
	"UP2H",
	"UP2US",
	"UP4B",
	"UP4UB",
	"X2D",
	"ARA",
	"ARR",
	"BRA",
	"CAL",
	"RET",
	"SSG",
	"CMP",
	"SCS",
	"TXB",
	"NRM",
	"DIV",
	"DP2",
	"TXL",
	"BRK",
	"IF",
	"BGNFOR",
	"REP",
	"ELSE",
	"ENDIF",
	"ENDFOR",
	"ENDREP",
	"PUSHA",
	"POPA",
	"CEIL",
	"I2F",
	"NOT",
	"TRUNC",
	"SHL",
	"(INVALID)",
	"AND",
	"OR",
	"MOD",
	"XOR",
	"SAD",
	"TXF",
	"TXQ",
	"CONT",
	"EMIT",
	"ENDPRIM",
	"BGNLOOP",
	"BGNSUB",
	"ENDLOOP",
	"ENDSUB",
	"(INVALID)",
	"(INVALID)",
	"(INVALID)",
	"(INVALID)",
	"NOP",
	"(INVALID)",
	"(INVALID)",
	"(INVALID)",
	"(INVALID)",
	"NRM4",
	"CALLNZ",
	"IFC",
	"BREAKC",
	"KIL",
	"END",
	"(INVALID)",
	"F2I",
	"IDIV",
	"IMAX",
	"IMIN",
	"INEG",
	"ISGE",
	"ISHR",
	"ISLT",
	"F2U",
	"U2F",
	"UADD",
	"UDIV",
	"UMAD",
	"UMAX",
	"UMIN",
	"UMOD",
	"UMUL",
	"USEQ",
	"USGE",
	"USHR",
	"USLT",
	"USNE",
	"SWITCH",
	"CASE",
	"DEFAULT",
	"ENDSWITCH",
	"VFETCH",
	"ENTRY",
};

static inline const char *c_get_name(const char *name[], unsigned i)
{
	return name[i];
}

static void pindent(unsigned indent)
{
	unsigned i;
	for (i = 0; i < indent; i++)
		fprintf(stderr, " ");
}

static void c_node_dump(struct c_node *node, unsigned indent)
{
	struct c_instruction *i;
	unsigned j, k;

	pindent(indent); fprintf(stderr, "# node %s\n", c_get_name(c_opcode_str, node->opcode));
	LIST_FOR_EACH_ENTRY(i, &node->insts, head) {
		for (k = 0; k < i->nop; k++) {
			pindent(indent);
			fprintf(stderr, "%s", c_get_name(c_opcode_str, i->op[k].opcode));
			fprintf(stderr, " %s[%d][%s]",
				c_get_name(c_file_str, i->op[k].output.vector->file),
				i->op[k].output.vector->id,
				c_get_name(c_file_swz, i->op[k].output.swizzle));
			for (j = 0; j < i->op[k].ninput; j++) {
				fprintf(stderr, " %s[%d][%s]",
						c_get_name(c_file_str, i->op[k].input[j].vector->file),
						i->op[k].input[j].vector->id,
						c_get_name(c_file_swz, i->op[k].input[j].swizzle));
			}
			fprintf(stderr, ";\n");
		}
	}
}

static void c_shader_dump_rec(struct c_shader *shader, struct c_node *node, unsigned indent)
{
	struct c_node_link *link;

	c_node_dump(node, indent);
	LIST_FOR_EACH_ENTRY(link, &node->childs, head) {
		c_shader_dump_rec(shader, link->node, indent + 1);
	}
}

void c_shader_dump(struct c_shader *shader)
{
	c_shader_dump_rec(shader, &shader->entry, 0);
}
