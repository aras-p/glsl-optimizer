/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "intel_batchbuffer.h"
#include "intel_context.h"
#include <errno.h>

#if 0
static void
intel_dump_batchbuffer(GLuint offset, GLuint * ptr, GLuint count)
{
   int i;
   fprintf(stderr, "\n\n\nSTART BATCH (%d dwords):\n", count / 4);
   for (i = 0; i < count / 4; i += 4)
      fprintf(stderr, "0x%x:\t0x%08x 0x%08x 0x%08x 0x%08x\n",
              offset + i * 4, ptr[i], ptr[i + 1], ptr[i + 2], ptr[i + 3]);
   fprintf(stderr, "END BATCH\n\n\n");
}
#endif

static void 
intel_realloc_relocs(struct intel_batchbuffer *batch, int num_relocs)
{
    unsigned long size = num_relocs * I915_RELOC0_STRIDE + I915_RELOC_HEADER;
    
    size *= sizeof(uint32_t);
    batch->reloc = realloc(batch->reloc, size);
    batch->reloc_size = num_relocs;
}


void
intel_batchbuffer_reset(struct intel_batchbuffer *batch)
{
   /*
    * Get a new, free batchbuffer.
    */
    drmBO *bo;
    struct drm_bo_info_req *req;
    
   driBOUnrefUserList(batch->list);
   driBOResetList(batch->list);

   batch->size = batch->intel->intelScreen->max_batch_size;
   driBOData(batch->buffer, batch->size, NULL, NULL, 0);

   /*
    * Add the batchbuffer to the validate list.
    */

   driBOAddListItem(batch->list, batch->buffer,
		    DRM_BO_FLAG_EXE | DRM_BO_FLAG_MEM_TT,
		    DRM_BO_FLAG_EXE | DRM_BO_MASK_MEM,
		    &batch->dest_location, &batch->node);

   req = &batch->node->bo_arg.d.req.bo_req;

   /*
    * Set up information needed for us to make relocations
    * relative to the underlying drm buffer objects.
    */

   driReadLockKernelBO();
   bo = driBOKernel(batch->buffer);
   req->presumed_offset = (uint64_t) bo->offset;
   req->hint = DRM_BO_HINT_PRESUMED_OFFSET;
   batch->drmBOVirtual = (uint8_t *) bo->virtual;
   driReadUnlockKernelBO();

   /*
    * Adjust the relocation buffer size.
    */

   if (batch->reloc_size > INTEL_MAX_RELOCS ||
       batch->reloc == NULL) 
     intel_realloc_relocs(batch, INTEL_DEFAULT_RELOCS);
   
   assert(batch->reloc != NULL);
   batch->reloc[0] = 0; /* No relocs yet. */
   batch->reloc[1] = 1; /* Reloc type 1 */
   batch->reloc[2] = 0; /* Only a single relocation list. */
   batch->reloc[3] = 0; /* Only a single relocation list. */

   batch->map = driBOMap(batch->buffer, DRM_BO_FLAG_WRITE, 0);
   batch->poolOffset = driBOPoolOffset(batch->buffer);
   batch->ptr = batch->map;
   batch->dirty_state = ~0;
   batch->nr_relocs = 0;
   batch->flags = 0;
   batch->id = 0;//batch->intel->intelScreen->batch_id++;
}

/*======================================================================
 * Public functions
 */
struct intel_batchbuffer *
intel_batchbuffer_alloc(struct intel_context *intel)
{
   struct intel_batchbuffer *batch = calloc(sizeof(*batch), 1);

   batch->intel = intel;

   driGenBuffers(intel->intelScreen->batchPool, "batchbuffer", 1,
                 &batch->buffer, 4096,
                 DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_EXE, 0);
   batch->last_fence = NULL;
   batch->list = driBOCreateList(20);
   batch->reloc = NULL;
   intel_batchbuffer_reset(batch);
   return batch;
}

void
intel_batchbuffer_free(struct intel_batchbuffer *batch)
{
   if (batch->last_fence) {
      driFenceFinish(batch->last_fence,
		     DRM_FENCE_TYPE_EXE, GL_FALSE);
      driFenceUnReference(&batch->last_fence);
   }
   if (batch->map) {
      driBOUnmap(batch->buffer);
      batch->map = NULL;
   }
   driBOUnReference(batch->buffer);
   driBOFreeList(batch->list);
   if (batch->reloc)
       free(batch->reloc);
   batch->buffer = NULL;
   free(batch);
}

void
intel_offset_relocation(struct intel_batchbuffer *batch,
			unsigned pre_add,
			struct _DriBufferObject *driBO,
			uint64_t val_flags,
			uint64_t val_mask)
{
    int itemLoc;
    struct _drmBONode *node;
    uint32_t *reloc;
    struct drm_bo_info_req *req;
    
    driBOAddListItem(batch->list, driBO, val_flags, val_mask,
		     &itemLoc, &node);
    req = &node->bo_arg.d.req.bo_req;

    if (!(req->hint &  DRM_BO_HINT_PRESUMED_OFFSET)) {

	/*
	 * Stop other threads from tampering with the underlying
	 * drmBO while we're reading its offset.
	 */

	driReadLockKernelBO();
	req->presumed_offset = (uint64_t) driBOKernel(driBO)->offset;
	driReadUnlockKernelBO();
	req->hint = DRM_BO_HINT_PRESUMED_OFFSET;
    }
    
    pre_add += driBOPoolOffset(driBO);

    if (batch->nr_relocs == batch->reloc_size)
	intel_realloc_relocs(batch, batch->reloc_size * 2);

    reloc = batch->reloc + 
	(I915_RELOC_HEADER + batch->nr_relocs * I915_RELOC0_STRIDE);

    reloc[0] = ((uint8_t *)batch->ptr - batch->drmBOVirtual);
    intel_batchbuffer_emit_dword(batch, req->presumed_offset + pre_add);
    reloc[1] = pre_add;
    reloc[2] = itemLoc;
    reloc[3] = batch->dest_location;
    batch->nr_relocs++;
}

static void
i915_drm_copy_reply(const struct drm_bo_info_rep * rep, drmBO * buf)
{
    buf->handle = rep->handle;
    buf->flags = rep->flags;
    buf->size = rep->size;
    buf->offset = rep->offset;
    buf->mapHandle = rep->arg_handle;
    buf->proposedFlags = rep->proposed_flags;
    buf->start = rep->buffer_start;
    buf->fenceFlags = rep->fence_flags;
    buf->replyFlags = rep->rep_flags;
    buf->pageAlignment = rep->page_alignment;
}

static int 
i915_execbuf(struct intel_batchbuffer *batch,
	     GLuint used,
	     GLboolean ignore_cliprects,
	     drmBOList *list,
	     struct drm_i915_execbuffer *ea)
{
   struct intel_context *intel = batch->intel;
   drmBONode *node;
   drmMMListHead *l;
   struct drm_i915_op_arg *arg, *first;
   struct drm_bo_op_req *req;
   struct drm_bo_info_rep *rep;
   uint64_t *prevNext = NULL;
   drmBO *buf;
   int ret = 0;
   uint32_t count = 0;

   first = NULL;
   for (l = list->list.next; l != &list->list; l = l->next) {
      node = DRMLISTENTRY(drmBONode, l, head);
      
      arg = &node->bo_arg;
      req = &arg->d.req;
      
      if (!first)
	 first = arg;
      
      if (prevNext)
	 *prevNext = (unsigned long)arg;
      
      prevNext = &arg->next;
      req->bo_req.handle = node->buf->handle;
      req->op = drm_bo_validate;
      req->bo_req.flags = node->arg0;
      req->bo_req.mask = node->arg1;
      req->bo_req.hint |= 0;
      count++;
   }

   memset(ea, 0, sizeof(*ea));
   ea->num_buffers = count;
   ea->batch.start = batch->poolOffset;
   ea->batch.used = used;
#if 0 /* ZZZ JB: no cliprects used */
   ea->batch.cliprects = intel->pClipRects;
   ea->batch.num_cliprects = ignore_cliprects ? 0 : intel->numClipRects;
   ea->batch.DR1 = 0;
   ea->batch.DR4 = 0;((((GLuint) intel->drawX) & 0xffff) |
		   (((GLuint) intel->drawY) << 16));
#else
   ea->batch.cliprects = NULL;
   ea->batch.num_cliprects = 0;
   ea->batch.DR1 = 0;
   ea->batch.DR4 = 0;
#endif
   ea->fence_arg.flags = DRM_I915_FENCE_FLAG_FLUSHED;
   ea->ops_list = (unsigned long) first;
   first->reloc_ptr = (unsigned long) batch->reloc;
   batch->reloc[0] = batch->nr_relocs;

   //return -EFAULT;
   do {
      ret = drmCommandWriteRead(intel->driFd, DRM_I915_EXECBUFFER, ea,
				sizeof(*ea));
   } while (ret == -EAGAIN);

   if (ret != 0)
      return ret;

   for (l = list->list.next; l != &list->list; l = l->next) {
      node = DRMLISTENTRY(drmBONode, l, head);
      arg = &node->bo_arg;
      rep = &arg->d.rep.bo_info;

      if (!arg->handled) {
	 return -EFAULT;
      }
      if (arg->d.rep.ret)
	 return arg->d.rep.ret;

      buf = node->buf;
      i915_drm_copy_reply(rep, buf);
   }
   return 0;
}

/* TODO: Push this whole function into bufmgr.
 */
static struct _DriFenceObject *
do_flush_locked(struct intel_batchbuffer *batch,
                GLuint used,
                GLboolean ignore_cliprects, GLboolean allow_unlock)
{
   struct intel_context *intel = batch->intel;
   struct _DriFenceObject *fo;
   drmFence fence;
   drmBOList *boList;
   struct drm_i915_execbuffer ea;
   int ret = 0;

   driBOValidateUserList(batch->list);
   boList = driGetdrmBOList(batch->list);

#if 0 /* ZZZ JB Allways run */
   if (!(intel->numClipRects == 0 && !ignore_cliprects)) {
#else
   if (1) {
#endif
      ret = i915_execbuf(batch, used, ignore_cliprects, boList, &ea);
   } else {
     driPutdrmBOList(batch->list);
     fo = NULL;
     goto out;
   }
   driPutdrmBOList(batch->list);
   if (ret)
      abort();

   if (ea.fence_arg.error != 0) {

     /*
      * The hardware has been idled by the kernel.
      * Don't fence the driBOs.
      */

       if (batch->last_fence)
	   driFenceUnReference(&batch->last_fence);
#if 0 /* ZZZ JB: no _mesa_* funcs in gallium */
       _mesa_printf("fence error\n");
#endif
       batch->last_fence = NULL;
       fo = NULL;
       goto out;
   }

   fence.handle = ea.fence_arg.handle;
   fence.fence_class = ea.fence_arg.fence_class;
   fence.type = ea.fence_arg.type;
   fence.flags = ea.fence_arg.flags;
   fence.signaled = ea.fence_arg.signaled;

   fo = driBOFenceUserList(batch->intel->intelScreen->mgr, batch->list,
			   "SuperFence", &fence);

   if (driFenceType(fo) & DRM_I915_FENCE_TYPE_RW) {
       if (batch->last_fence)
	   driFenceUnReference(&batch->last_fence);
   /*
	* FIXME: Context last fence??
	*/
       batch->last_fence = fo;
       driFenceReference(fo);
   } 
 out:
#if 0 /* ZZZ JB: fix this */
   intel->vtbl.lost_hardware(intel);
#else
   (void)intel;
#endif
   return fo;
}


struct _DriFenceObject *
intel_batchbuffer_flush(struct intel_batchbuffer *batch)
{
   struct intel_context *intel = batch->intel;
   GLuint used = batch->ptr - batch->map;
   GLboolean was_locked = intel->locked;
   struct _DriFenceObject *fence;

   if (used == 0) {
      driFenceReference(batch->last_fence);
      return batch->last_fence;
   }

   /* Add the MI_BATCH_BUFFER_END.  Always add an MI_FLUSH - this is a
    * performance drain that we would like to avoid.
    */
#if 0 /* ZZZ JB: what should we do here? */
   if (used & 4) {
      ((int *) batch->ptr)[0] = intel->vtbl.flush_cmd();
      ((int *) batch->ptr)[1] = 0;
      ((int *) batch->ptr)[2] = MI_BATCH_BUFFER_END;
      used += 12;
   }
   else {
      ((int *) batch->ptr)[0] = intel->vtbl.flush_cmd();
      ((int *) batch->ptr)[1] = MI_BATCH_BUFFER_END;
      used += 8;
   }
#else
   if (used & 4) {
      ((int *) batch->ptr)[0] = ((0<<29)|(4<<23)); // MI_FLUSH;
      ((int *) batch->ptr)[1] = 0;
      ((int *) batch->ptr)[2] = (0xA<<23); // MI_BATCH_BUFFER_END;
      used += 12;
   }
   else {
      ((int *) batch->ptr)[0] = ((0<<29)|(4<<23)); // MI_FLUSH;
      ((int *) batch->ptr)[1] = (0xA<<23); // MI_BATCH_BUFFER_END;
      used += 8;
   }
#endif
   driBOUnmap(batch->buffer);
   batch->ptr = NULL;
   batch->map = NULL;

   /* TODO: Just pass the relocation list and dma buffer up to the
    * kernel.
    */
   if (!was_locked)
      LOCK_HARDWARE(intel);

   fence = do_flush_locked(batch, used, !(batch->flags & INTEL_BATCH_CLIPRECTS),
			   GL_FALSE);

   if (!was_locked)
      UNLOCK_HARDWARE(intel);

   /* Reset the buffer:
    */
   intel_batchbuffer_reset(batch);
   return fence;
}

void
intel_batchbuffer_finish(struct intel_batchbuffer *batch)
{
   struct _DriFenceObject *fence = intel_batchbuffer_flush(batch);
   driFenceFinish(fence, driFenceType(fence), GL_FALSE);
   driFenceUnReference(&fence);
}

void
intel_batchbuffer_data(struct intel_batchbuffer *batch,
                       const void *data, GLuint bytes, GLuint flags)
{
   assert((bytes & 3) == 0);
   intel_batchbuffer_require_space(batch, bytes, flags);
   memcpy(batch->ptr, data, bytes);
   batch->ptr += bytes;
}
