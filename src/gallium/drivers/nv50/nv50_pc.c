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
#include "nv50_program.h"

#include <stdio.h>

/* returns TRUE if operands 0 and 1 can be swapped */
boolean
nv_op_commutative(uint opcode)
{
   switch (opcode) {
   case NV_OP_ADD:
   case NV_OP_MUL:
   case NV_OP_MAD:
   case NV_OP_AND:
   case NV_OP_OR:
   case NV_OP_XOR:
   case NV_OP_MIN:
   case NV_OP_MAX:
   case NV_OP_SAD:
     return TRUE;
   default:
     return FALSE;
   }
}

/* return operand to which the address register applies */
int
nv50_indirect_opnd(struct nv_instruction *i)
{
   if (!i->src[4])
      return -1;

   switch (i->opcode) {
   case NV_OP_MOV:
   case NV_OP_LDA:
   case NV_OP_STA:
      return 0;
   default:
      return 1;
   }
}

boolean
nv50_nvi_can_use_imm(struct nv_instruction *nvi, int s)
{
   if (nvi->flags_src || nvi->flags_def)
      return FALSE;

   switch (nvi->opcode) {
   case NV_OP_ADD:
   case NV_OP_MUL:
   case NV_OP_AND:
   case NV_OP_OR:
   case NV_OP_XOR:
   case NV_OP_SHL:
   case NV_OP_SHR:
      return (s == 1) && (nvi->src[0]->value->reg.file == NV_FILE_GPR) &&
         (nvi->def[0]->reg.file == NV_FILE_GPR);
   case NV_OP_MOV:
      assert(s == 0);
      return (nvi->def[0]->reg.file == NV_FILE_GPR);
   default:
      return FALSE;
   }
}

boolean
nv50_nvi_can_load(struct nv_instruction *nvi, int s, struct nv_value *value)
{
   int i;

   for (i = 0; i < 3 && nvi->src[i]; ++i)
      if (nvi->src[i]->value->reg.file == NV_FILE_IMM)
         return FALSE;

   switch (nvi->opcode) {
   case NV_OP_ABS:
   case NV_OP_ADD:
   case NV_OP_CEIL:
   case NV_OP_FLOOR:
   case NV_OP_TRUNC:
   case NV_OP_CVT:
   case NV_OP_ROUND:
   case NV_OP_NEG:
   case NV_OP_MAD:
   case NV_OP_MUL:
   case NV_OP_SAT:
   case NV_OP_SUB:
   case NV_OP_MAX:
   case NV_OP_MIN:
      if (s == 0 && (value->reg.file == NV_FILE_MEM_S ||
                     value->reg.file == NV_FILE_MEM_P))
         return TRUE;
      if (value->reg.file < NV_FILE_MEM_C(0) ||
          value->reg.file > NV_FILE_MEM_C(15))
         return FALSE;
      return (s == 1) ||
         ((s == 2) && (nvi->src[1]->value->reg.file == NV_FILE_GPR));
   case NV_OP_MOV:
      assert(s == 0);
      return /* TRUE */ FALSE; /* don't turn MOVs into loads */
   default:
      return FALSE;
   }
}

/* Return whether this instruction can be executed conditionally. */
boolean
nv50_nvi_can_predicate(struct nv_instruction *nvi)
{
   int i;

   if (nvi->flags_src)
      return FALSE;
   for (i = 0; i < 4 && nvi->src[i]; ++i)
      if (nvi->src[i]->value->reg.file == NV_FILE_IMM)
         return FALSE;
   return TRUE;
}

ubyte
nv50_supported_src_mods(uint opcode, int s)
{
   switch (opcode) {
   case NV_OP_ABS:
      return NV_MOD_NEG | NV_MOD_ABS; /* obviously */
   case NV_OP_ADD:
   case NV_OP_MUL:
   case NV_OP_MAD:
      return NV_MOD_NEG;
   case NV_OP_DFDX:
   case NV_OP_DFDY:
      assert(s == 0);
      return NV_MOD_NEG;
   case NV_OP_MAX:
   case NV_OP_MIN:
      return NV_MOD_ABS;
   case NV_OP_CVT:
   case NV_OP_LG2:
   case NV_OP_NEG:
   case NV_OP_PREEX2:
   case NV_OP_PRESIN:
   case NV_OP_RCP:
   case NV_OP_RSQ:
      return NV_MOD_ABS | NV_MOD_NEG;
   default:
      return 0;
   }
}

/* We may want an opcode table. */
boolean
nv50_op_can_write_flags(uint opcode)
{
   if (nv_is_vector_op(opcode))
      return FALSE;
   switch (opcode) { /* obvious ones like KIL, CALL, etc. not included */
   case NV_OP_PHI:
   case NV_OP_MOV:
   case NV_OP_SELECT:
   case NV_OP_LINTERP:
   case NV_OP_PINTERP:
   case NV_OP_LDA:
      return FALSE;
   default:
      break;
   }
   if (opcode >= NV_OP_RCP && opcode <= NV_OP_PREEX2)
      return FALSE;
   return TRUE;
}

int
nv_nvi_refcount(struct nv_instruction *nvi)
{
   int i, rc;

   rc = nvi->flags_def ? nvi->flags_def->refc : 0;

   for (i = 0; i < 4; ++i) {
      if (!nvi->def[i])
         return rc;
      rc += nvi->def[i]->refc;
   }
   return rc;
}

int
nvcg_replace_value(struct nv_pc *pc, struct nv_value *old_val,
                   struct nv_value *new_val)
{
   int i, n;

   if (old_val == new_val)
      return old_val->refc;

   for (i = 0, n = 0; i < pc->num_refs; ++i) {
      if (pc->refs[i]->value == old_val) {
         ++n;
         nv_reference(pc, &pc->refs[i], new_val);
      }
   }
   return n;
}

struct nv_value *
nvcg_find_constant(struct nv_ref *ref)
{
   struct nv_value *src;

   if (!ref)
      return NULL;

   src = ref->value;
   while (src->insn && src->insn->opcode == NV_OP_MOV) {
      assert(!src->insn->src[0]->mod);
      src = src->insn->src[0]->value;
   }
   if ((src->reg.file == NV_FILE_IMM) ||
       (src->insn && src->insn->opcode == NV_OP_LDA &&
        src->insn->src[0]->value->reg.file >= NV_FILE_MEM_C(0) &&
        src->insn->src[0]->value->reg.file <= NV_FILE_MEM_C(15)))
      return src;
   return NULL;
}

struct nv_value *
nvcg_find_immediate(struct nv_ref *ref)
{
   struct nv_value *src = nvcg_find_constant(ref);

   return (src && src->reg.file == NV_FILE_IMM) ? src : NULL;
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
nv_pc_pass_in_order(struct nv_basic_block *root, nv_pc_pass_func f, void *priv)
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
      nv_print_instruction(i);
}

void
nv_print_function(struct nv_basic_block *root)
{
   if (root->subroutine)
      debug_printf("SUBROUTINE %i\n", root->subroutine);
   else
      debug_printf("MAIN\n");

   nv_pc_pass_in_order(root, nv_do_print_function, root);
}

void
nv_print_program(struct nv_pc *pc)
{
   int i;
   for (i = 0; i < pc->num_subroutines + 1; ++i)
      if (pc->root[i])
         nv_print_function(pc->root[i]);
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
#endif /* NV50_DEBUG_PROG_CFLOW */

static INLINE void
nvcg_show_bincode(struct nv_pc *pc)
{
   unsigned i;

   for (i = 0; i < pc->bin_size / 4; ++i) {
      debug_printf("0x%08x ", pc->emit[i]);
      if ((i % 16) == 15)
         debug_printf("\n");
   }
   debug_printf("\n");
}

static int
nv50_emit_program(struct nv_pc *pc)
{
   uint32_t *code = pc->emit;
   int n;

   NV50_DBGMSG(SHADER, "emitting program: size = %u\n", pc->bin_size);

   for (n = 0; n < pc->num_blocks; ++n) {
      struct nv_instruction *i;
      struct nv_basic_block *b = pc->bb_list[n];

      for (i = b->entry; i; i = i->next) {
         nv50_emit_instruction(pc, i);

         pc->bin_pos += 1 + (pc->emit[0] & 1);
         pc->emit += 1 + (pc->emit[0] & 1);
      }
   }
   assert(pc->emit == &code[pc->bin_size / 4]);

   /* XXX: we can do better than this ... */
   if (!pc->bin_size ||
       !(pc->emit[-2] & 1) || (pc->emit[-2] & 2) || (pc->emit[-1] & 3)) {
      pc->emit[0] = 0xf0000001;
      pc->emit[1] = 0xe0000000;
      pc->bin_size += 8;
   }

   pc->emit = code;
   code[pc->bin_size / 4 - 1] |= 1;

#if NV50_DEBUG & NV50_DEBUG_SHADER
   nvcg_show_bincode(pc);
#endif

   return 0;
}

int
nv50_generate_code(struct nv50_translation_info *ti)
{
   struct nv_pc *pc;
   int ret;
   int i;

   pc = CALLOC_STRUCT(nv_pc);
   if (!pc)
      return 1;

   pc->root = CALLOC(ti->subr_nr + 1, sizeof(pc->root[0]));
   if (!pc->root) {
      FREE(pc);
      return 1;
   }
   pc->num_subroutines = ti->subr_nr;

   ret = nv50_tgsi_to_nc(pc, ti);
   if (ret)
      goto out;
#if NV50_DEBUG & NV50_DEBUG_PROG_IR
   nv_print_program(pc);
#endif

   pc->opt_reload_elim = ti->store_to_memory ? FALSE : TRUE;

   /* optimization */
   ret = nv_pc_exec_pass0(pc);
   if (ret)
      goto out;
#if NV50_DEBUG & NV50_DEBUG_PROG_IR
   nv_print_program(pc);
#endif

   /* register allocation */
   ret = nv_pc_exec_pass1(pc);
   if (ret)
      goto out;
#if NV50_DEBUG & NV50_DEBUG_PROG_CFLOW
   nv_print_program(pc);
   nv_print_cfgraph(pc, "nv50_shader_cfgraph.dot", 0);
#endif

   /* prepare for emission */
   ret = nv_pc_exec_pass2(pc);
   if (ret)
      goto out;
   assert(!(pc->bin_size % 8));

   pc->emit = CALLOC(pc->bin_size / 4 + 2, 4);
   if (!pc->emit) {
      ret = 3;
      goto out;
   }
   ret = nv50_emit_program(pc);
   if (ret)
      goto out;

   ti->p->code_size = pc->bin_size;
   ti->p->code = pc->emit;

   ti->p->immd_size = pc->immd_count * 4;
   ti->p->immd = pc->immd_buf;

   /* highest 16 bit reg to num of 32 bit regs, limit to >= 4 */
   ti->p->max_gpr = MAX2(4, (pc->max_reg[NV_FILE_GPR] >> 1) + 1);

   ti->p->fixups = pc->fixups;
   ti->p->num_fixups = pc->num_fixups;

   ti->p->uses_lmem = ti->store_to_memory;

   NV50_DBGMSG(SHADER, "SHADER TRANSLATION - %s\n", ret ? "failed" : "success");

out:
   nv_pc_free_refs(pc);

   for (i = 0; i < pc->num_blocks; ++i)
      FREE(pc->bb_list[i]);
   if (pc->root)
      FREE(pc->root);
   if (ret) { /* on success, these will be referenced by nv50_program */
      if (pc->emit)
         FREE(pc->emit);
      if (pc->immd_buf)
         FREE(pc->immd_buf);
      if (pc->fixups)
         FREE(pc->fixups);
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
nvbb_insert_tail(struct nv_basic_block *b, struct nv_instruction *i)
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

   if (i->prev && i->prev->is_terminator)
      nv_nvi_permute(i->prev, i);
}

void
nvi_insert_after(struct nv_instruction *at, struct nv_instruction *ni)
{
   if (!at->next) {
      nvbb_insert_tail(at->bb, ni);
      return;
   }
   ni->next = at->next;
   ni->prev = at;
   ni->next->prev = ni;
   ni->prev->next = ni;
}

void
nv_nvi_delete(struct nv_instruction *nvi)
{
   struct nv_basic_block *b = nvi->bb;
   int j;

   /* debug_printf("REM: "); nv_print_instruction(nvi); */

   for (j = 0; j < 5; ++j)
      nv_reference(NULL, &nvi->src[j], NULL);
   nv_reference(NULL, &nvi->flags_src, NULL);

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
nv_nvi_permute(struct nv_instruction *i1, struct nv_instruction *i2)
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
nvbb_attach_block(struct nv_basic_block *parent,
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
nvbb_dominated_by(struct nv_basic_block *b, struct nv_basic_block *d)
{
   int j;

   if (b == d)
      return TRUE;

   for (j = 0; j < b->num_in; ++j)
      if ((b->in_kind[j] != CFG_EDGE_BACK) && !nvbb_dominated_by(b->in[j], d))
         return FALSE;

   return j ? TRUE : FALSE;
}

/* check if @bf (future) can be reached from @bp (past), stop at @bt */
boolean
nvbb_reachable_by(struct nv_basic_block *bf, struct nv_basic_block *bp,
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

   if (!nvbb_dominated_by(df, b)) {
      for (i = 0; i < df->num_in; ++i) {
         if (df->in_kind[i] == CFG_EDGE_BACK)
            continue;
         if (nvbb_dominated_by(df->in[i], b))
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
nvbb_dom_frontier(struct nv_basic_block *b)
{
   struct nv_basic_block *df;
   int i;

   for (i = 0; i < 2 && b->out[i]; ++i)
      if ((df = nvbb_find_dom_frontier(b, b->out[i])))
         return df;
   return NULL;
}
