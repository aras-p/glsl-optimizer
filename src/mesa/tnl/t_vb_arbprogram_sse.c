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
 * \file t_vb_arb_program_sse.c
 *
 * Translate simplified vertex_program representation to x86/SSE/SSE2
 * machine code using mesa's rtasm runtime assembler.
 *
 * \author Keith Whitwell
 */

#include "glheader.h"
#include "context.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"
#include "arbprogparse.h"
#include "program.h"
#include "math/m_matrix.h"
#include "math/m_translate.h"
#include "t_context.h"
#include "t_vb_arbprogram.h"

#if defined(USE_SSE_ASM)

#include "x86/rtasm/x86sse.h"
#include "x86/common_x86_asm.h"


#define X    0
#define Y    1
#define Z    2
#define W    3

/* Reg usage:
 *
 * EAX - point to 'm->File[0]'
 * ECX - point to 'm->File[3]'
 * EDX,
 * EBX,
 * ESP,
 * EBP,
 * ESI,
 * EDI
 */



#define FAIL								\
do {									\
   _mesa_printf("x86 translation failed in %s\n", __FUNCTION__);	\
   return GL_FALSE;							\
} while (0)

struct compilation {
   struct x86_function func;
   struct arb_vp_machine *m;

   GLuint insn_counter;

   struct {
      GLuint file:2;
      GLuint idx:7;
      GLuint dirty:1;
      GLuint last_used:10;
   } xmm[8];

   struct {
      struct x86_reg base;
   } file[4];

   GLboolean have_sse2;
};

static INLINE GLboolean eq( struct x86_reg a,
			    struct x86_reg b )
{
   return (a.file == b.file &&
	   a.idx == b.idx &&
	   a.mod == b.mod &&
	   a.disp == b.disp);
}
      


static struct x86_reg get_reg_ptr(GLuint file,
				  GLuint idx )
{
   struct x86_reg reg;

   switch (file) {
   case FILE_REG:
      reg = x86_make_reg(file_REG32, reg_AX);
      assert(idx != REG_UNDEF);
      break;
   case FILE_STATE_PARAM:
      reg = x86_make_reg(file_REG32, reg_CX);
      break;
   default:
      assert(0);
   }

   return x86_make_disp(reg, 16 * idx);
}
			  

static void spill( struct compilation *cp, GLuint idx )
{
   struct x86_reg oldval = get_reg_ptr(cp->xmm[idx].file,
				       cp->xmm[idx].idx);

   assert(cp->xmm[idx].dirty);
   sse_movups(&cp->func, oldval, x86_make_reg(file_XMM, idx));
   cp->xmm[idx].dirty = 0;
}

static struct x86_reg get_xmm_reg( struct compilation *cp )
{
   GLuint i;
   GLuint oldest = 0;

   for (i = 0; i < 8; i++) 
      if (cp->xmm[i].last_used < cp->xmm[oldest].last_used)
	 oldest = i;

   /* Need to write out the old value?
    */
   if (cp->xmm[oldest].dirty) 
      spill(cp, oldest);

   assert(cp->xmm[oldest].last_used != cp->insn_counter);

   cp->xmm[oldest].file = FILE_REG;
   cp->xmm[oldest].idx = REG_UNDEF;
   cp->xmm[oldest].last_used = cp->insn_counter;
   return x86_make_reg(file_XMM, oldest);
}

      


static struct x86_reg get_dst_reg( struct compilation *cp, 
				   GLuint file, GLuint idx )
{
   struct x86_reg reg;
   GLuint i;

   /* Invalidate any old copy of this register in XMM0-7.  Don't reuse
    * as this may be one of the arguments.
    */
   for (i = 0; i < 8; i++) {
      if (cp->xmm[i].file == file && cp->xmm[i].idx == idx) {
	 cp->xmm[i].file = FILE_REG;
	 cp->xmm[i].idx = REG_UNDEF;
	 cp->xmm[i].dirty = 0;
	 break;
      }
   }

   reg = get_xmm_reg( cp );
   cp->xmm[reg.idx].file = file;
   cp->xmm[reg.idx].idx = idx;
   cp->xmm[reg.idx].dirty = 1;
   return reg;   
}


/* Return an XMM reg if the argument is resident, otherwise return a
 * base+offset pointer to the saved value.
 */
static struct x86_reg get_arg( struct compilation *cp, GLuint file, GLuint idx )
{
   GLuint i;

   for (i = 0; i < 8; i++) {
      if (cp->xmm[i].file == file &&
	  cp->xmm[i].idx == idx) {
	 cp->xmm[i].last_used = cp->insn_counter;
	 return x86_make_reg(file_XMM, i);
      }
   }

   return get_reg_ptr(file, idx);
}

static void emit_pshufd( struct compilation *cp,
			 struct x86_reg dst,
			 struct x86_reg arg0,
			 GLubyte shuf )
{
   if (cp->have_sse2) {
      sse2_pshufd(&cp->func, dst, arg0, shuf);
      cp->func.fn = 0;
   }
   else {
      if (!eq(dst, arg0)) 
	 sse_movups(&cp->func, dst, arg0);

      sse_shufps(&cp->func, dst, dst, shuf);
   }
}
			 


/* Perform a reduced swizzle.  
 */
static GLboolean emit_RSW( struct compilation *cp, union instruction op ) 
{
   struct x86_reg arg0 = get_arg(cp, op.rsw.file0, op.rsw.idx0);
   struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.rsw.dst);
   GLuint swz = op.rsw.swz;
   GLuint neg = op.rsw.neg;

   emit_pshufd(cp, dst, arg0, swz);
   
   if (neg) {
      struct x86_reg negs = get_arg(cp, FILE_REG, REG_SWZ);
      struct x86_reg tmp = get_xmm_reg(cp);
      /* Load 1,-1,0,0
       * Use neg as arg to pshufd
       * Multiply
       */
      emit_pshufd(cp, tmp, negs, 
		  SHUF((neg & 1) ? 1 : 0,
		       (neg & 2) ? 1 : 0,
		       (neg & 4) ? 1 : 0,
		       (neg & 8) ? 1 : 0));
      sse_mulps(&cp->func, dst, tmp);
   }

   return GL_TRUE;
}

/* Used to implement write masking.  This and most of the other instructions
 * here would be easier to implement if there had been a translation
 * to a 2 argument format (dst/arg0, arg1) at the shader level before
 * attempting to translate to x86/sse code.
 */
/* Hmm.  I went back to MSK from SEL to make things easier -- was that just BS?
 */
static GLboolean emit_MSK( struct compilation *cp, union instruction op )
{
   struct x86_reg arg = get_arg(cp, op.msk.file, op.msk.idx);
   struct x86_reg dst0 = get_arg(cp, FILE_REG, op.msk.dst);
   struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.msk.dst);
   
   sse_movups(&cp->func, dst, dst0);

   switch (op.msk.mask) {
   case 0:
      return GL_TRUE;

   case WRITEMASK_X:
      if (arg.file == file_XMM) {
	 sse_movss(&cp->func, dst, arg);
      }
      else {
	 struct x86_reg tmp = get_xmm_reg(cp);
	 sse_movss(&cp->func, tmp, arg);
	 sse_movss(&cp->func, dst, tmp);
      }
      return GL_TRUE;

   case WRITEMASK_Y: {
      struct x86_reg tmp = get_xmm_reg(cp);
      emit_pshufd(cp, dst, dst, SHUF(Y, X, Z, W));
      emit_pshufd(cp, tmp, arg, SHUF(Y, X, Z, W));
      sse_movss(&cp->func, dst, tmp);
      emit_pshufd(cp, dst, dst, SHUF(Y, X, Z, W));
      return GL_TRUE;
   }

   case WRITEMASK_Z: {
      struct x86_reg tmp = get_xmm_reg(cp);
      emit_pshufd(cp, dst, dst, SHUF(Z, Y, X, W));
      emit_pshufd(cp, tmp, arg, SHUF(Z, Y, X, W));
      sse_movss(&cp->func, dst, tmp);
      emit_pshufd(cp, dst, dst, SHUF(Z, Y, X, W));
      return GL_TRUE;
   }

   case WRITEMASK_W: {
      struct x86_reg tmp = get_xmm_reg(cp);
      emit_pshufd(cp, dst, dst, SHUF(W, Y, Z, X));
      emit_pshufd(cp, tmp, arg, SHUF(W, Y, Z, X));
      sse_movss(&cp->func, dst, tmp);
      emit_pshufd(cp, dst, dst, SHUF(W, Y, Z, X));
      return GL_TRUE;
   }

   case WRITEMASK_XY:
      sse_shufps(&cp->func, dst, arg, SHUF(X, Y, Z, W));
      return GL_TRUE;

   case WRITEMASK_ZW: {
      struct x86_reg tmp = get_xmm_reg(cp);      
      sse_movups(&cp->func, tmp, dst);
      sse_movups(&cp->func, dst, arg);
      sse_shufps(&cp->func, dst, tmp, SHUF(X, Y, Z, W));
      return GL_TRUE;
   }

   case WRITEMASK_YZW: {
      struct x86_reg tmp = get_xmm_reg(cp);      
      sse_movss(&cp->func, tmp, dst);
      sse_movups(&cp->func, dst, arg);
      sse_movss(&cp->func, dst, tmp);
      return GL_TRUE;
   }

   case WRITEMASK_XYZW:
      sse_movups(&cp->func, dst, arg);
      return GL_TRUE;      

   default:
      FAIL;
   }

#if 0
   /* The catchall implementation:
    */

   /* make full width bitmask in tmp 
    * dst = ~tmp
    * tmp &= arg0
    * dst &= arg1
    * dst |= tmp
    */
   {
      struct x86_reg negs = get_arg(cp, FILE_REG, REG_NEGS);
      emit_pshufd(cp, tmp, negs, 
		  SHUF((op.msk.mask & 1) ? 2 : 0,
		       (op.msk.mask & 2) ? 2 : 0,
		       (op.msk.mask & 4) ? 2 : 0,
		       (op.msk.mask & 8) ? 2 : 0));
      sse_mulps(&cp->func, dst, tmp);
   }

   return GL_TRUE;
#endif
   FAIL;
}



static GLboolean emit_PRT( struct compilation *cp, union instruction op )
{
   FAIL;
}


/**
 * The traditional instructions.  All operate on internal registers
 * and ignore write masks and swizzling issues.
 */

static GLboolean emit_ABS( struct compilation *cp, union instruction op ) 
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst);
   struct x86_reg neg = get_reg_ptr(FILE_REG, REG_NEG);

   sse_movups(&cp->func, dst, arg0);
   sse_mulps(&cp->func, dst, neg);
   sse_maxps(&cp->func, dst, arg0);
   return GL_TRUE;
}

static GLboolean emit_ADD( struct compilation *cp, union instruction op )
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1);
   struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst);

   sse_movups(&cp->func, dst, arg0);
   sse_addps(&cp->func, dst, arg1);
   return GL_TRUE;
}


/* The dotproduct instructions don't really do that well in sse:
 */
static GLboolean emit_DP3( struct compilation *cp, union instruction op )
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1);
   struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst);
   struct x86_reg tmp = get_xmm_reg(cp); 

   sse_movups(&cp->func, dst, arg0);
   sse_mulps(&cp->func, dst, arg1);
   
   /* Now the hard bit: sum the first 3 values:
    */ 
   sse_movhlps(&cp->func, tmp, dst);
   sse_addss(&cp->func, dst, tmp); /* a*x+c*z, b*y, ?, ? */
   emit_pshufd(cp, tmp, dst, SHUF(Y,X,W,Z));
   sse_addss(&cp->func, dst, tmp);
   sse_shufps(&cp->func, dst, dst, SHUF(X, X, X, X));
   return GL_TRUE;
}



static GLboolean emit_DP4( struct compilation *cp, union instruction op )
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1);
   struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst);
   struct x86_reg tmp = get_xmm_reg(cp);      

   sse_movups(&cp->func, dst, arg0);
   sse_mulps(&cp->func, dst, arg1);
   
   /* Now the hard bit: sum the values:
    */ 
   sse_movhlps(&cp->func, tmp, dst);
   sse_addps(&cp->func, dst, tmp); /* a*x+c*z, b*y+d*w, a*x+c*z, b*y+d*w */
   emit_pshufd(cp, tmp, dst, SHUF(Y,X,W,Z));
   sse_addss(&cp->func, dst, tmp);
   sse_shufps(&cp->func, dst, dst, SHUF(X, X, X, X));
   return GL_TRUE;
}

static GLboolean emit_DPH( struct compilation *cp, union instruction op )
{
/*    struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0); */
/*    struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1); */
   struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst);

/*    dst[0] = (arg0[0] * arg1[0] +  */
/* 	     arg0[1] * arg1[1] +  */
/* 	     arg0[2] * arg1[2] +  */
/* 	     1.0     * arg1[3]); */
   
   sse_shufps(&cp->func, dst, dst, SHUF(X, X, X, X));
   FAIL;
}

static GLboolean emit_DST( struct compilation *cp, union instruction op )
{
/*    struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0); */
/*    struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1); */
/*    struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst); */

/*    dst[0] = 1.0     * 1.0F; */
/*    dst[1] = arg0[1] * arg1[1]; */
/*    dst[2] = arg0[2] * 1.0; */
/*    dst[3] = 1.0     * arg1[3]; */

   FAIL;
}


static GLboolean emit_EX2( struct compilation *cp, union instruction op ) 
{
/*    struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0); */
   struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst);

/*    dst[0] = (GLfloat)RoughApproxPow2(arg0[0]); */
   sse_shufps(&cp->func, dst, dst, SHUF(X, X, X, X));
   FAIL;
}

static GLboolean emit_EXP( struct compilation *cp, union instruction op )
{
/*    struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0); */
/*    struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst); */

/*    GLfloat tmp = arg0[0]; */
/*    GLfloat flr_tmp = FLOORF(tmp); */
/*    dst[0] = (GLfloat) (1 << (int)flr_tmp); */
/*    dst[1] = tmp - flr_tmp; */
/*    dst[2] = RoughApproxPow2(tmp); */
/*    dst[3] = 1.0F; */
   FAIL;
}

static GLboolean emit_FLR( struct compilation *cp, union instruction op ) 
{
/*    struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0); */
/*    struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst); */

/*    dst[0] = FLOORF(arg0[0]); */
/*    dst[1] = FLOORF(arg0[1]); */
/*    dst[2] = FLOORF(arg0[2]); */
/*    dst[3] = FLOORF(arg0[3]); */
   FAIL;
}

static GLboolean emit_FRC( struct compilation *cp, union instruction op ) 
{
/*    struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0); */
/*    struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst); */

/*    dst[0] = arg0[0] - FLOORF(arg0[0]); */
/*    dst[1] = arg0[1] - FLOORF(arg0[1]); */
/*    dst[2] = arg0[2] - FLOORF(arg0[2]); */
/*    dst[3] = arg0[3] - FLOORF(arg0[3]); */
   FAIL;
}

static GLboolean emit_LG2( struct compilation *cp, union instruction op ) 
{
/*    struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0); */
/*    struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst); */

/*    dst[0] = RoughApproxLog2(arg0[0]); */

/*    sse_shufps(&cp->func, dst, dst, SHUF(X, X, X, X)); */
   FAIL;
}



static GLboolean emit_LIT( struct compilation *cp, union instruction op )
{
/*    struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0); */
/*    struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst); */

/*    const GLfloat epsilon = 1.0F / 256.0F; */
/*    GLfloat tmp[4]; */

/*    tmp[0] = MAX2(arg0[0], 0.0F); */
/*    tmp[1] = MAX2(arg0[1], 0.0F); */
/*    tmp[3] = CLAMP(arg0[3], -(128.0F - epsilon), (128.0F - epsilon)); */

/*    dst[0] = 1.0; */
/*    dst[1] = tmp[0]; */
/*    dst[2] = (tmp[0] > 0.0) ? RoughApproxPower(tmp[1], tmp[3]) : 0.0F; */
/*    dst[3] = 1.0; */
   FAIL;
}


static GLboolean emit_LOG( struct compilation *cp, union instruction op )
{
/*    struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0); */
/*    struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst); */

/*    GLfloat tmp = FABSF(arg0[0]); */
/*    int exponent; */
/*    GLfloat mantissa = FREXPF(tmp, &exponent); */
/*    dst[0] = (GLfloat) (exponent - 1); */
/*    dst[1] = 2.0 * mantissa; // map [.5, 1) -> [1, 2)  */
/*    dst[2] = dst[0] + LOG2(dst[1]); */
/*    dst[3] = 1.0; */
   FAIL;
}

static GLboolean emit_MAX( struct compilation *cp, union instruction op )
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1);
   struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst);

   sse_movups(&cp->func, dst, arg0);
   sse_maxps(&cp->func, dst, arg1);
   return GL_TRUE;
}


static GLboolean emit_MIN( struct compilation *cp, union instruction op )
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1);
   struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst);

   sse_movups(&cp->func, dst, arg0);
   sse_minps(&cp->func, dst, arg1);
   return GL_TRUE;
}

static GLboolean emit_MOV( struct compilation *cp, union instruction op )
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst);

   sse_movups(&cp->func, dst, arg0);
   return GL_TRUE;
}

static GLboolean emit_MUL( struct compilation *cp, union instruction op )
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1);
   struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst);

   sse_movups(&cp->func, dst, arg0);
   sse_mulps(&cp->func, dst, arg1);
   return GL_TRUE;
}


static GLboolean emit_POW( struct compilation *cp, union instruction op ) 
{
/*    struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0); */
/*    struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1); */
   struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst);

/*    dst[0] = (GLfloat)RoughApproxPower(arg0[0], arg1[0]); */

   sse_shufps(&cp->func, dst, dst, SHUF(X, X, X, X));
   FAIL;
}

static GLboolean emit_REL( struct compilation *cp, union instruction op )
{
/*    GLuint idx = (op.alu.idx0 + (GLint)cp->File[0][REG_ADDR][0]) & (MAX_NV_VERTEX_PROGRAM_PARAMS-1); */
/*    GLuint idx = 0; */
/*    struct x86_reg arg0 = get_arg(cp, op.alu.file0, idx); */
/*    struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst); */

/*    dst[0] = arg0[0]; */
/*    dst[1] = arg0[1]; */
/*    dst[2] = arg0[2]; */
/*    dst[3] = arg0[3]; */

   FAIL;
}

static GLboolean emit_RCP( struct compilation *cp, union instruction op )
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst);

   if (cp->have_sse2) {
      sse2_rcpss(&cp->func, dst, arg0);
   }
   else {
      struct x86_reg ones = get_reg_ptr(FILE_REG, REG_ONES);
      sse_movss(&cp->func, dst, ones);
      sse_divss(&cp->func, dst, arg0);
   }

   sse_shufps(&cp->func, dst, dst, SHUF(X, X, X, X));
   return GL_TRUE;
}

static GLboolean emit_RSQ( struct compilation *cp, union instruction op )
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst);

   sse_rsqrtss(&cp->func, dst, arg0);
   sse_shufps(&cp->func, dst, dst, SHUF(X, X, X, X));
   return GL_TRUE;
}


static GLboolean emit_SGE( struct compilation *cp, union instruction op )
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1);
   struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst);
   struct x86_reg ones = get_reg_ptr(FILE_REG, REG_ONES);

   sse_movups(&cp->func, dst, arg0);
   sse_cmpps(&cp->func, dst, arg1, cc_NotLessThan);
   sse_andps(&cp->func, dst, ones);
   return GL_TRUE;
}


static GLboolean emit_SLT( struct compilation *cp, union instruction op )
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1);
   struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst);
   struct x86_reg ones = get_reg_ptr(FILE_REG, REG_ONES);
   
   sse_movups(&cp->func, dst, arg0);
   sse_cmpps(&cp->func, dst, arg1, cc_LessThan);
   sse_andps(&cp->func, dst, ones);
   return GL_TRUE;
}

static GLboolean emit_SUB( struct compilation *cp, union instruction op ) 
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1);
   struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst);

   sse_movups(&cp->func, dst, arg0);
   sse_subps(&cp->func, dst, arg1);
   return GL_TRUE;
}


static GLboolean emit_XPD( struct compilation *cp, union instruction op ) 
{
   struct x86_reg arg0 = get_arg(cp, op.alu.file0, op.alu.idx0);
   struct x86_reg arg1 = get_arg(cp, op.alu.file1, op.alu.idx1);
   struct x86_reg dst = get_dst_reg(cp, FILE_REG, op.alu.dst);
   struct x86_reg tmp0 = get_xmm_reg(cp);
   struct x86_reg tmp1 = get_xmm_reg(cp);

   /* Could avoid tmp0, tmp1 if we overwrote arg0, arg1.  Need a way
    * to invalidate registers.  This will come with better analysis
    * (liveness analysis) of the incoming program.
    */
   emit_pshufd(cp, dst, arg0, SHUF(Y, Z, X, W));
   emit_pshufd(cp, tmp1, arg1, SHUF(Z, X, Y, W));
   sse_mulps(&cp->func, dst, tmp1);
   emit_pshufd(cp, tmp0, arg0, SHUF(Z, X, Y, W));
   emit_pshufd(cp, tmp1, arg1, SHUF(Y, Z, X, W));
   sse_mulps(&cp->func, tmp0, tmp1);
   sse_subps(&cp->func, dst, tmp0);

/*    dst[0] = arg0[1] * arg1[2] - arg0[2] * arg1[1]; */
/*    dst[1] = arg0[2] * arg1[0] - arg0[0] * arg1[2]; */
/*    dst[2] = arg0[0] * arg1[1] - arg0[1] * arg1[0]; */
/*    dst[3] is undef */

   return GL_TRUE;
}

static GLboolean emit_NOP( struct compilation *cp, union instruction op ) 
{
   return GL_TRUE;
}


static GLboolean (* const emit_func[])(struct compilation *, union instruction) = 
{
   emit_ABS,
   emit_ADD,
   emit_NOP,
   emit_DP3,
   emit_DP4,
   emit_DPH,
   emit_DST,
   emit_NOP,
   emit_EX2,
   emit_EXP,
   emit_FLR,
   emit_FRC,
   emit_LG2,
   emit_LIT,
   emit_LOG,
   emit_NOP,
   emit_MAX,
   emit_MIN,
   emit_MOV,
   emit_MUL,
   emit_POW,
   emit_PRT,
   emit_NOP,
   emit_RCP,
   emit_RSQ,
   emit_SGE,
   emit_SLT,
   emit_SUB,
   emit_RSW,
   emit_XPD,
   emit_RSW,
   emit_MSK,
   emit_REL,
};

static GLint get_offset( const void *a, const void *b )
{
   return (const char *)b - (const char *)a;
}


static GLboolean build_vertex_program( struct compilation *cp )
{
   GLuint j;

   struct x86_reg regEAX = x86_make_reg(file_REG32, reg_AX);
   struct x86_reg parmECX = x86_make_reg(file_REG32, reg_CX);

   x86_mov(&cp->func, regEAX, x86_fn_arg(&cp->func, 1));
   x86_mov(&cp->func, parmECX, regEAX);
   
   x86_mov(&cp->func, regEAX, x86_make_disp(regEAX, get_offset(cp->m, cp->m->File + FILE_REG)));
   x86_mov(&cp->func, parmECX, x86_make_disp(parmECX, get_offset(cp->m, cp->m->File + FILE_STATE_PARAM)));

   for (j = 0; j < cp->m->nr_instructions; j++) {
      union instruction inst = cp->m->instructions[j];	 
      cp->insn_counter = j+1;	/* avoid zero */
      
      _mesa_printf("%p: ", cp->func.csr); 
      _tnl_disassem_vba_insn( inst );
      cp->func.fn = NULL;

      if (!emit_func[inst.alu.opcode]( cp, inst )) {
	 return GL_FALSE;
      }
   }

   /* TODO: only for outputs:
    */
   for (j = 0; j < 8; j++) {
      if (cp->xmm[j].dirty) 
	 spill(cp, j);
   }
      

   /* Exit mmx state?
    */
   if (cp->func.need_emms)
      mmx_emms(&cp->func);

   x86_ret(&cp->func);

   return GL_TRUE;
}

/**
 * Execute the given vertex program.  
 * 
 * TODO: Integrate the t_vertex.c code here, to build machine vertices
 * directly at this point.
 *
 * TODO: Eliminate the VB struct entirely and just use
 * struct arb_vertex_machine.
 */
GLboolean
_tnl_sse_codegen_vertex_program(struct arb_vp_machine *m)
{
   struct compilation cp;
   
   memset(&cp, 0, sizeof(cp));
   cp.m = m;
   cp.have_sse2 = 1;

   if (m->func) {
      free((void *)m->func);
      m->func = NULL;
   }

   x86_init_func(&cp.func);

   if (!build_vertex_program(&cp)) {
      x86_release_func( &cp.func );
      return GL_FALSE;
   }

   m->func = (void (*)(struct arb_vp_machine *))x86_get_func( &cp.func );
   return GL_TRUE;
}



#else

GLboolean
_tnl_sse_codegen_vertex_program( GLcontext *ctx )
{
   /* Dummy version for when USE_SSE_ASM not defined */
   return GL_FALSE;
}

#endif
