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
#include "util/u_sse.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "tgsi_exec.h"
#include "tgsi_ppc.h"
#include "rtasm/rtasm_ppc.h"


/* for 1/sqrt()
 *
 * This costs about 100fps (close to 10%) in gears:
 */
#define HIGH_PRECISION 1

#define FAST_MATH 1


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
   int inputs_reg;    /**< register pointing to input params */
   int outputs_reg;   /**< register pointing to output params */
   int temps_reg;     /**< register pointing to temporary "registers" */
   int immed_reg;     /**< register pointing to immediates buffer */
   int const_reg;     /**< register pointing to constants buffer */
};



#if 0000

/**
 * X86 utility functions.
 */

static struct x86_reg
make_xmm(
   unsigned xmm )
{
   return x86_make_reg(
      file_XMM,
      (enum x86_reg_name) xmm );
}

/**
 * X86 register mapping helpers.
 */

static struct x86_reg
get_const_base( void )
{
   return x86_make_reg(
      file_REG32,
      reg_CX );
}

static struct x86_reg
get_input_base( void )
{
   return x86_make_reg(
      file_REG32,
      reg_AX );
}

static struct x86_reg
get_output_base( void )
{
   return x86_make_reg(
      file_REG32,
      reg_DX );
}

static struct x86_reg
get_temp_base( void )
{
   return x86_make_reg(
      file_REG32,
      reg_BX );
}

static struct x86_reg
get_coef_base( void )
{
   return get_output_base();
}

static struct x86_reg
get_immediate_base( void )
{
   return x86_make_reg(
      file_REG32,
      reg_DI );
}


/**
 * Data access helpers.
 */


static struct x86_reg
get_immediate(
   unsigned vec,
   unsigned chan )
{
   return x86_make_disp(
      get_immediate_base(),
      (vec * 4 + chan) * 4 );
}

static struct x86_reg
get_const(
   unsigned vec,
   unsigned chan )
{
   return x86_make_disp(
      get_const_base(),
      (vec * 4 + chan) * 4 );
}

static struct x86_reg
get_input(
   unsigned vec,
   unsigned chan )
{
   return x86_make_disp(
      get_input_base(),
      (vec * 4 + chan) * 16 );
}

static struct x86_reg
get_output(
   unsigned vec,
   unsigned chan )
{
   return x86_make_disp(
      get_output_base(),
      (vec * 4 + chan) * 16 );
}

static struct x86_reg
get_temp(
   unsigned vec,
   unsigned chan )
{
   return x86_make_disp(
      get_temp_base(),
      (vec * 4 + chan) * 16 );
}

static struct x86_reg
get_coef(
   unsigned vec,
   unsigned chan,
   unsigned member )
{
   return x86_make_disp(
      get_coef_base(),
      ((vec * 3 + member) * 4 + chan) * 4 );
}


static void
emit_ret(
   struct x86_function  *func )
{
   x86_ret( func );
}

#endif

/**
 * Data fetch helpers.
 */

#if 00
/**
 * Copy a shader constant to xmm register
 * \param xmm  the destination xmm register
 * \param vec  the src const buffer index
 * \param chan  src channel to fetch (X, Y, Z or W)
 */
static void
emit_const(
   struct x86_function *func,
   uint xmm,
   int vec,
   uint chan,
   uint indirect,
   uint indirectFile,
   int indirectIndex )
{
   if (indirect) {
      struct x86_reg r0 = get_input_base();
      struct x86_reg r1 = get_output_base();
      uint i;

      assert( indirectFile == TGSI_FILE_ADDRESS );
      assert( indirectIndex == 0 );

      x86_push( func, r0 );
      x86_push( func, r1 );

      for (i = 0; i < QUAD_SIZE; i++) {
         x86_lea( func, r0, get_const( vec, chan ) );
         x86_mov( func, r1, x86_make_disp( get_temp( TEMP_ADDR, CHAN_X ), i * 4 ) );

         /* Quick hack to multiply by 16 -- need to add SHL to rtasm.
          */
         x86_add( func, r1, r1 );
         x86_add( func, r1, r1 );
         x86_add( func, r1, r1 );
         x86_add( func, r1, r1 );

         x86_add( func, r0, r1 );
         x86_mov( func, r1, x86_deref( r0 ) );
         x86_mov( func, x86_make_disp( get_temp( TEMP_R0, CHAN_X ), i * 4 ), r1 );
      }

      x86_pop( func, r1 );
      x86_pop( func, r0 );

      sse_movaps(
         func,
         make_xmm( xmm ),
         get_temp( TEMP_R0, CHAN_X ) );
   }
   else {
      assert( vec >= 0 );

      sse_movss(
         func,
         make_xmm( xmm ),
         get_const( vec, chan ) );
      sse_shufps(
         func,
         make_xmm( xmm ),
         make_xmm( xmm ),
         SHUF( 0, 0, 0, 0 ) );
   }
}

static void
emit_immediate(
   struct x86_function *func,
   unsigned xmm,
   unsigned vec,
   unsigned chan )
{
   sse_movss(
      func,
      make_xmm( xmm ),
      get_immediate( vec, chan ) );
   sse_shufps(
      func,
      make_xmm( xmm ),
      make_xmm( xmm ),
      SHUF( 0, 0, 0, 0 ) );
}


/**
 * Copy a shader input to xmm register
 * \param xmm  the destination xmm register
 * \param vec  the src input attrib
 * \param chan  src channel to fetch (X, Y, Z or W)
 */
static void
emit_inputf(
   struct x86_function *func,
   unsigned xmm,
   unsigned vec,
   unsigned chan )
{
   sse_movups(
      func,
      make_xmm( xmm ),
      get_input( vec, chan ) );
}

/**
 * Store an xmm register to a shader output
 * \param xmm  the source xmm register
 * \param vec  the dest output attrib
 * \param chan  src dest channel to store (X, Y, Z or W)
 */
static void
emit_output(
   struct x86_function *func,
   unsigned xmm,
   unsigned vec,
   unsigned chan )
{
   sse_movups(
      func,
      get_output( vec, chan ),
      make_xmm( xmm ) );
}

/**
 * Copy a shader temporary to xmm register
 * \param xmm  the destination xmm register
 * \param vec  the src temp register
 * \param chan  src channel to fetch (X, Y, Z or W)
 */
static void
emit_tempf(
   struct x86_function *func,
   unsigned xmm,
   unsigned vec,
   unsigned chan )
{
   sse_movaps(
      func,
      make_xmm( xmm ),
      get_temp( vec, chan ) );
}

/**
 * Load an xmm register with an input attrib coefficient (a0, dadx or dady)
 * \param xmm  the destination xmm register
 * \param vec  the src input/attribute coefficient index
 * \param chan  src channel to fetch (X, Y, Z or W)
 * \param member  0=a0, 1=dadx, 2=dady
 */
static void
emit_coef(
   struct x86_function *func,
   unsigned xmm,
   unsigned vec,
   unsigned chan,
   unsigned member )
{
   sse_movss(
      func,
      make_xmm( xmm ),
      get_coef( vec, chan, member ) );
   sse_shufps(
      func,
      make_xmm( xmm ),
      make_xmm( xmm ),
      SHUF( 0, 0, 0, 0 ) );
}

/**
 * Data store helpers.
 */

static void
emit_inputs(
   struct x86_function *func,
   unsigned xmm,
   unsigned vec,
   unsigned chan )
{
   sse_movups(
      func,
      get_input( vec, chan ),
      make_xmm( xmm ) );
}

static void
emit_temps(
   struct x86_function *func,
   unsigned xmm,
   unsigned vec,
   unsigned chan )
{
   sse_movaps(
      func,
      get_temp( vec, chan ),
      make_xmm( xmm ) );
}

static void
emit_addrs(
   struct x86_function *func,
   unsigned xmm,
   unsigned vec,
   unsigned chan )
{
   assert( vec == 0 );

   emit_temps(
      func,
      xmm,
      vec + TGSI_EXEC_TEMP_ADDR,
      chan );
}

/**
 * Coefficent fetch helpers.
 */

static void
emit_coef_a0(
   struct x86_function *func,
   unsigned xmm,
   unsigned vec,
   unsigned chan )
{
   emit_coef(
      func,
      xmm,
      vec,
      chan,
      0 );
}

static void
emit_coef_dadx(
   struct x86_function *func,
   unsigned xmm,
   unsigned vec,
   unsigned chan )
{
   emit_coef(
      func,
      xmm,
      vec,
      chan,
      1 );
}

static void
emit_coef_dady(
   struct x86_function *func,
   unsigned xmm,
   unsigned vec,
   unsigned chan )
{
   emit_coef(
      func,
      xmm,
      vec,
      chan,
      2 );
}
#endif


/**
 * Function call helpers.
 */

#if 00
/**
 * NOTE: In gcc, if the destination uses the SSE intrinsics, then it must be 
 * defined with __attribute__((force_align_arg_pointer)), as we do not guarantee
 * that the stack pointer is 16 byte aligned, as expected.
 */
static void
emit_func_call_dst(
   struct x86_function *func,
   unsigned xmm_save,
   unsigned xmm_dst,
   void (PIPE_CDECL *code)() )
{
   struct x86_reg ecx = x86_make_reg( file_REG32, reg_CX );
   unsigned i, n, xmm;
   unsigned xmm_mask;
   
   /* Bitmask of the xmm registers to save */
   xmm_mask = (1 << xmm_save) - 1;
   xmm_mask &= ~(1 << xmm_dst);

   sse_movaps(
      func,
      get_temp( TEMP_R0, 0 ),
      make_xmm( xmm_dst ) );

   x86_push(
      func,
      x86_make_reg( file_REG32, reg_AX) );
   x86_push(
      func,
      x86_make_reg( file_REG32, reg_CX) );
   x86_push(
      func,
      x86_make_reg( file_REG32, reg_DX) );
   
   for(i = 0, n = 0; i < 8; ++i)
      if(xmm_mask & (1 << i))
         ++n;
   
   x86_sub_imm(
      func, 
      x86_make_reg( file_REG32, reg_SP ),
      n*16);

   for(i = 0, n = 0; i < 8; ++i)
      if(xmm_mask & (1 << i)) {
         sse_movups(
            func,
            x86_make_disp( x86_make_reg( file_REG32, reg_SP ), n*16 ),
            make_xmm( xmm ) );
         ++n;
      }
   
   x86_lea(
      func,
      ecx,
      get_temp( TEMP_R0, 0 ) );
   
   x86_push( func, ecx );
   x86_mov_reg_imm( func, ecx, (unsigned long) code );
   x86_call( func, ecx );
   x86_pop(func, ecx );
   
   for(i = 0, n = 0; i < 8; ++i)
      if(xmm_mask & (1 << i)) {
         sse_movups(
            func,
            make_xmm( xmm ),
            x86_make_disp( x86_make_reg( file_REG32, reg_SP ), n*16 ) );
         ++n;
      }
   
   x86_add_imm(
      func, 
      x86_make_reg( file_REG32, reg_SP ),
      n*16);

   /* Restore GP registers in a reverse order.
    */
   x86_pop(
      func,
      x86_make_reg( file_REG32, reg_DX) );
   x86_pop(
      func,
      x86_make_reg( file_REG32, reg_CX) );
   x86_pop(
      func,
      x86_make_reg( file_REG32, reg_AX) );

   sse_movaps(
      func,
      make_xmm( xmm_dst ),
      get_temp( TEMP_R0, 0 ) );
}

static void
emit_func_call_dst_src(
   struct x86_function *func,
   unsigned xmm_save, 
   unsigned xmm_dst,
   unsigned xmm_src,
   void (PIPE_CDECL *code)() )
{
   sse_movaps(
      func,
      get_temp( TEMP_R0, 1 ),
      make_xmm( xmm_src ) );

   emit_func_call_dst(
      func,
      xmm_save,
      xmm_dst,
      code );
}

/*
 * Fast SSE2 implementation of special math functions.
 */

#define POLY0(x, c0) _mm_set1_ps(c0)
#define POLY1(x, c0, c1) _mm_add_ps(_mm_mul_ps(POLY0(x, c1), x), _mm_set1_ps(c0))
#define POLY2(x, c0, c1, c2) _mm_add_ps(_mm_mul_ps(POLY1(x, c1, c2), x), _mm_set1_ps(c0))
#define POLY3(x, c0, c1, c2, c3) _mm_add_ps(_mm_mul_ps(POLY2(x, c1, c2, c3), x), _mm_set1_ps(c0))
#define POLY4(x, c0, c1, c2, c3, c4) _mm_add_ps(_mm_mul_ps(POLY3(x, c1, c2, c3, c4), x), _mm_set1_ps(c0))
#define POLY5(x, c0, c1, c2, c3, c4, c5) _mm_add_ps(_mm_mul_ps(POLY4(x, c1, c2, c3, c4, c5), x), _mm_set1_ps(c0))

#define EXP_POLY_DEGREE 3
#define LOG_POLY_DEGREE 5

/**
 * See http://www.devmaster.net/forums/showthread.php?p=43580
 */
static INLINE __m128 
exp2f4(__m128 x)
{
   __m128i ipart;
   __m128 fpart, expipart, expfpart;

   x = _mm_min_ps(x, _mm_set1_ps( 129.00000f));
   x = _mm_max_ps(x, _mm_set1_ps(-126.99999f));

   /* ipart = int(x - 0.5) */
   ipart = _mm_cvtps_epi32(_mm_sub_ps(x, _mm_set1_ps(0.5f)));

   /* fpart = x - ipart */
   fpart = _mm_sub_ps(x, _mm_cvtepi32_ps(ipart));

   /* expipart = (float) (1 << ipart) */
   expipart = _mm_castsi128_ps(_mm_slli_epi32(_mm_add_epi32(ipart, _mm_set1_epi32(127)), 23));

   /* minimax polynomial fit of 2**x, in range [-0.5, 0.5[ */
#if EXP_POLY_DEGREE == 5
   expfpart = POLY5(fpart, 9.9999994e-1f, 6.9315308e-1f, 2.4015361e-1f, 5.5826318e-2f, 8.9893397e-3f, 1.8775767e-3f);
#elif EXP_POLY_DEGREE == 4
   expfpart = POLY4(fpart, 1.0000026f, 6.9300383e-1f, 2.4144275e-1f, 5.2011464e-2f, 1.3534167e-2f);
#elif EXP_POLY_DEGREE == 3
   expfpart = POLY3(fpart, 9.9992520e-1f, 6.9583356e-1f, 2.2606716e-1f, 7.8024521e-2f);
#elif EXP_POLY_DEGREE == 2
   expfpart = POLY2(fpart, 1.0017247f, 6.5763628e-1f, 3.3718944e-1f);
#else
#error
#endif

   return _mm_mul_ps(expipart, expfpart);
}

/**
 * See http://www.devmaster.net/forums/showthread.php?p=43580
 */
static INLINE __m128 
log2f4(__m128 x)
{
   __m128i expmask = _mm_set1_epi32(0x7f800000);
   __m128i mantmask = _mm_set1_epi32(0x007fffff);
   __m128 one = _mm_set1_ps(1.0f);

   __m128i i = _mm_castps_si128(x);

   /* exp = (float) exponent(x) */
   __m128 exp = _mm_cvtepi32_ps(_mm_sub_epi32(_mm_srli_epi32(_mm_and_si128(i, expmask), 23), _mm_set1_epi32(127)));

   /* mant = (float) mantissa(x) */
   __m128 mant = _mm_or_ps(_mm_castsi128_ps(_mm_and_si128(i, mantmask)), one);

   __m128 logmant;

   /* Minimax polynomial fit of log2(x)/(x - 1), for x in range [1, 2[ 
    * These coefficients can be generate with 
    * http://www.boost.org/doc/libs/1_36_0/libs/math/doc/sf_and_dist/html/math_toolkit/toolkit/internals2/minimax.html
    */
#if LOG_POLY_DEGREE == 6
   logmant = POLY5(mant, 3.11578814719469302614f, -3.32419399085241980044f, 2.59883907202499966007f, -1.23152682416275988241f, 0.318212422185251071475f, -0.0344359067839062357313f);
#elif LOG_POLY_DEGREE == 5
   logmant = POLY4(mant, 2.8882704548164776201f, -2.52074962577807006663f, 1.48116647521213171641f, -0.465725644288844778798f, 0.0596515482674574969533f);
#elif LOG_POLY_DEGREE == 4
   logmant = POLY3(mant, 2.61761038894603480148f, -1.75647175389045657003f, 0.688243882994381274313f, -0.107254423828329604454f);
#elif LOG_POLY_DEGREE == 3
   logmant = POLY2(mant, 2.28330284476918490682f, -1.04913055217340124191f, 0.204446009836232697516f);
#else
#error
#endif

   /* This effectively increases the polynomial degree by one, but ensures that log2(1) == 0*/
   logmant = _mm_mul_ps(logmant, _mm_sub_ps(mant, one));

   return _mm_add_ps(logmant, exp);
}

static INLINE __m128
powf4(__m128 x, __m128 y)
{
   return exp2f4(_mm_mul_ps(log2f4(x), y));
}


/**
 * Low-level instruction translators.
 */

static void
emit_abs(
   struct x86_function *func,
   unsigned xmm )
{
   sse_andps(
      func,
      make_xmm( xmm ),
      get_temp(
         TGSI_EXEC_TEMP_7FFFFFFF_I,
         TGSI_EXEC_TEMP_7FFFFFFF_C ) );
}

static void
emit_add(
   struct x86_function *func,
   unsigned xmm_dst,
   unsigned xmm_src )
{
   sse_addps(
      func,
      make_xmm( xmm_dst ),
      make_xmm( xmm_src ) );
}

static void PIPE_CDECL
cos4f(
   float *store )
{
   store[0] = cosf( store[0] );
   store[1] = cosf( store[1] );
   store[2] = cosf( store[2] );
   store[3] = cosf( store[3] );
}

static void
emit_cos(
   struct x86_function *func,
   unsigned xmm_save, 
   unsigned xmm_dst )
{
   emit_func_call_dst(
      func,
      xmm_save, 
      xmm_dst,
      cos4f );
}

static void PIPE_CDECL
#if defined(PIPE_CC_GCC)
__attribute__((force_align_arg_pointer))
#endif
ex24f(
   float *store )
{
   _mm_store_ps(&store[0], exp2f4( _mm_load_ps(&store[0]) ));
}

static void
emit_ex2(
   struct x86_function *func,
   unsigned xmm_save, 
   unsigned xmm_dst )
{
   emit_func_call_dst(
      func,
      xmm_save,
      xmm_dst,
      ex24f );
}

static void
emit_f2it(
   struct x86_function *func,
   unsigned xmm )
{
   sse2_cvttps2dq(
      func,
      make_xmm( xmm ),
      make_xmm( xmm ) );
}

static void PIPE_CDECL
flr4f(
   float *store )
{
   store[0] = floorf( store[0] );
   store[1] = floorf( store[1] );
   store[2] = floorf( store[2] );
   store[3] = floorf( store[3] );
}

static void
emit_flr(
   struct x86_function *func,
   unsigned xmm_save, 
   unsigned xmm_dst )
{
   emit_func_call_dst(
      func,
      xmm_save,
      xmm_dst,
      flr4f );
}

static void PIPE_CDECL
frc4f(
   float *store )
{
   store[0] -= floorf( store[0] );
   store[1] -= floorf( store[1] );
   store[2] -= floorf( store[2] );
   store[3] -= floorf( store[3] );
}

static void
emit_frc(
   struct x86_function *func,
   unsigned xmm_save, 
   unsigned xmm_dst )
{
   emit_func_call_dst(
      func,
      xmm_save,
      xmm_dst,
      frc4f );
}

static void PIPE_CDECL
#if defined(PIPE_CC_GCC)
__attribute__((force_align_arg_pointer))
#endif
lg24f(
   float *store )
{
   _mm_store_ps(&store[0], log2f4( _mm_load_ps(&store[0]) ));
}

static void
emit_lg2(
   struct x86_function *func,
   unsigned xmm_save, 
   unsigned xmm_dst )
{
   emit_func_call_dst(
      func,
      xmm_save,
      xmm_dst,
      lg24f );
}

static void
emit_MOV(
   struct x86_function *func,
   unsigned xmm_dst,
   unsigned xmm_src )
{
   sse_movups(
      func,
      make_xmm( xmm_dst ),
      make_xmm( xmm_src ) );
}

static void
emit_mul (struct x86_function *func,
          unsigned xmm_dst,
          unsigned xmm_src)
{
   sse_mulps(
      func,
      make_xmm( xmm_dst ),
      make_xmm( xmm_src ) );
}

static void
emit_neg(
   struct x86_function *func,
   unsigned xmm )
{
   sse_xorps(
      func,
      make_xmm( xmm ),
      get_temp(
         TGSI_EXEC_TEMP_80000000_I,
         TGSI_EXEC_TEMP_80000000_C ) );
}

static void PIPE_CDECL
#if defined(PIPE_CC_GCC)
__attribute__((force_align_arg_pointer))
#endif
pow4f(
   float *store )
{
#if 1
   _mm_store_ps(&store[0], powf4( _mm_load_ps(&store[0]), _mm_load_ps(&store[4]) ));
#else
   store[0] = powf( store[0], store[4] );
   store[1] = powf( store[1], store[5] );
   store[2] = powf( store[2], store[6] );
   store[3] = powf( store[3], store[7] );
#endif
}

static void
emit_pow(
   struct x86_function *func,
   unsigned xmm_save, 
   unsigned xmm_dst,
   unsigned xmm_src )
{
   emit_func_call_dst_src(
      func,
      xmm_save,
      xmm_dst,
      xmm_src,
      pow4f );
}

static void
emit_rcp (
   struct x86_function *func,
   unsigned xmm_dst,
   unsigned xmm_src )
{
   /* On Intel CPUs at least, this is only accurate to 12 bits -- not
    * good enough.  Need to either emit a proper divide or use the
    * iterative technique described below in emit_rsqrt().
    */
   sse2_rcpps(
      func,
      make_xmm( xmm_dst ),
      make_xmm( xmm_src ) );
}

static void
emit_rsqrt(
   struct x86_function *func,
   unsigned xmm_dst,
   unsigned xmm_src )
{
#if HIGH_PRECISION
   /* Although rsqrtps() and rcpps() are low precision on some/all SSE
    * implementations, it is possible to improve its precision at
    * fairly low cost, using a newton/raphson step, as below:
    * 
    * x1 = 2 * rcpps(a) - a * rcpps(a) * rcpps(a)
    * x1 = 0.5 * rsqrtps(a) * [3.0 - (a * rsqrtps(a))* rsqrtps(a)]
    *
    * See: http://softwarecommunity.intel.com/articles/eng/1818.htm
    */
   {
      struct x86_reg dst = make_xmm( xmm_dst );
      struct x86_reg src = make_xmm( xmm_src );
      struct x86_reg tmp0 = make_xmm( 2 );
      struct x86_reg tmp1 = make_xmm( 3 );

      assert( xmm_dst != xmm_src );
      assert( xmm_dst != 2 && xmm_dst != 3 );
      assert( xmm_src != 2 && xmm_src != 3 );

      sse_movaps(  func, dst,  get_temp( TGSI_EXEC_TEMP_HALF_I, TGSI_EXEC_TEMP_HALF_C ) );
      sse_movaps(  func, tmp0, get_temp( TGSI_EXEC_TEMP_THREE_I, TGSI_EXEC_TEMP_THREE_C ) );
      sse_rsqrtps( func, tmp1, src  );
      sse_mulps(   func, src,  tmp1 );
      sse_mulps(   func, dst,  tmp1 );
      sse_mulps(   func, src,  tmp1 );
      sse_subps(   func, tmp0, src  );
      sse_mulps(   func, dst,  tmp0 );
   }
#else
   /* On Intel CPUs at least, this is only accurate to 12 bits -- not
    * good enough.
    */
   sse_rsqrtps(
      func,
      make_xmm( xmm_dst ),
      make_xmm( xmm_src ) );
#endif
}

static void
emit_setsign(
   struct x86_function *func,
   unsigned xmm )
{
   sse_orps(
      func,
      make_xmm( xmm ),
      get_temp(
         TGSI_EXEC_TEMP_80000000_I,
         TGSI_EXEC_TEMP_80000000_C ) );
}

static void PIPE_CDECL
sin4f(
   float *store )
{
   store[0] = sinf( store[0] );
   store[1] = sinf( store[1] );
   store[2] = sinf( store[2] );
   store[3] = sinf( store[3] );
}

static void
emit_sin (struct x86_function *func,
          unsigned xmm_save, 
          unsigned xmm_dst)
{
   emit_func_call_dst(
      func,
      xmm_save,
      xmm_dst,
      sin4f );
}

static void
emit_sub(
   struct x86_function *func,
   unsigned xmm_dst,
   unsigned xmm_src )
{
   sse_subps(
      func,
      make_xmm( xmm_dst ),
      make_xmm( xmm_src ) );
}
#endif


/**
 * Register fetch.
 */
static void
emit_fetch(struct gen_context *gen,
           unsigned vec_reg,
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
            ppc_lvx(gen->f, vec_reg, gen->inputs_reg, offset_reg);
            ppc_release_register(gen->f, offset_reg);
         }
         break;
      case TGSI_FILE_TEMPORARY:
         {
            int offset_reg = ppc_allocate_register(gen->f);
            int offset = (reg->SrcRegister.Index * 4 + swizzle) * 16;
            ppc_li(gen->f, offset_reg, offset);
            ppc_lvx(gen->f, vec_reg, gen->temps_reg, offset_reg);
            ppc_release_register(gen->f, offset_reg);
         }
         break;
      case TGSI_FILE_IMMEDIATE:
         {
            int offset_reg = ppc_allocate_register(gen->f);
            int offset = (reg->SrcRegister.Index * 4 + swizzle) * 16;
            ppc_li(gen->f, offset_reg, offset);
            ppc_lvx(gen->f, vec_reg, gen->immed_reg, offset_reg);
            ppc_release_register(gen->f, offset_reg);
         }
         break;
      case TGSI_FILE_CONSTANT:
         {
            int offset_reg = ppc_allocate_register(gen->f);
            int offset = (reg->SrcRegister.Index * 4 + swizzle) * 4;
            ppc_li(gen->f, offset_reg, offset);
            /* load vector word */
            ppc_lvewx(gen->f, vec_reg, gen->const_reg, offset_reg);
            /* splat word[0] across vector */
            ppc_vspltw(gen->f, vec_reg, vec_reg, 0);
            ppc_release_register(gen->f, offset_reg);
         }
         break;
      default:
         assert( 0 );
      }
      break;

   case TGSI_EXTSWIZZLE_ZERO:
#if 0
      emit_tempf(
         func,
         xmm,
         TGSI_EXEC_TEMP_00000000_I,
         TGSI_EXEC_TEMP_00000000_C );
#endif
      break;

   case TGSI_EXTSWIZZLE_ONE:
#if 0
      emit_tempf(
         func,
         xmm,
         TEMP_ONE_I,
         TEMP_ONE_C );
#endif
      break;

   default:
      assert( 0 );
   }

#if 0
   switch( tgsi_util_get_full_src_register_sign_mode( reg, chan_index ) ) {
   case TGSI_UTIL_SIGN_CLEAR:
      emit_abs( func, xmm );
      break;

   case TGSI_UTIL_SIGN_SET:
      emit_setsign( func, xmm );
      break;

   case TGSI_UTIL_SIGN_TOGGLE:
      emit_neg( func, xmm );
      break;

   case TGSI_UTIL_SIGN_KEEP:
      break;
   }
#endif
}

#define FETCH( GEN, INST, VEC_REG, SRC_REG, CHAN ) \
   emit_fetch( GEN, VEC_REG, &(INST).FullSrcRegisters[SRC_REG], CHAN )



/**
 * Register store.
 */
static void
emit_store(struct gen_context *gen,
           unsigned vec_reg,
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
         ppc_stvx(gen->f, vec_reg, gen->outputs_reg, offset_reg);
         ppc_release_register(gen->f, offset_reg);
      }
      break;
   case TGSI_FILE_TEMPORARY:
      {
         int offset_reg = ppc_allocate_register(gen->f);
         int offset = (reg->DstRegister.Index * 4 + chan_index) * 16;
         ppc_li(gen->f, offset_reg, offset);
         ppc_stvx(gen->f, vec_reg, gen->temps_reg, offset_reg);
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



#if 000
/**
 * High-level instruction translators.
 */

static void
emit_kil(
   struct x86_function *func,
   const struct tgsi_full_src_register *reg )
{
   unsigned uniquemask;
   unsigned registers[4];
   unsigned nextregister = 0;
   unsigned firstchan = ~0;
   unsigned chan_index;

   /* This mask stores component bits that were already tested. Note that
    * we test if the value is less than zero, so 1.0 and 0.0 need not to be
    * tested. */
   uniquemask = (1 << TGSI_EXTSWIZZLE_ZERO) | (1 << TGSI_EXTSWIZZLE_ONE);

   FOR_EACH_CHANNEL( chan_index ) {
      unsigned swizzle;

      /* unswizzle channel */
      swizzle = tgsi_util_get_full_src_register_extswizzle(
         reg,
         chan_index );

      /* check if the component has not been already tested */
      if( !(uniquemask & (1 << swizzle)) ) {
         uniquemask |= 1 << swizzle;

         /* allocate register */
         registers[chan_index] = nextregister;
         emit_fetch(
            func,
            nextregister,
            reg,
            chan_index );
         nextregister++;

         /* mark the first channel used */
         if( firstchan == ~0 ) {
            firstchan = chan_index;
         }
      }
   }

   x86_push(
      func,
      x86_make_reg( file_REG32, reg_AX ) );
   x86_push(
      func,
      x86_make_reg( file_REG32, reg_DX ) );

   FOR_EACH_CHANNEL( chan_index ) {
      if( uniquemask & (1 << chan_index) ) {
         sse_cmpps(
            func,
            make_xmm( registers[chan_index] ),
            get_temp(
               TGSI_EXEC_TEMP_00000000_I,
               TGSI_EXEC_TEMP_00000000_C ),
            cc_LessThan );

         if( chan_index == firstchan ) {
            sse_pmovmskb(
               func,
               x86_make_reg( file_REG32, reg_AX ),
               make_xmm( registers[chan_index] ) );
         }
         else {
            sse_pmovmskb(
               func,
               x86_make_reg( file_REG32, reg_DX ),
               make_xmm( registers[chan_index] ) );
            x86_or(
               func,
               x86_make_reg( file_REG32, reg_AX ),
               x86_make_reg( file_REG32, reg_DX ) );
         }
      }
   }

   x86_or(
      func,
      get_temp(
         TGSI_EXEC_TEMP_KILMASK_I,
         TGSI_EXEC_TEMP_KILMASK_C ),
      x86_make_reg( file_REG32, reg_AX ) );

   x86_pop(
      func,
      x86_make_reg( file_REG32, reg_DX ) );
   x86_pop(
      func,
      x86_make_reg( file_REG32, reg_AX ) );
}


static void
emit_kilp(
   struct x86_function *func )
{
   /* XXX todo / fix me */
}


static void
emit_setcc(
   struct x86_function *func,
   struct tgsi_full_instruction *inst,
   enum sse_cc cc )
{
   unsigned chan_index;

   FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
      FETCH( func, *inst, 0, 0, chan_index );
      FETCH( func, *inst, 1, 1, chan_index );
      sse_cmpps(
         func,
         make_xmm( 0 ),
         make_xmm( 1 ),
         cc );
      sse_andps(
         func,
         make_xmm( 0 ),
         get_temp(
            TEMP_ONE_I,
            TEMP_ONE_C ) );
      STORE( func, *inst, 0, 0, chan_index );
   }
}

static void
emit_cmp(
   struct x86_function *func,
   struct tgsi_full_instruction *inst )
{
   unsigned chan_index;

   FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
      FETCH( func, *inst, 0, 0, chan_index );
      FETCH( func, *inst, 1, 1, chan_index );
      FETCH( func, *inst, 2, 2, chan_index );
      sse_cmpps(
         func,
         make_xmm( 0 ),
         get_temp(
            TGSI_EXEC_TEMP_00000000_I,
            TGSI_EXEC_TEMP_00000000_C ),
         cc_LessThan );
      sse_andps(
         func,
         make_xmm( 1 ),
         make_xmm( 0 ) );
      sse_andnps(
         func,
         make_xmm( 0 ),
         make_xmm( 2 ) );
      sse_orps(
         func,
         make_xmm( 0 ),
         make_xmm( 1 ) );
      STORE( func, *inst, 0, 0, chan_index );
   }
}
#endif


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
   case TGSI_OPCODE_ADD:
   case TGSI_OPCODE_SUB:
   case TGSI_OPCODE_MUL:
   case TGSI_OPCODE_MIN:
   case TGSI_OPCODE_MAX:
      emit_binop(gen, inst);
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
   case TGSI_OPCODE_END:
      /* normal end */
      return 1;
   default:
      return 0;
   }

#if 0
   unsigned chan_index;

   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_ARL:
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( func, *inst, 0, 0, chan_index );
         emit_f2it( func, 0 );
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_MOV:
   case TGSI_OPCODE_SWZ:
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( func, *inst, 0, 0, chan_index );
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_LIT:
      if( IS_DST0_CHANNEL_ENABLED( *inst, CHAN_X ) ||
          IS_DST0_CHANNEL_ENABLED( *inst, CHAN_W ) ) {
         emit_tempf(
            func,
            0,
            TEMP_ONE_I,
            TEMP_ONE_C);
         if( IS_DST0_CHANNEL_ENABLED( *inst, CHAN_X ) ) {
            STORE( func, *inst, 0, 0, CHAN_X );
         }
         if( IS_DST0_CHANNEL_ENABLED( *inst, CHAN_W ) ) {
            STORE( func, *inst, 0, 0, CHAN_W );
         }
      }
      if( IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Y ) ||
          IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Z ) ) {
         if( IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Y ) ) {
            FETCH( func, *inst, 0, 0, CHAN_X );
            sse_maxps(
               func,
               make_xmm( 0 ),
               get_temp(
                  TGSI_EXEC_TEMP_00000000_I,
                  TGSI_EXEC_TEMP_00000000_C ) );
            STORE( func, *inst, 0, 0, CHAN_Y );
         }
         if( IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Z ) ) {
            /* XMM[1] = SrcReg[0].yyyy */
            FETCH( func, *inst, 1, 0, CHAN_Y );
            /* XMM[1] = max(XMM[1], 0) */
            sse_maxps(
               func,
               make_xmm( 1 ),
               get_temp(
                  TGSI_EXEC_TEMP_00000000_I,
                  TGSI_EXEC_TEMP_00000000_C ) );
            /* XMM[2] = SrcReg[0].wwww */
            FETCH( func, *inst, 2, 0, CHAN_W );
            /* XMM[2] = min(XMM[2], 128.0) */
            sse_minps(
               func,
               make_xmm( 2 ),
               get_temp(
                  TGSI_EXEC_TEMP_128_I,
                  TGSI_EXEC_TEMP_128_C ) );
            /* XMM[2] = max(XMM[2], -128.0) */
            sse_maxps(
               func,
               make_xmm( 2 ),
               get_temp(
                  TGSI_EXEC_TEMP_MINUS_128_I,
                  TGSI_EXEC_TEMP_MINUS_128_C ) );
            emit_pow( func, 3, 1, 2 );
            FETCH( func, *inst, 0, 0, CHAN_X );
            sse_xorps(
               func,
               make_xmm( 2 ),
               make_xmm( 2 ) );
            sse_cmpps(
               func,
               make_xmm( 2 ),
               make_xmm( 0 ),
               cc_LessThanEqual );
            sse_andps(
               func,
               make_xmm( 2 ),
               make_xmm( 1 ) );
            STORE( func, *inst, 2, 0, CHAN_Z );
         }
      }
      break;

   case TGSI_OPCODE_RCP:
   /* TGSI_OPCODE_RECIP */
      FETCH( func, *inst, 0, 0, CHAN_X );
      emit_rcp( func, 0, 0 );
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_RSQ:
   /* TGSI_OPCODE_RECIPSQRT */
      FETCH( func, *inst, 0, 0, CHAN_X );
      emit_rsqrt( func, 1, 0 );
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         STORE( func, *inst, 1, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_EXP:
      if (IS_DST0_CHANNEL_ENABLED( *inst, CHAN_X ) ||
          IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Y ) ||
          IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Z )) {
         FETCH( func, *inst, 0, 0, CHAN_X );
         if (IS_DST0_CHANNEL_ENABLED( *inst, CHAN_X ) ||
             IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Y )) {
            emit_MOV( func, 1, 0 );
            emit_flr( func, 2, 1 );
            /* dst.x = ex2(floor(src.x)) */
            if (IS_DST0_CHANNEL_ENABLED( *inst, CHAN_X )) {
               emit_MOV( func, 2, 1 );
               emit_ex2( func, 3, 2 );
               STORE( func, *inst, 2, 0, CHAN_X );
            }
            /* dst.y = src.x - floor(src.x) */
            if (IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Y )) {
               emit_MOV( func, 2, 0 );
               emit_sub( func, 2, 1 );
               STORE( func, *inst, 2, 0, CHAN_Y );
            }
         }
         /* dst.z = ex2(src.x) */
         if (IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Z )) {
            emit_ex2( func, 3, 0 );
            STORE( func, *inst, 0, 0, CHAN_Z );
         }
      }
      /* dst.w = 1.0 */
      if (IS_DST0_CHANNEL_ENABLED( *inst, CHAN_W )) {
         emit_tempf( func, 0, TEMP_ONE_I, TEMP_ONE_C );
         STORE( func, *inst, 0, 0, CHAN_W );
      }
      break;

   case TGSI_OPCODE_LOG:
      if (IS_DST0_CHANNEL_ENABLED( *inst, CHAN_X ) ||
          IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Y ) ||
          IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Z )) {
         FETCH( func, *inst, 0, 0, CHAN_X );
         emit_abs( func, 0 );
         emit_MOV( func, 1, 0 );
         emit_lg2( func, 2, 1 );
         /* dst.z = lg2(abs(src.x)) */
         if (IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Z )) {
            STORE( func, *inst, 1, 0, CHAN_Z );
         }
         if (IS_DST0_CHANNEL_ENABLED( *inst, CHAN_X ) ||
             IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Y )) {
            emit_flr( func, 2, 1 );
            /* dst.x = floor(lg2(abs(src.x))) */
            if (IS_DST0_CHANNEL_ENABLED( *inst, CHAN_X )) {
               STORE( func, *inst, 1, 0, CHAN_X );
            }
            /* dst.x = abs(src)/ex2(floor(lg2(abs(src.x)))) */
            if (IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Y )) {
               emit_ex2( func, 2, 1 );
               emit_rcp( func, 1, 1 );
               emit_mul( func, 0, 1 );
               STORE( func, *inst, 0, 0, CHAN_Y );
            }
         }
      }
      /* dst.w = 1.0 */
      if (IS_DST0_CHANNEL_ENABLED( *inst, CHAN_W )) {
         emit_tempf( func, 0, TEMP_ONE_I, TEMP_ONE_C );
         STORE( func, *inst, 0, 0, CHAN_W );
      }
      break;

   case TGSI_OPCODE_MUL:
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( func, *inst, 0, 0, chan_index );
         FETCH( func, *inst, 1, 1, chan_index );
         emit_mul( func, 0, 1 );
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_ADD:
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( func, *inst, 0, 0, chan_index );
         FETCH( func, *inst, 1, 1, chan_index );
         emit_add( func, 0, 1 );
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_DP3:
   /* TGSI_OPCODE_DOT3 */
      FETCH( func, *inst, 0, 0, CHAN_X );
      FETCH( func, *inst, 1, 1, CHAN_X );
      emit_mul( func, 0, 1 );
      FETCH( func, *inst, 1, 0, CHAN_Y );
      FETCH( func, *inst, 2, 1, CHAN_Y );
      emit_mul( func, 1, 2 );
      emit_add( func, 0, 1 );
      FETCH( func, *inst, 1, 0, CHAN_Z );
      FETCH( func, *inst, 2, 1, CHAN_Z );
      emit_mul( func, 1, 2 );
      emit_add( func, 0, 1 );
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_DP4:
   /* TGSI_OPCODE_DOT4 */
      FETCH( func, *inst, 0, 0, CHAN_X );
      FETCH( func, *inst, 1, 1, CHAN_X );
      emit_mul( func, 0, 1 );
      FETCH( func, *inst, 1, 0, CHAN_Y );
      FETCH( func, *inst, 2, 1, CHAN_Y );
      emit_mul( func, 1, 2 );
      emit_add( func, 0, 1 );
      FETCH( func, *inst, 1, 0, CHAN_Z );
      FETCH( func, *inst, 2, 1, CHAN_Z );
      emit_mul(func, 1, 2 );
      emit_add(func, 0, 1 );
      FETCH( func, *inst, 1, 0, CHAN_W );
      FETCH( func, *inst, 2, 1, CHAN_W );
      emit_mul( func, 1, 2 );
      emit_add( func, 0, 1 );
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_DST:
      IF_IS_DST0_CHANNEL_ENABLED( *inst, CHAN_X ) {
         emit_tempf(
            func,
            0,
            TEMP_ONE_I,
            TEMP_ONE_C );
         STORE( func, *inst, 0, 0, CHAN_X );
      }
      IF_IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Y ) {
         FETCH( func, *inst, 0, 0, CHAN_Y );
         FETCH( func, *inst, 1, 1, CHAN_Y );
         emit_mul( func, 0, 1 );
         STORE( func, *inst, 0, 0, CHAN_Y );
      }
      IF_IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Z ) {
         FETCH( func, *inst, 0, 0, CHAN_Z );
         STORE( func, *inst, 0, 0, CHAN_Z );
      }
      IF_IS_DST0_CHANNEL_ENABLED( *inst, CHAN_W ) {
         FETCH( func, *inst, 0, 1, CHAN_W );
         STORE( func, *inst, 0, 0, CHAN_W );
      }
      break;

   case TGSI_OPCODE_MIN:
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( func, *inst, 0, 0, chan_index );
         FETCH( func, *inst, 1, 1, chan_index );
         sse_minps(
            func,
            make_xmm( 0 ),
            make_xmm( 1 ) );
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_MAX:
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( func, *inst, 0, 0, chan_index );
         FETCH( func, *inst, 1, 1, chan_index );
         sse_maxps(
            func,
            make_xmm( 0 ),
            make_xmm( 1 ) );
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_SLT:
   /* TGSI_OPCODE_SETLT */
      emit_setcc( func, inst, cc_LessThan );
      break;

   case TGSI_OPCODE_SGE:
   /* TGSI_OPCODE_SETGE */
      emit_setcc( func, inst, cc_NotLessThan );
      break;

   case TGSI_OPCODE_MAD:
   /* TGSI_OPCODE_MADD */
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( func, *inst, 0, 0, chan_index );
         FETCH( func, *inst, 1, 1, chan_index );
         FETCH( func, *inst, 2, 2, chan_index );
         emit_mul( func, 0, 1 );
         emit_add( func, 0, 2 );
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_SUB:
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( func, *inst, 0, 0, chan_index );
         FETCH( func, *inst, 1, 1, chan_index );
         emit_sub( func, 0, 1 );
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_LERP:
   /* TGSI_OPCODE_LRP */
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( func, *inst, 0, 0, chan_index );
         FETCH( func, *inst, 1, 1, chan_index );
         FETCH( func, *inst, 2, 2, chan_index );
         emit_sub( func, 1, 2 );
         emit_mul( func, 0, 1 );
         emit_add( func, 0, 2 );
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_CND:
      return 0;
      break;

   case TGSI_OPCODE_CND0:
      return 0;
      break;

   case TGSI_OPCODE_DOT2ADD:
   /* TGSI_OPCODE_DP2A */
      return 0;
      break;

   case TGSI_OPCODE_INDEX:
      return 0;
      break;

   case TGSI_OPCODE_NEGATE:
      return 0;
      break;

   case TGSI_OPCODE_FRAC:
   /* TGSI_OPCODE_FRC */
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( func, *inst, 0, 0, chan_index );
         emit_frc( func, 0, 0 );
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_CLAMP:
      return 0;
      break;

   case TGSI_OPCODE_FLOOR:
   /* TGSI_OPCODE_FLR */
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( func, *inst, 0, 0, chan_index );
         emit_flr( func, 0, 0 );
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_ROUND:
      return 0;
      break;

   case TGSI_OPCODE_EXPBASE2:
   /* TGSI_OPCODE_EX2 */
      FETCH( func, *inst, 0, 0, CHAN_X );
      emit_ex2( func, 0, 0 );
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_LOGBASE2:
   /* TGSI_OPCODE_LG2 */
      FETCH( func, *inst, 0, 0, CHAN_X );
      emit_lg2( func, 0, 0 );
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_POWER:
   /* TGSI_OPCODE_POW */
      FETCH( func, *inst, 0, 0, CHAN_X );
      FETCH( func, *inst, 1, 1, CHAN_X );
      emit_pow( func, 0, 0, 1 );
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_CROSSPRODUCT:
   /* TGSI_OPCODE_XPD */
      if( IS_DST0_CHANNEL_ENABLED( *inst, CHAN_X ) ||
          IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Y ) ) {
         FETCH( func, *inst, 1, 1, CHAN_Z );
         FETCH( func, *inst, 3, 0, CHAN_Z );
      }
      if( IS_DST0_CHANNEL_ENABLED( *inst, CHAN_X ) ||
          IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Z ) ) {
         FETCH( func, *inst, 0, 0, CHAN_Y );
         FETCH( func, *inst, 4, 1, CHAN_Y );
      }
      IF_IS_DST0_CHANNEL_ENABLED( *inst, CHAN_X ) {
         emit_MOV( func, 2, 0 );
         emit_mul( func, 2, 1 );
         emit_MOV( func, 5, 3 );
         emit_mul( func, 5, 4 );
         emit_sub( func, 2, 5 );
         STORE( func, *inst, 2, 0, CHAN_X );
      }
      if( IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Y ) ||
          IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Z ) ) {
         FETCH( func, *inst, 2, 1, CHAN_X );
         FETCH( func, *inst, 5, 0, CHAN_X );
      }
      IF_IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Y ) {
         emit_mul( func, 3, 2 );
         emit_mul( func, 1, 5 );
         emit_sub( func, 3, 1 );
         STORE( func, *inst, 3, 0, CHAN_Y );
      }
      IF_IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Z ) {
         emit_mul( func, 5, 4 );
         emit_mul( func, 0, 2 );
         emit_sub( func, 5, 0 );
         STORE( func, *inst, 5, 0, CHAN_Z );
      }
      IF_IS_DST0_CHANNEL_ENABLED( *inst, CHAN_W ) {
	 emit_tempf(
	    func,
	    0,
	    TEMP_ONE_I,
	    TEMP_ONE_C );
         STORE( func, *inst, 0, 0, CHAN_W );
      }
      break;

   case TGSI_OPCODE_MULTIPLYMATRIX:
      return 0;
      break;

   case TGSI_OPCODE_ABS:
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( func, *inst, 0, 0, chan_index );
         emit_abs( func, 0) ;

         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_RCC:
      return 0;
      break;

   case TGSI_OPCODE_DPH:
      FETCH( func, *inst, 0, 0, CHAN_X );
      FETCH( func, *inst, 1, 1, CHAN_X );
      emit_mul( func, 0, 1 );
      FETCH( func, *inst, 1, 0, CHAN_Y );
      FETCH( func, *inst, 2, 1, CHAN_Y );
      emit_mul( func, 1, 2 );
      emit_add( func, 0, 1 );
      FETCH( func, *inst, 1, 0, CHAN_Z );
      FETCH( func, *inst, 2, 1, CHAN_Z );
      emit_mul( func, 1, 2 );
      emit_add( func, 0, 1 );
      FETCH( func, *inst, 1, 1, CHAN_W );
      emit_add( func, 0, 1 );
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_COS:
      FETCH( func, *inst, 0, 0, CHAN_X );
      emit_cos( func, 0, 0 );
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_DDX:
      return 0;
      break;

   case TGSI_OPCODE_DDY:
      return 0;
      break;

   case TGSI_OPCODE_KILP:
      /* predicated kill */
      emit_kilp( func );
      return 0; /* XXX fix me */
      break;

   case TGSI_OPCODE_KIL:
      /* conditional kill */
      emit_kil( func, &inst->FullSrcRegisters[0] );
      break;

   case TGSI_OPCODE_PK2H:
      return 0;
      break;

   case TGSI_OPCODE_PK2US:
      return 0;
      break;

   case TGSI_OPCODE_PK4B:
      return 0;
      break;

   case TGSI_OPCODE_PK4UB:
      return 0;
      break;

   case TGSI_OPCODE_RFL:
      return 0;
      break;

   case TGSI_OPCODE_SEQ:
      return 0;
      break;

   case TGSI_OPCODE_SFL:
      return 0;
      break;

   case TGSI_OPCODE_SGT:
      return 0;
      break;

   case TGSI_OPCODE_SIN:
      FETCH( func, *inst, 0, 0, CHAN_X );
      emit_sin( func, 0, 0 );
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_SLE:
      return 0;
      break;

   case TGSI_OPCODE_SNE:
      return 0;
      break;

   case TGSI_OPCODE_STR:
      return 0;
      break;

   case TGSI_OPCODE_TEX:
      if (0) {
	 /* Disable dummy texture code: 
	  */
	 emit_tempf(
	    func,
	    0,
	    TEMP_ONE_I,
	    TEMP_ONE_C );
	 FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
	    STORE( func, *inst, 0, 0, chan_index );
	 }
      }
      else {
	 return 0;
      }
      break;

   case TGSI_OPCODE_TXD:
      return 0;
      break;

   case TGSI_OPCODE_UP2H:
      return 0;
      break;

   case TGSI_OPCODE_UP2US:
      return 0;
      break;

   case TGSI_OPCODE_UP4B:
      return 0;
      break;

   case TGSI_OPCODE_UP4UB:
      return 0;
      break;

   case TGSI_OPCODE_X2D:
      return 0;
      break;

   case TGSI_OPCODE_ARA:
      return 0;
      break;

   case TGSI_OPCODE_ARR:
      return 0;
      break;

   case TGSI_OPCODE_BRA:
      return 0;
      break;

   case TGSI_OPCODE_CAL:
      return 0;
      break;

   case TGSI_OPCODE_RET:
      emit_ret( func );
      break;

   case TGSI_OPCODE_END:
      break;

   case TGSI_OPCODE_SSG:
      return 0;
      break;

   case TGSI_OPCODE_CMP:
      emit_cmp (func, inst);
      break;

   case TGSI_OPCODE_SCS:
      IF_IS_DST0_CHANNEL_ENABLED( *inst, CHAN_X ) {
         FETCH( func, *inst, 0, 0, CHAN_X );
         emit_cos( func, 0, 0 );
         STORE( func, *inst, 0, 0, CHAN_X );
      }
      IF_IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Y ) {
         FETCH( func, *inst, 0, 0, CHAN_X );
         emit_sin( func, 0, 0 );
         STORE( func, *inst, 0, 0, CHAN_Y );
      }
      IF_IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Z ) {
	 emit_tempf(
	    func,
	    0,
	    TGSI_EXEC_TEMP_00000000_I,
	    TGSI_EXEC_TEMP_00000000_C );
         STORE( func, *inst, 0, 0, CHAN_Z );
      }
      IF_IS_DST0_CHANNEL_ENABLED( *inst, CHAN_W ) {
	 emit_tempf(
	    func,
	    0,
	    TEMP_ONE_I,
	    TEMP_ONE_C );
         STORE( func, *inst, 0, 0, CHAN_W );
      }
      break;

   case TGSI_OPCODE_TXB:
      return 0;
      break;

   case TGSI_OPCODE_NRM:
      return 0;
      break;

   case TGSI_OPCODE_DIV:
      return 0;
      break;

   case TGSI_OPCODE_DP2:
      return 0;
      break;

   case TGSI_OPCODE_TXL:
      return 0;
      break;

   case TGSI_OPCODE_BRK:
      return 0;
      break;

   case TGSI_OPCODE_IF:
      return 0;
      break;

   case TGSI_OPCODE_LOOP:
      return 0;
      break;

   case TGSI_OPCODE_REP:
      return 0;
      break;

   case TGSI_OPCODE_ELSE:
      return 0;
      break;

   case TGSI_OPCODE_ENDIF:
      return 0;
      break;

   case TGSI_OPCODE_ENDLOOP:
      return 0;
      break;

   case TGSI_OPCODE_ENDREP:
      return 0;
      break;

   case TGSI_OPCODE_PUSHA:
      return 0;
      break;

   case TGSI_OPCODE_POPA:
      return 0;
      break;

   case TGSI_OPCODE_CEIL:
      return 0;
      break;

   case TGSI_OPCODE_I2F:
      return 0;
      break;

   case TGSI_OPCODE_NOT:
      return 0;
      break;

   case TGSI_OPCODE_TRUNC:
      return 0;
      break;

   case TGSI_OPCODE_SHL:
      return 0;
      break;

   case TGSI_OPCODE_SHR:
      return 0;
      break;

   case TGSI_OPCODE_AND:
      return 0;
      break;

   case TGSI_OPCODE_OR:
      return 0;
      break;

   case TGSI_OPCODE_MOD:
      return 0;
      break;

   case TGSI_OPCODE_XOR:
      return 0;
      break;

   case TGSI_OPCODE_SAD:
      return 0;
      break;

   case TGSI_OPCODE_TXF:
      return 0;
      break;

   case TGSI_OPCODE_TXQ:
      return 0;
      break;

   case TGSI_OPCODE_CONT:
      return 0;
      break;

   case TGSI_OPCODE_EMIT:
      return 0;
      break;

   case TGSI_OPCODE_ENDPRIM:
      return 0;
      break;

   default:
      return 0;
   }
#endif
   
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

#if 0
static void aos_to_soa( struct x86_function *func, 
                        uint arg_aos,
                        uint arg_soa, 
                        uint arg_num, 
                        uint arg_stride )
{
   struct x86_reg soa_input = x86_make_reg( file_REG32, reg_AX );
   struct x86_reg aos_input = x86_make_reg( file_REG32, reg_BX );
   struct x86_reg num_inputs = x86_make_reg( file_REG32, reg_CX );
   struct x86_reg stride = x86_make_reg( file_REG32, reg_DX );
   int inner_loop;


   /* Save EBX */
   x86_push( func, x86_make_reg( file_REG32, reg_BX ) );

   x86_mov( func, aos_input,  x86_fn_arg( func, arg_aos ) );
   x86_mov( func, soa_input,  x86_fn_arg( func, arg_soa ) );
   x86_mov( func, num_inputs, x86_fn_arg( func, arg_num ) );
   x86_mov( func, stride,     x86_fn_arg( func, arg_stride ) );

   /* do */
   inner_loop = x86_get_label( func );
   {
      x86_push( func, aos_input );
      sse_movlps( func, make_xmm( 0 ), x86_make_disp( aos_input, 0 ) );
      sse_movlps( func, make_xmm( 3 ), x86_make_disp( aos_input, 8 ) );
      x86_add( func, aos_input, stride );
      sse_movhps( func, make_xmm( 0 ), x86_make_disp( aos_input, 0 ) );
      sse_movhps( func, make_xmm( 3 ), x86_make_disp( aos_input, 8 ) );
      x86_add( func, aos_input, stride );
      sse_movlps( func, make_xmm( 1 ), x86_make_disp( aos_input, 0 ) );
      sse_movlps( func, make_xmm( 4 ), x86_make_disp( aos_input, 8 ) );
      x86_add( func, aos_input, stride );
      sse_movhps( func, make_xmm( 1 ), x86_make_disp( aos_input, 0 ) );
      sse_movhps( func, make_xmm( 4 ), x86_make_disp( aos_input, 8 ) );
      x86_pop( func, aos_input );

      sse_movaps( func, make_xmm( 2 ), make_xmm( 0 ) );
      sse_movaps( func, make_xmm( 5 ), make_xmm( 3 ) );
      sse_shufps( func, make_xmm( 0 ), make_xmm( 1 ), 0x88 );
      sse_shufps( func, make_xmm( 2 ), make_xmm( 1 ), 0xdd );
      sse_shufps( func, make_xmm( 3 ), make_xmm( 4 ), 0x88 );
      sse_shufps( func, make_xmm( 5 ), make_xmm( 4 ), 0xdd );

      sse_movups( func, x86_make_disp( soa_input, 0 ), make_xmm( 0 ) );
      sse_movups( func, x86_make_disp( soa_input, 16 ), make_xmm( 2 ) );
      sse_movups( func, x86_make_disp( soa_input, 32 ), make_xmm( 3 ) );
      sse_movups( func, x86_make_disp( soa_input, 48 ), make_xmm( 5 ) );

      /* Advance to next input */
      x86_lea( func, aos_input, x86_make_disp(aos_input, 16) );
      x86_lea( func, soa_input, x86_make_disp(soa_input, 64) );
   }
   /* while --num_inputs */
   x86_dec( func, num_inputs );
   x86_jcc( func, cc_NE, inner_loop );

   /* Restore EBX */
   x86_pop( func, aos_input );
}
#endif

#if 0
static void soa_to_aos( struct x86_function *func, uint aos, uint soa, uint num, uint stride )
{
   struct x86_reg soa_output;
   struct x86_reg aos_output;
   struct x86_reg num_outputs;
   struct x86_reg temp;
   int inner_loop;

   soa_output = x86_make_reg( file_REG32, reg_AX );
   aos_output = x86_make_reg( file_REG32, reg_BX );
   num_outputs = x86_make_reg( file_REG32, reg_CX );
   temp = x86_make_reg( file_REG32, reg_DX );

   /* Save EBX */
   x86_push( func, aos_output );

   x86_mov( func, soa_output, x86_fn_arg( func, soa ) );
   x86_mov( func, aos_output, x86_fn_arg( func, aos ) );
   x86_mov( func, num_outputs, x86_fn_arg( func, num ) );

   /* do */
   inner_loop = x86_get_label( func );
   {
      sse_movups( func, make_xmm( 0 ), x86_make_disp( soa_output, 0 ) );
      sse_movups( func, make_xmm( 1 ), x86_make_disp( soa_output, 16 ) );
      sse_movups( func, make_xmm( 3 ), x86_make_disp( soa_output, 32 ) );
      sse_movups( func, make_xmm( 4 ), x86_make_disp( soa_output, 48 ) );

      sse_movaps( func, make_xmm( 2 ), make_xmm( 0 ) );
      sse_movaps( func, make_xmm( 5 ), make_xmm( 3 ) );
      sse_unpcklps( func, make_xmm( 0 ), make_xmm( 1 ) );
      sse_unpckhps( func, make_xmm( 2 ), make_xmm( 1 ) );
      sse_unpcklps( func, make_xmm( 3 ), make_xmm( 4 ) );
      sse_unpckhps( func, make_xmm( 5 ), make_xmm( 4 ) );

      x86_mov( func, temp, x86_fn_arg( func, stride ) );
      x86_push( func, aos_output );
      sse_movlps( func, x86_make_disp( aos_output, 0 ), make_xmm( 0 ) );
      sse_movlps( func, x86_make_disp( aos_output, 8 ), make_xmm( 3 ) );
      x86_add( func, aos_output, temp );
      sse_movhps( func, x86_make_disp( aos_output, 0 ), make_xmm( 0 ) );
      sse_movhps( func, x86_make_disp( aos_output, 8 ), make_xmm( 3 ) );
      x86_add( func, aos_output, temp );
      sse_movlps( func, x86_make_disp( aos_output, 0 ), make_xmm( 2 ) );
      sse_movlps( func, x86_make_disp( aos_output, 8 ), make_xmm( 5 ) );
      x86_add( func, aos_output, temp );
      sse_movhps( func, x86_make_disp( aos_output, 0 ), make_xmm( 2 ) );
      sse_movhps( func, x86_make_disp( aos_output, 8 ), make_xmm( 5 ) );
      x86_pop( func, aos_output );

      /* Advance to next output */
      x86_lea( func, aos_output, x86_make_disp(aos_output, 16) );
      x86_lea( func, soa_output, x86_make_disp(soa_output, 64) );
   }
   /* while --num_outputs */
   x86_dec( func, num_outputs );
   x86_jcc( func, cc_NE, inner_loop );

   /* Restore EBX */
   x86_pop( func, aos_output );
}
#endif


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
   struct tgsi_parse_context parse;
   /*boolean instruction_phase = FALSE;*/
   unsigned ok = 1;
   uint num_immediates = 0;
   struct gen_context gen;

   util_init_math();

   tgsi_parse_init( &parse, tokens );

   gen.f = func;
   gen.inputs_reg = 3;   /* first function param */
   gen.outputs_reg = 4;  /* second function param */
   gen.temps_reg = 5;    /* ... */
   gen.immed_reg = 6;
   gen.const_reg = 7;

   emit_prologue(func);

   /*
    * Different function args for vertex/fragment shaders:
    */
#if 0
   if (parse.FullHeader.Processor.Processor == TGSI_PROCESSOR_FRAGMENT) {
      /* DECLARATION phase, do not load output argument. */
      x86_mov(
         func,
         get_input_base(),
         x86_fn_arg( func, 1 ) );
      /* skipping outputs argument here */
      x86_mov(
         func,
         get_const_base(),
         x86_fn_arg( func, 3 ) );
      x86_mov(
         func,
         get_temp_base(),
         x86_fn_arg( func, 4 ) );
      x86_mov(
         func,
         get_coef_base(),
         x86_fn_arg( func, 5 ) );
      x86_mov(
         func,
         get_immediate_base(),
         x86_fn_arg( func, 6 ) );
   }
   else {
      assert(parse.FullHeader.Processor.Processor == TGSI_PROCESSOR_VERTEX);

      if (do_swizzles)
         aos_to_soa( func, 
                     6,         /* aos_input */
                     1,         /* machine->input */
                     7,         /* num_inputs */
                     8 );       /* input_stride */

      x86_mov(
         func,
         get_input_base(),
         x86_fn_arg( func, 1 ) );
      x86_mov(
         func,
         get_output_base(),
         x86_fn_arg( func, 2 ) );
      x86_mov(
         func,
         get_const_base(),
         x86_fn_arg( func, 3 ) );
      x86_mov(
         func,
         get_temp_base(),
         x86_fn_arg( func, 4 ) );
      x86_mov(
         func,
         get_immediate_base(),
         x86_fn_arg( func, 5 ) );
   }
#endif

   while (!tgsi_parse_end_of_tokens(&parse) && ok) {
      tgsi_parse_token(&parse);

      switch (parse.FullToken.Token.Type) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         if (parse.FullHeader.Processor.Processor == TGSI_PROCESSOR_FRAGMENT) {
            emit_declaration(func, &parse.FullToken.FullDeclaration );
         }
         break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         if (parse.FullHeader.Processor.Processor == TGSI_PROCESSOR_FRAGMENT) {
#if 0
            if( !instruction_phase ) {
               /* INSTRUCTION phase, overwrite coeff with output. */
               instruction_phase = TRUE;
               x86_mov(
                  func,
                  get_output_base(),
                  x86_fn_arg( func, 2 ) );
            }
#endif
         }

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

#if 0
   if (parse.FullHeader.Processor.Processor == TGSI_PROCESSOR_VERTEX) {
      if (do_swizzles)
         soa_to_aos( func, 9, 2, 10, 11 );
   }
#endif

   emit_epilogue(func);

   tgsi_parse_free( &parse );

   return ok;
}

#endif /* PIPE_ARCH_PPC */
