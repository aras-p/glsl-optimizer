#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"

#include "nv50_context.h"
#include "nv50_dma.h"

static ubyte *
nv50_region_map(struct pipe_context *pipe, struct pipe_region *region)
{
	struct nv50_context *nv50 = (struct nv50_context *)pipe;
	struct pipe_winsys *ws = nv50->pipe.winsys;

	if (!region->map_refcount++) {
		region->map = ws->buffer_map(ws, region->buffer,
					     PIPE_BUFFER_FLAG_WRITE |
					     PIPE_BUFFER_FLAG_READ);
	}

	return region->map;
}

static void
nv50_region_unmap(struct pipe_context *pipe, struct pipe_region *region)
{
	struct nv50_context *nv50 = (struct nv50_context *)pipe;
	struct pipe_winsys *ws = nv50->pipe.winsys;

	if (!--region->map_refcount) {
		ws->buffer_unmap(ws, region->buffer);
		region->map = NULL;
	}
}

static void
nv50_region_data(struct pipe_context *pipe,
	       struct pipe_region *dst,
	       unsigned dst_offset,
	       unsigned dstx, unsigned dsty,
	       const void *src, unsigned src_pitch,
	       unsigned srcx, unsigned srcy, unsigned width, unsigned height)
{
	struct nv50_context *nv50 = (struct nv50_context *)pipe;
	struct nouveau_winsys *nvws = nv50->nvws;

	nvws->region_data(nvws, dst, dst_offset, dstx, dsty,
			  src, src_pitch, srcx, srcy, width, height);
}


static void
nv50_region_copy(struct pipe_context *pipe, struct pipe_region *dst,
		 unsigned dst_offset, unsigned dstx, unsigned dsty,
		 struct pipe_region *src, unsigned src_offset,
		 unsigned srcx, unsigned srcy, unsigned width, unsigned height)
{
	struct nv50_context *nv50 = (struct nv50_context *)pipe;
	struct nouveau_winsys *nvws = nv50->nvws;

	nvws->region_copy(nvws, dst, dst_offset, dstx, dsty,
			  src, src_offset, srcx, srcy, width, height);
}

static void
nv50_region_fill(struct pipe_context *pipe,
		 struct pipe_region *dst, unsigned dst_offset,
		 unsigned dstx, unsigned dsty,
		 unsigned width, unsigned height, unsigned value)
{
	struct nv50_context *nv50 = (struct nv50_context *)pipe;
	struct nouveau_winsys *nvws = nv50->nvws;

	nvws->region_fill(nvws, dst, dst_offset, dstx, dsty,
			  width, height, value);
}

void
nv50_init_region_functions(struct nv50_context *nv50)
{
	nv50->pipe.region_map = nv50_region_map;
	nv50->pipe.region_unmap = nv50_region_unmap;
	nv50->pipe.region_data = nv50_region_data;
	nv50->pipe.region_copy = nv50_region_copy;
	nv50->pipe.region_fill = nv50_region_fill;
}

