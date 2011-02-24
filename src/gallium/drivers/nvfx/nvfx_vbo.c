#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_transfer.h"
#include "translate/translate.h"

#include "nvfx_context.h"
#include "nvfx_state.h"
#include "nvfx_resource.h"

#include "nouveau/nouveau_channel.h"
#include "nouveau/nv04_pushbuf.h"

static inline unsigned
util_guess_unique_indices_count(unsigned mode, unsigned indices)
{
	/* Euler's formula gives V =
	 * = E - F + 2 =
	 * = F * (polygon_edges / 2 - 1) + 2 =
	 * =  F * (polygon_edges - 2) / 2 + 2 =
	 * =  indices * (polygon_edges - 2) / (2 * indices_per_face) + 2
	 * =  indices * (1 / 2 - 1 / polygon_edges) + 2
	 */
	switch(mode)
	{
	case PIPE_PRIM_LINES:
		return indices >> 1;
	case PIPE_PRIM_TRIANGLES:
	{
		// avoid an expensive division by 3 using the multiplicative inverse mod 2^32
		unsigned q;
		unsigned inv3 = 2863311531;
		indices >>= 1;
		q = indices * inv3;
		if(unlikely(q >= indices))
		{
			q += inv3;
			if(q >= indices)
				q += inv3;
		}
		return indices + 2;
		//return indices / 6 + 2;
	}
	// guess that indexed quads are created by successive connections, since a closed mesh seems unlikely
	case PIPE_PRIM_QUADS:
		return (indices >> 1) + 2;
	//	return (indices >> 2) + 2; // if it is a closed mesh
	default:
		return indices;
	}
}

static unsigned nvfx_decide_upload_mode(struct pipe_context *pipe, const struct pipe_draw_info *info)
{
	struct nvfx_context* nvfx = nvfx_context(pipe);
	unsigned hardware_cost = 0;
	unsigned inline_cost = 0;
	unsigned unique_vertices;
	unsigned upload_mode;
	float best_index_cost_for_hardware_vertices_as_inline_cost;
	boolean prefer_hardware_indices;
	unsigned index_inline_cost;
	unsigned index_hardware_cost;
	if (info->indexed)
		unique_vertices = util_guess_unique_indices_count(info->mode, info->count);
	else
		unique_vertices = info->count;

	/* Here we try to figure out if we are better off writing vertex data directly on the FIFO,
	 * or create hardware buffer objects and pointing the hardware to them.
	 *
	 * This is done by computing the total memcpy cost of each option, ignoring uploads
	 * if we think that the buffer is static and thus the upload cost will be amortized over
	 * future draw calls.
	 *
	 * For instance, if everything looks static, we will always create buffer objects, while if
	 * everything is a user buffer and we are not doing indexed drawing, we never do.
	 *
	 * Other interesting cases are where a small user vertex buffer, but a huge user index buffer,
	 * where we will upload the vertex buffer, so that we can use hardware index lookup, and
	 * the opposite case, where we instead do index lookup in software to avoid uploading
	 * a huge amount of vertex data that is not going to be used.
	 *
	 * Otherwise, we generally move to the GPU the after it has been pushed
	 * NVFX_STATIC_BUFFER_MIN_REUSE_TIMES times to the GPU without having
	 * been updated with a transfer (or just the buffer having been destroyed).
	 *
	 * There is no special handling for user buffers, since applications can use
	 * OpenGL VBOs in a one-shot fashion. OpenGL 3/4 core profile forces this
	 * by the way.
	 *
	 * Note that currently we don't support only putting some data on the FIFO, and
	 * some on vertex buffers (constant and instanced data is independent from this).
	 *
	 * nVidia doesn't seem to do this either, even though it should be at least
	 * doable with VTX_ATTR and possibly with VERTEX_DATA too if not indexed.
	 */

	for (unsigned i = 0; i < nvfx->vtxelt->num_per_vertex_buffer_infos; i++)
	{
		struct nvfx_per_vertex_buffer_info* vbi = &nvfx->vtxelt->per_vertex_buffer_info[i];
		struct pipe_vertex_buffer *vb = &nvfx->vtxbuf[vbi->vertex_buffer_index];
		struct nvfx_buffer* buffer = nvfx_buffer(vb->buffer);
		buffer->bytes_to_draw_until_static -= vbi->per_vertex_size * unique_vertices;
		if (!nvfx_buffer_seems_static(buffer))
		{
			hardware_cost += buffer->dirty_end - buffer->dirty_begin;
			if (!buffer->base.bo)
				hardware_cost += nvfx->screen->buffer_allocation_cost;
		}
		inline_cost += vbi->per_vertex_size * info->count;
	}

	best_index_cost_for_hardware_vertices_as_inline_cost = 0.0f;
	prefer_hardware_indices = FALSE;
	index_inline_cost = 0;
	index_hardware_cost = 0;

	if (info->indexed)
	{
		index_inline_cost = nvfx->idxbuf.index_size * info->count;
		if (nvfx->screen->index_buffer_reloc_flags
			&& (nvfx->idxbuf.index_size == 2 || nvfx->idxbuf.index_size == 4)
			&& !(nvfx->idxbuf.offset & (nvfx->idxbuf.index_size - 1)))
		{
			struct nvfx_buffer* buffer = nvfx_buffer(nvfx->idxbuf.buffer);
			buffer->bytes_to_draw_until_static -= index_inline_cost;

			prefer_hardware_indices = TRUE;

			if (!nvfx_buffer_seems_static(buffer))
			{
				index_hardware_cost = buffer->dirty_end - buffer->dirty_begin;
				if (!buffer->base.bo)
					index_hardware_cost += nvfx->screen->buffer_allocation_cost;
			}

			if ((float) index_inline_cost < (float) index_hardware_cost * nvfx->screen->inline_cost_per_hardware_cost)
			{
				best_index_cost_for_hardware_vertices_as_inline_cost = (float) index_inline_cost;
			}
			else
			{
				best_index_cost_for_hardware_vertices_as_inline_cost = (float) index_hardware_cost * nvfx->screen->inline_cost_per_hardware_cost;
				prefer_hardware_indices = TRUE;
			}
		}
	}

	/* let's finally figure out which of the 3 paths we want to take */
	if ((float) (inline_cost + index_inline_cost) > ((float) hardware_cost * nvfx->screen->inline_cost_per_hardware_cost + best_index_cost_for_hardware_vertices_as_inline_cost))
		upload_mode = 1 + prefer_hardware_indices;
	else
		upload_mode = 0;

#ifdef DEBUG
        if (unlikely(nvfx->screen->trace_draw))
          {
                  fprintf(stderr, "DRAW");
                  if (info->indexed)
                  {
                          fprintf(stderr, "_IDX%u", nvfx->idxbuf.index_size);
                          if (info->index_bias)
                                  fprintf(stderr, " biased %u", info->index_bias);
                          fprintf(stderr, " idxrange %u -> %u", info->min_index, info->max_index);
                  }
                  if (info->instance_count > 1)
                          fprintf(stderr, " %u instances from %u", info->instance_count, info->indexed);
                  fprintf(stderr, " start %u count %u prim %u", info->start, info->count, info->mode);
                  if (!upload_mode)
                          fprintf(stderr, " -> inline vertex data");
                  else if (upload_mode == 2 || !info->indexed)
                          fprintf(stderr, " -> buffer range");
                  else
                          fprintf(stderr, " -> inline indices");
                  fprintf(stderr, " [ivtx %u hvtx %u iidx %u hidx %u bidx %f] <", inline_cost, hardware_cost, index_inline_cost, index_hardware_cost, best_index_cost_for_hardware_vertices_as_inline_cost);
                  for (unsigned i = 0; i < nvfx->vtxelt->num_per_vertex_buffer_infos; ++i)
                  {
                          struct nvfx_per_vertex_buffer_info* vbi = &nvfx->vtxelt->per_vertex_buffer_info[i];
                          struct pipe_vertex_buffer *vb = &nvfx->vtxbuf[vbi->vertex_buffer_index];
                          struct nvfx_buffer* buffer = nvfx_buffer(vb->buffer);
                          if (i)
                                  fprintf(stderr, ", ");
                          fprintf(stderr, "%p%s left %Li", buffer, buffer->last_update_static ? " static" : "", buffer->bytes_to_draw_until_static);
                  }
                  fprintf(stderr, ">\n");
          }
#endif

	return upload_mode;
}

void nvfx_draw_vbo(struct pipe_context *pipe, const struct pipe_draw_info *info)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	unsigned upload_mode = 0;

	if (!nvfx->vtxelt->needs_translate)
		upload_mode = nvfx_decide_upload_mode(pipe, info);

	nvfx->use_index_buffer = upload_mode > 1;

	if ((upload_mode > 0) != nvfx->use_vertex_buffers)
	{
		nvfx->use_vertex_buffers = (upload_mode > 0);
		nvfx->dirty |= NVFX_NEW_ARRAYS;
		nvfx->draw_dirty |= NVFX_NEW_ARRAYS;
	}

	if (upload_mode > 0)
	{
		for (unsigned i = 0; i < nvfx->vtxelt->num_per_vertex_buffer_infos; i++)
		{
			struct nvfx_per_vertex_buffer_info* vbi = &nvfx->vtxelt->per_vertex_buffer_info[i];
			struct pipe_vertex_buffer *vb = &nvfx->vtxbuf[vbi->vertex_buffer_index];
			nvfx_buffer_upload(nvfx_buffer(vb->buffer));
		}

		if (upload_mode > 1)
		{
			nvfx_buffer_upload(nvfx_buffer(nvfx->idxbuf.buffer));

			if (unlikely(info->index_bias != nvfx->base_vertex))
			{
				nvfx->base_vertex = info->index_bias;
				nvfx->dirty |= NVFX_NEW_ARRAYS;
			}
		}
		else
		{
			if (unlikely(info->start < nvfx->base_vertex && nvfx->base_vertex))
			{
				nvfx->base_vertex = 0;
				nvfx->dirty |= NVFX_NEW_ARRAYS;
			}
		}
	}

	if (nvfx->screen->force_swtnl || !nvfx_state_validate(nvfx))
		nvfx_draw_vbo_swtnl(pipe, info);
	else
		nvfx_push_vbo(pipe, info);
}

boolean
nvfx_vbo_validate(struct nvfx_context *nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	struct nouveau_grobj *eng3d = nvfx->screen->eng3d;
	int i;
	int elements = MAX2(nvfx->vtxelt->num_elements, nvfx->hw_vtxelt_nr);
	unsigned vb_flags = nvfx->screen->vertex_buffer_reloc_flags | NOUVEAU_BO_RD;

	if (!elements)
		return TRUE;

	MARK_RING(chan, (5 + 2) * 16 + 2 + 11, 16 + 2);
	for(unsigned i = 0; i < nvfx->vtxelt->num_constant; ++i)
	{
		struct nvfx_low_frequency_element *ve = &nvfx->vtxelt->constant[i];
		struct pipe_vertex_buffer *vb = &nvfx->vtxbuf[ve->vertex_buffer_index];
		struct nvfx_buffer* buffer = nvfx_buffer(vb->buffer);
		float v[4];
		ve->fetch_rgba_float(v, buffer->data + vb->buffer_offset + ve->src_offset, 0, 0);
		nvfx_emit_vtx_attr(chan, eng3d, ve->idx, v, ve->ncomp);
	}


	BEGIN_RING(chan, eng3d, NV30_3D_VTXFMT(0), elements);
	if(nvfx->use_vertex_buffers)
	{
		unsigned idx = 0;
		for (i = 0; i < nvfx->vtxelt->num_per_vertex; i++) {
			struct nvfx_per_vertex_element *ve = &nvfx->vtxelt->per_vertex[i];
			struct pipe_vertex_buffer *vb = &nvfx->vtxbuf[ve->vertex_buffer_index];

			if(idx != ve->idx)
			{
				assert(idx < ve->idx);
				OUT_RINGp(chan, &nvfx->vtxelt->vtxfmt[idx], ve->idx - idx);
				idx = ve->idx;
			}

			OUT_RING(chan, nvfx->vtxelt->vtxfmt[idx] | (vb->stride << NV30_3D_VTXFMT_STRIDE__SHIFT));
			++idx;
		}
		if(idx != nvfx->vtxelt->num_elements)
			OUT_RINGp(chan, &nvfx->vtxelt->vtxfmt[idx], nvfx->vtxelt->num_elements - idx);
	}
	else
		OUT_RINGp(chan, nvfx->vtxelt->vtxfmt, nvfx->vtxelt->num_elements);

	for(i = nvfx->vtxelt->num_elements; i < elements; ++i)
		OUT_RING(chan, NV30_3D_VTXFMT_TYPE_V32_FLOAT);

	if(nvfx->is_nv4x) {
		unsigned i;
		/* seems to be some kind of cache flushing */
		for(i = 0; i < 3; ++i) {
			BEGIN_RING(chan, eng3d, 0x1718, 1);
			OUT_RING(chan, 0);
		}
	}

	BEGIN_RING(chan, eng3d, NV30_3D_VTXBUF(0), elements);
	if(nvfx->use_vertex_buffers)
	{
		unsigned idx = 0;
		for (i = 0; i < nvfx->vtxelt->num_per_vertex; i++) {
			struct nvfx_per_vertex_element *ve = &nvfx->vtxelt->per_vertex[i];
			struct pipe_vertex_buffer *vb = &nvfx->vtxbuf[ve->vertex_buffer_index];
			struct nouveau_bo* bo = nvfx_resource(vb->buffer)->bo;

			for(; idx < ve->idx; ++idx)
				OUT_RING(chan, 0);

			OUT_RELOC(chan, bo,
					vb->buffer_offset + ve->src_offset + nvfx->base_vertex * vb->stride,
					vb_flags | NOUVEAU_BO_LOW | NOUVEAU_BO_OR,
					0, NV30_3D_VTXBUF_DMA1);
			++idx;
		}

		for(; idx < elements; ++idx)
			OUT_RING(chan, 0);
	}
	else
	{
		for (i = 0; i < elements; i++)
			OUT_RING(chan, 0);
	}

	BEGIN_RING(chan, eng3d, 0x1710, 1);
	OUT_RING(chan, 0);

	nvfx->hw_vtxelt_nr = nvfx->vtxelt->num_elements;
	nvfx->relocs_needed &=~ NVFX_RELOCATE_VTXBUF;
	return TRUE;
}

void
nvfx_vbo_swtnl_validate(struct nvfx_context *nvfx)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	struct nouveau_grobj *eng3d = nvfx->screen->eng3d;
	unsigned num_outputs = nvfx->vertprog->draw_elements;
	int elements = MAX2(num_outputs, nvfx->hw_vtxelt_nr);

	if (!elements)
		return;

	BEGIN_RING(chan, eng3d, NV30_3D_VTXFMT(0), elements);
	for(unsigned i = 0; i < num_outputs; ++i)
		OUT_RING(chan, (4 << NV30_3D_VTXFMT_SIZE__SHIFT) | NV30_3D_VTXFMT_TYPE_V32_FLOAT);
	for(unsigned i = num_outputs; i < elements; ++i)
		OUT_RING(chan, NV30_3D_VTXFMT_TYPE_V32_FLOAT);

	if(nvfx->is_nv4x) {
		unsigned i;
		/* seems to be some kind of cache flushing */
		for(i = 0; i < 3; ++i) {
			BEGIN_RING(chan, eng3d, 0x1718, 1);
			OUT_RING(chan, 0);
		}
	}

	BEGIN_RING(chan, eng3d, NV30_3D_VTXBUF(0), elements);
	for (unsigned i = 0; i < elements; i++)
		OUT_RING(chan, 0);

	BEGIN_RING(chan, eng3d, 0x1710, 1);
	OUT_RING(chan, 0);

	nvfx->hw_vtxelt_nr = num_outputs;
	nvfx->relocs_needed &=~ NVFX_RELOCATE_VTXBUF;
}

void
nvfx_vbo_relocate(struct nvfx_context *nvfx)
{
	struct nouveau_channel* chan;
	unsigned vb_flags;
	int i;

        if(!nvfx->use_vertex_buffers)
                return;

	chan = nvfx->screen->base.channel;
	vb_flags = nvfx->screen->vertex_buffer_reloc_flags | NOUVEAU_BO_RD | NOUVEAU_BO_DUMMY;

	MARK_RING(chan, 2 * 16 + 3, 2 * 16 + 3);
        for (i = 0; i < nvfx->vtxelt->num_per_vertex; i++) {
                struct nvfx_per_vertex_element *ve = &nvfx->vtxelt->per_vertex[i];
                struct pipe_vertex_buffer *vb = &nvfx->vtxbuf[ve->vertex_buffer_index];
                struct nouveau_bo* bo = nvfx_resource(vb->buffer)->bo;

                OUT_RELOC(chan, bo, RING_3D(NV30_3D_VTXBUF(ve->idx), 1),
				vb_flags, 0, 0);
                OUT_RELOC(chan, bo, vb->buffer_offset + ve->src_offset + nvfx->base_vertex * vb->stride,
				vb_flags | NOUVEAU_BO_LOW | NOUVEAU_BO_OR,
				0, NV30_3D_VTXBUF_DMA1);
	}
        nvfx->relocs_needed &=~ NVFX_RELOCATE_VTXBUF;
}

static void
nvfx_idxbuf_emit(struct nvfx_context* nvfx, unsigned ib_flags)
{
	struct nouveau_channel* chan = nvfx->screen->base.channel;
	unsigned ib_format = (nvfx->idxbuf.index_size == 2) ? NV30_3D_IDXBUF_FORMAT_TYPE_U16 : NV30_3D_IDXBUF_FORMAT_TYPE_U32;
	struct nouveau_bo* bo = nvfx_resource(nvfx->idxbuf.buffer)->bo;
	ib_flags |= nvfx->screen->index_buffer_reloc_flags | NOUVEAU_BO_RD;

	assert(nvfx->screen->index_buffer_reloc_flags);

	MARK_RING(chan, 3, 3);
	if(ib_flags & NOUVEAU_BO_DUMMY)
		OUT_RELOC(chan, bo, RING_3D(NV30_3D_IDXBUF_OFFSET, 2), ib_flags, 0, 0);
	else
		OUT_RING(chan, RING_3D(NV30_3D_IDXBUF_OFFSET, 2));
	OUT_RELOC(chan, bo, nvfx->idxbuf.offset + 1, ib_flags | NOUVEAU_BO_LOW, 0, 0);
	OUT_RELOC(chan, bo, ib_format, ib_flags | NOUVEAU_BO_OR,
			0, NV30_3D_IDXBUF_FORMAT_DMA1);
	nvfx->relocs_needed &=~ NVFX_RELOCATE_IDXBUF;
}

void
nvfx_idxbuf_validate(struct nvfx_context* nvfx)
{
	nvfx_idxbuf_emit(nvfx, 0);
}

void
nvfx_idxbuf_relocate(struct nvfx_context* nvfx)
{
	nvfx_idxbuf_emit(nvfx, NOUVEAU_BO_DUMMY);
}

unsigned nvfx_vertex_formats[PIPE_FORMAT_COUNT] =
{
	[PIPE_FORMAT_R32_FLOAT] = NV30_3D_VTXFMT_TYPE_V32_FLOAT,
	[PIPE_FORMAT_R32G32_FLOAT] = NV30_3D_VTXFMT_TYPE_V32_FLOAT,
	[PIPE_FORMAT_R32G32B32_FLOAT] = NV30_3D_VTXFMT_TYPE_V32_FLOAT,
	[PIPE_FORMAT_R32G32B32A32_FLOAT] = NV30_3D_VTXFMT_TYPE_V32_FLOAT,
	[PIPE_FORMAT_R16_FLOAT] = NV30_3D_VTXFMT_TYPE_V16_FLOAT,
	[PIPE_FORMAT_R16G16_FLOAT] = NV30_3D_VTXFMT_TYPE_V16_FLOAT,
	[PIPE_FORMAT_R16G16B16_FLOAT] = NV30_3D_VTXFMT_TYPE_V16_FLOAT,
	[PIPE_FORMAT_R16G16B16A16_FLOAT] = NV30_3D_VTXFMT_TYPE_V16_FLOAT,
	[PIPE_FORMAT_R8_UNORM] = NV30_3D_VTXFMT_TYPE_U8_UNORM,
	[PIPE_FORMAT_R8G8_UNORM] = NV30_3D_VTXFMT_TYPE_U8_UNORM,
	[PIPE_FORMAT_R8G8B8_UNORM] = NV30_3D_VTXFMT_TYPE_U8_UNORM,
	[PIPE_FORMAT_R8G8B8A8_UNORM] = NV30_3D_VTXFMT_TYPE_U8_UNORM,
	[PIPE_FORMAT_R8G8B8A8_USCALED] = NV30_3D_VTXFMT_TYPE_U8_USCALED,
	[PIPE_FORMAT_R16_SNORM] = NV30_3D_VTXFMT_TYPE_V16_SNORM,
	[PIPE_FORMAT_R16G16_SNORM] = NV30_3D_VTXFMT_TYPE_V16_SNORM,
	[PIPE_FORMAT_R16G16B16_SNORM] = NV30_3D_VTXFMT_TYPE_V16_SNORM,
	[PIPE_FORMAT_R16G16B16A16_SNORM] = NV30_3D_VTXFMT_TYPE_V16_SNORM,
	[PIPE_FORMAT_R16_SSCALED] = NV30_3D_VTXFMT_TYPE_V16_SSCALED,
	[PIPE_FORMAT_R16G16_SSCALED] = NV30_3D_VTXFMT_TYPE_V16_SSCALED,
	[PIPE_FORMAT_R16G16B16_SSCALED] = NV30_3D_VTXFMT_TYPE_V16_SSCALED,
	[PIPE_FORMAT_R16G16B16A16_SSCALED] = NV30_3D_VTXFMT_TYPE_V16_SSCALED,
};

static void *
nvfx_vtxelts_state_create(struct pipe_context *pipe,
			  unsigned num_elements,
			  const struct pipe_vertex_element *elements)
{
	struct nvfx_vtxelt_state *cso = CALLOC_STRUCT(nvfx_vtxelt_state);
	struct translate_key transkey;
	unsigned per_vertex_size[16];
	unsigned vb_compacted_index[16];

	if(num_elements > 16)
	{
		_debug_printf("Error: application attempted to use %u vertex elements, but only 16 are supported: ignoring the rest\n", num_elements);
		num_elements = 16;
	}

	memset(per_vertex_size, 0, sizeof(per_vertex_size));
	memcpy(cso->pipe, elements, num_elements * sizeof(elements[0]));
	cso->num_elements = num_elements;
	cso->needs_translate = FALSE;

	transkey.nr_elements = 0;
	transkey.output_stride = 0;

	for(unsigned i = 0; i < num_elements; ++i)
        {
		const struct pipe_vertex_element* ve = &elements[i];
		if(!ve->instance_divisor)
                        per_vertex_size[ve->vertex_buffer_index] += util_format_get_stride(ve->src_format, 1);
        }

        for(unsigned i = 0; i < 16; ++i)
        {
                if(per_vertex_size[i])
                {
                        unsigned idx = cso->num_per_vertex_buffer_infos++;
                        cso->per_vertex_buffer_info[idx].vertex_buffer_index = i;
                        cso->per_vertex_buffer_info[idx].per_vertex_size = per_vertex_size[i];
                        vb_compacted_index[i] = idx;
                }
        }

	for(unsigned i = 0; i < num_elements; ++i)
	{
		const struct pipe_vertex_element* ve = &elements[i];
		unsigned type = nvfx_vertex_formats[ve->src_format];
		unsigned ncomp = util_format_get_nr_components(ve->src_format);

		//if(ve->frequency != PIPE_ELEMENT_FREQUENCY_PER_VERTEX)
		if(ve->instance_divisor)
		{
			struct nvfx_low_frequency_element* lfve;
			cso->vtxfmt[i] = NV30_3D_VTXFMT_TYPE_V32_FLOAT;

			//if(ve->frequency == PIPE_ELEMENT_FREQUENCY_CONSTANT)
			if(0)
				lfve = &cso->constant[cso->num_constant++];
			else
			{
				lfve = &cso->per_instance[cso->num_per_instance++].base;
				((struct nvfx_per_instance_element*)lfve)->instance_divisor = ve->instance_divisor;
			}

                        lfve->idx = i;
                        lfve->vertex_buffer_index = ve->vertex_buffer_index;
                        lfve->src_offset = ve->src_offset;
                        lfve->fetch_rgba_float = util_format_description(ve->src_format)->fetch_rgba_float;
                        lfve->ncomp = ncomp;
		}
		else
		{
			unsigned idx;

			idx = cso->num_per_vertex++;
			cso->per_vertex[idx].idx = i;
			cso->per_vertex[idx].vertex_buffer_index = ve->vertex_buffer_index;
			cso->per_vertex[idx].src_offset = ve->src_offset;

			idx = transkey.nr_elements++;
			transkey.element[idx].input_format = ve->src_format;
			transkey.element[idx].input_buffer = vb_compacted_index[ve->vertex_buffer_index];
			transkey.element[idx].input_offset = ve->src_offset;
			transkey.element[idx].instance_divisor = 0;
			transkey.element[idx].type = TRANSLATE_ELEMENT_NORMAL;
			if(type)
			{
				transkey.element[idx].output_format = ve->src_format;
				cso->vtxfmt[i] = (ncomp << NV30_3D_VTXFMT_SIZE__SHIFT) | type;
			}
			else
			{
				unsigned float32[4] = {PIPE_FORMAT_R32_FLOAT, PIPE_FORMAT_R32G32_FLOAT, PIPE_FORMAT_R32G32B32_FLOAT, PIPE_FORMAT_R32G32B32A32_FLOAT};
				transkey.element[idx].output_format = float32[ncomp - 1];
				cso->needs_translate = TRUE;
				cso->vtxfmt[i] = (ncomp << NV30_3D_VTXFMT_SIZE__SHIFT) | NV30_3D_VTXFMT_TYPE_V32_FLOAT;
			}
			transkey.element[idx].output_offset = transkey.output_stride;
			transkey.output_stride += (util_format_get_stride(transkey.element[idx].output_format, 1) + 3) & ~3;
		}
	}

	cso->translate = translate_create(&transkey);
	cso->vertex_length = transkey.output_stride >> 2;
	cso->max_vertices_per_packet = 2047 / MAX2(cso->vertex_length, 1);

	return (void *)cso;
}

static void
nvfx_vtxelts_state_delete(struct pipe_context *pipe, void *hwcso)
{
	FREE(hwcso);
}

static void
nvfx_vtxelts_state_bind(struct pipe_context *pipe, void *hwcso)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);

	nvfx->vtxelt = hwcso;
	nvfx->use_vertex_buffers = -1;
	nvfx->draw_dirty |= NVFX_NEW_ARRAYS;
}

static void
nvfx_set_vertex_buffers(struct pipe_context *pipe, unsigned count,
			const struct pipe_vertex_buffer *vb)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);

        util_copy_vertex_buffers(nvfx->vtxbuf,
                                 &nvfx->vtxbuf_nr,
                                 vb, count);

	nvfx->use_vertex_buffers = -1;
	nvfx->draw_dirty |= NVFX_NEW_ARRAYS;
}

static void
nvfx_set_index_buffer(struct pipe_context *pipe,
		      const struct pipe_index_buffer *ib)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);

	if(ib)
	{
		pipe_resource_reference(&nvfx->idxbuf.buffer, ib->buffer);
		nvfx->idxbuf.index_size = ib->index_size;
		nvfx->idxbuf.offset = ib->offset;
	}
	else
	{
		pipe_resource_reference(&nvfx->idxbuf.buffer, 0);
		nvfx->idxbuf.index_size = 0;
		nvfx->idxbuf.offset = 0;
	}

	nvfx->dirty |= NVFX_NEW_INDEX;
	nvfx->draw_dirty |= NVFX_NEW_INDEX;
}

void
nvfx_init_vbo_functions(struct nvfx_context *nvfx)
{
	nvfx->pipe.set_vertex_buffers = nvfx_set_vertex_buffers;
	nvfx->pipe.set_index_buffer = nvfx_set_index_buffer;

	nvfx->pipe.create_vertex_elements_state = nvfx_vtxelts_state_create;
	nvfx->pipe.delete_vertex_elements_state = nvfx_vtxelts_state_delete;
	nvfx->pipe.bind_vertex_elements_state = nvfx_vtxelts_state_bind;

	nvfx->pipe.redefine_user_buffer = u_default_redefine_user_buffer;
}
