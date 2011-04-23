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

#ifndef __NV50_COMPILER_H__
#define __NV50_COMPILER_H__

#include "nv50_debug.h"

#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"

#define NV_OP_PHI       0
#define NV_OP_EXTRACT   1
#define NV_OP_COMBINE   2
#define NV_OP_LDA       3
#define NV_OP_STA       4
#define NV_OP_MOV       5
#define NV_OP_ADD       6
#define NV_OP_SUB       7
#define NV_OP_NEG       8
#define NV_OP_MUL       9
#define NV_OP_MAD       10
#define NV_OP_CVT       11
#define NV_OP_SAT       12
#define NV_OP_NOT       13
#define NV_OP_AND       14
#define NV_OP_OR        15
#define NV_OP_XOR       16
#define NV_OP_SHL       17
#define NV_OP_SHR       18
#define NV_OP_RCP       19
#define NV_OP_UNDEF     20
#define NV_OP_RSQ       21
#define NV_OP_LG2       22
#define NV_OP_SIN       23
#define NV_OP_COS       24
#define NV_OP_EX2       25
#define NV_OP_PRESIN    26
#define NV_OP_PREEX2    27
#define NV_OP_MIN       28
#define NV_OP_MAX       29
#define NV_OP_SET       30
#define NV_OP_SAD       31
#define NV_OP_KIL       32
#define NV_OP_BRA       33
#define NV_OP_CALL      34
#define NV_OP_RET       35
#define NV_OP_BREAK     36
#define NV_OP_BREAKADDR 37
#define NV_OP_JOINAT    38
#define NV_OP_TEX       39
#define NV_OP_TXB       40
#define NV_OP_TXL       41
#define NV_OP_TXF       42
#define NV_OP_TXQ       43
#define NV_OP_DFDX      44
#define NV_OP_DFDY      45
#define NV_OP_QUADOP    46
#define NV_OP_LINTERP   47
#define NV_OP_PINTERP   48
#define NV_OP_ABS       49
#define NV_OP_CEIL      50
#define NV_OP_FLOOR     51
#define NV_OP_TRUNC     52
#define NV_OP_NOP       53
#define NV_OP_SELECT    54
#define NV_OP_EXPORT    55
#define NV_OP_JOIN      56
#define NV_OP_COUNT     57

#define NV_FILE_GPR      0
#define NV_FILE_OUT      1
#define NV_FILE_ADDR     2
#define NV_FILE_FLAGS    3
#define NV_FILE_IMM      16
#define NV_FILE_MEM_S    32
#define NV_FILE_MEM_P    33
#define NV_FILE_MEM_V    34
#define NV_FILE_MEM_L    48
#define NV_FILE_MEM_G(i) (64 + i)
#define NV_FILE_MEM_C(i) (80 + i)

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
#define NV_TYPE_LO  0x00
#define NV_TYPE_HI  0x80
#define NV_TYPE_ANY 0xff

#define NV_TYPE_ISINT(t) ((t) <= 5)
#define NV_TYPE_ISFLT(t) ((t) & 0x08)

/* $cX registers contain 4 bits: OCSZ (Z is bit 0) */
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

#define NV_PC_MAX_INSTRUCTIONS 2048
#define NV_PC_MAX_VALUES (NV_PC_MAX_INSTRUCTIONS * 4)

#define NV_PC_MAX_BASIC_BLOCKS 1024

static INLINE boolean
nv_is_vector_op(uint opcode)
{
   return (opcode >= NV_OP_TEX) && (opcode <= NV_OP_TXQ);
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
   int id;
   ubyte file;
   ubyte type; /* type of generating instruction's result */
   ubyte as_type; /* default type for new references to this value */
   union {
      float f32;
      double f64;
      int32_t s32;
      uint32_t u32;
   } imm;
};

struct nv_range {
   struct nv_range *next;
   int bgn;
   int end;
};

struct nv_value {
   struct nv_reg reg; 
   struct nv_instruction *insn;
   struct nv_value *join;
   int n;
   struct nv_range *livei;
   int refc;

   struct nv_value *next;
   struct nv_value *prev;
};

struct nv_ref {
   struct nv_value *value;
   ubyte mod;
   ubyte typecast;
   ubyte flags; /* not used yet */
};

#define NV_REF_FLAG_REGALLOC_PRIV (1 << 0)

struct nv_basic_block;

struct nv_instruction {
   struct nv_instruction *next;
   struct nv_instruction *prev;
   uint opcode;
   int serial;
   struct nv_value *def[4];
   struct nv_value *flags_def;
   struct nv_ref *src[5];
   struct nv_ref *flags_src;
   struct nv_basic_block *bb;
   struct nv_basic_block *target; /* target block of control flow insn */
   ubyte cc;
   unsigned set_cond      : 4;
   unsigned fixed         : 1; /* don't optimize away */
   unsigned is_terminator : 1;
   unsigned is_join       : 1;
   unsigned is_long       : 1; /* for emission */
   /* */
   unsigned saturate : 1;
   unsigned centroid : 1;
   unsigned flat     : 1;
   unsigned lanes    : 4;
   unsigned tex_live : 1;
   /* */
   ubyte tex_t; /* TIC binding */
   ubyte tex_s; /* TSC binding */
   unsigned tex_argc : 3;
   unsigned tex_cube : 1;
   unsigned tex_mask : 4;
   /* */
   ubyte quadop;
};

static INLINE int
nvi_vector_size(struct nv_instruction *nvi)
{
   int i;
   assert(nvi);
   for (i = 0; i < 4 && nvi->def[i]; ++i);
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

   uint32_t bin_pos; /* position, size in emitted code */
   uint32_t bin_size;

   uint32_t live_set[NV_PC_MAX_VALUES / 32];
};

struct nv50_translation_info;

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
   unsigned bin_size;
   unsigned bin_pos;

   void *fixups;
   unsigned num_fixups;

   /* optimization enables */
   boolean opt_reload_elim;
};

void nvbb_insert_tail(struct nv_basic_block *, struct nv_instruction *);
void nvi_insert_after(struct nv_instruction *, struct nv_instruction *);

static INLINE struct nv_instruction *
nv_alloc_instruction(struct nv_pc *pc, uint opcode)
{
   struct nv_instruction *insn;

   insn = &pc->instructions[pc->num_instructions++];
   assert(pc->num_instructions < NV_PC_MAX_INSTRUCTIONS);

   insn->cc = NV_CC_TR;
   insn->opcode = opcode;

   return insn;
}

static INLINE struct nv_instruction *
new_instruction(struct nv_pc *pc, uint opcode)
{
   struct nv_instruction *insn = nv_alloc_instruction(pc, opcode);

   nvbb_insert_tail(pc->current_block, insn);
   return insn;
}

static INLINE struct nv_instruction *
new_instruction_at(struct nv_pc *pc, struct nv_instruction *at, uint opcode)
{
   struct nv_instruction *insn = nv_alloc_instruction(pc, opcode);

   nvi_insert_after(at, insn);
   return insn;
}

static INLINE struct nv_value *
new_value(struct nv_pc *pc, ubyte file, ubyte type)
{
   struct nv_value *value = &pc->values[pc->num_values];

   assert(pc->num_values < NV_PC_MAX_VALUES - 1);

   value->n = pc->num_values++;
   value->join = value;
   value->reg.id = -1;
   value->reg.file = file;
   value->reg.type = value->reg.as_type = type;
   return value;
}

static INLINE struct nv_value *
new_value_like(struct nv_pc *pc, struct nv_value *like)
{
   struct nv_value *val = new_value(pc, like->reg.file, like->reg.type);
   val->reg.as_type = like->reg.as_type;
   return val;
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
   ref->typecast = val->reg.as_type;

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
nv_reference(struct nv_pc *pc, struct nv_ref **d, struct nv_value *s)
{
   if (*d)
      --(*d)->value->refc;

   if (s) {
      if (!*d)
         *d = new_ref(pc, s);
      else {
         (*d)->value = s;
         ++(s->refc);
      }
   } else {
      *d = NULL;
   }
}

/* nv50_emit.c */
void nv50_emit_instruction(struct nv_pc *, struct nv_instruction *);
unsigned nv50_inst_min_size(struct nv_instruction *);

/* nv50_print.c */
const char *nv_opcode_name(uint opcode);
void nv_print_instruction(struct nv_instruction *);

/* nv50_pc.c */

void nv_print_function(struct nv_basic_block *root);
void nv_print_program(struct nv_pc *);

boolean nv_op_commutative(uint opcode);
int nv50_indirect_opnd(struct nv_instruction *);
boolean nv50_nvi_can_use_imm(struct nv_instruction *, int s);
boolean nv50_nvi_can_predicate(struct nv_instruction *);
boolean nv50_nvi_can_load(struct nv_instruction *, int s, struct nv_value *);
boolean nv50_op_can_write_flags(uint opcode);
ubyte nv50_supported_src_mods(uint opcode, int s);
int nv_nvi_refcount(struct nv_instruction *);
void nv_nvi_delete(struct nv_instruction *);
void nv_nvi_permute(struct nv_instruction *, struct nv_instruction *);
void nvbb_attach_block(struct nv_basic_block *parent,
                       struct nv_basic_block *, ubyte edge_kind);
boolean nvbb_dominated_by(struct nv_basic_block *, struct nv_basic_block *);
boolean nvbb_reachable_by(struct nv_basic_block *, struct nv_basic_block *,
                          struct nv_basic_block *);
struct nv_basic_block *nvbb_dom_frontier(struct nv_basic_block *);
int nvcg_replace_value(struct nv_pc *pc, struct nv_value *old_val,
                       struct nv_value *new_val);
struct nv_value *nvcg_find_immediate(struct nv_ref *);
struct nv_value *nvcg_find_constant(struct nv_ref *);

typedef void (*nv_pc_pass_func)(void *priv, struct nv_basic_block *b);

void nv_pc_pass_in_order(struct nv_basic_block *, nv_pc_pass_func, void *);

int nv_pc_exec_pass0(struct nv_pc *pc);
int nv_pc_exec_pass1(struct nv_pc *pc);
int nv_pc_exec_pass2(struct nv_pc *pc);

int nv50_tgsi_to_nc(struct nv_pc *, struct nv50_translation_info *);

#endif // NV50_COMPILER_H
