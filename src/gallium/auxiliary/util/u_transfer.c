#include "pipe/p_context.h"
#include "util/u_rect.h"
#include "util/u_inlines.h"
#include "util/u_transfer.h"
#include "util/u_memory.h"

/* One-shot transfer operation with data supplied in a user
 * pointer.  XXX: strides??
 */
void u_default_transfer_inline_write( struct pipe_context *pipe,
				      struct pipe_resource *resource,
				      struct pipe_subresource sr,
				      unsigned usage,
				      const struct pipe_box *box,
				      const void *data,
				      unsigned stride,
				      unsigned slice_stride)
{
   struct pipe_transfer *transfer = NULL;
   uint8_t *map = NULL;

   transfer = pipe->get_transfer(pipe, 
				 resource,
				 sr,
				 usage,
				 box );
   if (transfer == NULL)
      goto out;

   map = pipe_transfer_map(pipe, transfer);
   if (map == NULL)
      goto out;

   assert(box->depth == 1);	/* XXX: fix me */
   
   util_copy_rect(map,
		  resource->format,
		  transfer->stride, /* bytes */
		  0, 0,
		  box->width,
		  box->height,
		  data,
		  stride,       /* bytes */
		  0, 0);

out:
   if (map)
      pipe_transfer_unmap(pipe, transfer);

   if (transfer)
      pipe_transfer_destroy(pipe, transfer);
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

unsigned u_default_is_resource_referenced( struct pipe_context *pipe,
					   struct pipe_resource *resource,
					unsigned face, unsigned level)
{
   return 0;
}

struct pipe_transfer * u_default_get_transfer(struct pipe_context *context,
					      struct pipe_resource *resource,
					      struct pipe_subresource sr,
					      unsigned usage,
					      const struct pipe_box *box)
{
   struct pipe_transfer *transfer = CALLOC_STRUCT(pipe_transfer);
   if (transfer == NULL)
      return NULL;

   transfer->resource = resource;
   transfer->sr = sr;
   transfer->usage = usage;
   transfer->box = *box;

   /* Note strides are zero, this is ok for buffers, but not for
    * textures 2d & higher at least. 
    */
   return transfer;
}

void u_default_transfer_unmap( struct pipe_context *pipe,
			      struct pipe_transfer *transfer )
{
}

void u_default_transfer_destroy(struct pipe_context *pipe,
				struct pipe_transfer *transfer)
{
   FREE(transfer);
}

