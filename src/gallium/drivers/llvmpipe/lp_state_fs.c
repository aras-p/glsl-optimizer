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
#include "util/u_memory.h"
#include "util/u_format.h"
#include "util/u_debug_dump.h"
#include "pipe/internal/p_winsys_screen.h"
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
#include "lp_setup.h"
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

   /* Pick the depth type. */
   dst_type = lp_depth_type(format_desc, src_type.width*src_type.length);

   /* FIXME: Cope with a depth test type with a different bit width. */
   assert(dst_type.width == src_type.width);
   assert(dst_type.length == src_type.length);

#if 1
   src = lp_build_clamped_float_to_unsigned_norm(builder,
                                                 src_type,
                                                 dst_type.width,
                                                 src);
#else
   lp_build_conv(builder, src_type, dst_type, &src, 1, &src, 1);
#endif

   lp_build_depth_test(builder,
                       &key->depth,
                       dst_type,
                       format_desc,
                       mask,
                       src,
                       dst_ptr);
}


/**
 * Generate the code to do inside/outside triangle testing for the
 * four pixels in a 2x2 quad.  This will set the four elements of the
 * quad mask vector to 0 or ~0.
 * \param i  which quad of the quad group to test, in [0,3]
 */
static void
generate_tri_edge_mask(LLVMBuilderRef builder,
                       unsigned i,
                       LLVMValueRef *mask,      /* ivec4, out */
                       LLVMValueRef c0,         /* int32 */
                       LLVMValueRef c1,         /* int32 */
                       LLVMValueRef c2,         /* int32 */
                       LLVMValueRef step0_ptr,  /* ivec4 */
                       LLVMValueRef step1_ptr,  /* ivec4 */
                       LLVMValueRef step2_ptr)  /* ivec4 */
{
   /*
     c0_vec = splat(c0)
     c1_vec = splat(c1)
     c2_vec = splat(c2)
     m0_vec = step0_ptr[i] > c0_vec
     m1_vec = step1_ptr[i] > c1_vec
     m2_vec = step2_ptr[i] > c2_vec
     mask = m0_vec & m1_vec & m2_vec
    */
   struct lp_type i32_type;
   LLVMTypeRef i32vec4_type;

   LLVMValueRef index;
   LLVMValueRef c0_vec, c1_vec, c2_vec;
   LLVMValueRef step0_vec, step1_vec, step2_vec;
   LLVMValueRef m0_vec, m1_vec, m2_vec;
   LLVMValueRef m;

   assert(i < 4);
   
   /* int32 vector type */
   memset(&i32_type, 0, sizeof i32_type);
   i32_type.floating = FALSE; /* values are integers */
   i32_type.sign = TRUE;      /* values are signed */
   i32_type.norm = FALSE;     /* values are not normalized */
   i32_type.width = 32;       /* 32-bit int values */
   i32_type.length = 4;       /* 4 elements per vector */

   i32vec4_type = lp_build_int32_vec4_type();

   /* c0_vec = {c0, c0, c0, c0}
    * Note that we emit this code four times but LLVM optimizes away
    * three instances of it.
    */
   c0_vec = lp_build_broadcast(builder, i32vec4_type, c0);
   c1_vec = lp_build_broadcast(builder, i32vec4_type, c1);
   c2_vec = lp_build_broadcast(builder, i32vec4_type, c2);

   lp_build_name(c0_vec, "edgeconst0vec");
   lp_build_name(c1_vec, "edgeconst1vec");
   lp_build_name(c2_vec, "edgeconst2vec");

   index = LLVMConstInt(LLVMInt32Type(), i, 0);
   step0_vec = LLVMBuildLoad(builder, LLVMBuildGEP(builder, step0_ptr, &index, 1, ""), "");
   step1_vec = LLVMBuildLoad(builder, LLVMBuildGEP(builder, step1_ptr, &index, 1, ""), "");
   step2_vec = LLVMBuildLoad(builder, LLVMBuildGEP(builder, step2_ptr, &index, 1, ""), "");

   lp_build_name(step0_vec, "step0vec");
   lp_build_name(step1_vec, "step1vec");
   lp_build_name(step2_vec, "step2vec");

   m0_vec = lp_build_compare(builder, i32_type, PIPE_FUNC_GREATER, step0_vec, c0_vec);
   m1_vec = lp_build_compare(builder, i32_type, PIPE_FUNC_GREATER, step1_vec, c1_vec);
   m2_vec = lp_build_compare(builder, i32_type, PIPE_FUNC_GREATER, step2_vec, c2_vec);

   m = LLVMBuildAnd(builder, m0_vec, m1_vec, "");
   m = LLVMBuildAnd(builder, m, m2_vec, "");

   lp_build_name(m, "inoutmaskvec");

   *mask = m;

   /*
    * if mask = {0,0,0,0} skip quad
    */
}


/**
 * Generate the fragment shader, depth/stencil test, and alpha tests.
 * \param i  which quad in the tile, in range [0,3]
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
            LLVMValueRef depth_ptr,
            LLVMValueRef c0,
            LLVMValueRef c1,
            LLVMValueRef c2,
            LLVMValueRef step0_ptr,
            LLVMValueRef step1_ptr,
            LLVMValueRef step2_ptr)
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

   assert(i < 4);

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

   /* do triangle edge testing */
   generate_tri_edge_mask(builder, i, pmask,
                          c0, c1, c2, step0_ptr, step1_ptr, step2_ptr);

   /* 'mask' will control execution based on quad's pixel alive/killed state */
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
      if(blend->colormask & (1 << chan)) {
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
   LLVMTypeRef arg_types[14];
   LLVMTypeRef func_type;
   LLVMTypeRef int32_vec4_type = lp_build_int32_vec4_type();
   LLVMValueRef context_ptr;
   LLVMValueRef x;
   LLVMValueRef y;
   LLVMValueRef a0_ptr;
   LLVMValueRef dadx_ptr;
   LLVMValueRef dady_ptr;
   LLVMValueRef color_ptr;
   LLVMValueRef depth_ptr;
   LLVMValueRef c0, c1, c2, step0_ptr, step1_ptr, step2_ptr;
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
      else if(key->blend.blend_enable) {
         debug_printf("blend.rgb_func = %s\n",   debug_dump_blend_func  (key->blend.rgb_func, TRUE));
         debug_printf("blend.rgb_src_factor = %s\n",   debug_dump_blend_factor(key->blend.rgb_src_factor, TRUE));
         debug_printf("blend.rgb_dst_factor = %s\n",   debug_dump_blend_factor(key->blend.rgb_dst_factor, TRUE));
         debug_printf("blend.alpha_func = %s\n",       debug_dump_blend_func  (key->blend.alpha_func, TRUE));
         debug_printf("blend.alpha_src_factor = %s\n", debug_dump_blend_factor(key->blend.alpha_src_factor, TRUE));
         debug_printf("blend.alpha_dst_factor = %s\n", debug_dump_blend_factor(key->blend.alpha_dst_factor, TRUE));
      }
      debug_printf("blend.colormask = 0x%x\n", key->blend.colormask);
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
   arg_types[6] = LLVMPointerType(blend_vec_type, 0);  /* color */
   arg_types[7] = LLVMPointerType(fs_int_vec_type, 0); /* depth */
   arg_types[8] = LLVMInt32Type();                     /* c0 */
   arg_types[9] = LLVMInt32Type();                    /* c1 */
   arg_types[10] = LLVMInt32Type();                    /* c2 */
   /* Note: the step arrays are built as int32[16] but we interpret
    * them here as int32_vec4[4].
    */
   arg_types[11] = LLVMPointerType(int32_vec4_type, 0);/* step0 */
   arg_types[12] = LLVMPointerType(int32_vec4_type, 0);/* step1 */
   arg_types[13] = LLVMPointerType(int32_vec4_type, 0);/* step2 */

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
   color_ptr    = LLVMGetParam(variant->function, 6);
   depth_ptr    = LLVMGetParam(variant->function, 7);
   c0           = LLVMGetParam(variant->function, 8);
   c1           = LLVMGetParam(variant->function, 9);
   c2           = LLVMGetParam(variant->function, 10);
   step0_ptr    = LLVMGetParam(variant->function, 11);
   step1_ptr    = LLVMGetParam(variant->function, 12);
   step2_ptr    = LLVMGetParam(variant->function, 13);

   lp_build_name(context_ptr, "context");
   lp_build_name(x, "x");
   lp_build_name(y, "y");
   lp_build_name(a0_ptr, "a0");
   lp_build_name(dadx_ptr, "dadx");
   lp_build_name(dady_ptr, "dady");
   lp_build_name(color_ptr, "color");
   lp_build_name(depth_ptr, "depth");
   lp_build_name(c0, "c0");
   lp_build_name(c1, "c1");
   lp_build_name(c2, "c2");
   lp_build_name(step0_ptr, "step0");
   lp_build_name(step1_ptr, "step1");
   lp_build_name(step2_ptr, "step2");

   /*
    * Function body
    */

   block = LLVMAppendBasicBlock(variant->function, "entry");
   builder = LLVMCreateBuilder();
   LLVMPositionBuilderAtEnd(builder, block);

   generate_pos0(builder, x, y, &x0, &y0);

   lp_build_interp_soa_init(&interp, shader->base.tokens, builder, fs_type,
                            a0_ptr, dadx_ptr, dady_ptr,
                            x0, y0);

   /* code generated texture sampling */
   sampler = lp_llvm_sampler_soa_create(key->sampler, context_ptr);

   /* loop over quads in the block */
   for(i = 0; i < num_fs; ++i) {
      LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), i, 0);
      LLVMValueRef out_color[NUM_CHANNELS];
      LLVMValueRef depth_ptr_i;

      if(i != 0)
         lp_build_interp_soa_update(&interp, i);

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
                  c0, c1, c2,
                  step0_ptr, step1_ptr, step2_ptr);

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

   if(LLVMVerifyFunction(variant->function, LLVMPrintMessageAction)) {
      LLVMDumpValue(variant->function);
      abort();
   }

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

   llvmpipe->fs = (struct lp_fragment_shader *) fs;

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
                             const struct pipe_constant_buffer *constants)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   struct pipe_buffer *buffer = constants ? constants->buffer : NULL;
   unsigned size = buffer ? buffer->size : 0;
   const void *data = buffer ? llvmpipe_buffer(buffer)->data : NULL;

   assert(shader < PIPE_SHADER_TYPES);
   assert(index == 0);

   if(llvmpipe->constants[shader].buffer == buffer)
      return;

   if(shader == PIPE_SHADER_VERTEX)
      draw_flush(llvmpipe->draw);

   /* note: reference counting */
   pipe_buffer_reference(&llvmpipe->constants[shader].buffer, buffer);

   if(shader == PIPE_SHADER_VERTEX) {
      draw_set_mapped_constant_buffer(llvmpipe->draw, data, size);
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
            key->blend.colormask &= ~(1 << chan);
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

   lp_setup_set_fs_function(lp->setup, 
                            shader->current->jit_function);
}
