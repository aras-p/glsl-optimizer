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
#include "util/u_format.h"

#include "brw_screen.h"
#include "brw_defines.h"
#include "brw_structs.h"
#include "brw_winsys.h"
#include "brw_batchbuffer.h"
#include "brw_context.h"
#include "brw_resource.h"


/**
 * Subclass of pipe_transfer
 */
struct brw_transfer
{
   struct pipe_transfer base;

   unsigned offset;
};

static INLINE struct brw_transfer *
brw_transfer(struct pipe_transfer *transfer)
{
   return (struct brw_transfer *)transfer;
}


static GLuint translate_tex_target( unsigned target )
{
   switch (target) {
   case PIPE_TEXTURE_1D: 
      return BRW_SURFACE_1D;

   case PIPE_TEXTURE_2D: 
   case PIPE_TEXTURE_RECT:
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

   case PIPE_FORMAT_L16_UNORM:
      return BRW_SURFACEFORMAT_L16_UNORM;

      /* XXX: Add these to gallium
   case PIPE_FORMAT_I16_UNORM:
      return BRW_SURFACEFORMAT_I16_UNORM;

   case PIPE_FORMAT_A16_UNORM:
      return BRW_SURFACEFORMAT_A16_UNORM; 
      */

   case PIPE_FORMAT_L8A8_UNORM:
      return BRW_SURFACEFORMAT_L8A8_UNORM;

   case PIPE_FORMAT_B5G6R5_UNORM:
      return BRW_SURFACEFORMAT_B5G6R5_UNORM;

   case PIPE_FORMAT_B5G5R5A1_UNORM:
      return BRW_SURFACEFORMAT_B5G5R5A1_UNORM;

   case PIPE_FORMAT_B4G4R4A4_UNORM:
      return BRW_SURFACEFORMAT_B4G4R4A4_UNORM;

   case PIPE_FORMAT_B8G8R8X8_UNORM:
      return BRW_SURFACEFORMAT_R8G8B8X8_UNORM;

   case PIPE_FORMAT_B8G8R8A8_UNORM:
      return BRW_SURFACEFORMAT_B8G8R8A8_UNORM;

   /*
    * Video formats
    */

   case PIPE_FORMAT_YUYV:
      return BRW_SURFACEFORMAT_YCRCB_NORMAL;

   case PIPE_FORMAT_UYVY:
      return BRW_SURFACEFORMAT_YCRCB_SWAPUVY;

   /*
    * Compressed formats.
    */
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

   /*
    * sRGB formats
    */

   case PIPE_FORMAT_A8B8G8R8_SRGB:
      return BRW_SURFACEFORMAT_B8G8R8A8_UNORM_SRGB;

   case PIPE_FORMAT_L8A8_SRGB:
      return BRW_SURFACEFORMAT_L8A8_UNORM_SRGB;

   case PIPE_FORMAT_L8_SRGB:
      return BRW_SURFACEFORMAT_L8_UNORM_SRGB;

   case PIPE_FORMAT_DXT1_SRGB:
      return BRW_SURFACEFORMAT_BC1_UNORM_SRGB;

   /*
    * Depth formats
    */

   case PIPE_FORMAT_Z16_UNORM:
         return BRW_SURFACEFORMAT_I16_UNORM;

   case PIPE_FORMAT_Z24_UNORM_S8_USCALED:
   case PIPE_FORMAT_Z24X8_UNORM:
         return BRW_SURFACEFORMAT_I24X8_UNORM;

   case PIPE_FORMAT_Z32_FLOAT:
         return BRW_SURFACEFORMAT_I32_FLOAT;

      /* XXX: presumably for bump mapping.  Add this to mesa state
       * tracker?
       *
       * XXX: Add flipped versions of these formats to Gallium.
       */
   case PIPE_FORMAT_R8G8_SNORM:
      return BRW_SURFACEFORMAT_R8G8_SNORM;

   case PIPE_FORMAT_R8G8B8A8_SNORM:
      return BRW_SURFACEFORMAT_R8G8B8A8_SNORM;

   default:
      return BRW_SURFACEFORMAT_INVALID;
   }
}


static boolean
brw_texture_get_handle(struct pipe_screen *screen,
                       struct pipe_resource *texture,
                       struct winsys_handle *whandle)
{
   struct brw_screen *bscreen = brw_screen(screen);
   struct brw_texture *tex = brw_texture(texture);
   unsigned stride;

   stride = tex->pitch * tex->cpp;

   return bscreen->sws->bo_get_handle(tex->bo, whandle, stride) == PIPE_OK;
}



static void brw_texture_destroy(struct pipe_screen *screen,
				struct pipe_resource *pt)
{
   struct brw_texture *tex = brw_texture(pt);
   bo_reference(&tex->bo, NULL);
   FREE(pt);
}


/*
 * Transfer functions
 */


static struct pipe_transfer * 
brw_texture_get_transfer(struct pipe_context *context,
                         struct pipe_resource *resource,
                         unsigned level,
                         unsigned usage,
                         const struct pipe_box *box)
{
   struct brw_texture *tex = brw_texture(resource);
   struct pipe_transfer *transfer = CALLOC_STRUCT(pipe_transfer);
   if (transfer == NULL)
      return NULL;

   transfer->resource = resource;
   transfer->level = level;
   transfer->usage = usage;
   transfer->box = *box;
   transfer->stride = tex->pitch * tex->cpp;
   /* FIXME: layer_stride */

   return transfer;
}


static void *
brw_texture_transfer_map(struct pipe_context *pipe,
                 struct pipe_transfer *transfer)
{
   struct pipe_resource *resource = transfer->resource;
   struct brw_texture *tex = brw_texture(transfer->resource);
   struct brw_winsys_screen *sws = brw_screen(pipe->screen)->sws;
   struct pipe_box *box = &transfer->box;
   enum pipe_format format = resource->format;
   unsigned usage = transfer->usage;
   unsigned offset;
   char *map;

   if (resource->target != PIPE_TEXTURE_3D &&
       resource->target != PIPE_TEXTURE_CUBE)
      assert(box->z == 0);
   offset = tex->image_offset[transfer->level][box->z];

   map = sws->bo_map(tex->bo, 
                     BRW_DATA_OTHER,
                     0,
                     tex->bo->size,
                     (usage & PIPE_TRANSFER_WRITE) ? TRUE : FALSE,
                     (usage & 0) ? TRUE : FALSE,
                     (usage & 0) ? TRUE : FALSE);

   if (!map)
      return NULL;

   return map + offset +
      box->y / util_format_get_blockheight(format) * transfer->stride +
      box->x / util_format_get_blockwidth(format) * util_format_get_blocksize(format);
}

static void
brw_texture_transfer_unmap(struct pipe_context *pipe,
                   struct pipe_transfer *transfer)
{
   struct brw_texture *tex = brw_texture(transfer->resource);
   struct brw_winsys_screen *sws = brw_screen(pipe->screen)->sws;

   sws->bo_unmap(tex->bo);
}





struct u_resource_vtbl brw_texture_vtbl = 
{
   brw_texture_get_handle,	      /* get_handle */
   brw_texture_destroy,	      /* resource_destroy */
   brw_texture_get_transfer,	      /* get_transfer */
   u_default_transfer_destroy,	      /* transfer_destroy */
   brw_texture_transfer_map,	      /* transfer_map */
   u_default_transfer_flush_region,   /* transfer_flush_region */
   brw_texture_transfer_unmap,	      /* transfer_unmap */
   u_default_transfer_inline_write    /* transfer_inline_write */
};





struct pipe_resource *
brw_texture_create( struct pipe_screen *screen,
		    const struct pipe_resource *template )
{  
   struct brw_screen *bscreen = brw_screen(screen);
   struct brw_texture *tex;
   enum brw_buffer_type buffer_type;
   enum pipe_error ret;
   GLuint format;
   
   tex = CALLOC_STRUCT(brw_texture);
   if (tex == NULL)
      return NULL;

   tex->b.b = *template;
   tex->b.vtbl = &brw_texture_vtbl;
   pipe_reference_init(&tex->b.b.reference, 1);
   tex->b.b.screen = screen;

   /* XXX: compressed textures need special treatment here
    */
   tex->cpp = util_format_get_blocksize(tex->b.b.format);
   tex->compressed = util_format_is_s3tc(tex->b.b.format);

   make_empty_list(&tex->views[0]);
   make_empty_list(&tex->views[1]);

   /* XXX: No tiling with compressed textures??
    */
   if (tex->compressed == 0 &&
       !bscreen->no_tiling) 
   {
      if (bscreen->gen < 5 &&
	  util_format_is_depth_or_stencil(template->format))
	 tex->tiling = BRW_TILING_Y;
      else
	 tex->tiling = BRW_TILING_X;
   } 
   else {
      tex->tiling = BRW_TILING_NONE;
   }


   if (!brw_texture_layout( bscreen, tex ))
      goto fail;

   
   if (template->bind & (PIPE_BIND_SCANOUT |
                           PIPE_BIND_SHARED)) {
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
   tex->ss.ss0.surface_type = translate_tex_target(tex->b.b.target);

   format = translate_tex_format(tex->b.b.format);
   assert(format != BRW_SURFACEFORMAT_INVALID);
   tex->ss.ss0.surface_format = format;

   /* This is ok for all textures with channel width 8bit or less:
    */
/*    tex->ss.ss0.data_return_format = BRW_SURFACERETURNFORMAT_S1; */


   /* XXX: what happens when tex->bo->offset changes???
    */
   tex->ss.ss1.base_addr = 0; /* reloc */
   tex->ss.ss2.mip_count = tex->b.b.last_level;
   tex->ss.ss2.width = tex->b.b.width0 - 1;
   tex->ss.ss2.height = tex->b.b.height0 - 1;

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
   tex->ss.ss3.depth = tex->b.b.depth0 - 1;

   tex->ss.ss4.min_lod = 0;
 
   if (tex->b.b.target == PIPE_TEXTURE_CUBE) {
      tex->ss.ss0.cube_pos_x = 1;
      tex->ss.ss0.cube_pos_y = 1;
      tex->ss.ss0.cube_pos_z = 1;
      tex->ss.ss0.cube_neg_x = 1;
      tex->ss.ss0.cube_neg_y = 1;
      tex->ss.ss0.cube_neg_z = 1;
   }

   return &tex->b.b;

fail:
   bo_reference(&tex->bo, NULL);
   FREE(tex);
   return NULL;
}


struct pipe_resource * 
brw_texture_from_handle(struct pipe_screen *screen,
                        const struct pipe_resource *template,
                        struct winsys_handle *whandle)
{
   struct brw_screen *bscreen = brw_screen(screen);
   struct brw_texture *tex;
   struct brw_winsys_buffer *buffer;
   unsigned tiling;
   unsigned pitch;
   GLuint format;

   if ((template->target != PIPE_TEXTURE_2D
         && template->target != PIPE_TEXTURE_RECT)  ||
       template->last_level != 0 ||
       template->depth0 != 1)
      return NULL;

   if (util_format_is_s3tc(template->format))
      return NULL;

   tex = CALLOC_STRUCT(brw_texture);
   if (!tex)
      return NULL;

   if (bscreen->sws->bo_from_handle(bscreen->sws, whandle, &pitch, &tiling, &buffer) != PIPE_OK)
      goto fail;

   tex->b.b = *template;
   tex->b.vtbl = &brw_texture_vtbl;
   pipe_reference_init(&tex->b.b.reference, 1);
   tex->b.b.screen = screen;

   /* XXX: cpp vs. blocksize
    */
   tex->cpp = util_format_get_blocksize(tex->b.b.format);
   tex->tiling = tiling;

   make_empty_list(&tex->views[0]);
   make_empty_list(&tex->views[1]);

   if (!brw_texture_layout(bscreen, tex))
      goto fail;

   /* XXX Maybe some more checks? */
   if ((pitch / tex->cpp) < tex->pitch)
      goto fail;

   tex->pitch = pitch / tex->cpp;

   tex->bo = buffer;

   /* fix this warning */
#if 0
   if (tex->size > buffer->size)
      goto fail;
#endif

   tex->ss.ss0.mipmap_layout_mode = BRW_SURFACE_MIPMAPLAYOUT_BELOW;
   tex->ss.ss0.surface_type = translate_tex_target(tex->b.b.target);

   format = translate_tex_format(tex->b.b.format);
   assert(format != BRW_SURFACEFORMAT_INVALID);
   tex->ss.ss0.surface_format = format;

   /* This is ok for all textures with channel width 8bit or less:
    */
/*    tex->ss.ss0.data_return_format = BRW_SURFACERETURNFORMAT_S1; */


   /* XXX: what happens when tex->bo->offset changes???
    */
   tex->ss.ss1.base_addr = 0; /* reloc */
   tex->ss.ss2.mip_count = tex->b.b.last_level;
   tex->ss.ss2.width = tex->b.b.width0 - 1;
   tex->ss.ss2.height = tex->b.b.height0 - 1;

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
   tex->ss.ss3.depth = tex->b.b.depth0 - 1;

   tex->ss.ss4.min_lod = 0;

   return &tex->b.b;

fail:
   FREE(tex);
   return NULL;
}


#if 0
boolean brw_is_format_supported( struct pipe_screen *screen,
				 enum pipe_format format,
				 enum pipe_texture_target target,
				 unsigned sample_count,
				 unsigned tex_usage,
				 unsigned geom_flags )
{
   return translate_tex_format(format) != BRW_SURFACEFORMAT_INVALID;
}
#endif
