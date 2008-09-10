/**************************************************************************
 * 
 * Copyright 2007-2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * TGSI interpreter/executor.
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

#include "pipe/p_compiler.h"
#include "pipe/p_state.h"
#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "tgsi_exec.h"
#include "util/u_memory.h"
#include "util/u_math.h"

#define FAST_MATH 1

#define TILE_TOP_LEFT     0
#define TILE_TOP_RIGHT    1
#define TILE_BOTTOM_LEFT  2
#define TILE_BOTTOM_RIGHT 3

#define CHAN_X  0
#define CHAN_Y  1
#define CHAN_Z  2
#define CHAN_W  3

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
#define TEMP_CC_I          TGSI_EXEC_TEMP_CC_I
#define TEMP_CC_C          TGSI_EXEC_TEMP_CC_C
#define TEMP_3_I           TGSI_EXEC_TEMP_THREE_I
#define TEMP_3_C           TGSI_EXEC_TEMP_THREE_C
#define TEMP_HALF_I        TGSI_EXEC_TEMP_HALF_I
#define TEMP_HALF_C        TGSI_EXEC_TEMP_HALF_C
#define TEMP_R0            TGSI_EXEC_TEMP_R0

#define IS_CHANNEL_ENABLED(INST, CHAN)\
   ((INST).FullDstRegisters[0].DstRegister.WriteMask & (1 << (CHAN)))

#define IS_CHANNEL_ENABLED2(INST, CHAN)\
   ((INST).FullDstRegisters[1].DstRegister.WriteMask & (1 << (CHAN)))

#define FOR_EACH_ENABLED_CHANNEL(INST, CHAN)\
   for (CHAN = 0; CHAN < NUM_CHANNELS; CHAN++)\
      if (IS_CHANNEL_ENABLED( INST, CHAN ))

#define FOR_EACH_ENABLED_CHANNEL2(INST, CHAN)\
   for (CHAN = 0; CHAN < NUM_CHANNELS; CHAN++)\
      if (IS_CHANNEL_ENABLED2( INST, CHAN ))


/** The execution mask depends on the conditional mask and the loop mask */
#define UPDATE_EXEC_MASK(MACH) \
      MACH->ExecMask = MACH->CondMask & MACH->LoopMask & MACH->ContMask & MACH->FuncMask

/**
 * Initialize machine state by expanding tokens to full instructions,
 * allocating temporary storage, setting up constants, etc.
 * After this, we can call tgsi_exec_machine_run() many times.
 */
void 
tgsi_exec_machine_bind_shader(
   struct tgsi_exec_machine *mach,
   const struct tgsi_token *tokens,
   uint numSamplers,
   struct tgsi_sampler *samplers)
{
   uint k;
   struct tgsi_parse_context parse;
   struct tgsi_exec_labels *labels = &mach->Labels;
   struct tgsi_full_instruction *instructions;
   struct tgsi_full_declaration *declarations;
   uint maxInstructions = 10, numInstructions = 0;
   uint maxDeclarations = 10, numDeclarations = 0;
   uint instno = 0;

#if 0
   tgsi_dump(tokens, 0);
#endif

   util_init_math();

   mach->Tokens = tokens;
   mach->Samplers = samplers;

   k = tgsi_parse_init (&parse, mach->Tokens);
   if (k != TGSI_PARSE_OK) {
      debug_printf( "Problem parsing!\n" );
      return;
   }

   mach->Processor = parse.FullHeader.Processor.Processor;
   mach->ImmLimit = 0;
   labels->count = 0;

   declarations = (struct tgsi_full_declaration *)
      MALLOC( maxDeclarations * sizeof(struct tgsi_full_declaration) );

   if (!declarations) {
      return;
   }

   instructions = (struct tgsi_full_instruction *)
      MALLOC( maxInstructions * sizeof(struct tgsi_full_instruction) );

   if (!instructions) {
      FREE( declarations );
      return;
   }

   while( !tgsi_parse_end_of_tokens( &parse ) ) {
      uint pointer = parse.Position;
      uint i;

      tgsi_parse_token( &parse );
      switch( parse.FullToken.Token.Type ) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         /* save expanded declaration */
         if (numDeclarations == maxDeclarations) {
            declarations = REALLOC(declarations,
                                   maxDeclarations
                                   * sizeof(struct tgsi_full_declaration),
                                   (maxDeclarations + 10)
                                   * sizeof(struct tgsi_full_declaration));
            maxDeclarations += 10;
         }
         memcpy(declarations + numDeclarations,
                &parse.FullToken.FullDeclaration,
                sizeof(declarations[0]));
         numDeclarations++;
         break;

      case TGSI_TOKEN_TYPE_IMMEDIATE:
         {
            uint size = parse.FullToken.FullImmediate.Immediate.Size - 1;
            assert( size % 4 == 0 );
            assert( mach->ImmLimit + size / 4 <= TGSI_EXEC_NUM_IMMEDIATES );

            for( i = 0; i < size; i++ ) {
               mach->Imms[mach->ImmLimit + i / 4][i % 4] = 
		  parse.FullToken.FullImmediate.u.ImmediateFloat32[i].Float;
            }
            mach->ImmLimit += size / 4;
         }
         break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         assert( labels->count < MAX_LABELS );

         labels->labels[labels->count][0] = instno;
         labels->labels[labels->count][1] = pointer;
         labels->count++;

         /* save expanded instruction */
         if (numInstructions == maxInstructions) {
            instructions = REALLOC(instructions,
                                   maxInstructions
                                   * sizeof(struct tgsi_full_instruction),
                                   (maxInstructions + 10)
                                   * sizeof(struct tgsi_full_instruction));
            maxInstructions += 10;
         }
         memcpy(instructions + numInstructions,
                &parse.FullToken.FullInstruction,
                sizeof(instructions[0]));
         numInstructions++;
         break;

      default:
         assert( 0 );
      }
   }
   tgsi_parse_free (&parse);

   if (mach->Declarations) {
      FREE( mach->Declarations );
   }
   mach->Declarations = declarations;
   mach->NumDeclarations = numDeclarations;

   if (mach->Instructions) {
      FREE( mach->Instructions );
   }
   mach->Instructions = instructions;
   mach->NumInstructions = numInstructions;
}


void
tgsi_exec_machine_init(
   struct tgsi_exec_machine *mach )
{
   uint i;

   mach->Temps = (struct tgsi_exec_vector *) tgsi_align_128bit( mach->_Temps);
   mach->Addrs = &mach->Temps[TGSI_EXEC_TEMP_ADDR];

   /* Setup constants. */
   for( i = 0; i < 4; i++ ) {
      mach->Temps[TEMP_0_I].xyzw[TEMP_0_C].u[i] = 0x00000000;
      mach->Temps[TEMP_7F_I].xyzw[TEMP_7F_C].u[i] = 0x7FFFFFFF;
      mach->Temps[TEMP_80_I].xyzw[TEMP_80_C].u[i] = 0x80000000;
      mach->Temps[TEMP_FF_I].xyzw[TEMP_FF_C].u[i] = 0xFFFFFFFF;
      mach->Temps[TEMP_1_I].xyzw[TEMP_1_C].f[i] = 1.0f;
      mach->Temps[TEMP_2_I].xyzw[TEMP_2_C].f[i] = 2.0f;
      mach->Temps[TEMP_128_I].xyzw[TEMP_128_C].f[i] = 128.0f;
      mach->Temps[TEMP_M128_I].xyzw[TEMP_M128_C].f[i] = -128.0f;
      mach->Temps[TEMP_3_I].xyzw[TEMP_3_C].f[i] = 3.0f;
      mach->Temps[TEMP_HALF_I].xyzw[TEMP_HALF_C].f[i] = 0.5f;
   }
}


void
tgsi_exec_machine_free_data(struct tgsi_exec_machine *mach)
{
   if (mach->Instructions) {
      FREE(mach->Instructions);
      mach->Instructions = NULL;
      mach->NumInstructions = 0;
   }
   if (mach->Declarations) {
      FREE(mach->Declarations);
      mach->Declarations = NULL;
      mach->NumDeclarations = 0;
   }
}


static void
micro_abs(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->f[0] = fabsf( src->f[0] );
   dst->f[1] = fabsf( src->f[1] );
   dst->f[2] = fabsf( src->f[2] );
   dst->f[3] = fabsf( src->f[3] );
}

static void
micro_add(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
   dst->f[0] = src0->f[0] + src1->f[0];
   dst->f[1] = src0->f[1] + src1->f[1];
   dst->f[2] = src0->f[2] + src1->f[2];
   dst->f[3] = src0->f[3] + src1->f[3];
}

static void
micro_iadd(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
   dst->i[0] = src0->i[0] + src1->i[0];
   dst->i[1] = src0->i[1] + src1->i[1];
   dst->i[2] = src0->i[2] + src1->i[2];
   dst->i[3] = src0->i[3] + src1->i[3];
}

static void
micro_and(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
   dst->u[0] = src0->u[0] & src1->u[0];
   dst->u[1] = src0->u[1] & src1->u[1];
   dst->u[2] = src0->u[2] & src1->u[2];
   dst->u[3] = src0->u[3] & src1->u[3];
}

static void
micro_ceil(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->f[0] = ceilf( src->f[0] );
   dst->f[1] = ceilf( src->f[1] );
   dst->f[2] = ceilf( src->f[2] );
   dst->f[3] = ceilf( src->f[3] );
}

static void
micro_cos(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->f[0] = cosf( src->f[0] );
   dst->f[1] = cosf( src->f[1] );
   dst->f[2] = cosf( src->f[2] );
   dst->f[3] = cosf( src->f[3] );
}

static void
micro_ddx(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->f[0] =
   dst->f[1] =
   dst->f[2] =
   dst->f[3] = src->f[TILE_BOTTOM_RIGHT] - src->f[TILE_BOTTOM_LEFT];
}

static void
micro_ddy(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->f[0] =
   dst->f[1] =
   dst->f[2] =
   dst->f[3] = src->f[TILE_TOP_LEFT] - src->f[TILE_BOTTOM_LEFT];
}

static void
micro_div(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
   if (src1->f[0] != 0) {
      dst->f[0] = src0->f[0] / src1->f[0];
   }
   if (src1->f[1] != 0) {
      dst->f[1] = src0->f[1] / src1->f[1];
   }
   if (src1->f[2] != 0) {
      dst->f[2] = src0->f[2] / src1->f[2];
   }
   if (src1->f[3] != 0) {
      dst->f[3] = src0->f[3] / src1->f[3];
   }
}

static void
micro_udiv(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
   dst->u[0] = src0->u[0] / src1->u[0];
   dst->u[1] = src0->u[1] / src1->u[1];
   dst->u[2] = src0->u[2] / src1->u[2];
   dst->u[3] = src0->u[3] / src1->u[3];
}

static void
micro_eq(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1,
   const union tgsi_exec_channel *src2,
   const union tgsi_exec_channel *src3 )
{
   dst->f[0] = src0->f[0] == src1->f[0] ? src2->f[0] : src3->f[0];
   dst->f[1] = src0->f[1] == src1->f[1] ? src2->f[1] : src3->f[1];
   dst->f[2] = src0->f[2] == src1->f[2] ? src2->f[2] : src3->f[2];
   dst->f[3] = src0->f[3] == src1->f[3] ? src2->f[3] : src3->f[3];
}

static void
micro_ieq(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1,
   const union tgsi_exec_channel *src2,
   const union tgsi_exec_channel *src3 )
{
   dst->i[0] = src0->i[0] == src1->i[0] ? src2->i[0] : src3->i[0];
   dst->i[1] = src0->i[1] == src1->i[1] ? src2->i[1] : src3->i[1];
   dst->i[2] = src0->i[2] == src1->i[2] ? src2->i[2] : src3->i[2];
   dst->i[3] = src0->i[3] == src1->i[3] ? src2->i[3] : src3->i[3];
}

static void
micro_exp2(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src)
{
#if FAST_MATH
   dst->f[0] = util_fast_exp2( src->f[0] );
   dst->f[1] = util_fast_exp2( src->f[1] );
   dst->f[2] = util_fast_exp2( src->f[2] );
   dst->f[3] = util_fast_exp2( src->f[3] );
#else
   dst->f[0] = powf( 2.0f, src->f[0] );
   dst->f[1] = powf( 2.0f, src->f[1] );
   dst->f[2] = powf( 2.0f, src->f[2] );
   dst->f[3] = powf( 2.0f, src->f[3] );
#endif
}

static void
micro_f2it(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->i[0] = (int) src->f[0];
   dst->i[1] = (int) src->f[1];
   dst->i[2] = (int) src->f[2];
   dst->i[3] = (int) src->f[3];
}

static void
micro_f2ut(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->u[0] = (uint) src->f[0];
   dst->u[1] = (uint) src->f[1];
   dst->u[2] = (uint) src->f[2];
   dst->u[3] = (uint) src->f[3];
}

static void
micro_flr(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->f[0] = floorf( src->f[0] );
   dst->f[1] = floorf( src->f[1] );
   dst->f[2] = floorf( src->f[2] );
   dst->f[3] = floorf( src->f[3] );
}

static void
micro_frc(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->f[0] = src->f[0] - floorf( src->f[0] );
   dst->f[1] = src->f[1] - floorf( src->f[1] );
   dst->f[2] = src->f[2] - floorf( src->f[2] );
   dst->f[3] = src->f[3] - floorf( src->f[3] );
}

static void
micro_ge(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1,
   const union tgsi_exec_channel *src2,
   const union tgsi_exec_channel *src3 )
{
   dst->f[0] = src0->f[0] >= src1->f[0] ? src2->f[0] : src3->f[0];
   dst->f[1] = src0->f[1] >= src1->f[1] ? src2->f[1] : src3->f[1];
   dst->f[2] = src0->f[2] >= src1->f[2] ? src2->f[2] : src3->f[2];
   dst->f[3] = src0->f[3] >= src1->f[3] ? src2->f[3] : src3->f[3];
}

static void
micro_i2f(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->f[0] = (float) src->i[0];
   dst->f[1] = (float) src->i[1];
   dst->f[2] = (float) src->i[2];
   dst->f[3] = (float) src->i[3];
}

static void
micro_lg2(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
#if FAST_MATH
   dst->f[0] = util_fast_log2( src->f[0] );
   dst->f[1] = util_fast_log2( src->f[1] );
   dst->f[2] = util_fast_log2( src->f[2] );
   dst->f[3] = util_fast_log2( src->f[3] );
#else
   dst->f[0] = logf( src->f[0] ) * 1.442695f;
   dst->f[1] = logf( src->f[1] ) * 1.442695f;
   dst->f[2] = logf( src->f[2] ) * 1.442695f;
   dst->f[3] = logf( src->f[3] ) * 1.442695f;
#endif
}

static void
micro_le(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1,
   const union tgsi_exec_channel *src2,
   const union tgsi_exec_channel *src3 )
{
   dst->f[0] = src0->f[0] <= src1->f[0] ? src2->f[0] : src3->f[0];
   dst->f[1] = src0->f[1] <= src1->f[1] ? src2->f[1] : src3->f[1];
   dst->f[2] = src0->f[2] <= src1->f[2] ? src2->f[2] : src3->f[2];
   dst->f[3] = src0->f[3] <= src1->f[3] ? src2->f[3] : src3->f[3];
}

static void
micro_lt(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1,
   const union tgsi_exec_channel *src2,
   const union tgsi_exec_channel *src3 )
{
   dst->f[0] = src0->f[0] < src1->f[0] ? src2->f[0] : src3->f[0];
   dst->f[1] = src0->f[1] < src1->f[1] ? src2->f[1] : src3->f[1];
   dst->f[2] = src0->f[2] < src1->f[2] ? src2->f[2] : src3->f[2];
   dst->f[3] = src0->f[3] < src1->f[3] ? src2->f[3] : src3->f[3];
}

static void
micro_ilt(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1,
   const union tgsi_exec_channel *src2,
   const union tgsi_exec_channel *src3 )
{
   dst->i[0] = src0->i[0] < src1->i[0] ? src2->i[0] : src3->i[0];
   dst->i[1] = src0->i[1] < src1->i[1] ? src2->i[1] : src3->i[1];
   dst->i[2] = src0->i[2] < src1->i[2] ? src2->i[2] : src3->i[2];
   dst->i[3] = src0->i[3] < src1->i[3] ? src2->i[3] : src3->i[3];
}

static void
micro_ult(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1,
   const union tgsi_exec_channel *src2,
   const union tgsi_exec_channel *src3 )
{
   dst->u[0] = src0->u[0] < src1->u[0] ? src2->u[0] : src3->u[0];
   dst->u[1] = src0->u[1] < src1->u[1] ? src2->u[1] : src3->u[1];
   dst->u[2] = src0->u[2] < src1->u[2] ? src2->u[2] : src3->u[2];
   dst->u[3] = src0->u[3] < src1->u[3] ? src2->u[3] : src3->u[3];
}

static void
micro_max(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
   dst->f[0] = src0->f[0] > src1->f[0] ? src0->f[0] : src1->f[0];
   dst->f[1] = src0->f[1] > src1->f[1] ? src0->f[1] : src1->f[1];
   dst->f[2] = src0->f[2] > src1->f[2] ? src0->f[2] : src1->f[2];
   dst->f[3] = src0->f[3] > src1->f[3] ? src0->f[3] : src1->f[3];
}

static void
micro_imax(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
   dst->i[0] = src0->i[0] > src1->i[0] ? src0->i[0] : src1->i[0];
   dst->i[1] = src0->i[1] > src1->i[1] ? src0->i[1] : src1->i[1];
   dst->i[2] = src0->i[2] > src1->i[2] ? src0->i[2] : src1->i[2];
   dst->i[3] = src0->i[3] > src1->i[3] ? src0->i[3] : src1->i[3];
}

static void
micro_umax(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
   dst->u[0] = src0->u[0] > src1->u[0] ? src0->u[0] : src1->u[0];
   dst->u[1] = src0->u[1] > src1->u[1] ? src0->u[1] : src1->u[1];
   dst->u[2] = src0->u[2] > src1->u[2] ? src0->u[2] : src1->u[2];
   dst->u[3] = src0->u[3] > src1->u[3] ? src0->u[3] : src1->u[3];
}

static void
micro_min(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
   dst->f[0] = src0->f[0] < src1->f[0] ? src0->f[0] : src1->f[0];
   dst->f[1] = src0->f[1] < src1->f[1] ? src0->f[1] : src1->f[1];
   dst->f[2] = src0->f[2] < src1->f[2] ? src0->f[2] : src1->f[2];
   dst->f[3] = src0->f[3] < src1->f[3] ? src0->f[3] : src1->f[3];
}

static void
micro_imin(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
   dst->i[0] = src0->i[0] < src1->i[0] ? src0->i[0] : src1->i[0];
   dst->i[1] = src0->i[1] < src1->i[1] ? src0->i[1] : src1->i[1];
   dst->i[2] = src0->i[2] < src1->i[2] ? src0->i[2] : src1->i[2];
   dst->i[3] = src0->i[3] < src1->i[3] ? src0->i[3] : src1->i[3];
}

static void
micro_umin(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
   dst->u[0] = src0->u[0] < src1->u[0] ? src0->u[0] : src1->u[0];
   dst->u[1] = src0->u[1] < src1->u[1] ? src0->u[1] : src1->u[1];
   dst->u[2] = src0->u[2] < src1->u[2] ? src0->u[2] : src1->u[2];
   dst->u[3] = src0->u[3] < src1->u[3] ? src0->u[3] : src1->u[3];
}

static void
micro_umod(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
   dst->u[0] = src0->u[0] % src1->u[0];
   dst->u[1] = src0->u[1] % src1->u[1];
   dst->u[2] = src0->u[2] % src1->u[2];
   dst->u[3] = src0->u[3] % src1->u[3];
}

static void
micro_mul(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
   dst->f[0] = src0->f[0] * src1->f[0];
   dst->f[1] = src0->f[1] * src1->f[1];
   dst->f[2] = src0->f[2] * src1->f[2];
   dst->f[3] = src0->f[3] * src1->f[3];
}

static void
micro_imul(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
   dst->i[0] = src0->i[0] * src1->i[0];
   dst->i[1] = src0->i[1] * src1->i[1];
   dst->i[2] = src0->i[2] * src1->i[2];
   dst->i[3] = src0->i[3] * src1->i[3];
}

static void
micro_imul64(
   union tgsi_exec_channel *dst0,
   union tgsi_exec_channel *dst1,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
   dst1->i[0] = src0->i[0] * src1->i[0];
   dst1->i[1] = src0->i[1] * src1->i[1];
   dst1->i[2] = src0->i[2] * src1->i[2];
   dst1->i[3] = src0->i[3] * src1->i[3];
   dst0->i[0] = 0;
   dst0->i[1] = 0;
   dst0->i[2] = 0;
   dst0->i[3] = 0;
}

static void
micro_umul64(
   union tgsi_exec_channel *dst0,
   union tgsi_exec_channel *dst1,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
   dst1->u[0] = src0->u[0] * src1->u[0];
   dst1->u[1] = src0->u[1] * src1->u[1];
   dst1->u[2] = src0->u[2] * src1->u[2];
   dst1->u[3] = src0->u[3] * src1->u[3];
   dst0->u[0] = 0;
   dst0->u[1] = 0;
   dst0->u[2] = 0;
   dst0->u[3] = 0;
}

static void
micro_movc(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1,
   const union tgsi_exec_channel *src2 )
{
   dst->u[0] = src0->u[0] ? src1->u[0] : src2->u[0];
   dst->u[1] = src0->u[1] ? src1->u[1] : src2->u[1];
   dst->u[2] = src0->u[2] ? src1->u[2] : src2->u[2];
   dst->u[3] = src0->u[3] ? src1->u[3] : src2->u[3];
}

static void
micro_neg(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->f[0] = -src->f[0];
   dst->f[1] = -src->f[1];
   dst->f[2] = -src->f[2];
   dst->f[3] = -src->f[3];
}

static void
micro_ineg(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->i[0] = -src->i[0];
   dst->i[1] = -src->i[1];
   dst->i[2] = -src->i[2];
   dst->i[3] = -src->i[3];
}

static void
micro_not(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->u[0] = ~src->u[0];
   dst->u[1] = ~src->u[1];
   dst->u[2] = ~src->u[2];
   dst->u[3] = ~src->u[3];
}

static void
micro_or(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
   dst->u[0] = src0->u[0] | src1->u[0];
   dst->u[1] = src0->u[1] | src1->u[1];
   dst->u[2] = src0->u[2] | src1->u[2];
   dst->u[3] = src0->u[3] | src1->u[3];
}

static void
micro_pow(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
#if FAST_MATH
   dst->f[0] = util_fast_pow( src0->f[0], src1->f[0] );
   dst->f[1] = util_fast_pow( src0->f[1], src1->f[1] );
   dst->f[2] = util_fast_pow( src0->f[2], src1->f[2] );
   dst->f[3] = util_fast_pow( src0->f[3], src1->f[3] );
#else
   dst->f[0] = powf( src0->f[0], src1->f[0] );
   dst->f[1] = powf( src0->f[1], src1->f[1] );
   dst->f[2] = powf( src0->f[2], src1->f[2] );
   dst->f[3] = powf( src0->f[3], src1->f[3] );
#endif
}

static void
micro_rnd(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->f[0] = floorf( src->f[0] + 0.5f );
   dst->f[1] = floorf( src->f[1] + 0.5f );
   dst->f[2] = floorf( src->f[2] + 0.5f );
   dst->f[3] = floorf( src->f[3] + 0.5f );
}

static void
micro_shl(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
   dst->i[0] = src0->i[0] << src1->i[0];
   dst->i[1] = src0->i[1] << src1->i[1];
   dst->i[2] = src0->i[2] << src1->i[2];
   dst->i[3] = src0->i[3] << src1->i[3];
}

static void
micro_ishr(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
   dst->i[0] = src0->i[0] >> src1->i[0];
   dst->i[1] = src0->i[1] >> src1->i[1];
   dst->i[2] = src0->i[2] >> src1->i[2];
   dst->i[3] = src0->i[3] >> src1->i[3];
}

static void
micro_trunc(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0 )
{
   dst->f[0] = (float) (int) src0->f[0];
   dst->f[1] = (float) (int) src0->f[1];
   dst->f[2] = (float) (int) src0->f[2];
   dst->f[3] = (float) (int) src0->f[3];
}

static void
micro_ushr(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
   dst->u[0] = src0->u[0] >> src1->u[0];
   dst->u[1] = src0->u[1] >> src1->u[1];
   dst->u[2] = src0->u[2] >> src1->u[2];
   dst->u[3] = src0->u[3] >> src1->u[3];
}

static void
micro_sin(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->f[0] = sinf( src->f[0] );
   dst->f[1] = sinf( src->f[1] );
   dst->f[2] = sinf( src->f[2] );
   dst->f[3] = sinf( src->f[3] );
}

static void
micro_sqrt( union tgsi_exec_channel *dst,
            const union tgsi_exec_channel *src )
{
   dst->f[0] = sqrtf( src->f[0] );
   dst->f[1] = sqrtf( src->f[1] );
   dst->f[2] = sqrtf( src->f[2] );
   dst->f[3] = sqrtf( src->f[3] );
}

static void
micro_sub(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
   dst->f[0] = src0->f[0] - src1->f[0];
   dst->f[1] = src0->f[1] - src1->f[1];
   dst->f[2] = src0->f[2] - src1->f[2];
   dst->f[3] = src0->f[3] - src1->f[3];
}

static void
micro_u2f(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->f[0] = (float) src->u[0];
   dst->f[1] = (float) src->u[1];
   dst->f[2] = (float) src->u[2];
   dst->f[3] = (float) src->u[3];
}

static void
micro_xor(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src0,
   const union tgsi_exec_channel *src1 )
{
   dst->u[0] = src0->u[0] ^ src1->u[0];
   dst->u[1] = src0->u[1] ^ src1->u[1];
   dst->u[2] = src0->u[2] ^ src1->u[2];
   dst->u[3] = src0->u[3] ^ src1->u[3];
}

static void
fetch_src_file_channel(
   const struct tgsi_exec_machine *mach,
   const uint file,
   const uint swizzle,
   const union tgsi_exec_channel *index,
   union tgsi_exec_channel *chan )
{
   switch( swizzle ) {
   case TGSI_EXTSWIZZLE_X:
   case TGSI_EXTSWIZZLE_Y:
   case TGSI_EXTSWIZZLE_Z:
   case TGSI_EXTSWIZZLE_W:
      switch( file ) {
      case TGSI_FILE_CONSTANT:
         assert(mach->Consts);
         chan->f[0] = mach->Consts[index->i[0]][swizzle];
         chan->f[1] = mach->Consts[index->i[1]][swizzle];
         chan->f[2] = mach->Consts[index->i[2]][swizzle];
         chan->f[3] = mach->Consts[index->i[3]][swizzle];
         break;

      case TGSI_FILE_INPUT:
         chan->u[0] = mach->Inputs[index->i[0]].xyzw[swizzle].u[0];
         chan->u[1] = mach->Inputs[index->i[1]].xyzw[swizzle].u[1];
         chan->u[2] = mach->Inputs[index->i[2]].xyzw[swizzle].u[2];
         chan->u[3] = mach->Inputs[index->i[3]].xyzw[swizzle].u[3];
         break;

      case TGSI_FILE_TEMPORARY:
         assert(index->i[0] < TGSI_EXEC_NUM_TEMPS);
         chan->u[0] = mach->Temps[index->i[0]].xyzw[swizzle].u[0];
         chan->u[1] = mach->Temps[index->i[1]].xyzw[swizzle].u[1];
         chan->u[2] = mach->Temps[index->i[2]].xyzw[swizzle].u[2];
         chan->u[3] = mach->Temps[index->i[3]].xyzw[swizzle].u[3];
         break;

      case TGSI_FILE_IMMEDIATE:
         assert( index->i[0] < (int) mach->ImmLimit );
         chan->f[0] = mach->Imms[index->i[0]][swizzle];
         assert( index->i[1] < (int) mach->ImmLimit );
         chan->f[1] = mach->Imms[index->i[1]][swizzle];
         assert( index->i[2] < (int) mach->ImmLimit );
         chan->f[2] = mach->Imms[index->i[2]][swizzle];
         assert( index->i[3] < (int) mach->ImmLimit );
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
         assert( 0 );
      }
      break;

   case TGSI_EXTSWIZZLE_ZERO:
      *chan = mach->Temps[TEMP_0_I].xyzw[TEMP_0_C];
      break;

   case TGSI_EXTSWIZZLE_ONE:
      *chan = mach->Temps[TEMP_1_I].xyzw[TEMP_1_C];
      break;

   default:
      assert( 0 );
   }
}

static void
fetch_source(
   const struct tgsi_exec_machine *mach,
   union tgsi_exec_channel *chan,
   const struct tgsi_full_src_register *reg,
   const uint chan_index )
{
   union tgsi_exec_channel index;
   uint swizzle;

   index.i[0] =
   index.i[1] =
   index.i[2] =
   index.i[3] = reg->SrcRegister.Index;

   if (reg->SrcRegister.Indirect) {
      union tgsi_exec_channel index2;
      union tgsi_exec_channel indir_index;

      index2.i[0] =
      index2.i[1] =
      index2.i[2] =
      index2.i[3] = reg->SrcRegisterInd.Index;

      swizzle = tgsi_util_get_src_register_swizzle( &reg->SrcRegisterInd, CHAN_X );
      fetch_src_file_channel(
         mach,
         reg->SrcRegisterInd.File,
         swizzle,
         &index2,
         &indir_index );

      index.i[0] += indir_index.i[0];
      index.i[1] += indir_index.i[1];
      index.i[2] += indir_index.i[2];
      index.i[3] += indir_index.i[3];
   }

   if( reg->SrcRegister.Dimension ) {
      switch( reg->SrcRegister.File ) {
      case TGSI_FILE_INPUT:
         index.i[0] *= 17;
         index.i[1] *= 17;
         index.i[2] *= 17;
         index.i[3] *= 17;
         break;
      case TGSI_FILE_CONSTANT:
         index.i[0] *= 4096;
         index.i[1] *= 4096;
         index.i[2] *= 4096;
         index.i[3] *= 4096;
         break;
      default:
         assert( 0 );
      }

      index.i[0] += reg->SrcRegisterDim.Index;
      index.i[1] += reg->SrcRegisterDim.Index;
      index.i[2] += reg->SrcRegisterDim.Index;
      index.i[3] += reg->SrcRegisterDim.Index;

      if (reg->SrcRegisterDim.Indirect) {
         union tgsi_exec_channel index2;
         union tgsi_exec_channel indir_index;

         index2.i[0] =
         index2.i[1] =
         index2.i[2] =
         index2.i[3] = reg->SrcRegisterDimInd.Index;

         swizzle = tgsi_util_get_src_register_swizzle( &reg->SrcRegisterDimInd, CHAN_X );
         fetch_src_file_channel(
            mach,
            reg->SrcRegisterDimInd.File,
            swizzle,
            &index2,
            &indir_index );

         index.i[0] += indir_index.i[0];
         index.i[1] += indir_index.i[1];
         index.i[2] += indir_index.i[2];
         index.i[3] += indir_index.i[3];
      }
   }

   swizzle = tgsi_util_get_full_src_register_extswizzle( reg, chan_index );
   fetch_src_file_channel(
      mach,
      reg->SrcRegister.File,
      swizzle,
      &index,
      chan );

   switch (tgsi_util_get_full_src_register_sign_mode( reg, chan_index )) {
   case TGSI_UTIL_SIGN_CLEAR:
      micro_abs( chan, chan );
      break;

   case TGSI_UTIL_SIGN_SET:
      micro_abs( chan, chan );
      micro_neg( chan, chan );
      break;

   case TGSI_UTIL_SIGN_TOGGLE:
      micro_neg( chan, chan );
      break;

   case TGSI_UTIL_SIGN_KEEP:
      break;
   }

   if (reg->SrcRegisterExtMod.Complement) {
      micro_sub( chan, &mach->Temps[TEMP_1_I].xyzw[TEMP_1_C], chan );
   }
}

static void
store_dest(
   struct tgsi_exec_machine *mach,
   const union tgsi_exec_channel *chan,
   const struct tgsi_full_dst_register *reg,
   const struct tgsi_full_instruction *inst,
   uint chan_index )
{
   uint i;
   union tgsi_exec_channel null;
   union tgsi_exec_channel *dst;
   uint execmask = mach->ExecMask;

   switch (reg->DstRegister.File) {
   case TGSI_FILE_NULL:
      dst = &null;
      break;

   case TGSI_FILE_OUTPUT:
      dst = &mach->Outputs[mach->Temps[TEMP_OUTPUT_I].xyzw[TEMP_OUTPUT_C].u[0]
                           + reg->DstRegister.Index].xyzw[chan_index];
      break;

   case TGSI_FILE_TEMPORARY:
      assert( reg->DstRegister.Index < TGSI_EXEC_NUM_TEMPS );
      dst = &mach->Temps[reg->DstRegister.Index].xyzw[chan_index];
      break;

   case TGSI_FILE_ADDRESS:
      dst = &mach->Addrs[reg->DstRegister.Index].xyzw[chan_index];
      break;

   default:
      assert( 0 );
      return;
   }

   if (inst->InstructionExtNv.CondFlowEnable) {
      union tgsi_exec_channel *cc = &mach->Temps[TEMP_CC_I].xyzw[TEMP_CC_C];
      uint swizzle;
      uint shift;
      uint mask;
      uint test;

      /* Only CC0 supported.
       */
      assert( inst->InstructionExtNv.CondFlowIndex < 1 );

      switch (chan_index) {
      case CHAN_X:
         swizzle = inst->InstructionExtNv.CondSwizzleX;
         break;
      case CHAN_Y:
         swizzle = inst->InstructionExtNv.CondSwizzleY;
         break;
      case CHAN_Z:
         swizzle = inst->InstructionExtNv.CondSwizzleZ;
         break;
      case CHAN_W:
         swizzle = inst->InstructionExtNv.CondSwizzleW;
         break;
      default:
         assert( 0 );
         return;
      }

      switch (swizzle) {
      case TGSI_SWIZZLE_X:
         shift = TGSI_EXEC_CC_X_SHIFT;
         mask = TGSI_EXEC_CC_X_MASK;
         break;
      case TGSI_SWIZZLE_Y:
         shift = TGSI_EXEC_CC_Y_SHIFT;
         mask = TGSI_EXEC_CC_Y_MASK;
         break;
      case TGSI_SWIZZLE_Z:
         shift = TGSI_EXEC_CC_Z_SHIFT;
         mask = TGSI_EXEC_CC_Z_MASK;
         break;
      case TGSI_SWIZZLE_W:
         shift = TGSI_EXEC_CC_W_SHIFT;
         mask = TGSI_EXEC_CC_W_MASK;
         break;
      default:
         assert( 0 );
         return;
      }

      switch (inst->InstructionExtNv.CondMask) {
      case TGSI_CC_GT:
         test = ~(TGSI_EXEC_CC_GT << shift) & mask;
         for (i = 0; i < QUAD_SIZE; i++)
            if (cc->u[i] & test)
               execmask &= ~(1 << i);
         break;

      case TGSI_CC_EQ:
         test = ~(TGSI_EXEC_CC_EQ << shift) & mask;
         for (i = 0; i < QUAD_SIZE; i++)
            if (cc->u[i] & test)
               execmask &= ~(1 << i);
         break;

      case TGSI_CC_LT:
         test = ~(TGSI_EXEC_CC_LT << shift) & mask;
         for (i = 0; i < QUAD_SIZE; i++)
            if (cc->u[i] & test)
               execmask &= ~(1 << i);
         break;

      case TGSI_CC_GE:
         test = ~((TGSI_EXEC_CC_GT | TGSI_EXEC_CC_EQ) << shift) & mask;
         for (i = 0; i < QUAD_SIZE; i++)
            if (cc->u[i] & test)
               execmask &= ~(1 << i);
         break;

      case TGSI_CC_LE:
         test = ~((TGSI_EXEC_CC_LT | TGSI_EXEC_CC_EQ) << shift) & mask;
         for (i = 0; i < QUAD_SIZE; i++)
            if (cc->u[i] & test)
               execmask &= ~(1 << i);
         break;

      case TGSI_CC_NE:
         test = ~((TGSI_EXEC_CC_GT | TGSI_EXEC_CC_LT | TGSI_EXEC_CC_UN) << shift) & mask;
         for (i = 0; i < QUAD_SIZE; i++)
            if (cc->u[i] & test)
               execmask &= ~(1 << i);
         break;

      case TGSI_CC_TR:
         break;

      case TGSI_CC_FL:
         for (i = 0; i < QUAD_SIZE; i++)
            execmask &= ~(1 << i);
         break;

      default:
         assert( 0 );
         return;
      }
   }

   switch (inst->Instruction.Saturate) {
   case TGSI_SAT_NONE:
      for (i = 0; i < QUAD_SIZE; i++)
         if (execmask & (1 << i))
            dst->i[i] = chan->i[i];
      break;

   case TGSI_SAT_ZERO_ONE:
      for (i = 0; i < QUAD_SIZE; i++)
         if (execmask & (1 << i)) {
            if (chan->f[i] < 0.0f)
               dst->f[i] = 0.0f;
            else if (chan->f[i] > 1.0f)
               dst->f[i] = 1.0f;
            else
               dst->i[i] = chan->i[i];
         }
      break;

   case TGSI_SAT_MINUS_PLUS_ONE:
      for (i = 0; i < QUAD_SIZE; i++)
         if (execmask & (1 << i)) {
            if (chan->f[i] < -1.0f)
               dst->f[i] = -1.0f;
            else if (chan->f[i] > 1.0f)
               dst->f[i] = 1.0f;
            else
               dst->i[i] = chan->i[i];
         }
      break;

   default:
      assert( 0 );
   }

   if (inst->InstructionExtNv.CondDstUpdate) {
      union tgsi_exec_channel *cc = &mach->Temps[TEMP_CC_I].xyzw[TEMP_CC_C];
      uint shift;
      uint mask;

      /* Only CC0 supported.
       */
      assert( inst->InstructionExtNv.CondDstIndex < 1 );

      switch (chan_index) {
      case CHAN_X:
         shift = TGSI_EXEC_CC_X_SHIFT;
         mask = ~TGSI_EXEC_CC_X_MASK;
         break;
      case CHAN_Y:
         shift = TGSI_EXEC_CC_Y_SHIFT;
         mask = ~TGSI_EXEC_CC_Y_MASK;
         break;
      case CHAN_Z:
         shift = TGSI_EXEC_CC_Z_SHIFT;
         mask = ~TGSI_EXEC_CC_Z_MASK;
         break;
      case CHAN_W:
         shift = TGSI_EXEC_CC_W_SHIFT;
         mask = ~TGSI_EXEC_CC_W_MASK;
         break;
      default:
         assert( 0 );
         return;
      }

      for (i = 0; i < QUAD_SIZE; i++)
         if (execmask & (1 << i)) {
            cc->u[i] &= mask;
            if (dst->f[i] < 0.0f)
               cc->u[i] |= TGSI_EXEC_CC_LT << shift;
            else if (dst->f[i] > 0.0f)
               cc->u[i] |= TGSI_EXEC_CC_GT << shift;
            else if (dst->f[i] == 0.0f)
               cc->u[i] |= TGSI_EXEC_CC_EQ << shift;
            else
               cc->u[i] |= TGSI_EXEC_CC_UN << shift;
         }
   }
}

#define FETCH(VAL,INDEX,CHAN)\
    fetch_source (mach, VAL, &inst->FullSrcRegisters[INDEX], CHAN)

#define STORE(VAL,INDEX,CHAN)\
    store_dest (mach, VAL, &inst->FullDstRegisters[INDEX], inst, CHAN )


/**
 * Execute ARB-style KIL which is predicated by a src register.
 * Kill fragment if any of the four values is less than zero.
 */
static void
exec_kil(struct tgsi_exec_machine *mach,
         const struct tgsi_full_instruction *inst)
{
   uint uniquemask;
   uint chan_index;
   uint kilmask = 0; /* bit 0 = pixel 0, bit 1 = pixel 1, etc */
   union tgsi_exec_channel r[1];

   /* This mask stores component bits that were already tested. Note that
    * we test if the value is less than zero, so 1.0 and 0.0 need not to be
    * tested. */
   uniquemask = (1 << TGSI_EXTSWIZZLE_ZERO) | (1 << TGSI_EXTSWIZZLE_ONE);

   for (chan_index = 0; chan_index < 4; chan_index++)
   {
      uint swizzle;
      uint i;

      /* unswizzle channel */
      swizzle = tgsi_util_get_full_src_register_extswizzle (
                        &inst->FullSrcRegisters[0],
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
   uint kilmask; /* bit 0 = pixel 0, bit 1 = pixel 1, etc */

   if (inst->InstructionExtNv.CondFlowEnable) {
      uint swizzle[4];
      uint chan_index;

      kilmask = 0x0;

      swizzle[0] = inst->InstructionExtNv.CondSwizzleX;
      swizzle[1] = inst->InstructionExtNv.CondSwizzleY;
      swizzle[2] = inst->InstructionExtNv.CondSwizzleZ;
      swizzle[3] = inst->InstructionExtNv.CondSwizzleW;

      for (chan_index = 0; chan_index < 4; chan_index++)
      {
         uint i;

         for (i = 0; i < 4; i++) {
            /* TODO: evaluate the condition code */
            if (0)
               kilmask |= 1 << i;
         }
      }
   }
   else {
      /* "unconditional" kil */
      kilmask = mach->ExecMask;
   }
   mach->Temps[TEMP_KILMASK_I].xyzw[TEMP_KILMASK_C].u[0] |= kilmask;
}


/*
 * Fetch a texel using STR texture coordinates.
 */
static void
fetch_texel( struct tgsi_sampler *sampler,
             const union tgsi_exec_channel *s,
             const union tgsi_exec_channel *t,
             const union tgsi_exec_channel *p,
             float lodbias,  /* XXX should be float[4] */
             union tgsi_exec_channel *r,
             union tgsi_exec_channel *g,
             union tgsi_exec_channel *b,
             union tgsi_exec_channel *a )
{
   uint j;
   float rgba[NUM_CHANNELS][QUAD_SIZE];

   sampler->get_samples(sampler, s->f, t->f, p->f, lodbias, rgba);

   for (j = 0; j < 4; j++) {
      r->f[j] = rgba[0][j];
      g->f[j] = rgba[1][j];
      b->f[j] = rgba[2][j];
      a->f[j] = rgba[3][j];
   }
}


static void
exec_tex(struct tgsi_exec_machine *mach,
         const struct tgsi_full_instruction *inst,
         boolean biasLod,
         boolean projected)
{
   const uint unit = inst->FullSrcRegisters[1].SrcRegister.Index;
   union tgsi_exec_channel r[8];
   uint chan_index;
   float lodBias;

   /*   debug_printf("Sampler %u unit %u\n", sampler, unit); */

   switch (inst->InstructionExtTexture.Texture) {
   case TGSI_TEXTURE_1D:

      FETCH(&r[0], 0, CHAN_X);

      if (projected) {
         FETCH(&r[1], 0, CHAN_W);
         micro_div( &r[0], &r[0], &r[1] );
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
         micro_div( &r[0], &r[0], &r[3] );
         micro_div( &r[1], &r[1], &r[3] );
         micro_div( &r[2], &r[2], &r[3] );
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
         micro_div( &r[0], &r[0], &r[3] );
         micro_div( &r[1], &r[1], &r[3] );
         micro_div( &r[2], &r[2], &r[3] );
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
      assert (0);
   }

   FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
      STORE( &r[chan_index], 0, chan_index );
   }
}


/**
 * Evaluate a constant-valued coefficient at the position of the
 * current quad.
 */
static void
eval_constant_coef(
   struct tgsi_exec_machine *mach,
   unsigned attrib,
   unsigned chan )
{
   unsigned i;

   for( i = 0; i < QUAD_SIZE; i++ ) {
      mach->Inputs[attrib].xyzw[chan].f[i] = mach->InterpCoefs[attrib].a0[chan];
   }
}

/**
 * Evaluate a linear-valued coefficient at the position of the
 * current quad.
 */
static void
eval_linear_coef(
   struct tgsi_exec_machine *mach,
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

/**
 * Evaluate a perspective-valued coefficient at the position of the
 * current quad.
 */
static void
eval_perspective_coef(
   struct tgsi_exec_machine *mach,
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


typedef void (* eval_coef_func)(
   struct tgsi_exec_machine *mach,
   unsigned attrib,
   unsigned chan );

static void
exec_declaration(
   struct tgsi_exec_machine *mach,
   const struct tgsi_full_declaration *decl )
{
   if( mach->Processor == TGSI_PROCESSOR_FRAGMENT ) {
      if( decl->Declaration.File == TGSI_FILE_INPUT ) {
         unsigned first, last, mask;
         eval_coef_func eval;

         first = decl->DeclarationRange.First;
         last = decl->DeclarationRange.Last;
         mask = decl->Declaration.UsageMask;

         switch( decl->Declaration.Interpolate ) {
         case TGSI_INTERPOLATE_CONSTANT:
            eval = eval_constant_coef;
            break;

         case TGSI_INTERPOLATE_LINEAR:
            eval = eval_linear_coef;
            break;

         case TGSI_INTERPOLATE_PERSPECTIVE:
            eval = eval_perspective_coef;
            break;

         default:
            assert( 0 );
         }

         if( mask == TGSI_WRITEMASK_XYZW ) {
            unsigned i, j;

            for( i = first; i <= last; i++ ) {
               for( j = 0; j < NUM_CHANNELS; j++ ) {
                  eval( mach, i, j );
               }
            }
         }
         else {
            unsigned i, j;

            for( j = 0; j < NUM_CHANNELS; j++ ) {
               if( mask & (1 << j) ) {
                  for( i = first; i <= last; i++ ) {
                     eval( mach, i, j );
                  }
               }
            }
         }
      }
   }
}

static void
exec_instruction(
   struct tgsi_exec_machine *mach,
   const struct tgsi_full_instruction *inst,
   int *pc )
{
   uint chan_index;
   union tgsi_exec_channel r[8];

   (*pc)++;

   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_ARL:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         micro_f2it( &r[0], &r[0] );
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_MOV:
   case TGSI_OPCODE_SWZ:
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
	    micro_max( &r[0], &r[0], &mach->Temps[TEMP_0_I].xyzw[TEMP_0_C] );
	    STORE( &r[0], 0, CHAN_Y );
	 }

	 if (IS_CHANNEL_ENABLED( *inst, CHAN_Z )) {
	    FETCH( &r[1], 0, CHAN_Y );
	    micro_max( &r[1], &r[1], &mach->Temps[TEMP_0_I].xyzw[TEMP_0_C] );

	    FETCH( &r[2], 0, CHAN_W );
	    micro_min( &r[2], &r[2], &mach->Temps[TEMP_128_I].xyzw[TEMP_128_C] );
	    micro_max( &r[2], &r[2], &mach->Temps[TEMP_M128_I].xyzw[TEMP_M128_C] );
	    micro_pow( &r[1], &r[1], &r[2] );
	    micro_lt( &r[0], &mach->Temps[TEMP_0_I].xyzw[TEMP_0_C], &r[0], &r[1], &mach->Temps[TEMP_0_I].xyzw[TEMP_0_C] );
	    STORE( &r[0], 0, CHAN_Z );
	 }
      }

      if (IS_CHANNEL_ENABLED( *inst, CHAN_W )) {
	 STORE( &mach->Temps[TEMP_1_I].xyzw[TEMP_1_C], 0, CHAN_W );
      }
      break;

   case TGSI_OPCODE_RCP:
   /* TGSI_OPCODE_RECIP */
      FETCH( &r[0], 0, CHAN_X );
      micro_div( &r[0], &mach->Temps[TEMP_1_I].xyzw[TEMP_1_C], &r[0] );
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
	 STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_RSQ:
   /* TGSI_OPCODE_RECIPSQRT */
      FETCH( &r[0], 0, CHAN_X );
      micro_sqrt( &r[0], &r[0] );
      micro_div( &r[0], &mach->Temps[TEMP_1_I].xyzw[TEMP_1_C], &r[0] );
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
	 STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_EXP:
      FETCH( &r[0], 0, CHAN_X );
      micro_flr( &r[1], &r[0] );  /* r1 = floor(r0) */
      if (IS_CHANNEL_ENABLED( *inst, CHAN_X )) {
         micro_exp2( &r[2], &r[1] );       /* r2 = 2 ^ r1 */
         STORE( &r[2], 0, CHAN_X );        /* store r2 */
      }
      if (IS_CHANNEL_ENABLED( *inst, CHAN_Y )) {
         micro_sub( &r[2], &r[0], &r[1] ); /* r2 = r0 - r1 */
         STORE( &r[2], 0, CHAN_Y );        /* store r2 */
      }
      if (IS_CHANNEL_ENABLED( *inst, CHAN_Z )) {
         micro_exp2( &r[2], &r[0] );       /* r2 = 2 ^ r0 */
         STORE( &r[2], 0, CHAN_Z );        /* store r2 */
      }
      if (IS_CHANNEL_ENABLED( *inst, CHAN_W )) {
         STORE( &mach->Temps[TEMP_1_I].xyzw[TEMP_1_C], 0, CHAN_W );
      }
      break;

   case TGSI_OPCODE_LOG:
      FETCH( &r[0], 0, CHAN_X );
      micro_abs( &r[2], &r[0] );  /* r2 = abs(r0) */
      micro_lg2( &r[1], &r[2] );  /* r1 = lg2(r2) */
      micro_flr( &r[0], &r[1] );  /* r0 = floor(r1) */
      if (IS_CHANNEL_ENABLED( *inst, CHAN_X )) {
         STORE( &r[0], 0, CHAN_X );
      }
      if (IS_CHANNEL_ENABLED( *inst, CHAN_Y )) {
         micro_exp2( &r[0], &r[0] );       /* r0 = 2 ^ r0 */
         micro_div( &r[0], &r[2], &r[0] ); /* r0 = r2 / r0 */
         STORE( &r[0], 0, CHAN_Y );
      }
      if (IS_CHANNEL_ENABLED( *inst, CHAN_Z )) {
         STORE( &r[1], 0, CHAN_Z );
      }
      if (IS_CHANNEL_ENABLED( *inst, CHAN_W )) {
         STORE( &mach->Temps[TEMP_1_I].xyzw[TEMP_1_C], 0, CHAN_W );
      }
      break;

   case TGSI_OPCODE_MUL:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index )
      {
         FETCH(&r[0], 0, chan_index);
         FETCH(&r[1], 1, chan_index);

         micro_mul( &r[0], &r[0], &r[1] );

         STORE(&r[0], 0, chan_index);
      }
      break;

   case TGSI_OPCODE_ADD:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );
         micro_add( &r[0], &r[0], &r[1] );
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_DP3:
   /* TGSI_OPCODE_DOT3 */
      FETCH( &r[0], 0, CHAN_X );
      FETCH( &r[1], 1, CHAN_X );
      micro_mul( &r[0], &r[0], &r[1] );

      FETCH( &r[1], 0, CHAN_Y );
      FETCH( &r[2], 1, CHAN_Y );
      micro_mul( &r[1], &r[1], &r[2] );
      micro_add( &r[0], &r[0], &r[1] );

      FETCH( &r[1], 0, CHAN_Z );
      FETCH( &r[2], 1, CHAN_Z );
      micro_mul( &r[1], &r[1], &r[2] );
      micro_add( &r[0], &r[0], &r[1] );

      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         STORE( &r[0], 0, chan_index );
      }
      break;

    case TGSI_OPCODE_DP4:
    /* TGSI_OPCODE_DOT4 */
       FETCH(&r[0], 0, CHAN_X);
       FETCH(&r[1], 1, CHAN_X);

       micro_mul( &r[0], &r[0], &r[1] );

       FETCH(&r[1], 0, CHAN_Y);
       FETCH(&r[2], 1, CHAN_Y);

       micro_mul( &r[1], &r[1], &r[2] );
       micro_add( &r[0], &r[0], &r[1] );

       FETCH(&r[1], 0, CHAN_Z);
       FETCH(&r[2], 1, CHAN_Z);

       micro_mul( &r[1], &r[1], &r[2] );
       micro_add( &r[0], &r[0], &r[1] );

       FETCH(&r[1], 0, CHAN_W);
       FETCH(&r[2], 1, CHAN_W);

       micro_mul( &r[1], &r[1], &r[2] );
       micro_add( &r[0], &r[0], &r[1] );

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
	 micro_mul( &r[0], &r[0], &r[1] );
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

         /* XXX use micro_min()?? */
         micro_lt( &r[0], &r[0], &r[1], &r[0], &r[1] );

         STORE(&r[0], 0, chan_index);
      }
      break;

   case TGSI_OPCODE_MAX:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH(&r[0], 0, chan_index);
         FETCH(&r[1], 1, chan_index);

         /* XXX use micro_max()?? */
         micro_lt( &r[0], &r[0], &r[1], &r[1], &r[0] );

         STORE(&r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_SLT:
   /* TGSI_OPCODE_SETLT */
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );
         micro_lt( &r[0], &r[0], &r[1], &mach->Temps[TEMP_1_I].xyzw[TEMP_1_C], &mach->Temps[TEMP_0_I].xyzw[TEMP_0_C] );
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_SGE:
   /* TGSI_OPCODE_SETGE */
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );
         micro_ge( &r[0], &r[0], &r[1], &mach->Temps[TEMP_1_I].xyzw[TEMP_1_C], &mach->Temps[TEMP_0_I].xyzw[TEMP_0_C] );
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_MAD:
   /* TGSI_OPCODE_MADD */
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );
         micro_mul( &r[0], &r[0], &r[1] );
         FETCH( &r[1], 2, chan_index );
         micro_add( &r[0], &r[0], &r[1] );
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_SUB:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH(&r[0], 0, chan_index);
         FETCH(&r[1], 1, chan_index);

         micro_sub( &r[0], &r[0], &r[1] );

         STORE(&r[0], 0, chan_index);
      }
      break;

   case TGSI_OPCODE_LERP:
   /* TGSI_OPCODE_LRP */
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH(&r[0], 0, chan_index);
         FETCH(&r[1], 1, chan_index);
         FETCH(&r[2], 2, chan_index);

         micro_sub( &r[1], &r[1], &r[2] );
         micro_mul( &r[0], &r[0], &r[1] );
         micro_add( &r[0], &r[0], &r[2] );

         STORE(&r[0], 0, chan_index);
      }
      break;

   case TGSI_OPCODE_CND:
      assert (0);
      break;

   case TGSI_OPCODE_CND0:
      assert (0);
      break;

   case TGSI_OPCODE_DOT2ADD:
      /* TGSI_OPCODE_DP2A */
      assert (0);
      break;

   case TGSI_OPCODE_INDEX:
      assert (0);
      break;

   case TGSI_OPCODE_NEGATE:
      assert (0);
      break;

   case TGSI_OPCODE_FRAC:
   /* TGSI_OPCODE_FRC */
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         micro_frc( &r[0], &r[0] );
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_CLAMP:
      assert (0);
      break;

   case TGSI_OPCODE_FLOOR:
   /* TGSI_OPCODE_FLR */
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         micro_flr( &r[0], &r[0] );
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_ROUND:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         micro_rnd( &r[0], &r[0] );
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_EXPBASE2:
    /* TGSI_OPCODE_EX2 */
      FETCH(&r[0], 0, CHAN_X);

#if FAST_MATH
      micro_exp2( &r[0], &r[0] );
#else
      micro_pow( &r[0], &mach->Temps[TEMP_2_I].xyzw[TEMP_2_C], &r[0] );
#endif

      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
	 STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_LOGBASE2:
   /* TGSI_OPCODE_LG2 */
      FETCH( &r[0], 0, CHAN_X );
      micro_lg2( &r[0], &r[0] );
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_POWER:
      /* TGSI_OPCODE_POW */
      FETCH(&r[0], 0, CHAN_X);
      FETCH(&r[1], 1, CHAN_X);

      micro_pow( &r[0], &r[0], &r[1] );

      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
	 STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_CROSSPRODUCT:
      /* TGSI_OPCODE_XPD */
      FETCH(&r[0], 0, CHAN_Y);
      FETCH(&r[1], 1, CHAN_Z);

      micro_mul( &r[2], &r[0], &r[1] );

      FETCH(&r[3], 0, CHAN_Z);
      FETCH(&r[4], 1, CHAN_Y);

      micro_mul( &r[5], &r[3], &r[4] );
      micro_sub( &r[2], &r[2], &r[5] );

      if (IS_CHANNEL_ENABLED( *inst, CHAN_X )) {
         STORE( &r[2], 0, CHAN_X );
      }

      FETCH(&r[2], 1, CHAN_X);

      micro_mul( &r[3], &r[3], &r[2] );

      FETCH(&r[5], 0, CHAN_X);

      micro_mul( &r[1], &r[1], &r[5] );
      micro_sub( &r[3], &r[3], &r[1] );

      if (IS_CHANNEL_ENABLED( *inst, CHAN_Y )) {
         STORE( &r[3], 0, CHAN_Y );
      }

      micro_mul( &r[5], &r[5], &r[4] );
      micro_mul( &r[0], &r[0], &r[2] );
      micro_sub( &r[5], &r[5], &r[0] );

      if (IS_CHANNEL_ENABLED( *inst, CHAN_Z )) {
         STORE( &r[5], 0, CHAN_Z );
      }

      if (IS_CHANNEL_ENABLED( *inst, CHAN_W )) {
         STORE( &mach->Temps[TEMP_1_I].xyzw[TEMP_1_C], 0, CHAN_W );
      }
      break;

    case TGSI_OPCODE_MULTIPLYMATRIX:
       assert (0);
       break;

    case TGSI_OPCODE_ABS:
       FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
          FETCH(&r[0], 0, chan_index);

          micro_abs( &r[0], &r[0] );

          STORE(&r[0], 0, chan_index);
       }
       break;

   case TGSI_OPCODE_RCC:
      assert (0);
      break;

   case TGSI_OPCODE_DPH:
      FETCH(&r[0], 0, CHAN_X);
      FETCH(&r[1], 1, CHAN_X);

      micro_mul( &r[0], &r[0], &r[1] );

      FETCH(&r[1], 0, CHAN_Y);
      FETCH(&r[2], 1, CHAN_Y);

      micro_mul( &r[1], &r[1], &r[2] );
      micro_add( &r[0], &r[0], &r[1] );

      FETCH(&r[1], 0, CHAN_Z);
      FETCH(&r[2], 1, CHAN_Z);

      micro_mul( &r[1], &r[1], &r[2] );
      micro_add( &r[0], &r[0], &r[1] );

      FETCH(&r[1], 1, CHAN_W);

      micro_add( &r[0], &r[0], &r[1] );

      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
	 STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_COS:
      FETCH(&r[0], 0, CHAN_X);

      micro_cos( &r[0], &r[0] );

      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
	 STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_DDX:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         micro_ddx( &r[0], &r[0] );
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_DDY:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         micro_ddy( &r[0], &r[0] );
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
      assert (0);
      break;

   case TGSI_OPCODE_PK2US:
      assert (0);
      break;

   case TGSI_OPCODE_PK4B:
      assert (0);
      break;

   case TGSI_OPCODE_PK4UB:
      assert (0);
      break;

   case TGSI_OPCODE_RFL:
      assert (0);
      break;

   case TGSI_OPCODE_SEQ:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );
         micro_eq( &r[0], &r[0], &r[1],
                   &mach->Temps[TEMP_1_I].xyzw[TEMP_1_C],
                   &mach->Temps[TEMP_0_I].xyzw[TEMP_0_C] );
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_SFL:
      assert (0);
      break;

   case TGSI_OPCODE_SGT:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );
         micro_le( &r[0], &r[0], &r[1], &mach->Temps[TEMP_0_I].xyzw[TEMP_0_C], &mach->Temps[TEMP_1_I].xyzw[TEMP_1_C] );
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_SIN:
      FETCH( &r[0], 0, CHAN_X );
      micro_sin( &r[0], &r[0] );
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_SLE:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );
         micro_le( &r[0], &r[0], &r[1], &mach->Temps[TEMP_1_I].xyzw[TEMP_1_C], &mach->Temps[TEMP_0_I].xyzw[TEMP_0_C] );
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_SNE:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );
         micro_eq( &r[0], &r[0], &r[1], &mach->Temps[TEMP_0_I].xyzw[TEMP_0_C], &mach->Temps[TEMP_1_I].xyzw[TEMP_1_C] );
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_STR:
      assert (0);
      break;

   case TGSI_OPCODE_TEX:
      /* simple texture lookup */
      /* src[0] = texcoord */
      /* src[1] = sampler unit */
      exec_tex(mach, inst, FALSE, FALSE);
      break;

   case TGSI_OPCODE_TXB:
      /* Texture lookup with lod bias */
      /* src[0] = texcoord (src[0].w = LOD bias) */
      /* src[1] = sampler unit */
      exec_tex(mach, inst, TRUE, FALSE);
      break;

   case TGSI_OPCODE_TXD:
      /* Texture lookup with explict partial derivatives */
      /* src[0] = texcoord */
      /* src[1] = d[strq]/dx */
      /* src[2] = d[strq]/dy */
      /* src[3] = sampler unit */
      assert (0);
      break;

   case TGSI_OPCODE_TXL:
      /* Texture lookup with explit LOD */
      /* src[0] = texcoord (src[0].w = LOD) */
      /* src[1] = sampler unit */
      exec_tex(mach, inst, TRUE, FALSE);
      break;

   case TGSI_OPCODE_TXP:
      /* Texture lookup with projection */
      /* src[0] = texcoord (src[0].w = projection) */
      /* src[1] = sampler unit */
      exec_tex(mach, inst, FALSE, TRUE);
      break;

   case TGSI_OPCODE_UP2H:
      assert (0);
      break;

   case TGSI_OPCODE_UP2US:
      assert (0);
      break;

   case TGSI_OPCODE_UP4B:
      assert (0);
      break;

   case TGSI_OPCODE_UP4UB:
      assert (0);
      break;

   case TGSI_OPCODE_X2D:
      assert (0);
      break;

   case TGSI_OPCODE_ARA:
      assert (0);
      break;

   case TGSI_OPCODE_ARR:
      assert (0);
      break;

   case TGSI_OPCODE_BRA:
      assert (0);
      break;

   case TGSI_OPCODE_CAL:
      /* skip the call if no execution channels are enabled */
      if (mach->ExecMask) {
         /* do the call */

         /* push the Cond, Loop, Cont stacks */
         assert(mach->CondStackTop < TGSI_EXEC_MAX_COND_NESTING);
         mach->CondStack[mach->CondStackTop++] = mach->CondMask;
         assert(mach->LoopStackTop < TGSI_EXEC_MAX_LOOP_NESTING);
         mach->LoopStack[mach->LoopStackTop++] = mach->LoopMask;
         assert(mach->ContStackTop < TGSI_EXEC_MAX_LOOP_NESTING);
         mach->ContStack[mach->ContStackTop++] = mach->ContMask;

         assert(mach->FuncStackTop < TGSI_EXEC_MAX_CALL_NESTING);
         mach->FuncStack[mach->FuncStackTop++] = mach->FuncMask;

         /* note that PC was already incremented above */
         mach->CallStack[mach->CallStackTop++] = *pc;
         *pc = inst->InstructionExtLabel.Label;
      }
      break;

   case TGSI_OPCODE_RET:
      mach->FuncMask &= ~mach->ExecMask;
      UPDATE_EXEC_MASK(mach);

      if (mach->FuncMask == 0x0) {
         /* really return now (otherwise, keep executing */

         if (mach->CallStackTop == 0) {
            /* returning from main() */
            *pc = -1;
            return;
         }
         *pc = mach->CallStack[--mach->CallStackTop];

         /* pop the Cond, Loop, Cont stacks */
         assert(mach->CondStackTop > 0);
         mach->CondMask = mach->CondStack[--mach->CondStackTop];
         assert(mach->LoopStackTop > 0);
         mach->LoopMask = mach->LoopStack[--mach->LoopStackTop];
         assert(mach->ContStackTop > 0);
         mach->ContMask = mach->ContStack[--mach->ContStackTop];
         assert(mach->FuncStackTop > 0);
         mach->FuncMask = mach->FuncStack[--mach->FuncStackTop];

         UPDATE_EXEC_MASK(mach);
      }
      break;

   case TGSI_OPCODE_SSG:
      assert (0);
      break;

   case TGSI_OPCODE_CMP:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH(&r[0], 0, chan_index);
         FETCH(&r[1], 1, chan_index);
         FETCH(&r[2], 2, chan_index);

         micro_lt( &r[0], &r[0], &mach->Temps[TEMP_0_I].xyzw[TEMP_0_C], &r[1], &r[2] );

         STORE(&r[0], 0, chan_index);
      }
      break;

   case TGSI_OPCODE_SCS:
      if( IS_CHANNEL_ENABLED( *inst, CHAN_X ) || IS_CHANNEL_ENABLED( *inst, CHAN_Y ) ) {
         FETCH( &r[0], 0, CHAN_X );
      }
      if( IS_CHANNEL_ENABLED( *inst, CHAN_X ) ) {
         micro_cos( &r[1], &r[0] );
         STORE( &r[1], 0, CHAN_X );
      }
      if( IS_CHANNEL_ENABLED( *inst, CHAN_Y ) ) {
         micro_sin( &r[1], &r[0] );
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
      assert (0);
      break;

   case TGSI_OPCODE_DIV:
      assert( 0 );
      break;

   case TGSI_OPCODE_DP2:
      FETCH( &r[0], 0, CHAN_X );
      FETCH( &r[1], 1, CHAN_X );
      micro_mul( &r[0], &r[0], &r[1] );

      FETCH( &r[1], 0, CHAN_Y );
      FETCH( &r[2], 1, CHAN_Y );
      micro_mul( &r[1], &r[1], &r[2] );
      micro_add( &r[0], &r[0], &r[1] );

      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_IF:
      /* push CondMask */
      assert(mach->CondStackTop < TGSI_EXEC_MAX_COND_NESTING);
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
         assert(mach->CondStackTop > 0);
         prevMask = mach->CondStack[mach->CondStackTop - 1];
         mach->CondMask = ~mach->CondMask & prevMask;
         UPDATE_EXEC_MASK(mach);
         /* Todo: If CondMask==0, jump to ENDIF */
      }
      break;

   case TGSI_OPCODE_ENDIF:
      /* pop CondMask */
      assert(mach->CondStackTop > 0);
      mach->CondMask = mach->CondStack[--mach->CondStackTop];
      UPDATE_EXEC_MASK(mach);
      break;

   case TGSI_OPCODE_END:
      /* halt execution */
      *pc = -1;
      break;

   case TGSI_OPCODE_REP:
      assert (0);
      break;

   case TGSI_OPCODE_ENDREP:
       assert (0);
       break;

   case TGSI_OPCODE_PUSHA:
      assert (0);
      break;

   case TGSI_OPCODE_POPA:
      assert (0);
      break;

   case TGSI_OPCODE_CEIL:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         micro_ceil( &r[0], &r[0] );
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_I2F:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         micro_i2f( &r[0], &r[0] );
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_NOT:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         micro_not( &r[0], &r[0] );
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_TRUNC:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         micro_trunc( &r[0], &r[0] );
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_SHL:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );
         micro_shl( &r[0], &r[0], &r[1] );
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_SHR:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );
         micro_ishr( &r[0], &r[0], &r[1] );
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_AND:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );
         micro_and( &r[0], &r[0], &r[1] );
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_OR:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );
         micro_or( &r[0], &r[0], &r[1] );
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_MOD:
      assert (0);
      break;

   case TGSI_OPCODE_XOR:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );
         micro_xor( &r[0], &r[0], &r[1] );
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_SAD:
      assert (0);
      break;

   case TGSI_OPCODE_TXF:
      assert (0);
      break;

   case TGSI_OPCODE_TXQ:
      assert (0);
      break;

   case TGSI_OPCODE_EMIT:
      mach->Temps[TEMP_OUTPUT_I].xyzw[TEMP_OUTPUT_C].u[0] += 16;
      mach->Primitives[mach->Temps[TEMP_PRIMITIVE_I].xyzw[TEMP_PRIMITIVE_C].u[0]]++;
      break;

   case TGSI_OPCODE_ENDPRIM:
      mach->Temps[TEMP_PRIMITIVE_I].xyzw[TEMP_PRIMITIVE_C].u[0]++;
      mach->Primitives[mach->Temps[TEMP_PRIMITIVE_I].xyzw[TEMP_PRIMITIVE_C].u[0]] = 0;
      break;

   case TGSI_OPCODE_LOOP:
      /* fall-through (for now) */
   case TGSI_OPCODE_BGNLOOP2:
      /* push LoopMask and ContMasks */
      assert(mach->LoopStackTop < TGSI_EXEC_MAX_LOOP_NESTING);
      mach->LoopStack[mach->LoopStackTop++] = mach->LoopMask;
      assert(mach->ContStackTop < TGSI_EXEC_MAX_LOOP_NESTING);
      mach->ContStack[mach->ContStackTop++] = mach->ContMask;
      break;

   case TGSI_OPCODE_ENDLOOP:
      /* fall-through (for now at least) */
   case TGSI_OPCODE_ENDLOOP2:
      /* Restore ContMask, but don't pop */
      assert(mach->ContStackTop > 0);
      mach->ContMask = mach->ContStack[mach->ContStackTop - 1];
      UPDATE_EXEC_MASK(mach);
      if (mach->ExecMask) {
         /* repeat loop: jump to instruction just past BGNLOOP */
         *pc = inst->InstructionExtLabel.Label + 1;
      }
      else {
         /* exit loop: pop LoopMask */
         assert(mach->LoopStackTop > 0);
         mach->LoopMask = mach->LoopStack[--mach->LoopStackTop];
         /* pop ContMask */
         assert(mach->ContStackTop > 0);
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

   case TGSI_OPCODE_NOISE1:
      assert( 0 );
      break;

   case TGSI_OPCODE_NOISE2:
      assert( 0 );
      break;

   case TGSI_OPCODE_NOISE3:
      assert( 0 );
      break;

   case TGSI_OPCODE_NOISE4:
      assert( 0 );
      break;

   case TGSI_OPCODE_NOP:
      break;

   default:
      assert( 0 );
   }
}


/**
 * Run TGSI interpreter.
 * \return bitmask of "alive" quad components
 */
uint
tgsi_exec_machine_run( struct tgsi_exec_machine *mach )
{
   uint i;
   int pc = 0;

   mach->CondMask = 0xf;
   mach->LoopMask = 0xf;
   mach->ContMask = 0xf;
   mach->FuncMask = 0xf;
   mach->ExecMask = 0xf;

   mach->CondStackTop = 0; /* temporarily subvert this assertion */
   assert(mach->CondStackTop == 0);
   assert(mach->LoopStackTop == 0);
   assert(mach->ContStackTop == 0);
   assert(mach->CallStackTop == 0);

   mach->Temps[TEMP_KILMASK_I].xyzw[TEMP_KILMASK_C].u[0] = 0;
   mach->Temps[TEMP_OUTPUT_I].xyzw[TEMP_OUTPUT_C].u[0] = 0;

   if( mach->Processor == TGSI_PROCESSOR_GEOMETRY ) {
      mach->Temps[TEMP_PRIMITIVE_I].xyzw[TEMP_PRIMITIVE_C].u[0] = 0;
      mach->Primitives[0] = 0;
   }

   for (i = 0; i < QUAD_SIZE; i++) {
      mach->Temps[TEMP_CC_I].xyzw[TEMP_CC_C].u[i] =
         (TGSI_EXEC_CC_EQ << TGSI_EXEC_CC_X_SHIFT) |
         (TGSI_EXEC_CC_EQ << TGSI_EXEC_CC_Y_SHIFT) |
         (TGSI_EXEC_CC_EQ << TGSI_EXEC_CC_Z_SHIFT) |
         (TGSI_EXEC_CC_EQ << TGSI_EXEC_CC_W_SHIFT);
   }

   /* execute declarations (interpolants) */
   for (i = 0; i < mach->NumDeclarations; i++) {
      exec_declaration( mach, mach->Declarations+i );
   }

   /* execute instructions, until pc is set to -1 */
   while (pc != -1) {
      assert(pc < (int) mach->NumInstructions);
      exec_instruction( mach, mach->Instructions + pc, &pc );
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


