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

#include "lp_context.h"
#include "lp_screen.h"
#include "lp_flush.h"
#include "lp_texture.h"
#include "lp_tile_size.h"
#include "state_tracker/sw_winsys.h"


/**
 * Conventional allocation path for non-display textures:
 * Simple, maximally packed layout.
 */
static boolean
llvmpipe_texture_layout(struct llvmpipe_screen *screen,
                        struct llvmpipe_texture *lpt)
{
   struct pipe_texture *pt = &lpt->base;
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
                              struct llvmpipe_texture *lpt)
{
   struct sw_winsys *winsys = screen->winsys;

   /* Round up the surface size to a multiple of the tile size to
    * avoid tile clipping.
    */
   unsigned width = align(lpt->base.width0, TILE_SIZE);
   unsigned height = align(lpt->base.height0, TILE_SIZE);

   lpt->dt = winsys->displaytarget_create(winsys,
                                          lpt->base.format,
                                          width, height,
                                          16,
                                          &lpt->stride[0] );

   return lpt->dt != NULL;
}


static struct pipe_texture *
llvmpipe_texture_create(struct pipe_screen *_screen,
                        const struct pipe_texture *templat)
{
   struct llvmpipe_screen *screen = llvmpipe_screen(_screen);
   struct llvmpipe_texture *lpt = CALLOC_STRUCT(llvmpipe_texture);
   if (!lpt)
      return NULL;

   lpt->base = *templat;
   pipe_reference_init(&lpt->base.reference, 1);
   lpt->base.screen = &screen->base;

   if (lpt->base.tex_usage & (PIPE_TEXTURE_USAGE_DISPLAY_TARGET |
                              PIPE_TEXTURE_USAGE_SCANOUT |
                              PIPE_TEXTURE_USAGE_SHARED)) {
      if (!llvmpipe_displaytarget_layout(screen, lpt))
         goto fail;
   }
   else {
      if (!llvmpipe_texture_layout(screen, lpt))
         goto fail;
   }
    
   return &lpt->base;

 fail:
   FREE(lpt);
   return NULL;
}


static void
llvmpipe_texture_destroy(struct pipe_texture *pt)
{
   struct llvmpipe_screen *screen = llvmpipe_screen(pt->screen);
   struct llvmpipe_texture *lpt = llvmpipe_texture(pt);

   if (lpt->dt) {
      /* display target */
      struct sw_winsys *winsys = screen->winsys;
      winsys->displaytarget_destroy(winsys, lpt->dt);
   }
   else {
      /* regular texture */
      align_free(lpt->data);
   }

   FREE(lpt);
}


/**
 * Map a texture. Without any synchronization.
 */
void *
llvmpipe_texture_map(struct pipe_texture *texture,
                     unsigned face,
                     unsigned level,
                     unsigned zslice)
{
   struct llvmpipe_texture *lpt = llvmpipe_texture(texture);
   uint8_t *map;

   if (lpt->dt) {
      /* display target */
      struct llvmpipe_screen *screen = llvmpipe_screen(texture->screen);
      struct sw_winsys *winsys = screen->winsys;
      const unsigned usage = PIPE_BUFFER_USAGE_CPU_READ_WRITE;

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
         to account for alignment done in llvmpipe_texture_layout ?
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
llvmpipe_texture_unmap(struct pipe_texture *texture,
                       unsigned face,
                       unsigned level,
                       unsigned zslice)
{
   struct llvmpipe_texture *lpt = llvmpipe_texture(texture);

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


static struct pipe_surface *
llvmpipe_get_tex_surface(struct pipe_screen *screen,
                         struct pipe_texture *pt,
                         unsigned face, unsigned level, unsigned zslice,
                         unsigned usage)
{
   struct llvmpipe_texture *lpt = llvmpipe_texture(pt);
   struct pipe_surface *ps;

   assert(level <= pt->last_level);

   ps = CALLOC_STRUCT(pipe_surface);
   if (ps) {
      pipe_reference_init(&ps->reference, 1);
      pipe_texture_reference(&ps->texture, pt);
      ps->format = pt->format;
      ps->width = u_minify(pt->width0, level);
      ps->height = u_minify(pt->height0, level);
      ps->usage = usage;

      /* Because we are llvmpipe, anything that the state tracker
       * thought was going to be done with the GPU will actually get
       * done with the CPU.  Let's adjust the flags to take that into
       * account.
       */
      if (ps->usage & PIPE_BUFFER_USAGE_GPU_WRITE) {
         /* GPU_WRITE means "render" and that can involve reads (blending) */
         ps->usage |= PIPE_BUFFER_USAGE_CPU_WRITE | PIPE_BUFFER_USAGE_CPU_READ;
      }

      if (ps->usage & PIPE_BUFFER_USAGE_GPU_READ)
         ps->usage |= PIPE_BUFFER_USAGE_CPU_READ;

      if (ps->usage & (PIPE_BUFFER_USAGE_CPU_WRITE |
                       PIPE_BUFFER_USAGE_GPU_WRITE)) {
         /* Mark the surface as dirty. */
         lpt->timestamp++;
         llvmpipe_screen(screen)->timestamp++;
      }

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
   pipe_texture_reference(&surf->texture, NULL);
   FREE(surf);
}


static struct pipe_transfer *
llvmpipe_get_tex_transfer(struct pipe_context *pipe,
                          struct pipe_texture *texture,
                          unsigned face, unsigned level, unsigned zslice,
                          enum pipe_transfer_usage usage,
                          unsigned x, unsigned y, unsigned w, unsigned h)
{
   struct llvmpipe_texture *lptex = llvmpipe_texture(texture);
   struct llvmpipe_transfer *lpt;

   assert(texture);
   assert(level <= texture->last_level);

   lpt = CALLOC_STRUCT(llvmpipe_transfer);
   if (lpt) {
      struct pipe_transfer *pt = &lpt->base;
      pipe_texture_reference(&pt->texture, texture);
      pt->x = x;
      pt->y = y;
      pt->width = align(w, TILE_SIZE);
      pt->height = align(h, TILE_SIZE);
      pt->stride = lptex->stride[level];
      pt->usage = usage;
      pt->face = face;
      pt->level = level;
      pt->zslice = zslice;

      return pt;
   }
   return NULL;
}


static void 
llvmpipe_tex_transfer_destroy(struct pipe_context *pipe,
                              struct pipe_transfer *transfer)
{
   /* Effectively do the texture_update work here - if texture images
    * needed post-processing to put them into hardware layout, this is
    * where it would happen.  For llvmpipe, nothing to do.
    */
   assert (transfer->texture);
   pipe_texture_reference(&transfer->texture, NULL);
   FREE(transfer);
}


static void *
llvmpipe_transfer_map( struct pipe_context *pipe,
                       struct pipe_transfer *transfer )
{
   struct llvmpipe_screen *screen = llvmpipe_screen(pipe->screen);
   ubyte *map;
   struct llvmpipe_texture *lpt;
   enum pipe_format format;

   assert(transfer->texture);
   lpt = llvmpipe_texture(transfer->texture);
   format = lpt->base.format;

   /*
    * Transfers, like other pipe operations, must happen in order, so flush the
    * context if necessary.
    */
   llvmpipe_flush_texture(pipe,
                          transfer->texture, transfer->face, transfer->level,
                          0, /* flush_flags */
                          !(transfer->usage & PIPE_TRANSFER_WRITE), /* read_only */
                          TRUE, /* cpu_access */
                          FALSE); /* do_not_flush */

   map = llvmpipe_texture_map(transfer->texture,
                              transfer->face, transfer->level, transfer->zslice);

   /* May want to different things here depending on read/write nature
    * of the map:
    */
   if (transfer->usage & PIPE_TRANSFER_WRITE) {
      /* Do something to notify sharing contexts of a texture change.
       */
      screen->timestamp++;
   }
   
   map +=
      transfer->y / util_format_get_blockheight(format) * transfer->stride +
      transfer->x / util_format_get_blockwidth(format) * util_format_get_blocksize(format);

   return map;
}


static void
llvmpipe_transfer_unmap(struct pipe_context *pipe,
                        struct pipe_transfer *transfer)
{
   assert(transfer->texture);

   llvmpipe_texture_unmap(transfer->texture,
                          transfer->face, transfer->level, transfer->zslice);
}


void
llvmpipe_init_screen_texture_funcs(struct pipe_screen *screen)
{
   screen->texture_create = llvmpipe_texture_create;
   screen->texture_destroy = llvmpipe_texture_destroy;

   screen->get_tex_surface = llvmpipe_get_tex_surface;
   screen->tex_surface_destroy = llvmpipe_tex_surface_destroy;
}


void
llvmpipe_init_context_texture_funcs(struct pipe_context *pipe)
{
   pipe->get_tex_transfer = llvmpipe_get_tex_transfer;
   pipe->tex_transfer_destroy = llvmpipe_tex_transfer_destroy;
   pipe->transfer_map = llvmpipe_transfer_map;
   pipe->transfer_unmap = llvmpipe_transfer_unmap;
}
