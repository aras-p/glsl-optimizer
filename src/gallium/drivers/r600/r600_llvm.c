#include "r600_llvm.h"

#include "gallivm/lp_bld_const.h"
#include "gallivm/lp_bld_intr.h"
#include "gallivm/lp_bld_gather.h"
#include "tgsi/tgsi_parse.h"
#include "util/u_double_list.h"
#include "util/u_memory.h"

#include "r600.h"
#include "r600_asm.h"
#include "r600_sq.h"
#include "r600_opcodes.h"
#include "r600_shader.h"
#include "r600_pipe.h"
#include "radeon_llvm.h"
#include "radeon_llvm_emit.h"

#include <stdio.h>

#if defined R600_USE_LLVM || defined HAVE_OPENCL

#define CONSTANT_BUFFER_0_ADDR_SPACE 9
#define CONSTANT_BUFFER_1_ADDR_SPACE (CONSTANT_BUFFER_0_ADDR_SPACE + R600_UCP_CONST_BUFFER)

static LLVMValueRef llvm_fetch_const(
	struct lp_build_tgsi_context * bld_base,
	const struct tgsi_full_src_register *reg,
	enum tgsi_opcode_type type,
	unsigned swizzle)
{
	LLVMValueRef offset[2] = {
		LLVMConstInt(LLVMInt64TypeInContext(bld_base->base.gallivm->context), 0, false),
		lp_build_const_int32(bld_base->base.gallivm, reg->Register.Index)
	};
	if (reg->Register.Indirect) {
		struct lp_build_tgsi_soa_context *bld = lp_soa_context(bld_base);
		LLVMValueRef index = LLVMBuildLoad(bld_base->base.gallivm->builder, bld->addr[reg->Indirect.Index][reg->Indirect.SwizzleX], "");
		offset[1] = LLVMBuildAdd(bld_base->base.gallivm->builder, offset[1], index, "");
	}
	LLVMTypeRef const_ptr_type = LLVMPointerType(LLVMArrayType(LLVMVectorType(bld_base->base.elem_type, 4), 1024),
							CONSTANT_BUFFER_0_ADDR_SPACE);
	LLVMValueRef const_ptr = LLVMBuildIntToPtr(bld_base->base.gallivm->builder, lp_build_const_int32(bld_base->base.gallivm, 0), const_ptr_type, "");
	LLVMValueRef ptr = LLVMBuildGEP(bld_base->base.gallivm->builder, const_ptr, offset, 2, "");
	LLVMValueRef cvecval = LLVMBuildLoad(bld_base->base.gallivm->builder, ptr, "");
	LLVMValueRef cval = LLVMBuildExtractElement(bld_base->base.gallivm->builder, cvecval, lp_build_const_int32(bld_base->base.gallivm, swizzle), "");
	return bitcast(bld_base, type, cval);
}

static void llvm_load_system_value(
		struct radeon_llvm_context * ctx,
		unsigned index,
		const struct tgsi_full_declaration *decl)
{
	unsigned chan;

	switch (decl->Semantic.Name) {
	case TGSI_SEMANTIC_INSTANCEID: chan = 3; break;
	case TGSI_SEMANTIC_VERTEXID: chan = 0; break;
	default: assert(!"unknown system value");
	}

	LLVMValueRef reg = lp_build_const_int32(
			ctx->soa.bld_base.base.gallivm, chan);
	ctx->system_values[index] = build_intrinsic(
			ctx->soa.bld_base.base.gallivm->builder,
			"llvm.R600.load.input",
			ctx->soa.bld_base.base.elem_type, &reg, 1,
			LLVMReadNoneAttribute);
}

static LLVMValueRef llvm_fetch_system_value(
		struct lp_build_tgsi_context * bld_base,
		const struct tgsi_full_src_register *reg,
		enum tgsi_opcode_type type,
		unsigned swizzle)
{
	struct radeon_llvm_context * ctx = radeon_llvm_context(bld_base);
	LLVMValueRef cval = ctx->system_values[reg->Register.Index];
	return bitcast(bld_base, type, cval);
}

static LLVMValueRef
llvm_load_input_helper(
	struct radeon_llvm_context * ctx,
	unsigned idx, int interp, int ij_index)
{
	const struct lp_build_context * bb = &ctx->soa.bld_base.base;
	LLVMValueRef arg[2];
	int arg_count;
	const char * intrinsic;

	arg[0] = lp_build_const_int32(bb->gallivm, idx);

	if (interp) {
		intrinsic = "llvm.R600.interp.input";
		arg[1] = lp_build_const_int32(bb->gallivm, ij_index);
		arg_count = 2;
	} else {
		intrinsic = "llvm.R600.load.input";
		arg_count = 1;
	}

	return build_intrinsic(bb->gallivm->builder, intrinsic,
		bb->elem_type, &arg[0], arg_count, LLVMReadNoneAttribute);
}

static LLVMValueRef
llvm_face_select_helper(
	struct radeon_llvm_context * ctx,
	unsigned face_loc, LLVMValueRef front_color, LLVMValueRef back_color)
{
	const struct lp_build_context * bb = &ctx->soa.bld_base.base;
	LLVMValueRef face = llvm_load_input_helper(ctx, face_loc, 0, 0);
	LLVMValueRef is_front = LLVMBuildFCmp(
		bb->gallivm->builder, LLVMRealUGT, face,
		lp_build_const_float(bb->gallivm, 0.0f),	"");
	return LLVMBuildSelect(bb->gallivm->builder, is_front,
		front_color, back_color, "");
}

static void llvm_load_input(
	struct radeon_llvm_context * ctx,
	unsigned input_index,
	const struct tgsi_full_declaration *decl)
{
	const struct r600_shader_io * input = &ctx->r600_inputs[input_index];
	unsigned chan;
	unsigned interp = 0;
	int ij_index;
	int two_side = (ctx->two_side && input->name == TGSI_SEMANTIC_COLOR);
	LLVMValueRef v;

	if (ctx->chip_class >= EVERGREEN && ctx->type == TGSI_PROCESSOR_FRAGMENT &&
			input->spi_sid) {
		interp = 1;
		ij_index = (input->interpolate > 0) ? input->ij_index : -1;
	}

	for (chan = 0; chan < 4; chan++) {
		unsigned soa_index = radeon_llvm_reg_index_soa(input_index, chan);
		int loc;

		if (interp) {
			loc = 4 * input->lds_pos + chan;
		} else {
			if (input->name == TGSI_SEMANTIC_FACE)
				loc = 4 * ctx->face_gpr;
			else
				loc = 4 * input->gpr + chan;
		}

		v = llvm_load_input_helper(ctx, loc, interp, ij_index);

		if (two_side) {
			struct r600_shader_io * back_input =
					&ctx->r600_inputs[input->back_color_input];
			int back_loc = interp ? back_input->lds_pos : back_input->gpr;
			LLVMValueRef v2;

			back_loc = 4 * back_loc + chan;
			v2 = llvm_load_input_helper(ctx, back_loc, interp, ij_index);
			v = llvm_face_select_helper(ctx, 4 * ctx->face_gpr, v, v2);
		} else if (input->name == TGSI_SEMANTIC_POSITION &&
				ctx->type == TGSI_PROCESSOR_FRAGMENT && chan == 3) {
			/* RCP for fragcoord.w */
			v = LLVMBuildFDiv(ctx->gallivm.builder,
					lp_build_const_float(&(ctx->gallivm), 1.0f),
					v, "");
		}

		ctx->inputs[soa_index] = v;
	}
}

static void llvm_emit_prologue(struct lp_build_tgsi_context * bld_base)
{
	struct radeon_llvm_context * ctx = radeon_llvm_context(bld_base);

}

static void llvm_emit_epilogue(struct lp_build_tgsi_context * bld_base)
{
	struct radeon_llvm_context * ctx = radeon_llvm_context(bld_base);
	struct lp_build_context * base = &bld_base->base;
	struct pipe_stream_output_info * so = ctx->stream_outputs;
	unsigned i;
	unsigned next_pos = 60;
	unsigned next_param = 0;

	unsigned color_count = 0;
	boolean has_color = false;

	if (ctx->type == TGSI_PROCESSOR_VERTEX && so->num_outputs) {
		for (i = 0; i < so->num_outputs; i++) {
			unsigned register_index = so->output[i].register_index;
			unsigned start_component = so->output[i].start_component;
			unsigned num_components = so->output[i].num_components;
			unsigned dst_offset = so->output[i].dst_offset;
			unsigned chan;
			LLVMValueRef elements[4];
			if (dst_offset < start_component) {
				for (chan = 0; chan < TGSI_NUM_CHANNELS; chan++) {
					elements[chan] = LLVMBuildLoad(base->gallivm->builder,
						ctx->soa.outputs[register_index][(chan + start_component) % TGSI_NUM_CHANNELS], "");
				}
				start_component = 0;
			} else {
				for (chan = 0; chan < TGSI_NUM_CHANNELS; chan++) {
					elements[chan] = LLVMBuildLoad(base->gallivm->builder,
						ctx->soa.outputs[register_index][chan], "");
				}
			}
			LLVMValueRef output = lp_build_gather_values(base->gallivm, elements, 4);
			LLVMValueRef args[4];
			args[0] = output;
			args[1] = lp_build_const_int32(base->gallivm, dst_offset - start_component);
			args[2] = lp_build_const_int32(base->gallivm, so->output[i].output_buffer);
			args[3] = lp_build_const_int32(base->gallivm, ((1 << num_components) - 1) << start_component);
			lp_build_intrinsic(base->gallivm->builder, "llvm.R600.store.stream.output",
				LLVMVoidTypeInContext(base->gallivm->context), args, 4);
		}
	}

	/* Add the necessary export instructions */
	for (i = 0; i < ctx->output_reg_count; i++) {
		unsigned chan;
		LLVMValueRef elements[4];
		for (chan = 0; chan < TGSI_NUM_CHANNELS; chan++) {
			elements[chan] = LLVMBuildLoad(base->gallivm->builder,
				ctx->soa.outputs[i][chan], "");
		}
		LLVMValueRef output = lp_build_gather_values(base->gallivm, elements, 4);

		if (ctx->type == TGSI_PROCESSOR_VERTEX) {
			switch (ctx->r600_outputs[i].name) {
			case TGSI_SEMANTIC_POSITION:
			case TGSI_SEMANTIC_PSIZE: {
				LLVMValueRef args[3];
				args[0] = output;
				args[1] = lp_build_const_int32(base->gallivm, next_pos++);
				args[2] = lp_build_const_int32(base->gallivm, V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_POS);
				build_intrinsic(
					base->gallivm->builder,
					"llvm.R600.store.swizzle",
					LLVMVoidTypeInContext(base->gallivm->context),
					args, 3, 0);
				break;
			}
			case TGSI_SEMANTIC_CLIPVERTEX: {
				LLVMValueRef args[3];
				unsigned reg_index;
				unsigned base_vector_chan;
				LLVMValueRef adjusted_elements[4];
				for (reg_index = 0; reg_index < 2; reg_index ++) {
					for (chan = 0; chan < TGSI_NUM_CHANNELS; chan++) {
						LLVMValueRef offset[2] = {
							LLVMConstInt(LLVMInt64TypeInContext(bld_base->base.gallivm->context), 0, false),
							lp_build_const_int32(bld_base->base.gallivm, reg_index * 4 + chan)
						};
						LLVMTypeRef const_ptr_type = LLVMPointerType(LLVMArrayType(LLVMVectorType(bld_base->base.elem_type, 4), 1024), CONSTANT_BUFFER_1_ADDR_SPACE);
						LLVMValueRef const_ptr = LLVMBuildIntToPtr(bld_base->base.gallivm->builder, lp_build_const_int32(bld_base->base.gallivm, 0), const_ptr_type, "");
						LLVMValueRef ptr = LLVMBuildGEP(bld_base->base.gallivm->builder, const_ptr, offset, 2, "");
						LLVMValueRef base_vector = LLVMBuildLoad(bld_base->base.gallivm->builder, ptr, "");
						args[0] = output;
						args[1] = base_vector;
						adjusted_elements[chan] = build_intrinsic(base->gallivm->builder,
							"llvm.AMDGPU.dp4", bld_base->base.elem_type,
							args, 2, LLVMReadNoneAttribute);
					}
					args[0] = lp_build_gather_values(base->gallivm,
						adjusted_elements, 4);
					args[1] = lp_build_const_int32(base->gallivm, next_pos++);
					args[2] = lp_build_const_int32(base->gallivm, V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_POS);
					build_intrinsic(
						base->gallivm->builder,
						"llvm.R600.store.swizzle",
						LLVMVoidTypeInContext(base->gallivm->context),
						args, 3, 0);
				}
				break;
			}
			case TGSI_SEMANTIC_CLIPDIST : {
				LLVMValueRef args[3];
				args[0] = output;
				args[1] = lp_build_const_int32(base->gallivm, next_pos++);
				args[2] = lp_build_const_int32(base->gallivm, V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_POS);
				build_intrinsic(
					base->gallivm->builder,
					"llvm.R600.store.swizzle",
					LLVMVoidTypeInContext(base->gallivm->context),
					args, 3, 0);
				args[1] = lp_build_const_int32(base->gallivm, next_param++);
				args[2] = lp_build_const_int32(base->gallivm, V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PARAM);
				build_intrinsic(
					base->gallivm->builder,
					"llvm.R600.store.swizzle",
					LLVMVoidTypeInContext(base->gallivm->context),
					args, 3, 0);
				break;
			}
			case TGSI_SEMANTIC_FOG: {
				elements[0] = LLVMBuildLoad(base->gallivm->builder,
					ctx->soa.outputs[i][0], "");
				elements[1] = elements[2] = lp_build_const_float(base->gallivm, 0.0f);
				elements[3] = lp_build_const_float(base->gallivm, 1.0f);

				LLVMValueRef args[3];
				args[0] = lp_build_gather_values(base->gallivm, elements, 4);
				args[1] = lp_build_const_int32(base->gallivm, next_param++);
				args[2] = lp_build_const_int32(base->gallivm, V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PARAM);
				build_intrinsic(
					base->gallivm->builder,
					"llvm.R600.store.swizzle",
					LLVMVoidTypeInContext(base->gallivm->context),
					args, 3, 0);
				break;
			}
			default: {
				LLVMValueRef args[3];
				args[0] = output;
				args[1] = lp_build_const_int32(base->gallivm, next_param++);
				args[2] = lp_build_const_int32(base->gallivm, V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PARAM);
				build_intrinsic(
					base->gallivm->builder,
					"llvm.R600.store.swizzle",
					LLVMVoidTypeInContext(base->gallivm->context),
					args, 3, 0);
				break;
			}
			}
		} else if (ctx->type == TGSI_PROCESSOR_FRAGMENT) {
			switch (ctx->r600_outputs[i].name) {
			case TGSI_SEMANTIC_COLOR:
				has_color = true;
				if ( color_count < ctx->color_buffer_count) {
					LLVMValueRef args[3];
					args[0] = output;
					if (ctx->fs_color_all) {
						for (unsigned j = 0; j < ctx->color_buffer_count; j++) {
							args[1] = lp_build_const_int32(base->gallivm, j);
							args[2] = lp_build_const_int32(base->gallivm, V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PIXEL);
							build_intrinsic(
								base->gallivm->builder,
								"llvm.R600.store.swizzle",
								LLVMVoidTypeInContext(base->gallivm->context),
								args, 3, 0);
						}
					} else {
						args[1] = lp_build_const_int32(base->gallivm, color_count++);
						args[2] = lp_build_const_int32(base->gallivm, V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PIXEL);
						build_intrinsic(
							base->gallivm->builder,
							"llvm.R600.store.swizzle",
							LLVMVoidTypeInContext(base->gallivm->context),
							args, 3, 0);
					}
				}
				break;
			case TGSI_SEMANTIC_POSITION:
				lp_build_intrinsic_unary(
					base->gallivm->builder,
					"llvm.R600.store.pixel.depth",
					LLVMVoidTypeInContext(base->gallivm->context),
					LLVMBuildLoad(base->gallivm->builder, ctx->soa.outputs[i][2], ""));
				break;
			case TGSI_SEMANTIC_STENCIL:
				lp_build_intrinsic_unary(
					base->gallivm->builder,
					"llvm.R600.store.pixel.stencil",
					LLVMVoidTypeInContext(base->gallivm->context),
					LLVMBuildLoad(base->gallivm->builder, ctx->soa.outputs[i][1], ""));
				break;
			}
		}
	}
	// Add dummy exports
	if (ctx->type == TGSI_PROCESSOR_VERTEX) {
		if (!next_param) {
			lp_build_intrinsic_unary(base->gallivm->builder, "llvm.R600.store.dummy",
				LLVMVoidTypeInContext(base->gallivm->context),
				lp_build_const_int32(base->gallivm, V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PARAM));
		}
		if (!(next_pos-60)) {
			lp_build_intrinsic_unary(base->gallivm->builder, "llvm.R600.store.dummy",
				LLVMVoidTypeInContext(base->gallivm->context),
				lp_build_const_int32(base->gallivm, V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_POS));
		}
	}
	if (ctx->type == TGSI_PROCESSOR_FRAGMENT) {
		if (!has_color) {
			lp_build_intrinsic_unary(base->gallivm->builder, "llvm.R600.store.dummy",
				LLVMVoidTypeInContext(base->gallivm->context),
				lp_build_const_int32(base->gallivm, V_SQ_CF_ALLOC_EXPORT_WORD0_SQ_EXPORT_PIXEL));
		}
	}

}

static void llvm_emit_tex(
	const struct lp_build_tgsi_action * action,
	struct lp_build_tgsi_context * bld_base,
	struct lp_build_emit_data * emit_data)
{
	struct gallivm_state * gallivm = bld_base->base.gallivm;
	LLVMValueRef args[6];
	unsigned c, sampler_src;

	assert(emit_data->arg_count + 2 <= Elements(args));

	for (c = 0; c < emit_data->arg_count; ++c)
		args[c] = emit_data->args[c];

	sampler_src = emit_data->inst->Instruction.NumSrcRegs-1;

	args[c++] = lp_build_const_int32(gallivm,
					emit_data->inst->Src[sampler_src].Register.Index + R600_MAX_CONST_BUFFERS);
	args[c++] = lp_build_const_int32(gallivm,
					emit_data->inst->Src[sampler_src].Register.Index);
	args[c++] = lp_build_const_int32(gallivm,
					emit_data->inst->Texture.Texture);

	emit_data->output[0] = build_intrinsic(gallivm->builder,
					action->intr_name,
					emit_data->dst_type, args, c, LLVMReadNoneAttribute);
}

static void emit_cndlt(
		const struct lp_build_tgsi_action * action,
		struct lp_build_tgsi_context * bld_base,
		struct lp_build_emit_data * emit_data)
{
	LLVMBuilderRef builder = bld_base->base.gallivm->builder;
	LLVMValueRef float_zero = lp_build_const_float(
		bld_base->base.gallivm, 0.0f);
	LLVMValueRef cmp = LLVMBuildFCmp(
		builder, LLVMRealULT, emit_data->args[0], float_zero, "");
	emit_data->output[emit_data->chan] = LLVMBuildSelect(builder,
		cmp, emit_data->args[1], emit_data->args[2], "");
}

static void dp_fetch_args(
	struct lp_build_tgsi_context * bld_base,
	struct lp_build_emit_data * emit_data)
{
	struct lp_build_context * base = &bld_base->base;
	unsigned chan;
	LLVMValueRef elements[2][4];
	unsigned opcode = emit_data->inst->Instruction.Opcode;
	unsigned dp_components = (opcode == TGSI_OPCODE_DP2 ? 2 :
					(opcode == TGSI_OPCODE_DP3 ? 3 : 4));
	for (chan = 0 ; chan < dp_components; chan++) {
		elements[0][chan] = lp_build_emit_fetch(bld_base,
						emit_data->inst, 0, chan);
		elements[1][chan] = lp_build_emit_fetch(bld_base,
						emit_data->inst, 1, chan);
	}

	for ( ; chan < 4; chan++) {
		elements[0][chan] = base->zero;
		elements[1][chan] = base->zero;
	}

	 /* Fix up for DPH */
	if (opcode == TGSI_OPCODE_DPH) {
		elements[0][TGSI_CHAN_W] = base->one;
	}

	emit_data->args[0] = lp_build_gather_values(bld_base->base.gallivm,
							elements[0], 4);
	emit_data->args[1] = lp_build_gather_values(bld_base->base.gallivm,
							elements[1], 4);
	emit_data->arg_count = 2;

	emit_data->dst_type = base->elem_type;
}

static struct lp_build_tgsi_action dot_action = {
	.fetch_args = dp_fetch_args,
	.emit = build_tgsi_intrinsic_nomem,
	.intr_name = "llvm.AMDGPU.dp4"
};



LLVMModuleRef r600_tgsi_llvm(
	struct radeon_llvm_context * ctx,
	const struct tgsi_token * tokens)
{
	struct tgsi_shader_info shader_info;
	struct lp_build_tgsi_context * bld_base = &ctx->soa.bld_base;
	radeon_llvm_context_init(ctx);
	tgsi_scan_shader(tokens, &shader_info);

	bld_base->info = &shader_info;
	bld_base->userdata = ctx;
	bld_base->emit_fetch_funcs[TGSI_FILE_CONSTANT] = llvm_fetch_const;
	bld_base->emit_fetch_funcs[TGSI_FILE_SYSTEM_VALUE] = llvm_fetch_system_value;
	bld_base->emit_prologue = llvm_emit_prologue;
	bld_base->emit_epilogue = llvm_emit_epilogue;
	ctx->userdata = ctx;
	ctx->load_input = llvm_load_input;
	ctx->load_system_value = llvm_load_system_value;

	bld_base->op_actions[TGSI_OPCODE_DP2] = dot_action;
	bld_base->op_actions[TGSI_OPCODE_DP3] = dot_action;
	bld_base->op_actions[TGSI_OPCODE_DP4] = dot_action;
	bld_base->op_actions[TGSI_OPCODE_DPH] = dot_action;
	bld_base->op_actions[TGSI_OPCODE_DDX].emit = llvm_emit_tex;
	bld_base->op_actions[TGSI_OPCODE_DDY].emit = llvm_emit_tex;
	bld_base->op_actions[TGSI_OPCODE_TEX].emit = llvm_emit_tex;
	bld_base->op_actions[TGSI_OPCODE_TEX2].emit = llvm_emit_tex;
	bld_base->op_actions[TGSI_OPCODE_TXB].emit = llvm_emit_tex;
	bld_base->op_actions[TGSI_OPCODE_TXB2].emit = llvm_emit_tex;
	bld_base->op_actions[TGSI_OPCODE_TXD].emit = llvm_emit_tex;
	bld_base->op_actions[TGSI_OPCODE_TXL].emit = llvm_emit_tex;
	bld_base->op_actions[TGSI_OPCODE_TXL2].emit = llvm_emit_tex;
	bld_base->op_actions[TGSI_OPCODE_TXF].emit = llvm_emit_tex;
	bld_base->op_actions[TGSI_OPCODE_TXQ].emit = llvm_emit_tex;
	bld_base->op_actions[TGSI_OPCODE_TXP].emit = llvm_emit_tex;
	bld_base->op_actions[TGSI_OPCODE_CMP].emit = emit_cndlt;

	lp_build_tgsi_llvm(bld_base, tokens);

	radeon_llvm_finalize_module(ctx);

	return ctx->gallivm.module;
}

const char * r600_llvm_gpu_string(enum radeon_family family)
{
	const char * gpu_family;

	switch (family) {
	case CHIP_R600:
	case CHIP_RV610:
	case CHIP_RV630:
	case CHIP_RV620:
	case CHIP_RV635:
	case CHIP_RS780:
	case CHIP_RS880:
		gpu_family = "r600";
		break;
	case CHIP_RV710:
		gpu_family = "rv710";
		break;
	case CHIP_RV730:
		gpu_family = "rv730";
		break;
	case CHIP_RV670:
	case CHIP_RV740:
	case CHIP_RV770:
		gpu_family = "rv770";
		break;
	case CHIP_PALM:
	case CHIP_CEDAR:
		gpu_family = "cedar";
		break;
	case CHIP_SUMO:
	case CHIP_SUMO2:
	case CHIP_REDWOOD:
		gpu_family = "redwood";
		break;
	case CHIP_JUNIPER:
		gpu_family = "juniper";
		break;
	case CHIP_HEMLOCK:
	case CHIP_CYPRESS:
		gpu_family = "cypress";
		break;
	case CHIP_BARTS:
		gpu_family = "barts";
		break;
	case CHIP_TURKS:
		gpu_family = "turks";
		break;
	case CHIP_CAICOS:
		gpu_family = "caicos";
		break;
	case CHIP_CAYMAN:
        case CHIP_ARUBA:
		gpu_family = "cayman";
		break;
	default:
		gpu_family = "";
		fprintf(stderr, "Chip not supported by r600 llvm "
			"backend, please file a bug at bugs.freedesktop.org\n");
		break;
	}
	return gpu_family;
}

unsigned r600_llvm_compile(
	LLVMModuleRef mod,
	unsigned char ** inst_bytes,
	unsigned * inst_byte_count,
	enum radeon_family family,
	unsigned dump)
{
	const char * gpu_family = r600_llvm_gpu_string(family);
	return radeon_llvm_compile(mod, inst_bytes, inst_byte_count,
							gpu_family, dump);
}

#endif
