/*
 * Copyright © 2010 Intel Corporation
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file ir_optimization.h
 *
 * Prototypes for optimization passes to be called by the compiler and drivers.
 */

#ifndef GLSL_IR_OPTIMIZATION_H
#define GLSL_IR_OPTIMIZATION_H

struct gl_shader;
struct gl_linked_shader;
struct gl_shader_program;

bool do_common_optimization(ir_exec_list *ir, bool linked, mesa_shader_stage stage,
                            const struct pipe_screen *screen);

bool do_rebalance_tree(ir_exec_list *instructions);
bool do_algebraic(ir_exec_list *instructions);
bool do_dead_code(ir_exec_list *instructions);
bool do_dead_code_local(ir_exec_list *instructions);
bool do_dead_code_unlinked(ir_exec_list *instructions);
bool do_lower_jumps(ir_exec_list *instructions, bool pull_out_jumps = true, bool lower_continue = false);
bool do_if_simplification(ir_exec_list *instructions);
bool opt_flatten_nested_if_blocks(ir_exec_list *instructions);
bool do_mat_op_to_vec(ir_exec_list *instructions);
bool do_minmax_prune(ir_exec_list *instructions);
bool do_tree_grafting(ir_exec_list *instructions);
bool do_vec_index_to_cond_assign(ir_exec_list *instructions);
bool lower_instructions(ir_exec_list *instructions,
                        bool force_abs_sqrt, bool have_gpu_shader5);
bool lower_packing_builtins(ir_exec_list *instructions,
                            bool has_shading_language_packing,
                            bool has_gpu_shader5,
                            bool has_half_float_packing);
bool lower_vector_derefs(gl_shader *shader, linear_ctx *linalloc);
void optimize_dead_builtin_variables(ir_exec_list *instructions,
                                     enum ir_variable_mode other);

bool lower_builtins(ir_exec_list *instructions);
bool lower_subroutine(ir_exec_list *instructions, struct _mesa_glsl_parse_state *state);
bool propagate_invariance(ir_exec_list *instructions);

namespace ir_builder { class ir_factory; };

void lower_precision(const struct pipe_screen *screen,
                     mesa_shader_stage stage,
                     ir_exec_list *instructions);

#endif /* GLSL_IR_OPTIMIZATION_H */
