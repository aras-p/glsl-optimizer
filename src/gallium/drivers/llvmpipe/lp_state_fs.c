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

#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "pipe/internal/p_winsys_screen.h"
#include "pipe/p_shader_tokens.h"
#include "draw/draw_context.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_scan.h"
#include "tgsi/tgsi_parse.h"
#include "lp_bld_type.h"
#include "lp_bld_tgsi.h"
#include "lp_bld_debug.h"
#include "lp_screen.h"
#include "lp_context.h"
#include "lp_state.h"
#include "lp_quad.h"


static void
shader_generate(struct llvmpipe_screen *screen,
                struct lp_fragment_shader *shader)
{
   const struct tgsi_token *tokens = shader->base.tokens;
   union lp_type type;
   LLVMTypeRef elem_type;
   LLVMTypeRef vec_type;
   LLVMTypeRef int_vec_type;
   LLVMTypeRef arg_types[9];
   LLVMTypeRef func_type;
   LLVMValueRef pos_ptr;
   LLVMValueRef a0_ptr;
   LLVMValueRef dadx_ptr;
   LLVMValueRef dady_ptr;
   LLVMValueRef consts_ptr;
   LLVMValueRef mask_ptr;
   LLVMValueRef color_ptr;
   LLVMValueRef depth_ptr;
   LLVMValueRef samplers_ptr;
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   LLVMValueRef pos[NUM_CHANNELS];
   LLVMValueRef outputs[PIPE_MAX_SHADER_OUTPUTS][NUM_CHANNELS];
   LLVMValueRef mask;
   unsigned i;
   unsigned attrib;
   unsigned chan;

   type.value = 0;
   type.floating = TRUE;
   type.sign = TRUE;
   type.norm = FALSE;
   type.width = 32;
   type.length = 4;

   elem_type = lp_build_elem_type(type);
   vec_type = lp_build_vec_type(type);
   int_vec_type = lp_build_int_vec_type(type);

   arg_types[0] = LLVMPointerType(vec_type, 0);        /* pos */
   arg_types[1] = LLVMPointerType(elem_type, 0);       /* a0 */
   arg_types[2] = LLVMPointerType(elem_type, 0);       /* dadx */
   arg_types[3] = LLVMPointerType(elem_type, 0);       /* dady */
   arg_types[4] = LLVMPointerType(elem_type, 0);       /* consts */
   arg_types[5] = LLVMPointerType(int_vec_type, 0);    /* mask */
   arg_types[6] = LLVMPointerType(vec_type, 0);        /* color */
   arg_types[7] = LLVMPointerType(vec_type, 0);        /* depth */
   arg_types[8] = LLVMPointerType(LLVMInt8Type(), 0);  /* samplers */

   func_type = LLVMFunctionType(LLVMVoidType(), arg_types, Elements(arg_types), 0);

   shader->function = LLVMAddFunction(screen->module, "shader", func_type);
   LLVMSetFunctionCallConv(shader->function, LLVMCCallConv);
   for(i = 0; i < Elements(arg_types); ++i)
      LLVMAddAttribute(LLVMGetParam(shader->function, i), LLVMNoAliasAttribute);

   pos_ptr = LLVMGetParam(shader->function, 0);
   a0_ptr = LLVMGetParam(shader->function, 1);
   dadx_ptr = LLVMGetParam(shader->function, 2);
   dady_ptr = LLVMGetParam(shader->function, 3);
   consts_ptr = LLVMGetParam(shader->function, 4);
   mask_ptr = LLVMGetParam(shader->function, 5);
   color_ptr = LLVMGetParam(shader->function, 6);
   depth_ptr = LLVMGetParam(shader->function, 7);
   samplers_ptr = LLVMGetParam(shader->function, 8);

   lp_build_name(pos_ptr, "pos");
   lp_build_name(a0_ptr, "a0");
   lp_build_name(dadx_ptr, "dadx");
   lp_build_name(dady_ptr, "dady");
   lp_build_name(consts_ptr, "consts");
   lp_build_name(mask_ptr, "mask");
   lp_build_name(color_ptr, "color");
   lp_build_name(depth_ptr, "depth");
   lp_build_name(samplers_ptr, "samplers");

   block = LLVMAppendBasicBlock(shader->function, "entry");
   builder = LLVMCreateBuilder();
   LLVMPositionBuilderAtEnd(builder, block);

   for(chan = 0; chan < NUM_CHANNELS; ++chan) {
      LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), chan, 0);
      pos[chan] = LLVMBuildLoad(builder, LLVMBuildGEP(builder, pos_ptr, &index, 1, ""), "");
      lp_build_name(pos[chan], "pos.%c", "xyzw"[chan]);
   }

   memset(outputs, 0, sizeof outputs);

   mask = lp_build_tgsi_soa(builder, tokens, type,
                            pos, a0_ptr, dadx_ptr, dady_ptr,
                            consts_ptr, outputs, samplers_ptr);

   for (attrib = 0; attrib < shader->info.num_outputs; ++attrib) {
      for(chan = 0; chan < NUM_CHANNELS; ++chan) {
         if(outputs[attrib][chan]) {
            lp_build_name(outputs[attrib][chan], "output%u.%c", attrib, "xyzw"[chan]);

            switch (shader->info.output_semantic_name[attrib]) {
            case TGSI_SEMANTIC_COLOR:
               {
                  unsigned cbuf = shader->info.output_semantic_index[attrib];
                  LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), cbuf*NUM_CHANNELS + chan, 0);
                  LLVMValueRef output_ptr = LLVMBuildGEP(builder, color_ptr, &index, 1, "");
                  lp_build_name(outputs[attrib][chan], "color%u.%c", attrib, "rgba"[chan]);
                  LLVMBuildStore(builder, outputs[attrib][chan], output_ptr);
                  break;
               }

            case TGSI_SEMANTIC_POSITION:
               if(chan == 3)
                  LLVMBuildStore(builder, outputs[attrib][chan], depth_ptr);
               break;
            }
         }
      }
   }

   if(mask)
      LLVMBuildStore(builder, mask, mask_ptr);

   LLVMBuildRetVoid(builder);;

   LLVMDisposeBuilder(builder);
}


void *
llvmpipe_create_fs_state(struct pipe_context *pipe,
                         const struct pipe_shader_state *templ)
{
   struct llvmpipe_screen *screen = llvmpipe_screen(pipe->screen);
   struct lp_fragment_shader *shader;
   LLVMValueRef fetch_texel;

   shader = CALLOC_STRUCT(lp_fragment_shader);
   if (!shader)
      return NULL;

   /* get/save the summary info for this shader */
   tgsi_scan_shader(templ->tokens, &shader->info);

   /* we need to keep a local copy of the tokens */
   shader->base.tokens = tgsi_dup_tokens(templ->tokens);

   shader->screen = screen;

#ifdef DEBUG
   tgsi_dump(templ->tokens, 0);
#endif

   shader_generate(screen, shader);

   LLVMRunFunctionPassManager(screen->pass, shader->function);

#ifdef DEBUG
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

#ifdef DEBUG
   lp_disassemble(shader->jit_function);
#endif

   return shader;
}


void
llvmpipe_bind_fs_state(struct pipe_context *pipe, void *fs)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);

   llvmpipe->fs = (struct lp_fragment_shader *) fs;

   llvmpipe->dirty |= LP_NEW_FS;
}


void
llvmpipe_delete_fs_state(struct pipe_context *pipe, void *fs)
{
   struct lp_fragment_shader *shader = fs;
   struct llvmpipe_screen *screen = shader->screen;

   assert(fs != llvmpipe_context(pipe)->fs);
   
   if(shader->function) {
      if(shader->jit_function)
         LLVMFreeMachineCodeForFunction(screen->engine, shader->function);
      LLVMDeleteFunction(shader->function);
   }

   FREE((void *) shader->base.tokens);
   FREE(shader);
}


void *
llvmpipe_create_vs_state(struct pipe_context *pipe,
                         const struct pipe_shader_state *templ)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   struct lp_vertex_shader *state;

   state = CALLOC_STRUCT(lp_vertex_shader);
   if (state == NULL ) 
      goto fail;

   /* copy shader tokens, the ones passed in will go away.
    */
   state->shader.tokens = tgsi_dup_tokens(templ->tokens);
   if (state->shader.tokens == NULL)
      goto fail;

   state->draw_data = draw_create_vertex_shader(llvmpipe->draw, templ);
   if (state->draw_data == NULL) 
      goto fail;

   return state;

fail:
   if (state) {
      FREE( (void *)state->shader.tokens );
      FREE( state->draw_data );
      FREE( state );
   }
   return NULL;
}


void
llvmpipe_bind_vs_state(struct pipe_context *pipe, void *vs)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);

   llvmpipe->vs = (const struct lp_vertex_shader *)vs;

   draw_bind_vertex_shader(llvmpipe->draw,
                           (llvmpipe->vs ? llvmpipe->vs->draw_data : NULL));

   llvmpipe->dirty |= LP_NEW_VS;
}


void
llvmpipe_delete_vs_state(struct pipe_context *pipe, void *vs)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);

   struct lp_vertex_shader *state =
      (struct lp_vertex_shader *)vs;

   draw_delete_vertex_shader(llvmpipe->draw, state->draw_data);
   FREE( state );
}



void
llvmpipe_set_constant_buffer(struct pipe_context *pipe,
                             uint shader, uint index,
                             const struct pipe_constant_buffer *buf)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);

   assert(shader < PIPE_SHADER_TYPES);
   assert(index == 0);

   /* note: reference counting */
   pipe_buffer_reference(&llvmpipe->constants[shader].buffer,
			 buf ? buf->buffer : NULL);

   llvmpipe->dirty |= LP_NEW_CONSTANTS;
}
