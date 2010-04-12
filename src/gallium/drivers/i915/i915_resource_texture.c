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

#include "pipe/p_state.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#include "i915_context.h"
#include "i915_resource.h"
#include "i915_screen.h"
#include "i915_winsys.h"


/*
 * Helper function and arrays
 */


/**
 * Initial offset for Cube map.
 */
static const int initial_offsets[6][2] = {
   {0, 0},
   {0, 2},
   {1, 0},
   {1, 2},
   {1, 1},
   {1, 3}
};

/**
 * Step offsets for Cube map.
 */
static const int step_offsets[6][2] = {
   {0, 2},
   {0, 2},
   {-1, 2},
   {-1, 2},
   {-1, 1},
   {-1, 1}
};

/* XXX really need twice the size if x is already pot?
   Otherwise just use util_next_power_of_two?
*/
static unsigned
power_of_two(unsigned x)
{
   unsigned value = 1;
   while (value < x)
      value = value << 1;
   return value;
}

/*
 * More advanced helper funcs
 */


static void
i915_texture_set_level_info(struct i915_texture *tex,
                            unsigned level,
                            unsigned nr_images,
                            unsigned w, unsigned h, unsigned d)
{
   assert(level < Elements(tex->nr_images));

   tex->nr_images[level] = nr_images;

   /*
   DBG("%s level %d size: %d,%d,%d offset %d,%d (0x%x)\n", __FUNCTION__,
       level, w, h, d, x, y, tex->level_offset[level]);
   */

   /* Not sure when this would happen, but anyway: 
    */
   if (tex->image_offset[level]) {
      FREE(tex->image_offset[level]);
      tex->image_offset[level] = NULL;
   }

   assert(nr_images);
   assert(!tex->image_offset[level]);

   tex->image_offset[level] = (unsigned *) MALLOC(nr_images * sizeof(unsigned));
   tex->image_offset[level][0] = 0;
}

static void
i915_texture_set_image_offset(struct i915_texture *tex,
                              unsigned level, unsigned img, unsigned x, unsigned y)
{
   if (img == 0 && level == 0)
      assert(x == 0 && y == 0);

   assert(img < tex->nr_images[level]);

   tex->image_offset[level][img] = y * tex->stride + x * util_format_get_blocksize(tex->b.b.format);

   /*
   printf("%s level %d img %d pos %d,%d image_offset %x\n",
       __FUNCTION__, level, img, x, y, tex->image_offset[level][img]);
   */
}


/*
 * Shared layout functions
 */


/**
 * Special case to deal with scanout textures.
 */
static boolean
i9x5_scanout_layout(struct i915_texture *tex)
{
   struct pipe_resource *pt = &tex->b.b;

   if (pt->last_level > 0 || util_format_get_blocksize(pt->format) != 4)
      return FALSE;

   i915_texture_set_level_info(tex, 0, 1,
                               pt->width0,
                               pt->height0,
                               1);
   i915_texture_set_image_offset(tex, 0, 0, 0, 0);

   if (pt->width0 >= 240) {
      tex->stride = power_of_two(util_format_get_stride(pt->format, pt->width0));
      tex->total_nblocksy = align(util_format_get_nblocksy(pt->format, pt->height0), 8);
      tex->hw_tiled = I915_TILE_X;
   } else if (pt->width0 == 64 && pt->height0 == 64) {
      tex->stride = power_of_two(util_format_get_stride(pt->format, pt->width0));
      tex->total_nblocksy = align(util_format_get_nblocksy(pt->format, pt->height0), 8);
   } else {
      return FALSE;
   }

   debug_printf("%s size: %d,%d,%d offset %d,%d (0x%x)\n", __FUNCTION__,
      pt->width0, pt->height0, util_format_get_blocksize(pt->format),
      tex->stride, tex->total_nblocksy, tex->stride * tex->total_nblocksy);

   return TRUE;
}

/**
 * Special case to deal with shared textures.
 */
static boolean
i9x5_display_target_layout(struct i915_texture *tex)
{
   struct pipe_resource *pt = &tex->b.b;

   if (pt->last_level > 0 || util_format_get_blocksize(pt->format) != 4)
      return FALSE;

   /* fallback to normal textures for small textures */
   if (pt->width0 < 240)
      return FALSE;

   i915_texture_set_level_info(tex, 0, 1,
                               pt->width0,
                               pt->height0,
                               1);
   i915_texture_set_image_offset(tex, 0, 0, 0, 0);

   tex->stride = power_of_two(util_format_get_stride(pt->format, pt->width0));
   tex->total_nblocksy = align(util_format_get_nblocksy(pt->format, pt->height0), 8);
   tex->hw_tiled = I915_TILE_X;

   debug_printf("%s size: %d,%d,%d offset %d,%d (0x%x)\n", __FUNCTION__,
      pt->width0, pt->height0, util_format_get_blocksize(pt->format),
      tex->stride, tex->total_nblocksy, tex->stride * tex->total_nblocksy);

   return TRUE;
}


/*
 * i915 layout functions
 */


static void
i915_texture_layout_2d(struct i915_texture *tex)
{
   struct pipe_resource *pt = &tex->b.b;
   unsigned level;
   unsigned width = pt->width0;
   unsigned height = pt->height0;
   unsigned nblocksy = util_format_get_nblocksy(pt->format, pt->width0);

   /* used for scanouts that need special layouts */
   if (pt->bind & PIPE_BIND_SCANOUT)
      if (i9x5_scanout_layout(tex))
         return;

   /* shared buffers needs to be compatible with X servers 
    * 
    * XXX: need a better name than shared for this if it is to be part
    * of core gallium, and probably move the flag to resource.flags,
    * rather than bindings.
    */
   if (pt->bind & (PIPE_BIND_SHARED | PIPE_BIND_DISPLAY_TARGET))
      if (i9x5_display_target_layout(tex))
         return;

   tex->stride = align(util_format_get_stride(pt->format, pt->width0), 4);
   tex->total_nblocksy = 0;

   for (level = 0; level <= pt->last_level; level++) {
      i915_texture_set_level_info(tex, level, 1, width, height, 1);
      i915_texture_set_image_offset(tex, level, 0, 0, tex->total_nblocksy);

      nblocksy = align(MAX2(2, nblocksy), 2);

      tex->total_nblocksy += nblocksy;

      width = u_minify(width, 1);
      height = u_minify(height, 1);
      nblocksy = util_format_get_nblocksy(pt->format, height);
   }
}

static void
i915_texture_layout_3d(struct i915_texture *tex)
{
   struct pipe_resource *pt = &tex->b.b;
   unsigned level;

   unsigned width = pt->width0;
   unsigned height = pt->height0;
   unsigned depth = pt->depth0;
   unsigned nblocksy = util_format_get_nblocksy(pt->format, pt->height0);
   unsigned stack_nblocksy = 0;

   /* Calculate the size of a single slice. 
    */
   tex->stride = align(util_format_get_stride(pt->format, pt->width0), 4);

   /* XXX: hardware expects/requires 9 levels at minimum.
    */
   for (level = 0; level <= MAX2(8, pt->last_level); level++) {
      i915_texture_set_level_info(tex, level, depth, width, height, depth);

      stack_nblocksy += MAX2(2, nblocksy);

      width = u_minify(width, 1);
      height = u_minify(height, 1);
      nblocksy = util_format_get_nblocksy(pt->format, height);
   }

   /* Fixup depth image_offsets: 
    */
   for (level = 0; level <= pt->last_level; level++) {
      unsigned i;
      for (i = 0; i < depth; i++) 
         i915_texture_set_image_offset(tex, level, i, 0, i * stack_nblocksy);

      depth = u_minify(depth, 1);
   }

   /* Multiply slice size by texture depth for total size.  It's
    * remarkable how wasteful of memory the i915 texture layouts
    * are.  They are largely fixed in the i945.
    */
   tex->total_nblocksy = stack_nblocksy * pt->depth0;
}

static void
i915_texture_layout_cube(struct i915_texture *tex)
{
   struct pipe_resource *pt = &tex->b.b;
   unsigned width = pt->width0, height = pt->height0;
   const unsigned nblocks = util_format_get_nblocksx(pt->format, pt->width0);
   unsigned level;
   unsigned face;

   assert(width == height); /* cubemap images are square */

   /* double pitch for cube layouts */
   tex->stride = align(nblocks * util_format_get_blocksize(pt->format) * 2, 4);
   tex->total_nblocksy = nblocks * 4;

   for (level = 0; level <= pt->last_level; level++) {
      i915_texture_set_level_info(tex, level, 6, width, height, 1);
      width /= 2;
      height /= 2;
   }

   for (face = 0; face < 6; face++) {
      unsigned x = initial_offsets[face][0] * nblocks;
      unsigned y = initial_offsets[face][1] * nblocks;
      unsigned d = nblocks;

      for (level = 0; level <= pt->last_level; level++) {
         i915_texture_set_image_offset(tex, level, face, x, y);
         d >>= 1;
         x += step_offsets[face][0] * d;
         y += step_offsets[face][1] * d;
      }
   }
}

static boolean
i915_texture_layout(struct i915_texture * tex)
{
   struct pipe_resource *pt = &tex->b.b;

   switch (pt->target) {
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_2D:
      i915_texture_layout_2d(tex);
      break;
   case PIPE_TEXTURE_3D:
      i915_texture_layout_3d(tex);
      break;
   case PIPE_TEXTURE_CUBE:
      i915_texture_layout_cube(tex);
      break;
   default:
      assert(0);
      return FALSE;
   }

   return TRUE;
}


/*
 * i945 layout functions
 */


static void
i945_texture_layout_2d(struct i915_texture *tex)
{
   struct pipe_resource *pt = &tex->b.b;
   const int align_x = 2, align_y = 4;
   unsigned level;
   unsigned x = 0;
   unsigned y = 0;
   unsigned width = pt->width0;
   unsigned height = pt->height0;
   unsigned nblocksx = util_format_get_nblocksx(pt->format, pt->width0);
   unsigned nblocksy = util_format_get_nblocksy(pt->format, pt->height0);

   /* used for scanouts that need special layouts */
   if (tex->b.b.bind & PIPE_BIND_SCANOUT)
      if (i9x5_scanout_layout(tex))
         return;

   /* shared buffers needs to be compatible with X servers */
   if (tex->b.b.bind & (PIPE_BIND_SHARED | PIPE_BIND_DISPLAY_TARGET))
      if (i9x5_display_target_layout(tex))
         return;

   tex->stride = align(util_format_get_stride(pt->format, pt->width0), 4);

   /* May need to adjust pitch to accomodate the placement of
    * the 2nd mipmap level.  This occurs when the alignment
    * constraints of mipmap placement push the right edge of the
    * 2nd mipmap level out past the width of its parent.
    */
   if (pt->last_level > 0) {
      unsigned mip1_nblocksx 
         = align(util_format_get_nblocksx(pt->format, u_minify(width, 1)), align_x)
         + util_format_get_nblocksx(pt->format, u_minify(width, 2));

      if (mip1_nblocksx > nblocksx)
         tex->stride = mip1_nblocksx * util_format_get_blocksize(pt->format);
   }

   /* Pitch must be a whole number of dwords
    */
   tex->stride = align(tex->stride, 64);
   tex->total_nblocksy = 0;

   for (level = 0; level <= pt->last_level; level++) {
      i915_texture_set_level_info(tex, level, 1, width, height, 1);
      i915_texture_set_image_offset(tex, level, 0, x, y);

      nblocksy = align(nblocksy, align_y);

      /* Because the images are packed better, the final offset
       * might not be the maximal one:
       */
      tex->total_nblocksy = MAX2(tex->total_nblocksy, y + nblocksy);

      /* Layout_below: step right after second mipmap level.
       */
      if (level == 1) {
         x += align(nblocksx, align_x);
      } else {
         y += nblocksy;
      }

      width  = u_minify(width, 1);
      height = u_minify(height, 1);
      nblocksx = util_format_get_nblocksx(pt->format, width);
      nblocksy = util_format_get_nblocksy(pt->format, height);
   }
}

static void
i945_texture_layout_3d(struct i915_texture *tex)
{
   struct pipe_resource *pt = &tex->b.b;
   unsigned width = pt->width0;
   unsigned height = pt->height0;
   unsigned depth = pt->depth0;
   unsigned nblocksy = util_format_get_nblocksy(pt->format, pt->width0);
   unsigned pack_x_pitch, pack_x_nr;
   unsigned pack_y_pitch;
   unsigned level;

   tex->stride = align(util_format_get_stride(pt->format, pt->width0), 4);
   tex->total_nblocksy = 0;

   pack_y_pitch = MAX2(nblocksy, 2);
   pack_x_pitch = tex->stride / util_format_get_blocksize(pt->format);
   pack_x_nr = 1;

   for (level = 0; level <= pt->last_level; level++) {
      int x = 0;
      int y = 0;
      unsigned q, j;

      i915_texture_set_level_info(tex, level, depth, width, height, depth);

      for (q = 0; q < depth;) {
         for (j = 0; j < pack_x_nr && q < depth; j++, q++) {
            i915_texture_set_image_offset(tex, level, q, x, y + tex->total_nblocksy);
            x += pack_x_pitch;
         }

         x = 0;
         y += pack_y_pitch;
      }

      tex->total_nblocksy += y;

      if (pack_x_pitch > 4) {
         pack_x_pitch >>= 1;
         pack_x_nr <<= 1;
         assert(pack_x_pitch * pack_x_nr * util_format_get_blocksize(pt->format) <= tex->stride);
      }

      if (pack_y_pitch > 2) {
         pack_y_pitch >>= 1;
      }

      width = u_minify(width, 1);
      height = u_minify(height, 1);
      depth = u_minify(depth, 1);
      nblocksy = util_format_get_nblocksy(pt->format, height);
   }
}

static void
i945_texture_layout_cube(struct i915_texture *tex)
{
   struct pipe_resource *pt = &tex->b.b;
   unsigned level;

   const unsigned nblocks = util_format_get_nblocksx(pt->format, pt->width0);
   unsigned face;
   unsigned width = pt->width0;
   unsigned height = pt->height0;

   /*
   printf("%s %i, %i\n", __FUNCTION__, pt->width0, pt->height0);
   */

   assert(width == height); /* cubemap images are square */

   /*
    * XXX Should only be used for compressed formats. But lets
    * keep this code active just in case.
    *
    * Depending on the size of the largest images, pitch can be
    * determined either by the old-style packing of cubemap faces,
    * or the final row of 4x4, 2x2 and 1x1 faces below this.
    */
   if (nblocks > 32)
      tex->stride = align(nblocks * util_format_get_blocksize(pt->format) * 2, 4);
   else
      tex->stride = 14 * 8 * util_format_get_blocksize(pt->format);

   tex->total_nblocksy = nblocks * 4;

   /* Set all the levels to effectively occupy the whole rectangular region.
   */
   for (level = 0; level <= pt->last_level; level++) {
      i915_texture_set_level_info(tex, level, 6, width, height, 1);
      width /= 2;
      height /= 2;
   }

   for (face = 0; face < 6; face++) {
      unsigned x = initial_offsets[face][0] * nblocks;
      unsigned y = initial_offsets[face][1] * nblocks;
      unsigned d = nblocks;

#if 0 /* Fix and enable this code for compressed formats */
      if (nblocks == 4 && face >= 4) {
         y = tex->total_height - 4;
         x = (face - 4) * 8;
      }
      else if (nblocks < 4 && (face > 0)) {
         y = tex->total_height - 4;
         x = face * 8;
      }
#endif

      for (level = 0; level <= pt->last_level; level++) {
         i915_texture_set_image_offset(tex, level, face, x, y);

         d >>= 1;

#if 0 /* Fix and enable this code for compressed formats */
         switch (d) {
            case 4:
               switch (face) {
                  case PIPE_TEX_FACE_POS_X:
                  case PIPE_TEX_FACE_NEG_X:
                     x += step_offsets[face][0] * d;
                     y += step_offsets[face][1] * d;
                     break;
                  case PIPE_TEX_FACE_POS_Y:
                  case PIPE_TEX_FACE_NEG_Y:
                     y += 12;
                     x -= 8;
                     break;
                  case PIPE_TEX_FACE_POS_Z:
                  case PIPE_TEX_FACE_NEG_Z:
                     y = tex->total_height - 4;
                     x = (face - 4) * 8;
                     break;
               }
            case 2:
               y = tex->total_height - 4;
               x = 16 + face * 8;
               break;

            case 1:
               x += 48;
               break;
            default:
#endif
               x += step_offsets[face][0] * d;
               y += step_offsets[face][1] * d;
#if 0
               break;
         }
#endif
      }
   }
}

static boolean
i945_texture_layout(struct i915_texture * tex)
{
   struct pipe_resource *pt = &tex->b.b;

   switch (pt->target) {
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_2D:
      i945_texture_layout_2d(tex);
      break;
   case PIPE_TEXTURE_3D:
      i945_texture_layout_3d(tex);
      break;
   case PIPE_TEXTURE_CUBE:
      i945_texture_layout_cube(tex);
      break;
   default:
      assert(0);
      return FALSE;
   }

   return TRUE;
}



/*
 * Screen texture functions
 */



static boolean
i915_texture_get_handle(struct pipe_screen * screen,
                        struct pipe_resource *texture,
                        struct winsys_handle *whandle)
{
   struct i915_screen *is = i915_screen(screen);
   struct i915_texture *tex = i915_texture(texture);
   struct i915_winsys *iws = is->iws;

   return iws->buffer_get_handle(iws, tex->buffer, whandle, tex->stride);
}


static void
i915_texture_destroy(struct pipe_screen *screen,
		     struct pipe_resource *pt)
{
   struct i915_texture *tex = i915_texture(pt);
   struct i915_winsys *iws = i915_screen(screen)->iws;
   uint i;

   /*
     DBG("%s deleting %p\n", __FUNCTION__, (void *) tex);
   */

   iws->buffer_destroy(iws, tex->buffer);

   for (i = 0; i < Elements(tex->image_offset); i++)
      if (tex->image_offset[i])
         FREE(tex->image_offset[i]);

   FREE(tex);
}

static struct pipe_transfer * 
i915_texture_get_transfer(struct pipe_context *context,
			  struct pipe_resource *resource,
			  struct pipe_subresource sr,
			  unsigned usage,
			  const struct pipe_box *box)
{
   struct i915_texture *tex = i915_texture(resource);
   struct pipe_transfer *transfer = CALLOC_STRUCT(pipe_transfer);
   if (transfer == NULL)
      return NULL;

   transfer->resource = resource;
   transfer->sr = sr;
   transfer->usage = usage;
   transfer->box = *box;
   transfer->stride = tex->stride;

   return transfer;
}


static void *
i915_texture_transfer_map(struct pipe_context *pipe,
			  struct pipe_transfer *transfer)
{
   struct pipe_resource *resource = transfer->resource;
   struct i915_texture *tex = i915_texture(resource);
   struct i915_winsys *iws = i915_screen(pipe->screen)->iws;
   struct pipe_subresource sr = transfer->sr;
   struct pipe_box *box = &transfer->box;
   enum pipe_format format = resource->format;
   unsigned offset;
   char *map;

   if (resource->target == PIPE_TEXTURE_CUBE) {
      offset = tex->image_offset[sr.level][sr.face];
   } else if (resource->target == PIPE_TEXTURE_3D) {
      offset = tex->image_offset[sr.level][box->z];
   } else {
      offset = tex->image_offset[sr.level][0];
      assert(sr.face == 0);
      assert(box->z == 0);
   }

   map = iws->buffer_map(iws, tex->buffer,
                         (transfer->usage & PIPE_TRANSFER_WRITE) ? TRUE : FALSE);
   if (map == NULL)
      return NULL;

   return map + offset +
      box->y / util_format_get_blockheight(format) * transfer->stride +
      box->x / util_format_get_blockwidth(format) * util_format_get_blocksize(format);
}

static void
i915_texture_transfer_unmap(struct pipe_context *pipe,
			    struct pipe_transfer *transfer)
{
   struct i915_texture *tex = i915_texture(transfer->resource);
   struct i915_winsys *iws = i915_screen(tex->b.b.screen)->iws;
   iws->buffer_unmap(iws, tex->buffer);
}



struct u_resource_vtbl i915_texture_vtbl = 
{
   i915_texture_get_handle,	      /* get_handle */
   i915_texture_destroy,	      /* resource_destroy */
   NULL,			      /* is_resource_referenced */
   i915_texture_get_transfer,	      /* get_transfer */
   u_default_transfer_destroy,	      /* transfer_destroy */
   i915_texture_transfer_map,	      /* transfer_map */
   u_default_transfer_flush_region,   /* transfer_flush_region */
   i915_texture_transfer_unmap,	      /* transfer_unmap */
   u_default_transfer_inline_write    /* transfer_inline_write */
};




struct pipe_resource *
i915_texture_create(struct pipe_screen *screen,
                    const struct pipe_resource *template)
{
   struct i915_screen *is = i915_screen(screen);
   struct i915_winsys *iws = is->iws;
   struct i915_texture *tex = CALLOC_STRUCT(i915_texture);
   size_t tex_size;
   unsigned buf_usage = 0;

   if (!tex)
      return NULL;

   tex->b.b = *template;
   tex->b.vtbl = &i915_texture_vtbl;
   pipe_reference_init(&tex->b.b.reference, 1);
   tex->b.b.screen = screen;

   if (is->is_i945) {
      if (!i945_texture_layout(tex))
         goto fail;
   } else {
      if (!i915_texture_layout(tex))
         goto fail;
   }

   tex_size = tex->stride * tex->total_nblocksy;

   /* for scanouts and cursors, cursors arn't scanouts */

   /* XXX: use a custom flag for cursors, don't rely on magically
    * guessing that this is Xorg asking for a cursor
    */
   if ((template->bind & PIPE_BIND_SCANOUT) && template->width0 != 64)
      buf_usage = I915_NEW_SCANOUT;
   else
      buf_usage = I915_NEW_TEXTURE;

   tex->buffer = iws->buffer_create(iws, tex_size, 64, buf_usage);
   if (!tex->buffer)
      goto fail;

   /* setup any hw fences */
   if (tex->hw_tiled) {
      assert(tex->sw_tiled == I915_TILE_NONE);
      iws->buffer_set_fence_reg(iws, tex->buffer, tex->stride, tex->hw_tiled);
   }

   
#if 0
   void *ptr = ws->buffer_map(ws, tex->buffer,
      PIPE_BUFFER_USAGE_CPU_WRITE);
   memset(ptr, 0x80, tex_size);
   ws->buffer_unmap(ws, tex->buffer);
#endif

   return &tex->b.b;

fail:
   FREE(tex);
   return NULL;
}

struct pipe_resource *
i915_texture_from_handle(struct pipe_screen * screen,
			  const struct pipe_resource *template,
			  struct winsys_handle *whandle)
{
   struct i915_screen *is = i915_screen(screen);
   struct i915_texture *tex;
   struct i915_winsys *iws = is->iws;
   struct i915_winsys_buffer *buffer;
   unsigned stride;

   assert(screen);

   buffer = iws->buffer_from_handle(iws, whandle, &stride);

   /* Only supports one type */
   if (template->target != PIPE_TEXTURE_2D ||
       template->last_level != 0 ||
       template->depth0 != 1) {
      return NULL;
   }

   tex = CALLOC_STRUCT(i915_texture);
   if (!tex)
      return NULL;

   tex->b.b = *template;
   tex->b.vtbl = &i915_texture_vtbl;
   pipe_reference_init(&tex->b.b.reference, 1);
   tex->b.b.screen = screen;

   tex->stride = stride;

   i915_texture_set_level_info(tex, 0, 1, template->width0, template->height0, 1);
   i915_texture_set_image_offset(tex, 0, 0, 0, 0);

   tex->buffer = buffer;

   return &tex->b.b;
}

