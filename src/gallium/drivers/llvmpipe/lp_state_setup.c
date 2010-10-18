/**************************************************************************
 *
 * Copyright 2010 VMware.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_simple_list.h"
#include "os/os_time.h"
#include "gallivm/lp_bld_debug.h"
#include "gallivm/lp_bld_init.h"
#include "gallivm/lp_bld_intr.h"
#include <llvm-c/Analysis.h>	/* for LLVMVerifyFunction */

#include "lp_perf.h"
#include "lp_debug.h"
#include "lp_flush.h"
#include "lp_screen.h"
#include "lp_context.h"
#include "lp_setup_context.h"
#include "lp_rast.h"
#include "lp_state.h"
#include "lp_state_fs.h"
#include "lp_state_setup.h"



/* currently organized to interpolate full float[4] attributes even
 * when some elements are unused.  Later, can pack vertex data more
 * closely.
 */


struct lp_setup_args
{
   /* Function arguments:
    */
   LLVMValueRef v0;
   LLVMValueRef v1;
   LLVMValueRef v2;
   LLVMValueRef facing;		/* boolean */
   LLVMValueRef a0;
   LLVMValueRef dadx;
   LLVMValueRef dady;

   /* Derived:
    */
   LLVMValueRef x0_center;
   LLVMValueRef y0_center;
   LLVMValueRef dy20_ooa;
   LLVMValueRef dy01_ooa;
   LLVMValueRef dx20_ooa;
   LLVMValueRef dx01_ooa;
};

static LLVMTypeRef type4f(void)
{
   return LLVMVectorType(LLVMFloatType(), 4);
}


/* Equivalent of _mm_setr_ps(a,b,c,d)
 */
static LLVMValueRef vec4f(LLVMBuilderRef bld,
			  LLVMValueRef a, LLVMValueRef b, LLVMValueRef c, LLVMValueRef d,
			  const char *name)
{
   LLVMValueRef i0 = LLVMConstInt(LLVMInt32Type(), 0, 0);
   LLVMValueRef i1 = LLVMConstInt(LLVMInt32Type(), 1, 0);
   LLVMValueRef i2 = LLVMConstInt(LLVMInt32Type(), 2, 0);
   LLVMValueRef i3 = LLVMConstInt(LLVMInt32Type(), 3, 0);

   LLVMValueRef res = LLVMGetUndef(type4f());

   res = LLVMBuildInsertElement(bld, res, a, i0, "");
   res = LLVMBuildInsertElement(bld, res, b, i1, "");
   res = LLVMBuildInsertElement(bld, res, c, i2, "");
   res = LLVMBuildInsertElement(bld, res, d, i3, name);

   return res;
}

/* Equivalent of _mm_set1_ps(a)
 */
static LLVMValueRef vec4f_from_scalar(LLVMBuilderRef bld,
				      LLVMValueRef a,
				      const char *name)
{
   LLVMValueRef res = LLVMGetUndef(type4f());
   int i;

   for(i = 0; i < 4; ++i) {
      LLVMValueRef index = LLVMConstInt(LLVMInt32Type(), i, 0);
      res = LLVMBuildInsertElement(bld, res, a, index, i == 3 ? name : "");
   }

   return res;
}

static void
store_coef(LLVMBuilderRef builder,
	   struct lp_setup_args *args,
	   unsigned slot,
	   LLVMValueRef a0,
	   LLVMValueRef dadx,
	   LLVMValueRef dady)
{
   LLVMValueRef idx = LLVMConstInt(LLVMInt32Type(), slot, 0);
   
   LLVMBuildStore(builder,
		  a0, 
		  LLVMBuildGEP(builder, args->a0, &idx, 1, ""));

   LLVMBuildStore(builder,
		  dadx, 
		  LLVMBuildGEP(builder, args->dadx, &idx, 1, ""));

   LLVMBuildStore(builder,
		  dady, 
		  LLVMBuildGEP(builder, args->dady, &idx, 1, ""));
}



static void 
emit_constant_coef4( LLVMBuilderRef builder,
		     struct lp_setup_args *args,
		     unsigned slot,
		     LLVMValueRef vert,
		     unsigned attr)
{
   LLVMValueRef zero      = LLVMConstReal(LLVMFloatType(), 0.0);
   LLVMValueRef zerovec   = vec4f_from_scalar(builder, zero, "zero");
   LLVMValueRef idx       = LLVMConstInt(LLVMInt32Type(), attr, 0);
   LLVMValueRef attr_ptr  = LLVMBuildGEP(builder, vert, &idx, 1, "attr_ptr");
   LLVMValueRef vert_attr = LLVMBuildLoad(builder, attr_ptr, "vert_attr");

   store_coef(builder, args, slot, vert_attr, zerovec, zerovec);
}



/**
 * Setup the fragment input attribute with the front-facing value.
 * \param frontface  is the triangle front facing?
 */
static void 
emit_facing_coef( LLVMBuilderRef builder,
		  struct lp_setup_args *args,
		  unsigned slot )
{
   LLVMValueRef a0_0 = args->facing;
   LLVMValueRef a0_0f = LLVMBuildSIToFP(builder, a0_0, LLVMFloatType(), "");
   LLVMValueRef zero = LLVMConstReal(LLVMFloatType(), 0.0);
   LLVMValueRef a0      = vec4f(builder, a0_0f, zero, zero, zero, "facing");
   LLVMValueRef zerovec = vec4f_from_scalar(builder, zero, "zero");

   store_coef(builder, args, slot, a0, zerovec, zerovec);
}


static LLVMValueRef
vert_attrib(LLVMBuilderRef b,
	    LLVMValueRef vert,
	    int attr,
	    int elem,
	    const char *name)
{
   LLVMValueRef idx[2];
   idx[0] = LLVMConstInt(LLVMInt32Type(), attr, 0);
   idx[1] = LLVMConstInt(LLVMInt32Type(), elem, 0);
   return LLVMBuildLoad(b, LLVMBuildGEP(b, vert, idx, 2, ""), name);
}



static void 
emit_coef4( LLVMBuilderRef b,
	    struct lp_setup_args *args,
	    unsigned slot,
	    LLVMValueRef a0,
	    LLVMValueRef a1,
	    LLVMValueRef a2)
{
   LLVMValueRef dy20_ooa = args->dy20_ooa;
   LLVMValueRef dy01_ooa = args->dy01_ooa;
   LLVMValueRef dx20_ooa = args->dx20_ooa;
   LLVMValueRef dx01_ooa = args->dx01_ooa;
   LLVMValueRef x0_center = args->x0_center;
   LLVMValueRef y0_center = args->y0_center;

   /* XXX: using fsub, fmul on vector types -- does this work??
    */
   LLVMValueRef da01 = LLVMBuildFSub(b, a0, a1, "da01");
   LLVMValueRef da20 = LLVMBuildFSub(b, a2, a0, "da20");

   /* Calculate dadx (vec4f)
    */
   LLVMValueRef da01_dy20_ooa = LLVMBuildFMul(b, da01, dy20_ooa, "da01_dy20_ooa");
   LLVMValueRef da20_dy01_ooa = LLVMBuildFMul(b, da20, dy01_ooa, "da20_dy01_ooa");
   LLVMValueRef dadx          = LLVMBuildFSub(b, da01_dy20_ooa, da20_dy01_ooa, "dadx");

   /* Calculate dady (vec4f)
    */
   LLVMValueRef da01_dx20_ooa = LLVMBuildFMul(b, da01, dx20_ooa, "da01_dx20_ooa");
   LLVMValueRef da20_dx01_ooa = LLVMBuildFMul(b, da20, dx01_ooa, "da20_dx01_ooa");
   LLVMValueRef dady          = LLVMBuildFSub(b, da20_dx01_ooa, da01_dx20_ooa, "dady");

   /* Calculate a0 - the attribute value at the origin
    */
   LLVMValueRef dadx_x0       = LLVMBuildFMul(b, dadx, x0_center, "dadx_x0"); 
   LLVMValueRef dady_y0       = LLVMBuildFMul(b, dady, y0_center, "dady_y0"); 
   LLVMValueRef attr_v0       = LLVMBuildFAdd(b, dadx_x0, dady_y0, "attr_v0"); 
   LLVMValueRef attr_0        = LLVMBuildFSub(b, a0, attr_v0, "attr_0"); 

   store_coef(b, args, slot, attr_0, dadx, dady);
}


static void 
emit_linear_coef( LLVMBuilderRef b,
		  struct lp_setup_args *args,
		  unsigned slot,
		  unsigned vert_attr)
{
   LLVMValueRef idx = LLVMConstInt(LLVMInt32Type(), vert_attr, 0);

   LLVMValueRef a0 = LLVMBuildLoad(b, LLVMBuildGEP(b, args->v0, &idx, 1, ""), "v0a");
   LLVMValueRef a1 = LLVMBuildLoad(b, LLVMBuildGEP(b, args->v1, &idx, 1, ""), "v1a");
   LLVMValueRef a2 = LLVMBuildLoad(b, LLVMBuildGEP(b, args->v2, &idx, 1, ""), "v2a");

   emit_coef4(b, args, slot, a0, a1, a2);
}



/**
 * Compute a0, dadx and dady for a perspective-corrected interpolant,
 * for a triangle.
 * We basically multiply the vertex value by 1/w before computing
 * the plane coefficients (a0, dadx, dady).
 * Later, when we compute the value at a particular fragment position we'll
 * divide the interpolated value by the interpolated W at that fragment.
 */
static void 
emit_perspective_coef( LLVMBuilderRef b,
		       struct lp_setup_args *args,
		       unsigned slot,
		       unsigned vert_attr)
{
   /* premultiply by 1/w  (v[0][3] is always 1/w):
    */
   LLVMValueRef idx = LLVMConstInt(LLVMInt32Type(), vert_attr, 0);

   LLVMValueRef v0a = LLVMBuildLoad(b, LLVMBuildGEP(b, args->v0, &idx, 1, ""), "v0a");
   LLVMValueRef v1a = LLVMBuildLoad(b, LLVMBuildGEP(b, args->v1, &idx, 1, ""), "v1a");
   LLVMValueRef v2a = LLVMBuildLoad(b, LLVMBuildGEP(b, args->v2, &idx, 1, ""), "v2a");

   LLVMValueRef v0_oow = vec4f_from_scalar(b, vert_attrib(b, args->v0, 0, 3, ""), "v0_oow");
   LLVMValueRef v1_oow = vec4f_from_scalar(b, vert_attrib(b, args->v1, 0, 3, ""), "v1_oow");
   LLVMValueRef v2_oow = vec4f_from_scalar(b, vert_attrib(b, args->v2, 0, 3, ""), "v2_oow");

   LLVMValueRef v0_oow_v0a = LLVMBuildFMul(b, v0a, v0_oow, "v0_oow_v0a");
   LLVMValueRef v1_oow_v1a = LLVMBuildFMul(b, v1a, v1_oow, "v1_oow_v1a");
   LLVMValueRef v2_oow_v2a = LLVMBuildFMul(b, v2a, v2_oow, "v2_oow_v2a");

   emit_coef4(b, args, slot, v0_oow_v0a, v1_oow_v1a, v2_oow_v2a);
}


static void
emit_position_coef( LLVMBuilderRef builder,
		    struct lp_setup_args *args,
		    int slot, int attrib )
{
   emit_linear_coef(builder, args, slot, attrib);
}




/**
 * Compute the inputs-> dadx, dady, a0 values.
 */
static void 
emit_tri_coef( LLVMBuilderRef builder,
	       const struct lp_setup_variant_key *key,
	       struct lp_setup_args *args )
{
   unsigned slot;

   /* The internal position input is in slot zero:
    */
   emit_position_coef(builder, args, 0, 0);

   /* setup interpolation for all the remaining attributes:
    */
   for (slot = 0; slot < key->num_inputs; slot++) {
      unsigned vert_attr = key->inputs[slot].src_index;

      switch (key->inputs[slot].interp) {
      case LP_INTERP_CONSTANT:
	 if (key->flatshade_first) {
	    emit_constant_coef4(builder, args, slot+1, args->v0, vert_attr);
	 }
	 else {
	    emit_constant_coef4(builder, args, slot+1, args->v2, vert_attr);
	 }
	 break;

      case LP_INTERP_LINEAR:
	 emit_linear_coef(builder, args, slot+1, vert_attr);
         break;

      case LP_INTERP_PERSPECTIVE:
	 emit_perspective_coef(builder, args, slot+1, vert_attr);
         break;

      case LP_INTERP_POSITION:
         /*
          * The generated pixel interpolators will pick up the coeffs from
          * slot 0.
          */
         break;

      case LP_INTERP_FACING:
         emit_facing_coef(builder, args, slot+1);
         break;

      default:
         assert(0);
      }
   }
}


/* XXX: This is generic code, share with fs/vs codegen:
 */
static lp_jit_setup_triangle
finalize_function(struct llvmpipe_screen *screen,
		  LLVMBuilderRef builder,
		  LLVMValueRef function)
{
   void *f;

   /* Verify the LLVM IR.  If invalid, dump and abort */
#ifdef DEBUG
   if (LLVMVerifyFunction(function, LLVMPrintMessageAction)) {
      if (1)
         lp_debug_dump_value(function);
      abort();
   }
#endif

   /* Apply optimizations to LLVM IR */
   LLVMRunFunctionPassManager(screen->pass, function);

   if (gallivm_debug & GALLIVM_DEBUG_IR)
   {
      /* Print the LLVM IR to stderr */
      lp_debug_dump_value(function);
      debug_printf("\n");
   }

   /*
    * Translate the LLVM IR into machine code.
    */
   f = LLVMGetPointerToGlobal(screen->engine, function);

   if (gallivm_debug & GALLIVM_DEBUG_ASM)
   {
      lp_disassemble(f);
   }

   lp_func_delete_body(function);

   return f;
}

/* XXX: Generic code:
 */
static void
lp_emit_emms(LLVMBuilderRef builder)
{
#ifdef PIPE_ARCH_X86
   /* Avoid corrupting the FPU stack on 32bit OSes. */
   lp_build_intrinsic(builder, "llvm.x86.mmx.emms", LLVMVoidType(), NULL, 0);
#endif
}


/* XXX: generic code:
 */
static void
set_noalias(LLVMBuilderRef builder,
	    LLVMValueRef function,
	    const LLVMTypeRef *arg_types,
	    int nr_args)
{
   int i;
   for(i = 0; i < Elements(arg_types); ++i)
      if(LLVMGetTypeKind(arg_types[i]) == LLVMPointerTypeKind)
         LLVMAddAttribute(LLVMGetParam(function, i),
			  LLVMNoAliasAttribute);
}

static void
init_args(LLVMBuilderRef b, 
	  struct lp_setup_args *args,
	  const struct lp_setup_variant *variant)
{
   LLVMValueRef v0_x = vert_attrib(b, args->v0, 0, 0, "v0_x");
   LLVMValueRef v0_y = vert_attrib(b, args->v0, 0, 1, "v0_y");

   LLVMValueRef v1_x = vert_attrib(b, args->v1, 0, 0, "v1_x");
   LLVMValueRef v1_y = vert_attrib(b, args->v1, 0, 1, "v1_y");

   LLVMValueRef v2_x = vert_attrib(b, args->v2, 0, 0, "v2_x");
   LLVMValueRef v2_y = vert_attrib(b, args->v2, 0, 1, "v2_y");

   LLVMValueRef pixel_center = LLVMConstReal(LLVMFloatType(),
					     variant->key.pixel_center_half ? 0.5 : 0);

   LLVMValueRef x0_center = LLVMBuildFSub(b, v0_x, pixel_center, "x0_center" );
   LLVMValueRef y0_center = LLVMBuildFSub(b, v0_y, pixel_center, "y0_center" );
   
   LLVMValueRef dx01 = LLVMBuildFSub(b, v0_x, v1_x, "dx01");
   LLVMValueRef dy01 = LLVMBuildFSub(b, v0_y, v1_y, "dy01");
   LLVMValueRef dx20 = LLVMBuildFSub(b, v2_x, v0_x, "dx20");
   LLVMValueRef dy20 = LLVMBuildFSub(b, v2_y, v0_y, "dy20");

   LLVMValueRef one  = LLVMConstReal(LLVMFloatType(), 1.0);
   LLVMValueRef e    = LLVMBuildFMul(b, dx01, dy20, "e");
   LLVMValueRef f    = LLVMBuildFMul(b, dx20, dy01, "f");
   LLVMValueRef ooa  = LLVMBuildFDiv(b, one, LLVMBuildFSub(b, e, f, ""), "ooa");

   LLVMValueRef dy20_ooa = LLVMBuildFMul(b, dy20, ooa, "dy20_ooa");
   LLVMValueRef dy01_ooa = LLVMBuildFMul(b, dy01, ooa, "dy01_ooa");
   LLVMValueRef dx20_ooa = LLVMBuildFMul(b, dx20, ooa, "dx20_ooa");
   LLVMValueRef dx01_ooa = LLVMBuildFMul(b, dx01, ooa, "dx01_ooa");

   args->dy20_ooa  = vec4f_from_scalar(b, dy20_ooa, "dy20_ooa_4f");
   args->dy01_ooa  = vec4f_from_scalar(b, dy01_ooa, "dy01_ooa_4f");

   args->dx20_ooa  = vec4f_from_scalar(b, dx20_ooa, "dx20_ooa_4f");
   args->dx01_ooa  = vec4f_from_scalar(b, dx01_ooa, "dx01_ooa_4f");

   args->x0_center = vec4f_from_scalar(b, x0_center, "x0_center_4f");
   args->y0_center = vec4f_from_scalar(b, y0_center, "y0_center_4f");
}

/**
 * Generate the runtime callable function for the coefficient calculation.
 *
 */
static struct lp_setup_variant *
generate_setup_variant(struct llvmpipe_screen *screen,
		       struct lp_setup_variant_key *key)
{
   struct lp_setup_variant *variant = NULL;
   struct lp_setup_args args;
   char func_name[256];
   LLVMTypeRef vec4f_type;
   LLVMTypeRef func_type;
   LLVMTypeRef arg_types[8];
   LLVMBasicBlockRef block;
   LLVMBuilderRef builder;
   int64_t t0, t1;

   if (0)
      goto fail;

   variant = CALLOC_STRUCT(lp_setup_variant);
   if (variant == NULL)
      goto fail;

   if (LP_DEBUG & DEBUG_COUNTERS) {
      t0 = os_time_get();
   }

   memcpy(&variant->key, key, key->size);
   variant->list_item_global.base = variant;

   util_snprintf(func_name, sizeof(func_name), "fs%u_setup%u",
		 0,
		 variant->no);

   /* Currently always deal with full 4-wide vertex attributes from
    * the vertices.
    */

   vec4f_type = LLVMVectorType(LLVMFloatType(), 4);

   arg_types[0] = LLVMPointerType(vec4f_type, 0);        /* v0 */
   arg_types[1] = LLVMPointerType(vec4f_type, 0);        /* v1 */
   arg_types[2] = LLVMPointerType(vec4f_type, 0);        /* v2 */
   arg_types[3] = LLVMInt32Type();			/* facing */
   arg_types[4] = LLVMPointerType(vec4f_type, 0);	/* a0, aligned */
   arg_types[5] = LLVMPointerType(vec4f_type, 0);	/* dadx, aligned */
   arg_types[6] = LLVMPointerType(vec4f_type, 0);	/* dady, aligned */
   arg_types[7] = LLVMPointerType(vec4f_type, 0);       /* key, unused */

   func_type = LLVMFunctionType(LLVMVoidType(), arg_types, Elements(arg_types), 0);

   variant->function = LLVMAddFunction(screen->module, func_name, func_type);
   if (!variant->function)
      goto fail;

   LLVMSetFunctionCallConv(variant->function, LLVMCCallConv);

   args.v0       = LLVMGetParam(variant->function, 0);
   args.v1       = LLVMGetParam(variant->function, 1);
   args.v2       = LLVMGetParam(variant->function, 2);
   args.facing   = LLVMGetParam(variant->function, 3);
   args.a0       = LLVMGetParam(variant->function, 4);
   args.dadx     = LLVMGetParam(variant->function, 5);
   args.dady     = LLVMGetParam(variant->function, 6);

   lp_build_name(args.v0, "in_v0");
   lp_build_name(args.v1, "in_v1");
   lp_build_name(args.v2, "in_v2");
   lp_build_name(args.facing, "in_facing");
   lp_build_name(args.a0, "out_a0");
   lp_build_name(args.dadx, "out_dadx");
   lp_build_name(args.dady, "out_dady");

   /*
    * Function body
    */
   block = LLVMAppendBasicBlock(variant->function, "entry");
   builder = LLVMCreateBuilder();
   LLVMPositionBuilderAtEnd(builder, block);

   set_noalias(builder, variant->function, arg_types, Elements(arg_types));
   init_args(builder, &args, variant);
   emit_tri_coef(builder, &variant->key, &args);

   lp_emit_emms(builder);
   LLVMBuildRetVoid(builder);
   LLVMDisposeBuilder(builder);

   variant->jit_function = finalize_function(screen, builder,
					     variant->function);
   if (!variant->jit_function)
      goto fail;

   /*
    * Update timing information:
    */
   if (LP_DEBUG & DEBUG_COUNTERS) {
      t1 = os_time_get();
      LP_COUNT_ADD(llvm_compile_time, t1 - t0);
      LP_COUNT_ADD(nr_llvm_compiles, 1);
   }
   
   return variant;

fail:
   if (variant) {
      if (variant->function) {
	 if (variant->jit_function)
	    LLVMFreeMachineCodeForFunction(screen->engine,
					   variant->function);
	 LLVMDeleteFunction(variant->function);
      }
      FREE(variant);
   }
   
   return NULL;
}



static void
lp_make_setup_variant_key(struct llvmpipe_context *lp,
			  struct lp_setup_variant_key *key)
{
   struct lp_fragment_shader *fs = lp->fs;
   unsigned i;

   assert(sizeof key->inputs[0] == sizeof(ushort));
   
   key->num_inputs = fs->info.num_inputs;
   key->flatshade_first = lp->rasterizer->flatshade_first;
   key->pixel_center_half = lp->rasterizer->gl_rasterization_rules;
   key->size = Offset(struct lp_setup_variant_key,
		      inputs[key->num_inputs]);
   key->pad = 0;

   memcpy(key->inputs, fs->inputs, key->num_inputs * sizeof key->inputs[0]);
   for (i = 0; i < key->num_inputs; i++) {
      if (key->inputs[i].interp == LP_INTERP_COLOR) {
	 if (lp->rasterizer->flatshade)
	    key->inputs[i].interp = LP_INTERP_CONSTANT;
	 else
	    key->inputs[i].interp = LP_INTERP_LINEAR;
      }
   }

}


static void
remove_setup_variant(struct llvmpipe_context *lp,
		     struct lp_setup_variant *variant)
{
   struct llvmpipe_screen *screen = llvmpipe_screen(lp->pipe.screen);

   if (gallivm_debug & GALLIVM_DEBUG_IR) {
      debug_printf("llvmpipe: del setup_variant #%u total %u\n",
		   variant->no, lp->nr_setup_variants);
   }

   if (variant->function) {
      if (variant->jit_function)
	 LLVMFreeMachineCodeForFunction(screen->engine,
					variant->function);
      LLVMDeleteFunction(variant->function);
   }

   remove_from_list(&variant->list_item_global);
   lp->nr_setup_variants--;
   FREE(variant);
}



/* When the number of setup variants exceeds a threshold, cull a
 * fraction (currently a quarter) of them.
 */
static void
cull_setup_variants(struct llvmpipe_context *lp)
{
   struct pipe_context *pipe = &lp->pipe;
   int i;

   /*
    * XXX: we need to flush the context until we have some sort of reference
    * counting in fragment shaders as they may still be binned
    * Flushing alone might not be sufficient we need to wait on it too.
    */
   llvmpipe_finish(pipe, __FUNCTION__);

   for (i = 0; i < LP_MAX_SETUP_VARIANTS / 4; i++) {
      struct lp_setup_variant_list_item *item = last_elem(&lp->setup_variants_list);
      remove_setup_variant(lp, item->base);
   }
}


/**
 * Update fragment/vertex shader linkage state.  This is called just
 * prior to drawing something when some fragment-related state has
 * changed.
 */
void 
llvmpipe_update_setup(struct llvmpipe_context *lp)
{
   struct llvmpipe_screen *screen = llvmpipe_screen(lp->pipe.screen);

   struct lp_setup_variant_key *key = &lp->setup_variant.key;
   struct lp_setup_variant *variant = NULL;
   struct lp_setup_variant_list_item *li;

   lp_make_setup_variant_key(lp, key);

   foreach(li, &lp->setup_variants_list) {
      if(li->base->key.size == key->size &&
	 memcmp(&li->base->key, key, key->size) == 0) {
         variant = li->base;
         break;
      }
   }

   if (variant) {
      move_to_head(&lp->setup_variants_list, &variant->list_item_global);
   }
   else {
      if (lp->nr_setup_variants >= LP_MAX_SETUP_VARIANTS) {
	 cull_setup_variants(lp);
      }

      variant = generate_setup_variant(screen, key);
      insert_at_head(&lp->setup_variants_list, &variant->list_item_global);
      lp->nr_setup_variants++;
   }

   lp_setup_set_setup_variant(lp->setup,
			      variant);
}

void
lp_delete_setup_variants(struct llvmpipe_context *lp)
{
   struct lp_setup_variant_list_item *li;
   li = first_elem(&lp->setup_variants_list);
   while(!at_end(&lp->setup_variants_list, li)) {
      struct lp_setup_variant_list_item *next = next_elem(li);
      remove_setup_variant(lp, li->base);
      li = next;
   }
}

void
lp_dump_setup_coef( const struct lp_setup_variant_key *key,
		    const float (*sa0)[4],
		    const float (*sdadx)[4],
		    const float (*sdady)[4])
{
   int i, slot;

   for (i = 0; i < NUM_CHANNELS; i++) {
      float a0   = sa0  [0][i];
      float dadx = sdadx[0][i];
      float dady = sdady[0][i];

      debug_printf("POS.%c: a0 = %f, dadx = %f, dady = %f\n",
		   "xyzw"[i],
		   a0, dadx, dady);
   }

   for (slot = 0; slot < key->num_inputs; slot++) {
      unsigned usage_mask = key->inputs[slot].usage_mask;
      for (i = 0; i < NUM_CHANNELS; i++) {
	 if (usage_mask & (1 << i)) {
	    float a0   = sa0  [1 + slot][i];
	    float dadx = sdadx[1 + slot][i];
	    float dady = sdady[1 + slot][i];

	    debug_printf("IN[%u].%c: a0 = %f, dadx = %f, dady = %f\n",
			 slot,
			 "xyzw"[i],
			 a0, dadx, dady);
	 }
      }
   }
}
