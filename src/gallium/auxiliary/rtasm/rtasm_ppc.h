/**************************************************************************
 *
 * Copyright (C) 2008 Tungsten Graphics, Inc.   All Rights Reserved.
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
 *
 **************************************************************************/

/**
 * PPC code generation.
 * \author Brian Paul
 */


#ifndef RTASM_PPC_H
#define RTASM_PPC_H


#include "pipe/p_compiler.h"


#define PPC_INST_SIZE 4  /**< 4 bytes / instruction */

#define PPC_NUM_VEC_REGS 32


struct ppc_function
{
   uint32_t *store;  /**< instruction buffer */
   uint num_inst;
   uint max_inst;
   uint32_t vec_used;   /** used/free vector registers bitmask */
   uint32_t reg_used;   /** used/free general-purpose registers bitmask */
};



extern void ppc_init_func(struct ppc_function *p, unsigned max_inst);
extern void ppc_release_func(struct ppc_function *p);

extern int ppc_allocate_vec_register(struct ppc_function *p, int reg);
extern void ppc_release_vec_register(struct ppc_function *p, int reg);


/**
 ** float vector arithmetic
 **/

/** vector float add */
extern void
ppc_vaddfp(struct ppc_function *p,uint vD, uint vA, uint vB);

/** vector float substract */
extern void
ppc_vsubfp(struct ppc_function *p, uint vD, uint vA, uint vB);

/** vector float min */
extern void
ppc_vminfp(struct ppc_function *p, uint vD, uint vA, uint vB);

/** vector float max */
extern void
ppc_vmaxfp(struct ppc_function *p, uint vD, uint vA, uint vB);

/** vector float mult add */
extern void
ppc_vmaddfp(struct ppc_function *p, uint vD, uint vA, uint vB, uint vC);

/** vector float compare greater than */
extern void
ppc_vcmpgtfpx(struct ppc_function *p, uint vD, uint vA, uint vB);

/** vector float compare greater than or equal to */
extern void
ppc_vcmpgefpx(struct ppc_function *p, uint vD, uint vA, uint vB);

/** vector float compare equal */
extern void
ppc_vcmpeqfpx(struct ppc_function *p, uint vD, uint vA, uint vB);

/** vector float 2^x */
extern void
ppc_vexptefp(struct ppc_function *p, uint vD, uint vB);

/** vector float log2(x) */
extern void
ppc_vlogefp(struct ppc_function *p, uint vD, uint vB);

/** vector float reciprocol */
extern void
ppc_vrefp(struct ppc_function *p, uint vD, uint vB);

/** vector float reciprocol sqrt estimate */
extern void
ppc_vrsqrtefp(struct ppc_function *p, uint vD, uint vB);

/** vector float round to negative infinity */
extern void
ppc_vrfim(struct ppc_function *p, uint vD, uint vB);

/** vector float round to positive infinity */
extern void
ppc_vrfip(struct ppc_function *p, uint vD, uint vB);

/** vector float round to nearest int */
extern void
ppc_vrfin(struct ppc_function *p, uint vD, uint vB);

/** vector float round to int toward zero */
extern void
ppc_vrfiz(struct ppc_function *p, uint vD, uint vB);



/**
 ** bitwise operations
 **/


/** vector and */
extern void
ppc_vand(struct ppc_function *p, uint vD, uint vA, uint vB);

/** vector and complement */
extern void
ppc_vandc(struct ppc_function *p, uint vD, uint vA, uint vB);

/** vector or */
extern void
ppc_vor(struct ppc_function *p, uint vD, uint vA, uint vB);

/** vector nor */
extern void
ppc_vnor(struct ppc_function *p, uint vD, uint vA, uint vB);

/** vector xor */
extern void
ppc_vxor(struct ppc_function *p, uint vD, uint vA, uint vB);


/**
 ** Vector shuffle / select / splat / etc
 **/

/** vector permute */
extern void
ppc_vperm(struct ppc_function *p, uint vD, uint vA, uint vB, uint vC);

/** vector select */
extern void
ppc_vsel(struct ppc_function *p, uint vD, uint vA, uint vB, uint vC);

/** vector splat byte */
extern void
ppc_vspltb(struct ppc_function *p, uint vD, uint vB, uint imm);

/** vector splat half word */
extern void
ppc_vsplthw(struct ppc_function *p, uint vD, uint vB, uint imm);

/** vector splat word */
extern void
ppc_vspltw(struct ppc_function *p, uint vD, uint vB, uint imm);


#endif /* RTASM_PPC_H */
