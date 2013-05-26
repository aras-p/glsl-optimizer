/*
 * Copyright (c) 2013 Rob Clark <robdclark@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef IR3_H_
#define IR3_H_

#include <stdint.h>
#include <stdbool.h>

#include "instr-a3xx.h"

/* low level intermediate representation of an adreno shader program */

struct ir3_shader;

struct ir3_shader * fd_asm_parse(const char *src);

struct ir3_shader_info {
	uint16_t sizedwords;
	/* NOTE: max_reg, etc, does not include registers not touched
	 * by the shader (ie. vertex fetched via VFD_DECODE but not
	 * touched by shader)
	 */
	int8_t   max_reg;   /* highest GPR # used by shader */
	int8_t   max_half_reg;
	int8_t   max_const;
};

struct ir3_register {
	enum {
		IR3_REG_CONST  = 0x001,
		IR3_REG_IMMED  = 0x002,
		IR3_REG_HALF   = 0x004,
		IR3_REG_RELATIV= 0x008,
		IR3_REG_R      = 0x010,
		IR3_REG_NEGATE = 0x020,
		IR3_REG_ABS    = 0x040,
		IR3_REG_EVEN   = 0x080,
		IR3_REG_POS_INF= 0x100,
		/* (ei) flag, end-input?  Set on last bary, presumably to signal
		 * that the shader needs no more input:
		 */
		IR3_REG_EI     = 0x200,
	} flags;
	union {
		/* normal registers: */
		struct {
			/* the component is in the low two bits of the reg #, so
			 * rN.x becomes: (n << 2) | x
			 */
			int num;
			int wrmask;
		};
		/* immediate: */
		int     iim_val;
		float   fim_val;
		/* relative: */
		int offset;
	};
};

struct ir3_instruction {
	struct ir3_shader *shader;
	int category;
	opc_t opc;
	enum {
		/* (sy) flag is set on first instruction, and after sample
		 * instructions (probably just on RAW hazard).
		 */
		IR3_INSTR_SY    = 0x001,
		/* (ss) flag is set on first instruction, and first instruction
		 * to depend on the result of "long" instructions (RAW hazard):
		 *
		 *   rcp, rsq, log2, exp2, sin, cos, sqrt
		 *
		 * It seems to synchronize until all in-flight instructions are
		 * completed, for example:
		 *
		 *   rsq hr1.w, hr1.w
		 *   add.f hr2.z, (neg)hr2.z, hc0.y
		 *   mul.f hr2.w, (neg)hr2.y, (neg)hr2.y
		 *   rsq hr2.x, hr2.x
		 *   (rpt1)nop
		 *   mad.f16 hr2.w, hr2.z, hr2.z, hr2.w
		 *   nop
		 *   mad.f16 hr2.w, (neg)hr0.w, (neg)hr0.w, hr2.w
		 *   (ss)(rpt2)mul.f hr1.x, (r)hr1.x, hr1.w
		 *   (rpt2)mul.f hr0.x, (neg)(r)hr0.x, hr2.x
		 *
		 * The last mul.f does not have (ss) set, presumably because the
		 * (ss) on the previous instruction does the job.
		 *
		 * The blob driver also seems to set it on WAR hazards, although
		 * not really clear if this is needed or just blob compiler being
		 * sloppy.  So far I haven't found a case where removing the (ss)
		 * causes problems for WAR hazard, but I could just be getting
		 * lucky:
		 *
		 *   rcp r1.y, r3.y
		 *   (ss)(rpt2)mad.f32 r3.y, (r)c9.x, r1.x, (r)r3.z
		 *
		 */
		IR3_INSTR_SS    = 0x002,
		/* (jp) flag is set on jump targets:
		 */
		IR3_INSTR_JP    = 0x004,
		IR3_INSTR_UL    = 0x008,
		IR3_INSTR_3D    = 0x010,
		IR3_INSTR_A     = 0x020,
		IR3_INSTR_O     = 0x040,
		IR3_INSTR_P     = 0x080,
		IR3_INSTR_S     = 0x100,
		IR3_INSTR_S2EN  = 0x200,
	} flags;
	int repeat;
	unsigned regs_count;
	struct ir3_register *regs[4];
	union {
		struct {
			char inv;
			char comp;
			int  immed;
		} cat0;
		struct {
			type_t src_type, dst_type;
		} cat1;
		struct {
			enum {
				IR3_COND_LT = 0,
				IR3_COND_LE = 1,
				IR3_COND_GT = 2,
				IR3_COND_GE = 3,
				IR3_COND_EQ = 4,
				IR3_COND_NE = 5,
			} condition;
		} cat2;
		struct {
			unsigned samp, tex;
			type_t type;
		} cat5;
		struct {
			type_t type;
			int offset;
			int iim_val;
		} cat6;
	};
};

/* this is just large to cope w/ the large test *.asm: */
#define MAX_INSTRS 10240

struct ir3_shader {
	unsigned instrs_count;
	struct ir3_instruction *instrs[MAX_INSTRS];
	uint32_t heap[128 * MAX_INSTRS];
	unsigned heap_idx;
};

struct ir3_shader * ir3_shader_create(void);
void ir3_shader_destroy(struct ir3_shader *shader);
void * ir3_shader_assemble(struct ir3_shader *shader,
		struct ir3_shader_info *info);

struct ir3_instruction * ir3_instr_create(struct ir3_shader *shader, int category, opc_t opc);
struct ir3_instruction * ir3_instr_clone(struct ir3_instruction *instr);

struct ir3_register * ir3_reg_create(struct ir3_instruction *instr,
		int num, int flags);

#endif /* IR3_H_ */
