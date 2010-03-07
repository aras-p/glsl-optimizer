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

#include "pipe/p_defines.h"
#include "util/u_inlines.h"

#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#include "sp_context.h"
#include "sp_texture.h"
#include "sp_screen.h"
#include "sp_winsys.h"


/**
 * Conventional allocation path for non-display textures:
 * Use a simple, maximally packed layout.
 */
static boolean
softpipe_texture_layout(struct pipe_screen *screen,
                        struct softpipe_texture * spt)
{
   struct pipe_texture *pt = &spt->base;
   unsigned level;
   unsigned width = pt->width0;
   unsigned height = pt->height0;
   unsigned depth = pt->depth0;
   unsigned buffer_size = 0;

   for (level = 0; level <= pt->last_level; level++) {
      spt->stride[level] = util_format_get_stride(pt->format, width);

      spt->level_offset[level] = buffer_size;

      buffer_size += (util_format_get_nblocksy(pt->format, height) *
                      ((pt->target == PIPE_TEXTURE_CUBE) ? 6 : depth) *
                      spt->stride[level]);

      width  = u_minify(width, 1);
      height = u_minify(height, 1);
      depth = u_minify(depth, 1);
   }

   spt->buffer = screen->buffer_create(screen, 32,
                                       PIPE_BUFFER_USAGE_PIXEL,
                                       buffer_size);

   return spt->buffer != NULL;
}


/**
 * Texture layout for simple color buffers.
 */
static boolean
softpipe_displaytarget_layout(struct pipe_screen *screen,
                              struct softpipe_texture * spt)
{
   unsigned usage = (PIPE_BUFFER_USAGE_CPU_READ_WRITE |
                     PIPE_BUFFER_USAGE_GPU_READ_WRITE);
   unsigned tex_usage = spt->base.tex_usage;

   spt->buffer = screen->surface_buffer_create( screen, 
                                                spt->base.width0, 
                                                spt->base.height0,
                                                spt->base.format,
                                                usage,
                                                tex_usage,
                                                &spt->stride[0]);

   return spt->buffer != NULL;
}


/**
 * Create new pipe_texture given the template information.
 */
static struct pipe_texture *
softpipe_texture_create(struct pipe_screen *screen,
                        const struct pipe_texture *template)
{
   struct softpipe_texture *spt = CALLOC_STRUCT(softpipe_texture);
   if (!spt)
      return NULL;

   spt->base = *template;
   pipe_reference_init(&spt->base.reference, 1);
   spt->base.screen = screen;

   spt->pot = (util_is_power_of_two(template->width0) &&
               util_is_power_of_two(template->height0) &&
               util_is_power_of_two(template->depth0));

   if (spt->base.tex_usage & (PIPE_TEXTURE_USAGE_DISPLAY_TARGET |
                              PIPE_TEXTURE_USAGE_PRIMARY)) {
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


/**
 * Create a new pipe_texture which wraps an existing buffer.
 */
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
       base->depth0 != 1) {
      return NULL;
   }

   spt = CALLOC_STRUCT(softpipe_texture);
   if (!spt)
      return NULL;

   spt->base = *base;
   pipe_reference_init(&spt->base.reference, 1);
   spt->base.screen = screen;
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


/**
 * Get a pipe_surface "view" into a texture.
 */
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
      ps->width = u_minify(pt->width0, level);
      ps->height = u_minify(pt->height0, level);
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
         spt->timestamp++;
         softpipe_screen(screen)->timestamp++;
      }

      ps->face = face;
      ps->level = level;
      ps->zslice = zslice;

      if (pt->target == PIPE_TEXTURE_CUBE) {
         ps->offset += face * util_format_get_nblocksy(pt->format, u_minify(pt->height0, level)) *
                       spt->stride[level];
      }
      else if (pt->target == PIPE_TEXTURE_3D) {
         ps->offset += zslice * util_format_get_nblocksy(pt->format, u_minify(pt->height0, level)) *
                       spt->stride[level];
      }
      else {
         assert(face == 0);
         assert(zslice == 0);
      }
   }
   return ps;
}


/**
 * Free a pipe_surface which was created with softpipe_get_tex_surface().
 */
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


/**
 * Geta pipe_transfer object which is used for moving data in/out of
 * a texture object.
 * \param face  one of PIPE_TEX_FACE_x or 0
 * \param level  texture mipmap level
 * \param zslice  2D slice of a 3D texture
 * \param usage  one of PIPE_TRANSFER_READ/WRITE/READ_WRITE
 * \param x  X position of region to read/write
 * \param y  Y position of region to read/write
 * \param width  width of region to read/write
 * \param height  height of region to read/write
 */
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

   /* make sure the requested region is in the image bounds */
   assert(x + w <= u_minify(texture->width0, level));
   assert(y + h <= u_minify(texture->height0, level));

   spt = CALLOC_STRUCT(softpipe_transfer);
   if (spt) {
      struct pipe_transfer *pt = &spt->base;
      int nblocksy = util_format_get_nblocksy(texture->format, u_minify(texture->height0, level));
      pipe_texture_reference(&pt->texture, texture);
      pt->x = x;
      pt->y = y;
      pt->width = w;
      pt->height = h;
      pt->stride = sptex->stride[level];
      pt->usage = usage;
      pt->face = face;
      pt->level = level;
      pt->zslice = zslice;

      spt->offset = sptex->level_offset[level];

      if (texture->target == PIPE_TEXTURE_CUBE) {
         spt->offset += face * nblocksy * pt->stride;
      }
      else if (texture->target == PIPE_TEXTURE_3D) {
         spt->offset += zslice * nblocksy * pt->stride;
      }
      else {
         assert(face == 0);
         assert(zslice == 0);
      }
      return pt;
   }
   return NULL;
}


/**
 * Free a pipe_transfer object which was created with
 * softpipe_get_tex_transfer().
 */
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


/**
 * Create memory mapping for given pipe_transfer object.
 */
static void *
softpipe_transfer_map( struct pipe_screen *screen,
                       struct pipe_transfer *transfer )
{
   ubyte *map, *xfer_map;
   struct softpipe_texture *spt;
   enum pipe_format format;

   assert(transfer->texture);
   spt = softpipe_texture(transfer->texture);
   format = transfer->texture->format;

   map = pipe_buffer_map(screen, spt->buffer, pipe_transfer_buffer_flags(transfer));
   if (map == NULL)
      return NULL;

   /* May want to different things here depending on read/write nature
    * of the map:
    */
   if (transfer->texture && (transfer->usage & PIPE_TRANSFER_WRITE)) {
      /* Do something to notify sharing contexts of a texture change.
       * In softpipe, that would mean flushing the texture cache.
       */
      softpipe_screen(screen)->timestamp++;
   }

   xfer_map = map + softpipe_transfer(transfer)->offset +
      transfer->y / util_format_get_blockheight(format) * transfer->stride +
      transfer->x / util_format_get_blockwidth(format) * util_format_get_blocksize(format);
   /*printf("map = %p  xfer map = %p\n", map, xfer_map);*/
   return xfer_map;
}


/**
 * Unmap memory mapping for given pipe_transfer object.
 */
static void
softpipe_transfer_unmap(struct pipe_screen *screen,
                        struct pipe_transfer *transfer)
{
   struct softpipe_texture *spt;

   assert(transfer->texture);
   spt = softpipe_texture(transfer->texture);

   pipe_buffer_unmap( screen, spt->buffer );

   if (transfer->usage & PIPE_TRANSFER_WRITE) {
      /* Mark the texture as dirty to expire the tile caches. */
      spt->timestamp++;
   }
}


static struct pipe_video_surface*
softpipe_video_surface_create(struct pipe_screen *screen,
                              enum pipe_video_chroma_format chroma_format,
                              unsigned width, unsigned height)
{
   struct softpipe_video_surface *sp_vsfc;
   struct pipe_texture template;

   assert(screen);
   assert(width && height);

   sp_vsfc = CALLOC_STRUCT(softpipe_video_surface);
   if (!sp_vsfc)
      return NULL;

   pipe_reference_init(&sp_vsfc->base.reference, 1);
   sp_vsfc->base.screen = screen;
   sp_vsfc->base.chroma_format = chroma_format;
   /*sp_vsfc->base.surface_format = PIPE_VIDEO_SURFACE_FORMAT_VUYA;*/
   sp_vsfc->base.width = width;
   sp_vsfc->base.height = height;

   memset(&template, 0, sizeof(struct pipe_texture));
   template.target = PIPE_TEXTURE_2D;
   template.format = PIPE_FORMAT_B8G8R8X8_UNORM;
   template.last_level = 0;
   /* vl_mpeg12_mc_renderer expects this when it's initialized with pot_buffers=true */
   template.width0 = util_next_power_of_two(width);
   template.height0 = util_next_power_of_two(height);
   template.depth0 = 1;
   template.tex_usage = PIPE_TEXTURE_USAGE_SAMPLER | PIPE_TEXTURE_USAGE_RENDER_TARGET;

   sp_vsfc->tex = screen->texture_create(screen, &template);
   if (!sp_vsfc->tex) {
      FREE(sp_vsfc);
      return NULL;
   }

   return &sp_vsfc->base;
}


static void
softpipe_video_surface_destroy(struct pipe_video_surface *vsfc)
{
   struct softpipe_video_surface *sp_vsfc = softpipe_video_surface(vsfc);

   pipe_texture_reference(&sp_vsfc->tex, NULL);
   FREE(sp_vsfc);
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

   screen->video_surface_create = softpipe_video_surface_create;
   screen->video_surface_destroy = softpipe_video_surface_destroy;
}


/**
 * Return pipe_buffer handle and stride for given texture object.
 * XXX used for???
 */
boolean
softpipe_get_texture_buffer( struct pipe_texture *texture,
                             struct pipe_buffer **buf,
                             unsigned *stride )
{
   struct softpipe_texture *tex = (struct softpipe_texture *) texture;

   if (!tex)
      return FALSE;

   pipe_buffer_reference(buf, tex->buffer);

   if (stride)
      *stride = tex->stride[0];

   return TRUE;
}
