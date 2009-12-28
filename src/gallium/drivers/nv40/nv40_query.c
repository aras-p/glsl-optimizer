#include "pipe/p_context.h"

#include "nv40_context.h"

struct nv40_query {
	struct nouveau_resource *object;
	unsigned type;
	boolean ready;
	uint64_t result;
};

static INLINE struct nv40_query *
nv40_query(struct pipe_query *pipe)
{
	return (struct nv40_query *)pipe;
}

static struct pipe_query *
nv40_query_create(struct pipe_context *pipe, unsigned query_type)
{
	struct nv40_query *q;

	q = CALLOC(1, sizeof(struct nv40_query));
	q->type = query_type;

	return (struct pipe_query *)q;
}

static void
nv40_query_destroy(struct pipe_context *pipe, struct pipe_query *pq)
{
	struct nv40_query *q = nv40_query(pq);

	if (q->object)
		nouveau_resource_free(&q->object);
	FREE(q);
}

static void
nv40_query_begin(struct pipe_context *pipe, struct pipe_query *pq)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	struct nv40_query *q = nv40_query(pq);
	struct nv40_screen *screen = nv40->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *curie = screen->curie;

	assert(q->type == PIPE_QUERY_OCCLUSION_COUNTER);

	/* Happens when end_query() is called, then another begin_query()
	 * without querying the result in-between.  For now we'll wait for
	 * the existing query to notify completion, but it could be better.
	 */
	if (q->object) {
		uint64_t tmp;
		pipe->get_query_result(pipe, pq, 1, &tmp);
	}

	if (nouveau_resource_alloc(nv40->screen->query_heap, 1, NULL, &q->object))
		assert(0);
	nouveau_notifier_reset(nv40->screen->query, q->object->start);

	BEGIN_RING(chan, curie, NV40TCL_QUERY_RESET, 1);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, curie, NV40TCL_QUERY_UNK17CC, 1);
	OUT_RING  (chan, 1);

	q->ready = FALSE;
}

static void
nv40_query_end(struct pipe_context *pipe, struct pipe_query *pq)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	struct nv40_query *q = nv40_query(pq);
	struct nv40_screen *screen = nv40->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *curie = screen->curie;

	BEGIN_RING(chan, curie, NV40TCL_QUERY_GET, 1);
	OUT_RING  (chan, (0x01 << NV40TCL_QUERY_GET_UNK24_SHIFT) |
		   ((q->object->start * 32) << NV40TCL_QUERY_GET_OFFSET_SHIFT));
	FIRE_RING(chan);
}

static boolean
nv40_query_result(struct pipe_context *pipe, struct pipe_query *pq,
		  boolean wait, uint64_t *result)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	struct nv40_query *q = nv40_query(pq);

	assert(q->object && q->type == PIPE_QUERY_OCCLUSION_COUNTER);

	if (!q->ready) {
		unsigned status;

		status = nouveau_notifier_status(nv40->screen->query,
						 q->object->start);
		if (status != NV_NOTIFY_STATE_STATUS_COMPLETED) {
			if (wait == FALSE)
				return FALSE;
			nouveau_notifier_wait_status(nv40->screen->query,
					      q->object->start,
					      NV_NOTIFY_STATE_STATUS_COMPLETED,
					      0);
		}

		q->result = nouveau_notifier_return_val(nv40->screen->query,
							q->object->start);
		q->ready = TRUE;
		nouveau_resource_free(&q->object);
	}

	*result = q->result;
	return TRUE;
}

void
nv40_init_query_functions(struct nv40_context *nv40)
{
	nv40->pipe.create_query = nv40_query_create;
	nv40->pipe.destroy_query = nv40_query_destroy;
	nv40->pipe.begin_query = nv40_query_begin;
	nv40->pipe.end_query = nv40_query_end;
	nv40->pipe.get_query_result = nv40_query_result;
}
