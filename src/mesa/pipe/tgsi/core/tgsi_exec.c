#include "tgsi_platform.h"
#include "tgsi_core.h"

#define MESA 1
#if MESA
#include "main/context.h"
#include "main/macros.h"
#include "main/colormac.h"
#include "swrast/swrast.h"
#include "swrast/s_context.h"
#endif

#define TILE_BOTTOM_LEFT  0
#define TILE_BOTTOM_RIGHT 1
#define TILE_TOP_LEFT     2
#define TILE_TOP_RIGHT    3

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
   ((INST).FullDstRegisters[0].DstRegister.WriteMask & (1 << (CHAN)))

#define IS_CHANNEL_ENABLED2(INST, CHAN)\
   ((INST).FullDstRegisters[1].DstRegister.WriteMask & (1 << (CHAN)))

#define FOR_EACH_ENABLED_CHANNEL(INST, CHAN)\
   FOR_EACH_CHANNEL( CHAN )\
      if (IS_CHANNEL_ENABLED( INST, CHAN ))

#define FOR_EACH_ENABLED_CHANNEL2(INST, CHAN)\
   FOR_EACH_CHANNEL( CHAN )\
      if (IS_CHANNEL_ENABLED2( INST, CHAN ))

#define CHAN_X  0
#define CHAN_Y  1
#define CHAN_Z  2
#define CHAN_W  3

void
tgsi_exec_machine_init(
   struct tgsi_exec_machine *mach,
   struct tgsi_token *tokens )
{
   GLuint i, k;
   struct tgsi_parse_context parse;

   mach->Tokens = tokens;

   k = tgsi_parse_init (&parse, mach->Tokens);
   if (k != TGSI_PARSE_OK) {
      printf("Problem parsing!\n");
      return;
   }

   mach->Processor = parse.FullHeader.Processor.Processor;
   tgsi_parse_free (&parse);

   mach->Temps = (struct tgsi_exec_vector *) tgsi_align_128bit( mach->_Temps);
   mach->Addrs = &mach->Temps[TGSI_EXEC_NUM_TEMPS];

#if XXX_SSE
    tgsi_emit_sse (tokens,
                   &mach->Function);
#endif

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
   }
}

void
tgsi_exec_prepare(
   struct tgsi_exec_machine *mach,
   struct tgsi_exec_labels *labels )
{
   struct tgsi_parse_context parse;
   GLuint k;

   mach->ImmLimit = 0;
   labels->count = 0;

   k = tgsi_parse_init( &parse, mach->Tokens );
   if (k != TGSI_PARSE_OK) {
      printf("Problem parsing!\n");
      return;
   }

   while( !tgsi_parse_end_of_tokens( &parse ) ) {
      GLuint pointer = parse.Position;
      GLuint i;
      tgsi_parse_token( &parse );
      switch( parse.FullToken.Token.Type ) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         break;
      case TGSI_TOKEN_TYPE_IMMEDIATE:
         assert( (parse.FullToken.FullImmediate.Immediate.Size - 1) % 4 == 0 );
         assert( mach->ImmLimit + (parse.FullToken.FullImmediate.Immediate.Size - 1) / 4 <= 256 );
         for( i = 0; i < parse.FullToken.FullImmediate.Immediate.Size - 1; i++ ) {
            mach->Imms[mach->ImmLimit + i / 4][i % 4] = parse.FullToken.FullImmediate.u.ImmediateFloat32[i].Float;
         }
         mach->ImmLimit += (parse.FullToken.FullImmediate.Immediate.Size - 1) / 4;
         break;
      case TGSI_TOKEN_TYPE_INSTRUCTION:
         if( parse.FullToken.FullInstruction.InstructionExtLabel.Label &&
             parse.FullToken.FullInstruction.InstructionExtLabel.Target ) {
            assert( labels->count < 128 );
            labels->labels[labels->count][0] = parse.FullToken.FullInstruction.InstructionExtLabel.Label;
            labels->labels[labels->count][1] = pointer;
            labels->count++;
         }
         break;
      default:
         assert( 0 );
      }
   }
   tgsi_parse_free (&parse);
}

void
tgsi_exec_machine_run(
   struct tgsi_exec_machine *mach )
{
   struct tgsi_exec_labels labels;

   tgsi_exec_prepare( mach, &labels );
   tgsi_exec_machine_run2( mach, &labels );
}

static void
micro_abs(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->f[0] = (GLfloat) fabs( (GLdouble) src->f[0] );
   dst->f[1] = (GLfloat) fabs( (GLdouble) src->f[1] );
   dst->f[2] = (GLfloat) fabs( (GLdouble) src->f[2] );
   dst->f[3] = (GLfloat) fabs( (GLdouble) src->f[3] );
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
   dst->f[0] = (GLfloat) ceil( (GLdouble) src->f[0] );
   dst->f[1] = (GLfloat) ceil( (GLdouble) src->f[1] );
   dst->f[2] = (GLfloat) ceil( (GLdouble) src->f[2] );
   dst->f[3] = (GLfloat) ceil( (GLdouble) src->f[3] );
}

static void
micro_cos(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->f[0] = (GLfloat) cos( (GLdouble) src->f[0] );
   dst->f[1] = (GLfloat) cos( (GLdouble) src->f[1] );
   dst->f[2] = (GLfloat) cos( (GLdouble) src->f[2] );
   dst->f[3] = (GLfloat) cos( (GLdouble) src->f[3] );
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
   dst->f[0] = src0->f[0] / src1->f[0];
   dst->f[1] = src0->f[1] / src1->f[1];
   dst->f[2] = src0->f[2] / src1->f[2];
   dst->f[3] = src0->f[3] / src1->f[3];
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
   const union tgsi_exec_channel *src )
{
   dst->f[0] = (GLfloat) pow( 2.0, (GLdouble) src->f[0] );
   dst->f[1] = (GLfloat) pow( 2.0, (GLdouble) src->f[1] );
   dst->f[2] = (GLfloat) pow( 2.0, (GLdouble) src->f[2] );
   dst->f[3] = (GLfloat) pow( 2.0, (GLdouble) src->f[3] );
}

static void
micro_f2it(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->i[0] = (GLint) src->f[0];
   dst->i[1] = (GLint) src->f[1];
   dst->i[2] = (GLint) src->f[2];
   dst->i[3] = (GLint) src->f[3];
}

static void
micro_f2ut(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->u[0] = (GLuint) src->f[0];
   dst->u[1] = (GLuint) src->f[1];
   dst->u[2] = (GLuint) src->f[2];
   dst->u[3] = (GLuint) src->f[3];
}

static void
micro_flr(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->f[0] = (GLfloat) floor( (GLdouble) src->f[0] );
   dst->f[1] = (GLfloat) floor( (GLdouble) src->f[1] );
   dst->f[2] = (GLfloat) floor( (GLdouble) src->f[2] );
   dst->f[3] = (GLfloat) floor( (GLdouble) src->f[3] );
}

static void
micro_frc(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->f[0] = src->f[0] - (GLfloat) floor( (GLdouble) src->f[0] );
   dst->f[1] = src->f[1] - (GLfloat) floor( (GLdouble) src->f[1] );
   dst->f[2] = src->f[2] - (GLfloat) floor( (GLdouble) src->f[2] );
   dst->f[3] = src->f[3] - (GLfloat) floor( (GLdouble) src->f[3] );
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
   dst->f[0] = (GLfloat) src->i[0];
   dst->f[1] = (GLfloat) src->i[1];
   dst->f[2] = (GLfloat) src->i[2];
   dst->f[3] = (GLfloat) src->i[3];
}

static void
micro_lg2(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->f[0] = (GLfloat) log( (GLdouble) src->f[0] ) * 1.442695f;
   dst->f[1] = (GLfloat) log( (GLdouble) src->f[1] ) * 1.442695f;
   dst->f[2] = (GLfloat) log( (GLdouble) src->f[2] ) * 1.442695f;
   dst->f[3] = (GLfloat) log( (GLdouble) src->f[3] ) * 1.442695f;
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
   dst->f[0] = (GLfloat) pow( (GLdouble) src0->f[0], (GLdouble) src1->f[0] );
   dst->f[1] = (GLfloat) pow( (GLdouble) src0->f[1], (GLdouble) src1->f[1] );
   dst->f[2] = (GLfloat) pow( (GLdouble) src0->f[2], (GLdouble) src1->f[2] );
   dst->f[3] = (GLfloat) pow( (GLdouble) src0->f[3], (GLdouble) src1->f[3] );
}

static void
micro_rnd(
   union tgsi_exec_channel *dst,
   const union tgsi_exec_channel *src )
{
   dst->f[0] = (GLfloat) floor( (GLdouble) (src->f[0] + 0.5f) );
   dst->f[1] = (GLfloat) floor( (GLdouble) (src->f[1] + 0.5f) );
   dst->f[2] = (GLfloat) floor( (GLdouble) (src->f[2] + 0.5f) );
   dst->f[3] = (GLfloat) floor( (GLdouble) (src->f[3] + 0.5f) );
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
   dst->f[0] = (GLfloat) sin( (GLdouble) src->f[0] );
   dst->f[1] = (GLfloat) sin( (GLdouble) src->f[1] );
   dst->f[2] = (GLfloat) sin( (GLdouble) src->f[2] );
   dst->f[3] = (GLfloat) sin( (GLdouble) src->f[3] );
}

static void
micro_sqrt( union tgsi_exec_channel *dst,
            const union tgsi_exec_channel *src )
{
   dst->f[0] = (GLfloat) sqrt( (GLdouble) src->f[0] );
   dst->f[1] = (GLfloat) sqrt( (GLdouble) src->f[1] );
   dst->f[2] = (GLfloat) sqrt( (GLdouble) src->f[2] );
   dst->f[3] = (GLfloat) sqrt( (GLdouble) src->f[3] );
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
   dst->f[0] = (GLfloat) src->u[0];
   dst->f[1] = (GLfloat) src->u[1];
   dst->f[2] = (GLfloat) src->u[2];
   dst->f[3] = (GLfloat) src->u[3];
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
   const GLuint file,
   const GLuint swizzle,
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
         chan->u[0] = mach->Temps[index->i[0]].xyzw[swizzle].u[0];
         chan->u[1] = mach->Temps[index->i[1]].xyzw[swizzle].u[1];
         chan->u[2] = mach->Temps[index->i[2]].xyzw[swizzle].u[2];
         chan->u[3] = mach->Temps[index->i[3]].xyzw[swizzle].u[3];
         break;

      case TGSI_FILE_IMMEDIATE:
         assert( index->i[0] < (GLint) mach->ImmLimit );
         chan->f[0] = mach->Imms[index->i[0]][swizzle];
         assert( index->i[1] < (GLint) mach->ImmLimit );
         chan->f[1] = mach->Imms[index->i[1]][swizzle];
         assert( index->i[2] < (GLint) mach->ImmLimit );
         chan->f[2] = mach->Imms[index->i[2]][swizzle];
         assert( index->i[3] < (GLint) mach->ImmLimit );
         chan->f[3] = mach->Imms[index->i[3]][swizzle];
         break;

      case TGSI_FILE_ADDRESS:
         chan->u[0] = mach->Addrs[index->i[0]].xyzw[swizzle].u[0];
         chan->u[1] = mach->Addrs[index->i[1]].xyzw[swizzle].u[1];
         chan->u[2] = mach->Addrs[index->i[2]].xyzw[swizzle].u[2];
         chan->u[3] = mach->Addrs[index->i[3]].xyzw[swizzle].u[3];
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
   const GLuint chan_index )
{
   union tgsi_exec_channel index;
   GLuint swizzle;

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
}

static void
store_dest(
   struct tgsi_exec_machine *mach,
   const union tgsi_exec_channel *chan,
   const struct tgsi_full_dst_register *reg,
   const struct tgsi_full_instruction *inst,
   GLuint chan_index )
{
   union tgsi_exec_channel *dst;

   switch( reg->DstRegister.File ) {
   case TGSI_FILE_NULL:
      return;

   case TGSI_FILE_OUTPUT:
      dst = &mach->Outputs[mach->Temps[TEMP_OUTPUT_I].xyzw[TEMP_OUTPUT_C].u[0] + reg->DstRegister.Index].xyzw[chan_index];
      break;

   case TGSI_FILE_TEMPORARY:
      dst = &mach->Temps[reg->DstRegister.Index].xyzw[chan_index];
      break;

   case TGSI_FILE_ADDRESS:
      dst = &mach->Addrs[reg->DstRegister.Index].xyzw[chan_index];
      break;

   default:
      assert( 0 );
   }

   switch (inst->Instruction.Saturate)
   {
   case TGSI_SAT_NONE:
      *dst = *chan;
      break;

   case TGSI_SAT_ZERO_ONE:
      micro_lt( dst, chan, &mach->Temps[TEMP_0_I].xyzw[TEMP_0_C], &mach->Temps[TEMP_0_I].xyzw[TEMP_0_C], chan );
      micro_lt( dst, chan, &mach->Temps[TEMP_1_I].xyzw[TEMP_1_C], chan, &mach->Temps[TEMP_1_I].xyzw[TEMP_1_C] );
      break;

   case TGSI_SAT_MINUS_PLUS_ONE:
      assert( 0 );
      break;

   default:
      assert( 0 );
   }
}

#define FETCH(VAL,INDEX,CHAN)\
    fetch_source (mach, VAL, &inst->FullSrcRegisters[INDEX], CHAN)

#define STORE(VAL,INDEX,CHAN)\
    store_dest (mach, VAL, &inst->FullDstRegisters[INDEX], inst, CHAN)

static void
exec_kil (struct tgsi_exec_machine *mach,
          const struct tgsi_full_instruction *inst)
{
    GLuint uniquemask;
    GLuint chan_index;
    GLuint kilmask = 0;
    union tgsi_exec_channel r[1];

    /* This mask stores component bits that were already tested. Note that
     * we test if the value is less than zero, so 1.0 and 0.0 need not to be
     * tested. */
    uniquemask = (1 << TGSI_EXTSWIZZLE_ZERO) | (1 << TGSI_EXTSWIZZLE_ONE);

    for (chan_index = 0; chan_index < 4; chan_index++)
    {
        GLuint swizzle;
        GLuint i;

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
                kilmask |= 1 << (i * 4);
    }

    mach->Temps[TEMP_KILMASK_I].xyzw[TEMP_KILMASK_C].u[0] |= kilmask;
}

#if MESA
/*
 * Fetch a texel using S texture coordinate.
 */
static void
fetch_texel_1d( GLcontext *ctx,
                struct tgsi_sampler_state *sampler,
                const union tgsi_exec_channel *s,
                GLuint unit,
                union tgsi_exec_channel *r,
                union tgsi_exec_channel *g,
                union tgsi_exec_channel *b,
                union tgsi_exec_channel *a )
{
    SWcontext *swrast = SWRAST_CONTEXT(ctx);
    GLuint fragment_index;
    GLfloat stpq[4][4];
    GLfloat lambdas[4];
    GLchan rgba[4][4];

    for (fragment_index = 0; fragment_index < 4; fragment_index++)
    {
        stpq[fragment_index][0] = s->f[fragment_index];
    }

    if (sampler->NeedLambda)
    {
        GLfloat dsdx = s->f[TILE_BOTTOM_RIGHT] - s->f[TILE_BOTTOM_LEFT];
        GLfloat dsdy = s->f[TILE_TOP_LEFT]     - s->f[TILE_BOTTOM_LEFT];

        GLfloat rho, lambda;

        dsdx = FABSF(dsdx);
        dsdy = FABSF(dsdy);

        rho = MAX2(dsdx, dsdy) * sampler->ImageWidth;

        lambda = LOG2(rho);

        if (sampler->NeedLodBias)
            lambda += sampler->LodBias;

        if (sampler->NeedLambdaClamp)
            lambda = CLAMP(lambda, sampler->MinLod, sampler->MaxLod);

        /* XXX: Use the same lambda value throughout the tile.  Could
         * end up with four unique values by recalculating partial
         * derivs in the other row and column, and calculating lambda
         * using the dx and dy values appropriate for each fragment in
         * the tile.
         */
        lambdas[0] =
        lambdas[1] =
        lambdas[2] = 
        lambdas[3] = lambda;
    }

    if (!swrast->TextureSample[unit]) {
       _swrast_update_texture_samplers(ctx);
    }

    /* XXX use a float-valued TextureSample routine here!!! */
    swrast->TextureSample[unit] (ctx,
                                 ctx->Texture.Unit[unit]._Current,
                                 4,
                                 (const GLfloat (*)[4])stpq,
                                 lambdas,
                                 rgba);

    for (fragment_index = 0; fragment_index < 4; fragment_index++)
    {
        r->f[fragment_index] = CHAN_TO_FLOAT(rgba[fragment_index][0]);
        g->f[fragment_index] = CHAN_TO_FLOAT(rgba[fragment_index][1]);
        b->f[fragment_index] = CHAN_TO_FLOAT(rgba[fragment_index][2]);
        a->f[fragment_index] = CHAN_TO_FLOAT(rgba[fragment_index][3]);
    }
}

/*
 * Fetch a texel using ST texture coordinates.
 */
static void
fetch_texel_2d( GLcontext *ctx,
                struct tgsi_sampler_state *sampler,
                const union tgsi_exec_channel *s,
                const union tgsi_exec_channel *t,
                GLuint unit,
                union tgsi_exec_channel *r,
                union tgsi_exec_channel *g,
                union tgsi_exec_channel *b,
                union tgsi_exec_channel *a )
{
   SWcontext *swrast = SWRAST_CONTEXT( ctx );
   GLuint fragment_index;
   GLfloat stpq[4][4];
   GLfloat lambdas[4];
   GLchan rgba[4][4];

   for (fragment_index = 0; fragment_index < 4; fragment_index++) {
      stpq[fragment_index][0] = s->f[fragment_index];
      stpq[fragment_index][1] = t->f[fragment_index];
   }

   if (sampler->NeedLambda) {
      GLfloat dsdx = s->f[TILE_BOTTOM_RIGHT] - s->f[TILE_BOTTOM_LEFT];
      GLfloat dsdy = s->f[TILE_TOP_LEFT]     - s->f[TILE_BOTTOM_LEFT];

      GLfloat dtdx = t->f[TILE_BOTTOM_RIGHT] - t->f[TILE_BOTTOM_LEFT];
      GLfloat dtdy = t->f[TILE_TOP_LEFT]     - t->f[TILE_BOTTOM_LEFT];

      GLfloat maxU, maxV, rho, lambda;

      dsdx = FABSF( dsdx );
      dsdy = FABSF( dsdy );
      dtdx = FABSF( dtdx );
      dtdy = FABSF( dtdy );

      maxU = MAX2( dsdx, dsdy ) * sampler->ImageWidth;
      maxV = MAX2( dtdx, dtdy ) * sampler->ImageHeight;

      rho = MAX2( maxU, maxV );

      lambda = LOG2( rho );

      if (sampler->NeedLodBias)
	 lambda += sampler->LodBias;

      if (sampler->NeedLambdaClamp)
         lambda = CLAMP(
		     lambda,
		     sampler->MinLod,
		     sampler->MaxLod );

      /* XXX: Use the same lambda value throughout the tile.  Could
	 * end up with four unique values by recalculating partial
	 * derivs in the other row and column, and calculating lambda
	 * using the dx and dy values appropriate for each fragment in
	 * the tile.
	 */
      lambdas[0] =
      lambdas[1] =
      lambdas[2] = 
      lambdas[3] = lambda;
   }

   if (!swrast->TextureSample[unit]) {
      _swrast_update_texture_samplers(ctx);
   }

   /* XXX use a float-valued TextureSample routine here!!! */
   swrast->TextureSample[unit](
      ctx,
      ctx->Texture.Unit[unit]._Current,
      4,
      (const GLfloat (*)[4]) stpq,
      lambdas,
      rgba );

   for (fragment_index = 0; fragment_index < 4; fragment_index++) {
      r->f[fragment_index] = CHAN_TO_FLOAT( rgba[fragment_index][0] );
      g->f[fragment_index] = CHAN_TO_FLOAT( rgba[fragment_index][1] );
      b->f[fragment_index] = CHAN_TO_FLOAT( rgba[fragment_index][2] );
      a->f[fragment_index] = CHAN_TO_FLOAT( rgba[fragment_index][3] );
   }
}

/*
 * Fetch a texel using STR texture coordinates.
 */
static void
fetch_texel_3d( GLcontext *ctx,
                struct tgsi_sampler_state *sampler,
                const union tgsi_exec_channel *s,
                const union tgsi_exec_channel *t,
                const union tgsi_exec_channel *p,
                GLuint unit,
                union tgsi_exec_channel *r,
                union tgsi_exec_channel *g,
                union tgsi_exec_channel *b,
                union tgsi_exec_channel *a )
{
    SWcontext *swrast = SWRAST_CONTEXT(ctx);
    GLuint fragment_index;
    GLfloat stpq[4][4];
    GLfloat lambdas[4];
    GLchan rgba[4][4];

    for (fragment_index = 0; fragment_index < 4; fragment_index++)
    {
        stpq[fragment_index][0] = s->f[fragment_index];
        stpq[fragment_index][1] = t->f[fragment_index];
        stpq[fragment_index][2] = p->f[fragment_index];
    }

    if (sampler->NeedLambda)
    {
        GLfloat dsdx = s->f[TILE_BOTTOM_RIGHT] - s->f[TILE_BOTTOM_LEFT];
        GLfloat dsdy = s->f[TILE_TOP_LEFT]     - s->f[TILE_BOTTOM_LEFT];

        GLfloat dtdx = t->f[TILE_BOTTOM_RIGHT] - t->f[TILE_BOTTOM_LEFT];
        GLfloat dtdy = t->f[TILE_TOP_LEFT]     - t->f[TILE_BOTTOM_LEFT];

        GLfloat dpdx = p->f[TILE_BOTTOM_RIGHT] - p->f[TILE_BOTTOM_LEFT];
        GLfloat dpdy = p->f[TILE_TOP_LEFT]     - p->f[TILE_BOTTOM_LEFT];

        GLfloat maxU, maxV, maxW, rho, lambda;

        dsdx = FABSF(dsdx);
        dsdy = FABSF(dsdy);
        dtdx = FABSF(dtdx);
        dtdy = FABSF(dtdy);
        dpdx = FABSF(dpdx);
        dpdy = FABSF(dpdy);

        maxU = MAX2(dsdx, dsdy) * sampler->ImageWidth;
        maxV = MAX2(dtdx, dtdy) * sampler->ImageHeight;
        maxW = MAX2(dpdx, dpdy) * sampler->ImageDepth;

        rho = MAX2(maxU, MAX2(maxV, maxW));

        lambda = LOG2(rho);

        if (sampler->NeedLodBias)
            lambda += sampler->LodBias;

        if (sampler->NeedLambdaClamp)
            lambda = CLAMP(lambda, sampler->MinLod, sampler->MaxLod);

        /* XXX: Use the same lambda value throughout the tile.  Could
         * end up with four unique values by recalculating partial
         * derivs in the other row and column, and calculating lambda
         * using the dx and dy values appropriate for each fragment in
         * the tile.
         */
        lambdas[0] =
        lambdas[1] =
        lambdas[2] = 
        lambdas[3] = lambda;
    }

    if (!swrast->TextureSample[unit]) {
       _swrast_update_texture_samplers(ctx);
    }

    /* XXX use a float-valued TextureSample routine here!!! */
    swrast->TextureSample[unit] (ctx,
                                 ctx->Texture.Unit[unit]._Current,
                                 4,
                                 (const GLfloat (*)[4])stpq,
                                 lambdas,
                                 rgba);

    for (fragment_index = 0; fragment_index < 4; fragment_index++)
    {
        r->f[fragment_index] = CHAN_TO_FLOAT(rgba[fragment_index][0]);
        g->f[fragment_index] = CHAN_TO_FLOAT(rgba[fragment_index][1]);
        b->f[fragment_index] = CHAN_TO_FLOAT(rgba[fragment_index][2]);
        a->f[fragment_index] = CHAN_TO_FLOAT(rgba[fragment_index][3]);
    }
}
#endif

static GLuint
map_label(
   GLuint label,
   struct tgsi_exec_labels *labels )
{
   GLuint i;

   for( i = 0; i < labels->count; i++ ) {
      if( labels->labels[i][0] == label ) {
         return labels->labels[i][1];
      }
   }
   assert( 0 );
   return 0;
}

static void
exec_instruction(
   struct tgsi_exec_machine *mach,
   const struct tgsi_full_instruction *inst,
   struct tgsi_exec_labels *labels,
   GLuint *programCounter )
{
#if MESA
   GET_CURRENT_CONTEXT(ctx);
#endif
   GLuint chan_index;
   union tgsi_exec_channel r[8];

   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_ARL:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
	 FETCH( &r[0], 0, chan_index );
	 micro_f2it( &r[0], &r[0] );
	 STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_MOV:
   /* TGSI_OPCODE_SWZ */
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
      assert (0);
      break;

    case TGSI_OPCODE_LOG:
        assert (0);
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

         micro_lt( &r[0], &r[0], &r[1], &r[0], &r[1] );

         STORE(&r[0], 0, chan_index);
      }
      break;

   case TGSI_OPCODE_MAX:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH(&r[0], 0, chan_index);
         FETCH(&r[1], 1, chan_index);

         micro_lt( &r[0], &r[0], &r[1], &r[1], &r[0] );

         STORE(&r[0], 0, chan_index);
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

      micro_pow( &r[0], &mach->Temps[TEMP_2_I].xyzw[TEMP_2_C], &r[0] );

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
      assert (0);
      break;

   case TGSI_OPCODE_SFL:
      assert (0);
      break;

   case TGSI_OPCODE_SGT:
      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( &r[0], 0, chan_index );
         FETCH( &r[1], 1, chan_index );
         micro_lt( &r[0], &r[0], &r[1], &mach->Temps[TEMP_0_I].xyzw[TEMP_0_C], &mach->Temps[TEMP_1_I].xyzw[TEMP_1_C] );
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
         micro_ge( &r[0], &r[0], &r[1], &mach->Temps[TEMP_0_I].xyzw[TEMP_0_C], &mach->Temps[TEMP_1_I].xyzw[TEMP_1_C] );
         STORE( &r[0], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_SNE:
      assert (0);
      break;

   case TGSI_OPCODE_STR:
      assert (0);
      break;

   case TGSI_OPCODE_TEX:
      switch (inst->InstructionExtTexture.Texture) {
      case TGSI_TEXTURE_1D:

         FETCH(&r[0], 0, CHAN_X);

         switch (inst->FullSrcRegisters[0].SrcRegisterExtSwz.ExtDivide) {
         case TGSI_EXTSWIZZLE_W:
            FETCH(&r[1], 0, CHAN_W);
            micro_div( &r[0], &r[0], &r[1] );
            break;

         case TGSI_EXTSWIZZLE_ONE:
            break;

         default:
            assert (0);
         }
#if MESA
         fetch_texel_1d (ctx,
                         &mach->Samplers[inst->FullSrcRegisters[1].SrcRegister.Index],
                         &r[0],
                         inst->FullSrcRegisters[1].SrcRegister.Index,
                         &r[0], &r[1], &r[2], &r[3]);
#endif
         break;

      case TGSI_TEXTURE_2D:
      case TGSI_TEXTURE_RECT:

         FETCH(&r[0], 0, CHAN_X);
         FETCH(&r[1], 0, CHAN_Y);

         switch (inst->FullSrcRegisters[0].SrcRegisterExtSwz.ExtDivide) {
         case TGSI_EXTSWIZZLE_W:
            FETCH(&r[2], 0, CHAN_W);
            micro_div( &r[0], &r[0], &r[2] );
            micro_div( &r[1], &r[1], &r[2] );
            break;

         case TGSI_EXTSWIZZLE_ONE:
            break;

         default:
            assert (0);
         }

#if MESA
         fetch_texel_2d (ctx,
                         &mach->Samplers[inst->FullSrcRegisters[1].SrcRegister.Index],
                         &r[0], &r[1],
                         inst->FullSrcRegisters[1].SrcRegister.Index,
                         &r[0], &r[1], &r[2], &r[3]);
#endif
         break;

      case TGSI_TEXTURE_3D:
      case TGSI_TEXTURE_CUBE:

         FETCH(&r[0], 0, CHAN_X);
         FETCH(&r[1], 0, CHAN_Y);
         FETCH(&r[2], 0, CHAN_Z);

         switch (inst->FullSrcRegisters[0].SrcRegisterExtSwz.ExtDivide) {
         case TGSI_EXTSWIZZLE_W:
            FETCH(&r[3], 0, CHAN_W);
            micro_div( &r[0], &r[0], &r[3] );
            micro_div( &r[1], &r[1], &r[3] );
            micro_div( &r[2], &r[2], &r[3] );
            break;

         case TGSI_EXTSWIZZLE_ONE:
            break;

         default:
            assert (0);
         }

#if MESA
         fetch_texel_3d (ctx,
                         &mach->Samplers[inst->FullSrcRegisters[1].SrcRegister.Index],
                         &r[0], &r[1], &r[2],
                         inst->FullSrcRegisters[1].SrcRegister.Index,
                         &r[0], &r[1], &r[2], &r[3]);
#endif
         break;

      default:
         assert (0);
      }

      FOR_EACH_ENABLED_CHANNEL( *inst, chan_index ) {
	 STORE( &r[chan_index], 0, chan_index );
      }
      break;

   case TGSI_OPCODE_TXD:
      assert (0);
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
      assert (0);
      break;

   case TGSI_OPCODE_RET:
      /* XXX: end of shader! */
      /*assert (0);*/
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

   case TGSI_OPCODE_TXB:
      assert (0);
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

   case TGSI_OPCODE_TXL:
      assert (0);
      break;

   case TGSI_OPCODE_BRK:
      assert (0);
      break;

   case TGSI_OPCODE_IF:
      assert (0);
      break;

   case TGSI_OPCODE_LOOP:
      assert (0);
      break;

   case TGSI_OPCODE_REP:
      assert (0);
      break;

   case TGSI_OPCODE_ELSE:
      assert (0);
      break;

   case TGSI_OPCODE_ENDIF:
       assert (0);
       break;

   case TGSI_OPCODE_ENDLOOP:
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
      /* TGSI_OPCODE_INT */
      assert (0);
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

   case TGSI_OPCODE_CONT:
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

   case TGSI_OPCODE_BGNLOOP2:
      assert( 0 );
      break;

   case TGSI_OPCODE_BGNSUB:
      assert( 0 );
      break;

   case TGSI_OPCODE_ENDLOOP2:
      assert( 0 );
      break;

   case TGSI_OPCODE_ENDSUB:
      assert( 0 );
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


#if !defined(XSTDCALL) 
#if defined(WIN32)
#define XSTDCALL __stdcall
#else
#define XSTDCALL
#endif
#endif

typedef void (XSTDCALL *fp_function) (const struct tgsi_exec_vector *input,
				     struct tgsi_exec_vector *output,
				     GLfloat (*constant)[4],
				     struct tgsi_exec_vector *temporary);

void
tgsi_exec_machine_run2(
   struct tgsi_exec_machine *mach,
   struct tgsi_exec_labels *labels )
{
#if MESA
   GET_CURRENT_CONTEXT(ctx);
   GLuint i;
#endif

#if XXX_SSE
   fp_function function;

   mach->Temps[TEMP_KILMASK_I].xyzw[TEMP_KILMASK_C].u[0] = 0;
    
   function = (fp_function) x86_get_func (&mach->Function);

   function (mach->Inputs,
             mach->Outputs,
             mach->Consts,
             mach->Temps);
#else
   struct tgsi_parse_context parse;
   GLuint k;

   mach->Temps[TEMP_KILMASK_I].xyzw[TEMP_KILMASK_C].u[0] = 0;
   mach->Temps[TEMP_OUTPUT_I].xyzw[TEMP_OUTPUT_C].u[0] = 0;

   if( mach->Processor == TGSI_PROCESSOR_GEOMETRY ) {
      mach->Temps[TEMP_PRIMITIVE_I].xyzw[TEMP_PRIMITIVE_C].u[0] = 0;
      mach->Primitives[0] = 0;
   }

   k = tgsi_parse_init( &parse, mach->Tokens );
   if (k != TGSI_PARSE_OK) {
      printf("Problem parsing!\n");
      return;
   }

   while( !tgsi_parse_end_of_tokens( &parse ) ) {
      tgsi_parse_token( &parse );
      switch( parse.FullToken.Token.Type ) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         break;
      case TGSI_TOKEN_TYPE_IMMEDIATE:
         break;
      case TGSI_TOKEN_TYPE_INSTRUCTION:
         exec_instruction( mach, &parse.FullToken.FullInstruction, labels, &parse.Position );
         break;
      default:
         assert( 0 );
      }
   }
   tgsi_parse_free (&parse);
#endif

#if MESA
   if (mach->Processor == TGSI_PROCESSOR_FRAGMENT) {
      /*
       * Scale back depth component.
       */
      for (i = 0; i < 4; i++)
         mach->Outputs[0].xyzw[2].f[i] *= ctx->DrawBuffer->_DepthMaxF;
   }
#endif
}

