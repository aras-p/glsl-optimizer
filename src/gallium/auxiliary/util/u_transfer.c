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
