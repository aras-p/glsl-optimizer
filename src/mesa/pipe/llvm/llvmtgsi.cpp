#include "llvmtgsi.h"

#include "instructions.h"
#include "storage.h"

#include "pipe/p_context.h"
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
#include <llvm/Pass.h>
#include <llvm/PassManager.h>
#include <llvm/ParameterAttributes.h>
#include <llvm/Support/PatternMatch.h>
#include <llvm/ExecutionEngine/JIT.h>
#include <llvm/ExecutionEngine/Interpreter.h>
#include <llvm/ExecutionEngine/GenericValue.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/LinkAllPasses.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Target/TargetData.h>
#include <llvm/Bitcode/ReaderWriter.h>
#include <iostream>

using namespace llvm;
#include "llvm_base_shader.cpp"


static inline void addPass(PassManager &PM, Pass *P) {
  // Add the pass to the pass manager...
  PM.add(P);
}

static inline void AddStandardCompilePasses(PassManager &PM) {
   PM.add(createVerifierPass());                  // Verify that input is correct

   addPass(PM, createLowerSetJmpPass());          // Lower llvm.setjmp/.longjmp

   // If the -strip-debug command line option was specified, do it.
   //if (StripDebug)
   //  addPass(PM, createStripSymbolsPass(true));

   addPass(PM, createRaiseAllocationsPass());     // call %malloc -> malloc inst
   addPass(PM, createCFGSimplificationPass());    // Clean up disgusting code
   addPass(PM, createPromoteMemoryToRegisterPass());// Kill useless allocas
   addPass(PM, createGlobalOptimizerPass());      // Optimize out global vars
   addPass(PM, createGlobalDCEPass());            // Remove unused fns and globs
   addPass(PM, createIPConstantPropagationPass());// IP Constant Propagation
   addPass(PM, createDeadArgEliminationPass());   // Dead argument elimination
   addPass(PM, createInstructionCombiningPass()); // Clean up after IPCP & DAE
   addPass(PM, createCFGSimplificationPass());    // Clean up after IPCP & DAE

   addPass(PM, createPruneEHPass());              // Remove dead EH info

   //if (!DisableInline)
   addPass(PM, createFunctionInliningPass());   // Inline small functions
   addPass(PM, createArgumentPromotionPass());    // Scalarize uninlined fn args

   addPass(PM, createTailDuplicationPass());      // Simplify cfg by copying code
   addPass(PM, createInstructionCombiningPass()); // Cleanup for scalarrepl.
   addPass(PM, createCFGSimplificationPass());    // Merge & remove BBs
   addPass(PM, createScalarReplAggregatesPass()); // Break up aggregate allocas
   addPass(PM, createInstructionCombiningPass()); // Combine silly seq's
   addPass(PM, createCondPropagationPass());      // Propagate conditionals

   addPass(PM, createTailCallEliminationPass());  // Eliminate tail calls
   addPass(PM, createCFGSimplificationPass());    // Merge & remove BBs
   addPass(PM, createReassociatePass());          // Reassociate expressions
   addPass(PM, createLoopRotatePass());
   addPass(PM, createLICMPass());                 // Hoist loop invariants
   addPass(PM, createLoopUnswitchPass());         // Unswitch loops.
   addPass(PM, createLoopIndexSplitPass());       // Index split loops.
   addPass(PM, createInstructionCombiningPass()); // Clean up after LICM/reassoc
   addPass(PM, createIndVarSimplifyPass());       // Canonicalize indvars
   addPass(PM, createLoopUnrollPass());           // Unroll small loops
   addPass(PM, createInstructionCombiningPass()); // Clean up after the unroller
   addPass(PM, createGVNPass());                  // Remove redundancies
   addPass(PM, createSCCPPass());                 // Constant prop with SCCP

   // Run instcombine after redundancy elimination to exploit opportunities
   // opened up by them.
   addPass(PM, createInstructionCombiningPass());
   addPass(PM, createCondPropagationPass());      // Propagate conditionals

   addPass(PM, createDeadStoreEliminationPass()); // Delete dead stores
   addPass(PM, createAggressiveDCEPass());        // SSA based 'Aggressive DCE'
   addPass(PM, createCFGSimplificationPass());    // Merge & remove BBs
   addPass(PM, createSimplifyLibCallsPass());     // Library Call Optimizations
   addPass(PM, createDeadTypeEliminationPass());  // Eliminate dead types
   addPass(PM, createConstantMergePass());        // Merge dup global constants
}

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
                      Storage *storage,
                      Instructions *instr,
                      struct tgsi_full_instruction *inst,
                      struct tgsi_full_instruction *fi)
{
   llvm::Value *inputs[4];
   printf("translate instr START\n");
   for (int i = 0; i < inst->Instruction.NumSrcRegs; ++i) {
      struct tgsi_full_src_register *src = &inst->FullSrcRegisters[i];
      llvm::Value *val = 0;
      if (src->SrcRegister.File == TGSI_FILE_CONSTANT) {
         val = storage->constElement(src->SrcRegister.Index);
      } else if (src->SrcRegister.File == TGSI_FILE_INPUT) {
         val = storage->inputElement(src->SrcRegister.Index);
      } else if (src->SrcRegister.File == TGSI_FILE_TEMPORARY) {
         val = storage->tempElement(src->SrcRegister.Index);
      } else {
         fprintf(stderr, "ERROR: not support llvm source\n");
         printf("translate instr END\n");
         return;
      }

      if (src->SrcRegister.Extended) {
         if (src->SrcRegisterExtSwz.ExtSwizzleX != TGSI_EXTSWIZZLE_X ||
             src->SrcRegisterExtSwz.ExtSwizzleY != TGSI_EXTSWIZZLE_Y ||
             src->SrcRegisterExtSwz.ExtSwizzleZ != TGSI_EXTSWIZZLE_Z ||
             src->SrcRegisterExtSwz.ExtSwizzleW != TGSI_EXTSWIZZLE_W) {
            int swizzle = src->SrcRegisterExtSwz.ExtSwizzleX * 1000;
            swizzle += src->SrcRegisterExtSwz.ExtSwizzleY * 100;
            swizzle += src->SrcRegisterExtSwz.ExtSwizzleZ * 10;
            swizzle += src->SrcRegisterExtSwz.ExtSwizzleW * 1;
            val = storage->shuffleVector(val, swizzle);
         }
      } else if (src->SrcRegister.SwizzleX != TGSI_SWIZZLE_X ||
                 src->SrcRegister.SwizzleY != TGSI_SWIZZLE_Y ||
                 src->SrcRegister.SwizzleZ != TGSI_SWIZZLE_Z ||
                 src->SrcRegister.SwizzleW != TGSI_SWIZZLE_W) {
         fprintf(stderr, "SWIZZLE is %d %d %d %d\n",
                 src->SrcRegister.SwizzleX, src->SrcRegister.SwizzleY,
                 src->SrcRegister.SwizzleZ, src->SrcRegister.SwizzleW);
         int swizzle = src->SrcRegister.SwizzleX * 1000;
         swizzle += src->SrcRegister.SwizzleY  * 100;
         swizzle += src->SrcRegister.SwizzleZ  * 10;
         swizzle += src->SrcRegister.SwizzleW  * 1;
         val = storage->shuffleVector(val, swizzle);
      }
      inputs[i] = val;
   }

   llvm::Value *out = 0;
   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_ARL:
      break;
   case TGSI_OPCODE_MOV: {
      out = inputs[0];
   }
      break;
   case TGSI_OPCODE_LIT: {
      //out = instr->lit(inputs[0]);
      return;
   }
      break;
   case TGSI_OPCODE_RCP: {
      out = instr->rcp(inputs[0]);
   }
      break;
   case TGSI_OPCODE_RSQ: {
      out = instr->rsq(inputs[0]);
   }
      break;
   case TGSI_OPCODE_EXP:
      break;
   case TGSI_OPCODE_LOG:
      break;
   case TGSI_OPCODE_MUL: {
      out = instr->mul(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_ADD: {
      out = instr->add(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_DP3: {
      out = instr->dp3(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_DP4: {
      out = instr->dp4(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_DST: {
      out = instr->dst(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_MIN:
      break;
   case TGSI_OPCODE_MAX:
      break;
   case TGSI_OPCODE_SLT:
      break;
   case TGSI_OPCODE_SGE:
      break;
   case TGSI_OPCODE_MAD: {
      out = instr->madd(inputs[0], inputs[1], inputs[2]);
   }
      break;
   case TGSI_OPCODE_SUB: {
      out = instr->sub(inputs[0], inputs[1]);
   }
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
   case TGSI_OPCODE_POWER: {
      out = instr->pow(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_CROSSPRODUCT:
      break;
   case TGSI_OPCODE_MULTIPLYMATRIX:
      break;
   case TGSI_OPCODE_ABS:
      break;
   case TGSI_OPCODE_RCC:
      break;
   case TGSI_OPCODE_DPH: {
      out = instr->dph(inputs[0], inputs[1]);
   }
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
      printf("translate instr END\n");
      return;
      break;
   default:
      fprintf(stderr, "ERROR: Unknown opcode %d\n",
              inst->Instruction.Opcode);
      assert(0);
      break;
   }

   if (!out) {
      fprintf(stderr, "ERROR: unsupported opcode %d\n",
              inst->Instruction.Opcode);
      assert(!"Unsupported opcode");
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

   for (int i = 0; i < inst->Instruction.NumDstRegs; ++i) {
      struct tgsi_full_dst_register *dst = &inst->FullDstRegisters[i];

      if (dst->DstRegister.File == TGSI_FILE_OUTPUT) {
         printf("--- storing to %d %p\n", dst->DstRegister.Index, out);
         storage->store(dst->DstRegister.Index, out);
      } else if (dst->DstRegister.File == TGSI_FILE_TEMPORARY) {
         storage->setTempElement(dst->DstRegister.Index, out);
      } else {
         fprintf(stderr, "ERROR: unsupported LLVM destination!");
      }

#if 0
      if (dst->DstRegister.WriteMask != TGSI_WRITEMASK_XYZW) {
         if (dst->DstRegister.WriteMask & TGSI_WRITEMASK_X) {
         }
         if (dst->DstRegister.WriteMask & TGSI_WRITEMASK_Y) {
         }
         if (dst->DstRegister.WriteMask & TGSI_WRITEMASK_Z) {
         }
         if (dst->DstRegister.WriteMask & TGSI_WRITEMASK_W) {
         }
      }
#endif
   }
   printf("translate instr END\n");
}


static llvm::Module *
tgsi_to_llvm(const struct tgsi_token *tokens)
{
   llvm::Module *mod = createBaseShader();
   struct tgsi_parse_context parse;
   struct tgsi_full_instruction fi;
   struct tgsi_full_declaration fd;

   Function* shader = mod->getFunction("execute_shader");
   shader->setName("execute_shader_2");

   Function::arg_iterator args = shader->arg_begin();
   Value *ptr_OUT = args++;
   ptr_OUT->setName("OUT");
   Value *ptr_IN = args++;
   ptr_IN->setName("IN");
   Value *ptr_CONST = args++;
   ptr_CONST->setName("CONST");

   BasicBlock *label_entry = new BasicBlock("entry", shader, 0);

   tgsi_parse_init(&parse, tokens);

   Function* func_printf = mod->getFunction("printf");
   //parse.FullHeader.Processor.Processor

   //parse.FullVersion.Version.MajorVersion
   //parse.FullVersion.Version.MinorVersion

   //parse.FullHeader.Header.HeaderSize
   //parse.FullHeader.Header.BodySize
   //parse.FullHeader.Processor.Processor

   fi = tgsi_default_full_instruction();
   fd = tgsi_default_full_declaration();
   Storage storage(label_entry, ptr_OUT, ptr_IN, ptr_CONST);
   Instructions instr(mod, label_entry);
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
         translate_instruction(mod, &storage, &instr,
                               &parse.FullToken.FullInstruction,
                               &fi);
         break;

      default:
         assert(0);
      }
   }

#if 0
   // Type Definitions
   ArrayType* ArrayTy_0 = ArrayType::get(IntegerType::get(8), 19);

   PointerType* PointerTy_1 = PointerType::get(ArrayTy_0);

   VectorType* VectorTy_4 = VectorType::get(Type::FloatTy, 4);

   PointerType* PointerTy_3 = PointerType::get(VectorTy_4);


   VectorType* VectorTy_5 = VectorType::get(IntegerType::get(32), 4);


   PointerType* PointerTy_8 = PointerType::get(IntegerType::get(8));


   // Global Variable Declarations


   GlobalVariable* gvar_array__str = new GlobalVariable(
      /*Type=*/ArrayTy_0,
      /*isConstant=*/true,
      /*Linkage=*/GlobalValue::InternalLinkage,
      /*Initializer=*/0, // has initializer, specified below
      /*Name=*/".str",
      mod);

   GlobalVariable* gvar_array__str1 = new GlobalVariable(
      /*Type=*/ArrayTy_0,
      /*isConstant=*/true,
      /*Linkage=*/GlobalValue::InternalLinkage,
      /*Initializer=*/0, // has initializer, specified below
      /*Name=*/".str1",
      mod);

   GlobalVariable* gvar_array__str2 = new GlobalVariable(
      /*Type=*/ArrayTy_0,
      /*isConstant=*/true,
      /*Linkage=*/GlobalValue::InternalLinkage,
      /*Initializer=*/0, // has initializer, specified below
      /*Name=*/".str2",
      mod);

   // Constant Definitions
   Constant* const_array_9 = ConstantArray::get("const %f %f %f %f\x0A", true);
   Constant* const_array_10 = ConstantArray::get("resul %f %f %f %f\x0A", true);
   Constant* const_array_11 = ConstantArray::get("outpu %f %f %f %f\x0A", true);
   UndefValue* const_packed_12 = UndefValue::get(VectorTy_4);
   Constant* const_packed_13 = Constant::getNullValue(VectorTy_5);
   std::vector<Constant*> const_packed_14_elems;
   ConstantInt* const_int32_15 = ConstantInt::get(APInt(32,  "1", 10));
   const_packed_14_elems.push_back(const_int32_15);
   const_packed_14_elems.push_back(const_int32_15);
   const_packed_14_elems.push_back(const_int32_15);
   const_packed_14_elems.push_back(const_int32_15);
   Constant* const_packed_14 = ConstantVector::get(VectorTy_5, const_packed_14_elems);
   std::vector<Constant*> const_packed_16_elems;
   ConstantInt* const_int32_17 = ConstantInt::get(APInt(32,  "2", 10));
   const_packed_16_elems.push_back(const_int32_17);
   const_packed_16_elems.push_back(const_int32_17);
   const_packed_16_elems.push_back(const_int32_17);
   const_packed_16_elems.push_back(const_int32_17);
   Constant* const_packed_16 = ConstantVector::get(VectorTy_5, const_packed_16_elems);
   std::vector<Constant*> const_packed_18_elems;
   ConstantInt* const_int32_19 = ConstantInt::get(APInt(32,  "3", 10));
   const_packed_18_elems.push_back(const_int32_19);
   const_packed_18_elems.push_back(const_int32_19);
   const_packed_18_elems.push_back(const_int32_19);
   const_packed_18_elems.push_back(const_int32_19);
   Constant* const_packed_18 = ConstantVector::get(VectorTy_5, const_packed_18_elems);
   std::vector<Constant*> const_ptr_20_indices;
   Constant* const_int32_21 = Constant::getNullValue(IntegerType::get(32));
   const_ptr_20_indices.push_back(const_int32_21);
   const_ptr_20_indices.push_back(const_int32_21);
   Constant* const_ptr_20 = ConstantExpr::getGetElementPtr(gvar_array__str, &const_ptr_20_indices[0], const_ptr_20_indices.size() );
   UndefValue* const_double_22 = UndefValue::get(Type::DoubleTy);
   std::vector<Constant*> const_ptr_23_indices;
   const_ptr_23_indices.push_back(const_int32_21);
   const_ptr_23_indices.push_back(const_int32_21);
   Constant* const_ptr_23 = ConstantExpr::getGetElementPtr(gvar_array__str1, &const_ptr_23_indices[0], const_ptr_23_indices.size() );
   std::vector<Constant*> const_ptr_24_indices;
   const_ptr_24_indices.push_back(const_int32_21);
   const_ptr_24_indices.push_back(const_int32_21);
   Constant* const_ptr_24 = ConstantExpr::getGetElementPtr(gvar_array__str2, &const_ptr_24_indices[0], const_ptr_24_indices.size() );

   // Global Variable Definitions
   gvar_array__str->setInitializer(const_array_9);
   gvar_array__str1->setInitializer(const_array_10);
   gvar_array__str2->setInitializer(const_array_11);

   // Function Definitions

   // Function: execute_shader (func_execute_shader)
   {
      // Block entry (label_entry)
      LoadInst* packed_tmp1 = new LoadInst(ptr_IN, "tmp1", false, label_entry);
      ShuffleVectorInst* packed_tmp3 = new ShuffleVectorInst(packed_tmp1, const_packed_12, const_packed_13, "tmp3", label_entry);
      LoadInst* packed_tmp6 = new LoadInst(ptr_CONST, "tmp6", false, label_entry);
      BinaryOperator* packed_mul = BinaryOperator::create(Instruction::Mul, packed_tmp3, packed_tmp6, "mul", label_entry);
      ShuffleVectorInst* packed_tmp8 = new ShuffleVectorInst(packed_tmp1, const_packed_12, const_packed_14, "tmp8", label_entry);
      GetElementPtrInst* ptr_arrayidx10 = new GetElementPtrInst(ptr_CONST, const_int32_15, "arrayidx10", label_entry);
      LoadInst* packed_tmp11 = new LoadInst(ptr_arrayidx10, "tmp11", false, label_entry);
      BinaryOperator* packed_mul12 = BinaryOperator::create(Instruction::Mul, packed_tmp8, packed_tmp11, "mul12", label_entry);
      BinaryOperator* packed_add = BinaryOperator::create(Instruction::Add, packed_mul12, packed_mul, "add", label_entry);
      ShuffleVectorInst* packed_tmp15 = new ShuffleVectorInst(packed_tmp1, const_packed_12, const_packed_16, "tmp15", label_entry);
      GetElementPtrInst* ptr_arrayidx17 = new GetElementPtrInst(ptr_CONST, const_int32_17, "arrayidx17", label_entry);
      LoadInst* packed_tmp18 = new LoadInst(ptr_arrayidx17, "tmp18", false, label_entry);
      BinaryOperator* packed_mul19 = BinaryOperator::create(Instruction::Mul, packed_tmp15, packed_tmp18, "mul19", label_entry);
      BinaryOperator* packed_add21 = BinaryOperator::create(Instruction::Add, packed_mul19, packed_add, "add21", label_entry);
      ShuffleVectorInst* packed_tmp25 = new ShuffleVectorInst(packed_tmp1, const_packed_12, const_packed_18, "tmp25", label_entry);
      GetElementPtrInst* ptr_arrayidx27 = new GetElementPtrInst(ptr_CONST, const_int32_19, "arrayidx27", label_entry);
      LoadInst* packed_tmp28 = new LoadInst(ptr_arrayidx27, "tmp28", false, label_entry);
      BinaryOperator* packed_mul29 = BinaryOperator::create(Instruction::Mul, packed_tmp25, packed_tmp28, "mul29", label_entry);
      BinaryOperator* packed_add31 = BinaryOperator::create(Instruction::Add, packed_mul29, packed_add21, "add31", label_entry);
      StoreInst* void_25 = new StoreInst(packed_add31, ptr_OUT, false, label_entry);
      GetElementPtrInst* ptr_arrayidx33 = new GetElementPtrInst(ptr_OUT, const_int32_15, "arrayidx33", label_entry);
      GetElementPtrInst* ptr_arrayidx35 = new GetElementPtrInst(ptr_IN, const_int32_15, "arrayidx35", label_entry);
      LoadInst* packed_tmp36 = new LoadInst(ptr_arrayidx35, "tmp36", false, label_entry);
      StoreInst* void_26 = new StoreInst(packed_tmp36, ptr_arrayidx33, false, label_entry);
      std::vector<Value*> int32_call_params;
      int32_call_params.push_back(const_ptr_20);
      int32_call_params.push_back(const_double_22);
      int32_call_params.push_back(const_double_22);
      int32_call_params.push_back(const_double_22);
      int32_call_params.push_back(const_double_22);
      //CallInst* int32_call = new CallInst(func_printf, int32_call_params.begin(), int32_call_params.end(), "call", label_entry);
      //int32_call->setCallingConv(CallingConv::C);
      //int32_call->setTailCall(true);
      ExtractElementInst* float_tmp52 = new ExtractElementInst(packed_tmp1, const_int32_21, "tmp52", label_entry);
      CastInst* double_conv53 = new FPExtInst(float_tmp52, Type::DoubleTy, "conv53", label_entry);
      ExtractElementInst* float_tmp55 = new ExtractElementInst(packed_tmp1, const_int32_15, "tmp55", label_entry);
      CastInst* double_conv56 = new FPExtInst(float_tmp55, Type::DoubleTy, "conv56", label_entry);
      ExtractElementInst* float_tmp58 = new ExtractElementInst(packed_tmp1, const_int32_17, "tmp58", label_entry);
      CastInst* double_conv59 = new FPExtInst(float_tmp58, Type::DoubleTy, "conv59", label_entry);
      ExtractElementInst* float_tmp61 = new ExtractElementInst(packed_tmp1, const_int32_19, "tmp61", label_entry);
      CastInst* double_conv62 = new FPExtInst(float_tmp61, Type::DoubleTy, "conv62", label_entry);
      std::vector<Value*> int32_call63_params;
      int32_call63_params.push_back(const_ptr_23);
      int32_call63_params.push_back(double_conv53);
      int32_call63_params.push_back(double_conv56);
      int32_call63_params.push_back(double_conv59);
      int32_call63_params.push_back(double_conv62);
      //CallInst* int32_call63 = new CallInst(func_printf, int32_call63_params.begin(), int32_call63_params.end(), "call63", label_entry);
      //int32_call63->setCallingConv(CallingConv::C);
      //int32_call63->setTailCall(true);
      ExtractElementInst* float_tmp65 = new ExtractElementInst(packed_add31, const_int32_21, "tmp65", label_entry);
      CastInst* double_conv66 = new FPExtInst(float_tmp65, Type::DoubleTy, "conv66", label_entry);
      ExtractElementInst* float_tmp68 = new ExtractElementInst(packed_add31, const_int32_15, "tmp68", label_entry);
      CastInst* double_conv69 = new FPExtInst(float_tmp68, Type::DoubleTy, "conv69", label_entry);
      ExtractElementInst* float_tmp71 = new ExtractElementInst(packed_add31, const_int32_17, "tmp71", label_entry);
      CastInst* double_conv72 = new FPExtInst(float_tmp71, Type::DoubleTy, "conv72", label_entry);
      ExtractElementInst* float_tmp74 = new ExtractElementInst(packed_add31, const_int32_19, "tmp74", label_entry);
      CastInst* double_conv75 = new FPExtInst(float_tmp74, Type::DoubleTy, "conv75", label_entry);
      std::vector<Value*> int32_call76_params;
      int32_call76_params.push_back(const_ptr_24);
      int32_call76_params.push_back(double_conv66);
      int32_call76_params.push_back(double_conv69);
      int32_call76_params.push_back(double_conv72);
      int32_call76_params.push_back(double_conv75);
      //CallInst* int32_call76 = new CallInst(func_printf, int32_call76_params.begin(), int32_call76_params.end(), "call76", label_entry);
      //int32_call76->setCallingConv(CallingConv::C);
      //int32_call76->setTailCall(true);
   }
#endif

   new ReturnInst(label_entry);

   //TXT("\ntgsi-dump end -------------------\n");

   tgsi_parse_free(&parse);

   std::cout<<"XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"<<std::endl;
   std::cout<<*mod<<std::endl;
   std::cout<<"YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY"<<std::endl;
   return mod;
}

struct ga_llvm_prog *
ga_llvm_from_tgsi(struct pipe_context *pipe, const struct tgsi_token *tokens)
{
   std::cout << "Creating llvm " <<std::endl;
   struct ga_llvm_prog *ga_llvm =
      (struct ga_llvm_prog *)malloc(sizeof(struct ga_llvm_prog));
   fprintf(stderr, "DUMPX \n");
   //tgsi_dump(tokens, TGSI_DUMP_VERBOSE);
   tgsi_dump(tokens, 0);
   fprintf(stderr, "DUMPEND \n");
   llvm::Module *mod = tgsi_to_llvm(tokens);

   /* Run optimization passes over it */
   PassManager passes;
   // Add an appropriate TargetData instance for this module...
   passes.add(new TargetData(mod));
   AddStandardCompilePasses(passes);
   std::cout<<"Running optimization passes..."<<std::endl;
   bool b = passes.run(*mod);
   std::cout<<"\tModified mod = "<<b<<std::endl;

   llvm::ExistingModuleProvider *mp =
      new llvm::ExistingModuleProvider(mod);
   llvm::ExecutionEngine *ee = 0;
   if (!pipe->llvm_execution_engine) {
      ee = llvm::ExecutionEngine::create(mp, false);
      pipe->llvm_execution_engine = ee;
   } else {
      ee = (llvm::ExecutionEngine*)pipe->llvm_execution_engine;
      ee->addModuleProvider(mp);
   }
   ga_llvm->module = mod;

   Function *func = mod->getFunction("run_vertex_shader");
   std::cout << "run_vertex_shader  = "<<func<<std::endl;
   ga_llvm->function = ee->getPointerToFunctionOrStub(func);
   std::cout << " -- FUNC is " <<ga_llvm->function<<std::endl;

   return ga_llvm;
}

void ga_llvm_prog_delete(struct ga_llvm_prog *prog)
{
   llvm::Module *mod = static_cast<llvm::Module*>(prog->module);
   delete mod;
   prog->module = 0;
   prog->engine = 0;
   prog->function = 0;
   free(prog);
}

typedef void (*vertex_shader_runner)(float (*ainputs)[PIPE_MAX_SHADER_INPUTS][4],
                                  float (*dests)[PIPE_MAX_SHADER_INPUTS][4],
                                  float (*aconsts)[4],
                                  int count,
                                  int num_attribs);

int ga_llvm_prog_exec(struct ga_llvm_prog *prog,
                      float (*inputs)[PIPE_MAX_SHADER_INPUTS][4],
                      float (*dests)[PIPE_MAX_SHADER_INPUTS][4],
                      float (*consts)[4],
                      int count,
                      int num_attribs)
{
   std::cout << "---- START LLVM Execution "<<std::endl;
   vertex_shader_runner runner = reinterpret_cast<vertex_shader_runner>(prog->function);
   runner(inputs, dests, consts, count, num_attribs);

   std::cout << "---- END LLVM Execution "<<std::endl;

   for (int i = 0; i < count; ++i) {
      for (int j = 0; j < num_attribs; ++j) {
         printf("OUT(%d, %d) [%f, %f, %f, %f]\n", i, j,
                dests[i][j][0], dests[i][j][1],
                dests[i][j][2], dests[i][j][3]);
      }
   }
   return 0;
}
