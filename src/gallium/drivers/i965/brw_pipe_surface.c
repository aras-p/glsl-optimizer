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
#include "util/u_math.h"

#include "pipe/p_screen.h"
#include "brw_screen.h"
#include "brw_context.h"
#include "brw_defines.h"
#include "brw_resource.h"
#include "brw_winsys.h"

enum {
   BRW_VIEW_LINEAR,
   BRW_VIEW_IN_PLACE
};


static boolean need_linear_view( struct brw_screen *brw_screen,
				 struct brw_texture *brw_texture,
				 union brw_surface_id id,
				 unsigned usage )
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
void brw_update_texture( struct brw_screen *brw_screen,
			 struct brw_texture *tex )
{
   /* currently nothing to do */
}


/* Create a new surface with linear layout to serve as a render-target
 * where it would be illegal (perhaps due to tiling constraints) to do
 * this in-place.
 * 
 * Currently not implemented, not sure if it's needed.
 */
static struct brw_surface *create_linear_view( struct brw_screen *brw_screen,
                                               struct pipe_context *pipe,
					       struct brw_texture *tex,
					       union brw_surface_id id,
					       unsigned usage )
{
   return NULL;
}


/* Create a pipe_surface that just points directly into the existing
 * texture's storage.
 */
static struct brw_surface *create_in_place_view( struct brw_screen *brw_screen,
                                                 struct pipe_context *pipe,
                                                 struct brw_texture *tex,
                                                 union brw_surface_id id,
                                                 unsigned usage )
{
   struct brw_surface *surface;

   surface = CALLOC_STRUCT(brw_surface);
   if (surface == NULL)
      return NULL;

   pipe_reference_init(&surface->base.reference, 1);

   /* XXX: ignoring render-to-slice-of-3d-texture
    */
   assert(tex->b.b.target != PIPE_TEXTURE_3D || id.bits.layer == 0);

   surface->base.context = pipe;
   surface->base.format = tex->b.b.format;
   surface->base.width = u_minify(tex->b.b.width0, id.bits.level);
   surface->base.height = u_minify(tex->b.b.height0, id.bits.level);
   surface->base.usage = usage;
   surface->base.u.tex.first_layer = id.bits.layer;
   surface->base.u.tex.last_layer = surface->base.u.tex.first_layer;
   surface->base.u.tex.level = id.bits.level;
   surface->id = id;
   surface->offset = tex->image_offset[id.bits.level][id.bits.layer];
   surface->cpp = tex->cpp;
   surface->pitch = tex->pitch;
   surface->tiling = tex->tiling;

   bo_reference( &surface->bo, tex->bo );
   pipe_resource_reference( &surface->base.texture, &tex->b.b );

   surface->ss.ss0.surface_format = tex->ss.ss0.surface_format;
   surface->ss.ss0.surface_type = BRW_SURFACE_2D;

   if (tex->tiling == BRW_TILING_NONE) {
      surface->ss.ss1.base_addr = surface->offset;
   } else {
      uint32_t tile_offset = surface->offset % 4096;

      surface->ss.ss1.base_addr = surface->offset - tile_offset;

      if (tex->tiling == BRW_TILING_X) {
	/* Note that the low bits of these fields are missing, so
	 * there's the possibility of getting in trouble.
	 */
	surface->ss.ss5.x_offset = (tile_offset % 512) / tex->cpp / 4;
	surface->ss.ss5.y_offset = tile_offset / 512 / 2;
      } else {
	surface->ss.ss5.x_offset = (tile_offset % 128) / tex->cpp / 4;
	    surface->ss.ss5.y_offset = tile_offset / 128 / 2;
      }
   }

#if 0
   if (region_bo != NULL)
      surface->ss.ss1.base_addr += region_bo->offset; /* reloc */
#endif

   surface->ss.ss2.width = surface->base.width - 1;
   surface->ss.ss2.height = surface->base.height - 1;
   surface->ss.ss3.tiled_surface = tex->ss.ss3.tiled_surface;
   surface->ss.ss3.tile_walk = tex->ss.ss3.tile_walk;
   surface->ss.ss3.pitch = tex->ss.ss3.pitch;

   return surface;
}

/* Get a surface which is view into a texture 
 */
static struct pipe_surface *brw_create_surface(struct pipe_context *pipe,
                                               struct pipe_resource *pt,
                                               const struct pipe_surface *surf_tmpl)
{
   struct brw_texture *tex = brw_texture(pt);
   struct brw_screen *bscreen = brw_screen(pipe->screen);
   struct brw_surface *surface;
   union brw_surface_id id;
   int type;

   assert(surf_tmpl->u.tex.first_layer == surf_tmpl->u.tex.last_layer);
   id.bits.level = surf_tmpl->u.tex.level;
   id.bits.layer = surf_tmpl->u.tex.first_layer;

   if (need_linear_view(bscreen, tex, id, surf_tmpl->usage))
      type = BRW_VIEW_LINEAR;
   else
      type = BRW_VIEW_IN_PLACE;

   
   foreach (surface, &tex->views[type]) {
      if (id.value == surface->id.value)
	 return &surface->base;
   }

   switch (type) {
   case BRW_VIEW_LINEAR:
      surface = create_linear_view( bscreen, pipe, tex, id, surf_tmpl->usage );
      break;
   case BRW_VIEW_IN_PLACE:
      surface = create_in_place_view( bscreen, pipe, tex, id, surf_tmpl->usage );
      break;
   }

   insert_at_head( &tex->views[type], surface );
   return &surface->base;
}


static void brw_surface_destroy( struct pipe_context *pipe,
                                 struct pipe_surface *surf )
{
   struct brw_surface *surface = brw_surface(surf);

   /* Unreference texture, shared buffer:
    */
   remove_from_list(surface);
   bo_reference(&surface->bo, NULL);
   pipe_resource_reference( &surface->base.texture, NULL );

   FREE(surface);
}


void brw_pipe_surface_init( struct brw_context *brw )
{
   brw->base.create_surface = brw_create_surface;
   brw->base.surface_destroy = brw_surface_destroy;
}
