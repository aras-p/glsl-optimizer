#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"

#include "nv40_context.h"
#include "nv40_dma.h"

static ubyte *
nv40_region_map(struct pipe_context *pipe, struct pipe_region *region)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	struct pipe_winsys *ws = nv40->pipe.winsys;

	if (!region->map_refcount++) {
		region->map = ws->buffer_map(ws, region->buffer,
					     PIPE_BUFFER_FLAG_WRITE |
					     PIPE_BUFFER_FLAG_READ);
	}

	return region->map;
}

static void
nv40_region_unmap(struct pipe_context *pipe, struct pipe_region *region)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	struct pipe_winsys *ws = nv40->pipe.winsys;

	if (!--region->map_refcount) {
		ws->buffer_unmap(ws, region->buffer);
		region->map = NULL;
	}
}

static void
nv40_region_data(struct pipe_context *pipe,
	       struct pipe_region *dst,
	       unsigned dst_offset,
	       unsigned dstx, unsigned dsty,
	       const void *src, unsigned src_pitch,
	       unsigned srcx, unsigned srcy, unsigned width, unsigned height)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	struct nouveau_winsys *nvws = nv40->nvws;

	nvws->region_data(nvws->nv, dst, dst_offset, dstx, dsty,
			  src, src_pitch, srcx, srcy, width, height);
}


static void
nv40_region_copy(struct pipe_context *pipe, struct pipe_region *dst,
		 unsigned dst_offset, unsigned dstx, unsigned dsty,
		 struct pipe_region *src, unsigned src_offset,
		 unsigned srcx, unsigned srcy, unsigned width, unsigned height)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	struct nouveau_winsys *nvws = nv40->nvws;

	nvws->region_copy(nvws->nv, dst, dst_offset, dstx, dsty,
			  src, src_offset, srcx, srcy, width, height);
}

static void
nv40_region_fill(struct pipe_context *pipe,
		 struct pipe_region *dst, unsigned dst_offset,
		 unsigned dstx, unsigned dsty,
		 unsigned width, unsigned height, unsigned value)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	struct nouveau_winsys *nvws = nv40->nvws;

	nvws->region_fill(nvws->nv, dst, dst_offset, dstx, dsty,
			  width, height, value);
}

void
nv40_init_region_functions(struct nv40_context *nv40)
{
	nv40->pipe.region_map = nv40_region_map;
	nv40->pipe.region_unmap = nv40_region_unmap;
	nv40->pipe.region_data = nv40_region_data;
	nv40->pipe.region_copy = nv40_region_copy;
	nv40->pipe.region_fill = nv40_region_fill;
}

