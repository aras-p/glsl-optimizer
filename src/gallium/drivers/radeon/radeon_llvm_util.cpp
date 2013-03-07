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

extern "C" void radeon_llvm_strip_unused_kernels(LLVMModuleRef mod, const char *kernel_name)
{
	llvm::Module *M = llvm::unwrap(mod);
	std::vector<const char *> export_list;
	export_list.push_back(kernel_name);
	llvm::PassManager PM;
	PM.add(llvm::createInternalizePass(export_list));
	PM.add(llvm::createGlobalDCEPass());
	PM.run(*M);
}

extern "C" unsigned radeon_llvm_get_num_kernels(const unsigned char *bitcode,
				unsigned bitcode_len)
{
	LLVMModuleRef mod = radeon_llvm_parse_bitcode(bitcode, bitcode_len);
	llvm::Module *M = llvm::unwrap(mod);
	const llvm::NamedMDNode *kernel_node
				= M->getNamedMetadata("opencl.kernels");
	unsigned kernel_count = kernel_node->getNumOperands();
	delete M;
	return kernel_count;
}

extern "C" LLVMModuleRef radeon_llvm_get_kernel_module(unsigned index,
		const unsigned char *bitcode, unsigned bitcode_len)
{
	LLVMModuleRef mod = radeon_llvm_parse_bitcode(bitcode, bitcode_len);
	llvm::Module *M = llvm::unwrap(mod);
	const llvm::NamedMDNode *kernel_node =
				M->getNamedMetadata("opencl.kernels");
	const char* kernel_name = kernel_node->getOperand(index)->
					getOperand(0)->getName().data();
	radeon_llvm_strip_unused_kernels(mod, kernel_name);
	return mod;

}
