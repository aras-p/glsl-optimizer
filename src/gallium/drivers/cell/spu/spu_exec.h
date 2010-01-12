/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
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

#if !defined SPU_EXEC_H
#define SPU_EXEC_H

#include "pipe/p_compiler.h"
#include "tgsi/tgsi_exec.h"

#if defined __cplusplus
extern "C" {
#endif

/**
  * Registers may be treated as float, signed int or unsigned int.
  */
union spu_exec_channel
{
   float    f[QUAD_SIZE];
   int      i[QUAD_SIZE];
   unsigned u[QUAD_SIZE];
   qword    q;
};

/**
  * A vector[RGBA] of channels[4 pixels]
  */
struct spu_exec_vector
{
   union spu_exec_channel xyzw[NUM_CHANNELS];
};

/**
 * For fragment programs, information for computing fragment input
 * values from plane equation of the triangle/line.
 */
struct spu_interp_coef
{
   float a0[NUM_CHANNELS];	/* in an xyzw layout */
   float dadx[NUM_CHANNELS];
   float dady[NUM_CHANNELS];
};


struct softpipe_tile_cache;  /**< Opaque to TGSI */

/**
 * Information for sampling textures, which must be implemented
 * by code outside the TGSI executor.
 */
struct spu_sampler
{
   const struct pipe_sampler_state *state;
   struct pipe_texture *texture;
   /** Get samples for four fragments in a quad */
   void (*get_samples)(struct spu_sampler *sampler,
                       const float s[QUAD_SIZE],
                       const float t[QUAD_SIZE],
                       const float p[QUAD_SIZE],
                       float lodbias,
                       float rgba[NUM_CHANNELS][QUAD_SIZE]);
   void *pipe; /*XXX temporary*/
   struct softpipe_tile_cache *cache;
};


/**
 * Run-time virtual machine state for executing TGSI shader.
 */
struct spu_exec_machine
{
   /*
    * 32 program temporaries
    * 4  internal temporaries
    * 1  address
    */
   PIPE_ALIGN_VAR(16)
   struct spu_exec_vector       Temps[TGSI_EXEC_NUM_TEMPS 
                                      + TGSI_EXEC_NUM_TEMP_EXTRAS + 1];

   struct spu_exec_vector       *Addrs;

   struct spu_sampler           *Samplers;

   float                         Imms[TGSI_EXEC_NUM_IMMEDIATES][4];
   unsigned                      ImmLimit;
   float                         (*Consts)[4];
   struct spu_exec_vector       *Inputs;
   struct spu_exec_vector       *Outputs;
   unsigned                      Processor;

   /* GEOMETRY processor only. */
   unsigned                      *Primitives;

   /* FRAGMENT processor only. */
   const struct spu_interp_coef *InterpCoefs;
   struct spu_exec_vector       QuadPos;

   /* Conditional execution masks */
   uint CondMask;  /**< For IF/ELSE/ENDIF */
   uint LoopMask;  /**< For BGNLOOP/ENDLOOP */
   uint ContMask;  /**< For loop CONT statements */
   uint FuncMask;  /**< For function calls */
   uint ExecMask;  /**< = CondMask & LoopMask */

   /** Condition mask stack (for nested conditionals) */
   uint CondStack[TGSI_EXEC_MAX_COND_NESTING];
   int CondStackTop;

   /** Loop mask stack (for nested loops) */
   uint LoopStack[TGSI_EXEC_MAX_LOOP_NESTING];
   int LoopStackTop;

   /** Loop continue mask stack (see comments in tgsi_exec.c) */
   uint ContStack[TGSI_EXEC_MAX_LOOP_NESTING];
   int ContStackTop;

   /** Function execution mask stack (for executing subroutine code) */
   uint FuncStack[TGSI_EXEC_MAX_CALL_NESTING];
   int FuncStackTop;

   /** Function call stack for saving/restoring the program counter */
   uint CallStack[TGSI_EXEC_MAX_CALL_NESTING];
   int CallStackTop;

   struct tgsi_full_instruction *Instructions;
   uint NumInstructions;

   struct tgsi_full_declaration *Declarations;
   uint NumDeclarations;
};


extern void
spu_exec_machine_init(struct spu_exec_machine *mach,
                      uint numSamplers,
                      struct spu_sampler *samplers,
                      unsigned processor);

extern uint
spu_exec_machine_run( struct spu_exec_machine *mach );


#if defined __cplusplus
} /* extern "C" */
#endif

#endif /* SPU_EXEC_H */
