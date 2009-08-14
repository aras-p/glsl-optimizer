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
(*lp_shader_fs_func)(void *pos,
                     void *a0,
                     void *dadx,
                     void *dady,
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

   union tgsi_exec_channel ALIGN16_ATTRIB pos[NUM_CHANNELS];
   union tgsi_exec_channel ALIGN16_ATTRIB a0[PIPE_MAX_SHADER_INPUTS][NUM_CHANNELS];
   union tgsi_exec_channel ALIGN16_ATTRIB dadx[PIPE_MAX_SHADER_INPUTS][NUM_CHANNELS];
   union tgsi_exec_channel ALIGN16_ATTRIB dady[PIPE_MAX_SHADER_INPUTS][NUM_CHANNELS];

   uint32_t magic;
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
   LLVMTypeRef arg_types[7];
   LLVMTypeRef func_type;
   LLVMValueRef pos_ptr;
   LLVMValueRef a0_ptr;
   LLVMValueRef dadx_ptr;
   LLVMValueRef dady_ptr;
   LLVMValueRef consts_ptr;
   LLVMValueRef outputs_ptr;
   LLVMValueRef samplers_ptr;
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   LLVMValueRef pos[NUM_CHANNELS];
   LLVMValueRef outputs[PIPE_MAX_SHADER_OUTPUTS][NUM_CHANNELS];
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

   arg_types[0] = LLVMPointerType(vec_type, 0);        /* pos */
   arg_types[1] = LLVMPointerType(vec_type, 0);        /* a0 */
   arg_types[2] = LLVMPointerType(vec_type, 0);        /* dadx */
   arg_types[3] = LLVMPointerType(vec_type, 0);        /* dady */
   arg_types[4] = LLVMPointerType(elem_type, 0);       /* consts */
   arg_types[5] = LLVMPointerType(vec_type, 0);        /* outputs */
   arg_types[6] = LLVMPointerType(LLVMInt8Type(), 0);  /* samplers */

   func_type = LLVMFunctionType(LLVMVoidType(), arg_types, Elements(arg_types), 0);

   shader->function = LLVMAddFunction(screen->module, "shader", func_type);
   LLVMSetFunctionCallConv(shader->function, LLVMCCallConv);

   pos_ptr = LLVMGetParam(shader->function, 0);
   a0_ptr = LLVMGetParam(shader->function, 1);
   dadx_ptr = LLVMGetParam(shader->function, 2);
   dady_ptr = LLVMGetParam(shader->function, 3);
   consts_ptr = LLVMGetParam(shader->function, 4);
   outputs_ptr = LLVMGetParam(shader->function, 5);
   samplers_ptr = LLVMGetParam(shader->function, 6);

   LLVMSetValueName(pos_ptr, "pos");
   LLVMSetValueName(a0_ptr, "a0");
   LLVMSetValueName(dadx_ptr, "dadx");
   LLVMSetValueName(dady_ptr, "dady");
   LLVMSetValueName(consts_ptr, "consts");
   LLVMSetValueName(outputs_ptr, "outputs");
   LLVMSetValueName(samplers_ptr, "samplers");

   block = LLVMAppendBasicBlock(shader->function, "entry");
   builder = LLVMCreateBuilder();
   LLVMPositionBuilderAtEnd(builder, block);

   for(j = 0; j < NUM_CHANNELS; ++j) {
      LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), j, 0);
      util_snprintf(name, sizeof name, "pos.%c", "xyzw"[j]);
      pos[j] = LLVMBuildLoad(builder, LLVMBuildGEP(builder, pos_ptr, &index, 1, ""), name);
   }

   memset(outputs, 0, sizeof outputs);

   lp_build_tgsi_soa(builder, tokens, type,
                     pos, a0_ptr, dadx_ptr, dady_ptr,
                     consts_ptr, outputs, samplers_ptr);

   for(i = 0; i < PIPE_MAX_SHADER_OUTPUTS; ++i) {
      for(j = 0; j < NUM_CHANNELS; ++j) {
         if(outputs[i][j]) {
            LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), i*NUM_CHANNELS + j, 0);
            util_snprintf(name, sizeof name, "output%u.%c", i, "xyzw"[j]);
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



static void
setup_pos_vector(struct lp_llvm_fragment_shader *shader,
                 const struct tgsi_interp_coef *coef,
                 float x, float y)
{
   uint chan;

   /* do X */
   shader->pos[0].f[0] = x;
   shader->pos[0].f[1] = x + 1;
   shader->pos[0].f[2] = x;
   shader->pos[0].f[3] = x + 1;

   /* do Y */
   shader->pos[1].f[0] = y;
   shader->pos[1].f[1] = y;
   shader->pos[1].f[2] = y + 1;
   shader->pos[1].f[3] = y + 1;

   /* do Z and W for all fragments in the quad */
   for (chan = 2; chan < 4; chan++) {
      const float dadx = coef->dadx[chan];
      const float dady = coef->dady[chan];
      const float a0 = coef->a0[chan] + dadx * x + dady * y;
      shader->pos[chan].f[0] = a0;
      shader->pos[chan].f[1] = a0 + dadx;
      shader->pos[chan].f[2] = a0 + dady;
      shader->pos[chan].f[3] = a0 + dadx + dady;
   }
}


static void
setup_coef_vector(struct lp_llvm_fragment_shader *shader,
                  const struct tgsi_interp_coef *coef)
{
   unsigned attrib, chan, i;

   for (attrib = 0; attrib < PIPE_MAX_SHADER_INPUTS; ++attrib) {
      for (chan = 0; chan < NUM_CHANNELS; ++chan) {
         for( i = 0; i < QUAD_SIZE; ++i ) {
            shader->a0[attrib][chan].f[i] = coef[attrib].a0[chan];
            shader->dadx[attrib][chan].f[i] = coef[attrib].dadx[chan];
            shader->dady[attrib][chan].f[i] = coef[attrib].dady[chan];
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
   unsigned mask;

   /* Compute X, Y, Z, W vals for this quad */
   setup_pos_vector(shader,
                    quad->posCoef,
                   (float)quad->input.x0, (float)quad->input.y0);

   setup_coef_vector(shader,
                     quad->coef);

   /* init kill mask */
   tgsi_set_kill_mask(machine, 0x0);
   tgsi_set_exec_mask(machine, 1, 1, 1, 1);

   memset(machine->Outputs, 0, sizeof machine->Outputs);

   shader->jit_function( shader->pos,
                         shader->a0, shader->dadx, shader->dady,
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

