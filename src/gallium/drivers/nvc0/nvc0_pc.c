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

#include "nvc0_pc.h"
#include "nvc0_program.h"

uint8_t
nvc0_ir_reverse_cc(uint8_t cc)
{
   static const uint8_t cc_swapped[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };

   return cc_swapped[cc & 7] | (cc & ~7);
}

boolean
nvc0_insn_can_load(struct nv_instruction *nvi, int s,
                   struct nv_instruction *ld)
{
   int i;

   if (ld->opcode == NV_OP_MOV && ld->src[0]->value->reg.file == NV_FILE_IMM) {
      if (s > 1 || !(nvc0_op_info_table[nvi->opcode].immediate & (1 << s)))
         return FALSE;
      if (!(nvc0_op_info_table[nvi->opcode].immediate & 4))
         if (ld->src[0]->value->reg.imm.u32 & 0xfff)
            return FALSE;
   } else
   if (!(nvc0_op_info_table[nvi->opcode].memory & (1 << s)))
      return FALSE;

   if (ld->indirect >= 0)
      return FALSE;

   /* a few ops can use g[] sources directly, but we don't support g[] yet */
   if (ld->src[0]->value->reg.file == NV_FILE_MEM_L ||
       ld->src[0]->value->reg.file == NV_FILE_MEM_G)
      return FALSE;

   for (i = 0; i < 3 && nvi->src[i]; ++i)
      if (nvi->src[i]->value->reg.file == NV_FILE_IMM)
         return FALSE;

   return TRUE;
}

/* Return whether this instruction can be executed conditionally. */
boolean
nvc0_insn_is_predicateable(struct nv_instruction *nvi)
{
   if (nvi->predicate >= 0) /* already predicated */
      return FALSE;
   if (!nvc0_op_info_table[nvi->opcode].predicate &&
       !nvc0_op_info_table[nvi->opcode].pseudo)
      return FALSE;
   return TRUE;
}

int
nvc0_insn_refcount(struct nv_instruction *nvi)
{
   int rc = 0;
   int i;
   for (i = 0; i < 5 && nvi->def[i]; ++i) {
      if (!nvi->def[i])
         return rc;
      rc += nvi->def[i]->refc;
   }
   return rc;
}

int
nvc0_pc_replace_value(struct nv_pc *pc,
		      struct nv_value *old_val,
		      struct nv_value *new_val)
{
   int i, n, s;

   if (old_val == new_val)
      return old_val->refc;

   for (i = 0, n = 0; i < pc->num_refs; ++i) {
      if (pc->refs[i]->value == old_val) {
         ++n;
         for (s = 0; s < 6 && pc->refs[i]->insn->src[s]; ++s)
            if (pc->refs[i]->insn->src[s] == pc->refs[i])
               break;
         assert(s < 6);
         nv_reference(pc, pc->refs[i]->insn, s, new_val);
      }
   }
   return n;
}

static INLINE boolean
is_gpr63(struct nv_value *val)
{
   return (val->reg.file == NV_FILE_GPR && val->reg.id == 63);
}

struct nv_value *
nvc0_pc_find_constant(struct nv_ref *ref)
{
   struct nv_value *src;

   if (!ref)
      return NULL;

   src = ref->value;
   while (src->insn && src->insn->opcode == NV_OP_MOV) {
      assert(!src->insn->src[0]->mod);
      src = src->insn->src[0]->value;
   }
   if ((src->reg.file == NV_FILE_IMM) || is_gpr63(src) ||
       (src->insn &&
        src->insn->opcode == NV_OP_LD &&
        src->insn->src[0]->value->reg.file >= NV_FILE_MEM_C(0) &&
        src->insn->src[0]->value->reg.file <= NV_FILE_MEM_C(15)))
      return src;
   return NULL;
}

struct nv_value *
nvc0_pc_find_immediate(struct nv_ref *ref)
{
   struct nv_value *src = nvc0_pc_find_constant(ref);

   return (src && (src->reg.file == NV_FILE_IMM || is_gpr63(src))) ? src : NULL;
}

static void
nv_pc_free_refs(struct nv_pc *pc)
{
   int i;
   for (i = 0; i < pc->num_refs; i += 64)
      FREE(pc->refs[i]);
   FREE(pc->refs);
}

static const char *
edge_name(ubyte type)
{
   switch (type) {
   case CFG_EDGE_FORWARD: return "forward";
   case CFG_EDGE_BACK: return "back";
   case CFG_EDGE_LOOP_ENTER: return "loop";
   case CFG_EDGE_LOOP_LEAVE: return "break";
   case CFG_EDGE_FAKE: return "fake";
   default:
      return "?";
   }
}

void
nvc0_pc_pass_in_order(struct nv_basic_block *root, nv_pc_pass_func f,
                      void *priv)
{
   struct nv_basic_block *bb[64], *bbb[16], *b;
   int j, p, pp;

   bb[0] = root;
   p = 1;
   pp = 0;

   while (p > 0) {
      b = bb[--p];
      b->priv = 0;

      for (j = 1; j >= 0; --j) {
         if (!b->out[j])
            continue;

         switch (b->out_kind[j]) {
         case CFG_EDGE_BACK:
            continue;
         case CFG_EDGE_FORWARD:
         case CFG_EDGE_FAKE:
            if (++b->out[j]->priv == b->out[j]->num_in)
               bb[p++] = b->out[j];
            break;
         case CFG_EDGE_LOOP_ENTER:
            bb[p++] = b->out[j];
            break;
         case CFG_EDGE_LOOP_LEAVE:
            if (!b->out[j]->priv) {
               bbb[pp++] = b->out[j];
               b->out[j]->priv = 1;
            }
            break;
         default:
            assert(0);
            break;
         }
      }

      f(priv, b);

      if (!p) {
         p = pp;
         for (; pp > 0; --pp)
            bb[pp - 1] = bbb[pp - 1];
      }
   }
}

static void
nv_do_print_function(void *priv, struct nv_basic_block *b)
{
   struct nv_instruction *i;

   debug_printf("=== BB %i ", b->id);
   if (b->out[0])
      debug_printf("[%s -> %i] ", edge_name(b->out_kind[0]), b->out[0]->id);
   if (b->out[1])
      debug_printf("[%s -> %i] ", edge_name(b->out_kind[1]), b->out[1]->id);
   debug_printf("===\n");

   i = b->phi;
   if (!i)
      i = b->entry;
   for (; i; i = i->next)
      nvc0_print_instruction(i);
}

void
nvc0_print_function(struct nv_basic_block *root)
{
   if (root->subroutine)
      debug_printf("SUBROUTINE %i\n", root->subroutine);
   else
      debug_printf("MAIN\n");

   nvc0_pc_pass_in_order(root, nv_do_print_function, root);
}

void
nvc0_print_program(struct nv_pc *pc)
{
   int i;
   for (i = 0; i < pc->num_subroutines + 1; ++i)
      if (pc->root[i])
         nvc0_print_function(pc->root[i]);
}

#if NV50_DEBUG & NV50_DEBUG_PROG_CFLOW
static void
nv_do_print_cfgraph(struct nv_pc *pc, FILE *f, struct nv_basic_block *b)
{
   int i;

   b->pass_seq = pc->pass_seq;

   fprintf(f, "\t%i [shape=box]\n", b->id);

   for (i = 0; i < 2; ++i) {
      if (!b->out[i])
         continue;
      switch (b->out_kind[i]) {
      case CFG_EDGE_FORWARD:
         fprintf(f, "\t%i -> %i;\n", b->id, b->out[i]->id);
         break;
      case CFG_EDGE_LOOP_ENTER:
         fprintf(f, "\t%i -> %i [color=green];\n", b->id, b->out[i]->id);
         break;
      case CFG_EDGE_LOOP_LEAVE:
         fprintf(f, "\t%i -> %i [color=red];\n", b->id, b->out[i]->id);
         break;
      case CFG_EDGE_BACK:
         fprintf(f, "\t%i -> %i;\n", b->id, b->out[i]->id);
         continue;
      case CFG_EDGE_FAKE:
         fprintf(f, "\t%i -> %i [style=dotted];\n", b->id, b->out[i]->id);
         break;
      default:
         assert(0);
         break;
      }
      if (b->out[i]->pass_seq < pc->pass_seq)
         nv_do_print_cfgraph(pc, f, b->out[i]);
   }
}

/* Print the control flow graph of subroutine @subr (0 == MAIN) to a file. */
static void
nv_print_cfgraph(struct nv_pc *pc, const char *filepath, int subr)
{
   FILE *f;

   f = fopen(filepath, "a");
   if (!f)
      return;

   fprintf(f, "digraph G {\n");

   ++pc->pass_seq;

   nv_do_print_cfgraph(pc, f, pc->root[subr]);

   fprintf(f, "}\n");

   fclose(f);
}
#endif

static INLINE void
nvc0_pc_print_binary(struct nv_pc *pc)
{
   unsigned i;

   NV50_DBGMSG(SHADER, "nvc0_pc_print_binary(%u ops)\n", pc->emit_size / 8);

   for (i = 0; i < pc->emit_size / 4; i += 2) {
      debug_printf("0x%08x ", pc->emit[i + 0]);
      debug_printf("0x%08x ", pc->emit[i + 1]);
      if ((i % 16) == 15)
         debug_printf("\n");
   }
   debug_printf("\n");
}

static int
nvc0_emit_program(struct nv_pc *pc)
{
   uint32_t *code = pc->emit;
   int n;

   NV50_DBGMSG(SHADER, "emitting program: size = %u\n", pc->emit_size);

   pc->emit_pos = 0;
   for (n = 0; n < pc->num_blocks; ++n) {
      struct nv_instruction *i;
      struct nv_basic_block *b = pc->bb_list[n];

      for (i = b->entry; i; i = i->next) {
         nvc0_emit_instruction(pc, i);
         pc->emit += 2;
         pc->emit_pos += 8;
      }
   }
   assert(pc->emit == &code[pc->emit_size / 4]);

   pc->emit[0] = 0x00001de7;
   pc->emit[1] = 0x80000000;
   pc->emit_size += 8;

   pc->emit = code;

#if NV50_DEBUG & NV50_DEBUG_SHADER
   nvc0_pc_print_binary(pc);
#endif

   return 0;
}

int
nvc0_generate_code(struct nvc0_translation_info *ti)
{
   struct nv_pc *pc;
   int ret;
   int i;

   pc = CALLOC_STRUCT(nv_pc);
   if (!pc)
      return 1;

   pc->is_fragprog = ti->prog->type == PIPE_SHADER_FRAGMENT;

   pc->root = CALLOC(ti->num_subrs + 1, sizeof(pc->root[0]));
   if (!pc->root) {
      FREE(pc);
      return 1;
   }
   pc->num_subroutines = ti->num_subrs;

   ret = nvc0_tgsi_to_nc(pc, ti);
   if (ret)
      goto out;
#if NV50_DEBUG & NV50_DEBUG_PROG_IR
   nvc0_print_program(pc);
#endif

   pc->opt_reload_elim = ti->require_stores ? FALSE : TRUE;

   /* optimization */
   ret = nvc0_pc_exec_pass0(pc);
   if (ret)
      goto out;
#if NV50_DEBUG & NV50_DEBUG_PROG_IR
   nvc0_print_program(pc);
#endif

   /* register allocation */
   ret = nvc0_pc_exec_pass1(pc);
   if (ret)
      goto out;
#if NV50_DEBUG & NV50_DEBUG_PROG_CFLOW
   nvc0_print_program(pc);
   nv_print_cfgraph(pc, "nvc0_shader_cfgraph.dot", 0);
#endif

   /* prepare for emission */
   ret = nvc0_pc_exec_pass2(pc);
   if (ret)
      goto out;
   assert(!(pc->emit_size % 8));

   pc->emit = CALLOC(pc->emit_size / 4 + 2, 4);
   if (!pc->emit) {
      ret = 3;
      goto out;
   }
   ret = nvc0_emit_program(pc);
   if (ret)
      goto out;

   ti->prog->code = pc->emit;
   ti->prog->code_base = 0;
   ti->prog->code_size = pc->emit_size;
   ti->prog->parm_size = 0;

   ti->prog->max_gpr = MAX2(4, pc->max_reg[NV_FILE_GPR] + 1);

   ti->prog->relocs = pc->reloc_entries;
   ti->prog->num_relocs = pc->num_relocs;

   NV50_DBGMSG(SHADER, "SHADER TRANSLATION - %s\n", ret ? "failed" : "success");

out:
   nv_pc_free_refs(pc);

   for (i = 0; i < pc->num_blocks; ++i)
      FREE(pc->bb_list[i]);
   if (pc->root)
      FREE(pc->root);
   if (ret) {
      /* on success, these will be referenced by struct nvc0_program */
      if (pc->emit)
         FREE(pc->emit);
      if (pc->immd_buf)
         FREE(pc->immd_buf);
      if (pc->reloc_entries)
         FREE(pc->reloc_entries);
   }
   FREE(pc);
   return ret;
}

static void
nvbb_insert_phi(struct nv_basic_block *b, struct nv_instruction *i)
{
   if (!b->phi) {
      i->prev = NULL;
      b->phi = i;
      i->next = b->entry;
      if (b->entry) {
         assert(!b->entry->prev && b->exit);
         b->entry->prev = i;
      } else {
         b->entry = i;
         b->exit = i;
      }
   } else {
      assert(b->entry);
      if (b->entry->opcode == NV_OP_PHI) { /* insert after entry */
         assert(b->entry == b->exit);
         b->entry->next = i;
         i->prev = b->entry;
         b->entry = i;
         b->exit = i;
      } else { /* insert before entry */
         assert(b->entry->prev && b->exit);
         i->next = b->entry;
         i->prev = b->entry->prev;
         b->entry->prev = i;
         i->prev->next = i;
      }
   }
}

void
nvc0_insn_append(struct nv_basic_block *b, struct nv_instruction *i)
{
   if (i->opcode == NV_OP_PHI) {
      nvbb_insert_phi(b, i);
   } else {
      i->prev = b->exit;
      if (b->exit)
         b->exit->next = i;
      b->exit = i;
      if (!b->entry)
         b->entry = i;
      else
      if (i->prev && i->prev->opcode == NV_OP_PHI)
         b->entry = i;
   }

   i->bb = b;
   b->num_instructions++;

   if (i->prev && i->prev->terminator)
      nvc0_insns_permute(i->prev, i);
}

void
nvc0_insn_insert_after(struct nv_instruction *at, struct nv_instruction *ni)
{
   if (!at->next) {
      nvc0_insn_append(at->bb, ni);
      return;
   }
   ni->next = at->next;
   ni->prev = at;
   ni->next->prev = ni;
   ni->prev->next = ni;
   ni->bb = at->bb;
   ni->bb->num_instructions++;
}

void
nvc0_insn_insert_before(struct nv_instruction *at, struct nv_instruction *ni)
{
   nvc0_insn_insert_after(at, ni);
   nvc0_insns_permute(at, ni);
}

void
nvc0_insn_delete(struct nv_instruction *nvi)
{
   struct nv_basic_block *b = nvi->bb;
   int s;

   /* debug_printf("REM: "); nv_print_instruction(nvi); */

   for (s = 0; s < 6 && nvi->src[s]; ++s)
      nv_reference(NULL, nvi, s, NULL);

   if (nvi->next)
      nvi->next->prev = nvi->prev;
   else {
      assert(nvi == b->exit);
      b->exit = nvi->prev;
   }

   if (nvi->prev)
      nvi->prev->next = nvi->next;

   if (nvi == b->entry) {
      /* PHIs don't get hooked to b->entry */
      b->entry = nvi->next;
      assert(!nvi->prev || nvi->prev->opcode == NV_OP_PHI);
   }

   if (nvi == b->phi) {
      if (nvi->opcode != NV_OP_PHI)
         NV50_DBGMSG(PROG_IR, "NOTE: b->phi points to non-PHI instruction\n");

      assert(!nvi->prev);
      if (!nvi->next || nvi->next->opcode != NV_OP_PHI)
         b->phi = NULL;
      else
         b->phi = nvi->next;
   }
}

void
nvc0_insns_permute(struct nv_instruction *i1, struct nv_instruction *i2)
{
   struct nv_basic_block *b = i1->bb;

   assert(i1->opcode != NV_OP_PHI &&
          i2->opcode != NV_OP_PHI);
   assert(i1->next == i2);

   if (b->exit == i2)
      b->exit = i1;

   if (b->entry == i1)
      b->entry = i2;

   i2->prev = i1->prev;
   i1->next = i2->next;
   i2->next = i1;
   i1->prev = i2;

   if (i2->prev)
      i2->prev->next = i2;
   if (i1->next)
      i1->next->prev = i1;
}

void
nvc0_bblock_attach(struct nv_basic_block *parent,
		   struct nv_basic_block *b, ubyte edge_kind)
{
   assert(b->num_in < 8);

   if (parent->out[0]) {
      assert(!parent->out[1]);
      parent->out[1] = b;
      parent->out_kind[1] = edge_kind;
   } else {
      parent->out[0] = b;
      parent->out_kind[0] = edge_kind;
   }

   b->in[b->num_in] = parent;
   b->in_kind[b->num_in++] = edge_kind;
}

/* NOTE: all BRKs are treated as conditional, so there are 2 outgoing BBs */

boolean
nvc0_bblock_dominated_by(struct nv_basic_block *b, struct nv_basic_block *d)
{
   int j;

   if (b == d)
      return TRUE;

   for (j = 0; j < b->num_in; ++j)
      if ((b->in_kind[j] != CFG_EDGE_BACK) &&
          !nvc0_bblock_dominated_by(b->in[j], d))
         return FALSE;

   return j ? TRUE : FALSE;
}

/* check if @bf (future) can be reached from @bp (past), stop at @bt */
boolean
nvc0_bblock_reachable_by(struct nv_basic_block *bf, struct nv_basic_block *bp,
			 struct nv_basic_block *bt)
{
   struct nv_basic_block *q[NV_PC_MAX_BASIC_BLOCKS], *b;
   int i, p, n;

   p = 0;
   n = 1;
   q[0] = bp;

   while (p < n) {
      b = q[p++];

      if (b == bf)
         break;
      if (b == bt)
         continue;
      assert(n <= (1024 - 2));

      for (i = 0; i < 2; ++i) {
         if (b->out[i] && !IS_WALL_EDGE(b->out_kind[i]) && !b->out[i]->priv) {
            q[n] = b->out[i];
            q[n++]->priv = 1;
         }
      }
   }
   for (--n; n >= 0; --n)
      q[n]->priv = 0;

   return (b == bf);
}

static struct nv_basic_block *
nvbb_find_dom_frontier(struct nv_basic_block *b, struct nv_basic_block *df)
{
   struct nv_basic_block *out;
   int i;

   if (!nvc0_bblock_dominated_by(df, b)) {
      for (i = 0; i < df->num_in; ++i) {
         if (df->in_kind[i] == CFG_EDGE_BACK)
            continue;
         if (nvc0_bblock_dominated_by(df->in[i], b))
            return df;
      }
   }
   for (i = 0; i < 2 && df->out[i]; ++i) {
      if (df->out_kind[i] == CFG_EDGE_BACK)
         continue;
      if ((out = nvbb_find_dom_frontier(b, df->out[i])))
         return out;
   }
   return NULL;
}

struct nv_basic_block *
nvc0_bblock_dom_frontier(struct nv_basic_block *b)
{
   struct nv_basic_block *df;
   int i;

   for (i = 0; i < 2 && b->out[i]; ++i)
      if ((df = nvbb_find_dom_frontier(b, b->out[i])))
         return df;
   return NULL;
}
