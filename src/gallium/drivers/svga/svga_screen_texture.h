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
#include "svga_screen_cache.h"

struct pipe_context;
struct pipe_screen;
struct svga_context;
struct svga_winsys_surface;
enum SVGA3dSurfaceFormat;


#define SVGA_MAX_TEXTURE_LEVELS 16


/**
 * A sampler's view into a texture
 *
 * We currently cache one sampler view on
 * the texture and in there by holding a reference
 * from the texture to the sampler view.
 *
 * Because of this we can not hold a refernce to the
 * texture from the sampler view. So the user
 * of the sampler views must make sure that the
 * texture has a reference take for as long as
 * the sampler view is refrenced.
 *
 * Just unreferencing the sampler_view before the
 * texture is enough.
 */
struct svga_sampler_view
{
   struct pipe_reference reference;

   struct pipe_texture *texture;

   int min_lod;
   int max_lod;

   unsigned age;

   struct svga_host_surface_cache_key key;
   struct svga_winsys_surface *handle;
};


struct svga_texture 
{
   struct pipe_texture base;

   boolean defined[6][PIPE_MAX_TEXTURE_LEVELS];
   
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
};


struct svga_surface
{
   struct pipe_surface base;

   struct svga_host_surface_cache_key key;
   struct svga_winsys_surface *handle;

   unsigned real_face;
   unsigned real_level;
   unsigned real_zslice;

   boolean dirty;
};


struct svga_transfer
{
   struct pipe_transfer base;

   struct svga_winsys_buffer *hwbuf;

   /* Height of the hardware buffer in pixel blocks */
   unsigned hw_nblocksy;

   /* Temporary malloc buffer when we can't allocate a hardware buffer
    * big enough */
   void *swbuf;
};


static INLINE struct svga_texture *
svga_texture(struct pipe_texture *texture)
{
   return (struct svga_texture *)texture;
}

static INLINE struct svga_surface *
svga_surface(struct pipe_surface *surface)
{
   assert(surface);
   return (struct svga_surface *)surface;
}

static INLINE struct svga_transfer *
svga_transfer(struct pipe_transfer *transfer)
{
   assert(transfer);
   return (struct svga_transfer *)transfer;
}

extern struct svga_sampler_view *
svga_get_tex_sampler_view(struct pipe_context *pipe,
                          struct pipe_texture *pt,
                          unsigned min_lod, unsigned max_lod);

void
svga_validate_sampler_view(struct svga_context *svga, struct svga_sampler_view *v);

void
svga_destroy_sampler_view_priv(struct svga_sampler_view *v);

static INLINE void
svga_sampler_view_reference(struct svga_sampler_view **ptr, struct svga_sampler_view *v)
{
   struct svga_sampler_view *old = *ptr;

   if (pipe_reference(&(*ptr)->reference, &v->reference))
      svga_destroy_sampler_view_priv(old);
   *ptr = v;
}

extern void
svga_propagate_surface(struct pipe_context *pipe, struct pipe_surface *surf);

extern boolean
svga_surface_needs_propagation(struct pipe_surface *surf);

extern void
svga_screen_init_texture_functions(struct pipe_screen *screen);

enum SVGA3dSurfaceFormat
svga_translate_format(enum pipe_format format);

enum SVGA3dSurfaceFormat
svga_translate_format_render(enum pipe_format format);


#endif /* SVGA_TEXTURE_H */
