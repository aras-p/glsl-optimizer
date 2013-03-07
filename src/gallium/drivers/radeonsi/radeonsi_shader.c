
/*
 * Copyright 2012 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *	Tom Stellard <thomas.stellard@amd.com>
 *	Michel Dänzer <michel.daenzer@amd.com>
 *      Christian König <christian.koenig@amd.com>
 */

#include "gallivm/lp_bld_tgsi_action.h"
#include "gallivm/lp_bld_const.h"
#include "gallivm/lp_bld_gather.h"
#include "gallivm/lp_bld_intr.h"
#include "gallivm/lp_bld_logic.h"
#include "gallivm/lp_bld_tgsi.h"
#include "gallivm/lp_bld_arit.h"
#include "radeon_llvm.h"
#include "radeon_llvm_emit.h"
#include "tgsi/tgsi_info.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_scan.h"
#include "tgsi/tgsi_dump.h"

#include "radeonsi_pipe.h"
#include "radeonsi_shader.h"
#include "si_state.h"
#include "sid.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>

struct si_shader_context
{
	struct radeon_llvm_context radeon_bld;
	struct r600_context *rctx;
	struct tgsi_parse_context parse;
	struct tgsi_token * tokens;
	struct si_pipe_shader *shader;
	struct si_shader_key key;
	unsigned type; /* TGSI_PROCESSOR_* specifies the type of shader. */
	unsigned ninput_emitted;
/*	struct list_head inputs; */
/*	unsigned * input_mappings *//* From TGSI to SI hw */
/*	struct tgsi_shader_info info;*/
};

static struct si_shader_context * si_shader_context(
	struct lp_build_tgsi_context * bld_base)
{
	return (struct si_shader_context *)bld_base;
}


#define PERSPECTIVE_BASE 0
#define LINEAR_BASE 9

#define SAMPLE_OFFSET 0
#define CENTER_OFFSET 2
#define CENTROID_OFSET 4

#define USE_SGPR_MAX_SUFFIX_LEN 5
#define CONST_ADDR_SPACE 2
#define USER_SGPR_ADDR_SPACE 8

/**
 * Build an LLVM bytecode indexed load using LLVMBuildGEP + LLVMBuildLoad
 *
 * @param offset The offset parameter specifies the number of
 * elements to offset, not the number of bytes or dwords.  An element is the
 * the type pointed to by the base_ptr parameter (e.g. int is the element of
 * an int* pointer)
 *
 * When LLVM lowers the load instruction, it will convert the element offset
 * into a dword offset automatically.
 *
 */
static LLVMValueRef build_indexed_load(
	struct gallivm_state * gallivm,
	LLVMValueRef base_ptr,
	LLVMValueRef offset)
{
	LLVMValueRef computed_ptr = LLVMBuildGEP(
		gallivm->builder, base_ptr, &offset, 1, "");

	return LLVMBuildLoad(gallivm->builder, computed_ptr, "");
}

static void declare_input_vs(
	struct si_shader_context * si_shader_ctx,
	unsigned input_index,
	const struct tgsi_full_declaration *decl)
{
	LLVMValueRef t_list_ptr;
	LLVMValueRef t_offset;
	LLVMValueRef t_list;
	LLVMValueRef attribute_offset;
	LLVMValueRef buffer_index_reg;
	LLVMValueRef args[3];
	LLVMTypeRef vec4_type;
	LLVMValueRef input;
	struct lp_build_context * base = &si_shader_ctx->radeon_bld.soa.bld_base.base;
	//struct pipe_vertex_element *velem = &rctx->vertex_elements->elements[input_index];
	unsigned chan;

	/* Load the T list */
	t_list_ptr = LLVMGetParam(si_shader_ctx->radeon_bld.main_fn, SI_PARAM_VERTEX_BUFFER);

	t_offset = lp_build_const_int32(base->gallivm, input_index);

	t_list = build_indexed_load(base->gallivm, t_list_ptr, t_offset);

	/* Build the attribute offset */
	attribute_offset = lp_build_const_int32(base->gallivm, 0);

	/* Load the buffer index, which is always stored in VGPR0
	 * for Vertex Shaders */
	buffer_index_reg = LLVMGetParam(si_shader_ctx->radeon_bld.main_fn, SI_PARAM_VERTEX_INDEX);

	vec4_type = LLVMVectorType(base->elem_type, 4);
	args[0] = t_list;
	args[1] = attribute_offset;
	args[2] = buffer_index_reg;
	input = lp_build_intrinsic(base->gallivm->builder,
		"llvm.SI.vs.load.input", vec4_type, args, 3);

	/* Break up the vec4 into individual components */
	for (chan = 0; chan < 4; chan++) {
		LLVMValueRef llvm_chan = lp_build_const_int32(base->gallivm, chan);
		/* XXX: Use a helper function for this.  There is one in
 		 * tgsi_llvm.c. */
		si_shader_ctx->radeon_bld.inputs[radeon_llvm_reg_index_soa(input_index, chan)] =
				LLVMBuildExtractElement(base->gallivm->builder,
				input, llvm_chan, "");
	}
}

static void declare_input_fs(
	struct si_shader_context * si_shader_ctx,
	unsigned input_index,
	const struct tgsi_full_declaration *decl)
{
	struct si_shader *shader = &si_shader_ctx->shader->shader;
	struct lp_build_context * base =
				&si_shader_ctx->radeon_bld.soa.bld_base.base;
	struct gallivm_state * gallivm = base->gallivm;
	LLVMTypeRef input_type = LLVMFloatTypeInContext(gallivm->context);
	LLVMValueRef main_fn = si_shader_ctx->radeon_bld.main_fn;

	LLVMValueRef interp_param;
	const char * intr_name;

	/* This value is:
	 * [15:0] NewPrimMask (Bit mask for each quad.  It is set it the
	 *                     quad begins a new primitive.  Bit 0 always needs
	 *                     to be unset)
	 * [32:16] ParamOffset
	 *
	 */
	LLVMValueRef params = LLVMGetParam(si_shader_ctx->radeon_bld.main_fn, SI_PARAM_PRIM_MASK);
	LLVMValueRef attr_number;

	unsigned chan;

	if (decl->Semantic.Name == TGSI_SEMANTIC_POSITION) {
		for (chan = 0; chan < TGSI_NUM_CHANNELS; chan++) {
			unsigned soa_index =
				radeon_llvm_reg_index_soa(input_index, chan);
			si_shader_ctx->radeon_bld.inputs[soa_index] =
				LLVMGetParam(main_fn, SI_PARAM_POS_X_FLOAT + chan);

			if (chan == 3)
				/* RCP for fragcoord.w */
				si_shader_ctx->radeon_bld.inputs[soa_index] =
					LLVMBuildFDiv(gallivm->builder,
						      lp_build_const_float(gallivm, 1.0f),
						      si_shader_ctx->radeon_bld.inputs[soa_index],
						      "");
		}
		return;
	}

	if (decl->Semantic.Name == TGSI_SEMANTIC_FACE) {
		LLVMValueRef face, is_face_positive;

		face = LLVMGetParam(main_fn, SI_PARAM_FRONT_FACE);

		is_face_positive = LLVMBuildFCmp(gallivm->builder,
						 LLVMRealUGT, face,
						 lp_build_const_float(gallivm, 0.0f),
						 "");

		si_shader_ctx->radeon_bld.inputs[radeon_llvm_reg_index_soa(input_index, 0)] =
			LLVMBuildSelect(gallivm->builder,
					is_face_positive,
					lp_build_const_float(gallivm, 1.0f),
					lp_build_const_float(gallivm, 0.0f),
					"");
		si_shader_ctx->radeon_bld.inputs[radeon_llvm_reg_index_soa(input_index, 1)] =
		si_shader_ctx->radeon_bld.inputs[radeon_llvm_reg_index_soa(input_index, 2)] =
			lp_build_const_float(gallivm, 0.0f);
		si_shader_ctx->radeon_bld.inputs[radeon_llvm_reg_index_soa(input_index, 3)] =
			lp_build_const_float(gallivm, 1.0f);

		return;
	}

	shader->input[input_index].param_offset = shader->ninterp++;
	attr_number = lp_build_const_int32(gallivm,
					   shader->input[input_index].param_offset);

	/* XXX: Handle all possible interpolation modes */
	switch (decl->Interp.Interpolate) {
	case TGSI_INTERPOLATE_COLOR:
		if (si_shader_ctx->key.flatshade) {
			interp_param = 0;
		} else {
			if (decl->Interp.Centroid)
				interp_param = LLVMGetParam(main_fn, SI_PARAM_PERSP_CENTROID);
			else
				interp_param = LLVMGetParam(main_fn, SI_PARAM_PERSP_CENTER);
		}
		break;
	case TGSI_INTERPOLATE_CONSTANT:
		interp_param = 0;
		break;
	case TGSI_INTERPOLATE_LINEAR:
		if (decl->Interp.Centroid)
			interp_param = LLVMGetParam(main_fn, SI_PARAM_LINEAR_CENTROID);
		else
			interp_param = LLVMGetParam(main_fn, SI_PARAM_LINEAR_CENTER);
		break;
	case TGSI_INTERPOLATE_PERSPECTIVE:
		if (decl->Interp.Centroid)
			interp_param = LLVMGetParam(main_fn, SI_PARAM_PERSP_CENTROID);
		else
			interp_param = LLVMGetParam(main_fn, SI_PARAM_PERSP_CENTER);
		break;
	default:
		fprintf(stderr, "Warning: Unhandled interpolation mode.\n");
		return;
	}

	if (!si_shader_ctx->ninput_emitted++) {
		/* Enable whole quad mode */
		lp_build_intrinsic(gallivm->builder,
				   "llvm.SI.wqm",
				   LLVMVoidTypeInContext(gallivm->context),
				   NULL, 0);
	}

	intr_name = interp_param ? "llvm.SI.fs.interp" : "llvm.SI.fs.constant";

	/* XXX: Could there be more than TGSI_NUM_CHANNELS (4) ? */
	if (decl->Semantic.Name == TGSI_SEMANTIC_COLOR &&
	    si_shader_ctx->key.color_two_side) {
		LLVMValueRef args[4];
		LLVMValueRef face, is_face_positive;
		LLVMValueRef back_attr_number =
			lp_build_const_int32(gallivm,
					     shader->input[input_index].param_offset + 1);

		face = LLVMGetParam(main_fn, SI_PARAM_FRONT_FACE);

		is_face_positive = LLVMBuildFCmp(gallivm->builder,
						 LLVMRealUGT, face,
						 lp_build_const_float(gallivm, 0.0f),
						 "");

		args[2] = params;
		args[3] = interp_param;
		for (chan = 0; chan < TGSI_NUM_CHANNELS; chan++) {
			LLVMValueRef llvm_chan = lp_build_const_int32(gallivm, chan);
			unsigned soa_index = radeon_llvm_reg_index_soa(input_index, chan);
			LLVMValueRef front, back;

			args[0] = llvm_chan;
			args[1] = attr_number;
			front = build_intrinsic(base->gallivm->builder, intr_name,
						input_type, args, args[3] ? 4 : 3,
						LLVMReadOnlyAttribute | LLVMNoUnwindAttribute);

			args[1] = back_attr_number;
			back = build_intrinsic(base->gallivm->builder, intr_name,
					       input_type, args, args[3] ? 4 : 3,
					       LLVMReadOnlyAttribute | LLVMNoUnwindAttribute);

			si_shader_ctx->radeon_bld.inputs[soa_index] =
				LLVMBuildSelect(gallivm->builder,
						is_face_positive,
						front,
						back,
						"");
		}

		shader->ninterp++;
	} else {
		for (chan = 0; chan < TGSI_NUM_CHANNELS; chan++) {
			LLVMValueRef args[4];
			LLVMValueRef llvm_chan = lp_build_const_int32(gallivm, chan);
			unsigned soa_index = radeon_llvm_reg_index_soa(input_index, chan);
			args[0] = llvm_chan;
			args[1] = attr_number;
			args[2] = params;
			args[3] = interp_param;
			si_shader_ctx->radeon_bld.inputs[soa_index] =
				build_intrinsic(base->gallivm->builder, intr_name,
						input_type, args, args[3] ? 4 : 3,
						LLVMReadOnlyAttribute | LLVMNoUnwindAttribute);
		}
	}
}

static void declare_input(
	struct radeon_llvm_context * radeon_bld,
	unsigned input_index,
	const struct tgsi_full_declaration *decl)
{
	struct si_shader_context * si_shader_ctx =
				si_shader_context(&radeon_bld->soa.bld_base);
	if (si_shader_ctx->type == TGSI_PROCESSOR_VERTEX) {
		declare_input_vs(si_shader_ctx, input_index, decl);
	} else if (si_shader_ctx->type == TGSI_PROCESSOR_FRAGMENT) {
		declare_input_fs(si_shader_ctx, input_index, decl);
	} else {
		fprintf(stderr, "Warning: Unsupported shader type,\n");
	}
}

static LLVMValueRef fetch_constant(
	struct lp_build_tgsi_context * bld_base,
	const struct tgsi_full_src_register *reg,
	enum tgsi_opcode_type type,
	unsigned swizzle)
{
	struct si_shader_context *si_shader_ctx = si_shader_context(bld_base);
	struct lp_build_context * base = &bld_base->base;

	LLVMValueRef ptr;
	LLVMValueRef args[2];
	LLVMValueRef result;

	if (swizzle == LP_CHAN_ALL) {
		unsigned chan;
		LLVMValueRef values[4];
		for (chan = 0; chan < TGSI_NUM_CHANNELS; ++chan)
			values[chan] = fetch_constant(bld_base, reg, type, chan);

		return lp_build_gather_values(bld_base->base.gallivm, values, 4);
	}

	/* Load the resource descriptor */
	ptr = LLVMGetParam(si_shader_ctx->radeon_bld.main_fn, SI_PARAM_CONST);
	args[0] = build_indexed_load(base->gallivm, ptr, bld_base->uint_bld.zero);

	args[1] = lp_build_const_int32(base->gallivm, (reg->Register.Index * 4 + swizzle) * 4);
	if (reg->Register.Indirect) {
		const struct tgsi_ind_register *ireg = &reg->Indirect;
		LLVMValueRef addr = si_shader_ctx->radeon_bld.soa.addr[ireg->Index][ireg->Swizzle];
		LLVMValueRef idx = LLVMBuildLoad(base->gallivm->builder, addr, "load addr reg");
		idx = lp_build_mul_imm(&bld_base->uint_bld, idx, 16);
		args[1] = lp_build_add(&bld_base->uint_bld, idx, args[1]);
	}

	result = build_intrinsic(base->gallivm->builder, "llvm.SI.load.const", base->elem_type,
                                 args, 2, LLVMReadOnlyAttribute | LLVMNoUnwindAttribute);

	return bitcast(bld_base, type, result);
}

/* Initialize arguments for the shader export intrinsic */
static void si_llvm_init_export_args(struct lp_build_tgsi_context *bld_base,
				     struct tgsi_full_declaration *d,
				     unsigned index,
				     unsigned target,
				     LLVMValueRef *args)
{
	struct si_shader_context *si_shader_ctx = si_shader_context(bld_base);
	struct lp_build_context *uint =
				&si_shader_ctx->radeon_bld.soa.bld_base.uint_bld;
	struct lp_build_context *base = &bld_base->base;
	unsigned compressed = 0;
	unsigned chan;

	if (si_shader_ctx->type == TGSI_PROCESSOR_FRAGMENT) {
		int cbuf = target - V_008DFC_SQ_EXP_MRT;

		if (cbuf >= 0 && cbuf < 8) {
			compressed = (si_shader_ctx->key.export_16bpc >> cbuf) & 0x1;

			if (compressed)
				si_shader_ctx->shader->spi_shader_col_format |=
					V_028714_SPI_SHADER_FP16_ABGR << (4 * cbuf);
			else
				si_shader_ctx->shader->spi_shader_col_format |=
					V_028714_SPI_SHADER_32_ABGR << (4 * cbuf);
		}
	}

	if (compressed) {
		/* Pixel shader needs to pack output values before export */
		for (chan = 0; chan < 2; chan++ ) {
			LLVMValueRef *out_ptr =
				si_shader_ctx->radeon_bld.soa.outputs[index];
			args[0] = LLVMBuildLoad(base->gallivm->builder,
						out_ptr[2 * chan], "");
			args[1] = LLVMBuildLoad(base->gallivm->builder,
						out_ptr[2 * chan + 1], "");
			args[chan + 5] =
				build_intrinsic(base->gallivm->builder,
						"llvm.SI.packf16",
						LLVMInt32TypeInContext(base->gallivm->context),
						args, 2,
						LLVMReadNoneAttribute | LLVMNoUnwindAttribute);
			args[chan + 7] = args[chan + 5] =
				LLVMBuildBitCast(base->gallivm->builder,
						 args[chan + 5],
						 LLVMFloatTypeInContext(base->gallivm->context),
						 "");
		}

		/* Set COMPR flag */
		args[4] = uint->one;
	} else {
		for (chan = 0; chan < 4; chan++ ) {
			LLVMValueRef out_ptr =
				si_shader_ctx->radeon_bld.soa.outputs[index][chan];
			/* +5 because the first output value will be
			 * the 6th argument to the intrinsic. */
			args[chan + 5] = LLVMBuildLoad(base->gallivm->builder,
						       out_ptr, "");
		}

		/* Clear COMPR flag */
		args[4] = uint->zero;
	}

	/* XXX: This controls which components of the output
	 * registers actually get exported. (e.g bit 0 means export
	 * X component, bit 1 means export Y component, etc.)  I'm
	 * hard coding this to 0xf for now.  In the future, we might
	 * want to do something else. */
	args[0] = lp_build_const_int32(base->gallivm, 0xf);

	/* Specify whether the EXEC mask represents the valid mask */
	args[1] = uint->zero;

	/* Specify whether this is the last export */
	args[2] = uint->zero;

	/* Specify the target we are exporting */
	args[3] = lp_build_const_int32(base->gallivm, target);

	/* XXX: We probably need to keep track of the output
	 * values, so we know what we are passing to the next
	 * stage. */
}

static void si_alpha_test(struct lp_build_tgsi_context *bld_base,
			  unsigned index)
{
	struct si_shader_context *si_shader_ctx = si_shader_context(bld_base);
	struct gallivm_state *gallivm = bld_base->base.gallivm;

	if (si_shader_ctx->key.alpha_func != PIPE_FUNC_NEVER) {
		LLVMValueRef out_ptr = si_shader_ctx->radeon_bld.soa.outputs[index][3];
		LLVMValueRef alpha_pass =
			lp_build_cmp(&bld_base->base,
				     si_shader_ctx->key.alpha_func,
				     LLVMBuildLoad(gallivm->builder, out_ptr, ""),
				     lp_build_const_float(gallivm, si_shader_ctx->key.alpha_ref));
		LLVMValueRef arg =
			lp_build_select(&bld_base->base,
					alpha_pass,
					lp_build_const_float(gallivm, 1.0f),
					lp_build_const_float(gallivm, -1.0f));

		build_intrinsic(gallivm->builder,
				"llvm.AMDGPU.kill",
				LLVMVoidTypeInContext(gallivm->context),
				&arg, 1, 0);
	} else {
		build_intrinsic(gallivm->builder,
				"llvm.AMDGPU.kilp",
				LLVMVoidTypeInContext(gallivm->context),
				NULL, 0, 0);
	}
}

/* XXX: This is partially implemented for VS only at this point.  It is not complete */
static void si_llvm_emit_epilogue(struct lp_build_tgsi_context * bld_base)
{
	struct si_shader_context * si_shader_ctx = si_shader_context(bld_base);
	struct si_shader * shader = &si_shader_ctx->shader->shader;
	struct lp_build_context * base = &bld_base->base;
	struct lp_build_context * uint =
				&si_shader_ctx->radeon_bld.soa.bld_base.uint_bld;
	struct tgsi_parse_context *parse = &si_shader_ctx->parse;
	LLVMValueRef args[9];
	LLVMValueRef last_args[9] = { 0 };
	unsigned color_count = 0;
	unsigned param_count = 0;
	int depth_index = -1, stencil_index = -1;

	while (!tgsi_parse_end_of_tokens(parse)) {
		struct tgsi_full_declaration *d =
					&parse->FullToken.FullDeclaration;
		unsigned target;
		unsigned index;
		int i;

		tgsi_parse_token(parse);

		if (parse->FullToken.Token.Type == TGSI_TOKEN_TYPE_PROPERTY &&
		    parse->FullToken.FullProperty.Property.PropertyName ==
		    TGSI_PROPERTY_FS_COLOR0_WRITES_ALL_CBUFS)
			shader->fs_write_all = TRUE;

		if (parse->FullToken.Token.Type != TGSI_TOKEN_TYPE_DECLARATION)
			continue;

		switch (d->Declaration.File) {
		case TGSI_FILE_INPUT:
			i = shader->ninput++;
			shader->input[i].name = d->Semantic.Name;
			shader->input[i].sid = d->Semantic.Index;
			shader->input[i].interpolate = d->Interp.Interpolate;
			shader->input[i].centroid = d->Interp.Centroid;
			continue;

		case TGSI_FILE_OUTPUT:
			i = shader->noutput++;
			shader->output[i].name = d->Semantic.Name;
			shader->output[i].sid = d->Semantic.Index;
			shader->output[i].interpolate = d->Interp.Interpolate;
			break;

		default:
			continue;
		}

		for (index = d->Range.First; index <= d->Range.Last; index++) {
			/* Select the correct target */
			switch(d->Semantic.Name) {
			case TGSI_SEMANTIC_PSIZE:
				target = V_008DFC_SQ_EXP_POS;
				break;
			case TGSI_SEMANTIC_POSITION:
				if (si_shader_ctx->type == TGSI_PROCESSOR_VERTEX) {
					target = V_008DFC_SQ_EXP_POS;
					break;
				} else {
					depth_index = index;
					continue;
				}
			case TGSI_SEMANTIC_STENCIL:
				stencil_index = index;
				continue;
			case TGSI_SEMANTIC_COLOR:
				if (si_shader_ctx->type == TGSI_PROCESSOR_VERTEX) {
			case TGSI_SEMANTIC_BCOLOR:
					target = V_008DFC_SQ_EXP_PARAM + param_count;
					shader->output[i].param_offset = param_count;
					param_count++;
				} else {
					target = V_008DFC_SQ_EXP_MRT + color_count;
					if (color_count == 0 &&
					    si_shader_ctx->key.alpha_func != PIPE_FUNC_ALWAYS)
						si_alpha_test(bld_base, index);

					color_count++;
				}
				break;
			case TGSI_SEMANTIC_FOG:
			case TGSI_SEMANTIC_GENERIC:
				target = V_008DFC_SQ_EXP_PARAM + param_count;
				shader->output[i].param_offset = param_count;
				param_count++;
				break;
			default:
				target = 0;
				fprintf(stderr,
					"Warning: SI unhandled output type:%d\n",
					d->Semantic.Name);
			}

			si_llvm_init_export_args(bld_base, d, index, target, args);

			if (si_shader_ctx->type == TGSI_PROCESSOR_VERTEX ?
			    (d->Semantic.Name == TGSI_SEMANTIC_POSITION) :
			    (d->Semantic.Name == TGSI_SEMANTIC_COLOR)) {
				if (last_args[0]) {
					lp_build_intrinsic(base->gallivm->builder,
							   "llvm.SI.export",
							   LLVMVoidTypeInContext(base->gallivm->context),
							   last_args, 9);
				}

				memcpy(last_args, args, sizeof(args));
			} else {
				lp_build_intrinsic(base->gallivm->builder,
						   "llvm.SI.export",
						   LLVMVoidTypeInContext(base->gallivm->context),
						   args, 9);
			}

		}
	}

	if (depth_index >= 0 || stencil_index >= 0) {
		LLVMValueRef out_ptr;
		unsigned mask = 0;

		/* Specify the target we are exporting */
		args[3] = lp_build_const_int32(base->gallivm, V_008DFC_SQ_EXP_MRTZ);

		if (depth_index >= 0) {
			out_ptr = si_shader_ctx->radeon_bld.soa.outputs[depth_index][2];
			args[5] = LLVMBuildLoad(base->gallivm->builder, out_ptr, "");
			mask |= 0x1;

			if (stencil_index < 0) {
				args[6] =
				args[7] =
				args[8] = args[5];
			}
		}

		if (stencil_index >= 0) {
			out_ptr = si_shader_ctx->radeon_bld.soa.outputs[stencil_index][1];
			args[7] =
			args[8] =
			args[6] = LLVMBuildLoad(base->gallivm->builder, out_ptr, "");
			mask |= 0x2;

			if (depth_index < 0)
				args[5] = args[6];
		}

		/* Specify which components to enable */
		args[0] = lp_build_const_int32(base->gallivm, mask);

		args[1] =
		args[2] =
		args[4] = uint->zero;

		if (last_args[0])
			lp_build_intrinsic(base->gallivm->builder,
					   "llvm.SI.export",
					   LLVMVoidTypeInContext(base->gallivm->context),
					   args, 9);
		else
			memcpy(last_args, args, sizeof(args));
	}

	if (!last_args[0]) {
		assert(si_shader_ctx->type == TGSI_PROCESSOR_FRAGMENT);

		/* Specify which components to enable */
		last_args[0] = lp_build_const_int32(base->gallivm, 0x0);

		/* Specify the target we are exporting */
		last_args[3] = lp_build_const_int32(base->gallivm, V_008DFC_SQ_EXP_MRT);

		/* Set COMPR flag to zero to export data as 32-bit */
		last_args[4] = uint->zero;

		/* dummy bits */
		last_args[5]= uint->zero;
		last_args[6]= uint->zero;
		last_args[7]= uint->zero;
		last_args[8]= uint->zero;

		si_shader_ctx->shader->spi_shader_col_format |=
			V_028714_SPI_SHADER_32_ABGR;
	}

	/* Specify whether the EXEC mask represents the valid mask */
	last_args[1] = lp_build_const_int32(base->gallivm,
					    si_shader_ctx->type == TGSI_PROCESSOR_FRAGMENT);

	if (shader->fs_write_all && shader->nr_cbufs > 1) {
		int i;

		/* Specify that this is not yet the last export */
		last_args[2] = lp_build_const_int32(base->gallivm, 0);

		for (i = 1; i < shader->nr_cbufs; i++) {
			/* Specify the target we are exporting */
			last_args[3] = lp_build_const_int32(base->gallivm,
							    V_008DFC_SQ_EXP_MRT + i);

			lp_build_intrinsic(base->gallivm->builder,
					   "llvm.SI.export",
					   LLVMVoidTypeInContext(base->gallivm->context),
					   last_args, 9);

			si_shader_ctx->shader->spi_shader_col_format |=
				si_shader_ctx->shader->spi_shader_col_format << 4;
		}

		last_args[3] = lp_build_const_int32(base->gallivm, V_008DFC_SQ_EXP_MRT);
	}

	/* Specify that this is the last export */
	last_args[2] = lp_build_const_int32(base->gallivm, 1);

	lp_build_intrinsic(base->gallivm->builder,
			   "llvm.SI.export",
			   LLVMVoidTypeInContext(base->gallivm->context),
			   last_args, 9);

/* XXX: Look up what this function does */
/*		ctx->shader->output[i].spi_sid = r600_spi_sid(&ctx->shader->output[i]);*/
}

static void tex_fetch_args(
	struct lp_build_tgsi_context * bld_base,
	struct lp_build_emit_data * emit_data)
{
	struct si_shader_context *si_shader_ctx = si_shader_context(bld_base);
	struct gallivm_state *gallivm = bld_base->base.gallivm;
	const struct tgsi_full_instruction * inst = emit_data->inst;
	unsigned opcode = inst->Instruction.Opcode;
	unsigned target = inst->Texture.Texture;
	LLVMValueRef ptr;
	LLVMValueRef offset;
	LLVMValueRef coords[4];
	LLVMValueRef address[16];
	unsigned count = 0;
	unsigned chan;

	/* WriteMask */
	/* XXX: should be optimized using emit_data->inst->Dst[0].Register.WriteMask*/
	emit_data->args[0] = lp_build_const_int32(bld_base->base.gallivm, 0xf);

	/* Fetch and project texture coordinates */
	coords[3] = lp_build_emit_fetch(bld_base, emit_data->inst, 0, TGSI_CHAN_W);
	for (chan = 0; chan < 3; chan++ ) {
		coords[chan] = lp_build_emit_fetch(bld_base,
						   emit_data->inst, 0,
						   chan);
		if (opcode == TGSI_OPCODE_TXP)
			coords[chan] = lp_build_emit_llvm_binary(bld_base,
								 TGSI_OPCODE_DIV,
								 coords[chan],
								 coords[3]);
	}

	if (opcode == TGSI_OPCODE_TXP)
		coords[3] = bld_base->base.one;

	/* Pack LOD bias value */
	if (opcode == TGSI_OPCODE_TXB)
		address[count++] = coords[3];

	if ((target == TGSI_TEXTURE_CUBE || target == TGSI_TEXTURE_SHADOWCUBE) &&
	    opcode != TGSI_OPCODE_TXQ)
		radeon_llvm_emit_prepare_cube_coords(bld_base, emit_data, coords);

	/* Pack depth comparison value */
	switch (target) {
	case TGSI_TEXTURE_SHADOW1D:
	case TGSI_TEXTURE_SHADOW1D_ARRAY:
	case TGSI_TEXTURE_SHADOW2D:
	case TGSI_TEXTURE_SHADOWRECT:
		address[count++] = coords[2];
		break;
	case TGSI_TEXTURE_SHADOWCUBE:
	case TGSI_TEXTURE_SHADOW2D_ARRAY:
		address[count++] = coords[3];
		break;
	case TGSI_TEXTURE_SHADOWCUBE_ARRAY:
		address[count++] = lp_build_emit_fetch(bld_base, inst, 1, 0);
	}

	/* Pack texture coordinates */
	address[count++] = coords[0];
	switch (target) {
	case TGSI_TEXTURE_2D:
	case TGSI_TEXTURE_2D_ARRAY:
	case TGSI_TEXTURE_3D:
	case TGSI_TEXTURE_CUBE:
	case TGSI_TEXTURE_RECT:
	case TGSI_TEXTURE_SHADOW2D:
	case TGSI_TEXTURE_SHADOWRECT:
	case TGSI_TEXTURE_SHADOW2D_ARRAY:
	case TGSI_TEXTURE_SHADOWCUBE:
	case TGSI_TEXTURE_2D_MSAA:
	case TGSI_TEXTURE_2D_ARRAY_MSAA:
	case TGSI_TEXTURE_CUBE_ARRAY:
	case TGSI_TEXTURE_SHADOWCUBE_ARRAY:
		address[count++] = coords[1];
	}
	switch (target) {
	case TGSI_TEXTURE_3D:
	case TGSI_TEXTURE_CUBE:
	case TGSI_TEXTURE_SHADOWCUBE:
	case TGSI_TEXTURE_CUBE_ARRAY:
	case TGSI_TEXTURE_SHADOWCUBE_ARRAY:
		address[count++] = coords[2];
	}

	/* Pack array slice */
	switch (target) {
	case TGSI_TEXTURE_1D_ARRAY:
		address[count++] = coords[1];
	}
	switch (target) {
	case TGSI_TEXTURE_2D_ARRAY:
	case TGSI_TEXTURE_2D_ARRAY_MSAA:
	case TGSI_TEXTURE_SHADOW2D_ARRAY:
		address[count++] = coords[2];
	}
	switch (target) {
	case TGSI_TEXTURE_CUBE_ARRAY:
	case TGSI_TEXTURE_SHADOW1D_ARRAY:
	case TGSI_TEXTURE_SHADOWCUBE_ARRAY:
		address[count++] = coords[3];
	}

	/* Pack LOD */
	if (opcode == TGSI_OPCODE_TXL)
		address[count++] = coords[3];

	if (count > 16) {
		assert(!"Cannot handle more than 16 texture address parameters");
		count = 16;
	}

	for (chan = 0; chan < count; chan++ ) {
		address[chan] = LLVMBuildBitCast(gallivm->builder,
						 address[chan],
						 LLVMInt32TypeInContext(gallivm->context),
						 "");
	}

	/* Pad to power of two vector */
	while (count < util_next_power_of_two(count))
		address[count++] = LLVMGetUndef(LLVMInt32TypeInContext(gallivm->context));

	emit_data->args[1] = lp_build_gather_values(gallivm, address, count);

	/* Resource */
	ptr = LLVMGetParam(si_shader_ctx->radeon_bld.main_fn, SI_PARAM_RESOURCE);
	offset = lp_build_const_int32(bld_base->base.gallivm,
				  emit_data->inst->Src[1].Register.Index);
	emit_data->args[2] = build_indexed_load(bld_base->base.gallivm,
						ptr, offset);

	/* Sampler */
	ptr = LLVMGetParam(si_shader_ctx->radeon_bld.main_fn, SI_PARAM_SAMPLER);
	offset = lp_build_const_int32(bld_base->base.gallivm,
				  emit_data->inst->Src[1].Register.Index);
	emit_data->args[3] = build_indexed_load(bld_base->base.gallivm,
						ptr, offset);

	/* Dimensions */
	emit_data->args[4] = lp_build_const_int32(bld_base->base.gallivm, target);

	emit_data->arg_count = 5;
	/* XXX: To optimize, we could use a float or v2f32, if the last bits of
	 * the writemask are clear */
	emit_data->dst_type = LLVMVectorType(
			LLVMFloatTypeInContext(bld_base->base.gallivm->context),
			4);
}

static void build_tex_intrinsic(const struct lp_build_tgsi_action * action,
				struct lp_build_tgsi_context * bld_base,
				struct lp_build_emit_data * emit_data)
{
	struct lp_build_context * base = &bld_base->base;
	char intr_name[23];

	sprintf(intr_name, "%sv%ui32", action->intr_name,
		LLVMGetVectorSize(LLVMTypeOf(emit_data->args[1])));

	emit_data->output[emit_data->chan] = lp_build_intrinsic(
		base->gallivm->builder, intr_name, emit_data->dst_type,
		emit_data->args, emit_data->arg_count);
}

static const struct lp_build_tgsi_action tex_action = {
	.fetch_args = tex_fetch_args,
	.emit = build_tex_intrinsic,
	.intr_name = "llvm.SI.sample."
};

static const struct lp_build_tgsi_action txb_action = {
	.fetch_args = tex_fetch_args,
	.emit = build_tex_intrinsic,
	.intr_name = "llvm.SI.sampleb."
};

static const struct lp_build_tgsi_action txl_action = {
	.fetch_args = tex_fetch_args,
	.emit = build_tex_intrinsic,
	.intr_name = "llvm.SI.samplel."
};

static void create_function(struct si_shader_context *si_shader_ctx)
{
	struct gallivm_state *gallivm = si_shader_ctx->radeon_bld.soa.bld_base.base.gallivm;
	LLVMTypeRef params[20], f32, i8, i32, v2i32, v3i32;
	unsigned i;

	i8 = LLVMInt8TypeInContext(gallivm->context);
	i32 = LLVMInt32TypeInContext(gallivm->context);
	f32 = LLVMFloatTypeInContext(gallivm->context);
	v2i32 = LLVMVectorType(i32, 2);
	v3i32 = LLVMVectorType(i32, 3);

	params[SI_PARAM_CONST] = LLVMPointerType(LLVMVectorType(i8, 16), CONST_ADDR_SPACE);
	params[SI_PARAM_SAMPLER] = params[SI_PARAM_CONST];
	params[SI_PARAM_RESOURCE] = LLVMPointerType(LLVMVectorType(i8, 32), CONST_ADDR_SPACE);

	if (si_shader_ctx->type == TGSI_PROCESSOR_VERTEX) {
		params[SI_PARAM_VERTEX_BUFFER] = params[SI_PARAM_SAMPLER];
		params[SI_PARAM_VERTEX_INDEX] = i32;
		radeon_llvm_create_func(&si_shader_ctx->radeon_bld, params, 5);

	} else {
		params[SI_PARAM_PRIM_MASK] = i32;
		params[SI_PARAM_PERSP_SAMPLE] = v2i32;
		params[SI_PARAM_PERSP_CENTER] = v2i32;
		params[SI_PARAM_PERSP_CENTROID] = v2i32;
		params[SI_PARAM_PERSP_PULL_MODEL] = v3i32;
		params[SI_PARAM_LINEAR_SAMPLE] = v2i32;
		params[SI_PARAM_LINEAR_CENTER] = v2i32;
		params[SI_PARAM_LINEAR_CENTROID] = v2i32;
		params[SI_PARAM_LINE_STIPPLE_TEX] = f32;
		params[SI_PARAM_POS_X_FLOAT] = f32;
		params[SI_PARAM_POS_Y_FLOAT] = f32;
		params[SI_PARAM_POS_Z_FLOAT] = f32;
		params[SI_PARAM_POS_W_FLOAT] = f32;
		params[SI_PARAM_FRONT_FACE] = f32;
		params[SI_PARAM_ANCILLARY] = f32;
		params[SI_PARAM_SAMPLE_COVERAGE] = f32;
		params[SI_PARAM_POS_FIXED_PT] = f32;
		radeon_llvm_create_func(&si_shader_ctx->radeon_bld, params, 20);
	}

	radeon_llvm_shader_type(si_shader_ctx->radeon_bld.main_fn, si_shader_ctx->type);
	for (i = SI_PARAM_CONST; i <= SI_PARAM_VERTEX_BUFFER; ++i) {
		LLVMValueRef P = LLVMGetParam(si_shader_ctx->radeon_bld.main_fn, i);
		LLVMAddAttribute(P, LLVMInRegAttribute);
	}
}

int si_pipe_shader_create(
	struct pipe_context *ctx,
	struct si_pipe_shader *shader,
	struct si_shader_key key)
{
	struct r600_context *rctx = (struct r600_context*)ctx;
	struct si_pipe_shader_selector *sel = shader->selector;
	struct si_shader_context si_shader_ctx;
	struct tgsi_shader_info shader_info;
	struct lp_build_tgsi_context * bld_base;
	LLVMModuleRef mod;
	unsigned char * inst_bytes;
	unsigned inst_byte_count;
	unsigned i;
	uint32_t *ptr;
	bool dump;

	dump = debug_get_bool_option("RADEON_DUMP_SHADERS", FALSE);

	assert(shader->shader.noutput == 0);
	assert(shader->shader.ninterp == 0);
	assert(shader->shader.ninput == 0);

	memset(&si_shader_ctx, 0, sizeof(si_shader_ctx));
	radeon_llvm_context_init(&si_shader_ctx.radeon_bld);
	bld_base = &si_shader_ctx.radeon_bld.soa.bld_base;

	tgsi_scan_shader(sel->tokens, &shader_info);
	shader->shader.uses_kill = shader_info.uses_kill;
	bld_base->info = &shader_info;
	bld_base->emit_fetch_funcs[TGSI_FILE_CONSTANT] = fetch_constant;
	bld_base->emit_epilogue = si_llvm_emit_epilogue;

	bld_base->op_actions[TGSI_OPCODE_TEX] = tex_action;
	bld_base->op_actions[TGSI_OPCODE_TXB] = txb_action;
	bld_base->op_actions[TGSI_OPCODE_TXL] = txl_action;
	bld_base->op_actions[TGSI_OPCODE_TXP] = tex_action;

	si_shader_ctx.radeon_bld.load_input = declare_input;
	si_shader_ctx.tokens = sel->tokens;
	tgsi_parse_init(&si_shader_ctx.parse, si_shader_ctx.tokens);
	si_shader_ctx.shader = shader;
	si_shader_ctx.key = key;
	si_shader_ctx.type = si_shader_ctx.parse.FullHeader.Processor.Processor;
	si_shader_ctx.rctx = rctx;

	create_function(&si_shader_ctx);

	shader->shader.nr_cbufs = rctx->framebuffer.nr_cbufs;

	/* Dump TGSI code before doing TGSI->LLVM conversion in case the
	 * conversion fails. */
	if (dump) {
		tgsi_dump(sel->tokens, 0);
	}

	if (!lp_build_tgsi_llvm(bld_base, sel->tokens)) {
		fprintf(stderr, "Failed to translate shader from TGSI to LLVM\n");
		return -EINVAL;
	}

	radeon_llvm_finalize_module(&si_shader_ctx.radeon_bld);

	mod = bld_base->base.gallivm->module;
	if (dump) {
		LLVMDumpModule(mod);
	}
	radeon_llvm_compile(mod, &inst_bytes, &inst_byte_count, "SI", dump);
	if (dump) {
		fprintf(stderr, "SI CODE:\n");
		for (i = 0; i < inst_byte_count; i+=4 ) {
			fprintf(stderr, "%02x%02x%02x%02x\n", inst_bytes[i + 3],
				inst_bytes[i + 2], inst_bytes[i + 1],
				inst_bytes[i]);
		}
	}

	shader->num_sgprs = util_le32_to_cpu(*(uint32_t*)inst_bytes);
	shader->num_vgprs = util_le32_to_cpu(*(uint32_t*)(inst_bytes + 4));
	shader->spi_ps_input_ena = util_le32_to_cpu(*(uint32_t*)(inst_bytes + 8));

	radeon_llvm_dispose(&si_shader_ctx.radeon_bld);
	tgsi_parse_free(&si_shader_ctx.parse);

	/* copy new shader */
	si_resource_reference(&shader->bo, NULL);
	shader->bo = si_resource_create_custom(ctx->screen, PIPE_USAGE_IMMUTABLE,
					       inst_byte_count - 12);
	if (shader->bo == NULL) {
		return -ENOMEM;
	}

	ptr = (uint32_t*)rctx->ws->buffer_map(shader->bo->cs_buf, rctx->cs, PIPE_TRANSFER_WRITE);
	if (0 /*R600_BIG_ENDIAN*/) {
		for (i = 0; i < (inst_byte_count-12)/4; ++i) {
			ptr[i] = util_bswap32(*(uint32_t*)(inst_bytes+12 + i*4));
		}
	} else {
		memcpy(ptr, inst_bytes + 12, inst_byte_count - 12);
	}
	rctx->ws->buffer_unmap(shader->bo->cs_buf);

	free(inst_bytes);

	return 0;
}

void si_pipe_shader_destroy(struct pipe_context *ctx, struct si_pipe_shader *shader)
{
	si_resource_reference(&shader->bo, NULL);
}
