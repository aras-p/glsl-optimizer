/**************************************************************************
 * 
 * Copyright 2007-2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * Copyright 2009-2010 VMware, Inc.  All rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#ifndef SPU_TGSI_EXEC_H
#define SPU_TGSI_EXEC_H

#include "pipe/p_compiler.h"
#include "pipe/p_state.h"

#if defined __cplusplus
extern "C" {
#endif


#define NUM_CHANNELS 4  /* R,G,B,A */
#define QUAD_SIZE    4  /* 4 pixel/quad */



#define TGSI_EXEC_NUM_TEMPS       128
#define TGSI_EXEC_NUM_IMMEDIATES  256

/*
 * Locations of various utility registers (_I = Index, _C = Channel)
 */
#define TGSI_EXEC_TEMP_00000000_IDX    (TGSI_EXEC_NUM_TEMPS + 0)
#define TGSI_EXEC_TEMP_00000000_CHAN   0

#define TGSI_EXEC_TEMP_7FFFFFFF_IDX    (TGSI_EXEC_NUM_TEMPS + 0)
#define TGSI_EXEC_TEMP_7FFFFFFF_CHAN   1

#define TGSI_EXEC_TEMP_80000000_IDX    (TGSI_EXEC_NUM_TEMPS + 0)
#define TGSI_EXEC_TEMP_80000000_CHAN   2

#define TGSI_EXEC_TEMP_FFFFFFFF_IDX    (TGSI_EXEC_NUM_TEMPS + 0)
#define TGSI_EXEC_TEMP_FFFFFFFF_CHAN   3

#define TGSI_EXEC_TEMP_ONE_IDX         (TGSI_EXEC_NUM_TEMPS + 1)
#define TGSI_EXEC_TEMP_ONE_CHAN        0

#define TGSI_EXEC_TEMP_TWO_IDX         (TGSI_EXEC_NUM_TEMPS + 1)
#define TGSI_EXEC_TEMP_TWO_CHAN        1

#define TGSI_EXEC_TEMP_128_IDX         (TGSI_EXEC_NUM_TEMPS + 1)
#define TGSI_EXEC_TEMP_128_CHAN        2

#define TGSI_EXEC_TEMP_MINUS_128_IDX   (TGSI_EXEC_NUM_TEMPS + 1)
#define TGSI_EXEC_TEMP_MINUS_128_CHAN  3

#define TGSI_EXEC_TEMP_KILMASK_IDX     (TGSI_EXEC_NUM_TEMPS + 2)
#define TGSI_EXEC_TEMP_KILMASK_CHAN    0

#define TGSI_EXEC_TEMP_OUTPUT_IDX      (TGSI_EXEC_NUM_TEMPS + 2)
#define TGSI_EXEC_TEMP_OUTPUT_CHAN     1

#define TGSI_EXEC_TEMP_PRIMITIVE_IDX   (TGSI_EXEC_NUM_TEMPS + 2)
#define TGSI_EXEC_TEMP_PRIMITIVE_CHAN  2

/* NVIDIA condition code (CC) vector
 */
#define TGSI_EXEC_CC_GT       0x01
#define TGSI_EXEC_CC_EQ       0x02
#define TGSI_EXEC_CC_LT       0x04
#define TGSI_EXEC_CC_UN       0x08

#define TGSI_EXEC_CC_X_MASK   0x000000ff
#define TGSI_EXEC_CC_X_SHIFT  0
#define TGSI_EXEC_CC_Y_MASK   0x0000ff00
#define TGSI_EXEC_CC_Y_SHIFT  8
#define TGSI_EXEC_CC_Z_MASK   0x00ff0000
#define TGSI_EXEC_CC_Z_SHIFT  16
#define TGSI_EXEC_CC_W_MASK   0xff000000
#define TGSI_EXEC_CC_W_SHIFT  24

#define TGSI_EXEC_TEMP_CC_IDX         (TGSI_EXEC_NUM_TEMPS + 2)
#define TGSI_EXEC_TEMP_CC_CHAN         3

#define TGSI_EXEC_TEMP_THREE_IDX      (TGSI_EXEC_NUM_TEMPS + 3)
#define TGSI_EXEC_TEMP_THREE_CHAN      0

#define TGSI_EXEC_TEMP_HALF_IDX       (TGSI_EXEC_NUM_TEMPS + 3)
#define TGSI_EXEC_TEMP_HALF_CHAN       1

/* execution mask, each value is either 0 or ~0 */
#define TGSI_EXEC_MASK_IDX            (TGSI_EXEC_NUM_TEMPS + 3)
#define TGSI_EXEC_MASK_CHAN            2

/* 4 register buffer for various purposes */
#define TGSI_EXEC_TEMP_R0           (TGSI_EXEC_NUM_TEMPS + 4)
#define TGSI_EXEC_NUM_TEMP_R        4

#define TGSI_EXEC_TEMP_ADDR         (TGSI_EXEC_NUM_TEMPS + 8)
#define TGSI_EXEC_NUM_ADDRS         1

/* predicate register */
#define TGSI_EXEC_TEMP_P0           (TGSI_EXEC_NUM_TEMPS + 9)
#define TGSI_EXEC_NUM_PREDS         1

#define TGSI_EXEC_NUM_TEMP_EXTRAS   10



#define TGSI_EXEC_MAX_NESTING  32
#define TGSI_EXEC_MAX_COND_NESTING  TGSI_EXEC_MAX_NESTING
#define TGSI_EXEC_MAX_LOOP_NESTING  TGSI_EXEC_MAX_NESTING
#define TGSI_EXEC_MAX_SWITCH_NESTING TGSI_EXEC_MAX_NESTING
#define TGSI_EXEC_MAX_CALL_NESTING  TGSI_EXEC_MAX_NESTING

/* The maximum number of input attributes per vertex. For 2D
 * input register files, this is the stride between two 1D
 * arrays.
 */
#define TGSI_EXEC_MAX_INPUT_ATTRIBS 17

/* The maximum number of constant vectors per constant buffer.
 */
#define TGSI_EXEC_MAX_CONST_BUFFER  4096

/* The maximum number of vertices per primitive */
#define TGSI_MAX_PRIM_VERTICES 6

/* The maximum number of primitives to be generated */
#define TGSI_MAX_PRIMITIVES 64

/* The maximum total number of vertices */
#define TGSI_MAX_TOTAL_VERTICES (TGSI_MAX_PRIM_VERTICES * TGSI_MAX_PRIMITIVES * PIPE_MAX_ATTRIBS)


#if defined __cplusplus
} /* extern "C" */
#endif

#endif /* TGSI_EXEC_H */
