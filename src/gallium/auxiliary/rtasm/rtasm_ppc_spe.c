/*
 * (C) Copyright IBM Corporation 2008
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * AUTHORS, COPYRIGHT HOLDERS, AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file
 * Real-time assembly generation interface for Cell B.E. SPEs.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include "pipe/p_compiler.h"
#include "pipe/p_util.h"
#include "rtasm_ppc_spe.h"

#ifdef GALLIUM_CELL
/**
 * SPE instruction types
 *
 * There are 6 primary instruction encodings used on the Cell's SPEs.  Each of
 * the following unions encodes one type.
 *
 * \bug
 * If, at some point, we start generating SPE code from a little-endian host
 * these unions will not work.
 */
/*@{*/
/**
 * Encode one output register with two input registers
 */
union spe_inst_RR {
    uint32_t bits;
    struct {
	unsigned op:11;
	unsigned rB:7;
	unsigned rA:7;
	unsigned rT:7;
    } inst;
};


/**
 * Encode one output register with three input registers
 */
union spe_inst_RRR {
    uint32_t bits;
    struct {
	unsigned op:4;
	unsigned rT:7;
	unsigned rB:7;
	unsigned rA:7;
	unsigned rC:7;
    } inst;
};


/**
 * Encode one output register with one input reg. and a 7-bit signed immed
 */
union spe_inst_RI7 {
    uint32_t bits;
    struct {
	unsigned op:11;
	unsigned i7:7;
	unsigned rA:7;
	unsigned rT:7;
    } inst;
};


/**
 * Encode one output register with one input reg. and an 8-bit signed immed
 */
union spe_inst_RI8 {
    uint32_t bits;
    struct {
	unsigned op:10;
	unsigned i8:8;
	unsigned rA:7;
	unsigned rT:7;
    } inst;
};


/**
 * Encode one output register with one input reg. and a 10-bit signed immed
 */
union spe_inst_RI10 {
    uint32_t bits;
    struct {
	unsigned op:8;
	unsigned i10:10;
	unsigned rA:7;
	unsigned rT:7;
    } inst;
};


/**
 * Encode one output register with a 16-bit signed immediate
 */
union spe_inst_RI16 {
    uint32_t bits;
    struct {
	unsigned op:9;
	unsigned i16:16;
	unsigned rT:7;
    } inst;
};


/**
 * Encode one output register with a 18-bit signed immediate
 */
union spe_inst_RI18 {
    uint32_t bits;
    struct {
	unsigned op:7;
	unsigned i18:18;
	unsigned rT:7;
    } inst;
};
/*@}*/


static void emit_RR(struct spe_function *p, unsigned op, unsigned rT,
		    unsigned rA, unsigned rB)
{
    union spe_inst_RR inst;
    inst.inst.op = op;
    inst.inst.rB = rB;
    inst.inst.rA = rA;
    inst.inst.rT = rT;
    *p->csr = inst.bits;
    p->csr++;
}


static void emit_RRR(struct spe_function *p, unsigned op, unsigned rT,
		    unsigned rA, unsigned rB, unsigned rC)
{
    union spe_inst_RRR inst;
    inst.inst.op = op;
    inst.inst.rT = rT;
    inst.inst.rB = rB;
    inst.inst.rA = rA;
    inst.inst.rC = rC;
    *p->csr = inst.bits;
    p->csr++;
}


static void emit_RI7(struct spe_function *p, unsigned op, unsigned rT,
		     unsigned rA, int imm)
{
    union spe_inst_RI7 inst;
    inst.inst.op = op;
    inst.inst.i7 = imm;
    inst.inst.rA = rA;
    inst.inst.rT = rT;
    *p->csr = inst.bits;
    p->csr++;
}



static void emit_RI8(struct spe_function *p, unsigned op, unsigned rT,
		     unsigned rA, int imm)
{
    union spe_inst_RI8 inst;
    inst.inst.op = op;
    inst.inst.i8 = imm;
    inst.inst.rA = rA;
    inst.inst.rT = rT;
    *p->csr = inst.bits;
    p->csr++;
}



static void emit_RI10(struct spe_function *p, unsigned op, unsigned rT,
		      unsigned rA, int imm)
{
    union spe_inst_RI10 inst;
    inst.inst.op = op;
    inst.inst.i10 = imm;
    inst.inst.rA = rA;
    inst.inst.rT = rT;
    *p->csr = inst.bits;
    p->csr++;
}


static void emit_RI16(struct spe_function *p, unsigned op, unsigned rT,
		      int imm)
{
    union spe_inst_RI16 inst;
    inst.inst.op = op;
    inst.inst.i16 = imm;
    inst.inst.rT = rT;
    *p->csr = inst.bits;
    p->csr++;
}


static void emit_RI18(struct spe_function *p, unsigned op, unsigned rT,
		      int imm)
{
    union spe_inst_RI18 inst;
    inst.inst.op = op;
    inst.inst.i18 = imm;
    inst.inst.rT = rT;
    *p->csr = inst.bits;
    p->csr++;
}




#define EMIT_(_name, _op) \
void _name (struct spe_function *p, unsigned rT) \
{ \
    emit_RR(p, _op, rT, 0, 0); \
}

#define EMIT_R(_name, _op) \
void _name (struct spe_function *p, unsigned rT, unsigned rA) \
{ \
    emit_RR(p, _op, rT, rA, 0); \
}

#define EMIT_RR(_name, _op) \
void _name (struct spe_function *p, unsigned rT, unsigned rA, unsigned rB) \
{ \
    emit_RR(p, _op, rT, rA, rB); \
}

#define EMIT_RRR(_name, _op) \
void _name (struct spe_function *p, unsigned rT, unsigned rA, unsigned rB, unsigned rC) \
{ \
    emit_RRR(p, _op, rT, rA, rB, rC); \
}

#define EMIT_RI7(_name, _op) \
void _name (struct spe_function *p, unsigned rT, unsigned rA, int imm) \
{ \
    emit_RI7(p, _op, rT, rA, imm); \
}

#define EMIT_RI8(_name, _op) \
void _name (struct spe_function *p, unsigned rT, unsigned rA, int imm) \
{ \
    emit_RI8(p, _op, rT, rA, 155 - imm); \
}

#define EMIT_RI10(_name, _op) \
void _name (struct spe_function *p, unsigned rT, unsigned rA, int imm) \
{ \
    emit_RI10(p, _op, rT, rA, imm); \
}

#define EMIT_RI16(_name, _op) \
void _name (struct spe_function *p, unsigned rT, int imm) \
{ \
    emit_RI16(p, _op, rT, imm); \
}

#define EMIT_RI18(_name, _op) \
void _name (struct spe_function *p, unsigned rT, int imm) \
{ \
    emit_RI18(p, _op, rT, imm); \
}

#define EMIT_I16(_name, _op) \
void _name (struct spe_function *p, int imm) \
{ \
    emit_RI16(p, _op, 0, imm); \
}

#include "rtasm_ppc_spe.h"


/*
 */
void spe_init_func(struct spe_function *p, unsigned code_size)
{
    p->store = align_malloc(code_size, 16);
    p->csr = p->store;
    
    /* Conservatively treat R0 - R2 and R80 - R127 as non-volatile.
     */
    p->regs[0] = ~7;
    p->regs[1] = (1U << (80 - 64)) - 1;
}


void spe_release_func(struct spe_function *p)
{
    if (p->store != NULL) {
        align_free(p->store);
    }
    p->store = NULL;
    p->csr = NULL;
}


int spe_allocate_available_register(struct spe_function *p)
{
   unsigned i;
   for (i = 0; i < 128; i++) {
      const uint64_t mask = (1ULL << (i % 64));
      const unsigned idx = i / 64;

      if ((p->regs[idx] & mask) != 0) {
         p->regs[idx] &= ~mask;
         return i;
      }
   }

   return -1;
}


int spe_allocate_register(struct spe_function *p, int reg)
{
   const unsigned idx = reg / 64;
   const unsigned bit = reg % 64;

   assert((p->regs[idx] & (1ULL << bit)) != 0);

   p->regs[idx] &= ~(1ULL << bit);
   return reg;
}


void spe_release_register(struct spe_function *p, int reg)
{
   const unsigned idx = reg / 64;
   const unsigned bit = reg % 64;

   assert((p->regs[idx] & (1ULL << bit)) == 0);

   p->regs[idx] |= (1ULL << bit);
}




void spe_bi(struct spe_function *p, unsigned rA, int d, int e)
{
    emit_RI7(p, 0x1a8, 0, rA, (d << 5) | (e << 4));
}

void spe_iret(struct spe_function *p, unsigned rA, int d, int e)
{
    emit_RI7(p, 0x1aa, 0, rA, (d << 5) | (e << 4));
}

void spe_bisled(struct spe_function *p, unsigned rT, unsigned rA, int d,
		int e)
{
    emit_RI7(p, 0x1ab, rT, rA, (d << 5) | (e << 4));
}

void spe_bisl(struct spe_function *p, unsigned rT, unsigned rA, int d,
		int e)
{
    emit_RI7(p, 0x1a9, rT, rA, (d << 5) | (e << 4));
}

void spe_biz(struct spe_function *p, unsigned rT, unsigned rA, int d,
		int e)
{
    emit_RI7(p, 0x128, rT, rA, (d << 5) | (e << 4));
}

void spe_binz(struct spe_function *p, unsigned rT, unsigned rA, int d, int e)
{
    emit_RI7(p, 0x129, rT, rA, (d << 5) | (e << 4));
}

void spe_bihz(struct spe_function *p, unsigned rT, unsigned rA, int d, int e)
{
    emit_RI7(p, 0x12a, rT, rA, (d << 5) | (e << 4));
}

void spe_bihnz(struct spe_function *p, unsigned rT, unsigned rA, int d, int e)
{
    emit_RI7(p, 0x12b, rT, rA, (d << 5) | (e << 4));
}


/* Hint-for-branch instructions
 */
#if 0
hbr;
hbra;
hbrr;
#endif


/* Control instructions
 */
#if 0
stop;
EMIT_RR  (spe_stopd, 0x140);
EMIT_    (spe_lnop,  0x001);
EMIT_    (spe_nop,   0x201);
sync;
EMIT_    (spe_dsync, 0x003);
EMIT_R   (spe_mfspr, 0x00c);
EMIT_R   (spe_mtspr, 0x10c);
#endif

#endif /* GALLIUM_CELL */
