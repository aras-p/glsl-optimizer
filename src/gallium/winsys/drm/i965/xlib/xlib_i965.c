/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Bismarck, ND., USA
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * 
 **************************************************************************/

/*
 * Authors:
 *   Keith Whitwell
 *   Brian Paul
 */


#include "util/u_memory.h"
#include "util/u_math.h"
#include "pipe/p_error.h"
#include "pipe/p_context.h"

#include "xm_winsys.h"

#include "i965/brw_winsys.h"
#include "i965/brw_screen.h"
#include "i965/brw_reg.h"

#define MAX_VRAM (128*1024*1024)

struct xlib_brw_buffer
{
   struct brw_winsys_buffer base;
   unsigned offset;
   unsigned type;
   char *virtual;
   unsigned cheesy_refcount;
   int map_count;
};


/**
 * Subclass of brw_winsys_screen for Xlib winsys
 */
struct xlib_brw_winsys
{
   struct brw_winsys_screen base;
   unsigned offset;
};

static struct xlib_brw_winsys *
xlib_brw_winsys( struct brw_winsys_screen *screen )
{
   return (struct xlib_brw_winsys *)screen;
}


static struct xlib_brw_buffer *
xlib_brw_buffer( struct brw_winsys_buffer *buffer )
{
   return (struct xlib_brw_buffer *)buffer;
}



const char *names[BRW_BUFFER_TYPE_MAX] = {
   "TEXTURE",
   "SCANOUT",
   "VERTEX",
   "CURBE",
   "QUERY",
   "SHADER_CONSTANTS",
   "WM_SCRATCH",
   "BATCH",
   "GENERAL_STATE",
   "SURFACE_STATE",
   "PIXEL",
   "GENERIC",
};

const char *usages[BRW_USAGE_MAX] = {
   "STATE",
   "QUERY_RESULT",
   "RENDER_TARGET",
   "DEPTH_BUFFER",
   "BLIT_SOURCE",
   "BLIT_DEST",
   "SAMPLER",
   "VERTEX",
   "SCRATCH"
};


const char *data_types[BRW_DATA_MAX] =
{
   "GS: CC_VP",
   "GS: CC_UNIT",
   "GS: WM_PROG",
   "GS: SAMPLER_DEFAULT_COLOR",
   "GS: SAMPLER",
   "GS: WM_UNIT",
   "GS: SF_PROG",
   "GS: SF_VP",
   "GS: SF_UNIT",
   "GS: VS_UNIT",
   "GS: VS_PROG",
   "GS: GS_UNIT",
   "GS: GS_PROG",
   "GS: CLIP_VP",
   "GS: CLIP_UNIT",
   "GS: CLIP_PROG",
   "SS: SURFACE",
   "SS: SURF_BIND",
   "(untyped)"
};


static struct brw_winsys_buffer *
xlib_brw_bo_alloc( struct brw_winsys_screen *sws,
		      enum brw_buffer_type type,
		      unsigned size,
		      unsigned alignment )
{
   struct xlib_brw_winsys *xbw = xlib_brw_winsys(sws);
   struct xlib_brw_buffer *buf;

   debug_printf("%s type %s sz %d align %d\n",
                __FUNCTION__, names[type], size, alignment );

   buf = CALLOC_STRUCT(xlib_brw_buffer);
   if (!buf)
      return NULL;

   buf->offset = align(xbw->offset, alignment);
   buf->type = type;
   buf->virtual = MALLOC(size);
   buf->cheesy_refcount = 1;
   buf->base.offset = &buf->offset; /* hmm, cheesy */
   buf->base.size = size;

   xbw->offset = align(xbw->offset, alignment) + size;
   if (xbw->offset > MAX_VRAM)
      goto err;

   return &buf->base;

err:
   assert(0);
   FREE(buf);
   return NULL;
}

static void 
xlib_brw_bo_reference( struct brw_winsys_buffer *buffer )
{
   struct xlib_brw_buffer *buf = xlib_brw_buffer(buffer);

   buf->cheesy_refcount++;
}

static void 
xlib_brw_bo_unreference( struct brw_winsys_buffer *buffer )
{
   struct xlib_brw_buffer *buf = xlib_brw_buffer(buffer);

   /* As a special favor in this call only, buffer is allowed to be
    * NULL:
    */
   if (buffer == NULL)
      return;

   if (--buf->cheesy_refcount == 0) {
      FREE(buffer);
   }
}

static int 
xlib_brw_bo_emit_reloc( struct brw_winsys_buffer *buffer,
                        enum brw_buffer_usage usage,
                        unsigned delta,
                        unsigned offset,
                        struct brw_winsys_buffer *buffer2)
{
   struct xlib_brw_buffer *buf = xlib_brw_buffer(buffer);
   struct xlib_brw_buffer *buf2 = xlib_brw_buffer(buffer2);

   debug_printf("%s buf %p offset %x val %x + %x buf2 %p/%s/%s\n",
                __FUNCTION__, (void *)buffer, offset,
                buf2->offset, delta,
                (void *)buffer2, names[buf2->type], usages[usage]);

   *(uint32_t *)(buf->virtual + offset) = buf2->offset + delta;

   return 0;
}

static int 
xlib_brw_bo_exec( struct brw_winsys_buffer *buffer,
		     unsigned bytes_used )
{
   debug_printf("execute buffer %p, bytes %d\n", (void *)buffer, bytes_used);

   return 0;
}

static int
xlib_brw_bo_subdata(struct brw_winsys_buffer *buffer,
                    enum brw_buffer_data_type data_type,
                    size_t offset,
                    size_t size,
                    const void *data)
{
   struct xlib_brw_buffer *buf = xlib_brw_buffer(buffer);

   debug_printf("%s buf %p off %d sz %d data %p %s\n", 
                __FUNCTION__, 
                (void *)buffer, offset, size, data, data_types[data_type]);

   memcpy(buf->virtual + offset, data, size);
   return 0;
}


static boolean 
xlib_brw_bo_is_busy(struct brw_winsys_buffer *buffer)
{
   debug_printf("%s %p\n", __FUNCTION__, (void *)buffer);
   return TRUE;
}

static boolean 
xlib_brw_bo_references(struct brw_winsys_buffer *a,
			  struct brw_winsys_buffer *b)
{
   debug_printf("%s %p %p\n", __FUNCTION__, (void *)a, (void *)b);
   return TRUE;
}

static enum pipe_error
xlib_brw_check_aperture_space( struct brw_winsys_screen *iws,
                                struct brw_winsys_buffer **buffers,
                                unsigned count )
{
   unsigned tot_size = 0;
   unsigned i;

   for (i = 0; i < count; i++)
      tot_size += buffers[i]->size;

   debug_printf("%s %d bufs, tot_size: %d kb\n", 
                __FUNCTION__, count, 
                (tot_size + 1023) / 1024);

   return PIPE_OK;
}

static void *
xlib_brw_bo_map(struct brw_winsys_buffer *buffer,
                enum brw_buffer_data_type data_type,
		   boolean write)
{
   struct xlib_brw_buffer *buf = xlib_brw_buffer(buffer);

   debug_printf("%s %p %s %s\n", __FUNCTION__, (void *)buffer, 
                write ? "read/write" : "read",
                write ? data_types[data_type] : "");

   buf->map_count++;
   return buf->virtual;
}

static void 
xlib_brw_bo_unmap(struct brw_winsys_buffer *buffer)
{
   struct xlib_brw_buffer *buf = xlib_brw_buffer(buffer);

   debug_printf("%s %p\n", __FUNCTION__, (void *)buffer);

   --buf->map_count;
   assert(buf->map_count >= 0);
}


static void
xlib_brw_winsys_destroy( struct brw_winsys_screen *screen )
{
   /* XXX: free all buffers */
   FREE(screen);
}

static struct brw_winsys_screen *
xlib_create_brw_winsys_screen( void )
{
   struct xlib_brw_winsys *ws;

   ws = CALLOC_STRUCT(xlib_brw_winsys);
   if (!ws)
      return NULL;

   ws->base.destroy              = xlib_brw_winsys_destroy;
   ws->base.bo_alloc             = xlib_brw_bo_alloc;
   ws->base.bo_reference         = xlib_brw_bo_reference;
   ws->base.bo_unreference       = xlib_brw_bo_unreference;
   ws->base.bo_emit_reloc        = xlib_brw_bo_emit_reloc;
   ws->base.bo_exec              = xlib_brw_bo_exec;
   ws->base.bo_subdata           = xlib_brw_bo_subdata;
   ws->base.bo_is_busy           = xlib_brw_bo_is_busy;
   ws->base.bo_references        = xlib_brw_bo_references;
   ws->base.check_aperture_space = xlib_brw_check_aperture_space;
   ws->base.bo_map               = xlib_brw_bo_map;
   ws->base.bo_unmap             = xlib_brw_bo_unmap;

   return &ws->base;
}


/***********************************************************************
 * Implementation of Xlib co-state-tracker's winsys interface
 */

static void
xlib_i965_display_surface(struct xmesa_buffer *xm_buffer,
                          struct pipe_surface *surf)
{
   /* struct brw_texture *texture = brw_texture(surf->texture); */

   debug_printf("%s tex %p, sz %dx%d\n", __FUNCTION__, 
                (void *)surf->texture,
                surf->texture->width[0],
                surf->texture->height[0]);
}

static void
xlib_i965_flush_frontbuffer(struct pipe_screen *screen,
			    struct pipe_surface *surf,
			    void *context_private)
{
   xlib_i965_display_surface(NULL, surf);
}


static struct pipe_screen *
xlib_create_i965_screen( void )
{
   struct brw_winsys_screen *winsys;
   struct pipe_screen *screen;

   winsys = xlib_create_brw_winsys_screen();
   if (winsys == NULL)
      return NULL;

   screen = brw_create_screen(winsys, PCI_CHIP_GM45_GM);
   if (screen == NULL)
      goto fail;

   screen->flush_frontbuffer = xlib_i965_flush_frontbuffer;
   return screen;

fail:
   if (winsys)
      winsys->destroy( winsys );

   return NULL;
}


static struct pipe_context *
xlib_create_i965_context( struct pipe_screen *screen,
                          void *context_private )
{
   struct pipe_context *pipe;
   
   pipe = brw_create_context(screen);
   if (pipe == NULL)
      goto fail;

   pipe->priv = context_private;
   return pipe;

fail:
   /* Free stuff here */
   return NULL;
}




struct xm_driver xlib_i965_driver = 
{
   .create_pipe_screen = xlib_create_i965_screen,
   .create_pipe_context = xlib_create_i965_context,
   .display_surface = xlib_i965_display_surface
};


/* Register this driver at library load: 
 */
static void _init( void ) __attribute__((constructor));
static void _init( void )
{
   xmesa_set_driver( &xlib_i965_driver );
}



/***********************************************************************
 *
 * Butt-ugly hack to convince the linker not to throw away public GL
 * symbols (they are all referenced from getprocaddress, I guess).
 */
extern void (*linker_foo(const unsigned char *procName))();
extern void (*glXGetProcAddress(const unsigned char *procName))();

extern void (*linker_foo(const unsigned char *procName))()
{
   return glXGetProcAddress(procName);
}
