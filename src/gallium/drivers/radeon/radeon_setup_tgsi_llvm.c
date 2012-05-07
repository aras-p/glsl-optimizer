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
#include "util/u_debug.h"

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

static void radeon_llvm_fetch_args_2_reverse_soa(
	struct lp_build_tgsi_context * bld_base,
	struct lp_build_emit_data * emit_data)
{
	assert(emit_data->info->num_src == 2);
	emit_data->args[0] = lp_build_emit_fetch(bld_base, emit_data->inst,
							1, emit_data->chan);
	emit_data->args[1] = lp_build_emit_fetch(bld_base, emit_data->inst,
							0, emit_data->chan);
	emit_data->arg_count = 2;
	emit_data->dst_type = LLVMTypeOf(emit_data->args[0]);
}

static LLVMValueRef emit_swizzle(
	struct lp_build_tgsi_context * bld_base,
        LLVMValueRef value,
	unsigned swizzle_x,
	unsigned swizzle_y,
	unsigned swizzle_z,
	unsigned swizzle_w)
{
	unsigned char swizzles[4];
	swizzles[0] = swizzle_x;
	swizzles[1] = swizzle_y;
	swizzles[2] = swizzle_z;
	swizzles[3] = swizzle_w;


	return lp_build_swizzle_aos(&bld_base->base, value, swizzles);
}

static LLVMValueRef
emit_array_index(
	struct lp_build_tgsi_soa_context *bld,
	const struct tgsi_full_src_register *reg,
	unsigned swizzle)
{
	struct gallivm_state * gallivm = bld->bld_base.base.gallivm;

	LLVMValueRef addr = LLVMBuildLoad(gallivm->builder,
	bld->addr[reg->Indirect.Index][swizzle], "");
	LLVMValueRef offset = lp_build_const_int32(gallivm, reg->Register.Index);
	LLVMValueRef hw_index = LLVMBuildAdd(gallivm->builder, addr, offset, "");
	LLVMValueRef soa_index = LLVMBuildMul(gallivm->builder, hw_index,
	lp_build_const_int32(gallivm, 4), "");
	LLVMValueRef array_index = LLVMBuildAdd(gallivm->builder, soa_index,
	lp_build_const_int32(gallivm, swizzle), "");

	return array_index;
}

static LLVMValueRef
emit_fetch_immediate(
	struct lp_build_tgsi_context *bld_base,
	const struct tgsi_full_src_register *reg,
	enum tgsi_opcode_type type,
	unsigned swizzle)
{
	LLVMTypeRef ctype;
	LLVMContextRef ctx = bld_base->base.gallivm->context;

	switch (type) {
	case TGSI_TYPE_UNSIGNED:
	case TGSI_TYPE_SIGNED:
		ctype = LLVMInt32TypeInContext(ctx);
		break;
	case TGSI_TYPE_UNTYPED:
	case TGSI_TYPE_FLOAT:
		ctype = LLVMFloatTypeInContext(ctx);
		break;
	default:
		ctype = 0;
		break;
	}

	struct lp_build_tgsi_soa_context *bld = lp_soa_context(bld_base);
	return LLVMConstBitCast(bld->immediates[reg->Register.Index][swizzle], ctype);
}

static LLVMValueRef
emit_fetch_input(
	struct lp_build_tgsi_context *bld_base,
	const struct tgsi_full_src_register *reg,
	enum tgsi_opcode_type type,
	unsigned swizzle)
{
	struct radeon_llvm_context * ctx = radeon_llvm_context(bld_base);
	if (swizzle == ~0) {
		LLVMValueRef values[TGSI_NUM_CHANNELS] = {};
		unsigned chan;
		for (chan = 0; chan < TGSI_NUM_CHANNELS; chan++) {
			values[chan] = ctx->inputs[radeon_llvm_reg_index_soa(
						reg->Register.Index, chan)];
		}
		return lp_build_gather_values(bld_base->base.gallivm, values,
						TGSI_NUM_CHANNELS);
	} else {
		return bitcast(bld_base, type, ctx->inputs[radeon_llvm_reg_index_soa(reg->Register.Index, swizzle)]);
	}
}

static LLVMValueRef
emit_fetch_temporary(
	struct lp_build_tgsi_context *bld_base,
	const struct tgsi_full_src_register *reg,
	enum tgsi_opcode_type type,
	unsigned swizzle)
{
	struct lp_build_tgsi_soa_context *bld = lp_soa_context(bld_base);
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	if (reg->Register.Indirect) {
		LLVMValueRef array_index = emit_array_index(bld, reg, swizzle);
		LLVMValueRef ptr = LLVMBuildGEP(builder, bld->temps_array, &array_index,
						1, "");
	return LLVMBuildLoad(builder, ptr, "");
	} else {
		LLVMValueRef temp_ptr;
		temp_ptr = lp_get_temp_ptr_soa(bld, reg->Register.Index, swizzle);
		return bitcast(bld_base,type,LLVMBuildLoad(builder, temp_ptr, ""));
	}
}

static LLVMValueRef
emit_fetch_output(
	struct lp_build_tgsi_context *bld_base,
	const struct tgsi_full_src_register *reg,
	enum tgsi_opcode_type type,
	unsigned swizzle)
{
	struct lp_build_tgsi_soa_context *bld = lp_soa_context(bld_base);
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	 if (reg->Register.Indirect) {
		LLVMValueRef array_index = emit_array_index(bld, reg, swizzle);
		LLVMValueRef ptr = LLVMBuildGEP(builder, bld->outputs_array, &array_index,
						1, "");
		return LLVMBuildLoad(builder, ptr, "");
	} else {
		LLVMValueRef temp_ptr;
		temp_ptr = lp_get_output_ptr(bld, reg->Register.Index, swizzle);
		return LLVMBuildLoad(builder, temp_ptr, "");
	 }
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

		value = bitcast(bld_base, TGSI_TYPE_FLOAT, value);

		LLVMBuildStore(builder, value, temp_ptr);
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

	LLVMValueRef coords[4];
	unsigned chan;
	for (chan = 0; chan < 4; chan++) {
		coords[chan] = lp_build_emit_fetch(bld_base, emit_data->inst, 0, chan);
	}

	emit_data->arg_count = 1;
	emit_data->args[0] = lp_build_gather_values(bld_base->base.gallivm,
						coords, 4);
	emit_data->dst_type = LLVMVectorType(bld_base->base.elem_type, 4);
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

void radeon_llvm_context_init(struct radeon_llvm_context * ctx)
{
	struct lp_type type;
	LLVMTypeRef main_fn_type;
	LLVMBasicBlockRef main_fn_body;

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

	/* Setup the module */
	main_fn_type = LLVMFunctionType(LLVMVoidTypeInContext(ctx->gallivm.context),
					 NULL, 0, 0);
	ctx->main_fn = LLVMAddFunction(ctx->gallivm.module, "main", main_fn_type);
	main_fn_body = LLVMAppendBasicBlockInContext(ctx->gallivm.context,
			ctx->main_fn, "main_body");
	 LLVMPositionBuilderAtEnd(ctx->gallivm.builder, main_fn_body);

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

	bld_base->emit_fetch_funcs[TGSI_FILE_IMMEDIATE] = emit_fetch_immediate;
	bld_base->emit_fetch_funcs[TGSI_FILE_INPUT] = emit_fetch_input;
	bld_base->emit_fetch_funcs[TGSI_FILE_TEMPORARY] = emit_fetch_temporary;
	bld_base->emit_fetch_funcs[TGSI_FILE_OUTPUT] = emit_fetch_output;

	/* Allocate outputs */
	ctx->soa.outputs = ctx->outputs;

	/* XXX: Is there a better way to initialize all this ? */

	lp_set_default_actions(bld_base);

	bld_base->op_actions[TGSI_OPCODE_ABS].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_ABS].intr_name = "llvm.AMDIL.fabs.";
	bld_base->op_actions[TGSI_OPCODE_ARL].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_ARL].intr_name = "llvm.AMDGPU.arl";
	bld_base->op_actions[TGSI_OPCODE_BGNLOOP].emit = bgnloop_emit;
	bld_base->op_actions[TGSI_OPCODE_BRK].emit = brk_emit;
	bld_base->op_actions[TGSI_OPCODE_CONT].emit = cont_emit;
	bld_base->op_actions[TGSI_OPCODE_CLAMP].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_CLAMP].intr_name = "llvm.AMDIL.clamp.";
	bld_base->op_actions[TGSI_OPCODE_CMP].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_CMP].intr_name = "llvm.AMDGPU.cndlt";
	bld_base->op_actions[TGSI_OPCODE_COS].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_COS].intr_name = "llvm.AMDGPU.cos";
	bld_base->op_actions[TGSI_OPCODE_DDX].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_DDX].intr_name = "llvm.AMDGPU.ddx";
	bld_base->op_actions[TGSI_OPCODE_DDY].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_DDY].intr_name = "llvm.AMDGPU.ddy";
	bld_base->op_actions[TGSI_OPCODE_DIV].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_DIV].intr_name = "llvm.AMDGPU.div";
	bld_base->op_actions[TGSI_OPCODE_ELSE].emit = else_emit;
	bld_base->op_actions[TGSI_OPCODE_ENDIF].emit = endif_emit;
	bld_base->op_actions[TGSI_OPCODE_ENDLOOP].emit = endloop_emit;
	bld_base->op_actions[TGSI_OPCODE_EX2].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_EX2].intr_name = "llvm.AMDIL.exp.";
	bld_base->op_actions[TGSI_OPCODE_FLR].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_FLR].intr_name = "llvm.AMDGPU.floor";
	bld_base->op_actions[TGSI_OPCODE_FRC].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_FRC].intr_name = "llvm.AMDIL.fraction.";
	bld_base->op_actions[TGSI_OPCODE_IF].emit = if_emit;
	bld_base->op_actions[TGSI_OPCODE_KIL].emit = kil_emit;
	bld_base->op_actions[TGSI_OPCODE_KIL].intr_name = "llvm.AMDGPU.kill";
	bld_base->op_actions[TGSI_OPCODE_KILP].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_KILP].intr_name = "llvm.AMDGPU.kilp";
	bld_base->op_actions[TGSI_OPCODE_LG2].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_LG2].intr_name = "llvm.AMDIL.log.";
	bld_base->op_actions[TGSI_OPCODE_LRP].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_LRP].intr_name = "llvm.AMDGPU.lrp";
	bld_base->op_actions[TGSI_OPCODE_MIN].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_MIN].intr_name = "llvm.AMDIL.min.";
	bld_base->op_actions[TGSI_OPCODE_MAD].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_MAD].intr_name = "llvm.AMDIL.mad.";
	bld_base->op_actions[TGSI_OPCODE_MAX].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_MAX].intr_name = "llvm.AMDIL.max.";
	bld_base->op_actions[TGSI_OPCODE_MUL].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_MUL].intr_name = "llvm.AMDGPU.mul";
	bld_base->op_actions[TGSI_OPCODE_POW].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_POW].intr_name = "llvm.AMDGPU.pow";
	bld_base->op_actions[TGSI_OPCODE_RCP].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_RCP].intr_name = "llvm.AMDGPU.rcp";
	bld_base->op_actions[TGSI_OPCODE_SSG].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_SSG].intr_name = "llvm.AMDGPU.ssg";
	bld_base->op_actions[TGSI_OPCODE_SGE].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_SGE].intr_name = "llvm.AMDGPU.sge.";
	bld_base->op_actions[TGSI_OPCODE_SEQ].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_SEQ].intr_name = "llvm.AMDGPU.seq";
	bld_base->op_actions[TGSI_OPCODE_SLE].fetch_args = radeon_llvm_fetch_args_2_reverse_soa;
	bld_base->op_actions[TGSI_OPCODE_SLE].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_SLE].intr_name = "llvm.AMDGPU.sge";
	bld_base->op_actions[TGSI_OPCODE_SLT].fetch_args = radeon_llvm_fetch_args_2_reverse_soa;
	bld_base->op_actions[TGSI_OPCODE_SLT].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_SLT].intr_name = "llvm.AMDGPU.sgt";
	bld_base->op_actions[TGSI_OPCODE_SNE].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_SNE].intr_name = "llvm.AMDGPU.sne";
	bld_base->op_actions[TGSI_OPCODE_SGT].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_SGT].intr_name = "llvm.AMDGPU.sgt";
	bld_base->op_actions[TGSI_OPCODE_SIN].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_SIN].intr_name = "llvm.AMDGPU.sin";
	bld_base->op_actions[TGSI_OPCODE_TEX].fetch_args = tex_fetch_args;
	bld_base->op_actions[TGSI_OPCODE_TEX].intr_name = "llvm.AMDGPU.tex";
	bld_base->op_actions[TGSI_OPCODE_TXB].fetch_args = tex_fetch_args;
	bld_base->op_actions[TGSI_OPCODE_TXB].intr_name = "llvm.AMDGPU.txb";
	bld_base->op_actions[TGSI_OPCODE_TXD].fetch_args = tex_fetch_args;
	bld_base->op_actions[TGSI_OPCODE_TXD].intr_name = "llvm.AMDGPU.txd";
	bld_base->op_actions[TGSI_OPCODE_TXL].fetch_args = tex_fetch_args;
	bld_base->op_actions[TGSI_OPCODE_TXL].intr_name = "llvm.AMDGPU.txl";
	bld_base->op_actions[TGSI_OPCODE_TXP].intr_name = "llvm.AMDGPU.tex";
	bld_base->op_actions[TGSI_OPCODE_TRUNC].emit = lp_build_tgsi_intrinsic;
	bld_base->op_actions[TGSI_OPCODE_TRUNC].intr_name = "llvm.AMDGPU.trunc";

	bld_base->rsq_action.emit = lp_build_tgsi_intrinsic;
	bld_base->rsq_action.intr_name = "llvm.AMDGPU.rsq";
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
