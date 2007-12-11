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

#ifndef LLVMTGSI_H
#define LLVMTGSI_H

#if defined __cplusplus
extern "C" {
#endif

#include "pipe/p_state.h"

#ifdef MESA_LLVM

struct tgsi_token;

struct gallivm_prog;
struct gallivm_cpu_engine;
struct tgsi_interp_coef;
struct tgsi_sampler;

enum gallivm_shader_type {
   GALLIVM_VS,
   GALLIVM_FS
};

struct gallivm_prog *gallivm_from_tgsi(const struct tgsi_token *tokens, enum gallivm_shader_type type);
void gallivm_prog_delete(struct gallivm_prog *prog);
int gallivm_prog_exec(struct gallivm_prog *prog,
                      float (*inputs)[PIPE_MAX_SHADER_INPUTS][4],
                      float (*dests)[PIPE_MAX_SHADER_INPUTS][4],
                      float (*consts)[4],
                      int num_vertices,
                      int num_inputs,
                      int num_attribs);
int gallivm_fragment_shader_exec(struct gallivm_prog *prog,
                                 float x, float y,
                                 float (*dests)[PIPE_MAX_SHADER_INPUTS][4],
                                 float (*inputs)[PIPE_MAX_SHADER_INPUTS][4],
                                 float (*consts)[4],
                                 struct tgsi_sampler *samplers);
void gallivm_prog_inputs_interpolate(struct gallivm_prog *prog,
                                     float (*inputs)[PIPE_MAX_SHADER_INPUTS][4],
                                     const struct tgsi_interp_coef *coefs);
void gallivm_prog_dump(struct gallivm_prog *prog, const char *file_prefix);


struct gallivm_cpu_engine *gallivm_cpu_engine_create(struct gallivm_prog *prog);
struct gallivm_cpu_engine *gallivm_global_cpu_engine();
void gallivm_cpu_jit_compile(struct gallivm_cpu_engine *ee, struct gallivm_prog *prog);
void gallivm_cpu_engine_delete(struct gallivm_cpu_engine *ee);

#endif /* MESA_LLVM */

#if defined __cplusplus
} // extern "C"
#endif

#endif
