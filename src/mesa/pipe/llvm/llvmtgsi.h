#ifndef LLVMTGSI_H
#define LLVMTGSI_H

#if defined __cplusplus
extern "C" {
#endif

struct tgsi_exec_machine;
struct tgsi_token;
struct tgsi_sampler;
struct pipe_context;

struct ga_llvm_prog {
   void *module;
   void *engine;
   void *function;
};
struct ga_llvm_prog *
ga_llvm_from_tgsi(struct pipe_context *pipe, const struct tgsi_token *tokens);

void ga_llvm_prog_delete(struct ga_llvm_prog *prog);

int ga_llvm_prog_exec(struct ga_llvm_prog *prog,
                      float (*inputs)[32][4],
                      float (*dests)[32][4],
                      float (*consts)[4],
                      int count,
                      int num_attribs);

#if defined __cplusplus
} // extern "C"
#endif

#endif
