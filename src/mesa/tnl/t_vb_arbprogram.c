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
 * \file t_arb_program.c
 * Compile vertex programs to an intermediate representation.
 * Execute vertex programs over a buffer of vertices.
 * \author Keith Whitwell, Brian Paul
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
#include "t_pipeline.h"
#include "t_vp_build.h"

/* Define to see the compiled program on stderr:
 */
#define DISASSEM 0


/* New, internal instructions:
 */
#define IN1        (VP_OPCODE_XPD+1)
#define IN2        (IN1+1)	/* intput-to-reg MOV */
#define IN3        (IN1+2)
#define IN4        (IN1+3)
#define OUT        (IN1+4)	/* reg-to-output MOV */
#define OUM        (IN1+5)	/* reg-to-output MOV with mask */
#define RSW        (IN1+6)
#define MSK        (IN1+7)	/* reg-to-reg MOV with mask */
#define PAR        (IN1+8)      /* parameter-to-reg MOV */
#define PRL        (IN1+9)      /* parameter-to-reg MOV */


/* Layout of register file:

  0 -- Scratch (Arg0)
  1 -- Scratch (Arg1)
  2 -- Scratch (Arg2)
  3 -- Scratch (Result)
  4 -- Program Temporary 0
  ..
  31 -- Program Temporary 27
  32 -- State/Input/Const shadow 0
  ..
  63 -- State/Input/Const shadow 31

*/



#define REG_ARG0  0
#define REG_ARG1  1
#define REG_ARG2  2
#define REG_RES   3
#define REG_TMP0  4
#define REG_TMP_MAX 32
#define REG_TMP_NR (REG_TMP_MAX-REG_TMP0)
#define REG_PAR0  32
#define REG_PAR_MAX 64
#define REG_PAR_NR (REG_PAR_MAX-REG_PAR0)

#define REG_MAX 64
#define REG_SWZDST_MAX 16

/* ARB_vp instructions are broken down into one or more of the
 * following micro-instructions, each representable in a 32 bit packed
 * structure.
 */


union instruction {
   struct {
      GLuint opcode:6;
      GLuint dst:5;
      GLuint arg0:6;
      GLuint arg1:6;
      GLuint elt:2;		/* x,y,z or w */
      GLuint pad:7;
   } scl;


   struct {
      GLuint opcode:6;
      GLuint dst:5;
      GLuint arg0:6;
      GLuint arg1:6;
      GLuint arg2:6;
      GLuint pad:3;
   } vec;

   struct {
      GLuint opcode:6;
      GLuint dst:4;		/* NOTE!  REG 0..16 only! */
      GLuint arg0:6;
      GLuint neg:4;		
      GLuint swz:12;		
   } swz;

   struct {
      GLuint opcode:6;
      GLuint dst:6;
      GLuint arg0:6;
      GLuint neg:1;		/* 1 bit only */
      GLuint swz:8;		/* xyzw only */
      GLuint pad:5;
   } rsw;

   struct {
      GLuint opcode:6;
      GLuint reg:6;
      GLuint file:5;
      GLuint idx:8;		/* plenty? */
      GLuint rel:1;
      GLuint pad:6;
   } inr;


   struct {
      GLuint opcode:6;
      GLuint reg:6;
      GLuint file:5;
      GLuint idx:8;		/* plenty? */
      GLuint mask:4;
      GLuint pad:3;
   } out;

   struct {
      GLuint opcode:6;
      GLuint dst:5;
      GLuint arg0:6;
      GLuint mask:4;
      GLuint pad:11;
   } msk;

   GLuint dword;
};



struct compilation {
   struct {
      GLuint file:5;
      GLuint idx:8; 
   } reg[REG_PAR_NR];

   GLuint par_active;
   GLuint par_protected;
   GLuint tmp_active;
   
   union instruction *csr;

   struct vertex_buffer *VB;	/* for input sizes! */
};

/*--------------------------------------------------------------------------- */

/*!
 * Private storage for the vertex program pipeline stage.
 */
struct arb_vp_machine {
   GLfloat reg[REG_MAX][4];	/* Program temporaries, shadowed parameters and inputs,
				   plus some internal values */

   GLfloat (*File[8])[4];	/* Src/Dest for PAR/PRL instructions. */
   GLint AddressReg;

   union instruction store[1024];
   union instruction *instructions;
   GLint nr_instructions;

   GLvector4f attribs[VERT_RESULT_MAX]; /**< result vectors. */
   GLvector4f ndcCoords;              /**< normalized device coords */
   GLubyte *clipmask;                 /**< clip flags */
   GLubyte ormask, andmask;           /**< for clipping */

   GLuint vtx_nr;		/**< loop counter */

   struct vertex_buffer *VB;
   GLcontext *ctx;
};


/*--------------------------------------------------------------------------- */

struct opcode_info {
   GLuint type;
   GLuint nr_args;
   const char *string;
   void (*func)( struct arb_vp_machine *, union instruction );
   void (*print)( union instruction , const struct opcode_info * );
};


#define ARB_VP_MACHINE(stage) ((struct arb_vp_machine *)(stage->privatePtr))



/**
 * Set x to positive or negative infinity.
 *
 * XXX: FIXME - type punning.
 */
#if defined(USE_IEEE) || defined(_WIN32)
#define SET_POS_INFINITY(x)  ( *((GLuint *) (void *)&x) = 0x7F800000 )
#define SET_NEG_INFINITY(x)  ( *((GLuint *) (void *)&x) = 0xFF800000 )
#elif defined(VMS)
#define SET_POS_INFINITY(x)  x = __MAXFLOAT
#define SET_NEG_INFINITY(x)  x = -__MAXFLOAT
#define IS_INF_OR_NAN(t)   ((t) == __MAXFLOAT)
#else
#define SET_POS_INFINITY(x)  x = (GLfloat) HUGE_VAL
#define SET_NEG_INFINITY(x)  x = (GLfloat) -HUGE_VAL
#endif

#define FREXPF(a,b) frexpf(a,b)

#define PUFF(x) ((x)[1] = (x)[2] = (x)[3] = (x)[0])

/* FIXME: more type punning (despite use of fi_type...)
 */
#define SET_FLOAT_BITS(x, bits) ((fi_type *) (void *) &(x))->i = bits


static GLfloat RoughApproxLog2(GLfloat t)
{
   return LOG2(t);
}

static GLfloat RoughApproxPow2(GLfloat t)
{   
   GLfloat q;
#ifdef USE_IEEE
   GLint ii = (GLint) t;
   ii = (ii < 23) + 0x3f800000;
   SET_FLOAT_BITS(q, ii);
   q = *((GLfloat *) (void *)&ii);
#else
   q = (GLfloat) pow(2.0, floor_t0);
#endif
   return q;
}

static GLfloat RoughApproxPower(GLfloat x, GLfloat y)
{
#if 0
   return (GLfloat) exp(y * log(x));
#else
   return (GLfloat) _mesa_pow(x, y);
#endif
}


static const GLfloat ZeroVec[4] = { 0.0F, 0.0F, 0.0F, 0.0F };




/**
 * This is probably the least-optimal part of the process, have to
 * multiply out the stride to access each incoming input value.
 */
static GLfloat *get_input( struct arb_vp_machine *m, GLuint index )
{
   return VEC_ELT(m->VB->AttribPtr[index], GLfloat, m->vtx_nr);
}


/**
 * Fetch a 4-element float vector from the given source register.
 * Deal with the possibility that not all elements are present.
 */
static void do_IN1( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.inr.reg];
   const GLfloat *src = get_input(m, op.inr.idx);

   result[0] = src[0];
   result[1] = 0;
   result[2] = 0;
   result[3] = 1;
}

static void do_IN2( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.inr.reg];
   const GLfloat *src = get_input(m, op.inr.idx);
   
   result[0] = src[0];
   result[1] = src[1];
   result[2] = 0;
   result[3] = 1;
}

static void do_IN3( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.inr.reg];
   const GLfloat *src = get_input(m, op.inr.idx);

   result[0] = src[0];
   result[1] = src[1];
   result[2] = src[2];
   result[3] = 1;
}

static void do_IN4( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.inr.reg];
   const GLfloat *src = get_input(m, op.inr.idx);
   
   result[0] = src[0];
   result[1] = src[1];
   result[2] = src[2];
   result[3] = src[3];
}

/**
 * Perform a reduced swizzle:
 */
static void do_RSW( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->reg[op.rsw.dst];
   const GLfloat *arg0 = m->reg[op.rsw.arg0];
   GLuint swz = op.rsw.swz;
   GLuint neg = op.rsw.neg;
   GLuint i;

   if (neg) 
      for (i = 0; i < 4; i++, swz >>= 2) 
	 result[i] = -arg0[swz & 0x3];
   else
      for (i = 0; i < 4; i++, swz >>= 2) 
	 result[i] = arg0[swz & 0x3];
}



/**
 * Store 4 floats into an external address.
 */
static void do_OUM( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *dst = m->attribs[op.out.idx].data[m->vtx_nr];
   const GLfloat *value = m->reg[op.out.reg];

   if (op.out.mask & 0x1) dst[0] = value[0];
   if (op.out.mask & 0x2) dst[1] = value[1];
   if (op.out.mask & 0x4) dst[2] = value[2];
   if (op.out.mask & 0x8) dst[3] = value[3];
}

static void do_OUT( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *dst = m->attribs[op.out.idx].data[m->vtx_nr];
   const GLfloat *value = m->reg[op.out.reg];

   dst[0] = value[0];
   dst[1] = value[1];
   dst[2] = value[2];
   dst[3] = value[3];
}

/* Register-to-register MOV with writemask.
 */
static void do_MSK( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *dst = m->reg[op.msk.dst];
   const GLfloat *arg0 = m->reg[op.msk.arg0];
 
   if (op.msk.mask & 0x1) dst[0] = arg0[0];
   if (op.msk.mask & 0x2) dst[1] = arg0[1];
   if (op.msk.mask & 0x4) dst[2] = arg0[2];
   if (op.msk.mask & 0x8) dst[3] = arg0[3];
}


/* Retreive parameters and other constant values:
 */
static void do_PAR( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.inr.reg];
   const GLfloat *src = m->File[op.inr.file][op.inr.idx];

   result[0] = src[0];
   result[1] = src[1];
   result[2] = src[2];
   result[3] = src[3];
}


#define RELADDR_MASK (MAX_NV_VERTEX_PROGRAM_PARAMS-1)

static void do_PRL( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.inr.reg];
   GLuint index = (op.inr.idx + m->AddressReg) & RELADDR_MASK;
   const GLfloat *src = m->File[op.inr.file][index];

   result[0] = src[0];
   result[1] = src[1];
   result[2] = src[2];
   result[3] = src[3];
}

static void do_PRT( struct arb_vp_machine *m, union instruction op )
{
   const GLfloat *arg0 = m->reg[op.vec.arg0];
   
   _mesa_printf("%d: %f %f %f %f\n", m->vtx_nr, 
		arg0[0], arg0[1], arg0[2], arg0[3]);
}


/**
 * The traditional ALU and texturing instructions.  All operate on
 * internal registers and ignore write masks and swizzling issues.
 */

static void do_ABS( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->reg[op.vec.dst];
   const GLfloat *arg0 = m->reg[op.vec.arg0];

   result[0] = (arg0[0] < 0.0) ? -arg0[0] : arg0[0];
   result[1] = (arg0[1] < 0.0) ? -arg0[1] : arg0[1];
   result[2] = (arg0[2] < 0.0) ? -arg0[2] : arg0[2];
   result[3] = (arg0[3] < 0.0) ? -arg0[3] : arg0[3];
}

static void do_ADD( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.vec.dst];
   const GLfloat *arg0 = m->reg[op.vec.arg0];
   const GLfloat *arg1 = m->reg[op.vec.arg1];

   result[0] = arg0[0] + arg1[0];
   result[1] = arg0[1] + arg1[1];
   result[2] = arg0[2] + arg1[2];
   result[3] = arg0[3] + arg1[3];
}


static void do_ARL( struct arb_vp_machine *m, union instruction op )
{
   const GLfloat *arg0 = m->reg[op.out.reg];
   m->AddressReg = (GLint) floor(arg0[0]);
}


static void do_DP3( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.scl.dst];
   const GLfloat *arg0 = m->reg[op.scl.arg0];
   const GLfloat *arg1 = m->reg[op.scl.arg1];

   result[0] = (arg0[0] * arg1[0] + 
		arg0[1] * arg1[1] + 
		arg0[2] * arg1[2]);

   PUFF(result);
}

#if 0
static void do_MAT4( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.scl.dst];
   const GLfloat *arg0 = m->reg[op.scl.arg0];
   const GLfloat *mat[] = m->reg[op.scl.arg1];

   result[0] = (arg0[0] * mat0[0] + arg0[1] * mat0[1] + arg0[2] * mat0[2] + arg0[3] * mat0[3]);
   result[1] = (arg0[0] * mat1[0] + arg0[1] * mat1[1] + arg0[2] * mat1[2] + arg0[3] * mat1[3]);
   result[2] = (arg0[0] * mat2[0] + arg0[1] * mat2[1] + arg0[2] * mat2[2] + arg0[3] * mat2[3]);
   result[3] = (arg0[0] * mat3[0] + arg0[1] * mat3[1] + arg0[2] * mat3[2] + arg0[3] * mat3[3]);
}
#endif


static void do_DP4( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.scl.dst];
   const GLfloat *arg0 = m->reg[op.scl.arg0];
   const GLfloat *arg1 = m->reg[op.scl.arg1];

   result[0] = (arg0[0] * arg1[0] + 
		arg0[1] * arg1[1] + 
		arg0[2] * arg1[2] + 
		arg0[3] * arg1[3]);

   PUFF(result);
}

static void do_DPH( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.scl.dst];
   const GLfloat *arg0 = m->reg[op.scl.arg0];
   const GLfloat *arg1 = m->reg[op.scl.arg1];

   result[0] = (arg0[0] * arg1[0] + 
		arg0[1] * arg1[1] + 
		arg0[2] * arg1[2] + 
		1.0     * arg1[3]);
   
   PUFF(result);
}

static void do_DST( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.vec.dst];
   const GLfloat *arg0 = m->reg[op.vec.arg0];
   const GLfloat *arg1 = m->reg[op.vec.arg1];

   result[0] = 1.0F;
   result[1] = arg0[1] * arg1[1];
   result[2] = arg0[2];
   result[3] = arg1[3];
}


static void do_EX2( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->reg[op.scl.dst];
   const GLfloat *arg0 = m->reg[op.scl.arg0];

   result[0] = (GLfloat)RoughApproxPow2(arg0[0]);
   PUFF(result);
}

static void do_EXP( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.vec.dst];
   const GLfloat *arg0 = m->reg[op.vec.arg0];
   GLfloat tmp = arg0[0];
   GLfloat flr_tmp = FLOORF(tmp);

   /* KW: nvvertexec has an optimized version of this which is pretty
    * hard to understand/validate, but avoids the RoughApproxPow2.
    */
   result[0] = (GLfloat) (1 << (int)flr_tmp);
   result[1] = tmp - flr_tmp;
   result[2] = RoughApproxPow2(tmp);
   result[3] = 1.0F;
}

static void do_FLR( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->reg[op.vec.dst];
   const GLfloat *arg0 = m->reg[op.vec.arg0];

   result[0] = FLOORF(arg0[0]);
   result[1] = FLOORF(arg0[1]);
   result[2] = FLOORF(arg0[2]);
   result[3] = FLOORF(arg0[3]);
}

static void do_FRC( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->reg[op.vec.dst];
   const GLfloat *arg0 = m->reg[op.vec.arg0];

   result[0] = arg0[0] - FLOORF(arg0[0]);
   result[1] = arg0[1] - FLOORF(arg0[1]);
   result[2] = arg0[2] - FLOORF(arg0[2]);
   result[3] = arg0[3] - FLOORF(arg0[3]);
}

static void do_LG2( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->reg[op.scl.dst];
   const GLfloat *arg0 = m->reg[op.scl.arg0];

   result[0] = RoughApproxLog2(arg0[0]);
   PUFF(result);
}



static void do_LIT( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.vec.dst];
   const GLfloat *arg0 = m->reg[op.vec.arg0];

   const GLfloat epsilon = 1.0F / 256.0F; /* per NV spec */
   GLfloat tmp[4];

   tmp[0] = MAX2(arg0[0], 0.0F);
   tmp[1] = MAX2(arg0[1], 0.0F);
   tmp[3] = CLAMP(arg0[3], -(128.0F - epsilon), (128.0F - epsilon));

   result[0] = 1.0;
   result[1] = tmp[0];
   result[2] = (tmp[0] > 0.0) ? RoughApproxPower(tmp[1], tmp[3]) : 0.0F;
   result[3] = 1.0;
}


static void do_LOG( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.vec.dst];
   const GLfloat *arg0 = m->reg[op.vec.arg0];
   GLfloat tmp = FABSF(arg0[0]);
   int exponent;
   GLfloat mantissa = FREXPF(tmp, &exponent);

   result[0] = (GLfloat) (exponent - 1);
   result[1] = 2.0 * mantissa; /* map [.5, 1) -> [1, 2) */
   result[2] = result[0] + LOG2(result[1]);
   result[3] = 1.0;
}


static void do_MAD( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.vec.dst];
   const GLfloat *arg0 = m->reg[op.vec.arg0];
   const GLfloat *arg1 = m->reg[op.vec.arg1];
   const GLfloat *arg2 = m->reg[op.vec.arg2];

   result[0] = arg0[0] * arg1[0] + arg2[0];
   result[1] = arg0[1] * arg1[1] + arg2[1];
   result[2] = arg0[2] * arg1[2] + arg2[2];
   result[3] = arg0[3] * arg1[3] + arg2[3];
}

static void do_MAX( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.vec.dst];
   const GLfloat *arg0 = m->reg[op.vec.arg0];
   const GLfloat *arg1 = m->reg[op.vec.arg1];

   result[0] = (arg0[0] > arg1[0]) ? arg0[0] : arg1[0];
   result[1] = (arg0[1] > arg1[1]) ? arg0[1] : arg1[1];
   result[2] = (arg0[2] > arg1[2]) ? arg0[2] : arg1[2];
   result[3] = (arg0[3] > arg1[3]) ? arg0[3] : arg1[3];
}


static void do_MIN( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.vec.dst];
   const GLfloat *arg0 = m->reg[op.vec.arg0];
   const GLfloat *arg1 = m->reg[op.vec.arg1];

   result[0] = (arg0[0] < arg1[0]) ? arg0[0] : arg1[0];
   result[1] = (arg0[1] < arg1[1]) ? arg0[1] : arg1[1];
   result[2] = (arg0[2] < arg1[2]) ? arg0[2] : arg1[2];
   result[3] = (arg0[3] < arg1[3]) ? arg0[3] : arg1[3];
}

static void do_MOV( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.vec.dst];
   const GLfloat *arg0 = m->reg[op.vec.arg0];

   result[0] = arg0[0];
   result[1] = arg0[1];
   result[2] = arg0[2];
   result[3] = arg0[3];
}

static void do_MUL( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.vec.dst];
   const GLfloat *arg0 = m->reg[op.vec.arg0];
   const GLfloat *arg1 = m->reg[op.vec.arg1];

   result[0] = arg0[0] * arg1[0];
   result[1] = arg0[1] * arg1[1];
   result[2] = arg0[2] * arg1[2];
   result[3] = arg0[3] * arg1[3];
}


static void do_POW( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->reg[op.scl.dst];
   const GLfloat *arg0 = m->reg[op.scl.arg0];
   const GLfloat *arg1 = m->reg[op.scl.arg1];

   result[0] = (GLfloat)RoughApproxPower(arg0[0], arg1[0]);
   PUFF(result);
}

static void do_RCP( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.scl.dst];
   const GLfloat *arg0 = m->reg[op.scl.arg0];

   result[0] = 1.0F / arg0[0];  
   PUFF(result);
}

static void do_RSQ( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.scl.dst];
   const GLfloat *arg0 = m->reg[op.scl.arg0];

   result[0] = INV_SQRTF(FABSF(arg0[0]));
   PUFF(result);
}


static void do_SGE( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.vec.dst];
   const GLfloat *arg0 = m->reg[op.vec.arg0];
   const GLfloat *arg1 = m->reg[op.vec.arg1];

   result[0] = (arg0[0] >= arg1[0]) ? 1.0F : 0.0F;
   result[1] = (arg0[1] >= arg1[1]) ? 1.0F : 0.0F;
   result[2] = (arg0[2] >= arg1[2]) ? 1.0F : 0.0F;
   result[3] = (arg0[3] >= arg1[3]) ? 1.0F : 0.0F;
}


static void do_SLT( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.vec.dst];
   const GLfloat *arg0 = m->reg[op.vec.arg0];
   const GLfloat *arg1 = m->reg[op.vec.arg1];

   result[0] = (arg0[0] < arg1[0]) ? 1.0F : 0.0F;
   result[1] = (arg0[1] < arg1[1]) ? 1.0F : 0.0F;
   result[2] = (arg0[2] < arg1[2]) ? 1.0F : 0.0F;
   result[3] = (arg0[3] < arg1[3]) ? 1.0F : 0.0F;
}

static void do_SWZ( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->reg[op.swz.dst];
   const GLfloat *arg0 = m->reg[op.swz.arg0];
   GLuint swz = op.swz.swz;
   GLuint neg = op.swz.neg;
   GLuint i;

   for (i = 0; i < 4; i++, swz >>= 3, neg >>= 1) {
      switch (swz & 0x7) {
      case SWIZZLE_ZERO: result[i] = 0.0; break;
      case SWIZZLE_ONE:  result[i] = 1.0; break;
      default:           result[i] = arg0[swz & 0x7]; break;
      }
      if (neg & 0x1)     result[i] = -result[i];
   }
}

static void do_SUB( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->reg[op.vec.dst];
   const GLfloat *arg0 = m->reg[op.vec.arg0];
   const GLfloat *arg1 = m->reg[op.vec.arg1];

   result[0] = arg0[0] - arg1[0];
   result[1] = arg0[1] - arg1[1];
   result[2] = arg0[2] - arg1[2];
   result[3] = arg0[3] - arg1[3];
}


static void do_XPD( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->reg[op.vec.dst];
   const GLfloat *arg0 = m->reg[op.vec.arg0];
   const GLfloat *arg1 = m->reg[op.vec.arg1];

   result[0] = arg0[1] * arg1[2] - arg0[2] * arg1[1];
   result[1] = arg0[2] * arg1[0] - arg0[0] * arg1[2];
   result[2] = arg0[0] * arg1[1] - arg0[1] * arg1[0];
}

static void do_NOP( struct arb_vp_machine *m, union instruction op ) 
{
}

/* Some useful debugging functions:
 */
static void print_reg( GLuint reg )
{
   if (reg == REG_RES) 
      _mesa_printf("RES");
   else if (reg >= REG_ARG0 && reg <= REG_ARG2)
      _mesa_printf("ARG%d", reg - REG_ARG0);
   else if (reg >= REG_TMP0 && reg < REG_TMP_MAX)
      _mesa_printf("TMP%d", reg - REG_TMP0);
   else if (reg >= REG_PAR0 && reg < REG_PAR_MAX)
      _mesa_printf("PAR%d", reg - REG_PAR0);
   else
      _mesa_printf("???");     
}

static void print_mask( GLuint mask )
{
   _mesa_printf(".");
   if (mask&0x1) _mesa_printf("x");
   if (mask&0x2) _mesa_printf("y");
   if (mask&0x4) _mesa_printf("z");
   if (mask&0x8) _mesa_printf("w");
}

static void print_extern( GLuint file, GLuint idx )
{
   static const char *reg_file[] = {
      "TEMPORARY",
      "INPUT",
      "OUTPUT",
      "LOCAL_PARAM",
      "ENV_PARAM",
      "NAMED_PARAM",
      "STATE_VAR",
      "WRITE_ONLY",
      "ADDRESS"
   };

   _mesa_printf("%s:%d", reg_file[file], idx);
}



static void print_SWZ( union instruction op, const struct opcode_info *info )
{
   GLuint swz = op.swz.swz;
   GLuint neg = op.swz.neg;
   GLuint i;

   _mesa_printf("%s ", info->string);
   print_reg(op.swz.dst);
   _mesa_printf(", ");
   print_reg(op.swz.arg0);
   _mesa_printf(".");
   for (i = 0; i < 4; i++, swz >>= 3, neg >>= 1) {
      const char *cswz = "xyzw01??";
      if (neg & 0x1)   
	 _mesa_printf("-");
      _mesa_printf("%c", cswz[swz&0x7]);
   }
   _mesa_printf("\n");
}

static void print_RSW( union instruction op, const struct opcode_info *info )
{
   GLuint swz = op.rsw.swz;
   GLuint neg = op.rsw.neg;
   GLuint i;

   _mesa_printf("%s ", info->string);
   print_reg(op.rsw.dst);
   _mesa_printf(", ");
   print_reg(op.rsw.arg0);
   _mesa_printf(".");
   for (i = 0; i < 4; i++, swz >>= 2) {
      const char *cswz = "xyzw";
      if (neg)   
	 _mesa_printf("-");
      _mesa_printf("%c", cswz[swz&0x3]);
   }
   _mesa_printf("\n");
}


static void print_SCL( union instruction op, const struct opcode_info *info )
{
   _mesa_printf("%s ", info->string);
   print_reg(op.scl.dst);
   _mesa_printf(", ");
   print_reg(op.scl.arg0);
   if (info->nr_args > 1) {
      _mesa_printf(", ");
      print_reg(op.scl.arg1);
   }
   _mesa_printf("\n");
}


static void print_VEC( union instruction op, const struct opcode_info *info )
{
   _mesa_printf("%s ", info->string);
   print_reg(op.vec.dst);
   _mesa_printf(", ");
   print_reg(op.vec.arg0);
   if (info->nr_args > 1) {
      _mesa_printf(", ");
      print_reg(op.vec.arg1);
   }
   if (info->nr_args > 2) {
      _mesa_printf(", ");
      print_reg(op.vec.arg2);
   }
   _mesa_printf("\n");
}

static void print_MSK( union instruction op, const struct opcode_info *info )
{
   _mesa_printf("%s ", info->string);
   print_reg(op.msk.dst);
   print_mask(op.msk.mask);
   _mesa_printf(", ");
   print_reg(op.msk.arg0);
   _mesa_printf("\n");
}

static void print_IN( union instruction op, const struct opcode_info *info )
{
   _mesa_printf("%s ", info->string);
   print_reg(op.inr.reg);
   _mesa_printf(", ");
   print_extern(op.inr.file, op.inr.idx);
   _mesa_printf("\n");
}

static void print_OUT( union instruction op, const struct opcode_info *info )
{
   _mesa_printf("%s ", info->string);
   print_extern(op.out.file, op.out.idx);
   if (op.out.opcode == OUM)
      print_mask(op.out.mask);
   _mesa_printf(", ");
   print_reg(op.out.reg);
   _mesa_printf("\n");
}

static void print_NOP( union instruction op, const struct opcode_info *info )
{
}

#define NOP 0
#define VEC 1
#define SCL 2
#define SWZ 3

static const struct opcode_info opcode_info[] = 
{
   { VEC, 1, "ABS", do_ABS, print_VEC },
   { VEC, 2, "ADD", do_ADD, print_VEC },
   { OUT, 1, "ARL", do_ARL, print_OUT },
   { SCL, 2, "DP3", do_DP3, print_SCL },
   { SCL, 2, "DP4", do_DP4, print_SCL },
   { SCL, 2, "DPH", do_DPH, print_SCL },
   { VEC, 2, "DST", do_DST, print_VEC },
   { NOP, 0, "END", do_NOP, print_NOP },
   { SCL, 1, "EX2", do_EX2, print_VEC },
   { VEC, 1, "EXP", do_EXP, print_VEC },
   { VEC, 1, "FLR", do_FLR, print_VEC },
   { VEC, 1, "FRC", do_FRC, print_VEC },
   { SCL, 1, "LG2", do_LG2, print_VEC },
   { VEC, 1, "LIT", do_LIT, print_VEC },
   { VEC, 1, "LOG", do_LOG, print_VEC },
   { VEC, 3, "MAD", do_MAD, print_VEC },
   { VEC, 2, "MAX", do_MAX, print_VEC },
   { VEC, 2, "MIN", do_MIN, print_VEC },
   { VEC, 1, "MOV", do_MOV, print_VEC },
   { VEC, 2, "MUL", do_MUL, print_VEC },
   { SCL, 2, "POW", do_POW, print_VEC },
   { VEC, 1, "PRT", do_PRT, print_VEC }, /* PRINT */
   { NOP, 1, "RCC", do_NOP, print_NOP },
   { SCL, 1, "RCP", do_RCP, print_VEC },
   { SCL, 1, "RSQ", do_RSQ, print_VEC },
   { VEC, 2, "SGE", do_SGE, print_VEC },
   { VEC, 2, "SLT", do_SLT, print_VEC },
   { VEC, 2, "SUB", do_SUB, print_VEC },
   { SWZ, 1, "SWZ", do_SWZ, print_SWZ },
   { VEC, 2, "XPD", do_XPD, print_VEC },
   { IN4, 1, "IN1", do_IN1, print_IN }, /* Internals */
   { IN4, 1, "IN2", do_IN2, print_IN },
   { IN4, 1, "IN3", do_IN3, print_IN },
   { IN4, 1, "IN4", do_IN4, print_IN },
   { OUT, 1, "OUT", do_OUT, print_OUT },
   { OUT, 1, "OUM", do_OUM, print_OUT },
   { SWZ, 1, "RSW", do_RSW, print_RSW },
   { MSK, 1, "MSK", do_MSK, print_MSK },
   { IN4, 1, "PAR", do_PAR, print_IN },
   { IN4, 1, "PRL", do_PRL, print_IN },
};


static GLuint cvp_load_reg( struct compilation *cp,
			    GLuint file,
			    GLuint index,
			    GLuint rel )
{
   GLuint i, op;

   if (file == PROGRAM_TEMPORARY)
      return index + REG_TMP0;

   /* Don't try to cache relatively addressed values yet:
    */
   if (!rel) {
      for (i = 0; i < REG_PAR_NR; i++) {
	 if ((cp->par_active & (1<<i)) &&
	     cp->reg[i].file == file &&
	     cp->reg[i].idx == index) {
	    cp->par_protected |= (1<<i);
	    return i + REG_PAR0;
	 }
      }
   }

   /* Not already loaded, so identify a slot and load it.  
    * TODO: preload these values once only!
    * TODO: better eviction strategy!
    */
   if (cp->par_active == ~0) {
      assert(cp->par_protected != ~0);
      cp->par_active = cp->par_protected;
   }

   i = ffs(~cp->par_active);
   assert(i);
   i--;


   if (file == PROGRAM_INPUT) 
      op = IN1 + cp->VB->AttribPtr[index]->size - 1;
   else if (rel)
      op = PRL;
   else
      op = PAR;

   cp->csr->dword = 0;
   cp->csr->inr.opcode = op;
   cp->csr->inr.reg = i + REG_PAR0;
   cp->csr->inr.file = file;
   cp->csr->inr.idx = index;
   cp->csr++;

   cp->reg[i].file = file;
   cp->reg[i].idx = index;
   cp->par_protected |= (1<<i);
   cp->par_active |= (1<<i);
   return i + REG_PAR0;
}

static void cvp_release_regs( struct compilation *cp )
{
   cp->par_protected = 0;
}



static GLuint cvp_emit_arg( struct compilation *cp,
			    const struct vp_src_register *src,
			    GLuint arg )
{
   GLuint reg = cvp_load_reg( cp, src->File, src->Index, src->RelAddr );
   union instruction rsw, noop;

   /* Emit any necessary swizzling.  
    */
   rsw.dword = 0;
   rsw.rsw.neg = src->Negate ? 1 : 0;
   rsw.rsw.swz = ((GET_SWZ(src->Swizzle, 0) << 0) |
		  (GET_SWZ(src->Swizzle, 1) << 2) |
		  (GET_SWZ(src->Swizzle, 2) << 4) |
		  (GET_SWZ(src->Swizzle, 3) << 6));

   noop.dword = 0;
   noop.rsw.neg = 0;
   noop.rsw.swz = ((0<<0) |
		   (1<<2) |
		   (2<<4) |
		   (3<<6));

   if (rsw.dword != noop.dword) {
      GLuint rsw_reg = arg;
      cp->csr->dword = rsw.dword;
      cp->csr->rsw.opcode = RSW;
      cp->csr->rsw.arg0 = reg;
      cp->csr->rsw.dst = rsw_reg;
      cp->csr++;
      return rsw_reg;
   }
   else
      return reg;
}

static GLuint cvp_choose_result( struct compilation *cp,
				 const struct vp_dst_register *dst,
				 union instruction *fixup,
				 GLuint maxreg)
{
   GLuint mask = dst->WriteMask;

   if (dst->File == PROGRAM_TEMPORARY) {
      
      /* Optimization: When writing (with a writemask) to an undefined
       * value for the first time, the writemask may be ignored.  In
       * practise this means that the MSK instruction to implement the
       * writemask can be dropped.
       */
      if (dst->Index < maxreg &&
	  (mask == 0xf || !(cp->tmp_active & (1<<dst->Index)))) {
	 fixup->dword = 0;
	 cp->tmp_active |= (1<<dst->Index);
	 return REG_TMP0 + dst->Index;
      }
      else if (mask != 0xf) {
	 fixup->msk.opcode = MSK;
	 fixup->msk.arg0 = REG_RES;
	 fixup->msk.dst = REG_TMP0 + dst->Index;
	 fixup->msk.mask = mask;
	 cp->tmp_active |= (1<<dst->Index);
	 return REG_RES;
      }
      else {
	 fixup->vec.opcode = VP_OPCODE_MOV;
	 fixup->vec.arg0 = REG_RES;
	 fixup->vec.dst = REG_TMP0 + dst->Index;
	 cp->tmp_active |= (1<<dst->Index);
	 return REG_RES;
      }
   }
   else {
      assert(dst->File == PROGRAM_OUTPUT);
      fixup->out.opcode = (mask == 0xf) ? OUT : OUM;
      fixup->out.reg = REG_RES;
      fixup->out.file = dst->File;
      fixup->out.idx = dst->Index;
      fixup->out.mask = mask;
      return REG_RES;
   }
}


static void cvp_emit_inst( struct compilation *cp,
			   const struct vp_instruction *inst )
{
   const struct opcode_info *info = &opcode_info[inst->Opcode];
   union instruction fixup;
   GLuint reg[3];
   GLuint result, i;

   /* Need to handle SWZ, ARL specially.
    */
   switch (info->type) {
   case OUT:
      assert(inst->Opcode == VP_OPCODE_ARL);
      reg[0] = cvp_emit_arg( cp, &inst->SrcReg[0], REG_ARG0 );

      cp->csr->dword = 0;
      cp->csr->out.opcode = inst->Opcode;
      cp->csr->out.reg = reg[0];
      cp->csr->out.file = PROGRAM_ADDRESS;
      cp->csr->out.idx = 0;
      break;
   case SWZ:
      assert(inst->Opcode == VP_OPCODE_SWZ);
      result = cvp_choose_result( cp, &inst->DstReg, &fixup, REG_SWZDST_MAX );

      reg[0] = cvp_emit_arg( cp, &inst->SrcReg[0], REG_ARG0 );

      cp->csr->dword = 0;
      cp->csr->swz.opcode = VP_OPCODE_SWZ;
      cp->csr->swz.arg0 = reg[0];
      cp->csr->swz.dst = result;
      cp->csr->swz.neg = inst->SrcReg[0].Negate;
      cp->csr->swz.swz = inst->SrcReg[0].Swizzle;
      cp->csr++;

      if (result == REG_RES) {
	 cp->csr->dword = fixup.dword;
	 cp->csr++;
      }
      break;

   case VEC:
   case SCL:			/* for now */
      result = cvp_choose_result( cp, &inst->DstReg, &fixup, REG_MAX );

      reg[0] = reg[1] = reg[2] = 0;

      for (i = 0; i < info->nr_args; i++)
	 reg[i] = cvp_emit_arg( cp, &inst->SrcReg[i], REG_ARG0 + i );

      cp->csr->dword = 0;
      cp->csr->vec.opcode = inst->Opcode;
      cp->csr->vec.arg0 = reg[0];
      cp->csr->vec.arg1 = reg[1];
      cp->csr->vec.arg2 = reg[2];
      cp->csr->vec.dst = result;
      cp->csr++;

      if (result == REG_RES) {
	 cp->csr->dword = fixup.dword;
	 cp->csr++;
      }      	 
      break;


   case NOP:
      break;

   default:
      assert(0);
      break;
   }

   cvp_release_regs( cp );
}


static void compile_vertex_program( struct arb_vp_machine *m,
				    const struct vertex_program *program )
{ 
   struct compilation cp;
   GLuint i;

   /* Initialize cp:
    */
   memset(&cp, 0, sizeof(cp));
   cp.VB = m->VB;
   cp.csr = m->store;

   /* Compile instructions:
    */
   for (i = 0; i < program->Base.NumInstructions; i++) {
      cvp_emit_inst(&cp, &program->Instructions[i]);
   }

   /* Finish up:
    */
   m->instructions = m->store;
   m->nr_instructions = cp.csr - m->store;


   /* Print/disassemble:
    */
   if (DISASSEM) {
      for (i = 0; i < m->nr_instructions; i++) {
	 union instruction insn = m->instructions[i];
	 const struct opcode_info *info = &opcode_info[insn.vec.opcode];
	 info->print( insn, info );
      }
      _mesa_printf("\n\n");
   }
}




/* ----------------------------------------------------------------------
 * Execution
 */
static void userclip( GLcontext *ctx,
		      GLvector4f *clip,
		      GLubyte *clipmask,
		      GLubyte *clipormask,
		      GLubyte *clipandmask )
{
   GLuint p;

   for (p = 0; p < ctx->Const.MaxClipPlanes; p++)
      if (ctx->Transform.ClipPlanesEnabled & (1 << p)) {
	 GLuint nr, i;
	 const GLfloat a = ctx->Transform._ClipUserPlane[p][0];
	 const GLfloat b = ctx->Transform._ClipUserPlane[p][1];
	 const GLfloat c = ctx->Transform._ClipUserPlane[p][2];
	 const GLfloat d = ctx->Transform._ClipUserPlane[p][3];
         GLfloat *coord = (GLfloat *)clip->data;
         GLuint stride = clip->stride;
         GLuint count = clip->count;

	 for (nr = 0, i = 0 ; i < count ; i++) {
	    GLfloat dp = (coord[0] * a + 
			  coord[1] * b +
			  coord[2] * c +
			  coord[3] * d);

	    if (dp < 0) {
	       nr++;
	       clipmask[i] |= CLIP_USER_BIT;
	    }

	    STRIDE_F(coord, stride);
	 }

	 if (nr > 0) {
	    *clipormask |= CLIP_USER_BIT;
	    if (nr == count) {
	       *clipandmask |= CLIP_USER_BIT;
	       return;
	    }
	 }
      }
}


static GLboolean do_ndc_cliptest( struct arb_vp_machine *m )
{
   GLcontext *ctx = m->ctx;
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = m->VB;

   /* Cliptest and perspective divide.  Clip functions must clear
    * the clipmask.
    */
   m->ormask = 0;
   m->andmask = CLIP_ALL_BITS;

   if (tnl->NeedNdcCoords) {
      VB->NdcPtr =
         _mesa_clip_tab[VB->ClipPtr->size]( VB->ClipPtr,
                                            &m->ndcCoords,
                                            m->clipmask,
                                            &m->ormask,
                                            &m->andmask );
   }
   else {
      VB->NdcPtr = NULL;
      _mesa_clip_np_tab[VB->ClipPtr->size]( VB->ClipPtr,
                                            NULL,
                                            m->clipmask,
                                            &m->ormask,
                                            &m->andmask );
   }

   if (m->andmask) {
      /* All vertices are outside the frustum */
      return GL_FALSE;
   }

   /* Test userclip planes.  This contributes to VB->ClipMask.
    */
   if (ctx->Transform.ClipPlanesEnabled && !ctx->VertexProgram._Enabled) {
      userclip( ctx,
		VB->ClipPtr,
		m->clipmask,
		&m->ormask,
		&m->andmask );

      if (m->andmask) {
	 return GL_FALSE;
      }
   }

   VB->ClipAndMask = m->andmask;
   VB->ClipOrMask = m->ormask;
   VB->ClipMask = m->clipmask;

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
static GLboolean
run_arb_vertex_program(GLcontext *ctx, struct tnl_pipeline_stage *stage)
{
   struct vertex_program *program = (ctx->VertexProgram._Enabled ?
				     ctx->VertexProgram.Current :
				     ctx->_TnlProgram);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   struct arb_vp_machine *m = ARB_VP_MACHINE(stage);
   GLuint i, j, outputs = program->OutputsWritten;

   if (program->Parameters) {
      _mesa_load_state_parameters(ctx, program->Parameters);
      m->File[PROGRAM_STATE_VAR] = program->Parameters->ParameterValues;
   }   

   /* Run the actual program:
    */
   for (m->vtx_nr = 0; m->vtx_nr < VB->Count; m->vtx_nr++) {
      for (j = 0; j < m->nr_instructions; j++) {
	 union instruction inst = m->instructions[j];	 
	 opcode_info[inst.vec.opcode].func( m, inst );
      }
   }

   /* Setup the VB pointers so that the next pipeline stages get
    * their data from the right place (the program output arrays).
    *
    * TODO: 1) Have tnl use these RESULT values for outputs rather
    * than trying to shoe-horn inputs and outputs into one set of
    * values.
    *
    * TODO: 2) Integrate t_vertex.c so that we just go straight ahead
    * and build machine vertices here.
    */
   VB->ClipPtr = &m->attribs[VERT_RESULT_HPOS];
   VB->ClipPtr->count = VB->Count;

   if (outputs & (1<<VERT_RESULT_COL0)) {
      VB->ColorPtr[0] = &m->attribs[VERT_RESULT_COL0];
      VB->AttribPtr[VERT_ATTRIB_COLOR0] = VB->ColorPtr[0];
   }

   if (outputs & (1<<VERT_RESULT_BFC0)) {
      VB->ColorPtr[1] = &m->attribs[VERT_RESULT_BFC0];
   }

   if (outputs & (1<<VERT_RESULT_COL1)) {
      VB->SecondaryColorPtr[0] = &m->attribs[VERT_RESULT_COL1];
      VB->AttribPtr[VERT_ATTRIB_COLOR1] = VB->SecondaryColorPtr[0];
   }

   if (outputs & (1<<VERT_RESULT_BFC1)) {
      VB->SecondaryColorPtr[1] = &m->attribs[VERT_RESULT_BFC1];
   }

   if (outputs & (1<<VERT_RESULT_FOGC)) {
      VB->FogCoordPtr = &m->attribs[VERT_RESULT_FOGC];
      VB->AttribPtr[VERT_ATTRIB_FOG] = VB->FogCoordPtr;
   }

   if (outputs & (1<<VERT_RESULT_PSIZ)) {
      VB->PointSizePtr = &m->attribs[VERT_RESULT_PSIZ];
      VB->AttribPtr[_TNL_ATTRIB_POINTSIZE] = &m->attribs[VERT_RESULT_PSIZ];
   }

   for (i = 0; i < ctx->Const.MaxTextureUnits; i++) {
      if (outputs & (1<<(VERT_RESULT_TEX0+i))) {
	 VB->TexCoordPtr[i] = &m->attribs[VERT_RESULT_TEX0 + i];
	 VB->AttribPtr[VERT_ATTRIB_TEX0+i] = VB->TexCoordPtr[i];
      }
   }

#if 0
   for (i = 0; i < VB->Count; i++) {
      printf("Out %d: %f %f %f %f %f %f %f %f\n", i,
	     VEC_ELT(VB->ClipPtr, GLfloat, i)[0],
	     VEC_ELT(VB->ClipPtr, GLfloat, i)[1],
	     VEC_ELT(VB->ClipPtr, GLfloat, i)[2],
	     VEC_ELT(VB->ClipPtr, GLfloat, i)[3],
	     VEC_ELT(VB->ColorPtr[0], GLfloat, i)[0],
	     VEC_ELT(VB->ColorPtr[0], GLfloat, i)[1],
	     VEC_ELT(VB->ColorPtr[0], GLfloat, i)[2],
	     VEC_ELT(VB->ColorPtr[0], GLfloat, i)[3]);
   }
#endif

   /* Perform NDC and cliptest operations:
    */
   return do_ndc_cliptest(m);
}


static void
validate_vertex_program( GLcontext *ctx, struct tnl_pipeline_stage *stage )
{
   struct arb_vp_machine *m = ARB_VP_MACHINE(stage);
   struct vertex_program *program = 
      (ctx->VertexProgram._Enabled ? ctx->VertexProgram.Current : 0);

#if TNL_FIXED_FUNCTION_PROGRAM
   if (!program) {
      program = ctx->_TnlProgram;
   }
#endif

   if (program) {
      compile_vertex_program( m, program );
      
      /* Grab the state GL state and put into registers:
       */
      m->File[PROGRAM_LOCAL_PARAM] = program->Base.LocalParams;
      m->File[PROGRAM_ENV_PARAM] = ctx->VertexProgram.Parameters;
      m->File[PROGRAM_STATE_VAR] = 0;
   }
}







/**
 * Called the first time stage->run is called.  In effect, don't
 * allocate data until the first time the stage is run.
 */
static GLboolean init_vertex_program( GLcontext *ctx,
				      struct tnl_pipeline_stage *stage )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &(tnl->vb);
   struct arb_vp_machine *m;
   const GLuint size = VB->Size;
   GLuint i;

   stage->privatePtr = MALLOC(sizeof(*m));
   m = ARB_VP_MACHINE(stage);
   if (!m)
      return GL_FALSE;

   /* arb_vertex_machine struct should subsume the VB:
    */
   m->VB = VB;
   m->ctx = ctx;

   /* Allocate arrays of vertex output values */
   for (i = 0; i < VERT_RESULT_MAX; i++) {
      _mesa_vector4f_alloc( &m->attribs[i], 0, size, 32 );
      m->attribs[i].size = 4;
   }

   /* a few other misc allocations */
   _mesa_vector4f_alloc( &m->ndcCoords, 0, size, 32 );
   m->clipmask = (GLubyte *) ALIGN_MALLOC(sizeof(GLubyte)*size, 32 );


#if TNL_FIXED_FUNCTION_PROGRAM
   _mesa_allow_light_in_model( ctx, GL_FALSE );
#endif


   return GL_TRUE;
}




/**
 * Destructor for this pipeline stage.
 */
static void dtr( struct tnl_pipeline_stage *stage )
{
   struct arb_vp_machine *m = ARB_VP_MACHINE(stage);

   if (m) {
      GLuint i;

      /* free the vertex program result arrays */
      for (i = 0; i < VERT_RESULT_MAX; i++)
         _mesa_vector4f_free( &m->attribs[i] );

      /* free misc arrays */
      _mesa_vector4f_free( &m->ndcCoords );
      ALIGN_FREE( m->clipmask );

      FREE( m );
      stage->privatePtr = NULL;
   }
}

/**
 * Public description of this pipeline stage.
 */
const struct tnl_pipeline_stage _tnl_arb_vertex_program_stage =
{
   "vertex-program",
   NULL,			/* private_data */
   init_vertex_program,		/* create */
   dtr,				/* destroy */
   validate_vertex_program,	/* validate */
   run_arb_vertex_program	/* run */
};
