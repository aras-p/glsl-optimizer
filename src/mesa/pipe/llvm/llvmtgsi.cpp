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
   /* i think this is going to be a noop */
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
   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_ARL:
      break;
   case TGSI_OPCODE_MOV:
      break;
   case TGSI_OPCODE_LIT:
      break;
   case TGSI_OPCODE_RCP:
      break;
   case TGSI_OPCODE_RSQ:
      break;
   case TGSI_OPCODE_EXP:
      break;
   case TGSI_OPCODE_LOG:
      break;
   case TGSI_OPCODE_MUL:
      break;
   case TGSI_OPCODE_ADD:
      break;
   case TGSI_OPCODE_DP3:
      break;
   case TGSI_OPCODE_DP4:
      break;
   case TGSI_OPCODE_DST:
      break;
   case TGSI_OPCODE_MIN:
      break;
   case TGSI_OPCODE_MAX:
      break;
   case TGSI_OPCODE_SLT:
      break;
   case TGSI_OPCODE_SGE:
      break;
   case TGSI_OPCODE_MAD:
      break;
   case TGSI_OPCODE_SUB:
      break;
   case TGSI_OPCODE_LERP:
      break;
   case TGSI_OPCODE_CND:
      break;
   case TGSI_OPCODE_CND0:
      break;
   case TGSI_OPCODE_DOT2ADD:
      break;
   case TGSI_OPCODE_INDEX:
      break;
   case TGSI_OPCODE_NEGATE:
      break;
   case TGSI_OPCODE_FRAC:
      break;
   case TGSI_OPCODE_CLAMP:
      break;
   case TGSI_OPCODE_FLOOR:
      break;
   case TGSI_OPCODE_ROUND:
      break;
   case TGSI_OPCODE_EXPBASE2:
      break;
   case TGSI_OPCODE_LOGBASE2:
      break;
   case TGSI_OPCODE_POWER:
      break;
   case TGSI_OPCODE_CROSSPRODUCT:
      break;
   case TGSI_OPCODE_MULTIPLYMATRIX:
      break;
   case TGSI_OPCODE_ABS:
      break;
   case TGSI_OPCODE_RCC:
      break;
   case TGSI_OPCODE_DPH:
      break;
   case TGSI_OPCODE_COS:
      break;
   case TGSI_OPCODE_DDX:
      break;
   case TGSI_OPCODE_DDY:
      break;
   case TGSI_OPCODE_KILP:
      break;
   case TGSI_OPCODE_PK2H:
      break;
   case TGSI_OPCODE_PK2US:
      break;
   case TGSI_OPCODE_PK4B:
      break;
   case TGSI_OPCODE_PK4UB:
      break;
   case TGSI_OPCODE_RFL:
      break;
   case TGSI_OPCODE_SEQ:
      break;
   case TGSI_OPCODE_SFL:
      break;
   case TGSI_OPCODE_SGT:
      break;
   case TGSI_OPCODE_SIN:
      break;
   case TGSI_OPCODE_SLE:
      break;
   case TGSI_OPCODE_SNE:
      break;
   case TGSI_OPCODE_STR:
      break;
   case TGSI_OPCODE_TEX:
      break;
   case TGSI_OPCODE_TXD:
      break;
   case TGSI_OPCODE_TXP:
      break;
   case TGSI_OPCODE_UP2H:
      break;
   case TGSI_OPCODE_UP2US:
      break;
   case TGSI_OPCODE_UP4B:
      break;
   case TGSI_OPCODE_UP4UB:
      break;
   case TGSI_OPCODE_X2D:
      break;
   case TGSI_OPCODE_ARA:
      break;
   case TGSI_OPCODE_ARR:
      break;
   case TGSI_OPCODE_BRA:
      break;
   case TGSI_OPCODE_CAL:
      break;
   case TGSI_OPCODE_RET:
      break;
   case TGSI_OPCODE_SSG:
      break;
   case TGSI_OPCODE_CMP:
      break;
   case TGSI_OPCODE_SCS:
      break;
   case TGSI_OPCODE_TXB:
      break;
   case TGSI_OPCODE_NRM:
      break;
   case TGSI_OPCODE_DIV:
      break;
   case TGSI_OPCODE_DP2:
      break;
   case TGSI_OPCODE_TXL:
      break;
   case TGSI_OPCODE_BRK:
      break;
   case TGSI_OPCODE_IF:
      break;
   case TGSI_OPCODE_LOOP:
      break;
   case TGSI_OPCODE_REP:
      break;
   case TGSI_OPCODE_ELSE:
      break;
   case TGSI_OPCODE_ENDIF:
      break;
   case TGSI_OPCODE_ENDLOOP:
      break;
   case TGSI_OPCODE_ENDREP:
      break;
   case TGSI_OPCODE_PUSHA:
      break;
   case TGSI_OPCODE_POPA:
      break;
   case TGSI_OPCODE_CEIL:
      break;
   case TGSI_OPCODE_I2F:
      break;
   case TGSI_OPCODE_NOT:
      break;
   case TGSI_OPCODE_TRUNC:
      break;
   case TGSI_OPCODE_SHL:
      break;
   case TGSI_OPCODE_SHR:
      break;
   case TGSI_OPCODE_AND:
      break;
   case TGSI_OPCODE_OR:
      break;
   case TGSI_OPCODE_MOD:
      break;
   case TGSI_OPCODE_XOR:
      break;
   case TGSI_OPCODE_SAD:
      break;
   case TGSI_OPCODE_TXF:
      break;
   case TGSI_OPCODE_TXQ:
      break;
   case TGSI_OPCODE_CONT:
      break;
   case TGSI_OPCODE_EMIT:
      break;
   case TGSI_OPCODE_ENDPRIM:
      break;
   case TGSI_OPCODE_BGNLOOP2:
      break;
   case TGSI_OPCODE_BGNSUB:
      break;
   case TGSI_OPCODE_ENDLOOP2:
      break;
   case TGSI_OPCODE_ENDSUB:
      break;
   case TGSI_OPCODE_NOISE1:
      break;
   case TGSI_OPCODE_NOISE2:
      break;
   case TGSI_OPCODE_NOISE3:
      break;
   case TGSI_OPCODE_NOISE4:
      break;
   case TGSI_OPCODE_NOP:
      break;
   case TGSI_OPCODE_TEXBEM:
      break;
   case TGSI_OPCODE_TEXBEML:
      break;
   case TGSI_OPCODE_TEXREG2AR:
      break;
   case TGSI_OPCODE_TEXM3X2PAD:
      break;
   case TGSI_OPCODE_TEXM3X2TEX:
      break;
   case TGSI_OPCODE_TEXM3X3PAD:
      break;
   case TGSI_OPCODE_TEXM3X3TEX:
      break;
   case TGSI_OPCODE_TEXM3X3SPEC:
      break;
   case TGSI_OPCODE_TEXM3X3VSPEC:
      break;
   case TGSI_OPCODE_TEXREG2GB:
      break;
   case TGSI_OPCODE_TEXREG2RGB:
      break;
   case TGSI_OPCODE_TEXDP3TEX:
      break;
   case TGSI_OPCODE_TEXDP3:
      break;
   case TGSI_OPCODE_TEXM3X3:
      break;
   case TGSI_OPCODE_TEXM3X2DEPTH:
      break;
   case TGSI_OPCODE_TEXDEPTH:
      break;
   case TGSI_OPCODE_BEM:
      break;
   case TGSI_OPCODE_M4X3:
      break;
   case TGSI_OPCODE_M3X4:
      break;
   case TGSI_OPCODE_M3X3:
      break;
   case TGSI_OPCODE_M3X2:
      break;
   case TGSI_OPCODE_NRM4:
      break;
   case TGSI_OPCODE_CALLNZ:
      break;
   case TGSI_OPCODE_IFC:
      break;
   case TGSI_OPCODE_BREAKC:
      break;
   case TGSI_OPCODE_KIL:
      break;
   case TGSI_OPCODE_END:
      break;
   default:
      fprintf(stderr, "ERROR: Unknown opcode %d\n",
              inst->Instruction.Opcode);
      assert(0);
      break;
   }

   switch( inst->Instruction.Saturate ) {
   case TGSI_SAT_NONE:
      break;
   case TGSI_SAT_ZERO_ONE:
      /*TXT( "_SAT" );*/
      break;
   case TGSI_SAT_MINUS_PLUS_ONE:
      /*TXT( "_SAT[-1,1]" );*/
      break;
   default:
      assert( 0 );
   }
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

int ga_llvm_prog_exec(struct ga_llvm_prog *prog,
                      float (*inputs)[32][4],
                      void *dests[16*32*4],
                      float (*consts)[4],
                      int count)
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
