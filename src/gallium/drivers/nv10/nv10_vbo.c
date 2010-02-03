#include "draw/draw_context.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"

#include "nv10_context.h"
#include "nv10_state.h"

#include "nouveau/nouveau_channel.h"
#include "nouveau/nouveau_pushbuf.h"

void nv10_draw_elements( struct pipe_context *pipe,
                    struct pipe_buffer *indexBuffer,
                    unsigned indexSize,
                    unsigned prim, unsigned start, unsigned count)
{
	struct nv10_context *nv10 = nv10_context( pipe );
	struct draw_context *draw = nv10->draw;
	struct pipe_screen *pscreen = pipe->screen;
	unsigned i;

	nv10_emit_hw_state(nv10);

	/*
	 * Map vertex buffers
	 */
	for (i = 0; i < PIPE_MAX_ATTRIBS; i++) {
		if (nv10->vtxbuf[i].buffer) {
			void *buf =
				pipe_buffer_map(pscreen, nv10->vtxbuf[i].buffer,
						PIPE_BUFFER_USAGE_CPU_READ);
			draw_set_mapped_vertex_buffer(draw, i, buf);
		}
	}
	/* Map index buffer, if present */
	if (indexBuffer) {
		void *mapped_indexes
			= pipe_buffer_map(pscreen, indexBuffer,
					  PIPE_BUFFER_USAGE_CPU_READ);
		draw_set_mapped_element_buffer(draw, indexSize, mapped_indexes);
	}
	else {
		/* no index/element buffer */
		draw_set_mapped_element_buffer(draw, 0, NULL);
	}

	draw_set_mapped_constant_buffer(draw,
                                        PIPE_SHADER_VERTEX,
                                        0,
					nv10->constbuf[PIPE_SHADER_VERTEX],
					nv10->constbuf_nr[PIPE_SHADER_VERTEX]);

	/* draw! */
	draw_arrays(nv10->draw, prim, start, count);

	/*
	 * unmap vertex/index buffers
	 */
	for (i = 0; i < PIPE_MAX_ATTRIBS; i++) {
		if (nv10->vtxbuf[i].buffer) {
			pipe_buffer_unmap(pscreen, nv10->vtxbuf[i].buffer);
			draw_set_mapped_vertex_buffer(draw, i, NULL);
		}
	}
	if (indexBuffer) {
		pipe_buffer_unmap(pscreen, indexBuffer);
		draw_set_mapped_element_buffer(draw, 0, NULL);
	}
}

void nv10_draw_arrays( struct pipe_context *pipe,
                       unsigned prim, unsigned start, unsigned count)
{
	nv10_draw_elements(pipe, NULL, 0, prim, start, count);
}



