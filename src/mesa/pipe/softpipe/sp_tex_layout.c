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
#include "pipe/p_util.h"
#include "sp_tex_layout.h"


/* At the moment, just make softpipe use the same layout for its
 * textures as the i945.  Softpipe needs some sort of texture layout,
 * this one was handy.  May be worthwhile to simplify this code a
 * little.
 */

static unsigned minify( unsigned d )
{
   return MAX2(1, d>>1);
}

static int align(int value, int alignment)
{
   return (value + alignment - 1) & ~(alignment - 1);
}


static void
sp_miptree_set_level_info(struct pipe_mipmap_tree *mt,
                             unsigned level,
                             unsigned nr_images,
                             unsigned x, unsigned y, unsigned w, unsigned h, unsigned d)
{
   assert(level < PIPE_MAX_TEXTURE_LEVELS);

   mt->level[level].width = w;
   mt->level[level].height = h;
   mt->level[level].depth = d;
   mt->level[level].level_offset = (x + y * mt->pitch) * mt->cpp;
   mt->level[level].nr_images = nr_images;

   /*
   DBG("%s level %d size: %d,%d,%d offset %d,%d (0x%x)\n", __FUNCTION__,
       level, w, h, d, x, y, mt->level[level].level_offset);
   */

   /* Not sure when this would happen, but anyway: 
    */
   if (mt->level[level].image_offset) {
      FREE( mt->level[level].image_offset );
      mt->level[level].image_offset = NULL;
   }

   assert(nr_images);
   assert(!mt->level[level].image_offset);

   mt->level[level].image_offset = (unsigned *) MALLOC( nr_images * sizeof(unsigned) );
   mt->level[level].image_offset[0] = 0;
}


static void
sp_miptree_set_image_offset(struct pipe_mipmap_tree *mt,
                               unsigned level, unsigned img, unsigned x, unsigned y)
{
   if (img == 0 && level == 0)
      assert(x == 0 && y == 0);

   assert(img < mt->level[level].nr_images);

   mt->level[level].image_offset[img] = (x + y * mt->pitch);

   /*
   DBG("%s level %d img %d pos %d,%d image_offset %x\n",
       __FUNCTION__, level, img, x, y, mt->level[level].image_offset[img]);
   */
}


static void
sp_miptree_layout_2d( struct pipe_mipmap_tree *mt )
{
   int align_h = 2, align_w = 4;
   unsigned level;
   unsigned x = 0;
   unsigned y = 0;
   unsigned width = mt->width0;
   unsigned height = mt->height0;

   mt->pitch = mt->width0;
   /* XXX FIX THIS:
    * we use alignment=64 bytes in sp_region_alloc(). If we change
    * that, change this too.
    */
   if (mt->pitch < 16)
      mt->pitch = 16;

   /* May need to adjust pitch to accomodate the placement of
    * the 2nd mipmap.  This occurs when the alignment
    * constraints of mipmap placement push the right edge of the
    * 2nd mipmap out past the width of its parent.
    */
   if (mt->first_level != mt->last_level) {
      unsigned mip1_width = align(minify(mt->width0), align_w)
			+ minify(minify(mt->width0));

      if (mip1_width > mt->width0)
	 mt->pitch = mip1_width;
   }

   /* Pitch must be a whole number of dwords, even though we
    * express it in texels.
    */
   mt->pitch = align(mt->pitch * mt->cpp, 4) / mt->cpp;
   mt->total_height = 0;

   for ( level = mt->first_level ; level <= mt->last_level ; level++ ) {
      unsigned img_height;

      sp_miptree_set_level_info(mt, level, 1, x, y, width, height, 1);

      if (mt->compressed)
	 img_height = MAX2(1, height/4);
      else
	 img_height = align(height, align_h);


      /* Because the images are packed better, the final offset
       * might not be the maximal one:
       */
      mt->total_height = MAX2(mt->total_height, y + img_height);

      /* Layout_below: step right after second mipmap.
       */
      if (level == mt->first_level + 1) {
	 x += align(width, align_w);
      }
      else {
	 y += img_height;
      }

      width  = minify(width);
      height = minify(height);
   }
}


static const int initial_offsets[6][2] = {
   {0, 0},
   {0, 2},
   {1, 0},
   {1, 2},
   {1, 1},
   {1, 3}
};

static const int step_offsets[6][2] = {
   {0, 2},
   {0, 2},
   {-1, 2},
   {-1, 2},
   {-1, 1},
   {-1, 1}
};



boolean
softpipe_mipmap_tree_layout(struct pipe_context *pipe, struct pipe_mipmap_tree * mt)
{
   unsigned level;

   switch (mt->target) {
   case PIPE_TEXTURE_CUBE:{
         const unsigned dim = mt->width0;
         unsigned face;
         unsigned lvlWidth = mt->width0, lvlHeight = mt->height0;

         assert(lvlWidth == lvlHeight); /* cubemap images are square */

         /* Depending on the size of the largest images, pitch can be
          * determined either by the old-style packing of cubemap faces,
          * or the final row of 4x4, 2x2 and 1x1 faces below this. 
          */
         if (dim > 32)
            mt->pitch = ((dim * mt->cpp * 2 + 3) & ~3) / mt->cpp;
         else
            mt->pitch = 14 * 8;

         mt->total_height = dim * 4 + 4;

         /* Set all the levels to effectively occupy the whole rectangular region. 
          */
         for (level = mt->first_level; level <= mt->last_level; level++) {
            sp_miptree_set_level_info(mt, level, 6,
                                         0, 0,
                                         lvlWidth, lvlHeight, 1);
	    lvlWidth /= 2;
	    lvlHeight /= 2;
	 }


         for (face = 0; face < 6; face++) {
            unsigned x = initial_offsets[face][0] * dim;
            unsigned y = initial_offsets[face][1] * dim;
            unsigned d = dim;

            if (dim == 4 && face >= 4) {
               y = mt->total_height - 4;
               x = (face - 4) * 8;
            }
            else if (dim < 4 && (face > 0 || mt->first_level > 0)) {
               y = mt->total_height - 4;
               x = face * 8;
            }

            for (level = mt->first_level; level <= mt->last_level; level++) {
               sp_miptree_set_image_offset(mt, level, face, x, y);

               d >>= 1;

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
                     y = mt->total_height - 4;
                     x = (face - 4) * 8;
                     break;
                  }

               case 2:
                  y = mt->total_height - 4;
                  x = 16 + face * 8;
                  break;

               case 1:
                  x += 48;
                  break;

               default:
                  x += step_offsets[face][0] * d;
                  y += step_offsets[face][1] * d;
                  break;
               }
            }
         }
         break;
      }
   case PIPE_TEXTURE_3D:{
         unsigned width = mt->width0;
         unsigned height = mt->height0;
         unsigned depth = mt->depth0;
         unsigned pack_x_pitch, pack_x_nr;
         unsigned pack_y_pitch;
         unsigned level;

         mt->pitch = ((mt->width0 * mt->cpp + 3) & ~3) / mt->cpp;
         mt->total_height = 0;

         pack_y_pitch = MAX2(mt->height0, 2);
         pack_x_pitch = mt->pitch;
         pack_x_nr = 1;

         for (level = mt->first_level; level <= mt->last_level; level++) {
            unsigned nr_images = mt->target == PIPE_TEXTURE_3D ? depth : 6;
            int x = 0;
            int y = 0;
            unsigned q, j;

            sp_miptree_set_level_info(mt, level, nr_images,
                                         0, mt->total_height,
                                         width, height, depth);

            for (q = 0; q < nr_images;) {
               for (j = 0; j < pack_x_nr && q < nr_images; j++, q++) {
                  sp_miptree_set_image_offset(mt, level, q, x, y);
                  x += pack_x_pitch;
               }

               x = 0;
               y += pack_y_pitch;
            }


            mt->total_height += y;

            if (pack_x_pitch > 4) {
               pack_x_pitch >>= 1;
               pack_x_nr <<= 1;
               assert(pack_x_pitch * pack_x_nr <= mt->pitch);
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
         sp_miptree_layout_2d(mt);
         break;
   default:
      assert(0);
      break;
   }

   /*
   DBG("%s: %dx%dx%d - sz 0x%x\n", __FUNCTION__,
       mt->pitch,
       mt->total_height, mt->cpp, mt->pitch * mt->total_height * mt->cpp);
   */

   return TRUE;
}

