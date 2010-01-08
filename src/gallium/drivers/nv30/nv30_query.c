#include "pipe/p_context.h"

#include "nv30_context.h"

struct nv30_query {
	struct nouveau_resource *object;
	unsigned type;
	boolean ready;
	uint64_t result;
};

static INLINE struct nv30_query *
nv30_query(struct pipe_query *pipe)
{
	return (struct nv30_query *)pipe;
}

static struct pipe_query *
nv30_query_create(struct pipe_context *pipe, unsigned query_type)
{
	struct nv30_query *q;

	q = CALLOC(1, sizeof(struct nv30_query));
	q->type = query_type;

	return (struct pipe_query *)q;
}

static void
nv30_query_destroy(struct pipe_context *pipe, struct pipe_query *pq)
{
	struct nv30_query *q = nv30_query(pq);

	if (q->object)
		nouveau_resource_free(&q->object);
	FREE(q);
}

static void
nv30_query_begin(struct pipe_context *pipe, struct pipe_query *pq)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	struct nv30_query *q = nv30_query(pq);
	struct nv30_screen *screen = nv30->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *rankine = screen->rankine;

	assert(q->type == PIPE_QUERY_OCCLUSION_COUNTER);

	/* Happens when end_query() is called, then another begin_query()
	 * without querying the result in-between.  For now we'll wait for
	 * the existing query to notify completion, but it could be better.
	 */
	if (q->object) {
		uint64_t tmp;
		pipe->get_query_result(pipe, pq, 1, &tmp);
	}

	if (nouveau_resource_alloc(nv30->screen->query_heap, 1, NULL, &q->object))
		assert(0);
	nouveau_notifier_reset(nv30->screen->query, q->object->start);

	BEGIN_RING(chan, rankine, NV34TCL_QUERY_RESET, 1);
	OUT_RING  (chan, 1);
	BEGIN_RING(chan, rankine, NV34TCL_QUERY_UNK17CC, 1);
	OUT_RING  (chan, 1);

	q->ready = FALSE;
}

static void
nv30_query_end(struct pipe_context *pipe, struct pipe_query *pq)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	struct nv30_screen *screen = nv30->screen;
	struct nouveau_channel *chan = screen->base.channel;
	struct nouveau_grobj *rankine = screen->rankine;
	struct nv30_query *q = nv30_query(pq);

	BEGIN_RING(chan, rankine, NV34TCL_QUERY_GET, 1);
	OUT_RING  (chan, (0x01 << NV34TCL_QUERY_GET_UNK24_SHIFT) |
		   ((q->object->start * 32) << NV34TCL_QUERY_GET_OFFSET_SHIFT));
	FIRE_RING(chan);
}

static boolean
nv30_query_result(struct pipe_context *pipe, struct pipe_query *pq,
		  boolean wait, uint64_t *result)
{
	struct nv30_context *nv30 = nv30_context(pipe);
	struct nv30_query *q = nv30_query(pq);

	assert(q->object && q->type == PIPE_QUERY_OCCLUSION_COUNTER);

	if (!q->ready) {
		unsigned status;

		status = nouveau_notifier_status(nv30->screen->query,
						 q->object->start);
		if (status != NV_NOTIFY_STATE_STATUS_COMPLETED) {
			if (wait == FALSE)
				return FALSE;

			nouveau_notifier_wait_status(nv30->screen->query,
					q->object->start,
					NV_NOTIFY_STATE_STATUS_COMPLETED, 0);
		}

		q->result = nouveau_notifier_return_val(nv30->screen->query,
							q->object->start);
		q->ready = TRUE;
		nouveau_resource_free(&q->object);
	}

	*result = q->result;
	return TRUE;
}

void
nv30_init_query_functions(struct nv30_context *nv30)
{
	nv30->pipe.create_query = nv30_query_create;
	nv30->pipe.destroy_query = nv30_query_destroy;
	nv30->pipe.begin_query = nv30_query_begin;
	nv30->pipe.end_query = nv30_query_end;
	nv30->pipe.get_query_result = nv30_query_result;
}
