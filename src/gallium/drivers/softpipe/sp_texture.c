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
#include "pipe/p_inlines.h"
#include "pipe/internal/p_winsys_screen.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#include "sp_context.h"
#include "sp_state.h"
#include "sp_texture.h"
#include "sp_tile_cache.h"
#include "sp_screen.h"
#include "sp_winsys.h"


/* Simple, maximally packed layout.
 */

static unsigned minify( unsigned d )
{
   return MAX2(1, d>>1);
}


/* Conventional allocation path for non-display textures:
 */
static boolean
softpipe_texture_layout(struct pipe_screen *screen,
                        struct softpipe_texture * spt)
{
   struct pipe_texture *pt = &spt->base;
   unsigned level;
   unsigned width = pt->width[0];
   unsigned height = pt->height[0];
   unsigned depth = pt->depth[0];

   unsigned buffer_size = 0;

   for (level = 0; level <= pt->last_level; level++) {
      pt->width[level] = width;
      pt->height[level] = height;
      pt->depth[level] = depth;
      pt->nblocksx[level] = pf_get_nblocksx(&pt->block, width);  
      pt->nblocksy[level] = pf_get_nblocksy(&pt->block, height);  
      spt->stride[level] = pt->nblocksx[level]*pt->block.size;

      spt->level_offset[level] = buffer_size;

      buffer_size += (pt->nblocksy[level] *
                      ((pt->target == PIPE_TEXTURE_CUBE) ? 6 : depth) *
                      spt->stride[level]);

      width  = minify(width);
      height = minify(height);
      depth = minify(depth);
   }

   spt->buffer = screen->buffer_create(screen, 32,
                                       PIPE_BUFFER_USAGE_PIXEL,
                                       buffer_size);

   return spt->buffer != NULL;
}

static boolean
softpipe_displaytarget_layout(struct pipe_screen *screen,
                              struct softpipe_texture * spt)
{
   unsigned usage = (PIPE_BUFFER_USAGE_CPU_READ_WRITE |
                     PIPE_BUFFER_USAGE_GPU_READ_WRITE);

   spt->base.nblocksx[0] = pf_get_nblocksx(&spt->base.block, spt->base.width[0]);  
   spt->base.nblocksy[0] = pf_get_nblocksy(&spt->base.block, spt->base.height[0]);  

   spt->buffer = screen->surface_buffer_create( screen, 
                                                spt->base.width[0], 
                                                spt->base.height[0],
                                                spt->base.format,
                                                usage,
                                                &spt->stride[0]);

   return spt->buffer != NULL;
}





static struct pipe_texture *
softpipe_texture_create(struct pipe_screen *screen,
                        const struct pipe_texture *templat)
{
   struct softpipe_texture *spt = CALLOC_STRUCT(softpipe_texture);
   if (!spt)
      return NULL;

   spt->base = *templat;
   pipe_reference_init(&spt->base.reference, 1);
   spt->base.screen = screen;

   if (spt->base.tex_usage & PIPE_TEXTURE_USAGE_DISPLAY_TARGET) {
      if (!softpipe_displaytarget_layout(screen, spt))
         goto fail;
   }
   else {
      if (!softpipe_texture_layout(screen, spt))
         goto fail;
   }
    
   return &spt->base;

 fail:
   FREE(spt);
   return NULL;
}


static struct pipe_texture *
softpipe_texture_blanket(struct pipe_screen * screen,
                         const struct pipe_texture *base,
                         const unsigned *stride,
                         struct pipe_buffer *buffer)
{
   struct softpipe_texture *spt;
   assert(screen);

   /* Only supports one type */
   if (base->target != PIPE_TEXTURE_2D ||
       base->last_level != 0 ||
       base->depth[0] != 1) {
      return NULL;
   }

   spt = CALLOC_STRUCT(softpipe_texture);
   if (!spt)
      return NULL;

   spt->base = *base;
   pipe_reference_init(&spt->base.reference, 1);
   spt->base.screen = screen;
   spt->base.nblocksx[0] = pf_get_nblocksx(&spt->base.block, spt->base.width[0]);  
   spt->base.nblocksy[0] = pf_get_nblocksy(&spt->base.block, spt->base.height[0]);  
   spt->stride[0] = stride[0];

   pipe_buffer_reference(&spt->buffer, buffer);

   return &spt->base;
}


static void
softpipe_texture_destroy(struct pipe_texture *pt)
{
   struct softpipe_texture *spt = softpipe_texture(pt);

   pipe_buffer_reference(&spt->buffer, NULL);
   FREE(spt);
}


static struct pipe_surface *
softpipe_get_tex_surface(struct pipe_screen *screen,
                         struct pipe_texture *pt,
                         unsigned face, unsigned level, unsigned zslice,
                         unsigned usage)
{
   struct softpipe_texture *spt = softpipe_texture(pt);
   struct pipe_surface *ps;

   assert(level <= pt->last_level);

   ps = CALLOC_STRUCT(pipe_surface);
   if (ps) {
      pipe_reference_init(&ps->reference, 1);
      pipe_texture_reference(&ps->texture, pt);
      ps->format = pt->format;
      ps->width = pt->width[level];
      ps->height = pt->height[level];
      ps->offset = spt->level_offset[level];
      ps->usage = usage;

      /* Because we are softpipe, anything that the state tracker
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
         /* Mark the surface as dirty.  The tile cache will look for this. */
         spt->modified = TRUE;
      }

      ps->face = face;
      ps->level = level;
      ps->zslice = zslice;

      if (pt->target == PIPE_TEXTURE_CUBE) {
         ps->offset += face * pt->nblocksy[level] * spt->stride[level];
      }
      else if (pt->target == PIPE_TEXTURE_3D) {
         ps->offset += zslice * pt->nblocksy[level] * spt->stride[level];
      }
      else {
         assert(face == 0);
         assert(zslice == 0);
      }
   }
   return ps;
}


static void 
softpipe_tex_surface_destroy(struct pipe_surface *surf)
{
   /* Effectively do the texture_update work here - if texture images
    * needed post-processing to put them into hardware layout, this is
    * where it would happen.  For softpipe, nothing to do.
    */
   assert(surf->texture);
   pipe_texture_reference(&surf->texture, NULL);
   FREE(surf);
}


static struct pipe_transfer *
softpipe_get_tex_transfer(struct pipe_screen *screen,
                          struct pipe_texture *texture,
                          unsigned face, unsigned level, unsigned zslice,
                          enum pipe_transfer_usage usage,
                          unsigned x, unsigned y, unsigned w, unsigned h)
{
   struct softpipe_texture *sptex = softpipe_texture(texture);
   struct softpipe_transfer *spt;

   assert(texture);
   assert(level <= texture->last_level);

   spt = CALLOC_STRUCT(softpipe_transfer);
   if (spt) {
      struct pipe_transfer *pt = &spt->base;
      pipe_texture_reference(&pt->texture, texture);
      pt->format = texture->format;
      pt->block = texture->block;
      pt->x = x;
      pt->y = y;
      pt->width = w;
      pt->height = h;
      pt->nblocksx = texture->nblocksx[level];
      pt->nblocksy = texture->nblocksy[level];
      pt->stride = sptex->stride[level];
      pt->usage = usage;
      pt->face = face;
      pt->level = level;
      pt->zslice = zslice;

      spt->offset = sptex->level_offset[level];

      if (texture->target == PIPE_TEXTURE_CUBE) {
         spt->offset += face * pt->nblocksy * pt->stride;
      }
      else if (texture->target == PIPE_TEXTURE_3D) {
         spt->offset += zslice * pt->nblocksy * pt->stride;
      }
      else {
         assert(face == 0);
         assert(zslice == 0);
      }
      return pt;
   }
   return NULL;
}


static void 
softpipe_tex_transfer_destroy(struct pipe_transfer *transfer)
{
   /* Effectively do the texture_update work here - if texture images
    * needed post-processing to put them into hardware layout, this is
    * where it would happen.  For softpipe, nothing to do.
    */
   assert (transfer->texture);
   pipe_texture_reference(&transfer->texture, NULL);
   FREE(transfer);
}


static void *
softpipe_transfer_map( struct pipe_screen *screen,
                       struct pipe_transfer *transfer )
{
   ubyte *map, *xfer_map;
   struct softpipe_texture *spt;
   unsigned flags = 0;

   assert(transfer->texture);
   spt = softpipe_texture(transfer->texture);

   if (transfer->usage != PIPE_TRANSFER_READ) {
      flags |= PIPE_BUFFER_USAGE_CPU_WRITE;
   }

   if (transfer->usage != PIPE_TRANSFER_WRITE) {
      flags |= PIPE_BUFFER_USAGE_CPU_READ;
   }

   map = pipe_buffer_map(screen, spt->buffer, flags);
   if (map == NULL)
      return NULL;

   /* May want to different things here depending on read/write nature
    * of the map:
    */
   if (transfer->texture && transfer->usage != PIPE_TRANSFER_READ) 
   {
      /* Do something to notify sharing contexts of a texture change.
       * In softpipe, that would mean flushing the texture cache.
       */
      softpipe_screen(screen)->timestamp++;
   }
   
   xfer_map = map + softpipe_transfer(transfer)->offset +
      transfer->y / transfer->block.height * transfer->stride +
      transfer->x / transfer->block.width * transfer->block.size;
   /*printf("map = %p  xfer map = %p\n", map, xfer_map);*/
   return xfer_map;
}


static void
softpipe_transfer_unmap(struct pipe_screen *screen,
                       struct pipe_transfer *transfer)
{
   struct softpipe_texture *spt;

   assert(transfer->texture);
   spt = softpipe_texture(transfer->texture);

   pipe_buffer_unmap( screen, spt->buffer );
}


void
softpipe_init_texture_funcs(struct softpipe_context *sp)
{
}


void
softpipe_init_screen_texture_funcs(struct pipe_screen *screen)
{
   screen->texture_create = softpipe_texture_create;
   screen->texture_blanket = softpipe_texture_blanket;
   screen->texture_destroy = softpipe_texture_destroy;

   screen->get_tex_surface = softpipe_get_tex_surface;
   screen->tex_surface_destroy = softpipe_tex_surface_destroy;

   screen->get_tex_transfer = softpipe_get_tex_transfer;
   screen->tex_transfer_destroy = softpipe_tex_transfer_destroy;
   screen->transfer_map = softpipe_transfer_map;
   screen->transfer_unmap = softpipe_transfer_unmap;
}


boolean
softpipe_get_texture_buffer( struct pipe_texture *texture,
                             struct pipe_buffer **buf,
                             unsigned *stride )
{
   struct softpipe_texture *tex = (struct softpipe_texture *)texture;

   if (!tex)
      return FALSE;

   pipe_buffer_reference(buf, tex->buffer);

   if (stride)
      *stride = tex->stride[0];

   return TRUE;
}
