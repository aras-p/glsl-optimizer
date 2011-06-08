#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_memory.h"
#include "util/u_math.h"
#include "util/u_staging.h"
#include "nvfx_context.h"
#include "nvfx_screen.h"
#include "nvfx_state.h"
#include "nvfx_resource.h"
#include "nvfx_transfer.h"

struct nvfx_staging_transfer
{
	struct util_staging_transfer base;

	unsigned offset;
	unsigned map_count;
};

struct pipe_transfer *
nvfx_transfer_new(struct pipe_context *pipe,
		  struct pipe_resource *pt,
		  unsigned level,
		  unsigned usage,
		  const struct pipe_box *box)
{
        if((usage & (PIPE_TRANSFER_UNSYNCHRONIZED | PIPE_TRANSFER_DONTBLOCK)) == PIPE_TRANSFER_DONTBLOCK)
        {
                struct nouveau_bo* bo = ((struct nvfx_resource*)pt)->bo;
                if(bo && nouveau_bo_busy(bo, NOUVEAU_BO_WR))
                        return NULL;
        }

	if(pt->target == PIPE_BUFFER)
	{
		// it would be nice if we could avoid all this ridiculous overhead...
		struct pipe_transfer* tx;
		struct nvfx_buffer* buffer = nvfx_buffer(pt);

		tx = CALLOC_STRUCT(pipe_transfer);
		if (!tx)
			return NULL;

		pipe_resource_reference(&tx->resource, pt);
		tx->level = level;
		tx->usage = usage;
		tx->box = *box;

		tx->layer_stride = tx->stride = util_format_get_stride(pt->format, box->width);
		tx->data = buffer->data + util_format_get_stride(pt->format, box->x);

		return tx;
	}
	else
	{
	        struct nvfx_staging_transfer* tx;
	        boolean direct = !nvfx_resource_on_gpu(pt) && pt->flags & NVFX_RESOURCE_FLAG_LINEAR;

	        tx = CALLOC_STRUCT(nvfx_staging_transfer);
	        if(!tx)
	        	return NULL;

	        util_staging_transfer_init(pipe, pt, level, usage, box, direct, &tx->base);

		if(direct)
		{
			tx->base.base.stride = nvfx_subresource_pitch(pt, level);
			tx->base.base.layer_stride = tx->base.base.stride * u_minify(pt->height0, level);
			tx->offset = nvfx_subresource_offset(pt, box->z, level, box->z)
				+ util_format_get_2d_size(pt->format, tx->base.base.stride, box->y)
				+ util_format_get_stride(pt->format, box->x);
		}
		else
		{
			tx->base.base.stride = nvfx_subresource_pitch(tx->base.staging_resource, 0);
			tx->base.base.layer_stride = tx->base.base.stride * tx->base.staging_resource->height0;
			tx->offset = 0;
		}

		assert(tx->base.base.stride);

		return &tx->base.base;
	}
}

static void nvfx_buffer_dirty_interval(struct nvfx_buffer* buffer, unsigned begin, unsigned size, boolean unsynchronized)
{
	struct nvfx_screen* screen = nvfx_screen(buffer->base.base.screen);
	buffer->last_update_static = buffer->bytes_to_draw_until_static < 0;
	if(buffer->dirty_begin == buffer->dirty_end)
	{
		buffer->dirty_begin = begin;
		buffer->dirty_end = begin + size;
		buffer->dirty_unsynchronized = unsynchronized;
	}
	else
	{
		buffer->dirty_begin = MIN2(buffer->dirty_begin, begin);
		buffer->dirty_end = MAX2(buffer->dirty_end, begin + size);
		buffer->dirty_unsynchronized &= unsynchronized;
	}

	if(unsynchronized)
	{
		// TODO: revisit this, it doesn't seem quite right
		//printf("UNSYNC UPDATE %p %u %u\n", buffer, begin, size);
		buffer->bytes_to_draw_until_static += size * screen->static_reuse_threshold;
	}
	else
		buffer->bytes_to_draw_until_static = buffer->size * screen->static_reuse_threshold;
}

static void nvfx_transfer_flush_region( struct pipe_context *pipe,
				      struct pipe_transfer *ptx,
				      const struct pipe_box *box)
{
	if(ptx->resource->target == PIPE_BUFFER && (ptx->usage & PIPE_TRANSFER_FLUSH_EXPLICIT))
	{
		struct nvfx_buffer* buffer = nvfx_buffer(ptx->resource);
		nvfx_buffer_dirty_interval(buffer,
				(uint8_t*)ptx->data - buffer->data + util_format_get_stride(buffer->base.base.format, box->x),
				util_format_get_stride(buffer->base.base.format, box->width),
				!!(ptx->usage & PIPE_TRANSFER_UNSYNCHRONIZED));
	}
}

static void
nvfx_transfer_destroy(struct pipe_context *pipe, struct pipe_transfer *ptx)
{
	if(ptx->resource->target == PIPE_BUFFER)
	{
		struct nvfx_buffer* buffer = nvfx_buffer(ptx->resource);
		if((ptx->usage & (PIPE_TRANSFER_WRITE | PIPE_TRANSFER_FLUSH_EXPLICIT)) == PIPE_TRANSFER_WRITE)
			nvfx_buffer_dirty_interval(buffer,
				(uint8_t*)ptx->data - buffer->data,
				ptx->stride,
				!!(ptx->usage & PIPE_TRANSFER_UNSYNCHRONIZED));
		pipe_resource_reference(&ptx->resource, 0);
		FREE(ptx);
	}
	else
	{
		struct nouveau_channel* chan = nvfx_context(pipe)->screen->base.channel;
		util_staging_transfer_destroy(pipe, ptx);

		FIRE_RING(chan);
	}
}

void *
nvfx_transfer_map(struct pipe_context *pipe, struct pipe_transfer *ptx)
{
	if(ptx->resource->target == PIPE_BUFFER)
		return ptx->data;
	else
	{
		struct nvfx_staging_transfer *tx = (struct nvfx_staging_transfer *)ptx;
		if(!ptx->data)
		{
			struct nvfx_miptree *mt = (struct nvfx_miptree *)tx->base.staging_resource;
			uint8_t *map = nouveau_screen_bo_map(pipe->screen, mt->base.bo, nouveau_screen_transfer_flags(ptx->usage));
			ptx->data = map + tx->offset;
		}

		++tx->map_count;
		return ptx->data;
	}
}

void
nvfx_transfer_unmap(struct pipe_context *pipe, struct pipe_transfer *ptx)
{
	if(ptx->resource->target != PIPE_BUFFER)
	{
		struct nvfx_staging_transfer *tx = (struct nvfx_staging_transfer *)ptx;
		struct nvfx_miptree *mt = (struct nvfx_miptree *)tx->base.staging_resource;

		if(!--tx->map_count)
		{
			nouveau_screen_bo_unmap(pipe->screen, mt->base.bo);
			ptx->data = 0;
		}
	}
}

static void nvfx_transfer_inline_write( struct pipe_context *pipe,
				      struct pipe_resource *pr,
				      unsigned level,
				      unsigned usage,
				      const struct pipe_box *box,
				      const void *data,
				      unsigned stride,
				      unsigned slice_stride)
{
	if(pr->target != PIPE_BUFFER)
	{
		u_default_transfer_inline_write(pipe, pr, level, usage, box, data, stride, slice_stride);
	}
	else
	{
		struct nvfx_buffer* buffer = nvfx_buffer(pr);
		unsigned begin = util_format_get_stride(pr->format, box->x);
		unsigned size = util_format_get_stride(pr->format, box->width);
		memcpy(buffer->data + begin, data, size);
		nvfx_buffer_dirty_interval(buffer, begin, size,
				!!(pr->flags & PIPE_TRANSFER_UNSYNCHRONIZED));
	}
}

void
nvfx_init_transfer_functions(struct pipe_context *pipe)
{
	pipe->get_transfer = nvfx_transfer_new;
	pipe->transfer_map = nvfx_transfer_map;
	pipe->transfer_flush_region = nvfx_transfer_flush_region;
	pipe->transfer_unmap = nvfx_transfer_unmap;
	pipe->transfer_destroy = nvfx_transfer_destroy;
	pipe->transfer_inline_write = nvfx_transfer_inline_write;
}
