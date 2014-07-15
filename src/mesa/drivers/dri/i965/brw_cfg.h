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

#ifndef __cplusplus
struct backend_instruction;
#endif

struct bblock_t {
#ifdef __cplusplus
   DECLARE_RALLOC_CXX_OPERATORS(bblock_t)

   bblock_t();

   void add_successor(void *mem_ctx, bblock_t *successor);
   void dump(backend_visitor *v);
#endif

   struct exec_node link;

   struct backend_instruction *start;
   struct backend_instruction *end;

   int start_ip;
   int end_ip;

   struct exec_list parents;
   struct exec_list children;
   int num;

   /* If the current basic block ends in an IF, ELSE, or ENDIF instruction,
    * these pointers will hold the locations of the other associated control
    * flow instructions.
    *
    * Otherwise they are NULL.
    */
   struct backend_instruction *if_inst;
   struct backend_instruction *else_inst;
   struct backend_instruction *endif_inst;
};

struct cfg_t {
#ifdef __cplusplus
   DECLARE_RALLOC_CXX_OPERATORS(cfg_t)

   cfg_t(exec_list *instructions);
   ~cfg_t();

   bblock_t *new_block();
   void set_next_block(bblock_t **cur, bblock_t *block, int ip);
   void make_block_array();

   void dump(backend_visitor *v);
#endif
   void *mem_ctx;

   /** Ordered list (by ip) of basic blocks */
   struct exec_list block_list;
   struct bblock_t **blocks;
   int num_blocks;
};

#define foreach_block_and_inst(__block, __type, __inst, __cfg) \
   foreach_block (__block, __cfg)                              \
      foreach_inst_in_block (__type, __inst, __block)

#define foreach_block_and_inst_safe(__block, __type, __inst, __cfg) \
   foreach_block_safe (__block, __cfg)                              \
      foreach_inst_in_block_safe (__type, __inst, __block)

#define foreach_block(__block, __cfg)                          \
   foreach_list_typed (bblock_t, __block, link, &(__cfg)->block_list)

#define foreach_block_safe(__block, __cfg)                     \
   foreach_list_typed_safe (bblock_t, __block, link, &(__cfg)->block_list)

#define foreach_inst_in_block(__type, __inst, __block)         \
   for (__type *__inst = (__type *)__block->start;             \
        __inst != __block->end->next;                          \
        __inst = (__type *)__inst->next)

#define foreach_inst_in_block_safe(__type, __inst, __block)    \
   for (__type *__inst = (__type *)__block->start,             \
               *__next = (__type *)__inst->next,               \
               *__end = (__type *)__block->end->next->next;    \
        __next != __end;                                       \
        __inst = __next,                                       \
        __next = (__type *)__next->next)

#define foreach_inst_in_block_reverse(__type, __inst, __block) \
   for (__type *__inst = (__type *)__block->end;               \
        __inst != __block->start->prev;                        \
        __inst = (__type *)__inst->prev)

#endif /* BRW_CFG_H */
