#include "pipe/p_context.h"

#include "nv40_context.h"
#include "nv40_dma.h"

/*XXX: Maybe go notifier per-query one day?  not sure if PRAMIN space is
 *     plentiful enough however.. */
struct nv40_query {
	unsigned type;
	int id;
};
#define nv40_query(o) ((struct nv40_query *)(o))

static struct pipe_query *
nv40_query_create(struct pipe_context *pipe, unsigned query_type)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	struct nv40_query *nv40query;
	int id;

	for (id = 0; id < nv40->num_query_objects; id++) {
		if (nv40->query_objects[id] == 0)
			break;
	}
	
	if (id == nv40->num_query_objects)
		return NULL;
	nv40->query_objects[id] = TRUE;

	nv40query = malloc(sizeof(struct nv40_query));
	nv40query->type = query_type;
	nv40query->id = id;

	return (struct pipe_query *)nv40query;
}

static void
nv40_query_destroy(struct pipe_context *pipe, struct pipe_query *q)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	struct nv40_query *nv40query = nv40_query(q);

	assert(nv40->query_objects[nv40query->id]);
	nv40->query_objects[nv40query->id] = FALSE;
	free(nv40query);
}

static void
nv40_query_begin(struct pipe_context *pipe, struct pipe_query *q)
{
	struct nv40_context *nv40 = nv40_context(pipe);
	struct nv40_query *nv40query = nv40_query(q);

	assert(nv40query->type == PIPE_QUERY_OCCLUSION_COUNTER);

	nv40->nvws->notifier_reset(nv40->query, nv40query->id);

	BEGIN_RING(curie, NV40TCL_QUERY_RESET, 1);
	OUT_RING  (1);
	BEGIN_RING(curie, NV40TCL_QUERY_UNK17CC, 1);
	OUT_RING  (1);
}

static void
nv40_query_end(struct pipe_context *pipe, struct pipe_query *q)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	struct nv40_query *nv40query = nv40_query(q);

	BEGIN_RING(curie, NV40TCL_QUERY_GET, 1);
	OUT_RING  ((0x01 << NV40TCL_QUERY_GET_UNK24_SHIFT) |
		   ((nv40query->id * 32) << NV40TCL_QUERY_GET_OFFSET_SHIFT));
	FIRE_RING();
}

static boolean
nv40_query_result(struct pipe_context *pipe, struct pipe_query *q,
		  boolean wait, uint64_t *result)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	struct nv40_query *nv40query = nv40_query(q);
	struct nouveau_winsys *nvws = nv40->nvws;
	boolean status;

	status = nvws->notifier_status(nv40->query, nv40query->id);
	if (status != NV_NOTIFY_STATE_STATUS_COMPLETED) {
		if (wait == FALSE)
			return FALSE;
		nvws->notifier_wait(nv40->query, nv40query->id,
				    NV_NOTIFY_STATE_STATUS_COMPLETED, 0);
	}

	*result = nvws->notifier_retval(nv40->query, nv40query->id);
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
