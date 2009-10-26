
#include "pipe/p_screen.h"
#include "brw_screen.h"

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
