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

#include "util/u_memory.h"

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

struct gallivm_cpu_engine {
   llvm::ExecutionEngine *engine;
};

static struct gallivm_cpu_engine *CPU = 0;

typedef int (*fragment_shader_runner)(float x, float y,
                                      float (*dests)[16][4],
                                      float (*inputs)[16][4],
                                      int num_attribs,
                                      float (*consts)[4], int num_consts,
                                      struct tgsi_sampler *samplers);

int gallivm_cpu_fs_exec(struct gallivm_prog *prog,
                        float fx, float fy,
                        float (*dests)[16][4],
                        float (*inputs)[16][4],
                        float (*consts)[4],
                        struct tgsi_sampler *samplers)
{
   fragment_shader_runner runner = reinterpret_cast<fragment_shader_runner>(prog->function);
   assert(runner);

   return runner(fx, fy, dests, inputs, prog->num_interp,
                 consts, prog->num_consts,
                 samplers);
}

static inline llvm::Function *func_for_shader(struct gallivm_prog *prog)
{
   llvm::Module *mod = prog->module;
   llvm::Function *func = 0;

   switch (prog->type) {
   case GALLIVM_VS:
      func = mod->getFunction("vs_shader");
      break;
   case GALLIVM_FS:
      func = mod->getFunction("fs_shader");
      break;
   default:
      assert(!"Unknown shader type!");
      break;
   }
   return func;
}

/*!
  This function creates a CPU based execution engine for the given gallivm_prog.
  gallivm_cpu_engine should be used as a singleton throughout the library. Before
  executing gallivm_prog_exec one needs to call gallivm_cpu_jit_compile.
  The gallivm_prog instance which is being passed to the constructor is being
  automatically JIT compiled so one shouldn't call gallivm_cpu_jit_compile
  with it again.
 */
struct gallivm_cpu_engine * gallivm_cpu_engine_create(struct gallivm_prog *prog)
{
   struct gallivm_cpu_engine *cpu = (struct gallivm_cpu_engine *)
                                    calloc(1, sizeof(struct gallivm_cpu_engine));
   llvm::Module *mod = static_cast<llvm::Module*>(prog->module);
   llvm::ExistingModuleProvider *mp = new llvm::ExistingModuleProvider(mod);
   llvm::ExecutionEngine *ee = llvm::ExecutionEngine::create(mp, false);
   ee->DisableLazyCompilation();
   cpu->engine = ee;

   llvm::Function *func = func_for_shader(prog);

   prog->function = ee->getPointerToFunction(func);
   CPU = cpu;
   return cpu;
}


/*!
  This function JIT compiles the given gallivm_prog with the given cpu based execution engine.
  The reference to the generated machine code entry point will be stored
  in the gallivm_prog program. After executing this function one can call gallivm_prog_exec
  in order to execute the gallivm_prog on the CPU.
 */
void gallivm_cpu_jit_compile(struct gallivm_cpu_engine *cpu, struct gallivm_prog *prog)
{
   llvm::Module *mod = static_cast<llvm::Module*>(prog->module);
   llvm::ExistingModuleProvider *mp = new llvm::ExistingModuleProvider(mod);
   llvm::ExecutionEngine *ee = cpu->engine;
   assert(ee);
   /*FIXME : remove */
   ee->DisableLazyCompilation();
   ee->addModuleProvider(mp);

   llvm::Function *func = func_for_shader(prog);
   prog->function = ee->getPointerToFunction(func);
}

void gallivm_cpu_engine_delete(struct gallivm_cpu_engine *cpu)
{
   free(cpu);
}

struct gallivm_cpu_engine * gallivm_global_cpu_engine()
{
   return CPU;
}


typedef void (*vertex_shader_runner)(void *ainputs,
                                     void *dests,
                                     float (*aconsts)[4],
                                     void *temps);

#define MAX_TGSI_VERTICES 4
/*!
  This function is used to execute the gallivm_prog in software. Before calling
  this function the gallivm_prog has to be JIT compiled with the gallivm_cpu_jit_compile
  function.
 */
int gallivm_cpu_vs_exec(struct gallivm_prog *prog,
                        struct tgsi_exec_machine *machine,
                        const float (*input)[4],
                        unsigned num_inputs,
                        float (*output)[4],
                        unsigned num_outputs,
                        const float (*constants)[4],
                        unsigned count,
                        unsigned input_stride,
                        unsigned output_stride )
{
   unsigned int i, j;
   unsigned slot;
   vertex_shader_runner runner = reinterpret_cast<vertex_shader_runner>(prog->function);

   assert(runner);

   for (i = 0; i < count; i += MAX_TGSI_VERTICES) {
      unsigned int max_vertices = MIN2(MAX_TGSI_VERTICES, count - i);

      /* Swizzle inputs.
       */
      for (j = 0; j < max_vertices; j++) {
	 for (slot = 0; slot < num_inputs; slot++) {
	    machine->Inputs[slot].xyzw[0].f[j] = input[slot][0];
	    machine->Inputs[slot].xyzw[1].f[j] = input[slot][1];
	    machine->Inputs[slot].xyzw[2].f[j] = input[slot][2];
	    machine->Inputs[slot].xyzw[3].f[j] = input[slot][3];
	 }

	 input = (const float (*)[4])((const char *)input + input_stride);
      }

      /* run shader */
      runner(machine->Inputs,
             machine->Outputs,
             (float (*)[4]) constants,
             machine->Temps);

      /* Unswizzle all output results
       */
      for (j = 0; j < max_vertices; j++) {
         for (slot = 0; slot < num_outputs; slot++) {
            output[slot][0] = machine->Outputs[slot].xyzw[0].f[j];
            output[slot][1] = machine->Outputs[slot].xyzw[1].f[j];
            output[slot][2] = machine->Outputs[slot].xyzw[2].f[j];
            output[slot][3] = machine->Outputs[slot].xyzw[3].f[j];
         }
         output = (float (*)[4])((char *)output + output_stride);
      }
   }

   return 0;
}

#endif
