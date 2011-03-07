
#include "util/u_memory.h"
#include "util/u_math.h"

#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"

#include "brw_resource.h"
#include "brw_context.h"
#include "brw_batchbuffer.h"
#include "brw_winsys.h"

static boolean
brw_buffer_get_handle(struct pipe_screen *screen,
		      struct pipe_resource *resource,
		      struct winsys_handle *handle)
{
   return FALSE;
}


static void
brw_buffer_destroy(struct pipe_screen *screen,
		    struct pipe_resource *resource)
{
   struct brw_buffer *buf = brw_buffer( resource );

   bo_reference(&buf->bo, NULL);
   FREE(buf);
}


static void *
brw_buffer_transfer_map( struct pipe_context *pipe,
			 struct pipe_transfer *transfer)
{
   struct brw_screen *bscreen = brw_screen(pipe->screen); 
   struct brw_winsys_screen *sws = bscreen->sws;
   struct brw_buffer *buf = brw_buffer(transfer->resource);
   unsigned offset = transfer->box.x;
   unsigned length = transfer->box.width;
   unsigned usage = transfer->usage;
   uint8_t *map;

   if (buf->user_buffer)
      map = buf->user_buffer;
   else
      map = sws->bo_map( buf->bo, 
			 BRW_DATA_OTHER,
			 offset,
			 length,
			 (usage & PIPE_TRANSFER_WRITE) ? TRUE : FALSE,
			 (usage & PIPE_TRANSFER_DISCARD) ? TRUE : FALSE,
			 (usage & PIPE_TRANSFER_FLUSH_EXPLICIT) ? TRUE : FALSE);

   return map + offset;
}


static void
brw_buffer_transfer_flush_region( struct pipe_context *pipe,
				  struct pipe_transfer *transfer,
				  const struct pipe_box *box)
{
   struct brw_screen *bscreen = brw_screen(pipe->screen); 
   struct brw_winsys_screen *sws = bscreen->sws;
   struct brw_buffer *buf = brw_buffer(transfer->resource);
   unsigned offset = box->x;
   unsigned length = box->width;

   if (buf->user_buffer)
      return;

   sws->bo_flush_range( buf->bo, 
                        offset,
                        length );
}


static void
brw_buffer_transfer_unmap( struct pipe_context *pipe,
			   struct pipe_transfer *transfer)
{
   struct brw_screen *bscreen = brw_screen(pipe->screen); 
   struct brw_winsys_screen *sws = bscreen->sws;
   struct brw_buffer *buf = brw_buffer( transfer->resource );
   
   if (buf->bo)
      sws->bo_unmap(buf->bo);
}


struct u_resource_vtbl brw_buffer_vtbl = 
{
   brw_buffer_get_handle,	     /* get_handle */
   brw_buffer_destroy,		     /* resource_destroy */
   u_default_get_transfer,	     /* get_transfer */
   u_default_transfer_destroy,	     /* transfer_destroy */
   brw_buffer_transfer_map,	     /* transfer_map */
   brw_buffer_transfer_flush_region,  /* transfer_flush_region */
   brw_buffer_transfer_unmap,	     /* transfer_unmap */
   u_default_transfer_inline_write   /* transfer_inline_write */
};


struct pipe_resource *
brw_buffer_create(struct pipe_screen *screen,
		  const struct pipe_resource *template)
{
   struct brw_screen *bscreen = brw_screen(screen);
   struct brw_winsys_screen *sws = bscreen->sws;
   struct brw_buffer *buf;
   unsigned buffer_type;
   enum pipe_error ret;
   
   buf = CALLOC_STRUCT(brw_buffer);
   if (!buf)
      return NULL;
      
   buf->b.b = *template;
   buf->b.vtbl = &brw_buffer_vtbl;
   pipe_reference_init(&buf->b.b.reference, 1);
   buf->b.b.screen = screen;

   switch (template->bind & (PIPE_BIND_VERTEX_BUFFER |
			      PIPE_BIND_INDEX_BUFFER |
			      PIPE_BIND_CONSTANT_BUFFER))
   {
   case PIPE_BIND_VERTEX_BUFFER:
   case PIPE_BIND_INDEX_BUFFER:
   case (PIPE_BIND_VERTEX_BUFFER|PIPE_BIND_INDEX_BUFFER):
      buffer_type = BRW_BUFFER_TYPE_VERTEX;
      break;
      
   case PIPE_BIND_CONSTANT_BUFFER:
      buffer_type = BRW_BUFFER_TYPE_SHADER_CONSTANTS;
      break;

   default:
      buffer_type = BRW_BUFFER_TYPE_GENERIC;
      break;
   }
   
   ret = sws->bo_alloc( sws, buffer_type,
                        template->width0,
			64,	/* alignment */
                        &buf->bo );
   if (ret != PIPE_OK)
      return NULL;
      
   return &buf->b.b; 
}


struct pipe_resource *
brw_user_buffer_create(struct pipe_screen *screen,
                       void *ptr,
                       unsigned bytes,
		       unsigned bind)
{
   struct brw_buffer *buf;
   
   buf = CALLOC_STRUCT(brw_buffer);
   if (!buf)
      return NULL;
   
   pipe_reference_init(&buf->b.b.reference, 1);
   buf->b.vtbl = &brw_buffer_vtbl;
   buf->b.b.screen = screen;
   buf->b.b.format = PIPE_FORMAT_R8_UNORM; /* ?? */
   buf->b.b.usage = PIPE_USAGE_IMMUTABLE;
   buf->b.b.bind = bind;
   buf->b.b.width0 = bytes;
   buf->b.b.height0 = 1;
   buf->b.b.depth0 = 1;
   buf->b.b.array_size = 1;

   buf->user_buffer = ptr;
   
   return &buf->b.b; 
}
