#ifndef RADEON_LLVM_UTIL_H
#define RADEON_LLVM_UTIL_H

#include <llvm-c/Core.h>

#ifdef __cplusplus
extern "C" {
#endif

unsigned radeon_llvm_get_num_kernels(const unsigned char *bitcode, unsigned bitcode_len);
LLVMModuleRef radeon_llvm_get_kernel_module(unsigned index,
			const unsigned char *bitcode, unsigned bitcode_len);

#ifdef __cplusplus
}
#endif

#endif
