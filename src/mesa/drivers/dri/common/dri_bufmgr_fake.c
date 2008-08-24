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

/* Originally a fake version of the buffer manager so that we can
 * prototype the changes in a driver fairly quickly, has been fleshed
 * out to a fully functional interim solution.
 *
 * Basically wraps the old style memory management in the new
 * programming interface, but is more expressive and avoids many of
 * the bugs in the old texture manager.
 */
#include "mtypes.h"
#include "dri_bufmgr.h"
#include "drm.h"

#include "simple_list.h"
#include "mm.h"
#include "imports.h"

#define DBG(...) do {					\
   if (bufmgr_fake->bufmgr.debug)			\
      _mesa_printf(__VA_ARGS__);			\
} while (0)

/* Internal flags:
 */
#define BM_NO_BACKING_STORE			0x00000001
#define BM_NO_FENCE_SUBDATA			0x00000002
#define BM_PINNED				0x00000004

/* Wrapper around mm.c's mem_block, which understands that you must
 * wait for fences to expire before memory can be freed.  This is
 * specific to our use of memcpy for uploads - an upload that was
 * processed through the command queue wouldn't need to care about
 * fences.
 */
#define MAX_RELOCS 4096

struct fake_buffer_reloc
{
   /** Buffer object that the relocation points at. */
   dri_bo *target_buf;
   /** Offset of the relocation entry within reloc_buf. */
   GLuint offset;
   /** Cached value of the offset when we last performed this relocation. */
   GLuint last_target_offset;
   /** Value added to target_buf's offset to get the relocation entry. */
   GLuint delta;
   /** Flags to validate the target buffer under. */
   uint64_t validate_flags;
};

struct block {
   struct block *next, *prev;
   struct mem_block *mem;	/* BM_MEM_AGP */

   /**
    * Marks that the block is currently in the aperture and has yet to be
    * fenced.
    */
   unsigned on_hardware:1;
   /**
    * Marks that the block is currently fenced (being used by rendering) and
    * can't be freed until @fence is passed.
    */
   unsigned fenced:1;

   /** Fence cookie for the block. */
   unsigned fence; /* Split to read_fence, write_fence */

   dri_bo *bo;
   void *virtual;
};

typedef struct _bufmgr_fake {
   dri_bufmgr bufmgr;

   unsigned long low_offset;
   unsigned long size;
   void *virtual;

   struct mem_block *heap;
   struct block lru;		/* only allocated, non-fence-pending blocks here */

   unsigned buf_nr;		/* for generating ids */

   struct block on_hardware;	/* after bmValidateBuffers */
   struct block fenced;		/* after bmFenceBuffers (mi_flush, emit irq, write dword) */
                                /* then to bufmgr->lru or free() */

   unsigned int last_fence;

   unsigned fail:1;
   unsigned need_fence:1;
   GLboolean thrashing;

   /**
    * Driver callback to emit a fence, returning the cookie.
    *
    * Currently, this also requires that a write flush be emitted before
    * emitting the fence, but this should change.
    */
   unsigned int (*fence_emit)(void *private);
   /** Driver callback to wait for a fence cookie to have passed. */
   int (*fence_wait)(void *private, unsigned int fence_cookie);
   /** Driver-supplied argument to driver callbacks */
   void *driver_priv;

   GLboolean debug;

   GLboolean performed_rendering;

   /* keep track of the current total size of objects we have relocs for */
   unsigned long current_total_size;
} dri_bufmgr_fake;

typedef struct _dri_bo_fake {
   dri_bo bo;

   unsigned id;			/* debug only */
   const char *name;

   unsigned dirty:1;
   unsigned size_accounted:1; /*this buffers size has been accounted against the aperture */
   unsigned card_dirty:1; /* has the card written to this buffer - we make need to copy it back */
   unsigned int refcount;
   /* Flags may consist of any of the DRM_BO flags, plus
    * DRM_BO_NO_BACKING_STORE and BM_NO_FENCE_SUBDATA, which are the first two
    * driver private flags.
    */
   uint64_t flags;
   unsigned int alignment;
   GLboolean is_static, validated;
   unsigned int map_count;

   /* Flags for the buffer to be validated with in command submission */
   uint64_t validate_flags;

   /** relocation list */
   struct fake_buffer_reloc *relocs;
   GLuint nr_relocs;

   struct block *block;
   void *backing_store;
   void (*invalidate_cb)(dri_bo *bo, void *ptr);
   void *invalidate_ptr;
} dri_bo_fake;

typedef struct _dri_fence_fake {
   dri_fence fence;

   const char *name;
   unsigned int refcount;
   unsigned int fence_cookie;
   GLboolean flushed;
} dri_fence_fake;

static int clear_fenced(dri_bufmgr_fake *bufmgr_fake,
			unsigned int fence_cookie);

static int dri_fake_check_aperture_space(dri_bo *bo);

#define MAXFENCE 0x7fffffff

static GLboolean FENCE_LTE( unsigned a, unsigned b )
{
   if (a == b)
      return GL_TRUE;

   if (a < b && b - a < (1<<24))
      return GL_TRUE;

   if (a > b && MAXFENCE - a + b < (1<<24))
      return GL_TRUE;

   return GL_FALSE;
}

static unsigned int
_fence_emit_internal(dri_bufmgr_fake *bufmgr_fake)
{
   bufmgr_fake->last_fence = bufmgr_fake->fence_emit(bufmgr_fake->driver_priv);
   return bufmgr_fake->last_fence;
}

static void
_fence_wait_internal(dri_bufmgr_fake *bufmgr_fake, unsigned int cookie)
{
   int ret;

   ret = bufmgr_fake->fence_wait(bufmgr_fake->driver_priv, cookie);
   if (ret != 0) {
      _mesa_printf("%s:%d: Error %d waiting for fence.\n",
		   __FILE__, __LINE__);
      abort();
   }
   clear_fenced(bufmgr_fake, cookie);
}

static GLboolean
_fence_test(dri_bufmgr_fake *bufmgr_fake, unsigned fence)
{
   /* Slight problem with wrap-around:
    */
   return fence == 0 || FENCE_LTE(fence, bufmgr_fake->last_fence);
}

/**
 * Allocate a memory manager block for the buffer.
 */
static GLboolean
alloc_block(dri_bo *bo)
{
   dri_bo_fake *bo_fake = (dri_bo_fake *)bo;
   dri_bufmgr_fake *bufmgr_fake= (dri_bufmgr_fake *)bo->bufmgr;
   struct block *block = (struct block *)calloc(sizeof *block, 1);
   unsigned int align_log2 = _mesa_ffs(bo_fake->alignment) - 1;
   GLuint sz;

   if (!block)
      return GL_FALSE;

   sz = (bo->size + bo_fake->alignment - 1) & ~(bo_fake->alignment - 1);

   block->mem = mmAllocMem(bufmgr_fake->heap, sz, align_log2, 0);
   if (!block->mem) {
      free(block);
      return GL_FALSE;
   }

   make_empty_list(block);

   /* Insert at head or at tail???   
    */
   insert_at_tail(&bufmgr_fake->lru, block);

   block->virtual = bufmgr_fake->virtual +
      block->mem->ofs - bufmgr_fake->low_offset;
   block->bo = bo;

   bo_fake->block = block;

   return GL_TRUE;
}

/* Release the card storage associated with buf:
 */
static void free_block(dri_bufmgr_fake *bufmgr_fake, struct block *block)
{
   dri_bo_fake *bo_fake;
   DBG("free block %p %08x %d %d\n", block, block->mem->ofs, block->on_hardware, block->fenced);

   if (!block)
      return;

   bo_fake = (dri_bo_fake *)block->bo;
   if (!(bo_fake->flags & BM_NO_BACKING_STORE) && (bo_fake->card_dirty == 1)) {
     memcpy(bo_fake->backing_store, block->virtual, block->bo->size);
     bo_fake->card_dirty = 1;
     bo_fake->dirty = 1;
   }

   if (block->on_hardware) {
      block->bo = NULL;
   }
   else if (block->fenced) {
      block->bo = NULL;
   }
   else {
      DBG("    - free immediately\n");
      remove_from_list(block);

      mmFreeMem(block->mem);
      free(block);
   }
}

static void
alloc_backing_store(dri_bo *bo)
{
   dri_bufmgr_fake *bufmgr_fake = (dri_bufmgr_fake *)bo->bufmgr;
   dri_bo_fake *bo_fake = (dri_bo_fake *)bo;
   assert(!bo_fake->backing_store);
   assert(!(bo_fake->flags & (BM_PINNED|BM_NO_BACKING_STORE)));

   bo_fake->backing_store = ALIGN_MALLOC(bo->size, 64);

   DBG("alloc_backing - buf %d %p %d\n", bo_fake->id, bo_fake->backing_store, bo->size);
   assert(bo_fake->backing_store);
}

static void
free_backing_store(dri_bo *bo)
{
   dri_bo_fake *bo_fake = (dri_bo_fake *)bo;

   if (bo_fake->backing_store) {
      assert(!(bo_fake->flags & (BM_PINNED|BM_NO_BACKING_STORE)));
      ALIGN_FREE(bo_fake->backing_store);
      bo_fake->backing_store = NULL;
   }
}

static void
set_dirty(dri_bo *bo)
{
   dri_bufmgr_fake *bufmgr_fake = (dri_bufmgr_fake *)bo->bufmgr;
   dri_bo_fake *bo_fake = (dri_bo_fake *)bo;

   if (bo_fake->flags & BM_NO_BACKING_STORE && bo_fake->invalidate_cb != NULL)
      bo_fake->invalidate_cb(bo, bo_fake->invalidate_ptr);

   assert(!(bo_fake->flags & BM_PINNED));

   DBG("set_dirty - buf %d\n", bo_fake->id);
   bo_fake->dirty = 1;
}

static GLboolean
evict_lru(dri_bufmgr_fake *bufmgr_fake, GLuint max_fence)
{
   struct block *block, *tmp;

   DBG("%s\n", __FUNCTION__);

   foreach_s(block, tmp, &bufmgr_fake->lru) {
      dri_bo_fake *bo_fake = (dri_bo_fake *)block->bo;

      if (bo_fake != NULL && (bo_fake->flags & BM_NO_FENCE_SUBDATA))
	 continue;

      if (block->fence && max_fence && !FENCE_LTE(block->fence, max_fence))
	 return 0;

      set_dirty(&bo_fake->bo);
      bo_fake->block = NULL;

      free_block(bufmgr_fake, block);
      return GL_TRUE;
   }

   return GL_FALSE;
}

#define foreach_s_rev(ptr, t, list)   \
        for(ptr=(list)->prev,t=(ptr)->prev; list != ptr; ptr=t, t=(t)->prev)

static GLboolean
evict_mru(dri_bufmgr_fake *bufmgr_fake)
{
   struct block *block, *tmp;

   DBG("%s\n", __FUNCTION__);

   foreach_s_rev(block, tmp, &bufmgr_fake->lru) {
      dri_bo_fake *bo_fake = (dri_bo_fake *)block->bo;

      if (bo_fake && (bo_fake->flags & BM_NO_FENCE_SUBDATA))
	 continue;

      set_dirty(&bo_fake->bo);
      bo_fake->block = NULL;

      free_block(bufmgr_fake, block);
      return GL_TRUE;
   }

   return GL_FALSE;
}

/**
 * Removes all objects from the fenced list older than the given fence.
 */
static int clear_fenced(dri_bufmgr_fake *bufmgr_fake,
			unsigned int fence_cookie)
{
   struct block *block, *tmp;
   int ret = 0;

   foreach_s(block, tmp, &bufmgr_fake->fenced) {
      assert(block->fenced);

      if (_fence_test(bufmgr_fake, block->fence)) {

	 block->fenced = 0;

	 if (!block->bo) {
	    DBG("delayed free: offset %x sz %x\n",
		block->mem->ofs, block->mem->size);
	    remove_from_list(block);
	    mmFreeMem(block->mem);
	    free(block);
	 }
	 else {
	    DBG("return to lru: offset %x sz %x\n",
		block->mem->ofs, block->mem->size);
	    move_to_tail(&bufmgr_fake->lru, block);
	 }

	 ret = 1;
      }
      else {
	 /* Blocks are ordered by fence, so if one fails, all from
	  * here will fail also:
	  */
	DBG("fence not passed: offset %x sz %x %d %d \n",
	    block->mem->ofs, block->mem->size, block->fence, bufmgr_fake->last_fence);
	 break;
      }
   }

   DBG("%s: %d\n", __FUNCTION__, ret);
   return ret;
}

static void fence_blocks(dri_bufmgr_fake *bufmgr_fake, unsigned fence)
{
   struct block *block, *tmp;

   foreach_s (block, tmp, &bufmgr_fake->on_hardware) {
      DBG("Fence block %p (sz 0x%x ofs %x buf %p) with fence %d\n", block,
	  block->mem->size, block->mem->ofs, block->bo, fence);
      block->fence = fence;

      block->on_hardware = 0;
      block->fenced = 1;

      /* Move to tail of pending list here
       */
      move_to_tail(&bufmgr_fake->fenced, block);
   }

   assert(is_empty_list(&bufmgr_fake->on_hardware));
}

static GLboolean evict_and_alloc_block(dri_bo *bo)
{
   dri_bufmgr_fake *bufmgr_fake = (dri_bufmgr_fake *)bo->bufmgr;
   dri_bo_fake *bo_fake = (dri_bo_fake *)bo;

   assert(bo_fake->block == NULL);

   /* Search for already free memory:
    */
   if (alloc_block(bo))
      return GL_TRUE;

   /* If we're not thrashing, allow lru eviction to dig deeper into
    * recently used textures.  We'll probably be thrashing soon:
    */
   if (!bufmgr_fake->thrashing) {
      while (evict_lru(bufmgr_fake, 0))
	 if (alloc_block(bo))
	    return GL_TRUE;
   }

   /* Keep thrashing counter alive?
    */
   if (bufmgr_fake->thrashing)
      bufmgr_fake->thrashing = 20;

   /* Wait on any already pending fences - here we are waiting for any
    * freed memory that has been submitted to hardware and fenced to
    * become available:
    */
   while (!is_empty_list(&bufmgr_fake->fenced)) {
      GLuint fence = bufmgr_fake->fenced.next->fence;
      _fence_wait_internal(bufmgr_fake, fence);

      if (alloc_block(bo))
	 return GL_TRUE;
   }

   if (!is_empty_list(&bufmgr_fake->on_hardware)) {
      while (!is_empty_list(&bufmgr_fake->fenced)) {
	 GLuint fence = bufmgr_fake->fenced.next->fence;
	 _fence_wait_internal(bufmgr_fake, fence);
      }

      if (!bufmgr_fake->thrashing) {
	 DBG("thrashing\n");
      }
      bufmgr_fake->thrashing = 20;

      if (alloc_block(bo))
	 return GL_TRUE;
   }

   while (evict_mru(bufmgr_fake))
      if (alloc_block(bo))
	 return GL_TRUE;

   DBG("%s 0x%x bytes failed\n", __FUNCTION__, bo->size);

   return GL_FALSE;
}

/***********************************************************************
 * Public functions
 */

/**
 * Wait for hardware idle by emitting a fence and waiting for it.
 */
static void
dri_bufmgr_fake_wait_idle(dri_bufmgr_fake *bufmgr_fake)
{
   unsigned int cookie;

   cookie = bufmgr_fake->fence_emit(bufmgr_fake->driver_priv);
   _fence_wait_internal(bufmgr_fake, cookie);
}

/**
 * Wait for execution pending on a buffer
 */
static void
dri_bufmgr_fake_bo_wait_idle(dri_bo *bo)
{
   dri_bufmgr_fake *bufmgr_fake = (dri_bufmgr_fake *)bo->bufmgr;
   dri_bo_fake *bo_fake = (dri_bo_fake *)bo;

   if (bo_fake->block == NULL || !bo_fake->block->fenced)
      return;

   _fence_wait_internal(bufmgr_fake, bo_fake->block->fence);
}

/* Specifically ignore texture memory sharing.
 *  -- just evict everything
 *  -- and wait for idle
 */
void
dri_bufmgr_fake_contended_lock_take(dri_bufmgr *bufmgr)
{
   dri_bufmgr_fake *bufmgr_fake = (dri_bufmgr_fake *)bufmgr;
   struct block *block, *tmp;

   bufmgr_fake->need_fence = 1;
   bufmgr_fake->fail = 0;

   /* Wait for hardware idle.  We don't know where acceleration has been
    * happening, so we'll need to wait anyway before letting anything get
    * put on the card again.
    */
   dri_bufmgr_fake_wait_idle(bufmgr_fake);

   /* Check that we hadn't released the lock without having fenced the last
    * set of buffers.
    */
   assert(is_empty_list(&bufmgr_fake->fenced));
   assert(is_empty_list(&bufmgr_fake->on_hardware));

   foreach_s(block, tmp, &bufmgr_fake->lru) {
      assert(_fence_test(bufmgr_fake, block->fence));
      set_dirty(block->bo);
   }
}

static dri_bo *
dri_fake_bo_alloc(dri_bufmgr *bufmgr, const char *name,
		  unsigned long size, unsigned int alignment,
		  uint64_t location_mask)
{
   dri_bufmgr_fake *bufmgr_fake;
   dri_bo_fake *bo_fake;

   bufmgr_fake = (dri_bufmgr_fake *)bufmgr;

   assert(size != 0);

   bo_fake = calloc(1, sizeof(*bo_fake));
   if (!bo_fake)
      return NULL;

   bo_fake->bo.size = size;
   bo_fake->bo.offset = -1;
   bo_fake->bo.virtual = NULL;
   bo_fake->bo.bufmgr = bufmgr;
   bo_fake->refcount = 1;

   /* Alignment must be a power of two */
   assert((alignment & (alignment - 1)) == 0);
   if (alignment == 0)
      alignment = 1;
   bo_fake->alignment = alignment;
   bo_fake->id = ++bufmgr_fake->buf_nr;
   bo_fake->name = name;
   bo_fake->flags = 0;
   bo_fake->is_static = GL_FALSE;

   DBG("drm_bo_alloc: (buf %d: %s, %d kb)\n", bo_fake->id, bo_fake->name,
       bo_fake->bo.size / 1024);

   return &bo_fake->bo;
}

static dri_bo *
dri_fake_bo_alloc_static(dri_bufmgr *bufmgr, const char *name,
			 unsigned long offset, unsigned long size,
			 void *virtual, uint64_t location_mask)
{
   dri_bufmgr_fake *bufmgr_fake;
   dri_bo_fake *bo_fake;

   bufmgr_fake = (dri_bufmgr_fake *)bufmgr;

   assert(size != 0);

   bo_fake = calloc(1, sizeof(*bo_fake));
   if (!bo_fake)
      return NULL;

   bo_fake->bo.size = size;
   bo_fake->bo.offset = offset;
   bo_fake->bo.virtual = virtual;
   bo_fake->bo.bufmgr = bufmgr;
   bo_fake->refcount = 1;
   bo_fake->id = ++bufmgr_fake->buf_nr;
   bo_fake->name = name;
   bo_fake->flags = BM_PINNED | DRM_BO_FLAG_NO_MOVE;
   bo_fake->is_static = GL_TRUE;

   DBG("drm_bo_alloc_static: (buf %d: %s, %d kb)\n", bo_fake->id, bo_fake->name,
       bo_fake->bo.size / 1024);

   return &bo_fake->bo;
}

static void
dri_fake_bo_reference(dri_bo *bo)
{
   dri_bo_fake *bo_fake = (dri_bo_fake *)bo;

   bo_fake->refcount++;
}

static void
dri_fake_bo_unreference(dri_bo *bo)
{
   dri_bufmgr_fake *bufmgr_fake = (dri_bufmgr_fake *)bo->bufmgr;
   dri_bo_fake *bo_fake = (dri_bo_fake *)bo;
   int i;

   if (!bo)
      return;

   if (--bo_fake->refcount == 0) {
      assert(bo_fake->map_count == 0);
      /* No remaining references, so free it */
      if (bo_fake->block)
	 free_block(bufmgr_fake, bo_fake->block);
      free_backing_store(bo);

      for (i = 0; i < bo_fake->nr_relocs; i++)
	 dri_bo_unreference(bo_fake->relocs[i].target_buf);

      DBG("drm_bo_unreference: free buf %d %s\n", bo_fake->id, bo_fake->name);

      free(bo_fake->relocs);
      free(bo);

      return;
   }
}

/**
 * Set the buffer as not requiring backing store, and instead get the callback
 * invoked whenever it would be set dirty.
 */
void dri_bo_fake_disable_backing_store(dri_bo *bo,
				       void (*invalidate_cb)(dri_bo *bo,
							     void *ptr),
				       void *ptr)
{
   dri_bufmgr_fake *bufmgr_fake = (dri_bufmgr_fake *)bo->bufmgr;
   dri_bo_fake *bo_fake = (dri_bo_fake *)bo;

   if (bo_fake->backing_store)
      free_backing_store(bo);

   bo_fake->flags |= BM_NO_BACKING_STORE;

   DBG("disable_backing_store set buf %d dirty\n", bo_fake->id);
   bo_fake->dirty = 1;
   bo_fake->invalidate_cb = invalidate_cb;
   bo_fake->invalidate_ptr = ptr;

   /* Note that it is invalid right from the start.  Also note
    * invalidate_cb is called with the bufmgr locked, so cannot
    * itself make bufmgr calls.
    */
   if (invalidate_cb != NULL)
      invalidate_cb(bo, ptr);
}

/**
 * Map a buffer into bo->virtual, allocating either card memory space (If
 * BM_NO_BACKING_STORE or BM_PINNED) or backing store, as necessary.
 */
static int
dri_fake_bo_map(dri_bo *bo, GLboolean write_enable)
{
   dri_bufmgr_fake *bufmgr_fake = (dri_bufmgr_fake *)bo->bufmgr;
   dri_bo_fake *bo_fake = (dri_bo_fake *)bo;

   /* Static buffers are always mapped. */
   if (bo_fake->is_static)
      return 0;

   /* Allow recursive mapping.  Mesa may recursively map buffers with
    * nested display loops, and it is used internally in bufmgr_fake
    * for relocation.
    */
   if (bo_fake->map_count++ != 0)
      return 0;

   {
      DBG("drm_bo_map: (buf %d: %s, %d kb)\n", bo_fake->id, bo_fake->name,
	  bo_fake->bo.size / 1024);

      if (bo->virtual != NULL) {
	 _mesa_printf("%s: already mapped\n", __FUNCTION__);
	 abort();
      }
      else if (bo_fake->flags & (BM_NO_BACKING_STORE|BM_PINNED)) {

	 if (!bo_fake->block && !evict_and_alloc_block(bo)) {
	    DBG("%s: alloc failed\n", __FUNCTION__);
	    bufmgr_fake->fail = 1;
	    return 1;
	 }
	 else {
	    assert(bo_fake->block);
	    bo_fake->dirty = 0;

	    if (!(bo_fake->flags & BM_NO_FENCE_SUBDATA) &&
		bo_fake->block->fenced) {
	       dri_bufmgr_fake_bo_wait_idle(bo);
	    }

	    bo->virtual = bo_fake->block->virtual;
	 }
      }
      else {
	 if (write_enable)
	    set_dirty(bo);

	 if (bo_fake->backing_store == 0)
	    alloc_backing_store(bo);

	 bo->virtual = bo_fake->backing_store;
      }
   }

   return 0;
}

static int
dri_fake_bo_unmap(dri_bo *bo)
{
   dri_bufmgr_fake *bufmgr_fake = (dri_bufmgr_fake *)bo->bufmgr;
   dri_bo_fake *bo_fake = (dri_bo_fake *)bo;

   /* Static buffers are always mapped. */
   if (bo_fake->is_static)
      return 0;

   assert(bo_fake->map_count != 0);
   if (--bo_fake->map_count != 0)
      return 0;

   DBG("drm_bo_unmap: (buf %d: %s, %d kb)\n", bo_fake->id, bo_fake->name,
       bo_fake->bo.size / 1024);

   bo->virtual = NULL;

   return 0;
}

static void
dri_fake_kick_all(dri_bufmgr_fake *bufmgr_fake)
{
   struct block *block, *tmp;

   bufmgr_fake->performed_rendering = GL_FALSE;
   /* okay for ever BO that is on the HW kick it off.
      seriously not afraid of the POLICE right now */
   foreach_s(block, tmp, &bufmgr_fake->on_hardware) {
      dri_bo_fake *bo_fake = (dri_bo_fake *)block->bo;

      block->on_hardware = 0;
      free_block(bufmgr_fake, block);
      bo_fake->block = NULL;
      bo_fake->validated = GL_FALSE;
      if (!(bo_fake->flags & BM_NO_BACKING_STORE))
         bo_fake->dirty = 1;
   }
}

static int
dri_fake_bo_validate(dri_bo *bo, uint64_t flags)
{
   dri_bufmgr_fake *bufmgr_fake;
   dri_bo_fake *bo_fake = (dri_bo_fake *)bo;

   /* XXX: Sanity-check whether we've already validated this one under
    * different flags.  See drmAddValidateItem().
    */
   bufmgr_fake = (dri_bufmgr_fake *)bo->bufmgr;

   DBG("drm_bo_validate: (buf %d: %s, %d kb)\n", bo_fake->id, bo_fake->name,
       bo_fake->bo.size / 1024);

   /* Sanity check: Buffers should be unmapped before being validated.
    * This is not so much of a problem for bufmgr_fake, but TTM refuses,
    * and the problem is harder to debug there.
    */
   assert(bo_fake->map_count == 0);

   if (bo_fake->is_static) {
      /* Add it to the needs-fence list */
      bufmgr_fake->need_fence = 1;
      return 0;
   }

   /* reset size accounted */
   bo_fake->size_accounted = 0;

   /* Allocate the card memory */
   if (!bo_fake->block && !evict_and_alloc_block(bo)) {
      bufmgr_fake->fail = 1;
      DBG("Failed to validate buf %d:%s\n", bo_fake->id, bo_fake->name);
      return -1;
   }

   assert(bo_fake->block);
   assert(bo_fake->block->bo == &bo_fake->bo);

   bo->offset = bo_fake->block->mem->ofs;

   /* Upload the buffer contents if necessary */
   if (bo_fake->dirty) {
      DBG("Upload dirty buf %d:%s, sz %d offset 0x%x\n", bo_fake->id,
	  bo_fake->name, bo->size, bo_fake->block->mem->ofs);

      assert(!(bo_fake->flags &
	       (BM_NO_BACKING_STORE|BM_PINNED)));

      /* Actually, should be able to just wait for a fence on the memory,
       * which we would be tracking when we free it.  Waiting for idle is
       * a sufficiently large hammer for now.
       */
      dri_bufmgr_fake_wait_idle(bufmgr_fake);

      /* we may never have mapped this BO so it might not have any backing
       * store if this happens it should be rare, but 0 the card memory
       * in any case */
      if (bo_fake->backing_store)
         memcpy(bo_fake->block->virtual, bo_fake->backing_store, bo->size);
      else
         memset(bo_fake->block->virtual, 0, bo->size);

      bo_fake->dirty = 0;
   }

   bo_fake->block->fenced = 0;
   bo_fake->block->on_hardware = 1;
   move_to_tail(&bufmgr_fake->on_hardware, bo_fake->block);

   bo_fake->validated = GL_TRUE;
   bufmgr_fake->need_fence = 1;

   return 0;
}

static dri_fence *
dri_fake_fence_validated(dri_bufmgr *bufmgr, const char *name,
			 GLboolean flushed)
{
   dri_fence_fake *fence_fake;
   dri_bufmgr_fake *bufmgr_fake = (dri_bufmgr_fake *)bufmgr;
   unsigned int cookie;

   fence_fake = malloc(sizeof(*fence_fake));
   if (!fence_fake)
      return NULL;

   fence_fake->refcount = 1;
   fence_fake->name = name;
   fence_fake->flushed = flushed;
   fence_fake->fence.bufmgr = bufmgr;

   cookie = _fence_emit_internal(bufmgr_fake);
   fence_fake->fence_cookie = cookie;
   fence_blocks(bufmgr_fake, cookie);

   DBG("drm_fence_validated: 0x%08x cookie\n", fence_fake->fence_cookie);

   return &fence_fake->fence;
}

static void
dri_fake_fence_reference(dri_fence *fence)
{
   dri_fence_fake *fence_fake = (dri_fence_fake *)fence;

   ++fence_fake->refcount;
}

static void
dri_fake_fence_unreference(dri_fence *fence)
{
   dri_fence_fake *fence_fake = (dri_fence_fake *)fence;

   if (!fence)
      return;

   if (--fence_fake->refcount == 0) {
      free(fence);
      return;
   }
}

static void
dri_fake_fence_wait(dri_fence *fence)
{
   dri_fence_fake *fence_fake = (dri_fence_fake *)fence;
   dri_bufmgr_fake *bufmgr_fake = (dri_bufmgr_fake *)fence->bufmgr;

   DBG("drm_fence_wait: 0x%08x cookie\n", fence_fake->fence_cookie);

   _fence_wait_internal(bufmgr_fake, fence_fake->fence_cookie);
}

static void
dri_fake_destroy(dri_bufmgr *bufmgr)
{
   dri_bufmgr_fake *bufmgr_fake = (dri_bufmgr_fake *)bufmgr;

   mmDestroy(bufmgr_fake->heap);
   free(bufmgr);
}

static int
dri_fake_emit_reloc(dri_bo *reloc_buf, uint64_t flags, GLuint delta,
		    GLuint offset, dri_bo *target_buf)
{
   dri_bufmgr_fake *bufmgr_fake = (dri_bufmgr_fake *)reloc_buf->bufmgr;
   struct fake_buffer_reloc *r;
   dri_bo_fake *reloc_fake = (dri_bo_fake *)reloc_buf;
   dri_bo_fake *target_fake = (dri_bo_fake *)target_buf;
   int i;

   assert(reloc_buf);
   assert(target_buf);

   assert(target_fake->is_static || target_fake->size_accounted);

   if (reloc_fake->relocs == NULL) {
      reloc_fake->relocs = malloc(sizeof(struct fake_buffer_reloc) *
				  MAX_RELOCS);
   }

   r = &reloc_fake->relocs[reloc_fake->nr_relocs++];

   assert(reloc_fake->nr_relocs <= MAX_RELOCS);

   dri_bo_reference(target_buf);

   r->target_buf = target_buf;
   r->offset = offset;
   r->last_target_offset = target_buf->offset;
   r->delta = delta;
   r->validate_flags = flags;

   if (bufmgr_fake->debug) {
      /* Check that a conflicting relocation hasn't already been emitted. */
      for (i = 0; i < reloc_fake->nr_relocs - 1; i++) {
	 struct fake_buffer_reloc *r2 = &reloc_fake->relocs[i];

	 assert(r->offset != r2->offset);
      }
   }

   return 0;
}

/**
 * Incorporates the validation flags associated with each relocation into
 * the combined validation flags for the buffer on this batchbuffer submission.
 */
static void
dri_fake_calculate_validate_flags(dri_bo *bo)
{
   dri_bo_fake *bo_fake = (dri_bo_fake *)bo;
   int i;

   for (i = 0; i < bo_fake->nr_relocs; i++) {
      struct fake_buffer_reloc *r = &bo_fake->relocs[i];
      dri_bo_fake *target_fake = (dri_bo_fake *)r->target_buf;

      /* Do the same for the tree of buffers we depend on */
      dri_fake_calculate_validate_flags(r->target_buf);

      if (target_fake->validate_flags == 0) {
	 target_fake->validate_flags = r->validate_flags;
      } else {
	 /* Mask the memory location to the intersection of all the memory
	  * locations the buffer is being validated to.
	  */
	 target_fake->validate_flags =
	    (target_fake->validate_flags & ~DRM_BO_MASK_MEM) |
	    (r->validate_flags & target_fake->validate_flags &
	     DRM_BO_MASK_MEM);
	 /* All the other flags just accumulate. */
	 target_fake->validate_flags |= r->validate_flags & ~DRM_BO_MASK_MEM;
      }
   }
}


static int
dri_fake_reloc_and_validate_buffer(dri_bo *bo)
{
   dri_bufmgr_fake *bufmgr_fake = (dri_bufmgr_fake *)bo->bufmgr;
   dri_bo_fake *bo_fake = (dri_bo_fake *)bo;
   int i, ret;

   assert(bo_fake->map_count == 0);

   for (i = 0; i < bo_fake->nr_relocs; i++) {
      struct fake_buffer_reloc *r = &bo_fake->relocs[i];
      dri_bo_fake *target_fake = (dri_bo_fake *)r->target_buf;
      uint32_t reloc_data;

      /* Validate the target buffer if that hasn't been done. */
      if (!target_fake->validated) {
         ret = dri_fake_reloc_and_validate_buffer(r->target_buf);
         if (ret != 0) {
            if (bo->virtual != NULL)
                dri_bo_unmap(bo);
            return ret;
         }
      }

      /* Calculate the value of the relocation entry. */
      if (r->target_buf->offset != r->last_target_offset) {
	 reloc_data = r->target_buf->offset + r->delta;

	 if (bo->virtual == NULL)
	    dri_bo_map(bo, GL_TRUE);

	 *(uint32_t *)(bo->virtual + r->offset) = reloc_data;

	 r->last_target_offset = r->target_buf->offset;
      }
   }

   if (bo->virtual != NULL)
      dri_bo_unmap(bo);

   if (bo_fake->validate_flags & DRM_BO_FLAG_WRITE) {
      if (!(bo_fake->flags & (BM_NO_BACKING_STORE|BM_PINNED))) {
         if (bo_fake->backing_store == 0)
            alloc_backing_store(bo);

         bo_fake->card_dirty = 1;
      }
      bufmgr_fake->performed_rendering = GL_TRUE;
   }

   return dri_fake_bo_validate(bo, bo_fake->validate_flags);
}

static void *
dri_fake_process_relocs(dri_bo *batch_buf, GLuint *count_p)
{
   dri_bufmgr_fake *bufmgr_fake = (dri_bufmgr_fake *)batch_buf->bufmgr;
   dri_bo_fake *batch_fake = (dri_bo_fake *)batch_buf;
   int ret;
   int retry_count = 0;

   bufmgr_fake->performed_rendering = GL_FALSE;

   dri_fake_calculate_validate_flags(batch_buf);

   batch_fake->validate_flags = DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_READ;

   /* we've ran out of RAM so blow the whole lot away and retry */
 restart:
   ret = dri_fake_reloc_and_validate_buffer(batch_buf);
   if (bufmgr_fake->fail == 1) {
      if (retry_count == 0) {
         retry_count++;
         dri_fake_kick_all(bufmgr_fake);
         bufmgr_fake->fail = 0;
         goto restart;
      } else /* dump out the memory here */
         mmDumpMemInfo(bufmgr_fake->heap);
   }

   assert(ret == 0);

   *count_p = 0; /* junk */

   bufmgr_fake->current_total_size = 0;
   return NULL;
}

static void
dri_bo_fake_post_submit(dri_bo *bo)
{
   dri_bufmgr_fake *bufmgr_fake = (dri_bufmgr_fake *)bo->bufmgr;
   dri_bo_fake *bo_fake = (dri_bo_fake *)bo;
   int i;

   for (i = 0; i < bo_fake->nr_relocs; i++) {
      struct fake_buffer_reloc *r = &bo_fake->relocs[i];
      dri_bo_fake *target_fake = (dri_bo_fake *)r->target_buf;

      if (target_fake->validated)
	 dri_bo_fake_post_submit(r->target_buf);

      DBG("%s@0x%08x + 0x%08x -> %s@0x%08x + 0x%08x\n",
	  bo_fake->name, (uint32_t)bo->offset, r->offset,
	  target_fake->name, (uint32_t)r->target_buf->offset, r->delta);
   }

   assert(bo_fake->map_count == 0);
   bo_fake->validated = GL_FALSE;
   bo_fake->validate_flags = 0;
}


static void
dri_fake_post_submit(dri_bo *batch_buf, dri_fence **last_fence)
{
   dri_bufmgr_fake *bufmgr_fake = (dri_bufmgr_fake *)batch_buf->bufmgr;
   dri_fence *fo;

   fo = dri_fake_fence_validated(batch_buf->bufmgr, "Batch fence", GL_TRUE);

   if (bufmgr_fake->performed_rendering) {
      dri_fence_unreference(*last_fence);
      *last_fence = fo;
   } else {
      dri_fence_unreference(fo);
   }

   dri_bo_fake_post_submit(batch_buf);
}

static int
dri_fake_check_aperture_space(dri_bo *bo)
{
   dri_bufmgr_fake *bufmgr_fake = (dri_bufmgr_fake *)bo->bufmgr;
   dri_bo_fake *bo_fake = (dri_bo_fake *)bo;
   GLuint sz;

   sz = (bo->size + bo_fake->alignment - 1) & ~(bo_fake->alignment - 1);

   if (bo_fake->size_accounted || bo_fake->is_static)
      return 0;

   if (bufmgr_fake->current_total_size + sz > bufmgr_fake->size) {
     DBG("check_space: %s bo %d %d overflowed bufmgr size %d\n", bo_fake->name, bo_fake->id, sz, bufmgr_fake->size);
      return -1;
   }

   bufmgr_fake->current_total_size += sz;
   bo_fake->size_accounted = 1;
   DBG("drm_check_space: buf %d, %s %d %d\n", bo_fake->id, bo_fake->name, bo->size, bufmgr_fake->current_total_size);
   return 0;
}

dri_bufmgr *
dri_bufmgr_fake_init(unsigned long low_offset, void *low_virtual,
		     unsigned long size,
		     unsigned int (*fence_emit)(void *private),
		     int (*fence_wait)(void *private, unsigned int cookie),
		     void *driver_priv)
{
   dri_bufmgr_fake *bufmgr_fake;

   bufmgr_fake = calloc(1, sizeof(*bufmgr_fake));

   /* Initialize allocator */
   make_empty_list(&bufmgr_fake->fenced);
   make_empty_list(&bufmgr_fake->on_hardware);
   make_empty_list(&bufmgr_fake->lru);

   bufmgr_fake->low_offset = low_offset;
   bufmgr_fake->virtual = low_virtual;
   bufmgr_fake->size = size;
   bufmgr_fake->heap = mmInit(low_offset, size);

   /* Hook in methods */
   bufmgr_fake->bufmgr.bo_alloc = dri_fake_bo_alloc;
   bufmgr_fake->bufmgr.bo_alloc_static = dri_fake_bo_alloc_static;
   bufmgr_fake->bufmgr.bo_reference = dri_fake_bo_reference;
   bufmgr_fake->bufmgr.bo_unreference = dri_fake_bo_unreference;
   bufmgr_fake->bufmgr.bo_map = dri_fake_bo_map;
   bufmgr_fake->bufmgr.bo_unmap = dri_fake_bo_unmap;
   bufmgr_fake->bufmgr.fence_wait = dri_fake_fence_wait;
   bufmgr_fake->bufmgr.fence_reference = dri_fake_fence_reference;
   bufmgr_fake->bufmgr.fence_unreference = dri_fake_fence_unreference;
   bufmgr_fake->bufmgr.destroy = dri_fake_destroy;
   bufmgr_fake->bufmgr.emit_reloc = dri_fake_emit_reloc;
   bufmgr_fake->bufmgr.process_relocs = dri_fake_process_relocs;
   bufmgr_fake->bufmgr.post_submit = dri_fake_post_submit;
   bufmgr_fake->bufmgr.check_aperture_space = dri_fake_check_aperture_space;
   bufmgr_fake->bufmgr.debug = GL_FALSE;

   bufmgr_fake->fence_emit = fence_emit;
   bufmgr_fake->fence_wait = fence_wait;
   bufmgr_fake->driver_priv = driver_priv;

   return &bufmgr_fake->bufmgr;
}

