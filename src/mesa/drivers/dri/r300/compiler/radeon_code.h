/*
 * Copyright 2009 Nicolai Hähnle <nhaehnle@gmail.com>
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
 * USE OR OTHER DEALINGS IN THE SOFTWARE. */

#ifndef RADEON_CODE_H
#define RADEON_CODE_H


#define R300_PFS_MAX_ALU_INST     64
#define R300_PFS_MAX_TEX_INST     32
#define R300_PFS_MAX_TEX_INDIRECT 4
#define R300_PFS_NUM_TEMP_REGS    32
#define R300_PFS_NUM_CONST_REGS   32

#define R500_PFS_MAX_INST         512
#define R500_PFS_NUM_TEMP_REGS    128
#define R500_PFS_NUM_CONST_REGS   256


#define STATE_R300_WINDOW_DIMENSION (STATE_INTERNAL_DRIVER+0)
#define STATE_R300_TEXRECT_FACTOR (STATE_INTERNAL_DRIVER+1)


/**
 * Stores state that influences the compilation of a fragment program.
 */
struct r300_fragment_program_external_state {
	struct {
		/**
		 * If the sampler is used as a shadow sampler,
		 * this field is:
		 *  0 - GL_LUMINANCE
		 *  1 - GL_INTENSITY
		 *  2 - GL_ALPHA
		 * depending on the depth texture mode.
		 */
		GLuint depth_texture_mode : 2;

		/**
		 * If the sampler is used as a shadow sampler,
		 * this field is (texture_compare_func - GL_NEVER).
		 * [e.g. if compare function is GL_LEQUAL, this field is 3]
		 *
		 * Otherwise, this field is 0.
		 */
		GLuint texture_compare_func : 3;
	} unit[16];
};



struct r300_fragment_program_node {
	int tex_offset; /**< first tex instruction */
	int tex_end; /**< last tex instruction, relative to tex_offset */
	int alu_offset; /**< first ALU instruction */
	int alu_end; /**< last ALU instruction, relative to alu_offset */
	int flags;
};

/**
 * Stores an R300 fragment program in its compiled-to-hardware form.
 */
struct r300_fragment_program_code {
	struct {
		int length; /**< total # of texture instructions used */
		GLuint inst[R300_PFS_MAX_TEX_INST];
	} tex;

	struct {
		int length; /**< total # of ALU instructions used */
		struct {
			GLuint inst0;
			GLuint inst1;
			GLuint inst2;
			GLuint inst3;
		} inst[R300_PFS_MAX_ALU_INST];
	} alu;

	struct r300_fragment_program_node node[4];
	int cur_node;
	int first_node_has_tex;

	/**
	 * Remember which program register a given hardware constant
	 * belongs to.
	 */
	struct prog_src_register constant[R300_PFS_NUM_CONST_REGS];
	int const_nr;

	int max_temp_idx;
};


struct r500_fragment_program_code {
	struct {
		GLuint inst0;
		GLuint inst1;
		GLuint inst2;
		GLuint inst3;
		GLuint inst4;
		GLuint inst5;
	} inst[R500_PFS_MAX_INST];

	int inst_offset;
	int inst_end;

	/**
	 * Remember which program register a given hardware constant
	 * belongs to.
	 */
	struct prog_src_register constant[R500_PFS_NUM_CONST_REGS];
	int const_nr;

	int max_temp_idx;
};

struct rX00_fragment_program_code {
	union {
		struct r300_fragment_program_code r300;
		struct r500_fragment_program_code r500;
	} code;

	GLboolean writes_depth;

	/* attribute that we are sending the WPOS in */
	gl_frag_attrib wpos_attr;
	/* attribute that we are sending the fog coordinate in */
	gl_frag_attrib fog_attr;
};


#define VSF_MAX_FRAGMENT_LENGTH (255*4)
#define VSF_MAX_FRAGMENT_TEMPS (14)

struct r300_vertex_program_external_state {
	GLuint FpReads;
	GLuint FogAttr;
	GLuint WPosAttr;
};

struct r300_vertex_program_code {
	int length;
	union {
		GLuint d[VSF_MAX_FRAGMENT_LENGTH];
		float f[VSF_MAX_FRAGMENT_LENGTH];
	} body;

	int pos_end;
	int num_temporaries;	/* Number of temp vars used by program */
	int inputs[VERT_ATTRIB_MAX];
	int outputs[VERT_RESULT_MAX];

	GLbitfield InputsRead;
	GLbitfield OutputsWritten;
};

#endif /* RADEON_CODE_H */