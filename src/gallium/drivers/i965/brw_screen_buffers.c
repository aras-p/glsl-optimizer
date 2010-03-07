
#include "util/u_memory.h"
#include "util/u_math.h"

#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"

#include "brw_screen.h"
#include "brw_winsys.h"



static void *
brw_buffer_map_range( struct pipe_screen *screen,
                      struct pipe_buffer *buffer,
                      unsigned offset,
                      unsigned length,
                      unsigned usage )
{
   struct brw_screen *bscreen = brw_screen(screen); 
   struct brw_winsys_screen *sws = bscreen->sws;
   struct brw_buffer *buf = brw_buffer( buffer );

   if (buf->user_buffer)
      return buf->user_buffer;

   return sws->bo_map( buf->bo, 
                       BRW_DATA_OTHER,
                       offset,
                       length,
                       (usage & PIPE_BUFFER_USAGE_CPU_WRITE) ? TRUE : FALSE,
                       (usage & PIPE_BUFFER_USAGE_DISCARD) ? TRUE : FALSE,
                       (usage & PIPE_BUFFER_USAGE_FLUSH_EXPLICIT) ? TRUE : FALSE);
}

static void *
brw_buffer_map( struct pipe_screen *screen,
                struct pipe_buffer *buffer,
                unsigned usage )
{
   struct brw_screen *bscreen = brw_screen(screen); 
   struct brw_winsys_screen *sws = bscreen->sws;
   struct brw_buffer *buf = brw_buffer( buffer );

   if (buf->user_buffer)
      return buf->user_buffer;

   return sws->bo_map( buf->bo, 
                       BRW_DATA_OTHER,
                       0,
                       buf->base.size,
                       (usage & PIPE_BUFFER_USAGE_CPU_WRITE) ? TRUE : FALSE,
                       FALSE,
                       FALSE);
}


static void 
brw_buffer_flush_mapped_range( struct pipe_screen *screen,
                               struct pipe_buffer *buffer,
                               unsigned offset,
                               unsigned length )
{
   struct brw_screen *bscreen = brw_screen(screen); 
   struct brw_winsys_screen *sws = bscreen->sws;
   struct brw_buffer *buf = brw_buffer( buffer );

   if (buf->user_buffer)
      return;

   sws->bo_flush_range( buf->bo, 
                        offset,
                        length );
}


static void 
brw_buffer_unmap( struct pipe_screen *screen,
                   struct pipe_buffer *buffer )
{
   struct brw_screen *bscreen = brw_screen(screen); 
   struct brw_winsys_screen *sws = bscreen->sws;
   struct brw_buffer *buf = brw_buffer( buffer );
   
   if (buf->bo)
      sws->bo_unmap(buf->bo);
}

static void
brw_buffer_destroy( struct pipe_buffer *buffer )
{
   struct brw_buffer *buf = brw_buffer( buffer );

   assert(!p_atomic_read(&buffer->reference.count));

   bo_reference(&buf->bo, NULL);
   FREE(buf);
}


static struct pipe_buffer *
brw_buffer_create(struct pipe_screen *screen,
                   unsigned alignment,
                   unsigned usage,
                   unsigned size)
{
   struct brw_screen *bscreen = brw_screen(screen);
   struct brw_winsys_screen *sws = bscreen->sws;
   struct brw_buffer *buf;
   unsigned buffer_type;
   enum pipe_error ret;
   
   buf = CALLOC_STRUCT(brw_buffer);
   if (!buf)
      return NULL;
      
   pipe_reference_init(&buf->base.reference, 1);
   buf->base.screen = screen;
   buf->base.alignment = alignment;
   buf->base.usage = usage;
   buf->base.size = size;

   switch (usage & (PIPE_BUFFER_USAGE_VERTEX |
                    PIPE_BUFFER_USAGE_INDEX |
                    PIPE_BUFFER_USAGE_PIXEL |
                    PIPE_BUFFER_USAGE_CONSTANT))
   {
   case PIPE_BUFFER_USAGE_VERTEX:
   case PIPE_BUFFER_USAGE_INDEX:
   case (PIPE_BUFFER_USAGE_VERTEX|PIPE_BUFFER_USAGE_INDEX):
      buffer_type = BRW_BUFFER_TYPE_VERTEX;
      break;
      
   case PIPE_BUFFER_USAGE_PIXEL:
      buffer_type = BRW_BUFFER_TYPE_PIXEL;
      break;

   case PIPE_BUFFER_USAGE_CONSTANT:
      buffer_type = BRW_BUFFER_TYPE_SHADER_CONSTANTS;
      break;

   default:
      buffer_type = BRW_BUFFER_TYPE_GENERIC;
      break;
   }
   
   ret = sws->bo_alloc( sws, buffer_type,
                        size, alignment,
                        &buf->bo );
   if (ret != PIPE_OK)
      return NULL;
      
   return &buf->base; 
}


static struct pipe_buffer *
brw_user_buffer_create(struct pipe_screen *screen,
                       void *ptr,
                       unsigned bytes)
{
   struct brw_buffer *buf;
   
   buf = CALLOC_STRUCT(brw_buffer);
   if (!buf)
      return NULL;
      
   buf->user_buffer = ptr;
   
   pipe_reference_init(&buf->base.reference, 1);
   buf->base.screen = screen;
   buf->base.alignment = 1;
   buf->base.usage = 0;
   buf->base.size = bytes;
   
   return &buf->base; 
}


boolean brw_is_buffer_referenced_by_bo( struct brw_screen *brw_screen,
                                     struct pipe_buffer *buffer,
                                     struct brw_winsys_buffer *bo )
{
   struct brw_buffer *buf = brw_buffer(buffer);
   if (buf->bo == NULL)
      return FALSE;

   return brw_screen->sws->bo_references( bo, buf->bo );
}

   
void brw_screen_buffer_init(struct brw_screen *brw_screen)
{
   brw_screen->base.buffer_create = brw_buffer_create;
   brw_screen->base.user_buffer_create = brw_user_buffer_create;
   brw_screen->base.buffer_map = brw_buffer_map;
   brw_screen->base.buffer_map_range = brw_buffer_map_range;
   brw_screen->base.buffer_flush_mapped_range = brw_buffer_flush_mapped_range;
   brw_screen->base.buffer_unmap = brw_buffer_unmap;
   brw_screen->base.buffer_destroy = brw_buffer_destroy;
}
