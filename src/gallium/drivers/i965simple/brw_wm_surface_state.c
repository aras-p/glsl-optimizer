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

#include "brw_context.h"
#include "brw_state.h"
#include "brw_defines.h"

static unsigned translate_tex_target( enum pipe_texture_target target )
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
      return 0;
   }
}

static unsigned translate_tex_format( enum pipe_format pipe_format )
{
   switch( pipe_format ) {
   case PIPE_FORMAT_L8_UNORM:
      return BRW_SURFACEFORMAT_L8_UNORM;

   case PIPE_FORMAT_I8_UNORM:
      return BRW_SURFACEFORMAT_I8_UNORM;

   case PIPE_FORMAT_A8_UNORM:
      return BRW_SURFACEFORMAT_A8_UNORM;

   case PIPE_FORMAT_A8L8_UNORM:
      return BRW_SURFACEFORMAT_L8A8_UNORM;

   case PIPE_FORMAT_R8G8B8_UNORM:
      assert(0);		/* not supported for sampling */
      return BRW_SURFACEFORMAT_R8G8B8_UNORM;

   case PIPE_FORMAT_B8G8R8A8_UNORM:
      return BRW_SURFACEFORMAT_B8G8R8A8_UNORM;

   case PIPE_FORMAT_R8G8B8A8_UNORM:
      return BRW_SURFACEFORMAT_R8G8B8A8_UNORM;

   case PIPE_FORMAT_R5G6B5_UNORM:
      return BRW_SURFACEFORMAT_B5G6R5_UNORM;

   case PIPE_FORMAT_A1R5G5B5_UNORM:
      return BRW_SURFACEFORMAT_B5G5R5A1_UNORM;

   case PIPE_FORMAT_A4R4G4B4_UNORM:
      return BRW_SURFACEFORMAT_B4G4R4A4_UNORM;

   case PIPE_FORMAT_YCBCR_REV:
      return BRW_SURFACEFORMAT_YCRCB_NORMAL;

   case PIPE_FORMAT_YCBCR:
      return BRW_SURFACEFORMAT_YCRCB_SWAPUVY;
#if 0
   case PIPE_FORMAT_RGB_FXT1:
   case PIPE_FORMAT_RGBA_FXT1:
      return BRW_SURFACEFORMAT_FXT1;
#endif

   case PIPE_FORMAT_Z16_UNORM:
      return BRW_SURFACEFORMAT_I16_UNORM;
#if 0
   case PIPE_FORMAT_RGB_DXT1:
       return BRW_SURFACEFORMAT_DXT1_RGB;

   case PIPE_FORMAT_RGBA_DXT1:
       return BRW_SURFACEFORMAT_BC1_UNORM;

   case PIPE_FORMAT_RGBA_DXT3:
       return BRW_SURFACEFORMAT_BC2_UNORM;

   case PIPE_FORMAT_RGBA_DXT5:
       return BRW_SURFACEFORMAT_BC3_UNORM;

   case PIPE_FORMAT_SRGBA8:
      return BRW_SURFACEFORMAT_R8G8B8A8_UNORM_SRGB;
   case PIPE_FORMAT_SRGB_DXT1:
      return BRW_SURFACEFORMAT_BC1_UNORM_SRGB;
#endif

   default:
      assert(0);
      return 0;
   }
}

static unsigned brw_buffer_offset(struct brw_context *brw,
                                  struct pipe_buffer *buffer)
{
   return brw->winsys->get_buffer_offset(brw->winsys,
                                         buffer,
                                         0);
}

static
void brw_update_texture_surface( struct brw_context *brw,
				 unsigned unit )
{
   const struct brw_texture *tObj = brw->attribs.Texture[unit];
   struct brw_surface_state surf;

   memset(&surf, 0, sizeof(surf));

   surf.ss0.mipmap_layout_mode = BRW_SURFACE_MIPMAPLAYOUT_BELOW;
   surf.ss0.surface_type = translate_tex_target(tObj->base.target);
   surf.ss0.surface_format = translate_tex_format(tObj->base.format);

   /* This is ok for all textures with channel width 8bit or less:
    */
/*    surf.ss0.data_return_format = BRW_SURFACERETURNFORMAT_S1; */

   /* Updated in emit_reloc */
   surf.ss1.base_addr = brw_buffer_offset( brw, tObj->buffer );

   surf.ss2.mip_count = tObj->base.last_level;
   surf.ss2.width = tObj->base.width[0] - 1;
   surf.ss2.height = tObj->base.height[0] - 1;

   surf.ss3.tile_walk = BRW_TILEWALK_XMAJOR;
   surf.ss3.tiled_surface = 0; /* always zero */
   surf.ss3.pitch = tObj->stride - 1;
   surf.ss3.depth = tObj->base.depth[0] - 1;

   surf.ss4.min_lod = 0;

   if (tObj->base.target == PIPE_TEXTURE_CUBE) {
      surf.ss0.cube_pos_x = 1;
      surf.ss0.cube_pos_y = 1;
      surf.ss0.cube_pos_z = 1;
      surf.ss0.cube_neg_x = 1;
      surf.ss0.cube_neg_y = 1;
      surf.ss0.cube_neg_z = 1;
   }

   brw->wm.bind.surf_ss_offset[unit + 1] =
      brw_cache_data( &brw->cache[BRW_SS_SURFACE], &surf );
}



#define OFFSET(TYPE, FIELD) ( (unsigned)&(((TYPE *)0)->FIELD) )


static void upload_wm_surfaces(struct brw_context *brw )
{
   unsigned i;

   {
      struct brw_surface_state surf;

      /* BRW_NEW_FRAMEBUFFER
       */
      struct pipe_surface *pipe_surface = brw->attribs.FrameBuffer.cbufs[0];/*fixme*/

      memset(&surf, 0, sizeof(surf));

      if (pipe_surface != NULL) {
	 if (pipe_surface->block.size == 4)
	    surf.ss0.surface_format = BRW_SURFACEFORMAT_B8G8R8A8_UNORM;
	 else
	    surf.ss0.surface_format = BRW_SURFACEFORMAT_B5G6R5_UNORM;

	 surf.ss0.surface_type = BRW_SURFACE_2D;

	 surf.ss1.base_addr = brw_buffer_offset( brw, pipe_surface->buffer );

	 surf.ss2.width = pipe_surface->width - 1;
	 surf.ss2.height = pipe_surface->height - 1;
	 surf.ss3.tile_walk = BRW_TILEWALK_XMAJOR;
	 surf.ss3.tiled_surface = 0;
	 surf.ss3.pitch = pipe_surface->stride - 1;
      } else {
	 surf.ss0.surface_format = BRW_SURFACEFORMAT_B8G8R8A8_UNORM;
	 surf.ss0.surface_type = BRW_SURFACE_NULL;
      }

      /* BRW_NEW_BLEND */
      surf.ss0.color_blend = (!brw->attribs.Blend->logicop_enable &&
			      brw->attribs.Blend->blend_enable);


      surf.ss0.writedisable_red =   !(brw->attribs.Blend->colormask & PIPE_MASK_R);
      surf.ss0.writedisable_green = !(brw->attribs.Blend->colormask & PIPE_MASK_G);
      surf.ss0.writedisable_blue =  !(brw->attribs.Blend->colormask & PIPE_MASK_B);
      surf.ss0.writedisable_alpha = !(brw->attribs.Blend->colormask & PIPE_MASK_A);




      brw->wm.bind.surf_ss_offset[0] = brw_cache_data( &brw->cache[BRW_SS_SURFACE], &surf );

      brw->wm.nr_surfaces = 1;
   }


   /* BRW_NEW_TEXTURE
    */
   for (i = 0; i < brw->num_textures && i < brw->num_samplers; i++) {
      const struct brw_texture *texUnit = brw->attribs.Texture[i];

      if (texUnit &&
	  texUnit->base.refcount/*(texUnit->refcount > 0) == really used */) {

	 brw_update_texture_surface(brw, i);

	 brw->wm.nr_surfaces = i+2;
      }
      else {
	 brw->wm.bind.surf_ss_offset[i+1] = 0;
      }
   }

   brw->wm.bind_ss_offset = brw_cache_data( &brw->cache[BRW_SS_SURF_BIND],
					    &brw->wm.bind );
}


/* KW: Will find a different way to acheive this, see for example the
 * state caches with relocs in the i915 swz driver.
 */
#if 0
static void emit_reloc_wm_surfaces(struct brw_context *brw)
{
   int unit;

   if (brw->state.draw_region != NULL) {
      /* Emit framebuffer relocation */
      dri_emit_reloc(brw_cache_buffer(brw, BRW_SS_SURFACE),
		     DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_READ | DRM_BO_FLAG_WRITE,
		     0,
		     brw->wm.bind.surf_ss_offset[0] +
		     offsetof(struct brw_surface_state, ss1),
		     brw->state.draw_region->buffer);
   }

   /* Emit relocations for texture buffers */
   for (unit = 0; unit < BRW_MAX_TEX_UNIT; unit++) {
      struct gl_texture_unit *texUnit = &brw->attribs.Texture->Unit[unit];
      struct gl_texture_object *tObj = texUnit->_Current;
      struct intel_texture_object *intelObj = intel_texture_object(tObj);

      if (texUnit->_ReallyEnabled && intelObj->mt != NULL) {
	 dri_emit_reloc(brw_cache_buffer(brw, BRW_SS_SURFACE),
			DRM_BO_FLAG_MEM_TT | DRM_BO_FLAG_READ,
			0,
			brw->wm.bind.surf_ss_offset[unit + 1] +
			offsetof(struct brw_surface_state, ss1),
			intelObj->mt->region->buffer);
      }
   }
}
#endif

const struct brw_tracked_state brw_wm_surfaces = {
   .dirty = {
      .brw = (BRW_NEW_FRAMEBUFFER |
	      BRW_NEW_BLEND |
	      BRW_NEW_TEXTURE),
      .cache = 0
   },
   .update = upload_wm_surfaces,
};
