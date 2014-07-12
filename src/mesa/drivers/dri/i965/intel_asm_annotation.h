/*
 * Copyright © 2014 Intel Corporation
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

#ifndef _INTEL_ASM_ANNOTATION_H
#define _INTEL_ASM_ANNOTATION_H

#ifdef __cplusplus
extern "C" {
#endif

struct bblock_t;
struct brw_context;
struct gl_program;
struct backend_instruction;
struct cfg_t;

struct annotation {
   int offset;

   /* Pointers to the basic block in the CFG if the instruction group starts
    * or ends a basic block.
    */
   struct bblock_t *block_start;
   struct bblock_t *block_end;

   /* Annotation for the generated IR.  One of the two can be set. */
   const void *ir;
   const char *annotation;
};

struct annotation_info {
   void *mem_ctx;
   struct annotation *ann;
   int ann_count;
   int ann_size;

   /** Block index in the cfg. */
   int cur_block;
};

void
dump_assembly(void *assembly, int num_annotations, struct annotation *annotation,
              struct brw_context *brw, const struct gl_program *prog);

void
annotate(struct brw_context *brw,
         struct annotation_info *annotation, const struct cfg_t *cfg,
         struct backend_instruction *inst, unsigned offset);
void
annotation_finalize(struct annotation_info *annotation, unsigned offset);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _INTEL_ASM_ANNOTATION_H */
