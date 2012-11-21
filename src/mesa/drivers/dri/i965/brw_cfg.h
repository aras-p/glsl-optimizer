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

class bblock_link : public exec_node {
public:
   bblock_link(bblock_t *block)
      : block(block)
   {
   }

   bblock_t *block;
};

class bblock_t {
public:
   static void* operator new(size_t size, void *ctx)
   {
      void *node;

      node = rzalloc_size(ctx, size);
      assert(node != NULL);

      return node;
   }

   bblock_link *make_list(void *mem_ctx);

   bblock_t();

   void add_successor(void *mem_ctx, bblock_t *successor);

   backend_instruction *start;
   backend_instruction *end;

   int start_ip;
   int end_ip;

   exec_list parents;
   exec_list children;
   int block_num;
};

class cfg_t {
public:
   static void* operator new(size_t size, void *ctx)
   {
      void *node;

      node = rzalloc_size(ctx, size);
      assert(node != NULL);

      return node;
   }

   cfg_t(backend_visitor *v);
   cfg_t(void *mem_ctx, exec_list *instructions);
   ~cfg_t();

   void create(void *mem_ctx, exec_list *instructions);

   bblock_t *new_block();
   void set_next_block(bblock_t *block);
   void make_block_array();

   /** @{
    *
    * Used while generating the block list.
    */
   bblock_t *cur;
   int ip;
   /** @} */

   void *mem_ctx;

   /** Ordered list (by ip) of basic blocks */
   exec_list block_list;
   bblock_t **blocks;
   int num_blocks;
};
