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
#include "pipe/p_inlines.h"
#include "pipe/p_util.h"
#include "pipe/p_winsys.h"

#include "i915_context.h"
#include "i915_texture.h"
#include "i915_debug.h"
#include "i915_screen.h"

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

static unsigned minify( unsigned d )
{
   return MAX2(1, d>>1);
}

static unsigned
power_of_two(unsigned x)
{
   unsigned value = 1;
   while (value <= x)
      value = value << 1;
   return value;
}

static unsigned
round_up(unsigned n, unsigned multiple)
{
   return (n + multiple - 1) & ~(multiple - 1);
}


/*
 * More advanced helper funcs
 */


static void
i915_miptree_set_level_info(struct i915_texture *tex,
                             unsigned level,
                             unsigned nr_images,
                             unsigned w, unsigned h, unsigned d)
{
   struct pipe_texture *pt = &tex->base;

   assert(level < PIPE_MAX_TEXTURE_LEVELS);

   pt->width[level] = w;
   pt->height[level] = h;
   pt->depth[level] = d;

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
i915_miptree_set_image_offset(struct i915_texture *tex,
			      unsigned level, unsigned img, unsigned x, unsigned y)
{
   if (img == 0 && level == 0)
      assert(x == 0 && y == 0);

   assert(img < tex->nr_images[level]);

   tex->image_offset[level][img] = (x + y * tex->pitch);

   /*
   printf("%s level %d img %d pos %d,%d image_offset %x\n",
       __FUNCTION__, level, img, x, y, tex->image_offset[level][img]);
   */
}


/*
 * Layout functions
 */


/**
 * Special case to deal with display targets.
 */
static boolean
i915_displaytarget_layout(struct i915_texture *tex)
{
   struct pipe_texture *pt = &tex->base;

   if (pt->last_level > 0 || pt->cpp != 4)
      return 0;

   i915_miptree_set_level_info( tex, 0, 1,
                                tex->base.width[0],
                                tex->base.height[0],
                                1 );
   i915_miptree_set_image_offset( tex, 0, 0, 0, 0 );

   if (tex->base.width[0] >= 128) {
      tex->pitch = power_of_two(tex->base.width[0] * pt->cpp) / pt->cpp;
      tex->total_height = round_up(tex->base.height[0], 8);
      tex->tiled = 1;
   } else {
      tex->pitch = round_up(tex->base.width[0], 64 / pt->cpp);
      tex->total_height = tex->base.height[0];
   }

   /*
   printf("%s size: %d,%d,%d offset %d,%d (0x%x)\n", __FUNCTION__,
      tex->base.width[0], tex->base.height[0], pt->cpp,
      tex->pitch, tex->total_height, tex->pitch * tex->total_height * 4);
   */

   return 1;
}

static void
i945_miptree_layout_2d( struct i915_texture *tex )
{
   struct pipe_texture *pt = &tex->base;
   int align_h = 2, align_w = 4;
   unsigned level;
   unsigned x = 0;
   unsigned y = 0;
   unsigned width = pt->width[0];
   unsigned height = pt->height[0];

#if 0 /* used for tiled display targets */
   if (pt->last_level == 0 && pt->cpp == 4)
      if (i915_displaytarget_layout(tex))
	 return;
#endif

   tex->pitch = pt->width[0];

   /* May need to adjust pitch to accomodate the placement of
    * the 2nd mipmap level.  This occurs when the alignment
    * constraints of mipmap placement push the right edge of the
    * 2nd mipmap level out past the width of its parent.
    */
   if (pt->last_level > 0) {
      unsigned mip1_width = align_int(minify(pt->width[0]), align_w)
			+ minify(minify(pt->width[0]));

      if (mip1_width > pt->width[0])
	 tex->pitch = mip1_width;
   }

   /* Pitch must be a whole number of dwords, even though we
    * express it in texels.
    */
   tex->pitch = align_int(tex->pitch * pt->cpp, 64) / pt->cpp;
   tex->total_height = 0;

   for (level = 0; level <= pt->last_level; level++) {
      unsigned img_height;

      i915_miptree_set_level_info(tex, level, 1, width, height, 1);
      i915_miptree_set_image_offset(tex, level, 0, x, y);

      if (pt->compressed)
	 img_height = MAX2(1, height/4);
      else
	 img_height = align_int(height, align_h);


      /* Because the images are packed better, the final offset
       * might not be the maximal one:
       */
      tex->total_height = MAX2(tex->total_height, y + img_height);

      /* Layout_below: step right after second mipmap level.
       */
      if (level == 1) {
	 x += align_int(width, align_w);
      }
      else {
	 y += img_height;
      }

      width  = minify(width);
      height = minify(height);
   }
}

static void
i945_miptree_layout_cube(struct i915_texture *tex)
{
   struct pipe_texture *pt = &tex->base;
   unsigned level;

   const unsigned dim = pt->width[0];
   unsigned face;
   unsigned lvlWidth = pt->width[0], lvlHeight = pt->height[0];

   /*
   printf("%s %i, %i\n", __FUNCTION__, pt->width[0], pt->height[0]);
   */

   assert(lvlWidth == lvlHeight); /* cubemap images are square */

   /*
    * XXX Should only be used for compressed formats. But lets
    * keep this code active just in case.
    *
    * Depending on the size of the largest images, pitch can be
    * determined either by the old-style packing of cubemap faces,
    * or the final row of 4x4, 2x2 and 1x1 faces below this.
    */
   if (dim > 32)
      tex->pitch = ((dim * pt->cpp * 2 + 3) & ~3) / pt->cpp;
   else
      tex->pitch = 14 * 8;

   /*
    * XXX The 4 is only needed for compressed formats. See above.
    */
   tex->total_height = dim * 4 + 4;

   /* Set all the levels to effectively occupy the whole rectangular region.
   */
   for (level = 0; level <= pt->last_level; level++) {
      i915_miptree_set_level_info(tex, level, 6, lvlWidth, lvlHeight, 1);
      lvlWidth /= 2;
      lvlHeight /= 2;
   }

   for (face = 0; face < 6; face++) {
      unsigned x = initial_offsets[face][0] * dim;
      unsigned y = initial_offsets[face][1] * dim;
      unsigned d = dim;

#if 0 /* Fix and enable this code for compressed formats */
      if (dim == 4 && face >= 4) {
         y = tex->total_height - 4;
         x = (face - 4) * 8;
      }
      else if (dim < 4 && (face > 0)) {
         y = tex->total_height - 4;
         x = face * 8;
      }
#endif

      for (level = 0; level <= pt->last_level; level++) {
         i915_miptree_set_image_offset(tex, level, face, x, y);

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
i915_miptree_layout(struct i915_texture * tex)
{
   struct pipe_texture *pt = &tex->base;
   unsigned level;

   switch (pt->target) {
   case PIPE_TEXTURE_CUBE: {
         const unsigned dim = pt->width[0];
         unsigned face;
         unsigned lvlWidth = pt->width[0], lvlHeight = pt->height[0];

         assert(lvlWidth == lvlHeight); /* cubemap images are square */

         /* double pitch for cube layouts */
         tex->pitch = ((dim * pt->cpp * 2 + 3) & ~3) / pt->cpp;
         tex->total_height = dim * 4;

         for (level = 0; level <= pt->last_level; level++) {
            i915_miptree_set_level_info(tex, level, 6,
                                         lvlWidth, lvlHeight,
                                         1);
            lvlWidth /= 2;
            lvlHeight /= 2;
         }

         for (face = 0; face < 6; face++) {
            unsigned x = initial_offsets[face][0] * dim;
            unsigned y = initial_offsets[face][1] * dim;
            unsigned d = dim;

            for (level = 0; level <= pt->last_level; level++) {
               i915_miptree_set_image_offset(tex, level, face, x, y);
               d >>= 1;
               x += step_offsets[face][0] * d;
               y += step_offsets[face][1] * d;
            }
         }
         break;
      }
   case PIPE_TEXTURE_3D:{
         unsigned width = pt->width[0];
         unsigned height = pt->height[0];
         unsigned depth = pt->depth[0];
         unsigned stack_height = 0;

         /* Calculate the size of a single slice. 
          */
         tex->pitch = ((pt->width[0] * pt->cpp + 3) & ~3) / pt->cpp;

         /* XXX: hardware expects/requires 9 levels at minimum.
          */
         for (level = 0; level <= MAX2(8, pt->last_level);
              level++) {
            i915_miptree_set_level_info(tex, level, depth,
                                         width, height, depth);


            stack_height += MAX2(2, height);

            width = minify(width);
            height = minify(height);
            depth = minify(depth);
         }

         /* Fixup depth image_offsets: 
          */
         depth = pt->depth[0];
         for (level = 0; level <= pt->last_level; level++) {
            unsigned i;
            for (i = 0; i < depth; i++) 
               i915_miptree_set_image_offset(tex, level, i,
                                              0, i * stack_height);

            depth = minify(depth);
         }


         /* Multiply slice size by texture depth for total size.  It's
          * remarkable how wasteful of memory the i915 texture layouts
          * are.  They are largely fixed in the i945.
          */
         tex->total_height = stack_height * pt->depth[0];
         break;
      }

   default:{
         unsigned width = pt->width[0];
         unsigned height = pt->height[0];
	 unsigned img_height;

         tex->pitch = ((pt->width[0] * pt->cpp + 3) & ~3) / pt->cpp;
         tex->total_height = 0;

         for (level = 0; level <= pt->last_level; level++) {
            i915_miptree_set_level_info(tex, level, 1,
                                         width, height, 1);
            i915_miptree_set_image_offset(tex, level, 0,
                                           0, tex->total_height);

            if (pt->compressed)
               img_height = MAX2(1, height / 4);
            else
               img_height = (MAX2(2, height) + 1) & ~1;

	    tex->total_height += img_height;

            width = minify(width);
            height = minify(height);
         }
         break;
      }
   }
   /*
   DBG("%s: %dx%dx%d - sz 0x%x\n", __FUNCTION__,
       tex->pitch,
       tex->total_height, pt->cpp, tex->pitch * tex->total_height * pt->cpp);
   */

   return TRUE;
}


static boolean
i945_miptree_layout(struct i915_texture * tex)
{
   struct pipe_texture *pt = &tex->base;
   unsigned level;

   switch (pt->target) {
   case PIPE_TEXTURE_CUBE:
      i945_miptree_layout_cube(tex);
      break;
   case PIPE_TEXTURE_3D:{
         unsigned width = pt->width[0];
         unsigned height = pt->height[0];
         unsigned depth = pt->depth[0];
         unsigned pack_x_pitch, pack_x_nr;
         unsigned pack_y_pitch;

         tex->pitch = ((pt->width[0] * pt->cpp + 3) & ~3) / pt->cpp;
         tex->total_height = 0;

         pack_y_pitch = MAX2(pt->height[0], 2);
         pack_x_pitch = tex->pitch;
         pack_x_nr = 1;

         for (level = 0; level <= pt->last_level; level++) {
            unsigned nr_images = pt->target == PIPE_TEXTURE_3D ? depth : 6;
            int x = 0;
            int y = 0;
            unsigned q, j;

            i915_miptree_set_level_info(tex, level, nr_images,
                                         width, height, depth);

            for (q = 0; q < nr_images;) {
               for (j = 0; j < pack_x_nr && q < nr_images; j++, q++) {
                  i915_miptree_set_image_offset(tex, level, q, x, y + tex->total_height);
                  x += pack_x_pitch;
               }

               x = 0;
               y += pack_y_pitch;
            }


            tex->total_height += y;

            if (pack_x_pitch > 4) {
               pack_x_pitch >>= 1;
               pack_x_nr <<= 1;
               assert(pack_x_pitch * pack_x_nr <= tex->pitch);
            }

            if (pack_y_pitch > 2) {
               pack_y_pitch >>= 1;
            }

            width = minify(width);
            height = minify(height);
            depth = minify(depth);
         }
         break;
      }

   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_2D:
//   case PIPE_TEXTURE_RECTANGLE:
         i945_miptree_layout_2d(tex);
         break;
   default:
      assert(0);
      return FALSE;
   }

   /*
   DBG("%s: %dx%dx%d - sz 0x%x\n", __FUNCTION__,
       tex->pitch,
       tex->total_height, pt->cpp, tex->pitch * tex->total_height * pt->cpp);
   */

   return TRUE;
}


static struct pipe_texture *
i915_texture_create(struct pipe_screen *screen,
                    const struct pipe_texture *templat)
{
   struct i915_screen *i915screen = i915_screen(screen);
   struct pipe_winsys *ws = screen->winsys;
   struct i915_texture *tex = CALLOC_STRUCT(i915_texture);

   if (!tex)
      return NULL;

   tex->base = *templat;
   tex->base.refcount = 1;
   tex->base.screen = screen;

   if (i915screen->is_i945) {
      if (!i945_miptree_layout(tex))
	 goto fail;
   } else {
      if (!i915_miptree_layout(tex))
	 goto fail;
   }

   tex->buffer = ws->buffer_create(ws, 64,
				   PIPE_BUFFER_USAGE_PIXEL,
				   tex->pitch * tex->base.cpp *
				   tex->total_height);

   if (!tex->buffer)
      goto fail;

   return &tex->base;

fail:
   FREE(tex);
   return NULL;
}


static void
i915_texture_release(struct pipe_screen *screen,
                     struct pipe_texture **pt)
{
   if (!*pt)
      return;

   /*
   DBG("%s %p refcount will be %d\n",
       __FUNCTION__, (void *) *pt, (*pt)->refcount - 1);
   */
   if (--(*pt)->refcount <= 0) {
      struct i915_texture *tex = (struct i915_texture *)*pt;
      uint i;

      /*
      DBG("%s deleting %p\n", __FUNCTION__, (void *) tex);
      */

      pipe_buffer_reference(screen->winsys, &tex->buffer, NULL);

      for (i = 0; i < PIPE_MAX_TEXTURE_LEVELS; i++)
         if (tex->image_offset[i])
            FREE(tex->image_offset[i]);

      FREE(tex);
   }
   *pt = NULL;
}

static struct pipe_surface *
i915_get_tex_surface(struct pipe_screen *screen,
                     struct pipe_texture *pt,
                     unsigned face, unsigned level, unsigned zslice,
                     unsigned flags)
{
   struct i915_texture *tex = (struct i915_texture *)pt;
   struct pipe_winsys *ws = screen->winsys;
   struct pipe_surface *ps;
   unsigned offset;  /* in bytes */

   if (pt->target == PIPE_TEXTURE_CUBE) {
      offset = tex->image_offset[level][face] * pt->cpp;
   }
   else if (pt->target == PIPE_TEXTURE_3D) {
      offset = tex->image_offset[level][zslice] * pt->cpp;
   }
   else {
      offset = tex->image_offset[level][0] * pt->cpp;
      assert(face == 0);
      assert(zslice == 0);
   }

   ps = CALLOC_STRUCT(pipe_surface);
   if (ps) {
      ps->refcount = 1;
      ps->winsys = ws;
      pipe_texture_reference(&ps->texture, pt);
      pipe_buffer_reference(ws, &ps->buffer, tex->buffer);
      ps->format = pt->format;
      ps->cpp = pt->cpp;
      ps->width = pt->width[level];
      ps->height = pt->height[level];
      ps->pitch = tex->pitch;
      ps->offset = offset;
      ps->usage = flags;
      ps->status = PIPE_SURFACE_STATUS_DEFINED;
   }
   return ps;
}

static struct pipe_texture *
i915_texture_blanket(struct pipe_screen * screen,
                     const struct pipe_texture *base,
                     const unsigned *pitch,
                     struct pipe_buffer *buffer)
{
   struct i915_texture *tex;
   assert(screen);

   /* Only supports one type */
   if (base->target != PIPE_TEXTURE_2D ||
       base->last_level != 0 ||
       base->depth[0] != 1) {
      return NULL;
   }

   tex = CALLOC_STRUCT(i915_texture);
   if (!tex)
      return NULL;

   tex->base = *base;

   tex->pitch = pitch[0];

   i915_miptree_set_level_info(tex, 0, 1, base->width[0], base->height[0], 1);
   i915_miptree_set_image_offset(tex, 0, 0, 0, 0);

   pipe_buffer_reference(screen->winsys, &tex->buffer, buffer);

   return &tex->base;
}

void
i915_init_texture_functions(struct i915_context *i915)
{
//   i915->pipe.texture_update = i915_texture_update;
}

static void
i915_tex_surface_release(struct pipe_screen *screen,
                         struct pipe_surface **surface)
{
   struct pipe_surface *surf = *surface;

   if (--surf->refcount == 0) {

      /* This really should not be possible, but it's actually
       * happening quite a bit...  Will fix.
       */
      if (surf->status == PIPE_SURFACE_STATUS_CLEAR) {
         debug_printf("XXX destroying a surface with pending clears...\n");
         assert(0);
      }

      pipe_texture_reference(&surf->texture, NULL);
      pipe_buffer_reference(screen->winsys, &surf->buffer, NULL);
      FREE(surf);
   }

   *surface = NULL;
}

void
i915_init_screen_texture_functions(struct pipe_screen *screen)
{
   screen->texture_create = i915_texture_create;
   screen->texture_release = i915_texture_release;
   screen->get_tex_surface = i915_get_tex_surface;
   screen->texture_blanket = i915_texture_blanket;
   screen->tex_surface_release = i915_tex_surface_release;
}
