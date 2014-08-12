/**********************************************************
 * Copyright 2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/


#include "svga_cmd.h"

#include "util/u_debug.h"
#include "util/u_memory.h"
#include "util/u_debug_stack.h"
#include "util/u_debug_flush.h"
#include "util/u_hash_table.h"
#include "pipebuffer/pb_buffer.h"
#include "pipebuffer/pb_validate.h"

#include "svga_winsys.h"
#include "vmw_context.h"
#include "vmw_screen.h"
#include "vmw_buffer.h"
#include "vmw_surface.h"
#include "vmw_fence.h"
#include "vmw_shader.h"

#define VMW_COMMAND_SIZE (64*1024)
#define VMW_SURFACE_RELOCS (1024)
#define VMW_SHADER_RELOCS (1024)
#define VMW_REGION_RELOCS (512)

#define VMW_MUST_FLUSH_STACK 8

/*
 * A factor applied to the maximum mob memory size to determine
 * the optimial time to preemptively flush the command buffer.
 * The constant is based on some performance trials with SpecViewperf.
 */
#define VMW_MAX_MOB_MEM_FACTOR  2

/*
 * A factor applied to the maximum surface memory size to determine
 * the optimial time to preemptively flush the command buffer.
 * The constant is based on some performance trials with SpecViewperf.
 */
#define VMW_MAX_SURF_MEM_FACTOR 2


struct vmw_buffer_relocation
{
   struct pb_buffer *buffer;
   boolean is_mob;
   uint32 offset;

   union {
      struct {
	 struct SVGAGuestPtr *where;
      } region;
      struct {
	 SVGAMobId *id;
	 uint32 *offset_into_mob;
      } mob;
   };
};

struct vmw_ctx_validate_item {
   union {
      struct vmw_svga_winsys_surface *vsurf;
      struct vmw_svga_winsys_shader *vshader;
   };
   boolean referenced;
};

struct vmw_svga_winsys_context
{
   struct svga_winsys_context base;

   struct vmw_winsys_screen *vws;
   struct util_hash_table *hash;

#ifdef DEBUG
   boolean must_flush;
   struct debug_stack_frame must_flush_stack[VMW_MUST_FLUSH_STACK];
   struct debug_flush_ctx *fctx;
#endif

   struct {
      uint8_t buffer[VMW_COMMAND_SIZE];
      uint32_t size;
      uint32_t used;
      uint32_t reserved;
   } command;

   struct {
      struct vmw_ctx_validate_item items[VMW_SURFACE_RELOCS];
      uint32_t size;
      uint32_t used;
      uint32_t staged;
      uint32_t reserved;
   } surface;
   
   struct {
      struct vmw_buffer_relocation relocs[VMW_REGION_RELOCS];
      uint32_t size;
      uint32_t used;
      uint32_t staged;
      uint32_t reserved;
   } region;

   struct {
      struct vmw_ctx_validate_item items[VMW_SHADER_RELOCS];
      uint32_t size;
      uint32_t used;
      uint32_t staged;
      uint32_t reserved;
   } shader;

   struct pb_validate *validate;

   /**
    * The amount of surface, GMR or MOB memory that is referred by the commands
    * currently batched in the context command buffer.
    */
   uint64_t seen_surfaces;
   uint64_t seen_regions;
   uint64_t seen_mobs;

   /**
    * Whether this context should fail to reserve more commands, not because it
    * ran out of command space, but because a substantial ammount of GMR was
    * referred.
    */
   boolean preemptive_flush;
};


static INLINE struct vmw_svga_winsys_context *
vmw_svga_winsys_context(struct svga_winsys_context *swc)
{
   assert(swc);
   return (struct vmw_svga_winsys_context *)swc;
}


static INLINE unsigned
vmw_translate_to_pb_flags(unsigned flags)
{
   unsigned f = 0;
   if (flags & SVGA_RELOC_READ)
      f |= PB_USAGE_GPU_READ;

   if (flags & SVGA_RELOC_WRITE)
      f |= PB_USAGE_GPU_WRITE;

   return f;
}

static enum pipe_error
vmw_swc_flush(struct svga_winsys_context *swc,
              struct pipe_fence_handle **pfence)
{
   struct vmw_svga_winsys_context *vswc = vmw_svga_winsys_context(swc);
   struct pipe_fence_handle *fence = NULL;
   unsigned i;
   enum pipe_error ret;

   ret = pb_validate_validate(vswc->validate);
   assert(ret == PIPE_OK);
   if(ret == PIPE_OK) {
   
      /* Apply relocations */
      for(i = 0; i < vswc->region.used; ++i) {
         struct vmw_buffer_relocation *reloc = &vswc->region.relocs[i];
         struct SVGAGuestPtr ptr;

         if(!vmw_gmr_bufmgr_region_ptr(reloc->buffer, &ptr))
            assert(0);

         ptr.offset += reloc->offset;

	 if (reloc->is_mob) {
	    if (reloc->mob.id)
	       *reloc->mob.id = ptr.gmrId;
	    if (reloc->mob.offset_into_mob)
	       *reloc->mob.offset_into_mob = ptr.offset;
	    else {
	       assert(ptr.offset == 0);
	    }
	 } else
	    *reloc->region.where = ptr;
      }

      if (vswc->command.used || pfence != NULL)
         vmw_ioctl_command(vswc->vws,
			   vswc->base.cid,
			   0,
                           vswc->command.buffer,
                           vswc->command.used,
                           &fence);

      pb_validate_fence(vswc->validate, fence);
   }

   vswc->command.used = 0;
   vswc->command.reserved = 0;

   for(i = 0; i < vswc->surface.used + vswc->surface.staged; ++i) {
      struct vmw_ctx_validate_item *isurf = &vswc->surface.items[i];
      if (isurf->referenced)
         p_atomic_dec(&isurf->vsurf->validated);
      vmw_svga_winsys_surface_reference(&isurf->vsurf, NULL);
   }

   util_hash_table_clear(vswc->hash);
   vswc->surface.used = 0;
   vswc->surface.reserved = 0;

   for(i = 0; i < vswc->shader.used + vswc->shader.staged; ++i) {
      struct vmw_ctx_validate_item *ishader = &vswc->shader.items[i];
      if (ishader->referenced)
         p_atomic_dec(&ishader->vshader->validated);
      vmw_svga_winsys_shader_reference(&ishader->vshader, NULL);
   }

   vswc->shader.used = 0;
   vswc->shader.reserved = 0;

   vswc->region.used = 0;
   vswc->region.reserved = 0;

#ifdef DEBUG
   vswc->must_flush = FALSE;
   debug_flush_flush(vswc->fctx);
#endif
   vswc->preemptive_flush = FALSE;
   vswc->seen_surfaces = 0;
   vswc->seen_regions = 0;
   vswc->seen_mobs = 0;

   if(pfence)
      vmw_fence_reference(vswc->vws, pfence, fence);

   vmw_fence_reference(vswc->vws, &fence, NULL);

   return ret;
}


static void *
vmw_swc_reserve(struct svga_winsys_context *swc,
                uint32_t nr_bytes, uint32_t nr_relocs )
{
   struct vmw_svga_winsys_context *vswc = vmw_svga_winsys_context(swc);

#ifdef DEBUG
   /* Check if somebody forgot to check the previous failure */
   if(vswc->must_flush) {
      debug_printf("Forgot to flush:\n");
      debug_backtrace_dump(vswc->must_flush_stack, VMW_MUST_FLUSH_STACK);
      assert(!vswc->must_flush);
   }
   debug_flush_might_flush(vswc->fctx);
#endif

   assert(nr_bytes <= vswc->command.size);
   if(nr_bytes > vswc->command.size)
      return NULL;

   if(vswc->preemptive_flush ||
      vswc->command.used + nr_bytes > vswc->command.size ||
      vswc->surface.used + nr_relocs > vswc->surface.size ||
      vswc->shader.used + nr_relocs > vswc->shader.size ||
      vswc->region.used + nr_relocs > vswc->region.size) {
#ifdef DEBUG
      vswc->must_flush = TRUE;
      debug_backtrace_capture(vswc->must_flush_stack, 1,
                              VMW_MUST_FLUSH_STACK);
#endif
      return NULL;
   }

   assert(vswc->command.used + nr_bytes <= vswc->command.size);
   assert(vswc->surface.used + nr_relocs <= vswc->surface.size);
   assert(vswc->shader.used + nr_relocs <= vswc->shader.size);
   assert(vswc->region.used + nr_relocs <= vswc->region.size);
   
   vswc->command.reserved = nr_bytes;
   vswc->surface.reserved = nr_relocs;
   vswc->surface.staged = 0;
   vswc->shader.reserved = nr_relocs;
   vswc->shader.staged = 0;
   vswc->region.reserved = nr_relocs;
   vswc->region.staged = 0;
   
   return vswc->command.buffer + vswc->command.used;
}

static void
vmw_swc_context_relocation(struct svga_winsys_context *swc,
			   uint32 *cid)
{
   *cid = swc->cid;
}

static boolean
vmw_swc_add_validate_buffer(struct vmw_svga_winsys_context *vswc,
			    struct pb_buffer *pb_buf,
			    unsigned flags)
{
   enum pipe_error ret;
   unsigned translated_flags;

   /*
    * TODO: Update pb_validate to provide a similar functionality
    * (Check buffer already present before adding)
    */
   if (util_hash_table_get(vswc->hash, pb_buf) != pb_buf) {
      translated_flags = vmw_translate_to_pb_flags(flags);
      ret = pb_validate_add_buffer(vswc->validate, pb_buf, translated_flags);
      /* TODO: Update pipebuffer to reserve buffers and not fail here */
      assert(ret == PIPE_OK);
      (void)ret;
      (void)util_hash_table_set(vswc->hash, pb_buf, pb_buf);
      return TRUE;
   }

   return FALSE;
}

static void
vmw_swc_region_relocation(struct svga_winsys_context *swc,
                          struct SVGAGuestPtr *where,
                          struct svga_winsys_buffer *buffer,
                          uint32 offset,
                          unsigned flags)
{
   struct vmw_svga_winsys_context *vswc = vmw_svga_winsys_context(swc);
   struct vmw_buffer_relocation *reloc;

   assert(vswc->region.staged < vswc->region.reserved);

   reloc = &vswc->region.relocs[vswc->region.used + vswc->region.staged];
   reloc->region.where = where;

   /*
    * pb_validate holds a refcount to the buffer, so no need to
    * refcount it again in the relocation.
    */
   reloc->buffer = vmw_pb_buffer(buffer);
   reloc->offset = offset;
   reloc->is_mob = FALSE;
   ++vswc->region.staged;

   if (vmw_swc_add_validate_buffer(vswc, reloc->buffer, flags)) {
      vswc->seen_regions += reloc->buffer->size;
      if(vswc->seen_regions >= VMW_GMR_POOL_SIZE/5)
         vswc->preemptive_flush = TRUE;
   }

#ifdef DEBUG
   if (!(flags & SVGA_RELOC_INTERNAL))
      debug_flush_cb_reference(vswc->fctx, vmw_debug_flush_buf(buffer));
#endif
}

static void
vmw_swc_mob_relocation(struct svga_winsys_context *swc,
		       SVGAMobId *id,
		       uint32 *offset_into_mob,
		       struct svga_winsys_buffer *buffer,
		       uint32 offset,
		       unsigned flags)
{
   struct vmw_svga_winsys_context *vswc = vmw_svga_winsys_context(swc);
   struct vmw_buffer_relocation *reloc;

   assert(vswc->region.staged < vswc->region.reserved);

   reloc = &vswc->region.relocs[vswc->region.used + vswc->region.staged];
   reloc->mob.id = id;
   reloc->mob.offset_into_mob = offset_into_mob;

   /*
    * pb_validate holds a refcount to the buffer, so no need to
    * refcount it again in the relocation.
    */
   reloc->buffer = vmw_pb_buffer(buffer);
   reloc->offset = offset;
   reloc->is_mob = TRUE;
   ++vswc->region.staged;

   if (vmw_swc_add_validate_buffer(vswc, reloc->buffer, flags)) {
      vswc->seen_mobs += reloc->buffer->size;
      /* divide by 5, tested for best performance */
      if (vswc->seen_mobs >= vswc->vws->ioctl.max_mob_memory / VMW_MAX_MOB_MEM_FACTOR)
         vswc->preemptive_flush = TRUE;
   }

#ifdef DEBUG
   if (!(flags & SVGA_RELOC_INTERNAL))
      debug_flush_cb_reference(vswc->fctx, vmw_debug_flush_buf(buffer));
#endif
}


/**
 * vmw_swc_surface_clear_reference - Clear referenced info for a surface
 *
 * @swc:   Pointer to an svga_winsys_context
 * @vsurf: Pointer to a vmw_svga_winsys_surface, the referenced info of which
 *         we want to clear
 *
 * This is primarily used by a discard surface map to indicate that the
 * surface data is no longer referenced by a draw call, and mapping it
 * should therefore no longer cause a flush.
 */
void
vmw_swc_surface_clear_reference(struct svga_winsys_context *swc,
                                struct vmw_svga_winsys_surface *vsurf)
{
   struct vmw_svga_winsys_context *vswc = vmw_svga_winsys_context(swc);
   struct vmw_ctx_validate_item *isrf =
      util_hash_table_get(vswc->hash, vsurf);

   if (isrf && isrf->referenced) {
      isrf->referenced = FALSE;
      p_atomic_dec(&vsurf->validated);
   }
}

static void
vmw_swc_surface_only_relocation(struct svga_winsys_context *swc,
				uint32 *where,
				struct vmw_svga_winsys_surface *vsurf,
				unsigned flags)
{
   struct vmw_svga_winsys_context *vswc = vmw_svga_winsys_context(swc);
   struct vmw_ctx_validate_item *isrf;

   assert(vswc->surface.staged < vswc->surface.reserved);
   isrf = util_hash_table_get(vswc->hash, vsurf);

   if (isrf == NULL) {
      isrf = &vswc->surface.items[vswc->surface.used + vswc->surface.staged];
      vmw_svga_winsys_surface_reference(&isrf->vsurf, vsurf);
      isrf->referenced = FALSE;
      /*
       * Note that a failure here may just fall back to unhashed behavior
       * and potentially cause unnecessary flushing, so ignore the
       * return code.
       */
      (void) util_hash_table_set(vswc->hash, vsurf, isrf);
      ++vswc->surface.staged;

      vswc->seen_surfaces += vsurf->size;
      /* divide by 5 not well tuned for performance */
      if (vswc->seen_surfaces >= vswc->vws->ioctl.max_surface_memory / VMW_MAX_SURF_MEM_FACTOR)
         vswc->preemptive_flush = TRUE;
   }

   if (!(flags & SVGA_RELOC_INTERNAL) && !isrf->referenced) {
      isrf->referenced = TRUE;
      p_atomic_inc(&vsurf->validated);
   }

   *where = vsurf->sid;
}

static void
vmw_swc_surface_relocation(struct svga_winsys_context *swc,
                           uint32 *where,
                           uint32 *mobid,
                           struct svga_winsys_surface *surface,
                           unsigned flags)
{
   struct vmw_svga_winsys_surface *vsurf;

   assert(swc->have_gb_objects || mobid == NULL);

   if(!surface) {
      *where = SVGA3D_INVALID_ID;
      if (mobid)
         *mobid = SVGA3D_INVALID_ID;
      return;
   }

   vsurf = vmw_svga_winsys_surface(surface);
   vmw_swc_surface_only_relocation(swc, where, vsurf, flags);

   if (swc->have_gb_objects && vsurf->buf != NULL) {

      /*
       * Make sure backup buffer ends up fenced.
       */

      pipe_mutex_lock(vsurf->mutex);
      assert(vsurf->buf != NULL);
      
      vmw_swc_mob_relocation(swc, mobid, NULL, (struct svga_winsys_buffer *)
                             vsurf->buf, 0, flags);
      pipe_mutex_unlock(vsurf->mutex);
   }
}

static void
vmw_swc_shader_relocation(struct svga_winsys_context *swc,
			  uint32 *shid,
			  uint32 *mobid,
			  uint32 *offset,
			  struct svga_winsys_gb_shader *shader)
{
   struct vmw_svga_winsys_context *vswc = vmw_svga_winsys_context(swc);
   struct vmw_svga_winsys_shader *vshader;
   struct vmw_ctx_validate_item *ishader;
   if(!shader) {
      *shid = SVGA3D_INVALID_ID;
      return;
   }

   assert(vswc->shader.staged < vswc->shader.reserved);
   vshader = vmw_svga_winsys_shader(shader);
   ishader = util_hash_table_get(vswc->hash, vshader);

   if (ishader == NULL) {
      ishader = &vswc->shader.items[vswc->shader.used + vswc->shader.staged];
      vmw_svga_winsys_shader_reference(&ishader->vshader, vshader);
      ishader->referenced = FALSE;
      /*
       * Note that a failure here may just fall back to unhashed behavior
       * and potentially cause unnecessary flushing, so ignore the
       * return code.
       */
      (void) util_hash_table_set(vswc->hash, vshader, ishader);
      ++vswc->shader.staged;
   }

   if (!ishader->referenced) {
      ishader->referenced = TRUE;
      p_atomic_inc(&vshader->validated);
   }

   *shid = vshader->shid;

   if (mobid != NULL && vshader->buf)
      vmw_swc_mob_relocation(swc, mobid, offset, vshader->buf,
			     0, SVGA_RELOC_READ);
}

static void
vmw_swc_commit(struct svga_winsys_context *swc)
{
   struct vmw_svga_winsys_context *vswc = vmw_svga_winsys_context(swc);

   assert(vswc->command.reserved);
   assert(vswc->command.used + vswc->command.reserved <= vswc->command.size);
   vswc->command.used += vswc->command.reserved;
   vswc->command.reserved = 0;

   assert(vswc->surface.staged <= vswc->surface.reserved);
   assert(vswc->surface.used + vswc->surface.staged <= vswc->surface.size);
   vswc->surface.used += vswc->surface.staged;
   vswc->surface.staged = 0;
   vswc->surface.reserved = 0;

   assert(vswc->shader.staged <= vswc->shader.reserved);
   assert(vswc->shader.used + vswc->shader.staged <= vswc->shader.size);
   vswc->shader.used += vswc->shader.staged;
   vswc->shader.staged = 0;
   vswc->shader.reserved = 0;

   assert(vswc->region.staged <= vswc->region.reserved);
   assert(vswc->region.used + vswc->region.staged <= vswc->region.size);
   vswc->region.used += vswc->region.staged;
   vswc->region.staged = 0;
   vswc->region.reserved = 0;
}


static void
vmw_swc_destroy(struct svga_winsys_context *swc)
{
   struct vmw_svga_winsys_context *vswc = vmw_svga_winsys_context(swc);
   unsigned i;

   for(i = 0; i < vswc->surface.used; ++i) {
      struct vmw_ctx_validate_item *isurf = &vswc->surface.items[i];
      if (isurf->referenced)
         p_atomic_dec(&isurf->vsurf->validated);
      vmw_svga_winsys_surface_reference(&isurf->vsurf, NULL);
   }

   for(i = 0; i < vswc->shader.used; ++i) {
      struct vmw_ctx_validate_item *ishader = &vswc->shader.items[i];
      if (ishader->referenced)
         p_atomic_dec(&ishader->vshader->validated);
      vmw_svga_winsys_shader_reference(&ishader->vshader, NULL);
   }

   util_hash_table_destroy(vswc->hash);
   pb_validate_destroy(vswc->validate);
   vmw_ioctl_context_destroy(vswc->vws, swc->cid);
#ifdef DEBUG
   debug_flush_ctx_destroy(vswc->fctx);
#endif
   FREE(vswc);
}

static unsigned vmw_hash_ptr(void *p)
{
   return (unsigned)(unsigned long)p;
}

static int vmw_ptr_compare(void *key1, void *key2)
{
   return (key1 == key2) ? 0 : 1;
}

struct svga_winsys_context *
vmw_svga_winsys_context_create(struct svga_winsys_screen *sws)
{
   struct vmw_winsys_screen *vws = vmw_winsys_screen(sws);
   struct vmw_svga_winsys_context *vswc;

   vswc = CALLOC_STRUCT(vmw_svga_winsys_context);
   if(!vswc)
      return NULL;

   vswc->base.destroy = vmw_swc_destroy;
   vswc->base.reserve = vmw_swc_reserve;
   vswc->base.surface_relocation = vmw_swc_surface_relocation;
   vswc->base.region_relocation = vmw_swc_region_relocation;
   vswc->base.mob_relocation = vmw_swc_mob_relocation;
   vswc->base.context_relocation = vmw_swc_context_relocation;
   vswc->base.shader_relocation = vmw_swc_shader_relocation;
   vswc->base.commit = vmw_swc_commit;
   vswc->base.flush = vmw_swc_flush;
   vswc->base.surface_map = vmw_svga_winsys_surface_map;
   vswc->base.surface_unmap = vmw_svga_winsys_surface_unmap;

   vswc->base.cid = vmw_ioctl_context_create(vws);
   vswc->base.have_gb_objects = sws->have_gb_objects;

   vswc->vws = vws;

   vswc->command.size = VMW_COMMAND_SIZE;
   vswc->surface.size = VMW_SURFACE_RELOCS;
   vswc->shader.size = VMW_SHADER_RELOCS;
   vswc->region.size = VMW_REGION_RELOCS;

   vswc->validate = pb_validate_create();
   if(!vswc->validate)
      goto out_no_validate;

   vswc->hash = util_hash_table_create(vmw_hash_ptr, vmw_ptr_compare);
   if (!vswc->hash)
      goto out_no_hash;

#ifdef DEBUG
   vswc->fctx = debug_flush_ctx_create(TRUE, VMW_DEBUG_FLUSH_STACK);
#endif

   return &vswc->base;

out_no_hash:
   pb_validate_destroy(vswc->validate);
out_no_validate:
   FREE(vswc);
   return NULL;
}
