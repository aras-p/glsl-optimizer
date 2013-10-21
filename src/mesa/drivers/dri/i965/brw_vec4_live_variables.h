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

#include "main/bitset.h"
#include "brw_vec4.h"

namespace brw {

struct block_data {
   /**
    * Which variables are defined before being used in the block.
    *
    * Note that for our purposes, "defined" means unconditionally, completely
    * defined.
    */
   BITSET_WORD *def;

   /**
    * Which variables are used before being defined in the block.
    */
   BITSET_WORD *use;

   /** Which defs reach the entry point of the block. */
   BITSET_WORD *livein;

   /** Which defs reach the exit point of the block. */
   BITSET_WORD *liveout;
};

class vec4_live_variables {
public:
   DECLARE_RALLOC_CXX_OPERATORS(vec4_live_variables)

   vec4_live_variables(vec4_visitor *v, cfg_t *cfg);
   ~vec4_live_variables();

   void setup_def_use();
   void compute_live_variables();

   vec4_visitor *v;
   cfg_t *cfg;
   void *mem_ctx;

   int num_vars;
   int bitset_words;

   /** Per-basic-block information on live variables */
   struct block_data *bd;
};

} /* namespace brw */
