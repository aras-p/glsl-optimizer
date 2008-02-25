#ifndef __R300_VERTPROG_H_
#define __R300_VERTPROG_H_

#include "r300_reg.h"

#define VSF_FLAG_X	1
#define VSF_FLAG_Y	2
#define VSF_FLAG_Z	4
#define VSF_FLAG_W	8
#define VSF_FLAG_XYZ	(VSF_FLAG_X | VSF_FLAG_Y | VSF_FLAG_Z)
#define VSF_FLAG_ALL  0xf
#define VSF_FLAG_NONE  0

/* first DWORD of an instruction */

#define PVS_VECTOR_OPCODE(op, out_reg_index, out_reg_fields, class) \
   ((op)  \
  	| ((out_reg_index) << R300_VPI_OUT_REG_INDEX_SHIFT) 	\
 	 | ((out_reg_fields) << 20) 	\
  	| ( (class) << 8 ) )

#define PVS_DST_MATH_INST	(1 << 6)

#define PVS_MATH_OPCODE(op, out_reg_index, out_reg_fields, class) \
   ((op)  \
    | PVS_DST_MATH_INST \
  	| ((out_reg_index) << R300_VPI_OUT_REG_INDEX_SHIFT) 	\
 	 | ((out_reg_fields) << 20) 	\
  	| ( (class) << 8 ) )

/* according to Nikolai, the subsequent 3 DWORDs are sources, use same define for each */

#define VSF_IN_CLASS_TMP	0
#define VSF_IN_CLASS_ATTR	1
#define VSF_IN_CLASS_PARAM	2
#define VSF_IN_CLASS_NONE	9

#define VSF_IN_COMPONENT_X	0
#define VSF_IN_COMPONENT_Y	1
#define VSF_IN_COMPONENT_Z	2
#define VSF_IN_COMPONENT_W	3
#define VSF_IN_COMPONENT_ZERO	4
#define VSF_IN_COMPONENT_ONE	5

#define MAKE_VSF_SOURCE(in_reg_index, comp_x, comp_y, comp_z, comp_w, class, negate) \
	( ((in_reg_index)<<R300_VPI_IN_REG_INDEX_SHIFT) \
	   | ((comp_x)<<R300_VPI_IN_X_SHIFT) \
	   | ((comp_y)<<R300_VPI_IN_Y_SHIFT) \
	   | ((comp_z)<<R300_VPI_IN_Z_SHIFT) \
	   | ((comp_w)<<R300_VPI_IN_W_SHIFT) \
	   | ((negate)<<25) | ((class)))

/* special sources: */

/* (1.0,1.0,1.0,1.0) vector (ATTR, plain ) */
#define VSF_ATTR_UNITY(reg) \
	MAKE_VSF_SOURCE(reg, VSF_IN_COMPONENT_ONE, VSF_IN_COMPONENT_ONE, VSF_IN_COMPONENT_ONE, VSF_IN_COMPONENT_ONE, VSF_IN_CLASS_ATTR, VSF_FLAG_NONE)
#define VSF_UNITY(reg) \
	MAKE_VSF_SOURCE(reg, VSF_IN_COMPONENT_ONE, VSF_IN_COMPONENT_ONE, VSF_IN_COMPONENT_ONE, VSF_IN_COMPONENT_ONE, VSF_IN_CLASS_NONE, VSF_FLAG_NONE)

/* contents of unmodified register */
#define VSF_REG(reg) \
	MAKE_VSF_SOURCE(reg, VSF_IN_COMPONENT_X, VSF_IN_COMPONENT_Y, VSF_IN_COMPONENT_Z, VSF_IN_COMPONENT_W, VSF_IN_CLASS_ATTR, VSF_FLAG_NONE)

/* contents of unmodified parameter */
#define VSF_PARAM(reg) \
	MAKE_VSF_SOURCE(reg, VSF_IN_COMPONENT_X, VSF_IN_COMPONENT_Y, VSF_IN_COMPONENT_Z, VSF_IN_COMPONENT_W, VSF_IN_CLASS_PARAM, VSF_FLAG_NONE)

/* contents of unmodified temporary register */
#define VSF_TMP(reg) \
	MAKE_VSF_SOURCE(reg, VSF_IN_COMPONENT_X, VSF_IN_COMPONENT_Y, VSF_IN_COMPONENT_Z, VSF_IN_COMPONENT_W, VSF_IN_CLASS_TMP, VSF_FLAG_NONE)

/* components of ATTR register */
#define VSF_ATTR_X(reg) \
	MAKE_VSF_SOURCE(reg, VSF_IN_COMPONENT_X, VSF_IN_COMPONENT_X, VSF_IN_COMPONENT_X, VSF_IN_COMPONENT_X, VSF_IN_CLASS_ATTR, VSF_FLAG_NONE)
#define VSF_ATTR_Y(reg) \
	MAKE_VSF_SOURCE(reg, VSF_IN_COMPONENT_Y, VSF_IN_COMPONENT_Y, VSF_IN_COMPONENT_Y, VSF_IN_COMPONENT_Y, VSF_IN_CLASS_ATTR, VSF_FLAG_NONE)
#define VSF_ATTR_Z(reg) \
	MAKE_VSF_SOURCE(reg, VSF_IN_COMPONENT_Z, VSF_IN_COMPONENT_Z, VSF_IN_COMPONENT_Z, VSF_IN_COMPONENT_Z, VSF_IN_CLASS_ATTR, VSF_FLAG_NONE)
#define VSF_ATTR_W(reg) \
	MAKE_VSF_SOURCE(reg, VSF_IN_COMPONENT_W, VSF_IN_COMPONENT_W, VSF_IN_COMPONENT_W, VSF_IN_COMPONENT_W, VSF_IN_CLASS_ATTR, VSF_FLAG_NONE)

#if 1

/**
 * Vertex program helper macros
 */

/* Produce out dword */
#define VP_OUTCLASS_TMP		R300_VPI_OUT_REG_CLASS_TEMPORARY
#define VP_OUTCLASS_OUT		R300_VPI_OUT_REG_CLASS_RESULT

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
	((outidx) << R300_VPI_OUT_REG_INDEX_SHIFT) |		\
	VP_OUTCLASS_##outclass |				\
	VP_OUTMASK_##outmask)

/* Produce in dword */
#define VP_INCLASS_TMP		R300_VPI_IN_REG_CLASS_TEMPORARY
#define VP_INCLASS_IN		R300_VPI_IN_REG_CLASS_ATTRIBUTE
#define VP_INCLASS_CONST	R300_VPI_IN_REG_CLASS_PARAMETER

#define VP_IN(class,idx) \
	(((idx) << R300_VPI_IN_REG_INDEX_SHIFT) |		\
	VP_INCLASS_##class |					\
	(R300_VPI_IN_SELECT_X << R300_VPI_IN_X_SHIFT) |		\
	(R300_VPI_IN_SELECT_Y << R300_VPI_IN_Y_SHIFT) |		\
	(R300_VPI_IN_SELECT_Z << R300_VPI_IN_Z_SHIFT) |		\
	(R300_VPI_IN_SELECT_W << R300_VPI_IN_W_SHIFT))
#define VP_ZERO() \
	((R300_VPI_IN_SELECT_ZERO << R300_VPI_IN_X_SHIFT) |	\
	(R300_VPI_IN_SELECT_ZERO << R300_VPI_IN_Y_SHIFT) |	\
	(R300_VPI_IN_SELECT_ZERO << R300_VPI_IN_Z_SHIFT) |	\
	(R300_VPI_IN_SELECT_ZERO << R300_VPI_IN_W_SHIFT))
#define VP_ONE() \
	((R300_VPI_IN_SELECT_ONE << R300_VPI_IN_X_SHIFT) |	\
	(R300_VPI_IN_SELECT_ONE << R300_VPI_IN_Y_SHIFT) |	\
	(R300_VPI_IN_SELECT_ONE << R300_VPI_IN_Z_SHIFT) |	\
	(R300_VPI_IN_SELECT_ONE << R300_VPI_IN_W_SHIFT))

#define VP_NEG(in,comp)		((in) ^ (R300_VPI_IN_NEG_##comp))
#define VP_NEGALL(in,comp)	VP_NEG(VP_NEG(VP_NEG(VP_NEG((in),X),Y),Z),W)

#endif

#endif
