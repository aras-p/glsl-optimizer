#include "util/u_memory.h"

#include "radeonsi_pipe.h"
#include "radeonsi_shader.h"

#include "radeon_llvm_util.h"

struct si_pipe_compute {
	struct r600_context *ctx;

	unsigned local_size;
	unsigned private_size;
	unsigned input_size;
	struct si_pipe_shader shader;
	unsigned num_user_sgprs;

        struct si_pm4_state *pm4_buffers;

};

static void *radeonsi_create_compute_state(
	struct pipe_context *ctx,
	const struct pipe_compute_state *cso)
{
	struct r600_context *rctx = (struct r600_context *)ctx;
	struct si_pipe_compute *program =
					CALLOC_STRUCT(si_pipe_compute);
	const struct pipe_llvm_program_header *header;
	const unsigned char *code;
	LLVMModuleRef mod;

	header = cso->prog;
	code = cso->prog + sizeof(struct pipe_llvm_program_header);

	program->ctx = rctx;
	program->local_size = cso->req_local_mem;
	program->private_size = cso->req_private_mem;
	program->input_size = cso->req_input_mem;

	mod = radeon_llvm_parse_bitcode(code, header->num_bytes);
	si_compile_llvm(rctx, &program->shader, mod);

	return program;
}

static void radeonsi_bind_compute_state(struct pipe_context *ctx, void *state)
{
	struct r600_context *rctx = (struct r600_context*)ctx;
	rctx->cs_shader_state.program = (struct si_pipe_compute*)state;
}

static void radeonsi_set_global_binding(
	struct pipe_context *ctx, unsigned first, unsigned n,
	struct pipe_resource **resources,
	uint32_t **handles)
{
	unsigned i;
	struct r600_context *rctx = (struct r600_context*)ctx;
	struct si_pipe_compute *program = rctx->cs_shader_state.program;
	struct si_pm4_state *pm4;

	if (!program->pm4_buffers) {
		program->pm4_buffers = CALLOC_STRUCT(si_pm4_state);
	}
	pm4 = program->pm4_buffers;
	pm4->compute_pkt = true;

	if (!resources) {
		return;
	}

	for (i = first; i < first + n; i++) {
		uint64_t va = r600_resource_va(ctx->screen, resources[i]);
		si_pm4_add_bo(pm4, (struct si_resource*)resources[i],
				RADEON_USAGE_READWRITE);
		memcpy(handles[i], &va, sizeof(va));
	}
}

static void radeonsi_launch_grid(
		struct pipe_context *ctx,
		const uint *block_layout, const uint *grid_layout,
		uint32_t pc, const void *input)
{
	struct r600_context *rctx = (struct r600_context*)ctx;
	struct si_pipe_compute *program = rctx->cs_shader_state.program;
	struct si_pm4_state *pm4 = CALLOC_STRUCT(si_pm4_state);
	uint64_t shader_va;
	unsigned arg_user_sgpr_count;
	unsigned i;

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

	arg_user_sgpr_count = program->input_size / 4;
	if (program->input_size % 4 != 0) {
		arg_user_sgpr_count++;
	}

	/* XXX: We should store arguments in memory if we run out of user sgprs.
	 */
	assert(arg_user_sgpr_count < 16);

	for (i = 0; i < arg_user_sgpr_count; i++) {
		uint32_t *args = (uint32_t*)input;
		si_pm4_set_reg(pm4, R_00B900_COMPUTE_USER_DATA_0 +
					(i * 4),
					args[i]);
	}

	si_pm4_set_reg(pm4, R_00B810_COMPUTE_START_X, 0);
	si_pm4_set_reg(pm4, R_00B814_COMPUTE_START_Y, 0);
	si_pm4_set_reg(pm4, R_00B818_COMPUTE_START_Z, 0);

	si_pm4_set_reg(pm4, R_00B81C_COMPUTE_NUM_THREAD_X,
				S_00B81C_NUM_THREAD_FULL(block_layout[0]));
	si_pm4_set_reg(pm4, R_00B820_COMPUTE_NUM_THREAD_Y,
				S_00B820_NUM_THREAD_FULL(block_layout[1]));
	si_pm4_set_reg(pm4, R_00B824_COMPUTE_NUM_THREAD_Z,
				S_00B824_NUM_THREAD_FULL(block_layout[2]));

	/* XXX: This should be:
	 * (number of compute units) * 4 * (waves per simd) - 1 */
	si_pm4_set_reg(pm4, R_00B82C_COMPUTE_MAX_WAVE_ID, 0x190 /* Default value */);

	shader_va = r600_resource_va(ctx->screen, (void *)program->shader.bo);
	si_pm4_add_bo(pm4, program->shader.bo, RADEON_USAGE_READ);
	si_pm4_set_reg(pm4, R_00B830_COMPUTE_PGM_LO, (shader_va >> 8) & 0xffffffff);
	si_pm4_set_reg(pm4, R_00B834_COMPUTE_PGM_HI, shader_va >> 40);

	si_pm4_set_reg(pm4, R_00B848_COMPUTE_PGM_RSRC1,
		/* We always use at least 3 VGPRS, these come from
		 * TIDIG_COMP_CNT.
		 * XXX: The compiler should account for this.
		 */
		S_00B848_VGPRS((MAX2(3, program->shader.num_vgprs) - 1) / 4)
		/* We always use at least 4 + arg_user_sgpr_count.  The 4 extra
		 * sgprs are from TGID_X_EN, TGID_Y_EN, TGID_Z_EN, TG_SIZE_EN
		 * XXX: The compiler should account for this.
		 */
		|  S_00B848_SGPRS(((MAX2(4 + arg_user_sgpr_count,
		                        program->shader.num_sgprs)) - 1) / 8))
		;

	si_pm4_set_reg(pm4, R_00B84C_COMPUTE_PGM_RSRC2,
		S_00B84C_SCRATCH_EN(0)
		| S_00B84C_USER_SGPR(arg_user_sgpr_count)
		| S_00B84C_TGID_X_EN(1)
		| S_00B84C_TGID_Y_EN(1)
		| S_00B84C_TGID_Z_EN(1)
		| S_00B84C_TG_SIZE_EN(1)
		| S_00B84C_TIDIG_COMP_CNT(2)
		| S_00B84C_LDS_SIZE(0)
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

	si_pm4_emit(rctx, program->pm4_buffers);
	si_pm4_emit(rctx, pm4);

#if 0
	fprintf(stderr, "cdw: %i\n", rctx->cs->cdw);
	for (i = 0; i < rctx->cs->cdw; i++) {
		fprintf(stderr, "%4i : 0x%08X\n", i, rctx->cs->buf[i]);
	}
#endif

	rctx->ws->cs_flush(rctx->cs, RADEON_FLUSH_COMPUTE);
	rctx->ws->buffer_wait(program->shader.bo->buf, 0);

	FREE(pm4);
}


static void si_delete_compute_state(struct pipe_context *ctx, void* state){}
static void si_set_compute_resources(struct pipe_context * ctx_,
		unsigned start, unsigned count,
		struct pipe_surface ** surfaces) { }
static void si_set_cs_sampler_view(struct pipe_context *ctx_,
		unsigned start_slot, unsigned count,
		struct pipe_sampler_view **views) { }

static void si_bind_compute_sampler_states(
	struct pipe_context *ctx_,
	unsigned start_slot,
	unsigned num_samplers,
	void **samplers_) { }
void si_init_compute_functions(struct r600_context *rctx)
{
	rctx->context.create_compute_state = radeonsi_create_compute_state;
	rctx->context.delete_compute_state = si_delete_compute_state;
	rctx->context.bind_compute_state = radeonsi_bind_compute_state;
/*	 ctx->context.create_sampler_view = evergreen_compute_create_sampler_view; */
	rctx->context.set_compute_resources = si_set_compute_resources;
	rctx->context.set_compute_sampler_views = si_set_cs_sampler_view;
	rctx->context.bind_compute_sampler_states = si_bind_compute_sampler_states;
	rctx->context.set_global_binding = radeonsi_set_global_binding;
	rctx->context.launch_grid = radeonsi_launch_grid;
}
