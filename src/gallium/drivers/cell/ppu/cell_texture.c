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
  *   Brian Paul
  */

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_transfer.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#include "cell_context.h"
#include "cell_screen.h"
#include "cell_state.h"
#include "cell_texture.h"

#include "state_tracker/sw_winsys.h"



static boolean
cell_resource_layout(struct pipe_screen *screen, 
		     struct cell_resource *ct)
{
   struct pipe_resource *pt = &ct->base;
   unsigned level;
   unsigned width = pt->width0;
   unsigned height = pt->height0;
   unsigned depth = pt->depth0;

   ct->buffer_size = 0;

   for (level = 0; level <= pt->last_level; level++) {
      unsigned size;
      unsigned w_tile, h_tile;

      assert(level < CELL_MAX_TEXTURE_LEVELS);

      /* width, height, rounded up to tile size */
      w_tile = align(width, TILE_SIZE);
      h_tile = align(height, TILE_SIZE);

      ct->stride[level] = util_format_get_stride(pt->format, w_tile);

      ct->level_offset[level] = ct->buffer_size;

      size = ct->stride[level] * util_format_get_nblocksy(pt->format, h_tile);
      if (pt->target == PIPE_TEXTURE_CUBE)
         size *= 6;
      else
         size *= depth;

      ct->buffer_size += size;

      width = u_minify(width, 1);
      height = u_minify(height, 1);
      depth = u_minify(depth, 1);
   }

   ct->data = align_malloc(ct->buffer_size, 16);
 
   return ct->data != NULL;
}


/**
 * Texture layout for simple color buffers.
 */
static boolean
cell_displaytarget_layout(struct pipe_screen *screen,
                          struct cell_resource * ct)
{
   struct sw_winsys *winsys = cell_screen(screen)->winsys;

   /* Round up the surface size to a multiple of the tile size?
    */
   ct->dt = winsys->displaytarget_create(winsys,
                                          ct->base.bind,
                                          ct->base.format,
                                          ct->base.width0, 
                                          ct->base.height0,
                                          16,
                                          &ct->dt_stride );

   return ct->dt != NULL;
}

static struct pipe_resource *
cell_resource_create(struct pipe_screen *screen,
                    const struct pipe_resource *templat)
{
   struct cell_resource *ct = CALLOC_STRUCT(cell_resource);
   if (!ct)
      return NULL;

   ct->base = *templat;
   pipe_reference_init(&ct->base.reference, 1);
   ct->base.screen = screen;

   /* Create both a displaytarget (linear) and regular texture
    * (twiddled).  Convert twiddled->linear at flush_frontbuffer time.
    */
   if (ct->base.bind & (PIPE_BIND_DISPLAY_TARGET |
                        PIPE_BIND_SCANOUT |
                        PIPE_BIND_SHARED)) {
      if (!cell_displaytarget_layout(screen, ct))
         goto fail;
   }

   if (!cell_resource_layout(screen, ct))
      goto fail;

   return &ct->base;

fail:
   if (ct->dt) {
      struct sw_winsys *winsys = cell_screen(screen)->winsys;
      winsys->displaytarget_destroy(winsys, ct->dt);
   }

   FREE(ct);

   return NULL;
}


static void
cell_resource_destroy(struct pipe_screen *scrn, struct pipe_resource *pt)
{
   struct cell_screen *screen = cell_screen(scrn);
   struct sw_winsys *winsys = screen->winsys;
   struct cell_resource *ct = cell_resource(pt);

   if (ct->dt) {
      /* display target */
      winsys->displaytarget_destroy(winsys, ct->dt);
   }
   else if (!ct->userBuffer) {
      align_free(ct->data);
   }

   FREE(ct);
}



/**
 * Convert image from linear layout to tiled layout.  4-byte pixels.
 */
static void
twiddle_image_uint(uint w, uint h, uint tile_size, uint *dst,
                   uint src_stride, const uint *src)
{
   const uint tile_size2 = tile_size * tile_size;
   const uint h_t = (h + tile_size - 1) / tile_size;
   const uint w_t = (w + tile_size - 1) / tile_size;

   uint it, jt;  /* tile counters */
   uint i, j;    /* intra-tile counters */

   src_stride /= 4; /* convert from bytes to pixels */

   /* loop over dest tiles */
   for (it = 0; it < h_t; it++) {
      for (jt = 0; jt < w_t; jt++) {
         /* start of dest tile: */
         uint *tdst = dst + (it * w_t + jt) * tile_size2;

         /* compute size of this tile (may be smaller than tile_size) */
         /* XXX note: a compiler bug was found here. That's why the code
          * looks as it does.
          */
         uint tile_width = w - jt * tile_size;
         tile_width = MIN2(tile_width, tile_size);
         uint tile_height = h - it * tile_size;
         tile_height = MIN2(tile_height, tile_size);

         /* loop over texels in the tile */
         for (i = 0; i < tile_height; i++) {
            for (j = 0; j < tile_width; j++) {
               const uint srci = it * tile_size + i;
               const uint srcj = jt * tile_size + j;
               ASSERT(srci < h);
               ASSERT(srcj < w);
               tdst[i * tile_size + j] = src[srci * src_stride + srcj];
            }
         }
      }
   }
}


/**
 * For Cell.  Basically, rearrange the pixels/quads from this layout:
 *  +--+--+--+--+
 *  |p0|p1|p2|p3|....
 *  +--+--+--+--+
 *
 * to this layout:
 *  +--+--+
 *  |p0|p1|....
 *  +--+--+
 *  |p2|p3|
 *  +--+--+
 */
static void
twiddle_tile(const uint *tileIn, uint *tileOut)
{
   int y, x;

   for (y = 0; y < TILE_SIZE; y+=2) {
      for (x = 0; x < TILE_SIZE; x+=2) {
         int k = 4 * (y/2 * TILE_SIZE/2 + x/2);
         tileOut[y * TILE_SIZE + (x + 0)] = tileIn[k];
         tileOut[y * TILE_SIZE + (x + 1)] = tileIn[k+1];
         tileOut[(y + 1) * TILE_SIZE + (x + 0)] = tileIn[k+2];
         tileOut[(y + 1) * TILE_SIZE + (x + 1)] = tileIn[k+3];
      }
   }
}


/**
 * Convert image from tiled layout to linear layout.  4-byte pixels.
 */
static void
untwiddle_image_uint(uint w, uint h, uint tile_size, uint *dst,
                     uint dst_stride, const uint *src)
{
   const uint tile_size2 = tile_size * tile_size;
   const uint h_t = (h + tile_size - 1) / tile_size;
   const uint w_t = (w + tile_size - 1) / tile_size;
   uint *tile_buf;
   uint it, jt;  /* tile counters */
   uint i, j;    /* intra-tile counters */

   dst_stride /= 4; /* convert from bytes to pixels */

   tile_buf = align_malloc(tile_size * tile_size * 4, 16);
   
   /* loop over src tiles */
   for (it = 0; it < h_t; it++) {
      for (jt = 0; jt < w_t; jt++) {
         /* start of src tile: */
         const uint *tsrc = src + (it * w_t + jt) * tile_size2;
         
         twiddle_tile(tsrc, tile_buf);
         tsrc = tile_buf;

         /* compute size of this tile (may be smaller than tile_size) */
         /* XXX note: a compiler bug was found here. That's why the code
          * looks as it does.
          */
         uint tile_width = w - jt * tile_size;
         tile_width = MIN2(tile_width, tile_size);
         uint tile_height = h - it * tile_size;
         tile_height = MIN2(tile_height, tile_size);

         /* loop over texels in the tile */
         for (i = 0; i < tile_height; i++) {
            for (j = 0; j < tile_width; j++) {
               uint dsti = it * tile_size + i;
               uint dstj = jt * tile_size + j;
               ASSERT(dsti < h);
               ASSERT(dstj < w);
               dst[dsti * dst_stride + dstj] = tsrc[i * tile_size + j];
            }
         }
      }
   }

   align_free(tile_buf);
}


static struct pipe_surface *
cell_create_surface(struct pipe_context *ctx,
                    struct pipe_resource *pt,
                    const struct pipe_surface *surf_tmpl)
{
   struct cell_resource *ct = cell_resource(pt);
   struct pipe_surface *ps;

   assert(surf_tmpl->u.tex.first_layer == surf_tmpl->u.tex.last_layer);
   ps = CALLOC_STRUCT(pipe_surface);
   if (ps) {
      pipe_reference_init(&ps->reference, 1);
      pipe_resource_reference(&ps->texture, pt);
      ps->format = surf_tmpl->format;
      ps->context = ctx;
      ps->width = u_minify(pt->width0, surf_tmpl->u.tex.level);
      ps->height = u_minify(pt->height0, surf_tmpl->u.tex.level);
      /* XXX may need to override usage flags (see sp_texture.c) */
      ps->usage = surf_tmpl->usage;
      ps->u.tex.level = surf_tmpl->u.tex.level;
      ps->u.tex.first_layer = surf_tmpl->u.tex.first_layer;
      ps->u.tex.last_layer = surf_tmpl->u.tex.last_layer;
   }
   return ps;
}


static void 
cell_surface_destroy(struct pipe_context *ctx, struct pipe_surface *surf)
{
   pipe_resource_reference(&surf->texture, NULL);
   FREE(surf);
}


/**
 * Create new pipe_transfer object.
 * This is used by the user to put tex data into a texture (and get it
 * back out for glGetTexImage).
 */
static struct pipe_transfer *
cell_get_transfer(struct pipe_context *ctx,
                  struct pipe_resource *resource,
                  unsigned level,
                  unsigned usage,
                  const struct pipe_box *box)
{
   struct cell_resource *ct = cell_resource(resource);
   struct cell_transfer *ctrans;
   enum pipe_format format = resource->format;

   assert(resource);
   assert(level <= resource->last_level);

   /* make sure the requested region is in the image bounds */
   assert(box->x + box->width <= u_minify(resource->width0, level));
   assert(box->y + box->height <= u_minify(resource->height0, level));
   assert(box->z + box->depth <= (u_minify(resource->depth0, level) + resource->array_size - 1));

   ctrans = CALLOC_STRUCT(cell_transfer);
   if (ctrans) {
      struct pipe_transfer *pt = &ctrans->base;
      pipe_resource_reference(&pt->resource, resource);
      pt->level = level;
      pt->usage = usage;
      pt->box = *box;
      pt->stride = ct->stride[level];

      ctrans->offset = ct->level_offset[level];

      if (resource->target == PIPE_TEXTURE_CUBE || resource->target == PIPE_TEXTURE_3D) {
         unsigned h_tile = align(u_minify(resource->height0, level), TILE_SIZE);
         ctrans->offset += box->z * util_format_get_nblocksy(format, h_tile) * pt->stride;
      }
      else {
         assert(box->z == 0);
      }

      return pt;
   }
   return NULL;
}


static void 
cell_transfer_destroy(struct pipe_context *ctx, struct pipe_transfer *t)
{
   struct cell_transfer *transfer = cell_transfer(t);
   /* Effectively do the texture_update work here - if texture images
    * needed post-processing to put them into hardware layout, this is
    * where it would happen.  For cell, nothing to do.
    */
   assert (transfer->base.resource);
   pipe_resource_reference(&transfer->base.resource, NULL);
   FREE(transfer);
}


/**
 * Return pointer to texture image data in linear layout.
 */
static void *
cell_transfer_map(struct pipe_context *ctx, struct pipe_transfer *transfer)
{
   struct cell_transfer *ctrans = cell_transfer(transfer);
   struct pipe_resource *pt = transfer->resource;
   struct cell_resource *ct = cell_resource(pt);

   assert(transfer->resource);

   if (ct->mapped == NULL) {
      ct->mapped = ct->data;
   }


   /* Better test would be resource->is_linear
    */
   if (transfer->resource->target != PIPE_BUFFER) {
      const uint level = ctrans->base.level;
      const uint texWidth = u_minify(pt->width0, level);
      const uint texHeight = u_minify(pt->height0, level);
      unsigned size;


      /*
       * Create a buffer of ordinary memory for the linear texture.
       * This is the memory that the user will read/write.
       */
      size = (util_format_get_stride(pt->format, align(texWidth, TILE_SIZE)) *
	      util_format_get_nblocksy(pt->format, align(texHeight, TILE_SIZE)));

      ctrans->map = align_malloc(size, 16);
      if (!ctrans->map)
	 return NULL; /* out of memory */

      if (transfer->usage & PIPE_TRANSFER_READ) {
	 /* Textures always stored twiddled, need to untwiddle the
	  * texture to make a linear version.
	  */
	 const uint bpp = util_format_get_blocksize(ct->base.format);
	 if (bpp == 4) {
	    const uint *src = (uint *) (ct->mapped + ctrans->offset);
	    uint *dst = ctrans->map;
	    untwiddle_image_uint(texWidth, texHeight, TILE_SIZE,
				 dst, transfer->stride, src);
	 }
	 else {
	    // xxx fix
	 }
      }
   }
   else {
      unsigned stride = transfer->stride;
      enum pipe_format format = pt->format;
      unsigned blocksize = util_format_get_blocksize(format);

      ctrans->map = (ct->mapped + 
		     ctrans->offset +
		     ctrans->base.box.y / util_format_get_blockheight(format) * stride +
		     ctrans->base.box.x / util_format_get_blockwidth(format) * blocksize);
   }


   return ctrans->map;
}


/**
 * Called when user is done reading/writing texture data.
 * If new data was written, this is where we convert the linear data
 * to tiled data.
 */
static void
cell_transfer_unmap(struct pipe_context *ctx,
                    struct pipe_transfer *transfer)
{
   struct cell_transfer *ctrans = cell_transfer(transfer);
   struct pipe_resource *pt = transfer->resource;
   struct cell_resource *ct = cell_resource(pt);
   const uint level = ctrans->base.level;
   const uint texWidth = u_minify(pt->width0, level);
   const uint texHeight = u_minify(pt->height0, level);
   const uint stride = ct->stride[level];

   if (!ct->mapped) {
      assert(0);
      return;
   }

   if (pt->target != PIPE_BUFFER) {
      if (transfer->usage & PIPE_TRANSFER_WRITE) {
	 /* The user wrote new texture data into the mapped buffer.
	  * We need to convert the new linear data into the twiddled/tiled format.
	  */
	 const uint bpp = util_format_get_blocksize(ct->base.format);
	 if (bpp == 4) {
	    const uint *src = ctrans->map;
	    uint *dst = (uint *) (ct->mapped + ctrans->offset);
	    twiddle_image_uint(texWidth, texHeight, TILE_SIZE, dst, stride, src);
	 }
	 else {
	    // xxx fix
	 }
      }
      
      align_free(ctrans->map);
   }
   else {
      /* nothing to do */
   }

   ctrans->map = NULL;
}



/* This used to be overriden by the co-state tracker, but really needs
 * to be active with sw_winsys.
 *
 * Contrasting with llvmpipe and softpipe, this is the only place
 * where we use the ct->dt display target in any real sense.
 *
 * Basically just untwiddle our local data into the linear
 * displaytarget.
 */
static void
cell_flush_frontbuffer(struct pipe_screen *_screen,
                       struct pipe_resource *resource,
                       unsigned level, unsigned layer,
                       void *context_private)
{
   struct cell_screen *screen = cell_screen(_screen);
   struct sw_winsys *winsys = screen->winsys;
   struct cell_resource *ct = cell_resource(resource);

   if (!ct->dt)
      return;

   /* Need to untwiddle from our internal representation here:
    */
   {
      unsigned *map = winsys->displaytarget_map(winsys, ct->dt,
                                                (PIPE_TRANSFER_READ |
                                                 PIPE_TRANSFER_WRITE));
      unsigned *src = (unsigned *)(ct->data + ct->level_offset[level]);

      untwiddle_image_uint(u_minify(resource->width0, level),
                           u_minify(resource->height0, level),
                           TILE_SIZE,
                           map,
                           ct->dt_stride,
                           src);

      winsys->displaytarget_unmap(winsys, ct->dt);
   }

   winsys->displaytarget_display(winsys, ct->dt, context_private);
}



/**
 * Create buffer which wraps user-space data.
 */
static struct pipe_resource *
cell_user_buffer_create(struct pipe_screen *screen,
                            void *ptr,
                            unsigned bytes,
			    unsigned bind_flags)
{
   struct cell_resource *buffer;

   buffer = CALLOC_STRUCT(cell_resource);
   if(!buffer)
      return NULL;

   pipe_reference_init(&buffer->base.reference, 1);
   buffer->base.screen = screen;
   buffer->base.format = PIPE_FORMAT_R8_UNORM; /* ?? */
   buffer->base.bind = PIPE_BIND_TRANSFER_READ | bind_flags;
   buffer->base.usage = PIPE_USAGE_IMMUTABLE;
   buffer->base.flags = 0;
   buffer->base.width0 = bytes;
   buffer->base.height0 = 1;
   buffer->base.depth0 = 1;
   buffer->base.array_size = 1;
   buffer->userBuffer = TRUE;
   buffer->data = ptr;

   return &buffer->base;
}


static struct pipe_resource *
cell_resource_from_handle(struct pipe_screen *screen,
                          const struct pipe_resource *templat,
                          struct winsys_handle *handle)
{
   /* XXX todo */
   return NULL;
}


static boolean 
cell_resource_get_handle(struct pipe_screen *scree,
                         struct pipe_resource *tex,
                         struct winsys_handle *handle)
{
   /* XXX todo */
   return FALSE;
}


void
cell_init_screen_texture_funcs(struct pipe_screen *screen)
{
   screen->resource_create = cell_resource_create;
   screen->resource_destroy = cell_resource_destroy;
   screen->resource_from_handle = cell_resource_from_handle;
   screen->resource_get_handle = cell_resource_get_handle;
   screen->user_buffer_create = cell_user_buffer_create;

   screen->flush_frontbuffer = cell_flush_frontbuffer;
}

void
cell_init_texture_transfer_funcs(struct cell_context *cell)
{
   cell->pipe.get_transfer = cell_get_transfer;
   cell->pipe.transfer_destroy = cell_transfer_destroy;
   cell->pipe.transfer_map = cell_transfer_map;
   cell->pipe.transfer_unmap = cell_transfer_unmap;

   cell->pipe.transfer_flush_region = u_default_transfer_flush_region;
   cell->pipe.transfer_inline_write = u_default_transfer_inline_write;

   cell->pipe.create_surface = cell_create_surface;
   cell->pipe.surface_destroy = cell_surface_destroy;
}
