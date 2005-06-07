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
#define RSW        (VP_MAX_OPCODE)
#define SEL        (VP_MAX_OPCODE+1)
#define REL        (VP_MAX_OPCODE+2)


/* Layout of register file:

  0 -- Scratch (Arg0)
  1 -- Scratch (Arg1)
  2 -- Scratch (Result)
  4 -- Program Temporary 0
  16 -- Program Temporary 12 (max for NV_VERTEX_PROGRAM)
  17 -- Output 0
  31 -- Output 15 (max for NV_VERTEX_PROGRAM) (Last writeable register)
  32 -- Parameter 0
  ..
  127 -- Parameter 63 (max for NV_VERTEX_PROGRAM)

*/

#define FILE_REG         0
#define FILE_LOCAL_PARAM 1
#define FILE_ENV_PARAM   2
#define FILE_STATE_PARAM 3


#define REG_ARG0   0
#define REG_ARG1   1
#define REG_ARG2   2
#define REG_RES    3
#define REG_ADDR   4
#define REG_TMP0   5
#define REG_TMP11  16
#define REG_OUT0   17
#define REG_OUT14  31
#define REG_IN0    32
#define REG_IN15   47
#define REG_ID     48		/* 0,0,0,1 */
#define REG_MAX    128
#define REG_INVALID ~0

/* ARB_vp instructions are broken down into one or more of the
 * following micro-instructions, each representable in a 32 bit packed
 * structure.
 */

struct reg {
   GLuint file:2;
   GLuint idx:7;
};


union instruction {
   struct {
      GLuint opcode:6;
      GLuint dst:5;
      GLuint file0:2;
      GLuint idx0:7;
      GLuint file1:2;
      GLuint idx1:7;
      GLuint pad:3;
   } alu;

   struct {
      GLuint opcode:6;
      GLuint dst:5;
      GLuint file0:2;
      GLuint idx0:7;
      GLuint neg:4;
      GLuint swz:8;		/* xyzw only */
   } rsw;

   struct {
      GLuint opcode:6;
      GLuint dst:5;
      GLuint idx0:7;		/* note! */
      GLuint file1:2;
      GLuint idx1:7;
      GLuint mask:4;
      GLuint pad:1;
   } sel;

   GLuint dword;
};



struct compilation {
   GLuint reg_active;
   union instruction *csr;
   struct vertex_buffer *VB;	/* for input sizes! */
};

struct input {
   GLuint idx;
   GLfloat *data;
   GLuint stride;
   GLuint size;
};

struct output {
   GLuint idx;
   GLfloat *data;
};

/*--------------------------------------------------------------------------- */

/*!
 * Private storage for the vertex program pipeline stage.
 */
struct arb_vp_machine {
   GLfloat reg[REG_MAX][4];	/* Program temporaries, inputs and outputs */
   GLfloat (*File[4])[4];	/* All values reference-able from the program. */
   GLint AddressReg;

   struct input input[16];
   GLuint nr_inputs;

   struct output output[15];
   GLuint nr_outputs;

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
   GLuint nr_args;
   const char *string;
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
   return (GLfloat) _mesa_pow(x, y);
}


static const GLfloat ZeroVec[4] = { 0.0F, 0.0F, 0.0F, 0.0F };



#define GET_RSW(swz, idx)      (((swz) >> ((idx)*2)) & 0x3)


/**
 * Perform a reduced swizzle:
 */
static void do_RSW( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->reg[op.rsw.dst];
   const GLfloat *arg0 = m->File[op.rsw.file0][op.rsw.idx0];
   GLuint swz = op.rsw.swz;
   GLuint neg = op.rsw.neg;

   result[0] = arg0[GET_RSW(swz, 0)];
   result[1] = arg0[GET_RSW(swz, 1)];
   result[2] = arg0[GET_RSW(swz, 2)];
   result[3] = arg0[GET_RSW(swz, 3)];
   
   if (neg) {
      if (neg & 0x1) result[0] = -result[0];
      if (neg & 0x2) result[1] = -result[1];
      if (neg & 0x4) result[2] = -result[2];
      if (neg & 0x8) result[3] = -result[3];
   }
}

/* Used to implement write masking
 */
static void do_SEL( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *dst = m->reg[op.sel.dst];
   const GLfloat *arg0 = m->reg[op.sel.idx0];
   const GLfloat *arg1 = m->File[op.sel.file1][op.sel.idx1];
 
   dst[0] = (op.sel.mask & 0x1) ? arg0[0] : arg1[0];
   dst[1] = (op.sel.mask & 0x2) ? arg0[1] : arg1[1];
   dst[2] = (op.sel.mask & 0x4) ? arg0[2] : arg1[2];
   dst[3] = (op.sel.mask & 0x8) ? arg0[3] : arg1[3];
}


static void do_PRT( struct arb_vp_machine *m, union instruction op )
{
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   
   _mesa_printf("%d: %f %f %f %f\n", m->vtx_nr, 
		arg0[0], arg0[1], arg0[2], arg0[3]);
}


/**
 * The traditional ALU and texturing instructions.  All operate on
 * internal registers and ignore write masks and swizzling issues.
 */

static void do_ABS( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->reg[op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];

   result[0] = (arg0[0] < 0.0) ? -arg0[0] : arg0[0];
   result[1] = (arg0[1] < 0.0) ? -arg0[1] : arg0[1];
   result[2] = (arg0[2] < 0.0) ? -arg0[2] : arg0[2];
   result[3] = (arg0[3] < 0.0) ? -arg0[3] : arg0[3];
}

static void do_ADD( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   result[0] = arg0[0] + arg1[0];
   result[1] = arg0[1] + arg1[1];
   result[2] = arg0[2] + arg1[2];
   result[3] = arg0[3] + arg1[3];
}


static void do_ARL( struct arb_vp_machine *m, union instruction op )
{
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   m->reg[REG_ADDR][0] = FLOORF(arg0[0]);
}


static void do_DP3( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   result[0] = (arg0[0] * arg1[0] + 
		arg0[1] * arg1[1] + 
		arg0[2] * arg1[2]);

   PUFF(result);
}



static void do_DP4( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   result[0] = (arg0[0] * arg1[0] + 
		arg0[1] * arg1[1] + 
		arg0[2] * arg1[2] + 
		arg0[3] * arg1[3]);

   PUFF(result);
}

static void do_DPH( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   result[0] = (arg0[0] * arg1[0] + 
		arg0[1] * arg1[1] + 
		arg0[2] * arg1[2] + 
		1.0     * arg1[3]);
   
   PUFF(result);
}

static void do_DST( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   result[0] = 1.0F;
   result[1] = arg0[1] * arg1[1];
   result[2] = arg0[2];
   result[3] = arg1[3];
}


static void do_EX2( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->reg[op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];

   result[0] = (GLfloat)RoughApproxPow2(arg0[0]);
   PUFF(result);
}

static void do_EXP( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
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
   GLfloat *result = m->reg[op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];

   result[0] = FLOORF(arg0[0]);
   result[1] = FLOORF(arg0[1]);
   result[2] = FLOORF(arg0[2]);
   result[3] = FLOORF(arg0[3]);
}

static void do_FRC( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->reg[op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];

   result[0] = arg0[0] - FLOORF(arg0[0]);
   result[1] = arg0[1] - FLOORF(arg0[1]);
   result[2] = arg0[2] - FLOORF(arg0[2]);
   result[3] = arg0[3] - FLOORF(arg0[3]);
}

static void do_LG2( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->reg[op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];

   result[0] = RoughApproxLog2(arg0[0]);
   PUFF(result);
}



static void do_LIT( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];

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
   GLfloat *result = m->reg[op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   GLfloat tmp = FABSF(arg0[0]);
   int exponent;
   GLfloat mantissa = FREXPF(tmp, &exponent);

   result[0] = (GLfloat) (exponent - 1);
   result[1] = 2.0 * mantissa; /* map [.5, 1) -> [1, 2) */
   result[2] = result[0] + LOG2(result[1]);
   result[3] = 1.0;
}

static void do_MAX( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   result[0] = (arg0[0] > arg1[0]) ? arg0[0] : arg1[0];
   result[1] = (arg0[1] > arg1[1]) ? arg0[1] : arg1[1];
   result[2] = (arg0[2] > arg1[2]) ? arg0[2] : arg1[2];
   result[3] = (arg0[3] > arg1[3]) ? arg0[3] : arg1[3];
}


static void do_MIN( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   result[0] = (arg0[0] < arg1[0]) ? arg0[0] : arg1[0];
   result[1] = (arg0[1] < arg1[1]) ? arg0[1] : arg1[1];
   result[2] = (arg0[2] < arg1[2]) ? arg0[2] : arg1[2];
   result[3] = (arg0[3] < arg1[3]) ? arg0[3] : arg1[3];
}

static void do_MOV( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];

   result[0] = arg0[0];
   result[1] = arg0[1];
   result[2] = arg0[2];
   result[3] = arg0[3];
}

static void do_MUL( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   result[0] = arg0[0] * arg1[0];
   result[1] = arg0[1] * arg1[1];
   result[2] = arg0[2] * arg1[2];
   result[3] = arg0[3] * arg1[3];
}


static void do_POW( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->reg[op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   result[0] = (GLfloat)RoughApproxPower(arg0[0], arg1[0]);
   PUFF(result);
}

static void do_REL( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.alu.dst];
   GLuint idx = (op.alu.idx0 + (GLint)m->reg[REG_ADDR][0]) & (MAX_NV_VERTEX_PROGRAM_PARAMS-1);
   const GLfloat *arg0 = m->File[op.alu.file0][idx];

   result[0] = arg0[0];
   result[1] = arg0[1];
   result[2] = arg0[2];
   result[3] = arg0[3];
}

static void do_RCP( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];

   result[0] = 1.0F / arg0[0];  
   PUFF(result);
}

static void do_RSQ( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];

   result[0] = INV_SQRTF(FABSF(arg0[0]));
   PUFF(result);
}


static void do_SGE( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   result[0] = (arg0[0] >= arg1[0]) ? 1.0F : 0.0F;
   result[1] = (arg0[1] >= arg1[1]) ? 1.0F : 0.0F;
   result[2] = (arg0[2] >= arg1[2]) ? 1.0F : 0.0F;
   result[3] = (arg0[3] >= arg1[3]) ? 1.0F : 0.0F;
}


static void do_SLT( struct arb_vp_machine *m, union instruction op )
{
   GLfloat *result = m->reg[op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   result[0] = (arg0[0] < arg1[0]) ? 1.0F : 0.0F;
   result[1] = (arg0[1] < arg1[1]) ? 1.0F : 0.0F;
   result[2] = (arg0[2] < arg1[2]) ? 1.0F : 0.0F;
   result[3] = (arg0[3] < arg1[3]) ? 1.0F : 0.0F;
}

static void do_SUB( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->reg[op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   result[0] = arg0[0] - arg1[0];
   result[1] = arg0[1] - arg1[1];
   result[2] = arg0[2] - arg1[2];
   result[3] = arg0[3] - arg1[3];
}


static void do_XPD( struct arb_vp_machine *m, union instruction op ) 
{
   GLfloat *result = m->reg[op.alu.dst];
   const GLfloat *arg0 = m->File[op.alu.file0][op.alu.idx0];
   const GLfloat *arg1 = m->File[op.alu.file1][op.alu.idx1];

   result[0] = arg0[1] * arg1[2] - arg0[2] * arg1[1];
   result[1] = arg0[2] * arg1[0] - arg0[0] * arg1[2];
   result[2] = arg0[0] * arg1[1] - arg0[1] * arg1[0];
}

static void do_NOP( struct arb_vp_machine *m, union instruction op ) 
{
}

/* Some useful debugging functions:
 */
static void print_mask( GLuint mask )
{
   _mesa_printf(".");
   if (mask&0x1) _mesa_printf("x");
   if (mask&0x2) _mesa_printf("y");
   if (mask&0x4) _mesa_printf("z");
   if (mask&0x8) _mesa_printf("w");
}

static void print_reg( GLuint file, GLuint reg )
{
   static const char *reg_file[] = {
      "REG",
      "LOCAL_PARAM",
      "ENV_PARAM",
      "STATE_VAR",
   };

   if (file == 0) {
      if (reg == REG_RES) 
	 _mesa_printf("RES");
      else if (reg >= REG_ARG0 && reg <= REG_ARG1)
	 _mesa_printf("ARG%d", reg - REG_ARG0);
      else if (reg >= REG_TMP0 && reg <= REG_TMP11)
	 _mesa_printf("TMP%d", reg - REG_TMP0);
      else if (reg >= REG_IN0 && reg <= REG_IN15)
	 _mesa_printf("IN%d", reg - REG_IN0);
      else if (reg >= REG_OUT0 && reg <= REG_OUT14)
	 _mesa_printf("OUT%d", reg - REG_OUT0);
      else if (reg == REG_ADDR)
	 _mesa_printf("ADDR");
      else if (reg == REG_ID)
	 _mesa_printf("ID");
      else
	 _mesa_printf("REG%d", reg);
   }
   else 
      _mesa_printf("%s:%d", reg_file[file], reg);
}


static void print_RSW( union instruction op, const struct opcode_info *info )
{
   GLuint swz = op.rsw.swz;
   GLuint neg = op.rsw.neg;
   GLuint i;

   _mesa_printf("%s ", info->string);
   print_reg(0, op.rsw.dst);
   _mesa_printf(", ");
   print_reg(op.rsw.file0, op.rsw.idx0);
   _mesa_printf(".");
   for (i = 0; i < 4; i++, swz >>= 2) {
      const char *cswz = "xyzw";
      if (neg & (1<<i))   
	 _mesa_printf("-");
      _mesa_printf("%c", cswz[swz&0x3]);
   }
   _mesa_printf("\n");
}


static void print_ALU( union instruction op, const struct opcode_info *info )
{
   _mesa_printf("%s ", info->string);
   print_reg(0, op.alu.dst);
   _mesa_printf(", ");
   print_reg(op.alu.file0, op.alu.idx0);
   if (info->nr_args > 1) {
      _mesa_printf(", ");
      print_reg(op.alu.file1, op.alu.idx1);
   }
   _mesa_printf("\n");
}

static void print_SEL( union instruction op, const struct opcode_info *info )
{
   _mesa_printf("%s ", info->string);
   print_reg(0, op.sel.dst);
   _mesa_printf(", ");
   print_reg(0, op.sel.idx0);
   print_mask(op.sel.mask);
   _mesa_printf(", ");
   print_reg(op.sel.file1, op.sel.idx1);
   print_mask(~op.sel.mask);
   _mesa_printf("\n");
}


static void print_NOP( union instruction op, const struct opcode_info *info )
{
}

#define NOP 0
#define ALU 1
#define SWZ 2

static const struct opcode_info opcode_info[] = 
{
   { 1, "ABS", print_ALU },
   { 2, "ADD", print_ALU },
   { 1, "ARL", print_ALU },
   { 2, "DP3", print_ALU },
   { 2, "DP4", print_ALU },
   { 2, "DPH", print_ALU },
   { 2, "DST", print_ALU },
   { 0, "END", print_NOP },
   { 1, "EX2", print_ALU },
   { 1, "EXP", print_ALU },
   { 1, "FLR", print_ALU },
   { 1, "FRC", print_ALU },
   { 1, "LG2", print_ALU },
   { 1, "LIT", print_ALU },
   { 1, "LOG", print_ALU },
   { 3, "MAD", print_NOP },
   { 2, "MAX", print_ALU },
   { 2, "MIN", print_ALU },
   { 1, "MOV", print_ALU },
   { 2, "MUL", print_ALU },
   { 2, "POW", print_ALU },
   { 1, "PRT", print_ALU }, /* PRINT */
   { 1, "RCC", print_NOP },
   { 1, "RCP", print_ALU },
   { 1, "RSQ", print_ALU },
   { 2, "SGE", print_ALU },
   { 2, "SLT", print_ALU },
   { 2, "SUB", print_ALU },
   { 1, "SWZ", print_NOP },
   { 2, "XPD", print_ALU },
   { 1, "RSW", print_RSW },
   { 2, "SEL", print_SEL },
   { 1, "REL", print_ALU },
};


static void (* const opcode_func[])(struct arb_vp_machine *, union instruction) = 
{
   do_ABS,
   do_ADD,
   do_ARL,
   do_DP3,
   do_DP4,
   do_DPH,
   do_DST,
   do_NOP,
   do_EX2,
   do_EXP,
   do_FLR,
   do_FRC,
   do_LG2,
   do_LIT,
   do_LOG,
   do_NOP,
   do_MAX,
   do_MIN,
   do_MOV,
   do_MUL,
   do_POW,
   do_PRT,
   do_NOP,
   do_RCP,
   do_RSQ,
   do_SGE,
   do_SLT,
   do_SUB,
   do_RSW,
   do_XPD,
   do_RSW,
   do_SEL,
   do_REL,
};

static union instruction *cvp_next_instruction( struct compilation *cp )
{
   union instruction *op = cp->csr++;
   op->dword = 0;
   return op;
}

static struct reg cvp_make_reg( GLuint file, GLuint idx )
{
   struct reg reg;
   reg.file = file;
   reg.idx = idx;
   return reg;
}

static struct reg cvp_emit_rel( struct compilation *cp,
				struct reg reg,
				struct reg tmpreg )
{
   union instruction *op = cvp_next_instruction(cp);
   op->alu.opcode = REL;
   op->alu.file0 = reg.file;
   op->alu.idx0 = reg.idx;
   op->alu.dst = tmpreg.idx;
   return tmpreg;
}


static struct reg cvp_load_reg( struct compilation *cp,
				GLuint file,
				GLuint index,
				GLuint rel,
				GLuint tmpidx )
{
   struct reg tmpreg = cvp_make_reg(FILE_REG, tmpidx);
   struct reg reg;

   switch (file) {
   case PROGRAM_TEMPORARY:
      return cvp_make_reg(FILE_REG, REG_TMP0 + index);

   case PROGRAM_INPUT:
      return cvp_make_reg(FILE_REG, REG_IN0 + index);

   case PROGRAM_OUTPUT:
      return cvp_make_reg(FILE_REG, REG_OUT0 + index);

      /* These two aren't populated by the parser?
       */
   case PROGRAM_LOCAL_PARAM: 
      reg = cvp_make_reg(FILE_LOCAL_PARAM, index);
      if (rel) 
	 return cvp_emit_rel(cp, reg, tmpreg);
      else
	 return reg;

   case PROGRAM_ENV_PARAM: 
      reg = cvp_make_reg(FILE_ENV_PARAM, index);
      if (rel) 
	 return cvp_emit_rel(cp, reg, tmpreg);
      else
	 return reg;

   case PROGRAM_STATE_VAR:
      reg = cvp_make_reg(FILE_STATE_PARAM, index);
      if (rel) 
	 return cvp_emit_rel(cp, reg, tmpreg);
      else
	 return reg;

      /* Invalid values:
       */
   case PROGRAM_WRITE_ONLY:
   case PROGRAM_ADDRESS:
   default:
      assert(0);
      return tmpreg;		/* can't happen */
   }
}

static struct reg cvp_emit_arg( struct compilation *cp,
				const struct vp_src_register *src,
				GLuint arg )
{
   struct reg reg = cvp_load_reg( cp, src->File, src->Index, src->RelAddr, arg );
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
      union instruction *op = cvp_next_instruction(cp);
      struct reg rsw_reg = cvp_make_reg(FILE_REG, REG_ARG0 + arg);
      op->dword = rsw.dword;
      op->rsw.opcode = RSW;
      op->rsw.file0 = reg.file;
      op->rsw.idx0 = reg.idx;
      op->rsw.dst = rsw_reg.idx;
      return rsw_reg;
   }
   else
      return reg;
}

static GLuint cvp_choose_result( struct compilation *cp,
				 const struct vp_dst_register *dst,
				 union instruction *fixup )
{
   GLuint mask = dst->WriteMask;
   GLuint idx;

   switch (dst->File) {
   case PROGRAM_TEMPORARY:
      idx = REG_TMP0 + dst->Index;
      break;
   case PROGRAM_OUTPUT:
      idx = REG_OUT0 + dst->Index;
      break;
   default:
      assert(0);
      return REG_RES;		/* can't happen */
   }

   /* Optimization: When writing (with a writemask) to an undefined
    * value for the first time, the writemask may be ignored. 
    */
   if (mask != WRITEMASK_XYZW && (cp->reg_active & (1 << idx))) {
      fixup->sel.opcode = SEL;
      fixup->sel.idx0 = REG_RES;
      fixup->sel.file1 = FILE_REG;
      fixup->sel.idx1 = idx;
      fixup->sel.dst = idx;
      fixup->sel.mask = mask;
      cp->reg_active |= 1 << idx;
      return REG_RES;
   }
   else {
      fixup->dword = 0;
      cp->reg_active |= 1 << idx;
      return idx;
   }
}

#define RSW_NOOP ((0<<0) | (1<<2) | (2<<4) | (3<<6))

static struct reg cvp_emit_rsw( struct compilation *cp, 
				GLuint dst,
				struct reg src,
				GLuint neg, 
				GLuint swz,
				GLboolean force)
{
   struct reg retval;

   if (swz != RSW_NOOP || neg != 0) {
      union instruction *op = cvp_next_instruction(cp);
      op->rsw.opcode = RSW;
      op->rsw.dst = dst;
      op->rsw.file0 = src.file;
      op->rsw.idx0 = src.idx;
      op->rsw.neg = neg;
      op->rsw.swz = swz;
	    
      retval.file = FILE_REG;
      retval.idx = dst;
      return retval;
   }
   else if (force) {
      /* Oops.  Degenerate case:
       */
      union instruction *op = cvp_next_instruction(cp);
      op->alu.opcode = VP_OPCODE_MOV;
      op->alu.dst = dst;
      op->alu.file0 = src.file;
      op->alu.idx0 = src.idx;
      
      retval.file = FILE_REG;
      retval.idx = dst;
      return retval;
   }
   else {
      return src;
   }
}


static void cvp_emit_inst( struct compilation *cp,
			   const struct vp_instruction *inst )
{
   const struct opcode_info *info = &opcode_info[inst->Opcode];
   union instruction *op;
   union instruction fixup;
   struct reg reg[3];
   GLuint result, i;

   /* Need to handle SWZ, ARL specially.
    */
   switch (inst->Opcode) {
      /* Split into mul and add:
       */
   case VP_OPCODE_MAD:
      result = cvp_choose_result( cp, &inst->DstReg, &fixup );
      for (i = 0; i < 3; i++) 
	 reg[i] = cvp_emit_arg( cp, &inst->SrcReg[i], REG_ARG0+i );

      op = cvp_next_instruction(cp);
      op->alu.opcode = VP_OPCODE_MUL;
      op->alu.file0 = reg[0].file;
      op->alu.idx0 = reg[0].idx;
      op->alu.file1 = reg[1].file;
      op->alu.idx1 = reg[1].idx;
      op->alu.dst = REG_ARG0;

      op = cvp_next_instruction(cp);
      op->alu.opcode = VP_OPCODE_ADD;
      op->alu.file0 = FILE_REG;
      op->alu.idx0 = REG_ARG0;
      op->alu.file1 = reg[2].file;
      op->alu.idx1 = reg[2].idx;
      op->alu.dst = result;
      break;

   case VP_OPCODE_ARL:
      reg[0] = cvp_emit_arg( cp, &inst->SrcReg[0], REG_ARG0 );

      op = cvp_next_instruction(cp);
      op->alu.opcode = inst->Opcode;
      op->alu.dst = REG_ADDR;
      op->alu.file0 = reg[0].file;
      op->alu.idx0 = reg[0].idx;
      break;

   case VP_OPCODE_SWZ: {
      GLuint swz0, swz1;
      GLuint neg0, neg1;
      GLuint mask = 0;

      /* Translate 3-bit-per-element swizzle into two 2-bit swizzles,
       * one from the source register the other from a constant
       * {0,0,0,1}.
       */
      for (i = 0; i < 4; i++) {
	 GLuint swzelt = GET_SWZ(inst->SrcReg[0].Swizzle, i);
	 if (swzelt >= SWIZZLE_ZERO) {
	    neg0 |= inst->SrcReg[0].Negate & (1<<i);
	    if (swzelt == SWIZZLE_ONE)
	       swz0 |= SWIZZLE_W << (i*2);
	    else if (i < SWIZZLE_W)
	       swz0 |= i << (i*2);
	 }
	 else {
	    mask |= 1<<i;
	    neg1 |= inst->SrcReg[0].Negate & (1<<i);
	    swz1 |= swzelt << (i*2);
	 }
      }

      result = cvp_choose_result( cp, &inst->DstReg, &fixup );
      reg[0].file = FILE_REG;
      reg[0].idx = REG_ID;
      reg[1] = cvp_emit_arg( cp, &inst->SrcReg[0], REG_ARG0 );

      if (mask == WRITEMASK_XYZW) {
	 cvp_emit_rsw(cp, result, reg[0], neg0, swz0, GL_TRUE);
	 
      }
      else if (mask == 0) {
	 cvp_emit_rsw(cp, result, reg[1], neg1, swz1, GL_TRUE);
      }
      else {
	 reg[0] = cvp_emit_rsw(cp, REG_ARG0, reg[0], neg0, swz0, GL_FALSE);
	 reg[1] = cvp_emit_rsw(cp, REG_ARG1, reg[1], neg1, swz1, GL_FALSE);

	 assert(reg[0].file == FILE_REG);

	 op = cvp_next_instruction(cp);
	 op->sel.opcode = SEL;
	 op->sel.dst = result;
	 op->sel.idx0 = reg[0].idx;
	 op->sel.file1 = reg[1].file;
	 op->sel.idx1 = reg[1].idx;
	 op->sel.mask = mask;
      }

      if (result == REG_RES) {
	 op = cvp_next_instruction(cp);
	 op->dword = fixup.dword;
      }
      break;
   }
   case VP_OPCODE_PRINT:
   case VP_OPCODE_END:
      break;

   default:
      result = cvp_choose_result( cp, &inst->DstReg, &fixup );
      for (i = 0; i < info->nr_args; i++) 
	 reg[i] = cvp_emit_arg( cp, &inst->SrcReg[i], REG_ARG0 + i );

      op = cvp_next_instruction(cp);
      op->alu.opcode = inst->Opcode;
      op->alu.file0 = reg[0].file;
      op->alu.idx0 = reg[0].idx;
      op->alu.file1 = reg[1].file;
      op->alu.idx1 = reg[1].idx;
      op->alu.dst = result;

      if (result == REG_RES) {
	 op = cvp_next_instruction(cp);
	 op->dword = fixup.dword;
      }      	 
      break;
   }
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
	 const struct opcode_info *info = &opcode_info[insn.alu.opcode];
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
   }   
   

   /* Initialize regs where necessary:
    */
   ASSIGN_4V(m->reg[REG_ID], 0, 0, 0, 1);

   m->nr_inputs = m->nr_outputs = 0;

   for (i = 0; i < 16; i++) {
      if (program->InputsRead & (1<<i)) {
	 GLuint j = m->nr_inputs++;
	 m->input[j].idx = i;
	 m->input[j].data = m->VB->AttribPtr[i]->data;
	 m->input[j].stride = m->VB->AttribPtr[i]->stride;
	 m->input[j].size = m->VB->AttribPtr[i]->size;
	 ASSIGN_4V(m->reg[REG_IN0 + i], 0, 0, 0, 1);
      }
   }     

   for (i = 0; i < 15; i++) {
      if (program->OutputsWritten & (1<<i)) {
	 GLuint j = m->nr_outputs++;
	 m->output[j].idx = i;
	 m->output[j].data = m->attribs[i].data;
      }
   }     


   /* Run the actual program:
    */
   for (m->vtx_nr = 0; m->vtx_nr < VB->Count; m->vtx_nr++) {
      for (j = 0; j < m->nr_inputs; j++) {
	 GLuint idx = REG_IN0 + m->input[j].idx;
	 switch (m->input[j].size) {
	 case 4: m->reg[idx][3] = m->input[j].data[3];
	 case 3: m->reg[idx][2] = m->input[j].data[2];
	 case 2: m->reg[idx][1] = m->input[j].data[1];
	 case 1: m->reg[idx][0] = m->input[j].data[0];
	 }

	 STRIDE_F(m->input[j].data, m->input[j].stride);
      }

      for (j = 0; j < m->nr_instructions; j++) {
	 union instruction inst = m->instructions[j];	 
	 opcode_func[inst.alu.opcode]( m, inst );
      }

      for (j = 0; j < m->nr_outputs; j++) {
	 GLuint idx = REG_OUT0 + m->output[j].idx;
	 m->output[j].data[0] = m->reg[idx][0];
	 m->output[j].data[1] = m->reg[idx][1];
	 m->output[j].data[2] = m->reg[idx][2];
	 m->output[j].data[3] = m->reg[idx][3];
	 m->output[j].data += 4;
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
      m->File[FILE_REG] = m->reg;
      m->File[FILE_LOCAL_PARAM] = program->Base.LocalParams;
      m->File[FILE_ENV_PARAM] = ctx->VertexProgram.Parameters;
      m->File[FILE_STATE_PARAM] = program->Parameters->ParameterValues;
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
