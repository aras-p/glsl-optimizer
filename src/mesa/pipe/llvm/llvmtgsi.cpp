/**************************************************************************
 *
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

 /*
  * Authors:
  *   Zack Rusin zack@tungstengraphics.com
  */

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

#include <sstream>
#include <fstream>
#include <iostream>

#ifdef MESA_LLVM

struct gallivm_prog {
   void *module;
   void *function;
   int   num_consts;
   int   id;
};

using namespace llvm;
#include "llvm_base_shader.cpp"

static int GLOBAL_ID = 0;

static inline void AddStandardCompilePasses(PassManager &PM) {
   PM.add(createVerifierPass());                  // Verify that input is correct

   PM.add(createLowerSetJmpPass());          // Lower llvm.setjmp/.longjmp

   //PM.add(createStripSymbolsPass(true));

   PM.add(createRaiseAllocationsPass());     // call %malloc -> malloc inst
   PM.add(createCFGSimplificationPass());    // Clean up disgusting code
   PM.add(createPromoteMemoryToRegisterPass());// Kill useless allocas
   PM.add(createGlobalOptimizerPass());      // Optimize out global vars
   PM.add(createGlobalDCEPass());            // Remove unused fns and globs
   PM.add(createIPConstantPropagationPass());// IP Constant Propagation
   PM.add(createDeadArgEliminationPass());   // Dead argument elimination
   PM.add(createInstructionCombiningPass()); // Clean up after IPCP & DAE
   PM.add(createCFGSimplificationPass());    // Clean up after IPCP & DAE

   PM.add(createPruneEHPass());              // Remove dead EH info

   PM.add(createFunctionInliningPass());   // Inline small functions
   PM.add(createArgumentPromotionPass());    // Scalarize uninlined fn args

   PM.add(createTailDuplicationPass());      // Simplify cfg by copying code
   PM.add(createInstructionCombiningPass()); // Cleanup for scalarrepl.
   PM.add(createCFGSimplificationPass());    // Merge & remove BBs
   PM.add(createScalarReplAggregatesPass()); // Break up aggregate allocas
   PM.add(createInstructionCombiningPass()); // Combine silly seq's
   PM.add(createCondPropagationPass());      // Propagate conditionals

   PM.add(createTailCallEliminationPass());  // Eliminate tail calls
   PM.add(createCFGSimplificationPass());    // Merge & remove BBs
   PM.add(createReassociatePass());          // Reassociate expressions
   PM.add(createLoopRotatePass());
   PM.add(createLICMPass());                 // Hoist loop invariants
   PM.add(createLoopUnswitchPass());         // Unswitch loops.
   PM.add(createLoopIndexSplitPass());       // Index split loops.
   PM.add(createInstructionCombiningPass()); // Clean up after LICM/reassoc
   PM.add(createIndVarSimplifyPass());       // Canonicalize indvars
   PM.add(createLoopUnrollPass());           // Unroll small loops
   PM.add(createInstructionCombiningPass()); // Clean up after the unroller
   PM.add(createGVNPass());                  // Remove redundancies
   PM.add(createSCCPPass());                 // Constant prop with SCCP

   // Run instcombine after redundancy elimination to exploit opportunities
   // opened up by them.
   PM.add(createInstructionCombiningPass());
   PM.add(createCondPropagationPass());      // Propagate conditionals

   PM.add(createDeadStoreEliminationPass()); // Delete dead stores
   PM.add(createAggressiveDCEPass());        // SSA based 'Aggressive DCE'
   PM.add(createCFGSimplificationPass());    // Merge & remove BBs
   PM.add(createSimplifyLibCallsPass());     // Library Call Optimizations
   PM.add(createDeadTypeEliminationPass());  // Eliminate dead types
   PM.add(createConstantMergePass());        // Merge dup global constants
}

static void
translate_declaration(llvm::Module *module,
                      Storage *storage,
                      struct tgsi_full_declaration *decl,
                      struct tgsi_full_declaration *fd)
{
   if (decl->Declaration.File == TGSI_FILE_TEMPORARY) {
      switch( decl->Declaration.Declare ) {
      case TGSI_DECLARE_RANGE: {
         int start = decl->u.DeclarationRange.First;
         int end   = decl->u.DeclarationRange.Last;
         for (int i = start; i <= end; ++i) {
            storage->declareTemp(i);
         }
      }
         break;
      default:
         assert( 0 );
      }
   }
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
   inputs[0] = 0;
   inputs[1] = 0;
   inputs[2] = 0;
   inputs[3] = 0;

   for (int i = 0; i < inst->Instruction.NumSrcRegs; ++i) {
      struct tgsi_full_src_register *src = &inst->FullSrcRegisters[i];
      llvm::Value *val = 0;
      llvm::Value *indIdx = 0;

      if (src->SrcRegister.Indirect) {
         indIdx = storage->addrElement(src->SrcRegisterInd.Index);
         indIdx = storage->extractIndex(indIdx);
      }
      if (src->SrcRegister.File == TGSI_FILE_CONSTANT) {
         val = storage->constElement(src->SrcRegister.Index, indIdx);
      } else if (src->SrcRegister.File == TGSI_FILE_INPUT) {
         val = storage->inputElement(src->SrcRegister.Index, indIdx);
      } else if (src->SrcRegister.File == TGSI_FILE_TEMPORARY) {
         val = storage->tempElement(src->SrcRegister.Index);
      } else if (src->SrcRegister.File == TGSI_FILE_TEMPORARY) {
         fprintf(stderr, "FIXME: do somethign with immediates?\n");
      } else {
         fprintf(stderr, "ERROR: not supported llvm source %d\n", src->SrcRegister.File);
         return;
      }

      if (src->SrcRegister.Extended) {
         if (src->SrcRegisterExtSwz.ExtSwizzleX != TGSI_EXTSWIZZLE_X ||
             src->SrcRegisterExtSwz.ExtSwizzleY != TGSI_EXTSWIZZLE_Y ||
             src->SrcRegisterExtSwz.ExtSwizzleZ != TGSI_EXTSWIZZLE_Z ||
             src->SrcRegisterExtSwz.ExtSwizzleW != TGSI_EXTSWIZZLE_W) {
            int swizzle = 0;

            if (src->SrcRegisterExtSwz.ExtSwizzleX != TGSI_EXTSWIZZLE_X)
               swizzle = src->SrcRegisterExtSwz.ExtSwizzleX * 1000;
            else
               swizzle = src->SrcRegister.SwizzleX * 1000;
            if (src->SrcRegisterExtSwz.ExtSwizzleY != TGSI_EXTSWIZZLE_Y)
               swizzle += src->SrcRegisterExtSwz.ExtSwizzleY * 100;
            else
               swizzle += src->SrcRegister.SwizzleY  * 100;
            if (src->SrcRegisterExtSwz.ExtSwizzleZ != TGSI_EXTSWIZZLE_Z)
               swizzle += src->SrcRegisterExtSwz.ExtSwizzleZ * 10;
            else
               swizzle += src->SrcRegister.SwizzleZ  * 10;
            if (src->SrcRegisterExtSwz.ExtSwizzleW != TGSI_EXTSWIZZLE_W)
               swizzle += src->SrcRegisterExtSwz.ExtSwizzleW * 1;
            else
               swizzle += src->SrcRegister.SwizzleW  * 1;
            /*fprintf(stderr, "EXT XXXXXXXX swizzle x = %d\n", swizzle);*/

            val = storage->shuffleVector(val, swizzle);
         }
      } else if (src->SrcRegister.SwizzleX != TGSI_SWIZZLE_X ||
                 src->SrcRegister.SwizzleY != TGSI_SWIZZLE_Y ||
                 src->SrcRegister.SwizzleZ != TGSI_SWIZZLE_Z ||
                 src->SrcRegister.SwizzleW != TGSI_SWIZZLE_W) {
         int swizzle = src->SrcRegister.SwizzleX * 1000;
         swizzle += src->SrcRegister.SwizzleY  * 100;
         swizzle += src->SrcRegister.SwizzleZ  * 10;
         swizzle += src->SrcRegister.SwizzleW  * 1;
         /*fprintf(stderr, "XXXXXXXX swizzle = %d\n", swizzle);*/
         val = storage->shuffleVector(val, swizzle);
      }
      inputs[i] = val;
   }

   /*if (inputs[0])
     instr->printVector(inputs[0]);
   if (inputs[1])
   instr->printVector(inputs[1]);*/
   llvm::Value *out = 0;
   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_ARL: {
      out = instr->arl(inputs[0]);
   }
      break;
   case TGSI_OPCODE_MOV: {
      out = inputs[0];
   }
      break;
   case TGSI_OPCODE_LIT: {
      out = instr->lit(inputs[0]);
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
   case TGSI_OPCODE_MIN: {
      out = instr->min(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_MAX: {
      out = instr->max(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_SLT: {
      out = instr->slt(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_SGE: {
      out = instr->sge(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_MAD: {
      out = instr->madd(inputs[0], inputs[1], inputs[2]);
   }
      break;
   case TGSI_OPCODE_SUB: {
      out = instr->sub(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_LERP: {
      out = instr->lerp(inputs[0], inputs[1], inputs[2]);
   }
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
   case TGSI_OPCODE_FRAC: {
      out = instr->frc(inputs[0]);
   }
      break;
   case TGSI_OPCODE_CLAMP:
      break;
   case TGSI_OPCODE_FLOOR: {
      out = instr->floor(inputs[0]);
   }
      break;
   case TGSI_OPCODE_ROUND:
      break;
   case TGSI_OPCODE_EXPBASE2: {
      out = instr->ex2(inputs[0]);
   }
      break;
   case TGSI_OPCODE_LOGBASE2: {
      out = instr->lg2(inputs[0]);
   }
      break;
   case TGSI_OPCODE_POWER: {
      out = instr->pow(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_CROSSPRODUCT: {
      out = instr->cross(inputs[0], inputs[1]);
   }
      break;
   case TGSI_OPCODE_MULTIPLYMATRIX:
      break;
   case TGSI_OPCODE_ABS: {
      out = instr->abs(inputs[0]);
   }
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
   case TGSI_OPCODE_SGT: {
      out = instr->sgt(inputs[0], inputs[1]);
   }
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
   case TGSI_OPCODE_IF: {
      instr->ifop(inputs[0]);
      storage->setCurrentBlock(instr->currentBlock());
      return;  //just update the state
   }
      break;
   case TGSI_OPCODE_LOOP:
      break;
   case TGSI_OPCODE_REP:
      break;
   case TGSI_OPCODE_ELSE:
      break;
   case TGSI_OPCODE_ENDIF: {
      instr->endif();
      storage->setCurrentBlock(instr->currentBlock());
      return; //just update the state
   }
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
         storage->store(dst->DstRegister.Index, out, dst->DstRegister.WriteMask);
      } else if (dst->DstRegister.File == TGSI_FILE_TEMPORARY) {
         storage->setTempElement(dst->DstRegister.Index, out, dst->DstRegister.WriteMask);
      } else if (dst->DstRegister.File == TGSI_FILE_ADDRESS) {
         storage->setAddrElement(dst->DstRegister.Index, out, dst->DstRegister.WriteMask);
      } else {
         fprintf(stderr, "ERROR: unsupported LLVM destination!");
         assert(!"wrong destination");
      }
   }
}

static llvm::Module *
tgsi_to_llvm(struct gallivm_prog *prog, const struct tgsi_token *tokens)
{
   llvm::Module *mod = createBaseShader();
   struct tgsi_parse_context parse;
   struct tgsi_full_instruction fi;
   struct tgsi_full_declaration fd;

   Function* shader = mod->getFunction("execute_shader");
   std::ostringstream stream;
   stream << "execute_shader";
   stream << prog->id;
   std::string func_name = stream.str();
   shader->setName(func_name.c_str());

   Function::arg_iterator args = shader->arg_begin();
   Value *ptr_OUT = args++;
   ptr_OUT->setName("OUT");
   Value *ptr_IN = args++;
   ptr_IN->setName("IN");
   Value *ptr_CONST = args++;
   ptr_CONST->setName("CONST");

   BasicBlock *label_entry = new BasicBlock("entry", shader, 0);

   tgsi_parse_init(&parse, tokens);

   fi = tgsi_default_full_instruction();
   fd = tgsi_default_full_declaration();
   Storage storage(label_entry, ptr_OUT, ptr_IN, ptr_CONST);
   Instructions instr(mod, shader, label_entry);
   while(!tgsi_parse_end_of_tokens(&parse)) {
      tgsi_parse_token(&parse);

      switch (parse.FullToken.Token.Type) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         translate_declaration(mod, &storage,
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

   new ReturnInst(instr.currentBlock());

   tgsi_parse_free(&parse);

   prog->num_consts = storage.numConsts();
   return mod;
}

struct gallivm_prog *
gallivm_from_tgsi(struct pipe_context *pipe, const struct tgsi_token *tokens)
{
   std::cout << "Creating llvm from: " <<std::endl;
   ++GLOBAL_ID;
   struct gallivm_prog *gallivm =
      (struct gallivm_prog *)malloc(sizeof(struct gallivm_prog));
   gallivm->id = GLOBAL_ID;
   tgsi_dump(tokens, 0);

   llvm::Module *mod = tgsi_to_llvm(gallivm, tokens);
   gallivm->module = mod;
   gallivm_prog_dump(gallivm, 0);

   /* Run optimization passes over it */
   PassManager passes;
   passes.add(new TargetData(mod));
   AddStandardCompilePasses(passes);
   passes.run(*mod);

   llvm::ExistingModuleProvider *mp = new llvm::ExistingModuleProvider(mod);
   llvm::ExecutionEngine *ee = 0;
   if (!pipe->llvm_execution_engine) {
      ee = llvm::ExecutionEngine::create(mp, false);
      pipe->llvm_execution_engine = ee;
   } else {
      ee = (llvm::ExecutionEngine*)pipe->llvm_execution_engine;
      ee->addModuleProvider(mp);
   }
   gallivm->module = mod;

   Function *func = mod->getFunction("run_vertex_shader");
   gallivm->function = ee->getPointerToFunctionOrStub(func);

   gallivm_prog_dump(gallivm, 0);

   return gallivm;
}

void gallivm_prog_delete(struct gallivm_prog *prog)
{
   llvm::Module *mod = static_cast<llvm::Module*>(prog->module);
   delete mod;
   prog->module = 0;
   prog->function = 0;
   free(prog);
}

typedef void (*vertex_shader_runner)(float (*ainputs)[PIPE_MAX_SHADER_INPUTS][4],
                                     float (*dests)[PIPE_MAX_SHADER_INPUTS][4],
                                     float (*aconsts)[4],
                                     int num_vertices,
                                     int num_inputs,
                                     int num_attribs,
                                     int num_consts);

int gallivm_prog_exec(struct gallivm_prog *prog,
                      float (*inputs)[PIPE_MAX_SHADER_INPUTS][4],
                      float (*dests)[PIPE_MAX_SHADER_INPUTS][4],
                      float (*consts)[4],
                      int num_vertices,
                      int num_inputs,
                      int num_attribs)
{
   vertex_shader_runner runner = reinterpret_cast<vertex_shader_runner>(prog->function);
   runner(inputs, dests, consts, num_vertices, num_inputs,
          num_attribs, prog->num_consts);

   return 0;
}

void gallivm_prog_dump(struct gallivm_prog *prog, const char *file_prefix)
{
   llvm::Module *mod;
   if (!prog || !prog->module)
      return;

   mod = static_cast<llvm::Module*>(prog->module);

   if (file_prefix) {
      std::ostringstream stream;
      stream << file_prefix;
      stream << prog->id;
      stream << ".ll";
      std::string name = stream.str();
      std::ofstream out(name.c_str());
      if (!out) {
         std::cerr<<"Can't open file : "<<stream.str()<<std::endl;;
         return;
      }
      out << (*mod);
      out.close();
   } else {
      std::ostringstream stream;
      stream << "execute_shader";
      stream << prog->id;
      std::string func_name = stream.str();
      llvm::Function *func = mod->getFunction(func_name.c_str());
      assert(func);
      std::cout<<"; ---------- Start shader "<<prog->id<<std::endl;
      std::cout<<*func<<std::endl;
      std::cout<<"; ---------- End shader "<<prog->id<<std::endl;
   }
}

#endif /* MESA_LLVM */
