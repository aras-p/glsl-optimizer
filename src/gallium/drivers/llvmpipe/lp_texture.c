/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Michel Dänzer <michel@tungstengraphics.com>
  */

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"

#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_transfer.h"

#include "lp_context.h"
#include "lp_screen.h"
#include "lp_flush.h"
#include "lp_texture.h"
#include "lp_setup.h"
#include "lp_tile_size.h"
#include "state_tracker/sw_winsys.h"


/**
 * Conventional allocation path for non-display textures:
 * Simple, maximally packed layout.
 */
static boolean
llvmpipe_resource_layout(struct llvmpipe_screen *screen,
                        struct llvmpipe_resource *lpt)
{
   struct pipe_resource *pt = &lpt->base;
   unsigned level;
   unsigned width = pt->width0;
   unsigned height = pt->height0;
   unsigned depth = pt->depth0;
   unsigned buffer_size = 0;

   for (level = 0; level <= pt->last_level; level++) {
      unsigned nblocksx, nblocksy;

      /* Allocate storage for whole quads. This is particularly important
       * for depth surfaces, which are currently stored in a swizzled format.
       */
      nblocksx = util_format_get_nblocksx(pt->format, align(width, TILE_SIZE));
      nblocksy = util_format_get_nblocksy(pt->format, align(height, TILE_SIZE));

      lpt->stride[level] = align(nblocksx * util_format_get_blocksize(pt->format), 16);

      lpt->level_offset[level] = buffer_size;

      buffer_size += (nblocksy *
                      ((pt->target == PIPE_TEXTURE_CUBE) ? 6 : depth) *
                      lpt->stride[level]);

      width = u_minify(width, 1);
      height = u_minify(height, 1);
      depth = u_minify(depth, 1);
   }

   lpt->data = align_malloc(buffer_size, 16);

   return lpt->data != NULL;
}



static boolean
llvmpipe_displaytarget_layout(struct llvmpipe_screen *screen,
                              struct llvmpipe_resource *lpt)
{
   struct sw_winsys *winsys = screen->winsys;

   /* Round up the surface size to a multiple of the tile size to
    * avoid tile clipping.
    */
   unsigned width = align(lpt->base.width0, TILE_SIZE);
   unsigned height = align(lpt->base.height0, TILE_SIZE);

   lpt->dt = winsys->displaytarget_create(winsys,
                                          lpt->base.bind,
                                          lpt->base.format,
                                          width, height,
                                          16,
                                          &lpt->stride[0] );

   return lpt->dt != NULL;
}


static struct pipe_resource *
llvmpipe_resource_create(struct pipe_screen *_screen,
                        const struct pipe_resource *templat)
{
   struct llvmpipe_screen *screen = llvmpipe_screen(_screen);
   struct llvmpipe_resource *lpt = CALLOC_STRUCT(llvmpipe_resource);
   if (!lpt)
      return NULL;

   lpt->base = *templat;
   pipe_reference_init(&lpt->base.reference, 1);
   lpt->base.screen = &screen->base;

   if (lpt->base.bind & (PIPE_BIND_DISPLAY_TARGET |
                         PIPE_BIND_SCANOUT |
                         PIPE_BIND_SHARED)) {
      if (!llvmpipe_displaytarget_layout(screen, lpt))
         goto fail;
   }
   else {
      if (!llvmpipe_resource_layout(screen, lpt))
         goto fail;
   }

   return &lpt->base;

 fail:
   FREE(lpt);
   return NULL;
}


static void
llvmpipe_resource_destroy(struct pipe_screen *pscreen,
			  struct pipe_resource *pt)
{
   struct llvmpipe_screen *screen = llvmpipe_screen(pscreen);
   struct llvmpipe_resource *lpt = llvmpipe_resource(pt);

   if (lpt->dt) {
      /* display target */
      struct sw_winsys *winsys = screen->winsys;
      winsys->displaytarget_destroy(winsys, lpt->dt);
   }
   else if (!lpt->userBuffer) {
      /* regular texture */
      align_free(lpt->data);
   }

   FREE(lpt);
}


/**
 * Map a texture. Without any synchronization.
 */
void *
llvmpipe_resource_map(struct pipe_resource *texture,
		      unsigned usage,
		      unsigned face,
		      unsigned level,
		      unsigned zslice)
{
   struct llvmpipe_resource *lpt = llvmpipe_resource(texture);
   uint8_t *map;

   if (lpt->dt) {
      /* display target */
      struct llvmpipe_screen *screen = llvmpipe_screen(texture->screen);
      struct sw_winsys *winsys = screen->winsys;

      assert(face == 0);
      assert(level == 0);
      assert(zslice == 0);

      /* FIXME: keep map count? */
      map = winsys->displaytarget_map(winsys, lpt->dt, usage);
   }
   else {
      /* regular texture */
      unsigned offset;
      unsigned stride;

      map = lpt->data;

      assert(level < LP_MAX_TEXTURE_2D_LEVELS);

      offset = lpt->level_offset[level];
      stride = lpt->stride[level];

      /* XXX shouldn't that rather be
         tex_height = align(u_minify(texture->height0, level), 2)
         to account for alignment done in llvmpipe_resource_layout ?
      */
      if (texture->target == PIPE_TEXTURE_CUBE) {
         unsigned tex_height = u_minify(texture->height0, level);
         offset += face *  util_format_get_nblocksy(texture->format, tex_height) * stride;
      }
      else if (texture->target == PIPE_TEXTURE_3D) {
         unsigned tex_height = u_minify(texture->height0, level);
         offset += zslice * util_format_get_nblocksy(texture->format, tex_height) * stride;
      }
      else {
         assert(face == 0);
         assert(zslice == 0);
      }

      map += offset;
   }

   return map;
}


/**
 * Unmap a texture. Without any synchronization.
 */
void
llvmpipe_resource_unmap(struct pipe_resource *texture,
                       unsigned face,
                       unsigned level,
                       unsigned zslice)
{
   struct llvmpipe_resource *lpt = llvmpipe_resource(texture);

   if (lpt->dt) {
      /* display target */
      struct llvmpipe_screen *lp_screen = llvmpipe_screen(texture->screen);
      struct sw_winsys *winsys = lp_screen->winsys;

      assert(face == 0);
      assert(level == 0);
      assert(zslice == 0);

      winsys->displaytarget_unmap(winsys, lpt->dt);
   }
}


static struct pipe_resource *
llvmpipe_resource_from_handle(struct pipe_screen *screen,
			      const struct pipe_resource *template,
			      struct winsys_handle *whandle)
{
   struct sw_winsys *winsys = llvmpipe_screen(screen)->winsys;
   struct llvmpipe_resource *lpt = CALLOC_STRUCT(llvmpipe_resource);
   if (!lpt)
      return NULL;

   lpt->base = *template;
   pipe_reference_init(&lpt->base.reference, 1);
   lpt->base.screen = screen;

   lpt->dt = winsys->displaytarget_from_handle(winsys,
                                               template,
                                               whandle,
                                               &lpt->stride[0]);
   if (!lpt->dt)
      goto fail;

   return &lpt->base;

 fail:
   FREE(lpt);
   return NULL;
}


static boolean
llvmpipe_resource_get_handle(struct pipe_screen *screen,
                            struct pipe_resource *pt,
                            struct winsys_handle *whandle)
{
   struct sw_winsys *winsys = llvmpipe_screen(screen)->winsys;
   struct llvmpipe_resource *lpt = llvmpipe_resource(pt);

   assert(lpt->dt);
   if (!lpt->dt)
      return FALSE;

   return winsys->displaytarget_get_handle(winsys, lpt->dt, whandle);
}


static struct pipe_surface *
llvmpipe_get_tex_surface(struct pipe_screen *screen,
                         struct pipe_resource *pt,
                         unsigned face, unsigned level, unsigned zslice,
                         unsigned usage)
{
   struct pipe_surface *ps;

   assert(level <= pt->last_level);

   ps = CALLOC_STRUCT(pipe_surface);
   if (ps) {
      pipe_reference_init(&ps->reference, 1);
      pipe_resource_reference(&ps->texture, pt);
      ps->format = pt->format;
      ps->width = u_minify(pt->width0, level);
      ps->height = u_minify(pt->height0, level);
      ps->usage = usage;

      ps->face = face;
      ps->level = level;
      ps->zslice = zslice;
   }
   return ps;
}


static void 
llvmpipe_tex_surface_destroy(struct pipe_surface *surf)
{
   /* Effectively do the texture_update work here - if texture images
    * needed post-processing to put them into hardware layout, this is
    * where it would happen.  For llvmpipe, nothing to do.
    */
   assert(surf->texture);
   pipe_resource_reference(&surf->texture, NULL);
   FREE(surf);
}


static struct pipe_transfer *
llvmpipe_get_transfer(struct pipe_context *pipe,
		      struct pipe_resource *resource,
		      struct pipe_subresource sr,
		      unsigned usage,
		      const struct pipe_box *box)
{
   struct llvmpipe_resource *lptex = llvmpipe_resource(resource);
   struct llvmpipe_transfer *lpt;

   assert(resource);
   assert(sr.level <= resource->last_level);

   lpt = CALLOC_STRUCT(llvmpipe_transfer);
   if (lpt) {
      struct pipe_transfer *pt = &lpt->base;
      pipe_resource_reference(&pt->resource, resource);
      pt->box = *box;
      pt->sr = sr;
      pt->stride = lptex->stride[sr.level];
      pt->usage = usage;

      return pt;
   }
   return NULL;
}


static void 
llvmpipe_transfer_destroy(struct pipe_context *pipe,
                              struct pipe_transfer *transfer)
{
   /* Effectively do the texture_update work here - if texture images
    * needed post-processing to put them into hardware layout, this is
    * where it would happen.  For llvmpipe, nothing to do.
    */
   assert (transfer->resource);
   pipe_resource_reference(&transfer->resource, NULL);
   FREE(transfer);
}


static void *
llvmpipe_transfer_map( struct pipe_context *pipe,
                       struct pipe_transfer *transfer )
{
   struct llvmpipe_screen *screen = llvmpipe_screen(pipe->screen);
   ubyte *map;
   struct llvmpipe_resource *lpt;
   enum pipe_format format;

   assert(transfer->resource);
   lpt = llvmpipe_resource(transfer->resource);
   format = lpt->base.format;

   /*
    * Transfers, like other pipe operations, must happen in order, so flush the
    * context if necessary.
    */
   llvmpipe_flush_texture(pipe,
                          transfer->resource,
			  transfer->sr.face,
			  transfer->sr.level,
                          0, /* flush_flags */
                          !(transfer->usage & PIPE_TRANSFER_WRITE), /* read_only */
                          TRUE, /* cpu_access */
                          FALSE); /* do_not_flush */

   map = llvmpipe_resource_map(transfer->resource,
			       transfer->usage,
			       transfer->sr.face,
			       transfer->sr.level,
			       transfer->box.z);

   /* May want to different things here depending on read/write nature
    * of the map:
    */
   if (transfer->usage & PIPE_TRANSFER_WRITE) {
      /* Do something to notify sharing contexts of a texture change.
       */
      screen->timestamp++;
   }
   
   map +=
      transfer->box.y / util_format_get_blockheight(format) * transfer->stride +
      transfer->box.x / util_format_get_blockwidth(format) * util_format_get_blocksize(format);

   return map;
}


static void
llvmpipe_transfer_unmap(struct pipe_context *pipe,
                        struct pipe_transfer *transfer)
{
   assert(transfer->resource);

   llvmpipe_resource_unmap(transfer->resource,
			   transfer->sr.face,
			   transfer->sr.level,
			   transfer->box.z);
}

static unsigned int
llvmpipe_is_resource_referenced( struct pipe_context *pipe,
				struct pipe_resource *presource,
				unsigned face, unsigned level)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context( pipe );

   if (presource->target == PIPE_BUFFER)
      return PIPE_UNREFERENCED;
   
   return lp_setup_is_resource_referenced(llvmpipe->setup, presource);
}



/**
 * Create buffer which wraps user-space data.
 */
static struct pipe_resource *
llvmpipe_user_buffer_create(struct pipe_screen *screen,
                            void *ptr,
                            unsigned bytes,
			    unsigned bind_flags)
{
   struct llvmpipe_resource *buffer;

   buffer = CALLOC_STRUCT(llvmpipe_resource);
   if(!buffer)
      return NULL;

   pipe_reference_init(&buffer->base.reference, 1);
   buffer->base.screen = screen;
   buffer->base.format = PIPE_FORMAT_R8_UNORM; /* ?? */
   buffer->base.bind = bind_flags;
   buffer->base._usage = PIPE_USAGE_IMMUTABLE;
   buffer->base.flags = 0;
   buffer->base.width0 = bytes;
   buffer->base.height0 = 1;
   buffer->base.depth0 = 1;
   buffer->userBuffer = TRUE;
   buffer->data = ptr;

   return &buffer->base;
}


void
llvmpipe_init_screen_resource_funcs(struct pipe_screen *screen)
{
   screen->resource_create = llvmpipe_resource_create;
   screen->resource_destroy = llvmpipe_resource_destroy;
   screen->resource_from_handle = llvmpipe_resource_from_handle;
   screen->resource_get_handle = llvmpipe_resource_get_handle;
   screen->user_buffer_create = llvmpipe_user_buffer_create;

   screen->get_tex_surface = llvmpipe_get_tex_surface;
   screen->tex_surface_destroy = llvmpipe_tex_surface_destroy;
}


void
llvmpipe_init_context_resource_funcs(struct pipe_context *pipe)
{
   pipe->get_transfer = llvmpipe_get_transfer;
   pipe->transfer_destroy = llvmpipe_transfer_destroy;
   pipe->transfer_map = llvmpipe_transfer_map;
   pipe->transfer_unmap = llvmpipe_transfer_unmap;
   pipe->is_resource_referenced = llvmpipe_is_resource_referenced;
 
   pipe->transfer_flush_region = u_default_transfer_flush_region;
   pipe->transfer_inline_write = u_default_transfer_inline_write;
}
