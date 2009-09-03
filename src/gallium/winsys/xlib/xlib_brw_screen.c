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


//#include "state_trackers/xlib/glxheader.h"
//#include "state_trackers/xlib/xmesaP.h"

#include "pipe/internal/p_winsys_screen.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "i965simple/brw_winsys.h"
#include "i965simple/brw_screen.h"
#include "i965simple/brw_context.h"


#include "xlib_brw_aub.h"
#include "xlib_brw.h"
#include "xlib.h"

static struct pipe_buffer *
buffer_from_surface(struct pipe_surface *surface)
{
   struct brw_texture *texture = (struct brw_texture *)surface;
   return texture->buffer;
}

struct aub_buffer {
   struct pipe_reference reference;
   char *data;
   unsigned offset;
   unsigned size;
   unsigned map_count;
   boolean dump_on_unmap;
};



struct aub_pipe_winsys {
   struct pipe_winsys winsys;

   struct brw_aubfile *aubfile;

   /* This is simple, isn't it:
    */
   char *pool;
   unsigned size;
   unsigned used;
};


/* Turn a pipe winsys into an aub/pipe winsys:
 */
static inline struct aub_pipe_winsys *
aub_pipe_winsys( struct pipe_winsys *winsys )
{
   return (struct aub_pipe_winsys *)winsys;
}



static INLINE struct aub_buffer *
aub_bo( struct pipe_buffer *bo )
{
   return (struct aub_buffer *)bo;
}

static INLINE struct pipe_buffer *
pipe_bo( struct aub_buffer *bo )
{
   return (struct pipe_buffer *)bo;
}




static void *aub_buffer_map(struct pipe_winsys *winsys, 
			      struct pipe_buffer *buf,
			      unsigned flags )
{
   struct aub_buffer *sbo = aub_bo(buf);

   assert(sbo->data);

   if (flags & PIPE_BUFFER_USAGE_CPU_WRITE)
      sbo->dump_on_unmap = 1;

   sbo->map_count++;
   return sbo->data;
}

static void aub_buffer_unmap(struct pipe_winsys *winsys, 
			       struct pipe_buffer *buf)
{
   struct aub_pipe_winsys *iws = aub_pipe_winsys(winsys);
   struct aub_buffer *sbo = aub_bo(buf);

   sbo->map_count--;

   if (sbo->map_count == 0 &&
       sbo->dump_on_unmap) {

      sbo->dump_on_unmap = 0;

      brw_aub_gtt_data( iws->aubfile, 
			sbo->offset,
			sbo->data,
			sbo->size,
			0,
			0);
   }
}


static void
aub_buffer_destroy(struct pipe_buffer *buf)
{
   free(buf);
}



void xlib_brw_commands_aub(struct pipe_winsys *winsys,
			unsigned *cmds,
			unsigned nr_dwords)
{
   struct aub_pipe_winsys *iws = aub_pipe_winsys(winsys);
   unsigned size = nr_dwords * 4;

   assert(iws->used + size < iws->size);

   brw_aub_gtt_cmds( iws->aubfile, 
		     AUB_BUF_START + iws->used,
		     cmds,
		     nr_dwords * sizeof(int) );

   iws->used += align(size, 4096);
}


/* XXX: fix me:
 */
static struct aub_pipe_winsys *global_winsys = NULL;




/* Pipe has no concept of pools.  We choose the tex/region pool
 * for all buffers.
 */
static struct pipe_buffer *
aub_buffer_create(struct pipe_winsys *winsys,
                  unsigned alignment,
                  unsigned usage,
                  unsigned size)
{
   struct aub_pipe_winsys *iws = aub_pipe_winsys(winsys);
   struct aub_buffer *sbo = CALLOC_STRUCT(aub_buffer);

   pipe_reference_init(&sbo->reference, 1);

   /* Could reuse buffers that are not referenced in current
    * batchbuffer.  Can't do that atm, so always reallocate:
    */
   assert(iws->used + size < iws->size);
   sbo->data = iws->pool + iws->used;
   sbo->offset = AUB_BUF_START + iws->used;
   iws->used += align(size, 4096);

   sbo->size = size;

   return pipe_bo(sbo);
}


static struct pipe_buffer *
aub_user_buffer_create(struct pipe_winsys *winsys, void *ptr, unsigned bytes)
{
   struct aub_buffer *sbo;

   /* Lets hope this is meant for upload, not as a result!  
    */
   sbo = aub_bo(aub_buffer_create( winsys, 0, 0, 0 ));

   sbo->data = ptr;
   sbo->size = bytes;

   return pipe_bo(sbo);
}


/* The state tracker (should!) keep track of whether the fake
 * frontbuffer has been touched by any rendering since the last time
 * we copied its contents to the real frontbuffer.  Our task is easy:
 */
static void
aub_flush_frontbuffer( struct pipe_winsys *winsys,
                       struct pipe_surface *surface,
                       void *context_private)
{
//   struct aub_pipe_winsys *iws = aub_pipe_winsys(winsys);
   brw_aub_dump_bmp( global_winsys->aubfile, 
		     surface,
		     aub_bo(buffer_from_surface(surface))->offset );
}


/**
 * Round n up to next multiple.
 */
static INLINE unsigned
round_up(unsigned n, unsigned multiple)
{
   return (n + multiple - 1) & ~(multiple - 1);
}

static struct pipe_buffer *
aub_i915_surface_buffer_create(struct pipe_winsys *winsys,
                               unsigned width, unsigned height,
                               enum pipe_format format,
                               unsigned usage,
                               unsigned *stride)
{
   const unsigned alignment = 64;
   struct pipe_format_block block;
   unsigned nblocksx, nblocksy;

   pf_get_block(format, &block);
   nblocksx = pf_get_nblocksx(&block, width);
   nblocksy = pf_get_nblocksy(&block, height);
   *stride = round_up(nblocksx * block.size, alignment);

   return winsys->buffer_create(winsys, alignment,
                                usage,
                                *stride * nblocksy);
}


static const char *
aub_get_name( struct pipe_winsys *winsys )
{
   return "Aub/xlib";
}

static void
xlib_brw_destroy_pipe_winsys_aub( struct pipe_winsys *winsys )

{
   struct aub_pipe_winsys *iws = aub_pipe_winsys(winsys);
   brw_aub_destroy(iws->aubfile);
   free(iws->pool);
   free(iws);
}



static struct pipe_winsys *
xlib_create_brw_winsys( void )
{
   struct aub_pipe_winsys *iws = CALLOC_STRUCT( aub_pipe_winsys );
   
   /* Fill in this struct with callbacks that pipe will need to
    * communicate with the window system, buffer manager, etc. 
    *
    * Pipe would be happy with a malloc based memory manager, but
    * the SwapBuffers implementation in this winsys driver requires
    * that rendering be done to an appropriate _DriBufferObject.  
    */
   iws->winsys.buffer_create = aub_buffer_create;
   iws->winsys.user_buffer_create = aub_user_buffer_create;
   iws->winsys.buffer_map = aub_buffer_map;
   iws->winsys.buffer_unmap = aub_buffer_unmap;
   iws->winsys.buffer_destroy = aub_buffer_destroy;
   iws->winsys.flush_frontbuffer = aub_flush_frontbuffer;
   iws->winsys.get_name = aub_get_name;
   iws->winsys.destroy = xlib_brw_destroy_pipe_winsys_aub;

   iws->winsys.surface_buffer_create = aub_i915_surface_buffer_create;

   iws->aubfile = brw_aubfile_create();
   iws->size = AUB_BUF_SIZE;
   iws->pool = malloc(AUB_BUF_SIZE);

   /* HACK: static copy of this pointer:
    */
   assert(global_winsys == NULL);
   global_winsys = iws;

   return &iws->winsys;
}


static struct pipe_screen *
xlib_create_brw_screen( void )
{
#ifndef GALLIUM_CELL
   struct pipe_winsys *winsys;
   struct pipe_screen *screen;

   winsys = xlib_create_brw_winsys();
   if (winsys == NULL)
      return NULL;

   screen = brw_create_screen(winsys, 0/* XXX pci_id */);
   if (screen == NULL)
      goto fail;

   return screen;

fail:
   if (winsys)
      winsys->destroy( winsys );

#endif
   return NULL;
}


/* These per-screen functions are acually made available to the driver
 * through the brw_winsys (per-context) entity.
 */
unsigned xlib_brw_get_buffer_offset( struct pipe_winsys *pws,
                                     struct pipe_buffer *buf,
                                     unsigned access_flags )
{
   return aub_bo(buf)->offset;
}

void xlib_brw_buffer_subdata_typed( struct pipe_winsys *pws,
                                    struct pipe_buffer *buf,
                                    unsigned long offset, 
                                    unsigned long size, 
                                    const void *data,
                                    unsigned data_type )
{
   unsigned aub_type = DW_GENERAL_STATE;
   unsigned aub_sub_type = 0;

   switch (data_type) {
   case BRW_CC_VP:
      aub_sub_type = DWGS_COLOR_CALC_VIEWPORT_STATE;
      break;
   case BRW_CC_UNIT:
      aub_sub_type = DWGS_COLOR_CALC_STATE;
      break;
   case BRW_WM_PROG:
      aub_sub_type = DWGS_KERNEL_INSTRUCTIONS;
      break;
   case BRW_SAMPLER_DEFAULT_COLOR:
      aub_sub_type = DWGS_SAMPLER_DEFAULT_COLOR;
      break;
   case BRW_SAMPLER:
      aub_sub_type = DWGS_SAMPLER_STATE;
      break;
   case BRW_WM_UNIT:
      aub_sub_type = DWGS_WINDOWER_IZ_STATE;
      break;
   case BRW_SF_PROG:
      aub_sub_type = DWGS_KERNEL_INSTRUCTIONS;
      break;
   case BRW_SF_VP:
      aub_sub_type = DWGS_STRIPS_FANS_VIEWPORT_STATE;
      break;
   case BRW_SF_UNIT:
      aub_sub_type = DWGS_STRIPS_FANS_STATE;
      break;
   case BRW_VS_UNIT:
      aub_sub_type = DWGS_VERTEX_SHADER_STATE;
      break;
   case BRW_VS_PROG:
      aub_sub_type = DWGS_KERNEL_INSTRUCTIONS;
      break;
   case BRW_GS_UNIT:
      aub_sub_type = DWGS_GEOMETRY_SHADER_STATE;
      break;
   case BRW_GS_PROG:
      aub_sub_type = DWGS_KERNEL_INSTRUCTIONS;
      break;
   case BRW_CLIP_VP:
      aub_sub_type = DWGS_CLIPPER_VIEWPORT_STATE;
      break;
   case BRW_CLIP_UNIT:
      aub_sub_type = DWGS_CLIPPER_STATE;
      break;
   case BRW_CLIP_PROG:
      aub_sub_type = DWGS_KERNEL_INSTRUCTIONS;
      break;
   case BRW_SS_SURFACE:
      aub_type = DW_SURFACE_STATE;
      aub_sub_type = DWSS_SURFACE_STATE; 
      break;
   case BRW_SS_SURF_BIND:
      aub_type = DW_SURFACE_STATE;
      aub_sub_type = DWSS_BINDING_TABLE_STATE; 
      break;
   case BRW_CONSTANT_BUFFER:
      aub_type = DW_CONSTANT_URB_ENTRY;
      aub_sub_type = 0; 
      break;

   default:
      assert(0);
      break;
   }

   {
      struct aub_pipe_winsys *iws = aub_pipe_winsys(pws);
      struct aub_buffer *sbo = aub_bo(buf);

      assert(sbo->size > offset + size);
      memcpy(sbo->data + offset, data, size);

      brw_aub_gtt_data( iws->aubfile, 
                        sbo->offset + offset,
                        sbo->data + offset,
                        size,
                        aub_type,
                        aub_sub_type );
   }
}
 

static void
xlib_brw_display_surface(struct xmesa_buffer *b, 
                         struct pipe_surface *surf)
{
   brw_aub_dump_bmp( global_winsys->aubfile, 
		     surf,
		     aub_bo(buffer_from_surface(surf))->offset );
}


struct xm_driver xlib_brw_driver = 
{
   .create_pipe_screen = xlib_create_brw_screen,
   .create_pipe_context = xlib_create_brw_context,
   .display_surface = xlib_brw_display_surface,
};
