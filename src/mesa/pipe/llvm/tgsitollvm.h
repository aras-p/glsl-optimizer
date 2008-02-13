#ifndef TGSITOLLVM_H
#define TGSITOLLVM_H


namespace llvm {
   class Module;
}

struct gallivm_ir;
struct tgsi_token;


llvm::Module * tgsi_to_llvm(struct gallivm_ir *ir,
                            const struct tgsi_token *tokens);


llvm::Module * tgsi_to_llvmir(struct gallivm_ir *ir,
                              const struct tgsi_token *tokens);

#endif
