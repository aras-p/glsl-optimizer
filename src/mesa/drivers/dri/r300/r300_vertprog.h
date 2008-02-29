#ifndef __R300_VERTPROG_H_
#define __R300_VERTPROG_H_

#include "r300_reg.h"

#define R300_VPI_IN_REG_INDEX_SHIFT             5
	/* GUESS based on fglrx native limits */
#define R300_VPI_IN_REG_INDEX_MASK              (255 << 5)

#define R300_VPI_IN_X_SHIFT                     13
#define R300_VPI_IN_Y_SHIFT                     16
#define R300_VPI_IN_Z_SHIFT                     19
#define R300_VPI_IN_W_SHIFT                     22

#define R300_VPI_IN_NEG_X                       (1 << 25)
#define R300_VPI_IN_NEG_Y                       (1 << 26)
#define R300_VPI_IN_NEG_Z                       (1 << 27)
#define R300_VPI_IN_NEG_W                       (1 << 28)

#define PVS_VECTOR_OPCODE(opcode, reg_index, reg_writemask, reg_class)	\
	 (((opcode & PVS_DST_OPCODE_MASK) << PVS_DST_OPCODE_SHIFT)	\
	 | ((reg_index & PVS_DST_OFFSET_MASK) << PVS_DST_OFFSET_SHIFT)	\
	 | ((reg_writemask & 0xf) << PVS_DST_WE_X_SHIFT)	/* X Y Z W */	\
	 | ((reg_class & PVS_DST_REG_TYPE_MASK) << PVS_DST_REG_TYPE_SHIFT))

#define PVS_MATH_OPCODE(opcode, reg_index, reg_writemask, reg_class)	\
	 (((opcode & PVS_DST_OPCODE_MASK) << PVS_DST_OPCODE_SHIFT)	\
	 | ((1 & PVS_DST_MATH_INST_MASK) << PVS_DST_MATH_INST_SHIFT)	\
	 | ((reg_index & PVS_DST_OFFSET_MASK) << PVS_DST_OFFSET_SHIFT)	\
	 | ((reg_writemask & 0xf) << PVS_DST_WE_X_SHIFT)	/* X Y Z W */	\
	 | ((reg_class & PVS_DST_REG_TYPE_MASK) << PVS_DST_REG_TYPE_SHIFT))


#define PVS_SOURCE_OPCODE(in_reg_index, comp_x, comp_y, comp_z, comp_w, reg_class, negate)	\
	(((in_reg_index) << R300_VPI_IN_REG_INDEX_SHIFT)					\
	 | ((comp_x) << R300_VPI_IN_X_SHIFT)							\
	 | ((comp_y) << R300_VPI_IN_Y_SHIFT)							\
	 | ((comp_z) << R300_VPI_IN_Z_SHIFT)							\
	 | ((comp_w) << R300_VPI_IN_W_SHIFT)							\
	 | ((negate) << 25)									\
	 | ((reg_class)))

#if 1

#define VSF_FLAG_X	1
#define VSF_FLAG_Y	2
#define VSF_FLAG_Z	4
#define VSF_FLAG_W	8
#define VSF_FLAG_XYZ	(VSF_FLAG_X | VSF_FLAG_Y | VSF_FLAG_Z)
#define VSF_FLAG_ALL  0xf
#define VSF_FLAG_NONE  0

#define R300_VPI_OUT_WRITE_X                    (1 << 20)
#define R300_VPI_OUT_WRITE_Y                    (1 << 21)
#define R300_VPI_OUT_WRITE_Z                    (1 << 22)
#define R300_VPI_OUT_WRITE_W                    (1 << 23)

#define VP_OUTMASK_X	R300_VPI_OUT_WRITE_X
#define VP_OUTMASK_Y	R300_VPI_OUT_WRITE_Y
#define VP_OUTMASK_Z	R300_VPI_OUT_WRITE_Z
#define VP_OUTMASK_W	R300_VPI_OUT_WRITE_W
#define VP_OUTMASK_XY	(VP_OUTMASK_X|VP_OUTMASK_Y)
#define VP_OUTMASK_XZ	(VP_OUTMASK_X|VP_OUTMASK_Z)
#define VP_OUTMASK_XW	(VP_OUTMASK_X|VP_OUTMASK_W)
#define VP_OUTMASK_XYZ	(VP_OUTMASK_XY|VP_OUTMASK_Z)
#define VP_OUTMASK_XYW	(VP_OUTMASK_XY|VP_OUTMASK_W)
#define VP_OUTMASK_XZW	(VP_OUTMASK_XZ|VP_OUTMASK_W)
#define VP_OUTMASK_XYZW	(VP_OUTMASK_XYZ|VP_OUTMASK_W)
#define VP_OUTMASK_YZ	(VP_OUTMASK_Y|VP_OUTMASK_Z)
#define VP_OUTMASK_YW	(VP_OUTMASK_Y|VP_OUTMASK_W)
#define VP_OUTMASK_YZW	(VP_OUTMASK_YZ|VP_OUTMASK_W)
#define VP_OUTMASK_ZW	(VP_OUTMASK_Z|VP_OUTMASK_W)

#define VP_OUT(instr,outclass,outidx,outmask) \
	(VE_##instr |				\
	((outidx & PVS_DST_OFFSET_MASK) << PVS_DST_OFFSET_SHIFT) |		\
	((PVS_DST_REG_##outclass & PVS_DST_REG_TYPE_MASK) << PVS_DST_REG_TYPE_SHIFT) |		\
	VP_OUTMASK_##outmask)

#define VP_IN(inclass,inidx) \
	(((inidx) << R300_VPI_IN_REG_INDEX_SHIFT) |		\
	(PVS_SRC_REG_##inclass << 0) |			\
	(PVS_SRC_SELECT_X << R300_VPI_IN_X_SHIFT) |		\
	(PVS_SRC_SELECT_Y << R300_VPI_IN_Y_SHIFT) |		\
	(PVS_SRC_SELECT_Z << R300_VPI_IN_Z_SHIFT) |		\
	(PVS_SRC_SELECT_W << R300_VPI_IN_W_SHIFT))
#define VP_ZERO() \
	((PVS_SRC_SELECT_FORCE_0 << R300_VPI_IN_X_SHIFT) |	\
	(PVS_SRC_SELECT_FORCE_0 << R300_VPI_IN_Y_SHIFT) |	\
	(PVS_SRC_SELECT_FORCE_0 << R300_VPI_IN_Z_SHIFT) |	\
	(PVS_SRC_SELECT_FORCE_0 << R300_VPI_IN_W_SHIFT))
#define VP_ONE() \
	((PVS_SRC_SELECT_FORCE_1 << R300_VPI_IN_X_SHIFT) |	\
	(PVS_SRC_SELECT_FORCE_1 << R300_VPI_IN_Y_SHIFT) |	\
	(PVS_SRC_SELECT_FORCE_1 << R300_VPI_IN_Z_SHIFT) |	\
	(PVS_SRC_SELECT_FORCE_1 << R300_VPI_IN_W_SHIFT))

#define VP_NEG(in,comp)		((in) ^ (R300_VPI_IN_NEG_##comp))
#define VP_NEGALL(in,comp)	VP_NEG(VP_NEG(VP_NEG(VP_NEG((in),X),Y),Z),W)

#endif

#endif
