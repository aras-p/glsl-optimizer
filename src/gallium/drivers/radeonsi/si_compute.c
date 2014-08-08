/*
 * Copyright 2013 Advanced Micro Devices, Inc.
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
 */

#include "util/u_memory.h"

#include "../radeon/r600_cs.h"
#include "si_pipe.h"
#include "si_shader.h"
#include "sid.h"

#include "radeon_llvm_util.h"

#define MAX_GLOBAL_BUFFERS 20
#if HAVE_LLVM < 0x0305
#define NUM_USER_SGPRS 2
#else
#define NUM_USER_SGPRS 4
#endif

struct si_pipe_compute {
	struct si_context *ctx;

	unsigned local_size;
	unsigned private_size;
	unsigned input_size;
	unsigned num_kernels;
	struct si_pipe_shader *kernels;
	unsigned num_user_sgprs;

	struct r600_resource *input_buffer;
	struct pipe_resource *global_buffers[MAX_GLOBAL_BUFFERS];

	LLVMContextRef llvm_ctx;
};

static void *si_create_compute_state(
	struct pipe_context *ctx,
	const struct pipe_compute_state *cso)
{
	struct si_context *sctx = (struct si_context *)ctx;
	struct si_pipe_compute *program =
					CALLOC_STRUCT(si_pipe_compute);
	const struct pipe_llvm_program_header *header;
	const unsigned char *code;
	unsigned i;

	program->llvm_ctx = LLVMContextCreate();

	header = cso->prog;
	code = cso->prog + sizeof(struct pipe_llvm_program_header);

	program->ctx = sctx;
	program->local_size = cso->req_local_mem;
	program->private_size = cso->req_private_mem;
	program->input_size = cso->req_input_mem;

	program->num_kernels = radeon_llvm_get_num_kernels(program->llvm_ctx, code,
							header->num_bytes);
	program->kernels = CALLOC(sizeof(struct si_pipe_shader),
							program->num_kernels);
	for (i = 0; i < program->num_kernels; i++) {
		LLVMModuleRef mod = radeon_llvm_get_kernel_module(program->llvm_ctx, i,
							code, header->num_bytes);
		si_compile_llvm(sctx, &program->kernels[i], mod);
		LLVMDisposeModule(mod);
	}

	program->input_buffer =	si_resource_create_custom(sctx->b.b.screen,
		PIPE_USAGE_IMMUTABLE, program->input_size);

	return program;
}

static void si_bind_compute_state(struct pipe_context *ctx, void *state)
{
	struct si_context *sctx = (struct si_context*)ctx;
	sctx->cs_shader_state.program = (struct si_pipe_compute*)state;
}

static void si_set_global_binding(
	struct pipe_context *ctx, unsigned first, unsigned n,
	struct pipe_resource **resources,
	uint32_t **handles)
{
	unsigned i;
	struct si_context *sctx = (struct si_context*)ctx;
	struct si_pipe_compute *program = sctx->cs_shader_state.program;

	if (!resources) {
		for (i = first; i < first + n; i++) {
			pipe_resource_reference(&program->global_buffers[i], NULL);
		}
		return;
	}

	for (i = first; i < first + n; i++) {
		uint64_t va;
		uint32_t offset;
		pipe_resource_reference(&program->global_buffers[i], resources[i]);
		va = r600_resource(resources[i])->gpu_address;
		offset = util_le32_to_cpu(*handles[i]);
		va += offset;
		va = util_cpu_to_le64(va);
		memcpy(handles[i], &va, sizeof(va));
	}
}

/**
 * This function computes the value for R_00B860_COMPUTE_TMPRING_SIZE.WAVES
 * /p block_layout is the number of threads in each work group.
 * /p grid layout is the number of work groups.
 */
static unsigned compute_num_waves_for_scratch(
		const struct radeon_info *info,
		const uint *block_layout,
		const uint *grid_layout)
{
	unsigned num_sh = MAX2(info->max_sh_per_se, 1);
	unsigned num_se = MAX2(info->max_se, 1);
	unsigned num_blocks = 1;
	unsigned threads_per_block = 1;
	unsigned waves_per_block;
	unsigned waves_per_sh;
	unsigned waves;
	unsigned scratch_waves;
	unsigned i;

	for (i = 0; i < 3; i++) {
		threads_per_block *= block_layout[i];
		num_blocks *= grid_layout[i];
	}

	waves_per_block = align(threads_per_block, 64) / 64;
	waves = waves_per_block * num_blocks;
	waves_per_sh = align(waves, num_sh * num_se) / (num_sh * num_se);
	scratch_waves = waves_per_sh * num_sh * num_se;

	if (waves_per_block > waves_per_sh) {
		scratch_waves = waves_per_block * num_sh * num_se;
	}

	return scratch_waves;
}

static void si_launch_grid(
		struct pipe_context *ctx,
		const uint *block_layout, const uint *grid_layout,
		uint32_t pc, const void *input)
{
	struct si_context *sctx = (struct si_context*)ctx;
	struct si_pipe_compute *program = sctx->cs_shader_state.program;
	struct si_pm4_state *pm4 = CALLOC_STRUCT(si_pm4_state);
	struct r600_resource *input_buffer = program->input_buffer;
	unsigned kernel_args_size;
	unsigned num_work_size_bytes = 36;
	uint32_t kernel_args_offset = 0;
	uint32_t *kernel_args;
	uint64_t kernel_args_va;
	uint64_t scratch_buffer_va = 0;
	uint64_t shader_va;
	unsigned arg_user_sgpr_count = NUM_USER_SGPRS;
	unsigned i;
	struct si_pipe_shader *shader = &program->kernels[pc];
	unsigned lds_blocks;
	unsigned num_waves_for_scratch;

	pm4->compute_pkt = true;
	si_cmd_context_control(pm4);

	si_pm4_cmd_begin(pm4, PKT3_EVENT_WRITE);
	si_pm4_cmd_add(pm4, EVENT_TYPE(EVENT_TYPE_CACHE_FLUSH) |
	                    EVENT_INDEX(0x7) |
			    EVENT_WRITE_INV_L2);
	si_pm4_cmd_end(pm4, false);

	si_pm4_inval_texture_cache(pm4);
	si_pm4_inval_shader_cache(pm4);
	si_cmd_surface_sync(pm4, pm4->cp_coher_cntl);

	/* Upload the kernel arguments */

	/* The extra num_work_size_bytes are for work group / work item size information */
	kernel_args_size = program->input_size + num_work_size_bytes + 8 /* For scratch va */;

	kernel_args = sctx->b.ws->buffer_map(input_buffer->cs_buf,
			sctx->b.rings.gfx.cs, PIPE_TRANSFER_WRITE);
	for (i = 0; i < 3; i++) {
		kernel_args[i] = grid_layout[i];
		kernel_args[i + 3] = grid_layout[i] * block_layout[i];
		kernel_args[i + 6] = block_layout[i];
	}

	num_waves_for_scratch =	compute_num_waves_for_scratch(
		&sctx->screen->b.info, block_layout, grid_layout);

	memcpy(kernel_args + (num_work_size_bytes / 4), input, program->input_size);

	if (shader->scratch_bytes_per_wave > 0) {
		unsigned scratch_bytes = shader->scratch_bytes_per_wave *
						num_waves_for_scratch;

		COMPUTE_DBG(sctx->screen, "Waves: %u; Scratch per wave: %u bytes; "
		            "Total Scratch: %u bytes\n", num_waves_for_scratch,
			    shader->scratch_bytes_per_wave, scratch_bytes);
		if (!shader->scratch_bo) {
			shader->scratch_bo = (struct r600_resource*)
				si_resource_create_custom(sctx->b.b.screen,
				PIPE_USAGE_DEFAULT, scratch_bytes);
		}
		scratch_buffer_va = shader->scratch_bo->gpu_address;
		si_pm4_add_bo(pm4, shader->scratch_bo,
				RADEON_USAGE_READWRITE,
				RADEON_PRIO_SHADER_RESOURCE_RW);

	}

	for (i = 0; i < (kernel_args_size / 4); i++) {
		COMPUTE_DBG(sctx->screen, "input %u : %u\n", i,
			kernel_args[i]);
	}

	sctx->b.ws->buffer_unmap(input_buffer->cs_buf);

	kernel_args_va = input_buffer->gpu_address;
	kernel_args_va += kernel_args_offset;

	si_pm4_add_bo(pm4, input_buffer, RADEON_USAGE_READ,
		RADEON_PRIO_SHADER_DATA);

	si_pm4_set_reg(pm4, R_00B900_COMPUTE_USER_DATA_0, kernel_args_va);
	si_pm4_set_reg(pm4, R_00B900_COMPUTE_USER_DATA_0 + 4, S_008F04_BASE_ADDRESS_HI (kernel_args_va >> 32) | S_008F04_STRIDE(0));
	si_pm4_set_reg(pm4, R_00B900_COMPUTE_USER_DATA_0 + 8, scratch_buffer_va);
	si_pm4_set_reg(pm4, R_00B900_COMPUTE_USER_DATA_0 + 12,
		S_008F04_BASE_ADDRESS_HI(scratch_buffer_va >> 32)
		|  S_008F04_STRIDE(shader->scratch_bytes_per_wave / 64));

	si_pm4_set_reg(pm4, R_00B810_COMPUTE_START_X, 0);
	si_pm4_set_reg(pm4, R_00B814_COMPUTE_START_Y, 0);
	si_pm4_set_reg(pm4, R_00B818_COMPUTE_START_Z, 0);

	si_pm4_set_reg(pm4, R_00B81C_COMPUTE_NUM_THREAD_X,
				S_00B81C_NUM_THREAD_FULL(block_layout[0]));
	si_pm4_set_reg(pm4, R_00B820_COMPUTE_NUM_THREAD_Y,
				S_00B820_NUM_THREAD_FULL(block_layout[1]));
	si_pm4_set_reg(pm4, R_00B824_COMPUTE_NUM_THREAD_Z,
				S_00B824_NUM_THREAD_FULL(block_layout[2]));

	/* Global buffers */
	for (i = 0; i < MAX_GLOBAL_BUFFERS; i++) {
		struct r600_resource *buffer =
				(struct r600_resource*)program->global_buffers[i];
		if (!buffer) {
			continue;
		}
		si_pm4_add_bo(pm4, buffer, RADEON_USAGE_READWRITE, RADEON_PRIO_SHADER_RESOURCE_RW);
	}

	/* This register has been moved to R_00CD20_COMPUTE_MAX_WAVE_ID
	 * and is now per pipe, so it should be handled in the
	 * kernel if we want to use something other than the default value,
	 * which is now 0x22f.
	 */
	if (sctx->b.chip_class <= SI) {
		/* XXX: This should be:
		 * (number of compute units) * 4 * (waves per simd) - 1 */

		si_pm4_set_reg(pm4, R_00B82C_COMPUTE_MAX_WAVE_ID,
						0x190 /* Default value */);
	}

	shader_va = shader->bo->gpu_address;
	si_pm4_add_bo(pm4, shader->bo, RADEON_USAGE_READ, RADEON_PRIO_SHADER_DATA);
	si_pm4_set_reg(pm4, R_00B830_COMPUTE_PGM_LO, (shader_va >> 8) & 0xffffffff);
	si_pm4_set_reg(pm4, R_00B834_COMPUTE_PGM_HI, shader_va >> 40);

	si_pm4_set_reg(pm4, R_00B848_COMPUTE_PGM_RSRC1,
		/* We always use at least 3 VGPRS, these come from
		 * TIDIG_COMP_CNT.
		 * XXX: The compiler should account for this.
		 */
		S_00B848_VGPRS((MAX2(3, shader->num_vgprs) - 1) / 4)
		/* We always use at least 4 + arg_user_sgpr_count.  The 4 extra
		 * sgprs are from TGID_X_EN, TGID_Y_EN, TGID_Z_EN, TG_SIZE_EN
		 * XXX: The compiler should account for this.
		 */
		|  S_00B848_SGPRS(((MAX2(4 + arg_user_sgpr_count,
		                        shader->num_sgprs)) - 1) / 8))
		;

	lds_blocks = shader->lds_size;
	/* XXX: We are over allocating LDS.  For SI, the shader reports LDS in
	 * blocks of 256 bytes, so if there are 4 bytes lds allocated in
	 * the shader and 4 bytes allocated by the state tracker, then
	 * we will set LDS_SIZE to 512 bytes rather than 256.
	 */
	if (sctx->b.chip_class <= SI) {
		lds_blocks += align(program->local_size, 256) >> 8;
	} else {
		lds_blocks += align(program->local_size, 512) >> 9;
	}

	assert(lds_blocks <= 0xFF);

	si_pm4_set_reg(pm4, R_00B84C_COMPUTE_PGM_RSRC2,
		S_00B84C_SCRATCH_EN(shader->scratch_bytes_per_wave > 0)
		| S_00B84C_USER_SGPR(arg_user_sgpr_count)
		| S_00B84C_TGID_X_EN(1)
		| S_00B84C_TGID_Y_EN(1)
		| S_00B84C_TGID_Z_EN(1)
		| S_00B84C_TG_SIZE_EN(1)
		| S_00B84C_TIDIG_COMP_CNT(2)
		| S_00B84C_LDS_SIZE(lds_blocks)
		| S_00B84C_EXCP_EN(0))
		;
	si_pm4_set_reg(pm4, R_00B854_COMPUTE_RESOURCE_LIMITS, 0);

	si_pm4_set_reg(pm4, R_00B858_COMPUTE_STATIC_THREAD_MGMT_SE0,
		S_00B858_SH0_CU_EN(0xffff /* Default value */)
		| S_00B858_SH1_CU_EN(0xffff /* Default value */))
		;

	si_pm4_set_reg(pm4, R_00B85C_COMPUTE_STATIC_THREAD_MGMT_SE1,
		S_00B85C_SH0_CU_EN(0xffff /* Default value */)
		| S_00B85C_SH1_CU_EN(0xffff /* Default value */))
		;

	si_pm4_set_reg(pm4, R_00B860_COMPUTE_TMPRING_SIZE,
		/* The maximum value for WAVES is 32 * num CU.
		 * If you program this value incorrectly, the GPU will hang if
		 * COMPUTE_PGM_RSRC2.SCRATCH_EN is enabled.
		 */
		S_00B860_WAVES(num_waves_for_scratch)
		| S_00B860_WAVESIZE(shader->scratch_bytes_per_wave >> 10))
		;

	si_pm4_cmd_begin(pm4, PKT3_DISPATCH_DIRECT);
	si_pm4_cmd_add(pm4, grid_layout[0]); /* Thread groups DIM_X */
	si_pm4_cmd_add(pm4, grid_layout[1]); /* Thread groups DIM_Y */
	si_pm4_cmd_add(pm4, grid_layout[2]); /* Thread gropus DIM_Z */
	si_pm4_cmd_add(pm4, 1); /* DISPATCH_INITIATOR */
        si_pm4_cmd_end(pm4, false);

	si_pm4_cmd_begin(pm4, PKT3_EVENT_WRITE);
	si_pm4_cmd_add(pm4, EVENT_TYPE(V_028A90_CS_PARTIAL_FLUSH | EVENT_INDEX(0x4)));
	si_pm4_cmd_end(pm4, false);

	si_pm4_inval_texture_cache(pm4);
	si_pm4_inval_shader_cache(pm4);
	si_cmd_surface_sync(pm4, pm4->cp_coher_cntl);

	si_pm4_emit(sctx, pm4);

#if 0
	fprintf(stderr, "cdw: %i\n", sctx->cs->cdw);
	for (i = 0; i < sctx->cs->cdw; i++) {
		fprintf(stderr, "%4i : 0x%08X\n", i, sctx->cs->buf[i]);
	}
#endif

	si_pm4_free_state(sctx, pm4, ~0);
}


static void si_delete_compute_state(struct pipe_context *ctx, void* state){
	struct si_pipe_compute *program = (struct si_pipe_compute *)state;

	if (!state) {
		return;
	}

	if (program->kernels) {
		for (int i = 0; i < program->num_kernels; i++){
			if (program->kernels[i].bo){
				si_pipe_shader_destroy(ctx, &program->kernels[i]);
			}
		}
		FREE(program->kernels);
	}

	if (program->llvm_ctx){
		LLVMContextDispose(program->llvm_ctx);
	}
	pipe_resource_reference(
		(struct pipe_resource **)&program->input_buffer, NULL);

	//And then free the program itself.
	FREE(program);
}

static void si_set_compute_resources(struct pipe_context * ctx_,
		unsigned start, unsigned count,
		struct pipe_surface ** surfaces) { }

void si_init_compute_functions(struct si_context *sctx)
{
	sctx->b.b.create_compute_state = si_create_compute_state;
	sctx->b.b.delete_compute_state = si_delete_compute_state;
	sctx->b.b.bind_compute_state = si_bind_compute_state;
/*	 ctx->context.create_sampler_view = evergreen_compute_create_sampler_view; */
	sctx->b.b.set_compute_resources = si_set_compute_resources;
	sctx->b.b.set_global_binding = si_set_global_binding;
	sctx->b.b.launch_grid = si_launch_grid;
}
