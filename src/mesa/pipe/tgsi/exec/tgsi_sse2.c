#include "tgsi_platform.h"
#include "tgsi_core.h"

#if defined USE_X86_ASM

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

#define ALIGN16( ADDR ) (((ADDR) + 15) & ~15)

static struct x86_reg
get_argument(
   unsigned index )
{
   return x86_make_disp(
      x86_make_reg( file_REG32, reg_SP ),
      (index + 1) * 4 );
}

static struct x86_reg
make_xmm(
   unsigned xmm )
{
   return x86_make_reg(
      file_XMM,
      (enum x86_reg_name) xmm );
}

static struct x86_reg
get_const_base( void )
{
   return x86_make_reg(
      file_REG32,
      reg_CX );
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
get_input_base( void )
{
   return x86_make_reg(
      file_REG32,
      reg_AX );
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
get_output_base( void )
{
   return x86_make_reg(
      file_REG32,
      reg_DX );
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
get_temp_base( void )
{
   return x86_make_reg(
      file_REG32,
      reg_BX );
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
get_addr(
   unsigned vec,
   unsigned chan )
{
   return get_temp(
      vec + TGSI_EXEC_NUM_TEMPS,
      chan );
}

static void
emit_const(
   struct x86_function *func,
   unsigned xmm,
   unsigned vec,
   unsigned chan )
{
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

static void
emit_input(
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

static void
emit_temps (struct x86_function *func,
            unsigned xmm,
            unsigned vec,
            unsigned chan)
{
    sse_movaps (func,
                get_temp (vec, chan),
                make_xmm (xmm));
}

static void
emit_addrf( struct x86_function *func,
	    unsigned xmm,
	    unsigned vec,
	    unsigned chan )
{
   emit_tempf( func, xmm, vec + TGSI_EXEC_NUM_TEMPS, chan );
}

static void
emit_addrs( struct x86_function *func,
	    unsigned xmm,
	    unsigned vec,
	    unsigned chan )
{
   emit_temps( func, xmm, vec + TGSI_EXEC_NUM_TEMPS, chan );
}

static void
emit_abs (struct x86_function *func,
          unsigned xmm)
{
    sse_andps (func,
               make_xmm (xmm),
               get_temp (TGSI_EXEC_TEMP_7FFFFFFF_I,
                         TGSI_EXEC_TEMP_7FFFFFFF_C));
}

static void
emit_neg (struct x86_function *func,
          unsigned xmm)
{
    sse_xorps (func,
               make_xmm (xmm),
               get_temp (TGSI_EXEC_TEMP_80000000_I,
                         TGSI_EXEC_TEMP_80000000_C));
}

static void
emit_setsign (struct x86_function *func,
              unsigned xmm)
{
    sse_orps (func,
              make_xmm (xmm),
              get_temp (TGSI_EXEC_TEMP_80000000_I,
                        TGSI_EXEC_TEMP_80000000_C));
}

static void
emit_add (struct x86_function *func,
          unsigned xmm_dst,
          unsigned xmm_src)
{
    sse_addps (func,
               make_xmm (xmm_dst),
               make_xmm (xmm_src));
}

static GLfloat g_cos_storage[4 + 3];

static void
cos4f (void)
{
    GLfloat *store = (GLfloat *) ALIGN16((unsigned) g_cos_storage);

#ifdef WIN32
    store[0] = (GLfloat) cos ((GLdouble) store[0]);
    store[1] = (GLfloat) cos ((GLdouble) store[1]);
    store[2] = (GLfloat) cos ((GLdouble) store[2]);
    store[3] = (GLfloat) cos ((GLdouble) store[3]);
#else
    store[0] = cosf (store[0]);
    store[1] = cosf (store[1]);
    store[2] = cosf (store[2]);
    store[3] = cosf (store[3]);
#endif
}

static void
emit_cos (struct x86_function *func,
          unsigned xmm_dst)
{
    x86_push (func,
              x86_make_reg (file_REG32, reg_AX));
    x86_mov_reg_imm (func,
                     x86_make_reg (file_REG32, reg_AX),
                     ALIGN16((GLint) g_cos_storage));
    sse_movaps (func,
                x86_deref (x86_make_reg (file_REG32, reg_AX)),
                make_xmm (xmm_dst));
    x86_push (func,
              x86_make_reg (file_REG32, reg_AX));
    x86_push (func,
              x86_make_reg (file_REG32, reg_BX));
    x86_push (func,
              x86_make_reg (file_REG32, reg_CX));
    x86_push (func,
              x86_make_reg (file_REG32, reg_DX));
    x86_call (func,
              (GLubyte *) cos4f);
    x86_pop (func,
             x86_make_reg (file_REG32, reg_DX));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_CX));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_BX));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_AX));
    sse_movaps (func,
                make_xmm (xmm_dst),
                x86_deref (x86_make_reg (file_REG32, reg_AX)));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_AX));
}

static GLfloat g_sin_storage[4 + 3];

static void
sin4f (void)
{
    GLfloat *store = (GLfloat *) ALIGN16((unsigned) g_sin_storage);

#ifdef WIN32
    store[0] = (GLfloat) sin ((GLdouble) store[0]);
    store[1] = (GLfloat) sin ((GLdouble) store[1]);
    store[2] = (GLfloat) sin ((GLdouble) store[2]);
    store[3] = (GLfloat) sin ((GLdouble) store[3]);
#else
    store[0] = sin (store[0]);
    store[1] = sin (store[1]);
    store[2] = sin (store[2]);
    store[3] = sin (store[3]);
#endif
}

static void
emit_sin (struct x86_function *func,
          unsigned xmm_dst)
{
    x86_push (func,
              x86_make_reg (file_REG32, reg_AX));
    x86_mov_reg_imm (func,
                     x86_make_reg (file_REG32, reg_AX),
                     ALIGN16((GLint) g_sin_storage));
    sse_movaps (func,
                x86_deref (x86_make_reg (file_REG32, reg_AX)),
                make_xmm (xmm_dst));
    x86_push (func,
              x86_make_reg (file_REG32, reg_AX));
    x86_push (func,
              x86_make_reg (file_REG32, reg_BX));
    x86_push (func,
              x86_make_reg (file_REG32, reg_CX));
    x86_push (func,
              x86_make_reg (file_REG32, reg_DX));
    x86_call (func,
              (GLubyte *) sin4f);
    x86_pop (func,
             x86_make_reg (file_REG32, reg_DX));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_CX));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_BX));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_AX));
    sse_movaps (func,
                make_xmm (xmm_dst),
                x86_deref (x86_make_reg (file_REG32, reg_AX)));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_AX));
}

static void
emit_mov (struct x86_function *func,
          unsigned xmm_dst,
          unsigned xmm_src)
{
    sse_movups (func,
                make_xmm (xmm_dst),
                make_xmm (xmm_src));
}

static void
emit_mul (struct x86_function *func,
          unsigned xmm_dst,
          unsigned xmm_src)
{
    sse_mulps (func,
               make_xmm (xmm_dst),
               make_xmm (xmm_src));
}

static GLfloat g_pow_storage[4 + 4 + 3];

static void
pow4f (void)
{
    GLfloat *store = (GLfloat *) ALIGN16((unsigned) g_pow_storage);

#ifdef WIN32
    store[0] = (GLfloat) pow ((GLdouble) store[0], (GLdouble) store[4]);
    store[1] = (GLfloat) pow ((GLdouble) store[1], (GLdouble) store[5]);
    store[2] = (GLfloat) pow ((GLdouble) store[2], (GLdouble) store[6]);
    store[3] = (GLfloat) pow ((GLdouble) store[3], (GLdouble) store[7]);
#else
    store[0] = powf (store[0], store[4]);
    store[1] = powf (store[1], store[5]);
    store[2] = powf (store[2], store[6]);
    store[3] = powf (store[3], store[7]);
#endif
}

static void
emit_pow (struct x86_function *func,
          unsigned xmm_dst,
          unsigned xmm_src)
{
    x86_push (func,
              x86_make_reg (file_REG32, reg_AX));
    x86_mov_reg_imm (func,
                     x86_make_reg (file_REG32, reg_AX),
                     ALIGN16((GLint) g_pow_storage));
    sse_movaps (func,
                x86_make_disp (x86_make_reg (file_REG32, reg_AX), 0),
                make_xmm (xmm_dst));
    sse_movaps (func,
                x86_make_disp (x86_make_reg (file_REG32, reg_AX), 16),
                make_xmm (xmm_src));
    x86_push (func,
              x86_make_reg (file_REG32, reg_AX));
    x86_push (func,
              x86_make_reg (file_REG32, reg_BX));
    x86_push (func,
              x86_make_reg (file_REG32, reg_CX));
    x86_push (func,
              x86_make_reg (file_REG32, reg_DX));
    x86_call (func,
              (GLubyte *) pow4f);
    x86_pop (func,
             x86_make_reg (file_REG32, reg_DX));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_CX));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_BX));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_AX));
    sse_movaps (func,
                make_xmm (xmm_dst),
                x86_deref (x86_make_reg (file_REG32, reg_AX)));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_AX));
}

static GLfloat g_ex2_storage[4 + 3];

static void
ex24f (void)
{
    GLfloat *store = (GLfloat *) ALIGN16((unsigned) g_ex2_storage);

    store[0] = (GLfloat) pow (2.0, (GLdouble) store[0]);
    store[1] = (GLfloat) pow (2.0, (GLdouble) store[1]);
    store[2] = (GLfloat) pow (2.0, (GLdouble) store[2]);
    store[3] = (GLfloat) pow (2.0, (GLdouble) store[3]);
}

static void
emit_ex2 (struct x86_function *func,
          unsigned xmm_dst)
{
    x86_push (func,
              x86_make_reg (file_REG32, reg_AX));
    x86_mov_reg_imm (func,
                     x86_make_reg (file_REG32, reg_AX),
                     ALIGN16((GLint) g_ex2_storage));
    sse_movaps (func,
                x86_deref (x86_make_reg (file_REG32, reg_AX)),
                make_xmm (xmm_dst));
    x86_push (func,
              x86_make_reg (file_REG32, reg_AX));
    x86_push (func,
              x86_make_reg (file_REG32, reg_BX));
    x86_push (func,
              x86_make_reg (file_REG32, reg_CX));
    x86_push (func,
              x86_make_reg (file_REG32, reg_DX));
    x86_call (func,
              (GLubyte *) ex24f);
    x86_pop (func,
             x86_make_reg (file_REG32, reg_DX));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_CX));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_BX));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_AX));
    sse_movaps (func,
                make_xmm (xmm_dst),
                x86_deref (x86_make_reg (file_REG32, reg_AX)));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_AX));
}

static GLfloat g_lg2_storage[4 + 3];

static void
lg24f (void)
{
    GLfloat *store = (GLfloat *) ALIGN16((unsigned) g_lg2_storage);

    store[0] = LOG2 (store[0]);
    store[1] = LOG2 (store[1]);
    store[2] = LOG2 (store[2]);
    store[3] = LOG2 (store[3]);
}

static void
emit_lg2 (struct x86_function *func,
          unsigned xmm_dst)
{
    x86_push (func,
              x86_make_reg (file_REG32, reg_AX));
    x86_mov_reg_imm (func,
                     x86_make_reg (file_REG32, reg_AX),
                     ALIGN16((GLint) g_lg2_storage));
    sse_movaps (func,
                x86_deref (x86_make_reg (file_REG32, reg_AX)),
                make_xmm (xmm_dst));
    x86_push (func,
              x86_make_reg (file_REG32, reg_AX));
    x86_push (func,
              x86_make_reg (file_REG32, reg_BX));
    x86_push (func,
              x86_make_reg (file_REG32, reg_CX));
    x86_push (func,
              x86_make_reg (file_REG32, reg_DX));
    x86_call (func,
              (GLubyte *) lg24f);
    x86_pop (func,
             x86_make_reg (file_REG32, reg_DX));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_CX));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_BX));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_AX));
    sse_movaps (func,
                make_xmm (xmm_dst),
                x86_deref (x86_make_reg (file_REG32, reg_AX)));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_AX));
}

static GLfloat g_flr_storage[4 + 3];

static void
flr4f (void)
{
    GLfloat *store = (GLfloat *) ALIGN16((unsigned) g_flr_storage);

    store[0] = (GLfloat) floor ((GLdouble) store[0]);
    store[1] = (GLfloat) floor ((GLdouble) store[1]);
    store[2] = (GLfloat) floor ((GLdouble) store[2]);
    store[3] = (GLfloat) floor ((GLdouble) store[3]);
}

static void
emit_flr (struct x86_function *func,
          unsigned xmm_dst)
{
    x86_push (func,
              x86_make_reg (file_REG32, reg_AX));
    x86_mov_reg_imm (func,
                     x86_make_reg (file_REG32, reg_AX),
                     ALIGN16((GLint) g_flr_storage));
    sse_movaps (func,
                x86_deref (x86_make_reg (file_REG32, reg_AX)),
                make_xmm (xmm_dst));
    x86_push (func,
              x86_make_reg (file_REG32, reg_AX));
    x86_push (func,
              x86_make_reg (file_REG32, reg_BX));
    x86_push (func,
              x86_make_reg (file_REG32, reg_CX));
    x86_push (func,
              x86_make_reg (file_REG32, reg_DX));
    x86_call (func,
              (GLubyte *) flr4f);
    x86_pop (func,
             x86_make_reg (file_REG32, reg_DX));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_CX));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_BX));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_AX));
    sse_movaps (func,
                make_xmm (xmm_dst),
                x86_deref (x86_make_reg (file_REG32, reg_AX)));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_AX));
}

static GLfloat g_frc_storage[4 + 3];

static void
frc4f (void)
{
    GLfloat *store = (GLfloat *) ALIGN16((unsigned) g_frc_storage);

    store[0] -= (GLfloat) floor ((GLdouble) store[0]);
    store[1] -= (GLfloat) floor ((GLdouble) store[1]);
    store[2] -= (GLfloat) floor ((GLdouble) store[2]);
    store[3] -= (GLfloat) floor ((GLdouble) store[3]);
}

static void
emit_frc (struct x86_function *func,
          unsigned xmm_dst)
{
    x86_push (func,
              x86_make_reg (file_REG32, reg_AX));
    x86_mov_reg_imm (func,
                     x86_make_reg (file_REG32, reg_AX),
                     ALIGN16((GLint) g_frc_storage));
    sse_movaps (func,
                x86_deref (x86_make_reg (file_REG32, reg_AX)),
                make_xmm (xmm_dst));
    x86_push (func,
              x86_make_reg (file_REG32, reg_AX));
    x86_push (func,
              x86_make_reg (file_REG32, reg_BX));
    x86_push (func,
              x86_make_reg (file_REG32, reg_CX));
    x86_push (func,
              x86_make_reg (file_REG32, reg_DX));
    x86_call (func,
              (GLubyte *) frc4f);
    x86_pop (func,
             x86_make_reg (file_REG32, reg_DX));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_CX));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_BX));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_AX));
    sse_movaps (func,
                make_xmm (xmm_dst),
                x86_deref (x86_make_reg (file_REG32, reg_AX)));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_AX));
}

static void
emit_rcp (struct x86_function *func,
          unsigned xmm_dst,
          unsigned xmm_src)
{
    sse2_rcpps (func,
                make_xmm (xmm_dst),
                make_xmm (xmm_src));
}

static void
emit_rsqrt (struct x86_function *func,
            unsigned xmm_dst,
            unsigned xmm_src)
{
    sse_rsqrtps (func,
                 make_xmm (xmm_dst),
                 make_xmm (xmm_src));
}

static void
emit_sub (struct x86_function *func,
          unsigned xmm_dst,
          unsigned xmm_src)
{
    sse_subps (func,
               make_xmm (xmm_dst),
               make_xmm (xmm_src));
}

static void
emit_fetch (struct x86_function *func,
            unsigned xmm,
            const struct tgsi_full_src_register *reg,
            const unsigned chan_index)
{
    unsigned swizzle = tgsi_util_get_full_src_register_extswizzle (reg, chan_index);

    switch (swizzle)
    {
    case TGSI_EXTSWIZZLE_X:
    case TGSI_EXTSWIZZLE_Y:
    case TGSI_EXTSWIZZLE_Z:
    case TGSI_EXTSWIZZLE_W:
        switch (reg->SrcRegister.File)
        {
        case TGSI_FILE_CONSTANT:
            emit_const (func, xmm, reg->SrcRegister.Index, swizzle);
            break;

        case TGSI_FILE_INPUT:
            emit_input (func, xmm, reg->SrcRegister.Index, swizzle);
            break;

        case TGSI_FILE_TEMPORARY:
            emit_tempf (func, xmm, reg->SrcRegister.Index, swizzle);
            break;

        default:
            assert (0);
        }
        break;

    case TGSI_EXTSWIZZLE_ZERO:
        emit_tempf (func,
                    xmm,
                    TGSI_EXEC_TEMP_00000000_I,
                    TGSI_EXEC_TEMP_00000000_C);
        break;

    case TGSI_EXTSWIZZLE_ONE:
        emit_tempf (func,
                    xmm,
                    TGSI_EXEC_TEMP_ONE_I,
                    TGSI_EXEC_TEMP_ONE_C);
        break;

    default:
        assert (0);
    }

    switch (tgsi_util_get_full_src_register_sign_mode (reg, chan_index))
    {
    case TGSI_UTIL_SIGN_CLEAR:
        emit_abs (func, xmm);
        break;

    case TGSI_UTIL_SIGN_SET:
        emit_setsign (func, xmm);
        break;

    case TGSI_UTIL_SIGN_TOGGLE:
        emit_neg (func, xmm);
        break;

    case TGSI_UTIL_SIGN_KEEP:
        break;
    }
}

static void
emit_store (struct x86_function *func,
            unsigned xmm,
            const struct tgsi_full_dst_register *reg,
            const struct tgsi_full_instruction *inst,
            unsigned chan_index)
{
    switch (reg->DstRegister.File)
    {
    case TGSI_FILE_OUTPUT:
        emit_output (func, xmm, reg->DstRegister.Index, chan_index);
        break;

    case TGSI_FILE_TEMPORARY:
        emit_temps (func, xmm, reg->DstRegister.Index, chan_index);
        break;

   case TGSI_FILE_ADDRESS:
      emit_addrs( func, xmm, reg->DstRegister.Index, chan_index );
      break;

    default:
        assert (0);
    }

    switch (inst->Instruction.Saturate)
    {
    case TGSI_SAT_NONE:
        break;

    case TGSI_SAT_ZERO_ONE:
//        assert (0);
        break;

    case TGSI_SAT_MINUS_PLUS_ONE:
        assert (0);
        break;
    }
}

#define FETCH(XMM,INDEX,CHAN)\
    emit_fetch (func, XMM, &inst->FullSrcRegisters[INDEX], CHAN)

#define STORE(XMM,INDEX,CHAN)\
    emit_store (func, XMM, &inst->FullDstRegisters[INDEX], inst, CHAN)

static void
emit_kil (struct x86_function *func,
          const struct tgsi_full_src_register *reg)
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

    for (chan_index = 0; chan_index < 4; chan_index++)
    {
        unsigned swizzle;

        /* unswizzle channel */
        swizzle = tgsi_util_get_full_src_register_extswizzle (reg, chan_index);

        /* check if the component has not been already tested */
        if (!(uniquemask & (1 << swizzle)))
        {
            uniquemask |= 1 << swizzle;

            /* allocate register */
            registers[chan_index] = nextregister;
            emit_fetch (func, nextregister, reg, chan_index);
            nextregister++;

            /* mark the first channel used */
            if (firstchan == ~0)
                firstchan = chan_index;
        }
    }

    x86_push (func,
              x86_make_reg (file_REG32, reg_AX));
    x86_push (func,
              x86_make_reg (file_REG32, reg_DX));

    for (chan_index = 0; chan_index < 4; chan_index++)
    {
        if (uniquemask & (1 << chan_index))
        {
            sse_cmpps (func,
                       make_xmm (registers[chan_index]),
                       get_temp (TGSI_EXEC_TEMP_00000000_I,
                                 TGSI_EXEC_TEMP_00000000_C),
                       cc_LessThan);

            if (chan_index == firstchan)
            {
                sse_pmovmskb (func,
                              x86_make_reg (file_REG32, reg_AX),
                              make_xmm (registers[chan_index]));
            }
            else
            {
                sse_pmovmskb (func,
                              x86_make_reg (file_REG32, reg_DX),
                              make_xmm (registers[chan_index]));
                x86_or (func,
                        x86_make_reg (file_REG32, reg_AX),
                        x86_make_reg (file_REG32, reg_DX));
            }
        }
    }

    x86_or (func,
            get_temp(TGSI_EXEC_TEMP_KILMASK_I,
                     TGSI_EXEC_TEMP_KILMASK_C),
            x86_make_reg (file_REG32, reg_AX));

    x86_pop (func,
             x86_make_reg (file_REG32, reg_DX));
    x86_pop (func,
             x86_make_reg (file_REG32, reg_AX));
}

static void
emit_setcc (struct x86_function *func,
            struct tgsi_full_instruction *inst,
            enum sse_cc cc)
{
    unsigned chan_index;

    FOR_EACH_ENABLED_CHANNEL
    {
        emit_fetch (func, 0, &inst->FullSrcRegisters[0], chan_index);
        emit_fetch (func, 1, &inst->FullSrcRegisters[1], chan_index);

        sse_cmpps (func,
                   make_xmm (0),
                   make_xmm (1),
                   cc);

        sse_andps (func,
                   make_xmm (0),
                   get_temp (TGSI_EXEC_TEMP_ONE_I,
                             TGSI_EXEC_TEMP_ONE_C));

        emit_store (func, 0, &inst->FullDstRegisters[0], inst, chan_index);
    }
}

static void
emit_cmp (struct x86_function *func,
          struct tgsi_full_instruction *inst)
{
    unsigned chan_index;

    FOR_EACH_ENABLED_CHANNEL
    {
        emit_fetch (func, 0, &inst->FullSrcRegisters[0], chan_index);
        emit_fetch (func, 1, &inst->FullSrcRegisters[1], chan_index);
        emit_fetch (func, 2, &inst->FullSrcRegisters[2], chan_index);

        sse_cmpps (func,
                   make_xmm (0),
                   get_temp (TGSI_EXEC_TEMP_00000000_I,
                             TGSI_EXEC_TEMP_00000000_C),
                   cc_LessThan);

        sse_andps (func,
                   make_xmm (1),
                   make_xmm (0));
        sse_andnps (func,
                    make_xmm (0),
                    make_xmm (2));
        sse_orps (func,
                  make_xmm (0),
                  make_xmm (1));

        emit_store (func, 0, &inst->FullDstRegisters[0], inst, chan_index);
    }
}

static void
emit_f2it( struct x86_function *func,
	   unsigned xmm )
{
   sse2_cvttps2dq( func, make_xmm( xmm ), make_xmm( xmm ) );
}

static void
emit_instruction (struct x86_function *func,
                  struct tgsi_full_instruction *inst)
{
    unsigned chan_index;

   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_ARL:
      FOR_EACH_ENABLED_CHANNEL {
	 FETCH( 0, 0, chan_index );
	 emit_f2it( func, 0 );
	 STORE( 0, 0, chan_index );
      }
      break;

    case TGSI_OPCODE_MOV:
    /* TGSI_OPCODE_SWZ */
        FOR_EACH_ENABLED_CHANNEL
        {
            FETCH(0, 0, chan_index);
            STORE(0, 0, chan_index);
        }
        break;

    case TGSI_OPCODE_LIT:
        if (IS_CHANNEL_ENABLED(CHAN_X) || IS_CHANNEL_ENABLED(CHAN_W))
        {
            emit_tempf (func, 0, TGSI_EXEC_TEMP_ONE_I, TGSI_EXEC_TEMP_ONE_C);
            if (IS_CHANNEL_ENABLED(CHAN_X))
                STORE(0, 0, CHAN_X);
            if (IS_CHANNEL_ENABLED(CHAN_W))
                STORE(0, 0, CHAN_W);
        }
        if (IS_CHANNEL_ENABLED(CHAN_Y) || IS_CHANNEL_ENABLED(CHAN_Z))
        {
            if (IS_CHANNEL_ENABLED(CHAN_Y))
            {
                FETCH(0, 0, CHAN_X);
                sse_maxps (func,
                           make_xmm (0),
                           get_temp (TGSI_EXEC_TEMP_00000000_I,
                                     TGSI_EXEC_TEMP_00000000_C));
                STORE(0, 0, CHAN_Y);
            }
            if (IS_CHANNEL_ENABLED(CHAN_Z))
            {
                FETCH(1, 0, CHAN_Y);
                sse_maxps (func,
                           make_xmm (1),
                           get_temp (TGSI_EXEC_TEMP_00000000_I,
                                     TGSI_EXEC_TEMP_00000000_C));

                FETCH(2, 0, CHAN_W);
                sse_minps (func,
                           make_xmm (2),
                           get_temp (TGSI_EXEC_TEMP_128_I,
                                     TGSI_EXEC_TEMP_128_C));
                sse_maxps (func,
                           make_xmm (2),
                           get_temp (TGSI_EXEC_TEMP_MINUS_128_I,
                                     TGSI_EXEC_TEMP_MINUS_128_C));

                emit_pow (func, 1, 2);

                FETCH(0, 0, CHAN_X);
                sse_xorps (func, make_xmm (2), make_xmm (2));
                sse_cmpps (func,
                           make_xmm (2),
                           make_xmm (0),
                           cc_LessThanEqual);
                sse_andps (func,
                           make_xmm (2),
                           make_xmm (1));

                emit_store (func, 2, &inst->FullDstRegisters[0], inst, CHAN_Z);
            }
        }
        break;

    case TGSI_OPCODE_RCP:
    /* TGSI_OPCODE_RECIP */
        FETCH(0, 0, CHAN_X);
        emit_rcp (func, 0, 0);

        FOR_EACH_ENABLED_CHANNEL
        {
            STORE(0, 0, chan_index);
        }
        break;

    case TGSI_OPCODE_RSQ:
    /* TGSI_OPCODE_RECIPSQRT */
        FETCH(0, 0, CHAN_X);
        emit_rsqrt (func, 0, 0);

        FOR_EACH_ENABLED_CHANNEL
        {
            STORE(0, 0, chan_index);
        }
        break;

    case TGSI_OPCODE_EXP:
        assert (0);
        break;

    case TGSI_OPCODE_LOG:
        assert (0);
        break;

    case TGSI_OPCODE_MUL:
        FOR_EACH_ENABLED_CHANNEL
        {
            FETCH(0, 0, chan_index);
            FETCH(1, 1, chan_index);
            emit_mul (func, 0, 1);

            STORE(0, 0, chan_index);
        }
        break;

    case TGSI_OPCODE_ADD:
        FOR_EACH_ENABLED_CHANNEL
        {
            FETCH(0, 0, chan_index);
            FETCH(1, 1, chan_index);
            emit_add (func, 0, 1);

            STORE(0, 0, chan_index);
        }
        break;

    case TGSI_OPCODE_DP3:
    /* TGSI_OPCODE_DOT3 */
        FETCH(0, 0, CHAN_X);
        FETCH(1, 1, CHAN_X);
        emit_mul (func, 0, 1);

        FETCH(1, 0, CHAN_Y);
        FETCH(2, 1, CHAN_Y);
        emit_mul (func, 1, 2);
        emit_add (func, 0, 1);

        FETCH(1, 0, CHAN_Z);
        FETCH(2, 1, CHAN_Z);
        emit_mul (func, 1, 2);
        emit_add (func, 0, 1);

        FOR_EACH_ENABLED_CHANNEL
        {
            STORE(0, 0, chan_index);
        }
        break;

    case TGSI_OPCODE_DP4:
    /* TGSI_OPCODE_DOT4 */
        FETCH(0, 0, CHAN_X);
        FETCH(1, 1, CHAN_X);
        emit_mul (func, 0, 1);

        FETCH(1, 0, CHAN_Y);
        FETCH(2, 1, CHAN_Y);
        emit_mul (func, 1, 2);
        emit_add (func, 0, 1);

        FETCH(1, 0, CHAN_Z);
        FETCH(2, 1, CHAN_Z);
        emit_mul (func, 1, 2);
        emit_add (func, 0, 1);

        FETCH(1, 0, CHAN_W);
        FETCH(2, 1, CHAN_W);
        emit_mul (func, 1, 2);
        emit_add (func, 0, 1);

        FOR_EACH_ENABLED_CHANNEL
        {
            STORE(0, 0, chan_index);
        }
        break;

    case TGSI_OPCODE_DST:
        IF_IS_CHANNEL_ENABLED(CHAN_X)
        {
            emit_tempf (func,
                        0,
                        TGSI_EXEC_TEMP_ONE_I,
                        TGSI_EXEC_TEMP_ONE_C);
            STORE(0, 0, CHAN_X);
        }
        IF_IS_CHANNEL_ENABLED(CHAN_Y)
        {
            FETCH(0, 0, CHAN_Y);
            FETCH(1, 1, CHAN_Y);
            emit_mul (func, 0, 1);
            STORE(0, 0, CHAN_Y);
        }
        IF_IS_CHANNEL_ENABLED(CHAN_Z)
        {
            FETCH(0, 0, CHAN_Z);
            STORE(0, 0, CHAN_Z);
        }
        IF_IS_CHANNEL_ENABLED(CHAN_W)
        {
            FETCH(0, 1, CHAN_W);
            STORE(0, 0, CHAN_W);
        }
        break;

    case TGSI_OPCODE_MIN:
        FOR_EACH_ENABLED_CHANNEL
        {
            FETCH(0, 0, chan_index);
            FETCH(1, 1, chan_index);
            sse_minps (func,
                       make_xmm (0),
                       make_xmm (1));
            STORE(0, 0, chan_index);
        }
        break;

    case TGSI_OPCODE_MAX:
        FOR_EACH_ENABLED_CHANNEL
        {
            FETCH(0, 0, chan_index);
            FETCH(1, 1, chan_index);
            sse_maxps (func,
                       make_xmm (0),
                       make_xmm (1));
            STORE(0, 0, chan_index);
        }
        break;

    case TGSI_OPCODE_SLT:
    /* TGSI_OPCODE_SETLT */
        emit_setcc (func, inst, cc_LessThan);
        break;

    case TGSI_OPCODE_SGE:
    /* TGSI_OPCODE_SETGE */
        emit_setcc (func, inst, cc_NotLessThan);
        break;

    case TGSI_OPCODE_MAD:
    /* TGSI_OPCODE_MADD */
        FOR_EACH_ENABLED_CHANNEL
        {
            FETCH(0, 0, chan_index);
            FETCH(1, 1, chan_index);
            FETCH(2, 2, chan_index);
            emit_mul (func, 0, 1);
            emit_add (func, 0, 2);

            STORE(0, 0, chan_index);
        }
        break;

    case TGSI_OPCODE_SUB:
        FOR_EACH_ENABLED_CHANNEL
        {
            FETCH(0, 0, chan_index);
            FETCH(1, 1, chan_index);
            emit_sub (func, 0, 1);

            STORE(0, 0, chan_index);
        }
        break;

    case TGSI_OPCODE_LERP:
    /* TGSI_OPCODE_LRP */
        FOR_EACH_ENABLED_CHANNEL
        {
            FETCH(0, 0, chan_index);
            FETCH(1, 1, chan_index);
            FETCH(2, 2, chan_index);
            emit_sub (func, 1, 2);
            emit_mul (func, 0, 1);
            emit_add (func, 0, 2);

            STORE(0, 0, chan_index);
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
        FOR_EACH_ENABLED_CHANNEL
        {
            FETCH(0, 0, chan_index);
            emit_frc (func, 0);
            STORE(0, 0, chan_index);
        }
        break;

    case TGSI_OPCODE_CLAMP:
        assert (0);
        break;

    case TGSI_OPCODE_FLOOR:
    /* TGSI_OPCODE_FLR */
        FOR_EACH_ENABLED_CHANNEL
        {
            FETCH(0, 0, chan_index);
            emit_flr (func, 0);
            STORE(0, 0, chan_index);
        }
        break;

    case TGSI_OPCODE_ROUND:
        assert (0);
        break;

    case TGSI_OPCODE_EXPBASE2:
    /* TGSI_OPCODE_EX2 */
        FETCH(0, 0, CHAN_X);
        emit_ex2 (func, 0);

        FOR_EACH_ENABLED_CHANNEL
        {
            STORE(0, 0, chan_index);
        }
        break;

    case TGSI_OPCODE_LOGBASE2:
    /* TGSI_OPCODE_LG2 */
        FETCH(0, 0, CHAN_X);
        emit_lg2 (func, 0);

        FOR_EACH_ENABLED_CHANNEL
        {
            STORE(0, 0, chan_index);
        }
        break;

    case TGSI_OPCODE_POWER:
    /* TGSI_OPCODE_POW */
        FETCH(0, 0, CHAN_X);
        FETCH(1, 1, CHAN_X);
        emit_pow (func, 0, 1);

        FOR_EACH_ENABLED_CHANNEL
        {
            STORE(0, 0, chan_index);
        }
        break;

    case TGSI_OPCODE_CROSSPRODUCT:
    /* TGSI_OPCODE_XPD */
        if (IS_CHANNEL_ENABLED(CHAN_X) || IS_CHANNEL_ENABLED(CHAN_Y))
        {
            FETCH(1, 1, CHAN_Z);
            FETCH(3, 0, CHAN_Z);
        }
        if (IS_CHANNEL_ENABLED(CHAN_X) || IS_CHANNEL_ENABLED(CHAN_Z))
        {
            FETCH(0, 0, CHAN_Y);
            FETCH(4, 1, CHAN_Y);
        }
        IF_IS_CHANNEL_ENABLED(CHAN_X)
        {
            emit_mov (func, 2, 0);
            emit_mul (func, 2, 1);
            emit_mov (func, 5, 3);
            emit_mul (func, 5, 4);
            emit_sub (func, 2, 5);
            STORE(2, 0, CHAN_X);
        }

        if (IS_CHANNEL_ENABLED(CHAN_Y) || IS_CHANNEL_ENABLED(CHAN_Z))
        {
            FETCH(2, 1, CHAN_X);
            FETCH(5, 0, CHAN_X);
        }
        IF_IS_CHANNEL_ENABLED(CHAN_Y)
        {
            emit_mul (func, 3, 2);
            emit_mul (func, 1, 5);
            emit_sub (func, 3, 1);
            STORE(3, 0, CHAN_Y);
        }

        IF_IS_CHANNEL_ENABLED(CHAN_Z)
        {
            emit_mul (func, 5, 4);
            emit_mul (func, 0, 2);
            emit_sub (func, 5, 0);
            STORE(5, 0, CHAN_Z);
        }

        IF_IS_CHANNEL_ENABLED(CHAN_W)
        {
            FETCH(0, TGSI_EXEC_TEMP_ONE_I, TGSI_EXEC_TEMP_ONE_C);
            STORE(0, 0, CHAN_W);
        }
        break;

    case TGSI_OPCODE_MULTIPLYMATRIX:
        assert (0);
        break;

    case TGSI_OPCODE_ABS:
        FOR_EACH_ENABLED_CHANNEL
        {
            FETCH(0, 0, chan_index);
            emit_abs (func, 0);

            STORE(0, 0, chan_index);
        }
        break;

    case TGSI_OPCODE_RCC:
        assert (0);
        break;

    case TGSI_OPCODE_DPH:
        FETCH(0, 0, CHAN_X);
        FETCH(1, 1, CHAN_X);
        emit_mul (func, 0, 1);

        FETCH(1, 0, CHAN_Y);
        FETCH(2, 1, CHAN_Y);
        emit_mul (func, 1, 2);
        emit_add (func, 0, 1);

        FETCH(1, 0, CHAN_Z);
        FETCH(2, 1, CHAN_Z);
        emit_mul (func, 1, 2);
        emit_add (func, 0, 1);

        FETCH(1, 1, CHAN_W);
        emit_add (func, 0, 1);

        FOR_EACH_ENABLED_CHANNEL
        {
            STORE(0, 0, chan_index);
        }
        break;

    case TGSI_OPCODE_COS:
        FETCH(0, 0, CHAN_X);
        emit_cos (func, 0);

        FOR_EACH_ENABLED_CHANNEL
        {
            STORE(0, 0, chan_index);
        }
        break;

    case TGSI_OPCODE_DDX:
        assert (0);
        break;

    case TGSI_OPCODE_DDY:
        assert (0);
        break;

    case TGSI_OPCODE_KIL:
        emit_kil (func, &inst->FullSrcRegisters[0]);
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
        assert (0);
        break;

    case TGSI_OPCODE_SIN:
        FETCH(0, 0, CHAN_X);
        emit_sin (func, 0);

        FOR_EACH_ENABLED_CHANNEL
        {
            STORE(0, 0, chan_index);
        }
        break;

    case TGSI_OPCODE_SLE:
        assert (0);
        break;

    case TGSI_OPCODE_SNE:
        assert (0);
        break;

    case TGSI_OPCODE_STR:
        assert (0);
        break;

    case TGSI_OPCODE_TEX:
        assert (0);
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
        assert (0);
        break;

    case TGSI_OPCODE_SSG:
        assert (0);
        break;

    case TGSI_OPCODE_CMP:
        emit_cmp (func, inst);
        break;

    case TGSI_OPCODE_SCS:
        IF_IS_CHANNEL_ENABLED(CHAN_X)
        {
            FETCH(0, 0, CHAN_X);
            emit_cos (func, 0);
            STORE(0, 0, CHAN_X);
        }

        IF_IS_CHANNEL_ENABLED(CHAN_Y)
        {
            FETCH(0, 0, CHAN_Y);
            emit_sin (func, 0);
            STORE(0, 0, CHAN_Y);
        }

        IF_IS_CHANNEL_ENABLED(CHAN_Z)
        {
            FETCH(0, TGSI_EXEC_TEMP_00000000_I, TGSI_EXEC_TEMP_00000000_C);
            STORE(0, 0, CHAN_Z);
        }

        IF_IS_CHANNEL_ENABLED(CHAN_W)
        {
            FETCH(0, TGSI_EXEC_TEMP_ONE_I, TGSI_EXEC_TEMP_ONE_C);
            STORE(0, 0, CHAN_W);
        }
        break;

    case TGSI_OPCODE_TXB:
        assert (0);
        break;

    case TGSI_OPCODE_NRM:
        assert (0);
        break;

    case TGSI_OPCODE_DIV:
        assert (0);
        break;

    case TGSI_OPCODE_DP2:
        assert (0);
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
        assert (0);
        break;

    case TGSI_OPCODE_I2F:
        assert (0);
        break;

    case TGSI_OPCODE_NOT:
        assert (0);
        break;

    case TGSI_OPCODE_TRUNC:
        assert (0);
        break;

    case TGSI_OPCODE_SHL:
        assert (0);
        break;

    case TGSI_OPCODE_SHR:
        assert (0);
        break;

    case TGSI_OPCODE_AND:
        assert (0);
        break;

    case TGSI_OPCODE_OR:
        assert (0);
        break;

    case TGSI_OPCODE_MOD:
        assert (0);
        break;

    case TGSI_OPCODE_XOR:
        assert (0);
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
        assert (0);
        break;

    case TGSI_OPCODE_ENDPRIM:
        assert (0);
        break;

    default:
        assert (0);
    }
}

GLboolean
tgsi_emit_sse (struct tgsi_token *tokens,
               struct x86_function *function)
{
    struct tgsi_parse_context parse;

    x86_init_func (function);

    x86_mov (function, get_input_base (), get_argument (0));
    x86_mov (function, get_output_base (), get_argument (1));
    x86_mov (function, get_const_base (), get_argument (2));
    x86_mov (function, get_temp_base (), get_argument (3));

    tgsi_parse_init (&parse, tokens);

    while (!tgsi_parse_end_of_tokens (&parse))
    {
        tgsi_parse_token (&parse);

        switch (parse.FullToken.Token.Type)
        {
        case TGSI_TOKEN_TYPE_DECLARATION:
            break;

        case TGSI_TOKEN_TYPE_INSTRUCTION:
            emit_instruction (function, &parse.FullToken.FullInstruction);
            break;

        default:
            assert (0);
        }
    }

    tgsi_parse_free (&parse);

#ifdef WIN32
    x86_retw (function, 16);
#else
    x86_ret (function);
#endif

    return GL_FALSE;
}

#endif
