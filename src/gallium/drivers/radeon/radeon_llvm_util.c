#include "radeon_llvm_util.h"
#include "util/u_memory.h"

#include <llvm-c/BitReader.h>
#include <llvm-c/Core.h>

static LLVMModuleRef radeon_llvm_parse_bitcode(const unsigned char * bitcode,
							unsigned bitcode_len)
{
	LLVMMemoryBufferRef buf;
	LLVMModuleRef module = LLVMModuleCreateWithName("radeon");

	buf = LLVMCreateMemoryBufferWithMemoryRangeCopy((const char*)bitcode,
							bitcode_len, "radeon");
	LLVMParseBitcode(buf, &module, NULL);
	return module;
}

unsigned radeon_llvm_get_num_kernels(const unsigned char *bitcode,
				unsigned bitcode_len)
{
	LLVMModuleRef mod = radeon_llvm_parse_bitcode(bitcode, bitcode_len);
	return LLVMGetNamedMetadataNumOperands(mod, "opencl.kernels");
}

LLVMModuleRef radeon_llvm_get_kernel_module(unsigned index,
		const unsigned char *bitcode, unsigned bitcode_len)
{
	LLVMModuleRef mod;
	unsigned num_kernels;
	LLVMValueRef *kernel_metadata;
	unsigned i;

	mod = radeon_llvm_parse_bitcode(bitcode, bitcode_len);
	num_kernels = LLVMGetNamedMetadataNumOperands(mod, "opencl.kernels");
	kernel_metadata = MALLOC(num_kernels * sizeof(LLVMValueRef));
	LLVMGetNamedMetadataOperands(mod, "opencl.kernels", kernel_metadata);
	for (i = 0; i < num_kernels; i++) {
		LLVMValueRef kernel_signature, kernel_function;
		if (i == index) {
			continue;
		}
		kernel_signature = kernel_metadata[i];
		LLVMGetMDNodeOperands(kernel_signature, &kernel_function);
		LLVMDeleteFunction(kernel_function);
	}
	FREE(kernel_metadata);
	return mod;
}
