 /**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/**
 * @file
 * SWIG interface definion for Gallium types.
 *
 * @author Jose Fonseca <jrfonseca@tungstengraphics.com>
 */


%nodefaultctor pipe_resource;
%nodefaultctor st_surface;

%nodefaultdtor pipe_resource;
%nodefaultdtor st_surface;

%ignore pipe_resource::screen;

%immutable st_surface::texture;
%immutable st_surface::level;
%immutable st_surface::layer;

%newobject pipe_resource::get_surface;

/* Avoid naming conflict with p_inlines.h's pipe_buffer_read/write */ 
%rename(read) read_; 
%rename(write) write_; 

%extend pipe_resource {

   ~pipe_resource() {
      struct pipe_resource *ptr = $self;
      pipe_resource_reference(&ptr, NULL);
   }

   unsigned get_width(unsigned level=0) {
      return u_minify($self->width0, level);
   }

   unsigned get_height(unsigned level=0) {
      return u_minify($self->height0, level);
   }

   unsigned get_depth(unsigned level=0) {
      return u_minify($self->depth0, level);
   }

   /** Get a surface which is a "view" into a texture */
   struct st_surface *
   get_surface(unsigned level=0, unsigned layer=0)
   {
      struct st_surface *surface;

      if(level > $self->last_level)
         SWIG_exception(SWIG_ValueError, "level out of bounds");
      if(layer >= ($self->target == PIPE_TEXTURE_3D ?
                         u_minify($self->depth0, level) : $self->depth0))
         SWIG_exception(SWIG_ValueError, "layer out of bounds");

      surface = CALLOC_STRUCT(st_surface);
      if(!surface)
         return NULL;

      pipe_resource_reference(&surface->texture, $self);
      surface->level = level;
      surface->layer = layer;

      return surface;

   fail:
      return NULL;
   }

   unsigned __len__(void) 
   {
      assert($self->target == PIPE_BUFFER);
      assert(p_atomic_read(&$self->reference.count) > 0);
      return $self->width0;
   }

};

struct st_surface
{
   %immutable;

   struct pipe_resource *texture;
   unsigned level;
   unsigned layer;

};

%extend st_surface {

   %immutable;

   unsigned format;
   unsigned width;
   unsigned height;

   ~st_surface() {
      pipe_resource_reference(&$self->texture, NULL);
      FREE($self);
   }


};

%{
   static enum pipe_format
   st_surface_format_get(struct st_surface *surface)
   {
      return surface->texture->format;
   }
   
   static unsigned
   st_surface_width_get(struct st_surface *surface)
   {
      return u_minify(surface->texture->width0, surface->level);
   }
   
   static unsigned
   st_surface_height_get(struct st_surface *surface)
   {
      return u_minify(surface->texture->height0, surface->level);
   }
%}
