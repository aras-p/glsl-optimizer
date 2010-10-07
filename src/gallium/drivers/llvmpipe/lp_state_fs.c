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
 * @file
 * Code generate the whole fragment pipeline.
 *
 * The fragment pipeline consists of the following stages:
 * - early depth test
 * - fragment shader
 * - alpha test
 * - depth/stencil test
 * - blending
 *
 * This file has only the glue to assemble the fragment pipeline.  The actual
 * plumbing of converting Gallium state into LLVM IR is done elsewhere, in the
 * lp_bld_*.[ch] files, and in a complete generic and reusable way. Here we
 * muster the LLVM JIT execution engine to create a function that follows an
 * established binary interface and that can be called from C directly.
 *
 * A big source of complexity here is that we often want to run different
 * stages with different precisions and data types and precisions. For example,
 * the fragment shader needs typically to be done in floats, but the
 * depth/stencil test and blending is better done in the type that most closely
 * matches the depth/stencil and color buffer respectively.
 *
 * Since the width of a SIMD vector register stays the same regardless of the
 * element type, different types imply different number of elements, so we must
 * code generate more instances of the stages with larger types to be able to
 * feed/consume the stages with smaller types.
 *
 * @author Jose Fonseca <jfonseca@vmware.com>
 */

#include <limits.h>
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_pointer.h"
#include "util/u_format.h"
#include "util/u_dump.h"
#include "util/u_string.h"
#include "util/u_simple_list.h"
#include "os/os_time.h"
#include "pipe/p_shader_tokens.h"
#include "draw/draw_context.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_scan.h"
#include "tgsi/tgsi_parse.h"
#include "gallivm/lp_bld_type.h"
#include "gallivm/lp_bld_const.h"
#include "gallivm/lp_bld_conv.h"
#include "gallivm/lp_bld_init.h"
#include "gallivm/lp_bld_intr.h"
#include "gallivm/lp_bld_logic.h"
#include "gallivm/lp_bld_tgsi.h"
#include "gallivm/lp_bld_swizzle.h"
#include "gallivm/lp_bld_flow.h"
#include "gallivm/lp_bld_debug.h"

#include "lp_bld_alpha.h"
#include "lp_bld_blend.h"
#include "lp_bld_depth.h"
#include "lp_bld_interp.h"
#include "lp_context.h"
#include "lp_debug.h"
#include "lp_perf.h"
#include "lp_screen.h"
#include "lp_setup.h"
#include "lp_state.h"
#include "lp_tex_sample.h"
#include "lp_flush.h"
#include "lp_state_fs.h"


#include <llvm-c/Analysis.h>


static unsigned fs_no = 0;


/**
 * Generate the depth /stencil test code.
 */
static void
generate_depth_stencil(LLVMBuilderRef builder,
                       const struct lp_fragment_shader_variant_key *key,
                       struct lp_type src_type,
                       struct lp_build_mask_context *mask,
                       LLVMValueRef stencil_refs[2],
                       LLVMValueRef src,
                       LLVMValueRef dst_ptr,
                       LLVMValueRef facing,
                       LLVMValueRef counter)
{
   const struct util_format_description *format_desc;
   struct lp_type dst_type;

   if (!key->depth.enabled && !key->stencil[0].enabled && !key->stencil[1].enabled)
      return;

   format_desc = util_format_description(key->zsbuf_format);
   assert(format_desc);

   /*
    * Depths are expected to be between 0 and 1, even if they are stored in
    * floats. Setting these bits here will ensure that the lp_build_conv() call
    * below won't try to unnecessarily clamp the incoming values.
    */
   if(src_type.floating) {
      src_type.sign = FALSE;
      src_type.norm = TRUE;
   }
   else {
      assert(!src_type.sign);
      assert(src_type.norm);
   }

   /* Pick the depth type. */
   dst_type = lp_depth_type(format_desc, src_type.width*src_type.length);

   /* FIXME: Cope with a depth test type with a different bit width. */
   assert(dst_type.width == src_type.width);
   assert(dst_type.length == src_type.length);

   /* Convert fragment Z from float to integer */
   lp_build_conv(builder, src_type, dst_type, &src, 1, &src, 1);

   dst_ptr = LLVMBuildBitCast(builder,
                              dst_ptr,
                              LLVMPointerType(lp_build_vec_type(dst_type), 0), "");
   lp_build_depth_stencil_test(builder,
                               &key->depth,
                               key->stencil,
                               dst_type,
                               format_desc,
                               mask,
                               stencil_refs,
                               src,
                               dst_ptr,
                               facing,
                               counter);
}


/**
 * Expand the relevent bits of mask_input to a 4-dword mask for the 
 * four pixels in a 2x2 quad.  This will set the four elements of the
 * quad mask vector to 0 or ~0.
 *
 * \param quad  which quad of the quad group to test, in [0,3]
 * \param mask_input  bitwise mask for the whole 4x4 stamp
 */
static LLVMValueRef
generate_quad_mask(LLVMBuilderRef builder,
                   struct lp_type fs_type,
                   unsigned quad,
                   LLVMValueRef mask_input) /* int32 */
{
   struct lp_type mask_type;
   LLVMTypeRef i32t = LLVMInt32Type();
   LLVMValueRef bits[4];
   LLVMValueRef mask;
   int shift;

   /*
    * XXX: We'll need a different path for 16 x u8
    */
   assert(fs_type.width == 32);
   assert(fs_type.length == 4);
   mask_type = lp_int_type(fs_type);

   /*
    * mask_input >>= (quad * 4)
    */
   
   switch (quad) {
   case 0:
      shift = 0;
      break;
   case 1:
      shift = 2;
      break;
   case 2:
      shift = 8;
      break;
   case 3:
      shift = 10;
      break;
   default:
      assert(0);
      shift = 0;
   }

   mask_input = LLVMBuildLShr(builder,
                              mask_input,
                              LLVMConstInt(i32t, shift, 0),
                              "");

   /*
    * mask = { mask_input & (1 << i), for i in [0,3] }
    */

   mask = lp_build_broadcast(builder, lp_build_vec_type(mask_type), mask_input);

   bits[0] = LLVMConstInt(i32t, 1 << 0, 0);
   bits[1] = LLVMConstInt(i32t, 1 << 1, 0);
   bits[2] = LLVMConstInt(i32t, 1 << 4, 0);
   bits[3] = LLVMConstInt(i32t, 1 << 5, 0);
   
   mask = LLVMBuildAnd(builder, mask, LLVMConstVector(bits, 4), "");

   /*
    * mask = mask != 0 ? ~0 : 0
    */

   mask = lp_build_compare(builder,
                           mask_type, PIPE_FUNC_NOTEQUAL,
                           mask,
                           lp_build_const_int_vec(mask_type, 0));

   return mask;
}



/**
 * Generate the fragment shader, depth/stencil test, and alpha tests.
 * \param i  which quad in the tile, in range [0,3]
 * \param partial_mask  if 1, do mask_input testing
 */
static void
generate_fs(struct llvmpipe_context *lp,
            struct lp_fragment_shader *shader,
            const struct lp_fragment_shader_variant_key *key,
            LLVMBuilderRef builder,
            struct lp_type type,
            LLVMValueRef context_ptr,
            unsigned i,
            struct lp_build_interp_soa_context *interp,
            struct lp_build_sampler_soa *sampler,
            LLVMValueRef *pmask,
            LLVMValueRef (*color)[4],
            LLVMValueRef depth_ptr,
            LLVMValueRef facing,
            unsigned partial_mask,
            LLVMValueRef mask_input,
            LLVMValueRef counter)
{
   const struct tgsi_token *tokens = shader->base.tokens;
   LLVMTypeRef vec_type;
   LLVMValueRef consts_ptr;
   LLVMValueRef outputs[PIPE_MAX_SHADER_OUTPUTS][NUM_CHANNELS];
   LLVMValueRef z;
   LLVMValueRef stencil_refs[2];
   struct lp_build_flow_context *flow;
   struct lp_build_mask_context mask;
   boolean early_depth_stencil_test;
   unsigned attrib;
   unsigned chan;
   unsigned cbuf;

   assert(i < 4);

   stencil_refs[0] = lp_jit_context_stencil_ref_front_value(builder, context_ptr);
   stencil_refs[1] = lp_jit_context_stencil_ref_back_value(builder, context_ptr);

   vec_type = lp_build_vec_type(type);

   consts_ptr = lp_jit_context_constants(builder, context_ptr);

   flow = lp_build_flow_create(builder);

   memset(outputs, 0, sizeof outputs);

   lp_build_flow_scope_begin(flow);

   /* Declare the color and z variables */
   for(cbuf = 0; cbuf < key->nr_cbufs; cbuf++) {
      for(chan = 0; chan < NUM_CHANNELS; ++chan) {
	 color[cbuf][chan] = lp_build_alloca(builder, vec_type, "color");
      }
   }

   /* do triangle edge testing */
   if (partial_mask) {
      *pmask = generate_quad_mask(builder, type,
                                  i, mask_input);
   }
   else {
      *pmask = lp_build_const_int_vec(type, ~0);
   }

   /* 'mask' will control execution based on quad's pixel alive/killed state */
   lp_build_mask_begin(&mask, flow, type, *pmask);

   lp_build_interp_soa_update_pos(interp, i);

   /* Try to avoid the 1/w for quads where mask is zero.  TODO: avoid
    * this for depth-fail quads also.
    */
   z = interp->pos[2];

   early_depth_stencil_test =
      (key->depth.enabled || key->stencil[0].enabled) &&
      !key->alpha.enabled &&
      !shader->info.uses_kill &&
      !shader->info.writes_z;

   if (early_depth_stencil_test)
      generate_depth_stencil(builder, key,
                             type, &mask,
                             stencil_refs, z, depth_ptr, facing, counter);

   lp_build_interp_soa_update_inputs(interp, i);

   lp_build_tgsi_soa(builder, tokens, type, &mask,
                     consts_ptr, interp->pos, interp->inputs,
                     outputs, sampler, &shader->info);

   /* loop over fragment shader outputs/results */
   for (attrib = 0; attrib < shader->info.num_outputs; ++attrib) {
      for(chan = 0; chan < NUM_CHANNELS; ++chan) {
         if(outputs[attrib][chan]) {
            LLVMValueRef out = LLVMBuildLoad(builder, outputs[attrib][chan], "");
            lp_build_name(out, "output%u.%u.%c", i, attrib, "xyzw"[chan]);

            switch (shader->info.output_semantic_name[attrib]) {
            case TGSI_SEMANTIC_COLOR:
               {
                  unsigned cbuf = shader->info.output_semantic_index[attrib];

                  lp_build_name(out, "color%u.%u.%c", i, attrib, "rgba"[chan]);

                  /* Alpha test */
		  /* XXX: should only test the final assignment to alpha */
                  if (cbuf == 0 && chan == 3 && key->alpha.enabled) {
                     LLVMValueRef alpha = out;
                     LLVMValueRef alpha_ref_value;
                     alpha_ref_value = lp_jit_context_alpha_ref_value(builder, context_ptr);
                     alpha_ref_value = lp_build_broadcast(builder, vec_type, alpha_ref_value);
                     lp_build_alpha_test(builder, key->alpha.func, type,
                                         &mask, alpha, alpha_ref_value);
                  }

                  LLVMBuildStore(builder, out, color[cbuf][chan]);
                  break;
               }

            case TGSI_SEMANTIC_POSITION:
               if(chan == 2)
                  z = out;
               break;
            }
         }
      }
   }

   if (!early_depth_stencil_test)
      generate_depth_stencil(builder, key,
                             type, &mask,
                             stencil_refs, z, depth_ptr, facing, counter);

   lp_build_mask_end(&mask);

   lp_build_flow_scope_end(flow);

   lp_build_flow_destroy(flow);

   *pmask = mask.value;

}


/**
 * Generate color blending and color output.
 * \param rt  the render target index (to index blend, colormask state)
 * \param type  the pixel color type
 * \param context_ptr  pointer to the runtime JIT context
 * \param mask  execution mask (active fragment/pixel mask)
 * \param src  colors from the fragment shader
 * \param dst_ptr  the destination color buffer pointer
 */
static void
generate_blend(const struct pipe_blend_state *blend,
               unsigned rt,
               LLVMBuilderRef builder,
               struct lp_type type,
               LLVMValueRef context_ptr,
               LLVMValueRef mask,
               LLVMValueRef *src,
               LLVMValueRef dst_ptr)
{
   struct lp_build_context bld;
   struct lp_build_flow_context *flow;
   struct lp_build_mask_context mask_ctx;
   LLVMTypeRef vec_type;
   LLVMValueRef const_ptr;
   LLVMValueRef con[4];
   LLVMValueRef dst[4];
   LLVMValueRef res[4];
   unsigned chan;

   lp_build_context_init(&bld, builder, type);

   flow = lp_build_flow_create(builder);

   /* we'll use this mask context to skip blending if all pixels are dead */
   lp_build_mask_begin(&mask_ctx, flow, type, mask);

   vec_type = lp_build_vec_type(type);

   const_ptr = lp_jit_context_blend_color(builder, context_ptr);
   const_ptr = LLVMBuildBitCast(builder, const_ptr,
                                LLVMPointerType(vec_type, 0), "");

   /* load constant blend color and colors from the dest color buffer */
   for(chan = 0; chan < 4; ++chan) {
      LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), chan, 0);
      con[chan] = LLVMBuildLoad(builder, LLVMBuildGEP(builder, const_ptr, &index, 1, ""), "");

      dst[chan] = LLVMBuildLoad(builder, LLVMBuildGEP(builder, dst_ptr, &index, 1, ""), "");

      lp_build_name(con[chan], "con.%c", "rgba"[chan]);
      lp_build_name(dst[chan], "dst.%c", "rgba"[chan]);
   }

   /* do blend */
   lp_build_blend_soa(builder, blend, type, rt, src, dst, con, res);

   /* store results to color buffer */
   for(chan = 0; chan < 4; ++chan) {
      if(blend->rt[rt].colormask & (1 << chan)) {
         LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), chan, 0);
         lp_build_name(res[chan], "res.%c", "rgba"[chan]);
         res[chan] = lp_build_select(&bld, mask, res[chan], dst[chan]);
         LLVMBuildStore(builder, res[chan], LLVMBuildGEP(builder, dst_ptr, &index, 1, ""));
      }
   }

   lp_build_mask_end(&mask_ctx);
   lp_build_flow_destroy(flow);
}


/**
 * Generate the runtime callable function for the whole fragment pipeline.
 * Note that the function which we generate operates on a block of 16
 * pixels at at time.  The block contains 2x2 quads.  Each quad contains
 * 2x2 pixels.
 */
static void
generate_fragment(struct llvmpipe_context *lp,
                  struct lp_fragment_shader *shader,
                  struct lp_fragment_shader_variant *variant,
                  unsigned partial_mask)
{
   struct llvmpipe_screen *screen = llvmpipe_screen(lp->pipe.screen);
   const struct lp_fragment_shader_variant_key *key = &variant->key;
   char func_name[256];
   struct lp_type fs_type;
   struct lp_type blend_type;
   LLVMTypeRef fs_elem_type;
   LLVMTypeRef fs_int_vec_type;
   LLVMTypeRef blend_vec_type;
   LLVMTypeRef arg_types[11];
   LLVMTypeRef func_type;
   LLVMValueRef context_ptr;
   LLVMValueRef x;
   LLVMValueRef y;
   LLVMValueRef a0_ptr;
   LLVMValueRef dadx_ptr;
   LLVMValueRef dady_ptr;
   LLVMValueRef color_ptr_ptr;
   LLVMValueRef depth_ptr;
   LLVMValueRef mask_input;
   LLVMValueRef counter = NULL;
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   struct lp_build_sampler_soa *sampler;
   struct lp_build_interp_soa_context interp;
   LLVMValueRef fs_mask[LP_MAX_VECTOR_LENGTH];
   LLVMValueRef fs_out_color[PIPE_MAX_COLOR_BUFS][NUM_CHANNELS][LP_MAX_VECTOR_LENGTH];
   LLVMValueRef blend_mask;
   LLVMValueRef function;
   LLVMValueRef facing;
   unsigned num_fs;
   unsigned i;
   unsigned chan;
   unsigned cbuf;


   /* TODO: actually pick these based on the fs and color buffer
    * characteristics. */

   memset(&fs_type, 0, sizeof fs_type);
   fs_type.floating = TRUE; /* floating point values */
   fs_type.sign = TRUE;     /* values are signed */
   fs_type.norm = FALSE;    /* values are not limited to [0,1] or [-1,1] */
   fs_type.width = 32;      /* 32-bit float */
   fs_type.length = 4;      /* 4 elements per vector */
   num_fs = 4;              /* number of quads per block */

   memset(&blend_type, 0, sizeof blend_type);
   blend_type.floating = FALSE; /* values are integers */
   blend_type.sign = FALSE;     /* values are unsigned */
   blend_type.norm = TRUE;      /* values are in [0,1] or [-1,1] */
   blend_type.width = 8;        /* 8-bit ubyte values */
   blend_type.length = 16;      /* 16 elements per vector */

   /* 
    * Generate the function prototype. Any change here must be reflected in
    * lp_jit.h's lp_jit_frag_func function pointer type, and vice-versa.
    */

   fs_elem_type = lp_build_elem_type(fs_type);
   fs_int_vec_type = lp_build_int_vec_type(fs_type);

   blend_vec_type = lp_build_vec_type(blend_type);

   util_snprintf(func_name, sizeof(func_name), "fs%u_variant%u_%s", 
		 shader->no, variant->no, partial_mask ? "partial" : "whole");

   arg_types[0] = screen->context_ptr_type;            /* context */
   arg_types[1] = LLVMInt32Type();                     /* x */
   arg_types[2] = LLVMInt32Type();                     /* y */
   arg_types[3] = LLVMFloatType();                     /* facing */
   arg_types[4] = LLVMPointerType(fs_elem_type, 0);    /* a0 */
   arg_types[5] = LLVMPointerType(fs_elem_type, 0);    /* dadx */
   arg_types[6] = LLVMPointerType(fs_elem_type, 0);    /* dady */
   arg_types[7] = LLVMPointerType(LLVMPointerType(blend_vec_type, 0), 0);  /* color */
   arg_types[8] = LLVMPointerType(fs_int_vec_type, 0); /* depth */
   arg_types[9] = LLVMInt32Type();                     /* mask_input */
   arg_types[10] = LLVMPointerType(LLVMInt32Type(), 0);/* counter */

   func_type = LLVMFunctionType(LLVMVoidType(), arg_types, Elements(arg_types), 0);

   function = LLVMAddFunction(screen->module, func_name, func_type);
   LLVMSetFunctionCallConv(function, LLVMCCallConv);

   variant->function[partial_mask] = function;


   /* XXX: need to propagate noalias down into color param now we are
    * passing a pointer-to-pointer?
    */
   for(i = 0; i < Elements(arg_types); ++i)
      if(LLVMGetTypeKind(arg_types[i]) == LLVMPointerTypeKind)
         LLVMAddAttribute(LLVMGetParam(function, i), LLVMNoAliasAttribute);

   context_ptr  = LLVMGetParam(function, 0);
   x            = LLVMGetParam(function, 1);
   y            = LLVMGetParam(function, 2);
   facing       = LLVMGetParam(function, 3);
   a0_ptr       = LLVMGetParam(function, 4);
   dadx_ptr     = LLVMGetParam(function, 5);
   dady_ptr     = LLVMGetParam(function, 6);
   color_ptr_ptr = LLVMGetParam(function, 7);
   depth_ptr    = LLVMGetParam(function, 8);
   mask_input   = LLVMGetParam(function, 9);

   lp_build_name(context_ptr, "context");
   lp_build_name(x, "x");
   lp_build_name(y, "y");
   lp_build_name(a0_ptr, "a0");
   lp_build_name(dadx_ptr, "dadx");
   lp_build_name(dady_ptr, "dady");
   lp_build_name(color_ptr_ptr, "color_ptr_ptr");
   lp_build_name(depth_ptr, "depth");
   lp_build_name(mask_input, "mask_input");

   if (key->occlusion_count) {
      counter = LLVMGetParam(function, 10);
      lp_build_name(counter, "counter");
   }

   /*
    * Function body
    */

   block = LLVMAppendBasicBlock(function, "entry");
   builder = LLVMCreateBuilder();
   LLVMPositionBuilderAtEnd(builder, block);

   /*
    * The shader input interpolation info is not explicitely baked in the
    * shader key, but everything it derives from (TGSI, and flatshade) is
    * already included in the shader key.
    */
   lp_build_interp_soa_init(&interp, 
                            lp->num_inputs,
                            lp->inputs,
                            builder, fs_type,
                            a0_ptr, dadx_ptr, dady_ptr,
                            x, y);

   /* code generated texture sampling */
   sampler = lp_llvm_sampler_soa_create(key->sampler, context_ptr);

   /* loop over quads in the block */
   for(i = 0; i < num_fs; ++i) {
      LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), i, 0);
      LLVMValueRef out_color[PIPE_MAX_COLOR_BUFS][NUM_CHANNELS];
      LLVMValueRef depth_ptr_i;

      depth_ptr_i = LLVMBuildGEP(builder, depth_ptr, &index, 1, "");

      generate_fs(lp, shader, key,
                  builder,
                  fs_type,
                  context_ptr,
                  i,
                  &interp,
                  sampler,
                  &fs_mask[i], /* output */
                  out_color,
                  depth_ptr_i,
                  facing,
                  partial_mask,
                  mask_input,
                  counter);

      for(cbuf = 0; cbuf < key->nr_cbufs; cbuf++)
	 for(chan = 0; chan < NUM_CHANNELS; ++chan)
	    fs_out_color[cbuf][chan][i] = out_color[cbuf][chan];
   }

   sampler->destroy(sampler);

   /* Loop over color outputs / color buffers to do blending.
    */
   for(cbuf = 0; cbuf < key->nr_cbufs; cbuf++) {
      LLVMValueRef color_ptr;
      LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), cbuf, 0);
      LLVMValueRef blend_in_color[NUM_CHANNELS];
      unsigned rt;

      /* 
       * Convert the fs's output color and mask to fit to the blending type. 
       */
      for(chan = 0; chan < NUM_CHANNELS; ++chan) {
         LLVMValueRef fs_color_vals[LP_MAX_VECTOR_LENGTH];
         
         for (i = 0; i < num_fs; i++) {
            fs_color_vals[i] =
               LLVMBuildLoad(builder, fs_out_color[cbuf][chan][i], "fs_color_vals");
         }

	 lp_build_conv(builder, fs_type, blend_type,
                       fs_color_vals,
                       num_fs,
		       &blend_in_color[chan], 1);

	 lp_build_name(blend_in_color[chan], "color%d.%c", cbuf, "rgba"[chan]);
      }

      if (partial_mask || !variant->opaque) {
         lp_build_conv_mask(builder, fs_type, blend_type,
                            fs_mask, num_fs,
                            &blend_mask, 1);
      } else {
         blend_mask = lp_build_const_int_vec(blend_type, ~0);
      }

      color_ptr = LLVMBuildLoad(builder, 
				LLVMBuildGEP(builder, color_ptr_ptr, &index, 1, ""),
				"");
      lp_build_name(color_ptr, "color_ptr%d", cbuf);

      /* which blend/colormask state to use */
      rt = key->blend.independent_blend_enable ? cbuf : 0;

      /*
       * Blending.
       */
      generate_blend(&key->blend,
                     rt,
		     builder,
		     blend_type,
		     context_ptr,
		     blend_mask,
		     blend_in_color,
		     color_ptr);
   }

#ifdef PIPE_ARCH_X86
   /* Avoid corrupting the FPU stack on 32bit OSes. */
   lp_build_intrinsic(builder, "llvm.x86.mmx.emms", LLVMVoidType(), NULL, 0);
#endif

   LLVMBuildRetVoid(builder);

   LLVMDisposeBuilder(builder);


   /* Verify the LLVM IR.  If invalid, dump and abort */
#ifdef DEBUG
   if(LLVMVerifyFunction(function, LLVMPrintMessageAction)) {
      if (1)
         lp_debug_dump_value(function);
      abort();
   }
#endif

   /* Apply optimizations to LLVM IR */
   LLVMRunFunctionPassManager(screen->pass, function);

   if ((gallivm_debug & GALLIVM_DEBUG_IR) || (LP_DEBUG & DEBUG_FS)) {
      /* Print the LLVM IR to stderr */
      lp_debug_dump_value(function);
      debug_printf("\n");
   }

   /*
    * Translate the LLVM IR into machine code.
    */
   {
      void *f = LLVMGetPointerToGlobal(screen->engine, function);

      variant->jit_function[partial_mask] = (lp_jit_frag_func)pointer_to_func(f);

      if ((gallivm_debug & GALLIVM_DEBUG_ASM) || (LP_DEBUG & DEBUG_FS)) {
         lp_disassemble(f);
      }
      lp_func_delete_body(function);
   }
}


static void
dump_fs_variant_key(const struct lp_fragment_shader_variant_key *key)
{
   unsigned i;

   debug_printf("fs variant %p:\n", (void *) key);

   if (key->flatshade) {
      debug_printf("flatshade = 1\n");
   }
   for (i = 0; i < key->nr_cbufs; ++i) {
      debug_printf("cbuf_format[%u] = %s\n", i, util_format_name(key->cbuf_format[i]));
   }
   if (key->depth.enabled) {
      debug_printf("depth.format = %s\n", util_format_name(key->zsbuf_format));
      debug_printf("depth.func = %s\n", util_dump_func(key->depth.func, TRUE));
      debug_printf("depth.writemask = %u\n", key->depth.writemask);
   }

   for (i = 0; i < 2; ++i) {
      if (key->stencil[i].enabled) {
         debug_printf("stencil[%u].func = %s\n", i, util_dump_func(key->stencil[i].func, TRUE));
         debug_printf("stencil[%u].fail_op = %s\n", i, util_dump_stencil_op(key->stencil[i].fail_op, TRUE));
         debug_printf("stencil[%u].zpass_op = %s\n", i, util_dump_stencil_op(key->stencil[i].zpass_op, TRUE));
         debug_printf("stencil[%u].zfail_op = %s\n", i, util_dump_stencil_op(key->stencil[i].zfail_op, TRUE));
         debug_printf("stencil[%u].valuemask = 0x%x\n", i, key->stencil[i].valuemask);
         debug_printf("stencil[%u].writemask = 0x%x\n", i, key->stencil[i].writemask);
      }
   }

   if (key->alpha.enabled) {
      debug_printf("alpha.func = %s\n", util_dump_func(key->alpha.func, TRUE));
   }

   if (key->occlusion_count) {
      debug_printf("occlusion_count = 1\n");
   }

   if (key->blend.logicop_enable) {
      debug_printf("blend.logicop_func = %s\n", util_dump_logicop(key->blend.logicop_func, TRUE));
   }
   else if (key->blend.rt[0].blend_enable) {
      debug_printf("blend.rgb_func = %s\n",   util_dump_blend_func  (key->blend.rt[0].rgb_func, TRUE));
      debug_printf("blend.rgb_src_factor = %s\n",   util_dump_blend_factor(key->blend.rt[0].rgb_src_factor, TRUE));
      debug_printf("blend.rgb_dst_factor = %s\n",   util_dump_blend_factor(key->blend.rt[0].rgb_dst_factor, TRUE));
      debug_printf("blend.alpha_func = %s\n",       util_dump_blend_func  (key->blend.rt[0].alpha_func, TRUE));
      debug_printf("blend.alpha_src_factor = %s\n", util_dump_blend_factor(key->blend.rt[0].alpha_src_factor, TRUE));
      debug_printf("blend.alpha_dst_factor = %s\n", util_dump_blend_factor(key->blend.rt[0].alpha_dst_factor, TRUE));
   }
   debug_printf("blend.colormask = 0x%x\n", key->blend.rt[0].colormask);
   for (i = 0; i < key->nr_samplers; ++i) {
      debug_printf("sampler[%u] = \n", i);
      debug_printf("  .format = %s\n",
                   util_format_name(key->sampler[i].format));
      debug_printf("  .target = %s\n",
                   util_dump_tex_target(key->sampler[i].target, TRUE));
      debug_printf("  .pot = %u %u %u\n",
                   key->sampler[i].pot_width,
                   key->sampler[i].pot_height,
                   key->sampler[i].pot_depth);
      debug_printf("  .wrap = %s %s %s\n",
                   util_dump_tex_wrap(key->sampler[i].wrap_s, TRUE),
                   util_dump_tex_wrap(key->sampler[i].wrap_t, TRUE),
                   util_dump_tex_wrap(key->sampler[i].wrap_r, TRUE));
      debug_printf("  .min_img_filter = %s\n",
                   util_dump_tex_filter(key->sampler[i].min_img_filter, TRUE));
      debug_printf("  .min_mip_filter = %s\n",
                   util_dump_tex_mipfilter(key->sampler[i].min_mip_filter, TRUE));
      debug_printf("  .mag_img_filter = %s\n",
                   util_dump_tex_filter(key->sampler[i].mag_img_filter, TRUE));
      if (key->sampler[i].compare_mode != PIPE_TEX_COMPARE_NONE)
         debug_printf("  .compare_func = %s\n", util_dump_func(key->sampler[i].compare_func, TRUE));
      debug_printf("  .normalized_coords = %u\n", key->sampler[i].normalized_coords);
      debug_printf("  .min_max_lod_equal = %u\n", key->sampler[i].min_max_lod_equal);
      debug_printf("  .lod_bias_non_zero = %u\n", key->sampler[i].lod_bias_non_zero);
      debug_printf("  .apply_min_lod = %u\n", key->sampler[i].apply_min_lod);
      debug_printf("  .apply_max_lod = %u\n", key->sampler[i].apply_max_lod);
   }
}


void
lp_debug_fs_variant(const struct lp_fragment_shader_variant *variant)
{
   debug_printf("llvmpipe: Fragment shader #%u variant #%u:\n", 
                variant->shader->no, variant->no);
   tgsi_dump(variant->shader->base.tokens, 0);
   dump_fs_variant_key(&variant->key);
   debug_printf("variant->opaque = %u\n", variant->opaque);
   debug_printf("\n");
}

static struct lp_fragment_shader_variant *
generate_variant(struct llvmpipe_context *lp,
                 struct lp_fragment_shader *shader,
                 const struct lp_fragment_shader_variant_key *key)
{
   struct lp_fragment_shader_variant *variant;
   boolean fullcolormask;

   variant = CALLOC_STRUCT(lp_fragment_shader_variant);
   if(!variant)
      return NULL;

   variant->shader = shader;
   variant->list_item_global.base = variant;
   variant->list_item_local.base = variant;
   variant->no = shader->variants_created++;

   memcpy(&variant->key, key, shader->variant_key_size);

   /*
    * Determine whether we are touching all channels in the color buffer.
    */
   fullcolormask = FALSE;
   if (key->nr_cbufs == 1) {
      const struct util_format_description *format_desc;
      format_desc = util_format_description(key->cbuf_format[0]);
      if ((~key->blend.rt[0].colormask &
           util_format_colormask(format_desc)) == 0) {
         fullcolormask = TRUE;
      }
   }

   variant->opaque =
         !key->blend.logicop_enable &&
         !key->blend.rt[0].blend_enable &&
         fullcolormask &&
         !key->stencil[0].enabled &&
         !key->alpha.enabled &&
         !key->depth.enabled &&
         !shader->info.uses_kill
         ? TRUE : FALSE;


   if ((LP_DEBUG & DEBUG_FS) || (gallivm_debug & GALLIVM_DEBUG_IR)) {
      lp_debug_fs_variant(variant);
   }

   generate_fragment(lp, shader, variant, RAST_EDGE_TEST);

   if (variant->opaque) {
      /* Specialized shader, which doesn't need to read the color buffer. */
      generate_fragment(lp, shader, variant, RAST_WHOLE);
   } else {
      variant->jit_function[RAST_WHOLE] = variant->jit_function[RAST_EDGE_TEST];
   }

   return variant;
}


static void *
llvmpipe_create_fs_state(struct pipe_context *pipe,
                         const struct pipe_shader_state *templ)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   struct lp_fragment_shader *shader;
   int nr_samplers;

   shader = CALLOC_STRUCT(lp_fragment_shader);
   if (!shader)
      return NULL;

   shader->no = fs_no++;
   make_empty_list(&shader->variants);

   /* get/save the summary info for this shader */
   tgsi_scan_shader(templ->tokens, &shader->info);

   /* we need to keep a local copy of the tokens */
   shader->base.tokens = tgsi_dup_tokens(templ->tokens);

   shader->draw_data = draw_create_fragment_shader(llvmpipe->draw, templ);
   if (shader->draw_data == NULL) {
      FREE((void *) shader->base.tokens);
      FREE(shader);
      return NULL;
   }

   nr_samplers = shader->info.file_max[TGSI_FILE_SAMPLER] + 1;

   shader->variant_key_size = Offset(struct lp_fragment_shader_variant_key,
				     sampler[nr_samplers]);

   if (LP_DEBUG & DEBUG_TGSI) {
      unsigned attrib;
      debug_printf("llvmpipe: Create fragment shader #%u %p:\n", shader->no, (void *) shader);
      tgsi_dump(templ->tokens, 0);
      debug_printf("usage masks:\n");
      for (attrib = 0; attrib < shader->info.num_inputs; ++attrib) {
         unsigned usage_mask = shader->info.input_usage_mask[attrib];
         debug_printf("  IN[%u].%s%s%s%s\n",
                      attrib,
                      usage_mask & TGSI_WRITEMASK_X ? "x" : "",
                      usage_mask & TGSI_WRITEMASK_Y ? "y" : "",
                      usage_mask & TGSI_WRITEMASK_Z ? "z" : "",
                      usage_mask & TGSI_WRITEMASK_W ? "w" : "");
      }
      debug_printf("\n");
   }

   return shader;
}


static void
llvmpipe_bind_fs_state(struct pipe_context *pipe, void *fs)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);

   if (llvmpipe->fs == fs)
      return;

   draw_flush(llvmpipe->draw);

   draw_bind_fragment_shader(llvmpipe->draw,
                             (llvmpipe->fs ? llvmpipe->fs->draw_data : NULL));

   llvmpipe->fs = fs;

   llvmpipe->dirty |= LP_NEW_FS;
}

static void
remove_shader_variant(struct llvmpipe_context *lp,
                      struct lp_fragment_shader_variant *variant)
{
   struct llvmpipe_screen *screen = llvmpipe_screen(lp->pipe.screen);
   unsigned i;

   if (gallivm_debug & GALLIVM_DEBUG_IR) {
      debug_printf("llvmpipe: del fs #%u var #%u v created #%u v cached #%u v total cached #%u\n",
                    variant->shader->no, variant->no, variant->shader->variants_created,
                    variant->shader->variants_cached, lp->nr_fs_variants);
   }
   for (i = 0; i < Elements(variant->function); i++) {
      if (variant->function[i]) {
         if (variant->jit_function[i])
            LLVMFreeMachineCodeForFunction(screen->engine,
                                           variant->function[i]);
         LLVMDeleteFunction(variant->function[i]);
      }
   }
   remove_from_list(&variant->list_item_local);
   variant->shader->variants_cached--;
   remove_from_list(&variant->list_item_global);
   lp->nr_fs_variants--;
   FREE(variant);
}

static void
llvmpipe_delete_fs_state(struct pipe_context *pipe, void *fs)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   struct lp_fragment_shader *shader = fs;
   struct lp_fs_variant_list_item *li;

   assert(fs != llvmpipe->fs);
   (void) llvmpipe;

   /*
    * XXX: we need to flush the context until we have some sort of reference
    * counting in fragment shaders as they may still be binned
    * Flushing alone might not sufficient we need to wait on it too.
    */

   llvmpipe_finish(pipe, __FUNCTION__);

   li = first_elem(&shader->variants);
   while(!at_end(&shader->variants, li)) {
      struct lp_fs_variant_list_item *next = next_elem(li);
      remove_shader_variant(llvmpipe, li->base);
      li = next;
   }

   draw_delete_fragment_shader(llvmpipe->draw, shader->draw_data);

   assert(shader->variants_cached == 0);
   FREE((void *) shader->base.tokens);
   FREE(shader);
}



static void
llvmpipe_set_constant_buffer(struct pipe_context *pipe,
                             uint shader, uint index,
                             struct pipe_resource *constants)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   unsigned size = constants ? constants->width0 : 0;
   const void *data = constants ? llvmpipe_resource_data(constants) : NULL;

   assert(shader < PIPE_SHADER_TYPES);
   assert(index < PIPE_MAX_CONSTANT_BUFFERS);

   if(llvmpipe->constants[shader][index] == constants)
      return;

   draw_flush(llvmpipe->draw);

   /* note: reference counting */
   pipe_resource_reference(&llvmpipe->constants[shader][index], constants);

   if(shader == PIPE_SHADER_VERTEX ||
      shader == PIPE_SHADER_GEOMETRY) {
      draw_set_mapped_constant_buffer(llvmpipe->draw, shader,
                                      index, data, size);
   }

   llvmpipe->dirty |= LP_NEW_CONSTANTS;
}


/**
 * Return the blend factor equivalent to a destination alpha of one.
 */
static INLINE unsigned
force_dst_alpha_one(unsigned factor)
{
   switch(factor) {
   case PIPE_BLENDFACTOR_DST_ALPHA:
      return PIPE_BLENDFACTOR_ONE;
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:
      return PIPE_BLENDFACTOR_ZERO;
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
      return PIPE_BLENDFACTOR_ZERO;
   }

   return factor;
}


/**
 * We need to generate several variants of the fragment pipeline to match
 * all the combinations of the contributing state atoms.
 *
 * TODO: there is actually no reason to tie this to context state -- the
 * generated code could be cached globally in the screen.
 */
static void
make_variant_key(struct llvmpipe_context *lp,
                 struct lp_fragment_shader *shader,
                 struct lp_fragment_shader_variant_key *key)
{
   unsigned i;

   memset(key, 0, shader->variant_key_size);

   if (lp->framebuffer.zsbuf) {
      if (lp->depth_stencil->depth.enabled) {
         key->zsbuf_format = lp->framebuffer.zsbuf->format;
         memcpy(&key->depth, &lp->depth_stencil->depth, sizeof key->depth);
      }
      if (lp->depth_stencil->stencil[0].enabled) {
         key->zsbuf_format = lp->framebuffer.zsbuf->format;
         memcpy(&key->stencil, &lp->depth_stencil->stencil, sizeof key->stencil);
      }
   }

   key->alpha.enabled = lp->depth_stencil->alpha.enabled;
   if(key->alpha.enabled)
      key->alpha.func = lp->depth_stencil->alpha.func;
   /* alpha.ref_value is passed in jit_context */

   key->flatshade = lp->rasterizer->flatshade;
   if (lp->active_query_count) {
      key->occlusion_count = TRUE;
   }

   if (lp->framebuffer.nr_cbufs) {
      memcpy(&key->blend, lp->blend, sizeof key->blend);
   }

   key->nr_cbufs = lp->framebuffer.nr_cbufs;
   for (i = 0; i < lp->framebuffer.nr_cbufs; i++) {
      enum pipe_format format = lp->framebuffer.cbufs[i]->format;
      struct pipe_rt_blend_state *blend_rt = &key->blend.rt[i];
      const struct util_format_description *format_desc;

      key->cbuf_format[i] = format;

      format_desc = util_format_description(format);
      assert(format_desc->colorspace == UTIL_FORMAT_COLORSPACE_RGB ||
             format_desc->colorspace == UTIL_FORMAT_COLORSPACE_SRGB);

      blend_rt->colormask = lp->blend->rt[i].colormask;

      /*
       * Mask out color channels not present in the color buffer.
       */
      blend_rt->colormask &= util_format_colormask(format_desc);

      /*
       * Our swizzled render tiles always have an alpha channel, but the linear
       * render target format often does not, so force here the dst alpha to be
       * one.
       *
       * This is not a mere optimization. Wrong results will be produced if the
       * dst alpha is used, the dst format does not have alpha, and the previous
       * rendering was not flushed from the swizzled to linear buffer. For
       * example, NonPowTwo DCT.
       *
       * TODO: This should be generalized to all channels for better
       * performance, but only alpha causes correctness issues.
       *
       * Also, force rgb/alpha func/factors match, to make AoS blending easier.
       */
      if (format_desc->swizzle[3] > UTIL_FORMAT_SWIZZLE_W) {
         blend_rt->rgb_src_factor   = force_dst_alpha_one(blend_rt->rgb_src_factor);
         blend_rt->rgb_dst_factor   = force_dst_alpha_one(blend_rt->rgb_dst_factor);
         blend_rt->alpha_func       = blend_rt->rgb_func;
         blend_rt->alpha_src_factor = blend_rt->rgb_src_factor;
         blend_rt->alpha_dst_factor = blend_rt->rgb_dst_factor;
      }
   }

   /* This value will be the same for all the variants of a given shader:
    */
   key->nr_samplers = shader->info.file_max[TGSI_FILE_SAMPLER] + 1;

   for(i = 0; i < key->nr_samplers; ++i) {
      if(shader->info.file_mask[TGSI_FILE_SAMPLER] & (1 << i)) {
         lp_sampler_static_state(&key->sampler[i],
				 lp->fragment_sampler_views[i],
				 lp->sampler[i]);
      }
   }
}

/**
 * Update fragment state.  This is called just prior to drawing
 * something when some fragment-related state has changed.
 */
void 
llvmpipe_update_fs(struct llvmpipe_context *lp)
{
   struct lp_fragment_shader *shader = lp->fs;
   struct lp_fragment_shader_variant_key key;
   struct lp_fragment_shader_variant *variant = NULL;
   struct lp_fs_variant_list_item *li;

   make_variant_key(lp, shader, &key);

   li = first_elem(&shader->variants);
   while(!at_end(&shader->variants, li)) {
      if(memcmp(&li->base->key, &key, shader->variant_key_size) == 0) {
         variant = li->base;
         break;
      }
      li = next_elem(li);
   }

   if (variant) {
      move_to_head(&lp->fs_variants_list, &variant->list_item_global);
   }
   else {
      int64_t t0, t1;
      int64_t dt;
      unsigned i;
      if (lp->nr_fs_variants >= LP_MAX_SHADER_VARIANTS) {
         struct pipe_context *pipe = &lp->pipe;

         /*
          * XXX: we need to flush the context until we have some sort of reference
          * counting in fragment shaders as they may still be binned
          * Flushing alone might not be sufficient we need to wait on it too.
          */
         llvmpipe_finish(pipe, __FUNCTION__);

         for (i = 0; i < LP_MAX_SHADER_VARIANTS / 4; i++) {
            struct lp_fs_variant_list_item *item = last_elem(&lp->fs_variants_list);
            remove_shader_variant(lp, item->base);
         }
      }
      t0 = os_time_get();

      variant = generate_variant(lp, shader, &key);

      t1 = os_time_get();
      dt = t1 - t0;
      LP_COUNT_ADD(llvm_compile_time, dt);
      LP_COUNT_ADD(nr_llvm_compiles, 2);  /* emit vs. omit in/out test */

      if (variant) {
         insert_at_head(&shader->variants, &variant->list_item_local);
         insert_at_head(&lp->fs_variants_list, &variant->list_item_global);
         lp->nr_fs_variants++;
         shader->variants_cached++;
      }
   }

   lp_setup_set_fs_variant(lp->setup, variant);
}



void
llvmpipe_init_fs_funcs(struct llvmpipe_context *llvmpipe)
{
   llvmpipe->pipe.create_fs_state = llvmpipe_create_fs_state;
   llvmpipe->pipe.bind_fs_state   = llvmpipe_bind_fs_state;
   llvmpipe->pipe.delete_fs_state = llvmpipe_delete_fs_state;

   llvmpipe->pipe.set_constant_buffer = llvmpipe_set_constant_buffer;
}
