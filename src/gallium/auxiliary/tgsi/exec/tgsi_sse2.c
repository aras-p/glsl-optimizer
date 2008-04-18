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

#include "pipe/p_util.h"
#include "pipe/p_shader_tokens.h"
#include "tgsi/util/tgsi_parse.h"
#include "tgsi/util/tgsi_util.h"
#include "tgsi_exec.h"
#include "tgsi_sse2.h"

#include "rtasm/rtasm_x86sse.h"

#if defined(__i386__) || defined(__386__)

#define HIGH_PRECISION 1  /* for 1/sqrt() */

#define DUMP_SSE  0

#if DUMP_SSE

static void
_print_reg(
   struct x86_reg reg )
{
   if (reg.mod != mod_REG) 
      debug_printf( "[" );
      
   switch( reg.file ) {
   case file_REG32:
      switch( reg.idx ) {
      case reg_AX:
         debug_printf( "EAX" );
         break;
      case reg_CX:
         debug_printf( "ECX" );
         break;
      case reg_DX:
         debug_printf( "EDX" );
         break;
      case reg_BX:
         debug_printf( "EBX" );
         break;
      case reg_SP:
         debug_printf( "ESP" );
         break;
      case reg_BP:
         debug_printf( "EBP" );
         break;
      case reg_SI:
         debug_printf( "ESI" );
         break;
      case reg_DI:
         debug_printf( "EDI" );
         break;
      }
      break;
   case file_MMX:
      assert( 0 );
      break;
   case file_XMM:
      debug_printf( "XMM%u", reg.idx );
      break;
   case file_x87:
      assert( 0 );
      break;
   }

   if (reg.mod == mod_DISP8 ||
       reg.mod == mod_DISP32)
      debug_printf("+%d", reg.disp);

   if (reg.mod != mod_REG) 
      debug_printf( "]" );
}

static void
_fill(
   const char  *op )
{
   unsigned count = 10 - strlen( op );

   while( count-- ) {
      debug_printf( " " );
   }
}

#define DUMP_START() debug_printf( "\nsse-dump start ----------------" )
#define DUMP_END() debug_printf( "\nsse-dump end ----------------\n" )
#define DUMP( OP ) debug_printf( "\n%s", OP )
#define DUMP_I( OP, I ) do {\
   debug_printf( "\n%s", OP );\
   _fill( OP );\
   debug_printf( "%u", I ); } while( 0 )
#define DUMP_R( OP, R0 ) do {\
   debug_printf( "\n%s", OP );\
   _fill( OP );\
   _print_reg( R0 ); } while( 0 )
#define DUMP_RR( OP, R0, R1 ) do {\
   debug_printf( "\n%s", OP );\
   _fill( OP );\
   _print_reg( R0 );\
   debug_printf( ", " );\
   _print_reg( R1 ); } while( 0 )
#define DUMP_RRI( OP, R0, R1, I ) do {\
   debug_printf( "\n%s", OP );\
   _fill( OP );\
   _print_reg( R0 );\
   debug_printf( ", " );\
   _print_reg( R1 );\
   debug_printf( ", " );\
   debug_printf( "%u", I ); } while( 0 )

#else

#define DUMP_START()
#define DUMP_END()
#define DUMP( OP )
#define DUMP_I( OP, I )
#define DUMP_R( OP, R0 )
#define DUMP_RR( OP, R0, R1 )
#define DUMP_RRI( OP, R0, R1, I )

#endif

#define FOR_EACH_CHANNEL( CHAN )\
   for( CHAN = 0; CHAN < 4; CHAN++ )

#define IS_DST0_CHANNEL_ENABLED( INST, CHAN )\
   ((INST).FullDstRegisters[0].DstRegister.WriteMask & (1 << (CHAN)))

#define IF_IS_DST0_CHANNEL_ENABLED( INST, CHAN )\
   if( IS_DST0_CHANNEL_ENABLED( INST, CHAN ))

#define FOR_EACH_DST0_ENABLED_CHANNEL( INST, CHAN )\
   FOR_EACH_CHANNEL( CHAN )\
      IF_IS_DST0_CHANNEL_ENABLED( INST, CHAN )

#define CHAN_X 0
#define CHAN_Y 1
#define CHAN_Z 2
#define CHAN_W 3

#define TEMP_R0   TGSI_EXEC_TEMP_R0

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
#ifdef WIN32
   return x86_make_reg(
      file_REG32,
      reg_BX );
#else
   return x86_make_reg(
      file_REG32,
      reg_SI );
#endif
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
get_argument(
   unsigned index )
{
   return x86_make_disp(
      x86_make_reg( file_REG32, reg_SP ),
      (index + 1) * 4 );
}

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

/**
 * X86 rtasm wrappers.
 */

static void
emit_addps(
   struct x86_function  *func,
   struct x86_reg       dst,
   struct x86_reg       src )
{
   DUMP_RR( "ADDPS", dst, src );
   sse_addps( func, dst, src );
}

static void
emit_andnps(
   struct x86_function  *func,
   struct x86_reg       dst,
   struct x86_reg       src )
{
   DUMP_RR( "ANDNPS", dst, src );
   sse_andnps( func, dst, src );
}

static void
emit_andps(
   struct x86_function  *func,
   struct x86_reg       dst,
   struct x86_reg       src )
{
   DUMP_RR( "ANDPS", dst, src );
   sse_andps( func, dst, src );
}

static void
emit_call(
   struct x86_function  *func,
   void                 (* addr)() )
{
   struct x86_reg ecx = x86_make_reg( file_REG32, reg_CX );

   DUMP_I( "CALL", addr );
   x86_mov_reg_imm( func, ecx, (unsigned long) addr );
   x86_call( func, ecx );
}

static void
emit_cmpps(
   struct x86_function  *func,
   struct x86_reg       dst,
   struct x86_reg       src,
   enum sse_cc          cc )
{
   DUMP_RRI( "CMPPS", dst, src, cc );
   sse_cmpps( func, dst, src, cc );
}

static void
emit_cvttps2dq(
   struct x86_function  *func,
   struct x86_reg       dst,
   struct x86_reg       src )
{
   DUMP_RR( "CVTTPS2DQ", dst, src );
   sse2_cvttps2dq( func, dst, src );
}

static void
emit_maxps(
   struct x86_function  *func,
   struct x86_reg       dst,
   struct x86_reg       src )
{
   DUMP_RR( "MAXPS", dst, src );
   sse_maxps( func, dst, src );
}

static void
emit_minps(
   struct x86_function  *func,
   struct x86_reg       dst,
   struct x86_reg       src )
{
   DUMP_RR( "MINPS", dst, src );
   sse_minps( func, dst, src );
}

static void
emit_mov(
   struct x86_function  *func,
   struct x86_reg       dst,
   struct x86_reg       src )
{
   DUMP_RR( "MOV", dst, src );
   x86_mov( func, dst, src );
}

static void
emit_movaps(
   struct x86_function  *func,
   struct x86_reg       dst,
   struct x86_reg       src )
{
   DUMP_RR( "MOVAPS", dst, src );
   sse_movaps( func, dst, src );
}

static void
emit_movss(
   struct x86_function  *func,
   struct x86_reg       dst,
   struct x86_reg       src )
{
   DUMP_RR( "MOVSS", dst, src );
   sse_movss( func, dst, src );
}

static void
emit_movups(
   struct x86_function  *func,
   struct x86_reg       dst,
   struct x86_reg       src )
{
   DUMP_RR( "MOVUPS", dst, src );
   sse_movups( func, dst, src );
}

static void
emit_mulps(
   struct x86_function  *func,
   struct x86_reg       dst,
   struct x86_reg       src )
{
   DUMP_RR( "MULPS", dst, src );
   sse_mulps( func, dst, src );
}

static void
emit_or(
   struct x86_function  *func,
   struct x86_reg       dst,
   struct x86_reg       src )
{
   DUMP_RR( "OR", dst, src );
   x86_or( func, dst, src );
}

static void
emit_orps(
   struct x86_function  *func,
   struct x86_reg       dst,
   struct x86_reg       src )
{
   DUMP_RR( "ORPS", dst, src );
   sse_orps( func, dst, src );
}

static void
emit_pmovmskb(
   struct x86_function  *func,
   struct x86_reg       dst,
   struct x86_reg       src )
{
   DUMP_RR( "PMOVMSKB", dst, src );
   sse_pmovmskb( func, dst, src );
}

static void
emit_pop(
   struct x86_function  *func,
   struct x86_reg       dst )
{
   DUMP_R( "POP", dst );
   x86_pop( func, dst );
}

static void
emit_push(
   struct x86_function  *func,
   struct x86_reg       dst )
{
   DUMP_R( "PUSH", dst );
   x86_push( func, dst );
}

static void
emit_rcpps(
   struct x86_function  *func,
   struct x86_reg       dst,
   struct x86_reg       src )
{
   DUMP_RR( "RCPPS", dst, src );
   sse2_rcpps( func, dst, src );
}

#ifdef WIN32
static void
emit_retw(
   struct x86_function  *func,
   unsigned             size )
{
   DUMP_I( "RET", size );
   x86_retw( func, size );
}
#else
static void
emit_ret(
   struct x86_function  *func )
{
   DUMP( "RET" );
   x86_ret( func );
}
#endif

static void
emit_rsqrtps(
   struct x86_function  *func,
   struct x86_reg       dst,
   struct x86_reg       src )
{
   DUMP_RR( "RSQRTPS", dst, src );
   sse_rsqrtps( func, dst, src );
}

static void
emit_shufps(
   struct x86_function  *func,
   struct x86_reg       dst,
   struct x86_reg       src,
   unsigned char        shuf )
{
   DUMP_RRI( "SHUFPS", dst, src, shuf );
   sse_shufps( func, dst, src, shuf );
}

static void
emit_subps(
   struct x86_function  *func,
   struct x86_reg       dst,
   struct x86_reg       src )
{
   DUMP_RR( "SUBPS", dst, src );
   sse_subps( func, dst, src );
}

static void
emit_xorps(
   struct x86_function  *func,
   struct x86_reg       dst,
   struct x86_reg       src )
{
   DUMP_RR( "XORPS", dst, src );
   sse_xorps( func, dst, src );
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
   unsigned xmm,
   unsigned vec,
   unsigned chan )
{
   emit_movss(
      func,
      make_xmm( xmm ),
      get_const( vec, chan ) );
   emit_shufps(
      func,
      make_xmm( xmm ),
      make_xmm( xmm ),
      SHUF( 0, 0, 0, 0 ) );
}

static void
emit_immediate(
   struct x86_function *func,
   unsigned xmm,
   unsigned vec,
   unsigned chan )
{
   emit_movss(
      func,
      make_xmm( xmm ),
      get_immediate( vec, chan ) );
   emit_shufps(
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
   emit_movups(
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
   emit_movups(
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
   emit_movaps(
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
   emit_movss(
      func,
      make_xmm( xmm ),
      get_coef( vec, chan, member ) );
   emit_shufps(
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
   emit_movups(
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
   emit_movaps(
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
   emit_temps(
      func,
      xmm,
      vec + TGSI_EXEC_NUM_TEMPS,
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
   emit_push(
      func,
      get_const_base() );
   emit_push(
      func,
      get_input_base() );
   emit_push(
      func,
      get_output_base() );

   /* It is important on non-win32 platforms that temp base is pushed last.
    */
   emit_push(
      func,
      get_temp_base() );
}

static void
emit_pop_gp(
   struct x86_function *func )
{
   /* Restore GP registers in a reverse order.
    */
   emit_pop(
      func,
      get_temp_base() );
   emit_pop(
      func,
      get_output_base() );
   emit_pop(
      func,
      get_input_base() );
   emit_pop(
      func,
      get_const_base() );
}

static void
emit_func_call_dst(
   struct x86_function *func,
   unsigned xmm_dst,
   void (*code)() )
{
   emit_movaps(
      func,
      get_temp( TEMP_R0, 0 ),
      make_xmm( xmm_dst ) );

   emit_push_gp(
      func );

#ifdef WIN32
   emit_push(
      func,
      get_temp( TEMP_R0, 0 ) );
#endif

   emit_call(
      func,
      code );

   emit_pop_gp(
      func );

   emit_movaps(
      func,
      make_xmm( xmm_dst ),
      get_temp( TEMP_R0, 0 ) );
}

static void
emit_func_call_dst_src(
   struct x86_function *func,
   unsigned xmm_dst,
   unsigned xmm_src,
   void (*code)() )
{
   emit_movaps(
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
   emit_andps(
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
   emit_addps(
      func,
      make_xmm( xmm_dst ),
      make_xmm( xmm_src ) );
}

static void XSTDCALL
cos4f(
   float *store )
{
#ifdef WIN32
   store[0] = (float) cos( (double) store[0] );
   store[1] = (float) cos( (double) store[1] );
   store[2] = (float) cos( (double) store[2] );
   store[3] = (float) cos( (double) store[3] );
#else
   const unsigned X = TEMP_R0 * 16;
   store[X + 0] = cosf( store[X + 0] );
   store[X + 1] = cosf( store[X + 1] );
   store[X + 2] = cosf( store[X + 2] );
   store[X + 3] = cosf( store[X + 3] );
#endif
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

static void XSTDCALL
ex24f(
   float *store )
{
#ifdef WIN32
   store[0] = (float) pow( 2.0, (double) store[0] );
   store[1] = (float) pow( 2.0, (double) store[1] );
   store[2] = (float) pow( 2.0, (double) store[2] );
   store[3] = (float) pow( 2.0, (double) store[3] );
#else
   const unsigned X = TEMP_R0 * 16;
   store[X + 0] = powf( 2.0f, store[X + 0] );
   store[X + 1] = powf( 2.0f, store[X + 1] );
   store[X + 2] = powf( 2.0f, store[X + 2] );
   store[X + 3] = powf( 2.0f, store[X + 3] );
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
   emit_cvttps2dq(
      func,
      make_xmm( xmm ),
      make_xmm( xmm ) );
}

static void XSTDCALL
flr4f(
   float *store )
{
#ifdef WIN32
   const unsigned X = 0;
#else
   const unsigned X = TEMP_R0 * 16;
#endif
   store[X + 0] = (float) floor( (double) store[X + 0] );
   store[X + 1] = (float) floor( (double) store[X + 1] );
   store[X + 2] = (float) floor( (double) store[X + 2] );
   store[X + 3] = (float) floor( (double) store[X + 3] );
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

static void XSTDCALL
frc4f(
   float *store )
{
#ifdef WIN32
   const unsigned X = 0;
#else
   const unsigned X = TEMP_R0 * 16;
#endif
   store[X + 0] -= (float) floor( (double) store[X + 0] );
   store[X + 1] -= (float) floor( (double) store[X + 1] );
   store[X + 2] -= (float) floor( (double) store[X + 2] );
   store[X + 3] -= (float) floor( (double) store[X + 3] );
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

static void XSTDCALL
lg24f(
   float *store )
{
#ifdef WIN32
   const unsigned X = 0;
#else
   const unsigned X = TEMP_R0 * 16;
#endif
   store[X + 0] = LOG2( store[X + 0] );
   store[X + 1] = LOG2( store[X + 1] );
   store[X + 2] = LOG2( store[X + 2] );
   store[X + 3] = LOG2( store[X + 3] );
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
   emit_movups(
      func,
      make_xmm( xmm_dst ),
      make_xmm( xmm_src ) );
}

static void
emit_mul (struct x86_function *func,
          unsigned xmm_dst,
          unsigned xmm_src)
{
   emit_mulps(
      func,
      make_xmm( xmm_dst ),
      make_xmm( xmm_src ) );
}

static void
emit_neg(
   struct x86_function *func,
   unsigned xmm )
{
   emit_xorps(
      func,
      make_xmm( xmm ),
      get_temp(
         TGSI_EXEC_TEMP_80000000_I,
         TGSI_EXEC_TEMP_80000000_C ) );
}

static void XSTDCALL
pow4f(
   float *store )
{
#ifdef WIN32
   store[0] = (float) pow( (double) store[0], (double) store[4] );
   store[1] = (float) pow( (double) store[1], (double) store[5] );
   store[2] = (float) pow( (double) store[2], (double) store[6] );
   store[3] = (float) pow( (double) store[3], (double) store[7] );
#else
   const unsigned X = TEMP_R0 * 16;
   store[X + 0] = powf( store[X + 0], store[X + 4] );
   store[X + 1] = powf( store[X + 1], store[X + 5] );
   store[X + 2] = powf( store[X + 2], store[X + 6] );
   store[X + 3] = powf( store[X + 3], store[X + 7] );
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
   emit_rcpps(
      func,
      make_xmm( xmm_dst ),
      make_xmm( xmm_src ) );
}

#if HIGH_PRECISION
static void XSTDCALL
rsqrt4f(
   float *store )
{
#ifdef WIN32
   store[0] = 1.0F / (float) sqrt( (double) store[0] );
   store[1] = 1.0F / (float) sqrt( (double) store[1] );
   store[2] = 1.0F / (float) sqrt( (double) store[2] );
   store[3] = 1.0F / (float) sqrt( (double) store[3] );
#else
   const unsigned X = TEMP_R0 * 16;
   store[X + 0] = 1.0F / sqrt( store[X + 0] );
   store[X + 1] = 1.0F / sqrt( store[X + 1] );
   store[X + 2] = 1.0F / sqrt( store[X + 2] );
   store[X + 3] = 1.0F / sqrt( store[X + 3] );
#endif
}
#endif

static void
emit_rsqrt(
   struct x86_function *func,
   unsigned xmm_dst,
   unsigned xmm_src )
{
#if HIGH_PRECISION
   emit_func_call_dst_src(
      func,
      xmm_dst,
      xmm_src,
      rsqrt4f );
#else
   emit_rsqrtps(
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
   emit_orps(
      func,
      make_xmm( xmm ),
      get_temp(
         TGSI_EXEC_TEMP_80000000_I,
         TGSI_EXEC_TEMP_80000000_C ) );
}

static void XSTDCALL
sin4f(
   float *store )
{
#ifdef WIN32
   store[0] = (float) sin( (double) store[0] );
   store[1] = (float) sin( (double) store[1] );
   store[2] = (float) sin( (double) store[2] );
   store[3] = (float) sin( (double) store[3] );
#else
   const unsigned X = TEMP_R0 * 16;
   store[X + 0] = sinf( store[X + 0] );
   store[X + 1] = sinf( store[X + 1] );
   store[X + 2] = sinf( store[X + 2] );
   store[X + 3] = sinf( store[X + 3] );
#endif
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
   emit_subps(
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

   switch( swizzle ) {
   case TGSI_EXTSWIZZLE_X:
   case TGSI_EXTSWIZZLE_Y:
   case TGSI_EXTSWIZZLE_Z:
   case TGSI_EXTSWIZZLE_W:
      switch( reg->SrcRegister.File ) {
      case TGSI_FILE_CONSTANT:
         emit_const(
            func,
            xmm,
            reg->SrcRegister.Index,
            swizzle );
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
         TGSI_EXEC_TEMP_ONE_I,
         TGSI_EXEC_TEMP_ONE_C );
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

   emit_push(
      func,
      x86_make_reg( file_REG32, reg_AX ) );
   emit_push(
      func,
      x86_make_reg( file_REG32, reg_DX ) );

   FOR_EACH_CHANNEL( chan_index ) {
      if( uniquemask & (1 << chan_index) ) {
         emit_cmpps(
            func,
            make_xmm( registers[chan_index] ),
            get_temp(
               TGSI_EXEC_TEMP_00000000_I,
               TGSI_EXEC_TEMP_00000000_C ),
            cc_LessThan );

         if( chan_index == firstchan ) {
            emit_pmovmskb(
               func,
               x86_make_reg( file_REG32, reg_AX ),
               make_xmm( registers[chan_index] ) );
         }
         else {
            emit_pmovmskb(
               func,
               x86_make_reg( file_REG32, reg_DX ),
               make_xmm( registers[chan_index] ) );
            emit_or(
               func,
               x86_make_reg( file_REG32, reg_AX ),
               x86_make_reg( file_REG32, reg_DX ) );
         }
      }
   }

   emit_or(
      func,
      get_temp(
         TGSI_EXEC_TEMP_KILMASK_I,
         TGSI_EXEC_TEMP_KILMASK_C ),
      x86_make_reg( file_REG32, reg_AX ) );

   emit_pop(
      func,
      x86_make_reg( file_REG32, reg_DX ) );
   emit_pop(
      func,
      x86_make_reg( file_REG32, reg_AX ) );
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
      emit_cmpps(
         func,
         make_xmm( 0 ),
         make_xmm( 1 ),
         cc );
      emit_andps(
         func,
         make_xmm( 0 ),
         get_temp(
            TGSI_EXEC_TEMP_ONE_I,
            TGSI_EXEC_TEMP_ONE_C ) );
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
      emit_cmpps(
         func,
         make_xmm( 0 ),
         get_temp(
            TGSI_EXEC_TEMP_00000000_I,
            TGSI_EXEC_TEMP_00000000_C ),
         cc_LessThan );
      emit_andps(
         func,
         make_xmm( 1 ),
         make_xmm( 0 ) );
      emit_andnps(
         func,
         make_xmm( 0 ),
         make_xmm( 2 ) );
      emit_orps(
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

   switch( inst->Instruction.Opcode ) {
   case TGSI_OPCODE_ARL:
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         FETCH( func, *inst, 0, 0, chan_index );
         emit_f2it( func, 0 );
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_MOV:
   /* TGSI_OPCODE_SWZ */
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
            TGSI_EXEC_TEMP_ONE_I,
            TGSI_EXEC_TEMP_ONE_C);
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
            emit_maxps(
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
            emit_maxps(
               func,
               make_xmm( 1 ),
               get_temp(
                  TGSI_EXEC_TEMP_00000000_I,
                  TGSI_EXEC_TEMP_00000000_C ) );
            /* XMM[2] = SrcReg[0].wwww */
            FETCH( func, *inst, 2, 0, CHAN_W );
            /* XMM[2] = min(XMM[2], 128.0) */
            emit_minps(
               func,
               make_xmm( 2 ),
               get_temp(
                  TGSI_EXEC_TEMP_128_I,
                  TGSI_EXEC_TEMP_128_C ) );
            /* XMM[2] = max(XMM[2], -128.0) */
            emit_maxps(
               func,
               make_xmm( 2 ),
               get_temp(
                  TGSI_EXEC_TEMP_MINUS_128_I,
                  TGSI_EXEC_TEMP_MINUS_128_C ) );
            emit_pow( func, 1, 2 );
            FETCH( func, *inst, 0, 0, CHAN_X );
            emit_xorps(
               func,
               make_xmm( 2 ),
               make_xmm( 2 ) );
            emit_cmpps(
               func,
               make_xmm( 2 ),
               make_xmm( 0 ),
               cc_LessThanEqual );
            emit_andps(
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
      emit_rsqrt( func, 0, 0 );
      FOR_EACH_DST0_ENABLED_CHANNEL( *inst, chan_index ) {
         STORE( func, *inst, 0, 0, chan_index );
      }
      break;

   case TGSI_OPCODE_EXP:
      return 0;
      break;

   case TGSI_OPCODE_LOG:
      return 0;
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
            TGSI_EXEC_TEMP_ONE_I,
            TGSI_EXEC_TEMP_ONE_C );
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
         emit_minps(
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
         emit_maxps(
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
	    TGSI_EXEC_TEMP_ONE_I,
	    TGSI_EXEC_TEMP_ONE_C );
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

   case TGSI_OPCODE_KIL:
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
	    TGSI_EXEC_TEMP_ONE_I,
	    TGSI_EXEC_TEMP_ONE_C );
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
   case TGSI_OPCODE_END:
#ifdef WIN32
      emit_retw( func, 16 );
#else
      emit_ret( func );
#endif
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
	    TGSI_EXEC_TEMP_ONE_I,
	    TGSI_EXEC_TEMP_ONE_C );
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

      assert( decl->Declaration.Declare == TGSI_DECLARE_RANGE );

      first = decl->u.DeclarationRange.First;
      last = decl->u.DeclarationRange.Last;
      mask = decl->Declaration.UsageMask;

      for( i = first; i <= last; i++ ) {
         for( j = 0; j < NUM_CHANNELS; j++ ) {
            if( mask & (1 << j) ) {
               switch( decl->Interpolation.Interpolate ) {
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
   float (*immediates)[4])
{
   struct tgsi_parse_context parse;
   boolean instruction_phase = FALSE;
   unsigned ok = 1;
   uint num_immediates = 0;

   DUMP_START();

   func->csr = func->store;

   tgsi_parse_init( &parse, tokens );

   /*
    * Different function args for vertex/fragment shaders:
    */
   if (parse.FullHeader.Processor.Processor == TGSI_PROCESSOR_FRAGMENT) {
      /* DECLARATION phase, do not load output argument. */
      emit_mov(
         func,
         get_input_base(),
         get_argument( 0 ) );
      /* skipping outputs argument here */
      emit_mov(
         func,
         get_const_base(),
         get_argument( 2 ) );
      emit_mov(
         func,
         get_temp_base(),
         get_argument( 3 ) );
      emit_mov(
         func,
         get_coef_base(),
         get_argument( 4 ) );
      emit_mov(
         func,
         get_immediate_base(),
         get_argument( 5 ) );
   }
   else {
      assert(parse.FullHeader.Processor.Processor == TGSI_PROCESSOR_VERTEX);

      emit_mov(
         func,
         get_input_base(),
         get_argument( 0 ) );
      emit_mov(
         func,
         get_output_base(),
         get_argument( 1 ) );
      emit_mov(
         func,
         get_const_base(),
         get_argument( 2 ) );
      emit_mov(
         func,
         get_temp_base(),
         get_argument( 3 ) );
      emit_mov(
         func,
         get_immediate_base(),
         get_argument( 4 ) );
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
               emit_mov(
                  func,
                  get_output_base(),
                  get_argument( 1 ) );
            }
         }

         ok = emit_instruction(
            func,
            &parse.FullToken.FullInstruction );

	 if (!ok) {
	    debug_printf("failed to translate tgsi opcode %d to SSE\n", 
			 parse.FullToken.FullInstruction.Instruction.Opcode );
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

   tgsi_parse_free( &parse );

   DUMP_END();

   return ok;
}

#endif /* i386 */
