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

#include "util/u_memory.h"
#include "util/u_simple_list.h"

#include "brw_screen.h"
#include "brw_defines.h"
#include "brw_structs.h"
#include "brw_winsys.h"



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
      return BRW_SURFACEFORMAT_INVALID;
   }
}





static struct pipe_texture *brw_texture_create( struct pipe_screen *screen,
						const struct pipe_texture *templ )

{  
   struct brw_screen *bscreen = brw_screen(screen);
   struct brw_texture *tex;
   enum brw_buffer_type buffer_type;
   enum pipe_error ret;
   
   tex = CALLOC_STRUCT(brw_texture);
   if (tex == NULL)
      return NULL;

   memcpy(&tex->base, templ, sizeof *templ);
   pipe_reference_init(&tex->base.reference, 1);
   tex->base.screen = screen;

   /* XXX: compressed textures need special treatment here
    */
   tex->cpp = pf_get_size(tex->base.format);
   tex->compressed = pf_is_compressed(tex->base.format);

   make_empty_list(&tex->views[0]);
   make_empty_list(&tex->views[1]);

   /* XXX: No tiling with compressed textures??
    */
   if (tex->compressed == 0 
       /* && bscreen->use_texture_tiling */
       /* && bscreen->kernel_exec_fencing */) 
   {
      if (1)
         tex->tiling = BRW_TILING_NONE;
      else if (bscreen->chipset.is_965 &&
               pf_is_depth_or_stencil(templ->format))
	 tex->tiling = BRW_TILING_Y;
      else
	 tex->tiling = BRW_TILING_X;
   } 
   else {
      tex->tiling = BRW_TILING_NONE;
   }




   if (!brw_texture_layout( bscreen, tex ))
      goto fail;

   
   if (templ->tex_usage & (PIPE_TEXTURE_USAGE_DISPLAY_TARGET |
                           PIPE_TEXTURE_USAGE_PRIMARY)) {
      buffer_type = BRW_BUFFER_TYPE_SCANOUT;
   }
   else {
      buffer_type = BRW_BUFFER_TYPE_TEXTURE;
   }

   ret = bscreen->sws->bo_alloc( bscreen->sws,
                                 buffer_type,
                                 tex->pitch * tex->total_height * tex->cpp,
                                 64,
                                 &tex->bo );
   if (ret)
      goto fail;

   tex->ss.ss0.mipmap_layout_mode = BRW_SURFACE_MIPMAPLAYOUT_BELOW;
   tex->ss.ss0.surface_type = translate_tex_target(tex->base.target);
   tex->ss.ss0.surface_format = translate_tex_format(tex->base.format);
   assert(tex->ss.ss0.surface_format != BRW_SURFACEFORMAT_INVALID);

   /* This is ok for all textures with channel width 8bit or less:
    */
/*    tex->ss.ss0.data_return_format = BRW_SURFACERETURNFORMAT_S1; */


   /* XXX: what happens when tex->bo->offset changes???
    */
   tex->ss.ss1.base_addr = 0; /* reloc */
   tex->ss.ss2.mip_count = tex->base.last_level;
   tex->ss.ss2.width = tex->base.width[0] - 1;
   tex->ss.ss2.height = tex->base.height[0] - 1;

   switch (tex->tiling) {
   case BRW_TILING_NONE:
      tex->ss.ss3.tiled_surface = 0;
      tex->ss.ss3.tile_walk = 0;
      break;
   case BRW_TILING_X:
      tex->ss.ss3.tiled_surface = 1;
      tex->ss.ss3.tile_walk = BRW_TILEWALK_XMAJOR;
      break;
   case BRW_TILING_Y:
      tex->ss.ss3.tiled_surface = 1;
      tex->ss.ss3.tile_walk = BRW_TILEWALK_YMAJOR;
      break;
   }

   tex->ss.ss3.pitch = (tex->pitch * tex->cpp) - 1;
   tex->ss.ss3.depth = tex->base.depth[0] - 1;

   tex->ss.ss4.min_lod = 0;
 
   if (tex->base.target == PIPE_TEXTURE_CUBE) {
      tex->ss.ss0.cube_pos_x = 1;
      tex->ss.ss0.cube_pos_y = 1;
      tex->ss.ss0.cube_pos_z = 1;
      tex->ss.ss0.cube_neg_x = 1;
      tex->ss.ss0.cube_neg_y = 1;
      tex->ss.ss0.cube_neg_z = 1;
   }

   return &tex->base;

fail:
   bo_reference(&tex->bo, NULL);
   FREE(tex);
   return NULL;
}



static struct pipe_texture *brw_texture_blanket(struct pipe_screen *screen,
						const struct pipe_texture *templ,
						const unsigned *stride,
						struct pipe_buffer *buffer)
{
   return NULL;
}

static void brw_texture_destroy(struct pipe_texture *pt)
{
   struct brw_texture *tex = brw_texture(pt);
   bo_reference(&tex->bo, NULL);
   FREE(pt);
}


static boolean brw_is_format_supported( struct pipe_screen *screen,
					enum pipe_format format,
					enum pipe_texture_target target,
					unsigned tex_usage, 
					unsigned geom_flags )
{
   return translate_tex_format(format) != BRW_SURFACEFORMAT_INVALID;
}


boolean brw_is_texture_referenced_by_bo( struct brw_screen *brw_screen,
                                      struct pipe_texture *texture,
                                      unsigned face, 
                                      unsigned level,
                                      struct brw_winsys_buffer *bo )
{
   struct brw_texture *tex = brw_texture(texture);
   struct brw_surface *surf;
   int i;

   /* XXX: this is subject to false positives if the underlying
    * texture BO is referenced, we can't tell whether the sub-region
    * we care about participates in that.
    */
   if (brw_screen->sws->bo_references( bo, tex->bo ))
      return TRUE;

   /* Find any view on this texture for this face/level and see if it
    * is referenced:
    */
   for (i = 0; i < 2; i++) {
      foreach (surf, &tex->views[i]) {
         if (surf->bo == tex->bo)
            continue;

         if (surf->id.bits.face != face ||
             surf->id.bits.level != level)
            continue;
         
         if (brw_screen->sws->bo_references( bo, surf->bo))
            return TRUE;
      }
   }

   return FALSE;
}



void brw_screen_tex_init( struct brw_screen *brw_screen )
{
   brw_screen->base.is_format_supported = brw_is_format_supported;
   brw_screen->base.texture_create = brw_texture_create;
   brw_screen->base.texture_destroy = brw_texture_destroy;
   brw_screen->base.texture_blanket = brw_texture_blanket;
}
