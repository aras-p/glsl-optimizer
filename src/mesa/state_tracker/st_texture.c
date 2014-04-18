/**************************************************************************
 * 
 * Copyright 2007 VMware, Inc.
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

#include <stdio.h>

#include "st_context.h"
#include "st_format.h"
#include "st_texture.h"
#include "st_cb_fbo.h"
#include "main/enums.h"

#include "pipe/p_state.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_rect.h"
#include "util/u_math.h"
#include "util/u_memory.h"


#define DBG if(0) printf


/**
 * Allocate a new pipe_resource object
 * width0, height0, depth0 are the dimensions of the level 0 image
 * (the highest resolution).  last_level indicates how many mipmap levels
 * to allocate storage for.  For non-mipmapped textures, this will be zero.
 */
struct pipe_resource *
st_texture_create(struct st_context *st,
                  enum pipe_texture_target target,
		  enum pipe_format format,
		  GLuint last_level,
		  GLuint width0,
		  GLuint height0,
		  GLuint depth0,
                  GLuint layers,
                  GLuint nr_samples,
                  GLuint bind )
{
   struct pipe_resource pt, *newtex;
   struct pipe_screen *screen = st->pipe->screen;

   assert(target < PIPE_MAX_TEXTURE_TYPES);
   assert(width0 > 0);
   assert(height0 > 0);
   assert(depth0 > 0);
   if (target == PIPE_TEXTURE_CUBE)
      assert(layers == 6);

   DBG("%s target %d format %s last_level %d\n", __FUNCTION__,
       (int) target, util_format_name(format), last_level);

   assert(format);
   assert(screen->is_format_supported(screen, format, target, 0,
                                      PIPE_BIND_SAMPLER_VIEW));

   memset(&pt, 0, sizeof(pt));
   pt.target = target;
   pt.format = format;
   pt.last_level = last_level;
   pt.width0 = width0;
   pt.height0 = height0;
   pt.depth0 = depth0;
   pt.array_size = (target == PIPE_TEXTURE_CUBE ? 6 : layers);
   pt.usage = PIPE_USAGE_DEFAULT;
   pt.bind = bind;
   pt.flags = 0;
   pt.nr_samples = nr_samples;

   newtex = screen->resource_create(screen, &pt);

   assert(!newtex || pipe_is_referenced(&newtex->reference));

   return newtex;
}


/**
 * In OpenGL the number of 1D array texture layers is the "height" and
 * the number of 2D array texture layers is the "depth".  In Gallium the
 * number of layers in an array texture is a separate 'array_size' field.
 * This function converts dimensions from the former to the later.
 */
void
st_gl_texture_dims_to_pipe_dims(GLenum texture,
                                GLuint widthIn,
                                GLuint heightIn,
                                GLuint depthIn,
                                GLuint *widthOut,
                                GLuint *heightOut,
                                GLuint *depthOut,
                                GLuint *layersOut)
{
   switch (texture) {
   case GL_TEXTURE_1D:
   case GL_PROXY_TEXTURE_1D:
      assert(heightIn == 1);
      assert(depthIn == 1);
      *widthOut = widthIn;
      *heightOut = 1;
      *depthOut = 1;
      *layersOut = 1;
      break;
   case GL_TEXTURE_1D_ARRAY:
   case GL_PROXY_TEXTURE_1D_ARRAY:
      assert(depthIn == 1);
      *widthOut = widthIn;
      *heightOut = 1;
      *depthOut = 1;
      *layersOut = heightIn;
      break;
   case GL_TEXTURE_2D:
   case GL_PROXY_TEXTURE_2D:
   case GL_TEXTURE_RECTANGLE:
   case GL_PROXY_TEXTURE_RECTANGLE:
   case GL_TEXTURE_EXTERNAL_OES:
   case GL_PROXY_TEXTURE_2D_MULTISAMPLE:
   case GL_TEXTURE_2D_MULTISAMPLE:
      assert(depthIn == 1);
      *widthOut = widthIn;
      *heightOut = heightIn;
      *depthOut = 1;
      *layersOut = 1;
      break;
   case GL_TEXTURE_CUBE_MAP:
   case GL_PROXY_TEXTURE_CUBE_MAP:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
   case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
   case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
      assert(depthIn == 1);
      *widthOut = widthIn;
      *heightOut = heightIn;
      *depthOut = 1;
      *layersOut = 6;
      break;
   case GL_TEXTURE_2D_ARRAY:
   case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:
   case GL_PROXY_TEXTURE_2D_ARRAY:
   case GL_PROXY_TEXTURE_2D_MULTISAMPLE_ARRAY:
      *widthOut = widthIn;
      *heightOut = heightIn;
      *depthOut = 1;
      *layersOut = depthIn;
      break;
   case GL_TEXTURE_CUBE_MAP_ARRAY:
   case GL_PROXY_TEXTURE_CUBE_MAP_ARRAY:
      *widthOut = widthIn;
      *heightOut = heightIn;
      *depthOut = 1;
      *layersOut = depthIn;
      break;
   default:
      assert(0 && "Unexpected texture in st_gl_texture_dims_to_pipe_dims()");
      /* fall-through */
   case GL_TEXTURE_3D:
   case GL_PROXY_TEXTURE_3D:
      *widthOut = widthIn;
      *heightOut = heightIn;
      *depthOut = depthIn;
      *layersOut = 1;
      break;
   }
}


/**
 * Check if a texture image can be pulled into a unified mipmap texture.
 */
GLboolean
st_texture_match_image(const struct pipe_resource *pt,
                       const struct gl_texture_image *image)
{
   GLuint ptWidth, ptHeight, ptDepth, ptLayers;

   /* Images with borders are never pulled into mipmap textures. 
    */
   if (image->Border) 
      return GL_FALSE;

   /* Check if this image's format matches the established texture's format.
    */
   if (st_mesa_format_to_pipe_format(image->TexFormat) != pt->format)
      return GL_FALSE;

   st_gl_texture_dims_to_pipe_dims(image->TexObject->Target,
                                   image->Width, image->Height, image->Depth,
                                   &ptWidth, &ptHeight, &ptDepth, &ptLayers);

   /* Test if this image's size matches what's expected in the
    * established texture.
    */
   if (ptWidth != u_minify(pt->width0, image->Level) ||
       ptHeight != u_minify(pt->height0, image->Level) ||
       ptDepth != u_minify(pt->depth0, image->Level) ||
       ptLayers != pt->array_size)
      return GL_FALSE;

   return GL_TRUE;
}


/**
 * Map a texture image and return the address for a particular 2D face/slice/
 * layer.  The stImage indicates the cube face and mipmap level.  The slice
 * of the 3D texture is passed in 'zoffset'.
 * \param usage  one of the PIPE_TRANSFER_x values
 * \param x, y, w, h  the region of interest of the 2D image.
 * \return address of mapping or NULL if any error
 */
GLubyte *
st_texture_image_map(struct st_context *st, struct st_texture_image *stImage,
                     enum pipe_transfer_usage usage,
                     GLuint x, GLuint y, GLuint z,
                     GLuint w, GLuint h, GLuint d,
                     struct pipe_transfer **transfer)
{
   struct st_texture_object *stObj =
      st_texture_object(stImage->base.TexObject);
   GLuint level;
   void *map;

   DBG("%s \n", __FUNCTION__);

   if (!stImage->pt)
      return NULL;

   if (stObj->pt != stImage->pt)
      level = 0;
   else
      level = stImage->base.Level;

   z += stImage->base.Face;

   map = pipe_transfer_map_3d(st->pipe, stImage->pt, level, usage,
                              x, y, z, w, h, d, transfer);
   if (map) {
      /* Enlarge the transfer array if it's not large enough. */
      if (z >= stImage->num_transfers) {
         unsigned new_size = z + 1;

         stImage->transfer = realloc(stImage->transfer,
                                     new_size * sizeof(void*));
         memset(&stImage->transfer[stImage->num_transfers], 0,
               (new_size - stImage->num_transfers) * sizeof(void*));
         stImage->num_transfers = new_size;
      }

      assert(!stImage->transfer[z]);
      stImage->transfer[z] = *transfer;
   }
   return map;
}


void
st_texture_image_unmap(struct st_context *st,
                       struct st_texture_image *stImage, unsigned slice)
{
   struct pipe_context *pipe = st->pipe;
   struct pipe_transfer **transfer =
      &stImage->transfer[slice + stImage->base.Face];

   DBG("%s\n", __FUNCTION__);

   pipe_transfer_unmap(pipe, *transfer);
   *transfer = NULL;
}


/* Upload data for a particular image.
 */
void
st_texture_image_data(struct st_context *st,
                      struct pipe_resource *dst,
                      GLuint face,
                      GLuint level,
                      void *src,
                      GLuint src_row_stride, GLuint src_image_stride)
{
   struct pipe_context *pipe = st->pipe;
   GLuint i;
   const GLubyte *srcUB = src;
   GLuint layers;

   if (dst->target == PIPE_TEXTURE_1D_ARRAY ||
       dst->target == PIPE_TEXTURE_2D_ARRAY ||
       dst->target == PIPE_TEXTURE_CUBE_ARRAY)
      layers = dst->array_size;
   else
      layers = u_minify(dst->depth0, level);

   DBG("%s\n", __FUNCTION__);

   for (i = 0; i < layers; i++) {
      struct pipe_box box;
      u_box_2d_zslice(0, 0, face + i,
                      u_minify(dst->width0, level),
                      u_minify(dst->height0, level),
                      &box);

      pipe->transfer_inline_write(pipe, dst, level, PIPE_TRANSFER_WRITE,
                                  &box, srcUB, src_row_stride, 0);

      srcUB += src_image_stride;
   }
}


/**
 * For debug only: get/print center pixel in the src resource.
 */
static void
print_center_pixel(struct pipe_context *pipe, struct pipe_resource *src)
{
   struct pipe_transfer *xfer;
   struct pipe_box region;
   ubyte *map;

   region.x = src->width0 / 2;
   region.y = src->height0 / 2;
   region.z = 0;
   region.width = 1;
   region.height = 1;
   region.depth = 1;

   map = pipe->transfer_map(pipe, src, 0, PIPE_TRANSFER_READ, &region, &xfer);

   printf("center pixel: %d %d %d %d\n", map[0], map[1], map[2], map[3]);

   pipe->transfer_unmap(pipe, xfer);
}


/**
 * Copy the image at level=0 in 'src' to the 'dst' resource at 'dstLevel'.
 * This is used to copy mipmap images from one texture buffer to another.
 * This typically happens when our initial guess at the total texture size
 * is incorrect (see the guess_and_alloc_texture() function).
 */
void
st_texture_image_copy(struct pipe_context *pipe,
                      struct pipe_resource *dst, GLuint dstLevel,
                      struct pipe_resource *src, GLuint srcLevel,
                      GLuint face)
{
   GLuint width = u_minify(dst->width0, dstLevel);
   GLuint height = u_minify(dst->height0, dstLevel);
   GLuint depth = u_minify(dst->depth0, dstLevel);
   struct pipe_box src_box;
   GLuint i;

   if (u_minify(src->width0, srcLevel) != width ||
       u_minify(src->height0, srcLevel) != height ||
       u_minify(src->depth0, srcLevel) != depth) {
      /* The source image size doesn't match the destination image size.
       * This can happen in some degenerate situations such as rendering to a
       * cube map face which was set up with mismatched texture sizes.
       */
      return;
   }

   src_box.x = 0;
   src_box.y = 0;
   src_box.width = width;
   src_box.height = height;
   src_box.depth = 1;
   /* Loop over 3D image slices */
   /* could (and probably should) use "true" 3d box here -
      but drivers can't quite handle it yet */
   for (i = face; i < face + depth; i++) {
      src_box.z = i;

      if (0)  {
         print_center_pixel(pipe, src);
      }

      pipe->resource_copy_region(pipe,
                                 dst,
                                 dstLevel,
                                 0, 0, i,/* destX, Y, Z */
                                 src,
                                 srcLevel,
                                 &src_box);
   }
}


struct pipe_resource *
st_create_color_map_texture(struct gl_context *ctx)
{
   struct st_context *st = st_context(ctx);
   struct pipe_resource *pt;
   enum pipe_format format;
   const uint texSize = 256; /* simple, and usually perfect */

   /* find an RGBA texture format */
   format = st_choose_format(st, GL_RGBA, GL_NONE, GL_NONE,
                             PIPE_TEXTURE_2D, 0, PIPE_BIND_SAMPLER_VIEW,
                             FALSE);

   /* create texture for color map/table */
   pt = st_texture_create(st, PIPE_TEXTURE_2D, format, 0,
                          texSize, texSize, 1, 1, 0, PIPE_BIND_SAMPLER_VIEW);
   return pt;
}

/**
 * Try to find a matching sampler view for the given context.
 * If none is found an empty slot is initialized with a
 * template and returned instead.
 */
struct pipe_sampler_view **
st_texture_get_sampler_view(struct st_context *st,
                            struct st_texture_object *stObj)
{
   struct pipe_sampler_view *used = NULL, **free = NULL;
   GLuint i;

   for (i = 0; i < stObj->num_sampler_views; ++i) {
      struct pipe_sampler_view **sv = &stObj->sampler_views[i];
      /* Is the array entry used ? */
      if (*sv) {
         /* Yes, check if it's the right one */
         if ((*sv)->context == st->pipe)
            return sv;

         /* Wasn't the right one, but remember it as template */
         used = *sv;
      } else {
         /* Found a free slot, remember that */
         free = sv;
      }
   }

   /* Couldn't find a slot for our context, create a new one */

   if (!free) {
      /* Haven't even found a free one, resize the array */
      GLuint old_size = stObj->num_sampler_views * sizeof(void *);
      GLuint new_size = old_size + sizeof(void *);
      stObj->sampler_views = REALLOC(stObj->sampler_views, old_size, new_size);
      free = &stObj->sampler_views[stObj->num_sampler_views++];
      *free = NULL;
   }

   /* Add just any sampler view to be used as a template */
   if (used)
      pipe_sampler_view_reference(free, used);

   return free;
}

void
st_texture_release_sampler_view(struct st_context *st,
                                struct st_texture_object *stObj)
{
   GLuint i;

   for (i = 0; i < stObj->num_sampler_views; ++i) {
      struct pipe_sampler_view **sv = &stObj->sampler_views[i];

      if (*sv && (*sv)->context == st->pipe) {
         pipe_sampler_view_reference(sv, NULL);
         break;
      }
   }
}

void
st_texture_release_all_sampler_views(struct st_texture_object *stObj)
{
   GLuint i;

   for (i = 0; i < stObj->num_sampler_views; ++i)
      pipe_sampler_view_reference(&stObj->sampler_views[i], NULL);
}


void
st_texture_free_sampler_views(struct st_texture_object *stObj)
{
   /* NOTE:
    * We use FREE() here to match REALLOC() above.  Both come from
    * u_memory.h, not imports.h.  If we mis-match MALLOC/FREE from
    * those two headers we can trash the heap.
    */
   FREE(stObj->sampler_views);
}
