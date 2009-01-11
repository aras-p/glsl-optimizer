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
 * \author Brian Paul
 */


#include <stdio.h>
#include "pipe/p_compiler.h"
#include "util/u_memory.h"
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


static void
indent(const struct spe_function *p)
{
   int i;
   for (i = 0; i < p->indent; i++) {
      putchar(' ');
   }
}


static const char *
rem_prefix(const char *longname)
{
   return longname + 4;
}


static const char *
reg_name(int reg)
{
   switch (reg) {
   case SPE_REG_SP:
      return "$sp";
   case SPE_REG_RA:
      return "$lr";
   default:
      {
         /* cycle through four buffers to handle multiple calls per printf */
         static char buf[4][10];
         static int b = 0;
         b = (b + 1) % 4;
         sprintf(buf[b], "$%d", reg);
         return buf[b];
      }
   }
}


static void
emit_instruction(struct spe_function *p, uint32_t inst_bits)
{
   if (!p->store)
      return;  /* out of memory, drop the instruction */

   if (p->num_inst == p->max_inst) {
      /* allocate larger buffer */
      uint32_t *newbuf;
      p->max_inst *= 2;  /* 2x larger */
      newbuf = align_malloc(p->max_inst * SPE_INST_SIZE, 16);
      if (newbuf) {
         memcpy(newbuf, p->store, p->num_inst * SPE_INST_SIZE);
      }
      align_free(p->store);
      p->store = newbuf;
      if (!p->store) {
         /* out of memory */
         p->num_inst = 0;
         return;
      }
   }

   p->store[p->num_inst++] = inst_bits;
}



static void emit_RR(struct spe_function *p, unsigned op, int rT,
		    int rA, int rB, const char *name)
{
    union spe_inst_RR inst;
    inst.inst.op = op;
    inst.inst.rB = rB;
    inst.inst.rA = rA;
    inst.inst.rT = rT;
    emit_instruction(p, inst.bits);
    if (p->print) {
       indent(p);
       printf("%s\t%s, %s, %s\n",
              rem_prefix(name), reg_name(rT), reg_name(rA), reg_name(rB));
    }
}


static void emit_RRR(struct spe_function *p, unsigned op, int rT,
                     int rA, int rB, int rC, const char *name)
{
    union spe_inst_RRR inst;
    inst.inst.op = op;
    inst.inst.rT = rT;
    inst.inst.rB = rB;
    inst.inst.rA = rA;
    inst.inst.rC = rC;
    emit_instruction(p, inst.bits);
    if (p->print) {
       indent(p);
       printf("%s\t%s, %s, %s, %s\n", rem_prefix(name), reg_name(rT),
              reg_name(rA), reg_name(rB), reg_name(rC));
    }
}


static void emit_RI7(struct spe_function *p, unsigned op, int rT,
		     int rA, int imm, const char *name)
{
    union spe_inst_RI7 inst;
    inst.inst.op = op;
    inst.inst.i7 = imm;
    inst.inst.rA = rA;
    inst.inst.rT = rT;
    emit_instruction(p, inst.bits);
    if (p->print) {
       indent(p);
       printf("%s\t%s, %s, 0x%x\n",
              rem_prefix(name), reg_name(rT), reg_name(rA), imm);
    }
}



static void emit_RI8(struct spe_function *p, unsigned op, int rT,
		     int rA, int imm, const char *name)
{
    union spe_inst_RI8 inst;
    inst.inst.op = op;
    inst.inst.i8 = imm;
    inst.inst.rA = rA;
    inst.inst.rT = rT;
    emit_instruction(p, inst.bits);
    if (p->print) {
       indent(p);
       printf("%s\t%s, %s, 0x%x\n",
              rem_prefix(name), reg_name(rT), reg_name(rA), imm);
    }
}



static void emit_RI10(struct spe_function *p, unsigned op, int rT,
		      int rA, int imm, const char *name)
{
    union spe_inst_RI10 inst;
    inst.inst.op = op;
    inst.inst.i10 = imm;
    inst.inst.rA = rA;
    inst.inst.rT = rT;
    emit_instruction(p, inst.bits);
    if (p->print) {
       indent(p);
       printf("%s\t%s, %s, 0x%x\n",
              rem_prefix(name), reg_name(rT), reg_name(rA), imm);
    }
}


/** As above, but do range checking on signed immediate value */
static void emit_RI10s(struct spe_function *p, unsigned op, int rT,
                       int rA, int imm, const char *name)
{
    assert(imm <= 511);
    assert(imm >= -512);
    emit_RI10(p, op, rT, rA, imm, name);
}


static void emit_RI16(struct spe_function *p, unsigned op, int rT,
		      int imm, const char *name)
{
    union spe_inst_RI16 inst;
    inst.inst.op = op;
    inst.inst.i16 = imm;
    inst.inst.rT = rT;
    emit_instruction(p, inst.bits);
    if (p->print) {
       indent(p);
       printf("%s\t%s, 0x%x\n", rem_prefix(name), reg_name(rT), imm);
    }
}


static void emit_RI18(struct spe_function *p, unsigned op, int rT,
		      int imm, const char *name)
{
    union spe_inst_RI18 inst;
    inst.inst.op = op;
    inst.inst.i18 = imm;
    inst.inst.rT = rT;
    emit_instruction(p, inst.bits);
    if (p->print) {
       indent(p);
       printf("%s\t%s, 0x%x\n", rem_prefix(name), reg_name(rT), imm);
    }
}


#define EMIT(_name, _op) \
void _name (struct spe_function *p) \
{ \
   emit_RR(p, _op, 0, 0, 0, __FUNCTION__); \
}

#define EMIT_(_name, _op) \
void _name (struct spe_function *p, int rT) \
{ \
   emit_RR(p, _op, rT, 0, 0, __FUNCTION__); \
}

#define EMIT_R(_name, _op) \
void _name (struct spe_function *p, int rT, int rA) \
{ \
   emit_RR(p, _op, rT, rA, 0, __FUNCTION__);                 \
}

#define EMIT_RR(_name, _op) \
void _name (struct spe_function *p, int rT, int rA, int rB) \
{ \
   emit_RR(p, _op, rT, rA, rB, __FUNCTION__);                \
}

#define EMIT_RRR(_name, _op) \
void _name (struct spe_function *p, int rT, int rA, int rB, int rC) \
{ \
   emit_RRR(p, _op, rT, rA, rB, rC, __FUNCTION__);           \
}

#define EMIT_RI7(_name, _op) \
void _name (struct spe_function *p, int rT, int rA, int imm) \
{ \
   emit_RI7(p, _op, rT, rA, imm, __FUNCTION__);              \
}

#define EMIT_RI8(_name, _op, bias) \
void _name (struct spe_function *p, int rT, int rA, int imm) \
{ \
   emit_RI8(p, _op, rT, rA, bias - imm, __FUNCTION__);       \
}

#define EMIT_RI10(_name, _op) \
void _name (struct spe_function *p, int rT, int rA, int imm) \
{ \
   emit_RI10(p, _op, rT, rA, imm, __FUNCTION__);             \
}

#define EMIT_RI10s(_name, _op) \
void _name (struct spe_function *p, int rT, int rA, int imm) \
{ \
   emit_RI10s(p, _op, rT, rA, imm, __FUNCTION__);             \
}

#define EMIT_RI16(_name, _op) \
void _name (struct spe_function *p, int rT, int imm) \
{ \
   emit_RI16(p, _op, rT, imm, __FUNCTION__);                 \
}

#define EMIT_RI18(_name, _op) \
void _name (struct spe_function *p, int rT, int imm) \
{ \
   emit_RI18(p, _op, rT, imm, __FUNCTION__);                 \
}

#define EMIT_I16(_name, _op) \
void _name (struct spe_function *p, int imm) \
{ \
   emit_RI16(p, _op, 0, imm, __FUNCTION__);                  \
}

#include "rtasm_ppc_spe.h"



/**
 * Initialize an spe_function.
 * \param code_size  initial size of instruction buffer to allocate, in bytes.
 *                   If zero, use a default.
 */
void spe_init_func(struct spe_function *p, unsigned code_size)
{
    uint i;

    if (!code_size)
       code_size = 64;

    p->num_inst = 0;
    p->max_inst = code_size / SPE_INST_SIZE;
    p->store = align_malloc(code_size, 16);

    p->set_count = 0;
    memset(p->regs, 0, SPE_NUM_REGS * sizeof(p->regs[0]));

    /* Conservatively treat R0 - R2 and R80 - R127 as non-volatile.
     */
    p->regs[0] = p->regs[1] = p->regs[2] = 1;
    for (i = 80; i <= 127; i++) {
      p->regs[i] = 1;
    }

    p->print = FALSE;
    p->indent = 0;
}


void spe_release_func(struct spe_function *p)
{
    assert(p->num_inst <= p->max_inst);
    if (p->store != NULL) {
        align_free(p->store);
    }
    p->store = NULL;
}


/** Return current code size in bytes. */
unsigned spe_code_size(const struct spe_function *p)
{
   return p->num_inst * SPE_INST_SIZE;
}


/**
 * Allocate a SPE register.
 * \return register index or -1 if none left.
 */
int spe_allocate_available_register(struct spe_function *p)
{
   unsigned i;
   for (i = 0; i < SPE_NUM_REGS; i++) {
      if (p->regs[i] == 0) {
         p->regs[i] = 1;
         return i;
      }
   }

   return -1;
}


/**
 * Mark the given SPE register as "allocated".
 */
int spe_allocate_register(struct spe_function *p, int reg)
{
   assert(reg < SPE_NUM_REGS);
   assert(p->regs[reg] == 0);
   p->regs[reg] = 1;
   return reg;
}


/**
 * Mark the given SPE register as "unallocated".  Note that this should
 * only be used on registers allocated in the current register set; an
 * assertion will fail if an attempt is made to deallocate a register
 * allocated in an earlier register set.
 */
void spe_release_register(struct spe_function *p, int reg)
{
   assert(reg >= 0);
   assert(reg < SPE_NUM_REGS);
   assert(p->regs[reg] == 1);

   p->regs[reg] = 0;
}

/**
 * Start a new set of registers.  This can be called if
 * it will be difficult later to determine exactly what
 * registers were actually allocated during a code generation
 * sequence, and you really just want to deallocate all of them.
 */
void spe_allocate_register_set(struct spe_function *p)
{
   uint i;

   /* Keep track of the set count.  If it ever wraps around to 0, 
    * we're in trouble.
    */
   p->set_count++;
   assert(p->set_count > 0);

   /* Increment the allocation count of all registers currently
    * allocated.  Then any registers that are allocated in this set
    * will be the only ones with a count of 1; they'll all be released
    * when the register set is released.
    */
   for (i = 0; i < SPE_NUM_REGS; i++) {
      if (p->regs[i] > 0)
         p->regs[i]++;
   }
}

void spe_release_register_set(struct spe_function *p)
{
   uint i;

   /* If the set count drops below zero, we're in trouble. */
   assert(p->set_count > 0);
   p->set_count--;

   /* Drop the allocation level of all registers.  Any allocated
    * during this register set will drop to 0 and then become
    * available.
    */
   for (i = 0; i < SPE_NUM_REGS; i++) {
      if (p->regs[i] > 0)
         p->regs[i]--;
   }
}


unsigned
spe_get_registers_used(const struct spe_function *p, ubyte used[])
{
   unsigned i, num = 0;
   /* only count registers in the range available to callers */
   for (i = 2; i < 80; i++) {
      if (p->regs[i]) {
         used[num++] = i;
      }
   }
   return num;
}


void
spe_print_code(struct spe_function *p, boolean enable)
{
   p->print = enable;
}


void
spe_indent(struct spe_function *p, int spaces)
{
   p->indent += spaces;
}


void
spe_comment(struct spe_function *p, int rel_indent, const char *s)
{
   if (p->print) {
      p->indent += rel_indent;
      indent(p);
      p->indent -= rel_indent;
      printf("# %s\n", s);
   }
}


/**
 * Load quad word.
 * NOTE: offset is in bytes and the least significant 4 bits must be zero!
 */
void spe_lqd(struct spe_function *p, int rT, int rA, int offset)
{
   const boolean pSave = p->print;

   /* offset must be a multiple of 16 */
   assert(offset % 16 == 0);
   /* offset must fit in 10-bit signed int field, after shifting */
   assert((offset >> 4) <= 511);
   assert((offset >> 4) >= -512);

   p->print = FALSE;
   emit_RI10(p, 0x034, rT, rA, offset >> 4, "spe_lqd");
   p->print = pSave;

   if (p->print) {
      indent(p);
      printf("lqd\t%s, %d(%s)\n", reg_name(rT), offset, reg_name(rA));
   }
}


/**
 * Store quad word.
 * NOTE: offset is in bytes and the least significant 4 bits must be zero!
 */
void spe_stqd(struct spe_function *p, int rT, int rA, int offset)
{
   const boolean pSave = p->print;

   /* offset must be a multiple of 16 */
   assert(offset % 16 == 0);
   /* offset must fit in 10-bit signed int field, after shifting */
   assert((offset >> 4) <= 511);
   assert((offset >> 4) >= -512);

   p->print = FALSE;
   emit_RI10(p, 0x024, rT, rA, offset >> 4, "spe_stqd");
   p->print = pSave;

   if (p->print) {
      indent(p);
      printf("stqd\t%s, %d(%s)\n", reg_name(rT), offset, reg_name(rA));
   }
}


/**
 * For branch instructions:
 * \param d  if 1, disable interupts if branch is taken
 * \param e  if 1, enable interupts if branch is taken
 * If d and e are both zero, don't change interupt status (right?)
 */

/** Branch Indirect to address in rA */
void spe_bi(struct spe_function *p, int rA, int d, int e)
{
   emit_RI7(p, 0x1a8, 0, rA, (d << 5) | (e << 4), __FUNCTION__);
}

/** Interupt Return */
void spe_iret(struct spe_function *p, int rA, int d, int e)
{
   emit_RI7(p, 0x1aa, 0, rA, (d << 5) | (e << 4), __FUNCTION__);
}

/** Branch indirect and set link on external data */
void spe_bisled(struct spe_function *p, int rT, int rA, int d,
		int e)
{
   emit_RI7(p, 0x1ab, rT, rA, (d << 5) | (e << 4), __FUNCTION__);
}

/** Branch indirect and set link.  Save PC in rT, jump to rA. */
void spe_bisl(struct spe_function *p, int rT, int rA, int d,
		int e)
{
   emit_RI7(p, 0x1a9, rT, rA, (d << 5) | (e << 4), __FUNCTION__);
}

/** Branch indirect if zero word.  If rT.word[0]==0, jump to rA. */
void spe_biz(struct spe_function *p, int rT, int rA, int d, int e)
{
   emit_RI7(p, 0x128, rT, rA, (d << 5) | (e << 4), __FUNCTION__);
}

/** Branch indirect if non-zero word.  If rT.word[0]!=0, jump to rA. */
void spe_binz(struct spe_function *p, int rT, int rA, int d, int e)
{
   emit_RI7(p, 0x129, rT, rA, (d << 5) | (e << 4), __FUNCTION__);
}

/** Branch indirect if zero halfword.  If rT.halfword[1]==0, jump to rA. */
void spe_bihz(struct spe_function *p, int rT, int rA, int d, int e)
{
   emit_RI7(p, 0x12a, rT, rA, (d << 5) | (e << 4), __FUNCTION__);
}

/** Branch indirect if non-zero halfword.  If rT.halfword[1]!=0, jump to rA. */
void spe_bihnz(struct spe_function *p, int rT, int rA, int d, int e)
{
   emit_RI7(p, 0x12b, rT, rA, (d << 5) | (e << 4), __FUNCTION__);
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
EMIT_    (spe_nop,   0x201);
sync;
EMIT_    (spe_dsync, 0x003);
EMIT_R   (spe_mfspr, 0x00c);
EMIT_R   (spe_mtspr, 0x10c);
#endif


/**
 ** Helper / "macro" instructions.
 ** Use somewhat verbose names as a reminder that these aren't native
 ** SPE instructions.
 **/


void
spe_load_float(struct spe_function *p, int rT, float x)
{
   if (x == 0.0f) {
      spe_il(p, rT, 0x0);
   }
   else if (x == 0.5f) {
      spe_ilhu(p, rT, 0x3f00);
   }
   else if (x == 1.0f) {
      spe_ilhu(p, rT, 0x3f80);
   }
   else if (x == -1.0f) {
      spe_ilhu(p, rT, 0xbf80);
   }
   else {
      union {
         float f;
         unsigned u;
      } bits;
      bits.f = x;
      spe_ilhu(p, rT, bits.u >> 16);
      spe_iohl(p, rT, bits.u & 0xffff);
   }
}


void
spe_load_int(struct spe_function *p, int rT, int i)
{
   if (-32768 <= i && i <= 32767) {
      spe_il(p, rT, i);
   }
   else {
      spe_ilhu(p, rT, i >> 16);
      if (i & 0xffff)
         spe_iohl(p, rT, i & 0xffff);
   }
}

void spe_load_uint(struct spe_function *p, int rT, uint ui)
{
   /* If the whole value is in the lower 18 bits, use ila, which
    * doesn't sign-extend.  Otherwise, if the two halfwords of
    * the constant are identical, use ilh.  Otherwise, if every byte of
    * the desired value is 0x00 or 0xff, we can use Form Select Mask for
    * Bytes Immediate (fsmbi) to load the value in a single instruction.
    * Otherwise, in the general case, we have to use ilhu followed by iohl.
    */
   if ((ui & 0x0003ffff) == ui) {
      spe_ila(p, rT, ui);
   }
   else if ((ui >> 16) == (ui & 0xffff)) {
      spe_ilh(p, rT, ui & 0xffff);
   }
   else if (
      ((ui & 0x000000ff) == 0 || (ui & 0x000000ff) == 0x000000ff) &&
      ((ui & 0x0000ff00) == 0 || (ui & 0x0000ff00) == 0x0000ff00) &&
      ((ui & 0x00ff0000) == 0 || (ui & 0x00ff0000) == 0x00ff0000) &&
      ((ui & 0xff000000) == 0 || (ui & 0xff000000) == 0xff000000)
   ) {
      uint mask = 0;
      /* fsmbi duplicates each bit in the given mask eight times,
       * using a 16-bit value to initialize a 16-byte quadword.
       * Each 4-bit nybble of the mask corresponds to a full word
       * of the result; look at the value and figure out the mask
       * (replicated for each word in the quadword), and then
       * form the "select mask" to get the value.
       */
      if ((ui & 0x000000ff) == 0x000000ff) mask |= 0x1111;
      if ((ui & 0x0000ff00) == 0x0000ff00) mask |= 0x2222;
      if ((ui & 0x00ff0000) == 0x00ff0000) mask |= 0x4444;
      if ((ui & 0xff000000) == 0xff000000) mask |= 0x8888;
      spe_fsmbi(p, rT, mask);
   }
   else {
      /* The general case: this usually uses two instructions, but
       * may use only one if the low-order 16 bits of each word are 0.
       */
      spe_ilhu(p, rT, ui >> 16);
      if (ui & 0xffff)
         spe_iohl(p, rT, ui & 0xffff);
   }
}

/**
 * This function is constructed identically to spe_xor_uint() below.
 * Changes to one should be made in the other.
 */
void
spe_and_uint(struct spe_function *p, int rT, int rA, uint ui)
{
   /* If we can, emit a single instruction, either And Byte Immediate
    * (which uses the same constant across each byte), And Halfword Immediate
    * (which sign-extends a 10-bit immediate to 16 bits and uses that
    * across each halfword), or And Word Immediate (which sign-extends
    * a 10-bit immediate to 32 bits).
    *
    * Otherwise, we'll need to use a temporary register.
    */
   uint tmp;

   /* If the upper 23 bits are all 0s or all 1s, sign extension
    * will work and we can use And Word Immediate
    */
   tmp = ui & 0xfffffe00;
   if (tmp == 0xfffffe00 || tmp  == 0) {
      spe_andi(p, rT, rA, ui & 0x000003ff);
      return;
   }
   
   /* If the ui field is symmetric along halfword boundaries and
    * the upper 7 bits of each halfword are all 0s or 1s, we
    * can use And Halfword Immediate
    */
   tmp = ui & 0xfe00fe00;
   if ((tmp == 0xfe00fe00 || tmp == 0) && ((ui >> 16) == (ui & 0x0000ffff))) {
      spe_andhi(p, rT, rA, ui & 0x000003ff);
      return;
   }

   /* If the ui field is symmetric in each byte, then we can use
    * the And Byte Immediate instruction.
    */
   tmp = ui & 0x000000ff;
   if ((ui >> 24) == tmp && ((ui >> 16) & 0xff) == tmp && ((ui >> 8) & 0xff) == tmp) {
      spe_andbi(p, rT, rA, tmp);
      return;
   }

   /* Otherwise, we'll have to use a temporary register. */
   int tmp_reg = spe_allocate_available_register(p);
   spe_load_uint(p, tmp_reg, ui);
   spe_and(p, rT, rA, tmp_reg);
   spe_release_register(p, tmp_reg);
}


/**
 * This function is constructed identically to spe_and_uint() above.
 * Changes to one should be made in the other.
 */
void
spe_xor_uint(struct spe_function *p, int rT, int rA, uint ui)
{
   /* If we can, emit a single instruction, either Exclusive Or Byte 
    * Immediate (which uses the same constant across each byte), Exclusive 
    * Or Halfword Immediate (which sign-extends a 10-bit immediate to 
    * 16 bits and uses that across each halfword), or Exclusive Or Word 
    * Immediate (which sign-extends a 10-bit immediate to 32 bits).
    *
    * Otherwise, we'll need to use a temporary register.
    */
   uint tmp;

   /* If the upper 23 bits are all 0s or all 1s, sign extension
    * will work and we can use Exclusive Or Word Immediate
    */
   tmp = ui & 0xfffffe00;
   if (tmp == 0xfffffe00 || tmp  == 0) {
      spe_xori(p, rT, rA, ui & 0x000003ff);
      return;
   }
   
   /* If the ui field is symmetric along halfword boundaries and
    * the upper 7 bits of each halfword are all 0s or 1s, we
    * can use Exclusive Or Halfword Immediate
    */
   tmp = ui & 0xfe00fe00;
   if ((tmp == 0xfe00fe00 || tmp == 0) && ((ui >> 16) == (ui & 0x0000ffff))) {
      spe_xorhi(p, rT, rA, ui & 0x000003ff);
      return;
   }

   /* If the ui field is symmetric in each byte, then we can use
    * the Exclusive Or Byte Immediate instruction.
    */
   tmp = ui & 0x000000ff;
   if ((ui >> 24) == tmp && ((ui >> 16) & 0xff) == tmp && ((ui >> 8) & 0xff) == tmp) {
      spe_xorbi(p, rT, rA, tmp);
      return;
   }

   /* Otherwise, we'll have to use a temporary register. */
   int tmp_reg = spe_allocate_available_register(p);
   spe_load_uint(p, tmp_reg, ui);
   spe_xor(p, rT, rA, tmp_reg);
   spe_release_register(p, tmp_reg);
}

void
spe_compare_equal_uint(struct spe_function *p, int rT, int rA, uint ui)
{
   /* If the comparison value is 9 bits or less, it fits inside a
    * Compare Equal Word Immediate instruction.
    */
   if ((ui & 0x000001ff) == ui) {
      spe_ceqi(p, rT, rA, ui);
   }
   /* Otherwise, we're going to have to load a word first. */
   else {
      int tmp_reg = spe_allocate_available_register(p);
      spe_load_uint(p, tmp_reg, ui);
      spe_ceq(p, rT, rA, tmp_reg);
      spe_release_register(p, tmp_reg);
   }
}

void
spe_compare_greater_uint(struct spe_function *p, int rT, int rA, uint ui)
{
   /* If the comparison value is 10 bits or less, it fits inside a
    * Compare Logical Greater Than Word Immediate instruction.
    */
   if ((ui & 0x000003ff) == ui) {
      spe_clgti(p, rT, rA, ui);
   }
   /* Otherwise, we're going to have to load a word first. */
   else {
      int tmp_reg = spe_allocate_available_register(p);
      spe_load_uint(p, tmp_reg, ui);
      spe_clgt(p, rT, rA, tmp_reg);
      spe_release_register(p, tmp_reg);
   }
}

void
spe_splat(struct spe_function *p, int rT, int rA)
{
   /* Use a temporary, just in case rT == rA */
   int tmp_reg = spe_allocate_available_register(p);
   /* Duplicate bytes 0, 1, 2, and 3 across the whole register */
   spe_ila(p, tmp_reg, 0x00010203);
   spe_shufb(p, rT, rA, rA, tmp_reg);
   spe_release_register(p, tmp_reg);
}


void
spe_complement(struct spe_function *p, int rT, int rA)
{
   spe_nor(p, rT, rA, rA);
}


void
spe_move(struct spe_function *p, int rT, int rA)
{
   /* Use different instructions depending on the instruction address
    * to take advantage of the dual pipelines.
    */
   if (p->num_inst & 1)
      spe_shlqbyi(p, rT, rA, 0);  /* odd pipe */
   else
      spe_ori(p, rT, rA, 0);  /* even pipe */
}


void
spe_zero(struct spe_function *p, int rT)
{
   spe_xor(p, rT, rT, rT);
}


void
spe_splat_word(struct spe_function *p, int rT, int rA, int word)
{
   assert(word >= 0);
   assert(word <= 3);

   if (word == 0) {
      int tmp1 = rT;
      spe_ila(p, tmp1, 66051);
      spe_shufb(p, rT, rA, rA, tmp1);
   }
   else {
      /* XXX review this, we may not need the rotqbyi instruction */
      int tmp1 = rT;
      int tmp2 = spe_allocate_available_register(p);

      spe_ila(p, tmp1, 66051);
      spe_rotqbyi(p, tmp2, rA, 4 * word);
      spe_shufb(p, rT, tmp2, tmp2, tmp1);

      spe_release_register(p, tmp2);
   }
}

/**
 * For each 32-bit float element of rA and rB, choose the smaller of the
 * two, compositing them into the rT register.
 * 
 * The Float Compare Greater Than (fcgt) instruction will put 1s into
 * compare_reg where rA > rB, and 0s where rA <= rB.
 *
 * Then the Select Bits (selb) instruction will take bits from rA where
 * compare_reg is 0, and from rB where compare_reg is 1; i.e., from rA
 * where rA <= rB and from rB where rB > rA, which is exactly the
 * "min" operation.
 *
 * The compare_reg could in many cases be the same as rT, unless
 * rT == rA || rt == rB.  But since this is common in constructions
 * like "x = min(x, a)", we always allocate a new register to be safe.
 */
void 
spe_float_min(struct spe_function *p, int rT, int rA, int rB)
{
   int compare_reg = spe_allocate_available_register(p);
   spe_fcgt(p, compare_reg, rA, rB);
   spe_selb(p, rT, rA, rB, compare_reg);
   spe_release_register(p, compare_reg);
}

/**
 * For each 32-bit float element of rA and rB, choose the greater of the
 * two, compositing them into the rT register.
 * 
 * The logic is similar to that of spe_float_min() above; the only
 * difference is that the registers on spe_selb() have been reversed,
 * so that the larger of the two is selected instead of the smaller.
 */
void 
spe_float_max(struct spe_function *p, int rT, int rA, int rB)
{
   int compare_reg = spe_allocate_available_register(p);
   spe_fcgt(p, compare_reg, rA, rB);
   spe_selb(p, rT, rB, rA, compare_reg);
   spe_release_register(p, compare_reg);
}

#endif /* GALLIUM_CELL */
