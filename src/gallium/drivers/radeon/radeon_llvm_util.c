/*
 * Copyright 2012, 2013 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors: Tom Stellard <thomas.stellard@amd.com>
 *
 */

#include "radeon_llvm_util.h"
#include "util/u_memory.h"

#include <llvm-c/BitReader.h>
#include <llvm-c/Core.h>
#include <llvm-c/Target.h>
#include <llvm-c/Transforms/IPO.h>
#include <llvm-c/Transforms/PassManagerBuilder.h>

LLVMModuleRef radeon_llvm_parse_bitcode(LLVMContextRef ctx,
							const unsigned char * bitcode, unsigned bitcode_len)
{
	LLVMMemoryBufferRef buf;
	LLVMModuleRef module;

	buf = LLVMCreateMemoryBufferWithMemoryRangeCopy((const char*)bitcode,
							bitcode_len, "radeon");
	LLVMParseBitcodeInContext(ctx, buf, &module, NULL);
	LLVMDisposeMemoryBuffer(buf);
	return module;
}

unsigned radeon_llvm_get_num_kernels(LLVMContextRef ctx,
				const unsigned char *bitcode, unsigned bitcode_len)
{
	LLVMModuleRef mod = radeon_llvm_parse_bitcode(ctx, bitcode, bitcode_len);
	return LLVMGetNamedMetadataNumOperands(mod, "opencl.kernels");
}

static void radeon_llvm_optimize(LLVMModuleRef mod)
{
	const char *data_layout = LLVMGetDataLayout(mod);
	LLVMTargetDataRef TD = LLVMCreateTargetData(data_layout);
	LLVMPassManagerBuilderRef builder = LLVMPassManagerBuilderCreate();
	LLVMPassManagerRef pass_manager = LLVMCreatePassManager();

	/* Functions calls are not supported yet, so we need to inline
	 * everything.  The most efficient way to do this is to add
	 * the always_inline attribute to all non-kernel functions
	 * and then run the Always Inline pass.  The Always Inline
	 * pass will automaically inline functions with this attribute
	 * and does not perform the expensive cost analysis that the normal
	 * inliner does.
	 */

	LLVMValueRef fn;
	for (fn = LLVMGetFirstFunction(mod); fn; fn = LLVMGetNextFunction(fn)) {
		/* All the non-kernel functions have internal linkage */
		if (LLVMGetLinkage(fn) == LLVMInternalLinkage) {
			LLVMAddFunctionAttr(fn, LLVMAlwaysInlineAttribute);
		}
	}

	LLVMAddTargetData(TD, pass_manager);
	LLVMAddAlwaysInlinerPass(pass_manager);
	LLVMPassManagerBuilderPopulateModulePassManager(builder, pass_manager);

	LLVMRunPassManager(pass_manager, mod);
	LLVMPassManagerBuilderDispose(builder);
	LLVMDisposePassManager(pass_manager);
	LLVMDisposeTargetData(TD);
}

LLVMModuleRef radeon_llvm_get_kernel_module(LLVMContextRef ctx, unsigned index,
		const unsigned char *bitcode, unsigned bitcode_len)
{
	LLVMModuleRef mod;
	unsigned num_kernels;
	LLVMValueRef *kernel_metadata;
	unsigned i;

	mod = radeon_llvm_parse_bitcode(ctx, bitcode, bitcode_len);
	num_kernels = LLVMGetNamedMetadataNumOperands(mod, "opencl.kernels");
	kernel_metadata = MALLOC(num_kernels * sizeof(LLVMValueRef));
	LLVMGetNamedMetadataOperands(mod, "opencl.kernels", kernel_metadata);
	for (i = 0; i < num_kernels; i++) {
		LLVMValueRef kernel_signature, *kernel_function;
		unsigned num_kernel_md_operands;
		if (i == index) {
			continue;
		}
		kernel_signature = kernel_metadata[i];
		num_kernel_md_operands = LLVMGetMDNodeNumOperands(kernel_signature);
		kernel_function = MALLOC(num_kernel_md_operands * sizeof (LLVMValueRef));
		LLVMGetMDNodeOperands(kernel_signature, kernel_function);
		LLVMDeleteFunction(*kernel_function);
		FREE(kernel_function);
	}
	FREE(kernel_metadata);
	radeon_llvm_optimize(mod);
	return mod;
}
