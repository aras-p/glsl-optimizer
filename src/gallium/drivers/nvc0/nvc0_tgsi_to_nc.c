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

#include <unistd.h>

#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "tgsi/tgsi_dump.h"
#include "util/u_dynarray.h"

#include "nvc0_pc.h"
#include "nvc0_program.h"

/* Arbitrary internal limits. */
#define BLD_MAX_TEMPS 64
#define BLD_MAX_ADDRS 4
#define BLD_MAX_PREDS 4
#define BLD_MAX_IMMDS 128
#define BLD_MAX_OUTPS PIPE_MAX_SHADER_OUTPUTS

#define BLD_MAX_COND_NESTING 8
#define BLD_MAX_LOOP_NESTING 4
#define BLD_MAX_CALL_NESTING 2

/* This structure represents a TGSI register. */
struct bld_register {
   struct nv_value *current;
   /* collect all SSA values assigned to it */
   struct util_dynarray vals;
   /* 1 bit per loop level, indicates if used/defd, reset when loop ends */
   uint16_t loop_use;
   uint16_t loop_def;
};

static INLINE struct nv_value **
bld_register_access(struct bld_register *reg, unsigned i)
{
   return util_dynarray_element(&reg->vals, struct nv_value *, i);
}

static INLINE void
bld_register_add_val(struct bld_register *reg, struct nv_value *val)
{
   struct nv_basic_block *bb = val->insn->bb;

   if (reg->vals.size &&
       (util_dynarray_top(&reg->vals, struct nv_value *))->insn->bb == bb)
      *(util_dynarray_top_ptr(&reg->vals, struct nv_value *)) = val;
   else
      util_dynarray_append(&reg->vals, struct nv_value *, val);
}

static INLINE boolean
bld_register_del_val(struct bld_register *reg, struct nv_value *val)
{
   unsigned i;

   for (i = reg->vals.size / sizeof(struct nv_value *); i > 0; --i)
      if (*bld_register_access(reg, i - 1) == val)
         break;
   if (!i)
      return FALSE;

   if (i != reg->vals.size / sizeof(struct nv_value *))
      *bld_register_access(reg, i - 1) = util_dynarray_pop(&reg->vals,
                                                           struct nv_value *);
   else
      reg->vals.size -= sizeof(struct nv_value *);

   return TRUE;
}

struct bld_context {
   struct nvc0_translation_info *ti;

   struct nv_pc *pc;
   struct nv_basic_block *b;

   struct tgsi_parse_context parse[BLD_MAX_CALL_NESTING];
   int call_lvl;

   struct nv_basic_block *cond_bb[BLD_MAX_COND_NESTING];
   struct nv_basic_block *join_bb[BLD_MAX_COND_NESTING];
   struct nv_basic_block *else_bb[BLD_MAX_COND_NESTING];
   int cond_lvl;
   struct nv_basic_block *loop_bb[BLD_MAX_LOOP_NESTING];
   struct nv_basic_block *brkt_bb[BLD_MAX_LOOP_NESTING];
   int loop_lvl;

   ubyte out_kind; /* CFG_EDGE_FORWARD, or FAKE in case of BREAK/CONT */

   struct bld_register tvs[BLD_MAX_TEMPS][4]; /* TGSI_FILE_TEMPORARY */
   struct bld_register avs[BLD_MAX_ADDRS][4]; /* TGSI_FILE_ADDRESS */
   struct bld_register pvs[BLD_MAX_PREDS][4]; /* TGSI_FILE_PREDICATE */
   struct bld_register ovs[BLD_MAX_OUTPS][4]; /* TGSI_FILE_OUTPUT, FP only */

   uint32_t outputs_written[(PIPE_MAX_SHADER_OUTPUTS + 7) / 8];
   int hpos_index;

   struct nv_value *zero;
   struct nv_value *frag_coord[4];

   /* wipe on new BB */
   struct nv_value *saved_sysvals[4];
   struct nv_value *saved_addr[4][2];
   struct nv_value *saved_inputs[PIPE_MAX_SHADER_INPUTS][4];
   struct nv_value *saved_immd[BLD_MAX_IMMDS];
   uint num_immds;
};

static INLINE ubyte
bld_register_file(struct bld_context *bld, struct bld_register *reg)
{
   if (reg >= &bld->pvs[0][0] &&
       reg <  &bld->ovs[0][0])
      return NV_FILE_PRED;
   return NV_FILE_GPR;
}

static INLINE struct nv_value *
bld_fetch(struct bld_context *bld, struct bld_register *regs, int i, int c)
{
   regs[i * 4 + c].loop_use |= 1 << bld->loop_lvl;
   return regs[i * 4 + c].current;
}

static struct nv_value *
bld_loop_phi(struct bld_context *, struct bld_register *, struct nv_value *);

/* If a variable is defined in a loop without prior use, we don't need
 * a phi in the loop header to account for backwards flow.
 *
 * However, if this variable is then also used outside the loop, we do
 * need a phi after all. But we must not use this phi's def inside the
 * loop, so we can eliminate the phi if it is unused later.
 */
static INLINE void
bld_store(struct bld_context *bld,
          struct bld_register *regs, int i, int c, struct nv_value *val)
{
   const uint16_t m = 1 << bld->loop_lvl;
   struct bld_register *reg = &regs[i * 4 + c];

   if (bld->loop_lvl && !(m & (reg->loop_def | reg->loop_use)))
      bld_loop_phi(bld, reg, val);

   reg->current = val;
   bld_register_add_val(reg, reg->current);

   reg->loop_def |= 1 << bld->loop_lvl;
}

#define FETCH_TEMP(i, c)    bld_fetch(bld, &bld->tvs[0][0], i, c)
#define STORE_TEMP(i, c, v) bld_store(bld, &bld->tvs[0][0], i, c, (v))
#define FETCH_ADDR(i, c)    bld_fetch(bld, &bld->avs[0][0], i, c)
#define STORE_ADDR(i, c, v) bld_store(bld, &bld->avs[0][0], i, c, (v))
#define FETCH_PRED(i, c)    bld_fetch(bld, &bld->pvs[0][0], i, c)
#define STORE_PRED(i, c, v) bld_store(bld, &bld->pvs[0][0], i, c, (v))
#define STORE_OUTP(i, c, v)                                         \
   do {                                                             \
      bld_store(bld, &bld->ovs[0][0], i, c, (v));                   \
      bld->outputs_written[(i) / 8] |= 1 << (((i) * 4 + (c)) % 32); \
   } while (0)

static INLINE void
bld_clear_def_use(struct bld_register *regs, int n, int lvl)
{
   int i;
   const uint16_t mask = ~(1 << lvl);

   for (i = 0; i < n * 4; ++i) {
      regs[i].loop_def &= mask;
      regs[i].loop_use &= mask;
   }
}

static INLINE void
bld_warn_uninitialized(struct bld_context *bld, int kind,
                       struct bld_register *reg, struct nv_basic_block *b)
{
#if NV50_DEBUG & NV50_DEBUG_SHADER
   long i = (reg - &bld->tvs[0][0]) / 4;
   long c = (reg - &bld->tvs[0][0]) & 3;

   if (c == 3)
      c = -1;
   debug_printf("WARNING: TEMP[%li].%c %s used uninitialized in BB:%i\n",
                i, (int)('x' + c), kind ? "may be" : "is", b->id);
#endif
}

static INLINE struct nv_value *
bld_def(struct nv_instruction *i, int c, struct nv_value *value)
{
   i->def[c] = value;
   value->insn = i;
   return value;
}

static INLINE struct nv_value *
find_by_bb(struct bld_register *reg, struct nv_basic_block *b)
{
   int i;

   if (reg->current && reg->current->insn->bb == b)
      return reg->current;

   for (i = 0; i < reg->vals.size / sizeof(struct nv_value *); ++i)
      if ((*bld_register_access(reg, i))->insn->bb == b)
         return *bld_register_access(reg, i);
   return NULL;
}

/* Fetch value from register that was defined in the specified BB,
 * or search for first definitions in all of its predecessors.
 */
static void
fetch_by_bb(struct bld_register *reg,
            struct nv_value **vals, int *n,
            struct nv_basic_block *b)
{
   int i;
   struct nv_value *val;

   assert(*n < 16); /* MAX_COND_NESTING */

   val = find_by_bb(reg, b);
   if (val) {
      for (i = 0; i < *n; ++i)
         if (vals[i] == val)
            return;
      vals[(*n)++] = val;
      return;
   }
   for (i = 0; i < b->num_in; ++i)
      if (!IS_WALL_EDGE(b->in_kind[i]))
         fetch_by_bb(reg, vals, n, b->in[i]);
}

static INLINE boolean
nvc0_bblock_is_terminated(struct nv_basic_block *bb)
{
   return bb->exit && bb->exit->terminator;
}

static INLINE struct nv_value *
bld_load_imm_u32(struct bld_context *bld, uint32_t u);

static INLINE struct nv_value *
bld_undef(struct bld_context *bld, ubyte file)
{
   struct nv_instruction *nvi = new_instruction(bld->pc, NV_OP_UNDEF);

   return bld_def(nvi, 0, new_value(bld->pc, file, 4));
}

static struct nv_value *
bld_phi(struct bld_context *bld, struct nv_basic_block *b,
        struct bld_register *reg)
{
   struct nv_basic_block *in;
   struct nv_value *vals[16] = { NULL };
   struct nv_value *val;
   struct nv_instruction *phi;
   int i, j, n;

   do {
      i = n = 0;
      fetch_by_bb(reg, vals, &n, b);

      if (!n) {
         bld_warn_uninitialized(bld, 0, reg, b);
         return NULL;
      }

      if (n == 1) {
         if (nvc0_bblock_dominated_by(b, vals[0]->insn->bb))
            break;

         bld_warn_uninitialized(bld, 1, reg, b);

         /* back-tracking to insert missing value of other path */
         in = b;
         while (in->in[0]) {
            if (in->num_in == 1) {
               in = in->in[0];
            } else {
               if (!nvc0_bblock_reachable_by(in->in[0], vals[0]->insn->bb, b))
                  in = in->in[0];
               else
               if (!nvc0_bblock_reachable_by(in->in[1], vals[0]->insn->bb, b))
                  in = in->in[1];
               else
                  in = in->in[0];
            }
         }
         bld->pc->current_block = in;

         /* should make this a no-op */
         bld_register_add_val(reg, bld_undef(bld, vals[0]->reg.file));
         continue;
      }

      for (i = 0; i < n; ++i) {
         /* if value dominates b, continue to the redefinitions */
         if (nvc0_bblock_dominated_by(b, vals[i]->insn->bb))
            continue;

         /* if value dominates any in-block, b should be the dom frontier */
         for (j = 0; j < b->num_in; ++j)
            if (nvc0_bblock_dominated_by(b->in[j], vals[i]->insn->bb))
               break;
         /* otherwise, find the dominance frontier and put the phi there */
         if (j == b->num_in) {
            in = nvc0_bblock_dom_frontier(vals[i]->insn->bb);
            val = bld_phi(bld, in, reg);
            bld_register_add_val(reg, val);
            break;
         }
      }
   } while(i < n);

   bld->pc->current_block = b;

   if (n == 1)
      return vals[0];

   phi = new_instruction(bld->pc, NV_OP_PHI);

   bld_def(phi, 0, new_value(bld->pc, vals[0]->reg.file, vals[0]->reg.size));
   for (i = 0; i < n; ++i)
      nv_reference(bld->pc, phi, i, vals[i]);

   return phi->def[0];
}

/* Insert a phi function in the loop header.
 * For nested loops, we need to insert phi functions in all the outer
 * loop headers if they don't have one yet.
 *
 * @def: redefinition from inside loop, or NULL if to be replaced later
 */
static struct nv_value *
bld_loop_phi(struct bld_context *bld, struct bld_register *reg,
             struct nv_value *def)
{
   struct nv_instruction *phi;
   struct nv_basic_block *bb = bld->pc->current_block;
   struct nv_value *val = NULL;

   if (bld->ti->require_stores) /* XXX: actually only for INDEXABLE_TEMP */
      return NULL;

   if (bld->loop_lvl > 1) {
      --bld->loop_lvl;
      if (!((reg->loop_def | reg->loop_use) & (1 << bld->loop_lvl)))
         val = bld_loop_phi(bld, reg, NULL);
      ++bld->loop_lvl;
   }

   if (!val)
      val = bld_phi(bld, bld->pc->current_block, reg); /* old definition */
   if (!val) {
      bld->pc->current_block = bld->loop_bb[bld->loop_lvl - 1]->in[0];
      val = bld_undef(bld, bld_register_file(bld, reg));
   }

   bld->pc->current_block = bld->loop_bb[bld->loop_lvl - 1];

   phi = new_instruction(bld->pc, NV_OP_PHI);

   bld_def(phi, 0, new_value_like(bld->pc, val));
   if (!def)
      def = phi->def[0];

   bld_register_add_val(reg, phi->def[0]);

   phi->target = (struct nv_basic_block *)reg; /* cheat */

   nv_reference(bld->pc, phi, 0, val);
   nv_reference(bld->pc, phi, 1, def);

   bld->pc->current_block = bb;

   return phi->def[0];
}

static INLINE struct nv_value *
bld_fetch_global(struct bld_context *bld, struct bld_register *reg)
{
   const uint16_t m = 1 << bld->loop_lvl;
   const uint16_t use = reg->loop_use;

   reg->loop_use |= m;

   /* If neither used nor def'd inside the loop, build a phi in foresight,
    * so we don't have to replace stuff later on, which requires tracking.
    */
   if (bld->loop_lvl && !((use | reg->loop_def) & m))
      return bld_loop_phi(bld, reg, NULL);

   return bld_phi(bld, bld->pc->current_block, reg);
}

static INLINE struct nv_value *
bld_imm_u32(struct bld_context *bld, uint32_t u)
{
   int i;
   unsigned n = bld->num_immds;

   for (i = 0; i < n; ++i)
      if (bld->saved_immd[i]->reg.imm.u32 == u)
         return bld->saved_immd[i];

   assert(n < BLD_MAX_IMMDS);
   bld->num_immds++;

   bld->saved_immd[n] = new_value(bld->pc, NV_FILE_IMM, 4);
   bld->saved_immd[n]->reg.imm.u32 = u;
   return bld->saved_immd[n];
}

static void
bld_replace_value(struct nv_pc *, struct nv_basic_block *, struct nv_value *,
                  struct nv_value *);

/* Replace the source of the phi in the loop header by the last assignment,
 * or eliminate the phi function if there is no assignment inside the loop.
 *
 * Redundancy situation 1 - (used) but (not redefined) value:
 *  %3 = phi %0, %3 = %3 is used
 *  %3 = phi %0, %4 = is new definition
 *
 * Redundancy situation 2 - (not used) but (redefined) value:
 *  %3 = phi %0, %2 = %2 is used, %3 could be used outside, deleted by DCE
 */
static void
bld_loop_end(struct bld_context *bld, struct nv_basic_block *bb)
{
   struct nv_basic_block *save = bld->pc->current_block;
   struct nv_instruction *phi, *next;
   struct nv_value *val;
   struct bld_register *reg;
   int i, s, n;

   for (phi = bb->phi; phi && phi->opcode == NV_OP_PHI; phi = next) {
      next = phi->next;

      reg = (struct bld_register *)phi->target;
      phi->target = NULL;

      /* start with s == 1, src[0] is from outside the loop */
      for (s = 1, n = 0; n < bb->num_in; ++n) {
         if (bb->in_kind[n] != CFG_EDGE_BACK)
            continue;

         assert(s < 4);
         bld->pc->current_block = bb->in[n];
         val = bld_fetch_global(bld, reg);

         for (i = 0; i < 4; ++i)
            if (phi->src[i] && phi->src[i]->value == val)
               break;
         if (i == 4) {
            /* skip values we do not want to replace */
            for (; phi->src[s] && phi->src[s]->value != phi->def[0]; ++s);
            nv_reference(bld->pc, phi, s++, val);
         }
      }
      bld->pc->current_block = save;

      if (phi->src[0]->value == phi->def[0] ||
          phi->src[0]->value == phi->src[1]->value)
         s = 1;
      else
      if (phi->src[1]->value == phi->def[0])
         s = 0;
      else
         continue;

      if (s >= 0) {
         /* eliminate the phi */
         bld_register_del_val(reg, phi->def[0]);

         ++bld->pc->pass_seq;
         bld_replace_value(bld->pc, bb, phi->def[0], phi->src[s]->value);

         nvc0_insn_delete(phi);
      }
   }
}

static INLINE struct nv_value *
bld_imm_f32(struct bld_context *bld, float f)
{
   return bld_imm_u32(bld, fui(f));
}

static struct nv_value *
bld_insn_1(struct bld_context *bld, uint opcode, struct nv_value *src0)
{
   struct nv_instruction *insn = new_instruction(bld->pc, opcode);

   nv_reference(bld->pc, insn, 0, src0);
   
   return bld_def(insn, 0, new_value(bld->pc, NV_FILE_GPR, src0->reg.size));
}

static struct nv_value *
bld_insn_2(struct bld_context *bld, uint opcode,
           struct nv_value *src0, struct nv_value *src1)
{
   struct nv_instruction *insn = new_instruction(bld->pc, opcode);

   nv_reference(bld->pc, insn, 0, src0);
   nv_reference(bld->pc, insn, 1, src1);

   return bld_def(insn, 0, new_value(bld->pc, NV_FILE_GPR, src0->reg.size));
}

static struct nv_value *
bld_insn_3(struct bld_context *bld, uint opcode,
           struct nv_value *src0, struct nv_value *src1,
           struct nv_value *src2)
{
   struct nv_instruction *insn = new_instruction(bld->pc, opcode);

   nv_reference(bld->pc, insn, 0, src0);
   nv_reference(bld->pc, insn, 1, src1);
   nv_reference(bld->pc, insn, 2, src2);

   return bld_def(insn, 0, new_value(bld->pc, NV_FILE_GPR, src0->reg.size));
}

static INLINE void
bld_src_predicate(struct bld_context *bld,
                  struct nv_instruction *nvi, int s, struct nv_value *val)
{
   nvi->predicate = s;
   nv_reference(bld->pc, nvi, s, val);
}

static INLINE void
bld_src_pointer(struct bld_context *bld,
                struct nv_instruction *nvi, int s, struct nv_value *val)
{
   nvi->indirect = s;
   nv_reference(bld->pc, nvi, s, val);
}

static void
bld_lmem_store(struct bld_context *bld, struct nv_value *ptr, int ofst,
               struct nv_value *val)
{
   struct nv_instruction *insn = new_instruction(bld->pc, NV_OP_ST);
   struct nv_value *loc;

   loc = new_value(bld->pc, NV_FILE_MEM_L, nv_type_sizeof(NV_TYPE_U32));

   loc->reg.address = ofst * 4;

   nv_reference(bld->pc, insn, 0, loc);
   nv_reference(bld->pc, insn, 1, val);
   if (ptr)
      bld_src_pointer(bld, insn, 2, ptr);
}

static struct nv_value *
bld_lmem_load(struct bld_context *bld, struct nv_value *ptr, int ofst)
{
   struct nv_value *loc, *val;

   loc = new_value(bld->pc, NV_FILE_MEM_L, nv_type_sizeof(NV_TYPE_U32));

   loc->reg.address = ofst * 4;

   val = bld_insn_1(bld, NV_OP_LD, loc);
   if (ptr)
      bld_src_pointer(bld, val->insn, 1, ptr);

   return val;
}

static struct nv_value *
bld_pow(struct bld_context *bld, struct nv_value *x, struct nv_value *e)
{
   struct nv_value *val;

   val = bld_insn_1(bld, NV_OP_LG2, x);
   val = bld_insn_2(bld, NV_OP_MUL_F32, e, val);

   val = bld_insn_1(bld, NV_OP_PREEX2, val);
   val = bld_insn_1(bld, NV_OP_EX2, val);

   return val;
}

static INLINE struct nv_value *
bld_load_imm_f32(struct bld_context *bld, float f)
{
   if (f == 0.0f)
      return bld->zero;
   return bld_insn_1(bld, NV_OP_MOV, bld_imm_f32(bld, f));
}

static INLINE struct nv_value *
bld_load_imm_u32(struct bld_context *bld, uint32_t u)
{
   if (u == 0)
      return bld->zero;
   return bld_insn_1(bld, NV_OP_MOV, bld_imm_u32(bld, u));
}

static INLINE struct nv_value *
bld_setp(struct bld_context *bld, uint op, uint8_t cc,
         struct nv_value *src0, struct nv_value *src1)
{
   struct nv_value *val = bld_insn_2(bld, op, src0, src1);

   val->reg.file = NV_FILE_PRED;
   val->reg.size = 1;
   val->insn->set_cond = cc & 0xf;
   return val;
}

static INLINE struct nv_value *
bld_cvt(struct bld_context *bld, uint8_t dt, uint8_t st, struct nv_value *src)
{
   struct nv_value *val = bld_insn_1(bld, NV_OP_CVT, src);
   val->insn->ext.cvt.d = dt;
   val->insn->ext.cvt.s = st;
   return val;
}

static void
bld_kil(struct bld_context *bld, struct nv_value *src)
{
   struct nv_instruction *nvi;

   src = bld_setp(bld, NV_OP_SET_F32, NV_CC_LT, src, bld->zero);

   nvi = new_instruction(bld->pc, NV_OP_KIL);
   nvi->fixed = 1;

   bld_src_predicate(bld, nvi, 0, src);
}

static void
bld_flow(struct bld_context *bld, uint opcode,
         struct nv_value *pred, uint8_t cc, struct nv_basic_block *target,
         boolean reconverge)
{
   struct nv_instruction *nvi;

   if (reconverge)
      new_instruction(bld->pc, NV_OP_JOINAT)->fixed = 1;

   nvi = new_instruction(bld->pc, opcode);
   nvi->target = target;
   nvi->terminator = 1;
   if (pred) {
      nvi->cc = cc;
      bld_src_predicate(bld, nvi, 0, pred);
   }
}

static ubyte
translate_setcc(unsigned opcode)
{
   switch (opcode) {
   case TGSI_OPCODE_SLT: return NV_CC_LT;
   case TGSI_OPCODE_SGE: return NV_CC_GE;
   case TGSI_OPCODE_SEQ: return NV_CC_EQ;
   case TGSI_OPCODE_SGT: return NV_CC_GT;
   case TGSI_OPCODE_SLE: return NV_CC_LE;
   case TGSI_OPCODE_SNE: return NV_CC_NE | NV_CC_U;
   case TGSI_OPCODE_STR: return NV_CC_TR;
   case TGSI_OPCODE_SFL: return NV_CC_FL;

   case TGSI_OPCODE_ISLT: return NV_CC_LT;
   case TGSI_OPCODE_ISGE: return NV_CC_GE;
   case TGSI_OPCODE_USEQ: return NV_CC_EQ;
   case TGSI_OPCODE_USGE: return NV_CC_GE;
   case TGSI_OPCODE_USLT: return NV_CC_LT;
   case TGSI_OPCODE_USNE: return NV_CC_NE;
   default:
      assert(0);
      return NV_CC_FL;
   }
}

static uint
translate_opcode(uint opcode)
{
   switch (opcode) {
   case TGSI_OPCODE_ABS: return NV_OP_ABS_F32;
   case TGSI_OPCODE_ADD: return NV_OP_ADD_F32;
   case TGSI_OPCODE_SUB: return NV_OP_SUB_F32;
   case TGSI_OPCODE_UADD: return NV_OP_ADD_B32;
   case TGSI_OPCODE_AND: return NV_OP_AND;
   case TGSI_OPCODE_EX2: return NV_OP_EX2;
   case TGSI_OPCODE_CEIL: return NV_OP_CEIL;
   case TGSI_OPCODE_FLR: return NV_OP_FLOOR;
   case TGSI_OPCODE_TRUNC: return NV_OP_TRUNC;
   case TGSI_OPCODE_COS: return NV_OP_COS;
   case TGSI_OPCODE_SIN: return NV_OP_SIN;
   case TGSI_OPCODE_DDX: return NV_OP_DFDX;
   case TGSI_OPCODE_DDY: return NV_OP_DFDY;
   case TGSI_OPCODE_F2I:
   case TGSI_OPCODE_F2U:
   case TGSI_OPCODE_I2F:
   case TGSI_OPCODE_U2F: return NV_OP_CVT;
   case TGSI_OPCODE_INEG: return NV_OP_NEG_S32;
   case TGSI_OPCODE_LG2: return NV_OP_LG2;
   case TGSI_OPCODE_ISHR: return NV_OP_SAR;
   case TGSI_OPCODE_USHR: return NV_OP_SHR;
   case TGSI_OPCODE_MAD: return NV_OP_MAD_F32;
   case TGSI_OPCODE_MAX: return NV_OP_MAX_F32;
   case TGSI_OPCODE_IMAX: return NV_OP_MAX_S32;
   case TGSI_OPCODE_UMAX: return NV_OP_MAX_U32;
   case TGSI_OPCODE_MIN: return NV_OP_MIN_F32;
   case TGSI_OPCODE_IMIN: return NV_OP_MIN_S32;
   case TGSI_OPCODE_UMIN: return NV_OP_MIN_U32;
   case TGSI_OPCODE_MUL: return NV_OP_MUL_F32;
   case TGSI_OPCODE_UMUL: return NV_OP_MUL_B32;
   case TGSI_OPCODE_OR: return NV_OP_OR;
   case TGSI_OPCODE_RCP: return NV_OP_RCP;
   case TGSI_OPCODE_RSQ: return NV_OP_RSQ;
   case TGSI_OPCODE_SAD: return NV_OP_SAD;
   case TGSI_OPCODE_SHL: return NV_OP_SHL;
   case TGSI_OPCODE_SLT:
   case TGSI_OPCODE_SGE:
   case TGSI_OPCODE_SEQ:
   case TGSI_OPCODE_SGT:
   case TGSI_OPCODE_SLE:
   case TGSI_OPCODE_SNE: return NV_OP_FSET_F32;
   case TGSI_OPCODE_ISLT:
   case TGSI_OPCODE_ISGE: return NV_OP_SET_S32;
   case TGSI_OPCODE_USEQ:
   case TGSI_OPCODE_USGE:
   case TGSI_OPCODE_USLT:
   case TGSI_OPCODE_USNE: return NV_OP_SET_U32;
   case TGSI_OPCODE_TEX: return NV_OP_TEX;
   case TGSI_OPCODE_TXP: return NV_OP_TEX;
   case TGSI_OPCODE_TXB: return NV_OP_TXB;
   case TGSI_OPCODE_TXL: return NV_OP_TXL;
   case TGSI_OPCODE_XOR: return NV_OP_XOR;
   default:
      return NV_OP_NOP;
   }
}

#if 0
static ubyte
infer_src_type(unsigned opcode)
{
   switch (opcode) {
   case TGSI_OPCODE_MOV:
   case TGSI_OPCODE_AND:
   case TGSI_OPCODE_OR:
   case TGSI_OPCODE_XOR:
   case TGSI_OPCODE_SAD:
   case TGSI_OPCODE_U2F:
   case TGSI_OPCODE_UADD:
   case TGSI_OPCODE_UDIV:
   case TGSI_OPCODE_UMOD:
   case TGSI_OPCODE_UMAD:
   case TGSI_OPCODE_UMUL:
   case TGSI_OPCODE_UMAX:
   case TGSI_OPCODE_UMIN:
   case TGSI_OPCODE_USEQ:
   case TGSI_OPCODE_USGE:
   case TGSI_OPCODE_USLT:
   case TGSI_OPCODE_USNE:
   case TGSI_OPCODE_USHR:
      return NV_TYPE_U32;
   case TGSI_OPCODE_I2F:
   case TGSI_OPCODE_IDIV:
   case TGSI_OPCODE_IMAX:
   case TGSI_OPCODE_IMIN:
   case TGSI_OPCODE_INEG:
   case TGSI_OPCODE_ISGE:
   case TGSI_OPCODE_ISHR:
   case TGSI_OPCODE_ISLT:
      return NV_TYPE_S32;
   default:
      return NV_TYPE_F32;
   }
}

static ubyte
infer_dst_type(unsigned opcode)
{
   switch (opcode) {
   case TGSI_OPCODE_MOV:
   case TGSI_OPCODE_F2U:
   case TGSI_OPCODE_AND:
   case TGSI_OPCODE_OR:
   case TGSI_OPCODE_XOR:
   case TGSI_OPCODE_SAD:
   case TGSI_OPCODE_UADD:
   case TGSI_OPCODE_UDIV:
   case TGSI_OPCODE_UMOD:
   case TGSI_OPCODE_UMAD:
   case TGSI_OPCODE_UMUL:
   case TGSI_OPCODE_UMAX:
   case TGSI_OPCODE_UMIN:
   case TGSI_OPCODE_USEQ:
   case TGSI_OPCODE_USGE:
   case TGSI_OPCODE_USLT:
   case TGSI_OPCODE_USNE:
   case TGSI_OPCODE_USHR:
      return NV_TYPE_U32;
   case TGSI_OPCODE_F2I:
   case TGSI_OPCODE_IDIV:
   case TGSI_OPCODE_IMAX:
   case TGSI_OPCODE_IMIN:
   case TGSI_OPCODE_INEG:
   case TGSI_OPCODE_ISGE:
   case TGSI_OPCODE_ISHR:
   case TGSI_OPCODE_ISLT:
      return NV_TYPE_S32;
   default:
      return NV_TYPE_F32;
   }
}
#endif

static void
emit_store(struct bld_context *bld, const struct tgsi_full_instruction *inst,
           unsigned chan, struct nv_value *res)
{
   const struct tgsi_full_dst_register *reg = &inst->Dst[0];
   struct nv_instruction *nvi;
   struct nv_value *mem;
   struct nv_value *ptr = NULL;
   int idx;

   idx = reg->Register.Index;
   assert(chan < 4);

   if (reg->Register.Indirect)
      ptr = FETCH_ADDR(reg->Indirect.Index,
                       tgsi_util_get_src_register_swizzle(&reg->Indirect, 0));

   switch (inst->Instruction.Saturate) {
   case TGSI_SAT_NONE:
      break;
   case TGSI_SAT_ZERO_ONE:
      res = bld_insn_1(bld, NV_OP_SAT, res);
      break;
   case TGSI_SAT_MINUS_PLUS_ONE:
      res = bld_insn_2(bld, NV_OP_MAX_F32, res, bld_load_imm_f32(bld, -1.0f));
      res = bld_insn_2(bld, NV_OP_MIN_F32, res, bld_load_imm_f32(bld, 1.0f));
      break;
   }

   switch (reg->Register.File) {
   case TGSI_FILE_OUTPUT:
      if (!res->insn)
         res = bld_insn_1(bld, NV_OP_MOV, res);

      if (bld->pc->is_fragprog) {
         assert(!ptr);
         STORE_OUTP(idx, chan, res);
      } else {
         nvi = new_instruction(bld->pc, NV_OP_EXPORT);
         mem = new_value(bld->pc, bld->ti->output_file, res->reg.size);
         nv_reference(bld->pc, nvi, 0, mem);
         nv_reference(bld->pc, nvi, 1, res);
         if (!ptr)
            mem->reg.address = bld->ti->output_loc[idx][chan];
         else
            mem->reg.address = 0x80 + idx * 16 + chan * 4;
         nvi->fixed = 1;
      }
      break;
   case TGSI_FILE_TEMPORARY:
      assert(idx < BLD_MAX_TEMPS);
      if (!res->insn || res->insn->bb != bld->pc->current_block)
         res = bld_insn_1(bld, NV_OP_MOV, res);

      assert(res->reg.file == NV_FILE_GPR);

      if (bld->ti->require_stores)
         bld_lmem_store(bld, ptr, idx * 4 + chan, res);
      else
         STORE_TEMP(idx, chan, res);
      break;
   case TGSI_FILE_ADDRESS:
      assert(idx < BLD_MAX_ADDRS);
      STORE_ADDR(idx, chan, res);
      break;
   }
}

static INLINE uint32_t
bld_is_output_written(struct bld_context *bld, int i, int c)
{
   if (c < 0)
      return bld->outputs_written[i / 8] & (0xf << ((i * 4) % 32));
   return bld->outputs_written[i / 8] & (1 << ((i * 4 + c) % 32));
}

static void
bld_append_vp_ucp(struct bld_context *bld)
{
   struct nv_value *res[6];
   struct nv_value *ucp, *vtx, *out;
   struct nv_instruction *insn;
   int i, c;

   assert(bld->ti->prog->vp.num_ucps <= 6);

   for (c = 0; c < 4; ++c) {
      vtx = bld_fetch_global(bld, &bld->ovs[bld->hpos_index][c]);

      for (i = 0; i < bld->ti->prog->vp.num_ucps; ++i) {
         ucp = new_value(bld->pc, NV_FILE_MEM_C(15), 4);
         ucp->reg.address = i * 16 + c * 4;

         if (c == 0)
            res[i] = bld_insn_2(bld, NV_OP_MUL_F32, vtx, ucp);
         else
            res[i] = bld_insn_3(bld, NV_OP_MAD_F32, vtx, ucp, res[i]);
      }
   }

   for (i = 0; i < bld->ti->prog->vp.num_ucps; ++i) {
      (out = new_value(bld->pc, NV_FILE_MEM_V, 4))->reg.address = 0x2c0 + i * 4;
      (insn = new_instruction(bld->pc, NV_OP_EXPORT))->fixed = 1;
      nv_reference(bld->pc, insn, 0, out);
      nv_reference(bld->pc, insn, 1, res[i]);
   }
}

static void
bld_export_fp_outputs(struct bld_context *bld)
{
   struct nv_value *vals[4];
   struct nv_instruction *nvi;
   int i, c, n;

   for (i = 0; i < PIPE_MAX_SHADER_OUTPUTS; ++i) {
      if (!bld_is_output_written(bld, i, -1))
         continue;
      for (n = 0, c = 0; c < 4; ++c) {
         if (!bld_is_output_written(bld, i, c))
            continue;
         vals[n] = bld_fetch_global(bld, &bld->ovs[i][c]);
         assert(vals[n]);
         vals[n] = bld_insn_1(bld, NV_OP_MOV, vals[n]);
         vals[n++]->reg.id = bld->ti->output_loc[i][c];
      }
      assert(n);

      (nvi = new_instruction(bld->pc, NV_OP_EXPORT))->fixed = 1;
      for (c = 0; c < n; ++c)
         nv_reference(bld->pc, nvi, c, vals[c]);
   }
}

static void
bld_new_block(struct bld_context *bld, struct nv_basic_block *b)
{
   int i, c;

   bld->pc->current_block = b;

   for (i = 0; i < 4; ++i)
      bld->saved_addr[i][0] = NULL;
   for (i = 0; i < PIPE_MAX_SHADER_INPUTS; ++i)
      for (c = 0; c < 4; ++c)
         bld->saved_inputs[i][c] = NULL;

   bld->out_kind = CFG_EDGE_FORWARD;
}

static struct nv_value *
bld_interp(struct bld_context *bld, unsigned mode, struct nv_value *val)
{
   unsigned cent = mode & NVC0_INTERP_CENTROID;

   mode &= ~NVC0_INTERP_CENTROID;
   
   if (val->reg.address == 0x3fc) {
      /* gl_FrontFacing: 0/~0 to -1.0/+1.0 */
      val = bld_insn_1(bld, NV_OP_LINTERP, val);
      val->insn->flat = 1;
      val = bld_insn_2(bld, NV_OP_SHL, val, bld_imm_u32(bld, 31));
      val = bld_insn_2(bld, NV_OP_XOR, val, bld_imm_f32(bld, -1.0f));
      return val;
   } else
   if (mode == NVC0_INTERP_PERSPECTIVE) {
      val = bld_insn_2(bld, NV_OP_PINTERP, val, bld->frag_coord[3]);
   } else {
      val = bld_insn_1(bld, NV_OP_LINTERP, val);
   }

   val->insn->flat = mode == NVC0_INTERP_FLAT ? 1 : 0;
   val->insn->centroid = cent ? 1 : 0;
   return val;
}

static struct nv_value *
emit_fetch(struct bld_context *bld, const struct tgsi_full_instruction *insn,
           const unsigned s, const unsigned chan)
{
   const struct tgsi_full_src_register *src = &insn->Src[s];
   struct nv_value *res = NULL;
   struct nv_value *ptr = NULL;
   int idx, ind_idx, dim_idx;
   unsigned swz, ind_swz, sgn;

   idx = src->Register.Index;
   swz = tgsi_util_get_full_src_register_swizzle(src, chan);

   if (src->Register.Indirect) {
      ind_idx = src->Indirect.Index;
      ind_swz = tgsi_util_get_src_register_swizzle(&src->Indirect, 0);

      ptr = FETCH_ADDR(ind_idx, ind_swz);
   }

   if (src->Register.Dimension)
      dim_idx = src->Dimension.Index;
   else
      dim_idx = 0;

   switch (src->Register.File) {
   case TGSI_FILE_CONSTANT:
      assert(dim_idx < 14);
      res = new_value(bld->pc, NV_FILE_MEM_C(dim_idx), 4);
      res->reg.address = idx * 16 + swz * 4;
      res = bld_insn_1(bld, NV_OP_LD, res);
      if (ptr)
         bld_src_pointer(bld, res->insn, 1, ptr);
      break;
   case TGSI_FILE_IMMEDIATE: /* XXX: type for MOV TEMP[0], -IMM[0] */
      assert(idx < bld->ti->immd32_nr);
      res = bld_load_imm_u32(bld, bld->ti->immd32[idx * 4 + swz]);
      break;
   case TGSI_FILE_INPUT:
      assert(!src->Register.Dimension);
      if (!ptr) {
         res = bld->saved_inputs[idx][swz];
         if (res)
            break;
      }
      res = new_value(bld->pc, bld->ti->input_file, 4);
      if (ptr)
         res->reg.address = 0x80 + idx * 16 + swz * 4;
      else
         res->reg.address = bld->ti->input_loc[idx][swz];

      if (bld->pc->is_fragprog)
         res = bld_interp(bld, bld->ti->interp_mode[idx], res);
      else
         res = bld_insn_1(bld, NV_OP_VFETCH, res);

      if (ptr)
         bld_src_pointer(bld, res->insn, res->insn->src[1] ? 2 : 1, ptr);
      else
         bld->saved_inputs[idx][swz] = res;
      break;
   case TGSI_FILE_TEMPORARY:
      if (bld->ti->require_stores)
         res = bld_lmem_load(bld, ptr, idx * 4 + swz);
      else
         res = bld_fetch_global(bld, &bld->tvs[idx][swz]);
      break;
   case TGSI_FILE_ADDRESS:
      res = bld_fetch_global(bld, &bld->avs[idx][swz]);
      break;
   case TGSI_FILE_PREDICATE:
      res = bld_fetch_global(bld, &bld->pvs[idx][swz]);
      break;
   case TGSI_FILE_SYSTEM_VALUE:
      assert(bld->ti->sysval_loc[idx] < 0xf00); /* >= would mean special reg */
      res = new_value(bld->pc,
                      bld->pc->is_fragprog ? NV_FILE_MEM_V : NV_FILE_MEM_A, 4);
      res->reg.address = bld->ti->sysval_loc[idx];

      if (res->reg.file == NV_FILE_MEM_A)
         res = bld_insn_1(bld, NV_OP_VFETCH, res);
      else
         res = bld_interp(bld, NVC0_INTERP_FLAT, res);

      /* mesa doesn't do real integers yet :-(and in GL this should be S32) */
      res = bld_cvt(bld, NV_TYPE_F32, NV_TYPE_U32, res);
      break;
   default:
      NOUVEAU_ERR("illegal/unhandled src reg file: %d\n", src->Register.File);
      abort();
      break;	   
   }
   if (!res)
      return bld_undef(bld, NV_FILE_GPR);

   sgn = tgsi_util_get_full_src_register_sign_mode(src, chan);

   switch (sgn) {
   case TGSI_UTIL_SIGN_KEEP:
      break;
   case TGSI_UTIL_SIGN_CLEAR:
      res = bld_insn_1(bld, NV_OP_ABS_F32, res);
      break;
   case TGSI_UTIL_SIGN_TOGGLE:
      res = bld_insn_1(bld, NV_OP_NEG_F32, res);
      break;
   case TGSI_UTIL_SIGN_SET:
      res = bld_insn_1(bld, NV_OP_ABS_F32, res);
      res = bld_insn_1(bld, NV_OP_NEG_F32, res);
      break;
   default:
      NOUVEAU_ERR("illegal/unhandled src reg sign mode\n");
      abort();
      break;
   }

   return res;
}

static void
bld_lit(struct bld_context *bld, struct nv_value *dst0[4],
        const struct tgsi_full_instruction *insn)
{
   struct nv_value *val0 = NULL;
   unsigned mask = insn->Dst[0].Register.WriteMask;

   if (mask & ((1 << 0) | (1 << 3)))
      dst0[3] = dst0[0] = bld_load_imm_f32(bld, 1.0f);

   if (mask & (3 << 1)) {
      val0 = bld_insn_2(bld, NV_OP_MAX, emit_fetch(bld, insn, 0, 0), bld->zero);
      if (mask & (1 << 1))
         dst0[1] = val0;
   }

   if (mask & (1 << 2)) {
      struct nv_value *val1, *val3, *src1, *src3, *pred;
      struct nv_value *pos128 = bld_load_imm_f32(bld, 127.999999f);
      struct nv_value *neg128 = bld_load_imm_f32(bld, -127.999999f);

      src1 = emit_fetch(bld, insn, 0, 1);
      src3 = emit_fetch(bld, insn, 0, 3);

      pred = bld_setp(bld, NV_OP_SET_F32, NV_CC_LE, val0, bld->zero);

      val1 = bld_insn_2(bld, NV_OP_MAX_F32, src1, bld->zero);
      val3 = bld_insn_2(bld, NV_OP_MAX_F32, src3, neg128);
      val3 = bld_insn_2(bld, NV_OP_MIN_F32, val3, pos128);
      val3 = bld_pow(bld, val1, val3);

      dst0[2] = bld_insn_1(bld, NV_OP_MOV, bld->zero);
      bld_src_predicate(bld, dst0[2]->insn, 1, pred);

      dst0[2] = bld_insn_2(bld, NV_OP_SELECT, val3, dst0[2]);
   }
}

static INLINE void
describe_texture_target(unsigned target, int *dim,
                        int *array, int *cube, int *shadow)
{
   *dim = *array = *cube = *shadow = 0;

   switch (target) {
   case TGSI_TEXTURE_1D:
      *dim = 1;
      break;
   case TGSI_TEXTURE_SHADOW1D:
      *dim = *shadow = 1;
      break;
   case TGSI_TEXTURE_UNKNOWN:
   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
      *dim = 2;
      break;
   case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_SHADOWRECT:
      *dim = 2;
      *shadow = 1;
      break;
   case TGSI_TEXTURE_3D:
      *dim = 3;
      break;
   case TGSI_TEXTURE_CUBE:
      *dim = 2;
      *cube = 1;
      break;
   case TGSI_TEXTURE_1D_ARRAY:
      *dim = *array = 1;
      break;
   case TGSI_TEXTURE_2D_ARRAY:
      *dim = 2;
      *array = 1;
      break;
      /*
   case TGSI_TEXTURE_SHADOW1D_ARRAY:
      *dim = *array = *shadow = 1;
      break;
   case TGSI_TEXTURE_SHADOW2D_ARRAY:
      *dim = 2;
      *array = *shadow = 1;
      break;
   case TGSI_TEXTURE_CUBE_ARRAY:
      *dim = 2;
      *cube = *array = 1;
      break;
      */
   default:
      assert(0);
      break;
   }
}

static struct nv_value *
bld_clone(struct bld_context *bld, struct nv_instruction *nvi)
{
   struct nv_instruction *dupi = new_instruction(bld->pc, nvi->opcode);
   struct nv_instruction *next, *prev;
   int c;

   next = dupi->next;
   prev = dupi->prev;

   *dupi = *nvi;

   dupi->next = next;
   dupi->prev = prev;

   for (c = 0; c < 5 && nvi->def[c]; ++c)
      bld_def(dupi, c, new_value_like(bld->pc, nvi->def[c]));

   for (c = 0; c < 6 && nvi->src[c]; ++c) {
      dupi->src[c] = NULL;
      nv_reference(bld->pc, dupi, c, nvi->src[c]->value);
   }

   return dupi->def[0];
}

/* NOTE: proj(t0) = (t0 / w) / (tc3 / w) = tc0 / tc2 handled by optimizer */
static void
load_proj_tex_coords(struct bld_context *bld,
                     struct nv_value *t[4], int dim, int shadow,
                     const struct tgsi_full_instruction *insn)
{
   int c;
   unsigned mask = (1 << dim) - 1;

   if (shadow)
      mask |= 4; /* depth comparison value */

   t[3] = emit_fetch(bld, insn, 0, 3);
   if (t[3]->insn->opcode == NV_OP_PINTERP) {
      t[3] = bld_clone(bld, t[3]->insn);
      t[3]->insn->opcode = NV_OP_LINTERP;
      nv_reference(bld->pc, t[3]->insn, 1, NULL);
   }
   t[3] = bld_insn_1(bld, NV_OP_RCP, t[3]);

   for (c = 0; c < 4; ++c) {
      if (!(mask & (1 << c)))
         continue;
      t[c] = emit_fetch(bld, insn, 0, c);

      if (t[c]->insn->opcode != NV_OP_PINTERP)
         continue;
      mask &= ~(1 << c);

      t[c] = bld_clone(bld, t[c]->insn);
      nv_reference(bld->pc, t[c]->insn, 1, t[3]);
   }
   if (mask == 0)
      return;

   t[3] = emit_fetch(bld, insn, 0, 3);
   t[3] = bld_insn_1(bld, NV_OP_RCP, t[3]);

   for (c = 0; c < 4; ++c)
      if (mask & (1 << c))
         t[c] = bld_insn_2(bld, NV_OP_MUL_F32, t[c], t[3]);
}

/* For a quad of threads / top left, top right, bottom left, bottom right
 * pixels, do a different operation, and take src0 from a specific thread.
 */
#define QOP_ADD 0
#define QOP_SUBR 1
#define QOP_SUB 2
#define QOP_MOV1 3

#define QOP(a, b, c, d) \
   ((QOP_##a << 0) | (QOP_##b << 2) | (QOP_##c << 4) | (QOP_##d << 6))

static INLINE struct nv_value *
bld_quadop(struct bld_context *bld, ubyte qop, struct nv_value *src0, int lane,
           struct nv_value *src1, boolean wp)
{
   struct nv_value *val = bld_insn_2(bld, NV_OP_QUADOP, src0, src1);
   val->insn->lanes = lane;
   val->insn->quadop = qop;
   if (wp) {
      assert(!"quadop predicate write");
   }
   return val;
}

/* order of TGSI operands: x y z layer shadow lod/bias */
/* order of native operands: layer x y z | lod/bias shadow */
static struct nv_instruction *
emit_tex(struct bld_context *bld, uint opcode, int tic, int tsc,
         struct nv_value *dst[4], struct nv_value *arg[4],
         int dim, int array, int cube, int shadow)
{
   struct nv_value *src[4];
   struct nv_instruction *nvi, *bnd;
   int c;
   int s = 0;
   boolean lodbias = opcode == NV_OP_TXB || opcode == NV_OP_TXL;

   if (array)
      arg[dim] = bld_cvt(bld, NV_TYPE_U32, NV_TYPE_F32, arg[dim]);

   /* bind { layer x y z } and { lod/bias shadow } to adjacent regs */

   bnd = new_instruction(bld->pc, NV_OP_BIND);
   if (array) {
      src[s] = new_value(bld->pc, NV_FILE_GPR, 4);
      bld_def(bnd, s, src[s]);
      nv_reference(bld->pc, bnd, s++, arg[dim + cube]);
   }
   for (c = 0; c < dim + cube; ++c, ++s) {
      src[s] = bld_def(bnd, s, new_value(bld->pc, NV_FILE_GPR, 4));
      nv_reference(bld->pc, bnd, s, arg[c]);
   }

   if (shadow || lodbias) {
      bnd = new_instruction(bld->pc, NV_OP_BIND);

      if (lodbias) {
         src[s] = new_value(bld->pc, NV_FILE_GPR, 4);
         bld_def(bnd, 0, src[s++]);
         nv_reference(bld->pc, bnd, 0, arg[dim + cube + array + shadow]);
      }
      if (shadow) {
         src[s] = new_value(bld->pc, NV_FILE_GPR, 4);
         bld_def(bnd, lodbias, src[s++]);
         nv_reference(bld->pc, bnd, lodbias, arg[dim + cube + array]);
      }
   }

   nvi = new_instruction(bld->pc, opcode);
   for (c = 0; c < 4; ++c)
      dst[c] = bld_def(nvi, c, new_value(bld->pc, NV_FILE_GPR, 4));
   for (c = 0; c < s; ++c)
      nv_reference(bld->pc, nvi, c, src[c]);

   nvi->ext.tex.t = tic;
   nvi->ext.tex.s = tsc;
   nvi->tex_mask = 0xf;
   nvi->tex_cube = cube;
   nvi->tex_dim = dim;
   nvi->tex_cube = cube;
   nvi->tex_shadow = shadow;
   nvi->tex_array = array;
   nvi->tex_live = 0;

   return nvi;
}

static void
bld_tex(struct bld_context *bld, struct nv_value *dst0[4],
        const struct tgsi_full_instruction *insn)
{
   struct nv_value *t[4], *s[3];
   uint opcode = translate_opcode(insn->Instruction.Opcode);
   int c, dim, array, cube, shadow;
   const int lodbias = opcode == NV_OP_TXB || opcode == NV_OP_TXL;
   const int tic = insn->Src[1].Register.Index;
   const int tsc = tic;

   describe_texture_target(insn->Texture.Texture, &dim, &array, &cube, &shadow);

   assert(dim + array + shadow + lodbias <= 5);

   if (!cube && !array && insn->Instruction.Opcode == TGSI_OPCODE_TXP)
      load_proj_tex_coords(bld, t, dim, shadow, insn);
   else {
      for (c = 0; c < dim + cube + array; ++c)
         t[c] = emit_fetch(bld, insn, 0, c);
      if (shadow)
         t[c] = emit_fetch(bld, insn, 0, MAX2(c, 2));
   }

   if (cube) {
      for (c = 0; c < 3; ++c)
         s[c] = bld_insn_1(bld, NV_OP_ABS_F32, t[c]);

      s[0] = bld_insn_2(bld, NV_OP_MAX_F32, s[0], s[1]);
      s[0] = bld_insn_2(bld, NV_OP_MAX_F32, s[0], s[2]);
      s[0] = bld_insn_1(bld, NV_OP_RCP, s[0]);

      for (c = 0; c < 3; ++c)
         t[c] = bld_insn_2(bld, NV_OP_MUL_F32, t[c], s[0]);
   }

   if (lodbias)
      t[dim + cube + array + shadow] = emit_fetch(bld, insn, 0, 3);

   emit_tex(bld, opcode, tic, tsc, dst0, t, dim, array, cube, shadow);
}

static INLINE struct nv_value *
bld_dot(struct bld_context *bld, const struct tgsi_full_instruction *insn,
        int n)
{
   struct nv_value *dotp, *src0, *src1;
   int c;

   src0 = emit_fetch(bld, insn, 0, 0);
   src1 = emit_fetch(bld, insn, 1, 0);
   dotp = bld_insn_2(bld, NV_OP_MUL_F32, src0, src1);

   for (c = 1; c < n; ++c) {
      src0 = emit_fetch(bld, insn, 0, c);
      src1 = emit_fetch(bld, insn, 1, c);
      dotp = bld_insn_3(bld, NV_OP_MAD_F32, src0, src1, dotp);
   }
   return dotp;
}

#define FOR_EACH_DST0_ENABLED_CHANNEL(chan, inst) \
   for (chan = 0; chan < 4; ++chan)               \
      if ((inst)->Dst[0].Register.WriteMask & (1 << chan))

static void
bld_instruction(struct bld_context *bld,
                const struct tgsi_full_instruction *insn)
{
   struct nv_value *src0;
   struct nv_value *src1;
   struct nv_value *src2;
   struct nv_value *dst0[4] = { NULL };
   struct nv_value *temp;
   int c;
   uint opcode = translate_opcode(insn->Instruction.Opcode);
   uint8_t mask = insn->Dst[0].Register.WriteMask;

#if NV50_DEBUG & NV50_DEBUG_PROG_IR
   debug_printf("bld_instruction:"); tgsi_dump_instruction(insn, 1);
#endif
	
   switch (insn->Instruction.Opcode) {
   case TGSI_OPCODE_ADD:
   case TGSI_OPCODE_MAX:
   case TGSI_OPCODE_MIN:
   case TGSI_OPCODE_MUL:
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn) {
         src0 = emit_fetch(bld, insn, 0, c);
         src1 = emit_fetch(bld, insn, 1, c);
         dst0[c] = bld_insn_2(bld, opcode, src0, src1);
      }
      break;
   case TGSI_OPCODE_ARL:
      src1 = bld_imm_u32(bld, 4);
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn) {
         src0 = emit_fetch(bld, insn, 0, c);
         src0 = bld_insn_1(bld, NV_OP_FLOOR, src0);
         src0->insn->ext.cvt.d = NV_TYPE_S32;
         src0->insn->ext.cvt.s = NV_TYPE_F32;
         dst0[c] = bld_insn_2(bld, NV_OP_SHL, src0, src1);
      }
      break;
   case TGSI_OPCODE_CMP:
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn) {
         src0 = emit_fetch(bld, insn, 0, c);
         src1 = emit_fetch(bld, insn, 1, c);
         src2 = emit_fetch(bld, insn, 2, c);
         dst0[c] = bld_insn_3(bld, NV_OP_SLCT_F32, src1, src2, src0);
         dst0[c]->insn->set_cond = NV_CC_LT;
      }
      break;
   case TGSI_OPCODE_COS:
   case TGSI_OPCODE_SIN:
      src0 = emit_fetch(bld, insn, 0, 0);
      temp = bld_insn_1(bld, NV_OP_PRESIN, src0);
      if (insn->Dst[0].Register.WriteMask & 7)
         temp = bld_insn_1(bld, opcode, temp);
      for (c = 0; c < 3; ++c)
         if (insn->Dst[0].Register.WriteMask & (1 << c))
            dst0[c] = temp;
      if (!(insn->Dst[0].Register.WriteMask & (1 << 3)))
         break;
      src0 = emit_fetch(bld, insn, 0, 3);
      temp = bld_insn_1(bld, NV_OP_PRESIN, src0);
      dst0[3] = bld_insn_1(bld, opcode, temp);
      break;
   case TGSI_OPCODE_DP2:
      temp = bld_dot(bld, insn, 2);
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn)
         dst0[c] = temp;
      break;
   case TGSI_OPCODE_DP3:
      temp = bld_dot(bld, insn, 3);
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn)
         dst0[c] = temp;
      break;
   case TGSI_OPCODE_DP4:
      temp = bld_dot(bld, insn, 4);
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn)
         dst0[c] = temp;
      break;
   case TGSI_OPCODE_DPH:
      src0 = bld_dot(bld, insn, 3);
      src1 = emit_fetch(bld, insn, 1, 3);
      temp = bld_insn_2(bld, NV_OP_ADD_F32, src0, src1);
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn)
         dst0[c] = temp;
      break;
   case TGSI_OPCODE_DST:
      if (insn->Dst[0].Register.WriteMask & 1)
         dst0[0] = bld_imm_f32(bld, 1.0f);
      if (insn->Dst[0].Register.WriteMask & 2) {
         src0 = emit_fetch(bld, insn, 0, 1);
         src1 = emit_fetch(bld, insn, 1, 1);
         dst0[1] = bld_insn_2(bld, NV_OP_MUL_F32, src0, src1);
      }
      if (insn->Dst[0].Register.WriteMask & 4)
         dst0[2] = emit_fetch(bld, insn, 0, 2);
      if (insn->Dst[0].Register.WriteMask & 8)
         dst0[3] = emit_fetch(bld, insn, 1, 3);
      break;
   case TGSI_OPCODE_EXP:
      src0 = emit_fetch(bld, insn, 0, 0);
      temp = bld_insn_1(bld, NV_OP_FLOOR, src0);

      if (insn->Dst[0].Register.WriteMask & 2)
         dst0[1] = bld_insn_2(bld, NV_OP_SUB_F32, src0, temp);
      if (insn->Dst[0].Register.WriteMask & 1) {
         temp = bld_insn_1(bld, NV_OP_PREEX2, temp);
         dst0[0] = bld_insn_1(bld, NV_OP_EX2, temp);
      }
      if (insn->Dst[0].Register.WriteMask & 4) {
         temp = bld_insn_1(bld, NV_OP_PREEX2, src0);
         dst0[2] = bld_insn_1(bld, NV_OP_EX2, temp);
      }
      if (insn->Dst[0].Register.WriteMask & 8)
         dst0[3] = bld_imm_f32(bld, 1.0f);
      break;
   case TGSI_OPCODE_EX2:
      src0 = emit_fetch(bld, insn, 0, 0);
      temp = bld_insn_1(bld, NV_OP_PREEX2, src0);
      temp = bld_insn_1(bld, NV_OP_EX2, temp);
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn)
         dst0[c] = temp;
      break;
   case TGSI_OPCODE_FRC:
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn) {
         src0 = emit_fetch(bld, insn, 0, c);
         dst0[c] = bld_insn_1(bld, NV_OP_FLOOR, src0);
         dst0[c] = bld_insn_2(bld, NV_OP_SUB_F32, src0, dst0[c]);
      }
      break;
   case TGSI_OPCODE_KIL:
      for (c = 0; c < 4; ++c)
         bld_kil(bld, emit_fetch(bld, insn, 0, c));
      break;
   case TGSI_OPCODE_KILP:
      (new_instruction(bld->pc, NV_OP_KIL))->fixed = 1;
      break;
   case TGSI_OPCODE_IF:
   {
      struct nv_basic_block *b = new_basic_block(bld->pc);
      struct nv_value *pred = emit_fetch(bld, insn, 0, 0);

      assert(bld->cond_lvl < BLD_MAX_COND_NESTING);

      nvc0_bblock_attach(bld->pc->current_block, b, CFG_EDGE_FORWARD);

      bld->join_bb[bld->cond_lvl] = bld->pc->current_block;
      bld->cond_bb[bld->cond_lvl] = bld->pc->current_block;

      if (pred->insn && NV_BASEOP(pred->insn->opcode) == NV_OP_SET) {
         pred = bld_clone(bld, pred->insn);
         pred->reg.size = 1;
         pred->reg.file = NV_FILE_PRED;
         if (pred->insn->opcode == NV_OP_FSET_F32)
            pred->insn->opcode = NV_OP_SET_F32;
      } else {
         pred = bld_setp(bld, NV_OP_SET_U32, NV_CC_NE | NV_CC_U,
                         pred, bld->zero);
      }
      assert(!mask);

      bld_flow(bld, NV_OP_BRA, pred, NV_CC_NOT_P, NULL, (bld->cond_lvl == 0));

      ++bld->cond_lvl;
      bld_new_block(bld, b);
   }
      break;
   case TGSI_OPCODE_ELSE:
   {
      struct nv_basic_block *b = new_basic_block(bld->pc);

      --bld->cond_lvl;
      nvc0_bblock_attach(bld->join_bb[bld->cond_lvl], b, CFG_EDGE_FORWARD);

      bld->cond_bb[bld->cond_lvl]->exit->target = b;
      bld->cond_bb[bld->cond_lvl] = bld->pc->current_block;

      new_instruction(bld->pc, NV_OP_BRA)->terminator = 1;

      ++bld->cond_lvl;
      bld_new_block(bld, b);
   }
      break;
   case TGSI_OPCODE_ENDIF:
   {
      struct nv_basic_block *b = new_basic_block(bld->pc);

      if (!nvc0_bblock_is_terminated(bld->pc->current_block))
         bld_flow(bld, NV_OP_BRA, NULL, NV_CC_P, b, FALSE);

      --bld->cond_lvl;
      nvc0_bblock_attach(bld->pc->current_block, b, bld->out_kind);
      nvc0_bblock_attach(bld->cond_bb[bld->cond_lvl], b, CFG_EDGE_FORWARD);

      bld->cond_bb[bld->cond_lvl]->exit->target = b;

      bld_new_block(bld, b);

      if (!bld->cond_lvl && bld->join_bb[bld->cond_lvl]) {
         bld->join_bb[bld->cond_lvl]->exit->prev->target = b;
         new_instruction(bld->pc, NV_OP_JOIN)->join = 1;
      }
   }
      break;
   case TGSI_OPCODE_BGNLOOP:
   {
      struct nv_basic_block *bl = new_basic_block(bld->pc);
      struct nv_basic_block *bb = new_basic_block(bld->pc);

      assert(bld->loop_lvl < BLD_MAX_LOOP_NESTING);

      bld->loop_bb[bld->loop_lvl] = bl;
      bld->brkt_bb[bld->loop_lvl] = bb;

      nvc0_bblock_attach(bld->pc->current_block, bl, CFG_EDGE_LOOP_ENTER);

      bld_new_block(bld, bld->loop_bb[bld->loop_lvl++]);

      if (bld->loop_lvl == bld->pc->loop_nesting_bound)
         bld->pc->loop_nesting_bound++;

      bld_clear_def_use(&bld->tvs[0][0], BLD_MAX_TEMPS, bld->loop_lvl);
      bld_clear_def_use(&bld->avs[0][0], BLD_MAX_ADDRS, bld->loop_lvl);
      bld_clear_def_use(&bld->pvs[0][0], BLD_MAX_PREDS, bld->loop_lvl);
   }
      break;
   case TGSI_OPCODE_BRK:
   {
      struct nv_basic_block *bb = bld->brkt_bb[bld->loop_lvl - 1];

      bld_flow(bld, NV_OP_BRA, NULL, NV_CC_P, bb, FALSE);

      if (bld->out_kind == CFG_EDGE_FORWARD) /* else we already had BRK/CONT */
         nvc0_bblock_attach(bld->pc->current_block, bb, CFG_EDGE_LOOP_LEAVE);

      bld->out_kind = CFG_EDGE_FAKE;
   }
      break;
   case TGSI_OPCODE_CONT:
   {
      struct nv_basic_block *bb = bld->loop_bb[bld->loop_lvl - 1];

      bld_flow(bld, NV_OP_BRA, NULL, NV_CC_P, bb, FALSE);

      nvc0_bblock_attach(bld->pc->current_block, bb, CFG_EDGE_BACK);

      if ((bb = bld->join_bb[bld->cond_lvl - 1])) {
         bld->join_bb[bld->cond_lvl - 1] = NULL;
         nvc0_insn_delete(bb->exit->prev);
      }
      bld->out_kind = CFG_EDGE_FAKE;
   }
      break;
   case TGSI_OPCODE_ENDLOOP:
   {
      struct nv_basic_block *bb = bld->loop_bb[bld->loop_lvl - 1];

      if (bld->out_kind != CFG_EDGE_FAKE) { /* else we already had BRK/CONT */
         bld_flow(bld, NV_OP_BRA, NULL, NV_CC_P, bb, FALSE);

         nvc0_bblock_attach(bld->pc->current_block, bb, CFG_EDGE_BACK);
      }

      bld_loop_end(bld, bb); /* replace loop-side operand of the phis */

      bld_new_block(bld, bld->brkt_bb[--bld->loop_lvl]);
   }
      break;
   case TGSI_OPCODE_ABS:
   case TGSI_OPCODE_CEIL:
   case TGSI_OPCODE_FLR:
   case TGSI_OPCODE_TRUNC:
   case TGSI_OPCODE_DDX:
   case TGSI_OPCODE_DDY:
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn) {
         src0 = emit_fetch(bld, insn, 0, c);
         dst0[c] = bld_insn_1(bld, opcode, src0);
      }	   
      break;
   case TGSI_OPCODE_LIT:
      bld_lit(bld, dst0, insn);
      break;
   case TGSI_OPCODE_LRP:
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn) {
         src0 = emit_fetch(bld, insn, 0, c);
         src1 = emit_fetch(bld, insn, 1, c);
         src2 = emit_fetch(bld, insn, 2, c);
         dst0[c] = bld_insn_2(bld, NV_OP_SUB_F32, src1, src2);
         dst0[c] = bld_insn_3(bld, NV_OP_MAD_F32, dst0[c], src0, src2);
      }
      break;
   case TGSI_OPCODE_MOV:
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn)
         dst0[c] = emit_fetch(bld, insn, 0, c);
      break;
   case TGSI_OPCODE_MAD:
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn) {
         src0 = emit_fetch(bld, insn, 0, c);
         src1 = emit_fetch(bld, insn, 1, c);
         src2 = emit_fetch(bld, insn, 2, c);
         dst0[c] = bld_insn_3(bld, opcode, src0, src1, src2);
      }
      break;
   case TGSI_OPCODE_POW:
      src0 = emit_fetch(bld, insn, 0, 0);
      src1 = emit_fetch(bld, insn, 1, 0);
      temp = bld_pow(bld, src0, src1);
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn)
         dst0[c] = temp;
      break;
   case TGSI_OPCODE_LOG:
      src0 = emit_fetch(bld, insn, 0, 0);
      src0 = bld_insn_1(bld, NV_OP_ABS_F32, src0);
      temp = bld_insn_1(bld, NV_OP_LG2, src0);
      dst0[2] = temp;
      if (insn->Dst[0].Register.WriteMask & 3) {
         temp = bld_insn_1(bld, NV_OP_FLOOR, temp);
         dst0[0] = temp;
      }
      if (insn->Dst[0].Register.WriteMask & 2) {
         temp = bld_insn_1(bld, NV_OP_PREEX2, temp);
         temp = bld_insn_1(bld, NV_OP_EX2, temp);
         temp = bld_insn_1(bld, NV_OP_RCP, temp);
         dst0[1] = bld_insn_2(bld, NV_OP_MUL_F32, src0, temp);
      }
      if (insn->Dst[0].Register.WriteMask & 8)
         dst0[3] = bld_imm_f32(bld, 1.0f);
      break;
   case TGSI_OPCODE_RCP:
   case TGSI_OPCODE_LG2:
      src0 = emit_fetch(bld, insn, 0, 0);
      temp = bld_insn_1(bld, opcode, src0);
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn)
         dst0[c] = temp;
      break;
   case TGSI_OPCODE_RSQ:
      src0 = emit_fetch(bld, insn, 0, 0);
      temp = bld_insn_1(bld, NV_OP_ABS_F32, src0);
      temp = bld_insn_1(bld, NV_OP_RSQ, temp);
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn)
         dst0[c] = temp;
      break;
   case TGSI_OPCODE_SLT:
   case TGSI_OPCODE_SGE:
   case TGSI_OPCODE_SEQ:
   case TGSI_OPCODE_SGT:
   case TGSI_OPCODE_SLE:
   case TGSI_OPCODE_SNE:
   case TGSI_OPCODE_ISLT:
   case TGSI_OPCODE_ISGE:
   case TGSI_OPCODE_USEQ:
   case TGSI_OPCODE_USGE:
   case TGSI_OPCODE_USLT:
   case TGSI_OPCODE_USNE:
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn) {
         src0 = emit_fetch(bld, insn, 0, c);
         src1 = emit_fetch(bld, insn, 1, c);
         dst0[c] = bld_insn_2(bld, opcode, src0, src1);
         dst0[c]->insn->set_cond = translate_setcc(insn->Instruction.Opcode);
      }
      break;
   case TGSI_OPCODE_SCS:
      if (insn->Dst[0].Register.WriteMask & 0x3) {
         src0 = emit_fetch(bld, insn, 0, 0);
         temp = bld_insn_1(bld, NV_OP_PRESIN, src0);
         if (insn->Dst[0].Register.WriteMask & 0x1)
            dst0[0] = bld_insn_1(bld, NV_OP_COS, temp);
         if (insn->Dst[0].Register.WriteMask & 0x2)
            dst0[1] = bld_insn_1(bld, NV_OP_SIN, temp);
      }
      if (insn->Dst[0].Register.WriteMask & 0x4)
         dst0[2] = bld_imm_f32(bld, 0.0f);
      if (insn->Dst[0].Register.WriteMask & 0x8)
         dst0[3] = bld_imm_f32(bld, 1.0f);
      break;
   case TGSI_OPCODE_SSG:
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn) { /* XXX: set lt, set gt, sub */
         src0 = emit_fetch(bld, insn, 0, c);
         src1 = bld_insn_2(bld, NV_OP_FSET_F32, src0, bld->zero);
         src2 = bld_insn_2(bld, NV_OP_FSET_F32, src0, bld->zero);
         src1->insn->set_cond = NV_CC_GT;
         src2->insn->set_cond = NV_CC_LT;
         dst0[c] = bld_insn_2(bld, NV_OP_SUB_F32, src1, src2);
      }
      break;
   case TGSI_OPCODE_SUB:
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn) {
         src0 = emit_fetch(bld, insn, 0, c);
         src1 = emit_fetch(bld, insn, 1, c);
         dst0[c] = bld_insn_2(bld, NV_OP_SUB_F32, src0, src1);
      }
      break;
   case TGSI_OPCODE_TEX:
   case TGSI_OPCODE_TXB:
   case TGSI_OPCODE_TXL:
   case TGSI_OPCODE_TXP:
      bld_tex(bld, dst0, insn);
      break;
   case TGSI_OPCODE_XPD:
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn) {
         if (c == 3) {
            dst0[3] = bld_imm_f32(bld, 1.0f);
            break;
         }
         src0 = emit_fetch(bld, insn, 1, (c + 1) % 3);
         src1 = emit_fetch(bld, insn, 0, (c + 2) % 3);
         dst0[c] = bld_insn_2(bld, NV_OP_MUL_F32, src0, src1);

         src0 = emit_fetch(bld, insn, 0, (c + 1) % 3);
         src1 = emit_fetch(bld, insn, 1, (c + 2) % 3);
         dst0[c] = bld_insn_3(bld, NV_OP_MAD_F32, src0, src1, dst0[c]);

         dst0[c]->insn->src[2]->mod ^= NV_MOD_NEG;
      }
      break;
   case TGSI_OPCODE_RET:
      (new_instruction(bld->pc, NV_OP_RET))->fixed = 1;
      break;
   case TGSI_OPCODE_END:
      /* VP outputs are exported in-place as scalars, optimization later */
      if (bld->pc->is_fragprog)
         bld_export_fp_outputs(bld);
      if (bld->ti->append_ucp)
         bld_append_vp_ucp(bld);
      return;
   default:
      NOUVEAU_ERR("unhandled opcode %u\n", insn->Instruction.Opcode);
      abort();
      return;
   }

   if (insn->Dst[0].Register.File == TGSI_FILE_OUTPUT &&
       !bld->pc->is_fragprog) {
      struct nv_instruction *mi = NULL;
      uint size;

      if (bld->ti->append_ucp) {
         if (bld->ti->output_loc[insn->Dst[0].Register.Index][0] == 0x70) {
            bld->hpos_index = insn->Dst[0].Register.Index;
            for (c = 0; c < 4; ++c)
               if (mask & (1 << c))
                  STORE_OUTP(insn->Dst[0].Register.Index, c, dst0[c]);
         }
      }

      for (c = 0; c < 4; ++c)
         if (mask & (1 << c))
            if ((dst0[c]->reg.file == NV_FILE_IMM) ||
                (dst0[c]->reg.file == NV_FILE_GPR && dst0[c]->reg.id == 63))
               dst0[c] = bld_insn_1(bld, NV_OP_MOV, dst0[c]);

      c = 0;
      if ((mask & 0x3) == 0x3) {
         mask &= ~0x3;
         size = 8;
         mi = bld_insn_2(bld, NV_OP_BIND, dst0[0], dst0[1])->insn;
      }
      if ((mask & 0xc) == 0xc) {
         mask &= ~0xc;
         if (mi) {
            size = 16;
            nv_reference(bld->pc, mi, 2, dst0[2]);
            nv_reference(bld->pc, mi, 3, dst0[3]);
         } else {
            c = 2;
            size = 8;
            mi = bld_insn_2(bld, NV_OP_BIND, dst0[2], dst0[3])->insn;
         }
      } else
      if (mi && (mask & 0x4)) {
         size = 12;
         mask &= ~0x4;
         nv_reference(bld->pc, mi, 2, dst0[2]);
      }

      if (mi) {
         struct nv_instruction *ex = new_instruction(bld->pc, NV_OP_EXPORT);
         int s;

         nv_reference(bld->pc, ex, 0, new_value(bld->pc, NV_FILE_MEM_V, 4));
         nv_reference(bld->pc, ex, 1, mi->def[0]);

         for (s = 1; s < size / 4; ++s) {
            bld_def(mi, s, new_value(bld->pc, NV_FILE_GPR, 4));
            nv_reference(bld->pc, ex, s + 1, mi->def[s]);
         }

         ex->fixed = 1;
         ex->src[0]->value->reg.size = size;
         ex->src[0]->value->reg.address =
            bld->ti->output_loc[insn->Dst[0].Register.Index][c];
      }
   }

   for (c = 0; c < 4; ++c)
      if (mask & (1 << c))
         emit_store(bld, insn, c, dst0[c]);
}

static INLINE void
bld_free_registers(struct bld_register *base, int n)
{
   int i, c;

   for (i = 0; i < n; ++i)
      for (c = 0; c < 4; ++c)
         util_dynarray_fini(&base[i * 4 + c].vals);
}

int
nvc0_tgsi_to_nc(struct nv_pc *pc, struct nvc0_translation_info *ti)
{
   struct bld_context *bld = CALLOC_STRUCT(bld_context);
   unsigned ip;

   pc->root[0] = pc->current_block = new_basic_block(pc);

   bld->pc = pc;
   bld->ti = ti;

   pc->loop_nesting_bound = 1;

   bld->zero = new_value(pc, NV_FILE_GPR, 4);
   bld->zero->reg.id = 63;

   if (pc->is_fragprog) {
      struct nv_value *mem = new_value(pc, NV_FILE_MEM_V, 4);
      mem->reg.address = 0x7c;

      bld->frag_coord[3] = bld_insn_1(bld, NV_OP_LINTERP, mem);
      bld->frag_coord[3] = bld_insn_1(bld, NV_OP_RCP, bld->frag_coord[3]);
   }

   for (ip = 0; ip < ti->num_insns; ++ip)
      bld_instruction(bld, &ti->insns[ip]);

   bld_free_registers(&bld->tvs[0][0], BLD_MAX_TEMPS);
   bld_free_registers(&bld->avs[0][0], BLD_MAX_ADDRS);
   bld_free_registers(&bld->pvs[0][0], BLD_MAX_PREDS);
   bld_free_registers(&bld->ovs[0][0], PIPE_MAX_SHADER_OUTPUTS);

   FREE(bld);
   return 0;
}

/* If a variable is assigned in a loop, replace all references to the value
 * from outside the loop with a phi value.
 */
static void
bld_replace_value(struct nv_pc *pc, struct nv_basic_block *b,
                  struct nv_value *old_val,
                  struct nv_value *new_val)
{
   struct nv_instruction *nvi;

   for (nvi = b->phi ? b->phi : b->entry; nvi; nvi = nvi->next) {
      int s;
      for (s = 0; s < 6 && nvi->src[s]; ++s)
         if (nvi->src[s]->value == old_val)
            nv_reference(pc, nvi, s, new_val);
   }

   b->pass_seq = pc->pass_seq;

   if (b->out[0] && b->out[0]->pass_seq < pc->pass_seq)
      bld_replace_value(pc, b->out[0], old_val, new_val);

   if (b->out[1] && b->out[1]->pass_seq < pc->pass_seq)
      bld_replace_value(pc, b->out[1], old_val, new_val);
}
