/*
 * Copyright 2010 Christoph Bumiller
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef __NVC0_COMPILER_H__
#define __NVC0_COMPILER_H__

#include "nv50/nv50_debug.h"

#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_double_list.h"

/* pseudo opcodes */
#define NV_OP_UNDEF      0
#define NV_OP_BIND       1
#define NV_OP_MERGE      2
#define NV_OP_PHI        3
#define NV_OP_SELECT     4
#define NV_OP_NOP        5

/**
 * BIND forces source operand i into the same register as destination operand i,
 *  and the operands will be assigned consecutive registers (needed for TEX).
 *  Beware conflicts !
 * SELECT forces its multiple source operands and its destination operand into
 *  one and the same register.
 */

/* base opcodes */
#define NV_OP_LD         6
#define NV_OP_ST         7
#define NV_OP_MOV        8
#define NV_OP_AND        9
#define NV_OP_OR        10
#define NV_OP_XOR       11
#define NV_OP_SHL       12
#define NV_OP_SHR       13
#define NV_OP_NOT       14
#define NV_OP_SET       15
#define NV_OP_ADD       16
#define NV_OP_SUB       17
#define NV_OP_MUL       18
#define NV_OP_MAD       19
#define NV_OP_ABS       20
#define NV_OP_NEG       21
#define NV_OP_MAX       22
#define NV_OP_MIN       23
#define NV_OP_CVT       24
#define NV_OP_CEIL      25
#define NV_OP_FLOOR     26
#define NV_OP_TRUNC     27
#define NV_OP_SAD       28

/* shader opcodes */
#define NV_OP_VFETCH    29
#define NV_OP_PFETCH    30
#define NV_OP_EXPORT    31
#define NV_OP_LINTERP   32
#define NV_OP_PINTERP   33
#define NV_OP_EMIT      34
#define NV_OP_RESTART   35
#define NV_OP_TEX       36
#define NV_OP_TXB       37
#define NV_OP_TXL       38
#define NV_OP_TXF       39
#define NV_OP_TXQ       40
#define NV_OP_QUADOP    41
#define NV_OP_DFDX      42
#define NV_OP_DFDY      43
#define NV_OP_KIL       44

/* control flow opcodes */
#define NV_OP_BRA       45
#define NV_OP_CALL      46
#define NV_OP_RET       47
#define NV_OP_EXIT      48
#define NV_OP_BREAK     49
#define NV_OP_BREAKADDR 50
#define NV_OP_JOINAT    51
#define NV_OP_JOIN      52

/* typed opcodes */
#define NV_OP_ADD_F32   NV_OP_ADD
#define NV_OP_ADD_B32   53
#define NV_OP_MUL_F32   NV_OP_MUL
#define NV_OP_MUL_B32   54
#define NV_OP_ABS_F32   NV_OP_ABS
#define NV_OP_ABS_S32   55
#define NV_OP_NEG_F32   NV_OP_NEG
#define NV_OP_NEG_S32   56
#define NV_OP_MAX_F32   NV_OP_MAX
#define NV_OP_MAX_S32   57
#define NV_OP_MAX_U32   58
#define NV_OP_MIN_F32   NV_OP_MIN
#define NV_OP_MIN_S32   59
#define NV_OP_MIN_U32   60
#define NV_OP_SET_F32   61
#define NV_OP_SET_S32   62
#define NV_OP_SET_U32   63
#define NV_OP_SAR       64
#define NV_OP_RCP       65
#define NV_OP_RSQ       66
#define NV_OP_LG2       67
#define NV_OP_SIN       68
#define NV_OP_COS       69
#define NV_OP_EX2       70
#define NV_OP_PRESIN    71
#define NV_OP_PREEX2    72
#define NV_OP_SAT       73

/* newly added opcodes */
#define NV_OP_SET_F32_AND 74
#define NV_OP_SET_F32_OR  75
#define NV_OP_SET_F32_XOR 76
#define NV_OP_SELP        77
#define NV_OP_SLCT        78
#define NV_OP_SLCT_F32    NV_OP_SLCT
#define NV_OP_SLCT_S32    79
#define NV_OP_SLCT_U32    80
#define NV_OP_SUB_F32     NV_OP_SUB
#define NV_OP_SUB_S32     81
#define NV_OP_MAD_F32     NV_OP_MAD
#define NV_OP_FSET_F32    82
#define NV_OP_TXG         83

#define NV_OP_COUNT     84

/* nv50 files omitted */
#define NV_FILE_GPR      0
#define NV_FILE_COND     1
#define NV_FILE_PRED     2
#define NV_FILE_IMM      16
#define NV_FILE_MEM_S    32
#define NV_FILE_MEM_V    34
#define NV_FILE_MEM_A    35
#define NV_FILE_MEM_L    48
#define NV_FILE_MEM_G    64
#define NV_FILE_MEM_C(i) (80 + i)

#define NV_IS_MEMORY_FILE(f) ((f) >= NV_FILE_MEM_S)

#define NV_MOD_NEG 1
#define NV_MOD_ABS 2
#define NV_MOD_NOT 4
#define NV_MOD_SAT 8

#define NV_TYPE_U8  0x00
#define NV_TYPE_S8  0x01
#define NV_TYPE_U16 0x02
#define NV_TYPE_S16 0x03
#define NV_TYPE_U32 0x04
#define NV_TYPE_S32 0x05
#define NV_TYPE_P32 0x07
#define NV_TYPE_F32 0x09
#define NV_TYPE_F64 0x0b
#define NV_TYPE_VEC(x, n) (NV_TYPE_##x | (n << 4))
#define NV_TYPE_ANY 0xff

#define NV_TYPE_ISINT(t) ((t) < 7)
#define NV_TYPE_ISSGD(t) ((t) & 1)

#define NV_CC_FL 0x0
#define NV_CC_LT 0x1
#define NV_CC_EQ 0x2
#define NV_CC_LE 0x3
#define NV_CC_GT 0x4
#define NV_CC_NE 0x5
#define NV_CC_GE 0x6
#define NV_CC_U  0x8
#define NV_CC_TR 0xf
#define NV_CC_O  0x10
#define NV_CC_C  0x11
#define NV_CC_A  0x12
#define NV_CC_S  0x13
#define NV_CC_INVERSE(cc) ((cc) ^ 0x7)
/* for 1 bit predicates: */
#define NV_CC_P     0
#define NV_CC_NOT_P 1

uint8_t nvc0_ir_reverse_cc(uint8_t cc);

#define NV_PC_MAX_INSTRUCTIONS 2048
#define NV_PC_MAX_VALUES (NV_PC_MAX_INSTRUCTIONS * 4)

#define NV_PC_MAX_BASIC_BLOCKS 1024

struct nv_op_info {
   uint base;                /* e.g. ADD_S32 -> ADD */
   char name[12];
   uint8_t type;
   uint16_t mods;
   unsigned flow        : 1;
   unsigned commutative : 1;
   unsigned vector      : 1;
   unsigned predicate   : 1;
   unsigned pseudo      : 1;
   unsigned immediate   : 3;
   unsigned memory      : 3;
};

extern struct nv_op_info nvc0_op_info_table[];

#define NV_BASEOP(op) (nvc0_op_info_table[op].base)
#define NV_OPTYPE(op) (nvc0_op_info_table[op].type)

static INLINE boolean
nv_is_texture_op(uint opcode)
{
   return (opcode >= NV_OP_TEX && opcode <= NV_OP_TXQ);
}

static INLINE boolean
nv_is_vector_op(uint opcode)
{
   return nvc0_op_info_table[opcode].vector ? TRUE : FALSE;
}

static INLINE boolean
nv_op_commutative(uint opcode)
{
   return nvc0_op_info_table[opcode].commutative ? TRUE : FALSE;
}

static INLINE uint8_t
nv_op_supported_src_mods(uint opcode, int s)
{
   return (nvc0_op_info_table[opcode].mods >> (s * 4)) & 0xf;
}

static INLINE uint
nv_type_order(ubyte type)
{
   switch (type & 0xf) {
   case NV_TYPE_U8:
   case NV_TYPE_S8:
      return 0;
   case NV_TYPE_U16:
   case NV_TYPE_S16:
      return 1;
   case NV_TYPE_U32:
   case NV_TYPE_F32:
   case NV_TYPE_S32:
   case NV_TYPE_P32:
      return 2;
   case NV_TYPE_F64:
      return 3;
   }
   assert(0);
   return 0;
}

static INLINE uint
nv_type_sizeof(ubyte type)
{
   if (type & 0xf0)
      return (1 << nv_type_order(type)) * (type >> 4);
   return 1 << nv_type_order(type);
}

static INLINE uint
nv_type_sizeof_base(ubyte type)
{
   return 1 << nv_type_order(type);
}

struct nv_reg {
   uint32_t address; /* for memory locations */
   int id; /* for registers */
   ubyte file;
   ubyte size;
   union {
      int32_t s32;
      int64_t s64;
      uint64_t u64;
      uint32_t u32; /* expected to be 0 for $r63 */
      float f32;
      double f64;
   } imm;
};

struct nv_range {
   struct nv_range *next;
   int bgn;
   int end;
};

struct nv_ref;

struct nv_value {
   struct nv_reg reg; 
   struct nv_instruction *insn;
   struct nv_value *join;
   struct nv_ref *last_use;
   int n;
   struct nv_range *livei;
   int refc;
   struct nv_value *next;
   struct nv_value *prev;
};

struct nv_ref {
   struct nv_value *value;
   struct nv_instruction *insn;
   struct list_head list; /* connects uses of the same value */
   uint8_t mod;
   uint8_t flags;
};

#define NV_REF_FLAG_REGALLOC_PRIV (1 << 0)

struct nv_basic_block;

struct nv_instruction {
   struct nv_instruction *next;
   struct nv_instruction *prev;
   uint opcode;
   uint serial;

   struct nv_value *def[5];
   struct nv_ref *src[6];

   int8_t predicate; /* index of predicate src */
   int8_t indirect;  /* index of pointer src */

   union {
      struct {
         uint8_t t; /* TIC binding */
         uint8_t s; /* TSC binding */
      } tex;
      struct {
         uint8_t d; /* output type */
         uint8_t s; /* input type */
      } cvt;
   } ext;

   struct nv_basic_block *bb;
   struct nv_basic_block *target; /* target block of control flow insn */

   unsigned cc         : 5; /* condition code */
   unsigned fixed      : 1; /* don't optimize away (prematurely) */
   unsigned terminator : 1;
   unsigned join       : 1;
   unsigned set_cond   : 4; /* 2nd byte */
   unsigned saturate   : 1;
   unsigned centroid   : 1;
   unsigned flat       : 1;
   unsigned patch      : 1;
   unsigned lanes      : 4; /* 3rd byte */
   unsigned tex_dim    : 2;
   unsigned tex_array  : 1;
   unsigned tex_cube   : 1;
   unsigned tex_shadow : 1; /* 4th byte */
   unsigned tex_live   : 1;
   unsigned tex_mask   : 4;

   uint8_t quadop;
};

static INLINE int
nvi_vector_size(struct nv_instruction *nvi)
{
   int i;
   assert(nvi);
   for (i = 0; i < 5 && nvi->def[i]; ++i);
   return i;
}

#define CFG_EDGE_FORWARD     0
#define CFG_EDGE_BACK        1
#define CFG_EDGE_LOOP_ENTER  2
#define CFG_EDGE_LOOP_LEAVE  4
#define CFG_EDGE_FAKE        8

/* 'WALL' edge means where reachability check doesn't follow */
/* 'LOOP' edge means just having to do with loops */
#define IS_LOOP_EDGE(k) ((k) & 7)
#define IS_WALL_EDGE(k) ((k) & 9)

struct nv_basic_block {
   struct nv_instruction *entry; /* first non-phi instruction */
   struct nv_instruction *exit;
   struct nv_instruction *phi; /* very first instruction */
   int num_instructions;

   struct nv_basic_block *out[2]; /* no indirect branches -> 2 */
   struct nv_basic_block *in[8]; /* hope that suffices */
   uint num_in;
   ubyte out_kind[2];
   ubyte in_kind[8];

   int id;
   int subroutine;
   uint priv; /* reset to 0 after you're done */
   uint pass_seq;

   uint32_t emit_pos; /* position, size in emitted code (in bytes) */
   uint32_t emit_size;

   uint32_t live_set[NV_PC_MAX_VALUES / 32];
};

struct nvc0_translation_info;

struct nv_pc {
   struct nv_basic_block **root;
   struct nv_basic_block *current_block;
   struct nv_basic_block *parent_block;

   int loop_nesting_bound;
   uint pass_seq;

   struct nv_value values[NV_PC_MAX_VALUES];
   struct nv_instruction instructions[NV_PC_MAX_INSTRUCTIONS];
   struct nv_ref **refs;
   struct nv_basic_block *bb_list[NV_PC_MAX_BASIC_BLOCKS];
   int num_values;
   int num_instructions;
   int num_refs;
   int num_blocks;
   int num_subroutines;

   int max_reg[4];

   uint32_t *immd_buf; /* populated on emit */
   unsigned immd_count;

   uint32_t *emit;
   uint32_t emit_size;
   uint32_t emit_pos;

   void *reloc_entries;
   unsigned num_relocs;

   /* optimization enables */
   boolean opt_reload_elim;
   boolean is_fragprog;
};

void nvc0_insn_append(struct nv_basic_block *, struct nv_instruction *);
void nvc0_insn_insert_before(struct nv_instruction *, struct nv_instruction *);
void nvc0_insn_insert_after(struct nv_instruction *, struct nv_instruction *);

static INLINE struct nv_instruction *
nv_alloc_instruction(struct nv_pc *pc, uint opcode)
{
   struct nv_instruction *insn;

   insn = &pc->instructions[pc->num_instructions++];
   assert(pc->num_instructions < NV_PC_MAX_INSTRUCTIONS);

   insn->opcode = opcode;
   insn->cc = NV_CC_P;
   insn->indirect = -1;
   insn->predicate = -1;

   return insn;
}

static INLINE struct nv_instruction *
new_instruction(struct nv_pc *pc, uint opcode)
{
   struct nv_instruction *insn = nv_alloc_instruction(pc, opcode);

   nvc0_insn_append(pc->current_block, insn);
   return insn;
}

static INLINE struct nv_instruction *
new_instruction_at(struct nv_pc *pc, struct nv_instruction *at, uint opcode)
{
   struct nv_instruction *insn = nv_alloc_instruction(pc, opcode);

   nvc0_insn_insert_after(at, insn);
   return insn;
}

static INLINE struct nv_value *
new_value(struct nv_pc *pc, ubyte file, ubyte size)
{
   struct nv_value *value = &pc->values[pc->num_values];

   assert(pc->num_values < NV_PC_MAX_VALUES - 1);

   value->n = pc->num_values++;
   value->join = value;
   value->reg.id = -1;
   value->reg.file = file;
   value->reg.size = size;
   return value;
}

static INLINE struct nv_value *
new_value_like(struct nv_pc *pc, struct nv_value *like)
{
   return new_value(pc, like->reg.file, like->reg.size);
}

static INLINE struct nv_ref *
new_ref(struct nv_pc *pc, struct nv_value *val)
{
   int i;
   struct nv_ref *ref;

   if ((pc->num_refs % 64) == 0) {
      const unsigned old_size = pc->num_refs * sizeof(struct nv_ref *);
      const unsigned new_size = (pc->num_refs + 64) * sizeof(struct nv_ref *);

      pc->refs = REALLOC(pc->refs, old_size, new_size);

      ref = CALLOC(64, sizeof(struct nv_ref));
      for (i = 0; i < 64; ++i)
         pc->refs[pc->num_refs + i] = &ref[i];
   }

   ref = pc->refs[pc->num_refs++];
   ref->value = val;

   LIST_INITHEAD(&ref->list);

   ++val->refc;
   return ref;
}

static INLINE struct nv_basic_block *
new_basic_block(struct nv_pc *pc)
{
   struct nv_basic_block *bb;

   if (pc->num_blocks >= NV_PC_MAX_BASIC_BLOCKS)
      return NULL;

   bb = CALLOC_STRUCT(nv_basic_block);

   bb->id = pc->num_blocks;
   pc->bb_list[pc->num_blocks++] = bb;
   return bb;
}

static INLINE void
nv_reference(struct nv_pc *pc,
             struct nv_instruction *nvi, int c, struct nv_value *s)
{
   struct nv_ref **d = &nvi->src[c];
   assert(c < 6);

   if (*d) {
      --(*d)->value->refc;
      LIST_DEL(&(*d)->list);
   }

   if (s) {
      if (!*d) {
         *d = new_ref(pc, s);
         (*d)->insn = nvi;
      } else {
         LIST_DEL(&(*d)->list);
         (*d)->value = s;
         ++(s->refc);
      }
      if (!s->last_use)
         s->last_use = *d;
      else
         LIST_ADDTAIL(&s->last_use->list, &(*d)->list);

      s->last_use = *d;
      (*d)->insn = nvi;
   } else {
      *d = NULL;
   }
}

/* nvc0_emit.c */
void nvc0_emit_instruction(struct nv_pc *, struct nv_instruction *);

/* nvc0_print.c */
const char *nvc0_opcode_name(uint opcode);
void nvc0_print_instruction(struct nv_instruction *);

/* nvc0_pc.c */
void nvc0_print_function(struct nv_basic_block *root);
void nvc0_print_program(struct nv_pc *);

boolean nvc0_insn_can_load(struct nv_instruction *, int s,
                           struct nv_instruction *);
boolean nvc0_insn_is_predicateable(struct nv_instruction *);

int nvc0_insn_refcount(struct nv_instruction *);
void nvc0_insn_delete(struct nv_instruction *);
void nvc0_insns_permute(struct nv_instruction *prev, struct nv_instruction *);

void nvc0_bblock_attach(struct nv_basic_block *parent,
                        struct nv_basic_block *child, ubyte edge_kind);
boolean nvc0_bblock_dominated_by(struct nv_basic_block *,
                                 struct nv_basic_block *);
boolean nvc0_bblock_reachable_by(struct nv_basic_block *future,
                                 struct nv_basic_block *past,
                                 struct nv_basic_block *final);
struct nv_basic_block *nvc0_bblock_dom_frontier(struct nv_basic_block *);

int nvc0_pc_replace_value(struct nv_pc *pc,
                          struct nv_value *old_val,
                          struct nv_value *new_val);

struct nv_value *nvc0_pc_find_immediate(struct nv_ref *);
struct nv_value *nvc0_pc_find_constant(struct nv_ref *);

typedef void (*nv_pc_pass_func)(void *priv, struct nv_basic_block *b);

void nvc0_pc_pass_in_order(struct nv_basic_block *, nv_pc_pass_func, void *);

int nvc0_pc_exec_pass0(struct nv_pc *pc);
int nvc0_pc_exec_pass1(struct nv_pc *pc);
int nvc0_pc_exec_pass2(struct nv_pc *pc);

int nvc0_tgsi_to_nc(struct nv_pc *, struct nvc0_translation_info *);

#endif // NV50_COMPILER_H
