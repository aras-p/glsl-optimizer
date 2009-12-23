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


%nodefaultctor pipe_texture;
%nodefaultctor st_surface;
%nodefaultctor pipe_buffer;

%nodefaultdtor pipe_texture;
%nodefaultdtor st_surface;
%nodefaultdtor pipe_buffer;

%ignore pipe_texture::screen;

%immutable st_surface::texture;
%immutable st_surface::face;
%immutable st_surface::level;
%immutable st_surface::zslice;

%newobject pipe_texture::get_surface;


%extend pipe_texture {
   
   ~pipe_texture() {
      struct pipe_texture *ptr = $self;
      pipe_texture_reference(&ptr, NULL);
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
   get_surface(unsigned face=0, unsigned level=0, unsigned zslice=0)
   {
      struct st_surface *surface;
      
      if(face >= ($self->target == PIPE_TEXTURE_CUBE ? 6U : 1U))
         SWIG_exception(SWIG_ValueError, "face out of bounds");
      if(level > $self->last_level)
         SWIG_exception(SWIG_ValueError, "level out of bounds");
      if(zslice >= u_minify($self->depth0, level))
         SWIG_exception(SWIG_ValueError, "zslice out of bounds");
      
      surface = CALLOC_STRUCT(st_surface);
      if(!surface)
         return NULL;
      
      pipe_texture_reference(&surface->texture, $self);
      surface->face = face;
      surface->level = level;
      surface->zslice = zslice;
      
      return surface;

   fail:
      return NULL;
   }
   
};

struct st_surface
{
   %immutable;
   
   struct pipe_texture *texture;
   unsigned face;
   unsigned level;
   unsigned zslice;
   
};

%extend st_surface {
   
   %immutable;
   
   unsigned format;
   unsigned width;
   unsigned height;
   
   ~st_surface() {
      pipe_texture_reference(&$self->texture, NULL);
      FREE($self);
   }
   
   %cstring_output_allocate_size(char **STRING, int *LENGTH, free(*$1));
   void get_tile_raw(unsigned x, unsigned y, unsigned w, unsigned h, char **STRING, int *LENGTH)
   {
      struct pipe_texture *texture = $self->texture;
      struct pipe_screen *screen = texture->screen;
      struct pipe_transfer *transfer;
      unsigned stride;

      stride = util_format_get_stride(texture->format, w);
      *LENGTH = util_format_get_nblocksy(texture->format, h) * stride;
      *STRING = (char *) malloc(*LENGTH);
      if(!*STRING)
         return;

      transfer = screen->get_tex_transfer(screen,
                                          $self->texture,
                                          $self->face,
                                          $self->level,
                                          $self->zslice,
                                          PIPE_TRANSFER_READ,
                                          x, y, w, h);
      if(transfer) {
         pipe_get_tile_raw(transfer, 0, 0, w, h, *STRING, stride);
         screen->tex_transfer_destroy(transfer);
      }
   }

   %cstring_input_binary(const char *STRING, unsigned LENGTH);
   void put_tile_raw(unsigned x, unsigned y, unsigned w, unsigned h, const char *STRING, unsigned LENGTH, unsigned stride = 0)
   {
      struct pipe_texture *texture = $self->texture;
      struct pipe_screen *screen = texture->screen;
      struct pipe_transfer *transfer;
     
      if(stride == 0)
         stride = util_format_get_stride(texture->format, w);
      
      if(LENGTH < util_format_get_nblocksy(texture->format, h) * stride)
         SWIG_exception(SWIG_ValueError, "offset must be smaller than buffer size");
         
      transfer = screen->get_tex_transfer(screen,
                                          $self->texture,
                                          $self->face,
                                          $self->level,
                                          $self->zslice,
                                          PIPE_TRANSFER_WRITE,
                                          x, y, w, h);
      if(!transfer)
         SWIG_exception(SWIG_MemoryError, "couldn't initiate transfer");
         
      pipe_put_tile_raw(transfer, 0, 0, w, h, STRING, stride);
      screen->tex_transfer_destroy(transfer);

   fail:
      return;
   }

   void
   get_tile_rgba(unsigned x, unsigned y, unsigned w, unsigned h, float *rgba) 
   {
      struct pipe_screen *screen = $self->texture->screen;
      struct pipe_transfer *transfer;
      transfer = screen->get_tex_transfer(screen,
                                          $self->texture,
                                          $self->face,
                                          $self->level,
                                          $self->zslice,
                                          PIPE_TRANSFER_READ,
                                          x, y, w, h);
      if(transfer) {
         pipe_get_tile_rgba(transfer, 0, 0, w, h, rgba);
         screen->tex_transfer_destroy(transfer);
      }
   }

   void
   put_tile_rgba(unsigned x, unsigned y, unsigned w, unsigned h, const float *rgba)
   {
      struct pipe_screen *screen = $self->texture->screen;
      struct pipe_transfer *transfer;
      transfer = screen->get_tex_transfer(screen,
                                          $self->texture,
                                          $self->face,
                                          $self->level,
                                          $self->zslice,
                                          PIPE_TRANSFER_WRITE,
                                          x, y, w, h);
      if(transfer) {
         pipe_put_tile_rgba(transfer, 0, 0, w, h, rgba);
         screen->tex_transfer_destroy(transfer);
      }
   }

   %cstring_output_allocate_size(char **STRING, int *LENGTH, free(*$1));
   void
   get_tile_rgba8(unsigned x, unsigned y, unsigned w, unsigned h, char **STRING, int *LENGTH) 
   {
      struct pipe_screen *screen = $self->texture->screen;
      struct pipe_transfer *transfer;
      float *rgba;
      unsigned char *rgba8;
      unsigned i, j, k;

      *LENGTH = 0;
      *STRING = NULL;
      
      if (!$self)
         return;

      *LENGTH = h*w*4;
      *STRING = (char *) malloc(*LENGTH);
      if(!*STRING)
         return;
      
      rgba = malloc(h*w*4*sizeof(float));
      if(!rgba)
         return;
      
      rgba8 = (unsigned char *) *STRING;

      transfer = screen->get_tex_transfer(screen,
                                          $self->texture,
                                          $self->face,
                                          $self->level,
                                          $self->zslice,
                                          PIPE_TRANSFER_READ,
                                          x, y,
                                          w, h);
      if(transfer) {
         pipe_get_tile_rgba(transfer, 0, 0, w, h, rgba);
         for(j = 0; j < h; ++j) {
            for(i = 0; i < w; ++i)
               for(k = 0; k <4; ++k)
                  rgba8[j*w*4 + i*4 + k] = float_to_ubyte(rgba[j*w*4 + i*4 + k]);
         }
         screen->tex_transfer_destroy(transfer);
      }
      
      free(rgba);
   }

   void
   get_tile_z(unsigned x, unsigned y, unsigned w, unsigned h, unsigned *z)
   {
      struct pipe_screen *screen = $self->texture->screen;
      struct pipe_transfer *transfer;
      transfer = screen->get_tex_transfer(screen,
                                          $self->texture,
                                          $self->face,
                                          $self->level,
                                          $self->zslice,
                                          PIPE_TRANSFER_READ,
                                          x, y, w, h);
      if(transfer) {
         pipe_get_tile_z(transfer, 0, 0, w, h, z);
         screen->tex_transfer_destroy(transfer);
      }
   }

   void
   put_tile_z(unsigned x, unsigned y, unsigned w, unsigned h, const unsigned *z)
   {
      struct pipe_screen *screen = $self->texture->screen;
      struct pipe_transfer *transfer;
      transfer = screen->get_tex_transfer(screen,
                                          $self->texture,
                                          $self->face,
                                          $self->level,
                                          $self->zslice,
                                          PIPE_TRANSFER_WRITE,
                                          x, y, w, h);
      if(transfer) {
         pipe_put_tile_z(transfer, 0, 0, w, h, z);
         screen->tex_transfer_destroy(transfer);
      }
   }
   
   void
   sample_rgba(float *rgba) {
      st_sample_surface($self, rgba);
   }
   
   unsigned
   compare_tile_rgba(unsigned x, unsigned y, unsigned w, unsigned h, const float *rgba, float tol = 0.0) 
   {
      struct pipe_screen *screen = $self->texture->screen;
      struct pipe_transfer *transfer;
      float *rgba2;
      const float *p1;
      const float *p2;
      unsigned i, j, n;
      
      rgba2 = MALLOC(h*w*4*sizeof(float));
      if(!rgba2)
         return ~0;

      transfer = screen->get_tex_transfer(screen,
                                          $self->texture,
                                          $self->face,
                                          $self->level,
                                          $self->zslice,
                                          PIPE_TRANSFER_READ,
                                          x, y, w, h);
      if(!transfer) {
         FREE(rgba2);
         return ~0;
      }

      pipe_get_tile_rgba(transfer, 0, 0, w, h, rgba2);
      screen->tex_transfer_destroy(transfer);

      p1 = rgba;
      p2 = rgba2;
      n = 0;
      for(i = h*w; i; --i) {
         unsigned differs = 0;
         for(j = 4; j; --j) {
            float delta = *p2++ - *p1++;
            if (delta < -tol || delta > tol)
                differs = 1;
         }
         n += differs;
      }
      
      FREE(rgba2);
      
      return n;
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

/* Avoid naming conflict with p_inlines.h's pipe_buffer_read/write */ 
%rename(read) read_; 
%rename(write) write_; 

%extend pipe_buffer {
   
   ~pipe_buffer() {
      struct pipe_buffer *ptr = $self;
      pipe_buffer_reference(&ptr, NULL);
   }
   
   unsigned __len__(void) 
   {
      assert(p_atomic_read(&$self->reference.count) > 0);
      return $self->size;
   }
   
   %cstring_output_allocate_size(char **STRING, int *LENGTH, free(*$1));
   void read_(char **STRING, int *LENGTH)
   {
      struct pipe_screen *screen = $self->screen;
      
      assert(p_atomic_read(&$self->reference.count) > 0);
      
      *LENGTH = $self->size;
      *STRING = (char *) malloc($self->size);
      if(!*STRING)
         return;
      
      pipe_buffer_read(screen, $self, 0, $self->size, *STRING);
   }
   
   %cstring_input_binary(const char *STRING, unsigned LENGTH);
   void write_(const char *STRING, unsigned LENGTH, unsigned offset = 0) 
   {
      struct pipe_screen *screen = $self->screen;
      
      assert(p_atomic_read(&$self->reference.count) > 0);
      
      if(offset > $self->size)
         SWIG_exception(SWIG_ValueError, "offset must be smaller than buffer size");

      if(offset + LENGTH > $self->size)
         SWIG_exception(SWIG_ValueError, "data length must fit inside the buffer");

      pipe_buffer_write(screen, $self, offset, LENGTH, STRING);

fail:
      return;
   }
};
