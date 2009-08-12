/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
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

/**
 * Execute fragment shader using LLVM code generation.
 */


#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_exec.h"
#include "tgsi/tgsi_dump.h"
#include "lp_bld_type.h"
#include "lp_bld_tgsi.h"
#include "lp_screen.h"
#include "lp_context.h"
#include "lp_state.h"
#include "lp_fs.h"
#include "lp_quad.h"


typedef void
(*lp_shader_fs_func)(void *inputs,
                     void *consts,
                     void *outputs,
                     struct tgsi_sampler **samplers);


/**
 * Subclass of lp_fragment_shader
 */
struct lp_llvm_fragment_shader
{
   struct lp_fragment_shader base;

   struct llvmpipe_screen *screen;

   LLVMValueRef function;

   lp_shader_fs_func jit_function;
};


/** cast wrapper */
static INLINE struct lp_llvm_fragment_shader *
lp_llvm_fragment_shader(const struct lp_fragment_shader *base)
{
   return (struct lp_llvm_fragment_shader *) base;
}


static void
shader_generate(struct llvmpipe_screen *screen,
                struct lp_llvm_fragment_shader *shader)
{
   const struct tgsi_token *tokens = shader->base.shader.tokens;
   union lp_type type;
   LLVMTypeRef elem_type;
   LLVMTypeRef vec_type;
   LLVMTypeRef args[4];
   LLVMValueRef inputs_ptr;
   LLVMValueRef consts_ptr;
   LLVMValueRef outputs_ptr;
   LLVMValueRef samplers_ptr;
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   LLVMValueRef inputs[PIPE_MAX_SHADER_INPUTS][4];
   LLVMValueRef outputs[PIPE_MAX_SHADER_OUTPUTS][4];
   char name[32];
   unsigned i, j;

   type.value = 0;
   type.floating = TRUE;
   type.sign = TRUE;
   type.norm = FALSE;
   type.width = 32;
   type.length = 4;

   elem_type = lp_build_elem_type(type);
   vec_type = lp_build_vec_type(type);

   args[0] = LLVMPointerType(vec_type, 0);
   args[1] = LLVMPointerType(elem_type, 0);
   args[2] = LLVMPointerType(vec_type, 0);
   args[3] = LLVMPointerType(LLVMInt8Type(), 0);
   shader->function = LLVMAddFunction(screen->module, "shader", LLVMFunctionType(LLVMVoidType(), args, 4, 0));
   LLVMSetFunctionCallConv(shader->function, LLVMCCallConv);

   inputs_ptr = LLVMGetParam(shader->function, 0);
   consts_ptr = LLVMGetParam(shader->function, 1);
   outputs_ptr = LLVMGetParam(shader->function, 2);
   samplers_ptr = LLVMGetParam(shader->function, 3);

   LLVMSetValueName(inputs_ptr, "inputs");
   LLVMSetValueName(consts_ptr, "consts");
   LLVMSetValueName(outputs_ptr, "outputs");
   LLVMSetValueName(samplers_ptr, "samplers");

   block = LLVMAppendBasicBlock(shader->function, "entry");
   builder = LLVMCreateBuilder();
   LLVMPositionBuilderAtEnd(builder, block);

   for(i = 0; i < PIPE_MAX_SHADER_INPUTS; ++i) {
      for(j = 0; j < 4; ++j) {
         LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), i*4 + j, 0);
         util_snprintf(name, sizeof name, "input%u.%c", i, "xywz"[j]);
         inputs[i][j] = LLVMBuildLoad(builder, LLVMBuildGEP(builder, inputs_ptr, &index, 1, ""), name);
      }
   }

   memset(outputs, 0, sizeof outputs);

   lp_build_tgsi_soa(builder, tokens, type, inputs, consts_ptr, outputs, samplers_ptr);

   for(i = 0; i < PIPE_MAX_SHADER_OUTPUTS; ++i) {
      for(j = 0; j < 4; ++j) {
         if(outputs[i][j]) {
            LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), i*4 + j, 0);
            util_snprintf(name, sizeof name, "output%u.%c", i, "xywz"[j]);
            LLVMBuildStore(builder, outputs[i][j], LLVMBuildGEP(builder, outputs_ptr, &index, 1, name));
         }
      }
   }

   LLVMBuildRetVoid(builder);;

   LLVMDisposeBuilder(builder);
}



static void
fs_llvm_prepare( const struct lp_fragment_shader *base,
		struct tgsi_exec_machine *machine,
		struct tgsi_sampler **samplers )
{
   /*
    * Bind tokens/shader to the interpreter's machine state.
    * Avoid redundant binding.
    */
   if (machine->Tokens != base->shader.tokens) {
      tgsi_exec_machine_bind_shader( machine,
                                     base->shader.tokens,
                                     PIPE_MAX_SAMPLERS,
                                     samplers );
   }
}




/**
 * Evaluate a constant-valued coefficient at the position of the
 * current quad.
 */
static void
eval_constant_coef(
   struct tgsi_exec_machine *mach,
   unsigned attrib,
   unsigned chan )
{
   unsigned i;

   for( i = 0; i < QUAD_SIZE; i++ ) {
      mach->Inputs[attrib].xyzw[chan].f[i] = mach->InterpCoefs[attrib].a0[chan];
   }
}

/**
 * Evaluate a linear-valued coefficient at the position of the
 * current quad.
 */
static void
eval_linear_coef(
   struct tgsi_exec_machine *mach,
   unsigned attrib,
   unsigned chan )
{
   const float x = mach->QuadPos.xyzw[0].f[0];
   const float y = mach->QuadPos.xyzw[1].f[0];
   const float dadx = mach->InterpCoefs[attrib].dadx[chan];
   const float dady = mach->InterpCoefs[attrib].dady[chan];
   const float a0 = mach->InterpCoefs[attrib].a0[chan] + dadx * x + dady * y;
   mach->Inputs[attrib].xyzw[chan].f[0] = a0;
   mach->Inputs[attrib].xyzw[chan].f[1] = a0 + dadx;
   mach->Inputs[attrib].xyzw[chan].f[2] = a0 + dady;
   mach->Inputs[attrib].xyzw[chan].f[3] = a0 + dadx + dady;
}

/**
 * Evaluate a perspective-valued coefficient at the position of the
 * current quad.
 */
static void
eval_perspective_coef(
   struct tgsi_exec_machine *mach,
   unsigned attrib,
   unsigned chan )
{
   const float x = mach->QuadPos.xyzw[0].f[0];
   const float y = mach->QuadPos.xyzw[1].f[0];
   const float dadx = mach->InterpCoefs[attrib].dadx[chan];
   const float dady = mach->InterpCoefs[attrib].dady[chan];
   const float a0 = mach->InterpCoefs[attrib].a0[chan] + dadx * x + dady * y;
   const float *w = mach->QuadPos.xyzw[3].f;
   /* divide by W here */
   mach->Inputs[attrib].xyzw[chan].f[0] = a0 / w[0];
   mach->Inputs[attrib].xyzw[chan].f[1] = (a0 + dadx) / w[1];
   mach->Inputs[attrib].xyzw[chan].f[2] = (a0 + dady) / w[2];
   mach->Inputs[attrib].xyzw[chan].f[3] = (a0 + dadx + dady) / w[3];
}


typedef void
(*eval_coef_func)(struct tgsi_exec_machine *mach,
                  unsigned attrib,
                  unsigned chan );


static void
exec_declaration(
   struct tgsi_exec_machine *mach,
   const struct tgsi_full_declaration *decl )
{
   if( mach->Processor == TGSI_PROCESSOR_FRAGMENT ) {
      if( decl->Declaration.File == TGSI_FILE_INPUT ) {
         unsigned first, last, mask;
         eval_coef_func eval;

         first = decl->DeclarationRange.First;
         last = decl->DeclarationRange.Last;
         mask = decl->Declaration.UsageMask;

         switch( decl->Declaration.Interpolate ) {
         case TGSI_INTERPOLATE_CONSTANT:
            eval = eval_constant_coef;
            break;

         case TGSI_INTERPOLATE_LINEAR:
            eval = eval_linear_coef;
            break;

         case TGSI_INTERPOLATE_PERSPECTIVE:
            eval = eval_perspective_coef;
            break;

         default:
            eval = NULL;
            assert( 0 );
         }

         if( mask == TGSI_WRITEMASK_XYZW ) {
            unsigned i, j;

            for( i = first; i <= last; i++ ) {
               for( j = 0; j < NUM_CHANNELS; j++ ) {
                  eval( mach, i, j );
               }
            }
         }
         else {
            unsigned i, j;

            for( j = 0; j < NUM_CHANNELS; j++ ) {
               if( mask & (1 << j) ) {
                  for( i = first; i <= last; i++ ) {
                     eval( mach, i, j );
                  }
               }
            }
         }
      }
   }
}

/* TODO: codegenerate the whole run function, skip this wrapper.
 * TODO: break dependency on tgsi_exec_machine struct
 * TODO: push Position calculation into the generated shader
 * TODO: process >1 quad at a time
 */
static unsigned 
fs_llvm_run( const struct lp_fragment_shader *base,
	    struct tgsi_exec_machine *machine,
	    struct quad_header *quad )
{
   struct lp_llvm_fragment_shader *shader = lp_llvm_fragment_shader(base);
   unsigned i;
   unsigned mask;

   /* Compute X, Y, Z, W vals for this quad */
   lp_setup_pos_vector(quad->posCoef, 
                       (float)quad->input.x0, (float)quad->input.y0,
                       &machine->QuadPos);

   /* init kill mask */
   tgsi_set_kill_mask(machine, 0x0);
   tgsi_set_exec_mask(machine, 1, 1, 1, 1);

   /* execute declarations (interpolants) */
   for (i = 0; i < machine->NumDeclarations; i++)
      exec_declaration( machine, &machine->Declarations[i] );

   memset(machine->Outputs, 0, sizeof machine->Outputs);

   shader->jit_function( machine->Inputs,
                         machine->Consts,
                         machine->Outputs,
                         machine->Samplers);

   /* FIXME */
   mask = ~0;

   return mask;
}


static void 
fs_llvm_delete( struct lp_fragment_shader *base )
{
   struct lp_llvm_fragment_shader *shader = lp_llvm_fragment_shader(base);
   struct llvmpipe_screen *screen = shader->screen;

   if(shader->function) {
      if(shader->jit_function)
         LLVMFreeMachineCodeForFunction(screen->engine, shader->function);
      LLVMDeleteFunction(shader->function);
   }

   FREE((void *) shader->base.shader.tokens);
   FREE(shader);
}


struct lp_fragment_shader *
llvmpipe_create_fs_llvm(struct llvmpipe_context *llvmpipe,
                        const struct pipe_shader_state *templ)
{
   struct llvmpipe_screen *screen = llvmpipe_screen(llvmpipe->pipe.screen);
   struct lp_llvm_fragment_shader *shader;
   LLVMValueRef fetch_texel;

   shader = CALLOC_STRUCT(lp_llvm_fragment_shader);
   if (!shader)
      return NULL;

   /* we need to keep a local copy of the tokens */
   shader->base.shader.tokens = tgsi_dup_tokens(templ->tokens);
   shader->base.prepare = fs_llvm_prepare;
   shader->base.run = fs_llvm_run;
   shader->base.delete = fs_llvm_delete;

   shader->screen = screen;

   tgsi_dump(templ->tokens, 0);

   shader_generate(screen, shader);

   LLVMRunFunctionPassManager(screen->pass, shader->function);

#if 1
   LLVMDumpValue(shader->function);
   debug_printf("\n");
#endif

   if(LLVMVerifyFunction(shader->function, LLVMPrintMessageAction)) {
      LLVMDumpValue(shader->function);
      abort();
   }

   fetch_texel = LLVMGetNamedFunction(screen->module, "fetch_texel");
   if(fetch_texel) {
      static boolean first_time = TRUE;
      if(first_time) {
         LLVMAddGlobalMapping(screen->engine, fetch_texel, lp_build_tgsi_fetch_texel_soa);
         first_time = FALSE;
      }
   }

   shader->jit_function = (lp_shader_fs_func)LLVMGetPointerToGlobal(screen->engine, shader->function);

   return &shader->base;
}

