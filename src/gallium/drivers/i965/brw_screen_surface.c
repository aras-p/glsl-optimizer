
#include "pipe/p_screen.h"
#include "brw_screen.h"


/**
 * Sets up a surface state structure to point at the given region.
 * While it is only used for the front/back buffer currently, it should be
 * usable for further buffers when doing ARB_draw_buffer support.
 */
static void
brw_update_renderbuffer_surface(struct brw_context *brw,
				struct gl_renderbuffer *rb,
				unsigned int unit)
{
   struct brw_winsys_buffer *region_bo = NULL;
   struct intel_renderbuffer *irb = intel_renderbuffer(rb);
   struct intel_region *region = irb ? irb->region : NULL;
   struct {
      unsigned int surface_type;
      unsigned int surface_format;
      unsigned int width, height, pitch, cpp;
      GLubyte color_mask[4];
      GLboolean color_blend;
      uint32_t tiling;
      uint32_t draw_offset;
   } key;

   memset(&key, 0, sizeof(key));

   if (region != NULL) {
      region_bo = region->buffer;

      key.surface_type = BRW_SURFACE_2D;
      switch (irb->texformat->MesaFormat) {
      case PIPE_FORMAT_ARGB8888:
	 key.surface_format = BRW_SURFACEFORMAT_B8G8R8A8_UNORM;
	 break;
      case PIPE_FORMAT_RGB565:
	 key.surface_format = BRW_SURFACEFORMAT_B5G6R5_UNORM;
	 break;
      case PIPE_FORMAT_ARGB1555:
	 key.surface_format = BRW_SURFACEFORMAT_B5G5R5A1_UNORM;
	 break;
      case PIPE_FORMAT_ARGB4444:
	 key.surface_format = BRW_SURFACEFORMAT_B4G4R4A4_UNORM;
	 break;
      default:
	 debug_printf("Bad renderbuffer format: %d\n",
		      irb->texformat->MesaFormat);
	 assert(0);
	 key.surface_format = BRW_SURFACEFORMAT_B8G8R8A8_UNORM;
	 return;
      }
      key.tiling = region->tiling;
      if (brw->intel.intelScreen->driScrnPriv->dri2.enabled) {
	 key.width = rb->Width;
	 key.height = rb->Height;
      } else {
	 key.width = region->width;
	 key.height = region->height;
      }
      key.pitch = region->pitch;
      key.cpp = region->cpp;
      key.draw_offset = region->draw_offset; /* cur 3d or cube face offset */
   } 

   memcpy(key.color_mask, ctx->Color.ColorMask,
	  sizeof(key.color_mask));

   key.color_blend = (!ctx->Color._LogicOpEnabled &&
		      ctx->Color.BlendEnabled);

   brw->sws->bo_unreference(brw->wm.surf_bo[unit]);
   brw->wm.surf_bo[unit] = brw_search_cache(&brw->surface_cache,
					    BRW_SS_SURFACE,
					    &key, sizeof(key),
					    &region_bo, 1,
					    NULL);

   if (brw->wm.surf_bo[unit] == NULL) {
      struct brw_surface_state surf;

      memset(&surf, 0, sizeof(surf));

      surf.ss0.surface_format = key.surface_format;
      surf.ss0.surface_type = key.surface_type;
      if (key.tiling == I915_TILING_NONE) {
	 surf.ss1.base_addr = key.draw_offset;
      } else {
	 uint32_t tile_offset = key.draw_offset % 4096;

	 surf.ss1.base_addr = key.draw_offset - tile_offset;

	 assert(BRW_IS_G4X(brw) || tile_offset == 0);
	 if (BRW_IS_G4X(brw)) {
	    if (key.tiling == I915_TILING_X) {
	       /* Note that the low bits of these fields are missing, so
		* there's the possibility of getting in trouble.
		*/
	       surf.ss5.x_offset = (tile_offset % 512) / key.cpp / 4;
	       surf.ss5.y_offset = tile_offset / 512 / 2;
	    } else {
	       surf.ss5.x_offset = (tile_offset % 128) / key.cpp / 4;
	       surf.ss5.y_offset = tile_offset / 128 / 2;
	    }
	 }
      }

      if (region_bo != NULL)
	 surf.ss1.base_addr += region_bo->offset; /* reloc */

      surf.ss2.width = key.width - 1;
      surf.ss2.height = key.height - 1;
      brw_set_surface_tiling(&surf, key.tiling);
      surf.ss3.pitch = (key.pitch * key.cpp) - 1;

}



struct brw_surface_id {
   unsigned face:3;
   unsigned zslice:13;
   unsigned level:16;
};

static boolean need_linear_view( struct brw_screen *brw_screen,
				 struct brw_texture *brw_texture,
				 unsigned face,
				 unsigned level,
				 unsigned zslice )
{
#if 0
   /* XXX: what about IDGNG?
    */
   if (!BRW_IS_G4X(brw->brw_screen->pci_id))
   {
      struct gl_renderbuffer *rb = ctx->DrawBuffer->_ColorDrawBuffers[i];
      struct intel_renderbuffer *irb = intel_renderbuffer(rb);

      /* The original gen4 hardware couldn't set up WM surfaces pointing
       * at an offset within a tile, which can happen when rendering to
       * anything but the base level of a texture or the +X face/0 depth.
       * This was fixed with the 4 Series hardware.
       *
       * For these original chips, you would have to make the depth and
       * color destination surfaces include information on the texture
       * type, LOD, face, and various limits to use them as a destination.
       *
       * This is easy in Gallium as surfaces are all backed by
       * textures, but there's also a nasty requirement that the depth
       * and the color surfaces all be of the same LOD, which is
       * harder to get around as we can't look at a surface in
       * isolation and decide if it's legal.
       *
       * Instead, end up being pessimistic and say that for i965,
       * ... ??
       */
      if (brw_tex->tiling != I915_TILING_NONE &&
	  (brw_tex_image_offset(brw_tex, face, level, zslize) & 4095)) {
	 if (BRW_DEBUG & DEBUG_VIEW)
	    debug_printf("%s: need surface view for non-aligned tex image\n",
			 __FUNCTION__);
	 return GL_TRUE;
      }
   }
#endif

   /* Tiled 3d textures don't have subsets that look like 2d surfaces:
    */
   
   /* Everything else should be fine to render to in-place:
    */
   return GL_FALSE;
}

/* Look at all texture views and figure out if any of them need to be
 * back-copied into the texture for sampling
 */
void brw_update_texture( struct pipe_screen *screen,
			 struct pipe_texture *texture )
{
   /* currently nothing to do */
}


static struct pipe_surface *create_linear_view( struct brw_screen *brw_screen,
						struct brw_texture *brw_tex,
						struct brw_surface_id id )
{
   
}

static struct pipe_surface *create_in_place_view( struct brw_screen *brw_screen,
						  struct brw_texture *brw_tex,
						  struct brw_surface_id id )
{
   struct brw_surface *surface = CALLOC_STRUCT(brw_surface);
   surface->id = id;
   
}

/* Get a surface which is view into a texture 
 */
struct pipe_surface *brw_get_tex_surface(struct pipe_screen *screen,
					 struct pipe_texture *texture,
					 unsigned face, unsigned level,
					 unsigned zslice,
					 unsigned usage )
{
   struct brw_screen *bscreen = brw_screen(screen);
   struct brw_surface_id id;

   id.face = face;
   id.level = level;
   id.zslice = zslice;

   if (need_linear_view(brw_screen, brw_tex, id)) 
      type = BRW_VIEW_LINEAR;
   else
      type = BRW_VIEW_IN_PLACE;

   
   foreach (surface, texture->views[type]) {
      if (id.value == surface->id.value)
	 return surface;
   }

   switch (type) {
   case BRW_VIEW_LINEAR:
      surface = create_linear_view( texture, id, type );
      break;
   case BRW_VIEW_IN_PLACE:
      surface = create_in_place_view( texture, id, type );
      break;
   default:
      return NULL;
   }

   insert_at_head( texture->views[type], surface );
   return surface;
}


void brw_tex_surface_destroy( struct pipe_surface *surface )
{
}
