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

#include "pipe/p_format.h"

#include "util/u_math.h"
#include "util/u_memory.h"

#include "brw_screen.h"
#include "brw_debug.h"
#include "brw_winsys.h"

/* Code to layout images in a mipmap tree for i965.
 */

static int 
brw_tex_pitch_align (struct brw_texture *tex,
		     int pitch)
{
   if (!tex->compressed) {
      int pitch_align;

      switch (tex->tiling) {
      case BRW_TILING_X:
	 pitch_align = 512;
	 break;
      case BRW_TILING_Y:
	 pitch_align = 128;
	 break;
      default:
	 /* XXX: Untiled pitch alignment of 64 bytes for now to allow
	  * render-to-texture to work in all cases. This should
	  * probably be replaced at some point by some scheme to only
	  * do this when really necessary, for example standalone
	  * render target views.
	  */
	 pitch_align = 64;
	 break;
      }

      pitch = align(pitch * tex->cpp, pitch_align);
      pitch /= tex->cpp;
   }

   return pitch;
}


static void 
brw_tex_alignment_unit(enum pipe_format pf, 
		       GLuint *w, GLuint *h)
{
    switch (pf) {
    case PIPE_FORMAT_DXT1_RGB:
    case PIPE_FORMAT_DXT1_RGBA:
    case PIPE_FORMAT_DXT3_RGBA:
    case PIPE_FORMAT_DXT5_RGBA:
    case PIPE_FORMAT_DXT1_SRGB:
    case PIPE_FORMAT_DXT1_SRGBA:
    case PIPE_FORMAT_DXT3_SRGBA:
    case PIPE_FORMAT_DXT5_SRGBA:
        *w = 4;
        *h = 4;
        break;

    default:
        *w = 4;
        *h = 2;
        break;
    }
}


static void 
brw_tex_set_level_info(struct brw_texture *tex,
		       GLuint level,
		       GLuint nr_images,
		       GLuint x, GLuint y,
		       GLuint w, GLuint h, GLuint d)
{

   if (BRW_DEBUG & DEBUG_TEXTURE)
      debug_printf("%s level %d size: %d,%d,%d offset %d,%d (0x%x)\n", __FUNCTION__,
		   level, w, h, d, x, y, tex->level_offset[level]);

   assert(tex->image_offset[level] == NULL);
   assert(nr_images >= 1);

   tex->level_offset[level] = (x + y * tex->pitch) * tex->cpp;
   tex->nr_images[level] = nr_images;

   tex->image_offset[level] = MALLOC(nr_images * sizeof(GLuint));
   tex->image_offset[level][0] = 0;
}


static void
brw_tex_set_image_offset(struct brw_texture *tex,
			 GLuint level, GLuint img,
			 GLuint x, GLuint y, 
			 GLuint offset)
{
   assert((x == 0 && y == 0) || img != 0 || level != 0);
   assert(img < tex->nr_images[level]);

   if (BRW_DEBUG & DEBUG_TEXTURE)
      debug_printf("%s level %d img %d pos %d,%d image_offset %x\n",
		   __FUNCTION__, level, img, x, y, 
		   tex->image_offset[level][img]);

   tex->image_offset[level][img] = (x + y * tex->pitch) * tex->cpp + offset;
}



static void brw_layout_2d( struct brw_texture *tex )
{
   GLuint align_h = 2, align_w = 4;
   GLuint level;
   GLuint x = 0;
   GLuint y = 0;
   GLuint width = tex->base.width0;
   GLuint height = tex->base.height0;

   tex->pitch = tex->base.width0;
   brw_tex_alignment_unit(tex->base.format, &align_w, &align_h);

   if (tex->compressed) {
       tex->pitch = align(tex->base.width0, align_w);
   }

   /* May need to adjust pitch to accomodate the placement of
    * the 2nd mipmap.  This occurs when the alignment
    * constraints of mipmap placement push the right edge of the
    * 2nd mipmap out past the width of its parent.
    */
   if (tex->base.last_level > 0) {
       GLuint mip1_width;

       if (tex->compressed) {
          mip1_width = (align(u_minify(tex->base.width0, 1), align_w) + 
                        align(u_minify(tex->base.width0, 2), align_w));
       } else {
          mip1_width = (align(u_minify(tex->base.width0, 1), align_w) + 
                        u_minify(tex->base.width0, 2));
       }

       if (mip1_width > tex->pitch) {
           tex->pitch = mip1_width;
       }
   }

   /* Pitch must be a whole number of dwords, even though we
    * express it in texels.
    */
   tex->pitch = brw_tex_pitch_align (tex, tex->pitch);
   tex->total_height = 0;

   for ( level = 0 ; level <= tex->base.last_level ; level++ ) {
      GLuint img_height;

      brw_tex_set_level_info(tex, level, 1, x, y, width, height, 1);

      if (tex->compressed)
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

      width  = u_minify(width, 1);
      height = u_minify(height, 1);
   }
}


static boolean 
brw_layout_cubemap_idgng( struct brw_texture *tex )
{
   GLuint align_h = 2, align_w = 4;
   GLuint level;
   GLuint x = 0;
   GLuint y = 0;
   GLuint width = tex->base.width0;
   GLuint height = tex->base.height0;
   GLuint qpitch = 0;
   GLuint y_pitch = 0;

   tex->pitch = tex->base.width0;
   brw_tex_alignment_unit(tex->base.format, &align_w, &align_h);
   y_pitch = align(height, align_h);

   if (tex->compressed) {
      tex->pitch = align(tex->base.width0, align_w);
   }

   if (tex->base.last_level != 0) {
      GLuint mip1_width;

      if (tex->compressed) {
	 mip1_width = (align(u_minify(tex->base.width0, 1), align_w) +
		       align(u_minify(tex->base.width0, 2), align_w));
      } else {
	 mip1_width = (align(u_minify(tex->base.width0, 1), align_w) +
		       u_minify(tex->base.width0, 2));
      }

      if (mip1_width > tex->pitch) {
	 tex->pitch = mip1_width;
      }
   }

   tex->pitch = brw_tex_pitch_align(tex, tex->pitch);

   if (tex->compressed) {
      qpitch = ((y_pitch + 
		 align(u_minify(y_pitch, 1), align_h) +
		 11 * align_h) / 4) * tex->pitch * tex->cpp;

      tex->total_height = ((y_pitch + 
			    align(u_minify(y_pitch, 1), align_h) + 
			    11 * align_h) / 4) * 6;
   } else {
      qpitch = (y_pitch + 
		align(u_minify(y_pitch, 1), align_h) + 
		11 * align_h) * tex->pitch * tex->cpp;

      tex->total_height = (y_pitch +
			   align(u_minify(y_pitch, 1), align_h) +
			   11 * align_h) * 6;
   }

   for (level = 0; level <= tex->base.last_level; level++) {
      GLuint img_height;
      GLuint nr_images = 6;
      GLuint q = 0;

      brw_tex_set_level_info(tex, level, nr_images, x, y, width, height, 1);

      for (q = 0; q < nr_images; q++)
	 brw_tex_set_image_offset(tex, level, q, x, y, q * qpitch);

      if (tex->compressed)
	 img_height = MAX2(1, height/4);
      else
	 img_height = align(height, align_h);

      if (level == 1) {
	 x += align(width, align_w);
      }
      else {
	 y += img_height;
      }

      width  = u_minify(width, 1);
      height = u_minify(height, 1);
   }

   return TRUE;
}


static boolean
brw_layout_3d_cube( struct brw_texture *tex )
{
   GLuint width  = tex->base.width0;
   GLuint height = tex->base.height0;
   GLuint depth = tex->base.depth0;
   GLuint pack_x_pitch, pack_x_nr;
   GLuint pack_y_pitch;
   GLuint level;
   GLuint align_h = 2;
   GLuint align_w = 4;

   tex->total_height = 0;
   brw_tex_alignment_unit(tex->base.format, &align_w, &align_h);

   if (tex->compressed) {
      tex->pitch = align(width, align_w);
      pack_y_pitch = (height + 3) / 4;
   } else {
      tex->pitch = brw_tex_pitch_align(tex, tex->base.width0);
      pack_y_pitch = align(tex->base.height0, align_h);
   }

   pack_x_pitch = width;
   pack_x_nr = 1;

   for (level = 0 ; level <= tex->base.last_level ; level++) {
      GLuint nr_images = tex->base.target == PIPE_TEXTURE_3D ? depth : 6;
      GLint x = 0;
      GLint y = 0;
      GLint q, j;

      brw_tex_set_level_info(tex, level, nr_images,
				   0, tex->total_height,
				   width, height, depth);

      for (q = 0; q < nr_images;) {
	 for (j = 0; j < pack_x_nr && q < nr_images; j++, q++) {
	    brw_tex_set_image_offset(tex, level, q, x, y, 0);
	    x += pack_x_pitch;
	 }

	 x = 0;
	 y += pack_y_pitch;
      }


      tex->total_height += y;
      width  = u_minify(width, 1);
      height = u_minify(height, 1);
      depth  = u_minify(depth, 1);

      if (tex->compressed) {
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

   /* The 965's sampler lays cachelines out according to how accesses
    * in the texture surfaces run, so they may be "vertical" through
    * memory.  As a result, the docs say in Surface Padding Requirements:
    * Sampling Engine Surfaces that two extra rows of padding are required.
    */
   if (tex->base.target == PIPE_TEXTURE_CUBE)
      tex->total_height += 2;

   return TRUE;
}



GLboolean brw_texture_layout(struct brw_screen *brw_screen,
			     struct brw_texture *tex )
{
   switch (tex->base.target) {
   case PIPE_TEXTURE_CUBE:
      if (brw_screen->chipset.is_igdng)
	 brw_layout_cubemap_idgng( tex );
      else
	 brw_layout_3d_cube( tex );
      break;
	    
   case PIPE_TEXTURE_3D:
      brw_layout_3d_cube( tex );
      break;

   default:
      brw_layout_2d( tex );
      break;
   }

   if (BRW_DEBUG & DEBUG_TEXTURE)
      debug_printf("%s: %dx%dx%d - sz 0x%x\n", __FUNCTION__,
		   tex->pitch,
		   tex->total_height,
		   tex->cpp,
		   tex->pitch * tex->total_height * tex->cpp );

   return GL_TRUE;
}
