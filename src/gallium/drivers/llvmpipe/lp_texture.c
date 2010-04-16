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

#include <stdio.h>

#include "pipe/p_context.h"
#include "pipe/p_defines.h"

#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_transfer.h"

#include "lp_context.h"
#include "lp_flush.h"
#include "lp_screen.h"
#include "lp_tile_image.h"
#include "lp_texture.h"
#include "lp_setup.h"
#include "lp_tile_size.h"

#include "state_tracker/sw_winsys.h"


static INLINE boolean
resource_is_texture(const struct pipe_resource *resource)
{
   const unsigned tex_binds = (PIPE_BIND_DISPLAY_TARGET |
                               PIPE_BIND_SCANOUT |
                               PIPE_BIND_SHARED |
                               PIPE_BIND_DEPTH_STENCIL |
                               PIPE_BIND_SAMPLER_VIEW);
   const struct llvmpipe_resource *lpr = llvmpipe_resource_const(resource);

   return (lpr->base.bind & tex_binds) ? TRUE : FALSE;
}



/**
 * Allocate storage for llvmpipe_texture::layout array.
 * The number of elements is width_in_tiles * height_in_tiles.
 */
static enum lp_texture_layout *
alloc_layout_array(unsigned width, unsigned height)
{
   const unsigned tx = align(width, TILE_SIZE) / TILE_SIZE;
   const unsigned ty = align(height, TILE_SIZE) / TILE_SIZE;

   assert(tx * ty > 0);
   assert(LP_TEX_LAYOUT_NONE == 0); /* calloc'ing LP_TEX_LAYOUT_NONE here */

   return (enum lp_texture_layout *)
      calloc(tx * ty, sizeof(enum lp_texture_layout));
}



/**
 * Conventional allocation path for non-display textures:
 * Just compute row strides here.  Storage is allocated on demand later.
 */
static boolean
llvmpipe_texture_layout(struct llvmpipe_screen *screen,
                        struct llvmpipe_resource *lpr)
{
   struct pipe_resource *pt = &lpr->base;
   unsigned level;
   unsigned width = pt->width0;
   unsigned height = pt->height0;

   assert(LP_MAX_TEXTURE_2D_LEVELS <= LP_MAX_TEXTURE_LEVELS);
   assert(LP_MAX_TEXTURE_3D_LEVELS <= LP_MAX_TEXTURE_LEVELS);

   for (level = 0; level <= pt->last_level; level++) {
      const unsigned num_faces = lpr->base.target == PIPE_TEXTURE_CUBE ? 6 : 1;
      unsigned nblocksx, face;

      /* Allocate storage for whole quads. This is particularly important
       * for depth surfaces, which are currently stored in a swizzled format.
       */
      nblocksx = util_format_get_nblocksx(pt->format, align(width, TILE_SIZE));

      lpr->stride[level] =
         align(nblocksx * util_format_get_blocksize(pt->format), 16);

      lpr->tiles_per_row[level] = align(width, TILE_SIZE) / TILE_SIZE;

      for (face = 0; face < num_faces; face++) {
         lpr->layout[face][level] = alloc_layout_array(width, height);
      }

      width = u_minify(width, 1);
      height = u_minify(height, 1);
   }

   return TRUE;
}



static boolean
llvmpipe_displaytarget_layout(struct llvmpipe_screen *screen,
                              struct llvmpipe_resource *lpr)
{
   struct sw_winsys *winsys = screen->winsys;

   /* Round up the surface size to a multiple of the tile size to
    * avoid tile clipping.
    */
   unsigned width = align(lpr->base.width0, TILE_SIZE);
   unsigned height = align(lpr->base.height0, TILE_SIZE);

   lpr->tiles_per_row[0] = align(width, TILE_SIZE) / TILE_SIZE;

   lpr->layout[0][0] = alloc_layout_array(width, height);

   lpr->dt = winsys->displaytarget_create(winsys,
                                          lpr->base.bind,
                                          lpr->base.format,
                                          width, height,
                                          16,
                                          &lpr->stride[0] );

   return lpr->dt != NULL;
}


static struct pipe_resource *
llvmpipe_resource_create(struct pipe_screen *_screen,
                         const struct pipe_resource *templat)
{
   static unsigned id_counter = 0;
   struct llvmpipe_screen *screen = llvmpipe_screen(_screen);
   struct llvmpipe_resource *lpr = CALLOC_STRUCT(llvmpipe_resource);
   if (!lpr)
      return NULL;

   lpr->base = *templat;
   pipe_reference_init(&lpr->base.reference, 1);
   lpr->base.screen = &screen->base;

   assert(lpr->base.bind);

   if (lpr->base.bind & (PIPE_BIND_DISPLAY_TARGET |
                         PIPE_BIND_SCANOUT |
                         PIPE_BIND_SHARED)) {
      /* displayable surface */
      if (!llvmpipe_displaytarget_layout(screen, lpr))
         goto fail;
      assert(lpr->layout[0][0][0] == LP_TEX_LAYOUT_NONE);
   }
   else if (lpr->base.bind & (PIPE_BIND_SAMPLER_VIEW |
                              PIPE_BIND_DEPTH_STENCIL)) {
      /* texture map */
      if (!llvmpipe_texture_layout(screen, lpr))
         goto fail;
      assert(lpr->layout[0][0][0] == LP_TEX_LAYOUT_NONE);
   }
   else {
      /* other data (vertex buffer, const buffer, etc) */
      const enum pipe_format format = templat->format;
      const uint w = templat->width0 / util_format_get_blockheight(format);
      const uint h = templat->height0 / util_format_get_blockwidth(format);
      const uint d = templat->depth0;
      const uint bpp = util_format_get_blocksize(format);
      const uint bytes = w * h * d * bpp;
      lpr->data = align_malloc(bytes, 16);
      if (!lpr->data)
         goto fail;
   }

   if (resource_is_texture(&lpr->base)) {
      assert(lpr->layout[0][0]);
   }

   lpr->id = id_counter++;

   return &lpr->base;

 fail:
   FREE(lpr);
   return NULL;
}


static void
llvmpipe_resource_destroy(struct pipe_screen *pscreen,
			  struct pipe_resource *pt)
{
   struct llvmpipe_screen *screen = llvmpipe_screen(pscreen);
   struct llvmpipe_resource *lpr = llvmpipe_resource(pt);

   if (lpr->dt) {
      /* display target */
      struct sw_winsys *winsys = screen->winsys;
      winsys->displaytarget_destroy(winsys, lpr->dt);
   }
   else if (resource_is_texture(pt)) {
      /* regular texture */
      const uint num_faces = pt->target == PIPE_TEXTURE_CUBE ? 6 : 1;
      uint level, face;

      /* free linear image data */
      for (level = 0; level < Elements(lpr->linear); level++) {
         if (lpr->linear[level].data) {
            align_free(lpr->linear[level].data);
            lpr->linear[level].data = NULL;
         }
      }

      /* free tiled image data */
      for (level = 0; level < Elements(lpr->tiled); level++) {
         if (lpr->tiled[level].data) {
            align_free(lpr->tiled[level].data);
            lpr->tiled[level].data = NULL;
         }
      }

      /* free layout flag arrays */
      for (level = 0; level < Elements(lpr->tiled); level++) {
         for (face = 0; face < num_faces; face++) {
            free(lpr->layout[face][level]);
            lpr->layout[face][level] = NULL;
         }
      }
   }
   else if (!lpr->userBuffer) {
      assert(lpr->data);
      align_free(lpr->data);
   }

   FREE(lpr);
}


/**
 * Map a resource for read/write.
 */
void *
llvmpipe_resource_map(struct pipe_resource *resource,
		      unsigned face,
		      unsigned level,
		      unsigned zslice,
                      enum lp_texture_usage tex_usage,
                      enum lp_texture_layout layout)
{
   struct llvmpipe_resource *lpr = llvmpipe_resource(resource);
   uint8_t *map;

   assert(face < 6);
   assert(level < LP_MAX_TEXTURE_LEVELS);

   assert(tex_usage == LP_TEX_USAGE_READ ||
          tex_usage == LP_TEX_USAGE_READ_WRITE ||
          tex_usage == LP_TEX_USAGE_WRITE_ALL);

   assert(layout == LP_TEX_LAYOUT_NONE ||
          layout == LP_TEX_LAYOUT_TILED ||
          layout == LP_TEX_LAYOUT_LINEAR);

   if (lpr->dt) {
      /* display target */
      struct llvmpipe_screen *screen = llvmpipe_screen(resource->screen);
      struct sw_winsys *winsys = screen->winsys;
      unsigned dt_usage;

      if (tex_usage == LP_TEX_USAGE_READ) {
         dt_usage = PIPE_TRANSFER_READ;
      }
      else {
         dt_usage = PIPE_TRANSFER_READ_WRITE;
      }

      assert(face == 0);
      assert(level == 0);
      assert(zslice == 0);

      /* FIXME: keep map count? */
      map = winsys->displaytarget_map(winsys, lpr->dt, dt_usage);

      /* install this linear image in texture data structure */
      lpr->linear[level].data = map;

      map = llvmpipe_get_texture_image(lpr, face, level, tex_usage, layout);
      assert(map);

      return map;
   }
   else if (resource_is_texture(resource)) {
      /* regular texture */
      const unsigned tex_height = u_minify(resource->height0, level);
      const unsigned nblocksy =
         util_format_get_nblocksy(resource->format, tex_height);
      const unsigned stride = lpr->stride[level];
      unsigned offset = 0;

      if (resource->target == PIPE_TEXTURE_CUBE) {
         /* XXX incorrect
         offset = face * nblocksy * stride;
         */
      }
      else if (resource->target == PIPE_TEXTURE_3D) {
         offset = zslice * nblocksy * stride;
      }
      else {
         assert(face == 0);
         assert(zslice == 0);
         offset = 0;
      }

      map = llvmpipe_get_texture_image(lpr, face, level, tex_usage, layout);
      assert(map);
      map += offset;
      return map;
   }
   else {
      return lpr->data;
   }
}


/**
 * Unmap a resource.
 */
void
llvmpipe_resource_unmap(struct pipe_resource *resource,
                       unsigned face,
                       unsigned level,
                       unsigned zslice)
{
   struct llvmpipe_resource *lpr = llvmpipe_resource(resource);

   if (lpr->dt) {
      /* display target */
      struct llvmpipe_screen *lp_screen = llvmpipe_screen(resource->screen);
      struct sw_winsys *winsys = lp_screen->winsys;

      assert(face == 0);
      assert(level == 0);
      assert(zslice == 0);

      /* make sure linear image is up to date */
      (void) llvmpipe_get_texture_image(lpr, 0, 0,
                                        LP_TEX_USAGE_READ,
                                        LP_TEX_LAYOUT_LINEAR);

      winsys->displaytarget_unmap(winsys, lpr->dt);
   }
}


void *
llvmpipe_resource_data(struct pipe_resource *resource)
{
   struct llvmpipe_resource *lpr = llvmpipe_resource(resource);

   assert((lpr->base.bind & (PIPE_BIND_DISPLAY_TARGET |
                             PIPE_BIND_SCANOUT |
                             PIPE_BIND_SHARED |
                             PIPE_BIND_SAMPLER_VIEW)) == 0);

   return lpr->data;
}


static struct pipe_resource *
llvmpipe_resource_from_handle(struct pipe_screen *screen,
			      const struct pipe_resource *template,
			      struct winsys_handle *whandle)
{
   struct sw_winsys *winsys = llvmpipe_screen(screen)->winsys;
   struct llvmpipe_resource *lpr = CALLOC_STRUCT(llvmpipe_resource);
   if (!lpr)
      return NULL;

   lpr->base = *template;
   pipe_reference_init(&lpr->base.reference, 1);
   lpr->base.screen = screen;

   lpr->dt = winsys->displaytarget_from_handle(winsys,
                                               template,
                                               whandle,
                                               &lpr->stride[0]);
   if (!lpr->dt)
      goto fail;

   return &lpr->base;

 fail:
   FREE(lpr);
   return NULL;
}


static boolean
llvmpipe_resource_get_handle(struct pipe_screen *screen,
                            struct pipe_resource *pt,
                            struct winsys_handle *whandle)
{
   struct sw_winsys *winsys = llvmpipe_screen(screen)->winsys;
   struct llvmpipe_resource *lpr = llvmpipe_resource(pt);

   assert(lpr->dt);
   if (!lpr->dt)
      return FALSE;

   return winsys->displaytarget_get_handle(winsys, lpr->dt, whandle);
}


static struct pipe_surface *
llvmpipe_get_tex_surface(struct pipe_screen *screen,
                         struct pipe_resource *pt,
                         unsigned face, unsigned level, unsigned zslice,
                         enum lp_texture_usage usage)
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
   struct llvmpipe_resource *lprex = llvmpipe_resource(resource);
   struct llvmpipe_transfer *lpr;

   assert(resource);
   assert(sr.level <= resource->last_level);

   lpr = CALLOC_STRUCT(llvmpipe_transfer);
   if (lpr) {
      struct pipe_transfer *pt = &lpr->base;
      pipe_resource_reference(&pt->resource, resource);
      pt->box = *box;
      pt->sr = sr;
      pt->stride = lprex->stride[sr.level];
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
   struct llvmpipe_resource *lpr;
   enum pipe_format format;
   enum lp_texture_usage tex_usage;
   const char *mode;

   assert(transfer->sr.face < 6);
   assert(transfer->sr.level < LP_MAX_TEXTURE_LEVELS);

   /*
   printf("tex_transfer_map(%d, %d  %d x %d of %d x %d,  usage %d )\n",
          transfer->x, transfer->y, transfer->width, transfer->height,
          transfer->texture->width0,
          transfer->texture->height0,
          transfer->usage);
   */

   if (transfer->usage == PIPE_TRANSFER_READ) {
      tex_usage = LP_TEX_USAGE_READ;
      mode = "read";
   }
   else {
      tex_usage = LP_TEX_USAGE_READ_WRITE;
      mode = "read/write";
   }

   if (0) {
      struct llvmpipe_resource *lpr = llvmpipe_resource(transfer->resource);
      printf("transfer map tex %u  mode %s\n", lpr->id, mode);
   }


   assert(transfer->resource);
   lpr = llvmpipe_resource(transfer->resource);
   format = lpr->base.format;

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
			       transfer->sr.face,
			       transfer->sr.level,
			       transfer->box.z,
                               tex_usage, LP_TEX_LAYOUT_LINEAR);


   /* May want to do different things here depending on read/write nature
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


/**
 * Compute size (in bytes) need to store a texture image / mipmap level,
 * for just one cube face.
 */
static unsigned
tex_image_face_size(const struct llvmpipe_resource *lpr, unsigned level,
                    enum lp_texture_layout layout)
{
   /* for tiled layout, force a 32bpp format */
   enum pipe_format format = layout == LP_TEX_LAYOUT_TILED
      ? PIPE_FORMAT_B8G8R8A8_UNORM : lpr->base.format;
   const unsigned height = u_minify(lpr->base.height0, level);
   const unsigned depth = u_minify(lpr->base.depth0, level);
   const unsigned nblocksy =
      util_format_get_nblocksy(format, align(height, TILE_SIZE));
   const unsigned buffer_size =
      nblocksy * lpr->stride[level] *
      (lpr->base.target == PIPE_TEXTURE_3D ? depth : 1);
   return buffer_size;
}


/**
 * Compute size (in bytes) need to store a texture image / mipmap level,
 * including all cube faces.
 */
static unsigned
tex_image_size(const struct llvmpipe_resource *lpr, unsigned level,
               enum lp_texture_layout layout)
{
   const unsigned buf_size = tex_image_face_size(lpr, level, layout);
   const unsigned num_faces = lpr->base.target == PIPE_TEXTURE_CUBE ? 6 : 1;
   return buf_size * num_faces;
}


/**
 * This function encapsulates some complicated logic for determining
 * how to convert a tile of image data from linear layout to tiled
 * layout, or vice versa.
 * \param cur_layout  the current tile layout
 * \param target_layout  the desired tile layout
 * \param usage  how the tile will be accessed (R/W vs. read-only, etc)
 * \param new_layout_return  returns the new layout mode
 * \param convert_return  returns TRUE if image conversion is needed
 */
static void
layout_logic(enum lp_texture_layout cur_layout,
             enum lp_texture_layout target_layout,
             enum lp_texture_usage usage,
             enum lp_texture_layout *new_layout_return,
             boolean *convert)
{
   enum lp_texture_layout other_layout, new_layout;

   *convert = FALSE;

   new_layout = 99; /* debug check */

   if (target_layout == LP_TEX_LAYOUT_LINEAR) {
      other_layout = LP_TEX_LAYOUT_TILED;
   }
   else {
      assert(target_layout == LP_TEX_LAYOUT_TILED);
      other_layout = LP_TEX_LAYOUT_LINEAR;
   }

   new_layout = target_layout;  /* may get changed below */

   if (cur_layout == LP_TEX_LAYOUT_BOTH) {
      if (usage == LP_TEX_USAGE_READ) {
         new_layout = LP_TEX_LAYOUT_BOTH;
      }
   }
   else if (cur_layout == other_layout) {
      if (usage != LP_TEX_USAGE_WRITE_ALL) {
         /* need to convert tiled data to linear or vice versa */
         *convert = TRUE;

         if (usage == LP_TEX_USAGE_READ)
            new_layout = LP_TEX_LAYOUT_BOTH;
      }
   }
   else {
      assert(cur_layout == LP_TEX_LAYOUT_NONE ||
             cur_layout == target_layout);
   }

   assert(new_layout == LP_TEX_LAYOUT_BOTH ||
          new_layout == target_layout);

   *new_layout_return = new_layout;
}


/**
 * Return pointer to a texture image.  No tiled/linear conversion is done.
 */
void *
llvmpipe_get_texture_image_address(struct llvmpipe_resource *lpr,
                                   unsigned face, unsigned level,
                                   enum lp_texture_layout layout)
{
   struct llvmpipe_texture_image *img;
   unsigned face_offset;

   if (layout == LP_TEX_LAYOUT_LINEAR) {
      img = &lpr->linear[level];
   }
   else {
      assert (layout == LP_TEX_LAYOUT_TILED);
      img = &lpr->tiled[level];
   }

   if (face > 0)
      face_offset = face * tex_image_face_size(lpr, level, layout);
   else
      face_offset = 0;

   return (ubyte *) img->data + face_offset;
}



/**
 * Return pointer to texture image data (either linear or tiled layout).
 * \param usage  one of LP_TEX_USAGE_READ/WRITE_ALL/READ_WRITE
 * \param layout  either LP_TEX_LAYOUT_LINEAR or LP_TEX_LAYOUT_TILED
 */
void *
llvmpipe_get_texture_image(struct llvmpipe_resource *lpr,
                           unsigned face, unsigned level,
                           enum lp_texture_usage usage,
                           enum lp_texture_layout layout)
{
   /*
    * 'target' refers to the image which we're retrieving (either in
    * tiled or linear layout).
    * 'other' refers to the same image but in the other layout. (it may
    *  or may not exist.
    */
   struct llvmpipe_texture_image *target_img;
   struct llvmpipe_texture_image *other_img;
   void *target_data;
   void *other_data;
   const unsigned width = u_minify(lpr->base.width0, level);
   const unsigned height = u_minify(lpr->base.height0, level);
   const unsigned width_t = align(width, TILE_SIZE) / TILE_SIZE;
   const unsigned height_t = align(height, TILE_SIZE) / TILE_SIZE;
   enum lp_texture_layout other_layout;

   assert(layout == LP_TEX_LAYOUT_NONE ||
          layout == LP_TEX_LAYOUT_TILED ||
          layout == LP_TEX_LAYOUT_LINEAR);

   assert(usage == LP_TEX_USAGE_READ ||
          usage == LP_TEX_USAGE_READ_WRITE ||
          usage == LP_TEX_USAGE_WRITE_ALL);

   if (lpr->dt) {
      assert(lpr->linear[level].data);
   }

   /* which is target?  which is other? */
   if (layout == LP_TEX_LAYOUT_LINEAR) {
      target_img = &lpr->linear[level];
      other_img = &lpr->tiled[level];
      other_layout = LP_TEX_LAYOUT_TILED;
   }
   else {
      target_img = &lpr->tiled[level];
      other_img = &lpr->linear[level];
      other_layout = LP_TEX_LAYOUT_LINEAR;
   }

   target_data = target_img->data;
   other_data = other_img->data;

   if (!target_data) {
      /* allocate memory for the target image now */
      unsigned buffer_size = tex_image_size(lpr, level, layout);
      target_img->data = align_malloc(buffer_size, 16);
      target_data = target_img->data;
   }

   if (face > 0) {
      unsigned offset = face * tex_image_face_size(lpr, level, layout);
      if (target_data) {
         target_data = (uint8_t *) target_data + offset;
      }
      if (other_data) {
         other_data = (uint8_t *) other_data + offset;
      }
   }

   if (layout == LP_TEX_LAYOUT_NONE) {
      /* just allocating memory */
      return target_data;
   }

   if (other_data) {
      /* may need to convert other data to the requested layout */
      enum lp_texture_layout new_layout;
      unsigned x, y, i = 0;

      /* loop over all image tiles, doing layout conversion where needed */
      for (y = 0; y < height_t; y++) {
         for (x = 0; x < width_t; x++) {
            enum lp_texture_layout cur_layout = lpr->layout[face][level][i];
            boolean convert;

            layout_logic(cur_layout, layout, usage, &new_layout, &convert);

            if (convert) {
               if (layout == LP_TEX_LAYOUT_TILED) {
                  lp_linear_to_tiled(other_data, target_data,
                                     x * TILE_SIZE, y * TILE_SIZE,
                                     TILE_SIZE, TILE_SIZE,
                                     lpr->base.format,
                                     lpr->stride[level]);
               }
               else {
                  lp_tiled_to_linear(other_data, target_data,
                                     x * TILE_SIZE, y * TILE_SIZE,
                                     TILE_SIZE, TILE_SIZE,
                                     lpr->base.format,
                                     lpr->stride[level]);
               }
            }

            lpr->layout[face][level][i] = new_layout;
            i++;
         }
      }
   }
   else {
      /* no other data */
      unsigned i;
      for (i = 0; i < width_t * height_t; i++) {
         lpr->layout[face][level][i] = layout;
      }
   }

   assert(target_data);

   return target_data;
}


static INLINE enum lp_texture_layout
llvmpipe_get_texture_tile_layout(const struct llvmpipe_resource *lpr,
                                 unsigned face, unsigned level,
                                 unsigned x, unsigned y)
{
   uint i;
   assert(resource_is_texture(&lpr->base));
   assert(x < lpr->tiles_per_row[level]);
   i = y * lpr->tiles_per_row[level] + x;
   return lpr->layout[face][level][i];
}


static INLINE void
llvmpipe_set_texture_tile_layout(struct llvmpipe_resource *lpr,
                                 unsigned face, unsigned level,
                                 unsigned x, unsigned y,
                                 enum lp_texture_layout layout)
{
   uint i;
   assert(resource_is_texture(&lpr->base));
   assert(x < lpr->tiles_per_row[level]);
   i = y * lpr->tiles_per_row[level] + x;
   lpr->layout[face][level][i] = layout;
}


/**
 * Get pointer to a linear image where the tile at (x,y) is known to be
 * in linear layout.
 * Conversion from tiled to linear will be done if necessary.
 * \return pointer to start of image/face (not the tile)
 */
ubyte *
llvmpipe_get_texture_tile_linear(struct llvmpipe_resource *lpr,
                                 unsigned face, unsigned level,
                                 enum lp_texture_usage usage,
                                 unsigned x, unsigned y)
{
   struct llvmpipe_texture_image *tiled_img = &lpr->tiled[level];
   struct llvmpipe_texture_image *linear_img = &lpr->linear[level];
   enum lp_texture_layout cur_layout, new_layout;
   const unsigned tx = x / TILE_SIZE, ty = y / TILE_SIZE;
   boolean convert;

   assert(resource_is_texture(&lpr->base));
   assert(x % TILE_SIZE == 0);
   assert(y % TILE_SIZE == 0);

   if (!linear_img->data) {
      /* allocate memory for the tiled image now */
      unsigned buffer_size = tex_image_size(lpr, level, LP_TEX_LAYOUT_LINEAR);
      linear_img->data = align_malloc(buffer_size, 16);
   }

   cur_layout = llvmpipe_get_texture_tile_layout(lpr, face, level, tx, ty);

   layout_logic(cur_layout, LP_TEX_LAYOUT_LINEAR, usage,
                &new_layout, &convert);

   if (convert) {
      lp_tiled_to_linear(tiled_img->data, linear_img->data,
                         x, y, TILE_SIZE, TILE_SIZE, lpr->base.format,
                         lpr->stride[level]);
   }

   if (new_layout != cur_layout)
      llvmpipe_set_texture_tile_layout(lpr, face, level, tx, ty, new_layout);

   if (face > 0) {
      unsigned offset
         = face * tex_image_face_size(lpr, level, LP_TEX_LAYOUT_LINEAR);
      return (ubyte *) linear_img->data + offset;
   }
   else {
      return linear_img->data;
   }
}


/**
 * Get pointer to tiled data for rendering.
 * \return pointer to the tiled data at the given tile position
 */
ubyte *
llvmpipe_get_texture_tile(struct llvmpipe_resource *lpr,
                          unsigned face, unsigned level,
                          enum lp_texture_usage usage,
                          unsigned x, unsigned y)
{
   const unsigned width = u_minify(lpr->base.width0, level);
   struct llvmpipe_texture_image *tiled_img = &lpr->tiled[level];
   struct llvmpipe_texture_image *linear_img = &lpr->linear[level];
   enum lp_texture_layout cur_layout, new_layout;
   const unsigned tx = x / TILE_SIZE, ty = y / TILE_SIZE;
   boolean convert;

   assert(x % TILE_SIZE == 0);
   assert(y % TILE_SIZE == 0);

   if (!tiled_img->data) {
      /* allocate memory for the tiled image now */
      unsigned buffer_size = tex_image_size(lpr, level, LP_TEX_LAYOUT_TILED);
      tiled_img->data = align_malloc(buffer_size, 16);
   }

   cur_layout = llvmpipe_get_texture_tile_layout(lpr, face, level, tx, ty);

   layout_logic(cur_layout, LP_TEX_LAYOUT_TILED, usage, &new_layout, &convert);
   if (convert) {
      lp_linear_to_tiled(linear_img->data, tiled_img->data,
                         x, y, TILE_SIZE, TILE_SIZE, lpr->base.format,
                         lpr->stride[level]);
   }

   if (new_layout != cur_layout)
      llvmpipe_set_texture_tile_layout(lpr, face, level, tx, ty, new_layout);

   /* compute, return address of the 64x64 tile */
   {
      unsigned tiles_per_row, tile_offset, face_offset;

      tiles_per_row = align(width, TILE_SIZE) / TILE_SIZE;

      assert(tiles_per_row == lpr->tiles_per_row[level]);

      tile_offset = ty * tiles_per_row + tx;
      tile_offset *= TILE_SIZE * TILE_SIZE * 4;

      assert(tiled_img->data);

      face_offset = (face > 0)
         ? (face * tex_image_face_size(lpr, level, LP_TEX_LAYOUT_TILED))
         : 0;

      return (ubyte *) tiled_img->data + face_offset + tile_offset;
   }
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
