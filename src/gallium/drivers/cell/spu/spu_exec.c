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

/**
 * TGSI interpretor/executor.
 *
 * Flow control information:
 *
 * Since we operate on 'quads' (4 pixels or 4 vertices in parallel)
 * flow control statements (IF/ELSE/ENDIF, LOOP/ENDLOOP) require special
 * care since a condition may be true for some quad components but false
 * for other components.
 *
 * We basically execute all statements (even if they're in the part of
 * an IF/ELSE clause that's "not taken") and use a special mask to
 * control writing to destination registers.  This is the ExecMask.
 * See store_dest().
 *
 * The ExecMask is computed from three other masks (CondMask, LoopMask and
 * ContMask) which are controlled by the flow control instructions (namely:
 * (IF/ELSE/ENDIF, LOOP/ENDLOOP and CONT).
 *
 *
 * Authors:
 *   Michal Krol
 *   Brian Paul
 */

#include <transpose_matrix4x4.h>
#include <simdmath/ceilf4.h>
#include <simdmath/cosf4.h>
#include <simdmath/divf4.h>
#include <simdmath/floorf4.h>
#include <simdmath/log2f4.h>
#include <simdmath/powf4.h>
#include <simdmath/sinf4.h>
#include <simdmath/sqrtf4.h>
#include <simdmath/truncf4.h>

#include "pipe/p_compiler.h"
#include "pipe/p_state.h"
#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "spu_exec.h"
#include "spu_main.h"
#include "spu_vertex_shader.h"
#include "spu_dcache.h"
#include "cell/common.h"

#define TILE_TOP_LEFT     0
#define TILE_TOP_RIGHT    1
#define TILE_BOTTOM_LEFT  2
#define TILE_BOTTOM_RIGHT 3

/*
 * Shorthand locations of various utility registers (_I = Index, _C = Channel)
 */
#define TEMP_0_I           TGSI_EXEC_TEMP_00000000_I
#define TEMP_0_C           TGSI_EXEC_TEMP_00000000_C
#define TEMP_7F_I          TGSI_EXEC_TEMP_7FFFFFFF_I
#define TEMP_7F_C          TGSI_EXEC_TEMP_7FFFFFFF_C
#define TEMP_80_I          TGSI_EXEC_TEMP_80000000_I
#define TEMP_80_C          TGSI_EXEC_TEMP_80000000_C
#define TEMP_FF_I          TGSI_EXEC_TEMP_FFFFFFFF_I
#define TEMP_FF_C          TGSI_EXEC_TEMP_FFFFFFFF_C
#define TEMP_1_I           TGSI_EXEC_TEMP_ONE_I
#define TEMP_1_C           TGSI_EXEC_TEMP_ONE_C
#define TEMP_2_I           TGSI_EXEC_TEMP_TWO_I
#define TEMP_2_C           TGSI_EXEC_TEMP_TWO_C
#define TEMP_128_I         TGSI_EXEC_TEMP_128_I
#define TEMP_128_C         TGSI_EXEC_TEMP_128_C
#define TEMP_M128_I        TGSI_EXEC_TEMP_MINUS_128_I
#define TEMP_M128_C        TGSI_EXEC_TEMP_MINUS_128_C
#define TEMP_KILMASK_I     TGSI_EXEC_TEMP_KILMASK_I
#define TEMP_KILMASK_C     TGSI_EXEC_TEMP_KILMASK_C
#define TEMP_OUTPUT_I      TGSI_EXEC_TEMP_OUTPUT_I
#define TEMP_OUTPUT_C      TGSI_EXEC_TEMP_OUTPUT_C
#define TEMP_PRIMITIVE_I   TGSI_EXEC_TEMP_PRIMITIVE_I
#define TEMP_PRIMITIVE_C   TGSI_EXEC_TEMP_PRIMITIVE_C
#define TEMP_R0            TGSI_EXEC_TEMP_R0

#define FOR_EACH_CHANNEL(CHAN)\
   for (CHAN = 0; CHAN < 4; CHAN++)

#define IS_CHANNEL_ENABLED(INST, CHAN)\
   ((INST).Dst[0].Register.WriteMask & (1 << (CHAN)))

#define IS_CHANNEL_ENABLED2(INST, CHAN)\
   ((INST).Dst[1].Register.WriteMask & (1 << (CHAN)))

#define FOR_EACH_ENABLED_CHANNEL(INST, CHAN)\
   FOR_EACH_CHANNEL( CHAN )\
      if (IS_CHANNEL_ENABLED( INST, CHAN ))

#define FOR_EACH_ENABLED_CHANNEL2(INST, CHAN)\
   FOR_EACH_CHANNEL( CHAN )\
      if (IS_CHANNEL_ENABLED2( INST, CHAN ))


/** The execution mask depends on the conditional mask and the loop mask */
#define UPDATE_EXEC_MASK(MACH) \
      MACH->ExecMask = MACH->CondMask & MACH->LoopMask & MACH->ContMask & MACH->FuncMask


#define CHAN_X  0
#define CHAN_Y  1
#define CHAN_Z  2
#define CHAN_W  3



/**
 * Initialize machine state by expanding tokens to full instructions,
 * allocating temporary storage, setting up constants, etc.
 * After this, we can call spu_exec_machine_run() many times.
 */
void
spu_exec_machine_init(struct spu_exec_machine *mach,
                      uint numSamplers,
                      struct spu_sampler *samplers,
                      unsigned processor)
{
   const qword zero = si_il(0);
   const qword not_zero = si_il(~0);

   (void) numSamplers;
   mach->Samplers = samplers;
   mach->Processor = processor;
   mach->Addrs = &mach->Temps[TGSI_EXEC_NUM_TEMPS];

   /* Setup constants. */
   mach->Temps[TEMP_0_I].xyzw[TEMP_0_C].q = zero;
   mach->Temps[TEMP_FF_I].xyzw[TEMP_FF_C].q = not_zero;
   mach->Temps[TEMP_7F_I].xyzw[TEMP_7F_C].q = si_shli(not_zero, -1);
   mach->Temps[TEMP_80_I].xyzw[TEMP_80_C].q = si_shli(not_zero, 31);

   mach->Temps[TEMP_1_I].xyzw[TEMP_1_C].q = (qword) spu_splats(1.0f);
   mach->Temps[TEMP_2_I].xyzw[TEMP_2_C].q = (qword) spu_splats(2.0f);
   mach->Temps[TEMP_128_I].xyzw[TEMP_128_C].q = (qword) spu_splats(128.0f);
   mach->Temps[TEMP_M128_I].xyzw[TEMP_M128_C].q = (qword) spu_splats(-128.0f);
}


static INLINE qword
micro_abs(qword src)
{
   return si_rotmi(si_shli(src, 1), -1);
}

static INLINE qword
micro_ceil(qword src)
{
   return (qword) _ceilf4((vec_float4) src);
}

static INLINE qword
micro_cos(qword src)
{
   return (qword) _cosf4((vec_float4) src);
}

static const qword br_shuf = {
   TILE_BOTTOM_RIGHT + 0, TILE_BOTTOM_RIGHT + 1,
   TILE_BOTTOM_RIGHT + 2, TILE_BOTTOM_RIGHT + 3,
   TILE_BOTTOM_RIGHT + 0, TILE_BOTTOM_RIGHT + 1,
   TILE_BOTTOM_RIGHT + 2, TILE_BOTTOM_RIGHT + 3,
   TILE_BOTTOM_RIGHT + 0, TILE_BOTTOM_RIGHT + 1,
   TILE_BOTTOM_RIGHT + 2, TILE_BOTTOM_RIGHT + 3,
   TILE_BOTTOM_RIGHT + 0, TILE_BOTTOM_RIGHT + 1,
   TILE_BOTTOM_RIGHT + 2, TILE_BOTTOM_RIGHT + 3,
};

static const qword bl_shuf = {
   TILE_BOTTOM_LEFT + 0, TILE_BOTTOM_LEFT + 1,
   TILE_BOTTOM_LEFT + 2, TILE_BOTTOM_LEFT + 3,
   TILE_BOTTOM_LEFT + 0, TILE_BOTTOM_LEFT + 1,
   TILE_BOTTOM_LEFT + 2, TILE_BOTTOM_LEFT + 3,
   TILE_BOTTOM_LEFT + 0, TILE_BOTTOM_LEFT + 1,
   TILE_BOTTOM_LEFT + 2, TILE_BOTTOM_LEFT + 3,
   TILE_BOTTOM_LEFT + 0, TILE_BOTTOM_LEFT + 1,
   TILE_BOTTOM_LEFT + 2, TILE_BOTTOM_LEFT + 3,
};

static const qword tl_shuf = {
   TILE_TOP_LEFT + 0, TILE_TOP_LEFT + 1,
   TILE_TOP_LEFT + 2, TILE_TOP_LEFT + 3,
   TILE_TOP_LEFT + 0, TILE_TOP_LEFT + 1,
   TILE_TOP_LEFT + 2, TILE_TOP_LEFT + 3,
   TILE_TOP_LEFT + 0, TILE_TOP_LEFT + 1,
   TILE_TOP_LEFT + 2, TILE_TOP_LEFT + 3,
   TILE_TOP_LEFT + 0, TILE_TOP_LEFT + 1,
   TILE_TOP_LEFT + 2, TILE_TOP_LEFT + 3,
};

static qword
micro_ddx(qword src)
{
   qword bottom_right = si_shufb(src, src, br_shuf);
   qword bottom_left = si_shufb(src, src, bl_shuf);

   return si_fs(bottom_right, bottom_left);
}

static qword
micro_ddy(qword src)
{
   qword top_left = si_shufb(src, src, tl_shuf);
   qword bottom_left = si_shufb(src, src, bl_shuf);

   return si_fs(top_left, bottom_left);
}

static INLINE qword
micro_div(qword src0, qword src1)
{
   return (qword) _divf4((vec_float4) src0, (vec_float4) src1);
}

static qword
micro_flr(qword src)
{
   return (qword) _floorf4((vec_float4) src);
}

static qword
micro_frc(qword src)
{
   return si_fs(src, (qword) _floorf4((vec_float4) src));
}

static INLINE qword
micro_ge(qword src0, qword src1)
{
   return si_or(si_fceq(src0, src1), si_fcgt(src0, src1));
}

static qword
micro_lg2(qword src)
{
   return (qword) _log2f4((vec_float4) src);
}

static INLINE qword
micro_lt(qword src0, qword src1)
{
   const qword tmp = si_or(si_fceq(src0, src1), si_fcgt(src0, src1));

   return si_xori(tmp, 0xff);
}

static INLINE qword
micro_max(qword src0, qword src1)
{
   return si_selb(src1, src0, si_fcgt(src0, src1));
}

static INLINE qword
micro_min(qword src0, qword src1)
{
   return si_selb(src0, src1, si_fcgt(src0, src1));
}

static qword
micro_neg(qword src)
{
   return si_xor(src, (qword) spu_splats(0x80000000));
}

static qword
micro_set_sign(qword src)
{
   return si_or(src, (qword) spu_splats(0x80000000));
}

static qword
micro_pow(qword src0, qword src1)
{
   return (qword) _powf4((vec_float4) src0, (vec_float4) src1);
}

static qword
micro_rnd(qword src)
{
   const qword half = (qword) spu_splats(0.5f);

   /* May be able to use _roundf4.  There may be some difference, though.
    */
   return (qword) _floorf4((vec_float4) si_fa(src, half));
}

static INLINE qword
micro_ishr(qword src0, qword src1)
{
   return si_rotma(src0, si_sfi(src1, 0));
}

static qword
micro_trunc(qword src)
{
   return (qword) _truncf4((vec_float4) src);
}

static qword
micro_sin(qword src)
{
   return (qword) _sinf4((vec_float4) src);
}

static INLINE qword
micro_sqrt(qword src)
{
   return (qword) _sqrtf4((vec_float4) src);
}

static void
fetch_src_file_channel(
   const struct spu_exec_machine *mach,
   const uint file,
   const uint swizzle,
   const union spu_exec_channel *index,
   union spu_exec_channel *chan )
{
   switch( swizzle ) {
   case TGSI_SWIZZLE_X:
   case TGSI_SWIZZLE_Y:
   case TGSI_SWIZZLE_Z:
   case TGSI_SWIZZLE_W:
      switch( file ) {
      case TGSI_FILE_CONSTANT: {
         unsigned i;

         for (i = 0; i < 4; i++) {
            const float *ptr = mach->Consts[index->i[i]];
            float tmp[4];

            spu_dcache_fetch_unaligned((qword *) tmp,
                                       (uintptr_t)(ptr + swizzle),
                                       sizeof(float));

            chan->f[i] = tmp[0];
         }
         break;
      }

      case TGSI_FILE_INPUT:
         chan->u[0] = mach->Inputs[index->i[0]].xyzw[swizzle].u[0];
         chan->u[1] = mach->Inputs[index->i[1]].xyzw[swizzle].u[1];
         chan->u[2] = mach->Inputs[index->i[2]].xyzw[swizzle].u[2];
         chan->u[3] = mach->Inputs[index->i[3]].xyzw[swizzle].u[3];
         break;

      case TGSI_FILE_TEMPORARY:
         chan->u[0] = mach->Temps[index->i[0]].xyzw[swizzle].u[0];
         chan->u[1] = mach->Temps[index->i[1]].xyzw[swizzle].u[1];
         chan->u[2] = mach->Temps[index->i[2]].xyzw[swizzle].u[2];
         chan->u[3] = mach->Temps[index->i[3]].xyzw[swizzle].u[3];
         break;

      case TGSI_FILE_IMMEDIATE:
         ASSERT( index->i[0] < (int) mach->ImmLimit );
         ASSERT( index->i[1] < (int) mach->ImmLimit );
         ASSERT( index->i[2] < (int) mach->ImmLimit );
         ASSERT( index->i[3] < (int) mach->ImmLimit );

         chan->f[0] = mach->Imms[index->i[0]][swizzle];
         chan->f[1] = mach->Imms[index->i[1]][swizzle];
         chan->f[2] = mach->Imms[index->i[2]][swizzle];
         chan->f[3] = mach->Imms[index->i[3]][swizzle];
         break;

      case TGSI_FILE_ADDRESS:
         chan->u[0] = mach->Addrs[index->i[0]].xyzw[swizzle].u[0];
         chan->u[1] = mach->Addrs[index->i[1]].xyzw[swizzle].u[1];
         chan->u[2] = mach->Addrs[index->i[2]].xyzw[swizzle].u[2];
         chan->u[3] = mach->Addrs[index->i[3]].xyzw[swizzle].u[3];
         break;

      case TGSI_FILE_OUTPUT:
         /* vertex/fragment output vars can be read too */
         chan->u[0] = mach->Outputs[index->i[0]].xyzw[swizzle].u[0];
         chan->u[1] = mach->Outputs[index->i[1]].xyzw[swizzle].u[1];
         chan->u[2] = mach->Outputs[index->i[2]].xyzw[swizzle].u[2];
         chan->u[3] = mach->Outputs[index->i[3]].xyzw[swizzle].u[3];
         break;

      default:
         ASSERT( 0 );
      }
      break;

   default:
      ASSERT( 0 );
   }
}

static void
fetch_source(
   const struct spu_exec_machine *mach,
   union spu_exec_channel *chan,
   const struct tgsi_full_src_register *reg,
   const uint chan_index )
{
   union spu_exec_channel index;
   uint swizzle;

   index.i[0] =
   index.i[1] =
   index.i[2] =
   index.i[3] = reg->Register.Index;

   if (reg->Register.Indirect) {
      union spu_exec_channel index2;
      union spu_exec_channel indir_index;

      index2.i[0] =
      index2.i[1] =
      index2.i[2] =
      index2.i[3] = reg->Indirect.Index;

      swizzle = tgsi_util_get_src_register_swizzle(&reg->Indirect,
                                                   CHAN_X);
      fetch_src_file_channel(
         mach,
         reg->Indirect.File,
         swizzle,
         &index2,
         &indir_index );

      index.q = si_a(index.q, indir_index.q);
   }

   if( reg->Register.Dimension ) {
      switch( reg->Register.File ) {
      case TGSI_FILE_INPUT:
         index.q = si_mpyi(index.q, 17);
         break;
      case TGSI_FILE_CONSTANT:
         index.q = si_shli(index.q, 12);
         break;
      default:
         ASSERT( 0 );
      }

      index.i[0] += reg->Dimension.Index;
      index.i[1] += reg->Dimension.Index;
      index.i[2] += reg->Dimension.Index;
      index.i[3] += reg->Dimension.Index;

      if (reg->Dimension.Indirect) {
         union spu_exec_channel index2;
         union spu_exec_channel indir_index;

         index2.i[0] =
         index2.i[1] =
         index2.i[2] =
         index2.i[3] = reg->DimIndirect.Index;

         swizzle = tgsi_util_get_src_register_swizzle( &reg->DimIndirect, CHAN_X );
         fetch_src_file_channel(
            mach,
            reg->DimIndirect.File,
            swizzle,
            &index2,
            &indir_index );

         index.q = si_a(index.q, indir_index.q);
      }
   }

   swizzle = tgsi_util_get_full_src_register_swizzle( reg, chan_index );
   fetch_src_file_channel(
      mach,
      reg->Register.File,
      swizzle,
      &index,
      chan );

   switch (tgsi_util_get_full_src_register_sign_mode( reg, chan_index )) {
   case TGSI_UTIL_SIGN_CLEAR:
      chan->q = micro_abs(chan->q);
      break;

   case TGSI_UTIL_SIGN_SET:
      chan->q = micro_set_sign(chan->q);
      break;

   case TGSI_UTIL_SIGN_TOGGLE:
      chan->q = micro_neg(chan->q);
      break;

   case TGSI_UTIL_SIGN_KEEP:
      break;
   }

   if (reg->RegisterExtMod.Complement) {
      chan->q = si_fs(mach->Temps[TEMP_1_I].xyzw[TEMP_1_C].q, chan->q);
   }
}

static void
store_dest(
   struct spu_exec_machine *mach,
   const union spu_exec_channel *chan,
   const struct tgsi_full_dst_register *reg,
   const struct tgsi_full_instruction *inst,
   uint chan_index )
{
   union spu_exec_channel *dst;

   switch( reg->Register.File ) {
   case TGSI_FILE_NULL:
      return;

   case TGSI_FILE_OUTPUT:
      dst = &mach->Outputs[mach->Temps[TEMP_OUTPUT_I].xyzw[TEMP_OUTPUT_C].u[0]
                           + reg->Register.Index].xyzw[chan_index];
      break;

   case TGSI_FILE_TEMPORARY:
      dst = &mach->Temps[reg->Register.Index].xyzw[chan_index];
      break;

   case TGSI_FILE_ADDRESS:
      dst = &mach->Addrs[reg->Register.Index].xyzw[chan_index];
      break;

   default:
      ASSERT( 0 );
      return;
   }

   switch (inst->Instruction.Saturate)
   {
   case TGSI_SAT_NONE:
      if (mach->ExecMask & 0x1)
         dst->i[0] = chan->i[0];
      if (mach->ExecMask & 0x2)
         dst->i[1] = chan->i[1];
      if (mach->ExecMask & 0x4)
         dst->i[2] = chan->i[2];
      if (mach->ExecMask & 0x8)
         dst->i[3] = chan->i[3];
      break;

   case TGSI_SAT_ZERO_ONE:
      /* XXX need to obey ExecMask here */
      dst->q = micro_max(chan->q, mach->Temps[TEMP_0_I].xyzw[TEMP_0_C].q);
      dst->q = micro_min(dst->q, mach->Temps[TEMP_1_I].xyzw[TEMP_1_C].q);
      break;

   case TGSI_SAT_MINUS_PLUS_ONE:
      ASSERT( 0 );
      break;

   default:
      ASSERT( 0 );
   }
}

#define FETCH(VAL,INDEX,CHAN)\
    fetch_source (mach, VAL, &inst->Src[INDEX], CHAN)

#define STORE(VAL,INDEX,CHAN)\
    store_dest (mach, VAL, &inst->Dst[INDEX], inst, CHAN )


/**
 * Execute ARB-style KIL which is predicated by a src register.
 * Kill fragment if any of the four values is less than zero.
 */
static void
exec_kil(struct spu_exec_machine *mach,
         const struct tgsi_full_instruction *inst)
{
   uint uniquemask;
   uint chan_index;
   uint kilmask = 0; /* bit 0 = pixel 0, bit 1 = pixel 1, etc */
   union spu_exec_channel r[1];

   /* This mask stores component bits that were already tested. */
   uniquemask = 0;

   for (chan_index = 0; chan_index < 4; chan_index++)
   {
      uint swizzle;
      uint i;

      /* unswizzle channel */
      swizzle = tgsi_util_get_full_src_register_swizzle (
                        &inst->Src[0],
                        chan_index);

      /* check if the component has not been already tested */
      if (uniquemask & (1 << swizzle))
         continue;
      uniquemask |= 1 << swizzle;

      FETCH(&r[0], 0, chan_index);
      for (i = 0; i < 4; i++)
         if (r[0].f[i] < 0.0f)
            kilmask |= 1 << i;
   }

   mach->Temps[TEMP_KILMASK_I].xyzw[TEMP_KILMASK_C].u[0] |= kilmask;
}

/**
 * Execute NVIDIA-style KIL which is predicated by a condition code.
 * Kill fragment if the condition code is TRUE.
 */
static void
exec_kilp(struct tgsi_exec_machine *mach,
          const struct tgsi_full_instruction *inst)
{
   uint kilmask = 0; /* bit 0 = pixel 0, bit 1 = pixel 1, etc */

   /* TODO: build kilmask from CC mask */

   mach->Temps[TEMP_KILMASK_I].xyzw[TEMP_KILMASK_C].u[0] |= kilmask;
}

/*
 * Fetch a texel using STR texture coordinates.
 */
static void
fetch_texel( struct spu_sampler *sampler,
             const union spu_exec_channel *s,
             const union spu_exec_channel *t,
             const union spu_exec_channel *p,
             float lodbias,  /* XXX should be float[4] */
             union spu_exec_channel *r,
             union spu_exec_channel *g,
             union spu_exec_channel *b,
             union spu_exec_channel *a )
{
   qword rgba[4];
   qword out[4];

   sampler->get_samples(sampler, s->f, t->f, p->f, lodbias, 
			(float (*)[4]) rgba);

   _transpose_matrix4x4((vec_float4 *) out, (vec_float4 *) rgba);
   r->q = out[0];
   g->q = out[1];
   b->q = out[2];
   a->q = out[3];
}


static void
exec_tex(struct spu_exec_machine *mach,
         const struct tgsi_full_instruction *inst,
         boolean biasLod, boolean projected)
{
   const uint unit = inst->Src[1].Register.Index;
   union spu_exec_channel r[8];
   uint chan_index;
   float lodBias;

   /*   printf("Sampler %u unit %u\n", sampler, unit); */

   switch (inst->InstructionExtTexture.Texture) {
   case TGSI_TEXTURE_1D:

      FETCH(&r[0], 0, CHAN_X);

      if (projected) {
         FETCH(&r[1], 0, CHAN_W);
         r[0].q = micro_div(r[0].q, r[1].q);
      }

      if (biasLod) {
         FETCH(&r[1], 0, CHAN_W);
         lodBias = r[2].f[0];
      }
      else
         lodBias = 0.0;

      fetch_texel(&mach->Samplers[unit],
                  &r[0], NULL, NULL, lodBias,  /* S, T, P, BIAS */
                  &r[0], &r[1], &r[2], &r[3]); /* R, G, B, A */
      break;

   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:

      FETCH(&r[0], 0, CHAN_X);
      FETCH(&r[1], 0, CHAN_Y);
      FETCH(&r[2], 0, CHAN_Z);

      if (projected) {
         FETCH(&r[3], 0, CHAN_W);
         r[0].q = micro_div(r[0].q, r[3].q);
         r[1].q = micro_div(r[1].q, r[3].q);
         r[2].q = micro_div(r[2].q, r[3].q);
      }

      if (biasLod) {
         FETCH(&r[3], 0, CHAN_W);
         lodBias = r[3].f[0];
      }
      else
         lodBias = 0.0;

      fetch_texel(&mach->Samplers[unit],
                  &r[0], &r[1], &r[2], lodBias,  /* inputs */
                  &r[0], &r[1], &r[2], &r[3]);  /* outputs */
      break;

   case TGSI_TEXTURE_3D:
   case TGSI_TEXTURE_CUBE:

      FETCH(&r[0], 0, CHAN_X);
      FETCH(&r[1], 0, CHAN_Y);
      FETCH(&r[2], 0, CHAN_Z);

      if (projected) {
         FETCH(&r[3], 0, CHAN_W);
         r[0].q = micro_div(r[0].q, r[3].q);
         r[1].q = micro_div(r[1].q, r[3].q);
         r[2].q = micro_div(r[2].q, r[3].q);
      }

      if (biasLod) {
         FETCH(&r[3], 0, CHAN_W);
         lodBias = r[3].f[0];
      }
      else
         lodBias = 0.0;

      fetch_texel(&mach->Samplers[unit],
                  &r[0], &r[1], &r[2], lodBias,
                  &r[0], &r[1], &r[2], &r[3]);
      break;

   default:
      ASSERT (0);
   }

   FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
      STORE( &r[chan_index], 0, chan_index );
   }
}



static void
constant_interpolation(
   struct spu_exec_machine *mach,
   unsigned attrib,
   unsigned chan )
{
   unsigned i;

   for( i = 0; i < QUAD_SIZE; i++ ) {
      mach->Inputs[attrib].xyzw[chan].f[i] = mach->InterpCoefs[attrib].a0[chan];
   }
}

static void
linear_interpolation(
   struct spu_exec_machine *mach,
   unsigned attrib,
   unsigned chan )
{
   const float x = mach->QuadPos.xyzw[0].f[0];
   const float y = mach->QuadPos.xyzw[1].f[0];
   const float dadx = mach->InterpCoefs[attrib].dadx[chan];
   const float dady = mach->InterpCoefs[attrib].dady[chan];
   const float a0 = mach->InterpCoefs[attrib].a0[chan] + dadx * x + dady * y;
   mach->Inputs[attrib].xyzw[chan].f[0] = a0;
   mach->Inputs[attrib].xyzw[chan].f[1] = a0 + dadx;
   mach->Inputs[attrib].xyzw[chan].f[2] = a0 + dady;
   mach->Inputs[attrib].xyzw[chan].f[3] = a0 + dadx + dady;
}

static void
perspective_interpolation(
   struct spu_exec_machine *mach,
   unsigned attrib,
   unsigned chan )
{
   const float x = mach->QuadPos.xyzw[0].f[0];
   const float y = mach->QuadPos.xyzw[1].f[0];
   const float dadx = mach->InterpCoefs[attrib].dadx[chan];
   const float dady = mach->InterpCoefs[attrib].dady[chan];
   const float a0 = mach->InterpCoefs[attrib].a0[chan] + dadx * x + dady * y;
   const float *w = mach->QuadPos.xyzw[3].f;
   /* divide by W here */
   mach->Inputs[attrib].xyzw[chan].f[0] = a0 / w[0];
   mach->Inputs[attrib].xyzw[chan].f[1] = (a0 + dadx) / w[1];
   mach->Inputs[attrib].xyzw[chan].f[2] = (a0 + dady) / w[2];
   mach->Inputs[attrib].xyzw[chan].f[3] = (a0 + dadx + dady) / w[3];
}


typedef void (* interpolation_func)(
   struct spu_exec_machine *mach,
   unsigned attrib,
   unsigned chan );

static void
exec_declaration(struct spu_exec_machine *mach,
                 const struct tgsi_full_declaration *decl)
{
   if( mach->Processor == TGSI_PROCESSOR_FRAGMENT ) {
      if( decl->Declaration.File == TGSI_FILE_INPUT ) {
         unsigned first, last, mask;
         interpolation_func interp;

         first = decl->Range.First;
         last = decl->Range.Last;
         mask = decl->Declaration.UsageMask;

         switch( decl->Declaration.Interpolate ) {
         case TGSI_INTERPOLATE_CONSTANT:
            interp = constant_interpolation;
            break;

         case TGSI_INTERPOLATE_LINEAR:
            interp = linear_interpolation;
            break;

         case TGSI_INTERPOLATE_PERSPECTIVE:
            interp = perspective_interpolation;
            break;

         default:
            ASSERT( 0 );
         }

         if( mask == TGSI_WRITEMASK_XYZW ) {
            unsigned i, j;

            for( i = first; i <= last; i++ ) {
               for( j = 0; j < NUM_CHANNELS; j++ ) {
                  interp( mach, i, j );
               }
            }
         }
         else {
            unsigned i, j;

            for( j = 0; j < NUM_CHANNELS; j++ ) {
               if( mask & (1 << j) ) {
                  for( i = first; i <= last; i++ ) {
                     interp( mach, i, j );
                  }
               }
            }
         }
      }
   }
}

static void
exec_instruction(
   struct spu_exec_machine *mach,
   const struct tgsi_full_instruction *inst,
   int *pc )
{
   uint chan_index;
   union spu_exec_channel r[8];

   (*pc)++;

   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_ARL:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
	 FETCH( &r[0], 0, chan_index );
         r[0].q = si_cflts(r[0].q, 0);
	 STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_MOV:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_LIT:
      if (IS_CHANNEL_ENABLED( *inst, CHAN_X )) {
	 STORE( &mach->Temps[TEMP_1_I].xyzw[TEMP_1_C], 0, CHAN_X );
      }

      if (IS_CHANNEL_ENABLED( *inst, CHAN_Y ) || IS_CHANNEL_ENABLED( *inst, CHAN_Z )) {
	 FETCH( &r[0], 0, CHAN_X );
         if (IS_CHANNEL_ENABLED( *inst, CHAN_Y )) {
            r[0].q = micro_max(r[0].q, mach->Temps[TEMP_0_I].xyzw[TEMP_0_C].q);
	    STORE( &r[0], 0, CHAN_Y );
	 }

         if (IS_CHANNEL_ENABLED( *inst, CHAN_Z )) {
            FETCH( &r[1], 0, CHAN_Y );
            r[1].q = micro_max(r[1].q, mach->Temps[TEMP_0_I].xyzw[TEMP_0_C].q);

            FETCH( &r[2], 0, CHAN_W );
            r[2].q = micro_min(r[2].q, mach->Temps[TEMP_128_I].xyzw[TEMP_128_C].q);
            r[2].q = micro_max(r[2].q, mach->Temps[TEMP_M128_I].xyzw[TEMP_M128_C].q);
            r[1].q = micro_pow(r[1].q, r[2].q);

            /* r0 = (r0 > 0.0) ? r1 : 0.0
             */
            r[0].q = si_fcgt(r[0].q, mach->Temps[TEMP_0_I].xyzw[TEMP_0_C].q);
            r[0].q = si_selb(mach->Temps[TEMP_0_I].xyzw[TEMP_0_C].q, r[1].q,
                             r[0].q);
            STORE( &r[0], 0, CHAN_Z );
         }
      }

      if (IS_CHANNEL_ENABLED( *inst, CHAN_W )) {
	 STORE( &mach->Temps[TEMP_1_I].xyzw[TEMP_1_C], 0, CHAN_W );
      }
      break;

   case TGSI_OPCODE_RCP:
      FETCH( &r[0], 0, CHAN_X );
      r[0].q = micro_div(mach->Temps[TEMP_1_I].xyzw[TEMP_1_C].q, r[0].q);
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
	 STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_RSQ:
      FETCH( &r[0], 0, CHAN_X );
      r[0].q = micro_sqrt(r[0].q);
      r[0].q = micro_div(mach->Temps[TEMP_1_I].xyzw[TEMP_1_C].q, r[0].q);
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
	 STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_EXP:
      ASSERT (0);
      break;

   case TGSI_OPCODE_LOG:
      ASSERT (0);
      break;

   case TGSI_OPCODE_MUL:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index )
      {
         FETCH(&r[0], 0, chan_index);
         FETCH(&r[1], 1, chan_index);

         r[0].q = si_fm(r[0].q, r[1].q);

         STORE(&r[0], 0, chan_index);
      }
      break;

   case TGSI_OPCODE_ADD:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );
         r[0].q = si_fa(r[0].q, r[1].q);
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_DP3:
   /* TGSI_OPCODE_DOT3 */
      FETCH( &r[0], 0, CHAN_X );
      FETCH( &r[1], 1, CHAN_X );
      r[0].q = si_fm(r[0].q, r[1].q);

      FETCH( &r[1], 0, CHAN_Y );
      FETCH( &r[2], 1, CHAN_Y );
      r[0].q = si_fma(r[1].q, r[2].q, r[0].q);


      FETCH( &r[1], 0, CHAN_Z );
      FETCH( &r[2], 1, CHAN_Z );
      r[0].q = si_fma(r[1].q, r[2].q, r[0].q);

      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         STORE( &r[0], 0, chan_index );
      }
      break;

    case TGSI_OPCODE_DP4:
    /* TGSI_OPCODE_DOT4 */
       FETCH(&r[0], 0, CHAN_X);
       FETCH(&r[1], 1, CHAN_X);

      r[0].q = si_fm(r[0].q, r[1].q);

       FETCH(&r[1], 0, CHAN_Y);
       FETCH(&r[2], 1, CHAN_Y);

      r[0].q = si_fma(r[1].q, r[2].q, r[0].q);

       FETCH(&r[1], 0, CHAN_Z);
       FETCH(&r[2], 1, CHAN_Z);

      r[0].q = si_fma(r[1].q, r[2].q, r[0].q);

       FETCH(&r[1], 0, CHAN_W);
       FETCH(&r[2], 1, CHAN_W);

      r[0].q = si_fma(r[1].q, r[2].q, r[0].q);

      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
	 STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_DST:
      if (IS_CHANNEL_ENABLED( *inst, CHAN_X )) {
	 STORE( &mach->Temps[TEMP_1_I].xyzw[TEMP_1_C], 0, CHAN_X );
      }

      if (IS_CHANNEL_ENABLED( *inst, CHAN_Y )) {
	 FETCH( &r[0], 0, CHAN_Y );
	 FETCH( &r[1], 1, CHAN_Y);
      r[0].q = si_fm(r[0].q, r[1].q);
	 STORE( &r[0], 0, CHAN_Y );
      }

      if (IS_CHANNEL_ENABLED( *inst, CHAN_Z )) {
	 FETCH( &r[0], 0, CHAN_Z );
	 STORE( &r[0], 0, CHAN_Z );
      }

      if (IS_CHANNEL_ENABLED( *inst, CHAN_W )) {
	 FETCH( &r[0], 1, CHAN_W );
	 STORE( &r[0], 0, CHAN_W );
      }
      break;

   case TGSI_OPCODE_MIN:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH(&r[0], 0, chan_index);
         FETCH(&r[1], 1, chan_index);

         r[0].q = micro_min(r[0].q, r[1].q);

         STORE(&r[0], 0, chan_index);
      }
      break;

   case TGSI_OPCODE_MAX:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH(&r[0], 0, chan_index);
         FETCH(&r[1], 1, chan_index);

         r[0].q = micro_max(r[0].q, r[1].q);

         STORE(&r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_SLT:
   /* TGSI_OPCODE_SETLT */
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );

         r[0].q = micro_ge(r[0].q, r[1].q);
         r[0].q = si_xori(r[0].q, 0xff);

         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_SGE:
   /* TGSI_OPCODE_SETGE */
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );
         r[0].q = micro_ge(r[0].q, r[1].q);
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_MAD:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );
         FETCH( &r[2], 2, chan_index );
         r[0].q = si_fma(r[0].q, r[1].q, r[2].q);
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_SUB:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH(&r[0], 0, chan_index);
         FETCH(&r[1], 1, chan_index);

         r[0].q = si_fs(r[0].q, r[1].q);

         STORE(&r[0], 0, chan_index);
      }
      break;

   case TGSI_OPCODE_LRP:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH(&r[0], 0, chan_index);
         FETCH(&r[1], 1, chan_index);
         FETCH(&r[2], 2, chan_index);

         r[1].q = si_fs(r[1].q, r[2].q);
         r[0].q = si_fma(r[0].q, r[1].q, r[2].q);

         STORE(&r[0], 0, chan_index);
      }
      break;

   case TGSI_OPCODE_CND:
      ASSERT (0);
      break;

   case TGSI_OPCODE_DP2A:
      ASSERT (0);
      break;

   case TGSI_OPCODE_FRC:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         r[0].q = micro_frc(r[0].q);
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_CLAMP:
      ASSERT (0);
      break;

   case TGSI_OPCODE_FLR:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         r[0].q = micro_flr(r[0].q);
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_ROUND:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         r[0].q = micro_rnd(r[0].q);
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_EX2:
      FETCH(&r[0], 0, CHAN_X);

      r[0].q = micro_pow(mach->Temps[TEMP_2_I].xyzw[TEMP_2_C].q, r[0].q);

      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
	 STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_LG2:
      FETCH( &r[0], 0, CHAN_X );
      r[0].q = micro_lg2(r[0].q);
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_POW:
      FETCH(&r[0], 0, CHAN_X);
      FETCH(&r[1], 1, CHAN_X);

      r[0].q = micro_pow(r[0].q, r[1].q);

      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
	 STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_XPD:
      /* TGSI_OPCODE_XPD */
      FETCH(&r[0], 0, CHAN_Y);
      FETCH(&r[1], 1, CHAN_Z);
      FETCH(&r[3], 0, CHAN_Z);
      FETCH(&r[4], 1, CHAN_Y);

      /* r2 = (r0 * r1) - (r3 * r5)
       */
      r[2].q = si_fm(r[3].q, r[5].q);
      r[2].q = si_fms(r[0].q, r[1].q, r[2].q);

      if (IS_CHANNEL_ENABLED( *inst, CHAN_X )) {
         STORE( &r[2], 0, CHAN_X );
      }

      FETCH(&r[2], 1, CHAN_X);
      FETCH(&r[5], 0, CHAN_X);

      /* r3 = (r3 * r2) - (r1 * r5)
       */
      r[1].q = si_fm(r[1].q, r[5].q);
      r[3].q = si_fms(r[3].q, r[2].q, r[1].q);

      if (IS_CHANNEL_ENABLED( *inst, CHAN_Y )) {
         STORE( &r[3], 0, CHAN_Y );
      }

      /* r5 = (r5 * r4) - (r0 * r2)
       */
      r[0].q = si_fm(r[0].q, r[2].q);
      r[5].q = si_fms(r[5].q, r[4].q, r[0].q);

      if (IS_CHANNEL_ENABLED( *inst, CHAN_Z )) {
         STORE( &r[5], 0, CHAN_Z );
      }

      if (IS_CHANNEL_ENABLED( *inst, CHAN_W )) {
         STORE( &mach->Temps[TEMP_1_I].xyzw[TEMP_1_C], 0, CHAN_W );
      }
      break;

    case TGSI_OPCODE_ABS:
       FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
          FETCH(&r[0], 0, chan_index);

          r[0].q = micro_abs(r[0].q);

          STORE(&r[0], 0, chan_index);
       }
       break;

   case TGSI_OPCODE_RCC:
      ASSERT (0);
      break;

   case TGSI_OPCODE_DPH:
      FETCH(&r[0], 0, CHAN_X);
      FETCH(&r[1], 1, CHAN_X);

      r[0].q = si_fm(r[0].q, r[1].q);

      FETCH(&r[1], 0, CHAN_Y);
      FETCH(&r[2], 1, CHAN_Y);

      r[0].q = si_fma(r[1].q, r[2].q, r[0].q);

      FETCH(&r[1], 0, CHAN_Z);
      FETCH(&r[2], 1, CHAN_Z);

      r[0].q = si_fma(r[1].q, r[2].q, r[0].q);

      FETCH(&r[1], 1, CHAN_W);

      r[0].q = si_fa(r[0].q, r[1].q);

      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
	 STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_COS:
      FETCH(&r[0], 0, CHAN_X);

      r[0].q = micro_cos(r[0].q);

      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
	 STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_DDX:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         r[0].q = micro_ddx(r[0].q);
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_DDY:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         r[0].q = micro_ddy(r[0].q);
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_KILP:
      exec_kilp (mach, inst);
      break;

   case TGSI_OPCODE_KIL:
      exec_kil (mach, inst);
      break;

   case TGSI_OPCODE_PK2H:
      ASSERT (0);
      break;

   case TGSI_OPCODE_PK2US:
      ASSERT (0);
      break;

   case TGSI_OPCODE_PK4B:
      ASSERT (0);
      break;

   case TGSI_OPCODE_PK4UB:
      ASSERT (0);
      break;

   case TGSI_OPCODE_RFL:
      ASSERT (0);
      break;

   case TGSI_OPCODE_SEQ:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );

         r[0].q = si_fceq(r[0].q, r[1].q);

         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_SFL:
      ASSERT (0);
      break;

   case TGSI_OPCODE_SGT:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );
         r[0].q = si_fcgt(r[0].q, r[1].q);
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_SIN:
      FETCH( &r[0], 0, CHAN_X );
      r[0].q = micro_sin(r[0].q);
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_SLE:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );

         r[0].q = si_fcgt(r[0].q, r[1].q);
         r[0].q = si_xori(r[0].q, 0xff);

         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_SNE:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );

         r[0].q = si_fceq(r[0].q, r[1].q);
         r[0].q = si_xori(r[0].q, 0xff);

         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_STR:
      ASSERT (0);
      break;

   case TGSI_OPCODE_TEX:
      /* simple texture lookup */
      /* src[0] = texcoord */
      /* src[1] = sampler unit */
      exec_tex(mach, inst, FALSE, FALSE);
      break;

   case TGSI_OPCODE_TXB:
      /* Texture lookup with lod bias */
      /* src[0] = texcoord (src[0].w = load bias) */
      /* src[1] = sampler unit */
      exec_tex(mach, inst, TRUE, FALSE);
      break;

   case TGSI_OPCODE_TXD:
      /* Texture lookup with explict partial derivatives */
      /* src[0] = texcoord */
      /* src[1] = d[strq]/dx */
      /* src[2] = d[strq]/dy */
      /* src[3] = sampler unit */
      ASSERT (0);
      break;

   case TGSI_OPCODE_TXL:
      /* Texture lookup with explit LOD */
      /* src[0] = texcoord (src[0].w = load bias) */
      /* src[1] = sampler unit */
      exec_tex(mach, inst, TRUE, FALSE);
      break;

   case TGSI_OPCODE_TXP:
      /* Texture lookup with projection */
      /* src[0] = texcoord (src[0].w = projection) */
      /* src[1] = sampler unit */
      exec_tex(mach, inst, TRUE, TRUE);
      break;

   case TGSI_OPCODE_UP2H:
      ASSERT (0);
      break;

   case TGSI_OPCODE_UP2US:
      ASSERT (0);
      break;

   case TGSI_OPCODE_UP4B:
      ASSERT (0);
      break;

   case TGSI_OPCODE_UP4UB:
      ASSERT (0);
      break;

   case TGSI_OPCODE_X2D:
      ASSERT (0);
      break;

   case TGSI_OPCODE_ARA:
      ASSERT (0);
      break;

   case TGSI_OPCODE_ARR:
      ASSERT (0);
      break;

   case TGSI_OPCODE_BRA:
      ASSERT (0);
      break;

   case TGSI_OPCODE_CAL:
      /* skip the call if no execution channels are enabled */
      if (mach->ExecMask) {
         /* do the call */

         /* push the Cond, Loop, Cont stacks */
         ASSERT(mach->CondStackTop < TGSI_EXEC_MAX_COND_NESTING);
         mach->CondStack[mach->CondStackTop++] = mach->CondMask;
         ASSERT(mach->LoopStackTop < TGSI_EXEC_MAX_LOOP_NESTING);
         mach->LoopStack[mach->LoopStackTop++] = mach->LoopMask;
         ASSERT(mach->ContStackTop < TGSI_EXEC_MAX_LOOP_NESTING);
         mach->ContStack[mach->ContStackTop++] = mach->ContMask;

         ASSERT(mach->FuncStackTop < TGSI_EXEC_MAX_CALL_NESTING);
         mach->FuncStack[mach->FuncStackTop++] = mach->FuncMask;

         /* note that PC was already incremented above */
         mach->CallStack[mach->CallStackTop++] = *pc;
         *pc = inst->InstructionExtLabel.Label;
      }
      break;

   case TGSI_OPCODE_RET:
      mach->FuncMask &= ~mach->ExecMask;
      UPDATE_EXEC_MASK(mach);

      if (mach->ExecMask == 0x0) {
         /* really return now (otherwise, keep executing */

         if (mach->CallStackTop == 0) {
            /* returning from main() */
            *pc = -1;
            return;
         }
         *pc = mach->CallStack[--mach->CallStackTop];

         /* pop the Cond, Loop, Cont stacks */
         ASSERT(mach->CondStackTop > 0);
         mach->CondMask = mach->CondStack[--mach->CondStackTop];
         ASSERT(mach->LoopStackTop > 0);
         mach->LoopMask = mach->LoopStack[--mach->LoopStackTop];
         ASSERT(mach->ContStackTop > 0);
         mach->ContMask = mach->ContStack[--mach->ContStackTop];
         ASSERT(mach->FuncStackTop > 0);
         mach->FuncMask = mach->FuncStack[--mach->FuncStackTop];

         UPDATE_EXEC_MASK(mach);
      }
      break;

   case TGSI_OPCODE_SSG:
      ASSERT (0);
      break;

   case TGSI_OPCODE_CMP:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH(&r[0], 0, chan_index);
         FETCH(&r[1], 1, chan_index);
         FETCH(&r[2], 2, chan_index);

         /* r0 = (r0 < 0.0) ? r1 : r2
          */
         r[3].q = si_xor(r[3].q, r[3].q);
         r[0].q = micro_lt(r[0].q, r[3].q);
         r[0].q = si_selb(r[1].q, r[2].q, r[0].q);

         STORE(&r[0], 0, chan_index);
      }
      break;

   case TGSI_OPCODE_SCS:
      if( IS_CHANNEL_ENABLED( *inst, CHAN_X ) || IS_CHANNEL_ENABLED( *inst, CHAN_Y ) ) {
         FETCH( &r[0], 0, CHAN_X );
      }
      if( IS_CHANNEL_ENABLED( *inst, CHAN_X ) ) {
         r[1].q = micro_cos(r[0].q);
         STORE( &r[1], 0, CHAN_X );
      }
      if( IS_CHANNEL_ENABLED( *inst, CHAN_Y ) ) {
         r[1].q = micro_sin(r[0].q);
         STORE( &r[1], 0, CHAN_Y );
      }
      if( IS_CHANNEL_ENABLED( *inst, CHAN_Z ) ) {
         STORE( &mach->Temps[TEMP_0_I].xyzw[TEMP_0_C], 0, CHAN_Z );
      }
      if( IS_CHANNEL_ENABLED( *inst, CHAN_W ) ) {
         STORE( &mach->Temps[TEMP_1_I].xyzw[TEMP_1_C], 0, CHAN_W );
      }
      break;

   case TGSI_OPCODE_NRM:
      ASSERT (0);
      break;

   case TGSI_OPCODE_DIV:
      ASSERT( 0 );
      break;

   case TGSI_OPCODE_DP2:
      FETCH( &r[0], 0, CHAN_X );
      FETCH( &r[1], 1, CHAN_X );
      r[0].q = si_fm(r[0].q, r[1].q);

      FETCH( &r[1], 0, CHAN_Y );
      FETCH( &r[2], 1, CHAN_Y );
      r[0].q = si_fma(r[1].q, r[2].q, r[0].q);

      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_IF:
      /* push CondMask */
      ASSERT(mach->CondStackTop < TGSI_EXEC_MAX_COND_NESTING);
      mach->CondStack[mach->CondStackTop++] = mach->CondMask;
      FETCH( &r[0], 0, CHAN_X );
      /* update CondMask */
      if( ! r[0].u[0] ) {
         mach->CondMask &= ~0x1;
      }
      if( ! r[0].u[1] ) {
         mach->CondMask &= ~0x2;
      }
      if( ! r[0].u[2] ) {
         mach->CondMask &= ~0x4;
      }
      if( ! r[0].u[3] ) {
         mach->CondMask &= ~0x8;
      }
      UPDATE_EXEC_MASK(mach);
      /* Todo: If CondMask==0, jump to ELSE */
      break;

   case TGSI_OPCODE_ELSE:
      /* invert CondMask wrt previous mask */
      {
         uint prevMask;
         ASSERT(mach->CondStackTop > 0);
         prevMask = mach->CondStack[mach->CondStackTop - 1];
         mach->CondMask = ~mach->CondMask & prevMask;
         UPDATE_EXEC_MASK(mach);
         /* Todo: If CondMask==0, jump to ENDIF */
      }
      break;

   case TGSI_OPCODE_ENDIF:
      /* pop CondMask */
      ASSERT(mach->CondStackTop > 0);
      mach->CondMask = mach->CondStack[--mach->CondStackTop];
      UPDATE_EXEC_MASK(mach);
      break;

   case TGSI_OPCODE_END:
      /* halt execution */
      *pc = -1;
      break;

   case TGSI_OPCODE_REP:
      ASSERT (0);
      break;

   case TGSI_OPCODE_ENDREP:
       ASSERT (0);
       break;

   case TGSI_OPCODE_PUSHA:
      ASSERT (0);
      break;

   case TGSI_OPCODE_POPA:
      ASSERT (0);
      break;

   case TGSI_OPCODE_CEIL:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         r[0].q = micro_ceil(r[0].q);
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_I2F:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         r[0].q = si_csflt(r[0].q, 0);
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_NOT:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         r[0].q = si_xorbi(r[0].q, 0xff);
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_TRUNC:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         r[0].q = micro_trunc(r[0].q);
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_SHL:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );

         r[0].q = si_shl(r[0].q, r[1].q);

         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_ISHR:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );
         r[0].q = micro_ishr(r[0].q, r[1].q);
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_AND:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );
         r[0].q = si_and(r[0].q, r[1].q);
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_OR:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );
         r[0].q = si_or(r[0].q, r[1].q);
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_MOD:
      ASSERT (0);
      break;

   case TGSI_OPCODE_XOR:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );
         r[0].q = si_xor(r[0].q, r[1].q);
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_SAD:
      ASSERT (0);
      break;

   case TGSI_OPCODE_TXF:
      ASSERT (0);
      break;

   case TGSI_OPCODE_TXQ:
      ASSERT (0);
      break;

   case TGSI_OPCODE_EMIT:
      mach->Temps[TEMP_OUTPUT_I].xyzw[TEMP_OUTPUT_C].u[0] += 16;
      mach->Primitives[mach->Temps[TEMP_PRIMITIVE_I].xyzw[TEMP_PRIMITIVE_C].u[0]]++;
      break;

   case TGSI_OPCODE_ENDPRIM:
      mach->Temps[TEMP_PRIMITIVE_I].xyzw[TEMP_PRIMITIVE_C].u[0]++;
      mach->Primitives[mach->Temps[TEMP_PRIMITIVE_I].xyzw[TEMP_PRIMITIVE_C].u[0]] = 0;
      break;

   case TGSI_OPCODE_BGNFOR:
      /* fall-through (for now) */
   case TGSI_OPCODE_BGNLOOP:
      /* push LoopMask and ContMasks */
      ASSERT(mach->LoopStackTop < TGSI_EXEC_MAX_LOOP_NESTING);
      mach->LoopStack[mach->LoopStackTop++] = mach->LoopMask;
      ASSERT(mach->ContStackTop < TGSI_EXEC_MAX_LOOP_NESTING);
      mach->ContStack[mach->ContStackTop++] = mach->ContMask;
      break;

   case TGSI_OPCODE_ENDFOR:
      /* fall-through (for now at least) */
   case TGSI_OPCODE_ENDLOOP:
      /* Restore ContMask, but don't pop */
      ASSERT(mach->ContStackTop > 0);
      mach->ContMask = mach->ContStack[mach->ContStackTop - 1];
      if (mach->LoopMask) {
         /* repeat loop: jump to instruction just past BGNLOOP */
         *pc = inst->InstructionExtLabel.Label + 1;
      }
      else {
         /* exit loop: pop LoopMask */
         ASSERT(mach->LoopStackTop > 0);
         mach->LoopMask = mach->LoopStack[--mach->LoopStackTop];
         /* pop ContMask */
         ASSERT(mach->ContStackTop > 0);
         mach->ContMask = mach->ContStack[--mach->ContStackTop];
      }
      UPDATE_EXEC_MASK(mach);
      break;

   case TGSI_OPCODE_BRK:
      /* turn off loop channels for each enabled exec channel */
      mach->LoopMask &= ~mach->ExecMask;
      /* Todo: if mach->LoopMask == 0, jump to end of loop */
      UPDATE_EXEC_MASK(mach);
      break;

   case TGSI_OPCODE_CONT:
      /* turn off cont channels for each enabled exec channel */
      mach->ContMask &= ~mach->ExecMask;
      /* Todo: if mach->LoopMask == 0, jump to end of loop */
      UPDATE_EXEC_MASK(mach);
      break;

   case TGSI_OPCODE_BGNSUB:
      /* no-op */
      break;

   case TGSI_OPCODE_ENDSUB:
      /* no-op */
      break;

   case TGSI_OPCODE_NOP:
      break;

   default:
      ASSERT( 0 );
   }
}


/**
 * Run TGSI interpreter.
 * \return bitmask of "alive" quad components
 */
uint
spu_exec_machine_run( struct spu_exec_machine *mach )
{
   uint i;
   int pc = 0;

   mach->CondMask = 0xf;
   mach->LoopMask = 0xf;
   mach->ContMask = 0xf;
   mach->FuncMask = 0xf;
   mach->ExecMask = 0xf;

   mach->CondStackTop = 0; /* temporarily subvert this ASSERTion */
   ASSERT(mach->CondStackTop == 0);
   ASSERT(mach->LoopStackTop == 0);
   ASSERT(mach->ContStackTop == 0);
   ASSERT(mach->CallStackTop == 0);

   mach->Temps[TEMP_KILMASK_I].xyzw[TEMP_KILMASK_C].u[0] = 0;
   mach->Temps[TEMP_OUTPUT_I].xyzw[TEMP_OUTPUT_C].u[0] = 0;

   if( mach->Processor == TGSI_PROCESSOR_GEOMETRY ) {
      mach->Temps[TEMP_PRIMITIVE_I].xyzw[TEMP_PRIMITIVE_C].u[0] = 0;
      mach->Primitives[0] = 0;
   }


   /* execute declarations (interpolants) */
   if( mach->Processor == TGSI_PROCESSOR_FRAGMENT ) {
      for (i = 0; i < mach->NumDeclarations; i++) {
         PIPE_ALIGN_VAR(16)
         union {
            struct tgsi_full_declaration decl;
            qword buffer[ROUNDUP16(sizeof(struct tgsi_full_declaration)) / 16];
         } d;
         unsigned ea = (unsigned) (mach->Declarations + pc);

         spu_dcache_fetch_unaligned(d.buffer, ea, sizeof(d.decl));

         exec_declaration( mach, &d.decl );
      }
   }

   /* execute instructions, until pc is set to -1 */
   while (pc != -1) {
      PIPE_ALIGN_VAR(16)
      union {
         struct tgsi_full_instruction inst;
         qword buffer[ROUNDUP16(sizeof(struct tgsi_full_instruction)) / 16];
      } i;
      unsigned ea = (unsigned) (mach->Instructions + pc);

      spu_dcache_fetch_unaligned(i.buffer, ea, sizeof(i.inst));
      exec_instruction( mach, & i.inst, &pc );
   }

#if 0
   /* we scale from floats in [0,1] to Zbuffer ints in sp_quad_depth_test.c */
   if (mach->Processor == TGSI_PROCESSOR_FRAGMENT) {
      /*
       * Scale back depth component.
       */
      for (i = 0; i < 4; i++)
         mach->Outputs[0].xyzw[2].f[i] *= ctx->DrawBuffer->_DepthMaxF;
   }
#endif

   return ~mach->Temps[TEMP_KILMASK_I].xyzw[TEMP_KILMASK_C].u[0];
}


