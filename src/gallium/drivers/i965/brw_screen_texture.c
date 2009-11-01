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

#define FILE_DEBUG_FLAG DEBUG_MIPTREE



static GLuint translate_tex_target( unsigned target )
{
   switch (target) {
   case PIPE_TEXTURE_1D: 
      return BRW_SURFACE_1D;

   case PIPE_TEXTURE_2D: 
      return BRW_SURFACE_2D;

   case PIPE_TEXTURE_3D: 
      return BRW_SURFACE_3D;

   case PIPE_TEXTURE_CUBE:
      return BRW_SURFACE_CUBE;

   default: 
      assert(0); 
      return BRW_SURFACE_1D;
   }
}


static GLuint translate_tex_format( enum pipe_format pf )
{
   switch( pf ) {
   case PIPE_FORMAT_L8_UNORM:
      return BRW_SURFACEFORMAT_L8_UNORM;

   case PIPE_FORMAT_I8_UNORM:
      return BRW_SURFACEFORMAT_I8_UNORM;

   case PIPE_FORMAT_A8_UNORM:
      return BRW_SURFACEFORMAT_A8_UNORM; 

   case PIPE_FORMAT_A8L8_UNORM:
      return BRW_SURFACEFORMAT_L8A8_UNORM;

   case PIPE_FORMAT_A8R8G8B8_UNORM: /* XXX */
   case PIPE_FORMAT_B8G8R8A8_UNORM: /* XXX */
   case PIPE_FORMAT_R8G8B8A8_UNORM: /* XXX */
      return BRW_SURFACEFORMAT_B8G8R8A8_UNORM;

   case PIPE_FORMAT_R8G8B8X8_UNORM:
      return BRW_SURFACEFORMAT_R8G8B8X8_UNORM;

   case PIPE_FORMAT_R5G6B5_UNORM:
      return BRW_SURFACEFORMAT_B5G6R5_UNORM;

   case PIPE_FORMAT_A1R5G5B5_UNORM:
      return BRW_SURFACEFORMAT_B5G5R5A1_UNORM;

   case PIPE_FORMAT_A4R4G4B4_UNORM:
      return BRW_SURFACEFORMAT_B4G4R4A4_UNORM;


   case PIPE_FORMAT_L16_UNORM:
      return BRW_SURFACEFORMAT_L16_UNORM;

      /* XXX: Z texturing: 
   case PIPE_FORMAT_I16_UNORM:
      return BRW_SURFACEFORMAT_I16_UNORM;
       */

      /* XXX: Z texturing:
   case PIPE_FORMAT_A16_UNORM:
      return BRW_SURFACEFORMAT_A16_UNORM; 
      */

   case PIPE_FORMAT_YCBCR_REV:
      return BRW_SURFACEFORMAT_YCRCB_NORMAL;

   case PIPE_FORMAT_YCBCR:
      return BRW_SURFACEFORMAT_YCRCB_SWAPUVY;

      /* XXX: Add FXT to gallium?
   case PIPE_FORMAT_FXT1_RGBA:
      return BRW_SURFACEFORMAT_FXT1;
      */

   case PIPE_FORMAT_DXT1_RGB:
       return BRW_SURFACEFORMAT_DXT1_RGB;

   case PIPE_FORMAT_DXT1_RGBA:
       return BRW_SURFACEFORMAT_BC1_UNORM;
       
   case PIPE_FORMAT_DXT3_RGBA:
       return BRW_SURFACEFORMAT_BC2_UNORM;
       
   case PIPE_FORMAT_DXT5_RGBA:
       return BRW_SURFACEFORMAT_BC3_UNORM;

   case PIPE_FORMAT_R8G8B8A8_SRGB:
      return BRW_SURFACEFORMAT_B8G8R8A8_UNORM_SRGB;

   case PIPE_FORMAT_A8L8_SRGB:
      return BRW_SURFACEFORMAT_L8A8_UNORM_SRGB;

   case PIPE_FORMAT_L8_SRGB:
      return BRW_SURFACEFORMAT_L8_UNORM_SRGB;

   case PIPE_FORMAT_DXT1_SRGB:
      return BRW_SURFACEFORMAT_BC1_UNORM_SRGB;

      /* XXX: which pipe depth formats does i965 suppport
       */
   case PIPE_FORMAT_S8Z24_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM:
   case PIPE_FORMAT_Z24S8_UNORM:
   case PIPE_FORMAT_Z24X8_UNORM:
         return BRW_SURFACEFORMAT_I24X8_UNORM;

#if 0
      /* XXX: these different surface formats don't seem to
       * make any difference for shadow sampler/compares.
       */
      if (depth_mode == GL_INTENSITY) 
         return BRW_SURFACEFORMAT_I24X8_UNORM;
      else if (depth_mode == GL_ALPHA)
         return BRW_SURFACEFORMAT_A24X8_UNORM;
      else
         return BRW_SURFACEFORMAT_L24X8_UNORM;
#endif

      /* XXX: presumably for bump mapping.  Add this to mesa state
       * tracker?
       */
   case PIPE_FORMAT_R8G8_SNORM:
      return BRW_SURFACEFORMAT_R8G8_SNORM;

   case PIPE_FORMAT_R8G8B8A8_SNORM:
      return BRW_SURFACEFORMAT_R8G8B8A8_SNORM;

   default:
      assert(0);
      return 0;
   }
}

static void
brw_set_surface_tiling(struct brw_surface_state *surf, uint32_t tiling)
{
   switch (tiling) {
   case BRW_TILING_NONE:
      surf->ss3.tiled_surface = 0;
      surf->ss3.tile_walk = 0;
      break;
   case BRW_TILING_X:
      surf->ss3.tiled_surface = 1;
      surf->ss3.tile_walk = BRW_TILEWALK_XMAJOR;
      break;
   case BRW_TILING_Y:
      surf->ss3.tiled_surface = 1;
      surf->ss3.tile_walk = BRW_TILEWALK_YMAJOR;
      break;
   }
}




static void brw_create_texture( struct pipe_screen *screen,
				const pipe_texture *templ )

{  

   tex->compressed = pf_is_compressed(tex->base.format);

   if (intel->use_texture_tiling && compress_byte == 0 &&
       intel->intelScreen->kernel_exec_fencing) {
      if (IS_965(intel->intelScreen->deviceID) &&
	  (base_format == GL_DEPTH_COMPONENT ||
	   base_format == GL_DEPTH_STENCIL_EXT))
	 tiling = I915_TILING_Y;
      else
	 tiling = I915_TILING_X;
   } else
      tiling = I915_TILING_NONE;



   key.format = tex->base.format;
   key.pitch = tex->pitch;
   key.depth = tex->base.depth[0];
   key.bo = tex->buffer;
   key.offset = 0;

   key.target = tex->brw_target;	/* translated to BRW enum */
   //key.depthmode = 0; /* XXX: add this to gallium? or handle in the state tracker? */
   key.last_level = tex->base.last_level;
   key.width = tex->base.depth[0];
   key.height = tex->base.height[0];
   key.cpp = tex->cpp;
   key.tiling = tex->tiling;



   surf.ss0.mipmap_layout_mode = BRW_SURFACE_MIPMAPLAYOUT_BELOW;
   surf.ss0.surface_type = translate_tex_target(key->target);
   surf.ss0.surface_format = translate_tex_format(key->format /* , key->depthmode */ );

   /* This is ok for all textures with channel width 8bit or less:
    */
/*    surf.ss0.data_return_format = BRW_SURFACERETURNFORMAT_S1; */
   assert(key->bo);
   surf.ss1.base_addr = key->bo->offset; /* reloc */
   surf.ss2.mip_count = key->last_level;
   surf.ss2.width = key->width - 1;
   surf.ss2.height = key->height - 1;
   brw_set_surface_tiling(&surf, key->tiling);
   surf.ss3.pitch = (key->pitch * key->cpp) - 1;
   surf.ss3.depth = key->depth - 1;

   surf.ss4.min_lod = 0;
 
   if (key->target == PIPE_TEXTURE_CUBE) {
      surf.ss0.cube_pos_x = 1;
      surf.ss0.cube_pos_y = 1;
      surf.ss0.cube_pos_z = 1;
      surf.ss0.cube_neg_x = 1;
      surf.ss0.cube_neg_y = 1;
      surf.ss0.cube_neg_z = 1;
   }

}





