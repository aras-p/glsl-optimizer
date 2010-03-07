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

#ifndef TGSI_EXEC_H
#define TGSI_EXEC_H

#include "pipe/p_compiler.h"
#include "pipe/p_state.h"

#if defined __cplusplus
extern "C" {
#endif


#define MAX_LABELS (4 * 1024)  /**< basically, max instructions */

#define NUM_CHANNELS 4  /* R,G,B,A */
#define QUAD_SIZE    4  /* 4 pixel/quad */


/**
  * Registers may be treated as float, signed int or unsigned int.
  */
union tgsi_exec_channel
{
   float    f[QUAD_SIZE];
   int      i[QUAD_SIZE];
   unsigned u[QUAD_SIZE];
};

/**
  * A vector[RGBA] of channels[4 pixels]
  */
struct tgsi_exec_vector
{
   union tgsi_exec_channel xyzw[NUM_CHANNELS];
};

/**
 * For fragment programs, information for computing fragment input
 * values from plane equation of the triangle/line.
 */
struct tgsi_interp_coef
{
   float a0[NUM_CHANNELS];	/* in an xyzw layout */
   float dadx[NUM_CHANNELS];
   float dady[NUM_CHANNELS];
};

enum tgsi_sampler_control {
   tgsi_sampler_lod_bias,
   tgsi_sampler_lod_explicit
};

/**
 * Information for sampling textures, which must be implemented
 * by code outside the TGSI executor.
 */
struct tgsi_sampler
{
   /** Get samples for four fragments in a quad */
   void (*get_samples)(struct tgsi_sampler *sampler,
                       const float s[QUAD_SIZE],
                       const float t[QUAD_SIZE],
                       const float p[QUAD_SIZE],
                       const float c0[QUAD_SIZE],
                       enum tgsi_sampler_control control,
                       float rgba[NUM_CHANNELS][QUAD_SIZE]);
};

/**
 * For branching/calling subroutines.
 */
struct tgsi_exec_labels
{
   unsigned labels[MAX_LABELS][2];
   unsigned count;
};


#define TGSI_EXEC_NUM_TEMPS       128
#define TGSI_EXEC_NUM_IMMEDIATES  256

/*
 * Locations of various utility registers (_I = Index, _C = Channel)
 */
#define TGSI_EXEC_TEMP_00000000_I   (TGSI_EXEC_NUM_TEMPS + 0)
#define TGSI_EXEC_TEMP_00000000_C   0

#define TGSI_EXEC_TEMP_7FFFFFFF_I   (TGSI_EXEC_NUM_TEMPS + 0)
#define TGSI_EXEC_TEMP_7FFFFFFF_C   1

#define TGSI_EXEC_TEMP_80000000_I   (TGSI_EXEC_NUM_TEMPS + 0)
#define TGSI_EXEC_TEMP_80000000_C   2

#define TGSI_EXEC_TEMP_FFFFFFFF_I   (TGSI_EXEC_NUM_TEMPS + 0)
#define TGSI_EXEC_TEMP_FFFFFFFF_C   3

#define TGSI_EXEC_TEMP_ONE_I        (TGSI_EXEC_NUM_TEMPS + 1)
#define TGSI_EXEC_TEMP_ONE_C        0

#define TGSI_EXEC_TEMP_TWO_I        (TGSI_EXEC_NUM_TEMPS + 1)
#define TGSI_EXEC_TEMP_TWO_C        1

#define TGSI_EXEC_TEMP_128_I        (TGSI_EXEC_NUM_TEMPS + 1)
#define TGSI_EXEC_TEMP_128_C        2

#define TGSI_EXEC_TEMP_MINUS_128_I  (TGSI_EXEC_NUM_TEMPS + 1)
#define TGSI_EXEC_TEMP_MINUS_128_C  3

#define TGSI_EXEC_TEMP_KILMASK_I    (TGSI_EXEC_NUM_TEMPS + 2)
#define TGSI_EXEC_TEMP_KILMASK_C    0

#define TGSI_EXEC_TEMP_OUTPUT_I     (TGSI_EXEC_NUM_TEMPS + 2)
#define TGSI_EXEC_TEMP_OUTPUT_C     1

#define TGSI_EXEC_TEMP_PRIMITIVE_I  (TGSI_EXEC_NUM_TEMPS + 2)
#define TGSI_EXEC_TEMP_PRIMITIVE_C  2

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

#define TGSI_EXEC_TEMP_CC_I         (TGSI_EXEC_NUM_TEMPS + 2)
#define TGSI_EXEC_TEMP_CC_C         3

#define TGSI_EXEC_TEMP_THREE_I      (TGSI_EXEC_NUM_TEMPS + 3)
#define TGSI_EXEC_TEMP_THREE_C      0

#define TGSI_EXEC_TEMP_HALF_I       (TGSI_EXEC_NUM_TEMPS + 3)
#define TGSI_EXEC_TEMP_HALF_C       1

/* execution mask, each value is either 0 or ~0 */
#define TGSI_EXEC_MASK_I            (TGSI_EXEC_NUM_TEMPS + 3)
#define TGSI_EXEC_MASK_C            2

/* 4 register buffer for various purposes */
#define TGSI_EXEC_TEMP_R0           (TGSI_EXEC_NUM_TEMPS + 4)
#define TGSI_EXEC_NUM_TEMP_R        4

#define TGSI_EXEC_TEMP_ADDR         (TGSI_EXEC_NUM_TEMPS + 8)
#define TGSI_EXEC_NUM_ADDRS         1

/* predicate register */
#define TGSI_EXEC_TEMP_P0           (TGSI_EXEC_NUM_TEMPS + 9)
#define TGSI_EXEC_NUM_PREDS         1

#define TGSI_EXEC_NUM_TEMP_EXTRAS   10



#define TGSI_EXEC_MAX_COND_NESTING  32
#define TGSI_EXEC_MAX_LOOP_NESTING  32
#define TGSI_EXEC_MAX_SWITCH_NESTING 32
#define TGSI_EXEC_MAX_CALL_NESTING  32

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

/** function call/activation record */
struct tgsi_call_record
{
   uint CondStackTop;
   uint LoopStackTop;
   uint ContStackTop;
   int SwitchStackTop;
   int BreakStackTop;
   uint ReturnAddr;
};


/* Switch-case block state. */
struct tgsi_switch_record {
   uint mask;                          /**< execution mask */
   union tgsi_exec_channel selector;   /**< a value case statements are compared to */
   uint defaultMask;                   /**< non-execute mask for default case */
};


enum tgsi_break_type {
   TGSI_EXEC_BREAK_INSIDE_LOOP,
   TGSI_EXEC_BREAK_INSIDE_SWITCH
};


#define TGSI_EXEC_MAX_BREAK_STACK (TGSI_EXEC_MAX_LOOP_NESTING + TGSI_EXEC_MAX_SWITCH_NESTING)


/**
 * Run-time virtual machine state for executing TGSI shader.
 */
struct tgsi_exec_machine
{
   /* Total = program temporaries + internal temporaries
    */
   struct tgsi_exec_vector       Temps[TGSI_EXEC_NUM_TEMPS +
                                       TGSI_EXEC_NUM_TEMP_EXTRAS];

   float                         Imms[TGSI_EXEC_NUM_IMMEDIATES][4];

   struct tgsi_exec_vector       Inputs[TGSI_MAX_PRIM_VERTICES * PIPE_MAX_ATTRIBS];
   struct tgsi_exec_vector       Outputs[TGSI_MAX_TOTAL_VERTICES];

   struct tgsi_exec_vector       *Addrs;
   struct tgsi_exec_vector       *Predicates;

   struct tgsi_sampler           **Samplers;

   unsigned                      ImmLimit;
   const void *Consts[PIPE_MAX_CONSTANT_BUFFERS];
   const struct tgsi_token       *Tokens;   /**< Declarations, instructions */
   unsigned                      Processor; /**< TGSI_PROCESSOR_x */

   /* GEOMETRY processor only. */
   unsigned                      *Primitives;
   unsigned                       NumOutputs;
   unsigned                       MaxGeometryShaderOutputs;

   /* FRAGMENT processor only. */
   const struct tgsi_interp_coef *InterpCoefs;
   struct tgsi_exec_vector       QuadPos;
   float                         Face;    /**< +1 if front facing, -1 if back facing */

   /* Conditional execution masks */
   uint CondMask;  /**< For IF/ELSE/ENDIF */
   uint LoopMask;  /**< For BGNLOOP/ENDLOOP */
   uint ContMask;  /**< For loop CONT statements */
   uint FuncMask;  /**< For function calls */
   uint ExecMask;  /**< = CondMask & LoopMask */

   /* Current switch-case state. */
   struct tgsi_switch_record Switch;

   /* Current break type. */
   enum tgsi_break_type BreakType;

   /** Condition mask stack (for nested conditionals) */
   uint CondStack[TGSI_EXEC_MAX_COND_NESTING];
   int CondStackTop;

   /** Loop mask stack (for nested loops) */
   uint LoopStack[TGSI_EXEC_MAX_LOOP_NESTING];
   int LoopStackTop;

   /** Loop label stack */
   uint LoopLabelStack[TGSI_EXEC_MAX_LOOP_NESTING];
   int LoopLabelStackTop;

   /** Loop counter stack (x = index, y = counter, z = step) */
   struct tgsi_exec_vector LoopCounterStack[TGSI_EXEC_MAX_LOOP_NESTING];
   int LoopCounterStackTop;
   
   /** Loop continue mask stack (see comments in tgsi_exec.c) */
   uint ContStack[TGSI_EXEC_MAX_LOOP_NESTING];
   int ContStackTop;

   /** Switch case stack */
   struct tgsi_switch_record SwitchStack[TGSI_EXEC_MAX_SWITCH_NESTING];
   int SwitchStackTop;

   enum tgsi_break_type BreakStack[TGSI_EXEC_MAX_BREAK_STACK];
   int BreakStackTop;

   /** Function execution mask stack (for executing subroutine code) */
   uint FuncStack[TGSI_EXEC_MAX_CALL_NESTING];
   int FuncStackTop;

   /** Function call stack for saving/restoring the program counter */
   struct tgsi_call_record CallStack[TGSI_EXEC_MAX_CALL_NESTING];
   int CallStackTop;

   struct tgsi_full_instruction *Instructions;
   uint NumInstructions;

   struct tgsi_full_declaration *Declarations;
   uint NumDeclarations;

   struct tgsi_exec_labels Labels;
};

struct tgsi_exec_machine *
tgsi_exec_machine_create( void );

void
tgsi_exec_machine_destroy(struct tgsi_exec_machine *mach);


void 
tgsi_exec_machine_bind_shader(
   struct tgsi_exec_machine *mach,
   const struct tgsi_token *tokens,
   uint numSamplers,
   struct tgsi_sampler **samplers);

uint
tgsi_exec_machine_run(
   struct tgsi_exec_machine *mach );


void
tgsi_exec_machine_free_data(struct tgsi_exec_machine *mach);


boolean
tgsi_check_soa_dependencies(const struct tgsi_full_instruction *inst);


static INLINE void
tgsi_set_kill_mask(struct tgsi_exec_machine *mach, unsigned mask)
{
   mach->Temps[TGSI_EXEC_TEMP_KILMASK_I].xyzw[TGSI_EXEC_TEMP_KILMASK_C].u[0] =
      mask;
}


/** Set execution mask values prior to executing the shader */
static INLINE void
tgsi_set_exec_mask(struct tgsi_exec_machine *mach,
                   boolean ch0, boolean ch1, boolean ch2, boolean ch3)
{
   int *mask = mach->Temps[TGSI_EXEC_MASK_I].xyzw[TGSI_EXEC_MASK_C].i;
   mask[0] = ch0 ? ~0 : 0;
   mask[1] = ch1 ? ~0 : 0;
   mask[2] = ch2 ? ~0 : 0;
   mask[3] = ch3 ? ~0 : 0;
}


#if defined __cplusplus
} /* extern "C" */
#endif

#endif /* TGSI_EXEC_H */
