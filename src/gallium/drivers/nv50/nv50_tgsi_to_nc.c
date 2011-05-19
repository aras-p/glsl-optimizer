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

#include "nv50_context.h"
#include "nv50_pc.h"

#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"

#include "tgsi/tgsi_dump.h"

#define BLD_MAX_TEMPS 64
#define BLD_MAX_ADDRS 4
#define BLD_MAX_PREDS 4
#define BLD_MAX_IMMDS 128

#define BLD_MAX_COND_NESTING 8
#define BLD_MAX_LOOP_NESTING 4
#define BLD_MAX_CALL_NESTING 2

/* collects all values assigned to the same TGSI register */
struct bld_value_stack {
   struct nv_value *top;
   struct nv_value **body;
   unsigned size;
   uint16_t loop_use; /* 1 bit per loop level, indicates if used/defd */
   uint16_t loop_def;
};

static INLINE void
bld_vals_push_val(struct bld_value_stack *stk, struct nv_value *val)
{
   assert(!stk->size || (stk->body[stk->size - 1] != val));

   if (!(stk->size % 8)) {
      unsigned old_sz = (stk->size + 0) * sizeof(struct nv_value *);
      unsigned new_sz = (stk->size + 8) * sizeof(struct nv_value *);
      stk->body = (struct nv_value **)REALLOC(stk->body, old_sz, new_sz);
   }
   stk->body[stk->size++] = val;
}

static INLINE boolean
bld_vals_del_val(struct bld_value_stack *stk, struct nv_value *val)
{
   unsigned i;

   for (i = stk->size; i > 0; --i)
      if (stk->body[i - 1] == val)
         break;
   if (!i)
      return FALSE;

   if (i != stk->size)
      stk->body[i - 1] = stk->body[stk->size - 1];

   --stk->size; /* XXX: old size in REALLOC */
   return TRUE;
}

static INLINE void
bld_vals_push(struct bld_value_stack *stk)
{
   bld_vals_push_val(stk, stk->top);
   stk->top = NULL;
}

static INLINE void
bld_push_values(struct bld_value_stack *stacks, int n)
{
   int i, c;

   for (i = 0; i < n; ++i)
      for (c = 0; c < 4; ++c)
         if (stacks[i * 4 + c].top)
            bld_vals_push(&stacks[i * 4 + c]);
}

struct bld_context {
   struct nv50_translation_info *ti;

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

   struct bld_value_stack tvs[BLD_MAX_TEMPS][4]; /* TGSI_FILE_TEMPORARY */
   struct bld_value_stack avs[BLD_MAX_ADDRS][4]; /* TGSI_FILE_ADDRESS */
   struct bld_value_stack pvs[BLD_MAX_PREDS][4]; /* TGSI_FILE_PREDICATE */
   struct bld_value_stack ovs[PIPE_MAX_SHADER_OUTPUTS][4];

   uint32_t outputs_written[(PIPE_MAX_SHADER_OUTPUTS + 7) / 8];

   struct nv_value *frgcrd[4];
   struct nv_value *sysval[4];

   /* wipe on new BB */
   struct nv_value *saved_addr[4][2];
   struct nv_value *saved_inputs[128];
   struct nv_value *saved_immd[BLD_MAX_IMMDS];
   uint num_immds;
};

static INLINE ubyte
bld_stack_file(struct bld_context *bld, struct bld_value_stack *stk)
{
   if (stk < &bld->avs[0][0])
      return NV_FILE_GPR;
   else
   if (stk < &bld->pvs[0][0])
      return NV_FILE_ADDR;
   else
   if (stk < &bld->ovs[0][0])
      return NV_FILE_FLAGS;
   else
      return NV_FILE_OUT;
}

static INLINE struct nv_value *
bld_fetch(struct bld_context *bld, struct bld_value_stack *stk, int i, int c)
{
   stk[i * 4 + c].loop_use |= 1 << bld->loop_lvl;

   return stk[i * 4 + c].top;
}

static struct nv_value *
bld_loop_phi(struct bld_context *, struct bld_value_stack *, struct nv_value *);

/* If a variable is defined in a loop without prior use, we don't need
 * a phi in the loop header to account for backwards flow.
 *
 * However, if this variable is then also used outside the loop, we do
 * need a phi after all. But we must not use this phi's def inside the
 * loop, so we can eliminate the phi if it is unused later.
 */
static INLINE void
bld_store(struct bld_context *bld, struct bld_value_stack *stk, int i, int c,
          struct nv_value *val)
{
   const uint16_t m = 1 << bld->loop_lvl;

   stk = &stk[i * 4 + c];

   if (bld->loop_lvl && !(m & (stk->loop_def | stk->loop_use)))
      bld_loop_phi(bld, stk, val);

   stk->top = val;
   stk->loop_def |= 1 << bld->loop_lvl;
}

static INLINE void
bld_clear_def_use(struct bld_value_stack *stk, int n, int lvl)
{
   int i;
   const uint16_t mask = ~(1 << lvl);

   for (i = 0; i < n * 4; ++i) {
      stk[i].loop_def &= mask;
      stk[i].loop_use &= mask;
   }
}

#define FETCH_TEMP(i, c)    bld_fetch(bld, &bld->tvs[0][0], i, c)
#define STORE_TEMP(i, c, v) bld_store(bld, &bld->tvs[0][0], i, c, (v))
#define FETCH_ADDR(i, c)    bld_fetch(bld, &bld->avs[0][0], i, c)
#define STORE_ADDR(i, c, v) bld_store(bld, &bld->avs[0][0], i, c, (v))
#define FETCH_PRED(i, c)    bld_fetch(bld, &bld->pvs[0][0], i, c)
#define STORE_PRED(i, c, v) bld_store(bld, &bld->pvs[0][0], i, c, (v))

#define STORE_OUTR(i, c, v)                                         \
   do {                                                             \
      bld->ovs[i][c].top = (v);                                     \
      bld->outputs_written[(i) / 8] |= 1 << (((i) * 4 + (c)) % 32); \
   } while (0)

static INLINE void
bld_warn_uninitialized(struct bld_context *bld, int kind,
                       struct bld_value_stack *stk, struct nv_basic_block *b)
{
#if NV50_DEBUG & NV50_DEBUG_PROG_IR
   long i = (stk - &bld->tvs[0][0]) / 4;
   long c = (stk - &bld->tvs[0][0]) & 3;

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
find_by_bb(struct bld_value_stack *stack, struct nv_basic_block *b)
{
   int i;

   if (stack->top && stack->top->insn->bb == b)
      return stack->top;

   for (i = stack->size - 1; i >= 0; --i)
      if (stack->body[i]->insn->bb == b)
         return stack->body[i];
   return NULL;
}

/* fetch value from stack that was defined in the specified basic block,
 * or search for first definitions in all of its predecessors
 */
static void
fetch_by_bb(struct bld_value_stack *stack,
            struct nv_value **vals, int *n,
            struct nv_basic_block *b)
{
   int i;
   struct nv_value *val;

   assert(*n < 16); /* MAX_COND_NESTING */

   val = find_by_bb(stack, b);
   if (val) {
      for (i = 0; i < *n; ++i)
         if (vals[i] == val)
            return;
      vals[(*n)++] = val;
      return;
   }
   for (i = 0; i < b->num_in; ++i)
      if (!IS_WALL_EDGE(b->in_kind[i]))
         fetch_by_bb(stack, vals, n, b->in[i]);
}

static INLINE boolean
nvbb_is_terminated(struct nv_basic_block *bb)
{
   return bb->exit && bb->exit->is_terminator;
}

static INLINE struct nv_value *
bld_load_imm_u32(struct bld_context *bld, uint32_t u);

static INLINE struct nv_value *
bld_undef(struct bld_context *bld, ubyte file)
{
   struct nv_instruction *nvi = new_instruction(bld->pc, NV_OP_UNDEF);

   return bld_def(nvi, 0, new_value(bld->pc, file, NV_TYPE_U32));
}

static struct nv_value *
bld_phi(struct bld_context *bld, struct nv_basic_block *b,
        struct bld_value_stack *stack)
{
   struct nv_basic_block *in;
   struct nv_value *vals[16] = { 0 };
   struct nv_value *val;
   struct nv_instruction *phi;
   int i, j, n;

   do {
      i = n = 0;
      fetch_by_bb(stack, vals, &n, b);

      if (!n) {
         bld_warn_uninitialized(bld, 0, stack, b);
         return NULL;
      }

      if (n == 1) {
         if (nvbb_dominated_by(b, vals[0]->insn->bb))
            break;

         bld_warn_uninitialized(bld, 1, stack, b);

         /* back-tracking to insert missing value of other path */
         in = b;
         while (in->in[0]) {
            if (in->num_in == 1) {
               in = in->in[0];
            } else {
               if (!nvbb_reachable_by(in->in[0], vals[0]->insn->bb, b))
                  in = in->in[0];
               else
               if (!nvbb_reachable_by(in->in[1], vals[0]->insn->bb, b))
                  in = in->in[1];
               else
                  in = in->in[0];
            }
         }
         bld->pc->current_block = in;

         /* should make this a no-op */
         bld_vals_push_val(stack, bld_undef(bld, vals[0]->reg.file));
         continue;
      }

      for (i = 0; i < n; ++i) {
         /* if value dominates b, continue to the redefinitions */
         if (nvbb_dominated_by(b, vals[i]->insn->bb))
            continue;

         /* if value dominates any in-block, b should be the dom frontier */
         for (j = 0; j < b->num_in; ++j)
            if (nvbb_dominated_by(b->in[j], vals[i]->insn->bb))
               break;
         /* otherwise, find the dominance frontier and put the phi there */
         if (j == b->num_in) {
            in = nvbb_dom_frontier(vals[i]->insn->bb);
            val = bld_phi(bld, in, stack);
            bld_vals_push_val(stack, val);
            break;
         }
      }
   } while(i < n);

   bld->pc->current_block = b;

   if (n == 1)
      return vals[0];

   phi = new_instruction(bld->pc, NV_OP_PHI);

   bld_def(phi, 0, new_value(bld->pc, vals[0]->reg.file, vals[0]->reg.type));
   for (i = 0; i < n; ++i)
      phi->src[i] = new_ref(bld->pc, vals[i]);

   return phi->def[0];
}

/* Insert a phi function in the loop header.
 * For nested loops, we need to insert phi functions in all the outer
 * loop headers if they don't have one yet.
 *
 * @def: redefinition from inside loop, or NULL if to be replaced later
 */
static struct nv_value *
bld_loop_phi(struct bld_context *bld, struct bld_value_stack *stack,
             struct nv_value *def)
{
   struct nv_instruction *phi;
   struct nv_basic_block *bb = bld->pc->current_block;
   struct nv_value *val = NULL;

   if (bld->loop_lvl > 1) {
      --bld->loop_lvl;
      if (!((stack->loop_def | stack->loop_use) & (1 << bld->loop_lvl)))
         val = bld_loop_phi(bld, stack, NULL);
      ++bld->loop_lvl;
   }

   if (!val)
      val = bld_phi(bld, bld->pc->current_block, stack); /* old definition */
   if (!val) {
      bld->pc->current_block = bld->loop_bb[bld->loop_lvl - 1]->in[0];
      val = bld_undef(bld, bld_stack_file(bld, stack));
   }

   bld->pc->current_block = bld->loop_bb[bld->loop_lvl - 1];

   phi = new_instruction(bld->pc, NV_OP_PHI);

   bld_def(phi, 0, new_value_like(bld->pc, val));
   if (!def)
      def = phi->def[0];

   bld_vals_push_val(stack, phi->def[0]);

   phi->target = (struct nv_basic_block *)stack; /* cheat */

   nv_reference(bld->pc, &phi->src[0], val);
   nv_reference(bld->pc, &phi->src[1], def);

   bld->pc->current_block = bb;

   return phi->def[0];
}

static INLINE struct nv_value *
bld_fetch_global(struct bld_context *bld, struct bld_value_stack *stack)
{
   const uint16_t m = 1 << bld->loop_lvl;
   const uint16_t use = stack->loop_use;

   stack->loop_use |= m;

   /* If neither used nor def'd inside the loop, build a phi in foresight,
    * so we don't have to replace stuff later on, which requires tracking.
    */
   if (bld->loop_lvl && !((use | stack->loop_def) & m))
      return bld_loop_phi(bld, stack, NULL);

   return bld_phi(bld, bld->pc->current_block, stack);
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

   bld->saved_immd[n] = new_value(bld->pc, NV_FILE_IMM, NV_TYPE_U32);
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
   struct bld_value_stack *stk;
   int i, s, n;

   for (phi = bb->phi; phi && phi->opcode == NV_OP_PHI; phi = next) {
      next = phi->next;

      stk = (struct bld_value_stack *)phi->target;
      phi->target = NULL;

      /* start with s == 1, src[0] is from outside the loop */
      for (s = 1, n = 0; n < bb->num_in; ++n) {
         if (bb->in_kind[n] != CFG_EDGE_BACK)
            continue;

         assert(s < 4);
         bld->pc->current_block = bb->in[n];
         val = bld_fetch_global(bld, stk);

         for (i = 0; i < 4; ++i)
            if (phi->src[i] && phi->src[i]->value == val)
               break;
         if (i == 4) {
            /* skip values we do not want to replace */
            for (; phi->src[s] && phi->src[s]->value != phi->def[0]; ++s);
            nv_reference(bld->pc, &phi->src[s++], val);
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
         bld_vals_del_val(stk, phi->def[0]);

         ++bld->pc->pass_seq;
         bld_replace_value(bld->pc, bb, phi->def[0], phi->src[s]->value);

         nv_nvi_delete(phi);
      }
   }
}

static INLINE struct nv_value *
bld_imm_f32(struct bld_context *bld, float f)
{
   return bld_imm_u32(bld, fui(f));
}

#define SET_TYPE(v, t) ((v)->reg.type = (v)->reg.as_type = (t))

static struct nv_value *
bld_insn_1(struct bld_context *bld, uint opcode, struct nv_value *src0)
{
   struct nv_instruction *insn = new_instruction(bld->pc, opcode);

   nv_reference(bld->pc, &insn->src[0], src0);
   
   return bld_def(insn, 0, new_value(bld->pc, NV_FILE_GPR, src0->reg.as_type));
}

static struct nv_value *
bld_insn_2(struct bld_context *bld, uint opcode,
           struct nv_value *src0, struct nv_value *src1)
{
   struct nv_instruction *insn = new_instruction(bld->pc, opcode);

   nv_reference(bld->pc, &insn->src[0], src0);
   nv_reference(bld->pc, &insn->src[1], src1);

   return bld_def(insn, 0, new_value(bld->pc, NV_FILE_GPR, src0->reg.as_type));
}

static struct nv_value *
bld_insn_3(struct bld_context *bld, uint opcode,
           struct nv_value *src0, struct nv_value *src1,
           struct nv_value *src2)
{
   struct nv_instruction *insn = new_instruction(bld->pc, opcode);

   nv_reference(bld->pc, &insn->src[0], src0);
   nv_reference(bld->pc, &insn->src[1], src1);
   nv_reference(bld->pc, &insn->src[2], src2);

   return bld_def(insn, 0, new_value(bld->pc, NV_FILE_GPR, src0->reg.as_type));
}

static struct nv_value *
bld_duplicate_insn(struct bld_context *bld, struct nv_instruction *nvi)
{
   struct nv_instruction *dupi = new_instruction(bld->pc, nvi->opcode);
   int c;

   if (nvi->def[0])
      bld_def(dupi, 0, new_value_like(bld->pc, nvi->def[0]));

   if (nvi->flags_def) {
      dupi->flags_def = new_value_like(bld->pc, nvi->flags_def);
      dupi->flags_def->insn = dupi;
   }

   for (c = 0; c < 5; ++c)
      if (nvi->src[c])
         nv_reference(bld->pc, &dupi->src[c], nvi->src[c]->value);
   if (nvi->flags_src)
      nv_reference(bld->pc, &dupi->flags_src, nvi->flags_src->value);

   dupi->cc = nvi->cc;
   dupi->saturate = nvi->saturate;
   dupi->centroid = nvi->centroid;
   dupi->flat = nvi->flat;

   return dupi->def[0];
}

static void
bld_lmem_store(struct bld_context *bld, struct nv_value *ptr, int ofst,
               struct nv_value *val)
{
   struct nv_instruction *insn = new_instruction(bld->pc, NV_OP_STA);
   struct nv_value *loc;

   loc = new_value(bld->pc, NV_FILE_MEM_L, NV_TYPE_U32);

   loc->reg.id = ofst * 4;

   nv_reference(bld->pc, &insn->src[0], loc);
   nv_reference(bld->pc, &insn->src[1], val);
   nv_reference(bld->pc, &insn->src[4], ptr);
}

static struct nv_value *
bld_lmem_load(struct bld_context *bld, struct nv_value *ptr, int ofst)
{
   struct nv_value *loc, *val;

   loc = new_value(bld->pc, NV_FILE_MEM_L, NV_TYPE_U32);

   loc->reg.id = ofst * 4;

   val = bld_insn_1(bld, NV_OP_LDA, loc);

   nv_reference(bld->pc, &val->insn->src[4], ptr);

   return val;
}

#define BLD_INSN_1_EX(d, op, dt, s0, s0t)           \
   do {                                             \
      (d) = bld_insn_1(bld, (NV_OP_##op), (s0));    \
      SET_TYPE(d, NV_TYPE_##dt);                    \
      (d)->insn->src[0]->typecast = NV_TYPE_##s0t;  \
   } while(0)

#define BLD_INSN_2_EX(d, op, dt, s0, s0t, s1, s1t)       \
   do {                                                  \
      (d) = bld_insn_2(bld, (NV_OP_##op), (s0), (s1));   \
      SET_TYPE(d, NV_TYPE_##dt);                         \
      (d)->insn->src[0]->typecast = NV_TYPE_##s0t;       \
      (d)->insn->src[1]->typecast = NV_TYPE_##s1t;       \
   } while(0)

static struct nv_value *
bld_pow(struct bld_context *bld, struct nv_value *x, struct nv_value *e)
{
   struct nv_value *val;

   BLD_INSN_1_EX(val, LG2, F32, x, F32);
   BLD_INSN_2_EX(val, MUL, F32, e, F32, val, F32);
   val = bld_insn_1(bld, NV_OP_PREEX2, val);
   val = bld_insn_1(bld, NV_OP_EX2, val);

   return val;
}

static INLINE struct nv_value *
bld_load_imm_f32(struct bld_context *bld, float f)
{
   struct nv_value *imm = bld_insn_1(bld, NV_OP_MOV, bld_imm_f32(bld, f));

   SET_TYPE(imm, NV_TYPE_F32);
   return imm;
}

static INLINE struct nv_value *
bld_load_imm_u32(struct bld_context *bld, uint32_t u)
{
   return bld_insn_1(bld, NV_OP_MOV, bld_imm_u32(bld, u));
}

static struct nv_value *
bld_get_address(struct bld_context *bld, int id, struct nv_value *indirect)
{
   int i;
   struct nv_instruction *nvi;
   struct nv_value *val;

   for (i = 0; i < 4; ++i) {
      if (!bld->saved_addr[i][0])
         break;
      if (bld->saved_addr[i][1] == indirect) {
         nvi = bld->saved_addr[i][0]->insn;
         if (nvi->src[0]->value->reg.imm.u32 == id)
            return bld->saved_addr[i][0];
      }
   }
   i &= 3;

   val = bld_imm_u32(bld, id);
   if (indirect)
      val = bld_insn_2(bld, NV_OP_ADD, indirect, val);
   else
      val = bld_insn_1(bld, NV_OP_MOV, val);

   bld->saved_addr[i][0] = val;
   bld->saved_addr[i][0]->reg.file = NV_FILE_ADDR;
   bld->saved_addr[i][0]->reg.type = NV_TYPE_U16;
   bld->saved_addr[i][1] = indirect;
   return bld->saved_addr[i][0];
}


static struct nv_value *
bld_predicate(struct bld_context *bld, struct nv_value *src, boolean bool_only)
{
   struct nv_instruction *s0i, *nvi = src->insn;

   if (!nvi) {
      nvi = bld_insn_1(bld,
                       (src->reg.file == NV_FILE_IMM) ? NV_OP_MOV : NV_OP_LDA,
                       src)->insn;
      src = nvi->def[0];
   } else
   if (bool_only) {
      while (nvi->opcode == NV_OP_ABS || nvi->opcode == NV_OP_NEG ||
             nvi->opcode == NV_OP_CVT) {
         s0i = nvi->src[0]->value->insn;
         if (!s0i || !nv50_op_can_write_flags(s0i->opcode))
            break;
         nvi = s0i;
         assert(!nvi->flags_src);
      }
   }

   if (!nv50_op_can_write_flags(nvi->opcode) ||
       nvi->bb != bld->pc->current_block) {
      nvi = new_instruction(bld->pc, NV_OP_CVT);
      nv_reference(bld->pc, &nvi->src[0], src);
   }

   if (!nvi->flags_def) {
      nvi->flags_def = new_value(bld->pc, NV_FILE_FLAGS, NV_TYPE_U16);
      nvi->flags_def->insn = nvi;
   }
   return nvi->flags_def;
}

static void
bld_kil(struct bld_context *bld, struct nv_value *src)
{
   struct nv_instruction *nvi;

   src = bld_predicate(bld, src, FALSE);
   nvi = new_instruction(bld->pc, NV_OP_KIL);
   nvi->fixed = 1;
   nvi->flags_src = new_ref(bld->pc, src);
   nvi->cc = NV_CC_LT;
}

static void
bld_flow(struct bld_context *bld, uint opcode, ubyte cc,
         struct nv_value *src, struct nv_basic_block *target,
         boolean plan_reconverge)
{
   struct nv_instruction *nvi;

   if (plan_reconverge)
      new_instruction(bld->pc, NV_OP_JOINAT)->fixed = 1;

   nvi = new_instruction(bld->pc, opcode);
   nvi->is_terminator = 1;
   nvi->cc = cc;
   nvi->target = target;
   if (src)
      nvi->flags_src = new_ref(bld->pc, src);
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
   case TGSI_OPCODE_ABS: return NV_OP_ABS;
   case TGSI_OPCODE_ADD:
   case TGSI_OPCODE_SUB:
   case TGSI_OPCODE_UADD: return NV_OP_ADD;
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
   case TGSI_OPCODE_INEG: return NV_OP_NEG;
   case TGSI_OPCODE_LG2: return NV_OP_LG2;
   case TGSI_OPCODE_ISHR:
   case TGSI_OPCODE_USHR: return NV_OP_SHR;
   case TGSI_OPCODE_MAD:
   case TGSI_OPCODE_UMAD: return NV_OP_MAD;
   case TGSI_OPCODE_MAX:
   case TGSI_OPCODE_IMAX:
   case TGSI_OPCODE_UMAX: return NV_OP_MAX;
   case TGSI_OPCODE_MIN:
   case TGSI_OPCODE_IMIN:
   case TGSI_OPCODE_UMIN: return NV_OP_MIN;
   case TGSI_OPCODE_MUL:
   case TGSI_OPCODE_UMUL: return NV_OP_MUL;
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
   case TGSI_OPCODE_SNE:
   case TGSI_OPCODE_ISLT:
   case TGSI_OPCODE_ISGE:
   case TGSI_OPCODE_USEQ:
   case TGSI_OPCODE_USGE:
   case TGSI_OPCODE_USLT:
   case TGSI_OPCODE_USNE: return NV_OP_SET;
   case TGSI_OPCODE_TEX: return NV_OP_TEX;
   case TGSI_OPCODE_TXP: return NV_OP_TEX;
   case TGSI_OPCODE_TXB: return NV_OP_TXB;
   case TGSI_OPCODE_TXL: return NV_OP_TXL;
   case TGSI_OPCODE_XOR: return NV_OP_XOR;
   default:
      return NV_OP_NOP;
   }
}

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

static void
emit_store(struct bld_context *bld, const struct tgsi_full_instruction *inst,
           unsigned chan, struct nv_value *value)
{
   struct nv_value *ptr;
   const struct tgsi_full_dst_register *reg = &inst->Dst[0];

   if (reg->Register.Indirect) {
      ptr = FETCH_ADDR(reg->Indirect.Index,
                       tgsi_util_get_src_register_swizzle(&reg->Indirect, 0));
   } else {
      ptr = NULL;
   }

   assert(chan < 4);

   if (inst->Instruction.Opcode != TGSI_OPCODE_MOV)
      value->reg.type = infer_dst_type(inst->Instruction.Opcode);

   switch (inst->Instruction.Saturate) {
   case TGSI_SAT_NONE:
      break;
   case TGSI_SAT_ZERO_ONE:
      BLD_INSN_1_EX(value, SAT, F32, value, F32);
      break;
   case TGSI_SAT_MINUS_PLUS_ONE:
      value->reg.as_type = NV_TYPE_F32;
      value = bld_insn_2(bld, NV_OP_MAX, value, bld_load_imm_f32(bld, -1.0f));
      value = bld_insn_2(bld, NV_OP_MIN, value, bld_load_imm_f32(bld, 1.0f));
      break;
   }

   switch (reg->Register.File) {
   case TGSI_FILE_OUTPUT:
      if (!value->insn && (bld->ti->output_file == NV_FILE_OUT))
         value = bld_insn_1(bld, NV_OP_MOV, value);
      value = bld_insn_1(bld, NV_OP_MOV, value);
      value->reg.file = bld->ti->output_file;

      if (bld->ti->p->type == PIPE_SHADER_FRAGMENT) {
         STORE_OUTR(reg->Register.Index, chan, value);
      } else {
         value->insn->fixed = 1;
         value->reg.id = bld->ti->output_map[reg->Register.Index][chan];
      }
      break;
   case TGSI_FILE_TEMPORARY:
      assert(reg->Register.Index < BLD_MAX_TEMPS);
      if (!value->insn || (value->insn->bb != bld->pc->current_block))
         value = bld_insn_1(bld, NV_OP_MOV, value);
      value->reg.file = NV_FILE_GPR;

      if (bld->ti->store_to_memory)
         bld_lmem_store(bld, ptr, reg->Register.Index * 4 + chan, value);
      else
         STORE_TEMP(reg->Register.Index, chan, value);
      break;
   case TGSI_FILE_ADDRESS:
      assert(reg->Register.Index < BLD_MAX_ADDRS);
      value->reg.file = NV_FILE_ADDR;
      value->reg.type = NV_TYPE_U16;
      STORE_ADDR(reg->Register.Index, chan, value);
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
bld_export_outputs(struct bld_context *bld)
{
   struct nv_value *vals[4];
   struct nv_instruction *nvi;
   int i, c, n;

   bld_push_values(&bld->ovs[0][0], PIPE_MAX_SHADER_OUTPUTS);

   for (i = 0; i < PIPE_MAX_SHADER_OUTPUTS; ++i) {
      if (!bld_is_output_written(bld, i, -1))
         continue;
      for (n = 0, c = 0; c < 4; ++c) {
         if (!bld_is_output_written(bld, i, c))
            continue;
         vals[n] = bld_fetch_global(bld, &bld->ovs[i][c]);
         assert(vals[n]);
         vals[n] = bld_insn_1(bld, NV_OP_MOV, vals[n]);
         vals[n++]->reg.id = bld->ti->output_map[i][c];
      }
      assert(n);

      (nvi = new_instruction(bld->pc, NV_OP_EXPORT))->fixed = 1;

      for (c = 0; c < n; ++c)
         nvi->src[c] = new_ref(bld->pc, vals[c]);
   }
}

static void
bld_new_block(struct bld_context *bld, struct nv_basic_block *b)
{
   int i;

   bld_push_values(&bld->tvs[0][0], BLD_MAX_TEMPS);
   bld_push_values(&bld->avs[0][0], BLD_MAX_ADDRS);
   bld_push_values(&bld->pvs[0][0], BLD_MAX_PREDS);
   bld_push_values(&bld->ovs[0][0], PIPE_MAX_SHADER_OUTPUTS);

   bld->pc->current_block = b;

   for (i = 0; i < 4; ++i)
      bld->saved_addr[i][0] = NULL;

   for (i = 0; i < 128; ++i)
      bld->saved_inputs[i] = NULL;

   bld->out_kind = CFG_EDGE_FORWARD;
}

static struct nv_value *
bld_saved_input(struct bld_context *bld, unsigned i, unsigned c)
{
   unsigned idx = bld->ti->input_map[i][c];

   if (bld->ti->p->type != PIPE_SHADER_FRAGMENT)
      return NULL;
   if (bld->saved_inputs[idx])
      return bld->saved_inputs[idx];
   return NULL;
}

static struct nv_value *
bld_interpolate(struct bld_context *bld, unsigned mode, struct nv_value *val)
{
   if (val->reg.id == 255) {
      /* gl_FrontFacing: 0/~0 to -1.0/+1.0 */
      val = bld_insn_1(bld, NV_OP_LINTERP, val);
      val = bld_insn_2(bld, NV_OP_SHL, val, bld_imm_u32(bld, 31));
      val->insn->src[0]->typecast = NV_TYPE_U32;
      val = bld_insn_2(bld, NV_OP_XOR, val, bld_imm_f32(bld, -1.0f));
      val->insn->src[0]->typecast = NV_TYPE_U32;
   } else
   if (mode & (NV50_INTERP_LINEAR | NV50_INTERP_FLAT))
      val = bld_insn_1(bld, NV_OP_LINTERP, val);
   else
      val = bld_insn_2(bld, NV_OP_PINTERP, val, bld->frgcrd[3]);

   val->insn->flat = (mode & NV50_INTERP_FLAT) ? 1 : 0;
   val->insn->centroid = (mode & NV50_INTERP_CENTROID) ? 1 : 0;
   return val;
}

static struct nv_value *
emit_fetch(struct bld_context *bld, const struct tgsi_full_instruction *insn,
           const unsigned s, const unsigned chan)
{
   const struct tgsi_full_src_register *src = &insn->Src[s];
   struct nv_value *res;
   struct nv_value *ptr = NULL;
   unsigned idx, swz, dim_idx, ind_idx, ind_swz, sgn;
   ubyte type = infer_src_type(insn->Instruction.Opcode);

   idx = src->Register.Index;
   swz = tgsi_util_get_full_src_register_swizzle(src, chan);
   dim_idx = -1;
   ind_idx = -1;
   ind_swz = 0;

   if (src->Register.Indirect) {
      ind_idx = src->Indirect.Index;
      ind_swz = tgsi_util_get_src_register_swizzle(&src->Indirect, 0);

      ptr = FETCH_ADDR(ind_idx, ind_swz);
   }
   if (idx >= (128 / 4) && src->Register.File == TGSI_FILE_CONSTANT)
      ptr = bld_get_address(bld, (idx * 16) & ~0x1ff, ptr);

   switch (src->Register.File) {
   case TGSI_FILE_CONSTANT:
      dim_idx = src->Dimension.Index;
      assert(dim_idx < 15);

      res = new_value(bld->pc, NV_FILE_MEM_C(dim_idx), type);
      SET_TYPE(res, type);
      res->reg.id = (idx * 4 + swz) & 127;
      res = bld_insn_1(bld, NV_OP_LDA, res);

      if (ptr)
         res->insn->src[4] = new_ref(bld->pc, ptr);
      break;
   case TGSI_FILE_IMMEDIATE:
      assert(idx < bld->ti->immd32_nr);
      res = bld_load_imm_u32(bld, bld->ti->immd32[idx * 4 + swz]);

      switch (bld->ti->immd32_ty[idx]) {
      case TGSI_IMM_FLOAT32: SET_TYPE(res, NV_TYPE_F32); break;
      case TGSI_IMM_UINT32: SET_TYPE(res, NV_TYPE_U32); break;
      case TGSI_IMM_INT32: SET_TYPE(res, NV_TYPE_S32); break;
      default:
         SET_TYPE(res, type);
         break;
      }
      break;
   case TGSI_FILE_INPUT:
      res = bld_saved_input(bld, idx, swz);
      if (res && (insn->Instruction.Opcode != TGSI_OPCODE_TXP))
         break;

      res = new_value(bld->pc, bld->ti->input_file, type);
      res->reg.id = bld->ti->input_map[idx][swz];

      if (res->reg.file == NV_FILE_MEM_V) {
         res = bld_interpolate(bld, bld->ti->interp_mode[idx], res);
      } else {
         assert(src->Dimension.Dimension == 0);
         res = bld_insn_1(bld, NV_OP_LDA, res);
         assert(res->reg.type == type);
      }
      bld->saved_inputs[bld->ti->input_map[idx][swz]] = res;
      break;
   case TGSI_FILE_TEMPORARY:
      if (bld->ti->store_to_memory)
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
      res = new_value(bld->pc, bld->ti->input_file, NV_TYPE_U32);
      res->reg.id = bld->ti->sysval_map[idx];
      res = bld_insn_1(bld, NV_OP_LDA, res);
      res = bld_insn_1(bld, NV_OP_CVT, res);
      res->reg.type = NV_TYPE_F32;
      break;
   default:
      NOUVEAU_ERR("illegal/unhandled src reg file: %d\n", src->Register.File);
      abort();
      break;	   
   }
   if (!res)
      return bld_undef(bld, NV_FILE_GPR);

   sgn = tgsi_util_get_full_src_register_sign_mode(src, chan);

   if (insn->Instruction.Opcode != TGSI_OPCODE_MOV)
      res->reg.as_type = type;
   else
   if (sgn != TGSI_UTIL_SIGN_KEEP) /* apparently "MOV A, -B" assumes float */
      res->reg.as_type = NV_TYPE_F32;

   switch (sgn) {
   case TGSI_UTIL_SIGN_KEEP:
      break;
   case TGSI_UTIL_SIGN_CLEAR:
      res = bld_insn_1(bld, NV_OP_ABS, res);
      break;
   case TGSI_UTIL_SIGN_TOGGLE:
      res = bld_insn_1(bld, NV_OP_NEG, res);
      break;
   case TGSI_UTIL_SIGN_SET:
      res = bld_insn_1(bld, NV_OP_ABS, res);
      res = bld_insn_1(bld, NV_OP_NEG, res);
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
   struct nv_value *zero = NULL;
   unsigned mask = insn->Dst[0].Register.WriteMask;

   if (mask & ((1 << 0) | (1 << 3)))
      dst0[3] = dst0[0] = bld_load_imm_f32(bld, 1.0f);

   if (mask & (3 << 1)) {
      zero = bld_load_imm_f32(bld, 0.0f);
      val0 = bld_insn_2(bld, NV_OP_MAX, emit_fetch(bld, insn, 0, 0), zero);

      if (mask & (1 << 1))
         dst0[1] = val0;
   }

   if (mask & (1 << 2)) {
      struct nv_value *val1, *val3, *src1, *src3;
      struct nv_value *pos128 = bld_load_imm_f32(bld, 127.999999f);
      struct nv_value *neg128 = bld_load_imm_f32(bld, -127.999999f);

      src1 = emit_fetch(bld, insn, 0, 1);
      src3 = emit_fetch(bld, insn, 0, 3);

      val0->insn->flags_def = new_value(bld->pc, NV_FILE_FLAGS, NV_TYPE_U16);
      val0->insn->flags_def->insn = val0->insn;

      val1 = bld_insn_2(bld, NV_OP_MAX, src1, zero);
      val3 = bld_insn_2(bld, NV_OP_MAX, src3, neg128);
      val3 = bld_insn_2(bld, NV_OP_MIN, val3, pos128);
      val3 = bld_pow(bld, val1, val3);

      dst0[2] = bld_insn_1(bld, NV_OP_MOV, zero);
      dst0[2]->insn->cc = NV_CC_LE;
      dst0[2]->insn->flags_src = new_ref(bld->pc, val0->insn->flags_def);

      dst0[2] = bld_insn_2(bld, NV_OP_SELECT, val3, dst0[2]);
   }
}

static INLINE void
get_tex_dim(const struct tgsi_full_instruction *insn, int *dim, int *arg)
{
   switch (insn->Texture.Texture) {
   case TGSI_TEXTURE_1D:
      *arg = *dim = 1;
      break;
   case TGSI_TEXTURE_SHADOW1D:
      *dim = 1;
      *arg = 2;
      break;
   case TGSI_TEXTURE_UNKNOWN:
   case TGSI_TEXTURE_2D:
   case TGSI_TEXTURE_RECT:
      *arg = *dim = 2;
      break;
   case TGSI_TEXTURE_SHADOW2D:
   case TGSI_TEXTURE_SHADOWRECT:
      *dim = 2;
      *arg = 3;
      break;
   case TGSI_TEXTURE_3D:
   case TGSI_TEXTURE_CUBE:
      *dim = *arg = 3;
      break;
   default:
      assert(0);
      break;
   }
}

static void
load_proj_tex_coords(struct bld_context *bld,
                     struct nv_value *t[4], int dim, int arg,
                     const struct tgsi_full_instruction *insn)
{
   int c, mask;

   mask = (1 << dim) - 1;
   if (arg != dim)
      mask |= 4; /* depth comparison value */

   t[3] = emit_fetch(bld, insn, 0, 3);

   if (t[3]->insn->opcode == NV_OP_PINTERP) {
      t[3] = bld_duplicate_insn(bld, t[3]->insn);
      t[3]->insn->opcode = NV_OP_LINTERP;
      nv_reference(bld->pc, &t[3]->insn->src[1], NULL);
   }

   t[3] = bld_insn_1(bld, NV_OP_RCP, t[3]);

   for (c = 0; c < 4; ++c) {
      if (!(mask & (1 << c)))
         continue;
      t[c] = emit_fetch(bld, insn, 0, c);

      if (t[c]->insn->opcode != NV_OP_LINTERP &&
          t[c]->insn->opcode != NV_OP_PINTERP)
         continue;
      t[c] = bld_duplicate_insn(bld, t[c]->insn);
      t[c]->insn->opcode = NV_OP_PINTERP;
      nv_reference(bld->pc, &t[c]->insn->src[1], t[3]);

      mask &= ~(1 << c);
   }

   for (c = 0; mask; ++c, mask >>= 1) {
      if (!(mask & 1))
         continue;
      t[c] = bld_insn_2(bld, NV_OP_MUL, t[c], t[3]);
   }
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
      val->insn->flags_def = new_value(bld->pc, NV_FILE_FLAGS, NV_TYPE_U16);
      val->insn->flags_def->insn = val->insn;
   }
   return val;
}

static INLINE struct nv_value *
bld_cmov(struct bld_context *bld,
         struct nv_value *src, ubyte cc, struct nv_value *cr)
{
   src = bld_insn_1(bld, NV_OP_MOV, src);

   src->insn->cc = cc;
   src->insn->flags_src = new_ref(bld->pc, cr);

   return src;
}

static struct nv_instruction *
emit_tex(struct bld_context *bld, uint opcode,
         struct nv_value *dst[4], struct nv_value *t_in[4],
         int argc, int tic, int tsc, int cube)
{
   struct nv_value *t[4];
   struct nv_instruction *nvi;
   int c;

   /* the inputs to a tex instruction must be separate values */
   for (c = 0; c < argc; ++c) {
      t[c] = bld_insn_1(bld, NV_OP_MOV, t_in[c]);
      SET_TYPE(t[c], NV_TYPE_F32);
      t[c]->insn->fixed = 1;
   }

   nvi = new_instruction(bld->pc, opcode);

   for (c = 0; c < 4; ++c)
      dst[c] = bld_def(nvi, c, new_value(bld->pc, NV_FILE_GPR, NV_TYPE_F32));

   for (c = 0; c < argc; ++c)
      nvi->src[c] = new_ref(bld->pc, t[c]);

   nvi->tex_t = tic;
   nvi->tex_s = tsc;
   nvi->tex_mask = 0xf;
   nvi->tex_cube = cube;
   nvi->tex_live = 0;
   nvi->tex_argc = argc;

   return nvi;
}

static void
bld_texlod_sequence(struct bld_context *bld,
                    struct nv_value *dst[4], struct nv_value *t[4], int arg,
                    int tic, int tsc, int cube)
{
   emit_tex(bld, NV_OP_TXL, dst, t, arg, tic, tsc, cube); /* TODO */
}


/* The lanes of a quad are grouped by the bit in the condition register
 * they have set, which is selected by differing bias values.
 * Move the input values for TEX into a new register set for each group
 * and execute TEX only for a specific group.
 * We always need to use 4 new registers for the inputs/outputs because
 * the implicitly calculated derivatives must be correct.
 */
static void
bld_texbias_sequence(struct bld_context *bld,
                     struct nv_value *dst[4], struct nv_value *t[4], int arg,
                     int tic, int tsc, int cube)
{
   struct nv_instruction *sel, *tex;
   struct nv_value *bit[4], *cr[4], *res[4][4], *val;
   int l, c;

   const ubyte cc[4] = { NV_CC_EQ, NV_CC_S, NV_CC_C, NV_CC_O };

   for (l = 0; l < 4; ++l) {
      bit[l] = bld_load_imm_u32(bld, 1 << l);

      val = bld_quadop(bld, QOP(SUBR, SUBR, SUBR, SUBR),
                       t[arg - 1], l, t[arg - 1], TRUE);

      cr[l] = bld_cmov(bld, bit[l], NV_CC_EQ, val->insn->flags_def);

      cr[l]->reg.file = NV_FILE_FLAGS;
      SET_TYPE(cr[l], NV_TYPE_U16);
   }

   sel = new_instruction(bld->pc, NV_OP_SELECT);

   for (l = 0; l < 4; ++l)
      sel->src[l] = new_ref(bld->pc, cr[l]);

   bld_def(sel, 0, new_value(bld->pc, NV_FILE_FLAGS, NV_TYPE_U16));

   for (l = 0; l < 4; ++l) {
      tex = emit_tex(bld, NV_OP_TXB, dst, t, arg, tic, tsc, cube);

      tex->cc = cc[l];
      tex->flags_src = new_ref(bld->pc, sel->def[0]);

      for (c = 0; c < 4; ++c)
         res[l][c] = tex->def[c];
   }

   for (l = 0; l < 4; ++l)
      for (c = 0; c < 4; ++c)
         res[l][c] = bld_cmov(bld, res[l][c], cc[l], sel->def[0]);

   for (c = 0; c < 4; ++c) {
      sel = new_instruction(bld->pc, NV_OP_SELECT);

      for (l = 0; l < 4; ++l)
         sel->src[l] = new_ref(bld->pc, res[l][c]);

      bld_def(sel, 0, (dst[c] = new_value(bld->pc, NV_FILE_GPR, NV_TYPE_F32)));
   }
}

static boolean
bld_is_constant(struct nv_value *val)
{
   if (val->reg.file == NV_FILE_IMM)
      return TRUE;
   return val->insn && nvcg_find_constant(val->insn->src[0]);
}

static void
bld_tex(struct bld_context *bld, struct nv_value *dst0[4],
        const struct tgsi_full_instruction *insn)
{
   struct nv_value *t[4], *s[3];
   uint opcode = translate_opcode(insn->Instruction.Opcode);
   int arg, dim, c;
   const int tic = insn->Src[1].Register.Index;
   const int tsc = tic;
   const int cube = (insn->Texture.Texture  == TGSI_TEXTURE_CUBE) ? 1 : 0;

   get_tex_dim(insn, &dim, &arg);

   if (!cube && insn->Instruction.Opcode == TGSI_OPCODE_TXP)
      load_proj_tex_coords(bld, t, dim, arg, insn);
   else {
      for (c = 0; c < dim; ++c)
         t[c] = emit_fetch(bld, insn, 0, c);
      if (arg != dim)
         t[dim] = emit_fetch(bld, insn, 0, 2);
   }

   if (cube) {
      assert(dim >= 3);
      for (c = 0; c < 3; ++c)
         s[c] = bld_insn_1(bld, NV_OP_ABS, t[c]);

      s[0] = bld_insn_2(bld, NV_OP_MAX, s[0], s[1]);
      s[0] = bld_insn_2(bld, NV_OP_MAX, s[0], s[2]);
      s[0] = bld_insn_1(bld, NV_OP_RCP, s[0]);

      for (c = 0; c < 3; ++c)
         t[c] = bld_insn_2(bld, NV_OP_MUL, t[c], s[0]);
   }

   if (opcode == NV_OP_TXB || opcode == NV_OP_TXL) {
      t[arg++] = emit_fetch(bld, insn, 0, 3);

      if ((bld->ti->p->type == PIPE_SHADER_FRAGMENT) &&
          !bld_is_constant(t[arg - 1])) {
         if (opcode == NV_OP_TXB)
            bld_texbias_sequence(bld, dst0, t, arg, tic, tsc, cube);
         else
            bld_texlod_sequence(bld, dst0, t, arg, tic, tsc, cube);
         return;
      }
   }

   emit_tex(bld, opcode, dst0, t, arg, tic, tsc, cube);
}

static INLINE struct nv_value *
bld_dot(struct bld_context *bld, const struct tgsi_full_instruction *insn,
        int n)
{
   struct nv_value *dotp, *src0, *src1;
   int c;

   src0 = emit_fetch(bld, insn, 0, 0);
   src1 = emit_fetch(bld, insn, 1, 0);
   dotp = bld_insn_2(bld, NV_OP_MUL, src0, src1);

   for (c = 1; c < n; ++c) {
      src0 = emit_fetch(bld, insn, 0, c);
      src1 = emit_fetch(bld, insn, 1, c);
      dotp = bld_insn_3(bld, NV_OP_MAD, src0, src1, dotp);
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
   struct nv50_program *prog = bld->ti->p;
   const struct tgsi_full_dst_register *dreg = &insn->Dst[0];
   struct nv_value *src0;
   struct nv_value *src1;
   struct nv_value *src2;
   struct nv_value *dst0[4] = { 0 };
   struct nv_value *temp;
   int c;
   uint opcode = translate_opcode(insn->Instruction.Opcode);

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
         temp = bld_insn_1(bld, NV_OP_FLOOR, src0);
         SET_TYPE(temp, NV_TYPE_S32);
         dst0[c] = bld_insn_2(bld, NV_OP_SHL, temp, src1);
      }
      break;
   case TGSI_OPCODE_CMP:
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn) {
         src0 = emit_fetch(bld, insn, 0, c);
         src1 = emit_fetch(bld, insn, 1, c);
         src2 = emit_fetch(bld, insn, 2, c);
         src0 = bld_predicate(bld, src0, FALSE);

         src1 = bld_insn_1(bld, NV_OP_MOV, src1);
         src1->insn->flags_src = new_ref(bld->pc, src0);
         src1->insn->cc = NV_CC_LT;

         src2 = bld_insn_1(bld, NV_OP_MOV, src2);
         src2->insn->flags_src = new_ref(bld->pc, src0);
         src2->insn->cc = NV_CC_GE;

         dst0[c] = bld_insn_2(bld, NV_OP_SELECT, src1, src2);
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
      temp = bld_insn_2(bld, NV_OP_ADD, src0, src1);
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn)
         dst0[c] = temp;
      break;
   case TGSI_OPCODE_DST:
      if (insn->Dst[0].Register.WriteMask & 1)
         dst0[0] = bld_imm_f32(bld, 1.0f);
      if (insn->Dst[0].Register.WriteMask & 2) {
         src0 = emit_fetch(bld, insn, 0, 1);
         src1 = emit_fetch(bld, insn, 1, 1);
         dst0[1] = bld_insn_2(bld, NV_OP_MUL, src0, src1);
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
         dst0[1] = bld_insn_2(bld, NV_OP_SUB, src0, temp);
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
         dst0[c] = bld_insn_2(bld, NV_OP_SUB, src0, dst0[c]);
      }
      break;
   case TGSI_OPCODE_KIL:
      for (c = 0; c < 4; ++c) {
         src0 = emit_fetch(bld, insn, 0, c);
         bld_kil(bld, src0);
      }
      break;
   case TGSI_OPCODE_KILP:
      (new_instruction(bld->pc, NV_OP_KIL))->fixed = 1;
      break;
   case TGSI_OPCODE_IF:
   {
      struct nv_basic_block *b = new_basic_block(bld->pc);

      assert(bld->cond_lvl < BLD_MAX_COND_NESTING);

      nvbb_attach_block(bld->pc->current_block, b, CFG_EDGE_FORWARD);

      bld->join_bb[bld->cond_lvl] = bld->pc->current_block;
      bld->cond_bb[bld->cond_lvl] = bld->pc->current_block;

      src1 = bld_predicate(bld, emit_fetch(bld, insn, 0, 0), TRUE);

      bld_flow(bld, NV_OP_BRA, NV_CC_EQ, src1, NULL, (bld->cond_lvl == 0));

      ++bld->cond_lvl;
      bld_new_block(bld, b);
   }
      break;
   case TGSI_OPCODE_ELSE:
   {
      struct nv_basic_block *b = new_basic_block(bld->pc);

      --bld->cond_lvl;
      nvbb_attach_block(bld->join_bb[bld->cond_lvl], b, CFG_EDGE_FORWARD);

      bld->cond_bb[bld->cond_lvl]->exit->target = b;
      bld->cond_bb[bld->cond_lvl] = bld->pc->current_block;

      new_instruction(bld->pc, NV_OP_BRA)->is_terminator = 1;

      ++bld->cond_lvl;
      bld_new_block(bld, b);
   }
      break;
   case TGSI_OPCODE_ENDIF:
   {
      struct nv_basic_block *b = new_basic_block(bld->pc);

      if (!nvbb_is_terminated(bld->pc->current_block))
         bld_flow(bld, NV_OP_BRA, NV_CC_TR, NULL, b, FALSE);

      --bld->cond_lvl;
      nvbb_attach_block(bld->pc->current_block, b, bld->out_kind);
      nvbb_attach_block(bld->cond_bb[bld->cond_lvl], b, CFG_EDGE_FORWARD);

      bld->cond_bb[bld->cond_lvl]->exit->target = b;

      bld_new_block(bld, b);

      if (!bld->cond_lvl && bld->join_bb[bld->cond_lvl]) {
         bld->join_bb[bld->cond_lvl]->exit->prev->target = b;
         new_instruction(bld->pc, NV_OP_JOIN)->is_join = TRUE;
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

      bld_flow(bld, NV_OP_BREAKADDR, NV_CC_TR, NULL, bb, FALSE);

      nvbb_attach_block(bld->pc->current_block, bl, CFG_EDGE_LOOP_ENTER);

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

      bld_flow(bld, NV_OP_BREAK, NV_CC_TR, NULL, bb, FALSE);

      if (bld->out_kind == CFG_EDGE_FORWARD) /* else we already had BRK/CONT */
         nvbb_attach_block(bld->pc->current_block, bb, CFG_EDGE_LOOP_LEAVE);

      bld->out_kind = CFG_EDGE_FAKE;
   }
      break;
   case TGSI_OPCODE_CONT:
   {
      struct nv_basic_block *bb = bld->loop_bb[bld->loop_lvl - 1];

      bld_flow(bld, NV_OP_BRA, NV_CC_TR, NULL, bb, FALSE);

      nvbb_attach_block(bld->pc->current_block, bb, CFG_EDGE_BACK);

      if ((bb = bld->join_bb[bld->cond_lvl - 1])) {
         bld->join_bb[bld->cond_lvl - 1] = NULL;
         nv_nvi_delete(bb->exit->prev);
      }
      bld->out_kind = CFG_EDGE_FAKE;
   }
      break;
   case TGSI_OPCODE_ENDLOOP:
   {
      struct nv_basic_block *bb = bld->loop_bb[bld->loop_lvl - 1];

      if (!nvbb_is_terminated(bld->pc->current_block))
         bld_flow(bld, NV_OP_BRA, NV_CC_TR, NULL, bb, FALSE);

      nvbb_attach_block(bld->pc->current_block, bb, CFG_EDGE_BACK);

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
         dst0[c] = bld_insn_2(bld, NV_OP_SUB, src1, src2);
         dst0[c] = bld_insn_3(bld, NV_OP_MAD, dst0[c], src0, src2);
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
      src0 = bld_insn_1(bld, NV_OP_ABS, src0);
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
         dst0[1] = bld_insn_2(bld, NV_OP_MUL, src0, temp);
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
      temp = bld_insn_1(bld, NV_OP_ABS, src0);
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
         dst0[c] = bld_insn_2(bld, NV_OP_SET, src0, src1);
         dst0[c]->insn->set_cond = translate_setcc(insn->Instruction.Opcode);
         SET_TYPE(dst0[c], infer_dst_type(insn->Instruction.Opcode));

         dst0[c]->insn->src[0]->typecast =
         dst0[c]->insn->src[1]->typecast =
            infer_src_type(insn->Instruction.Opcode);

         if (dst0[c]->reg.type != NV_TYPE_F32)
            break;
         dst0[c]->reg.as_type = NV_TYPE_S32;
         dst0[c] = bld_insn_1(bld, NV_OP_ABS, dst0[c]);
         dst0[c] = bld_insn_1(bld, NV_OP_CVT, dst0[c]);
         SET_TYPE(dst0[c], NV_TYPE_F32);
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
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn) {
         src0 = emit_fetch(bld, insn, 0, c);
         src1 = bld_predicate(bld, src0, FALSE);
         temp = bld_insn_2(bld, NV_OP_AND, src0, bld_imm_u32(bld, 0x80000000));
         temp = bld_insn_2(bld, NV_OP_OR,  temp, bld_imm_f32(bld, 1.0f));
         dst0[c] = bld_insn_2(bld, NV_OP_XOR, temp, temp);
         dst0[c]->insn->cc = NV_CC_EQ;
         nv_reference(bld->pc, &dst0[c]->insn->flags_src, src1);
         dst0[c] = bld_insn_2(bld, NV_OP_SELECT, dst0[c], temp);
      }
      break;
   case TGSI_OPCODE_SUB:
      FOR_EACH_DST0_ENABLED_CHANNEL(c, insn) {
         src0 = emit_fetch(bld, insn, 0, c);
         src1 = emit_fetch(bld, insn, 1, c);
         dst0[c] = bld_insn_2(bld, NV_OP_ADD, src0, src1);
         dst0[c]->insn->src[1]->mod ^= NV_MOD_NEG;
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
         dst0[c] = bld_insn_2(bld, NV_OP_MUL, src0, src1);

         src0 = emit_fetch(bld, insn, 0, (c + 1) % 3);
         src1 = emit_fetch(bld, insn, 1, (c + 2) % 3);
         dst0[c] = bld_insn_3(bld, NV_OP_MAD, src0, src1, dst0[c]);

         dst0[c]->insn->src[2]->mod ^= NV_MOD_NEG;
      }
      break;
   case TGSI_OPCODE_RET:
      (new_instruction(bld->pc, NV_OP_RET))->fixed = 1;
      break;
   case TGSI_OPCODE_END:
      if (bld->ti->p->type == PIPE_SHADER_FRAGMENT)
         bld_export_outputs(bld);
      break;
   default:
      NOUVEAU_ERR("unhandled opcode %u\n", insn->Instruction.Opcode);
      abort();
      break;
   }

   FOR_EACH_DST0_ENABLED_CHANNEL(c, insn)
      emit_store(bld, insn, c, dst0[c]);

   if (prog->type == PIPE_SHADER_VERTEX && prog->vp.clpd_nr &&
       dreg->Register.File == TGSI_FILE_OUTPUT && !dreg->Register.Indirect &&
       prog->out[dreg->Register.Index].sn == TGSI_SEMANTIC_POSITION) {

      int p;
      for (p = 0; p < prog->vp.clpd_nr; p++) {
         struct nv_value *clipd = NULL;

         for (c = 0; c < 4; c++) {
            temp = new_value(bld->pc, NV_FILE_MEM_C(15), NV_TYPE_F32);
            temp->reg.id = p * 4 + c;
            temp = bld_insn_1(bld, NV_OP_LDA, temp);

            clipd = clipd ?
                        bld_insn_3(bld, NV_OP_MAD, dst0[c], temp, clipd) :
                        bld_insn_2(bld, NV_OP_MUL, dst0[c], temp);
         }

         temp = bld_insn_1(bld, NV_OP_MOV, clipd);
         temp->reg.file = NV_FILE_OUT;
         temp->reg.id = bld->ti->p->vp.clpd + p;
         temp->insn->fixed = 1;
      }
   }
}

static INLINE void
bld_free_value_trackers(struct bld_value_stack *base, int n)
{
   int i, c;

   for (i = 0; i < n; ++i)
      for (c = 0; c < 4; ++c)
         if (base[i * 4 + c].body)
            FREE(base[i * 4 + c].body);
}

int
nv50_tgsi_to_nc(struct nv_pc *pc, struct nv50_translation_info *ti)
{
   struct bld_context *bld = CALLOC_STRUCT(bld_context);
   int c;
   unsigned ip;

   pc->root[0] = pc->current_block = new_basic_block(pc);

   bld->pc = pc;
   bld->ti = ti;

   pc->loop_nesting_bound = 1;

   c = util_bitcount(bld->ti->p->fp.interp >> 24);
   if (c && ti->p->type == PIPE_SHADER_FRAGMENT) {
      bld->frgcrd[3] = new_value(pc, NV_FILE_MEM_V, NV_TYPE_F32);
      bld->frgcrd[3]->reg.id = c - 1;
      bld->frgcrd[3] = bld_insn_1(bld, NV_OP_LINTERP, bld->frgcrd[3]);
      bld->frgcrd[3] = bld_insn_1(bld, NV_OP_RCP, bld->frgcrd[3]);
   }

   for (ip = 0; ip < ti->inst_nr; ++ip)
      bld_instruction(bld, &ti->insns[ip]);

   bld_free_value_trackers(&bld->tvs[0][0], BLD_MAX_TEMPS);
   bld_free_value_trackers(&bld->avs[0][0], BLD_MAX_ADDRS);
   bld_free_value_trackers(&bld->pvs[0][0], BLD_MAX_PREDS);

   bld_free_value_trackers(&bld->ovs[0][0], PIPE_MAX_SHADER_OUTPUTS);

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
      for (s = 0; s < 5; ++s) {
         if (!nvi->src[s])
            continue;
         if (nvi->src[s]->value == old_val)
            nv_reference(pc, &nvi->src[s], new_val);
      }
      if (nvi->flags_src && nvi->flags_src->value == old_val)
         nv_reference(pc, &nvi->flags_src, new_val);
   }

   b->pass_seq = pc->pass_seq;

   if (b->out[0] && b->out[0]->pass_seq < pc->pass_seq)
      bld_replace_value(pc, b->out[0], old_val, new_val);

   if (b->out[1] && b->out[1]->pass_seq < pc->pass_seq)
      bld_replace_value(pc, b->out[1], old_val, new_val);
}
