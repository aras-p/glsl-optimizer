
#include "i915_sw_winsys.h"
#include "util/u_memory.h"

static struct i915_winsys_buffer *
i915_sw_buffer_create(struct i915_winsys *iws,
                      unsigned size, unsigned alignment,
                      enum i915_winsys_buffer_type type)
{
   struct i915_sw_buffer *buf = CALLOC_STRUCT(i915_sw_buffer);
   char *name;

   if (!buf)
      return NULL;

   if (type == I915_NEW_TEXTURE) {
      name = "gallium3d_texture";
   } else if (type == I915_NEW_VERTEX) {
      name = "gallium3d_vertex";
   } else if (type == I915_NEW_SCANOUT) {
      name = "gallium3d_scanout";
   } else {
      assert(0);
      name = "gallium3d_unknown";
   }

   buf->magic = 0xDEAD1337;
   buf->name = name;
   buf->type = type;
   buf->ptr = CALLOC(size, 1);

   if (!buf->ptr)
      goto err;

   return (struct i915_winsys_buffer *)buf;

err:
   assert(0);
   FREE(buf);
   return NULL;
}

static int
i915_sw_buffer_set_fence_reg(struct i915_winsys *iws,
                               struct i915_winsys_buffer *buffer,
                               unsigned stride,
                               enum i915_winsys_buffer_tile tile)
{
   struct i915_sw_buffer *buf = i915_sw_buffer(buffer);

   if (tile != I915_TILE_NONE) {
      assert(buf->map_count == 0);
   }

   buf->tile = tile;

   return 0;
}

static void *
i915_sw_buffer_map(struct i915_winsys *iws,
                   struct i915_winsys_buffer *buffer,
                   boolean write)
{
   struct i915_sw_buffer *buf = i915_sw_buffer(buffer);

   buf->map_count += 1;
   return buf->ptr;
}

static void
i915_sw_buffer_unmap(struct i915_winsys *iws,
                     struct i915_winsys_buffer *buffer)
{
   struct i915_sw_buffer *buf = i915_sw_buffer(buffer);

   buf->map_count -= 1;
}

static int
i915_sw_buffer_write(struct i915_winsys *iws,
                     struct i915_winsys_buffer *buffer,
                     size_t offset,
                     size_t size,
                     const void *data)
{
   struct i915_sw_buffer *buf = i915_sw_buffer(buffer);

   memcpy(buf->ptr + offset, data, size);
   return 0;
}

static void
i915_sw_buffer_destroy(struct i915_winsys *iws,
                       struct i915_winsys_buffer *buffer)
{
   struct i915_sw_buffer *buf = i915_sw_buffer(buffer);

#ifdef DEBUG
   buf->magic = 0;
#endif

   FREE(buf->ptr);
   FREE(buf);
}

void
i915_sw_winsys_init_buffer_functions(struct i915_sw_winsys *isws)
{
   isws->base.buffer_create = i915_sw_buffer_create;
   isws->base.buffer_set_fence_reg = i915_sw_buffer_set_fence_reg;
   isws->base.buffer_map = i915_sw_buffer_map;
   isws->base.buffer_unmap = i915_sw_buffer_unmap;
   isws->base.buffer_write = i915_sw_buffer_write;
   isws->base.buffer_destroy = i915_sw_buffer_destroy;
}
