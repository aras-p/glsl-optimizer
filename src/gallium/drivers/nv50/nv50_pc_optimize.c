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
   return (!(nvi->is_terminator ||
             nvi->target ||
             nvi->fixed ||
             nv_nvi_refcount(nvi)));
}

static INLINE boolean
nvi_isnop(struct nv_instruction *nvi)
{
   if (nvi->opcode == NV_OP_EXPORT)
      return TRUE;

   if (nvi->fixed ||
       nvi->is_terminator ||
       nvi->flags_src ||
       nvi->flags_def)
      return FALSE;

   if (nvi->def[0]->join->reg.id < 0)
      return TRUE;

   if (nvi->opcode != NV_OP_MOV && nvi->opcode != NV_OP_SELECT)
      return FALSE;

   if (nvi->def[0]->reg.file != nvi->src[0]->value->reg.file)
      return FALSE;

   if (nvi->src[0]->value->join->reg.id < 0) {
      debug_printf("nvi_isnop: orphaned value detected\n");
      return TRUE;
   }

   if (nvi->opcode == NV_OP_SELECT)
      if (!values_equal(nvi->def[0], nvi->src[1]->value))
         return FALSE;

   return values_equal(nvi->def[0], nvi->src[0]->value);
}

static void
nv_pc_pass_pre_emission(struct nv_pc *pc, struct nv_basic_block *b)
{
   struct nv_basic_block *in;
   struct nv_instruction *nvi, *next;
   int j;
   uint size, n32 = 0;

   b->priv = 0;

   for (j = pc->num_blocks - 1; j >= 0 && !pc->bb_list[j]->bin_size; --j);
   if (j >= 0) {
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
         debug_printf("permuting: ");
         nv_print_instruction(nvi);
         nv_print_instruction(nvi->next);
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
      debug_printf("block %p is now empty\n", b);
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

   /* descend CFG */

   if (!b->out[0])
      return;
   if (!b->out[1] && ++(b->out[0]->priv) != b->out[0]->num_in)
      return;

   for (j = 0; j < 2; ++j)
      if (b->out[j] && b->out[j] != b)
         nv_pc_pass_pre_emission(pc, b->out[j]);
}

int
nv_pc_exec_pass2(struct nv_pc *pc)
{
   debug_printf("preparing %u blocks for emission\n", pc->num_blocks);

   pc->bb_list = CALLOC(pc->num_blocks, sizeof(struct nv_basic_block *));
  
   pc->num_blocks = 0;
   nv_pc_pass_pre_emission(pc, pc->root);

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
      nvi->set_cond = cc_swapped[nvi->set_cond];
}

struct nv_pass {
   struct nv_pc *pc;
   int n;
   void *priv;
};

static int
nv_pass_fold_stores(struct nv_pass *ctx, struct nv_basic_block *b)
{
   struct nv_instruction *nvi, *sti;
   int j;

   for (sti = b->entry; sti; sti = sti->next) {
      if (!sti->def[0] || sti->def[0]->reg.file != NV_FILE_OUT)
         continue;

      /* only handling MOV to $oX here */
      if (sti->opcode != NV_OP_MOV && sti->opcode != NV_OP_STA)
         continue;

      nvi = sti->src[0]->value->insn;
      if (!nvi || nvi->opcode == NV_OP_PHI)
         continue;
      assert(nvi->def[0] == sti->src[0]->value);

      if (nvi->def[0]->refc > 1)
         continue;

      /* cannot MOV immediate to $oX */
      if (nvi->src[0]->value->reg.file == NV_FILE_IMM)
         continue;

      nvi->def[0] = sti->def[0];
      sti->def[0] = NULL;
      nvi->fixed = sti->fixed;
      sti->fixed = 0;
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
            debug_printf("folded immediate %i\n", ld->def[0]->n);
            continue;
         }

         if (ld->opcode != NV_OP_LDA)
            continue;
         if (!nv50_nvi_can_load(nvi, j, ld->src[0]->value))
            continue;

         if (j == 0 && ld->src[4]) /* can't load shared mem */
            continue;

         /* fold it ! */ /* XXX: ref->insn */
         nv_reference(ctx->pc, &nvi->src[j], ld->src[0]->value);
         if (ld->src[4])
            nv_reference(ctx->pc, &nvi->src[4], ld->src[4]->value);
      }
   }
   DESCEND_ARBITRARY(j, nv_pass_fold_loads);

   return 0;
}

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

      /* should not put any modifiers on NEG and ABS */
      assert(nvi->opcode != NV_MOD_NEG || !nvi->src[0]->mod);
      assert(nvi->opcode != NV_MOD_ABS || !nvi->src[0]->mod);

      for (j = 0; j < 4; ++j) {
         if (!nvi->src[j])
            break;

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

         if (nvi->opcode == NV_OP_ABS)
            mod &= ~(NV_MOD_NEG | NV_MOD_ABS);
         else
         if (nvi->opcode == NV_OP_NEG && mod == NV_MOD_NEG) {
            nvi->opcode = NV_OP_MOV;
            mod = 0;
         }

         if (!(nv50_supported_src_mods(nvi->opcode, j) & mod))
            continue;

         nv_reference(ctx->pc, &nvi->src[j], mi->src[0]->value);

         nvi->src[j]->mod ^= mod;
      }

      if (nvi->opcode == NV_OP_SAT) {
         mi = nvi->src[0]->value->insn;

         if ((mi->opcode == NV_OP_MAD) && !mi->flags_def) {
            mi->saturate = 1;
            mi->def[0] = nvi->def[0];
            nv_nvi_delete(nvi);
         }
      }
   }
   DESCEND_ARBITRARY(j, nv_pass_lower_mods);

   return 0;
}

#define SRC_IS_MUL(s) ((s)->insn && (s)->insn->opcode == NV_OP_MUL)

static struct nv_value *
find_immediate(struct nv_ref *ref)
{
   struct nv_value *src;

   if (!ref)
      return NULL;

   src = ref->value;
   while (src->insn && src->insn->opcode == NV_OP_MOV) {
      assert(!src->insn->src[0]->mod);
      src = src->insn->src[0]->value;
   }
   return (src->reg.file == NV_FILE_IMM) ? src : NULL;
}

static void
constant_operand(struct nv_pc *pc,
                 struct nv_instruction *nvi, struct nv_value *val, int s)
{
   int t = s ? 0 : 1;
   ubyte type;

   if (!nvi->def[0])
      return;
   type = nvi->def[0]->reg.type;

   switch (nvi->opcode) {
   case NV_OP_MUL:
      if ((type == NV_TYPE_F32 && val->reg.imm.f32 == 1.0f) ||
          (NV_TYPE_ISINT(type) && val->reg.imm.u32 == 1)) {
         nvi->opcode = NV_OP_MOV;
         nv_reference(pc, &nvi->src[s], NULL);
         if (!s) {
            nvi->src[0] = nvi->src[1];
            nvi->src[1] = NULL;
         }
      } else
      if ((type == NV_TYPE_F32 && val->reg.imm.f32 == 2.0f) ||
          (NV_TYPE_ISINT(type) && val->reg.imm.u32 == 2)) {
         nvi->opcode = NV_OP_ADD;
         nv_reference(pc, &nvi->src[s], nvi->src[t]->value);
      } else
      if (type == NV_TYPE_F32 && val->reg.imm.f32 == -1.0f) {
         nvi->opcode = NV_OP_NEG;
         nv_reference(pc, &nvi->src[s], NULL);
         nvi->src[0] = nvi->src[t];
         nvi->src[1] = NULL;
      } else
      if (type == NV_TYPE_F32 && val->reg.imm.f32 == -2.0f) {
         nvi->opcode = NV_OP_ADD;
         assert(!nvi->src[s]->mod);
         nv_reference(pc, &nvi->src[s], nvi->src[t]->value);
         nvi->src[t]->mod ^= NV_MOD_NEG;
         nvi->src[s]->mod |= NV_MOD_NEG;
      } else
      if (val->reg.imm.u32 == 0) {
         nvi->opcode = NV_OP_MOV;
         nv_reference(pc, &nvi->src[t], NULL);
         if (s) {
            nvi->src[0] = nvi->src[1];
            nvi->src[1] = NULL;
         }
      }
      break;
   case NV_OP_ADD:
      if (val->reg.imm.u32 == 0) {
         nvi->opcode = NV_OP_MOV;
         nv_reference(pc, &nvi->src[s], NULL);
         nvi->src[0] = nvi->src[t];
         nvi->src[1] = NULL;
      }
      break;
   default:
      break;
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

      if ((src = find_immediate(nvi->src[0])) != NULL)
         constant_operand(ctx->pc, nvi, src, 0);
      else
      if ((src = find_immediate(nvi->src[1])) != NULL)
         constant_operand(ctx->pc, nvi, src, 1);

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

/*
set $r2 g f32 $r2 $r3
cvt abs rn f32 $r2 s32 $r2
cvt f32 $c0 # f32 $r2
e $c0 bra 0x80
*/
#if 0
static int
nv_pass_lower_cond(struct nv_pass *ctx, struct nv_basic_block *b)
{
   /* XXX: easier in IR builder for now */
   return 0;
}
#endif

/* TODO: redundant store elimination */

struct load_record {
   struct load_record *next;
   uint64_t data;
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

static int
nv_pass_reload_elim(struct nv_pass_reld_elim *ctx, struct nv_basic_block *b)
{
   struct load_record **rec, *it;
   struct nv_instruction *ld, *next;
   uint64_t data;
   struct nv_value *val;
   int j;

   for (ld = b->entry; ld; ld = next) {
      next = ld->next;
      if (!ld->src[0])
         continue;
      val = ld->src[0]->value;
      rec = NULL;

      if (ld->opcode == NV_OP_LINTERP || ld->opcode == NV_OP_PINTERP) {
         data = val->reg.id;
         rec = &ctx->mem_v;
      } else
      if (ld->opcode == NV_OP_LDA) {
         data = val->reg.id;
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
         data = val->reg.imm.u32;
         rec = &ctx->imm;
      }

      if (!rec || !ld->def[0]->refc)
         continue;

      for (it = *rec; it; it = it->next)
         if (it->data == data)
            break;

      if (it) {
#if 1
         nvcg_replace_value(ctx->pc, ld->def[0], it->value);
#else
         ld->opcode = NV_OP_MOV;
         nv_reference(ctx->pc, &ld->src[0], it->value);
#endif
      } else {
         if (ctx->alloc == LOAD_RECORD_POOL_SIZE)
            continue;
         it = &ctx->pool[ctx->alloc++];
         it->next = *rec;
         it->data = data;
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

   for (nvi = b->entry; nvi; nvi = next) {
      next = nvi->next;

      if (inst_cullable(nvi)) {
         nv_nvi_delete(nvi);

         ++ctx->removed;
      }
   }
   DESCEND_ARBITRARY(j, nv_pass_dce);

   return 0;
}

static INLINE boolean
bb_simple_if_endif(struct nv_basic_block *bb)
{
   return (bb->out[0] && bb->out[1] &&
           bb->out[0]->out[0] == bb->out[1] &&
           !bb->out[0]->out[1]);
}

static int
nv_pass_flatten(struct nv_pass *ctx, struct nv_basic_block *b)
{
   int j;

   if (bb_simple_if_endif(b)) {
      ++ctx->n;
      debug_printf("nv_pass_flatten: total IF/ENDIF constructs: %i\n", ctx->n);
   }
   DESCEND_ARBITRARY(j, nv_pass_flatten);

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
            if (ir->opcode != ik->opcode)
               continue;

            if (ik->opcode == NV_OP_LDA ||
                ik->opcode == NV_OP_STA ||
                ik->opcode == NV_OP_MOV ||
                nv_is_vector_op(ik->opcode))
               continue; /* ignore loads, stores & moves */

            if (ik->src[4] || ir->src[4])
               continue; /* don't mess with address registers */

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

int
nv_pc_exec_pass0(struct nv_pc *pc)
{
   struct nv_pass_reld_elim *reldelim;
   struct nv_pass pass;
   struct nv_pass_dce dce;
   int ret;

   pass.pc = pc;

   pc->pass_seq++;
   ret = nv_pass_flatten(&pass, pc->root);
   if (ret)
      return ret;

   /* Do this first, so we don't have to pay attention
    * to whether sources are supported memory loads.
    */
   pc->pass_seq++;
   ret = nv_pass_lower_arith(&pass, pc->root);
   if (ret)
      return ret;

   pc->pass_seq++;
   ret = nv_pass_fold_loads(&pass, pc->root);
   if (ret)
      return ret;

   pc->pass_seq++;
   ret = nv_pass_fold_stores(&pass, pc->root);
   if (ret)
      return ret;

   reldelim = CALLOC_STRUCT(nv_pass_reld_elim);
   reldelim->pc = pc;
   pc->pass_seq++;
   ret = nv_pass_reload_elim(reldelim, pc->root);
   FREE(reldelim);
   if (ret)
      return ret;

   pc->pass_seq++;
   ret = nv_pass_cse(&pass, pc->root);
   if (ret)
      return ret;

   pc->pass_seq++;
   ret = nv_pass_lower_mods(&pass, pc->root);
   if (ret)
      return ret;

   dce.pc = pc;
   do {
      dce.removed = 0;
      pc->pass_seq++;
      ret = nv_pass_dce(&dce, pc->root);
      if (ret)
         return ret;
   } while (dce.removed);

   ret = nv_pass_tex_mask(&pass, pc->root);
   if (ret)
      return ret;

   return ret;
}
