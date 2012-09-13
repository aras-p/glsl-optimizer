#include <llvm/ADT/OwningPtr.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/LLVMContext.h>
#include <llvm/PassManager.h>
#include <llvm/Support/IRReader.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Transforms/IPO.h>

#include "llvm_wrapper.h"


extern "C" LLVMModuleRef llvm_parse_bitcode(const unsigned char * bitcode, unsigned bitcode_len)
{
	llvm::OwningPtr<llvm::Module> M;
	llvm::StringRef str((const char*)bitcode, bitcode_len);
	llvm::MemoryBuffer*  buffer = llvm::MemoryBuffer::getMemBufferCopy(str);
	llvm::SMDiagnostic Err;
	M.reset(llvm::ParseIR(buffer, Err, llvm::getGlobalContext()));
	return wrap(M.take());
}

extern "C" void llvm_strip_unused_kernels(LLVMModuleRef mod, const char *kernel_name)
{
	llvm::Module *M = llvm::unwrap(mod);
	std::vector<const char *> export_list;
	export_list.push_back(kernel_name);
	llvm::PassManager PM;
	PM.add(llvm::createInternalizePass(export_list));
	PM.add(llvm::createGlobalDCEPass());
	PM.run(*M);
}

extern "C" unsigned llvm_get_num_kernels(const unsigned char *bitcode,
				unsigned bitcode_len)
{
	LLVMModuleRef mod = llvm_parse_bitcode(bitcode, bitcode_len);
	llvm::Module *M = llvm::unwrap(mod);
	const llvm::NamedMDNode *kernel_node
				= M->getNamedMetadata("opencl.kernels");
	unsigned kernel_count = kernel_node->getNumOperands();
	delete M;
	return kernel_count;
}

extern "C" LLVMModuleRef llvm_get_kernel_module(unsigned index,
		const unsigned char *bitcode, unsigned bitcode_len)
{
	LLVMModuleRef mod = llvm_parse_bitcode(bitcode, bitcode_len);
	llvm::Module *M = llvm::unwrap(mod);
	const llvm::NamedMDNode *kernel_node =
				M->getNamedMetadata("opencl.kernels");
	const char* kernel_name = kernel_node->getOperand(index)->
					getOperand(0)->getName().data();
	llvm_strip_unused_kernels(mod, kernel_name);
	return mod;
}
