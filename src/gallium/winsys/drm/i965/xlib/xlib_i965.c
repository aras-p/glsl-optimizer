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
#include "i965/brw_structs_dump.h"

#define MAX_VRAM (128*1024*1024)

extern int brw_disasm (FILE *file, 
                       const struct brw_instruction *inst,
                       unsigned count );

extern int intel_decode(const uint32_t *data, 
                        int count,
                        uint32_t hw_offset,
                        uint32_t devid);

struct xlib_brw_buffer
{
   struct brw_winsys_buffer base;
   char *virtual;
   unsigned offset;
   unsigned type;
   int map_count;
   boolean modified;
};


/**
 * Subclass of brw_winsys_screen for Xlib winsys
 */
struct xlib_brw_winsys
{
   struct brw_winsys_screen base;
   struct brw_chipset chipset;

   unsigned size;
   unsigned used;
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


static enum pipe_error
xlib_brw_bo_alloc( struct brw_winsys_screen *sws,
                   enum brw_buffer_type type,
                   unsigned size,
                   unsigned alignment,
                   struct brw_winsys_buffer **bo_out )
{
   struct xlib_brw_winsys *xbw = xlib_brw_winsys(sws);
   struct xlib_brw_buffer *buf;

   debug_printf("%s type %s sz %d align %d\n",
                __FUNCTION__, names[type], size, alignment );

   buf = CALLOC_STRUCT(xlib_brw_buffer);
   if (!buf)
      return PIPE_ERROR_OUT_OF_MEMORY;

   pipe_reference_init(&buf->base.reference, 1);

   buf->offset = align(xbw->used, alignment);
   buf->type = type;
   buf->virtual = MALLOC(size);
   buf->base.size = size;
   buf->base.sws = sws;

   xbw->used = align(xbw->used, alignment) + size;
   if (xbw->used > MAX_VRAM)
      goto err;

   /* XXX: possibly rentrant call to bo_destroy:
    */
   bo_reference(bo_out, &buf->base);
   return PIPE_OK;

err:
   assert(0);
   FREE(buf->virtual);
   FREE(buf);
   return PIPE_ERROR_OUT_OF_MEMORY;
}

static void 
xlib_brw_bo_destroy( struct brw_winsys_buffer *buffer )
{
   struct xlib_brw_buffer *buf = xlib_brw_buffer(buffer);

   FREE(buf);
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

static void dump_data( struct xlib_brw_winsys *xbw,
                       enum brw_buffer_data_type data_type,
                       const void *data,
                       size_t size )
{
   switch (data_type) {
   case BRW_DATA_GS_CC_VP:
      brw_dump_cc_viewport( data );
      break;
   case BRW_DATA_GS_CC_UNIT:
      brw_dump_cc_unit_state( data );
      break;
   case BRW_DATA_GS_WM_PROG:
   case BRW_DATA_GS_SF_PROG:
   case BRW_DATA_GS_VS_PROG:
   case BRW_DATA_GS_GS_PROG:
   case BRW_DATA_GS_CLIP_PROG:
      brw_disasm( stderr, data, size / sizeof(struct brw_instruction) );
      break;
   case BRW_DATA_GS_SAMPLER_DEFAULT_COLOR:
      brw_dump_sampler_default_color( data );
      break;
   case BRW_DATA_GS_SAMPLER:
      brw_dump_sampler_state( data );
      break;
   case BRW_DATA_GS_WM_UNIT:
      brw_dump_wm_unit_state( data );
      break;
   case BRW_DATA_GS_SF_VP:
      brw_dump_sf_viewport( data );
      break;
   case BRW_DATA_GS_SF_UNIT:
      brw_dump_sf_unit_state( data );
      break;
   case BRW_DATA_GS_VS_UNIT:
      brw_dump_vs_unit_state( data );
      break;
   case BRW_DATA_GS_GS_UNIT:
      brw_dump_gs_unit_state( data );
      break;
   case BRW_DATA_GS_CLIP_VP:
      brw_dump_clipper_viewport( data );
      break;
   case BRW_DATA_GS_CLIP_UNIT:
      brw_dump_clip_unit_state( data );
      break;
   case BRW_DATA_SS_SURFACE:
      brw_dump_surface_state( data );
      break;
   case BRW_DATA_SS_SURF_BIND:
      break;
   case BRW_DATA_OTHER:
      break;
   case BRW_DATA_BATCH_BUFFER:
      intel_decode(data, size / 4, 0, xbw->chipset.pci_id);
      break;
   case BRW_DATA_CONSTANT_BUFFER:
      break;
   default:
      assert(0);
      break;
   }
}


static int
xlib_brw_bo_subdata(struct brw_winsys_buffer *buffer,
                    enum brw_buffer_data_type data_type,
                    size_t offset,
                    size_t size,
                    const void *data)
{
   struct xlib_brw_buffer *buf = xlib_brw_buffer(buffer);
   struct xlib_brw_winsys *xbw = xlib_brw_winsys(buffer->sws);

   debug_printf("%s buf %p off %d sz %d %s\n", 
                __FUNCTION__, 
                (void *)buffer, offset, size, data_types[data_type]);

   if (1)
      dump_data( xbw, data_type, data, size );

   assert(buf->base.size >= offset + size);
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

   if (write)
      buf->modified = 1;

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

   if (buf->map_count == 0 &&
       buf->modified) {

      buf->modified = 0;
      
      /* Consider dumping new buffer contents here.
       */
   }
}


static void
xlib_brw_bo_wait_idle( struct brw_winsys_buffer *buffer )
{
}


static void
xlib_brw_winsys_destroy( struct brw_winsys_screen *sws )
{
   struct xlib_brw_winsys *xbw = xlib_brw_winsys(sws);

   FREE(xbw);
}

static struct brw_winsys_screen *
xlib_create_brw_winsys_screen( void )
{
   struct xlib_brw_winsys *ws;

   ws = CALLOC_STRUCT(xlib_brw_winsys);
   if (!ws)
      return NULL;

   ws->used = 0;

   ws->base.destroy              = xlib_brw_winsys_destroy;
   ws->base.bo_alloc             = xlib_brw_bo_alloc;
   ws->base.bo_destroy           = xlib_brw_bo_destroy;
   ws->base.bo_emit_reloc        = xlib_brw_bo_emit_reloc;
   ws->base.bo_exec              = xlib_brw_bo_exec;
   ws->base.bo_subdata           = xlib_brw_bo_subdata;
   ws->base.bo_is_busy           = xlib_brw_bo_is_busy;
   ws->base.bo_references        = xlib_brw_bo_references;
   ws->base.check_aperture_space = xlib_brw_check_aperture_space;
   ws->base.bo_map               = xlib_brw_bo_map;
   ws->base.bo_unmap             = xlib_brw_bo_unmap;
   ws->base.bo_wait_idle         = xlib_brw_bo_wait_idle;

   return &ws->base;
}


/***********************************************************************
 * Implementation of Xlib co-state-tracker's winsys interface
 */

static void
xlib_i965_display_surface(struct xmesa_buffer *xm_buffer,
                          struct pipe_surface *surf)
{
   struct brw_surface *surface = brw_surface(surf);
   struct xlib_brw_buffer *bo = xlib_brw_buffer(surface->bo);

   debug_printf("%s offset %x+%x sz %dx%d\n", __FUNCTION__, 
                bo->offset,
                surface->draw_offset,
                surf->width,
                surf->height);
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

   xlib_brw_winsys(winsys)->chipset = brw_screen(screen)->chipset;

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
