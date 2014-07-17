/*
 * Copyright 2011 Advanced Micro Devices, Inc.
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
#include "radeon_llvm_emit.h"
#include "radeon_elf_util.h"
#include "util/u_memory.h"
#include "pipe/p_shader_tokens.h"

#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>
#include <llvm-c/Core.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CPU_STRING_LEN 30
#define FS_STRING_LEN 30
#define TRIPLE_STRING_LEN 7

/**
 * Set the shader type we want to compile
 *
 * @param type shader type to set
 */
void radeon_llvm_shader_type(LLVMValueRef F, unsigned type)
{
	char Str[2];
	sprintf(Str, "%1d", type);

	LLVMAddTargetDependentFunctionAttr(F, "ShaderType", Str);

	if (type != TGSI_PROCESSOR_COMPUTE) {
		LLVMAddTargetDependentFunctionAttr(F, "unsafe-fp-math", "true");
	}
}

static void init_r600_target()
{
	static unsigned initialized = 0;
	if (!initialized) {
		LLVMInitializeR600TargetInfo();
		LLVMInitializeR600Target();
		LLVMInitializeR600TargetMC();
		LLVMInitializeR600AsmPrinter();
		initialized = 1;
	}
}

static LLVMTargetRef get_r600_target()
{
	LLVMTargetRef target = NULL;

	for (target = LLVMGetFirstTarget(); target;
					target = LLVMGetNextTarget(target)) {
		if (!strncmp(LLVMGetTargetName(target), "r600", 4)) {
			break;
		}
	}

	if (!target) {
		fprintf(stderr, "Can't find target r600\n");
		return NULL;
	}
	return target;
}

#if HAVE_LLVM >= 0x0305

static void radeonDiagnosticHandler(LLVMDiagnosticInfoRef di, void *context)
{
	if (LLVMGetDiagInfoSeverity(di) == LLVMDSError) {
		unsigned int *diagnosticflag = (unsigned int *)context;
		char *diaginfo_message = LLVMGetDiagInfoDescription(di);

		*diagnosticflag = 1;
		fprintf(stderr,"LLVM triggered Diagnostic Handler: %s\n", diaginfo_message);
		LLVMDisposeMessage(diaginfo_message);
	}
}

#endif

/**
 * Compile an LLVM module to machine code.
 *
 * @returns 0 for success, 1 for failure
 */
unsigned radeon_llvm_compile(LLVMModuleRef M, struct radeon_shader_binary *binary,
					  const char *gpu_family, unsigned dump)
{

	LLVMTargetRef target;
	LLVMTargetMachineRef tm;
	char cpu[CPU_STRING_LEN];
	char fs[FS_STRING_LEN];
	char *err;
	LLVMContextRef llvm_ctx;
	unsigned rval = 0;
	LLVMMemoryBufferRef out_buffer;
	unsigned buffer_size;
	const char *buffer_data;
	char triple[TRIPLE_STRING_LEN];
	LLVMBool mem_err;

	/* initialise */
	init_r600_target();

	target = get_r600_target();
	if (!target) {
		return 1;
	}

	strncpy(cpu, gpu_family, CPU_STRING_LEN);
	memset(fs, 0, sizeof(fs));
	if (dump) {
		LLVMDumpModule(M);
		strncpy(fs, "+DumpCode", FS_STRING_LEN);
	}
	strncpy(triple, "r600--", TRIPLE_STRING_LEN);

	/* Setup Diagnostic Handler*/
	llvm_ctx = LLVMGetModuleContext(M);

#if HAVE_LLVM >= 0x0305
	LLVMContextSetDiagnosticHandler(llvm_ctx, radeonDiagnosticHandler, &rval);
#endif
	rval = 0;

	/* Compile IR*/
	tm = LLVMCreateTargetMachine(target, triple, cpu, fs,
				  LLVMCodeGenLevelDefault, LLVMRelocDefault,
						  LLVMCodeModelDefault);
	mem_err = LLVMTargetMachineEmitToMemoryBuffer(tm, M, LLVMObjectFile, &err,
								 &out_buffer);

	/* Process Errors/Warnings */
	if (mem_err) {
		fprintf(stderr, "%s: %s", __FUNCTION__, err);
		FREE(err);
		LLVMDisposeTargetMachine(tm);
		return 1;
	}

	if (0 != rval) {
		fprintf(stderr, "%s: Processing Diag Flag\n", __FUNCTION__);
	}

	/* Extract Shader Code*/
	buffer_size = LLVMGetBufferSize(out_buffer);
	buffer_data = LLVMGetBufferStart(out_buffer);

	radeon_elf_read(buffer_data, buffer_size, binary, dump);

	/* Clean up */
	LLVMDisposeMemoryBuffer(out_buffer);
	LLVMDisposeTargetMachine(tm);
	return rval;
}
