#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_split_prim.h"
#include "translate/translate.h"

#include "nvfx_context.h"
#include "nvfx_resource.h"

struct push_context {
	struct nouveau_channel* chan;
	struct nouveau_grobj *eng3d;

	void *idxbuf;
	int32_t idxbias;

	float edgeflag;
	int edgeflag_attr;

	unsigned vertex_length;
	unsigned max_vertices_per_packet;

	struct translate* translate;
};

static void
emit_edgeflag(void *priv, boolean enabled)
{
	struct push_context* ctx = priv;
	struct nouveau_grobj *eng3d = ctx->eng3d;
	struct nouveau_channel *chan = ctx->chan;

	BEGIN_RING(chan, eng3d, NV30_3D_EDGEFLAG, 1);
	OUT_RING(chan, enabled ? 1 : 0);
}

static void
emit_vertices_lookup8(void *priv, unsigned start, unsigned count)
{
        struct push_context *ctx = priv;
        struct nouveau_grobj *eng3d = ctx->eng3d;
        uint8_t* elts = (uint8_t*)ctx->idxbuf + start;

        while(count)
        {
                unsigned push = MIN2(count, ctx->max_vertices_per_packet);
                unsigned length = push * ctx->vertex_length;

                BEGIN_RING_NI(ctx->chan, eng3d, NV30_3D_VERTEX_DATA, length);
                ctx->translate->run_elts8(ctx->translate, elts, push, 0, ctx->chan->cur);
                ctx->chan->cur += length;

                count -= push;
                elts += push;
        }
}

static void
emit_vertices_lookup16(void *priv, unsigned start, unsigned count)
{
	struct push_context *ctx = priv;
	struct nouveau_grobj *eng3d = ctx->eng3d;
        uint16_t* elts = (uint16_t*)ctx->idxbuf + start;

        while(count)
        {
                unsigned push = MIN2(count, ctx->max_vertices_per_packet);
                unsigned length = push * ctx->vertex_length;

                BEGIN_RING_NI(ctx->chan, eng3d, NV30_3D_VERTEX_DATA, length);
                ctx->translate->run_elts16(ctx->translate, elts, push, 0, ctx->chan->cur);
                ctx->chan->cur += length;

                count -= push;
                elts += push;
        }
}

static void
emit_vertices_lookup32(void *priv, unsigned start, unsigned count)
{
        struct push_context *ctx = priv;
        struct nouveau_grobj *eng3d = ctx->eng3d;
        uint32_t* elts = (uint32_t*)ctx->idxbuf + start;

        while(count)
        {
                unsigned push = MIN2(count, ctx->max_vertices_per_packet);
                unsigned length = push * ctx->vertex_length;

                BEGIN_RING_NI(ctx->chan, eng3d, NV30_3D_VERTEX_DATA, length);
                ctx->translate->run_elts(ctx->translate, elts, push, 0, ctx->chan->cur);
                ctx->chan->cur += length;

                count -= push;
                elts += push;
        }
}

static void
emit_vertices(void *priv, unsigned start, unsigned count)
{
        struct push_context *ctx = priv;
        struct nouveau_grobj *eng3d = ctx->eng3d;

        while(count)
        {
		unsigned push = MIN2(count, ctx->max_vertices_per_packet);
		unsigned length = push * ctx->vertex_length;

		BEGIN_RING_NI(ctx->chan, eng3d, NV30_3D_VERTEX_DATA, length);
		ctx->translate->run(ctx->translate, start, push, 0, ctx->chan->cur);
		ctx->chan->cur += length;

		count -= push;
		start += push;
        }
}

static void
emit_ranges(void* priv, unsigned start, unsigned vc, unsigned reg)
{
	struct push_context* ctx = priv;
	struct nouveau_grobj *eng3d = ctx->eng3d;
	struct nouveau_channel *chan = ctx->chan;
	unsigned nr = (vc & 0xff);
	if (nr) {
		BEGIN_RING(chan, eng3d, reg, 1);
		OUT_RING  (chan, ((nr - 1) << 24) | start);
		start += nr;
	}

	nr = vc >> 8;
	while (nr) {
		unsigned push = nr > 2047 ? 2047 : nr;

		nr -= push;

		BEGIN_RING_NI(chan, eng3d, reg, push);
		while (push--) {
			OUT_RING(chan, ((0x100 - 1) << 24) | start);
			start += 0x100;
		}
	}
}

static void
emit_ib_ranges(void* priv, unsigned start, unsigned vc)
{
	emit_ranges(priv, start, vc, NV30_3D_VB_INDEX_BATCH);
}

static void
emit_vb_ranges(void* priv, unsigned start, unsigned vc)
{
	emit_ranges(priv, start, vc, NV30_3D_VB_VERTEX_BATCH);
}

static INLINE void
emit_elt8(void* priv, unsigned start, unsigned vc)
{
	struct push_context* ctx = priv;
	struct nouveau_grobj *eng3d = ctx->eng3d;
	struct nouveau_channel *chan = ctx->chan;
	uint8_t *elts = (uint8_t *)ctx->idxbuf + start;
	int idxbias = ctx->idxbias;

	if (vc & 1) {
		BEGIN_RING(chan, eng3d, NV30_3D_VB_ELEMENT_U32, 1);
		OUT_RING  (chan, elts[0]);
		elts++; vc--;
	}

	while (vc) {
		unsigned i;
		unsigned push = MIN2(vc, 2047 * 2);

		BEGIN_RING_NI(chan, eng3d, NV30_3D_VB_ELEMENT_U16, push >> 1);
		for (i = 0; i < push; i+=2)
			OUT_RING(chan, ((elts[i+1] + idxbias) << 16) | (elts[i] + idxbias));

		vc -= push;
		elts += push;
	}
}

static INLINE void
emit_elt16(void* priv, unsigned start, unsigned vc)
{
	struct push_context* ctx = priv;
	struct nouveau_grobj *eng3d = ctx->eng3d;
	struct nouveau_channel *chan = ctx->chan;
	uint16_t *elts = (uint16_t *)ctx->idxbuf + start;
	int idxbias = ctx->idxbias;

	if (vc & 1) {
		BEGIN_RING(chan, eng3d, NV30_3D_VB_ELEMENT_U32, 1);
		OUT_RING  (chan, elts[0]);
		elts++; vc--;
	}

	while (vc) {
		unsigned i;
		unsigned push = MIN2(vc, 2047 * 2);

		BEGIN_RING_NI(chan, eng3d, NV30_3D_VB_ELEMENT_U16, push >> 1);
		for (i = 0; i < push; i+=2)
			OUT_RING(chan, ((elts[i+1] + idxbias) << 16) | (elts[i] + idxbias));

		vc -= push;
		elts += push;
	}
}

static INLINE void
emit_elt32(void* priv, unsigned start, unsigned vc)
{
	struct push_context* ctx = priv;
	struct nouveau_grobj *eng3d = ctx->eng3d;
	struct nouveau_channel *chan = ctx->chan;
	uint32_t *elts = (uint32_t *)ctx->idxbuf + start;
	int idxbias = ctx->idxbias;

	while (vc) {
		unsigned push = MIN2(vc, 2047);

		BEGIN_RING_NI(chan, eng3d, NV30_3D_VB_ELEMENT_U32, push);
		if(idxbias)
		{
			for(unsigned i = 0; i < push; ++i)
				OUT_RING(chan, elts[i] + idxbias);
		}
		else
			OUT_RINGp(chan, elts, push);

		vc -= push;
		elts += push;
	}
}

void
nvfx_push_vbo(struct pipe_context *pipe, const struct pipe_draw_info *info)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	struct nouveau_channel *chan = nvfx->screen->base.channel;
	struct nouveau_grobj *eng3d = nvfx->screen->eng3d;
	struct push_context ctx;
	struct util_split_prim s;
	unsigned instances_left = info->instance_count;
	int vtx_value;
	unsigned hw_mode = nvgl_primitive(info->mode);
	int i;
	struct
	{
		uint8_t* map;
		unsigned step;
	} per_instance[16];
	unsigned p_overhead = 64 /* magic fix */
			+ 4 /* begin/end */
			+ 4; /* potential edgeflag enable/disable */

	ctx.chan = nvfx->screen->base.channel;
	ctx.eng3d = nvfx->screen->eng3d;
	ctx.translate = nvfx->vtxelt->translate;
	ctx.idxbuf = NULL;
	ctx.vertex_length = nvfx->vtxelt->vertex_length;
	ctx.max_vertices_per_packet = nvfx->vtxelt->max_vertices_per_packet;
	ctx.edgeflag = 0.5f;
	// TODO: figure out if we really want to handle this, and do so in that case
	ctx.edgeflag_attr = 0xff; // nvfx->vertprog->cfg.edgeflag_in;

	if(!nvfx->use_vertex_buffers)
	{
		for(i = 0; i < nvfx->vtxelt->num_per_vertex_buffer_infos; ++i)
		{
			struct nvfx_per_vertex_buffer_info* vbi = &nvfx->vtxelt->per_vertex_buffer_info[i];
			struct pipe_vertex_buffer *vb = &nvfx->vtxbuf[vbi->vertex_buffer_index];
			uint8_t* data = nvfx_buffer(vb->buffer)->data + vb->buffer_offset;
			if(info->indexed)
				data += info->index_bias * vb->stride;
			ctx.translate->set_buffer(ctx.translate, i, data, vb->stride, ~0);
		}

		if(ctx.edgeflag_attr < 16)
			vtx_value = -(ctx.vertex_length + 3);  /* vertex data and edgeflag header and value */
		else
		{
			p_overhead += 1; /* initial vertex_data header */
			vtx_value = -ctx.vertex_length;  /* vertex data and edgeflag header and value */
		}

		if (info->indexed) {
			// XXX: this case and is broken and probably need a new VTX_ATTR push path
			if (nvfx->idxbuf.index_size == 1)
				s.emit = emit_vertices_lookup8;
			else if (nvfx->idxbuf.index_size == 2)
				s.emit = emit_vertices_lookup16;
			else
				s.emit = emit_vertices_lookup32;
		} else
			s.emit = emit_vertices;
	}
	else
	{
		if(!info->indexed || nvfx->use_index_buffer)
		{
			s.emit = info->indexed ? emit_ib_ranges : emit_vb_ranges;
			p_overhead += 3;
			vtx_value = 0;
		}
		else if (nvfx->idxbuf.index_size == 4)
		{
			s.emit = emit_elt32;
			p_overhead += 1;
			vtx_value = 8;
		}
		else
		{
			s.emit = (nvfx->idxbuf.index_size == 2) ? emit_elt16 : emit_elt8;
			p_overhead += 3;
			vtx_value = 7;
		}
	}

	ctx.idxbias = info->index_bias;
	if(nvfx->use_vertex_buffers)
		ctx.idxbias -= nvfx->base_vertex;

	/* map index buffer, if present */
	if (info->indexed && !nvfx->use_index_buffer)
		ctx.idxbuf = nvfx_buffer(nvfx->idxbuf.buffer)->data + nvfx->idxbuf.offset;

	s.priv = &ctx;
	s.edge = emit_edgeflag;

	for (i = 0; i < nvfx->vtxelt->num_per_instance; ++i)
	{
		struct nvfx_per_instance_element *ve = &nvfx->vtxelt->per_instance[i];
		struct pipe_vertex_buffer *vb = &nvfx->vtxbuf[ve->base.vertex_buffer_index];
		float v[4];
		per_instance[i].step = info->start_instance % ve->instance_divisor;
		per_instance[i].map = nvfx_buffer(vb->buffer)->data + vb->buffer_offset + ve->base.src_offset;

		nvfx->vtxelt->per_instance[i].base.fetch_rgba_float(v, per_instance[i].map, 0, 0);

		nvfx_emit_vtx_attr(chan, eng3d,
				   nvfx->vtxelt->per_instance[i].base.idx, v,
				   nvfx->vtxelt->per_instance[i].base.ncomp);
	}

	/* per-instance loop */
	while (instances_left--) {
		int max_verts;
		boolean done;

		util_split_prim_init(&s, info->mode, info->start, info->count);
		nvfx_state_emit(nvfx);
		for(;;) {
			max_verts  = AVAIL_RING(chan);
			max_verts -= p_overhead;

			/* if vtx_value < 0, each vertex is -vtx_value words long
			 * otherwise, each vertex is 2^(vtx_value) / 255 words long (this is an approximation)
			 */
			if(vtx_value < 0)
			{
				max_verts /= -vtx_value;
				max_verts -= (max_verts >> 10); /* vertex data headers */
			}
			else
			{
				if(max_verts >= (1 << 23)) /* avoid overflow here */
					max_verts = (1 << 23);
				max_verts = (max_verts * 255) >> vtx_value;
			}

			//printf("avail %u max_verts %u\n", AVAIL_RING(chan), max_verts);

			if(max_verts >= 16)
			{
				/* XXX: any command a lot of times seems to (mostly) fix corruption that would otherwise happen */
				/* this seems to cause issues on nv3x, and also be unneeded there */
				if(nvfx->is_nv4x)
				{
					int i;
					for(i = 0; i < 32; ++i)
					{
						BEGIN_RING(chan, eng3d,
							   0x1dac, 1);
						OUT_RING(chan, 0);
					}
				}

				BEGIN_RING(chan, eng3d,
					   NV30_3D_VERTEX_BEGIN_END, 1);
				OUT_RING(chan, hw_mode);
				done = util_split_prim_next(&s, max_verts);
				BEGIN_RING(chan, eng3d,
					   NV30_3D_VERTEX_BEGIN_END, 1);
				OUT_RING(chan, 0);

				if(done)
					break;
			}

			FIRE_RING(chan);
			nvfx_state_emit(nvfx);
		}

		/* set data for the next instance, if any changed */
		for (i = 0; i < nvfx->vtxelt->num_per_instance; ++i)
		{
			struct nvfx_per_instance_element *ve = &nvfx->vtxelt->per_instance[i];
			struct pipe_vertex_buffer *vb = &nvfx->vtxbuf[ve->base.vertex_buffer_index];

			if(++per_instance[i].step == ve->instance_divisor)
			{
				float v[4];
				per_instance[i].map += vb->stride;
				per_instance[i].step = 0;

				nvfx->vtxelt->per_instance[i].base.fetch_rgba_float(v, per_instance[i].map, 0, 0);
				nvfx_emit_vtx_attr(chan, eng3d,
						   nvfx->vtxelt->per_instance[i].base.idx,
						   v,
						   nvfx->vtxelt->per_instance[i].base.ncomp);
			}
		}
	}
}
