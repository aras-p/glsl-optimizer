/*
 * Mesa 3-D graphics library
 * Version:  7.7
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file mtypes.h
 * Main Mesa data structures.
 *
 * Please try to mark derived values with a leading underscore ('_').
 */

#ifndef MTYPES_H
#define MTYPES_H

#define MAX_DRAW_BUFFERS 8
#define MAX_VARYING 16

#include <GL/gl.h>

/**
 * Indexes for vertex program attributes.
 * GL_NV_vertex_program aliases generic attributes over the conventional
 * attributes.  In GL_ARB_vertex_program shader the aliasing is optional.
 * In GL_ARB_vertex_shader / OpenGL 2.0 the aliasing is disallowed (the
 * generic attributes are distinct/separate).
 */
typedef enum
{
   VERT_ATTRIB_POS = 0,
   VERT_ATTRIB_WEIGHT = 1,
   VERT_ATTRIB_NORMAL = 2,
   VERT_ATTRIB_COLOR0 = 3,
   VERT_ATTRIB_COLOR1 = 4,
   VERT_ATTRIB_FOG = 5,
   VERT_ATTRIB_COLOR_INDEX = 6,
   VERT_ATTRIB_POINT_SIZE = 6,  /*alias*/
   VERT_ATTRIB_EDGEFLAG = 7,
   VERT_ATTRIB_TEX0 = 8,
   VERT_ATTRIB_TEX1 = 9,
   VERT_ATTRIB_TEX2 = 10,
   VERT_ATTRIB_TEX3 = 11,
   VERT_ATTRIB_TEX4 = 12,
   VERT_ATTRIB_TEX5 = 13,
   VERT_ATTRIB_TEX6 = 14,
   VERT_ATTRIB_TEX7 = 15,
   VERT_ATTRIB_GENERIC0 = 16,
   VERT_ATTRIB_GENERIC1 = 17,
   VERT_ATTRIB_GENERIC2 = 18,
   VERT_ATTRIB_GENERIC3 = 19,
   VERT_ATTRIB_GENERIC4 = 20,
   VERT_ATTRIB_GENERIC5 = 21,
   VERT_ATTRIB_GENERIC6 = 22,
   VERT_ATTRIB_GENERIC7 = 23,
   VERT_ATTRIB_GENERIC8 = 24,
   VERT_ATTRIB_GENERIC9 = 25,
   VERT_ATTRIB_GENERIC10 = 26,
   VERT_ATTRIB_GENERIC11 = 27,
   VERT_ATTRIB_GENERIC12 = 28,
   VERT_ATTRIB_GENERIC13 = 29,
   VERT_ATTRIB_GENERIC14 = 30,
   VERT_ATTRIB_GENERIC15 = 31,
   VERT_ATTRIB_MAX = 32
} gl_vert_attrib;

/**
 * Bitflags for vertex attributes.
 * These are used in bitfields in many places.
 */
/*@{*/
#define VERT_BIT_POS         (1 << VERT_ATTRIB_POS)
#define VERT_BIT_WEIGHT      (1 << VERT_ATTRIB_WEIGHT)
#define VERT_BIT_NORMAL      (1 << VERT_ATTRIB_NORMAL)
#define VERT_BIT_COLOR0      (1 << VERT_ATTRIB_COLOR0)
#define VERT_BIT_COLOR1      (1 << VERT_ATTRIB_COLOR1)
#define VERT_BIT_FOG         (1 << VERT_ATTRIB_FOG)
#define VERT_BIT_COLOR_INDEX (1 << VERT_ATTRIB_COLOR_INDEX)
#define VERT_BIT_EDGEFLAG    (1 << VERT_ATTRIB_EDGEFLAG)
#define VERT_BIT_TEX0        (1 << VERT_ATTRIB_TEX0)
#define VERT_BIT_TEX1        (1 << VERT_ATTRIB_TEX1)
#define VERT_BIT_TEX2        (1 << VERT_ATTRIB_TEX2)
#define VERT_BIT_TEX3        (1 << VERT_ATTRIB_TEX3)
#define VERT_BIT_TEX4        (1 << VERT_ATTRIB_TEX4)
#define VERT_BIT_TEX5        (1 << VERT_ATTRIB_TEX5)
#define VERT_BIT_TEX6        (1 << VERT_ATTRIB_TEX6)
#define VERT_BIT_TEX7        (1 << VERT_ATTRIB_TEX7)
#define VERT_BIT_GENERIC0    (1 << VERT_ATTRIB_GENERIC0)
#define VERT_BIT_GENERIC1    (1 << VERT_ATTRIB_GENERIC1)
#define VERT_BIT_GENERIC2    (1 << VERT_ATTRIB_GENERIC2)
#define VERT_BIT_GENERIC3    (1 << VERT_ATTRIB_GENERIC3)
#define VERT_BIT_GENERIC4    (1 << VERT_ATTRIB_GENERIC4)
#define VERT_BIT_GENERIC5    (1 << VERT_ATTRIB_GENERIC5)
#define VERT_BIT_GENERIC6    (1 << VERT_ATTRIB_GENERIC6)
#define VERT_BIT_GENERIC7    (1 << VERT_ATTRIB_GENERIC7)
#define VERT_BIT_GENERIC8    (1 << VERT_ATTRIB_GENERIC8)
#define VERT_BIT_GENERIC9    (1 << VERT_ATTRIB_GENERIC9)
#define VERT_BIT_GENERIC10   (1 << VERT_ATTRIB_GENERIC10)
#define VERT_BIT_GENERIC11   (1 << VERT_ATTRIB_GENERIC11)
#define VERT_BIT_GENERIC12   (1 << VERT_ATTRIB_GENERIC12)
#define VERT_BIT_GENERIC13   (1 << VERT_ATTRIB_GENERIC13)
#define VERT_BIT_GENERIC14   (1 << VERT_ATTRIB_GENERIC14)
#define VERT_BIT_GENERIC15   (1 << VERT_ATTRIB_GENERIC15)

#define VERT_BIT_TEX(u)  (1 << (VERT_ATTRIB_TEX0 + (u)))
#define VERT_BIT_GENERIC(g)  (1 << (VERT_ATTRIB_GENERIC0 + (g)))
/*@}*/


/**
 * Indexes for vertex program result attributes
 */
typedef enum
{
   VERT_RESULT_HPOS = 0,
   VERT_RESULT_COL0 = 1,
   VERT_RESULT_COL1 = 2,
   VERT_RESULT_FOGC = 3,
   VERT_RESULT_TEX0 = 4,
   VERT_RESULT_TEX1 = 5,
   VERT_RESULT_TEX2 = 6,
   VERT_RESULT_TEX3 = 7,
   VERT_RESULT_TEX4 = 8,
   VERT_RESULT_TEX5 = 9,
   VERT_RESULT_TEX6 = 10,
   VERT_RESULT_TEX7 = 11,
   VERT_RESULT_PSIZ = 12,
   VERT_RESULT_BFC0 = 13,
   VERT_RESULT_BFC1 = 14,
   VERT_RESULT_EDGE = 15,
   VERT_RESULT_VAR0 = 16,  /**< shader varying */
   VERT_RESULT_MAX = (VERT_RESULT_VAR0 + MAX_VARYING)
} gl_vert_result;


/**
 * Indexes for fragment program input attributes.
 */
typedef enum
{
   FRAG_ATTRIB_WPOS = 0,
   FRAG_ATTRIB_COL0 = 1,
   FRAG_ATTRIB_COL1 = 2,
   FRAG_ATTRIB_FOGC = 3,
   FRAG_ATTRIB_TEX0 = 4,
   FRAG_ATTRIB_TEX1 = 5,
   FRAG_ATTRIB_TEX2 = 6,
   FRAG_ATTRIB_TEX3 = 7,
   FRAG_ATTRIB_TEX4 = 8,
   FRAG_ATTRIB_TEX5 = 9,
   FRAG_ATTRIB_TEX6 = 10,
   FRAG_ATTRIB_TEX7 = 11,
   FRAG_ATTRIB_FACE = 12,  /**< front/back face */
   FRAG_ATTRIB_PNTC = 13,  /**< sprite/point coord */
   FRAG_ATTRIB_VAR0 = 14,  /**< shader varying */
   FRAG_ATTRIB_MAX = (FRAG_ATTRIB_VAR0 + MAX_VARYING)
} gl_frag_attrib;

/**
 * Bitflags for fragment program input attributes.
 */
/*@{*/
#define FRAG_BIT_WPOS  (1 << FRAG_ATTRIB_WPOS)
#define FRAG_BIT_COL0  (1 << FRAG_ATTRIB_COL0)
#define FRAG_BIT_COL1  (1 << FRAG_ATTRIB_COL1)
#define FRAG_BIT_FOGC  (1 << FRAG_ATTRIB_FOGC)
#define FRAG_BIT_FACE  (1 << FRAG_ATTRIB_FACE)
#define FRAG_BIT_PNTC  (1 << FRAG_ATTRIB_PNTC)
#define FRAG_BIT_TEX0  (1 << FRAG_ATTRIB_TEX0)
#define FRAG_BIT_TEX1  (1 << FRAG_ATTRIB_TEX1)
#define FRAG_BIT_TEX2  (1 << FRAG_ATTRIB_TEX2)
#define FRAG_BIT_TEX3  (1 << FRAG_ATTRIB_TEX3)
#define FRAG_BIT_TEX4  (1 << FRAG_ATTRIB_TEX4)
#define FRAG_BIT_TEX5  (1 << FRAG_ATTRIB_TEX5)
#define FRAG_BIT_TEX6  (1 << FRAG_ATTRIB_TEX6)
#define FRAG_BIT_TEX7  (1 << FRAG_ATTRIB_TEX7)
#define FRAG_BIT_VAR0  (1 << FRAG_ATTRIB_VAR0)

#define FRAG_BIT_TEX(U)  (FRAG_BIT_TEX0 << (U))
#define FRAG_BIT_VAR(V)  (FRAG_BIT_VAR0 << (V))

#define FRAG_BITS_TEX_ANY (FRAG_BIT_TEX0|	\
			   FRAG_BIT_TEX1|	\
			   FRAG_BIT_TEX2|	\
			   FRAG_BIT_TEX3|	\
			   FRAG_BIT_TEX4|	\
			   FRAG_BIT_TEX5|	\
			   FRAG_BIT_TEX6|	\
			   FRAG_BIT_TEX7)
/*@}*/


/**
 * Fragment program results
 */
typedef enum
{
   FRAG_RESULT_DEPTH = 0,
   FRAG_RESULT_COLOR = 1,
   FRAG_RESULT_DATA0 = 2,
   FRAG_RESULT_MAX = (FRAG_RESULT_DATA0 + MAX_DRAW_BUFFERS)
} gl_frag_result;

/**
 * Names of the various vertex/fragment program register files, etc.
 *
 * NOTE: first four tokens must fit into 2 bits (see t_vb_arbprogram.c)
 * All values should fit in a 4-bit field.
 *
 * NOTE: PROGRAM_ENV_PARAM, PROGRAM_STATE_VAR, PROGRAM_NAMED_PARAM,
 * PROGRAM_CONSTANT, and PROGRAM_UNIFORM can all be considered to
 * be "uniform" variables since they can only be set outside glBegin/End.
 * They're also all stored in the same Parameters array.
 */
typedef enum
{
   PROGRAM_TEMPORARY,   /**< machine->Temporary[] */
   PROGRAM_INPUT,       /**< machine->Inputs[] */
   PROGRAM_OUTPUT,      /**< machine->Outputs[] */
   PROGRAM_VARYING,     /**< machine->Inputs[]/Outputs[] */
   PROGRAM_LOCAL_PARAM, /**< gl_program->LocalParams[] */
   PROGRAM_ENV_PARAM,   /**< gl_program->Parameters[] */
   PROGRAM_STATE_VAR,   /**< gl_program->Parameters[] */
   PROGRAM_NAMED_PARAM, /**< gl_program->Parameters[] */
   PROGRAM_CONSTANT,    /**< gl_program->Parameters[] */
   PROGRAM_UNIFORM,     /**< gl_program->Parameters[] */
   PROGRAM_WRITE_ONLY,  /**< A dummy, write-only register */
   PROGRAM_ADDRESS,     /**< machine->AddressReg */
   PROGRAM_SAMPLER,     /**< for shader samplers, compile-time only */
   PROGRAM_UNDEFINED,   /**< Invalid/TBD value */
   PROGRAM_FILE_MAX
} gl_register_file;

/**
 * An index for each type of texture object.  These correspond to the GL
 * texture target enums, such as GL_TEXTURE_2D, GL_TEXTURE_CUBE_MAP, etc.
 * Note: the order is from highest priority to lowest priority.
 */
typedef enum
{
   TEXTURE_2D_ARRAY_INDEX,
   TEXTURE_1D_ARRAY_INDEX,
   TEXTURE_CUBE_INDEX,
   TEXTURE_3D_INDEX,
   TEXTURE_RECT_INDEX,
   TEXTURE_2D_INDEX,
   TEXTURE_1D_INDEX,
   NUM_TEXTURE_TARGETS
} gl_texture_index;

#endif /* MTYPES_H */
