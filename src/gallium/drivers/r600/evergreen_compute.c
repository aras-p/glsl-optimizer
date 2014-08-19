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
#include "evergreend.h"
#include "r600_shader.h"
#include "r600_pipe.h"
#include "r600_formats.h"
#include "evergreen_compute.h"
#include "evergreen_compute_internal.h"
#include "compute_memory_pool.h"
#include "sb/sb_public.h"
#ifdef HAVE_OPENCL
#include "radeon_llvm_util.h"
#endif
#include <inttypes.h>

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

struct r600_resource* r600_compute_buffer_alloc_vram(
       struct r600_screen *screen,
       unsigned size)
{
	struct pipe_resource * buffer = NULL;
	assert(size);

	buffer = pipe_buffer_create(
		(struct pipe_screen*) screen,
		PIPE_BIND_CUSTOM,
		PIPE_USAGE_IMMUTABLE,
		size);

	return (struct r600_resource *)buffer;
}


static void evergreen_set_rat(
	struct r600_pipe_compute *pipe,
	unsigned id,
	struct r600_resource* bo,
	int start,
	int size)
{
	struct pipe_surface rat_templ;
	struct r600_surface *surf = NULL;
	struct r600_context *rctx = NULL;

	assert(id < 12);
	assert((size & 3) == 0);
	assert((start & 0xFF) == 0);

	rctx = pipe->ctx;

	COMPUTE_DBG(rctx->screen, "bind rat: %i \n", id);

	/* Create the RAT surface */
	memset(&rat_templ, 0, sizeof(rat_templ));
	rat_templ.format = PIPE_FORMAT_R32_UINT;
	rat_templ.u.tex.level = 0;
	rat_templ.u.tex.first_layer = 0;
	rat_templ.u.tex.last_layer = 0;

	/* Add the RAT the list of color buffers */
	pipe->ctx->framebuffer.state.cbufs[id] = pipe->ctx->b.b.create_surface(
		(struct pipe_context *)pipe->ctx,
		(struct pipe_resource *)bo, &rat_templ);

	/* Update the number of color buffers */
	pipe->ctx->framebuffer.state.nr_cbufs =
		MAX2(id + 1, pipe->ctx->framebuffer.state.nr_cbufs);

	/* Update the cb_target_mask
	 * XXX: I think this is a potential spot for bugs once we start doing
	 * GL interop.  cb_target_mask may be modified in the 3D sections
	 * of this driver. */
	pipe->ctx->compute_cb_target_mask |= (0xf << (id * 4));

	surf = (struct r600_surface*)pipe->ctx->framebuffer.state.cbufs[id];
	evergreen_init_color_surface_rat(rctx, surf);
}

static void evergreen_cs_set_vertex_buffer(
	struct r600_context * rctx,
	unsigned vb_index,
	unsigned offset,
	struct pipe_resource * buffer)
{
	struct r600_vertexbuf_state *state = &rctx->cs_vertex_buffer_state;
	struct pipe_vertex_buffer *vb = &state->vb[vb_index];
	vb->stride = 1;
	vb->buffer_offset = offset;
	vb->buffer = buffer;
	vb->user_buffer = NULL;

	/* The vertex instructions in the compute shaders use the texture cache,
	 * so we need to invalidate it. */
	rctx->b.flags |= R600_CONTEXT_INV_VERTEX_CACHE;
	state->enabled_mask |= 1 << vb_index;
	state->dirty_mask |= 1 << vb_index;
	state->atom.dirty = true;
}

static void evergreen_cs_set_constant_buffer(
	struct r600_context * rctx,
	unsigned cb_index,
	unsigned offset,
	unsigned size,
	struct pipe_resource * buffer)
{
	struct pipe_constant_buffer cb;
	cb.buffer_size = size;
	cb.buffer_offset = offset;
	cb.buffer = buffer;
	cb.user_buffer = NULL;

	rctx->b.b.set_constant_buffer(&rctx->b.b, PIPE_SHADER_COMPUTE, cb_index, &cb);
}

static const struct u_resource_vtbl r600_global_buffer_vtbl =
{
	u_default_resource_get_handle, /* get_handle */
	r600_compute_global_buffer_destroy, /* resource_destroy */
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
	struct r600_pipe_compute *shader = CALLOC_STRUCT(r600_pipe_compute);

#ifdef HAVE_OPENCL
	const struct pipe_llvm_program_header * header;
	const unsigned char * code;
	unsigned i;

	shader->llvm_ctx = LLVMContextCreate();

	COMPUTE_DBG(ctx->screen, "*** evergreen_create_compute_state\n");

	header = cso->prog;
	code = cso->prog + sizeof(struct pipe_llvm_program_header);
#endif

	shader->ctx = (struct r600_context*)ctx;
	shader->local_size = cso->req_local_mem;
	shader->private_size = cso->req_private_mem;
	shader->input_size = cso->req_input_mem;

#ifdef HAVE_OPENCL 
	shader->num_kernels = radeon_llvm_get_num_kernels(shader->llvm_ctx, code,
							header->num_bytes);
	shader->kernels = CALLOC(sizeof(struct r600_kernel), shader->num_kernels);

	for (i = 0; i < shader->num_kernels; i++) {
		struct r600_kernel *kernel = &shader->kernels[i];
		kernel->llvm_module = radeon_llvm_get_kernel_module(shader->llvm_ctx, i,
							code, header->num_bytes);
	}
#endif
	return shader;
}

void evergreen_delete_compute_state(struct pipe_context *ctx, void* state)
{
	struct r600_pipe_compute *shader = (struct r600_pipe_compute *)state;

	if (!shader)
		return;

	FREE(shader->kernels);

#ifdef HAVE_OPENCL
	if (shader->llvm_ctx){
		LLVMContextDispose(shader->llvm_ctx);
	}
#endif

	FREE(shader);
}

static void evergreen_bind_compute_state(struct pipe_context *ctx_, void *state)
{
	struct r600_context *ctx = (struct r600_context *)ctx_;

	COMPUTE_DBG(ctx->screen, "*** evergreen_bind_compute_state\n");

	ctx->cs_shader_state.shader = (struct r600_pipe_compute *)state;
}

/* The kernel parameters are stored a vtx buffer (ID=0), besides the explicit
 * kernel parameters there are implicit parameters that need to be stored
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
	struct r600_pipe_compute *shader = ctx->cs_shader_state.shader;
	unsigned i;
	/* We need to reserve 9 dwords (36 bytes) for implicit kernel
	 * parameters.
	 */
	unsigned input_size = shader->input_size + 36;
	uint32_t * num_work_groups_start;
	uint32_t * global_size_start;
	uint32_t * local_size_start;
	uint32_t * kernel_parameters_start;
	struct pipe_box box;
	struct pipe_transfer *transfer = NULL;

	if (shader->input_size == 0) {
		return;
	}

	if (!shader->kernel_param) {
		/* Add space for the grid dimensions */
		shader->kernel_param = (struct r600_resource *)
			pipe_buffer_create(ctx_->screen, PIPE_BIND_CUSTOM,
					PIPE_USAGE_IMMUTABLE, input_size);
	}

	u_box_1d(0, input_size, &box);
	num_work_groups_start = ctx_->transfer_map(ctx_,
			(struct pipe_resource*)shader->kernel_param,
			0, PIPE_TRANSFER_WRITE | PIPE_TRANSFER_DISCARD_RANGE,
			&box, &transfer);
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
	memcpy(kernel_parameters_start, input, shader->input_size);

	for (i = 0; i < (input_size / 4); i++) {
		COMPUTE_DBG(ctx->screen, "input %i : %u\n", i,
			((unsigned*)num_work_groups_start)[i]);
	}

	ctx_->transfer_unmap(ctx_, transfer);

	/* ID=0 is reserved for the parameters */
	evergreen_cs_set_constant_buffer(ctx, 0, 0, input_size,
			(struct pipe_resource*)shader->kernel_param);
}

static void evergreen_emit_direct_dispatch(
		struct r600_context *rctx,
		const uint *block_layout, const uint *grid_layout)
{
	int i;
	struct radeon_winsys_cs *cs = rctx->b.rings.gfx.cs;
	struct r600_pipe_compute *shader = rctx->cs_shader_state.shader;
	unsigned num_waves;
	unsigned num_pipes = rctx->screen->b.info.r600_max_pipes;
	unsigned wave_divisor = (16 * num_pipes);
	int group_size = 1;
	int grid_size = 1;
	unsigned lds_size = shader->local_size / 4 + shader->active_kernel->bc.nlds_dw;

	/* Calculate group_size/grid_size */
	for (i = 0; i < 3; i++) {
		group_size *= block_layout[i];
	}

	for (i = 0; i < 3; i++)	{
		grid_size *= grid_layout[i];
	}

	/* num_waves = ceil((tg_size.x * tg_size.y, tg_size.z) / (16 * num_pipes)) */
	num_waves = (block_layout[0] * block_layout[1] * block_layout[2] +
			wave_divisor - 1) / wave_divisor;

	COMPUTE_DBG(rctx->screen, "Using %u pipes, "
				"%u wavefronts per thread block, "
				"allocating %u dwords lds.\n",
				num_pipes, num_waves, lds_size);

	r600_write_config_reg(cs, R_008970_VGT_NUM_INDICES, group_size);

	r600_write_config_reg_seq(cs, R_00899C_VGT_COMPUTE_START_X, 3);
	radeon_emit(cs, 0); /* R_00899C_VGT_COMPUTE_START_X */
	radeon_emit(cs, 0); /* R_0089A0_VGT_COMPUTE_START_Y */
	radeon_emit(cs, 0); /* R_0089A4_VGT_COMPUTE_START_Z */

	r600_write_config_reg(cs, R_0089AC_VGT_COMPUTE_THREAD_GROUP_SIZE,
								group_size);

	r600_write_compute_context_reg_seq(cs, R_0286EC_SPI_COMPUTE_NUM_THREAD_X, 3);
	radeon_emit(cs, block_layout[0]); /* R_0286EC_SPI_COMPUTE_NUM_THREAD_X */
	radeon_emit(cs, block_layout[1]); /* R_0286F0_SPI_COMPUTE_NUM_THREAD_Y */
	radeon_emit(cs, block_layout[2]); /* R_0286F4_SPI_COMPUTE_NUM_THREAD_Z */

	if (rctx->b.chip_class < CAYMAN) {
		assert(lds_size <= 8192);
	} else {
		/* Cayman appears to have a slightly smaller limit, see the
		 * value of CM_R_0286FC_SPI_LDS_MGMT.NUM_LS_LDS */
		assert(lds_size <= 8160);
	}

	r600_write_compute_context_reg(cs, CM_R_0288E8_SQ_LDS_ALLOC,
					lds_size | (num_waves << 14));

	/* Dispatch packet */
	radeon_emit(cs, PKT3C(PKT3_DISPATCH_DIRECT, 3, 0));
	radeon_emit(cs, grid_layout[0]);
	radeon_emit(cs, grid_layout[1]);
	radeon_emit(cs, grid_layout[2]);
	/* VGT_DISPATCH_INITIATOR = COMPUTE_SHADER_EN */
	radeon_emit(cs, 1);
}

static void compute_emit_cs(struct r600_context *ctx, const uint *block_layout,
		const uint *grid_layout)
{
	struct radeon_winsys_cs *cs = ctx->b.rings.gfx.cs;
	unsigned i;

	/* make sure that the gfx ring is only one active */
	if (ctx->b.rings.dma.cs && ctx->b.rings.dma.cs->cdw) {
		ctx->b.rings.dma.flush(ctx, RADEON_FLUSH_ASYNC, NULL);
	}

	/* Initialize all the compute-related registers.
	 *
	 * See evergreen_init_atom_start_compute_cs() in this file for the list
	 * of registers initialized by the start_compute_cs_cmd atom.
	 */
	r600_emit_command_buffer(cs, &ctx->start_compute_cs_cmd);

	ctx->b.flags |= R600_CONTEXT_WAIT_3D_IDLE | R600_CONTEXT_FLUSH_AND_INV;
	r600_flush_emit(ctx);

	/* Emit colorbuffers. */
	/* XXX support more than 8 colorbuffers (the offsets are not a multiple of 0x3C for CB8-11) */
	for (i = 0; i < 8 && i < ctx->framebuffer.state.nr_cbufs; i++) {
		struct r600_surface *cb = (struct r600_surface*)ctx->framebuffer.state.cbufs[i];
		unsigned reloc = r600_context_bo_reloc(&ctx->b, &ctx->b.rings.gfx,
						       (struct r600_resource*)cb->base.texture,
						       RADEON_USAGE_READWRITE,
						       RADEON_PRIO_SHADER_RESOURCE_RW);

		r600_write_compute_context_reg_seq(cs, R_028C60_CB_COLOR0_BASE + i * 0x3C, 7);
		radeon_emit(cs, cb->cb_color_base);	/* R_028C60_CB_COLOR0_BASE */
		radeon_emit(cs, cb->cb_color_pitch);	/* R_028C64_CB_COLOR0_PITCH */
		radeon_emit(cs, cb->cb_color_slice);	/* R_028C68_CB_COLOR0_SLICE */
		radeon_emit(cs, cb->cb_color_view);	/* R_028C6C_CB_COLOR0_VIEW */
		radeon_emit(cs, cb->cb_color_info);	/* R_028C70_CB_COLOR0_INFO */
		radeon_emit(cs, cb->cb_color_attrib);	/* R_028C74_CB_COLOR0_ATTRIB */
		radeon_emit(cs, cb->cb_color_dim);		/* R_028C78_CB_COLOR0_DIM */

		radeon_emit(cs, PKT3(PKT3_NOP, 0, 0)); /* R_028C60_CB_COLOR0_BASE */
		radeon_emit(cs, reloc);

		if (!ctx->keep_tiling_flags) {
			radeon_emit(cs, PKT3(PKT3_NOP, 0, 0)); /* R_028C70_CB_COLOR0_INFO */
			radeon_emit(cs, reloc);
		}

		radeon_emit(cs, PKT3(PKT3_NOP, 0, 0)); /* R_028C74_CB_COLOR0_ATTRIB */
		radeon_emit(cs, reloc);
	}
	if (ctx->keep_tiling_flags) {
		for (; i < 8 ; i++) {
			r600_write_compute_context_reg(cs, R_028C70_CB_COLOR0_INFO + i * 0x3C,
						       S_028C70_FORMAT(V_028C70_COLOR_INVALID));
		}
		for (; i < 12; i++) {
			r600_write_compute_context_reg(cs, R_028E50_CB_COLOR8_INFO + (i - 8) * 0x1C,
						       S_028C70_FORMAT(V_028C70_COLOR_INVALID));
		}
	}

	/* Set CB_TARGET_MASK  XXX: Use cb_misc_state */
	r600_write_compute_context_reg(cs, R_028238_CB_TARGET_MASK,
					ctx->compute_cb_target_mask);


	/* Emit vertex buffer state */
	ctx->cs_vertex_buffer_state.atom.num_dw = 12 * util_bitcount(ctx->cs_vertex_buffer_state.dirty_mask);
	r600_emit_atom(ctx, &ctx->cs_vertex_buffer_state.atom);

	/* Emit constant buffer state */
	r600_emit_atom(ctx, &ctx->constbuf_state[PIPE_SHADER_COMPUTE].atom);

	/* Emit compute shader state */
	r600_emit_atom(ctx, &ctx->cs_shader_state.atom);

	/* Emit dispatch state and dispatch packet */
	evergreen_emit_direct_dispatch(ctx, block_layout, grid_layout);

	/* XXX evergreen_flush_emit() hardcodes the CP_COHER_SIZE to 0xffffffff
	 */
	ctx->b.flags |= R600_CONTEXT_INV_CONST_CACHE |
		      R600_CONTEXT_INV_VERTEX_CACHE |
	              R600_CONTEXT_INV_TEX_CACHE;
	r600_flush_emit(ctx);
	ctx->b.flags = 0;

	if (ctx->b.chip_class >= CAYMAN) {
		cs->buf[cs->cdw++] = PKT3(PKT3_EVENT_WRITE, 0, 0);
		cs->buf[cs->cdw++] = EVENT_TYPE(EVENT_TYPE_CS_PARTIAL_FLUSH) | EVENT_INDEX(4);
		/* DEALLOC_STATE prevents the GPU from hanging when a
		 * SURFACE_SYNC packet is emitted some time after a DISPATCH_DIRECT
		 * with any of the CB*_DEST_BASE_ENA or DB_DEST_BASE_ENA bits set.
		 */
		cs->buf[cs->cdw++] = PKT3C(PKT3_DEALLOC_STATE, 0, 0);
		cs->buf[cs->cdw++] = 0;
	}

#if 0
	COMPUTE_DBG(ctx->screen, "cdw: %i\n", cs->cdw);
	for (i = 0; i < cs->cdw; i++) {
		COMPUTE_DBG(ctx->screen, "%4i : 0x%08X\n", i, cs->buf[i]);
	}
#endif

}


/**
 * Emit function for r600_cs_shader_state atom
 */
void evergreen_emit_cs_shader(
		struct r600_context *rctx,
		struct r600_atom *atom)
{
	struct r600_cs_shader_state *state =
					(struct r600_cs_shader_state*)atom;
	struct r600_pipe_compute *shader = state->shader;
	struct r600_kernel *kernel = &shader->kernels[state->kernel_index];
	struct radeon_winsys_cs *cs = rctx->b.rings.gfx.cs;

	r600_write_compute_context_reg_seq(cs, R_0288D0_SQ_PGM_START_LS, 3);
	radeon_emit(cs, kernel->code_bo->gpu_address >> 8); /* R_0288D0_SQ_PGM_START_LS */
	radeon_emit(cs,           /* R_0288D4_SQ_PGM_RESOURCES_LS */
			S_0288D4_NUM_GPRS(kernel->bc.ngpr)
			| S_0288D4_STACK_SIZE(kernel->bc.nstack));
	radeon_emit(cs, 0);	/* R_0288D8_SQ_PGM_RESOURCES_LS_2 */

	radeon_emit(cs, PKT3C(PKT3_NOP, 0, 0));
	radeon_emit(cs, r600_context_bo_reloc(&rctx->b, &rctx->b.rings.gfx,
					      kernel->code_bo, RADEON_USAGE_READ,
					      RADEON_PRIO_SHADER_DATA));
}

static void evergreen_launch_grid(
		struct pipe_context *ctx_,
		const uint *block_layout, const uint *grid_layout,
		uint32_t pc, const void *input)
{
	struct r600_context *ctx = (struct r600_context *)ctx_;

	struct r600_pipe_compute *shader = ctx->cs_shader_state.shader;
	struct r600_kernel *kernel = &shader->kernels[pc];

	COMPUTE_DBG(ctx->screen, "*** evergreen_launch_grid: pc = %u\n", pc);

#ifdef HAVE_OPENCL

	if (!kernel->code_bo) {
		void *p;
		struct r600_bytecode *bc = &kernel->bc;
		LLVMModuleRef mod = kernel->llvm_module;
		boolean use_kill = false;
		bool dump = (ctx->screen->b.debug_flags & DBG_CS) != 0;
		unsigned use_sb = ctx->screen->b.debug_flags & DBG_SB_CS;
		unsigned sb_disasm = use_sb ||
			(ctx->screen->b.debug_flags & DBG_SB_DISASM);

		r600_bytecode_init(bc, ctx->b.chip_class, ctx->b.family,
			   ctx->screen->has_compressed_msaa_texturing);
		bc->type = TGSI_PROCESSOR_COMPUTE;
		bc->isa = ctx->isa;
		r600_llvm_compile(mod, ctx->b.family, bc, &use_kill, dump);

		if (dump && !sb_disasm) {
			r600_bytecode_disasm(bc);
		} else if ((dump && sb_disasm) || use_sb) {
			if (r600_sb_bytecode_process(ctx, bc, NULL, dump, use_sb))
				R600_ERR("r600_sb_bytecode_process failed!\n");
		}

		kernel->code_bo = r600_compute_buffer_alloc_vram(ctx->screen,
							kernel->bc.ndw * 4);
		p = r600_buffer_map_sync_with_rings(&ctx->b, kernel->code_bo, PIPE_TRANSFER_WRITE);
		memcpy(p, kernel->bc.bytecode, kernel->bc.ndw * 4);
		ctx->b.ws->buffer_unmap(kernel->code_bo->cs_buf);
	}
#endif
	shader->active_kernel = kernel;
	ctx->cs_shader_state.kernel_index = pc;
	evergreen_compute_upload_input(ctx_, block_layout, grid_layout, input);
	compute_emit_cs(ctx, block_layout, grid_layout);
}

static void evergreen_set_compute_resources(struct pipe_context * ctx_,
		unsigned start, unsigned count,
		struct pipe_surface ** surfaces)
{
	struct r600_context *ctx = (struct r600_context *)ctx_;
	struct r600_surface **resources = (struct r600_surface **)surfaces;

	COMPUTE_DBG(ctx->screen, "*** evergreen_set_compute_resources: start = %u count = %u\n",
			start, count);

	for (unsigned i = 0; i < count; i++) {
		/* The First two vertex buffers are reserved for parameters and
		 * global buffers. */
		unsigned vtx_id = 2 + i;
		if (resources[i]) {
			struct r600_resource_global *buffer =
				(struct r600_resource_global*)
				resources[i]->base.texture;
			if (resources[i]->base.writable) {
				assert(i+1 < 12);

				evergreen_set_rat(ctx->cs_shader_state.shader, i+1,
				(struct r600_resource *)resources[i]->base.texture,
				buffer->chunk->start_in_dw*4,
				resources[i]->base.texture->width0);
			}

			evergreen_cs_set_vertex_buffer(ctx, vtx_id,
					buffer->chunk->start_in_dw * 4,
					resources[i]->base.texture);
		}
	}
}

void evergreen_set_cs_sampler_view(struct pipe_context *ctx_,
		unsigned start_slot, unsigned count,
		struct pipe_sampler_view **views)
{
	struct r600_pipe_sampler_view **resource =
		(struct r600_pipe_sampler_view **)views;

	for (unsigned i = 0; i < count; i++)	{
		if (resource[i]) {
			assert(i+1 < 12);
			/* XXX: Implement */
			assert(!"Compute samplers not implemented.");
			///FETCH0 = VTX0 (param buffer),
			//FETCH1 = VTX1 (global buffer pool), FETCH2... = TEX
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
	unsigned i;

	COMPUTE_DBG(ctx->screen, "*** evergreen_set_global_binding first = %u n = %u\n",
			first, n);

	if (!resources) {
		/* XXX: Unset */
		return;
	}

	/* We mark these items for promotion to the pool if they
	 * aren't already there */
	for (i = first; i < first + n; i++) {
		struct compute_memory_item *item = buffers[i]->chunk;

		if (!is_item_in_pool(item))
			buffers[i]->chunk->status |= ITEM_FOR_PROMOTING;
	}

	if (compute_memory_finalize_pending(pool, ctx_) == -1) {
		/* XXX: Unset */
		return;
	}

	for (i = first; i < first + n; i++)
	{
		uint32_t buffer_offset;
		uint32_t handle;
		assert(resources[i]->target == PIPE_BUFFER);
		assert(resources[i]->bind & PIPE_BIND_GLOBAL);

		buffer_offset = util_le32_to_cpu(*(handles[i]));
		handle = buffer_offset + buffers[i]->chunk->start_in_dw * 4;

		*(handles[i]) = util_cpu_to_le32(handle);
	}

	evergreen_set_rat(ctx->cs_shader_state.shader, 0, pool->bo, 0, pool->size_in_dw * 4);
	evergreen_cs_set_vertex_buffer(ctx, 1, 0,
				(struct pipe_resource*)pool->bo);
}

/**
 * This function initializes all the compute specific registers that need to
 * be initialized for each compute command stream.  Registers that are common
 * to both compute and 3D will be initialized at the beginning of each compute
 * command stream by the start_cs_cmd atom.  However, since the SET_CONTEXT_REG
 * packet requires that the shader type bit be set, we must initialize all
 * context registers needed for compute in this function.  The registers
 * intialized by the start_cs_cmd atom can be found in evereen_state.c in the
 * functions evergreen_init_atom_start_cs or cayman_init_atom_start_cs depending
 * on the GPU family.
 */
void evergreen_init_atom_start_compute_cs(struct r600_context *ctx)
{
	struct r600_command_buffer *cb = &ctx->start_compute_cs_cmd;
	int num_threads;
	int num_stack_entries;

	/* since all required registers are initialised in the
	 * start_compute_cs_cmd atom, we can EMIT_EARLY here.
	 */
	r600_init_command_buffer(cb, 256);
	cb->pkt_flags = RADEON_CP_PACKET3_COMPUTE_MODE;

	/* This must be first. */
	r600_store_value(cb, PKT3(PKT3_CONTEXT_CONTROL, 1, 0));
	r600_store_value(cb, 0x80000000);
	r600_store_value(cb, 0x80000000);

	/* We're setting config registers here. */
	r600_store_value(cb, PKT3(PKT3_EVENT_WRITE, 0, 0));
	r600_store_value(cb, EVENT_TYPE(EVENT_TYPE_CS_PARTIAL_FLUSH) | EVENT_INDEX(4));

	switch (ctx->b.family) {
	case CHIP_CEDAR:
	default:
		num_threads = 128;
		num_stack_entries = 256;
		break;
	case CHIP_REDWOOD:
		num_threads = 128;
		num_stack_entries = 256;
		break;
	case CHIP_JUNIPER:
		num_threads = 128;
		num_stack_entries = 512;
		break;
	case CHIP_CYPRESS:
	case CHIP_HEMLOCK:
		num_threads = 128;
		num_stack_entries = 512;
		break;
	case CHIP_PALM:
		num_threads = 128;
		num_stack_entries = 256;
		break;
	case CHIP_SUMO:
		num_threads = 128;
		num_stack_entries = 256;
		break;
	case CHIP_SUMO2:
		num_threads = 128;
		num_stack_entries = 512;
		break;
	case CHIP_BARTS:
		num_threads = 128;
		num_stack_entries = 512;
		break;
	case CHIP_TURKS:
		num_threads = 128;
		num_stack_entries = 256;
		break;
	case CHIP_CAICOS:
		num_threads = 128;
		num_stack_entries = 256;
		break;
	}

	/* Config Registers */
	if (ctx->b.chip_class < CAYMAN)
		evergreen_init_common_regs(cb, ctx->b.chip_class, ctx->b.family,
					   ctx->screen->b.info.drm_minor);
	else
		cayman_init_common_regs(cb, ctx->b.chip_class, ctx->b.family,
					ctx->screen->b.info.drm_minor);

	/* The primitive type always needs to be POINTLIST for compute. */
	r600_store_config_reg(cb, R_008958_VGT_PRIMITIVE_TYPE,
						V_008958_DI_PT_POINTLIST);

	if (ctx->b.chip_class < CAYMAN) {

		/* These registers control which simds can be used by each stage.
		 * The default for these registers is 0xffffffff, which means
		 * all simds are available for each stage.  It's possible we may
		 * want to play around with these in the future, but for now
		 * the default value is fine.
		 *
		 * R_008E20_SQ_STATIC_THREAD_MGMT1
		 * R_008E24_SQ_STATIC_THREAD_MGMT2
		 * R_008E28_SQ_STATIC_THREAD_MGMT3
		 */

		/* XXX: We may need to adjust the thread and stack resouce
		 * values for 3D/compute interop */

		r600_store_config_reg_seq(cb, R_008C18_SQ_THREAD_RESOURCE_MGMT_1, 5);

		/* R_008C18_SQ_THREAD_RESOURCE_MGMT_1
		 * Set the number of threads used by the PS/VS/GS/ES stage to
		 * 0.
		 */
		r600_store_value(cb, 0);

		/* R_008C1C_SQ_THREAD_RESOURCE_MGMT_2
		 * Set the number of threads used by the CS (aka LS) stage to
		 * the maximum number of threads and set the number of threads
		 * for the HS stage to 0. */
		r600_store_value(cb, S_008C1C_NUM_LS_THREADS(num_threads));

		/* R_008C20_SQ_STACK_RESOURCE_MGMT_1
		 * Set the Control Flow stack entries to 0 for PS/VS stages */
		r600_store_value(cb, 0);

		/* R_008C24_SQ_STACK_RESOURCE_MGMT_2
		 * Set the Control Flow stack entries to 0 for GS/ES stages */
		r600_store_value(cb, 0);

		/* R_008C28_SQ_STACK_RESOURCE_MGMT_3
		 * Set the Contol Flow stack entries to 0 for the HS stage, and
		 * set it to the maximum value for the CS (aka LS) stage. */
		r600_store_value(cb,
			S_008C28_NUM_LS_STACK_ENTRIES(num_stack_entries));
	}
	/* Give the compute shader all the available LDS space.
	 * NOTE: This only sets the maximum number of dwords that a compute
	 * shader can allocate.  When a shader is executed, we still need to
	 * allocate the appropriate amount of LDS dwords using the
	 * CM_R_0288E8_SQ_LDS_ALLOC register.
	 */
	if (ctx->b.chip_class < CAYMAN) {
		r600_store_config_reg(cb, R_008E2C_SQ_LDS_RESOURCE_MGMT,
			S_008E2C_NUM_PS_LDS(0x0000) | S_008E2C_NUM_LS_LDS(8192));
	} else {
		r600_store_context_reg(cb, CM_R_0286FC_SPI_LDS_MGMT,
			S_0286FC_NUM_PS_LDS(0) |
			S_0286FC_NUM_LS_LDS(255)); /* 255 * 32 = 8160 dwords */
	}

	/* Context Registers */

	if (ctx->b.chip_class < CAYMAN) {
		/* workaround for hw issues with dyn gpr - must set all limits
		 * to 240 instead of 0, 0x1e == 240 / 8
		 */
		r600_store_context_reg(cb, R_028838_SQ_DYN_GPR_RESOURCE_LIMIT_1,
				S_028838_PS_GPRS(0x1e) |
				S_028838_VS_GPRS(0x1e) |
				S_028838_GS_GPRS(0x1e) |
				S_028838_ES_GPRS(0x1e) |
				S_028838_HS_GPRS(0x1e) |
				S_028838_LS_GPRS(0x1e));
	}

	/* XXX: Investigate setting bit 15, which is FAST_COMPUTE_MODE */
	r600_store_context_reg(cb, R_028A40_VGT_GS_MODE,
		S_028A40_COMPUTE_MODE(1) | S_028A40_PARTIAL_THD_AT_EOI(1));

	r600_store_context_reg(cb, R_028B54_VGT_SHADER_STAGES_EN, 2/*CS_ON*/);

	r600_store_context_reg(cb, R_0286E8_SPI_COMPUTE_INPUT_CNTL,
						S_0286E8_TID_IN_GROUP_ENA
						| S_0286E8_TGID_ENA
						| S_0286E8_DISABLE_INDEX_PACK)
						;

	/* The LOOP_CONST registers are an optimizations for loops that allows
	 * you to store the initial counter, increment value, and maximum
	 * counter value in a register so that hardware can calculate the
	 * correct number of iterations for the loop, so that you don't need
	 * to have the loop counter in your shader code.  We don't currently use
	 * this optimization, so we must keep track of the counter in the
	 * shader and use a break instruction to exit loops.  However, the
	 * hardware will still uses this register to determine when to exit a
	 * loop, so we need to initialize the counter to 0, set the increment
	 * value to 1 and the maximum counter value to the 4095 (0xfff) which
	 * is the maximum value allowed.  This gives us a maximum of 4096
	 * iterations for our loops, but hopefully our break instruction will
	 * execute before some time before the 4096th iteration.
	 */
	eg_store_loop_const(cb, R_03A200_SQ_LOOP_CONST_0 + (160 * 4), 0x1000FFF);
}

void evergreen_init_compute_state_functions(struct r600_context *ctx)
{
	ctx->b.b.create_compute_state = evergreen_create_compute_state;
	ctx->b.b.delete_compute_state = evergreen_delete_compute_state;
	ctx->b.b.bind_compute_state = evergreen_bind_compute_state;
//	 ctx->context.create_sampler_view = evergreen_compute_create_sampler_view;
	ctx->b.b.set_compute_resources = evergreen_set_compute_resources;
	ctx->b.b.set_global_binding = evergreen_set_global_binding;
	ctx->b.b.launch_grid = evergreen_launch_grid;

}

struct pipe_resource *r600_compute_global_buffer_create(
	struct pipe_screen *screen,
	const struct pipe_resource *templ)
{
	struct r600_resource_global* result = NULL;
	struct r600_screen* rscreen = NULL;
	int size_in_dw = 0;

	assert(templ->target == PIPE_BUFFER);
	assert(templ->bind & PIPE_BIND_GLOBAL);
	assert(templ->array_size == 1 || templ->array_size == 0);
	assert(templ->depth0 == 1 || templ->depth0 == 0);
	assert(templ->height0 == 1 || templ->height0 == 0);

	result = (struct r600_resource_global*)
	CALLOC(sizeof(struct r600_resource_global), 1);
	rscreen = (struct r600_screen*)screen;

	COMPUTE_DBG(rscreen, "*** r600_compute_global_buffer_create\n");
	COMPUTE_DBG(rscreen, "width = %u array_size = %u\n", templ->width0,
			templ->array_size);

	result->base.b.vtbl = &r600_global_buffer_vtbl;
	result->base.b.b.screen = screen;
	result->base.b.b = *templ;
	pipe_reference_init(&result->base.b.b.reference, 1);

	size_in_dw = (templ->width0+3) / 4;

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
	struct r600_resource_global* buffer = NULL;
	struct r600_screen* rscreen = NULL;

	assert(res->target == PIPE_BUFFER);
	assert(res->bind & PIPE_BIND_GLOBAL);

	buffer = (struct r600_resource_global*)res;
	rscreen = (struct r600_screen*)screen;

	compute_memory_free(rscreen->global_pool, buffer->chunk->id);

	buffer->chunk = NULL;
	free(res);
}

void *r600_compute_global_transfer_map(
	struct pipe_context *ctx_,
	struct pipe_resource *resource,
	unsigned level,
	unsigned usage,
	const struct pipe_box *box,
	struct pipe_transfer **ptransfer)
{
	struct r600_context *rctx = (struct r600_context*)ctx_;
	struct compute_memory_pool *pool = rctx->screen->global_pool;
	struct r600_resource_global* buffer =
		(struct r600_resource_global*)resource;

	struct compute_memory_item *item = buffer->chunk;
	struct pipe_resource *dst = NULL;
	unsigned offset = box->x;

	if (is_item_in_pool(item)) {
		compute_memory_demote_item(pool, item, ctx_);
	}
	else {
		if (item->real_buffer == NULL) {
			item->real_buffer = (struct r600_resource*)
					r600_compute_buffer_alloc_vram(pool->screen, item->size_in_dw * 4);
		}
	}

	dst = (struct pipe_resource*)item->real_buffer;

	if (usage & PIPE_TRANSFER_READ)
		buffer->chunk->status |= ITEM_MAPPED_FOR_READING;

	COMPUTE_DBG(rctx->screen, "* r600_compute_global_transfer_map()\n"
			"level = %u, usage = %u, box(x = %u, y = %u, z = %u "
			"width = %u, height = %u, depth = %u)\n", level, usage,
			box->x, box->y, box->z, box->width, box->height,
			box->depth);
	COMPUTE_DBG(rctx->screen, "Buffer id = %"PRIi64" offset = "
		"%u (box.x)\n", item->id, box->x);


	assert(resource->target == PIPE_BUFFER);
	assert(resource->bind & PIPE_BIND_GLOBAL);
	assert(box->x >= 0);
	assert(box->y == 0);
	assert(box->z == 0);

	///TODO: do it better, mapping is not possible if the pool is too big
	return pipe_buffer_map_range(ctx_, dst,
			offset, box->width, usage, ptransfer);
}

void r600_compute_global_transfer_unmap(
	struct pipe_context *ctx_,
	struct pipe_transfer* transfer)
{
	/* struct r600_resource_global are not real resources, they just map
	 * to an offset within the compute memory pool.  The function
	 * r600_compute_global_transfer_map() maps the memory pool
	 * resource rather than the struct r600_resource_global passed to
	 * it as an argument and then initalizes ptransfer->resource with
	 * the memory pool resource (via pipe_buffer_map_range).
	 * When transfer_unmap is called it uses the memory pool's
	 * vtable which calls r600_buffer_transfer_map() rather than
	 * this function.
	 */
	assert (!"This function should not be called");
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
