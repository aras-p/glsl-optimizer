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

#if HAVE_LLVM < 0x0303
#include <llvm/LLVMContext.h>
#include <llvm/Module.h>
#include <llvm/Function.h>
#include <llvm/DataLayout.h>
#else
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/DataLayout.h>
#endif

#include <llvm/PassManager.h>
#include <llvm/ADT/Triple.h>
#include <llvm/Support/CBindingWrapping.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/Threading.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm-c/Target.h>

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <libelf.h>
#include <gelf.h>

using namespace llvm;

namespace {

class LLVMEnsureMultithreaded {
public:
   LLVMEnsureMultithreaded()
   {
      llvm_start_multithreaded();
   }
};

static LLVMEnsureMultithreaded lLVMEnsureMultithreaded;

}

/**
 * Set the shader type we want to compile
 *
 * @param type shader type to set
 */
extern "C" void
radeon_llvm_shader_type(LLVMValueRef F, unsigned type)
{
  Function *Func = unwrap<Function>(F);
  int Idx = AttributeSet::FunctionIndex;
  AttrBuilder B;
  char Str[2];

  sprintf(Str, "%1d", type);
  B.addAttribute("ShaderType", Str);

  AttributeSet Set = AttributeSet::get(Func->getContext(), Idx, B);
  Func->addAttributes(Idx, Set);
}

/**
 * Compile an LLVM module to machine code.
 *
 * @param bytes This function allocates memory for the byte stream, it is the
 * caller's responsibility to free it.
 */
extern "C" unsigned
radeon_llvm_compile(LLVMModuleRef M, struct radeon_llvm_binary *binary,
                 const char * gpu_family, unsigned dump) {

   Triple AMDGPUTriple(sys::getDefaultTargetTriple());

   LLVMInitializeR600TargetInfo();
   LLVMInitializeR600Target();
   LLVMInitializeR600TargetMC();
   LLVMInitializeR600AsmPrinter();

   std::string err;
   const Target * AMDGPUTarget = TargetRegistry::lookupTarget("r600", err);
   if(!AMDGPUTarget) {
      fprintf(stderr, "Can't find target: %s\n", err.c_str());
      return 1;
   }
   
   Triple::ArchType Arch = Triple::getArchTypeForLLVMName("r600");
   if (Arch == Triple::UnknownArch) {
      fprintf(stderr, "Unknown Arch\n");
   }
   AMDGPUTriple.setArch(Arch);

   Module * mod = unwrap(M);
   std::string FS;
   TargetOptions TO;

   if (dump) {
      mod->dump();
      FS += "+DumpCode";
   }

   std::auto_ptr<TargetMachine> tm(AMDGPUTarget->createTargetMachine(
                     AMDGPUTriple.getTriple(), gpu_family, FS,
                     TO, Reloc::Default, CodeModel::Default,
                     CodeGenOpt::Default
                     ));
   TargetMachine &AMDGPUTargetMachine = *tm.get();
   PassManager PM;
   PM.add(new DataLayout(*AMDGPUTargetMachine.getDataLayout()));
   PM.add(createPromoteMemoryToRegisterPass());
   AMDGPUTargetMachine.setAsmVerbosityDefault(true);

   std::string CodeString;
   raw_string_ostream oStream(CodeString);
   formatted_raw_ostream out(oStream);

   /* Optional extra paramater true / false to disable verify */
   if (AMDGPUTargetMachine.addPassesToEmitFile(PM, out, TargetMachine::CGFT_ObjectFile,
                                               true)){
      fprintf(stderr, "AddingPasses failed.\n");
      return 1;
   }
   PM.run(*mod);

   out.flush();
   std::string &data = oStream.str();

   char *elf_buffer;

   /* One of the libelf implementations (http://www.mr511.de/software/english.htm)
    * requires calling elf_version() before elf_memory().
    */
   elf_version(EV_CURRENT);
   elf_buffer = (char*)malloc(data.length());
   memcpy(elf_buffer, data.c_str(), data.length());

   Elf *elf = elf_memory(elf_buffer, data.length());
   Elf_Scn *section = NULL;
   size_t section_str_index;

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
         binary->code = (unsigned char*)malloc(binary->code_size * sizeof(unsigned char));
         memcpy(binary->code, section_data->d_buf, binary->code_size);
      } else if (!strcmp(name, ".AMDGPU.config")) {
         section_data = elf_getdata(section, section_data);
         binary->config_size = section_data->d_size;
         binary->config = (unsigned char*)malloc(binary->config_size * sizeof(unsigned char));
         memcpy(binary->config, section_data->d_buf, binary->config_size);
      }
   }

   return 0;
}
