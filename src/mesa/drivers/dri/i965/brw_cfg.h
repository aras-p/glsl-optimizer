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

#pragma once
#ifndef BRW_CFG_H
#define BRW_CFG_H

#include "brw_shader.h"

struct bblock_t;

struct bblock_link {
#ifdef __cplusplus
   DECLARE_RALLOC_CXX_OPERATORS(bblock_link)

   bblock_link(bblock_t *block)
      : block(block)
   {
   }
#endif

   struct exec_node link;
   struct bblock_t *block;
};

struct backend_instruction;

struct bblock_t {
#ifdef __cplusplus
   DECLARE_RALLOC_CXX_OPERATORS(bblock_t)

   explicit bblock_t(cfg_t *cfg);

   void add_successor(void *mem_ctx, bblock_t *successor);
   bool is_predecessor_of(const bblock_t *block) const;
   bool is_successor_of(const bblock_t *block) const;
   bool can_combine_with(const bblock_t *that) const;
   void combine_with(bblock_t *that);
   void dump(backend_visitor *v) const;

   backend_instruction *start();
   const backend_instruction *start() const;
   backend_instruction *end();
   const backend_instruction *end() const;
#endif

   struct exec_node link;
   struct cfg_t *cfg;

   int start_ip;
   int end_ip;

   struct exec_list instructions;
   struct exec_list parents;
   struct exec_list children;
   int num;

   /* If the current basic block ends in an IF or ELSE instruction, these will
    * point to the basic blocks containing the other associated instruction.
    *
    * Otherwise they are NULL.
    */
   struct bblock_t *if_block;
   struct bblock_t *else_block;
};

static inline struct backend_instruction *
bblock_start(struct bblock_t *block)
{
   return (struct backend_instruction *)exec_list_get_head(&block->instructions);
}

static inline const struct backend_instruction *
bblock_start_const(const struct bblock_t *block)
{
   return (const struct backend_instruction *)exec_list_get_head_const(&block->instructions);
}

static inline struct backend_instruction *
bblock_end(struct bblock_t *block)
{
   return (struct backend_instruction *)exec_list_get_tail(&block->instructions);
}

static inline const struct backend_instruction *
bblock_end_const(const struct bblock_t *block)
{
   return (const struct backend_instruction *)exec_list_get_tail_const(&block->instructions);
}

#ifdef __cplusplus
inline backend_instruction *
bblock_t::start()
{
   return bblock_start(this);
}

inline const backend_instruction *
bblock_t::start() const
{
   return bblock_start_const(this);
}

inline backend_instruction *
bblock_t::end()
{
   return bblock_end(this);
}

inline const backend_instruction *
bblock_t::end() const
{
   return bblock_end_const(this);
}
#endif

struct cfg_t {
#ifdef __cplusplus
   DECLARE_RALLOC_CXX_OPERATORS(cfg_t)

   cfg_t(exec_list *instructions);
   ~cfg_t();

   void remove_block(bblock_t *block);

   bblock_t *new_block();
   void set_next_block(bblock_t **cur, bblock_t *block, int ip);
   void make_block_array();

   void dump(backend_visitor *v) const;
#endif
   void *mem_ctx;

   /** Ordered list (by ip) of basic blocks */
   struct exec_list block_list;
   struct bblock_t **blocks;
   int num_blocks;
};

/* Note that this is implemented with a double for loop -- break will
 * break from the inner loop only!
 */
#define foreach_block_and_inst(__block, __type, __inst, __cfg) \
   foreach_block (__block, __cfg)                              \
      foreach_inst_in_block (__type, __inst, __block)

/* Note that this is implemented with a double for loop -- break will
 * break from the inner loop only!
 */
#define foreach_block_and_inst_safe(__block, __type, __inst, __cfg) \
   foreach_block_safe (__block, __cfg)                              \
      foreach_inst_in_block_safe (__type, __inst, __block)

#define foreach_block(__block, __cfg)                          \
   foreach_list_typed (bblock_t, __block, link, &(__cfg)->block_list)

#define foreach_block_safe(__block, __cfg)                     \
   foreach_list_typed_safe (bblock_t, __block, link, &(__cfg)->block_list)

#define foreach_inst_in_block(__type, __inst, __block)         \
   foreach_in_list(__type, __inst, &(__block)->instructions)

#define foreach_inst_in_block_safe(__type, __inst, __block)    \
   for (__type *__inst = (__type *)__block->instructions.head, \
               *__next = (__type *)__inst->next,               \
               *__end = (__type *)__block->instructions.tail;  \
        __next != __end;                                       \
        __inst = __next,                                       \
        __next = (__type *)__next->next)

#define foreach_inst_in_block_reverse(__type, __inst, __block) \
   foreach_in_list_reverse(__type, __inst, &(__block)->instructions)

#define foreach_inst_in_block_starting_from(__type, __scan_inst, __inst, __block) \
   for (__type *__scan_inst = (__type *)__inst->next;          \
        !__scan_inst->is_tail_sentinel();                      \
        __scan_inst = (__type *)__scan_inst->next)

#define foreach_inst_in_block_reverse_starting_from(__type, __scan_inst, __inst, __block) \
   for (__type *__scan_inst = (__type *)__inst->prev;          \
        !__scan_inst->is_head_sentinel();                      \
        __scan_inst = (__type *)__scan_inst->prev)

#endif /* BRW_CFG_H */
