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
 * - stipple (TBI)
 * - early depth test
 * - fragment shader
 * - alpha test
 * - depth/stencil test (stencil TBI)
 * - blending
 *
 * This file has only the glue to assembly the fragment pipeline.  The actual
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

#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_format.h"
#include "util/u_debug_dump.h"
#include "pipe/p_shader_tokens.h"
#include "draw/draw_context.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_scan.h"
#include "tgsi/tgsi_parse.h"
#include "lp_bld_type.h"
#include "lp_bld_const.h"
#include "lp_bld_conv.h"
#include "lp_bld_intr.h"
#include "lp_bld_logic.h"
#include "lp_bld_depth.h"
#include "lp_bld_interp.h"
#include "lp_bld_tgsi.h"
#include "lp_bld_alpha.h"
#include "lp_bld_blend.h"
#include "lp_bld_swizzle.h"
#include "lp_bld_flow.h"
#include "lp_bld_debug.h"
#include "lp_screen.h"
#include "lp_context.h"
#include "lp_buffer.h"
#include "lp_state.h"
#include "lp_tex_sample.h"
#include "lp_debug.h"


static const unsigned char quad_offset_x[4] = {0, 1, 0, 1};
static const unsigned char quad_offset_y[4] = {0, 0, 1, 1};


/*
 * Derive from the quad's upper left scalar coordinates the coordinates for
 * all other quad pixels
 */
static void
generate_pos0(LLVMBuilderRef builder,
              LLVMValueRef x,
              LLVMValueRef y,
              LLVMValueRef *x0,
              LLVMValueRef *y0)
{
   LLVMTypeRef int_elem_type = LLVMInt32Type();
   LLVMTypeRef int_vec_type = LLVMVectorType(int_elem_type, QUAD_SIZE);
   LLVMTypeRef elem_type = LLVMFloatType();
   LLVMTypeRef vec_type = LLVMVectorType(elem_type, QUAD_SIZE);
   LLVMValueRef x_offsets[QUAD_SIZE];
   LLVMValueRef y_offsets[QUAD_SIZE];
   unsigned i;

   x = lp_build_broadcast(builder, int_vec_type, x);
   y = lp_build_broadcast(builder, int_vec_type, y);

   for(i = 0; i < QUAD_SIZE; ++i) {
      x_offsets[i] = LLVMConstInt(int_elem_type, quad_offset_x[i], 0);
      y_offsets[i] = LLVMConstInt(int_elem_type, quad_offset_y[i], 0);
   }

   x = LLVMBuildAdd(builder, x, LLVMConstVector(x_offsets, QUAD_SIZE), "");
   y = LLVMBuildAdd(builder, y, LLVMConstVector(y_offsets, QUAD_SIZE), "");

   *x0 = LLVMBuildSIToFP(builder, x, vec_type, "");
   *y0 = LLVMBuildSIToFP(builder, y, vec_type, "");
}


/**
 * Generate the depth test.
 */
static void
generate_depth(LLVMBuilderRef builder,
               const struct lp_fragment_shader_variant_key *key,
               struct lp_type src_type,
               struct lp_build_mask_context *mask,
               LLVMValueRef src,
               LLVMValueRef dst_ptr)
{
   const struct util_format_description *format_desc;
   struct lp_type dst_type;

   if(!key->depth.enabled)
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

   lp_build_conv(builder, src_type, dst_type, &src, 1, &src, 1);

   dst_ptr = LLVMBuildBitCast(builder,
                              dst_ptr,
                              LLVMPointerType(lp_build_vec_type(dst_type), 0), "");

   lp_build_depth_test(builder,
                       &key->depth,
                       dst_type,
                       format_desc,
                       mask,
                       src,
                       dst_ptr);
}


/**
 * Generate the fragment shader, depth/stencil test, and alpha tests.
 */
static void
generate_fs(struct llvmpipe_context *lp,
            struct lp_fragment_shader *shader,
            const struct lp_fragment_shader_variant_key *key,
            LLVMBuilderRef builder,
            struct lp_type type,
            LLVMValueRef context_ptr,
            unsigned i,
            const struct lp_build_interp_soa_context *interp,
            struct lp_build_sampler_soa *sampler,
            LLVMValueRef *pmask,
            LLVMValueRef *color,
            LLVMValueRef depth_ptr)
{
   const struct tgsi_token *tokens = shader->base.tokens;
   LLVMTypeRef elem_type;
   LLVMTypeRef vec_type;
   LLVMTypeRef int_vec_type;
   LLVMValueRef consts_ptr;
   LLVMValueRef outputs[PIPE_MAX_SHADER_OUTPUTS][NUM_CHANNELS];
   LLVMValueRef z = interp->pos[2];
   struct lp_build_flow_context *flow;
   struct lp_build_mask_context mask;
   boolean early_depth_test;
   unsigned attrib;
   unsigned chan;

   elem_type = lp_build_elem_type(type);
   vec_type = lp_build_vec_type(type);
   int_vec_type = lp_build_int_vec_type(type);

   consts_ptr = lp_jit_context_constants(builder, context_ptr);

   flow = lp_build_flow_create(builder);

   memset(outputs, 0, sizeof outputs);

   lp_build_flow_scope_begin(flow);

   /* Declare the color and z variables */
   for(chan = 0; chan < NUM_CHANNELS; ++chan) {
      color[chan] = LLVMGetUndef(vec_type);
      lp_build_flow_scope_declare(flow, &color[chan]);
   }
   lp_build_flow_scope_declare(flow, &z);

   lp_build_mask_begin(&mask, flow, type, *pmask);

   early_depth_test =
      key->depth.enabled &&
      !key->alpha.enabled &&
      !shader->info.uses_kill &&
      !shader->info.writes_z;

   if(early_depth_test)
      generate_depth(builder, key,
                     type, &mask,
                     z, depth_ptr);

   lp_build_tgsi_soa(builder, tokens, type, &mask,
                     consts_ptr, interp->pos, interp->inputs,
                     outputs, sampler);

   for (attrib = 0; attrib < shader->info.num_outputs; ++attrib) {
      for(chan = 0; chan < NUM_CHANNELS; ++chan) {
         if(outputs[attrib][chan]) {
            lp_build_name(outputs[attrib][chan], "output%u.%u.%c", i, attrib, "xyzw"[chan]);

            switch (shader->info.output_semantic_name[attrib]) {
            case TGSI_SEMANTIC_COLOR:
               {
                  unsigned cbuf = shader->info.output_semantic_index[attrib];

                  lp_build_name(outputs[attrib][chan], "color%u.%u.%c", i, attrib, "rgba"[chan]);

                  /* Alpha test */
                  /* XXX: should the alpha reference value be passed separately? */
                  if(cbuf == 0 && chan == 3) {
                     LLVMValueRef alpha = outputs[attrib][chan];
                     LLVMValueRef alpha_ref_value;
                     alpha_ref_value = lp_jit_context_alpha_ref_value(builder, context_ptr);
                     alpha_ref_value = lp_build_broadcast(builder, vec_type, alpha_ref_value);
                     lp_build_alpha_test(builder, &key->alpha, type,
                                         &mask, alpha, alpha_ref_value);
                  }

                  if(cbuf == 0)
                     color[chan] = outputs[attrib][chan];

                  break;
               }

            case TGSI_SEMANTIC_POSITION:
               if(chan == 2)
                  z = outputs[attrib][chan];
               break;
            }
         }
      }
   }

   if(!early_depth_test)
      generate_depth(builder, key,
                     type, &mask,
                     z, depth_ptr);

   lp_build_mask_end(&mask);

   lp_build_flow_scope_end(flow);

   lp_build_flow_destroy(flow);

   *pmask = mask.value;

}


/**
 * Generate color blending and color output.
 */
static void
generate_blend(const struct pipe_blend_state *blend,
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
   LLVMTypeRef int_vec_type;
   LLVMValueRef const_ptr;
   LLVMValueRef con[4];
   LLVMValueRef dst[4];
   LLVMValueRef res[4];
   unsigned chan;

   lp_build_context_init(&bld, builder, type);

   flow = lp_build_flow_create(builder);
   lp_build_mask_begin(&mask_ctx, flow, type, mask);

   vec_type = lp_build_vec_type(type);
   int_vec_type = lp_build_int_vec_type(type);

   const_ptr = lp_jit_context_blend_color(builder, context_ptr);
   const_ptr = LLVMBuildBitCast(builder, const_ptr,
                                LLVMPointerType(vec_type, 0), "");

   for(chan = 0; chan < 4; ++chan) {
      LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), chan, 0);
      con[chan] = LLVMBuildLoad(builder, LLVMBuildGEP(builder, const_ptr, &index, 1, ""), "");

      dst[chan] = LLVMBuildLoad(builder, LLVMBuildGEP(builder, dst_ptr, &index, 1, ""), "");

      lp_build_name(con[chan], "con.%c", "rgba"[chan]);
      lp_build_name(dst[chan], "dst.%c", "rgba"[chan]);
   }

   lp_build_blend_soa(builder, blend, type, src, dst, con, res);

   for(chan = 0; chan < 4; ++chan) {
      if(blend->rt[0].colormask & (1 << chan)) {
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
 */
static struct lp_fragment_shader_variant *
generate_fragment(struct llvmpipe_context *lp,
                  struct lp_fragment_shader *shader,
                  const struct lp_fragment_shader_variant_key *key)
{
   struct llvmpipe_screen *screen = llvmpipe_screen(lp->pipe.screen);
   struct lp_fragment_shader_variant *variant;
   struct lp_type fs_type;
   struct lp_type blend_type;
   LLVMTypeRef fs_elem_type;
   LLVMTypeRef fs_vec_type;
   LLVMTypeRef fs_int_vec_type;
   LLVMTypeRef blend_vec_type;
   LLVMTypeRef blend_int_vec_type;
   LLVMTypeRef arg_types[9];
   LLVMTypeRef func_type;
   LLVMValueRef context_ptr;
   LLVMValueRef x;
   LLVMValueRef y;
   LLVMValueRef a0_ptr;
   LLVMValueRef dadx_ptr;
   LLVMValueRef dady_ptr;
   LLVMValueRef mask_ptr;
   LLVMValueRef color_ptr;
   LLVMValueRef depth_ptr;
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   LLVMValueRef x0;
   LLVMValueRef y0;
   struct lp_build_sampler_soa *sampler;
   struct lp_build_interp_soa_context interp;
   LLVMValueRef fs_mask[LP_MAX_VECTOR_LENGTH];
   LLVMValueRef fs_out_color[NUM_CHANNELS][LP_MAX_VECTOR_LENGTH];
   LLVMValueRef blend_mask;
   LLVMValueRef blend_in_color[NUM_CHANNELS];
   unsigned num_fs;
   unsigned i;
   unsigned chan;

   if (LP_DEBUG & DEBUG_JIT) {
      tgsi_dump(shader->base.tokens, 0);
      if(key->depth.enabled) {
         debug_printf("depth.format = %s\n", pf_name(key->zsbuf_format));
         debug_printf("depth.func = %s\n", debug_dump_func(key->depth.func, TRUE));
         debug_printf("depth.writemask = %u\n", key->depth.writemask);
      }
      if(key->alpha.enabled) {
         debug_printf("alpha.func = %s\n", debug_dump_func(key->alpha.func, TRUE));
         debug_printf("alpha.ref_value = %f\n", key->alpha.ref_value);
      }
      if(key->blend.logicop_enable) {
         debug_printf("blend.logicop_func = %u\n", key->blend.logicop_func);
      }
      else if(key->blend.rt[0].blend_enable) {
         debug_printf("blend.rgb_func = %s\n",   debug_dump_blend_func  (key->blend.rt[0].rgb_func, TRUE));
         debug_printf("rgb_src_factor = %s\n",   debug_dump_blend_factor(key->blend.rt[0].rgb_src_factor, TRUE));
         debug_printf("rgb_dst_factor = %s\n",   debug_dump_blend_factor(key->blend.rt[0].rgb_dst_factor, TRUE));
         debug_printf("alpha_func = %s\n",       debug_dump_blend_func  (key->blend.rt[0].alpha_func, TRUE));
         debug_printf("alpha_src_factor = %s\n", debug_dump_blend_factor(key->blend.rt[0].alpha_src_factor, TRUE));
         debug_printf("alpha_dst_factor = %s\n", debug_dump_blend_factor(key->blend.rt[0].alpha_dst_factor, TRUE));
      }
      debug_printf("blend.colormask = 0x%x\n", key->blend.rt[0].colormask);
      for(i = 0; i < PIPE_MAX_SAMPLERS; ++i) {
         if(key->sampler[i].format) {
            debug_printf("sampler[%u] = \n", i);
            debug_printf("  .format = %s\n",
                         pf_name(key->sampler[i].format));
            debug_printf("  .target = %s\n",
                         debug_dump_tex_target(key->sampler[i].target, TRUE));
            debug_printf("  .pot = %u %u %u\n",
                         key->sampler[i].pot_width,
                         key->sampler[i].pot_height,
                         key->sampler[i].pot_depth);
            debug_printf("  .wrap = %s %s %s\n",
                         debug_dump_tex_wrap(key->sampler[i].wrap_s, TRUE),
                         debug_dump_tex_wrap(key->sampler[i].wrap_t, TRUE),
                         debug_dump_tex_wrap(key->sampler[i].wrap_r, TRUE));
            debug_printf("  .min_img_filter = %s\n",
                         debug_dump_tex_filter(key->sampler[i].min_img_filter, TRUE));
            debug_printf("  .min_mip_filter = %s\n",
                         debug_dump_tex_mipfilter(key->sampler[i].min_mip_filter, TRUE));
            debug_printf("  .mag_img_filter = %s\n",
                         debug_dump_tex_filter(key->sampler[i].mag_img_filter, TRUE));
            if(key->sampler[i].compare_mode != PIPE_TEX_COMPARE_NONE)
               debug_printf("  .compare_func = %s\n", debug_dump_func(key->sampler[i].compare_func, TRUE));
            debug_printf("  .normalized_coords = %u\n", key->sampler[i].normalized_coords);
            debug_printf("  .prefilter = %u\n", key->sampler[i].prefilter);
         }
      }
   }

   variant = CALLOC_STRUCT(lp_fragment_shader_variant);
   if(!variant)
      return NULL;

   variant->shader = shader;
   memcpy(&variant->key, key, sizeof *key);

   /* TODO: actually pick these based on the fs and color buffer
    * characteristics. */

   memset(&fs_type, 0, sizeof fs_type);
   fs_type.floating = TRUE; /* floating point values */
   fs_type.sign = TRUE;     /* values are signed */
   fs_type.norm = FALSE;    /* values are not limited to [0,1] or [-1,1] */
   fs_type.width = 32;      /* 32-bit float */
   fs_type.length = 4;      /* 4 element per vector */
   num_fs = 4;

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
   fs_vec_type = lp_build_vec_type(fs_type);
   fs_int_vec_type = lp_build_int_vec_type(fs_type);

   blend_vec_type = lp_build_vec_type(blend_type);
   blend_int_vec_type = lp_build_int_vec_type(blend_type);

   arg_types[0] = screen->context_ptr_type;            /* context */
   arg_types[1] = LLVMInt32Type();                     /* x */
   arg_types[2] = LLVMInt32Type();                     /* y */
   arg_types[3] = LLVMPointerType(fs_elem_type, 0);    /* a0 */
   arg_types[4] = LLVMPointerType(fs_elem_type, 0);    /* dadx */
   arg_types[5] = LLVMPointerType(fs_elem_type, 0);    /* dady */
   arg_types[6] = LLVMPointerType(fs_int_vec_type, 0); /* mask */
   arg_types[7] = LLVMPointerType(blend_vec_type, 0);  /* color */
   arg_types[8] = LLVMPointerType(fs_int_vec_type, 0); /* depth */

   func_type = LLVMFunctionType(LLVMVoidType(), arg_types, Elements(arg_types), 0);

   variant->function = LLVMAddFunction(screen->module, "shader", func_type);
   LLVMSetFunctionCallConv(variant->function, LLVMCCallConv);
   for(i = 0; i < Elements(arg_types); ++i)
      if(LLVMGetTypeKind(arg_types[i]) == LLVMPointerTypeKind)
         LLVMAddAttribute(LLVMGetParam(variant->function, i), LLVMNoAliasAttribute);

   context_ptr  = LLVMGetParam(variant->function, 0);
   x            = LLVMGetParam(variant->function, 1);
   y            = LLVMGetParam(variant->function, 2);
   a0_ptr       = LLVMGetParam(variant->function, 3);
   dadx_ptr     = LLVMGetParam(variant->function, 4);
   dady_ptr     = LLVMGetParam(variant->function, 5);
   mask_ptr     = LLVMGetParam(variant->function, 6);
   color_ptr    = LLVMGetParam(variant->function, 7);
   depth_ptr    = LLVMGetParam(variant->function, 8);

   lp_build_name(context_ptr, "context");
   lp_build_name(x, "x");
   lp_build_name(y, "y");
   lp_build_name(a0_ptr, "a0");
   lp_build_name(dadx_ptr, "dadx");
   lp_build_name(dady_ptr, "dady");
   lp_build_name(mask_ptr, "mask");
   lp_build_name(color_ptr, "color");
   lp_build_name(depth_ptr, "depth");

   /*
    * Function body
    */

   block = LLVMAppendBasicBlock(variant->function, "entry");
   builder = LLVMCreateBuilder();
   LLVMPositionBuilderAtEnd(builder, block);

   generate_pos0(builder, x, y, &x0, &y0);

   lp_build_interp_soa_init(&interp, shader->base.tokens, builder, fs_type,
                            a0_ptr, dadx_ptr, dady_ptr,
                            x0, y0, 2, 0);

   /* code generated texture sampling */
   sampler = lp_llvm_sampler_soa_create(key->sampler, context_ptr);

   for(i = 0; i < num_fs; ++i) {
      LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), i, 0);
      LLVMValueRef out_color[NUM_CHANNELS];
      LLVMValueRef depth_ptr_i;

      if(i != 0)
         lp_build_interp_soa_update(&interp);

      fs_mask[i] = LLVMBuildLoad(builder, LLVMBuildGEP(builder, mask_ptr, &index, 1, ""), "");
      depth_ptr_i = LLVMBuildGEP(builder, depth_ptr, &index, 1, "");

      generate_fs(lp, shader, key,
                  builder,
                  fs_type,
                  context_ptr,
                  i,
                  &interp,
                  sampler,
                  &fs_mask[i],
                  out_color,
                  depth_ptr_i);

      for(chan = 0; chan < NUM_CHANNELS; ++chan)
         fs_out_color[chan][i] = out_color[chan];
   }

   sampler->destroy(sampler);

   /* 
    * Convert the fs's output color and mask to fit to the blending type. 
    */

   for(chan = 0; chan < NUM_CHANNELS; ++chan) {
      lp_build_conv(builder, fs_type, blend_type,
                    fs_out_color[chan], num_fs,
                    &blend_in_color[chan], 1);
      lp_build_name(blend_in_color[chan], "color.%c", "rgba"[chan]);

   }

   lp_build_conv_mask(builder, fs_type, blend_type,
                      fs_mask, num_fs,
                      &blend_mask, 1);

   /*
    * Blending.
    */

   generate_blend(&key->blend,
                  builder,
                  blend_type,
                  context_ptr,
                  blend_mask,
                  blend_in_color,
                  color_ptr);

   LLVMBuildRetVoid(builder);

   LLVMDisposeBuilder(builder);

   /*
    * Translate the LLVM IR into machine code.
    */

#ifdef DEBUG
   if(LLVMVerifyFunction(variant->function, LLVMPrintMessageAction)) {
      LLVMDumpValue(variant->function);
      assert(0);
   }
#endif

   LLVMRunFunctionPassManager(screen->pass, variant->function);

   if (LP_DEBUG & DEBUG_JIT) {
      LLVMDumpValue(variant->function);
      debug_printf("\n");
   }

   variant->jit_function = (lp_jit_frag_func)LLVMGetPointerToGlobal(screen->engine, variant->function);

   if (LP_DEBUG & DEBUG_ASM)
      lp_disassemble(variant->jit_function);

   variant->next = shader->variants;
   shader->variants = variant;

   return variant;
}


void *
llvmpipe_create_fs_state(struct pipe_context *pipe,
                         const struct pipe_shader_state *templ)
{
   struct lp_fragment_shader *shader;

   shader = CALLOC_STRUCT(lp_fragment_shader);
   if (!shader)
      return NULL;

   /* get/save the summary info for this shader */
   tgsi_scan_shader(templ->tokens, &shader->info);

   /* we need to keep a local copy of the tokens */
   shader->base.tokens = tgsi_dup_tokens(templ->tokens);

   return shader;
}


void
llvmpipe_bind_fs_state(struct pipe_context *pipe, void *fs)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);

   if (llvmpipe->fs == fs)
      return;

   draw_flush(llvmpipe->draw);

   llvmpipe->fs = fs;

   llvmpipe->dirty |= LP_NEW_FS;
}


void
llvmpipe_delete_fs_state(struct pipe_context *pipe, void *fs)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   struct llvmpipe_screen *screen = llvmpipe_screen(pipe->screen);
   struct lp_fragment_shader *shader = fs;
   struct lp_fragment_shader_variant *variant;

   assert(fs != llvmpipe->fs);
   (void) llvmpipe;

   variant = shader->variants;
   while(variant) {
      struct lp_fragment_shader_variant *next = variant->next;

      if(variant->function) {
         if(variant->jit_function)
            LLVMFreeMachineCodeForFunction(screen->engine, variant->function);
         LLVMDeleteFunction(variant->function);
      }

      FREE(variant);

      variant = next;
   }

   FREE((void *) shader->base.tokens);
   FREE(shader);
}



void
llvmpipe_set_constant_buffer(struct pipe_context *pipe,
                             uint shader, uint index,
                             struct pipe_buffer *constants)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   unsigned size = constants ? constants->size : 0;
   const void *data = constants ? llvmpipe_buffer(constants)->data : NULL;

   assert(shader < PIPE_SHADER_TYPES);
   assert(index == 0);

   draw_flush(llvmpipe->draw);

   /* note: reference counting */
   pipe_buffer_reference(&llvmpipe->constants[shader], constants);

   if(shader == PIPE_SHADER_FRAGMENT) {
      llvmpipe->jit_context.constants = data;
   }

   if(shader == PIPE_SHADER_VERTEX) {
      draw_set_mapped_constant_buffer(llvmpipe->draw, PIPE_SHADER_VERTEX, 0,
                                      data, size);
   }

   llvmpipe->dirty |= LP_NEW_CONSTANTS;
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

   memset(key, 0, sizeof *key);

   if(lp->framebuffer.zsbuf &&
      lp->depth_stencil->depth.enabled) {
      key->zsbuf_format = lp->framebuffer.zsbuf->format;
      memcpy(&key->depth, &lp->depth_stencil->depth, sizeof key->depth);
   }

   key->alpha.enabled = lp->depth_stencil->alpha.enabled;
   if(key->alpha.enabled)
      key->alpha.func = lp->depth_stencil->alpha.func;
   /* alpha.ref_value is passed in jit_context */

   if(lp->framebuffer.cbufs[0]) {
      const struct util_format_description *format_desc;
      unsigned chan;

      memcpy(&key->blend, lp->blend, sizeof key->blend);

      format_desc = util_format_description(lp->framebuffer.cbufs[0]->format);
      assert(format_desc->layout == UTIL_FORMAT_COLORSPACE_RGB ||
             format_desc->layout == UTIL_FORMAT_COLORSPACE_SRGB);

      /* mask out color channels not present in the color buffer */
      for(chan = 0; chan < 4; ++chan) {
         enum util_format_swizzle swizzle = format_desc->swizzle[chan];
         if(swizzle > 4)
            key->blend.rt[0].colormask &= ~(1 << chan);
      }
   }

   for(i = 0; i < PIPE_MAX_SAMPLERS; ++i)
      if(shader->info.file_mask[TGSI_FILE_SAMPLER] & (1 << i))
         lp_sampler_static_state(&key->sampler[i], lp->texture[i], lp->sampler[i]);
}


void 
llvmpipe_update_fs(struct llvmpipe_context *lp)
{
   struct lp_fragment_shader *shader = lp->fs;
   struct lp_fragment_shader_variant_key key;
   struct lp_fragment_shader_variant *variant;

   make_variant_key(lp, shader, &key);

   variant = shader->variants;
   while(variant) {
      if(memcmp(&variant->key, &key, sizeof key) == 0)
         break;

      variant = variant->next;
   }

   if(!variant)
      variant = generate_fragment(lp, shader, &key);

   shader->current = variant;
}
