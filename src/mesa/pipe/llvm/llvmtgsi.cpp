#include "llvmtgsi.h"

#include "pipe/tgsi/exec/tgsi_exec.h"
#include "pipe/tgsi/exec/tgsi_token.h"
#include "pipe/tgsi/exec/tgsi_build.h"
#include "pipe/tgsi/exec/tgsi_util.h"
#include "pipe/tgsi/exec/tgsi_parse.h"
#include "pipe/tgsi/exec/tgsi_dump.h"
//#include "pipe/tgsi/tgsi_platform.h"

#include <llvm/Module.h>
#include <llvm/CallingConv.h>
#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Instructions.h>
#include <llvm/ModuleProvider.h>
#include <llvm/ParameterAttributes.h>
#include <llvm/Support/PatternMatch.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <iostream>


static void
translate_declaration(llvm::Module *module,
                      struct tgsi_full_declaration *decl,
                      struct tgsi_full_declaration *fd)
{
}


static void
translate_immediate(llvm::Module *module,
                    struct tgsi_full_immediate *imm)
{
}


static void
translate_instruction(llvm::Module *module,
                      struct tgsi_full_instruction *inst,
                      struct tgsi_full_instruction *fi)
{
}


static llvm::Module *
tgsi_to_llvm(const struct tgsi_token *tokens)
{
   llvm::Module *mod = new llvm::Module("tgsi");
   struct tgsi_parse_context parse;
   struct tgsi_full_instruction fi;
   struct tgsi_full_declaration fd;

   tgsi_parse_init(&parse, tokens);

   //parse.FullHeader.Processor.Processor

   //parse.FullVersion.Version.MajorVersion
   //parse.FullVersion.Version.MinorVersion

   //parse.FullHeader.Header.HeaderSize
   //parse.FullHeader.Header.BodySize
   //parse.FullHeader.Processor.Processor

   fi = tgsi_default_full_instruction();
   fd = tgsi_default_full_declaration();

   while(!tgsi_parse_end_of_tokens(&parse)) {
      tgsi_parse_token(&parse);

      fprintf(stderr, "Translating %d\n", parse.FullToken.Token.Type);

      switch (parse.FullToken.Token.Type) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         translate_declaration(mod,
                               &parse.FullToken.FullDeclaration,
                               &fd);
         break;

      case TGSI_TOKEN_TYPE_IMMEDIATE:
         translate_immediate(mod,
                             &parse.FullToken.FullImmediate);
         break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         translate_instruction(mod,
                               &parse.FullToken.FullInstruction,
                               &fi);
         break;

      default:
         assert(0);
      }
   }

   //TXT("\ntgsi-dump end -------------------\n");

   tgsi_parse_free(&parse);

   return mod;
}

struct ga_llvm_prog *
ga_llvm_from_tgsi(const struct tgsi_token *tokens)
{
   std::cout << "Creating llvm " <<std::endl;
   struct ga_llvm_prog *ga_llvm =
      (struct ga_llvm_prog *)malloc(sizeof(struct ga_llvm_prog));
   llvm::Module *mod = tgsi_to_llvm(tokens);
   llvm::ExistingModuleProvider *mp =
      new llvm::ExistingModuleProvider(mod);
   //llvm::ExecutionEngine *ee =
   //   llvm::ExecutionEngine::create(mp, false);

   ga_llvm->module = mod;
   ga_llvm->engine = 0;//ee;
   fprintf(stderr, "DUMPX \n");
   //tgsi_dump(tokens, TGSI_DUMP_VERBOSE);
   tgsi_dump(tokens, 0);
   fprintf(stderr, "DUMPEND \n");

   return ga_llvm;
}

void ga_llvm_prog_delete(struct ga_llvm_prog *prog)
{
   llvm::Module *mod = static_cast<llvm::Module*>(prog->module);
   delete mod;
   prog->module = 0;
   prog->engine = 0;
   free(prog);
}

int ga_llvm_prog_exec(struct ga_llvm_prog *prog)
{
   //std::cout << "START "<<std::endl;
   llvm::Module *mod = static_cast<llvm::Module*>(prog->module);
   llvm::Function *func = mod->getFunction("main");
   llvm::ExecutionEngine *ee = static_cast<llvm::ExecutionEngine*>(prog->engine);

   std::vector<llvm::GenericValue> args(0);
   //args[0] = GenericValue(&st);
   //std::cout << "Mod is "<<*mod;
   //std::cout << "\n\nRunning llvm: " << std::endl;
   if (func) {
      std::cout << "Func is "<<func;
      llvm::GenericValue gv = ee->runFunction(func, args);
   }

//delete ee;
//delete mp;

   return 0;
}
