/**************************************************************************
 *
 * Copyright 2006 Tungsten Graphics, Inc., Bismarck, ND., USA
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 *
 **************************************************************************/
/*
 * Authors: Thomas Hellström <thomas-at-tungstengraphics-dot-com>
 *          Keith Whitwell <keithw-at-tungstengraphics-dot-com>
 */

#ifndef _PSB_BUFMGR_H_
#define _PSB_BUFMGR_H_
#include <xf86mm.h>
#include "i915_drm.h"
#include "ws_dri_fencemgr.h"

typedef struct _drmBONode
{
    drmMMListHead head;
    drmBO *buf;
    struct drm_i915_op_arg bo_arg;
    uint64_t arg0;
    uint64_t arg1;
} drmBONode;

typedef struct _drmBOList {
    unsigned numTarget;
    unsigned numCurrent;
    unsigned numOnList;
    drmMMListHead list;
    drmMMListHead free;
} drmBOList;


struct _DriFenceObject;
struct _DriBufferObject;
struct _DriBufferPool;
struct _DriBufferList;

/*
 * Return a pointer to the libdrm buffer object this DriBufferObject
 * uses.
 */

extern drmBO *driBOKernel(struct _DriBufferObject *buf);
extern void *driBOMap(struct _DriBufferObject *buf, unsigned flags,
                      unsigned hint);
extern void driBOUnmap(struct _DriBufferObject *buf);
extern unsigned long driBOOffset(struct _DriBufferObject *buf);
extern unsigned long driBOPoolOffset(struct _DriBufferObject *buf);

extern uint64_t driBOFlags(struct _DriBufferObject *buf);
extern struct _DriBufferObject *driBOReference(struct _DriBufferObject *buf);
extern void driBOUnReference(struct _DriBufferObject *buf);

extern int driBOData(struct _DriBufferObject *r_buf,
		     unsigned size, const void *data,
		     struct _DriBufferPool *pool, uint64_t flags);

extern void driBOSubData(struct _DriBufferObject *buf,
                         unsigned long offset, unsigned long size,
                         const void *data);
extern void driBOGetSubData(struct _DriBufferObject *buf,
                            unsigned long offset, unsigned long size,
                            void *data);
extern int driGenBuffers(struct _DriBufferPool *pool,
			 const char *name,
			 unsigned n,
			 struct _DriBufferObject *buffers[],
			 unsigned alignment, uint64_t flags, unsigned hint);
extern void driGenUserBuffer(struct _DriBufferPool *pool,
                             const char *name,
                             struct _DriBufferObject *buffers[],
                             void *ptr, unsigned bytes);
extern void driDeleteBuffers(unsigned n, struct _DriBufferObject *buffers[]);
extern void driInitBufMgr(int fd);
extern struct _DriBufferList *driBOCreateList(int target);
extern int driBOResetList(struct _DriBufferList * list);
extern void driBOAddListItem(struct _DriBufferList * list,
			     struct _DriBufferObject *buf,
                             uint64_t flags, uint64_t mask, int *itemLoc,
			     struct _drmBONode **node);

extern void driBOValidateList(int fd, struct _DriBufferList * list);
extern void driBOFreeList(struct _DriBufferList * list);
extern struct _DriFenceObject *driBOFenceUserList(struct _DriFenceMgr *mgr,
						  struct _DriBufferList *list,
						  const char *name,
						  drmFence *kFence);
extern void driBOUnrefUserList(struct _DriBufferList *list);
extern void driBOValidateUserList(struct _DriBufferList * list);
extern drmBOList *driGetdrmBOList(struct _DriBufferList *list);
extern void driPutdrmBOList(struct _DriBufferList *list);

extern void driBOFence(struct _DriBufferObject *buf,
                       struct _DriFenceObject *fence);

extern void driPoolTakeDown(struct _DriBufferPool *pool);
extern void driBOSetReferenced(struct _DriBufferObject *buf,
			       unsigned long handle);
unsigned long driBOSize(struct _DriBufferObject *buf);
extern void driBOWaitIdle(struct _DriBufferObject *buf, int lazy);
extern void driPoolTakeDown(struct _DriBufferPool *pool);

extern void driReadLockKernelBO(void);
extern void driReadUnlockKernelBO(void);
extern void driWriteLockKernelBO(void);
extern void driWriteUnlockKernelBO(void);

/*
 * For debugging purposes.
 */

extern drmBOList *driBOGetDRMBuffers(struct _DriBufferList *list);
extern drmBOList *driBOGetDRIBuffers(struct _DriBufferList *list);
#endif
