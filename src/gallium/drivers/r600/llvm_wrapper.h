#ifndef LLVM_WRAPPER_H
#define LLVM_WRAPPER_H

#include <llvm-c/Core.h>

#ifdef __cplusplus
extern "C" {
#endif

LLVMModuleRef llvm_parse_bitcode(const unsigned char * bitcode, unsigned bitcode_len);
void llvm_strip_unused_kernels(LLVMModuleRef mod, const char *kernel_name);
unsigned llvm_get_num_kernels(const unsigned char *bitcode, unsigned bitcode_len);
LLVMModuleRef llvm_get_kernel_module(unsigned index,
			const unsigned char *bitcode, unsigned bitcode_len);

#ifdef __cplusplus
}
#endif

#endif
