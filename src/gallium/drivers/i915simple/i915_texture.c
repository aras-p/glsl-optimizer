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
#include "pipe/p_winsys.h"
#include "util/u_math.h"
#include "util/u_memory.h"

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
   while (value < x)
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
   
   pt->nblocksx[level] = pf_get_nblocksx(&pt->block, w);
   pt->nblocksy[level] = pf_get_nblocksy(&pt->block, h);

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

   tex->image_offset[level][img] = y * tex->stride + x * tex->base.block.size;

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

   if (pt->last_level > 0 || pt->block.size != 4)
      return 0;

   i915_miptree_set_level_info( tex, 0, 1,
                                tex->base.width[0],
                                tex->base.height[0],
                                1 );
   i915_miptree_set_image_offset( tex, 0, 0, 0, 0 );

   if (tex->base.width[0] >= 128) {
      tex->stride = power_of_two(tex->base.nblocksx[0] * pt->block.size);
      tex->total_nblocksy = round_up(tex->base.nblocksy[0], 8);
      tex->tiled = 1;
   } else {
      tex->stride = round_up(tex->base.nblocksx[0] * pt->block.size, 64);
      tex->total_nblocksy = tex->base.nblocksy[0];
   }

   /*
   printf("%s size: %d,%d,%d offset %d,%d (0x%x)\n", __FUNCTION__,
      tex->base.width[0], tex->base.height[0], pt->block.size,
      tex->stride, tex->total_nblocksy, tex->stride * tex->total_nblocksy);
   */

   return 1;
}

static void
i945_miptree_layout_2d( struct i915_texture *tex )
{
   struct pipe_texture *pt = &tex->base;
   const int align_x = 2, align_y = 4;
   unsigned level;
   unsigned x = 0;
   unsigned y = 0;
   unsigned width = pt->width[0];
   unsigned height = pt->height[0];
   unsigned nblocksx = pt->nblocksx[0];
   unsigned nblocksy = pt->nblocksy[0];

#if 0 /* used for tiled display targets */
   if (pt->last_level == 0 && pt->block.size == 4)
      if (i915_displaytarget_layout(tex))
	 return;
#endif

   tex->stride = round_up(pt->nblocksx[0] * pt->block.size, 4);

   /* May need to adjust pitch to accomodate the placement of
    * the 2nd mipmap level.  This occurs when the alignment
    * constraints of mipmap placement push the right edge of the
    * 2nd mipmap level out past the width of its parent.
    */
   if (pt->last_level > 0) {
      unsigned mip1_nblocksx 
	 = align(pf_get_nblocksx(&pt->block, minify(width)), align_x)
         + pf_get_nblocksx(&pt->block, minify(minify(width)));

      if (mip1_nblocksx > nblocksx)
	 tex->stride = mip1_nblocksx * pt->block.size;
   }

   /* Pitch must be a whole number of dwords
    */
   tex->stride = align(tex->stride, 64);
   tex->total_nblocksy = 0;

   for (level = 0; level <= pt->last_level; level++) {
      i915_miptree_set_level_info(tex, level, 1, width, height, 1);
      i915_miptree_set_image_offset(tex, level, 0, x, y);

      nblocksy = align(nblocksy, align_y);

      /* Because the images are packed better, the final offset
       * might not be the maximal one:
       */
      tex->total_nblocksy = MAX2(tex->total_nblocksy, y + nblocksy);

      /* Layout_below: step right after second mipmap level.
       */
      if (level == 1) {
	 x += align(nblocksx, align_x);
      }
      else {
	 y += nblocksy;
      }

      width  = minify(width);
      height = minify(height);
      nblocksx = pf_get_nblocksx(&pt->block, width);
      nblocksy = pf_get_nblocksy(&pt->block, height);
   }
}

static void
i945_miptree_layout_cube(struct i915_texture *tex)
{
   struct pipe_texture *pt = &tex->base;
   unsigned level;

   const unsigned nblocks = pt->nblocksx[0];
   unsigned face;
   unsigned width = pt->width[0];
   unsigned height = pt->height[0];

   /*
   printf("%s %i, %i\n", __FUNCTION__, pt->width[0], pt->height[0]);
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
      tex->stride = round_up(nblocks * pt->block.size * 2, 4);
   else
      tex->stride = 14 * 8 * pt->block.size;

   tex->total_nblocksy = nblocks * 4;

   /* Set all the levels to effectively occupy the whole rectangular region.
   */
   for (level = 0; level <= pt->last_level; level++) {
      i915_miptree_set_level_info(tex, level, 6, width, height, 1);
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
         const unsigned nblocks = pt->nblocksx[0];
         unsigned face;
         unsigned width = pt->width[0], height = pt->height[0];

         assert(width == height); /* cubemap images are square */

         /* double pitch for cube layouts */
         tex->stride = round_up(nblocks * pt->block.size * 2, 4);
         tex->total_nblocksy = nblocks * 4;

         for (level = 0; level <= pt->last_level; level++) {
            i915_miptree_set_level_info(tex, level, 6,
                                         width, height,
                                         1);
            width /= 2;
            height /= 2;
         }

         for (face = 0; face < 6; face++) {
            unsigned x = initial_offsets[face][0] * nblocks;
            unsigned y = initial_offsets[face][1] * nblocks;
            unsigned d = nblocks;

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
         unsigned nblocksx = pt->nblocksx[0];
         unsigned nblocksy = pt->nblocksy[0];
         unsigned stack_nblocksy = 0;

         /* Calculate the size of a single slice. 
          */
         tex->stride = round_up(pt->nblocksx[0] * pt->block.size, 4);

         /* XXX: hardware expects/requires 9 levels at minimum.
          */
         for (level = 0; level <= MAX2(8, pt->last_level);
              level++) {
            i915_miptree_set_level_info(tex, level, depth,
                                        width, height, depth);


            stack_nblocksy += MAX2(2, nblocksy);

            width = minify(width);
            height = minify(height);
            depth = minify(depth);
            nblocksx = pf_get_nblocksx(&pt->block, width);
            nblocksy = pf_get_nblocksy(&pt->block, height);
         }

         /* Fixup depth image_offsets: 
          */
         depth = pt->depth[0];
         for (level = 0; level <= pt->last_level; level++) {
            unsigned i;
            for (i = 0; i < depth; i++) 
               i915_miptree_set_image_offset(tex, level, i,
                                             0, i * stack_nblocksy);

            depth = minify(depth);
         }


         /* Multiply slice size by texture depth for total size.  It's
          * remarkable how wasteful of memory the i915 texture layouts
          * are.  They are largely fixed in the i945.
          */
         tex->total_nblocksy = stack_nblocksy * pt->depth[0];
         break;
      }

   default:{
         unsigned width = pt->width[0];
         unsigned height = pt->height[0];
         unsigned nblocksx = pt->nblocksx[0];
         unsigned nblocksy = pt->nblocksy[0];

         tex->stride = round_up(pt->nblocksx[0] * pt->block.size, 4);
         tex->total_nblocksy = 0;

         for (level = 0; level <= pt->last_level; level++) {
            i915_miptree_set_level_info(tex, level, 1,
                                        width, height, 1);
            i915_miptree_set_image_offset(tex, level, 0,
                                          0, tex->total_nblocksy);

            nblocksy = round_up(MAX2(2, nblocksy), 2);

	    tex->total_nblocksy += nblocksy;

            width = minify(width);
            height = minify(height);
            nblocksx = pf_get_nblocksx(&pt->block, width);
            nblocksy = pf_get_nblocksy(&pt->block, height);
         }
         break;
      }
   }
   /*
   DBG("%s: %dx%dx%d - sz 0x%x\n", __FUNCTION__,
       tex->pitch,
       tex->total_nblocksy, pt->block.size, tex->stride * tex->total_nblocksy);
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
         unsigned nblocksx = pt->nblocksx[0];
         unsigned nblocksy = pt->nblocksy[0];
         unsigned pack_x_pitch, pack_x_nr;
         unsigned pack_y_pitch;

         tex->stride = round_up(pt->nblocksx[0] * pt->block.size, 4);
         tex->total_nblocksy = 0;

         pack_y_pitch = MAX2(pt->nblocksy[0], 2);
         pack_x_pitch = tex->stride / pt->block.size;
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
                  i915_miptree_set_image_offset(tex, level, q, x, y + tex->total_nblocksy);
                  x += pack_x_pitch;
               }

               x = 0;
               y += pack_y_pitch;
            }


            tex->total_nblocksy += y;

            if (pack_x_pitch > 4) {
               pack_x_pitch >>= 1;
               pack_x_nr <<= 1;
               assert(pack_x_pitch * pack_x_nr * pt->block.size <= tex->stride);
            }

            if (pack_y_pitch > 2) {
               pack_y_pitch >>= 1;
            }

            width = minify(width);
            height = minify(height);
            depth = minify(depth);
            nblocksx = pf_get_nblocksx(&pt->block, width);
            nblocksy = pf_get_nblocksy(&pt->block, height);
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
       tex->total_nblocksy, pt->block.size, tex->stride * tex->total_nblocksy);
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
   size_t tex_size;

   if (!tex)
      return NULL;

   tex->base = *templat;
   tex->base.refcount = 1;
   tex->base.screen = screen;

   tex->base.nblocksx[0] = pf_get_nblocksx(&tex->base.block, tex->base.width[0]);
   tex->base.nblocksy[0] = pf_get_nblocksy(&tex->base.block, tex->base.height[0]);
   
   if (i915screen->is_i945) {
      if (!i945_miptree_layout(tex))
	 goto fail;
   } else {
      if (!i915_miptree_layout(tex))
	 goto fail;
   }

   tex_size = tex->stride * tex->total_nblocksy;

   tex->buffer = ws->buffer_create(ws, 64,
				   PIPE_BUFFER_USAGE_PIXEL,
				   tex_size);

   if (!tex->buffer)
      goto fail;

#if 0
   void *ptr = ws->buffer_map(ws, tex->buffer,
      PIPE_BUFFER_USAGE_CPU_WRITE);
   memset(ptr, 0x80, tex_size);
   ws->buffer_unmap(ws, tex->buffer);
#endif

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

      pipe_buffer_reference(screen, &tex->buffer, NULL);

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
      offset = tex->image_offset[level][face];
   }
   else if (pt->target == PIPE_TEXTURE_3D) {
      offset = tex->image_offset[level][zslice];
   }
   else {
      offset = tex->image_offset[level][0];
      assert(face == 0);
      assert(zslice == 0);
   }

   ps = CALLOC_STRUCT(pipe_surface);
   if (ps) {
      ps->refcount = 1;
      ps->winsys = ws;
      pipe_texture_reference(&ps->texture, pt);
      pipe_buffer_reference(screen, &ps->buffer, tex->buffer);
      ps->format = pt->format;
      ps->width = pt->width[level];
      ps->height = pt->height[level];
      ps->block = pt->block;
      ps->nblocksx = pt->nblocksx[level];
      ps->nblocksy = pt->nblocksy[level];
      ps->stride = tex->stride;
      ps->offset = offset;
      ps->usage = flags;
      ps->status = PIPE_SURFACE_STATUS_DEFINED;
   }
   return ps;
}

static struct pipe_texture *
i915_texture_blanket(struct pipe_screen * screen,
                     const struct pipe_texture *base,
                     const unsigned *stride,
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
   tex->base.refcount = 1;
   tex->base.screen = screen;

   tex->stride = stride[0];

   i915_miptree_set_level_info(tex, 0, 1, base->width[0], base->height[0], 1);
   i915_miptree_set_image_offset(tex, 0, 0, 0, 0);

   pipe_buffer_reference(screen, &tex->buffer, buffer);

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
      pipe_buffer_reference(screen, &surf->buffer, NULL);
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
