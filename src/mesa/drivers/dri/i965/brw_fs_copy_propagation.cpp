/*
 * Copyright © 2012 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/** @file brw_fs_copy_propagation.cpp
 *
 * Support for global copy propagation in two passes: A local pass that does
 * intra-block copy (and constant) propagation, and a global pass that uses
 * dataflow analysis on the copies available at the end of each block to re-do
 * local copy propagation with more copies available.
 *
 * See Muchnik's Advanced Compiler Design and Implementation, section
 * 12.5 (p356).
 */

#define ACP_HASH_SIZE 16

#include "main/bitset.h"
#include "brw_fs.h"
#include "brw_cfg.h"

namespace { /* avoid conflict with opt_copy_propagation_elements */
struct acp_entry : public exec_node {
   fs_reg dst;
   fs_reg src;
};

struct block_data {
   /**
    * Which entries in the fs_copy_prop_dataflow acp table are live at the
    * start of this block.  This is the useful output of the analysis, since
    * it lets us plug those into the local copy propagation on the second
    * pass.
    */
   BITSET_WORD *livein;

   /**
    * Which entries in the fs_copy_prop_dataflow acp table are live at the end
    * of this block.  This is done in initial setup from the per-block acps
    * returned by the first local copy prop pass.
    */
   BITSET_WORD *liveout;

   /**
    * Which entries in the fs_copy_prop_dataflow acp table are killed over the
    * course of this block.
    */
   BITSET_WORD *kill;
};

class fs_copy_prop_dataflow
{
public:
   fs_copy_prop_dataflow(void *mem_ctx, cfg_t *cfg,
                         exec_list *out_acp[ACP_HASH_SIZE]);

   void setup_kills();
   void run();

   void *mem_ctx;
   cfg_t *cfg;

   acp_entry **acp;
   int num_acp;
   int bitset_words;

  struct block_data *bd;
};
} /* anonymous namespace */

fs_copy_prop_dataflow::fs_copy_prop_dataflow(void *mem_ctx, cfg_t *cfg,
                                             exec_list *out_acp[ACP_HASH_SIZE])
   : mem_ctx(mem_ctx), cfg(cfg)
{
   bd = rzalloc_array(mem_ctx, struct block_data, cfg->num_blocks);

   num_acp = 0;
   for (int b = 0; b < cfg->num_blocks; b++) {
      for (int i = 0; i < ACP_HASH_SIZE; i++) {
         foreach_list(entry_node, &out_acp[b][i]) {
            num_acp++;
         }
      }
   }

   acp = rzalloc_array(mem_ctx, struct acp_entry *, num_acp);

   bitset_words = ALIGN(num_acp, BITSET_WORDBITS) / BITSET_WORDBITS;

   int next_acp = 0;
   for (int b = 0; b < cfg->num_blocks; b++) {
      bd[b].livein = rzalloc_array(bd, BITSET_WORD, bitset_words);
      bd[b].liveout = rzalloc_array(bd, BITSET_WORD, bitset_words);
      bd[b].kill = rzalloc_array(bd, BITSET_WORD, bitset_words);

      for (int i = 0; i < ACP_HASH_SIZE; i++) {
         foreach_list(entry_node, &out_acp[b][i]) {
            acp_entry *entry = (acp_entry *)entry_node;

            acp[next_acp] = entry;
            BITSET_SET(bd[b].liveout, next_acp);
            next_acp++;
         }
      }
   }

   assert(next_acp == num_acp);

   setup_kills();
   run();
}

/**
 * Walk the set of instructions in the block, marking which entries in the acp
 * are killed by the block.
 */
void
fs_copy_prop_dataflow::setup_kills()
{
   for (int b = 0; b < cfg->num_blocks; b++) {
      bblock_t *block = cfg->blocks[b];

      for (fs_inst *inst = (fs_inst *)block->start;
           inst != block->end->next;
           inst = (fs_inst *)inst->next) {
         if (inst->dst.file != GRF)
            continue;

         for (int i = 0; i < num_acp; i++) {
            if (inst->overwrites_reg(acp[i]->dst) ||
                inst->overwrites_reg(acp[i]->src)) {
               BITSET_SET(bd[b].kill, i);
            }
         }
      }
   }
}

/**
 * Walk the set of instructions in the block, marking which entries in the acp
 * are killed by the block.
 */
void
fs_copy_prop_dataflow::run()
{
   bool cont = true;

   while (cont) {
      cont = false;

      for (int b = 0; b < cfg->num_blocks; b++) {
         for (int i = 0; i < bitset_words; i++) {
            BITSET_WORD new_liveout = (bd[b].livein[i] &
                                       ~bd[b].kill[i] &
                                       ~bd[b].liveout[i]);
            if (new_liveout) {
               bd[b].liveout[i] |= new_liveout;
               cont = true;
            }

            /* Update livein: if it's live at the end of all parents, it's
             * live at our start.
             */
            BITSET_WORD new_livein = ~bd[b].livein[i];
            foreach_list(block_node, &cfg->blocks[b]->parents) {
               bblock_link *link = (bblock_link *)block_node;
               bblock_t *block = link->block;
               new_livein &= bd[block->block_num].liveout[i];
               if (!new_livein)
                  break;
            }
            if (new_livein) {
               bd[b].livein[i] |= new_livein;
               cont = true;
            }
         }
      }
   }
}

bool
fs_visitor::try_copy_propagate(fs_inst *inst, int arg, acp_entry *entry)
{
   if (entry->src.file == IMM)
      return false;

   if (inst->src[arg].file != entry->dst.file ||
       inst->src[arg].reg != entry->dst.reg ||
       inst->src[arg].reg_offset != entry->dst.reg_offset) {
      return false;
   }

   /* See resolve_ud_negate() and comment in brw_fs_emit.cpp. */
   if (inst->conditional_mod &&
       inst->src[arg].type == BRW_REGISTER_TYPE_UD &&
       entry->src.negate)
      return false;

   bool has_source_modifiers = entry->src.abs || entry->src.negate;

   if ((has_source_modifiers || entry->src.file == UNIFORM ||
        entry->src.smear != -1) && !can_do_source_mods(inst))
      return false;

   inst->src[arg].file = entry->src.file;
   inst->src[arg].reg = entry->src.reg;
   inst->src[arg].reg_offset = entry->src.reg_offset;
   if (entry->src.smear != -1)
      inst->src[arg].smear = entry->src.smear;

   if (!inst->src[arg].abs) {
      inst->src[arg].abs = entry->src.abs;
      inst->src[arg].negate ^= entry->src.negate;
   }

   return true;
}


bool
fs_visitor::try_constant_propagate(fs_inst *inst, acp_entry *entry)
{
   bool progress = false;

   if (entry->src.file != IMM)
      return false;

   for (int i = 2; i >= 0; i--) {
      if (inst->src[i].file != entry->dst.file ||
          inst->src[i].reg != entry->dst.reg ||
          inst->src[i].reg_offset != entry->dst.reg_offset)
         continue;

      /* Don't bother with cases that should have been taken care of by the
       * GLSL compiler's constant folding pass.
       */
      if (inst->src[i].negate || inst->src[i].abs)
         continue;

      switch (inst->opcode) {
      case BRW_OPCODE_MOV:
         inst->src[i] = entry->src;
         progress = true;
         break;

      case BRW_OPCODE_MACH:
      case BRW_OPCODE_MUL:
      case BRW_OPCODE_ADD:
         if (i == 1) {
            inst->src[i] = entry->src;
            progress = true;
         } else if (i == 0 && inst->src[1].file != IMM) {
            /* Fit this constant in by commuting the operands.
             * Exception: we can't do this for 32-bit integer MUL/MACH
             * because it's asymmetric.
             */
            if ((inst->opcode == BRW_OPCODE_MUL ||
                 inst->opcode == BRW_OPCODE_MACH) &&
                (inst->src[1].type == BRW_REGISTER_TYPE_D ||
                 inst->src[1].type == BRW_REGISTER_TYPE_UD))
               break;
            inst->src[0] = inst->src[1];
            inst->src[1] = entry->src;
            progress = true;
         }
         break;

      case BRW_OPCODE_CMP:
      case BRW_OPCODE_IF:
         if (i == 1) {
            inst->src[i] = entry->src;
            progress = true;
         } else if (i == 0 && inst->src[1].file != IMM) {
            uint32_t new_cmod;

            new_cmod = brw_swap_cmod(inst->conditional_mod);
            if (new_cmod != ~0u) {
               /* Fit this constant in by swapping the operands and
                * flipping the test
                */
               inst->src[0] = inst->src[1];
               inst->src[1] = entry->src;
               inst->conditional_mod = new_cmod;
               progress = true;
            }
         }
         break;

      case BRW_OPCODE_SEL:
         if (i == 1) {
            inst->src[i] = entry->src;
            progress = true;
         } else if (i == 0 && inst->src[1].file != IMM) {
            inst->src[0] = inst->src[1];
            inst->src[1] = entry->src;

            /* If this was predicated, flipping operands means
             * we also need to flip the predicate.
             */
            if (inst->conditional_mod == BRW_CONDITIONAL_NONE) {
               inst->predicate_inverse =
                  !inst->predicate_inverse;
            }
            progress = true;
         }
         break;

      case SHADER_OPCODE_RCP:
         /* The hardware doesn't do math on immediate values
          * (because why are you doing that, seriously?), but
          * the correct answer is to just constant fold it
          * anyway.
          */
         assert(i == 0);
         if (inst->src[0].imm.f != 0.0f) {
            inst->opcode = BRW_OPCODE_MOV;
            inst->src[0] = entry->src;
            inst->src[0].imm.f = 1.0f / inst->src[0].imm.f;
            progress = true;
         }
         break;

      case FS_OPCODE_UNIFORM_PULL_CONSTANT_LOAD:
         inst->src[i] = entry->src;
         progress = true;
         break;

      default:
         break;
      }
   }

   return progress;
}
/* Walks a basic block and does copy propagation on it using the acp
 * list.
 */
bool
fs_visitor::opt_copy_propagate_local(void *mem_ctx, bblock_t *block,
                                     exec_list *acp)
{
   bool progress = false;

   for (fs_inst *inst = (fs_inst *)block->start;
	inst != block->end->next;
	inst = (fs_inst *)inst->next) {

      /* Try propagating into this instruction. */
      for (int i = 0; i < 3; i++) {
         if (inst->src[i].file != GRF)
            continue;

         foreach_list(entry_node, &acp[inst->src[i].reg % ACP_HASH_SIZE]) {
            acp_entry *entry = (acp_entry *)entry_node;

            if (try_constant_propagate(inst, entry))
               progress = true;

            if (try_copy_propagate(inst, i, entry))
               progress = true;
         }
      }

      /* kill the destination from the ACP */
      if (inst->dst.file == GRF) {
	 foreach_list_safe(entry_node, &acp[inst->dst.reg % ACP_HASH_SIZE]) {
	    acp_entry *entry = (acp_entry *)entry_node;

	    if (inst->overwrites_reg(entry->dst)) {
	       entry->remove();
	    }
	 }

         /* Oops, we only have the chaining hash based on the destination, not
          * the source, so walk across the entire table.
          */
         for (int i = 0; i < ACP_HASH_SIZE; i++) {
            foreach_list_safe(entry_node, &acp[i]) {
               acp_entry *entry = (acp_entry *)entry_node;
               if (inst->overwrites_reg(entry->src))
                  entry->remove();
            }
	 }
      }

      /* If this instruction's source could potentially be folded into the
       * operand of another instruction, add it to the ACP.
       */
      if (inst->opcode == BRW_OPCODE_MOV &&
	  inst->dst.file == GRF &&
	  ((inst->src[0].file == GRF &&
	    (inst->src[0].reg != inst->dst.reg ||
	     inst->src[0].reg_offset != inst->dst.reg_offset)) ||
           inst->src[0].file == UNIFORM ||
           inst->src[0].file == IMM) &&
	  inst->src[0].type == inst->dst.type &&
	  !inst->saturate &&
	  !inst->predicate &&
	  !inst->force_uncompressed &&
	  !inst->force_sechalf) {
	 acp_entry *entry = ralloc(mem_ctx, acp_entry);
	 entry->dst = inst->dst;
	 entry->src = inst->src[0];
	 acp[entry->dst.reg % ACP_HASH_SIZE].push_tail(entry);
      }
   }

   return progress;
}

bool
fs_visitor::opt_copy_propagate()
{
   bool progress = false;
   void *mem_ctx = ralloc_context(this->mem_ctx);
   cfg_t cfg(this);
   exec_list *out_acp[cfg.num_blocks];
   for (int i = 0; i < cfg.num_blocks; i++)
      out_acp[i] = new exec_list [ACP_HASH_SIZE];

   /* First, walk through each block doing local copy propagation and getting
    * the set of copies available at the end of the block.
    */
   for (int b = 0; b < cfg.num_blocks; b++) {
      bblock_t *block = cfg.blocks[b];

      progress = opt_copy_propagate_local(mem_ctx, block,
                                          out_acp[b]) || progress;
   }

   /* Do dataflow analysis for those available copies. */
   fs_copy_prop_dataflow dataflow(mem_ctx, &cfg, out_acp);

   /* Next, re-run local copy propagation, this time with the set of copies
    * provided by the dataflow analysis available at the start of a block.
    */
   for (int b = 0; b < cfg.num_blocks; b++) {
      bblock_t *block = cfg.blocks[b];
      exec_list in_acp[ACP_HASH_SIZE];

      for (int i = 0; i < dataflow.num_acp; i++) {
         if (BITSET_TEST(dataflow.bd[b].livein, i)) {
            struct acp_entry *entry = dataflow.acp[i];
            in_acp[entry->dst.reg % ACP_HASH_SIZE].push_tail(entry);
         }
      }

      progress = opt_copy_propagate_local(mem_ctx, block, in_acp) || progress;
   }

   for (int i = 0; i < cfg.num_blocks; i++)
      delete [] out_acp[i];
   ralloc_free(mem_ctx);

   if (progress)
      live_intervals_valid = false;

   return progress;
}
