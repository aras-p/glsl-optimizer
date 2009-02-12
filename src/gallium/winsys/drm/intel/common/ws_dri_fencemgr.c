#include "ws_dri_fencemgr.h"
#include "pipe/p_thread.h"
#include <xf86mm.h>
#include <string.h>
#include <unistd.h>

/*
 * Note: Locking order is
 * _DriFenceObject::mutex
 * _DriFenceMgr::mutex
 */

struct _DriFenceMgr {
    /*
     * Constant members. Need no mutex protection.
     */
    struct _DriFenceMgrCreateInfo info;
    void *private;

    /*
     * These members are protected by this->mutex
     */
    pipe_mutex mutex;
    int refCount;
    drmMMListHead *heads;
    int num_fences;
};

struct _DriFenceObject {

    /*
     * These members are constant and need no mutex protection.
     */
    struct _DriFenceMgr *mgr;
    uint32_t fence_class;
    uint32_t fence_type;

    /*
     * These members are protected by mgr->mutex.
     */
    drmMMListHead head;
    int refCount;

    /*
     * These members are protected by this->mutex.
     */
    pipe_mutex mutex;
    uint32_t signaled_type;
    void *private;
};

uint32_t
driFenceType(struct _DriFenceObject *fence)
{
  return fence->fence_type;
}

struct _DriFenceMgr *
driFenceMgrCreate(const struct _DriFenceMgrCreateInfo *info)
{
  struct _DriFenceMgr *tmp;
  uint32_t i;

  tmp = calloc(1, sizeof(*tmp));
  if (!tmp)
      return NULL;

  pipe_mutex_init(tmp->mutex);
  pipe_mutex_lock(tmp->mutex);
  tmp->refCount = 1;
  tmp->info = *info;
  tmp->num_fences = 0;
  tmp->heads = calloc(tmp->info.num_classes, sizeof(*tmp->heads));
  if (!tmp->heads)
      goto out_err;

  for (i=0; i<tmp->info.num_classes; ++i) {
      DRMINITLISTHEAD(&tmp->heads[i]);
  }
  pipe_mutex_unlock(tmp->mutex);
  return tmp;

  out_err:
  if (tmp)
      free(tmp);
  return NULL;
}

static void
driFenceMgrUnrefUnlock(struct _DriFenceMgr **pMgr)
{
    struct _DriFenceMgr *mgr = *pMgr;

    *pMgr = NULL;
    if (--mgr->refCount == 0)
	free(mgr);
    else
	pipe_mutex_unlock(mgr->mutex);
}

void
driFenceMgrUnReference(struct _DriFenceMgr **pMgr)
{
    pipe_mutex_lock((*pMgr)->mutex);
    driFenceMgrUnrefUnlock(pMgr);
}

static void
driFenceUnReferenceLocked(struct _DriFenceObject **pFence)
{
    struct _DriFenceObject *fence = *pFence;
    struct _DriFenceMgr *mgr = fence->mgr;

    *pFence = NULL;
    if (--fence->refCount == 0) {
	DRMLISTDELINIT(&fence->head);
	if (fence->private)
	    mgr->info.unreference(mgr, &fence->private);
    --mgr->num_fences;
	fence->mgr = NULL;
	--mgr->refCount;
	free(fence);

    }
}


static void
driSignalPreviousFencesLocked(struct _DriFenceMgr *mgr,
			      drmMMListHead *list,
			      uint32_t fence_class,
			      uint32_t fence_type)
{
    struct _DriFenceObject *entry;
    drmMMListHead *prev;

    while(list != &mgr->heads[fence_class]) {
	entry = DRMLISTENTRY(struct _DriFenceObject, list, head);

	/*
	 * Up refcount so that entry doesn't disappear from under us
	 * when we unlock-relock mgr to get the correct locking order.
	 */

	++entry->refCount;
	pipe_mutex_unlock(mgr->mutex);
	pipe_mutex_lock(entry->mutex);
	pipe_mutex_lock(mgr->mutex);

	prev = list->prev;



	if (list->prev == list) {

		/*
		 * Somebody else removed the entry from the list.
		 */

		pipe_mutex_unlock(entry->mutex);
		driFenceUnReferenceLocked(&entry);
		return;
	}

	entry->signaled_type |= (fence_type & entry->fence_type);
	if (entry->signaled_type == entry->fence_type) {
	    DRMLISTDELINIT(list);
	    mgr->info.unreference(mgr, &entry->private);
	}
	pipe_mutex_unlock(entry->mutex);
	driFenceUnReferenceLocked(&entry);
	list = prev;
    }
}


int
driFenceFinish(struct _DriFenceObject *fence, uint32_t fence_type,
	       int lazy_hint)
{
    struct _DriFenceMgr *mgr = fence->mgr;
    int ret = 0;

    pipe_mutex_lock(fence->mutex);

    if ((fence->signaled_type & fence_type) == fence_type)
	goto out0;

    ret = mgr->info.finish(mgr, fence->private, fence_type, lazy_hint);
    if (ret)
	goto out0;

    pipe_mutex_lock(mgr->mutex);
    pipe_mutex_unlock(fence->mutex);

    driSignalPreviousFencesLocked(mgr, &fence->head, fence->fence_class,
				  fence_type);
    pipe_mutex_unlock(mgr->mutex);
    return 0;

  out0:
    pipe_mutex_unlock(fence->mutex);
    return ret;
}

uint32_t driFenceSignaledTypeCached(struct _DriFenceObject *fence)
{
    uint32_t ret;

    pipe_mutex_lock(fence->mutex);
    ret = fence->signaled_type;
    pipe_mutex_unlock(fence->mutex);

    return ret;
}

int
driFenceSignaledType(struct _DriFenceObject *fence, uint32_t flush_type,
		 uint32_t *signaled)
{
    int ret = 0;
    struct _DriFenceMgr *mgr;

    pipe_mutex_lock(fence->mutex);
    mgr = fence->mgr;
    *signaled = fence->signaled_type;
    if ((fence->signaled_type & flush_type) == flush_type)
	goto out0;

    ret = mgr->info.signaled(mgr, fence->private, flush_type, signaled);
    if (ret) {
	*signaled = fence->signaled_type;
	goto out0;
    }

    if ((fence->signaled_type | *signaled) == fence->signaled_type)
	goto out0;

    pipe_mutex_lock(mgr->mutex);
    pipe_mutex_unlock(fence->mutex);

    driSignalPreviousFencesLocked(mgr, &fence->head, fence->fence_class,
				  *signaled);

    pipe_mutex_unlock(mgr->mutex);
    return 0;
  out0:
    pipe_mutex_unlock(fence->mutex);
    return ret;
}

struct _DriFenceObject *
driFenceReference(struct _DriFenceObject *fence)
{
    pipe_mutex_lock(fence->mgr->mutex);
    ++fence->refCount;
    pipe_mutex_unlock(fence->mgr->mutex);
    return fence;
}

void
driFenceUnReference(struct _DriFenceObject **pFence)
{
    struct _DriFenceMgr *mgr;

    if (*pFence == NULL)
	return;

    mgr = (*pFence)->mgr;
    pipe_mutex_lock(mgr->mutex);
    ++mgr->refCount;
    driFenceUnReferenceLocked(pFence);
    driFenceMgrUnrefUnlock(&mgr);
}

struct _DriFenceObject
*driFenceCreate(struct _DriFenceMgr *mgr, uint32_t fence_class,
		uint32_t fence_type, void *private, size_t private_size)
{
    struct _DriFenceObject *fence;
    size_t fence_size = sizeof(*fence);

    if (private_size)
	fence_size = ((fence_size + 15) & ~15);

    fence = calloc(1, fence_size + private_size);

    if (!fence) {
	int ret = mgr->info.finish(mgr, private, fence_type, 0);

	if (ret)
	    usleep(10000000);

	return NULL;
    }

    pipe_mutex_init(fence->mutex);
    pipe_mutex_lock(fence->mutex);
    pipe_mutex_lock(mgr->mutex);
    fence->refCount = 1;
    DRMLISTADDTAIL(&fence->head, &mgr->heads[fence_class]);
    fence->mgr = mgr;
    ++mgr->refCount;
    ++mgr->num_fences;
    pipe_mutex_unlock(mgr->mutex);
    fence->fence_class = fence_class;
    fence->fence_type = fence_type;
    fence->signaled_type = 0;
    fence->private = private;
    if (private_size) {
        fence->private = (void *)(((uint8_t *) fence) + fence_size);
	memcpy(fence->private, private, private_size);
    }

    pipe_mutex_unlock(fence->mutex);
    return fence;
}


static int
tSignaled(struct _DriFenceMgr *mgr, void *private, uint32_t flush_type,
	  uint32_t *signaled_type)
{
  long fd = (long) mgr->private;
  int dummy;
  drmFence *fence = (drmFence *) private;
  int ret;

  *signaled_type = 0;
  ret = drmFenceSignaled((int) fd, fence, flush_type, &dummy);
  if (ret)
    return ret;

  *signaled_type = fence->signaled;

  return 0;
}

static int
tFinish(struct _DriFenceMgr *mgr, void *private, uint32_t fence_type,
	int lazy_hint)
{
  long fd = (long) mgr->private;
  unsigned flags = lazy_hint ? DRM_FENCE_FLAG_WAIT_LAZY : 0;

  return drmFenceWait((int)fd, flags, (drmFence *) private, fence_type);
}

static int
tUnref(struct _DriFenceMgr *mgr, void **private)
{
  long fd = (long) mgr->private;
  drmFence *fence = (drmFence *) *private;
  *private = NULL;

  return drmFenceUnreference(fd, fence);
}

struct _DriFenceMgr *driFenceMgrTTMInit(int fd)
{
  struct _DriFenceMgrCreateInfo info;
  struct _DriFenceMgr *mgr;

  info.flags = DRI_FENCE_CLASS_ORDERED;
  info.num_classes = 4;
  info.signaled = tSignaled;
  info.finish = tFinish;
  info.unreference = tUnref;

  mgr = driFenceMgrCreate(&info);
  if (mgr == NULL)
    return NULL;

  mgr->private = (void *) (long) fd;
  return mgr;
}

