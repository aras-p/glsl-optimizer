/*
 * Copyright 2014 Advanced Micro Devices, Inc.
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

#include "radeon_elf_util.h"
#include "r600_pipe_common.h"

#include "util/u_memory.h"

#include <gelf.h>
#include <libelf.h>
#include <stdio.h>

void radeon_elf_read(const char *elf_data, unsigned elf_size,
					struct radeon_shader_binary *binary,
					unsigned debug)
{
	char *elf_buffer;
	Elf *elf;
	Elf_Scn *section = NULL;
	size_t section_str_index;

	/* One of the libelf implementations
	 * (http://www.mr511.de/software/english.htm) requires calling
	 * elf_version() before elf_memory().
	 */
	elf_version(EV_CURRENT);
	elf_buffer = MALLOC(elf_size);
	memcpy(elf_buffer, elf_data, elf_size);

	elf = elf_memory(elf_buffer, elf_size);

	elf_getshdrstrndx(elf, &section_str_index);
	binary->disassembled = 0;

	while ((section = elf_nextscn(elf, section))) {
		const char *name;
		Elf_Data *section_data = NULL;
		GElf_Shdr section_header;
		if (gelf_getshdr(section, &section_header) != &section_header) {
			fprintf(stderr, "Failed to read ELF section header\n");
			return;
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
		} else if (debug && !strcmp(name, ".AMDGPU.disasm")) {
			binary->disassembled = 1;
			section_data = elf_getdata(section, section_data);
			fprintf(stderr, "\nShader Disassembly:\n\n");
			fprintf(stderr, "%.*s\n", (int)section_data->d_size,
						  (char *)section_data->d_buf);
		}
	}

	if (elf){
		elf_end(elf);
	}
	FREE(elf_buffer);
}
