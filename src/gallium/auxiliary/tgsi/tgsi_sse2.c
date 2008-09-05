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

#include "pipe/p_debug.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_math.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "tgsi_exec.h"
#include "tgsi_sse2.h"

#include "rtasm/rtasm_x86sse.h"

#ifdef PIPE_ARCH_X86

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


/**
 * Data fetch helpers.
 */

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

/**
 * Function call helpers.
 */

static void
emit_push_gp(
   struct x86_function *func )
{
   x86_push(
      func,
      x86_make_reg( file_REG32, reg_AX) );
   x86_push(
      func,
      x86_make_reg( file_REG32, reg_CX) );
   x86_push(
      func,
      x86_make_reg( file_REG32, reg_DX) );
}

static void
x86_pop_gp(
   struct x86_function *func )
{
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
}

static void
emit_func_call_dst(
   struct x86_function *func,
   unsigned xmm_dst,
   void (PIPE_CDECL *code)() )
{
   sse_movaps(
      func,
      get_temp( TEMP_R0, 0 ),
      make_xmm( xmm_dst ) );

   emit_push_gp(
      func );

   {
      struct x86_reg ecx = x86_make_reg( file_REG32, reg_CX );

      x86_lea(
         func,
         ecx,
         get_temp( TEMP_R0, 0 ) );

      x86_push( func, ecx );
      x86_mov_reg_imm( func, ecx, (unsigned long) code );
      x86_call( func, ecx );
      x86_pop(func, ecx ); 
   }


   x86_pop_gp(
      func );

   sse_movaps(
      func,
      make_xmm( xmm_dst ),
      get_temp( TEMP_R0, 0 ) );
}

static void
emit_func_call_dst_src(
   struct x86_function *func,
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
      xmm_dst,
      code );
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
   unsigned xmm_dst )
{
   emit_func_call_dst(
      func,
      xmm_dst,
      cos4f );
}

static void PIPE_CDECL
ex24f(
   float *store )
{
#if FAST_MATH
   store[0] = util_fast_exp2( store[0] );
   store[1] = util_fast_exp2( store[1] );
   store[2] = util_fast_exp2( store[2] );
   store[3] = util_fast_exp2( store[3] );
#else
   store[0] = powf( 2.0f, store[0] );
   store[1] = powf( 2.0f, store[1] );
   store[2] = powf( 2.0f, store[2] );
   store[3] = powf( 2.0f, store[3] );
#endif
}

static void
emit_ex2(
   struct x86_function *func,
   unsigned xmm_dst )
{
   emit_func_call_dst(
      func,
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
   unsigned xmm_dst )
{
   emit_func_call_dst(
      func,
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
   unsigned xmm_dst )
{
   emit_func_call_dst(
      func,
      xmm_dst,
      frc4f );
}

static void PIPE_CDECL
lg24f(
   float *store )
{
   store[0] = util_fast_log2( store[0] );
   store[1] = util_fast_log2( store[1] );
   store[2] = util_fast_log2( store[2] );
   store[3] = util_fast_log2( store[3] );
}

static void
emit_lg2(
   struct x86_function *func,
   unsigned xmm_dst )
{
   emit_func_call_dst(
      func,
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
pow4f(
   float *store )
{
#if FAST_MATH
   store[0] = util_fast_pow( store[0], store[4] );
   store[1] = util_fast_pow( store[1], store[5] );
   store[2] = util_fast_pow( store[2], store[6] );
   store[3] = util_fast_pow( store[3], store[7] );
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
   unsigned xmm_dst,
   unsigned xmm_src )
{
   emit_func_call_dst_src(
      func,
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
          unsigned xmm_dst)
{
   emit_func_call_dst(
      func,
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

/**
 * Register fetch.
 */

static void
emit_fetch(
   struct x86_function *func,
   unsigned xmm,
   const struct tgsi_full_src_register *reg,
   const unsigned chan_index )
{
   unsigned swizzle = tgsi_util_get_full_src_register_extswizzle( reg, chan_index );

   switch (swizzle) {
   case TGSI_EXTSWIZZLE_X:
   case TGSI_EXTSWIZZLE_Y:
   case TGSI_EXTSWIZZLE_Z:
   case TGSI_EXTSWIZZLE_W:
      switch (reg->SrcRegister.File) {
      case TGSI_FILE_CONSTANT:
         emit_const(
            func,
            xmm,
            reg->SrcRegister.Index,
            swizzle,
            reg->SrcRegister.Indirect,
            reg->SrcRegisterInd.File,
            reg->SrcRegisterInd.Index );
         break;

      case TGSI_FILE_IMMEDIATE:
         emit_immediate(
            func,
            xmm,
            reg->SrcRegister.Index,
            swizzle );
         break;

      case TGSI_FILE_INPUT:
         emit_inputf(
            func,
            xmm,
            reg->SrcRegister.Index,
            swizzle );
         break;

      case TGSI_FILE_TEMPORARY:
         emit_tempf(
            func,
            xmm,
            reg->SrcRegister.Index,
            swizzle );
         break;

      default:
         assert( 0 );
      }
      break;

   case TGSI_EXTSWIZZLE_ZERO:
      emit_tempf(
         func,
         xmm,
         TGSI_EXEC_TEMP_00000000_I,
         TGSI_EXEC_TEMP_00000000_C );
      break;

   case TGSI_EXTSWIZZLE_ONE:
      emit_tempf(
         func,
         xmm,
         TEMP_ONE_I,
         TEMP_ONE_C );
      break;

   default:
      assert( 0 );
   }

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
}

#define FETCH( FUNC, INST, XMM, INDEX, CHAN )\
   emit_fetch( FUNC, XMM, &(INST).FullSrcRegisters[INDEX], CHAN )

/**
 * Register store.
 */

static void
emit_store(
   struct x86_function *func,
   unsigned xmm,
   const struct tgsi_full_dst_register *reg,
   const struct tgsi_full_instruction *inst,
   unsigned chan_index )
{
   switch( reg->DstRegister.File ) {
   case TGSI_FILE_OUTPUT:
      emit_output(
         func,
         xmm,
         reg->DstRegister.Index,
         chan_index );
      break;

   case TGSI_FILE_TEMPORARY:
      emit_temps(
         func,
         xmm,
         reg->DstRegister.Index,
         chan_index );
      break;

   case TGSI_FILE_ADDRESS:
      emit_addrs(
         func,
         xmm,
         reg->DstRegister.Index,
         chan_index );
      break;

   default:
      assert( 0 );
   }

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
}

#define STORE( FUNC, INST, XMM, INDEX, CHAN )\
   emit_store( FUNC, XMM, &(INST).FullDstRegisters[INDEX], &(INST), CHAN )

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

static int
emit_instruction(
   struct x86_function *func,
   struct tgsi_full_instruction *inst )
{
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
            emit_pow( func, 1, 2 );
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
            emit_flr( func, 1 );
            /* dst.x = ex2(floor(src.x)) */
            if (IS_DST0_CHANNEL_ENABLED( *inst, CHAN_X )) {
               emit_MOV( func, 2, 1 );
               emit_ex2( func, 2 );
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
            emit_ex2( func, 0 );
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
         emit_lg2( func, 1 );
         /* dst.z = lg2(abs(src.x)) */
         if (IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Z )) {
            STORE( func, *inst, 1, 0, CHAN_Z );
         }
         if (IS_DST0_CHANNEL_ENABLED( *inst, CHAN_X ) ||
             IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Y )) {
            emit_flr( func, 1 );
            /* dst.x = floor(lg2(abs(src.x))) */
            if (IS_DST0_CHANNEL_ENABLED( *inst, CHAN_X )) {
               STORE( func, *inst, 1, 0, CHAN_X );
            }
            /* dst.x = abs(src)/ex2(floor(lg2(abs(src.x)))) */
            if (IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Y )) {
               emit_ex2( func, 1 );
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
         emit_frc( func, 0 );
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
         emit_flr( func, 0 );
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_ROUND:
      return 0;
      break;

   case TGSI_OPCODE_EXPBASE2:
   /* TGSI_OPCODE_EX2 */
      FETCH( func, *inst, 0, 0, CHAN_X );
      emit_ex2( func, 0 );
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_LOGBASE2:
   /* TGSI_OPCODE_LG2 */
      FETCH( func, *inst, 0, 0, CHAN_X );
      emit_lg2( func, 0 );
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_POWER:
   /* TGSI_OPCODE_POW */
      FETCH( func, *inst, 0, 0, CHAN_X );
      FETCH( func, *inst, 1, 1, CHAN_X );
      emit_pow( func, 0, 1 );
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
      emit_cos( func, 0 );
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
      emit_sin( func, 0 );
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
         emit_cos( func, 0 );
         STORE( func, *inst, 0, 0, CHAN_X );
      }
      IF_IS_DST0_CHANNEL_ENABLED( *inst, CHAN_Y ) {
         FETCH( func, *inst, 0, 0, CHAN_X );
         emit_sin( func, 0 );
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
   
   return 1;
}

static void
emit_declaration(
   struct x86_function *func,
   struct tgsi_full_declaration *decl )
{
   if( decl->Declaration.File == TGSI_FILE_INPUT ) {
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
   }
}

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

/**
 * Translate a TGSI vertex/fragment shader to SSE2 code.
 * Slightly different things are done for vertex vs. fragment shaders.
 *
 * Note that fragment shaders are responsible for interpolating shader
 * inputs. Because on x86 we have only 4 GP registers, and here we
 * have 5 shader arguments (input, output, const, temp and coef), the
 * code is split into two phases -- DECLARATION and INSTRUCTION phase.
 * GP register holding the output argument is aliased with the coeff
 * argument, as outputs are not needed in the DECLARATION phase.
 *
 * \param tokens  the TGSI input shader
 * \param func  the output SSE code/function
 * \param immediates  buffer to place immediates, later passed to SSE func
 * \param return  1 for success, 0 if translation failed
 */
unsigned
tgsi_emit_sse2(
   const struct tgsi_token *tokens,
   struct x86_function *func,
   float (*immediates)[4],
   boolean do_swizzles )
{
   struct tgsi_parse_context parse;
   boolean instruction_phase = FALSE;
   unsigned ok = 1;
   uint num_immediates = 0;

   util_init_math();

   func->csr = func->store;

   tgsi_parse_init( &parse, tokens );

   /* Can't just use EDI, EBX without save/restoring them:
    */
   x86_push(
      func,
      get_immediate_base() );

   x86_push(
      func,
      get_temp_base() );


   /*
    * Different function args for vertex/fragment shaders:
    */
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

   while( !tgsi_parse_end_of_tokens( &parse ) && ok ) {
      tgsi_parse_token( &parse );

      switch( parse.FullToken.Token.Type ) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         if (parse.FullHeader.Processor.Processor == TGSI_PROCESSOR_FRAGMENT) {
            emit_declaration(
               func,
               &parse.FullToken.FullDeclaration );
         }
         break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         if (parse.FullHeader.Processor.Processor == TGSI_PROCESSOR_FRAGMENT) {
            if( !instruction_phase ) {
               /* INSTRUCTION phase, overwrite coeff with output. */
               instruction_phase = TRUE;
               x86_mov(
                  func,
                  get_output_base(),
                  x86_fn_arg( func, 2 ) );
            }
         }

         ok = emit_instruction(
            func,
            &parse.FullToken.FullInstruction );

	 if (!ok) {
	    debug_printf("failed to translate tgsi opcode %d to SSE (%s)\n", 
			 parse.FullToken.FullInstruction.Instruction.Opcode,
                         parse.FullHeader.Processor.Processor == TGSI_PROCESSOR_VERTEX ?
                         "vertex shader" : "fragment shader");
	 }
         break;

      case TGSI_TOKEN_TYPE_IMMEDIATE:
         /* simply copy the immediate values into the next immediates[] slot */
         {
            const uint size = parse.FullToken.FullImmediate.Immediate.Size - 1;
            uint i;
            assert(size <= 4);
            assert(num_immediates < TGSI_EXEC_NUM_IMMEDIATES);
            for( i = 0; i < size; i++ ) {
               immediates[num_immediates][i] =
		  parse.FullToken.FullImmediate.u.ImmediateFloat32[i].Float;
            }
#if 0
            debug_printf("SSE FS immediate[%d] = %f %f %f %f\n",
                   num_immediates,
                   immediates[num_immediates][0],
                   immediates[num_immediates][1],
                   immediates[num_immediates][2],
                   immediates[num_immediates][3]);
#endif
            num_immediates++;
         }
         break;

      default:
	 ok = 0;
         assert( 0 );
      }
   }

   if (parse.FullHeader.Processor.Processor == TGSI_PROCESSOR_VERTEX) {
      if (do_swizzles)
         soa_to_aos( func, 9, 2, 10, 11 );
   }

   /* Can't just use EBX, EDI without save/restoring them:
    */
   x86_pop(
      func,
      get_temp_base() );

   x86_pop(
      func,
      get_immediate_base() );

   emit_ret( func );

   tgsi_parse_free( &parse );

   return ok;
}

#endif /* PIPE_ARCH_X86 */

