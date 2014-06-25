/**************************************************************************
 * 
 * Copyright 2006 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keithw@vmware.com>
  *   Michel Dänzer <daenzer@vmware.com>
  */

#include <stdio.h>

#include "pipe/p_context.h"
#include "pipe/p_defines.h"

#include "util/u_inlines.h"
#include "util/u_cpu_detect.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_simple_list.h"
#include "util/u_transfer.h"

#include "lp_context.h"
#include "lp_flush.h"
#include "lp_screen.h"
#include "lp_texture.h"
#include "lp_setup.h"
#include "lp_state.h"
#include "lp_rast.h"

#include "state_tracker/sw_winsys.h"


#ifdef DEBUG
static struct llvmpipe_resource resource_list;
#endif
static unsigned id_counter = 0;


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
   unsigned depth = pt->depth0;
   uint64_t total_size = 0;
   unsigned layers = pt->array_size;

   assert(LP_MAX_TEXTURE_2D_LEVELS <= LP_MAX_TEXTURE_LEVELS);
   assert(LP_MAX_TEXTURE_3D_LEVELS <= LP_MAX_TEXTURE_LEVELS);

   for (level = 0; level <= pt->last_level; level++) {

      /* Row stride and image stride */
      {
         unsigned align_x, align_y, nblocksx, nblocksy, block_size;

         /* For non-compressed formats we need 4x4 pixel alignment
          * so we can read/write LP_RASTER_BLOCK_SIZE when rendering to them.
          * We also want cache line size in x direction,
          * otherwise same cache line could end up in multiple threads.
          * For explicit 1d resources however we reduce this to 4x1 and
          * handle specially in render output code (as we need to do special
          * handling there for buffers in any case).
          */
         if (util_format_is_compressed(pt->format))
            align_x = align_y = 1;
         else {
            align_x = LP_RASTER_BLOCK_SIZE;
            if (llvmpipe_resource_is_1d(&lpr->base))
               align_y = 1;
            else
               align_y = LP_RASTER_BLOCK_SIZE;
         }

         nblocksx = util_format_get_nblocksx(pt->format,
                                             align(width, align_x));
         nblocksy = util_format_get_nblocksy(pt->format,
                                             align(height, align_y));
         block_size = util_format_get_blocksize(pt->format);

         if (util_format_is_compressed(pt->format))
            lpr->row_stride[level] = nblocksx * block_size;
         else
            lpr->row_stride[level] = align(nblocksx * block_size, util_cpu_caps.cacheline);

         /* if row_stride * height > LP_MAX_TEXTURE_SIZE */
         if ((uint64_t)lpr->row_stride[level] * nblocksy > LP_MAX_TEXTURE_SIZE) {
            /* image too large */
            goto fail;
         }

         lpr->img_stride[level] = lpr->row_stride[level] * nblocksy;
      }

      /* Number of 3D image slices, cube faces or texture array layers */
      {
         unsigned num_slices;

         if (lpr->base.target == PIPE_TEXTURE_CUBE)
            num_slices = 6;
         else if (lpr->base.target == PIPE_TEXTURE_3D)
            num_slices = depth;
         else if (lpr->base.target == PIPE_TEXTURE_1D_ARRAY ||
                  lpr->base.target == PIPE_TEXTURE_2D_ARRAY)
            num_slices = layers;
         else
            num_slices = 1;

         lpr->num_slices_faces[level] = num_slices;
      }

      /* if img_stride * num_slices_faces > LP_MAX_TEXTURE_SIZE */
      if (lpr->img_stride[level] >
          LP_MAX_TEXTURE_SIZE / lpr->num_slices_faces[level]) {
         /* volume too large */
         goto fail;
      }

      total_size += (uint64_t) lpr->num_slices_faces[level]
                  * (uint64_t) lpr->img_stride[level];
      if (total_size > LP_MAX_TEXTURE_SIZE) {
         goto fail;
      }

      /* Compute size of next mipmap level */
      width = u_minify(width, 1);
      height = u_minify(height, 1);
      depth = u_minify(depth, 1);
   }

   return TRUE;

fail:
   return FALSE;
}


/**
 * Check the size of the texture specified by 'res'.
 * \return TRUE if OK, FALSE if too large.
 */
static boolean
llvmpipe_can_create_resource(struct pipe_screen *screen,
                             const struct pipe_resource *res)
{
   struct llvmpipe_resource lpr;
   memset(&lpr, 0, sizeof(lpr));
   lpr.base = *res;
   return llvmpipe_texture_layout(llvmpipe_screen(screen), &lpr);
}


static boolean
llvmpipe_displaytarget_layout(struct llvmpipe_screen *screen,
                              struct llvmpipe_resource *lpr)
{
   struct sw_winsys *winsys = screen->winsys;

   /* Round up the surface size to a multiple of the tile size to
    * avoid tile clipping.
    */
   const unsigned width = MAX2(1, align(lpr->base.width0, TILE_SIZE));
   const unsigned height = MAX2(1, align(lpr->base.height0, TILE_SIZE));

   lpr->num_slices_faces[0] = 1;
   lpr->img_stride[0] = 0;

   lpr->dt = winsys->displaytarget_create(winsys,
                                          lpr->base.bind,
                                          lpr->base.format,
                                          width, height,
                                          64,
                                          &lpr->row_stride[0] );

   if (lpr->dt == NULL)
      return FALSE;

   {
      void *map = winsys->displaytarget_map(winsys, lpr->dt,
                                            PIPE_TRANSFER_WRITE);

      if (map)
         memset(map, 0, height * lpr->row_stride[0]);

      winsys->displaytarget_unmap(winsys, lpr->dt);
   }

   return TRUE;
}


static struct pipe_resource *
llvmpipe_resource_create(struct pipe_screen *_screen,
                         const struct pipe_resource *templat)
{
   struct llvmpipe_screen *screen = llvmpipe_screen(_screen);
   struct llvmpipe_resource *lpr = CALLOC_STRUCT(llvmpipe_resource);
   if (!lpr)
      return NULL;

   lpr->base = *templat;
   pipe_reference_init(&lpr->base.reference, 1);
   lpr->base.screen = &screen->base;

   /* assert(lpr->base.bind); */

   if (llvmpipe_resource_is_texture(&lpr->base)) {
      if (lpr->base.bind & (PIPE_BIND_DISPLAY_TARGET |
                            PIPE_BIND_SCANOUT |
                            PIPE_BIND_SHARED)) {
         /* displayable surface */
         if (!llvmpipe_displaytarget_layout(screen, lpr))
            goto fail;
      }
      else {
         /* texture map */
         if (!llvmpipe_texture_layout(screen, lpr))
            goto fail;
      }
   }
   else {
      /* other data (vertex buffer, const buffer, etc) */
      const uint bytes = templat->width0;
      assert(util_format_get_blocksize(templat->format) == 1);
      assert(templat->height0 == 1);
      assert(templat->depth0 == 1);
      assert(templat->last_level == 0);
      /*
       * Reserve some extra storage since if we'd render to a buffer we
       * read/write always LP_RASTER_BLOCK_SIZE pixels, but the element
       * offset doesn't need to be aligned to LP_RASTER_BLOCK_SIZE.
       */
      lpr->data = align_malloc(bytes + (LP_RASTER_BLOCK_SIZE - 1) * 4 * sizeof(float), 64);
      /*
       * buffers don't really have stride but it's probably safer
       * (for code doing same calculations for buffers and textures)
       * to put something sane in there.
       */
      lpr->row_stride[0] = bytes;
      if (!lpr->data)
         goto fail;
      memset(lpr->data, 0, bytes);
   }

   lpr->id = id_counter++;

#ifdef DEBUG
   insert_at_tail(&resource_list, lpr);
#endif

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
   else if (llvmpipe_resource_is_texture(pt)) {
      /* free linear image data */
      if (lpr->linear_img.data) {
         align_free(lpr->linear_img.data);
         lpr->linear_img.data = NULL;
      }
   }
   else if (!lpr->userBuffer) {
      assert(lpr->data);
      align_free(lpr->data);
   }

#ifdef DEBUG
   if (lpr->next)
      remove_from_list(lpr);
#endif

   FREE(lpr);
}


/**
 * Map a resource for read/write.
 */
void *
llvmpipe_resource_map(struct pipe_resource *resource,
                      unsigned level,
                      unsigned layer,
                      enum lp_texture_usage tex_usage)
{
   struct llvmpipe_resource *lpr = llvmpipe_resource(resource);
   uint8_t *map;

   assert(level < LP_MAX_TEXTURE_LEVELS);
   assert(layer < (u_minify(resource->depth0, level) + resource->array_size - 1));

   assert(tex_usage == LP_TEX_USAGE_READ ||
          tex_usage == LP_TEX_USAGE_READ_WRITE ||
          tex_usage == LP_TEX_USAGE_WRITE_ALL);

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

      assert(level == 0);
      assert(layer == 0);

      /* FIXME: keep map count? */
      map = winsys->displaytarget_map(winsys, lpr->dt, dt_usage);

      /* install this linear image in texture data structure */
      lpr->linear_img.data = map;

      return map;
   }
   else if (llvmpipe_resource_is_texture(resource)) {

      map = llvmpipe_get_texture_image(lpr, layer, level, tex_usage);
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
                       unsigned level,
                       unsigned layer)
{
   struct llvmpipe_resource *lpr = llvmpipe_resource(resource);

   if (lpr->dt) {
      /* display target */
      struct llvmpipe_screen *lp_screen = llvmpipe_screen(resource->screen);
      struct sw_winsys *winsys = lp_screen->winsys;

      assert(level == 0);
      assert(layer == 0);

      winsys->displaytarget_unmap(winsys, lpr->dt);
   }
}


void *
llvmpipe_resource_data(struct pipe_resource *resource)
{
   struct llvmpipe_resource *lpr = llvmpipe_resource(resource);

   assert(!llvmpipe_resource_is_texture(resource));

   return lpr->data;
}


static struct pipe_resource *
llvmpipe_resource_from_handle(struct pipe_screen *screen,
                              const struct pipe_resource *template,
                              struct winsys_handle *whandle)
{
   struct sw_winsys *winsys = llvmpipe_screen(screen)->winsys;
   struct llvmpipe_resource *lpr;

   /* XXX Seems like from_handled depth textures doesn't work that well */

   lpr = CALLOC_STRUCT(llvmpipe_resource);
   if (!lpr) {
      goto no_lpr;
   }

   lpr->base = *template;
   pipe_reference_init(&lpr->base.reference, 1);
   lpr->base.screen = screen;

   /*
    * Looks like unaligned displaytargets work just fine,
    * at least sampler/render ones.
    */
#if 0
   assert(lpr->base.width0 == width);
   assert(lpr->base.height0 == height);
#endif

   lpr->num_slices_faces[0] = 1;
   lpr->img_stride[0] = 0;

   lpr->dt = winsys->displaytarget_from_handle(winsys,
                                               template,
                                               whandle,
                                               &lpr->row_stride[0]);
   if (!lpr->dt) {
      goto no_dt;
   }

   lpr->id = id_counter++;

#ifdef DEBUG
   insert_at_tail(&resource_list, lpr);
#endif

   return &lpr->base;

no_dt:
   FREE(lpr);
no_lpr:
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


static void *
llvmpipe_transfer_map( struct pipe_context *pipe,
                       struct pipe_resource *resource,
                       unsigned level,
                       unsigned usage,
                       const struct pipe_box *box,
                       struct pipe_transfer **transfer )
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context(pipe);
   struct llvmpipe_screen *screen = llvmpipe_screen(pipe->screen);
   struct llvmpipe_resource *lpr = llvmpipe_resource(resource);
   struct llvmpipe_transfer *lpt;
   struct pipe_transfer *pt;
   ubyte *map;
   enum pipe_format format;
   enum lp_texture_usage tex_usage;
   const char *mode;

   assert(resource);
   assert(level <= resource->last_level);

   /*
    * Transfers, like other pipe operations, must happen in order, so flush the
    * context if necessary.
    */
   if (!(usage & PIPE_TRANSFER_UNSYNCHRONIZED)) {
      boolean read_only = !(usage & PIPE_TRANSFER_WRITE);
      boolean do_not_block = !!(usage & PIPE_TRANSFER_DONTBLOCK);
      if (!llvmpipe_flush_resource(pipe, resource,
                                   level,
                                   read_only,
                                   TRUE, /* cpu_access */
                                   do_not_block,
                                   __FUNCTION__)) {
         /*
          * It would have blocked, but state tracker requested no to.
          */
         assert(do_not_block);
         return NULL;
      }
   }

   /* Check if we're mapping the current constant buffer */
   if ((usage & PIPE_TRANSFER_WRITE) &&
       (resource->bind & PIPE_BIND_CONSTANT_BUFFER)) {
      unsigned i;
      for (i = 0; i < Elements(llvmpipe->constants[PIPE_SHADER_FRAGMENT]); ++i) {
         if (resource == llvmpipe->constants[PIPE_SHADER_FRAGMENT][i].buffer) {
            /* constants may have changed */
            llvmpipe->dirty |= LP_NEW_CONSTANTS;
            break;
         }
      }
   }

   lpt = CALLOC_STRUCT(llvmpipe_transfer);
   if (!lpt)
      return NULL;
   pt = &lpt->base;
   pipe_resource_reference(&pt->resource, resource);
   pt->box = *box;
   pt->level = level;
   pt->stride = lpr->row_stride[level];
   pt->layer_stride = lpr->img_stride[level];
   pt->usage = usage;
   *transfer = pt;

   assert(level < LP_MAX_TEXTURE_LEVELS);

   /*
   printf("tex_transfer_map(%d, %d  %d x %d of %d x %d,  usage %d )\n",
          transfer->x, transfer->y, transfer->width, transfer->height,
          transfer->texture->width0,
          transfer->texture->height0,
          transfer->usage);
   */

   if (usage == PIPE_TRANSFER_READ) {
      tex_usage = LP_TEX_USAGE_READ;
      mode = "read";
   }
   else {
      tex_usage = LP_TEX_USAGE_READ_WRITE;
      mode = "read/write";
   }

   if (0) {
      printf("transfer map tex %u  mode %s\n", lpr->id, mode);
   }

   format = lpr->base.format;

   map = llvmpipe_resource_map(resource,
                               level,
                               box->z,
                               tex_usage);


   /* May want to do different things here depending on read/write nature
    * of the map:
    */
   if (usage & PIPE_TRANSFER_WRITE) {
      /* Do something to notify sharing contexts of a texture change.
       */
      screen->timestamp++;
   }

   map +=
      box->y / util_format_get_blockheight(format) * pt->stride +
      box->x / util_format_get_blockwidth(format) * util_format_get_blocksize(format);

   return map;
}


static void
llvmpipe_transfer_unmap(struct pipe_context *pipe,
                        struct pipe_transfer *transfer)
{
   assert(transfer->resource);

   llvmpipe_resource_unmap(transfer->resource,
                           transfer->level,
                           transfer->box.z);

   /* Effectively do the texture_update work here - if texture images
    * needed post-processing to put them into hardware layout, this is
    * where it would happen.  For llvmpipe, nothing to do.
    */
   assert (transfer->resource);
   pipe_resource_reference(&transfer->resource, NULL);
   FREE(transfer);
}

unsigned int
llvmpipe_is_resource_referenced( struct pipe_context *pipe,
                                 struct pipe_resource *presource,
                                 unsigned level)
{
   struct llvmpipe_context *llvmpipe = llvmpipe_context( pipe );

   /*
    * XXX checking only resources with the right bind flags
    * is unsafe since with opengl state tracker we can end up
    * with resources bound to places they weren't supposed to be
    * (buffers bound as sampler views is one possibility here).
    */
   if (!(presource->bind & (PIPE_BIND_DEPTH_STENCIL |
                            PIPE_BIND_RENDER_TARGET |
                            PIPE_BIND_SAMPLER_VIEW)))
      return LP_UNREFERENCED;

   return lp_setup_is_resource_referenced(llvmpipe->setup, presource);
}


/**
 * Returns the largest possible alignment for a format in llvmpipe
 */
unsigned
llvmpipe_get_format_alignment( enum pipe_format format )
{
   const struct util_format_description *desc = util_format_description(format);
   unsigned size = 0;
   unsigned bytes;
   unsigned i;

   for (i = 0; i < desc->nr_channels; ++i) {
      size += desc->channel[i].size;
   }

   bytes = size / 8;

   if (!util_is_power_of_two(bytes)) {
      bytes /= desc->nr_channels;
   }

   if (bytes % 2 || bytes < 1) {
      return 1;
   } else {
      return bytes;
   }
}


/**
 * Create buffer which wraps user-space data.
 */
struct pipe_resource *
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


/**
 * Compute size (in bytes) need to store a texture image / mipmap level,
 * for just one cube face, one array layer or one 3D texture slice
 */
static unsigned
tex_image_face_size(const struct llvmpipe_resource *lpr, unsigned level)
{
   return lpr->img_stride[level];
}


/**
 * Compute size (in bytes) need to store a texture image / mipmap level,
 * including all cube faces or 3D image slices
 */
static unsigned
tex_image_size(const struct llvmpipe_resource *lpr, unsigned level)
{
   const unsigned buf_size = tex_image_face_size(lpr, level);
   return buf_size * lpr->num_slices_faces[level];
}


/**
 * Return pointer to a 2D texture image/face/slice.
 * No tiled/linear conversion is done.
 */
ubyte *
llvmpipe_get_texture_image_address(struct llvmpipe_resource *lpr,
                                   unsigned face_slice, unsigned level)
{
   struct llvmpipe_texture_image *img;
   unsigned offset;

   img = &lpr->linear_img;
   offset = lpr->linear_mip_offsets[level];

   if (face_slice > 0)
      offset += face_slice * tex_image_face_size(lpr, level);

   return (ubyte *) img->data + offset;
}


/**
 * Allocate storage for a linear image
 * (all cube faces and all 3D slices, all levels).
 */
static void
alloc_image_data(struct llvmpipe_resource *lpr)
{
   uint alignment = MAX2(64, util_cpu_caps.cacheline);
   uint level;
   uint offset = 0;

   if (lpr->dt) {
      /* we get the linear memory from the winsys, and it has
       * already been zeroed
       */
      struct llvmpipe_screen *screen = llvmpipe_screen(lpr->base.screen);
      struct sw_winsys *winsys = screen->winsys;

      assert(lpr->base.last_level == 0);

      lpr->linear_img.data =
         winsys->displaytarget_map(winsys, lpr->dt,
                                   PIPE_TRANSFER_READ_WRITE);
   }
   else {
      /* not a display target - allocate regular memory */
      /*
       * Offset calculation for start of a specific mip/layer is always
       * offset = lpr->linear_mip_offsets[level] + lpr->img_stride[level] * layer
       */
      for (level = 0; level <= lpr->base.last_level; level++) {
         uint buffer_size = tex_image_size(lpr, level);
         lpr->linear_mip_offsets[level] = offset;
         offset += align(buffer_size, alignment);
      }
      lpr->linear_img.data = align_malloc(offset, alignment);
      if (lpr->linear_img.data) {
         memset(lpr->linear_img.data, 0, offset);
      }
   }
}



/**
 * Return pointer to texture image data
 * for a particular cube face or 3D texture slice.
 *
 * \param face_slice  the cube face or 3D slice of interest
 * \param usage  one of LP_TEX_USAGE_READ/WRITE_ALL/READ_WRITE
 */
void *
llvmpipe_get_texture_image(struct llvmpipe_resource *lpr,
                           unsigned face_slice, unsigned level,
                           enum lp_texture_usage usage)
{
   struct llvmpipe_texture_image *target_img;
   void *target_data;
   unsigned target_offset;
   unsigned *target_off_ptr;

   assert(usage == LP_TEX_USAGE_READ ||
          usage == LP_TEX_USAGE_READ_WRITE ||
          usage == LP_TEX_USAGE_WRITE_ALL);

   if (lpr->dt) {
      assert(lpr->linear_img.data);
   }

   target_img = &lpr->linear_img;
   target_off_ptr = lpr->linear_mip_offsets;
   target_data = target_img->data;

   if (!target_data) {
      /* allocate memory for the target image now */
      alloc_image_data(lpr);
      target_data = target_img->data;
   }

   target_offset = target_off_ptr[level];

   if (face_slice > 0) {
      target_offset += face_slice * tex_image_face_size(lpr, level);
   }

   if (target_data) {
      target_data = (uint8_t *) target_data + target_offset;
   }

   return target_data;
}


/**
 * Return pointer to start of a texture image (1D, 2D, 3D, CUBE).
 * This is typically used when we're about to sample from a texture.
 */
void *
llvmpipe_get_texture_image_all(struct llvmpipe_resource *lpr,
                               unsigned level,
                               enum lp_texture_usage usage)
{
   const int slices = lpr->num_slices_faces[level];
   int slice;
   void *map = NULL;

   assert(slices > 0);

   for (slice = slices - 1; slice >= 0; slice--) {
      map = llvmpipe_get_texture_image(lpr, slice, level, usage);
   }

   return map;
}


/**
 * Get pointer to a linear image (not the tile!) at tile (x,y).
 * \return pointer to start of image/face (not the tile)
 */
ubyte *
llvmpipe_get_texture_tile_linear(struct llvmpipe_resource *lpr,
                                 unsigned face_slice, unsigned level,
                                 enum lp_texture_usage usage,
                                 unsigned x, unsigned y)
{
   struct llvmpipe_texture_image *linear_img = &lpr->linear_img;
   uint8_t *linear_image;

   assert(llvmpipe_resource_is_texture(&lpr->base));
   assert(x % TILE_SIZE == 0);
   assert(y % TILE_SIZE == 0);

   if (!linear_img->data) {
      /* allocate memory for the linear image now */
      /* XXX should probably not do that here? */
      alloc_image_data(lpr);
   }
   assert(linear_img->data);

   /* compute address of the slice/face of the image that contains the tile */
   linear_image = llvmpipe_get_texture_image_address(lpr, face_slice, level);

   return linear_image;
}


/**
 * Return size of resource in bytes
 */
unsigned
llvmpipe_resource_size(const struct pipe_resource *resource)
{
   const struct llvmpipe_resource *lpr = llvmpipe_resource_const(resource);
   unsigned lvl, size = 0;

   if (llvmpipe_resource_is_texture(resource)) {
      for (lvl = 0; lvl <= lpr->base.last_level; lvl++) {
         if (lpr->linear_img.data)
            size += tex_image_size(lpr, lvl);
      }
   }
   else {
      size = resource->width0;
   }

   return size;
}


#ifdef DEBUG
void
llvmpipe_print_resources(void)
{
   struct llvmpipe_resource *lpr;
   unsigned n = 0, total = 0;

   debug_printf("LLVMPIPE: current resources:\n");
   foreach(lpr, &resource_list) {
      unsigned size = llvmpipe_resource_size(&lpr->base);
      debug_printf("resource %u at %p, size %ux%ux%u: %u bytes, refcount %u\n",
                   lpr->id, (void *) lpr,
                   lpr->base.width0, lpr->base.height0, lpr->base.depth0,
                   size, lpr->base.reference.count);
      total += size;
      n++;
   }
   debug_printf("LLVMPIPE: total size of %u resources: %u\n", n, total);
}
#endif


void
llvmpipe_init_screen_resource_funcs(struct pipe_screen *screen)
{
#ifdef DEBUG
   /* init linked list for tracking resources */
   {
      static boolean first_call = TRUE;
      if (first_call) {
         memset(&resource_list, 0, sizeof(resource_list));
         make_empty_list(&resource_list);
         first_call = FALSE;
      }
   }
#endif

   screen->resource_create = llvmpipe_resource_create;
   screen->resource_destroy = llvmpipe_resource_destroy;
   screen->resource_from_handle = llvmpipe_resource_from_handle;
   screen->resource_get_handle = llvmpipe_resource_get_handle;
   screen->can_create_resource = llvmpipe_can_create_resource;
}


void
llvmpipe_init_context_resource_funcs(struct pipe_context *pipe)
{
   pipe->transfer_map = llvmpipe_transfer_map;
   pipe->transfer_unmap = llvmpipe_transfer_unmap;

   pipe->transfer_flush_region = u_default_transfer_flush_region;
   pipe->transfer_inline_write = u_default_transfer_inline_write;
}
