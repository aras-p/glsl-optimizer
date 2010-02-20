#include "pipe/p_context.h"

#include "nvfx_context.h"

struct nvfx_query {
	struct nouveau_resource *object;
	unsigned type;
	boolean ready;
	uint64_t result;
};

static INLINE struct nvfx_query *
nvfx_query(struct pipe_query *pipe)
{
	return (struct nvfx_query *)pipe;
}

static struct pipe_query *
nvfx_query_create(struct pipe_context *pipe, unsigned query_type)
{
	struct nvfx_query *q;

	q = CALLOC(1, sizeof(struct nvfx_query));
	q->type = query_type;

	return (struct pipe_query *)q;
}

static void
nvfx_query_destroy(struct pipe_context *pipe, struct pipe_query *pq)
{
	struct nvfx_query *q = nvfx_query(pq);

	if (q->object)
		nouveau_resource_free(&q->object);
	FREE(q);
}

static void
nvfx_query_begin(struct pipe_context *pipe, struct pipe_query *pq)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	struct nvfx_query *q = nvfx_query(pq);
	struct nvfx_screen *screen = nvfx->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *eng3d = screen->eng3d;

	assert(q->type == PIPE_QUERY_OCCLUSION_COUNTER);

	/* Happens when end_query() is called, then another begin_query()
	 * without querying the result in-between.  For now we'll wait for
	 * the existing query to notify completion, but it could be better.
	 */
	if (q->object) {
		uint64_t tmp;
		pipe->get_query_result(pipe, pq, 1, &tmp);
	}

	if (nouveau_resource_alloc(nvfx->screen->query_heap, 1, NULL, &q->object))
		assert(0);
	nouveau_notifier_reset(nvfx->screen->query, q->object->start);

	BEGIN_RING(chan, eng3d, NV34TCL_QUERY_RESET, 1);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, eng3d, NV34TCL_QUERY_UNK17CC, 1);
	OUT_RING  (chan, 1);

	q->ready = FALSE;
}

static void
nvfx_query_end(struct pipe_context *pipe, struct pipe_query *pq)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	struct nvfx_screen *screen = nvfx->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *eng3d = screen->eng3d;
	struct nvfx_query *q = nvfx_query(pq);

	BEGIN_RING(chan, eng3d, NV34TCL_QUERY_GET, 1);
	OUT_RING  (chan, (0x01 << NV34TCL_QUERY_GET_UNK24_SHIFT) |
		   ((q->object->start * 32) << NV34TCL_QUERY_GET_OFFSET_SHIFT));
	FIRE_RING(chan);
}

static boolean
nvfx_query_result(struct pipe_context *pipe, struct pipe_query *pq,
		  boolean wait, uint64_t *result)
{
	struct nvfx_context *nvfx = nvfx_context(pipe);
	struct nvfx_query *q = nvfx_query(pq);

	assert(q->object && q->type == PIPE_QUERY_OCCLUSION_COUNTER);

	if (!q->ready) {
		unsigned status;

		status = nouveau_notifier_status(nvfx->screen->query,
						 q->object->start);
		if (status != NV_NOTIFY_STATE_STATUS_COMPLETED) {
			if (wait == FALSE)
				return FALSE;

			nouveau_notifier_wait_status(nvfx->screen->query,
					q->object->start,
					NV_NOTIFY_STATE_STATUS_COMPLETED, 0);
		}

		q->result = nouveau_notifier_return_val(nvfx->screen->query,
							q->object->start);
		q->ready = TRUE;
		nouveau_resource_free(&q->object);
	}

	*result = q->result;
	return TRUE;
}

void
nvfx_init_query_functions(struct nvfx_context *nvfx)
{
	nvfx->pipe.create_query = nvfx_query_create;
	nvfx->pipe.destroy_query = nvfx_query_destroy;
	nvfx->pipe.begin_query = nvfx_query_begin;
	nvfx->pipe.end_query = nvfx_query_end;
	nvfx->pipe.get_query_result = nvfx_query_result;
}
