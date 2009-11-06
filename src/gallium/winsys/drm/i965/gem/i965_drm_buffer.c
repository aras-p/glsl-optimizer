
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

static enum pipe_error 
i965_libdrm_bo_alloc(struct brw_winsys_screen *sws,
                     enum brw_buffer_type type,
                     unsigned size,
                     unsigned alignment,
                     struct brw_winsys_buffer **bo_out)
{
   struct i965_libdrm_winsys *idws = i965_libdrm_winsys(sws);
   struct i965_libdrm_buffer *buf;

   buf = CALLOC_STRUCT(i965_libdrm_buffer);
   if (!buf)
      return PIPE_ERROR_OUT_OF_MEMORY;

   switch (type) {
   case BRW_BUFFER_TYPE_TEXTURE:
/* case BRW_BUFFER_TYPE_SCANOUT:*/
   case BRW_BUFFER_TYPE_VERTEX:
   case BRW_BUFFER_TYPE_CURBE:
   case BRW_BUFFER_TYPE_QUERY:
   case BRW_BUFFER_TYPE_SHADER_CONSTANTS:
   case BRW_BUFFER_TYPE_SHADER_SCRATCH:
   case BRW_BUFFER_TYPE_BATCH:
   case BRW_BUFFER_TYPE_GENERAL_STATE:
   case BRW_BUFFER_TYPE_SURFACE_STATE:
   case BRW_BUFFER_TYPE_PIXEL:
   case BRW_BUFFER_TYPE_GENERIC:
      break;
   case BRW_BUFFER_TYPE_SCANOUT:
      buf->map_gtt = TRUE;
      break;
   default:
      assert(0);
      break;
   }

   buf->bo = drm_intel_bo_alloc(idws->gem, 
                                names[type], 
                                size, 
                                alignment);

   if (!buf->bo)
      goto err;

   pipe_reference_init(&buf->base.reference, 1);
   buf->base.size = size;
   buf->base.sws = sws;

   *bo_out = &buf->base;
   return PIPE_OK;

err:
   assert(0);
   FREE(buf);
   return PIPE_ERROR_OUT_OF_MEMORY;
}

static void 
i965_libdrm_bo_destroy(struct brw_winsys_buffer *buffer)
{
   struct i965_libdrm_buffer *buf = i965_libdrm_buffer(buffer);

   drm_intel_bo_unreference(buf->bo);
   FREE(buffer);
}

static enum pipe_error
i965_libdrm_bo_emit_reloc(struct brw_winsys_buffer *buffer,
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
   case BRW_USAGE_BLIT_DEST:
      read = I915_GEM_DOMAIN_RENDER;
      write = I915_GEM_DOMAIN_RENDER;
      break;
   case BRW_USAGE_BLIT_SOURCE:
      read = 0;
      write = I915_GEM_DOMAIN_RENDER;
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

static enum pipe_error 
i965_libdrm_bo_exec(struct brw_winsys_buffer *buffer,
                    unsigned bytes_used)
{
   struct i965_libdrm_buffer *buf = i965_libdrm_buffer(buffer);
   struct i965_libdrm_winsys *idws = i965_libdrm_winsys(buffer->sws);
   int ret;

   if (idws->send_cmd) {
      ret = dri_bo_exec(buf->bo, bytes_used, NULL, 0, 0);
      if (ret)
         return PIPE_ERROR;
   }

   return PIPE_OK;
}

static enum pipe_error
i965_libdrm_bo_subdata(struct brw_winsys_buffer *buffer,
                       enum brw_buffer_data_type data_type,
                       size_t offset,
                       size_t size,
                       const void *data,
                       const struct brw_winsys_reloc *reloc,
                       unsigned nr_reloc)
{
   struct i965_libdrm_buffer *buf = i965_libdrm_buffer(buffer);
   int ret, i;

   (void)data_type;

   /* XXX: use bo_map_gtt/memcpy/unmap_gtt under some circumstances???
    */
   ret = drm_intel_bo_subdata(buf->bo, offset, size, (void*)data);
   if (ret)
      return PIPE_ERROR;
  
   for (i = 0; i < nr_reloc; i++) {
      i965_libdrm_bo_emit_reloc(buffer, reloc[i].usage, reloc[i].delta,
                                reloc[i].offset, reloc[i].bo);
   }

   return PIPE_OK;
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
static enum pipe_error
i965_libdrm_check_aperture_space(struct brw_winsys_screen *iws,
                                 struct brw_winsys_buffer **buffers,
                                 unsigned count)
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

static void *
i965_libdrm_bo_map(struct brw_winsys_buffer *buffer,
                   enum brw_buffer_data_type data_type,
                   unsigned offset,
                   unsigned length,
                   boolean write,
                   boolean discard,
                   boolean flush_explicit)
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

static void
i965_libdrm_bo_flush_range(struct brw_winsys_buffer *buffer,
                           unsigned offset,
                           unsigned length)
{

}

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
   idws->base.bo_destroy           = i965_libdrm_bo_destroy;
   idws->base.bo_emit_reloc        = i965_libdrm_bo_emit_reloc;
   idws->base.bo_exec              = i965_libdrm_bo_exec;
   idws->base.bo_subdata           = i965_libdrm_bo_subdata;
   idws->base.bo_is_busy           = i965_libdrm_bo_is_busy;
   idws->base.bo_references        = i965_libdrm_bo_references;
   idws->base.check_aperture_space = i965_libdrm_check_aperture_space;
   idws->base.bo_map               = i965_libdrm_bo_map;
   idws->base.bo_flush_range       = i965_libdrm_bo_flush_range;
   idws->base.bo_unmap             = i965_libdrm_bo_unmap;
}
