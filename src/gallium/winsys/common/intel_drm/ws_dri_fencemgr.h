#ifndef DRI_FENCEMGR_H
#define DRI_FENCEMGR_H

#include <stdint.h>
#include <stdlib.h>

struct _DriFenceObject;
struct _DriFenceMgr;

/*
 * Do a quick check to see if the fence manager has registered the fence
 * object as signaled. Note that this function may return a false negative
 * answer.
 */
extern uint32_t driFenceSignaledTypeCached(struct _DriFenceObject *fence);

/*
 * Check if the fence object is signaled. This function can be substantially
 * more expensive to call than the above function, but will not return a false
 * negative answer. The argument "flush_type" sets the types that the
 * underlying mechanism must make sure will eventually signal.
 */
extern int driFenceSignaledType(struct _DriFenceObject *fence,
				uint32_t flush_type, uint32_t *signaled);

/*
 * Convenience functions.
 */

static inline int driFenceSignaled(struct _DriFenceObject *fence,
				   uint32_t flush_type)
{
    uint32_t signaled_types;
    int ret = driFenceSignaledType(fence, flush_type, &signaled_types);
    if (ret)
	return 0;
    return ((signaled_types & flush_type) == flush_type);
}

static inline int driFenceSignaledCached(struct _DriFenceObject *fence,
					 uint32_t flush_type)
{
    uint32_t signaled_types =
	driFenceSignaledTypeCached(fence);

    return ((signaled_types & flush_type) == flush_type);
}

/*
 * Reference a fence object.
 */
extern struct _DriFenceObject *driFenceReference(struct _DriFenceObject *fence);

/*
 * Unreference a fence object. The fence object pointer will be reset to NULL.
 */

extern void driFenceUnReference(struct _DriFenceObject **pFence);


/*
 * Wait for a fence to signal the indicated fence_type.
 * If "lazy_hint" is true, it indicates that the wait may sleep to avoid
 * busy-wait polling.
 */
extern int driFenceFinish(struct _DriFenceObject *fence, uint32_t fence_type,
			  int lazy_hint);

/*
 * Create a DriFenceObject for manager "mgr".
 *
 * "private" is a pointer that should be used for the callbacks in
 * struct _DriFenceMgrCreateInfo.
 *
 * if private_size is nonzero, then the info stored at *private, with size
 * private size will be copied and the fence manager will instead use a
 * pointer to the copied data for the callbacks in
 * struct _DriFenceMgrCreateInfo. In that case, the object pointed to by
 * "private" may be destroyed after the call to driFenceCreate.
 */
extern struct _DriFenceObject *driFenceCreate(struct _DriFenceMgr *mgr,
					      uint32_t fence_class,
					      uint32_t fence_type,
					      void *private,
					      size_t private_size);

extern uint32_t driFenceType(struct _DriFenceObject *fence);

/*
 * Fence creations are ordered. If a fence signals a fence_type,
 * it is safe to assume that all fences of the same class that was
 * created before that fence has signaled the same type.
 */

#define DRI_FENCE_CLASS_ORDERED (1 << 0)

struct _DriFenceMgrCreateInfo {
    uint32_t flags;
    uint32_t num_classes;
    int (*signaled) (struct _DriFenceMgr *mgr, void *private, uint32_t flush_type,
		     uint32_t *signaled_type);
    int (*finish) (struct _DriFenceMgr *mgr, void *private, uint32_t fence_type, int lazy_hint);
    int (*unreference) (struct _DriFenceMgr *mgr, void **private);
};

extern struct _DriFenceMgr *
driFenceMgrCreate(const struct _DriFenceMgrCreateInfo *info);

void
driFenceMgrUnReference(struct _DriFenceMgr **pMgr);

extern struct _DriFenceMgr *
driFenceMgrTTMInit(int fd);

#endif
