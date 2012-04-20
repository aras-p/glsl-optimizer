//
// Copyright 2012 Francisco Jerez
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
// OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//

#include "core/compiler.hpp"

#if 0
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <llvm/LLVMContext.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/MemoryBuffer.h>

#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdio>
#endif

using namespace clover;

#if 0
namespace {
   void
   build_binary(const std::string &source, const std::string &target,
                const std::string &name) {
      clang::CompilerInstance c;
      clang::EmitObjAction act(&llvm::getGlobalContext());
      std::string log;
      llvm::raw_string_ostream s_log(log);

      LLVMInitializeTGSITarget();
      LLVMInitializeTGSITargetInfo();
      LLVMInitializeTGSITargetMC();
      LLVMInitializeTGSIAsmPrinter();

      c.getFrontendOpts().Inputs.push_back(
         std::make_pair(clang::IK_OpenCL, name));
      c.getHeaderSearchOpts().UseBuiltinIncludes = false;
      c.getHeaderSearchOpts().UseStandardIncludes = false;
      c.getLangOpts().NoBuiltin = true;
      c.getTargetOpts().Triple = target;
      c.getInvocation().setLangDefaults(clang::IK_OpenCL);
      c.createDiagnostics(0, NULL, new clang::TextDiagnosticPrinter(
                             s_log, c.getDiagnosticOpts()));

      c.getPreprocessorOpts().addRemappedFile(
         name, llvm::MemoryBuffer::getMemBuffer(source));

      if (!c.ExecuteAction(act))
         throw build_error(log);
   }

   module
   load_binary(const char *name) {
      std::ifstream fs((name));
      std::vector<unsigned char> str((std::istreambuf_iterator<char>(fs)),
                                     (std::istreambuf_iterator<char>()));
      compat::istream cs(str);
      return module::deserialize(cs);
   }
}
#endif

module
clover::compile_program_llvm(const compat::string &source,
                             const compat::string &target) {
#if 0
   build_binary(source, target, "cl_input");
   module m = load_binary("cl_input.o");
   std::remove("cl_input.o");
   return m;
#endif
   return module();
}
