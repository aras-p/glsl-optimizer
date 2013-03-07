#ifndef RADEON_LLVM_UTIL_H
#define RADEON_LLVM_UTIL_H

#include <llvm-c/Core.h>

unsigned radeon_llvm_get_num_kernels(const unsigned char *bitcode, unsigned bitcode_len);
LLVMModuleRef radeon_llvm_get_kernel_module(unsigned index,
			const unsigned char *bitcode, unsigned bitcode_len);

#endif
