#ifndef __NVGL_PIPE_H__
#define __NVGL_PIPE_H__

#include <GL/gl.h>

static INLINE unsigned
nvgl_blend_func(unsigned factor)
{
	switch (factor) {
	case PIPE_BLENDFACTOR_ONE:
		return GL_ONE;
	case PIPE_BLENDFACTOR_SRC_COLOR:
		return GL_SRC_COLOR;
	case PIPE_BLENDFACTOR_SRC_ALPHA:
		return GL_SRC_ALPHA;
	case PIPE_BLENDFACTOR_DST_ALPHA:
		return GL_DST_ALPHA;
	case PIPE_BLENDFACTOR_DST_COLOR:
		return GL_DST_COLOR;
	case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
		return GL_SRC_ALPHA_SATURATE;
	case PIPE_BLENDFACTOR_CONST_COLOR:
		return GL_CONSTANT_COLOR;
	case PIPE_BLENDFACTOR_CONST_ALPHA:
		return GL_CONSTANT_ALPHA;
	case PIPE_BLENDFACTOR_ZERO:
		return GL_ZERO;
	case PIPE_BLENDFACTOR_INV_SRC_COLOR:
		return GL_ONE_MINUS_SRC_COLOR;
	case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
		return GL_ONE_MINUS_SRC_ALPHA;
	case PIPE_BLENDFACTOR_INV_DST_ALPHA:
		return GL_ONE_MINUS_DST_ALPHA;
	case PIPE_BLENDFACTOR_INV_DST_COLOR:
		return GL_ONE_MINUS_DST_COLOR;
	case PIPE_BLENDFACTOR_INV_CONST_COLOR:
		return GL_ONE_MINUS_CONSTANT_COLOR;
	case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
		return GL_ONE_MINUS_CONSTANT_ALPHA;
	default:
		return GL_ONE;
	}
}

static INLINE unsigned
nvgl_blend_eqn(unsigned func)
{
	switch (func) {
	case PIPE_BLEND_ADD:
		return GL_FUNC_ADD;
	case PIPE_BLEND_SUBTRACT:
		return GL_FUNC_SUBTRACT;
	case PIPE_BLEND_REVERSE_SUBTRACT:
		return GL_FUNC_REVERSE_SUBTRACT;
	case PIPE_BLEND_MIN:
		return GL_MIN;
	case PIPE_BLEND_MAX:
		return GL_MAX;
	default:
		return GL_FUNC_ADD;
	}
}

static INLINE unsigned
nvgl_logicop_func(unsigned func)
{
	switch (func) {
	case PIPE_LOGICOP_CLEAR:
		return GL_CLEAR;
	case PIPE_LOGICOP_NOR:
		return GL_NOR;
	case PIPE_LOGICOP_AND_INVERTED:
		return GL_AND_INVERTED;
	case PIPE_LOGICOP_COPY_INVERTED:
		return GL_COPY_INVERTED;
	case PIPE_LOGICOP_AND_REVERSE:
		return GL_AND_REVERSE;
	case PIPE_LOGICOP_INVERT:
		return GL_INVERT;
	case PIPE_LOGICOP_XOR:
		return GL_XOR;
	case PIPE_LOGICOP_NAND:
		return GL_NAND;
	case PIPE_LOGICOP_AND:
		return GL_AND;
	case PIPE_LOGICOP_EQUIV:
		return GL_EQUIV;
	case PIPE_LOGICOP_NOOP:
		return GL_NOOP;
	case PIPE_LOGICOP_OR_INVERTED:
		return GL_OR_INVERTED;
	case PIPE_LOGICOP_COPY:
		return GL_COPY;
	case PIPE_LOGICOP_OR_REVERSE:
		return GL_OR_REVERSE;
	case PIPE_LOGICOP_OR:
		return GL_OR;
	case PIPE_LOGICOP_SET:
		return GL_SET;
	default:
		return GL_CLEAR;
	}
}

static INLINE unsigned
nvgl_comparison_op(unsigned op)
{
	switch (op) {
	case PIPE_FUNC_NEVER:
		return GL_NEVER;
	case PIPE_FUNC_LESS:
		return GL_LESS;
	case PIPE_FUNC_EQUAL:
		return GL_EQUAL;
	case PIPE_FUNC_LEQUAL:
		return GL_LEQUAL;
	case PIPE_FUNC_GREATER:
		return GL_GREATER;
	case PIPE_FUNC_NOTEQUAL:
		return GL_NOTEQUAL;
	case PIPE_FUNC_GEQUAL:
		return GL_GEQUAL;
	case PIPE_FUNC_ALWAYS:
		return GL_ALWAYS;
	default:
		return GL_NEVER;
	}
}

static INLINE unsigned
nvgl_polygon_mode(unsigned mode)
{
	switch (mode) {
	case PIPE_POLYGON_MODE_FILL:
		return GL_FILL;
	case PIPE_POLYGON_MODE_LINE:
		return GL_LINE;
	case PIPE_POLYGON_MODE_POINT:
		return GL_POINT;
	default:
		return GL_FILL;
	}
}

static INLINE unsigned
nvgl_stencil_op(unsigned op)
{
	switch (op) {
	case PIPE_STENCIL_OP_KEEP:
		return GL_KEEP;
	case PIPE_STENCIL_OP_ZERO:
		return GL_ZERO;
	case PIPE_STENCIL_OP_REPLACE:
		return GL_REPLACE;
	case PIPE_STENCIL_OP_INCR:
		return GL_INCR;
	case PIPE_STENCIL_OP_DECR:
		return GL_DECR;
	case PIPE_STENCIL_OP_INCR_WRAP:
		return GL_INCR_WRAP;
	case PIPE_STENCIL_OP_DECR_WRAP:
		return GL_DECR_WRAP;
	case PIPE_STENCIL_OP_INVERT:
		return GL_INVERT;
	default:
		return GL_KEEP;
	}
}

static INLINE unsigned
nvgl_primitive(unsigned prim) {
	switch (prim) {
	case PIPE_PRIM_POINTS:
		return GL_POINTS + 1;
	case PIPE_PRIM_LINES:
		return GL_LINES + 1;
	case PIPE_PRIM_LINE_LOOP:
		return GL_LINE_LOOP + 1;
	case PIPE_PRIM_LINE_STRIP:
		return GL_LINE_STRIP + 1;
	case PIPE_PRIM_TRIANGLES:
		return GL_TRIANGLES + 1;
	case PIPE_PRIM_TRIANGLE_STRIP:
		return GL_TRIANGLE_STRIP + 1;
	case PIPE_PRIM_TRIANGLE_FAN:
		return GL_TRIANGLE_FAN + 1;
	case PIPE_PRIM_QUADS:
		return GL_QUADS + 1;
	case PIPE_PRIM_QUAD_STRIP:
		return GL_QUAD_STRIP + 1;
	case PIPE_PRIM_POLYGON:
		return GL_POLYGON + 1;
	default:
		return GL_POINTS + 1;
	}
}

#endif
