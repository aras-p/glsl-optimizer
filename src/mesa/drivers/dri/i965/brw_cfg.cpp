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
 *
 * Authors:
 *    Eric Anholt <eric@anholt.net>
 *
 */

#include "brw_cfg.h"

/** @file brw_cfg.cpp
 *
 * Walks the shader instructions generated and creates a set of basic
 * blocks with successor/predecessor edges connecting them.
 */

static bblock_t *
pop_stack(exec_list *list)
{
   bblock_link *link = (bblock_link *)list->get_tail();
   bblock_t *block = link->block;
   link->remove();

   return block;
}

bblock_t::bblock_t() :
   start_ip(0), end_ip(0), block_num(0)
{
   start = NULL;
   end = NULL;

   parents.make_empty();
   children.make_empty();

   if_inst = NULL;
   else_inst = NULL;
   endif_inst = NULL;
}

void
bblock_t::add_successor(void *mem_ctx, bblock_t *successor)
{
   successor->parents.push_tail(new(mem_ctx) bblock_link(this));
   children.push_tail(new(mem_ctx) bblock_link(successor));
}

void
bblock_t::dump(backend_visitor *v)
{
   int ip = this->start_ip;
   for (backend_instruction *inst = (backend_instruction *)this->start;
	inst != this->end->next;
	inst = (backend_instruction *) inst->next) {
      fprintf(stderr, "%5d: ", ip);
      v->dump_instruction(inst);
      ip++;
   }
}

cfg_t::cfg_t(exec_list *instructions)
{
   mem_ctx = ralloc_context(NULL);
   block_list.make_empty();
   blocks = NULL;
   num_blocks = 0;

   bblock_t *cur = NULL;
   int ip = 0;

   bblock_t *entry = new_block();
   bblock_t *cur_if = NULL;    /**< BB ending with IF. */
   bblock_t *cur_else = NULL;  /**< BB ending with ELSE. */
   bblock_t *cur_endif = NULL; /**< BB starting with ENDIF. */
   bblock_t *cur_do = NULL;    /**< BB ending with DO. */
   bblock_t *cur_while = NULL; /**< BB immediately following WHILE. */
   exec_list if_stack, else_stack, do_stack, while_stack;
   bblock_t *next;

   set_next_block(&cur, entry, ip);

   entry->start = (backend_instruction *) instructions->get_head();

   foreach_list(node, instructions) {
      backend_instruction *inst = (backend_instruction *)node;

      cur->end = inst;

      /* set_next_block wants the post-incremented ip */
      ip++;

      switch (inst->opcode) {
      case BRW_OPCODE_IF:
	 /* Push our information onto a stack so we can recover from
	  * nested ifs.
	  */
	 if_stack.push_tail(new(mem_ctx) bblock_link(cur_if));
	 else_stack.push_tail(new(mem_ctx) bblock_link(cur_else));

	 cur_if = cur;
	 cur_else = NULL;
         cur_endif = NULL;

	 /* Set up our immediately following block, full of "then"
	  * instructions.
	  */
	 next = new_block();
	 next->start = (backend_instruction *)inst->next;
	 cur_if->add_successor(mem_ctx, next);

	 set_next_block(&cur, next, ip);
	 break;

      case BRW_OPCODE_ELSE:
         cur_else = cur;

	 next = new_block();
	 next->start = (backend_instruction *)inst->next;
	 cur_if->add_successor(mem_ctx, next);

	 set_next_block(&cur, next, ip);
	 break;

      case BRW_OPCODE_ENDIF: {
         if (cur->start == inst) {
            /* New block was just created; use it. */
            cur_endif = cur;
         } else {
            cur_endif = new_block();
            cur_endif->start = inst;

            cur->end = (backend_instruction *)inst->prev;
            cur->add_successor(mem_ctx, cur_endif);

            set_next_block(&cur, cur_endif, ip - 1);
         }

         backend_instruction *else_inst = NULL;
         if (cur_else) {
            else_inst = (backend_instruction *)cur_else->end;

            cur_else->add_successor(mem_ctx, cur_endif);
         } else {
            cur_if->add_successor(mem_ctx, cur_endif);
         }

         assert(cur_if->end->opcode == BRW_OPCODE_IF);
         assert(!else_inst || else_inst->opcode == BRW_OPCODE_ELSE);
         assert(inst->opcode == BRW_OPCODE_ENDIF);

         cur_if->if_inst = cur_if->end;
         cur_if->else_inst = else_inst;
         cur_if->endif_inst = inst;

	 if (cur_else) {
            cur_else->if_inst = cur_if->end;
            cur_else->else_inst = else_inst;
            cur_else->endif_inst = inst;
         }

         cur->if_inst = cur_if->end;
         cur->else_inst = else_inst;
         cur->endif_inst = inst;

	 /* Pop the stack so we're in the previous if/else/endif */
	 cur_if = pop_stack(&if_stack);
	 cur_else = pop_stack(&else_stack);
	 break;
      }
      case BRW_OPCODE_DO:
	 /* Push our information onto a stack so we can recover from
	  * nested loops.
	  */
	 do_stack.push_tail(new(mem_ctx) bblock_link(cur_do));
	 while_stack.push_tail(new(mem_ctx) bblock_link(cur_while));

	 /* Set up the block just after the while.  Don't know when exactly
	  * it will start, yet.
	  */
	 cur_while = new_block();

	 /* Set up our immediately following block, full of "then"
	  * instructions.
	  */
	 next = new_block();
	 next->start = (backend_instruction *)inst->next;
	 cur->add_successor(mem_ctx, next);
	 cur_do = next;

	 set_next_block(&cur, next, ip);
	 break;

      case BRW_OPCODE_CONTINUE:
	 cur->add_successor(mem_ctx, cur_do);

	 next = new_block();
	 next->start = (backend_instruction *)inst->next;
	 if (inst->predicate)
	    cur->add_successor(mem_ctx, next);

	 set_next_block(&cur, next, ip);
	 break;

      case BRW_OPCODE_BREAK:
	 cur->add_successor(mem_ctx, cur_while);

	 next = new_block();
	 next->start = (backend_instruction *)inst->next;
	 if (inst->predicate)
	    cur->add_successor(mem_ctx, next);

	 set_next_block(&cur, next, ip);
	 break;

      case BRW_OPCODE_WHILE:
	 cur_while->start = (backend_instruction *)inst->next;

	 cur->add_successor(mem_ctx, cur_do);
	 set_next_block(&cur, cur_while, ip);

	 /* Pop the stack so we're in the previous loop */
	 cur_do = pop_stack(&do_stack);
	 cur_while = pop_stack(&while_stack);
	 break;

      default:
	 break;
      }
   }

   cur->end_ip = ip;

   make_block_array();
}

cfg_t::~cfg_t()
{
   ralloc_free(mem_ctx);
}

bblock_t *
cfg_t::new_block()
{
   bblock_t *block = new(mem_ctx) bblock_t();

   return block;
}

void
cfg_t::set_next_block(bblock_t **cur, bblock_t *block, int ip)
{
   if (*cur) {
      assert((*cur)->end->next == block->start);
      (*cur)->end_ip = ip - 1;
   }

   block->start_ip = ip;
   block->block_num = num_blocks++;
   block_list.push_tail(new(mem_ctx) bblock_link(block));
   *cur = block;
}

void
cfg_t::make_block_array()
{
   blocks = ralloc_array(mem_ctx, bblock_t *, num_blocks);

   int i = 0;
   foreach_list(block_node, &block_list) {
      bblock_link *link = (bblock_link *)block_node;
      blocks[i++] = link->block;
   }
   assert(i == num_blocks);
}

void
cfg_t::dump(backend_visitor *v)
{
   for (int b = 0; b < this->num_blocks; b++) {
        bblock_t *block = this->blocks[b];
      fprintf(stderr, "START B%d", b);
      foreach_list(node, &block->parents) {
         bblock_link *link = (bblock_link *)node;
         fprintf(stderr, " <-B%d",
                 link->block->block_num);
      }
      fprintf(stderr, "\n");
      block->dump(v);
      fprintf(stderr, "END B%d", b);
      foreach_list(node, &block->children) {
         bblock_link *link = (bblock_link *)node;
         fprintf(stderr, " ->B%d",
                 link->block->block_num);
      }
      fprintf(stderr, "\n");
   }
}
