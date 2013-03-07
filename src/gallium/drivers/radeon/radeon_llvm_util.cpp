#include <llvm/ADT/OwningPtr.h>
#include <llvm/ADT/StringRef.h>
#if HAVE_LLVM < 0x0303
#include <llvm/LLVMContext.h>
#else
#include <llvm/IR/LLVMContext.h>
#endif
#include <llvm/PassManager.h>
#include <llvm/Support/IRReader.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Transforms/IPO.h>
#include <llvm-c/BitReader.h>
#include <llvm-c/Core.h>

#include "radeon_llvm_util.h"
#include "util/u_memory.h"


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

extern "C" unsigned radeon_llvm_get_num_kernels(const unsigned char *bitcode,
				unsigned bitcode_len)
{
	LLVMModuleRef mod = radeon_llvm_parse_bitcode(bitcode, bitcode_len);
	return LLVMGetNamedMetadataNumOperands(mod, "opencl.kernels");
}

extern "C" LLVMModuleRef radeon_llvm_get_kernel_module(unsigned index,
		const unsigned char *bitcode, unsigned bitcode_len)
{
	LLVMModuleRef mod;
	unsigned num_kernels;
	LLVMValueRef *kernel_metadata;
	unsigned i;

	mod = radeon_llvm_parse_bitcode(bitcode, bitcode_len);
	num_kernels = LLVMGetNamedMetadataNumOperands(mod, "opencl.kernels");
	kernel_metadata = (LLVMValueRef*)MALLOC(num_kernels * sizeof(LLVMValueRef));
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
