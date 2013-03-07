/*
 * Copyright 2011 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors: Tom Stellard <thomas.stellard@amd.com>
 *
 */
#include "radeon_llvm.h"

#include "gallivm/lp_bld_const.h"
#include "gallivm/lp_bld_gather.h"
#include "gallivm/lp_bld_flow.h"
#include "gallivm/lp_bld_init.h"
#include "gallivm/lp_bld_intr.h"
#include "gallivm/lp_bld_swizzle.h"
#include "tgsi/tgsi_info.h"
#include "tgsi/tgsi_parse.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_debug.h"

#include <llvm-c/Core.h>
#include <llvm-c/Transforms/Scalar.h>

static struct radeon_llvm_loop * get_current_loop(struct radeon_llvm_context * ctx)
{
	return ctx->loop_depth > 0 ? ctx->loop + (ctx->loop_depth - 1) : NULL;
}

static struct radeon_llvm_branch * get_current_branch(
	struct radeon_llvm_context * ctx)
{
	return ctx->branch_depth > 0 ?
			ctx->branch + (ctx->branch_depth - 1) : NULL;
}

unsigned radeon_llvm_reg_index_soa(unsigned index, unsigned chan)
{
 return (index * 4) + chan;
}

static LLVMValueRef emit_swizzle(
	struct lp_build_tgsi_context * bld_base,
        LLVMValueRef value,
	unsigned swizzle_x,
	unsigned swizzle_y,
	unsigned swizzle_z,
	unsigned swizzle_w)
{
	LLVMValueRef swizzles[4];
	LLVMTypeRef i32t =
		LLVMInt32TypeInContext(bld_base->base.gallivm->context);

	swizzles[0] = LLVMConstInt(i32t, swizzle_x, 0);
	swizzles[1] = LLVMConstInt(i32t, swizzle_y, 0);
	swizzles[2] = LLVMConstInt(i32t, swizzle_z, 0);
	swizzles[3] = LLVMConstInt(i32t, swizzle_w, 0);

	return LLVMBuildShuffleVector(bld_base->base.gallivm->builder,
		value,
		LLVMGetUndef(LLVMTypeOf(value)),
		LLVMConstVector(swizzles, 4), "");
}

static struct tgsi_declaration_range
get_array_range(struct lp_build_tgsi_context *bld_base,
		unsigned File, const struct tgsi_ind_register *reg)
{
	struct radeon_llvm_context * ctx = radeon_llvm_context(bld_base);
	if (File != TGSI_FILE_TEMPORARY || reg->ArrayID == 0 ||
            reg->ArrayID > RADEON_LLVM_MAX_ARRAYS) {
		struct tgsi_declaration_range range;
		range.First = 0;
		range.Last = bld_base->info->file_max[File];
		return range;
	}

	return ctx->arrays[reg->ArrayID - 1];
}

static LLVMValueRef
emit_array_index(
	struct lp_build_tgsi_soa_context *bld,
	const struct tgsi_ind_register *reg,
	unsigned offset)
{
	struct gallivm_state * gallivm = bld->bld_base.base.gallivm;

	LLVMValueRef addr = LLVMBuildLoad(gallivm->builder, bld->addr[reg->Index][reg->Swizzle], "");
	return LLVMBuildAdd(gallivm->builder, addr, lp_build_const_int32(gallivm, offset), "");
}

static LLVMValueRef
emit_fetch(
	struct lp_build_tgsi_context *bld_base,
	const struct tgsi_full_src_register *reg,
	enum tgsi_opcode_type type,
	unsigned swizzle);

static LLVMValueRef
emit_array_fetch(
	struct lp_build_tgsi_context *bld_base,
	unsigned File, enum tgsi_opcode_type type,
	struct tgsi_declaration_range range,
	unsigned swizzle)
{
	struct lp_build_tgsi_soa_context *bld = lp_soa_context(bld_base);
	struct gallivm_state * gallivm = bld->bld_base.base.gallivm;
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;

	unsigned i, size = range.Last - range.First + 1;
	LLVMTypeRef vec = LLVMVectorType(tgsi2llvmtype(bld_base, type), size);
	LLVMValueRef result = LLVMGetUndef(vec);

	struct tgsi_full_src_register tmp_reg = {};
	tmp_reg.Register.File = File;

	for (i = 0; i < size; ++i) {
		tmp_reg.Register.Index = i + range.First;
		LLVMValueRef temp = emit_fetch(bld_base, &tmp_reg, type, swizzle);
		result = LLVMBuildInsertElement(builder, result, temp,
			lp_build_const_int32(gallivm, i), "");
	}
	return result;
}

static LLVMValueRef
emit_fetch(
	struct lp_build_tgsi_context *bld_base,
	const struct tgsi_full_src_register *reg,
	enum tgsi_opcode_type type,
	unsigned swizzle)
{
	struct radeon_llvm_context * ctx = radeon_llvm_context(bld_base);
	struct lp_build_tgsi_soa_context *bld = lp_soa_context(bld_base);
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	LLVMValueRef result, ptr;

	if (swizzle == ~0) {
		LLVMValueRef values[TGSI_NUM_CHANNELS];
		unsigned chan;
		for (chan = 0; chan < TGSI_NUM_CHANNELS; chan++) {
			values[chan] = emit_fetch(bld_base, reg, type, chan);
		}
		return lp_build_gather_values(bld_base->base.gallivm, values,
					      TGSI_NUM_CHANNELS);
	}

	if (reg->Register.Indirect) {
		struct tgsi_declaration_range range = get_array_range(bld_base,
			reg->Register.File, &reg->Indirect);
		return LLVMBuildExtractElement(builder,
			emit_array_fetch(bld_base, reg->Register.File, type, range, swizzle),
			emit_array_index(bld, &reg->Indirect, reg->Register.Index - range.First),
			"");
	}

	switch(reg->Register.File) {
	case TGSI_FILE_IMMEDIATE: {
		LLVMTypeRef ctype = tgsi2llvmtype(bld_base, type);
		return LLVMConstBitCast(bld->immediates[reg->Register.Index][swizzle], ctype);
	}

	case TGSI_FILE_INPUT:
		result = ctx->inputs[radeon_llvm_reg_index_soa(reg->Register.Index, swizzle)];
		break;

	case TGSI_FILE_TEMPORARY:
		ptr = lp_get_temp_ptr_soa(bld, reg->Register.Index, swizzle);
		result = LLVMBuildLoad(builder, ptr, "");
		break;

	case TGSI_FILE_OUTPUT:
		ptr = lp_get_output_ptr(bld, reg->Register.Index, swizzle);
		result = LLVMBuildLoad(builder, ptr, "");
		break;

	default:
		return LLVMGetUndef(tgsi2llvmtype(bld_base, type));
	}

	return bitcast(bld_base, type, result);
}

static void emit_declaration(
	struct lp_build_tgsi_context * bld_base,
	const struct tgsi_full_declaration *decl)
{
	struct radeon_llvm_context * ctx = radeon_llvm_context(bld_base);
	switch(decl->Declaration.File) {
	case TGSI_FILE_ADDRESS:
	{
		 unsigned idx;
		for (idx = decl->Range.First; idx <= decl->Range.Last; idx++) {
			unsigned chan;
			for (chan = 0; chan < TGSI_NUM_CHANNELS; chan++) {
				 ctx->soa.addr[idx][chan] = lp_build_alloca(
					&ctx->gallivm,
					ctx->soa.bld_base.uint_bld.elem_type, "");
			}
		}
		break;
	}

	case TGSI_FILE_TEMPORARY:
		if (decl->Declaration.Array && decl->Array.ArrayID <= RADEON_LLVM_MAX_ARRAYS)
			ctx->arrays[decl->Array.ArrayID - 1] = decl->Range;
		lp_emit_declaration_soa(bld_base, decl);
		break;

	case TGSI_FILE_INPUT:
	{
		unsigned idx;
		for (idx = decl->Range.First; idx <= decl->Range.Last; idx++) {
			ctx->load_input(ctx, idx, decl);
		}
	}
	break;

	case TGSI_FILE_SYSTEM_VALUE:
	{
		unsigned idx;
		for (idx = decl->Range.First; idx <= decl->Range.Last; idx++) {
			ctx->load_system_value(ctx, idx, decl);
		}
	}
	break;

	case TGSI_FILE_OUTPUT:
	{
		unsigned idx;
		for (idx = decl->Range.First; idx <= decl->Range.Last; idx++) {
			unsigned chan;
			assert(idx < RADEON_LLVM_MAX_OUTPUTS);
			for (chan = 0; chan < TGSI_NUM_CHANNELS; chan++) {
				ctx->soa.outputs[idx][chan] = lp_build_alloca(&ctx->gallivm,
					ctx->soa.bld_base.base.elem_type, "");
			}
		}

		ctx->output_reg_count = MAX2(ctx->output_reg_count,
							 decl->Range.Last + 1);
		break;
	}

	default:
		break;
	}
}

static void
emit_store(
	struct lp_build_tgsi_context * bld_base,
	const struct tgsi_full_instruction * inst,
	const struct tgsi_opcode_info * info,
	LLVMValueRef dst[4])
{
	struct lp_build_tgsi_soa_context *bld = lp_soa_context(bld_base);
	struct gallivm_state *gallivm = bld->bld_base.base.gallivm;
	struct lp_build_context base = bld->bld_base.base;
	const struct tgsi_full_dst_register *reg = &inst->Dst[0];
	LLVMBuilderRef builder = bld->bld_base.base.gallivm->builder;
	LLVMValueRef temp_ptr;
	unsigned chan, chan_index;
	boolean is_vec_store = FALSE;

	if (dst[0]) {
		LLVMTypeKind k = LLVMGetTypeKind(LLVMTypeOf(dst[0]));
		is_vec_store = (k == LLVMVectorTypeKind);
	}

	if (is_vec_store) {
		LLVMValueRef values[4] = {};
		TGSI_FOR_EACH_DST0_ENABLED_CHANNEL(inst, chan) {
			LLVMValueRef index = lp_build_const_int32(gallivm, chan);
			values[chan]  = LLVMBuildExtractElement(gallivm->builder,
							dst[0], index, "");
		}
		bld_base->emit_store(bld_base, inst, info, values);
		return;
	}

	TGSI_FOR_EACH_DST0_ENABLED_CHANNEL( inst, chan_index ) {
		LLVMValueRef value = dst[chan_index];

		if (inst->Instruction.Saturate != TGSI_SAT_NONE) {
			struct lp_build_emit_data clamp_emit_data;

			memset(&clamp_emit_data, 0, sizeof(clamp_emit_data));
			clamp_emit_data.arg_count = 3;
			clamp_emit_data.args[0] = value;
			clamp_emit_data.args[2] = base.one;

			switch(inst->Instruction.Saturate) {
			case TGSI_SAT_ZERO_ONE:
				clamp_emit_data.args[1] = base.zero;
				break;
			case TGSI_SAT_MINUS_PLUS_ONE:
				clamp_emit_data.args[1] = LLVMConstReal(
						base.elem_type, -1.0f);
				break;
			default:
				assert(0);
			}
			value = lp_build_emit_llvm(bld_base, TGSI_OPCODE_CLAMP,
						&clamp_emit_data);
		}

		if (reg->Register.File == TGSI_FILE_ADDRESS) {
			temp_ptr = bld->addr[reg->Register.Index][chan_index];
			LLVMBuildStore(builder, value, temp_ptr);
			continue;
		}
	
		value = bitcast(bld_base, TGSI_TYPE_FLOAT, value);

		if (reg->Register.Indirect) {
			struct tgsi_declaration_range range = get_array_range(bld_base,
				reg->Register.File, &reg->Indirect);

        		unsigned i, size = range.Last - range.First + 1;
			LLVMValueRef array = LLVMBuildInsertElement(builder,
				emit_array_fetch(bld_base, reg->Register.File, TGSI_TYPE_FLOAT, range, chan_index),
				value,  emit_array_index(bld, &reg->Indirect, reg->Register.Index - range.First), "");

        		for (i = 0; i < size; ++i) {
				switch(reg->Register.File) {
				case TGSI_FILE_OUTPUT:
					temp_ptr = bld->outputs[i + range.First][chan_index];
					break;

				case TGSI_FILE_TEMPORARY:
					temp_ptr = lp_get_temp_ptr_soa(bld, i + range.First, chan_index);
					break;

				default:
					return;
				}
				value = LLVMBuildExtractElement(builder, array, 
					lp_build_const_int32(gallivm, i), "");
				LLVMBuildStore(builder, value, temp_ptr);
			}

		} else {
			switch(reg->Register.File) {
			case TGSI_FILE_OUTPUT:
				temp_ptr = bld->outputs[reg->Register.Index][chan_index];
				break;

			case TGSI_FILE_TEMPORARY:
				temp_ptr = lp_get_temp_ptr_soa(bld, reg->Register.Index, chan_index);
				break;

			default:
				return;
			}
			LLVMBuildStore(builder, value, temp_ptr);
		}
	}
}

static void bgnloop_emit(
	const struct lp_build_tgsi_action * action,
	struct lp_build_tgsi_context * bld_base,
	struct lp_build_emit_data * emit_data)
{
	struct radeon_llvm_context * ctx = radeon_llvm_context(bld_base);
	struct gallivm_state * gallivm = bld_base->base.gallivm;
	LLVMBasicBlockRef loop_block;
	LLVMBasicBlockRef endloop_block;
	endloop_block = LLVMAppendBasicBlockInContext(gallivm->context,
						ctx->main_fn, "ENDLOOP");
	loop_block = LLVMInsertBasicBlockInContext(gallivm->context,
						endloop_block, "LOOP");
	LLVMBuildBr(gallivm->builder, loop_block);
	LLVMPositionBuilderAtEnd(gallivm->builder, loop_block);
	ctx->loop_depth++;
	ctx->loop[ctx->loop_depth - 1].loop_block = loop_block;
	ctx->loop[ctx->loop_depth - 1].endloop_block = endloop_block;
}

static void brk_emit(
	const struct lp_build_tgsi_action * action,
	struct lp_build_tgsi_context * bld_base,
	struct lp_build_emit_data * emit_data)
{
	struct radeon_llvm_context * ctx = radeon_llvm_context(bld_base);
	struct gallivm_state * gallivm = bld_base->base.gallivm;
	struct radeon_llvm_loop * current_loop = get_current_loop(ctx);

	LLVMBuildBr(gallivm->builder, current_loop->endloop_block);
}

static void cont_emit(
	const struct lp_build_tgsi_action * action,
	struct lp_build_tgsi_context * bld_base,
	struct lp_build_emit_data * emit_data)
{
	struct radeon_llvm_context * ctx = radeon_llvm_context(bld_base);
	struct gallivm_state * gallivm = bld_base->base.gallivm;
	struct radeon_llvm_loop * current_loop = get_current_loop(ctx);

	LLVMBuildBr(gallivm->builder, current_loop->loop_block);
}

static void else_emit(
	const struct lp_build_tgsi_action * action,
	struct lp_build_tgsi_context * bld_base,
	struct lp_build_emit_data * emit_data)
{
	struct radeon_llvm_context * ctx = radeon_llvm_context(bld_base);
	struct gallivm_state * gallivm = bld_base->base.gallivm;
	struct radeon_llvm_branch * current_branch = get_current_branch(ctx);
	LLVMBasicBlockRef current_block = LLVMGetInsertBlock(gallivm->builder);

	/* We need to add a terminator to the current block if the previous
	 * instruction was an ENDIF.Example:
	 * IF
	 *   [code]
	 *   IF
	 *     [code]
	 *   ELSE
	 *    [code]
	 *   ENDIF <--
	 * ELSE<--
	 *   [code]
	 * ENDIF
	 */

	if (current_block != current_branch->if_block) {
		LLVMBuildBr(gallivm->builder, current_branch->endif_block);
	}
	if (!LLVMGetBasicBlockTerminator(current_branch->if_block)) {
		LLVMBuildBr(gallivm->builder, current_branch->endif_block);
	}
	current_branch->has_else = 1;
	LLVMPositionBuilderAtEnd(gallivm->builder, current_branch->else_block);
}

static void endif_emit(
	const struct lp_build_tgsi_action * action,
	struct lp_build_tgsi_context * bld_base,
	struct lp_build_emit_data * emit_data)
{
	struct radeon_llvm_context * ctx = radeon_llvm_context(bld_base);
	struct gallivm_state * gallivm = bld_base->base.gallivm;
	struct radeon_llvm_branch * current_branch = get_current_branch(ctx);
	LLVMBasicBlockRef current_block = LLVMGetInsertBlock(gallivm->builder);

	/* If we have consecutive ENDIF instructions, then the first ENDIF
	 * will not have a terminator, so we need to add one. */
	if (current_block != current_branch->if_block
			&& current_block != current_branch->else_block
			&& !LLVMGetBasicBlockTerminator(current_block)) {

		 LLVMBuildBr(gallivm->builder, current_branch->endif_block);
	}
	if (!LLVMGetBasicBlockTerminator(current_branch->else_block)) {
		LLVMPositionBuilderAtEnd(gallivm->builder, current_branch->else_block);
		LLVMBuildBr(gallivm->builder, current_branch->endif_block);
	}

	if (!LLVMGetBasicBlockTerminator(current_branch->if_block)) {
		LLVMPositionBuilderAtEnd(gallivm->builder, current_branch->if_block);
		LLVMBuildBr(gallivm->builder, current_branch->endif_block);
	}

	LLVMPositionBuilderAtEnd(gallivm->builder, current_branch->endif_block);
	ctx->branch_depth--;
}

static void endloop_emit(
	const struct lp_build_tgsi_action * action,
	struct lp_build_tgsi_context * bld_base,
	struct lp_build_emit_data * emit_data)
{
	struct radeon_llvm_context * ctx = radeon_llvm_context(bld_base);
	struct gallivm_state * gallivm = bld_base->base.gallivm;
	struct radeon_llvm_loop * current_loop = get_current_loop(ctx);

	if (!LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(gallivm->builder))) {
		 LLVMBuildBr(gallivm->builder, current_loop->loop_block);
	}

	LLVMPositionBuilderAtEnd(gallivm->builder, current_loop->endloop_block);
	ctx->loop_depth--;
}

static void if_emit(
	const struct lp_build_tgsi_action * action,
	struct lp_build_tgsi_context * bld_base,
	struct lp_build_emit_data * emit_data)
{
	struct radeon_llvm_context * ctx = radeon_llvm_context(bld_base);
	struct gallivm_state * gallivm = bld_base->base.gallivm;
	LLVMValueRef cond;
	LLVMBasicBlockRef if_block, else_block, endif_block;

	cond = LLVMBuildICmp(gallivm->builder, LLVMIntNE,
	        bitcast(bld_base, TGSI_TYPE_UNSIGNED, emit_data->args[0]),
			bld_base->int_bld.zero, "");

	endif_block = LLVMAppendBasicBlockInContext(gallivm->context,
						ctx->main_fn, "ENDIF");
	if_block = LLVMInsertBasicBlockInContext(gallivm->context,
						endif_block, "IF");
	else_block = LLVMInsertBasicBlockInContext(gallivm->context,
						endif_block, "ELSE");
	LLVMBuildCondBr(gallivm->builder, cond, if_block, else_block);
	LLVMPositionBuilderAtEnd(gallivm->builder, if_block);

	ctx->branch_depth++;
	ctx->branch[ctx->branch_depth - 1].endif_block = endif_block;
	ctx->branch[ctx->branch_depth - 1].if_block = if_block;
	ctx->branch[ctx->branch_depth - 1].else_block = else_block;
	ctx->branch[ctx->branch_depth - 1].has_else = 0;
}

static void kil_emit(
	const struct lp_build_tgsi_action * action,
	struct lp_build_tgsi_context * bld_base,
	struct lp_build_emit_data * emit_data)
{
	unsigned i;
	for (i = 0; i < emit_data->arg_count; i++) {
		emit_data->output[i] = lp_build_intrinsic_unary(
			bld_base->base.gallivm->builder,
			action->intr_name,
			emit_data->dst_type, emit_data->args[i]);
	}
}

void radeon_llvm_emit_prepare_cube_coords(
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data,
		LLVMValueRef *coords_arg)
{

	unsigned target = emit_data->inst->Texture.Texture;
	unsigned opcode = emit_data->inst->Instruction.Opcode;
	struct gallivm_state * gallivm = bld_base->base.gallivm;
	LLVMBuilderRef builder = gallivm->builder;
	LLVMTypeRef type = bld_base->base.elem_type;
	LLVMValueRef coords[4];
	LLVMValueRef mad_args[3];
	LLVMValueRef idx;
	struct LLVMOpaqueValue *cube_vec;
	LLVMValueRef v;
	unsigned i;

	cube_vec = lp_build_gather_values(bld_base->base.gallivm, coords_arg, 4);
	v = build_intrinsic(builder, "llvm.AMDGPU.cube", LLVMVectorType(type, 4),
                            &cube_vec, 1, LLVMReadNoneAttribute);

	for (i = 0; i < 4; ++i) {
		idx = lp_build_const_int32(gallivm, i);
		coords[i] = LLVMBuildExtractElement(builder, v, idx, "");
	}

	coords[2] = build_intrinsic(builder, "fabs",
			type, &coords[2], 1, LLVMReadNoneAttribute);
	coords[2] = lp_build_emit_llvm_unary(bld_base, TGSI_OPCODE_RCP, coords[2]);

	mad_args[1] = coords[2];
	mad_args[2] = LLVMConstReal(type, 1.5);

	mad_args[0] = coords[0];
	coords[0] = lp_build_emit_llvm_ternary(bld_base, TGSI_OPCODE_MAD,
			mad_args[0], mad_args[1], mad_args[2]);

	mad_args[0] = coords[1];
	coords[1] = lp_build_emit_llvm_ternary(bld_base, TGSI_OPCODE_MAD,
			mad_args[0], mad_args[1], mad_args[2]);

	/* apply xyz = yxw swizzle to cooords */
	coords[2] = coords[3];
	coords[3] = coords[1];
	coords[1] = coords[0];
	coords[0] = coords[3];

	/* all cases except simple cube map sampling require special handling
	 * for coord vector */
	if (target != TGSI_TEXTURE_CUBE ||
		opcode != TGSI_OPCODE_TEX) {

		/* for cube arrays coord.z = coord.w(array_index) * 8 + face */
		if (target == TGSI_TEXTURE_CUBE_ARRAY ||
			target == TGSI_TEXTURE_SHADOWCUBE_ARRAY) {

			/* coords_arg.w component - array_index for cube arrays or
			 * compare value for SHADOWCUBE */
			coords[2] = lp_build_emit_llvm_ternary(bld_base, TGSI_OPCODE_MAD,
					coords_arg[3], lp_build_const_float(gallivm, 8.0), coords[2]);
		}

		/* for instructions that need additional src (compare/lod/bias),
		 * put it in coord.w */
		if (opcode == TGSI_OPCODE_TEX2 ||
			opcode == TGSI_OPCODE_TXB2 ||
			opcode == TGSI_OPCODE_TXL2) {
			coords[3] = coords_arg[4];
		}
	}

	memcpy(coords_arg, coords, sizeof(coords));
}

static void txd_fetch_args(
	struct lp_build_tgsi_context * bld_base,
	struct lp_build_emit_data * emit_data)
{
	const struct tgsi_full_instruction * inst = emit_data->inst;

	LLVMValueRef coords[4];
	unsigned chan, src;
	for (src = 0; src < 3; src++) {
		for (chan = 0; chan < 4; chan++)
			coords[chan] = lp_build_emit_fetch(bld_base, inst, src, chan);

		emit_data->args[src] = lp_build_gather_values(bld_base->base.gallivm,
				coords, 4);
	}
	emit_data->arg_count = 3;
	emit_data->dst_type = LLVMVectorType(bld_base->base.elem_type, 4);
}


static void txp_fetch_args(
	struct lp_build_tgsi_context * bld_base,
	struct lp_build_emit_data * emit_data)
{
	const struct tgsi_full_instruction * inst = emit_data->inst;
	LLVMValueRef src_w;
	unsigned chan;
	LLVMValueRef coords[4];

	emit_data->dst_type = LLVMVectorType(bld_base->base.elem_type, 4);
	src_w = lp_build_emit_fetch(bld_base, emit_data->inst, 0, TGSI_CHAN_W);

	for (chan = 0; chan < 3; chan++ ) {
		LLVMValueRef arg = lp_build_emit_fetch(bld_base,
						emit_data->inst, 0, chan);
		coords[chan] = lp_build_emit_llvm_binary(bld_base,
					TGSI_OPCODE_DIV, arg, src_w);
	}
	coords[3] = bld_base->base.one;

	if ((inst->Texture.Texture == TGSI_TEXTURE_CUBE ||
	     inst->Texture.Texture == TGSI_TEXTURE_CUBE_ARRAY ||
	     inst->Texture.Texture == TGSI_TEXTURE_SHADOWCUBE ||
	     inst->Texture.Texture == TGSI_TEXTURE_SHADOWCUBE_ARRAY) &&
	    inst->Instruction.Opcode != TGSI_OPCODE_TXQ &&
	    inst->Instruction.Opcode != TGSI_OPCODE_TXQ_LZ) {
		radeon_llvm_emit_prepare_cube_coords(bld_base, emit_data, coords);
	}

	emit_data->args[0] = lp_build_gather_values(bld_base->base.gallivm,
						coords, 4);
	emit_data->arg_count = 1;
}

static void tex_fetch_args(
	struct lp_build_tgsi_context * bld_base,
	struct lp_build_emit_data * emit_data)
{
	/* XXX: lp_build_swizzle_aos() was failing with wrong arg types,
	 * when we used CHAN_ALL.  We should be able to get this to work,
	 * but for now we will swizzle it ourselves
	emit_data->args[0] = lp_build_emit_fetch(bld_base, emit_data->inst,
						 0, CHAN_ALL);

	*/

	const struct tgsi_full_instruction * inst = emit_data->inst;

	LLVMValueRef coords[5];
	unsigned chan;
	for (chan = 0; chan < 4; chan++) {
		coords[chan] = lp_build_emit_fetch(bld_base, inst, 0, chan);
	}

	if (inst->Instruction.Opcode == TGSI_OPCODE_TEX2 ||
		inst->Instruction.Opcode == TGSI_OPCODE_TXB2 ||
		inst->Instruction.Opcode == TGSI_OPCODE_TXL2) {
		/* These instructions have additional operand that should be packed
		 * into the cube coord vector by radeon_llvm_emit_prepare_cube_coords.
		 * That operand should be passed as a float value in the args array
		 * right after the coord vector. After packing it's not used anymore,
		 * that's why arg_count is not increased */
		coords[4] = lp_build_emit_fetch(bld_base, inst, 1, 0);
	}

	if ((inst->Texture.Texture == TGSI_TEXTURE_CUBE ||
	     inst->Texture.Texture == TGSI_TEXTURE_CUBE_ARRAY ||
	     inst->Texture.Texture == TGSI_TEXTURE_SHADOWCUBE ||
	     inst->Texture.Texture == TGSI_TEXTURE_SHADOWCUBE_ARRAY) &&
	    inst->Instruction.Opcode != TGSI_OPCODE_TXQ &&
	    inst->Instruction.Opcode != TGSI_OPCODE_TXQ_LZ) {
		radeon_llvm_emit_prepare_cube_coords(bld_base, emit_data, coords);
	}

	emit_data->arg_count = 1;
	emit_data->args[0] = lp_build_gather_values(bld_base->base.gallivm,
						coords, 4);
	emit_data->dst_type = LLVMVectorType(bld_base->base.elem_type, 4);
}

static void txf_fetch_args(
	struct lp_build_tgsi_context * bld_base,
	struct lp_build_emit_data * emit_data)
{
	const struct tgsi_full_instruction * inst = emit_data->inst;
	struct lp_build_tgsi_soa_context *bld = lp_soa_context(bld_base);
	const struct tgsi_texture_offset * off = inst->TexOffsets;
	LLVMTypeRef offset_type = bld_base->int_bld.elem_type;

	/* fetch tex coords */
	tex_fetch_args(bld_base, emit_data);

	/* fetch tex offsets */
	if (inst->Texture.NumOffsets) {
		assert(inst->Texture.NumOffsets == 1);

		emit_data->args[1] = LLVMConstBitCast(
			bld->immediates[off->Index][off->SwizzleX],
			offset_type);
		emit_data->args[2] = LLVMConstBitCast(
			bld->immediates[off->Index][off->SwizzleY],
			offset_type);
		emit_data->args[3] = LLVMConstBitCast(
			bld->immediates[off->Index][off->SwizzleZ],
			offset_type);
	} else {
		emit_data->args[1] = bld_base->int_bld.zero;
		emit_data->args[2] = bld_base->int_bld.zero;
		emit_data->args[3] = bld_base->int_bld.zero;
	}

	emit_data->arg_count = 4;
}

static void emit_icmp(
		const struct lp_build_tgsi_action * action,
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data)
{
	unsigned pred;
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	LLVMContextRef context = bld_base->base.gallivm->context;

	switch (emit_data->inst->Instruction.Opcode) {
	case TGSI_OPCODE_USEQ: pred = LLVMIntEQ; break;
	case TGSI_OPCODE_USNE: pred = LLVMIntNE; break;
	case TGSI_OPCODE_USGE: pred = LLVMIntUGE; break;
	case TGSI_OPCODE_USLT: pred = LLVMIntULT; break;
	case TGSI_OPCODE_ISGE: pred = LLVMIntSGE; break;
	case TGSI_OPCODE_ISLT: pred = LLVMIntSLT; break;
	default:
		assert(!"unknown instruction");
		pred = 0;
		break;
	}

	LLVMValueRef v = LLVMBuildICmp(builder, pred,
			emit_data->args[0], emit_data->args[1],"");

	v = LLVMBuildSExtOrBitCast(builder, v,
			LLVMInt32TypeInContext(context), "");

	emit_data->output[emit_data->chan] = v;
}

static void emit_ucmp(
		const struct lp_build_tgsi_action * action,
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data)
{
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;

	LLVMValueRef v = LLVMBuildFCmp(builder, LLVMRealUGE,
			emit_data->args[0], lp_build_const_float(bld_base->base.gallivm, 0.), "");

	emit_data->output[emit_data->chan] = LLVMBuildSelect(builder, v, emit_data->args[2], emit_data->args[1], "");
}

static void emit_cmp(
		const struct lp_build_tgsi_action *action,
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data)
{
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	LLVMRealPredicate pred;
	LLVMValueRef cond;

	/* XXX I'm not sure whether to do unordered or ordered comparisons,
	 * but llvmpipe uses unordered comparisons, so for consistency we use
	 * unordered.  (The authors of llvmpipe aren't sure about using
	 * unordered vs ordered comparisons either.
	 */
	switch (emit_data->inst->Instruction.Opcode) {
	case TGSI_OPCODE_SGE: pred = LLVMRealUGE; break;
	case TGSI_OPCODE_SEQ: pred = LLVMRealUEQ; break;
	case TGSI_OPCODE_SLE: pred = LLVMRealULE; break;
	case TGSI_OPCODE_SLT: pred = LLVMRealULT; break;
	case TGSI_OPCODE_SNE: pred = LLVMRealUNE; break;
	case TGSI_OPCODE_SGT: pred = LLVMRealUGT; break;
	default: assert(!"unknown instruction"); pred = 0; break;
	}

	cond = LLVMBuildFCmp(builder,
		pred, emit_data->args[0], emit_data->args[1], "");

	emit_data->output[emit_data->chan] = LLVMBuildSelect(builder,
		cond, bld_base->base.one, bld_base->base.zero, "");
}

static void emit_not(
		const struct lp_build_tgsi_action * action,
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data)
{
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	LLVMValueRef v = bitcast(bld_base, TGSI_TYPE_UNSIGNED,
			emit_data->args[0]);
	emit_data->output[emit_data->chan] = LLVMBuildNot(builder, v, "");
}

static void emit_arl(
		const struct lp_build_tgsi_action * action,
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data)
{
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	LLVMValueRef floor_index =  lp_build_emit_llvm_unary(bld_base, TGSI_OPCODE_FLR, emit_data->args[0]);
	emit_data->output[emit_data->chan] = LLVMBuildFPToSI(builder,
			floor_index, bld_base->base.int_elem_type , "");
}

static void emit_and(
		const struct lp_build_tgsi_action * action,
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data)
{
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	emit_data->output[emit_data->chan] = LLVMBuildAnd(builder,
			emit_data->args[0], emit_data->args[1], "");
}

static void emit_or(
		const struct lp_build_tgsi_action * action,
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data)
{
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	emit_data->output[emit_data->chan] = LLVMBuildOr(builder,
			emit_data->args[0], emit_data->args[1], "");
}

static void emit_uadd(
		const struct lp_build_tgsi_action * action,
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data)
{
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	emit_data->output[emit_data->chan] = LLVMBuildAdd(builder,
			emit_data->args[0], emit_data->args[1], "");
}

static void emit_udiv(
		const struct lp_build_tgsi_action * action,
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data)
{
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	emit_data->output[emit_data->chan] = LLVMBuildUDiv(builder,
			emit_data->args[0], emit_data->args[1], "");
}

static void emit_idiv(
		const struct lp_build_tgsi_action * action,
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data)
{
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	emit_data->output[emit_data->chan] = LLVMBuildSDiv(builder,
			emit_data->args[0], emit_data->args[1], "");
}

static void emit_mod(
		const struct lp_build_tgsi_action * action,
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data)
{
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	emit_data->output[emit_data->chan] = LLVMBuildSRem(builder,
			emit_data->args[0], emit_data->args[1], "");
}

static void emit_umod(
		const struct lp_build_tgsi_action * action,
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data)
{
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	emit_data->output[emit_data->chan] = LLVMBuildURem(builder,
			emit_data->args[0], emit_data->args[1], "");
}

static void emit_shl(
		const struct lp_build_tgsi_action * action,
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data)
{
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	emit_data->output[emit_data->chan] = LLVMBuildShl(builder,
			emit_data->args[0], emit_data->args[1], "");
}

static void emit_ushr(
		const struct lp_build_tgsi_action * action,
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data)
{
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	emit_data->output[emit_data->chan] = LLVMBuildLShr(builder,
			emit_data->args[0], emit_data->args[1], "");
}
static void emit_ishr(
		const struct lp_build_tgsi_action * action,
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data)
{
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	emit_data->output[emit_data->chan] = LLVMBuildAShr(builder,
			emit_data->args[0], emit_data->args[1], "");
}

static void emit_xor(
		const struct lp_build_tgsi_action * action,
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data)
{
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	emit_data->output[emit_data->chan] = LLVMBuildXor(builder,
			emit_data->args[0], emit_data->args[1], "");
}

static void emit_ssg(
		const struct lp_build_tgsi_action * action,
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data)
{
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;

	LLVMValueRef cmp, val;

	if (emit_data->inst->Instruction.Opcode == TGSI_OPCODE_ISSG) {
		cmp = LLVMBuildICmp(builder, LLVMIntSGT, emit_data->args[0], bld_base->int_bld.zero, "");
		val = LLVMBuildSelect(builder, cmp, bld_base->int_bld.one, emit_data->args[0], "");
		cmp = LLVMBuildICmp(builder, LLVMIntSGE, val, bld_base->int_bld.zero, "");
		val = LLVMBuildSelect(builder, cmp, val, LLVMConstInt(bld_base->int_bld.elem_type, -1, true), "");
	} else { // float SSG
		cmp = LLVMBuildFCmp(builder, LLVMRealUGT, emit_data->args[0], bld_base->base.zero, "");
		val = LLVMBuildSelect(builder, cmp, bld_base->base.one, emit_data->args[0], "");
		cmp = LLVMBuildFCmp(builder, LLVMRealUGE, val, bld_base->base.zero, "");
		val = LLVMBuildSelect(builder, cmp, val, LLVMConstReal(bld_base->base.elem_type, -1), "");
	}

	emit_data->output[emit_data->chan] = val;
}

static void emit_ineg(
		const struct lp_build_tgsi_action * action,
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data)
{
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	emit_data->output[emit_data->chan] = LLVMBuildNeg(builder,
			emit_data->args[0], "");
}

static void emit_f2i(
		const struct lp_build_tgsi_action * action,
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data)
{
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	emit_data->output[emit_data->chan] = LLVMBuildFPToSI(builder,
			emit_data->args[0], bld_base->int_bld.elem_type, "");
}

static void emit_f2u(
		const struct lp_build_tgsi_action * action,
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data)
{
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	emit_data->output[emit_data->chan] = LLVMBuildFPToUI(builder,
			emit_data->args[0], bld_base->uint_bld.elem_type, "");
}

static void emit_i2f(
		const struct lp_build_tgsi_action * action,
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data)
{
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	emit_data->output[emit_data->chan] = LLVMBuildSIToFP(builder,
			emit_data->args[0], bld_base->base.elem_type, "");
}

static void emit_u2f(
		const struct lp_build_tgsi_action * action,
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data)
{
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	emit_data->output[emit_data->chan] = LLVMBuildUIToFP(builder,
			emit_data->args[0], bld_base->base.elem_type, "");
}

static void emit_immediate(struct lp_build_tgsi_context * bld_base,
		const struct tgsi_full_immediate *imm)
{
	unsigned i;
	struct radeon_llvm_context * ctx = radeon_llvm_context(bld_base);

	for (i = 0; i < 4; ++i) {
		ctx->soa.immediates[ctx->soa.num_immediates][i] =
				LLVMConstInt(bld_base->uint_bld.elem_type, imm->u[i].Uint, false   );
	}

	ctx->soa.num_immediates++;
}

LLVMValueRef
build_intrinsic(LLVMBuilderRef builder,
                   const char *name,
                   LLVMTypeRef ret_type,
                   LLVMValueRef *args,
                   unsigned num_args,
                   LLVMAttribute attr)
{
   LLVMModuleRef module = LLVMGetGlobalParent(LLVMGetBasicBlockParent(LLVMGetInsertBlock(builder)));
   LLVMValueRef function;

   function = LLVMGetNamedFunction(module, name);
   if(!function) {
      LLVMTypeRef arg_types[LP_MAX_FUNC_ARGS];
      unsigned i;

      assert(num_args <= LP_MAX_FUNC_ARGS);

      for(i = 0; i < num_args; ++i) {
         assert(args[i]);
         arg_types[i] = LLVMTypeOf(args[i]);
      }

      function = lp_declare_intrinsic(module, name, ret_type, arg_types, num_args);

      if (attr)
          LLVMAddFunctionAttr(function, attr);
   }

   return LLVMBuildCall(builder, function, args, num_args, "");
}

static void build_tgsi_intrinsic(
 const struct lp_build_tgsi_action * action,
 struct lp_build_tgsi_context * bld_base,
 struct lp_build_emit_data * emit_data,
 LLVMAttribute attr)
{
   struct lp_build_context * base = &bld_base->base;
   emit_data->output[emit_data->chan] = build_intrinsic(
               base->gallivm->builder, action->intr_name,
               emit_data->dst_type, emit_data->args,
               emit_data->arg_count, attr);
}
void
build_tgsi_intrinsic_nomem(
 const struct lp_build_tgsi_action * action,
 struct lp_build_tgsi_context * bld_base,
 struct lp_build_emit_data * emit_data)
{
	build_tgsi_intrinsic(action, bld_base, emit_data, LLVMReadNoneAttribute);
}

static void build_tgsi_intrinsic_readonly(
 const struct lp_build_tgsi_action * action,
 struct lp_build_tgsi_context * bld_base,
 struct lp_build_emit_data * emit_data)
{
	build_tgsi_intrinsic(action, bld_base, emit_data, LLVMReadOnlyAttribute);
}

void radeon_llvm_context_init(struct radeon_llvm_context * ctx)
{
	struct lp_type type;

	/* Initialize the gallivm object:
	 * We are only using the module, context, and builder fields of this struct.
	 * This should be enough for us to be able to pass our gallivm struct to the
	 * helper functions in the gallivm module.
	 */
	memset(&ctx->gallivm, 0, sizeof (ctx->gallivm));
	memset(&ctx->soa, 0, sizeof(ctx->soa));
	ctx->gallivm.context = LLVMContextCreate();
	ctx->gallivm.module = LLVMModuleCreateWithNameInContext("tgsi",
						ctx->gallivm.context);
	ctx->gallivm.builder = LLVMCreateBuilderInContext(ctx->gallivm.context);

	ctx->store_output_intr = "llvm.AMDGPU.store.output.";
	ctx->swizzle_intr = "llvm.AMDGPU.swizzle";
	struct lp_build_tgsi_context * bld_base = &ctx->soa.bld_base;

	/* XXX: We need to revisit this.I think the correct way to do this is
	 * to use length = 4 here and use the elem_bld for everything. */
	type.floating = TRUE;
	type.sign = TRUE;
	type.width = 32;
	type.length = 1;

	lp_build_context_init(&bld_base->base, &ctx->gallivm, type);
	lp_build_context_init(&ctx->soa.bld_base.uint_bld, &ctx->gallivm, lp_uint_type(type));
	lp_build_context_init(&ctx->soa.bld_base.int_bld, &ctx->gallivm, lp_int_type(type));

	bld_base->soa = 1;
	bld_base->emit_store = emit_store;
	bld_base->emit_swizzle = emit_swizzle;
	bld_base->emit_declaration = emit_declaration;
	bld_base->emit_immediate = emit_immediate;

	bld_base->emit_fetch_funcs[TGSI_FILE_IMMEDIATE] = emit_fetch;
	bld_base->emit_fetch_funcs[TGSI_FILE_INPUT] = emit_fetch;
	bld_base->emit_fetch_funcs[TGSI_FILE_TEMPORARY] = emit_fetch;
	bld_base->emit_fetch_funcs[TGSI_FILE_OUTPUT] = emit_fetch;

	/* Allocate outputs */
	ctx->soa.outputs = ctx->outputs;

	ctx->num_arrays = 0;

	/* XXX: Is there a better way to initialize all this ? */

	lp_set_default_actions(bld_base);

	bld_base->op_actions[TGSI_OPCODE_ABS].emit = build_tgsi_intrinsic_readonly;
	bld_base->op_actions[TGSI_OPCODE_ABS].intr_name = "fabs";
	bld_base->op_actions[TGSI_OPCODE_ARL].emit = emit_arl;
	bld_base->op_actions[TGSI_OPCODE_AND].emit = emit_and;
	bld_base->op_actions[TGSI_OPCODE_BGNLOOP].emit = bgnloop_emit;
	bld_base->op_actions[TGSI_OPCODE_BRK].emit = brk_emit;
	bld_base->op_actions[TGSI_OPCODE_CEIL].emit = build_tgsi_intrinsic_readonly;
	bld_base->op_actions[TGSI_OPCODE_CEIL].intr_name = "ceil";
	bld_base->op_actions[TGSI_OPCODE_CLAMP].emit = build_tgsi_intrinsic_nomem;
	bld_base->op_actions[TGSI_OPCODE_CLAMP].intr_name = "llvm.AMDIL.clamp.";
	bld_base->op_actions[TGSI_OPCODE_CMP].emit = build_tgsi_intrinsic_nomem;
	bld_base->op_actions[TGSI_OPCODE_CMP].intr_name = "llvm.AMDGPU.cndlt";
	bld_base->op_actions[TGSI_OPCODE_CONT].emit = cont_emit;
	bld_base->op_actions[TGSI_OPCODE_COS].emit = build_tgsi_intrinsic_readonly;
	bld_base->op_actions[TGSI_OPCODE_COS].intr_name = "llvm.cos.f32";
	bld_base->op_actions[TGSI_OPCODE_DDX].intr_name = "llvm.AMDGPU.ddx";
	bld_base->op_actions[TGSI_OPCODE_DDX].fetch_args = tex_fetch_args;
	bld_base->op_actions[TGSI_OPCODE_DDY].intr_name = "llvm.AMDGPU.ddy";
	bld_base->op_actions[TGSI_OPCODE_DDY].fetch_args = tex_fetch_args;
	bld_base->op_actions[TGSI_OPCODE_ELSE].emit = else_emit;
	bld_base->op_actions[TGSI_OPCODE_ENDIF].emit = endif_emit;
	bld_base->op_actions[TGSI_OPCODE_ENDLOOP].emit = endloop_emit;
	bld_base->op_actions[TGSI_OPCODE_EX2].emit = build_tgsi_intrinsic_nomem;
	bld_base->op_actions[TGSI_OPCODE_EX2].intr_name = "llvm.AMDIL.exp.";
	bld_base->op_actions[TGSI_OPCODE_FLR].emit = build_tgsi_intrinsic_readonly;
	bld_base->op_actions[TGSI_OPCODE_FLR].intr_name = "floor";
	bld_base->op_actions[TGSI_OPCODE_FRC].emit = build_tgsi_intrinsic_nomem;
	bld_base->op_actions[TGSI_OPCODE_FRC].intr_name = "llvm.AMDIL.fraction.";
	bld_base->op_actions[TGSI_OPCODE_F2I].emit = emit_f2i;
	bld_base->op_actions[TGSI_OPCODE_F2U].emit = emit_f2u;
	bld_base->op_actions[TGSI_OPCODE_IABS].emit = build_tgsi_intrinsic_nomem;
	bld_base->op_actions[TGSI_OPCODE_IABS].intr_name = "llvm.AMDIL.abs.";
	bld_base->op_actions[TGSI_OPCODE_IDIV].emit = emit_idiv;
	bld_base->op_actions[TGSI_OPCODE_IF].emit = if_emit;
	bld_base->op_actions[TGSI_OPCODE_IMAX].emit = build_tgsi_intrinsic_nomem;
	bld_base->op_actions[TGSI_OPCODE_IMAX].intr_name = "llvm.AMDGPU.imax";
	bld_base->op_actions[TGSI_OPCODE_IMIN].emit = build_tgsi_intrinsic_nomem;
	bld_base->op_actions[TGSI_OPCODE_IMIN].intr_name = "llvm.AMDGPU.imin";
	bld_base->op_actions[TGSI_OPCODE_INEG].emit = emit_ineg;
	bld_base->op_actions[TGSI_OPCODE_ISHR].emit = emit_ishr;
	bld_base->op_actions[TGSI_OPCODE_ISGE].emit = emit_icmp;
	bld_base->op_actions[TGSI_OPCODE_ISLT].emit = emit_icmp;
	bld_base->op_actions[TGSI_OPCODE_ISSG].emit = emit_ssg;
	bld_base->op_actions[TGSI_OPCODE_I2F].emit = emit_i2f;
	bld_base->op_actions[TGSI_OPCODE_KIL].emit = kil_emit;
	bld_base->op_actions[TGSI_OPCODE_KIL].intr_name = "llvm.AMDGPU.kill";
	bld_base->op_actions[TGSI_OPCODE_KILP].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_KILP].intr_name = "llvm.AMDGPU.kilp";
	bld_base->op_actions[TGSI_OPCODE_LG2].emit = build_tgsi_intrinsic_readonly;
	bld_base->op_actions[TGSI_OPCODE_LG2].intr_name = "llvm.log2.f32";
	bld_base->op_actions[TGSI_OPCODE_LRP].emit = build_tgsi_intrinsic_nomem;
	bld_base->op_actions[TGSI_OPCODE_LRP].intr_name = "llvm.AMDGPU.lrp";
	bld_base->op_actions[TGSI_OPCODE_MOD].emit = emit_mod;
	bld_base->op_actions[TGSI_OPCODE_NOT].emit = emit_not;
	bld_base->op_actions[TGSI_OPCODE_OR].emit = emit_or;
	bld_base->op_actions[TGSI_OPCODE_POW].emit = build_tgsi_intrinsic_readonly;
	bld_base->op_actions[TGSI_OPCODE_POW].intr_name = "llvm.pow.f32";
	bld_base->op_actions[TGSI_OPCODE_ROUND].emit = build_tgsi_intrinsic_nomem;
	bld_base->op_actions[TGSI_OPCODE_ROUND].intr_name = "llvm.AMDIL.round.nearest.";
	bld_base->op_actions[TGSI_OPCODE_SGE].emit = emit_cmp;
	bld_base->op_actions[TGSI_OPCODE_SEQ].emit = emit_cmp;
	bld_base->op_actions[TGSI_OPCODE_SHL].emit = emit_shl;
	bld_base->op_actions[TGSI_OPCODE_SLE].emit = emit_cmp;
	bld_base->op_actions[TGSI_OPCODE_SLT].emit = emit_cmp;
	bld_base->op_actions[TGSI_OPCODE_SNE].emit = emit_cmp;
	bld_base->op_actions[TGSI_OPCODE_SGT].emit = emit_cmp;
	bld_base->op_actions[TGSI_OPCODE_SIN].emit = build_tgsi_intrinsic_readonly;
	bld_base->op_actions[TGSI_OPCODE_SIN].intr_name = "llvm.sin.f32";
	bld_base->op_actions[TGSI_OPCODE_SSG].emit = emit_ssg;
	bld_base->op_actions[TGSI_OPCODE_TEX].fetch_args = tex_fetch_args;
	bld_base->op_actions[TGSI_OPCODE_TEX].intr_name = "llvm.AMDGPU.tex";
	bld_base->op_actions[TGSI_OPCODE_TEX2].fetch_args = tex_fetch_args;
	bld_base->op_actions[TGSI_OPCODE_TEX2].intr_name = "llvm.AMDGPU.tex";
	bld_base->op_actions[TGSI_OPCODE_TXB].fetch_args = tex_fetch_args;
	bld_base->op_actions[TGSI_OPCODE_TXB].intr_name = "llvm.AMDGPU.txb";
	bld_base->op_actions[TGSI_OPCODE_TXB2].fetch_args = tex_fetch_args;
	bld_base->op_actions[TGSI_OPCODE_TXB2].intr_name = "llvm.AMDGPU.txb";
	bld_base->op_actions[TGSI_OPCODE_TXD].fetch_args = txd_fetch_args;
	bld_base->op_actions[TGSI_OPCODE_TXD].intr_name = "llvm.AMDGPU.txd";
	bld_base->op_actions[TGSI_OPCODE_TXF].fetch_args = txf_fetch_args;
	bld_base->op_actions[TGSI_OPCODE_TXF].intr_name = "llvm.AMDGPU.txf";
	bld_base->op_actions[TGSI_OPCODE_TXL].fetch_args = tex_fetch_args;
	bld_base->op_actions[TGSI_OPCODE_TXL].intr_name = "llvm.AMDGPU.txl";
	bld_base->op_actions[TGSI_OPCODE_TXL2].fetch_args = tex_fetch_args;
	bld_base->op_actions[TGSI_OPCODE_TXL2].intr_name = "llvm.AMDGPU.txl";
	bld_base->op_actions[TGSI_OPCODE_TXP].fetch_args = txp_fetch_args;
	bld_base->op_actions[TGSI_OPCODE_TXP].intr_name = "llvm.AMDGPU.tex";
	bld_base->op_actions[TGSI_OPCODE_TXQ].fetch_args = tex_fetch_args;
	bld_base->op_actions[TGSI_OPCODE_TXQ].intr_name = "llvm.AMDGPU.txq";
	bld_base->op_actions[TGSI_OPCODE_TRUNC].emit = build_tgsi_intrinsic_nomem;
	bld_base->op_actions[TGSI_OPCODE_TRUNC].intr_name = "llvm.AMDGPU.trunc";
	bld_base->op_actions[TGSI_OPCODE_UADD].emit = emit_uadd;
	bld_base->op_actions[TGSI_OPCODE_UDIV].emit = emit_udiv;
	bld_base->op_actions[TGSI_OPCODE_UMAX].emit = build_tgsi_intrinsic_nomem;
	bld_base->op_actions[TGSI_OPCODE_UMAX].intr_name = "llvm.AMDGPU.umax";
	bld_base->op_actions[TGSI_OPCODE_UMIN].emit = build_tgsi_intrinsic_nomem;
	bld_base->op_actions[TGSI_OPCODE_UMIN].intr_name = "llvm.AMDGPU.umin";
	bld_base->op_actions[TGSI_OPCODE_UMOD].emit = emit_umod;
	bld_base->op_actions[TGSI_OPCODE_USEQ].emit = emit_icmp;
	bld_base->op_actions[TGSI_OPCODE_USGE].emit = emit_icmp;
	bld_base->op_actions[TGSI_OPCODE_USHR].emit = emit_ushr;
	bld_base->op_actions[TGSI_OPCODE_USLT].emit = emit_icmp;
	bld_base->op_actions[TGSI_OPCODE_USNE].emit = emit_icmp;
	bld_base->op_actions[TGSI_OPCODE_U2F].emit = emit_u2f;
	bld_base->op_actions[TGSI_OPCODE_XOR].emit = emit_xor;
	bld_base->op_actions[TGSI_OPCODE_UCMP].emit = emit_ucmp;

	bld_base->rsq_action.emit = build_tgsi_intrinsic_nomem;
	bld_base->rsq_action.intr_name = "llvm.AMDGPU.rsq";
}

void radeon_llvm_create_func(struct radeon_llvm_context * ctx,
			     LLVMTypeRef *ParamTypes, unsigned ParamCount)
{
	LLVMTypeRef main_fn_type;
	LLVMBasicBlockRef main_fn_body;

	/* Setup the function */
	main_fn_type = LLVMFunctionType(LLVMVoidTypeInContext(ctx->gallivm.context),
					ParamTypes, ParamCount, 0);
	ctx->main_fn = LLVMAddFunction(ctx->gallivm.module, "main", main_fn_type);
	main_fn_body = LLVMAppendBasicBlockInContext(ctx->gallivm.context,
			ctx->main_fn, "main_body");
	LLVMPositionBuilderAtEnd(ctx->gallivm.builder, main_fn_body);
}

void radeon_llvm_finalize_module(struct radeon_llvm_context * ctx)
{
	struct gallivm_state * gallivm = ctx->soa.bld_base.base.gallivm;
	/* End the main function with Return*/
	LLVMBuildRetVoid(gallivm->builder);

	/* Create the pass manager */
	ctx->gallivm.passmgr = LLVMCreateFunctionPassManagerForModule(
							gallivm->module);

	/* This pass should eliminate all the load and store instructions */
	LLVMAddPromoteMemoryToRegisterPass(gallivm->passmgr);

	/* Add some optimization passes */
	LLVMAddScalarReplAggregatesPass(gallivm->passmgr);
	LLVMAddLICMPass(gallivm->passmgr);
	LLVMAddAggressiveDCEPass(gallivm->passmgr);
	LLVMAddCFGSimplificationPass(gallivm->passmgr);

	/* Run the passs */
	LLVMRunFunctionPassManager(gallivm->passmgr, ctx->main_fn);

	LLVMDisposeBuilder(gallivm->builder);
	LLVMDisposePassManager(gallivm->passmgr);

}

void radeon_llvm_dispose(struct radeon_llvm_context * ctx)
{
	LLVMDisposeModule(ctx->soa.bld_base.base.gallivm->module);
	LLVMContextDispose(ctx->soa.bld_base.base.gallivm->context);
}
