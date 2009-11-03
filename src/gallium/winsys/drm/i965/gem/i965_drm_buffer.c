
#include "i965_drm_winsys.h"
#include "util/u_memory.h"

#include "i915_drm.h"
#include "intel_bufmgr.h"

const char *names[BRW_BUFFER_TYPE_MAX] = {
   "texture",
   "scanout",
   "vertex",
   "curbe",
   "query",
   "shader_constants",
   "wm_scratch",
   "batch",
   "state_cache",
};

static struct brw_winsys_buffer *
i965_libdrm_bo_alloc( struct brw_winsys_screen *sws,
		      enum brw_buffer_type type,
		      unsigned size,
		      unsigned alignment )
{
   struct i965_libdrm_winsys *idws = i965_libdrm_winsys(sws);
   struct i965_libdrm_buffer *buf;

   buf = CALLOC_STRUCT(i965_libdrm_buffer);
   if (!buf)
      return NULL;

   switch (type) {
   case BRW_BUFFER_TYPE_TEXTURE:
      break;
   case BRW_BUFFER_TYPE_VERTEX:
      buf->map_gtt = TRUE;
      break;
   case BRW_BUFFER_TYPE_SCANOUT:
      buf->map_gtt = TRUE;
      break;
   default:
      break;
   }

   buf->bo = drm_intel_bo_alloc(idws->gem, 
				names[type], 
				size, 
				alignment);

   if (!buf->bo)
      goto err;

   buf->base.offset = &buf->bo->offset;
   buf->base.size = size;

   return &buf->base;

err:
   assert(0);
   FREE(buf);
   return NULL;
}




/* Reference and unreference buffers:
 */
static void 
i965_libdrm_bo_reference( struct brw_winsys_buffer *buffer )
{
   struct i965_libdrm_buffer *buf = i965_libdrm_buffer(buffer);

   /* I think we have to refcount ourselves and then just pass through
    * the final dereference to the bo on destruction.
    */
   buf->cheesy_refcount++;
}

static void 
i965_libdrm_bo_unreference( struct brw_winsys_buffer *buffer )
{
   struct i965_libdrm_buffer *buf = i965_libdrm_buffer(buffer);

   if (--buf->cheesy_refcount == 0) {
      drm_intel_bo_unreference(buf->bo);
      FREE(buffer);
   }
}

   /* XXX: parameter names!!
    */
static int 
i965_libdrm_bo_emit_reloc( struct brw_winsys_buffer *buffer,
			   enum brw_buffer_usage usage,
			   unsigned delta,
			   unsigned offset,
			   struct brw_winsys_buffer *buffer2)
{
   struct i965_libdrm_buffer *buf = i965_libdrm_buffer(buffer);
   struct i965_libdrm_buffer *buf2 = i965_libdrm_buffer(buffer2);
   int read, write;
   int ret;

   switch (usage) {
   case BRW_USAGE_STATE:
      read = I915_GEM_DOMAIN_INSTRUCTION;
      write = 0;
      break;
   case BRW_USAGE_QUERY_RESULT:
      read = I915_GEM_DOMAIN_INSTRUCTION;
      write = I915_GEM_DOMAIN_INSTRUCTION;
      break;
   case BRW_USAGE_RENDER_TARGET:
      read = I915_GEM_DOMAIN_RENDER;
      write = 0;
      break;
   case BRW_USAGE_DEPTH_BUFFER:
      read = I915_GEM_DOMAIN_RENDER;
      write = I915_GEM_DOMAIN_RENDER;
      break;
   case BRW_USAGE_SAMPLER:
      read = I915_GEM_DOMAIN_SAMPLER;
      write = 0;
      break;
   case BRW_USAGE_VERTEX:
      read = I915_GEM_DOMAIN_VERTEX;
      write = 0;
      break;
   case BRW_USAGE_SCRATCH:
      read = 0;
      write = 0;
      break;
   default:
      assert(0);
      return -1;
   }

   ret = dri_bo_emit_reloc( buf->bo, read, write, delta, offset, buf2->bo );
   if (ret)
      return -1;

   return 0;
}

static int 
i965_libdrm_bo_exec( struct brw_winsys_buffer *buffer,
		     unsigned bytes_used )
{
   struct i965_libdrm_buffer *buf = i965_libdrm_buffer(buffer);
   int ret;

   ret = dri_bo_exec(buf->bo, bytes_used, NULL, 0, 0);
   if (ret)
      return -1;

   return 0;
}

static int
i965_libdrm_bo_subdata(struct brw_winsys_buffer *buffer,
		       size_t offset,
		       size_t size,
		       const void *data)
{
   struct i965_libdrm_buffer *buf = i965_libdrm_buffer(buffer);
   int ret;

   /* XXX: use bo_map_gtt/memcpy/unmap_gtt under some circumstances???
    */
   ret = drm_intel_bo_subdata(buf->bo, offset, size, (void*)data);
   if (ret)
      return -1;
   
   return 0;
}


static boolean 
i965_libdrm_bo_is_busy(struct brw_winsys_buffer *buffer)
{
   struct i965_libdrm_buffer *buf = i965_libdrm_buffer(buffer);

   return drm_intel_bo_busy(buf->bo);
}

static boolean 
i965_libdrm_bo_references(struct brw_winsys_buffer *a,
			  struct brw_winsys_buffer *b)
{
   struct i965_libdrm_buffer *bufa = i965_libdrm_buffer(a);
   struct i965_libdrm_buffer *bufb = i965_libdrm_buffer(b);

   /* XXX: can't find this func:
    */
   return drm_intel_bo_references(bufa->bo, bufb->bo);
}

/* XXX: couldn't this be handled by returning true/false on
 * bo_emit_reloc?
 */
static boolean 
i965_libdrm_check_aperture_space( struct brw_winsys_screen *iws,
				  struct brw_winsys_buffer **buffers,
				  unsigned count )
{
   static drm_intel_bo *bos[128];
   int i;

   if (count > Elements(bos)) {
      assert(0);
      return FALSE;
   }

   for (i = 0; i < count; i++)
      bos[i] = i965_libdrm_buffer(buffers[i])->bo;

   return dri_bufmgr_check_aperture_space(bos, count);
}

/**
 * Map a buffer.
 */
static void *
i965_libdrm_bo_map(struct brw_winsys_buffer *buffer,
		   boolean write)
{
   struct i965_libdrm_buffer *buf = i965_libdrm_buffer(buffer);
   int ret;

   if (!buf->map_count) {
      if (buf->map_gtt) {
	 ret = drm_intel_gem_bo_map_gtt(buf->bo);
	 if (ret)
	    return NULL;
      }
      else {
	 ret = drm_intel_bo_map(buf->bo, write);
	 if (ret)
	    return NULL;
      }
   }

   buf->map_count++;
   return buf->bo->virtual;
}

/**
 * Unmap a buffer.
 */
static void 
i965_libdrm_bo_unmap(struct brw_winsys_buffer *buffer)
{
   struct i965_libdrm_buffer *buf = i965_libdrm_buffer(buffer);

   if (--buf->map_count > 0)
      return;

   if (buf->map_gtt)
      drm_intel_gem_bo_unmap_gtt(buf->bo);
   else
      drm_intel_bo_unmap(buf->bo);
}


void
i965_libdrm_winsys_init_buffer_functions(struct i965_libdrm_winsys *idws)
{
   idws->base.bo_alloc             = i965_libdrm_bo_alloc;
   idws->base.bo_reference         = i965_libdrm_bo_reference;
   idws->base.bo_unreference       = i965_libdrm_bo_unreference;
   idws->base.bo_emit_reloc        = i965_libdrm_bo_emit_reloc;
   idws->base.bo_exec              = i965_libdrm_bo_exec;
   idws->base.bo_subdata           = i965_libdrm_bo_subdata;
   idws->base.bo_is_busy           = i965_libdrm_bo_is_busy;
   idws->base.bo_references        = i965_libdrm_bo_references;
   idws->base.check_aperture_space = i965_libdrm_check_aperture_space;
   idws->base.bo_map               = i965_libdrm_bo_map;
   idws->base.bo_unmap             = i965_libdrm_bo_unmap;
}
