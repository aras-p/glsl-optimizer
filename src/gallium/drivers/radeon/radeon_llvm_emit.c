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
#include "util/u_memory.h"

#include <llvm-c/Target.h>
#include <llvm-c/TargetMachine.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <libelf.h>
#include <gelf.h>

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
}

static void init_r600_target() {
	static unsigned initialized = 0;
	if (!initialized) {
		LLVMInitializeR600TargetInfo();
		LLVMInitializeR600Target();
		LLVMInitializeR600TargetMC();
		LLVMInitializeR600AsmPrinter();
		initialized = 1;
	}
}

static LLVMTargetRef get_r600_target() {
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

/**
 * Compile an LLVM module to machine code.
 *
 * @returns 0 for success, 1 for failure
 */
unsigned radeon_llvm_compile(LLVMModuleRef M, struct radeon_llvm_binary *binary,
					  const char * gpu_family, unsigned dump) {

	LLVMTargetRef target;
	LLVMTargetMachineRef tm;
	char cpu[CPU_STRING_LEN];
	char fs[FS_STRING_LEN];
	char *err;
	LLVMMemoryBufferRef out_buffer;
	unsigned buffer_size;
	const char *buffer_data;
	char triple[TRIPLE_STRING_LEN];
	char *elf_buffer;
	Elf *elf;
	Elf_Scn *section = NULL;
	size_t section_str_index;
	LLVMBool r;

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
	tm = LLVMCreateTargetMachine(target, triple, cpu, fs,
				  LLVMCodeGenLevelDefault, LLVMRelocDefault,
						  LLVMCodeModelDefault);

	r = LLVMTargetMachineEmitToMemoryBuffer(tm, M, LLVMObjectFile, &err,
								 &out_buffer);
	if (r) {
		fprintf(stderr, "%s", err);
		FREE(err);
		return 1;
	}

	buffer_size = LLVMGetBufferSize(out_buffer);
	buffer_data = LLVMGetBufferStart(out_buffer);

	/* One of the libelf implementations
	 * (http://www.mr511.de/software/english.htm) requires calling
	 * elf_version() before elf_memory().
	 */
	elf_version(EV_CURRENT);
	elf_buffer = MALLOC(buffer_size);
	memcpy(elf_buffer, buffer_data, buffer_size);

	elf = elf_memory(elf_buffer, buffer_size);

	elf_getshdrstrndx(elf, &section_str_index);

	while ((section = elf_nextscn(elf, section))) {
		const char *name;
		Elf_Data *section_data = NULL;
		GElf_Shdr section_header;
		if (gelf_getshdr(section, &section_header) != &section_header) {
			fprintf(stderr, "Failed to read ELF section header\n");
			return 1;
		}
		name = elf_strptr(elf, section_str_index, section_header.sh_name);
		if (!strcmp(name, ".text")) {
			section_data = elf_getdata(section, section_data);
			binary->code_size = section_data->d_size;
			binary->code = MALLOC(binary->code_size * sizeof(unsigned char));
			memcpy(binary->code, section_data->d_buf, binary->code_size);
		} else if (!strcmp(name, ".AMDGPU.config")) {
			section_data = elf_getdata(section, section_data);
			binary->config_size = section_data->d_size;
			binary->config = MALLOC(binary->config_size * sizeof(unsigned char));
			memcpy(binary->config, section_data->d_buf, binary->config_size);
		}
	}

	LLVMDisposeMemoryBuffer(out_buffer);
	LLVMDisposeTargetMachine(tm);
	return 0;
}
