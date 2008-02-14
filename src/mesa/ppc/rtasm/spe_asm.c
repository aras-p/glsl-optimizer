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
 * \file spe_asm.c
 * Real-time assembly generation interface for Cell B.E. SPEs.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */
#ifdef GALLIUM_CELL
#include <inttypes.h>
#include <imports.h>
#include "spe_asm.h"

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

#include "spe_asm.h"


/*
 */
void spe_init_func(struct spe_function *p, unsigned code_size)
{
    p->store = _mesa_align_malloc(code_size, 16);
    p->csr = p->store;
}


void spe_release_func(struct spe_function *p)
{
    _mesa_align_free(p->store);
    p->store = NULL;
    p->csr = NULL;
}


void spu_bi(struct spe_function *p, unsigned rA, int d, int e)
{
    emit_RI7(p, 0x1a8, 0, rA, (d << 5) | (e << 4));
}

void spu_iret(struct spe_function *p, unsigned rA, int d, int e)
{
    emit_RI7(p, 0x1aa, 0, rA, (d << 5) | (e << 4));
}

void spu_bisled(struct spe_function *p, unsigned rT, unsigned rA, int d,
		int e)
{
    emit_RI7(p, 0x1ab, rT, rA, (d << 5) | (e << 4));
}

void spu_bisl(struct spe_function *p, unsigned rT, unsigned rA, int d,
		int e)
{
    emit_RI7(p, 0x1a9, rT, rA, (d << 5) | (e << 4));
}

void spu_biz(struct spe_function *p, unsigned rT, unsigned rA, int d,
		int e)
{
    emit_RI7(p, 0x128, rT, rA, (d << 5) | (e << 4));
}

void spu_binz(struct spe_function *p, unsigned rT, unsigned rA, int d, int e)
{
    emit_RI7(p, 0x129, rT, rA, (d << 5) | (e << 4));
}

void spu_bihz(struct spe_function *p, unsigned rT, unsigned rA, int d, int e)
{
    emit_RI7(p, 0x12a, rT, rA, (d << 5) | (e << 4));
}

void spu_bihnz(struct spe_function *p, unsigned rT, unsigned rA, int d, int e)
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
EMIT_RR  (spu_stopd, 0x140);
EMIT_    (spu_lnop,  0x001);
EMIT_    (spu_nop,   0x201);
sync;
EMIT_    (spu_dsync, 0x003);
EMIT_R   (spu_mfspr, 0x00c);
EMIT_R   (spu_mtspr, 0x10c);
#endif

#endif /* GALLIUM_CELL */
