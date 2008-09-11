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
#ifdef MESA_LLVM

#include "gallivm.h"
#include "gallivm_p.h"

#include "instructions.h"
#include "loweringpass.h"
#include "storage.h"
#include "tgsitollvm.h"

#include "pipe/p_context.h"
#include "pipe/p_shader_tokens.h"

#include "tgsi/tgsi_exec.h"
#include "tgsi/tgsi_dump.h"

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
#include <llvm/Transforms/Utils/Cloning.h>

#include <sstream>
#include <fstream>
#include <iostream>

static int GLOBAL_ID = 0;

using namespace llvm;

static inline
void AddStandardCompilePasses(PassManager &PM)
{
   PM.add(new LoweringPass());
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

void gallivm_prog_delete(struct gallivm_prog *prog)
{
   delete prog->module;
   prog->module = 0;
   prog->function = 0;
   free(prog);
}

static inline void
constant_interpolation(float (*inputs)[16][4],
                       const struct tgsi_interp_coef *coefs,
                       unsigned attrib,
                       unsigned chan)
{
   unsigned i;

   for (i = 0; i < QUAD_SIZE; ++i) {
      inputs[i][attrib][chan] = coefs[attrib].a0[chan];
   }
}

static inline void
linear_interpolation(float (*inputs)[16][4],
                     const struct tgsi_interp_coef *coefs,
                     unsigned attrib,
                     unsigned chan)
{
   unsigned i;

   for( i = 0; i < QUAD_SIZE; i++ ) {
      const float x = inputs[i][0][0];
      const float y = inputs[i][0][1];

      inputs[i][attrib][chan] =
         coefs[attrib].a0[chan] +
         coefs[attrib].dadx[chan] * x +
         coefs[attrib].dady[chan] * y;
   }
}

static inline void
perspective_interpolation(float (*inputs)[16][4],
                          const struct tgsi_interp_coef *coefs,
                          unsigned attrib,
                          unsigned chan )
{
   unsigned i;

   for( i = 0; i < QUAD_SIZE; i++ ) {
      const float x = inputs[i][0][0];
      const float y = inputs[i][0][1];
      /* WPOS.w here is really 1/w */
      const float w = 1.0f / inputs[i][0][3];
      assert(inputs[i][0][3] != 0.0);

      inputs[i][attrib][chan] =
         (coefs[attrib].a0[chan] +
          coefs[attrib].dadx[chan] * x +
          coefs[attrib].dady[chan] * y) * w;
   }
}

void gallivm_ir_dump(struct gallivm_ir *ir, const char *file_prefix)
{
   if (!ir || !ir->module)
      return;

   if (file_prefix) {
      std::ostringstream stream;
      stream << file_prefix;
      stream << ir->id;
      stream << ".ll";
      std::string name = stream.str();
      std::ofstream out(name.c_str());
      if (!out) {
         std::cerr<<"Can't open file : "<<stream.str()<<std::endl;;
         return;
      }
      out << (*ir->module);
      out.close();
   } else {
      const llvm::Module::FunctionListType &funcs = ir->module->getFunctionList();
      llvm::Module::FunctionListType::const_iterator itr;
      std::cout<<"; ---------- Start shader "<<ir->id<<std::endl;
      for (itr = funcs.begin(); itr != funcs.end(); ++itr) {
         const llvm::Function &func = (*itr);
         std::string name = func.getName();
         const llvm::Function *found = 0;
         if (name.find("vs_shader") != std::string::npos ||
             name.find("fs_shader") != std::string::npos ||
             name.find("function") != std::string::npos)
            found = &func;
         if (found) {
            std::cout<<*found<<std::endl;
         }
      }
      std::cout<<"; ---------- End shader "<<ir->id<<std::endl;
   }
}


void gallivm_prog_inputs_interpolate(struct gallivm_prog *prog,
                                     float (*inputs)[16][4],
                                     const struct tgsi_interp_coef *coef)
{
   for (int i = 0; i < prog->num_interp; ++i) {
      const gallivm_interpolate &interp = prog->interpolators[i];
      switch (interp.type) {
      case TGSI_INTERPOLATE_CONSTANT:
         constant_interpolation(inputs, coef, interp.attrib, interp.chan);
         break;

      case TGSI_INTERPOLATE_LINEAR:
         linear_interpolation(inputs, coef, interp.attrib, interp.chan);
         break;

      case TGSI_INTERPOLATE_PERSPECTIVE:
         perspective_interpolation(inputs, coef, interp.attrib, interp.chan);
         break;

      default:
         assert( 0 );
      }
   }
}


struct gallivm_ir * gallivm_ir_new(enum gallivm_shader_type type)
{
   struct gallivm_ir *ir =
      (struct gallivm_ir *)calloc(1, sizeof(struct gallivm_ir));
   ++GLOBAL_ID;
   ir->id   = GLOBAL_ID;
   ir->type = type;

   return ir;
}

void gallivm_ir_set_layout(struct gallivm_ir *ir,
                           enum gallivm_vector_layout layout)
{
   ir->layout = layout;
}

void gallivm_ir_set_components(struct gallivm_ir *ir, int num)
{
   ir->num_components = num;
}

void gallivm_ir_fill_from_tgsi(struct gallivm_ir *ir,
                               const struct tgsi_token *tokens)
{
   std::cout << "Creating llvm from: " <<std::endl;
   tgsi_dump(tokens, 0);

   llvm::Module *mod = tgsi_to_llvmir(ir, tokens);
   ir->module = mod;
   gallivm_ir_dump(ir, 0);
}

void gallivm_ir_delete(struct gallivm_ir *ir)
{
   delete ir->module;
   free(ir);
}

struct gallivm_prog * gallivm_ir_compile(struct gallivm_ir *ir)
{
   struct gallivm_prog *prog =
      (struct gallivm_prog *)calloc(1, sizeof(struct gallivm_prog));

   std::cout << "Before optimizations:"<<std::endl;
   ir->module->dump();
   std::cout<<"-------------------------------"<<std::endl;

   PassManager veri;
   veri.add(createVerifierPass());
   veri.run(*ir->module);
   llvm::Module *mod = llvm::CloneModule(ir->module);
   prog->num_consts = ir->num_consts;
   memcpy(prog->interpolators, ir->interpolators, sizeof(prog->interpolators));
   prog->num_interp = ir->num_interp;

   /* Run optimization passes over it */
   PassManager passes;
   passes.add(new TargetData(mod));
   AddStandardCompilePasses(passes);
   passes.run(*mod);
   prog->module = mod;

   std::cout << "After optimizations:"<<std::endl;
   mod->dump();

   return prog;
}

#endif /* MESA_LLVM */
