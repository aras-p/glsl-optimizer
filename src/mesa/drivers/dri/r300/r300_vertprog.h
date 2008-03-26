#ifndef __R300_VERTPROG_H_
#define __R300_VERTPROG_H_

#include "r300_reg.h"

#define PVS_OP_DST_OPERAND(opcode, math_inst, macro_inst, reg_index, reg_writemask, reg_class)	\
	 (((opcode & PVS_DST_OPCODE_MASK) << PVS_DST_OPCODE_SHIFT)	\
	 | ((math_inst & PVS_DST_MATH_INST_MASK) << PVS_DST_MATH_INST_SHIFT)	\
	 | ((macro_inst & PVS_DST_MACRO_INST_MASK) << PVS_DST_MACRO_INST_SHIFT)	\
	 | ((reg_index & PVS_DST_OFFSET_MASK) << PVS_DST_OFFSET_SHIFT)	\
	 | ((reg_writemask & 0xf) << PVS_DST_WE_X_SHIFT)	/* X Y Z W */	\
	 | ((reg_class & PVS_DST_REG_TYPE_MASK) << PVS_DST_REG_TYPE_SHIFT))

#define PVS_SRC_OPERAND(in_reg_index, comp_x, comp_y, comp_z, comp_w, reg_class, negate)	\
	(((in_reg_index & PVS_SRC_OFFSET_MASK) << PVS_SRC_OFFSET_SHIFT)				\
	 | ((comp_x & PVS_SRC_SWIZZLE_X_MASK) << PVS_SRC_SWIZZLE_X_SHIFT)			\
	 | ((comp_y & PVS_SRC_SWIZZLE_Y_MASK) << PVS_SRC_SWIZZLE_Y_SHIFT)			\
	 | ((comp_z & PVS_SRC_SWIZZLE_Z_MASK) << PVS_SRC_SWIZZLE_Z_SHIFT)			\
	 | ((comp_w & PVS_SRC_SWIZZLE_W_MASK) << PVS_SRC_SWIZZLE_W_SHIFT)			\
	 | ((negate & 0xf) << PVS_SRC_MODIFIER_X_SHIFT)	/* X Y Z W */				\
	 | ((reg_class & PVS_SRC_REG_TYPE_MASK) << PVS_SRC_REG_TYPE_SHIFT))

#if 1

#define VSF_FLAG_X	1
#define VSF_FLAG_Y	2
#define VSF_FLAG_Z	4
#define VSF_FLAG_W	8
#define VSF_FLAG_XYZ	(VSF_FLAG_X | VSF_FLAG_Y | VSF_FLAG_Z)
#define VSF_FLAG_ALL  0xf
#define VSF_FLAG_NONE  0

#endif

#endif
