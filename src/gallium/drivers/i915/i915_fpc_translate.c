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


#include <stdarg.h>

#include "i915_reg.h"
#include "i915_context.h"
#include "i915_fpc.h"

#include "pipe/p_shader_tokens.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_string.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_dump.h"

#include "draw/draw_vertex.h"


/**
 * Simple pass-through fragment shader to use when we don't have
 * a real shader (or it fails to compile for some reason).
 */
static unsigned passthrough[] = 
{
   _3DSTATE_PIXEL_SHADER_PROGRAM | ((2*3)-1),

   /* declare input color:
    */
   (D0_DCL | 
    (REG_TYPE_T << D0_TYPE_SHIFT) | 
    (T_DIFFUSE << D0_NR_SHIFT) | 
    D0_CHANNEL_ALL),
   0,
   0,

   /* move to output color:
    */
   (A0_MOV | 
    (REG_TYPE_OC << A0_DEST_TYPE_SHIFT) | 
    A0_DEST_CHANNEL_ALL | 
    (REG_TYPE_T << A0_SRC0_TYPE_SHIFT) |
    (T_DIFFUSE << A0_SRC0_NR_SHIFT)),
   0x01230000,			/* .xyzw */
   0
};


/* 1, -1/3!, 1/5!, -1/7! */
static const float sin_constants[4] = { 1.0,
   -1.0f / (3 * 2 * 1),
   1.0f / (5 * 4 * 3 * 2 * 1),
   -1.0f / (7 * 6 * 5 * 4 * 3 * 2 * 1)
};

/* 1, -1/2!, 1/4!, -1/6! */
static const float cos_constants[4] = { 1.0,
   -1.0f / (2 * 1),
   1.0f / (4 * 3 * 2 * 1),
   -1.0f / (6 * 5 * 4 * 3 * 2 * 1)
};



/**
 * component-wise negation of ureg
 */
static INLINE int
negate(int reg, int x, int y, int z, int w)
{
   /* Another neat thing about the UREG representation */
   return reg ^ (((x & 1) << UREG_CHANNEL_X_NEGATE_SHIFT) |
                 ((y & 1) << UREG_CHANNEL_Y_NEGATE_SHIFT) |
                 ((z & 1) << UREG_CHANNEL_Z_NEGATE_SHIFT) |
                 ((w & 1) << UREG_CHANNEL_W_NEGATE_SHIFT));
}


/**
 * In the event of a translation failure, we'll generate a simple color
 * pass-through program.
 */
static void
i915_use_passthrough_shader(struct i915_fragment_shader *fs)
{
   fs->program = (uint *) MALLOC(sizeof(passthrough));
   if (fs->program) {
      memcpy(fs->program, passthrough, sizeof(passthrough));
      fs->program_len = Elements(passthrough);
   }
   fs->num_constants = 0;
}


void
i915_program_error(struct i915_fp_compile *p, const char *msg, ...)
{
   va_list args;
   char buffer[1024];

   debug_printf("i915_program_error: ");
   va_start( args, msg );  
   util_vsnprintf( buffer, sizeof(buffer), msg, args );
   va_end( args );
   debug_printf("%s", buffer);
   debug_printf("\n");

   p->error = 1;
}



/**
 * Construct a ureg for the given source register.  Will emit
 * constants, apply swizzling and negation as needed.
 */
static uint
src_vector(struct i915_fp_compile *p,
           const struct tgsi_full_src_register *source)
{
   uint index = source->Register.Index;
   uint src = 0, sem_name, sem_ind;

   switch (source->Register.File) {
   case TGSI_FILE_TEMPORARY:
      if (source->Register.Index >= I915_MAX_TEMPORARY) {
         i915_program_error(p, "Exceeded max temporary reg");
         return 0;
      }
      src = UREG(REG_TYPE_R, index);
      break;
   case TGSI_FILE_INPUT:
      /* XXX: Packing COL1, FOGC into a single attribute works for
       * texenv programs, but will fail for real fragment programs
       * that use these attributes and expect them to be a full 4
       * components wide.  Could use a texcoord to pass these
       * attributes if necessary, but that won't work in the general
       * case.
       * 
       * We also use a texture coordinate to pass wpos when possible.
       */

      sem_name = p->shader->info.input_semantic_name[index];
      sem_ind = p->shader->info.input_semantic_index[index];

      switch (sem_name) {
      case TGSI_SEMANTIC_POSITION:
         debug_printf("SKIP SEM POS\n");
         /*
         assert(p->wpos_tex != -1);
         src = i915_emit_decl(p, REG_TYPE_T, p->wpos_tex, D0_CHANNEL_ALL);
         */
         break;
      case TGSI_SEMANTIC_COLOR:
         if (sem_ind == 0) {
            src = i915_emit_decl(p, REG_TYPE_T, T_DIFFUSE, D0_CHANNEL_ALL);
         }
         else {
            /* secondary color */
            assert(sem_ind == 1);
            src = i915_emit_decl(p, REG_TYPE_T, T_SPECULAR, D0_CHANNEL_XYZ);
            src = swizzle(src, X, Y, Z, ONE);
         }
         break;
      case TGSI_SEMANTIC_FOG:
         src = i915_emit_decl(p, REG_TYPE_T, T_FOG_W, D0_CHANNEL_W);
         src = swizzle(src, W, W, W, W);
         break;
      case TGSI_SEMANTIC_GENERIC:
         /* usually a texcoord */
         src = i915_emit_decl(p, REG_TYPE_T, T_TEX0 + sem_ind, D0_CHANNEL_ALL);
         break;
      default:
         i915_program_error(p, "Bad source->Index");
         return 0;
      }
      break;

   case TGSI_FILE_IMMEDIATE:
      assert(index < p->num_immediates);
      index = p->immediates_map[index];
      /* fall-through */
   case TGSI_FILE_CONSTANT:
      src = UREG(REG_TYPE_CONST, index);
      break;

   default:
      i915_program_error(p, "Bad source->File");
      return 0;
   }

   src = swizzle(src,
		 source->Register.SwizzleX,
		 source->Register.SwizzleY,
		 source->Register.SwizzleZ,
		 source->Register.SwizzleW);


   /* There's both negate-all-components and per-component negation.
    * Try to handle both here.
    */
   {
      int n = source->Register.Negate;
      src = negate(src, n, n, n, n);
   }

   /* no abs() */
#if 0
   /* XXX assertions disabled to allow arbfplight.c to run */
   /* XXX enable these assertions, or fix things */
   assert(!source->Register.Absolute);
#endif
   return src;
}


/**
 * Construct a ureg for a destination register.
 */
static uint
get_result_vector(struct i915_fp_compile *p,
                  const struct tgsi_full_dst_register *dest)
{
   switch (dest->Register.File) {
   case TGSI_FILE_OUTPUT:
      {
         uint sem_name = p->shader->info.output_semantic_name[dest->Register.Index];
         switch (sem_name) {
         case TGSI_SEMANTIC_POSITION:
            return UREG(REG_TYPE_OD, 0);
         case TGSI_SEMANTIC_COLOR:
            return UREG(REG_TYPE_OC, 0);
         default:
            i915_program_error(p, "Bad inst->DstReg.Index/semantics");
            return 0;
         }
      }
   case TGSI_FILE_TEMPORARY:
      return UREG(REG_TYPE_R, dest->Register.Index);
   default:
      i915_program_error(p, "Bad inst->DstReg.File");
      return 0;
   }
}


/**
 * Compute flags for saturation and writemask.
 */
static uint
get_result_flags(const struct tgsi_full_instruction *inst)
{
   const uint writeMask
      = inst->Dst[0].Register.WriteMask;
   uint flags = 0x0;

   if (inst->Instruction.Saturate == TGSI_SAT_ZERO_ONE)
      flags |= A0_DEST_SATURATE;

   if (writeMask & TGSI_WRITEMASK_X)
      flags |= A0_DEST_CHANNEL_X;
   if (writeMask & TGSI_WRITEMASK_Y)
      flags |= A0_DEST_CHANNEL_Y;
   if (writeMask & TGSI_WRITEMASK_Z)
      flags |= A0_DEST_CHANNEL_Z;
   if (writeMask & TGSI_WRITEMASK_W)
      flags |= A0_DEST_CHANNEL_W;

   return flags;
}


/**
 * Convert TGSI_TEXTURE_x token to DO_SAMPLE_TYPE_x token
 */
static uint
translate_tex_src_target(struct i915_fp_compile *p, uint tex)
{
   switch (tex) {
   case TGSI_TEXTURE_SHADOW1D:
      /* fall-through */
   case TGSI_TEXTURE_1D:
      return D0_SAMPLE_TYPE_2D;

   case TGSI_TEXTURE_SHADOW2D:
      /* fall-through */
   case TGSI_TEXTURE_2D:
      return D0_SAMPLE_TYPE_2D;

   case TGSI_TEXTURE_SHADOWRECT:
      /* fall-through */
   case TGSI_TEXTURE_RECT:
      return D0_SAMPLE_TYPE_2D;

   case TGSI_TEXTURE_3D:
      return D0_SAMPLE_TYPE_VOLUME;

   case TGSI_TEXTURE_CUBE:
      return D0_SAMPLE_TYPE_CUBE;

   default:
      i915_program_error(p, "TexSrc type");
      return 0;
   }
}


/**
 * Generate texel lookup instruction.
 */
static void
emit_tex(struct i915_fp_compile *p,
         const struct tgsi_full_instruction *inst,
         uint opcode)
{
   uint texture = inst->Texture.Texture;
   uint unit = inst->Src[1].Register.Index;
   uint tex = translate_tex_src_target( p, texture );
   uint sampler = i915_emit_decl(p, REG_TYPE_S, unit, tex);
   uint coord = src_vector( p, &inst->Src[0]);

   i915_emit_texld( p,
                    get_result_vector( p, &inst->Dst[0] ),
                    get_result_flags( inst ),
                    sampler,
                    coord,
                    opcode);
}


/**
 * Generate a simple arithmetic instruction
 * \param opcode  the i915 opcode
 * \param numArgs  the number of input/src arguments
 */
static void
emit_simple_arith(struct i915_fp_compile *p,
                  const struct tgsi_full_instruction *inst,
                  uint opcode, uint numArgs)
{
   uint arg1, arg2, arg3;

   assert(numArgs <= 3);

   arg1 = (numArgs < 1) ? 0 : src_vector( p, &inst->Src[0] );
   arg2 = (numArgs < 2) ? 0 : src_vector( p, &inst->Src[1] );
   arg3 = (numArgs < 3) ? 0 : src_vector( p, &inst->Src[2] );

   i915_emit_arith( p,
                    opcode,
                    get_result_vector( p, &inst->Dst[0]),
                    get_result_flags( inst ), 0,
                    arg1,
                    arg2,
                    arg3 );
}


/** As above, but swap the first two src regs */
static void
emit_simple_arith_swap2(struct i915_fp_compile *p,
                        const struct tgsi_full_instruction *inst,
                        uint opcode, uint numArgs)
{
   struct tgsi_full_instruction inst2;

   assert(numArgs == 2);

   /* transpose first two registers */
   inst2 = *inst;
   inst2.Src[0] = inst->Src[1];
   inst2.Src[1] = inst->Src[0];

   emit_simple_arith(p, &inst2, opcode, numArgs);
}


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/*
 * Translate TGSI instruction to i915 instruction.
 *
 * Possible concerns:
 *
 * SIN, COS -- could use another taylor step?
 * LIT      -- results seem a little different to sw mesa
 * LOG      -- different to mesa on negative numbers, but this is conformant.
 */ 
static void
i915_translate_instruction(struct i915_fp_compile *p,
                           const struct tgsi_full_instruction *inst)
{
   uint writemask;
   uint src0, src1, src2, flags;
   uint tmp = 0;

   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_ABS:
      src0 = src_vector(p, &inst->Src[0]);
      i915_emit_arith(p,
                      A0_MAX,
                      get_result_vector(p, &inst->Dst[0]),
                      get_result_flags(inst), 0,
                      src0, negate(src0, 1, 1, 1, 1), 0);
      break;

   case TGSI_OPCODE_ADD:
      emit_simple_arith(p, inst, A0_ADD, 2);
      break;

   case TGSI_OPCODE_CMP:
      src0 = src_vector(p, &inst->Src[0]);
      src1 = src_vector(p, &inst->Src[1]);
      src2 = src_vector(p, &inst->Src[2]);
      i915_emit_arith(p, A0_CMP, 
                      get_result_vector(p, &inst->Dst[0]),
                      get_result_flags(inst), 
                      0, src0, src2, src1);   /* NOTE: order of src2, src1 */
      break;

   case TGSI_OPCODE_COS:
      src0 = src_vector(p, &inst->Src[0]);
      tmp = i915_get_utemp(p);

      i915_emit_arith(p,
                      A0_MUL,
                      tmp, A0_DEST_CHANNEL_X, 0,
                      src0, i915_emit_const1f(p, 1.0f / (float) (M_PI * 2.0)), 0);

      i915_emit_arith(p, A0_MOD, tmp, A0_DEST_CHANNEL_X, 0, tmp, 0, 0);

      /* By choosing different taylor constants, could get rid of this mul:
       */
      i915_emit_arith(p,
                      A0_MUL,
                      tmp, A0_DEST_CHANNEL_X, 0,
                      tmp, i915_emit_const1f(p, (float) (M_PI * 2.0)), 0);

      /* 
       * t0.xy = MUL x.xx11, x.x1111  ; x^2, x, 1, 1
       * t0 = MUL t0.xyxy t0.xx11 ; x^4, x^3, x^2, 1
       * t0 = MUL t0.xxz1 t0.z111    ; x^6 x^4 x^2 1
       * result = DP4 t0, cos_constants
       */
      i915_emit_arith(p,
                      A0_MUL,
                      tmp, A0_DEST_CHANNEL_XY, 0,
                      swizzle(tmp, X, X, ONE, ONE),
                      swizzle(tmp, X, ONE, ONE, ONE), 0);

      i915_emit_arith(p,
                      A0_MUL,
                      tmp, A0_DEST_CHANNEL_XYZ, 0,
                      swizzle(tmp, X, Y, X, ONE),
                      swizzle(tmp, X, X, ONE, ONE), 0);

      i915_emit_arith(p,
                      A0_MUL,
                      tmp, A0_DEST_CHANNEL_XYZ, 0,
                      swizzle(tmp, X, X, Z, ONE),
                      swizzle(tmp, Z, ONE, ONE, ONE), 0);

      i915_emit_arith(p,
                      A0_DP4,
                      get_result_vector(p, &inst->Dst[0]),
                      get_result_flags(inst), 0,
                      swizzle(tmp, ONE, Z, Y, X),
                      i915_emit_const4fv(p, cos_constants), 0);
      break;

   case TGSI_OPCODE_DP3:
      emit_simple_arith(p, inst, A0_DP3, 2);
      break;

   case TGSI_OPCODE_DP4:
      emit_simple_arith(p, inst, A0_DP4, 2);
      break;

   case TGSI_OPCODE_DPH:
      src0 = src_vector(p, &inst->Src[0]);
      src1 = src_vector(p, &inst->Src[1]);

      i915_emit_arith(p,
                      A0_DP4,
                      get_result_vector(p, &inst->Dst[0]),
                      get_result_flags(inst), 0,
                      swizzle(src0, X, Y, Z, ONE), src1, 0);
      break;

   case TGSI_OPCODE_DST:
      src0 = src_vector(p, &inst->Src[0]);
      src1 = src_vector(p, &inst->Src[1]);

      /* result[0] = 1    * 1;
       * result[1] = a[1] * b[1];
       * result[2] = a[2] * 1;
       * result[3] = 1    * b[3];
       */
      i915_emit_arith(p,
                      A0_MUL,
                      get_result_vector(p, &inst->Dst[0]),
                      get_result_flags(inst), 0,
                      swizzle(src0, ONE, Y, Z, ONE),
                      swizzle(src1, ONE, Y, ONE, W), 0);
      break;

   case TGSI_OPCODE_END:
      /* no-op */
      break;

   case TGSI_OPCODE_EX2:
      src0 = src_vector(p, &inst->Src[0]);

      i915_emit_arith(p,
                      A0_EXP,
                      get_result_vector(p, &inst->Dst[0]),
                      get_result_flags(inst), 0,
                      swizzle(src0, X, X, X, X), 0, 0);
      break;

   case TGSI_OPCODE_FLR:
      emit_simple_arith(p, inst, A0_FLR, 1);
      break;

   case TGSI_OPCODE_FRC:
      emit_simple_arith(p, inst, A0_FRC, 1);
      break;

   case TGSI_OPCODE_KIL:
      /* kill if src[0].x < 0 || src[0].y < 0 ... */
      src0 = src_vector(p, &inst->Src[0]);
      tmp = i915_get_utemp(p);

      i915_emit_texld(p,
                      tmp,                   /* dest reg: a dummy reg */
                      A0_DEST_CHANNEL_ALL,   /* dest writemask */
                      0,                     /* sampler */
                      src0,                  /* coord*/
                      T0_TEXKILL);           /* opcode */
      break;

   case TGSI_OPCODE_KILP:
      assert(0); /* not tested yet */
      break;

   case TGSI_OPCODE_LG2:
      src0 = src_vector(p, &inst->Src[0]);

      i915_emit_arith(p,
                      A0_LOG,
                      get_result_vector(p, &inst->Dst[0]),
                      get_result_flags(inst), 0,
                      swizzle(src0, X, X, X, X), 0, 0);
      break;

   case TGSI_OPCODE_LIT:
      src0 = src_vector(p, &inst->Src[0]);
      tmp = i915_get_utemp(p);

      /* tmp = max( a.xyzw, a.00zw )
       * XXX: Clamp tmp.w to -128..128
       * tmp.y = log(tmp.y)
       * tmp.y = tmp.w * tmp.y
       * tmp.y = exp(tmp.y)
       * result = cmp (a.11-x1, a.1x01, a.1xy1 )
       */
      i915_emit_arith(p, A0_MAX, tmp, A0_DEST_CHANNEL_ALL, 0,
                      src0, swizzle(src0, ZERO, ZERO, Z, W), 0);

      i915_emit_arith(p, A0_LOG, tmp, A0_DEST_CHANNEL_Y, 0,
                      swizzle(tmp, Y, Y, Y, Y), 0, 0);

      i915_emit_arith(p, A0_MUL, tmp, A0_DEST_CHANNEL_Y, 0,
                      swizzle(tmp, ZERO, Y, ZERO, ZERO),
                      swizzle(tmp, ZERO, W, ZERO, ZERO), 0);

      i915_emit_arith(p, A0_EXP, tmp, A0_DEST_CHANNEL_Y, 0,
                      swizzle(tmp, Y, Y, Y, Y), 0, 0);

      i915_emit_arith(p, A0_CMP,
                      get_result_vector(p, &inst->Dst[0]),
                      get_result_flags(inst), 0,
                      negate(swizzle(tmp, ONE, ONE, X, ONE), 0, 0, 1, 0),
                      swizzle(tmp, ONE, X, ZERO, ONE),
                      swizzle(tmp, ONE, X, Y, ONE));

      break;

   case TGSI_OPCODE_LRP:
      src0 = src_vector(p, &inst->Src[0]);
      src1 = src_vector(p, &inst->Src[1]);
      src2 = src_vector(p, &inst->Src[2]);
      flags = get_result_flags(inst);
      tmp = i915_get_utemp(p);

      /* b*a + c*(1-a)
       *
       * b*a + c - ca 
       *
       * tmp = b*a + c, 
       * result = (-c)*a + tmp 
       */
      i915_emit_arith(p, A0_MAD, tmp,
                      flags & A0_DEST_CHANNEL_ALL, 0, src1, src0, src2);

      i915_emit_arith(p, A0_MAD,
                      get_result_vector(p, &inst->Dst[0]),
                      flags, 0, negate(src2, 1, 1, 1, 1), src0, tmp);
      break;

   case TGSI_OPCODE_MAD:
      emit_simple_arith(p, inst, A0_MAD, 3);
      break;

   case TGSI_OPCODE_MAX:
      emit_simple_arith(p, inst, A0_MAX, 2);
      break;

   case TGSI_OPCODE_MIN:
      src0 = src_vector(p, &inst->Src[0]);
      src1 = src_vector(p, &inst->Src[1]);
      tmp = i915_get_utemp(p);
      flags = get_result_flags(inst);

      i915_emit_arith(p,
                      A0_MAX,
                      tmp, flags & A0_DEST_CHANNEL_ALL, 0,
                      negate(src0, 1, 1, 1, 1),
                      negate(src1, 1, 1, 1, 1), 0);

      i915_emit_arith(p,
                      A0_MOV,
                      get_result_vector(p, &inst->Dst[0]),
                      flags, 0, negate(tmp, 1, 1, 1, 1), 0, 0);
      break;

   case TGSI_OPCODE_MOV:
      emit_simple_arith(p, inst, A0_MOV, 1);
      break;

   case TGSI_OPCODE_MUL:
      emit_simple_arith(p, inst, A0_MUL, 2);
      break;

   case TGSI_OPCODE_POW:
      src0 = src_vector(p, &inst->Src[0]);
      src1 = src_vector(p, &inst->Src[1]);
      tmp = i915_get_utemp(p);
      flags = get_result_flags(inst);

      /* XXX: masking on intermediate values, here and elsewhere.
       */
      i915_emit_arith(p,
                      A0_LOG,
                      tmp, A0_DEST_CHANNEL_X, 0,
                      swizzle(src0, X, X, X, X), 0, 0);

      i915_emit_arith(p, A0_MUL, tmp, A0_DEST_CHANNEL_X, 0, tmp, src1, 0);

      i915_emit_arith(p,
                      A0_EXP,
                      get_result_vector(p, &inst->Dst[0]),
                      flags, 0, swizzle(tmp, X, X, X, X), 0, 0);
      break;
      
   case TGSI_OPCODE_RET:
      /* XXX: no-op? */
      break;
      
   case TGSI_OPCODE_RCP:
      src0 = src_vector(p, &inst->Src[0]);

      i915_emit_arith(p,
                      A0_RCP,
                      get_result_vector(p, &inst->Dst[0]),
                         get_result_flags(inst), 0,
                      swizzle(src0, X, X, X, X), 0, 0);
      break;

   case TGSI_OPCODE_RSQ:
      src0 = src_vector(p, &inst->Src[0]);

      i915_emit_arith(p,
                      A0_RSQ,
                      get_result_vector(p, &inst->Dst[0]),
                      get_result_flags(inst), 0,
                      swizzle(src0, X, X, X, X), 0, 0);
      break;

   case TGSI_OPCODE_SCS:
      src0 = src_vector(p, &inst->Src[0]);
      tmp = i915_get_utemp(p);

      /* 
       * t0.xy = MUL x.xx11, x.x1111  ; x^2, x, 1, 1
       * t0 = MUL t0.xyxy t0.xx11 ; x^4, x^3, x^2, x
       * t1 = MUL t0.xyyw t0.yz11    ; x^7 x^5 x^3 x
       * scs.x = DP4 t1, sin_constants
       * t1 = MUL t0.xxz1 t0.z111    ; x^6 x^4 x^2 1
       * scs.y = DP4 t1, cos_constants
       */
      i915_emit_arith(p,
                      A0_MUL,
                      tmp, A0_DEST_CHANNEL_XY, 0,
                      swizzle(src0, X, X, ONE, ONE),
                      swizzle(src0, X, ONE, ONE, ONE), 0);

      i915_emit_arith(p,
                      A0_MUL,
                      tmp, A0_DEST_CHANNEL_ALL, 0,
                      swizzle(tmp, X, Y, X, Y),
                      swizzle(tmp, X, X, ONE, ONE), 0);

      writemask = inst->Dst[0].Register.WriteMask;

      if (writemask & TGSI_WRITEMASK_Y) {
         uint tmp1;

         if (writemask & TGSI_WRITEMASK_X)
            tmp1 = i915_get_utemp(p);
         else
            tmp1 = tmp;

         i915_emit_arith(p,
                         A0_MUL,
                         tmp1, A0_DEST_CHANNEL_ALL, 0,
                         swizzle(tmp, X, Y, Y, W),
                         swizzle(tmp, X, Z, ONE, ONE), 0);

         i915_emit_arith(p,
                         A0_DP4,
                         get_result_vector(p, &inst->Dst[0]),
                         A0_DEST_CHANNEL_Y, 0,
                         swizzle(tmp1, W, Z, Y, X),
                         i915_emit_const4fv(p, sin_constants), 0);
      }

      if (writemask & TGSI_WRITEMASK_X) {
         i915_emit_arith(p,
                         A0_MUL,
                         tmp, A0_DEST_CHANNEL_XYZ, 0,
                         swizzle(tmp, X, X, Z, ONE),
                         swizzle(tmp, Z, ONE, ONE, ONE), 0);

         i915_emit_arith(p,
                         A0_DP4,
                         get_result_vector(p, &inst->Dst[0]),
                         A0_DEST_CHANNEL_X, 0,
                         swizzle(tmp, ONE, Z, Y, X),
                         i915_emit_const4fv(p, cos_constants), 0);
      }
      break;

   case TGSI_OPCODE_SGE:
      emit_simple_arith(p, inst, A0_SGE, 2);
      break;

   case TGSI_OPCODE_SLE:
      /* like SGE, but swap reg0, reg1 */
      emit_simple_arith_swap2(p, inst, A0_SGE, 2);
      break;

   case TGSI_OPCODE_SIN:
      src0 = src_vector(p, &inst->Src[0]);
      tmp = i915_get_utemp(p);

      i915_emit_arith(p,
                      A0_MUL,
                      tmp, A0_DEST_CHANNEL_X, 0,
                      src0, i915_emit_const1f(p, 1.0f / (float) (M_PI * 2.0)), 0);

      i915_emit_arith(p, A0_MOD, tmp, A0_DEST_CHANNEL_X, 0, tmp, 0, 0);

      /* By choosing different taylor constants, could get rid of this mul:
       */
      i915_emit_arith(p,
                      A0_MUL,
                      tmp, A0_DEST_CHANNEL_X, 0,
                      tmp, i915_emit_const1f(p, (float) (M_PI * 2.0)), 0);

      /* 
       * t0.xy = MUL x.xx11, x.x1111  ; x^2, x, 1, 1
       * t0 = MUL t0.xyxy t0.xx11 ; x^4, x^3, x^2, x
       * t1 = MUL t0.xyyw t0.yz11    ; x^7 x^5 x^3 x
       * result = DP4 t1.wzyx, sin_constants
       */
      i915_emit_arith(p,
                      A0_MUL,
                      tmp, A0_DEST_CHANNEL_XY, 0,
                      swizzle(tmp, X, X, ONE, ONE),
                      swizzle(tmp, X, ONE, ONE, ONE), 0);

      i915_emit_arith(p,
                      A0_MUL,
                      tmp, A0_DEST_CHANNEL_ALL, 0,
                      swizzle(tmp, X, Y, X, Y),
                      swizzle(tmp, X, X, ONE, ONE), 0);

      i915_emit_arith(p,
                      A0_MUL,
                      tmp, A0_DEST_CHANNEL_ALL, 0,
                      swizzle(tmp, X, Y, Y, W),
                      swizzle(tmp, X, Z, ONE, ONE), 0);

      i915_emit_arith(p,
                      A0_DP4,
                      get_result_vector(p, &inst->Dst[0]),
                      get_result_flags(inst), 0,
                      swizzle(tmp, W, Z, Y, X),
                      i915_emit_const4fv(p, sin_constants), 0);
      break;

   case TGSI_OPCODE_SLT:
      emit_simple_arith(p, inst, A0_SLT, 2);
      break;

   case TGSI_OPCODE_SGT:
      /* like SLT, but swap reg0, reg1 */
      emit_simple_arith_swap2(p, inst, A0_SLT, 2);
      break;

   case TGSI_OPCODE_SUB:
      src0 = src_vector(p, &inst->Src[0]);
      src1 = src_vector(p, &inst->Src[1]);

      i915_emit_arith(p,
                      A0_ADD,
                      get_result_vector(p, &inst->Dst[0]),
                      get_result_flags(inst), 0,
                      src0, negate(src1, 1, 1, 1, 1), 0);
      break;

   case TGSI_OPCODE_TEX:
      emit_tex(p, inst, T0_TEXLD);
      break;

   case TGSI_OPCODE_TXB:
      emit_tex(p, inst, T0_TEXLDB);
      break;

   case TGSI_OPCODE_TXP:
      emit_tex(p, inst, T0_TEXLDP);
      break;

   case TGSI_OPCODE_XPD:
      /* Cross product:
       *      result.x = src0.y * src1.z - src0.z * src1.y;
       *      result.y = src0.z * src1.x - src0.x * src1.z;
       *      result.z = src0.x * src1.y - src0.y * src1.x;
       *      result.w = undef;
       */
      src0 = src_vector(p, &inst->Src[0]);
      src1 = src_vector(p, &inst->Src[1]);
      tmp = i915_get_utemp(p);

      i915_emit_arith(p,
                      A0_MUL,
                      tmp, A0_DEST_CHANNEL_ALL, 0,
                      swizzle(src0, Z, X, Y, ONE),
                      swizzle(src1, Y, Z, X, ONE), 0);

      i915_emit_arith(p,
                      A0_MAD,
                      get_result_vector(p, &inst->Dst[0]),
                      get_result_flags(inst), 0,
                      swizzle(src0, Y, Z, X, ONE),
                      swizzle(src1, Z, X, Y, ONE),
                      negate(tmp, 1, 1, 1, 0));
      break;

   default:
      i915_program_error(p, "bad opcode %d", inst->Instruction.Opcode);
      p->error = 1;
      return;
   }

   i915_release_utemps(p);
}


/**
 * Translate TGSI fragment shader into i915 hardware instructions.
 * \param p  the translation state
 * \param tokens  the TGSI token array
 */
static void
i915_translate_instructions(struct i915_fp_compile *p,
                            const struct tgsi_token *tokens)
{
   struct i915_fragment_shader *ifs = p->shader;
   struct tgsi_parse_context parse;

   tgsi_parse_init( &parse, tokens );

   while( !tgsi_parse_end_of_tokens( &parse ) ) {

      tgsi_parse_token( &parse );

      switch( parse.FullToken.Token.Type ) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         if (parse.FullToken.FullDeclaration.Declaration.File
                  == TGSI_FILE_CONSTANT) {
            uint i;
            for (i = parse.FullToken.FullDeclaration.Range.First;
                 i <= parse.FullToken.FullDeclaration.Range.Last;
                 i++) {
               assert(ifs->constant_flags[i] == 0x0);
               ifs->constant_flags[i] = I915_CONSTFLAG_USER;
               ifs->num_constants = MAX2(ifs->num_constants, i + 1);
            }
         }
         else if (parse.FullToken.FullDeclaration.Declaration.File
                  == TGSI_FILE_TEMPORARY) {
            uint i;
            for (i = parse.FullToken.FullDeclaration.Range.First;
                 i <= parse.FullToken.FullDeclaration.Range.Last;
                 i++) {
               assert(i < I915_MAX_TEMPORARY);
               /* XXX just use shader->info->file_mask[TGSI_FILE_TEMPORARY] */
               p->temp_flag |= (1 << i); /* mark temp as used */
            }
         }
         break;

      case TGSI_TOKEN_TYPE_IMMEDIATE:
         {
            const struct tgsi_full_immediate *imm
               = &parse.FullToken.FullImmediate;
            const uint pos = p->num_immediates++;
            uint j;
            assert( imm->Immediate.NrTokens <= 4 + 1 );
            for (j = 0; j < imm->Immediate.NrTokens - 1; j++) {
               p->immediates[pos][j] = imm->u[j].Float;
            }
         }
         break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         if (p->first_instruction) {
            /* resolve location of immediates */
            uint i, j;
            for (i = 0; i < p->num_immediates; i++) {
               /* find constant slot for this immediate */
               for (j = 0; j < I915_MAX_CONSTANT; j++) {
                  if (ifs->constant_flags[j] == 0x0) {
                     memcpy(ifs->constants[j],
                            p->immediates[i],
                            4 * sizeof(float));
                     /*printf("immediate %d maps to const %d\n", i, j);*/
                     ifs->constant_flags[j] = 0xf;  /* all four comps used */
                     p->immediates_map[i] = j;
                     ifs->num_constants = MAX2(ifs->num_constants, j + 1);
                     break;
                  }
               }
            }

            p->first_instruction = FALSE;
         }

         i915_translate_instruction(p, &parse.FullToken.FullInstruction);
         break;

      default:
         assert( 0 );
      }

   } /* while */

   tgsi_parse_free (&parse);
}


static struct i915_fp_compile *
i915_init_compile(struct i915_context *i915,
                  struct i915_fragment_shader *ifs)
{
   struct i915_fp_compile *p = CALLOC_STRUCT(i915_fp_compile);

   p->shader = ifs;

   /* Put new constants at end of const buffer, growing downward.
    * The problem is we don't know how many user-defined constants might
    * be specified with pipe->set_constant_buffer().
    * Should pre-scan the user's program to determine the highest-numbered
    * constant referenced.
    */
   ifs->num_constants = 0;
   memset(ifs->constant_flags, 0, sizeof(ifs->constant_flags));

   p->first_instruction = TRUE;

   p->nr_tex_indirect = 1;      /* correct? */
   p->nr_tex_insn = 0;
   p->nr_alu_insn = 0;
   p->nr_decl_insn = 0;

   p->csr = p->program;
   p->decl = p->declarations;
   p->decl_s = 0;
   p->decl_t = 0;
   p->temp_flag = ~0x0 << I915_MAX_TEMPORARY;
   p->utemp_flag = ~0x7;

   p->wpos_tex = -1;

   /* initialize the first program word */
   *(p->decl++) = _3DSTATE_PIXEL_SHADER_PROGRAM;

   return p;
}


/* Copy compile results to the fragment program struct and destroy the
 * compilation context.
 */
static void
i915_fini_compile(struct i915_context *i915, struct i915_fp_compile *p)
{
   struct i915_fragment_shader *ifs = p->shader;
   unsigned long program_size = (unsigned long) (p->csr - p->program);
   unsigned long decl_size = (unsigned long) (p->decl - p->declarations);

   if (p->nr_tex_indirect > I915_MAX_TEX_INDIRECT)
      i915_program_error(p, "Exceeded max nr indirect texture lookups");

   if (p->nr_tex_insn > I915_MAX_TEX_INSN)
      i915_program_error(p, "Exceeded max TEX instructions");

   if (p->nr_alu_insn > I915_MAX_ALU_INSN)
      i915_program_error(p, "Exceeded max ALU instructions");

   if (p->nr_decl_insn > I915_MAX_DECL_INSN)
      i915_program_error(p, "Exceeded max DECL instructions");

   if (p->error) {
      p->NumNativeInstructions = 0;
      p->NumNativeAluInstructions = 0;
      p->NumNativeTexInstructions = 0;
      p->NumNativeTexIndirections = 0;

      i915_use_passthrough_shader(ifs);
   }
   else {
      p->NumNativeInstructions
         = p->nr_alu_insn + p->nr_tex_insn + p->nr_decl_insn;
      p->NumNativeAluInstructions = p->nr_alu_insn;
      p->NumNativeTexInstructions = p->nr_tex_insn;
      p->NumNativeTexIndirections = p->nr_tex_indirect;

      /* patch in the program length */
      p->declarations[0] |= program_size + decl_size - 2;

      /* Copy compilation results to fragment program struct: 
       */
      assert(!ifs->program);
      ifs->program
         = (uint *) MALLOC((program_size + decl_size) * sizeof(uint));
      if (ifs->program) {
         ifs->program_len = program_size + decl_size;

         memcpy(ifs->program,
                p->declarations, 
                decl_size * sizeof(uint));

         memcpy(ifs->program + decl_size, 
                p->program, 
                program_size * sizeof(uint));
      }
   }

   /* Release the compilation struct: 
    */
   FREE(p);
}


/**
 * Find an unused texture coordinate slot to use for fragment WPOS.
 * Update p->fp->wpos_tex with the result (-1 if no used texcoord slot is found).
 */
static void
i915_find_wpos_space(struct i915_fp_compile *p)
{
#if 0
   const uint inputs
      = p->shader->inputs_read | (1 << TGSI_ATTRIB_POS); /*XXX hack*/
   uint i;

   p->wpos_tex = -1;

   if (inputs & (1 << TGSI_ATTRIB_POS)) {
      for (i = 0; i < I915_TEX_UNITS; i++) {
	 if ((inputs & (1 << (TGSI_ATTRIB_TEX0 + i))) == 0) {
	    p->wpos_tex = i;
	    return;
	 }
      }

      i915_program_error(p, "No free texcoord for wpos value");
   }
#else
   if (p->shader->info.input_semantic_name[0] == TGSI_SEMANTIC_POSITION) {
      /* frag shader using the fragment position input */
#if 0
      assert(0);
#endif
   }
#endif
}




/**
 * Rather than trying to intercept and jiggle depth writes during
 * emit, just move the value into its correct position at the end of
 * the program:
 */
static void
i915_fixup_depth_write(struct i915_fp_compile *p)
{
   /* XXX assuming pos/depth is always in output[0] */
   if (p->shader->info.output_semantic_name[0] == TGSI_SEMANTIC_POSITION) {
      const uint depth = UREG(REG_TYPE_OD, 0);

      i915_emit_arith(p,
                      A0_MOV,                     /* opcode */
                      depth,                      /* dest reg */
                      A0_DEST_CHANNEL_W,          /* write mask */
                      0,                          /* saturate? */
                      swizzle(depth, X, Y, Z, Z), /* src0 */
                      0, 0 /* src1, src2 */);
   }
}


void
i915_translate_fragment_program( struct i915_context *i915,
                                 struct i915_fragment_shader *fs)
{
   struct i915_fp_compile *p = i915_init_compile(i915, fs);
   const struct tgsi_token *tokens = fs->state.tokens;

   i915_find_wpos_space(p);

#if 0
   tgsi_dump(tokens, 0);
#endif

   i915_translate_instructions(p, tokens);
   i915_fixup_depth_write(p);

   i915_fini_compile(i915, p);
}
