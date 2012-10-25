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

LLVMModuleRef radeon_llvm_parse_bitcode(const unsigned char * bitcode,
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
