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

#include "brw_fs.h"
#include "brw_cfg.h"

/** @file brw_cfg_t.cpp
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

bblock_t::bblock_t()
{
   start = NULL;
   end = NULL;

   parents.make_empty();
   children.make_empty();
}

void
bblock_t::add_successor(void *mem_ctx, bblock_t *successor)
{
   successor->parents.push_tail(this->make_list(mem_ctx));
   children.push_tail(successor->make_list(mem_ctx));
}

bblock_link *
bblock_t::make_list(void *mem_ctx)
{
   return new(mem_ctx) bblock_link(this);
}

cfg_t::cfg_t(backend_visitor *v)
{
   create(v->mem_ctx, &v->instructions);
}

cfg_t::cfg_t(void *mem_ctx, exec_list *instructions)
{
   create(mem_ctx, instructions);
}

void
cfg_t::create(void *parent_mem_ctx, exec_list *instructions)
{
   mem_ctx = ralloc_context(parent_mem_ctx);
   block_list.make_empty();
   num_blocks = 0;
   ip = 0;
   cur = NULL;

   bblock_t *entry = new_block();
   bblock_t *cur_if = NULL, *cur_else = NULL, *cur_endif = NULL;
   bblock_t *cur_do = NULL, *cur_while = NULL;
   exec_list if_stack, else_stack, endif_stack, do_stack, while_stack;
   bblock_t *next;

   set_next_block(entry);

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
	 if_stack.push_tail(cur_if->make_list(mem_ctx));
	 else_stack.push_tail(cur_else->make_list(mem_ctx));
	 endif_stack.push_tail(cur_endif->make_list(mem_ctx));

	 cur_if = cur;
	 cur_else = NULL;
	 /* Set up the block just after the endif.  Don't know when exactly
	  * it will start, yet.
	  */
	 cur_endif = new_block();

	 /* Set up our immediately following block, full of "then"
	  * instructions.
	  */
	 next = new_block();
	 next->start = (backend_instruction *)inst->next;
	 cur_if->add_successor(mem_ctx, next);

	 set_next_block(next);
	 break;

      case BRW_OPCODE_ELSE:
	 cur->add_successor(mem_ctx, cur_endif);

	 next = new_block();
	 next->start = (backend_instruction *)inst->next;
	 cur_if->add_successor(mem_ctx, next);
	 cur_else = next;

	 set_next_block(next);
	 break;

      case BRW_OPCODE_ENDIF:
	 cur_endif->start = (backend_instruction *)inst->next;
	 cur->add_successor(mem_ctx, cur_endif);
	 set_next_block(cur_endif);

	 if (!cur_else)
	    cur_if->add_successor(mem_ctx, cur_endif);

	 /* Pop the stack so we're in the previous if/else/endif */
	 cur_if = pop_stack(&if_stack);
	 cur_else = pop_stack(&else_stack);
	 cur_endif = pop_stack(&endif_stack);
	 break;

      case BRW_OPCODE_DO:
	 /* Push our information onto a stack so we can recover from
	  * nested loops.
	  */
	 do_stack.push_tail(cur_do->make_list(mem_ctx));
	 while_stack.push_tail(cur_while->make_list(mem_ctx));

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

	 set_next_block(next);
	 break;

      case BRW_OPCODE_CONTINUE:
	 cur->add_successor(mem_ctx, cur_do);

	 next = new_block();
	 next->start = (backend_instruction *)inst->next;
	 if (inst->predicate)
	    cur->add_successor(mem_ctx, next);

	 set_next_block(next);
	 break;

      case BRW_OPCODE_BREAK:
	 cur->add_successor(mem_ctx, cur_while);

	 next = new_block();
	 next->start = (backend_instruction *)inst->next;
	 if (inst->predicate)
	    cur->add_successor(mem_ctx, next);

	 set_next_block(next);
	 break;

      case BRW_OPCODE_WHILE:
	 cur_while->start = (backend_instruction *)inst->next;

	 cur->add_successor(mem_ctx, cur_do);
	 set_next_block(cur_while);

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
cfg_t::set_next_block(bblock_t *block)
{
   if (cur) {
      assert(cur->end->next == block->start);
      cur->end_ip = ip - 1;
   }

   block->start_ip = ip;
   block->block_num = num_blocks++;
   block_list.push_tail(block->make_list(mem_ctx));
   cur = block;
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
