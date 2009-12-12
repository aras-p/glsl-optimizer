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


#include "i915_context.h"
#include "i915_reg.h"



#define I915_PROGRAM_SIZE 192



/**
 * Program translation state
 */
struct i915_fp_compile {
   struct i915_fragment_shader *shader;  /* the shader we're compiling */

   boolean used_constants[I915_MAX_CONSTANT];

   /** maps TGSI immediate index to constant slot */
   uint num_immediates;
   uint immediates_map[I915_MAX_CONSTANT];
   float immediates[I915_MAX_CONSTANT][4];

   boolean first_instruction;

   uint declarations[I915_PROGRAM_SIZE];
   uint program[I915_PROGRAM_SIZE];

   uint *csr;            /**< Cursor, points into program. */

   uint *decl;           /**< Cursor, points into declarations. */

   uint decl_s;          /**< flags for which s regs need to be decl'd */
   uint decl_t;          /**< flags for which t regs need to be decl'd */

   uint temp_flag;       /**< Tracks temporary regs which are in use */
   uint utemp_flag;      /**< Tracks TYPE_U temporary regs which are in use */

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
extern void
i915_translate_fragment_program( struct i915_context *i915,
                                 struct i915_fragment_shader *fs);



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
i915_program_error(struct i915_fp_compile *p, const char *msg, ...);


#endif
