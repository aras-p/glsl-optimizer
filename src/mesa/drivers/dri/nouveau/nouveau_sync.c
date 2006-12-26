#include "vblank.h" /* for DO_USLEEP */

#include "nouveau_context.h"
#include "nouveau_buffers.h"
#include "nouveau_object.h"
#include "nouveau_fifo.h"
#include "nouveau_reg.h"
#include "nouveau_msg.h"
#include "nouveau_sync.h"

nouveau_notifier *
nouveau_notifier_new(GLcontext *ctx, GLuint handle)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	nouveau_notifier *notifier;

	notifier = CALLOC_STRUCT(nouveau_notifier_t);
	if (!notifier)
		return NULL;

	notifier->mem = nouveau_mem_alloc(ctx, NOUVEAU_MEM_FB, 32, 0);
	if (!notifier->mem) {
		FREE(notifier);
		return NULL;
	}

	if (!nouveauCreateDmaObject(nmesa, handle, notifier->mem->offset,
						   notifier->mem->size,
						   0 /* NV_DMA_TARGET_FB */,
						   0 /* NV_DMA_ACCESS_RW */)) {
		nouveau_mem_free(ctx, notifier->mem);
		FREE(notifier);
		return NULL;
	}

	notifier->handle = handle;
	return notifier;
}

void
nouveau_notifier_destroy(GLcontext *ctx, nouveau_notifier *notifier)
{
	/*XXX: free DMA object.. */
	nouveau_mem_free(ctx, notifier->mem);
	FREE(notifier);
}

void
nouveau_notifier_reset(nouveau_notifier *notifier)
{
	volatile GLuint *n = notifier->mem->map;

	n[NV_NOTIFY_TIME_0      /4] = 0x00000000;
	n[NV_NOTIFY_TIME_1      /4] = 0x00000000;
	n[NV_NOTIFY_RETURN_VALUE/4] = 0x00000000;
	n[NV_NOTIFY_STATE       /4] = (NV_NOTIFY_STATE_STATUS_IN_PROCESS <<
				       NV_NOTIFY_STATE_STATUS_SHIFT);
}

GLboolean
nouveau_notifier_wait_status(nouveau_notifier *notifier, GLuint status,
							 GLuint timeout)
{
	volatile GLuint *n = notifier->mem->map;
	unsigned int time = 0;

	while (time <= timeout) {
		if (n[NV_NOTIFY_STATE] & NV_NOTIFY_STATE_ERROR_CODE_MASK) {
			MESSAGE("Notifier returned error: 0x%04x\n",
					n[NV_NOTIFY_STATE] &
					NV_NOTIFY_STATE_ERROR_CODE_MASK);
			return GL_FALSE;
		}

		if (((n[NV_NOTIFY_STATE] & NV_NOTIFY_STATE_STATUS_MASK) >>
				NV_NOTIFY_STATE_STATUS_SHIFT) == status)
			return GL_TRUE;

		if (timeout) {
			DO_USLEEP(1);
			time++;
		}
	}

	MESSAGE("Notifier timed out\n");
	return GL_FALSE;
}

void
nouveau_notifier_wait_nop(GLcontext *ctx, nouveau_notifier *notifier,
					  GLuint subc)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	GLboolean ret;

	nouveau_notifier_reset(notifier);

	BEGIN_RING_SIZE(subc, NV_NOTIFY, 1);
	OUT_RING       (NV_NOTIFY_STYLE_WRITE_ONLY);
	BEGIN_RING_SIZE(subc, NV_NOP, 1);
	OUT_RING       (0);

	ret = nouveau_notifier_wait_status(notifier,
					   NV_NOTIFY_STATE_STATUS_COMPLETED,
					   0 /* no timeout */);
	if (ret) MESSAGE("wait on notifier failed\n");
}

void nouveauSyncInitFuncs(GLcontext *ctx)
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	nmesa->syncNotifier = nouveau_notifier_new(ctx, NvSyncNotify);
}

