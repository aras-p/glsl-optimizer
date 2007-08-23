/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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


#ifndef I915_FPC_H
#define I915_FPC_H

#include "pipe/p_util.h"

#include "i915_context.h"
#include "i915_reg.h"



#define I915_PROGRAM_SIZE 192

#define MAX_VARYING 8

enum
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
   FRAG_ATTRIB_VAR0 = 12,  /**< shader varying */
   FRAG_ATTRIB_MAX = (FRAG_ATTRIB_VAR0 + MAX_VARYING)
};

/**
 * Bitflags for fragment program input attributes.
 */
/*@{*/
#define FRAG_BIT_WPOS  (1 << FRAG_ATTRIB_WPOS)
#define FRAG_BIT_COL0  (1 << FRAG_ATTRIB_COL0)
#define FRAG_BIT_COL1  (1 << FRAG_ATTRIB_COL1)
#define FRAG_BIT_FOGC  (1 << FRAG_ATTRIB_FOGC)
#define FRAG_BIT_TEX0  (1 << FRAG_ATTRIB_TEX0)
#define FRAG_BIT_TEX1  (1 << FRAG_ATTRIB_TEX1)
#define FRAG_BIT_TEX2  (1 << FRAG_ATTRIB_TEX2)
#define FRAG_BIT_TEX3  (1 << FRAG_ATTRIB_TEX3)
#define FRAG_BIT_TEX4  (1 << FRAG_ATTRIB_TEX4)
#define FRAG_BIT_TEX5  (1 << FRAG_ATTRIB_TEX5)
#define FRAG_BIT_TEX6  (1 << FRAG_ATTRIB_TEX6)
#define FRAG_BIT_TEX7  (1 << FRAG_ATTRIB_TEX7)
#define FRAG_BIT_VAR0  (1 << FRAG_ATTRIB_VAR0)

#define MAX_DRAW_BUFFERS 4

enum
{
   FRAG_RESULT_COLR = 0,
   FRAG_RESULT_COLH = 1,
   FRAG_RESULT_DEPR = 2,
   FRAG_RESULT_DATA0 = 3,
   FRAG_RESULT_MAX = (FRAG_RESULT_DATA0 + MAX_DRAW_BUFFERS)
};



/**
 * Program translation state
 */
struct i915_fp_compile {
   struct pipe_shader_state *shader;

   uint declarations[I915_PROGRAM_SIZE];
   uint program[I915_PROGRAM_SIZE];

   /** points into the i915->current.constants array: */
   float (*constants)[4];
   uint num_constants;
   uint constant_flags[I915_MAX_CONSTANT]; /**< status of each constant */

   uint *csr;                 /* Cursor, points into program.
                                 */

   uint *decl;                /* Cursor, points into declarations.
                                 */

   uint decl_s;               /* flags for which s regs need to be decl'd */
   uint decl_t;               /* flags for which t regs need to be decl'd */

   uint temp_flag;            /* Tracks temporary regs which are in
                                 * use.
                                 */

   uint utemp_flag;           /* Tracks TYPE_U temporary regs which are in
                                 * use.
                                 */

   uint nr_tex_indirect;
   uint nr_tex_insn;
   uint nr_alu_insn;
   uint nr_decl_insn;

   boolean error;      /**< Set if i915_program_error() is called */
   uint wpos_tex;
   uint NumNativeInstructions;
   uint NumNativeAluInstructions;
   uint NumNativeTexInstructions;
   uint NumNativeTexIndirections;
};


/* Having zero and one in here makes the definition of swizzle a lot
 * easier.
 */
#define UREG_TYPE_SHIFT               29
#define UREG_NR_SHIFT                 24
#define UREG_CHANNEL_X_NEGATE_SHIFT   23
#define UREG_CHANNEL_X_SHIFT          20
#define UREG_CHANNEL_Y_NEGATE_SHIFT   19
#define UREG_CHANNEL_Y_SHIFT          16
#define UREG_CHANNEL_Z_NEGATE_SHIFT   15
#define UREG_CHANNEL_Z_SHIFT          12
#define UREG_CHANNEL_W_NEGATE_SHIFT   11
#define UREG_CHANNEL_W_SHIFT          8
#define UREG_CHANNEL_ZERO_NEGATE_MBZ  5
#define UREG_CHANNEL_ZERO_SHIFT       4
#define UREG_CHANNEL_ONE_NEGATE_MBZ   1
#define UREG_CHANNEL_ONE_SHIFT        0

#define UREG_BAD          0xffffffff    /* not a valid ureg */

#define X    SRC_X
#define Y    SRC_Y
#define Z    SRC_Z
#define W    SRC_W
#define ZERO SRC_ZERO
#define ONE  SRC_ONE

/* Construct a ureg:
 */
#define UREG( type, nr ) (((type)<< UREG_TYPE_SHIFT) |		\
			  ((nr)  << UREG_NR_SHIFT) |		\
			  (X     << UREG_CHANNEL_X_SHIFT) |	\
			  (Y     << UREG_CHANNEL_Y_SHIFT) |	\
			  (Z     << UREG_CHANNEL_Z_SHIFT) |	\
			  (W     << UREG_CHANNEL_W_SHIFT) |	\
			  (ZERO  << UREG_CHANNEL_ZERO_SHIFT) |	\
			  (ONE   << UREG_CHANNEL_ONE_SHIFT))

#define GET_CHANNEL_SRC( reg, channel ) ((reg<<(channel*4)) & (0xf<<20))
#define CHANNEL_SRC( src, channel ) (src>>(channel*4))

#define GET_UREG_TYPE(reg) (((reg)>>UREG_TYPE_SHIFT)&REG_TYPE_MASK)
#define GET_UREG_NR(reg)   (((reg)>>UREG_NR_SHIFT)&REG_NR_MASK)



#define UREG_XYZW_CHANNEL_MASK 0x00ffff00

/* One neat thing about the UREG representation:  
 */
static INLINE int
swizzle(int reg, uint x, uint y, uint z, uint w)
{
   assert(x <= SRC_ONE);
   assert(y <= SRC_ONE);
   assert(z <= SRC_ONE);
   assert(w <= SRC_ONE);
   return ((reg & ~UREG_XYZW_CHANNEL_MASK) |
           CHANNEL_SRC(GET_CHANNEL_SRC(reg, x), 0) |
           CHANNEL_SRC(GET_CHANNEL_SRC(reg, y), 1) |
           CHANNEL_SRC(GET_CHANNEL_SRC(reg, z), 2) |
           CHANNEL_SRC(GET_CHANNEL_SRC(reg, w), 3));
}



/***********************************************************************
 * Public interface for the compiler
 */
extern void i915_translate_fragment_program( struct i915_context *i915 );



extern uint i915_get_temp(struct i915_fp_compile *p);
extern uint i915_get_utemp(struct i915_fp_compile *p);
extern void i915_release_utemps(struct i915_fp_compile *p);


extern uint i915_emit_texld(struct i915_fp_compile *p,
                              uint dest,
                              uint destmask,
                              uint sampler, uint coord, uint op);

extern uint i915_emit_arith(struct i915_fp_compile *p,
                              uint op,
                              uint dest,
                              uint mask,
                              uint saturate,
                              uint src0, uint src1, uint src2);

extern uint i915_emit_decl(struct i915_fp_compile *p,
                             uint type, uint nr, uint d0_flags);


extern uint i915_emit_const1f(struct i915_fp_compile *p, float c0);

extern uint i915_emit_const2f(struct i915_fp_compile *p,
                                float c0, float c1);

extern uint i915_emit_const4fv(struct i915_fp_compile *p,
                                 const float * c);

extern uint i915_emit_const4f(struct i915_fp_compile *p,
                                float c0, float c1,
                                float c2, float c3);


/*======================================================================
 * i915_fpc_debug.c
 */
extern void i915_disassemble_program(const uint * program, uint sz);


/*======================================================================
 * i915_fpc_translate.c
 */

extern void
i915_program_error(struct i915_fp_compile *p, const char *msg);

extern void
i915_translate_fragment_program(struct i915_context *i915);


#endif
