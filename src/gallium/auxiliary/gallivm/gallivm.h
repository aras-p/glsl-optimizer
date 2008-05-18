/**************************************************************************
 *
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

 /*
  * Authors:
  *   Zack Rusin zack@tungstengraphics.com
  */

#ifndef GALLIVM_H
#define GALLIVM_H

/*
  LLVM representation consists of two stages - layout independent
  intermediate representation gallivm_ir and driver specific
  gallivm_prog. TGSI is first being translated into gallivm_ir
  after that driver can set number of options on gallivm_ir and
  have it compiled into gallivm_prog. gallivm_prog can be either
  executed (assuming there's LLVM JIT backend for the current
  target) or machine code generation can be done (assuming there's
  a LLVM code generator for thecurrent target)
 */
#if defined __cplusplus
extern "C" {
#endif

#include "pipe/p_state.h"

#ifdef MESA_LLVM

struct tgsi_token;

struct gallivm_ir;
struct gallivm_prog;
struct gallivm_cpu_engine;
struct tgsi_interp_coef;
struct tgsi_sampler;
struct tgsi_exec_vector;

enum gallivm_shader_type {
   GALLIVM_VS,
   GALLIVM_FS
};

enum gallivm_vector_layout {
   GALLIVM_AOS,
   GALLIVM_SOA
};

struct gallivm_ir *gallivm_ir_new(enum gallivm_shader_type type);
void               gallivm_ir_set_layout(struct gallivm_ir *ir,
                                         enum gallivm_vector_layout layout);
void               gallivm_ir_set_components(struct gallivm_ir *ir, int num);
void               gallivm_ir_fill_from_tgsi(struct gallivm_ir *ir,
                                             const struct tgsi_token *tokens);
void               gallivm_ir_delete(struct gallivm_ir *ir);


struct gallivm_prog *gallivm_ir_compile(struct gallivm_ir *ir);

void gallivm_prog_inputs_interpolate(struct gallivm_prog *prog,
                                     float (*inputs)[PIPE_MAX_SHADER_INPUTS][4],
                                     const struct tgsi_interp_coef *coefs);
void gallivm_prog_dump(struct gallivm_prog *prog, const char *file_prefix);


struct gallivm_cpu_engine *gallivm_cpu_engine_create(struct gallivm_prog *prog);
struct gallivm_cpu_engine *gallivm_global_cpu_engine();
int gallivm_cpu_vs_exec(struct gallivm_prog *prog,
                        struct tgsi_exec_machine *machine,
                        const float (*input)[4],
                        unsigned num_inputs,
                        float (*output)[4],
                        unsigned num_outputs,
                        const float (*constants)[4],
                        unsigned count,
                        unsigned input_stride,
                        unsigned output_stride);
int gallivm_cpu_fs_exec(struct gallivm_prog *prog,
                        float x, float y,
                        float (*dests)[PIPE_MAX_SHADER_INPUTS][4],
                        float (*inputs)[PIPE_MAX_SHADER_INPUTS][4],
                        float (*consts)[4],
                        struct tgsi_sampler *samplers);
void gallivm_cpu_jit_compile(struct gallivm_cpu_engine *ee, struct gallivm_prog *prog);
void gallivm_cpu_engine_delete(struct gallivm_cpu_engine *ee);


#endif /* MESA_LLVM */

#if defined __cplusplus
}
#endif

#endif
