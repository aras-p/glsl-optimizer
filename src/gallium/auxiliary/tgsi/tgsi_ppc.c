/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/**
 * TGSI to PowerPC code generation.
 */

#include "pipe/p_config.h"

#if defined(PIPE_ARCH_PPC)

#include "pipe/p_debug.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_sse.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "tgsi_exec.h"
#include "tgsi_ppc.h"
#include "rtasm/rtasm_ppc.h"


/**
 * Since it's pretty much impossible to form PPC vector immediates, load
 * them from memory here:
 */
const float ppc_builtin_constants[] ALIGN16_ATTRIB = {
   1.0f, -128.0f, 128.0, 0.0
};


#define FOR_EACH_CHANNEL( CHAN )\
   for (CHAN = 0; CHAN < NUM_CHANNELS; CHAN++)

#define IS_DST0_CHANNEL_ENABLED( INST, CHAN )\
   ((INST).FullDstRegisters[0].DstRegister.WriteMask & (1 << (CHAN)))

#define IF_IS_DST0_CHANNEL_ENABLED( INST, CHAN )\
   if (IS_DST0_CHANNEL_ENABLED( INST, CHAN ))

#define FOR_EACH_DST0_ENABLED_CHANNEL( INST, CHAN )\
   FOR_EACH_CHANNEL( CHAN )\
      IF_IS_DST0_CHANNEL_ENABLED( INST, CHAN )

#define CHAN_X 0
#define CHAN_Y 1
#define CHAN_Z 2
#define CHAN_W 3

#define TEMP_ONE_I   TGSI_EXEC_TEMP_ONE_I
#define TEMP_ONE_C   TGSI_EXEC_TEMP_ONE_C

#define TEMP_R0   TGSI_EXEC_TEMP_R0
#define TEMP_ADDR TGSI_EXEC_TEMP_ADDR


/**
 * Context/state used during code gen.
 */
struct gen_context
{
   struct ppc_function *f;
   int inputs_reg;    /**< GP register pointing to input params */
   int outputs_reg;   /**< GP register pointing to output params */
   int temps_reg;     /**< GP register pointing to temporary "registers" */
   int immed_reg;     /**< GP register pointing to immediates buffer */
   int const_reg;     /**< GP register pointing to constants buffer */
   int builtins_reg;  /**< GP register pointint to built-in constants */

   int one_vec;       /**< vector register with {1.0, 1.0, 1.0, 1.0} */
   int bit31_vec;     /**< vector register with {1<<31, 1<<31, 1<<31, 1<<31} */
};


/**
 * Load the given vector register with {value, value, value, value}.
 * The value must be in the ppu_builtin_constants[] array.
 * We wouldn't need this if there was a simple way to load PPC vector
 * registers with immediate values!
 */
static void
load_constant_vec(struct gen_context *gen, int dst_vec, float value)
{
   uint pos;
   for (pos = 0; pos < Elements(ppc_builtin_constants); pos++) {
      if (ppc_builtin_constants[pos] == value) {
         int offset_reg = ppc_allocate_register(gen->f);
         int offset = pos * 4;

         ppc_li(gen->f, offset_reg, offset);
         /* Load 4-byte word into vector register.
          * The vector slot depends on the effective address we load from.
          * We know that our builtins start at a 16-byte boundary so we
          * know that 'swizzle' tells us which vector slot will have the
          * loaded word.  The other vector slots will be undefined.
          */
         ppc_lvewx(gen->f, dst_vec, gen->builtins_reg, offset_reg);
         /* splat word[pos % 4] across the vector reg */
         ppc_vspltw(gen->f, dst_vec, dst_vec, pos % 4);
         ppc_release_register(gen->f, offset_reg);
         return;
      }
   }
   assert(0 && "Need to add new constant to ppc_builtin_constants array");
}


/**
 * Return index of vector register containing {1.0, 1.0, 1.0, 1.0}.
 */
static int
gen_one_vec(struct gen_context *gen)
{
   if (gen->one_vec < 0) {
      gen->one_vec = ppc_allocate_vec_register(gen->f);
      load_constant_vec(gen, gen->one_vec, 1.0f);
   }
   return gen->one_vec;
}

/**
 * Return index of vector register containing {1<<31, 1<<31, 1<<31, 1<<31}.
 */
static int
gen_get_bit31_vec(struct gen_context *gen)
{
   if (gen->bit31_vec < 0) {
      gen->bit31_vec = ppc_allocate_vec_register(gen->f);
      ppc_vspltisw(gen->f, gen->bit31_vec, -1);
      ppc_vslw(gen->f, gen->bit31_vec, gen->bit31_vec, gen->bit31_vec);
   }
   return gen->bit31_vec;
}


/**
 * Register fetch, put result in 'dst_vec'.
 */
static void
emit_fetch(struct gen_context *gen,
           unsigned dst_vec,
           const struct tgsi_full_src_register *reg,
           const unsigned chan_index)
{
   uint swizzle = tgsi_util_get_full_src_register_extswizzle(reg, chan_index);

   switch (swizzle) {
   case TGSI_EXTSWIZZLE_X:
   case TGSI_EXTSWIZZLE_Y:
   case TGSI_EXTSWIZZLE_Z:
   case TGSI_EXTSWIZZLE_W:
      switch (reg->SrcRegister.File) {
      case TGSI_FILE_INPUT:
         {
            int offset_reg = ppc_allocate_register(gen->f);
            int offset = (reg->SrcRegister.Index * 4 + swizzle) * 16;
            ppc_li(gen->f, offset_reg, offset);
            ppc_lvx(gen->f, dst_vec, gen->inputs_reg, offset_reg);
            ppc_release_register(gen->f, offset_reg);
         }
         break;
      case TGSI_FILE_TEMPORARY:
         {
            int offset_reg = ppc_allocate_register(gen->f);
            int offset = (reg->SrcRegister.Index * 4 + swizzle) * 16;
            ppc_li(gen->f, offset_reg, offset);
            ppc_lvx(gen->f, dst_vec, gen->temps_reg, offset_reg);
            ppc_release_register(gen->f, offset_reg);
         }
         break;
      case TGSI_FILE_IMMEDIATE:
         {
            int offset_reg = ppc_allocate_register(gen->f);
            int offset = (reg->SrcRegister.Index * 4 + swizzle) * 16;
            ppc_li(gen->f, offset_reg, offset);
            ppc_lvx(gen->f, dst_vec, gen->immed_reg, offset_reg);
            ppc_release_register(gen->f, offset_reg);
         }
         break;
      case TGSI_FILE_CONSTANT:
         {
            int offset_reg = ppc_allocate_register(gen->f);
            int offset = (reg->SrcRegister.Index * 4 + swizzle) * 4;
            ppc_li(gen->f, offset_reg, offset);
            /* Load 4-byte word into vector register.
             * The vector slot depends on the effective address we load from.
             * We know that our constants start at a 16-byte boundary so we
             * know that 'swizzle' tells us which vector slot will have the
             * loaded word.  The other vector slots will be undefined.
             */
            ppc_lvewx(gen->f, dst_vec, gen->const_reg, offset_reg);
            /* splat word[swizzle] across the vector reg */
            ppc_vspltw(gen->f, dst_vec, dst_vec, swizzle);
            ppc_release_register(gen->f, offset_reg);
         }
         break;
      default:
         assert( 0 );
      }
      break;
   case TGSI_EXTSWIZZLE_ZERO:
      ppc_vzero(gen->f, dst_vec);
      break;
   case TGSI_EXTSWIZZLE_ONE:
      {
         int one_vec = gen_one_vec(gen);
         ppc_vmove(gen->f, dst_vec, one_vec);
      }
      break;
   default:
      assert( 0 );
   }

   {
      uint sign_op = tgsi_util_get_full_src_register_sign_mode(reg, chan_index);
      if (sign_op != TGSI_UTIL_SIGN_KEEP) {
         int bit31_vec = gen_get_bit31_vec(gen);

         switch (sign_op) {
         case TGSI_UTIL_SIGN_CLEAR:
            /* vec = vec & ~bit31 */
            ppc_vandc(gen->f, dst_vec, dst_vec, bit31_vec);
            break;
         case TGSI_UTIL_SIGN_SET:
            /* vec = vec | bit31 */
            ppc_vor(gen->f, dst_vec, dst_vec, bit31_vec);
            break;
         case TGSI_UTIL_SIGN_TOGGLE:
            /* vec = vec ^ bit31 */
            ppc_vxor(gen->f, dst_vec, dst_vec, bit31_vec);
            break;
         default:
            assert(0);
         }
      }
   }
}

#define FETCH( GEN, INST, DST_VEC, SRC_REG, CHAN ) \
   emit_fetch( GEN, DST_VEC, &(INST).FullSrcRegisters[SRC_REG], CHAN )



/**
 * Register store.  Store 'src_vec' at location indicated by 'reg'.
 */
static void
emit_store(struct gen_context *gen,
           unsigned src_vec,
           const struct tgsi_full_dst_register *reg,
           const struct tgsi_full_instruction *inst,
           unsigned chan_index)
{
   switch (reg->DstRegister.File) {
   case TGSI_FILE_OUTPUT:
      {
         int offset_reg = ppc_allocate_register(gen->f);
         int offset = (reg->DstRegister.Index * 4 + chan_index) * 16;
         ppc_li(gen->f, offset_reg, offset);
         ppc_stvx(gen->f, src_vec, gen->outputs_reg, offset_reg);
         ppc_release_register(gen->f, offset_reg);
      }
      break;
   case TGSI_FILE_TEMPORARY:
      {
         int offset_reg = ppc_allocate_register(gen->f);
         int offset = (reg->DstRegister.Index * 4 + chan_index) * 16;
         ppc_li(gen->f, offset_reg, offset);
         ppc_stvx(gen->f, src_vec, gen->temps_reg, offset_reg);
         ppc_release_register(gen->f, offset_reg);
      }
      break;
#if 0
   case TGSI_FILE_ADDRESS:
      emit_addrs(
         func,
         xmm,
         reg->DstRegister.Index,
         chan_index );
      break;
#endif
   default:
      assert( 0 );
   }

#if 0
   switch( inst->Instruction.Saturate ) {
   case TGSI_SAT_NONE:
      break;

   case TGSI_SAT_ZERO_ONE:
      /* assert( 0 ); */
      break;

   case TGSI_SAT_MINUS_PLUS_ONE:
      assert( 0 );
      break;
   }
#endif
}


#define STORE( GEN, INST, XMM, INDEX, CHAN )\
   emit_store( GEN, XMM, &(INST).FullDstRegisters[INDEX], &(INST), CHAN )



static void
emit_scalar_unaryop(struct gen_context *gen, struct tgsi_full_instruction *inst)
{
   int v0 = ppc_allocate_vec_register(gen->f);
   int v1 = ppc_allocate_vec_register(gen->f);
   uint chan_index;

   FETCH(gen, *inst, v0, 0, CHAN_X);

   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_RSQ:
      /* v1 = 1.0 / sqrt(v0) */
      ppc_vrsqrtefp(gen->f, v1, v0);
      break;
   case TGSI_OPCODE_RCP:
      /* v1 = 1.0 / v0 */
      ppc_vrefp(gen->f, v1, v0);
      break;
   default:
      assert(0);
   }

   FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
      STORE(gen, *inst, v1, 0, chan_index);
   }
   ppc_release_vec_register(gen->f, v0);
   ppc_release_vec_register(gen->f, v1);
}


static void
emit_unaryop(struct gen_context *gen, struct tgsi_full_instruction *inst)
{
   int v0 = ppc_allocate_vec_register(gen->f);
   uint chan_index;
   FOR_EACH_DST0_ENABLED_CHANNEL(*inst, chan_index) {
      FETCH(gen, *inst, 0, 0, chan_index);   /* v0 = srcreg[0] */
      switch (inst->Instruction.Opcode) {
      case TGSI_OPCODE_ABS:
         /* turn off the most significant bit of each vector float word */
         {
            int v1 = ppc_allocate_vec_register(gen->f);
            ppc_vspltisw(gen->f, v1, -1);  /* v1 = {-1, -1, -1, -1} */
            ppc_vslw(gen->f, v1, v1, v1);  /* v1 = {1<<31, 1<<31, 1<<31, 1<<31} */
            ppc_vandc(gen->f, v0, v0, v1); /* v0 = v0 & ~v1 */
            ppc_release_vec_register(gen->f, v1);
         }
         break;
      case TGSI_OPCODE_FLOOR:
         ppc_vrfim(gen->f, v0, v0);         /* v0 = floor(v0) */
         break;
      case TGSI_OPCODE_FRAC:
         {
            int v1 = ppc_allocate_vec_register(gen->f);
            ppc_vrfim(gen->f, v1, v0);         /* v1 = floor(v0) */
            ppc_vsubfp(gen->f, v0, v0, v1);    /* v0 = v0 - v1 */
            ppc_release_vec_register(gen->f, v1);
         }
         break;
      case TGSI_OPCODE_EXPBASE2:
         ppc_vexptefp(gen->f, v0, v0);      /* v0 = 2^v0 */
         break;
      case TGSI_OPCODE_LOGBASE2:
         /* XXX this may be broken! */
         ppc_vlogefp(gen->f, v0, v0);      /* v0 = log2(v0) */
         break;
      case TGSI_OPCODE_MOV:
         /* nothing */
         break;
      default:
         assert(0);
      }
      STORE(gen, *inst, v0, 0, chan_index);   /* store v0 */
   }
   ppc_release_vec_register(gen->f, v0);
}


static void
emit_binop(struct gen_context *gen, struct tgsi_full_instruction *inst)
{
   int v0 = ppc_allocate_vec_register(gen->f);
   int v1 = ppc_allocate_vec_register(gen->f);
   int v2 = ppc_allocate_vec_register(gen->f);
   uint chan_index;
   FOR_EACH_DST0_ENABLED_CHANNEL(*inst, chan_index) {
      FETCH(gen, *inst, v0, 0, chan_index);   /* v0 = srcreg[0] */
      FETCH(gen, *inst, v1, 1, chan_index);   /* v1 = srcreg[1] */
      switch (inst->Instruction.Opcode) {
      case TGSI_OPCODE_ADD:
         ppc_vaddfp(gen->f, v2, v0, v1);
         break;
      case TGSI_OPCODE_SUB:
         ppc_vsubfp(gen->f, v2, v0, v1);
         break;
      case TGSI_OPCODE_MUL:
         ppc_vxor(gen->f, v2, v2, v2);        /* v2 = {0, 0, 0, 0} */
         ppc_vmaddfp(gen->f, v2, v0, v1, v2); /* v2 = v0 * v1 + v0 */
         break;
      case TGSI_OPCODE_MIN:
         ppc_vminfp(gen->f, v2, v0, v1);
         break;
      case TGSI_OPCODE_MAX:
         ppc_vmaxfp(gen->f, v2, v0, v1);
         break;
      default:
         assert(0);
      }
      STORE(gen, *inst, v2, 0, chan_index);   /* store v2 */
   }
   ppc_release_vec_register(gen->f, v0);
   ppc_release_vec_register(gen->f, v1);
   ppc_release_vec_register(gen->f, v2);
}


/**
 * Vector comparisons, resulting in 1.0 or 0.0 values.
 */
static void
emit_inequality(struct gen_context *gen, struct tgsi_full_instruction *inst)
{
   int v0 = ppc_allocate_vec_register(gen->f);
   int v1 = ppc_allocate_vec_register(gen->f);
   int v2 = ppc_allocate_vec_register(gen->f);
   uint chan_index;
   boolean complement = FALSE;
   int one_vec = gen_one_vec(gen);

   FOR_EACH_DST0_ENABLED_CHANNEL(*inst, chan_index) {
      FETCH(gen, *inst, v0, 0, chan_index);   /* v0 = srcreg[0] */
      FETCH(gen, *inst, v1, 1, chan_index);   /* v1 = srcreg[1] */

      switch (inst->Instruction.Opcode) {
      case TGSI_OPCODE_SNE:
         complement = TRUE;
         /* fall-through */
      case TGSI_OPCODE_SEQ:
         ppc_vcmpeqfpx(gen->f, v2, v0, v1); /* v2 = v0 == v1 ? ~0 : 0 */
         break;

      case TGSI_OPCODE_SGE:
         complement = TRUE;
         /* fall-through */
      case TGSI_OPCODE_SLT:
         ppc_vcmpgtfpx(gen->f, v2, v1, v0); /* v2 = v1 > v0 ? ~0 : 0 */
         break;

      case TGSI_OPCODE_SLE:
         complement = TRUE;
         /* fall-through */
      case TGSI_OPCODE_SGT:
         ppc_vcmpgtfpx(gen->f, v2, v0, v1); /* v2 = v0 > v1 ? ~0 : 0 */
         break;
      default:
         assert(0);
      }

      /* v2 is now {0,0,0,0} or {~0,~0,~0,~0} */

      if (complement)
         ppc_vandc(gen->f, v2, one_vec, v2);    /* v2 = one_vec & ~v2 */
      else
         ppc_vand(gen->f, v2, one_vec, v2);     /* v2 = one_vec & v2 */

      STORE(gen, *inst, v2, 0, chan_index);   /* store v2 */
   }

   ppc_release_vec_register(gen->f, v0);
   ppc_release_vec_register(gen->f, v1);
   ppc_release_vec_register(gen->f, v2);
}


static void
emit_dotprod(struct gen_context *gen, struct tgsi_full_instruction *inst)
{
   int v0 = ppc_allocate_vec_register(gen->f);
   int v1 = ppc_allocate_vec_register(gen->f);
   int v2 = ppc_allocate_vec_register(gen->f);
   uint chan_index;

   ppc_vxor(gen->f, v2, v2, v2);           /* v2 = {0, 0, 0, 0} */

   FETCH(gen, *inst, v0, 0, CHAN_X);       /* v0 = src0.XXXX */
   FETCH(gen, *inst, v1, 1, CHAN_X);       /* v1 = src1.XXXX */
   ppc_vmaddfp(gen->f, v2, v0, v1, v2);    /* v2 = v0 * v1 + v2 */

   FETCH(gen, *inst, v0, 0, CHAN_Y);       /* v0 = src0.YYYY */
   FETCH(gen, *inst, v1, 1, CHAN_Y);       /* v1 = src1.YYYY */
   ppc_vmaddfp(gen->f, v2, v0, v1, v2);    /* v2 = v0 * v1 + v2 */

   FETCH(gen, *inst, v0, 0, CHAN_Z);       /* v0 = src0.ZZZZ */
   FETCH(gen, *inst, v1, 1, CHAN_Z);       /* v1 = src1.ZZZZ */
   ppc_vmaddfp(gen->f, v2, v0, v1, v2);    /* v2 = v0 * v1 + v2 */

   if (inst->Instruction.Opcode == TGSI_OPCODE_DP4) {
      FETCH(gen, *inst, v0, 0, CHAN_W);    /* v0 = src0.WWWW */
      FETCH(gen, *inst, v1, 1, CHAN_W);    /* v1 = src1.WWWW */
      ppc_vmaddfp(gen->f, v2, v0, v1, v2); /* v2 = v0 * v1 + v2 */
   }
   else if (inst->Instruction.Opcode == TGSI_OPCODE_DPH) {
      FETCH(gen, *inst, v1, 1, CHAN_W);    /* v1 = src1.WWWW */
      ppc_vaddfp(gen->f, v2, v2, v1);      /* v2 = v2 + v1 */
   }

   FOR_EACH_DST0_ENABLED_CHANNEL(*inst, chan_index) {
      STORE(gen, *inst, v2, 0, chan_index);  /* store v2 */
   }
   ppc_release_vec_register(gen->f, v0);
   ppc_release_vec_register(gen->f, v1);
   ppc_release_vec_register(gen->f, v2);
}


static void
emit_triop(struct gen_context *gen, struct tgsi_full_instruction *inst)
{
   int v0 = ppc_allocate_vec_register(gen->f);
   int v1 = ppc_allocate_vec_register(gen->f);
   int v2 = ppc_allocate_vec_register(gen->f);
   int v3 = ppc_allocate_vec_register(gen->f);
   uint chan_index;
   FOR_EACH_DST0_ENABLED_CHANNEL(*inst, chan_index) {
      FETCH(gen, *inst, v0, 0, chan_index);   /* v0 = srcreg[0] */
      FETCH(gen, *inst, v1, 1, chan_index);   /* v1 = srcreg[1] */
      FETCH(gen, *inst, v2, 2, chan_index);   /* v2 = srcreg[2] */
      switch (inst->Instruction.Opcode) {
      case TGSI_OPCODE_MAD:
         ppc_vmaddfp(gen->f, v3, v0, v1, v2);   /* v3 = v0 * v1 + v2 */
         break;
      case TGSI_OPCODE_LRP:
         ppc_vsubfp(gen->f, v3, v1, v2);        /* v3 = v1 - v2 */
         ppc_vmaddfp(gen->f, v3, v0, v3, v2);   /* v3 = v0 * v3 + v2 */
         break;
      default:
         assert(0);
      }
      STORE(gen, *inst, v3, 0, chan_index);   /* store v3 */
   }
   ppc_release_vec_register(gen->f, v0);
   ppc_release_vec_register(gen->f, v1);
   ppc_release_vec_register(gen->f, v2);
   ppc_release_vec_register(gen->f, v3);
}



/** Approximation for vr = pow(va, vb) */
static void
ppc_vec_pow(struct ppc_function *f, int vr, int va, int vb)
{
   /* pow(a,b) ~= exp2(log2(a) * b) */
   int t_vec = ppc_allocate_vec_register(f);
   int zero_vec = ppc_allocate_vec_register(f);

   ppc_vzero(f, zero_vec);

   ppc_vlogefp(f, t_vec, va);                   /* t = log2(va) */
   ppc_vmaddfp(f, t_vec, t_vec, vb, zero_vec);  /* t = t * vb */
   ppc_vexptefp(f, vr, t_vec);                  /* vr = 2^t */

   ppc_release_vec_register(f, t_vec);
   ppc_release_vec_register(f, zero_vec);
}


static void
emit_lit(struct gen_context *gen, struct tgsi_full_instruction *inst)
{
   int one_vec = gen_one_vec(gen);

   /* Compute X */
   if (IS_DST0_CHANNEL_ENABLED(*inst, CHAN_X)) {
      STORE(gen, *inst, one_vec, 0, CHAN_X);
   }

   /* Compute Y, Z */
   if (IS_DST0_CHANNEL_ENABLED(*inst, CHAN_Y) ||
       IS_DST0_CHANNEL_ENABLED(*inst, CHAN_Z)) {
      int x_vec = ppc_allocate_vec_register(gen->f);
      int zero_vec = ppc_allocate_vec_register(gen->f);

      FETCH(gen, *inst, x_vec, 0, CHAN_X);        /* x_vec = src[0].x */

      ppc_vzero(gen->f, zero_vec);                /* zero = {0,0,0,0} */
      ppc_vmaxfp(gen->f, x_vec, x_vec, zero_vec); /* x_vec = max(x_vec, 0) */

      if (IS_DST0_CHANNEL_ENABLED(*inst, CHAN_Y)) {
         STORE(gen, *inst, x_vec, 0, CHAN_Y);        /* store Y */
      }

      if (IS_DST0_CHANNEL_ENABLED(*inst, CHAN_Z)) {
         int y_vec = ppc_allocate_vec_register(gen->f);
         int z_vec = ppc_allocate_vec_register(gen->f);
         int w_vec = ppc_allocate_vec_register(gen->f);
         int pow_vec = ppc_allocate_vec_register(gen->f);
         int pos_vec = ppc_allocate_vec_register(gen->f);
         int p128_vec = ppc_allocate_vec_register(gen->f);
         int n128_vec = ppc_allocate_vec_register(gen->f);

         FETCH(gen, *inst, y_vec, 0, CHAN_Y);        /* y_vec = src[0].y */
         ppc_vmaxfp(gen->f, y_vec, y_vec, zero_vec); /* y_vec = max(y_vec, 0) */

         FETCH(gen, *inst, w_vec, 0, CHAN_W);        /* w_vec = src[0].w */

         /* clamp Y to [-128, 128] */
         load_constant_vec(gen, p128_vec, 128.0f);
         load_constant_vec(gen, n128_vec, -128.0f);
         ppc_vmaxfp(gen->f, y_vec, y_vec, n128_vec); /* y = max(y, -128) */
         ppc_vminfp(gen->f, y_vec, y_vec, p128_vec); /* y = min(y, 128) */

         /* if temp.x > 0
          *    z = pow(tmp.y, tmp.w)
          * else
          *    z = 0.0
          */
         ppc_vec_pow(gen->f, pow_vec, y_vec, w_vec);      /* pow = pow(y, w) */
         ppc_vcmpgtfpx(gen->f, pos_vec, x_vec, zero_vec); /* pos = x > 0 */
         ppc_vand(gen->f, z_vec, pow_vec, pos_vec);       /* z = pow & pos */

         STORE(gen, *inst, z_vec, 0, CHAN_Z);             /* store Z */

         ppc_release_vec_register(gen->f, y_vec);
         ppc_release_vec_register(gen->f, z_vec);
         ppc_release_vec_register(gen->f, w_vec);
         ppc_release_vec_register(gen->f, pow_vec);
         ppc_release_vec_register(gen->f, pos_vec);
         ppc_release_vec_register(gen->f, p128_vec);
         ppc_release_vec_register(gen->f, n128_vec);
      }

      ppc_release_vec_register(gen->f, x_vec);
      ppc_release_vec_register(gen->f, zero_vec);
   }

   /* Compute W */
   if (IS_DST0_CHANNEL_ENABLED(*inst, CHAN_W)) {
      STORE(gen, *inst, one_vec, 0, CHAN_W);
   }
}


static int
emit_instruction(struct gen_context *gen,
                 struct tgsi_full_instruction *inst)
{
   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_MOV:
   case TGSI_OPCODE_ABS:
   case TGSI_OPCODE_FLOOR:
   case TGSI_OPCODE_FRAC:
   case TGSI_OPCODE_EXPBASE2:
   case TGSI_OPCODE_LOGBASE2:
      emit_unaryop(gen, inst);
      break;
   case TGSI_OPCODE_RSQ:
   case TGSI_OPCODE_RCP:
      emit_scalar_unaryop(gen, inst);
      break;
   case TGSI_OPCODE_ADD:
   case TGSI_OPCODE_SUB:
   case TGSI_OPCODE_MUL:
   case TGSI_OPCODE_MIN:
   case TGSI_OPCODE_MAX:
      emit_binop(gen, inst);
      break;
   case TGSI_OPCODE_SEQ:
   case TGSI_OPCODE_SNE:
   case TGSI_OPCODE_SLT:
   case TGSI_OPCODE_SGT:
   case TGSI_OPCODE_SLE:
   case TGSI_OPCODE_SGE:
      emit_inequality(gen, inst);
      break;
   case TGSI_OPCODE_MAD:
   case TGSI_OPCODE_LRP:
      emit_triop(gen, inst);
      break;
   case TGSI_OPCODE_DP3:
   case TGSI_OPCODE_DP4:
   case TGSI_OPCODE_DPH:
      emit_dotprod(gen, inst);
      break;
   case TGSI_OPCODE_LIT:
      emit_lit(gen, inst);
      break;
   case TGSI_OPCODE_END:
      /* normal end */
      return 1;
   default:
      return 0;
   }

   
   return 1;
}

static void
emit_declaration(
   struct ppc_function *func,
   struct tgsi_full_declaration *decl )
{
   if( decl->Declaration.File == TGSI_FILE_INPUT ) {
#if 0
      unsigned first, last, mask;
      unsigned i, j;

      first = decl->DeclarationRange.First;
      last = decl->DeclarationRange.Last;
      mask = decl->Declaration.UsageMask;

      for( i = first; i <= last; i++ ) {
         for( j = 0; j < NUM_CHANNELS; j++ ) {
            if( mask & (1 << j) ) {
               switch( decl->Declaration.Interpolate ) {
               case TGSI_INTERPOLATE_CONSTANT:
                  emit_coef_a0( func, 0, i, j );
                  emit_inputs( func, 0, i, j );
                  break;

               case TGSI_INTERPOLATE_LINEAR:
                  emit_tempf( func, 0, 0, TGSI_SWIZZLE_X );
                  emit_coef_dadx( func, 1, i, j );
                  emit_tempf( func, 2, 0, TGSI_SWIZZLE_Y );
                  emit_coef_dady( func, 3, i, j );
                  emit_mul( func, 0, 1 );    /* x * dadx */
                  emit_coef_a0( func, 4, i, j );
                  emit_mul( func, 2, 3 );    /* y * dady */
                  emit_add( func, 0, 4 );    /* x * dadx + a0 */
                  emit_add( func, 0, 2 );    /* x * dadx + y * dady + a0 */
                  emit_inputs( func, 0, i, j );
                  break;

               case TGSI_INTERPOLATE_PERSPECTIVE:
                  emit_tempf( func, 0, 0, TGSI_SWIZZLE_X );
                  emit_coef_dadx( func, 1, i, j );
                  emit_tempf( func, 2, 0, TGSI_SWIZZLE_Y );
                  emit_coef_dady( func, 3, i, j );
                  emit_mul( func, 0, 1 );    /* x * dadx */
                  emit_tempf( func, 4, 0, TGSI_SWIZZLE_W );
                  emit_coef_a0( func, 5, i, j );
                  emit_rcp( func, 4, 4 );    /* 1.0 / w */
                  emit_mul( func, 2, 3 );    /* y * dady */
                  emit_add( func, 0, 5 );    /* x * dadx + a0 */
                  emit_add( func, 0, 2 );    /* x * dadx + y * dady + a0 */
                  emit_mul( func, 0, 4 );    /* (x * dadx + y * dady + a0) / w */
                  emit_inputs( func, 0, i, j );
                  break;

               default:
                  assert( 0 );
		  break;
               }
            }
         }
      }
#endif
   }
}



static void
emit_prologue(struct ppc_function *func)
{
   /* XXX set up stack frame */
}


static void
emit_epilogue(struct ppc_function *func)
{
   ppc_return(func);
   /* XXX restore prev stack frame */
}



/**
 * Translate a TGSI vertex/fragment shader to PPC code.
 *
 * \param tokens  the TGSI input shader
 * \param func  the output PPC code/function
 * \param immediates  buffer to place immediates, later passed to PPC func
 * \return TRUE for success, FALSE if translation failed
 */
boolean
tgsi_emit_ppc(const struct tgsi_token *tokens,
              struct ppc_function *func,
              float (*immediates)[4],
              boolean do_swizzles )
{
   static int use_ppc_asm = -1;
   struct tgsi_parse_context parse;
   /*boolean instruction_phase = FALSE;*/
   unsigned ok = 1;
   uint num_immediates = 0;
   struct gen_context gen;

   if (use_ppc_asm < 0) {
      /* If GALLIUM_NOPPC is set, don't use PPC codegen */
      use_ppc_asm = !debug_get_bool_option("GALLIUM_NOPPC", FALSE);
   }
   if (!use_ppc_asm)
      return FALSE;

   util_init_math();

   gen.f = func;
   gen.inputs_reg = ppc_reserve_register(func, 3);   /* first function param */
   gen.outputs_reg = ppc_reserve_register(func, 4);  /* second function param */
   gen.temps_reg = ppc_reserve_register(func, 5);    /* ... */
   gen.immed_reg = ppc_reserve_register(func, 6);
   gen.const_reg = ppc_reserve_register(func, 7);
   gen.builtins_reg = ppc_reserve_register(func, 8);
   gen.one_vec = -1;
   gen.bit31_vec = -1;

   emit_prologue(func);

   tgsi_parse_init( &parse, tokens );

   while (!tgsi_parse_end_of_tokens(&parse) && ok) {
      tgsi_parse_token(&parse);

      switch (parse.FullToken.Token.Type) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         if (parse.FullHeader.Processor.Processor == TGSI_PROCESSOR_FRAGMENT) {
            emit_declaration(func, &parse.FullToken.FullDeclaration );
         }
         break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         ok = emit_instruction(&gen, &parse.FullToken.FullInstruction);

	 if (!ok) {
	    debug_printf("failed to translate tgsi opcode %d to PPC (%s)\n", 
			 parse.FullToken.FullInstruction.Instruction.Opcode,
                         parse.FullHeader.Processor.Processor == TGSI_PROCESSOR_VERTEX ?
                         "vertex shader" : "fragment shader");
	 }
         break;

      case TGSI_TOKEN_TYPE_IMMEDIATE:
         /* splat each immediate component into a float[4] vector for SoA */
         {
            const uint size = parse.FullToken.FullImmediate.Immediate.Size - 1;
            float *imm = (float *) immediates;
            uint i;
            assert(size <= 4);
            assert(num_immediates < TGSI_EXEC_NUM_IMMEDIATES);
            for (i = 0; i < size; i++) {
               const float value =
                  parse.FullToken.FullImmediate.u.ImmediateFloat32[i].Float;
               imm[num_immediates * 4 + 0] = 
               imm[num_immediates * 4 + 1] = 
               imm[num_immediates * 4 + 2] = 
               imm[num_immediates * 4 + 3] = value;
               num_immediates++;
            }
         }
         break;

      default:
	 ok = 0;
         assert( 0 );
      }
   }

   emit_epilogue(func);

   tgsi_parse_free( &parse );

   return ok;
}

#endif /* PIPE_ARCH_PPC */
