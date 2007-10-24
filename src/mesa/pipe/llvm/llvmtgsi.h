#ifndef LLVMTGSI_H
#define LLVMTGSI_H

#if defined __cplusplus
extern "C" {
#endif

#include "pipe/p_state.h"

#ifdef MESA_LLVM

struct tgsi_exec_machine;
struct tgsi_token;
struct tgsi_sampler;
struct pipe_context;

struct ga_llvm_prog;

struct ga_llvm_prog *
ga_llvm_from_tgsi(struct pipe_context *pipe, const struct tgsi_token *tokens);

void ga_llvm_prog_delete(struct ga_llvm_prog *prog);

int ga_llvm_prog_exec(struct ga_llvm_prog *prog,
                      float (*inputs)[PIPE_MAX_SHADER_INPUTS][4],
                      float (*dests)[PIPE_MAX_SHADER_INPUTS][4],
                      float (*consts)[4],
                      int num_vertices,
                      int num_inputs,
                      int num_attribs);

void ga_llvm_prog_dump(struct ga_llvm_prog *prog, const char *file_prefix);

#endif /* MESA_LLVM */

#if defined __cplusplus
} // extern "C"
#endif

#endif
