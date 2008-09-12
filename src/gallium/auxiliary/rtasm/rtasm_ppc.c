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


#include "util/u_memory.h"
#include "pipe/p_debug.h"
#include "rtasm_ppc.h"


void
ppc_init_func(struct ppc_function *p, unsigned max_inst)
{
    p->store = align_malloc(max_inst * PPC_INST_SIZE, 16);
    p->num_inst = 0;
    p->max_inst = max_inst;
    p->vec_used = ~0;
}


void
ppc_release_func(struct ppc_function *p)
{
    assert(p->num_inst <= p->max_inst);
    if (p->store != NULL) {
        align_free(p->store);
    }
    p->store = NULL;
}


/**
 * Alloate a vector register.
 * \return register index or -1 if none left.
 */
int
ppc_allocate_vec_register(struct ppc_function *p, int reg)
{
   unsigned i;
   for (i = 0; i < PPC_NUM_VEC_REGS; i++) {
      const uint64_t mask = 1 << i;
      if ((p->vec_used & mask) != 0) {
         p->vec_used &= ~mask;
         return i;
      }
   }

   return -1;
}


/**
 * Mark the given vector register as "unallocated".
 */
void
ppc_release_vec_register(struct ppc_function *p, int reg)
{
   assert(reg < PPC_NUM_VEC_REGS);
   assert((p->vec_used & (1 << reg)) == 0);

   p->vec_used |= (1 << reg);
}



union vx_inst {
   uint32_t bits;
   struct {
      unsigned op:6;
      unsigned vD:5;
      unsigned vA:5;
      unsigned vB:5;
      unsigned op2:11;
   } inst;
};

union vxr_inst {
   uint32_t bits;
   struct {
      unsigned op:6;
      unsigned vD:5;
      unsigned vA:5;
      unsigned vB:5;
      unsigned rC:1;
      unsigned op2:10;
   } inst;
};

union va_inst {
   uint32_t bits;
   struct {
      unsigned op:6;
      unsigned vD:5;
      unsigned vA:5;
      unsigned vB:5;
      unsigned vC:5;
      unsigned op2:6;
   } inst;
};


static inline void
emit_vx(struct ppc_function *p, uint op2, uint vD, uint vA, uint vB)
{
   union vx_inst inst;
   inst.inst.op = 4;
   inst.inst.vD = vD;
   inst.inst.vA = vA;
   inst.inst.vB = vB;
   inst.inst.op2 = op2;
   p->store[p->num_inst++] = inst.bits;
   assert(p->num_inst <= p->max_inst);
};

static inline void
emit_vxr(struct ppc_function *p, uint op2, uint vD, uint vA, uint vB)
{
   union vxr_inst inst;
   inst.inst.op = 4;
   inst.inst.vD = vD;
   inst.inst.vA = vA;
   inst.inst.vB = vB;
   inst.inst.rC = 0;
   inst.inst.op2 = op2;
   p->store[p->num_inst++] = inst.bits;
   assert(p->num_inst <= p->max_inst);
};

static inline void
emit_va(struct ppc_function *p, uint op2, uint vD, uint vA, uint vB, uint vC)
{
   union va_inst inst;
   inst.inst.op = 4;
   inst.inst.vD = vD;
   inst.inst.vA = vA;
   inst.inst.vB = vB;
   inst.inst.vC = vC;
   inst.inst.op2 = op2;
   p->store[p->num_inst++] = inst.bits;
   assert(p->num_inst <= p->max_inst);
};



/**
 ** float vector arithmetic
 **/

/** vector float add */
void
ppc_vaddfp(struct ppc_function *p,uint vD, uint vA, uint vB)
{
   emit_vx(p, 10, vD, vA, vB);
}

/** vector float substract */
void
ppc_vsubfp(struct ppc_function *p, uint vD, uint vA, uint vB)
{
   emit_vx(p, 74, vD, vA, vB);
}

/** vector float min */
void
ppc_vminfp(struct ppc_function *p, uint vD, uint vA, uint vB)
{
   emit_vx(p, 1098, vD, vA, vB);
}

/** vector float max */
void
ppc_vmaxfp(struct ppc_function *p, uint vD, uint vA, uint vB)
{
   emit_vx(p, 1034, vD, vA, vB);
}

/** vector float mult add */
void
ppc_vmaddfp(struct ppc_function *p, uint vD, uint vA, uint vB, uint vC)
{
   emit_va(p, 46, vD, vA, vB, vC);
}

/** vector float compare greater than */
void
ppc_vcmpgtfpx(struct ppc_function *p, uint vD, uint vA, uint vB)
{
   emit_vxr(p, 710, vD, vA, vB);
}

/** vector float compare greater than or equal to */
void
ppc_vcmpgefpx(struct ppc_function *p, uint vD, uint vA, uint vB)
{
   emit_vxr(p, 454, vD, vA, vB);
}

/** vector float compare equal */
void
ppc_vcmpeqfpx(struct ppc_function *p, uint vD, uint vA, uint vB)
{
   emit_vxr(p, 198, vD, vA, vB);
}

/** vector float 2^x */
void
ppc_vexptefp(struct ppc_function *p, uint vD, uint vB)
{
   emit_vx(p, 394, vD, 0, vB);
}

/** vector float log2(x) */
void
ppc_vlogefp(struct ppc_function *p, uint vD, uint vB)
{
   emit_vx(p, 458, vD, 0, vB);
}

/** vector float reciprocol */
void
ppc_vrefp(struct ppc_function *p, uint vD, uint vB)
{
   emit_vx(p, 266, vD, 0, vB);
}

/** vector float reciprocol sqrt estimate */
void
ppc_vrsqrtefp(struct ppc_function *p, uint vD, uint vB)
{
   emit_vx(p, 330, vD, 0, vB);
}

/** vector float round to negative infinity */
void
ppc_vrfim(struct ppc_function *p, uint vD, uint vB)
{
   emit_vx(p, 714, vD, 0, vB);
}

/** vector float round to positive infinity */
void
ppc_vrfip(struct ppc_function *p, uint vD, uint vB)
{
   emit_vx(p, 650, vD, 0, vB);
}

/** vector float round to nearest int */
void
ppc_vrfin(struct ppc_function *p, uint vD, uint vB)
{
   emit_vx(p, 522, vD, 0, vB);
}

/** vector float round to int toward zero */
void
ppc_vrfiz(struct ppc_function *p, uint vD, uint vB)
{
   emit_vx(p, 586, vD, 0, vB);
}



/**
 ** bitwise operations
 **/


/** vector and */
void
ppc_vand(struct ppc_function *p, uint vD, uint vA, uint vB)
{
   emit_vx(p, 1028, vD, vA, vB);
}

/** vector and complement */
void
ppc_vandc(struct ppc_function *p, uint vD, uint vA, uint vB)
{
   emit_vx(p, 1092, vD, vA, vB);
}

/** vector or */
void
ppc_vor(struct ppc_function *p, uint vD, uint vA, uint vB)
{
   emit_vx(p, 1156, vD, vA, vB);
}

/** vector nor */
void
ppc_vnor(struct ppc_function *p, uint vD, uint vA, uint vB)
{
   emit_vx(p, 1284, vD, vA, vB);
}

/** vector xor */
void
ppc_vxor(struct ppc_function *p, uint vD, uint vA, uint vB)
{
   emit_vx(p, 1220, vD, vA, vB);
}


/**
 ** Vector shuffle / select / splat / etc
 **/

/** vector permute */
void
ppc_vperm(struct ppc_function *p, uint vD, uint vA, uint vB, uint vC)
{
   emit_va(p, 43, vD, vA, vB, vC);
}

/** vector select */
void
ppc_vsel(struct ppc_function *p, uint vD, uint vA, uint vB, uint vC)
{
   emit_va(p, 42, vD, vA, vB, vC);
}

/** vector splat byte */
void
ppc_vspltb(struct ppc_function *p, uint vD, uint vB, uint imm)
{
   emit_vx(p, 42, vD, imm, vB);
}

/** vector splat half word */
void
ppc_vsplthw(struct ppc_function *p, uint vD, uint vB, uint imm)
{
   emit_vx(p, 588, vD, imm, vB);
}

/** vector splat word */
void
ppc_vspltw(struct ppc_function *p, uint vD, uint vB, uint imm)
{
   emit_vx(p, 652, vD, imm, vB);
}
