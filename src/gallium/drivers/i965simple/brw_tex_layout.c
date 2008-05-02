/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */


/* Code to layout images in a mipmap tree for i965.
 */

#include "brw_tex_layout.h"

#include "pipe/p_state.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"
#include "pipe/p_inlines.h"
#include "pipe/p_winsys.h"

#include "brw_context.h"

#define FILE_DEBUG_FLAG DEBUG_TEXTURE

#if 0
unsigned intel_compressed_alignment(unsigned internalFormat)
{
    unsigned alignment = 4;

    switch (internalFormat) {
    case GL_COMPRESSED_RGB_FXT1_3DFX:
    case GL_COMPRESSED_RGBA_FXT1_3DFX:
        alignment = 8;
        break;

    default:
        break;
    }

    return alignment;
}
#endif

static unsigned minify( unsigned d )
{
   return MAX2(1, d>>1);
}


static void intel_miptree_set_image_offset(struct brw_texture *tex,
                                           unsigned level,
                                           unsigned img,
                                           unsigned x, unsigned y)
{
   struct pipe_texture *pt = &tex->base;
   if (img == 0 && level == 0)
      assert(x == 0 && y == 0);
   assert(img < tex->nr_images[level]);

   tex->image_offset[level][img] = (x + y * tex->pitch) * pt->cpp;
}

static void intel_miptree_set_level_info(struct brw_texture *tex,
                                         unsigned level,
                                         unsigned nr_images,
                                         unsigned x, unsigned y,
                                         unsigned w, unsigned h, unsigned d)
{
   struct pipe_texture *pt = &tex->base;

   assert(level < PIPE_MAX_TEXTURE_LEVELS);

   pt->width[level] = w;
   pt->height[level] = h;
   pt->depth[level] = d;

   tex->level_offset[level] = (x + y * tex->pitch) * pt->cpp;
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

static void i945_miptree_layout_2d(struct brw_texture *tex)
{
   struct pipe_texture *pt = &tex->base;
   unsigned align_h = 2, align_w = 4;
   unsigned level;
   unsigned x = 0;
   unsigned y = 0;
   unsigned width = pt->width[0];
   unsigned height = pt->height[0];

   tex->pitch = pt->width[0];

#if 0
   if (pt->compressed) {
      align_w = intel_compressed_alignment(pt->internal_format);
      tex->pitch = ALIGN(pt->width[0], align_w);
   }
#endif

   /* May need to adjust pitch to accomodate the placement of
    * the 2nd mipmap.  This occurs when the alignment
    * constraints of mipmap placement push the right edge of the
    * 2nd mipmap out past the width of its parent.
    */
   if (pt->last_level > 0) {
      unsigned mip1_width;

      if (pt->compressed) {
         mip1_width = align(minify(pt->width[0]), align_w)
                      + align(minify(minify(pt->width[0])), align_w);
      } else {
         mip1_width = align(minify(pt->width[0]), align_w)
                      + minify(minify(pt->width[0]));
      }

      if (mip1_width > tex->pitch) {
         tex->pitch = mip1_width;
      }
   }

   /* Pitch must be a whole number of dwords, even though we
    * express it in texels.
    */
   tex->pitch = align(tex->pitch * pt->cpp, 4) / pt->cpp;
   tex->total_height = 0;

   for (level = 0; level <= pt->last_level; level++) {
      unsigned img_height;

      intel_miptree_set_level_info(tex, level, 1, x, y, width,
				   height, 1);

      if (pt->compressed)
	 img_height = MAX2(1, height/4);
      else
	 img_height = align(height, align_h);


      /* Because the images are packed better, the final offset
       * might not be the maximal one:
       */
      tex->total_height = MAX2(tex->total_height, y + img_height);

      /* Layout_below: step right after second mipmap.
       */
      if (level == 1) {
	 x += align(width, align_w);
      }
      else {
	 y += img_height;
      }

      width  = minify(width);
      height = minify(height);
   }
}

static boolean brw_miptree_layout(struct brw_texture *tex)
{
   struct pipe_texture *pt = &tex->base;
   /* XXX: these vary depending on image format:
    */
/*    int align_w = 4; */

   switch (pt->target) {
   case PIPE_TEXTURE_CUBE:
   case PIPE_TEXTURE_3D: {
      unsigned width  = pt->width[0];
      unsigned height = pt->height[0];
      unsigned depth = pt->depth[0];
      unsigned pack_x_pitch, pack_x_nr;
      unsigned pack_y_pitch;
      unsigned level;
      unsigned align_h = 2;
      unsigned align_w = 4;

      tex->total_height = 0;
#if 0
      if (pt->compressed) {
         align_w = intel_compressed_alignment(pt->internal_format);
         pt->pitch = align(width, align_w);
         pack_y_pitch = (height + 3) / 4;
      } else
#endif
      {
         tex->pitch = align(pt->width[0] * pt->cpp, 4) / pt->cpp;
         pack_y_pitch = align(pt->height[0], align_h);
      }

      pack_x_pitch = tex->pitch;
      pack_x_nr = 1;

      for (level = 0; level <= pt->last_level; level++) {
	 unsigned nr_images = pt->target == PIPE_TEXTURE_3D ? depth : 6;
	 int x = 0;
	 int y = 0;
	 uint q, j;

	 intel_miptree_set_level_info(tex, level, nr_images,
				      0, tex->total_height,
				      width, height, depth);

	 for (q = 0; q < nr_images;) {
	    for (j = 0; j < pack_x_nr && q < nr_images; j++, q++) {
	       intel_miptree_set_image_offset(tex, level, q, x, y);
	       x += pack_x_pitch;
	    }

	    x = 0;
	    y += pack_y_pitch;
	 }


	 tex->total_height += y;
	 width  = minify(width);
	 height = minify(height);
	 depth  = minify(depth);

         if (pt->compressed) {
            pack_y_pitch = (height + 3) / 4;

            if (pack_x_pitch > align(width, align_w)) {
               pack_x_pitch = align(width, align_w);
               pack_x_nr <<= 1;
            }
         } else {
            if (pack_x_pitch > 4) {
               pack_x_pitch >>= 1;
               pack_x_nr <<= 1;
               assert(pack_x_pitch * pack_x_nr <= tex->pitch);
            }

            if (pack_y_pitch > 2) {
               pack_y_pitch >>= 1;
               pack_y_pitch = align(pack_y_pitch, align_h);
            }
         }

      }
      break;
   }

   default:
      i945_miptree_layout_2d(tex);
      break;
   }
#if 0
   PRINT("%s: %dx%dx%d - sz 0x%x\n", __FUNCTION__,
       pt->pitch,
       pt->total_height,
       pt->cpp,
       pt->pitch * pt->total_height * pt->cpp );
#endif

   return TRUE;
}


static struct pipe_texture *
brw_texture_create_screen(struct pipe_screen *screen,
                          const struct pipe_texture *templat)
{
   struct pipe_winsys *ws = screen->winsys;
   struct brw_texture *tex = CALLOC_STRUCT(brw_texture);

   if (tex) {
      tex->base = *templat;
      tex->base.refcount = 1;

      if (brw_miptree_layout(tex))
	 tex->buffer = ws->buffer_create(ws, 64,
                                         PIPE_BUFFER_USAGE_PIXEL,
                                         tex->pitch * tex->base.cpp *
                                         tex->total_height);

      if (!tex->buffer) {
	 FREE(tex);
         return NULL;
      }
   }

   return &tex->base;
}


static void
brw_texture_release_screen(struct pipe_screen *screen,
                           struct pipe_texture **pt)
{
   if (!*pt)
      return;

   /*
   DBG("%s %p refcount will be %d\n",
       __FUNCTION__, (void *) *pt, (*pt)->refcount - 1);
   */
   if (--(*pt)->refcount <= 0) {
      struct pipe_winsys *ws = screen->winsys;
      struct brw_texture *tex = (struct brw_texture *)*pt;
      uint i;

      /*
      DBG("%s deleting %p\n", __FUNCTION__, (void *) tex);
      */

      pipe_buffer_reference(ws, &tex->buffer, NULL);

      for (i = 0; i < PIPE_MAX_TEXTURE_LEVELS; i++)
         if (tex->image_offset[i])
            free(tex->image_offset[i]);

      free(tex);
   }
   *pt = NULL;
}


static struct pipe_surface *
brw_get_tex_surface_screen(struct pipe_screen *screen,
                           struct pipe_texture *pt,
                           unsigned face, unsigned level, unsigned zslice)
{
   struct pipe_winsys *ws = screen->winsys;
   struct brw_texture *tex = (struct brw_texture *)pt;
   struct pipe_surface *ps;
   unsigned offset;  /* in bytes */

   offset = tex->level_offset[level];

   if (pt->target == PIPE_TEXTURE_CUBE) {
      offset += tex->image_offset[level][face] * pt->cpp;
   }
   else if (pt->target == PIPE_TEXTURE_3D) {
      offset += tex->image_offset[level][zslice] * pt->cpp;
   }
   else {
      assert(face == 0);
      assert(zslice == 0);
   }

   ps = ws->surface_alloc(ws);
   if (ps) {
      assert(ps->format);
      assert(ps->refcount);
      pipe_buffer_reference(ws, &ps->buffer, tex->buffer);
      ps->format = pt->format;
      ps->cpp = pt->cpp;
      ps->width = pt->width[level];
      ps->height = pt->height[level];
      ps->pitch = tex->pitch;
      ps->offset = offset;
   }
   return ps;
}


void
brw_init_texture_functions(struct brw_context *brw)
{
//   brw->pipe.texture_update = brw_texture_update;
}


void
brw_init_screen_texture_funcs(struct pipe_screen *screen)
{
   screen->texture_create  = brw_texture_create_screen;
   screen->texture_release = brw_texture_release_screen;
   screen->get_tex_surface = brw_get_tex_surface_screen;
}

