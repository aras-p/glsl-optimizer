#include "pipe/p_context.h"
#include "util/u_surface.h"
#include "util/u_inlines.h"
#include "util/u_transfer.h"
#include "util/u_memory.h"

/* One-shot transfer operation with data supplied in a user
 * pointer.  XXX: strides??
 */
void u_default_transfer_inline_write( struct pipe_context *pipe,
                                      struct pipe_resource *resource,
                                      unsigned level,
                                      unsigned usage,
                                      const struct pipe_box *box,
                                      const void *data,
                                      unsigned stride,
                                      unsigned layer_stride)
{
   struct pipe_transfer *transfer = NULL;
   uint8_t *map = NULL;

   assert(!(usage & PIPE_TRANSFER_READ));

   /* the write flag is implicit by the nature of transfer_inline_write */
   usage |= PIPE_TRANSFER_WRITE;

   /* transfer_inline_write implicitly discards the rewritten buffer range */
   if (box->x == 0 && box->width == resource->width0) {
      usage |= PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE;
   } else {
      usage |= PIPE_TRANSFER_DISCARD_RANGE;
   }

   map = pipe->transfer_map(pipe,
                            resource,
                            level,
                            usage,
                            box, &transfer);
   if (map == NULL)
      return;

   if (resource->target == PIPE_BUFFER) {
      assert(box->height == 1);
      assert(box->depth == 1);

      memcpy(map, data, box->width);
   }
   else {
      const uint8_t *src_data = data;

      util_copy_box(map,
		    resource->format,
		    transfer->stride, /* bytes */
		    transfer->layer_stride, /* bytes */
                    0, 0, 0,
		    box->width,
		    box->height,
		    box->depth,
		    src_data,
		    stride,       /* bytes */
		    layer_stride, /* bytes */
		    0, 0, 0);
   }

   pipe_transfer_unmap(pipe, transfer);
}


boolean u_default_resource_get_handle(struct pipe_screen *screen,
                                      struct pipe_resource *resource,
                                      struct winsys_handle *handle)
{
   return FALSE;
}



void u_default_transfer_flush_region( struct pipe_context *pipe,
                                      struct pipe_transfer *transfer,
                                      const struct pipe_box *box)
{
   /* This is a no-op implementation, nothing to do.
    */
}

void u_default_transfer_unmap( struct pipe_context *pipe,
                               struct pipe_transfer *transfer )
{
}


static INLINE struct u_resource *
u_resource( struct pipe_resource *res )
{
   return (struct u_resource *)res;
}

boolean u_resource_get_handle_vtbl(struct pipe_screen *screen,
                                   struct pipe_resource *resource,
                                   struct winsys_handle *handle)
{
   struct u_resource *ur = u_resource(resource);
   return ur->vtbl->resource_get_handle(screen, resource, handle);
}

void u_resource_destroy_vtbl(struct pipe_screen *screen,
                             struct pipe_resource *resource)
{
   struct u_resource *ur = u_resource(resource);
   ur->vtbl->resource_destroy(screen, resource);
}

void *u_transfer_map_vtbl(struct pipe_context *context,
                          struct pipe_resource *resource,
                          unsigned level,
                          unsigned usage,
                          const struct pipe_box *box,
                          struct pipe_transfer **transfer)
{
   struct u_resource *ur = u_resource(resource);
   return ur->vtbl->transfer_map(context, resource, level, usage, box,
                                 transfer);
}

void u_transfer_flush_region_vtbl( struct pipe_context *pipe,
                                   struct pipe_transfer *transfer,
                                   const struct pipe_box *box)
{
   struct u_resource *ur = u_resource(transfer->resource);
   ur->vtbl->transfer_flush_region(pipe, transfer, box);
}

void u_transfer_unmap_vtbl( struct pipe_context *pipe,
                            struct pipe_transfer *transfer )
{
   struct u_resource *ur = u_resource(transfer->resource);
   ur->vtbl->transfer_unmap(pipe, transfer);
}

void u_transfer_inline_write_vtbl( struct pipe_context *pipe,
                                   struct pipe_resource *resource,
                                   unsigned level,
                                   unsigned usage,
                                   const struct pipe_box *box,
                                   const void *data,
                                   unsigned stride,
                                   unsigned layer_stride)
{
   struct u_resource *ur = u_resource(resource);
   ur->vtbl->transfer_inline_write(pipe,
                                   resource,
                                   level,
                                   usage,
                                   box,
                                   data,
                                   stride,
                                   layer_stride);
}




