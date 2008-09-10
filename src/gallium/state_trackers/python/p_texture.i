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
%nodefaultctor pipe_surface;
%nodefaultctor st_buffer;

%nodefaultdtor pipe_texture;
%nodefaultdtor pipe_surface;
%nodefaultdtor st_buffer;

%ignore pipe_texture::screen;

%ignore pipe_surface::winsys;
%immutable pipe_surface::texture;
%immutable pipe_surface::buffer;

%newobject pipe_texture::get_surface;


%extend pipe_texture {
   
   ~pipe_texture() {
      struct pipe_texture *ptr = $self;
      pipe_texture_reference(&ptr, NULL);
   }
   
   unsigned get_width(unsigned level=0) {
      return $self->width[level];
   }
   
   unsigned get_height(unsigned level=0) {
      return $self->height[level];
   }
   
   unsigned get_depth(unsigned level=0) {
      return $self->depth[level];
   }
   
   unsigned get_nblocksx(unsigned level=0) {
      return $self->nblocksx[level];
   }
   
   unsigned get_nblocksy(unsigned level=0) {
      return $self->nblocksy[level];
   }
   
   /** Get a surface which is a "view" into a texture */
   struct pipe_surface *
   get_surface(unsigned face=0, unsigned level=0, unsigned zslice=0, unsigned usage=0 )
   {
      struct pipe_screen *screen = $self->screen;
      return screen->get_tex_surface(screen, $self, face, level, zslice, usage);
   }
   
};


%extend pipe_surface {
   
   ~pipe_surface() {
      struct pipe_surface *ptr = $self;
      pipe_surface_reference(&ptr, NULL);
   }
   
   // gets mapped to pipe_surface_map automatically
   void * map( unsigned flags );

   // gets mapped to pipe_surface_unmap automatically
   void unmap( void );

   void
   get_tile_raw(unsigned x, unsigned y, unsigned w, unsigned h, char *raw, unsigned stride) {
      pipe_get_tile_raw($self, x, y, w, h, raw, stride);
   }

   void
   put_tile_raw(unsigned x, unsigned y, unsigned w, unsigned h, const char *raw, unsigned stride) {
      pipe_put_tile_raw($self, x, y, w, h, raw, stride);
   }

   void
   get_tile_rgba(unsigned x, unsigned y, unsigned w, unsigned h, float *rgba) {
      pipe_get_tile_rgba($self, x, y, w, h, rgba);
   }

   void
   put_tile_rgba(unsigned x, unsigned y, unsigned w, unsigned h, const float *rgba) {
      pipe_put_tile_rgba($self, x, y, w, h, rgba);
   }

   void
   get_tile_z(unsigned x, unsigned y, unsigned w, unsigned h, unsigned *z) {
      pipe_get_tile_z($self, x, y, w, h, z);
   }

   void
   put_tile_z(unsigned x, unsigned y, unsigned w, unsigned h, const unsigned *z) {
      pipe_put_tile_z($self, x, y, w, h, z);
   }
   
   void
   sample_rgba(float *rgba) {
      st_sample_surface($self, rgba);
   }
   
   unsigned
   compare_tile_rgba(unsigned x, unsigned y, unsigned w, unsigned h, const float *rgba, float tol = 0.0) 
   {
      float *rgba2;
      const float *p1;
      const float *p2;
      unsigned i, j, n;
      
      rgba2 = MALLOC(h*w*4*sizeof(float));
      if(!rgba2)
         return ~0;

      pipe_get_tile_rgba($self, x, y, w, h, rgba2);

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

struct st_buffer {
};

%extend st_buffer {
   
   ~st_buffer() {
      st_buffer_destroy($self);
   }
   
   void write( const char *STRING, unsigned LENGTH, unsigned offset = 0) {
      struct pipe_screen *screen = $self->st_dev->screen;
      char *map;
      
      assert($self->buffer->refcount);
      
      if(offset > $self->buffer->size) {
         PyErr_SetString(PyExc_ValueError, "offset must be smaller than buffer size");
         return;
      }

      if(offset + LENGTH > $self->buffer->size) {
         PyErr_SetString(PyExc_ValueError, "data length must fit inside the buffer");
         return;
      }

      map = pipe_buffer_map(screen, $self->buffer, PIPE_BUFFER_USAGE_CPU_WRITE);
      if(map) {
         memcpy(map + offset, STRING, LENGTH);
         pipe_buffer_unmap(screen, $self->buffer);
      }
   }
};
