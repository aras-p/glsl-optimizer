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
 * @author Jose Fonseca <jfonseca@vmware.com>
 * @author Keith Whitwell <keith@tungstengraphics.com>
 */

#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_debug_dump.h"
#include "lp_screen.h"
#include "lp_context.h"
#include "lp_state.h"

#include "lp_bld_type.h"
#include "lp_bld_arit.h"
#include "lp_bld_logic.h"
#include "lp_bld_blend.h"
#include "lp_bld_debug.h"


/**
 * Generate blending code according to blend->base state.
 * The blend function will look like:
 *    blend(mask, src_color, constant color, dst_color)
 * dst_color will be modified and contain the result of the blend func.
 */
static void
blend_generate(struct llvmpipe_screen *screen,
               struct lp_blend_state *blend)
{
   union lp_type type;
   struct lp_build_context bld;
   LLVMTypeRef vec_type;
   LLVMTypeRef int_vec_type;
   LLVMTypeRef arg_types[4];
   LLVMTypeRef func_type;
   LLVMValueRef mask_ptr;
   LLVMValueRef src_ptr;
   LLVMValueRef dst_ptr;
   LLVMValueRef const_ptr;
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   LLVMValueRef mask;
   LLVMValueRef src[4];
   LLVMValueRef con[4];
   LLVMValueRef dst[4];
   LLVMValueRef res[4];
   unsigned i;

   type.value = 0;
   type.floating = FALSE; /* values are integers */
   type.sign = FALSE;     /* values are unsigned */
   type.norm = TRUE;      /* values are in [0,1] or [-1,1] */
   type.width = 8;        /* 8-bit ubyte values */
   type.length = 16;      /* 16 elements per vector */

   vec_type = lp_build_vec_type(type);
   int_vec_type = lp_build_int_vec_type(type);

   arg_types[0] = LLVMPointerType(int_vec_type, 0); /* mask */
   arg_types[1] = LLVMPointerType(vec_type, 0);     /* src */
   arg_types[2] = LLVMPointerType(vec_type, 0);     /* con */
   arg_types[3] = LLVMPointerType(vec_type, 0);     /* dst */
   func_type = LLVMFunctionType(LLVMVoidType(), arg_types, Elements(arg_types), 0);
   blend->function = LLVMAddFunction(screen->module, "blend", func_type);
   LLVMSetFunctionCallConv(blend->function, LLVMCCallConv);

   mask_ptr = LLVMGetParam(blend->function, 0);
   src_ptr = LLVMGetParam(blend->function, 1);
   const_ptr = LLVMGetParam(blend->function, 2);
   dst_ptr = LLVMGetParam(blend->function, 3);

   block = LLVMAppendBasicBlock(blend->function, "entry");
   builder = LLVMCreateBuilder();
   LLVMPositionBuilderAtEnd(builder, block);

   lp_build_context_init(&bld, builder, type);

   mask = LLVMBuildLoad(builder, mask_ptr, "mask");

   for(i = 0; i < 4; ++i) {
      LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), i, 0);

      src[i] = LLVMBuildLoad(builder, LLVMBuildGEP(builder, src_ptr, &index, 1, ""), "");
      con[i] = LLVMBuildLoad(builder, LLVMBuildGEP(builder, const_ptr, &index, 1, ""), "");
      dst[i] = LLVMBuildLoad(builder, LLVMBuildGEP(builder, dst_ptr, &index, 1, ""), "");

      lp_build_name(src[i], "src.%c", "rgba"[i]);
      lp_build_name(con[i], "con.%c", "rgba"[i]);
      lp_build_name(dst[i], "dst.%c", "rgba"[i]);
   }

   lp_build_blend_soa(builder, &blend->base, type, src, dst, con, res);

   for(i = 0; i < 4; ++i) {
      LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), i, 0);
      lp_build_name(res[i], "res.%c", "rgba"[i]);
      res[i] = lp_build_select(&bld, mask, res[i], dst[i]);
      LLVMBuildStore(builder, res[i], LLVMBuildGEP(builder, dst_ptr, &index, 1, ""));
   }

   LLVMBuildRetVoid(builder);;

   LLVMDisposeBuilder(builder);
}


void *
llvmpipe_create_blend_state(struct pipe_context *pipe,
                            const struct pipe_blend_state *base)
{
   struct llvmpipe_screen *screen = llvmpipe_screen(pipe->screen);
   struct lp_blend_state *blend;

   blend = CALLOC_STRUCT(lp_blend_state);
   if(!blend)
      return NULL;

   blend->base = *base;

   blend_generate(screen, blend);

   LLVMRunFunctionPassManager(screen->pass, blend->function);

#ifdef DEBUG
   debug_printf("%s=%s %s=%s %s=%s %s=%s %s=%s %s=%s\n",
                "rgb_func",         debug_dump_blend_func  (blend->base.rgb_func, TRUE),
                "rgb_src_factor",   debug_dump_blend_factor(blend->base.rgb_src_factor, TRUE),
                "rgb_dst_factor",   debug_dump_blend_factor(blend->base.rgb_dst_factor, TRUE),
                "alpha_func",       debug_dump_blend_func  (blend->base.alpha_func, TRUE),
                "alpha_src_factor", debug_dump_blend_factor(blend->base.alpha_src_factor, TRUE),
                "alpha_dst_factor", debug_dump_blend_factor(blend->base.alpha_dst_factor, TRUE));
   LLVMDumpValue(blend->function);
   debug_printf("\n");
#endif

   if(LLVMVerifyFunction(blend->function, LLVMPrintMessageAction)) {
      LLVMDumpValue(blend->function);
      abort();
   }

   blend->jit_function = (lp_blend_func)LLVMGetPointerToGlobal(screen->engine, blend->function);

#ifdef DEBUG
   lp_disassemble(blend->jit_function);
#endif

   return blend;
}

void llvmpipe_bind_blend_state( struct pipe_context *pipe,
                                void *blend )
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);

   llvmpipe->blend = (struct lp_blend_state *)blend;

   llvmpipe->dirty |= LP_NEW_BLEND;
}

void llvmpipe_delete_blend_state(struct pipe_context *pipe,
                                 void *_blend)
{
   struct llvmpipe_screen *screen = llvmpipe_screen(pipe->screen);
   struct lp_blend_state *blend = (struct lp_blend_state *)_blend;

   if(blend->function) {
      if(blend->jit_function)
         LLVMFreeMachineCodeForFunction(screen->engine, blend->function);
      LLVMDeleteFunction(blend->function);
   }

   FREE( blend );
}


void llvmpipe_set_blend_color( struct pipe_context *pipe,
			     const struct pipe_blend_color *blend_color )
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   unsigned i, j;

   for (i = 0; i < 4; ++i)
      for (j = 0; j < 16; ++j)
         llvmpipe->blend_color[i][j] = float_to_ubyte(blend_color->color[i]);

   llvmpipe->dirty |= LP_NEW_BLEND;
}


/** XXX move someday?  Or consolidate all these simple state setters
 * into one file.
 */


void *
llvmpipe_create_depth_stencil_state(struct pipe_context *pipe,
				    const struct pipe_depth_stencil_alpha_state *depth_stencil)
{
   return mem_dup(depth_stencil, sizeof(*depth_stencil));
}

void
llvmpipe_bind_depth_stencil_state(struct pipe_context *pipe,
                                  void *depth_stencil)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);

   llvmpipe->depth_stencil = (const struct pipe_depth_stencil_alpha_state *)depth_stencil;

   llvmpipe->dirty |= LP_NEW_DEPTH_STENCIL_ALPHA;
}

void
llvmpipe_delete_depth_stencil_state(struct pipe_context *pipe, void *depth)
{
   FREE( depth );
}
