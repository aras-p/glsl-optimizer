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
#ifndef R600_COMPILER_H
#define R600_COMPILER_H

struct c_vector;

/* operand are the basic source/destination of each operation */
struct c_channel {
	struct c_channel	*next;
	struct c_channel	*prev;
	unsigned		vindex;		/**< index in vector X,Y,Z,W (0,1,2,3) */
	unsigned		value;		/**< immediate value 32bits */
	struct c_vector		*vector;	/**< vector to which it belongs */
};

/* in GPU world most of the time operand are grouped into vector
 * of 4 component this structure is mostly and handler to group
 * operand into a same vector
 */
struct c_vector {
	struct c_vector		*next;
	struct c_vector		*prev;
	unsigned		id;		/**< vector uniq id */
	unsigned		name;		/**< semantic name */
	unsigned		file;		/**< operand file C_FILE_* */
	int			sid;		/**< semantic id */
	struct c_channel	*channel[4];	/**< operands */
};

#define c_list_init(e) do { (e)->next = e; (e)->prev = e; } while(0)
#define c_list_add(e, h) do { (e)->next = (h)->next; (e)->prev = h;  (h)->next = e; (e)->next->prev = e; } while(0)
#define c_list_add_tail(e, h) do { (e)->next = h; (e)->prev = (h)->prev;  (h)->prev = e; (e)->prev->next = e; } while(0)
#define c_list_del(e) do { (e)->next->prev = (e)->prev; (e)->prev->next = (e)->next; c_list_init(e); } while(0)
#define c_list_for_each(p, h) for (p = (h)->next; p != (h); p = p->next)
#define c_list_for_each_from(p, s, h) for (p = s; p != (h); p = p->next)
#define c_list_for_each_safe(p, n, h) for (p = (h)->next, n = p->next; p != (h); p = n, n = p->next)
#define c_list_empty(h) ((h)->next == h)


#define C_PROGRAM_TYPE_VS	0
#define C_PROGRAM_TYPE_FS	1
#define C_PROGRAM_TYPE_COUNT	2

#define C_NODE_FLAG_ALU		1
#define C_NODE_FLAG_FETCH	2

#define C_SWIZZLE_X		0
#define C_SWIZZLE_Y		1
#define C_SWIZZLE_Z		2
#define C_SWIZZLE_W		3
#define C_SWIZZLE_0		4
#define C_SWIZZLE_1		5
#define C_SWIZZLE_D		6	/**< discard */

#define C_FILE_NULL		0
#define C_FILE_CONSTANT		1
#define C_FILE_INPUT		2
#define C_FILE_OUTPUT		3
#define C_FILE_TEMPORARY	4
#define C_FILE_SAMPLER		5
#define C_FILE_ADDRESS		6
#define C_FILE_IMMEDIATE	7
#define C_FILE_LOOP		8
#define C_FILE_PREDICATE	9
#define C_FILE_SYSTEM_VALUE	10
#define C_FILE_RESOURCE		11
#define C_FILE_COUNT		12

#define C_SEMANTIC_POSITION	0
#define C_SEMANTIC_COLOR	1
#define C_SEMANTIC_BCOLOR	2  /**< back-face color */
#define C_SEMANTIC_FOG		3
#define C_SEMANTIC_PSIZE	4
#define C_SEMANTIC_GENERIC	5
#define C_SEMANTIC_NORMAL	6
#define C_SEMANTIC_FACE		7
#define C_SEMANTIC_EDGEFLAG	8
#define C_SEMANTIC_PRIMID	9
#define C_SEMANTIC_INSTANCEID	10
#define C_SEMANTIC_VERTEXID	11
#define C_SEMANTIC_COUNT	12 /**< number of semantic values */

#define C_OPCODE_NOP		0
#define C_OPCODE_MOV		1
#define C_OPCODE_LIT		2
#define C_OPCODE_RCP		3
#define C_OPCODE_RSQ		4
#define C_OPCODE_EXP		5
#define C_OPCODE_LOG		6
#define C_OPCODE_MUL		7
#define C_OPCODE_ADD		8
#define C_OPCODE_DP3		9
#define C_OPCODE_DP4		10
#define C_OPCODE_DST		11
#define C_OPCODE_MIN		12
#define C_OPCODE_MAX		13
#define C_OPCODE_SLT		14
#define C_OPCODE_SGE		15
#define C_OPCODE_MAD		16
#define C_OPCODE_SUB		17
#define C_OPCODE_LRP		18
#define C_OPCODE_CND		19
/* gap */
#define C_OPCODE_DP2A		21
/* gap */
#define C_OPCODE_FRC		24
#define C_OPCODE_CLAMP		25
#define C_OPCODE_FLR		26
#define C_OPCODE_ROUND		27
#define C_OPCODE_EX2		28
#define C_OPCODE_LG2		29
#define C_OPCODE_POW		30
#define C_OPCODE_XPD		31
/* gap */
#define C_OPCODE_ABS		33
#define C_OPCODE_RCC		34
#define C_OPCODE_DPH		35
#define C_OPCODE_COS		36
#define C_OPCODE_DDX		37
#define C_OPCODE_DDY		38
#define C_OPCODE_KILP		39		/* predicated kill */
#define C_OPCODE_PK2H		40
#define C_OPCODE_PK2US		41
#define C_OPCODE_PK4B		42
#define C_OPCODE_PK4UB		43
#define C_OPCODE_RFL		44
#define C_OPCODE_SEQ		45
#define C_OPCODE_SFL		46
#define C_OPCODE_SGT		47
#define C_OPCODE_SIN		48
#define C_OPCODE_SLE		49
#define C_OPCODE_SNE		50
#define C_OPCODE_STR		51
#define C_OPCODE_TEX		52
#define C_OPCODE_TXD		53
#define C_OPCODE_TXP		54
#define C_OPCODE_UP2H		55
#define C_OPCODE_UP2US		56
#define C_OPCODE_UP4B		57
#define C_OPCODE_UP4UB		58
#define C_OPCODE_X2D		59
#define C_OPCODE_ARA		60
#define C_OPCODE_ARR		61
#define C_OPCODE_BRA		62
#define C_OPCODE_CAL		63
#define C_OPCODE_RET		64
#define C_OPCODE_SSG		65		/* SGN */
#define C_OPCODE_CMP		66
#define C_OPCODE_SCS		67
#define C_OPCODE_TXB		68
#define C_OPCODE_NRM		69
#define C_OPCODE_DIV		70
#define C_OPCODE_DP2		71
#define C_OPCODE_TXL		72
#define C_OPCODE_BRK		73
#define C_OPCODE_IF		74
#define C_OPCODE_BGNFOR		75
#define C_OPCODE_REP		76
#define C_OPCODE_ELSE		77
#define C_OPCODE_ENDIF		78
#define C_OPCODE_ENDFOR		79
#define C_OPCODE_ENDREP		80
#define C_OPCODE_PUSHA		81
#define C_OPCODE_POPA		82
#define C_OPCODE_CEIL		83
#define C_OPCODE_I2F		84
#define C_OPCODE_NOT		85
#define C_OPCODE_TRUNC		86
#define C_OPCODE_SHL		87
/* gap */
#define C_OPCODE_AND		89
#define C_OPCODE_OR		90
#define C_OPCODE_MOD		91
#define C_OPCODE_XOR		92
#define C_OPCODE_SAD		93
#define C_OPCODE_TXF		94
#define C_OPCODE_TXQ		95
#define C_OPCODE_CONT		96
#define C_OPCODE_EMIT		97
#define C_OPCODE_ENDPRIM	98
#define C_OPCODE_BGNLOOP	99
#define C_OPCODE_BGNSUB		100
#define C_OPCODE_ENDLOOP	101
#define C_OPCODE_ENDSUB		102
/* gap */
#define C_OPCODE_NRM4		112
#define C_OPCODE_CALLNZ		113
#define C_OPCODE_IFC		114
#define C_OPCODE_BREAKC		115
#define C_OPCODE_KIL		116	/* conditional kill */
#define C_OPCODE_END		117	/* aka HALT */
/* gap */
#define C_OPCODE_F2I		119
#define C_OPCODE_IDIV		120
#define C_OPCODE_IMAX		121
#define C_OPCODE_IMIN		122
#define C_OPCODE_INEG		123
#define C_OPCODE_ISGE		124
#define C_OPCODE_ISHR		125
#define C_OPCODE_ISLT		126
#define C_OPCODE_F2U		127
#define C_OPCODE_U2F		128
#define C_OPCODE_UADD		129
#define C_OPCODE_UDIV		130
#define C_OPCODE_UMAD		131
#define C_OPCODE_UMAX		132
#define C_OPCODE_UMIN		133
#define C_OPCODE_UMOD		134
#define C_OPCODE_UMUL		135
#define C_OPCODE_USEQ		136
#define C_OPCODE_USGE		137
#define C_OPCODE_USHR		138
#define C_OPCODE_USLT		139
#define C_OPCODE_USNE		140
#define C_OPCODE_SWITCH		141
#define C_OPCODE_CASE		142
#define C_OPCODE_DEFAULT	143
#define C_OPCODE_ENDSWITCH	144
#define C_OPCODE_VFETCH		145
#define C_OPCODE_ENTRY		146
#define C_OPCODE_ARL		147
#define C_OPCODE_LAST		148

#define C_OPERAND_FLAG_ABS		(1 << 0)
#define C_OPERAND_FLAG_NEG		(1 << 1)

struct c_operand {
	struct c_vector		*vector;
	unsigned		swizzle[4];
	unsigned		flag[4];
};

struct c_instruction {
	struct c_instruction	*next, *prev;
	unsigned		opcode;
	unsigned		ninput;
	struct c_operand	input[3];
	struct c_operand	output;
	unsigned		write_mask;
	void			*backend;
};

struct c_node;

struct c_node_link {
	struct c_node_link	*next;
	struct c_node_link	*prev;
	struct c_node		*node;
};

/**
 * struct c_node
 *
 * @next:		all node are in a double linked list, this point to
 * 			next node
 * @next:		all node are in a double linked list, this point to
 * 			previous node
 * @predecessors:	list of all predecessor nodes in the flow graph
 * @successors:		list of all sucessor nodes in the flow graph
 * @parent:		parent node in the depth first walk tree
 * @childs:		child nodes in the depth first walk tree
 */
struct c_node {
	struct c_node		*next, *prev;
	struct c_node_link	predecessors;
	struct c_node_link	successors;
	struct c_node		*parent;
	struct c_node_link	childs;
	struct c_instruction	insts;
	unsigned		opcode;
	unsigned		visited;
	unsigned		done;
	void			*backend;
};

struct c_file {
	unsigned		nvectors;
	struct c_vector		vectors;
};

struct c_shader {
	unsigned			nvectors;
	struct c_file			files[C_FILE_COUNT];
	struct c_node			nodes;
	struct c_node			entry;
	struct c_node			end;
	unsigned			type;
};

int c_shader_init(struct c_shader *shader, unsigned type);
void c_shader_destroy(struct c_shader *shader);
struct c_vector *c_shader_vector_new(struct c_shader *shader, unsigned file, unsigned name, int sid);
int c_shader_build_dominator_tree(struct c_shader *shader);
void c_shader_dump(struct c_shader *shader);

void c_node_init(struct c_node *node);
int c_node_add_new_instruction(struct c_node *node, struct c_instruction *instruction);
int c_node_add_new_instruction_head(struct c_node *node, struct c_instruction *instruction);

/* control flow graph functions */
int c_node_cfg_link(struct c_node *predecessor, struct c_node *successor);
struct c_node *c_node_cfg_new_after(struct c_node *predecessor);
struct c_node *c_shader_cfg_new_node_after(struct c_shader *shader, struct c_node *predecessor);

struct c_vector *c_vector_new(void);

#endif
