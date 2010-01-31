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


#include "util/u_memory.h"
#include "util/u_math.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_debug.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "tgsi/tgsi_exec.h"
#include "tgsi/tgsi_dump.h"

#include "draw_vs.h"
#include "draw_vs_aos.h"

#include "rtasm/rtasm_x86sse.h"

#ifdef PIPE_ARCH_X86
#define DISASSEM 0
#define FAST_MATH 1

static const char *files[] =
{
   "NULL",
   "CONST",
   "IN",
   "OUT",
   "TEMP",
   "SAMP",
   "ADDR",
   "IMM",
   "INTERNAL",
};

static INLINE boolean eq( struct x86_reg a,
			    struct x86_reg b )
{
   return (a.file == b.file &&
	   a.idx == b.idx &&
	   a.mod == b.mod &&
	   a.disp == b.disp);
}
      
struct x86_reg aos_get_x86( struct aos_compilation *cp,
                            unsigned which_reg, /* quick hack */
                            unsigned value )
{
   struct x86_reg reg;

   if (which_reg == 0)
      reg = cp->temp_EBP;
   else
      reg = cp->tmp_EAX;

   if (cp->x86_reg[which_reg] != value) {
      unsigned offset;

      switch (value) {
      case X86_IMMEDIATES:
         assert(which_reg == 0);
         offset = Offset(struct aos_machine, immediates);
         break;
      case X86_CONSTANTS:
         assert(which_reg == 1);
         offset = Offset(struct aos_machine, constants);
         break;
      case X86_BUFFERS:
         assert(which_reg == 0);
         offset = Offset(struct aos_machine, buffer);
         break;
      default:
         assert(0);
         offset = 0;
      }


      x86_mov(cp->func, reg, 
              x86_make_disp(cp->machine_EDX, offset));

      cp->x86_reg[which_reg] = value;
   }

   return reg;
}


static struct x86_reg get_reg_ptr(struct aos_compilation *cp,
                                  unsigned file,
				  unsigned idx )
{
   struct x86_reg ptr = cp->machine_EDX;

   switch (file) {
   case TGSI_FILE_INPUT:
      assert(idx < MAX_INPUTS);
      return x86_make_disp(ptr, Offset(struct aos_machine, input[idx]));

   case TGSI_FILE_OUTPUT:
      return x86_make_disp(ptr, Offset(struct aos_machine, output[idx]));

   case TGSI_FILE_TEMPORARY:
      assert(idx < MAX_TEMPS);
      return x86_make_disp(ptr, Offset(struct aos_machine, temp[idx]));

   case AOS_FILE_INTERNAL:
      assert(idx < MAX_INTERNALS);
      return x86_make_disp(ptr, Offset(struct aos_machine, internal[idx]));

   case TGSI_FILE_IMMEDIATE: 
      assert(idx < MAX_IMMEDIATES);  /* just a sanity check */
      return x86_make_disp(aos_get_x86(cp, 0, X86_IMMEDIATES), idx * 4 * sizeof(float));

   case TGSI_FILE_CONSTANT: 
      assert(idx < MAX_CONSTANTS);  /* just a sanity check */
      return x86_make_disp(aos_get_x86(cp, 1, X86_CONSTANTS), idx * 4 * sizeof(float));

   default:
      AOS_ERROR(cp, "unknown reg file");
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




static void spill( struct aos_compilation *cp, unsigned idx )
{
   if (!cp->xmm[idx].dirty ||
       (cp->xmm[idx].file != TGSI_FILE_INPUT && /* inputs are fetched into xmm & set dirty */
        cp->xmm[idx].file != TGSI_FILE_OUTPUT &&
        cp->xmm[idx].file != TGSI_FILE_TEMPORARY)) {
      AOS_ERROR(cp, "invalid spill");
      return;
   }
   else {
      struct x86_reg oldval = get_reg_ptr(cp,
                                          cp->xmm[idx].file,
                                          cp->xmm[idx].idx);
     
      if (0) debug_printf("\nspill %s[%d]", 
                          files[cp->xmm[idx].file],
                          cp->xmm[idx].idx);
 
      assert(cp->xmm[idx].dirty);
      sse_movaps(cp->func, oldval, x86_make_reg(file_XMM, idx));
      cp->xmm[idx].dirty = 0;
   }
}


void aos_spill_all( struct aos_compilation *cp )
{
   unsigned i;

   for (i = 0; i < 8; i++) {
      if (cp->xmm[i].dirty) 
         spill(cp, i);
      aos_release_xmm_reg(cp, i);
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

   cp->xmm[reg.idx].last_used = cp->insn_counter;
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

   cp->xmm[reg.idx].last_used = cp->insn_counter;
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
   cp->xmm[oldest].dirty = 0;
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


static void aos_soft_release_xmm( struct aos_compilation *cp,
                                  struct x86_reg reg )
{
   if (reg.file == file_XMM) {
      assert(cp->xmm[reg.idx].last_used == cp->insn_counter);
      cp->xmm[reg.idx].last_used = cp->insn_counter - 1;
   }
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
          cp->xmm[i].idx == idx) 
      {
         /* If an xmm reg is already holding this shader reg, take into account its
          * dirty flag...
          */
         dirty |= cp->xmm[i].dirty;
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
   unsigned file = dst->Register.File;
   unsigned idx = dst->Register.Index;
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
   struct x86_reg reg = get_xmm( cp,
                                 aos_get_shader_reg( cp, file, idx ) );

   aos_adopt_xmm_reg( cp,
                      reg,
                      file,
                      idx,
                      FALSE );
   
   return reg;
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
                                            src->Register.File, 
                                            src->Register.Index);
   unsigned i;
   ubyte swz = 0;
   unsigned negs = 0;
   unsigned abs = 0;

   for (i = 0; i < 4; i++) {
      unsigned swizzle = tgsi_util_get_full_src_register_swizzle( src, i );
      unsigned neg = tgsi_util_get_full_src_register_sign_mode( src, i );

      swz |= (swizzle & 0x3) << (i * 2);

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
         AOS_ERROR(cp, "unsupported sign-mode");
         break;
      }
   }

   if (swz != SSE_SWIZZLE_NOOP || negs != 0 || abs != 0) {
      struct x86_reg dst = aos_get_xmm_reg(cp);

      if (swz != SSE_SWIZZLE_NOOP)
         emit_pshufd(cp, dst, arg0, swz);
      else
         sse_movaps(cp->func, dst, arg0);

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
         sse_mulps(cp->func, dst, tmp);

         aos_release_xmm_reg(cp, tmp.idx);
         aos_soft_release_xmm(cp, imm_swz);
      }
      else if (negs) {
         struct x86_reg imm_negs = aos_get_internal_xmm(cp, IMM_NEGS);
         sse_mulps(cp->func, dst, imm_negs);
         aos_soft_release_xmm(cp, imm_negs);
      }


      if (abs && abs != 0xf) {
         AOS_ERROR(cp, "unsupported partial abs");
      }
      else if (abs) {
         struct x86_reg neg = aos_get_internal(cp, IMM_NEGS);
         struct x86_reg tmp = aos_get_xmm_reg(cp);

         sse_movaps(cp->func, tmp, dst);
         sse_mulps(cp->func, tmp, neg);
         sse_maxps(cp->func, dst, tmp);

         aos_release_xmm_reg(cp, tmp.idx);
         aos_soft_release_xmm(cp, neg);
      }

      aos_soft_release_xmm(cp, arg0);
      return dst;
   }
      
   return arg0;
}

static void x87_fld_src( struct aos_compilation *cp, 
                         const struct tgsi_full_src_register *src,
                         unsigned channel ) 
{
   struct x86_reg arg0 = aos_get_shader_reg_ptr(cp, 
                                                src->Register.File, 
                                                src->Register.Index);

   unsigned swizzle = tgsi_util_get_full_src_register_swizzle( src, channel );
   unsigned neg = tgsi_util_get_full_src_register_sign_mode( src, channel );

   x87_fld( cp->func, x86_make_disp(arg0, (swizzle & 3) * sizeof(float)) );

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
      AOS_ERROR(cp, "unsupported sign-mode");
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

   switch (reg->Register.WriteMask) {
   case 0:
      return;
   
   case TGSI_WRITEMASK_XYZW:
      aos_adopt_xmm_reg(cp, 
                        get_xmm_writable(cp, result), 
                        reg->Register.File,
                        reg->Register.Index,
                        TRUE);
      return;
   default: 
      break;
   }

   dst = aos_get_shader_reg_xmm(cp, 
                                reg->Register.File,
                                reg->Register.Index);

   switch (reg->Register.WriteMask) {
   case TGSI_WRITEMASK_X:
      sse_movss(cp->func, dst, get_xmm(cp, result));
      break;
      
   case TGSI_WRITEMASK_ZW:
      sse_shufps(cp->func, dst, get_xmm(cp, result), SHUF(X, Y, Z, W));
      break;

   case TGSI_WRITEMASK_XY: 
      result = get_xmm_writable(cp, result);
      sse_shufps(cp->func, result, dst, SHUF(X, Y, Z, W));
      dst = result;
      break;

   case TGSI_WRITEMASK_YZW: 
      result = get_xmm_writable(cp, result);
      sse_movss(cp->func, result, dst);
      dst = result;
      break;

   default:
      mask_write(cp, dst, result, reg->Register.WriteMask);
      break;
   }

   aos_adopt_xmm_reg(cp, 
                     dst, 
                     reg->Register.File,
                     reg->Register.Index,
                     TRUE);

}

static void inject_scalar( struct aos_compilation *cp,
                           struct x86_reg dst,
                           struct x86_reg result,
                           ubyte swizzle )
{
   sse_shufps(cp->func, dst, dst, swizzle);
   sse_movss(cp->func, dst, result);
   sse_shufps(cp->func, dst, dst, swizzle);
}


static void store_scalar_dest( struct aos_compilation *cp, 
                               const struct tgsi_full_dst_register *reg,
                               struct x86_reg result )
{
   unsigned writemask = reg->Register.WriteMask;
   struct x86_reg dst;

   if (writemask != TGSI_WRITEMASK_X &&
       writemask != TGSI_WRITEMASK_Y &&
       writemask != TGSI_WRITEMASK_Z &&
       writemask != TGSI_WRITEMASK_W &&
       writemask != 0) 
   {
      result = get_xmm_writable(cp, result); /* already true, right? */
      sse_shufps(cp->func, result, result, SHUF(X,X,X,X));
      store_dest(cp, reg, result);
      return;
   }

   result = get_xmm(cp, result);
   dst = aos_get_shader_reg_xmm(cp, 
                                reg->Register.File,
                                reg->Register.Index);



   switch (reg->Register.WriteMask) {
   case TGSI_WRITEMASK_X:
      sse_movss(cp->func, dst, result);
      break;

   case TGSI_WRITEMASK_Y:
      inject_scalar(cp, dst, result, SHUF(Y, X, Z, W));
      break;

   case TGSI_WRITEMASK_Z:
      inject_scalar(cp, dst, result, SHUF(Z, Y, X, W));
      break;

   case TGSI_WRITEMASK_W:
      inject_scalar(cp, dst, result, SHUF(W, Y, Z, X));
      break;

   default:
      break;
   }

   aos_adopt_xmm_reg(cp, 
                     dst, 
                     reg->Register.File,
                     reg->Register.Index,
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
   unsigned writemask = dst->Register.WriteMask;

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

#if 0
static void x87_emit_ex2( struct aos_compilation *cp )
{
   struct x86_reg st0 = x86_make_reg(file_x87, 0);
   struct x86_reg st1 = x86_make_reg(file_x87, 1);
   int stack = cp->func->x87_stack;

   /* set_fpu_round_neg_inf( cp ); */

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
#endif

#if 0
static void PIPE_CDECL print_reg( const char *msg,
                                  const float *reg )
{
   debug_printf("%s: %f %f %f %f\n", msg, reg[0], reg[1], reg[2], reg[3]);
}
#endif

#if 0
static void emit_print( struct aos_compilation *cp,
                        const char *message, /* must point to a static string! */
                        unsigned file,
                        unsigned idx )
{
   struct x86_reg ecx = x86_make_reg( file_REG32, reg_CX );
   struct x86_reg arg = aos_get_shader_reg_ptr( cp, file, idx );
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
#endif

/**
 * The traditional instructions.  All operate on internal registers
 * and ignore write masks and swizzling issues.
 */

static boolean emit_ABS( struct aos_compilation *cp, const struct tgsi_full_instruction *op ) 
{
   struct x86_reg arg0 = fetch_src(cp, &op->Src[0]);
   struct x86_reg neg = aos_get_internal(cp, IMM_NEGS);
   struct x86_reg tmp = aos_get_xmm_reg(cp);

   sse_movaps(cp->func, tmp, arg0);
   sse_mulps(cp->func, tmp, neg);
   sse_maxps(cp->func, tmp, arg0);
   
   store_dest(cp, &op->Dst[0], tmp);
   return TRUE;
}

static boolean emit_ADD( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->Src[0]);
   struct x86_reg arg1 = fetch_src(cp, &op->Src[1]);
   struct x86_reg dst = get_xmm_writable(cp, arg0);

   sse_addps(cp->func, dst, arg1);

   store_dest(cp, &op->Dst[0], dst);
   return TRUE;
}

static boolean emit_COS( struct aos_compilation *cp, const struct tgsi_full_instruction *op ) 
{
   x87_fld_src(cp, &op->Src[0], 0);
   x87_fcos(cp->func);
   x87_fstp_dest4(cp, &op->Dst[0]);
   return TRUE;
}

/* The dotproduct instructions don't really do that well in sse:
 * XXX: produces wrong results -- disabled.
 */
static boolean emit_DP3( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->Src[0]);
   struct x86_reg arg1 = fetch_src(cp, &op->Src[1]);
   struct x86_reg tmp = aos_get_xmm_reg(cp); 
   struct x86_reg dst = get_xmm_writable(cp, arg0);

   sse_mulps(cp->func, dst, arg1);
   /* Now the hard bit: sum the first 3 values:
    */ 
   sse_movhlps(cp->func, tmp, dst);
   sse_addss(cp->func, dst, tmp); /* a*x+c*z, b*y, ?, ? */
   emit_pshufd(cp, tmp, dst, SHUF(Y,X,W,Z));
   sse_addss(cp->func, dst, tmp);
   
   aos_release_xmm_reg(cp, tmp.idx);
   store_scalar_dest(cp, &op->Dst[0], dst);
   return TRUE;
}

static boolean emit_DP4( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->Src[0]);
   struct x86_reg arg1 = fetch_src(cp, &op->Src[1]);
   struct x86_reg tmp = aos_get_xmm_reg(cp);      
   struct x86_reg dst = get_xmm_writable(cp, arg0);

   sse_mulps(cp->func, dst, arg1);
   
   /* Now the hard bit: sum the values:
    */ 
   sse_movhlps(cp->func, tmp, dst);
   sse_addps(cp->func, dst, tmp); /* a*x+c*z, b*y+d*w, a*x+c*z, b*y+d*w */
   emit_pshufd(cp, tmp, dst, SHUF(Y,X,W,Z));
   sse_addss(cp->func, dst, tmp);

   aos_release_xmm_reg(cp, tmp.idx);
   store_scalar_dest(cp, &op->Dst[0], dst);
   return TRUE;
}

static boolean emit_DPH( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->Src[0]);
   struct x86_reg arg1 = fetch_src(cp, &op->Src[1]);
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

   aos_release_xmm_reg(cp, tmp.idx);
   store_scalar_dest(cp, &op->Dst[0], dst);
   return TRUE;
}

static boolean emit_DST( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
    struct x86_reg arg0 = fetch_src(cp, &op->Src[0]);
    struct x86_reg arg1 = fetch_src(cp, &op->Src[1]);
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
    store_dest(cp, &op->Dst[0], dst);
    return TRUE;
}

static boolean emit_LG2( struct aos_compilation *cp, const struct tgsi_full_instruction *op ) 
{
   x87_fld1(cp->func);		/* 1 */
   x87_fld_src(cp, &op->Src[0], 0);	/* a0 1 */
   x87_fyl2x(cp->func);	/* log2(a0) */
   x87_fstp_dest4(cp, &op->Dst[0]);
   return TRUE;
}

#if 0
static boolean emit_EX2( struct aos_compilation *cp, const struct tgsi_full_instruction *op ) 
{
   x87_fld_src(cp, &op->Src[0], 0);
   x87_emit_ex2(cp);
   x87_fstp_dest4(cp, &op->Dst[0]);
   return TRUE;
}
#endif


static boolean emit_FLR( struct aos_compilation *cp, const struct tgsi_full_instruction *op ) 
{
   struct x86_reg dst = get_dst_ptr(cp, &op->Dst[0]); 
   unsigned writemask = op->Dst[0].Register.WriteMask;
   int i;

   set_fpu_round_neg_inf( cp );

   /* Load all sources first to avoid aliasing
    */
   for (i = 3; i >= 0; i--) {
      if (writemask & (1<<i)) {
         x87_fld_src(cp, &op->Src[0], i);   
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
   struct x86_reg dst = get_dst_ptr(cp, &op->Dst[0]); 
   unsigned writemask = op->Dst[0].Register.WriteMask;
   int i;

   set_fpu_round_nearest( cp );

   /* Load all sources first to avoid aliasing
    */
   for (i = 3; i >= 0; i--) {
      if (writemask & (1<<i)) {
         x87_fld_src(cp, &op->Src[0], i);   
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
   struct x86_reg dst = get_dst_ptr(cp, &op->Dst[0]); 
   struct x86_reg st0 = x86_make_reg(file_x87, 0);
   struct x86_reg st1 = x86_make_reg(file_x87, 1);
   unsigned writemask = op->Dst[0].Register.WriteMask;
   int i;

   set_fpu_round_neg_inf( cp );

   /* suck all the source values onto the stack before writing out any
    * dst, which may alias...
    */
   for (i = 3; i >= 0; i--) {
      if (writemask & (1<<i)) {
         x87_fld_src(cp, &op->Src[0], i);   
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






static boolean emit_LIT( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg ecx = x86_make_reg( file_REG32, reg_CX );
   unsigned writemask = op->Dst[0].Register.WriteMask;
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
      result = get_dst_ptr(cp, &op->Dst[0]);    

   
   arg0 = fetch_src( cp, &op->Src[0] );
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
      x86_mov_reg_imm( cp->func, ecx, (int)aos_do_lit );
   }

   x86_call( cp->func, ecx );
            
   x86_pop( cp->func, ecx );    /* fixme... */
   x86_pop( cp->func, ecx );
   x86_pop( cp->func, ecx );
   x86_pop( cp->func, ecx );

   x86_cdecl_caller_pop_regs( cp->func );

   if (writemask != TGSI_WRITEMASK_XYZW) {
      store_dest( cp, 
                  &op->Dst[0],
                  get_xmm_writable( cp, result ) );
   }

   return TRUE;
}

#if 0   
static boolean emit_inline_LIT( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg dst = get_dst_ptr(cp, &op->Dst[0]); 
   unsigned writemask = op->Dst[0].Register.WriteMask;

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
      x87_fld_src(cp, &op->Src[0], 1); /* a1 1 0  */
      x87_fcomi(cp->func, st2);	                    /* a1 1 0  */
      x87_fcmovb(cp->func, st1);                    /* a1' 1 0  */
      x87_fstp(cp->func, st1);                      /* a1' 0  */
      x87_fstp(cp->func, st1);                      /* a1'  */

      x87_fld_src(cp, &op->Src[0], 3); /* a3 a1'  */
      x87_fxch(cp->func, st1);                      /* a1' a3  */
      

      /* Compute pow(a1, a3)
       */
      x87_fyl2x(cp->func);	/* a3*log2(a1)      */
      x87_emit_ex2( cp );       /* 2^(a3*log2(a1))   */


      /* a0' = max2(a0, 0):
       */
      x87_fldz(cp->func);                           /* 0 r2 */
      x87_fld_src(cp, &op->Src[0], 0); /* a0 0 r2 */
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
#endif



static boolean emit_MAX( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->Src[0]);
   struct x86_reg arg1 = fetch_src(cp, &op->Src[1]);
   struct x86_reg dst = get_xmm_writable(cp, arg0);

   sse_maxps(cp->func, dst, arg1);

   store_dest(cp, &op->Dst[0], dst);
   return TRUE;
}


static boolean emit_MIN( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->Src[0]);
   struct x86_reg arg1 = fetch_src(cp, &op->Src[1]);
   struct x86_reg dst = get_xmm_writable(cp, arg0);

   sse_minps(cp->func, dst, arg1);

   store_dest(cp, &op->Dst[0], dst);
   return TRUE;
}

static boolean emit_MOV( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->Src[0]);
   struct x86_reg dst = get_xmm_writable(cp, arg0);

   /* potentially nothing to do */

   store_dest(cp, &op->Dst[0], dst);
   return TRUE;
}

static boolean emit_MUL( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->Src[0]);
   struct x86_reg arg1 = fetch_src(cp, &op->Src[1]);
   struct x86_reg dst = get_xmm_writable(cp, arg0);

   sse_mulps(cp->func, dst, arg1);

   store_dest(cp, &op->Dst[0], dst);
   return TRUE;
}


static boolean emit_MAD( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->Src[0]);
   struct x86_reg arg1 = fetch_src(cp, &op->Src[1]);
   struct x86_reg arg2 = fetch_src(cp, &op->Src[2]);

   /* If we can't clobber old contents of arg0, get a temporary & copy
    * it there, then clobber it...
    */
   arg0 = get_xmm_writable(cp, arg0);

   sse_mulps(cp->func, arg0, arg1);
   sse_addps(cp->func, arg0, arg2);
   store_dest(cp, &op->Dst[0], arg0);
   return TRUE;
}



/* A wrapper for powf().
 * Makes sure it is cdecl and operates on floats.
 */
static float PIPE_CDECL _powerf( float x, float y )
{
#if FAST_MATH
   return util_fast_pow(x, y);
#else
   return powf( x, y );
#endif
}

#if FAST_MATH
static float PIPE_CDECL _exp2(float x)
{
   return util_fast_exp2(x);
}
#endif


/* Really not sufficient -- need to check for conditions that could
 * generate inf/nan values, which will slow things down hugely.
 */
static boolean emit_POW( struct aos_compilation *cp, const struct tgsi_full_instruction *op ) 
{
#if 0
   x87_fld_src(cp, &op->Src[1], 0);  /* a1.x */
   x87_fld_src(cp, &op->Src[0], 0);	/* a0.x a1.x */
   x87_fyl2x(cp->func);	                                /* a1*log2(a0) */

   x87_emit_ex2( cp );		/* 2^(a1*log2(a0)) */

   x87_fstp_dest4(cp, &op->Dst[0]);
#else
   uint i;

   /* For absolute correctness, need to spill/invalidate all XMM regs
    * too.  
    */
   for (i = 0; i < 8; i++) {
      if (cp->xmm[i].dirty) 
         spill(cp, i);
      aos_release_xmm_reg(cp, i);
   }

   /* Push caller-save (ie scratch) regs.  
    */
   x86_cdecl_caller_push_regs( cp->func );

   x86_lea( cp->func, cp->stack_ESP, x86_make_disp(cp->stack_ESP, -8) );

   x87_fld_src( cp, &op->Src[1], 0 );
   x87_fstp( cp->func, x86_make_disp( cp->stack_ESP, 4 ) );
   x87_fld_src( cp, &op->Src[0], 0 );
   x87_fstp( cp->func, x86_make_disp( cp->stack_ESP, 0 ) );

   /* tmp_EAX has been pushed & will be restored below */
   x86_mov_reg_imm( cp->func, cp->tmp_EAX, (unsigned long) _powerf );
   x86_call( cp->func, cp->tmp_EAX );

   x86_lea( cp->func, cp->stack_ESP, x86_make_disp(cp->stack_ESP, 8) );

   x86_cdecl_caller_pop_regs( cp->func );

   /* Note retval on x87 stack:
    */
   cp->func->x87_stack++;

   x87_fstp_dest4( cp, &op->Dst[0] );
#endif
   return TRUE;
}


#if FAST_MATH
static boolean emit_EXPBASE2( struct aos_compilation *cp, const struct tgsi_full_instruction *op ) 
{
   uint i;

   /* For absolute correctness, need to spill/invalidate all XMM regs
    * too.  
    */
   for (i = 0; i < 8; i++) {
      if (cp->xmm[i].dirty) 
         spill(cp, i);
      aos_release_xmm_reg(cp, i);
   }

   /* Push caller-save (ie scratch) regs.  
    */
   x86_cdecl_caller_push_regs( cp->func );

   x86_lea( cp->func, cp->stack_ESP, x86_make_disp(cp->stack_ESP, -4) );

   x87_fld_src( cp, &op->Src[0], 0 );
   x87_fstp( cp->func, x86_make_disp( cp->stack_ESP, 0 ) );

   /* tmp_EAX has been pushed & will be restored below */
   x86_mov_reg_imm( cp->func, cp->tmp_EAX, (unsigned long) _exp2 );
   x86_call( cp->func, cp->tmp_EAX );

   x86_lea( cp->func, cp->stack_ESP, x86_make_disp(cp->stack_ESP, 4) );

   x86_cdecl_caller_pop_regs( cp->func );

   /* Note retval on x87 stack:
    */
   cp->func->x87_stack++;

   x87_fstp_dest4( cp, &op->Dst[0] );

   return TRUE;
}
#endif


static boolean emit_RCP( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->Src[0]);
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

   store_scalar_dest(cp, &op->Dst[0], dst);
   return TRUE;
}


/* Although rsqrtps() and rcpps() are low precision on some/all SSE
 * implementations, it is possible to improve its precision at
 * fairly low cost, using a newton/raphson step, as below:
 * 
 * x1 = 2 * rcpps(a) - a * rcpps(a) * rcpps(a)
 * x1 = 0.5 * rsqrtps(a) * [3.0 - (a * rsqrtps(a))* rsqrtps(a)]
 * or:
 *   x1 = rsqrtps(a) * [1.5 - .5 * a * rsqrtps(a) * rsqrtps(a)]
 * 
 *
 * See: http://softwarecommunity.intel.com/articles/eng/1818.htm
 */
static boolean emit_RSQ( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   if (0) {
      struct x86_reg arg0 = fetch_src(cp, &op->Src[0]);
      struct x86_reg r = aos_get_xmm_reg(cp);
      sse_rsqrtss(cp->func, r, arg0);
      store_scalar_dest(cp, &op->Dst[0], r);
      return TRUE;
   }
   else {
      struct x86_reg arg0           = fetch_src(cp, &op->Src[0]);
      struct x86_reg r              = aos_get_xmm_reg(cp);

      struct x86_reg neg_half       = get_reg_ptr( cp, AOS_FILE_INTERNAL, IMM_RSQ );
      struct x86_reg one_point_five = x86_make_disp( neg_half, 4 );
      struct x86_reg src            = get_xmm_writable( cp, arg0 );
      struct x86_reg neg            = aos_get_internal(cp, IMM_NEGS);
      struct x86_reg tmp            = aos_get_xmm_reg(cp);

      sse_movaps(cp->func, tmp, src);
      sse_mulps(cp->func, tmp, neg);
      sse_maxps(cp->func, tmp, src);
   
      sse_rsqrtss( cp->func, r, tmp  );             /* rsqrtss(a) */
      sse_mulss(   cp->func, tmp, neg_half  );      /* -.5 * a */
      sse_mulss(   cp->func, tmp,  r );             /* -.5 * a * r */
      sse_mulss(   cp->func, tmp,  r );             /* -.5 * a * r * r */
      sse_addss(   cp->func, tmp, one_point_five ); /* 1.5 - .5 * a * r * r */
      sse_mulss(   cp->func, r,  tmp );             /* r * (1.5 - .5 * a * r * r) */

      store_scalar_dest(cp, &op->Dst[0], r);

      aos_release_xmm_reg(cp, tmp.idx);

      return TRUE;
   }
}


static boolean emit_SGE( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->Src[0]);
   struct x86_reg arg1 = fetch_src(cp, &op->Src[1]);
   struct x86_reg ones = aos_get_internal(cp, IMM_ONES);
   struct x86_reg dst = get_xmm_writable(cp, arg0);

   sse_cmpps(cp->func, dst, arg1, cc_NotLessThan);
   sse_andps(cp->func, dst, ones);

   store_dest(cp, &op->Dst[0], dst);
   return TRUE;
}

static boolean emit_SIN( struct aos_compilation *cp, const struct tgsi_full_instruction *op ) 
{
   x87_fld_src(cp, &op->Src[0], 0);
   x87_fsin(cp->func);
   x87_fstp_dest4(cp, &op->Dst[0]);
   return TRUE;
}



static boolean emit_SLT( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->Src[0]);
   struct x86_reg arg1 = fetch_src(cp, &op->Src[1]);
   struct x86_reg ones = aos_get_internal(cp, IMM_ONES);
   struct x86_reg dst = get_xmm_writable(cp, arg0);
   
   sse_cmpps(cp->func, dst, arg1, cc_LessThan);
   sse_andps(cp->func, dst, ones);

   store_dest(cp, &op->Dst[0], dst);
   return TRUE;
}

static boolean emit_SUB( struct aos_compilation *cp, const struct tgsi_full_instruction *op ) 
{
   struct x86_reg arg0 = fetch_src(cp, &op->Src[0]);
   struct x86_reg arg1 = fetch_src(cp, &op->Src[1]);
   struct x86_reg dst = get_xmm_writable(cp, arg0);

   sse_subps(cp->func, dst, arg1);

   store_dest(cp, &op->Dst[0], dst);
   return TRUE;
}

static boolean emit_TRUNC( struct aos_compilation *cp, const struct tgsi_full_instruction *op )
{
   struct x86_reg arg0 = fetch_src(cp, &op->Src[0]);
   struct x86_reg tmp0 = aos_get_xmm_reg(cp);

   sse2_cvttps2dq(cp->func, tmp0, arg0);
   sse2_cvtdq2ps(cp->func, tmp0, tmp0);

   store_dest(cp, &op->Dst[0], tmp0);
   return TRUE;
}

static boolean emit_XPD( struct aos_compilation *cp, const struct tgsi_full_instruction *op ) 
{
   struct x86_reg arg0 = fetch_src(cp, &op->Src[0]);
   struct x86_reg arg1 = fetch_src(cp, &op->Src[1]);
   struct x86_reg tmp0 = aos_get_xmm_reg(cp);
   struct x86_reg tmp1 = aos_get_xmm_reg(cp);

   emit_pshufd(cp, tmp1, arg1, SHUF(Y, Z, X, W));
   sse_mulps(cp->func, tmp1, arg0);
   emit_pshufd(cp, tmp0, arg0, SHUF(Y, Z, X, W));
   sse_mulps(cp->func, tmp0, arg1);
   sse_subps(cp->func, tmp1, tmp0);
   sse_shufps(cp->func, tmp1, tmp1, SHUF(Y, Z, X, W));

/*    dst[2] = arg0[0] * arg1[1] - arg0[1] * arg1[0]; */
/*    dst[0] = arg0[1] * arg1[2] - arg0[2] * arg1[1]; */
/*    dst[1] = arg0[2] * arg1[0] - arg0[0] * arg1[2]; */
/*    dst[3] is undef */


   aos_release_xmm_reg(cp, tmp0.idx);
   store_dest(cp, &op->Dst[0], tmp1);
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
      /*return emit_EXP(cp, inst);*/
      return FALSE;

   case TGSI_OPCODE_LOG:
      /*return emit_LOG(cp, inst);*/
      return FALSE;

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
 
   case TGSI_OPCODE_LRP:
      /*return emit_LERP(cp, inst);*/
      return FALSE;

   case TGSI_OPCODE_FRC:
      return emit_FRC(cp, inst);

   case TGSI_OPCODE_CLAMP:
      /*return emit_CLAMP(cp, inst);*/
      return FALSE;

   case TGSI_OPCODE_FLR:
      return emit_FLR(cp, inst);

   case TGSI_OPCODE_ROUND:
      return emit_RND(cp, inst);

   case TGSI_OPCODE_EX2:
#if FAST_MATH
      return emit_EXPBASE2(cp, inst);
#elif 0
      /* this seems to fail for "larger" exponents.
       * See glean tvertProg1's EX2 test.
       */
      return emit_EX2(cp, inst);
#else
      return FALSE;
#endif

   case TGSI_OPCODE_LG2:
      return emit_LG2(cp, inst);

   case TGSI_OPCODE_POW:
      return emit_POW(cp, inst);

   case TGSI_OPCODE_XPD:
      return emit_XPD(cp, inst);

   case TGSI_OPCODE_ABS:
      return emit_ABS(cp, inst);

   case TGSI_OPCODE_DPH:
      return emit_DPH(cp, inst);

   case TGSI_OPCODE_COS:
      return emit_COS(cp, inst);

   case TGSI_OPCODE_SIN:
      return emit_SIN(cp, inst);

   case TGSI_OPCODE_TRUNC:
      return emit_TRUNC(cp, inst);

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
                                               cp->vaos->draw->vs.position_output );

   struct x86_reg scale = x86_make_disp(cp->machine_EDX, 
                                        Offset(struct aos_machine, scale));

   struct x86_reg translate = x86_make_disp(cp->machine_EDX, 
                                        Offset(struct aos_machine, translate));

   sse_mulps(cp->func, pos, scale);
   sse_addps(cp->func, pos, translate);

   aos_adopt_xmm_reg( cp,
                      pos,
                      TGSI_FILE_OUTPUT,
                      cp->vaos->draw->vs.position_output,
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
                                               cp->vaos->draw->vs.position_output);

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
                      cp->vaos->draw->vs.position_output,
                      TRUE );
   return TRUE;
}


#if 0
static boolean note_immediate( struct aos_compilation *cp,
                               struct tgsi_full_immediate *imm )
{
   unsigned pos = cp->num_immediates++;
   unsigned j;

   assert( imm->Immediate.NrTokens <= 4 + 1 );
   for (j = 0; j < imm->Immediate.NrTokens - 1; j++) {
      cp->vaos->machine->immediate[pos][j] = imm->u[j].Float;
   }

   return TRUE;
}
#endif




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
         if (parse.FullToken.FullInstruction.Dst[i].Register.File ==
             TGSI_FILE_OUTPUT) 
         {
            unsigned idx = parse.FullToken.FullInstruction.Dst[i].Register.Index;
            cp->output_last_write[idx] = this_instruction;
         }
      }

      this_instruction++;
   }

   tgsi_parse_free( &parse );
}


#define ARG_MACHINE    1
#define ARG_START_ELTS 2
#define ARG_COUNT      3
#define ARG_OUTBUF     4


static boolean build_vertex_program( struct draw_vs_varient_aos_sse *varient,
                                     boolean linear )
{ 
   struct tgsi_parse_context parse;
   struct aos_compilation cp;
   unsigned fixup, label;

   util_init_math();

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
   cp.temp_EBP     = x86_make_reg(file_REG32, reg_BP);
   cp.stack_ESP = x86_make_reg( file_REG32, reg_SP );

   x86_init_func(cp.func);

   find_last_write_outputs(&cp);

   x86_push(cp.func, cp.idx_EBX);
   x86_push(cp.func, cp.count_ESI);
   x86_push(cp.func, cp.temp_EBP);


   /* Load arguments into regs:
    */
   x86_mov(cp.func, cp.machine_EDX, x86_fn_arg(cp.func, ARG_MACHINE));
   x86_mov(cp.func, cp.idx_EBX, x86_fn_arg(cp.func, ARG_START_ELTS));
   x86_mov(cp.func, cp.count_ESI, x86_fn_arg(cp.func, ARG_COUNT));
   x86_mov(cp.func, cp.outbuf_ECX, x86_fn_arg(cp.func, ARG_OUTBUF));


   /* Compare count to zero and possibly bail.
    */
   x86_xor(cp.func, cp.tmp_EAX, cp.tmp_EAX);
   x86_cmp(cp.func, cp.count_ESI, cp.tmp_EAX);
   fixup = x86_jcc_forward(cp.func, cc_E);


   save_fpu_state( &cp );
   set_fpu_round_nearest( &cp );

   aos_init_inputs( &cp, linear );

   cp.x86_reg[0] = 0;
   cp.x86_reg[1] = 0;
   
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
#if 0
            if (!note_immediate( &cp, &parse.FullToken.FullImmediate ))
               goto fail;
#endif
            break;

         case TGSI_TOKEN_TYPE_INSTRUCTION:
            if (DISASSEM)
               tgsi_dump_instruction( &parse.FullToken.FullInstruction, cp.insn_counter );

            if (!emit_instruction( &cp, &parse.FullToken.FullInstruction ))
               goto fail;
            break;
         }

         x87_assert_stack_empty(cp.func);
         cp.insn_counter++;

         if (DISASSEM)
            debug_printf("\n");
      }

   
      {
         unsigned i;
         for (i = 0; i < 8; i++) {
            if (cp.xmm[i].file != TGSI_FILE_OUTPUT) {
               cp.xmm[i].file = TGSI_FILE_NULL;
               cp.xmm[i].dirty = 0;
            }
         }
      }

      if (cp.error)
         goto fail;

      if (cp.vaos->base.key.clip) {
         /* not really handling clipping, just do the rhw so we can
          * see the results...
          */
         emit_rhw_viewport(&cp); 
      }
      else if (cp.vaos->base.key.viewport) {
         emit_viewport(&cp);
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
      aos_incr_inputs( &cp, linear );
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

   x86_pop(cp.func, cp.temp_EBP);
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

   if (buf < vaos->nr_vb) {
      vaos->buffer[buf].base_ptr = (char *)ptr;
      vaos->buffer[buf].stride = stride;
   }

   if (0) debug_printf("%s %d/%d: %p %d\n", __FUNCTION__, buf, vaos->nr_vb, ptr, stride);
}



static void PIPE_CDECL vaos_run_elts( struct draw_vs_varient *varient,
                                      const unsigned *elts,
                                      unsigned count,
                                      void *output_buffer )
{
   struct draw_vs_varient_aos_sse *vaos = (struct draw_vs_varient_aos_sse *)varient;
   struct aos_machine *machine = vaos->draw->vs.aos_machine;
   unsigned i;

   if (0) debug_printf("%s %d\n", __FUNCTION__, count);

   machine->internal[IMM_PSIZE][0] = vaos->draw->rasterizer->point_size;
   for (i = 0; i < PIPE_MAX_CONSTANT_BUFFERS; i++) {
      machine->constants[i] = vaos->draw->vs.aligned_constants[i];
   }
   machine->immediates = vaos->base.vs->immediates;
   machine->buffer = vaos->buffer;

   vaos->gen_run_elts( machine,
                       elts,
                       count,
                       output_buffer );
}

static void PIPE_CDECL vaos_run_linear( struct draw_vs_varient *varient,
                                        unsigned start,
                                        unsigned count,
                                        void *output_buffer )
{
   struct draw_vs_varient_aos_sse *vaos = (struct draw_vs_varient_aos_sse *)varient;
   struct aos_machine *machine = vaos->draw->vs.aos_machine;
   unsigned i;

   if (0) debug_printf("%s %d %d const: %x\n", __FUNCTION__, start, count, 
                       vaos->base.key.const_vbuffers);

   machine->internal[IMM_PSIZE][0] = vaos->draw->rasterizer->point_size;
   for (i = 0; i < PIPE_MAX_CONSTANT_BUFFERS; i++) {
      machine->constants[i] = vaos->draw->vs.aligned_constants[i];
   }
   machine->immediates = vaos->base.vs->immediates;
   machine->buffer = vaos->buffer;

   vaos->gen_run_linear( machine,
                         start,
                         count,
                         output_buffer );

   /* Sanity spot checks to make sure we didn't trash our constants */
   assert(machine->internal[IMM_ONES][0] == 1.0f);
   assert(machine->internal[IMM_IDENTITY][0] == 0.0f);
   assert(machine->internal[IMM_NEGS][0] == -1.0f);
}



static void vaos_destroy( struct draw_vs_varient *varient )
{
   struct draw_vs_varient_aos_sse *vaos = (struct draw_vs_varient_aos_sse *)varient;

   FREE( vaos->buffer );

   x86_release_func( &vaos->func[0] );
   x86_release_func( &vaos->func[1] );

   FREE(vaos);
}



static struct draw_vs_varient *varient_aos_sse( struct draw_vertex_shader *vs,
                                                 const struct draw_vs_varient_key *key )
{
   unsigned i;
   struct draw_vs_varient_aos_sse *vaos = CALLOC_STRUCT(draw_vs_varient_aos_sse);

   if (!vaos)
      goto fail;
   
   vaos->base.key = *key;
   vaos->base.vs = vs;
   vaos->base.set_buffer = vaos_set_buffer;
   vaos->base.destroy = vaos_destroy;
   vaos->base.run_linear = vaos_run_linear;
   vaos->base.run_elts = vaos_run_elts;

   vaos->draw = vs->draw;

   for (i = 0; i < key->nr_inputs; i++) 
      vaos->nr_vb = MAX2( vaos->nr_vb, key->element[i].in.buffer + 1 );

   vaos->buffer = MALLOC( vaos->nr_vb * sizeof(vaos->buffer[0]) );
   if (!vaos->buffer)
      goto fail;

   if (0)
      debug_printf("nr_vb: %d const: %x\n", vaos->nr_vb, vaos->base.key.const_vbuffers);

#if 0
   tgsi_dump(vs->state.tokens, 0);
#endif

   if (!build_vertex_program( vaos, TRUE ))
      goto fail;

   if (!build_vertex_program( vaos, FALSE ))
      goto fail;

   vaos->gen_run_linear = (vaos_run_linear_func)x86_get_func(&vaos->func[0]);
   if (!vaos->gen_run_linear)
      goto fail;

   vaos->gen_run_elts = (vaos_run_elts_func)x86_get_func(&vaos->func[1]);
   if (!vaos->gen_run_elts)
      goto fail;

   return &vaos->base;

 fail:
   if (vaos && vaos->buffer)
      FREE(vaos->buffer);

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
      varient = draw_vs_varient_generic( vs, key );
   }

   return varient;
}



#endif /* PIPE_ARCH_X86 */
