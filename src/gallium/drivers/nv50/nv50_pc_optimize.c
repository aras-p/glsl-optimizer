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

#include "nv50_pc.h"

#define DESCEND_ARBITRARY(j, f)                                 \
do {                                                            \
   b->pass_seq = ctx->pc->pass_seq;                             \
                                                                \
   for (j = 0; j < 2; ++j)                                      \
      if (b->out[j] && b->out[j]->pass_seq < ctx->pc->pass_seq) \
         f(ctx, b->out[j]);	                                  \
} while (0)

extern unsigned nv50_inst_min_size(struct nv_instruction *);

struct nv_pc_pass {
   struct nv_pc *pc;
};

static INLINE boolean
values_equal(struct nv_value *a, struct nv_value *b)
{
   /* XXX: sizes */
   return (a->reg.file == b->reg.file && a->join->reg.id == b->join->reg.id);
}

static INLINE boolean
inst_commutation_check(struct nv_instruction *a,
                       struct nv_instruction *b)
{
   int si, di;

   for (di = 0; di < 4; ++di) {
      if (!a->def[di])
         break;
      for (si = 0; si < 5; ++si) {
         if (!b->src[si])
            continue;
         if (values_equal(a->def[di], b->src[si]->value))
            return FALSE;
      }
   }

   if (b->flags_src && b->flags_src->value == a->flags_def)
      return FALSE;

   return TRUE;
}

/* Check whether we can swap the order of the instructions,
 * where a & b may be either the earlier or the later one.
 */
static boolean
inst_commutation_legal(struct nv_instruction *a,
		       struct nv_instruction *b)
{
   return inst_commutation_check(a, b) && inst_commutation_check(b, a);
}

static INLINE boolean
inst_cullable(struct nv_instruction *nvi)
{
   if (nvi->opcode == NV_OP_STA)
      return FALSE;
   return (!(nvi->is_terminator || nvi->is_join ||
             nvi->target ||
             nvi->fixed ||
             nv_nvi_refcount(nvi)));
}

static INLINE boolean
nvi_isnop(struct nv_instruction *nvi)
{
   if (nvi->opcode == NV_OP_EXPORT || nvi->opcode == NV_OP_UNDEF)
      return TRUE;

   /* NOTE: 'fixed' now only means that it shouldn't be optimized away,
    *  but we can still remove it if it is a no-op move.
    */
   if (/* nvi->fixed || */
       /* nvi->flags_src || */ /* cond. MOV to same register is still NOP */
       nvi->flags_def ||
       nvi->is_terminator ||
       nvi->is_join)
      return FALSE;

   if (nvi->def[0] && nvi->def[0]->join->reg.id < 0)
      return TRUE;

   if (nvi->opcode != NV_OP_MOV && nvi->opcode != NV_OP_SELECT)
      return FALSE;

   if (nvi->def[0]->reg.file != nvi->src[0]->value->reg.file)
      return FALSE;

   if (nvi->src[0]->value->join->reg.id < 0) {
      NV50_DBGMSG(PROG_IR, "nvi_isnop: orphaned value detected\n");
      return TRUE;
   }

   if (nvi->opcode == NV_OP_SELECT)
      if (!values_equal(nvi->def[0], nvi->src[1]->value))
         return FALSE;

   return values_equal(nvi->def[0], nvi->src[0]->value);
}

struct nv_pass {
   struct nv_pc *pc;
   int n;
   void *priv;
};

static int
nv_pass_flatten(struct nv_pass *ctx, struct nv_basic_block *b);

static void
nv_pc_pass_pre_emission(void *priv, struct nv_basic_block *b)
{
   struct nv_pc *pc = (struct nv_pc *)priv;
   struct nv_basic_block *in;
   struct nv_instruction *nvi, *next;
   int j;
   uint size, n32 = 0;

   /* find first non-empty block emitted before b */
   for (j = pc->num_blocks - 1; j >= 0 && !pc->bb_list[j]->bin_size; --j);
   for (; j >= 0; --j) {
      in = pc->bb_list[j];

      /* check for no-op branches (BRA $PC+8) */
      if (in->exit && in->exit->opcode == NV_OP_BRA && in->exit->target == b) {
         in->bin_size -= 8;
         pc->bin_size -= 8;

         for (++j; j < pc->num_blocks; ++j)
            pc->bb_list[j]->bin_pos -= 8;

         nv_nvi_delete(in->exit);
      }
      b->bin_pos = in->bin_pos + in->bin_size;

      if (in->bin_size) /* no more no-op branches to b */
         break;
   }

   pc->bb_list[pc->num_blocks++] = b;

   /* visit node */

   for (nvi = b->entry; nvi; nvi = next) {
      next = nvi->next;
      if (nvi_isnop(nvi))
         nv_nvi_delete(nvi);
   }

   for (nvi = b->entry; nvi; nvi = next) {
      next = nvi->next;

      size = nv50_inst_min_size(nvi);
      if (nvi->next && size < 8)
         ++n32;
      else
      if ((n32 & 1) && nvi->next &&
          nv50_inst_min_size(nvi->next) == 4 &&
          inst_commutation_legal(nvi, nvi->next)) {
         ++n32;
         nv_nvi_permute(nvi, nvi->next);
         next = nvi;
      } else {
         nvi->is_long = 1;

         b->bin_size += n32 & 1;
         if (n32 & 1)
            nvi->prev->is_long = 1;
         n32 = 0;
      }
      b->bin_size += 1 + nvi->is_long;
   }

   if (!b->entry) {
      NV50_DBGMSG(PROG_IR, "block %p is now empty\n", b);
   } else
   if (!b->exit->is_long) {
      assert(n32);
      b->exit->is_long = 1;
      b->bin_size += 1;

      /* might have del'd a hole tail of instructions */
      if (!b->exit->prev->is_long && !(n32 & 1)) {
         b->bin_size += 1;
         b->exit->prev->is_long = 1;
      }
   }
   assert(!b->entry || (b->exit && b->exit->is_long));

   pc->bin_size += b->bin_size *= 4;
}

static int
nv_pc_pass2(struct nv_pc *pc, struct nv_basic_block *root)
{
   struct nv_pass pass;

   pass.pc = pc;

   pc->pass_seq++;

   nv_pass_flatten(&pass, root);

   nv_pc_pass_in_order(root, nv_pc_pass_pre_emission, pc);

   return 0;
}

int
nv_pc_exec_pass2(struct nv_pc *pc)
{
   int i, ret;

   NV50_DBGMSG(PROG_IR, "preparing %u blocks for emission\n", pc->num_blocks);

   pc->num_blocks = 0; /* will reorder bb_list */

   for (i = 0; i < pc->num_subroutines + 1; ++i)
      if (pc->root[i] && (ret = nv_pc_pass2(pc, pc->root[i])))
         return ret;
   return 0;
}

static INLINE boolean
is_cmem_load(struct nv_instruction *nvi)
{
   return (nvi->opcode == NV_OP_LDA &&
	   nvi->src[0]->value->reg.file >= NV_FILE_MEM_C(0) &&
	   nvi->src[0]->value->reg.file <= NV_FILE_MEM_C(15));
}

static INLINE boolean
is_smem_load(struct nv_instruction *nvi)
{
   return (nvi->opcode == NV_OP_LDA &&
	   (nvi->src[0]->value->reg.file == NV_FILE_MEM_S ||
	    nvi->src[0]->value->reg.file <= NV_FILE_MEM_P));
}

static INLINE boolean
is_immd_move(struct nv_instruction *nvi)
{
   return (nvi->opcode == NV_OP_MOV &&
	   nvi->src[0]->value->reg.file == NV_FILE_IMM);
}

static INLINE void
check_swap_src_0_1(struct nv_instruction *nvi)
{
   static const ubyte cc_swapped[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };

   struct nv_ref *src0 = nvi->src[0], *src1 = nvi->src[1];

   if (!nv_op_commutative(nvi->opcode))
      return;
   assert(src0 && src1);

   if (src1->value->reg.file == NV_FILE_IMM)
      return;

   if (is_cmem_load(src0->value->insn)) {
      if (!is_cmem_load(src1->value->insn)) {
         nvi->src[0] = src1;
         nvi->src[1] = src0;
         /* debug_printf("swapping cmem load to 1\n"); */
      }
   } else
   if (is_smem_load(src1->value->insn)) {
      if (!is_smem_load(src0->value->insn)) {
         nvi->src[0] = src1;
         nvi->src[1] = src0;
         /* debug_printf("swapping smem load to 0\n"); */
      }
   }

   if (nvi->opcode == NV_OP_SET && nvi->src[0] != src0)
      nvi->set_cond = (nvi->set_cond & ~7) | cc_swapped[nvi->set_cond & 7];
}

static int
nv_pass_fold_stores(struct nv_pass *ctx, struct nv_basic_block *b)
{
   struct nv_instruction *nvi, *sti, *next;
   int j;

   for (sti = b->entry; sti; sti = next) {
      next = sti->next;

      /* only handling MOV to $oX here */
      if (!sti->def[0] || sti->def[0]->reg.file != NV_FILE_OUT)
         continue;
      if (sti->opcode != NV_OP_MOV && sti->opcode != NV_OP_STA)
         continue;

      nvi = sti->src[0]->value->insn;
      if (!nvi || nvi->opcode == NV_OP_PHI || nv_is_vector_op(nvi->opcode))
         continue;
      assert(nvi->def[0] == sti->src[0]->value);

      if (nvi->opcode == NV_OP_SELECT)
         continue;
      if (nvi->def[0]->refc > 1)
         continue;

      /* cannot write to $oX when using immediate */
      for (j = 0; j < 4 && nvi->src[j]; ++j)
         if (nvi->src[j]->value->reg.file == NV_FILE_IMM ||
             nvi->src[j]->value->reg.file == NV_FILE_MEM_L)
            break;
      if (j < 4 && nvi->src[j])
         continue;

      nvi->def[0] = sti->def[0];
      nvi->def[0]->insn = nvi;
      nvi->fixed = sti->fixed;

      nv_nvi_delete(sti);
   }
   DESCEND_ARBITRARY(j, nv_pass_fold_stores);

   return 0;
}

static int
nv_pass_fold_loads(struct nv_pass *ctx, struct nv_basic_block *b)
{
   struct nv_instruction *nvi, *ld;
   int j;

   for (nvi = b->entry; nvi; nvi = nvi->next) {
      check_swap_src_0_1(nvi);

      for (j = 0; j < 3; ++j) {
         if (!nvi->src[j])
            break;
         ld = nvi->src[j]->value->insn;
         if (!ld)
            continue;

         if (is_immd_move(ld) && nv50_nvi_can_use_imm(nvi, j)) {
            nv_reference(ctx->pc, &nvi->src[j], ld->src[0]->value);
            continue;
         }

         if (ld->opcode != NV_OP_LDA)
            continue;
         if (!nv50_nvi_can_load(nvi, j, ld->src[0]->value))
            continue;

         if (j == 0 && ld->src[4]) /* can't load shared mem */
            continue;

         /* fold it ! */
         nv_reference(ctx->pc, &nvi->src[j], ld->src[0]->value);
         if (ld->src[4])
            nv_reference(ctx->pc, &nvi->src[4], ld->src[4]->value);

         if (!nv_nvi_refcount(ld))
            nv_nvi_delete(ld);
      }
   }
   DESCEND_ARBITRARY(j, nv_pass_fold_loads);

   return 0;
}

/* NOTE: Assumes loads have not yet been folded. */
static int
nv_pass_lower_mods(struct nv_pass *ctx, struct nv_basic_block *b)
{
   int j;
   struct nv_instruction *nvi, *mi, *next;
   ubyte mod;

   for (nvi = b->entry; nvi; nvi = next) {
      next = nvi->next;
      if (nvi->opcode == NV_OP_SUB) {
         nvi->opcode = NV_OP_ADD;
         nvi->src[1]->mod ^= NV_MOD_NEG;
      }

      for (j = 0; j < 4 && nvi->src[j]; ++j) {
         mi = nvi->src[j]->value->insn;
         if (!mi)
            continue;
         if (mi->def[0]->refc > 1)
            continue;

         if (mi->opcode == NV_OP_NEG) mod = NV_MOD_NEG;
         else
         if (mi->opcode == NV_OP_ABS) mod = NV_MOD_ABS;
         else
            continue;
         assert(!(mod & mi->src[0]->mod & NV_MOD_NEG));

         mod |= mi->src[0]->mod;

         if (mi->flags_def || mi->flags_src)
            continue;

         if ((nvi->opcode == NV_OP_ABS) || (nvi->src[j]->mod & NV_MOD_ABS)) {
            /* abs neg [abs] = abs */
            mod &= ~(NV_MOD_NEG | NV_MOD_ABS);
         } else
         if ((nvi->opcode == NV_OP_NEG) && (mod & NV_MOD_NEG)) {
            /* neg as opcode and modifier on same insn cannot occur */
            /* neg neg abs = abs, neg neg = identity */
            assert(j == 0);
            if (mod & NV_MOD_ABS)
               nvi->opcode = NV_OP_ABS;
            else
            if (nvi->flags_def)
               nvi->opcode = NV_OP_CVT;
            else
               nvi->opcode = NV_OP_MOV;
            mod = 0;
         }

         if ((nv50_supported_src_mods(nvi->opcode, j) & mod) != mod)
            continue;

         nv_reference(ctx->pc, &nvi->src[j], mi->src[0]->value);

         nvi->src[j]->mod ^= mod;
      }

      if (nvi->opcode == NV_OP_SAT) {
         mi = nvi->src[0]->value->insn;

         if (mi->opcode != NV_OP_ADD && mi->opcode != NV_OP_MAD)
            continue;
         if (mi->flags_def || mi->def[0]->refc > 1)
            continue;

         mi->saturate = 1;
         mi->def[0] = nvi->def[0];
         mi->def[0]->insn = mi;
         if (nvi->flags_def) {
            mi->flags_def = nvi->flags_def;
            mi->flags_def->insn = mi;
         }
         nv_nvi_delete(nvi);
      }
   }
   DESCEND_ARBITRARY(j, nv_pass_lower_mods);

   return 0;
}

#define SRC_IS_MUL(s) ((s)->insn && (s)->insn->opcode == NV_OP_MUL)

static void
modifiers_apply(uint32_t *val, ubyte type, ubyte mod)
{
   if (mod & NV_MOD_ABS) {
      if (type == NV_TYPE_F32)
         *val &= 0x7fffffff;
      else
      if ((*val) & (1 << 31))
         *val = ~(*val) + 1;
   }
   if (mod & NV_MOD_NEG) {
      if (type == NV_TYPE_F32)
         *val ^= 0x80000000;
      else
         *val = ~(*val) + 1;
   }
}

static INLINE uint
modifiers_opcode(ubyte mod)
{
   switch (mod) {
   case NV_MOD_NEG: return NV_OP_NEG;
   case NV_MOD_ABS: return NV_OP_ABS;
   case 0:
      return NV_OP_MOV;
   default:
      return NV_OP_NOP;
   }
}

static void
constant_expression(struct nv_pc *pc, struct nv_instruction *nvi,
                    struct nv_value *src0, struct nv_value *src1)
{
   struct nv_value *val;
   union {
      float f32;
      uint32_t u32;
      int32_t s32;
   } u0, u1, u;
   ubyte type;

   if (!nvi->def[0])
      return;
   type = nvi->def[0]->reg.type;

   u.u32 = 0;
   u0.u32 = src0->reg.imm.u32;
   u1.u32 = src1->reg.imm.u32;

   modifiers_apply(&u0.u32, type, nvi->src[0]->mod);
   modifiers_apply(&u1.u32, type, nvi->src[1]->mod);

   switch (nvi->opcode) {
   case NV_OP_MAD:
      if (nvi->src[2]->value->reg.file != NV_FILE_GPR)
         return;
      /* fall through */
   case NV_OP_MUL:
      switch (type) {
      case NV_TYPE_F32: u.f32 = u0.f32 * u1.f32; break;
      case NV_TYPE_U32: u.u32 = u0.u32 * u1.u32; break;
      case NV_TYPE_S32: u.s32 = u0.s32 * u1.s32; break;
      default:
         assert(0);
         break;
      }
      break;
   case NV_OP_ADD:
      switch (type) {
      case NV_TYPE_F32: u.f32 = u0.f32 + u1.f32; break;
      case NV_TYPE_U32: u.u32 = u0.u32 + u1.u32; break;
      case NV_TYPE_S32: u.s32 = u0.s32 + u1.s32; break;
      default:
         assert(0);
         break;
      }
      break;
   case NV_OP_SUB:
      switch (type) {
      case NV_TYPE_F32: u.f32 = u0.f32 - u1.f32; break;
      case NV_TYPE_U32: u.u32 = u0.u32 - u1.u32; break;
      case NV_TYPE_S32: u.s32 = u0.s32 - u1.s32; break;
      default:
         assert(0);
         break;
      }
      break;
   default:
      return;
   }

   nvi->opcode = NV_OP_MOV;

   val = new_value(pc, NV_FILE_IMM, type);

   val->reg.imm.u32 = u.u32;

   nv_reference(pc, &nvi->src[1], NULL);
   nv_reference(pc, &nvi->src[0], val);

   if (nvi->src[2]) { /* from MAD */
      nvi->src[1] = nvi->src[0];
      nvi->src[0] = nvi->src[2];
      nvi->src[2] = NULL;
      nvi->opcode = NV_OP_ADD;

      if (val->reg.imm.u32 == 0) {
         nvi->src[1] = NULL;
         nvi->opcode = NV_OP_MOV;
      }
   }
}

static void
constant_operand(struct nv_pc *pc,
                 struct nv_instruction *nvi, struct nv_value *val, int s)
{
   union {
      float f32;
      uint32_t u32;
      int32_t s32;
   } u;
   int t = s ? 0 : 1;
   uint op;
   ubyte type;

   if (!nvi->def[0])
      return;
   type = nvi->def[0]->reg.type;

   u.u32 = val->reg.imm.u32;
   modifiers_apply(&u.u32, type, nvi->src[s]->mod);

   switch (nvi->opcode) {
   case NV_OP_MUL:
      if ((type == NV_TYPE_F32 && u.f32 == 1.0f) ||
          (NV_TYPE_ISINT(type) && u.u32 == 1)) {
         if ((op = modifiers_opcode(nvi->src[t]->mod)) == NV_OP_NOP)
            break;
         nvi->opcode = op;
         nv_reference(pc, &nvi->src[s], NULL);
         nvi->src[0] = nvi->src[t];
         nvi->src[1] = NULL;
      } else
      if ((type == NV_TYPE_F32 && u.f32 == 2.0f) ||
          (NV_TYPE_ISINT(type) && u.u32 == 2)) {
         nvi->opcode = NV_OP_ADD;
         nv_reference(pc, &nvi->src[s], nvi->src[t]->value);
         nvi->src[s]->mod = nvi->src[t]->mod;
      } else
      if (type == NV_TYPE_F32 && u.f32 == -1.0f) {
         if (nvi->src[t]->mod & NV_MOD_NEG)
            nvi->opcode = NV_OP_MOV;
         else
            nvi->opcode = NV_OP_NEG;
         nv_reference(pc, &nvi->src[s], NULL);
         nvi->src[0] = nvi->src[t];
         nvi->src[1] = NULL;
      } else
      if (type == NV_TYPE_F32 && u.f32 == -2.0f) {
         nvi->opcode = NV_OP_ADD;
         nv_reference(pc, &nvi->src[s], nvi->src[t]->value);
         nvi->src[s]->mod = (nvi->src[t]->mod ^= NV_MOD_NEG);
      } else
      if (u.u32 == 0) {
         nvi->opcode = NV_OP_MOV;
         nv_reference(pc, &nvi->src[t], NULL);
         if (s) {
            nvi->src[0] = nvi->src[1];
            nvi->src[1] = NULL;
         }
      }
      break;
   case NV_OP_ADD:
      if (u.u32 == 0) {
         if ((op = modifiers_opcode(nvi->src[t]->mod)) == NV_OP_NOP)
            break;
         nvi->opcode = op;
         nv_reference(pc, &nvi->src[s], NULL);
         nvi->src[0] = nvi->src[t];
         nvi->src[1] = NULL;
      }
      break;
   case NV_OP_RCP:
      u.f32 = 1.0f / u.f32;
      (val = new_value(pc, NV_FILE_IMM, NV_TYPE_F32))->reg.imm.f32 = u.f32;
      nvi->opcode = NV_OP_MOV;
      assert(s == 0);
      nv_reference(pc, &nvi->src[0], val);
      break;
   case NV_OP_RSQ:
      u.f32 = 1.0f / sqrtf(u.f32);
      (val = new_value(pc, NV_FILE_IMM, NV_TYPE_F32))->reg.imm.f32 = u.f32;
      nvi->opcode = NV_OP_MOV;
      assert(s == 0);
      nv_reference(pc, &nvi->src[0], val);
      break;
   default:
      break;
   }

   if (nvi->opcode == NV_OP_MOV && nvi->flags_def) {
      struct nv_instruction *cvt = new_instruction_at(pc, nvi, NV_OP_CVT);

      nv_reference(pc, &cvt->src[0], nvi->def[0]);

      cvt->flags_def = nvi->flags_def;
      nvi->flags_def = NULL;
   }
}

static int
nv_pass_lower_arith(struct nv_pass *ctx, struct nv_basic_block *b)
{
   struct nv_instruction *nvi, *next;
   int j;

   for (nvi = b->entry; nvi; nvi = next) {
      struct nv_value *src0, *src1, *src;
      int mod;

      next = nvi->next;

      src0 = nvcg_find_immediate(nvi->src[0]);
      src1 = nvcg_find_immediate(nvi->src[1]);

      if (src0 && src1)
         constant_expression(ctx->pc, nvi, src0, src1);
      else {
         if (src0)
            constant_operand(ctx->pc, nvi, src0, 0);
         else
         if (src1)
            constant_operand(ctx->pc, nvi, src1, 1);
      }

      /* try to combine MUL, ADD into MAD */
      if (nvi->opcode != NV_OP_ADD)
         continue;

      src0 = nvi->src[0]->value;
      src1 = nvi->src[1]->value;

      if (SRC_IS_MUL(src0) && src0->refc == 1)
         src = src0;
      else
      if (SRC_IS_MUL(src1) && src1->refc == 1)
         src = src1;
      else
         continue;

      /* could have an immediate from above constant_*  */
      if (src0->reg.file != NV_FILE_GPR || src1->reg.file != NV_FILE_GPR)
         continue;

      nvi->opcode = NV_OP_MAD;
      mod = nvi->src[(src == src0) ? 0 : 1]->mod;
      nv_reference(ctx->pc, &nvi->src[(src == src0) ? 0 : 1], NULL);
      nvi->src[2] = nvi->src[(src == src0) ? 1 : 0];

      assert(!(mod & ~NV_MOD_NEG));
      nvi->src[0] = new_ref(ctx->pc, src->insn->src[0]->value);
      nvi->src[1] = new_ref(ctx->pc, src->insn->src[1]->value);
      nvi->src[0]->mod = src->insn->src[0]->mod ^ mod;
      nvi->src[1]->mod = src->insn->src[1]->mod;
   }
   DESCEND_ARBITRARY(j, nv_pass_lower_arith);

   return 0;
}

/* TODO: redundant store elimination */

struct load_record {
   struct load_record *next;
   uint64_t data[2];
   struct nv_value *value;
};

#define LOAD_RECORD_POOL_SIZE 1024

struct nv_pass_reld_elim {
   struct nv_pc *pc;

   struct load_record *imm;
   struct load_record *mem_s;
   struct load_record *mem_v;
   struct load_record *mem_c[16];
   struct load_record *mem_l;

   struct load_record pool[LOAD_RECORD_POOL_SIZE];
   int alloc;
};

/* TODO: properly handle loads from l[] memory in the presence of stores */
static int
nv_pass_reload_elim(struct nv_pass_reld_elim *ctx, struct nv_basic_block *b)
{
   struct load_record **rec, *it;
   struct nv_instruction *ld, *next;
   uint64_t data[2];
   struct nv_value *val;
   int j;

   for (ld = b->entry; ld; ld = next) {
      next = ld->next;
      if (!ld->src[0])
         continue;
      val = ld->src[0]->value;
      rec = NULL;

      if (ld->opcode == NV_OP_LINTERP || ld->opcode == NV_OP_PINTERP) {
         data[0] = val->reg.id;
         data[1] = 0;
         rec = &ctx->mem_v;
      } else
      if (ld->opcode == NV_OP_LDA) {
         data[0] = val->reg.id;
         data[1] = ld->src[4] ? ld->src[4]->value->n : ~0ULL;
         if (val->reg.file >= NV_FILE_MEM_C(0) &&
             val->reg.file <= NV_FILE_MEM_C(15))
            rec = &ctx->mem_c[val->reg.file - NV_FILE_MEM_C(0)];
         else
         if (val->reg.file == NV_FILE_MEM_S)
            rec = &ctx->mem_s;
         else
         if (val->reg.file == NV_FILE_MEM_L)
            rec = &ctx->mem_l;
      } else
      if ((ld->opcode == NV_OP_MOV) && (val->reg.file == NV_FILE_IMM)) {
         data[0] = val->reg.imm.u32;
         data[1] = 0;
         rec = &ctx->imm;
      }

      if (!rec || !ld->def[0]->refc)
         continue;

      for (it = *rec; it; it = it->next)
         if (it->data[0] == data[0] && it->data[1] == data[1])
            break;

      if (it) {
         if (ld->def[0]->reg.id >= 0)
            it->value = ld->def[0];
         else
         if (!ld->fixed)
            nvcg_replace_value(ctx->pc, ld->def[0], it->value);
      } else {
         if (ctx->alloc == LOAD_RECORD_POOL_SIZE)
            continue;
         it = &ctx->pool[ctx->alloc++];
         it->next = *rec;
         it->data[0] = data[0];
         it->data[1] = data[1];
         it->value = ld->def[0];
         *rec = it;
      }
   }

   ctx->imm = NULL;
   ctx->mem_s = NULL;
   ctx->mem_v = NULL;
   for (j = 0; j < 16; ++j)
      ctx->mem_c[j] = NULL;
   ctx->mem_l = NULL;
   ctx->alloc = 0;

   DESCEND_ARBITRARY(j, nv_pass_reload_elim);

   return 0;
}

static int
nv_pass_tex_mask(struct nv_pass *ctx, struct nv_basic_block *b)
{
   int i, c, j;

   for (i = 0; i < ctx->pc->num_instructions; ++i) {
      struct nv_instruction *nvi = &ctx->pc->instructions[i];
      struct nv_value *def[4];

      if (!nv_is_vector_op(nvi->opcode))
         continue;
      nvi->tex_mask = 0;

      for (c = 0; c < 4; ++c) {
         if (nvi->def[c]->refc)
            nvi->tex_mask |= 1 << c;
         def[c] = nvi->def[c];
      }

      j = 0;
      for (c = 0; c < 4; ++c)
         if (nvi->tex_mask & (1 << c))
            nvi->def[j++] = def[c];
      for (c = 0; c < 4; ++c)
         if (!(nvi->tex_mask & (1 << c)))
           nvi->def[j++] = def[c];
      assert(j == 4);
   }
   return 0;
}

struct nv_pass_dce {
   struct nv_pc *pc;
   uint removed;
};

static int
nv_pass_dce(struct nv_pass_dce *ctx, struct nv_basic_block *b)
{
   int j;
   struct nv_instruction *nvi, *next;

   for (nvi = b->phi ? b->phi : b->entry; nvi; nvi = next) {
      next = nvi->next;

      if (inst_cullable(nvi)) {
         nv_nvi_delete(nvi);

         ++ctx->removed;
      }
   }
   DESCEND_ARBITRARY(j, nv_pass_dce);

   return 0;
}

/* Register allocation inserted ELSE blocks for all IF/ENDIF without ELSE.
 * Returns TRUE if @bb initiates an IF/ELSE/ENDIF clause, or is an IF with
 * BREAK and dummy ELSE block.
 */
static INLINE boolean
bb_is_if_else_endif(struct nv_basic_block *bb)
{
   if (!bb->out[0] || !bb->out[1])
      return FALSE;

   if (bb->out[0]->out_kind[0] == CFG_EDGE_LOOP_LEAVE) {
      return (bb->out[0]->out[1] == bb->out[1]->out[0] &&
              !bb->out[1]->out[1]);
   } else {
      return (bb->out[0]->out[0] == bb->out[1]->out[0] &&
              !bb->out[0]->out[1] &&
              !bb->out[1]->out[1]);
   }
}

/* predicate instructions and remove branch at the end */
static void
predicate_instructions(struct nv_pc *pc, struct nv_basic_block *b,
                       struct nv_value *p, ubyte cc)
{
   struct nv_instruction *nvi;

   if (!b->entry)
      return;
   for (nvi = b->entry; nvi->next; nvi = nvi->next) {
      if (!nvi_isnop(nvi)) {
         nvi->cc = cc;
         nv_reference(pc, &nvi->flags_src, p);
      }
   }

   if (nvi->opcode == NV_OP_BRA)
      nv_nvi_delete(nvi);
   else
   if (!nvi_isnop(nvi)) {
      nvi->cc = cc;
      nv_reference(pc, &nvi->flags_src, p);
   }
}

/* NOTE: Run this after register allocation, we can just cut out the cflow
 * instructions and hook the predicates to the conditional OPs if they are
 * not using immediates; better than inserting SELECT to join definitions.
 *
 * NOTE: Should adapt prior optimization to make this possible more often.
 */
static int
nv_pass_flatten(struct nv_pass *ctx, struct nv_basic_block *b)
{
   struct nv_instruction *nvi;
   struct nv_value *pred;
   int i;
   int n0 = 0, n1 = 0;

   if (bb_is_if_else_endif(b)) {

      NV50_DBGMSG(PROG_IR,
                  "pass_flatten: IF/ELSE/ENDIF construct at BB:%i\n", b->id);

      for (n0 = 0, nvi = b->out[0]->entry; nvi; nvi = nvi->next, ++n0)
         if (!nv50_nvi_can_predicate(nvi))
            break;
      if (!nvi) {
         for (n1 = 0, nvi = b->out[1]->entry; nvi; nvi = nvi->next, ++n1)
            if (!nv50_nvi_can_predicate(nvi))
               break;
#if NV50_DEBUG & NV50_DEBUG_PROG_IR
         if (nvi) {
            debug_printf("cannot predicate: "); nv_print_instruction(nvi);
         }
      } else {
         debug_printf("cannot predicate: "); nv_print_instruction(nvi);
#endif
      }

      if (!nvi && n0 < 12 && n1 < 12) { /* 12 as arbitrary limit */
         assert(b->exit && b->exit->flags_src);
         pred = b->exit->flags_src->value;

         predicate_instructions(ctx->pc, b->out[0], pred, NV_CC_NE | NV_CC_U);
         predicate_instructions(ctx->pc, b->out[1], pred, NV_CC_EQ);

         assert(b->exit && b->exit->opcode == NV_OP_BRA);
         nv_nvi_delete(b->exit);

         if (b->exit && b->exit->opcode == NV_OP_JOINAT)
            nv_nvi_delete(b->exit);

         i = (b->out[0]->out_kind[0] == CFG_EDGE_LOOP_LEAVE) ? 1 : 0;

         if ((nvi = b->out[0]->out[i]->entry)) {
            nvi->is_join = 0;
            if (nvi->opcode == NV_OP_JOIN)
               nv_nvi_delete(nvi);
         }
      }
   }
   DESCEND_ARBITRARY(i, nv_pass_flatten);

   return 0;
}

/* local common subexpression elimination, stupid O(n^2) implementation */
static int
nv_pass_cse(struct nv_pass *ctx, struct nv_basic_block *b)
{
   struct nv_instruction *ir, *ik, *next;
   struct nv_instruction *entry = b->phi ? b->phi : b->entry;
   int s;
   unsigned int reps;

   do {
      reps = 0;
      for (ir = entry; ir; ir = next) {
         next = ir->next;
         for (ik = entry; ik != ir; ik = ik->next) {
            if (ir->opcode != ik->opcode || ir->fixed)
               continue;

            if (!ir->def[0] || !ik->def[0] ||
                ik->opcode == NV_OP_LDA ||
                ik->opcode == NV_OP_STA ||
                ik->opcode == NV_OP_MOV ||
                nv_is_vector_op(ik->opcode))
               continue; /* ignore loads, stores & moves */

            if (ik->src[4] || ir->src[4])
               continue; /* don't mess with address registers */

            if (ik->flags_src || ir->flags_src ||
                ik->flags_def || ir->flags_def)
               continue; /* and also not with flags, for now */

            if (ik->def[0]->reg.file == NV_FILE_OUT ||
                ir->def[0]->reg.file == NV_FILE_OUT ||
                !values_equal(ik->def[0], ir->def[0]))
               continue;

            for (s = 0; s < 3; ++s) {
               struct nv_value *a, *b;

               if (!ik->src[s]) {
                  if (ir->src[s])
                     break;
                  continue;
               }
               if (ik->src[s]->mod != ir->src[s]->mod)
                  break;
               a = ik->src[s]->value;
               b = ir->src[s]->value;
               if (a == b)
                  continue;
               if (a->reg.file != b->reg.file ||
                   a->reg.id < 0 ||
                   a->reg.id != b->reg.id)
                  break;
            }
            if (s == 3) {
               nv_nvi_delete(ir);
               ++reps;
               nvcg_replace_value(ctx->pc, ir->def[0], ik->def[0]);
               break;
            }
         }
      }
   } while(reps);

   DESCEND_ARBITRARY(s, nv_pass_cse);

   return 0;
}

static int
nv_pc_pass0(struct nv_pc *pc, struct nv_basic_block *root)
{
   struct nv_pass_reld_elim *reldelim;
   struct nv_pass pass;
   struct nv_pass_dce dce;
   int ret;

   pass.n = 0;
   pass.pc = pc;

   /* Do this first, so we don't have to pay attention
    * to whether sources are supported memory loads.
    */
   pc->pass_seq++;
   ret = nv_pass_lower_arith(&pass, root);
   if (ret)
      return ret;

   pc->pass_seq++;
   ret = nv_pass_lower_mods(&pass, root);
   if (ret)
      return ret;

   pc->pass_seq++;
   ret = nv_pass_fold_loads(&pass, root);
   if (ret)
      return ret;

   pc->pass_seq++;
   ret = nv_pass_fold_stores(&pass, root);
   if (ret)
      return ret;

   if (pc->opt_reload_elim) {
      reldelim = CALLOC_STRUCT(nv_pass_reld_elim);
      reldelim->pc = pc;
      pc->pass_seq++;
      ret = nv_pass_reload_elim(reldelim, root);
      FREE(reldelim);
      if (ret)
         return ret;
   }

   pc->pass_seq++;
   ret = nv_pass_cse(&pass, root);
   if (ret)
      return ret;

   dce.pc = pc;
   do {
      dce.removed = 0;
      pc->pass_seq++;
      ret = nv_pass_dce(&dce, root);
      if (ret)
         return ret;
   } while (dce.removed);

   ret = nv_pass_tex_mask(&pass, root);
   if (ret)
      return ret;

   return ret;
}

int
nv_pc_exec_pass0(struct nv_pc *pc)
{
   int i, ret;

   for (i = 0; i < pc->num_subroutines + 1; ++i)
      if (pc->root[i] && (ret = nv_pc_pass0(pc, pc->root[i])))
         return ret;
   return 0;
}
