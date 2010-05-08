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
#ifndef R600_SHADER_H
#define R600_SHADER_H

#include "r600_compiler.h"
#include "radeon.h"

struct r600_state;

struct r600_shader_vfetch {
	struct r600_shader_vfetch	*next;
	struct r600_shader_vfetch	*prev;
	unsigned			cf_addr;
	unsigned			sel[4];
	unsigned			chan[4];
	unsigned			dsel[4];
};

struct r600_shader_operand {
	struct c_vector			*vector;
	unsigned			sel;
	unsigned			chan;
	unsigned			neg;
	unsigned			abs;
};

struct r600_shader_inst {
	unsigned			is_op3;
	unsigned			opcode;
	unsigned			inst;
	struct r600_shader_operand	src[3];
	struct r600_shader_operand	dst;
	unsigned			last;
};

struct r600_shader_alu {
	struct r600_shader_alu		*next;
	struct r600_shader_alu		*prev;
	unsigned			nalu;
	unsigned			nliteral;
	struct r600_shader_inst		alu[5];
	u32				literal[4];
};

struct r600_shader_node {
	struct r600_shader_node		*next;
	struct r600_shader_node		*prev;
	unsigned			cf_id;		/**< cf index (in dw) in byte code */
	unsigned			cf_addr;	/**< instructions index (in dw) in byte code */
	unsigned			nslot;		/**< number of slot (2 dw) needed by this node */
	unsigned			nfetch;
	struct c_node			*node;		/**< compiler node from which this node originate */
	struct r600_shader_vfetch	vfetch;		/**< list of vfetch instructions */
	struct r600_shader_alu		alu;		/**< list of alu instructions */
};

struct r600_shader_io {
	unsigned	name;
	unsigned	gpr;
	int		sid;
};

struct r600_shader {
	unsigned			stack_size;		/**< stack size needed by this shader */
	unsigned			ngpr;			/**< number of GPR needed by this shader */
	unsigned			nconstant;		/**< number of constants used by this shader */
	unsigned			nresource;		/**< number of resources used by this shader */
	unsigned			noutput;
	unsigned			ninput;
	unsigned			nvector;
	unsigned			ncf;			/**< total number of cf clauses */
	unsigned			nslot;			/**< total number of slots (2 dw) */
	unsigned			flat_shade;		/**< are we flat shading */
	struct r600_shader_node		nodes;			/**< list of node */
	struct r600_shader_node		*cur_node;		/**< current node (used during nodes list creation) */
	struct r600_shader_io		input[32];
	struct r600_shader_io		output[32];
	/* TODO replace GPR by some better register allocator */
	struct c_vector			**gpr;
	unsigned			ndw;			/**< bytes code size in dw */
	u32				*bcode;			/**< bytes code */
	enum pipe_format		resource_format[160];	/**< format of resource */
	struct c_shader			cshader;
};

void r600_shader_cleanup(struct r600_shader *rshader);
int r600_shader_register(struct r600_shader *rshader);
int r600_shader_node(struct r600_shader *shader);
void r600_shader_node_place(struct r600_shader *rshader);
int r600_shader_find_gpr(struct r600_shader *rshader, struct c_vector *v,
			unsigned swizzle, unsigned *sel, unsigned *chan);
int r600_shader_vfetch_bytecode(struct r600_shader *rshader,
				struct r600_shader_node *rnode,
				struct r600_shader_vfetch *vfetch,
				unsigned *cid);
int r600_shader_update(struct r600_shader *rshader,
			enum pipe_format *resource_format);
int r600_shader_legalize(struct r600_shader *rshader);

int r700_shader_translate(struct r600_shader *rshader);

int c_shader_from_tgsi(struct c_shader *shader, unsigned type,
			const struct tgsi_token *tokens);
int r600_shader_register(struct r600_shader *rshader);
int r600_shader_translate_rec(struct r600_shader *rshader, struct c_node *node);
int r700_shader_translate(struct r600_shader *rshader);
int r600_shader_insert_fetch(struct c_shader *shader);

#endif
