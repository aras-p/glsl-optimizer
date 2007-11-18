#include "pipe/p_context.h"

#include "nv40_context.h"
#include "nv40_dma.h"

static uint
nv40_query_object_find(struct nv40_context *nv40, struct pipe_query_object *q)
{
	int id;

	for (id = 0; id < nv40->num_query_objects; id++) {
		if (nv40->query_objects[id] == q)
			return id;
	}

	return -1;
}

void
nv40_query_begin(struct pipe_context *pipe, struct pipe_query_object *q)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	int id;

	assert(q->type == PIPE_QUERY_OCCLUSION_COUNTER);

	id = nv40_query_object_find(nv40, NULL);
	assert(id >= 0);
	nv40->query_objects[id] = q;

	nv40->nvws->notifier_reset(nv40->query, id);
	q->ready = 0;

	BEGIN_RING(curie, NV40TCL_QUERY_RESET, 1);
	OUT_RING  (1);
	BEGIN_RING(curie, NV40TCL_QUERY_UNK17CC, 1);
	OUT_RING  (1);
}

static void
nv40_query_update(struct pipe_context *pipe, struct pipe_query_object *q)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	int id;

	id = nv40_query_object_find(nv40, q);
	assert(id >= 0);

	if (nv40->nvws->notifier_status(nv40->query, id) == 0) {
		q->ready = 1;
		q->count = nv40->nvws->notifier_retval(nv40->query, id);
		nv40->query_objects[id] = NULL;
	}
}

void
nv40_query_end(struct pipe_context *pipe, struct pipe_query_object *q)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	int id;

	id = nv40_query_object_find(nv40, q);
	assert(id >= 0);

	BEGIN_RING(curie, NV40TCL_QUERY_GET, 1);
	OUT_RING  ((0x01 << NV40TCL_QUERY_GET_UNK24_SHIFT) |
		   ((id * 32) << NV40TCL_QUERY_GET_OFFSET_SHIFT));
	FIRE_RING ();

	/*XXX: Some apps spin waiting for GL_QUERY_RESULT_AVAILABLE_ARB.
	 *     Core mesa won't ask the driver to update the query object's
	 *     status in this case, so the app waits forever.. fix this some
	 *     day.
	 */
#if 0
	nv40_query_update(pipe, q);
#else
	nv40_query_wait(pipe, q);
#endif
}

void
nv40_query_wait(struct pipe_context *pipe, struct pipe_query_object *q)
{
	nv40_query_update(pipe, q);
	if (!q->ready) {
		struct nv40_context *nv40 = (struct nv40_context *)pipe;
		int id;
		
		id = nv40_query_object_find(nv40, q);
		assert(id >= 0);

		nv40->nvws->notifier_wait(nv40->query, id, 0, 0);
		nv40_query_update(pipe, q);
		assert(q->ready);
	}
}

