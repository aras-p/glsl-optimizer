/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

#ifndef SVGA_TEXTURE_H
#define SVGA_TEXTURE_H


#include "pipe/p_compiler.h"
#include "pipe/p_state.h"
#include "util/u_inlines.h"
#include "util/u_memory.h"
#include "util/u_transfer.h"
#include "svga_screen_cache.h"

struct pipe_context;
struct pipe_screen;
struct svga_context;
struct svga_winsys_surface;
enum SVGA3dSurfaceFormat;


#define SVGA_MAX_TEXTURE_LEVELS 16


extern struct u_resource_vtbl svga_texture_vtbl;


struct svga_texture 
{
   struct u_resource b;

   boolean defined[6][SVGA_MAX_TEXTURE_LEVELS];
   
   struct svga_sampler_view *cached_view;

   unsigned view_age[SVGA_MAX_TEXTURE_LEVELS];
   unsigned age;

   boolean views_modified;

   /**
    * Creation key for the host surface handle.
    * 
    * This structure describes all the host surface characteristics so that it 
    * can be looked up in cache, since creating a host surface is often a slow
    * operation.
    */
   struct svga_host_surface_cache_key key;

   /**
    * Handle for the host side surface.
    *
    * This handle is owned by this texture. Views should hold on to a reference
    * to this texture and never destroy this handle directly.
    */
   struct svga_winsys_surface *handle;

   unsigned size;  /**< Approximate size in bytes */

   /** array indexed by cube face or 3D/array slice, one bit per mipmap level */
   ushort *rendered_to;
};



/* Note this is only used for texture (not buffer) transfers:
 */
struct svga_transfer
{
   struct pipe_transfer base;

   unsigned face;

   struct svga_winsys_buffer *hwbuf;

   /* Height of the hardware buffer in pixel blocks */
   unsigned hw_nblocksy;

   /* Temporary malloc buffer when we can't allocate a hardware buffer
    * big enough */
   void *swbuf;

   boolean use_direct_map;
};


static INLINE struct svga_texture *svga_texture( struct pipe_resource *resource )
{
   struct svga_texture *tex = (struct svga_texture *)resource;
   assert(tex == NULL || tex->b.vtbl == &svga_texture_vtbl);
   return tex;
}


static INLINE struct svga_transfer *
svga_transfer(struct pipe_transfer *transfer)
{
   assert(transfer);
   return (struct svga_transfer *)transfer;
}


/**
 * Increment the age of a view into a texture
 * This is used to track updates to textures when we draw into
 * them via a surface.
 */
static INLINE void
svga_age_texture_view(struct svga_texture *tex, unsigned level)
{
   assert(level < Elements(tex->view_age));
   tex->view_age[level] = ++(tex->age);
}


/**
 * Mark the given texture face/level as being defined.
 */
static INLINE void
svga_define_texture_level(struct svga_texture *tex,
                          unsigned face,unsigned level)
{
   assert(face < Elements(tex->defined));
   assert(level < Elements(tex->defined[0]));
   tex->defined[face][level] = TRUE;
}


static INLINE bool
svga_is_texture_level_defined(const struct svga_texture *tex,
                              unsigned face, unsigned level)
{
   assert(face < Elements(tex->defined));
   assert(level < Elements(tex->defined[0]));
   return tex->defined[face][level];
}


/** For debugging, check that face and level are legal */
static inline void
check_face_level(const struct svga_texture *tex,
                 unsigned face, unsigned level)
{
   if (tex->b.b.target == PIPE_TEXTURE_CUBE) {
      assert(face < 6);
   }
   else if (tex->b.b.target == PIPE_TEXTURE_3D) {
      assert(face < tex->b.b.depth0);
   }
   else {
      assert(face < tex->b.b.array_size);
   }

   assert(level < 8 * sizeof(tex->rendered_to[0]));
}


static INLINE void
svga_set_texture_rendered_to(struct svga_texture *tex,
                             unsigned face, unsigned level)
{
   check_face_level(tex, face, level);
   tex->rendered_to[face] |= 1 << level;
}


static INLINE void
svga_clear_texture_rendered_to(struct svga_texture *tex,
                               unsigned face, unsigned level)
{
   check_face_level(tex, face, level);
   tex->rendered_to[face] &= ~(1 << level);
}


static INLINE boolean
svga_was_texture_rendered_to(const struct svga_texture *tex,
                             unsigned face, unsigned level)
{
   check_face_level(tex, face, level);
   return !!(tex->rendered_to[face] & (1 << level));
}


struct pipe_resource *
svga_texture_create(struct pipe_screen *screen,
                    const struct pipe_resource *template);

struct pipe_resource *
svga_texture_from_handle(struct pipe_screen * screen,
			const struct pipe_resource *template,
			struct winsys_handle *whandle);




#endif /* SVGA_TEXTURE_H */
