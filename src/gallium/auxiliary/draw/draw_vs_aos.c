/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * Translate tgsi vertex programs to x86/x87/SSE/SSE2 machine code
 * using the rtasm runtime assembler.  Based on the old
 * t_vb_arb_program_sse.c
 */


#include "pipe/p_util.h"
#include "pipe/p_shader_tokens.h"
#include "tgsi/util/tgsi_parse.h"
#include "tgsi/util/tgsi_util.h"
#include "tgsi/exec/tgsi_exec.h"
#include "tgsi/util/tgsi_dump.h"

#include "draw_vs.h"
#include "draw_vs_aos.h"

#include "rtasm/rtasm_x86sse.h"

#ifdef PIPE_ARCH_X86


static INLINE boolean eq( struct x86_reg a,
			    struct x86_reg b )
{
   return (a.file == b.file &&
	   a.idx == b.idx &&
	   a.mod == b.mod &&
	   a.disp == b.disp);
}
      

static struct x86_reg get_reg_ptr(struct aos_compilation *cp,
                                  unsigned file,
				  unsigned idx )
{
   struct x86_reg ptr = cp->machine_EDX;

   switch (file) {
   case TGSI_FILE_INPUT:
      return x86_make_disp(ptr, Offset(struct aos_machine, input[idx]));

   case TGSI_FILE_OUTPUT:
      return x86_make_disp(ptr, Offset(struct aos_machine, output[idx]));

   case TGSI_FILE_TEMPORARY:
      return x86_make_disp(ptr, Offset(struct aos_machine, temp[idx]));

   case TGSI_FILE_IMMEDIATE:
      return x86_make_disp(ptr, Offset(struct aos_machine, immediate[idx]));

   case TGSI_FILE_CONSTANT:       
      return x86_make_disp(ptr, Offset(struct aos_machine, constant[idx]));

   case AOS_FILE_INTERNAL:
      return x86_make_disp(ptr, Offset(struct aos_machine, internal[idx]));

   default:
      ERROR(cp, "unknown reg file");
      return x86_make_reg(0,0);
   }
}
		


#define X87_CW_EXCEPTION_INV_OP       (1<<0)
#define X87_CW_EXCEPTION_DENORM_OP    (1<<1)
#define X87_CW_EXCEPTION_ZERO_DIVIDE  (1<<2)
#define X87_CW_EXCEPTION_OVERFLOW     (1<<3)
#define X87_CW_EXCEPTION_UNDERFLOW    (1<<4)
#define X87_CW_EXCEPTION_PRECISION    (1<<5)
#define X87_CW_PRECISION_SINGLE       (0<<8)
#define X87_CW_PRECISION_RESERVED     (1<<8)
#define X87_CW_PRECISION_DOUBLE       (2<<8)
#define X87_CW_PRECISION_DOUBLE_EXT   (3<<8)
#define X87_CW_PRECISION_MASK         (3<<8)
#define X87_CW_ROUND_NEAREST          (0<<10)
#define X87_CW_ROUND_DOWN             (1<<10)
#define X87_CW_ROUND_UP               (2<<10)
#define X87_CW_ROUND_ZERO             (3<<10)
#define X87_CW_ROUND_MASK             (3<<10)
#define X87_CW_INFINITY               (1<<12)

static void do_populate_lut( struct shine_tab *tab,
                             float unclamped_exponent )
{
   const float epsilon = 1.0F / 256.0F;    
   float exponent = CLAMP(unclamped_exponent, -(128.0F - epsilon), (128.0F - epsilon));
   unsigned i;

   tab->exponent = unclamped_exponent; /* for later comparison */
   
   tab->values[0] = 0;
   if (exponent == 0) {
      for (i = 1; i < 258; i++) {
         tab->values[i] = 1.0;
      }      
   }
   else {
      for (i = 1; i < 258; i++) {
         tab->values[i] = powf((float)i * epsilon, exponent);
      }
   }
}

static void init_internals( struct aos_machine *machine )
{
   unsigned i;
   float inv = 1.0f/255.0f;
   float f255 = 255.0f;

   ASSIGN_4V(machine->internal[IMM_SWZ],       1.0f,  -1.0f,  0.0f, 1.0f);
   *(unsigned *)&machine->internal[IMM_SWZ][3] = 0xffffffff;

   ASSIGN_4V(machine->internal[IMM_ONES],      1.0f,  1.0f,  1.0f,  1.0f);
   ASSIGN_4V(machine->internal[IMM_NEGS],     -1.0f, -1.0f, -1.0f, -1.0f);
   ASSIGN_4V(machine->internal[IMM_IDENTITY],  0.0f,  0.0f,  0.0f,  1.0f);
   ASSIGN_4V(machine->internal[IMM_INV_255],   inv,   inv,   inv,   inv);
   ASSIGN_4V(machine->internal[IMM_255],       f255,   f255,   f255,   f255);


   machine->fpu_rnd_nearest = (X87_CW_EXCEPTION_INV_OP |
                               X87_CW_EXCEPTION_DENORM_OP |
                               X87_CW_EXCEPTION_ZERO_DIVIDE |
                               X87_CW_EXCEPTION_OVERFLOW |
                               X87_CW_EXCEPTION_UNDERFLOW |
                               X87_CW_EXCEPTION_PRECISION |
                               (1<<6) |
                               X87_CW_ROUND_NEAREST |
                               X87_CW_PRECISION_DOUBLE_EXT);

   assert(machine->fpu_rnd_nearest == 0x37f);
                               
   machine->fpu_rnd_neg_inf = (X87_CW_EXCEPTION_INV_OP |
                               X87_CW_EXCEPTION_DENORM_OP |
                               X87_CW_EXCEPTION_ZERO_DIVIDE |
                               X87_CW_EXCEPTION_OVERFLOW |
                               X87_CW_EXCEPTION_UNDERFLOW |
                               X87_CW_EXCEPTION_PRECISION |
                               (1<<6) |
                               X87_CW_ROUND_DOWN |
                               X87_CW_PRECISION_DOUBLE_EXT);

   for (i = 0; i < MAX_SHINE_TAB; i++)
      do_populate_lut( &machine->shine_tab[i], 1.0f );
}


static void spill( struct aos_compilation *cp, unsigned idx )
{
   if (!cp->xmm[idx].dirty ||
       (cp->xmm[idx].file != TGSI_FILE_INPUT && /* inputs are fetched into xmm & set dirty */
        cp->xmm[idx].file != TGSI_FILE_OUTPUT &&
        cp->xmm[idx].file != TGSI_FILE_TEMPORARY)) {
      ERROR(cp, "invalid spill");
      return;
   }
   else {
      struct x86_reg oldval = get_reg_ptr(cp,
                                          cp->xmm[idx].file,
                                          cp->xmm[idx].idx);
      
      assert(cp->xmm[idx].dirty);
      sse_movaps(cp->func, oldval, x86_make_reg(file_XMM, idx));
      cp->xmm[idx].dirty = 0;
   }
}


static struct x86_reg get_xmm_writable( struct aos_compilation *cp,
                                        struct x86_reg reg )
{
   if (reg.file != file_XMM ||
       cp->xmm[reg.idx].file != TGSI_FILE_NULL)
   {
      struct x86_reg tmp = aos_get_xmm_reg(cp);
      sse_movaps(cp->func, tmp, reg);
      reg = tmp;
   }

   return reg;
}

static struct x86_reg get_xmm( struct aos_compilation *cp,
                               struct x86_reg reg )
{
   if (reg.file != file_XMM) 
   {
      struct x86_reg tmp = aos_get_xmm_reg(cp);
      sse_movaps(cp->func, tmp, reg);
      reg = tmp;
   }

   return reg;
}


/* Allocate an empty xmm register, either as a temporary or later to
 * "adopt" as a shader reg.
 */
struct x86_reg aos_get_xmm_reg( struct aos_compilation *cp )
{
   unsigned i;
   unsigned oldest = 0;
   boolean found = FALSE;

   for (i = 0; i < 8; i++) 
      if (cp->xmm[i].last_used != cp->insn_counter &&
          cp->xmm[i].file == TGSI_FILE_NULL) {
	 oldest = i;
         found = TRUE;
      }

   if (!found) {
      for (i = 0; i < 8; i++) 
         if (cp->xmm[i].last_used < cp->xmm[oldest].last_used)
            oldest = i;
   }

   /* Need to write out the old value?
    */
   if (cp->xmm[oldest].dirty) 
      spill(cp, oldest);

   assert(cp->xmm[oldest].last_used != cp->insn_counter);

   cp->xmm[oldest].file = TGSI_FILE_NULL;
   cp->xmm[oldest].idx = 0;
   cp->xmm[oldest].last_used = cp->insn_counter;
   return x86_make_reg(file_XMM, oldest);
}

void aos_release_xmm_reg( struct aos_compilation *cp,
                          unsigned idx )
{
   cp->xmm[idx].file = TGSI_FILE_NULL;
   cp->xmm[idx].idx = 0;
   cp->xmm[idx].dirty = 0;
   cp->xmm[idx].last_used = 0;
}



     
/* Mark an xmm reg as holding the current copy of a shader reg.
 */
void aos_adopt_xmm_reg( struct aos_compilation *cp,
                        struct x86_reg reg,
                        unsigned file,
                        unsigned idx,
                        unsigned dirty )
{
   unsigned i;

   if (reg.file != file_XMM) {
      assert(0);
      return;
   }

   /* If any xmm reg thinks it holds this shader reg, break the
    * illusion.
    */
   for (i = 0; i < 8; i++) {
      if (cp->xmm[i].file == file && 
          cp->xmm[i].idx == idx) {
         aos_release_xmm_reg(cp, i);
      }
   }

   cp->xmm[reg.idx].file = file;
   cp->xmm[reg.idx].idx = idx;
   cp->xmm[reg.idx].dirty = dirty;
   cp->xmm[reg.idx].last_used = cp->insn_counter;
}


/* Return a pointer to the in-memory copy of the reg, making sure it is uptodate.
 */
static struct x86_reg aos_get_shader_reg_ptr( struct aos_compilation *cp, 
                                              unsigned file,
                                              unsigned idx )
{
   unsigned i;

   /* Ensure the in-memory copy of this reg is up-to-date
    */
   for (i = 0; i < 8; i++) {
      if (cp->xmm[i].file == file && 
          cp->xmm[i].idx == idx &&
          cp->xmm[i].dirty) {
         spill(cp, i);
      }
   }

   return get_reg_ptr( cp, file, idx );
}


/* As above, but return a pointer.  Note - this pointer may alias
 * those returned by get_arg_ptr().
 */
static struct x86_reg get_dst_ptr( struct aos_compilation *cp, 
                                   const struct tgsi_full_dst_register *dst )
{
   unsigned file = dst->DstRegister.File;
   unsigned idx = dst->DstRegister.Index;
   unsigned i;
   

   /* Ensure in-memory copy of this reg is up-to-date and invalidate
    * any xmm copies.
    */
   for (i = 0; i < 8; i++) {
      if (cp->xmm[i].file == file &&
          cp->xmm[i].idx == idx)
      {
         if (cp->xmm[i].dirty) 
            spill(cp, i);
         
         aos_release_xmm_reg(cp, i);
      }
   }

   return get_reg_ptr( cp, file, idx );
}





/* Return an XMM reg if the argument is resident, otherwise return a
 * base+offset pointer to the saved value.
 */
struct x86_reg aos_get_shader_reg( struct aos_compilation *cp, 
                                   unsigned file,
                                   unsigned idx )
{
   unsigned i;

   for (i = 0; i < 8; i++) {
      if (cp->xmm[i].file == file &&
	  cp->xmm[i].idx  == idx) 
      {
	 cp->xmm[i].last_used = cp->insn_counter;
	 return x86_make_reg(file_XMM, i);
      }
   }

   /* If not found in the XMM register file, return an indirect
    * reference to the in-memory copy:
    */
   return get_reg_ptr( cp, file, idx );
}



static struct x86_reg aos_get_shader_reg_xmm( struct aos_compilation *cp, 
                                              unsigned file,
                                              unsigned idx )
{
   struct x86_reg reg = aos_get_shader_reg( cp, file, idx );
   return get_xmm( cp, reg );
}



struct x86_reg aos_get_internal_xmm( struct aos_compilation *cp,
                                     unsigned imm )
{
   return aos_get_shader_reg_xmm( cp, AOS_FILE_INTERNAL, imm );
}


struct x86_reg aos_get_internal( struct aos_compilation *cp,
                                 unsigned imm )
{
   return aos_get_shader_reg( cp, AOS_FILE_INTERNAL, imm );
}





/* Emulate pshufd insn in regular SSE, if necessary:
 */
static void emit_pshufd( struct aos_compilation *cp,
			 struct x86_reg dst,
			 struct x86_reg arg0,
			 ubyte shuf )
{
   if (cp->have_sse2) {
      sse2_pshufd(cp->func, dst, arg0, shuf);
   }
   else {
      if (!eq(dst, arg0)) 
	 sse_movaps(cp->func, dst, arg0);

      sse_shufps(cp->func, dst, dst, shuf);
   }
}

/* load masks (pack into negs??)
 * pshufd - shuffle according to writemask
 * and - result, mask
 * nand - dest, mask
 * or - dest, result
 */
static boolean mask_write( struct aos_compilation *cp,
                           struct x86_reg dst,
                           struct x86_reg result,
                           unsigned mask )
{
   struct x86_reg imm_swz = aos_get_internal_xmm(cp, IMM_SWZ);
   struct x86_reg tmp = aos_get_xmm_reg(cp);
   
   emit_pshufd(cp, tmp, imm_swz, 
               SHUF((mask & 1) ? 2 : 3,
                    (mask & 2) ? 2 : 3,
                    (mask & 4) ? 2 : 3,
                    (mask & 8) ? 2 : 3));

   sse_andps(cp->func, dst, tmp);
   sse_andnps(cp->func, tmp, result);
   sse_orps(cp->func, dst, tmp);

   aos_release_xmm_reg(cp, tmp.idx);
   return TRUE;
}




/* Helper for writemask:
 */
static boolean emit_shuf_copy2( struct aos_compilation *cp,
				  struct x86_reg dst,
				  struct x86_reg arg0,
				  struct x86_reg arg1,
				  ubyte shuf )
{
   struct x86_reg tmp = aos_get_xmm_reg(cp);

   emit_pshufd(cp, dst, arg1, shuf);
   emit_pshufd(cp, tmp, arg0, shuf);
   sse_shufps(cp->func, dst, tmp, SHUF(X, Y, Z, W));
   emit_pshufd(cp, dst, dst, shuf);

   aos_release_xmm_reg(cp, tmp.idx);
   return TRUE;
}



#define SSE_SWIZZLE_NOOP ((0<<0) | (1<<2) | (2<<4) | (3<<6))


/* Locate a source register and perform any required (simple) swizzle.  
 * 
 * Just fail on complex swizzles at this point.
 */
static struct x86_reg fetch_src( struct aos_compilation *cp, 
                                 const struct tgsi_full_src_register *src ) 
{
   struct x86_reg arg0 = aos_get_shader_reg(cp, 
                                            src->SrcRegister.File, 
                                            src->SrcRegister.Index);
   unsigned i;
   unsigned swz = 0;
   unsigned negs = 0;
   unsigned abs = 0;

   for (i = 0; i < 4; i++) {
      unsigned swizzle = tgsi_util_get_full_src_register_extswizzle( src, i );
      unsigned neg = tgsi_util_get_full_src_register_sign_mode( src, i );

      switch (swizzle) {
      case TGSI_EXTSWIZZLE_ZERO:
      case TGSI_EXTSWIZZLE_ONE:
         ERROR(cp, "not supporting full swizzles yet in tgsi_aos_sse2");
         break;

      default:
         swz |= (swizzle & 0x3) << (i * 2);
         break;
      }

      switch (neg) {
      case TGSI_UTIL_SIGN_TOGGLE:
         negs |= (1<<i);
         break;
         
      case TGSI_UTIL_SIGN_KEEP:
         break;

      case TGSI_UTIL_SIGN_CLEAR:
         abs |= (1<<i);
         break;

      default:
         ERROR(cp, "unsupported sign-mode");
         break;
      }
   }

   if (swz != SSE_SWIZZLE_NOOP || negs != 0 || abs != 0) {
      struct x86_reg dst = aos_get_xmm_reg(cp);

      if (swz != SSE_SWIZZLE_NOOP) {
         emit_pshufd(cp, dst, arg0, swz);
         arg0 = dst;
      }

      if (negs && negs != 0xf) {
         struct x86_reg imm_swz = aos_get_internal_xmm(cp, IMM_SWZ);
         struct x86_reg tmp = aos_get_xmm_reg(cp);

         /* Load 1,-1,0,0
          * Use neg as arg to pshufd
          * Multiply
          */
         emit_pshufd(cp, tmp, imm_swz, 
                     SHUF((negs & 1) ? 1 : 0,
                          (negs & 2) ? 1 : 0,
                          (negs & 4) ? 1 : 0,
                          (negs & 8) ? 1 : 0));
         sse_mulps(cp->func, dst, arg0);

         aos_release_xmm_reg(cp, tmp.idx);
         arg0 = dst;
      }
      else if (negs) {
         struct x86_reg imm_negs = aos_get_internal_xmm(cp, IMM_NEGS);
         sse_mulps(cp->func, dst, imm_negs);
         arg0 = dst;
      }


      if (abs && abs != 0xf) {
         ERROR(cp, "unsupported partial abs");
      }
      else if (abs) {
         struct x86_reg neg = aos_get_internal(cp, IMM_NEGS);
         struct x86_reg tmp = aos_get_xmm_reg(cp);

         sse_movaps(cp->func, tmp, arg0);
         sse_mulps(cp->func, tmp, neg);
         sse_maxps(cp->func, dst, arg0);

         aos_release_xmm_reg(cp, tmp.idx);
         arg0 = dst;
      }
   }
      
   return arg0;
}

static void x87_fld_src( struct aos_compilation *cp, 
                         const struct tgsi_full_src_register *src,
                         unsigned channel ) 
{
   struct x86_reg arg0 = aos_get_shader_reg_ptr(cp, 
                                                src->SrcRegister.File, 
                                                src->SrcRegister.Index);

   unsigned swizzle = tgsi_util_get_full_src_register_extswizzle( src, channel );
   unsigned neg = tgsi_util_get_full_src_register_sign_mode( src, channel );

   switch (swizzle) {
   case TGSI_EXTSWIZZLE_ZERO:
      x87_fldz( cp->func );
      break;

   case TGSI_EXTSWIZZLE_ONE:
      x87_fld1( cp->func );
      break;

   default:
      x87_fld( cp->func, x86_make_disp(arg0, (swizzle & 3) * sizeof(float)) );
      break;
   }
   

   switch (neg) {
   case TGSI_UTIL_SIGN_TOGGLE:
      /* Flip the sign:
       */
      x87_fchs( cp->func );
      break;
         
   case TGSI_UTIL_SIGN_KEEP:
      break;

   case TGSI_UTIL_SIGN_CLEAR:
      x87_fabs( cp->func );
      break;

   case TGSI_UTIL_SIGN_SET:
      x87_fabs( cp->func );
      x87_fchs( cp->func );
      break;

   default:
      ERROR(cp, "unsupported sign-mode");
      break;
   }
}






/* Used to implement write masking.  This and most of the other instructions
 * here would be easier to implement if there had been a translation
 * to a 2 argument format (dst/arg0, arg1) at the shader level before
 * attempting to translate to x86/sse code.
 */
static void store_dest( struct aos_compilation *cp, 
                        const struct tgsi_full_dst_register *reg,
                        struct x86_reg result )
{
   struct x86_reg dst;

   switch (reg->DstRegister.WriteMask) {
   case 0:
      return;
   
   case TGSI_WRITEMASK_XYZW:
      aos_adopt_xmm_reg(cp, 
                        get_xmm_writable(cp, result), 
                        reg->DstRegister.File,
                        reg->DstRegister.Index,
                        TRUE);
      return;
   default: 
      break;
   }

   dst = aos_get_shader_reg_xmm(cp, 
                                reg->DstRegister.File,
                                reg->DstRegister.Index);

   switch (reg->DstRegister.WriteMask) {
   case TGSI_WRITEMASK_X:
      sse_movss(cp->func, dst, get_xmm(cp, result));
      break;
      
   case TGSI_WRITEMASK_XY:
      sse_shufps(cp->func, dst, get_xmm(cp, result), SHUF(X, Y, Z, W));
      break;

   case TGSI_WRITEMASK_ZW: 
      result = get_xmm_writable(cp, result);
      sse_shufps(cp->func, result, dst, SHUF(X, Y, Z, W));
      dst = result;
      break;

   case TGSI_WRITEMASK_YZW: 
      sse_movss(cp->func, result, dst);
      dst = result;
      break;

   default:
      mask_write(cp, dst, result, reg->DstRegister.WriteMask);
      break;
   }

   aos_adopt_xmm_reg(cp, 
                     dst, 
                     reg->DstRegister.File,
                     reg->DstRegister.Index,
                     TRUE);

}


static void x87_fst_or_nop( struct x86_function *func,
                            unsigned writemask,
                            unsigned channel,
                            struct x86_reg ptr )
{
   assert(ptr.file == file_REG32);
   if (writemask & (1<<channel)) 
      x87_fst( func, x86_make_disp(ptr, channel * sizeof(float)) );
}

static void x87_fstp_or_pop( struct x86_function *func,
                             unsigned writemask,
                             unsigned channel,
                             struct x86_reg ptr )
{
   assert(ptr.file == file_REG32);
   if (writemask & (1<<channel)) 
      x87_fstp( func, x86_make_disp(ptr, channel * sizeof(float)) );
   else
      x87_fstp( func, x86_make_reg( file_x87, 0 ));
}



/* 
 */
static void x87_fstp_dest4( struct aos_compilation *cp,
                            const struct tgsi_full_dst_register *dst )
{
   struct x86_reg ptr = get_dst_ptr(cp, dst); 
   unsigned writemask = dst->DstRegister.WriteMask;

   x87_fst_or_nop(cp->func, writemask, 0, ptr);
   x87_fst_or_nop(cp->func, writemask, 1, ptr);
   x87_fst_or_nop(cp->func, writemask, 2, ptr);
   x87_fstp_or_pop(cp->func, writemask, 3, ptr);
}

/* Save current x87 state and put it into single precision mode.
 */
static void save_fpu_state( struct aos_compilation *cp )
{
   x87_fnstcw( cp->func, x86_make_disp(cp->machine_EDX, 
                                       Offset(struct aos_machine, fpu_restore)));
}

static void restore_fpu_state( struct aos_compilation *cp )
{
   x87_fnclex(cp->func);
   x87_fldcw( cp->func, x86_make_disp(cp->machine_EDX, 
                                      Offset(struct aos_machine, fpu_restore)));
}

static void set_fpu_round_neg_inf( struct aos_compilation *cp )
{
   if (cp->fpucntl != FPU_RND_NEG) {
      cp->fpucntl = FPU_RND_NEG;
      x87_fnclex(cp->func);
      x87_fldcw( cp->func, x86_make_disp(cp->machine_EDX, 
                                         Offset(struct aos_machine, fpu_rnd_neg_inf)));
   }
}

static void set_fpu_round_nearest( struct aos_compilation *cp )
{
   if (cp->fpucntl != FPU_RND_NEAREST) {
      cp->fpucntl = FPU_RND_NEAREST;
      x87_fnclex(cp->func);
      x87_fldcw( cp->func, x86_make_disp(cp->machine_EDX, 
                                         Offset(struct aos_machine, fpu_rnd_nearest)));
   }
}


static void x87_emit_ex2( struct aos_compilation *cp )
{
   struct x86_reg st0 = x86_make_reg(file_x87, 0);
   struct x86_reg st1 = x86_make_reg(file_x87, 1);
   int stack = cp->func->x87_stack;

//   set_fpu_round_neg_inf( cp );

   x87_fld(cp->func, st0);      /* a a */
   x87_fprndint( cp->func );	/* int(a) a*/
   x87_fsubr(cp->func, st1, st0);    /* int(a) frc(a) */
   x87_fxch(cp->func, st1);     /* frc(a) int(a) */
   x87_f2xm1(cp->func);         /* (2^frc(a))-1 int(a) */
   x87_fld1(cp->func);          /* 1 (2^frc(a))-1 int(a) */
   x87_faddp(cp->func, st1);	/* 2^frac(a) int(a)  */
   x87_fscale(cp->func);	/* (2^frac(a)*2^int(int(a))) int(a) */
                                /* 2^a int(a) */
   x87_fstp(cp->func, st1);     /* 2^a */

   assert( stack == cp->func->x87_stack);
      
}

static void PIPE_CDECL print_reg( const char *msg,
                                  const float *reg )
{
   debug_printf("%s: %f %f %f %f\n", msg, reg[0], reg[1], reg[2], reg[3]);
}

static void emit_print( struct aos_compilation *cp,
                        const char *message, /* must point to a static string! */
                        unsigned file,
                        unsigned idx )
{
   struct x86_reg ecx = x86_make_reg( file_REG32, reg_CX );
   struct x86_reg arg = get_reg_ptr( cp, file, idx );
   unsigned i;

   /* There shouldn't be anything on the x87 stack.  Can add this
    * capacity later if need be.
    */
   assert(cp->func->x87_stack == 0);

   /* For absolute correctness, need to spill/invalidate all XMM regs
    * too.  We're obviously not concerned about performance on this
    * debug path, so here goes:
    */
   for (i = 0; i < 8; i++) {
      if (cp->xmm[i].dirty) 
         spill(cp, i);

      aos_release_xmm_reg(cp, i);
   }

   /* Push caller-save (ie scratch) regs.  
    */
   x86_cdecl_caller_push_regs( cp->func );


   /* Push the arguments:
    */
   x86_lea( cp->func, ecx, arg );
   x86_push( cp->func, ecx );
   x86_push_imm32( cp->func, (int)message );

   /* Call the helper.  Could call debug_printf directly, but
    * print_reg is a nice place to put a breakpoint if need be.
    */
   x86_mov_reg_imm( cp->func, ecx, (int)print_reg );
   x86_call( cp->func, ecx );
   x86_pop( cp->func, ecx );
   x86_pop( cp->func, ecx );

   /* Pop caller-save regs 
    */
   x86_cdecl_caller_pop_regs( cp->func );

   /* Done... 
    */
}

/**
 * The traditional instructions.  All operate on internal registers
 * and ignore write masks and swizzling issues.
 */

static boolean emit_ABS( struct aos_compilation *cp, const struct tgsi_full_instruction *op ) 
{
   struct x86_reg arg0 = fetch_src(cp, &op->FullSrcRegisters[0]);
   struct x86_reg neg = aos_get_internal(cp, IMM_NEGS);
   struct x86_reg dst = get_xmm_writable(cp, arg0);

   sse_mulps(cp->func, dst, neg);
   sse_maxps(cp->func, dst, arg0);
   
   store_dest(cp, &op->FullDstRegisters[0], dst);
   return TRUE;
}

static boolean emit_ADD( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->FullSrcRegisters[0]);
   struct x86_reg arg1 = fetch_src(cp, &op->FullSrcRegisters[1]);
   struct x86_reg dst = get_xmm_writable(cp, arg0);

   sse_addps(cp->func, dst, arg1);

   store_dest(cp, &op->FullDstRegisters[0], dst);
   return TRUE;
}

static boolean emit_COS( struct aos_compilation *cp, const struct tgsi_full_instruction *op ) 
{
   x87_fld_src(cp, &op->FullSrcRegisters[0], 0);
   x87_fcos(cp->func);
   x87_fstp_dest4(cp, &op->FullDstRegisters[0]);
   return TRUE;
}


/* The dotproduct instructions don't really do that well in sse:
 */
static boolean emit_DP3( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->FullSrcRegisters[0]);
   struct x86_reg arg1 = fetch_src(cp, &op->FullSrcRegisters[1]);
   struct x86_reg tmp = aos_get_xmm_reg(cp); 
   struct x86_reg dst = get_xmm_writable(cp, arg0);

   sse_mulps(cp->func, dst, arg1);
   /* Now the hard bit: sum the first 3 values:
    */ 
   sse_movhlps(cp->func, tmp, dst);
   sse_addss(cp->func, dst, tmp); /* a*x+c*z, b*y, ?, ? */
   emit_pshufd(cp, tmp, dst, SHUF(Y,X,W,Z));
   sse_addss(cp->func, dst, tmp);
   
   if (op->FullDstRegisters[0].DstRegister.WriteMask != 0x1)
      sse_shufps(cp->func, dst, dst, SHUF(X, X, X, X));

   aos_release_xmm_reg(cp, tmp.idx);
   store_dest(cp, &op->FullDstRegisters[0], dst);
   return TRUE;
}



static boolean emit_DP4( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->FullSrcRegisters[0]);
   struct x86_reg arg1 = fetch_src(cp, &op->FullSrcRegisters[1]);
   struct x86_reg tmp = aos_get_xmm_reg(cp);      
   struct x86_reg dst = get_xmm_writable(cp, arg0);

   sse_mulps(cp->func, dst, arg1);
   
   /* Now the hard bit: sum the values:
    */ 
   sse_movhlps(cp->func, tmp, dst);
   sse_addps(cp->func, dst, tmp); /* a*x+c*z, b*y+d*w, a*x+c*z, b*y+d*w */
   emit_pshufd(cp, tmp, dst, SHUF(Y,X,W,Z));
   sse_addss(cp->func, dst, tmp);

   if (op->FullDstRegisters[0].DstRegister.WriteMask != 0x1)
      sse_shufps(cp->func, dst, dst, SHUF(X, X, X, X));

   aos_release_xmm_reg(cp, tmp.idx);
   store_dest(cp, &op->FullDstRegisters[0], dst);
   return TRUE;
}

static boolean emit_DPH( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->FullSrcRegisters[0]);
   struct x86_reg arg1 = fetch_src(cp, &op->FullSrcRegisters[1]);
   struct x86_reg tmp = aos_get_xmm_reg(cp);
   struct x86_reg dst = get_xmm_writable(cp, arg0);

   sse_mulps(cp->func, dst, arg1);

   /* Now the hard bit: sum the values (from DP3):
    */ 
   sse_movhlps(cp->func, tmp, dst);
   sse_addss(cp->func, dst, tmp); /* a*x+c*z, b*y, ?, ? */
   emit_pshufd(cp, tmp, dst, SHUF(Y,X,W,Z));
   sse_addss(cp->func, dst, tmp);
   emit_pshufd(cp, tmp, arg1, SHUF(W,W,W,W));
   sse_addss(cp->func, dst, tmp);

   if (op->FullDstRegisters[0].DstRegister.WriteMask != 0x1)
      sse_shufps(cp->func, dst, dst, SHUF(X, X, X, X));

   aos_release_xmm_reg(cp, tmp.idx);
   store_dest(cp, &op->FullDstRegisters[0], dst);
   return TRUE;
}

static boolean emit_DST( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
    struct x86_reg arg0 = fetch_src(cp, &op->FullSrcRegisters[0]);
    struct x86_reg arg1 = fetch_src(cp, &op->FullSrcRegisters[1]);
    struct x86_reg dst = aos_get_xmm_reg(cp);
    struct x86_reg tmp = aos_get_xmm_reg(cp);
    struct x86_reg ones = aos_get_internal(cp, IMM_ONES);

/*    dst[0] = 1.0     * 1.0F; */
/*    dst[1] = arg0[1] * arg1[1]; */
/*    dst[2] = arg0[2] * 1.0; */
/*    dst[3] = 1.0     * arg1[3]; */

    emit_shuf_copy2(cp, dst, arg0, ones, SHUF(X,W,Z,Y));
    emit_shuf_copy2(cp, tmp, arg1, ones, SHUF(X,Z,Y,W));
    sse_mulps(cp->func, dst, tmp);

    aos_release_xmm_reg(cp, tmp.idx);
    store_dest(cp, &op->FullDstRegisters[0], dst);
    return TRUE;
}

static boolean emit_LG2( struct aos_compilation *cp, const struct tgsi_full_instruction *op ) 
{
   x87_fld1(cp->func);		/* 1 */
   x87_fld_src(cp, &op->FullSrcRegisters[0], 0);	/* a0 1 */
   x87_fyl2x(cp->func);	/* log2(a0) */
   x87_fstp_dest4(cp, &op->FullDstRegisters[0]);
   return TRUE;
}


static boolean emit_EX2( struct aos_compilation *cp, const struct tgsi_full_instruction *op ) 
{
   x87_fld_src(cp, &op->FullSrcRegisters[0], 0);
   x87_emit_ex2(cp);
   x87_fstp_dest4(cp, &op->FullDstRegisters[0]);
   return TRUE;
}

static boolean emit_EXP( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
    struct x86_reg dst = get_dst_ptr(cp, &op->FullDstRegisters[0]); 
    struct x86_reg st0 = x86_make_reg(file_x87, 0);
    struct x86_reg st1 = x86_make_reg(file_x87, 1);
    struct x86_reg st3 = x86_make_reg(file_x87, 3);
    unsigned writemask = op->FullDstRegisters[0].DstRegister.WriteMask;

    /* CAUTION: dst may alias arg0!
     */
    x87_fld_src(cp, &op->FullSrcRegisters[0], 0);	/* arg0.x */
    x87_fld(cp->func, st0); /* arg arg */

    /* by default, fpu is setup to round-to-nearest.  We want to
     * change this now, and track the state through to the end of the
     * generated function so that it isn't repeated unnecessarily.
     * Alternately, could subtract .5 to get round to -inf behaviour.
     */
    set_fpu_round_neg_inf( cp );
    x87_fprndint( cp->func );	/* flr(a) a */
    x87_fld(cp->func, st0); /* flr(a) flr(a) a */
    x87_fld1(cp->func);    /* 1 floor(a) floor(a) a */
    x87_fst_or_nop(cp->func, writemask, 3, dst);  /* stack unchanged */

    x87_fscale(cp->func);  /* 2^floor(a) floor(a) a */
    x87_fst(cp->func, st3); /* 2^floor(a) floor(a) a 2^floor(a)*/

    x87_fstp_or_pop(cp->func, writemask, 0, dst); /* flr(a) a 2^flr(a) */

    x87_fsubp(cp->func, st1);                     /* frac(a) 2^flr(a) */

    x87_fst_or_nop(cp->func, writemask, 1, dst);    /* frac(a) 2^flr(a) */

    x87_f2xm1(cp->func);    /* (2^frac(a))-1 2^flr(a)*/
    x87_fld1(cp->func);    /* 1 (2^frac(a))-1 2^flr(a)*/
    x87_faddp(cp->func, st1);	/* 2^frac(a) 2^flr(a) */
    x87_fmulp(cp->func, st1);	/* 2^a */
    
    x87_fstp_or_pop(cp->func, writemask, 2, dst);    

/*    dst[0] = 2^floor(tmp); */
/*    dst[1] = frac(tmp); */
/*    dst[2] = 2^floor(tmp) * 2^frac(tmp); */
/*    dst[3] = 1.0F; */
    return TRUE;
}

static boolean emit_LOG( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
    struct x86_reg dst = get_dst_ptr(cp, &op->FullDstRegisters[0]); 
    struct x86_reg st0 = x86_make_reg(file_x87, 0);
    struct x86_reg st1 = x86_make_reg(file_x87, 1);
    struct x86_reg st2 = x86_make_reg(file_x87, 2);
    unsigned writemask = op->FullDstRegisters[0].DstRegister.WriteMask;
 
    /* CAUTION: dst may alias arg0!
     */
    x87_fld_src(cp, &op->FullSrcRegisters[0], 0);	/* arg0.x */
    x87_fabs(cp->func);	/* |arg0.x| */
    x87_fxtract(cp->func);	/* mantissa(arg0.x), exponent(arg0.x) */
    x87_fst(cp->func, st2);	/* mantissa, exponent, mantissa */
    x87_fld1(cp->func);	/* 1, mantissa, exponent, mantissa */
    x87_fyl2x(cp->func); 	/* log2(mantissa), exponent, mantissa */
    x87_fadd(cp->func, st0, st1);	/* e+l2(m), e, m  */
    
    x87_fstp_or_pop(cp->func, writemask, 2, dst); /* e, m */

    x87_fld1(cp->func);	/* 1, e, m */
    x87_fsub(cp->func, st1, st0);	/* 1, e-1, m */

    x87_fstp_or_pop(cp->func, writemask, 3, dst); /* e-1,m */
    x87_fstp_or_pop(cp->func, writemask, 0, dst);	/* m */

    x87_fadd(cp->func, st0, st0);	/* 2m */

    x87_fstp_or_pop( cp->func, writemask, 1, dst );

    return TRUE;
}

static boolean emit_FLR( struct aos_compilation *cp, const struct tgsi_full_instruction *op ) 
{
   struct x86_reg dst = get_dst_ptr(cp, &op->FullDstRegisters[0]); 
   unsigned writemask = op->FullDstRegisters[0].DstRegister.WriteMask;
   int i;

   set_fpu_round_neg_inf( cp );

   /* Load all sources first to avoid aliasing
    */
   for (i = 3; i >= 0; i--) {
      if (writemask & (1<<i)) {
         x87_fld_src(cp, &op->FullSrcRegisters[0], i);   
      }
   }

   for (i = 0; i < 4; i++) {
      if (writemask & (1<<i)) {
         x87_fprndint( cp->func );   
         x87_fstp(cp->func, x86_make_disp(dst, i*4));
      }
   }

   return TRUE;
}


static boolean emit_RND( struct aos_compilation *cp, const struct tgsi_full_instruction *op ) 
{
   struct x86_reg dst = get_dst_ptr(cp, &op->FullDstRegisters[0]); 
   unsigned writemask = op->FullDstRegisters[0].DstRegister.WriteMask;
   int i;

   set_fpu_round_nearest( cp );

   /* Load all sources first to avoid aliasing
    */
   for (i = 3; i >= 0; i--) {
      if (writemask & (1<<i)) {
         x87_fld_src(cp, &op->FullSrcRegisters[0], i);   
      }
   }

   for (i = 0; i < 4; i++) {
      if (writemask & (1<<i)) {
         x87_fprndint( cp->func );   
         x87_fstp(cp->func, x86_make_disp(dst, i*4));
      }
   }

   return TRUE;
}


static boolean emit_FRC( struct aos_compilation *cp, const struct tgsi_full_instruction *op ) 
{
   struct x86_reg dst = get_dst_ptr(cp, &op->FullDstRegisters[0]); 
   struct x86_reg st0 = x86_make_reg(file_x87, 0);
   struct x86_reg st1 = x86_make_reg(file_x87, 1);
   unsigned writemask = op->FullDstRegisters[0].DstRegister.WriteMask;
   int i;

   set_fpu_round_neg_inf( cp );

   /* suck all the source values onto the stack before writing out any
    * dst, which may alias...
    */
   for (i = 3; i >= 0; i--) {
      if (writemask & (1<<i)) {
         x87_fld_src(cp, &op->FullSrcRegisters[0], i);   
      }
   }

   for (i = 0; i < 4; i++) {
      if (writemask & (1<<i)) {
         x87_fld(cp->func, st0);     /* a a */
         x87_fprndint( cp->func );   /* flr(a) a */
         x87_fsubp(cp->func, st1);  /* frc(a) */
         x87_fstp(cp->func, x86_make_disp(dst, i*4));
      }
   }

   return TRUE;
}

static PIPE_CDECL void do_lit( struct aos_machine *machine,
                               float *result,
                               const float *in,
                               unsigned count )
{
   if (in[0] > 0) 
   {
      if (in[1] <= 0.0) 
      {
         result[0] = 1.0F;
         result[1] = in[0];
         result[2] = 1.0;
         result[3] = 1.0F;
      }
      else
      {
         const float epsilon = 1.0F / 256.0F;    
         float exponent = CLAMP(in[3], -(128.0F - epsilon), (128.0F - epsilon));
         result[0] = 1.0F;
         result[1] = in[0];
         result[2] = powf(in[1], exponent);
         result[3] = 1.0;
      }
   }
   else 
   {
      result[0] = 1.0F;
      result[1] = 0.0;
      result[2] = 0.0;
      result[3] = 1.0F;
   }
}


static PIPE_CDECL void do_lit_lut( struct aos_machine *machine,
                                   float *result,
                                   const float *in,
                                   unsigned count )
{
   if (in[0] > 0) 
   {
      if (in[1] <= 0.0) 
      {
         result[0] = 1.0F;
         result[1] = in[0];
         result[2] = 1.0;
         result[3] = 1.0F;
         return;
      }
      
      if (machine->lit_info[count].shine_tab->exponent != in[3]) {
         machine->lit_info[count].func = do_lit;
         goto no_luck;
      }

      if (in[1] <= 1.0)
      {
         const float *tab = machine->lit_info[count].shine_tab->values;
         float f = in[1] * 256;
         int k = (int)f;
         float frac = f - (float)k;
         
         result[0] = 1.0F;
         result[1] = in[0];
         result[2] = tab[k] + frac*(tab[k+1]-tab[k]);
         result[3] = 1.0;
         return;
      }
      
   no_luck:
      {
         const float epsilon = 1.0F / 256.0F;    
         float exponent = CLAMP(in[3], -(128.0F - epsilon), (128.0F - epsilon));
         result[0] = 1.0F;
         result[1] = in[0];
         result[2] = powf(in[1], exponent);
         result[3] = 1.0;
      }
   }
   else 
   {
      result[0] = 1.0F;
      result[1] = 0.0;
      result[2] = 0.0;
      result[3] = 1.0F;
   }
}



static void PIPE_CDECL populate_lut( struct aos_machine *machine,
                                     float *result,
                                     const float *in,
                                     unsigned count )
{
   unsigned i, tab;

   /* Search for an existing table for this value.  Note that without
    * static analysis we don't really know if in[3] will be constant,
    * but it usually is...
    */
   for (tab = 0; tab < 4; tab++) {
      if (machine->shine_tab[tab].exponent == in[3]) {
         goto found;
      }
   }

   for (tab = 0, i = 1; i < 4; i++) {
      if (machine->shine_tab[i].last_used < machine->shine_tab[tab].last_used)
         tab = i;
   }

   if (machine->shine_tab[tab].last_used == machine->now) {
      /* No unused tables (this is not a ffvertex program...).  Just
       * call pow each time:
       */
      machine->lit_info[count].func = do_lit;
      machine->lit_info[count].func( machine, result, in, count );
      return;
   }
   else {
      do_populate_lut( &machine->shine_tab[tab], in[3] );
   }

 found:
   machine->shine_tab[tab].last_used = machine->now;
   machine->lit_info[count].shine_tab = &machine->shine_tab[tab];
   machine->lit_info[count].func = do_lit_lut;
   machine->lit_info[count].func( machine, result, in, count );
}





static boolean emit_LIT( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg ecx = x86_make_reg( file_REG32, reg_CX );
   unsigned writemask = op->FullDstRegisters[0].DstRegister.WriteMask;
   unsigned lit_count = cp->lit_count++;
   struct x86_reg result, arg0;
   unsigned i;

#if 1
   /* For absolute correctness, need to spill/invalidate all XMM regs
    * too.  
    */
   for (i = 0; i < 8; i++) {
      if (cp->xmm[i].dirty) 
         spill(cp, i);
      aos_release_xmm_reg(cp, i);
   }
#endif

   if (writemask != TGSI_WRITEMASK_XYZW) 
      result = x86_make_disp(cp->machine_EDX, Offset(struct aos_machine, tmp[0]));
   else 
      result = get_dst_ptr(cp, &op->FullDstRegisters[0]);    

   
   arg0 = fetch_src( cp, &op->FullSrcRegisters[0] );
   if (arg0.file == file_XMM) {
      struct x86_reg tmp = x86_make_disp(cp->machine_EDX, 
                                         Offset(struct aos_machine, tmp[1]));
      sse_movaps( cp->func, tmp, arg0 );
      arg0 = tmp;
   }
                  
      

   /* Push caller-save (ie scratch) regs.  
    */
   x86_cdecl_caller_push_regs( cp->func );

   /* Push the arguments:
    */
   x86_push_imm32( cp->func, lit_count );

   x86_lea( cp->func, ecx, arg0 );
   x86_push( cp->func, ecx );

   x86_lea( cp->func, ecx, result );
   x86_push( cp->func, ecx );

   x86_push( cp->func, cp->machine_EDX );

   if (lit_count < MAX_LIT_INFO) {
      x86_mov( cp->func, ecx, x86_make_disp( cp->machine_EDX, 
                                             Offset(struct aos_machine, lit_info) + 
                                             lit_count * sizeof(struct lit_info) + 
                                             Offset(struct lit_info, func)));
   }
   else {
      x86_mov_reg_imm( cp->func, ecx, (int)do_lit );
   }

   x86_call( cp->func, ecx );
            
   x86_pop( cp->func, ecx );    /* fixme... */
   x86_pop( cp->func, ecx );
   x86_pop( cp->func, ecx );
   x86_pop( cp->func, ecx );

   x86_cdecl_caller_pop_regs( cp->func );

   if (writemask != TGSI_WRITEMASK_XYZW) {
      store_dest( cp, 
                  &op->FullDstRegisters[0],
                  get_xmm_writable( cp, result ) );
   }

   return TRUE;
}

   
static boolean emit_inline_LIT( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg dst = get_dst_ptr(cp, &op->FullDstRegisters[0]); 
   unsigned writemask = op->FullDstRegisters[0].DstRegister.WriteMask;

   if (writemask & TGSI_WRITEMASK_YZ) {
      struct x86_reg st1 = x86_make_reg(file_x87, 1);
      struct x86_reg st2 = x86_make_reg(file_x87, 2);

      /* a1' = a1 <= 0 ? 1 : a1;  
       */
      x87_fldz(cp->func);                           /* 1 0  */
#if 1
      x87_fld1(cp->func);                           /* 1 0  */
#else
      /* Correct but slow due to fp exceptions generated in fyl2x - fix me.
       */
      x87_fldz(cp->func);                           /* 1 0  */
#endif
      x87_fld_src(cp, &op->FullSrcRegisters[0], 1); /* a1 1 0  */
      x87_fcomi(cp->func, st2);	                    /* a1 1 0  */
      x87_fcmovb(cp->func, st1);                    /* a1' 1 0  */
      x87_fstp(cp->func, st1);                      /* a1' 0  */
      x87_fstp(cp->func, st1);                      /* a1'  */

      x87_fld_src(cp, &op->FullSrcRegisters[0], 3); /* a3 a1'  */
      x87_fxch(cp->func, st1);                      /* a1' a3  */
      

      /* Compute pow(a1, a3)
       */
      x87_fyl2x(cp->func);	/* a3*log2(a1)      */
      x87_emit_ex2( cp );       /* 2^(a3*log2(a1))   */


      /* a0' = max2(a0, 0):
       */
      x87_fldz(cp->func);                           /* 0 r2 */
      x87_fld_src(cp, &op->FullSrcRegisters[0], 0); /* a0 0 r2 */
      x87_fcomi(cp->func, st1);	
      x87_fcmovb(cp->func, st1);                    /* a0' 0 r2 */

      x87_fst_or_nop(cp->func, writemask, 1, dst); /* result[1] = a0' */

      x87_fcomi(cp->func, st1);  /* a0' 0 r2 */
      x87_fcmovnbe(cp->func, st2); /* r2' 0' r2 */

      x87_fstp_or_pop(cp->func, writemask, 2, dst); /* 0 r2 */
      x87_fpop(cp->func);       /* r2 */
      x87_fpop(cp->func);
   }

   if (writemask & TGSI_WRITEMASK_XW) {
      x87_fld1(cp->func);
      x87_fst_or_nop(cp->func, writemask, 0, dst);
      x87_fstp_or_pop(cp->func, writemask, 3, dst);
   }

   return TRUE;
}



static boolean emit_MAX( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->FullSrcRegisters[0]);
   struct x86_reg arg1 = fetch_src(cp, &op->FullSrcRegisters[1]);
   struct x86_reg dst = get_xmm_writable(cp, arg0);

   sse_maxps(cp->func, dst, arg1);

   store_dest(cp, &op->FullDstRegisters[0], dst);
   return TRUE;
}


static boolean emit_MIN( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->FullSrcRegisters[0]);
   struct x86_reg arg1 = fetch_src(cp, &op->FullSrcRegisters[1]);
   struct x86_reg dst = get_xmm_writable(cp, arg0);

   sse_minps(cp->func, dst, arg1);

   store_dest(cp, &op->FullDstRegisters[0], dst);
   return TRUE;
}

static boolean emit_MOV( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->FullSrcRegisters[0]);
   struct x86_reg dst = get_xmm_writable(cp, arg0);

   /* potentially nothing to do */

   store_dest(cp, &op->FullDstRegisters[0], dst);
   return TRUE;
}

static boolean emit_MUL( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->FullSrcRegisters[0]);
   struct x86_reg arg1 = fetch_src(cp, &op->FullSrcRegisters[1]);
   struct x86_reg dst = get_xmm_writable(cp, arg0);

   sse_mulps(cp->func, dst, arg1);

   store_dest(cp, &op->FullDstRegisters[0], dst);
   return TRUE;
}


static boolean emit_MAD( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->FullSrcRegisters[0]);
   struct x86_reg arg1 = fetch_src(cp, &op->FullSrcRegisters[1]);
   struct x86_reg arg2 = fetch_src(cp, &op->FullSrcRegisters[2]);

   /* If we can't clobber old contents of arg0, get a temporary & copy
    * it there, then clobber it...
    */
   arg0 = get_xmm_writable(cp, arg0);

   sse_mulps(cp->func, arg0, arg1);
   sse_addps(cp->func, arg0, arg2);
   store_dest(cp, &op->FullDstRegisters[0], arg0);
   return TRUE;
}


static boolean emit_POW( struct aos_compilation *cp, const struct tgsi_full_instruction *op ) 
{
   x87_fld_src(cp, &op->FullSrcRegisters[1], 0);  /* a1.x */
   x87_fld_src(cp, &op->FullSrcRegisters[0], 0);	/* a0.x a1.x */
   x87_fyl2x(cp->func);	                                /* a1*log2(a0) */

   x87_emit_ex2( cp );		/* 2^(a1*log2(a0)) */

   x87_fstp_dest4(cp, &op->FullDstRegisters[0]);
   return TRUE;
}


static boolean emit_RCP( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->FullSrcRegisters[0]);
   struct x86_reg dst = aos_get_xmm_reg(cp);

   if (cp->have_sse2) {
      sse2_rcpss(cp->func, dst, arg0);
      /* extend precision here...
       */
   }
   else {
      struct x86_reg ones = aos_get_internal(cp, IMM_ONES);
      sse_movss(cp->func, dst, ones);
      sse_divss(cp->func, dst, arg0);
   }

   if (op->FullDstRegisters[0].DstRegister.WriteMask != 0x1)
      sse_shufps(cp->func, dst, dst, SHUF(X, X, X, X));

   store_dest(cp, &op->FullDstRegisters[0], dst);
   return TRUE;
}

static boolean emit_RSQ( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->FullSrcRegisters[0]);
   struct x86_reg dst = aos_get_xmm_reg(cp);

   sse_rsqrtss(cp->func, dst, arg0);

   /* Extend precision here...
    */

   if (op->FullDstRegisters[0].DstRegister.WriteMask != 0x1)
      sse_shufps(cp->func, dst, dst, SHUF(X, X, X, X));

   store_dest(cp, &op->FullDstRegisters[0], dst);
   return TRUE;
}


static boolean emit_SGE( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->FullSrcRegisters[0]);
   struct x86_reg arg1 = fetch_src(cp, &op->FullSrcRegisters[1]);
   struct x86_reg ones = aos_get_internal(cp, IMM_ONES);
   struct x86_reg dst = get_xmm_writable(cp, arg0);

   sse_cmpps(cp->func, dst, arg1, cc_NotLessThan);
   sse_andps(cp->func, dst, ones);

   store_dest(cp, &op->FullDstRegisters[0], dst);
   return TRUE;
}

static boolean emit_SIN( struct aos_compilation *cp, const struct tgsi_full_instruction *op ) 
{
   x87_fld_src(cp, &op->FullSrcRegisters[0], 0);
   x87_fsin(cp->func);
   x87_fstp_dest4(cp, &op->FullDstRegisters[0]);
   return TRUE;
}



static boolean emit_SLT( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->FullSrcRegisters[0]);
   struct x86_reg arg1 = fetch_src(cp, &op->FullSrcRegisters[1]);
   struct x86_reg ones = aos_get_internal(cp, IMM_ONES);
   struct x86_reg dst = get_xmm_writable(cp, arg0);
   
   sse_cmpps(cp->func, dst, arg1, cc_LessThan);
   sse_andps(cp->func, dst, ones);

   store_dest(cp, &op->FullDstRegisters[0], dst);
   return TRUE;
}

static boolean emit_SUB( struct aos_compilation *cp, const struct tgsi_full_instruction *op ) 
{
   struct x86_reg arg0 = fetch_src(cp, &op->FullSrcRegisters[0]);
   struct x86_reg arg1 = fetch_src(cp, &op->FullSrcRegisters[1]);
   struct x86_reg dst = get_xmm_writable(cp, arg0);

   sse_subps(cp->func, dst, arg1);

   store_dest(cp, &op->FullDstRegisters[0], dst);
   return TRUE;
}


static boolean emit_XPD( struct aos_compilation *cp, const struct tgsi_full_instruction *op ) 
{
   struct x86_reg arg0 = fetch_src(cp, &op->FullSrcRegisters[0]);
   struct x86_reg arg1 = fetch_src(cp, &op->FullSrcRegisters[1]);
   struct x86_reg dst = aos_get_xmm_reg(cp);
   struct x86_reg tmp0 = aos_get_xmm_reg(cp);
   struct x86_reg tmp1 = aos_get_xmm_reg(cp);

   /* Could avoid tmp0, tmp1 if we overwrote arg0, arg1.  Need a way
    * to invalidate registers.  This will come with better analysis
    * (liveness analysis) of the incoming program.
    */
   emit_pshufd(cp, dst, arg0, SHUF(Y, Z, X, W));
   emit_pshufd(cp, tmp1, arg1, SHUF(Z, X, Y, W));
   sse_mulps(cp->func, dst, tmp1);
   emit_pshufd(cp, tmp0, arg0, SHUF(Z, X, Y, W));
   emit_pshufd(cp, tmp1, arg1, SHUF(Y, Z, X, W));
   sse_mulps(cp->func, tmp0, tmp1);
   sse_subps(cp->func, dst, tmp0);

/*    dst[0] = arg0[1] * arg1[2] - arg0[2] * arg1[1]; */
/*    dst[1] = arg0[2] * arg1[0] - arg0[0] * arg1[2]; */
/*    dst[2] = arg0[0] * arg1[1] - arg0[1] * arg1[0]; */
/*    dst[3] is undef */


   aos_release_xmm_reg(cp, tmp0.idx);
   aos_release_xmm_reg(cp, tmp1.idx);
   store_dest(cp, &op->FullDstRegisters[0], dst);
   return TRUE;
}



static boolean
emit_instruction( struct aos_compilation *cp,
                  struct tgsi_full_instruction *inst )
{
   x87_assert_stack_empty(cp->func);

   switch( inst->Instruction.Opcode ) {
   case TGSI_OPCODE_MOV:
      return emit_MOV( cp, inst );

   case TGSI_OPCODE_LIT:
      return emit_LIT(cp, inst);

   case TGSI_OPCODE_RCP:
      return emit_RCP(cp, inst);

   case TGSI_OPCODE_RSQ:
      return emit_RSQ(cp, inst);

   case TGSI_OPCODE_EXP:
      return emit_EXP(cp, inst);

   case TGSI_OPCODE_LOG:
      return emit_LOG(cp, inst);

   case TGSI_OPCODE_MUL:
      return emit_MUL(cp, inst);

   case TGSI_OPCODE_ADD:
      return emit_ADD(cp, inst);

   case TGSI_OPCODE_DP3:
      return emit_DP3(cp, inst);

   case TGSI_OPCODE_DP4:
      return emit_DP4(cp, inst);

   case TGSI_OPCODE_DST:
      return emit_DST(cp, inst);

   case TGSI_OPCODE_MIN:
      return emit_MIN(cp, inst);

   case TGSI_OPCODE_MAX:
      return emit_MAX(cp, inst);

   case TGSI_OPCODE_SLT:
      return emit_SLT(cp, inst);

   case TGSI_OPCODE_SGE:
      return emit_SGE(cp, inst);

   case TGSI_OPCODE_MAD:
      return emit_MAD(cp, inst);

   case TGSI_OPCODE_SUB:
      return emit_SUB(cp, inst);
 
   case TGSI_OPCODE_LERP:
//      return emit_LERP(cp, inst);
      return FALSE;

   case TGSI_OPCODE_FRAC:
      return emit_FRC(cp, inst);

   case TGSI_OPCODE_CLAMP:
//      return emit_CLAMP(cp, inst);
      return FALSE;

   case TGSI_OPCODE_FLOOR:
      return emit_FLR(cp, inst);

   case TGSI_OPCODE_ROUND:
      return emit_RND(cp, inst);

   case TGSI_OPCODE_EXPBASE2:
      return emit_EX2(cp, inst);

   case TGSI_OPCODE_LOGBASE2:
      return emit_LG2(cp, inst);

   case TGSI_OPCODE_POWER:
      return emit_POW(cp, inst);

   case TGSI_OPCODE_CROSSPRODUCT:
      return emit_XPD(cp, inst);

   case TGSI_OPCODE_ABS:
      return emit_ABS(cp, inst);

   case TGSI_OPCODE_DPH:
      return emit_DPH(cp, inst);

   case TGSI_OPCODE_COS:
      return emit_COS(cp, inst);

   case TGSI_OPCODE_SIN:
      return emit_SIN(cp, inst);

   case TGSI_OPCODE_END:
      return TRUE;

   default:
      return FALSE;
   }
}


static boolean emit_viewport( struct aos_compilation *cp )
{
   struct x86_reg pos = aos_get_shader_reg_xmm(cp, 
                                               TGSI_FILE_OUTPUT, 
                                               0);

   struct x86_reg scale = x86_make_disp(cp->machine_EDX, 
                                        Offset(struct aos_machine, scale));

   struct x86_reg translate = x86_make_disp(cp->machine_EDX, 
                                        Offset(struct aos_machine, translate));

   sse_mulps(cp->func, pos, scale);
   sse_addps(cp->func, pos, translate);

   aos_adopt_xmm_reg( cp,
                      pos,
                      TGSI_FILE_OUTPUT,
                      0,
                      TRUE );
   return TRUE;
}


/* This is useful to be able to see the results on softpipe.  Doesn't
 * do proper clipping, just assumes the backend can do it during
 * rasterization -- for debug only...
 */
static boolean emit_rhw_viewport( struct aos_compilation *cp )
{
   struct x86_reg tmp = aos_get_xmm_reg(cp);
   struct x86_reg pos = aos_get_shader_reg_xmm(cp, 
                                               TGSI_FILE_OUTPUT, 
                                               0);

   struct x86_reg scale = x86_make_disp(cp->machine_EDX, 
                                        Offset(struct aos_machine, scale));

   struct x86_reg translate = x86_make_disp(cp->machine_EDX, 
                                        Offset(struct aos_machine, translate));



   emit_pshufd(cp, tmp, pos, SHUF(W, W, W, W));
   sse2_rcpss(cp->func, tmp, tmp);
   sse_shufps(cp->func, tmp, tmp, SHUF(X, X, X, X));
   
   sse_mulps(cp->func, pos, scale);
   sse_mulps(cp->func, pos, tmp);
   sse_addps(cp->func, pos, translate);

   /* Set pos[3] = w 
    */
   mask_write(cp, pos, tmp, TGSI_WRITEMASK_W);

   aos_adopt_xmm_reg( cp,
                      pos,
                      TGSI_FILE_OUTPUT,
                      0,
                      TRUE );
   return TRUE;
}


static boolean note_immediate( struct aos_compilation *cp,
                               struct tgsi_full_immediate *imm )
{
   unsigned pos = cp->num_immediates++;
   unsigned j;

   for (j = 0; j < imm->Immediate.Size; j++) {
      cp->vaos->machine->immediate[pos][j] = imm->u.ImmediateFloat32[j].Float;
   }

   return TRUE;
}




static void find_last_write_outputs( struct aos_compilation *cp )
{
   struct tgsi_parse_context parse;
   unsigned this_instruction = 0;
   unsigned i;

   tgsi_parse_init( &parse, cp->vaos->base.vs->state.tokens );

   while (!tgsi_parse_end_of_tokens( &parse )) {
      
      tgsi_parse_token( &parse );

      if (parse.FullToken.Token.Type != TGSI_TOKEN_TYPE_INSTRUCTION) 
         continue;

      for (i = 0; i < TGSI_FULL_MAX_DST_REGISTERS; i++) {
         if (parse.FullToken.FullInstruction.FullDstRegisters[i].DstRegister.File ==
             TGSI_FILE_OUTPUT) 
         {
            unsigned idx = parse.FullToken.FullInstruction.FullDstRegisters[i].DstRegister.Index;
            cp->output_last_write[idx] = this_instruction;
         }
      }

      this_instruction++;
   }

   tgsi_parse_free( &parse );
}


#define ARG_VARIENT    1
#define ARG_START_ELTS 2
#define ARG_COUNT      3
#define ARG_OUTBUF     4


static boolean build_vertex_program( struct draw_vs_varient_aos_sse *varient,
                                     boolean linear )
{ 
   struct tgsi_parse_context parse;
   struct aos_compilation cp;
   unsigned fixup, label;

   tgsi_parse_init( &parse, varient->base.vs->state.tokens );

   memset(&cp, 0, sizeof(cp));

   cp.insn_counter = 1;
   cp.vaos = varient;
   cp.have_sse2 = 1;
   cp.func = &varient->func[ linear ? 0 : 1 ];

   cp.tmp_EAX       = x86_make_reg(file_REG32, reg_AX);
   cp.idx_EBX      = x86_make_reg(file_REG32, reg_BX);
   cp.outbuf_ECX    = x86_make_reg(file_REG32, reg_CX);
   cp.machine_EDX   = x86_make_reg(file_REG32, reg_DX);
   cp.count_ESI     = x86_make_reg(file_REG32, reg_SI);

   x86_init_func(cp.func);

   find_last_write_outputs(&cp);

   x86_push(cp.func, cp.idx_EBX);
   x86_push(cp.func, cp.count_ESI);


   /* Load arguments into regs:
    */
   x86_mov(cp.func, cp.machine_EDX, x86_fn_arg(cp.func, ARG_VARIENT));
   x86_mov(cp.func, cp.idx_EBX, x86_fn_arg(cp.func, ARG_START_ELTS));
   x86_mov(cp.func, cp.count_ESI, x86_fn_arg(cp.func, ARG_COUNT));
   x86_mov(cp.func, cp.outbuf_ECX, x86_fn_arg(cp.func, ARG_OUTBUF));


   /* Compare count to zero and possibly bail.
    */
   x86_xor(cp.func, cp.tmp_EAX, cp.tmp_EAX);
   x86_cmp(cp.func, cp.count_ESI, cp.tmp_EAX);
   fixup = x86_jcc_forward(cp.func, cc_E);

   /* Dig out the machine pointer from inside the varient arg 
    */
   x86_mov(cp.func, cp.machine_EDX, 
           x86_make_disp(cp.machine_EDX,
                         Offset( struct draw_vs_varient_aos_sse, machine )));

   save_fpu_state( &cp );
   set_fpu_round_nearest( &cp );

   /* Note address for loop jump 
    */
   label = x86_get_label(cp.func);
   {
      /* Fetch inputs...  TODO:  fetch lazily...
       */
      if (!aos_fetch_inputs( &cp, linear ))
         goto fail;

      /* Emit the shader:
       */
      while( !tgsi_parse_end_of_tokens( &parse ) && !cp.error ) 
      {
         tgsi_parse_token( &parse );

         switch (parse.FullToken.Token.Type) {
         case TGSI_TOKEN_TYPE_IMMEDIATE:
            if (!note_immediate( &cp, &parse.FullToken.FullImmediate ))
               goto fail;
            break;

         case TGSI_TOKEN_TYPE_INSTRUCTION:
            if (!emit_instruction( &cp, &parse.FullToken.FullInstruction ))
               goto fail;
            break;
         }

         x87_assert_stack_empty(cp.func);
         cp.insn_counter++;
         debug_printf("\n");
      }

      if (cp.error)
         goto fail;

      if (cp.vaos->base.key.viewport) {
         if (0)
            emit_viewport(&cp);
         else
            emit_rhw_viewport(&cp);
      }

      /* Emit output...  TODO: do this eagerly after the last write to a
       * given output.
       */
      if (!aos_emit_outputs( &cp ))
         goto fail;


      /* Next vertex:
       */
      x86_lea(cp.func, 
              cp.outbuf_ECX, 
              x86_make_disp(cp.outbuf_ECX, 
                            cp.vaos->base.key.output_stride));

      /* Incr index
       */   
      if (linear) {
         x86_inc(cp.func, cp.idx_EBX);
      } 
      else {
         x86_lea(cp.func, cp.idx_EBX, x86_make_disp(cp.idx_EBX, 4));
      }

   }
   /* decr count, loop if not zero
    */
   x86_dec(cp.func, cp.count_ESI);
   x86_jcc(cp.func, cc_NZ, label);

   restore_fpu_state(&cp);

   /* Land forward jump here:
    */
   x86_fixup_fwd_jump(cp.func, fixup);

   /* Exit mmx state?
    */
   if (cp.func->need_emms)
      mmx_emms(cp.func);

   x86_pop(cp.func, cp.count_ESI);
   x86_pop(cp.func, cp.idx_EBX);

   x87_assert_stack_empty(cp.func);
   x86_ret(cp.func);

   tgsi_parse_free( &parse );
   return !cp.error;

 fail:
   tgsi_parse_free( &parse );
   return FALSE;
}



static void vaos_set_buffer( struct draw_vs_varient *varient,
                             unsigned buf,
                             const void *ptr,
                             unsigned stride )
{
   struct draw_vs_varient_aos_sse *vaos = (struct draw_vs_varient_aos_sse *)varient;
   unsigned i;

   for (i = 0; i < vaos->base.vs->info.num_inputs; i++) {
      if (vaos->base.key.element[i].in.buffer == buf) {
         vaos->machine->attrib[i].input_ptr = ((char *)ptr +
                                               vaos->base.key.element[i].in.offset);
         vaos->machine->attrib[i].input_stride = stride;
      }
   }
}


static void vaos_destroy( struct draw_vs_varient *varient )
{
   struct draw_vs_varient_aos_sse *vaos = (struct draw_vs_varient_aos_sse *)varient;

   if (vaos->machine)
      align_free( vaos->machine );

   x86_release_func( &vaos->func[0] );
   x86_release_func( &vaos->func[1] );

   FREE(vaos);
}

static void vaos_run_elts( struct draw_vs_varient *varient,
                           const unsigned *elts,
                           unsigned count,
                           void *output_buffer )
{
   struct draw_vs_varient_aos_sse *vaos = (struct draw_vs_varient_aos_sse *)varient;

   vaos->gen_run_elts( varient,
                       elts,
                       count,
                       output_buffer );
}

static void vaos_run_linear( struct draw_vs_varient *varient,
                             unsigned start,
                             unsigned count,
                             void *output_buffer )
{
   struct draw_vs_varient_aos_sse *vaos = (struct draw_vs_varient_aos_sse *)varient;

   vaos->gen_run_linear( varient,
                         start,
                         count,
                         output_buffer );
}


static void vaos_set_constants( struct draw_vs_varient *varient,
                                const float (*constants)[4] )
{
   struct draw_vs_varient_aos_sse *vaos = (struct draw_vs_varient_aos_sse *)varient;

   memcpy(vaos->machine->constant,
          constants,
          (vaos->base.vs->info.file_max[TGSI_FILE_CONSTANT] + 1) * 4 * sizeof(float));

#if 0
   unsigned i;
   for (i =0; i < vaos->base.vs->info.file_max[TGSI_FILE_CONSTANT] + 1; i++)
      debug_printf("state %d: %f %f %f %f\n",
                   i, 
                   constants[i][0],
                   constants[i][1],
                   constants[i][2],
                   constants[i][3]);
#endif

   {
      unsigned i;
      for (i = 0; i < MAX_LIT_INFO; i++) {
         vaos->machine->lit_info[i].func = populate_lut;
         vaos->machine->now++;
      }
   }
}


static void vaos_set_viewport( struct draw_vs_varient *varient,
                               const struct pipe_viewport_state *viewport )
{
   struct draw_vs_varient_aos_sse *vaos = (struct draw_vs_varient_aos_sse *)varient;

   memcpy(vaos->machine->scale, viewport->scale, 4 * sizeof(float));
   memcpy(vaos->machine->translate, viewport->translate, 4 * sizeof(float));
}



static struct draw_vs_varient *varient_aos_sse( struct draw_vertex_shader *vs,
                                                 const struct draw_vs_varient_key *key )
{
   struct draw_vs_varient_aos_sse *vaos = CALLOC_STRUCT(draw_vs_varient_aos_sse);

   if (key->clip)
      return NULL;

   if (!vaos)
      goto fail;
   
   vaos->base.key = *key;
   vaos->base.vs = vs;
   vaos->base.set_input = vaos_set_buffer;
   vaos->base.set_constants = vaos_set_constants;
   vaos->base.set_viewport = vaos_set_viewport;
   vaos->base.destroy = vaos_destroy;
   vaos->base.run_linear = vaos_run_linear;
   vaos->base.run_elts = vaos_run_elts;

   vaos->machine = align_malloc( sizeof(struct aos_machine), 16 );
   if (!vaos->machine)
      goto fail;
   
   memset(vaos->machine, 0, sizeof(struct aos_machine));
   init_internals(vaos->machine);

   tgsi_dump(vs->state.tokens, 0);

   if (!build_vertex_program( vaos, TRUE ))
      goto fail;

   if (!build_vertex_program( vaos, FALSE ))
      goto fail;

   vaos->gen_run_linear = (vsv_run_linear_func)x86_get_func(&vaos->func[0]);
   if (!vaos->gen_run_linear)
      goto fail;

   vaos->gen_run_elts = (vsv_run_elts_func)x86_get_func(&vaos->func[1]);
   if (!vaos->gen_run_elts)
      goto fail;

   return &vaos->base;

 fail:
   if (vaos->machine)
      align_free( vaos->machine );

   if (vaos)
      x86_release_func( &vaos->func[0] );

   if (vaos)
      x86_release_func( &vaos->func[1] );

   FREE(vaos);
   
   return NULL;
}


struct draw_vs_varient *draw_vs_varient_aos_sse( struct draw_vertex_shader *vs,
                                                 const struct draw_vs_varient_key *key )
{
   struct draw_vs_varient *varient = varient_aos_sse( vs, key );

   if (varient == NULL) {
      assert(0);
      varient = draw_vs_varient_generic( vs, key );
   }

   return varient;
}



#endif
