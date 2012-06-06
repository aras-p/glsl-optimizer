/*
 * Copyright 2011 Adam Rak <adam.rak@streamnovation.com>
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
 *      Adam Rak <adam.rak@streamnovation.com>
 */

#include <stdio.h>
#include <errno.h>
#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "pipe/p_context.h"
#include "util/u_blitter.h"
#include "util/u_double_list.h"
#include "util/u_transfer.h"
#include "util/u_surface.h"
#include "util/u_pack_color.h"
#include "util/u_memory.h"
#include "util/u_inlines.h"
#include "util/u_framebuffer.h"
#include "pipebuffer/pb_buffer.h"
#include "r600.h"
#include "evergreend.h"
#include "r600_resource.h"
#include "r600_shader.h"
#include "r600_pipe.h"
#include "r600_formats.h"
#include "evergreen_compute.h"
#include "r600_hw_context_priv.h"
#include "evergreen_compute_internal.h"
#include "compute_memory_pool.h"
#ifdef HAVE_OPENCL
#include "llvm_wrapper.h"
#endif

/**
RAT0 is for global binding write
VTX1 is for global binding read

for wrting images RAT1...
for reading images TEX2...
  TEX2-RAT1 is paired

TEX2... consumes the same fetch resources, that VTX2... would consume

CONST0 and VTX0 is for parameters
  CONST0 is binding smaller input parameter buffer, and for constant indexing,
  also constant cached
  VTX0 is for indirect/non-constant indexing, or if the input is bigger than
  the constant cache can handle

RAT-s are limited to 12, so we can only bind at most 11 texture for writing
because we reserve RAT0 for global bindings. With byteaddressing enabled,
we should reserve another one too.=> 10 image binding for writing max.

from Nvidia OpenCL:
  CL_DEVICE_MAX_READ_IMAGE_ARGS:        128
  CL_DEVICE_MAX_WRITE_IMAGE_ARGS:       8 

so 10 for writing is enough. 176 is the max for reading according to the docs

writable images should be listed first < 10, so their id corresponds to RAT(id+1)
writable images will consume TEX slots, VTX slots too because of linear indexing

*/

const struct u_resource_vtbl r600_global_buffer_vtbl =
{
	u_default_resource_get_handle, /* get_handle */
	r600_compute_global_buffer_destroy, /* resource_destroy */
	r600_compute_global_get_transfer, /* get_transfer */
	r600_compute_global_transfer_destroy, /* transfer_destroy */
	r600_compute_global_transfer_map, /* transfer_map */
	r600_compute_global_transfer_flush_region,/* transfer_flush_region */
	r600_compute_global_transfer_unmap, /* transfer_unmap */
	r600_compute_global_transfer_inline_write /* transfer_inline_write */
};


void *evergreen_create_compute_state(
	struct pipe_context *ctx_,
	const const struct pipe_compute_state *cso)
{
	struct r600_context *ctx = (struct r600_context *)ctx_;

#ifdef HAVE_OPENCL
	const struct pipe_llvm_program_header * header;
	const unsigned char * code;

	header = cso->prog;
	code = cso->prog + sizeof(struct pipe_llvm_program_header);
#endif

	if (!ctx->screen->screen.get_param(&ctx->screen->screen,
							PIPE_CAP_COMPUTE)) {
		fprintf(stderr, "Compute is not supported\n");
		return NULL;
	}
	struct r600_pipe_compute *shader =	CALLOC_STRUCT(r600_pipe_compute);

	shader->ctx = (struct r600_context*)ctx;
	shader->resources = (struct evergreen_compute_resource*)
			CALLOC(sizeof(struct evergreen_compute_resource),
			get_compute_resource_num());
	shader->local_size = cso->req_local_mem; ///TODO: assert it
	shader->private_size = cso->req_private_mem;
	shader->input_size = cso->req_input_mem;

#ifdef HAVE_OPENCL 
	shader->mod = llvm_parse_bitcode(code, header->num_bytes);

	r600_compute_shader_create(ctx_, shader->mod, &shader->bc);
#endif
	return shader;
}

void evergreen_delete_compute_state(struct pipe_context *ctx, void* state)
{
	struct r600_pipe_compute *shader = (struct r600_pipe_compute *)state;

	free(shader->resources);
	free(shader);
}

static void evergreen_bind_compute_state(struct pipe_context *ctx_, void *state)
{
	struct r600_context *ctx = (struct r600_context *)ctx_;

	ctx->cs_shader = (struct r600_pipe_compute *)state;

	if (!ctx->cs_shader->shader_code_bo) {

		ctx->cs_shader->shader_code_bo =
			r600_compute_buffer_alloc_vram(ctx->screen,
					ctx->cs_shader->bc.ndw * 4);

		void *p = ctx->ws->buffer_map(
					ctx->cs_shader->shader_code_bo->cs_buf,
					ctx->cs, PIPE_TRANSFER_WRITE);

		memcpy(p, ctx->cs_shader->bc.bytecode, ctx->cs_shader->bc.ndw * 4);

		ctx->ws->buffer_unmap(ctx->cs_shader->shader_code_bo->cs_buf);

	}

	evergreen_compute_init_config(ctx);

	struct evergreen_compute_resource* res = get_empty_res(ctx->cs_shader,
						COMPUTE_RESOURCE_SHADER, 0);

	if (ctx->chip_class < CAYMAN) {
		evergreen_reg_set(res, R_008C0C_SQ_GPR_RESOURCE_MGMT_3,
			S_008C0C_NUM_LS_GPRS(ctx->cs_shader->bc.ngpr));
	}

	///maybe we can use it later
	evergreen_reg_set(res, R_0286C8_SPI_THREAD_GROUPING, 0);
	///maybe we can use it later
	evergreen_reg_set(res, R_008C14_SQ_GLOBAL_GPR_RESOURCE_MGMT_2, 0);

	evergreen_reg_set(res, R_0288D4_SQ_PGM_RESOURCES_LS,
		S_0288D4_NUM_GPRS(ctx->cs_shader->bc.ngpr)
		| S_0288D4_STACK_SIZE(ctx->cs_shader->bc.nstack));
	evergreen_reg_set(res, R_0288D8_SQ_PGM_RESOURCES_LS_2, 0);

	evergreen_reg_set(res, R_0288D0_SQ_PGM_START_LS, 0);
	res->bo = ctx->cs_shader->shader_code_bo;
	res->usage = RADEON_USAGE_READ;
	res->coher_bo_size = ctx->cs_shader->bc.ndw*4;
	res->flags = COMPUTE_RES_SH_FLUSH;

	/* We can't always determine the
	 * number of iterations in a loop before it's executed,
	 * so we just need to set up the loop counter to give us the maximum
	 * number of iterations possible.  Currently, loops in shader code
	 * ignore the loop counter and use a break instruction to exit the
	 * loop at the correct time.
	 */
	evergreen_set_loop_const(ctx->cs_shader,
		0, /* index */
		0xFFF, /* Maximum value of the loop counter (i.e. when the loop
			* counter reaches this value, the program will break
			* out of the loop. */
		0x0,   /* Starting value of the loop counter. */
		0x1);  /* Amount to increment the loop counter each iteration. */
}

/* The kernel parameters are stored a vtx buffer (ID=0), besides the explicit
 * kernel parameters there are inplicit parameters that need to be stored
 * in the vertex buffer as well.  Here is how these parameters are organized in
 * the buffer:
 *
 * DWORDS 0-2: Number of work groups in each dimension (x,y,z)
 * DWORDS 3-5: Number of global work items in each dimension (x,y,z)
 * DWORDS 6-8: Number of work items within each work group in each dimension
 *             (x,y,z)
 * DWORDS 9+ : Kernel parameters
 */
void evergreen_compute_upload_input(
	struct pipe_context *ctx_,
	const uint *block_layout,
	const uint *grid_layout,
	const void *input)
{
	struct r600_context *ctx = (struct r600_context *)ctx_;
	int i;
	unsigned kernel_parameters_offset_bytes = 36;
	uint32_t * num_work_groups_start;
	uint32_t * global_size_start;
	uint32_t * local_size_start;
	uint32_t * kernel_parameters_start;

	if (ctx->cs_shader->input_size == 0) {
		return;
	}

	if (!ctx->cs_shader->kernel_param) {
		unsigned buffer_size = ctx->cs_shader->input_size;

		/* Add space for the grid dimensions */
		buffer_size += kernel_parameters_offset_bytes * sizeof(uint);
		ctx->cs_shader->kernel_param =
				r600_compute_buffer_alloc_vram(ctx->screen,
						buffer_size);
	}

	num_work_groups_start = ctx->ws->buffer_map(
			ctx->cs_shader->kernel_param->cs_buf,
			ctx->cs, PIPE_TRANSFER_WRITE);
	global_size_start = num_work_groups_start + (3 * (sizeof(uint) /4));
	local_size_start = global_size_start + (3 * (sizeof(uint)) / 4);
	kernel_parameters_start = local_size_start + (3 * (sizeof(uint)) / 4);

	/* Copy the work group size */
	memcpy(num_work_groups_start, grid_layout, 3 * sizeof(uint));

	/* Copy the global size */
	for (i = 0; i < 3; i++) {
		global_size_start[i] = grid_layout[i] * block_layout[i];
	}

	/* Copy the local dimensions */
	memcpy(local_size_start, block_layout, 3 * sizeof(uint));

	/* Copy the kernel inputs */
	memcpy(kernel_parameters_start, input, ctx->cs_shader->input_size);

	for (i = 0; i < (kernel_parameters_offset_bytes / 4) +
					(ctx->cs_shader->input_size / 4); i++) {
		COMPUTE_DBG("input %i : %i\n", i,
			((unsigned*)num_work_groups_start)[i]);
	}

	ctx->ws->buffer_unmap(ctx->cs_shader->kernel_param->cs_buf);

	///ID=0 is reserved for the parameters
	evergreen_set_vtx_resource(ctx->cs_shader,
		ctx->cs_shader->kernel_param, 0, 0, 0);
	///ID=0 is reserved for parameters
	evergreen_set_const_cache(ctx->cs_shader, 0,
		ctx->cs_shader->kernel_param, ctx->cs_shader->input_size, 0);
}

void evergreen_direct_dispatch(
		struct pipe_context *ctx_,
		const uint *block_layout, const uint *grid_layout)
{
	struct r600_context *ctx = (struct r600_context *)ctx_;

	int i;

	struct evergreen_compute_resource* res = get_empty_res(ctx->cs_shader,
		COMPUTE_RESOURCE_DISPATCH, 0);

	evergreen_reg_set(res, R_008958_VGT_PRIMITIVE_TYPE, V_008958_DI_PT_POINTLIST);

	evergreen_reg_set(res, R_00899C_VGT_COMPUTE_START_X, 0);
	evergreen_reg_set(res, R_0089A0_VGT_COMPUTE_START_Y, 0);
	evergreen_reg_set(res, R_0089A4_VGT_COMPUTE_START_Z, 0);

	evergreen_reg_set(res, R_0286EC_SPI_COMPUTE_NUM_THREAD_X, block_layout[0]);
	evergreen_reg_set(res, R_0286F0_SPI_COMPUTE_NUM_THREAD_Y, block_layout[1]);
	evergreen_reg_set(res, R_0286F4_SPI_COMPUTE_NUM_THREAD_Z, block_layout[2]);

	int group_size = 1;

	int grid_size = 1;

	for (i = 0; i < 3; i++) {
		group_size *= block_layout[i];
	}

	for (i = 0; i < 3; i++)	{
		grid_size *= grid_layout[i];
	}

	evergreen_reg_set(res, R_008970_VGT_NUM_INDICES, group_size);
	evergreen_reg_set(res, R_0089AC_VGT_COMPUTE_THREAD_GROUP_SIZE, group_size);

	evergreen_emit_raw_value(res, PKT3C(PKT3_DISPATCH_DIRECT, 3, 0));
	evergreen_emit_raw_value(res, grid_layout[0]);
	evergreen_emit_raw_value(res, grid_layout[1]);
	evergreen_emit_raw_value(res, grid_layout[2]);
	///VGT_DISPATCH_INITIATOR = COMPUTE_SHADER_EN
	evergreen_emit_raw_value(res, 1);
}

static void compute_emit_cs(struct r600_context *ctx)
{
	struct radeon_winsys_cs *cs = ctx->cs;
	int i;

	r600_emit_atom(ctx, &ctx->start_cs_cmd.atom);

	struct r600_resource *onebo = NULL;

	for (i = 0; i < get_compute_resource_num(); i++) {
		if (ctx->cs_shader->resources[i].enabled) {
			int j;
			COMPUTE_DBG("resnum: %i, cdw: %i\n", i, cs->cdw);

			for (j = 0; j < ctx->cs_shader->resources[i].cs_end; j++) {
				if (ctx->cs_shader->resources[i].do_reloc[j]) {
					assert(ctx->cs_shader->resources[i].bo);
					evergreen_emit_ctx_reloc(ctx,
						ctx->cs_shader->resources[i].bo,
						ctx->cs_shader->resources[i].usage);
				}

				cs->buf[cs->cdw++] = ctx->cs_shader->resources[i].cs[j];
			}

			if (ctx->cs_shader->resources[i].bo) {
				onebo = ctx->cs_shader->resources[i].bo;
				evergreen_emit_ctx_reloc(ctx,
					ctx->cs_shader->resources[i].bo,
					ctx->cs_shader->resources[i].usage);

				///special case for textures
				if (ctx->cs_shader->resources[i].do_reloc
					[ctx->cs_shader->resources[i].cs_end] == 2) {
					evergreen_emit_ctx_reloc(ctx,
						ctx->cs_shader->resources[i].bo,
						ctx->cs_shader->resources[i].usage);
				}

				evergreen_set_buffer_sync(ctx, ctx->cs_shader->resources[i].bo,
					ctx->cs_shader->resources[i].coher_bo_size,
					ctx->cs_shader->resources[i].flags,
					ctx->cs_shader->resources[i].usage);
			}
		}
	}

#if 0
	COMPUTE_DBG("cdw: %i\n", cs->cdw);
	for (i = 0; i < cs->cdw; i++) {
		COMPUTE_DBG("%4i : 0x%08X\n", i, ctx->cs->buf[i]);
	}
#endif

	ctx->ws->cs_flush(ctx->cs, RADEON_FLUSH_ASYNC | RADEON_FLUSH_COMPUTE);

	ctx->pm4_dirty_cdwords = 0;
	ctx->flags = 0;

	COMPUTE_DBG("shader started\n");

	ctx->ws->buffer_wait(onebo->buf, 0);

	COMPUTE_DBG("...\n");

	r600_emit_atom(ctx, &ctx->start_cs_cmd.atom);

	ctx->streamout_start = TRUE;
	ctx->streamout_append_bitmask = ~0;

}

static void evergreen_launch_grid(
		struct pipe_context *ctx_,
		const uint *block_layout, const uint *grid_layout,
		uint32_t pc, const void *input)
{
	COMPUTE_DBG("PC: %i\n", pc);

	struct r600_context *ctx = (struct r600_context *)ctx_;
	unsigned num_waves;
	unsigned num_pipes = ctx->screen->info.r600_max_pipes;
	unsigned wave_divisor = (16 * num_pipes);

	/* num_waves = ceil((tg_size.x * tg_size.y, tg_size.z) / (16 * num_pipes)) */
	num_waves = (block_layout[0] * block_layout[1] * block_layout[2] +
			wave_divisor - 1) / wave_divisor;

	COMPUTE_DBG("Using %u pipes, there are %u wavefronts per thread block\n",
							num_pipes, num_waves);

	evergreen_set_lds(ctx->cs_shader, 0, 0, num_waves);
	evergreen_compute_upload_input(ctx_, block_layout, grid_layout, input);
	evergreen_direct_dispatch(ctx_, block_layout, grid_layout);
	compute_emit_cs(ctx);
}

static void evergreen_set_compute_resources(struct pipe_context * ctx_,
		unsigned start, unsigned count,
		struct pipe_surface ** surfaces)
{
	struct r600_context *ctx = (struct r600_context *)ctx_;
	struct r600_surface **resources = (struct r600_surface **)surfaces;
	for (int i = 0; i < count; i++)	{
		if (resources[i]) {
			struct r600_resource_global *buffer =
				(struct r600_resource_global*)resources[i]->base.texture;
			if (resources[i]->base.writable) {
				assert(i+1 < 12);
				struct r600_resource_global *buffer =
					(struct r600_resource_global*)
					resources[i]->base.texture;

				evergreen_set_rat(ctx->cs_shader, i+1,
				(struct r600_resource *)resources[i]->base.texture,
				buffer->chunk->start_in_dw*4,
				resources[i]->base.texture->width0);
			}

			evergreen_set_vtx_resource(ctx->cs_shader,
				(struct r600_resource *)resources[i]->base.texture, i+2,
				 buffer->chunk->start_in_dw*4, resources[i]->base.writable);
		}
	}

}

static void evergreen_set_cs_sampler_view(struct pipe_context *ctx_,
		unsigned start_slot, unsigned count,
		struct pipe_sampler_view **views)
{
	struct r600_context *ctx = (struct r600_context *)ctx_;
	struct r600_pipe_sampler_view **resource =
		(struct r600_pipe_sampler_view **)views;

	for (int i = 0; i < count; i++)	{
		if (resource[i]) {
			assert(i+1 < 12);
			///FETCH0 = VTX0 (param buffer),
			//FETCH1 = VTX1 (global buffer pool), FETCH2... = TEX
			evergreen_set_tex_resource(ctx->cs_shader, resource[i], i+2);
		}
	}
}

static void evergreen_bind_compute_sampler_states(
	struct pipe_context *ctx_,
	unsigned start_slot,
	unsigned num_samplers,
	void **samplers_)
{
	struct r600_context *ctx = (struct r600_context *)ctx_;
	struct compute_sampler_state ** samplers =
		(struct compute_sampler_state **)samplers_;

	for (int i = 0; i < num_samplers; i++) {
		if (samplers[i]) {
			evergreen_set_sampler_resource(ctx->cs_shader, samplers[i], i);
		}
	}
}

static void evergreen_set_global_binding(
	struct pipe_context *ctx_, unsigned first, unsigned n,
	struct pipe_resource **resources,
	uint32_t **handles)
{
	struct r600_context *ctx = (struct r600_context *)ctx_;
	struct compute_memory_pool *pool = ctx->screen->global_pool;
	struct r600_resource_global **buffers =
		(struct r600_resource_global **)resources;

	if (!resources) {
		/* XXX: Unset */
		return;
	}

	compute_memory_finalize_pending(pool, ctx_);

	for (int i = 0; i < n; i++)
	{
		assert(resources[i]->target == PIPE_BUFFER);
		assert(resources[i]->bind & PIPE_BIND_GLOBAL);

		*(handles[i]) = buffers[i]->chunk->start_in_dw * 4;
	}

	evergreen_set_rat(ctx->cs_shader, 0, pool->bo, 0, pool->size_in_dw * 4);
	evergreen_set_vtx_resource(ctx->cs_shader, pool->bo, 1, 0, 1);
}


void evergreen_compute_init_config(struct r600_context *ctx)
{
	struct evergreen_compute_resource* res =
		get_empty_res(ctx->cs_shader, COMPUTE_RESOURCE_CONFIG, 0);

	int num_threads;
	int num_stack_entries;
	int num_temp_gprs;

	enum radeon_family family;
	unsigned tmp;

	family = ctx->family;

	switch (family) {
	case CHIP_CEDAR:
	default:
		num_temp_gprs = 4;
		num_threads = 128;
		num_stack_entries = 256;
		break;
	case CHIP_REDWOOD:
		num_temp_gprs = 4;
		num_threads = 128;
		num_stack_entries = 256;
		break;
	case CHIP_JUNIPER:
		num_temp_gprs = 4;
		num_threads = 128;
		num_stack_entries = 512;
		break;
	case CHIP_CYPRESS:
	case CHIP_HEMLOCK:
		num_temp_gprs = 4;
		num_threads = 128;
		num_stack_entries = 512;
		break;
	case CHIP_PALM:
		num_temp_gprs = 4;
		num_threads = 128;
		num_stack_entries = 256;
		break;
	case CHIP_SUMO:
		num_temp_gprs = 4;
		num_threads = 128;
		num_stack_entries = 256;
		break;
	case CHIP_SUMO2:
		num_temp_gprs = 4;
		num_threads = 128;
		num_stack_entries = 512;
		break;
	case CHIP_BARTS:
		num_temp_gprs = 4;
		num_threads = 128;
		num_stack_entries = 512;
		break;
	case CHIP_TURKS:
		num_temp_gprs = 4;
		num_threads = 128;
		num_stack_entries = 256;
		break;
	case CHIP_CAICOS:
		num_temp_gprs = 4;
		num_threads = 128;
		num_stack_entries = 256;
		break;
	}

	tmp = 0x00000000;
	switch (family) {
	case CHIP_CEDAR:
	case CHIP_PALM:
	case CHIP_SUMO:
	case CHIP_SUMO2:
	case CHIP_CAICOS:
		break;
	default:
		tmp |= S_008C00_VC_ENABLE(1);
		break;
	}
	tmp |= S_008C00_EXPORT_SRC_C(1);
	tmp |= S_008C00_CS_PRIO(0);
	tmp |= S_008C00_LS_PRIO(0);
	tmp |= S_008C00_HS_PRIO(0);
	tmp |= S_008C00_PS_PRIO(0);
	tmp |= S_008C00_VS_PRIO(0);
	tmp |= S_008C00_GS_PRIO(0);
	tmp |= S_008C00_ES_PRIO(0);

	evergreen_reg_set(res, R_008C00_SQ_CONFIG, tmp);

	evergreen_reg_set(res, R_008C04_SQ_GPR_RESOURCE_MGMT_1,
				S_008C04_NUM_CLAUSE_TEMP_GPRS(num_temp_gprs));
	if (ctx->chip_class < CAYMAN) {
		evergreen_reg_set(res, R_008C08_SQ_GPR_RESOURCE_MGMT_2, 0);
	}
	evergreen_reg_set(res, R_008C10_SQ_GLOBAL_GPR_RESOURCE_MGMT_1, 0);
	evergreen_reg_set(res, R_008C14_SQ_GLOBAL_GPR_RESOURCE_MGMT_2, 0);
	evergreen_reg_set(res, R_008D8C_SQ_DYN_GPR_CNTL_PS_FLUSH_REQ, (1 << 8));

	/* workaround for hw issues with dyn gpr - must set all limits to 240
	 * instead of 0, 0x1e == 240/8 */
	if (ctx->chip_class < CAYMAN) {
		evergreen_reg_set(res, R_028838_SQ_DYN_GPR_RESOURCE_LIMIT_1,
				S_028838_PS_GPRS(0x1e) |
				S_028838_VS_GPRS(0x1e) |
				S_028838_GS_GPRS(0x1e) |
				S_028838_ES_GPRS(0x1e) |
				S_028838_HS_GPRS(0x1e) |
				S_028838_LS_GPRS(0x1e));
	} else {
		evergreen_reg_set(res, 0x286f8,
				S_028838_PS_GPRS(0x1e) |
				S_028838_VS_GPRS(0x1e) |
				S_028838_GS_GPRS(0x1e) |
				S_028838_ES_GPRS(0x1e) |
				S_028838_HS_GPRS(0x1e) |
				S_028838_LS_GPRS(0x1e));
	}

	if (ctx->chip_class < CAYMAN) {

		evergreen_reg_set(res, R_008E20_SQ_STATIC_THREAD_MGMT1, 0xFFFFFFFF);
		evergreen_reg_set(res, R_008E24_SQ_STATIC_THREAD_MGMT2, 0xFFFFFFFF);
		evergreen_reg_set(res, R_008E20_SQ_STATIC_THREAD_MGMT1, 0xFFFFFFFF);
		evergreen_reg_set(res, R_008E24_SQ_STATIC_THREAD_MGMT2, 0xFFFFFFFF);
		evergreen_reg_set(res, R_008E28_SQ_STATIC_THREAD_MGMT3, 0xFFFFFFFF);
		evergreen_reg_set(res, R_008C18_SQ_THREAD_RESOURCE_MGMT_1, 0);
		tmp = S_008C1C_NUM_LS_THREADS(num_threads);
		evergreen_reg_set(res, R_008C1C_SQ_THREAD_RESOURCE_MGMT_2, tmp);
		evergreen_reg_set(res, R_008C20_SQ_STACK_RESOURCE_MGMT_1, 0);
		evergreen_reg_set(res, R_008C24_SQ_STACK_RESOURCE_MGMT_2, 0);
		tmp = S_008C28_NUM_LS_STACK_ENTRIES(num_stack_entries);
		evergreen_reg_set(res, R_008C28_SQ_STACK_RESOURCE_MGMT_3, tmp);
	}
	evergreen_reg_set(res, R_0286CC_SPI_PS_IN_CONTROL_0, S_0286CC_LINEAR_GRADIENT_ENA(1));
	evergreen_reg_set(res, R_0286D0_SPI_PS_IN_CONTROL_1, 0);
	evergreen_reg_set(res, R_0286E4_SPI_PS_IN_CONTROL_2, 0);
	evergreen_reg_set(res, R_0286D8_SPI_INPUT_Z, 0);
	evergreen_reg_set(res, R_0286E0_SPI_BARYC_CNTL, 1 << 20);
	tmp = S_0286E8_TID_IN_GROUP_ENA | S_0286E8_TGID_ENA | S_0286E8_DISABLE_INDEX_PACK;
	evergreen_reg_set(res, R_0286E8_SPI_COMPUTE_INPUT_CNTL, tmp);
	tmp = S_028A40_COMPUTE_MODE(1) | S_028A40_PARTIAL_THD_AT_EOI(1);
	evergreen_reg_set(res, R_028A40_VGT_GS_MODE, tmp);
	evergreen_reg_set(res, R_028B54_VGT_SHADER_STAGES_EN, 2/*CS_ON*/);
	evergreen_reg_set(res, R_028800_DB_DEPTH_CONTROL, 0);
	evergreen_reg_set(res, R_02880C_DB_SHADER_CONTROL, 0);
	evergreen_reg_set(res, R_028000_DB_RENDER_CONTROL, S_028000_COLOR_DISABLE(1));
	evergreen_reg_set(res, R_02800C_DB_RENDER_OVERRIDE, 0);
	evergreen_reg_set(res, R_0286E8_SPI_COMPUTE_INPUT_CNTL,
						S_0286E8_TID_IN_GROUP_ENA
						| S_0286E8_TGID_ENA
						| S_0286E8_DISABLE_INDEX_PACK)
						;
}

void evergreen_init_compute_state_functions(struct r600_context *ctx)
{
	ctx->context.create_compute_state = evergreen_create_compute_state;
	ctx->context.delete_compute_state = evergreen_delete_compute_state;
	ctx->context.bind_compute_state = evergreen_bind_compute_state;
//	 ctx->context.create_sampler_view = evergreen_compute_create_sampler_view;
	ctx->context.set_compute_resources = evergreen_set_compute_resources;
	ctx->context.set_compute_sampler_views = evergreen_set_cs_sampler_view;
	ctx->context.bind_compute_sampler_states = evergreen_bind_compute_sampler_states;
	ctx->context.set_global_binding = evergreen_set_global_binding;
	ctx->context.launch_grid = evergreen_launch_grid;
}


struct pipe_resource *r600_compute_global_buffer_create(
	struct pipe_screen *screen,
	const struct pipe_resource *templ)
{
	assert(templ->target == PIPE_BUFFER);
	assert(templ->bind & PIPE_BIND_GLOBAL);
	assert(templ->array_size == 1 || templ->array_size == 0);
	assert(templ->depth0 == 1 || templ->depth0 == 0);
	assert(templ->height0 == 1 || templ->height0 == 0);

	struct r600_resource_global* result = (struct r600_resource_global*)
		CALLOC(sizeof(struct r600_resource_global), 1);
	struct r600_screen* rscreen = (struct r600_screen*)screen;

	result->base.b.vtbl = &r600_global_buffer_vtbl;
	result->base.b.b.screen = screen;
	result->base.b.b = *templ;
	pipe_reference_init(&result->base.b.b.reference, 1);

	int size_in_dw = (templ->width0+3) / 4;

	result->chunk = compute_memory_alloc(rscreen->global_pool, size_in_dw);

	if (result->chunk == NULL)
	{
		free(result);
		return NULL;
	}

	return &result->base.b.b;
}

void r600_compute_global_buffer_destroy(
	struct pipe_screen *screen,
	struct pipe_resource *res)
{
	assert(res->target == PIPE_BUFFER);
	assert(res->bind & PIPE_BIND_GLOBAL);

	struct r600_resource_global* buffer = (struct r600_resource_global*)res;
	struct r600_screen* rscreen = (struct r600_screen*)screen;

	compute_memory_free(rscreen->global_pool, buffer->chunk->id);

	buffer->chunk = NULL;
	free(res);
}

void* r600_compute_global_transfer_map(
	struct pipe_context *ctx_,
	struct pipe_transfer* transfer)
{
	assert(transfer->resource->target == PIPE_BUFFER);
	assert(transfer->resource->bind & PIPE_BIND_GLOBAL);
	assert(transfer->box.x >= 0);
	assert(transfer->box.y == 0);
	assert(transfer->box.z == 0);

	struct r600_context *ctx = (struct r600_context *)ctx_;
	struct r600_resource_global* buffer =
		(struct r600_resource_global*)transfer->resource;

	uint32_t* map;
	///TODO: do it better, mapping is not possible if the pool is too big

	if (!(map = ctx->ws->buffer_map(buffer->chunk->pool->bo->cs_buf,
						ctx->cs, transfer->usage))) {
		return NULL;
	}

	COMPUTE_DBG("buffer start: %lli\n", buffer->chunk->start_in_dw);
	return ((char*)(map + buffer->chunk->start_in_dw)) + transfer->box.x;
}

void r600_compute_global_transfer_unmap(
	struct pipe_context *ctx_,
	struct pipe_transfer* transfer)
{
	assert(transfer->resource->target == PIPE_BUFFER);
	assert(transfer->resource->bind & PIPE_BIND_GLOBAL);

	struct r600_context *ctx = (struct r600_context *)ctx_;
	struct r600_resource_global* buffer =
		(struct r600_resource_global*)transfer->resource;

	ctx->ws->buffer_unmap(buffer->chunk->pool->bo->cs_buf);
}

struct pipe_transfer * r600_compute_global_get_transfer(
	struct pipe_context *ctx_,
	struct pipe_resource *resource,
	unsigned level,
	unsigned usage,
	const struct pipe_box *box)
{
	struct r600_context *ctx = (struct r600_context *)ctx_;
	struct compute_memory_pool *pool = ctx->screen->global_pool;

	compute_memory_finalize_pending(pool, ctx_);

	assert(resource->target == PIPE_BUFFER);
	struct r600_context *rctx = (struct r600_context*)ctx_;
	struct pipe_transfer *transfer = util_slab_alloc(&rctx->pool_transfers);

	transfer->resource = resource;
	transfer->level = level;
	transfer->usage = usage;
	transfer->box = *box;
	transfer->stride = 0;
	transfer->layer_stride = 0;
	transfer->data = NULL;

	/* Note strides are zero, this is ok for buffers, but not for
	* textures 2d & higher at least.
	*/
	return transfer;
}

void r600_compute_global_transfer_destroy(
	struct pipe_context *ctx_,
	struct pipe_transfer *transfer)
{
	struct r600_context *rctx = (struct r600_context*)ctx_;
	util_slab_free(&rctx->pool_transfers, transfer);
}

void r600_compute_global_transfer_flush_region(
	struct pipe_context *ctx_,
	struct pipe_transfer *transfer,
	const struct pipe_box *box)
{
	assert(0 && "TODO");
}

void r600_compute_global_transfer_inline_write(
	struct pipe_context *pipe,
	struct pipe_resource *resource,
	unsigned level,
	unsigned usage,
	const struct pipe_box *box,
	const void *data,
	unsigned stride,
	unsigned layer_stride)
{
	assert(0 && "TODO");
}
