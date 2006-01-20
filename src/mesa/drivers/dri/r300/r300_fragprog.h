#ifndef __R300_FRAGPROG_H_
#define __R300_FRAGPROG_H_

#include "glheader.h"
#include "macros.h"
#include "enums.h"

#include "program.h"
#include "r300_context.h"
#include "program_instruction.h"

/* representation of a register for emit_arith/swizzle */
typedef struct _pfs_reg_t {
	enum {
		REG_TYPE_INPUT,
		REG_TYPE_OUTPUT,
		REG_TYPE_TEMP,
		REG_TYPE_CONST
	} type:2;
	GLuint index:6;
	GLuint v_swz:5;
	GLuint s_swz:5;
	GLuint negate:1;    //XXX: we need to handle negate individually
	GLuint absolute:1;
	GLboolean no_use:1;
	GLboolean valid:1;
} pfs_reg_t;

typedef struct r300_fragment_program_swizzle {
	GLuint length;
	GLuint src[4];
	GLuint inst[8];
} r300_fragment_program_swizzle_t;

/* supported hw opcodes */
#define PFS_OP_MAD 0
#define PFS_OP_DP3 1
#define PFS_OP_DP4 2
#define PFS_OP_MIN 3
#define PFS_OP_MAX 4
#define PFS_OP_CMP 5
#define PFS_OP_FRC 6
#define PFS_OP_EX2 7
#define PFS_OP_LG2 8
#define PFS_OP_RCP 9
#define PFS_OP_RSQ 10
#define PFS_OP_REPL_ALPHA 11
#define MAX_PFS_OP 11

#define PFS_FLAG_SAT	(1 << 0)
#define PFS_FLAG_ABS	(1 << 1)

#define ARG_NEG			(1 << 5)
#define ARG_ABS			(1 << 6)
#define ARG_MASK		(127 << 0)
#define ARG_STRIDE		7
#define SRC_CONST		(1 << 5)
#define SRC_MASK		(63 << 0)
#define SRC_STRIDE		6

#define NOP_INST0 ( \
		(R300_FPI0_OUTC_MAD) | \
		(R300_FPI0_ARGC_ZERO << R300_FPI0_ARG0C_SHIFT) | \
		(R300_FPI0_ARGC_ZERO << R300_FPI0_ARG1C_SHIFT) | \
		(R300_FPI0_ARGC_ZERO << R300_FPI0_ARG2C_SHIFT))
#define NOP_INST1 ( \
		((0 | SRC_CONST) << R300_FPI1_SRC0C_SHIFT) | \
		((0 | SRC_CONST) << R300_FPI1_SRC1C_SHIFT) | \
		((0 | SRC_CONST) << R300_FPI1_SRC2C_SHIFT))
#define NOP_INST2 ( \
		(R300_FPI2_OUTA_MAD) | \
		(R300_FPI2_ARGA_ZERO << R300_FPI2_ARG0A_SHIFT) | \
		(R300_FPI2_ARGA_ZERO << R300_FPI2_ARG1A_SHIFT) | \
		(R300_FPI2_ARGA_ZERO << R300_FPI2_ARG2A_SHIFT))
#define NOP_INST3 ( \
		((0 | SRC_CONST) << R300_FPI3_SRC0A_SHIFT) | \
		((0 | SRC_CONST) << R300_FPI3_SRC1A_SHIFT) | \
		((0 | SRC_CONST) << R300_FPI3_SRC2A_SHIFT))


#endif

